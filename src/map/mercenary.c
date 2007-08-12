#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "db.h"
#include "timer.h"
#include "socket.h"
#include "nullpo.h"
#include "malloc.h"
#include "pc.h"
#include "map.h"
#include "intif.h"
#include "clif.h"
#include "chrif.h"
#include "mercenary.h"
#include "itemdb.h"
#include "battle.h"
#include "mob.h"
#include "npc.h"
#include "script.h"
#include "status.h"
#include "unit.h"
#include "skill.h"

#ifdef MEMWATCH
#include "memwatch.h"
#endif

#define USE_SERVER_SIDE_AI
#define MIN_MERCENARY_THINKTIME 100
#define MERCENARY_CLASS 1
#define MERCENARY_SKILL_USE_RATE	35

struct mercenary_db	mercenary_db[MERCENARY_MAX_DB];

static int dirx[8]={ 0,-1,-1,-1, 0, 1, 1, 1};
static int diry[8]={ 1, 1, 0,-1,-1,-1, 0, 1};

enum{
	MERCENARY_TARGET_ENEMY = 0x01,
	MERCENARY_TARGET_SELF,
	MERCENARY_TARGET_MASTER,
};

enum{
	MERCENARY_STATE_IDLE = 0x01,
	MERCENARY_STATE_WALKING,
	MERCENARY_STATE_BATTLE,
	MERCENARY_STATE_ALWAYS,
};

enum{
	MERCENARY_COND_HP = 0x01,
	MERCENARY_COND_SP,
	MERCENARY_COND_STATUS_CHANGE,
};

int mercenary_calc_movetopos(struct mercenary_data *mcd,int tx,int ty,int dir)
{
	int x=0,y=0,dx=0,dy=0;
	int i,ret=0;
	nullpo_retr(0, mcd);

	mcd->ud.to_x = tx;
	mcd->ud.to_y = ty;

	if(dir < 0 || dir >= 8)	return 1;

	for(i = 0; i < 24; i++)
	{
		if(i % 2 == 0)	//偶数回目か
		{
			int k = (i == 0) ? dir : atn_rand() % 8;
			dx = -dirx[k]*2;
			dy = -diry[k]*2;
			x = dx + tx;
			y = dy + ty;
		}else{
			if(dx > 0) x--;
			else if(dx < 0) x++;
			if(dy > 0) y--;
			else if(dy < 0) y++;
		}
		ret = unit_can_reach(&mcd->bl,x,y);
		if(ret)	break;
	}
	if(ret == 0) {
		x = tx;
		y = ty;
		if(!unit_can_reach(&mcd->bl,x,y))	return 1;
	}
	mcd->ud.to_x = x;
	mcd->ud.to_y = y;
	return 0;
}

static int mercenary_search_db(int type,int key)
{
	int i;

	for(i=0;i<MERCENARY_MAX_DB;i++) {
		if(mercenary_db[i].db_id <= 0)
			continue;
		switch(type) {
			case MERCENARY_CLASS:
				if(mercenary_db[i].db_id == key)
					return i;
				break;
			default:
				return -1;
		}
	}
	return -1;
}

static int mercenary_natural_heal_hp(int tid,unsigned int tick,int id,int data)
{
	struct mercenary_data *mcd = map_id2mcd(id);
	int bhp;

	nullpo_retr(0, mcd);

	if(mcd->natural_heal_hp != tid){
		if(battle_config.error_log)
			printf("mercenary_natural_heal_hp %d != %d\n",mcd->natural_heal_hp,tid);
		return 0;
	}
	mcd->natural_heal_hp = -1;

	bhp=mcd->hp;

	if(mcd->ud.walktimer == -1) {
		mcd->hp += mcd->nhealhp;
		if(mcd->hp > mcd->max_hp)
			mcd->hp = mcd->max_hp;
		if(bhp != mcd->hp && mcd->msd)
			clif_send_mecstatus(mcd->msd,0);
	}
	mcd->natural_heal_hp = add_timer(tick+battle_config.natural_healhp_interval,mercenary_natural_heal_hp,mcd->bl.id,0);

	return 0;
}

static int mercenary_natural_heal_sp(int tid,unsigned int tick,int id,int data)
{
	struct mercenary_data *mcd = map_id2mcd(id);
	int bsp;

	nullpo_retr(0, mcd);

	if(mcd->natural_heal_sp != tid){
		if(battle_config.error_log)
			printf("mercenary_natural_heal_sp %d != %d\n",mcd->natural_heal_sp,tid);
		return 0;
	}
	mcd->natural_heal_sp = -1;

	bsp = mcd->sp;

	if(mcd->ud.walktimer == -1){
		mcd->sp += mcd->nhealsp;
		if(mcd->sp > mcd->max_sp)
			mcd->sp = mcd->max_sp;
		if(bsp != mcd->sp && mcd->msd)
			clif_send_mecstatus(mcd->msd,0);
	}
	mcd->natural_heal_sp = add_timer(tick+battle_config.natural_healsp_interval,mercenary_natural_heal_sp,mcd->bl.id,0);

	return 0;
}

static int mercenary_free_timer(int tid,unsigned int tick,int id,int data)
{
	struct mercenary_data *mcd = map_id2mcd(id);

	nullpo_retr(0, mcd);

	mercenary_free(mcd);

	return 0;
}

static int mercenary_data_init(struct map_session_data *sd,int mec_id)
{
	int i=0;

	nullpo_retr(1, sd);

	sd->mcd = (struct mercenary_data *)aCalloc(1,sizeof(struct mercenary_data));

	if((sd->mcd->db_idx = mercenary_search_db(MERCENARY_CLASS,mec_id)) == -1){
		aFree(sd->mcd);
		return 1;
	}
	sd->mcd->bl.m = sd->bl.m;
	sd->mcd->bl.prev = sd->mcd->bl.next = NULL;
	sd->mcd->bl.x = sd->bl.x;
	sd->mcd->bl.y = sd->bl.y;
	mercenary_calc_movetopos(sd->mcd,sd->bl.x,sd->bl.y,sd->dir);
	sd->mcd->bl.x = sd->mcd->ud.to_x;
	sd->mcd->bl.y = sd->mcd->ud.to_y;
	sd->mcd->bl.id = npc_get_new_npc_id();

	sd->mcd->dir = sd->dir;
	sd->mcd->speed = status_get_speed(&sd->bl);
	sd->mcd->bl.subtype = MONS;
	sd->mcd->bl.type = BL_MEC;
	sd->mcd->msd = sd;
	sd->mcd->view_class = mercenary_db[sd->mcd->db_idx].view_class;
	sd->mcd->view_size =  mercenary_db[sd->mcd->db_idx].size;
	sd->mcd->base_level =  mercenary_db[sd->mcd->db_idx].base_level;
	sd->mcd->race =  mercenary_db[sd->mcd->db_idx].race;
	sd->mcd->element =  mercenary_db[sd->mcd->db_idx].element;
	sd->mcd->range =  mercenary_db[sd->mcd->db_idx].range;
	sd->mcd->move_fail_count = 0;
	sd->mcd->attaker_id = 0;
	sd->mcd->equip = 0;
	sd->mcd->job_id = mercenary_db[sd->mcd->db_idx].job_id;
	sd->mcd->next_walktime = sd->mcd->last_thinktime = gettick();
	memcpy(sd->mcd->name,mercenary_db[sd->mcd->db_idx].jname,24);

	for(i=0;i<MAX_STATUSCHANGE;i++) {
		sd->mcd->sc_data[i].timer=-1;
		sd->mcd->sc_data[i].val1 = sd->mcd->sc_data[i].val2 = sd->mcd->sc_data[i].val3 = sd->mcd->sc_data[i].val4 = 0;
	}
	sd->mcd->sc_count = sd->mcd->opt1 = sd->mcd->opt2 = sd->mcd->opt3 = sd->mcd->option = 0;
	sd->mcd->option &= OPTION_MASK;

	mercenary_status_calc(sd->mcd);

	sd->mcd->state = MERCENARY_STATE_IDLE;

	unit_dataset(&sd->mcd->bl);
	map_addiddb(&sd->mcd->bl);

	sd->mcd->natural_heal_hp = add_timer(gettick()+battle_config.natural_healhp_interval,mercenary_natural_heal_hp,sd->mcd->bl.id,0);
	sd->mcd->natural_heal_sp = add_timer(gettick()+battle_config.natural_healsp_interval,mercenary_natural_heal_sp,sd->mcd->bl.id,0);
	if(battle_config.mercenary_free_time != -1)
		sd->mcd->mercenary_free_tid = add_timer(gettick()+battle_config.mercenary_free_time,mercenary_free_timer,sd->mcd->bl.id,0);

	return 0;
}

#ifdef USE_SERVER_SIDE_AI
int mercenary_lock_target(struct mercenary_data *mcd,struct block_list *target)
{
	nullpo_retr(0, mcd);

	if(target != NULL || mcd->attaker_id==0 || atn_rand()%100<25)
		mcd->attaker_id = target->id;
	return 0;
}

static void mercenary_chase_target(struct block_list *src,struct block_list *target)
{
	int ret,i=0,dx,dy;
	do {
		if(i==0) {	// 最初はAEGISと同じ方法で検索
			dx=target->x - src->x;
			dy=target->y - src->y;
			if(dx<0) dx++;
			else if(dx>0) dx--;
			if(dy<0) dy++;
			else if(dy>0) dy--;
		}else {	// だめならAthena式(ランダム)
			dx=target->x - src->x + rand()%3 - 1;
			dy=target->y - src->y + rand()%3 - 1;
		}
		ret=unit_walktoxy(src,src->x+dx,src->y+dy);
		i++;
	} while(ret && i<5);

	if(ret) { // 移動不可能な所からの攻撃なら2歩下る
		if(dx<0) dx=2;
		else if(dx>0) dx=-2;
		if(dy<0) dy=2;
		else if(dy>0) dy=-2;
		unit_walktoxy(src,src->x+dx,src->y+dy);
	}
	return;
}

static void mercenary_run_skill_list(struct mercenary_data* mcd)
{
	int i;
	struct block_list *target = NULL;

	for(i = 0; i < mercenary_db[mcd->db_idx].skill_num; i++)
	{
		int cond,param1,param2,param3;
		switch(mercenary_db[mcd->db_idx].skill[i].target)
		{
			case MERCENARY_TARGET_ENEMY:
				target = map_id2bl(mcd->attaker_id);
				break;
			case MERCENARY_TARGET_SELF:
				target = &mcd->bl;
				break;
			case MERCENARY_TARGET_MASTER:
				target = &mcd->msd->bl;
					break;
		}

		if(target == NULL)	continue;

		if(mercenary_db[mcd->db_idx].skill[i].state != MERCENARY_STATE_ALWAYS && mercenary_db[mcd->db_idx].skill[i].state != mcd->state)
			continue;

		cond = mercenary_db[mcd->db_idx].skill[i].cond;
		param1 = mercenary_db[mcd->db_idx].skill[i].param1;
		param2 = mercenary_db[mcd->db_idx].skill[i].param2;
		param3 = mercenary_db[mcd->db_idx].skill[i].param3;
		if(cond == MERCENARY_COND_HP){
			int rate = status_get_hp(target) * 100 / status_get_max_hp(target);
			if(rate > param1)
				continue;
		}else if(cond == MERCENARY_COND_SP){
			int rate = status_get_sp(target) * 100 / status_get_max_sp(target);
			if(rate > param1)
				continue;
		}else if(cond == MERCENARY_COND_STATUS_CHANGE){
			struct status_change *sc_data = status_get_sc_data(target);
			if((param2 == 1 && sc_data[param1].timer != -1) || (param2 == 0 && sc_data[param1].timer == -1))
				continue;
		}

		if(atn_rand() % 100 > mercenary_db[mcd->db_idx].skill[i].rate)
			continue;

		if( skill_get_inf( mercenary_db[mcd->db_idx].skill[i].id ) & 2 )
			unit_skilluse_pos( &mcd->bl, target->x,target->y, mercenary_db[mcd->db_idx].skill[i].id, mercenary_db[mcd->db_idx].skill[i].lv);
		else
			unit_skilluse_id(&mcd->bl, target->id, mercenary_db[mcd->db_idx].skill[i].id, mercenary_db[mcd->db_idx].skill[i].lv);
		return;
	}
	if(i == mercenary_db[mcd->db_idx].skill_num && mcd->attaker_id)
		unit_attack(&mcd->bl,mcd->attaker_id,0);
	return;
}

static int mercenary_ai_main(struct map_session_data *sd,va_list ap)
{
	unsigned int tick;
	struct mercenary_data* mcd;
	int dist;

	nullpo_retr(0, sd);
	nullpo_retr(0, ap);

	tick=va_arg(ap,unsigned int);
	mcd=sd->mcd;

	if(mcd == NULL || mcd->bl.prev == NULL || sd->bl.prev == NULL)
		return 0;

	if(DIFF_TICK(tick,mcd->last_thinktime) < MIN_MERCENARY_THINKTIME)
		return 0;
	if(mcd->ud.attacktimer != -1 || mcd->ud.skilltimer != -1 || mcd->bl.m != sd->bl.m)
		return 0;
	// 歩行中は３歩ごとにAI処理を行う
	if( mcd->ud.walktimer != -1 && mcd->ud.walkpath.path_pos <= 2)
		return 0;
	mcd->last_thinktime=tick;

	dist = unit_distance(sd->bl.x,sd->bl.y,mcd->bl.x,mcd->bl.y);
	if(dist>15){
		if(mcd->attaker_id>0)
			mcd->attaker_id = 0;
		if(mcd->ud.walktimer != -1 && unit_distance(mcd->ud.to_x,mcd->ud.to_y,sd->bl.x,sd->bl.y) < 5)
			return 0;
		mcd->speed = status_get_speed(&sd->bl)>>1;
		if(mcd->speed <= 0)
			mcd->speed = 1;
		mercenary_return_master(sd->mcd);
		mcd->state = MERCENARY_STATE_WALKING;
	}else if(mcd->attaker_id  > MAX_FLOORITEM){
		struct block_list *attaker = map_id2bl(mcd->attaker_id);
		struct status_change *sc_data;
		int range = status_get_range(&mcd->bl);

		if(attaker == NULL){
			mcd->attaker_id=0;
			return 0;
		}
		sc_data = status_get_sc_data(attaker);

		if(mcd->bl.m != attaker->m || attaker->prev == NULL ||
			mcd->ud.skilltimer != -1 || mcd->ud.attacktimer != -1 ||
			sc_data[SC_TRICKDEAD].timer != -1  || sc_data[SC_FORCEWALKING].timer != -1 || sc_data[SC_WINKCHARM].timer != -1
		  ){
			mcd->attaker_id = 0;
			return 0;
		}
		if(mcd->sc_data[SC_BLIND].timer != -1 || mcd->sc_data[SC_FOGWALLPENALTY].timer != -1)
			range = 1;
		if(battle_check_range(&mcd->bl,attaker,range)==0)
			mercenary_chase_target(&mcd->bl,attaker);
		else{
			if(mcd->ud.walktimer != -1)
				unit_stop_walking(&mcd->bl,1);
			if(mcd->ud.attacktimer != -1 || mcd->ud.skilltimer != -1)
				return 0;
			mcd->state = MERCENARY_STATE_BATTLE;
		}
	}else if(dist >= 5){
		if(mcd->ud.walktimer != -1 && unit_distance(mcd->ud.to_x,mcd->ud.to_y,sd->bl.x,sd->bl.y) < 5)
		 	return 0;
		mcd->speed = status_get_speed(&sd->bl);
		mercenary_return_master(mcd);
		mcd->state = MERCENARY_STATE_WALKING;
	}else
			mcd->state = MERCENARY_STATE_IDLE;
	mercenary_run_skill_list(mcd);
	return 0;
}

static int mercenary_call_ai(int tid,unsigned int tick,int id,int data)
{
	clif_foreachclient(mercenary_ai_main,tick);
	return 0;
}
#else
void mercenary_target_lock(struct mercenary_data *mcd,struct block_list *target)
{
	retrun;
}
#endif

/*=================================================
 * min～maxの範囲から適当な値を返す(min < max)
 *-------------------------------------------------
 */
static int mercenary_get_rndvalue(int min,int max)
{
	if(min == max)	return min;
	else return min + atn_rand() % (max-min);
}

int mercenary_call(struct map_session_data *sd,int mec_id)
{
	nullpo_retr(1, sd);
	if(sd->mcd)
		return 1;
	if(mec_id < MERCENARY_ID || mec_id > MERCENARY_ID + MERCENARY_MAX_DB)
		return 1;
	mercenary_data_init(sd,mec_id);
	map_addblock(&sd->mcd->bl);
	mob_ai_hard_spawn(&sd->mcd->bl,1);
	//clif_send_mecdata(sd->mcd,0x0,0x0);
	clif_spawnmec(sd->mcd);
	return 0;
}

int mercenary_free(struct mercenary_data *mcd)
{
	nullpo_retr(0, mcd);

	status_change_end(&mcd->bl,SC_MERCENAY_INCATK,-1);
	status_change_end(&mcd->bl,SC_MERCENAY_INCFLEE,-1);
	status_change_end(&mcd->bl,SC_MERCENAY_INCDEF,-1);
	status_change_end(&mcd->bl,SC_MERCENAY_INCHP,-1);
	status_change_end(&mcd->bl,SC_MERCENAY_INCSP,-1);
	status_change_end(&mcd->bl,SC_MERCENAY_INCHIT,-1);
	unit_free(&mcd->bl,1);
	return 0;
}

int mercenary_status_calc(struct mercenary_data *mcd)
{
	mcd->str =  mercenary_db[mcd->db_idx].str;
	mcd->agi =  mercenary_db[mcd->db_idx].agi;
	mcd->vit =  mercenary_db[mcd->db_idx].vit;
	mcd->int_ =  mercenary_db[mcd->db_idx].int_;
	mcd->dex =  mercenary_db[mcd->db_idx].dex;
	mcd->luk =  mercenary_db[mcd->db_idx].luk;
	mcd->atk = mercenary_get_rndvalue(mercenary_db[mcd->db_idx].atk_min,mercenary_db[mcd->db_idx].atk_max);
	mcd->matk = mercenary_get_rndvalue(mercenary_db[mcd->db_idx].matk_min,mercenary_db[mcd->db_idx].matk_max);
	mcd->nhealhp=0;
	mcd->nhealsp=0;

	if(mcd->sc_count && mcd->sc_data)
	{
		//ゴスペルALL+20
		if(mcd->sc_data[SC_INCALLSTATUS].timer!=-1){
			mcd->str+= mcd->sc_data[SC_INCALLSTATUS].val1;
			mcd->agi+= mcd->sc_data[SC_INCALLSTATUS].val1;
			mcd->vit+= mcd->sc_data[SC_INCALLSTATUS].val1;
			mcd->int_+= mcd->sc_data[SC_INCALLSTATUS].val1;
			mcd->dex+= mcd->sc_data[SC_INCALLSTATUS].val1;
			mcd->luk+= mcd->sc_data[SC_INCALLSTATUS].val1;
		}

		if(mcd->sc_data[SC_INCREASEAGI].timer!=-1)	// 速度増加
			mcd->agi += 2+mcd->sc_data[SC_INCREASEAGI].val1;

		if(mcd->sc_data[SC_DECREASEAGI].timer!=-1)	// 速度減少(agiはbattle.cで)
			mcd->agi-= 2+mcd->sc_data[SC_DECREASEAGI].val1;

		if(mcd->sc_data[SC_BLESSING].timer!=-1){	// ブレッシング
			mcd->str+= mcd->sc_data[SC_BLESSING].val1;
			mcd->dex+= mcd->sc_data[SC_BLESSING].val1;
			mcd->int_+= mcd->sc_data[SC_BLESSING].val1;
		}
		if(mcd->sc_data[SC_SUITON].timer!=-1){	// 水遁
			if(mcd->sc_data[SC_SUITON].val3)
				mcd->agi+=mcd->sc_data[SC_SUITON].val3;
			if(mcd->sc_data[SC_SUITON].val4)
				mcd->speed = mcd->speed*2;
		}

		if(mcd->sc_data[SC_GLORIA].timer!=-1)	// グロリア
			mcd->luk+= 30;
			
		if(mcd->sc_data[SC_QUAGMIRE].timer!=-1){	// クァグマイア
			short subagi = 0;
			short subdex = 0;
			subagi = (mercenary_db[mcd->db_idx].agi/2 < mcd->sc_data[SC_QUAGMIRE].val1*10) ? mercenary_db[mcd->db_idx].agi/2 : mcd->sc_data[SC_QUAGMIRE].val1*10;
			subdex = (mercenary_db[mcd->db_idx].dex/2 < mcd->sc_data[SC_QUAGMIRE].val1*10) ? mercenary_db[mcd->db_idx].dex/2 : mcd->sc_data[SC_QUAGMIRE].val1*10;
			if(map[mcd->bl.m].flag.pvp || map[mcd->bl.m].flag.gvg){
				subagi/= 2;
				subdex/= 2;
			}
			mcd->speed = mcd->speed*4/3;
			mcd->agi-= subagi;
			mcd->dex-= subdex;
		}
	}

	mcd->atk		+= mcd->str * 2 + mcd->base_level + (mcd->str/10) * (mcd->str/10);
	mcd->matk		+= mcd->int_+(mcd->int_/ 5) * (mcd->int_/ 5);
	mcd->def		 = mercenary_db[mcd->db_idx].def;
	mcd->def2		 = mcd->vit;
	mcd->mdef		 = mercenary_db[mcd->db_idx].mdef;
	mcd->mdef2		 = mcd->int_;
	mcd->hit		 = mcd->dex + mcd->base_level;
	mcd->flee		 = mcd->agi + mcd->base_level;
	mcd->critical	 = mcd->luk / 3 + 1;
	mcd->aspd		 = mercenary_db[mcd->db_idx].aspd - (mcd->agi*4+mcd->dex)*mercenary_db[mcd->db_idx].aspd/1000;
	mcd->hp = mcd->max_hp = mercenary_db[mcd->db_idx].max_hp * (100 + mcd->vit)/100;
	mcd->sp = mcd->max_sp = mercenary_db[mcd->db_idx].max_sp * (100 + mcd->int_)/100;

	//ディフェンス
	if(mcd->sc_data[SC_DEFENCE].timer!=-1)
		mcd->def += mcd->sc_data[SC_DEFENCE].val1*2;
	//オーバードスピード
	if(mcd->sc_data[SC_SPEED].timer!=-1)
		mcd->flee = mcd->flee + 10 + mcd->sc_data[SC_SPEED].val1*10;
	//各種ステータス上昇
	if(mcd->sc_data[SC_MERCENAY_INCATK].timer!=-1)
	{
		mcd->atk += mcd->sc_data[SC_MERCENAY_INCATK].val1 * 30;
		mcd->matk += mcd->sc_data[SC_MERCENAY_INCATK].val1 * 30;
	}
	if(mcd->sc_data[SC_MERCENAY_INCFLEE].timer!=-1)
		mcd->flee += mcd->sc_data[SC_MERCENAY_INCFLEE].val1 * 20;
	if(mcd->sc_data[SC_MERCENAY_INCDEF].timer!=-1)
	{
		mcd->def += mcd->sc_data[SC_MERCENAY_INCDEF].val1 * 30;
		mcd->mdef += mcd->sc_data[SC_MERCENAY_INCDEF].val1 * 30;
	}
	if(mcd->sc_data[SC_MERCENAY_INCHP].timer!=-1)
	{
		int rate = 100 + mcd->sc_data[SC_MERCENAY_INCHP].val1 * 20;
		mcd->max_hp = mcd->max_hp * rate / 100;
		mcd->hp = mcd->max_hp;
	}
	if(mcd->sc_data[SC_MERCENAY_INCSP].timer!=-1)
	{
		int rate = 100 + mcd->sc_data[SC_MERCENAY_INCSP].val1 * 20;
		mcd->max_sp = mcd->max_sp * rate / 100;
		mcd->sp = mcd->max_sp;
	}
	if(mcd->sc_data[SC_MERCENAY_INCHIT].timer!=-1)
		mcd->hit += mcd->sc_data[SC_MERCENAY_INCHIT].val1 * 40;
	//メンタルチェンジ
	if(mcd->sc_data && mcd->sc_data[SC_CHANGE].timer!=-1)
	{
		int atk_,hp_;
		//
		atk_= mcd->atk;
		mcd->atk = mcd->matk;
		mcd->matk = atk_;
		//
		hp_= mcd->max_hp;
		mcd->max_hp = mcd->max_sp;
		mcd->max_sp = hp_;
	}
	if(mcd->max_hp<=0) mcd->max_hp=1;	// mhp 0 だとクライアントエラー
	if(mcd->max_sp<=0) mcd->max_sp=1;
	mcd->nhealhp = mcd->max_hp/100 + mcd->vit/5 + 2;
	mcd->nhealsp = (mcd->int_/6)+(mcd->max_sp/100)+1;
	if(mcd->int_ >= 120)
		mcd->nhealsp += ((mcd->int_-120)>>1) + 4;
	return 0;
}

int mercenary_menu(struct map_session_data *sd,int menunum)
{
	nullpo_retr(0, sd);
	if(!sd->mcd) return 0;

	switch(menunum) {
		case 0:
			break;
		case 1:
			break;
		case 2:
			break;
	}
	return 0;
}

int mercenary_return_master(struct mercenary_data *mcd)
{
	nullpo_retr(0, mcd);
	nullpo_retr(0, mcd->msd);
	mercenary_calc_movetopos(mcd,mcd->msd->bl.x,mcd->msd->bl.y,mcd->msd->dir);
	unit_walktoxy(&mcd->bl,mcd->ud.to_x,mcd->ud.to_y);
	return 0;
}

int mercenary_gainexp(struct mercenary_data *mcd,struct mob_data *md,int base_exp,int job_exp)
{
	atn_bignumber bexp=base_exp,jexp=job_exp;
	int mbexp=0,mjexp=0;

	nullpo_retr(0, mcd);
	if(mcd->bl.prev == NULL || mcd->msd == NULL || unit_isdead(&mcd->bl) || unit_isdead(&mcd->msd->bl))
		return 0;

	if(md && md->sc_data && md->sc_data[SC_RICHMANKIM].timer != -1) {
		bexp = bexp*(125 + md->sc_data[SC_RICHMANKIM].val1*11)/100;
		jexp = jexp*(125 + md->sc_data[SC_RICHMANKIM].val1*11)/100;
	}
	base_exp = (bexp>0x7fffffff)? 0x7fffffff: (int)bexp;
	job_exp  = (jexp>0x7fffffff)? 0x7fffffff: (int)jexp;
	mbexp = base_exp;
	mjexp = job_exp;

	if(mcd->msd)	// 主人へ同じだけ経験値
		pc_gainexp(mcd->msd,md,mbexp,mjexp);

	return 0;
}

int mercenary_damage(struct block_list *src,struct mercenary_data *mcd,int damage)
{
	struct map_session_data *sd = NULL;

	nullpo_retr(0, mcd);
	nullpo_retr(0,(sd=mcd->msd));

	// 既に死んでいたら無効
	if(unit_isdead(&mcd->bl))
		return 0;

	// 歩いていたら足を止める
	unit_stop_walking(&mcd->bl,battle_config.pc_hit_stop_type);

	if(damage>0)
		skill_stop_gravitation(&mcd->bl);

	if(mcd->bl.prev==NULL){
		if(battle_config.error_log)
			printf("homun_damage : BlockError!!\n");
		return 0;
	}

	if(mcd->hp > mcd->max_hp)
		mcd->hp = mcd->max_hp;

	// over kill分は丸める
	if(damage>mcd->hp)
		damage=mcd->hp;

	mcd->hp -= damage;

	if (mcd->option & 0x02)
		status_change_end(&mcd->bl, SC_HIDING, -1);
	if ((mcd->option & 0x4004) == 4)
		status_change_end(&mcd->bl, SC_CLOAKING, -1);
	if ((mcd->option & 0x4004) == 0x4004)
		status_change_end(&mcd->bl, SC_CHASEWALK, -1);

	clif_send_mecstatus(sd,0);

	// 死亡していた
	if(mcd->hp<=0){
		mcd->hp = 1;
		skill_unit_move(&mcd->bl,gettick(),0);
		mcd->hp = 0;
		mcd->msd->status.mec_intimate[mcd->job_id]--;
		if(mcd->msd->status.mec_intimate[mcd->job_id] < 0)
			mcd->msd->status.mec_intimate[mcd->job_id] = 0;
		mercenary_free(mcd);
	}else
		mercenary_lock_target(mcd,src);

	return 0;
}

int mercenary_heal(struct mercenary_data *mcd,int hp,int sp)
{
	nullpo_retr(0, mcd);

	// バーサーク中は回復させない
	if(mcd->sc_data && mcd->sc_data[SC_BERSERK].timer!=-1) {
		if (sp > 0)
			sp = 0;
		if (hp > 0)
			hp = 0;
	}

	if(hp+mcd->hp > mcd->max_hp)
		hp = mcd->max_hp - mcd->hp;
	if(sp+mcd->sp > mcd->max_sp)
		sp = mcd->max_sp - mcd->sp;
	mcd->hp+=hp;
	if(mcd->hp <= 0) {
		mcd->hp = 0;
		mercenary_damage(NULL,mcd,1);
		hp = 0;
	}
	mcd->sp+=sp;
	if(mcd->sp <= 0)
		mcd->sp = 0;
	if((hp | sp) && mcd)
		clif_send_mecstatus(mcd->msd,0);

	return hp + sp;
}

int mercenary_natural_heal_timer_delete(struct mercenary_data *mcd)
{
	nullpo_retr(0, mcd);

	if(mcd->natural_heal_hp != -1) {
		delete_timer(mcd->natural_heal_hp,mercenary_natural_heal_hp);
		mcd->natural_heal_hp = -1;
	}
	if(mcd->natural_heal_sp != -1) {
		delete_timer(mcd->natural_heal_sp,mercenary_natural_heal_sp);
		mcd->natural_heal_sp = -1;
	}

	return 0;
}

int mercenary_free_timer_delete(struct mercenary_data *mcd)
{
	nullpo_retr(0, mcd);

	if(mcd->mercenary_free_tid != -1)
		delete_timer(mcd->mercenary_free_tid,mercenary_free_timer);

	return 0;
}

int mercenary_inc_killcount(struct block_list *src)
{
	struct map_session_data	*sd	= NULL;

	if(src == NULL)	return 0;

	sd = BL_DOWNCAST(BL_PC,src);
	if(src->type == BL_MEC)	sd = ((struct mercenary_data*)src)->msd;
	if(sd == NULL || sd->mcd == NULL)	return 0;

	pc_setglobalreg(sd,MERCENARY_REG_KILL_COUNT,pc_readglobalreg(sd,MERCENARY_REG_KILL_COUNT)+1);
	return 0;
}

int mercenary_get_killcount(struct block_list *src)
{
	struct map_session_data	*sd	= NULL;

	if(src == NULL)	return 0;

	sd = BL_DOWNCAST(BL_PC,src);

	if(src->type == BL_MEC)	sd = ((struct mercenary_data*)src)->msd;
	if(sd == NULL || sd->mcd == NULL)	return 0;

	return pc_readglobalreg(sd,MERCENARY_REG_KILL_COUNT);
}

#define CSV_STR_SIZE	25

struct csv_token
{
	char **str;
	int str_size;
	int token_num;
};

static void csv_init(struct csv_token *t)
{
	t->token_num = 0;
	t->str_size = 0;
	t->str = NULL;
	return;
}

static void csv_parse(struct csv_token *t,char line[])
{
	int i;
	char *p,*np;
	t->str_size = CSV_STR_SIZE;
	t->str = malloc(sizeof(char*)*t->str_size);

	for(i = 0,p  = np = line; *p; p++)
	{
		t->str[i] = np;
		if(*p == ','){
			*p = '\0';
			np = p + 1;
			i++;	
			if(i == t->str_size)
			{
				t->str_size += CSV_STR_SIZE;
				t->str = (char**)realloc(t->str,sizeof(char*)*t->str_size);
			}
		}
	}
	t->token_num = (p[-1] == '\0') ? i -1 :i;
	return;
}

static void csv_final(struct csv_token *t)
{
	free(t->str);
	t->token_num = 0;
	t->str_size = 0;
	return;
}

int mercenary_readdb(void)
{
	FILE *fp;
	char line[1024];
	int i,j=0,lines, count = 0;
	struct script_code *script = NULL;
	char *filename[]={"db/mercenary_db.txt","db/addon/mercenary_db_add.txt"};

	for(i=0; i<MERCENARY_MAX_DB; i++) {
		if(mercenary_db[i].script)
			script_free_code(mercenary_db[i].script);
	}

	for(i=0;i<sizeof(filename)/sizeof(filename[0]);i++){
		fp=fopen(filename[i],"r");
		if(fp==NULL){
			if(i>0)
				continue;
			printf("can't read %s\n",filename[i]);
			return -1;
		}
		lines=0;
		while(fgets(line,1020,fp)){
			int nameid;
			struct csv_token	t;
			lines++;

			if(line[0] == '/' && line[1] == '/')
				continue;

			csv_init(&t);

			csv_parse(&t,line);

			nameid=atoi(t.str[0]);
			j = nameid-MERCENARY_ID;
			if(j<0 || j>=MERCENARY_MAX_DB)
			{
				csv_final(&t);
				continue;
			}
			mercenary_db[j].db_id = nameid;
			mercenary_db[j].view_class = atoi(t.str[1]);
			memcpy(mercenary_db[j].name,t.str[2],24);
			memcpy(mercenary_db[j].jname,t.str[3],24);
			mercenary_db[j].job_id = atoi(t.str[4]);
			mercenary_db[j].base_level = atoi(t.str[5]);
			mercenary_db[j].max_hp = atoi(t.str[6]);
			mercenary_db[j].max_sp = atoi(t.str[7]);
			mercenary_db[j].str = atoi(t.str[8]);
			mercenary_db[j].agi = atoi(t.str[9]);
			mercenary_db[j].vit = atoi(t.str[10]);
			mercenary_db[j].int_ = atoi(t.str[11]);
			mercenary_db[j].dex = atoi(t.str[12]);
			mercenary_db[j].luk = atoi(t.str[13]);
			mercenary_db[j].atk_min = atoi(t.str[14]);
			mercenary_db[j].atk_max = atoi(t.str[15]);
			mercenary_db[j].matk_min = atoi(t.str[16]);
			mercenary_db[j].matk_max = atoi(t.str[17]);
			mercenary_db[j].def = atoi(t.str[18]);
			mercenary_db[j].mdef = atoi(t.str[19]);
			mercenary_db[j].aspd=atoi(t.str[20]);
			mercenary_db[j].size=atoi(t.str[21]);
			mercenary_db[j].race=atoi(t.str[22]);
			mercenary_db[j].element=atoi(t.str[23]);
			mercenary_db[j].range=atoi(t.str[24]);

			if(mercenary_db[j].job_id > MECENARY_MAX_JOB_ID)
			{
				mercenary_db[j].job_id = 0;
				printf("mercenary_db:job_id is overflow!\n");
				continue;
			}
			if(strchr(t.str[25],'{')==NULL)
				continue;
			if(mercenary_db[j].script)
				script_free_code(mercenary_db[j].script);
			script = parse_script(t.str[25],filename[i],lines);

			mercenary_db[j].script = (script != &error_code)? script: NULL;
			count++;

			csv_final(&t);
		}
		fclose(fp);
		printf("read %s done (count=%d)\n",filename[i],count);
	}
	return 0;
}

static int mercenary_parse_skill_db(char *s,char *type)
{
	int i = 0;
	struct _parse_table{
		char *tag;
		int num;
	} target[] = {
			{"enemy",MERCENARY_TARGET_ENEMY},
			{"self",MERCENARY_TARGET_SELF},
			{"master",MERCENARY_TARGET_MASTER},
			{NULL,-1},
	},state[] = {
			{"battle",MERCENARY_STATE_BATTLE},
			{"idle",MERCENARY_STATE_IDLE},
			{"always",MERCENARY_STATE_ALWAYS},
			{NULL,-1},
	},cond[] = {
			{"hp_rate",MERCENARY_COND_HP},
			{"sp_rate",MERCENARY_COND_SP},
			{"status",MERCENARY_COND_STATUS_CHANGE},
			{NULL,-1},
	},*p=NULL;

	if(strcmp(type,"target")==0)
		p = target;
	else if(strcmp(type,"state")==0)
		p = state;
	else if(strcmp(type,"cond")==0)
		p = cond;
	else
		return -1;

	for(i = 0; p[i].tag != NULL; i++)
		if(strcmp(s,p[i].tag) == 0)
			return p[i].num;

	return -1;
}

int mercenary_read_skill_db(void)
{
	FILE *fp;
	char line[1024];
	int i,lines, count = 0;
	char *filename[]={"db/mercenary_skill_db.txt","db/addon/mercenary_skill_db_add.txt"};

	for(i=0;i<sizeof(filename)/sizeof(filename[0]);i++){
		fp=fopen(filename[i],"r");
		if(fp==NULL){
			if(i>0)
				continue;
			printf("can't read %s\n",filename[i]);
			return -1;
		}
		lines = 0;
		while(fgets(line,1020,fp)){
			struct csv_token	t;
			int mec_idx,idx;
			lines++;

			if(line[0] == '/' && line[1] == '/')
				continue;

			csv_init(&t);

			csv_parse(&t,line);

			mec_idx = atoi(t.str[0]) - MERCENARY_ID;
			if(mec_idx < 0 || mec_idx >= MERCENARY_MAX_DB)
				goto PARSE_ERROR;

			idx = mercenary_db[mec_idx].skill_num;

			mercenary_db[mec_idx].skill[idx].id = atoi(t.str[2]);

			mercenary_db[mec_idx].skill[idx].lv = atoi(t.str[3]);

			mercenary_db[mec_idx].skill[idx].target = mercenary_parse_skill_db(t.str[4],"target");
			if(mercenary_db[mec_idx].skill[idx].target == -1){
				printf("unkown target type(line:%d)\n",lines);
				exit(1);
			}

			mercenary_db[mec_idx].skill[idx].rate = atoi(t.str[5]);

			mercenary_db[mec_idx].skill[idx].state = mercenary_parse_skill_db(t.str[6],"state");
			if(mercenary_db[mec_idx].skill[idx].state == -1){
				printf("unkown state type(line:%d)\n",lines);
				exit(1);
			}

			mercenary_db[mec_idx].skill[idx].cond = mercenary_parse_skill_db(t.str[7],"cond");
			if(mercenary_db[mec_idx].skill[idx].cond == MERCENARY_COND_HP){
				mercenary_db[mec_idx].skill[idx].param1 = atoi(t.str[8]);
			}else if(mercenary_db[mec_idx].skill[idx].cond == MERCENARY_COND_SP){
				mercenary_db[mec_idx].skill[idx].param1 = atoi(t.str[8]);
			}else if(mercenary_db[mec_idx].skill[idx].cond == MERCENARY_COND_STATUS_CHANGE){
				mercenary_db[mec_idx].skill[idx].param1 = atoi(t.str[8]);
				mercenary_db[mec_idx].skill[idx].param2 = strcmp(t.str[9],"except") == 0 ? 1 : 0;
				if(mercenary_db[mec_idx].skill[idx].param1 < 0 || mercenary_db[mec_idx].skill[idx].param1 > MAX_STATUSCHANGE)
				{
					printf("unkown status changed type(line:%d)\n",lines);
					mercenary_db[mec_idx].skill[idx].cond = 0;
					exit(1);
				}
			}

			mercenary_db[mec_idx].skill_num = ++idx;

			count++;
PARSE_ERROR:
			csv_final(&t);
		}
		fclose(fp);
		printf("read %s done (count=%d)\n",filename[i],count);
	}
	return 0;
}

int do_init_mercenary(void)
{
	mercenary_readdb();
	mercenary_read_skill_db();
	add_timer_func_list(mercenary_natural_heal_hp,"mercenary_natural_heal_hp");
	add_timer_func_list(mercenary_natural_heal_sp,"mercenary_natural_heal_sp");
	add_timer_func_list(mercenary_free_timer,"mercenary_free_timer");
#ifdef USE_SERVER_SIDE_AI
	add_timer_func_list(mercenary_call_ai,"mercenary_call_ai");
	add_timer_interval(gettick()+MIN_MERCENARY_THINKTIME,mercenary_call_ai,0,0,MIN_MERCENARY_THINKTIME);
#endif

	return 0;
}

int do_final_mercenary(void) {
	int i;
	for(i = 0;i < MAX_HOMUN_DB; i++) {
		if(mercenary_db[i].script) {
			script_free_code(mercenary_db[i].script);
		}
	}
	return 0;
}
