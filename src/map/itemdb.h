// $Id: itemdb.h 683 2006-07-08 02:48:38Z zugule $

#ifndef _ITEMDB_H_
#define _ITEMDB_H_
#define ITEM_NAME_LENGTH 50

#include "map.h"

struct item_data
{
	int nameid;
	char name[ITEM_NAME_LENGTH], jname[ITEM_NAME_LENGTH];
	char cardillustname[64];
	int value_buy; // 0-600000
	int value_sell; // 0-1000 (but can be more)
	int type; // 0-11
	int maxchance;
	int sex; // 0-3
	int equip; // equip_location 0-32768
	int weight; // 0-8000
	int atk; // 0-250
	int def; // 0-11
	int range; // 0-11
	int slot; // 0-5
	int look; // 0-207
	int elv; // equip_level 0-95
	int wlv; // weapon_level 0-4
	unsigned int class_base[3];     //Specifies if the base can wear this item (split in 3 indexes per type: 1-1, 2-1, 2-2)
	unsigned class_upper : 3; //Specifies if the upper-type can equip it (1: normal, 2: upper, 3: baby)
	struct {
		unsigned short chance;
		int id;
	} mob[5];
	unsigned char *script;
	unsigned char *equip_script;
	unsigned char *unequip_script;
	struct
	{
		unsigned available : 1;
		unsigned value_notdc : 1;
		unsigned value_notoc : 1;
		unsigned no_equip : 3;
		unsigned no_drop : 1;		// Allow Item Dropping	'0: Yes' '1: No'
		unsigned no_use : 1;		// Allow Item Using		'0: Yes' '1: No'
		unsigned no_refine : 1;		// Allow Item Refining	'0: Yes' '1: No'
		unsigned no_trade : 2;		// Allow Item Trading	'0: Yes' '1: No' '2: Only With Partner'
		unsigned delay_consume : 1;
		unsigned autoequip : 1;
	} flag;
	short gm_lv_trade_override;
	int view_id;
};

struct random_item_data {
	int nameid;
	int per;
};

struct item_data* itemdb_searchname(const char *name);
struct item_data* itemdb_search(intptr_t nameid);
struct item_data* itemdb_exists(intptr_t nameid);
#define itemdb_type(n) itemdb_search(n)->type
#define itemdb_atk(n) itemdb_search(n)->atk
#define itemdb_def(n) itemdb_search(n)->def
#define itemdb_look(n) itemdb_search(n)->look
#define itemdb_weight(n) itemdb_search(n)->weight
#define itemdb_equip(n) itemdb_search(n)->equip
#define itemdb_usescript(n) itemdb_search(n)->script
#define itemdb_equipscript(n) itemdb_search(n)->equip_script
#define itemdb_unequipscript(n) itemdb_search(n)->unequip_script
#define itemdb_wlv(n) itemdb_search(n)->wlv
#define itemdb_range(n) itemdb_search(n)->range
#define itemdb_slot(n) itemdb_search(n)->slot
#define	itemdb_available(n) (itemdb_exists(n) && itemdb_search(n)->flag.available)
#define	itemdb_viewid(n) (itemdb_search(n)->view_id)

int itemdb_searchrandomid(int flags);

int itemdb_isequip(int);
int itemdb_isequip2(struct item_data *);
int itemdb_isequip3(int);
int itemdb_isdropable(int nameid);

void itemdb_reload(void);

void do_final_itemdb(void);
int do_init_itemdb(void);

#endif // _ITEMDB_H_
