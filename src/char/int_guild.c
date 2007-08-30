// Copyright (c) Freya Development Team - Licensed under GNU GPL
// For more information, see License.txt in the main folder

#include <config.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <mmo.h>
#include "../common/db.h"
#include "../common/lock.h"
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

static struct dbt *guild_db;
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

// ギルドデータの文字列への変換
int inter_guild_tostr(char *str, struct guild *g) {
	int i, c, len;

	// 基本データ
	len = sprintf(str, "%d\t%s\t%s\t%d,%d,%d,%d,%d\t%s#\t%s#\t",
	              g->guild_id, g->name, g->master,
	//              g->guild_lv, g->max_member, g->exp, g->skill_point, g->castle_id, // castle_id in guild_db is not used (a guild can have more than 1 castle)
	              g->guild_lv, g->max_member, g->exp, g->skill_point, -1, // castle_id in guild_db is not used (a guild can have more than 1 castle)
	              g->mes1, g->mes2);
	// メンバー
	for(i = 0; i < g->max_member; i++) {
		struct guild_member *m = &g->member[i];
		if (m->account_id > 0) { // don't save void members
			len += sprintf(str + len, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\t%s\t",
			               m->account_id, m->char_id,
			               m->hair, m->hair_color, m->gender,
			               m->class, m->lv, m->exp, m->exp_payper, m->position,
			               m->name);
		}
	}
	// 役職
	for(i = 0; i < MAX_GUILDPOSITION; i++) {
		struct guild_position *p = &g->position[i];
		len += sprintf(str + len, "%d,%d\t%s#\t", p->mode, p->exp_mode, p->name);
	}
	// エンブレム
	len += sprintf(str + len, "%d,%d,", g->emblem_len, g->emblem_id);
	for(i = 0; i < g->emblem_len; i++) {
		len += sprintf(str + len, "%02x", (unsigned char)(g->emblem_data[i]));
	}
	len += sprintf(str + len, "$\t");
	// 同盟リスト
	c = 0;
	for(i = 0; i < MAX_GUILDALLIANCE; i++)
		if (g->alliance[i].guild_id > 0)
			c++;
	len += sprintf(str + len, "%d\t", c);
	for(i = 0; i < MAX_GUILDALLIANCE; i++) {
		struct guild_alliance *a = &g->alliance[i];
		if (a->guild_id > 0)
			len += sprintf(str + len, "%d,%d\t%s\t", a->guild_id, a->opposition, a->name);
	}
	// 追放リスト
	c = 0;
	for(i = 0; i < MAX_GUILDEXPLUSION; i++)
		if (g->explusion[i].account_id > 0)
			c++;
	len += sprintf(str + len, "%d\t", c);
	for(i = 0; i < MAX_GUILDEXPLUSION; i++) {
		struct guild_explusion *e = &g->explusion[i];
		if (e->account_id > 0)
			len += sprintf(str + len, "%d,%d,%d,%d\t%s\t%s\t%s#\t",
			               e->account_id, e->rsv1, e->rsv2, e->rsv3, e->name, e->acc, e->mes);
	}
	// ギルドスキル
	for(i = 0; i < MAX_GUILDSKILL; i++) {
		len += sprintf(str + len, "%d,%d ", g->skill[i].id, g->skill[i].lv);
	}
	len += sprintf(str + len, "\t");

	return 0;
}

// ギルドデータの文字列からの変換
int inter_guild_fromstr(char *str, struct guild *g) {
	int i, j, c;
	int tmp_int[16];
	char tmp_str[4][256];
	char tmp_str2[8192];
	char *pstr;

	// 基本データ
	memset(g, 0, sizeof(struct guild));
	if (sscanf(str, "%d\t%[^\t]\t%[^\t]\t%d,%d,%d,%d,%d\t%[^\t]\t%[^\t]\t%n", &tmp_int[0],
	           tmp_str[0], tmp_str[1],
	           &tmp_int[1], &tmp_int[2], &tmp_int[3], &tmp_int[4], &tmp_int[5],
	           tmp_str[2], tmp_str[3], &j) < 8)
		return 1;

	g->guild_id = tmp_int[0];
	g->guild_lv = tmp_int[1];
	g->max_member = tmp_int[2];
	if (g->max_member > MAX_GUILD) // fix reduction of MAX_GUILD
		g->max_member = MAX_GUILD;
	if (g->max_member > (16 + (10 * guild_extension_bonus))) // Fix reduction of MAX_GUILD
		g->max_member = 16 + (10 * guild_extension_bonus);
	g->exp = tmp_int[3];
	g->skill_point = tmp_int[4];
//	g->castle_id = tmp_int[5]; // castle_id in guild_db is not used (a guild can have more than 1 castle)
//	g->castle_id = -1; // castle_id in guild_db is not used (a guild can have more than 1 castle)
	strncpy(g->name, tmp_str[0], 24);
	strncpy(g->master, tmp_str[1], 24);
	while((c = strlen(tmp_str[2])) > 0 && tmp_str[2][c - 1] == '#')
		tmp_str[2][c - 1] = '\0'; // suppress #
	strncpy(g->mes1, tmp_str[2], 60);
	while((c = strlen(tmp_str[3])) > 0 && tmp_str[3][c - 1] == '#')
		tmp_str[3][c - 1] = '\0'; // suppress #
	strncpy(g->mes2, tmp_str[3], 120);

	str = str + j;
//	printf("GuildBaseInfo OK\n");

	// メンバー
	i = 0;
	while (sscanf(str, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\t%[^\t]\t%n",
		          &tmp_int[0], &tmp_int[1], &tmp_int[2], &tmp_int[3], &tmp_int[4],
		          &tmp_int[5], &tmp_int[6], &tmp_int[7], &tmp_int[8], &tmp_int[9],
		          tmp_str[0], &j) == 11) {
		if (i < g->max_member && tmp_int[0] > 0) { // account not void
			struct guild_member *m = &g->member[i];
			m->account_id = tmp_int[0];
			m->char_id = tmp_int[1];
			m->hair = tmp_int[2];
			m->hair_color = tmp_int[3];
			m->gender = tmp_int[4];
			m->class = tmp_int[5];
			m->lv = tmp_int[6];
			m->exp = tmp_int[7];
			m->exp_payper = tmp_int[8];
			if (tmp_int[9] >= MAX_GUILDPOSITION) // fix reduction of MAX_GUILDPOSITION
				m->position = MAX_GUILDPOSITION - 1;
			else
				m->position = tmp_int[9];
			strncpy(m->name, tmp_str[0], 24);
			i++;
		}
		str = str + j;
	}
//	printf("GuildMemberInfo OK\n");

	// 役職
	i = 0;
	while (sscanf(str, "%d,%d\t%[^\t]\t%n", &tmp_int[0], &tmp_int[1], tmp_str2, &j) == 3 && tmp_str2[strlen(tmp_str2) - 1] == '#') {
		if (i < MAX_GUILDPOSITION) {
			struct guild_position *p = &g->position[i];
			p->mode = tmp_int[0];
			p->exp_mode = tmp_int[1];
			while((c = strlen(tmp_str2)) > 0 && tmp_str2[c - 1] == '#')
				tmp_str2[c - 1] = '\0'; // suppress #
			strncpy(p->name, tmp_str2, 24);
			if (p->name[0] == '\0') { // fix increasement of MAX_GUILDPOSITION
				if (i == 0)
					strncpy(p->name, "GuildMaster", 24);
				else if (i == MAX_GUILDPOSITION - 1)
					strncpy(p->name, "Newbie", 24);
				else
					sprintf(p->name, "Position %d", i + 1);
			}
			i++;
		}
		str = str + j;
	}
//	printf("GuildPositionInfo OK\n");

	// エンブレム
	tmp_int[1] = 0;
	if (sscanf(str, "%d,%d,%[^\t]\t%n", &tmp_int[0], &tmp_int[1], tmp_str2, &j)< 3 &&
		sscanf(str, "%d,%[^\t]\t%n", &tmp_int[0], tmp_str2, &j) < 2)
		return 1;
	g->emblem_len = tmp_int[0];
	g->emblem_id = tmp_int[1];
	for(i = 0, pstr = tmp_str2; i < g->emblem_len; i++, pstr += 2) {
		int c1 = pstr[0], c2 = pstr[1], x1 = 0, x2 = 0;
		if (c1 >= '0' && c1 <= '9') x1 = c1 - '0';
		if (c1 >= 'a' && c1 <= 'f') x1 = c1 - 'a' + 10;
		if (c1 >= 'A' && c1 <= 'F') x1 = c1 - 'A' + 10;
		if (c2 >= '0' && c2 <= '9') x2 = c2 - '0';
		if (c2 >= 'a' && c2 <= 'f') x2 = c2 - 'a' + 10;
		if (c2 >= 'A' && c2 <= 'F') x2 = c2 - 'A' + 10;
		g->emblem_data[i] = (x1 << 4) | x2;
	}
	str = str + j;
//	printf("GuildEmblemInfo OK\n");

	// 同盟リスト
	if (sscanf(str, "%d\t", &c) < 1)
		return 1;
	str = strchr(str + 1, '\t'); // 位置スキップ
	for(i = 0; i < c; i++) {
		struct guild_alliance *a = &g->alliance[i];
		if (sscanf(str + 1, "%d,%d\t%[^\t]\t", &tmp_int[0], &tmp_int[1], tmp_str[0]) < 3)
			return 1;
		a->guild_id = tmp_int[0];
		a->opposition = tmp_int[1];
		strncpy(a->name, tmp_str[0], 24);

		for(j = 0; j < 2 && str != NULL; j++)	// 位置スキップ
			str = strchr(str + 1, '\t');
	}
//	printf("GuildAllianceInfo OK\n");

	// 追放リスト
	if (sscanf(str+1, "%d\t", &c) < 1)
		return 1;
	str = strchr(str + 1, '\t');	// 位置スキップ
	for(i = 0; i < c; i++) {
		struct guild_explusion *e = &g->explusion[i];
		if (sscanf(str + 1, "%d,%d,%d,%d\t%[^\t]\t%[^\t]\t%[^\t]\t",
		           &tmp_int[0], &tmp_int[1], &tmp_int[2], &tmp_int[3],
		           tmp_str[0], tmp_str[1], tmp_str[2]) < 6)
			return 1;
		e->account_id = tmp_int[0];
		e->rsv1 = tmp_int[1];
		e->rsv2 = tmp_int[2];
		e->rsv3 = tmp_int[3];
		strncpy(e->name, tmp_str[0], 24);
		strncpy(e->acc, tmp_str[1], 24);
		while((j = strlen(tmp_str[2])) > 0 && tmp_str[2][j - 1] == '#')
			tmp_str[2][j - 1] = '\0'; // suppress #
		strncpy(e->mes, tmp_str[2], 40);

		for(j = 0; j < 4 && str != NULL; j++)	// 位置スキップ
			str = strchr(str + 1, '\t');
	}
//	printf("GuildExplusionInfo OK\n");

	// ギルドスキル
	i = 0;
	j = 0;
	while (sscanf(str + 1,"%d,%d ", &tmp_int[0], &tmp_int[1]) == 2 && i < MAX_GUILDSKILL) {
		g->skill[i].id = tmp_int[0];
		g->skill[i].lv = tmp_int[1];
		i++;
		if (tmp_int[1] > 0)
			j++;
		str = strchr(str + 1, ' ');
	}
	str = strchr(str + 1, '\t');
	if ((g->guild_lv-1) > (j+g->skill_point)) {
		j=(g->guild_lv-1)-j;
		if (j>=0) g->skill_point=j;
	}
//	printf("GuildSkillInfo OK\n");

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
					gc->Ghp7 = tmp_int[25];
				} else
					printf("read_castles_data: invalid castle id (%d) [%s] line %d.\n", tmp_int[0], castle_txt, c);
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
	char line[65536];
	struct guild *g;
	FILE *fp;
	int i, j, c = 0;
	int guild_counter;

	// Read exp_guild.txt
	inter_guild_readdb();

	guild_db = numdb_init();

	if ((fp = fopen(guild_txt, "r")) == NULL) {
		// try default file name
		fp = fopen("save/guild.txt", "r");
	}

	guild_counter = 0;
	if (fp) {
		while(fgets(line, sizeof(line), fp)) { // fgets reads until maximum one less than size and add '\0' -> so, it's not necessary to add -1
			if ((line[0] == '/' && line[1] == '/') || line[0] == '\0' || line[0] == '\n' || line[0] == '\r')
				continue;
			// it's not necessary to remove 'carriage return ('\n' or '\r')
			j = 0;
			if (sscanf(line, "%d\t%%newid%%\n%n", &i, &j) == 1 && j > 0) {
				if (guild_newid < i)
					guild_newid = i;
				continue;
			}

			CALLOC(g, struct guild, 1);
			if (inter_guild_fromstr(line, g) == 0 && g->guild_id > 0) {
				if (g->guild_id >= guild_newid)
					guild_newid = g->guild_id + 1;
				numdb_insert(guild_db, (CPU_INT)g->guild_id, g);
				guild_check_empty(g);
				guild_calcinfo(g);
				guild_counter++;
			} else {
				printf("int_guild: broken data [%s] line %d\n", guild_txt, c);
				FREE(g);
			}
			c++;
		}
		fclose(fp);
//		printf("int_guild: %s read done (%d guilds)\n", guild_txt, c);
	}

	printf("Total guild data -> %d.\n", guild_counter);
	printf("Set guild_newid: %d.\n", guild_newid);

	read_castles_data();

	return;
}

// ギルドデータのセーブ用
int inter_guild_save_sub(void *key, void *data, va_list ap) {
	char line[16384];
	FILE *fp;

	inter_guild_tostr(line, (struct guild *)data);
	fp = va_arg(ap, FILE *);
	fprintf(fp, "%s" RETCODE, line);

	return 0;
}

// ギルドデータのセーブ
void inter_guild_save() {
	FILE *fp;
	int lock;

	if ((fp = lock_fopen(guild_txt, &lock)) == NULL) {
		printf("int_guild: cant write [%s] !!! data is lost !!!\n", guild_txt);
		return;
	}

	numdb_foreach(guild_db, inter_guild_save_sub, fp);
//	fprintf(fp, "%d\t%%newid%%" RETCODE, guild_newid);
	lock_fclose(fp, guild_txt, &lock);
//	printf("int_guild: %s saved.\n", guild_txt);

	castles_save();

	return;
}

// ギルド名検索用
int search_guildname_sub(void *key, void *data, va_list ap) {
	struct guild *g = (struct guild *)data, **dst;
	char *str;

	str = va_arg(ap, char *);
	dst = va_arg(ap, struct guild **);
	if (strcasecmp(g->name, str) == 0)
		*dst = g;

	return 0;
}

// ギルド名検索
struct guild* search_guildname(char *str) {
	struct guild *g = NULL;
	numdb_foreach(guild_db, search_guildname_sub, str, &g);

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
	numdb_foreach(guild_db, guild_break_sub, g->guild_id);
	numdb_erase(guild_db, (CPU_INT)g->guild_id);
	inter_guild_storage_delete(g->guild_id);
	mapif_guild_broken(g->guild_id, 0);
	for (i = 0; i < MAX_GUILDCASTLE; i++)
		if (castle_db[i].guild_id == g->guild_id)
			mapif_parse_GuildCastleDataSave(0, i, 1, 0); // set guild_id (1) to 0
	FREE(g);

	return 1;
}

// キャラの競合がないかチェック用
int guild_check_conflict_sub(void *key, void *data, va_list ap) {
	struct guild *g = (struct guild *)data;
	int guild_id, account_id, char_id, i;

	guild_id = va_arg(ap, int);
	account_id = va_arg(ap, int);
	char_id = va_arg(ap, int);

	if (g->guild_id == guild_id) // 本来の所属なので問題なし
		return 0;

	for(i = 0; i < MAX_GUILD; i++) {
		// just check char_id (char_id is unique)
		if (g->member[i].char_id == char_id) {
			// 別のギルドに偽の所属データがあるので脱退
			printf("int_guild: guild conflict! %d,%d %d!=%d\n", account_id, char_id, guild_id, g->guild_id);
			mapif_parse_GuildLeave(-1, g->guild_id, account_id, char_id, 0, "**Guild Data Bug**");
		}
	}

	return 0;
}

// キャラの競合がないかチェック
int guild_check_conflict(int guild_id, int account_id, int char_id) {
	numdb_foreach(guild_db, guild_check_conflict_sub, guild_id, account_id, char_id);

	return 0;
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
	}else{
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
	SENDPACKET(fd,8);
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
int mapif_guild_basicinfochanged(int guild_id, int type, const void *data, int len) {
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
	WPACKETW( 0) = 0x383e;
	WPACKETL( 2) = g->guild_id;
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
int mapif_parse_CreateGuild(int fd, int account_id, char *name, struct guild_member *master) {
	struct guild *g;
	int i;

	for(i = 0; i < 24 && name[i]; i++) {
		if (!(name[i] & 0xe0) || name[i] == 0x7f) {
			printf("int_guild: illeagal guild name [%s]\n", name);
			mapif_guild_created(fd, account_id, NULL);
			return 0;
		}
	}

	if ((g = search_guildname(name)) != NULL) {
		printf("int_guild_create: same name guild exists [%s]\n", name);
		mapif_guild_created(fd, account_id, NULL);
		return 0;
	}
	CALLOC(g, struct guild, 1);
	g->guild_id = guild_newid++;
	strncpy(g->name, name, 24);
	strncpy(g->master, master->name, 24);
	memcpy(&g->member[0], master, sizeof(struct guild_member));

	g->position[0].mode = 0x11;
	strncpy(g->position[                    0].name, "GuildMaster", 24);
	strncpy(g->position[MAX_GUILDPOSITION - 1].name, "Newbie", 24);
	for(i = 1; i < MAX_GUILDPOSITION - 1; i++)
		sprintf(g->position[i].name, "Position %d", i + 1);

	// Initialize guild property
	g->max_member = 16;
	g->average_lv = master->lv;
//	g->castle_id = -1; // castle_id in guild_db is not used (a guild can have more than 1 castle)
	for(i = 0; i < MAX_GUILDSKILL; i++)
		g->skill[i].id = i + GD_SKILLBASE;

	numdb_insert(guild_db, (CPU_INT)g->guild_id, g);

	// Report to client
	mapif_guild_created(fd, account_id, g);
	mapif_guild_info(fd, g);

	inter_log("guild %s (id=%d) created by master %s (id=%d)" RETCODE, name, g->guild_id, master->name, master->account_id);

	return 0;
}

// ギルド情報要求
int mapif_parse_GuildInfo(int fd, int guild_id) { // 0x3031 <guild_id>.L - ask for guild
	struct guild *g;

	g = numdb_search(guild_db, (CPU_INT)guild_id);
	if (g != NULL && g->guild_id > 0) {
		guild_calcinfo(g);
		mapif_guild_info(fd, g);
	} else
		mapif_guild_noinfo(fd, guild_id);

	return 0;
}

// Add member to guild
int mapif_parse_GuildAddMember(int fd, int guild_id, struct guild_member *m) {
	struct guild *g;
	int i;

	g = numdb_search(guild_db, (CPU_INT)guild_id);
	if (g == NULL) {
		mapif_guild_memberadded(fd, guild_id, m->account_id, m->char_id, 1);
		return 0;
	}

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

			return 0;
		}
	}
	mapif_guild_memberadded(fd, guild_id, m->account_id, m->char_id, 1);

	return 0;
}

// Delete member from guild
int mapif_parse_GuildLeave(int fd, int guild_id, int account_id, int char_id, int flag, const char *mes) {
	struct guild *g;
	int i, j;

	g = numdb_search(guild_db, (CPU_INT)guild_id);
	if (g != NULL && g->guild_id > 0) {
		for(i = 0; i < MAX_GUILD; i++) {
			// just check char_id (char_id is unique)
			if (g->member[i].char_id == char_id) {
//				printf("%d %d\n", i, (int)(&g->member[i]));
//				printf("%d %s\n", i, g->member[i].name);

				if (flag) {	// 追放の場合追放リストに入れる
					for(j = 0; j < MAX_GUILDEXPLUSION; j++) {
						if (g->explusion[j].account_id == 0)
							break;
					}
					if (j == MAX_GUILDEXPLUSION) {	// 一杯なので古いのを消す
						for(j = 0; j < MAX_GUILDEXPLUSION - 1; j++)
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
					mapif_guild_info(-1, g);// まだ人がいるのでデータ送信

				return 0;
			}
		}
	}

	return 0;
}

// オンライン/Lv更新
int mapif_parse_GuildChangeMemberInfoShort(int fd, int guild_id, int account_id, int char_id, int online, int lv, int class) {
	struct guild *g;
	int i, alv, c;

	g = numdb_search(guild_db, (CPU_INT)guild_id);
	if (g == NULL)
		return 0;

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
			mapif_guild_memberinfoshort(g, i);
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

	return 0;
}

int guild_break_sub(void *key, void *data, va_list ap)
{
	struct guild *g = (struct guild *)data;
	int guild_id = va_arg(ap, int);
	register int i;

	for(i = 0; i < MAX_GUILDALLIANCE; i++)
	{
		if(g->alliance[i].guild_id == guild_id)
			g->alliance[i].guild_id = 0;
	}

	return 0;
}

// BreakGuild
int mapif_parse_BreakGuild(int fd, int guild_id) {
	int i;
	struct guild *g;

	g = numdb_search(guild_db, (CPU_INT)guild_id);
	if (g == NULL)
		return 0;

	numdb_foreach(guild_db, guild_break_sub, guild_id);
	numdb_erase(guild_db, (CPU_INT)guild_id);

	inter_guild_storage_delete(guild_id);
	mapif_guild_broken(guild_id, 0);
	for (i = 0; i < MAX_GUILDCASTLE; i++)
		if (castle_db[i].guild_id == g->guild_id)
			mapif_parse_GuildCastleDataSave(0, i, 1, 0); // set guild_id (1) to 0

	inter_log("guild %s (id=%d) broken." RETCODE, g->name, guild_id);
	FREE(g);

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

	g = numdb_search(guild_db, (CPU_INT)guild_id);
	if (g == NULL)
		return 0;

	if (g->guild_id <= 0)
		return 0;

	switch(type) {
	case GBI_GUILDLV:
//		printf("GBI_GUILDLV\n");
		if (dw > 0 && g->guild_lv + dw <= 50) {
			g->guild_lv+=dw;
			g->skill_point+=dw;
		} else if (dw < 0 && g->guild_lv + dw >= 1)
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
	int i;
	struct guild *g;

	g = numdb_search(guild_db, (CPU_INT)guild_id);
	if (g == NULL)
		return 0;

	//printf("GuildMemberInfoChange %s.\n", (type == GMI_EXP) ? "GMI_EXP" : "OTHER");

	for(i = 0; i < g->max_member; i++)
		if (g->member[i].account_id == account_id && g->member[i].char_id == char_id)
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

	g = numdb_search(guild_db, (CPU_INT)guild_id);
	if (g == NULL)
		return 0;

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
	int idx;
	struct guild *g;

	idx = skill_num - GD_SKILLBASE;
	if (idx < 0 || idx >= MAX_GUILDSKILL)
		return 0;

	g = numdb_search(guild_db, (CPU_INT)guild_id);
	if (g == NULL)
		return 0;
	//printf("GuildSkillUp\n");

	if (g->skill_point > 0 && g->skill[idx].id > 0 && g->skill[idx].lv < 10) {
		g->skill[idx].lv++;
		g->skill_point--;
		if (guild_calcinfo(g) == 0)
			mapif_guild_info(-1, g);
		mapif_guild_skillupack(guild_id, skill_num, account_id);
		printf("int_guild: skill %d up\n", skill_num);
	}

	return 0;
}

// ギルド同盟要求
int mapif_parse_GuildAlliance(int fd, int guild_id1, int guild_id2, int account_id1, int account_id2, int flag) {
	struct guild *g[2];
	int j, i;

	if (guild_id1 <= 0 || guild_id2 <= 0)
		return 0;

	g[0] = numdb_search(guild_db, (CPU_INT)guild_id1);
	g[1] = numdb_search(guild_db, (CPU_INT)guild_id2);
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
	} else {	// 関係解消
		for(i = 0; i < 2 - (flag & 1); i++) {
			for(j = 0; j < MAX_GUILDALLIANCE; j++)
				if (g[i]->alliance[j].guild_id == g[1-i]->guild_id && g[i]->alliance[j].opposition == (flag & 1)) {
					g[i]->alliance[j].guild_id = 0;
					break;
				}
		}
	}
	mapif_guild_alliance(guild_id1, guild_id2, account_id1, account_id2, flag, g[0]->name, g[1]->name);

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

	g = numdb_search(guild_db, (CPU_INT)guild_id);
	if (g == NULL)
		return 0;

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

	g = numdb_search(guild_db, (CPU_INT)guild_id);
	if (g == NULL)
		return 0;

	if (g->guild_id <= 0)
		return 0;

	memcpy(g->emblem_data, data, len);
	g->emblem_len = len;
	g->emblem_id++;

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
	case 18: return mapif_guild_castle_dataload(gc->castle_id, idx, gc->Ghp0);	// guardian HP [Valaris]
	case 19: return mapif_guild_castle_dataload(gc->castle_id, idx, gc->Ghp1);
	case 20: return mapif_guild_castle_dataload(gc->castle_id, idx, gc->Ghp2);
	case 21: return mapif_guild_castle_dataload(gc->castle_id, idx, gc->Ghp3);
	case 22: return mapif_guild_castle_dataload(gc->castle_id, idx, gc->Ghp4);
	case 23: return mapif_guild_castle_dataload(gc->castle_id, idx, gc->Ghp5);
	case 24: return mapif_guild_castle_dataload(gc->castle_id, idx, gc->Ghp6);
	case 25: return mapif_guild_castle_dataload(gc->castle_id, idx, gc->Ghp7);	// end additions [Valaris]

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
	return guild_check_conflict(guild_id, account_id, char_id);
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
	case 0x3037: mapif_parse_GuildMessage(fd, RFIFOL(fd,4), RFIFOL(fd,8), RFIFOP(fd,12), RFIFOW(fd,2)-12); break;
	case 0x3038: mapif_parse_GuildCheck(fd, RFIFOL(fd,2), RFIFOL(fd,6), RFIFOL(fd,10)); break;
	case 0x3039: mapif_parse_GuildBasicInfoChange(fd, RFIFOL(fd,4), RFIFOW(fd,8), RFIFOP(fd,10), RFIFOW(fd,2)-10); break;
	case 0x303A: mapif_parse_GuildMemberInfoChange(fd, RFIFOL(fd,4), RFIFOL(fd,8), RFIFOL(fd,12), RFIFOW(fd,16), RFIFOP(fd,18), RFIFOW(fd,2)-18); break;
	case 0x303B: mapif_parse_GuildPosition(fd, RFIFOL(fd,4), RFIFOL(fd,8), (struct guild_position *)RFIFOP(fd,12)); break;
	case 0x303C: mapif_parse_GuildSkillUp(fd, RFIFOL(fd,2), RFIFOL(fd,6), RFIFOL(fd,10)); break;
	case 0x303D: mapif_parse_GuildAlliance(fd, RFIFOL(fd,2), RFIFOL(fd,6), RFIFOL(fd,10), RFIFOL(fd,14), RFIFOB(fd,18)); break;
	case 0x303E: mapif_parse_GuildNotice(fd, RFIFOL(fd,2), RFIFOP(fd,6), RFIFOP(fd,66)); break;
	case 0x303F: mapif_parse_GuildEmblem(fd, RFIFOW(fd,2)-12, RFIFOL(fd,4), RFIFOL(fd,8), RFIFOP(fd,12)); break;
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
