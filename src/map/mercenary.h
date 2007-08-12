#ifndef _MERCENARY_H_
#define _MERCENARY_H_

#define NATURAL_HEAL_HP_INTERVAL 2*1000
#define NATURAL_HEAL_SP_INTERVAL 4*1000

struct mercenary_db {
	int db_id;
	char name[24],jname[24];
	int view_class;
	int max_hp,max_sp;
	int base_level;
	int str,agi,vit,int_,dex,luk;
	int aspd,size,race,element,range;
	int atk_min,atk_max,matk_min,matk_max,def,mdef;
	int job_id;
	struct
	{
		int id;
		int lv;
		int target;
		int rate;
		int state;
		int cond;
		int param1,param2,param3;
	}	skill[MERCENARY_MAX_SKILL_NUM];
	int skill_num;
	struct script_code *script;
};
extern struct mercenary_db	mercenary_db[MERCENARY_MAX_DB];

int mercenary_calc_movetopos(struct mercenary_data *mcd,int tx,int ty,int dir);
int mercenary_call(struct map_session_data *sd,int mec_id);
int mercenary_free(struct mercenary_data *mcd);
int mercenary_menu(struct map_session_data *sd,int menunum);
int mercenary_status_calc(struct mercenary_data *mcd);
int mercenary_return_master(struct mercenary_data *mcd);
int mercenary_gainexp(struct mercenary_data *mcd,struct mob_data *md,int base_exp,int job_exp);
int mercenary_damage(struct block_list *src,struct mercenary_data *mcd,int damage);
int mercenary_heal(struct mercenary_data *mcd,int hp,int sp);
int mercenary_natural_heal_timer_delete(struct mercenary_data *mcd);
int mercenary_lock_target(struct mercenary_data *mcd,struct block_list *target);
int mercenary_free_timer_delete(struct mercenary_data *mcd);
int mercenary_inc_killcount(struct block_list *src);
int mercenary_get_killcount(struct block_list *src);

int mercenary_readdb(void);
int mercenary_read_skill_db(void);

int do_init_mercenary(void);
int do_final_mercenary(void);

#endif
