// Copyright (c) Freya Development Team - Licensed under GNU GPL
// For more information, see License.txt in the main folder

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../common/socket.h"
#include "../common/timer.h"
#include "../common/db.h"
#include "../common/malloc.h"
#include "../common/lock.h"
#include "../common/utils.h"
#include "map.h"
#include "chrif.h"
#include "clif.h"
#include "intif.h"
#include "pc.h"
#include "npc.h"
#include "mob.h"
#include "pet.h"
#include "itemdb.h"
#include "script.h"
#include "battle.h"
#include "skill.h"
#include "party.h"
#include "guild.h"
#include "chat.h"
#include "trade.h"
#include "storage.h"
#include "vending.h"
#include "nullpo.h"
#include "atcommand.h"
#include "status.h"
#include "ranking.h"
#include "unit.h"

#ifdef MEMWATCH
#include "memwatch.h"
#endif

#define PVP_CALCRANK_INTERVAL 1000

#define STATE_BLIND 0x10

static unsigned int exp_table[16][MAX_LEVEL];
static short statp[MAX_LEVEL];

static int dirx[8] = {0,-1,-1,-1,0,1,1,1};
static int diry[8] = {1,1,0,-1,-1,-1,0,1};

static unsigned int equip_pos[11] = {0x0080,0x0008,0x0040,0x0004,0x0001,0x0200,0x0100,0x0010,0x0020,0x0002,0x8000};

static struct gm_account *gm_account = NULL;
static int GM_num = 0;
static int use_nameditem, use_itemid;

/*==========================================
 *
 *------------------------------------------
 */
int pc_isGM(struct map_session_data *sd) {

	int i;

	nullpo_retr(0, sd);

	if (sd->bl.type != BL_PC)
		return 0;

	if (sd->fd == 0)
		return 99;

	for(i = 0; i < GM_num; i++)
		if (gm_account[i].account_id == sd->status.account_id)
			return gm_account[i].level;

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
void pc_set_gm_level(int account_id, unsigned char level) {

	int i;
	struct map_session_data *sd;

	if (level > 99)
		return;

	// Set GM level in the structure of the GM
	for (i = 0; i < fd_max; i++)
		if (session[i] && (sd = session[i]->session_data) && sd->state.auth)
			if (sd->status.account_id == account_id) {
				sd->GM_level = level;
				break;
			}

	// Set GM level in the local database
	for (i = 0; i < GM_num; i++) {
		if (account_id == gm_account[i].account_id) {
			gm_account[i].level = level;
			return;
		}
	}

	// Update database only if level != 0
	if (level > 0) {
		if (GM_num == 0) {
			FREE(gm_account);
			MALLOC(gm_account, struct gm_account, 1);
		} else {
			REALLOC(gm_account, struct gm_account, GM_num + 1);
		}
		gm_account[GM_num].account_id = account_id;
		gm_account[GM_num].level = level;
		GM_num++;
	}

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void pc_set_gm_level_by_gm(int account_id, signed char level, int account_id_of_gm) {
	char output[MAX_MSG_LEN];
	struct map_session_data *sd;

	// Display message
	if (account_id_of_gm != -1) {
		// search if gm on this server
		if ((sd = map_id2sd(account_id_of_gm)) != NULL && sd->state.auth) {
			switch (level) {
			case -1: // player not found
				sprintf(output, msg_txt(678), account_id); // Player (account: %d) that you want to change the GM level doesn't exist.
				break;
			case -2: // gm level doesn't authorise you
				sprintf(output, msg_txt(679), account_id); // You are not authorised to change the GM level of this player (account: %d).
				break;
			case -3: // already right value
				sprintf(output, msg_txt(680), account_id); // The player (account: %d) already has the specified GM level.
				break;
			default:
				sprintf(output, msg_txt(681), account_id, level); // GM level of the player (account: %d) changed to %d.
				break;
			}
			clif_displaymessage(sd->fd, output);
		}
	}

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
int pc_iskiller(struct map_session_data *src, struct map_session_data *target) {

	nullpo_retr(0, src);

	if (src->bl.type != BL_PC)
		return 0;
	if (src->special_state.killer)
		return 1;

	if (target->bl.type != BL_PC)
		return 0;
	if (target->special_state.killable)
		return 1;

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
static int distance(int x0, int y_0, int x1, int y_1) {

	int dx, dy;

	dx = abs(x0  - x1);
	dy = abs(y_0 - y_1);

	return dx > dy ? dx : dy;
}

/*==========================================
 *
 *------------------------------------------
 */
int pc_invincible_timer(int tid, unsigned int tick, int id, int data) {

	struct map_session_data *sd;

	if ((sd = (struct map_session_data *)map_id2bl(id)) == NULL || sd->bl.type != BL_PC)
		return 1;

	if (sd->invincible_timer != tid) {
		if (battle_config.error_log)
			printf("invincible_timer %d != %d\n", sd->invincible_timer, tid);
		return 0;
	}

	sd->invincible_timer = -1;
	skill_unit_move(&sd->bl, tick, 1);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
void pc_setinvincibletimer(struct map_session_data *sd, int val) {

	nullpo_retv(sd);

	pc_delinvincibletimer(sd);
	sd->invincible_timer = add_timer(gettick_cache + val, pc_invincible_timer, sd->bl.id, 0);

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void pc_delinvincibletimer(struct map_session_data *sd) {

	if (sd->invincible_timer != -1) {
		delete_timer(sd->invincible_timer, pc_invincible_timer);
		sd->invincible_timer = -1;
	}
	skill_unit_move(&sd->bl, gettick_cache, 1);

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
int pc_jail_timer(int tid, unsigned int tick, int id, int data) {

	struct map_session_data *sd;

	if ((sd = (struct map_session_data *)map_id2bl(id)) == NULL || sd->bl.type != BL_PC)
		return 1;

	if (sd->jailtimer != tid) {
		if (battle_config.error_log)
			printf("jail_timer %d != %d\n", sd->jailtimer, tid);
		return 0;
	}

	sd->jailtimer = -1;
	if (pc_readglobalreg(sd, "JAIL_TIMER") != 0) {
		pc_setglobalreg(sd, "JAIL_TIMER", 0);
		chrif_save(sd); // do pc_makesavestatus and save storage + account_reg/account_reg2 too
	}

	// send message to player
	clif_displaymessage(sd->fd, msg_txt(624)); // You have been discharged.
	// send general message to all players
	if (battle_config.jail_discharge_message & 1) { // Do we send message to ALL players when a player is discharged?
		char *message;
		CALLOC(message, char, 16 + strlen(msg_txt(631)) + 1); // name (16) + message (%s has been discharged from jail.) + NULL (1)
		sprintf(message, msg_txt(631), sd->status.name); // %s has been discharged from jail.
		intif_GMmessage(message, 0);
		FREE(message);
	}

	if (strcmp(sd->status.last_point.map, "sec_pri.gat") == 0) {
		pc_setsavepoint(sd, "prontera.gat", 156, 191); // Save char respawn point in Prontera
		pc_setpos(sd, "prontera.gat", 156, 191, 3, 0);
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
static int pc_spiritball_timer(int tid, unsigned int tick, int id, int data) {

	struct map_session_data *sd;
	int i;

	if ((sd = (struct map_session_data *)map_id2bl(id)) == NULL || sd->bl.type != BL_PC)
		return 1;

	if (sd->spirit_timer[0] != tid) {
		if (battle_config.error_log)
			printf("spirit_timer %d != %d\n",sd->spirit_timer[0],tid);
		return 0;
	}
	sd->spirit_timer[0] = -1;
	for(i = 1; i < sd->spiritball && i < MAX_SKILL_LEVEL; i++) {
		sd->spirit_timer[i-1] = sd->spirit_timer[i];
		sd->spirit_timer[i] = -1;
	}
	sd->spiritball--;
	if (sd->spiritball < 0)
		sd->spiritball = 0;
	clif_spiritball(sd);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
void pc_addspiritball(struct map_session_data *sd,int interval,int max) {

	int i;

	nullpo_retv(sd);

	if (max > MAX_SKILL_LEVEL)
		max = MAX_SKILL_LEVEL;
	if (sd->spiritball < 0)
		sd->spiritball = 0;

	if (sd->spiritball >= max) {
		if (sd->spirit_timer[0] != -1) {
			delete_timer(sd->spirit_timer[0], pc_spiritball_timer);
			sd->spirit_timer[0] = -1;
		}
		for(i = 1; i < max; i++) {
			sd->spirit_timer[i-1] = sd->spirit_timer[i];
			sd->spirit_timer[i] = -1;
		}
	}
	else
		sd->spiritball++;

	sd->spirit_timer[sd->spiritball-1] = add_timer(gettick_cache + interval, pc_spiritball_timer, sd->bl.id, 0);
	clif_spiritball(sd);

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void pc_delspiritball(struct map_session_data *sd, int count, int type) {

	int i;

	nullpo_retv(sd);

	if (sd->spiritball <= 0) {
		sd->spiritball = 0;
		return;
	}

	if (count > sd->spiritball)
		count = sd->spiritball;
	sd->spiritball -= count;
	if (count > MAX_SKILL_LEVEL)
		count = MAX_SKILL_LEVEL;

	if (count > 0) {
		for(i = 0; i < count; i++) {
			if (sd->spirit_timer[i] != -1) {
				delete_timer(sd->spirit_timer[i], pc_spiritball_timer);
				sd->spirit_timer[i] = -1;
			}
		}
		for(i = count; i < MAX_SKILL_LEVEL; i++) {
			sd->spirit_timer[i-count] = sd->spirit_timer[i];
			sd->spirit_timer[i] = -1;
		}
	}

	if (!type)
		clif_spiritball(sd);

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void pc_setrestartvalue(struct map_session_data *sd,int type) {


	unsigned int s_class;

	nullpo_retv(sd);

	s_class = pc_calc_base_job2(sd->status.class);

	if (sd->state.snovice_flag == 3) {
		sd->status.hp = sd->status.max_hp;
		sd->status.sp = sd->status.max_sp;
		sd->state.snovice_flag = 0;
		status_change_start(&sd->bl, SkillStatusChangeTable[MO_STEELBODY], 1, 0, 0, 0, skill_get_time(MO_STEELBODY, 1), 0);
	} else if (sd->special_state.restart_full_recover) {
		sd->status.hp = sd->status.max_hp;
		sd->status.sp = sd->status.max_sp;
	}
	else {
		if (s_class == JOB_NOVICE && battle_config.restart_hp_rate < 50) {
			sd->status.hp = (sd->status.max_hp) / 2;
		}
		else {
			if (battle_config.restart_hp_rate <= 0)
				sd->status.hp = 1;
			else {
				sd->status.hp = sd->status.max_hp * battle_config.restart_hp_rate /100;
				if (sd->status.hp <= 0)
					sd->status.hp = 1;
			}
		}
		if (battle_config.restart_sp_rate > 0) {
			int sp = sd->status.max_sp * battle_config.restart_sp_rate /100;
			if (sd->status.sp < sp)
				sd->status.sp = sp;
		}
	}
	if (type & 1)
		clif_updatestatus(sd, SP_HP);
	if (type & 1)
		clif_updatestatus(sd, SP_SP);

	if (type & 2 && s_class != JOB_NOVICE && // Only Novices/High Novice/Baby Novice will recieve no penalty
	    !map[sd->bl.m].flag.nozenypenalty) {
		double zeny_penalty;
		zeny_penalty = (double)battle_config.zeny_penalty + ((double)sd->status.base_level * (double)battle_config.zeny_penalty_by_lvl);
		if (battle_config.zeny_penalty_percent > 0)
			zeny_penalty = zeny_penalty + ((double)sd->status.zeny * (double)battle_config.zeny_penalty_percent / 10000.);
		if (zeny_penalty >= 1) {
			sd->status.zeny -= zeny_penalty;
			if (sd->status.zeny < 0)
				sd->status.zeny = 0;
			clif_updatestatus(sd, SP_ZENY);
		}
	}

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
static int pc_walktoxy_sub(struct map_session_data *);

/*==========================================
 *
 *------------------------------------------
 */
void pc_makesavestatus(struct map_session_data *sd) {

	int i;

	nullpo_retv(sd);

	if (!battle_config.save_clothcolor)
		sd->status.clothes_color=0;

	if (pc_isdead(sd)) {
		pc_setrestartvalue(sd, 0);
		memcpy(&sd->status.last_point, &sd->status.save_point, sizeof(sd->status.last_point));
	} else {
		memset(sd->status.last_point.map, 0, sizeof(sd->status.last_point.map));
		strncpy(sd->status.last_point.map, sd->mapname, 16); // 17 - NULL
		sd->status.last_point.x = sd->bl.x;
		sd->status.last_point.y = sd->bl.y;
	}

	if (map[sd->bl.m].flag.nosave) {
		struct map_data *m = &map[sd->bl.m];
		if (strcmp(m->save.map, "SavePoint") == 0)
			memcpy(&sd->status.last_point, &sd->status.save_point, sizeof(sd->status.last_point));
		else
			memcpy(&sd->status.last_point, &m->save, sizeof(sd->status.last_point));
	}

	if (battle_config.muting_players && sd->status.manner > 0)
		sd->status.manner = 0;

	for(i = 0; i < MAX_SKILL; i++) {
		// flag: 0 (normal), 1 (only card), 2-12 (card and skill (skill level +2)), 13 (cloneskill)
		if (sd->status.skill[i].flag == 13)
		{
			sd->status.skill[i].id = 0;
			sd->status.skill[i].lv = 0;
			sd->status.skill[i].flag = 0;
		} else if ((sd->status.skill[i].lv != 0) && (sd->status.skill[i].id == 0))
			sd->status.skill[i].id = i;
	}

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void pc_setnewpc(struct map_session_data *sd, int account_id, int char_id, int login_id1, int client_tick, unsigned char sex, int fd) {

	sd->bl.id             = account_id;
	sd->char_id           = char_id;
	sd->login_id1         = login_id1;
	sd->client_tick       = client_tick;
	sd->sex               = sex;
	sd->bl.type           = BL_PC;
	sd->canact_tick       = gettick_cache;
	sd->canmove_tick      = gettick_cache;
	sd->canlog_tick       = gettick_cache;

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
int pc_equippoint(struct map_session_data *sd,int n) {

	int ep = 0;

	nullpo_retr(0, sd);

	if (sd->inventory_data[n]) {
		ep = sd->inventory_data[n]->equip;
		if (sd->inventory_data[n]->look == 1 || sd->inventory_data[n]->look == 2 || sd->inventory_data[n]->look == 6) {
			if (ep == 2 && (pc_checkskill(sd,AS_LEFT) > 0 || pc_calc_base_job2(sd->status.class) == JOB_ASSASSIN))
				return 34;
		}
	}

	return ep;
}

/*==========================================
 *
 *------------------------------------------
 */
void pc_setinventorydata(struct map_session_data *sd) {

	int i, id;

	nullpo_retv(sd);

	for(i = 0; i < MAX_INVENTORY; i++) {
		id = sd->status.inventory[i].nameid;
		sd->inventory_data[i] = itemdb_search(id);
	}

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void pc_calcweapontype(struct map_session_data *sd) {
	nullpo_retv(sd);

	if (sd->weapontype1 != 0 && sd->weapontype2 == 0)
		sd->status.weapon = sd->weapontype1; // Use Left-Handed Weapon Type (Right-Handed Weapon not equipped)
	if (sd->weapontype1 == 0 && sd->weapontype2 != 0)
		sd->status.weapon = sd->weapontype2; // Use Right-Handed Weapon Type (Left-Handed Weapon not equipped)
	else if (sd->weapontype1 == 1 && sd->weapontype2 == 1)
		sd->status.weapon = 50; // Dual Daggers
	else if (sd->weapontype1 == 2 && sd->weapontype2 == 2)
		sd->status.weapon = 51; // Dual One-Handed Swords
	else if (sd->weapontype1 == 6 && sd->weapontype2 == 6)
		sd->status.weapon = 52; // Dual One-Handed Axes
	else if ((sd->weapontype1 == 1 && sd->weapontype2 == 2) ||
	         (sd->weapontype1 == 2 && sd->weapontype2 == 1))
		sd->status.weapon = 53; // Dual-Wield: Dagger + One-Handed Sword
	else if ((sd->weapontype1 == 1 && sd->weapontype2 == 6) ||
	         (sd->weapontype1 == 6 && sd->weapontype2 == 1))
		sd->status.weapon = 54; // Dual-Wield: Dagger + One-Handed Axe
	else if ((sd->weapontype1 == 2 && sd->weapontype2 == 6) ||
	         (sd->weapontype1 == 6 && sd->weapontype2 == 2))
		sd->status.weapon = 55; // Dual-Wield: One-Handed Axe + One-Handed Sword
	else
		sd->status.weapon = sd->weapontype1; // If all else fails, use Left-Handed Weapon Type

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void pc_setequipindex(struct map_session_data *sd) {
	int i, j;

	nullpo_retv(sd);

	// Calculates total number of slots in array sd->equip_index
	for(i=0;i<11;i++) // Ten equipment slots total
		// Sets each value to -1 (No equip present), for the time being
		sd->equip_index[i] = -1;

	for(i=0;i<MAX_INVENTORY;i++) {
		if (sd->status.inventory[i].nameid <= 0)
			continue;
		if (sd->status.inventory[i].equip) {
			for(j=0;j<11;j++)
				if (sd->status.inventory[i].equip & equip_pos[j])
					sd->equip_index[j] = i;
			if (sd->status.inventory[i].equip & 0x0002) { // Player Left-Handed Weapon Slot
				if (sd->inventory_data[i])
					sd->weapontype1 = sd->inventory_data[i]->look; // Gets Left-Handed Weapon Type (item_db: "view" parameter)
				else
					sd->weapontype1 = 0;
			}
			if (sd->status.inventory[i].equip & 0x0020) { // Player Shield/Right-Handed Weapon Slot
				if (sd->inventory_data[i]) {
					if (sd->inventory_data[i]->type == 4) {
						if (sd->status.inventory[i].equip == 0x0020)
							sd->weapontype2 = sd->inventory_data[i]->look; // Gets Right-Handed Weapon Type (item_db: view parameter)
						else
							sd->weapontype2 = 0;
					}
					else
						sd->weapontype2 = 0;
				}
				else
					sd->weapontype2 = 0;
			}
		}
	}
	pc_calcweapontype(sd); // Calculates Weapon Type: weapontype1 and weapontype2 -> status.weapon

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
int pc_isequip(struct map_session_data *sd, int n) {

	struct item_data *item;
	short s_class, upper_type;

	nullpo_retr(0, sd);

	item = sd->inventory_data[n];
	
	int item_id;
	item_id = sd->status.inventory[n].nameid;

	if (battle_config.gm_allequip > 0 && sd->GM_level >= battle_config.gm_allequip)
		return 1;

	if (map[sd->bl.m].flag.pvp && (item->flag.no_equip&1))
		return 0;

	else if (map[sd->bl.m].flag.gvg && (item->flag.no_equip&2))
		return 0;

	if (sd->sc_count) { // Item restriction based on status changes
		if ((item->equip & 0x0002 || item->equip & 0x0020) && sd->sc_data[SC_STRIPWEAPON].timer != -1)
			return 0;
		if (item->equip & 0x0020 && item->type == 5 && sd->sc_data[SC_STRIPSHIELD].timer != -1)
			return 0;
		if (item->equip & 0x0010 && sd->sc_data[SC_STRIPARMOR].timer != -1)
			return 0;
		if (item->equip & 0x0100 && sd->sc_data[SC_STRIPHELM].timer != -1)
			return 0;
		if (sd->sc_data[SC_SPIRIT].timer != -1 && sd->sc_data[SC_SPIRIT].val2 == SL_SUPERNOVICE) {
			// If Spirit of the Super Novice is active, and Super Novice Lv is over 90, can equip all upper headgears
			if (item->equip & 0x0100 && sd->status.base_level >= 90)
				return 1; // All upper Headgears are equipable, regardless of item class restriction
			if (item->equip & 0x0002 && sd->status.base_level >= 96 && (item->look == 1 || item->look == 2 || (item->look >= 6 && item->look <= 10)))
				return 1; // All Daggers, One-Handed Swords, Maces, Axes, and Staffs are equipable, regardless of item class restriction
		}
	}

	if (item == NULL)
		return 0;
	// Item gender restriction
	if (item->sex != 2 && sd->status.sex != item->sex)
		return 0;
	// Item base level restriction
	if (item->elv > 0 && sd->status.base_level < item->elv)
		return 0;

	// Upper job restriction
	upper_type = pc_get_upper_type(sd->status.class);
	if (upper_type == 0)
		return 0;
	// 0 = 1 + 2 + 4 = 7 = All classes
	if (item->flag.upper != 0) {
		if (!(item->flag.upper & upper_type))
			return 0;
	}

	s_class = pc_calc_base_job2(sd->status.class);

	// Normalize special classes into their normal version
	switch(s_class) {
		case JOB_KNIGHT2:
			s_class = JOB_KNIGHT;
			break;
		case JOB_CRUSADER2:
			s_class = JOB_CRUSADER;
			break;
		case JOB_STAR_GLADIATOR2:
			s_class = JOB_STAR_GLADIATOR;
	}

	// Item class restriction
	if (((1<<s_class)&item->class) == 0)
		return 0;

	return 1;
}

/*==========================================
 *
 *------------------------------------------
 */
int pc_break_equip(struct map_session_data *dstsd, unsigned int where) {

	int i, j;
	char output[255];

	nullpo_retr(-1, dstsd);

	if (dstsd->unbreakable_equip&where)
		return 0;
	if (dstsd->unbreakable >= rand() % 100)
		return 0;
	
	if (where == EQP_WEAPON) {
		switch(dstsd->status.weapon) {
			case 0:  // Bare fists
			case 6:  // One-Handed Axes
			case 7:  // Two-Handed Axes
			case 8:  // One-Handed Maces
			case 9:  // Two-Handed Maces (Unused)
			case 10: // Rods
			case 15: // Books
			case 22: // Fuuma Shurikens
				return 0; // Weapons that can't be broken
		}
	}

	switch (where) {
		case EQP_WEAPON: i = SC_CP_WEAPON; break;
		case EQP_ARMOR:  i = SC_CP_ARMOR;  break;
		case EQP_SHIELD: i = SC_CP_SHIELD; break;
		case EQP_HELM:   i = SC_CP_HELM;   break;
		default:
			return 0;
	}
	if (dstsd->sc_data[i].timer != -1)
		return 0;

	for (i = 0; i < 11; i++) {
		if ((j = dstsd->equip_index[i]) > 0 && dstsd->status.inventory[j].attribute != 1 &&
		    ((where == EQP_HELM && i == 6) ||
		     (where == EQP_ARMOR && i == 7) ||
		     (where == EQP_WEAPON && (i == 8 || i == 9) && dstsd->inventory_data[j]->type == 4) ||
		     (where == EQP_SHIELD && i == 9 && dstsd->inventory_data[j]->type == 5)))
		{
			dstsd->status.inventory[j].attribute = 1;
			pc_unequipitem(dstsd, j, 3);
			clif_emotion(&dstsd->bl, 23);
			sprintf(output, msg_txt(669), dstsd->inventory_data[j]->jname); // %s has broken.
			clif_displaymessage(dstsd->fd, output);
			clif_equiplist(dstsd);
			return 1;
		}
	}

	return 1;
}

/*==========================================
 *
 *------------------------------------------
 */
void pc_authok_final_step(int id, time_t connect_until_time) {
	struct map_session_data *sd = NULL;

	struct party *p;
	struct guild *g;
	int i;

	sd = map_id2sd(id);

	if (!sd)
		return;

	sd->state.connect_new = 1;
	sd->view_class = sd->status.class;
	sd->speed = DEFAULT_WALK_SPEED;
	sd->walktimer = -1;
	sd->next_walktime = -1;
	sd->attacktimer = -1;
	sd->followtimer = -1;
	sd->skilltimer = -1;
	sd->skillitem = -1;
	sd->skillitemlv = -1;
	sd->invincible_timer = -1;
	sd->jailtimer = -1;
	sd->pvp_timer = -1;
	sd->pet_hungry_timer = -1;
	sd->party_x = -1;
	sd->party_y = -1;
	sd->party_hp = -1;
	sd->canact_tick     = gettick_cache;
	sd->canmove_tick    = gettick_cache;
	sd->canregen_tick   = gettick_cache;
	sd->canuseitem_tick = gettick_cache;
	sd->attackabletime  = gettick_cache;
	sd->GM_level = pc_isGM(sd);
	sd->change_level = pc_readglobalreg(sd, "jobchange_level");

	sd->state.autoloot_rate = battle_config.item_auto_get;
	if (battle_config.item_auto_get_loot)
		sd->state.autolootloot_flag = 1;
	if (battle_config.disp_experience)
		sd->state.displayexp_flag = 1;
	if (battle_config.idle_disconnect != 0)
		sd->lastpackettime = time(NULL);
	if ((battle_config.atc_gmonly == 0 || sd->GM_level) &&  (sd->GM_level >= get_atcommand_level(AtCommand_Hide)))
		sd->status.option &= (OPTION_MASK | OPTION_HIDE);
	else
		sd->status.option &= OPTION_MASK;

	for(i = 0; i < MAX_SKILL_LEVEL; i++)
		sd->spirit_timer[i] = -1;
	for(i = 0; i < MAX_SKILLTIMERSKILL; i++)
		sd->skilltimerskill[i].timer = -1;

	pc_setinventorydata(sd);
	pc_checkitem(sd);

	for(i = 0; i < SC_MAX; i++)
		sd->sc_data[i].timer = -1;

	sd->last_saving = gettick_cache;

	for(i = 0; i < MAX_EVENTTIMER; i++)
		sd->eventtimer[i] = -1;

	if (pc_setpos(sd, sd->status.last_point.map, sd->status.last_point.x, sd->status.last_point.y, 0, 0) != 0) {
		if (battle_config.error_log)
			printf("Last_point_map %s not found.\n", sd->status.last_point.map);
		if (map_checkmapname("prontera.gat") == -1 || pc_setpos(sd, "prontera.gat", 273, 354, 0, 0) != 0) {
			clif_authfail_fd(sd->fd, 0); // 00 = Disconnected from server
		}
	}

	if (sd->status.party_id > 0) {
		if ((p = party_search(sd->status.party_id)) == NULL) {
			party_request_info(sd->status.party_id);
			intif_party_changemap(sd, 1); // flag: 0: offline, 1:online
		} else {
			for(i = 0; i < MAX_PARTY; i++)
				if (p->member[i].account_id == sd->status.account_id)
					if (strcmp(p->member[i].name, sd->status.name) == 0) {
						if (p->exp && !party_check_share_range(p, sd->status.base_level))
							intif_party_changeoption(sd->status.party_id, sd->status.account_id, 0, p->item);
						intif_party_changemap(sd, 1); // flag: 0: offline, 1:online
						break;
					}
			if (i == MAX_PARTY) {
				sd->status.party_id = 0;
				sd->party_sended = 0;
			}
		}
	}
	if (sd->status.guild_id > 0) {
		if ((g = guild_search(sd->status.guild_id)) == NULL)
			guild_request_info(sd->status.guild_id);
		else {
			if (strcmp(sd->status.name, g->master) == 0)
				sd->state.gmaster_flag = g;
			else {
				for(i = 0; i < g->max_member; i++) {
					if (g->member[i].char_id == sd->status.char_id)
						break;
				}
				if (i == g->max_member) {
					sd->status.guild_id = 0;
					sd->guild_sended = 0;
					sd->guild_emblem_id = 0;
				}
			}
		}
	}

	clif_authok(sd);
	map_addchariddb(sd->status.char_id, sd->status.name);

	if ((i = pc_checkskill(sd, RG_PLAGIARISM)) > 0) {
		sd->cloneskill_id = pc_readglobalreg(sd,"CLONE_SKILL");
		if (sd->cloneskill_id > 0) {
			sd->status.skill[sd->cloneskill_id].id = sd->cloneskill_id;
			sd->status.skill[sd->cloneskill_id].lv = pc_readglobalreg(sd,"CLONE_SKILL_LV");
			if (i < sd->status.skill[sd->cloneskill_id].lv)
				sd->status.skill[sd->cloneskill_id].lv = i;
			sd->status.skill[sd->cloneskill_id].flag = 13;
			clif_skillinfoblock(sd);
		}
	}

	// Automated script events
	if (script_config.event_requires_trigger) {
		sd->state.event_death = pc_readglobalreg(sd, script_config.die_event_name);
		sd->state.event_kill = pc_readglobalreg(sd, script_config.kill_event_name);
		sd->state.event_disconnect = pc_readglobalreg(sd, script_config.logout_event_name);
	} else {
		sd->state.event_death = 1;
		sd->state.event_kill = 1;
		sd->state.event_disconnect = 1;
	}

	sd->tk_mission_target_id = 0;
	sd->tk_mission_count = 0;
	if (pc_checkskill(sd, TK_MISSION))
	{
		sd->tk_mission_target_id = pc_readglobalreg(sd, "TK_MISSION_ID");
		sd->tk_mission_count = pc_readglobalreg(sd, "TK_MISSION_COUNT");
	}

	status_calc_pc(sd, 1);
	sd->state.auth = 1; // Auth the player when all the status vars are loaded. Otherwise it could crash in pc_natural_heal timer

	if (sd->GM_level)
		printf("Connection accepted: character '" CL_WHITE "%s" CL_RESET "' (account: " CL_WHITE "%d" CL_RESET "; GM level " CL_WHITE "%d" CL_RESET ").\n", sd->status.name, sd->status.account_id, sd->GM_level);
	else
		printf("Connection accepted: Character '" CL_WHITE "%s" CL_RESET "' (account: " CL_WHITE "%d" CL_RESET ").\n", sd->status.name, sd->status.account_id);

  {
	struct npc_data *npc;
	if ((npc = npc_name2id(script_config.login_event_name))) {
		run_script(npc->u.scr.script, 0, sd->bl.id, npc->bl.id);
	}
  }

	// Message of the Day
  {
	char buf[256];
	FILE *fp;
	if ((fp = fopen(motd_txt, "r")) != NULL) {
		while (fgets(buf, sizeof(buf), fp) != NULL) { // fgets reads until maximum one less than size and add '\0' -> so, it's not necessary to add -1
			if (buf[0] == '/' && buf[1] == '/')
				continue;
			for(i = 0; buf[i]; i++) {
				if (buf[i] == '\r' || buf[i]== '\n') {
					buf[i] = 0;
					break;
				}
			}
			if (battle_config.motd_type)
				clif_disp_onlyself(sd, buf);
			else
				clif_displaymessage(sd->fd, buf);
		}
		fclose(fp);
	} else if (battle_config.error_log)
		printf("pc_authok_final_step: '" CL_YELLOW "%s" CL_RESET "' not found.\n", motd_txt);
  }

	// message about activation of main channel
	if (sd->GM_level >= get_atcommand_level(AtCommand_Main)) {
		// == 0 // not activated
		if (battle_config.atcommand_main_channel_at_start == 1) // not activated (with message)
			clif_wis_message(sd->fd, wisp_server_name, msg_txt(608), strlen(msg_txt(608)) + 1); // Your Main channel is OFF.
		else if (battle_config.atcommand_main_channel_at_start > 1) {
			// == 2 // activated
			sd->state.main_flag = 1;
			if (battle_config.atcommand_main_channel_at_start == 3) // activated (with message)
				clif_wis_message(sd->fd, wisp_server_name, msg_txt(609), strlen(msg_txt(609)) + 1); // Your Main channel is ON.
		}
		// option about members of guild
		if (agit_flag == 1 && // 0: WoE not starting, Woe is running
		    sd->status.guild_id > 0) {
			if (battle_config.atcommand_main_channel_when_woe > sd->GM_level) // is not possible to use @main when WoE and in guild
				clif_wis_message(sd->fd, wisp_server_name, msg_txt(682), strlen(msg_txt(682)) + 1); // For the record: War of Emperium is actually running. Because you are member of a guild, you can not use 'Main channel'.
			else if (battle_config.atcommand_main_channel_on_gvg_map_woe > sd->GM_level) // is not possible to use @main when WoE and in guild ON GvG maps
				clif_wis_message(sd->fd, wisp_server_name, msg_txt(684), strlen(msg_txt(684)) + 1); // For the record: War of Emperium is actually running. Because you are member of a guild, you can not use 'Main channel' on GvG maps.
		}
	}

	// message about War Of Emperium
	if (agit_flag == 1) {
		char tmpstr[MAX_MSG_LEN + 100]; // max size of msg_txt + security (char name, char id, etc...) (100)
		sprintf(tmpstr, msg_txt(668), wisp_server_name); // (From %s) The War Of Emperium is running...
		clif_disp_onlyself(sd, tmpstr); // (From %s) The War Of Emperium is running...
	}

	// message of the limited time of the account
	if (connect_until_time != 0) { // don't display if it's unlimited or unknow value
		char tmpstr[MAX_MSG_LEN + 100]; // max size of msg_txt + security (char name, char id, etc...) (100)
		strftime(tmpstr, sizeof(tmpstr) - 1, msg_txt(501), localtime(&connect_until_time)); // "Your account time limit is: %d-%m-%Y %H:%M:%S."
		clif_wis_message(sd->fd, wisp_server_name, tmpstr, strlen(tmpstr) + 1);
	}

	// check for jail
  {
	time_t timestamp;
	if ((timestamp = (time_t)pc_readglobalreg(sd, "JAIL_TIMER")) != 0) {
		// finished time
		if (timestamp < time(NULL) + 5) { // wait at least 5 seconds
			// timer can do all discharge function.
			sd->jailtimer = add_timer(gettick_cache + (5 * 1000), pc_jail_timer, sd->bl.id, 0); // discharge in 5 sec (time to send all informations)
		} else {
			clif_displaymessage(sd->fd, msg_txt(626)); // Remember, you are in jails.
			sd->jailtimer = add_timer(gettick_cache + ((timestamp - time(NULL)) * 1000), pc_jail_timer, sd->bl.id, 0);
			// prisoner not in jail map. 2 solutions:
			// - jail map doesn't exist. he must wait end of jail time
			// - GM help him to escape from jail :)
			if (strcmp(sd->status.last_point.map, "sec_pri.gat") != 0) { // curious: jail time but not in jails?
				int x, y;
				switch(rand() % 2) {
				case 0:
					x = 24;
					y = 75;
					break;
				default:
					x = 49;
					y = 75;
					break;
				}
				pc_setsavepoint(sd, "sec_pri.gat", x, y);
				pc_setpos(sd, "sec_pri.gat", x, y, 3, 0);
				// because it's map change at connection -> for disconnection
				clif_setwaitclose(sd->fd);
				clif_displaymessage(sd->fd, msg_txt(630)); // Map error, please reconnect.
			}
		}
	}
  }

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void pc_authok(int id, int login_id2, struct mmo_charstatus *st) {

	struct map_session_data *sd;

	sd = map_id2sd(id);

	if (!sd) {
		return;
	}

	sd->login_id2 = login_id2;
	memcpy(&sd->status, st, sizeof(*st));

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
int pc_authfail(int id) {

	struct map_session_data *sd;

	sd = map_id2sd(id);
	if (sd == NULL)
		return 1;

	clif_authfail_fd(sd->fd, 0); // Disconnected from server

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int pc_doubble_connection(int id) {

	struct map_session_data *sd;

	sd = map_id2sd(id);
	if (sd == NULL)
		return 1;

	clif_authfail_fd(sd->fd, 2);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
static int pc_calc_skillpoint(struct map_session_data* sd) {

	int i, skill, skill_point = 0;

	nullpo_retr(0, sd);

	for(i = 1; i < MAX_SKILL; i++) {
		if ((skill = pc_checkskill(sd, i)) > 0) {
			if (!(skill_get_inf2(i) & 0x01) || battle_config.quest_skill_learn) {
				if ((i == 142 || i == 143) && sd->status.class >= 4001 && sd->status.class <= 4022) // if platinum skills were given for free
					continue;
				if (!sd->status.skill[i].flag) // flag: 0 (normal), 1 (only card), 2-12 (card and skill (skill level +2)), 13 (cloneskill)
					skill_point += skill;
				else if (sd->status.skill[i].flag >= 2 && sd->status.skill[i].flag != 13) {
					skill_point += (sd->status.skill[i].flag - 2);
				}
			}
		}
	}

	return skill_point;
}

/*==========================================
 *
 *------------------------------------------
 */
void pc_checkminskill(struct map_session_data* sd) {

	int skill_points, min_points;
	int previous_class_level;

	// Check Novice skill
	if (sd->status.skill[NV_BASIC].id == 0) {
		sd->status.skill[NV_BASIC].id = NV_BASIC;
		sd->status.skill[NV_BASIC].lv = 0;
		clif_skillinfoblock(sd);
	}

	// If we don't check minimum skills points
	if (!battle_config.check_minimum_skill_points)
		return;

	// Calculate actual skills points
	skill_points = sd->status.skill_point + pc_calc_skillpoint(sd);

	// Calculate minimum skills points
	min_points = 0;
	switch (sd->status.class) {
	case 0: // Novice
		min_points = sd->status.job_level - 1;
		break;
	case 1: // Swordsman
	case 2: // Mage
	case 3: // Archer
	case 4: // Acolyte
	case 5: // Merchant
	case 6: // Thief
		min_points = 9 + sd->status.job_level - 1;
		break;
	case 7: // Knight
	case 8: // Priest
	case 9: // Wizard
	case 10: // Blacksmith
	case 11: // Hunter
	case 12: // Assassin
	case 13: // Peco knight
	case 14: // Crusader
	case 15: // Monk
	case 16: // Sage
	case 17: // Rogue
	case 18: // Alchemist
	case 19: // Bard
	case 20: // Dancer
	case 21: // Peco crusader
		previous_class_level = sd->change_level;
		if (previous_class_level < 40 || previous_class_level > 50)
			previous_class_level = 40;
		min_points = 9 + (previous_class_level - 1) + sd->status.job_level - 1; // Minimum calculation, so changement at job level 40
		break;
	case 22: // Wedding
		break;
	case 23: // Super Novice
		min_points = 9 + 20 + sd->status.job_level - 1; // Minimum calculation, so changement at base level (not job level) 45 + 20 points bonus
		break;
	case 24: // Gunslinger
	case 25: // Ninja
		min_points = 9 + sd->status.job_level - 1; // Both are 1st class characters, max job level 70 [GoodKat]
		break;
	case 26: // Xmas
		break;
	case 4001: // Novice High
		min_points = sd->status.job_level - 1;
		break;
	case 4002: // Swordsman High
	case 4003: // Mage High
	case 4004: // Archer High
	case 4005: // Acolyte High
	case 4006: // Merchant High
	case 4007: // Thief High
		min_points = 9 + sd->status.job_level - 1;
		break;
	case 4008: // Lord Knight
	case 4009: // High Priest
	case 4010: // High Wizard
	case 4011: // Whitesmith
	case 4012: // Sniper
	case 4013: // Assassin Cross
	case 4014: // Peco Knight
	case 4015: // Paladin
	case 4016: // Champion
	case 4017: // Professor
	case 4018: // Stalker
	case 4019: // Creator
	case 4020: // Clown
	case 4021: // Gypsy
	case 4022: // Peco Paladin
		previous_class_level = sd->change_level;
		if (previous_class_level < 40 || previous_class_level > 50)
			previous_class_level = 40;
		min_points = 9 + (previous_class_level - 1) + sd->status.job_level - 1; // Minimum calculation, so changed at job level 40
		break;
	case 4023: // Baby Novice
		min_points = sd->status.job_level - 1;
		break;
	case 4024: // Baby Swordsman
	case 4025: // Baby Mage
	case 4026: // Baby Archer
	case 4027: // Baby Acolyte
	case 4028: // Baby Merchant
	case 4029: // Baby Thief
		min_points = 9 + sd->status.job_level - 1;
		break;
	case 4030: // Baby Knight
	case 4031: // Baby Priest
	case 4032: // Baby Wizard
	case 4033: // Baby Blacksmith
	case 4034: // Baby Hunter
	case 4035: // Baby Assassin
	case 4036: // Baby Peco Knight
	case 4037: // Baby Crusader
	case 4038: // Baby Monk
	case 4039: // Baby Sage
	case 4040: // Baby Rogue
	case 4041: // Baby Alchemist
	case 4042: // Baby Bard
	case 4043: // Baby Dancer
	case 4044: // Baby Peco Crusader
		previous_class_level = sd->change_level;
		if (previous_class_level < 40 || previous_class_level > 50)
			previous_class_level = 40;
		min_points = 9 + (previous_class_level - 1) + sd->status.job_level - 1; // Minimum calculation, so changed at job level 40
		break;
	case 4045: // Super Baby
		min_points = 9 + 20 + sd->status.job_level - 1; // Minimum calculation, so changed at base level 45 (not job level) + 20 points bonus
		break;
	case 4046: // Taekwon Kid
		min_points = 9 + sd->status.job_level - 1;
		break;
	case 4047: // Taekwon Master/Star Gladiator
	case 4048: // Taekwon Master/Star Gladiator (Flying)
	case 4049: // Soul Linker
		previous_class_level = sd->change_level;
		if (previous_class_level < 40 || previous_class_level > 50)
			previous_class_level = 40;
		min_points = 9 + (previous_class_level - 1) + sd->status.job_level - 1; // Minimum calculation, so changed at job level 40
		break;
	case 4050: // Bon Gun
	case 4051: // Death Knight
	case 4052: // Dark Collector
	case 4053: // Munak
		min_points = 9 + sd->status.job_level - 1;
		break;
	}

	if (skill_points < min_points) {
		sd->status.skill_point = sd->status.skill_point + (min_points - skill_points);
		clif_updatestatus(sd, SP_SKILLPOINT);
	}

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
int pc_checkmaxskill(struct map_session_data* sd) {

	int i, id;
	int skill_points, max_points, excess_points;
	int previous_class_level;
	struct pc_base_job s_class;

	// If we don't check maximum skills points
	if (battle_config.check_maximum_skill_points < 0)
		return 0;

	// Calculate actual skills points
	skill_points = sd->status.skill_point + pc_calc_skillpoint(sd);

	// Calculate minimum skills points
	max_points = 0;
	switch (sd->status.class) {
	case 0: // Novice
		max_points = sd->status.job_level - 1;
		break;
	case 1: // Swordsman
	case 2: // Mage
	case 3: // Archer
	case 4: // Acolyte
	case 5: // Merchant
	case 6: // Thief
		max_points = 9 + sd->status.job_level - 1;
		break;
	case 7: // Knight
	case 8: // Priest
	case 9: // Wizard
	case 10: // Blacksmith
	case 11: // Hunter
	case 12: // Assassin
	case 13: // Peco knight
	case 14: // Crusader
	case 15: // Monk
	case 16: // Sage
	case 17: // Rogue
	case 18: // Alchemist
	case 19: // Bard
	case 20: // Dancer
	case 21: // Peco crusader
		previous_class_level = sd->change_level;
		if (previous_class_level < 40 || previous_class_level > 50)
			previous_class_level = 40;
		max_points = 9 + (previous_class_level - 1) + sd->status.job_level - 1; // Minimum calculation, so changed at job level 40
		break;
	case 22: // Wedding
		break;
	case 23: // Super Novice
		max_points = 9 + 20 + sd->status.job_level - 1; // Minimum calculation, so changed at base level (not job level) 45 + 20 points bonus
		break;
	case 24: // Gunslinger
	case 25: // Ninja
		max_points = 9 + sd->status.job_level - 1; // Both are 1st class characters, max job level 70 [GoodKat]
		break;
	case 26: // Xmas
		break;
	case 4001: // Novice High
		max_points = sd->status.job_level - 1;
		break;
	case 4002: // Swordsman High
	case 4003: // Mage High
	case 4004: // Archer High
	case 4005: // Acolyte High
	case 4006: // Merchant High
	case 4007: // Thief High
		max_points = 9 + sd->status.job_level - 1;
		break;
	case 4008: // Lord Knight
	case 4009: // High Priest
	case 4010: // High Wizard
	case 4011: // Whitesmith
	case 4012: // Sniper
	case 4013: // Assassin Cross
	case 4014: // Peco Knight
	case 4015: // Paladin
	case 4016: // Champion
	case 4017: // Professor
	case 4018: // Stalker
	case 4019: // Creator
	case 4020: // Clown
	case 4021: // Gypsy
	case 4022: // Peco Paladin
		previous_class_level = sd->change_level;
		if (previous_class_level < 40 || previous_class_level > 50)
			previous_class_level = 40;
		max_points = 9 + (previous_class_level - 1) + sd->status.job_level - 1; // Minimum calculation, so changed at job level 40
		break;
	case 4023: // Baby Novice
		max_points = sd->status.job_level - 1;
		break;
	case 4024: // Baby Swordsman
	case 4025: // Baby Mage
	case 4026: // Baby Archer
	case 4027: // Baby Acolyte
	case 4028: // Baby Merchant
	case 4029: // Baby Thief
		max_points = 9 + sd->status.job_level - 1;
		break;
	case 4030: // Baby Knight
	case 4031: // Baby Priest
	case 4032: // Baby Wizard
	case 4033: // Baby Blacksmith
	case 4034: // Baby Hunter
	case 4035: // Baby Assassin
	case 4036: // Baby Peco Knight
	case 4037: // Baby Crusader
	case 4038: // Baby Monk
	case 4039: // Baby Sage
	case 4040: // Baby Rogue
	case 4041: // Baby Alchemist
	case 4042: // Baby Bard
	case 4043: // Baby Dancer
	case 4044: // Baby Peco Crusader
		previous_class_level = sd->change_level;
		if (previous_class_level < 40 || previous_class_level > 50)
			previous_class_level = 40;
		max_points = 9 + (previous_class_level - 1) + sd->status.job_level - 1; // Minimum calculation, so changed at job level 40
		break;
	case 4045: // Super Baby
		max_points = 9 + 20 + sd->status.job_level - 1; // Minimum calculation, so changed at base level 45 (not job level) + 20 points bonus
		break;
	case 4046: // Taekwon Kid
		max_points = 9 + sd->status.job_level - 1;
		break;
	case 4047: // Taekwon Master/Star Gladiator
	case 4048: // Taekwon Master/Star Gladiator (Flying)
	case 4049: // Soul Linker
		previous_class_level = sd->change_level;
		if (previous_class_level < 40 || previous_class_level > 50)
			previous_class_level = 40;
		max_points = 9 + (previous_class_level - 1) + sd->status.job_level - 1; // Minimum calculation, so changed at job level 40
		break;
	case 4050: // Bon Gun
	case 4051: // Death Knight
	case 4052: // Dark Collector
	case 4053: // Munak
		max_points = 9 + sd->status.job_level - 1;
		break;
	}

	// If player has too many skills points
	excess_points = skill_points - (max_points + battle_config.check_maximum_skill_points);
	if (excess_points <= 0) // No need to modify
		return 0;
	else {

		// First, remove excess points from reserve of skills points
		if (sd->status.skill_point > 0) {
			if (sd->status.skill_point < excess_points) {
				excess_points = excess_points - sd->status.skill_point;
				sd->status.skill_point = 0;
			} else {
				sd->status.skill_point = sd->status.skill_point - excess_points;
				excess_points = 0;
			}
			// Update skill points
			clif_updatestatus(sd, SP_SKILLPOINT);
		}

		if (excess_points > 0) {
			// If we check skill tree, there are only job skills to check
			if (!battle_config.skillfree) {
				s_class = pc_calc_base_job(sd->status.class);
				// Second: remove excess skill points from skills (begin from end of skill tree)
				for(i = MAX_SKILL_PER_TREE - 1; i >= 0 && excess_points > 0; i--)
					if ((id = skill_tree[s_class.upper][s_class.job][i].id) > 0)
						if (!(skill_get_inf2(id) & 0x01) || battle_config.quest_skill_learn) {
							if ((id == 142 || id == 143) && sd->status.class >= 4001 && sd->status.class < 4023) // If platinum skills were given for free
								continue;
							if (sd->status.skill[id].lv > 0) {
								if (!sd->status.skill[id].flag) { // flag: 0 (normal), 1 (only card), 2-12 (card and skill (skill level +2)), 13 (cloneskill)
									if (sd->status.skill[id].lv < excess_points) {
										excess_points = excess_points - sd->status.skill[id].lv;
										sd->status.skill[id].lv = 0;
									} else {
										sd->status.skill[id].lv = sd->status.skill[id].lv - excess_points;
										excess_points = 0;
									}
								}
							}
						}
			// Else, we must check all skill
			} else {
				// Second: remove excess skill point from skills (begin by end of skills)
				for(id = MAX_SKILL - 1; id >= 1 && excess_points > 0; id--)
					if (!(skill_get_inf2(id) & 0x01) || battle_config.quest_skill_learn) {
						if ((id == 142 || id == 143) && sd->status.class >= 4001 && sd->status.class < 4023) // If platinum skills were given for free
							continue;
						if (sd->status.skill[id].lv > 0) {
							if (!sd->status.skill[id].flag) { // flag: 0 (normal), 1 (only card), 2-12 (card and skill (skill level +2)), 13 (cloneskill)
								if (sd->status.skill[id].lv < excess_points) {
									excess_points = excess_points - sd->status.skill[id].lv;
									sd->status.skill[id].lv = 0;
								} else {
									sd->status.skill[id].lv = sd->status.skill[id].lv - excess_points;
									excess_points = 0;
								}
							}
						}
					}
			}
			// Update skill tree to player
			clif_skillinfoblock(sd);
		}
	}

	return 1;
}

/*==========================================
 *
 *------------------------------------------
 */
void pc_calc_skilltree(struct map_session_data *sd) {

	int i, j, k, id = 0, qid = 0, flag;
	int c = 0, s = 0;

	struct pc_base_job s_class;

	nullpo_retv(sd);

	// Reset skill IDs
	for(i = 0; i < MAX_SKILL; i++) {
		if (sd->status.skill[i].flag != 13) // flag: 0 (normal), 1 (only card), 2-12 (card and skill (skill level +2)), 13 (cloneskill)
			sd->status.skill[i].id = 0;
		if (sd->status.skill[i].flag && sd->status.skill[i].flag != 13) {
			sd->status.skill[i].lv = (sd->status.skill[i].flag == 1) ? 0 : sd->status.skill[i].flag - 2;
			sd->status.skill[i].flag = 0;
		}
	}

	s_class = pc_calc_base_job(sd->status.class);
	c = s_class.job;
	s = s_class.upper;

	// GM all skills (any jobs)
	if (sd->GM_level >= battle_config.gm_allskill) {
		for(s = 0; s < 3; s++)
			for(c = 0; c < MAX_SKILL_TREE; c++)
				for(i = 0; i < MAX_SKILL_PER_TREE && (id = skill_tree[s][c][i].id) > 0; i++)
					if (sd->status.skill[id].flag != 13) { // flag: 0 (normal), 1 (only card), 2-12 (card and skill (skill level +2)), 13 (cloneskill)
						if (sd->GM_level >= battle_config.gm_all_skill_platinum || !(skill_get_inf2(id) & 0x01) ||
						    (s_class.job == 1 && (id == 142 || id == 143))) { // High Novice free skills
							sd->status.skill[id].id = id;
							if (sd->status.skill[id].lv < skill_tree[s][c][i].max)
								sd->status.skill[id].lv = skill_tree[s][c][i].max;
						} else
							sd->status.skill[id].lv = 0;
					}

		// Remove skill points
		if (sd->status.skill_point > 0) {
			sd->status.skill_point = 0; // 0 Skill Points
			clif_updatestatus(sd, SP_SKILLPOINT); // Update
		}

	// Other Players
	} else {
		// GM All Skills (Only actual job)
		if (sd->GM_level >= battle_config.gm_all_skill_job) {
			// Remove invalid skills (Not for the actual job)
			if (!battle_config.skillfree) {
				// Remove skills that are not in correct job
				for(i = 0; i < MAX_SKILL; i++)
					if (sd->status.skill[i].flag != 13) // flag: 0 (normal), 1 (only card), 2-12 (card and skill (skill level +2)), 13 (cloneskill)
						if (sd->status.skill[i].lv > 0) {
							for(j = 0; j < MAX_SKILL_PER_TREE && (id = skill_tree[s][c][j].id) > 0; j++)
								if (i == id) {
									// Check if database was changed (reduction of maximum level)
									if (sd->status.skill[i].lv > skill_tree[s][c][j].max)
										sd->status.skill[i].lv = skill_tree[s][c][j].max;
									break;
								}
							// If not found
							if (i != id)
								sd->status.skill[i].lv = 0;
						}
			}

			// Add all (job) skills to max value
			for(i = 0; i < MAX_SKILL_PER_TREE && (id = skill_tree[s][c][i].id) > 0; i++)
				if (sd->status.skill[id].flag != 13) { // flag: 0 (normal), 1 (only card), 2-12 (card and skill (skill level +2)), 13 (cloneskill)
					if (sd->GM_level >= battle_config.gm_all_skill_platinum || !(skill_get_inf2(id) & 0x01) ||
					    (s == 1 && (id == 142 || id == 143))) { // High Novice free skills
						sd->status.skill[id].id = id;
						sd->status.skill[id].lv = skill_tree[s][c][i].max;
					} else
						sd->status.skill[id].lv = 0;
				}

			// Remove skill points
			if (sd->status.skill_point > 0) {
				sd->status.skill_point = 0; // 0 Skill points
				clif_updatestatus(sd, SP_SKILLPOINT); // Update
			}

		// If normal player
		} else {
			// Check previous job and skills
			c = pc_calc_skilltree_normalize_job(c, s, sd);

			// Remove invalid skills (not for the actual job)
			if (!battle_config.skillfree) {
				// Remove skills that are not in correct job
				for(i = 0; i < MAX_SKILL; i++)
					if (sd->status.skill[i].flag != 13) // flag: 0 (normal), 1 (only card), 2-12 (card and skill (skill level +2)), 13 (cloneskill)
						if (sd->status.skill[i].lv > 0) {
							for(j = 0; j < MAX_SKILL_PER_TREE && (id = skill_tree[s][c][j].id) > 0; j++)
								if (i == id) {
									// Check if database was changed (reduction of maximum level)
									if (sd->status.skill[i].lv > skill_tree[s][c][j].max)
										sd->status.skill[i].lv = skill_tree[s][c][j].max;
									break;
								}
							// If not found
							if (i != id) {
								if (c != s_class.job && skill_get_inf2(i) & 0x01) {	// 2nd class quest skills
									for(k = 0; k < MAX_SKILL_PER_TREE && (qid = skill_tree[s][s_class.job][k].id) > 0; k++) {
										if (i == qid) {
											sd->status.skill[i].id = qid;
											if (sd->status.skill[i].lv > skill_tree[s][s_class.job][k].max)
												sd->status.skill[i].lv = skill_tree[s][s_class.job][k].max;
											break;
										}
									}
								}
								if (i != qid)
									sd->status.skill[i].lv = 0;
							}
						}
			}

			do {
				flag = 0;
				for(i = 0; i < MAX_SKILL_PER_TREE && (id = skill_tree[s][c][i].id) > 0; i++) {
					if (sd->status.skill[id].flag != 13) {
						int f = 1, needed_id;
						// Check skill tree
						if (!battle_config.skillfree) {
							struct skill_tree_entry *s_t_ent;
							s_t_ent = &skill_tree[s][c][i];
							for(j = 0; j < 5; j++) {
								if ((needed_id = s_t_ent->need[j].id)) {
									if (sd->status.skill[needed_id].flag == 13 || sd->status.skill[needed_id].lv < s_t_ent->need[j].lv) {
										f = 0;
										break;
									}
								}
							}
							if (sd->status.job_level < s_t_ent->joblv) {
								f = 0;
							}
						}
						// Check quest skills
						if (f == 1 && // If can be known
						    !battle_config.quest_skill_learn && // Quest must be learn by quest (not by skill points)
						    sd->status.skill[id].lv == 0 && // If skill is not known
						    skill_get_inf2(id) & 0x01) // It's a quest skill
							f = 0;
						// Analyze result
						if (f) {
							if (sd->status.skill[id].id == 0) { // Check ID to avoid infinite loop
								flag = 1;
								sd->status.skill[id].id = id;
							}
						} else {
							if (sd->status.skill[id].id != 0) { // Check ID to avoid infinite loop
								flag = 1; // Need to recheck for other skills
								sd->status.skill[id].id = 0;
							}
							sd->status.skill[id].lv = 0; // Reset level for minimum skill points calculation and recheck
						}
					}
				}
			                // Check maximum number of skill points (ID must be defined to check correctly maximum skill points)
			} while(flag || pc_checkmaxskill(sd)); // Remove some skills if necessary
		}
	}

	if (sd->sc_data && sd->sc_data[SC_SPIRIT].timer != -1 && sd->sc_data[SC_SPIRIT].val2 == SL_HUNTER && pc_checkskill(sd,AC_DOUBLE) == 10 && pc_checkskill(sd,HT_POWER) == 0)
	{
		sd->status.skill[HT_POWER].id = HT_POWER;
		sd->status.skill[HT_POWER].lv = 1;
		sd->status.skill[HT_POWER].flag = 1;
	}
	
	if (sd->sc_data && sd->sc_data[SC_SPIRIT].timer != -1 && sd->sc_data[SC_SPIRIT].val2 == SL_KNIGHT && pc_checkskill(sd,KN_TWOHANDQUICKEN) == 10 && pc_checkskill(sd,KN_ONEHAND) == 0)
	{
		sd->status.skill[KN_ONEHAND].id = KN_ONEHAND;
		sd->status.skill[KN_ONEHAND].lv = 1;
		sd->status.skill[KN_ONEHAND].flag = 1;
	}

	if (sd->sc_data && sd->sc_data[SC_SPIRIT].timer != -1 && sd->sc_data[SC_SPIRIT].val2 == SL_BLACKSMITH && pc_checkskill(sd,BS_ADRENALINE) >= 5 && pc_checkskill(sd,BS_ADRENALINE2) == 0)
	{
		sd->status.skill[BS_ADRENALINE2].id = BS_ADRENALINE2;
		sd->status.skill[BS_ADRENALINE2].lv = 1;
		sd->status.skill[BS_ADRENALINE2].flag = 1;
	}

	if (sd->sc_data && sd->sc_data[SC_SPIRIT].timer != -1 && sd->sc_data[SC_SPIRIT].val2 == SL_ALCHEMIST && pc_checkskill(sd,AM_BERSERKPITCHER) == 0)
	{
		sd->status.skill[AM_BERSERKPITCHER].id = AM_BERSERKPITCHER;
		sd->status.skill[AM_BERSERKPITCHER].lv = 1;
		sd->status.skill[AM_BERSERKPITCHER].flag = 1;
		
		if (pc_checkskill(sd,AM_PHARMACY) >= 10)
		{
			sd->status.skill[AM_TWILIGHT1].id = AM_TWILIGHT1;
			sd->status.skill[AM_TWILIGHT1].lv = 1;
			sd->status.skill[AM_TWILIGHT1].flag = 1;
		}
	}

	pc_checkminskill(sd);

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
int pc_calc_skilltree_normalize_job(int c, int s, struct map_session_data *sd) {

	// Check skills up limit
	if (battle_config.skillup_limit) {
		int skill_point, i, id;
		// Calculation of basic skill and check novice skill points
		skill_point = sd->status.skill[NV_BASIC].lv;
		if (battle_config.quest_skill_learn && s != 1) // If platinum skills are not free
			skill_point = skill_point + sd->status.skill[142].lv + sd->status.skill[143].lv; // Add the 2 novice quests skills

		if (skill_point < 9) // If not all basic skills
			return 0;

		// Check second classes and first class skills
		if ((c >= 7 && c <= 21) || (c >= 4046 && c <= 4049)) {
			int c1 = c, previous_class_level;
			// Which class to check
			switch(c) {
			case 7:
			case 13:
			case 14:
			case 21:
				c1 = 1;
				break;
			case 8:
			case 15:
				c1 = 4;
				break;
			case 9:
			case 16:
				c1 = 2;
				break;
			case 10:
			case 18:
				c1 = 5;
				break;
			case 11:
			case 19:
			case 20:
				c1 = 3;
				break;
			case 12:
			case 17:
				c1 = 6;
				break;
			case 4046:
			case 4047:
			case 4048:
			case 4049:
				c1 = 4046;
			}
			// Totalize actual skills points
			for(i = 0; (id = skill_tree[s][c1][i].id) > 0; i++) {
				if (id == NV_BASIC || id == 142 || id == 143) // Already added
					continue;
				if (sd->status.skill[id].flag != 13 && (battle_config.quest_skill_learn || !(skill_get_inf2(id) & 0x01)))
					skill_point += sd->status.skill[id].lv;
			}
			// Calculate skill points for 1st class and check
			previous_class_level = sd->change_level;
			if (previous_class_level < 40 || previous_class_level > 50)
				previous_class_level = 40;
			if (skill_point < 9 + (previous_class_level - 1))
				return c1;
		}
	}

	return c;
}

/*==========================================
 *
 *------------------------------------------
 */
void pc_checkweighticon(struct map_session_data *sd) {

	int flag = 0;

	nullpo_retv(sd);

	if (sd->weight * 2 >= sd->max_weight)
		flag = 1;
	if (sd->weight * 10 >= sd->max_weight*9)
		flag = 2;

	if (flag == 1) {
		if (sd->sc_data[SC_WEIGHT50].timer == -1)
			status_change_start(&sd->bl, SC_WEIGHT50, 0, 0, 0, 0, 0, 0);
	} else {
		status_change_end(&sd->bl, SC_WEIGHT50, -1);
	}
	if (flag == 2) {
		if (sd->sc_data[SC_WEIGHT90].timer == -1)
			status_change_start(&sd->bl, SC_WEIGHT90, 0 , 0 , 0 , 0 , 0 , 0);
	} else
		status_change_end(&sd->bl, SC_WEIGHT90, -1);

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void pc_bonus(struct map_session_data *sd, int type, int val) {

	nullpo_retv(sd);

	switch(type) {
	case SP_STR:
	case SP_AGI:
	case SP_VIT:
	case SP_INT:
	case SP_DEX:
	case SP_LUK:
		if (sd->state.lr_flag != 2)
			sd->parame[type-SP_STR]+=val;
		break;
	case SP_ATK1:
		if (!sd->state.lr_flag)
			sd->watk+=val;
		else if(sd->state.lr_flag == 1)
			sd->watk_+=val;
		break;
	case SP_ATK2:
		if (!sd->state.lr_flag)
			sd->watk2+=val;
		else if(sd->state.lr_flag == 1)
			sd->watk_2+=val;
		break;
	case SP_BASE_ATK:
		if (sd->state.lr_flag != 2)
			sd->base_atk+=val;
		break;
	case SP_MATK1:
		if (sd->state.lr_flag != 2)
			sd->matk1 += val;
		break;
	case SP_MATK2:
		if (sd->state.lr_flag != 2)
			sd->matk2 += val;
		break;
	case SP_MATK:
		if (sd->state.lr_flag != 2) {
			sd->matk1 += val;
			sd->matk2 += val;
		}
		break;
	case SP_DEF1:
		if (sd->state.lr_flag != 2)
			sd->def+=val;
		break;
	case SP_MDEF1:
		if (sd->state.lr_flag != 2)
			sd->mdef+=val;
		break;
	case SP_MDEF2:
		if (sd->state.lr_flag != 2)
			sd->mdef+=val;
		break;
	case SP_HIT:
		if (sd->state.lr_flag != 2)
			sd->hit+=val;
		else
			sd->arrow_hit+=val;
		break;
	case SP_FLEE1:
		if (sd->state.lr_flag != 2)
			sd->flee+=val;
		break;
	case SP_FLEE2:
		if (sd->state.lr_flag != 2)
			sd->flee2+=val*10;
		break;
	case SP_CRITICAL:
		if (sd->state.lr_flag != 2)
			sd->critical+=val*10;
		else
			sd->arrow_cri += val*10;
		break;
	case SP_ATKELE:
		if (!sd->state.lr_flag)
			sd->atk_ele=val;
		else if(sd->state.lr_flag == 1)
			sd->atk_ele_=val;
		else if(sd->state.lr_flag == 2)
			sd->arrow_ele=val;
		break;
	case SP_DEFELE:
		if (sd->state.lr_flag != 2)
			sd->def_ele=val;
		break;
	case SP_MAXHP:
		if (sd->state.lr_flag != 2)
			sd->status.max_hp+=val;
		break;
	case SP_MAXSP:
		if (sd->state.lr_flag != 2)
			sd->status.max_sp+=val;
		break;
	case SP_CASTRATE:
		if (sd->state.lr_flag != 2)
			sd->castrate+=val;
		break;
	case SP_MAXHPRATE:
		if (sd->state.lr_flag != 2)
			sd->hprate+=val;
		break;
	case SP_MAXSPRATE:
		if (sd->state.lr_flag != 2)
			sd->sprate += val;
		break;
	case SP_SPRATE:
		if (sd->state.lr_flag != 2)
			sd->dsprate += val;
		break;
	case SP_ATTACKRANGE:
		if (!sd->state.lr_flag)
			sd->attackrange += val;
		else if(sd->state.lr_flag == 1)
			sd->attackrange_ += val;
		else if(sd->state.lr_flag == 2)
			sd->arrow_range += val;
		break;
	case SP_ADD_SPEED:
		if (sd->state.lr_flag != 2)
			sd->speed -= val;
		break;
	case SP_SPEED_RATE:
		if (sd->state.lr_flag != 2) {
			if (sd->speed_rate > 100-val)
				sd->speed_rate = 100-val;
		}
		break;
	case SP_SPEED_ADDRATE:
		if (sd->state.lr_flag != 2)
			sd->speed_add_rate = sd->speed_add_rate * (100-val)/100;
		break;
	case SP_ASPD:
		if (sd->state.lr_flag != 2)
			sd->aspd -= val*10;
		break;
	case SP_ASPD_RATE:
		if (sd->state.lr_flag != 2) {
			if (sd->aspd_rate > 100-val)
				sd->aspd_rate = 100-val;
		}
		break;
	case SP_ASPD_ADDRATE:
		if (sd->state.lr_flag != 2)
			sd->aspd_add_rate = sd->aspd_add_rate * (100-val)/100;
		break;
	case SP_HP_RECOV_RATE:
		if (sd->state.lr_flag != 2)
			sd->hprecov_rate += val;
		break;
	case SP_SP_RECOV_RATE:
		if (sd->state.lr_flag != 2)
			sd->sprecov_rate += val;
		break;
	case SP_CRITICAL_DEF:
		if (sd->state.lr_flag != 2)
			sd->critical_def += val;
		break;
	case SP_NEAR_ATK_DEF:
		if (sd->state.lr_flag != 2)
			sd->near_attack_def_rate += val;
		break;
	case SP_LONG_ATK_DEF:
		if (sd->state.lr_flag != 2)
			sd->long_attack_def_rate += val;
		break;
	case SP_DOUBLE_RATE:
		if (sd->state.lr_flag == 0 && sd->double_rate < val)
			sd->double_rate = val;
		break;
	case SP_DOUBLE_ADD_RATE:
		if (sd->state.lr_flag == 0)
			sd->double_add_rate += val;
		break;
	case SP_MATK_RATE:
		if (sd->state.lr_flag != 2)
			sd->matk_rate += val;
		break;
	case SP_IGNORE_DEF_ELE:
		if (!sd->state.lr_flag)
			sd->ignore_def_ele |= 1<<val;
		else if(sd->state.lr_flag == 1)
			sd->ignore_def_ele_ |= 1<<val;
		break;
	case SP_IGNORE_DEF_RACE:
		if (!sd->state.lr_flag)
			sd->ignore_def_race |= 1<<val;
		else if(sd->state.lr_flag == 1)
			sd->ignore_def_race_ |= 1<<val;
		break;
	case SP_ATK_RATE:
		if (sd->state.lr_flag != 2)
			sd->atk_rate += val;
		break;
	case SP_MAGIC_ATK_DEF:
		if (sd->state.lr_flag != 2)
			sd->magic_def_rate += val;
		break;
	case SP_MISC_ATK_DEF:
		if (sd->state.lr_flag != 2)
			sd->misc_def_rate += val;
		break;
	case SP_IGNORE_MDEF_ELE:
		if (sd->state.lr_flag != 2)
			sd->ignore_mdef_ele |= 1<<val;
		break;
	case SP_IGNORE_MDEF_RACE:
		if (sd->state.lr_flag != 2)
			sd->ignore_mdef_race |= 1<<val;
		break;
	case SP_PERFECT_HIT_RATE:
		if (sd->state.lr_flag != 2 && sd->perfect_hit < val)
			sd->perfect_hit = val;
		break;
	case SP_PERFECT_HIT_ADD_RATE:
		if (sd->state.lr_flag != 2)
			sd->perfect_hit_add += val;
		break;
	case SP_CRITICAL_RATE:
		if (sd->state.lr_flag != 2)
			sd->critical_rate+=val;
		break;
	case SP_DEF_RATIO_ATK_ELE:
		if (!sd->state.lr_flag)
			sd->def_ratio_atk_ele |= 1<<val;
		else if(sd->state.lr_flag == 1)
			sd->def_ratio_atk_ele_ |= 1<<val;
		break;
	case SP_DEF_RATIO_ATK_RACE:
		if (!sd->state.lr_flag)
			sd->def_ratio_atk_race |= 1<<val;
		else if(sd->state.lr_flag == 1)
			sd->def_ratio_atk_race_ |= 1<<val;
		break;
	case SP_HIT_RATE:
		if (sd->state.lr_flag != 2)
			sd->hit_rate += val;
		break;
	case SP_FLEE_RATE:
		if (sd->state.lr_flag != 2)
			sd->flee_rate += val;
		break;
	case SP_FLEE2_RATE:
		if (sd->state.lr_flag != 2)
			sd->flee2_rate += val;
		break;
	case SP_DEF_RATE:
		if (sd->state.lr_flag != 2)
			sd->def_rate += val;
		break;
	case SP_DEF2_RATE:
		if (sd->state.lr_flag != 2)
			sd->def2_rate += val;
		break;
	case SP_MDEF_RATE:
		if (sd->state.lr_flag != 2)
			sd->mdef_rate += val;
		break;
	case SP_MDEF2_RATE:
		if (sd->state.lr_flag != 2)
			sd->mdef2_rate += val;
		break;
	case SP_RESTART_FULL_RECORVER:
		if (sd->state.lr_flag != 2)
			sd->special_state.restart_full_recover = 1;
		break;
	case SP_NO_CASTCANCEL:
		if (sd->state.lr_flag != 2)
			sd->special_state.no_castcancel = 1;
		break;
	case SP_NO_CASTCANCEL2:
		if (sd->state.lr_flag != 2)
			sd->special_state.no_castcancel2 = 1;
		break;
	case SP_NO_SIZEFIX:
		if (sd->state.lr_flag != 2)
			sd->special_state.no_sizefix = 1;
		break;
	case SP_NO_MAGIC_DAMAGE:
		if (sd->state.lr_flag != 2)
			sd->special_state.no_magic_damage = 1;
		break;
	case SP_NO_WEAPON_DAMAGE:
		if (sd->state.lr_flag != 2)
			sd->special_state.no_weapon_damage = 1;
		break;
	case SP_NO_GEMSTONE:
		if (sd->state.lr_flag != 2)
			sd->special_state.no_gemstone = 1;
		break;
	case SP_INFINITE_ENDURE:
		if (sd->state.lr_flag != 2)
			sd->special_state.infinite_endure = 1;
		break;
	case SP_UNBREAKABLE_WEAPON:
		if (sd->state.lr_flag != 2)
			sd->unbreakable_equip |= EQP_WEAPON;
		break;
	case SP_UNBREAKABLE_ARMOR:
		if (sd->state.lr_flag != 2)
			sd->unbreakable_equip |= EQP_ARMOR;
		break;
	case SP_UNBREAKABLE_HELM:
		if (sd->state.lr_flag != 2)
			sd->unbreakable_equip |= EQP_HELM;
		break;
	case SP_UNBREAKABLE_SHIELD:
		if (sd->state.lr_flag != 2)
			sd->unbreakable_equip |= EQP_SHIELD;
		break;
	case SP_SPLASH_RANGE:
		if (sd->state.lr_flag != 2 && sd->splash_range < val)
			sd->splash_range = val;
		break;
	case SP_SPLASH_ADD_RANGE:
		if (sd->state.lr_flag != 2)
			sd->splash_add_range += val;
		break;
	case SP_SHORT_WEAPON_DAMAGE_RETURN:
		if (sd->state.lr_flag != 2)
			sd->short_weapon_damage_return += val;
		break;
	case SP_LONG_WEAPON_DAMAGE_RETURN:
		if (sd->state.lr_flag != 2)
			sd->long_weapon_damage_return += val;
		break;
	case SP_MAGIC_DAMAGE_RETURN:
		if (sd->state.lr_flag != 2)
			sd->magic_damage_return += val;
		break;
	case SP_ALL_STATS:
		if (sd->state.lr_flag!=2) {
			sd->parame[SP_STR-SP_STR] += val;
			sd->parame[SP_AGI-SP_STR] += val;
			sd->parame[SP_VIT-SP_STR] += val;
			sd->parame[SP_INT-SP_STR] += val;
			sd->parame[SP_DEX-SP_STR] += val;
			sd->parame[SP_LUK-SP_STR] += val;
		}
		break;
	case SP_AGI_VIT:
		if (sd->state.lr_flag != 2) {
			sd->parame[SP_AGI-SP_STR] += val;
			sd->parame[SP_VIT-SP_STR] += val;
		}
		break;
	case SP_AGI_DEX_STR:
		if (sd->state.lr_flag != 2) {
			sd->parame[SP_AGI-SP_STR] += val;
			sd->parame[SP_DEX-SP_STR] += val;
			sd->parame[SP_STR-SP_STR] += val;
		}
		break;
	case SP_PERFECT_HIDE:
		if (sd->state.lr_flag != 2) {
			sd->perfect_hiding = 1;
		}
		break;
	case SP_DISGUISE:
		if (sd->state.lr_flag != 2 && sd->disguiseflag == 0) {
			if (pc_isriding(sd)) {
				clif_displaymessage(sd->fd, msg_txt(529));
				break;
			}
			sd->disguise = val;
			clif_clearchar(&sd->bl, 9);
			pc_setpos(sd, sd->mapname, sd->bl.x, sd->bl.y, 3, 1);
		}
		break;
	case SP_UNBREAKABLE:
		if (sd->state.lr_flag!=2) {
			sd->unbreakable += val;
		}
		break;
	case SP_CLASSCHANGE:
		if (sd->state.lr_flag != 2) {
			sd->classchange = val;
		}
		break;
	case SP_LONG_ATK_RATE:
		if (sd->status.weapon == 11 && sd->state.lr_flag != 2)
			sd->atk_rate += val;
		break;
	case SP_BREAK_WEAPON_RATE:
		if (sd->state.lr_flag != 2)
			sd->break_weapon_rate += val;
		break;
	case SP_BREAK_ARMOR_RATE:
		if (sd->state.lr_flag != 2)
			sd->break_armor_rate += val;
		break;
	case SP_ADD_STEAL_RATE:
		if (sd->state.lr_flag != 2)
			sd->add_steal_rate += val;
		break;
	case SP_DELAYRATE:
		if (sd->state.lr_flag != 2)
			sd->delayrate += val;
		break;
	case SP_CRIT_ATK_RATE:
		if (sd->state.lr_flag != 2)
			sd->crit_atk_rate += val;
		break;
	case SP_NO_REGEN:
		if (sd->state.lr_flag != 2)
			sd->no_regen = val;
		break;
	case SP_UNSTRIPABLE:
		if (sd->state.lr_flag != 2)
			sd->unstripable_equip |= EQP_ARMOR;
		break;
	case SP_HP_DRAIN_VALUE:
		if (!sd->state.lr_flag) {
			sd->hp_drain_value += val;
		}
		else if(sd->state.lr_flag == 1) {
			sd->hp_drain_value_ += val;
		}
		break;
	case SP_SP_DRAIN_VALUE:
		if (!sd->state.lr_flag) {
			sd->sp_drain_value += val;
		}
		else if(sd->state.lr_flag == 1) {
			sd->sp_drain_value_ += val;
		}
		break;
	case SP_SP_GAIN_VALUE:
		if (!sd->state.lr_flag)
			sd->sp_gain_value += val;
		break;
	case SP_IGNORE_DEF_MOB:
		if (!sd->state.lr_flag)
			sd->ignore_def_mob |= 1 << val;
		else if (sd->state.lr_flag == 1)
			sd->ignore_def_mob_ |= 1 << val;
		break;
	case SP_HP_GAIN_VALUE:
		if (!sd->state.lr_flag)
			sd->hp_gain_value += val;
		break;
	case SP_DAMAGE_WHEN_UNEQUIP:
		if (!sd->state.lr_flag)
			sd->unequip_hpdamage += val;
		break;
	case SP_LOSESP_WHEN_UNEQUIP:
		if (!sd->state.lr_flag)
			sd->unequip_spdamage += val;
		break;
	case SP_INTRAVISION:
		if (sd->state.lr_flag != 2)
			sd->special_state.intravision = 1;
		break;
	case SP_NOKNOCKBACK:
		if (sd->state.lr_flag != 2)
			sd->special_state.noknockback = 1;
		break;
	default:
		if (battle_config.error_log)
			printf("pc_bonus: unknown type %d %d !\n", type, val);
		break;
	}

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void pc_bonus2(struct map_session_data *sd, int type, int type2, int val) {

	int i;

	nullpo_retv(sd);

	switch(type) {
	case SP_ADDELE:
		if (!sd->state.lr_flag)
			sd->addele[type2]+=val;
		else if(sd->state.lr_flag == 1)
			sd->addele_[type2]+=val;
		else if(sd->state.lr_flag == 2)
			sd->arrow_addele[type2]+=val;
		break;
	case SP_ADDRACE:
		if (!sd->state.lr_flag)
			sd->addrace[type2]+=val;
		else if(sd->state.lr_flag == 1)
			sd->addrace_[type2]+=val;
		else if(sd->state.lr_flag == 2)
			sd->arrow_addrace[type2]+=val;
		break;
	case SP_ADDSIZE:
		if (!sd->state.lr_flag)
			sd->addsize[type2]+=val;
		else if(sd->state.lr_flag == 1)
			sd->addsize_[type2]+=val;
		else if(sd->state.lr_flag == 2)
			sd->arrow_addsize[type2]+=val;
		break;
	case SP_SUBELE:
		if (sd->state.lr_flag != 2)
			sd->subele[type2]+=val;
		break;
	case SP_SUBRACE:
		if (sd->state.lr_flag != 2)
			sd->subrace[type2]+=val;
		break;
	case SP_ADDEFF:
		if (sd->state.lr_flag != 2)
			sd->addeff[type2]+=val;
		else
			sd->arrow_addeff[type2]+=val;
		break;
	case SP_ADDEFF2:
		if (sd->state.lr_flag != 2)
			sd->addeff2[type2]+=val;
		else
			sd->arrow_addeff2[type2]+=val;
		break;
	case SP_RESEFF:
		if (sd->state.lr_flag != 2)
			sd->reseff[type2]+=val;
		break;
	case SP_MAGIC_ADDELE:
		if (sd->state.lr_flag != 2)
			sd->magic_addele[type2]+=val;
		break;
	case SP_MAGIC_ADDRACE:
		if (sd->state.lr_flag != 2)
			sd->magic_addrace[type2]+=val;
		break;
	case SP_MAGIC_SUBRACE:
		if (sd->state.lr_flag != 2)
			sd->magic_subrace[type2]+=val;
		break;
	case SP_ADD_DAMAGE_CLASS:
		if (!sd->state.lr_flag) {
			for(i=0;i<sd->add_damage_class_count && i < 10; i++) {
				if (sd->add_damage_classid[i] == type2) {
					sd->add_damage_classrate[i] += val;
					break;
				}
			}
			if (i >= sd->add_damage_class_count && sd->add_damage_class_count < 10) {
				sd->add_damage_classid[sd->add_damage_class_count] = type2;
				sd->add_damage_classrate[sd->add_damage_class_count] += val;
				sd->add_damage_class_count++;
			}
		}
		else if(sd->state.lr_flag == 1) {
			for(i=0;i<sd->add_damage_class_count_ && i < 10;i++) {
				if (sd->add_damage_classid_[i] == type2) {
					sd->add_damage_classrate_[i] += val;
					break;
				}
			}
			if (i >= sd->add_damage_class_count_ && sd->add_damage_class_count_ < 10) {
				sd->add_damage_classid_[sd->add_damage_class_count_] = type2;
				sd->add_damage_classrate_[sd->add_damage_class_count_] += val;
				sd->add_damage_class_count_++;
			}
		}
		break;
	case SP_ADD_MAGIC_DAMAGE_CLASS:
		if (sd->state.lr_flag != 2) {
			for(i=0;i<sd->add_magic_damage_class_count && i < 10; i++) {
				if (sd->add_magic_damage_classid[i] == type2) {
					sd->add_magic_damage_classrate[i] += val;
					break;
				}
			}
			if (i >= sd->add_magic_damage_class_count && sd->add_magic_damage_class_count < 10) {
				sd->add_magic_damage_classid[sd->add_magic_damage_class_count] = type2;
				sd->add_magic_damage_classrate[sd->add_magic_damage_class_count] += val;
				sd->add_magic_damage_class_count++;
			}
		}
		break;
	case SP_ADD_DEF_CLASS:
		if (sd->state.lr_flag != 2) {
			for(i=0;i<sd->add_def_class_count && i < 10;i++) {
				if (sd->add_def_classid[i] == type2) {
					sd->add_def_classrate[i] += val;
					break;
				}
			}
			if (i >= sd->add_def_class_count && sd->add_def_class_count < 10) {
				sd->add_def_classid[sd->add_def_class_count] = type2;
				sd->add_def_classrate[sd->add_def_class_count] += val;
				sd->add_def_class_count++;
			}
		}
		break;
	case SP_ADD_MDEF_CLASS:
		if (sd->state.lr_flag != 2) {
			for(i=0;i<sd->add_mdef_class_count;i++) {
				if (sd->add_mdef_classid[i] == type2) {
					sd->add_mdef_classrate[i] += val;
					break;
				}
			}
			if (i >= sd->add_mdef_class_count && sd->add_mdef_class_count < 10) {
				sd->add_mdef_classid[sd->add_mdef_class_count] = type2;
				sd->add_mdef_classrate[sd->add_mdef_class_count] += val;
				sd->add_mdef_class_count++;
			}
		}
		break;
	case SP_HP_DRAIN_RATE:
		if (!sd->state.lr_flag) {
			sd->hp_drain_rate += type2;
			sd->hp_drain_per += val;
		}
		else if(sd->state.lr_flag == 1) {
			sd->hp_drain_rate_ += type2;
			sd->hp_drain_per_ += val;
		}
		break;
	case SP_HP_DRAIN_VALUE:
		if (!sd->state.lr_flag)
			sd->hp_drain_value += type2;
		else if(sd->state.lr_flag == 1)
			sd->hp_drain_value_ += type2;
		sd->sp_drain_type = val;
		break;
	case SP_SP_DRAIN_RATE:
		if (!sd->state.lr_flag) {
			sd->sp_drain_rate += type2;
			sd->sp_drain_per += val;
		}
		else if(sd->state.lr_flag == 1) {
			sd->sp_drain_rate_ += type2;
			sd->sp_drain_per_ += val;
		}
		sd->sp_drain_type = 0;
		break;
	case SP_SP_DRAIN_VALUE:
		if (!sd->state.lr_flag)
			sd->sp_drain_value += type2;
		else if(sd->state.lr_flag == 1)
			sd->sp_drain_value_ += type2;
		sd->sp_drain_type = val;
		break;
	case SP_SP_GAIN_VALUE_RACE:
		if (sd->state.lr_flag != 2)
			sd->sp_gain_value_race[type2] += val;
		break;
	case SP_WEAPON_COMA_ELE:
		if (sd->state.lr_flag != 2)
			sd->weapon_coma_ele[type2] += val;
		break;
	case SP_WEAPON_COMA_RACE:
		if (sd->state.lr_flag != 2)
			sd->weapon_coma_race[type2] += val;
		break;
	case SP_GET_ZENY_NUM:
		if (sd->state.lr_flag != 2 && sd->get_zeny_rate < val) {
			sd->get_zeny_rate = val;
			sd->get_zeny_num = type2;
		}
		break;
	case SP_ADD_GET_ZENY_NUM:
		if (sd->state.lr_flag != 2) {
			sd->get_zeny_rate += val;
			sd->get_zeny_num += type2;
		}
		break;
	case SP_RANDOM_ATTACK_INCREASE:
		if (sd->state.lr_flag !=2) {
			sd->random_attack_increase_add = type2;
			sd->random_attack_increase_per += val;
		}
		break;
	case SP_WEAPON_ATK:
		if (sd->state.lr_flag != 2)
			sd->weapon_atk[type2] += val;
		break;
	case SP_WEAPON_ATK_RATE:
		if (sd->state.lr_flag != 2)
			sd->weapon_atk_rate[type2] += val;
		break;
	case SP_CRITICAL_ADDRACE:
		if (sd->state.lr_flag != 2)
			sd->critaddrace[type2] += val;
		break;
	case SP_ADDEFF_WHENHIT:
		if (sd->state.lr_flag != 2)
			sd->addeff3[type2] += val;
		break;
	case SP_SKILL_ATK:
		if (sd->state.lr_flag != 2) {
			for (i = 0; i < MAX_PC_BONUS; i++) {
				if (sd->skillatk[i].id == 0) {
					sd->skillatk[i].id = type2;
					sd->skillatk[i].val = val;
					break;
				}
				else if (sd->skillatk[i].id == type2) {
					sd->skillatk[i].val += val;
					break;
				}
			}
		}
		break;
	case SP_ADD_DAMAGE_BY_CLASS:
		if (sd->state.lr_flag != 2) {
			for(i = 0; i < sd->add_damage_class_count2 && i < 10; i++) {
				if (sd->add_damage_classid2[i] == type2) {
					sd->add_damage_classrate2[i] += val;
					break;
				}
			}
			if (i >= sd->add_damage_class_count2 && sd->add_damage_class_count2 < 10) {
				sd->add_damage_classid2[sd->add_damage_class_count2] = type2;
				sd->add_damage_classrate2[sd->add_damage_class_count2] += val;
				sd->add_damage_class_count2++;
			}
		}
		break;
	case SP_HP_LOSS_RATE:
		if (sd->state.lr_flag != 2) {
			sd->hp_loss_value = type2;
			sd->hp_loss_rate = val;
		}
		break;
	case SP_ADDRACE2:
		if (type2 <= 0 || type2 >= MAX_MOB_RACE_DB)
			break;
		if (sd->state.lr_flag != 2)
			sd->addrace2[type2] += val;
		else
			sd->addrace2_[type2] += val;
		break;
	case SP_SUBSIZE:
		if (sd->state.lr_flag != 2)
			sd->subsize[type2] += val;
		break;
	case SP_ADD_ITEM_HEAL_RATE:
		if (sd->state.lr_flag != 2)
			sd->itemhealrate[type2 - 1] += val;
		break;
	case SP_SP_GAIN_RACE:
		if (sd->state.lr_flag != 2)
			sd->sp_gain_race[type2] += val;
		break;
	case SP_EXP_ADDRACE:
		if (sd->state.lr_flag != 2)
			sd->expaddrace[type2] += val;
		break;
	case SP_ADD_MONSTER_DROP_ITEM:
		if (sd->state.lr_flag != 2) {
			for(i = 0; i < sd->monster_drop_item_count && i < MAX_PC_BONUS; i++) {
				if (sd->monster_drop[i].id == type2) {
					sd->monster_drop[i].race |= (1<<10)|(1<<11);
					if (sd->monster_drop[i].rate < val)
						sd->monster_drop[i].rate = val;
					break;
				}
			}
			if (i >= sd->monster_drop_item_count && sd->monster_drop_item_count < MAX_PC_BONUS) {
				sd->monster_drop[sd->monster_drop_item_count].id = type2;
				sd->monster_drop[sd->monster_drop_item_count].race |= (1<<10)|(1<<11);
				sd->monster_drop[sd->monster_drop_item_count].rate = val;
				sd->monster_drop_item_count++;
			}
		}
		break;
	case SP_ADD_MONSTER_DROP_ITEMGROUP:
		if (sd->state.lr_flag != 2) {
			for(i = 0; i < sd->monster_drop_item_count && i < MAX_PC_BONUS; i++) {
				if (sd->monster_drop[i].group == type2) {
					sd->monster_drop[i].id = 0;
					sd->monster_drop[i].race |= (1<<10)|(1<<11);
					if (sd->monster_drop[i].rate < val)
						sd->monster_drop[i].rate = val;
					break;
				}
			}
			if (i >= sd->monster_drop_item_count && sd->monster_drop_item_count < MAX_PC_BONUS) {
				sd->monster_drop[sd->monster_drop_item_count].group = type2;
				sd->monster_drop[sd->monster_drop_item_count].id = 0;
				sd->monster_drop[sd->monster_drop_item_count].race |= (1<<10)|(1<<11);
				sd->monster_drop[sd->monster_drop_item_count].rate = val;
				sd->monster_drop_item_count++;
			}
		}
		break;
	case SP_SP_VANISH_RATE:
		if (sd->state.lr_flag != 2) {
			sd->sp_vanish_rate += type2;
			sd->sp_vanish_per += val;
		}
		break;
	default:
		if (battle_config.error_log)
			printf("pc_bonus2: unknown type %d %d %d!\n", type, type2, val);
		break;
	}

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void pc_bonus3(struct map_session_data *sd, int type, int type2, int type3, int val) {

	int i;
	switch(type) {
	case SP_ADD_MONSTER_DROP_ITEM:
		if (sd->state.lr_flag != 2) {
			for(i = 0; i < sd->monster_drop_item_count && i < MAX_PC_BONUS; i++) {
				if (sd->monster_drop[i].id == type2) {
					sd->monster_drop[i].race |= 1 << type3;
					if (sd->monster_drop[i].rate < val)
						sd->monster_drop[i].rate = val;
					break;
				}
			}
			if (i >= sd->monster_drop_item_count && sd->monster_drop_item_count < MAX_PC_BONUS) {
				sd->monster_drop[sd->monster_drop_item_count].id = type2;
				sd->monster_drop[sd->monster_drop_item_count].race |= 1 << type3;
				sd->monster_drop[sd->monster_drop_item_count].rate = val;
				sd->monster_drop_item_count++;
			}
		}
		break;
	case SP_HP_LOSS_RATE:
		if (sd->state.lr_flag != 2) {
			sd->hp_loss_value = type2;
			sd->hp_loss_rate = type3;
			sd->hp_loss_type = val;
		}
		break;
	case SP_SP_DRAIN_RATE:
		if (!sd->state.lr_flag) {
			sd->sp_drain_rate += type2;
			sd->sp_drain_per += type3;
		} else if (sd->state.lr_flag == 1) {
			sd->sp_drain_rate_ += type2;
			sd->sp_drain_per_ += type3;
		}
		sd->sp_drain_type = val;
		break;
	case SP_ADD_MONSTER_DROP_ITEMGROUP:
		if (sd->state.lr_flag != 2) {
			for(i = 0; i < sd->monster_drop_item_count && i < MAX_PC_BONUS; i++) {
				if (sd->monster_drop[i].group == type2) {
					sd->monster_drop[i].id = 0;
					sd->monster_drop[i].race |= 1<<type3;
					if (sd->monster_drop[i].rate < val)
						sd->monster_drop[i].rate = val;
					break;
				}
			}
			if (i >= sd->monster_drop_item_count && sd->monster_drop_item_count < MAX_PC_BONUS) {
				sd->monster_drop[sd->monster_drop_item_count].group = type2;
				sd->monster_drop[sd->monster_drop_item_count].id = 0;
				sd->monster_drop[sd->monster_drop_item_count].race |= 1<<type3;
				sd->monster_drop[sd->monster_drop_item_count].rate = val;
				sd->monster_drop_item_count++;
			}
		}
		break;

	default:
		if (battle_config.error_log)
			printf("pc_bonus3: unknown type %d %d %d %d!\n", type, type2, type3, val);
		break;
	}

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void pc_bonus4(struct map_session_data *sd, int type, int type2, int type3, int type4, int val) {

	int n;

	nullpo_retv(sd);

	switch(type) {
	case SP_AUTOSPELL:
		if (sd->state.lr_flag != 2) {
			for (n = 0; n < MAX_PC_BONUS; n++) {
				if (sd->autospell[n].id == 0 || sd->autospell[n].id == type2) {
					if (sd->autospell[n].lv > type3 || (sd->autospell[n].lv == type3 && sd->autospell[n].rate >= type4))
						break;
					sd->autospell[n].id = type2;
					sd->autospell[n].lv = type3;
					sd->autospell[n].rate = type4;
					sd->autospell[n].type = val;
					break;
				}
			}
		}
		break;
	case SP_AUTOSPELL_WHENHIT:
		if (sd->state.lr_flag != 2) {
			for (n = 0; n < MAX_PC_BONUS; n++) {
				if (sd->autospell2[n].id == 0 || sd->autospell2[n].id == type2) {
					if (sd->autospell2[n].lv > type3 || (sd->autospell2[n].lv == type3 && sd->autospell2[n].rate >= type4))
						break;
					sd->autospell2[n].id = type2;
					sd->autospell2[n].lv = type3;
					sd->autospell2[n].rate = type4;
					sd->autospell2[n].type = val;
					break;
				}
			}
		}
		break;
	case SP_AUTOSPELL_WHENHITMAGIC:
		if (sd->state.lr_flag != 2) {
			for (n = 0; n < MAX_PC_BONUS; n++) {
				if (sd->autospell3[n].id == 0 || sd->autospell3[n].id == type2) {
					if (sd->autospell3[n].lv > type3 || (sd->autospell3[n].lv == type3 && sd->autospell3[n].rate >= type4))
						break;
					sd->autospell3[n].id = type2;
					sd->autospell3[n].lv = type3;
					sd->autospell3[n].rate = type4;
					sd->autospell3[n].type = val;
					break;
				}
			}
		}
		break;
	default:
		if (battle_config.error_log)
			printf("pc_bonus4: unknown type %d %d %d %d %d!\n", type, type2, type3, type4, val);
		break;
	}

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
int pc_skill(struct map_session_data *sd, int id, int level, int flag) {

	nullpo_retr(0, sd);

	if (level > MAX_SKILL_LEVEL) {
		if (battle_config.error_log)
			printf("support card skill only!\n");
		return 0;
	}

	if (!flag) {
		// Check card
		if (sd->status.skill[id].flag == 1) { // Not known before
			if (sd->status.skill[id].lv <= level) {
				sd->status.skill[id].lv = level;
				sd->status.skill[id].flag = 0; // flag: 0 (normal), 1 (only card), 2-12 (card and skill (skill level +2)), 13 (cloneskill)
			} else {
				sd->status.skill[id].flag = level + 2;
			}
		} else if (sd->status.skill[id].flag >= 2 && sd->status.skill[id].flag != 13) { // Card, but skill known before
			if (sd->status.skill[id].lv <= level) {
				sd->status.skill[id].lv = level;
				sd->status.skill[id].flag = 0; // flag: 0 (normal), 1 (only card), 2-12 (card and skill (skill level +2)), 13 (cloneskill)
			} else {
				sd->status.skill[id].flag = level + 2;
			}
		// Skill with no card
		} else // Check here for clone skill (not yet done)
			sd->status.skill[id].lv = level;

		if (sd->status.skill[id].lv)
			sd->status.skill[id].id = id;
		status_calc_pc(sd, 0);
		clif_skillinfoblock(sd);

	} else if (flag == 2) {
		// check card
		if (sd->status.skill[id].flag == 1) { // Not known before (add from nothing)
			if (sd->status.skill[id].lv <= level) {
				sd->status.skill[id].lv = level;
				sd->status.skill[id].flag = 0; // flag: 0 (normal), 1 (only card), 2-12 (card and skill (skill level +2)), 13 (cloneskill)
			} else {
				sd->status.skill[id].flag = level + 2;
			}
		} else if (sd->status.skill[id].flag >= 2 && sd->status.skill[id].flag != 13) { // Card, but skill known before
			if (sd->status.skill[id].lv <= (sd->status.skill[id].flag - 2) + level) {
				sd->status.skill[id].lv = (sd->status.skill[id].flag - 2) + level;
				sd->status.skill[id].flag = 0; // flag: 0 (normal), 1 (only card), 2-12 (card and skill (skill level +2)), 13 (cloneskill)
			} else {
				sd->status.skill[id].flag = sd->status.skill[id].flag + level;
			}
		} else // Check here for clone skill (not yet done)
			sd->status.skill[id].lv += level;

		if (sd->status.skill[id].lv)
			sd->status.skill[id].id = id;
		status_calc_pc(sd, 0);
		clif_skillinfoblock(sd);

	// Set card (flag == 1)
	} else if (sd->status.skill[id].lv < level) {
		if (!sd->status.skill[id].flag) { // Check for clone skill not yet done
			if (sd->status.skill[id].id == id) {
				// flag: 0 (normal), 1 (only card), 2-12 (card and skill (skill level +2)), 13 (cloneskill)
				sd->status.skill[id].flag = sd->status.skill[id].lv + 2;
			} else {
				sd->status.skill[id].id = id;
				sd->status.skill[id].flag = 1;
			}
		}
		sd->status.skill[id].lv = level;
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int pc_blockskill_end(int tid, unsigned int tick, int id, int data) {

	struct map_session_data *sd = map_id2sd(id);

	if (data <= 0 || data >= MAX_SKILL)
		return 0;
	if (sd)
		sd->blockskill[data] = 0;

	return 1;
}

/*==========================================
 *
 *------------------------------------------
 */
int pc_blockskill_start(struct map_session_data *sd, int skillid, int tick) {

	nullpo_retr(-1, sd);

	if (skillid >= 10000 && skillid < 10015)
		skillid -= 9100;
	else if (skillid < 0 || skillid > MAX_SKILL)
		return -1;

	sd->blockskill[skillid] = 1;

	return add_timer(gettick_cache + tick, pc_blockskill_end, sd->bl.id, skillid);
}

/*==========================================
 *
 *------------------------------------------
 */
void pc_insert_card(struct map_session_data *sd, short idx_card, short idx_equip) {

	int i;
	int nameid;
	int cardid;
	int ep;

	if (idx_card >= 0 && idx_card < MAX_INVENTORY && idx_equip >= 0 && idx_equip < MAX_INVENTORY && sd->inventory_data[idx_card]) {
		nameid = sd->status.inventory[idx_equip].nameid;
		cardid = sd->status.inventory[idx_card].nameid;
		ep = sd->inventory_data[idx_card]->equip;

		if (nameid <= 0 || sd->inventory_data[idx_equip] == NULL ||
		    (sd->inventory_data[idx_equip]->type != 4 && sd->inventory_data[idx_equip]->type != 5) || // 4: Weapon, 5: Armor
		    (sd->status.inventory[idx_equip].identify == 0) || 
		    (sd->inventory_data[idx_card]->type != 6) || // Check if its a card (Prevent Hack)
		    (sd->status.inventory[idx_equip].card[0] == 0x00ff) || 
		    (sd->status.inventory[idx_equip].card[0] == 0x00fe) ||
		    ((sd->inventory_data[idx_equip]->equip & ep) == 0) || 
		    (sd->inventory_data[idx_equip]->type == 4 && ep == 32) || 
		    (sd->status.inventory[idx_equip].card[0] == (short)0xff00) || sd->status.inventory[idx_equip].equip) {
			clif_insert_card(sd, idx_equip, idx_card, 1); // Flag: 1= Fail, 0: Success
			return;
		}

		for(i = 0; i < sd->inventory_data[idx_equip]->slot; i++) {
			if (sd->status.inventory[idx_equip].card[i] == 0) {
				sd->status.inventory[idx_equip].card[i] = cardid;
				clif_insert_card(sd, idx_equip, idx_card, 0); // Flag: 1: Fail, 0: Success
				pc_delitem(sd, idx_card, 1, 1);
				return;
			}
		}
	} else
		clif_insert_card(sd, idx_equip, idx_card, 1); // Flag: 1: Fail, 0: Success

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
int pc_modifybuyvalue(struct map_session_data *sd,int orig_value) {

	int skill,val = orig_value,rate1 = 0,rate2 = 0;
	if ((skill=pc_checkskill(sd,MC_DISCOUNT))>0)
		rate1 = 5+skill*2-((skill==10)? 1:0); // Skill(1-10) -> 7 to 24
	if ((skill=pc_checkskill(sd,RG_COMPULSION))>0)
		rate2 = 5+skill*4; // Skill(1-5) -> 9 to 25
	if (rate1 < rate2) rate1 = rate2;
	if (rate1)
		val = (int)((double)orig_value*(double)(100-rate1)/100.);
	if (val < 0) val = 0;
	if (orig_value > 0 && val < 1) val = 1;

	return val;
}

/*==========================================
 *
 *------------------------------------------
 */
int pc_modifysellvalue(struct map_session_data *sd,int orig_value) {

	int skill,val = orig_value,rate = 0;
	if ((skill=pc_checkskill(sd,MC_OVERCHARGE))>0)
		rate = 5+skill*2-((skill==10)? 1:0); // Skill(1-10) -> 7 to 24
	if (rate)
		val = (int)((double)orig_value*(double)(100+rate)/100.);
	if (val < 0) val = 0;
	if (orig_value > 0 && val < 1) val = 1;

	return val;
}

/*==========================================
 *
 *------------------------------------------
 */
int pc_checkadditem(struct map_session_data *sd,int nameid,int amount) {

	int i;

	nullpo_retr(0, sd);

	if (itemdb_isequip(nameid))
		return ADDITEM_NEW;

	for(i=0;i<MAX_INVENTORY;i++) {
		if (sd->status.inventory[i].nameid == nameid) {
			if (sd->status.inventory[i].amount+amount > MAX_AMOUNT)
				return ADDITEM_OVERAMOUNT;
			return ADDITEM_EXIST;
		}
	}

	if (amount > MAX_AMOUNT)
		return ADDITEM_OVERAMOUNT;

	return ADDITEM_NEW;
}

/*==========================================
 *
 *------------------------------------------
 */
int pc_inventoryblank(struct map_session_data *sd) {

	int i,b;

	nullpo_retr(0, sd);

	for(i=0,b=0;i<MAX_INVENTORY;i++) {
		if (sd->status.inventory[i].nameid==0)
			b++;
	}

	return b;
}

/*==========================================
 *
 *------------------------------------------
 */
int pc_payzeny(struct map_session_data *sd, int zeny) {

	double z;

	nullpo_retr(0, sd);

	z = (double)sd->status.zeny;
	if (sd->status.zeny < zeny || (z - (double)zeny) > MAX_ZENY)
		return 1;
	sd->status.zeny -= zeny;
	clif_updatestatus(sd, SP_ZENY);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
void pc_getzeny(struct map_session_data *sd,int zeny) {

	double z;

	nullpo_retv(sd);

	z = (double)sd->status.zeny;
	if (z + (double)zeny > (double)MAX_ZENY) {
		zeny = 0;
		sd->status.zeny = MAX_ZENY;
	}
	sd->status.zeny += zeny;
	clif_updatestatus(sd, SP_ZENY);

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
int pc_search_inventory(struct map_session_data *sd, int item_id) {

	int i;

	nullpo_retr(-1, sd);

	for(i=0;i<MAX_INVENTORY;i++) {
		if (sd->status.inventory[i].nameid == item_id &&
		 (sd->status.inventory[i].amount > 0 || item_id == 0))
			return i;
	}

	return -1;
}

/*==========================================
 *
 *------------------------------------------
 */
int pc_additem(struct map_session_data *sd, struct item *item_data, int amount) {

	struct item_data *data;
	int i, w;

	nullpo_retr(1, sd);
	nullpo_retr(1, item_data);

	if (item_data->nameid <= 0 || amount <= 0)
		return 1;
	data = itemdb_search(item_data->nameid);
	if ((w = data->weight * amount) + sd->weight > sd->max_weight)
		return 2;

	i = MAX_INVENTORY;
	if (!itemdb_isequip2(data)) {
		for(i = 0; i < MAX_INVENTORY; i++)
			if (sd->status.inventory[i].nameid == item_data->nameid &&
			    sd->status.inventory[i].identify == item_data->identify &&
			    sd->status.inventory[i].refine == item_data->refine &&
			    sd->status.inventory[i].attribute == item_data->attribute &&
			    sd->status.inventory[i].card[0] == item_data->card[0] &&
			    sd->status.inventory[i].card[1] == item_data->card[1] &&
			    sd->status.inventory[i].card[2] == item_data->card[2] &&
			    sd->status.inventory[i].card[3] == item_data->card[3]) {
				if (sd->status.inventory[i].amount + amount > MAX_AMOUNT)
					return 5;
				sd->status.inventory[i].amount += amount;
				clif_additem(sd,i,amount,0); // 0: You got...
				break;
			}
	}
	if (i >= MAX_INVENTORY) {
		i = pc_search_inventory(sd,0);
		if (i >= 0) {
			memcpy(&sd->status.inventory[i], item_data, sizeof(sd->status.inventory[0]));
			sd->status.inventory[i].amount = amount;
			sd->inventory_data[i] = data;
			clif_additem(sd, i, amount, 0); // 0: You got...
		} else
			return 4;
	}
	sd->weight += w;
	clif_updatestatus(sd, SP_WEIGHT);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
void pc_delitem(struct map_session_data *sd, int n, int amount, int type) {

	nullpo_retv(sd);

	if (sd->status.inventory[n].nameid == 0 || amount <= 0 || sd->status.inventory[n].amount < amount || sd->inventory_data[n] == NULL)
		return;

	sd->status.inventory[n].amount -= amount;
	sd->weight -= sd->inventory_data[n]->weight * amount;
	if (sd->status.inventory[n].amount <= 0) {
		if (sd->status.inventory[n].equip)
			pc_unequipitem(sd, n, 3);
		memset(&sd->status.inventory[n], 0, sizeof(sd->status.inventory[0]));
		sd->inventory_data[n] = NULL;
	}
	if (!(type & 1))
		clif_delitem(sd, n, amount);
	if (!(type & 2))
		clif_updatestatus(sd, SP_WEIGHT);

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void pc_dropitem(struct map_session_data *sd, int n, int amount) {

	if (n < 0 || n >= MAX_INVENTORY)
		return;
	if (amount <= 0)
		return;

	if (sd->status.inventory[n].nameid <= 0 ||
	    sd->status.inventory[n].amount < amount ||
	    sd->trade_partner != 0 || sd->vender_id != 0 ||
	    sd->status.inventory[n].amount <= 0)
		return;
	
	// The client does not likes being silently ignored, so we send it a del of 0 qty
	if (!pc_candrop(sd, sd->status.inventory[n].nameid)) {
		clif_delitem(sd,n,0);
		clif_displaymessage (sd->fd, msg_txt(285));
		return;
	}

	map_addflooritem(&sd->status.inventory[n], amount, sd->bl.m, sd->bl.x, sd->bl.y, NULL, NULL, NULL, sd->bl.id, 0);
	pc_delitem(sd, n, amount, 0);

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void pc_takeitem(struct map_session_data *sd, struct flooritem_data *fitem) {

	struct map_session_data *first_sd, *second_sd, *third_sd;
	int flag;

	if (fitem->first_get_id > 0) {
		first_sd = map_id2sd(fitem->first_get_id);
		if (gettick_cache < fitem->first_get_tick) {
			if (fitem->first_get_id != sd->bl.id && !(first_sd && first_sd->status.party_id == sd->status.party_id && first_sd->status.party_id)) {
				clif_additem(sd, 0, 0, 6); // 6: You cannot get the item.
				return;
			}
		} else if (fitem->second_get_id > 0) {
			second_sd = map_id2sd(fitem->second_get_id);
			if (gettick_cache < fitem->second_get_tick) {
				if (fitem->first_get_id != sd->bl.id && fitem->second_get_id != sd->bl.id &&
				    !(first_sd  &&  first_sd->status.party_id == sd->status.party_id &&  first_sd->status.party_id) &&
				    !(second_sd && second_sd->status.party_id == sd->status.party_id && second_sd->status.party_id)) {
					clif_additem(sd, 0, 0, 6); // 6: You cannot get the item.
					return;
				}
			} else if (fitem->third_get_id > 0) {
				third_sd = map_id2sd(fitem->third_get_id);
				if (gettick_cache < fitem->third_get_tick) {
					if (fitem->first_get_id != sd->bl.id && fitem->second_get_id != sd->bl.id && fitem->third_get_id != sd->bl.id &&
					    !(first_sd  &&  first_sd->status.party_id == sd->status.party_id &&  first_sd->status.party_id) &&
					    !(second_sd && second_sd->status.party_id == sd->status.party_id && second_sd->status.party_id) &&
					    !(third_sd  &&  third_sd->status.party_id == sd->status.party_id &&  third_sd->status.party_id)) {
						clif_additem(sd, 0, 0, 6); // 6: You cannot get the item.
						return;
					}
				}
			}
		}
	}

	// Cannot pick up items while in Hiding / Cloaking / Chase Walk modes
	if (pc_ishiding(sd) || pc_iscloaking(sd) || pc_ischasewalk(sd)) {
		clif_additem(sd, 0, 0, 6);
		return;
	}

	if ((flag = pc_additem(sd, &fitem->item_data, fitem->item_data.amount))) {
		clif_additem(sd, 0, 0, flag);
		return;
	}

	if (sd->attacktimer != -1)
		pc_stopattack(sd);
	
	clif_takeitem(&sd->bl, &fitem->bl);
	map_clearflooritem(fitem->bl.id);

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
int pc_isUseitem(struct map_session_data *sd, int n) {

	struct item_data *item;
	short s_class, upper_type;
	
	nullpo_retr(0, sd);

	item = sd->inventory_data[n];
	if (item == NULL)
		return 0;
	if (item->type != 0 && item->type != 2) // Not consumable item
		return 0;

	switch(sd->status.inventory[n].nameid) { // Items map flags based restrictions
		case 601: // Fly Wing restriction
			if (map[sd->bl.m].flag.noteleport || map[sd->bl.m].flag.gvg) {
				clif_skill_teleportmessage(sd, 0);
				return 0;	
			}
			break;
		case 602: // Butterfly Wing restriction
			if (map[sd->bl.m].flag.noreturn)
				return 0;
			break;
		case 604: // Dead Branch
		case 12103: // Bloody Branch
		case 12109: // Poring Box
			if (map[sd->bl.m].flag.nobranch || map[sd->bl.m].flag.gvg)
				return 0;
			break;
	}

	if (item->elv > 0 && sd->status.base_level < item->elv) // Item base level restriction
		return 0;

	if (item->sex != 2 && sd->status.sex != item->sex) // Item gender restriction
		return 0;
		
	// Upper job restriction
	upper_type = pc_get_upper_type(sd->status.class);
	if (upper_type == 0)
		return 0;
	if (item->flag.upper != 0) {		// 0 = 1 + 2 + 4 = 7 = all class
		if (!(item->flag.upper & upper_type))
			return 0;
	}

	s_class = pc_calc_base_job2(sd->status.class);
	switch(s_class) { // Normalize special classes into their normal version
		case JOB_KNIGHT2:
			s_class = JOB_KNIGHT;
			break;
		case JOB_CRUSADER2:
			s_class = JOB_CRUSADER;
			break;
		case JOB_STAR_GLADIATOR2:
			s_class = JOB_STAR_GLADIATOR;
			break;
	}

	if (((1<<s_class)&item->class) == 0) // Item class restriction
		return 0;

	return 1;
}

/*==========================================
 *
 *------------------------------------------
 */
void pc_useitem(struct map_session_data *sd, short n) {

	if (n >= 0 && n < MAX_INVENTORY) {
		int itemid, amount;
		itemid = sd->status.inventory[n].nameid;
		if (itemid <= 0)
			return;
		amount = sd->status.inventory[n].amount;
		if (amount <= 0)
			return;
		if (gettick_cache < sd->canuseitem_tick ||
		    sd->sc_data[SC_BERSERK].timer != -1 ||
			sd->sc_data[SC_GRAVITATION].timer != -1 ||
		    (pc_issit(sd) && (itemid == 605 || itemid == 606)) || // 605: Anodyne, 606: Aloevera
		    (map[sd->bl.m].flag.pvp && (sd->inventory_data[n]->flag.no_equip & 1)) || // PVP // no_equip = 1- not in PvP, 2- GvG restriction, 3- PvP and GvG which restriction
		    (map[sd->bl.m].flag.gvg && (sd->inventory_data[n]->flag.no_equip & 2)) || // GVG // no_equip = 1- not in PvP, 2- GvG restriction, 3- PvP and GvG which restriction
		    !pc_isUseitem(sd, n)) {
			clif_useitemack(sd, n, 0, 0);
			return;
		}

		use_itemid = itemid; // For script of heal items
		use_nameditem = *((unsigned long *)(&sd->status.inventory[n].card[2]));

		if (sd->inventory_data[n])
			run_script(sd->inventory_data[n]->use_script, 0, sd->bl.id, 0);

		use_itemid = use_nameditem = 0;
		pc_delitem(sd, n, 1, 1);
		amount = sd->status.inventory[n].amount;
		clif_useitemack(sd, n, amount, 1);
		sd->canuseitem_tick = gettick_cache + battle_config.item_use_interval; // Update item use time
	}

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
int pc_cart_additem(struct map_session_data *sd, struct item *item_data, int amount) {

	struct item_data *data;
	int i, w, equip_flag;

	nullpo_retr(1, sd);
	nullpo_retr(1, item_data);

	if (item_data->nameid <= 0 || amount <= 0)
		return 1;
	data = itemdb_search(item_data->nameid);

	if (!itemdb_cancartstore(item_data->nameid, sd->GM_level)) {
		clif_displaymessage (sd->fd, msg_txt(286));
		return 1;
	}

	if ((w = data->weight * amount) + sd->cart_weight > sd->cart_max_weight)
		return 1;

	i = MAX_CART;
	if (!(equip_flag = itemdb_isequip2(data))) {
		for(i = 0; i < MAX_CART; i++) {
			if (sd->status.cart[i].nameid == item_data->nameid &&
			    sd->status.cart[i].identify == item_data->identify &&
			    sd->status.cart[i].refine == item_data->refine &&
			    sd->status.cart[i].attribute == item_data->attribute &&
			    sd->status.cart[i].card[0] == item_data->card[0] &&
			    sd->status.cart[i].card[1] == item_data->card[1] &&
			    sd->status.cart[i].card[2] == item_data->card[2] &&
			    sd->status.cart[i].card[3] == item_data->card[3]) {
				if (sd->status.cart[i].amount + amount > MAX_AMOUNT)
					return 1;
				sd->status.cart[i].amount += amount;
				clif_cart_additem(sd, i, amount);
				break;
			}
		}
	}
	if (i >= MAX_CART) {
		for(i = 0; i < MAX_CART; i++) {
			if (sd->status.cart[i].nameid == 0) {
				memcpy(&sd->status.cart[i], item_data, sizeof(sd->status.cart[0]));
				sd->status.cart[i].amount = amount;
				sd->cart_num++;
				if (equip_flag)
					clif_cart_addequipitem(sd, i, amount);
				else
					clif_cart_additem(sd, i, amount);
				break;
			}
		}
		if (i >= MAX_CART)
			return 1;
	}
	sd->cart_weight += w;
	clif_updatestatus(sd, SP_CARTINFO);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
void pc_cart_delitem(struct map_session_data *sd,int n,int amount,int type) {

	nullpo_retv(sd);

	if (sd->status.cart[n].nameid == 0 ||
	    sd->status.cart[n].amount < amount)
		return;

	sd->status.cart[n].amount -= amount;
	sd->cart_weight -= itemdb_weight(sd->status.cart[n].nameid) * amount;
	if (sd->status.cart[n].amount <= 0) {
		memset(&sd->status.cart[n], 0, sizeof(sd->status.cart[0]));
		sd->cart_num--;
	}
	if (!type) {
		clif_cart_delitem(sd, n, amount);
		clif_updatestatus(sd, SP_CARTINFO);
	}

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void pc_putitemtocart(struct map_session_data *sd, short idx, int amount) {

	struct item *item_data;

	if (idx < 0 || idx >= MAX_INVENTORY)
		return;

	item_data = &sd->status.inventory[idx];
	if (item_data->nameid == 0 || item_data->amount < amount)
		return;

	if (pc_cart_additem(sd, item_data, amount) == 0)
		pc_delitem(sd, idx, amount, 0);

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
int pc_cartitem_amount(struct map_session_data *sd, int idx, int amount) {

	struct item *item_data;

	nullpo_retr(-1, sd);

	if (idx < 0 || idx >= MAX_CART)
		return -1;

	item_data = &sd->status.cart[idx];
	if (item_data->nameid == 0 || !item_data->amount)
		return -1;

	return item_data->amount - amount;
}

/*==========================================
 *
 *------------------------------------------
 */
void pc_getitemfromcart(struct map_session_data *sd, short idx, int amount) {

	struct item *item_data;
	int flag;

	if (idx < 0 || idx >= MAX_CART)
		return;

	item_data = &sd->status.cart[idx];
	if (item_data->nameid == 0 || item_data->amount < amount)
		return;

	if ((flag = pc_additem(sd, item_data, amount)) == 0) {
		pc_cart_delitem(sd, idx, amount, 0);
		return;
	}

	clif_additem(sd, 0, 0, flag);

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void pc_item_identify(struct map_session_data *sd, short idx) {

	if (idx < 0 || idx >= MAX_INVENTORY) {
		clif_item_identified(sd, idx, 1);
		return;
	}

	if (sd->skillid == BS_REPAIRWEAPON) {
		pc_item_repair(sd, idx);
		return;
	} else if (sd->skillid == WS_WEAPONREFINE) {
		pc_item_refine(sd, idx);
		return;
	}

	if (sd->status.inventory[idx].nameid > 0 && sd->status.inventory[idx].identify == 0) {
		sd->status.inventory[idx].identify = 1;
		clif_item_identified(sd, idx, 0);
	} else
		clif_item_identified(sd, idx, 1);

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
int pc_item_repair(struct map_session_data *sd, short idx) {

	int material;
	int materials[5] = { 0, 1002, 998, 999, 756 };
	struct item *item;

	item = &(sd->repair_target)->status.inventory[idx];
	if (item->nameid > 0 && item->attribute == 1) {
		if (itemdb_type(item->nameid) == 4)
			material = materials[(int)itemdb_wlv(item->nameid)];
		else
			material = materials[3];
		if (pc_search_inventory(sd, material) < 0) {
			clif_skill_fail(sd, sd->skillid, 0, 0);
			return 0;
		}
		item->attribute = 0;
		pc_delitem(sd, pc_search_inventory(sd, material), 1, 0);
		clif_equiplist(sd->repair_target);
		clif_produceeffect(sd, 0, item->nameid);
		clif_misceffect(&sd->bl, 3);
		return item->nameid;
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
void pc_item_refine(struct map_session_data *sd, short idx) {

	int i = 0, ep = 0, per;
	int material[5] = { 0, 1010, 1011, 984, 984 };
	struct item *item;

	item = &sd->status.inventory[idx];

	if (item->nameid > 0 && itemdb_type(item->nameid) == 4) {
		struct item_data *ditem = sd->inventory_data[idx];
		if (item->refine >= sd->skilllv || item->refine == 10 || ditem->flag.no_refine) {
			clif_skill_fail(sd, sd->skillid, 0, 0);
			return;
		}
		if ((i = pc_search_inventory(sd, material[(int)ditem->wlv])) < 0) {
			clif_skill_fail(sd, sd->skillid, 0, 0);
			return;
		}
		per = percentrefinery[(int)ditem->wlv][(int)item->refine];
			per += (sd->status.job_level - 50) / 2;

		if (per > rand() % 100) {
			// Success
			item->refine++;
			pc_delitem(sd, i, 1, 0);
			if (item->equip) {
				ep = item->equip;
				pc_unequipitem(sd, idx, 3);
			}
			clif_refine(sd->fd, sd, 0, idx, item->refine);
			clif_delitem(sd, idx, 1);
			clif_additem(sd, idx, 1, 0); // 0: You got...
			if (ep)
				pc_equipitem(sd, idx, ep);
			clif_misceffect(&sd->bl, 3);
			if (item->refine == 10 && item->card[0] == 0x00ff &&
				// Blacksmith Fame Point System
				*((unsigned long *)(&sd->status.inventory[idx].card[2])) == sd->status.char_id) {
				switch(ditem->wlv) {
					case 1:
						ranking_gain_point(sd, RK_BLACKSMITH, 1); // Success to refine to +10 a lv1 weapon you forged = +1 fame point
						break;
					case 2:
						ranking_gain_point(sd, RK_BLACKSMITH, 25); // Success to refine to +10 a lv2 weapon you forged = +25 fame point
						break;
					case 3:
						ranking_gain_point(sd, RK_BLACKSMITH, 1000); // Success to refine to +10 a lv3 weapon you forged = +1000 fame point
						break;
				}
			}
		} else {
			// Failure
			pc_delitem(sd, i, 1, 0);
			item->refine = 0;
			if (item->equip)
				pc_unequipitem(sd, idx, 3);
			clif_refine(sd->fd, sd, 1, idx, item->refine);
			pc_delitem(sd, idx, 1, 0);
			clif_misceffect(&sd->bl, 2);

			clif_emotion(&sd->bl, 23);
		}
	}

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
int pc_show_steal(struct block_list *bl,va_list ap) {

	struct map_session_data *sd;
	int itemid;
	int type;

	struct item_data *item=NULL;
	char output[100];

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);
	nullpo_retr(0, sd=va_arg(ap,struct map_session_data *));

	itemid=va_arg(ap,int);
	type=va_arg(ap,int);

	if (!type) {
		if ((item=itemdb_exists(itemid))==NULL)
			sprintf(output, msg_txt(531), sd->status.name); // %s stole an unknown item.
		else
			sprintf(output, msg_txt(532), sd->status.name,item->jname); // %s stole a(n) %s.
		clif_displaymessage( ((struct map_session_data *)bl)->fd, output);
	}else{
		sprintf(output, msg_txt(533), sd->status.name); // %s has not stolen the item because of being overweight.
		clif_displaymessage( ((struct map_session_data *)bl)->fd, output);
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int pc_steal_item(struct map_session_data *sd,struct block_list *bl) {

	int i, skill, itemid, flag, count;
	struct mob_data *md;
	double temp_rate, rate;
	int ratemin, ratemax, type;
	int drop_rate;
	
	if (!sd || !bl || bl->type != BL_MOB)
		return 0;
	
	md = (struct mob_data *)bl;
	
	if (md->state.steal_flag || // Prevent stealing twice or more
		(mob_db[md->class].mode & 0x20) || md->master_id || // Prevent stealing from bosses/summoned slaves
		(md->class >= 1324 && md->class < 1364) || // Prevent stealing from treasure boxes
		map[md->bl.m].flag.nomobdrop) // Prevent stealing when nomobdrop flag is ON
			return 0;
	
	if (md->sc_data && (md->sc_data[SC_STONE].timer != -1 || md->sc_data[SC_FREEZE].timer != -1))
			return 0; // Prevent stealing from froze or stoned mob
			
	skill = battle_config.skill_steal_type == 1 ?
	       (sd->paramc[4] - mob_db[md->class].dex) / 2 + pc_checkskill(sd, TF_STEAL) * 6 + 4 : // kRO (new formula)
	        sd->paramc[4] - mob_db[md->class].dex      + pc_checkskill(sd, TF_STEAL) * 3 + 10; // iRO (old formula)
	
	if ((rand() % 100) < skill) {
		for(count = 0; count < 10; count++) {
			i = rand() % 10;
			itemid = mob_db[md->class].dropitem[i].nameid;
			if (itemid > 0 && (type = itemdb_type(itemid)) != 6) {
				// Get value for calculation of drop rate
				switch (type) {
				case 0:
					rate = (double)battle_config.item_rate_heal;
					ratemin = battle_config.item_drop_heal_min;
					ratemax = battle_config.item_drop_heal_max;
					break;
				case 2:
					rate = (double)battle_config.item_rate_use;
					ratemin = battle_config.item_drop_use_min;
					ratemax = battle_config.item_drop_use_max;
					break;
				case 4:
				case 5:
				case 8: // Changed to include Pet Equip
					rate = (double)battle_config.item_rate_equip;
					ratemin = battle_config.item_drop_equip_min;
					ratemax = battle_config.item_drop_equip_max;
					break;
				default:
					rate = (double)battle_config.item_rate_common;
					ratemin = battle_config.item_drop_common_min;
					ratemax = battle_config.item_drop_common_max;
					break;
				}
				// Calculation of drop rate
				if (mob_db[md->class].dropitem[i].p <= 0)
					drop_rate = 0;
				else {
					temp_rate = ((double)mob_db[md->class].dropitem[i].p) * rate / 100.;
					if (temp_rate > 10000. || temp_rate < 0 || ((int)temp_rate) > 10000)
						drop_rate = 10000;
					else
						drop_rate = (int)temp_rate;
				}
				// Check minimum/maximum with configuration
				if (drop_rate < ratemin)
					drop_rate = ratemin;
				else if (drop_rate > ratemax)
					drop_rate = ratemax;
				if (drop_rate <= 0 && !battle_config.drop_rate0item)
					drop_rate = 1;
				// Add personnal bonus of the player
				drop_rate += sd->add_steal_rate;
				// Check drop success
				if (rand() % 10000 < drop_rate) {
					struct item tmp_item;
					memset(&tmp_item, 0, sizeof(tmp_item));
					tmp_item.nameid = itemid;
					tmp_item.amount = 1;
					tmp_item.identify = 1;
					flag = pc_additem(sd, &tmp_item, 1);
					if (battle_config.show_steal_in_same_party)
						party_foreachsamemap(pc_show_steal, sd, 1, sd, tmp_item.nameid, 0);
					if (flag) {
						if (battle_config.show_steal_in_same_party)
							party_foreachsamemap(pc_show_steal, sd, 1, sd, tmp_item.nameid, 1);
						clif_additem(sd, 0, 0, flag);
					}
					md->state.steal_flag = 1;
					return 1;
				}
			}
		}
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int pc_steal_coin(struct map_session_data *sd,struct block_list *bl) {

	if (sd != NULL && bl != NULL && bl->type == BL_MOB) {
		int rate,skill;
		struct mob_data *md=(struct mob_data *)bl;
		if (md && !md->state.steal_coin_flag) {
			if (md->sc_data && (md->sc_data[SC_STONE].timer != -1 || md->sc_data[SC_FREEZE].timer != -1))
				return 0;
			skill = pc_checkskill(sd,RG_STEALCOIN)*10;
			rate = skill + (sd->status.base_level - mob_db[md->class].lv)*3 + sd->paramc[4]*2 + sd->paramc[5]*2;
			if (rand()%1000 < rate) {
				pc_getzeny(sd,mob_db[md->class].lv*10 + rand()%100);
				md->state.steal_coin_flag = 1;
				return 1;
			}
		}
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int pc_setpos(struct map_session_data *sd, char *mapname_org, int x, int y, int clrtype, unsigned int cellcheck) {

	// Create needed variables
	char mapname[17]; // 16 + NULL
	int m = 0, disguise = 0;

	// Verify pointer is valid
	nullpo_retr(0, sd);

	// Add a check cell coordinates flag if necessary
	if (!cellcheck && x <= 0 && y <= 0)
		cellcheck = 1;

	strncpy(mapname, mapname_org, 16); // 17 - NULL
	mapname[16] = '\0';
	if (strstr(mapname, ".gat") == NULL && strlen(mapname) < 13) { // 16 - 4 (.gat)
		strcat(mapname, ".gat");
	}
	m = map_mapname2mapid(mapname);

	// Clear chatrooms, trades, vending shops, and storage
	if (sd->chatID)
		chat_leavechat(sd);
	if (sd->trade_partner)
		trade_tradecancel(sd);
	if (sd->vender_id)
		vending_closevending(sd);
	if (sd->state.storage_flag)
		storage_guild_storage_quit(sd);
	else
		storage_storage_quit(sd);

	// Clear friend requests
	if (sd->friend_invite > 0) {
		struct map_session_data *tsd = map_charid2sd(sd->friend_invite);
		if (tsd != NULL)
			clif_friend_add_ack(tsd->fd, sd->bl.id, sd->status.char_id, sd->status.name, 1);
		sd->friend_invite = 0;
	}

	// Clear party invite requests
	if (sd->party_invite > 0)
		party_reply_invite(sd, sd->party_invite_account, 0);

	// Clear guild invite requests
	if (sd->guild_invite > 0)
		guild_reply_invite(sd, sd->guild_invite, 0);

	// Clear guild alliance requests
	if (sd->guild_alliance > 0)
		guild_reply_reqalliance(sd, sd->guild_alliance_account, 0);

	// Delete timer before the player moved to another place
	if (sd->pvp_timer != -1 && !battle_config.pk_mode) {
		delete_timer(sd->pvp_timer, pc_calc_pvprank_timer);
		sd->pvp_timer = -1;
	}

	// Clear casting, walking, and attacking actions
	skill_castcancel(&sd->bl, 0, 0);
	pc_stop_walking(sd, 0);
	pc_stopattack(sd);

	// Change sitting players to standing
	// Also clear Gangster's Paradise and Taekwon Rest statuses
	if (pc_issit(sd)) {
		pc_setstand(sd);
		skill_gangsterparadise(sd, 0);
		skill_rest(sd, 0);
	}

	// Clear certain statuses
	if (sd->sc_count) {
		// Only dispell Stone Curse if it's in its second phase
		if (sd->sc_data[SC_STONE].timer != -1 && sd->opt1 != 6)
			status_change_end(&sd->bl, SC_STONE, -1);
		if (sd->sc_data[SC_FREEZE].timer != -1)
			status_change_end(&sd->bl, SC_FREEZE, -1);
		if (sd->sc_data[SC_STUN].timer != -1)
			status_change_end(&sd->bl, SC_STUN, -1);
		if (sd->sc_data[SC_SLEEP].timer != -1)
			status_change_end(&sd->bl, SC_SLEEP, -1);

		if (sd->sc_data[SC_TRICKDEAD].timer != -1)
			status_change_end(&sd->bl, SC_TRICKDEAD, -1);
		if (sd->sc_data[SC_BLADESTOP].timer != -1)
			status_change_end(&sd->bl, SC_BLADESTOP,-1);
		if (sd->sc_data[SC_RUN].timer != -1)
			status_change_end(&sd->bl,SC_RUN,-1);
		if (sd->sc_data[SC_DANCING].timer != -1)
			skill_stop_dancing(&sd->bl, 0);
		if (sd->sc_data[SC_CLOSECONFINE].timer != -1)
			status_change_end(&sd->bl,SC_CLOSECONFINE,-1);
		if (sd->sc_data[SC_CLOSECONFINE2].timer != -1)
			status_change_end(&sd->bl,SC_CLOSECONFINE2,-1);
		if (sd->status.option&2)
			status_change_end(&sd->bl,SC_HIDING,-1);
		if (pc_iscloaking(sd))
			status_change_end(&sd->bl,SC_CLOAKING,-1);
		if (pc_ischasewalk(sd))
			status_change_end(&sd->bl,SC_CHASEWALK,-1);
		if (sd->sc_data[SC_DEVOTION].timer != -1)
			status_change_end(&sd->bl,SC_DEVOTION,-1);
		if (sd->sc_data[SC_MARIONETTE].timer != -1 )
			status_change_end(&sd->bl,SC_MARIONETTE,-1);
		if (sd->sc_data[SC_MARIONETTE2].timer != -1)
			status_change_end(&sd->bl,SC_MARIONETTE2,-1);

		// If target map does not match current map
		if (sd->bl.m != m) {
			if (sd->sc_data[SC_WARM].timer != -1)
				status_change_end(&sd->bl, SC_WARM, -1);
			if (sd->sc_data[SC_SUN_COMFORT].timer != -1)
				status_change_end(&sd->bl, SC_SUN_COMFORT, -1);
			if (sd->sc_data[SC_MOON_COMFORT].timer != -1)
				status_change_end(&sd->bl, SC_MOON_COMFORT, -1);
			if (sd->sc_data[SC_STAR_COMFORT].timer != -1)
				status_change_end(&sd->bl, SC_STAR_COMFORT, -1);
		}

		// Cooking Items are no longer dispelled upon map change (kRO Patch - 11/06/2006)
		/*
		// If target map does not match current map
		if (sd->bl.m != m) {
			// Dispell statuses
			if (sd->sc_data[SC_STRFOOD].timer != -1)
				status_change_end(&sd->bl, SC_STRFOOD, -1);
			if (sd->sc_data[SC_AGIFOOD].timer != -1)
				status_change_end(&sd->bl, SC_AGIFOOD, -1);
			if (sd->sc_data[SC_VITFOOD].timer != -1)
				status_change_end(&sd->bl, SC_VITFOOD, -1);
			if (sd->sc_data[SC_INTFOOD].timer != -1)
				status_change_end(&sd->bl, SC_INTFOOD, -1);
			if (sd->sc_data[SC_DEXFOOD].timer != -1)
				status_change_end(&sd->bl, SC_DEXFOOD, -1);
			if (sd->sc_data[SC_LUKFOOD].timer != -1)
				status_change_end(&sd->bl, SC_LUKFOOD, -1);
			if (sd->sc_data[SC_HITFOOD].timer != -1)
				status_change_end(&sd->bl, SC_HITFOOD, -1);
			if (sd->sc_data[SC_FLEEFOOD].timer != -1)
				status_change_end(&sd->bl, SC_FLEEFOOD, -1);
			if (sd->sc_data[SC_BATKFOOD].timer != -1)
				status_change_end(&sd->bl, SC_BATKFOOD, -1);
			if (sd->sc_data[SC_WATKFOOD].timer != -1)
				status_change_end(&sd->bl, SC_WATKFOOD, -1);
			if (sd->sc_data[SC_MATKFOOD].timer != -1)
				status_change_end(&sd->bl, SC_MATKFOOD, -1);
		}
		*/
	}

	// Clear AoE spells upon map change
	if (sd->bl.m != m)
		skill_clear_element_field(&sd->bl);

	// Prepare pet for warping
	if (sd->status.pet_id > 0 && sd->pd && sd->pet.intimate > 0) {
		pet_stopattack(sd->pd);
		pet_changestate(sd->pd, MS_IDLE, 0);
	}

	// Temporary disguise crash fix
	if (sd->disguise) {
		clif_clearchar(&sd->bl, 9);
		disguise = sd->disguise;
		sd->disguise = 0;
	}

	if (m < 0) {
		if (sd->mapname[0]) {
			int ip, port;
			if (map_mapname2ipport(mapname, &ip, &port) == 0) {
				skill_stop_dancing(&sd->bl,1);
				skill_unit_move(&sd->bl, gettick_cache, 0);
				clif_clearchar_area(&sd->bl,clrtype&0xffff);
				skill_gangsterparadise(sd,0);
				map_delblock(&sd->bl);
				if (sd->status.pet_id > 0 && sd->pd) {
					if (sd->pd->bl.m != m && sd->pet.intimate <= 0) {
						intif_delete_petdata(sd->status.pet_id);
						pet_remove_map(sd);
						sd->status.pet_id = 0;
						sd->petDB = NULL;
						if (battle_config.pet_status_support)
							status_calc_pc(sd,2);
					}
					else if(sd->pet.intimate > 0) {
						pet_stopattack(sd->pd);
						pet_changestate(sd->pd,MS_IDLE,0);
						clif_clearchar_area(&sd->pd->bl,clrtype&0xffff);
						map_delblock(&sd->pd->bl);
					}
				}
				strncpy(sd->mapname, mapname, 16); // 17 - NULL
				sd->mapname[16] = '\0';
				sd->bl.x = x;
				sd->bl.y = y;
				sd->state.waitingdisconnect = 1;
				if (sd->status.pet_id > 0 && sd->pd)
					intif_save_petdata(sd->status.account_id, &sd->pet);
				chrif_save(sd); // Do pc_makesavestatus and save storage + account_reg/account_reg2 too
				storage_delete(sd->status.account_id);
				chrif_changemapserver(sd, mapname, x, y, ip, port);
				return 0;
			}
		}
#if 0
		clif_authfail_fd(sd->fd, 0); // Disconnected from server
		clif_setwaitclose(sd->fd);
#endif
		return 1;
	}

	// Check if target cell coordinates are unwalkable
	// If they are, randomly find a new map cell to place the player
	if (cellcheck) {
		if (x < 0 || x >= map[m].xs || y < 0 || y >= map[m].ys)
			x = y = 0;
		if ((x == 0 && y == 0) || map_getcell(m, x, y, CELL_CHKNOPASS)) {
			do {
				x = rand() % (map[m].xs - 2) + 1;
				y = rand() % (map[m].ys - 2) + 1;
			} while(map_getcell(m, x, y, CELL_CHKNOPASS));
		}
	}

	if (sd->mapname[0] && sd->bl.prev != NULL) {
		skill_unit_move(&sd->bl, gettick_cache, 0);
		clif_clearchar_area(&sd->bl,clrtype&0xffff);
		skill_gangsterparadise(sd,0);
		map_delblock(&sd->bl);
		if (sd->status.pet_id > 0 && sd->pd) {
			if (sd->pd->bl.m != m && sd->pet.intimate <= 0) {
				intif_delete_petdata(sd->status.pet_id);
				pet_remove_map(sd);
				sd->status.pet_id = 0;
				sd->petDB = NULL;
				if (battle_config.pet_status_support)
					status_calc_pc(sd, 2);
				chrif_save(sd); // Do pc_makesavestatus and save storage + account_reg/account_reg2 too
			}
			else if (sd->pet.intimate > 0) {
				pet_stopattack(sd->pd);
				pet_changestate(sd->pd,MS_IDLE, 0);
				clif_clearchar_area(&sd->pd->bl,clrtype&0xffff);
				map_delblock(&sd->pd->bl);
			}
		}
		clif_changemap(sd, map[m].name, x, y);
	} else if(sd->state.auth && sd->state.connect_new == 0)
		sd->state.relocate = 1;

	// Temporary disguise crash fix
	if (disguise)
		sd->disguise = disguise;

	// Change player location data
	strncpy(sd->mapname, mapname, 16); // 17 - NULL
	sd->mapname[16] = '\0';
	sd->bl.m = m;
	sd->to_x = x;
	sd->to_y = y;
	sd->bl.x = x;
	sd->bl.y = y;

	// Change pet location data
	if (sd->status.pet_id > 0 && sd->pd && sd->pet.intimate > 0) {
		sd->pd->bl.m = m;
		sd->pd->bl.x = sd->pd->to_x = x;
		sd->pd->bl.y = sd->pd->to_y = y;
		sd->pd->dir = sd->dir;
	}

	// Activate NPC OnTouch: if necessary
	if (map_getcell(sd->bl.m, sd->bl.x, sd->bl.y, CELL_CHKNPC)) {
		npc_touch_areanpc(sd, sd->bl.m, sd->bl.x, sd->bl.y);
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
void pc_randomwarp(struct map_session_data *sd) {

	int x, y, i;
	int m;

	nullpo_retv(sd);

	m = sd->bl.m;

	if (map[m].flag.noteleport)
		return;

	i = 0;
	do {
		x = rand() % (map[m].xs - 2) + 1;
		y = rand() % (map[m].ys - 2) + 1;
	} while(map_getcell(m, x, y, CELL_CHKNOPASS) && (i++) < 1000);

	if (i < 1000)
		pc_setpos(sd, map[m].name, x, y, 3, 0);

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void pc_memo(struct map_session_data *sd) {

	int skill;
	int i, j;

	if (map[sd->bl.m].flag.nomemo || (map[sd->bl.m].flag.nowarpto && battle_config.any_warp_GM_min_level > sd->GM_level)) {
		clif_skill_teleportmessage(sd, 1);
		return;
	}

	skill = pc_checkskill(sd, AL_WARP);

	if (skill < 1 || skill > MAX_PORTAL_MEMO + 1) // +1 for save point (skill: 0-4, MAX_PORTAL_MEMO: 3)
		clif_skill_memo(sd, 2); // 00: success to take memo., 01: insuffisant skill level., 02: You don't know warp skill.

	if (skill < 2) { // || i < -1 || i > 2) {
		clif_skill_memo(sd, 1); // 00: success to take memo., 01: insuffisant skill level., 02: You don't know warp skill.
		return;
	}

	i = 0;
	for(j = 0; j < MAX_PORTAL_MEMO; j++) {
		if (strcmp(sd->status.memo_point[j].map, map[sd->bl.m].name) == 0) {
			i = j;
			break;
		}
	}
	if (j == MAX_PORTAL_MEMO) {
		for(i = skill - 3; i >= 0; i--) {
			memcpy(&sd->status.memo_point[i+1], &sd->status.memo_point[i], sizeof(struct point));
		}
		i = 0;
	}

	memset(&sd->status.memo_point[i], 0, sizeof(struct point));
	strncpy(sd->status.memo_point[i].map, map[sd->bl.m].name, 16); // 17 - NULL
	sd->status.memo_point[i].x = sd->bl.x;
	sd->status.memo_point[i].y = sd->bl.y;

	clif_skill_memo(sd, 0); // 00: success to take memo., 01: insuffisant skill level., 02: You don't know warp skill.

	return;
}

/*==========================================
 * pc駆け足要求
 *------------------------------------------
 */
int pc_run(struct map_session_data *sd, int skilllv, int dir) {

	int i,to_x,to_y,dir_x,dir_y;

	nullpo_retr(0, sd);

	if (sd->canmove_tick > gettick_cache)
		return 0;

	to_x = sd->bl.x;
	to_y = sd->bl.y;
	dir_x = dirx[dir];
	dir_y = diry[dir];

	for(i=0;i<AREA_SIZE;i++) {
		if (!map_getcell(sd->bl.m,to_x+dir_x,to_y+dir_y,CELL_CHKPASS))
			break;
		to_x += dir_x;
		to_y += dir_y;
	}

	if (to_x == sd->bl.x && to_y == sd->bl.y){
		if (sd->sc_data[SC_RUN].timer!=-1)
			status_change_end(&sd->bl,SC_RUN,-1);
		if (sd->sc_data[SC_SPURT].timer!=-1)
			status_change_end(&sd->bl,SC_SPURT,-1);
	} else
		pc_walktoxy(sd, to_x, to_y);

	return 1;
}

/*==========================================
 *
 *------------------------------------------
 */
int pc_can_reach(struct map_session_data *sd,int x,int y) {

	struct walkpath_data wpd;

	nullpo_retr(0, sd);

	if ( sd->bl.x==x && sd->bl.y==y )
		return 1;

	wpd.path_len=0;
	wpd.path_pos=0;
	wpd.path_half=0;

	return (path_search(&wpd,sd->bl.m,sd->bl.x,sd->bl.y,x,y,0)!=-1)?1:0;
}

/*==========================================
 * 次の1歩にかかる時間を計算
 *------------------------------------------
 */
static int calc_next_walk_step(struct map_session_data *sd) {

	nullpo_retr(0, sd);

	if (sd->walkpath.path_pos>=sd->walkpath.path_len)
		return -1;
	if (sd->walkpath.path[sd->walkpath.path_pos]&1)
		return sd->speed*14/10;

	return sd->speed;
}

/*==========================================
 * 半歩進む(timer関数)
 *------------------------------------------
 */
static int pc_walk(int tid, unsigned int tick, int id, int data) {

	struct map_session_data *sd;
	int i, x, y, dx, dy;
	int moveblock;

	if ((sd = map_id2sd(id)) == NULL)
		return 0;

	if (sd->walktimer != tid) {
		if (battle_config.error_log)
			printf("pc_walk %d != %d\n", sd->walktimer, tid);
		return 0;
	}
	sd->walktimer = -1;
	if (sd->walkpath.path_pos >= sd->walkpath.path_len || sd->walkpath.path_pos != data)
		return 0;

	sd->inchealspirithptick = 0;
	sd->inchealspiritsptick = 0;

	sd->walkpath.path_half ^= 1;
	if (sd->walkpath.path_half == 0) {
		sd->walkpath.path_pos++;
		if (sd->state.change_walk_target) {
			pc_walktoxy_sub(sd);
			return 0;
		}
	} else {
		if (sd->walkpath.path[sd->walkpath.path_pos] >= 8)
			return 1;

		x = sd->bl.x;
		y = sd->bl.y;
		if (map_getcell(sd->bl.m, x, y, CELL_CHKNOPASS)) {
			pc_stop_walking(sd, 1);
			return 0;
		}
		sd->dir = sd->head_dir = sd->walkpath.path[sd->walkpath.path_pos];
		dx = dirx[(int)sd->dir];
		dy = diry[(int)sd->dir];
		if (map_getcell(sd->bl.m, x + dx, y + dy, CELL_CHKNOPASS)) {
			pc_walktoxy_sub(sd);
			return 0;
		}

		moveblock = (x / BLOCK_SIZE != (x+dx) / BLOCK_SIZE || y / BLOCK_SIZE != (y+dy) / BLOCK_SIZE);

		sd->walktimer = 1;	// Temporarily set (so that in clif_set007x the player will still appear as walking)
		map_foreachinmovearea(clif_pcoutsight, sd->bl.m, x - AREA_SIZE, y - AREA_SIZE, x + AREA_SIZE, y + AREA_SIZE, dx, dy, 0, sd);

		x += dx;
		y += dy;

		sd->walktimer = -1;	// Set back so not to disturb future pc_stop_walking calls
		skill_unit_move(&sd->bl, tick, 0);
		if (moveblock) map_delblock(&sd->bl);
		sd->bl.x = x;
		sd->bl.y = y;
		if (moveblock) map_addblock(&sd->bl);
		skill_unit_move(&sd->bl, tick, 1);

		sd->walktimer = 1;	// Temporarily set (so that in clif_set007x the player will still appear as walking)
		map_foreachinmovearea(clif_pcinsight, sd->bl.m, x-AREA_SIZE, y-AREA_SIZE, x+AREA_SIZE, y+AREA_SIZE, -dx, -dy, 0, sd);
		sd->walktimer = -1;	// Set back so not to disturb future pc_stop_walking calls

		if (sd->status.party_id > 0) {
			struct party *p = party_search(sd->status.party_id);
			if (p != NULL) {
				int p_flag = 0;
				map_foreachinmovearea(party_send_hp_check,sd->bl.m,x-AREA_SIZE,y-AREA_SIZE,x+AREA_SIZE,y+AREA_SIZE,-dx,-dy,BL_PC,sd->status.party_id,&p_flag);
				if (p_flag)
					sd->party_hp = -1;
			}
		}

		if (pc_iscloaking(sd))
			skill_check_cloaking(&sd->bl);
		for(i=0;i<5;i++)
			if (sd->dev.val1[i]) {
				skill_devotion3(&sd->bl, sd->dev.val1[i]);
				break;
			}

		if (sd->sc_count) {
			if (sd->sc_data[SC_DANCING].timer != -1) {
				if (sd->sc_data[SC_DANCING].val4) {	// Ensemble skill
					struct skill_unit *unit = map_find_skill_unit_oncell(&sd->bl, sd->bl.x, sd->bl.y, sd->sc_data[SC_DANCING].val1, NULL);
					if (unit) {
						if (unit->group && unit->group->src_id != sd->bl.id && unit->group->src_id != sd->sc_data[SC_DANCING].val4)
							skill_stop_dancing(&sd->bl,0);
					}
					else
						skill_stop_dancing(&sd->bl,0);
				}
				else
					skill_unit_move_unit_group((struct skill_unit_group *)sd->sc_data[SC_DANCING].val2,sd->bl.m,dx,dy);
			}
			if (sd->sc_data[SC_DEVOTION].val1)
				skill_devotion2(&sd->bl,sd->sc_data[SC_DEVOTION].val1);
		}

		if (map_getcell(sd->bl.m, x, y, CELL_CHKNPC))
			npc_touch_areanpc(sd, sd->bl.m, x, y);
		else
			sd->areanpc_id = 0;
	}

	if ((i = calc_next_walk_step(sd)) > 0) {
		i = i >> 1;
		if (i < 1 && sd->walkpath.path_half == 0)
			i = 1;
		sd->walktimer = add_timer(tick + i, pc_walk, id, sd->walkpath.path_pos);
	} else if(sd->sc_data[SC_RUN].timer != -1 ) // Keep trying to run
		pc_run(sd, sd->sc_data[SC_RUN].val1, sd->sc_data[SC_RUN].val2);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
static int pc_walktoxy_sub(struct map_session_data *sd) {

	struct walkpath_data wpd;
	int i;

	nullpo_retr(1, sd);

	if (path_search(&wpd, sd->bl.m, sd->bl.x, sd->bl.y, sd->to_x, sd->to_y, 0))
		return 1;
	memcpy(&sd->walkpath, &wpd, sizeof(wpd));

	clif_walkok(sd);
	sd->state.change_walk_target = 0;

	if ((i = calc_next_walk_step(sd)) > 0) {
		i = i >> 2;
		if (sd->walktimer != -1) // In a call, it's possible to already have a timer
			delete_timer(sd->walktimer, pc_walk);
		sd->walktimer = add_timer(gettick_cache + i, pc_walk, sd->bl.id, 0);
	}
	clif_movechar(sd);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
void pc_walktoxy(struct map_session_data *sd, short x, short y) {

	if (sd->chatID != 0)
		return; // To avoid moving while the player is in a chat [Proximus]
	
	if (sd->sc_data[SC_CONFUSION].timer != -1) {
		if (pc_randomxy(sd))
			return;
	} else {
		sd->to_x = x;
		sd->to_y = y;
	}

	sd->idletime = gettick_cache; // For party experience

	if (sd->walktimer != -1 /*&& sd->state.change_walk_target == 0*/) {
		sd->state.change_walk_target = 1;
	} else
		pc_walktoxy_sub(sd); // Here, we can wall pc_walk timer even if we already have one (fixed in pc_walktoxy_sub)

	if (sd->state.gmaster_flag) {
		int flag = 0;
		if (guild_checkskill(sd->state.gmaster_flag, GD_LEADERSHIP) > 0) flag |= 1 << 0;
		if (guild_checkskill(sd->state.gmaster_flag, GD_GLORYWOUNDS) > 0) flag |= 1 << 1;
		if (guild_checkskill(sd->state.gmaster_flag, GD_SOULCOLD) > 0) flag |= 1 << 2;
		if (guild_checkskill(sd->state.gmaster_flag, GD_HAWKEYES) > 0) flag |= 1 << 3;
		if (flag > 0) {
			map_foreachinarea(skill_guildaura_sub, sd->bl.m,
			                  sd->bl.x - 2, sd->bl.y - 2, sd->bl.x + 2, sd->bl.y + 2, BL_PC,
			                  sd->bl.id, sd->status.guild_id, flag);
		}
	}

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void pc_stop_walking(struct map_session_data *sd, int type) {

	nullpo_retv(sd);

	if (sd->walktimer != -1) {
		delete_timer(sd->walktimer, pc_walk);
		sd->walktimer = -1;
	}
	sd->walkpath.path_len = 0;
	sd->to_x = sd->bl.x;
	sd->to_y = sd->bl.y;
	if (type & 0x01)
		clif_fixpos(&sd->bl);
	if (type & 0x02 && battle_config.pc_damage_delay) {
		if (DIFF_TICK(sd->canmove_tick, gettick_cache) > 0)
			sd->canmove_tick = gettick_cache + status_get_dmotion(&sd->bl);
	}
	if (sd->sc_data[SC_RUN].timer != -1)
		status_change_end(&sd->bl, SC_RUN, -1);
	if (sd->sc_data[SC_CONFUSION].timer != -1)
		status_change_end(&sd->bl, SC_CONFUSION, -1);

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
int pc_randomxy(struct map_session_data *sd) {

	int i, x, y, d;

	d = rand() % 7 + 2;
	for(i = 0; i < 3; i++) { // Search of a movable place
		int r = rand();
		x = sd->bl.x + r % (d * 2 + 1) - d;
		y = sd->bl.y + r / (d * 2 + 1) % (d * 2 + 1) - d;
		if (map_getcell(sd->bl.m, x, y, CELL_CHKPASS)) {
			sd->to_x = x;
			sd->to_y = y;
			return 0;
		}
	}

	return 1;
}

/*==========================================
 *
 *------------------------------------------
 */
int pc_movepos(struct map_session_data *sd, int dst_x, int dst_y, int freepath) {

	int moveblock;
	int dx, dy;

	struct walkpath_data wpd;

	nullpo_retr(0, sd);

	if (!freepath && path_search(&wpd,sd->bl.m,sd->bl.x,sd->bl.y,dst_x,dst_y,0))
		return 1;

	sd->dir = sd->head_dir = map_calc_dir(&sd->bl, dst_x,dst_y);

	dx = dst_x - sd->bl.x;
	dy = dst_y - sd->bl.y;

	moveblock = ( sd->bl.x/BLOCK_SIZE != dst_x/BLOCK_SIZE || sd->bl.y/BLOCK_SIZE != dst_y/BLOCK_SIZE);

	map_foreachinmovearea(clif_pcoutsight,sd->bl.m,sd->bl.x-AREA_SIZE,sd->bl.y-AREA_SIZE,sd->bl.x+AREA_SIZE,sd->bl.y+AREA_SIZE,dx,dy,0,sd);

	skill_unit_move(&sd->bl, gettick_cache, 0);
	if (moveblock) map_delblock(&sd->bl);
	sd->bl.x = dst_x;
	sd->bl.y = dst_y;
	if (moveblock) map_addblock(&sd->bl);
	skill_unit_move(&sd->bl, gettick_cache, 1);

	map_foreachinmovearea(clif_pcinsight,sd->bl.m,sd->bl.x-AREA_SIZE,sd->bl.y-AREA_SIZE,sd->bl.x+AREA_SIZE,sd->bl.y+AREA_SIZE,-dx,-dy,0,sd);

	if (sd->status.party_id>0) {
		struct party *p=party_search(sd->status.party_id);
		if (p!=NULL) {
			int flag=0;
			map_foreachinmovearea(party_send_hp_check,sd->bl.m,sd->bl.x-AREA_SIZE,sd->bl.y-AREA_SIZE,sd->bl.x+AREA_SIZE,sd->bl.y+AREA_SIZE,-dx,-dy,BL_PC,sd->status.party_id,&flag);
			if (flag)
				sd->party_hp=-1;
		}
	}

	if (pc_iscloaking(sd))
		skill_check_cloaking(&sd->bl);

	if (map_getcell(sd->bl.m, sd->bl.x, sd->bl.y, CELL_CHKNPC))
		npc_touch_areanpc(sd, sd->bl.m, sd->bl.x, sd->bl.y);
	else
		sd->areanpc_id = 0;

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int pc_checkskill(struct map_session_data *sd,int skill_id) {

	if (sd == NULL) return 0;
	if (skill_id >= 10000) {
		struct guild *g;
		if (sd->status.guild_id > 0 && (g = guild_search(sd->status.guild_id)) != NULL)
			return guild_checkskill(g, skill_id);
		return 0;
	}

	if (sd->status.skill[skill_id].id == skill_id)
		return (sd->status.skill[skill_id].lv);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
void pc_checkallowskill(struct map_session_data *sd) {

	nullpo_retv(sd);
	
	if (!sd->sc_count)
		return;

	// Player must have a Two-Handed weapon equipped to get bonuses
	if (sd->sc_data[SC_TWOHANDQUICKEN].timer != -1 && !(skill_get_weapontype(KN_TWOHANDQUICKEN)&(1<<sd->status.weapon)))
		status_change_end(&sd->bl, SC_TWOHANDQUICKEN, -1);

	// Player must have a One-Handed weapon equipped to get bonuses
	if (sd->sc_data[SC_ONEHAND].timer != -1 && !(skill_get_weapontype(KN_ONEHAND)&(1<<sd->status.weapon)))
		status_change_end(&sd->bl, SC_ONEHAND, -1);
		
	// Player must have a Sword-Type weapon equipped to get bonuses
	if (sd->sc_data[SC_AURABLADE].timer != -1 && !(skill_get_weapontype(LK_AURABLADE)&(1<<sd->status.weapon)))
		status_change_end(&sd->bl, SC_AURABLADE, -1);

	// Player must have a Sword-Type weapon equipped to get bonuses
	if (sd->sc_data[SC_PARRYING].timer != -1 && !(skill_get_weapontype(LK_PARRYING)&(1<<sd->status.weapon)))
		status_change_end(&sd->bl, SC_PARRYING, -1);

	// Player must have a Spear-Type weapon equipped to get bonuses
	if (sd->sc_data[SC_SPEARQUICKEN].timer != -1 && !(skill_get_weapontype(CR_SPEARQUICKEN)&(1<<sd->status.weapon)))
		status_change_end(&sd->bl, SC_SPEARQUICKEN, -1);

	// Player must have an Axe-Type weapon equipped to get bonuses
	if (sd->sc_data[SC_ADRENALINE].timer != -1 && !(skill_get_weapontype(BS_ADRENALINE)&(1<<sd->status.weapon)))
		status_change_end(&sd->bl, SC_ADRENALINE, -1);

	// Player must not have a weapon to receive STR bonus
	if (sd->sc_data[SC_SPURT].timer != -1 && sd->status.weapon)
		status_change_end(&sd->bl, SC_SPURT, -1);

	// Player must have a Gatling Gun equipped to get bonuses
	if (sd->sc_data[SC_GATLINGFEVER].timer != -1 && sd->status.weapon != 21)
		status_change_end(&sd->bl, SC_GATLINGFEVER, -1);

	// Player must have a Shield equipped to get bonuses
	if (sd->status.shield <= 0) {
		if (sd->sc_data[SC_AUTOGUARD].timer != -1)
			status_change_end(&sd->bl, SC_AUTOGUARD, -1);

		if (sd->sc_data[SC_DEFENDER].timer != -1)
			status_change_end(&sd->bl, SC_DEFENDER, -1);
	
		if (sd->sc_data[SC_REFLECTSHIELD].timer != -1)
			status_change_end(&sd->bl, SC_REFLECTSHIELD, -1);
	}

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
int pc_checkequip(struct map_session_data *sd,int pos) {

	int i;

	nullpo_retr(-1, sd);

	for(i=0;i<11;i++) {
		if (pos & equip_pos[i])
			return sd->equip_index[i];
	}

	return -1;
}

/*==========================================
 *
 *------------------------------------------
 */
struct pc_base_job pc_calc_base_job(unsigned int b_class) {

	struct pc_base_job bj;

	// First, Second, and Super Novice Class Calculation
	if (b_class <= JOB_SUPER_NOVICE) {
		bj.job = b_class;
		bj.upper = 0;
	// First, and Second Advanced Class Calculation
	} else if (b_class >= JOB_NOVICE_HIGH && b_class < JOB_BABY) {
		bj.job = b_class - JOB_NOVICE_HIGH;
		bj.upper = 1;
	// Taekwon Class Calculation
	} else if (b_class >= JOB_TAEKWON && b_class <= JOB_SOUL_LINKER) {
		if (b_class == JOB_STAR_GLADIATOR2)
			bj.job = 25;
		else
			bj.job = 24 + b_class - JOB_TAEKWON;
		bj.upper = 0;
	// Gunslinger/Ninja Calculation
	} else if (b_class == JOB_GUNSLINGER || b_class == JOB_NINJA) {
		if (b_class == JOB_GUNSLINGER)
			bj.job = 29;
		if (b_class == JOB_NINJA)
			bj.job = 28;
		bj.upper = 0;
	// First, Second, and Super Novice Baby Class Calculation
	} else {
		if (b_class == JOB_SUPER_BABY)
			bj.job = JOB_SUPER_NOVICE;
		else
			bj.job = b_class - JOB_BABY;
		bj.upper = 2;
	}

	// Novice Type Calculation
	if (bj.job == JOB_NOVICE)
		bj.type = 0;
	// First Class Type Calculation
	else if (bj.job <= JOB_THIEF || bj.job == JOB_TAEKWON || bj.job == JOB_GUNSLINGER || bj.job == JOB_NINJA)
		bj.type = 1;
	// Second Class Type Calculation
	else
		bj.type = 2;

	return bj; // Returns calculation results
}

/*==========================================
 *
 *------------------------------------------
 */
int pc_calc_base_job2(unsigned int b_class) {

	// First, Second, and Super Novice Class Calculation
	if (b_class < JOB_SUPER_NOVICE)
		return b_class;

	// First, and Second Advanced Class Calculation
	if (b_class >= JOB_NOVICE_HIGH && b_class < JOB_BABY)
		return b_class - JOB_NOVICE_HIGH;

	// Taekwon Class Calculation
	if (b_class >= JOB_TAEKWON && b_class <= JOB_SOUL_LINKER) {
		if (b_class == JOB_STAR_GLADIATOR2)
			return 24 + JOB_STAR_GLADIATOR - JOB_TAEKWON;
		else
			return 24 + b_class - JOB_TAEKWON;
	}

	// Gunslinger/Ninja Calculation
	if (b_class == JOB_GUNSLINGER)
		return 29;
	if (b_class == JOB_NINJA)
		return  28;

	// Super Baby Calculation
	if (b_class == JOB_SUPER_BABY)
		return JOB_SUPER_NOVICE;

	// First, and Second Baby Class Calculation
	if (b_class >= JOB_BABY && b_class < JOB_SUPER_BABY)
		return b_class - 4023;

	return b_class; // If unable to calculate, just return b_class
}

/*==========================================
 *
 *------------------------------------------
 */
int pc_calc_upper(unsigned int b_class) {

	if (b_class <= JOB_NINJA || (b_class >= JOB_TAEKWON && b_class <= JOB_MUNAK))	// Normal Classes
		return 0;

	if (b_class >= JOB_NOVICE_HIGH && b_class < JOB_BABY)	// Advanced classes
		return 1;

	if (b_class >= JOB_BABY && b_class <= JOB_SUPER_BABY)	// Baby Classes
		return 2;

	return 0; // If unable to calculate, just return 0 (Normal Classes)
}

/*==========================================
 *
 *------------------------------------------
 */
short pc_get_upper_type(unsigned int b_class) {

	if (b_class <= JOB_NINJA || (b_class >= JOB_TAEKWON && b_class <= JOB_MUNAK)) // Normal Classes
		return 1;
	else if (b_class >= JOB_NOVICE_HIGH && b_class < JOB_BABY) // Advanced Classes
		return 2;
	else if (b_class >= JOB_BABY && b_class <= JOB_SUPER_BABY) // Baby Classes
		return 4;

	return 0; // If unable to calculate, return 0 (Unknown)
}

/*==========================================
 *
 *------------------------------------------
 */
int pc_attack_timer(int tid, unsigned int tick, int id, int data) {

	struct map_session_data *sd;
	struct block_list *bl;
	struct status_change *sc_data;
	short *opt;
	int dist, skill, range;

	sd = map_id2sd(id);
	if (sd == NULL)
		return 0;

	sd->idletime = tick; // For party experience

	if (sd->attacktimer != tid) {
		if (battle_config.error_log)
			printf("pc_attack_timer %d != %d\n", sd->attacktimer, tid);
		return 0;
	}
	sd->attacktimer = -1;

	if (sd->bl.prev == NULL)
		return 0;

	bl = map_id2bl(sd->attacktarget);
	if (bl == NULL || bl->prev == NULL)
		return 0;

	if (bl->type == BL_PC) {
		if (pc_isdead((struct map_session_data *)bl))
			return 0;
		else if (pc_ishiding((struct map_session_data *)bl))
			return 0;
	}

	if (sd->bl.m != bl->m || pc_isdead(sd))
		return 0;

	if (sd->opt1 > 0 || sd->status.option & 2 || pc_ischasewalk(sd) || sd->view_class == 22)
		return 0;
	
	if (sd->sc_count) {
		if (sd->sc_data[SC_AUTOCOUNTER].timer != -1)
			return 0;
		if (sd->sc_data[SC_BLADESTOP].timer != -1)
			return 0;
	}

	if ((opt = status_get_option(bl)) != NULL && *opt & 0x42)
		return 0;
	if ((sc_data = status_get_sc_data(bl)) != NULL) {
		if (sc_data[SC_TRICKDEAD].timer != -1 || sc_data[SC_BASILICA].timer != -1)
			return 0;
	}

	if (sd->skilltimer != -1 && pc_checkskill(sd, SA_FREECAST) <= 0)
		return 0;

	if (!battle_config.sdelay_attack_enable && pc_checkskill(sd,SA_FREECAST) <= 0) {
		if (DIFF_TICK(tick , sd->canact_tick) < 0) {
			clif_skill_fail(sd, 1, 4, 0);
			return 0;
		}
	}

	dist = distance(sd->bl.x, sd->bl.y, bl->x, bl->y);
	range = sd->attackrange;
	if (sd->status.weapon != 11) range++;
	if (dist > range) {
		if (pc_can_reach(sd, bl->x, bl->y))
			clif_movetoattack(sd, bl);
		return 0;
	}

	if (dist <= range && !battle_check_range(&sd->bl, bl, range)) {
		if (pc_can_reach(sd, bl->x, bl->y) && sd->canmove_tick < tick && (sd->sc_data[SC_ANKLE].timer == -1 || sd->sc_data[SC_SPIDERWEB].timer == -1))
			pc_walktoxy(sd, bl->x, bl->y);
		sd->attackabletime = tick + (sd->aspd << 1);
	}
	else {
		if (battle_config.pc_attack_direction_change)
			sd->dir = sd->head_dir = map_calc_dir(&sd->bl, bl->x, bl->y);

		if (sd->walktimer != -1)
			pc_stop_walking(sd, 1);

		if (sd->sc_data[SC_COMBO].timer == -1) {
			map_freeblock_lock();
			pc_stop_walking(sd, 0);
			sd->attacktarget_lv = battle_weapon_attack(&sd->bl, bl, tick, 0);
			if (!(battle_config.pc_cloak_check_type & 2) && sd->sc_data[SC_CLOAKING].timer != -1)
				status_change_end(&sd->bl, SC_CLOAKING,-1);
			if (sd->status.pet_id > 0 && sd->pd && sd->petDB && battle_config.pet_attack_support)
				pet_target_check(sd, bl, 0);
			map_freeblock_unlock();
			if (sd->skilltimer != -1 && (skill = pc_checkskill(sd, SA_FREECAST)) > 0)
				sd->attackabletime = tick + ((sd->aspd << 1) * (150 - skill * 5) / 100);
			else
				sd->attackabletime = tick + (sd->aspd << 1);
		}
		else if (sd->attackabletime <= tick) {
			if (sd->skilltimer != -1 && (skill = pc_checkskill(sd, SA_FREECAST)) > 0)
				sd->attackabletime = tick + ((sd->aspd << 1) * (150 - skill * 5) / 100);
			else
				sd->attackabletime = tick + (sd->aspd << 1);
		}
		if (sd->attackabletime <= tick) sd->attackabletime = tick + (battle_config.max_aspd<<1);
	}

	if (sd->state.attack_continue)
		sd->attacktimer = add_timer(sd->attackabletime, pc_attack_timer, sd->bl.id, 0);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int pc_attack(struct map_session_data *sd, int target_id, int type) {

	struct block_list *bl;
	int d;

	nullpo_retr(0, sd);

	bl = map_id2bl(target_id);
	if (bl == NULL) {
		return 1;
	}

	sd->idletime = gettick_cache; // For party experience

	if (bl->type == BL_NPC) { // Monster NPCs
		if (sd->npc_id == 0)
			npc_click(sd, target_id);
		return 0;
	}

	if (battle_check_target(&sd->bl, bl, BCT_ENEMY) <= 0)
		return 1;

	if (sd->attacktimer != -1) {
		sd->attacktarget = target_id;
		sd->state.attack_continue = type;
		return 0;
	}

	sd->attacktarget = target_id;
	sd->state.attack_continue = type;

	d = DIFF_TICK(sd->attackabletime, gettick_cache);
	if (d > 0 && d < 2000) {
		sd->attacktimer = add_timer(sd->attackabletime, pc_attack_timer, sd->bl.id, 0);
	} else {
		pc_attack_timer(-1, gettick_cache, sd->bl.id, 0);
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
void pc_stopattack(struct map_session_data *sd) {

	/* Pressure is uninterruptable */
	if (sd->skillid == PA_PRESSURE)
		return;

	if (sd->attacktimer != -1) {

		delete_timer(sd->attacktimer, pc_attack_timer);
		sd->attacktimer = -1;
	}

	sd->attacktarget = 0;
	sd->state.attack_continue = 0;

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
int pc_follow_timer(int tid, unsigned int tick, int id, int data) {

	struct map_session_data *sd, *pl_sd;

	sd = map_id2sd(id);

	if (sd == NULL || sd->followtimer != tid)
		return 0;

	sd->followtimer = -1;

	if (pc_isdead(sd) || pc_iscloaking(sd) || pc_ishiding(sd) || pc_ischasewalk(sd)) {
		sd->followtarget = -1;
		return 0;
	}

	pl_sd = map_id2sd(sd->followtarget);

	if (pl_sd == NULL || !pl_sd->state.auth) {
		sd->followtarget = -1;
		return 0;
	}

	if (pc_isdead(pl_sd) && battle_config.atcommand_follow_stop_dead_target) {
		sd->followtarget = -1;
		return 0;
	}

	do {
		if (sd->bl.prev == NULL)
			break;

		if (sd->sc_data[SC_TRICKDEAD].timer != -1) // if player act dead, don't follow until not more act dead (it's bug of dual sprite)
			break;

		if (sd->skilltimer == -1 && sd->attacktimer == -1 && sd->walktimer == -1) {
			if ((sd->bl.m == pl_sd->bl.m) && pc_can_reach(sd, pl_sd->bl.x, pl_sd->bl.y)) {
				// change follower speed if its speed is slower than followtarget
				if (sd->speed > pl_sd->speed) {
					sd->speed = pl_sd->speed;
					clif_updatestatus(sd, SP_SPEED);
					clif_displaymessage(sd->fd, msg_txt(667)); // Server has adjusted your speed to your follow target.
				}
				if (distance(sd->bl.x, sd->bl.y, pl_sd->bl.x, pl_sd->bl.y) > 5) {
					int x_diff, y_diff;
					if (sd->bl.x < pl_sd->bl.x)
						x_diff = -1;
					else if (sd->bl.x > pl_sd->bl.x)
						x_diff = 1;
					else
						x_diff = 0;
					if (sd->bl.y < pl_sd->bl.y)
						y_diff = -1;
					else if (sd->bl.y > pl_sd->bl.y)
						y_diff = 1;
					else
						y_diff = 0;
					// try to not move exactly at same position
					if (pc_can_reach(sd, pl_sd->bl.x + x_diff, pl_sd->bl.y + y_diff))
						pc_walktoxy(sd, pl_sd->bl.x + x_diff, pl_sd->bl.y + y_diff);
					else
						pc_walktoxy(sd, pl_sd->bl.x, pl_sd->bl.y);
				}
			} else {
				pc_setpos(sd, pl_sd->mapname, pl_sd->bl.x, pl_sd->bl.y, 3, 1);
			}
		}
	} while (0);

	sd->followtimer = add_timer(tick + sd->aspd, pc_follow_timer, sd->bl.id, 0);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int pc_follow(struct map_session_data *sd, int target_id) {

	struct block_list *bl;

	bl = map_id2bl(target_id);
	if (bl == NULL)
		return 1;

	sd->followtarget = target_id;
	if (sd->followtimer != -1) {
		delete_timer(sd->followtimer, pc_follow_timer);
		sd->followtimer = -1;
	}

	pc_follow_timer(-1, gettick_cache, sd->bl.id, 0);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int pc_checkbaselevelup(struct map_session_data *sd) {

	int next = pc_nextbaseexp(sd);

	nullpo_retr(0, sd);

	if (sd->status.base_exp >= next && next > 0) {
		unsigned int s_class = pc_calc_base_job2(sd->status.class);

		sd->status.base_exp -= next;

		sd->status.base_level++;
		sd->status.status_point += (sd->status.base_level + 14) / 5;
		if (sd->status.str  >= battle_config.max_parameter &&
		    sd->status.agi  >= battle_config.max_parameter &&
		    sd->status.vit  >= battle_config.max_parameter &&
		    sd->status.int_ >= battle_config.max_parameter &&
		    sd->status.dex  >= battle_config.max_parameter &&
		    sd->status.luk  >= battle_config.max_parameter)
			sd->status.status_point = 0;
		clif_updatestatus(sd, SP_STATUSPOINT);
		clif_updatestatus(sd, SP_BASELEVEL);
		clif_updatestatus(sd, SP_NEXTBASEEXP);
		status_calc_pc(sd, 0);
		pc_heal(sd, sd->status.max_hp, sd->status.max_sp);

		// Super Novice Level Up Buffs
		if (s_class == JOB_SUPER_NOVICE) {
			status_change_start(&sd->bl, SkillStatusChangeTable[PR_KYRIE], 1, 0, 0, 0, skill_get_time(PR_KYRIE, 1), 0);
			status_change_start(&sd->bl, SkillStatusChangeTable[PR_IMPOSITIO], 1, 0, 0, 0, skill_get_time(PR_IMPOSITIO, 1), 0);
			status_change_start(&sd->bl, SkillStatusChangeTable[PR_MAGNIFICAT], 1, 0, 0, 0, skill_get_time(PR_MAGNIFICAT, 1), 0);
			status_change_start(&sd->bl, SkillStatusChangeTable[PR_GLORIA], 1, 0, 0, 0, skill_get_time(PR_GLORIA, 1), 0);
			status_change_start(&sd->bl, SkillStatusChangeTable[PR_SUFFRAGIUM], 1, 0, 0, 0, skill_get_time(PR_SUFFRAGIUM, 1), 0);
		}
		
		// Taekwon-Class Level Up Buffs (L10 Blessing, L10 Increase Agi)
		if (s_class >= 24 && s_class <= 27) {
			status_change_start(&sd->bl, SkillStatusChangeTable[AL_BLESSING], 10, 0, 0, 0, skill_get_time(AL_BLESSING, 10), 0);
			status_change_start(&sd->bl, SkillStatusChangeTable[AL_INCAGI], 10, 0, 0, 0, skill_get_time(AL_INCAGI, 10), 0);
		}

		clif_misceffect(&sd->bl, 0);

		if (sd->status.party_id > 0) {
			party_send_movemap(sd);
		}

		return 1;
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int pc_checkjoblevelup(struct map_session_data *sd) {

	int next = pc_nextjobexp(sd);

	nullpo_retr(0, sd);

	if (sd->status.job_exp >= next && next > 0) {
		sd->status.job_exp -= next;
		sd->status.job_level ++;
		clif_updatestatus(sd, SP_JOBLEVEL);
		clif_updatestatus(sd, SP_NEXTJOBEXP);
		sd->status.skill_point ++;
		clif_updatestatus(sd, SP_SKILLPOINT);
		status_calc_pc(sd, 0);

		clif_misceffect(&sd->bl,1);
		return 1;
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
void pc_gainexp(struct map_session_data *sd, unsigned int base_exp, unsigned int job_exp) {

	char output[256];
	nullpo_retv(sd);

	if (sd->bl.prev == NULL || pc_isdead(sd))
		return;

	if ((battle_config.pvp_exp == 0) && map[sd->bl.m].flag.pvp)
		return;

	if (sd->sc_data[SC_RICHMANKIM].timer != -1) {
		base_exp = base_exp * (125 + sd->sc_data[SC_RICHMANKIM].val1 * 11) / 100;
		job_exp = job_exp * (125 + sd->sc_data[SC_RICHMANKIM].val1 * 11) / 100;
  }

	if (sd->status.guild_id > 0) {
		base_exp -= guild_payexp(sd, base_exp);
	}

	if (!battle_config.multi_level_up && pc_nextbaseafter(sd) && sd->status.base_exp+base_exp >= pc_nextbaseafter(sd)) {
		base_exp = pc_nextbaseafter(sd) - sd->status.base_exp;
	}

	sd->status.base_exp += base_exp;

	while(pc_checkbaselevelup(sd));

	clif_updatestatus(sd,SP_BASEEXP);

	if (!battle_config.multi_level_up && pc_nextjobafter(sd) && sd->status.job_exp + job_exp >= pc_nextjobafter(sd)) {
		job_exp = pc_nextjobafter(sd) - sd->status.job_exp;
	}

	sd->status.job_exp += job_exp;

	while(pc_checkjoblevelup(sd));

	clif_updatestatus(sd, SP_JOBEXP);

	if (sd->state.displayexp_flag) { // 0 no xp display, 1: exp display
		if (battle_config.disp_experience_type) {
			double base_percent, job_percent;
			base_percent = (double)pc_nextbaseexp(sd);
			job_percent = (double)pc_nextjobexp(sd);
			sprintf(output, msg_txt(610), base_exp, (base_percent < 1.) ? 0. : ((double)base_exp) / base_percent * 100.,
			                              job_exp,  (job_percent  < 1.) ? 0. : ((double)job_exp)  / job_percent * 100.); // Experience gained: Base:%d (%02.02f%%), Job:%d (%02.02f%%).
		} else
			sprintf(output, msg_txt(594), base_exp, job_exp); // Experience gained: Base:%d, Job:%d.
		clif_disp_onlyself(sd, output);
	}

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
unsigned int pc_nextbaseexp(struct map_session_data *sd) {

	int i;

	nullpo_retr(0, sd);

	if (sd->status.base_level >= MAX_LEVEL || sd->status.base_level <= 0)
		return 0;

	if (sd->status.class == 0 || sd->status.class == 4023) i = 0; // Novice & Baby Novice
	else if (sd->status.class <= 6 || (sd->status.class >= 4024 && sd->status.class <= 4029) ||
					(sd->status.class == 4046)) i = 1; // 1st Job & Baby 1st Job
	else if (sd->status.class <= 21 || (sd->status.class >= 4030 && sd->status.class <= 4044) || sd->status.class == 4049) i = 2; // 2nd Job & Baby 2nd Job
	else if (sd->status.class == 23 || sd->status.class == 4045) i = 3; // Super Novice & Super Baby
	else if (sd->status.class == 4001) i = 4; // High Novice
	else if (sd->status.class >= 4002 && sd->status.class <= 4007) i = 5; // High 1st Job
	else if (sd->status.class == 4047 || sd->status.class == 4048) i = 2; // Star Gladiator
	else if (sd->status.class == 24 || sd->status.class == 25) i = 1; // Gunslinger/Ninja
	else if (sd->status.class >= 4050 && sd->status.class <= 4053) i = 2; // Death Knight/Dark Collector/Munak/Bon Gun Temp Values
	else i = 6; // 3rd Job

	return exp_table[i][sd->status.base_level-1];
}

/*==========================================
 *
 *------------------------------------------
 */
unsigned int pc_nextjobexp(struct map_session_data *sd) {

	int i;

	nullpo_retr(0, sd);

	if (sd->status.job_level >= MAX_LEVEL || sd->status.job_level <= 0)
		return 0;

	if (sd->status.class == 0 || sd->status.class == 4023) i = 7; // Novice & Baby Novice
	else if (sd->status.class <= 6 || (sd->status.class >= 4024 && sd->status.class <= 4029) ||
					(sd->status.class == 4046)) i = 8; // 1st Job & Baby 1st Job
	else if (sd->status.class <= 21 || (sd->status.class >= 4030 && sd->status.class <= 4044) || sd->status.class == 4049) i = 9; // 2nd Job & Baby 2nd Job
	else if (sd->status.class == 23 || sd->status.class == 4045) i = 10; // Super Novice & Super Baby
	else if (sd->status.class == 4001) i = 11; // High Novice
	else if (sd->status.class >= 4002 && sd->status.class <= 4007) i = 12; // High 1st Job
	else if (sd->status.class == 4047 || sd->status.class == 4048) i = 14; // Star Gladiator
	else if (sd->status.class == 24 || sd->status.class == 25) i = 15; // Gunslinger/Ninja
	else if (sd->status.class >= 4050 && sd->status.class <= 4053) i = 9; // Death Knight/Dark Collector/Munak/Bon Gun Temp Values
	else i = 13; // 3rd Job

	return exp_table[i][sd->status.job_level-1];
}

/*==========================================
 *
 *------------------------------------------
 */
unsigned int pc_nextbaseafter(struct map_session_data *sd) {

	int i;

	nullpo_retr(0, sd);

	if (sd->status.base_level >= MAX_LEVEL || sd->status.base_level <= 0)
		return 0;

	if (sd->status.class == 0 || sd->status.class == 4023) i = 0; // Novice & Baby Novice
	else if (sd->status.class <= 6 || (sd->status.class >= 4024 && sd->status.class <= 4029) ||
					(sd->status.class == 4046)) i = 1; // 1st Job & Baby 1st Job
	else if (sd->status.class <= 21 || (sd->status.class >= 4030 && sd->status.class <= 4044) || sd->status.class == 4049) i = 2; // 2nd Job & Baby 2nd Job
	else if (sd->status.class == 23 || sd->status.class == 4045) i = 3; // Super Novice & Super Baby
	else if (sd->status.class == 4001) i = 4; // High Novice
	else if (sd->status.class >= 4002 && sd->status.class <= 4007) i = 5; // High 1st Job
	else if (sd->status.class == 4047 || sd->status.class == 4048) i = 2; // Star Gladiator
	else if (sd->status.class == 24 || sd->status.class == 25) i = 1; // Gunslinger/Ninja
	else if (sd->status.class >= 4050 && sd->status.class <= 4053) i = 2; // Death Knight/Dark Collector/Munak/Bon Gun Temp Values
	else i = 6; // 3rd Job

	return exp_table[i][sd->status.base_level];
}

/*==========================================
 *
 *------------------------------------------
 */
unsigned int pc_nextjobafter(struct map_session_data *sd) {

	int i;

	nullpo_retr(0, sd);

	if (sd->status.job_level>=MAX_LEVEL || sd->status.job_level<=0)
		return 0;

	if (sd->status.class == 0 || sd->status.class == 4023) i = 7; // Novice & Baby Novice
	else if (sd->status.class <= 6 || (sd->status.class >= 4024 && sd->status.class <= 4029) ||
					(sd->status.class == 4046)) i = 8; // 1st Job & Baby 1st Job
	else if (sd->status.class <= 21 || (sd->status.class >= 4030 && sd->status.class <= 4044) || sd->status.class == 4049) i = 9; // 2nd Job & Baby 2nd Job
	else if (sd->status.class == 23 || sd->status.class == 4045) i = 10; // Super Novice & Super Baby
	else if (sd->status.class == 4001) i = 11; // High Novice
	else if (sd->status.class >= 4002 && sd->status.class <= 4007) i = 12; // High 1st Job
	else if (sd->status.class == 4047 || sd->status.class == 4048) i = 14; // Star Gladiator
	else if (sd->status.class == 24 || sd->status.class == 25) i = 15; // Gunslinger/Ninja
	else if (sd->status.class >= 4050 && sd->status.class <= 4053) i = 9; // Death Knight/Dark Collector/Munak/Bon Gun Temp Values
	else i = 13; // 3rd Job

	return exp_table[i][sd->status.job_level];
}

/*==========================================
 *
 *------------------------------------------
 */
int pc_need_status_point(struct map_session_data *sd,int type) {

	int val;

	nullpo_retr(-1, sd);

	if (type<SP_STR || type>SP_LUK)
		return -1;
	val =
		type==SP_STR ? sd->status.str :
		type==SP_AGI ? sd->status.agi :
		type==SP_VIT ? sd->status.vit :
		type==SP_INT ? sd->status.int_:
		type==SP_DEX ? sd->status.dex : sd->status.luk;

	return (val+9)/10+1;
}

/*==========================================
 *
 *------------------------------------------
 */
void pc_statusup(struct map_session_data *sd, int type) {

	int max, need,val = 0;

	nullpo_retv(sd);

	max = (pc_calc_upper(sd->status.class) == 2) ? 80 : battle_config.max_parameter;

	need = pc_need_status_point(sd, type);
	if (type < SP_STR || type > SP_LUK || need < 0 || need>sd->status.status_point) {
		clif_statusupack(sd, type, 0, 0);
		return;
	}
	switch(type) {
	case SP_STR:
		if (sd->status.str >= max) {
			clif_statusupack(sd, type, 0, 0);
			return;
		}
		val= ++sd->status.str;
		break;
	case SP_AGI:
		if (sd->status.agi >= max) {
			clif_statusupack(sd, type, 0, 0);
			return;
		}
		val= ++sd->status.agi;
		break;
	case SP_VIT:
		if (sd->status.vit >= max) {
			clif_statusupack(sd, type, 0, 0);
			return;
		}
		val= ++sd->status.vit;
		break;
	case SP_INT:
		if (sd->status.int_ >= max) {
			clif_statusupack(sd, type, 0, 0);
			return;
		}
		val= ++sd->status.int_;
		break;
	case SP_DEX:
		if (sd->status.dex >= max) {
			clif_statusupack(sd, type, 0, 0);
			return;
		}
		val= ++sd->status.dex;
		break;
	case SP_LUK:
		if (sd->status.luk >= max) {
			clif_statusupack(sd, type, 0, 0);
			return;
		}
		val= ++sd->status.luk;
		break;
	}
	sd->status.status_point -= need;
	if (need != pc_need_status_point(sd, type)) {
		clif_updatestatus(sd, type - SP_STR + SP_USTR);
	}
	if (sd->status.str >= battle_config.max_parameter &&
	    sd->status.agi >= battle_config.max_parameter &&
	    sd->status.vit >= battle_config.max_parameter &&
	    sd->status.int_ >= battle_config.max_parameter &&
	    sd->status.dex >= battle_config.max_parameter &&
	    sd->status.luk >= battle_config.max_parameter)
		sd->status.status_point = 0;
	clif_updatestatus(sd, SP_STATUSPOINT);
	clif_updatestatus(sd, type);
	status_calc_pc(sd, 0);
	clif_statusupack(sd, type, 1, val);

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void pc_statusup2(struct map_session_data *sd, int type, int val) {

	nullpo_retv(sd);

	if (type < SP_STR || type > SP_LUK) {
		clif_statusupack(sd, type, 0, 0);
		return;
	}
	switch(type) {
	case SP_STR:
		if (sd->status.str + val >= battle_config.max_parameter)
			val = battle_config.max_parameter;
		else if (sd->status.str + val < 1)
			val = 1;
		else
			val += sd->status.str;
		sd->status.str = val;
		break;
	case SP_AGI:
		if (sd->status.agi + val >= battle_config.max_parameter)
			val = battle_config.max_parameter;
		else if (sd->status.agi + val < 1)
			val = 1;
		else
			val += sd->status.agi;
		sd->status.agi = val;
		break;
	case SP_VIT:
		if (sd->status.vit + val >= battle_config.max_parameter)
			val = battle_config.max_parameter;
		else if (sd->status.vit + val < 1)
			val = 1;
		else
			val += sd->status.vit;
		sd->status.vit = val;
		break;
	case SP_INT:
		if (sd->status.int_ + val >= battle_config.max_parameter)
			val = battle_config.max_parameter;
		else if (sd->status.int_ + val < 1)
			val = 1;
		else
			val += sd->status.int_;
		sd->status.int_ = val;
		break;
	case SP_DEX:
		if (sd->status.dex + val >= battle_config.max_parameter)
			val = battle_config.max_parameter;
		else if (sd->status.dex + val < 1)
			val = 1;
		else
			val += sd->status.dex;
		sd->status.dex = val;
		break;
	case SP_LUK:
		if (sd->status.luk + val >= battle_config.max_parameter)
			val = battle_config.max_parameter;
		else if (sd->status.luk + val < 1)
			val = 1;
		else
			val = sd->status.luk + val;
		sd->status.luk = val;
		break;
	}
	// if player have max in all stats, don't give status_point
	if (sd->status.str  >= battle_config.max_parameter &&
	    sd->status.agi  >= battle_config.max_parameter &&
	    sd->status.vit  >= battle_config.max_parameter &&
	    sd->status.int_ >= battle_config.max_parameter &&
	    sd->status.dex  >= battle_config.max_parameter &&
	    sd->status.luk  >= battle_config.max_parameter &&
	    sd->status.status_point != 0) {
		sd->status.status_point = 0;
		clif_updatestatus(sd, SP_STATUSPOINT);
	}
	clif_updatestatus(sd, type - SP_STR + SP_USTR);
	clif_updatestatus(sd, type);
	status_calc_pc(sd, 0);
	clif_statusupack(sd, type, 1, val);

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void pc_skillup(struct map_session_data *sd, short skill_num) {

	if (skill_num >= GD_SKILLBASE) {
		guild_skillup(sd, skill_num, 0);
		return;
	}

	if (skill_num < 0 || skill_num >= MAX_SKILL)
		return;
		
	if (sd->status.skill_point > 0 &&
	    sd->status.skill[skill_num].id != 0 &&
	    sd->status.skill[skill_num].lv < skill_tree_get_max(skill_num, sd->status.class)) {
		sd->status.skill[skill_num].lv++;
		sd->status.skill_point--;
		status_calc_pc(sd, 0);
		clif_skillup(sd, skill_num);
		clif_updatestatus(sd, SP_SKILLPOINT);
		clif_skillinfoblock(sd);
	}

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void pc_allskillup(struct map_session_data *sd) {

	int i, id;
	int c = 0, s = 0;
	struct pc_base_job s_class;

	nullpo_retv(sd);

	for(i = 0; i < MAX_SKILL; i++) {
		sd->status.skill[i].id = 0;
		// flag: 0 (normal), 1 (only card), 2-12 (card and skill (skill level +2)), 13 (cloneskill)
		if (sd->status.skill[i].flag && sd->status.skill[i].flag != 13) {
			sd->status.skill[i].lv = (sd->status.skill[i].flag == 1) ? 0 : sd->status.skill[i].flag - 2;
			sd->status.skill[i].flag = 0;
		}
	}

	s_class = pc_calc_base_job(sd->status.class);
	c = s_class.job;
	s = s_class.upper;

	// GM with all skills (any jobs)
	if (sd->GM_level >= battle_config.gm_allskill) {
		for(s = 0; s < 3; s++)
			for(c = 0; c < MAX_SKILL_TREE; c++)
				for(i = 0; i < MAX_SKILL_PER_TREE && (id = skill_tree[s][c][i].id) > 0; i++)
					if (sd->status.skill[id].flag != 13) { // flag: 0 (normal), 1 (only card), 2-12 (card and skill (skill level +2)), 13 (cloneskill)
						if (sd->GM_level >= battle_config.gm_all_skill_platinum || !(skill_get_inf2(id) & 0x01) ||
						    (s_class.job == 1 && (id == 142 || id == 143))) { // high novice free skills
							sd->status.skill[id].id = id;
							if (sd->status.skill[id].lv < skill_tree[s][c][i].max)
								sd->status.skill[id].lv = skill_tree[s][c][i].max;
						} else
							sd->status.skill[id].lv = 0;
					}

	// other players
	} else {
		// GM with all skills (actual job)
		if (sd->GM_level >= battle_config.gm_all_skill_job) {
			for(i = 0; i < MAX_SKILL_PER_TREE && (id = skill_tree[s][c][i].id) > 0; i++) {
				if (sd->status.skill[id].flag != 13) { // flag: 0 (normal), 1 (only card), 2-12 (card and skill (skill level +2)), 13 (cloneskill)
					if (sd->GM_level >= battle_config.gm_all_skill_platinum || !(skill_get_inf2(id) & 0x01) ||
					    (s == 1 && (id == 142 || id == 143))) { // high novice free skills
						sd->status.skill[id].id = id;
						sd->status.skill[id].lv = skill_tree[s][c][i].max;
					} else
						sd->status.skill[i].lv = 0;
				}
			}

		// normal player
		} else {
			for(i = 0; i < MAX_SKILL_PER_TREE && (id = skill_tree[s][c][i].id) > 0; i++) {
				if (sd->status.skill[id].flag != 13) { // flag: 0 (normal), 1 (only card), 2-12 (card and skill (skill level +2)), 13 (cloneskill)
					if (!(skill_get_inf2(id) & 0x01) || battle_config.quest_skill_learn ||
					    (s == 1 && (id == 142 || id == 143))) { // high novice free skills
						sd->status.skill[id].id = id;
						// sd->status.skill[id].lv = skill_get_max(id);
						//sd->status.skill[id].lv = skill_tree_get_max(id, sd->status.class);
						sd->status.skill[id].lv = skill_tree[s][c][i].max;
					}
				}
			}
		}
	}

	clif_skillinfoblock(sd);

	// remove skill points
	if (sd->status.skill_point > 0) {
		sd->status.skill_point = 0; // 0 skill points
		clif_updatestatus(sd, SP_SKILLPOINT); // update
	}

	status_calc_pc(sd, 0);

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void pc_resetlvl(struct map_session_data* sd, int type) {

	int i;

	nullpo_retv(sd);

	for(i=1;i<MAX_SKILL;i++) {
		sd->status.skill[i].lv = 0;
	}

	if (type == 1) {
		sd->status.skill_point=0;
		sd->status.base_level=1;
		sd->status.job_level=1;
		sd->status.base_exp=sd->status.base_exp=0;
		sd->status.job_exp=sd->status.job_exp=0;
		if (sd->status.option != 0)
			sd->status.option = 0;

		sd->status.str = 1;
		sd->status.agi = 1;
		sd->status.vit = 1;
		sd->status.int_ = 1;
		sd->status.dex = 1;
		sd->status.luk = 1;
		if (sd->status.class == 4001)
			sd->status.status_point = 100;
			pc_skill(sd, 142, 1, 0);
			pc_skill(sd, 143, 1, 0);
	} else

	if (type == 2) {
		sd->status.skill_point=0;
		sd->status.base_level=1;
		sd->status.job_level=1;
		sd->status.base_exp=0;
		sd->status.job_exp=0;
	} else
	if (type == 3) {
		sd->status.base_level=1;
		sd->status.base_exp=0;
	} else
	if (type == 4) {
		sd->status.job_level=1;
		sd->status.job_exp=0;
	}

	clif_updatestatus(sd,SP_STATUSPOINT);
	clif_updatestatus(sd,SP_STR);
	clif_updatestatus(sd,SP_AGI);
	clif_updatestatus(sd,SP_VIT);
	clif_updatestatus(sd,SP_INT);
	clif_updatestatus(sd,SP_DEX);
	clif_updatestatus(sd,SP_LUK);
	clif_updatestatus(sd,SP_BASELEVEL);
	clif_updatestatus(sd,SP_JOBLEVEL);
	clif_updatestatus(sd,SP_STATUSPOINT);
	clif_updatestatus(sd,SP_NEXTBASEEXP);
	clif_updatestatus(sd,SP_NEXTJOBEXP);
	clif_updatestatus(sd,SP_SKILLPOINT);

	clif_updatestatus(sd,SP_USTR);
	clif_updatestatus(sd,SP_UAGI);
	clif_updatestatus(sd,SP_UVIT);
	clif_updatestatus(sd,SP_UINT);
	clif_updatestatus(sd,SP_UDEX);
	clif_updatestatus(sd,SP_ULUK);

	for(i=0;i<11;i++) {
		if (sd->equip_index[i] >= 0)
			if (!pc_isequip(sd,sd->equip_index[i]))
				pc_unequipitem(sd, sd->equip_index[i], 2);
	}

	clif_skillinfoblock(sd);
	status_calc_pc(sd,0);

	if (sd->status.party_id > 0) {
		if (!(battle_config.basic_skill_check == 0 || (sd->status.skill[NV_BASIC].id == NV_BASIC && sd->status.skill[NV_BASIC].lv >= 5))) {
			party_member_leaved(sd->status.party_id, sd->status.account_id, sd->status.name);
		}
	}

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void pc_resetstate(struct map_session_data* sd) {

	int lv;

	#define sumsp(a) ((a)*(((a)-2)/10+2) - 5*(((a)-2)/10)*(((a)-2)/10) - 6*(((a)-2)/10) -2)

	nullpo_retv(sd);

	lv = sd->status.base_level < MAX_LEVEL ? sd->status.base_level : MAX_LEVEL;
	sd->status.status_point = statp[lv - 1];
	if (sd->status.class >= JOB_NOVICE_HIGH && sd->status.class < JOB_BABY)
		sd->status.status_point += 52;

	clif_updatestatus(sd, SP_STATUSPOINT);

	sd->status.str = 1;
	sd->status.agi = 1;
	sd->status.vit = 1;
	sd->status.int_ = 1;
	sd->status.dex = 1;
	sd->status.luk = 1;

	clif_updatestatus(sd,SP_STR);
	clif_updatestatus(sd,SP_AGI);
	clif_updatestatus(sd,SP_VIT);
	clif_updatestatus(sd,SP_INT);
	clif_updatestatus(sd,SP_DEX);
	clif_updatestatus(sd,SP_LUK);

	clif_updatestatus(sd,SP_USTR);
	clif_updatestatus(sd,SP_UAGI);
	clif_updatestatus(sd,SP_UVIT);
	clif_updatestatus(sd,SP_UINT);
	clif_updatestatus(sd,SP_UDEX);
	clif_updatestatus(sd,SP_ULUK);

	status_calc_pc(sd,0);

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void pc_resetskill(struct map_session_data* sd) {

	int i, skill;
	struct pc_base_job s_class;

	nullpo_retv(sd);

	if (sd->GM_level >= battle_config.gm_allskill || sd->GM_level >= battle_config.gm_all_skill_job)
		return;

	s_class = pc_calc_base_job(sd->status.class);

	for (i = 1; i < MAX_SKILL; i++) {
		if (i == NV_BASIC) { // never reset novice skills (it's necessary to not reset basic novice skill to conserv quest skills (battle_config.quest_skill_reset). status_calc_pc reset all skills after)
			if (!battle_config.quest_skill_learn || s_class.upper == 1) // if quest_skills are not learned or if 142 and 143 are high novice free skills
				continue;
			if (sd->status.skill[NV_BASIC].lv == 9) // quests skills must be learned -> reset only if no quests skills have been learned
				continue;
		}
		if ((skill = sd->status.skill[i].lv) > 0) {
			if (!(skill_get_inf2(i) & 0x01) || battle_config.quest_skill_learn) {
				if (!sd->status.skill[i].flag) // flag: 0 (normal), 1 (only card), 2-12 (card and skill (skill level +2)), 13 (cloneskill)
					sd->status.skill_point += skill;
				else if (sd->status.skill[i].flag > 2 && sd->status.skill[i].flag != 13)
					sd->status.skill_point += (sd->status.skill[i].flag - 2);
				sd->status.skill[i].lv = 0;
			} else if (battle_config.quest_skill_reset)
				sd->status.skill[i].lv = 0;
			sd->status.skill[i].flag = 0;
		} else {
			sd->status.skill[i].lv = 0;
		}
	}

	clif_updatestatus(sd, SP_SKILLPOINT);
	clif_skillinfoblock(sd);
	status_calc_pc(sd, 0);

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
int pc_damage(struct block_list *src, struct map_session_data *sd, int damage) {

	int i = 0, j = 0;
	unsigned int s_class;

	nullpo_retr(0, sd);

	s_class = pc_calc_base_job2(sd->status.class);

	if (pc_isdead(sd))
		return 0;

	if (pc_issit(sd)) {
		pc_setstand(sd);
		skill_gangsterparadise(sd, 0);
		skill_rest(sd,0);
	}

	if (sd->sc_data) {
		if (sd->sc_data[SC_GRAVITATION].timer != -1 && sd->sc_data[SC_GRAVITATION].val3 == BCT_SELF) {
			struct skill_unit_group *sg = (struct skill_unit_group *)sd->sc_data[SC_GRAVITATION].val4;
			if (sg) {
				skill_delunitgroup(sg);
				status_change_end(&sd->bl, SC_GRAVITATION, -1);
			}
		}
		if ((sd->sc_data[SC_BERSERK].timer != -1 || sd->special_state.infinite_endure) && !map[sd->bl.m].flag.gvg)
			;
		else if (sd->sc_data[SC_ENDURE].timer != -1 && (src != NULL && src->type == BL_MOB) && !map[sd->bl.m].flag.gvg) {
			if ((--sd->sc_data[SC_ENDURE].val2) < 0)
				status_change_end(&sd->bl, SC_ENDURE, -1);
		} else
			pc_stop_walking(sd, 2);
	}

	if (sd->sc_data[SC_DANCING].timer != -1 && damage > (sd->status.max_hp >> 2))
		skill_stop_dancing(&sd->bl, 0);

	sd->status.hp -= damage;
	if (sd->sc_data[SC_TRICKDEAD].timer != -1)
		status_change_end(&sd->bl, SC_TRICKDEAD, -1);
	if (sd->status.option&2)
		status_change_end(&sd->bl, SC_HIDING, -1);
	if (pc_iscloaking(sd))
		status_change_end(&sd->bl, SC_CLOAKING, -1);
	if (pc_ischasewalk(sd))
		status_change_end(&sd->bl, SC_CHASEWALK, -1);

	if (sd->status.pet_id > 0 && sd->pd && sd->petDB && battle_config.pet_damage_support)
		pet_target_check(sd, src, 1);

	if (sd->status.hp > 0) {
		clif_updatestatus(sd, SP_HP);

		if (sd->sc_data[SC_AUTOBERSERK].timer != -1 && sd->status.hp<sd->status.max_hp >> 2 &&
			(sd->sc_data[SC_PROVOKE].timer == -1 || sd->sc_data[SC_PROVOKE].val2 == 0))
			status_change_start(&sd->bl, SC_PROVOKE, 10, 1, 0, 0, 0, 0);

		sd->canlog_tick = gettick_cache;

		if (sd->status.party_id > 0) {
			struct party *p = party_search(sd->status.party_id);
			if (p != NULL)
				clif_party_hp(p, sd);
		}

		return 0;
	}

	sd->status.hp = 0;
	
	if (sd->sc_data && sd->sc_data[SC_KAIZEL].timer != -1 && map[sd->bl.m].flag.gvg == 0) {
		clif_skill_nodamage(&sd->bl,&sd->bl,ALL_RESURRECTION,4,1);
		sd->status.hp = sd->status.max_hp * sd->sc_data[SC_KAIZEL].val1 / 10;
		clif_updatestatus(sd,SP_HP);
		status_change_end(&sd->bl,SC_KAIZEL,-1);
		status_change_start(&sd->bl,SC_KYRIE,sd->sc_data[SC_KAIZEL].val1,0,0,0,2000,0);
		return 0;
	}

	if (sd->vender_id)
		vending_closevending(sd);

	if (sd->status.pet_id > 0 && sd->pd) {
		if (sd->petDB) {
			sd->pet.intimate -= sd->petDB->die;
			if (sd->pet.intimate < 0)
				sd->pet.intimate = 0;
			clif_send_petdata(sd, 1, sd->pet.intimate);
		}
	}

	pc_stop_walking(sd, 0);
	skill_castcancel(&sd->bl, 0, 0);
	clif_clearchar_area(&sd->bl, 1);
	pc_setdead(sd);
	skill_unit_move(&sd->bl, gettick_cache, 0);
	if (sd->sc_data[SC_BLADESTOP].timer != -1)
		status_change_end(&sd->bl, SC_BLADESTOP, -1);
	sd->status.die_counter++;
	status_change_clear(&sd->bl, 0);
	clif_updatestatus(sd, SP_HP);
	status_calc_pc(sd, 0);

	if (src && src->type == BL_PC) {
		struct map_session_data *ssd = (struct map_session_data *)src;
		if (ssd) {
			if (sd->state.event_death)
				pc_setglobalreg(sd, "killerrid", ssd->status.account_id);
			if (ssd->state.event_kill) {
				struct npc_data *npc;
				if ((npc = npc_name2id(script_config.kill_event_name))) {
					run_script(npc->u.scr.script, 0, sd->bl.id, npc->bl.id);
				}
			}
		}
	}

	if (sd->state.event_death) {
		struct npc_data *npc;
		if ((npc = npc_name2id(script_config.die_event_name))) {
			run_script(npc->u.scr.script, 0, sd->bl.id, npc->bl.id);
		}
	}

	if (battle_config.bone_drop == 2
	    || (battle_config.bone_drop == 1 && map[sd->bl.m].flag.pvp)) {
		struct item item_tmp;
		memset(&item_tmp, 0, sizeof(item_tmp));
		item_tmp.nameid = 7420;
		item_tmp.identify = 1;
		item_tmp.card[0] = 0x00fe;
		item_tmp.card[1] = 0;
		*((unsigned long *)(&item_tmp.card[2])) = sd->char_id;
		map_addflooritem(&item_tmp, 1, sd->bl.m, sd->bl.x, sd->bl.y, NULL, NULL, NULL, sd->bl.id, 0);
	}

	for(i = 0; i < 5; i++) {
		if (sd->dev.val1[i]) {
			status_change_end(&map_id2sd(sd->dev.val1[i])->bl, SC_DEVOTION, -1);
			sd->dev.val1[i] = sd->dev.val2[i] = 0;
		}
	}

	if (s_class == JOB_SUPER_NOVICE) {
		if ((i = pc_nextbaseexp(sd)) <= 0)
			i = sd->status.base_exp;
		if (i > 0 && (j = sd->status.base_exp * 1000 / i) >= 990 && j <= 1000)
			sd->state.snovice_flag = 3;
	}

	// Experience penalty upon death
	if (s_class != JOB_NOVICE && !map[sd->bl.m].flag.nopenalty && !map[sd->bl.m].flag.gvg) {
		if (battle_config.death_penalty_type_base > 0) {
			double death_penalty_base;
			if (sd->status.base_level >= battle_config.death_penalty_base5_lvl)
				death_penalty_base = (double)battle_config.death_penalty_base5;
			else if (sd->status.base_level >= battle_config.death_penalty_base4_lvl)
				death_penalty_base = (double)battle_config.death_penalty_base4;
			else if (sd->status.base_level >= battle_config.death_penalty_base3_lvl)
				death_penalty_base = (double)battle_config.death_penalty_base3;
			else if (sd->status.base_level >= battle_config.death_penalty_base2_lvl)
				death_penalty_base = (double)battle_config.death_penalty_base2;
			else
				death_penalty_base = (double)battle_config.death_penalty_base;
			if (death_penalty_base > 0) {
				if (battle_config.death_penalty_type_base == 1) {
					sd->status.base_exp -= (double)pc_nextbaseexp(sd) * death_penalty_base / 10000;
					if (battle_config.pk_mode && src && src->type == BL_PC)
						sd->status.base_exp -= (double)pc_nextbaseexp(sd) * death_penalty_base / 10000;
				} else if (battle_config.death_penalty_type_base == 2) {
					if (pc_nextbaseexp(sd) > 0) {
						sd->status.base_exp -= (double)sd->status.base_exp * death_penalty_base / 10000;
						if (battle_config.pk_mode && src && src->type == BL_PC)
							sd->status.base_exp -= (double)sd->status.base_exp * death_penalty_base / 10000;
					}
				}
			}
			clif_updatestatus(sd, SP_BASEEXP);
		}

		if (battle_config.death_penalty_type_job > 0) {
			double death_penalty_job;
			if (sd->status.job_level >= battle_config.death_penalty_job5_lvl)
				death_penalty_job = (double)battle_config.death_penalty_job5;
			else if (sd->status.job_level >= battle_config.death_penalty_job4_lvl)
				death_penalty_job = (double)battle_config.death_penalty_job4;
			else if (sd->status.job_level >= battle_config.death_penalty_job3_lvl)
				death_penalty_job = (double)battle_config.death_penalty_job3;
			else if (sd->status.job_level >= battle_config.death_penalty_job2_lvl)
				death_penalty_job = (double)battle_config.death_penalty_job2;
			else
				death_penalty_job = (double)battle_config.death_penalty_job;
			if (death_penalty_job > 0) {
				if (battle_config.death_penalty_type_job == 1) {
					sd->status.job_exp -= (double)pc_nextjobexp(sd) * death_penalty_job / 10000;
					if (battle_config.pk_mode && src && src->type == BL_PC)
						sd->status.job_exp -= (double)pc_nextjobexp(sd) * death_penalty_job / 10000;
				} else if (battle_config.death_penalty_type_job == 2) {
					if (pc_nextjobexp(sd) > 0) {
						sd->status.job_exp -= (double)sd->status.job_exp * death_penalty_job / 10000;
						if (battle_config.pk_mode && src && src->type == BL_PC)
							sd->status.job_exp -= (double)sd->status.job_exp * death_penalty_job / 10000;
					}
				}
			}
			clif_updatestatus(sd, SP_JOBEXP);
		}
	}

	if (map[sd->bl.m].flag.pvp_nightmaredrop) {
		for(j = 0; j < map[sd->bl.m].drop_list_num; j++) {
			int id = map[sd->bl.m].drop_list[j].drop_id;
			char type = map[sd->bl.m].drop_list[j].drop_type;
			int per = map[sd->bl.m].drop_list[j].drop_per;
			if (id == 0)
				continue;
			if (id == -1) {
				int eq_num = 0, eq_n[MAX_INVENTORY];
				memset(eq_n, 0, sizeof(eq_n));
				for(i = 0; i < MAX_INVENTORY; i++) {
					int k;
					if ((type == 1 && !sd->status.inventory[i].equip)
						|| (type == 2 && sd->status.inventory[i].equip)
						||  type == 3) {
						for(k = 0; k < MAX_INVENTORY; k++) {
							if (eq_n[k] <= 0) {
								eq_n[k] = i;
								break;
							}
						}
						eq_num++;
					}
				}
				if (eq_num > 0) {
					int n = eq_n[rand() % eq_num];
					if (rand() % 10000 < per) {
						if (sd->status.inventory[n].equip)
							pc_unequipitem(sd, n, 3);
						pc_dropitem(sd, n, 1);
					}
				}
			}
			else if (id > 0) {
				for(i = 0; i < MAX_INVENTORY; i++) {
					if (sd->status.inventory[i].nameid == id
					   && rand() % 10000 < per
					   && ((type == 1 && !sd->status.inventory[i].equip)
					       || (type == 2 && sd->status.inventory[i].equip)
					       || type == 3)) {
						if (sd->status.inventory[i].equip)
							pc_unequipitem(sd, i, 3);
						pc_dropitem(sd, i, 1);
						break;
					}
				}
			}
		}
	}

	if (map[sd->bl.m].flag.pvp && !battle_config.pk_mode) {
		if (!map[sd->bl.m].flag.pvp_nocalcrank) {
			sd->pvp_point-=5;
			if (src && src->type == BL_PC)
				((struct map_session_data *)src)->pvp_point++;
			pc_setdead(sd);
		}
		if (sd->pvp_point < 0) {
			sd->pvp_point = 0;
			pc_setstand(sd);
			pc_setrestartvalue(sd, 3);
			pc_setpos(sd, sd->status.save_point.map, sd->status.save_point.x, sd->status.save_point.y, 0, 0);
		}
	}

	if (map[sd->bl.m].flag.gvg) {
		pc_setstand(sd);
		pc_setrestartvalue(sd, 3);
		pc_setpos(sd, sd->status.save_point.map, sd->status.save_point.x, sd->status.save_point.y, 0, 0);
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int pc_readparam(struct map_session_data *sd,int type) {

	int val = 0;
	int job;
	struct pc_base_job s_class;

	s_class = pc_calc_base_job(sd->status.class);

	nullpo_retr(0, sd);

	switch(type) {
	case SP_SKILLPOINT:
		val= sd->status.skill_point;
		break;
	case SP_STATUSPOINT:
		val= sd->status.status_point;
		break;
	case SP_ZENY:
		val= sd->status.zeny;
		break;
	case SP_BASELEVEL:
		val= sd->status.base_level;
		break;
	case SP_JOBLEVEL:
		val= sd->status.job_level;
		break;
	case SP_CLASS:
		val= sd->status.class;
		break;
	case SP_BASEJOB:
		// Fixes for the new classes (Taekwon, Gunslinger, Ninja, Death Knight, Dark Collector)
		if ((sd->status.class >= 24 && sd->status.class <= 25) || (sd->status.class >= 4046 && sd->status.class <= 4053))
			val= sd->status.class;
		else
			val= s_class.job;
		break;
	case SP_BASECLASS:
		// Fixes for the new classes (Taekwon, Gunslinger, Ninja, Death Knight, Dark Collector)
		if ((sd->status.class >= 24 && sd->status.class <= 25) || (sd->status.class >= 4046 && sd->status.class <= 4053))
			job = sd->status.class;
		else
			job = s_class.job;
		// Main Procedure
		switch(job) {
			// Novice Class
			case JOB_NOVICE:
			case JOB_SUPER_NOVICE:
				val = JOB_NOVICE;
				break;
			// Swordsman Class
			case JOB_SWORDMAN:
			case JOB_KNIGHT:
			case JOB_KNIGHT2:
			case JOB_CRUSADER:
			case JOB_CRUSADER2:
				val= JOB_SWORDMAN;
				break;
			// Mage Class
			case JOB_MAGE:
			case JOB_WIZARD:
			case JOB_SAGE:
				val= JOB_MAGE;
				break;
			// Archer Class
			case JOB_ARCHER:
			case JOB_HUNTER:
			case JOB_BARD:
			case JOB_DANCER:
				val= JOB_ARCHER;
				break;
			// Acolyte Class
			case JOB_ACOLYTE:
			case JOB_PRIEST:
			case JOB_MONK:
				val= JOB_ACOLYTE;
				break;
			// Merchant Class
			case JOB_MERCHANT:
			case JOB_BLACKSMITH:
			case JOB_ALCHEMIST:
				val= JOB_MERCHANT;
				break;
			// Thief Class
			case JOB_THIEF:
			case JOB_ASSASSIN:
			case JOB_ROGUE:
				val= JOB_THIEF;
				break;
			// Taekwon Class
			case JOB_TAEKWON:
			case JOB_STAR_GLADIATOR:
			case JOB_STAR_GLADIATOR2:
			case JOB_SOUL_LINKER:
				val= JOB_TAEKWON;
				break;
			default:
				val= job;
		}
		break;
	case SP_UPPER:
		val= s_class.upper;
		break;
	case SP_SEX:
		val= sd->sex;
		break;
	case SP_WEIGHT:
		val= sd->weight;
		break;
	case SP_MAXWEIGHT:
		val= sd->max_weight;
		break;
	case SP_BASEEXP:
		val= sd->status.base_exp;
		break;
	case SP_JOBEXP:
		val= sd->status.job_exp;
		break;
	case SP_NEXTBASEEXP:
		val= pc_nextbaseexp(sd);
		break;
	case SP_NEXTJOBEXP:
		val= pc_nextjobexp(sd);
		break;
	case SP_HP:
		val= sd->status.hp;
		break;
	case SP_MAXHP:
		val= sd->status.max_hp;
		break;
	case SP_SP:
		val= sd->status.sp;
		break;
	case SP_MAXSP:
		val= sd->status.max_sp;
		break;
	case SP_STR:
		val= sd->status.str;
		break;
	case SP_AGI:
		val= sd->status.agi;
		break;
	case SP_VIT:
		val= sd->status.vit;
		break;
	case SP_INT:
		val= sd->status.int_;
		break;
	case SP_DEX:
		val= sd->status.dex;
		break;
	case SP_LUK:
		val= sd->status.luk;
		break;
	case SP_KARMA:
		val = sd->status.karma;
		break;
	case SP_MANNER:
		val = sd->status.manner;
		break;
	}

	return val;
}

/*==========================================
 *
 *------------------------------------------
 */
void pc_setparam(struct map_session_data *sd,int type,int val) {

	int i = 0,up_level = 50;
	struct pc_base_job s_class;

	nullpo_retv(sd);

	s_class = pc_calc_base_job(sd->status.class);

	switch(type) {
	case SP_BASELEVEL:
		if (val > sd->status.base_level) {
			for (i = 1; i <= (val - sd->status.base_level); i++)
				sd->status.status_point += (sd->status.base_level + i + 14) / 5;
		}
		sd->status.base_level = val;
		sd->status.base_exp = 0;
		clif_updatestatus(sd, SP_BASELEVEL);
		clif_updatestatus(sd, SP_NEXTBASEEXP);
		clif_updatestatus(sd, SP_STATUSPOINT);
		clif_updatestatus(sd, SP_BASEEXP);
		status_calc_pc(sd, 0);
		pc_heal(sd, sd->status.max_hp, sd->status.max_sp);
		break;
	case SP_JOBLEVEL:
		if (s_class.job == 0)
			up_level -= 40;
		else if (s_class.job == 23)
			up_level += 49;
		else if (sd->status.class >= 4008 && sd->status.class <= 4022)
			up_level += 20;
		else if (sd->status.class == 24 || sd->status.class == 25)
			up_level += 20;
		if (val >= sd->status.job_level) {
			if (val > up_level)val = up_level;
			sd->status.skill_point += (val-sd->status.job_level);
		sd->status.job_level = val;
			sd->status.job_exp = 0;
			clif_updatestatus(sd, SP_JOBLEVEL);
			clif_updatestatus(sd, SP_NEXTJOBEXP);
			clif_updatestatus(sd, SP_JOBEXP);
			clif_updatestatus(sd, SP_SKILLPOINT);
			status_calc_pc(sd, 0);
			clif_misceffect(&sd->bl, 1);
		} else {
			sd->status.job_level = val;
			sd->status.job_exp = 0;
			clif_updatestatus(sd, SP_JOBLEVEL);
			clif_updatestatus(sd, SP_NEXTJOBEXP);
			clif_updatestatus(sd, SP_JOBEXP);
			status_calc_pc(sd, 0);
		}
		clif_updatestatus(sd,type);
		break;
	case SP_SKILLPOINT:
		sd->status.skill_point = val;
		break;
	case SP_STATUSPOINT:
		sd->status.status_point = val;
		break;
	case SP_ZENY:
		if (val > MAX_ZENY)
			val = MAX_ZENY;
		sd->status.zeny = val;
		break;
	case SP_BASEEXP:
		if (pc_nextbaseexp(sd) > 0) {
			sd->status.base_exp = val;
			pc_checkbaselevelup(sd);
		}
		break;
	case SP_JOBEXP:
		if (pc_nextjobexp(sd) > 0) {
			sd->status.job_exp = val;
			pc_checkjoblevelup(sd);
		}
		break;
	case SP_SEX:
		sd->sex = val;
		break;
	case SP_WEIGHT:
		sd->weight = val;
		break;
	case SP_MAXWEIGHT:
		sd->max_weight = val;
		break;
	case SP_HP:
		sd->status.hp = val;
		break;
	case SP_MAXHP:
		sd->status.max_hp = val;
		break;
	case SP_SP:
		sd->status.sp = val;
		break;
	case SP_MAXSP:
		sd->status.max_sp = val;
		break;
	case SP_STR:
		sd->status.str = val;
		break;
	case SP_AGI:
		sd->status.agi = val;
		break;
	case SP_VIT:
		sd->status.vit = val;
		break;
	case SP_INT:
		sd->status.int_ = val;
		break;
	case SP_DEX:
		sd->status.dex = val;
		break;
	case SP_LUK:
		sd->status.luk = val;
		break;
	case SP_KARMA:
		sd->status.karma = val;
		break;
	case SP_MANNER:
		sd->status.manner = val;
		break;
	}
	clif_updatestatus(sd, type);

	return;
}

/*==========================================
 * HP/SP回復
 *------------------------------------------
 */
int pc_heal(struct map_session_data *sd,int hp,int sp) {

	nullpo_retr(0, sd);

	if (pc_checkoverhp(sd)) {
		if (hp > 0)
			hp = 0;
	}
	if (pc_checkoversp(sd)) {
		if (sp > 0)
			sp = 0;
	}

	if (sd->sc_data[SC_BERSERK].timer != -1)
		return 0;

	if (hp + sd->status.hp > sd->status.max_hp)
		hp = sd->status.max_hp - sd->status.hp;
	if (sp + sd->status.sp > sd->status.max_sp)
		sp = sd->status.max_sp - sd->status.sp;
	sd->status.hp += hp;
	if (sd->status.hp <= 0) {
		sd->status.hp = 0;
		pc_damage(NULL, sd, 1);
		hp = 0;
	}
	sd->status.sp += sp;
	if (sd->status.sp <= 0)
		sd->status.sp = 0;
	if (hp)
		clif_updatestatus(sd, SP_HP);
	if (sp)
		clif_updatestatus(sd, SP_SP);

	if (sd->status.party_id > 0) {
		struct party *p = party_search(sd->status.party_id);
		if (p != NULL)
			clif_party_hp(p, sd);
	}

	return hp + sp;
}

/*==========================================
 *
 *------------------------------------------
 */
void pc_itemheal(struct map_session_data *sd, int hp, int sp) {

	int bonus;

	nullpo_retv(sd);

	if (sd->state.potionpitcher_flag) {
		sd->potion_hp = hp;
		sd->potion_sp = sp;
		return;
	}

	if (pc_checkoverhp(sd)) {
		if (hp > 0)
			hp = 0;
	}
	if (pc_checkoversp(sd)) {
		if (sp > 0)
			sp = 0;
	}
	
	if (hp > 0) {
		unsigned int use_itemtype;
		switch(use_itemid) {
			//501: Red potion, 503: Yellow Potion, 504: White Potion (HP Potions)
			case 501:	case 503:	case 504:	use_itemtype = 1;	break;
			//507: Red Herb, 508: Yellow Herb, 509: White_Herb, (Herbs)
			case 507:	case 508:	case 509:	use_itemtype = 2;	break;
			//512: Apple, 513: Banana, 514: Grape, 515: Carrot, 516: Sweet Potato (Fruits)
			case 512:	case 513:	case 514:	case 515:	case 516:	use_itemtype = 3;	break;
			//517: Meat, 528: Monster's_Feed (Meats)
			case 517:	case 528:	use_itemtype = 4;	break;
			//529: Candy, 530: Candy Cane (Candy)
			case 529:	case 530:	use_itemtype = 5;	break;
			//531: Apple Juice, 532: Banana Juice, 534: Carrot Juice (HP Juices)
			case 531:	case 532:	case 534:	use_itemtype = 6; break;
			//544: Raw Fish, 551: Sushi (Sashimi)
			case 544: case 551: use_itemtype = 7; break;
			default: use_itemtype = 0; break;
		}

		bonus = 100 + (sd->paramc[2] << 1) + pc_checkskill(sd, SM_RECOVERY)*10	+ pc_checkskill(sd, AM_LEARNINGPOTION)*5;
		if (use_nameditem && ranking_id2rank(use_nameditem, RK_ALCHEMIST)) {
			bonus += 50; // A potion produced by an Alchemist in the Fame Top 10 gets +50% effect
			if (use_itemid == 504 && sd->sc_data && sd->sc_data[SC_SPIRIT].timer != -1 && sd->sc_data[SC_SPIRIT].val2 == SL_ROGUE)
				bonus += 50;
		}

		if (use_itemtype > 0) // int itemhealrate[7];
			bonus = bonus * (100 + sd->itemhealrate[use_itemtype - 1]) / 100;

		if (bonus != 100)
			hp = hp * bonus / 100;
	}
	if (sp > 0) {
		bonus = (sd->paramc[3] << 1) + 100 + pc_checkskill(sd, MG_SRECOVERY) * 10
		      + pc_checkskill(sd, AM_LEARNINGPOTION) * 5;

		if (use_nameditem && ranking_id2rank(use_nameditem, RK_ALCHEMIST))
			bonus += 50; // A potion produced by an Alchemist in the Fame Top 10 gets +50% effect*

		if (bonus != 100)
			sp = sp * bonus / 100;
	}
	if (hp + sd->status.hp > sd->status.max_hp)
		hp = sd->status.max_hp - sd->status.hp;
	if (sp + sd->status.sp > sd->status.max_sp)
		sp = sd->status.max_sp - sd->status.sp;
	sd->status.hp += hp;
	if (sd->status.hp <= 0) {
		sd->status.hp = 0;
		pc_damage(NULL, sd, 1);
		hp = 0;
	}
	sd->status.sp += sp;
	if (sd->status.sp <= 0)
		sd->status.sp = 0;
	if (hp)
		clif_updatestatus(sd, SP_HP);
	if (sp)
		clif_updatestatus(sd, SP_SP);

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void pc_percentheal(struct map_session_data *sd,int hp,int sp) {

	nullpo_retv(sd);

	if (sd->state.potionpitcher_flag) {
		sd->potion_per_hp = hp;
		sd->potion_per_sp = sp;
		return;
	}

	if (pc_checkoverhp(sd)) {
		if (hp > 0)
			hp = 0;
	}
	if (pc_checkoversp(sd)) {
		if (sp > 0)
			sp = 0;
	}
	if (hp) {
		if (hp >= 100) {
			sd->status.hp = sd->status.max_hp;
		}
		else if (hp <= -100) {
			sd->status.hp = 0;
			pc_damage(NULL, sd, 1);
		}
		else {
			sd->status.hp += sd->status.max_hp*hp/100;
			if (sd->status.hp > sd->status.max_hp)
				sd->status.hp = sd->status.max_hp;
			if (sd->status.hp <= 0) {
				sd->status.hp = 0;
				pc_damage(NULL, sd, 1);
				hp = 0;
			}
		}
	}
	if (sp) {
		if (sp >= 100) {
			sd->status.sp = sd->status.max_sp;
		}
		else if(sp <= -100) {
			sd->status.sp = 0;
		}
		else {
			sd->status.sp += sd->status.max_sp*sp/100;
			if (sd->status.sp > sd->status.max_sp)
				sd->status.sp = sd->status.max_sp;
			if (sd->status.sp < 0)
				sd->status.sp = 0;
		}
	}
	if (hp)
		clif_updatestatus(sd,SP_HP);
	if (sp)
		clif_updatestatus(sd,SP_SP);

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
int pc_jobchange(struct map_session_data *sd, int job, int upper) {

	int i;
	int b_class = 0;

	struct pc_base_job s_class = pc_calc_base_job(sd->status.class);

	nullpo_retr(0, sd);

	// Fixes invalid upper types
	if (upper < 0 || upper > 2)
		upper = s_class.upper;

	b_class = job;
	// Normalizes Advanced/Baby Jobs
	if (job < JOB_SUPER_NOVICE) {
		if (upper == 1)
			b_class += JOB_NOVICE_HIGH;
		else if (upper == 2)
			b_class += JOB_BABY;
	// Normailzes Baby Super Novice
	} else if (job == JOB_SUPER_NOVICE)	{
		if (upper == 1) // No such thing as an Advanced Super Novice, so fail
			return 1;
		else if (upper == 2)
			b_class += 4022;
	// Incompatible/non-existant jobs, function fails
	} else if (job == JOB_WEDDING || (job > JOB_NINJA && job < JOB_NOVICE_HIGH) || job > MAX_PC_CLASS) {
		return 1;
	}

	job = pc_calc_base_job2(b_class); // Check base class

	// Normalizes Taekwon Class compatibility for this part of the function
	if (job >= 24 && job <= 27) {
		job += 4022;
		s_class.job += 4022;
	}

	// If class is Dancer and gender is male, and vice-versa, fail
	if ((sd->status.sex == 0 && job == 19) || (sd->status.sex == 1 && job == 20) ||
		job == 22 || sd->status.class == b_class)
		return 1;

	// Check if we are changing from 1st to 2nd job
	if ((job >= 7 && job <= 21) || (job >= 4008 && job <= 4022) || (job >= 4030 && job <= 4044) ||
		(job >= 4047 && job <= 4049)) {
		if ((s_class.job > 0 && s_class.job < 7) || (s_class.job > 4001 && s_class.job < 4008) ||
			(s_class.job > 4023 && s_class.job < 4030) || s_class.job == 4046)
			sd->change_level = sd->status.job_level;
		else
			sd->change_level = 40;
	} else
		sd->change_level = 0;

	// Begin job change process
	pc_setglobalreg(sd, "jobchange_level", sd->change_level);

	sd->status.class = sd->view_class = b_class; // Changes job

	sd->status.job_level = 1; // Resets job level
	sd->status.job_exp = 0; // Resets job experience
	clif_updatestatus(sd, SP_JOBLEVEL); // Updates client's job level display
	clif_updatestatus(sd, SP_JOBEXP); // Updates client's job exp display
	clif_updatestatus(sd, SP_NEXTJOBEXP); // Updates client's next job exp display

	for(i=0;i<11;i++) { // Removes new job-incompatible items
		if (sd->equip_index[i] >= 0)
			if (!pc_isequip(sd, sd->equip_index[i]))
				pc_unequipitem(sd, sd->equip_index[i], 2);
	}

	clif_changelook(&sd->bl, LOOK_BASE, sd->view_class); // Move sprite update to prevent client crashes with incompatible equipment [Valaris]
	if (sd->status.clothes_color > 0) // Updates client's clothing palette
		clif_changelook(&sd->bl, LOOK_CLOTHES_COLOR, sd->status.clothes_color);
	if (battle_config.muting_players && sd->status.manner < 0) // Updates manner status
		clif_changestatus(&sd->bl, SP_MANNER, sd->status.manner);

	status_calc_pc(sd, 0); // Recalculates the player's status
	pc_checkallowskill(sd); // Removes statuses that are incompatible with your new job/weapon
	pc_equiplookall(sd); // Updates client's display of weapons, shoes, and all headgear types
	clif_equiplist(sd); // Updates inventory and sends client updated info via packets

	if (pc_isriding(sd)) { // Removes Peco status if new job is incompatible with Pecos
		if (pc_checkskill(sd, KN_RIDING) <= 0)
			pc_setoption(sd, sd->status.option & ~0x0020);
		else
			pc_setriding(sd);
	}

	// Save player immediately (job changing is important)
	chrif_save(sd); // Do pc_makesavestatus and save storage + account_reg/account_reg2 too

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
void pc_equiplookall(struct map_session_data *sd) {

	nullpo_retv(sd);

	clif_changelook(&sd->bl,LOOK_WEAPON,0);
	clif_changelook(&sd->bl,LOOK_SHOES,0);
	clif_changelook(&sd->bl,LOOK_HEAD_BOTTOM,sd->status.head_bottom);
	clif_changelook(&sd->bl,LOOK_HEAD_TOP,sd->status.head_top);
	clif_changelook(&sd->bl,LOOK_HEAD_MID,sd->status.head_mid);

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void pc_changelook(struct map_session_data *sd,int type,int val) {

	nullpo_retv(sd);

	switch(type) {
	case LOOK_HAIR:
		sd->status.hair=val;
		break;
	case LOOK_WEAPON:
		sd->status.weapon=val;
		break;
	case LOOK_HEAD_BOTTOM:
		sd->status.head_bottom=val;
		break;
	case LOOK_HEAD_TOP:
		sd->status.head_top=val;
		break;
	case LOOK_HEAD_MID:
		sd->status.head_mid=val;
		break;
	case LOOK_HAIR_COLOR:
		sd->status.hair_color=val;
		break;
	case LOOK_CLOTHES_COLOR:
		sd->status.clothes_color=val;
		break;
	case LOOK_SHIELD:
		sd->status.shield=val;
		break;
	case LOOK_SHOES:
		break;
	}
	clif_changelook(&sd->bl,type,val);

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void pc_setoption(struct map_session_data *sd, short type) {

	nullpo_retv(sd);

	sd->status.option = type;
	clif_changeoption(&sd->bl);
	status_calc_pc(sd, 0);

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void pc_setcart(struct map_session_data *sd, unsigned short type) {

	short cart[6] = {0x0000, 0x0008, 0x0080, 0x0100, 0x0200, 0x0400};

	if (type > 5)
		return;

	if (pc_checkskill(sd, MC_PUSHCART) > 0) {
		if (!pc_iscarton(sd)) {
			pc_setoption(sd, sd->status.option | cart[type]);
			clif_cart_itemlist(sd);
			clif_cart_equiplist(sd);
			clif_updatestatus(sd, SP_CARTINFO);
			clif_status_change(&sd->bl, 0x0c, 0);
		} else {
			sd->status.option &= ~((short)CART_MASK);
			pc_setoption(sd, sd->status.option | cart[type]);
		}
	}

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void pc_setfalcon(struct map_session_data *sd) {

	if (pc_checkskill(sd, HT_FALCON) > 0)
		pc_setoption(sd, sd->status.option|0x0010);

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void pc_setriding(struct map_session_data *sd) {

	// Peco / Disguise crash fix
	if (sd->disguise > 0) {
		clif_displaymessage(sd->fd, msg_txt(534));
		return;
	}

	// Only change sprite if the player has the appropriate Peco Riding skill
	if ((pc_checkskill(sd, KN_RIDING) > 0)) {
		pc_setoption(sd, sd->status.option | 0x0020);

		// Normal classes
		switch(sd->status.class) {
			case JOB_KNIGHT:
				sd->status.class = sd->view_class = JOB_KNIGHT2;
				break;
			case JOB_CRUSADER:
				sd->status.class = sd->view_class = JOB_CRUSADER2;
				break;
			case JOB_LORD_KNIGHT:
				sd->status.class = sd->view_class = JOB_LORD_KNIGHT2;
				break;
			case JOB_PALADIN:
				sd->status.class = sd->view_class = JOB_PALADIN2;
				break;
			case JOB_BABY_KNIGHT:
				sd->status.class = sd->view_class = JOB_BABY_KNIGHT2;
				break;
			case JOB_BABY_CRUSADER:
				sd->status.class = sd->view_class = JOB_BABY_CRUSADER2;
				break;
		}
	}

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
int pc_candrop(struct map_session_data *sd, int item_id) {

	if (sd->GM_level > 0 && sd->GM_level < battle_config.gm_can_drop_lv)
		return 1;

	return (itemdb_isdropable(item_id, sd->GM_level));
}

/*==========================================
 *
 *------------------------------------------
 */
int pc_readreg(struct map_session_data *sd, int reg) {

	int i;

	nullpo_retr(0, sd);

	for(i = 0; i < sd->reg_num; i++)
		if (sd->reg[i].index == reg)
			return sd->reg[i].data;

	return 0;
}

/*==========================================
 * script用変数の値を設定
 *------------------------------------------
 */
int pc_setreg(struct map_session_data *sd,int reg,int val) {

	int i;

	nullpo_retr(0, sd);

	for (i = 0; i < sd->reg_num; i++) {
		if (sd->reg[i].index == reg) {
			sd->reg[i].data = val;
			return 0;
		}
	}
	sd->reg_num++;
	REALLOC(sd->reg, struct script_reg, sd->reg_num);
	memset(sd->reg + (sd->reg_num - 1), 0, sizeof(struct script_reg));
	sd->reg[i].index = reg;
	sd->reg[i].data = val;

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
char *pc_readregstr(struct map_session_data *sd,int reg) {

	int i;

	nullpo_retr(0, sd);

	for(i=0;i<sd->regstr_num;i++)
		if (sd->regstr[i].index==reg)
			return sd->regstr[i].data;

	return NULL;
}

/*==========================================
 *
 *------------------------------------------
 */
int pc_setregstr(struct map_session_data *sd,int reg,char *str) {

	int i;

	nullpo_retr(0, sd);

	if (strlen(str)+1 >= sizeof(sd->regstr[0].data)) {
		printf("pc_setregstr: string too long !\n");
		return 0;
	}

	for(i=0;i<sd->regstr_num;i++)
		if (sd->regstr[i].index==reg) {
			strcpy(sd->regstr[i].data,str);
			return 0;
		}

	sd->regstr_num++;
	REALLOC(sd->regstr, struct script_regstr, sd->regstr_num);
	memset(sd->regstr + (sd->regstr_num - 1), 0, sizeof(struct script_regstr));
	sd->regstr[i].index = reg;
	strcpy(sd->regstr[i].data, str);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int pc_readglobalreg(struct map_session_data *sd, char *reg) {

	int i;

	nullpo_retr(0, sd);

	for(i = 0; i < sd->global_reg_num; i++) {
		if (strcmp(sd->global_reg[i].str, reg) == 0)
			return sd->global_reg[i].value;
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
void pc_setglobalreg(struct map_session_data *sd, char *reg, int val) {

	int i;

	nullpo_retv(sd);

	if (reg[0] == '\0')
		return;

	if (strcmp(reg, script_config.die_event_name) == 0)
		sd->state.event_death = val;
	else if (strcmp(reg, script_config.kill_event_name) == 0)
		sd->state.event_kill = val;
	else if (strcmp(reg, script_config.logout_event_name) == 0)
		sd->state.event_disconnect = val;

	if (val == 0) {
		for(i = 0; i < sd->global_reg_num; i++) {
			if (strcmp(sd->global_reg[i].str, reg) == 0) {
				if (sd->global_reg_num > 1) {
					if (i != sd->global_reg_num - 1) {
						memcpy(&sd->global_reg[i], &sd->global_reg[sd->global_reg_num - 1], sizeof(struct global_reg));
						memset(&sd->global_reg[sd->global_reg_num - 1], 0, sizeof(struct global_reg));
					}
					REALLOC(sd->global_reg, struct global_reg, sd->global_reg_num - 1);
					sd->global_reg_num--;
				} else {
					FREE(sd->global_reg);
					sd->global_reg_num = 0;
				}
				sd->global_reg_dirty = 1;
				break;
			}
		}
		return;
	}

	for(i = 0; i < sd->global_reg_num; i++) {
		if (strcmp(sd->global_reg[i].str, reg) == 0) {
			if (sd->global_reg[i].value != val) {
				sd->global_reg[i].value = val;
				sd->global_reg_dirty = 1;
			}
			return;
		}
	}

	if (sd->global_reg_num < GLOBAL_REG_NUM) {
		if (sd->global_reg_num == 0) {
			MALLOC(sd->global_reg, struct global_reg, 1);
		} else {
			REALLOC(sd->global_reg, struct global_reg, sd->global_reg_num + 1);
		}
		memset(&sd->global_reg[sd->global_reg_num], 0, sizeof(struct global_reg));
		strncpy(sd->global_reg[sd->global_reg_num].str, reg, 32);
		sd->global_reg[sd->global_reg_num].value = val;
		sd->global_reg_num++;
		sd->global_reg_dirty = 1;
		return;
	}

	if (battle_config.error_log)
		printf("pc_setglobalreg : couldn't set %s (GLOBAL_REG_NUM = %d)\n", reg, GLOBAL_REG_NUM);

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
int pc_readaccountreg(struct map_session_data *sd, char *reg) {

	int i;

	for(i = 0; i < sd->account_reg_num; i++) {
		if (strcmp(sd->account_reg[i].str, reg) == 0)
			return sd->account_reg[i].value;
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
void pc_setaccountreg(struct map_session_data *sd, char *reg, int val) {

	int i;

	nullpo_retv(sd);

	if (val == 0) {
		for(i = 0; i < sd->account_reg_num; i++) {
			if (strcmp(sd->account_reg[i].str, reg) == 0) {
				if (sd->account_reg_num > 1) {
					if (i != sd->account_reg_num - 1)
						memcpy(&sd->account_reg[i], &sd->account_reg[sd->account_reg_num - 1], sizeof(struct global_reg));
					REALLOC(sd->account_reg, struct global_reg, sd->account_reg_num - 1);
					sd->account_reg_num--;
				} else {
					FREE(sd->account_reg);
					sd->account_reg_num = 0;
				}
				sd->account_reg_dirty = 1;
				break;
			}
		}
		return;
	}

	for(i = 0; i < sd->account_reg_num; i++) {
		if (strcmp(sd->account_reg[i].str, reg) == 0) {
			if (sd->account_reg[i].value != val) {
				sd->account_reg[i].value = val;
				sd->account_reg_dirty = 1;
			}
			return;
		}
	}

	if (sd->account_reg_num < ACCOUNT_REG_NUM) {
		if (sd->account_reg_num == 0) {
			MALLOC(sd->account_reg, struct global_reg, 1);
		} else {
			REALLOC(sd->account_reg, struct global_reg, sd->account_reg_num + 1);
		}
		strncpy(sd->account_reg[sd->account_reg_num].str, reg, 32);
		sd->account_reg[sd->account_reg_num].str[32] = '\0';
		sd->account_reg[sd->account_reg_num].value = val;
		sd->account_reg_num++;
		sd->account_reg_dirty = 1;
		return;
	}

	if (battle_config.error_log)
			printf("pc_setaccountreg : couldn't set %s (ACCOUNT_REG_NUM = %d)\n", reg, ACCOUNT_REG_NUM);

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
int pc_readaccountreg2(struct map_session_data *sd, char *reg) {

	int i;

	for(i = 0; i < sd->account_reg2_num; i++) {
		if (strcmp(sd->account_reg2[i].str, reg) == 0)
			return sd->account_reg2[i].value;
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
void pc_setaccountreg2(struct map_session_data *sd, char *reg, int val) {

	int i;

	nullpo_retv(sd);

	if (val == 0) {
		for(i = 0; i < sd->account_reg2_num; i++) {
			if (strcmp(sd->account_reg2[i].str, reg) == 0) {
				if (sd->account_reg2_num > 1) {
					if (i != sd->account_reg2_num - 1)
						memcpy(&sd->account_reg2[i], &sd->account_reg2[sd->account_reg2_num - 1], sizeof(struct global_reg));
					REALLOC(sd->account_reg2, struct global_reg, sd->account_reg2_num - 1);
					sd->account_reg2_num--;
				} else {
					FREE(sd->account_reg2);
					sd->account_reg2_num = 0;
				}
				sd->account_reg2_dirty = 1;
				return;
			}
		}
		return;
	}

	for(i = 0; i < sd->account_reg2_num; i++) {
		if (strcmp(sd->account_reg2[i].str, reg) == 0) {
			if (sd->account_reg2[i].value != val) {
				sd->account_reg2[i].value = val;
				sd->account_reg2_dirty = 1;
			}
			return;
		}
	}

	if (sd->account_reg2_num < ACCOUNT_REG2_NUM) {
		if (sd->account_reg2_num == 0) {
			MALLOC(sd->account_reg2, struct global_reg, 1);
		} else {
			REALLOC(sd->account_reg2, struct global_reg, sd->account_reg2_num + 1);
		}
		strncpy(sd->account_reg2[sd->account_reg2_num].str, reg, 32);
		sd->account_reg2[sd->account_reg2_num].str[32] = '\0';
		sd->account_reg2[sd->account_reg2_num].value = val;
		sd->account_reg2_num++;
		sd->account_reg2_dirty = 1;
		return;
	}

	if (battle_config.error_log)
		printf("pc_setaccountreg2 : couldn't set %s (ACCOUNT_REG2_NUM = %d)\n", reg, ACCOUNT_REG2_NUM);

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
int pc_eventtimer(int tid, unsigned int tick, int id, int data) {

	struct map_session_data *sd = map_id2sd(id);
	char *evname;
	int i;

	if (sd == NULL)
		return 0;

	evname = (char *)data;
	for(i = 0; i < MAX_EVENTTIMER; i++) {
		if (sd->eventtimer[i] == tid) {
			sd->eventtimer[i] = -1;
			npc_event(sd, evname, 0);
			break;
		}
	}
	FREE(evname);
	if (i == MAX_EVENTTIMER) {
		if (battle_config.error_log)
			printf("pc_eventtimer: no such event timer.\n");
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
void pc_addeventtimer(struct map_session_data *sd, int tick, const char *name) {

	int i;
	char *evname;

	nullpo_retv(sd);

	for(i = 0; i < MAX_EVENTTIMER; i++)
		if (sd->eventtimer[i] == -1)
			break;
	if (i < MAX_EVENTTIMER) {
		CALLOC(evname, char, 25); // 24 + NULL
		strncpy(evname, name, 24);
		sd->eventtimer[i] = add_timer(gettick_cache + tick, pc_eventtimer, sd->bl.id, (int)evname);
	} else {
		if (battle_config.error_log)
			printf("pc_addtimer: event timer is full !\n");
	}

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void pc_deleventtimer(struct map_session_data *sd, const char *name) {

	int i;
	char *evname;

	nullpo_retv(sd);

	for(i = 0; i < MAX_EVENTTIMER; i++)
		if (sd->eventtimer[i] != -1) {
			evname = (char *)(get_timer(sd->eventtimer[i])->data);
			if (strcmp(evname, name) == 0) {
				delete_timer(sd->eventtimer[i], pc_eventtimer);
				sd->eventtimer[i] = -1;
				FREE(evname);
				break;
			}
		}

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void pc_addeventtimercount(struct map_session_data *sd, const char *name, int tick) {

	int i;

	nullpo_retv(sd);

	for(i = 0; i < MAX_EVENTTIMER; i++)
		if (sd->eventtimer[i] != -1 && strcmp((char *)(get_timer(sd->eventtimer[i])->data), name) == 0) {
			addtick_timer(sd->eventtimer[i], tick);
			break;
		}

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void pc_cleareventtimer(struct map_session_data *sd) {

	int i;
	char *evname;

	nullpo_retv(sd);

	for(i = 0; i < MAX_EVENTTIMER; i++)
		if (sd->eventtimer[i] != -1) {
			evname = (char *)(get_timer(sd->eventtimer[i])->data);
			delete_timer(sd->eventtimer[i], pc_eventtimer);
			sd->eventtimer[i] = -1;
			FREE(evname);
		}

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void pc_equipitem(struct map_session_data *sd, int n, int pos) {

	int i, nameid, arrow;
	struct item_data *id;

	nameid = sd->status.inventory[n].nameid;
	id = sd->inventory_data[n];
	pos = pc_equippoint(sd, n);
	if (battle_config.battle_log)
		printf("equip %d(%d) %x:%x\n", nameid, n, id->equip, pos);
	if (!pc_isequip(sd, n) || !pos || sd->status.inventory[n].attribute == 1) {
		clif_equipitemack(sd, n, 0, 0);
		return;
	}

	// Accessory equip coordination
	if (pos==0x88) { // Position is Accessory #1 or Accessory #2
		int epor=0;
		if (sd->equip_index[0] >= 0)
			epor |= sd->status.inventory[sd->equip_index[0]].equip;
		if (sd->equip_index[1] >= 0)
			epor |= sd->status.inventory[sd->equip_index[1]].equip;
		epor &= 0x88;
		pos = epor == 0x08 ? 0x80 : 0x08;
	}

	// Dual Weapon Wielding
	if ((pos == 0x22) // Position is left-hand/right-hand
	    && (id->equip == 2) // Right-hand weapon equip
	    && (pc_checkskill(sd, AS_LEFT) > 0 || 
	    pc_calc_base_job2(sd->status.class) == JOB_ASSASSIN))
	{
		int tpos = 0;
		if (sd->equip_index[8] >= 0)
			tpos |= sd->status.inventory[sd->equip_index[8]].equip;
		if (sd->equip_index[9] >= 0)
			tpos |= sd->status.inventory[sd->equip_index[9]].equip;
		tpos &= 0x02;
		pos = tpos == 0x02 ? 0x20 : 0x02;
	}

	arrow=pc_search_inventory(sd,pc_checkequip(sd,9));
	for(i=0;i<11;i++) {
		if (sd->equip_index[i] >= 0 && sd->status.inventory[sd->equip_index[i]].equip&pos) {
			pc_unequipitem(sd, sd->equip_index[i], 2);
		}
	}

	if (pos == 0x8000) { // Position is Arrow slot
		clif_arrowequip(sd, n);
		clif_arrow_fail(sd, 3);
	} else
		clif_equipitemack(sd, n, pos, 1);

	for(i=0;i<11;i++) {
		if (pos & equip_pos[i])
			sd->equip_index[i] = n;
	}
	sd->status.inventory[n].equip=pos;

	if (sd->status.inventory[n].equip & 0x0002) { // Left-Handed Weapon Slot
		if (sd->inventory_data[n])
			sd->weapontype1 = sd->inventory_data[n]->look;
		else
			sd->weapontype1 = 0; // Left-Hand Weapon Type is Barefist
		pc_calcweapontype(sd);
		clif_changelook(&sd->bl,LOOK_WEAPON,sd->status.weapon);
	}
	if (sd->status.inventory[n].equip & 0x0020) { // Shield/Right-Handed Weapon Slot
		if (sd->inventory_data[n]) {
			if (sd->inventory_data[n]->type == 4) { // Weapon Equip
				sd->status.shield = 0;
				if (sd->status.inventory[n].equip == 0x0020)
					sd->weapontype2 = sd->inventory_data[n]->look;
				else
					sd->weapontype2 = 0; // Right-Hand Weapon Type is Barefist
			}
			else if(sd->inventory_data[n]->type == 5) { // Non-Weapon Equip (Armor Equipment)
				sd->status.shield = sd->inventory_data[n]->look;
				sd->weapontype2 = 0;
			}
		}
		else
			sd->status.shield = sd->weapontype2 = 0;
		pc_calcweapontype(sd);
		clif_changelook(&sd->bl,LOOK_SHIELD,sd->status.shield);
	}
	if (sd->status.inventory[n].equip & 0x0001) { // Lower Headgear Slot
		if (sd->inventory_data[n])
			sd->status.head_bottom = sd->inventory_data[n]->look;
		else
			sd->status.head_bottom = 0;
		clif_changelook(&sd->bl,LOOK_HEAD_BOTTOM,sd->status.head_bottom);
	}
	if (sd->status.inventory[n].equip & 0x0100) { // Upper Headgear Slot
		if (sd->inventory_data[n])
			sd->status.head_top = sd->inventory_data[n]->look;
		else
			sd->status.head_top = 0;
		clif_changelook(&sd->bl,LOOK_HEAD_TOP,sd->status.head_top);
	}
	if (sd->status.inventory[n].equip & 0x0200) { // Mid Headgear Slot
		if (sd->inventory_data[n])
			sd->status.head_mid = sd->inventory_data[n]->look;
		else
			sd->status.head_mid = 0;
		clif_changelook(&sd->bl,LOOK_HEAD_MID,sd->status.head_mid);
	}
	if (sd->status.inventory[n].equip & 0x0040) // Footgear Slot
		clif_changelook(&sd->bl,LOOK_SHOES,0);

	pc_checkallowskill(sd);
	if ((arrow >= 0) && 
			(itemdb_look(sd->status.inventory[n].nameid) == 11 ||
			 itemdb_look(sd->status.inventory[n].nameid) == 17 || 
			 itemdb_look(sd->status.inventory[n].nameid) == 18 || 
			 itemdb_look(sd->status.inventory[n].nameid) == 19 || 
			 itemdb_look(sd->status.inventory[n].nameid) == 20 || 
			 itemdb_look(sd->status.inventory[n].nameid) == 21 || 
			 itemdb_look(sd->status.inventory[n].nameid) == 22)) {
		clif_arrowequip(sd,arrow);
		sd->status.inventory[arrow].equip=32768;
	}
	status_calc_pc(sd,0);

	if (sd->special_state.infinite_endure) {
		if (sd->sc_data[SC_ENDURE].timer == -1)
			status_change_start(&sd->bl,SC_ENDURE,10,1,0,0,0,0);
	} else {
		if (sd->sc_count && sd->sc_data[SC_ENDURE].timer != -1 && sd->sc_data[SC_ENDURE].val2)
			status_change_end(&sd->bl, SC_ENDURE, -1);
	}

	if (sd->sc_count) {
		if (sd->sc_data[SC_SIGNUMCRUCIS].timer != -1 && !battle_check_undead(7, sd->def_ele))
			status_change_end(&sd->bl, SC_SIGNUMCRUCIS, -1);
		if (sd->sc_data[SC_DANCING].timer != -1 && (sd->status.weapon != 13 && sd->status.weapon != 14))
			skill_stop_dancing(&sd->bl, 0);
	}

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void pc_unequipitem(struct map_session_data *sd, int n, int flag) {

	nullpo_retv(sd);

	if (flag < 2 && sd->sc_count && (sd->sc_data[SC_BLADESTOP].timer != -1 || sd->sc_data[SC_BERSERK].timer != -1)) {
		clif_unequipitemack(sd,n,0,0);
		return;
	}

	if (battle_config.battle_log)
		printf("unequip %d %x:%x\n",n,pc_equippoint(sd,n),sd->status.inventory[n].equip);
	if (sd->status.inventory[n].equip) {
		int i;
		for(i = 0; i < 11; i++) {
			if (sd->status.inventory[n].equip & equip_pos[i]) {
				sd->equip_index[i] = -1;
			}
		}
		if (sd->status.inventory[n].equip&0x0002) {
			sd->weapontype1 = 0;
			sd->status.weapon = sd->weapontype2;
			pc_calcweapontype(sd);
			clif_changelook(&sd->bl,LOOK_WEAPON,sd->status.weapon);
		}
		if (sd->status.inventory[n].equip&0x0020) {
			sd->status.shield = sd->weapontype2 = 0;
			pc_calcweapontype(sd);
			clif_changelook(&sd->bl,LOOK_SHIELD,sd->status.shield);
		}
		if (sd->status.inventory[n].equip&0x0001) {
			sd->status.head_bottom = 0;
			clif_changelook(&sd->bl,LOOK_HEAD_BOTTOM,sd->status.head_bottom);
		}
		if (sd->status.inventory[n].equip&0x0100) {
			sd->status.head_top = 0;
			clif_changelook(&sd->bl,LOOK_HEAD_TOP,sd->status.head_top);
		}
		if (sd->status.inventory[n].equip&0x0200) {
			sd->status.head_mid = 0;
			clif_changelook(&sd->bl,LOOK_HEAD_MID,sd->status.head_mid);
		}
		if (sd->status.inventory[n].equip&0x0040) {
			clif_changelook(&sd->bl,LOOK_SHOES,0);
		}
		clif_unequipitemack(sd, n, sd->status.inventory[n].equip, 1);
		sd->status.inventory[n].equip = 0;
		if (flag&1)
			pc_checkallowskill(sd);
		if (sd->weapontype1 == 0 && sd->weapontype2 == 0)
			skill_enchant_elemental_end(&sd->bl, -1);
	} else
		clif_unequipitemack(sd, n, 0, 0);

	if (sd->unequip_hpdamage > 0) {
		short dmg = sd->unequip_hpdamage;
		if (dmg > sd->status.hp)
			dmg = sd->status.hp;
		pc_heal(sd, -dmg, 0);
	}
	if (sd->unequip_spdamage > 0) {
		short dmg = sd->unequip_spdamage;
		if (dmg > sd->status.sp)
			dmg = sd->status.sp;
		pc_heal(sd, 0, -dmg);
	}

	if (flag&1) {
		status_calc_pc(sd, 0);
		if (sd->sc_count && sd->sc_data[SC_SIGNUMCRUCIS].timer != -1 && !battle_check_undead(7,sd->def_ele))
			status_change_end(&sd->bl,SC_SIGNUMCRUCIS,-1);
	}

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
int pc_checkitem(struct map_session_data *sd) {

	int i, j, k, id, calc_flag = 0;
	struct item_data *it = NULL;

	nullpo_retr(0, sd);

	if (sd->vender_id)
		return 0;

	for(i = j = 0; i < MAX_INVENTORY; i++) {
		if ((id = sd->status.inventory[i].nameid) == 0)
			continue;
		if (battle_config.item_check && !itemdb_available(id)) {
			if (battle_config.error_log)
				printf("illegal item id %d in %d[%s] inventory.\n", id, sd->bl.id, sd->status.name);
			pc_delitem(sd, i, sd->status.inventory[i].amount, 3);
			continue;
		}
		if (i > j) {
			memcpy(&sd->status.inventory[j], &sd->status.inventory[i], sizeof(struct item));
			sd->inventory_data[j] = sd->inventory_data[i];
		}
		j++;
	}
	if (j < MAX_INVENTORY)
		memset(&sd->status.inventory[j], 0, sizeof(struct item) * (MAX_INVENTORY - j));
	for(k = j; k < MAX_INVENTORY; k++)
		sd->inventory_data[k] = NULL;

	for(i = j = 0; i < MAX_CART; i++) {
		if ((id = sd->status.cart[i].nameid) == 0)
			continue;
		if (battle_config.item_check && !itemdb_available(id)) {
			if (battle_config.error_log)
				printf("illegal item id %d in %d[%s] cart.\n", id, sd->bl.id, sd->status.name);
			pc_cart_delitem(sd, i, sd->status.cart[i].amount, 1);
			continue;
		}
		if (i > j) {
			memcpy(&sd->status.cart[j], &sd->status.cart[i], sizeof(struct item));
		}
		j++;
	}
	if (j < MAX_CART)
		memset(&sd->status.cart[j], 0, sizeof(struct item) * (MAX_CART - j));

	if (battle_config.check_invalid_slot) {
		for(i = 0; i < MAX_INVENTORY; i++) {
			if (sd->status.inventory[i].nameid == 0)
				continue;
			if (sd->status.inventory[i].card[0] == 0x00ff)
				continue;
			if (sd->status.inventory[i].card[0] == (short)0xff00 || sd->status.inventory[i].card[0] == 0x00fe)
				continue;
			for (j = 0; j < 4; j++) {
				if (sd->status.inventory[i].card[j] != 0)
					if (itemdb_type(sd->status.inventory[i].card[j]) != 6) {
						if (battle_config.error_log)
							printf("Illegal sloted card id %d in %d[%s] inventory.\n", sd->status.inventory[i].card[j], sd->bl.id, sd->status.name);
						pc_delitem(sd, i, sd->status.inventory[i].amount, 3);
						break;
					}
			}
		}
	}

	for(i = 0; i < MAX_INVENTORY; i++) {

		it = sd->inventory_data[i];

		if (sd->status.inventory[i].nameid == 0)
			continue;
		if (sd->status.inventory[i].equip & ~pc_equippoint(sd, i)) {
			sd->status.inventory[i].equip = 0;
			calc_flag = 1;
		}
		if (sd->status.inventory[i].equip && map[sd->bl.m].flag.pvp && (it->flag.no_equip & 1)) { // no_equip = 1- not in PvP, 2- GvG restriction, 3- PvP and GvG which restriction
			sd->status.inventory[i].equip = 0;
			calc_flag = 1;
		} else if (sd->status.inventory[i].equip && map[sd->bl.m].flag.gvg && (it->flag.no_equip & 2)) { // no_equip = 1- not in PvP, 2- GvG restriction, 3- PvP and GvG which restriction
			sd->status.inventory[i].equip = 0;
			calc_flag = 1;
		}
	}

	pc_setequipindex(sd);
	if (calc_flag)
		status_calc_pc(sd,2);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int pc_checkoverhp(struct map_session_data *sd) {

	nullpo_retr(0, sd);

	if (sd->status.hp == sd->status.max_hp)
		return 1;
	if (sd->status.hp > sd->status.max_hp) {
		sd->status.hp = sd->status.max_hp;
		clif_updatestatus(sd, SP_HP);
		return 2;
	} else if(sd->status.hp < 0) {
		sd->status.hp = 1;
		clif_updatestatus(sd, SP_HP);
		return 2;
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int pc_checkoversp(struct map_session_data *sd) {

	nullpo_retr(0, sd);

	if (sd->status.sp == sd->status.max_sp)
		return 1;
	if (sd->status.sp > sd->status.max_sp) {
		sd->status.sp = sd->status.max_sp;
		clif_updatestatus(sd, SP_SP);
		return 2;
	} else if(sd->status.sp < 0) {
		sd->status.sp = 1;
		clif_updatestatus(sd, SP_SP);
		return 2;
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int pc_calc_pvprank_sub(struct block_list *bl,va_list ap) {

	struct map_session_data *sd1,*sd2=NULL;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);
	nullpo_retr(0, sd1 = (struct map_session_data *)bl);
	nullpo_retr(0, sd2 = va_arg(ap, struct map_session_data *));

	if (sd1->pvp_point > sd2->pvp_point)
		sd2->pvp_rank++;

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int pc_calc_pvprank(struct map_session_data *sd) {

	int old;
	struct map_data *m;

	nullpo_retr(0, sd);
	nullpo_retr(0, m = &map[sd->bl.m]);

	old = sd->pvp_rank;

	if (!(m->flag.pvp))
		return 0;
	sd->pvp_rank = 1;
	map_foreachinarea(pc_calc_pvprank_sub, sd->bl.m, 0, 0, m->xs, m->ys, BL_PC, sd);

	if (old != sd->pvp_rank || sd->pvp_lastusers != m->users)
		clif_pvpset(sd, sd->pvp_rank, sd->pvp_lastusers = m->users, 0);

	return sd->pvp_rank;
}

/*==========================================
 *
 *------------------------------------------
 */
int pc_calc_pvprank_timer(int tid, unsigned int tick, int id, int data) {

	struct map_session_data *sd = NULL;

	sd = map_id2sd(id);
	if (sd == NULL)
		return 0;

	sd->pvp_timer = -1;

	if (battle_config.pk_mode)
		return 0;

	if (pc_calc_pvprank(sd) > 0)
		sd->pvp_timer = add_timer(tick + PVP_CALCRANK_INTERVAL, pc_calc_pvprank_timer, id, data);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int pc_ismarried(struct map_session_data *sd) {

	if (sd == NULL)
		return -1;
	if (sd->status.partner_id > 0)
		return sd->status.partner_id;
	else
		return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int pc_marriage(struct map_session_data *sd, struct map_session_data *dstsd) {

	if (sd == NULL || dstsd == NULL || sd->status.partner_id > 0 || dstsd->status.partner_id > 0 || pc_calc_upper(sd->status.class) == 2)
		return -1;

	sd->status.partner_id = dstsd->status.char_id;
	dstsd->status.partner_id = sd->status.char_id;

	// Save the 2 players to synchronize them
	chrif_save(sd); // Do pc_makesavestatus and save storage + account_reg/account_reg2 too
	chrif_save(dstsd); // Do pc_makesavestatus and save storage + account_reg/account_reg2 too

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int pc_divorce(struct map_session_data *sd) {

	struct map_session_data *p_sd = NULL;

	if (sd == NULL || !pc_ismarried(sd))
		return -1;

	if ((p_sd = map_nick2sd(map_charid2nick(sd->status.partner_id))) != NULL) {
		int i;
		if (p_sd->status.partner_id != sd->status.char_id || sd->status.partner_id != p_sd->status.char_id) {
			printf("pc_divorce: Illegal partner_id sd=%d p_sd=%d\n", sd->status.partner_id, p_sd->status.partner_id);
			return -1;
		}
		sd->status.partner_id = 0;
		p_sd->status.partner_id = 0;
		for(i = 0; i < MAX_INVENTORY; i++)
			if (sd->status.inventory[i].nameid == WEDDING_RING_M || sd->status.inventory[i].nameid == WEDDING_RING_F)
				pc_delitem(sd, i, 1, 0);
		for(i = 0; i < MAX_INVENTORY; i++)
			if (p_sd->status.inventory[i].nameid == WEDDING_RING_M || p_sd->status.inventory[i].nameid == WEDDING_RING_F)
				pc_delitem(p_sd, i, 1, 0);
		// Save the 2 players to synchronize them
		chrif_save(sd); // Do pc_makesavestatus and save storage + account_reg/account_reg2 too
		chrif_save(p_sd); // Do pc_makesavestatus and save storage + account_reg/account_reg2 too
	} else {
		printf("pc_divorce: p_sd nullpo\n");
		return -1;
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
void pc_adoption(struct map_session_data *sd, struct map_session_data *tsd, char type) {

	struct map_session_data *p_sd;

	if (sd->status.base_level < 70 || 
		(p_sd = pc_get_partner(sd)) == NULL || p_sd->status.child > 0 || p_sd->status.base_level < 70)
		return; // Base level requirements

	if (tsd->status.father > 0 || tsd->status.mother > 0 || tsd->status.class > JOB_THIEF)
		return; // Child requirements, has no father/mother and is a novice/1st class 

	if (sd->status.party_id != p_sd->status.party_id || sd->status.party_id != tsd->status.party_id)
		return; // Same party requirement

	if (type == 0) {
		clif_adoption_invite(sd, tsd);
		return;
	}

	tsd->status.father = sd->status.char_id;
	tsd->status.mother = p_sd->status.char_id;
	sd->status.child = tsd->status.char_id;
	p_sd->status.child = tsd->status.char_id;

	if (pc_jobchange(tsd, tsd->status.class, 2) == 0) {
		// Baby skills
		pc_skill(tsd, WE_BABY, 1, 0);
		pc_skill(tsd, WE_CALLPARENT, 1, 0);
		// Parent skills
		pc_skill(sd, WE_CALLBABY, 1, 0);
		pc_skill(p_sd, WE_CALLBABY, 1, 0);
	}

	chrif_save(sd); // Do pc_makesavestatus and save storage + account_reg/account_reg2 too
	chrif_save(p_sd); // Do pc_makesavestatus and save storage + account_reg/account_reg2 too

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
struct map_session_data *pc_get_partner(struct map_session_data *sd) {

	if (sd && pc_ismarried(sd))
		return map_charid2sd(sd->status.partner_id);

	return NULL;
}

/*==========================================
 *
 *------------------------------------------
 */
struct map_session_data *pc_get_father(struct map_session_data *sd) {
	if (sd && sd->status.father > 0)
		return map_charid2sd(sd->status.father);

	return NULL;
}

/*==========================================
 *
 *------------------------------------------
 */
struct map_session_data *pc_get_mother(struct map_session_data *sd) {
	if (sd && sd->status.mother > 0)
		return map_charid2sd(sd->status.mother);

	return NULL;
}

/*==========================================
 *
 *------------------------------------------
 */
struct map_session_data *pc_get_child(struct map_session_data *sd) {
	if (sd && pc_ismarried(sd) && sd->status.child > 0)
		return map_charid2sd(sd->status.child);

	return NULL;
}

/*==========================================
 *
 *------------------------------------------
 */
static unsigned int natural_heal_tick, natural_heal_prev_tick;
static int natural_heal_diff_tick;
static int pc_spheal(struct map_session_data *sd) {
	int a, skill;
	struct guild_castle *gc = NULL;

	nullpo_retr(0, sd);

	a = natural_heal_diff_tick;
	if (pc_issit(sd))
		if (sd->state.previously_sit_sp)
			a += a;

	if (sd->sc_count) {
		if (sd->sc_data[SC_MAGNIFICAT].timer != -1)
			a += a;
		if (sd->sc_data[SC_REGENERATION].timer != -1)
			a *= sd->sc_data[SC_REGENERATION].val1;
	}
	if ((skill = pc_checkskill(sd, HP_MEDITATIO)) > 0)
		a += a * skill * 3 / 100;

	if (sd->status.guild_id > 0) {
		gc = guild_mapname2gc(sd->mapname);
		if (gc) {
			struct guild *g;
			g = guild_search(sd->status.guild_id);
			if (g && g->guild_id == gc->guild_id)
				a += a;
		}
	}

	return a;
}

/*==========================================
 *
 *------------------------------------------
 */
static int pc_hpheal(struct map_session_data *sd) {

	int a;
	struct guild_castle *gc;

	nullpo_retr(0, sd);

	a = natural_heal_diff_tick;
	if (pc_issit(sd))
		if (sd->state.previously_sit_hp)
			a += a;

	if (sd->sc_count) {
		if (sd->sc_data[SC_MAGNIFICAT].timer != -1)
			a += a;
		if (sd->sc_data[SC_REGENERATION].timer != -1)
			a *= sd->sc_data[SC_REGENERATION].val1;
	}

	if (sd->status.guild_id > 0) {
		gc = guild_mapname2gc(sd->mapname);
		if (gc) {
			struct guild *g;
			g = guild_search(sd->status.guild_id);
			if (g && g->guild_id == gc->guild_id)
				a += a;
		}
	}

	return a;
}

/*==========================================
 *
 *------------------------------------------
 */
static void pc_natural_heal_hp(struct map_session_data *sd) {

	int bhp;
	int inc_num, bonus, hp_flag;

	nullpo_retv(sd);

	if (sd->no_regen&1)
		return;

	if (pc_checkoverhp(sd)) {
		sd->hp_sub = sd->inchealhptick = 0;
		return;
	}

	bhp = sd->status.hp;
	hp_flag = (pc_checkskill(sd, SM_MOVINGRECOVERY) > 0 && sd->walktimer != -1);

	if (sd->walktimer == -1) {
		inc_num = pc_hpheal(sd);
		if (sd->sc_data[SC_TENSIONRELAX].timer != -1) {
			sd->hp_sub += 2 * inc_num;
			sd->inchealhptick += 3 * natural_heal_diff_tick;
		}else{
			sd->hp_sub += inc_num;
			sd->inchealhptick += natural_heal_diff_tick;
		}
	} else if (hp_flag) {
		inc_num = pc_hpheal(sd);
		sd->hp_sub += inc_num;
		sd->inchealhptick = 0;
	} else {
		sd->hp_sub = sd->inchealhptick = 0;
		return;
	}

	if (sd->hp_sub >= battle_config.natural_healhp_interval) {
		if (pc_issit(sd))
			sd->state.previously_sit_hp = 1;
		else
			sd->state.previously_sit_hp = 0;
		bonus = sd->nhealhp;
		if (hp_flag) {
			bonus >>= 2;
			if (bonus <= 0) bonus = 1;
		}
		while (sd->hp_sub >= battle_config.natural_healhp_interval) {
			sd->hp_sub -= battle_config.natural_healhp_interval;
			if (sd->status.hp + bonus <= sd->status.max_hp)
				sd->status.hp += bonus;
			else {
				sd->status.hp = sd->status.max_hp;
				sd->hp_sub = sd->inchealhptick = 0;
			}
		}
	}

	if (bhp != sd->status.hp) {
		clif_updatestatus(sd, SP_HP);
		if (battle_config.idle_disconnect != 0 && battle_config.idle_disconnect_disable_for_restore) {
			sd->lastpackettime = time(NULL); // For disconnection if player is inactive
		}
	}

	if (sd->nshealhp > 0) {
		if (sd->inchealhptick >= battle_config.natural_heal_skill_interval && sd->status.hp < sd->status.max_hp) {
			bonus = sd->nshealhp;
			while(sd->inchealhptick >= battle_config.natural_heal_skill_interval) {
				sd->inchealhptick -= battle_config.natural_heal_skill_interval;
				if (sd->status.hp + bonus <= sd->status.max_hp)
					sd->status.hp += bonus;
				else {
					bonus = sd->status.max_hp - sd->status.hp;
					sd->status.hp = sd->status.max_hp;
					sd->hp_sub = sd->inchealhptick = 0;
				}
				clif_heal(sd->fd, SP_HP, bonus);
			}
		}
	}
	else sd->inchealhptick = 0;

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
static void pc_natural_heal_sp(struct map_session_data *sd) {

	int bsp;
	int inc_num, bonus;

	nullpo_retv(sd);

	if (sd->no_regen&2)
		return;

	if (pc_checkoversp(sd)) {
		sd->sp_sub = sd->inchealsptick = 0;
		return;
	}

	bsp = sd->status.sp;

	inc_num = pc_spheal(sd);
	if (sd->sc_data[SC_EXPLOSIONSPIRITS].timer == -1)
		sd->sp_sub += inc_num;
	if (sd->walktimer == -1)
		sd->inchealsptick += natural_heal_diff_tick;
	else sd->inchealsptick = 0;

	if (sd->sp_sub >= battle_config.natural_healsp_interval) {
		if (pc_issit(sd))
			sd->state.previously_sit_sp = 1;
		else
			sd->state.previously_sit_sp = 0;
		bonus = sd->nhealsp;
		while(sd->sp_sub >= battle_config.natural_healsp_interval) {
			sd->sp_sub -= battle_config.natural_healsp_interval;
			if (sd->status.sp + bonus <= sd->status.max_sp)
				sd->status.sp += bonus;
			else {
				sd->status.sp = sd->status.max_sp;
				sd->sp_sub = sd->inchealsptick = 0;
			}
		}
	}

	if (bsp != sd->status.sp) {
		clif_updatestatus(sd, SP_SP);
		if (battle_config.idle_disconnect != 0 && battle_config.idle_disconnect_disable_for_restore) {
			sd->lastpackettime = time(NULL); // For disconnection if player is inactive
		}
	}

	if (sd->nshealsp > 0) {
		if (sd->inchealsptick >= battle_config.natural_heal_skill_interval && sd->status.sp < sd->status.max_sp) {

			if (sd->doridori_counter && pc_calc_base_job2(sd->status.class) == JOB_SUPER_NOVICE)
				bonus = sd->nshealsp * 2;
			else
				bonus = sd->nshealsp;
			
			sd->doridori_counter = 0;
			while(sd->inchealsptick >= battle_config.natural_heal_skill_interval) {
				sd->inchealsptick -= battle_config.natural_heal_skill_interval;
				if (sd->status.sp + bonus <= sd->status.max_sp)
					sd->status.sp += bonus;
				else {
					bonus = sd->status.max_sp - sd->status.sp;
					sd->status.sp = sd->status.max_sp;
					sd->sp_sub = sd->inchealsptick = 0;
				}
				clif_heal(sd->fd, SP_SP, bonus);
			}
		}
	}
	else
		sd->inchealsptick = 0;

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
static void pc_spirit_heal_hp(struct map_session_data *sd) {

	int bonus_hp, interval = battle_config.natural_heal_skill_interval;

	nullpo_retv(sd);

	if (pc_checkoverhp(sd)) {
		sd->inchealspirithptick = 0;
		return;
	}

	sd->inchealspirithptick += natural_heal_diff_tick;

	if (sd->weight * 100 / sd->max_weight >= battle_config.natural_heal_weight_rate)
		interval += interval;

	if (sd->inchealspirithptick >= interval) {
		bonus_hp = sd->nsshealhp;
		while(sd->inchealspirithptick >= interval) {
			if (pc_issit(sd) && sd->state.previously_sit_hp) {
				sd->inchealspirithptick -= interval;
				if (sd->status.hp < sd->status.max_hp) {
					if (sd->status.hp + bonus_hp <= sd->status.max_hp)
						sd->status.hp += bonus_hp;
					else {
						bonus_hp = sd->status.max_hp - sd->status.hp;
						sd->status.hp = sd->status.max_hp;
					}
					clif_heal(sd->fd, SP_HP, bonus_hp);
					sd->inchealspirithptick = 0;
				}
			} else {
				sd->inchealspirithptick -= natural_heal_diff_tick;
				break;
			}
		}
	}

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
static void pc_spirit_heal_sp(struct map_session_data *sd) {

	int bonus_sp, interval = battle_config.natural_heal_skill_interval;

	nullpo_retv(sd);

	if (pc_checkoversp(sd)) {
		sd->inchealspiritsptick = 0;
		return;
	}

	sd->inchealspiritsptick += natural_heal_diff_tick;

	if (sd->weight * 100 / sd->max_weight >= battle_config.natural_heal_weight_rate)
		interval += interval;

	if (sd->inchealspiritsptick >= interval) {
		bonus_sp = sd->nsshealsp;
		while(sd->inchealspiritsptick >= interval) {
			if (pc_issit(sd) && sd->state.previously_sit_sp) {
				sd->inchealspiritsptick -= interval;
				if (sd->status.sp < sd->status.max_sp) {
					if (sd->status.sp + bonus_sp <= sd->status.max_sp)
						sd->status.sp += bonus_sp;
					else {
						bonus_sp = sd->status.max_sp - sd->status.sp;
						sd->status.sp = sd->status.max_sp;
					}
					clif_heal(sd->fd, SP_SP, bonus_sp);
					sd->inchealspiritsptick = 0;
				}
			} else {
				sd->inchealspiritsptick -= natural_heal_diff_tick;
				break;
			}
		}
	}

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
static void pc_bleeding(struct map_session_data *sd) {

	int interval, hp;

	nullpo_retv(sd);

	interval = sd->hp_loss_rate;
	hp = sd->hp_loss_value;

	sd->hp_loss_tick += natural_heal_diff_tick;
	if (sd->hp_loss_tick >= interval) {
		while(sd->hp_loss_tick >= interval) {
			sd->hp_loss_tick -= interval;
			if (sd->status.hp < hp)
				hp = sd->status.hp;
			if (sd->hp_loss_type == 1)
				clif_damage(&sd->bl, &sd->bl, gettick_cache, 0, 0, hp, 0, 0, 0);
			pc_heal(sd, -hp, 0);
			sd->hp_loss_tick = 0;
		}
	}

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
static int pc_natural_heal_sub(struct map_session_data *sd, va_list ap) {

	int tick;

	tick = va_arg(ap, int);

	if (pc_isdead(sd) || pc_ishiding(sd) ||
		 (sd->sc_count &&
		((sd->sc_data[SC_POISON].timer != -1 && sd->sc_data[SC_SLOWPOISON].timer == -1) ||
		 (sd->sc_data[SC_DPOISON].timer != -1 && sd->sc_data[SC_SLOWPOISON].timer == -1) ||
		 (sd->sc_data[SC_GOSPEL].timer != -1 && sd->sc_data[SC_GOSPEL].val4 == BCT_SELF) ||
		  sd->sc_data[SC_BLEEDING].timer != -1 ||
		  sd->sc_data[SC_BERSERK].timer != -1 ||
		  sd->sc_data[SC_TRICKDEAD].timer != -1))) {
		sd->hp_sub = sd->inchealhptick = sd->inchealspirithptick = 0;
		sd->sp_sub = sd->inchealsptick = sd->inchealspiritsptick = 0;
	} else {
		if (DIFF_TICK (tick, sd->canregen_tick) < 0 ||
			sd->weight * 100 / sd->max_weight >= battle_config.natural_heal_weight_rate) {
			sd->hp_sub = sd->inchealhptick = 0;
			sd->sp_sub = sd->inchealsptick = 0;
		} else {
			pc_natural_heal_hp(sd);
			if (sd->sc_count && ((sd->sc_data[SC_EXTREMITYFIST].timer != -1 && sd->sc_data[SC_SPIRIT].timer == -1) || sd->sc_data[SC_DANCING].timer != -1))
					sd->sp_sub = sd->inchealsptick = 0;
			else
				pc_natural_heal_sp(sd);
			sd->canregen_tick = tick;
		}
		if (sd->nsshealhp)
			pc_spirit_heal_hp(sd);
		if (sd->nsshealsp)
			pc_spirit_heal_sp(sd);
	}

	if (sd->hp_loss_value > 0)
		pc_bleeding(sd);
	else
		sd->hp_loss_tick = 0;

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
static int pc_natural_heal(int tid, unsigned int tick, int id, int data) {

	natural_heal_tick = tick;
	natural_heal_diff_tick = DIFF_TICK(natural_heal_tick, natural_heal_prev_tick);
	clif_foreachclient(pc_natural_heal_sub, tick);

	natural_heal_prev_tick = tick;

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int pc_setsavepoint(struct map_session_data *sd,char *mapname,int x,int y) {

	nullpo_retr(0, sd);

	memset(sd->status.save_point.map, 0, sizeof(sd->status.save_point.map));
	strncpy(sd->status.save_point.map, mapname, 16); // 17 - NULL
	sd->status.save_point.x = x;
	sd->status.save_point.y = y;

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int pc_idle_timer(int tid, unsigned int tick, int id, int data) {

	struct map_session_data *pl_sd = NULL;
	int i, flag;
	time_t time_for_idle;

	// Do nothing if admin doesn't disconnect idle players
	if (battle_config.idle_disconnect == 0)
		return 0;

	// Init idle times to speed up
	time_for_idle = time(NULL);

	for(i = 0; i < fd_max; i++) {
		if (session[i] && (pl_sd = session[i]->session_data) && pl_sd->state.auth) {
			// Check GM
			if (pl_sd->GM_level >= battle_config.idle_disconnect_ignore_GM)
				continue;

			// Check for ok
			if (pl_sd->lastpackettime < time_for_idle - battle_config.idle_disconnect + 30) {
				if (pl_sd->lastpackettime < time_for_idle - battle_config.idle_disconnect)
					flag = 0; // Disconnection
				else
					flag = 1; // Send message
			// If it's okay
			} else
				continue; // Stay connected

			// Check for chat
			if (pl_sd->chatID) {
				if (battle_config.idle_disconnect_chat == 0 ||
				    pl_sd->lastpackettime >= time_for_idle - battle_config.idle_disconnect_chat + 30)
					continue; // Stay connected
				else if (pl_sd->lastpackettime >= time_for_idle - battle_config.idle_disconnect_chat)
					flag = 1; // Send message
			}

			// Check for shop
			if (pl_sd->vender_id) {
				if (battle_config.idle_disconnect_vender == 0 ||
				    pl_sd->lastpackettime >= time_for_idle - battle_config.idle_disconnect_vender + 30)
					continue; // Stay connected
				else if (pl_sd->lastpackettime >= time_for_idle - battle_config.idle_disconnect_vender)
					flag = 1; // Send message
			}

			// If send message
			if (flag == 1)
				clif_wis_message(pl_sd->fd, wisp_server_name, msg_txt(612), strlen(msg_txt(612)) + 1); // You are not active. In order to provide better service to other players, you will be disconnected in 30 seconds.
			// If disconnection
			else if (flag == 0) {
				clif_wis_message(pl_sd->fd, wisp_server_name, msg_txt(613), strlen(msg_txt(613)) + 1); // You were disconnected because of your inactivity.
				clif_authfail_fd(pl_sd->fd, 0); // Disconnected from server
				if (battle_config.etc_log) {
					if (pl_sd->GM_level)
							printf("Player [" CL_WHITE "%s" CL_RESET "(gm lvl:" CL_WHITE "%d" CL_RESET ")] is inactive -> disconnection.\n", pl_sd->status.name, pl_sd->GM_level);
					else
							printf("Player [" CL_WHITE "%s" CL_RESET "] is inactive -> disconnection.\n", pl_sd->status.name);
				}
			}
		}
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
static int Ghp[MAX_GUILDCASTLE][8];
void pc_guardiansave(void) {

	struct guild_castle *gc;
	int i;

	for(i = 0; i < MAX_GUILDCASTLE; i++) {
		gc = guild_castle_search(i);
		if (!gc) continue;
		if (gc->visibleG0 == 1 && Ghp[i][0] != gc->Ghp0) {
			guild_castledatasave(gc->castle_id, 18, gc->Ghp0);
			Ghp[i][0] = gc->Ghp0;
		}
		if (gc->visibleG1 == 1 && Ghp[i][1] != gc->Ghp1) {
			guild_castledatasave(gc->castle_id, 19, gc->Ghp1);
			Ghp[i][1] = gc->Ghp1;
		}
		if (gc->visibleG2 == 1 && Ghp[i][2] != gc->Ghp2) {
			guild_castledatasave(gc->castle_id, 20, gc->Ghp2);
			Ghp[i][2] = gc->Ghp2;
		}
		if (gc->visibleG3 == 1 && Ghp[i][3] != gc->Ghp3) {
			guild_castledatasave(gc->castle_id, 21, gc->Ghp3);
			Ghp[i][3] = gc->Ghp3;
		}
		if (gc->visibleG4 == 1 && Ghp[i][4] != gc->Ghp4) {
			guild_castledatasave(gc->castle_id, 22, gc->Ghp4);
			Ghp[i][4] = gc->Ghp4;
		}
		if (gc->visibleG5 == 1 && Ghp[i][5] != gc->Ghp5) {
			guild_castledatasave(gc->castle_id, 23, gc->Ghp5);
			Ghp[i][5] = gc->Ghp5;
		}
		if (gc->visibleG6 == 1 && Ghp[i][6] != gc->Ghp6) {
			guild_castledatasave(gc->castle_id, 24, gc->Ghp6);
			Ghp[i][6] = gc->Ghp6;
		}
		if (gc->visibleG7 == 1 && Ghp[i][7] != gc->Ghp7) {
			guild_castledatasave(gc->castle_id, 25, gc->Ghp7);
			Ghp[i][7] = gc->Ghp7;
		}
	}

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
static int last_save_fd = 0; // Init last saved to first connection (0)
static int guardian_save_count = 3; // To save guardians 3 times less of all players (reduce lags)
int pc_autosave(int tid, unsigned int tick, int id, int data) {

	struct map_session_data *sd;
	int i;
	int interval;

	for(i = last_save_fd + 1; i < fd_max; i++) { // + 1: to avoid console (0) or latest saved player
		if (session[i] && (sd = session[i]->session_data) && sd->state.auth)
			if (sd->last_saving + 10000 < gettick_cache) { // not save if previous saving was done recently // to limit successive savings with auto-save (you can not do a lot of thing in 10 seconds)
				// pet
				if (sd->status.pet_id > 0 && sd->pd)
					intif_save_petdata(sd->status.account_id, &sd->pet);
				chrif_save(sd); // do pc_makesavestatus and save storage + account_reg/account_reg2 too
				last_save_fd = i;
				break;
			}
	}
	// no player saved
	if (i >= fd_max) {
		last_save_fd = 0; // init last saved to first connection (0)
		guardian_save_count--;
		if (guardian_save_count == 0) { // to save guardians 3 times less of all players (reduce lags)
			pc_guardiansave(); // save guardians (if necessary)
			guardian_save_count = 3;
		}
	}

	// calculation for the next save of a character
	interval = autosave_interval / (clif_countusers() + 1);
	if (interval < 80)
		interval = 80; // minimum 80 msec -> max 750 players every min (otherwise, we can cause lag)
	else if (interval > 10000)
		interval = 10000; // maximum 10 sec
	if (agit_flag == 1) // in WoE, save less often to reduce lag
		interval = interval * 3 / 2;
	add_timer(gettick_cache + interval, pc_autosave, 0, 0);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int pc_read_gm_account(int fd) {

	int i, j, account_id;
	struct map_session_data *sd;

	FREE(gm_account);
	GM_num = 0;

	if (RFIFOW(fd,2) > 4) {
		/* init GM level of all players */
		for(j = 0; j < fd_max; j++)
			if (session[j] && (sd = session[j]->session_data) && sd->state.auth)
				sd->GM_level = 0;
		/* init DB */
		MALLOC(gm_account, struct gm_account, (RFIFOW(fd,2) - 4) / 5);
		for (i = 4; i < RFIFOW(fd,2); i = i + 5) {
			account_id = RFIFOL(fd,i); /* speed up */
			gm_account[GM_num].account_id = account_id;
			gm_account[GM_num].level = RFIFOB(fd,i + 4);
			// Set gm_level in online players
			for(j = 0; j < fd_max; j++)
				if (session[j] && (sd = session[j]->session_data) && sd->state.auth)
					if (account_id == sd->status.account_id)
						sd->GM_level = gm_account[GM_num].level;
			GM_num++;
		}
	}

	return GM_num;
}

/*==========================================
 *
 *------------------------------------------
 */
int map_day_timer(int tid, unsigned int tick, int id, int data) {

	struct map_session_data *pl_sd = NULL;
	int i;
	int msg_len;

	if (battle_config.day_duration > 0) { // if we want a day
		if (night_flag != 0) {
			msg_len = strlen(msg_txt(502)) + 1;
			night_flag = 0; // 0=day, 1=night
			for(i = 0; i < fd_max; i++) {
				if (session[i] && (pl_sd = session[i]->session_data) && pl_sd->state.auth && !map[pl_sd->bl.m].flag.indoors) {
					clif_status_change(&pl_sd->bl, ICO_NIGHT, 0);
					clif_wis_message(pl_sd->fd, wisp_server_name, msg_txt(502), msg_len); // The day has arrived!
				}
			}
		}
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int map_night_timer(int tid, unsigned int tick, int id, int data) {

	struct map_session_data *pl_sd = NULL;
	int i;
	int msg_len;

	if (battle_config.night_duration > 0) { // if we want a night
		if (night_flag == 0) {
			msg_len = strlen(msg_txt(503)) + 1;
			night_flag = 1; // 0=day, 1=night
			for(i = 0; i < fd_max; i++) {
				if (session[i] && (pl_sd = session[i]->session_data) && pl_sd->state.auth && !map[pl_sd->bl.m].flag.indoors) {
					clif_status_change(&pl_sd->bl, ICO_NIGHT, 1);
					clif_wis_message(pl_sd->fd, wisp_server_name, msg_txt(503), msg_len); // The night has fallen...
				}
			}
		}
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
void pc_setstand(struct map_session_data *sd) {

	nullpo_retv(sd);

	if (sd->sc_count && sd->sc_data[SC_TENSIONRELAX].timer != -1)
		status_change_end(&sd->bl, SC_TENSIONRELAX, -1);

	sd->state.dead_sit = 0; // 0: standup, 1: dead, 2: sit
	sd->state.previously_sit_hp = 0; // 0: not sit when is was previously HP regen, 1: sit... (to avoid people that sit down and stand up between 2 timers)
	sd->state.previously_sit_sp = 0; // 0: not sit when is was previously SP regen, 1: sit... (to avoid people that sit down and stand up between 2 timers)
}

/*==========================================
 *
 *------------------------------------------
 */
void pc_readdb(void) {

	int i, j, k, u;
	struct pc_base_job s_class;
	FILE *fp;
	char line[1024],*p;

	memset(&exp_table, 0, sizeof(exp_table));
	fp = fopen("db/exp.txt", "r");
	if (fp == NULL) {
		printf(CL_RED "Error:" CL_RESET " Failed to load db/exp.txt.\n");
		return;
	}

	i = 0;
	while(fgets(line, sizeof(line), fp)) { // fgets reads until maximum one less than size and add '\0' -> so, it's not necessary to add -1
		unsigned int bn,b1,b2,b3,b4,b5,b6,jn,j1,j2,j3,j4,j5,j6,j7,j8;
		if ((line[0] == '/' && line[1] == '/') || line[0] == '\0' || line[0] == '\n' || line[0] == '\r')
			continue;
		// it's not necessary to remove 'carriage return ('\n' or '\r')
		if (sscanf(line,"%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u",&bn,&b1,&b2,&b3,&b4,&b5,&b6,&jn,&j1,&j2,&j3,&j4,&j5,&j6,&j7,&j8) != 16)
			continue;
		exp_table[ 0][i] = bn;
		exp_table[ 1][i] = b1;
		exp_table[ 2][i] = b2;
		exp_table[ 3][i] = b3;
		exp_table[ 4][i] = b4;
		exp_table[ 5][i] = b5;
		exp_table[ 6][i] = b6;
		exp_table[ 7][i] = jn;
		exp_table[ 8][i] = j1;
		exp_table[ 9][i] = j2;
		exp_table[10][i] = j3;
		exp_table[11][i] = j4;
		exp_table[12][i] = j5;
		exp_table[13][i] = j6;
		exp_table[14][i] = j7;
		exp_table[15][i] = j8;
		i++;
		if (i >= battle_config.maximum_level)
			break;
	}
	fclose(fp);
	printf(CL_GREEN "Loaded: " CL_RESET "'" CL_WHITE "db/exp.txt" CL_RESET "' read ('" CL_WHITE "%d" CL_RESET "' entrie%s).\n", i, (i > 1) ? "s" : "");

	memset(&skill_tree, 0, sizeof(skill_tree));
	fp = fopen("db/skill_tree.txt", "r");
	if (fp == NULL) {
		printf(CL_RED "Error:" CL_RESET " Failed to load db/skill_tree.txt.\n");
		return;
	}
	while(fgets(line, sizeof(line), fp)) { // fgets reads until maximum one less than size and add '\0' -> so, it's not necessary to add -1
		char *split[50];
		int f = 0;
		if ((line[0] == '/' && line[1] == '/') || line[0] == '\0' || line[0] == '\n' || line[0] == '\r')
			continue;
		// it's not necessary to remove 'carriage return ('\n' or '\r')
		for(j = 0, p = line; j < 14 && p; j++) {
			split[j] = p;
			p = strchr(p,',');
			if (p) *p++ = 0;
		}
		if (j < 13)
			continue;
		if (j == 14) {
			f = 1; // MinJobLvl has been added
		}
		s_class = pc_calc_base_job(atoi(split[0]));
		i = s_class.job;
		u = s_class.upper;

		// Check for bounds
		if (i >= MAX_SKILL_TREE || u >= 3)
			continue;
		for(j = 0; j < MAX_SKILL_PER_TREE && skill_tree[u][i][j].id; j++)
			;
		if (j == MAX_SKILL_PER_TREE)
			continue;
		skill_tree[u][i][j].id = atoi(split[1]);
		skill_tree[u][i][j].max = atoi(split[2]);
		if (f)
			skill_tree[u][i][j].joblv = atoi(split[3]);

		for(k = 0; k < 5; k++) {
			skill_tree[u][i][j].need[k].id = atoi(split[k*2+3+f]);
			skill_tree[u][i][j].need[k].lv = atoi(split[k*2+4+f]);
		}
	}
	fclose(fp);
	printf(CL_GREEN "Loaded: " CL_RESET "'" CL_WHITE "db/skill_tree.txt" CL_RESET "' read.\n");

	for(i=0;i<4;i++)
		for(j=0;j<10;j++)
			for(k=0;k<10;k++)
				attr_fix_table[i][j][k]=100;
	fp = fopen("db/attr_fix.txt", "r");
	if (fp == NULL) {
		printf(CL_RED "Error:" CL_RESET " Failed to load db/attr_fix.txt.\n");
		return;
	}
	while(fgets(line, sizeof(line), fp)) { // fgets reads until maximum one less than size and add '\0' -> so, it's not necessary to add -1
		char *split[10];
		int lv, n;
		if ((line[0] == '/' && line[1] == '/') || line[0] == '\0' || line[0] == '\n' || line[0] == '\r')
			continue;
		// it's not necessary to remove 'carriage return ('\n' or '\r')
		for(j=0,p=line;j<3 && p;j++) {
			split[j]=p;
			p=strchr(p,',');
			if (p) *p++=0;
		}
		lv = atoi(split[0]);
		n = atoi(split[1]);
//		printf("%d %d\n", lv, n);

		for(i = 0; i < n; ) {
			if (fgets(line, sizeof(line), fp) == NULL) // fgets reads until maximum one less than size and add '\0' -> so, it's not necessary to add -1
				break;
			if ((line[0] == '/' && line[1] == '/') || line[0] == '\0' || line[0] == '\n' || line[0] == '\r')
				continue;
			// it's not necessary to remove 'carriage return ('\n' or '\r')

			for(j = 0, p = line; j < n && p; j++) {
				while(*p == 32 && *p > 0)
					p++;
				attr_fix_table[lv-1][i][j] = atoi(p);
				if (battle_config.attr_recover == 0 && attr_fix_table[lv-1][i][j] < 0)
					attr_fix_table[lv-1][i][j] = 0;
				p = strchr(p,',');
				if (p) *p++ = 0;
			}

			i++;
		}
	}
	fclose(fp);
	printf(CL_GREEN "Loaded: " CL_RESET "'" CL_WHITE "db/attr_fix.txt" CL_RESET "' read.\n");

	// stat point
	memset(&statp, 0, sizeof(statp));
	if ((fp = fopen("db/statpoint.txt", "r")) == NULL) {
		printf(CL_RED "Error:" CL_RESET " Failed to load db/statpoint.txt... Generating basic DB.\n");
		// generate the remaining parts of the db if necessary
		j = 45; // base points
		for(i = 0; i < MAX_LEVEL; i++) {
			j += (i + 15) / 5;
			statp[i] = j;
		}
		printf(CL_GREEN "Loaded: " CL_RESET "'" CL_WHITE "db/statpoint.txt" CL_RESET "' generated (levels='" CL_WHITE "%d/%d" CL_RESET "').\n", i, MAX_LEVEL);
	} else {
		i = 0;
		while(fgets(line, sizeof(line), fp)) { // fgets reads until maximum one less than size and add '\0' -> so, it's not necessary to add -1
			if ((line[0] == '/' && line[1] == '/') || line[0] == '\0' || line[0] == '\n' || line[0] == '\r')
				continue;
			// it's not necessary to remove 'carriage return ('\n' or '\r')
			if (i >= MAX_LEVEL)
				break;
			if ((j = atoi(line)) < 0)
				j = 0;
			statp[i] = j;
			i++;
		}
		fclose(fp);
		printf(CL_GREEN "Loaded: " CL_RESET "'" CL_WHITE "db/statpoint.txt" CL_RESET "' read (levels='" CL_WHITE "%d/%d" CL_RESET "').\n", i, MAX_LEVEL);
	}

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
static char extra_file_txt[1024] = "save/map_extra.txt";
static struct extra {
	int item_id; // -1: zeny, other: id item
	long quantity; // quantity of items or zeny
	char name[24]; // player name
} *extra_dat = NULL;
static int extra_num = 0;

/*==========================================
 *
 *------------------------------------------
 */
int pc_extra(int tid, unsigned int tick, int id, int data) {

	FILE *fp;
	static int extra_file_readed = 0;
	int change_flag = 0; // must we rewrite extra file? (0: no, 1: yes)
	int lock, i;
	char line[1024], name[1024];
	int item_id;
	long quantity;
	struct map_session_data *pl_sd;
	struct item_data *item_data;
	char output[MAX_MSG_LEN + 512];

	// do we use extra system?
	if (!battle_config.extra_system_flag)
		return 0;

	// if extra file not read, read it at first
	if (extra_file_readed == 0) {
		extra_file_readed = 1; // file readed.
		if ((fp = fopen(extra_file_txt, "r")) != NULL) {
			while(fgets(line, sizeof(line) - 1, fp) != NULL) {
				if ((line[0] == '/' && line[1] == '/') || line[0] == '\0' || line[0] == '\n' || line[0] == '\r')
					continue;
				// if line is valid
				if ((sscanf(line, "%d,%ld,%[^\r\n]", &item_id, &quantity, name) == 3 ||
				    sscanf(line, "%d\t%ld\t%[^\r\n]", &item_id, &quantity, name) == 3) &&
				    (item_id == -1 || (itemdb_exists(item_id) && (!battle_config.item_check || itemdb_available(item_id)))) && // zeny or valid items
				    quantity != 0 && // quantity
				    strlen(name) >= 4 && strlen(name) < 24) { // name
					// manage max quantity
					if (item_id == -1) {
						if (quantity > (long)MAX_ZENY) {
							quantity = (long)MAX_ZENY;
							// Invalid quantity -> file must be rewriten
							change_flag = 1;
						} else if (quantity < -((long)MAX_ZENY)) {
							quantity = -((long)MAX_ZENY);
							// Invalid quantity -> file must be rewriten
							change_flag = 1;
						}
					} else {
						if (quantity > (long)MAX_AMOUNT) {
							quantity = (long)MAX_AMOUNT;
							// Invalid quantity -> file must be rewriten
							change_flag = 1;
						} else if (quantity < -((long)MAX_AMOUNT)) {
							quantity = -((long)MAX_AMOUNT);
							// Invalid quantity -> file must be rewriten
							change_flag = 1;
						}
					}
					// add an index
					if (extra_num == 0) {
						CALLOC(extra_dat, struct extra, 1);
					} else {
						REALLOC(extra_dat, struct extra, extra_num + 1);
						memset(&extra_dat[extra_num], 0, sizeof(struct extra));
					}
					extra_dat[extra_num].item_id = item_id;
					extra_dat[extra_num].quantity = quantity;
					strncpy(extra_dat[extra_num].name, name, 24);
					extra_num++;
				} else { // Invalid line -> file must be rewriten
					change_flag = 1;
				}
			}
			fclose(fp);
		} else {
			// file doesn't exist. Create it for explanation -> file must be rewriten
			change_flag = 1;
		}
	}

	// if extra_add_file can be read and exists
	if ((fp = fopen(extra_add_file_txt, "r")) != NULL) {
		while(fgets(line, sizeof(line) - 1, fp) != NULL) {
			if ((line[0] == '/' && line[1] == '/') || line[0] == '\0' || line[0] == '\n' || line[0] == '\r')
				continue;
			// if line is valid
			if ((sscanf(line, "%d,%ld,%[^\r\n]", &item_id, &quantity, name) == 3 ||
			    sscanf(line, "%d\t%ld\t%[^\r\n]", &item_id, &quantity, name) == 3) &&
			    (item_id == -1 || (itemdb_exists(item_id) && (!battle_config.item_check || itemdb_available(item_id)))) && // zeny or valid items
			    quantity != 0 && // quantity
			    strlen(name) >= 4 && strlen(name) < 24) { // name
				// manage max quantity
				if (item_id == -1) {
					if (quantity > (long)MAX_ZENY)
						quantity = (long)MAX_ZENY;
					else if (quantity < -((long)MAX_ZENY))
						quantity = -((long)MAX_ZENY);
				} else {
					if (quantity > (long)MAX_AMOUNT)
						quantity = (long)MAX_AMOUNT;
					else if (quantity < -((long)MAX_AMOUNT))
						quantity = -((long)MAX_AMOUNT);
				}
				// add an index
				if (extra_num == 0) {
					CALLOC(extra_dat, struct extra, 1);
				} else {
					REALLOC(extra_dat, struct extra, extra_num + 1);
					memset(&extra_dat[extra_num], 0, sizeof(struct extra));
				}
				extra_dat[extra_num].item_id = item_id;
				extra_dat[extra_num].quantity = quantity;
				strncpy(extra_dat[extra_num].name, name, 24);
				extra_num++;
				// new line -> files must be rewriten
				change_flag = 1;
			}
		}
		fclose(fp);
		// erase file
		remove(extra_add_file_txt);
	}

	// check players to give zenys or items
	for (i = 0; i < extra_num; i++) {
		if ((pl_sd = map_nick2sd(extra_dat[i].name)) != NULL) {
			item_id = extra_dat[i].item_id;
			quantity = extra_dat[i].quantity;

			// work on zenys
			if (item_id == -1) {
				// substract
				if (quantity < 0) {
					if ((long)pl_sd->status.zeny < -quantity)
						quantity = -((long)pl_sd->status.zeny);
					if (quantity < 0) {
						char output[200];
						sprintf(output, msg_txt(686), -quantity); // Server (special action): you lost %ld zenys.
						clif_displaymessage(pl_sd->fd, output);
						pl_sd->status.zeny += quantity;
						clif_updatestatus(pl_sd, SP_ZENY);
						// line changed -> file must be rewriten
						extra_dat[i].quantity -= quantity;
						change_flag = 1;
					}
				// add
				} else {
					if (quantity > (long)(MAX_ZENY - pl_sd->status.zeny))
						quantity = (long)(MAX_ZENY - pl_sd->status.zeny);
					if (quantity > 0) {
						sprintf(output, msg_txt(687), quantity); // Server (special action): you gain %ld zenys.
						clif_displaymessage(pl_sd->fd, output);
						pl_sd->status.zeny += quantity;
						clif_updatestatus(pl_sd, SP_ZENY);
						// line changed -> file must be rewriten
						extra_dat[i].quantity -= quantity;
						change_flag = 1;
					}
				}

			// items -> check against item_id, because database can be reloaded.
			} else if ((item_data = itemdb_exists(item_id)) && (!battle_config.item_check || itemdb_available(item_id))) {
				int j;
				if (quantity < 0) { // remove items
					for (j = 0; j < MAX_INVENTORY; j++) {
						if (pl_sd->status.inventory[j].nameid == item_id && pl_sd->status.inventory[j].equip == 0) {
							if ((long)pl_sd->status.inventory[j].amount < -quantity)
								quantity = -((long)pl_sd->status.inventory[j].amount);
							if (quantity < 0) {
								pc_delitem(pl_sd, j, -quantity, 0);
								sprintf(output, msg_txt(688), -quantity, item_data->jname); // Server (special action): you lost %ld %s.
								clif_displaymessage(pl_sd->fd, output);
								// line changed -> file must be rewriten
								extra_dat[i].quantity -= quantity;
								quantity = extra_dat[i].quantity;
								change_flag = 1;
							}
						}
					}
				} else {
					int loop;
					struct item item_tmp;
					if ((long)item_data->weight * quantity > (long)(pl_sd->max_weight - pl_sd->weight)) {
						quantity = (long)((pl_sd->max_weight - pl_sd->weight) / item_data->weight);
						if (quantity <= 0)
							continue;
					}
					loop = 1;
					if (item_data->type == 4 || item_data->type == 5 ||
					    item_data->type == 7 || item_data->type == 8) {
						loop = (int)quantity;
						quantity = 1;
					}
					for(j = 0; j < loop; j++) {
						memset(&item_tmp, 0, sizeof(item_tmp));
						item_tmp.nameid = item_id;
						item_tmp.identify = 1;
						if (pc_additem(pl_sd, &item_tmp, quantity) == 0) {
							sprintf(output, msg_txt(689), quantity, item_data->jname);
							clif_displaymessage(pl_sd->fd, output);
							extra_dat[i].quantity -= quantity;
							change_flag = 1;
						}
					}
				}

			} else {
				extra_dat[i].quantity = 0;
				change_flag = 1;
			}
		}
	}

	if (change_flag) {
		for (i = 0; i < extra_num; i++) {
			if (extra_dat[i].quantity == 0) {
				if (extra_num == 1) {
					FREE(extra_dat);
					extra_num--;
				} else {
					if (i != (extra_num - 1))
						memcpy(&extra_dat[i], &extra_dat[extra_num - 1], sizeof(struct extra));
					extra_num--;
					REALLOC(extra_dat, struct extra, extra_num);
				}
				i--;
			}
		}
		if ((fp = lock_fopen(extra_file_txt, &lock)) != NULL) {
			fprintf(fp, "// DON'T change or modify this file?" RETCODE
			            "// The map-server manage it automatically." RETCODE
			            "// To add a line in this file, use the file '%s':" RETCODE
			            "//" RETCODE, extra_add_file_txt);
			fprintf(fp, "// An external software can add changes of zenys or items of players using file '%s'." RETCODE
			            "// You can change location and/or name of this file with appropriate option in 'conf/map_freya.conf'." RETCODE
			            "// To add a line in this file, prefer the append method." RETCODE
			            "// The map-server will read this file periodically and automatically destroy it." RETCODE
			            "// IMPORTANT: Be sure that the map-server can READ and DESTROY this file." RETCODE
			            "// If concerned player is online, line will be executed. Otherwise line will be added in '%s'." RETCODE
			            "// A line: <item_id>,<quantity>,<player name>" RETCODE
			            "// - Item_id can be -1 for zenys, or id of the item in the database" RETCODE
			            "// - Quantity can be negativ to substact" RETCODE
			            "// Example of lines (first: substract 1000 zenys, second: add 3 apples):" RETCODE
			            "// -1,-1000,Test_name" RETCODE
			            "// 512,2,Test_name2" RETCODE
			            "//" RETCODE, extra_add_file_txt, extra_file_txt);
			for (i = 0; i < extra_num; i++)
				fprintf(fp,"%d,%ld,%s" RETCODE, extra_dat[i].item_id, extra_dat[i].quantity, extra_dat[i].name);
			lock_fclose(fp, extra_file_txt, &lock);
		}
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int do_final_pc(void) {

	if (extra_num > 0) {
		FREE(extra_dat);
		extra_num = 0;
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int do_init_pc(void) {

	pc_readdb();

	add_timer_func_list(pc_walk, "pc_walk");
	add_timer_func_list(pc_attack_timer, "pc_attack_timer");
	add_timer_func_list(pc_natural_heal, "pc_natural_heal");
	add_timer_func_list(pc_invincible_timer, "pc_invincible_timer");
	add_timer_func_list(pc_jail_timer, "pc_jail_timer");
	add_timer_func_list(pc_eventtimer, "pc_eventtimer");
	add_timer_func_list(pc_calc_pvprank_timer, "pc_calc_pvprank_timer");
	add_timer_func_list(pc_autosave, "pc_autosave");
	add_timer_func_list(pc_idle_timer, "pc_idle_timer");
	add_timer_func_list(pc_spiritball_timer, "pc_spiritball_timer");
	add_timer_func_list(pc_follow_timer, "pc_follow_timer");
	add_timer_func_list(pc_extra, "pc_extra");

	add_timer_interval((natural_heal_prev_tick = gettick_cache + NATURAL_HEAL_INTERVAL), pc_natural_heal, 0, 0, NATURAL_HEAL_INTERVAL);
	add_timer(gettick_cache + autosave_interval, pc_autosave, 0, 0);
	add_timer_interval(gettick_cache + 30000, pc_idle_timer, 0, 0, 30000);
	add_timer_interval(gettick_cache + 10000, pc_extra, 0, 0, 60000);

	// add night/day timer
	add_timer_func_list(map_day_timer, "map_day_timer");
	add_timer_func_list(map_night_timer, "map_night_timer");
  {
	int day_duration = battle_config.day_duration;
	int night_duration = battle_config.night_duration;
	if (day_duration < 60000)
		day_duration = 60000;
	if (night_duration < 60000)
		night_duration = 60000;
	if (battle_config.night_at_start == 0) {
		night_flag = 0;
		day_timer_tid = add_timer_interval(gettick_cache + day_duration + night_duration, map_day_timer, 0, 0, day_duration + night_duration);
		night_timer_tid = add_timer_interval(gettick_cache + day_duration, map_night_timer, 0, 0, day_duration + night_duration);
	} else {
		night_flag = 1;
		day_timer_tid = add_timer_interval(gettick_cache + night_duration, map_day_timer, 0, 0, day_duration + night_duration);
		night_timer_tid = add_timer_interval(gettick_cache + day_duration + night_duration, map_night_timer, 0, 0, day_duration + night_duration);
	}
  }

	return 0;
}
