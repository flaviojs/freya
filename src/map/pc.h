// $Id: pc.h 558 2005-11-28 16:55:15Z Yor $

#ifndef _PC_H_
#define _PC_H_

#include "map.h"

#define OPTION_MASK 0xd7b8
#define CART_MASK 0x788

#define MAX_SKILL_TREE 51 // supernovice have 51 skills.

// dead_sit -> 0: standup, 1: dead, 2: sit
// previously_sit_hp -> 0: not sit when is was previously HP regen, 1: sit... (to avoid people that sit down and stand up between 2 timers)
// previously_sit_sp -> 0: not sit when is was previously SP regen, 1: sit... (to avoid people that sit down and stand up between 2 timers)
#define pc_setdead(sd) ((sd)->state.dead_sit = 1, (sd)->state.previously_sit_hp = 0, (sd)->state.previously_sit_sp = 0)
#define pc_setsit(sd) ((sd)->state.dead_sit = 2) // 0: standup, 1: dead, 2: sit
//#define pc_setstand(sd) ((sd)->state.dead_sit = 0, (sd)->state.previously_sit_hp = 0, (sd)->state.previously_sit_sp = 0)
#define pc_isdead(sd) ((sd)->state.dead_sit == 1) // 0: standup, 1: dead, 2: sit
#define pc_issit(sd) ((sd)->state.dead_sit == 2) // 0: standup, 1: dead, 2: sit

//#define pc_setdir(sd,b,h) ((sd)->dir = (b), (sd)->head_dir = (h))
#define pc_setchatid(sd,n) ((sd)->chatID = n)
#define pc_ishiding(sd) ((sd)->status.option & 0x4006)
#define pc_iscloaking(sd) (!((sd)->status.option & 0x4000) && ((sd)->status.option & 0x0004))
#define pc_ischasewalk(sd) ((sd)->status.option & 0x4000)
#define pc_iscarton(sd) ((sd)->status.option & CART_MASK)
#define pc_isfalcon(sd) ((sd)->status.option & 0x0010)
#define pc_isriding(sd) ((sd)->status.option & 0x0020)
#define pc_isinvisible(sd) ((sd)->status.option & 0x0040)
#define pc_is50overweight(sd) (sd->weight * 2 >= sd->max_weight)
#define pc_is90overweight(sd) (sd->weight * 10 >= sd->max_weight * 9)

int pc_iskiller(struct map_session_data *src, struct map_session_data *target); // [MouseJstr]
int pc_getrefinebonus(int lv,int type);

int pc_setrestartvalue(struct map_session_data *sd, int type);
int pc_makesavestatus(struct map_session_data *);
void pc_setnewpc(struct map_session_data*, int, int, int, int, unsigned char, int);
void pc_authok(int, int, struct mmo_charstatus *);
void pc_authok_final_step(int, time_t);
int pc_authfail(int);
int pc_doubble_connection(int id);

int pc_isequip(struct map_session_data *sd, int n);
int pc_equippoint(struct map_session_data *sd, int n);

int pc_break_equip(struct map_session_data *sd, unsigned short where);
#define pc_breakweapon(sd) (pc_break_equip(sd, EQP_WEAPON))
#define pc_breakarmor(sd) (pc_break_equip(sd, EQP_ARMOR))
#define pc_breakshield(sd) (pc_break_equip(sd, EQP_SHIELD))
#define pc_breakhelm(sd) (pc_break_equip(sd, EQP_HELM))

int pc_checkskill(struct map_session_data *sd, int skill_id);
int pc_checkallowskill(struct map_session_data *sd);
int pc_checkequip(struct map_session_data *sd, int pos);

int pc_calc_skilltree(struct map_session_data *sd);
int pc_calc_skilltree_normalize_job(int c, int s, struct map_session_data *sd);

int pc_checkoverhp(struct map_session_data*);
int pc_checkoversp(struct map_session_data*);

int pc_can_reach(struct map_session_data*, int, int);
void pc_walktoxy(struct map_session_data*, short, short);
int pc_stop_walking(struct map_session_data*, int);
int pc_movepos(struct map_session_data*, int, int);
int pc_setpos(struct map_session_data*, char*, int, int, int);
int pc_setsavepoint(struct map_session_data*, char*, int, int);
void pc_randomwarp(struct map_session_data *sd);
void pc_memo(struct map_session_data *sd); // , int i); // i always -1
void pc_randomwalk(struct map_session_data*);

int pc_checkadditem(struct map_session_data*, int, int);
int pc_inventoryblank(struct map_session_data*);
int pc_search_inventory(struct map_session_data *sd, int item_id);
int pc_payzeny(struct map_session_data*, int);
int pc_additem(struct map_session_data*, struct item*, int);
int pc_getzeny(struct map_session_data*, int);
void pc_delitem(struct map_session_data*, int, int, int);
int pc_checkitem(struct map_session_data*);

int pc_cart_additem(struct map_session_data *sd, struct item *item_data, int amount);
void pc_cart_delitem(struct map_session_data *sd, int n, int amount, int type);
void pc_putitemtocart(struct map_session_data *sd, short idx, int amount);
void pc_getitemfromcart(struct map_session_data *sd, short idx, int amount);
int pc_cartitem_amount(struct map_session_data *sd, int idx, int amount);

void pc_takeitem(struct map_session_data*, struct flooritem_data*);
void pc_dropitem(struct map_session_data*, int, int);

int pc_checkweighticon(struct map_session_data *sd);

int pc_bonus(struct map_session_data*, int, int);
int pc_bonus2(struct map_session_data *sd, int, int, int);
int pc_bonus3(struct map_session_data *sd, int, int, int, int);
int pc_bonus4(struct map_session_data *sd, int, int, int, int, int);
int pc_skill(struct map_session_data*, int, int, int);

int pc_blockskill_start(struct map_session_data*, int, int);	// [celest]

void pc_insert_card(struct map_session_data *sd, short idx_card, short idx_equip);

void pc_item_identify(struct map_session_data *sd, short idx);
void pc_item_repair(struct map_session_data *sd, short idx); // [Celest]
void pc_item_refine(struct map_session_data *sd, short idx); // [Celest]
int pc_steal_item(struct map_session_data *sd, struct block_list *bl);
int pc_steal_coin(struct map_session_data *sd, struct block_list *bl);

int pc_modifybuyvalue(struct map_session_data*, int);
int pc_modifysellvalue(struct map_session_data*, int);

int pc_attack(struct map_session_data*, int, int);
void pc_stopattack(struct map_session_data*);

int pc_follow_timer(int tid, unsigned int tick, int id, int data);
int pc_follow(struct map_session_data*, int); // [MouseJstr]

int pc_checkbaselevelup(struct map_session_data *sd);
int pc_checkjoblevelup(struct map_session_data *sd);
int pc_gainexp(struct map_session_data*, int, int);
int pc_nextbaseexp(struct map_session_data *);
int pc_nextbaseafter(struct map_session_data *); // [Valaris]
int pc_nextjobexp(struct map_session_data *);
int pc_nextjobafter(struct map_session_data *); // [Valaris]
int pc_need_status_point(struct map_session_data *, int);
void pc_statusup(struct map_session_data*, int);
void pc_statusup2(struct map_session_data*, int, int);
void pc_skillup(struct map_session_data*, short);
int pc_allskillup(struct map_session_data*);
void pc_resetlvl(struct map_session_data*, int type);
int pc_resetstate(struct map_session_data*);
int pc_resetskill(struct map_session_data*);
void pc_equipitem(struct map_session_data*, int, int);
void pc_unequipitem(struct map_session_data*, int, int);
int pc_checkitem(struct map_session_data*);
void pc_useitem(struct map_session_data*, short);

int pc_damage(struct block_list *,struct map_session_data*,int);
int pc_heal(struct map_session_data *, int, int);
int pc_itemheal(struct map_session_data *sd, int hp, int sp);
int pc_percentheal(struct map_session_data *sd, int, int);
int pc_jobchange(struct map_session_data *, int, int);
void pc_setoption(struct map_session_data *, short);
void pc_setcart(struct map_session_data *sd, unsigned short type);
int pc_setfalcon(struct map_session_data *sd);
int pc_setriding(struct map_session_data *sd);
int pc_changelook(struct map_session_data *, int, int);
int pc_equiplookall(struct map_session_data *sd);

int pc_readparam(struct map_session_data*, int);
int pc_setparam(struct map_session_data*, int, int);
int pc_readreg(struct map_session_data*, int);
int pc_setreg(struct map_session_data*, int, int);
char *pc_readregstr(struct map_session_data *sd, int reg);
int pc_setregstr(struct map_session_data *sd, int reg, char *str);
int pc_readglobalreg(struct map_session_data*, char*);
void pc_setglobalreg(struct map_session_data*, char*, int);
int pc_readaccountreg(struct map_session_data*, char*);
void pc_setaccountreg(struct map_session_data*, char*, int);
int pc_readaccountreg2(struct map_session_data*, char*);
void pc_setaccountreg2(struct map_session_data*, char*, int);

int pc_addeventtimer(struct map_session_data *sd, int tick, const char *name);
int pc_deleventtimer(struct map_session_data *sd, const char *name);
int pc_cleareventtimer(struct map_session_data *sd);
int pc_addeventtimercount(struct map_session_data *sd, const char *name, int tick);

int pc_calc_pvprank(struct map_session_data *sd);
int pc_calc_pvprank_timer(int tid, unsigned int tick, int id, int data);

int pc_ismarried(struct map_session_data *sd);
int pc_marriage(struct map_session_data *sd, struct map_session_data *dstsd);
int pc_divorce(struct map_session_data *sd);
struct map_session_data *pc_get_partner(struct map_session_data *sd);
int pc_set_gm_level(int account_id, int level);
void pc_setstand(struct map_session_data *sd);
//int pc_break_equip(struct map_session_data *sd, unsigned short where);
int pc_candrop(struct map_session_data *sd, int item_id);

struct pc_base_job{
	int job; //E‹ÆA‚½‚¾‚µ“]¶E‚â—{ŽqE‚Ìê‡‚ÍŒ³‚ÌE‹Æ‚ð•Ô‚·(”pƒvƒŠ¨ƒvƒŠ)
	int type; //ƒmƒr 0, ˆêŽŸE 1, “ñŽŸE 2, ƒXƒpƒmƒr 3
	int upper; //’Êí 0, “]¶ 1, —{Žq 2
};

struct pc_base_job pc_calc_base_job(int b_class);//“]¶‚â—{ŽqE‚ÌŒ³‚ÌE‹Æ‚ð•Ô‚·

int pc_calc_base_job2(int b_class); // Celest
int pc_calc_upper(int b_class);

struct skill_tree_entry {
	short id;
	unsigned char max;
	unsigned char joblv;
	struct {
		unsigned id : 11; // max = 407(499) -> 9 bits (11 for security)
		unsigned lv : 5; // max = 10 -> 4 bits (5 for security) (11 + 5 = 16, 2 bytes)
	} need[5]; // 5 are used
} skill_tree[3][25][MAX_SKILL_TREE]; // from freya's forum (thanks to Celest) [Yor]

void pc_guardiansave(void);
int pc_read_gm_account(int fd);
int pc_setinvincibletimer(struct map_session_data *sd, int);
void pc_delinvincibletimer(struct map_session_data *sd);
int pc_invincible_timer(int tid, unsigned int tick, int id, int data);

int pc_jail_timer(int tid, unsigned int tick, int id, int data);

int pc_addspiritball(struct map_session_data *sd, int, int);
int pc_delspiritball(struct map_session_data *sd, int, int);

int pc_eventtimer(int tid, unsigned int tick, int id, int data); // for npc_dequeue

int do_init_pc(void);

enum {ADDITEM_EXIST, ADDITEM_NEW, ADDITEM_OVERAMOUNT};

// timer for night.day
int day_timer_tid;
int night_timer_tid;
int map_day_timer(int, unsigned int, int, int); // by [yor]
int map_night_timer(int, unsigned int, int, int); // by [yor]

#endif // _PC_H_

