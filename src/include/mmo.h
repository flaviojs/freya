// $Id: mmo.h 646 2006-06-09 20:10:49Z akrus $

#ifndef	_MMO_H_
#define	_MMO_H_

#include <time.h>
#include <stdint.h>
#include "../common/utils.h"

#if defined __CYGWIN || defined __WIN32
#define RETCODE "\r\n"
#else
#define RETCODE "\n"
#endif

#define RFIFOSIZE_SERVER (64*1024) /* memo size is 60000 bytes */
#define WFIFOSIZE_SERVER (32*1024) /* a character take 17kb. it's sended often, and buffer must have at east this size */

#define MAX_INVENTORY 100
#define MAX_AMOUNT 30000
#define MAX_ZENY 1000000000
#define MAX_CART 100
#define MAX_SKILL 1100
#define GLOBAL_REG_NUM 700 // (dynamic allocation, but with a limit for size of str line in file)
#define ACCOUNT_REG_NUM 700 // (dynamic allocation, but with a limit for size of str line in file)
#define ACCOUNT_REG2_NUM 700 // (dynamic allocation, but with a limit for size of str line in file)
#define DEFAULT_WALK_SPEED 150
#define MIN_WALK_SPEED 0
#define MAX_WALK_SPEED 1000
#define MAX_STORAGE 300
#define MAX_GUILD_STORAGE 1000
#define MAX_PARTY 12
#define GUILD_EXTENTION_BONUS 6
//#define MAX_GUILD 56 // (16+4*10) SKILL Guild EXTENTION +4 (http://guide.ragnarok.co.kr/GuildSystem.asp)
#define MAX_GUILD 16 + (GUILD_EXTENTION_BONUS * 10) // (16+6*10) SKILL Guild EXTENTION +6 (kRO Patch - 9/27/05: The Guild skill [Expand Capacity] which increases number of people allowed in the guild, now adds 6 people capacity per level instead of 4.)
#define MAX_GUILDPOSITION 20 // increased max guild positions to accomodate for all members [Valaris] (removed) [PoW]
#define MAX_GUILDEXPLUSION 32
#define MAX_GUILDALLIANCE 16
#define MAX_GUILDSKILL 15
#define MAX_GUILDCASTLE 24 // increased to include novice castles [Valaris]
#define MAX_GUILDLEVEL 50

// for produce
#define MIN_ATTRIBUTE 0
#define MAX_ATTRIBUTE 4
#define ATTRIBUTE_NORMAL 0
#define MIN_STAR 0
#define MAX_STAR 3

//#define MIN_PORTAL_MEMO 0
#define MAX_PORTAL_MEMO 3

#define MAX_STATUS_TYPE 5

#define WEDDING_RING_M 2634
#define WEDDING_RING_F 2635

#define MAX_FRIENDS 40

enum {
	JOB_NOVICE,
	JOB_SWORDMAN,
	JOB_MAGE,
	JOB_ARCHER,
	JOB_ACOLYTE,
	JOB_MERCHANT,
	JOB_THIEF,
	JOB_KNIGHT,
	JOB_PRIEST,
	JOB_WIZARD,
	JOB_BLACKSMITH,
	JOB_HUNTER,
	JOB_ASSASSIN,
	JOB_KNIGHT2,
	JOB_CRUSADER,
	JOB_MONK,
	JOB_SAGE,
	JOB_ROGUE,
	JOB_ALCHEMIST,
	JOB_BARD,
	JOB_DANCER,
	JOB_CRUSADER2,
	JOB_WEDDING,
	JOB_SUPER_NOVICE,
	JOB_GUNSLINGER,
	JOB_NINJA,
	JOB_XMAS,

	JOB_NOVICE_HIGH = 4001,
	JOB_SWORDMAN_HIGH,
	JOB_MAGE_HIGH,
	JOB_ARCHER_HIGH,
	JOB_ACOLYTE_HIGH,
	JOB_MERCHANT_HIGH,
	JOB_THIEF_HIGH,
	JOB_LORD_KNIGHT,
	JOB_HIGH_PRIEST,
	JOB_HIGH_WIZARD,
	JOB_WHITESMITH,
	JOB_SNIPER,
	JOB_ASSASSIN_CROSS,
	JOB_LORD_KNIGHT2,
	JOB_PALADIN,
	JOB_CHAMPION,
	JOB_PROFESSOR,
	JOB_STALKER,
	JOB_CREATOR,
	JOB_CLOWN,
	JOB_GYPSY,
	JOB_PALADIN2,

	JOB_BABY,
	JOB_BABY_SWORDMAN,
	JOB_BABY_MAGE,
	JOB_BABY_ARCHER,
	JOB_BABY_ACOLYTE,
	JOB_BABY_MERCHANT,
	JOB_BABY_THIEF,
	JOB_BABY_KNIGHT,
	JOB_BABY_PRIEST,
	JOB_BABY_WIZARD,
	JOB_BABY_BLACKSMITH,
	JOB_BABY_HUNTER,
	JOB_BABY_ASSASSIN,
	JOB_BABY_KNIGHT2,
	JOB_BABY_CRUSADER,
	JOB_BABY_MONK,
	JOB_BABY_SAGE,
	JOB_BABY_ROGUE,
	JOB_BABY_ALCHEMIST,
	JOB_BABY_BARD,
	JOB_BABY_DANCER,
	JOB_BABY_CRUSADER2,
	JOB_SUPER_BABY,

	JOB_TAEKWON,
	JOB_STAR_GLADIATOR,
	JOB_STAR_GLADIATOR2,
	JOB_SOUL_LINKER,

	JOB_BON_GUN,
	JOB_DEATH_KNIGHT,
	JOB_DARK_COLLECTOR,
	JOB_MUNAK,

};

struct item {
	int id;
	short nameid;
	short amount;
	unsigned short equip;
	unsigned identify : 1; // 0-1
	unsigned refine : 4; // 0-10
	char attribute;
	short card[4];
};

struct point{
	char map[17]; // 16 + NULL
	short x, y;
};

struct skill {
	unsigned short id; // 0-10015
	unsigned char lv; // 0-100
	unsigned char flag; // 0, 1, 2-12, 13
};

struct global_reg {
	char str[33]; // 32 + NULL
	int value;
};

#ifdef USE_SQL
struct status_change_data {
	unsigned short type;
	int tick, val1, val2, val3, val4;
};
#endif

struct s_pet {
	int account_id;
	int char_id;
	int pet_id;
	short class;
	short level;
	short egg_id; // pet egg id
	short equip; // pet equip name_id
	short intimate; // pet friendly
	short hungry; // pet hungry
	char name[25]; // 24 + NULL
	unsigned rename_flag : 4; // 0-1 (bits: 1->4)
	unsigned incuvate : 4; // 0-1 (bits: 1->4)
};

struct friend {
	int account_id;
	int char_id;
	char name[25]; // 24 + NULL
};

struct mmo_charstatus {
	intptr_t char_id;
	intptr_t account_id;
	intptr_t partner_id;

	int base_exp, job_exp, zeny;

	short class;
	short status_point, skill_point;
	int hp, max_hp, sp, max_sp;
	short option, karma, manner;
	short hair, hair_color, clothes_color;
	int party_id, guild_id, pet_id;

	short weapon, shield;
	short head_top, head_mid, head_bottom;

	char name[25]; // 24 + NULL
	unsigned char base_level, job_level;
	short str, agi, vit, int_, dex, luk;
	unsigned char_num : 5; // 0-9 (bits: 4->5)
	unsigned sex : 3; // 0-2 (bits: 2->3)

	struct point last_point, save_point, memo_point[MAX_PORTAL_MEMO];
	struct item inventory[MAX_INVENTORY], cart[MAX_CART];
	struct skill skill[MAX_SKILL];
};

struct storage {
	int account_id;
	unsigned char storage_status;
	short storage_amount;
	struct item storage[MAX_STORAGE];
};

struct guild_storage {
	int guild_id;
	unsigned char storage_status;
	unsigned modified_storage_flag : 1; // set to 1 when guild storage was modified
	short storage_amount;
	struct item storage[MAX_GUILD_STORAGE];
};

struct map_session_data;

struct gm_account {
	int account_id;
	unsigned char level;
};

struct party_member {
	int account_id;
	char name[25]; // 24 + NULL
	char map[17]; // 16 + NULL
	unsigned leader : 1; // 0-1
	unsigned online : 1; // 0-1
	unsigned short lv;
	struct map_session_data *sd;
};

struct party {
	int party_id;
	char name[25]; // 24 - NULL
	unsigned char exp; // 0-1
	unsigned char item, itemc;
	struct party_member member[MAX_PARTY];
};

struct guild_member {
	int account_id, char_id;
	short hair, hair_color, gender, class, lv;
	int exp, exp_payper;
	short online, position;
	int rsv1, rsv2;
	char name[25]; // 24 + NULL
	struct map_session_data *sd;
};

struct guild_position {
	char name[25]; // 24 + NULL
	int mode;
	int exp_mode;
};

struct guild_alliance {
	int opposition;
	int guild_id;
	char name[25]; // 24 + NULL
};

struct guild_explusion {
	char name[25]; // 24 + NULL
	char mes[41]; // 40 + NULL
	char acc[41]; // 40 + NULL
	int account_id;
	int rsv1, rsv2, rsv3;
};

struct guild_skill {
	short id; // 10000-10014 (GD_SKILLBASE & MAX_GUILDSKILL)
	short lv;
};

struct guild {
	int guild_id;
	short guild_lv, connect_member, max_member, average_lv;
	int exp, next_exp;
	short skill_point;
//	char castle_id; // not more used (always -1)
	char name[25], master[25]; // 24 + NULL
	struct guild_member member[MAX_GUILD];
	struct guild_position position[MAX_GUILDPOSITION];
	char mes1[61], mes2[121]; // 61 + NULL, 120 + NULL
	short emblem_len;
	int emblem_id;
	char emblem_data[2048];
	struct guild_alliance alliance[MAX_GUILDALLIANCE];
	struct guild_explusion explusion[MAX_GUILDEXPLUSION];
	struct guild_skill skill[MAX_GUILDSKILL];
};

struct guild_castle {
	int castle_id;
	char map_name[17]; // 16 + NULL
	char castle_name[25]; // 24 + NULL
	char castle_event[25]; // 24 + NULL
	int guild_id;
	int economy;
	int defense;
	int triggerE;
	int triggerD;
	int nextTime;
	int payTime;
	int createTime;
	unsigned visibleC : 1; // 0 or 1
	unsigned visibleG0 : 1; // 0 or 1
	unsigned visibleG1 : 1; // 0 or 1
	unsigned visibleG2 : 1; // 0 or 1
	unsigned visibleG3 : 1; // 0 or 1
	unsigned visibleG4 : 1; // 0 or 1
	unsigned visibleG5 : 1; // 0 or 1
	unsigned visibleG6 : 1; // 0 or 1
	unsigned visibleG7 : 1; // 0 or 1
	int Ghp0; // added Guardian HP
	int Ghp1;
	int Ghp2;
	int Ghp3;
	int Ghp4;
	int Ghp5;
	int Ghp6;
	int Ghp7;
	int GID0;
	int GID1;
	int GID2;
	int GID3;
	int GID4;
	int GID5;
	int GID6;
	int GID7;
};

struct square {
	int val1[5];
	int val2[5];
};

enum {
	GBI_EXP        = 1, // ギルドのEXP
	GBI_GUILDLV    = 2, // ギルドのLv
	GBI_SKILLPOINT = 3, // ギルドのスキルポイント
	GBI_SKILLLV    = 4, // ギルドスキルLv

	GMI_POSITION   = 0, // メンバーの役職変更
	GMI_EXP        = 1  // メンバーのEXP
};

enum {
	GD_SKILLBASE        = 10000,
	GD_APPROVAL         = 10000,
	GD_KAFRACONTACT     = 10001,
	GD_GUARDIANRESEARCH = 10002,
	GD_CHARISMA         = 10003,
	GD_GUARDUP          = 10003,
	GD_EXTENSION        = 10004,
	GD_GLORYGUILD       = 10005,
	GD_LEADERSHIP       = 10006,
	GD_GLORYWOUNDS      = 10007,
	GD_SOULCOLD         = 10008,
	GD_HAWKEYES         = 10009,
	GD_BATTLEORDER      = 10010,
	GD_REGENERATION     = 10011,
	GD_RESTORE          = 10012,
	GD_EMERGENCYCALL    = 10013,
	GD_DEVELOPMENT      = 10014
};

#endif // _MMO_H_
