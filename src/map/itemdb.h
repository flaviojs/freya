// Copyright (c) Freya Development Team - Licensed under GNU GPL
// For more information, see License.txt in the main folder

#ifndef _ITEMDB_H_
#define _ITEMDB_H_

#include "map.h"

#define MAX_RANDITEM	50000 // Max number of total random items in the /db/random/ databases
#define MAX_ITEMGROUP	32 // Max item groups
#define MAX_GROUPITEMS	50 // Max number of items per group

struct item_data
{
	int nameid;
	char name[25], jname[25]; // 24 + NULL
	char cardillustname[64];
	unsigned int value_buy; // 0-600000
	unsigned int value_sell; // 0-1000 (but can be more)
	char type; // 0-11
	unsigned int class; // 0-16777215
	char sex; // 0-3
	unsigned int equip; // equip_location 0-32768
	int weight; // 0-8000
	int atk; // 0-250
	int def; // 0-11
	char range; // 0-11
	char slot; // 0-5
	short look; // 0-207
	short elv; // equip_level 0-95
	char wlv; // weapon_level 0-4
	unsigned char *use_script;	// ‰ñ•œ‚Æ‚©‚à‘S•”‚±‚Ì’†‚Å‚â‚ë‚¤‚©‚È‚Æ
	unsigned char *equip_script;	// UŒ‚,–hŒä‚Ì‘®«Ý’è‚à‚±‚Ì’†‚Å‰Â”\‚©‚È?
	struct
	{
		unsigned available : 1;
		unsigned value_notdc : 1;
		unsigned value_notoc : 1;
		unsigned no_equip : 3;
		unsigned no_drop : 1;
		unsigned no_use : 1;
		unsigned no_refine : 1;
		unsigned trade_restriction : 7;
		unsigned upper : 4;			// 0 : all; 1 : base class; 2 : advanced class; 4 : baby class
		unsigned ammotype : 6;	/* 1:arrow 2:bullet, 3:sphere, 4:shuriken, 5:kunai, 6:venomknife */
	} flag;
	short gm_lv_trade_override;	// GM level required to bypass trade_restriction
	int view_id;
};

struct item_group {
	int qty; //Counts amount of items in the group.
	int id[MAX_GROUPITEMS];
};

struct random_item_data {
	int nameid;
	int per;
};

struct item_data* itemdb_searchname(const char *name);
struct item_data* itemdb_search(int nameid);
struct item_data* itemdb_exists(int nameid);
#define itemdb_type(n) itemdb_search(n)->type
#define itemdb_atk(n) itemdb_search(n)->atk
#define itemdb_def(n) itemdb_search(n)->def
#define itemdb_look(n) itemdb_search(n)->look
#define itemdb_weight(n) itemdb_search(n)->weight
#define itemdb_equip(n) itemdb_search(n)->equip
#define itemdb_usescript(n) itemdb_search(n)->use_script
#define itemdb_equipscript(n) itemdb_search(n)->equip_script
#define itemdb_wlv(n) itemdb_search(n)->wlv
#define itemdb_range(n) itemdb_search(n)->range
#define itemdb_slot(n) itemdb_search(n)->slot
#define	itemdb_available(n) (itemdb_exists(n) && itemdb_search(n)->flag.available)
#define	itemdb_viewid(n) (itemdb_search(n)->view_id)

int itemdb_group(int nameid);
int itemdb_searchrandomgroup(int groupid);
int itemdb_searchrandomid(int flags);

int itemdb_isequip(int);
int itemdb_isequip2(struct item_data *);
int itemdb_isequip3(int);

// item trade restrictions
int itemdb_isdropable(int nameid, int gmlv);
int itemdb_cantrade(int nameid, int gmlv, int gmlv2);
int itemdb_cansell(int nameid, int gmlv);
int itemdb_canstore(int nameid, int gmlv);
int itemdb_canguildstore(int nameid, int gmlv);
int itemdb_cancartstore(int nameid, int gmlv);
int itemdb_canonlypartnertrade(int nameid, int gmlv, int gmlv2);

void itemdb_reload(void);

void do_final_itemdb(void);
int do_init_itemdb(void);

#endif // _ITEMDB_H_
