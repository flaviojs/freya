// Copyright (c) Freya Development Team - Licensed under GNU GPL
// For more information, see License.txt in the main folder

#include <config.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <mmo.h>
#include "../common/malloc.h"
#include "../common/socket.h"

#include "char.h"
#include "inter.h"
#include "int_guild.h"
#include "int_storage.h"

#ifdef TXT_ONLY
char guild_txt[1024] = "save/guild.txt";
char castle_txt[1024] = "save/castle.txt";
#endif /* TXT_ONLY */

static struct guild *guild_pt;
static struct guild *guild_pt2;
static struct guild_castle castle_db[MAX_GUILDCASTLE];

static int guild_newid = 10000;

static int guild_exp[100];

int mapif_parse_GuildLeave(int fd, int guild_id, int account_id, int char_id, int flag, const char *mes);
int mapif_guild_broken(int guild_id, int flag);
int guild_check_empty(struct guild *g);
int guild_calcinfo(struct guild *g);
int mapif_guild_basicinfochanged(int guild_id, int type, const void *data, int len);
int mapif_guild_info(int fd, struct guild *g);
int guild_break_sub(void *key, void *data, va_list ap);
int mapif_parse_GuildCastleDataSave(int fd, int castle_id, int idx, int value);

// Save guild into sql
int inter_guild_tosql(struct guild *g, int flag) {
	// 1 `guild` (`guild_id`, `name`,`master`,`guild_lv`,`connect_member`,`max_member`,`average_lv`,`exp`,`next_exp`,`skill_point`,`castle_id`,`mes1`,`mes2`,`emblem_len`,`emblem_id`,`emblem_data`)
	// 2 `guild_member` (`guild_id`,`account_id`,`char_id`,`hair`,`hair_color`,`gender`,`class`,`lv`,`exp`,`exp_payper`,`online`,`position`,`rsv1`,`rsv2`,`name`)
	// 4 `guild_position` (`guild_id`,`position`,`name`,`mode`,`exp_mode`)
	// 8 `guild_alliance` (`guild_id`,`opposition`,`alliance_id`,`name`)
	// 16 `guild_expulsion` (`guild_id`,`name`,`mes`,`acc`,`account_id`,`rsv1`,`rsv2`,`rsv3`)
	// 32 `guild_skill` (`guild_id`,`id`,`lv`)

	char t_name[100], t_master[49], t_mes1[121], t_mes2[241], t_member[49], t_position[49], t_alliance[49]; // temporay storage for str convertion;
	char t_ename[49], t_emes[81];
	char emblem_data[4096];
	int i = 0;
	int guild_exist = 0, guild_member = 0, guild_online_member = 0;

	if (g->guild_id <= 0)
		return -1;

	printf("(" CL_MAGENTA "%d" CL_RESET ") Request save guild - ", g->guild_id);

	mysql_escape_string(t_name, g->name, strlen(g->name));

	//printf("- Check if guild %d exists.\n", g->guild_id);
	if (sql_request("SELECT count(1) FROM `%s` WHERE `guild_id`='%d'", guild_db, g->guild_id))
		if (sql_get_row()) {
			guild_exist = sql_get_integer(0);
			//printf("- Check if guild %d exists: %s.\n", g->guild_id, ((guild_exist == 0) ? "No" : "Yes"));
		}

	if (guild_exist > 0) {
		// Check members in party
		if (!sql_request("SELECT count(1) FROM `%s` WHERE `guild_id`='%d'", guild_member_db, g->guild_id))
			return -1;
		if (sql_get_row()) {
			guild_member = sql_get_integer(0);
			//printf("- Check members in guild %d: %d.\n", g->guild_id, guild_member);
		}

		// Delete old guild from sql
		if (flag & 1 || guild_member == 0) {
		//	printf("- Delete guild %d from guild.\n", g->guild_id);
			sql_request("DELETE FROM `%s` WHERE `guild_id`='%d'", guild_db, g->guild_id);
		}
		if (flag & 2 || guild_member == 0) {
		//	printf("- Delete guild %d from guild_member.\n", g->guild_id);
			sql_request("DELETE FROM `%s` WHERE `guild_id`='%d'", guild_member_db, g->guild_id);
		//	printf("- Delete guild %d from char\n",g->guild_id);
			sql_request("UPDATE `%s` SET `guild_id`='0' WHERE `guild_id`='%d'", char_db, g->guild_id);
		}
		if (flag & 32 || guild_member == 0) {
		//	printf("- Delete guild %d from guild_skill.\n", g->guild_id);
			sql_request("DELETE FROM `%s` WHERE `guild_id`='%d'", guild_skill_db, g->guild_id);
		}
		if (flag & 4 || guild_member == 0) {
		//	printf("- Delete guild %d from guild_position.\n", g->guild_id);
			sql_request("DELETE FROM `%s` WHERE `guild_id`='%d'", guild_position_db, g->guild_id);
		}
		if (flag & 16 || guild_member == 0) {
		//	printf("- Delete guild %d from guild_expulsion.\n", g->guild_id);
			sql_request("DELETE FROM `%s` WHERE `guild_id`='%d'", guild_expulsion_db, g->guild_id);
		}
		if (flag & 8||guild_member == 0) {
		//	printf("- Delete guild %d from guild_alliance.\n", g->guild_id);
			sql_request("DELETE FROM `%s` WHERE `guild_id`='%d' OR `alliance_id`='%d'", guild_alliance_db, g->guild_id, g->guild_id);
		}
		if (guild_member == 0) {
		//	printf("- Delete guild %d from guild_castle.\n", g->guild_id);
			sql_request("DELETE FROM `%s` WHERE `guild_id`='%d'", guild_castle_db, g->guild_id);
		}
	}

	guild_online_member = 0;
	for(i = 0; i < g->max_member; i++)
		if (g->member[i].account_id > 0)
			guild_online_member++;

	// No member in guild, no need to create it in sql
	if (guild_member <= 0 && guild_online_member <= 0) {
		inter_guild_storage_delete(g->guild_id);
		printf("No member in guild %d, break it!\n", g->guild_id);
		return -2;
	}

	// Insert new guild to sqlserver
	if (flag & 1 || guild_member == 0) {
		int len = 0;
		//printf("- Insert guild %d to guild\n", g->guild_id);
		for(i = 0; i < g->emblem_len; i++) {
			len += sprintf(emblem_data + len, "%02x", (unsigned char)(g->emblem_data[i]));
			//printf("%02x", (unsigned char)(g->emblem_data[i]));
		}
		emblem_data[len] = '\0';
		//printf("- emblem_len = %d \n", g->emblem_len);
		mysql_escape_string(t_master, g->master, strlen(g->master));
		mysql_escape_string(t_mes1, g->mes1, strlen(g->mes1));
		mysql_escape_string(t_mes2, g->mes2, strlen(g->mes2));
		sql_request("INSERT INTO `%s` "
		            "(`guild_id`, `name`,`master`,`guild_lv`,`connect_member`,`max_member`,`average_lv`,`exp`,`next_exp`,`skill_point`,`castle_id`,`mes1`,`mes2`,`emblem_len`,`emblem_id`,`emblem_data`) "
		            "VALUES ('%d', '%s', '%s', '%d', '%d', '%d', '%d', '%d', '%d', '%d', '%d', '%s', '%s', '%d', '%d', '%s')",
		            guild_db, g->guild_id, t_name, t_master,
		//            g->guild_lv, g->connect_member, g->max_member, g->average_lv, g->exp, g->next_exp, g->skill_point, g->castle_id, // castle_id in guild_db is not used (a guild can have more than 1 castle)
		            g->guild_lv, g->connect_member, g->max_member, g->average_lv, g->exp, g->next_exp, g->skill_point, -1, // castle_id in guild_db is not used (a guild can have more than 1 castle)
		            t_mes1, t_mes2, g->emblem_len, g->emblem_id, emblem_data);
	}

	if (flag & 2 || guild_member == 0) {
		//printf("- Insert guild %d to guild_member.\n", g->guild_id);
		for(i = 0; i < g->max_member; i++) {
			if (g->member[i].account_id > 0) {
				struct guild_member *m = &g->member[i];
				sql_request("DELETE FROM `%s` WHERE `char_id`='%d'", guild_member_db, m->char_id);
				mysql_escape_string(t_member, m->name, strlen(m->name));
				sql_request("INSERT INTO `%s` "
				            "(`guild_id`,`account_id`,`char_id`,`hair`,`hair_color`,`gender`,`class`,`lv`,`exp`,`exp_payper`,`online`,`position`,`rsv1`,`rsv2`,`name`) "
				            "VALUES ('%d','%d','%d','%d','%d', '%d','%d','%d','%d','%d','%d','%d','%d','%d','%s')",
				            guild_member_db, g->guild_id,
				            m->account_id, m->char_id,
				            m->hair, m->hair_color, m->gender,
				            m->class, m->lv, m->exp, m->exp_payper, m->online, m->position,
				            0, 0,
				            t_member);
				sql_request("UPDATE `%s` SET `guild_id`='%d' WHERE `char_id`='%d'", char_db, g->guild_id, m->char_id); // char_id is unique and is a key
			}
		}
	}

	if (flag & 4 || guild_member == 0) {
		//printf("- Insert guild %d to guild_position.\n", g->guild_id);
		for(i = 0; i < MAX_GUILDPOSITION; i++) {
			struct guild_position *p = &g->position[i];
			mysql_escape_string(t_position, p->name, strlen(p->name));
			sql_request("INSERT INTO `%s` (`guild_id`,`position`,`name`,`mode`,`exp_mode`) VALUES ('%d','%d', '%s','%d','%d')",
			            guild_position_db, g->guild_id, i, t_position, p->mode, p->exp_mode);
		}
	}

	if (flag & 8 || guild_member == 0) {
		//printf("- Insert guild %d to guild_alliance.\n", g->guild_id);
		for(i = 0; i < MAX_GUILDALLIANCE; i++) {
			struct guild_alliance *a = &g->alliance[i];
			if (a->guild_id > 0) {
				mysql_escape_string(t_alliance, a->name, strlen(a->name));
				sql_request("INSERT INTO `%s` (`guild_id`,`opposition`,`alliance_id`,`name`) "
				            "VALUES ('%d','%d','%d','%s')",
				            guild_alliance_db, g->guild_id, a->opposition, a->guild_id, t_alliance);
				sql_request("INSERT INTO `%s` (`guild_id`,`opposition`,`alliance_id`,`name`) "
				            "VALUES ('%d','%d','%d','%s')",
				            guild_alliance_db, a->guild_id, a->opposition, g->guild_id, t_name);
			}
		}
	}

	if (flag & 16 || guild_member == 0) {
		//printf("- Insert guild %d to guild_expulsion.\n", g->guild_id);
		for(i = 0; i < MAX_GUILDEXPLUSION; i++) {
			struct guild_explusion *e = &g->explusion[i];
			if (e->account_id > 0) {
				mysql_escape_string(t_ename, e->name, strlen(e->name));
				mysql_escape_string(t_emes, e->mes, strlen(e->mes));
				sql_request("INSERT INTO `%s` (`guild_id`,`name`,`mes`,`acc`,`account_id`,`rsv1`,`rsv2`,`rsv3`) "
				            "VALUES ('%d','%s','%s','%s','%d','%d','%d','%d')",
				            guild_expulsion_db, g->guild_id, t_ename, t_emes, e->acc, e->account_id, e->rsv1, e->rsv2, e->rsv3);
			}
		}
	}

	if (flag & 32 || guild_member == 0) {
		//printf("- Insert guild %d to guild_skill.\n", g->guild_id);
		for(i = 0; i < MAX_GUILDSKILL; i++) {
			if (g->skill[i].id > 0) {
				sql_request("INSERT INTO `%s` (`guild_id`,`id`,`lv`) VALUES ('%d','%d','%d')",
				            guild_skill_db, g->guild_id, g->skill[i].id, g->skill[i].lv);
			}
		}
	}

	printf("Save guild done.\n");

	return 0;
}

// Read guild from sql
int inter_guild_fromsql(int guild_id, struct guild *g) {
	int i;
	char emblem_data[4096];
	char *pstr;

	if (g == NULL)
		return 0;

	memset(g, 0, sizeof(struct guild));

	if (guild_id <= 0)
		return 0;

//	printf("Retrieve guild information from sql...\n");
//	printf("- Read guild %d from sql.\n", guild_id);

	if (!sql_request("SELECT `guild_id`, `name`,`master`,`guild_lv`,`connect_member`,`max_member`,`average_lv`,`exp`,`next_exp`,`skill_point`,`castle_id`,`mes1`,`mes2`,`emblem_len`,`emblem_id`,`emblem_data` "
		"FROM `%s` WHERE `guild_id`='%d'", guild_db, guild_id))
		return 0;
	if (!sql_get_row())
		return 0;

	g->guild_id = sql_get_integer(0);
	strncpy(g->name, sql_get_string(1), 24);
	strncpy(g->master, sql_get_string(2), 24);
	g->guild_lv = sql_get_integer(3);
	g->connect_member = sql_get_integer(4);
	g->max_member = sql_get_integer(5);
	if (g->max_member > MAX_GUILD) // Fix reduction of MAX_GUILD
		g->max_member = MAX_GUILD;
	if (g->max_member > (16 + (10 * guild_extension_bonus))) // Fix reduction of MAX_GUILD
		g->max_member = 16 + (10 * guild_extension_bonus);
	g->average_lv = sql_get_integer(6);
	g->exp = sql_get_integer(7);
	g->next_exp = sql_get_integer(8);
	g->skill_point = sql_get_integer(9);
//	g->castle_id = sql_get_integer(10); // castle_id in guild_db is not used (a guild can have more than 1 castle)
//	g->castle_id = -1; // castle_id in guild_db is not used (a guild can have more than 1 castle)
	strncpy(g->mes1, sql_get_string(11), 60);
	strncpy(g->mes2, sql_get_string(12), 120);
//	printf("GuildBaseInfo OK\n");

	g->emblem_len = sql_get_integer(13);
	g->emblem_id = sql_get_integer(14);
	strncpy(emblem_data, sql_get_string(15), 4096);
	for(i = 0, pstr = emblem_data; i < g->emblem_len; i++, pstr += 2) {
		int c1 = pstr[0], c2 = pstr[1], x1 = 0, x2 = 0;
		if (c1 >= '0' && c1 <= '9') x1 = c1 - '0';
		if (c1 >= 'a' && c1 <= 'f') x1 = c1 - 'a' + 10;
		if (c1 >= 'A' && c1 <= 'F') x1 = c1 - 'A' + 10;
		if (c2 >= '0' && c2 <= '9') x2 = c2 - '0';
		if (c2 >= 'a' && c2 <= 'f') x2 = c2 - 'a' + 10;
		if (c2 >= 'A' && c2 <= 'F') x2 = c2 - 'A' + 10;
		g->emblem_data[i] = (x1 << 4) | x2;
	}
//	printf("GuildEmblemInfo OK\n");

	//printf("- Read guild_member %d from sql.\n",guild_id);
	if (!sql_request("SELECT `account_id`,`char_id`,`hair`,`hair_color`,`gender`,`class`,`lv`,`exp`,`exp_payper`,`online`,`position`,`name` "
	                 "FROM `%s` WHERE `guild_id`='%d' ORDER BY `position`", guild_member_db, guild_id))
		return 0;
	for(i = 0; sql_get_row() && i < g->max_member; i++) {
		struct guild_member *m = &g->member[i];
		m->account_id = sql_get_integer(0);
		m->char_id = sql_get_integer(1);
		m->hair = sql_get_integer(2);
		m->hair_color = sql_get_integer(3);
		m->gender = sql_get_integer(4);
		m->class = sql_get_integer(5);
		m->lv = sql_get_integer(6);
		m->exp = sql_get_integer(7);
		m->exp_payper = sql_get_integer(8);
		m->online = sql_get_integer(9);
		if (sql_get_integer(10) >= MAX_GUILDPOSITION) // Fix reduction of MAX_GUILDPOSITION [PoW]
			m->position = MAX_GUILDPOSITION - 1;
		else
			m->position = sql_get_integer(10);
		strncpy(m->name, sql_get_string(11), 24);
	}
//	printf("GuildMemberInfo OK\n");

	//printf("- Read guild_position %d from sql.\n", guild_id);
	if (!sql_request("SELECT `position`,`name`,`mode`,`exp_mode` FROM `%s` WHERE `guild_id`='%d'", guild_position_db, guild_id))
		return 0;
	while(sql_get_row()) {
		i = sql_get_integer(0);
		if (i < MAX_GUILDPOSITION) {
			struct guild_position *p = &g->position[i];
			p->mode = sql_get_integer(2);
			p->exp_mode = sql_get_integer(3);
			strncpy(p->name, sql_get_string(1), 24);
			if (p->name[0] == '\0') { // fix increasement of MAX_GUILDPOSITION
				if (i == 0)
					strncpy(p->name, "GuildMaster", 24);
				else if (i == MAX_GUILDPOSITION - 1)
					strncpy(p->name, "Newbie", 24);
				else
					sprintf(p->name, "Position %d", i + 1);
//			} else {
//				p->name[23] = '\0';
			}
		}
	}
//	printf("GuildPositionInfo OK\n");

	//printf("- Read guild_alliance %d from sql.\n", guild_id);
	if (!sql_request("SELECT `opposition`,`alliance_id`,`name` FROM `%s` WHERE `guild_id`='%d'", guild_alliance_db, guild_id))
		return 0;
	for(i = 0; sql_get_row() && i < MAX_GUILDALLIANCE; i++) {
		struct guild_alliance *a = &g->alliance[i];
		a->opposition = sql_get_integer(0);
		a->guild_id = sql_get_integer(1);
		strncpy(a->name, sql_get_string(2), 24);
	}
//	printf("GuildAllianceInfo OK\n");

	//printf("- Read guild_expulsion %d from sql.\n", guild_id);
	if (!sql_request("SELECT `name`,`mes`,`acc`,`account_id`,`rsv1`,`rsv2`,`rsv3` FROM `%s` WHERE `guild_id`='%d'",guild_expulsion_db, guild_id))
		return 0;
	for(i = 0; sql_get_row() && i < MAX_GUILDEXPLUSION; i++) {
		struct guild_explusion *e = &g->explusion[i];
		strncpy(e->name, sql_get_string(0), 24);
		strncpy(e->mes, sql_get_string(1), 40);
		strncpy(e->acc, sql_get_string(2), 24);
		e->account_id = sql_get_integer(3);
		e->rsv1 = sql_get_integer(4);
		e->rsv2 = sql_get_integer(5);
		e->rsv3 = sql_get_integer(6);
	}
//	printf("GuildExplusionInfo OK\n");

	//printf("- Read guild_skill %d from sql.\n", guild_id);
	if (!sql_request("SELECT `id`,`lv` FROM `%s` WHERE `guild_id`='%d' ORDER BY `id`", guild_skill_db, guild_id))
		return 0;
	for(i = 0; sql_get_row() && i < MAX_GUILDSKILL; i++) {
		g->skill[i].id = sql_get_integer(0);
		g->skill[i].lv = sql_get_integer(1);
	}
//	printf("GuildSkillInfo OK\n");

//	printf("Successfully retrieve guild information from sql!\n");

	return 0;
}

// Read exp_guild.txt
int inter_guild_readdb() {
	int i;
	FILE *fp;
	char line[1024];

	for (i = 0; i < 100; i++)
		guild_exp[i] = 0;

	if ((fp = fopen("db/exp_guild.txt", "r")) == NULL) {
		printf("can't read db/exp_guild.txt\n");
		return 1;
	}

	i = 0;
	while(fgets(line, sizeof(line), fp) && i < 100) { // fgets reads until maximum one less than size and add '\0' -> so, it's not necessary to add -1
		if ((line[0] == '/' && line[1] == '/') || line[0] == '\0' || line[0] == '\n' || line[0] == '\r')
			continue;
		// it's not necessary to remove 'carriage return ('\n' or '\r')
		guild_exp[i] = atoi(line);
		i++;
	}
	fclose(fp);

	return 0;
}

// Read castles database
void read_castles_data() {
	int c;
	struct guild_castle *gc; // speed up
#ifdef TXT_ONLY
	char line[65536];
	int tmp_int[26];
	FILE *fp;
#endif /* TXT_ONLY */

	/* init castles data */
	memset(&castle_db, 0, sizeof(struct guild_castle) * MAX_GUILDCASTLE);
	for (c = 0; c < MAX_GUILDCASTLE; c++)
		castle_db[c].castle_id = c;

#ifdef TXT_ONLY
	if ((fp = fopen(castle_txt, "r")) == NULL) {
		// try default file name
		fp = fopen("save/castle.txt", "r");
	}

	if (fp != NULL) {
		c = 0; // line counter
		while(fgets(line, sizeof(line), fp)) { // fgets reads until maximum one less than size and add '\0' -> so, it's not necessary to add -1
			c++;
			if ((line[0] == '/' && line[1] == '/') || line[0] == '\0' || line[0] == '\n' || line[0] == '\r')
				continue;
			// it's not necessary to remove 'carriage return ('\n' or '\r')
			// new structure of guild castle
			if (sscanf(line, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",
			           &tmp_int[0], &tmp_int[1], &tmp_int[2], &tmp_int[3], &tmp_int[4], &tmp_int[5], &tmp_int[6],
			           &tmp_int[7], &tmp_int[8], &tmp_int[9], &tmp_int[10], &tmp_int[11], &tmp_int[12], &tmp_int[13],
			           &tmp_int[14], &tmp_int[15], &tmp_int[16], &tmp_int[17], &tmp_int[18], &tmp_int[19], &tmp_int[20],
			           &tmp_int[21], &tmp_int[22], &tmp_int[23], &tmp_int[24], &tmp_int[25]) == 26) {
				if (tmp_int[0] >= 0 && tmp_int[0] < MAX_GUILDCASTLE) {
					gc = &castle_db[tmp_int[0]];
					gc->castle_id = tmp_int[0];
					gc->guild_id = tmp_int[1];
					gc->economy = tmp_int[2];
					gc->defense = tmp_int[3];
					gc->triggerE = tmp_int[4];
					gc->triggerD = tmp_int[5];
					gc->nextTime = tmp_int[6];
					gc->payTime = tmp_int[7];
					gc->createTime = tmp_int[8];
					gc->visibleC = tmp_int[9];
					gc->visibleG0 = tmp_int[10];
					gc->visibleG1 = tmp_int[11];
					gc->visibleG2 = tmp_int[12];
					gc->visibleG3 = tmp_int[13];
					gc->visibleG4 = tmp_int[14];
					gc->visibleG5 = tmp_int[15];
					gc->visibleG6 = tmp_int[16];
					gc->visibleG7 = tmp_int[17];
					gc->Ghp0 = tmp_int[18];
					gc->Ghp1 = tmp_int[19];
					gc->Ghp2 = tmp_int[20];
					gc->Ghp3 = tmp_int[21];
					gc->Ghp4 = tmp_int[22];
					gc->Ghp5 = tmp_int[23];
					gc->Ghp6 = tmp_int[24];
					gc->Ghp7 = tmp_int[25]; // end additions [Valaris]
				} else
					printf("read_castles_data: invalid castle id (%d) [%s] line %d.\n", tmp_int[0], castle_txt, c);
			// old structure of guild castle
			} else if (sscanf(line, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",
			                  &tmp_int[0], &tmp_int[1], &tmp_int[2], &tmp_int[3], &tmp_int[4], &tmp_int[5], &tmp_int[6],
			                  &tmp_int[7], &tmp_int[8], &tmp_int[9], &tmp_int[10], &tmp_int[11], &tmp_int[12], &tmp_int[13],
			                  &tmp_int[14], &tmp_int[15], &tmp_int[16], &tmp_int[17]) == 18) {
				if (tmp_int[0] >= 0 && tmp_int[0] < MAX_GUILDCASTLE) {
					gc = &castle_db[tmp_int[0]];
					gc->castle_id = tmp_int[0];
					gc->guild_id = tmp_int[1];
					gc->economy = tmp_int[2];
					gc->defense = tmp_int[3];
					gc->triggerE = tmp_int[4];
					gc->triggerD = tmp_int[5];
					gc->nextTime = tmp_int[6];
					gc->payTime = tmp_int[7];
					gc->createTime = tmp_int[8];
					gc->visibleC = tmp_int[9];
					gc->visibleG0 = tmp_int[10];
					gc->visibleG1 = tmp_int[11];
					gc->visibleG2 = tmp_int[12];
					gc->visibleG3 = tmp_int[13];
					gc->visibleG4 = tmp_int[14];
					gc->visibleG5 = tmp_int[15];
					gc->visibleG6 = tmp_int[16];
					gc->visibleG7 = tmp_int[17];
					if (gc->visibleG0 == 1)
						gc->Ghp0 = 15670 + 2000 * gc->defense;
					else
						gc->Ghp0 = 0;
					if (gc->visibleG1 == 1)
						gc->Ghp1 = 15670 + 2000 * gc->defense;
					else
						gc->Ghp1 = 0;
					if (gc->visibleG2 == 1)
						gc->Ghp2 = 15670 + 2000 * gc->defense;
					else
						gc->Ghp2 = 0;
					if (gc->visibleG3 == 1)
						gc->Ghp3 = 30214 + 2000 * gc->defense;
					else
						gc->Ghp3 = 0;
					if (gc->visibleG4 == 1)
						gc->Ghp4 = 30214 + 2000 * gc->defense;
					else
						gc->Ghp4 = 0;
					if (gc->visibleG5 == 1)
						gc->Ghp5 = 28634 + 2000 * gc->defense;
					else
						gc->Ghp5 = 0;
					if (gc->visibleG6 == 1)
						gc->Ghp6 = 28634 + 2000 * gc->defense;
					else
						gc->Ghp6 = 0;
					if (gc->visibleG7 == 1)
						gc->Ghp7 = 28634 + 2000 * gc->defense;
					else
						gc->Ghp7 = 0;
				} else
					printf("read_castles_data: invalid castle id (%d) [%s] line %d.\n", tmp_int[0], castle_txt, c);
			} else {
				printf("read_castles_data: broken data [%s] line %d.\n", castle_txt, c);
			}
		}
		fclose(fp);
	}
#endif /* TXT_ONLY */

#ifdef USE_SQL
	if (sql_request("SELECT `castle_id`, `guild_id`, `economy`, `defense`, `triggerE`, `triggerD`, `nextTime`, `payTime`, `createTime`, "
	                 "`visibleC`, `visibleG0`, `visibleG1`, `visibleG2`, `visibleG3`, `visibleG4`, `visibleG5`, `visibleG6`, `visibleG7`,"
	                 "`Ghp0`, `Ghp1`, `Ghp2`, `Ghp3`, `Ghp4`, `Ghp5`, `Ghp6`, `Ghp7`"
	                 " FROM `%s`", guild_castle_db)) {
		while (sql_get_row()) {
			c = sql_get_integer(0);
			if (c >= 0 && c < MAX_GUILDCASTLE) {
				gc = &castle_db[c];
				gc->castle_id  = c;
				gc->guild_id   = sql_get_integer(1);
				gc->economy    = sql_get_integer(2);
				gc->defense    = sql_get_integer(3);
				gc->triggerE   = sql_get_integer(4);
				gc->triggerD   = sql_get_integer(5);
				gc->nextTime   = sql_get_integer(6);
				gc->payTime    = sql_get_integer(7);
				gc->createTime = sql_get_integer(8);
				gc->visibleC   = sql_get_integer(9);
				gc->visibleG0  = sql_get_integer(10);
				gc->visibleG1  = sql_get_integer(11);
				gc->visibleG2  = sql_get_integer(12);
				gc->visibleG3  = sql_get_integer(13);
				gc->visibleG4  = sql_get_integer(14);
				gc->visibleG5  = sql_get_integer(15);
				gc->visibleG6  = sql_get_integer(16);
				gc->visibleG7  = sql_get_integer(17);
				gc->Ghp0 = sql_get_integer(18);
				gc->Ghp1 = sql_get_integer(19);
				gc->Ghp2 = sql_get_integer(20);
				gc->Ghp3 = sql_get_integer(21);
				gc->Ghp4 = sql_get_integer(22);
				gc->Ghp5 = sql_get_integer(23);
				gc->Ghp6 = sql_get_integer(24);
				gc->Ghp7 = sql_get_integer(25);
			} else {
				printf("read_castles_data: invalid castle id (%d) [%s].\n", c, guild_castle_db);
				sql_request("DELETE FROM `%s` WHERE `castle_id`='%d'", guild_castle_db, c);
			}
		}
	} else {
		printf("SQL error: Unable to read '%s' database.\n", guild_castle_db);
		exit(0);
	}
#endif /* USE_SQL */

	printf("Total castles -> %d.\n", MAX_GUILDCASTLE);

	return;
}

#ifdef TXT_ONLY
// saving castles
void castles_save() {
	struct guild_castle *gc; // speed up
	int i;
	int lock;
	FILE *fp;

	if ((fp = lock_fopen(castle_txt, &lock)) == NULL)
		printf("int_guild: cant write [%s] !!! data is lost !!!\n", castle_txt);
	else {
		for (i = 0; i < MAX_GUILDCASTLE; i++) {
			gc = &castle_db[i];
			fprintf(fp, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d" RETCODE,
			          gc->castle_id, gc->guild_id, gc->economy, gc->defense, gc->triggerE,
			          gc->triggerD, gc->nextTime, gc->payTime, gc->createTime, gc->visibleC,
			          gc->visibleG0, gc->visibleG1, gc->visibleG2, gc->visibleG3, gc->visibleG4,
			          gc->visibleG5, gc->visibleG6, gc->visibleG7, gc->Ghp0, gc->Ghp1, gc->Ghp2,
			          gc->Ghp3, gc->Ghp4, gc->Ghp5, gc->Ghp6, gc->Ghp7);
		}
		/* close file */
		lock_fclose(fp, castle_txt, &lock);
//		printf("castles_save: %s saved.\n", castle_txt);
	}
#endif /* TXT_ONLY */

#ifdef TXT_ONLY
	return;
}
#endif /* TXT_ONLY */

// Initialize guild database
void inter_guild_init() {
	int guild_counter;

	printf("interserver guild memory initialize... (%d byte)\n", sizeof(struct guild));
	CALLOC(guild_pt, struct guild, 1);
	CALLOC(guild_pt2, struct guild, 1);

	// Read exp_guild.txt
	inter_guild_readdb();

	if (!sql_request("UPDATE `%s` SET `online`='0'", guild_member_db)) {
		printf("SQL error: Unable to reset 'online' in guild_member_db.\n");
		exit(0);
	}

	if (!sql_request("UPDATE `%s` SET `connect_member`='0'", guild_db)) {
		printf("SQL error: Unable to reset 'connect_member' in guild_db.\n");
		exit(0);
	}

	sql_request("SELECT count(1) FROM `%s`", guild_db);
	guild_counter = 0;
	if (sql_get_row()) {
		guild_counter = sql_get_integer(0);
		if (guild_counter > 0) {
			// set guild_newid
			if (!sql_request("SELECT max(`guild_id`) FROM `%s`", guild_db)) {
				printf("SQL error: Unable to found max 'guild_id' in guild_db.\n");
				exit(0);
			}
			if (sql_get_row())
				if (sql_get_integer(0) >= guild_newid)
					guild_newid = sql_get_integer(0) + 1;
		}
	}

	printf("Total guild data -> %d.\n", guild_counter);
	printf("Set guild_newid: %d.\n", guild_newid);

	read_castles_data();

	return;
}

// Get guild by its name
struct guild* search_guildname(char *str) {
	struct guild *g;
	char t_name[48];

	printf("search_guildname.\n");

	g = guild_pt;

	if (g != NULL) {
		memset(g, 0, sizeof(struct guild));
		mysql_escape_string(t_name, str, strlen(str));
		sql_request("SELECT `guild_id` FROM `%s` WHERE `name`='%s'", guild_db, t_name);
		if (sql_get_row() && sql_get_integer(0) > 0)
			inter_guild_fromsql(sql_get_integer(0), g);
	}

	return g;
}

// Check if guild is empty
int guild_check_empty(struct guild *g) {
	int i;

	for(i = 0; i < g->max_member; i++) {
		if (g->member[i].account_id > 0) {
			return 0;
		}
	}

	// 誰もいないので解散
	mapif_guild_broken(g->guild_id, 0);
	inter_guild_storage_delete(g->guild_id);
	for (i = 0; i < MAX_GUILDCASTLE; i++)
		if (castle_db[i].guild_id == g->guild_id)
			mapif_parse_GuildCastleDataSave(0, i, 1, 0); // set guild_id (1) to 0
	inter_guild_tosql(g, 255);
	memset(g, 0, sizeof(struct guild));

	return 1;
}

int guild_nextexp(int level) {
	if (level < 100 && level > 0)
		return guild_exp[level - 1];

	return 0;
}

// ギルドスキルがあるか確認
int guild_checkskill(struct guild *g, int id) {
	int idx = id - GD_SKILLBASE;

	if (idx < 0 || idx >= MAX_GUILDSKILL)
		return 0;

	return g->skill[idx].lv;
}

// ギルドの情報の再計算
int guild_calcinfo(struct guild *g) {
	int i, c, nextexp;
	struct guild before = *g;

	// スキルIDの設定
	for(i = 0; i < MAX_GUILDSKILL; i++)
		g->skill[i].id = i + GD_SKILLBASE;

	// ギルドレベル
	if (g->guild_lv <= 0)
		g->guild_lv = 1;
	nextexp = guild_nextexp(g->guild_lv);
	if (nextexp > 0) {
		while(g->exp >= nextexp && nextexp > 0) { // レベルアップ処理
			g->exp -= nextexp;
			g->guild_lv++;
			g->skill_point++;
			nextexp = guild_nextexp(g->guild_lv);
		}
	}

	// ギルドの次の経験値
	g->next_exp = guild_nextexp(g->guild_lv);

	// メンバ上限（ギルド拡張適用）
	g->max_member = 16 + guild_checkskill(g, GD_EXTENSION) * guild_extension_bonus; // SKILL Guild EXTENTION +6 (kRO Patch - 9/27/05: The Guild skill [Expand Capacity] which increases number of people allowed in the guild, now adds 6 people capacity per level instead of 4.)

	// 平均レベルとオンライン人数
	g->average_lv = 0;
	g->connect_member = 0;
	c = 0;
	for(i = 0; i < g->max_member; i++) {
		if (g->member[i].account_id > 0) {
			g->average_lv += g->member[i].lv;
			c++;
			if (g->member[i].online > 0)
				g->connect_member++;
		}
	}
	if (c)
		g->average_lv /= c;

	// 全データを送る必要がありそう
	if (g->max_member != before.max_member ||
	    g->guild_lv != before.guild_lv ||
	    g->skill_point != before.skill_point) {
		mapif_guild_info(-1, g);
		return 1;
	}

	return 0;
}

//-------------------------------------------------------------------
// map serverへの通信

// ギルド作成可否
int mapif_guild_created(int fd, int account_id, struct guild *g) {
	WPACKETW(0) = 0x3830;
	WPACKETL(2) = account_id;
	if (g != NULL) {
		WPACKETL(6) = g->guild_id;
		printf("int_guild: created! %d - %s\n", g->guild_id, g->name);
	} else {
		WPACKETL(6) = 0;
	}
	SENDPACKET(fd, 10);

	return 0;
}

// ギルド情報見つからず
int mapif_guild_noinfo(int fd, int guild_id) { // 0x3831 <size>.W (<guild_id>.L | <struct guild>.?B) - answer of 0x3031 - if size = 8: guild doesn't exist - otherwise: guild structure
	WPACKETW(0) = 0x3831; // 0x3831 <size>.W (<guild_id>.L | <struct guild>.?B) - answer of 0x3031 - if size = 8: guild doesn't exist - otherwise: guild structure
	WPACKETW(2) = 8;
	WPACKETL(4) = guild_id;
	SENDPACKET(fd, 8);
	printf("int_guild: info not found %d\n", guild_id);

	return 0;
}

// ギルド情報まとめ送り
int mapif_guild_info(int fd, struct guild *g) {
	WPACKETW(0) = 0x3831; // 0x3831 <size>.W (<guild_id>.L | <struct guild>.?B) - answer of 0x3031 - if size = 8: guild doesn't exist - otherwise: guild structure
	WPACKETW(2) = 4 + sizeof(struct guild);
	memcpy(WPACKETP(4), g, sizeof(struct guild));
//	printf("int_guild: sizeof(guild)=%d\n", sizeof(struct guild));
	if (fd < 0)
		mapif_sendall(4 + sizeof(struct guild));
	else
		mapif_send(fd, 4 + sizeof(struct guild));
//	printf("int_guild: info %d %s\n", p->guild_id, p->name);

	return 0;
}

// メンバ追加可否
int mapif_guild_memberadded(int fd, int guild_id, int account_id, int char_id, int flag) {
	WPACKETW( 0) = 0x3832;
	WPACKETL( 2) = guild_id;
	WPACKETL( 6) = account_id;
	WPACKETL(10) = char_id;
	WPACKETB(14) = flag;
	SENDPACKET(fd, 15);

	return 0;
}

// 脱退/追放通知
int mapif_guild_leaved(int guild_id, int account_id, int char_id, int flag, const char *name, const char *mes) {
	WPACKETW( 0) = 0x3834;
	WPACKETL( 2) = guild_id;
	WPACKETL( 6) = account_id;
	WPACKETL(10) = char_id;
	WPACKETB(14) = flag;
	strncpy(WPACKETP(15), mes, 40);
	strncpy(WPACKETP(55), name, 24);
	mapif_sendall(79);
	printf("int_guild: guild leaved %d %d %s %s\n", guild_id, account_id, name, mes);

	return 0;
}

// オンライン状態とLv更新通知
int mapif_guild_memberinfoshort(struct guild *g, int idx) {
	WPACKETW( 0) = 0x3835;
	WPACKETL( 2) = g->guild_id;
	WPACKETL( 6) = g->member[idx].account_id;
	WPACKETL(10) = g->member[idx].char_id;
	WPACKETB(14) = g->member[idx].online;
	WPACKETW(15) = g->member[idx].lv;
	WPACKETW(17) = g->member[idx].class;
	mapif_sendall(19);

	return 0;
}

// 解散通知
int mapif_guild_broken(int guild_id, int flag) {
	WPACKETW(0) = 0x3836;
	WPACKETL(2) = guild_id;
	WPACKETB(6) = flag;
	mapif_sendall(7);
	printf("int_guild: broken %d\n", guild_id);

	return 0;
}

// ギルド基本情報変更通知
int mapif_guild_basicinfochanged(int guild_id,int type,const void *data,int len) {
	WPACKETW(0) = 0x3839;
	WPACKETW(2) = len + 10;
	WPACKETL(4) = guild_id;
	WPACKETW(8) = type;
	memcpy(WPACKETP(10), data, len);
	mapif_sendall(len + 10);

	return 0;
}

// ギルドメンバ情報変更通知
int mapif_guild_memberinfochanged(int guild_id, int account_id, int char_id, int type, const void *data, int len) {
	WPACKETW( 0) = 0x383a;
	WPACKETW( 2) = len + 18;
	WPACKETL( 4) = guild_id;
	WPACKETL( 8) = account_id;
	WPACKETL(12) = char_id;
	WPACKETW(16) = type;
	memcpy(WPACKETP(18), data, len);
	mapif_sendall(len + 18);

	return 0;
}

// ギルドスキルアップ通知
int mapif_guild_skillupack(int guild_id, int skill_num, int account_id) {
	WPACKETW( 0) = 0x383c;
	WPACKETL( 2) = guild_id;
	WPACKETL( 6) = skill_num;
	WPACKETL(10) = account_id;
	mapif_sendall(14);

	return 0;
}

// ギルド同盟/敵対通知
int mapif_guild_alliance(int guild_id1, int guild_id2, int account_id1, int account_id2, int flag, const char *name1, const char *name2) {
	WPACKETW( 0) = 0x383d;
	WPACKETL( 2) = guild_id1;
	WPACKETL( 6) = guild_id2;
	WPACKETL(10) = account_id1;
	WPACKETL(14) = account_id2;
	WPACKETB(18) = flag;
	memcpy(WPACKETP(19), name1, 24);
	memcpy(WPACKETP(43), name2, 24);
	mapif_sendall(67);

	return 0;
}

// ギルド役職変更通知
int mapif_guild_position(struct guild *g, int idx) {
	WPACKETW(0) = 0x383b;
	WPACKETW(2) = sizeof(struct guild_position) + 12;
	WPACKETL(4) = g->guild_id;
	WPACKETL(8) = idx;
	memcpy(WPACKETP(12), &g->position[idx], sizeof(struct guild_position));
	mapif_sendall(sizeof(struct guild_position) + 12);

	return 0;
}

// ギルド告知変更通知
int mapif_guild_notice(struct guild *g) {
	WPACKETW(0) = 0x383e;
	WPACKETL(2) = g->guild_id;
	strncpy(WPACKETP( 6), g->mes1, 60);
	strncpy(WPACKETP(66), g->mes2, 120);
	mapif_sendall(186);

	return 0;
}

// ギルドエンブレム変更通知
int mapif_guild_emblem(struct guild *g) {
	WPACKETW(0) = 0x383f;
	WPACKETW(2) = g->emblem_len + 12;
	WPACKETL(4) = g->guild_id;
	WPACKETL(8) = g->emblem_id;
	memcpy(WPACKETP(12), g->emblem_data, g->emblem_len);
	mapif_sendall(WPACKETW(2));

	return 0;
}

int mapif_guild_castle_dataload(int castle_id, int idx, int value) { // <Agit>
	WPACKETW(0) = 0x3840;
	WPACKETW(2) = castle_id;
	WPACKETB(4) = idx;
	WPACKETL(5) = value;
	mapif_sendall(9);

	return 0;
}

int mapif_guild_castle_datasave(int castle_id, int idx, int value) { // <Agit>
	WPACKETW(0) = 0x3841;
	WPACKETW(2) = castle_id;
	WPACKETB(4) = idx;
	WPACKETL(5) = value;
	mapif_sendall(9);

	return 0;
}

// マップサーバーの接続時処理
void inter_guild_mapif_init(int fd) {
	int i;

	WPACKETW(0) = 0x3842;
	for (i = 0; i < MAX_GUILDCASTLE; i++)
		memcpy(WPACKETP(4 + i * sizeof(struct guild_castle)), &castle_db[i], sizeof(struct guild_castle));
	WPACKETW(2) = 4 + MAX_GUILDCASTLE * sizeof(struct guild_castle);
	SENDPACKET(fd, 4 + MAX_GUILDCASTLE * sizeof(struct guild_castle));

	return;
}

//-------------------------------------------------------------------
// map serverからの通信

// ギルド作成要求
int mapif_parse_CreateGuild(int fd,int account_id,char *name,struct guild_member *master) {
	struct guild *g;
	int i;

	printf("CreateGuild\n");
	g = search_guildname(name);
	if (g != NULL && g->guild_id > 0) {
		printf("int_guild_create: same name guild exists [%s].\n", name);
		mapif_guild_created(fd,account_id,NULL);
		return 0;
	}
	g = guild_pt;
	memset(g, 0, sizeof(struct guild));
	g->guild_id = guild_newid++;
	strncpy(g->name, name, 24);
	strncpy(g->master, master->name, 24);
	memcpy(&g->member[0], master, sizeof(struct guild_member));

	g->position[0].mode = 0x11;
	strncpy(g->position[                  0].name, "GuildMaster", 24);
	strncpy(g->position[MAX_GUILDPOSITION-1].name, "Newbie", 24);
	for(i = 1; i < MAX_GUILDPOSITION-1; i++)
		sprintf(g->position[i].name, "Position %d", i + 1);

	// Initialize guild property
	g->max_member = 16;
	g->average_lv = master->lv;
//	g->castle_id = -1; // castle_id in guild_db is not used (a guild can have more than 1 castle)
	for(i = 0; i < MAX_GUILDSKILL; i++)
		g->skill[i].id = i + GD_SKILLBASE;

	// Save to sql
	printf("Create initialize OK!\n");
	i = inter_guild_tosql(g, 255);

	if (i < 0) {
		mapif_guild_created(fd, account_id, NULL);
		return 0;
	}

	// Report to client
	mapif_guild_created(fd, account_id, g);
	mapif_guild_info(fd, g);

	inter_log("guild %s (id=%d) created by master %s (id=%d)" RETCODE, name, g->guild_id, master->name, master->account_id);

	return 0;
}

// Return guild info to client
int mapif_parse_GuildInfo(int fd,int guild_id) { // 0x3031 <guild_id>.L - ask for guild
	struct guild *g;

	g = guild_pt;
	inter_guild_fromsql(guild_id, g);
	if (g != NULL && g->guild_id > 0) {
		guild_calcinfo(g);
		mapif_guild_info(fd, g);
		//inter_guild_tosql(g, 1); // Change guild
	} else
		mapif_guild_noinfo(fd, guild_id);

	return 0;
}

// Add member to guild
int mapif_parse_GuildAddMember(int fd,int guild_id,struct guild_member *m) {
	struct guild *g;
	int i;

	g = guild_pt;
	if (g == NULL) {
		mapif_guild_memberadded(fd, guild_id, m->account_id, m->char_id, 1);
		return 0;
	}

	inter_guild_fromsql(guild_id, g);
	if (g->guild_id <= 0) {
		mapif_guild_memberadded(fd, guild_id, m->account_id, m->char_id, 1);
		return 0;
	}

	for(i = 0; i < g->max_member; i++) {
		if (g->member[i].account_id == 0) {

			memcpy(&g->member[i], m, sizeof(struct guild_member));
			mapif_guild_memberadded(fd, guild_id, m->account_id, m->char_id, 0);
			guild_calcinfo(g);
			mapif_guild_info(-1, g);
			inter_guild_tosql(g, 3); // Change guild & guild_member
			return 0;
		}
	}
	mapif_guild_memberadded(fd, guild_id, m->account_id, m->char_id, 1);
	//inter_guild_tosql(g, 3); // Change guild & guild_member

	return 0;
}

// Delete member from guild
int mapif_parse_GuildLeave(int fd, int guild_id, int account_id, int char_id, int flag, const char *mes) {
	struct guild *g;
	int i, j;

	g = guild_pt;
	inter_guild_fromsql(guild_id, g);

	if (g != NULL && g->guild_id > 0) {
		for(i = 0; i < MAX_GUILD; i++) {
			// just check char_id (char_id is unique)
			if (g->member[i].char_id == char_id) {
//				printf("%d %d\n", i, (int)(&g->member[i]));
//				printf("%d %s\n", i, g->member[i].name);

				if (flag) { // 追放の場合追放リストに入れる
					for(j = 0; j < MAX_GUILDEXPLUSION; j++) {
						if (g->explusion[j].account_id == 0)
							break;
					}
					if (j == MAX_GUILDEXPLUSION) { // 一杯なので古いのを消す
						for(j = 0; j < MAX_GUILDEXPLUSION-1; j++)
							g->explusion[j] = g->explusion[j+1];
						j = MAX_GUILDEXPLUSION - 1;
					}
					g->explusion[j].account_id = account_id;
					memset(g->explusion[j].acc, 0, sizeof(g->explusion[j].acc));
					strncpy(g->explusion[j].acc, "dummy", 24);
					memset(g->explusion[j].name, 0, sizeof(g->explusion[j].name));
					strncpy(g->explusion[j].name, g->member[i].name, 24);
					memset(g->explusion[j].mes, 0, sizeof(g->explusion[j].mes));
					strncpy(g->explusion[j].mes, mes, 40);
				}

				mapif_guild_leaved(guild_id, account_id, char_id, flag, g->member[i].name, mes);
//				printf("%d %d\n", i, (int)(&g->member[i]));
//				printf("%d %s\n", i, (&g->member[i])->name);
				memset(&g->member[i], 0, sizeof(struct guild_member));

				if (guild_check_empty(g) == 0)
					mapif_guild_info(-1, g); // まだ人がいるのでデータ送信
				/*
				else
					inter_guild_save(); // 解散したので一応セーブ
				return 0;*/
			}
		}
		guild_calcinfo(g);
		inter_guild_tosql(g, 19); // Change guild & guild_member & guild_expulsion
	} else {
		sql_request("UPDATE `%s` SET `guild_id`='0' WHERE `account_id`='%d' AND `char_id`='%d'", char_db, account_id, char_id);
		// mapif_guild_leaved(guild_id, account_id, char_id, flag, g->member[i].name, mes);
	}

	return 0;
}

// Change member info
int mapif_parse_GuildChangeMemberInfoShort(int fd, int guild_id, int account_id, int char_id, int online, int lv, int class) {
	// Could speed up by manipulating only guild_member
	struct guild *g;
	int i, alv, c;

	g = guild_pt;

	if (g == NULL)
		return 0;

	inter_guild_fromsql(guild_id, g);

	if (g->guild_id <= 0)
		return 0;

	g->connect_member = 0;

	alv = 0;
	c = 0;
	for(i = 0; i < MAX_GUILD; i++) {
		// just check char_id (char_id is unique)
		if (g->member[i].char_id == char_id) {
			g->member[i].online = online;
			g->member[i].lv = lv;
			g->member[i].class = class;
			mapif_guild_memberinfoshort(g,i);
#ifdef USE_SQL
			sql_request("UPDATE `%s` SET `online`=%d,`lv`=%d,`class`=%d WHERE `char_id`=%d", guild_member_db, online, lv, class, char_id);
#endif /* USE_SQL */
		}
		if (g->member[i].account_id > 0) {
			alv += g->member[i].lv;
			c++;
		}
		if (g->member[i].online)
			g->connect_member++;
	}
	// 平均レベル
	g->average_lv = (c) ? alv / c : 0;

/* suppress call of inter_guild_tosql and replace by 2 requests (this and the previous in the function) */
//	inter_guild_tosql(g, 3); // Change guild & guild_member
	sql_request("UPDATE `%s` SET `connect_member`=%d,`average_lv`=%d WHERE `guild_id`='%d'", guild_db, g->connect_member, g->average_lv, g->guild_id);

	return 0;
}

// BreakGuild
int mapif_parse_BreakGuild(int fd, int guild_id) {
	int i;
	struct guild *g;

	g = guild_pt;
	if (g == NULL)
		return 0;

	inter_guild_fromsql(guild_id, g);

	// Delete guild from sql
	//printf("- Delete guild %d from guild.\n",guild_id);
	sql_request("DELETE FROM `%s` WHERE `guild_id`='%d'", guild_db, guild_id);
	//printf("- Delete guild %d from guild_member.\n",guild_id);
	sql_request("DELETE FROM `%s` WHERE `guild_id`='%d'", guild_member_db, guild_id);
	//printf("- Delete guild %d from guild_skill.\n",guild_id);
	sql_request("DELETE FROM `%s` WHERE `guild_id`='%d'", guild_skill_db, guild_id);
	//printf("- Delete guild %d from guild_position.\n",guild_id);
	sql_request("DELETE FROM `%s` WHERE `guild_id`='%d'", guild_position_db, guild_id);
	//printf("- Delete guild %d from guild_expulsion.\n",guild_id);
	sql_request("DELETE FROM `%s` WHERE `guild_id`='%d'", guild_expulsion_db, guild_id);
	//printf("- Delete guild %d from guild_alliance.\n",guild_id);
	sql_request("DELETE FROM `%s` WHERE `guild_id`='%d' OR `alliance_id`='%d'", guild_alliance_db, guild_id, guild_id);

	//printf("- Delete guild %d from guild_castle.\n",guild_id);
	sql_request("DELETE FROM `%s` WHERE `guild_id`='%d'", guild_castle_db, guild_id);

	//printf("- Update guild %d of char.\n",guild_id);
	sql_request("UPDATE `%s` SET `guild_id`='0' WHERE `guild_id`='%d'", char_db, guild_id);

	inter_guild_storage_delete(guild_id);
	mapif_guild_broken(guild_id, 0);
	for (i = 0; i < MAX_GUILDCASTLE; i++)
		if (castle_db[i].guild_id == g->guild_id)
			mapif_parse_GuildCastleDataSave(0, i, 1, 0); // set guild_id (1) to 0

	inter_log("guild %s (id=%d) broken." RETCODE, g->name, guild_id);

	return 0;
}

// ギルドメッセージ送信
// ギルド内発言
void mapif_parse_GuildMessage(int fd, int guild_id, int account_id, char *mes, int len) {
	WPACKETW(0) = 0x3837;
	WPACKETW(2) = len + 12;
	WPACKETL(4) = guild_id;
	WPACKETL(8) = account_id;
	strncpy(WPACKETP(12), mes, len);
	mapif_sendallwos(fd, len + 12);

	return;
}

// ギルド基本データ変更要求
int mapif_parse_GuildBasicInfoChange(int fd, int guild_id, int type, const char *data, int len) {
	struct guild *g;
	short dw = *((short *)data);

	g = guild_pt;

	if (g == NULL)
		return 0;

	inter_guild_fromsql(guild_id, g);

	if (g->guild_id <= 0)
		return 0;

	switch(type) {
	case GBI_GUILDLV:
//		printf("GBI_GUILDLV\n");
		if (dw > 0 && g->guild_lv + dw <= 50) {
			g->guild_lv += dw;
			g->skill_point += dw;
		}else if (dw < 0 && g->guild_lv + dw >= 1)
			g->guild_lv += dw;
		mapif_guild_info(-1, g);
#ifdef USE_SQL
/* suppress call of inter_guild_tosql and replace by 1 request */
//		inter_guild_tosql(g, 1); // Change guild
		sql_request("UPDATE `%s` SET `guild_lv`=%d,`skill_point`=%d WHERE `guild_id`='%d'", guild_db, g->guild_lv, g->skill_point, g->guild_id);
#endif /* USE_SQL */
		return 0;
	default:
		printf("int_guild: GuildBasicInfoChange: Unknown type %d.\n", type);
		break;
	}
	mapif_guild_basicinfochanged(guild_id, type, data, len);
#ifdef USE_SQL
	//inter_guild_tosql(g, 1); // Change guild
#endif /* USE_SQL */

	return 0;
}

// ギルドメンバデータ変更要求
int mapif_parse_GuildMemberInfoChange(int fd, int guild_id, int account_id, int char_id, int type, const char *data, int len) {
	// Could make some improvement in speed, because only change guild_member
	int i;
	struct guild *g;

	g = guild_pt;

	if (g == NULL)
		return 0;

	inter_guild_fromsql(guild_id, g);
	//printf("GuildMemberInfoChange %s.\n", (type == GMI_EXP) ? "GMI_EXP" : "OTHER");

	for(i = 0; i < g->max_member; i++)
		// just check char_id (char_id is unique)
		if (g->member[i].char_id == char_id)
			break;
	if (i == g->max_member) {
		printf("int_guild: GuildMemberChange: Not found %d,%d in %d[%s].\n", account_id, char_id, guild_id, g->name);
		return 0;
	}

	switch(type) {
	case GMI_POSITION: // 役職
		g->member[i].position = *((int *)data);
		mapif_guild_memberinfochanged(guild_id, account_id, char_id, type, data, len);
#ifdef USE_SQL
/* suppress call of inter_guild_tosql and replace by 1 request */
//		inter_guild_tosql(g, 3); // Change guild & guild_member
		sql_request("UPDATE `%s` SET `position`=%d WHERE `char_id`=%d", guild_member_db, g->member[i].position, char_id);
#endif /* USE_SQL */
		break;
	case GMI_EXP: // EXP
	  {
		int exp = *((unsigned int *)data);
		g->exp += exp;
		if (g->exp < 0)
			g->exp = 0x7FFFFFFF;
		g->member[i].exp += exp;
		if (g->member[i].exp < 0)
			g->member[i].exp = 0x7FFFFFFF;
		guild_calcinfo(g); // Lvアップ判断
		mapif_guild_basicinfochanged(guild_id, GBI_EXP, &g->exp, 4);
		mapif_guild_memberinfochanged(guild_id, account_id, char_id, type, &g->member[i].exp, sizeof(g->member[i].exp));
#ifdef USE_SQL
/* suppress call of inter_guild_tosql and replace by 2 requests */
//		inter_guild_tosql(g, 3); // Change guild & guild_member
		sql_request("UPDATE `%s` SET `exp`=%d WHERE `char_id`=%d", guild_member_db, g->member[i].exp, char_id);
		sql_request("UPDATE `%s` SET `guild_lv`=%d,`connect_member`=%d,`max_member`=%d,`average_lv`=%d,`exp`=%d,`next_exp`=%d,`skill_point`=%d WHERE `guild_id`='%d'",
		            guild_db, g->guild_lv, g->connect_member, g->max_member, g->average_lv, g->exp, g->next_exp, g->skill_point, g->guild_id);
#endif /* USE_SQL */
	  }
		break;
	default:
		printf("int_guild: GuildMemberInfoChange: Unknown type %d.\n", type);
		break;
	}

	return 0;
}

// ギルド役職名変更要求
int mapif_parse_GuildPosition(int fd, int guild_id, int idx, struct guild_position *p) {
	struct guild *g;
#ifdef USE_SQL
	char t_position[48]; // temporay storage for str convertion;
#endif /* USE_SQL */

	if (idx < 0 || idx >= MAX_GUILDPOSITION)
		return 0;

	g = guild_pt;

	if (g == NULL)
		return 0;

	inter_guild_fromsql(guild_id, g);

	if (g->guild_id <= 0)
		return 0;

	memcpy(&g->position[idx], p, sizeof(struct guild_position));
	mapif_guild_position(g, idx);
	printf("int_guild: position changed %d\n", idx);

#ifdef USE_SQL
/* suppress call of inter_guild_tosql and replace by 1 request */
//	inter_guild_tosql(g, 4); // Change guild_position
	mysql_escape_string(t_position, g->position[idx].name, strlen(g->position[idx].name));
	sql_request("UPDATE `%s` SET `name`='%s',`mode`='%d',`exp_mode`='%d' WHERE `guild_id`='%d' AND `position`='%d'", guild_position_db, t_position, g->position[idx].mode, g->position[idx].exp_mode, guild_id, idx);
#endif /* USE_SQL */

	return 0;
}

// ギルドスキルアップ要求
int mapif_parse_GuildSkillUp(int fd, int guild_id, int skill_num, int account_id) {
	// Could make some improvement in speed, because only change guild_position
	int idx;
	struct guild *g;

	idx = skill_num - GD_SKILLBASE;
	if (idx < 0 || idx >= MAX_GUILDSKILL)
		return 0;

	g = guild_pt;
	if (g == NULL)
		return 0;
	//printf("GuildSkillUp\n");

	inter_guild_fromsql(guild_id, g);

	if (g->skill_point > 0 && g->skill[idx].id > 0 && g->skill[idx].lv < 10) {
		g->skill[idx].lv++;
		g->skill_point--;
		if (guild_calcinfo(g) == 0)
			mapif_guild_info(-1, g);
		mapif_guild_skillupack(guild_id, skill_num, account_id);
		printf("int_guild: skill %d up\n", skill_num);
		inter_guild_tosql(g, 33); // Change guild & guild_skill
	}

	return 0;
}

// ギルド同盟要求
int mapif_parse_GuildAlliance(int fd, int guild_id1, int guild_id2, int account_id1, int account_id2, int flag) {
	// Could speed up
	struct guild *g[2];
	int j, i;

	if (guild_id1 <= 0 || guild_id2 <= 0)
		return 0;

	g[0] = guild_pt;
	g[1] = guild_pt2;
	inter_guild_fromsql(guild_id1, g[0]);
	inter_guild_fromsql(guild_id2, g[1]);

	if (g[0] == NULL || g[1] == NULL || g[0]->guild_id == 0 || g[1]->guild_id == 0)
		return 0;

	if (!(flag & 0x8)) {
		for(i = 0; i < 2 - (flag & 1); i++) {
			for(j = 0; j < MAX_GUILDALLIANCE; j++)
				if (g[i]->alliance[j].guild_id == 0) {
					g[i]->alliance[j].guild_id = g[1-i]->guild_id;
					memset(g[i]->alliance[j].name, 0, sizeof(g[i]->alliance[j].name));
					strncpy(g[i]->alliance[j].name, g[1-i]->name, 24);
					g[i]->alliance[j].opposition = flag & 1;
					break;
				}
		}
	} else { // 関係解消
		for(i = 0; i < 2 - (flag & 1); i++) {
			for(j = 0; j < MAX_GUILDALLIANCE; j++)
				if (g[i]->alliance[j].guild_id == g[1-i]->guild_id && g[i]->alliance[j].opposition == (flag&1)) {
					g[i]->alliance[j].guild_id = 0;
					break;
				}
		}
	}
	mapif_guild_alliance(guild_id1, guild_id2, account_id1, account_id2, flag, g[0]->name, g[1]->name);
	inter_guild_tosql(g[0], 8); // Change guild_alliance
	inter_guild_tosql(g[1], 8); // Change guild_alliance

	return 0;
}

// ギルド告知変更要求
int mapif_parse_GuildNotice(int fd, int guild_id, const char *mes1, const char *mes2) {
	struct guild *g;
#ifdef USE_SQL
	char t_mes1[121], t_mes2[241]; // temporay storage for str convertion;
#endif /* USE_SQL */

	if (guild_id <= 0)
		return 0;

	g = guild_pt;

	if (g == NULL)
		return 0;

	inter_guild_fromsql(guild_id, g);

	if (g->guild_id <= 0)
		return 0;

	memset(g->mes1, 0, sizeof(g->mes1));
	strncpy(g->mes1, mes1, 60);
	memset(g->mes2, 0, sizeof(g->mes2));
	strncpy(g->mes2, mes2, 120);
#ifdef USE_SQL
/* suppress call of inter_guild_tosql and replace by 1 requests */
//	inter_guild_tosql(g, 1); // Change mes of guild
	mysql_escape_string(t_mes1, g->mes1, strlen(g->mes1));
	mysql_escape_string(t_mes2, g->mes2, strlen(g->mes2));
	sql_request("UPDATE `%s` SET `mes1`='%s',`mes2`='%s' WHERE `guild_id`='%d'", guild_db, t_mes1, t_mes2, g->guild_id);
#endif /* USE_SQL */

	return mapif_guild_notice(g);
}

// ギルドエンブレム変更要求
int mapif_parse_GuildEmblem(int fd, int len, int guild_id, int dummy, const char *data) {
	struct guild *g;

	if (guild_id <= 0)
		return 0;

	g = guild_pt;

	if (g == NULL)
		return 0;

	inter_guild_fromsql(guild_id, g);

	if (g->guild_id <= 0)
		return 0;

	memcpy(g->emblem_data, data, len);
	g->emblem_len = len;
	g->emblem_id++;
	inter_guild_tosql(g, 1); // Change guild

	return mapif_guild_emblem(g);
}

int mapif_parse_GuildCastleDataLoad(int fd, int castle_id, int idx) { // <Agit>
	struct guild_castle *gc;

	if (castle_id < 0 && castle_id >= MAX_GUILDCASTLE)
		return mapif_guild_castle_dataload(castle_id, 0, 0);

	gc = &castle_db[castle_id];
	switch(idx) {
	case  1: return mapif_guild_castle_dataload(gc->castle_id, idx, gc->guild_id);
	case  2: return mapif_guild_castle_dataload(gc->castle_id, idx, gc->economy);
	case  3: return mapif_guild_castle_dataload(gc->castle_id, idx, gc->defense);
	case  4: return mapif_guild_castle_dataload(gc->castle_id, idx, gc->triggerE);
	case  5: return mapif_guild_castle_dataload(gc->castle_id, idx, gc->triggerD);
	case  6: return mapif_guild_castle_dataload(gc->castle_id, idx, gc->nextTime);
	case  7: return mapif_guild_castle_dataload(gc->castle_id, idx, gc->payTime);
	case  8: return mapif_guild_castle_dataload(gc->castle_id, idx, gc->createTime);
	case  9: return mapif_guild_castle_dataload(gc->castle_id, idx, gc->visibleC);
	case 10: return mapif_guild_castle_dataload(gc->castle_id, idx, gc->visibleG0);
	case 11: return mapif_guild_castle_dataload(gc->castle_id, idx, gc->visibleG1);
	case 12: return mapif_guild_castle_dataload(gc->castle_id, idx, gc->visibleG2);
	case 13: return mapif_guild_castle_dataload(gc->castle_id, idx, gc->visibleG3);
	case 14: return mapif_guild_castle_dataload(gc->castle_id, idx, gc->visibleG4);
	case 15: return mapif_guild_castle_dataload(gc->castle_id, idx, gc->visibleG5);
	case 16: return mapif_guild_castle_dataload(gc->castle_id, idx, gc->visibleG6);
	case 17: return mapif_guild_castle_dataload(gc->castle_id, idx, gc->visibleG7);
	case 18: return mapif_guild_castle_dataload(gc->castle_id, idx, gc->Ghp0); // guardian HP [Valaris]
	case 19: return mapif_guild_castle_dataload(gc->castle_id, idx, gc->Ghp1);
	case 20: return mapif_guild_castle_dataload(gc->castle_id, idx, gc->Ghp2);
	case 21: return mapif_guild_castle_dataload(gc->castle_id, idx, gc->Ghp3);
	case 22: return mapif_guild_castle_dataload(gc->castle_id, idx, gc->Ghp4);
	case 23: return mapif_guild_castle_dataload(gc->castle_id, idx, gc->Ghp5);
	case 24: return mapif_guild_castle_dataload(gc->castle_id, idx, gc->Ghp6);
	case 25: return mapif_guild_castle_dataload(gc->castle_id, idx, gc->Ghp7); // end additions [Valaris]

	default:
		printf("mapif_parse_GuildCastleDataLoad ERROR!! (Not found index=%d).\n", idx);
		return 0;
	}

	return 0;
}

int mapif_parse_GuildCastleDataSave(int fd, int castle_id, int idx, int value) { // <Agit>
	struct guild_castle *gc;

	if (castle_id < 0 && castle_id >= MAX_GUILDCASTLE)
		return mapif_guild_castle_datasave(castle_id, idx, value);

	gc = &castle_db[castle_id];
	switch(idx) {
	case 1:
		if (gc->guild_id != value) {
			int gid = (value) ? value : gc->guild_id;
#ifdef TXT_ONLY
			struct guild *g = numdb_search(guild_db, (CPU_INT)gid);
#endif /* TXT_ONLY */
#ifdef USE_SQL
			struct guild *g = guild_pt;
			inter_guild_fromsql(gid, g);
			// update SQL DB
//			sql_request("UPDATE `%s` SET `castle_id`='-1' WHERE `castle_id`='%d'", guild_db, castle_id); // castle_id is not used (a guild can have more than 1 castle)
//			sql_request("UPDATE `%s` SET `castle_id`='%d' WHERE `guild_id`='%d'", guild_db, castle_id, value); // castle_id is not used (a guild can have more than 1 castle)
#endif /* USE_SQL */
			inter_log("guild %s (id=%d) %s castle id=%d" RETCODE, (g) ? g->name : "??", gid, (value) ? "occupy" : "abandon", castle_id);
			gc->guild_id = value;
			break;
		} else
			return 0;
	case  2: if (gc->economy    != value) { gc->economy    = value; break; } else return 0;
	case  3: if (gc->defense    != value) { gc->defense    = value; break; } else return 0;
	case  4: if (gc->triggerE   != value) { gc->triggerE   = value; break; } else return 0;
	case  5: if (gc->triggerD   != value) { gc->triggerD   = value; break; } else return 0;
	case  6: if (gc->nextTime   != value) { gc->nextTime   = value; break; } else return 0;
	case  7: if (gc->payTime    != value) { gc->payTime    = value; break; } else return 0;
	case  8: if (gc->createTime != value) { gc->createTime = value; break; } else return 0;
	case  9: if (gc->visibleC   != value) { gc->visibleC   = value; break; } else return 0;
	case 10: if (gc->visibleG0  != value) { gc->visibleG0  = value; break; } else return 0;
	case 11: if (gc->visibleG1  != value) { gc->visibleG1  = value; break; } else return 0;
	case 12: if (gc->visibleG2  != value) { gc->visibleG2  = value; break; } else return 0;
	case 13: if (gc->visibleG3  != value) { gc->visibleG3  = value; break; } else return 0;
	case 14: if (gc->visibleG4  != value) { gc->visibleG4  = value; break; } else return 0;
	case 15: if (gc->visibleG5  != value) { gc->visibleG5  = value; break; } else return 0;
	case 16: if (gc->visibleG6  != value) { gc->visibleG6  = value; break; } else return 0;
	case 17: if (gc->visibleG7  != value) { gc->visibleG7  = value; break; } else return 0;
	case 18: if (gc->Ghp0 != value) { gc->Ghp0 = value; break; } else return 0; // guardian HP [Valaris]
	case 19: if (gc->Ghp1 != value) { gc->Ghp1 = value; break; } else return 0;
	case 20: if (gc->Ghp2 != value) { gc->Ghp2 = value; break; } else return 0;
	case 21: if (gc->Ghp3 != value) { gc->Ghp3 = value; break; } else return 0;
	case 22: if (gc->Ghp4 != value) { gc->Ghp4 = value; break; } else return 0;
	case 23: if (gc->Ghp5 != value) { gc->Ghp5 = value; break; } else return 0;
	case 24: if (gc->Ghp6 != value) { gc->Ghp6 = value; break; } else return 0;
	case 25: if (gc->Ghp7 != value) { gc->Ghp7 = value; break; } else return 0; // end additions [Valaris]
	default:
		printf("mapif_parse_GuildCastleDataSave ERROR!! (Not found index=%d).\n", idx);
		return 0;
	}

#ifdef USE_SQL
	sql_request("REPLACE INTO `%s` "
	            "(`castle_id`, `guild_id`, `economy`, `defense`, `triggerE`, `triggerD`, `nextTime`, `payTime`, `createTime`,"
	            "`visibleC`, `visibleG0`, `visibleG1`, `visibleG2`, `visibleG3`, `visibleG4`, `visibleG5`, `visibleG6`, `visibleG7`,"
	            "`Ghp0`, `Ghp1`, `Ghp2`, `Ghp3`, `Ghp4`, `Ghp5`, `Ghp6`, `Ghp7`)"
	            "VALUES ('%d','%d','%d','%d','%d','%d','%d','%d','%d','%d','%d','%d','%d','%d','%d','%d','%d','%d','%d','%d','%d','%d','%d','%d','%d','%d')",
	            guild_castle_db, gc->castle_id, gc->guild_id, gc->economy, gc->defense, gc->triggerE, gc->triggerD, gc->nextTime, gc->payTime,
	            gc->createTime, gc->visibleC, gc->visibleG0, gc->visibleG1, gc->visibleG2, gc->visibleG3, gc->visibleG4, gc->visibleG5,
	            gc->visibleG6, gc->visibleG7, gc->Ghp0, gc->Ghp1, gc->Ghp2, gc->Ghp3, gc->Ghp4, gc->Ghp5, gc->Ghp6, gc->Ghp7);
#endif /* USE_SQL */

	return mapif_guild_castle_datasave(gc->castle_id, idx, value);
}

// ギルドチェック要求
int mapif_parse_GuildCheck(int fd, int guild_id, int account_id, int char_id) {
	// What does this mean? Check if belong to another guild?

	return 0;
}

// map server からの通信
// ・１パケットのみ解析すること
// ・パケット長データはinter.cにセットしておくこと
// ・パケット長チェックや、RFIFOSKIPは呼び出し元で行われるので行ってはならない
// ・エラーなら0(false)、そうでないなら1(true)をかえさなければならない
int inter_guild_parse_frommap(int fd) {
//	printf("inter_guild_parse_frommap: connection #%d, packet: 0x%x (with being read: %d bytes).\n", fd, RFIFOW(fd,0), RFIFOREST(fd));

	switch(RFIFOW(fd,0)) {
	case 0x3030: mapif_parse_CreateGuild(fd, RFIFOL(fd,4), RFIFOP(fd,8), (struct guild_member *)RFIFOP(fd,32)); break;
	case 0x3031: mapif_parse_GuildInfo(fd, RFIFOL(fd,2)); break; // 0x3031 <guild_id>.L - ask for guild
	case 0x3032: mapif_parse_GuildAddMember(fd, RFIFOL(fd,4), (struct guild_member *)RFIFOP(fd,8)); break;
	case 0x3034: mapif_parse_GuildLeave(fd, RFIFOL(fd,2), RFIFOL(fd,6), RFIFOL(fd,10), RFIFOB(fd,14), RFIFOP(fd,15)); break;
	case 0x3035: mapif_parse_GuildChangeMemberInfoShort(fd, RFIFOL(fd,2), RFIFOL(fd,6), RFIFOL(fd,10), RFIFOB(fd,14), RFIFOW(fd,15), RFIFOW(fd,17)); break;
	case 0x3036: mapif_parse_BreakGuild(fd, RFIFOL(fd,2)); break;
	case 0x3037: mapif_parse_GuildMessage(fd, RFIFOL(fd,4), RFIFOL(fd,8), RFIFOP(fd,12), RFIFOW(fd,2) - 12); break;
	case 0x3038: mapif_parse_GuildCheck(fd, RFIFOL(fd,2), RFIFOL(fd,6), RFIFOL(fd,10)); break;
	case 0x3039: mapif_parse_GuildBasicInfoChange(fd, RFIFOL(fd,4), RFIFOW(fd,8), RFIFOP(fd,10), RFIFOW(fd,2) - 10); break;
	case 0x303A: mapif_parse_GuildMemberInfoChange(fd, RFIFOL(fd,4), RFIFOL(fd,8), RFIFOL(fd,12), RFIFOW(fd,16), RFIFOP(fd,18), RFIFOW(fd,2) - 18); break;
	case 0x303B: mapif_parse_GuildPosition(fd, RFIFOL(fd,4), RFIFOL(fd,8), (struct guild_position *)RFIFOP(fd,12)); break;
	case 0x303C: mapif_parse_GuildSkillUp(fd, RFIFOL(fd,2), RFIFOL(fd,6), RFIFOL(fd,10)); break;
	case 0x303D: mapif_parse_GuildAlliance(fd, RFIFOL(fd,2), RFIFOL(fd,6), RFIFOL(fd,10), RFIFOL(fd,14), RFIFOB(fd,18)); break;
	case 0x303E: mapif_parse_GuildNotice(fd, RFIFOL(fd,2), RFIFOP(fd,6), RFIFOP(fd,66)); break;
	case 0x303F: mapif_parse_GuildEmblem(fd, RFIFOW(fd,2) - 12, RFIFOL(fd,4), RFIFOL(fd,8), RFIFOP(fd,12)); break;
	case 0x3040: mapif_parse_GuildCastleDataLoad(fd, RFIFOW(fd,2), RFIFOB(fd,4)); break;
	case 0x3041: mapif_parse_GuildCastleDataSave(fd, RFIFOW(fd,2), RFIFOB(fd,4), RFIFOL(fd,5)); break;

	default:
		return 0;
	}

	return 1;
}

// サーバーから脱退要求（キャラ削除用）
void inter_guild_leave(int guild_id, int account_id, int char_id) {
	mapif_parse_GuildLeave(-1, guild_id, account_id, char_id, 0, "**サーバー命令**"); // babelfish: * Server order *

	return;
}
