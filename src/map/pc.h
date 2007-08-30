// Copyright (c) Freya Development Team - Licensed under GNU GPL
// For more information, see License.txt in the main folder

#ifndef _PC_H_
#define _PC_H_

#include "map.h"

#define OPTION_MASK 0xd7b8
#define CART_MASK 0x788

#define MAX_SKILL_PER_TREE 51 // Super Novice has 51 skills

#define MAX_SKILL_TREE 34

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

int pc_isGM(struct map_session_data *sd);
int pc_iskiller(struct map_session_data *src, struct map_session_data *target);

void pc_setrestartvalue(struct map_session_data *sd, int type);
void pc_makesavestatus(struct map_session_data *);
void pc_setnewpc(struct map_session_data*, int, int, int, int, unsigned char, int);
void pc_authok(int, int, struct mmo_charstatus *);
void pc_authok_final_step(int, time_t);
int pc_authfail(int);
int pc_doubble_connection(int id);

int pc_isequip(struct map_session_data *sd, int n);
int pc_equippoint(struct map_session_data *sd, int n);

int pc_break_equip(struct map_session_data *sd, unsigned int where);
#define pc_breakweapon(sd) (pc_break_equip(sd, EQP_WEAPON))
#define pc_breakarmor(sd) (pc_break_equip(sd, EQP_ARMOR))
#define pc_breakshield(sd) (pc_break_equip(sd, EQP_SHIELD))
#define pc_breakhelm(sd) (pc_break_equip(sd, EQP_HELM))

int pc_checkskill(struct map_session_data *sd, int skill_id);
void pc_checkallowskill(struct map_session_data *sd);
int pc_checkequip(struct map_session_data *sd, int pos);

void pc_calc_skilltree(struct map_session_data *sd);
int pc_calc_skilltree_normalize_job(int c, int s, struct map_session_data *sd);

int pc_checkoverhp(struct map_session_data*);
int pc_checkoversp(struct map_session_data*);

int pc_can_reach(struct map_session_data*, int, int);
void pc_walktoxy(struct map_session_data*, short, short);
void pc_stop_walking(struct map_session_data*, int);
int pc_movepos(struct map_session_data*, int, int, int);
int pc_setpos(struct map_session_data*, char*, int, int, int, unsigned int);
int pc_setsavepoint(struct map_session_data*, char*, int, int);
void pc_randomwarp(struct map_session_data *sd);
void pc_memo(struct map_session_data *sd);
int pc_randomxy(struct map_session_data*);

int pc_checkadditem(struct map_session_data*, int, int);
int pc_inventoryblank(struct map_session_data*);
int pc_search_inventory(struct map_session_data *sd, int item_id);
int pc_payzeny(struct map_session_data*, int);
int pc_additem(struct map_session_data*, struct item*, int);
void pc_getzeny(struct map_session_data*, int);
void pc_delitem(struct map_session_data*, int, int, int);
int pc_checkitem(struct map_session_data*);

int pc_cart_additem(struct map_session_data *sd, struct item *item_data, int amount);
void pc_cart_delitem(struct map_session_data *sd, int n, int amount, int type);
void pc_putitemtocart(struct map_session_data *sd, short idx, int amount);
void pc_getitemfromcart(struct map_session_data *sd, short idx, int amount);
int pc_cartitem_amount(struct map_session_data *sd, int idx, int amount);

void pc_takeitem(struct map_session_data*, struct flooritem_data*);
void pc_dropitem(struct map_session_data*, int, int);

void pc_checkweighticon(struct map_session_data *sd);

void pc_bonus(struct map_session_data*, int, int);
void pc_bonus2(struct map_session_data *sd, int, int, int);
void pc_bonus3(struct map_session_data *sd, int, int, int, int);
void pc_bonus4(struct map_session_data *sd, int, int, int, int, int);
int pc_skill(struct map_session_data*, int, int, int);

int pc_blockskill_start(struct map_session_data*, int, int);

void pc_insert_card(struct map_session_data *sd, short idx_card, short idx_equip);

void pc_item_identify(struct map_session_data *sd, short idx);
int pc_item_repair(struct map_session_data *sd, short idx);
void pc_item_refine(struct map_session_data *sd, short idx);
int pc_steal_item(struct map_session_data *sd, struct block_list *bl);
int pc_steal_coin(struct map_session_data *sd, struct block_list *bl);

int pc_modifybuyvalue(struct map_session_data*, int);
int pc_modifysellvalue(struct map_session_data*, int);

int pc_attack(struct map_session_data*, int, int);
void pc_stopattack(struct map_session_data*);

int pc_follow_timer(int tid, unsigned int tick, int id, int data);
int pc_follow(struct map_session_data*, int);

int pc_checkbaselevelup(struct map_session_data *sd);
int pc_checkjoblevelup(struct map_session_data *sd);
void pc_gainexp(struct map_session_data*, unsigned int, unsigned int);
unsigned int pc_nextbaseexp(struct map_session_data *);
unsigned int pc_nextbaseafter(struct map_session_data *);
unsigned int pc_nextjobexp(struct map_session_data *);
unsigned int pc_nextjobafter(struct map_session_data *);
int pc_need_status_point(struct map_session_data *, int);
void pc_statusup(struct map_session_data*, int);
void pc_statusup2(struct map_session_data*, int, int);
void pc_skillup(struct map_session_data*, short);
void pc_allskillup(struct map_session_data*);
void pc_resetlvl(struct map_session_data*, int type);
void pc_resetstate(struct map_session_data*);
void pc_resetskill(struct map_session_data*);
void pc_equipitem(struct map_session_data*, int, int);
void pc_unequipitem(struct map_session_data*, int, int);
int pc_checkitem(struct map_session_data*);
void pc_useitem(struct map_session_data*, short);

int pc_damage(struct block_list *,struct map_session_data*,int);
int pc_heal(struct map_session_data *, int, int);
void pc_itemheal(struct map_session_data *sd, int hp, int sp);
void pc_percentheal(struct map_session_data *sd, int, int);
int pc_jobchange(struct map_session_data *, int, int);
void pc_setoption(struct map_session_data *, short);
void pc_setcart(struct map_session_data *sd, unsigned short type);
void pc_setfalcon(struct map_session_data *sd);
void pc_setriding(struct map_session_data *sd);
void pc_changelook(struct map_session_data *, int, int);
void pc_equiplookall(struct map_session_data *sd);

int pc_readparam(struct map_session_data*, int);
void pc_setparam(struct map_session_data*, int, int);
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

void pc_addeventtimer(struct map_session_data *sd, int tick, const char *name);
void pc_deleventtimer(struct map_session_data *sd, const char *name);
void pc_cleareventtimer(struct map_session_data *sd);
void pc_addeventtimercount(struct map_session_data *sd, const char *name, int tick);

int pc_calc_pvprank(struct map_session_data *sd);
int pc_calc_pvprank_timer(int tid, unsigned int tick, int id, int data);

int pc_ismarried(struct map_session_data *sd);
int pc_marriage(struct map_session_data *sd, struct map_session_data *dstsd);
int pc_divorce(struct map_session_data *sd);
void pc_adoption(struct map_session_data *sd, struct map_session_data *tsd, char type);

struct map_session_data *pc_get_partner(struct map_session_data *sd);
struct map_session_data *pc_get_father(struct map_session_data *sd);
struct map_session_data *pc_get_mother(struct map_session_data *sd);
struct map_session_data *pc_get_child(struct map_session_data *sd);

void pc_set_gm_level(int account_id, unsigned char level);
void pc_set_gm_level_by_gm(int account_id, signed char level, int account_id_of_gm);
void pc_setstand(struct map_session_data *sd);
int pc_candrop(struct map_session_data *sd, int item_id);

struct pc_base_job{
	int job;
	int type;
	int upper;
};

struct pc_base_job pc_calc_base_job(unsigned int b_class);

int pc_calc_base_job2(unsigned int b_class);
int pc_calc_upper(unsigned int b_class);
short pc_get_upper_type(unsigned int class);

struct skill_tree_entry {
	short id;
	unsigned char max;
	unsigned char joblv;
	struct {
		unsigned id : 11; // max = 407(499) -> 9 bits (11 for security)
		unsigned lv : 5; // max = 10 -> 4 bits (5 for security) (11 + 5 = 16, 2 bytes)
	} need[5]; // 5 are used
} skill_tree[3][MAX_SKILL_TREE][MAX_SKILL_PER_TREE];

void pc_guardiansave(void);
int pc_read_gm_account(int fd);
void pc_setinvincibletimer(struct map_session_data *sd, int);
void pc_delinvincibletimer(struct map_session_data *sd);
int pc_invincible_timer(int tid, unsigned int tick, int id, int data);

int pc_jail_timer(int tid, unsigned int tick, int id, int data);

void pc_addspiritball(struct map_session_data *sd, int, int);
void pc_delspiritball(struct map_session_data *sd, int, int);

int pc_eventtimer(int tid, unsigned int tick, int id, int data); // for npc_dequeue

int pc_run(struct map_session_data *sd, int skilllv, int dir);

int do_final_pc(void);
int do_init_pc(void);

enum {ADDITEM_EXIST, ADDITEM_NEW, ADDITEM_OVERAMOUNT};

// Timer for night.day
int day_timer_tid;
int night_timer_tid;
int map_day_timer(int, unsigned int, int, int);
int map_night_timer(int, unsigned int, int, int);

#endif // _PC_H_

