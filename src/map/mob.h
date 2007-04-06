// $Id: mob.h 595 2006-05-12 10:05:56Z zugule $
#ifndef _MOB_H_
#define _MOB_H_

#define MAX_RANDOMMONSTER 3
#define MAX_MOB_RACE_DB 6
#define MAX_MOB_DROP 10

struct mob_skill {
	short state;
	short skill_id,skill_lv;
	short permillage;
	int casttime,delay;
	short cancel;
	short cond1, cond2;
	short target;
	int val[5];
	short emotion;
};

struct mob_db {
	char name[25], jname[25]; // 24 + NULL
	int lv;
	int max_hp, max_sp;
	int base_exp, job_exp;
	int atk1, atk2;
	int def, mdef;
	int str, agi, vit, int_, dex, luk;
	int range, range2, range3;
	int size, race, element, mode;
	short race2;	// celest
	int speed, adelay, amotion, dmotion;
	int mexp, mexpper;
	struct { int nameid; short p; } dropitem[MAX_MOB_DROP]; // p: 0-10000(100%)
	struct { int nameid; short p; } mvpitem[3]; // p: 0-10000(100%)
	int view_class, sex;
	short hair, hair_color, weapon, shield, head_top, head_mid, head_buttom, option, clothes_color; // [Valaris]
	int equip; // [Valaris]
	int summonper[MAX_RANDOMMONSTER];
	int maxskill;
	struct mob_skill skill[MAX_MOBSKILL];
};
extern struct mob_db mob_db[];

enum {
	MST_TARGET =	0,
	MST_SELF,
	MST_FRIEND,
	MST_MASTER,
	MST_AROUND5,
	MST_AROUND6,
	MST_AROUND7,
	MST_AROUND8,
	MST_AROUND1,
	MST_AROUND2,
	MST_AROUND3,
	MST_AROUND4,
	MST_AROUND			=	MST_AROUND4,

	MSC_ALWAYS             = 0x0000,
	MSC_MYHPLTMAXRATE,
	MSC_MYHPINRATE,
	MSC_FRIENDHPLTMAXRATE,
	MSC_FRIENDHPINRATE,
	MSC_MYSTATUSON,
	MSC_MYSTATUSOFF,
	MSC_FRIENDSTATUSON,
	MSC_FRIENDSTATUSOFF,
	MSC_ATTACKPCGT,
	MSC_ATTACKPCGE,
	MSC_SLAVELT,
	MSC_SLAVELE,
	MSC_CLOSEDATTACKED,
	MSC_LONGRANGEATTACKED,
	MSC_AFTERSKILL,
	MSC_SKILLUSED,
	MSC_CASTTARGETED,
	MSC_RUDEATTACKED,
	MSC_MASTERHPLTMAXRATE,
	MSC_MASTERATTACKED,
	MSC_ALCHEMIST,
	MSC_SPAWN,
};

//Mob skill states.
enum {
	MSS_IDLE,
	MSS_WALK,
	MSS_LOOT,
	MSS_DEAD,
	MSS_BERSERK, //Aggressive mob attacking
	MSS_ANGRY,   //Mob retaliating from being attacked.
	MSS_RUSH,    //Mob following a player after being attacked.
	MSS_FOLLOW,  //Mob following a player without being attacked.
};

int mobdb_searchname(const char *str);
int mobdb_checkid(const int id);
int mob_once_spawn(struct map_session_data *sd,char *mapname,
	int x,int y,const char *mobname,int class,int amount,const char *event);
int mob_once_spawn_area(struct map_session_data *sd,char *mapname,
	int x0,int y_0,int x1,int y_1,
	const char *mobname,int class,int amount,const char *event);

int mob_spawn_guardian(struct map_session_data *sd, char *mapname,	// Spawning Guardians [Valaris]
	int x, int y, const char *mobname, int class, int amount, const char *event, int guardian);	// Spawning Guardians [Valaris]


int mob_walktoxy(struct mob_data *md, int x, int y, int easy);
//int mob_randomwalk(struct mob_data *md, unsigned int tick);
//int mob_can_move(struct mob_data *md);

int mob_target(struct mob_data *md, struct block_list *bl, int dist);
int mob_stop_walking(struct mob_data *md, int type);
void mob_stopattack(struct mob_data *);
int mob_spawn(int);
int mob_damage(struct block_list *, struct mob_data*, int, int);
void mob_changestate(struct mob_data *md, int state, int type);
int mob_heal(struct mob_data*, int);
int mob_exclusion_add(struct mob_data *md, int type, int id);
int mob_exclusion_check(struct mob_data *md, struct map_session_data *sd);
int mob_get_viewclass(int);
int mob_get_sex(int);
short mob_get_hair(int);
short mob_get_hair_color(int);
short mob_get_weapon(int);
short mob_get_shield(int);
short mob_get_head_top(int);
short mob_get_head_mid(int);
short mob_get_head_buttom(int);
short mob_get_clothes_color(int);	//player mob dye [Valaris]
int mob_get_equip(int); // mob equip [Valaris]
int do_init_mob(void);

int mob_delete(struct mob_data *md);
void mob_catch_delete(struct mob_data *md, int type);
TIMER_FUNC(mob_timer_delete);

int mob_deleteslave(struct mob_data *md);

int mob_class_change(struct mob_data *md, int *value, int valuesize);
int mob_warp(struct mob_data *md, int m, int x, int y, int type);

int mobskill_use(struct mob_data *md, unsigned int tick, int event);
int mobskill_event(struct mob_data *md, int flag);
TIMER_FUNC(mobskill_castend_id);
TIMER_FUNC(mobskill_castend_pos);
int mob_summonslave(struct mob_data *md2, int *value, int amount, int flag);
int mob_warpslave(struct mob_data *md,int x, int y);

int mob_gvmobcheck(struct map_session_data *sd, struct block_list *bl);
void mob_reload(void);

#endif // _MOB_H_
