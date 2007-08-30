// Copyright (c) Freya Development Team - Licensed under GNU GPL
// For more information, see License.txt in the main folder

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> // For access function

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

// ** ITEMDB_OVERRIDE_NAME_VERBOSE **
//#define ITEMDB_OVERRIDE_NAME_VERBOSE	1

static struct dbt* item_db;

static struct random_item_data blue_box[MAX_RANDITEM], violet_box[MAX_RANDITEM], card_album[MAX_RANDITEM], gift_box[MAX_RANDITEM], cookie[MAX_RANDITEM], rand_quiver[MAX_RANDITEM], tame_gift[MAX_RANDITEM], jewel_box[MAX_RANDITEM], wrap_mask[MAX_RANDITEM], scroll_pack[MAX_RANDITEM], aid_kit[MAX_RANDITEM], food_bundle[MAX_RANDITEM], red_box[MAX_RANDITEM], green_box[MAX_RANDITEM];
static int blue_box_count=0,violet_box_count=0,card_album_count=0,gift_box_count=0,cookie_count=0,rand_quiver_count=0,tame_gift_count=0,jewel_box_count=0,wrap_mask_count=0,scroll_pack_count=0,aid_kit_count=0,food_bundle_count=0,red_box_count=0,green_box_count=0;
static int blue_box_default=0,violet_box_default=0,card_album_default=0,gift_box_default=0,cookie_default=0,rand_quiver_default=0,tame_gift_default=0,jewel_box_default=0,wrap_mask_default=0,scroll_pack_default=0,aid_kit_default=0,food_bundle_default=0,red_box_default=0,green_box_default=0;
static struct item_group itemgroup_db[MAX_ITEMGROUP];

// Function declarations

void itemdb_read(void);
static int itemdb_readdb(void);
#ifndef TXT_ONLY
static int itemdb_read_sqldb(void);
#endif /* Not TXT_ONLY */
static int itemdb_read_itemgroup();
static int itemdb_read_randomitem();
static int itemdb_read_itemavail(void);
static int itemdb_read_itemnametable(void);
static int itemdb_read_noequip(void);
static int itemdb_read_norefine(void);
static int itemdb_read_itemtrade(void);
static int itemdb_read_upper(void);
static int itemdb_read_ammo(void);
static int itemdb_read_itemslottable(void);
static int itemdb_read_itemslotcounttable(void);
static int itemdb_read_cardillustnametable(void);
static int itemdb_check_gender(struct item_data *);
void itemdb_reload(void);

/*==========================================
 * 名前で検索用
 *------------------------------------------
 */
// Name = Item alias, so we should find items aliases first. If not found then look for "jname" (full name)
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
	} data[13];

	// For BCC32 compile error
	data[0].nameid = 0;                    data[0].count = 0;                  data[0].list = NULL;
	data[1].nameid = blue_box_default;     data[1].count = blue_box_count;     data[1].list = blue_box;
	data[2].nameid = violet_box_default;   data[2].count = violet_box_count;   data[2].list = violet_box;
	data[3].nameid = card_album_default;   data[3].count = card_album_count;   data[3].list = card_album;
	data[4].nameid = gift_box_default;     data[4].count = gift_box_count;     data[4].list = gift_box;
	data[5].nameid = cookie_default;       data[5].count = cookie_count;       data[5].list = cookie;

//	data[6].nameid = finding_ore_default;  data[6].count = finding_ore_count;  data[6].list = finding_ore;

	data[6].nameid = rand_quiver_default;  data[6].count = rand_quiver_count;  data[6].list = rand_quiver;
	data[7].nameid = tame_gift_default;    data[7].count = tame_gift_count;    data[7].list = tame_gift;
	data[8].nameid = jewel_box_default;    data[8].count = jewel_box_count;    data[8].list = jewel_box;
	data[9].nameid = wrap_mask_default;    data[9].count = wrap_mask_count;    data[9].list = wrap_mask;
	data[10].nameid = scroll_pack_default; data[10].count = scroll_pack_count; data[10].list = scroll_pack;
	data[11].nameid = aid_kit_default;     data[11].count = aid_kit_count;     data[11].list = aid_kit;
	data[12].nameid = food_bundle_default; data[12].count = food_bundle_count; data[12].list = food_bundle;
	data[13].nameid = red_box_default;     data[13].count = red_box_count;     data[13].list = red_box;
	data[14].nameid = green_box_default;   data[14].count = green_box_count;   data[14].list = green_box;

	if (flags >= 1 && flags <= 14) {
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
 *
 *------------------------------------------
 */
int itemdb_group (int nameid)
{
	int i, j;
	for (i = 0; i < MAX_ITEMGROUP; i++) {
		for (j = 0; j < itemgroup_db[i].qty && itemgroup_db[i].id[j]; j++) {
			if (itemgroup_db[i].id[j] == nameid)
				return i;
		}
	}
	return -1;
}
int itemdb_searchrandomgroup (int groupid)
{
	if (groupid < 0 || groupid >= MAX_ITEMGROUP ||
		itemgroup_db[groupid].qty == 0 || itemgroup_db[groupid].id[0] == 0)
		return 0;
	
	return itemgroup_db[groupid].id[ rand()%itemgroup_db[groupid].qty ];
}

/*==========================================
 * DBの存在確認
 *------------------------------------------
 */
struct item_data* itemdb_exists(int nameid)
{
	return numdb_search(item_db,nameid);
}

/*==========================================
 * DBの検索
 *------------------------------------------
 */
struct item_data* itemdb_search(int nameid)
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
	id->class=0xffffffff;
	id->flag.available=0;
	id->flag.value_notdc=0;
	id->flag.value_notoc=0;
	id->flag.no_equip=0;
	id->flag.no_refine=0;
	id->flag.ammotype=0;
	id->view_id=0;

	// Last updated March 4th 2007 [Tsuyuki] -->
	if (nameid > 500 && nameid < 600)
		id->type = 0;   // Usable Healing Item
	else if ((nameid > 600 && nameid < 700) ||
						(nameid >= 12000 && nameid < 13000))
		id->type = 2;   // Usable Item
	else if ((nameid > 700 && nameid < 1100) ||
	         (nameid > 7000 && nameid < 8000) ||
	         (nameid >= 11000 && nameid < 12000))
		id->type = 3;   // Collection
	else if ((nameid >= 1750 && nameid < 1800) ||
						(nameid >= 13200 && nameid < 13300))
		id->type = 10;  // Ammunition
	else if ((nameid > 1100 && nameid < 2000) ||
						(nameid >= 13000 && nameid < 13200) ||
						(nameid > 13300))
		id->type = 4;   // Weapon
	else if ((nameid > 2100 && nameid < 3000) ||
	         (nameid > 5000 && nameid < 6000))
		id->type = 5;   // Armor
	else if (nameid > 4000 && nameid < 5000)
		id->type = 6;   // Card
	else if (nameid > 9000 && nameid < 10000)
		id->type = 7;   // Egg
	else if (nameid > 10000 && nameid < 11000)
		id->type = 8;   // Pet Equip
	// <--

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
 * Trade Restriction functions [Skotlex]
 *------------------------------------------
 */
int itemdb_isdropable(int nameid, int gmlv)
{
	struct item_data* item = itemdb_exists(nameid);
	return (item && (!(item->flag.trade_restriction&1) || gmlv >= item->gm_lv_trade_override));
}

int itemdb_cantrade(int nameid, int gmlv, int gmlv2)
{
	struct item_data* item = itemdb_exists(nameid);
	return (item && (!(item->flag.trade_restriction&2) || gmlv >= item->gm_lv_trade_override || gmlv2 >= item->gm_lv_trade_override));
}

int itemdb_canonlypartnertrade(int nameid, int gmlv, int gmlv2)
{
	struct item_data* item = itemdb_exists(nameid);
	return (item && item->flag.trade_restriction&4 && gmlv < item->gm_lv_trade_override && gmlv2 < item->gm_lv_trade_override);
}

int itemdb_cansell(int nameid, int gmlv)
{
	struct item_data* item = itemdb_exists(nameid);
	return (item && (!(item->flag.trade_restriction&8) || gmlv >= item->gm_lv_trade_override));
}

int itemdb_cancartstore(int nameid, int gmlv)
{	
	struct item_data* item = itemdb_exists(nameid);
	return (item && (!(item->flag.trade_restriction&16) || gmlv >= item->gm_lv_trade_override));
}

int itemdb_canstore(int nameid, int gmlv)
{	
	struct item_data* item = itemdb_exists(nameid);
	return (item && (!(item->flag.trade_restriction&32) || gmlv >= item->gm_lv_trade_override));
}

int itemdb_canguildstore(int nameid, int gmlv)
{	
	struct item_data* item = itemdb_exists(nameid);
	return (item && (!(item->flag.trade_restriction&64) || gmlv >= item->gm_lv_trade_override));
}

/* ------------------------------- *
 * Load All Item Related Databases *
 * ------------------------------- */
void itemdb_read(void)
{
#ifdef TXT_ONLY
	itemdb_readdb();
#else
	if(db_use_sqldbs)
		itemdb_read_sqldb();
	else
		itemdb_readdb();
#endif

	itemdb_read_itemgroup();
	itemdb_read_randomitem();
	itemdb_read_itemavail();
	itemdb_read_noequip();
	itemdb_read_norefine();
	itemdb_read_itemtrade();
	itemdb_read_upper();
	itemdb_read_ammo();

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
	FILE *item_db_fp = NULL; // Need to be initialized to avoid 'false' warning
	#define ITEM_DB_SQL_NAME "item_db_map.sql"

//#ifndef TXT_ONLY
	// Code to create SQL item database
	if (create_item_db_script) {
		// If file exists (doesn't exist, was renamed!... not normal that continue to exist)
		if (access(ITEM_DB_SQL_NAME, 0) == 0) // 0: File exists or not, 0 = success
			remove(ITEM_DB_SQL_NAME); // Delete the file. Return value = 0 (success), return value = -1 (access denied or file not found)
		if ((item_db_fp = fopen(ITEM_DB_SQL_NAME, "a")) != NULL) {
			printf(CL_GREEN "Loaded: " CL_RESET "'" CL_WHITE ITEM_DB_SQL_NAME CL_RESET "' file generated.\n");
			fprintf(item_db_fp, "# You can regenerate this file with an option in inter_freya.conf" RETCODE);
			fprintf(item_db_fp, RETCODE);
			fprintf(item_db_fp, "CREATE TABLE `%s` (" RETCODE, item_db_db);
			fprintf(item_db_fp, "  `id` smallint(5) unsigned NOT NULL default '0'," RETCODE);
			fprintf(item_db_fp, "  `name_english` varchar(32) NOT NULL default ''," RETCODE);
			fprintf(item_db_fp, "  `name_japanese` varchar(32) NOT NULL default ''," RETCODE);
			fprintf(item_db_fp, "  `type` tinyint(2) unsigned NOT NULL default '0'," RETCODE);
			fprintf(item_db_fp, "  `price_buy` int(10) unsigned default NULL," RETCODE);
			fprintf(item_db_fp, "  `price_sell` int(10) unsigned default NULL," RETCODE);
			fprintf(item_db_fp, "  `weight` int(10) unsigned NOT NULL default '0'," RETCODE);
			fprintf(item_db_fp, "  `attack` mediumint(9) unsigned default NULL," RETCODE);
			fprintf(item_db_fp, "  `defence` mediumint(9) unsigned default NULL," RETCODE);
			fprintf(item_db_fp, "  `range` tinyint(2) unsigned default NULL," RETCODE);
			fprintf(item_db_fp, "  `slots` tinyint(2) unsigned default NULL," RETCODE); // Max 5, but set to tinyint(2) to avoid possible BOOL replacement
			fprintf(item_db_fp, "  `equip_jobs` int(10) unsigned default NULL," RETCODE);
			fprintf(item_db_fp, "  `equip_genders` tinyint(2) unsigned default NULL," RETCODE); // Max 3 (1+2), but set to tinyint(2) to avoid possible BOOL replacement
			fprintf(item_db_fp, "  `equip_locations` smallint(4) unsigned default NULL," RETCODE);
			fprintf(item_db_fp, "  `weapon_level` tinyint(2) unsigned default NULL," RETCODE); // Max 4, but set to tinyint(2) to avoid possible BOOL replacement
			fprintf(item_db_fp, "  `equip_level` tinyint(3) unsigned default NULL," RETCODE);
			fprintf(item_db_fp, "  `view` tinyint(3) unsigned default NULL," RETCODE);
			fprintf(item_db_fp, "  `script_use` text," RETCODE);
			fprintf(item_db_fp, "  `script_equip` text," RETCODE);
			fprintf(item_db_fp, "  PRIMARY KEY (`id`)" RETCODE);
			fprintf(item_db_fp, ") TYPE=MyISAM;" RETCODE);
			fprintf(item_db_fp, RETCODE);
		} else {
			printf(CL_RED "Error: " CL_RESET "Failed to generate the '" ITEM_DB_SQL_NAME "' file." CL_RESET" \n");
			create_item_db_script = 0; // Not continue to try to create file after
		}
	}
//#endif /* Not TXT_ONLY */

	for(i = 0; i < 2; i++) {

		ln = 0;
		fp = fopen(filename[i], "r");
		if (fp == NULL) {
			if (i > 0)
				continue;
			printf(CL_RED "Error:" CL_RESET " Failed to load %s\n", filename[i]);
//#ifndef TXT_ONLY
			if (create_item_db_script && item_db_fp != NULL) {
				fclose(item_db_fp);
			}
//#endif /* Not TXT_ONLY */
			exit(1);
		}

		lines = 0;
		while(fgets(line, sizeof(line), fp)) { // fgets reads until maximum one less than size and add '\0' -> so, it's not necessary to add -1
			lines++;
			if ((line[0] == '/' && line[1] == '/') || line[0] == '\0' || line[0] == '\n' || line[0] == '\r') {
//#ifndef TXT_ONLY
				// Code to create SQL item db
				if (create_item_db_script && item_db_fp != NULL) {
					/* Remove carriage return if exist */
					while(line[0] != '\0' && (line[(j = strlen(line) - 1)] == '\n' || line[j] == '\r'))
						line[j] = '\0';
					// Add comments in the SQL script
					if ((line[0] == '/' && line[1] == '/' && strlen(line) > 2)) {
						fprintf(item_db_fp, "#%s" RETCODE, line + 2);
					} else {
						fprintf(item_db_fp, RETCODE);
					}
				}
//#endif /* Not TXT_ONLY */
				continue;
			}

			/* Remove carriage return if exist */
			while(line[0] != '\0' && (line[(j = strlen(line) - 1)] == '\n' || line[j] == '\r'))
				line[j] = '\0';

			memset(str, 0, sizeof(str));
			for(j = 0, np = p = line; j < 17 && p; j++) {
				str[j] = p;
				p = strchr(p, ',');
				if (p) { *p++ = 0; np = p; }
			}
			if (str[0] == NULL)
				continue;

			nameid = atoi(str[0]);
			if (nameid <= 0 || nameid >= 20000)
				continue;
			if (battle_config.etc_log) {
				if (strlen(str[1]) > 24)
					printf(CL_YELLOW "Warning: " CL_RESET "Invalid item name (id: %d) - Name too long (> 24 char.) -> only 24 first characters are used.\n", nameid);
				if (strlen(str[2]) > 24)
					printf(CL_YELLOW "Warning: " CL_RESET "Invalid item jname (id: %d) - Name too long (> 24 char.) -> only 24 first characters are used.\n", nameid);
			}

			ln++;
			if (ln % 20 == 19) {
				printf("Reading item #%d (id: %d)...\r", ln, nameid);
				fflush(stdout);
			}

			// ID,Name,Jname,Type,Price,Sell,Weight,ATK,DEF,Range,Slot,Job,Gender,Loc,wLV,eLV,View
			id = itemdb_search(nameid);
			memset(id->name, 0, sizeof(id->name));
			strncpy(id->name, str[1], 24);
			memset(id->jname, 0, sizeof(id->jname));
			strncpy(id->jname, str[2], 24);
			id->type = atoi(str[3]);

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
			// Check for bad prices that can possibly cause exploits
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
			id->class = atoi(str[11]);
			id->sex = atoi(str[12]);
			id->sex = itemdb_check_gender(id); // Apply gender filtering
			if (id->equip != atoi(str[13])) {
				id->equip = atoi(str[13]);
			}
			id->wlv = atoi(str[14]);
			id->elv = atoi(str[15]);
			id->look = atoi(str[16]);
			id->flag.available = 1;
			id->flag.value_notdc = 0;
			id->flag.value_notoc = 0;
			id->flag.upper = 0;
			id->view_id = 0;

			id->use_script = NULL;
			id->equip_script = NULL;

			if ((p = strchr(np, '{')) != NULL) {
				id->use_script = parse_script((unsigned char *)p, lines);
				if ((p = strchr(p + 1, '{')) != NULL)
					id->equip_script = parse_script((unsigned char *)p, lines);
			}

//#ifndef TXT_ONLY
			// Code to create SQL item database
			if (create_item_db_script && item_db_fp != NULL) {
				char item_name[49]; // 24 * 2 + NULL
				char item_jname[49]; // 24 * 2 + NULL
				char script1[1024], script2[1024], temp_script[1024], comment_script[1024];
				char *p1, *p2;
				struct item_data *actual_item;

				actual_item = id;
				// Escape item names
				memset(item_name, 0, sizeof(item_name));
				mysql_escape_string(item_name, actual_item->name, strlen(actual_item->name));
				memset(item_jname, 0, sizeof(item_jname));
				mysql_escape_string(item_jname, actual_item->jname, strlen(actual_item->jname));
				// Extract item database scripts #1 and #2
				memset(script1, 0, sizeof(script1));
				memset(script2, 0, sizeof(script2));
				if ((p1 = strchr(np, '{')) != NULL && (p2 = strchr(p1, '}')) != NULL) {
					while(*p1 == ' ' || *p1 == '{')
						p1++;
					while((*p2 == ' ' || *p2 == '}') && p2 > p1)
						p2--;
					if (p2 > p1) {
						memset(temp_script, 0, sizeof(temp_script));
						strncpy(temp_script, p1, p2 - p1 + 1);
						memset(script1, 0, sizeof(script1));
						script1[0] = '\'';
						mysql_escape_string(script1 + 1, temp_script, strlen(temp_script));
						strcat(script1, "'");
					}
					// Search second script
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
							mysql_escape_string(script2 + 1, temp_script, strlen(temp_script));
							strcat(script2, "'");
						}
					}
				}
				memset(comment_script, 0 , sizeof(comment_script));
				if ((p1 = strchr(np, '/')) != NULL && p1[1] == '/') {
					comment_script[0] = ' ';
					comment_script[1] = '#';
					strcpy(comment_script + 2, p1 + 2);
				}
				// Create request
				fprintf(item_db_fp, "INSERT INTO `%s` VALUES (%d, '%s', '%s', %d,"
				                                             " %d, %d,"
				                                             " %d, %d, %d, %d,"
				                                             " %d, %d, %d, %d,"
				                                             " %d, %d, %d,"
				                                             " %s, %s);%s" RETCODE,
				                    item_db_db, nameid, item_name, item_jname, actual_item->type,
				                    atoi(str[4]), atoi(str[5]), // id->value_buy, id->value_sell: Not modified
				                    actual_item->weight, actual_item->atk, actual_item->def, actual_item->range,
				                    actual_item->slot, actual_item->class, actual_item->sex, actual_item->equip,
				                    actual_item->wlv, actual_item->elv, actual_item->look,
				                    (script1[0] == 0) ? "NULL" : script1, (script2[0] == 0) ? "NULL" : script2, comment_script);
			}
//#endif /* Not TXT_ONLY */
		}
		fclose(fp);
		printf(CL_GREEN "Loaded: " CL_RESET "'" CL_WHITE "%s" CL_RESET "' read ('" CL_WHITE "%d" CL_RESET "' entrie%s).\n", filename[i], ln, (ln > 1) ? "s" : "");
	}

//#ifndef TXT_ONLY
		// Code to create SQL item db
		if (create_item_db_script && item_db_fp != NULL) {
			fclose(item_db_fp);
		}
//#endif /* Not TXT_ONLY */

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
		{"db/random/item_bluebox.txt",     blue_box,    &blue_box_count,    &blue_box_default },
		{"db/random/item_purplebox.txt",   violet_box,  &violet_box_count,  &violet_box_default },
		{"db/random/item_cardalbum.txt",   card_album,  &card_album_count,  &card_album_default },
		{"db/random/item_giftbox.txt",     gift_box,    &gift_box_count,    &gift_box_default },
		{"db/random/item_cookiebag.txt",   cookie,      &cookie_count,      &cookie_default },
		{"db/random/item_randquiver.txt",  rand_quiver, &rand_quiver_count, &rand_quiver_default },
		{"db/random/item_taminggift.txt",  tame_gift,   &tame_gift_count,   &tame_gift_default },
		{"db/random/item_jewelrybox.txt",  jewel_box,   &jewel_box_count,   &jewel_box_default },
		{"db/random/item_wrappedmask.txt", wrap_mask,   &wrap_mask_count,   &wrap_mask_default },
		{"db/random/item_scrollpack.txt",  scroll_pack, &scroll_pack_count, &scroll_pack_default },
		{"db/random/item_firstaidkit.txt", aid_kit,     &aid_kit_count,     &aid_kit_default },
		{"db/random/item_foodbundle.txt",  food_bundle, &food_bundle_count, &food_bundle_default },
		{"db/random/item_redbox.txt",      red_box,     &red_box_count,     &red_box_default },
		{"db/random/item_greenbox.txt",    green_box,   &green_box_count,   &green_box_default },
	};

	for(i = 0; i < sizeof(data) / sizeof(data[0]); i++) {
		struct random_item_data *pd = data[i].pdata;
		int *pc = data[i].pcount;
		int *pdefault = data[i].pdefault;
		char *fn = data[i].filename;

		*pdefault = 0;
		if ((fp = fopen(fn, "r")) == NULL) {
			printf(CL_RED "Error:" CL_RESET " Failed to load %s\n", fn);
			continue;
		}

		while(fgets(line, sizeof(line), fp)) { // fgets reads until maximum one less than size and add '\0' -> so, it's not necessary to add -1
			if ((line[0] == '/' && line[1] == '/') || line[0] == '\0' || line[0] == '\n' || line[0] == '\r')
				continue;
			// It's not necessary to remove 'carriage return ('\n' or '\r')
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
		printf(CL_GREEN "Loaded: " CL_RESET "'" CL_WHITE "%s" CL_RESET "' read ('" CL_WHITE "%d" CL_RESET "' entrie%s).\n", fn, *pc, (*pc > 1) ? "s" : "");
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
		printf(CL_RED "Error:" CL_RESET " Failed to load db/item_avail.txt.\n");
		return -1;
	}

	while(fgets(line, sizeof(line), fp)) { // fgets reads until maximum one less than size and add '\0' -> so, it's not necessary to add -1
		struct item_data *id;
		if ((line[0] == '/' && line[1] == '/') || line[0] == '\0' || line[0] == '\n' || line[0] == '\r')
			continue;
		// It's not necessary to remove 'carriage return ('\n' or '\r')
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
	printf(CL_GREEN "Loaded: " CL_RESET "'" CL_WHITE "db/item_avail.txt" CL_RESET "' read ('" CL_WHITE "%d" CL_RESET "' entrie%s).\n", ln, (ln > 1) ? "s" : "");

	return 0;
}

/*==========================================
 * Reads item_group_db.txt file
 *------------------------------------------
 */
static int itemdb_read_itemgroup(void)
{
	FILE *fp;
	char line[1024];
	int ln = 0;
	int groupid, itemid, j;
	char *str[MAX_GROUPITEMS], *p;

	if ((fp = fopen("db/item_group_db.txt", "r")) == NULL) {
		printf(CL_RED "Error:" CL_RESET " Failed to load db/item_group_db.txt.\n");
		return -1;
	}

	while(fgets(line, sizeof(line), fp)) { // fgets reads until maximum one less than size and add '\0' -> so, it's not necessary to add -1
		if ((line[0] == '/' && line[1] == '/') || line[0] == '\0' || line[0] == '\n' || line[0] == '\r')
			continue;

		memset(str,0,sizeof(str));
		for(j = 0, p = line; j < MAX_GROUPITEMS && p; j++) {
			str[j] = p;
			p = strchr(p, ',');
			if (p) *p++ = 0;
		}

		if (str[0] == NULL)
			continue;

		groupid = atoi(str[0]);
		if (groupid < 0 || groupid >= MAX_ITEMGROUP)
			continue;

		for (j = 1; j <= MAX_GROUPITEMS && str[j]; j++) {
			itemid = atoi(str[j]);

			if (itemid < 0 || itemid >= 20000 || !itemdb_exists(itemid))
				continue;

			itemgroup_db[groupid].id[j-1] = itemid;
			itemgroup_db[groupid].qty = j;
		}
		for (j = 1; j < MAX_GROUPITEMS && itemgroup_db[groupid].qty; j++) {	// If no item, shift trailing elements
			if (itemgroup_db[groupid].id[j-1] == 0 &&
				itemgroup_db[groupid].id[j] != 0) 
			{
				itemgroup_db[groupid].id[j-1] = itemgroup_db[groupid].id[j];
				itemgroup_db[groupid].id[j] = 0;
				itemgroup_db[groupid].qty = j;
			}
		}
		ln++;
	}
	fclose(fp);
	printf(CL_GREEN "Loaded: " CL_RESET "'" CL_WHITE "db/item_group_db.txt" CL_RESET "' read ('" CL_WHITE "%d" CL_RESET "' entrie%s).\n", ln, (ln > 1) ? "s" : "");

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

	printf(CL_GREEN "Loaded: " CL_RESET "File '" CL_WHITE "data\\idnum2itemdisplaynametable.txt" CL_RESET "' read.\n");

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

	printf(CL_GREEN "Loaded: " CL_RESET "'" CL_WHITE "data\\num2cardillustnametable.txt" CL_RESET "' read.\n");

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
		// Fix unuseable cards
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

	printf(CL_GREEN "Loaded: " CL_RESET "File '" CL_WHITE "data\\itemslottable.txt" CL_RESET "' read.\n");

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

	printf(CL_GREEN "Loaded: " CL_RESET "File '" CL_WHITE "data\\itemslotcounttable.txt" CL_RESET "' read.\n");

	return 0;
}

/*==========================================
 * 装備制限ファイル読み出し
 *------------------------------------------
 */
static int itemdb_read_noequip(void)
{
	FILE *fp;
	char line[1024];
	int ln = 0;
	int nameid, j;
	char *str[32],*p;
	struct item_data *id;

	if ((fp = fopen("db/item_noequip.txt", "r")) == NULL) {
		printf(CL_RED "Error:" CL_RESET " Failed to load db/item_noequip.txt\n");
		return -1;
	}

	while(fgets(line, sizeof(line), fp)) { // fgets reads until maximum one less than size and add '\0' -> so, it's not necessary to add -1
		if ((line[0] == '/' && line[1] == '/') || line[0] == '\0' || line[0] == '\n' || line[0] == '\r')
			continue;
		// It's not necessary to remove 'carriage return ('\n' or '\r')
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

		id->flag.no_equip = atoi(str[1]); // Mode = 1- not in PvP, 2- GvG restriction, 3- PvP and GvG which restriction

		ln++;
	}
	fclose(fp);

	printf(CL_GREEN "Loaded: " CL_RESET "'" CL_WHITE "db/item_noequip.txt" CL_RESET "' read ('" CL_WHITE "%d" CL_RESET "' entrie%s).\n", ln, (ln > 1) ? "s" : "");

	return 0;
}

/*==========================================
 * Reads item_norefine.txt database [Harbin]
 *------------------------------------------
 */
static int itemdb_read_norefine(void) {
	struct item_data *itemdat;
	char linebuffer[128];
	int itemid, counter = 0;

	FILE *db = fopen("db/item_norefine.txt", "r");

	if(db == NULL) {
		printf(CL_RED "Error: " CL_RESET "db/item_norefine.txt (missing file) \n");
		exit(1); // It's obligatory
	}

	while(fgets(linebuffer, sizeof(linebuffer), db)) {
		if(linebuffer[0] == '/' && linebuffer[1] == '/') // Skip commentaries
			continue;
		if(linebuffer[0] == '\0' || linebuffer[0] == '\n' || linebuffer[0] == '\r') // Skip empty lines
			continue;

		sscanf(linebuffer, "%d", &itemid);

		if(!itemid)
			continue;
		if(itemid < 0 || itemid >= 20000 || !(itemdat = itemdb_exists(itemid))) {
			printf("Invalid item id (%d) on db/item_norefine.txt file database\n", itemid);
			continue;
		}

		itemdat->flag.no_refine = 1;
		counter++;
	}

	printf(CL_GREEN "Loaded: " CL_RESET "'" CL_WHITE "db/item_norefine.txt" CL_RESET "' read ('" CL_WHITE "%d" CL_RESET "' entries).\n", counter);
	return 1;
}

/*==========================================
 * Reads item trade restrictions [Skotlex]
 *------------------------------------------
 */
static int itemdb_read_itemtrade(void)
{
	FILE *fp;
	int nameid, j, flag, gmlv, ln = 0;
	char line[1024], *str[10], *p;
	struct item_data *id;

	if ((fp = fopen("db/item_bound.txt", "r")) == NULL) {
		printf(CL_RED "Error: " CL_RESET "db/item_bound.txt\n");
		return -1;
	}

	while (fgets(line, sizeof(line) - 1, fp)) {
		if (line[0] == '/' && line[1] == '/')
			continue;
		memset(str, 0, sizeof(str));
		for (j = 0, p = line; j < 3 && p; j++) {
			str[j] = p;
			p = strchr(p, ',');
			if(p) *p++ = 0;
		}

		if (j < 3 || str[0] == NULL || (nameid = atoi(str[0])) < 0 || nameid >= 20000 || !(id = itemdb_exists(nameid)))
			continue;

		flag = atoi(str[1]);
		gmlv = atoi(str[2]);
		
		if (flag > 0 && flag < 128 && gmlv > 0) { // Check range
			id->flag.trade_restriction = flag;
			id->gm_lv_trade_override = gmlv;
			ln++;
		}
	}
	fclose(fp);

	printf(CL_GREEN "Loaded: " CL_RESET "'" CL_WHITE "db/item_bound.txt" CL_RESET "' read ('" CL_WHITE "%d" CL_RESET "' entrie%s).\n", ln, (ln > 1) ? "s" : "");

	return 0;
}

/*==========================================
 * Reads item_upper.txt database
 *------------------------------------------
 */
static int itemdb_read_upper(void) {
	FILE *fp;
	char line[1024];
	int ln = 0, nameid, j;
	char *str[32], *p;
	struct item_data *id;

	if((fp = fopen("db/item_upper.txt","r")) == NULL){
		printf(CL_RED "Error:" CL_RESET " Failed to load db/item_upper.txt\n");
		return -1;
	}
	
	while(fgets(line, sizeof(line) - 1, fp)){
		if(line[0] == '/' && line[1] == '/')
			continue;
			
		memset(str, 0, sizeof(str));
		
		for(j = 0, p = line; j < 2 && p; j++){
			str[j] = p;
			p = strchr(p, ',');
			if(p)
				*p++ = 0;
		}
		
		if(str[0] == NULL || str[1] == NULL)
			continue;

		nameid = atoi(str[0]);
		if(nameid <= 0 || nameid >= 20000 || !(id = itemdb_exists(nameid))) {
			printf("Invalid item id (%d) on db/item_upper.txt file database\n", nameid);

			continue;
		}

		id->flag.upper = atoi(str[1]);

		ln++;
	}
	fclose(fp);
	
	printf(CL_GREEN "Loaded: " CL_RESET "'" CL_WHITE "db/item_upper.txt" CL_RESET "' read ('" CL_WHITE "%d" CL_RESET "' entrie%s).\n", ln, (ln > 1) ? "s" : "");

	return 0;
}

/* ----------------------- *
 * Read Ammo Type Database *
 * ----------------------- */
static int itemdb_read_ammo(void)
{
	FILE *db;
	struct item_data *idat;
	char line[1024];
	int i, id, val, counter = 0;

	db = fopen("db/item_ammo.txt", "r");

	if(db == NULL)
	{
		printf(CL_RED "Error: " CL_RESET "Failed to load db/item_ammo.txt\n");
		exit(1);
	}

	while(fgets(line, sizeof(line), db))
	{
		/* skip comments and empty lines */
		if(line[0] == '/' && line[1] == '/')
			continue;
		if(line[0] == '\0' || line[0] == '\n' || line[0] == '\r')
			continue;

		/* get values */
		if(sscanf(line, "%d,%d", &id, &val) != 2)
		{
			printf(CL_YELLOW "Warning: " CL_RESET "Invalid line (%d) on db/item_ammo.txt\n", counter);
			continue;
		}

		/* validity checking */
		if(id < 0 || id >= 20000 || !(idat = itemdb_exists(id)) || idat->type != 10)
		{
			printf(CL_YELLOW "Warning: " CL_RESET "Invalid itemid (%d) on db/item_ammo.txt\n", id);
			continue;
		}
		if(val < 1 || val > 6)
		{
			printf(CL_YELLOW "Warning: " CL_RESET "Invalid ammo type for itemid %d on db/item_ammo.txt\n", id);
			continue;
		}

		/* set ammotype and update counter */
		idat->flag.ammotype = val;
		counter++;
	}

	/* more validity checking */
	for(i = 1; i <= 20000; i++)
	{
		if(!(idat = itemdb_exists(i)))
			continue;
		if(idat->type == 10 && idat->flag.ammotype == 0)
		{
			printf(CL_YELLOW "Warning: " CL_RESET "Item %d has no ammotype set\n", i);
			continue;
		}
	}

	/* display some information */
	if(counter == 1)
		printf(CL_GREEN "Loaded: " CL_RESET "'" CL_WHITE "db/item_ammo.txt" CL_RESET "' read ('" CL_WHITE "1" CL_RESET "' entry).\n");
	else
		printf(CL_GREEN "Loaded: " CL_RESET "'" CL_WHITE "db/item_ammo.txt" CL_RESET "' read ('" CL_WHITE "%d" CL_RESET "' entries).\n", counter);

	fclose(db);
	return 0;
}

/*==========================================
 * Wether item gender restriction is applied or not [Proximus]
 * Based on certain items and settings
 *------------------------------------------
 */
static int itemdb_check_gender(struct item_data *id) {

	switch(id->nameid) {
		case 2338: // Wedding Dress
		case WEDDING_RING_M: // Male Wedding Ring
		case WEDDING_RING_F: // Female Wedding Ring
		case 7170: // Tuxedo
			return id->sex;
	}

	if (id->look == 13 && id->type == 4) // Musical instruments are always Male-only
		return id->sex;
	if (id->look == 14 && id->type == 4) // Whips are always Female-only
		return id->sex;

	return (battle_config.ignore_items_gender? 2 : id->sex);
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
		/* +----+--------------+---------------+------+-----------+------------+--------+--------+---------+-------+-------+------------+---------------+-----------------+--------------+-------------+------+------------+--------------+
		   |  0 |            1 |             2 |    3 |         4 |          5 |      6 |      7 |       8 |     9 |    10 |         11 |            12 |              13 |           14 |          15 |   16 |         17 |           18 |
		   +----+--------------+---------------+------+-----------+------------+--------+--------+---------+-------+-------+------------+---------------+-----------------+--------------+-------------+------+------------+--------------+
		   | id | name_english | name_japanese | type | price_buy | price_sell | weight | attack | defence | range | slots | equip_jobs | equip_genders | equip_locations | weapon_level | equip_level | view | script_use | script_equip |
		   +----+--------------+---------------+------+-----------+------------+--------+--------+---------+-------+-------+------------+---------------+-----------------+--------------+-------------+------+------------+--------------+ */

		nameid = sql_get_integer(0);

		// If the identifier is not within the valid range, process the next row
		if (nameid == 0 || nameid >= 20000)
			continue;
		if (battle_config.etc_log) {
			if (strlen(sql_get_string(1)) > 24)
				printf(CL_YELLOW "WARNING: Invalid item name" CL_RESET" (id: %d) - Name too long (> 24 char.) -> only 24 first characters are used.\n", nameid);
			if (strlen(sql_get_string(2)) > 24)
				printf(CL_YELLOW "WARNING: Invalid item jname" CL_RESET" (id: %d) - Name too long (> 24 char.) -> only 24 first characters are used.\n", nameid);
		}

		ln++;
		if (ln % 20 == 19) {
			printf("Reading item #%ld (id: %d)...\r", ln, nameid);
			fflush(stdout);
		}

		// Insert a new row into the item database

		/*CALLOC(id, struct item_data, 1);

		numdb_insert(item_db, (int) nameid, id);*/

		// ----------
		id = itemdb_search(nameid);

		memset(id->name, 0, sizeof(id->name));
		strncpy(id->name, sql_get_string(1), 24);
		memset(id->jname, 0, sizeof(id->jname));
		strncpy(id->jname, sql_get_string(2), 24);

		id->type = sql_get_integer(3);

		// Fix NULL for buy/sell prices
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
		// Check for bad prices that can possibly cause exploits
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
		id->class = (sql_get_string(11) != NULL) ? sql_get_integer(11) : 0;
		id->sex   = (sql_get_string(12) != NULL) ? sql_get_integer(12) : 0;
		id->sex   = itemdb_check_gender(id); // Apply gender filtering
		id->equip = (sql_get_string(13) != NULL) ? sql_get_integer(13) : 0;
		id->wlv   = (sql_get_string(14) != NULL) ? sql_get_integer(14) : 0;
		id->elv   = (sql_get_string(15) != NULL) ? sql_get_integer(15) : 0;
		id->look  = (sql_get_string(16) != NULL) ? sql_get_integer(16) : 0;
		
		id->view_id = 0;

		// ----------

		if (sql_get_string(17) != NULL) {
			if (sql_get_string(17)[0] == '{')
				id->use_script = parse_script((unsigned char *)sql_get_string(17), 0);
			else {
				sprintf(script, "{%s}", sql_get_string(17));
				id->use_script = parse_script((unsigned char *)script, 0);
			}
		} else {
			id->use_script = NULL;
		}

		if (sql_get_string(18) != NULL) {
			if (sql_get_string(18)[0] == '{')
				id->equip_script = parse_script((unsigned char *)sql_get_string(18), 0);
			else {
				sprintf(script, "{%s}", sql_get_string(18));
				id->equip_script = parse_script((unsigned char *)script, 0);
			}
		} else {
			id->equip_script = NULL;
		}

		// ----------

		id->flag.available   = 1;
		id->flag.value_notdc = 0;
		id->flag.value_notoc = 0;
		id->flag.upper = 0;
	}

	printf(CL_GREEN "Loaded: " CL_RESET "'" CL_WHITE "%s" CL_RESET "' read ('" CL_WHITE "%ld" CL_RESET "' entrie%s).\n", item_db_db, ln, (ln > 1) ? "s" : "");

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

	FREE(id->use_script);
	FREE(id->equip_script);
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
