// $Id: battle.c 539 2005-11-21 09:58:59Z Yor $
#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "battle.h"

#include "../common/timer.h"
#include "../common/malloc.h"

#include "nullpo.h"
#include "map.h"
#include "pc.h"
#include "skill.h"
#include "mob.h"
#include "itemdb.h"
#include "clif.h"
#include "pet.h"
#include "guild.h"
#include "status.h"

#ifdef MEMWATCH
#include "memwatch.h"
#endif

int attr_fix_table[4][10][10];

struct Battle_Config battle_config;

/*==========================================
 * 二点間の距離を返す
 * 戻りは整数で0以上
 *------------------------------------------
 */
static int distance(int x0, int y_0, int x1, int y_1)
{
	int dx, dy;

	dx = abs(x0  - x1);
	dy = abs(y_0 - y_1);

	return dx > dy ? dx : dy;
}

/*==========================================
 * 自分をロックしているMOBの?を?える(foreachclient)
 *------------------------------------------
 */
static int battle_counttargeted_sub(struct block_list *bl, va_list ap)
{
	int id, *c, target_lv;
	struct block_list *src;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);

	id = va_arg(ap, int);
	nullpo_retr(0, c = va_arg(ap, int *));
	src = va_arg(ap, struct block_list *);
	target_lv = va_arg(ap, int);

	if (id == bl->id || (src && id == src->id))
		return 0;

	if (bl->type == BL_PC) {
		struct map_session_data *sd = (struct map_session_data *)bl;
		if (sd && sd->attacktarget == id && sd->attacktimer != -1 && sd->attacktarget_lv >= target_lv)
			(*c)++;
	}
	else if (bl->type == BL_MOB) {
		struct mob_data *md = (struct mob_data *)bl;
		if (md && md->target_id == id && md->timer != -1 && md->state.state == MS_ATTACK && md->target_lv >= target_lv)
			(*c)++;
		//printf("md->target_lv:%d, target_lv:%d\n", md->target_lv, target_lv);
	}
	else if (bl->type == BL_PET) {
		struct pet_data *pd = (struct pet_data *)bl;
		if (pd && pd->target_id == id && pd->timer != -1 && pd->state.state == MS_ATTACK && pd->target_lv >= target_lv)
			(*c)++;
	}

	return 0;
}

/*==========================================
 * 自分をロックしている対象の数を返す(汎用)
 * 戻りは整数で0以上
 *------------------------------------------
 */
int battle_counttargeted(struct block_list *bl, struct block_list *src, int target_lv)
{
	int c;

	nullpo_retr(0, bl);

	c = 0;
	map_foreachinarea(battle_counttargeted_sub, bl->m,
	                  bl->x - AREA_SIZE, bl->y - AREA_SIZE,
	                  bl->x + AREA_SIZE, bl->y + AREA_SIZE, 0,
	                  bl->id, &c, src, target_lv);

	return c;
}

//-------------------------------------------------------------------

// ダメージの遅延
struct battle_delay_damage_ {
	struct block_list *src;
	int target;
	int damage;
	int flag;
};

int battle_delay_damage_sub(int tid, unsigned int tick, int id, int data)
{
	struct battle_delay_damage_ *dat = (struct battle_delay_damage_ *)data;
	struct block_list *target = map_id2bl(dat->target);

	if (dat && map_id2bl(id) == dat->src && target && target->prev != NULL)
		battle_damage(dat->src, target, dat->damage, dat->flag);
	FREE(dat);

	return 0;
}

int battle_delay_damage(unsigned int tick, struct block_list *src, struct block_list *target, int damage, int flag)
{
	struct battle_delay_damage_ *dat;

	nullpo_retr(0, src);
	nullpo_retr(0, target);

	CALLOC(dat, struct battle_delay_damage_, 1);

	dat->src = src;
	dat->target = target->id;
	dat->damage = damage;
	dat->flag = flag;
	add_timer(tick, battle_delay_damage_sub, src->id, (int)dat);

	return 0;
}

// 実際にHPを操作
int battle_damage(struct block_list *bl,struct block_list *target,int damage,int flag)
{
	struct map_session_data *sd = NULL;
	struct status_change *sc_data;
	short *sc_count;
	int i;

	nullpo_retr(0, target); //blはNULLで呼ばれることがあるので他でチェック

	sc_data = status_get_sc_data(target);
	if(damage==0 || target->type == BL_PET)
		return 0;

	if(target->prev == NULL)
		return 0;

	if(bl) {
		if(bl->prev==NULL)
			return 0;

		if(bl->type==BL_PC)
			sd=(struct map_session_data *)bl;
	}

	if(damage<0)
		return battle_heal(bl,target,-damage,0,flag);

	if (!flag && (sc_count = status_get_sc_count(target)) != NULL && *sc_count > 0) {
		// 凍結、石化、睡眠を消去
		if(sc_data[SC_FREEZE].timer!=-1)
			status_change_end(target,SC_FREEZE,-1);
		if(sc_data[SC_STONE].timer!=-1 && sc_data[SC_STONE].val2==0)
			status_change_end(target,SC_STONE,-1);
		if(sc_data[SC_SLEEP].timer!=-1)
			status_change_end(target,SC_SLEEP,-1);
	}

	if (target->type == BL_MOB) { // MOB
		struct mob_data *md = (struct mob_data *)target;
		if (md && md->skilltimer != -1 && md->state.skillcastcancel) // 詠唱妨害
			skill_castcancel(target, 0);
		return mob_damage(bl, md, damage, 0);
	}
	else if (target->type == BL_PC) { // PC

		struct map_session_data *tsd = (struct map_session_data *)target;

		if (tsd && tsd->sc_data && tsd->sc_data[SC_DEVOTION].val1) { // ディボーションをかけられている
			struct map_session_data *md;
			if ((md = map_id2sd(tsd->sc_data[SC_DEVOTION].val1)) && md->state.auth) {
				if (skill_devotion3(&md->bl,target->id)) {
					skill_devotion(md, target->id);
				} else if (bl)
					for(i = 0; i < 5; i++)
						if (md->dev.val1[i] == target->id) {
							clif_damage(bl, &md->bl, gettick_cache, 0, 0, damage, 0 , 0, 0);
							pc_damage(&md->bl, md, damage);
							return 0;
						}
			}
		}

		if (tsd && tsd->skilltimer != -1) { // 詠唱妨害
			// フェンカードや妨害されないスキルかの検査
			if ((!tsd->special_state.no_castcancel || map[bl->m].flag.gvg) && tsd->state.skillcastcancel &&
			    !tsd->special_state.no_castcancel2)
				skill_castcancel(target, 0);
		}

		return pc_damage(bl, tsd, damage);

	}
	else if (target->type == BL_SKILL)
		return skill_unit_ondamaged((struct skill_unit *)target, bl, damage, gettick_cache);

	return 0;
}

int battle_heal(struct block_list *bl, struct block_list *target, int hp, int sp, int flag)
{
	nullpo_retr(0, target); //blはNULLで呼ばれることがあるので他でチェック

	if(target->type == BL_PET)
		return 0;
	if(target->type == BL_PC && pc_isdead((struct map_session_data *)target))
		return 0;
	if(hp == 0 && sp == 0)
		return 0;

	if (hp < 0)
		return battle_damage(bl, target, -hp, flag);

	if(target->type==BL_MOB)
		return mob_heal((struct mob_data *)target,hp);
	else if(target->type==BL_PC)
		return pc_heal((struct map_session_data *)target,hp,sp);

	return 0;
}

// 攻撃停止
void battle_stopattack(struct block_list *bl) {
	nullpo_retv(bl);

	if (bl->type == BL_MOB)
		mob_stopattack((struct mob_data*)bl);
	else if (bl->type == BL_PC)
		pc_stopattack((struct map_session_data*)bl);
	else if (bl->type == BL_PET)
		pet_stopattack((struct pet_data*)bl);

	return;
}

// 移動停止
int battle_stopwalking(struct block_list *bl,int type)
{
	nullpo_retr(0, bl);

	if(bl->type==BL_MOB)
		return mob_stop_walking((struct mob_data*)bl,type);
	else if(bl->type==BL_PC)
		return pc_stop_walking((struct map_session_data*)bl, type);
	else if(bl->type==BL_PET)
		return pet_stop_walking((struct pet_data*)bl,type);

	return 0;
}

/*==========================================
 * ダメージの属性修正
 *------------------------------------------
 */
int battle_attr_fix(int damage, int atk_elem, int def_elem)
{
	int def_type = def_elem % 10, def_lv = def_elem / 10 / 2;

	if (atk_elem<0 || atk_elem > 9 || def_type < 0 || def_type > 9 ||
		def_lv < 1 || def_lv > 4) { // 属 性値がおかしいのでとりあえずそのまま返す
		if (battle_config.error_log)
			printf("battle_attr_fix: unknown attr type: atk=%d def_type=%d def_lv=%d\n", atk_elem, def_type, def_lv);
		return damage;
	}

	return damage * attr_fix_table[def_lv-1][atk_elem][def_type] / 100;
}

/*==========================================
 * ダメージ最終計算
 *------------------------------------------
 */
int battle_calc_damage(struct block_list *src, struct block_list *bl, int damage, int div_, int skill_num, int skill_lv, int flag) {
	struct map_session_data *sd = NULL;
	struct mob_data *md = NULL;
	struct status_change *sc_data, *sc;
	short *sc_count;
	int class;

	nullpo_retr(0, bl);

	class = status_get_class(bl);
	if (bl->type == BL_MOB) md = (struct mob_data *)bl;
	else sd = (struct map_session_data *)bl;

	sc_data = status_get_sc_data(bl);
	sc_count = status_get_sc_count(bl);

	if (sc_count != NULL && *sc_count > 0) {

		if (sc_data[SC_SAFETYWALL].timer != -1 && damage > 0 && flag & BF_WEAPON && flag & BF_SHORT && skill_num != NPC_GUIDEDATTACK) {
			// セーフティウォール
			struct skill_unit *unit;
			unit = (struct skill_unit *)sc_data[SC_SAFETYWALL].val2;
			if (unit) {
				if (unit->group && (--unit->group->val2) <= 0)
					skill_delunit(unit);
				damage = 0;
			} else {
				status_change_end(bl, SC_SAFETYWALL, -1);
			}
		}

		if (sc_data[SC_PNEUMA].timer != -1 && damage > 0 && ((flag & BF_WEAPON && flag & BF_LONG && skill_num != NPC_GUIDEDATTACK) ||
// A lot of Corrections to BREAKER SKILL (pneuma included) (Posted on freya's bug report by Gawaine)
//		    (flag & BF_MISC && (skill_num == HT_BLITZBEAT || skill_num == SN_FALCONASSAULT)))) { // [DracoRPG]
		    (flag & BF_MISC && (skill_num == HT_BLITZBEAT || skill_num == SN_FALCONASSAULT || skill_num == TF_THROWSTONE || skill_num == CR_ACIDDEMONSTRATION)) ||
		    (flag & BF_MAGIC && skill_num == ASC_BREAKER))) { // It should block only physical part of Breaker! [Lupus], on the contrary, players all over the boards say it completely blocks Breaker x.x' [Skotlex]

// End ----------- A lot of Corrections to BREAKER SKILL (pneuma included) (Posted on freya's bug report by Gawaine)
			// ニューマ
			damage=0;
		}

		if (sc_data[SC_ROKISWEIL].timer != -1 && damage > 0 &&
		    flag & BF_MAGIC) {
			// ニューマ
			damage = 0;
		}

		if (sc_data[SC_AETERNA].timer != -1 && damage > 0) { // レックスエーテルナ
			damage <<= 1;
			status_change_end(bl, SC_AETERNA, -1);
		}

		//属性場のダメージ増加
		if (sc_data[SC_VOLCANO].timer != -1) { // ボルケーノ
			if (flag & BF_SKILL && skill_get_pl(skill_num) == 3)
				damage += damage * sc_data[SC_VOLCANO].val4 / 100;
			else if (!flag & BF_SKILL && status_get_attack_element(bl) == 3)
				damage += damage * sc_data[SC_VOLCANO].val4 / 100;
		}

		if(sc_data[SC_VIOLENTGALE].timer!=-1){	// バイオレントゲイル
			if(flag&BF_SKILL && skill_get_pl(skill_num)==4)
				damage += damage*sc_data[SC_VIOLENTGALE].val4/100;
			else if(!flag&BF_SKILL && status_get_attack_element(bl)==4)
				damage += damage*sc_data[SC_VIOLENTGALE].val4/100;
		}

		if(sc_data[SC_DELUGE].timer!=-1){	// デリュージ
			if(flag&BF_SKILL && skill_get_pl(skill_num)==1)
				damage += damage*sc_data[SC_DELUGE].val4/100;
			else if(!flag&BF_SKILL && status_get_attack_element(bl)==1)
				damage += damage*sc_data[SC_DELUGE].val4/100;
		}

		if (sc_data[SC_ENERGYCOAT].timer != -1 && damage > 0  && flag & BF_WEAPON) { // エナジーコート
			if (sd) {
				if (sd->status.sp > 0) {
					int per = sd->status.sp * 5 / (sd->status.max_sp + 1);
					sd->status.sp -= sd->status.sp * (per * 5 + 10) / 1000;
					if (sd->status.sp < 0)
						sd->status.sp = 0;
					damage -= damage * ((per + 1) * 6) / 100;
					clif_updatestatus(sd, SP_SP);
				}
				if (sd->status.sp <= 0)
					status_change_end(bl, SC_ENERGYCOAT, -1);
			} else
				damage -= damage * (sc_data[SC_ENERGYCOAT].val1 * 6) / 100;
		}

		if(sc_data[SC_KYRIE].timer!=-1 && damage > 0){	// キリエエレイソン
			sc=&sc_data[SC_KYRIE];
			sc->val2-=damage;
			if(flag&BF_WEAPON){
				if(sc->val2>=0)	damage=0;
				else damage=-sc->val2;
			}
			if((--sc->val3)<=0 || (sc->val2<=0) || skill_num == AL_HOLYLIGHT)
				status_change_end(bl, SC_KYRIE, -1);
		}

		if(sc_data[SC_BASILICA].timer!=-1 && damage > 0){
			// ニューマ
			damage=0;
		}
		if(sc_data[SC_LANDPROTECTOR].timer!=-1 && damage>0 && flag&BF_MAGIC){
			// ニューマ
			damage=0;
		}

		if(sc_data[SC_AUTOGUARD].timer != -1 && damage > 0 && flag&BF_WEAPON) {
			if(rand()%100 < sc_data[SC_AUTOGUARD].val2) {
				damage = 0;
				clif_skill_nodamage(bl, bl, CR_AUTOGUARD, sc_data[SC_AUTOGUARD].val1, 1);
				// different delay depending on skill level [celest]
			  {
				int delay;
				if (sc_data[SC_AUTOGUARD].val1 <= 5)
					delay = 300;
				else if (sc_data[SC_AUTOGUARD].val1 > 5 && sc_data[SC_AUTOGUARD].val1 <= 9)
					delay = 200;
				else
					delay = 100;
				if(sd)
					sd->canmove_tick = gettick_cache + delay;
				else if(md)
					md->canmove_tick = gettick_cache + delay;
			  }
			}
		}
// -- moonsoul (chance to block attacks with new Lord Knight skill parrying)
//
		if(sc_data[SC_PARRYING].timer != -1 && damage > 0 && flag&BF_WEAPON) {
			if(rand()%100 < sc_data[SC_PARRYING].val2) {
				damage = 0;
				clif_skill_nodamage(bl,bl,LK_PARRYING,sc_data[SC_PARRYING].val1,1);
			}
		}
		// リジェクトソード
		// Reject Sword: Fixed the condition check - The weapon condition was incorrect. Wasn't working for swords - [Aalye]
		if (sc_data[SC_REJECTSWORD].timer != -1 && damage > 0 && flag&BF_WEAPON &&
		   ((src->type == BL_PC && (((struct map_session_data *)src)->status.weapon == 1 ||
		    ((struct map_session_data *)src)->status.weapon == 2 || ((struct map_session_data *)src)->status.weapon == 3)) ||
		    src->type==BL_MOB) && sc_data[SC_REJECTSWORD].val3 != 1) {
			if (rand()%100 < (15 * sc_data[SC_REJECTSWORD].val1)) { //反射確率は15*Lv
				damage = damage * 50 / 100;
				clif_damage(bl, src, gettick_cache, 0, 0, damage, 0, 0, 0); // So to display the damage add this [Celest] from Freya' forum
				battle_damage(bl, src, damage, 0);
				//ダメージを与えたのは良いんだが、ここからどうして表示するんだかわかんねぇ
				//エフェクトもこれでいいのかわかんねぇ
				clif_skill_nodamage(bl, bl, ST_REJECTSWORD, sc_data[SC_REJECTSWORD].val1, 1);
				sc_data[SC_REJECTSWORD].val3 = 1; // Reject Sword: Turn the damage reflect state on - [Aalye]
				if ((--sc_data[SC_REJECTSWORD].val2) <= 0)
					status_change_end(bl, SC_REJECTSWORD, -1);
			}
		}
		if (sc_data[SC_SPIDERWEB].timer!=-1 && damage > 0)
			if ((flag & BF_SKILL && skill_get_pl(skill_num) == 3) ||
			    (!flag & BF_SKILL && status_get_attack_element(src) == 3)) {
				damage<<=1;
				status_change_end(bl, SC_SPIDERWEB, -1);
			}

		if (sc_data[SC_FOGWALL].timer != -1 && flag&BF_MAGIC)
			if (rand() % 100 < sc_data[SC_FOGWALL].val2)
				damage = 0;
	}

	if (class >= 1285 && class <= 1288) {
//	if (class == 1288) {
		if (class == 1288 && flag & BF_SKILL)
			damage = 0;
		if (src->type == BL_PC) {
			struct guild *g = guild_search(((struct map_session_data *)src)->status.guild_id);
			struct guild_castle *gc = guild_mapname2gc(map[bl->m].name);
			if (!((struct map_session_data *)src)->status.guild_id)
				damage = 0;
			else if (gc && agit_flag == 0 && class != 1288) // guardians cannot be damaged during non-woe [Valaris]
				damage = 0; // end woe check [Valaris]
			else if (g == NULL)
				damage = 0; //ギルド未加入ならダメージ無し
			else if ((gc != NULL) && guild_isallied(g, gc))
				damage = 0; //自占領ギルドのエンペならダメージ無し
			else if (guild_checkskill(g, GD_APPROVAL) <= 0)
				damage = 0; //正規ギルド承認がないとダメージ無し
			else if (battle_config.guild_max_castles != 0 && guild_checkcastles(g) >= battle_config.guild_max_castles)
				damage = 0; // [MouseJstr]
		} else
			damage = 0;
	}

	if (damage > 0) { // damage reductions
		if (map[bl->m].flag.gvg) { //GvG
			if (bl->type == BL_MOB){ //defenseがあればダメージが減るらしい？
				struct guild_castle *gc = guild_mapname2gc(map[bl->m].name);
				if (gc) damage -= damage * (gc->defense / 100) * (battle_config.castle_defense_rate / 100);
			}
			if (flag & BF_WEAPON) {
				if (flag & BF_SHORT)
					damage = damage * battle_config.gvg_short_damage_rate / 100;
				if (flag & BF_LONG)
					damage = damage * battle_config.gvg_long_damage_rate / 100;
			}
			if (flag & BF_MAGIC)
				damage = damage * battle_config.gvg_magic_damage_rate / 100;
			if (flag & BF_MISC)
				damage = damage * battle_config.gvg_misc_damage_rate / 100;
		} else if (battle_config.pk_mode && bl->type == BL_PC) {
			if (flag & BF_WEAPON) {
				if (flag & BF_SHORT)
					damage = damage * 4 / 5; // damage = damage * 80 / 100;
				if (flag & BF_LONG)
					damage = damage * 7 / 10; // damage = damage * 70 / 100;
			}
			if (flag & BF_MAGIC)
				damage = damage * 3 / 5; // damage = damage * 60 / 100;
			if (flag & BF_MISC)
				damage = damage * 3 / 5; // damage = damage * 60 / 100;
		}
		if (damage < 1) damage  = 1;
	}

	if (battle_config.skill_min_damage || flag & BF_MISC) {
		if (div_ < 255) {
			if (damage > 0 && damage < div_)
				damage = div_;
		}
		else if (damage > 0 && damage < 3)
			damage = 3;
	}

	if (md != NULL && md->hp > 0 && damage > 0) // 反撃などのMOBスキル判定
		mobskill_event(md, flag);

	return damage;
}

/*==========================================
 * HP/SP吸収の計算
 *------------------------------------------
 */
int battle_calc_drain(int damage, int rate, int per, int val)
{
	int diff = 0;

	if (damage <= 0 || rate <= 0)
		return 0;

	if (per && rand() % 100 < rate) {
		diff = (damage * per) / 100;
		if (diff == 0) {
			if (per > 0)
				diff = 1;
			else
				diff = -1;
		}
	}

	if (val && rand() % 100 < rate) {
		diff += val;
	}

	return diff;
}

/*==========================================
 * 修練ダメージ
 *------------------------------------------
 */
int battle_addmastery(struct map_session_data *sd,struct block_list *target,int dmg,int type)
{
	int damage,skill;
	int race=status_get_race(target);
	int weapon;
	damage = 0;

	nullpo_retr(0, sd);

	// デーモンベイン(+3 ～ +30) vs 不死 or 悪魔 (死人は含めない？)
	if ((skill = pc_checkskill(sd, AL_DEMONBANE)) > 0 && (battle_check_undead(race,status_get_elem_type(target)) || race == 6))
		damage += (skill * (3 + (sd->status.base_level + 1) / 20)); // submitted by orn
		//damage += (skill * 3);

	// ビーストベイン(+4 ～ +40) vs 動物 or 昆虫
	if ((skill = pc_checkskill(sd, HT_BEASTBANE)) > 0 && (race == 2 || race == 4))
		damage += (skill * 4);

	if(type == 0)
		weapon = sd->weapontype1;
	else
		weapon = sd->weapontype2;
	switch(weapon)
	{
		case 0x01:	// 短剣 Knife
		case 0x02:	// 1HS
		{
			// 剣修練(+4 ～ +40) 片手剣 短剣含む
			if((skill = pc_checkskill(sd,SM_SWORD)) > 0) {
				damage += (skill * 4);
			}
			break;
		}
		case 0x03:	// 2HS
		{
			// 両手剣修練(+4 ～ +40) 両手剣
			if((skill = pc_checkskill(sd,SM_TWOHAND)) > 0) {
				damage += (skill * 4);
			}
			break;
		}
		case 0x04:	// 1HL
		case 0x05:	// 2HL
		{
			// 槍修練(+4 ～ +40,+5 ～ +50) 槍
			if((skill = pc_checkskill(sd,KN_SPEARMASTERY)) > 0) {
				if(!pc_isriding(sd))
					damage += (skill * 4);	// ペコに乗ってない
				else
					damage += (skill * 5);	// ペコに乗ってる
			}
			break;
		}
		case 0x06: // 片手斧
		case 0x07: // Axe by Tato
		{
			if((skill = pc_checkskill(sd,AM_AXEMASTERY)) > 0) {
				damage += (skill * 3);
			}
			break;
		}
		case 0x08:	// メイス
		{
			// メイス修練(+3 ～ +30) メイス
			if((skill = pc_checkskill(sd,PR_MACEMASTERY)) > 0) {
				damage += (skill * 3);
			}
			break;
		}
		case 0x09:	// なし?
			break;
		case 0x0a:	// 杖
			break;
		case 0x0b:	// 弓
			break;
		case 0x00:	// 素手 Bare Hands
		case 0x0c:	// Knuckles
		{
			// 鉄拳(+3 ～ +30) 素手,ナックル
			if((skill = pc_checkskill(sd,MO_IRONHAND)) > 0) {
				damage += (skill * 3);
			}
			break;
		}
		case 0x0d:	// Musical Instrument
		{
			// 楽器の練習(+3 ～ +30) 楽器
			if((skill = pc_checkskill(sd,BA_MUSICALLESSON)) > 0) {
				damage += (skill * 3);
			}
			break;
		}
		case 0x0e:	// Dance Mastery
		{
			// Dance Lesson Skill Effect(+3 damage for every lvl = +30) 鞭
			if((skill = pc_checkskill(sd,DC_DANCINGLESSON)) > 0) {
				damage += (skill * 3);
			}
			break;
		}
		case 0x0f:	// Book
		{
			// Advance Book Skill Effect(+3 damage for every lvl = +30) {
			if((skill = pc_checkskill(sd,SA_ADVANCEDBOOK)) > 0) {
				damage += (skill * 3);
			}
			break;
		}
		case 0x10:	// Katars
		{
			// カタール修練(+3 ～ +30) カタール
			if((skill = pc_checkskill(sd,AS_KATAR)) > 0) {
				//ソニックブロー時は別処理（1撃に付き1/8適応)
				damage += (skill * 3);
			}
			break;
		}
	}
	damage = dmg + damage;

	return (damage);
}

static struct Damage battle_calc_pet_weapon_attack(
	struct block_list *src, struct block_list *target, int skill_num, int skill_lv, int wflag)
{
	struct pet_data *pd = (struct pet_data *)src;
	struct mob_data *tmd = NULL;
	int hitrate, flee, cri = 0, atkmin, atkmax;
	int luk,target_count;
	int def1 = status_get_def(target);
	int def2 = status_get_def2(target);
	int t_vit = status_get_vit(target);
	struct Damage wd;
	int damage, damage2 = 0, type, div_, blewcount = skill_get_blewcount(skill_num, skill_lv);
	int flag, dmg_lv = 0;
	int t_mode = 0, t_race = 0, t_size = 1, s_race = 0, s_ele = 0;
	struct status_change *t_sc_data;
	int div_flag = 0; // 0: total damage is to be divided by div_
	                  // 1: damage is distributed,and has to be multiplied by div_ [celest]

	//return前の処理があるので情報出力部のみ変更
	if (target == NULL || pd == NULL) { //srcは内容に直接触れていないのでスルーしてみる
		memset(&wd, 0, sizeof(wd));
		return wd;
	}

	s_race = status_get_race(src);
	s_ele = status_get_attack_element(src);

	// ターゲット
	if (target->type == BL_MOB)
		tmd = (struct mob_data *)target;
	else {
		memset(&wd, 0, sizeof(wd));
		return wd;
	}
	t_race = status_get_race(target);
	t_size = status_get_size(target);
	t_mode = status_get_mode(target);
	t_sc_data = status_get_sc_data(target);

	flag = BF_SHORT | BF_WEAPON | BF_NORMAL; // 攻撃の種類の設定

	// 回避率計算、回避判定は後で
	flee = status_get_flee(target);
	if (battle_config.agi_penalty_type > 0) {
		target_count = battle_counttargeted(target, src, battle_config.agi_penalty_count_lv) - battle_config.agi_penalty_count;
		if (target_count > 0) {
			if (battle_config.agi_penalty_type == 1)
				flee = (flee * (100 - target_count * battle_config.agi_penalty_num)) / 100;
			else if (battle_config.agi_penalty_type == 2)
				flee -= (target_count * battle_config.agi_penalty_num);
			if (flee < 1) flee = 1;
		}
	}
	hitrate = status_get_hit(src) - flee + 80;

	type = 0; // normal
	if (skill_num > 0) {
		div_ = skill_get_num(skill_num, skill_lv);
		if (div_ < 1) div_ = 1; // Avoid the rare case where the db says div_ is 0 and below
	} else
		div_ = 1; // single attack

	luk=status_get_luk(src);

	if(battle_config.pet_str)
		damage = status_get_baseatk(src);
	else
		damage = 0;

	if (skill_num == HW_MAGICCRASHER){			/* マジッククラッシャーはMATKで殴る */
		atkmin = status_get_matk1(src);
		atkmax = status_get_matk2(src);
	} else {
		atkmin = status_get_atk(src);
		atkmax = status_get_atk2(src);
	}
	if (mob_db[pd->class].range > 3)
		flag = (flag & ~BF_RANGEMASK) | BF_LONG;

	if (atkmin > atkmax) atkmin = atkmax;

	cri = status_get_critical(src);
	cri -= status_get_luk(target) * 2; // luk/5*10 => target_luk*2 not target_luk*3
	if(battle_config.enemy_critical_rate != 100) {
		cri = cri * battle_config.enemy_critical_rate / 100;
		if (cri < 1)
			cri = 1;
	}
	if (t_sc_data) {
		if (t_sc_data[SC_SLEEP].timer != -1)
			cri <<= 1;
		if (t_sc_data[SC_JOINTBEAT].timer != -1 &&
		    t_sc_data[SC_JOINTBEAT].val2 == 6) // Always take crits with Neck broken by Joint Beat [DracoRPG]
			cri = 1000;
	}

	if((skill_num == 0 || skill_num == KN_AUTOCOUNTER) && skill_lv >= 0 && battle_config.enemy_critical && (rand() % 1000) < cri)	// 判定（スキルの場合は無視）
			// 敵の判定
	{
		damage += atkmax;
		type = 0x0a;
	}
	else {
		int vitbonusmax;

		if (atkmax > atkmin)
			damage += atkmin + rand() % (atkmax - atkmin + 1);
		else
			damage += atkmin;
		// スキル修正１（攻撃力倍化系）
		// オーバートラスト(+5% ～ +25%),他攻撃系スキルの場合ここで補正
		// バッシュ,マグナムブレイク,
		// ボーリングバッシュ,スピアブーメラン,ブランディッシュスピア,スピアスタッブ,
		// メマーナイト,カートレボリューション
		// ダブルストレイフィング,アローシャワー,チャージアロー,
		// ソニックブロー
		if (skill_num > 0) {
			int i;
			if ((i = skill_get_pl(skill_num)) > 0)
				s_ele = i;

			flag = (flag & ~BF_SKILLMASK) | BF_SKILL;
			switch(skill_num) {
			case SM_BASH:		// バッシュ
				damage = damage*(100+ 30*skill_lv)/100;
				hitrate = (hitrate*(100+5*skill_lv))/100;
				break;
			case SM_MAGNUM:		// マグナムブレイク
				damage = damage * (5 * skill_lv + (wflag ? 65 : 115)) / 100;
				hitrate += 10 * skill_lv;
				break;
			case MC_MAMMONITE:	// メマーナイト
				damage = damage*(100+ 50*skill_lv)/100;
				break;
			case AC_DOUBLE:	// ダブルストレイフィング
				damage = damage*(180+ 20*skill_lv)/100;
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				break;
			case AC_SHOWER:	// アローシャワー
				damage = damage*(75 + 5*skill_lv)/100;
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				break;
			case AC_CHARGEARROW:	// チャージアロー
				damage = damage*150/100;
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				break;
			case KN_PIERCE:	// ピアース
				damage = damage * (100 + 10 * skill_lv) / 100;
				hitrate = hitrate * (100 + 5 * skill_lv) / 100;
				div_ = t_size + 1;
				div_flag = 1;
				break;
			case KN_SPEARSTAB:	// スピアスタブ
				damage = damage*(100+ 15*skill_lv)/100;
				break;
			case KN_SPEARBOOMERANG:	// スピアブーメラン
				damage = damage*(100+ 50*skill_lv)/100;
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				break;
			case KN_BRANDISHSPEAR: // ブランディッシュスピア
				damage = damage*(100+ 20*skill_lv)/100;
				if(skill_lv>3 && wflag==1) damage2+=damage/2;
				if(skill_lv>6 && wflag==1) damage2+=damage/4;
				if(skill_lv>9 && wflag==1) damage2+=damage/8;
				if(skill_lv>6 && wflag==2) damage2+=damage/2;
				if(skill_lv>9 && wflag==2) damage2+=damage/4;
				if(skill_lv>9 && wflag==3) damage2+=damage/2;
				damage +=damage2;
				blewcount=0;
				break;
			case KN_BOWLINGBASH:	// ボウリングバッシュ
				damage = damage*(100+ 50*skill_lv)/100;
				blewcount=0;
				break;
			case AS_GRIMTOOTH:
				damage = damage*(100+ 20*skill_lv)/100;
				break;
			case AS_POISONREACT: // celest
				s_ele = 0;
				damage = damage*(100+ 30*skill_lv)/100;
				break;
			case AS_SONICBLOW:	// ソニックブロウ
				damage = damage*(300+ 50*skill_lv)/100;
				break;
			case TF_SPRINKLESAND:	// 砂まき
				damage = damage*125/100;
				break;
			case MC_CARTREVOLUTION:	// カートレボリューション
				damage = (damage*150)/100;
				break;
			// 以下MOB
			case NPC_COMBOATTACK:	// 多段攻撃
				div_flag = 1;
				break;
			case NPC_RANDOMATTACK:	// ランダムATK攻撃
				damage = damage*(50+rand()%150)/100;
				break;
			// 属性攻撃（適当）
			case NPC_WATERATTACK:
			case NPC_GROUNDATTACK:
			case NPC_FIREATTACK:
			case NPC_WINDATTACK:
			case NPC_POISONATTACK:
			case NPC_HOLYATTACK:
			case NPC_DARKNESSATTACK:
			case NPC_UNDEADATTACK:
			case NPC_TELEKINESISATTACK:
				div_= pd->skillduration; // [Valaris]
				break;
			case NPC_GUIDEDATTACK:
				hitrate = 1000000;
				break;
			case NPC_RANGEATTACK:
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				break;
			case NPC_PIERCINGATT:
				flag=(flag&~BF_RANGEMASK)|BF_SHORT;
				break;
			case RG_BACKSTAP:	// バックスタブ
				damage = damage*(300+ 40*skill_lv)/100;
				hitrate = 1000000;
				break;
			case RG_RAID:	// サプライズアタック
				damage = damage*(100+ 40*skill_lv)/100;
				break;
			case RG_INTIMIDATE:	// インティミデイト
				damage = damage*(100+ 30*skill_lv)/100;
				break;
			case CR_SHIELDCHARGE:	// シールドチャージ
				damage = damage*(100+ 20*skill_lv)/100;
				flag=(flag&~BF_RANGEMASK)|BF_SHORT;
				s_ele = 0;
				break;
			case CR_SHIELDBOOMERANG:	// シールドブーメラン
				damage = damage * (100 + 30 * skill_lv) / 100;
				flag = (flag&~BF_RANGEMASK)|BF_LONG;
				s_ele = 0;
				break;
			case CR_HOLYCROSS:	// ホーリークロス
				damage = damage * (100 + 35 * skill_lv) / 100;
				break;
			case CR_GRANDCROSS:
				hitrate = 1000000;
				break;
			case PA_SHIELDCHAIN:
				damage = damage * (100 + 30 * skill_lv) / 100;
				flag = (flag&~BF_RANGEMASK)|BF_LONG;
				s_ele = 0;
				break;
			case AM_DEMONSTRATION:	// デモンストレーション
				hitrate= 1000000;
				damage = damage * (100 + 20 * skill_lv) / 100;
				damage2 = damage2 * (100 + 20 * skill_lv) / 100;
				break;
			case AM_ACIDTERROR:	// アシッドテラー
				hitrate = 1000000;
				damage = damage * (100 + 40 * skill_lv) / 100;
				damage2 = damage2 * (100 + 40 * skill_lv) / 100;
				break;
			case MO_FINGEROFFENSIVE:	//指弾
				damage = damage * (125 + 25 * skill_lv) / 100;
				flag = (flag&~BF_RANGEMASK)|BF_LONG;   //orn
				break;
			case MO_INVESTIGATE:	// 発 勁
				if (def1 < 1000000)
					damage = damage * (100 + 75 * skill_lv) / 100 * (def1 + def2) / 50;
				hitrate = 1000000;
				s_ele = 0;
				break;
			case MO_EXTREMITYFIST:	// 阿修羅覇鳳拳
				damage = damage * 8 + 250 + (skill_lv * 150);
				hitrate = 1000000;
				s_ele = 0;
				break;
			case MO_CHAINCOMBO:	// 連打掌
				damage = damage*(150+ 50*skill_lv)/100;
				break;
			case MO_COMBOFINISH:	// 猛龍拳
				damage = damage*(240+ 60*skill_lv)/100;
				break;
			case DC_THROWARROW:	// 矢撃ち
				damage = damage * (125 + 25 * skill_lv) / 100; // updated damage (thanks to [Mr_KrzYch00] from freya's bug report)
				flag = (flag & ~BF_RANGEMASK) | BF_LONG;
				break;
			case BA_MUSICALSTRIKE:	// ミュージカルストライク
				damage = damage * (125 + 25 * skill_lv) / 100; // updated damage (thanks to [Mr_KrzYch00] from freya's bug report)
				flag = (flag & ~BF_RANGEMASK) | BF_LONG;
				break;
			case CH_TIGERFIST: // 伏虎拳
				damage = damage * (40 + 100 * skill_lv) / 100; // kRO patch 12/14/04 - Damage has been increased to 140% ~ 540%.
				break;
			case CH_CHAINCRUSH: // 連柱崩撃
				damage = damage * (400 + 100 * skill_lv) / 100; // kRO patch 12/14/04 - Damage has been increased to 500% ~ 1400%.
				break;
			case CH_PALMSTRIKE: // 猛虎硬派山
				damage = damage * (200 + 100 * skill_lv) / 100; // kRO patch 12/14/04 - SP taken at maximum has been reduced to 10, and damage has been increased to 300% ~ 700%.
				break;
			case LK_SPIRALPIERCE:			/* スパイラルピアース */
				damage = damage * (100 + 50 * skill_lv) / 100; //増加量が分からないので適当に
				flag = (flag & ~BF_RANGEMASK) | BF_LONG;
				if (target->type == BL_PC)
					((struct map_session_data *)target)->canmove_tick = gettick_cache + 1000;
				else if (target->type == BL_MOB)
					((struct mob_data *)target)->canmove_tick = gettick_cache + 1000;
				break;
			case LK_HEADCRUSH:				/* ヘッドクラッシュ */
				damage = damage*(100+ 40*skill_lv)/100;
				break;
			case LK_JOINTBEAT:				/* ジョイントビート */
				damage = damage*(50+ 10*skill_lv)/100;
				break;
			case ASC_METEORASSAULT:			/* メテオアサルト */
				damage = damage*(40+ 40*skill_lv)/100;
				break;
			case SN_SHARPSHOOTING:			/* シャープシューティング */
				damage += damage*(30*skill_lv)/100;
				break;
			case CG_ARROWVULCAN: /* アローバルカン */
				damage = damage * (200 + 100 * skill_lv) / 100;
				break;
			case AS_SPLASHER:		/* ベナムスプラッシャー */
				damage = damage*(200+20*skill_lv)/100;
				break;
			}
			if (div_flag && div_ > 1) { // [Skotlex]
				damage *= div_;
				damage2 *= div_;
			}
		}

		if (skill_num != NPC_CRITICALSLASH) {
			// 対 象の防御力によるダメージの減少
			// ディバインプロテクション（ここでいいのかな？）
			if (skill_num != MO_INVESTIGATE && skill_num != MO_EXTREMITYFIST && skill_num != KN_AUTOCOUNTER && skill_num != AM_ACIDTERROR && def1 < 1000000) {	//DEF, VIT無視
				int t_def;
				if (battle_config.vit_penalty_type > 0) {
					target_count = battle_counttargeted(target, src, battle_config.vit_penalty_count_lv) - battle_config.vit_penalty_count;
					if (target_count > 0) {
						if (battle_config.vit_penalty_type == 1) {
							def1 = (def1 * (100 - target_count * battle_config.vit_penalty_num)) / 100;
							def2 = (def2 * (100 - target_count * battle_config.vit_penalty_num)) / 100;
							t_vit = (t_vit * (100 - target_count * battle_config.vit_penalty_num)) / 100;
						} else if (battle_config.vit_penalty_type == 2) {
							def1 -= (target_count * battle_config.vit_penalty_num);
							def2 -= (target_count * battle_config.vit_penalty_num);
							t_vit -= (target_count * battle_config.vit_penalty_num);
						} else if (battle_config.vit_penalty_type == 3) { // (penalize only VIT, not DEF)
							t_vit = (t_vit * (100 - target_count * battle_config.vit_penalty_num)) / 100;
						} else if (battle_config.vit_penalty_type == 4) { // (penalize only VIT, not DEF)
							t_vit -= (target_count * battle_config.vit_penalty_num);
						}
						if (def1 < 0) def1 = 0;
						if (def2 < 1) def2 = 1;
						if (t_vit < 1) t_vit = 1;
					}
				}
				t_def = def2 * 8 / 10;
				vitbonusmax = (t_vit / 20) * (t_vit / 20) - 1;
				if (battle_config.pet_defense_type) {
					damage = damage - (def1 * battle_config.pet_defense_type) - t_def - ((vitbonusmax < 1) ? 0 : rand() % (vitbonusmax + 1));
				} else {
					damage = damage * (100 - def1) / 100 - t_def - ((vitbonusmax < 1) ? 0 : rand() % (vitbonusmax + 1));
				}
			}
		}
	}

	// 0未満だった場合1に補正
	if (damage < 1) damage = 1;

	// 回避修正
	if (hitrate < 1000000 && t_sc_data) { // 必中攻撃
		if (t_sc_data[SC_FOGWALL].timer != -1 && flag & BF_LONG)
			hitrate -= 50;
		if (t_sc_data[SC_SLEEP].timer != -1 ||	// 睡眠は必中
		    t_sc_data[SC_STAN].timer != -1 ||		// スタンは必中
		    t_sc_data[SC_FREEZE].timer != -1 ||
		    (t_sc_data[SC_STONE].timer != -1 && t_sc_data[SC_STONE].val2 == 0)) // 凍結は必中
			hitrate = 1000000;
	}
	if (hitrate < 1000000)
		hitrate = ((hitrate > 95) ? 95: ((hitrate < 5) ? 5 : hitrate));
	if (type == 0 && rand() % 100 >= hitrate) {
		damage = damage2 = 0;
		dmg_lv = ATK_FLEE;
	} else {
		dmg_lv = ATK_DEF;
	}


	if(t_sc_data) {
		int cardfix=100;
		if(t_sc_data[SC_DEFENDER].timer != -1 && flag&BF_LONG)
			cardfix=cardfix*(100-t_sc_data[SC_DEFENDER].val2)/100;
		if(t_sc_data[SC_FOGWALL].timer != -1 && flag&BF_LONG)
			cardfix=cardfix*(100-t_sc_data[SC_FOGWALL].val2)/100;
		if(cardfix != 100)
			damage=damage*cardfix/100;
	}
	if(damage < 0) damage = 0;

	// 属 性の適用
	if(skill_num != 0 || s_ele != 0 || !battle_config.pet_attack_attr_none)
	damage=battle_attr_fix(damage, s_ele, status_get_element(target));

	if(skill_num==PA_PRESSURE) /* プレッシャー 必中? */
		damage = 500+300*skill_lv;

	// インベナム修正
	if(skill_num==TF_POISON){
		damage = battle_attr_fix(damage + 15*skill_lv, s_ele, status_get_element(target));
	}
	if(skill_num==MC_CARTREVOLUTION){
		damage = battle_attr_fix(damage, 0, status_get_element(target));
	}

	// 完全回避の判定
	if (battle_config.enemy_perfect_flee) {
		if (skill_num == 0 && tmd != NULL && rand() % 1000 < status_get_flee2(target)) {
			damage = 0;
			type = 0x0b;
			dmg_lv = ATK_LUCKY;
		}
	}

//	if(def1 >= 1000000 && damage > 0)
	if(t_mode&0x40 && damage > 0)
		damage = 1;

	if(skill_num != CR_GRANDCROSS)
		damage=battle_calc_damage(src,target,damage,div_,skill_num,skill_lv,flag);

	wd.damage = damage;
	wd.damage2 = 0;
	wd.type = type;
	wd.div_ = div_;
	wd.amotion = status_get_amotion(src);
	if (skill_num == KN_AUTOCOUNTER)
		wd.amotion >>= 1;
	wd.dmotion = status_get_dmotion(target);
	wd.blewcount = blewcount;
	wd.flag = flag;
	wd.dmg_lv = dmg_lv;

	return wd;
}

static struct Damage battle_calc_mob_weapon_attack(
	struct block_list *src, struct block_list *target, int skill_num, int skill_lv, int wflag)
{
	struct map_session_data *tsd = NULL;
	struct mob_data* md = (struct mob_data *)src, *tmd = NULL;
	int hitrate, flee, cri = 0, atkmin, atkmax;
	int luk, target_count;
	int def1 = status_get_def(target);
	int def2 = status_get_def2(target);
	int t_vit = status_get_vit(target);
	struct Damage wd;
	int damage, damage2 = 0, type, div_, blewcount = skill_get_blewcount(skill_num, skill_lv);
	int flag, skill, ac_flag = 0, dmg_lv = 0;
	int t_mode = 0, t_race = 0, t_size = 1, s_race = 0, s_ele = 0, s_size = 0;
	struct status_change *sc_data, *t_sc_data;
	short *sc_count;
	short *option, *opt1, *opt2;
	int div_flag = 0; // 0: total damage is to be divided by div_
	                  // 1: damage is distributed,and has to be multiplied by div_ [celest]

	//return前の処理があるので情報出力部のみ変更
	if (src == NULL || target == NULL || md == NULL) {
		memset(&wd, 0, sizeof(wd));
		return wd;
	}

	s_race = status_get_race(src);
	s_ele = status_get_attack_element(src);
	s_size = status_get_size(src);
	sc_data = status_get_sc_data(src);
	sc_count = status_get_sc_count(src);
	option = status_get_option(src);
	opt1 = status_get_opt1(src);
	opt2 = status_get_opt2(src);

	// ターゲット
	if(target->type==BL_PC)
		tsd=(struct map_session_data *)target;
	else if(target->type==BL_MOB)
		tmd=(struct mob_data *)target;
	t_race=status_get_race(target);
	t_size=status_get_size(target);
	t_mode=status_get_mode(target);
	t_sc_data=status_get_sc_data(target);

	if((skill_num == 0 || (target->type == BL_PC && battle_config.pc_auto_counter_type&2) ||
		(target->type == BL_MOB && battle_config.monster_auto_counter_type&2)) && skill_lv >= 0) {
		if(skill_num != CR_GRANDCROSS && t_sc_data && t_sc_data[SC_AUTOCOUNTER].timer != -1) {
			int dir = map_calc_dir(src,target->x,target->y);
			int t_dir = status_get_dir(target);
			int dist = distance(src->x,src->y,target->x,target->y);
			if(dist <= 0 || map_check_dir(dir,t_dir) ) {
				memset(&wd,0,sizeof(wd));
				t_sc_data[SC_AUTOCOUNTER].val3 = 0;
				t_sc_data[SC_AUTOCOUNTER].val4 = 1;
				if(sc_data && sc_data[SC_AUTOCOUNTER].timer == -1) {
					int range = status_get_range(target);
					if((target->type == BL_PC && ((struct map_session_data *)target)->status.weapon != 11 && dist <= range+1) ||
						(target->type == BL_MOB && range <= 3 && dist <= range+1) )
						t_sc_data[SC_AUTOCOUNTER].val3 = src->id;
				}
				return wd;
			} else
				ac_flag = 1;
		} else if(skill_num != CR_GRANDCROSS && t_sc_data && t_sc_data[SC_POISONREACT].timer != -1) {   // poison react [Celest]
			t_sc_data[SC_POISONREACT].val3 = 0;
			t_sc_data[SC_POISONREACT].val4 = 1;
			t_sc_data[SC_POISONREACT].val3 = src->id;
		}
	}
	flag=BF_SHORT|BF_WEAPON|BF_NORMAL;	// 攻撃の種類の設定

	// 回避率計算、回避判定は後で
	flee = status_get_flee(target);
	if(battle_config.agi_penalty_type > 0) {
		target_count = battle_counttargeted(target, src, battle_config.agi_penalty_count_lv) - battle_config.agi_penalty_count;
		if (target_count > 0) {
			if (battle_config.agi_penalty_type == 1)
				flee = (flee * (100 - target_count * battle_config.agi_penalty_num)) / 100;
			else if (battle_config.agi_penalty_type == 2)
				flee -= (target_count * battle_config.agi_penalty_num);
			if (flee < 1) flee = 1;
		}
	}
	hitrate = status_get_hit(src) - flee + 80;

	type = 0; // normal
	if (skill_num > 0) {
		div_ = skill_get_num(skill_num, skill_lv);
		if (div_ < 1) div_ = 1; // Avoid the rare case where the db says div_ is 0 and below
	} else
		div_ = 1; // single attack

	luk = status_get_luk(src);

	if (battle_config.enemy_str)
		damage = status_get_baseatk(src);
	else
		damage = 0;
	if (skill_num == HW_MAGICCRASHER) { /* マジッククラッシャーはMATKで殴る */
		atkmin = status_get_matk1(src);
		atkmax = status_get_matk2(src);
	} else {
		atkmin = status_get_atk(src);
		atkmax = status_get_atk2(src);
	}
	if (mob_db[md->class].range > 3)
		flag=(flag&~BF_RANGEMASK)|BF_LONG;

	if(atkmin > atkmax) atkmin = atkmax;

	if(sc_data != NULL && sc_data[SC_MAXIMIZEPOWER].timer!=-1 ){	// マキシマイズパワー
		atkmin=atkmax;
	}

	cri = status_get_critical(src);
	cri -= status_get_luk(target) * 3;
	if(battle_config.enemy_critical_rate != 100) {
		cri = cri*battle_config.enemy_critical_rate/100;
		if(cri < 1)
			cri = 1;
	}
	if (t_sc_data) {
		if (t_sc_data[SC_SLEEP].timer != -1) // 睡眠中はクリティカルが倍に
			cri <<= 1;
		if (t_sc_data[SC_JOINTBEAT].timer != -1 &&
		    t_sc_data[SC_JOINTBEAT].val2 == 6) // Always take crits with Neck broken by Joint Beat [DracoRPG]
			cri = 1000;
	}

	if(ac_flag) cri = 1000;

	if(skill_num == KN_AUTOCOUNTER) {
		if(!(battle_config.monster_auto_counter_type&1))
			cri = 1000;
		else
			cri <<= 1;
	}

	if(tsd && tsd->critical_def)
		cri = cri * (100 - tsd->critical_def) / 100;

	if((skill_num == 0 || skill_num == KN_AUTOCOUNTER) && skill_lv >= 0 && battle_config.enemy_critical && (rand() % 1000) < cri)	// 判定（スキルの場合は無視）
			// 敵の判定
	{
		damage += atkmax;
		type = 0x0a;
	}
	else {
		int vitbonusmax;

		if(atkmax > atkmin)
			damage += atkmin + rand() % (atkmax-atkmin + 1);
		else
			damage += atkmin;
		// スキル修正１（攻撃力倍化系）
		// オーバートラスト(+5% ～ +25%),他攻撃系スキルの場合ここで補正
		// バッシュ,マグナムブレイク,
		// ボーリングバッシュ,スピアブーメラン,ブランディッシュスピア,スピアスタッブ,
		// メマーナイト,カートレボリューション
		// ダブルストレイフィング,アローシャワー,チャージアロー,
		// ソニックブロー
		if(sc_data){ //状態異常中のダメージ追加
			if(sc_data[SC_OVERTHRUST].timer!=-1)	// オーバートラスト
			damage += damage*(5*sc_data[SC_OVERTHRUST].val1)/100;
			if(sc_data[SC_TRUESIGHT].timer!=-1)	// トゥルーサイト
			damage += damage*(2*sc_data[SC_TRUESIGHT].val1)/100;
			if(sc_data[SC_BERSERK].timer!=-1)	// バーサーク
				damage += damage*2;
			if(sc_data && sc_data[SC_AURABLADE].timer!=-1)	//[DracoRPG]
				damage += sc_data[SC_AURABLADE].val1 * 20;
		}

		if (skill_num > 0) {
			int i;
			if ((i = skill_get_pl(skill_num)) > 0)
				s_ele = i;

			flag = (flag & ~BF_SKILLMASK) | BF_SKILL;
			switch(skill_num) {
			case SM_BASH:		// バッシュ
				damage = damage*(100+ 30*skill_lv)/100;
				hitrate = (hitrate*(100+5*skill_lv))/100;
				break;
			case SM_MAGNUM:		// マグナムブレイク
				damage = damage * (5 * skill_lv + (wflag ? 65 : 115)) / 100;
				hitrate += 10 * skill_lv;
				break;
			case MC_MAMMONITE:	// メマーナイト
				damage = damage*(100+ 50*skill_lv)/100;
				break;
			case AC_DOUBLE:	// ダブルストレイフィング
				damage = damage*(180+ 20*skill_lv)/100;
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				break;
			case AC_SHOWER:	// アローシャワー
				damage = damage*(75 + 5*skill_lv)/100;
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				break;
			case AC_CHARGEARROW:	// チャージアロー
				damage = damage*150/100;
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				break;
			case KN_PIERCE:	// ピアース
				damage = damage * (100 + 10 * skill_lv) / 100;
				hitrate = hitrate * (100 + 5 * skill_lv) / 100;
				div_ = t_size + 1;
				div_flag = 1;
				break;
			case KN_SPEARSTAB:	// スピアスタブ
				damage = damage*(100+ 15*skill_lv)/100;
				break;
			case KN_SPEARBOOMERANG:	// スピアブーメラン
				damage = damage*(100+ 50*skill_lv)/100;
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				break;
			case KN_BRANDISHSPEAR: // ブランディッシュスピア
				damage = damage*(100+ 20*skill_lv)/100;
				if(skill_lv>3 && wflag==1) damage2+=damage/2;
				if(skill_lv>6 && wflag==1) damage2+=damage/4;
				if(skill_lv>9 && wflag==1) damage2+=damage/8;
				if(skill_lv>6 && wflag==2) damage2+=damage/2;
				if(skill_lv>9 && wflag==2) damage2+=damage/4;
				if(skill_lv>9 && wflag==3) damage2+=damage/2;
				damage +=damage2;
				blewcount=0;
				break;
			case KN_BOWLINGBASH:	// ボウリングバッシュ
				damage = damage*(100+ 50*skill_lv)/100;
				blewcount=0;
				break;
			case KN_AUTOCOUNTER:
				if(battle_config.monster_auto_counter_type&1)
					hitrate += 20;
				else
					hitrate = 1000000;
				flag=(flag&~BF_SKILLMASK)|BF_NORMAL;
				break;
			case AS_GRIMTOOTH:
				damage = damage*(100+ 20*skill_lv)/100;
				break;
			case AS_POISONREACT: // celest
				s_ele = 0;
				damage = damage*(100+ 30*skill_lv)/100;
				break;
			case AS_SONICBLOW:	// ソニックブロウ
				damage = damage*(300+ 50*skill_lv)/100;
				break;
			case TF_SPRINKLESAND:	// 砂まき
				damage = damage*125/100;
				break;
			case MC_CARTREVOLUTION:	// カートレボリューション
				damage = (damage*150)/100;
				break;
			// 以下MOB
			case NPC_COMBOATTACK:	// 多段攻撃
				div_flag = 1;
				break;
			case NPC_RANDOMATTACK:	// ランダムATK攻撃
				damage = damage*(50+rand()%150)/100;
				break;
			// 属性攻撃（適当）
			case NPC_WATERATTACK:
			case NPC_GROUNDATTACK:
			case NPC_FIREATTACK:
			case NPC_WINDATTACK:
			case NPC_POISONATTACK:
			case NPC_HOLYATTACK:
			case NPC_DARKNESSATTACK:
			case NPC_UNDEADATTACK:
			case NPC_TELEKINESISATTACK:
				damage = damage*(100+25*(skill_lv-1))/100;
				break;
			case NPC_GUIDEDATTACK:
				hitrate = 1000000;
				break;
			case NPC_RANGEATTACK:
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				break;
			case NPC_PIERCINGATT:
				flag=(flag&~BF_RANGEMASK)|BF_SHORT;
				break;
			case RG_BACKSTAP:	// バックスタブ
				damage = damage*(300+ 40*skill_lv)/100;
				hitrate = 1000000;
				break;
			case RG_RAID:	// サプライズアタック
				damage = damage*(100+ 40*skill_lv)/100;
				break;
			case RG_INTIMIDATE:	// インティミデイト
				damage = damage*(100+ 30*skill_lv)/100;
				break;
			case CR_SHIELDCHARGE:	// シールドチャージ
				damage = damage*(100+ 20*skill_lv)/100;
				flag=(flag&~BF_RANGEMASK)|BF_SHORT;
				s_ele = 0;
				break;
			case CR_SHIELDBOOMERANG:	// シールドブーメラン
				damage = damage * (100 + 30 * skill_lv) / 100;
				flag = (flag&~BF_RANGEMASK)|BF_LONG;
				s_ele = 0;
				break;
			case CR_HOLYCROSS:	// ホーリークロス
				damage = damage * (100 + 35 * skill_lv) / 100;
				break;
			case CR_GRANDCROSS:
				hitrate = 1000000;
				break;
			case AM_DEMONSTRATION:	// デモンストレーション
				hitrate = 1000000;
				damage = damage * (100 + 20 * skill_lv) / 100;
				damage2 = damage2 * (100 + 20 * skill_lv) / 100;
				break;
			case AM_ACIDTERROR:	// アシッドテラー
				hitrate = 1000000;
				damage = damage * (100 + 40 * skill_lv) / 100;
				damage2 = damage2 * (100 + 40 * skill_lv) / 100;
				break;
			case MO_FINGEROFFENSIVE:	//指弾
				damage = damage * (125 + 25 * skill_lv) / 100;
				flag = (flag&~BF_RANGEMASK)|BF_LONG;   //orn
				break;
			case MO_INVESTIGATE:	// 発 勁
				if (def1 < 1000000)
					damage = damage * (100 + 75 * skill_lv) / 100 * (def1 + def2) / 50;
				hitrate = 1000000;
				s_ele = 0;
				break;
			case MO_EXTREMITYFIST:	// 阿修羅覇鳳拳
				damage = damage * 8 + 250 + (skill_lv * 150);
				hitrate = 1000000;
				s_ele = 0;
				break;
			case MO_CHAINCOMBO:	// 連打掌
				damage = damage*(150+ 50*skill_lv)/100;
				break;
			case BA_MUSICALSTRIKE:	// ミュージカルストライク
				damage = damage * (125 + 25 * skill_lv) / 100; // updated damage (thanks to [Mr_KrzYch00] from freya's bug report)
				flag = (flag & ~BF_RANGEMASK) | BF_LONG;
				break;
			case DC_THROWARROW:	// 矢撃ち
				damage = damage * (125 + 25 * skill_lv) / 100; // updated damage (thanks to [Mr_KrzYch00] from freya's bug report)
				flag = (flag & ~BF_RANGEMASK) | BF_LONG;
				break;
			case MO_COMBOFINISH:	// 猛龍拳
				damage = damage * (240 + 60 * skill_lv) / 100;
				break;
			case CH_TIGERFIST: // 伏虎拳
				damage = damage * (40 + 100 * skill_lv) / 100; // kRO patch 12/14/04 - Damage has been increased to 140% ~ 540%.
				break;
			case CH_CHAINCRUSH: // 連柱崩撃
				damage = damage * (400 + 100 * skill_lv) / 100; // kRO patch 12/14/04 - Damage has been increased to 500% ~ 1400%.
				break;
			case CH_PALMSTRIKE: // 猛虎硬派山
				damage = damage * (200 + 100 * skill_lv) / 100; // kRO patch 12/14/04 - SP taken at maximum has been reduced to 10, and damage has been increased to 300% ~ 700%.
				break;
			case LK_SPIRALPIERCE:			/* スパイラルピアース */
				damage = damage * (100 + 50 * skill_lv) / 100; //増加量が分からないので適当に
				flag = (flag & ~BF_RANGEMASK) | BF_LONG;
				if (tsd)
					tsd->canmove_tick = gettick_cache + 1000;
				else if(tmd)
					tmd->canmove_tick = gettick_cache + 1000;
				break;
			case LK_HEADCRUSH:				/* ヘッドクラッシュ */
				damage = damage*(100+ 40*skill_lv)/100;
				break;
			case LK_JOINTBEAT:				/* ジョイントビート */
				damage = damage*(50+ 10*skill_lv)/100;
				break;
			case ASC_METEORASSAULT:			/* メテオアサルト */
				damage = damage*(40+ 40*skill_lv)/100;
				break;
			case SN_SHARPSHOOTING:			/* シャープシューティング */
				damage += damage*(30*skill_lv)/100;
				break;
			case CG_ARROWVULCAN: /* アローバルカン */
				damage = damage * (200 + 100 * skill_lv) / 100;
				break;
			case AS_SPLASHER:		/* ベナムスプラッシャー */
				damage = damage*(200+20*skill_lv)/100;
				break;
			}
			if (div_flag && div_ > 1) { // [Skotlex]
				damage *= div_;
				damage2 *= div_;
			}
		}

		if (skill_num != NPC_CRITICALSLASH) {
			// 対 象の防御力によるダメージの減少
			// ディバインプロテクション（ここでいいのかな？）
			if (skill_num != MO_INVESTIGATE && skill_num != MO_EXTREMITYFIST && skill_num != KN_AUTOCOUNTER && skill_num != KN_AUTOCOUNTER && def1 < 1000000) {	//DEF, VIT無視
				int t_def;
				if (battle_config.vit_penalty_type > 0) {
					target_count = battle_counttargeted(target,src,battle_config.vit_penalty_count_lv) - battle_config.vit_penalty_count;
					if (target_count > 0) {
						if (battle_config.vit_penalty_type == 1) {
							def1 = (def1 * (100 - target_count * battle_config.vit_penalty_num)) / 100;
							def2 = (def2 * (100 - target_count * battle_config.vit_penalty_num)) / 100;
							t_vit = (t_vit * (100 - target_count * battle_config.vit_penalty_num)) / 100;
						} else if (battle_config.vit_penalty_type == 2) {
							def1 -= (target_count * battle_config.vit_penalty_num);
							def2 -= (target_count * battle_config.vit_penalty_num);
							t_vit -= (target_count * battle_config.vit_penalty_num);
						} else if (battle_config.vit_penalty_type == 3) { // (penalize only VIT, not DEF)
							t_vit = (t_vit * (100 - target_count * battle_config.vit_penalty_num)) / 100;
						} else if (battle_config.vit_penalty_type == 4) { // (penalize only VIT, not DEF)
							t_vit -= (target_count * battle_config.vit_penalty_num);
						}
						if (def1 < 0) def1 = 0;
						if (def2 < 1) def2 = 1;
						if (t_vit < 1) t_vit = 1;
					}
				}
				t_def = def2 * 8 / 10;
				if(battle_check_undead(s_race,status_get_elem_type(src)) || s_race==6)
					if(tsd && (skill=pc_checkskill(tsd,AL_DP)) > 0 )
						t_def += skill* (int) (3 + (tsd->status.base_level+1)*0.04);	// submitted by orn
						//t_def += skill*3;

				vitbonusmax = (t_vit/20)*(t_vit/20)-1;
				if (battle_config.monster_defense_type) {
					damage = damage - (def1 * battle_config.monster_defense_type) - t_def - ((vitbonusmax < 1)?0: rand()%(vitbonusmax+1) );
				} else {
					damage = damage * (100 - def1) /100 - t_def - ((vitbonusmax < 1)?0: rand()%(vitbonusmax+1) );
				}
			}
		}
	}

	// 0未満だった場合1に補正
	if(damage<1) damage=1;

	// 回避修正
	if(	hitrate < 1000000 && t_sc_data ) {			// 必中攻撃
		if(t_sc_data[SC_FOGWALL].timer != -1 && flag&BF_LONG)
			hitrate -= 50;
		if (t_sc_data[SC_SLEEP].timer!=-1 ||	// 睡眠は必中
			t_sc_data[SC_STAN].timer!=-1 ||		// スタンは必中
			t_sc_data[SC_FREEZE].timer!=-1 ||
			(t_sc_data[SC_STONE].timer!=-1 && t_sc_data[SC_STONE].val2==0))	// 凍結は必中
			hitrate = 1000000;
	}
	if (hitrate < 1000000)
		hitrate = ((hitrate > 95) ? 95: ((hitrate < 5) ? 5 : hitrate));
	if (type == 0 && rand() % 100 >= hitrate) {
		damage = damage2 = 0;
		dmg_lv = ATK_FLEE;
	} else {
		dmg_lv = ATK_DEF;
	}

	if (tsd) {
		int cardfix = 100, i;
		cardfix = cardfix * (100 - tsd->subele[s_ele]) / 100; // 属 性によるダメージ耐性
		cardfix = cardfix * (100 - tsd->subrace[s_race]) / 100; // 種族によるダメージ耐性
		cardfix = cardfix * (100 - tsd->subsize[s_size]) / 100;
		if (mob_db[md->class].mode & 0x20)
			cardfix = cardfix * (100 - tsd->subrace[10]) / 100;
		else
			cardfix = cardfix * (100 - tsd->subrace[11]) / 100;
		for(i = 0; i < tsd->add_def_class_count; i++) {
			if (tsd->add_def_classid[i] == md->class) {
				cardfix = cardfix * (100 - tsd->add_def_classrate[i]) / 100;
				break;
			}
		}
		for(i = 0; i < tsd->add_damage_class_count2; i++) {
			if (tsd->add_damage_classid2[i] == md->class) {
				cardfix = cardfix * (100 + tsd->add_damage_classrate2[i]) / 100;
				break;
			}
		}
		if (flag & BF_LONG)
			cardfix = cardfix * (100 - tsd->long_attack_def_rate) / 100;
		if (flag & BF_SHORT)
			cardfix = cardfix * (100 - tsd->near_attack_def_rate) / 100;
		damage = damage * cardfix / 100;
	}
	if(t_sc_data) {
		int cardfix=100;
		if(t_sc_data[SC_DEFENDER].timer != -1 && flag&BF_LONG)
			cardfix=cardfix*(100-t_sc_data[SC_DEFENDER].val2)/100;
		if(t_sc_data[SC_FOGWALL].timer != -1 && flag&BF_LONG)
			cardfix=cardfix*(100-t_sc_data[SC_FOGWALL].val2)/100;
		if(cardfix != 100)
			damage=damage*cardfix/100;
	}
	if(t_sc_data && t_sc_data[SC_ASSUMPTIO].timer != -1){ //アシャンプティオ
		if(map[target->m].flag.pvp || map[target->m].flag.gvg)
			damage = damage * 2 / 3;
		else
			damage = damage / 2;
	}

	if(damage < 0) damage = 0;

	// 属 性の適用
	if (!((battle_config.mob_ghostring_fix == 1) &&
	    (status_get_elem_type(target) == 8) && // fix from Celest (Freya's forum)
	    (target->type==BL_PC))) // [MouseJstr]
	if(skill_num != 0 || s_ele != 0 || !battle_config.mob_attack_attr_none)
	damage=battle_attr_fix(damage, s_ele, status_get_element(target) );

	//if(sc_data && sc_data[SC_AURABLADE].timer!=-1)	/* オーラブレード 必中 */
	//	damage += sc_data[SC_AURABLADE].val1 * 10;
	if(skill_num==PA_PRESSURE) /* プレッシャー 必中? */
		damage = 500+300*skill_lv;

	// インベナム修正
	if(skill_num==TF_POISON){
		damage = battle_attr_fix(damage + 15*skill_lv, s_ele, status_get_element(target) );
	}
	if(skill_num==MC_CARTREVOLUTION){
		damage = battle_attr_fix(damage, 0, status_get_element(target) );
	}

	// 完全回避の判定
	if (skill_num == 0 && tsd != NULL && div_ < 255 && rand() % 1000 < status_get_flee2(target) ){
		damage = 0;
		type = 0x0b;
		dmg_lv = ATK_LUCKY;
	}

	if(battle_config.enemy_perfect_flee) {
		if(skill_num == 0 && tmd!=NULL && div_ < 255 && rand()%1000 < status_get_flee2(target) ){
			damage=0;
			type=0x0b;
			dmg_lv = ATK_LUCKY;
		}
	}

//	if (def1 >= 1000000 && damage > 0)
	if (t_mode & 0x40 && damage > 0)
		damage = 1;

	if (tsd && tsd->special_state.no_weapon_damage)
		damage = 0;

	if (skill_num != CR_GRANDCROSS)
		damage=battle_calc_damage(src, target, damage, div_, skill_num, skill_lv, flag);

	wd.damage = damage;
	wd.damage2 = 0;
	wd.type = type;
	wd.div_ = div_;
	wd.amotion = status_get_amotion(src);
	if (skill_num == KN_AUTOCOUNTER)
		wd.amotion >>= 1;
	wd.dmotion = status_get_dmotion(target);
	wd.blewcount = blewcount;
	wd.flag = flag;
	wd.dmg_lv = dmg_lv;

	return wd;
}

/*
 * =========================================================================
 * PCの武器による攻撃
 *-------------------------------------------------------------------------
 */
static struct Damage battle_calc_pc_weapon_attack(
	struct block_list *src, struct block_list *target, int skill_num, int skill_lv, int wflag)
{
	struct map_session_data *sd = (struct map_session_data *)src, *tsd = NULL;
	struct mob_data *tmd = NULL;
	int hitrate, flee, cri = 0, atkmin, atkmax;
	int dex, luk, target_count;
	int no_cardfix = 0;
	int def1 = status_get_def(target);
	int def2 = status_get_def2(target);
//	int mdef1, mdef2;
	int t_vit = status_get_vit(target);
	struct Damage wd;
	int damage, damage2, type, div_, blewcount = skill_get_blewcount(skill_num, skill_lv);
	int flag, skill, dmg_lv = 0;
	int t_mode = 0, t_race = 0, t_size = 1, s_race = 7, s_ele = 0, s_size = 0;
	int t_race2 = 0;
	struct status_change *sc_data, *t_sc_data;
	short *sc_count;
	short *option, *opt1, *opt2;
	int atkmax_ = 0, atkmin_ = 0, s_ele_; //二刀流用
	int watk,watk_, cardfix, t_ele;
	int da = 0, i, t_class, ac_flag = 0;
	int idef_flag = 0, idef_flag_ = 0;
	int div_flag = 0; // 0: total damage is to be divided by div_
	                  // 1: damage is distributed,and has to be multiplied by div_ [celest]
	int hit_emp_flag = 1; // 1: should hit emperium, 0: should not hit emperium (Skill Sacrifice Strike/Ritual Sacrifice corrections of [Gawaine])
	int class; // (Skill Sacrifice Strike/Ritual Sacrifice corrections of [Gawaine])

	//return前の処理があるので情報出力部のみ変更
	if (src == NULL || target == NULL || sd == NULL) {
		memset(&wd, 0, sizeof(wd));
		return wd;
	}

	// アタッカー
	s_race = status_get_race(src); //種族
	s_ele = status_get_attack_element(src); //属性
	s_ele_ = status_get_attack_element2(src); //左手属性
	s_size = status_get_size(src);
	sc_data = status_get_sc_data(src); //ステータス異常
	sc_count = status_get_sc_count(src); //ステータス異常の数
	option = status_get_option(src); //鷹とかペコとかカートとか
	opt1 = status_get_opt1(src); //石化、凍結、スタン、睡眠、暗闇
	opt2 = status_get_opt2(src); //毒、呪い、沈黙、暗闇？
	t_race2 = status_get_race2(target);
	class = status_get_class(target); // (Skill Sacrifice Strike/Ritual Sacrifice corrections of [Gawaine])

	if (skill_num != CR_GRANDCROSS) //グランドクロスでないなら
		sd->state.attack_type = BF_WEAPON; //攻撃タイプは武器攻撃

	// ターゲット
	if (target->type == BL_PC) //対象がPCなら
		tsd = (struct map_session_data *)target; //tsdに代入(tmdはNULL)
	else if (target->type == BL_MOB) //対象がMobなら
		tmd = (struct mob_data *)target; //tmdに代入(tsdはNULL)
	t_race = status_get_race(target); //対象の種族
	t_ele = status_get_elem_type(target); //対象の属性
	t_size = status_get_size(target); //対象のサイズ
	t_mode = status_get_mode(target); //対象のMode
	t_sc_data = status_get_sc_data(target); //対象のステータス異常

//オートカウンター処理ここから
	if((skill_num == 0 || (target->type == BL_PC && battle_config.pc_auto_counter_type&2) ||
		(target->type == BL_MOB && battle_config.monster_auto_counter_type&2)) && skill_lv >= 0) {
		if(skill_num != CR_GRANDCROSS && t_sc_data && t_sc_data[SC_AUTOCOUNTER].timer != -1) { //グランドクロスでなく、対象がオートカウンター状態の場合
			int dir = map_calc_dir(src,target->x,target->y);
			int t_dir = status_get_dir(target);
			int dist = distance(src->x,src->y,target->x,target->y);
			if(dist <= 0 || map_check_dir(dir,t_dir) ) { //対象との距離が0以下、または対象の正面？
				memset(&wd,0,sizeof(wd));
				t_sc_data[SC_AUTOCOUNTER].val3 = 0;
				t_sc_data[SC_AUTOCOUNTER].val4 = 1;
				if(sc_data && sc_data[SC_AUTOCOUNTER].timer == -1) { //自分がオートカウンター状態
					int range = status_get_range(target);
					if((target->type == BL_PC && ((struct map_session_data *)target)->status.weapon != 11 && dist <= range+1) || //対象がPCで武器が弓矢でなく射程内
						(target->type == BL_MOB && range <= 3 && dist <= range+1) ) //または対象がMobで射程が3以下で射程内
						t_sc_data[SC_AUTOCOUNTER].val3 = src->id;
				}
				return wd; //ダメージ構造体を返して終了
			} else
				ac_flag = 1;
		} else if(skill_num != CR_GRANDCROSS && t_sc_data && t_sc_data[SC_POISONREACT].timer != -1) {   // poison react [Celest]
			t_sc_data[SC_POISONREACT].val3 = 0;
			t_sc_data[SC_POISONREACT].val4 = 1;
			t_sc_data[SC_POISONREACT].val3 = src->id;
		}
	}
//オートカウンター処理ここまで

	flag=BF_SHORT|BF_WEAPON|BF_NORMAL;	// 攻撃の種類の設定

	// 回避率計算、回避判定は後で
	flee = status_get_flee(target);
	if(battle_config.agi_penalty_type > 0) { //AGI、VITペナルティ設定が有効
		target_count = battle_counttargeted(target, src, battle_config.agi_penalty_count_lv) - battle_config.agi_penalty_count; //対象の数を算出
		if (target_count > 0) { //ペナルティ設定より対象が多い
			if (battle_config.agi_penalty_type == 1) //回避率がagi_penalty_num%ずつ減少
				flee = (flee * (100 - target_count * battle_config.agi_penalty_num)) / 100;
			else if (battle_config.agi_penalty_type == 2) //回避率がagi_penalty_num分減少
				flee -= target_count * battle_config.agi_penalty_num;
			if (flee < 1) flee = 1; //回避率は最低でも1
		}
	}
	hitrate = status_get_hit(src) - flee + 80; //命中率計算

	type = 0; // normal
	if (skill_num > 0) {
		div_ = skill_get_num(skill_num, skill_lv);
		if (div_ < 1) div_ = 1; // Avoid the rare case where the db says div_ is 0 and below
	} else
		div_ = 1; // single attack

	dex = status_get_dex(src); //DEX
	luk = status_get_luk(src); //LUK
	watk = status_get_atk(src); //ATK
	watk_ = status_get_atk_(src); //ATK左手

	if (skill_num==HW_MAGICCRASHER) { /* マジッククラッシャーはMATKで殴る */
		damage = damage2 = status_get_matk1(src); //damega,damega2初登場、base_atkの取得
	} else {
		damage = damage2 = status_get_baseatk(&sd->bl); //damega,damega2初登場、base_atkの取得
	}
	atkmin = atkmin_ = dex; //最低ATKはDEXで初期化？
	sd->state.arrow_atk = 0; //arrow_atk初期化
	if(sd->equip_index[9] >= 0 && sd->inventory_data[sd->equip_index[9]])
		atkmin = atkmin*(80 + sd->inventory_data[sd->equip_index[9]]->wlv*20)/100;
	if(sd->equip_index[8] >= 0 && sd->inventory_data[sd->equip_index[8]])
		atkmin_ = atkmin_*(80 + sd->inventory_data[sd->equip_index[8]]->wlv*20)/100;
	if(sd->status.weapon == 11) { //武器が弓矢の場合
		atkmin = watk * ((atkmin<watk)? atkmin:watk)/100; //弓用最低ATK計算
		flag=(flag&~BF_RANGEMASK)|BF_LONG; //遠距離攻撃フラグを有効
		if(sd->arrow_ele > 0) //属性矢なら属性を矢の属性に変更
			s_ele = sd->arrow_ele;
		sd->state.arrow_atk = 1; //arrow_atk有効化
	}

	// サイズ修正
	// ペコ騎乗していて、槍で攻撃した場合は中型のサイズ修正を100にする
	// ウェポンパーフェクション,ドレイクC
	if(((sd->special_state.no_sizefix) || (pc_isriding(sd) && (sd->status.weapon==4 || sd->status.weapon==5) && t_size==1) || skill_num == MO_EXTREMITYFIST)){	//ペコ騎乗していて、槍で中型を攻撃
		atkmax = watk;
		atkmax_ = watk_;
	} else {
		atkmax = (watk * sd->atkmods[ t_size ]) / 100;
		atkmin = (atkmin * sd->atkmods[ t_size ]) / 100;
			atkmax_ = (watk_ * sd->atkmods_[ t_size ]) / 100;
		atkmin_ = (atkmin_ * sd->atkmods[ t_size ]) / 100;
	}
	if( (sc_data != NULL && sc_data[SC_WEAPONPERFECTION].timer!=-1) || (sd->special_state.no_sizefix)) {	// ウェポンパーフェクション || ドレイクカード
		atkmax = watk;
		atkmax_ = watk_;
	}

	if(atkmin > atkmax && !(sd->state.arrow_atk)) atkmin = atkmax;	//弓は最低が上回る場合あり
	if(atkmin_ > atkmax_) atkmin_ = atkmax_;

	if(sc_data != NULL && sc_data[SC_MAXIMIZEPOWER].timer!=-1 ){	// マキシマイズパワー
		atkmin=atkmax;
		atkmin_=atkmax_;
	}

	// Higher level between Sidewinder Card/Nagan & Double Attack skill should be used
	// Thanks to [akrus] for the condensed code posted on freya's bug report (replace following (commented) code)
	// Monk Skills overpowered fixed by [BeoWulf] (posted on freya's bug report)
	if (skill_num == 0 && (((skill = 5 * pc_checkskill(sd, TF_DOUBLE)) > 0 && sd->weapontype1 == 0x01) || sd->double_rate > 0))
		da = (rand() % 100 < (skill > sd->double_rate ? skill:sd->double_rate)) ? 1 : 0;
	if (skill_num == 0 && (skill = pc_checkskill(sd, MO_TRIPLEATTACK)) > 0 && sd->status.weapon <= 16)
		da = (rand() % 100 < (30 - skill)) ? 2 : da;

/*	//ダブルアタック判定
	if(sd->weapontype1 == 0x01) {
		if(skill_num == 0 && skill_lv >= 0 && (skill = pc_checkskill(sd,TF_DOUBLE)) > 0)
			da = (rand()%100 < (skill*5)) ? 1:0;
	}

	//三段掌
	//if(skill_num == 0 && skill_lv >= 0 && (skill = pc_checkskill(sd,MO_TRIPLEATTACK)) > 0 && sd->status.weapon <= 16 && !sd->state.arrow_atk) {
	if (skill_num == 0 && (skill = pc_checkskill(sd, MO_TRIPLEATTACK)) > 0 && sd->status.weapon <= 16) { // triple blow works with bows ^^ [celest]
		da = (rand() % 100 < (30 - skill)) ? 2 : 0;
	}

	if(sd->double_rate > 0 && da == 0 && skill_num == 0)
		da = (rand() % 100 < sd->double_rate) ? 1 : 0;*/

	// 過剰精錬ボーナス
	if (sd->overrefine > 0)
		damage += (rand() % sd->overrefine) + 1;
	if (sd->overrefine_ > 0)
		damage2 += (rand() % sd->overrefine_) + 1;

	if (da == 0) { //ダブルアタックが発動していない
		// クリティカル計算
		cri = status_get_critical(src);
		cri += sd->critaddrace[t_race];

		if (sd->state.arrow_atk)
			cri += sd->arrow_cri;
		if (sd->status.weapon == 16)
				// カタールの場合、クリティカルを倍に
			cri <<= 1;
		cri -= status_get_luk(target) * 3;
		if (t_sc_data) {
			if (t_sc_data[SC_SLEEP].timer != -1) // 睡眠中はクリティカルが倍に
				cri <<= 1;
			if (t_sc_data[SC_JOINTBEAT].timer != -1 &&
			    t_sc_data[SC_JOINTBEAT].val2 == 6) // Always take crits with Neck broken by Joint Beat [DracoRPG]
				cri = 1000;
		}
		if (ac_flag)
			cri = 1000;

		if (skill_num == KN_AUTOCOUNTER) {
			if (!(battle_config.pc_auto_counter_type & 1))
				cri = 1000;
			else
				cri <<= 1;
		} else if (skill_num == SN_SHARPSHOOTING)
			cri += 200;
	}

	if (tsd && tsd->critical_def)
		cri = cri * (100 - tsd->critical_def) / 100;

	if (da == 0 && (skill_num == 0 || skill_num == KN_AUTOCOUNTER || skill_num == SN_SHARPSHOOTING) && //ダブルアタックが発動していない
	    (rand() % 1000) < cri) // 判定（スキルの場合は無視）
	{
		damage += atkmax;
		damage2 += atkmax_;
		if (sd->atk_rate != 100 || sd->weapon_atk_rate != 0) {
			if (sd->status.weapon < 16) {
				damage = (damage * (sd->atk_rate + sd->weapon_atk_rate[sd->status.weapon])) / 100;
				damage2 = (damage2 * (sd->atk_rate + sd->weapon_atk_rate[sd->status.weapon])) / 100;
			}
		}
		if (sd->state.arrow_atk)
			damage += sd->arrow_atk;

		damage += damage * sd->crit_atk_rate / 100;

		type = 0x0a;

		// EDP (Deadly Potion Enhancement) on critical attack effect, extracted from [Gawaine Fix]
		if (!no_cardfix && sc_data[SC_EDP].timer != -1 && skill_num != ASC_BREAKER && skill_num != ASC_METEORASSAULT) {
			damage += damage * (150 + sc_data[SC_EDP].val1 * 50) / 100;
			no_cardfix = 1;
		}
		// EDP (Deadly Potion Enhancement) on critical attack effect, extracted from [Gawaine Fix]

	} else {
		int vitbonusmax;

		if(atkmax > atkmin)
			damage += atkmin + rand() % (atkmax-atkmin + 1);
		else
			damage += atkmin;
		if(atkmax_ > atkmin_)
			damage2 += atkmin_ + rand() % (atkmax_-atkmin_ + 1);
		else
			damage2 += atkmin_;
		if (sd->atk_rate != 100 || sd->weapon_atk_rate != 0) {
			if (sd->status.weapon < 16) {
				damage = (damage * (sd->atk_rate + sd->weapon_atk_rate[sd->status.weapon])) / 100;
				damage2 = (damage2 * (sd->atk_rate + sd->weapon_atk_rate[sd->status.weapon])) / 100;
			}
		}

		if(sd->state.arrow_atk) {
			if(sd->arrow_atk > 0)
				damage += rand()%(sd->arrow_atk+1);
			hitrate += sd->arrow_hit;
		}

		if (skill_num != MO_INVESTIGATE && def1 < 1000000) {
			if (sd->def_ratio_atk_ele & (1<<t_ele) || sd->def_ratio_atk_race & (1<<t_race)) {
				damage = (damage * (def1 + def2)) / 100;
				idef_flag = 1;
			}
			if (sd->def_ratio_atk_ele_ & (1<<t_ele) || sd->def_ratio_atk_race_ & (1<<t_race)) {
				damage2 = (damage2 * (def1 + def2)) / 100;
				idef_flag_ = 1;
			}
			if(t_mode & 0x20) {
				if(!idef_flag && sd->def_ratio_atk_race & (1<<10)) {
					damage = (damage * (def1 + def2)) / 100;
					idef_flag = 1;
				}
				if(!idef_flag_ && sd->def_ratio_atk_race_ & (1<<10)) {
					damage2 = (damage2 * (def1 + def2)) / 100;
					idef_flag_ = 1;
				}
			}
			else {
				if(!idef_flag && sd->def_ratio_atk_race & (1<<11)) {
					damage = (damage * (def1 + def2)) / 100;
					idef_flag = 1;
				}
				if(!idef_flag_ && sd->def_ratio_atk_race_ & (1<<11)) {
					damage2 = (damage2 * (def1 + def2)) / 100;
					idef_flag_ = 1;
				}
			}
		}

		// スキル修正１（攻撃力倍化系）
		// オーバートラスト(+5% ～ +25%),他攻撃系スキルの場合ここで補正
		// バッシュ,マグナムブレイク,
		// ボーリングバッシュ,スピアブーメラン,ブランディッシュスピア,スピアスタッブ,
		// メマーナイト,カートレボリューション
		// ダブルストレイフィング,アローシャワー,チャージアロー,
		// ソニックブロー
		if(sc_data){ //状態異常中のダメージ追加
			if(sc_data[SC_OVERTHRUST].timer!=-1){ // オーバートラスト
			damage += damage*(5*sc_data[SC_OVERTHRUST].val1)/100;
			damage2 += damage2*(5*sc_data[SC_OVERTHRUST].val1)/100;
		}
			if(sc_data[SC_TRUESIGHT].timer!=-1){ // トゥルーサイト
			damage += damage*(2*sc_data[SC_TRUESIGHT].val1)/100;
			damage2 += damage2*(2*sc_data[SC_TRUESIGHT].val1)/100;
		}
			if(sc_data[SC_BERSERK].timer!=-1){ // バーサーク
				damage += damage*2;
				damage2 += damage2*2;
			}
			if(sc_data && sc_data[SC_AURABLADE].timer!=-1) {
				damage += sc_data[SC_AURABLADE].val1 * 20;
				damage2 += sc_data[SC_AURABLADE].val1 * 20;
			}
		}

		if(skill_num>0){
			if( (i=skill_get_pl(skill_num))>0 )
				s_ele=s_ele_=i;

			flag=(flag&~BF_SKILLMASK)|BF_SKILL;
			switch( skill_num ){
			case SM_BASH:		// バッシュ
				damage = damage*(100+ 30*skill_lv)/100;
				damage2 = damage2*(100+ 30*skill_lv)/100;
				hitrate = (hitrate*(100+5*skill_lv))/100;
				break;
			case SM_MAGNUM:		// マグナムブレイク
				damage = damage * (5 * skill_lv + (wflag ? 65 : 115)) / 100;
				damage2 = damage2 * (5 * skill_lv + (wflag ? 65 : 115)) / 100;
				break;
			case MC_MAMMONITE:	// メマーナイト
				damage = damage*(100+ 50*skill_lv)/100;
				damage2 = damage2*(100+ 50*skill_lv)/100;
				break;
			case AC_DOUBLE:	// ダブルストレイフィング
				if(!sd->state.arrow_atk && sd->arrow_atk > 0) {
					int arr = rand()%(sd->arrow_atk+1);
					damage += arr;
					damage2 += arr;
				}
				damage = damage*(180+ 20*skill_lv)/100;
				damage2 = damage2*(180+ 20*skill_lv)/100;
				if(sd->arrow_ele > 0) {
					s_ele = sd->arrow_ele;
					s_ele_ = sd->arrow_ele;
				}
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				sd->state.arrow_atk = 1;
				break;
			case AC_SHOWER:	// アローシャワー
				if(!sd->state.arrow_atk && sd->arrow_atk > 0) {
					int arr = rand()%(sd->arrow_atk+1);
					damage += arr;
					damage2 += arr;
				}
				damage = damage*(75 + 5*skill_lv)/100;
				damage2 = damage2*(75 + 5*skill_lv)/100;
				if(sd->arrow_ele > 0) {
					s_ele = sd->arrow_ele;
					s_ele_ = sd->arrow_ele;
				}
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				sd->state.arrow_atk = 1;
				break;
			case AC_CHARGEARROW:	// チャージアロー
				if(!sd->state.arrow_atk && sd->arrow_atk > 0) {
					int arr = rand()%(sd->arrow_atk+1);
					damage += arr;
					damage2 += arr;
				}
				damage = damage*150/100;
				damage2 = damage2*150/100;
				if(sd->arrow_ele > 0) {
					s_ele = sd->arrow_ele;
					s_ele_ = sd->arrow_ele;
				}
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				sd->state.arrow_atk = 1;
				break;
			case KN_PIERCE:	// ピアース
				damage = damage * (100 + 10 * skill_lv) / 100;
				damage2 = damage2 * (100 + 10 * skill_lv) / 100;
				hitrate = hitrate * (100 + 5 * skill_lv) / 100;
				div_ = t_size + 1;
				div_flag = 1;
				break;
			case KN_SPEARSTAB:	// スピアスタブ
				damage = damage*(100+ 15*skill_lv)/100;
				damage2 = damage2*(100+ 15*skill_lv)/100;
				break;
			case KN_SPEARBOOMERANG:	// スピアブーメラン
				damage = damage*(100+ 50*skill_lv)/100;
				damage2 = damage2*(100+ 50*skill_lv)/100;
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				break;
			case KN_BRANDISHSPEAR: // ブランディッシュスピア
			  {
				int damage3 = 0;
				damage = damage * (100 + 20 * skill_lv) / 100;
				damage2 = damage2 * (100 + 20 * skill_lv) / 100;
				if (skill_lv > 3 && wflag == 1) damage3 += damage / 2;
				if (skill_lv > 6 && wflag == 1) damage3 += damage / 4;
				if (skill_lv > 9 && wflag == 1) damage3 += damage / 8;
				if (skill_lv > 6 && wflag == 2) damage3 += damage / 2;
				if (skill_lv > 9 && wflag == 2) damage3 += damage / 4;
				if (skill_lv > 9 && wflag == 3) damage3 += damage / 2;
				damage += damage3;
				damage3 = 0;
				if (skill_lv > 3 && wflag == 1) damage3 += damage2 / 2;
				if (skill_lv > 6 && wflag == 1) damage3 += damage2 / 4;
				if (skill_lv > 9 && wflag == 1) damage3 += damage2 / 8;
				if (skill_lv > 6 && wflag == 2) damage3 += damage2 / 2;
				if (skill_lv > 9 && wflag == 2) damage3 += damage2 / 4;
				if (skill_lv > 9 && wflag == 3) damage3 += damage2 / 2;
				damage2 += damage3;
				blewcount = 0;
			  }
				break;
			case KN_BOWLINGBASH:	// ボウリングバッシュ
				damage = damage*(100+ 50*skill_lv)/100;
				damage2 = damage2*(100+ 50*skill_lv)/100;
				blewcount=0;
				break;
			case KN_AUTOCOUNTER:
				if(battle_config.pc_auto_counter_type&1)
					hitrate += 20;
				else
					hitrate = 1000000;
				flag=(flag&~BF_SKILLMASK)|BF_NORMAL;
				break;
			case AS_GRIMTOOTH:
				damage = damage*(100+ 20*skill_lv)/100;
				damage2 = damage2*(100+ 20*skill_lv)/100;
				break;
			case AS_POISONREACT: // celest
				s_ele = 0;
				damage = damage*(100+ 30*skill_lv)/100;
				damage2 = damage2*(100+ 30*skill_lv)/100;
				break;
			case AS_SONICBLOW:	// ソニックブロウ
				hitrate += 30; // hitrate +30, thanks to midas
				damage = damage*(300+ 50*skill_lv)/100;
				damage2 = damage2*(300+ 50*skill_lv)/100;
				break;
			case TF_SPRINKLESAND:	// 砂まき
				damage = damage*125/100;
				damage2 = damage2*125/100;
				break;
			case MC_CARTREVOLUTION:	// カートレボリューション
				if (sd->cart_max_weight > 0 && sd->cart_weight > 0) {
					damage = (damage * (1.5 + sd->cart_weight / sd->cart_max_weight));	// fixed CARTREV damage [Raversth] (from freya-forum)
					damage2 = (damage2 * (1.5 + sd->cart_weight / sd->cart_max_weight));
					//damage = ( damage*(150 + sd->cart_weight/800) )/100;	//fixed CARTREV damage [Lupus] // should be 800, not 80... weight is *10 ^_- [celest]
					//damage2 = ( damage2*(150 + sd->cart_weight/800) )/100;
					//damage = (damage*(150 + pc_checkskill(sd,BS_WEAPONRESEARCH) + (sd->cart_weight*100/sd->cart_max_weight) ) )/100;
					//damage2 = (damage2*(150 + pc_checkskill(sd,BS_WEAPONRESEARCH) + (sd->cart_weight*100/sd->cart_max_weight) ) )/100;
				}
				else {
					damage = (damage*150)/100;
					damage2 = (damage2*150)/100;
				}
				break;
			case WS_CARTTERMINATION:
				if(sd && sd->cart_max_weight > 0 && sd->cart_weight > 0) {
					double weight = (8000.*sd->cart_weight)/sd->cart_max_weight;
					damage = (int)(damage*(weight/(16-skill_lv)/100));
					damage2 = (int)(damage2*(weight/(16-skill_lv)/100));
				}
				else if (!sd) {
					damage = (damage*100)/100;
					damage2 = (damage2*100)/100;
				}
				no_cardfix = 1;
				break;
			// 以下MOB
			case NPC_COMBOATTACK:	// 多段攻撃
				div_flag = 1;
				break;
			case NPC_RANDOMATTACK:	// ランダムATK攻撃
				damage = damage*(50+rand()%150)/100;
				damage2 = damage2*(50+rand()%150)/100;
				break;
			// 属性攻撃（適当）
			case NPC_WATERATTACK:
			case NPC_GROUNDATTACK:
			case NPC_FIREATTACK:
			case NPC_WINDATTACK:
			case NPC_POISONATTACK:
			case NPC_HOLYATTACK:
			case NPC_DARKNESSATTACK:
			case NPC_UNDEADATTACK:
			case NPC_TELEKINESISATTACK:
				damage = damage*(100+25*skill_lv)/100;
				damage2 = damage2*(100+25*skill_lv)/100;
				break;
			case NPC_GUIDEDATTACK:
				hitrate = 1000000;
				break;
			case NPC_RANGEATTACK:
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				break;
			case NPC_PIERCINGATT:
				flag=(flag&~BF_RANGEMASK)|BF_SHORT;
				break;
			case RG_BACKSTAP:	// バックスタブ
				if(battle_config.backstab_bow_penalty == 1 && sd->status.weapon == 11){
					damage = (damage*(300+ 40*skill_lv)/100)/2;
					damage2 = (damage2*(300+ 40*skill_lv)/100)/2;
				}else{
					damage = damage*(300+ 40*skill_lv)/100;
					damage2 = damage2*(300+ 40*skill_lv)/100;
				}
				hitrate = 1000000;
				break;
			case RG_RAID:	// サプライズアタック
				damage = damage*(100+ 40*skill_lv)/100;
				damage2 = damage2*(100+ 40*skill_lv)/100;
				break;
			case RG_INTIMIDATE:	// インティミデイト
				damage = damage*(100+ 30*skill_lv)/100;
				damage2 = damage2*(100+ 30*skill_lv)/100;
				break;
			case CR_SHIELDCHARGE:	// シールドチャージ
				damage = damage*(100+ 20*skill_lv)/100;
				damage2 = damage2*(100+ 20*skill_lv)/100;
				flag=(flag&~BF_RANGEMASK)|BF_SHORT;
				s_ele = 0;
				break;
			case CR_SHIELDBOOMERANG:	// シールドブーメラン
				damage = damage*(100+ 30*skill_lv)/100;
				damage2 = damage2*(100+ 30*skill_lv)/100;
				flag=(flag&~BF_RANGEMASK)|BF_LONG;
				s_ele = 0;
				break;
			case CR_HOLYCROSS:	// ホーリークロス
				damage = damage * (100 + 35 * skill_lv) / 100;
				damage2 = damage2 * (100 + 35 * skill_lv) / 100;
				break;
			case CR_GRANDCROSS:
				hitrate = 1000000;
				if (!battle_config.gx_cardfix)
					no_cardfix = 1;
				break;
			case PA_SHIELDCHAIN:
				damage = damage * (100 + 10 * skill_lv) / 100;
				damage2 = damage2 * (100 + 10 * skill_lv) / 100;
				hitrate += 20;
				div_ = 5;
				flag = (flag&~BF_RANGEMASK)|BF_LONG;
				s_ele = 0;
				break;
			case AM_DEMONSTRATION:	// デモンストレーション
				damage = damage * (100 + 20 * skill_lv) / 100;
				damage2 = damage2 * (100 + 20 * skill_lv) / 100;
				no_cardfix = 1;
				break;
			case AM_ACIDTERROR:	// アシッドテラー
				hitrate = 1000000;
				damage = damage * (100 + 40 * skill_lv) / 100;
				damage2 = damage2 * (100 + 40 * skill_lv) / 100;
				s_ele = 0;
				s_ele_ = 0;
				no_cardfix = 1;
				break;
			case MO_FINGEROFFENSIVE:	//指弾
				damage = damage * (125 + 25 * skill_lv) / 100;
				damage2 = damage2 * (125 + 25 * skill_lv) / 100;
				if (battle_config.finger_offensive_type == 0) {
					div_ = sd->spiritball_old;
					div_flag = 1;
				} else
					div_ = 1;
				flag = (flag & ~BF_RANGEMASK) | BF_LONG; //orn
				break;
			case MO_INVESTIGATE:	// 発 勁
				if (def1 < 1000000) {
					damage = damage * (100 + 75 * skill_lv) / 100 * (def1 + def2) / 50;
					damage2 = damage2 * (100 + 75 * skill_lv) / 100 * (def1 + def2) / 50;
				}
				hitrate = 1000000;
				s_ele = 0;
				s_ele_ = 0;
				break;
			case MO_EXTREMITYFIST:	// 阿修羅覇鳳拳
				damage = damage * (8 + ((sd->status.sp)/10)) + 250 + (skill_lv * 150);
				damage2 = damage2 * (8 + ((sd->status.sp)/10)) + 250 + (skill_lv * 150);
				sd->status.sp = 0;
				clif_updatestatus(sd,SP_SP);
				hitrate = 1000000;
				s_ele = 0;
				s_ele_ = 0;
				break;
			case MO_CHAINCOMBO:	// 連打掌
				damage = damage*(150+ 50*skill_lv)/100;
				damage2 = damage2*(150+ 50*skill_lv)/100;
				break;
			case MO_COMBOFINISH:	// 猛龍拳
				damage = damage*(240+ 60*skill_lv)/100;
				damage2 = damage2*(240+ 60*skill_lv)/100;
				break;
			case BA_MUSICALSTRIKE:	// ミュージカルストライク
				damage = damage * (125 + 25 * skill_lv) / 100; // updated damage (thanks to [Mr_KrzYch00] from freya's bug report)
				damage2 = damage2 * (125 + 25 * skill_lv) / 100; // updated damage (thanks to [Mr_KrzYch00] from freya's bug report)
				if (sd->arrow_ele > 0) {
					s_ele = sd->arrow_ele;
					s_ele_ = sd->arrow_ele;
				}
				flag = (flag & ~BF_RANGEMASK) | BF_LONG;
				break;
			case DC_THROWARROW:	// 矢撃ち
				if(!sd->state.arrow_atk && sd->arrow_atk > 0) {
					int arr = rand()%(sd->arrow_atk+1);
					damage += arr;
					damage2 += arr;
				}
				damage = damage * (125+ 25 * skill_lv) / 100; // updated damage (thanks to [Mr_KrzYch00] from freya's bug report)
				damage2 = damage2 * (125+ 25 * skill_lv) / 100; // updated damage (thanks to [Mr_KrzYch00] from freya's bug report)
				if (sd->arrow_ele > 0) {
					s_ele = sd->arrow_ele;
					s_ele_ = sd->arrow_ele;
				}
				flag = (flag & ~BF_RANGEMASK) | BF_LONG;
				sd->state.arrow_atk = 1;
				break;
			case CH_TIGERFIST: // 伏虎拳
				damage = damage * (40 + 100 * skill_lv) / 100; // kRO patch 12/14/04 - Damage has been increased to 140% ~ 540%.
				damage2 = damage2 * (40 + 100 * skill_lv) / 100; // kRO patch 12/14/04 - Damage has been increased to 140% ~ 540%.
				break;
			case CH_CHAINCRUSH: // 連柱崩撃
				damage = damage * (400 + 100 * skill_lv) / 100; // kRO patch 12/14/04 - Damage has been increased to 500% ~ 1400%.
				damage2 = damage2 * (400 + 100 * skill_lv) / 100; // kRO patch 12/14/04 - Damage has been increased to 500% ~ 1400%.
				break;
			case CH_PALMSTRIKE: // 猛虎硬派山
				damage = damage * (200 + 100 * skill_lv) / 100; // kRO patch 12/14/04 - SP taken at maximum has been reduced to 10, and damage has been increased to 300% ~ 700%.
				damage2 = damage2 * (200 + 100 * skill_lv) / 100;
				break;
			case LK_SPIRALPIERCE: /* スパイラルピアース */
				damage = damage * (100 + 50 * skill_lv) / 100; //増加量が分からないので適当に
				flag = (flag & ~BF_RANGEMASK) | BF_LONG;
				if (target->type == BL_PC)
					((struct map_session_data *)target)->canmove_tick = gettick_cache + 1000;
				else if (target->type == BL_MOB)
					((struct mob_data *)target)->canmove_tick = gettick_cache + 1000;
				break;
			case LK_HEADCRUSH:				/* ヘッドクラッシュ */
				damage = damage*(100+ 40*skill_lv)/100;
				damage2 = damage2*(100+ 40*skill_lv)/100;
				break;
			case LK_JOINTBEAT:				/* ジョイントビート */
				damage = damage*(50+ 10*skill_lv)/100;
				damage2 = damage2*(50+ 10*skill_lv)/100;
				break;
			case ASC_METEORASSAULT:			/* メテオアサルト */
				damage = damage*(40+ 40*skill_lv)/100;
				damage2 = damage2*(40+ 40*skill_lv)/100;
				no_cardfix = 1;
				break;
			case SN_SHARPSHOOTING:			/* シャープシューティング */
				damage += damage*(30*skill_lv)/100;
				damage2 += damage2*(30*skill_lv)/100;
				break;
			case CG_ARROWVULCAN: /* アローバルカン */
				damage = damage * (200 + 100 * skill_lv) / 100;
				damage2 = damage2 * (200 + 100 * skill_lv) / 100;
				if (sd->arrow_ele > 0) { // kRO patch 12/14/04 - The total damage will also include the bonus from the card as well as element of the arrow.
					s_ele = sd->arrow_ele;
					s_ele_ = sd->arrow_ele;
				}
				flag = (flag&~BF_RANGEMASK) | BF_LONG;
				break;
			case AS_SPLASHER: /* ベナムスプラッシャー */
				damage = damage*(200+20*skill_lv+20*pc_checkskill(sd,AS_POISONREACT))/100;
				damage2 = damage2*(200+20*skill_lv+20*pc_checkskill(sd,AS_POISONREACT))/100;
				break;
			case PA_SACRIFICE:
				if (sd) {
					int hp, mhp;
					hp = status_get_hp(src);
					mhp = status_get_max_hp(src);
					damage = mhp * 9 / 100 * (90 + 10 * skill_lv) / 100; // corrected by [frostycpu] - Freya' forum
					damage2 = damage; // corrected by [frostycpu] - Freya' forum
				}
				break;
			case ASC_BREAKER:		// -- moonsoul (special damage for ASC_BREAKER skill)
				if (sd) {
// A lot of Corrections to BREAKER SKILL (pneuma included) (Posted on freya's bug report by Gawaine)
					damage = (damage * skill_lv) /2;
					damage2 = (damage2 * skill_lv) /2;
// -- removed
/*					int mdef1 = status_get_mdef(target);
					int mdef2 = status_get_mdef2(target);
					int imdef_flag = 0;

					damage = ((damage * 5) + (skill_lv * status_get_int(src) * 5) + rand() % 500 + 500) /2;
					damage2 = ((damage2 * 5) + (skill_lv * status_get_int(src) * 5) + rand() % 500 + 500) /2;
					damage3 = damage;
					// physical damage can miss
					hitrate = 1000000;*/

					// calculate physical part of damage
// -- removed		damage = damage * skill_lv;
// -- removed		damage2 = damage2 * skill_lv;
					// element modifier added right after this

					// calculate magic part of damage
// -- removed		damage3 = skill_lv * status_get_int(src) * 5;

					// ignores magic defense now [Celest]
/*					if (sd->ignore_mdef_ele & (1<<t_ele) || sd->ignore_mdef_race & (1<<t_race))
						imdef_flag = 1;
					if (t_mode & 0x20) {
						if (sd->ignore_mdef_race & (1<<10))
							imdef_flag = 1;
					} else {
						if (sd->ignore_mdef_race & (1<<11))
							imdef_flag = 1;
					}
					if (!imdef_flag) {
						if (battle_config.magic_defense_type) {
							damage3 = damage3 - (mdef1 * battle_config.magic_defense_type) - mdef2;
						} else {
							damage3 = (damage3 * (100 - mdef1)) / 100 - mdef2;
						}
					}
					if (damage3 < 1)
						damage3 = 1;
					damage3 = battle_attr_fix(damage2, s_ele_, status_get_element(target));*/
// End ----------- A lot of Corrections to BREAKER SKILL (pneuma included) (Posted on freya's bug report by Gawaine)
				}
				flag = (flag & ~BF_RANGEMASK) | BF_LONG;
				break;
			}
			if (div_flag && div_ > 1) { // [Skotlex]
				damage *= div_;
				damage2 *= div_;
			}
			if (sd && skill_num > 0 && sd->skillatk[0] == skill_num)
				damage += damage * sd->skillatk[1] / 100;
		}
		if (da == 2) { //三段掌が発動しているか
			type = 0x08;
			div_ = 255; //三段掌用に…
			damage = damage * (100 + 20 * pc_checkskill(sd, MO_TRIPLEATTACK)) / 100;
		}

	// (Skill Sacrifice Strike/Ritual Sacrifice corrections of [Gawaine])
		// 状態異常中のダメージ追加でクリティカルにも有効なスキル
		if (sc_data) {
			// エンチャントデッドリーポイズン
			if (!no_cardfix && sc_data[SC_EDP].timer != -1 && skill_num != ASC_BREAKER && skill_num != ASC_METEORASSAULT) {
				damage += damage * (150 + sc_data[SC_EDP].val1 * 50) / 100;
				no_cardfix = 1;
			}
			// sacrifice works on boss monsters, and does 9% damage to self [Celest]
			if (!skill_num && sc_data[SC_SACRIFICE].timer != -1) {
				int dmg = status_get_max_hp(src) * 9 / 100;
				pc_heal(sd, -dmg, 0);
				damage = dmg * (90 + sc_data[SC_SACRIFICE].val1 * 10) / 100;
				damage2 = 0;
				hitrate = 1000000;
				s_ele = 0;
				s_ele_ = 0;
				sc_data[SC_SACRIFICE].val2 --;
				if (sc_data[SC_SACRIFICE].val2 == 0)
					status_change_end(src, SC_SACRIFICE,-1);
				if (class == 1288)
					hit_emp_flag = 0; 
			}
		}
	// end - (Skill Sacrifice Strike/Ritual Sacrifice corrections of [Gawaine])

		if (skill_num != NPC_CRITICALSLASH) {
			// 対 象の防御力によるダメージの減少
			// ディバインプロテクション（ここでいいのかな？）
			if (skill_num != MO_INVESTIGATE && skill_num != MO_EXTREMITYFIST && skill_num != KN_AUTOCOUNTER && skill_num != AM_ACIDTERROR && def1 < 1000000) {	//DEF, VIT無視
				int t_def;
				if (battle_config.vit_penalty_type > 0) {
					target_count = battle_counttargeted(target, src, battle_config.vit_penalty_count_lv) - battle_config.vit_penalty_count;
					if (target_count > 0) {
						if (battle_config.vit_penalty_type == 1) {
							def1 = (def1 * (100 - target_count * battle_config.vit_penalty_num)) / 100;
							def2 = (def2 * (100 - target_count * battle_config.vit_penalty_num)) / 100;
							t_vit = (t_vit * (100 - target_count * battle_config.vit_penalty_num)) / 100;
						} else if (battle_config.vit_penalty_type == 2) {
							def1 -= (target_count * battle_config.vit_penalty_num);
							def2 -= (target_count * battle_config.vit_penalty_num);
							t_vit -= (target_count * battle_config.vit_penalty_num);
						} else if (battle_config.vit_penalty_type == 3) { // (penalize only VIT, not DEF)
							t_vit = (t_vit * (100 - target_count * battle_config.vit_penalty_num)) / 100;
						} else if (battle_config.vit_penalty_type == 4) { // (penalize only VIT, not DEF)
							t_vit -= (target_count * battle_config.vit_penalty_num);
						}
						if (def1 < 0) def1 = 0;
						if (def2 < 1) def2 = 1;
						if (t_vit < 1) t_vit = 1;
					}
				}
				t_def = def2 * 8 / 10;
				vitbonusmax = (t_vit/20)*(t_vit/20)-1;
				if (tmd) {
					if (t_mode & 0x20) {
						if (sd->ignore_def_mob & 2)
							idef_flag = 1;
						if (sd->ignore_def_mob_ & 2)
							idef_flag_ = 1;
					} else {
						if (sd->ignore_def_mob & 1)
							idef_flag = 1;
						if (sd->ignore_def_mob_ & 1)
							idef_flag_ = 1;
					}
				}
				if(sd->ignore_def_ele & (1<<t_ele) || sd->ignore_def_race & (1<<t_race))
					idef_flag = 1;
				if(sd->ignore_def_ele_ & (1<<t_ele) || sd->ignore_def_race_ & (1<<t_race))
					idef_flag_ = 1;
				if(t_mode & 0x20) {
					if(sd->ignore_def_race & (1<<10))
						idef_flag = 1;
					if(sd->ignore_def_race_ & (1<<10))
						idef_flag_ = 1;
				} else {
					if(sd->ignore_def_race & (1<<11))
						idef_flag = 1;
					if(sd->ignore_def_race_ & (1<<11))
						idef_flag_ = 1;
				}

				if(!idef_flag){
					if (battle_config.player_defense_type) {
						damage = damage - (def1 * battle_config.player_defense_type) - t_def - ((vitbonusmax < 1)?0: rand()%(vitbonusmax+1) );
					} else {
						damage = damage * (100 - def1) /100 - t_def - ((vitbonusmax < 1)?0: rand()%(vitbonusmax+1) );
					}
				}
				if(!idef_flag_){
					if (battle_config.player_defense_type) {
						damage2 = damage2 - (def1 * battle_config.player_defense_type) - t_def - ((vitbonusmax < 1)?0: rand()%(vitbonusmax+1) );
					} else {
						damage2 = damage2 * (100 - def1) /100 - t_def - ((vitbonusmax < 1)?0: rand()%(vitbonusmax+1) );
					}
				}
			}
		}
	}

/*	// (Skill Sacrifice Strike/Ritual Sacrifice corrections of [Gawaine])
	// 状態異常中のダメージ追加でクリティカルにも有効なスキル
	if (sc_data) {
		// エンチャントデッドリーポイズン
		if (!no_cardfix && sc_data[SC_EDP].timer != -1 && skill_num != ASC_BREAKER && skill_num != ASC_METEORASSAULT) {
			damage += damage * (150 + sc_data[SC_EDP].val1 * 50) / 100;
			no_cardfix = 1;
		}
		// sacrifice works on boss monsters, and does 9% damage to self [Celest]
		if (!skill_num && sc_data[SC_SACRIFICE].timer != -1) {
			int dmg = status_get_max_hp(src) * 9 / 100;
			pc_heal(sd, -dmg, 0);
			damage = dmg * (90 + sc_data[SC_SACRIFICE].val1 * 10) / 100;
			damage2 = 0;
			hitrate = 1000000;
			s_ele = 0;
			s_ele_ = 0;
			sc_data[SC_SACRIFICE].val2 --;
			if (sc_data[SC_SACRIFICE].val2 == 0)
				status_change_end(src, SC_SACRIFICE,-1);
			if (class == 1288)
				hit_emp_flag = 0; 
		}
	}*/

	// 精錬ダメージの追加
	if (skill_num != MO_INVESTIGATE && skill_num != MO_EXTREMITYFIST) { //DEF, VIT無視
		damage += status_get_atk2(src);
		damage2 += status_get_atk_2(src);
	}
	if (skill_num == CR_SHIELDBOOMERANG) {
		if (sd && sd->equip_index[8] >= 0) {
			int idx = sd->equip_index[8];
			if (sd->inventory_data[idx] && sd->inventory_data[idx]->type == 5) {
				damage += sd->inventory_data[idx]->weight / 10;
				damage += sd->status.inventory[idx].refine * status_getrefinebonus(0, 1);
			}
		}
	}

	if (skill_num == LK_SPIRALPIERCE) { /* スパイラルピアース */
		if (sd && sd->equip_index[9] >= 0) { //重量で追加ダメージらしいのでシールドブーメランを参考に追加
			int idx = sd->equip_index[9];
			if (sd->inventory_data[idx] && sd->inventory_data[idx]->type == 4) {
				damage += (int)(double)(sd->inventory_data[idx]->weight * (0.8 * skill_lv * 4 / 10));
				damage += sd->status.inventory[idx].refine * status_getrefinebonus(0, 1);
			}
		}
	}

	if(skill_num == PA_SHIELDCHAIN) {			/* シールドチェインもブーメラン同様に */
		if(sd && sd->equip_index[8] >= 0) {
			int idx = sd->equip_index[8];
			if(sd->inventory_data[idx] && sd->inventory_data[idx]->type == 5) {
				damage += sd->inventory_data[idx]->weight / 10;
				damage += sd->status.inventory[idx].refine * status_getrefinebonus(0, 1);
			}
		}
	}

	// 0未満だった場合1に補正
	if(damage<1) damage=1;
	if(damage2<1) damage2=1;

	// スキル修正２（修練系）
	// 修練ダメージ(右手のみ) ソニックブロー時は別処理（1撃に付き1/8適応)
	if (skill_num != MO_INVESTIGATE && skill_num != MO_EXTREMITYFIST && skill_num != CR_GRANDCROSS && skill_num != LK_SPIRALPIERCE) { //修練ダメージ無視
		damage = battle_addmastery(sd, target, damage, 0);
		damage2 = battle_addmastery(sd, target, damage2, 1);
	}

	if(sd->perfect_hit > 0) {
		if(rand()%100 < sd->perfect_hit)
			hitrate = 1000000;
	}

	// 回避修正
	if(	hitrate < 1000000 && t_sc_data ) {			// 必中攻撃
		if(t_sc_data[SC_FOGWALL].timer != -1 && flag&BF_LONG)
			hitrate -= 50;
		if (t_sc_data[SC_SLEEP].timer!=-1 ||	// 睡眠は必中
			t_sc_data[SC_STAN].timer!=-1 ||		// スタンは必中
			t_sc_data[SC_FREEZE].timer!=-1 ||
			(t_sc_data[SC_STONE].timer!=-1 && t_sc_data[SC_STONE].val2==0))	// 凍結は必中
			hitrate = 1000000;
	}
	hitrate = (hitrate<5)?5:hitrate;
	if(type == 0 && rand()%100 >= hitrate) {
		damage = damage2 = 0;
		dmg_lv = ATK_FLEE;
	} else {
		dmg_lv = ATK_DEF;
	}
	// スキル修正３（武器研究）
	if( (skill=pc_checkskill(sd,BS_WEAPONRESEARCH)) > 0) {
		damage+= skill*2;
		damage2+= skill*2;
	}
	//Advanced Katar Research by zanetheinsane
	if(sd->weapontype1 == 0x10 || sd->weapontype2 == 0x10){
		if((skill = pc_checkskill(sd,ASC_KATAR)) > 0) {
			damage += damage*(10+(skill * 2))/100;
		}
	}

//スキルによるダメージ補正ここまで

//カードによるダメージ追加処理ここから
	cardfix = 100;
	if (!sd->state.arrow_atk) { //弓矢以外
		if (!battle_config.left_cardfix_to_right) { //左手カード補正設定無し
			cardfix = cardfix * (100 + sd->addrace[t_race]) / 100;	// 種族によるダメージ修正
			cardfix = cardfix * (100 + sd->addele[t_ele]) / 100;	// 属性によるダメージ修正
			cardfix = cardfix * (100 + sd->addsize[t_size]) / 100;	// サイズによるダメージ修正
			cardfix = cardfix * (100 + sd->addrace2[t_race2]) / 100;
		} else {
			cardfix = cardfix * (100 + sd->addrace[t_race] + sd->addrace_[t_race]) / 100;	// 種族によるダメージ修正(左手による追加あり)
			cardfix = cardfix * (100 + sd->addele[t_ele] + sd->addele_[t_ele]) / 100;	// 属性によるダメージ修正(左手による追加あり)
			cardfix = cardfix * (100 + sd->addsize[t_size] + sd->addsize_[t_size]) / 100;	// サイズによるダメージ修正(左手による追加あり)
			cardfix = cardfix * (100 + sd->addrace2[t_race2] + sd->addrace2_[t_race2]) / 100;
		}
	} else { //弓矢
		cardfix = cardfix * (100 + sd->addrace[t_race] + sd->arrow_addrace[t_race]) / 100;	// 種族によるダメージ修正(弓矢による追加あり)
		cardfix = cardfix * (100 + sd->addele[t_ele] + sd->arrow_addele[t_ele]) / 100;	// 属性によるダメージ修正(弓矢による追加あり)
		cardfix = cardfix * (100 + sd->addsize[t_size] + sd->arrow_addsize[t_size]) / 100;	// サイズによるダメージ修正(弓矢による追加あり)
		cardfix = cardfix * (100 + sd->addrace2[t_race2]) / 100;
	}
	if (t_mode & 0x20) { //ボス
		if (!sd->state.arrow_atk) { //弓矢攻撃以外なら
			if (!battle_config.left_cardfix_to_right) //左手カード補正設定無し
				cardfix = cardfix * (100 + sd->addrace[10]) / 100; //ボスモンスターに追加ダメージ
			else //左手カード補正設定あり
				cardfix = cardfix * (100 + sd->addrace[10] + sd->addrace_[10]) / 100; //ボスモンスターに追加ダメージ(左手による追加あり)
		} else //弓矢攻撃
			cardfix = cardfix * (100 + sd->addrace[10] + sd->arrow_addrace[10]) / 100; //ボスモンスターに追加ダメージ(弓矢による追加あり)
	} else { //ボスじゃない
		if (!sd->state.arrow_atk) { //弓矢攻撃以外
			if (!battle_config.left_cardfix_to_right) //左手カード補正設定無し
				cardfix = cardfix * (100 + sd->addrace[11]) / 100; //ボス以外モンスターに追加ダメージ
			else //左手カード補正設定あり
				cardfix = cardfix * (100 + sd->addrace[11] + sd->addrace_[11]) / 100; //ボス以外モンスターに追加ダメージ(左手による追加あり)
		} else
			cardfix = cardfix * (100 + sd->addrace[11] + sd->arrow_addrace[11]) / 100; //ボス以外モンスターに追加ダメージ(弓矢による追加あり)
	}
	//特定Class用補正処理(少女の日記→ボンゴン用？)
	t_class = status_get_class(target);
	for(i = 0; i < sd->add_damage_class_count; i++) {
		if (sd->add_damage_classid[i] == t_class) {
			cardfix = cardfix * (100 + sd->add_damage_classrate[i]) / 100;
			break;
		}
	}
	if (!no_cardfix)
		damage = damage * cardfix / 100; //カード補正によるダメージ増加
//カードによるダメージ増加処理ここまで

//カードによるダメージ追加処理(左手)ここから
	cardfix = 100;
	if (!battle_config.left_cardfix_to_right) { //左手カード補正設定無し
		cardfix = cardfix * (100 + sd->addrace_[t_race]) / 100; // 種族によるダメージ修正左手
		cardfix = cardfix * (100 + sd->addele_[t_ele]) / 100; // 属 性によるダメージ修正左手
		cardfix = cardfix * (100 + sd->addsize_[t_size]) / 100; // サイズによるダメージ修正左手
		cardfix = cardfix * (100 + sd->addrace2_[t_race2]) / 100;
		if (t_mode & 0x20) //ボス
			cardfix = cardfix * (100 + sd->addrace_[10]) / 100; //ボスモンスターに追加ダメージ左手
		else
			cardfix = cardfix * (100 + sd->addrace_[11]) / 100; //ボス以外モンスターに追加ダメージ左手
	}
	//特定Class用補正処理左手(少女の日記→ボンゴン用？)
	for(i = 0; i < sd->add_damage_class_count_; i++) {
		if (sd->add_damage_classid_[i] == t_class) {
			cardfix = cardfix * (100 + sd->add_damage_classrate_[i]) / 100;
			break;
		}
	}
	if (!no_cardfix) //カード補正による左手ダメージ増加
		damage2 = damage2 * cardfix / 100;
//カードによるダメージ増加処理(左手)ここまで

// A lot of Corrections to BREAKER SKILL (pneuma included) (Posted on freya's bug report by Gawaine)
/* -- removed
// -- moonsoul (cardfix for magic damage portion of ASC_BREAKER)
	if (skill_num == ASC_BREAKER) {
		damage3 = damage3 * cardfix / 100;
		dmg_lv = ATK_DEF; // ignores flee [celest]
	}*/
// End ----------- A lot of Corrections to BREAKER SKILL (pneuma included) (Posted on freya's bug report by Gawaine)

//カードによるダメージ減衰処理ここから
	if (tsd) { //対象がPCの場合
		cardfix = 100;
		cardfix = cardfix * (100 - tsd->subrace[s_race] )/ 100;	// 種族によるダメージ耐性
		cardfix = cardfix * (100 - tsd->subele[s_ele]) / 100;	// 属性によるダメージ耐性
		cardfix = cardfix * (100 - tsd->subsize[s_size]) / 100;
		if (status_get_mode(src) & 0x20)
			cardfix = cardfix * (100 - tsd->subrace[10]) / 100; //ボスからの攻撃はダメージ減少
		else
			cardfix = cardfix * (100 - tsd->subrace[11]) / 100; //ボス以外からの攻撃はダメージ減少
		//特定Class用補正処理左手(少女の日記→ボンゴン用？)
		for(i = 0; i < tsd->add_def_class_count; i++) {
			if (tsd->add_def_classid[i] == sd->status.class) {
				cardfix = cardfix * (100 - tsd->add_def_classrate[i]) / 100;
				break;
			}
		}
		if (flag & BF_LONG)
			cardfix = cardfix * (100 - tsd->long_attack_def_rate) / 100; //遠距離攻撃はダメージ減少(ホルンCとか)
		if (flag & BF_SHORT)
			cardfix = cardfix * (100 - tsd->near_attack_def_rate) / 100; //近距離攻撃はダメージ減少(該当無し？)
		damage = damage * cardfix / 100; //カード補正によるダメージ減少
		damage2 = damage2 * cardfix / 100; //カード補正による左手ダメージ減少
	}
//カードによるダメージ減衰処理ここまで

//対象にステータス異常がある場合のダメージ減算処理ここから
	if(t_sc_data) {
		cardfix=100;
		if(t_sc_data[SC_DEFENDER].timer != -1 && flag&BF_LONG) //ディフェンダー状態で遠距離攻撃
			cardfix=cardfix*(100-t_sc_data[SC_DEFENDER].val2)/100; //ディフェンダーによる減衰
		if(t_sc_data[SC_FOGWALL].timer != -1 && flag&BF_LONG)
			cardfix=cardfix*(100-t_sc_data[SC_FOGWALL].val2)/100;
		if(cardfix != 100) {
			damage=damage*cardfix/100; //ディフェンダー補正によるダメージ減少
			damage2=damage2*cardfix/100; //ディフェンダー補正による左手ダメージ減少
		}
		if (t_sc_data[SC_ASSUMPTIO].timer != -1) { //アスムプティオ
			if (map[target->m].flag.pvp || map[target->m].flag.gvg) {
			damage = damage * 2 / 3;
			damage2 = damage2 / 3;
		} else {
			damage = damage / 2;
			damage2 = damage2 / 2;
		}
	}
	}
//対象にステータス異常がある場合のダメージ減算処理ここまで

	if(damage < 0) damage = 0;
	if(damage2 < 0) damage2 = 0;

	// 属 性の適用
	damage=battle_attr_fix(damage,s_ele, status_get_element(target) );
	damage2=battle_attr_fix(damage2,s_ele_, status_get_element(target) );

	// 星のかけら、気球の適用
	damage += sd->star;
	damage2 += sd->star_;
	damage += sd->spiritball*3;
	damage2 += sd->spiritball*3;

	//if(sc_data && sc_data[SC_AURABLADE].timer!=-1){	/* オーラブレード 必中 */
	//	damage += sc_data[SC_AURABLADE].val1 * 10;
	//	damage2 += sc_data[SC_AURABLADE].val1 * 10;
	//}
	if(skill_num==PA_PRESSURE){ /* プレッシャー 必中? */
		damage = 500+300*skill_lv;
		damage2 = 500+300*skill_lv;
	}

	// >二刀流の左右ダメージ計算誰かやってくれぇぇぇぇえええ！
	// >map_session_data に左手ダメージ(atk,atk2)追加して
	// >status_calc_pc()でやるべきかな？
	// map_session_data に左手武器(atk,atk2,ele,star,atkmods)追加して
	// status_calc_pc()でデータを入力しています

	//左手のみ武器装備
	if(sd->weapontype1 == 0 && sd->weapontype2 > 0) {
		damage = damage2;
		damage2 = 0;
	}
	// 右手、左手修練の適用
	if(sd->status.weapon > 16) {// 二刀流か?
		int dmg = damage, dmg2 = damage2;
		// 右手修練(60% ～ 100%) 右手全般
		skill = pc_checkskill(sd,AS_RIGHT);
		damage = damage * (50 + (skill * 10))/100;
		if(dmg > 0 && damage < 1) damage = 1;
		// 左手修練(40% ～ 80%) 左手全般
		skill = pc_checkskill(sd,AS_LEFT);
		damage2 = damage2 * (30 + (skill * 10))/100;
		if(dmg2 > 0 && damage2 < 1) damage2 = 1;
	}
	else //二刀流でなければ左手ダメージは0
		damage2 = 0;

		// 右手,短剣のみ
	if(da == 1) { //ダブルアタックが発動しているか
		div_ = 2;
		damage += damage;
		type = 0x08;
	}

	if(sd->status.weapon == 16) {
		// カタール追撃ダメージ
		skill = pc_checkskill(sd,TF_DOUBLE);
		damage2 = damage * (1 + (skill * 2))/100;
		if(damage > 0 && damage2 < 1) damage2 = 1;
	}

	// インベナム修正
	if(skill_num==TF_POISON){
		damage = battle_attr_fix(damage + 15*skill_lv, s_ele, status_get_element(target) );
	}
	if(skill_num==MC_CARTREVOLUTION){
		damage = battle_attr_fix(damage, 0, status_get_element(target) );
	}

	// 完全回避の判定
	if(skill_num == 0 && tsd!=NULL && div_ < 255 && rand()%1000 < status_get_flee2(target) ){
		damage=damage2=0;
		type=0x0b;
		dmg_lv = ATK_LUCKY;
	}

	// 対象が完全回避をする設定がONなら
	if(battle_config.enemy_perfect_flee) {
		if(skill_num == 0 && tmd!=NULL && div_ < 255 && rand()%1000 < status_get_flee2(target) ) {
			damage=damage2=0;
			type=0x0b;
			dmg_lv = ATK_LUCKY;
		}
	}

	//MobのModeに頑強フラグが立っているときの処理
	if(t_mode&0x40){
		if(damage > 0)
			damage = 1;
		if(damage2 > 0)
			damage2 = 1;
	}

	//bNoWeaponDamage(設定アイテム無し？)でグランドクロスじゃない場合はダメージが0
	if( tsd && tsd->special_state.no_weapon_damage && skill_num != CR_GRANDCROSS)
		damage = damage2 = 0;

	if(skill_num != CR_GRANDCROSS && (damage > 0 || damage2 > 0) ) {
		if(damage2<1)		// ダメージ最終修正
			damage=battle_calc_damage(src,target,damage,div_,skill_num,skill_lv,flag);
		else if(damage<1)	// 右手がミス？
			damage2=battle_calc_damage(src,target,damage2,div_,skill_num,skill_lv,flag);
		else {	// 両 手/カタールの場合はちょっと計算ややこしい
			int d1=damage+damage2,d2=damage2;
			damage=battle_calc_damage(src,target,damage+damage2,div_,skill_num,skill_lv,flag);
			damage2=(d2*100/d1)*damage/100;
			if(damage > 1 && damage2 < 1) damage2=1;
			damage-=damage2;
		}
	}

	/*				For executioner card [Valaris]				*/
		if(src->type == BL_PC && sd->random_attack_increase_add > 0 && sd->random_attack_increase_per > 0 && skill_num == 0 ){
			if(rand()%100 < sd->random_attack_increase_per){
				if(damage >0) damage*=sd->random_attack_increase_add/100;
				if(damage2 >0) damage2*=sd->random_attack_increase_add/100;
				}
		}
	/*					End addition					*/

	// for azoth weapon [Valaris]
	if(src->type == BL_PC && target->type == BL_MOB && sd->classchange) {
		if(rand()%10000 < sd->classchange) {
			int changeclass[] = {
			1001,1002,1004,1005,1007,1008,1009,1010,1011,1012,1013,1014,1015,1016,1018,1019,1020,
			1021,1023,1024,1025,1026,1028,1029,1030,1031,1032,1033,1034,1035,1036,1037,1040,1041,
			1042,1044,1045,1047,1048,1049,1050,1051,1052,1053,1054,1055,1056,1057,1058,1060,1061,
			1062,1063,1064,1065,1066,1067,1068,1069,1070,1071,1076,1077,1078,1079,1080,1081,1083,
			1084,1085,1094,1095,1097,1099,1100,1101,1102,1103,1104,1105,1106,1107,1108,1109,1110,
			1111,1113,1114,1116,1117,1118,1119,1121,1122,1123,1124,1125,1126,1127,1128,1129,1130,
			1131,1132,1133,1134,1135,1138,1139,1140,1141,1142,1143,1144,1145,1146,1148,1149,1151,
			1152,1153,1154,1155,1156,1158,1160,1161,1163,1164,1165,1166,1167,1169,1170,1174,1175,
			1176,1177,1178,1179,1180,1182,1183,1184,1185,1188,1189,1191,1192,1193,1194,1195,1196,
			1197,1199,1200,1201,1202,1204,1205,1206,1207,1208,1209,1211,1212,1213,1214,1215,1216,
			1219,1242,1243,1245,1246,1247,1248,1249,1250,1253,1254,1255,1256,1257,1258,1260,1261,
			1263,1264,1265,1266,1267,1269,1270,1271,1273,1274,1275,1276,1277,1278,1280,1281,1282,
			1291,1292,1293,1294,1295,1297,1298,1300,1301,1302,1304,1305,1306,1308,1309,1310,1311,
			1313,1314,1315,1316,1317,1318,1319,1320,1321,1322,1323,1364,1365,1366,1367,1368,1369,
			1370,1371,1372,1374,1375,1376,1377,1378,1379,1380,1381,1382,1383,1384,1385,1386,1387,
			1390,1391,1392,1400,1401,1402,1403,1404,1405,1406,1408,1409,1410,1412,1413,1415,1416,
			1417,1493,1494,1495,1497,1498,1499,1500,1502,1503,1504,1505,1506,1507,1508,1509,1510,
			1511,1512,1513,1514,1515,1516,1517,1519,1520,1582,1584,1585,1586,1587 };
			mob_class_change(((struct mob_data *)target),changeclass);
		}
	}

// A lot of Corrections to BREAKER SKILL (pneuma included) (Posted on freya's bug report by Gawaine)
/* -- removed
// -- moonsoul (final combination of phys, mag damage for ASC_BREAKER)
	if (skill_num == ASC_BREAKER) {
		damage3 += rand() % 500 + 500;
		damage += damage3;
//		damage2 += damage3;
	}*/
// End ----------- A lot of Corrections to BREAKER SKILL (pneuma included) (Posted on freya's bug report by Gawaine)

	// (Skill Sacrifice Strike/Ritual Sacrifice corrections of [Gawaine])
	if (!hit_emp_flag) {
		damage = 0;
		damage2 = 0;
		type = 0;
	}
	// end - (Skill Sacrifice Strike/Ritual Sacrifice corrections of [Gawaine])

	wd.damage = damage;
	wd.damage2 = damage2;
	wd.type = type;
	wd.div_ = div_;
	wd.amotion = status_get_amotion(src);
	if (skill_num == KN_AUTOCOUNTER)
		wd.amotion >>= 1;
	wd.dmotion = status_get_dmotion(target);
	wd.blewcount = blewcount;
	wd.flag = flag;
	wd.dmg_lv = dmg_lv;

	return wd;
}

/*==========================================
 * 武器ダメージ計算
 *------------------------------------------
 */
struct Damage battle_calc_weapon_attack(
	struct block_list *src,struct block_list *target,int skill_num,int skill_lv,int wflag)
{
	struct Damage wd;

	//return前の処理があるので情報出力部のみ変更
	if (src == NULL || target == NULL) {
		memset(&wd, 0, sizeof(wd));
		return wd;
	}

	if (target->type == BL_PET)
		memset(&wd, 0, sizeof(wd));
	else if (src->type == BL_PC)
		wd = battle_calc_pc_weapon_attack(src, target, skill_num, skill_lv, wflag);
	else if (src->type == BL_MOB)
		wd = battle_calc_mob_weapon_attack(src, target, skill_num, skill_lv, wflag);
	else if (src->type == BL_PET)
		wd = battle_calc_pet_weapon_attack(src, target, skill_num, skill_lv, wflag);
	else
		memset(&wd, 0, sizeof(wd));

	if (battle_config.equipment_breaking && src->type==BL_PC && (wd.damage > 0 || wd.damage2 > 0)) {
		struct map_session_data *sd=(struct map_session_data *)src;
		// weapon = 0, armor = 1
		int breakrate = 1; // 0.01% default self weapon breaking chance [DracoRPG]
		int breakrate_[2] = {0,0}; //enemy breaking chance [celest]
		int breaktime = 5000;

		breakrate_[0] += sd->break_weapon_rate;
		breakrate_[1] += sd->break_armor_rate;

		if (sd->sc_count) {
			if (sd->sc_data[SC_MELTDOWN].timer!=-1) {
				breakrate_[0] += 100 * sd->sc_data[SC_MELTDOWN].val1;
				breakrate_[1] = 70 * sd->sc_data[SC_MELTDOWN].val1;
				breaktime = skill_get_time2(WS_MELTDOWN, 1);
			}
			if (sd->sc_data[SC_OVERTHRUST].timer != -1)
				breakrate += 10;
		}

		if (sd->status.weapon && sd->status.weapon != 11) {
			if (rand() % 10000 < breakrate * battle_config.equipment_break_rate / 100 || breakrate >= 10000)
				if (pc_breakweapon(sd) == 1)
					wd = battle_calc_pc_weapon_attack(src, target, skill_num, skill_lv, wflag);
		}
		if (rand() % 10000 < breakrate_[0] * battle_config.equipment_break_rate / 100 || breakrate_[0] >= 10000) {
			if (target->type == BL_PC) {
				struct map_session_data *tsd = (struct map_session_data *)target;
				if (tsd->status.weapon != 11)
					pc_breakweapon(tsd);
			} else
				status_change_start(target, SC_STRIPWEAPON, 1, 75, 0, 0, breaktime, 0);
		}
		if (rand() % 10000 < breakrate_[1] * battle_config.equipment_break_rate / 100 || breakrate_[1] >= 10000) {
			if (target->type == BL_PC) {
				struct map_session_data *tsd = (struct map_session_data *)target;
				if (tsd->status.weapon != 11)
					pc_breakarmor(tsd);
			} else
				status_change_start(target, SC_STRIPSHIELD, 1, 75, 0, 0, breaktime, 0);
		}
	}

	return wd;
}

/*==========================================
 * 魔法ダメージ計算
 *------------------------------------------
 */
struct Damage battle_calc_magic_attack(
	struct block_list *bl, struct block_list *target, int skill_num, int skill_lv, int flag)
{
	int mdef1 = status_get_mdef(target);
	int mdef2 = status_get_mdef2(target);
	int matk1, matk2, damage = 0, div_ = 1, blewcount = skill_get_blewcount(skill_num, skill_lv), rdamage = 0;
	struct Damage md;
	int aflag;
	int normalmagic_flag = 1;
	int matk_flag = 1;
	int ele = 0, race = 7, size = 1, t_ele = 0, t_race = 7, t_mode = 0, cardfix, t_class, i;
	struct map_session_data *sd = NULL, *tsd = NULL;
	struct mob_data *tmd = NULL;

	//return前の処理があるので情報出力部のみ変更
	if (bl == NULL || target == NULL) {
		memset(&md, 0, sizeof(md));
		return md;
	}

	if (target->type == BL_PET) {
		memset(&md, 0, sizeof(md));
		return md;
	}

	matk1 = status_get_matk1(bl);
	matk2 = status_get_matk2(bl);
	ele = skill_get_pl(skill_num);
	race = status_get_race(bl);
	size = status_get_size(bl);
	t_ele = status_get_elem_type(target);
	t_race = status_get_race(target);
	t_mode = status_get_mode(target);

#define MATK_FIX(a, b) { matk1 = matk1 * (a) / (b); matk2 = matk2 * (a) / (b); }

	if( bl->type==BL_PC && (sd=(struct map_session_data *)bl) ){
		sd->state.attack_type = BF_MAGIC;
		if(sd->matk_rate != 100)
			MATK_FIX(sd->matk_rate,100);
		sd->state.arrow_atk = 0;
	}
	if( target->type==BL_PC )
		tsd=(struct map_session_data *)target;
	else if( target->type==BL_MOB )
		tmd=(struct mob_data *)target;

	aflag=BF_MAGIC|BF_LONG|BF_SKILL;

	if(skill_num > 0){
		switch(skill_num){	// 基本ダメージ計算(スキルごとに処理)
					// ヒールor聖体
		case AL_HEAL:
		case PR_BENEDICTIO:
			damage = skill_calc_heal(bl,skill_lv)/2;
			normalmagic_flag=0;
			break;
		case PR_ASPERSIO:		/* アスペルシオ */
			damage = 40; //固定ダメージ
			normalmagic_flag=0;
			break;
		case PR_SANCTUARY:	// サンクチュアリ
			damage = (skill_lv>6)?388:skill_lv*50;
			normalmagic_flag=0;
			blewcount|=0x10000;
			break;
		case ALL_RESURRECTION:
		case PR_TURNUNDEAD:	// 攻撃リザレクションとターンアンデッド
			if(target->type != BL_PC && battle_check_undead(t_race,t_ele)){
				int hp, mhp, thres;
				hp = status_get_hp(target);
				mhp = status_get_max_hp(target);
				thres = (skill_lv * 20) + status_get_luk(bl)+
						status_get_int(bl) + status_get_lv(bl)+
						((200 - hp * 200 / mhp));
				if(thres > 700) thres = 700;
//				if(battle_config.battle_log)
//					printf("ターンアンデッド！ 確率%d ‰(千分率)\n", thres);
				if(rand()%1000 < thres && !(t_mode&0x20))	// 成功
					damage = hp;
				else					// 失敗
					damage = status_get_lv(bl) + status_get_int(bl) + skill_lv * 10;
			}
			normalmagic_flag=0;
			break;

		case MG_NAPALMBEAT:	// ナパームビート（分散計算込み）
			MATK_FIX(70+ skill_lv*10,100);
			if(flag>0){
				MATK_FIX(1,flag);
			}else {
				if(battle_config.error_log)
					printf("battle_calc_magic_attack(): napam enemy count=0 !\n");
			}
			break;
		case MG_FIREBALL:	// ファイヤーボール
			{
				const int drate[]={100,90,70};
				if(flag>2)
					matk1=matk2=0;
				else
					MATK_FIX( (95+skill_lv*5)*drate[flag] ,10000 );
			}
			break;
		case MG_FIREWALL:	// ファイヤーウォール
/*
			if( (t_ele!=3 && !battle_check_undead(t_race,t_ele)) || target->type==BL_PC ) //PCは火属性でも飛ぶ？そもそもダメージ受ける？
				blewcount |= 0x10000;
			else
				blewcount = 0;
*/
			if((t_ele==3 || battle_check_undead(t_race,t_ele)) && target->type!=BL_PC)
				blewcount = 0;
			else
				blewcount |= 0x10000;
			MATK_FIX( 1,2 );
			break;
		case MG_THUNDERSTORM:	// サンダーストーム
			MATK_FIX( 80,100 );
			break;
		case MG_FROSTDIVER:	// フロストダイバ
			MATK_FIX( 100+skill_lv*10, 100);
			break;
		case WZ_FROSTNOVA:	// フロストダイバ
			MATK_FIX((100+skill_lv*10)*2/3, 100);
			break;
		case WZ_FIREPILLAR:	// ファイヤーピラー
			if(mdef1 < 1000000)
				mdef1=mdef2=0;	// MDEF無視
			MATK_FIX( 1,5 );
			matk1+=50;
			matk2+=50;
			break;
		case WZ_SIGHTRASHER:
			MATK_FIX( 100+skill_lv*20, 100);
			break;
		case WZ_METEOR:
		case WZ_JUPITEL:	// ユピテルサンダー
			break;
		case WZ_VERMILION:	// ロードオブバーミリオン
			MATK_FIX( skill_lv*20+80, 100 );
			break;
		case WZ_WATERBALL:	// ウォーターボール
			//matk1+= skill_lv*30;
			//matk2+= skill_lv*30;
			MATK_FIX(100 + skill_lv * 30, 100);
			break;
		case WZ_STORMGUST:	// ストームガスト
			MATK_FIX( skill_lv*40+100 ,100 );
//			blewcount|=0x10000;
			break;
		case AL_HOLYLIGHT:	// ホーリーライト
			MATK_FIX( 125,100 );
			break;
		case AL_RUWACH:
			MATK_FIX( 145,100 );
			break;
		case HW_NAPALMVULCAN:	// ナパームビート（分散計算込み）
			MATK_FIX(70+ skill_lv*10,100);
			if(flag>0){
				MATK_FIX(1,flag);
			} else {
				if(battle_config.error_log)
					printf("battle_calc_magic_attack(): napalmvulcan enemy count=0 !\n");
			}
			break;
		case PF_SOULBURN: // Celest
			if (target->type != BL_PC || skill_lv < 5) {
				memset(&md,0,sizeof(md));
				return md;
			} else if (target->type == BL_PC) {
				damage = ((struct map_session_data *)target)->status.sp * 2;
				matk_flag = 0; // don't consider matk and matk2
			}
			break;
// A lot of Corrections to BREAKER SKILL (pneuma included) (Posted on freya's bug report by Gawaine)
		case ASC_BREAKER:
			damage = rand() % 500 + 500 + skill_lv * status_get_int(bl) * 5;
			matk_flag = 0; // don't consider matk and matk2
			break;
		}
	}
// End ----------- A lot of Corrections to BREAKER SKILL (pneuma included) (Posted on freya's bug report by Gawaine)

	if(normalmagic_flag){	// 一般魔法ダメージ計算
		int imdef_flag=0;
		if (matk_flag) {
			if(matk1>matk2)
				damage= matk2+rand()%(matk1-matk2+1);
			else
				damage= matk2;
		}
		if(sd) {
			if(sd->ignore_mdef_ele & (1<<t_ele) || sd->ignore_mdef_race & (1<<t_race))
				imdef_flag = 1;
			if(t_mode & 0x20) {
				if(sd->ignore_mdef_race & (1<<10))
					imdef_flag = 1;
			}
			else {
				if(sd->ignore_mdef_race & (1<<11))
					imdef_flag = 1;
			}
		}
		if(!imdef_flag){
			if (battle_config.magic_defense_type) {
				damage = damage - (mdef1 * battle_config.magic_defense_type) - mdef2;
			} else {
				damage = (damage*(100-mdef1))/100 - mdef2;
			}
		}

		if(damage<1)
			damage=1;
	}

	if (sd) {
		cardfix = 100;
		cardfix = cardfix * (100 + sd->magic_addrace[t_race]) / 100;
		cardfix = cardfix * (100 + sd->magic_addele[t_ele]) / 100;
		if (t_mode & 0x20)
			cardfix = cardfix * (100 + sd->magic_addrace[10]) / 100;
		else
			cardfix = cardfix * (100 + sd->magic_addrace[11]) / 100;
		t_class = status_get_class(target);
		for(i = 0; i < sd->add_magic_damage_class_count; i++) {
			if (sd->add_magic_damage_classid[i] == t_class) {
				cardfix = cardfix * (100 + sd->add_magic_damage_classrate[i]) / 100;
				break;
			}
		}
		damage = damage * cardfix / 100;
		if (skill_num > 0 && sd->skillatk[0] == skill_num)
			damage += damage * sd->skillatk[1] / 100;
	}

	if (tsd) {
		int s_class = status_get_class(bl);
		cardfix = 100;
		cardfix = cardfix * (100 - tsd->subele[ele]) / 100;	// 属 性によるダメージ耐性
		cardfix = cardfix * (100 - tsd->subrace[race]) / 100;	// 種族によるダメージ耐性
		cardfix = cardfix * (100 - tsd->subsize[size]) / 100;
		cardfix = cardfix * (100 - tsd->magic_subrace[race]) / 100;
		if (status_get_mode(bl) & 0x20)
			cardfix = cardfix * (100 - tsd->magic_subrace[10]) / 100;
		else
			cardfix = cardfix * (100 - tsd->magic_subrace[11]) / 100;
		for(i = 0; i < tsd->add_mdef_class_count; i++) {
			if (tsd->add_mdef_classid[i] == s_class) {
				cardfix = cardfix * (100 - tsd->add_mdef_classrate[i]) / 100;
				break;
			}
		}
		cardfix = cardfix * (100 - tsd->magic_def_rate) / 100;
		damage = damage * cardfix / 100;
	}
	if (damage < 0) damage = 0;

	damage = battle_attr_fix(damage, ele, status_get_element(target)); // 属 性修正

	if(skill_num == CR_GRANDCROSS) {	// グランドクロス
		struct Damage wd;
		wd=battle_calc_weapon_attack(bl,target,skill_num,skill_lv,flag);
		damage = (damage + wd.damage) * (100 + 40*skill_lv)/100;
		if(battle_config.gx_dupele) damage=battle_attr_fix(damage, ele, status_get_element(target) );	//属性2回かかる
		if(bl==target) damage=damage/2;	//反動は半分
	}

	div_=skill_get_num( skill_num,skill_lv );

	if (div_ > 1 && skill_num != WZ_VERMILION)
		damage*=div_;

//	if(mdef1 >= 1000000 && damage > 0)
	if(t_mode&0x40 && damage > 0)
		damage = 1;

	if( tsd && tsd->special_state.no_magic_damage) {
		if (battle_config.gtb_pvp_only != 0) { // [MouseJstr]
			if ((map[target->m].flag.pvp || map[target->m].flag.gvg) && target->type==BL_PC)
				damage = (damage * (100 - battle_config.gtb_pvp_only)) / 100;
		} else
			damage=0; // 黄 金蟲カード（魔法ダメージ０）
	}

	damage=battle_calc_damage(bl,target,damage,div_,skill_num,skill_lv,aflag);	// 最終修正

	/* magic_damage_return by [AppleGirl] and [Valaris]		*/
	if( target->type==BL_PC && tsd && tsd->magic_damage_return > 0 ){
		rdamage += damage * tsd->magic_damage_return / 100;
		if (rdamage < 1) rdamage = 1;
		clif_damage(target, bl, gettick_cache, 0, 0, rdamage, 0, 0, 0);
		battle_damage(target, bl, rdamage, 0);
	}
	/*			end magic_damage_return			*/

	md.damage = damage;
	md.div_ = div_;
	md.amotion = status_get_amotion(bl);
	md.dmotion = status_get_dmotion(target);
	md.damage2 = 0;
	md.type = 0;
	md.blewcount = blewcount;
	md.flag = aflag;

	return md;
}

/*==========================================
 * その他ダメージ計算
 *------------------------------------------
 */
struct Damage  battle_calc_misc_attack(
	struct block_list *bl, struct block_list *target, int skill_num, int skill_lv, int flag)
{
	int int_=status_get_int(bl);
//	int luk = status_get_luk(bl);
	int dex = status_get_dex(bl);
	int skill, ele, race, size, cardfix;
	struct map_session_data *sd = NULL, *tsd = NULL;
	int damage = 0,div_ = 1, blewcount = skill_get_blewcount(skill_num, skill_lv);
	struct Damage md;
	int damagefix = 1;

	int aflag = BF_MISC | BF_LONG | BF_SKILL;

	//return前の処理があるので情報出力部のみ変更
	if( bl == NULL || target == NULL ){
		memset(&md,0,sizeof(md));
		return md;
	}

	if(target->type == BL_PET) {
		memset(&md,0,sizeof(md));
		return md;
	}

	if( bl->type == BL_PC && (sd=(struct map_session_data *)bl) ) {
		sd->state.attack_type = BF_MISC;
		sd->state.arrow_atk = 0;
	}

	if( target->type==BL_PC )
		tsd=(struct map_session_data *)target;

	switch(skill_num){

	case HT_LANDMINE:	// ランドマイン
		damage=skill_lv*(dex+75)*(100+int_)/100;
		break;

	case HT_BLASTMINE:	// ブラストマイン
		damage=skill_lv*(dex/2+50)*(100+int_)/100;
		break;

	case HT_CLAYMORETRAP:	// クレイモアートラップ
		damage=skill_lv*(dex/2+75)*(100+int_)/100;
		break;

	case HT_BLITZBEAT:	// ブリッツビート
		if (sd == NULL || (skill = pc_checkskill(sd, HT_STEELCROW)) <= 0)
			skill = 0;
		damage = (dex / 10 + int_ / 2 + skill * 3 + 40) * 2;
		if (flag > 1)
			damage /= flag;
		if (status_get_mode(target) & 0x40)
			damage = 1;
		break;

	case TF_THROWSTONE:	// 石投げ
		damage=50;
		damagefix=0;
		break;

	case BA_DISSONANCE:	// 不協和音
		damage=(skill_lv)*20+pc_checkskill(sd,BA_MUSICALLESSON)*3;
		break;

	case NPC_SELFDESTRUCTION:	// 自爆
		damage=status_get_hp(bl)-(bl==target?1:0);
		damagefix=0;
		break;

	case NPC_SMOKING:	// タバコを吸う
		damage=3;
		damagefix=0;
		break;

	case CR_ACIDDEMONSTRATION:
		damage = int_ * (int)(sqrt(100*status_get_vit(target))) / 3;
		if (tsd) damage/=2;
		if (status_get_mode(target) & 0x40)
			damage = 1;
		break;

	case NPC_DARKBREATH:
		{
			struct status_change *sc_data = status_get_sc_data(target);
			int hitrate = status_get_hit(bl) - status_get_flee(target) + 80;
			hitrate = ((hitrate > 95) ? 95: ((hitrate < 5) ? 5 : hitrate));
			if(sc_data && (sc_data[SC_SLEEP].timer!=-1 || sc_data[SC_STAN].timer!=-1 ||
				sc_data[SC_FREEZE].timer!=-1 || (sc_data[SC_STONE].timer!=-1 && sc_data[SC_STONE].val2==0) ) )
				hitrate = 1000000;
			if(rand()%100 < hitrate) {
				damage = 500 + (skill_lv-1)*1000 + rand()%1000;
				if(damage > 9999) damage = 9999;
			}
		}
		break;

	case SN_FALCONASSAULT: /* ファルコンアサルト */
		if (sd == NULL || (skill = pc_checkskill(sd, HT_STEELCROW)) <= 0)
			skill = 0;
		//damage = ((150 + skill_lv * 70) * (dex / 10 + int_ / 2 + skill * 3 + 40) * 2) / 100; // [Yor] according desc
		damage = (15 + skill_lv * 7) * (dex / 10 + int_ / 2 + skill * 3 + 40) / 5; // [Yor] simplify
		if (flag > 1)
			damage /= flag;
		if (status_get_mode(target) & 0x40)
			damage = 1;
		break;
	}

	ele = skill_get_pl(skill_num);
	race = status_get_race(bl);
	size = status_get_size(bl);

	if (damagefix) {
		if (damage < 1 && skill_num != NPC_DARKBREATH)
			damage = 1;

		if (tsd) {
			cardfix = 100;
			cardfix = cardfix * (100 - tsd->subele[ele]) / 100; // 属性によるダメージ耐性
			cardfix = cardfix * (100 - tsd->subrace[race]) / 100; // 種族によるダメージ耐性
			cardfix = cardfix * (100 - tsd->subsize[size]) / 100;
			cardfix = cardfix * (100 - tsd->misc_def_rate) / 100;
			damage = damage * cardfix / 100;
		}
		if (sd && skill_num > 0 && sd->skillatk[0] == skill_num)
			damage += damage * sd->skillatk[1] / 100;
		if (damage < 0) damage = 0;
		damage = battle_attr_fix(damage, ele, status_get_element(target)); // 属性修正
	}

	div_ = skill_get_num(skill_num, skill_lv);
	if (div_ > 1)
		damage *= div_;

	if (damage > 0 && (damage < div_ || (status_get_def(target) >= 1000000 && status_get_mdef(target) >= 1000000))) {
		damage = div_;
	}

	damage=battle_calc_damage(bl,target,damage,div_,skill_num,skill_lv,aflag);	// 最終修正

	md.damage = damage;
	md.div_ = div_;
	md.amotion = status_get_amotion(bl);
	md.dmotion = status_get_dmotion(target);
	md.damage2 = 0;
	md.type = 0;
	md.blewcount = blewcount;
	md.flag = aflag;

	return md;
}

/*==========================================
 * ダメージ計算一括処理用
 *------------------------------------------
 */
struct Damage battle_calc_attack(int attack_type,
	struct block_list *bl, struct block_list *target, int skill_num, int skill_lv, int flag)
{
	struct Damage d;

	switch(attack_type) {
	case BF_WEAPON:
		return battle_calc_weapon_attack(bl, target, skill_num, skill_lv, flag);
	case BF_MAGIC:
		return battle_calc_magic_attack(bl, target, skill_num, skill_lv, flag);
	case BF_MISC:
		return battle_calc_misc_attack(bl, target, skill_num, skill_lv, flag);
	default:
		if (battle_config.error_log)
			printf("battle_calc_attack: unknwon attack type ! %d\n", attack_type);
		memset(&d, 0, sizeof(d));
		break;
	}

	return d;
}

/*==========================================
 * 通常攻撃処理まとめ
 *------------------------------------------
 */
int battle_weapon_attack(struct block_list *src, struct block_list *target, unsigned int tick, int flag) {
	struct map_session_data *sd = NULL;
	struct status_change *sc_data, *t_sc_data;
	short *opt1;
	int race = 7, ele = 0;
	int damage, rdamage = 0;
	struct Damage wd;

	nullpo_retr(0, src);
	nullpo_retr(0, target);

	sc_data = status_get_sc_data(src);
	t_sc_data=status_get_sc_data(target);

	if (src->type == BL_PC)
		sd = (struct map_session_data *)src;

	if (src->prev == NULL || target->prev == NULL)
		return 0;
	if (src->type == BL_PC && pc_isdead(sd))
		return 0;
	if (target->type == BL_PC && pc_isdead((struct map_session_data *)target))
		return 0;

	opt1 = status_get_opt1(src);
	if (opt1 && *opt1 > 0) {
		battle_stopattack(src);
		return 0;
	}

//	removed: fix found on freya's forum (thanks to celest and Aalye) [Yor]
//	// General Hiding: People using auto-attack should not be able to follow/attack a hidden/cloaked target - [Aalye]
//	if (t_sc_data && (t_sc_data[SC_HIDING].timer != -1 || t_sc_data[SC_CLOAKING].timer != -1 || t_sc_data[SC_CHASEWALK].timer != -1)) {
//		battle_stopattack(src);
//		return 0;
//	}

	if (sc_data && sc_data[SC_BLADESTOP].timer != -1) {
		battle_stopattack(src);
		return 0;
	}

	if (battle_check_target(src, target, BCT_ENEMY) <= 0 && !battle_check_range(src, target, 0))
		return 0; // 攻撃対象外

	race = status_get_race(target);
	ele = status_get_elem_type(target);
	if (battle_check_target(src, target, BCT_ENEMY) > 0 && battle_check_range(src, target, 0)) {
		// 攻撃対象となりうるので攻撃
		if (sd && sd->status.weapon == 11) {
			if (sd->equip_index[10] >= 0) {
				if (battle_config.arrow_decrement)
					pc_delitem(sd, sd->equip_index[10], 1, 0);
			} else {
				clif_arrow_fail(sd, 0);
				return 0;
			}
		}
		if(flag&0x8000) {
			if(sd && battle_config.pc_attack_direction_change)
				sd->dir = sd->head_dir = map_calc_dir(src, target->x,target->y );
			else if(src->type == BL_MOB && battle_config.monster_attack_direction_change)
				((struct mob_data *)src)->dir = map_calc_dir(src, target->x,target->y );
			wd=battle_calc_weapon_attack(src,target,KN_AUTOCOUNTER,flag&0xff,0);
		}
		else if(flag&AS_POISONREACT && sc_data && sc_data[SC_POISONREACT].timer!=-1) {
			wd=battle_calc_weapon_attack(src,target,AS_POISONREACT,sc_data[SC_POISONREACT].val1,0);
		}
		else
			wd=battle_calc_weapon_attack(src,target,0,0,0);
		if((damage = wd.damage + wd.damage2) > 0 && src != target) {
			if (t_sc_data && t_sc_data[SC_REJECTSWORD].val3 != 0) {
				rdamage += damage;
				if (rdamage < 1) rdamage = 1;
				t_sc_data[SC_REJECTSWORD].val3 = 0; // Reject Sword: Turn the damage reflect off - [Aalye]
			}
			if (wd.flag & BF_SHORT) {
				if (target->type == BL_PC) {
					struct map_session_data *tsd = (struct map_session_data *)target;
					if (tsd && tsd->short_weapon_damage_return > 0) {
						rdamage += damage * tsd->short_weapon_damage_return / 100;
						if (rdamage < 1) rdamage = 1;
					}
				}
				if (t_sc_data && t_sc_data[SC_REFLECTSHIELD].timer != -1) {
					rdamage += damage * t_sc_data[SC_REFLECTSHIELD].val2 / 100;
					if (rdamage < 1) rdamage = 1;
				}
			}
			else if (wd.flag&BF_LONG) {
				if (target->type == BL_PC) {
					struct map_session_data *tsd = (struct map_session_data *)target;
					if (tsd && tsd->long_weapon_damage_return > 0) {
						rdamage += damage * tsd->long_weapon_damage_return / 100;
						if (rdamage < 1) rdamage = 1;
					}
				}
			}
			if(rdamage > 0)
				clif_damage(src,src,tick, wd.amotion,0,rdamage,1,4,0);
		}

		if (wd.div_ == 255 && sd) { //三段掌
			int delay = 1000 - 4 * status_get_agi(src) - 2 *  status_get_dex(src);
			int skilllv;
			if (wd.damage + wd.damage2 < status_get_hp(target)) {
				struct map_session_data *tsd = (struct map_session_data *)target;
				if ((skilllv = pc_checkskill(sd, MO_CHAINCOMBO)) > 0)
					delay += 300 * battle_config.combo_delay_rate / 100; //追加ディレイをconfにより調整

				status_change_start(src, SC_COMBO, MO_TRIPLEATTACK, skilllv, 0, 0, delay, 0);
				if (pc_checkskill(tsd, RG_PLAGIARISM) > 0)
					skill_copy_skill(tsd, MO_TRIPLEATTACK, skilllv);
			}
			sd->attackabletime = sd->canmove_tick = tick + delay;
			clif_combo_delay(src, delay);
			clif_skill_damage(src, target, tick, wd.amotion, wd.dmotion,
				wd.damage, 3, MO_TRIPLEATTACK, pc_checkskill(sd,MO_TRIPLEATTACK), -1);
		}
		else {
			clif_damage(src, target, tick, wd.amotion, wd.dmotion,
				wd.damage, wd.div_, wd.type, wd.damage2);
		//二刀流左手とカタール追撃のミス表示(無理やり～)
			if (sd && sd->status.weapon >= 16 && wd.damage2 == 0)
				clif_damage(src, target, tick + 10, wd.amotion, wd.dmotion, 0, 1, 0, 0);
		}
		if(sd && sd->splash_range > 0 && (wd.damage > 0 || wd.damage2 > 0) )
			skill_castend_damage_id(src,target,0,-1,tick,0);
		map_freeblock_lock();
		battle_delay_damage(tick + wd.amotion, src, target, (wd.damage + wd.damage2), 0);
		if(target->prev != NULL &&
			(target->type != BL_PC || (target->type == BL_PC && !pc_isdead((struct map_session_data *)target) ) ) ) {
			if(wd.damage > 0 || wd.damage2 > 0) {
				skill_additional_effect(src,target,0,0,BF_WEAPON,tick);
				if(sd) {
					if (sd->weapon_coma_ele[ele] > 0 && rand()%10000 < sd->weapon_coma_ele[ele])
						battle_damage(src,target,status_get_max_hp(target),1);
					if (sd->weapon_coma_race[race] > 0 && rand()%10000 < sd->weapon_coma_race[race])
						battle_damage(src,target,status_get_max_hp(target),1);
					if (status_get_mode(target) & 0x20) {
						if(sd->weapon_coma_race[10] > 0 && rand()%10000 < sd->weapon_coma_race[10])
							battle_damage(src,target,status_get_max_hp(target),1);
					} else {
						if(sd->weapon_coma_race[11] > 0 && rand()%10000 < sd->weapon_coma_race[11])
							battle_damage(src,target,status_get_max_hp(target),1);
					}
				}
			}
		}
		if(sc_data && sc_data[SC_AUTOSPELL].timer != -1 && rand()%100 < sc_data[SC_AUTOSPELL].val4) {
			int skilllv=sc_data[SC_AUTOSPELL].val3,i,f=0;
			i = rand()%100;
			if (i >= 50) skilllv -= 2;
			else if(i >= 15) skilllv--;
			if (skilllv < 1) skilllv = 1;
			if (sd) {
				int sp = skill_get_sp(sc_data[SC_AUTOSPELL].val2,skilllv)*2/3;
				if(sd->status.sp >= sp) {
					if((i=skill_get_inf(sc_data[SC_AUTOSPELL].val2) == 2) || i == 32)
						f = skill_castend_pos2(src,target->x,target->y,sc_data[SC_AUTOSPELL].val2,skilllv,tick,flag);
					else {
						switch( skill_get_nk(sc_data[SC_AUTOSPELL].val2) ) {
						case 0:
						case 2:
							f = skill_castend_damage_id(src,target,sc_data[SC_AUTOSPELL].val2,skilllv,tick,flag);
							break;
						case 1:/* 支援系 */
							if((sc_data[SC_AUTOSPELL].val2==AL_HEAL || (sc_data[SC_AUTOSPELL].val2==ALL_RESURRECTION && target->type != BL_PC)) && battle_check_undead(race,ele))
								f = skill_castend_damage_id(src,target,sc_data[SC_AUTOSPELL].val2,skilllv,tick,flag);
							else
								f = skill_castend_nodamage_id(src,target,sc_data[SC_AUTOSPELL].val2,skilllv,tick,flag);
							break;
						}
					}
					if(!f) pc_heal(sd,0,-sp);
				}
			}
			else {
				if((i=skill_get_inf(sc_data[SC_AUTOSPELL].val2) == 2) || i == 32)
					skill_castend_pos2(src,target->x,target->y,sc_data[SC_AUTOSPELL].val2,skilllv,tick,flag);
				else {
					switch( skill_get_nk(sc_data[SC_AUTOSPELL].val2) ) {
					case 0:
					case 2:
						skill_castend_damage_id(src,target,sc_data[SC_AUTOSPELL].val2,skilllv,tick,flag);
						break;
					case 1:/* 支援系 */
						if((sc_data[SC_AUTOSPELL].val2==AL_HEAL || (sc_data[SC_AUTOSPELL].val2==ALL_RESURRECTION && target->type != BL_PC)) && battle_check_undead(race,ele))
							skill_castend_damage_id(src,target,sc_data[SC_AUTOSPELL].val2,skilllv,tick,flag);
						else
							skill_castend_nodamage_id(src,target,sc_data[SC_AUTOSPELL].val2,skilllv,tick,flag);
						break;
					}
				}
			}
		}
		if (sd) {
			if (sd->autospell_id > 0 && rand() % 100 < sd->autospell_rate) {
				int skilllv = sd->autospell_lv, i, f = 0, sp;
				sp = skill_get_sp(sd->autospell_id, skilllv) * 2 / 3;
				if (battle_config.autospell_consume_sp) {
					if (sd->status.sp >= sp) {
						if ((i = skill_get_inf(sd->autospell_id) == 2) || i == 32)
							f = skill_castend_pos2(src, target->x, target->y, sd->autospell_id, skilllv, tick, flag);
						else {
							switch(skill_get_nk(sd->autospell_id)) {
							case 0:
							case 2:
								f = skill_castend_damage_id(src, target, sd->autospell_id, skilllv, tick, flag);
								break;
							case 1:
								if((sd->autospell_id==AL_HEAL || (sd->autospell_id==ALL_RESURRECTION && target->type != BL_PC)) && battle_check_undead(race,ele))
									f = skill_castend_damage_id(src,target,sd->autospell_id,skilllv,tick,flag);
								else
									f = skill_castend_nodamage_id(src,target,sd->autospell_id,skilllv,tick,flag);
								break;
							}
						}
						if (!f)
							pc_heal(sd, 0, -sp);
					}
				} else {
					if ((i = skill_get_inf(sd->autospell_id) == 2) || i == 32)
						skill_castend_pos2(src, target->x, target->y, sd->autospell_id, skilllv, tick, flag);
					else {
						switch(skill_get_nk(sd->autospell_id)) {
						case 0:
						case 2:
							skill_castend_damage_id(src, target, sd->autospell_id, skilllv, tick, flag);
							break;
						case 1:
							if((sd->autospell_id==AL_HEAL || (sd->autospell_id==ALL_RESURRECTION && target->type != BL_PC)) && battle_check_undead(race,ele))
								skill_castend_damage_id(src,target,sd->autospell_id,skilllv,tick,flag);
							else
								skill_castend_nodamage_id(src,target,sd->autospell_id,skilllv,tick,flag);
							break;
						}
					}
				}
			}
			if (wd.flag & BF_WEAPON && src != target && (wd.damage > 0 || wd.damage2 > 0)) {
				int hp = 0,sp = 0;
				if (!battle_config.left_cardfix_to_right) { // 二刀流左手カードの吸収系効果を右手に追加しない場合
					hp += battle_calc_drain(wd.damage, sd->hp_drain_rate, sd->hp_drain_per, sd->hp_drain_value);
					hp += battle_calc_drain(wd.damage2, sd->hp_drain_rate_, sd->hp_drain_per_, sd->hp_drain_value_);
					sp += battle_calc_drain(wd.damage, sd->sp_drain_rate, sd->sp_drain_per, sd->sp_drain_value);
					sp += battle_calc_drain(wd.damage2, sd->sp_drain_rate_, sd->sp_drain_per_, sd->sp_drain_value_);
				} else { // 二刀流左手カードの吸収系効果を右手に追加する場合
					int hp_drain_rate = sd->hp_drain_rate + sd->hp_drain_rate_;
					int hp_drain_per = sd->hp_drain_per + sd->hp_drain_per_;
					int hp_drain_value = sd->hp_drain_value + sd->hp_drain_value_;
					int sp_drain_rate = sd->sp_drain_rate + sd->sp_drain_rate_;
					int sp_drain_per = sd->sp_drain_per + sd->sp_drain_per_;
					int sp_drain_value = sd->sp_drain_value + sd->sp_drain_value_;
					hp += battle_calc_drain(wd.damage, hp_drain_rate, hp_drain_per, hp_drain_value);
					sp += battle_calc_drain(wd.damage, sp_drain_rate, sp_drain_per, sp_drain_value);
				}
				if (hp || sp) pc_heal(sd, hp, sp);
				if (sd->sp_drain_type && target->type == BL_PC)
					battle_heal(NULL, target, 0, -sp, 0);
			}
		}

		if (target->type == BL_PC && (wd.damage > 0 || wd.damage2 > 0)) {
			struct map_session_data *tsd = (struct map_session_data *)target;
			if (tsd->autospell2_id > 0 && rand() % 100 < tsd->autospell2_rate) {
				int skilllv = tsd->autospell2_lv, i, f = 0, sp;
				sp = skill_get_sp(tsd->autospell2_id, skilllv) * 2 / 3;
				if (battle_config.autospell_consume_sp) {
					if (tsd->status.sp >= sp) {
						struct block_list *tbl;
						if (tsd->autospell2_type == 0)
							tbl = target;
						else
							tbl = src;
						if ((i = skill_get_inf(tsd->autospell2_id) == 2) || i == 32)
							f = skill_castend_pos2(target, tbl->x, tbl->y, tsd->autospell2_id, skilllv, tick, flag);
						else {
							switch(skill_get_nk(tsd->autospell2_id)) {
								case 0:
								case 2:
									f = skill_castend_damage_id(target, tbl, tsd->autospell2_id, skilllv, tick, flag);
									break;
								case 1: /* 支援系 */
									if ((tsd->autospell2_id == AL_HEAL || (tsd->autospell2_id == ALL_RESURRECTION && tbl->type != BL_PC)) &&
									    battle_check_undead(status_get_race(tbl), status_get_elem_type(tbl)))
										f = skill_castend_damage_id(target, tbl, tsd->autospell2_id, skilllv, tick, flag);
									else
										f = skill_castend_nodamage_id(target, tbl, tsd->autospell2_id, skilllv, tick, flag);
									break;
							}
						}
						if (!f)
							pc_heal(tsd, 0, -sp);
					}
				} else {
					struct block_list *tbl;
					if (tsd->autospell2_type == 0)
						tbl = target;
					else
						tbl = src;
					if ((i = skill_get_inf(tsd->autospell2_id) == 2) || i == 32)
						skill_castend_pos2(target, tbl->x, tbl->y, tsd->autospell2_id, skilllv, tick, flag);
					else {
						switch(skill_get_nk(tsd->autospell2_id)) {
							case 0:
							case 2:
								skill_castend_damage_id(target, tbl, tsd->autospell2_id, skilllv, tick, flag);
								break;
							case 1: /* 支援系 */
								if ((tsd->autospell2_id == AL_HEAL || (tsd->autospell2_id == ALL_RESURRECTION && tbl->type != BL_PC)) &&
								    battle_check_undead(status_get_race(tbl), status_get_elem_type(tbl)))
									skill_castend_damage_id(target, tbl, tsd->autospell2_id, skilllv, tick, flag);
								else
									skill_castend_nodamage_id(target, tbl, tsd->autospell2_id, skilllv, tick, flag);
								break;
						}
					}
				}
			}
		}

		if (rdamage > 0)
			battle_delay_damage(tick + wd.amotion, target, src, rdamage, 0);
		if (t_sc_data) {
			if (t_sc_data[SC_AUTOCOUNTER].timer != -1 && t_sc_data[SC_AUTOCOUNTER].val4 > 0) {
				if (t_sc_data[SC_AUTOCOUNTER].val3 == src->id)
					battle_weapon_attack(target, src, tick, 0x8000 | t_sc_data[SC_AUTOCOUNTER].val1);
				status_change_end(target, SC_AUTOCOUNTER, -1);
			}
			if (t_sc_data[SC_POISONREACT].timer != -1 && t_sc_data[SC_POISONREACT].val4 > 0 && t_sc_data[SC_POISONREACT].val3 == src->id) {   // poison react [Celest]
				struct map_session_data *tsd = (struct map_session_data *)target;
				if (status_get_elem_type(src) == 5) {
					t_sc_data[SC_POISONREACT].val2 = 0;
					battle_weapon_attack(target, src, tick, flag | AS_POISONREACT);
				} else {
					skill_use_id(tsd, src->id, TF_POISON, 5);
					--t_sc_data[SC_POISONREACT].val2;
				}
				if (t_sc_data[SC_POISONREACT].val2 <= 0)
					status_change_end(target, SC_POISONREACT, -1);
			}
			if (t_sc_data[SC_BLADESTOP_WAIT].timer != -1 &&
			    !(status_get_mode(src)&0x20)) { // ボスには無効
				int lv = t_sc_data[SC_BLADESTOP_WAIT].val1;
				status_change_end(target,SC_BLADESTOP_WAIT,-1);
				status_change_start(src,SC_BLADESTOP,lv,1,(int)src,(int)target,skill_get_time2(MO_BLADESTOP,lv),0);
				status_change_start(target,SC_BLADESTOP,lv,2,(int)target,(int)src,skill_get_time2(MO_BLADESTOP,lv),0);
			}
			if(t_sc_data[SC_SPLASHER].timer!=-1)	//殴ったので対象のベナムスプラッシャー状態を解除
				status_change_end(target,SC_SPLASHER,-1);
		}

		map_freeblock_unlock();
	}

	return wd.dmg_lv;
}

int battle_check_undead(int race,int element)
{
	if(battle_config.undead_detect_type == 0) {
		if(element == 9)
			return 1;
	}
	else if(battle_config.undead_detect_type == 1) {
		if(race == 1)
			return 1;
	}
	else {
		if(element == 9 || race == 1)
			return 1;
	}

	return 0;
}

/*==========================================
 * 敵味方判定(1=肯定,0=否定,-1=エラー)
 * flag&0xf0000 = 0x00000:敵じゃないか判定（ret:1＝敵ではない）
 *				= 0x10000:パーティー判定（ret:1=パーティーメンバ)
 *				= 0x20000:全て(ret:1=敵味方両方)
 *				= 0x40000:敵か判定(ret:1=敵)
 *				= 0x50000:パーティーじゃないか判定(ret:1=パーティでない)
 *------------------------------------------
 */
int battle_check_target( struct block_list *src, struct block_list *target, int flag)
{
	int s_p, s_g, t_p, t_g;
	struct block_list *ss = src;
	struct status_change *sc_data;
	struct status_change *tsc_data;

	nullpo_retr(0, src);
	nullpo_retr(0, target);

	if (flag & 0x40000) { // 反転フラグ
		int ret = battle_check_target(src, target, flag & 0x30000);
		if (ret != -1)
			return !ret;
		return -1;
	}

	if (flag & 0x20000) {
		if (target->type == BL_MOB || target->type == BL_PC)
			return 1;
		else
			return -1;
	}

	if (src->type == BL_SKILL && target->type == BL_SKILL) // 対象がスキルユニットなら無条件肯定
		return -1;

	if (target->type == BL_PET)
		return -1;

	if (target->type == BL_PC && ((struct map_session_data *)target)->invincible_timer != -1)
		return -1;

	// Celest
	sc_data = status_get_sc_data(src);
	tsc_data = status_get_sc_data(target);
	if ((sc_data && sc_data[SC_BASILICA].timer != -1) ||
	    (tsc_data && tsc_data[SC_BASILICA].timer != -1))
		return -1;

	if (target->type == BL_SKILL) {
		switch(((struct skill_unit *)target)->group->unit_id) {
		case 0x8d:
		case 0x8f:
		case 0x98:
			return 0;
			break;
		}
	}

	// スキルユニットの場合、親を求める
	if (src->type == BL_SKILL) {
		struct skill_unit *su = (struct skill_unit *)src;
		if (su && su->group) {
			int skillid, inf2;
			skillid = su->group->skill_id;
			inf2 = skill_get_inf2(skillid);
			if ((ss = map_id2bl(su->group->src_id)) == NULL)
				return -1;
			if (ss->prev == NULL)
				return -1;
			if (inf2 & 0x80 &&
			    (map[src->m].flag.pvp || pc_iskiller((struct map_session_data *)src, (struct map_session_data *)target) ||
			     (skillid >= 115 && skillid <= 125 && map[src->m].flag.gvg)) &&
			    !(target->type == BL_PC && pc_isinvisible((struct map_session_data *)target)))
				return 0;
			if (ss == target) {
				if (inf2 & 0x100)
					return 0;
				if (inf2 & 0x200)
					return -1;
			}
		}
	}

	// Mobでmaster_idがあってspecial_mob_aiなら、召喚主を求める
	if (src->type == BL_MOB) {
		struct mob_data *md=(struct mob_data *)src;
		if(md && md->master_id>0){
			if (md->master_id == target->id)	// 主なら肯定
				return 1;
			if (md->state.special_mob_ai) { // 0: nothing, 1: cannibalize, 2-3: spheremine
				if (target->type==BL_MOB) {	//special_mob_aiで対象がMob
					struct mob_data *tmd=(struct mob_data *)target;
					if(tmd){
						if (tmd->master_id != md->master_id)	//召喚主が一緒でなければ否定
							return 0;
						else{	//召喚主が一緒なので肯定したいけど自爆は否定
							if (md->state.special_mob_ai > 2) // 0: nothing, 1: cannibalize, 2-3: spheremine
								return 0;
							else
								return 1;
						}
					}
				}
			}
			if ((ss = map_id2bl(md->master_id)) == NULL)
				return -1;
		}
	}

	if( src==target || ss==target )	// 同じなら肯定
		return 1;

	if(target->type == BL_PC && pc_isinvisible((struct map_session_data *)target))
		return -1;

	if( src->prev==NULL ||	// 死んでるならエラー
		(src->type==BL_PC && pc_isdead((struct map_session_data *)src) ) )
		return -1;

	if( (ss->type == BL_PC && target->type==BL_MOB) ||
		(ss->type == BL_MOB && target->type==BL_PC) )
		return 0;	// PCvsMOBなら否定

	if(ss->type == BL_PET && target->type==BL_MOB)
		return 0;

	s_p = status_get_party_id(ss);
	s_g = status_get_guild_id(ss);

	t_p = status_get_party_id(target);
	t_g = status_get_guild_id(target);

	if(flag&0x10000) {
		if(s_p && t_p && s_p == t_p)	// 同じパーティなら肯定（味方）
			return 1;
		else		// パーティ検索なら同じパーティじゃない時点で否定
			return 0;
	}

	if(ss->type == BL_MOB && s_g > 0 && t_g > 0 && s_g == t_g )	// 同じギルド/mobクラスなら肯定（味方）
		return 1;

//printf("ss:%d src:%d target:%d flag:0x%x %d %d ",ss->id,src->id,target->id,flag,src->type,target->type);
//printf("p:%d %d g:%d %d\n",s_p,t_p,s_g,t_g);

	if( ss->type==BL_PC && target->type==BL_PC) { // 両方PVPモードなら否定（敵）
		struct skill_unit *su=NULL;
		if(src->type==BL_SKILL)
			su=(struct skill_unit *)src;
		if(map[ss->m].flag.pvp || pc_iskiller((struct map_session_data *)ss, (struct map_session_data*)target)) { // [MouseJstr]
			if(su && su->group->target_flag==BCT_NOENEMY)
				return 1;
			else if (battle_config.pk_mode &&
			         (((struct map_session_data*)ss)->status.class == 0 || ((struct map_session_data*)target)->status.class == 0 ||
			          ((struct map_session_data*)ss)->status.base_level < battle_config.pk_min_level ||
			          ((struct map_session_data*)target)->status.base_level < battle_config.pk_min_level))
				return 1; // prevent novice engagement in pk_mode [Valaris]
			else if (map[ss->m].flag.pvp_noparty && s_p > 0 && t_p > 0 && s_p == t_p)
				return 1;
			else if (map[ss->m].flag.pvp_noguild && s_g > 0 && t_g > 0 && s_g == t_g)
				return 1;
			return 0;
		}
		if (map[src->m].flag.gvg) {
			struct guild *g = NULL;
			if (su && su->group->target_flag == BCT_NOENEMY)
				return 1;
			if (s_g > 0 && s_g == t_g)
				return 1;
			if (map[src->m].flag.gvg_noparty && s_p > 0 && t_p > 0 && s_p == t_p)
				return 1;
			if ((g = guild_search(s_g))) {
				int i;
				for(i = 0; i < MAX_GUILDALLIANCE; i++) {
					if (g->alliance[i].guild_id > 0 && g->alliance[i].guild_id == t_g) {
						if (g->alliance[i].opposition)
							return 0;//敵対ギルドなら無条件に敵
						else
							return 1;//同盟ギルドなら無条件に味方
					}
				}
			}
			return 0;
		}
	}

	return 1;	// 該当しないので無関係人物（まあ敵じゃないので味方）
}

/*==========================================
 * 射程判定
 *------------------------------------------
 */
int battle_check_range(struct block_list *src, struct block_list *bl, int range) {
	int dx, dy;
//	struct walkpath_data wpd;
	int arange;

	nullpo_retr(0, src);
	nullpo_retr(0, bl);

	dx = abs(bl->x - src->x);
	dy = abs(bl->y - src->y);
	arange = ((dx > dy) ? dx : dy);

	if (src->m != bl->m) // 違うマップ
		return 0;

	if (range > 0 && range < arange) // 遠すぎる
		return 0;

	if (arange < 2) // 同じマスか隣接
		return 1;

//	if(bl->type == BL_SKILL && ((struct skill_unit *)bl)->group->unit_id == 0x8d)
//		return 1;

// restored, because path_search_long doesn't check correctly wall -------
/*	// 障害物判定
	wpd.path_len = 0;
	wpd.path_pos = 0;
	wpd.path_half = 0;
	if (path_search(&wpd, src->m, src->x, src->y, bl->x, bl->y, 0x10001) != -1)
		return 1;

	dx = (dx > 0) ? 1 : ((dx < 0) ? -1 : 0);
	dy = (dy > 0) ? 1 : ((dy < 0) ? -1 : 0);

	return (path_search(&wpd, src->m, src->x + dx, src->y + dy, bl->x - dx, bl->y - dy, 0x10001) != -1) ? 1 : 0;*/
//-------------------------------------------------------------------------

	// 障害物判定
	return path_search_long(src->m, src->x, src->y, bl->x, bl->y); // doesn't check correctly wall
}

static const struct {
	char str[128];
	int *val;
} battle_data[] = {
	{ "warp_point_debug",                           &battle_config.warp_point_debug },
	{ "enemy_critical",                             &battle_config.enemy_critical },
	{ "enemy_critical_rate",                        &battle_config.enemy_critical_rate },
	{ "enemy_str",                                  &battle_config.enemy_str },
	{ "enemy_perfect_flee",                         &battle_config.enemy_perfect_flee },
	{ "casting_rate",                               &battle_config.cast_rate },
	{ "delay_rate",                                 &battle_config.delay_rate },
	{ "delay_dependon_dex",                         &battle_config.delay_dependon_dex },
	{ "skill_delay_attack_enable",                  &battle_config.sdelay_attack_enable },
	{ "left_cardfix_to_right",                      &battle_config.left_cardfix_to_right },
	{ "player_skill_add_range",                     &battle_config.pc_skill_add_range },
	{ "skill_out_range_consume",                    &battle_config.skill_out_range_consume },
	{ "monster_skill_add_range",                    &battle_config.mob_skill_add_range },
	{ "player_damage_delay",                        &battle_config.pc_damage_delay },
	{ "player_damage_delay_rate",                   &battle_config.pc_damage_delay_rate },
	{ "defunit_not_enemy",                          &battle_config.defnotenemy },
	{ "random_monster_checklv",                     &battle_config.random_monster_checklv },
	{ "attribute_recover",                          &battle_config.attr_recover },
	{ "flooritem_lifetime",                         &battle_config.flooritem_lifetime },
	{ "item_auto_get",                              &battle_config.item_auto_get }, // drop rate of items concerned by the autoloot (from 0% to x%).
	{ "item_auto_get_loot",                         &battle_config.item_auto_get_loot },
	{ "item_auto_get_distance",                     &battle_config.item_auto_get_distance },
	{ "item_first_get_time",                        &battle_config.item_first_get_time },
	{ "item_second_get_time",                       &battle_config.item_second_get_time },
	{ "item_third_get_time",                        &battle_config.item_third_get_time },
	{ "mvp_item_first_get_time",                    &battle_config.mvp_item_first_get_time },
	{ "mvp_item_second_get_time",                   &battle_config.mvp_item_second_get_time },
	{ "mvp_item_third_get_time",                    &battle_config.mvp_item_third_get_time },
	{ "drop_rate0item",                             &battle_config.drop_rate0item },
	{ "base_exp_rate",                              &battle_config.base_exp_rate },
	{ "job_exp_rate",                               &battle_config.job_exp_rate },
	{ "pvp_exp",                                    &battle_config.pvp_exp },
	{ "gtb_pvp_only",                               &battle_config.gtb_pvp_only },
	{ "guild_max_castles",                          &battle_config.guild_max_castles },
	{ "death_penalty_type_base",                    &battle_config.death_penalty_type_base },
	{ "death_penalty_base",                         &battle_config.death_penalty_base },
	{ "death_penalty_base2_lvl",                    &battle_config.death_penalty_base2_lvl },
	{ "death_penalty_base2",                        &battle_config.death_penalty_base2 },
	{ "death_penalty_base3_lvl",                    &battle_config.death_penalty_base3_lvl },
	{ "death_penalty_base3",                        &battle_config.death_penalty_base3 },
	{ "death_penalty_base4_lvl",                    &battle_config.death_penalty_base4_lvl },
	{ "death_penalty_base4",                        &battle_config.death_penalty_base4 },
	{ "death_penalty_base5_lvl",                    &battle_config.death_penalty_base5_lvl },
	{ "death_penalty_base5",                        &battle_config.death_penalty_base5 },
	{ "death_penalty_type_job",                     &battle_config.death_penalty_type_job },
	{ "death_penalty_job",                          &battle_config.death_penalty_job },
	{ "death_penalty_job2_lvl",                     &battle_config.death_penalty_job2_lvl },
	{ "death_penalty_job2",                         &battle_config.death_penalty_job2 },
	{ "death_penalty_job3_lvl",                     &battle_config.death_penalty_job3_lvl },
	{ "death_penalty_job3",                         &battle_config.death_penalty_job3 },
	{ "death_penalty_job4_lvl",                     &battle_config.death_penalty_job4_lvl },
	{ "death_penalty_job4",                         &battle_config.death_penalty_job4 },
	{ "death_penalty_job5_lvl",                     &battle_config.death_penalty_job5_lvl },
	{ "death_penalty_job5",                         &battle_config.death_penalty_job5 },
	{ "zeny_penalty",                               &battle_config.zeny_penalty },
	{ "zeny_penalty_percent",                       &battle_config.zeny_penalty_percent },
	{ "zeny_penalty_by_lvl",                        &battle_config.zeny_penalty_by_lvl },
	{ "restart_hp_rate",                            &battle_config.restart_hp_rate },
	{ "restart_sp_rate",                            &battle_config.restart_sp_rate },
	{ "mvp_hp_rate",                                &battle_config.mvp_hp_rate },
	{ "mvp_item_rate",                              &battle_config.mvp_item_rate },
	{ "mvp_exp_rate",                               &battle_config.mvp_exp_rate },
	{ "monster_hp_rate",                            &battle_config.monster_hp_rate },
	{ "monster_max_aspd",                           &battle_config.monster_max_aspd },
	{ "atcommand_gm_only",                          &battle_config.atc_gmonly },
	{ "atcommand_spawn_quantity_limit",             &battle_config.atc_spawn_quantity_limit },
	{ "atcommand_map_mob_limit",                    &battle_config.atc_map_mob_limit },
	{ "atcommand_local_spawn_interval",             &battle_config.atc_local_spawn_interval },
	{ "gm_all_skill",                               &battle_config.gm_allskill },
	{ "gm_all_skill_job",                           &battle_config.gm_all_skill_job },
	{ "gm_all_skill_platinum",                      &battle_config.gm_all_skill_platinum },
	{ "gm_all_skill_add_abra",                      &battle_config.gm_allskill_addabra },
	{ "gm_all_equipment",                           &battle_config.gm_allequip },
	{ "gm_skill_unconditional",                     &battle_config.gm_skilluncond },
	{ "player_skillfree",                           &battle_config.skillfree },
	{ "player_skillup_limit",                       &battle_config.skillup_limit },
	{ "check_minimum_skill_points",                 &battle_config.check_minimum_skill_points },
	{ "check_maximum_skill_points",                 &battle_config.check_maximum_skill_points },
	{ "weapon_produce_rate",                        &battle_config.wp_rate },
	{ "potion_produce_rate",                        &battle_config.pp_rate },
	{ "deadly_potion_produce_rate",                 &battle_config.cdp_rate },
	{ "monster_active_enable",                      &battle_config.monster_active_enable },
	{ "monster_damage_delay_rate",                  &battle_config.monster_damage_delay_rate},
	{ "monster_loot_type",                          &battle_config.monster_loot_type },
	{ "mob_skill_use",                              &battle_config.mob_skill_use },
	{ "mob_count_rate",                             &battle_config.mob_count_rate },
	{ "quest_skill_learn",                          &battle_config.quest_skill_learn },
	{ "quest_skill_reset",                          &battle_config.quest_skill_reset },
	{ "basic_skill_check",                          &battle_config.basic_skill_check },
	{ "no_caption_cloaking",                        &battle_config.no_caption_cloaking }, // remove the caption "cloaking !!" when skill is casted
	{ "display_hallucination",                      &battle_config.display_hallucination }, // Enable or disable hallucination Skill.
	{ "no_guilds_glory",                            &battle_config.no_guilds_glory },
	{ "guild_emperium_check",                       &battle_config.guild_emperium_check },
	{ "guild_exp_limit",                            &battle_config.guild_exp_limit },
	{ "player_invincible_time",                     &battle_config.pc_invincible_time },
	{ "pet_birth_effect",                           &battle_config.pet_birth_effect },
	{ "pet_catch_rate",                             &battle_config.pet_catch_rate },
	{ "pet_rename",                                 &battle_config.pet_rename },
	{ "pet_friendly_rate",                          &battle_config.pet_friendly_rate },
	{ "pet_hungry_delay_rate",                      &battle_config.pet_hungry_delay_rate },
	{ "pet_hungry_friendly_decrease",               &battle_config.pet_hungry_friendly_decrease},
	{ "pet_str",                                    &battle_config.pet_str },
	{ "pet_status_support",                         &battle_config.pet_status_support },
	{ "pet_attack_support",                         &battle_config.pet_attack_support },
	{ "pet_damage_support",                         &battle_config.pet_damage_support },
	{ "pet_support_rate",                           &battle_config.pet_support_rate },
	{ "pet_attack_exp_to_master",                   &battle_config.pet_attack_exp_to_master },
	{ "pet_attack_exp_rate",                        &battle_config.pet_attack_exp_rate },
	{ "skill_min_damage",                           &battle_config.skill_min_damage },
	{ "finger_offensive_type",                      &battle_config.finger_offensive_type },
	{ "heal_exp",                                   &battle_config.heal_exp },
	{ "resurrection_exp",                           &battle_config.resurrection_exp },
	{ "shop_exp",                                   &battle_config.shop_exp },
	{ "combo_delay_rate",                           &battle_config.combo_delay_rate },
	{ "item_check",                                 &battle_config.item_check },
	{ "item_use_interval",                          &battle_config.item_use_interval },
	{ "wedding_modifydisplay",                      &battle_config.wedding_modifydisplay },
	{ "natural_healhp_interval",                    &battle_config.natural_healhp_interval },
	{ "natural_healsp_interval",                    &battle_config.natural_healsp_interval },
	{ "natural_heal_skill_interval",                &battle_config.natural_heal_skill_interval},
	{ "natural_heal_weight_rate",                   &battle_config.natural_heal_weight_rate },
	{ "item_name_override_grffile",                 &battle_config.item_name_override_grffile},
	{ "item_equip_override_grffile",                &battle_config.item_equip_override_grffile},	// [Celest]
	{ "item_slots_override_grffile",                &battle_config.item_slots_override_grffile},	// [Celest]
	{ "indoors_override_grffile",                   &battle_config.indoors_override_grffile},	// [Celest]
	{ "skill_sp_override_grffile",                  &battle_config.skill_sp_override_grffile},	// [Celest]
	{ "cardillust_read_grffile",                    &battle_config.cardillust_read_grffile},	// [Celest]
	{ "arrow_decrement",                            &battle_config.arrow_decrement },
	{ "max_aspd",                                   &battle_config.max_aspd },
	{ "max_hp",                                     &battle_config.max_hp },
	{ "max_sp",                                     &battle_config.max_sp },
	{ "max_lv",                                     &battle_config.max_lv },
	{ "max_parameter",                              &battle_config.max_parameter },
	{ "max_cart_weight",                            &battle_config.max_cart_weight },
	{ "player_skill_log",                           &battle_config.pc_skill_log },
	{ "monster_skill_log",                          &battle_config.mob_skill_log },
	{ "battle_log",                                 &battle_config.battle_log },
	{ "save_log",                                   &battle_config.save_log },
	{ "error_log",                                  &battle_config.error_log },
	{ "etc_log",                                    &battle_config.etc_log },
	{ "save_clothcolor",                            &battle_config.save_clothcolor },
	{ "undead_detect_type",                         &battle_config.undead_detect_type },
	{ "player_auto_counter_type",                   &battle_config.pc_auto_counter_type },
	{ "monster_auto_counter_type",                  &battle_config.monster_auto_counter_type},
	{ "agi_penalty_type",                           &battle_config.agi_penalty_type },
	{ "agi_penalty_count",                          &battle_config.agi_penalty_count },
	{ "agi_penalty_num",                            &battle_config.agi_penalty_num },
	{ "agi_penalty_count_lv",                       &battle_config.agi_penalty_count_lv },
	{ "vit_penalty_type",                           &battle_config.vit_penalty_type },
	{ "vit_penalty_count",                          &battle_config.vit_penalty_count },
	{ "vit_penalty_num",                            &battle_config.vit_penalty_num },
	{ "vit_penalty_count_lv",                       &battle_config.vit_penalty_count_lv },
	{ "player_defense_type",                        &battle_config.player_defense_type },
	{ "monster_defense_type",                       &battle_config.monster_defense_type },
	{ "pet_defense_type",                           &battle_config.pet_defense_type },
	{ "magic_defense_type",                         &battle_config.magic_defense_type },
	{ "player_skill_reiteration",                   &battle_config.pc_skill_reiteration },
	{ "monster_skill_reiteration",                  &battle_config.monster_skill_reiteration},
	{ "player_skill_nofootset",                     &battle_config.pc_skill_nofootset },
	{ "monster_skill_nofootset",                    &battle_config.monster_skill_nofootset },
	{ "player_cloak_check_type",                    &battle_config.pc_cloak_check_type },
	{ "monster_cloak_check_type",                   &battle_config.monster_cloak_check_type },
	{ "gvg_short_attack_damage_rate",               &battle_config.gvg_short_damage_rate },
	{ "gvg_long_attack_damage_rate",                &battle_config.gvg_long_damage_rate },
	{ "gvg_magic_attack_damage_rate",               &battle_config.gvg_magic_damage_rate },
	{ "gvg_misc_attack_damage_rate",                &battle_config.gvg_misc_damage_rate },
	{ "gvg_eliminate_time",                         &battle_config.gvg_eliminate_time },
	{ "mob_changetarget_byskill",                   &battle_config.mob_changetarget_byskill},
	{ "player_attack_direction_change",             &battle_config.pc_attack_direction_change },
	{ "monster_attack_direction_change",            &battle_config.monster_attack_direction_change },
	{ "player_land_skill_limit",                    &battle_config.pc_land_skill_limit },
	{ "monster_land_skill_limit",                   &battle_config.monster_land_skill_limit},
	{ "party_skill_penalty",                        &battle_config.party_skill_penalty },
	{ "monster_class_change_full_recover",          &battle_config.monster_class_change_full_recover },
	{ "produce_item_name_input",                    &battle_config.produce_item_name_input },
	{ "produce_potion_name_input",                  &battle_config.produce_potion_name_input},
	{ "making_arrow_name_input",                    &battle_config.making_arrow_name_input },
	{ "holywater_name_input",                       &battle_config.holywater_name_input },
	{ "atcommand_item_creation_name_input",         &battle_config.atcommand_item_creation_name_input },
	{ "display_delay_skill_fail",                   &battle_config.display_delay_skill_fail },
	{ "display_snatcher_skill_fail",                &battle_config.display_snatcher_skill_fail	},
	{ "chat_warpportal",                            &battle_config.chat_warpportal },
	{ "mob_warpportal",                             &battle_config.mob_warpportal },
	{ "dead_branch_active",                         &battle_config.dead_branch_active },
	{ "vending_max_value",                          &battle_config.vending_max_value },
//	{ "pet_lootitem",                               &battle_config.pet_lootitem },
//	{ "pet_weight",                                 &battle_config.pet_weight },
	{ "show_steal_in_same_party",                   &battle_config.show_steal_in_same_party },
	{ "enable_upper_class",                         &battle_config.enable_upper_class },
	{ "pet_attack_attr_none",                       &battle_config.pet_attack_attr_none },
	{ "mob_attack_attr_none",                       &battle_config.mob_attack_attr_none },
	{ "mob_ghostring_fix",                          &battle_config.mob_ghostring_fix },
	{ "pc_attack_attr_none",                        &battle_config.pc_attack_attr_none },
	{ "gx_allhit",                                  &battle_config.gx_allhit },
	{ "gx_cardfix",                                 &battle_config.gx_cardfix },
	{ "gx_dupele",                                  &battle_config.gx_dupele },
	{ "gx_disptype",                                &battle_config.gx_disptype },
	{ "devotion_level_difference",                  &battle_config.devotion_level_difference	},
	{ "player_skill_partner_check",                 &battle_config.player_skill_partner_check},
	{ "hide_GM_session",                            &battle_config.hide_GM_session }, // minimum level of hidden GMs
	{ "unit_movement_type",                         &battle_config.unit_movement_type },
	{ "invite_request_check",                       &battle_config.invite_request_check }, // Are other requests accepted during a request or not?
	{ "skill_removetrap_type",                      &battle_config.skill_removetrap_type },
	{ "disp_experience",                            &battle_config.disp_experience },
	{ "disp_experience_type",                       &battle_config.disp_experience_type }, // with/without %
	{ "castle_defense_rate",                        &battle_config.castle_defense_rate },
	{ "riding_weight",                              &battle_config.riding_weight },
	{ "hp_rate",                                    &battle_config.hp_rate },
	{ "sp_rate",                                    &battle_config.sp_rate },
	{ "gm_can_drop_lv",                             &battle_config.gm_can_drop_lv },
	{ "disp_hpmeter",                               &battle_config.disp_hpmeter },
	{ "bone_drop",                                  &battle_config.bone_drop },

	{ "item_rate_common",                           &battle_config.item_rate_common },
	{ "item_rate_equip",                            &battle_config.item_rate_equip },
	{ "item_rate_card",                             &battle_config.item_rate_card },
	{ "item_rate_heal",                             &battle_config.item_rate_heal },
	{ "item_rate_use",                              &battle_config.item_rate_use },
	{ "item_drop_common_min",                       &battle_config.item_drop_common_min }, // Added by TyrNemesis^
	{ "item_drop_common_max",                       &battle_config.item_drop_common_max },
	{ "item_drop_equip_min",                        &battle_config.item_drop_equip_min },
	{ "item_drop_equip_max",                        &battle_config.item_drop_equip_max },
	{ "item_drop_card_min",                         &battle_config.item_drop_card_min },
	{ "item_drop_card_max",                         &battle_config.item_drop_card_max },
	{ "item_drop_mvp_min",                          &battle_config.item_drop_mvp_min },
	{ "item_drop_mvp_max",                          &battle_config.item_drop_mvp_max }, // End Addition
	{ "item_drop_heal_min",                         &battle_config.item_drop_heal_min },
	{ "item_drop_heal_max",                         &battle_config.item_drop_heal_max },
	{ "item_drop_use_min",                          &battle_config.item_drop_use_min },
	{ "item_drop_use_max",                          &battle_config.item_drop_use_max },
	{ "prevent_logout",                             &battle_config.prevent_logout }, // Added by RoVeRT
	{ "alchemist_summon_reward",                    &battle_config.alchemist_summon_reward }, // [Valaris]
	{ "maximum_level",                              &battle_config.maximum_level }, // [Valaris]
	{ "atcommand_max_job_level_novice",             &battle_config.atcommand_max_job_level_novice },
	{ "atcommand_max_job_level_job1",               &battle_config.atcommand_max_job_level_job1 },
	{ "atcommand_max_job_level_job2",               &battle_config.atcommand_max_job_level_job2 },
	{ "atcommand_max_job_level_supernovice",        &battle_config.atcommand_max_job_level_supernovice },
	{ "atcommand_max_job_level_highnovice",         &battle_config.atcommand_max_job_level_highnovice },
	{ "atcommand_max_job_level_highjob1",           &battle_config.atcommand_max_job_level_highjob1 },
	{ "atcommand_max_job_level_highjob2",           &battle_config.atcommand_max_job_level_highjob2 },
	{ "atcommand_max_job_level_babynovice",         &battle_config.atcommand_max_job_level_babynovice },
	{ "atcommand_max_job_level_babyjob1",           &battle_config.atcommand_max_job_level_babyjob1 },
	{ "atcommand_max_job_level_babyjob2",           &battle_config.atcommand_max_job_level_babyjob2 },
	{ "atcommand_max_job_level_superbaby",          &battle_config.atcommand_max_job_level_superbaby },
	{ "drops_by_luk",                               &battle_config.drops_by_luk }, // [Valaris]
	{ "monsters_ignore_gm",                         &battle_config.monsters_ignore_gm }, // [Valaris]
	{ "equipment_breaking",                         &battle_config.equipment_breaking }, // [Valaris]
	{ "equipment_break_rate",                       &battle_config.equipment_break_rate }, // [Valaris]
	{ "pk_mode",                                    &battle_config.pk_mode }, // [Valaris]
	{ "pet_equip_required",                         &battle_config.pet_equip_required }, // [Valaris]
	{ "multi_level_up",                             &battle_config.multi_level_up }, // [Valaris]
	{ "backstab_bow_penalty",                       &battle_config.backstab_bow_penalty },
	{ "night_at_start",                             &battle_config.night_at_start }, // added by [Yor]
	{ "day_duration",                               &battle_config.day_duration }, // added by [Yor]
	{ "night_duration",                             &battle_config.night_duration }, // added by [Yor]
	{ "show_mob_hp",                                &battle_config.show_mob_hp }, // [Valaris]
	{ "ban_spoof_namer",                            &battle_config.ban_spoof_namer }, // added by [Yor]
	{ "ban_hack_trade",                             &battle_config.ban_hack_trade }, // added by [Yor]
	{ "ban_bot",                                    &battle_config.ban_bot }, // added by [Yor]
	{ "check_ban_bot",                              &battle_config.check_ban_bot }, // added by [Yor]
	{ "max_message_length",                         &battle_config.max_message_length }, // added by [Yor]
	{ "max_global_message_length",                  &battle_config.max_global_message_length }, // added by [Yor]
	{ "hack_info_GM_level",                         &battle_config.hack_info_GM_level }, // added by [Yor]
	{ "speed_hack_info_GM_level",                   &battle_config.speed_hack_info_GM_level }, // added by [Yor]
	{ "any_warp_GM_min_level",                      &battle_config.any_warp_GM_min_level }, // added by [Yor]
	{ "packet_ver_flag",                            &battle_config.packet_ver_flag }, // added by [Yor]
	{ "min_hair_style",                             &battle_config.min_hair_style }, // added by [Yor]
	{ "max_hair_style",                             &battle_config.max_hair_style }, // added by [Yor]
	{ "min_hair_color",                             &battle_config.min_hair_color }, // added by [Yor]
	{ "max_hair_color",                             &battle_config.max_hair_color }, // added by [Yor]
	{ "min_cloth_color",                            &battle_config.min_cloth_color }, // added by [Yor]
	{ "max_cloth_color",                            &battle_config.max_cloth_color }, // added by [Yor]
	{ "clothes_color_for_assassin",                 &battle_config.clothes_color_for_assassin }, // added by [Yor]
	{ "castrate_dex_scale",                         &battle_config.castrate_dex_scale }, // added by [Yor]
	{ "area_size",                                  &battle_config.area_size }, // added by [Yor]
	{ "muting_players",                             &battle_config.muting_players }, // added by [Apple]
	{ "zeny_from_mobs",                             &battle_config.zeny_from_mobs}, // [Valaris]
	{ "mobs_level_up",                              &battle_config.mobs_level_up}, // [Valaris]
	{ "pk_min_level",                               &battle_config.pk_min_level}, // [celest]
	{ "skill_steal_type",                           &battle_config.skill_steal_type}, // [celest]
	{ "skill_steal_rate",                           &battle_config.skill_steal_rate}, // [celest]
//	{ "night_darkness_level",                       &battle_config.night_darkness_level}, // [celest] // ************** This option is not used! : we don't know how to remove effect (night -> day)
	{ "skill_range_leniency",                       &battle_config.skill_range_leniency}, // [celest]
	{ "motd_type",                                  &battle_config.motd_type}, // [celest]
	{ "allow_atcommand_when_mute",                  &battle_config.allow_atcommand_when_mute}, // [celest]
	{ "manner_action",                              &battle_config.manner_action}, // [Yor]
	{ "finding_ore_rate",                           &battle_config.finding_ore_rate}, // [celest]
	{ "min_skill_delay_limit",                      &battle_config.min_skill_delay_limit}, // [celest]
	{ "idle_no_share",                              &battle_config.idle_no_share}, // [celest], for a feature by [MouseJstr]
	{ "chat_no_share",                              &battle_config.chat_no_share}, // [Yor]
	{ "npc_chat_no_share",                          &battle_config.npc_chat_no_share}, // [Yor]
	{ "shop_no_share",                              &battle_config.shop_no_share}, // [Yor]
	{ "trade_no_share",                             &battle_config.trade_no_share}, // [Yor]
	{ "idle_before_disconnect",                     &battle_config.idle_disconnect}, // [Yor]
	{ "idle_before_disconnect_chat",                &battle_config.idle_disconnect_chat}, // [Yor]
	{ "idle_before_disconnect_vender",              &battle_config.idle_disconnect_vender}, // [Yor]
	{ "idle_before_disconnect_disable_for_restore", &battle_config.idle_disconnect_disable_for_restore}, // [Yor]
	{ "idle_before_disconnect_ignore_GM",           &battle_config.idle_disconnect_ignore_GM}, // [Yor]
	{ "jail_message",                               &battle_config.jail_message}, // [Yor] Do we send message to ALL players when a player is put in jail?
	{ "jail_discharge_message",                     &battle_config.jail_discharge_message}, // [Yor] Do we send message to ALL players when a player is discharged?
	{ "mingmlvl_message",                           &battle_config.mingmlvl_message}, // [Yor] Which message do we send when a GM can use a command, but mingmlvl map flag block it?
	{ "check_invalid_slot",                         &battle_config.check_invalid_slot}, // [Yor] Do we check invalid slotted cards?
	{ "ruwach_range",                               &battle_config.ruwach_range}, // [Yor] Set the range (number of squares/tiles around you) of 'ruwach' skill to detect invisible.
	{ "sight_range",                                &battle_config.sight_range}, // [Yor] Set the range (number of squares/tiles around you) of 'sight' skill to detect invisible.
	{ "max_icewall",                                &battle_config.max_icewall}, // [Yor] Set maximum number of ice walls active at the same time.

	{ "atcommand_main_channel_at_start",            &battle_config.atcommand_main_channel_at_start},
	{ "atcommand_min_GM_level_for_request",         &battle_config.atcommand_min_GM_level_for_request},
	{ "atcommand_follow_stop_dead_target",          &battle_config.atcommand_follow_stop_dead_target},
	{ "atcommand_add_local_message_info",           &battle_config.atcommand_add_local_message_info},
	{ "atcommand_storage_on_pvp_map",               &battle_config.atcommand_storage_on_pvp_map},
	{ "atcommand_gstorage_on_pvp_map",              &battle_config.atcommand_gstorage_on_pvp_map},

	{ "pm_gm_not_ignored",                          &battle_config.pm_gm_not_ignored}, // GM minimum level to be not ignored in private message. [BeoWulf] (from freya's bug report)

	{ "char_disconnect_mode",                       &battle_config.char_disconnect_mode},

	{ "autospell_consume_sp",                       &battle_config.autospell_consume_sp},

//SQL-only options start
#ifdef USE_SQL
	{ "mail_system",                                &battle_config.mail_system }, // added by [Valaris]
//SQL-only options end
#endif /* USE_SQL */
};

int battle_set_value(char *w1, char *w2) {
	int i;

	for(i = 0; i < sizeof(battle_data) / (sizeof(battle_data[0])); i++)
		if (strcasecmp(w1, battle_data[i].str) == 0) {
			*battle_data[i].val = config_switch(w2);
			return 1;
		}

	return 0;
}

void battle_set_defaults() {
	battle_config.warp_point_debug=0;
	battle_config.enemy_critical=0;
	battle_config.enemy_critical_rate=100;
	battle_config.enemy_str=1;
	battle_config.enemy_perfect_flee=0;
	battle_config.cast_rate=100;
	battle_config.delay_rate=100;
	battle_config.delay_dependon_dex = 0;
	battle_config.sdelay_attack_enable = 0;
	battle_config.left_cardfix_to_right = 1;
	battle_config.pc_skill_add_range=0;
	battle_config.skill_out_range_consume = 0;
	battle_config.mob_skill_add_range=0;
	battle_config.pc_damage_delay = 1;
	battle_config.pc_damage_delay_rate = 100;
	battle_config.defnotenemy=1;
	battle_config.random_monster_checklv = 1;
	battle_config.attr_recover=1;
	battle_config.flooritem_lifetime=LIFETIME_FLOORITEM*1000;
	battle_config.item_auto_get = 0; // drop rate of items concerned by the autoloot (from 0% to x%).
	battle_config.item_auto_get_loot = 0;
	battle_config.item_auto_get_distance = 2; // max square between character and item
	battle_config.item_first_get_time=3000;
	battle_config.item_second_get_time=1000;
	battle_config.item_third_get_time=1000;
	battle_config.mvp_item_first_get_time=10000;
	battle_config.mvp_item_second_get_time=10000;
	battle_config.mvp_item_third_get_time=2000;

	battle_config.drop_rate0item = 0;
	battle_config.base_exp_rate = 100;
	battle_config.job_exp_rate = 100;
	battle_config.pvp_exp=1;
	battle_config.gtb_pvp_only = 0;
	battle_config.death_penalty_type_base = 1;
	battle_config.death_penalty_base = 100;
	battle_config.death_penalty_base2 = 100; battle_config.death_penalty_base2_lvl = 256;
	battle_config.death_penalty_base3 = 100; battle_config.death_penalty_base3_lvl = 256;
	battle_config.death_penalty_base4 = 100; battle_config.death_penalty_base4_lvl = 256;
	battle_config.death_penalty_base5 = 100; battle_config.death_penalty_base5_lvl = 256;
	battle_config.death_penalty_type_job = 1;
	battle_config.death_penalty_job = 100;
	battle_config.death_penalty_job2 = 100; battle_config.death_penalty_job2_lvl = 256;
	battle_config.death_penalty_job3 = 100; battle_config.death_penalty_job3_lvl = 256;
	battle_config.death_penalty_job4 = 100; battle_config.death_penalty_job4_lvl = 256;
	battle_config.death_penalty_job5 = 100; battle_config.death_penalty_job5_lvl = 256;
	battle_config.zeny_penalty = 0;
	battle_config.zeny_penalty_percent = 0;
	battle_config.zeny_penalty_by_lvl = 0;
	battle_config.restart_hp_rate = 0;
	battle_config.restart_sp_rate = 0;
	battle_config.mvp_item_rate = 100;
	battle_config.mvp_exp_rate = 100;
	battle_config.mvp_hp_rate = 100;
	battle_config.monster_hp_rate = 100;
	battle_config.monster_max_aspd = 199;
	battle_config.atc_gmonly = 0;
	battle_config.atc_spawn_quantity_limit = 100;
	battle_config.atc_map_mob_limit = 20000;
	battle_config.atc_local_spawn_interval = 0;
	battle_config.gm_allskill = 100; // 100 disabled
	battle_config.gm_all_skill_job = 60;
	battle_config.gm_all_skill_platinum = 0;
	battle_config.gm_allequip = 0;
	battle_config.gm_skilluncond = 0;
	battle_config.guild_max_castles = 0;
	battle_config.skillfree = 0;
	battle_config.skillup_limit = 1;
	battle_config.check_minimum_skill_points = 1; // yes
	battle_config.check_maximum_skill_points = -1; // -1: no
	battle_config.wp_rate = 100;
	battle_config.pp_rate = 100;
	battle_config.cdp_rate = 100;
	battle_config.monster_active_enable = 1;
	battle_config.monster_damage_delay_rate = 100;
	battle_config.monster_loot_type = 0;
	battle_config.mob_skill_use = 1;
	battle_config.mob_count_rate = 100; /* Rate of monsters on a map */
	battle_config.quest_skill_learn = 0;
	battle_config.quest_skill_reset = 0;
	battle_config.basic_skill_check = 1;
	battle_config.no_caption_cloaking = 0; // remove the caption "cloaking !!" when skill is casted
	battle_config.display_hallucination = 1; // Enable or disable hallucination Skill.
	battle_config.no_guilds_glory = 0;
	battle_config.guild_emperium_check = 1;
	battle_config.guild_exp_limit = 50;
	battle_config.pc_invincible_time = 5000;
	battle_config.pet_birth_effect = 1; // yes
	battle_config.pet_catch_rate = 100;
	battle_config.pet_rename = 0;
	battle_config.pet_friendly_rate = 100;
	battle_config.pet_hungry_delay_rate = 100;
	battle_config.pet_hungry_friendly_decrease = 5;
	battle_config.pet_str = 1;
	battle_config.pet_status_support = 1;
	battle_config.pet_attack_support = 0;
	battle_config.pet_damage_support = 0;
	battle_config.pet_support_rate = 100;
	battle_config.pet_attack_exp_to_master = 0;
	battle_config.pet_attack_exp_rate = 100;
	battle_config.skill_min_damage = 0;
	battle_config.finger_offensive_type = 0;
	battle_config.heal_exp = 0;
	battle_config.resurrection_exp = 0;
	battle_config.shop_exp = 0;
	battle_config.combo_delay_rate = 100;
	battle_config.item_check = 0;
	battle_config.item_use_interval = 150; // 150 ms -> Can you can hit a key more than 6 times/second?
	battle_config.wedding_modifydisplay = 1;
	battle_config.natural_healhp_interval=6000;
	battle_config.natural_healsp_interval=8000;
	battle_config.natural_heal_skill_interval=10000;
	battle_config.natural_heal_weight_rate=50;
	battle_config.item_name_override_grffile = 0;
	battle_config.item_equip_override_grffile = 0; // [Celest]
	battle_config.item_slots_override_grffile = 0; // [Celest]
	battle_config.indoors_override_grffile = 0; // [Celest]
	battle_config.skill_sp_override_grffile = 0; // [Celest]
	battle_config.cardillust_read_grffile = 1; // [Celest]
	battle_config.arrow_decrement = 1;
	battle_config.max_aspd = 190;
	battle_config.max_hp = 32500;
	battle_config.max_sp = 32500;
	battle_config.max_lv = 99; // [MouseJstr]
	battle_config.max_parameter = 99;
	battle_config.max_cart_weight = 8000;
	battle_config.pc_skill_log = 0;
	battle_config.mob_skill_log = 0;
	battle_config.battle_log = 0;
	battle_config.save_log = 0;
	battle_config.error_log = 0;
	battle_config.etc_log = 0;
	battle_config.save_clothcolor = 1; // save clothes color by default [Yor]
	battle_config.undead_detect_type = 0;
	battle_config.pc_auto_counter_type = 0;
	battle_config.monster_auto_counter_type = 0;
	battle_config.agi_penalty_type = 1;
	battle_config.agi_penalty_count = 3;
	battle_config.agi_penalty_num = 10;
	battle_config.agi_penalty_count_lv = ATK_FLEE;
	battle_config.vit_penalty_type = 1;
	battle_config.vit_penalty_count = 3;
	battle_config.vit_penalty_num = 5;
	battle_config.vit_penalty_count_lv = ATK_DEF;
	battle_config.player_defense_type = 0;
	battle_config.monster_defense_type = 0;
	battle_config.pet_defense_type = 0;
	battle_config.magic_defense_type = 0;
	battle_config.pc_skill_reiteration = 0;
	battle_config.monster_skill_reiteration = 0;
	battle_config.pc_skill_nofootset = 1;
	battle_config.monster_skill_nofootset = 1;
	battle_config.pc_cloak_check_type = 0;
	battle_config.monster_cloak_check_type = 0;
	battle_config.gvg_short_damage_rate = 100;
	battle_config.gvg_long_damage_rate = 60;
	battle_config.gvg_magic_damage_rate = 50;
	battle_config.gvg_misc_damage_rate = 60;
	battle_config.gvg_eliminate_time = 7000;
	battle_config.mob_changetarget_byskill = 0;
	battle_config.pc_attack_direction_change = 1;
	battle_config.monster_attack_direction_change = 1;
	battle_config.pc_land_skill_limit = 1;
	battle_config.monster_land_skill_limit = 1;
	battle_config.party_skill_penalty = 1;
	battle_config.monster_class_change_full_recover = 0;
	battle_config.produce_item_name_input = 1;
	battle_config.produce_potion_name_input = 1;
	battle_config.making_arrow_name_input = 1;
	battle_config.holywater_name_input = 1;
	battle_config.atcommand_item_creation_name_input = 1; // Add name to all items, except item with slot (to give possibility to add cards)
	battle_config.display_delay_skill_fail = 1;
	battle_config.display_snatcher_skill_fail = 1;
	battle_config.chat_warpportal = 0;
	battle_config.mob_warpportal = 0;
	battle_config.dead_branch_active = 1;
	battle_config.vending_max_value = 100000000;
	battle_config.show_steal_in_same_party = 0;
	battle_config.enable_upper_class = 1;
	battle_config.pet_attack_attr_none = 0;
	battle_config.pc_attack_attr_none = 0;
	battle_config.mob_attack_attr_none = 0;
	battle_config.mob_ghostring_fix = 1;
	battle_config.gx_allhit = 0;
	battle_config.gx_cardfix = 0;
	battle_config.gx_dupele = 1;
	battle_config.gx_disptype = 1;
	battle_config.devotion_level_difference = 10;
	battle_config.player_skill_partner_check = 1;
	battle_config.hide_GM_session = 99; // (99 = administrators) minimum level of hidden GMs
	battle_config.unit_movement_type = 0;
	battle_config.invite_request_check = 0; // Are other requests accepted during a request or not?
	battle_config.skill_removetrap_type = 0;
	battle_config.disp_experience = 0;
	battle_config.disp_experience_type = 0; // without %
	battle_config.alchemist_summon_reward = 0; // no
	battle_config.castle_defense_rate = 100;
	battle_config.riding_weight = 0;
	battle_config.hp_rate = 100;
	battle_config.sp_rate = 100;
	battle_config.gm_can_drop_lv = 0;
	battle_config.disp_hpmeter = 60; // min level to look hp of players
	battle_config.bone_drop = 0;
	battle_config.item_rate_common = 100;
	battle_config.item_rate_equip = 100;
	battle_config.item_rate_card = 100;
	battle_config.item_rate_heal = 100;
	battle_config.item_rate_use = 100;
	battle_config.item_drop_common_min = 1;	// Added by TyrNemesis^
	battle_config.item_drop_common_max = 10000;
	battle_config.item_drop_equip_min = 1;
	battle_config.item_drop_equip_max = 10000;
	battle_config.item_drop_card_min = 1;
	battle_config.item_drop_card_max = 10000;
	battle_config.item_drop_mvp_min = 1;
	battle_config.item_drop_mvp_max = 10000;	// End Addition
	battle_config.item_drop_heal_min = 1;		// Added by Valaris
	battle_config.item_drop_heal_max = 10000;
	battle_config.item_drop_use_min = 1;
	battle_config.item_drop_use_max = 10000;	// End
	battle_config.prevent_logout = 1;	// Added by RoVeRT
	battle_config.maximum_level = 255;	// Added by Valaris
	battle_config.atcommand_max_job_level_novice = 10;
	battle_config.atcommand_max_job_level_job1 = 50;
	battle_config.atcommand_max_job_level_job2 = 50;
	battle_config.atcommand_max_job_level_supernovice = 99;
	battle_config.atcommand_max_job_level_highnovice = 10;
	battle_config.atcommand_max_job_level_highjob1 = 50;
	battle_config.atcommand_max_job_level_highjob2 = 70;
	battle_config.atcommand_max_job_level_babynovice = 10;
	battle_config.atcommand_max_job_level_babyjob1 = 50;
	battle_config.atcommand_max_job_level_babyjob2 = 50;
	battle_config.atcommand_max_job_level_superbaby = 99;
	battle_config.drops_by_luk = 0;	// [Valaris]
	battle_config.monsters_ignore_gm = 60;
	battle_config.equipment_breaking = 0; // [Valaris]
	battle_config.equipment_break_rate = 100; // [Valaris]
	battle_config.pk_mode = 0; // [Valaris]
	battle_config.pet_equip_required = 1; // [Valaris]
	battle_config.multi_level_up = 0; // [Valaris]
	battle_config.backstab_bow_penalty = 1; // Akaru
	battle_config.night_at_start = 0; // added by [Yor]
	battle_config.day_duration = 2*60*60*1000; // added by [Yor] (2 hours)
	battle_config.night_duration = 20*60*1000; // added by [Yor] (20 minutes)
	battle_config.show_mob_hp = 0; // [Valaris]
	battle_config.ban_spoof_namer = 5; // added by [Yor] (default: 5 minutes)
	battle_config.ban_hack_trade = -1; // added by [Yor] (default: negativ, permanent ban)
	battle_config.ban_bot = -1; // added by [Yor] (default: negativ, permanent ban)
	battle_config.check_ban_bot = 80; // added by [Yor] (default: 80, experimental code)
	battle_config.max_message_length = 100; // added by [Yor] (default: max message length sended by a player: 100 char, except global message)
	battle_config.max_global_message_length = 150; // added by [Yor] (default: max global message length sended by a player: 150 char)
	battle_config.hack_info_GM_level = 20; // added by [Yor] (default: 20, GM team member)
	battle_config.speed_hack_info_GM_level = 99; // added by [Yor] (default: 99, experimental code)
	battle_config.any_warp_GM_min_level = 20; // added by [Yor]
	battle_config.packet_ver_flag = 6655; // added by [Yor]
	battle_config.muting_players = 0;
	battle_config.min_hair_style = 0;
	battle_config.max_hair_style = 24;
	battle_config.min_hair_color = 0;
	battle_config.max_hair_color = 8;
	battle_config.min_cloth_color = 0;
	battle_config.max_cloth_color = 4;
	battle_config.clothes_color_for_assassin = 0; // no
	battle_config.zeny_from_mobs = 0;
	battle_config.mobs_level_up = 0;
	battle_config.pk_min_level = 55;
	battle_config.skill_steal_type = 1;
	battle_config.skill_steal_rate = 100;
//	battle_config.night_darkness_level = 0; // ************** This option is not used! : we don't know how to remove effect (night -> day)
	battle_config.night_darkness_level = 0;
	battle_config.skill_range_leniency = 1;
	battle_config.motd_type = 1;
	battle_config.allow_atcommand_when_mute = 0;
	battle_config.manner_action = 3; // [Yor]
	battle_config.finding_ore_rate = 100;
	battle_config.min_skill_delay_limit = 150;
	battle_config.idle_no_share = 0; // [Yor]
	battle_config.chat_no_share = 0; // [Yor]
	battle_config.npc_chat_no_share = 1; // [Yor]
	battle_config.shop_no_share = 1; // [Yor]
	battle_config.trade_no_share = 1; // [Yor]
	battle_config.idle_disconnect = 300; // (5 minutes) [Yor]
	battle_config.idle_disconnect_chat = 1800; // (30 minutes) [Yor]
	battle_config.idle_disconnect_vender = 0; // (no limit) [Yor]
	battle_config.idle_disconnect_disable_for_restore = 1; // 1 = yes [Yor]
	battle_config.idle_disconnect_ignore_GM = 99; // (admin) [Yor]
	battle_config.jail_message = 1; // 1 = yes [Yor] Do we send message to ALL players when a player is put in jail?
	battle_config.jail_discharge_message = 3; // (in all cases)[Yor] Do we send message to ALL players when a player is discharged?
	battle_config.mingmlvl_message = 2; // (You're not authorized...) [Yor] Which message do we send when a GM can use a command, but mingmlvl map flag block it?
	battle_config.check_invalid_slot = 0; // 0 : no [Yor] Do we check invalid slotted cards?
	battle_config.ruwach_range = 2; // (2 squares in all directions -> a square of 5x5) [Yor] Set the range (number of squares/tiles around you) of 'ruwach' skill to detect invisible.
	battle_config.sight_range = 3; // (3 squares in all directions -> a square of 7x7) [Yor] Set the range (number of squares/tiles around you) of 'sight' skill to detect invisible.
	battle_config.max_icewall = 5 ; // [Yor] Set maximum number of ice walls active at the same time.

	battle_config.castrate_dex_scale = 150;
	battle_config.area_size = 16;
	battle_config.atcommand_main_channel_at_start = 3;
	battle_config.atcommand_min_GM_level_for_request = 20;
	battle_config.atcommand_follow_stop_dead_target = 0; // no
	battle_config.atcommand_add_local_message_info = 1; // yes
	battle_config.atcommand_storage_on_pvp_map = 1; // yes
	battle_config.atcommand_gstorage_on_pvp_map = 1; // yes

	battle_config.pm_gm_not_ignored = 60; // GM minimum level to be not ignored in private message. [BeoWulf] (from freya's bug report)

	battle_config.char_disconnect_mode = 2; // nobody is disconnected

	battle_config.autospell_consume_sp = 1; // SP is consumed each time skill used

//SQL-only options start
#ifdef USE_SQL
	battle_config.mail_system = 0;
//SQL-only options end
#endif /* USE_SQL */
}

void battle_validate_conf() {
	if (battle_config.flooritem_lifetime < 1000)
		battle_config.flooritem_lifetime = LIFETIME_FLOORITEM * 1000;

	if (battle_config.item_auto_get < 0) // drop rate of items concerned by the autoloot (from 0% to x%).
		battle_config.item_auto_get = 0;
	else if (battle_config.item_auto_get > 10000)
		battle_config.item_auto_get = 10000;
	if (battle_config.item_auto_get_distance < 0)
		battle_config.item_auto_get_distance = 0;

	if (battle_config.base_exp_rate < 1)
		battle_config.base_exp_rate = 1;
	else if (battle_config.base_exp_rate > 1000000000)
		battle_config.base_exp_rate = 1000000000;
	if (battle_config.job_exp_rate < 1)
		battle_config.job_exp_rate = 1;
	else if (battle_config.job_exp_rate > 1000000000)
		battle_config.job_exp_rate = 1000000000;

	if (battle_config.zeny_penalty_percent < 0)
		battle_config.zeny_penalty_percent = 0;

	if (battle_config.restart_hp_rate < 0)
		battle_config.restart_hp_rate = 0;
	else if (battle_config.restart_hp_rate > 100)
		battle_config.restart_hp_rate = 100;
	if (battle_config.restart_sp_rate < 0)
		battle_config.restart_sp_rate = 0;
	else if (battle_config.restart_sp_rate > 100)
		battle_config.restart_sp_rate = 100;
	if (battle_config.natural_healhp_interval < NATURAL_HEAL_INTERVAL)
		battle_config.natural_healhp_interval = NATURAL_HEAL_INTERVAL;
	if (battle_config.natural_healsp_interval < NATURAL_HEAL_INTERVAL)
		battle_config.natural_healsp_interval = NATURAL_HEAL_INTERVAL;
	if (battle_config.natural_heal_skill_interval < NATURAL_HEAL_INTERVAL)
		battle_config.natural_heal_skill_interval = NATURAL_HEAL_INTERVAL;
	if (battle_config.natural_heal_weight_rate < 50)
		battle_config.natural_heal_weight_rate = 50;
	if (battle_config.natural_heal_weight_rate > 101)
		battle_config.natural_heal_weight_rate = 101;
	battle_config.monster_max_aspd = 2000 - battle_config.monster_max_aspd * 10;
	if (battle_config.monster_max_aspd < 10)
		battle_config.monster_max_aspd = 10;
	if (battle_config.monster_max_aspd > 1000)
		battle_config.monster_max_aspd = 1000;
	if (battle_config.atc_spawn_quantity_limit < 0)
		battle_config.atc_spawn_quantity_limit = 0;
	if (battle_config.atc_map_mob_limit < 0)
		battle_config.atc_map_mob_limit = 0;
	battle_config.max_aspd = 2000 - battle_config.max_aspd * 10;
	if (battle_config.max_aspd < 10)
		battle_config.max_aspd = 10;
	if (battle_config.max_aspd > 1000)
		battle_config.max_aspd = 1000;
	if(battle_config.hp_rate < 0)
		battle_config.hp_rate = 1;
	if(battle_config.sp_rate < 0)
		battle_config.sp_rate = 1;
	if (battle_config.max_hp > 1000000)
		battle_config.max_hp = 1000000;
	if (battle_config.max_hp < 100)
		battle_config.max_hp = 100;
	if (battle_config.max_sp > 1000000)
		battle_config.max_sp = 1000000;
	if (battle_config.max_sp < 100)
		battle_config.max_sp = 100;
	if (battle_config.max_parameter < 10)
		battle_config.max_parameter = 10;
	if (battle_config.max_parameter > 10000)
		battle_config.max_parameter = 10000;
	if (battle_config.max_cart_weight > 1000000)
		battle_config.max_cart_weight = 1000000;
	if (battle_config.max_cart_weight < 100)
		battle_config.max_cart_weight = 100;
	battle_config.max_cart_weight *= 10;

	if (battle_config.agi_penalty_count < 2)
		battle_config.agi_penalty_count = 2;
	if (battle_config.vit_penalty_count < 2)
		battle_config.vit_penalty_count = 2;

	if (battle_config.atc_local_spawn_interval < 0)
		battle_config.atc_local_spawn_interval = 0;

	if (battle_config.guild_exp_limit > 99)
		battle_config.guild_exp_limit = 99;
	if (battle_config.guild_exp_limit < 0)
		battle_config.guild_exp_limit = 0;
//	if (battle_config.pet_weight < 0)
//		battle_config.pet_weight = 0;

	if (battle_config.hide_GM_session < 0)
		battle_config.hide_GM_session = 0;
	else if (battle_config.hide_GM_session > 100)
		battle_config.hide_GM_session = 100;

	if (battle_config.castle_defense_rate < 0)
		battle_config.castle_defense_rate = 0;
	else if (battle_config.castle_defense_rate > 100)
		battle_config.castle_defense_rate = 100;

	if (battle_config.item_rate_common < 1)
		battle_config.item_rate_common = 1;
	else if (battle_config.item_rate_common > 1000000)
		battle_config.item_rate_common = 1000000; // 1 (0,01%) * 1000000 / 100 = 10000 (100%)
	if (battle_config.item_rate_equip < 1)
		battle_config.item_rate_equip = 1;
	else if (battle_config.item_rate_equip > 1000000)
		battle_config.item_rate_equip = 1000000; // 1 (0,01%) * 1000000 / 100 = 10000 (100%)
	if (battle_config.item_rate_card < 1)
		battle_config.item_rate_card = 1;
	else if (battle_config.item_rate_card > 1000000)
		battle_config.item_rate_card = 1000000; // 1 (0,01%) * 1000000 / 100 = 10000 (100%)
	if (battle_config.item_rate_heal < 1)
		battle_config.item_rate_heal = 1;
	else if (battle_config.item_rate_heal > 1000000)
		battle_config.item_rate_heal = 1000000; // 1 (0,01%) * 1000000 / 100 = 10000 (100%)
	if (battle_config.item_rate_use < 1)
		battle_config.item_rate_use = 1;
	else if (battle_config.item_rate_use > 1000000)
		battle_config.item_rate_use = 1000000; // 1 (0,01%) * 1000000 / 100 = 10000 (100%)
	if (battle_config.item_drop_common_min < 1) // Added by TyrNemesis^
		battle_config.item_drop_common_min = 1;
	if (battle_config.item_drop_common_max > 10000)
		battle_config.item_drop_common_max = 10000;
	if (battle_config.item_drop_use_min < 1)
		battle_config.item_drop_use_min = 1;
	if (battle_config.item_drop_use_max > 10000)
		battle_config.item_drop_use_max = 10000;
	if (battle_config.item_drop_equip_min < 1)
		battle_config.item_drop_equip_min = 1;
	if (battle_config.item_drop_equip_max > 10000)
		battle_config.item_drop_equip_max = 10000;
	if (battle_config.item_drop_card_min < 1)
		battle_config.item_drop_card_min = 1;
	if (battle_config.item_drop_card_max > 10000)
		battle_config.item_drop_card_max = 10000;
	if (battle_config.item_drop_mvp_min < 1)
		battle_config.item_drop_mvp_min = 1;
	if (battle_config.item_drop_mvp_max > 10000)
		battle_config.item_drop_mvp_max = 10000; // End Addition

	if (battle_config.maximum_level < 1)
		battle_config.maximum_level = 255;
	if (battle_config.atcommand_max_job_level_novice < 1)
		battle_config.atcommand_max_job_level_novice = 10;
	if (battle_config.atcommand_max_job_level_job1 < 1)
		battle_config.atcommand_max_job_level_job1 = 50;
	if (battle_config.atcommand_max_job_level_job2 < 1)
		battle_config.atcommand_max_job_level_job2 = 50;
	if (battle_config.atcommand_max_job_level_supernovice < 1)
		battle_config.atcommand_max_job_level_supernovice = 99;
	if (battle_config.atcommand_max_job_level_highnovice < 1)
		battle_config.atcommand_max_job_level_highnovice = 10;
	if (battle_config.atcommand_max_job_level_highjob1 < 1)
		battle_config.atcommand_max_job_level_highjob1 = 50;
	if (battle_config.atcommand_max_job_level_highjob2 < 1)
		battle_config.atcommand_max_job_level_highjob2 = 70;
	if (battle_config.atcommand_max_job_level_babynovice < 1)
		battle_config.atcommand_max_job_level_babynovice = 10;
	if (battle_config.atcommand_max_job_level_babyjob1 < 1)
		battle_config.atcommand_max_job_level_babyjob1 = 50;
	if (battle_config.atcommand_max_job_level_babyjob2 < 1)
		battle_config.atcommand_max_job_level_babyjob2 = 50;
	if (battle_config.atcommand_max_job_level_superbaby < 1)
		battle_config.atcommand_max_job_level_superbaby = 99;
	if (battle_config.pk_min_level < 1)
		battle_config.pk_min_level = 55;

	if (battle_config.night_at_start < 0) // added by [Yor]
		battle_config.night_at_start = 0;
	else if (battle_config.night_at_start > 1) // added by [Yor]
		battle_config.night_at_start = 1;
	if (battle_config.day_duration < 0) // added by [Yor]
		battle_config.day_duration = 0;
	if (battle_config.night_duration < 0) // added by [Yor]
		battle_config.night_duration = 0;

	if (battle_config.check_ban_bot < 0) // added by [Yor]
		battle_config.check_ban_bot = 0;
	else if (battle_config.check_ban_bot > 100)
		battle_config.check_ban_bot = 100;

	if (battle_config.max_message_length < 80) // added by [Yor]
		battle_config.max_message_length = 80; // maximum that a normal client can send is: 70, but we give security -> 80
	if (battle_config.max_global_message_length < 130) // added by [Yor]
		battle_config.max_global_message_length = 130; // maximum that a normal iRO client can send is: 130

	if (battle_config.hack_info_GM_level < 0) // added by [Yor]
		battle_config.hack_info_GM_level = 0;
	else if (battle_config.hack_info_GM_level > 100)
		battle_config.hack_info_GM_level = 100;

	if (battle_config.speed_hack_info_GM_level < 0) // added by [Yor]
		battle_config.speed_hack_info_GM_level = 0;
	else if (battle_config.speed_hack_info_GM_level > 100)
		battle_config.speed_hack_info_GM_level = 100;

	if (battle_config.any_warp_GM_min_level < 0) // added by [Yor]
		battle_config.any_warp_GM_min_level = 0;
	else if (battle_config.any_warp_GM_min_level > 100)
		battle_config.any_warp_GM_min_level = 100;

	// at least 1 client must be accepted
	if ((battle_config.packet_ver_flag & 8191) == 0) // added by [Yor] (we must avoid to set 256 and 512 together)
		battle_config.packet_ver_flag = 6655; // accept all clients (except 2004-12-06aSakexe client (-512) and 2005-01-10bSakexe client (-1024), similar to 2004-10-25aSakexe client and 2005-06-28aSakexe client)
	else {
		if ((battle_config.packet_ver_flag & (256 + 512)) == (256 + 512)) {
			printf("WARNING: Avoid to accept both clients 2004-12-06aSakexe AND 2004-10-25aSakexe.\n");
			printf("         It's possible that some players will not be able to connect\n");
			printf("         if you don't choose between the 2 versions and accept the 2 versions.\n");
		}
		if ((battle_config.packet_ver_flag & (1024 + 4096)) == (1024 + 4096)) {
			printf("WARNING: Avoid to accept both clients 2005-01-10bSakexe AND 2005-06-28aSakexe.\n");
			printf("         It's possible that some players will not be able to connect\n");
			printf("         if you don't choose between the 2 versions and accept the 2 versions.\n");
		}
	}

	if (battle_config.night_darkness_level > 10) // Celest
		battle_config.night_darkness_level = 10;

	if (battle_config.finding_ore_rate < 0)
		battle_config.finding_ore_rate = 0;
	else if (battle_config.finding_ore_rate > 10000)
		battle_config.finding_ore_rate = 10000;

	if (battle_config.skill_range_leniency < 0) // Celest
		battle_config.skill_range_leniency = 0;

	if (battle_config.motd_type < 0)
		battle_config.motd_type = 0;
	else if (battle_config.motd_type > 1)
		battle_config.motd_type = 1;

	if (battle_config.manner_action < 0) // added by [Yor]
		battle_config.manner_action = 0;
	else if (battle_config.manner_action > 3)
		battle_config.manner_action = 3;

	if (battle_config.atcommand_item_creation_name_input < 0)
		battle_config.atcommand_item_creation_name_input = 0;

	if (battle_config.vending_max_value > 2000000000 || battle_config.vending_max_value <= 0)
		battle_config.vending_max_value = 2000000000;

	if (battle_config.min_skill_delay_limit < 0)
		battle_config.min_skill_delay_limit = 0; // minimum delay of 10ms (timers have minimum of 50 ms)

	if (battle_config.idle_no_share < 0)
		battle_config.idle_no_share = 0;
	else if (battle_config.idle_no_share > 2)
		battle_config.idle_no_share = 2;
	if (battle_config.chat_no_share < 0)
		battle_config.chat_no_share = 0;
	else if (battle_config.chat_no_share > 2)
		battle_config.chat_no_share = 2;
	if (battle_config.npc_chat_no_share < 0)
		battle_config.npc_chat_no_share = 0;
	else if (battle_config.npc_chat_no_share > 2)
		battle_config.npc_chat_no_share = 2;
	if (battle_config.shop_no_share < 0)
		battle_config.shop_no_share = 0;
	else if (battle_config.shop_no_share > 2)
		battle_config.shop_no_share = 2;
	if (battle_config.trade_no_share < 0)
		battle_config.trade_no_share = 0;
	else if (battle_config.trade_no_share > 2)
		battle_config.trade_no_share = 2;

	if (battle_config.area_size < 5) // do you want to look something? (in some function, we do: battle_config.area_size - 5)
		battle_config.area_size = 5;
	else if (battle_config.area_size > 50) // more is not necessary, because client can not display more than 20-25 !
		battle_config.area_size = 50;

	if (battle_config.idle_disconnect < 60)
		battle_config.idle_disconnect = 0;
	if (battle_config.idle_disconnect_chat < 60)
		battle_config.idle_disconnect_chat = 0;
	if (battle_config.idle_disconnect_vender < 60)
		battle_config.idle_disconnect_vender = 0;
	if (battle_config.idle_disconnect_ignore_GM < 0)
		battle_config.idle_disconnect_ignore_GM = 0;
	else if (battle_config.idle_disconnect_ignore_GM > 100)
		battle_config.idle_disconnect_ignore_GM = 100;
	//printf("idle disconnection: normal %d, chat %d, vender %d, disable for restore: %s, ignore GM :d\n", battle_config.idle_disconnect, battle_config.idle_disconnect_chat, battle_config.idle_disconnect_vender, (battle_config.idle_disconnect_disable_for_restore) ? "yes" : "no", battle_config.idle_disconnect_ignore_GM);

	if (battle_config.jail_discharge_message < 0)
		battle_config.jail_discharge_message = 0;
	else if (battle_config.jail_discharge_message > 3)
		battle_config.jail_discharge_message = 3;

	if (battle_config.mingmlvl_message < 0)
		battle_config.mingmlvl_message = 0;
	else if (battle_config.mingmlvl_message > 2)
		battle_config.mingmlvl_message = 2;

	if (battle_config.ruwach_range < 1)
		battle_config.ruwach_range = 2; // set default value
	else if (battle_config.ruwach_range > 100) // we can not see more than 100 squares
		battle_config.ruwach_range = 100;
	if (battle_config.sight_range < 1)
		battle_config.sight_range = 3; // set default value
	else if (battle_config.sight_range > 100) // we can not see more than 100 squares
		battle_config.sight_range = 100;

	if (battle_config.max_icewall < 0) // [Yor] Set maximum number of ice walls active at the same time.
		battle_config.max_icewall = 0; // no limit

	if (battle_config.atcommand_main_channel_at_start < 0)
		battle_config.atcommand_main_channel_at_start = 0;
	else if (battle_config.atcommand_main_channel_at_start > 3)
		battle_config.atcommand_main_channel_at_start = 3;
	if (battle_config.atcommand_min_GM_level_for_request < 0)
		battle_config.atcommand_min_GM_level_for_request = 0;
	else if (battle_config.atcommand_min_GM_level_for_request > 100)
		battle_config.atcommand_min_GM_level_for_request = 100;

	if (battle_config.atcommand_follow_stop_dead_target != 0)
		battle_config.atcommand_follow_stop_dead_target = 1;

	if (battle_config.pm_gm_not_ignored < 0 || battle_config.pm_gm_not_ignored > 100) // GM minimum level to be not ignored in private message. [BeoWulf] (from freya's bug report)
		battle_config.pm_gm_not_ignored = 100;

	if (battle_config.char_disconnect_mode < 0)
		battle_config.char_disconnect_mode = 0;
	else if (battle_config.char_disconnect_mode > 2)
		battle_config.char_disconnect_mode = 2;

	if (battle_config.autospell_consume_sp < 0 || battle_config.autospell_consume_sp > 1)
		battle_config.autospell_consume_sp = 0;

	return;
}

/*==========================================
 * 設定ファイルを読み込む
 *------------------------------------------
 */
int battle_config_read(const char *cfgName) {
	char line[1024], w1[1024], w2[1024];
	FILE *fp;
	static int count = 0;

	if ((count++) == 0)
		battle_set_defaults();

	if ((fp = fopen(cfgName, "r")) == NULL) {
//		if ((fp = fopen("conf/battle_athena.conf", "r")) == NULL) { // not try default, possible infinite loop with import
			printf("File not found: %s\n", cfgName);
			return 1;
//		}
	}

	while(fgets(line, sizeof(line), fp)) { // fgets reads until maximum one less than size and add '\0' -> so, it's not necessary to add -1
		if ((line[0] == '/' && line[1] == '/') || line[0] == '\0' || line[0] == '\n' || line[0] == '\r')
			continue;
		// it's not necessary to remove 'carriage return ('\n' or '\r')

		memset(w2, 0, sizeof(w2));
		if (sscanf(line, "%[^:]:%s", w1, w2) != 2)
			continue;
// import
		if (strcasecmp(w1, "import") == 0) {
			printf("battle_config_read: Import file: %s.\n", w2);
			battle_config_read(w2);
		} else {
			if (!battle_set_value(w1, w2)) {
#ifndef USE_SQL
				if (strcasecmp(w1, "mail_system") != 0) // don't display error message if in TXT version and read mail_system configuration
#endif /* USE_SQL */
					printf("Unknown configuration value: '%s'.\n", w1);
			}
		}
	}
	fclose(fp);

	if (--count == 0) {
		battle_validate_conf();
		add_timer_func_list(battle_delay_damage_sub, "battle_delay_damage_sub");
	}

	printf("File '" CL_WHITE "%s" CL_RESET "' readed.\n", cfgName);

	return 0;
}

