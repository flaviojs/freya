// Copyright (c) Freya Development Team - Licensed under GNU GPL
// For more information, see License.txt in the main folder

#include <config.h>

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>

#ifdef __WIN32
# define __USE_W32_SOCKETS
# include <windows.h>
#else
# include <sys/socket.h>
# include <netinet/in.h>
# include <sys/ioctl.h>
# include <arpa/inet.h>
#endif

#include "char.h"
#include "../common/malloc.h"
#include "../common/utils.h"
#include "../common/db_mysql.h"

#ifdef MEMWATCH
#include "memwatch.h"
#endif

char pet_txt[256] = "save/pet.txt";
char storage_txt[256] = "save/storage.txt";

MYSQL mysql_handle;

int db_conv_server_port = 3306;
char db_conv_server_ip[16] = "127.0.0.1";
char db_conv_server_id[32] = "ragnarok";
char db_conv_server_pw[32] = "ragnarok";
char db_conv_server_logindb[32] = "ragnarok";

struct storage *storage = NULL;

char char_txt[256];

char t_name[256];

int char_id_count = 100000;
struct mmo_charstatus *char_dat;
int char_num, char_max;
struct global_reg_db { // same size of char_dat, to conserv global reg
	unsigned short global_reg_num; // 0-700 (GLOBAL_REG_NUM)
	struct global_reg *global_reg; // same size of char_dat, to conserv global reg
} *global_reg_db = NULL;

static inline int inter_pet_fromstr(char *str, struct s_pet *p) {
	int s;
	int tmp_int[16];
	char tmp_str[256];

	memset(p, 0, sizeof(struct s_pet));

//	printf("sscanf pet main info\n");
	s = sscanf(str, "%d, %d,%[^\t]\t%d, %d, %d, %d, %d, %d, %d, %d, %d", &tmp_int[0], &tmp_int[1], tmp_str, &tmp_int[2],
	           &tmp_int[3], &tmp_int[4], &tmp_int[5], &tmp_int[6], &tmp_int[7], &tmp_int[8], &tmp_int[9], &tmp_int[10]);

	if (s != 12)
		return 1;

	p->pet_id = tmp_int[0];
	p->class = tmp_int[1];
	strncpy(p->name, tmp_str, 24);
	p->account_id = tmp_int[2];
	p->char_id = tmp_int[3];
	p->level = tmp_int[4];
	p->egg_id = tmp_int[5];
	p->equip = tmp_int[6];
	p->intimate = tmp_int[7];
	p->hungry = tmp_int[8];
	p->rename_flag = tmp_int[9];
	p->incuvate = tmp_int[10];

	if (p->hungry < 0)
		p->hungry = 0;
	else if (p->hungry > 100)
		p->hungry = 100;
	if (p->intimate < 0)
		p->intimate = 0;
	else if (p->intimate > 1000)
		p->intimate = 1000;

	return 0;
}

//---------------------------------------------------------
static inline void inter_pet_tosql(int pet_id, struct s_pet *p) {
	//`pet` (`pet_id`, `class`,`name`,`account_id`,`char_id`,`level`,`egg_id`,`equip`,`intimate`,`hungry`,`rename_flag`,`incuvate`)

	mysql_escape_string(t_name, p->name, strlen(p->name));
	if (p->hungry < 0)
		p->hungry = 0;
	else if (p->hungry > 100)
		p->hungry = 100;
	if (p->intimate < 0)
		p->intimate = 0;
	else if (p->intimate > 1000)
		p->intimate = 1000;
	sql_request("SELECT * FROM `pet` WHERE `pet_id`='%d'", pet_id);
	if (sql_get_row()) //no row -> insert
		sql_request("INSERT INTO `pet` (`pet_id`, `class`,`name`,`account_id`,`char_id`,`level`,`egg_id`,`equip`,`intimate`,`hungry`,`rename_flag`,`incuvate`) VALUES ('%d', '%d', '%s', '%d', '%d', '%d', '%d', '%d', '%d', '%d', '%d', '%d')",
		            p->pet_id, p->class, t_name, p->account_id, p->char_id, p->level, p->egg_id,
		            p->equip, p->intimate, p->hungry, p->rename_flag, p->incuvate);
	else //row reside -> updating
		sql_request("UPDATE `pet` SET `class`='%d',`name`='%s',`account_id`='%d',`char_id`='%d',`level`='%d',`egg_id`='%d',`equip`='%d',`intimate`='%d',`hungry`='%d',`rename_flag`='%d',`incuvate`='%d' WHERE `pet_id`='%d'",
		            p->class, t_name, p->account_id, p->char_id, p->level, p->egg_id,
		            p->equip, p->intimate, p->hungry, p->rename_flag, p->incuvate, p->pet_id);

	printf("pet dump success! - %d:%s.\n", pet_id, p->name);

	return;
}

static inline void storage_tosql(int account_id, struct storage *p) {
	// id -> DB dump
	// storage {`account_id`/`id`/`nameid`/`amount`/`equip`/`identify`/`refine`/`attribute`/`card0`/`card1`/`card2`/`card3`}
	int i, j;

	j = 0;

	//printf("starting storage dump to DB - id: %d\n", account_id);

	//delete old data.
	sql_request("DELETE FROM `storage` WHERE `account_id`='%d'",account_id);

	//printf("all storage item was deleted ok\n");

	for(i = 0; i < MAX_STORAGE; i++) {
		//printf("save storage num: %d (%d:%d)\n", i, p->storage[i].nameid, p->storage[i].amount);

		if ((p->storage[i].nameid) && (p->storage[i].amount)) {
			sql_request("INSERT INTO `storage` (`account_id`,`nameid`,`amount`,`equip`,`identify`,`refine`,`attribute`,`card0`,`card1`,`card2`,`card3`) VALUES ('%d', '%d', '%d', '%d', '%d', '%d', '%d', '%d', '%d', '%d', '%d')",
			            p->account_id, p->storage[i].nameid, p->storage[i].amount, p->storage[i].equip,
			            p->storage[i].identify, p->storage[i].refine, p->storage[i].attribute,
			            p->storage[i].card[0], p->storage[i].card[1], p->storage[i].card[2], p->storage[i].card[3]);
			j++;
		}
	}

	printf("storage dump to DB - id: %d (total: %d)\n", account_id, j);

	return;
}

// char to storage
static inline void storage_fromstr(const char *str, struct storage *p) {
	int tmp_int[11];
	int set, next, len, i;

	set = sscanf(str, "%d, %d%n", &tmp_int[0], &tmp_int[1], &next);
	p->storage_amount = tmp_int[1];

	if (set != 2)
		return;
	if (str[next] == '\n' || str[next] == '\r')
		return;
	next++;
	for(i = 0; str[next] && str[next] != '\t'; i++) {
		if (sscanf(str + next, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d%n",
		    &tmp_int[0], &tmp_int[1], &tmp_int[2], &tmp_int[3],
		    &tmp_int[4], &tmp_int[5], &tmp_int[6],
		    &tmp_int[7], &tmp_int[8], &tmp_int[9], &tmp_int[10], &len) == 11) {
			p->storage[i].id = tmp_int[0];
			p->storage[i].nameid = tmp_int[1];
			p->storage[i].amount = tmp_int[2];
			p->storage[i].equip = tmp_int[3];
			p->storage[i].identify = tmp_int[4];
			p->storage[i].refine = tmp_int[5];
			p->storage[i].attribute = tmp_int[6];
			p->storage[i].card[0] = tmp_int[7];
			p->storage[i].card[1] = tmp_int[8];
			p->storage[i].card[2] = tmp_int[9];
			p->storage[i].card[3] = tmp_int[10];
			if (p->storage[i].refine > 10)
				p->storage[i].refine = 10;
			else if (p->storage[i].refine < 0)
				p->storage[i].refine = 0;
			next += len;
			if (str[next] == ' ')
				next++;
		} else
			return;
	}

	return;
}

/////////////////////////////////
static inline int mmo_char_fromstr(char *str, struct mmo_charstatus *p, int idx) {
	int tmp_int[256];
	char tmp_str[256];
	int set, next, len, i, j;
	struct global_reg reg[GLOBAL_REG_NUM];

	// initilialise character
	memset(p, 0, sizeof(struct mmo_charstatus));

	// If it's not char structure of version 1008 and after
	if ((set = sscanf(str, "%d\t%d,%d\t%[^\t]\t%d,%d,%d\t%d,%d,%d\t%d,%d,%d,%d\t%d,%d,%d,%d,%d,%d\t%d,%d"
	   "\t%d,%d,%d\t%d,%d,%d\t%d,%d,%d\t%d,%d,%d,%d,%d"
	   "\t%[^,],%d,%d\t%[^,],%d,%d,%d%n",
	   &tmp_int[0], &tmp_int[1], &tmp_int[2], p->name, //
	   &tmp_int[3], &tmp_int[4], &tmp_int[5],
	   &tmp_int[6], &tmp_int[7], &tmp_int[8],
	   &tmp_int[9], &tmp_int[10], &tmp_int[11], &tmp_int[12],
	   &tmp_int[13], &tmp_int[14], &tmp_int[15], &tmp_int[16], &tmp_int[17], &tmp_int[18],
	   &tmp_int[19], &tmp_int[20],
	   &tmp_int[21], &tmp_int[22], &tmp_int[23], //
	   &tmp_int[24], &tmp_int[25], &tmp_int[26],
	   &tmp_int[27], &tmp_int[28], &tmp_int[29],
	   &tmp_int[30], &tmp_int[31], &tmp_int[32], &tmp_int[33], &tmp_int[34],
	   p->last_point.map, &tmp_int[35], &tmp_int[36], //
	   p->save_point.map, &tmp_int[37], &tmp_int[38], &tmp_int[39], &next)) != 43) {
		tmp_int[39] = 0; // partner id
		// If not char structure from version 384 to 1007
		if ((set = sscanf(str, "%d\t%d,%d\t%[^\t]\t%d,%d,%d\t%d,%d,%d\t%d,%d,%d,%d\t%d,%d,%d,%d,%d,%d\t%d,%d"
		   "\t%d,%d,%d\t%d,%d,%d\t%d,%d,%d\t%d,%d,%d,%d,%d"
		   "\t%[^,],%d,%d\t%[^,],%d,%d%n",
		   &tmp_int[0], &tmp_int[1], &tmp_int[2], p->name, //
		   &tmp_int[3], &tmp_int[4], &tmp_int[5],
		   &tmp_int[6], &tmp_int[7], &tmp_int[8],
		   &tmp_int[9], &tmp_int[10], &tmp_int[11], &tmp_int[12],
		   &tmp_int[13], &tmp_int[14], &tmp_int[15], &tmp_int[16], &tmp_int[17], &tmp_int[18],
		   &tmp_int[19], &tmp_int[20],
		   &tmp_int[21], &tmp_int[22], &tmp_int[23], //
		   &tmp_int[24], &tmp_int[25], &tmp_int[26],
		   &tmp_int[27], &tmp_int[28], &tmp_int[29],
		   &tmp_int[30], &tmp_int[31], &tmp_int[32], &tmp_int[33], &tmp_int[34],
		   p->last_point.map, &tmp_int[35], &tmp_int[36], //
		   p->save_point.map, &tmp_int[37], &tmp_int[38], &next)) != 42) {
			// It's char structure of a version before 384
			tmp_int[26] = 0; // pet id
			set = sscanf(str, "%d\t%d,%d\t%[^\t]\t%d,%d,%d\t%d,%d,%d\t%d,%d,%d,%d\t%d,%d,%d,%d,%d,%d\t%d,%d"
			      "\t%d,%d,%d\t%d,%d\t%d,%d,%d\t%d,%d,%d,%d,%d"
			      "\t%[^,],%d,%d\t%[^,],%d,%d%n",
			      &tmp_int[0], &tmp_int[1], &tmp_int[2], p->name, //
			      &tmp_int[3], &tmp_int[4], &tmp_int[5],
			      &tmp_int[6], &tmp_int[7], &tmp_int[8],
			      &tmp_int[9], &tmp_int[10], &tmp_int[11], &tmp_int[12],
			      &tmp_int[13], &tmp_int[14], &tmp_int[15], &tmp_int[16], &tmp_int[17], &tmp_int[18],
			      &tmp_int[19], &tmp_int[20],
			      &tmp_int[21], &tmp_int[22], &tmp_int[23], //
			      &tmp_int[24], &tmp_int[25], //
			      &tmp_int[27], &tmp_int[28], &tmp_int[29],
			      &tmp_int[30], &tmp_int[31], &tmp_int[32], &tmp_int[33], &tmp_int[34],
			      p->last_point.map, &tmp_int[35], &tmp_int[36], //
			      p->save_point.map, &tmp_int[37], &tmp_int[38], &next);
			set += 2;
			//printf("char: old char data ver.1\n");
		// Char structure of version 1007 or older
		} else {
			set++;
			//printf("char: old char data ver.2\n");
		}
	// Char structure of version 1008+
	} else {
		//printf("char: new char data ver.3\n");
	}
	if (set != 43)
		return 0;

	p->char_id = tmp_int[0];
	p->account_id = tmp_int[1];
	p->char_num = tmp_int[2];
	p->class = tmp_int[3];
	p->base_level = tmp_int[4];
	p->job_level = tmp_int[5];
	p->base_exp = tmp_int[6];
	p->job_exp = tmp_int[7];
	p->zeny = tmp_int[8];
	p->hp = tmp_int[9];
	p->max_hp = tmp_int[10];
	p->sp = tmp_int[11];
	p->max_sp = tmp_int[12];
	p->str = tmp_int[13];
	p->agi = tmp_int[14];
	p->vit = tmp_int[15];
	p->int_ = tmp_int[16];
	p->dex = tmp_int[17];
	p->luk = tmp_int[18];
	p->status_point = tmp_int[19];
	p->skill_point = tmp_int[20];
	p->option = tmp_int[21];
	p->karma = tmp_int[22];
	p->manner = tmp_int[23];
	p->party_id = tmp_int[24];
	p->guild_id = tmp_int[25];
	p->pet_id = tmp_int[26];
	p->hair = tmp_int[27];
	p->hair_color = tmp_int[28];
	p->clothes_color = tmp_int[29];
	p->weapon = tmp_int[30];
	p->shield = tmp_int[31];
	p->head_top = tmp_int[32];
	p->head_mid = tmp_int[33];
	p->head_bottom = tmp_int[34];
	p->last_point.x = tmp_int[35];
	p->last_point.y = tmp_int[36];
	p->save_point.x = tmp_int[37];
	p->save_point.y = tmp_int[38];
	p->partner_id = tmp_int[39];

	// Some checks
	for(i = 0; i < char_num; i++) {
		if (char_dat[i].char_id == p->char_id) {
			printf(CL_RED "mmo_auth_init: ******Error: a character has an identical id to another.\n");
			printf(       "               character id #%d -> new character not read.\n", p->char_id);
			printf(       "               Character saved in log file.\n" CL_RESET);
			return -1;
		} else if (strcmp(char_dat[i].name, p->name) == 0) {
			printf(CL_RED "mmo_auth_init: ******Error: character name already exists.\n");
			printf(       "               character name '%s' -> new character not read.\n", p->name);
			printf(       "               Character saved in log file.\n" CL_RESET);
			return -2;
		}
	}

	if (str[next] == '\n' || str[next] == '\r')
		return 1;	// 新規データ

	next++;

	i = 0;
	while(str[next] && str[next] != '\t' && i < MAX_PORTAL_MEMO) {
		set = sscanf(str + next, "%[^,],%d,%d%n", p->memo_point[i].map, &tmp_int[0], &tmp_int[1], &len);
		if (set != 3)
			return -3;
		p->memo_point[i].x = tmp_int[0];
		p->memo_point[i].y = tmp_int[1];
		i++;
		next += len;
		if (str[next] == ' ')
			next++;
	}

	next++;

	for(i = 0; str[next] && str[next] != '\t'; i++) {
		if (sscanf(str + next, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d%n",
		    &tmp_int[0], &tmp_int[1], &tmp_int[2], &tmp_int[3],
		    &tmp_int[4], &tmp_int[5], &tmp_int[6],
		    &tmp_int[7], &tmp_int[8], &tmp_int[9], &tmp_int[10], &len) != 11) {
			// invalid structure
			return -4;
		}
		p->inventory[i].id = tmp_int[0];
		p->inventory[i].nameid = tmp_int[1];
		p->inventory[i].amount = tmp_int[2];
		p->inventory[i].equip = tmp_int[3];
		p->inventory[i].identify = tmp_int[4];
		p->inventory[i].refine = tmp_int[5];
		p->inventory[i].attribute = tmp_int[6];
		p->inventory[i].card[0] = tmp_int[7];
		p->inventory[i].card[1] = tmp_int[8];
		p->inventory[i].card[2] = tmp_int[9];
		p->inventory[i].card[3] = tmp_int[10];
		if (p->inventory[i].refine > 10)
			p->inventory[i].refine = 10;
		else if (p->inventory[i].refine < 0)
			p->inventory[i].refine = 0;
		next += len;
		if (str[next] == ' ')
			next++;
	}

	next++;

	for(i = 0; str[next] && str[next] != '\t'; i++) {
		if (sscanf(str + next, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d%n",
		    &tmp_int[0], &tmp_int[1], &tmp_int[2], &tmp_int[3],
		    &tmp_int[4], &tmp_int[5], &tmp_int[6],
		    &tmp_int[7], &tmp_int[8], &tmp_int[9], &tmp_int[10], &len) != 11) {
			// invalid structure
			return -5;
		}
		p->cart[i].id = tmp_int[0];
		p->cart[i].nameid = tmp_int[1];
		p->cart[i].amount = tmp_int[2];
		p->cart[i].equip = tmp_int[3];
		p->cart[i].identify = tmp_int[4];
		p->cart[i].refine = tmp_int[5];
		p->cart[i].attribute = tmp_int[6];
		p->cart[i].card[0] = tmp_int[7];
		p->cart[i].card[1] = tmp_int[8];
		p->cart[i].card[2] = tmp_int[9];
		p->cart[i].card[3] = tmp_int[10];
		if (p->cart[i].refine > 10)
			p->cart[i].refine = 10;
		else if (p->cart[i].refine < 0)
			p->cart[i].refine = 0;
		next += len;
		if (str[next] == ' ')
			next++;
	}

	next++;

	while(str[next] && str[next] != '\t') {
		set = sscanf(str + next, "%d,%d%n", &i, &tmp_int[1], &len);
		if (set != 2)
			return -6;
		if (i >= 0 && i < MAX_SKILL && tmp_int[1] >= 0) {
			p->skill[i].id = i;
			p->skill[i].lv = tmp_int[1];
		}
		next += len;
		if (str[next] == ' ')
			next++;
	}

	next++;

	i = 0;
	while(str[next] && str[next] != '\t' && str[next] != '\n' && str[next] != '\r' && i < GLOBAL_REG_NUM) {
		memset(tmp_str, 0, sizeof(tmp_str));
		if (sscanf(str + next, "%[^,],%d%n", tmp_str, &tmp_int[0], &len) != 2) {
			// because some scripts are not correct, the str can be "". So, we must check that.
			// If it's, we must not refuse the character, but just this REG value.
			// Character line will have something like: nov_2nd_cos,9 ,9 nov_1_2_cos_c,1 (here, ,9 is not good)
			if (str[next] != ',' || sscanf(str + next, ",%d%n", &tmp_int[0], &len) != 1)
				return -7;
		}
		if (tmp_str[0] && tmp_int[0] != 0) {
			// check if global_reg already exist
			for (j = 0; j < i; j++)
				if (strcmp(reg[j].str, tmp_str) == 0)
					break;
			// if not found
			if (j == i) {
				strncpy(reg[i].str, tmp_str, 32);
				reg[i].str[32] = '\0';
				reg[i].value = tmp_int[0];
				i++;
			}
		}
		next += len;
		if (str[next] == ' ')
			next++;
	}
	if (i > 0) {
		global_reg_db[idx].global_reg_num = i;
		MALLOC(global_reg_db[idx].global_reg, struct global_reg, i);
		memcpy(global_reg_db[idx].global_reg, reg, sizeof(struct global_reg) * i);
	}

	return 1;
}

//==========================================================================================================
static inline void mmo_char_tosql(struct mmo_charstatus *p, int idx) {
	int i;

	printf("request save char data... (%d)\n", p->char_id);

	//`char` (`char_id`,`account_id`,`char_num`,`name`,`class`,`base_level`,`job_level`,`base_exp`,`job_exp`,`zeny`, //9
	//`str`,`agi`,`vit`,`int`,`dex`,`luk`, //15
	//`max_hp`,`hp`,`max_sp`,`sp`,`status_point`,`skill_point`, //21
	//`option`,`karma`,`manner`,`party_id`,`guild_id`,`pet_id`, //27
	//`hair`,`hair_color`,`clothes_color`,`weapon`,`shield`,`head_top`,`head_mid`,`head_bottom`, //35
	//`last_map`,`last_x`,`last_y`,`save_map`,`save_x`,`save_y`)
	sql_request("INSERT INTO `char` SET `char_id`='%d',`account_id`='%d',`char_num`='%d',`name`='%s',`class`='%d',`base_level`='%d',`job_level`='%d',"
	            "`base_exp`='%d',`job_exp`='%d',`zeny`='%d',"
	            "`max_hp`='%d',`hp`='%d',`max_sp`='%d',`sp`='%d',`status_point`='%d',`skill_point`='%d',"
	            "`str`='%d',`agi`='%d',`vit`='%d',`int`='%d',`dex`='%d',`luk`='%d',"
	            "`option`='%d',`karma`='%d',`manner`='%d',`party_id`='%d',`guild_id`='%d',`pet_id`='%d',"
	            "`hair`='%d',`hair_color`='%d',`clothes_color`='%d',`weapon`='%d',`shield`='%d',`head_top`='%d',`head_mid`='%d',`head_bottom`='%d',"
	            "`last_map`='%s',`last_x`='%d',`last_y`='%d',`save_map`='%s',`save_x`='%d',`save_y`='%d',`partner_id` = '%d'",
	            p->char_id,p->account_id,p->char_num,p->name,p->class, p->base_level, p->job_level,
	            p->base_exp, p->job_exp, p->zeny,
	            p->max_hp, p->hp, p->max_sp, p->sp, p->status_point, p->skill_point,
	            p->str, p->agi, p->vit, p->int_, p->dex, p->luk,
	            p->option, p->karma, p->manner, p->party_id, p->guild_id, p->pet_id,
	            p->hair, p->hair_color, p->clothes_color,
	            p->weapon, p->shield, p->head_top, p->head_mid, p->head_bottom,
	            p->last_point.map, p->last_point.x, p->last_point.y,
	            p->save_point.map, p->save_point.x, p->save_point.y, p->partner_id
	);

	//`memo` (`memo_id`,`char_id`,`map`,`x`,`y`)
	sql_request("DELETE FROM `memo` WHERE `char_id`='%d'", p->char_id);

	//insert here.
	for(i = 0; i < MAX_PORTAL_MEMO; i++) {
		if (p->memo_point[i].map[0]) {
			sql_request("INSERT INTO `memo`(`char_id`,`map`,`x`,`y`) VALUES ('%d', '%s', '%d', '%d')",
			            p->char_id, p->memo_point[i].map, p->memo_point[i].x, p->memo_point[i].y);
		}
	}
	//`inventory` (`id`,`char_id`, `nameid`, `amount`, `equip`, `identify`, `refine`, `attribute`, `card0`, `card1`, `card2`, `card3`)
	sql_request("DELETE FROM `inventory` WHERE `char_id`='%d'", p->char_id);

	//insert here.
	for(i = 0; i < MAX_INVENTORY; i++) {
		if (p->inventory[i].nameid) {
			sql_request("INSERT INTO `inventory`(`id`,`char_id`, `nameid`, `amount`, `equip`, `identify`, `refine`, `attribute`, `card0`, `card1`, `card2`, `card3`)"
			            " VALUES ('%d', '%d', '%d', '%d', '%d', '%d', '%d', '%d', '%d', '%d', '%d', '%d')",
			            p->inventory[i].id, p->char_id,p->inventory[i].nameid, p->inventory[i].amount, p->inventory[i].equip,
			            p->inventory[i].identify, p->inventory[i].refine, p->inventory[i].attribute,
			            p->inventory[i].card[0], p->inventory[i].card[1], p->inventory[i].card[2], p->inventory[i].card[3]);
		}
	}

	//`cart_inventory` (`id`,`char_id`, `nameid`, `amount`, `equip`, `identify`, `refine`, `attribute`, `card0`, `card1`, `card2`, `card3`)
	sql_request("DELETE FROM `cart_inventory` WHERE `char_id`='%d'", p->char_id);

	//insert here.
	for(i = 0; i < MAX_CART; i++) {
		if (p->cart[i].nameid) {
			sql_request("INSERT INTO `cart_inventory`(`id`,`char_id`, `nameid`, `amount`, `equip`, `identify`, `refine`, `attribute`, `card0`, `card1`, `card2`, `card3`)"
			            " VALUES ('%d', '%d', '%d', '%d', '%d', '%d', '%d', '%d', '%d', '%d', '%d', '%d')",
			            p->cart[i].id, p->char_id, p->cart[i].nameid, p->cart[i].amount, p->cart[i].equip,
			            p->cart[i].identify, p->cart[i].refine, p->cart[i].attribute,
			            p->cart[i].card[0], p->cart[i].card[1], p->cart[i].card[2], p->cart[i].card[3]);
		}
	}

	//`skill` (`char_id`, `id`, `lv`)
	sql_request("DELETE FROM `skill` WHERE `char_id`='%d'", p->char_id);

	//insert here.
	for(i = 0; i < MAX_SKILL; i++) {
		if (p->skill[i].id) {
			if (p->skill[i].id && p->skill[i].flag != 1 && p->skill[i].flag != 13) { // flag: 0 (normal), 1 (only card), 2-12 (card and skill (skill level +2)), 13 (cloneskill)
				sql_request("INSERT INTO `skill`(`char_id`,`id`, `lv`) VALUES ('%d', '%d', '%d')",
				            p->char_id, p->skill[i].id, (p->skill[i].flag == 0) ? p->skill[i].lv : p->skill[i].flag - 2);
			}
		}
	}

	//`global_reg_value` (`char_id`, `str`, `value`)
	sql_request("DELETE FROM `global_reg_value` WHERE `char_id`='%d'", p->char_id);
	for(i = 0; i < global_reg_db[idx].global_reg_num; i++) {
		if (global_reg_db[idx].global_reg[i].str[0] && global_reg_db[idx].global_reg[i].value != 0) {
			sql_request("INSERT INTO `global_reg_value` (`char_id`, `str`, `value`) VALUES ('%d', '%s','%d')",
			            p->char_id, global_reg_db[idx].global_reg[i].str, global_reg_db[idx].global_reg[i].value);
		}
	}

	printf("saving char is done... (%d)\n", p->char_id);

	return;
}
//==========================================================================================================

static inline void mmo_char_init(void) {
	char line[65536];
	struct s_pet *p;
	int ret;
	int i = 0, j, tmp_int[2], c = 0;
	char input;
	FILE *fp;

	//DB connection initialized
	mysql_init(&mysql_handle);
	printf("Connect DB server... (inter server)\n");
	if (!mysql_real_connect(&mysql_handle, db_conv_server_ip, db_conv_server_id, db_conv_server_pw,
	    db_conv_server_logindb, db_conv_server_port, (char *)NULL, 0)) {
		//pointer check
		printf("%s\n", mysql_error(&mysql_handle));
		exit(1);
	} else {
		printf("connect success! (inter server)\n");
	}

	printf("Warning : Make sure you backup your databases before continuing!\n");
	printf("\nDo you wish to convert your Character Database to SQL? (y/n) : ");
	input = getchar();
	if (input == 'y' || input == 'Y') {
		printf("\nConverting Character Database...\n");
		char_max = 256;
		CALLOC(char_dat, struct mmo_charstatus, 256);
		CALLOC(global_reg_db, struct global_reg_db, 256);
		fp = fopen("save/athena.txt", "r");
		if (fp == NULL)
			return;
		while(fgets(line, sizeof(line), fp)) { // fgets reads until maximum one less than size and add '\0' -> so, it's not necessary to add -1
			if ((line[0] == '/' && line[1] == '/') || line[0] == '\0' || line[0] == '\n' || line[0] == '\r')
				continue;
			// it's not necessary to remove 'carriage return ('\n' or '\r')
			if (char_num >= char_max) {
				char_max += 256;
				REALLOC(char_dat, struct mmo_charstatus, char_max);
				REALLOC(global_reg_db, struct global_reg_db, char_max);
				for(j = char_max - 256; j < char_max; j++) {
					global_reg_db[j].global_reg_num = 0;
					global_reg_db[j].global_reg = NULL;
				}
			}
			memset(&char_dat[char_num], 0, sizeof(struct mmo_charstatus));
			ret = mmo_char_fromstr(line, &char_dat[char_num], char_num);
			if (ret) {
				mmo_char_tosql(&char_dat[char_num], char_num);
				printf("convert %d -> DB\n", char_dat[char_num].char_id);
				if (char_dat[char_num].char_id >= char_id_count)
					char_id_count = char_dat[char_num].char_id + 1;
				char_num++;
			}
		}
		printf("char data convert end.\n");
		fclose(fp);
	}

	while(getchar() != '\n');

	printf("\nDo you wish to convert your Storage Database to SQL? (y/n) : ");
	input = getchar();
	if (input == 'y' || input == 'Y') {
		printf("\nConverting Storage Database...\n");
		fp = fopen(storage_txt, "r");
		if (fp == NULL) {
			printf("cant't read : %s\n",storage_txt);
			return;
		}

		while(fgets(line, sizeof(line), fp)) { // fgets reads until maximum one less than size and add '\0' -> so, it's not necessary to add -1
			if ((line[0] == '/' && line[1] == '/') || line[0] == '\0' || line[0] == '\n' || line[0] == '\r')
				continue;
			// it's not necessary to remove 'carriage return ('\n' or '\r')
			if (sscanf(line, "%d,%d", &tmp_int[0], &tmp_int[1]) == 2) {
				if (i == 0) {
					CALLOC(storage, struct storage, 1);
				}else{
					REALLOC(storage, struct storage, i + 1);
					memset(&storage[i], 0, sizeof(struct storage));
				}
				storage[i].account_id = tmp_int[0];
				storage_fromstr(line, &storage[i]);
				storage_tosql(tmp_int[0], &storage[i]); //to sql. (dump)
				i++;
			}
		}
		fclose(fp);
	}

	while(getchar() != '\n');

	printf("\nDo you wish to convert your Pet Database to SQL? (y/n) : ");
	input = getchar();
	if (input == 'y' || input == 'Y') {
		printf("\nConverting Pet Database...\n");
		if ((fp = fopen(pet_txt, "r")) == NULL)
			return;

		CALLOC(p, struct s_pet, 1);
		while(fgets(line, sizeof(line), fp)) { // fgets reads until maximum one less than size and add '\0' -> so, it's not necessary to add -1
			if ((line[0] == '/' && line[1] == '/') || line[0] == '\0' || line[0] == '\n' || line[0] == '\r')
				continue;
			// it's not necessary to remove 'carriage return ('\n' or '\r')
			if (inter_pet_fromstr(line, p) == 0 && p->pet_id > 0) {
				// pet dump
				inter_pet_tosql(p->pet_id, p);
			} else {
				printf("int_pet: broken data [%s] line %d.\n", pet_txt, c);
			}
			c++;
		}
		fclose(fp);
	}

	return;
}

static void inter_config_read(const char *cfgName) { // not inline, called too often
	char line[1024], w1[1024], w2[1024];
	FILE *fp;

	printf("start reading interserver configuration: %s.\n", cfgName);

	if ((fp = fopen(cfgName, "r")) == NULL) {
//		if ((fp = fopen("conf/inter_freya.conf", "r")) == NULL) { // not try default, possible infinite loop with import
			printf("File not found: %s.\n", cfgName);
			return;
//		}
	}
	while(fgets(line, sizeof(line), fp)) { // fgets reads until maximum one less than size and add '\0' -> so, it's not necessary to add -1
		if ((line[0] == '/' && line[1] == '/') || line[0] == '\0' || line[0] == '\n' || line[0] == '\r')
			continue;
		// it's not necessary to remove 'carriage return ('\n' or '\r')
		memset(w2, 0, sizeof(w2));
		if (sscanf(line,"%[^:]:%s", w1, w2) != 2)
			continue;

		if (strcasecmp(w1, "storage_txt") == 0) {
			memset(storage_txt, 0, sizeof(storage_txt));
			printf("set storage_txt : %s\n", w2);
			strncpy(storage_txt, w2, sizeof(storage_txt) - 1);
		} else if (strcasecmp(w1, "pet_txt") == 0) {
			memset(pet_txt, 0, sizeof(pet_txt));
			printf("set pet_txt : %s\n", w2);
			strncpy(pet_txt, w2, sizeof(pet_txt) - 1);
		}
		// add for DB connection
		else if (strcasecmp(w1, "db_conv_server_ip") == 0) {
			strcpy(db_conv_server_ip, w2);
			printf("set db_conv_server_ip : %s\n", w2);
		}
		else if (strcasecmp(w1, "db_conv_server_port") == 0) {
			db_conv_server_port = atoi(w2);
			printf("set db_conv_server_port : %s\n", w2);
		}
		else if (strcasecmp(w1, "db_conv_server_id") == 0) {
			strcpy(db_conv_server_id, w2);
			printf("set db_conv_server_id : %s\n", w2);
		}
		else if (strcasecmp(w1, "db_conv_server_pw") == 0) {
			strcpy(db_conv_server_pw, w2);
			printf("set db_conv_server_pw : %s\n", w2);
		}
		else if (strcasecmp(w1, "db_conv_server_logindb") == 0) {
			strcpy(db_conv_server_logindb, w2);
			printf("set db_conv_server_logindb : %s\n", w2);
// import
		} else if (strcasecmp(w1, "import") == 0) {
			printf("inter_config_read: Import file: %s.\n", w2);
			inter_config_read(w2);
		}
	}
	fclose(fp);

	printf("success reading interserver configuration.\n");

	return;
}

static void char_config_read(const char *cfgName) { // not inline, called too often
	char line[1024], w1[1024], w2[1024];
	FILE *fp;

	printf("start reading interserver configuration: %s.\n", cfgName);

	if ((fp = fopen(cfgName, "r")) == NULL) {
//		if ((fp = fopen("conf/char_freya.conf", "r")) == NULL) { // not try default, possible infinite loop with import
			printf("File not found: %s\n", cfgName);
			return;
//		}
	}

	while(fgets(line, sizeof(line), fp)) { // fgets reads until maximum one less than size and add '\0' -> so, it's not necessary to add -1
		if ((line[0] == '/' && line[1] == '/') || line[0] == '\0' || line[0] == '\n' || line[0] == '\r')
			continue;
		// it's not necessary to remove 'carriage return ('\n' or '\r')

		memset(w2, 0, sizeof(w2));
		if (sscanf(line,"%[^:]:%s", w1, w2) != 2)
			continue;
		if (strcasecmp(w1, "char_txt") == 0) {
			printf("set char_txt : %s\n", w2);
			strcpy(char_txt, w2);
// import
		} else if (strcasecmp(w1, "import") == 0) {
			printf("inter_config_read: Import file: %s.\n", w2);
			char_config_read(w2);
		}
	}
	fclose(fp);

	printf("reading configure done.....\n");

	return;
}

void do_init(const int argc, char **argv) {

	char_config_read((argc > 1) ? argv[1] : "conf/char_freya.conf");
	inter_config_read((argc > 2) ? argv[2] : "conf/inter_freya.conf");

	mmo_char_init();
	printf("all conversion success!\n");
	exit(0);

	return;
}
