// Copyright (c) Freya Development Team - Licensed under GNU GPL
// For more information, see License.txt in the main folder

#include <config.h>

#include <string.h>
#include <stdlib.h>

#include "int_storage.h"
#include "int_pet.h"
#include "int_guild.h"
#include <mmo.h>
#include "char.h"
#include "../common/socket.h"
#include "../common/lock.h"
#include "../common/malloc.h"

// ファイル名のデフォルト
#ifdef TXT_ONLY
char storage_txt[1024] = "save/storage.txt";
#endif /* TXT_ONLY */
static struct storagedb {
	int account_id;
	char * s_line;
} *storagedb = NULL;
static int storage_num, storage_max;

#ifdef TXT_ONLY
char guild_storage_txt[1024] = "save/g_storage.txt";
#endif /* TXT_ONLY */
static struct guild_storagedb {
	int guild_id;
	char * s_line;
} *guild_storagedb = NULL;
static int guild_storage_num, guild_storage_max;

char line[65536];

// 倉庫データを文字列に変換
inline void storage_tostr(char *str, struct storage *p) {
	char *str_p = str;
	struct item *item_p, *item_end;

	str_p += int2string(str_p, p->account_id);
	*(str_p++) = ',';
	str_p += int2string(str_p, p->storage_amount);
	*(str_p++) = '\t';

	item_end = &p->storage[MAX_STORAGE];
	for(item_p = &p->storage[0]; item_p < item_end; item_p++) {
		if (item_p->nameid != 0) {
			str_p += int2string(str_p, item_p->id);
			*(str_p++) = ',';
			str_p += int2string(str_p, item_p->nameid);
			*(str_p++) = ',';
			str_p += int2string(str_p, item_p->amount);
			*(str_p++) = ',';
			str_p += int2string(str_p, item_p->equip);
			*(str_p++) = ',';
			str_p += int2string(str_p, item_p->identify);
			*(str_p++) = ',';
			str_p += int2string(str_p, item_p->refine);
			*(str_p++) = ',';
			str_p += int2string(str_p, item_p->attribute);
			*(str_p++) = ',';
			str_p += int2string(str_p, item_p->card[0]);
			*(str_p++) = ',';
			str_p += int2string(str_p, item_p->card[1]);
			*(str_p++) = ',';
			str_p += int2string(str_p, item_p->card[2]);
			*(str_p++) = ',';
			str_p += int2string(str_p, item_p->card[3]);
			*(str_p++) = ' ';
		}
	}

	*str_p = '\0';

	return;
}

// 文字列を倉庫データに変換
inline void storage_fromstr(const char *str, struct storage *p) {
	int tmp_int[11];
	int next, len, i;
	struct item * sitem;

	p->storage_amount = 0;

	if (sscanf(str, "%d,%d%n", &tmp_int[0], &tmp_int[1], &next) != 2)
		return;
	p->account_id = tmp_int[0];

	if (str[next] == '\n' || str[next] == '\r')
		return;

	next++;
	i = 0;
	while(str[next] && str[next] != '\t' && i < MAX_STORAGE) { // '\t' for compatibility with old versions
		if (sscanf(str + next, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d%n",
		    &tmp_int[0], &tmp_int[1], &tmp_int[2], &tmp_int[3],
		    &tmp_int[4], &tmp_int[5], &tmp_int[6],
		    &tmp_int[7], &tmp_int[8], &tmp_int[9], &tmp_int[10], &len) == 11) {
			if (tmp_int[1] > 0) {
				sitem = &p->storage[i];
				sitem->id = tmp_int[0];
				sitem->nameid = tmp_int[1];
				sitem->amount = tmp_int[2];
				sitem->equip = tmp_int[3];
				sitem->identify = tmp_int[4];
				sitem->refine = tmp_int[5];
				sitem->attribute = tmp_int[6];
				sitem->card[0] = tmp_int[7];
				sitem->card[1] = tmp_int[8];
				sitem->card[2] = tmp_int[9];
				sitem->card[3] = tmp_int[10];
				if (sitem->refine > 10)
					sitem->refine = 10;
				else if (sitem->refine < 0)
					sitem->refine = 0;
				i++;
			}
			next += len;
			if (str[next] == ' ')
				next++;
		} else
			return;
	}

	p->storage_amount = i;

	return;
}

inline void guild_storage_tostr(char *str, struct guild_storage *p) {
	char *str_p = str;
	struct item *item_p, *item_end;

	str_p += int2string(str_p, p->guild_id);
	*(str_p++) = ',';
	str_p += int2string(str_p, p->storage_amount);
	*(str_p++) = '\t';

	item_end = &p->storage[MAX_GUILD_STORAGE];
	for(item_p = &p->storage[0]; item_p < item_end; item_p++) {
		if (item_p->nameid != 0) {
			str_p += int2string(str_p, item_p->id);
			*(str_p++) = ',';
			str_p += int2string(str_p, item_p->nameid);
			*(str_p++) = ',';
			str_p += int2string(str_p, item_p->amount);
			*(str_p++) = ',';
			str_p += int2string(str_p, item_p->equip);
			*(str_p++) = ',';
			str_p += int2string(str_p, item_p->identify);
			*(str_p++) = ',';
			str_p += int2string(str_p, item_p->refine);
			*(str_p++) = ',';
			str_p += int2string(str_p, item_p->attribute);
			*(str_p++) = ',';
			str_p += int2string(str_p, item_p->card[0]);
			*(str_p++) = ',';
			str_p += int2string(str_p, item_p->card[1]);
			*(str_p++) = ',';
			str_p += int2string(str_p, item_p->card[2]);
			*(str_p++) = ',';
			str_p += int2string(str_p, item_p->card[3]);
			*(str_p++) = ' ';
		}
	}

	*str_p = '\0';

	return;
}

inline void guild_storage_fromstr(const char *str, struct guild_storage *p) {
	int tmp_int[11];
	int next, len, i;
	struct item * gsitem;

	p->storage_amount = 0;

	if (sscanf(str, "%d,%d%n", &tmp_int[0], &tmp_int[1], &next) != 2)
		return;
	p->guild_id = tmp_int[0];

	if (str[next] == '\n' || str[next] == '\r')
		return;

	next++;
	i = 0;
	while(str[next] && str[next] != '\t' && i < MAX_GUILD_STORAGE) { // '\t' for compatibility with old versions
		if (sscanf(str + next, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d%n",
		    &tmp_int[0], &tmp_int[1], &tmp_int[2], &tmp_int[3],
		    &tmp_int[4], &tmp_int[5], &tmp_int[6],
		    &tmp_int[7], &tmp_int[8], &tmp_int[9], &tmp_int[10], &len) == 11) {
			if (tmp_int[1] > 0) {
				gsitem = &p->storage[i];
				gsitem->id = tmp_int[0];
				gsitem->nameid = tmp_int[1];
				gsitem->amount = tmp_int[2];
				gsitem->equip = tmp_int[3];
				gsitem->identify = tmp_int[4];
				gsitem->refine = tmp_int[5];
				gsitem->attribute = tmp_int[6];
				gsitem->card[0] = tmp_int[7];
				gsitem->card[1] = tmp_int[8];
				gsitem->card[2] = tmp_int[9];
				gsitem->card[3] = tmp_int[10];
				if (gsitem->refine > 10)
					gsitem->refine = 10;
				else if (gsitem->refine < 0)
					gsitem->refine = 0;
				i++;
			}
			next += len;
			if (str[next] == ' ')
				next++;
		} else
			return;
	}

	p->storage_amount = i;

	return;
}

#ifdef USE_SQL
// here, we remove (guild) storage from memory that have no character online
int storage_sync_timer(int tid, unsigned int tick, int id, int data) {
	int i, j;
	int ident; // speed up

	// searching if a character is in memory. if not, remove storage information from memory
	for(i = 0; i < storage_num; i++) {
		ident = storagedb[i].account_id; // speed up
		for(j = 0; j < char_num; j++)
			if (char_dat[j].account_id == ident) // if a player is in memory, storage must stay in memory
				break;
		if (j == char_num) {
			FREE(storagedb[i].s_line);
			if (i != (storage_num - 1))
				memcpy(&storagedb[i], &storagedb[storage_num - 1], sizeof(struct storagedb));
			storage_num--;
			i--; // to continue with actual 'new' storage
		}
	}

	// searching if a character is in memory. if not, remove guild storage information from memory
	for(i = 0; i < guild_storage_num; i++) {
		ident = guild_storagedb[i].guild_id; // speed up
		for(j = 0; j < char_num; j++)
			if (char_dat[j].guild_id == ident) // if a player is in memory, guild storage must stay in memory
				break;
		if (j == char_num) {
			FREE(guild_storagedb[i].s_line);
			if (i != (guild_storage_num - 1))
				memcpy(&guild_storagedb[i], &guild_storagedb[guild_storage_num - 1], sizeof(struct guild_storagedb));
			guild_storage_num--;
			i--; // to continue with actual 'new' guild storage
		}
	}

	return 0;
}
#endif /* USE_SQL */

//---------------------------------------------------------
// 倉庫データを読み込む
void inter_storage_init() {
	int c, i;
#ifdef TXT_ONLY
	int id, amount, next;
	char *s_line;
	FILE *fp;
#endif /* TXT_ONLY */

	// init db
	storage_num = 0;
	storage_max = 256;
	CALLOC(storagedb, struct storagedb, 256);

#ifdef TXT_ONLY
	c = 0;
	if ((fp = fopen(storage_txt, "r")) == NULL) {
		printf("Can't read storage file: %s.\n", storage_txt);
	} else {
		while(fgets(line, sizeof(line), fp)) { // fgets reads until maximum one less than size and add '\0' -> so, it's not necessary to add -1
			c++;
			if ((line[0] == '/' && line[1] == '/') || line[0] == '\0' || line[0] == '\n' || line[0] == '\r')
				continue;
			/* remove carriage return if exist */
			while(line[0] != '\0' && (line[(i = strlen(line) - 1)] == '\n' || line[i] == '\r'))
				line[i] = '\0';
			if (sscanf(line, "%d,%d\t%n", &id, &amount, &next) == 2 && id > 0 && line[next] != '\0') {
				CALLOC(s_line, char, strlen(line) + 1); // + NULL
				strcpy(s_line, line);
				/* increase size of db if necessary */
				if (storage_num >= storage_max) {
					storage_max += 256;
					REALLOC(storagedb, struct storagedb, storage_max);
					// not need to init new datas
				}
				storagedb[storage_num].account_id = id;
				storagedb[storage_num].s_line = s_line;
				storage_num++;
			} else {
				printf("int_storage: broken data [storage file: %s] line %d.\n", storage_txt, c);
			}
		}
		fclose(fp);
	}
	if (storage_num == 0)
		printf("No storage found in storage file [%s].\n", storage_txt);
	else if (storage_num == 1)
		printf("1 storage found in storage file [%s].\n", storage_txt);
	else
		printf("%d storages found in storage file [%s].\n", storage_num, storage_txt);
#endif /* TXT_ONLY */

#ifdef USE_SQL
	c = 0;
	if (sql_request("SELECT COUNT(distinct `account_id`) FROM `%s` WHERE 1", storage_db)) {
		if (sql_get_row())
			c = sql_get_integer(0);
	} else
		printf("Impossible to do a request on '%s'\n", storage_db);
	if (c == 0)
		printf("No storage found in storage DB [%s].\n", storage_db);
	else if (c == 1)
		printf("1 storage found in storage DB [%s].\n", storage_db);
	else
		printf("%d storages found in storage DB [%s].\n", c, storage_db);
#endif /* USE_SQL */

	// init db
	guild_storage_num = 0;
	guild_storage_max = 256;
	CALLOC(guild_storagedb, struct guild_storagedb, 256);

#ifdef TXT_ONLY
	c = 0;
	if ((fp = fopen(guild_storage_txt, "r")) == NULL) {
		printf("Can't read guild storage file: %s.\n", guild_storage_txt);
	} else {
		while(fgets(line, sizeof(line), fp)) { // fgets reads until maximum one less than size and add '\0' -> so, it's not necessary to add -1
			c++;
			if ((line[0] == '/' && line[1] == '/') || line[0] == '\0' || line[0] == '\n' || line[0] == '\r')
				continue;
			/* remove carriage return if exist */
			while(line[0] != '\0' && (line[(i = strlen(line) - 1)] == '\n' || line[i] == '\r'))
				line[i] = '\0';
			if (sscanf(line, "%d,%d\t%n", &id, &amount, &next) == 2 && id > 0 && line[next] != '\0') {
				CALLOC(s_line, char, strlen(line) + 1); // + NULL
				strcpy(s_line, line);
				/* increase size of db if necessary */
				if (guild_storage_num >= guild_storage_max) {
					guild_storage_max += 256;
					REALLOC(guild_storagedb, struct guild_storagedb, guild_storage_max);
					// not need to init new datas
				}
				guild_storagedb[guild_storage_num].guild_id = id;
				guild_storagedb[guild_storage_num].s_line = s_line;
				guild_storage_num++;
			} else {
				printf("int_storage: broken data [guild storage file: %s] line %d.\n", guild_storage_txt, c);
			}
		}
		fclose(fp);
	}
	if (guild_storage_num == 0)
		printf("No guild storage found in guild storage DB [%s].\n", guild_storage_txt);
	else if (guild_storage_num == 1)
		printf("1 guild storage found in guild storage DB [%s].\n", guild_storage_txt);
	else
		printf("%d guild storages found in guild storage DB [%s].\n", guild_storage_num, guild_storage_txt);
#endif /* TXT_ONLY */

#ifdef USE_SQL
	c = 0;
	if (sql_request("SELECT COUNT(distinct `guild_id`) FROM `%s` WHERE 1", guild_storage_db)) {
		if (sql_get_row())
			c = sql_get_integer(0);
	} else
		printf("Impossible to do a request on '%s'\n", storage_db);
	if (c == 0)
		printf("No storage found in guild storage DB [%s].\n", guild_storage_db);
	else if (c == 1)
		printf("1 storage found in guild storage DB [%s].\n", guild_storage_db);
	else
		printf("%d storages found in guild storage DB [%s].\n", c, guild_storage_db);
#endif /* USE_SQL */

#ifdef USE_SQL
	add_timer_func_list(storage_sync_timer, "storage_sync_timer");
	i = add_timer_interval(gettick_cache + 60 * 1000, storage_sync_timer, 0, 0, 60 * 1000); // to check storage in memory and free if not used
#endif /* USE_SQL */

	return;
}

#ifdef TXT_ONLY
/*--------------------------
  quick sorting
--------------------------*/
void storage_speed_sorting(int tableau[], const int premier, const int dernier) {
	int temp, vmin, vmax, separateur_de_listes;

	vmin = premier;
	vmax = dernier;
	separateur_de_listes = storagedb[tableau[(premier + dernier) / 2]].account_id;

	do {
		while(storagedb[tableau[vmin]].account_id < separateur_de_listes)
			vmin++;
		while(storagedb[tableau[vmax]].account_id > separateur_de_listes)
			vmax--;

		if (vmin <= vmax) {
			temp = tableau[vmin];
			tableau[vmin++] = tableau[vmax];
			tableau[vmax--] = temp;
		}
	} while(vmin <= vmax);

	if (premier < vmax)
		storage_speed_sorting(tableau, premier, vmax);
	if (vmin < dernier)
		storage_speed_sorting(tableau, vmin, dernier);
}

//---------------------------------------------------------
// 倉庫データを書き込む
void inter_storage_save() {
	FILE *fp;
	int lock;
	int i, account_id, amount, next;
	char * str;
	int *id;

	if ((fp = lock_fopen(storage_txt, &lock)) == NULL) {
		printf("int_storage: can't write storage file [%s] !!! data is lost !!!\n", storage_txt);
		return;
	}

	CALLOC(id, int, storage_num);

	/* Sorting before save */
	for(i = 0; i < storage_num; i++)
		id[i] = i;
	if (storage_num > 1)
		storage_speed_sorting(id, 0, storage_num - 1);

	for (i = 0; i < storage_num; i++) {
		str = storagedb[id[i]].s_line; /* use of sorted index */
		if (sscanf(str, "%d,%d%n", &account_id, &amount, &next) == 2 && amount > 0 && str[next] == '\t' && str[next + 1] != '\0') {
			fputs(str, fp);
			fputs(RETCODE, fp);
		}
	}

	lock_fclose(fp, storage_txt, &lock);

	FREE(id);

//	printf("inter_storage_save: %s saved.\n", storage_txt);

	return;
}

/*--------------------------
  quick sorting
--------------------------*/
void guild_storage_speed_sorting(int tableau[], const int premier, const int dernier) {
	int temp, vmin, vmax, separateur_de_listes;

	vmin = premier;
	vmax = dernier;
	separateur_de_listes = guild_storagedb[tableau[(premier + dernier) / 2]].guild_id;

	do {
		while(guild_storagedb[tableau[vmin]].guild_id < separateur_de_listes)
			vmin++;
		while(guild_storagedb[tableau[vmax]].guild_id > separateur_de_listes)
			vmax--;

		if (vmin <= vmax) {
			temp = tableau[vmin];
			tableau[vmin++] = tableau[vmax];
			tableau[vmax--] = temp;
		}
	} while(vmin <= vmax);

	if (premier < vmax)
		guild_storage_speed_sorting(tableau, premier, vmax);
	if (vmin < dernier)
		guild_storage_speed_sorting(tableau, vmin, dernier);
}

//---------------------------------------------------------
// 倉庫データを書き込む
void inter_guild_storage_save() {
	FILE *fp;
	int lock;
	int i, guild_id, amount, next;
	char * str;
	int *id;

	if ((fp = lock_fopen(guild_storage_txt, &lock)) == NULL) {
		printf("int_storage: can't write guild storage file [%s] !!! data is lost !!!\n", guild_storage_txt);
		return;
	}

	CALLOC(id, int, guild_storage_num);

	/* Sorting before save */
	for(i = 0; i < guild_storage_num; i++)
		id[i] = i;
	if (guild_storage_num > 1)
		guild_storage_speed_sorting(id, 0, guild_storage_num - 1);

	for (i = 0; i < guild_storage_num; i++) {
		str = guild_storagedb[id[i]].s_line; /* use of sorted index */
		if (sscanf(str, "%d,%d%n", &guild_id, &amount, &next) == 2 && amount > 0 && str[next] == '\t' && str[next + 1] != '\0') {
			fputs(str, fp);
			fputs(RETCODE, fp);
		}
	}

	lock_fclose(fp, guild_storage_txt, &lock);

	FREE(id);

//	printf("inter_guild_storage_save: %s saved.\n", guild_storage_txt);

	return;
}
#endif /* TXT_ONLY */

void load_storage(const int account_id, struct storage *s) {
	char *s_line;
	int i;
#ifdef USE_SQL
	struct item * sitem;
#endif /* USE_SQL */

	memset(s, 0, sizeof(struct storage));
	for (i = 0; i < storage_num; i++)
		if (storagedb[i].account_id == account_id) {
			storage_fromstr(storagedb[i].s_line, s);
			break;
		}

	if (i == storage_num) {
		s->account_id = account_id;
#ifdef USE_SQL
		// search storage from SQL
		i = 0;
		if (sql_request("SELECT `id`,`nameid`,`amount`,`equip`,`identify`,`refine`,`attribute`,`card0`,`card1`,`card2`,`card3` FROM `%s` WHERE `account_id`='%d'", storage_db, account_id)) {
			while(sql_get_row() && i < MAX_STORAGE) {
				if (sql_get_integer(1) > 0 && sql_get_integer(2) > 0) {
					sitem = &s->storage[i];
					sitem->id = sql_get_integer(0);
					sitem->nameid = sql_get_integer(1);
					sitem->amount = sql_get_integer(2);
					sitem->equip = sql_get_integer(3);
					sitem->identify = sql_get_integer(4);
					sitem->refine = sql_get_integer(5);
					sitem->attribute = sql_get_integer(6);
					sitem->card[0] = sql_get_integer(7);
					sitem->card[1] = sql_get_integer(8);
					sitem->card[2] = sql_get_integer(9);
					sitem->card[3] = sql_get_integer(10);
					if (sitem->refine > 10)
						sitem->refine = 10;
					else if (sitem->refine < 0)
						sitem->refine = 0;
					i++;
				}
			}
		}
		s->storage_amount = i;
#endif /* USE_SQL */
		// add storage in memory
		storage_tostr(line, s);
		CALLOC(s_line, char, strlen(line) + 1); // + NULL
		strcpy(s_line, line);
		/* increase size of db if necessary */
		if (storage_num >= storage_max) {
			storage_max += 256;
			REALLOC(storagedb, struct storagedb, storage_max);
			// not need to init new datas
		}
		storagedb[storage_num].account_id = account_id;
		storagedb[storage_num].s_line = s_line;
		storage_num++;
	}

	return;
}

void load_guild_storage(const int guild_id, struct guild_storage *gs) {
	char *s_line;
	int i;
#ifdef USE_SQL
	struct item * gsitem;
#endif /* USE_SQL */

	memset(gs, 0, sizeof(struct guild_storage));
	for (i = 0; i < guild_storage_num; i++)
		if (guild_storagedb[i].guild_id == guild_id) {
			guild_storage_fromstr(guild_storagedb[i].s_line, gs);
			break;
		}

	// if storage not in memory, try to found it in SQL
	if (i == guild_storage_num) {
		gs->guild_id = guild_id;
#ifdef USE_SQL
		// search storage from SQL
		i = 0;
		if (sql_request("SELECT `id`,`nameid`,`amount`,`equip`,`identify`,`refine`,`attribute`,`card0`,`card1`,`card2`,`card3` FROM `%s` WHERE `guild_id`='%d'", guild_storage_db, guild_id)) {
			while(sql_get_row() && i < MAX_GUILD_STORAGE) {
				if (sql_get_integer(1) > 0 && sql_get_integer(2) > 0) {
					gsitem = &gs->storage[i];
					gsitem->id = sql_get_integer(0);
					gsitem->nameid = sql_get_integer(1);
					gsitem->amount = sql_get_integer(2);
					gsitem->equip = sql_get_integer(3);
					gsitem->identify = sql_get_integer(4);
					gsitem->refine = sql_get_integer(5);
					gsitem->attribute = sql_get_integer(6);
					gsitem->card[0] = sql_get_integer(7);
					gsitem->card[1] = sql_get_integer(8);
					gsitem->card[2] = sql_get_integer(9);
					gsitem->card[3] = sql_get_integer(10);
					if (gsitem->refine > 10)
						gsitem->refine = 10;
					else if (gsitem->refine < 0)
						gsitem->refine = 0;
					i++;
				}
			}
		}
		gs->storage_amount = i;
#endif /* USE_SQL */
		// add guild storage in memory
		guild_storage_tostr(line, gs);
		CALLOC(s_line, char, strlen(line) + 1); // + NULL
		strcpy(s_line, line);
		/* increase size of db if necessary */
		if (guild_storage_num >= guild_storage_max) {
			guild_storage_max += 256;
			REALLOC(guild_storagedb, struct guild_storagedb, guild_storage_max);
			// not need to init new datas
		}
		guild_storagedb[guild_storage_num].guild_id = guild_id;
		guild_storagedb[guild_storage_num].s_line = s_line;
		guild_storage_num++;
	}

	return;
}

// 倉庫データ削除
void inter_storage_delete(const int account_id) {
	int i;
	struct storage *s;

	// remove pet in pet db
	CALLOC(s, struct storage, 1);
	load_storage(account_id, s);
	for(i = 0; i < s->storage_amount; i++) {
		if (s->storage[i].card[0] == (short)0xff00)
			inter_pet_delete(*((long *)(&s->storage[i].card[2])));
	}
	FREE(s);

#ifdef USE_SQL
	// remove storage from SQL
	sql_request("DELETE FROM `%s` WHERE `account_id`='%d'", storage_db, account_id);
#endif /* USE_SQL */
	// remove storage from Memory
	for (i = 0; i < storage_num; i++) {
		if (storagedb[i].account_id == account_id) {
			FREE(storagedb[i].s_line);
			if (i < storage_num - 1)
				memcpy(&storagedb[i], &storagedb[storage_num - 1], sizeof(struct storagedb));
			storage_num--;
			break;
		}
	}

	return;
}

// ギルド倉庫データ削除
int inter_guild_storage_delete(const int guild_id) {
	int i;
	struct guild_storage *gs;

	// remove pet in pet db
	CALLOC(gs, struct guild_storage, 1);
	load_guild_storage(guild_id, gs);
	for(i = 0; i < gs->storage_amount; i++) {
		if (gs->storage[i].card[0] == (short)0xff00)
			inter_pet_delete(*((long *)(&gs->storage[i].card[2])));
	}
	FREE(gs);

#ifdef USE_SQL
	// remove storage from SQL
	sql_request("DELETE FROM `%s` WHERE `guild_id`='%d'", guild_storage_db, guild_id);
#endif /* USE_SQL */
	// remove storage from Memory
	for (i = 0; i < guild_storage_num; i++) {
		if (guild_storagedb[i].guild_id == guild_id) {
			FREE(guild_storagedb[i].s_line);
			if (i < guild_storage_num - 1)
				memcpy(&guild_storagedb[i], &guild_storagedb[guild_storage_num - 1], sizeof(struct guild_storagedb));
			guild_storage_num--;
			break;
		}
	}

	return 0;
}

//---------------------------------------------------------
// map serverへの通信

// 倉庫データ保存完了送信
void mapif_save_storage_ack(const int fd, const int account_id) { // need to remove storage's flag of saving
	WPACKETW(0) = 0x3811;
	WPACKETL(2) = account_id;
	SENDPACKET(fd, 6);

	return;
}

void mapif_get_storage(const int fd, const int account_id) { // same of mapif_parse_LoadStorage, but storage is not display at reception
	struct storage *s;

	CALLOC(s, struct storage, 1);
	load_storage(account_id, s);

	WPACKETW(0) = 0x3812;
	WPACKETW(2) = sizeof(struct storage) + 8;
	WPACKETL(4) = account_id;
	memcpy(WPACKETP(8), s, sizeof(struct storage));
	SENDPACKET(fd, sizeof(struct storage) + 8);

	FREE(s);

	return;
}

/*void mapif_save_guild_storage_ack(const int fd, const int account_id, const int guild_id, const int fail) { // no real usage for this packet
	WPACKETW( 0) = 0x3819;
	WPACKETL( 2) = account_id;
	WPACKETL( 6) = guild_id;
	WPACKETB(10) = fail;
	SENDPACKET(fd, 11);

	return;
}*/

//---------------------------------------------------------
// map serverからの通信

// 倉庫データ要求受信
void mapif_parse_LoadStorage(const int fd, const int account_id) {
	struct storage *s;

	CALLOC(s, struct storage, 1);
	load_storage(account_id, s);

	WPACKETW(0) = 0x3810;
	WPACKETW(2) = sizeof(struct storage) + 8;
	WPACKETL(4) = account_id;
	memcpy(WPACKETP(8), s, sizeof(struct storage));
	SENDPACKET(fd, sizeof(struct storage) + 8);

	FREE(s);

	return;
}

// 倉庫データ受信＆保存
void mapif_parse_SaveStorage(const int fd) {
	int account_id;
	int len = RFIFOW(fd,2);
	int line_len, i;
	int save_flag;

	if (sizeof(struct storage) != len - 8) {
		printf("inter storage: data size error %d %d\n", (int)sizeof(struct storage), len - 8);
	} else {
		account_id = RFIFOL(fd,4);
		storage_tostr(line, (struct storage *)RFIFOP(fd,8));
		line_len = strlen(line);
		//printf("mapif_parse_SaveStorage: account %d.\n", account_id);
		for (i = 0; i < storage_num; i++)
			if (storagedb[i].account_id == account_id) {
				// save only if different
				if (strcmp(storagedb[i].s_line, line) != 0) {
					REALLOC(storagedb[i].s_line, char, line_len + 1); // +1 for the NULL
					memcpy(storagedb[i].s_line, line, line_len + 1); // memcpy speeder than strcpy
				}
				break;
			}
		/* if storage was not found in the memory */
		if (i == storage_num) {
			/* increase size of db if necessary */
			if (storage_num >= storage_max) {
				storage_max += 256;
				REALLOC(storagedb, struct storagedb, storage_max);
				// not need to init new datas
			}
			storagedb[storage_num].account_id = account_id;
			MALLOC(storagedb[storage_num].s_line, char, line_len + 1); // + NULL
			memcpy(storagedb[storage_num].s_line, line, line_len + 1); // memcpy speeder than strcpy
			storage_num++;
		}
#ifdef TXT_ONLY
		save_flag = 1;
#endif /* TXT_ONLY */
#ifdef USE_SQL
		save_flag = memitemdata_to_sql(((struct storage *)RFIFOP(fd,8))->storage, account_id, TABLE_STORAGE);
		//printf("Storage save to DB - account_id: %d.\n", account_id);
#endif /* USE_SQL */
		if (save_flag)
			mapif_save_storage_ack(fd, account_id); // need to remove storage's flag of saving
	}

	return;
}

void mapif_parse_LoadGuildStorage(const int fd, const int account_id, const int guild_id) {
	struct guild_storage *gs;
	int i;

	if (guild_id > 0)
		// check if a player with guild_id and account_id is online
		for(i = 0; i < char_num; i++)
			if (char_dat[i].guild_id == guild_id && char_dat[i].account_id == account_id) { // if a player is in memory, request can exist
				CALLOC(gs, struct guild_storage, 1);
				load_guild_storage(guild_id, gs);
				WPACKETW(0) = 0x3818;
				WPACKETW(2) = sizeof(struct guild_storage) + 12;
				WPACKETL(4) = account_id;
				WPACKETL(8) = guild_id;
				memcpy(WPACKETP(12), gs, sizeof(struct guild_storage));
				SENDPACKET(fd, sizeof(struct guild_storage) + 12);
				FREE(gs);
				return;
			}

/*	// else // send this information do nothing in map server, so, don't send it
	WPACKETW(0) = 0x3818;
	WPACKETW(2) = 12;
	WPACKETL(4) = account_id;
	WPACKETL(8) = 0;
	SENDPACKET(fd, 12);*/

	return;
}

void mapif_parse_SaveGuildStorage(int fd) {
	int guild_id;
	int len = RFIFOW(fd,2);
	int line_len, i;

	if (sizeof(struct guild_storage) != len - 12) {
		printf("inter storage: data size error %d %d\n", (int)sizeof(struct guild_storage), len - 12);
	} else {
		guild_id = RFIFOL(fd,8);
		guild_storage_tostr(line, (struct guild_storage*)RFIFOP(fd,12));
		line_len = strlen(line);
		//printf("mapif_parse_SaveGuildStorage: guild %d.\n", guild_id);
		for (i = 0; i < guild_storage_num; i++)
			if (guild_storagedb[i].guild_id == guild_id) {
				// save only if different
				if (strcmp(guild_storagedb[i].s_line, line) != 0) {
					REALLOC(guild_storagedb[i].s_line, char, line_len + 1); // +1 for the NULL
					memcpy(guild_storagedb[i].s_line, line, line_len + 1); // memcpy speeder than strcpy
				}
				break;
			}
		/* if guild storage was not found in the memory */
		if (i == guild_storage_num) {
			/* increase size of db if necessary */
			if (guild_storage_num >= guild_storage_max) {
				guild_storage_max += 256;
				REALLOC(guild_storagedb, struct guild_storagedb, guild_storage_max);
				// not need to init new datas
			}
			guild_storagedb[guild_storage_num].guild_id = guild_id;
			MALLOC(guild_storagedb[guild_storage_num].s_line, char, line_len + 1); // + NULL
			memcpy(guild_storagedb[guild_storage_num].s_line, line, line_len + 1); // memcpy speeder than strcpy
			guild_storage_num++;
		}
#ifdef USE_SQL
		memitemdata_to_sql(((struct guild_storage*)RFIFOP(fd,12))->storage, guild_id, TABLE_GUILD_STORAGE);
		//printf("Guild storage save to DB - guild_id: %d.\n", guild_id);
#endif /* USE_SQL */
//		mapif_save_guild_storage_ack(fd, RFIFOL(fd,4), guild_id, 0); // no real usage for this packet
	}

	return;
}

// map server からの通信
// ・１パケットのみ解析すること
// ・パケット長データはinter.cにセットしておくこと
// ・パケット長チェックや、RFIFOSKIPは呼び出し元で行われるので行ってはならない
// ・エラーなら0(false)、そうでないなら1(true)をかえさなければならない
int inter_storage_parse_frommap(int fd) {
	switch(RFIFOW(fd,0)) {
	case 0x3010: mapif_parse_LoadStorage(fd, RFIFOL(fd,2)); break;
	case 0x3011: mapif_parse_SaveStorage(fd); break;
	case 0x3018: mapif_parse_LoadGuildStorage(fd, RFIFOL(fd,2), RFIFOL(fd,6)); break;
	case 0x3019: mapif_parse_SaveGuildStorage(fd); break;
	default:
		return 0;
	}

	return 1;
}
