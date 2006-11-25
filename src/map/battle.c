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
#include "party.h"
#include "status.h"

#ifdef MEMWATCH
#include "memwatch.h"
#endif

int attr_fix_table[4][10][10];

struct Battle_Config battle_config;

/*==========================================
 * ìÒì_ä‘ÇÃãóó£Çï‘Ç∑
 * ñﬂÇËÇÕêÆêîÇ≈0à»è„
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
 * é©ï™ÇÉçÉbÉNÇµÇƒÇ¢ÇÈMOBÇÃ?Ç?Ç¶ÇÈ(foreachclient)
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

int battle_getcurrentskill(struct block_list *bl) {	//Returns the current/last skill in use by this bl.

	if(bl->type == BL_PC)
		return ((struct map_session_data*)bl)->skillid;
	else if(bl->type == BL_MOB)
		return ((struct mob_data*)bl)->skillid;
	else if(bl->type == BL_SKILL) {
		struct skill_unit * su = (struct skill_unit*)bl;
		if (su->group)
			return su->group->skill_id;
	}

	return 0;
}

/*==========================================
 * é©ï™ÇÉçÉbÉNÇµÇƒÇ¢ÇÈëŒè€ÇÃêîÇï‘Ç∑(îƒóp)
 * ñﬂÇËÇÕêÆêîÇ≈0à»è„
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

// É_ÉÅÅ[ÉWÇÃíxâÑ
struct battle_delay_damage_ {
	struct block_list *src;
	int target;
	int damage;
	unsigned short distance;
	unsigned short skill_lv;
	unsigned short skill_id;
	unsigned short dmg_lv;
	unsigned short flag;
	unsigned char attack_type;
};

int battle_delay_damage_sub(int tid, unsigned int tick, int id, int data) {
	struct battle_delay_damage_ *dat = (struct battle_delay_damage_ *)data;
	struct block_list *target = map_id2bl(dat->target);

	if(dat && map_id2bl(id) == dat->src && target && target->prev != NULL && !status_isdead(target) &&
	   target->m == dat->src->m && distance(dat->src->x, dat->src->y, target->x, target->y) <= dat->distance) {

		map_freeblock_lock();

		battle_damage(dat->src, target, dat->damage, dat->flag);
		if (!status_isdead(target) && (dat->dmg_lv == ATK_DEF || dat->damage > 0) && dat->attack_type)
			skill_additional_effect(dat->src, target, dat->skill_id, dat->skill_lv, dat->attack_type, tick);

		map_freeblock_unlock(); //now it can be released without any problems, since no other function needs to access the block
	}
	FREE(dat);

	return 0;
}

int battle_delay_damage(unsigned int tick, struct block_list *src, struct block_list *target, int attack_type, int skill_id, int skill_lv, int damage, int dmg_lv, int flag)
{
	struct battle_delay_damage_ *dat;

	nullpo_retr(0, src);
	nullpo_retr(0, target);

	/*if (!battle_config.delay_battle_damage) {
		battle_damage(src, target, damage, flag);
		if (!status_isdead(target) && damage > 0 && attack_type)
			skill_additional_effect(src, target, skill_id, skill_lv, attack_type, gettick_cache);
		return 0;
	}*/

	CALLOC(dat, struct battle_delay_damage_, 1);

	dat->src = src;
	dat->target = target->id;
	dat->skill_id = skill_id;
	dat->skill_lv = skill_lv;
	dat->attack_type = attack_type;
	dat->damage = damage;
	dat->dmg_lv = dmg_lv;
	dat->flag = flag;
	dat->distance = distance(src->x, src->y, target->x, target->y) + 10;
	add_timer(tick, battle_delay_damage_sub, src->id, (int)dat);

	return 0;
}

// é¿ç€Ç…HPÇëÄçÏ
int battle_damage(struct block_list *bl,struct block_list *target,int damage,int flag)
{
	struct map_session_data *sd = NULL;
	struct status_change *sc_data;
	short *sc_count;
	int i;

	nullpo_retr(0, target); //blÇÕNULLÇ≈åƒÇŒÇÍÇÈÇ±Ç∆Ç™Ç†ÇÈÇÃÇ≈ëºÇ≈É`ÉFÉbÉN
	
	if(damage == 0 || target->type == BL_PET)
		return 0;

	if(target->prev == NULL)
		return 0;

	if(bl) {
		if(bl->prev == NULL)
			return 0;

		if(bl->type == BL_PC)
			sd = (struct map_session_data *)bl;
	}

	if(damage < 0)
		return battle_heal(bl, target, -damage, 0, flag);

	if (!flag && (sc_count = status_get_sc_count(target)) != NULL && *sc_count > 0) {
		sc_data = status_get_sc_data(target);
		// ìÄåãÅAêŒâªÅAêáñ∞Çè¡ãé
		if(sc_data[SC_FREEZE].timer != -1)
			status_change_end(target, SC_FREEZE,-1);
		if(sc_data[SC_STONE].timer != -1 && sc_data[SC_STONE].val2 == 0)
			status_change_end(target, SC_STONE,-1);
		if(sc_data[SC_SLEEP].timer != -1)
			status_change_end(target, SC_SLEEP, -1);
		if (sc_data[SC_CONFUSION].timer != -1)
			status_change_end(target, SC_CONFUSION, -1);
	}

	if (target->type == BL_MOB) { // MOB
		struct mob_data *md = (struct mob_data *)target;
		if (md && md->skilltimer != -1 && md->state.skillcastcancel) // ârè•ñWäQ
			skill_castcancel(target, 0);
		return mob_damage(bl, md, damage, 0);
	}
	else if (target->type == BL_PC) { // PC
		struct map_session_data *tsd = (struct map_session_data *)target;

		if (tsd) {
			if (tsd->sc_data && tsd->sc_data[SC_DEVOTION].val1) { // ÉfÉBÉ{Å[ÉVÉáÉìÇÇ©ÇØÇÁÇÍÇƒÇ¢ÇÈ
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

			if (tsd->skilltimer != -1) { // ârè•ñWäQ
				// ÉtÉFÉìÉJÅ[ÉhÇ‚ñWäQÇ≥ÇÍÇ»Ç¢ÉXÉLÉãÇ©ÇÃåüç∏
				if ((!tsd->special_state.no_castcancel || map[target->m].flag.gvg) && tsd->state.skillcastcancel &&
				    !tsd->special_state.no_castcancel2)
					skill_castcancel(target, 0);
			}
		}
		return pc_damage(bl, tsd, damage);

	}
	else if (target->type == BL_SKILL)
		return skill_unit_ondamaged((struct skill_unit *)target, bl, damage, gettick_cache);

	return 0;
}

int battle_heal(struct block_list *bl, struct block_list *target, int hp, int sp, int flag)
{
	nullpo_retr(0, target); //blÇÕNULLÇ≈åƒÇŒÇÍÇÈÇ±Ç∆Ç™Ç†ÇÈÇÃÇ≈ëºÇ≈É`ÉFÉbÉN

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

/* stop attacking and casting */
void battle_stopattack(struct block_list *bl)
{
	nullpo_retv(bl);

	if(bl->type == BL_MOB)
		mob_stopattack((struct mob_data*)bl);
	else if(bl->type == BL_PC)
		pc_stopattack((struct map_session_data*)bl);
	else if(bl->type == BL_PET)
		pet_stopattack((struct pet_data*)bl);

	return;
}

// à⁄ìÆí‚é~
void battle_stopwalking(struct block_list *bl,int type)
{
	nullpo_retv(bl);

	if(bl->type==BL_MOB)
		return mob_stop_walking((struct mob_data*)bl,type);
	else if(bl->type==BL_PC)
		return pc_stop_walking((struct map_session_data*)bl, type);
	else if(bl->type==BL_PET)
		return pet_stop_walking((struct pet_data*)bl,type);

	return;
}

/* battle_attr_fix: apply damage attribute fix
   ------------------------------------------- */
int battle_attr_fix(int damage, int atk_elem, int def_elem)
{
	int def_type = def_elem % 10, def_lv = def_elem / 10 / 2;
	
	if(atk_elem < 0 || atk_elem > 9)
	{
		printf("battle_attr_fix: invalid attack element '%i'. ignoring attribute fix \n", atk_elem);
		return damage;
	}
	
	if(def_type < 0 || def_type > 9)
	{
		printf("battle_attr_fix: invalid defence element '%i'. ignoring attribute fix \n", def_elem);
		return damage;
	}
	
	if(def_lv < 1 || def_lv > 4)
	{
		printf("battle_attr_fix: invalid defence level '%i'. ignoring attribute fix \n", def_lv);
		return damage;
	}

	return damage * attr_fix_table[def_lv - 1][atk_elem][def_type] / 100;
}

/*==========================================
 * É_É??[ÉW?≈?IåvéZ
 *------------------------------------------
 */
int battle_calc_damage(struct block_list *src, struct block_list *bl, int damage, int div_, int skill_num, int skill_lv, int flag) {
	struct map_session_data *tsd = NULL;
	struct mob_data *tmd = NULL;
	struct status_change *sc_data, *tsc_data;
//	struct block_list *target; // Target Data Structure - Part of WiP Ghost Fix [Tsuyuki]
	short *sc_count;

//	short t_element = status_get_element(target); // Target Element - Part of WiP Ghost Fix [Tsuyuki]
//	int s_ele = status_get_attack_element(src);   // Right-Hand Weapon Element - Part of WiP Ghost Fix [Tsuyuki]

	nullpo_retr(0, src);
	nullpo_retr(0, bl);

	if (damage <= 0)
		return 0; // No need to continue calculating.. return 0 damage

	if (bl->type == BL_PC)
		tsd = (struct map_session_data *)bl;
	else if (bl->type == BL_MOB)
		tmd = (struct mob_data *)bl;

	sc_data = status_get_sc_data(src);

	/* SC_MAGNUM adds 20% firedamage bonus */
	if(sc_data && sc_data[SC_MAGNUM].timer != -1 && !(flag & BF_MAGIC))
		damage = damage + (battle_attr_fix(damage, 3, status_get_element(bl)) * 20 / 100);

	sc_count = status_get_sc_count(bl);

	if(sc_count != NULL && *sc_count > 0)
	{
		tsc_data = status_get_sc_data(bl);

		if(tsc_data[SC_SAFETYWALL].timer != -1)
		{
			if((flag&BF_SHORT && skill_num != NPC_GUIDEDATTACK && skill_num != AM_DEMONSTRATION) || (skill_num == AS_GRIMTOOTH && skill_lv <= 2))
			{
				struct skill_unit *unit = (struct skill_unit *)tsc_data[SC_SAFETYWALL].val2;

				if(unit)
				{
					if(unit->group && (--unit->group->val2) <= 0)
						skill_delunit(unit);
					return 0;
				} else {
					status_change_end(bl, SC_SAFETYWALL, -1);
				}
			}
		}

		if(tsc_data[SC_PNEUMA].timer != -1)
		{
			if(skill_num == AS_GRIMTOOTH)
			{
				if(skill_lv >= 3)
					return 0;
			} else if(flag&BF_LONG && ((flag&BF_WEAPON && skill_num != NPC_GUIDEDATTACK) || (flag&BF_MAGIC && skill_num == ASC_BREAKER) || flag&BF_MISC )) {
				return 0;
			}
		}

		if(tsc_data[SC_BASILICA].timer != -1)
			return 0;

		if(flag & BF_MAGIC)
		{
			if(tsc_data[SC_LANDPROTECTOR].timer != -1)
				return 0;
			if(tsc_data[SC_FOGWALL].timer != -1 && rand()%100 < tsc_data[SC_FOGWALL].val2)
				return 0;
		} else if(flag & BF_WEAPON) {
			// -- moonsoul (chance to block attacks with new Lord Knight skill parrying)
			if(tsc_data[SC_PARRYING].timer != -1) {
				if(rand() % 100 < tsc_data[SC_PARRYING].val2) {
					clif_skill_nodamage(bl, bl, LK_PARRYING, tsc_data[SC_PARRYING].val1, 1);
					return 0; // No need to continue calculating.. return 0 damage
				}
			}

			if (tsc_data[SC_READYDODGE].timer != -1 && tsd && !tsd->opt1 &&
				(flag&BF_LONG || tsc_data[SC_SPURT].timer != -1) &&	rand()%100 < 20) {
				if (tsd && pc_issit(tsd)) pc_setstand(tsd); //Stand it to dodge.
				clif_skill_nodamage(bl, bl, TK_DODGE, 1, 1);
				if (tsc_data[SC_COMBO].timer == -1)
					status_change_start(bl, SC_COMBO, TK_JUMPKICK, src->id, 0, 0, 2000,0);
				return 0;
			}

			if(tsc_data[SC_AUTOGUARD].timer != -1) {
				if(rand() % 100 < tsc_data[SC_AUTOGUARD].val2) {
					clif_skill_nodamage(bl, bl, CR_AUTOGUARD, tsc_data[SC_AUTOGUARD].val1, 1);
					// different delay depending on skill level [celest]
				  {
					int delay;
					if (tsc_data[SC_AUTOGUARD].val1 <= 5)
						delay = 300;
					else if (tsc_data[SC_AUTOGUARD].val1 > 5 && tsc_data[SC_AUTOGUARD].val1 <= 9)
						delay = 200;
					else
						delay = 100;
					if(tsd)
						tsd->canmove_tick = gettick_cache + delay;
					else if(tmd)
						tmd->canmove_tick = gettick_cache + delay;
				  }
					//Shrink effect
					if(tsc_data[SC_SHRINK].timer != -1 && rand()%100 < 5 * tsc_data[SC_AUTOGUARD].val1)
						skill_blown(bl, src, 2);
				return 0; // No need to continue calculating.. return 0 damage
				}
			}
		}
		
		if(tsc_data[SC_KAUPE].timer != -1 && (rand()%99) < (tsc_data[SC_KAUPE].val1 * 33) && (src->type == BL_PC || !skill_num)) {
			clif_specialeffect(bl, 462, AREA);
			status_change_end(bl, SC_KAUPE, -1);
			return 0;
		}
		
		if (tsc_data[SC_AETERNA].timer != -1) { // ÉåÉbÉNÉXÉGÅ[ÉeÉãÉi
			damage <<= 1; //double damage
			status_change_end(bl, SC_AETERNA, -1);
		}
				
		//ëÆê´èÍÇÃÉ_ÉÅÅ[ÉWëùâ¡
		if(tsc_data[SC_VOLCANO].timer != -1) { // É{ÉãÉPÅ[Ém
			if(flag&BF_SKILL) {
				if(skill_get_pl(skill_num) == 3)
					damage += damage * tsc_data[SC_VOLCANO].val4 / 100;
			} else {
				if(status_get_attack_element(bl) == 3)
					damage += damage * tsc_data[SC_VOLCANO].val4 / 100;
			}
		}

		if(tsc_data[SC_VIOLENTGALE].timer != -1) {	// ÉoÉCÉIÉåÉìÉgÉQÉCÉã
			if(flag&BF_SKILL) {
				if(skill_get_pl(skill_num) == 4)
					damage += damage * tsc_data[SC_VIOLENTGALE].val4 / 100;
			} else {
				if(status_get_attack_element(bl) == 4)
					damage += damage * tsc_data[SC_VIOLENTGALE].val4 / 100;
			}
		}

		if(tsc_data[SC_DELUGE].timer != -1) {	// ÉfÉäÉÖÅ[ÉW
			if(flag&BF_SKILL) {
				if(skill_get_pl(skill_num) == 1)
					damage += damage * tsc_data[SC_DELUGE].val4 / 100;
			} else {
				if(status_get_attack_element(bl) == 1)
					damage += damage * tsc_data[SC_DELUGE].val4 / 100;
			}
		}
		
		if (tsc_data[SC_SPIDERWEB].timer != -1 && damage > 0) {
			if ((flag&BF_SKILL && skill_get_pl(skill_num) == 3) ||
			   (!flag&BF_SKILL && status_get_attack_element(src) == 3)) {
				damage <<= 1;
				status_change_end(bl, SC_SPIDERWEB, -1);
			}
		}
		
		if(tsc_data[SC_ASSUMPTIO].timer != -1) { //ÉAÉVÉÉÉìÉvÉeÉBÉI
			if(map[bl->m].flag.pvp || map[bl->m].flag.gvg)
				damage = (damage * 2) / 3;	// 33% damage reduction on GvG or PvP map
			else
				damage >>= 1;		// Damage on PvM maps should be half of the original dmg.
		}

		if(flag&BF_WEAPON) { //Weapon attacks
			if(flag&BF_LONG) { //Long ranged attacks
				if(tsc_data[SC_DEFENDER].timer != -1)
					damage = damage * (100 - tsc_data[SC_DEFENDER].val2) / 100;
		
				if(tsc_data[SC_FOGWALL].timer != -1)
					damage >>= 1;
			}
			
			if (tsc_data[SC_ENERGYCOAT].timer != -1 && damage > 0) { // ÉGÉiÉWÅ[ÉRÅ[Ég
				if (tsd) {
					if (tsd->status.sp > 0) {
						int per = tsd->status.sp * 5 / (tsd->status.max_sp + 1);
						tsd->status.sp -= tsd->status.sp * (per * 5 + 10) / 1000;
						if (tsd->status.sp < 0)
							tsd->status.sp = 0;
						damage -= damage * ((per + 1) * 6) / 100;
						clif_updatestatus(tsd, SP_SP);
					}
					if (tsd->status.sp <= 0)
						status_change_end(bl, SC_ENERGYCOAT, -1);
				} else
					damage -= damage * (tsc_data[SC_ENERGYCOAT].val1 * 6) / 100;
			}
		}

		if(tsc_data[SC_KYRIE].timer != -1 && damage > 0)
		{
			sc_data[SC_KYRIE].val2 -= damage;

			if(flag & BF_WEAPON || skill_num == TF_THROWSTONE)
			{
				if(sc_data[SC_KYRIE].val2 >= 0)	
					damage = 0;
				else
					damage -= sc_data[SC_KYRIE].val2;
			}

			if((--sc_data[SC_KYRIE].val3) <= 0 || (sc_data[SC_KYRIE].val2 <= 0) || skill_num == AL_HOLYLIGHT)
				status_change_end(bl, SC_KYRIE, -1);
		}

		// ÉäÉWÉFÉNÉgÉ\Å[Éh
		// Reject Sword: Fixed the condition check - The weapon condition was incorrect. Wasn't working for swords - [Aalye]
		if(tsc_data[SC_REJECTSWORD].timer != -1 && damage > 0 && flag&BF_WEAPON &&
			(src->type == BL_MOB || (src->type == BL_PC && (((struct map_session_data *)src)->status.weapon == 1 ||
			((struct map_session_data *)src)->status.weapon == 2 ||
			((struct map_session_data *)src)->status.weapon == 3)))) {
			if (rand() % 100 < (15 * tsc_data[SC_REJECTSWORD].val1)) { //îΩéÀämó¶ÇÕ15*Lv
				damage = damage * 50 / 100;
				clif_damage(bl, src, gettick_cache, 0, 0, damage, 0, 0, 0); // So to display the damage add this [Celest] from Freya' forum
				battle_damage(bl, src, damage, 0);
				//É_ÉÅÅ[ÉWÇó^Ç¶ÇΩÇÃÇÕó«Ç¢ÇÒÇæÇ™ÅAÇ±Ç±Ç©ÇÁÇ«Ç§ÇµÇƒï\é¶Ç∑ÇÈÇÒÇæÇ©ÇÌÇ©ÇÒÇÀÇ•
				//ÉGÉtÉFÉNÉgÇ‡Ç±ÇÍÇ≈Ç¢Ç¢ÇÃÇ©ÇÌÇ©ÇÒÇÀÇ•
				clif_skill_nodamage(bl, bl, ST_REJECTSWORD, tsc_data[SC_REJECTSWORD].val1, 1);
				//tsc_data[SC_REJECTSWORD].val3 = 1; // Reject Sword: Turn the damage reflect state on - [Aalye]
				if ((--tsc_data[SC_REJECTSWORD].val2) <= 0)
					status_change_end(bl, SC_REJECTSWORD, -1);
			}
		}
	} // End of (sc_count != NULL && *sc_count > 0)

	if(map[bl->m].flag.gvg && tmd) {
		if (tmd->class >= 1285 && tmd->class <= 1288) {
			// Skills can't hit Emperium, except for Raging Triple Blow and Gravitation Field
			// Note: Gloria Domini/Pressure can no longer hit the Emperium, for future reference (kRO Patch) [Tsuyuki]
			if ((tmd->class == 1288 && (flag&BF_SKILL && skill_num != MO_TRIPLEATTACK)))
				return 0; // No need to continue calculating.. return 0 damage
				// Gravitation Field does 400 damage to Emperium regardless of other factors
			else if (tmd->class == 1288 && (flag&BF_SKILL && skill_num != HW_GRAVITATION)) {
				damage = 400;
				return damage;
			}
			
			if (src->type == BL_PC) {
				struct guild *g = guild_search(((struct map_session_data *)src)->status.guild_id);
				struct guild_castle *gc = guild_mapname2gc(map[bl->m].name);
			/*if (!((struct map_session_data *)src)->status.guild_id)
					return 0; //If you don't belong to a guild, return damage = 0
				else if (gc && agit_flag == 0 && class != 1288) //
					return 0; // guardians cannot be damaged during non-woe times */
				if (g == NULL)
					return 0; //return 0, player with no guild
				else if ((gc != NULL) && guild_isallied(g, gc))
					return 0; // return 0, if player is in one of his ally's castle
				else if (guild_checkskill(g, GD_APPROVAL) <= 0)
					return 0; // return 0, if player's guild does not have APPROVAL guild skill
				else if (battle_config.guild_max_castles != 0 && guild_checkcastles(g) >= battle_config.guild_max_castles)
					return 0; // [MouseJstr]
			} else
				return 0;
		}
	}

	/* War of Emperium damage reduction */
	if(damage > 0 && map[bl->m].flag.gvg && skill_num != HW_GRAVITATION && skill_num != PA_PRESSURE)
	{
		if(tmd)
		{
			struct guild_castle *gc = guild_mapname2gc(map[bl->m].name);
			if(gc)
				damage -= damage * (gc->defense / 100) * (battle_config.castle_defense_rate / 100);
		}

		if(flag & BF_SKILL)
		{
			if(flag & BF_WEAPON)
				damage = damage * battle_config.gvg_weapon_damage_rate / 100;
			else if(flag & BF_MAGIC)
				damage = damage * battle_config.gvg_magic_damage_rate / 100;
			else	/* BF_MISC */
				damage = damage * battle_config.gvg_misc_damage_rate / 100;
		} else {	/* BF_WEAPON */
			if(flag & BF_SHORT)
				damage = damage * battle_config.gvg_short_damage_rate / 100;
			else	/* BF_LONG */
				damage = damage * battle_config.gvg_long_damage_rate / 100;
		}

		if(damage < 1)
			damage = 1;
	/* pkmode pvp reduction */
	} else if(damage > 0 && tsd && battle_config.pk_mode) {
		if(flag & BF_WEAPON)
		{
			if(flag & BF_SHORT)
				damage = damage * 4 / 5;
			if(flag & BF_LONG)
				damage = damage * 7 / 10;
		}
		if(flag & BF_MAGIC || flag & BF_MISC)
			damage = damage * 3 / 5;

		if(damage < 1)
			damage = 1;
	}

	if(battle_config.skill_min_damage || flag & BF_MISC)
	{
		if (div_ < 255) {
			if (damage > 0 && damage < div_)
				damage = div_;
		}
		else if (damage > 0 && damage < 3)
			damage = 3;
	}

	if (tmd != NULL && tmd->hp > 0 && damage > 0) // îΩåÇÇ»Ç«ÇÃMOBÉXÉLÉãîªíË
		mobskill_event(tmd, flag);

/* Work In Progress - DO NOT UNCOMMENT */
// Ghostring Card and Ghost-Armor-Element Fix [Tsuyuki]
// If player's weapon ele is neutral and target is wearing Ghostring card, damage is 25%
//	if (s_ele == 0 && t_element == 8 && tsd && tsd->status.inventory.card == 4047)
//			damage /= 4;		
// If target is not wearing Ghostring Card, but element is Ghost, damage is 0
//	else if (s_ele == 0 && t_element == 8)
//		return 0; // Return 0 damage

	return damage;
}

/*==========================================
 * HP/SPãzé˚ÇÃåvéZ
 *------------------------------------------
 */
int battle_calc_drain(int damage, int rate, int per, int val)
{
	int diff = 0;

	if (damage <= 0)
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

	if (val) {
		diff += val;
	}

	return diff;
}

/*==========================================
 * èCó˚É_ÉÅÅ[ÉW
 *------------------------------------------
 */
int battle_addmastery(struct map_session_data *sd, struct block_list *target, int dmg, short race, int type)
{
	int damage, skill;
	int weapon;
	damage = 0;

	nullpo_retr(0, sd);

	// ÉfÅ[ÉÇÉìÉxÉCÉì(+3 Å` +30) vs ïséÄ or à´ñÇ (éÄêlÇÕä‹ÇﬂÇ»Ç¢ÅH)
	if ((skill = pc_checkskill(sd, AL_DEMONBANE)) > 0 && (battle_check_undead(race, status_get_elem_type(target)) || race == 6))
		damage += (skill * (3 + (sd->status.base_level + 1) / 20)); // submitted by orn
		//damage += (skill * 3);

	// ÉrÅ[ÉXÉgÉxÉCÉì(+4 Å` +40) vs ìÆï® or ç©íé
	if ((skill = pc_checkskill(sd, HT_BEASTBANE)) > 0 && (race == 2 || race == 4)) {
		damage += (skill * 4);
		if (sd->sc_data[SC_SPIRIT].timer != -1 && sd->sc_data[SC_SPIRIT].val2 == SL_HUNTER)
				damage += sd->status.str;
	}

	if(type == 0)
		weapon = sd->weapontype1;
	else
		weapon = sd->weapontype2;
	switch(weapon)
	{
		case 0x01:	// íZåï Knife
		case 0x02:	// 1HS
		{
			// åïèCó˚(+4 Å` +40) ï–éËåï íZåïä‹Çﬁ
			if((skill = pc_checkskill(sd, SM_SWORD)) > 0) {
				damage += (skill * 4);
			}
			break;
		}
		case 0x03:	// 2HS
		{
			// óºéËåïèCó˚(+4 Å` +40) óºéËåï
			if((skill = pc_checkskill(sd, SM_TWOHAND)) > 0) {
				damage += (skill * 4);
			}
			break;
		}
		case 0x04:	// 1HL
		case 0x05:	// 2HL
		{
			// ëÑèCó˚(+4 Å` +40,+5 Å` +50) ëÑ
			if((skill = pc_checkskill(sd, KN_SPEARMASTERY)) > 0) {
				if(!pc_isriding(sd))
					damage += (skill * 4);	// ÉyÉRÇ…èÊÇ¡ÇƒÇ»Ç¢
				else
					damage += (skill * 5);	// ÉyÉRÇ…èÊÇ¡ÇƒÇÈ
			}
			break;
		}
		case 0x06: // ï–éËïÄ
		case 0x07: // Axe by Tato
		{
			if((skill = pc_checkskill(sd, AM_AXEMASTERY)) > 0) {
				damage += (skill * 3);
			}
			break;
		}
		case 0x08:	// ÉÅÉCÉX
		{
			// ÉÅÉCÉXèCó˚(+3 Å` +30) ÉÅÉCÉX
			if((skill = pc_checkskill(sd, PR_MACEMASTERY)) > 0) {
				damage += (skill * 3);
			}
			break;
		}
		case 0x09:	// Ç»Çµ?
			break;
		case 0x0a:	// èÒ
			break;
		case 0x0b:	// ã|
			break;
		case 0x00:	// ëféË Bare Hands
		case 0x0c:	// Knuckles
		{
			// ìSåù(+3 Å` +30) ëféË,ÉiÉbÉNÉã
			if((skill = pc_checkskill(sd, MO_IRONHAND)) > 0) {
				damage += (skill * 3);
			}
			break;
		}
		case 0x0d:	// Musical Instrument
		{
			// äyäÌÇÃó˚èK(+3 Å` +30) äyäÌ
			if((skill = pc_checkskill(sd, BA_MUSICALLESSON)) > 0) {
				damage += (skill * 3);
			}
			break;
		}
		case 0x0e:	// Dance Mastery
		{
			// Dance Lesson Skill Effect(+3 damage for every lvl = +30) ï⁄
			if((skill = pc_checkskill(sd, DC_DANCINGLESSON)) > 0) {
				damage += (skill * 3);
			}
			break;
		}
		case 0x0f:	// Book
		{
			// Advance Book Skill Effect(+3 damage for every lvl = +30) {
			if((skill = pc_checkskill(sd, SA_ADVANCEDBOOK)) > 0) {
				damage += (skill * 3);
			}
			break;
		}
		case 0x10:	// Katars
		{
			if((skill = pc_checkskill(sd, ASC_KATAR)) > 0)
				damage += dmg * (10 + skill * 2) / 100;
			// ÉJÉ^Å[ÉãèCó˚(+3 Å` +30) ÉJÉ^Å[Éã
			if((skill = pc_checkskill(sd, AS_KATAR)) > 0) {
				//É\ÉjÉbÉNÉuÉçÅ[éûÇÕï èàóùÅi1åÇÇ…ïtÇ´1/8ìKâû)
				damage += (skill * 3);
			}
			break;
		}
	}
	damage = dmg + damage;

	return (damage);
}

struct Damage battle_calc_weapon_attack(struct block_list *src, struct block_list *target, int skill_num, int skill_lv, int wflag)
{
	struct map_session_data *sd = NULL, *tsd = NULL;
	struct pet_data *pd = NULL;
	struct mob_data *md = NULL, *tmd = NULL;

	int hitrate, flee, cri = 0;
	unsigned short baseatk = 0, baseatk_ = 0, atkmin = 0, atkmax = 0, atkmin_ = 0, atkmax_ = 0;
	int target_count;
	int def1 = status_get_def(target);
	int def2 = status_get_def2(target);
	int t_vit = status_get_vit(target);
	struct Damage wd;
	short skill =  0;
	unsigned short skillratio = 100;	//Skill dmg modifiers.
	short i;
	short t_mode = 0, t_size = 0, t_race = 0, t_ele = 0;	//target variables
	short s_race = 0, s_ele = 0, s_ele_ = 0;	//source variables
	struct status_change *sc_data, *t_sc_data;
	int bonus_damage = 0;	// fixed additional damage to be appended after damage scaling modifiers (skillratio)

	struct {
		unsigned hit : 1; //1: hitting attack, 0: miss attack
		unsigned cri : 1;		//Critical hit
		unsigned idef : 1;	//Ignore defense
		unsigned idef2 : 1;	//Ignore defense (left weapon)
		unsigned infdef : 1;	//Infinite defense (plants)
		unsigned arrow : 1;	//Attack is arrow-based
		unsigned righthand : 1;		//Attack considers right hand (wd.damage)
		unsigned lefthand : 1;		//Attack considers left hand (wd.damage2)
		unsigned weapon : 1; //It's a weapon attack (consider VVS, and all that)
		unsigned cardfix : 1; //Attack considers card % modifiers
		unsigned vit_penalty : 1;	// Apply penalty to VIT defense
		unsigned calc_base_dmg : 1;	// Already calculated base damage
		unsigned refine : 1;	// Apply weapon's refine bonus
		unsigned mastery : 1;	// Apply weapon mastery bonus
	}	flag;	

	/* Damage Templates */	// Note: Right-hand already checked by default
	//ATK_SCALE scales the damage. 100 = no change. 50 is halved, 200 is doubled, etc
	#define ATK_SCALE(a) { wd.damage = wd.damage*(a)/100 ; if(flag.lefthand) wd.damage2 = wd.damage2*(a)/100; }
	#define ATK_SCALE2(a,b) { wd.damage = wd.damage*(a)/100 ; if(flag.lefthand) wd.damage2 = wd.damage2*(b)/100; }

	//Adds dmg%. 100 = +100% (double) damage. 10 = +10% damage
	#define ATK_ADDRATE(a) { wd.damage += wd.damage*(a)/100 ; if(flag.lefthand) wd.damage2+= wd.damage2*(a)/100; }
	#define ATK_ADDRATE2(a,b) { wd.damage += wd.damage*(a)/100 ; if(flag.lefthand) wd.damage2+= wd.damage2*(b)/100; }

	//Adds an absolute value to damage. 100 = +100 damage
	#define ATK_ADD(a) { wd.damage += a; if (flag.lefthand) wd.damage2+= a; }
	#define ATK_ADD2(a,b) { wd.damage += a; if (flag.lefthand) wd.damage2+= b; }

	memset(&flag, 0, sizeof(flag));
	memset(&wd, 0, sizeof(wd)); //Initialize wd structure with 0's

	if (!(src && target))
		return wd;

	/* Initialize source and target attributes */
	s_ele = status_get_attack_element(src);   // right-hand weapon element
	s_race = status_get_race(src);            // race of source
	sc_data = status_get_sc_data(src);        // statuses of source

	t_mode = status_get_mode(target);         // mode of target (boss, plant, etc)
	t_race = status_get_race(target);         // race of target
	t_size = status_get_size(target);         // size of target
	t_sc_data = status_get_sc_data(target);   // statuses of target

	// source type
	switch (src->type) {
		case BL_PC:
			sd = (struct map_session_data *) src;
			s_race = 7;
			s_ele_ = status_get_attack_element2(src);	// left-hand weapon's element
			//wd.damage = 0; //done by menset
			break;
		case BL_MOB:
			md = (struct mob_data *) src;
			if (battle_config.enemy_str) wd.damage = status_get_baseatk(src);
			break;
		case BL_PET:
			pd = (struct pet_data *) src;
			if (battle_config.pet_str) wd.damage = status_get_baseatk(src);
			break;
	}

	// target type
	switch (target->type) {
		case BL_PC:
			tsd = (struct map_session_data *) target;
			if (pd) // pets can't target players
				return wd;
			break;
		case BL_MOB:
			tmd = (struct mob_data *) target;
			break;
	}

	/* Initial skill flags */
	flag.righthand = 1;               // ON: Attack considers right hand (wd.damage)
	flag.weapon = 1;                  // ON: It's a weapon attack (consider VVS, and all that)
	flag.cardfix = 1;                 // ON: Attack takes card % modifiers
	flag.infdef = (t_mode&0x40?1:0);	// ON: if the target is a plant type mob, otherwise OFF
	flag.vit_penalty = 1;             // ON: Apply penalty to vit def
	flag.refine = 1;                  // ON: Apply weapon's refine bonus
	flag.mastery = 1;                 // ON: Apply weapon mastery bonus

	wd.div_ = skill_num ? skill_get_num(skill_num, skill_lv) : 1;	// Number of hits based on skill, otherwise 1
	wd.amotion = (skill_num && skill_get_inf(skill_num)&2) ? 0 : status_get_amotion(src); // Amotion should be 0 for ground skills.
	if (skill_num == KN_AUTOCOUNTER)
		wd.amotion >>= 1;
	wd.dmotion = status_get_dmotion(target);
	wd.blewcount = skill_get_blewcount(skill_num, skill_lv);	// # of cells to knockback target, based on skill
	wd.flag = BF_SHORT | BF_WEAPON | BF_NORMAL;		// initial flag: short ranged, weapon attack, normal
	wd.dmg_lv = ATK_DEF;										// ATK_DEF (Hitting attack), ATK_FLEE (miss attack), ATK_LUCKY (lucky dodge)
	wd.type = 0; // normal

	/* counter-type skills */
	if (!pd) {	// players and mobs
		if((!skill_num || (tsd && battle_config.pc_auto_counter_type&2) ||
			(tmd && battle_config.monster_auto_counter_type&2)) && skill_lv >= 0) {
			if (t_sc_data) {
				if(skill_num != CR_GRANDCROSS && t_sc_data[SC_AUTOCOUNTER].timer != -1) {
					int dir = map_calc_dir(src,target->x,target->y);
					int t_dir = status_get_dir(target);
					int dist = distance(src->x,src->y,target->x,target->y);
					if(dist <= 0 || map_check_dir(dir,t_dir) ) {
						t_sc_data[SC_AUTOCOUNTER].val3 = 0;
						t_sc_data[SC_AUTOCOUNTER].val4 = 1;
						if(sc_data && sc_data[SC_AUTOCOUNTER].timer == -1) {
							int range = status_get_range(target);
							if((tsd && tsd->status.weapon != 11 && dist <= range+1) ||
								(tmd && range <= 3 && dist <= range+1) )
								t_sc_data[SC_AUTOCOUNTER].val3 = src->id;
						}
						return wd;
					} else
						flag.cri = 1;
				} else if(skill_num != CR_GRANDCROSS && t_sc_data[SC_POISONREACT].timer != -1) {
					t_sc_data[SC_POISONREACT].val3 = 0;
					t_sc_data[SC_POISONREACT].val4 = 1;
					t_sc_data[SC_POISONREACT].val3 = src->id;
				}
			}
		}
	}

	// Attack weapon type
	if (sd) {
		if (skill_num != CR_GRANDCROSS)
			sd->state.attack_type = BF_WEAPON;

		if (sd->status.weapon == 11) {				// bow attacks
			wd.flag = (wd.flag&~BF_RANGEMASK)|BF_LONG;	//Normal Attacks done with a bow type weapon: Long ranged
			flag.arrow = 1;						//Consume arrows
			if (sd->arrow_ele)
				s_ele = sd->arrow_ele;
		}
		if (!sd->weapontype1 && sd->weapontype2) {	// left-handed weapons
			flag.righthand = 0;
			flag.lefthand = 1;
		}
		if (sd->status.weapon > 16)					// two-handed weapons
			flag.righthand = flag.lefthand = 1;
	}
	else if ((pd && mob_db[pd->class].range > 3) || (md && mob_db[md->class].range > 3))
		wd.flag = (wd.flag & ~BF_RANGEMASK) | BF_LONG;

	// Perfect flee
	if (tsd || (tmd && battle_config.enemy_perfect_flee)) {
		if (!skill_num && rand()%1000 < status_get_flee2(target)) {
			wd.damage = 0;
			wd.type = 0x0b;
			wd.dmg_lv = ATK_LUCKY;
			return wd;
		}
	}

	// Flee calculation before skill mods
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
	hitrate = status_get_hit(src) - flee + 80; // AttackerHit - DefenderFlee + 80 percent chance of occuring. [Ro DataZone]

	// ********** Crit calculation before skill mods **********
	//Monsters only do a critical hit if they have used the monster skills Critical Attack or
	//Counter Attack to make an attack (and then it is an automatic critical hit).
	if(sd && !flag.cri && battle_config.enemy_critical_rate &&
	   (!skill_num || skill_num == KN_AUTOCOUNTER || skill_num == SN_SHARPSHOOTING)) {
			cri = status_get_critical(src);
			//The official equation is *2, but that only applies when sd's do critical.
			cri -= status_get_luk(target) * 2;

			cri += sd->critaddrace[t_race];
			if(flag.arrow)
				cri += sd->arrow_cri;
			if(sd->status.weapon == 16)
				cri <<= 1;

			if(battle_config.enemy_critical_rate != 100) {
				cri = cri * battle_config.enemy_critical_rate / 100;
				if (cri < 1)
					cri = 1;
			}
	
			if(t_sc_data) {
				if (t_sc_data[SC_SLEEP].timer != -1)
					cri <<= 1;
				if (t_sc_data[SC_JOINTBEAT].timer != -1 && t_sc_data[SC_JOINTBEAT].val2 == 6)
					flag.cri = 1; // Always take crits with Neck broken by Joint Beat [DracoRPG]
			}
		}

	// Skill mods
	if (skill_num) {
		if ((i = skill_get_pl(skill_num)) > 0)
			s_ele = s_ele_ = i; // take the fixed element of skill

		wd.flag = (wd.flag & ~BF_SKILLMASK) | BF_SKILL;
		switch(skill_num) {

			/************ 1st CLASS SKILLS ************/
			// archer
			case AC_CHARGEARROW:
				flag.arrow = 1;
				wd.flag = (wd.flag&~BF_RANGEMASK) | BF_LONG;
				skillratio += 50; // FORMULA: damage * 150%;
				if (sd && sd->arrow_ele > 0)
					s_ele = s_ele_ = sd->arrow_ele;
				break;
			case AC_DOUBLE:
				flag.arrow = 1;
				wd.flag = (wd.flag&~BF_RANGEMASK)|BF_LONG;
				skillratio += 80 + 20 * skill_lv; // FORMULA: damage + (100 + 20 * skill_lv)%
				if (sd && sd->arrow_ele > 0)
					s_ele = s_ele_ = sd->arrow_ele;
				break;
			case AC_SHOWER:
				flag.arrow = 1;
				wd.flag = (wd.flag&~BF_RANGEMASK) | BF_LONG;
				skillratio += 5 * skill_lv - 25; // FORMULA: damage * (75 + 5 * skill_lv) / 100
				if (sd && sd->arrow_ele > 0)
					s_ele = s_ele_ = sd->arrow_ele;
				break;
			// merchant
			case MC_CARTREVOLUTION:
				skillratio += 50;	// FORMULA: 150% + 1% every 1% weight
				if(sd && sd->cart_max_weight > 0 && sd->cart_weight > 0)
					skillratio += 100 * sd->cart_weight / sd->cart_max_weight;
				break;
			case MC_MAMMONITE:
				skillratio += 50 * skill_lv;
				s_ele = 0;
				s_ele_ = 0;
				flag.cardfix = 0;
				break;
			// swordsman
			case SM_BASH:
				skillratio += 30 * skill_lv;	// FORMULA: damage * (100 + 30 * skill_lv) / 100
				hitrate += 5 * skill_lv;
				break;
			case SM_MAGNUM:
				skillratio += 20 * skill_lv; 	// FORMULA: damage * (5 * skill_lv + (wflag ? 65 : 115)) / 100
				hitrate += 10 * skill_lv;
				break;
			// thief
			case TF_SPRINKLESAND:
				skillratio += 30;	// FORMULA?: damage * 130 / 100
				break;
			// taekwon
			case TK_COUNTER:
				flag.hit = 1;
				skillratio += 90 + 30 * skill_lv;	// FORMULA: damage * (190 + 30 * skill_lv) / 100
				if (sd && sd->weapontype1 == 0 && sd->weapontype2 == 0)
					bonus_damage += 10 * pc_checkskill(sd, TK_RUN);
				break;
			case TK_DOWNKICK:
				skillratio += 60 + 20 * skill_lv;	// FORMULA: damage * (160 + 20 * skill_lv) / 100
				if (sd && sd->weapontype1 == 0 && sd->weapontype2 == 0)
					bonus_damage += 10 * pc_checkskill(sd, TK_RUN);
				break;
			case TK_JUMPKICK:
				skillratio += -70 + 10 * skill_lv;
				if (sc_data && sc_data[SC_COMBO].timer != -1 && sc_data[SC_COMBO].val1 == skill_num)
					skillratio += 10 * status_get_lv(src) / 3;
				if (sd && sd->weapontype1 == 0 && sd->weapontype2 == 0)
					bonus_damage += 10 * pc_checkskill(sd, TK_RUN);
				break;
			case TK_STORMKICK:
				skillratio += 60 + 20 * skill_lv;	// FORMULA: damage * (160 + 20 * skill_lv) / 100
				if (sd && sd->weapontype1 == 0 && sd->weapontype2 == 0)
					bonus_damage += 10 * pc_checkskill(sd, TK_RUN);
				break;
			case TK_TURNKICK:
				wd.blewcount = 0;
				skillratio += 90 + 30 * skill_lv;	// FORMULA: damage * (190 + 30 * skill_lv) / 100
				if (sd && sd->weapontype1 == 0 && sd->weapontype2 == 0)
					bonus_damage += 10 * pc_checkskill(sd, TK_RUN);
				break;

			/************ 2-1 CLASS SKILLS ************/
			// hunter
			case HT_FREEZINGTRAP:
				skillratio += -50 + 10 * skill_lv;
				flag.hit = 1;
				break;
			// assassin
			case HT_PHANTASMIC:
				skillratio += 50;
				break;
			case AS_GRIMTOOTH:
				skillratio += 20 * skill_lv;	// FORMULA: damage * (100 + 20 * skill_lv) / 100
				wd.flag = (wd.flag & ~BF_RANGEMASK) | BF_LONG;
				break;
			case AS_POISONREACT:
				s_ele = 0;
				skillratio += 30 * skill_lv;	// FORMULA: damage * (100 + 30 * skill_lv) / 100
				break;
			case AS_SONICBLOW:
				skillratio += 300 + 40 * skill_lv;	// FORMULA?: damage * (400 + 40 * skill_lv) / 100
				if(sd && pc_checkskill(sd, AS_SONICACCEL) > 0)
					hitrate += 50;
				break;
			case AS_SPLASHER:
				skillratio += 100 + 20 * skill_lv;	// FORMULA: damage * (200 + 20 * skill_lv + 20 * pc_checkskill(sd, AS_POISONREACT)) / 100
				if(sd)
					skillratio += 20 * pc_checkskill(sd, AS_POISONREACT);
				if(wflag > 1)
					skillratio /= wflag;
				break;
			// knight
			case KN_AUTOCOUNTER:
				flag.vit_penalty = 0;
				flag.idef = flag.idef2 = 1; // ignores vit and armor defense.
				if(battle_config.monster_auto_counter_type & 1) {
					cri <<= 1;
					hitrate += 20;
				}
				else {
					flag.cri = 1;
					flag.hit = 1;
				}
				wd.flag = (wd.flag & ~BF_SKILLMASK) | BF_NORMAL;
				break;
			case KN_BOWLINGBASH:
				// DAMAGE: basedamage * (100 + 40 * skill_lv) / 100
				skillratio += 100 + (40 * skill_lv);
				wd.blewcount = 0;
				break;
			case KN_BRANDISHSPEAR:
					{
						//FORMULA: [damage * (100 + 20 * skill_lv) / 100]
						int ratio = 100 + 20 * skill_lv;
						skillratio += ratio - 100;
						if(skill_lv > 3 && wflag == 1) skillratio += ratio / 2;
						if(skill_lv > 6 && wflag == 1) skillratio += ratio / 4;
						if(skill_lv > 9 && wflag == 1) skillratio += ratio / 8;
						if(skill_lv > 6 && wflag == 2) skillratio += ratio / 2;
						if(skill_lv > 9 && wflag == 2) skillratio += ratio / 4;
						if(skill_lv > 9 && wflag == 3) skillratio += ratio / 2;
						wd.blewcount = 0;
					}
				break;
			case KN_CHARGEATK:
				skillratio += wflag * 15; // How much is the actual bonus?
				wd.blewcount = wflag > 1? wflag : 0;
				break;
			case KN_PIERCE:
				wd.div_= t_size + 1;	//number of hits depending on size of target
				skillratio += wd.div_ * (100 + 10 * skill_lv) - 100; // FORMULA: damage * (100 + 10 * skill_lv) / 100
				hitrate += hitrate * (5 * skill_lv) / 100;
				break;
			case KN_SPEARBOOMERANG:
				skillratio += 50 * skill_lv;  // FORMULA: damage * (100 + 50 * skill_lv) / 100
				wd.flag = (wd.flag & ~BF_RANGEMASK) | BF_LONG;
				break;
			case KN_SPEARSTAB:
				skillratio += 15 * skill_lv; // FORMULA: damage * (100 + 15 * skill_lv) / 100
				wd.flag = (wd.flag & ~BF_RANGEMASK) | BF_LONG;
				break;
			
			/************ 2-2 CLASS SKILLS ************/
			
			// alchemist
			case AM_ACIDTERROR:
				flag.vit_penalty = 0;
				flag.hit = 1;
				flag.cardfix = 0;
				s_ele = s_ele_ = 0; //Always neutral element attack
				skillratio += 40 * skill_lv; // FORMULA: damage * (100 + 40 * skill_lv) / 100
				wd.flag = (wd.flag & ~BF_RANGEMASK) | BF_LONG;
				break;
			case AM_DEMONSTRATION:
				flag.hit = 1;
				flag.cardfix = 0;
				skillratio += 20 * skill_lv; // FORMULA: damage * (100 + 20 * skill_lv) / 100
				break;
			// bard / dancer
			case BA_MUSICALSTRIKE:
				flag.arrow = 1;
				wd.flag = (wd.flag & ~BF_RANGEMASK) | BF_LONG;
				skillratio += 25 + 25 * skill_lv; // FORMULA?: damage * (125 + 25 * skill_lv) / 100;
				if (sd && sd->arrow_ele > 0)
					s_ele = s_ele_ = sd->arrow_ele;
				break;
			case DC_THROWARROW:
				flag.arrow = 1;
				wd.flag = (wd.flag & ~BF_RANGEMASK) | BF_LONG;
				skillratio += 25 + 25 * skill_lv;	// FORMULA?: damage * (125 + 25 * skill_lv) / 100
				if (sd && sd->arrow_ele > 0)
					s_ele = s_ele_ = sd->arrow_ele;
				break;
			// crusader
			case CR_GRANDCROSS:
			case NPC_GRANDDARKNESS:
				flag.hit = 1;
				flag.mastery = 0;
				if (!battle_config.gx_cardfix) flag.cardfix = 0;
				wd.flag = (wd.flag & ~BF_RANGEMASK) | BF_LONG;
				break;
			case CR_HOLYCROSS:
			case NPC_DARKCROSS:
				skillratio += 35 * skill_lv;	// FORMULA: damage * (100 + 35 * skill_lv) / 100
				break;
			case CR_SHIELDBOOMERANG:
				flag.weapon = 0;
				flag.calc_base_dmg = 1;
				s_ele = 0;	// always neutral element attack
				wd.flag = (wd.flag&~BF_RANGEMASK)|BF_LONG;
				
				if (sd) {
					short idx = sd->equip_index[8];
					
					wd.damage = status_get_baseatk(src);
					if (flag.lefthand)
						wd.damage2 = status_get_baseatk(src);

					if (idx >= 0 && sd->inventory_data[idx] && sd->inventory_data[idx]->type == 5)
						ATK_ADD(sd->inventory_data[idx]->weight / 10);
				}
				skillratio += 30 * skill_lv;	// FORMULA: damage * (100 + 30 * skill_lv) / 100
				// If Spirit of the Crusader is active, Shield Boomerang damage is doubled and it does not miss
				if (sd && sd->sc_data[SC_SPIRIT].timer != -1 && sd->sc_data[SC_SPIRIT].val2 == SL_CRUSADER) {
					skillratio += 100;
					flag.hit = 1;
				}
				break;
			case CR_SHIELDCHARGE:
				flag.weapon = 0;
				s_ele = 0;
				skillratio += 20 * skill_lv;	// FORMULA: damage * (100 + 20 * skill_lv) / 100
				wd.flag=(wd.flag & ~BF_RANGEMASK) | BF_SHORT;
				break;
			// monk
			case MO_BALKYOUNG:
				wd.blewcount = 0;
				skillratio += 200;
				break;
			case MO_CHAINCOMBO:
				skillratio += 50 + 50 * skill_lv;	// FORMULA: damage * (150 + 50 * skill_lv) / 100
				break;
			case MO_COMBOFINISH:
				skillratio += 140 + 60 * skill_lv;	// FORMULA: damage * (240 + 60 * skill_lv) / 100
				break;
			case MO_EXTREMITYFIST:
				// FORMULA: damage * [8 + ((sd->status.sp)/10)] + 250 + (skill_lv * 150)
				{
					unsigned int ratio = skillratio + 100 * (8 + (((sd) ? sd->status.sp : 0) / 10));
					// You'd need something like 6K SP to reach this max, so should be fine for most purposes.
					if (ratio > 60000) ratio = 60000; //We leave some room here in case skillratio gets further increased.
					skillratio = (unsigned short) ratio;
					bonus_damage += 250 + 150 * skill_lv;
					flag.hit = 1;
					flag.vit_penalty = 0;
					flag.idef = flag.idef2 = 1;	// ignore armor def and vit def
					flag.refine = 0;
					flag.mastery = 0;
					s_ele = s_ele_ = 0; //Always neutral element attack
				}
				break;
			case MO_FINGEROFFENSIVE:
				// FORMULA: damage * (100 + 50 * skill_lv) / 100
				if(!battle_config.finger_offensive_type) {
					if (sd)
						wd.div_ = sd->spiritball_old;
					skillratio += wd.div_ * (100 + 50 * skill_lv) -100;
				} else
					skillratio += 50 * skill_lv;
				wd.flag = (wd.flag&~BF_RANGEMASK)|BF_LONG;
				break;
			case MO_INVESTIGATE:
				flag.hit = 1;
				flag.vit_penalty = 0;
				flag.idef = flag.idef2 = 1;	// ignore armor and vit def
				flag.refine = 0;
				flag.mastery = 0;
				skillratio += 75 * skill_lv;
				s_ele = s_ele_ = 0; // always neutral element attack
				break;
			case MO_TRIPLEATTACK:
				skillratio += 20 * skill_lv;	// FORMULA: damage * (100 + 20 * skill_lv) / 100
				break;
			// rogue
			case RG_BACKSTAP:
				flag.hit = 1;
				//FORMULA: [damage * (200 + 40 * skill_lv) / 100) / 2]
				if(sd && sd->status.weapon == 11 && battle_config.backstab_bow_penalty)
					skillratio += (200 + 40 * skill_lv) / 2;
				else
					skillratio += 200 + 40 * skill_lv;
				break;
			case RG_INTIMIDATE:
				skillratio += 30 * skill_lv;	// FORMULA: damage * (100 + 30 * skill_lv) / 100
				break;
			case RG_RAID:
				skillratio += 40 * skill_lv;	// FORMULA: damage * (100 + 40 * skill_lv) / 100
				break;

			/************ TRANSCENDENT CLASS SKILLS [2-1] ************/
			// assassin cross
			case ASC_BREAKER:
				flag.cardfix = 0;
				skillratio += 100 * skill_lv - 100; // FORMULA: damage * skill_lv
				wd.flag = (wd.flag & ~BF_RANGEMASK) | BF_LONG;
				break;
			case ASC_METEORASSAULT:
				flag.cardfix = 0;
				skillratio += 40 * skill_lv - 60;	// FORMULA: damage * (40 + 40 * skill_lv) / 100
				break;
			// lord knight
			case LK_HEADCRUSH:
				skillratio += 40 * skill_lv;	// FORMULA: damage * (100 + 40 * skill_lv) / 100
				break;
			case LK_JOINTBEAT:
				skillratio += 10 * skill_lv - 50;	// FORMULA: damage * (50 + 10 * skill_lv) / 100
				break;
			case LK_SPIRALPIERCE:
				flag.calc_base_dmg = 1;
				flag.idef = flag.idef2 = 1;	// ignores armor and vit def
				flag.refine = 0;
				flag.mastery = 0;
				wd.flag = (wd.flag & ~BF_RANGEMASK) | BF_LONG;
				if (sd) {
					short idx = sd->equip_index[9];
					int refineDmg = 0;
					short wlv;

					if (idx >= 0) {
						wlv = sd->inventory_data[idx]->wlv;
						if (sd->inventory_data[idx] && sd->inventory_data[idx]->type == 4)
							wd.damage = sd->inventory_data[idx]->weight * 8 / 100; //80% of weight
						
						refineDmg = sd->status.inventory[idx].refine * status_getrefinebonus(wlv, 0);	// Add in refine damage. [Bison]
					}
					
					ATK_ADDRATE(50 * skill_lv); //Skill modifier applies to weight only.
					idx = sd->paramc[0] / 10; //status_get_str(src) / 10
					idx = idx * idx;
					ATK_ADD(idx); // add str bonus.
					
					//Size-fix relative modifier. t_size: 2 (large), 0 (small)
					if (t_size == 0) {
						ATK_SCALE(125);
					}
					else if (t_size == 2) {
						ATK_SCALE(75);
					}

					// Order of operations is important here, so the bonus from refinements
					// must be applied after the size multiplication. [Bison]
					ATK_ADD(refineDmg);

					ATK_SCALE(wd.div_ * 100); //Increase overall damage by number of this
				}
				else
					skillratio += 50 * skill_lv;

				if (tsd)
					tsd->canmove_tick = gettick_cache + 1000;
				else if (tmd)
					tmd->canmove_tick = gettick_cache + 1000;
				break;
			// sniper
			case SN_SHARPSHOOTING:
				skillratio += 50 * skill_lv;	// FORMULA?: damage * (50 * skill_lv) / 100
				if (!flag.cri) cri += 200;
				break;
			// whitesmith
			case WS_CARTTERMINATION:
				flag.cardfix = 0;
				// FORMULA: (damage * (cart_weight / (10 * (16 - skill_lv)))) / 100
				if(sd && sd->cart_weight > 0)
					skillratio += sd->cart_weight / (10 * (16 - skill_lv)) - 100;
				else
					skillratio += battle_config.max_cart_weight / (10 * (16 - skill_lv)) - 100;
				break;

			/************ TRANSCENDENT CLASS SKILLS [2-2] ************/
			// paladin
			case PA_SACRIFICE:
				flag.weapon = 0;
				flag.hit = 1;
				flag.calc_base_dmg = 1;
				flag.idef = flag.idef2 = 1; // ignores armor and vit defense
				s_ele = s_ele_ = 0; //Always neutral element attack
				wd.damage = status_get_max_hp(src) * 9 / 100;
				wd.damage2 = 0;
				battle_damage(src, src, wd.damage, 0); //Damage to self is always 9%
				skillratio += 10 * skill_lv - 10;	// FORMULA: damage * (90 + skill_lv * 10) / 100
				sc_data[SC_SACRIFICE].val2 --;
				if (sc_data[SC_SACRIFICE].val2 == 0)
					status_change_end(src, SC_SACRIFICE,-1);
				break;
			case PA_SHIELDCHAIN:
				// Should only calc the skill mod, not the 5 hits along with it, since the formula
				// says the 5 hits will be multiplied near the end. [Bison]
				flag.weapon = 0;
				flag.calc_base_dmg = 1;
				hitrate += 20;
				s_ele = 0; // always neutral element attack
				wd.flag=(wd.flag&~BF_RANGEMASK)|BF_LONG;

				if (sd) {
					short idx = sd->equip_index[8];
					
					wd.damage = status_get_baseatk(src);
					if (flag.lefthand)
						wd.damage2 = status_get_baseatk(src);

					if (idx >= 0) {
						if (sd->inventory_data[idx] && sd->inventory_data[idx]->type == 5)
							ATK_ADD(sd->inventory_data[idx]->weight / 10);
						
						// add 4 * shield refine level to the damage before any reductions/mods. [Bison]
						ATK_ADD(sd->status.inventory[idx].refine * status_getrefinebonus(0,1));
					}
				}
				skillratio += 30 * skill_lv;
				break;
			// champion
			case CH_CHAINCRUSH:
				skillratio += 300 + 100 * skill_lv;	// FORMULA: damage * (400 + 100 * skill_lv) / 100
				break;
			case CH_PALMSTRIKE:
				skillratio += 100 + 100 * skill_lv;	// FORMULA: damage * (200 + 100 * skill_lv) / 100
				break;
			case CH_TIGERFIST:
				skillratio += 100 * skill_lv - 60;	// FORMULA: damage * (40 + 100 * skill_lv) / 100
				break;
			// clown / gypsy
			case CG_ARROWVULCAN:
				flag.arrow = 1;
				wd.flag = (wd.flag & ~BF_RANGEMASK) | BF_LONG;
				skillratio += 100 + 100 * skill_lv;	// FORMULA: damage * (200 + 100 * skill_lv) / 100
				// kRO patch 12/14/04 - The total damage will also include the bonus from the card as well as element of the arrow.
				if (sd && sd->arrow_ele > 0)
					s_ele = s_ele_ = sd->arrow_ele;
				break;

			/************ NPC SKILLS ************/
			case NPC_COMBOATTACK:
//				div_flag = 1;
				break;
			case NPC_CRITICALSLASH:
				flag.vit_penalty = 0;
				break;
			case NPC_GUIDEDATTACK:
				flag.hit = 1;
				break;
			case NPC_PIERCINGATT:
				wd.flag=(wd.flag&~BF_RANGEMASK)|BF_SHORT;
				break;
			case NPC_RANDOMATTACK:
				skillratio += rand()%150 - 50;
				break;
			case NPC_RANGEATTACK:
				wd.flag=(wd.flag&~BF_RANGEMASK)|BF_LONG;
				break;
			case NPC_WATERATTACK:
			case NPC_GROUNDATTACK:
			case NPC_FIREATTACK:
			case NPC_WINDATTACK:
			case NPC_POISONATTACK:
			case NPC_HOLYATTACK:
			case NPC_DARKNESSATTACK:
			case NPC_UNDEADATTACK:
			case NPC_TELEKINESISATTACK:
				if (md)
					skillratio += 25 * (skill_lv-1);	// FORMULA: damage * (100 + 25 * (skill_lv-1)) / 100;
				else if (pd)
					wd.div_= pd->skillduration;
				break;

			/************ ITEM SKILLS ************/
			case ITM_TOMAHAWK:
				wd.flag = (wd.flag & ~BF_RANGEMASK) | BF_LONG;
				break;

		}
	}

	// Crit calculation after skill mods
	if (!flag.cri && cri) {
		if(tsd && tsd->critical_def) //Critical rate reduced by targets cards
			cri = cri * (100 - tsd->critical_def) / 100;
		if (rand()%1000 < cri) //Chance to perform critical attacks
			flag.cri = 1;
	}
	if (flag.cri)	{
		wd.type = 0x0a; // the type of a critical attack (used to display the damage as a critical, client side)
		flag.idef = flag.idef2 = flag.hit = 1; // Critical attacks ignore armor def, vit defense, and always hit
	}
	else if (!flag.hit) { // check for Perfect Hit
		if(sd && sd->perfect_hit && rand()%100 < sd->perfect_hit)
			flag.hit = 1;
		else if(t_sc_data && (t_sc_data[SC_SLEEP].timer  != -1 ||
		                      t_sc_data[SC_STUN].timer   != -1 ||
		                      t_sc_data[SC_FREEZE].timer != -1 ||
		                      (t_sc_data[SC_STONE].timer  != -1 && t_sc_data[SC_STONE].val2 == 0)))
				flag.hit = 1;	//If the target is froze, stone cursed, stunned, sleeping. It's a 100% hit.
		else {
			if(sd) {
				if (flag.arrow)
					hitrate += sd->arrow_hit;

				// Weaponry Research hitrate bonus
				if((skill = pc_checkskill(sd, BS_WEAPONRESEARCH)) > 0)
					hitrate += hitrate * (2 * skill) / 100;
			}
			
			if (t_sc_data && t_sc_data[SC_FOGWALL].timer != -1 && wd.flag & BF_LONG)
				hitrate -= 50;
		
			if (!sd && hitrate > 95)
				hitrate = 95;
			hitrate = (hitrate < 5)? 5 : hitrate;

			if(t_sc_data && t_sc_data[SC_READYDODGE].timer != -1 &&
			   (wd.flag & BF_LONG || t_sc_data[SC_SPURT].timer!=-1) && rand()%100 < 20) {
				wd.damage = wd.damage2 = 0;
				wd.dmg_lv = ATK_FLEE;
			}
			else if(rand()%100 >= hitrate) {
				wd.damage = wd.damage2 = 0; // miss
				wd.dmg_lv = ATK_FLEE;
			}
			else
				flag.hit = 1; // hit Connected
		}
	}

	if(tsd && tsd->special_state.no_weapon_damage)	
		return wd;

	/* Initialize needed source and target attributes */
	t_ele = status_get_elem_type(target);	// element of target

	// Hitting Attack and not a plant
	if (flag.infdef) {
		//Plants receive 1 damage when hit
		if (flag.righthand && (flag.hit || wd.damage > 1))
			wd.damage = 1;
		if (flag.lefthand && (flag.hit || wd.damage2 > 1))
			wd.damage2 = 1;
		return wd;
	} else if (flag.hit) {
		if (!flag.calc_base_dmg) { // base damage not yet calculated
			if (!sd) {	//Mobs/Pets
				baseatk = status_get_baseatk(src);
				if(skill_num == HW_MAGICCRASHER) {
					if (!flag.cri)
						atkmin = status_get_matk2(src);
					atkmax = status_get_matk1(src);
				} else {
					if (!flag.cri)
						atkmin = status_get_atk(src);
					atkmax = status_get_atk2(src);
				}
				if (atkmin > atkmax)
					atkmin = atkmax;
			} else {
				if (skill_num == HW_MAGICCRASHER) {
					baseatk = sd->matk2; // status_get_matk2(src)
					if (flag.lefthand) baseatk_ = baseatk;
				} else {
					baseatk = status_get_baseatk(src);
					if (flag.lefthand) baseatk_ = baseatk;
				}
	
				// rodatazone says that Overrefine bonuses are part of baseatk
				if(sd->overrefine)
					baseatk += rand() % sd->overrefine + 1;
				if(flag.lefthand && sd->overrefine_)
					baseatk_ += rand() % sd->overrefine_ + 1;
		
				atkmax = sd->watk; // status_get_atk(src);
				if (flag.lefthand)
					atkmax_ = sd->watk_; // status_get_atk(src);

				if (!flag.cri || flag.arrow) { // normal/bow attacks
					atkmin = atkmin_ = sd->paramc[4]; //status_get_dex(src);
					if (sd->equip_index[9] >= 0 && sd->inventory_data[sd->equip_index[9]])
						atkmin = atkmin * (80 + sd->inventory_data[sd->equip_index[9]]->wlv * 20) / 100;
					if (atkmin > atkmax)
						atkmin = atkmax;
					if (flag.lefthand) {
						if(sd->equip_index[8] >= 0 && sd->inventory_data[sd->equip_index[8]])
							atkmin_ = atkmin_ * (80 + sd->inventory_data[sd->equip_index[8]]->wlv * 20) / 100;
						if (atkmin_ > atkmax_)
							atkmin_ = atkmax_;
					}
					if (flag.arrow) { // bows
						sd->state.arrow_atk = 1;
						atkmin = atkmin * atkmax / 100;
						if (atkmin > atkmax)
							atkmax = atkmin;
					}
				}
			}

			if(sc_data && sc_data[SC_MAXIMIZEPOWER].timer != -1) {
				atkmin = atkmax;
				atkmin_ = atkmax_;
			}

			// Weapon Damage Calculation
			if(!flag.cri) {
				wd.damage += (atkmax > atkmin? rand()% (atkmax - atkmin) : 0) + atkmin;
				if(flag.lefthand)
					wd.damage2 += (atkmax_>atkmin_? rand() % (atkmax_ - atkmin_) : 0) +atkmin_;
			} else {
				wd.damage += atkmax;
				if (flag.lefthand)
					wd.damage2 += atkmax_;
			}

			if (sd) {
				// rodatazone says the range is 0~arrow_atk-1 for non-crits
				if(sd->arrow_atk)
					wd.damage += ((flag.cri) ? sd->arrow_atk : rand()%sd->arrow_atk);

				// size fix for players
				if (!(sd->special_state.no_sizefix || (sc_data && sc_data[SC_WEAPONPERFECTION].timer != -1) ||
					(pc_isriding(sd) && (sd->status.weapon == 4 || sd->status.weapon == 5) && t_size == 1) ||
					skill_num == MO_EXTREMITYFIST)) {
					wd.damage = wd.damage * (sd->atkmods[t_size])/100;
					if(flag.lefthand)
						wd.damage2 = wd.damage2 * (sd->atkmods_[t_size])/100;
				}
			}	

			// Add BaseATK
			wd.damage += baseatk;
			if(flag.lefthand)
				wd.damage2 += baseatk_;

			//Add any bonuses that modify the base baseatk+watk (pre-skills)
			if (sd) {
				int c = 0;
				if (sd->status.weapon < 16 && (sd->atk_rate!= 100 || sd->weapon_atk_rate[sd->status.weapon] != 0))
					ATK_SCALE(sd->atk_rate+ sd->weapon_atk_rate[sd->status.weapon]);
				if (flag.cri && sd->crit_atk_rate)
					ATK_ADDRATE(sd->crit_atk_rate);
				if(sd->status.party_id && (skill=pc_checkskill(sd,TK_POWER)) > 0){
					party_foreachsamemap(party_sub_count, sd, 1, &c);
					ATK_ADDRATE(2*skill*(c-1));
				}
			}
		}

		// skill damage bonuses
		if(sc_data && skill_num != PA_SACRIFICE && skill_num != MC_MAMMONITE) {
			if(sc_data[SC_OVERTHRUST].timer != -1)
				skillratio += 5 * sc_data[SC_OVERTHRUST].val1;
			if(sc_data[SC_MAXOVERTHRUST].timer != -1)
				skillratio += 20 * sc_data[SC_MAXOVERTHRUST].val1;
			if(sc_data[SC_BERSERK].timer != -1)
				skillratio += 100;
		}

		if (sd && !skill_num) {
			// Random chance to deal multiplied damage - Consider it as part of skill-based-damage
			if(sd->random_attack_increase_add > 0 && sd->random_attack_increase_per && rand()%100 < sd->random_attack_increase_per)
				skillratio += sd->random_attack_increase_add;
		}

		if (skill_num == MO_INVESTIGATE)	// Special target defence adjustment for Occult Impaction
			ATK_SCALE(2 * (def1 + def2));

		ATK_SCALE(skillratio);	// apply damage modifer

		skillratio = 100;	// Second pass for skills that stack to the previously defined % damage
		// Skill damage modifiers that affect linearly stacked damage.
		if (sc_data && skill_num != PA_SACRIFICE) {
			if(sc_data[SC_TRUESIGHT].timer != -1)
				skillratio += 2 * sc_data[SC_TRUESIGHT].val1;
			if(sc_data[SC_EDP].timer != -1 && skill_num != ASC_BREAKER && skill_num != ASC_METEORASSAULT && skill_num != AS_VENOMKNIFE)
				skillratio += 50 + 50 * sc_data[SC_EDP].val1;
		}
			
		if(skill_num == AS_SONICBLOW) {
			// Soul Link bonus does not stack with EDP
			if (sd && sd->sc_data[SC_EDP].timer == -1 && sd->sc_data[SC_SPIRIT].timer != -1 && sd->sc_data[SC_SPIRIT].val2 == SL_ASSASIN) {
				if(map[sd->bl.m].flag.gvg) // If GvG map, +50% damage with Spirit of the Assassin, if not GvG, +100%
					skillratio += 50;
				else
					skillratio += 100;
			}
			if(sd && pc_checkskill(sd, AS_SONICACCEL) > 0) // Sonic Acceleration 10% Sonic Blow damage bonus
					skillratio += 10;
		}
			

		if(sd && sd->skillatk[0].id != 0 && skill_num) {
			for (i = 0; i < MAX_PC_BONUS && sd->skillatk[i].id != 0 && sd->skillatk[i].id != skill_num; i++);
				if (i < MAX_PC_BONUS && sd->skillatk[i].id == skill_num)
					skillratio += sd->skillatk[i].val;
		}

		ATK_ADD(bonus_damage);	// add fixed amount of additional damage
		if(skillratio != 100)
			ATK_SCALE(skillratio);	// apply damage modifier

		if(sd && skill_num != CR_GRANDCROSS) {
			// Added Shield Chain/Boomerang  to ignore the Ice Pick effects. [Bison]
			if (skill_num != PA_SACRIFICE && skill_num != MO_INVESTIGATE && skill_num != PA_SHIELDCHAIN
			    && skill_num != CR_SHIELDBOOMERANG && !flag.cri) { //Elemental/Racial adjustments
				char raceele_flag = 0, raceele_flag_ = 0;
				if(sd->def_ratio_atk_ele & (1<<t_ele) || sd->def_ratio_atk_race & (1<<t_race) ||
					sd->def_ratio_atk_race & (t_mode&0x20?1<<10:1<<11))
					raceele_flag = flag.idef = 1;

				// Pass effect onto right hand if configured so
				if(sd->def_ratio_atk_ele_ & (1<<t_ele) || sd->def_ratio_atk_race_ & (1<<t_race) ||
					sd->def_ratio_atk_race_ & (t_mode&0x20?1<<10:1<<11)) {
					if (battle_config.left_cardfix_to_right && flag.righthand)
						raceele_flag = flag.idef = 1;
					else
						raceele_flag_ = flag.idef2 = 1;
				}

				if (raceele_flag || raceele_flag_)
					ATK_SCALE2(raceele_flag?(def1 + def2):100, raceele_flag_?(def1 + def2):100);
			}
	
			if (!flag.idef && ((tmd && sd->ignore_def_mob_ & (t_mode&0x20?2:1)) || sd->ignore_def_ele & (1<<t_ele) ||
				sd->ignore_def_race & (1<<t_race) || sd->ignore_def_race & (t_mode&0x20?1<<10:1<<11)))
				flag.idef = 1;

			if (!flag.idef2 && ((tmd && sd->ignore_def_mob_ & (t_mode&0x20?2:1)) || sd->ignore_def_ele_ & (1<<t_ele) ||
				sd->ignore_def_race_ & (1<<t_race) || sd->ignore_def_race_ & (t_mode&0x20?1<<10:1<<11))) {
				if(battle_config.left_cardfix_to_right && flag.righthand) // move effect to right hand
					flag.idef = 1;
				else
					flag.idef2 = 1;
			}
		}

		// Defense Reduction
		if (!flag.idef || !flag.idef2) {
			if (flag.vit_penalty && def1 < 1000000) {	//DEF, VITñ≥éã
				short vit_def;
				if(battle_config.vit_penalty_type) {
					target_count = battle_counttargeted(target, src, battle_config.vit_penalty_count_lv) - battle_config.vit_penalty_count;
					if(target_count > 0) {
						if(battle_config.vit_penalty_type == 1) {
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
					}
					if (def1 < 0 || skill_num == AM_ACIDTERROR) def1 = 0; //Acid Terror ignores only armor defense. [Skotlex]
					if (def2 < 1) def2 = 1;
					if (t_vit < 1) t_vit = 1;
				}
				// Vitality reduction from rodatazone: http://rodatazone.simgaming.net/mechanics/substats.php#def	
				if (tsd) {	//Player vit eq: [VIT*0.5] + rnd([VIT*0.3], max([VIT*0.3],[VIT^2/150]-1))
					vit_def = def2 * (def2 - 15) / 150;
					vit_def = def2 / 2 + (vit_def > 0?rand()%vit_def:0);
				
					if((battle_check_undead(s_race, status_get_elem_type(src)) || s_race == 6) && (skill = pc_checkskill(tsd, AL_DP)) >0)
					vit_def += skill * (int)(3 +(tsd->status.base_level + 1) * 0.04);
				} else { //Mob-Pet vit-eq
					//VIT + rnd(0,[VIT/20]^2-1)
					vit_def = (def2 / 20) * (def2 / 20);
					vit_def = def2 + (vit_def > 0?rand()%vit_def:0);
				}
			
				if (sd && battle_config.player_defense_type)
					vit_def += def1 * battle_config.player_defense_type;
				else if (md && battle_config.monster_defense_type)

					vit_def += def1 * battle_config.monster_defense_type;
				else if (pd && battle_config.pet_defense_type)

					vit_def += def1 * battle_config.pet_defense_type;
				else
					ATK_SCALE2(flag.idef?100:100-def1, flag.idef2?100:100-def1);
				ATK_ADD2(flag.idef?0:-vit_def, flag.idef2?0:-vit_def);
			}
		}

		// Post skill/vit reduction damage increases
		if (sc_data && sc_data[SC_AURABLADE].timer != -1 && skill_num != LK_SPIRALPIERCE)
			ATK_ADD(20 * sc_data[SC_AURABLADE].val1);

		// Refine bonus. Added Spiral Pierce because it gets its own treatment up above. [Bison]
		if (sd && flag.weapon && flag.refine) {
			if (skill_num == MO_FINGEROFFENSIVE) { //Counts refine bonus multiple times
				ATK_ADD2(wd.div_ * sd->watk2/*status_get_atk2(src)*/, wd.div_ * sd->watk_2/*status_get_atk_2(src)*/);
			} else {
				ATK_ADD2(sd->watk2/*status_get_atk2(src)*/, sd->watk_2/*status_get_atk_2(src)*/);
			}
		}

		// Set weapon damage to min of 1
		if (flag.righthand && wd.damage < 1) wd.damage = 1;
		if (flag.lefthand && wd.damage2 < 1) wd.damage2 = 1;

		// Added Spiral Pierce, since mastery should not add to the damage of the skill. [Bison]
		if (sd && flag.weapon && flag.mastery) {	//Add mastery damage
			wd.damage = battle_addmastery(sd, target, wd.damage, t_race, 0);
			if (flag.lefthand)
				wd.damage2 = battle_addmastery(sd, target, wd.damage2, t_race, 1);
		}
	}

	if(skill_num == CR_GRANDCROSS)
		return wd; //Enough, rest is not needed.

	if(sd && (skill = pc_checkskill(sd, BS_WEAPONRESEARCH)) > 0)
		ATK_ADD(skill * 2);

	if(skill_num == TF_POISON)
		ATK_ADD(15 * skill_lv);

	/* elemental damage fix */
	if((sd && (skill_num || !battle_config.pc_attack_attr_none)) || (md && (skill_num || !battle_config.mob_attack_attr_none)) || (pd && (skill_num || !battle_config.pet_attack_attr_none)))
	{
		if(!(sd && tsd && battle_config.mob_ghostring_fix && t_ele == 8))
		{
			short t_element = status_get_element(target);
			if(wd.damage > 0)
			{
				/* Cart Revolution always deals neutral damage */
				if(skill_num == MC_CARTREVOLUTION)
					s_ele = 0;
				wd.damage = battle_attr_fix(wd.damage, s_ele, t_element);
			}
			if(wd.damage2 > 0 && flag.lefthand)
				wd.damage2 = battle_attr_fix(wd.damage2, s_ele, t_element);
		}
	}

	if ((!flag.righthand || wd.damage == 0) && (!flag.lefthand || wd.damage2 == 0))
		flag.cardfix = 0;	// When the attack does no damage, avoid doing %bonuses

	if (sd) {
		if (skill_num != CR_SHIELDBOOMERANG) // Only Shield Boomerang doesn't takes the Star Crumbs bonus.
			ATK_ADD2(wd.div_ * sd->star, wd.div_ * sd->star_);
		if (skill_num == MO_FINGEROFFENSIVE) { // The finger offensive spheres on moment of attack do count. [Skotlex]
			ATK_ADD(wd.div_ * sd->spiritball_old * 3);
		} else {
			ATK_ADD(wd.div_ * sd->spiritball * 3);
		}

		// Shield Chain formula where: SRR + rand() % 101 is added after def reductions, but before card mods. [Bison]
		if (skill_num == PA_SHIELDCHAIN) {
			short idx = sd->equip_index[8];
			int shieldRefineAmt = 0;
			int SRR = 0;
			
			if (idx >= 0 && sd->inventory_data[idx] && sd->inventory_data[idx]->type == 5)
				shieldRefineAmt = sd->status.inventory[idx].refine;
				
			// The SRR values are from Doddler's RO calc. [Bison]
			SRR = shieldRefineAmt;
			switch(shieldRefineAmt) {
				case 3:
					SRR = 7;
					break;
				case 4:
					SRR = 16;
					break;
				case 5:
					SRR = 27;
					break;
				case 6:
					SRR = 40;
					break;
				case 7:
					SRR = 55;
					break;
				case 8:
					SRR = 72;
					break;
				case 9:
					SRR = 91;
					break;
				case 10:
					SRR = 112;
					break;
			}
			// According to the formula, there should be a random damage mod that goes from 0-100 inclusive. [Bison]
			ATK_ADD(SRR + rand() % 101);
			// Multiply by the number of hits Shield Chain does, which is 5 hits for all levels.
			// This is essentially multiplying the current damage by 5.
			// Notice where this is done, which is after the damage reductions from def.  [Bison]
			ATK_SCALE(wd.div_ * 100);
		}

		//Card Fix for players
		if (flag.cardfix) {
			short cardfix = 1000, cardfix_ = 1000;
			short t_class = status_get_class(target);
			short t_race2 = status_get_race2(target);	
			if(sd->state.arrow_atk) {
				cardfix = cardfix * (100 + sd->addrace[t_race] + sd->arrow_addrace[t_race])/100;
				cardfix = cardfix * (100 + sd->addele[t_ele]   + sd->arrow_addele[t_ele])/100;
				cardfix = cardfix * (100 + sd->addsize[t_size] + sd->arrow_addsize[t_size])/100;
				cardfix = cardfix * (100 + sd->addrace2[t_race2])/100;
				cardfix = cardfix * (100 + sd->addrace[t_mode&0x20?10:11] + sd->arrow_addrace[t_mode&0x20?10:11])/100;
			} else {	//Melee attack
				if(!battle_config.left_cardfix_to_right) {
					cardfix = cardfix * (100 + sd->addrace[t_race])/100;
					cardfix = cardfix * (100 + sd->addele[t_ele])/100;
					cardfix = cardfix * (100 + sd->addsize[t_size])/100;
					cardfix = cardfix * (100 + sd->addrace2[t_race2])/100;
					cardfix = cardfix * (100 + sd->addrace[t_mode&0x20?10:11])/100;

					if (flag.lefthand) {
						cardfix_ = cardfix_ *(100 + sd->addrace_[t_race])/100;
						cardfix_ = cardfix_ *(100 + sd->addele_[t_ele])/100;
						cardfix_ = cardfix_ *(100 + sd->addsize_[t_size])/100;
						cardfix_ = cardfix_ *(100 + sd->addrace2_[t_race2])/100;
						cardfix_ = cardfix_ *(100 + sd->addrace_[t_mode&0x20?10:11])/100;
					}					
				} else {
					cardfix = cardfix * (100 + sd->addrace[t_race]   + sd->addrace_[t_race])/100;
					cardfix = cardfix * (100 + sd->addele[t_ele]     + sd->addele_[t_ele])/100;
					cardfix = cardfix * (100 + sd->addsize[t_size]   + sd->addsize_[t_size])/100;
					cardfix = cardfix * (100 + sd->addrace2[t_race2] + sd->addrace2_[t_race2])/100;
					cardfix = cardfix * (100 + sd->addrace[t_mode&0x20?10:11] + sd->addrace_[t_mode&0x20?10:11])/100;
				}
			}
			
			/* SP_ADD_DAMAGE_CLASS bonus*/
			for(i = 0; i < sd->add_damage_class_count; i++) {
				if(sd->add_damage_classid[i] == t_class) {
					cardfix = cardfix * (100 + sd->add_damage_classrate[i]) / 100;
					break;
				}
			}

			if (flag.lefthand) {
				for(i = 0; i < sd->add_damage_class_count_; i++) {
					if(sd->add_damage_classid_[i] == t_class) {
						cardfix_ = cardfix_ * (100 + sd->add_damage_classrate_[i]) / 100;
						break;
					}
				}
			}
			/* End SP_ADD_DAMAGE_CLASS bonus*/
			
			if (cardfix != 1000 || cardfix_ != 1000)
				ATK_SCALE2(cardfix / 10, cardfix_ / 10);	//What happens if you use right-to-left and there's no right weapon, only left?
		}

	}

	if (sd && skill_num == CR_SHIELDBOOMERANG) { //Refine bonus applies after cards and elements.
		short idx = sd->equip_index[8];
		if (idx >= 0 && sd->inventory_data[idx] && sd->inventory_data[idx]->type == 5)
			ATK_ADD(10 * sd->status.inventory[idx].refine);
	}

	//Card Fix, tsd side - Cards always apply on the target. [Skotlex]
	if (tsd) {
		short cardfix = 1000;
		short s_size = status_get_size(src);			// size of source

		cardfix = cardfix * (100 - tsd->subele[s_ele]) / 100;
		cardfix = cardfix * (100 - tsd->subsize[s_size]) / 100;
		cardfix = cardfix * (100 - tsd->subrace[s_race]) / 100;

		if (md && mob_db[md->class].mode & 0x20)
			cardfix = cardfix * (100 - tsd->subrace[10]) / 100;
		else
			cardfix = cardfix * (100 - tsd->subrace[11]) / 100;

		for(i = 0; i < tsd->add_def_class_count; i++) {
			if (md && tsd->add_def_classid[i] == md->class) {
				cardfix = cardfix * (100 - tsd->add_def_classrate[i]) / 100;
				break;
			}
		}

		/* Add damage by race */
		for(i = 0; i < tsd->add_damage_class_count2; i++) {
			if ((sd && tsd->add_damage_classid2[i] == sd->status.class) ||
			    (md && tsd->add_damage_classid2[i] == md->class))  {
				cardfix = cardfix * (100 + tsd->add_damage_classrate2[i]) / 100;
				break;
			}
		}
	
		if(wd.flag&BF_SHORT)
			cardfix = cardfix * (100 - tsd->near_attack_def_rate)/100;
		else	// BF_LONG (there's no other choice)
			cardfix = cardfix * (100 - tsd->long_attack_def_rate)/100;

		if (cardfix != 1000)
			ATK_SCALE(cardfix / 10);
	}

	// Double Attack.
	if(sd && !skill_num && !flag.cri) {	
		if(((skill_lv = 5 * pc_checkskill(sd, TF_DOUBLE)) > 0 && sd->weapontype1 == 0x01) ||
			sd->double_rate > 0) //Success chance is not added, the higher one is used? [Skotlex]
			if (rand()%100 < (skill_lv > sd->double_rate?skill_lv : sd->double_rate)) {
				wd.damage *= 2;
				wd.div_ = 2; //Number of hits (double attack)
				wd.type = 0x08;
			}
	}

	if(!flag.righthand/* || wd.damage < 1*/) wd.damage = 0;
	if(!flag.lefthand/* || wd.damage2 < 1*/) wd.damage2 = 0;

	if (sd) {
		if (!flag.righthand && flag.lefthand) { // move lefthand damage to the righthand
			wd.damage = wd.damage2;
			wd.damage2 = 0;
			flag.righthand = 1;
			flag.lefthand = 0;
		} else if(sd->status.weapon > 16) { //Dual-wield
			if (wd.damage > 0) {
				skill = pc_checkskill(sd, AS_RIGHT);
				wd.damage = wd.damage * (50 + (skill * 10))/100;
				if(wd.damage < 1) wd.damage = 1;
			}
			if (wd.damage2 > 0) {
				skill = pc_checkskill(sd, AS_LEFT);
				wd.damage2 = wd.damage2 * (30 + (skill * 10))/100;
				if(wd.damage2 < 1) wd.damage2 = 1;
			}
		} else if(sd->status.weapon == 16) { //Katars
			skill = pc_checkskill(sd, TF_DOUBLE);
			wd.damage2 = wd.damage * (1 + (skill * 2))/100;

			if(wd.damage > 0 && wd.damage2 < 1) wd.damage2 = 1;
			flag.lefthand = 1;
		}
	}

	if (wd.damage > 0 || wd.damage2 > 0) {
		if(wd.damage2 < 1)
			wd.damage = battle_calc_damage(src, target, wd.damage, wd.div_, skill_num, skill_lv, wd.flag);
		else if(wd.damage < 1)
			wd.damage2 = battle_calc_damage(src, target, wd.damage2, wd.div_, skill_num, skill_lv, wd.flag);
		else {
			int d1 = wd.damage + wd.damage2, d2 = wd.damage2;
			wd.damage = battle_calc_damage(src, target, d1, wd.div_, skill_num, skill_lv, wd.flag);
			wd.damage2 = (d2 * 100 / d1) * wd.damage / 100;
			if(wd.damage > 1 && wd.damage2 < 1) wd.damage2 = 1;
			wd.damage -= wd.damage2;
		}
	}

	// Equipment Breaking
	if (sd && battle_config.equipment_breaking && (wd.damage > 0 || wd.damage2 > 0)) {
		int breakrate = 1; // 0.01% default self weapon breaking chance [DracoRPG]
		int breakrate_[2] = {0,0}; //enemy breaking chance, weapon = 0, armor = 1 [celest]

		breakrate_[0] += sd->break_weapon_rate;
		breakrate_[1] += sd->break_armor_rate;

		if(sd->sc_count)
		{
			if(sd->sc_data[SC_MELTDOWN].timer != -1 && skill_num != WS_CARTTERMINATION)
			{
				breakrate_[0] += 100 * sd->sc_data[SC_MELTDOWN].val1;	//weapon
				breakrate_[1] = 70 * sd->sc_data[SC_MELTDOWN].val1;		//armor
			}
			if(sd->sc_data[SC_OVERTHRUST].timer != -1)
				breakrate += 10;
			if(sd->sc_data[SC_MAXOVERTHRUST].timer != -1)
				breakrate += 10;
		}

		if (rand() % 10000 < breakrate * battle_config.equipment_break_rate / 100 || breakrate >= 10000)
			pc_breakweapon(sd);

		if (rand() % 10000 < breakrate_[0] * battle_config.equipment_break_rate / 100 || breakrate_[0] >= 10000) {
			if (tsd)
				pc_breakweapon(tsd); //target is a player, break his weapon
		}
		if (rand() % 10000 < breakrate_[1] * battle_config.equipment_break_rate / 100 || breakrate_[1] >= 10000) {
			if (tsd)
				pc_breakarmor(tsd); //target is a player, break his armor
		}
	}

	// Azoth/Hylozoist Card: Changes monster into a random, Non-MvP/Duplicate/Event monster
	if(sd && sd->classchange && tmd && !(tmd->guild_id || tmd->class == 1288 || t_mode&0x20)) {
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
			1176,1177,1178,1179,1180,1188,1189,1191,1192,1193,1194,1195,1196,1197,1199,1200,1201,
			1202,1204,1205,1206,1207,1208,1209,1211,1212,1213,1214,1215,1216,1219,1242,1243,1245,
			1246,1247,1248,1249,1250,1253,1254,1255,1256,1257,1258,1260,1261,1263,1264,1265,1266,
			1267,1269,1270,1271,1273,1274,1275,1276,1277,1278,1280,1281,1282,1291,1292,1293,1294,
			1295,1297,1298,1300,1301,1302,1304,1305,1306,1308,1309,1310,1311,1313,1314,1315,1316,
			1317,1318,1319,1320,1321,1322,1323,1365,1366,1367,1368,1369,1370,1371,1372,1374,1375,
			1376,1377,1378,1379,1380,1381,1382,1383,1384,1385,1386,1387,1390,1391,1392,1400,1401,
			1402,1403,1404,1405,1406,1408,1409,1410,1412,1413,1415,1416,1417,1493,1494,1495,1497,
			1498,1499,1500,1503,1504,1505,1506,1507,1508,1509,1510,1512,1513,1514,1515,1516,1517,
			1519,1520,1582,1584,1585,1586,1587,1613,1614,1615,1616,1617,1618,1619,1620,1621,1622,
			1623,1627,1628,1629,1632,1633,1634,1635,1636,1637,1638,1639,1652,1653,1654,1655,1656,
			1657,1664,1669,1674,1676,1680,1681,1682,1683,1684,1686,1687,1692,1693,1698,1699,1700,
			1701,1702,1703,1704,1705,1706,1707,1713,1714,1715,1716,1717,1718,1720,1721,1735,1736,
			1737,1738,1752,1753,1754,1755,1765,1769,1770,1771,1772,1773,1774,1775,1776,1777,1778,
			1780,1781,1782,1783,1784 };
			mob_class_change(tmd, changeclass, sizeof(changeclass) / sizeof(changeclass[1]));
		}
	}

	return wd;
}

/*==========================================
 * ñÇñ@É_ÉÅÅ[ÉWåvéZ
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
	int flag_aoe = 0;
	int ele = 0, race = 7, size = 1, t_ele = 0, t_race = 7, t_mode = 0, cardfix, t_class, i;
	struct map_session_data *sd = NULL, *tsd = NULL;
	struct mob_data *mb = NULL, *tmd = NULL;

	//returnëOÇÃèàóùÇ™Ç†ÇÈÇÃÇ≈èÓïÒèoóÕïîÇÃÇ›ïœçX
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

	if (bl->type == BL_PC && (sd = (struct map_session_data *)bl)) {
		sd->state.attack_type = BF_MAGIC;
		sd->state.arrow_atk = 0;
	} else if (bl->type == BL_MOB)
		mb = (struct mob_data *)bl;
		
	if( target->type == BL_PC )
		tsd = (struct map_session_data *)target;
	else if( target->type == BL_MOB )
		tmd = (struct mob_data *)target;

	if (mb && distance(bl->x, bl->y, target->x, target->y) < 3)
		aflag = BF_MAGIC|BF_SHORT|BF_SKILL;
	else
		aflag = BF_MAGIC|BF_LONG|BF_SKILL;

	if(skill_num > 0)
	{
		switch(skill_num)
		{
		case AL_HEAL:
		case PR_BENEDICTIO:
			damage = skill_calc_heal(bl, skill_lv) / 2;
			if(sd)
				damage += damage * pc_checkskill(sd, HP_MEDITATIO) * 2 / 100;
			normalmagic_flag = 0;
			break;
		case PR_ASPERSIO:
			damage = 40;
			normalmagic_flag = 0;
			break;
		case PR_SANCTUARY:
			damage = (skill_lv > 6)? 388 : skill_lv * 50;
			normalmagic_flag = 0;
			blewcount |= 0x10000;
			break;
		case ALL_RESURRECTION:
		case PR_TURNUNDEAD:	// çUåÇÉäÉUÉåÉNÉVÉáÉìÇ∆É^Å[ÉìÉAÉìÉfÉbÉh
			if(battle_check_undead(t_race, t_ele)) {
				int hp, mhp, thres;
				hp = status_get_hp(target);
				mhp = status_get_max_hp(target);
				thres = (skill_lv * 20) + status_get_luk(bl)+
						status_get_int(bl) + status_get_lv(bl)+
						((200 - hp * 200 / mhp));
				if(thres > 700) thres = 700;
//				if(battle_config.battle_log)
//					printf("É^Å[ÉìÉAÉìÉfÉbÉhÅI ämó¶%d ÅÒ(êÁï™ó¶)\n", thres);
				if(rand()%1000 < thres && !(t_mode&0x20))	// ê¨å˜
					damage = hp;
				else					// é∏îs
					damage = status_get_lv(bl) + status_get_int(bl) + skill_lv * 10;
			}
			normalmagic_flag = 0;
			break;

		case MG_NAPALMBEAT:	// ÉiÉpÅ[ÉÄÉrÅ[ÉgÅiï™éUåvéZçûÇ›Åj
			MATK_FIX(70+ skill_lv*10,100);
			if(flag>0){
				MATK_FIX(1,flag);
			}else {
				if(battle_config.error_log)
					printf("battle_calc_magic_attack(): napam enemy count=0 !\n");
			}
			break;
		case MG_FIREBALL:
			{
				const int drate[] = {100, 90, 70};
				if(flag > 2)
					matk1 = matk2 = 0;
				else
					MATK_FIX((95 + skill_lv * 5) * drate[flag], 10000);
				flag_aoe = 1;
			}
			break;
		case MG_FIREWALL:
			if(t_ele == 3 || battle_check_undead(t_race, t_ele))
				blewcount = 0;
			else
				blewcount |= 0x10000;
			MATK_FIX(1, 2);
			flag_aoe = 1;
			break;
		case MG_THUNDERSTORM:
			MATK_FIX(80, 100);
			flag_aoe = 1;
			break;
		case MG_FROSTDIVER:
			MATK_FIX(100 + skill_lv * 10, 100);
			break;
		case WZ_FROSTNOVA:
			MATK_FIX((100 + skill_lv * 10) * 2 / 3, 100);
			flag_aoe = 1;
			break;
		case WZ_FIREPILLAR:
			if(mdef1 < 1000000)
				mdef1 = mdef2 = 0;
			MATK_FIX(1, 5);
			matk1+=50;
			matk2+=50;
			break;
		case WZ_SIGHTRASHER:
			MATK_FIX(100 + skill_lv * 20, 100);
			break;
		case WZ_METEOR:
		case WZ_JUPITEL:
			flag_aoe = 1;
			break;
		case WZ_VERMILION:
			MATK_FIX(skill_lv * 20 + 80, 100);
			flag_aoe = 1;
			break;
		case WZ_WATERBALL:
			MATK_FIX(100 + skill_lv * 30, 100);
			break;
		case WZ_STORMGUST:
			MATK_FIX(skill_lv * 40 + 100, 100);
			flag_aoe = 1;
			break;
		case AL_HOLYLIGHT:
			if (sd && sd->sc_data[SC_SPIRIT].timer != -1 && sd->sc_data[SC_SPIRIT].val2 == SL_PRIEST) {
				MATK_FIX(625, 100);
			}	else {
				MATK_FIX(125, 100);
			}
			break;
		case AL_RUWACH:
			MATK_FIX(145, 100);
			break;
		case HW_NAPALMVULCAN:
			MATK_FIX(70 + skill_lv * 10, 100);
			if(flag>0){
				MATK_FIX(1, flag);
			} else {
				if(battle_config.error_log)
					printf("battle_calc_magic_attack(): napalmvulcan enemy count=0 !\n");
			}
			break;
		case PF_SOULBURN:
			if (target->type != BL_PC || skill_lv < 5) {
				memset(&md, 0, sizeof(md));
				return md;
			} else if (tsd) {
				damage = tsd->status.sp * 2;
				matk_flag = 0; // don't consider matk and matk2
			}
			break;
// A lot of Corrections to BREAKER SKILL (pneuma included) (Posted on freya's bug report by Gawaine)
		case ASC_BREAKER:
			damage = rand() % 500 + 500 + skill_lv * status_get_int(bl) * 5;
			normalmagic_flag = 0; //Damage is not affected by matk and ignores mdef
			break;
// End ----------- A lot of Corrections to BREAKER SKILL (pneuma included) (Posted on freya's bug report by Gawaine)

		case HW_GRAVITATION:
			damage = 200 + 200 * skill_lv;
			if(tmd && tmd->class == 1288) //According reports, it has a fixed damage of 400 on emp [Proximus]
				damage = 400;
			flag_aoe = 1;
			normalmagic_flag = 0;
			break;
		case SL_SMA:
			MATK_FIX(40 + sd->status.base_level, 100);
			// Esma takes element of Warm Wind
			ele = status_get_attack_element(bl);
			break;
		case SL_STIN:
			if(status_get_size(target) == 0) {
				MATK_FIX(10 * skill_lv, 100);
			} else {
				MATK_FIX(skill_lv, 100);
			}
			break;
		case SL_STUN:
			MATK_FIX(5 * skill_lv, 100);
			break;
		}
	}

	if(normalmagic_flag){	// àÍî ñÇñ@É_ÉÅÅ[ÉWåvéZ
		int imdef_flag = 0;
		if (matk_flag) {
			if(matk1 > matk2)
				damage = matk2 + rand()%(matk1 - matk2 + 1);
			else
				damage = matk2;
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
				damage = (damage * (100 - mdef1)) / 100 - mdef2;
			}
		}

		if(damage < 1)
			damage = 1;
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

		if (sd && skill_num > 0 && sd->skillatk[0].id != 0) {
			for (i = 0; i < MAX_PC_BONUS && sd->skillatk[i].id != 0; i++) {
				if (sd->skillatk[i].id == skill_num) {
					damage += damage * sd->skillatk[i].val / 100;
					break;
				}
			}
		}
	}

	if (tsd && skill_num != HW_GRAVITATION) { //Gravitation ignores target's cards
		int s_class = status_get_class(bl);
		cardfix = 100;
		cardfix = cardfix * (100 - tsd->subele[ele]) / 100;	// ëÆ ê´Ç…ÇÊÇÈÉ_ÉÅÅ[ÉWëœê´
		cardfix = cardfix * (100 - tsd->subrace[race]) / 100;	// éÌë∞Ç…ÇÊÇÈÉ_ÉÅÅ[ÉWëœê´
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

	damage = battle_attr_fix(damage, ele, status_get_element(target)); // ëÆ ê´èCê≥

	if(skill_num == CR_GRANDCROSS) {	// ÉOÉâÉìÉhÉNÉçÉX
		struct Damage wd;
		wd=battle_calc_weapon_attack(bl,target,skill_num,skill_lv,flag);
		damage = (damage + wd.damage) * (100 + 40*skill_lv)/100;
		if(battle_config.gx_dupele) damage=battle_attr_fix(damage, ele, status_get_element(target) );	//ëÆê´2âÒÇ©Ç©ÇÈ
		if(bl==target)
			damage >>= 1;
	}

	div_ = skill_get_num(skill_num, skill_lv);

	if (div_ > 1 && skill_num != WZ_VERMILION)
		damage*=div_;

	if(t_mode & 0x40 && damage > 0)
		damage = 1;

	if(tsd && status_isimmune(target))
	{
		if(battle_config.gtb_pvp_only != 0)
		{
			if((map[target->m].flag.pvp || map[target->m].flag.gvg))
				damage = (damage * (100 - battle_config.gtb_pvp_only)) / 100;
		} else
			damage = 0;
	}

	if(skill_num != HW_GRAVITATION)
		damage = battle_calc_damage(bl, target, damage, div_, skill_num, skill_lv, aflag);

	if(tsd && tsd->magic_damage_return > 0 && tsd->magic_damage_return > rand()%100)
	{
		short calc_rdamage = 1;

		if(skill_num == AL_HEAL || skill_num == PR_SANCTUARY)
		{
			if(status_get_element(target) == 7)
			{
				if(skill_num == PR_SANCTUARY)
				{
					if(rand()%100 > 50)
					{
						rdamage += -damage;
						damage = 0;
					} else {
						battle_heal(NULL, target, 0, 0, 0);
						damage = 0;
						calc_rdamage = 0;
					}
				} else {
					rdamage += -damage;
					damage = 0;
				}
			} else {
				calc_rdamage = 0;
			}
		} else {
			rdamage += damage;
			damage = 0;
		}

		if(calc_rdamage == 1)
		{
			if(rdamage < 1)
				rdamage = 1;
			clif_damage(target, bl, gettick_cache, 0, 0, rdamage, 0, 0, 0);
			battle_damage(target, bl, rdamage, 0);
		}
	}

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
 * ÇªÇÃëºÉ_ÉÅÅ[ÉWåvéZ
 *------------------------------------------
 */
struct Damage  battle_calc_misc_attack(
	struct block_list *bl, struct block_list *target, int skill_num, int skill_lv, int flag)
{
	struct map_session_data *sd = NULL, *tsd = NULL;
	int int_ = status_get_int(bl);
	int dex = status_get_dex(bl);
	int skill, cardfix;
	int damage = 0, div_ = 1, blewcount = skill_get_blewcount(skill_num, skill_lv);
	struct Damage md;
	int damagefix = 1;

	int aflag = BF_MISC | BF_LONG | BF_SKILL;

	//returnëOÇÃèàóùÇ™Ç†ÇÈÇÃÇ≈èÓïÒèoóÕïîÇÃÇ›ïœçX
	if(bl == NULL || target == NULL) {
		memset(&md, 0, sizeof(md));
		return md;
	}

	if(target->type == BL_PC)
		tsd = (struct map_session_data *)target;
	else if(target->type == BL_PET) {
		memset(&md, 0, sizeof(md));
		return md;
	}

	if(bl->type == BL_PC && (sd=(struct map_session_data *)bl)) {
		sd->state.attack_type = BF_MISC;
		sd->state.arrow_atk = 0;
	}

	switch(skill_num) {
	case HT_LANDMINE:	// ÉâÉìÉhÉ}ÉCÉì
		damage = skill_lv * (dex+75) * (100 + int_) / 100;
		break;

	case HT_BLASTMINE:	// ÉuÉâÉXÉgÉ}ÉCÉì
		damage = skill_lv*(dex/2+50)*(100+int_)/100;
		break;

	case HT_CLAYMORETRAP:	// ÉNÉåÉCÉÇÉAÅ[ÉgÉâÉbÉv
		damage = skill_lv*(dex/2+75)*(100+int_)/100;
		break;

	case HT_BLITZBEAT:	// ÉuÉäÉbÉcÉrÅ[Ég
		if (sd == NULL || (skill = pc_checkskill(sd, HT_STEELCROW)) <= 0)
			skill = 0;
		damage = (dex / 10 + int_ / 2 + skill * 3 + 40) * 2;
		if (flag > 1)
			damage /= flag;
		if (status_get_mode(target) & 0x40)
			damage = 1;
		aflag = (aflag&~BF_RANGEMASK)|BF_LONG;
		break;

	case TF_THROWSTONE:	// êŒìäÇ∞
		damage = 50;
		damagefix = 0;
		if (status_get_mode(target) & 0x40)
			damage = 1;
		aflag = (aflag&~BF_RANGEMASK)|BF_LONG;
		break;

	case BA_DISSONANCE:	// ïsã¶òaâπ
		damage = 30 + (skill_lv) * 10 + pc_checkskill(sd, BA_MUSICALLESSON) * 3;
		if (status_get_mode(target) & 0x40)
			damage = 1;
		break;

	case NPC_SELFDESTRUCTION:	// é©îö
		damage=status_get_hp(bl)-(bl==target?1:0);
		damagefix=0;
		break;

	case NPC_SMOKING:	// É^ÉoÉRÇãzÇ§
		damage=3;
		damagefix=0;
		break;

	case NPC_DARKBREATH:
		{
			struct status_change *sc_data = status_get_sc_data(target);
			int hitrate = status_get_hit(bl) - status_get_flee(target) + 80;
			hitrate = ((hitrate > 95) ? 95: ((hitrate < 5) ? 5 : hitrate));
			if(sc_data && (sc_data[SC_SLEEP].timer!=-1 || sc_data[SC_STUN].timer!=-1 ||
				sc_data[SC_FREEZE].timer!=-1 || (sc_data[SC_STONE].timer!=-1 && sc_data[SC_STONE].val2==0) ) )
				hitrate = 1000000;
			if(rand()%100 < hitrate) {
				damage = 500 + (skill_lv-1)*1000 + rand()%1000;
				if(damage > 9999) damage = 9999;
			}
		}
		break;

	case SN_FALCONASSAULT: /* ÉtÉ@ÉãÉRÉìÉAÉTÉãÉg */
		if (sd == NULL || (skill = pc_checkskill(sd, HT_STEELCROW)) <= 0)
			skill = 0;
		//damage = ((150 + skill_lv * 70) * (dex / 10 + int_ / 2 + skill * 3 + 40) * 2) / 100; // [Yor] according desc
		damage = (15 + skill_lv * 7) * (dex / 10 + int_ / 2 + skill * 3 + 40) / 5; // [Yor] simplify
		if (flag > 1)
			damage /= flag;
		if (status_get_mode(target) & 0x40)
			damage = 1;
		aflag = (aflag & ~BF_RANGEMASK) | BF_LONG;
		break;
	
	case PA_PRESSURE: //Gloria Domini/Pressure is the only skill that can do full damage to plants
		damage = 500 + 300 * skill_lv;
		damagefix = 0;
		break;
	
	case PA_GOSPEL:
		damage = 1 + rand() % 9999;
		aflag = (aflag & ~BF_RANGEMASK) | BF_LONG;
		break;

	case CR_ACIDDEMONSTRATION:
		{
			int t_vit = status_get_vit(target);
			damage = 7 * (t_vit * int_ * int_) / ( 10 * (t_vit + int_));
			if (tsd) //If target is a player, damage is reduced by 50%
				damage = damage * 1 / 2;
			else if (status_get_mode(target) & 0x40) //Else if target is a plant type, damage is reduced to 1
				damage = 1;
			aflag = (aflag&~BF_RANGEMASK)|BF_LONG;
		}
		break;
	}

	if (damagefix) {
		int ele = skill_get_pl(skill_num);
		int race = status_get_race(bl);
		int size = status_get_size(bl);
		if (damage < 1 && skill_num != NPC_DARKBREATH)
			damage = 1;

		if (tsd) {
			cardfix = 100;
			cardfix = cardfix * (100 - tsd->subele[ele]) / 100; // ëÆê´Ç…ÇÊÇÈÉ_ÉÅÅ[ÉWëœê´
			cardfix = cardfix * (100 - tsd->subrace[race]) / 100; // éÌë∞Ç…ÇÊÇÈÉ_ÉÅÅ[ÉWëœê´
			cardfix = cardfix * (100 - tsd->subsize[size]) / 100;
			cardfix = cardfix * (100 - tsd->misc_def_rate) / 100;
			damage = damage * cardfix / 100;
		}
		if (sd && sd->skillatk[0].id != 0) {
			//Recycled "int_" variable since its useless by now.
			for (int_ = 0; int_ < MAX_PC_BONUS && sd->skillatk[int_].id != 0; int_++) {
				if (sd->skillatk[int_].id == skill_num) {
					damage += damage * sd->skillatk[int_].val / 100;
					break;
				}
			}
		}
		if (damage < 0) damage = 0;
		damage = battle_attr_fix(damage, ele, status_get_element(target)); // ëÆê´èCê≥
	}

	div_ = skill_get_num(skill_num, skill_lv);
	if (div_ > 1)
		damage *= div_;

	if (damage > 0 && (damage < div_ || (status_get_def(target) >= 1000000 && status_get_mdef(target) >= 1000000)))
		damage = div_;
	
	if (skill_num != PA_PRESSURE) //The following does NOT affect Gloria Domini/Pressure at all
		damage = battle_calc_damage(bl, target, damage, div_, skill_num, skill_lv, aflag);	// ç≈èIèCê≥

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
 * É_ÉÅÅ[ÉWåvéZàÍäáèàóùóp
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
 * í èÌçUåÇèàóùÇ‹Ç∆Çﬂ
 *------------------------------------------
 */
int battle_weapon_attack(struct block_list *src, struct block_list *target, unsigned int tick, int flag) {
	struct map_session_data *sd = NULL;
	struct mob_data *md = NULL;
	struct status_change *sc_data, *t_sc_data;
	short *opt1;
	int race = 7, ele = 0;
	int damage, rdamage = 0;
	struct Damage wd;

	nullpo_retr(0, src);
	nullpo_retr(0, target);

	sc_data = status_get_sc_data(src);
	t_sc_data = status_get_sc_data(target);

	if (src->type == BL_PC)
		sd = (struct map_session_data *)src;
	else if (src->type == BL_MOB)
		md = (struct mob_data *)src;

	if (src->prev == NULL || target->prev == NULL)
		return 0;
	if (sd && pc_isdead(sd))
		return 0;
	if (target->type == BL_PC && pc_isdead((struct map_session_data *)target))
		return 0;

	opt1 = status_get_opt1(src);
	if (opt1 && *opt1 > 0) {
		battle_stopattack(src);
		return 0;
	}

	if (sc_data && sc_data[SC_BLADESTOP].timer != -1) {
		battle_stopattack(src);
		return 0;
	}
	
	if (battle_check_target(src, target, BCT_ENEMY) <= 0) {
		if(!battle_check_range(src, target, 0))
			return 0; // çUåÇëŒè€äO
	}
	else if(battle_check_range(src, target, 0)) {
		race = status_get_race(target);
		ele = status_get_elem_type(target);
		// çUåÇëŒè€Ç∆Ç»ÇËÇ§ÇÈÇÃÇ≈çUåÇ
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
			else if(md && battle_config.monster_attack_direction_change)
				md->dir = map_calc_dir(src, target->x,target->y);
			wd = battle_calc_weapon_attack(src, target, KN_AUTOCOUNTER, flag&0xff, 0);
		}
		else if(flag&AS_POISONREACT && sc_data && sc_data[SC_POISONREACT].timer != -1) {
			wd = battle_calc_weapon_attack(src,target,AS_POISONREACT,sc_data[SC_POISONREACT].val1,0);
		}
		else if(sd && (rdamage = pc_checkskill(sd, MO_TRIPLEATTACK)) > 0 && sd->status.weapon <= 16 && rand()%100 < (30 - rdamage)) // Recycling rdamage to store skill lvl [Proximus]
			return skill_attack(BF_WEAPON, src, src, target, MO_TRIPLEATTACK, rdamage, tick, 0);
		else if (sc_data && sc_data[SC_SACRIFICE].timer != -1)
			return skill_attack(BF_WEAPON, src, src, target, PA_SACRIFICE, sc_data[SC_SACRIFICE].val1, tick, 0);
		else
			wd = battle_calc_weapon_attack(src, target, 0, 0, 0);

		if((damage = wd.damage + wd.damage2) > 0 && src != target) {
			rdamage = 0;
			if (wd.flag&BF_SHORT) {
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
			else { //It has to be BF_LONG (no other choice)
				if (target->type == BL_PC) {
					struct map_session_data *tsd = (struct map_session_data *)target;
					if (tsd && tsd->long_weapon_damage_return > 0) {
						rdamage += damage * tsd->long_weapon_damage_return / 100;
						if (rdamage < 1) rdamage = 1;
					}
				}
			}
			if(rdamage > 0)
				clif_damage(src, src, tick, wd.amotion, 0, rdamage, 1, 4, 0);
		}

		//printf("battle_weapon_attack: wd.amotion %d, wd.dmotion %d, gettick %d, tick %d\n", wd.amotion, wd.dmotion, gettick_cache, tick);
		clif_damage(src, target, tick, wd.amotion, wd.dmotion, wd.damage, wd.div_, wd.type, wd.damage2);
		//ìÒìÅó¨ç∂éËÇ∆ÉJÉ^Å[Éãí«åÇÇÃÉ~ÉXï\é¶(ñ≥óùÇ‚ÇËÅ`)

		if(sd)  {
			if(sd->status.weapon >= 16 && wd.damage2 == 0)
				clif_damage(src, target, tick + 10, wd.amotion, wd.dmotion, 0, 1, 0, 0);		
			
			if(sd->splash_range > 0 && (wd.damage > 0 || wd.damage2 > 0) )
				skill_castend_damage_id(src, target, 0, -1, tick, 0);
		}

		map_freeblock_lock();

		battle_delay_damage(tick + wd.amotion, src, target, BF_WEAPON, 0, 0, (wd.damage + wd.damage2), wd.dmg_lv, 0);

		if(target->prev != NULL &&
			(target->type != BL_PC || (target->type == BL_PC && !pc_isdead((struct map_session_data *)target)))) {
			if(sd && (wd.damage > 0 || wd.damage2 > 0)) {
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
						case 1:/* éxâáån */
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
					case 1:/* éxâáån */
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
			if (wd.flag&BF_WEAPON && src != target && (wd.damage > 0 || wd.damage2 > 0)) {
				int hp = 0,sp = 0;
				if (!battle_config.left_cardfix_to_right) { // ìÒìÅó¨ç∂éËÉJÅ[ÉhÇÃãzé˚ånå¯â ÇâEéËÇ…í«â¡ÇµÇ»Ç¢èÍçá
					hp += battle_calc_drain(wd.damage, sd->hp_drain_rate, sd->hp_drain_per, sd->hp_drain_value);
					hp += battle_calc_drain(wd.damage2, sd->hp_drain_rate_, sd->hp_drain_per_, sd->hp_drain_value_);
					sp += battle_calc_drain(wd.damage, sd->sp_drain_rate, sd->sp_drain_per, sd->sp_drain_value);
					sp += battle_calc_drain(wd.damage2, sd->sp_drain_rate_, sd->sp_drain_per_, sd->sp_drain_value_);
				} else { // ìÒìÅó¨ç∂éËÉJÅ[ÉhÇÃãzé˚ånå¯â ÇâEéËÇ…í«â¡Ç∑ÇÈèÍçá
					int hp_drain_rate = sd->hp_drain_rate + sd->hp_drain_rate_;
					int hp_drain_per = sd->hp_drain_per + sd->hp_drain_per_;
					int hp_drain_value = sd->hp_drain_value + sd->hp_drain_value_;
					int sp_drain_rate = sd->sp_drain_rate + sd->sp_drain_rate_;
					int sp_drain_per = sd->sp_drain_per + sd->sp_drain_per_;
					int sp_drain_value = sd->sp_drain_value + sd->sp_drain_value_;
					hp += battle_calc_drain(wd.damage, hp_drain_rate, hp_drain_per, hp_drain_value);
					sp += battle_calc_drain(wd.damage, sp_drain_rate, sp_drain_per, sp_drain_value);
				}
				if (hp || sp)
					pc_heal(sd, hp, sp);
				if(target->type == BL_PC) {
					if (sd->sp_drain_type)
						battle_heal(NULL, target, 0, -sp, 0);
					if (sd->sp_vanish_per && rand()%100 < sd->sp_vanish_rate) {
						sp = ((struct map_session_data *)target)->status.sp * sd->sp_vanish_per / 100;
						if (sp > 0)
							battle_heal(NULL, target, 0, -sp, 0);
					}
				}
			}
		}
		/* Future implementation of Autocast from alchemist plants
		else {
			struct mob_data *md = NULL;
			if (src->type == BL_MOB && (md = (struct mob_data *)src)) {
				struct block_list *bl;
				if (md->state.special_mob_ai && md->master_id && (bl = map_id2bl(md->master_id)) != NULL) {
					if(bl->type == BL_PC) {
						struct map_session_data *msd = (struct map_session_data *)bl;
						int n;
						for (n = 0; n < MAX_PC_BONUS; n++) {
							if (msd->autospell[n].id == 0)
								break;
							else if (rand() % 10000 < msd->autospell[n].rate) {
								int skilllv = msd->autospell[n].lv, i;
								if ((i = skill_get_inf(msd->autospell[n].id) == 2) || i == 32)	// area or trap
									skill_castend_pos2(bl, target->x, target->y, msd->autospell[n].id, skilllv, tick, flag);
								else {			// target						
									switch(skill_get_nk(msd->autospell[n].id))
									{
									case 0:	// normal skill
									case 1:	// no damage skill
										if (msd->autospell[n].type) {	// target
											if((msd->autospell[n].id==AL_HEAL || (msd->autospell[n].id==ALL_RESURRECTION && target->type != BL_PC)) &&
												!battle_check_undead(race,ele))
													break;
											 skill_castend_damage_id(bl, target, msd->autospell[n].id, skilllv, tick, flag);
										}
										else { // self
											if (msd->autospell[n].id == AL_HEAL)
												skill_castend_nodamage_id(bl, bl, msd->autospell[n].id, skilllv, tick, flag);
											else
												skill_castend_nodamage_id(bl, target, msd->autospell[n].id, skilllv, tick, flag);
										}
										break;
									case 2:	// splash damage skill
										skill_castend_damage_id(bl, target, msd->autospell[n].id, skilllv, tick, flag);
										break;
									}
								}
							}
						}
					}
				}
			}
		}
		*/
		
		/* Autospell from TARGET */
		if(target->type == BL_PC && (wd.dmg_lv == ATK_DEF || wd.damage > 0 || wd.damage2 > 0)) {
			struct map_session_data *tsd = (struct map_session_data *)target;
			int n;
			for (n = 0; n < MAX_PC_BONUS; n++) {
				if (tsd->autospell2[n].id == 0)
					break;
				else if (rand() % 10000 < tsd->autospell2[n].rate) {
					int skilllv = tsd->autospell2[n].lv, i;
					struct block_list *tbl;
					if (tsd->autospell2[n].type == 0)
						tbl = target;
					else
						tbl = src;
					if ((i = skill_get_inf(tsd->autospell2[n].id) == 2) || i == 32)	// area or trap
						skill_castend_pos2(target, tbl->x, tbl->y, tsd->autospell2[n].id, skilllv, tick, flag);
					else {
						switch(skill_get_nk(tsd->autospell2[n].id)) {
							case 0:	// normal skill
							case 1: // no damage skill
								if (tsd->autospell2[n].type) {	// target
									if ((tsd->autospell2[n].id == AL_HEAL || (tsd->autospell2[n].id == ALL_RESURRECTION && tbl->type != BL_PC)) &&
										!battle_check_undead(status_get_race(tbl), status_get_elem_type(tbl)))
											break;
									}
								skill_castend_nodamage_id(target, tbl, tsd->autospell2[n].id, skilllv, tick, flag);
								break;
							case 2:	// splash damage skill
								skill_castend_damage_id(target, tbl, tsd->autospell2[n].id, skilllv, tick, flag);
								break;
						}
					}
				}
			}
		}
		/* End autospell from TARGET */
		
		if (rdamage > 0)
			battle_delay_damage(tick + wd.amotion, target, src, BF_WEAPON, 0, 0, rdamage, 0, 0); // Reflected damages
		
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
			if (t_sc_data[SC_BLADESTOP_WAIT].timer != -1 && !(status_get_mode(src)&0x20)) { // É{ÉXÇ…ÇÕñ≥å¯
				int skilllv = t_sc_data[SC_BLADESTOP_WAIT].val1;
				int duration = skill_get_time2(MO_BLADESTOP, skilllv);
				status_change_end(target, SC_BLADESTOP_WAIT, -1);
				clif_damage(src, target, tick, status_get_amotion(src), 1, 0, 1, 0, 0); //Display MISS.
				status_change_start(target, SC_BLADESTOP, skilllv, 2, (int)target, (int)src, duration, 0);
				skilllv = sd? pc_checkskill(sd, MO_BLADESTOP):1;
				status_change_start(src, SC_BLADESTOP, skilllv, 1, (int)src, (int)target, duration, 0);
			}
			if(t_sc_data[SC_SPLASHER].timer!=-1)	//â£Ç¡ÇΩÇÃÇ≈ëŒè€ÇÃÉxÉiÉÄÉXÉvÉâÉbÉVÉÉÅ[èÛë‘Çâèú
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
 * Checks the state between two targets
 * (enemy, friend, party, guild, etc)
 * See battle.h for possible values/combinations
 * to be used here (BCT_* constants)
 * Return value is:
 * 1: flag holds true (is enemy, party, etc)
 * -1: flag fails
 * 0: Invalid target (non-targetable ever)
 *------------------------------------------
 */
int battle_check_target(struct block_list *src, struct block_list *target, int flag)
{
	int state = 0; //Initial state none
	unsigned short strip_enemy = 1; //Flag which marks whether to remove the BCT_ENEMY status if it's also friend/ally.
	struct block_list *s_bl = src, *t_bl = target;
	struct status_change *sc_data; //source sc_data
	struct status_change *tsc_data; //target sc_data

	nullpo_retr(0, src);
	nullpo_retr(0, target);

	if (flag&BCT_ENEMY && !map[src->m].flag.gvg && !(status_get_mode(src)&0x20)) {
		sc_data = status_get_sc_data(src); //source sc_data
		tsc_data = status_get_sc_data(target); //target sc_data
		if ((sc_data && sc_data[SC_BASILICA].timer != -1) || (tsc_data && tsc_data[SC_BASILICA].timer != -1))
			return -1;
	}
	
	if (target->type == BL_SKILL) {//Needed out of the switch in case the ownership needs to be passed skill->mob->master
		struct skill_unit *su = (struct skill_unit *)target;
		if (!su->group)
			return 0;
		// Added WZ_QUAGMIRE to be ignored by the skills that can target traps, since arrow shower could hit Quagmire, and cause it to act funny. [Bison]
		if (skill_get_inf2(su->group->skill_id)&0x80 && su->group->skill_id != WZ_QUAGMIRE) {	//Only a few skills can target traps...
			switch (battle_getcurrentskill(src)) {
				case HT_REMOVETRAP:
				case AC_SHOWER:
				case WZ_HEAVENDRIVE:
					state |= BCT_ENEMY;
					strip_enemy = 0;
					break;
				default:
					return 0;
			}
		} else if (su->group->skill_id == WZ_ICEWALL)	{	//Icewall can be hit by anything except skills.
			/*if (src->type == BL_SKILL)
				return 0;*/ //doesnt works
			state |= BCT_ENEMY;
			strip_enemy = 0;
		} else	//Excepting traps and icewall, you should not be able to target skills.
			return 0;
		if ((t_bl = map_id2bl(su->group->src_id)) == NULL)
			t_bl = target; //Fallback on the trap itself, otherwise consider this a "versus caster" scenario.
	}

	switch (t_bl->type) { //target
		case BL_PC: //target is a PLAYER
		{
			struct map_session_data *sd = (struct map_session_data *)t_bl;
			//if player is invisible or have a invincible timer on (eg. when teleported)
			if (sd->invincible_timer != -1 || pc_isinvisible(sd))
				return -1; //Cannot be targeted yet.
			if (sd->special_state.killable) {
				state |= BCT_ENEMY; //For @charkillable command
				strip_enemy = 0;
			}
			break;
		}
		case BL_MOB: //target is a MONSTER
		{
			struct mob_data *md = (struct mob_data *)t_bl;
			if (!agit_flag && md->guild_id)
				return 0; //Disable guardians/emperiums owned by Guilds on non-woe times. (Assuming EMP is not spawned in non WoE Times)
			if (md->state.special_mob_ai == 2){	//Mines are sort of universal enemies.
				state |= BCT_ENEMY;
				strip_enemy = 0;
			}
			if (md->master_id && (t_bl = map_id2bl(md->master_id)) == NULL)
				t_bl = &md->bl; //Fallback on the mob itself, otherwise consider this a "versus master" scenario.
			break;
		}
		case BL_PET: //target is a PET
			return 0; //Pets cannot be targetted.
		case BL_SKILL: //target is an SKILL
			break;
		default:	//Invalid target
			return 0;
	}

	if(src->prev == NULL)
		return -1;
		
	if (src->type == BL_SKILL) { //source
		struct skill_unit *su = (struct skill_unit *)src;
		if (!su->group)
			return 0;

		if (su->group->src_id == target->id)
		{
			int inf2;
			inf2 = skill_get_inf2(su->group->skill_id);
			if (inf2&0x200)
				return -1;
			if (inf2&0x100)
				return 1;
		}
		if ((s_bl = map_id2bl(su->group->src_id)) == NULL)
			s_bl = src; //Fallback on the trap itself, otherwise consider this a "caster versus enemy" scenario.
	}

	switch (s_bl->type)	{ //source bl
		case BL_PC: //source is a PLAYER
		{
			struct map_session_data *sd = (struct map_session_data *) s_bl;
			if(pc_isdead(sd)) return -1;
			if (sd->special_state.killer) {
				state |= BCT_ENEMY; //For @charkillable command
				strip_enemy = 0;
			}
			if (map[target->m].flag.gvg && !sd->status.guild_id &&
				t_bl->type == BL_MOB && ((struct mob_data *)t_bl)->guild_id)
				return 0; //If you don't belong to a guild, can't target guardians/emperium. (Will bypass if castle has no owner)
			if (t_bl->type != BL_PC)
				state |= BCT_ENEMY; //Natural enemy.
			break;
		}
		case BL_MOB: //source is a MONSTER
		{
			struct mob_data *md = (struct mob_data *)s_bl;
			if (!agit_flag && md->guild_id)
				return 0; //Disable guardians/emperium owned by Guilds on non-woe times. (Assuming EMP is not spawned in non WoE Times)
			if (!md->state.special_mob_ai) { //Normal mobs.
				if (t_bl->type == BL_MOB && !((struct mob_data*)t_bl)->state.special_mob_ai)
					state |= BCT_PARTY; //Normal mobs with no ai are friends.
				else
					state |= BCT_ENEMY; //However, all else are enemies.
			} else if (t_bl->type != BL_PC)
				state |= BCT_ENEMY; //Natural enemy for AI mobs are nonplayers.
			if (md->master_id && (s_bl = map_id2bl(md->master_id)) == NULL)
				s_bl = &md->bl; //Fallback on the mob itself, otherwise consider this a "from master" scenario.
			break;
		}
		case BL_PET: //source is a PET
		{
			struct pet_data *pd = (struct pet_data *)s_bl;
			if (t_bl->type != BL_MOB && flag&BCT_ENEMY)
				return 0; //Pet may not attack non-mobs/items.
			if (t_bl->type == BL_MOB && ((struct mob_data *)t_bl)->guild_id && flag&BCT_ENEMY)
				return 0; //pet may not attack Guardians/Emperium. (Will bypass if castle has no owner)
			if (t_bl->type != BL_PC)
				state |= BCT_ENEMY; //Stock enemy type.
			if (pd->msd)
				s_bl = &pd->msd->bl; //"My master's enemies are my enemies..."
			break;
		}
		case BL_SKILL: //Skill with no owner? Fishy, but let it through.
			break;
		default:	//Invalid source of attack?
			return 0;
	}
	
	if((flag&BCT_ALL) == BCT_ALL) { //All actually stands for all players/mobs
		if (target->type == BL_MOB || target->type == BL_PC)
			return 1;
		else
			return -1;
	}
	
	if(t_bl == s_bl)
	{	//No need for further testing.
		state |= BCT_SELF|BCT_PARTY|BCT_GUILD;
		if (state & BCT_ENEMY && strip_enemy)
			state&=~BCT_ENEMY;
		return (flag&state)?1:-1;
	}
	
	if(map[target->m].flag.pvp || map[target->m].flag.gvg_dungeon || map[target->m].flag.gvg) {
		if (flag&(BCT_PARTY|BCT_ENEMY)) {
			int s_party = status_get_party_id(s_bl); //gets the source's party id, to compare later with target's id
			if(
				((map[target->m].flag.pvp && map[target->m].flag.pvp_noparty) || //pvp and pvp_noparty mf OFF, cant hit party members in pvp
				(map[target->m].flag.gvg && !map[target->m].flag.gvg_noparty) || //gvg and gvg_noparty mf OFF, cant hit party members in gvg
				(map[target->m].flag.gvg_dungeon && !map[target->m].flag.gvg_noparty)) &&
				s_party && s_party == status_get_party_id(t_bl) //check if the source is in the same party as the target
			  )
				state |= BCT_PARTY; //can not target party mate
			else
				state |= BCT_ENEMY; //can target party mate.. its an enemy
		}
		if (flag&(BCT_GUILD|BCT_ENEMY)) {
			int s_guild = status_get_guild_id(s_bl); //gets source's guild id
			int t_guild = status_get_guild_id(t_bl); //gets target's guild id
			if (
				((map[target->m].flag.pvp && map[target->m].flag.pvp_noguild) || //pvp and pvp_noguild mf OFF, cant hit guild memvers in pvp
				  map[target->m].flag.gvg || map[target->m].flag.gvg_dungeon) &&
				s_guild && t_guild && (s_guild == t_guild || guild_idisallied(s_guild, t_guild))
			)
				state |= BCT_GUILD; //can not target guildmates
			else
				state |= BCT_ENEMY; //can target guildmates.. its an enemy
		}
		if (state&BCT_ENEMY && battle_config.pk_mode && !map[target->m].flag.gvg && s_bl->type == BL_PC && t_bl->type == BL_PC)	{	
			//Prevent novice engagement on pk_mode
			struct map_session_data* sd;
			if ((sd = (struct map_session_data*)s_bl) &&
				(sd->status.class == 0 || sd->status.base_level < battle_config.pk_min_level))
				state&=~BCT_ENEMY;
			else
			if ((sd = (struct map_session_data*)t_bl) &&
				(sd->status.class == 0 || sd->status.base_level < battle_config.pk_min_level))
				state&=~BCT_ENEMY;
		}
	} else { //Non pvp/gvg, check party/guild settings.
		if (flag&BCT_PARTY || state&BCT_ENEMY) {
			int s_party = status_get_party_id(s_bl);
			if (s_party && s_party == status_get_party_id(t_bl))
				state |= BCT_PARTY;
		}
		if (flag & BCT_GUILD || state & BCT_ENEMY) {
			int s_guild = status_get_guild_id(s_bl);
			int t_guild = status_get_guild_id(t_bl);
			if (s_guild && t_guild && (s_guild == t_guild || guild_idisallied(s_guild, t_guild)))
				state |= BCT_GUILD;
		}
	}
	
	if (!state) //If not an enemy, nor a guild, nor party, nor yourself, it's neutral.
		state = BCT_NEUTRAL;
	//Alliance state takes precedence over enemy one.
	else if (state&BCT_ENEMY && strip_enemy && state&(BCT_SELF|BCT_PARTY|BCT_GUILD))
		state&=~BCT_ENEMY;

	return (flag&state)?1:-1;
}

/*==========================================
 * éÀíˆîªíË
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

	if (src->m != bl->m) // à·Ç§É}ÉbÉv
		return 0;

	if (range > 0 && range < arange) // âìÇ∑Ç¨ÇÈ
		return 0;

	if (arange < 2) // ìØÇ∂É}ÉXÇ©ó◊ê⁄
		return 1;

//	if(bl->type == BL_SKILL && ((struct skill_unit *)bl)->group->unit_id == 0x8d)
//		return 1;

// restored, because path_search_long doesn't check correctly wall -------
/*	// è·äQï®îªíË
	wpd.path_len = 0;
	wpd.path_pos = 0;
	wpd.path_half = 0;
	if (path_search(&wpd, src->m, src->x, src->y, bl->x, bl->y, 0x10001) != -1)
		return 1;

	dx = (dx > 0) ? 1 : ((dx < 0) ? -1 : 0);
	dy = (dy > 0) ? 1 : ((dy < 0) ? -1 : 0);

	return (path_search(&wpd, src->m, src->x + dx, src->y + dy, bl->x - dx, bl->y - dy, 0x10001) != -1) ? 1 : 0;*/
//-------------------------------------------------------------------------

	// è·äQï®îªíË
	return path_search_long(src->m, src->x, src->y, bl->x, bl->y); // doesn't check correctly wall
}

static const struct battle_config_short{
	char str[128];
	unsigned short *val;
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
	{ "drop_rate0item",                             &battle_config.drop_rate0item },
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
	{ "zeny_penalty_percent",                       &battle_config.zeny_penalty_percent },
	{ "zeny_penalty_by_lvl",                        &battle_config.zeny_penalty_by_lvl },
	{ "restart_hp_rate",                            &battle_config.restart_hp_rate },
	{ "restart_sp_rate",                            &battle_config.restart_sp_rate },
	{ "mvp_hp_rate",                                &battle_config.mvp_hp_rate },
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
	{ "natural_heal_weight_rate",                   &battle_config.natural_heal_weight_rate },
	{ "item_name_override_grffile",                 &battle_config.item_name_override_grffile},
	{ "item_equip_override_grffile",                &battle_config.item_equip_override_grffile},	// [Celest]
	{ "item_slots_override_grffile",                &battle_config.item_slots_override_grffile},	// [Celest]
	{ "indoors_override_grffile",                   &battle_config.indoors_override_grffile},	// [Celest]
	{ "skill_sp_override_grffile",                  &battle_config.skill_sp_override_grffile},	// [Celest]
	{ "cardillust_read_grffile",                    &battle_config.cardillust_read_grffile},	// [Celest]
	{ "arrow_decrement",                            &battle_config.arrow_decrement },
	{ "max_aspd",                                   &battle_config.max_aspd },
	{ "max_lv",                                     &battle_config.max_lv },
	{ "max_parameter",                              &battle_config.max_parameter },
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
	{ "gvg_weapon_attack_damage_rate",              &battle_config.gvg_weapon_damage_rate },
	{ "gvg_magic_attack_damage_rate",               &battle_config.gvg_magic_damage_rate },
	{ "gvg_misc_attack_damage_rate",                &battle_config.gvg_misc_damage_rate },
	{ "gvg_flee_penalty",                           &battle_config.gvg_flee_penalty			},
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
	{ "atcommand_max_player_gm_level",              &battle_config.atcommand_max_player_gm_level },
	{ "display_delay_skill_fail",                   &battle_config.display_delay_skill_fail },
	{ "display_snatcher_skill_fail",                &battle_config.display_snatcher_skill_fail	},
	{ "chat_warpportal",                            &battle_config.chat_warpportal },
	{ "mob_warpportal",                             &battle_config.mob_warpportal },
	{ "dead_branch_active",                         &battle_config.dead_branch_active },
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
	{ "display_hpmeter",                            &battle_config.display_hpmeter },
	{ "bone_drop",                                  &battle_config.bone_drop },

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
	{ "show_mob_hp",                                &battle_config.show_mob_hp }, // [Valaris]
	{ "ban_spoof_namer",                            &battle_config.ban_spoof_namer }, // added by [Yor]
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
	{ "zeny_from_mobs",                             &battle_config.zeny_from_mobs }, // [Valaris]
	{ "mobs_level_up",                              &battle_config.mobs_level_up }, // [Valaris]
	{ "pk_min_level",                               &battle_config.pk_min_level }, // [celest]
	{ "skill_steal_type",                           &battle_config.skill_steal_type }, // [celest]
	{ "skill_steal_rate",                           &battle_config.skill_steal_rate }, // [celest]
//	{ "night_darkness_level",                       &battle_config.night_darkness_level}, // [celest] // ************** This option is not used! : we don't know how to remove effect (night -> day)
	{ "skill_range_leniency",                       &battle_config.skill_range_leniency }, // [celest]
	{ "motd_type",                                  &battle_config.motd_type }, // [celest]
	{ "allow_atcommand_when_mute",                  &battle_config.allow_atcommand_when_mute }, // [celest]
	{ "manner_action",                              &battle_config.manner_action }, // [Yor]
	{ "finding_ore_rate",                           &battle_config.finding_ore_rate }, // [celest]
	{ "min_skill_delay_limit",                      &battle_config.min_skill_delay_limit }, // [celest]
	{ "idle_no_share",                              &battle_config.idle_no_share }, // [celest], for a feature by [MouseJstr]
	{ "chat_no_share",                              &battle_config.chat_no_share }, // [Yor]
	{ "npc_chat_no_share",                          &battle_config.npc_chat_no_share }, // [Yor]
	{ "shop_no_share",                              &battle_config.shop_no_share }, // [Yor]
	{ "trade_no_share",                             &battle_config.trade_no_share }, // [Yor]
	{ "idle_before_disconnect",                     &battle_config.idle_disconnect }, // [Yor]
	{ "idle_before_disconnect_chat",                &battle_config.idle_disconnect_chat }, // [Yor]
	{ "idle_before_disconnect_vender",              &battle_config.idle_disconnect_vender }, // [Yor]
	{ "idle_before_disconnect_disable_for_restore", &battle_config.idle_disconnect_disable_for_restore }, // [Yor]
	{ "idle_before_disconnect_ignore_GM",           &battle_config.idle_disconnect_ignore_GM}, // [Yor]
	{ "jail_message",                               &battle_config.jail_message }, // [Yor] Do we send message to ALL players when a player is put in jail?
	{ "jail_discharge_message",                     &battle_config.jail_discharge_message }, // [Yor] Do we send message to ALL players when a player is discharged?
	{ "mingmlvl_message",                           &battle_config.mingmlvl_message }, // [Yor] Which message do we send when a GM can use a command, but mingmlvl map flag block it?
	{ "check_invalid_slot",                         &battle_config.check_invalid_slot }, // [Yor] Do we check invalid slotted cards?
	{ "ruwach_range",                               &battle_config.ruwach_range }, // [Yor] Set the range (number of squares/tiles around you) of 'ruwach' skill to detect invisible.
	{ "sight_range",                                &battle_config.sight_range }, // [Yor] Set the range (number of squares/tiles around you) of 'sight' skill to detect invisible.
	{ "max_icewall",                                &battle_config.max_icewall }, // [Yor] Set maximum number of ice walls active at the same time.
	{ "ignore_items_gender",                        &battle_config.ignore_items_gender }, //[Proximus] Whether item_db gender restrictions will be ignored or not
	{ "party_invite_same_account",                  &battle_config.party_invite_same_account }, //[Proximus] Whether its possible to invite 2 characters from the same account into a party

	{ "atcommand_main_channel_at_start",            &battle_config.atcommand_main_channel_at_start },
	{ "atcommand_main_channel_on_gvg_map_woe",      &battle_config.atcommand_main_channel_on_gvg_map_woe},
	{ "atcommand_main_channel_when_woe",            &battle_config.atcommand_main_channel_when_woe},
	{ "atcommand_min_GM_level_for_request",         &battle_config.atcommand_min_GM_level_for_request },
	{ "atcommand_follow_stop_dead_target",          &battle_config.atcommand_follow_stop_dead_target },
	{ "atcommand_add_local_message_info",           &battle_config.atcommand_add_local_message_info },
	{ "atcommand_storage_on_pvp_map",               &battle_config.atcommand_storage_on_pvp_map },
	{ "atcommand_gstorage_on_pvp_map",              &battle_config.atcommand_gstorage_on_pvp_map },
	{ "pm_gm_not_ignored",                          &battle_config.pm_gm_not_ignored }, // GM minimum level to be not ignored in private message. [BeoWulf] (from freya's bug report)

	{ "char_disconnect_mode",                       &battle_config.char_disconnect_mode },

	{ "extra_system_flag",                          &battle_config.extra_system_flag },

//SQL-only options start
#ifdef USE_SQL
	{ "mail_system",                                &battle_config.mail_system }, // added by [Valaris]
//SQL-only options end
#endif /* USE_SQL */
};

static const struct battle_config_int {
	const char *str;
	int *val;
} battle_data_int[] = {	
	{ "item_first_get_time",                        &battle_config.item_first_get_time },
	{ "item_second_get_time",                       &battle_config.item_second_get_time },
	{ "item_third_get_time",                        &battle_config.item_third_get_time },
	{ "mvp_item_first_get_time",                    &battle_config.mvp_item_first_get_time },
	{ "mvp_item_second_get_time",                   &battle_config.mvp_item_second_get_time },
	{ "mvp_item_third_get_time",                    &battle_config.mvp_item_third_get_time },
	{ "base_exp_rate",                              &battle_config.base_exp_rate			},
	{ "job_exp_rate",                               &battle_config.job_exp_rate				},
	{ "zeny_penalty",                               &battle_config.zeny_penalty				},
	{ "mvp_exp_rate",                               &battle_config.mvp_exp_rate				},
	{ "mvp_item_rate",                              &battle_config.mvp_item_rate },
	{ "natural_healhp_interval",                    &battle_config.natural_healhp_interval	},
	{ "natural_healsp_interval",                    &battle_config.natural_healsp_interval	},
	{ "natural_heal_skill_interval",                &battle_config.natural_heal_skill_interval},
	{ "max_hp",                                     &battle_config.max_hp					},
	{ "max_sp",                                     &battle_config.max_sp					},
	{ "max_cart_weight",                            &battle_config.max_cart_weight			},
	{ "vending_max_value",                          &battle_config.vending_max_value		},
	{ "item_rate_common",                           &battle_config.item_rate_common },
	{ "item_rate_equip",                            &battle_config.item_rate_equip },
	{ "item_rate_card",                             &battle_config.item_rate_card },
	{ "item_rate_heal",                             &battle_config.item_rate_heal },
	{ "item_rate_use",                              &battle_config.item_rate_use },
	{ "day_duration",                               &battle_config.day_duration }, // added by [Yor]
	{ "night_duration",                             &battle_config.night_duration }, // added by [Yor]
	{ "check_maximum_skill_points",                 &battle_config.check_maximum_skill_points },
	{ "idle_delay_no_share",                        &battle_config.idle_delay_no_share}, // [Yor]
	{ "ban_hack_trade",                             &battle_config.ban_hack_trade }, // added by [Yor]
	{ "ban_bot",                                    &battle_config.ban_bot }, // added by [Yor]
	{ "atcommand_send_usage_type",                  &battle_config.atcommand_send_usage_type }, // added by [Yor]
	{ "atcommand_main_channel_type",                &battle_config.atcommand_main_channel_type }, // added by [Yor]
};

int battle_set_value(char *w1, char *w2) {
	int i;

	for(i = 0; i < sizeof(battle_data) / (sizeof(battle_data[0])); i++)
		if (strcasecmp(w1, battle_data[i].str) == 0) {
			*battle_data[i].val = config_switch(w2);
			return 1;
		}
	
	for(i = 0; i < sizeof(battle_data_int) / (sizeof(battle_data_int[0])); i++)
		if (strcasecmp(w1, battle_data_int[i].str) == 0) {
			*battle_data_int[i].val = config_switch(w2);
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
	battle_config.no_caption_cloaking = 1; // remove the caption "cloaking !!" when skill is casted
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
	battle_config.gvg_short_damage_rate = 100; //All melee non-skill attacks do 100% (normal) damage
	battle_config.gvg_long_damage_rate = 75; //All ranged non-skill attacks get -25% damage penalty
	battle_config.gvg_weapon_damage_rate = 60; //All weapon skills get -40% damage penalty in WoE
	battle_config.gvg_magic_damage_rate = 50; //All magic skills get -50% damage penalty in WoE
	battle_config.gvg_misc_damage_rate = 60; //Same as weapon skill reduction -40%
	battle_config.gvg_flee_penalty = 20; //There is also a -20% flee penalty in WoE maps (not guild dungeons, just the castles)
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
	battle_config.atcommand_max_player_gm_level = 10; // GM level 10 is maximum GM level of a 'normal' player
	battle_config.atcommand_send_usage_type = -1; // Send usage of GM commands like a chat message
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
	battle_config.display_hpmeter = 0; // min level to look hp of players
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
	battle_config.equipment_breaking = 1; // [Valaris]
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
	//battle_config.night_darkness_level = 0;
	battle_config.skill_range_leniency = 1;
	battle_config.motd_type = 1;
	battle_config.allow_atcommand_when_mute = 0;
	battle_config.manner_action = 3; // [Yor]
	battle_config.finding_ore_rate = 100;
	battle_config.min_skill_delay_limit = 150;
	battle_config.idle_no_share = 0; // [Yor]
	battle_config.idle_delay_no_share = 120000; // [Yor] (2 minutes)
	battle_config.chat_no_share = 0; // [Yor]
	battle_config.npc_chat_no_share = 1; // [Yor]
	battle_config.shop_no_share = 1; // [Yor]
	battle_config.trade_no_share = 1; // [Yor]
	battle_config.idle_disconnect = 0; // (5 minutes) [Yor]
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
	battle_config.ignore_items_gender = 1; //Wether item_db gender restrictions will be ignored or not [Proximus]
	battle_config.party_invite_same_account = 0; //Whether its possible to invite 2 characters of the same account into a party [Proximus]
	
	battle_config.castrate_dex_scale = 150;
	battle_config.area_size = 16;
	battle_config.atcommand_main_channel_at_start = 3;
	battle_config.atcommand_main_channel_type = -3; // Send main channel like a guild message
	battle_config.atcommand_main_channel_on_gvg_map_woe = 2; // only good players and above can use 'Main channel' on GvG maps during WoE -> against abuse
	battle_config.atcommand_main_channel_when_woe = 0; // all players can use the @main command
	battle_config.atcommand_min_GM_level_for_request = 20;
	battle_config.atcommand_follow_stop_dead_target = 0; // no
	battle_config.atcommand_add_local_message_info = 1; // yes
	battle_config.atcommand_storage_on_pvp_map = 1; // yes
	battle_config.atcommand_gstorage_on_pvp_map = 1; // yes

	battle_config.pm_gm_not_ignored = 60; // GM minimum level to be not ignored in private message. [BeoWulf] (from freya's bug report)

	battle_config.char_disconnect_mode = 2; // nobody is disconnected

	battle_config.extra_system_flag = 1;

//SQL-only options start
#ifdef USE_SQL
	battle_config.mail_system = 0;
//SQL-only options end
#endif /* USE_SQL */
}

void battle_validate_conf() {
	if (battle_config.flooritem_lifetime < 1000)
		battle_config.flooritem_lifetime = LIFETIME_FLOORITEM * 1000;

	if (battle_config.item_auto_get < 1) // drop rate of items concerned by the autoloot (from 0% to x%).
		battle_config.item_auto_get = 0;
	else if (battle_config.item_auto_get > 10000)
		battle_config.item_auto_get = 10000;
	if (battle_config.item_auto_get_distance < 1)
		battle_config.item_auto_get_distance = 0;

	if (battle_config.base_exp_rate < 1)
		battle_config.base_exp_rate = 1;
	else if (battle_config.base_exp_rate > 1000000000)
		battle_config.base_exp_rate = 1000000000;
	if (battle_config.job_exp_rate < 1)
		battle_config.job_exp_rate = 1;
	else if (battle_config.job_exp_rate > 1000000000)
		battle_config.job_exp_rate = 1000000000;

	if (battle_config.zeny_penalty_percent < 1)
		battle_config.zeny_penalty_percent = 0;

	if (battle_config.restart_hp_rate < 1)
		battle_config.restart_hp_rate = 0;
	else if (battle_config.restart_hp_rate > 100)
		battle_config.restart_hp_rate = 100;
	if (battle_config.restart_sp_rate < 1)
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
	if (battle_config.atc_spawn_quantity_limit < 1)
		battle_config.atc_spawn_quantity_limit = 0;
	if (battle_config.atc_map_mob_limit < 1)
		battle_config.atc_map_mob_limit = 0;
	battle_config.max_aspd = 2000 - battle_config.max_aspd * 10;
	if (battle_config.max_aspd < 10)
		battle_config.max_aspd = 10;
	if (battle_config.max_aspd > 1000)
		battle_config.max_aspd = 1000;
	if(battle_config.hp_rate < 1)
		battle_config.hp_rate = 1;
	if(battle_config.sp_rate < 1)
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

	if (battle_config.atc_local_spawn_interval < 1)
		battle_config.atc_local_spawn_interval = 0;

	if (battle_config.guild_exp_limit > 99)
		battle_config.guild_exp_limit = 99;
	if (battle_config.guild_exp_limit < 1)
		battle_config.guild_exp_limit = 0;
//	if (battle_config.pet_weight < 0)
//		battle_config.pet_weight = 0;

	if (battle_config.hide_GM_session < 1)
		battle_config.hide_GM_session = 0;
	else if (battle_config.hide_GM_session > 100)
		battle_config.hide_GM_session = 100;

	if (battle_config.castle_defense_rate < 1)
		battle_config.castle_defense_rate = 0;
	else if (battle_config.castle_defense_rate > 100)
		battle_config.castle_defense_rate = 100;

	if (battle_config.item_rate_common < 0)
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

	if (battle_config.night_at_start < 1) // added by [Yor]
		battle_config.night_at_start = 0;
	else if (battle_config.night_at_start > 1) // added by [Yor]
		battle_config.night_at_start = 1;
	if (battle_config.day_duration < 0) // added by [Yor]
		battle_config.day_duration = 0;
	if (battle_config.night_duration < 0) // added by [Yor]
		battle_config.night_duration = 0;

	if (battle_config.check_ban_bot < 1) // added by [Yor]
		battle_config.check_ban_bot = 0;
	else if (battle_config.check_ban_bot > 100)
		battle_config.check_ban_bot = 100;

	if (battle_config.max_message_length < 80) // added by [Yor]
		battle_config.max_message_length = 80; // maximum that a normal client can send is: 70, but we give security -> 80
	if (battle_config.max_global_message_length < 130) // added by [Yor]
		battle_config.max_global_message_length = 130; // maximum that a normal iRO client can send is: 130

	if (battle_config.hack_info_GM_level < 1) // added by [Yor]
		battle_config.hack_info_GM_level = 0;
	else if (battle_config.hack_info_GM_level > 100)
		battle_config.hack_info_GM_level = 100;

	if (battle_config.speed_hack_info_GM_level < 1) // added by [Yor]
		battle_config.speed_hack_info_GM_level = 0;
	else if (battle_config.speed_hack_info_GM_level > 100)
		battle_config.speed_hack_info_GM_level = 100;

	if (battle_config.any_warp_GM_min_level < 1) // added by [Yor]
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
	/* New blue night effect, battle_config option not needed
	if (battle_config.night_darkness_level > 10) // Celest
		battle_config.night_darkness_level = 10; */

	if (battle_config.finding_ore_rate < 1)
		battle_config.finding_ore_rate = 0;
	else if (battle_config.finding_ore_rate > 10000)
		battle_config.finding_ore_rate = 10000;

	if (battle_config.skill_range_leniency < 1) // Celest
		battle_config.skill_range_leniency = 0;

	if (battle_config.motd_type < 1)
		battle_config.motd_type = 0;
	else if (battle_config.motd_type > 1)
		battle_config.motd_type = 1;

	if (battle_config.manner_action < 1) // added by [Yor]
		battle_config.manner_action = 0;
	else if (battle_config.manner_action > 3)
		battle_config.manner_action = 3;

	if (battle_config.atcommand_item_creation_name_input < 1)
		battle_config.atcommand_item_creation_name_input = 0;

	if (battle_config.atcommand_max_player_gm_level < 1)
		battle_config.atcommand_max_player_gm_level = 0;
	else if (battle_config.atcommand_max_player_gm_level > 100)
		battle_config.atcommand_max_player_gm_level = 100;
	if (battle_config.atcommand_send_usage_type < -5) // Send usage of GM commands like a chat message
		battle_config.atcommand_send_usage_type = -1; // set default
	else if (battle_config.atcommand_send_usage_type > 16777215) // 0xFFFFFF
		battle_config.atcommand_send_usage_type = -1; // set default

	if (battle_config.vending_max_value > 2000000000 || battle_config.vending_max_value <= 0)
		battle_config.vending_max_value = 2000000000;

	if (battle_config.min_skill_delay_limit < 1)
		battle_config.min_skill_delay_limit = 0; // minimum delay of 10ms (timers have minimum of 50 ms)

	if (battle_config.idle_no_share < 1)
		battle_config.idle_no_share = 0;
	else if (battle_config.idle_no_share > 2)
		battle_config.idle_no_share = 2;
	if (battle_config.idle_delay_no_share < 15000)
		battle_config.idle_delay_no_share = 15000;
	if (battle_config.chat_no_share < 1)
		battle_config.chat_no_share = 0;
	else if (battle_config.chat_no_share > 2)
		battle_config.chat_no_share = 2;
	if (battle_config.npc_chat_no_share < 1)
		battle_config.npc_chat_no_share = 0;
	else if (battle_config.npc_chat_no_share > 2)
		battle_config.npc_chat_no_share = 2;
	if (battle_config.shop_no_share < 1)
		battle_config.shop_no_share = 0;
	else if (battle_config.shop_no_share > 2)
		battle_config.shop_no_share = 2;
	if (battle_config.trade_no_share < 1)
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
	if (battle_config.idle_disconnect_ignore_GM < 1)
		battle_config.idle_disconnect_ignore_GM = 0;
	else if (battle_config.idle_disconnect_ignore_GM > 100)
		battle_config.idle_disconnect_ignore_GM = 100;
	//printf("idle disconnection: normal %d, chat %d, vender %d, disable for restore: %s, ignore GM :d\n", battle_config.idle_disconnect, battle_config.idle_disconnect_chat, battle_config.idle_disconnect_vender, (battle_config.idle_disconnect_disable_for_restore) ? "yes" : "no", battle_config.idle_disconnect_ignore_GM);

	if (battle_config.jail_discharge_message < 1)
		battle_config.jail_discharge_message = 0;
	else if (battle_config.jail_discharge_message > 3)
		battle_config.jail_discharge_message = 3;

	if (battle_config.mingmlvl_message < 1)
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

	if (battle_config.max_icewall < 1) // [Yor] Set maximum number of ice walls active at the same time.
		battle_config.max_icewall = 0; // no limit

	if (battle_config.atcommand_main_channel_at_start < 1)
		battle_config.atcommand_main_channel_at_start = 0;
	else if (battle_config.atcommand_main_channel_at_start > 3)
		battle_config.atcommand_main_channel_at_start = 3;
	if (battle_config.atcommand_main_channel_type < -5) // how to send main channel
		battle_config.atcommand_main_channel_type = -3; // set default
	else if (battle_config.atcommand_main_channel_type > 16777215) // 0xFFFFFF
		battle_config.atcommand_main_channel_type = -3; // set default
	if (battle_config.atcommand_main_channel_on_gvg_map_woe < 1)
		battle_config.atcommand_main_channel_on_gvg_map_woe = 0;
	else if (battle_config.atcommand_main_channel_on_gvg_map_woe > 100)
		battle_config.atcommand_main_channel_on_gvg_map_woe = 100;
	if (battle_config.atcommand_main_channel_when_woe < 1)
		battle_config.atcommand_main_channel_when_woe = 0;
	else if (battle_config.atcommand_main_channel_when_woe > 100)
		battle_config.atcommand_main_channel_when_woe = 100;
	if (battle_config.atcommand_min_GM_level_for_request < 1)
		battle_config.atcommand_min_GM_level_for_request = 0;
	else if (battle_config.atcommand_min_GM_level_for_request > 100)
		battle_config.atcommand_min_GM_level_for_request = 100;

	if (battle_config.atcommand_follow_stop_dead_target != 0)
		battle_config.atcommand_follow_stop_dead_target = 1;

	if (battle_config.pm_gm_not_ignored < 1 || battle_config.pm_gm_not_ignored > 100) // GM minimum level to be not ignored in private message. [BeoWulf] (from freya's bug report)
		battle_config.pm_gm_not_ignored = 100;

	if (battle_config.char_disconnect_mode < 1)
		battle_config.char_disconnect_mode = 0;
	else if (battle_config.char_disconnect_mode > 2)
		battle_config.char_disconnect_mode = 2;

	return;
}

/*==========================================
 * ê›íËÉtÉ@ÉCÉãÇì«Ç›çûÇﬁ
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

	printf("File '" CL_WHITE "%s" CL_RESET "' read.\n", cfgName);

	return 0;
}
