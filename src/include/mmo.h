// Copyright (c) Freya Development Team - Licensed under GNU GPL
// For more information, see License.txt in the main folder

#ifndef	_MMO_H_
#define	_MMO_H_

#include <time.h>
#include "../common/utils.h" // LCCWIN32

#if defined __CYGWIN || defined __WIN32
// txtaâlogaa!aR�a«�oa·t@C⬹aR⬰ü�sR�[h
#define RETCODE "\r\n" // (CR/LF�FWindowsn)
#else
#define RETCODE "\n" // (LF�FUnixn�j
#endif

#define RFIFOSIZE_SERVER (64*1024) /* memo size is 60000 bytes */
#define WFIFOSIZE_SERVER (32*1024) /* a character take 17kb. it's sended often, and buffer must have at east this size */

//#define MAX_MAP_PER_SERVER 768 // no more used in char-servers or map-servers (dynamic allocations)
#define MAX_INVENTORY 100
#define MAX_AMOUNT 30000
#define MAX_ZENY 1000000000 // 1G zeny
#define MAX_CART 100
#define MAX_SKILL 1020
#define GLOBAL_REG_NUM 700 // (dynamic allocation, but with a limit for size of str line in file)
#define ACCOUNT_REG_NUM 700 // (dynamic allocation, but with a limit for size of str line in file)
#define ACCOUNT_REG2_NUM 700 // (dynamic allocation, but with a limit for size of str line in file)
#define DEFAULT_WALK_SPEED 150
#define MIN_WALK_SPEED 0
#define MAX_WALK_SPEED 1000
#define MAX_STORAGE 300
#define MAX_GUILD_STORAGE 1000
#define MAX_PARTY 12
#define MAX_PARTY_SHARE 10
#define GUILD_EXTENTION_BONUS 6
//#define MAX_GUILD 56 // (16+4*10) SKILL Guild EXTENTION +4 (http://guide.ragnarok.co.kr/GuildSystem.asp)
#define MAX_GUILD 16 + (GUILD_EXTENTION_BONUS * 10) // (16+6*10) SKILL Guild EXTENTION +6 (kRO Patch - 9/27/05: The Guild skill [Expand Capacity] which increases number of people allowed in the guild, now adds 6 people capacity per level instead of 4.)
#define MAX_GUILDPOSITION 20 // increased max guild positions to accomodate for all members [Valaris] (removed) [PoW]
#define MAX_GUILDEXPLUSION 32
#define MAX_GUILDALLIANCE 16
#define MAX_GUILDSKILL 15
#define MAX_GUILDCASTLE 24 // increased to include novice castles [Valaris]
#define MAX_GUILDLEVEL 50
#define MAX_LEVEL 1000

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

#define MAX_RANKER 10

#define MAIL_STORE_MAX 30

enum {
	RK_BASE,
	RK_BLACKSMITH = 0,
	RK_ALCHEMIST,
	RK_TAEKWON,
	RK_PK,
	RK_DEATHKNIGHT,
	RK_MAX
};

enum {
	LOG_BASE = 0,
	LOG_ATCOMMAND = 0,
	LOG_TRADE,
	LOG_SCRIPT,
	LOG_VENDING,
	LOG_MAX
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

#ifdef USE_SQL //TXT version is still in dev
// For saving status changes
struct status_change_data {
	unsigned short type; // SC_type
	int tick, val1, val2, val3, val4; // remaining duration.
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
	int char_id;
	int account_id;
	unsigned int partner_id;
	unsigned int father, mother;
	unsigned int child;

	unsigned int base_exp, job_exp;
	int zeny;

	short class;
	short status_point, skill_point;
	int hp, max_hp, sp, max_sp;
	short option, karma, manner;
	short hair, hair_color, clothes_color;
	int party_id, guild_id, pet_id;

	short weapon, shield;
	short head_top, head_mid, head_bottom;

	char name[25]; // 24 + NULL
#if MAX_LEVEL > 255
	unsigned int base_level, job_level;
#else
	unsigned char base_level, job_level;
#endif
	short str, agi, vit, int_, dex, luk;
	unsigned char_num : 5; // 0-9 (bits: 4->5)
	unsigned sex : 3; // 0-2 (bits: 2->3)

	int die_counter;

	struct point last_point, save_point, memo_point[MAX_PORTAL_MEMO];
	struct item inventory[MAX_INVENTORY], cart[MAX_CART];
	struct skill skill[MAX_SKILL];
	
	unsigned int fame_point[RK_MAX];
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

struct Ranking_Data {
	char name[25];
	int  point;
	int  char_id;
};

enum {
	GBI_EXP        = 1, // M⬹haREXP
	GBI_GUILDLV    = 2, // M⬹haRLv
	GBI_SKILLPOINT = 3, // M⬹haRXL⬹|CSg
	GBI_SKILLLV    = 4, // M⬹hXL⬹Lv

	GMI_POSITION   = 0, // �So�[aRð�E⬢Ï�X
	GMI_EXP        = 1  // �So�[aREXP
};

struct mail {
	int account_id;
	int char_id;
	int rates;
	int store;
};

enum {
	GD_SKILLBASE        = 10000,
	GD_APPROVAL         = 10000,
	GD_KAFRACONTRACT    = 10001,
	GD_GUARDIANRESEARCH = 10002,
//	GD_CHARISMA         = 10003,
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

struct mail_data {
	int mail_num;
	int char_id;
	char char_name[24];
	int receive_id;
	char receive_name[24];
	int read;
	unsigned int times;
	char title[40];
	char body[35*14];
	unsigned int body_size;
	int zeny;
	struct item item;
};

#endif // _MMO_H_
