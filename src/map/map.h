// Copyright (c) Freya Development Team - Licensed under GNU GPL
// For more information, see License.txt in the main folder

#ifndef _MAP_H_
#define _MAP_H_

#include <stdarg.h>
#include <config.h>
#include <mmo.h>

#define MAX_PC_CLASS 4054
#define MAX_PC_JOB_CLASS 34
#define PC_CLASS_BASE 0
#define PC_CLASS_BASE2 (PC_CLASS_BASE + 4001)
#define PC_CLASS_BASE3 (PC_CLASS_BASE2 + 22)
#define MAX_NPC_PER_MAP 512
#define BLOCK_SIZE 8 // Never zero
#define AREA_SIZE battle_config.area_size
#define LOCAL_REG_NUM 16
#define LIFETIME_FLOORITEM 60
#define DAMAGELOG_SIZE 30
#define LOOTITEM_SIZE 10
#define MAX_SKILL_LEVEL 99
#define MAX_STATUSCHANGE 257
#define SC_COMMON_MIN 0
#define SC_COMMON_MAX 10
#define MAX_SKILLUNITGROUP 32
#define MAX_MOBSKILLUNITGROUP 8
#define MAX_SKILLUNITGROUPTICKSET 32
#define MAX_SKILLTIMERSKILL 32
#define MAX_MOBSKILLTIMERSKILL 10
#define MAX_MOBSKILL 50
#define MAX_EVENTQUEUE 2
#define MAX_EVENTTIMER 32
#define NATURAL_HEAL_INTERVAL 500
#define MAX_FLOORITEM 500000
#define MAX_WALKPATH 32 // previously 48
//#define MAX_DROP_PER_MAP 48 // -> now, dynamic
#define MAX_IGNORE_LIST 1000 // now it's used in dynamic, but maximum is to send all names in 1 packet -> (32768/24) = 1365 ->1000
#define MAX_VENDING 12
#define MAX_CHATTERS 20
#define MAX_MOB_DB 2000

#define MAX_PC_BONUS 10

#define DEFAULT_AUTOSAVE_INTERVAL 120*1000

#define OPTION_HIDE 0x40

char messages_filename[1024];

enum { BL_NUL, BL_PC, BL_NPC, BL_MOB, BL_ITEM, BL_CHAT, BL_SKILL , BL_PET };
enum { WARP, SHOP, SCRIPT, MONS };
struct block_list {
	struct block_list *next, *prev;
	int id;
	short m, x, y;
	unsigned char type;
	unsigned char subtype;
};

struct walkpath_data {
	unsigned char path_len,path_pos,path_half;
	unsigned char path[MAX_WALKPATH];
};
struct script_reg {
	int index;
	int data;
};
struct script_regstr {
	int index;
	char data[256];
};
struct status_change {
	int timer;
	int val1, val2, val3, val4;
};
struct vending {
	short index;
	unsigned short amount;
	unsigned int value;
};

struct skill_unit_group;
struct skill_unit {
	struct block_list bl;

	struct skill_unit_group *group;

	int limit;
	int val1, val2;
	short alive, range;
};
struct skill_unit_group {
	int src_id;
	int party_id;
	int guild_id;
	int map;
	int target_flag;
	unsigned int tick;
	int limit, interval;

	int skill_id, skill_lv;
	int val1, val2, val3;
	char *valstr;
	int unit_id;
	int group_id;
	int unit_count, alive_count;
	struct skill_unit *unit;
};
struct skill_unit_group_tickset {
	unsigned int tick;
	int id;
};
struct skill_timerskill {
	int timer;
	int src_id;
	int target_id;
	int map;
	short x, y;
	short skill_id, skill_lv;
	int type;
	int flag;
};

struct map_session_data {
	struct block_list bl;
	struct {
		unsigned auth : 1;
		unsigned change_walk_target : 1;
		unsigned attack_continue : 1;
		unsigned menu_or_input : 1;
		unsigned dead_sit : 2; // 0: standup, 1: dead, 2: sit
		unsigned previously_sit_hp : 1; // 0: not sit when is was previously HP regen, 1: sit... (to avoid people that sit down and stand up between 2 timers)
		unsigned previously_sit_sp : 1; // 0: not sit when is was previously SP regen, 1: sit... (to avoid people that sit down and stand up between 2 timers)
		unsigned skillcastcancel : 1;
		unsigned waitingdisconnect : 1;
		unsigned lr_flag : 2;
		unsigned connect_new : 1;
		unsigned arrow_atk : 1;
		unsigned attack_type : 3; // 1: BF_WEAPON, 2: BF_MAGIC, 4: BF_MISC
		unsigned skill_flag : 1;
		unsigned gangsterparadise : 1;
		unsigned rest : 1;
		unsigned produce_flag : 1;
		unsigned make_arrow_flag : 1;
		unsigned potionpitcher_flag : 1;
		unsigned storage_flag : 1;
		unsigned modified_storage_flag : 2; // 0: not modified, 1: modified, 2: modified and sended to char-server for saving
		unsigned snovice_flag : 2; // (0 to 3)
		struct guild * gmaster_flag;
		unsigned event_death : 1;
		unsigned event_kill : 1;
		unsigned event_disconnect : 1;

		unsigned autoloot_rate : 14; // (0-10000) drop rate of items concerned by the autoloot (from 0% to x%).
		unsigned autolootloot_flag : 1; // 0: no auto-loot, 1: autoloot (for looted items)
		unsigned displayexp_flag : 1; // 0 no xp display, 1: exp display
		unsigned displaydrop_rate : 14; // (0-10000) rate is from 0% to <max_drop_rate>%
		unsigned displaylootdrop : 1; // Displays or not items (of loot) dropped by a monster when you kill it.
		//unsigned display_player_hp : 1; // 0 no hp display, 1: hp display
		unsigned main_flag : 1;
		unsigned refuse_request_flag : 1;
//		unsigned night : 1; // 0 day, 1 night
		unsigned relocate : 1; //1: Re-warp a player
	} state;
	struct {
		unsigned killer : 1;
		unsigned killable : 1;
		unsigned restart_full_recover : 1;
		unsigned no_castcancel : 1;
		unsigned no_castcancel2 : 1;
		unsigned no_sizefix : 1;
		unsigned no_magic_damage : 1;
		unsigned no_weapon_damage : 1;
		unsigned no_gemstone : 1;
		unsigned infinite_endure : 1;
		unsigned intravision : 1;
		unsigned noknockback : 1;
	} special_state;
	int char_id, login_id1, login_id2;
	unsigned char sex, GM_level;
	unsigned char packet_ver; // 0: old, 1: 7july04, 2: 13july04, 3: 26july04, 4: 9aug04/16aug04/17aug04, 5: 6sept04, 6: 21sept04, 7: 18oct04, 8: 25oct04/08nov04, 9: 6dec04, 10: 10jan05, 11: 9may05, 12: 28jun05, 13: 4april06
	struct mmo_charstatus status;
	struct item_data *inventory_data[MAX_INVENTORY]; // just pointers on items of database
	short equip_index[11];
	unsigned short unbreakable_equip;
	unsigned short unbreakable;
	int weight, max_weight;
	int cart_weight, cart_max_weight;
	short cart_num; // , cart_max_num; it's always MAX_CART... removed

	char mapname[17];

	int fd;
	short to_x, to_y;
	short speed, prev_speed;
	short opt1, opt2, opt3;
	char dir, head_dir;
	unsigned int client_tick;
	//unsigned int first_client_tick; // to check speed hack
	//unsigned int tick_at_start; // to check speed hack
	//char first_check_done; // to check speed hack (to avoid lag when we set value, so don't considere first check)
	struct walkpath_data walkpath;
	int walktimer;
	int next_walktime;
	int npc_id, areanpc_id, npc_shopid;
	int npc_pos;
	int npc_menu;
	int npc_amount;
	int npc_stack, npc_stackmax;
	unsigned char *npc_script, *npc_scriptroot; // just pointers on scripts
	struct script_data *npc_stackbuf;
	char npc_str[256]; // npc_str = 255 + NULL
	unsigned int chatID;
	unsigned int idletime; // for party experience
	time_t lastpackettime; // for disconnection if player is inactive

	struct ignore {
		char name[25]; // 24 + NULL
	} *ignore; // dynamic system
	unsigned ignore_num : 15; // (0-32767) number of ignored people (real max: 10000, agains hackers)
	unsigned ignoreAll : 1; // ignore (yes/no)

	unsigned global_reg_num : 15; // 0-700 (GLOBAL_REG_NUM)
	unsigned global_reg_dirty : 1; // must be saved or not
	struct global_reg *global_reg;
	unsigned account_reg_num : 15; // 0-700 (ACCOUNT_REG_NUM)
	unsigned account_reg_dirty : 1; // must be saved or not
	struct global_reg *account_reg;
	unsigned account_reg2_num : 15; // 0-700 (ACCOUNT_REG2_NUM)
	unsigned account_reg2_dirty : 1; // must be saved or not
	struct global_reg *account_reg2;
	unsigned char friend_num; // 0-40 (MAX_FRIENDS)
	struct friend *friends;

	int attacktimer;
	int attacktarget;
	short attacktarget_lv;
	unsigned int attackabletime;

	int followtimer; // [MouseJstr]
	int followtarget;

	unsigned int emotionlasttime; // to limit flood with emotion packets
	unsigned int last_saving; // to limit successive savings with auto-save

	short attackrange, attackrange_;
	int skilltimer;
	int skilltarget;
	short skillx, skilly;
	short skillid, skilllv;
	short skillitem, skillitemlv;
	short skillid_old, skilllv_old;
	short skillid_dance, skilllv_dance;
	struct skill_unit_group skillunit[MAX_SKILLUNITGROUP];
	struct skill_unit_group_tickset skillunittick[MAX_SKILLUNITGROUPTICKSET];
	struct skill_timerskill skilltimerskill[MAX_SKILLTIMERSKILL];
	char blockskill[MAX_SKILL];
	short cloneskill_id;
	int potion_hp, potion_sp, potion_per_hp, potion_per_sp;

	int invincible_timer;
	unsigned int canact_tick;
	unsigned int canmove_tick;
	unsigned int canlog_tick;
	unsigned int canregen_tick;
	unsigned int canuseitem_tick; // [Skotlex]
	int hp_sub, sp_sub;
	int inchealhptick, inchealsptick, inchealspirithptick, inchealspiritsptick;

	unsigned int success_counter; //For Potion success counter. Fame system [Proximus]

	short view_class;
	short weapontype1, weapontype2;
	short disguiseflag, disguise; // [Valaris]
	int paramb[6], paramc[6], parame[6], paramcard[6];
	int hit,flee, flee2, aspd, amotion, dmotion;
	int watk, watk2, atkmods[3];
	int def, def2, mdef, mdef2, critical, matk1, matk2;
	int atk_ele, def_ele, star, overrefine;
	int castrate, delayrate, hprate, sprate, dsprate;
	int addele[10], addrace[12], addsize[3], subele[10], subrace[12];
	int addeff[10], addeff2[10], reseff[10];
	int watk_,watk_2, atkmods_[3], addele_[10], addrace_[12], addsize_[3]; //�����0�
	int atk_ele_, star_,overrefine_; //�����0�
	int base_atk, atk_rate;
	int weapon_atk[16], weapon_atk_rate[16];
	int arrow_atk, arrow_ele, arrow_cri, arrow_hit, arrow_range;
	int arrow_addele[10], arrow_addrace[12], arrow_addsize[3], arrow_addeff[10], arrow_addeff2[10];
	int nhealhp, nhealsp, nshealhp, nshealsp, nsshealhp, nsshealsp;
	int aspd_rate, speed_rate, hprecov_rate, sprecov_rate, critical_def, double_rate;
	int near_attack_def_rate, long_attack_def_rate, magic_def_rate, misc_def_rate;
	int matk_rate, ignore_def_ele, ignore_def_race, ignore_def_ele_, ignore_def_race_;
	int ignore_mdef_ele, ignore_mdef_race;
	int magic_addele[10], magic_addrace[12], magic_subrace[12];
	int perfect_hit, get_zeny_num, get_zeny_rate;
	int critical_rate, hit_rate, flee_rate, flee2_rate, def_rate, def2_rate, mdef_rate, mdef2_rate;
	int def_ratio_atk_ele, def_ratio_atk_ele_, def_ratio_atk_race, def_ratio_atk_race_;
	int add_damage_class_count, add_damage_class_count_, add_magic_damage_class_count;
	short add_damage_classid[10], add_damage_classid_[10], add_magic_damage_classid[10];
	int add_damage_classrate[10], add_damage_classrate_[10], add_magic_damage_classrate[10];
	short add_def_class_count, add_mdef_class_count;
	short add_def_classid[10], add_mdef_classid[10];
	int add_def_classrate[10], add_mdef_classrate[10];
	short monster_drop_item_count;
	struct {
		short id, group;
		int race, rate;
	} monster_drop[MAX_PC_BONUS];
	int double_add_rate, speed_add_rate, aspd_add_rate, perfect_hit_add, get_zeny_add_num;
	short splash_range, splash_add_range;
	struct {
		short id, lv, type;
		int rate;
	} autospell[MAX_PC_BONUS] /* Auto-spell on attack*/ , autospell2[MAX_PC_BONUS] /* Auto-spell when being hit */ , autospell3[MAX_PC_BONUS]; /* Auto-spell when being hit with magical damage */
	short hp_drain_rate, hp_drain_per, sp_drain_rate, sp_drain_per;
	short hp_drain_rate_, hp_drain_per_, sp_drain_rate_, sp_drain_per_;
	short hp_drain_value, sp_drain_value, hp_drain_value_, sp_drain_value_;
	short sp_gain_race[12];
	short sp_gain_value_race[12];
	int expaddrace[12];
	int short_weapon_damage_return, long_weapon_damage_return;
	int weapon_coma_ele[10], weapon_coma_race[12];
	int break_weapon_rate, break_armor_rate;
	short add_steal_rate;
	int crit_atk_rate;
	int critaddrace[12];
	short no_regen;
	int addeff3[10];
	struct { // skillatk raises bonus dmg% of skills, skillblown increases bonus blewcount for some skills.
		short id, val;
	} skillatk[MAX_PC_BONUS];
	unsigned short unstripable_equip;
	short add_damage_classid2[10], add_damage_class_count2;
	int add_damage_classrate2[10];
	short sp_gain_value, hp_gain_value;
	short sp_drain_type;
	short sp_vanish_rate, sp_vanish_per;	
	short ignore_def_mob, ignore_def_mob_;
	int hp_loss_tick, hp_loss_rate;
	short hp_loss_value, hp_loss_type;
	int addrace2[6], addrace2_[6];
	int subsize[3];
	short unequip_hpdamage;
	short unequip_spdamage;

	int itemhealrate[7];

	short spiritball, spiritball_old;
	int spirit_timer[MAX_SKILL_LEVEL];
	int magic_damage_return;
	int random_attack_increase_add, random_attack_increase_per; // [Valaris]
	int perfect_hiding; // [Valaris]
	int classchange; // [Valaris]

	short doridori_counter;

	int jailtimer; // timer to finish jail

	int reg_num;
	struct script_reg *reg;
	int regstr_num;
	struct script_regstr *regstr;

	struct status_change sc_data[MAX_STATUSCHANGE];
	short sc_count;
	struct square dev;

	int trade_partner;
	int deal_item_index[10];
	int deal_item_amount[10];
	int deal_zeny;
	char deal_locked; // 0: no trade, 1: trade accepted, 2: trade valided (button 'Ok' pressed)

	int party_sended, party_invite, party_invite_account;
	int party_hp, party_x, party_y;

	int guild_sended, guild_invite, guild_invite_account;
	int guild_emblem_id, guild_alliance, guild_alliance_account;

	int friend_invite; // char_id of player that invite the player

	int guildspy; // [Syrus22]
	int partyspy; // [Syrus22]

	int vender_id;
	int vend_num;
	char message[81]; // 80 + NULL (shop title)
	struct vending vending[MAX_VENDING];

	int catch_target_class;
	struct s_pet pet;
	struct pet_db *petDB; // just pointer on the pet
	struct pet_data *pd;
	int pet_hungry_timer;

	int pvp_point, pvp_rank, pvp_timer, pvp_lastusers;

	char eventqueue[MAX_EVENTQUEUE][50]; // 49 + NULL
	int eventtimer[MAX_EVENTTIMER];

	int last_skillid, last_skilllv;

	unsigned char change_level;
	
	short tk_mission_target_id; // Stores the target mob_id for TK_MISSION
	short tk_mission_count; // Stores the bounty kill count for TK_MISSION
	char fakename[24];
	short viewsize; // for tiny/large types

	struct map_session_data *repair_target;
};

struct npc_timerevent_list {
	int timer, pos;
};
struct npc_label_list {
	char name[25]; // 24 + NULL
	int pos;
};
struct npc_item_list {
	int nameid,value;
};
struct npc_data {
	struct block_list bl;
	short n;
	short class, dir;
	short speed;
	char name[25]; // 24 + NULL
	char exname[25]; // 24 + NULL
	int chat_id;
	short opt1, opt2, opt3, option;
	short flag;
	int walktimer; // [Valaris]
	short to_x, to_y; // [Valaris]
	struct walkpath_data walkpath;
	unsigned int next_walktime;
	unsigned int canmove_tick;

	struct { // [Valaris]
		unsigned state : 8;
		unsigned change_walk_target : 1;
		unsigned walk_easy : 1;
	} state;

	union {
		struct {
			unsigned char *script;
			short xs, ys;
			int guild_id;
			int timer, timerid, timeramount, nexttimer, rid;
			unsigned int timertick;
			struct npc_timerevent_list *timer_event;
			short label_list_num; // more than 32767... not in actual configuration
			struct npc_label_list *label_list;
			int src_id;
		} scr;
		struct npc_item_list *shop_item;
		struct {
			short xs, ys;
			short x, y;
			char name[17]; // 16 + NULL
		} warp;
	} u;
	// �����ɒ����o���0������(shop_item���"���)

	char eventqueue[MAX_EVENTQUEUE][50]; // 49 + NULL
	int eventtimer[MAX_EVENTTIMER];
	short arenaflag;
};
struct mob_data {
	struct block_list bl;
	short n;
	short base_class, class, dir, mode, level;
	short m, x0, y0, xs, ys;
	char name[25]; // 24 + NULL
	int spawndelay1, spawndelay2;
	struct {
		unsigned state : 8;
		unsigned skillstate : 8;
		unsigned targettype : 1;
		unsigned steal_flag : 1;
		unsigned steal_coin_flag : 1;
		unsigned skillcastcancel : 1;
		unsigned master_check : 1;
		unsigned change_walk_target : 1;
		unsigned walk_easy : 1;
		unsigned special_mob_ai : 3; // 0: nothing, 1: cannibalize, 2-3: spheremine
		unsigned soul_change_flag : 1;
		int provoke_flag;
	} state;
	int timer;
	short to_x, to_y;
	short target_dir;
	short speed;
	short attacked_count;
	int hp;
	int target_id, attacked_id;
	short target_lv;
	struct walkpath_data walkpath;
	unsigned int next_walktime;
	unsigned int attackabletime;
	unsigned int last_deadtime, last_spawntime, last_thinktime;
	unsigned int canmove_tick;
	short move_fail_count;
	struct {
		int id; // char_id
		int dmg;
	} dmglog[DAMAGELOG_SIZE];
	struct item *lootitem;
	short lootitem_count;

	struct status_change sc_data[MAX_STATUSCHANGE];
	short sc_count;
	short opt1, opt2, opt3, option;
	short min_chase;

	int guild_id; // Guilds' gardians & emperiums, otherwize = 0

	int deletetimer;

	int skilltimer;
	int skilltarget;
	short skillx, skilly;
	short skillid, skilllv, skillidx;
	unsigned int skilldelay[MAX_MOBSKILL];
	int def_ele;
	int master_id, master_dist;
	int exclusion_src, exclusion_party, exclusion_guild;
	struct skill_timerskill skilltimerskill[MAX_MOBSKILLTIMERSKILL];
	struct skill_unit_group skillunit[MAX_MOBSKILLUNITGROUP];
	struct skill_unit_group_tickset skillunittick[MAX_SKILLUNITGROUPTICKSET];
	char npc_event[50];
	unsigned char size;
	int owner;
};

struct pet_data {
	struct block_list bl;
	short n;
	short class, dir;
	short speed;
	char name[25]; // 24 + NULL
	struct {
		unsigned state : 8;
		unsigned skillstate : 8;
		unsigned change_walk_target : 1;
		short skillbonus;
	} state;
	int timer;
	short to_x, to_y;
	short equip;
	struct walkpath_data walkpath;
	int target_id;
	short target_lv;
	int move_fail_count;
	unsigned int attackabletime, next_walktime, last_thinktime;
	int skilltype, skillval, skilltimer, skillduration; // [Valaris]
	int skillbonustype, skillbonusval; // [Valaris]
	int skillbonustimer, recoverytimer, healtimer, magtimer, skillattacktimer; // timers
	struct item *lootitem;
	short loot; // [Valaris]
	short lootmax; // [Valaris]
	short lootitem_count;
	short lootitem_weight;
	unsigned int lootitem_timer;
	struct skill_timerskill skilltimerskill[MAX_MOBSKILLTIMERSKILL]; // [Valaris]
	struct skill_unit_group skillunit[MAX_MOBSKILLUNITGROUP]; // [Valaris]
	struct skill_unit_group_tickset skillunittick[MAX_SKILLUNITGROUPTICKSET]; // [Valaris]
	struct map_session_data *msd;
};

enum { MS_IDLE, MS_WALK, MS_ATTACK, MS_DEAD, MS_DELAY };

enum { NONE_ATTACKABLE, ATTACKABLE };

enum { ATK_LUCKY = 1, ATK_FLEE, ATK_DEF}; // ���y�i���e�B�v�Z�p

// For equipment breaking/stripping effects
enum {
	EQP_WEAPON = 1, // Both weapons
	EQP_ARMOR  = 2, // Armor
	EQP_SHIELD = 4, // Shield
	EQP_HELM   = 8  // Top-head headgear
};

struct map_data {
	char name[17]; // 16 + NULL
	char *gat; // NULL�0��map_data_other_server������
	char *alias; // [MouseJstr]
	struct block_list **block;
	struct block_list **block_mob;
	short m;
	short xs, ys;
	short bxs, bys;
	int npc_num;
	int users;
	struct {
		unsigned alias : 1; // for alias name at load, not a map flag
		unsigned nomemo : 1; // 0
		unsigned noteleport : 1; // 1
		unsigned nosave : 1; // 2
		unsigned nobranch : 1; // 3 // forbid usage of Dead Branch (604), Bloody Branch (12103) and Poring Box (12109)
		unsigned nopenalty : 1; // 4
		unsigned nozenypenalty : 1; // 5
		unsigned pvp : 1; // 6
		unsigned pvp_noparty : 1; // 7
		unsigned pvp_noguild : 1; // 8
		unsigned gvg : 1; // 9
		unsigned gvg_noparty : 1; // 10
		unsigned notrade : 1; // 11
		unsigned noskill : 1; // 12
		unsigned nowarp : 1; // 13
	//	unsigned nopvp : 1; // 14 (reverse of pvp) - not a real map flag
		unsigned noicewall : 1; // 15
		unsigned snow : 1; // 16
		unsigned fog : 1; // 17
		unsigned sakura : 1; // 18
		unsigned leaves : 1; // 19
		unsigned rain : 1; // 20
		unsigned indoors : 1; // 21
		unsigned nogo : 1; // 22
		unsigned nospell : 1; // 23
		unsigned nowarpto : 1; // 24
		unsigned noreturn : 1; // 25
		unsigned monster_noteleport : 1; // 26
		unsigned pvp_nocalcrank : 1; // 27
		unsigned nobaseexp : 1; // 28/29
		unsigned nojobexp : 1; // 28/30
		unsigned nomobdrop : 1; // 31/32
		unsigned nomvpdrop : 1; // 31/33
		unsigned pvp_nightmaredrop :1; // 34
		unsigned nogmcmd :7; // (0-100) 35 // when you use this mapflag (nogmcmd, you forbid GM commands upper than the mentionned level, included @ban, @kick, etc...)
		unsigned mingmlvl :7; // (0-100) 36
		unsigned gvg_dungeon : 1; // 37
	} flag;
	struct point save;
	struct npc_data *npc[MAX_NPC_PER_MAP];
	struct drop_list {
		int drop_id;
		char drop_type; // 1-3
		int drop_per;
	} *drop_list; // MAX_DROP_PER_MAP -> now, dynamic
	unsigned short drop_list_num;
};

struct map_data_other_server {
	char name[25]; // 24 + NULL
	char *gat; // NULL�������f
	unsigned long ip;
	unsigned int port;
	struct map_data *map;
};
struct flooritem_data {
	struct block_list bl;
	short subx, suby;
	int cleartimer;
	int first_get_id, second_get_id, third_get_id;
	unsigned int first_get_tick, second_get_tick, third_get_tick;
	int owner; // who drop the item (to authorize to get item at any moment)
	struct item item_data;
};

// Job IDs
enum {
	JOB_NOVICE,		// 0
	JOB_SWORDMAN, JOB_MAGE, JOB_ARCHER, JOB_ACOLYTE, JOB_MERCHANT, JOB_THIEF,	// 1-6
	JOB_KNIGHT,	JOB_PRIEST,	JOB_WIZARD,	JOB_BLACKSMITH,	JOB_HUNTER, JOB_ASSASSIN, JOB_KNIGHT2,	// 7-13
	JOB_CRUSADER, JOB_MONK, JOB_SAGE, JOB_ROGUE, JOB_ALCHEMIST, JOB_BARD, JOB_DANCER, JOB_CRUSADER2,	// 14-21

	JOB_WEDDING, // 22
	JOB_SUPER_NOVICE, // 23
	JOB_GUNSLINGER, // 24
	JOB_NINJA, // 25
	JOB_XMAS, // 26

	JOB_NOVICE_HIGH = 4001,	// 4001
	JOB_SWORDMAN_HIGH, JOB_MAGE_HIGH, JOB_ARCHER_HIGH, JOB_ACOLYTE_HIGH, JOB_MERCHANT_HIGH, JOB_THIEF_HIGH,	// 4002-4007
	JOB_LORD_KNIGHT, JOB_HIGH_PRIEST, JOB_HIGH_WIZARD, JOB_WHITESMITH, JOB_SNIPER, JOB_ASSASSIN_CROSS,		// 4008-4013
	JOB_LORD_KNIGHT2,	// 4014
	JOB_PALADIN, JOB_CHAMPION, JOB_PROFESSOR, JOB_STALKER, JOB_CREATOR, JOB_CLOWN, JOB_GYPSY, // 4015-4021
	JOB_PALADIN2,	// 4022

	JOB_BABY,		// 4023
	JOB_BABY_SWORDMAN, JOB_BABY_MAGE, JOB_BABY_ARCHER, JOB_BABY_ACOLYTE, JOB_BABY_MERCHANT, JOB_BABY_THIEF,	// 4024-4029
	JOB_BABY_KNIGHT, JOB_BABY_PRIEST, JOB_BABY_WIZARD, JOB_BABY_BLACKSMITH, JOB_BABY_HUNTER, JOB_BABY_ASSASSIN,	// 4030-4035
	JOB_BABY_KNIGHT2,	// 4036
	JOB_BABY_CRUSADER, JOB_BABY_MONK, JOB_BABY_SAGE, JOB_BABY_ROGUE, JOB_BABY_ALCHEMIST, JOB_BABY_BARD, JOB_BABY_DANCER,	// 4037-4043
	JOB_BABY_CRUSADER2,	//4044

	JOB_SUPER_BABY,	// 4045

	JOB_TAEKWON, JOB_STAR_GLADIATOR, JOB_STAR_GLADIATOR2, JOB_SOUL_LINKER,	// 4046-4049
	
	JOB_BON_GUN, JOB_DEATH_KNIGHT, JOB_DARK_COLLECTOR, JOB_MUNAK, // 4050-4053
};

enum {
	SP_SPEED,SP_BASEEXP,SP_JOBEXP,SP_KARMA,SP_MANNER,SP_HP,SP_MAXHP,SP_SP,	// 0-7
	SP_MAXSP,SP_STATUSPOINT,SP_0a,SP_BASELEVEL,SP_SKILLPOINT,SP_STR,SP_AGI,SP_VIT,	// 8-15
	SP_INT,SP_DEX,SP_LUK,SP_CLASS,SP_ZENY,SP_SEX,SP_NEXTBASEEXP,SP_NEXTJOBEXP,	// 16-23
	SP_WEIGHT,SP_MAXWEIGHT,SP_1a,SP_1b,SP_1c,SP_1d,SP_1e,SP_1f,	// 24-31
	SP_USTR,SP_UAGI,SP_UVIT,SP_UINT,SP_UDEX,SP_ULUK,SP_26,SP_27,	// 32-39
	SP_28,SP_ATK1,SP_ATK2,SP_MATK1,SP_MATK2,SP_DEF1,SP_DEF2,SP_MDEF1,	// 40-47
	SP_MDEF2,SP_HIT,SP_FLEE1,SP_FLEE2,SP_CRITICAL,SP_ASPD,SP_36,SP_JOBLEVEL,	// 48-55
	SP_UPPER,SP_PARTNER,SP_CART,SP_FAME,SP_UNBREAKABLE,	//56-60
	SP_CARTINFO = 99,	// 99

	SP_BASEJOB = 119,	// 100+19
	SP_BASECLASS = 120, // GetPureJob
	// original 1000-
	SP_ATTACKRANGE = 1000,	SP_ATKELE,SP_DEFELE,	// 1000-1002
	SP_CASTRATE, SP_MAXHPRATE, SP_MAXSPRATE, SP_SPRATE, // 1003-1006
	SP_ADDELE, SP_ADDRACE, SP_ADDSIZE, SP_SUBELE, SP_SUBRACE, // 1007-1011
	SP_ADDEFF, SP_RESEFF,	// 1012-1013
	SP_BASE_ATK,SP_ASPD_RATE,SP_HP_RECOV_RATE,SP_SP_RECOV_RATE,SP_SPEED_RATE, // 1014-1018
	SP_CRITICAL_DEF,SP_NEAR_ATK_DEF,SP_LONG_ATK_DEF, // 1019-1021
	SP_DOUBLE_RATE, SP_DOUBLE_ADD_RATE, SP_MATK, SP_MATK_RATE, // 1022-1025
	SP_IGNORE_DEF_ELE,SP_IGNORE_DEF_RACE, // 1026-1027
	SP_ATK_RATE,SP_SPEED_ADDRATE,SP_ASPD_ADDRATE, // 1028-1030
	SP_MAGIC_ATK_DEF,SP_MISC_ATK_DEF, // 1031-1032
	SP_IGNORE_MDEF_ELE,SP_IGNORE_MDEF_RACE, // 1033-1034
	SP_MAGIC_ADDELE,SP_MAGIC_ADDRACE,SP_MAGIC_SUBRACE, // 1035-1037
	SP_PERFECT_HIT_RATE,SP_PERFECT_HIT_ADD_RATE,SP_CRITICAL_RATE,SP_GET_ZENY_NUM,SP_ADD_GET_ZENY_NUM, // 1038-1042
	SP_ADD_DAMAGE_CLASS, SP_ADD_MAGIC_DAMAGE_CLASS, SP_ADD_DEF_CLASS, SP_ADD_MDEF_CLASS, // 1043-1046
	SP_ADD_MONSTER_DROP_ITEM, SP_DEF_RATIO_ATK_ELE, SP_DEF_RATIO_ATK_RACE, SP_ADD_SPEED, // 1047-1050
	SP_HIT_RATE, SP_FLEE_RATE, SP_FLEE2_RATE, SP_DEF_RATE, SP_DEF2_RATE, SP_MDEF_RATE, SP_MDEF2_RATE, // 1051-1057
	SP_SPLASH_RANGE, SP_SPLASH_ADD_RANGE, SP_AUTOSPELL, SP_HP_DRAIN_RATE, SP_SP_DRAIN_RATE, // 1058-1062
	SP_SHORT_WEAPON_DAMAGE_RETURN, SP_LONG_WEAPON_DAMAGE_RETURN, SP_WEAPON_COMA_ELE, SP_WEAPON_COMA_RACE, // 1063-1066
	SP_ADDEFF2, SP_BREAK_WEAPON_RATE, SP_BREAK_ARMOR_RATE, SP_ADD_STEAL_RATE, // 1067-1070
	SP_MAGIC_DAMAGE_RETURN, SP_RANDOM_ATTACK_INCREASE, SP_ALL_STATS, SP_AGI_VIT, SP_AGI_DEX_STR, SP_PERFECT_HIDE, // 1071-1076
	SP_DISGUISE, SP_CLASSCHANGE, // 1077-1078
	SP_HP_DRAIN_VALUE, SP_SP_DRAIN_VALUE, // 1079-1080
	SP_WEAPON_ATK, SP_WEAPON_ATK_RATE, // 1081-1082
	SP_DELAYRATE, // 1083

	// Missing 1084 (bHPDrainRateRace)
	// Missing 1085 (bSPDrainRateRace)

	SP_RESTART_FULL_RECORVER=2000, SP_NO_CASTCANCEL, SP_NO_SIZEFIX, SP_NO_MAGIC_DAMAGE, SP_NO_WEAPON_DAMAGE, SP_NO_GEMSTONE, // 2000-2005
	SP_NO_CASTCANCEL2, SP_INFINITE_ENDURE, SP_UNBREAKABLE_WEAPON, SP_UNBREAKABLE_ARMOR, SP_UNBREAKABLE_HELM, // 2006-2010
	SP_UNBREAKABLE_SHIELD, SP_LONG_ATK_RATE, // 2011-2012

	SP_CRIT_ATK_RATE, SP_CRITICAL_ADDRACE, SP_NO_REGEN, SP_ADDEFF_WHENHIT, SP_AUTOSPELL_WHENHIT, // 2013-2017
	SP_SKILL_ATK, SP_UNSTRIPABLE, SP_ADD_DAMAGE_BY_CLASS, // 2018-2020
	SP_SP_GAIN_VALUE, SP_IGNORE_DEF_MOB, SP_HP_LOSS_RATE, SP_ADDRACE2, SP_HP_GAIN_VALUE, // 2021-2025
	SP_SUBSIZE, SP_DAMAGE_WHEN_UNEQUIP, SP_ADD_ITEM_HEAL_RATE, SP_LOSESP_WHEN_UNEQUIP, SP_EXP_ADDRACE, // 2026-2030
	SP_SP_GAIN_RACE = 2031, // 2031

	// Missing 2032 (bSPSubRace2)
	// Missing 2033 (AddEffWhenHitShort)
	// Missing 2034 (UnstripableWeapon)
	// Missing 2035 (UnstripableArmor)
	// Missing 2036 (bUnstripableHelm)
	// Missing 2037 (bUnstripableShield)

	SP_INTRAVISION = 2038, // 2038
	SP_ADD_MONSTER_DROP_ITEMGROUP, SP_SP_LOSS_RATE, // 2039-2040

	// Missing 2041 (bAddSkillBlow)

	SP_SP_VANISH_RATE = 2042, SP_SP_GAIN_VALUE_RACE, // 2042-2043
	SP_NOKNOCKBACK, SP_AUTOSPELL_WHENHITMAGIC, // 2044-2045
};

enum {
	LOOK_BASE,LOOK_HAIR,LOOK_WEAPON,LOOK_HEAD_BOTTOM,LOOK_HEAD_TOP,LOOK_HEAD_MID,LOOK_HAIR_COLOR,LOOK_CLOTHES_COLOR,LOOK_SHIELD,LOOK_SHOES
};

// CELL
#define CELL_MASK		0x0f
#define CELL_NPC		0x80	// NPC�Z��
#define CELL_BASILICA	0x40	// BASILICA�Z��
#define CELL_MOONLIT	0x100
#define CELL_REGEN		0x200
/*
 * map_getcell()�}g�p�������t���O
 */
typedef enum {
	CELL_CHKWALL = 0,   // ��(�Z���^�C�v1)
	CELL_CHKWATER,      // ����(�Z���^�C�v3)
	CELL_CHKGROUND,     // �n�ʏ��Q��(�Z���^�C�v5)
	CELL_CHKPASS,       // �00\(�Z���^�C�v1,5�`O)
	CELL_CHKNOPASS,     // �0"s��(�Z���^�C�v1,5)
	CELL_GETTYPE,       // �Z���^�C�v����
	CELL_CHKNOPASS_NPC, // check if it is NOPASS or NPC
	CELL_CHKNPC=0x10,   // �^�b�`�^�C�v��NPC(�Z���^�C�v0x80�t���O)
	CELL_CHKBASILICA    // �o�W���J(�Z���^�C�v0x40�t���O)
} cell_t;
// map_setcell()�}g�p�������t���O
enum {
	CELL_SETNPC=0x10, // �^�b�`�^�C�v��NPC���Z�b�g
	CELL_SETBASILICA, // �o�W���J���Z�b�g
	CELL_CLRBASILICA  // �o�W���J���N���A
};

struct chat_data {
	struct block_list bl;

	char pass[9];   /* password */
	char title[61]; /* room title MAX 60 */
	unsigned char limit;     /* join limit */
	unsigned char trigger;
	unsigned char users;     /* current users */
	unsigned char pub;       /* room attribute */
	struct map_session_data *usersd[MAX_CHATTERS];
	struct block_list *owner_;
	struct block_list **owner;
	char npc_event[50];
};

extern struct map_data *map;
extern int map_num;
extern int autosave_interval;
extern unsigned char agit_flag; // 0: WoE not starting, Woe is running
extern unsigned char night_flag; // 0=Day, 1=Night

int map_getcell(int, int, int, cell_t);
int map_getcellp(struct map_data*, int, int, cell_t);
void map_setcell(int, int, int, int);
//extern int map_read_flag;
enum {
	READ_FROM_GAT,
	READ_FROM_BITMAP, CREATE_BITMAP,
	READ_FROM_BITMAP_COMPRESSED, CREATE_BITMAP_COMPRESSED
};

extern char motd_txt[];
extern char help_txt[];
extern char extra_add_file_txt[]; // To add items from external software (Use append to add a line)

extern char talkie_mes[];

extern char wisp_server_name[25]; // 24 + NULL
extern int server_char_id; // char id used by server
extern int server_fake_mob_id; // mob id of a invisible mod to check bots
extern char npc_language[6];
extern unsigned char map_is_alone; // define communication usage of map-server if it is alone

// �I�S�̏���
void map_setusers(int);
int map_getusers(void);
// block�S�A
int map_freeblock(void *bl);
void map_freeblock_lock(void);
void map_freeblock_unlock(void);
// block�A
int map_addblock(struct block_list *);
int map_delblock(struct block_list *);
void map_foreachinarea(int (*)(struct block_list*, va_list), int, int, int, int, int, int, ...);
// -- moonsoul (added map_foreachincell)
//void map_foreachincell(int (*)(struct block_list*, va_list), int, int, int, int, ...);
void map_foreachinmovearea(int (*)(struct block_list*, va_list), int, int, int, int, int, int, int, int, ...);
void map_foreachinpath(int (*)(struct block_list*, va_list), int, int, int, int, int, int, int, ...);
/*int map_countnearpc(int, int, int);*/
//block�A�0�
int map_count_oncell(int m, int x, int y);
struct skill_unit *map_find_skill_unit_oncell(struct block_list *, int x, int y, int skill_id, struct skill_unit *);
// �}~�Iobject�A
int map_addobject(struct block_list *);
int map_delobject(int);
int map_delobjectnofree(int id);
void map_foreachobject(int (*)(struct block_list*, va_list), int, ...);
//
void map_quit(struct map_session_data *);
void map_quit2(struct map_session_data *); // to free memory of dynamic allocation in session (even if player is auth or not)
// npc
int map_addnpc(int,struct npc_data *);

// ���A�C�e���A
int map_clearflooritem_timer(int,unsigned int,int,int);
#define map_clearflooritem(id) map_clearflooritem_timer(0,0,id,1)
int map_addflooritem(struct item *,int,int,int,int,struct map_session_data *,struct map_session_data *,struct map_session_data *, int owner_id, int);
int map_searchrandfreecell(int,int,int,int);

// �L����id�����L������ �`��A
void map_addchariddb(int charid, char *name);
void map_delchariddb(int charid);
int map_reqchariddb(struct map_session_data * sd, int charid);
char * map_charid2nick(int);

struct map_session_data * map_charid2sd(int);
struct map_session_data * map_id2sd(int);
struct block_list * map_id2bl(int);
int map_mapname2mapid(const char*); // map id on this server (m == -1 if not in actual map-server)
int map_mapname2ipport(const char*, int*, int*);
short map_checkmapname(const char *mapname);
int map_setipport(char *name, unsigned long ip, int port);
int map_eraseipport(char *name, unsigned long ip, int port);
int map_eraseallipport(void);
void map_addiddb(struct block_list *);
void map_deliddb(struct block_list *bl);
void map_foreachiddb(int (*)(void*, void*, va_list), ...);
//void map_addnickdb(struct map_session_data *);
struct map_session_data * map_nick2sd(char*);

// ����
int map_check_dir(int s_dir, int t_dir);
int map_calc_dir(struct block_list *src, int x, int y);

// path.c����
int path_search(struct walkpath_data*, int, int, int, int, int, int);
int path_search_long(int m, int x0, int y_0, int x1, int y_1);
int path_blownpos(int m, int x0, int y_0, int dx, int dy, int count);

int map_who(int fd);

void map_helpscreen(); // [Valaris]

extern char item_db_db[32]; // need in TXT too to generate SQL DB script (not change the default name in this case)
extern char mob_db_db[32]; // need in TXT too to generate SQL DB script (not change the default name in this case)

extern char create_item_db_script; // generate a script file to create SQL item_db (0: no, 1: yes)
extern char create_mob_db_script; // generate a script file to create SQL mob_db (0: no, 1: yes)

#ifdef USE_MYSQL

#ifdef __WIN32
#define __USE_W32_SOCKETS
#include <windows.h>
#endif

// MySQL
#include "../common/db_mysql.h"

extern int db_use_sqldbs;

extern char char_db[32];

extern unsigned char optimize_table; // optimize mob/item SQL db

#endif /* USE_MYSQL */

#endif // _MAP_H_
