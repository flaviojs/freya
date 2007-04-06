// $Id: itemdb.c 683 2006-07-08 02:48:38Z zugule $

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> // for access function
#include <stdint.h>

#include "../common/db.h"
#include "../common/utils.h"
#include "../common/malloc.h"
#include "grfio.h"
#include "nullpo.h"
#include "map.h"
#include "battle.h"
#include "itemdb.h"
#include "script.h"
#include "pc.h"

#ifdef MEMWATCH
#include "memwatch.h"
#endif

#define MAX_RANDITEM	2000

// #define ITEMDB_OVERRIDE_NAME_VERBOSE	1

static struct dbt* item_db;

static struct random_item_data blue_box[MAX_RANDITEM], violet_box[MAX_RANDITEM], card_album[MAX_RANDITEM], gift_box[MAX_RANDITEM], scroll[MAX_RANDITEM];
static int blue_box_count=0,violet_box_count=0,card_album_count=0,gift_box_count=0,scroll_count=0;
static int blue_box_default=0,violet_box_default=0,card_album_default=0,gift_box_default=0,scroll_default=0;

// Function declarations

void itemdb_read(void);
static int itemdb_readdb(void);
#ifndef TXT_ONLY
static int itemdb_read_sqldb(void);
#endif /* NOT TXT_ONLY */
static int itemdb_read_randomitem();
static int itemdb_read_itemavail(void);
static int itemdb_read_itemnametable(void);
static int itemdb_read_noequip(void);
static void itemdb_read_notrade(void);
static void itemdb_read_norefine(void);
static int itemdb_read_itemslottable(void);
static int itemdb_read_itemslotcounttable(void);
static int itemdb_read_cardillustnametable(void);
void itemdb_reload(void);

/*==========================================
 * 名前で検索用
 *------------------------------------------
 */
// name = item alias, so we should find items aliases first. if not found then look for "jname" (full name)
int itemdb_searchname_sub(void *key,void *data,va_list ap)
{
	struct item_data *item = (struct item_data *)data, **dst;
	char *str;

	str = va_arg(ap, char *);
	dst = va_arg(ap, struct item_data **);
//	if (strcasecmp(item->name, str) == 0 || strcmp(item->jname, str) == 0 ||
//		memcmp(item->name, str, 24) == 0 || memcmp(item->jname, str, 24) == 0)
	if (strcasecmp(item->name, str) == 0) //by lupus
		*dst = item;

	return 0;
}

/*==========================================
 * 名前で検索用
 *------------------------------------------
 */
int itemdb_searchjname_sub(void *key,void *data,va_list ap)
{
	struct item_data *item = (struct item_data *)data, **dst;
	char *str;

	str = va_arg(ap,char *);
	dst = va_arg(ap,struct item_data **);
	if (strcasecmp(item->jname, str) == 0)
		*dst = item;

	return 0;
}

/*==========================================
 * 名前で検索
 *------------------------------------------
 */
struct item_data* itemdb_searchname(const char *str)
{
	struct item_data *item=NULL;
	numdb_foreach(item_db,itemdb_searchname_sub,str,&item);

	return item;
}

/*==========================================
 * 箱系アイテム検索
 *------------------------------------------
 */
int itemdb_searchrandomid(int flags)
{
	int nameid = 0, i, idx, count;
	struct random_item_data *list = NULL;

	struct {
		int nameid, count;
		struct random_item_data *list;
	} data[6];

	// for BCC32 compile error
	data[0].nameid = 0;                   data[0].count = 0;                 data[0].list = NULL;
	data[1].nameid = blue_box_default;    data[1].count = blue_box_count;    data[1].list = blue_box;
	data[2].nameid = violet_box_default;  data[2].count = violet_box_count;  data[2].list = violet_box;
	data[3].nameid = card_album_default;  data[3].count = card_album_count;  data[3].list = card_album;
	data[4].nameid = gift_box_default;    data[4].count = gift_box_count;    data[4].list = gift_box;
	data[5].nameid = scroll_default;      data[5].count = scroll_count;      data[5].list = scroll;
//	data[6].nameid = finding_ore_default; data[6].count = finding_ore_count; data[6].list = finding_ore;

	if (flags >= 1 && flags <= 5) {
		nameid = data[flags].nameid;
		count = data[flags].count;
		list = data[flags].list;

		if (count > 0) {
			for(i = 0; i < 1000; i++) {
				idx = rand() % count;
				if (rand() % 1000000 < list[idx].per) {
					nameid = list[idx].nameid;
					break;
				}
			}
		}
	}

	return nameid;
}

/*==========================================
 * DBの存在確認
 *------------------------------------------
 */
struct item_data* itemdb_exists(intptr_t nameid)
{
	return numdb_search(item_db,nameid);
}

/*==========================================
 * Converts the jobid from the format in itemdb 
 * to the format used by the map server. [Skotlex]
 *------------------------------------------
 */
static void itemdb_jobid2mapid(unsigned int *bclass, unsigned int jobmask)
{
	int i;
	bclass[0]= bclass[1]= bclass[2]= 0;
	//Base classes
	if (jobmask & 1<<JOB_NOVICE)
	{	//Both Novice/Super-Novice are counted with the same ID
		bclass[0] |= 1<<MAPID_NOVICE;
		bclass[1] |= 1<<MAPID_NOVICE;
	}
	for (i = JOB_NOVICE+1; i <= JOB_THIEF; i++)
	{
		if (jobmask & 1<<i)
			bclass[0] |= 1<<(MAPID_NOVICE+i);
	}
	//2-1 classes
	if (jobmask & 1<<JOB_KNIGHT)
		bclass[1] |= 1<<MAPID_SWORDMAN;
	if (jobmask & 1<<JOB_PRIEST)
		bclass[1] |= 1<<MAPID_ACOLYTE;
	if (jobmask & 1<<JOB_WIZARD)
		bclass[1] |= 1<<MAPID_MAGE;
	if (jobmask & 1<<JOB_BLACKSMITH)
		bclass[1] |= 1<<MAPID_MERCHANT;
	if (jobmask & 1<<JOB_HUNTER)
		bclass[1] |= 1<<MAPID_ARCHER;
	if (jobmask & 1<<JOB_ASSASSIN)
		bclass[1] |= 1<<MAPID_THIEF;
	//2-2 classes
	if (jobmask & 1<<JOB_CRUSADER)
		bclass[2] |= 1<<MAPID_SWORDMAN;
	if (jobmask & 1<<JOB_MONK)
		bclass[2] |= 1<<MAPID_ACOLYTE;
	if (jobmask & 1<<JOB_SAGE)
		bclass[2] |= 1<<MAPID_MAGE;
	if (jobmask & 1<<JOB_ALCHEMIST)
		bclass[2] |= 1<<MAPID_MERCHANT;
	if (jobmask & 1<<JOB_BARD)
		bclass[2] |= 1<<MAPID_ARCHER;
	if (jobmask & 1<<JOB_DANCER)
		bclass[2] |= 1<<MAPID_ARCHER;
	if (jobmask & 1<<JOB_ROGUE)
		bclass[2] |= 1<<MAPID_THIEF;
	//Special classes that don't fit above.
	if (jobmask & 1<<21) //Taekwon boy
		bclass[0] |= 1<<MAPID_TAEKWON;
	if (jobmask & 1<<22) //Star Gladiator
		bclass[1] |= 1<<MAPID_TAEKWON;
	if (jobmask & 1<<23) //Soul Linker
		bclass[2] |= 1<<MAPID_TAEKWON;
}

/*==========================================
 * Search in item database
 *------------------------------------------
 */
struct item_data* itemdb_search(intptr_t nameid)
{
	struct item_data *id;

	id=numdb_search(item_db,nameid);
	if(id)
		return id;

	CALLOC(id, struct item_data, 1);
	numdb_insert(item_db,nameid,id);

	id->nameid=nameid;
	id->value_buy=10;
	id->value_sell=id->value_buy/2;
	id->weight=10;
	id->sex=2;
	id->elv=0;
	id->class_base[0]=0xff;
	id->class_base[1]=0xff;
	id->class_base[2]=0xff;
	id->class_upper=5;
	id->flag.available=0;
	id->flag.value_notdc=0;
	id->flag.value_notoc=0;
	id->flag.no_equip=0;
	id->view_id=0;

	if (nameid > 500 && nameid < 600)
		id->type = 0;   //heal item
	else if (nameid > 600 && nameid < 700)
		id->type = 2;   //use item
	else if ((nameid > 700 && nameid < 1100) ||
	         (nameid > 7000 && nameid < 8000))
		id->type = 3;   //correction
	else if (nameid >= 1750 && nameid < 1771)
		id->type = 10;  //arrow
	else if (nameid > 1100 && nameid < 2000)
		id->type = 4;   //weapon
	else if ((nameid > 2100 && nameid < 3000) ||
	         (nameid > 5000 && nameid < 6000))
		id->type = 5;   //armor
	else if (nameid > 4000 && nameid < 5000)
		id->type = 6;   //card
	else if (nameid > 9000 && nameid < 10000)
		id->type = 7;   //egg
	else if (nameid > 10000)
		id->type = 8;   //petequip

	return id;
}

/*==========================================
 *
 *------------------------------------------
 */
int itemdb_isequip(int nameid)
{
	int type=itemdb_type(nameid);
	if(type==0 || type==2 || type==3 || type==6 || type==10)
		return 0;

	return 1;
}

/*==========================================
 *
 *------------------------------------------
 */
int itemdb_isequip2(struct item_data *data)
{
	if(data) {
		int type=data->type;
		if(type==0 || type==2 || type==3 || type==6 || type==10)
			return 0;
		else
			return 1;
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int itemdb_isequip3(int nameid)
{
	int type=itemdb_type(nameid);
	if(type==4 || type==5 || type == 8)
		return 1;

	return 0;
}

/*==========================================
 * 捨てられるアイテムは1、そうでないアイテムは0
 *------------------------------------------
 */
int itemdb_isdropable(int nameid)
{
	//結婚指輪は捨てられない
	switch(nameid) {
	case 2634: //結婚指輪 // Wedding_Ring_M
	case 2635: //結婚指輪 // Wedding_Ring_F
		return 0;
	}

	return 1;
}

// === READ ALL ITEM RELATED DATABASES ===
// =======================================
void itemdb_read(void)
{
#ifndef TXT_ONLY
	if(db_use_sqldbs)
		itemdb_read_sqldb();
	else
#endif /* NOT TXT_ONLY */
		itemdb_readdb();

	itemdb_read_randomitem();
	itemdb_read_itemavail();
	itemdb_read_noequip();
	itemdb_read_notrade();
	itemdb_read_norefine();

	if(battle_config.cardillust_read_grffile)
		itemdb_read_cardillustnametable();
	if(battle_config.item_equip_override_grffile)
		itemdb_read_itemslottable();
	if(battle_config.item_slots_override_grffile)
		itemdb_read_itemslotcounttable();
	if(battle_config.item_name_override_grffile)
		itemdb_read_itemnametable();
}

/*==========================================
 * アイテムデータベースの読み込み
 *------------------------------------------
 */
static int itemdb_readdb(void)
{
	FILE *fp;
	char line[1024];
	int ln, lines = 0;
	int nameid, j;
	char *str[32], *p, *np;
	struct item_data *id;
	int i = 0;
	int buy_price, sell_price;
	char *filename[] = { "db/item_db.txt", "db/item_db2.txt" };
	#define ITEM_DB_SQL_NAME "item_db_map.sql"

#ifdef USE_SQL
	FILE *item_db_fp = NULL; // need to be initialized to avoid 'false' warning
	// code to create SQL item db
	if (create_item_db_script) {
		// if file exists (doesn't exist, was renamed!... unnormal that continue to exist)
		if (access(ITEM_DB_SQL_NAME, 0) == 0) // 0: file exist or not, 0 = success
			remove(ITEM_DB_SQL_NAME); // delete the file. return value = 0 (success), return value = -1 (denied access or not found file)
		if ((item_db_fp = fopen(ITEM_DB_SQL_NAME, "a")) != NULL) {
			printf("Generating the '" CL_WHITE ITEM_DB_SQL_NAME CL_RESET "' file.\n");
			fprintf(item_db_fp, "# You can regenerate this file with an option in inter_athena.conf" RETCODE);
			fprintf(item_db_fp, RETCODE);
			fprintf(item_db_fp, "DROP TABLE IF EXISTS `%s`;" RETCODE, item_db_db);
			fprintf(item_db_fp, "CREATE TABLE `%s` (" RETCODE, item_db_db);
			fprintf(item_db_fp, "  `id` smallint(5) unsigned NOT NULL default '0'," RETCODE);
			fprintf(item_db_fp, "  `name_english` varchar(50) NOT NULL default ''," RETCODE);
			fprintf(item_db_fp, "  `name_japanese` varchar(50) NOT NULL default ''," RETCODE);
			fprintf(item_db_fp, "  `type` tinyint(2) unsigned NOT NULL default '0'," RETCODE);
			fprintf(item_db_fp, "  `price_buy` meduimint(10) unsigned default NULL," RETCODE);
			fprintf(item_db_fp, "  `price_sell` mediumint(10) unsigned default NULL," RETCODE);
			fprintf(item_db_fp, "  `weight` smallint(5) unsigned NOT NULL default '0'," RETCODE);
			fprintf(item_db_fp, "  `attack` tinyint(4) unsigned default NULL," RETCODE);
			fprintf(item_db_fp, "  `defence` tinyint(4) unsigned default NULL," RETCODE);
			fprintf(item_db_fp, "  `range` tinyint(2) unsigned default NULL," RETCODE);
			fprintf(item_db_fp, "  `slots` tinyint(2) unsigned default NULL," RETCODE); // max 5, but set to tinyint(2) to avoid possible BOOL replacement
			fprintf(item_db_fp, "  `equip_jobs` int(12) unsigned default NULL," RETCODE);
			fprintf(item_db_fp, "  `equip_upper` tinyint(8) unsigned default NULL," RETCODE);
			fprintf(item_db_fp, "  `equip_genders` tinyint(2) unsigned default NULL," RETCODE); // max 3 (1+2), but set to tinyint(2) to avoid possible BOOL replacement
			fprintf(item_db_fp, "  `equip_locations` smallint(4) unsigned default NULL," RETCODE);
			fprintf(item_db_fp, "  `weapon_level` tinyint(2) unsigned default NULL," RETCODE); // max 4, but set to tinyint(2) to avoid possible BOOL replacement
			fprintf(item_db_fp, "  `equip_level` tinyint(3) unsigned default NULL," RETCODE);
			fprintf(item_db_fp, "  `refineable` tinyint(1) unsigned default NULL," RETCODE);
			fprintf(item_db_fp, "  `view` tinyint(3) unsigned default NULL," RETCODE);
			fprintf(item_db_fp, "  `script` text," RETCODE);
			fprintf(item_db_fp, "  `equip_script` text," RETCODE);
			fprintf(item_db_fp, "  `unequip_script` text," RETCODE);
			fprintf(item_db_fp, "  PRIMARY KEY (`id`)" RETCODE);
			fprintf(item_db_fp, ") TYPE=MyISAM;" RETCODE);
			fprintf(item_db_fp, RETCODE);
		} else {
			printf(CL_RED "Can not generate the '" ITEM_DB_SQL_NAME "' file." CL_RESET" \n");
			create_item_db_script = 0; // not continue to try to create file after
		}
	}
#endif /* USE_SQL */

	for(i = 0; i < 2; i++) {

		ln = 0;
		fp = fopen(filename[i], "r");
		if (fp == NULL) {
			if (i > 0)
				continue;
			printf("can't read %s\n", filename[i]);
#ifdef USE_SQL
			if (create_item_db_script && item_db_fp != NULL) {
				fclose(item_db_fp);
			}
#endif /* USE_SQL */
			exit(1);
		}

		lines = 0;
		while(fgets(line, sizeof(line), fp)) { // fgets reads until maximum one less than size and add '\0' -> so, it's not necessary to add -1
			lines++;
			if ((line[0] == '/' && line[1] == '/') || line[0] == '\0' || line[0] == '\n' || line[0] == '\r') {
#ifdef USE_SQL
				// code to create SQL item db
				if (create_item_db_script && item_db_fp != NULL) {
					/* remove carriage return if exist */
					while(line[0] != '\0' && (line[(j = strlen(line) - 1)] == '\n' || line[j] == '\r'))
						line[j] = '\0';
					// add comments in the sql script
					if ((line[0] == '/' && line[1] == '/' && strlen(line) > 2)) {
						fprintf(item_db_fp, "#%s" RETCODE, line + 2);
					} else {
						fprintf(item_db_fp, RETCODE);
					}
				}
#endif /* USE_SQL */
				continue;
			}

			/* remove carriage return if exist */
			while(line[0] != '\0' && (line[(j = strlen(line) - 1)] == '\n' || line[j] == '\r'))
				line[j] = '\0';

			memset(str, 0, sizeof(str));
			for(j = 0, np = p = line; j < 19 && p; j++) {
				str[j] = p;
				p = strchr(p, ',');
				if (p) { *p++ = 0; np = p; }
			}
			if (str[0] == NULL)
				continue;

			nameid = atoi(str[0]);
			if (nameid <= 0 || nameid >= 20000)
				continue;
#ifdef __DEBUG
			if (battle_config.etc_log) {
				if (strlen(str[1]) > ITEM_NAME_LENGTH)
					printf(CL_YELLOW "WARNING: Invalid item name" CL_RESET" (id: %d) - Name too long (> ITEM_NAME_LENGTH char.) -> only ITEM_NAME_LENGTH first characters are used.\n", nameid);
				if (strlen(str[2]) > ITEM_NAME_LENGTH)
					printf(CL_YELLOW "WARNING: Invalid item jname" CL_RESET" (id: %d) - Name too long (> ITEM_NAME_LENGTH char.) -> only ITEM_NAME_LENGTH first characters are used.\n", nameid);
			}
#endif

			ln++;
			if (ln % 20 == 19) {
				printf("Reading item #%d (id: %d)...\r", ln, nameid);
				fflush(stdout);
			}

			id = itemdb_search(nameid);
			memset(id->name, 0, sizeof(id->name));
			strncpy(id->name, str[1], ITEM_NAME_LENGTH);
			memset(id->jname, 0, sizeof(id->jname));
			strncpy(id->jname, str[2], ITEM_NAME_LENGTH);
			id->type = atoi(str[3]);
			if (id->type == 11) {
				id->type = 2;
				id->flag.delay_consume = 1;
			}

			buy_price = atoi(str[4]);
			sell_price = atoi(str[5]);
			// If price_buy is not 0 and price_sell is not 0...
			if (buy_price > 0 && sell_price > 0) {
				id->value_buy = buy_price;
				id->value_sell = sell_price;
			// If price_buy is not 0 and price_sell is 0...
			} else if (buy_price > 0 && sell_price == 0) {
				id->value_buy = buy_price;
				id->value_sell = buy_price / 2;
			// If price_buy is 0 and price_sell is not 0...
			} else if (buy_price == 0 && sell_price > 0) {
				id->value_buy = sell_price * 2;
				id->value_sell = sell_price;
			// If price_buy is 0 and price_sell is 0...
			} else {
				id->value_buy = 0;
				id->value_sell = 0;
			}
			// check for bad prices that can possibly cause exploits
			if (((double)id->value_buy * (double)75) / 100 < ((double)id->value_sell * (double)124) / 100) {
				printf("Item %s [%d]: prices: buy %d / sell %d (" CL_YELLOW "WARNING" CL_RESET ").\n", id->name, id->nameid, id->value_buy, id->value_sell);
				printf("for merchant: buying: %d < selling:%d (" CL_YELLOW "possible exploit OC/DC" CL_RESET ").\n", id->value_buy * 75 / 100, id->value_sell * 124 / 100);
				id->value_buy = ((double)id->value_sell * (double)124) / (double)75 + 1;
				printf("->set to: buy %d, sell %d (change in database please).\n", id->value_buy, id->value_sell);
			}

			id->weight = atoi(str[6]);
			id->atk = atoi(str[7]);
			id->def = atoi(str[8]);
			id->range = atoi(str[9]);
			id->slot = atoi(str[10]);
			itemdb_jobid2mapid(id->class_base, (unsigned int)strtoul(str[11],NULL,0));
			id->class_upper = atoi(str[12]);
			id->sex = atoi(str[13]);
			if (id->equip != atoi(str[14])) {
				id->equip = atoi(str[14]);
			}
			id->wlv = atoi(str[15]);
			id->elv = atoi(str[16]);
			id->flag.no_refine = atoi(str[17]) ? 0 : 1;
			id->look = atoi(str[18]);
			id->flag.available = 1;
			id->flag.value_notdc = 0;
			id->flag.value_notoc = 0;
			id->view_id = 0;

			id->script = NULL;
			id->equip_script = NULL;
			id->unequip_script = NULL;

			if ((p = strchr(np, '{')) != NULL) {
				id->script = parse_script((unsigned char *)p, lines);
				if ((p = strchr(p + 1, '{')) != NULL)
					id->equip_script = parse_script((unsigned char *)p, lines);
				if ((p = strchr(p + 1, '{')) != NULL)
					id->unequip_script = parse_script((unsigned char *)p, lines);
			}

#ifdef USE_SQL
/*			// code to create SQL item db
			if (create_item_db_script && item_db_fp != NULL) {
				char item_name[50]; // 24 * 2 + 1 + NULL
				char item_jname[50]; // 24 * 2 + 1 + NULL
				char script1[1024], script2[1024], script3[1024], temp_script[1024], comment_script[1024];
				char *p1, *p2, *p3;
				struct item_data *actual_item;

				actual_item = id;
				// escape item names
				memset(item_name, 0, sizeof(item_name));
				db_sql_escape_string(item_name, actual_item->name, strlen(actual_item->name));
				memset(item_jname, 0, sizeof(item_jname));
				db_sql_escape_string(item_jname, actual_item->jname, strlen(actual_item->jname));
				// extract script 1, 2 and 3
				memset(script1, 0, sizeof(script1));
				memset(script2, 0, sizeof(script2));
				memset(script3, 0, sizeof(script3));
				if ((p1 = strchr(np, '{')) != NULL && (p2 = strchr(p1, '}')) != NULL && (p3 = strchr(p3, '}')) != NULL)) {
					while(*p1 == ' ' || *p1 == '{')
						p1++;
					while((*p2 == ' ' || *p2 == '}') && p2 > p1)
						p2--;
					while((*p3 == ' ' || *p3 == '}') && p3 > p2)
						p3--;
					if (p2 > p1) {
						memset(temp_script, 0, sizeof(temp_script));
						strncpy(temp_script, p1, p2 - p1 + 1);
						memset(script1, 0, sizeof(script1));
						script1[0] = '\'';
						db_sql_escape_string(script1 + 1, temp_script, strlen(temp_script));
						strcat(script1, "'");
					}
					// search second script
					if ((p1 = strchr(p2, '{')) != NULL && (p2 = strchr(p1, '}')) != NULL) {
						while(*p1 == ' ' || *p1 == '{')
							p1++;
						while((*p2 == ' ' || *p2 == '}') && p2 > p1)
							p2--;
						if (p2 > p1) {
							memset(temp_script, 0, sizeof(temp_script));
							strncpy(temp_script, p1, p2 - p1 + 1);
							memset(script2, 0, sizeof(script2));
							script2[0] = '\'';
							db_sql_escape_string(script2 + 1, temp_script, strlen(temp_script));
							strcat(script2, "'");
						}
					}
					if ((p1 = strchr(p2, '{')) != NULL && (p2 = strchr(p1, '}')) != NULL) {
						while(*p1 == ' ' || *p1 == '{')
							p1++;
						while((*p2 == ' ' || *p2 == '}') && p2 > p1)
							p2--;
						if (p2 > p1) {
							memset(temp_script, 0, sizeof(temp_script));
							strncpy(temp_script, p1, p2 - p1 + 1);
							memset(script3, 0, sizeof(script3));
							script3[0] = '\'';
							db_sql_escape_string(script3 + 1, temp_script, strlen(temp_script));
							strcat(script3, "'");
						}
					}
				}

				memset(comment_script, 0 , sizeof(comment_script));
				if ((p1 = strchr(np, '/')) != NULL && p1[1] == '/') {
					comment_script[0] = ' ';
					comment_script[1] = '#';
					strcpy(comment_script + 2, p1 + 2);
				}
				// create request
				fprintf(item_db_fp, "INSERT INTO `%s` VALUES (%d, '%s', '%s', %d,"
				                                             " %d, %d,"
				                                             " %d, %d, %d, %d,"
				                                             " %d, '%s', %d, %d, %d,"
				                                             " %d, %d, %d, %d,"
				                                             " '%s', '%s', '%s');%s" RETCODE,
				                    item_db_db, nameid, item_name, item_jname, actual_item->type,
				                    atoi(str[4]), atoi(str[5]), // id->value_buy, id->value_sell: not modified
				                    actual_item->weight, actual_item->atk, actual_item->def, actual_item->range,
				                    actual_item->slot, str[11], actual_item->class_upper, actual_item->sex, actual_item->equip,
				                    actual_item->wlv, actual_item->elv, actual_item->flag.no_refine, actual_item->look,
				                    (script1[0] == 0) ? "NULL" : script1, (script2[0] == 0) ? "NULL" : script2, (script3[0] == 0) ? "NULL" : script3, comment_script);

			}
*/
#endif /* USE_SQL */
		}
		fclose(fp);
		printf("DB '" CL_WHITE "%s" CL_RESET "' readed ('" CL_WHITE "%d" CL_RESET "' %s).\n", filename[i], ln, (ln > 1) ? "entries" : "entry");
	}

#ifdef USE_SQL
		// code to create SQL item db
		if (create_item_db_script && item_db_fp != NULL) {
			fclose(item_db_fp);
		}
#endif /* USE_SQL */

	return 0;
}

// Removed item_value_db, don't re-add!

/*==========================================
 * ランダムアイテム出現データの読み込み
 *------------------------------------------
 */
static int itemdb_read_randomitem()
{
	FILE *fp;
	char line[1024];
	int ln=0;
	int nameid,i,j;
	char *str[10],*p;

	const struct {
		char filename[64];
		struct random_item_data *pdata;
		int *pcount,*pdefault;
	} data[] = {
		{"db/item_bluebox.txt",   blue_box,   &blue_box_count,   &blue_box_default },
		{"db/item_violetbox.txt", violet_box, &violet_box_count, &violet_box_default },
		{"db/item_cardalbum.txt", card_album, &card_album_count, &card_album_default },
		{"db/item_giftbox.txt",   gift_box,   &gift_box_count,   &gift_box_default },
		{"db/item_scroll.txt",    scroll,     &scroll_count,     &scroll_default },
	};

	for(i = 0; i < sizeof(data) / sizeof(data[0]); i++) {
		struct random_item_data *pd = data[i].pdata;
		int *pc = data[i].pcount;
		int *pdefault = data[i].pdefault;
		char *fn = data[i].filename;

		*pdefault = 0;
		if ((fp = fopen(fn, "r")) == NULL) {
			printf("can't read %s\n", fn);
			continue;
		}

		while(fgets(line, sizeof(line), fp)) { // fgets reads until maximum one less than size and add '\0' -> so, it's not necessary to add -1
			if ((line[0] == '/' && line[1] == '/') || line[0] == '\0' || line[0] == '\n' || line[0] == '\r')
				continue;
			// it's not necessary to remove 'carriage return ('\n' or '\r')
			memset(str, 0, sizeof(str));
			for(j = 0,p = line; j < 3 && p; j++) {
				str[j] = p;
				p = strchr(p, ',');
				if (p) *p++ = 0;
			}

			if (str[0] == NULL)
				continue;

			nameid = atoi(str[0]);
			if (nameid < 0 || nameid >= 20000)
				continue;
			if (nameid == 0) {
				if (str[2])
					*pdefault = atoi(str[2]);
				continue;
			}

			if (str[2]) {
				pd[ *pc   ].nameid = nameid;
				pd[(*pc)++].per    = atoi(str[2]);
			}

			if (ln >= MAX_RANDITEM)
				break;
			ln++;
		}
		fclose(fp);
		printf("DB '" CL_WHITE "%s" CL_RESET "' readed ('" CL_WHITE "%d" CL_RESET "' entrie%s).\n", fn, *pc, (*pc > 1) ? "s" : "");
	}

	return 0;
}

/*==========================================
 * アイテム使用可能フラグのオーバーライド
 *------------------------------------------
 */
static int itemdb_read_itemavail(void)
{
	FILE *fp;
	char line[1024];
	int ln = 0;
	int nameid, j, k;
	char *str[10], *p;

	if ((fp = fopen("db/item_avail.txt", "r")) == NULL) {
		printf("can't read db/item_avail.txt.\n");
		return -1;
	}

	while(fgets(line, sizeof(line), fp)) { // fgets reads until maximum one less than size and add '\0' -> so, it's not necessary to add -1
		struct item_data *id;
		if ((line[0] == '/' && line[1] == '/') || line[0] == '\0' || line[0] == '\n' || line[0] == '\r')
			continue;
		// it's not necessary to remove 'carriage return ('\n' or '\r')
		memset(str,0,sizeof(str));
		for(j = 0, p = line; j < 2 && p; j++) {
			str[j] = p;
			p = strchr(p, ',');
			if (p) *p++ = 0;
		}

		if (str[0] == NULL)
			continue;

		nameid = atoi(str[0]);
		if (nameid < 0 || nameid >= 20000 || !(id = itemdb_exists(nameid)))
			continue;
		k = atoi(str[1]);
		if (k > 0) {
			id->flag.available = 1;
			id->view_id = k;
		}
		else
			id->flag.available = 0;
		ln++;
	}
	fclose(fp);
	printf("DB '" CL_WHITE "db/item_avail.txt" CL_RESET "' readed ('" CL_WHITE "%d" CL_RESET "' entrie%s).\n", ln, (ln > 1) ? "s" : "");

	return 0;
}

/*==========================================
 * アイテムの名前テーブルを読み込む
 *------------------------------------------
 */
static int itemdb_read_itemnametable(void)
{
	char *buf,*p;
	int s;

	buf = grfio_reads("data\\idnum2itemdisplaynametable.txt",&s);

	if (buf == NULL)
		return -1;

	buf[s] = 0;
	p = buf;
	while(p - buf < s) {
		int nameid;
		char buf2[64];

		if (sscanf(p, "%d#%[^#]#", &nameid, buf2) == 2) {

#ifdef ITEMDB_OVERRIDE_NAME_VERBOSE
			if (itemdb_exists(nameid) &&
			    strncmp(itemdb_search(nameid)->jname, buf2, 24) != 0) {
				printf("[override] %d %s => %s\n", nameid, itemdb_search(nameid)->jname, buf2);
			}
#endif

			memset(itemdb_search(nameid)->jname, 0, sizeof(itemdb_search(nameid)->jname));
			strncpy(itemdb_search(nameid)->jname, buf2, 24);
		}

		p = strchr(p, 10);
		if (!p)
			break;
		p++;
	}
	FREE(buf);

	printf("File '" CL_WHITE "data\\idnum2itemdisplaynametable.txt" CL_RESET "' readed.\n");

	return 0;
}

/*==========================================
 * カードイラストのリソース名前テーブルを読み込む
 *------------------------------------------
 */
static int itemdb_read_cardillustnametable(void)
{
	char *buf,*p;
	int s;

	buf = grfio_reads("data\\num2cardillustnametable.txt", &s);

	if (buf == NULL)
		return -1;

	buf[s] = 0;
	p = buf;
	while(p - buf < s) {
		int nameid;
		char buf2[64];

		if (sscanf(p, "%d#%[^#]#", &nameid, buf2) == 2) {
			strcat(buf2, ".bmp");
			memset(itemdb_search(nameid)->cardillustname, 0, sizeof(itemdb_search(nameid)->cardillustname));
			strncpy(itemdb_search(nameid)->cardillustname, buf2, 64);
//			printf("%d %s\n", nameid, itemdb_search(nameid)->cardillustname);
		}

		p = strchr(p,10);
		if(!p)
			break;
		p++;
	}
	FREE(buf);

	printf("File '" CL_WHITE "data\\num2cardillustnametable.txt" CL_RESET "' readed.\n");

	return 0;
}

//
// 初期化
//
/*==========================================
 *
 *------------------------------------------
 */
static int itemdb_read_itemslottable(void) {
	char *buf,*p;
	int s;

	buf = grfio_reads("data\\itemslottable.txt", &s);
	if (buf == NULL)
		return -1;

	buf[s] = 0;
	p = buf;
	while(p - buf < s) {
		int nameid, equip;
		struct item_data* item;
		sscanf(p, "%d#%d#", &nameid, &equip);
		// fix unuseable cards
		item = itemdb_search(nameid);
		if (item && itemdb_isequip2(item))
			item->equip = equip;
		p = strchr(p, 10);
		if(!p)
			break;
		p++;
		p = strchr(p, 10);
		if (!p)
			break;
		p++;
	}
	FREE(buf);

	printf("File '" CL_WHITE "data\\itemslottable.txt" CL_RESET "' readed.\n");

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
static int itemdb_read_itemslotcounttable(void) {
	char *buf,*p;
	int s;

	buf = grfio_reads("data\\itemslotcounttable.txt", &s);
	if (buf == NULL)
		return -1;

	buf[s] = 0;
	p = buf;
	while(p - buf < s) {
		int nameid, slot;
		sscanf(p, "%d#%d#", &nameid, &slot);
		itemdb_search(nameid)->slot = slot;
		p = strchr(p, 10);
		if (!p)
			break;
		p++;
		p = strchr(p, 10);
		if (!p)
			break;
		p++;
	}
	FREE(buf);

	printf("File '" CL_WHITE "data\\itemslotcounttable.txt" CL_RESET "' readed.\n");

	return 0;
}

// === READ ITEM NOEQUIP DATABASE ===
// ==================================
static int itemdb_read_noequip(void)
{
	FILE *fp;
	char line[1024];
	int ln = 0;
	int nameid, j;
	char *str[32],*p;
	struct item_data *id;

	if ((fp = fopen("db/item_noequip.txt", "r")) == NULL) {
		printf("can't read db/item_noequip.txt\n");
		return -1;
	}

	while(fgets(line, sizeof(line), fp)) { // fgets reads until maximum one less than size and add '\0' -> so, it's not necessary to add -1
		if ((line[0] == '/' && line[1] == '/') || line[0] == '\0' || line[0] == '\n' || line[0] == '\r')
			continue;
		// it's not necessary to remove 'carriage return ('\n' or '\r')
		memset(str, 0, sizeof(str));
		for(j = 0, p = line; j < 2 && p; j++) {
			str[j] = p;
			p = strchr(p, ',');
			if (p)
				*p++ = 0;
		}
		if (str[0] == NULL)
			continue;

		nameid = atoi(str[0]);
		if (nameid <= 0 || nameid >= 20000 || !(id = itemdb_exists(nameid)))
			continue;

		id->flag.no_equip = atoi(str[1]); // mode = 1- not in PvP, 2- GvG restriction, 3- PvP and GvG which restriction

		ln++;
	}
	fclose(fp);

	printf("DB '" CL_WHITE "db/item_noequip.txt" CL_RESET "' readed ('" CL_WHITE "%d" CL_RESET "' entrie%s).\n", ln, (ln > 1) ? "s" : "");

	return 0;
}

// === READ ITEM NOTRADE DATABASE ===
// ==================================
static void itemdb_read_notrade(void)
{
	struct item_data *id;
	char line[8];
	int itemid, itemtype;

	FILE *db = fopen("db/item_notrade.txt", "r");

	if(db == NULL)
	{
		printf(CL_WHITE "warning: " CL_RESET "failed to read item notrade database \n");
		return;
	}

	while(fgets(line, 8, db))
	{
		if(line[0] == '/' && line[1] == '/')
			continue;
		if(line[0] == '\0' || line[0] == '\n' || line[0] == '\r')
			continue;

		sscanf(line, "%d %d", &itemid, &itemtype);

		if(!itemid || !itemtype)
			continue;

		if(itemid <= 0 || itemid >= 20000 || !(id = itemdb_exists(itemid)))
			continue;

		if(itemtype < 0)
			itemtype = 0;
		if(itemtype > 2)
			itemtype = 2;

		id->flag.no_trade = itemtype;

//		printf(CL_WHITE "debug: " CL_RESET "item id '%d' notrade flag is %d \n", id->nameid, id->flag.no_trade);
	}

	fclose(db);
	printf(CL_WHITE "status: " CL_RESET "succesfully loaded item notrade database \n");
	return;
}

// === READ ITEM NOREFINE DATABASE ===
// ===================================
static void itemdb_read_norefine(void)
{
	struct item_data *id;
	char line[8];
	int itemid;

	FILE *db = fopen("db/item_norefine.txt", "r");

	if(db == NULL)
	{
		printf(CL_WHITE "warning: " CL_RESET "failed to read item norefine database \n");
		return;
	}

	while(fgets(line, 8, db))
	{
		if(line[0] == '/' && line[1] == '/')
			continue;
		if(line[0] == '\0' || line[0] == '\n' || line[0] == '\r')
			continue;

		sscanf(line, "%d", &itemid);

		if(!itemid)
			continue;

		if(itemid <= 0 || itemid >= 20000 || !(id = itemdb_exists(itemid)))
			continue;

		id->flag.no_refine = 1;

//		printf(CL_WHITE "debug: " CL_RESET "item id '%d' norefine flag is %d \n", id->nameid, id->flag.no_refine);
	}

	fclose(db);
	printf(CL_WHITE "status: " CL_RESET "succesfully loaded item norefine database \n");
	return;
}

#ifndef TXT_ONLY
/*==================================
* SQL
*===================================
*/
static int itemdb_read_sqldb(void) {
	unsigned short nameid;
	unsigned long ln = 0;
	struct item_data *id;
	int buy_price, sell_price;
	char script[65535 + 2 + 1]; // Maximum length of MySQL TEXT type (65535) + 2 bytes for curly brackets + 1 byte for terminator

	// ----------

	sql_request("SELECT * FROM `%s`", item_db_db);

	while (sql_get_row()) {
		/* +----+--------------+---------------+------+-----------+------------+--------+--------+---------+-------+-------+------------+-------------+---------------+-----------------+--------------+-------------+------------+------+--------+--------------+----------------+
		   |  0 |            1 |             2 |    3 |         4 |          5 |      6 |      7 |       8 |     9 |    10 |         11 |          12 |            13 |              14 |           15 |          16 |         17 |   18 |     19 |           20 |             21 |
		   +----+--------------+---------------+------+-----------+------------+--------+--------+---------+-------+-------+------------+-------------+---------------+-----------------+--------------+-------------+------------+------+--------+--------------+----------------+
		   | id | name_english | name_japanese | type | price_buy | price_sell | weight | attack | defence | range | slots | equip_jobs | equip_upper | equip_genders | equip_locations | weapon_level | equip_level | refineable | view | script | equip_script | unequip_script |
		   +----+--------------+---------------+------+-----------+------------+--------+--------+---------+-------+-------+------------+-------------+---------------+-----------------+--------------+-------------+------------+------+--------+--------------+----------------+ */

		nameid = sql_get_integer(0);

		// If the identifier is not within the valid range, process the next row
		if (nameid == 0 || nameid >= 20000)
			continue;
#ifdef __DEBUG
		if (battle_config.etc_log) {
			if (strlen(sql_get_string(1)) > 24)
				printf(CL_YELLOW "WARNING: Invalid item name" CL_RESET" (id: %d) - Name too long (> 24 char.) -> only 24 first characters are used.\n", nameid);
			if (strlen(sql_get_string(2)) > 24)
				printf(CL_YELLOW "WARNING: Invalid item jname" CL_RESET" (id: %d) - Name too long (> 24 char.) -> only 24 first characters are used.\n", nameid);
		}
#endif

		ln++;
		if (ln % 22 == 21) {
			printf("Reading item #%ld (id: %d)...\r", ln, nameid);
			fflush(stdout);
		}

		// Insert a new row into the item database

		// ----------
		id = itemdb_search(nameid);

		memset(id->name, 0, sizeof(id->name));
		strncpy(id->name, sql_get_string(1), ITEM_NAME_LENGTH);
		memset(id->jname, 0, sizeof(id->jname));
		strncpy(id->jname, sql_get_string(2), ITEM_NAME_LENGTH);

		id->type = sql_get_integer(3);
		if (id->type == 11) {
			id->type = 2;
			id->flag.delay_consume=1;
		}

		// fix NULL for buy/sell prices
		if (sql_get_string(4) == NULL)
			buy_price = 0;
		else
			buy_price = sql_get_integer(4);
		if (sql_get_string(5) == NULL)
			sell_price = 0;
		else
			sell_price = sql_get_integer(5);
		// If price_buy is not 0 and price_sell is not 0...
		if (buy_price > 0 && sell_price > 0) {
			id->value_buy = buy_price;
			id->value_sell = sell_price;
		// If price_buy is not 0 and price_sell is 0...
		} else if (buy_price > 0 && sell_price == 0) {
			id->value_buy = buy_price;
			id->value_sell = buy_price / 2;
		// If price_buy is 0 and price_sell is not 0...
		} else if (buy_price == 0 && sell_price > 0) {
			id->value_buy = sell_price * 2;
			id->value_sell = sell_price;
		// If price_buy is 0 and price_sell is 0...
		} else {
			id->value_buy = 0;
			id->value_sell = 0;
		}
		// check for bad prices that can possibly cause exploits
		if (((double)id->value_buy * (double)75) / 100 < ((double)id->value_sell * (double)124) / 100) {
			printf("Item %s [%d]: prices: buy %d / sell %d (" CL_YELLOW "WARNING" CL_RESET ").\n", id->name, id->nameid, id->value_buy, id->value_sell);
			printf("for merchant: buying: %d < selling:%d (" CL_YELLOW "possible exploit OC/DC" CL_RESET ").\n", id->value_buy * 75 / 100, id->value_sell * 124 / 100);
			id->value_buy = ((double)id->value_sell * (double)124) / (double)75 + 1;
			printf("->set to: buy %d, sell %d (change in database please).\n", id->value_buy, id->value_sell);
		}

		id->weight	= sql_get_integer( 6);

		id->atk   = (sql_get_string( 7) != NULL) ? sql_get_integer( 7) : 0;
		id->def   = (sql_get_string( 8) != NULL) ? sql_get_integer( 8) : 0;
		id->range = (sql_get_string( 9) != NULL) ? sql_get_integer( 9) : 0;
		id->slot  = (sql_get_string(10) != NULL) ? sql_get_integer(10) : 0;
		itemdb_jobid2mapid(id->class_base, (sql_get_string(11) != NULL) ? (unsigned int)strtoul(sql_get_string(11), NULL, 0) : 0);
		id->class_upper = (sql_get_string(12) != NULL) ? sql_get_integer(12) : 0;
		id->sex   = (sql_get_string(13) != NULL) ? sql_get_integer(13) : 0;
		id->equip = (sql_get_string(14) != NULL) ? sql_get_integer(14) : 0;
		id->wlv   = (sql_get_string(15) != NULL) ? sql_get_integer(15) : 0;
		id->elv   = (sql_get_string(16) != NULL) ? sql_get_integer(16) : 0;
		id->flag.no_refine = (sql_get_integer(17) == 0 || sql_get_integer(17) == 1)?0:1;
		id->look  = (sql_get_string(18) != NULL) ? sql_get_integer(18) : 0;

		id->view_id = 0;

		// ----------

		if (sql_get_string(19) != NULL) {
			if (sql_get_string(19)[0] == '{')
				id->script = parse_script((unsigned char *)sql_get_string(19), 0);
			else {
				sprintf(script, "{%s}", sql_get_string(19));
				id->script = parse_script((unsigned char *)script, 0);
			}
		} else {
			id->script = NULL;
		}

		if (sql_get_string(20) != NULL) {
			if (sql_get_string(20)[0] == '{')
				id->equip_script = parse_script((unsigned char *)sql_get_string(20), 0);
			else {
				sprintf(script, "{%s}", sql_get_string(20));
				id->equip_script = parse_script((unsigned char *)script, 0);
			}
		} else {
			id->equip_script = NULL;
		}

		if (sql_get_string(21) != NULL) {
			if (sql_get_string(21)[0] == '{')
				id->unequip_script = parse_script((unsigned char *)sql_get_string(21), 0);
			else {
				sprintf(script, "{%s}", sql_get_string(21));
				id->unequip_script = parse_script((unsigned char *)script, 0);
			}
		} else {
			id->equip_script = NULL;
		}

		// ----------

		id->flag.available   = 1;
		id->flag.value_notdc = 0;
		id->flag.value_notoc = 0;
	}

	printf("DB '" CL_WHITE "%s" CL_RESET "' readed ('" CL_WHITE "%ld" CL_RESET "' entrie%s).\n", item_db_db, ln, (ln > 1) ? "s" : "");

	return 0;
}
#endif /* not TXT_ONLY */

/*==========================================
 *
 *------------------------------------------
 */
static int itemdb_final(void *key,void *data, va_list ap) {
	struct item_data *id;

	nullpo_retr(0, id = data);

	FREE(id->script);
	FREE(id->equip_script);
	FREE(id->unequip_script);
	FREE(id);

	return 0;
}

void itemdb_reload(void) {
	/*

	<empty item databases>
	itemdb_read();

	*/

	do_init_itemdb();
}

/*==========================================
 *
 *------------------------------------------
 */
void do_final_itemdb(void) {
	if (item_db) {
		numdb_final(item_db, itemdb_final);
		item_db = NULL;
	}
}

/*
static FILE *dfp;
static int itemdebug(void *key, void *data, va_list ap) {
//	struct item_data *id = (struct item_data *)data;
	fprintf(dfp, "%6d", (int)key);

	return 0;
}

void itemdebugtxt() {
	df p= fopen("itemdebug.txt", "wt");
	numdb_foreach(item_db, itemdebug);
	fclose(dfp);
}
*/

/*==========================================
 *
 *------------------------------------------
 */
int do_init_itemdb(void) {
	item_db = numdb_init();

	itemdb_read();

	return 0;
}
