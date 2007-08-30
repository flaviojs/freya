// Copyright (c) Freya Development Team - Licensed under GNU GPL
// For more information, see License.txt in the main folder

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>

#include "../common/timer.h"
#include "../common/socket.h"
#include "../common/db.h"
#include "../common/malloc.h"
#include "../common/utils.h"
#include "nullpo.h"
#include "map.h"
#include "clif.h"
#include "intif.h"
#include "pc.h"
#include "mob.h"
#include "guild.h"
#include "itemdb.h"
#include "skill.h"
#include "battle.h"
#include "party.h"
#include "npc.h"
#include "status.h"
#include "atcommand.h"
#include "ranking.h"
#include "unit.h"

#ifdef MEMWATCH
#include "memwatch.h"
#endif

#define MIN_MOBTHINKTIME 100

#define MOB_LAZYMOVEPERC 50 // Move probability in the negligent mode MOB (rate of 1000 minute)
#define MOB_LAZYWARPPERC 20 // Warp probability in the negligent mode MOB (rate of 1000 minute)

struct mob_db mob_db[MAX_MOB_DB];

/*==========================================
 *
 *------------------------------------------
 */
static int distance(int, int, int, int);
void mob_makedummymobdb(int);
static int mob_timer(int, unsigned int, int, int);
int mobskill_use(struct mob_data *md, unsigned int tick, int event);
int mob_skillid2skillidx(int class, int skillid);
int mobskill_use_id(struct mob_data *md, struct block_list *target, int skill_idx);
static int mob_unlocktarget(struct mob_data *md, unsigned int tick);

/*==========================================
 *
 *------------------------------------------
 */
int mobdb_searchname(const char *str) {

	int i;

	for(i = 0; i < sizeof(mob_db) / sizeof(mob_db[0]); i++) {
		if (strcasecmp(mob_db[i].name, str) == 0 || strcmp(mob_db[i].jname, str) == 0 ||
		    memcmp(mob_db[i].name, str, 24) == 0 || memcmp(mob_db[i].jname, str, 24) == 0)
			return i;
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int mobdb_checkid(const int id) {

	if (id <= 0 || id >= (sizeof(mob_db) / sizeof(mob_db[0])) || mob_db[id].name[0] == '\0')
		return 0;

	return id;
}

/*==========================================
 *
 *------------------------------------------
 */
void mob_spawn_dataset(struct mob_data *md, const char *mobname, int class) {

	if (strcmp(mobname, "--en--") == 0)
		strncpy(md->name, mob_db[class].name, 24);
	else if (strcmp(mobname, "--ja--") == 0)
		strncpy(md->name, mob_db[class].jname, 24);
	else
		strncpy(md->name, mobname, 24);

	md->base_class = class;
	md->class = class;
	md->bl.id = npc_get_new_npc_id();
	md->timer = -1;
	md->speed = mob_db[class].speed;

	return;
}


/*==========================================
 * The mob appearance for one time (For scripts)
 *------------------------------------------
 */
int mob_once_spawn(struct map_session_data *sd, char *mapname,
	int x, int y, const char *mobname, int class, int amount, const char *event) {

	struct mob_data *md = NULL;
	int m, count, real_class;

	if (sd && strcmp(mapname, "this") == 0)
		m = sd->bl.m;
	else
		m = map_mapname2mapid(mapname);

	if (m < 0 || amount <= 0 || (class >= 0 && class <= 1000) || class >= (MAX_MOB_DB * 3))
		return 0;

	real_class = class;
	if (class < 0) {
		int j = -class-1;
		if (j >= 0 && j < MAX_RANDOMMONSTER) {
			int i = 0, lv = 255;
			if (sd)
				lv = sd->status.base_level;
			do {
				real_class = rand() % 1000 + 1001;
			} while((mob_db[real_class].max_hp <= 0 || mob_db[real_class].summonper[j] <= (rand() % 1000000) ||
			        (lv < mob_db[real_class].lv && battle_config.random_monster_checklv) ||
			        (real_class == 1288 && j == 0)) && (i++) < 2000); // No Emperium (1288) on dead_branch (0)
			if (i >= 2000) {
				real_class = mob_db[0].summonper[j];
			}
		} else {
			return 0;
		}
	} else if (class >= (MAX_MOB_DB * 2)) { // Large/Tiny mobs
		real_class -= (MAX_MOB_DB * 2); // Big
	} else if (class >= MAX_MOB_DB) {
		real_class -= MAX_MOB_DB; // Small
	}

	if (real_class >= 0 && real_class <= 1000)
		return 0;

	if (sd) {
		if (x <= 0 || y <= 0) {
			if (x <= 0)
				x = sd->bl.x + rand() % 3 - 1;
			if (y <= 0)
				y = sd->bl.y + rand() % 3 - 1;
			if (map_getcell(m, x, y, CELL_CHKNOPASS_NPC)) {
				x = sd->bl.x;
				y = sd->bl.y;
			}
		}
	} else if (x <= 0 || y <= 0) {
		int j;
		j = 0;
		do {
			x = rand() % (map[m].xs - 2) + 1;
			y = rand() % (map[m].ys - 2) + 1;
			j++;
		} while (map_getcell(m, x, y, CELL_CHKNOPASS_NPC) && j < 256);
		if (j >= 256) {
			x = 0;
			y = 0;
		}
	}

	for(count = 0; count < amount; count++) {
		CALLOC(md, struct mob_data, 1);

		if (class >= (MAX_MOB_DB * 2)) // Large/Tiny mobs
			md->size = 2; // Big
		else if (class >= MAX_MOB_DB)
			md->size = 1; // Small

		if (mob_db[real_class].mode & 0x02) {
			CALLOC(md->lootitem, struct item, LOOTITEM_SIZE);
		} else
			md->lootitem = NULL;

		mob_spawn_dataset(md, mobname, real_class);
		md->bl.m = m;
		md->bl.x = x;
		md->bl.y = y;
		if (class < 0 && battle_config.dead_branch_active)
			md->mode = 0x1 + 0x4 + 0x80;
		md->m = m;
		md->x0 = x;
		md->y0 = y;
		md->spawndelay1 = -1;
		md->spawndelay2 = -1;

		memcpy(md->npc_event, event, (sizeof(md->npc_event) > strlen(event)) ? strlen(event) : sizeof(md->npc_event));

		md->bl.type = BL_MOB;
		map_addiddb(&md->bl);
		if (mob_spawn(md->bl.id) == -1 && sd)
			printf("spawn -1\n");

		if (real_class == 1288) {
			struct guild_castle *gc = guild_mapname2gc(map[md->bl.m].name);
			if (gc) {
				md->hp = mob_db[real_class].max_hp + 2000 * gc->defense;
			}
		}
	}

	return (amount > 0) ? md->bl.id : 0;
}

/*==========================================
 * The mob appearance for one time (And area specification for scripts)
 *------------------------------------------
 */
int mob_once_spawn_area(struct map_session_data *sd, char *mapname,
	int x0, int y0, int x1, int y1,
	const char *mobname, int class, int amount, const char *event) {

	int x, y, i, max, lx = -1, ly = -1, id = 0;
	int m;

	if (strcmp(mapname, "this") == 0)
		m = sd->bl.m;
	else
		m = map_mapname2mapid(mapname);

	if (m < 0 || amount <= 0 || (class >= 0 && class <= 1000) || class >= (MAX_MOB_DB * 3)) // A summon is stopped if a value is unusual
		return 0;

	max = (y1 - y0 + 1) * (x1 - x0 + 1) * 3;
	if (max > 1000)
		max = 1000;

	for(i = 0; i < amount; i++) {
		int j;
		j = 0;
		do{
			x = rand() % (x1 - x0 + 1) + x0;
			y = rand() % (y1 - y0 + 1) + y0;
			j++;
		} while(map_getcell(m, x, y, CELL_CHKNOPASS_NPC) && j < max);
		if (j >= max) {
			if (lx >= 0) { // Since reference went wrong, the place which boiled before is used
				x = lx;
				y = ly;
			} else
				return 0; // Since reference of the place which boils first went wrong, it stops
		}
		if (x == 0 || y == 0)
			printf("mob_once_spawn_area: xory=0, x=%d,y=%d,x0=%d,y0=%d.\n", x, y, x0, y0);
		id = mob_once_spawn(sd, mapname, x, y, mobname, class, 1, event);
		lx = x;
		ly = y;
	}

	return id;
}

/*==========================================
 * Summoning War of Emperium Guardians
 *------------------------------------------
 */
int mob_spawn_guardian(struct map_session_data *sd,char *mapname,
	int x, int y, const char *mobname, int class, int amount, const char *event, int guardian) {

	struct mob_data *md = NULL;
	int m, count = 1, lv = 255, real_class;

	if (sd)
		lv = sd->status.base_level;

	if (sd && strcmp(mapname, "this") == 0)
		m = sd->bl.m;
	else
		m = map_mapname2mapid(mapname);

	if (m < 0 || amount <= 0 || (class >= 0 && class <= 1000) || class >= (MAX_MOB_DB * 3))
		return 0;

	if (class < 0)
		return 0;

	real_class = class;
	if (class >= (MAX_MOB_DB * 2)) // Large/Tiny mobs
		real_class -= (MAX_MOB_DB * 2); // Big
	else if (class >= MAX_MOB_DB)
		real_class -= MAX_MOB_DB; // Small

	if (real_class >= 0 && real_class <= 1000)
		return 0;

	if (sd) {
		if (x <= 0 || y <= 0) {
			if (x <= 0) x = sd->bl.x + rand() % 3 - 1;
			if (y <= 0) y = sd->bl.y + rand() % 3 - 1;
			if (map_getcell(m, x, y, CELL_CHKNOPASS_NPC)) {
				x = sd->bl.x;
				y = sd->bl.y;
			}
		}
	} else if (x <= 0 || y <= 0) {
		int j = 0;
		do {
			x = rand() % (map[m].xs - 2) + 1;
			y = rand() % (map[m].ys - 2) + 1;
			j++;
		} while (map_getcell(m, x, y, CELL_CHKNOPASS_NPC) && j < 256);
		if (j >= 256) {
			x = 0;
			y = 0;
		}
	}

	for(count = 0; count < amount; count++) {
		struct guild_castle *gc;
		CALLOC(md, struct mob_data, 1);

		if (class >= (MAX_MOB_DB * 2)) // Large/Tiny mobs
			md->size = 2; // Big
		else if (class >= MAX_MOB_DB)
			md->size = 1; // Small

		if (mob_db[real_class].mode & 0x02) {
			CALLOC(md->lootitem, struct item, LOOTITEM_SIZE);
		} else
			md->lootitem = NULL;

		mob_spawn_dataset(md, mobname, real_class);
		md->bl.m = m;
		md->bl.x = x;
		md->bl.y = y;
		md->m = m;
		md->x0 = x;
		md->y0 = y;
		md->xs = 0;
		md->ys = 0;
		md->spawndelay1 = -1; // Only once is a flag
		md->spawndelay2 = -1; // Only once is a flag

		memcpy(md->npc_event, event, sizeof(md->npc_event));

		md->bl.type = BL_MOB;
		map_addiddb(&md->bl);
		mob_spawn(md->bl.id);

		gc = guild_mapname2gc(map[md->bl.m].name);
		if (gc) {
			switch (guardian) {
			case 0:
				md->hp = gc->Ghp0;
				gc->GID0 = md->bl.id;
				break;
			case 1:
				md->hp = gc->Ghp1;
				gc->GID1 = md->bl.id;
				break;
			case 2:
				md->hp = gc->Ghp2;
				gc->GID2 = md->bl.id;
				break;
			case 3:
				md->hp = gc->Ghp3;
				gc->GID3 = md->bl.id;
				break;
			case 4:
				md->hp = gc->Ghp4;
				gc->GID4 = md->bl.id;
				break;
			case 5:
				md->hp = gc->Ghp5;
				gc->GID5 = md->bl.id;
				break;
			case 6:
				md->hp = gc->Ghp6;
				gc->GID6 = md->bl.id;
				break;
			case 7:
				md->hp = gc->Ghp7;
				gc->GID7 = md->bl.id;
				break;
			}
		}
	}

	return (amount > 0) ? md->bl.id : 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int mob_exclusion_add(struct mob_data *md,int type,int id) {

	nullpo_retr(0, md);

	if (type == 1)
		md->exclusion_src=id;
	if (type == 2)
		md->exclusion_party=id;
	if (type == 3)
		md->exclusion_guild=id;

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int mob_exclusion_check(struct mob_data *md,struct map_session_data *sd) {

	nullpo_retr(0, sd);
	nullpo_retr(0, md);

	if (sd->bl.type == BL_PC){
		if (md->exclusion_src && md->exclusion_src==sd->bl.id)
			return 1;
		if (md->exclusion_party && md->exclusion_party==sd->status.party_id)
			return 2;
		if (md->exclusion_guild && md->exclusion_guild==sd->status.guild_id)
			return 3;
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int mob_get_viewclass(int class) {

	return mob_db[class].view_class;
}

int mob_get_sex(int class) {

	return mob_db[class].sex;
}

short mob_get_hair(int class) {

	return mob_db[class].hair;
}

short mob_get_hair_color(int class) {

	return mob_db[class].hair_color;
}

short mob_get_weapon(int class) {

	return mob_db[class].weapon;
}

short mob_get_shield(int class) {

	return mob_db[class].shield;
}

short mob_get_head_top(int class) {

	return mob_db[class].head_top;
}

short mob_get_head_mid(int class) {

	return mob_db[class].head_mid;
}

short mob_get_head_buttom(int class) {

	return mob_db[class].head_buttom;
}

short mob_get_clothes_color(int class) {

	return mob_db[class].clothes_color;
}

int mob_get_equip(int class) {

	return mob_db[class].equip;
}

/*==========================================
 *
 *------------------------------------------
 */
static int calc_next_walk_step(struct mob_data *md) {

	nullpo_retr(0, md);

	if (md->walkpath.path_pos >= md->walkpath.path_len)
		return -1;
	if (md->walkpath.path[md->walkpath.path_pos] & 1) {
		status_calc_speed(&md->bl);
		return (md->speed * 14 / 10);
	}

	status_calc_speed(&md->bl);
	return md->speed;
}

static int mob_walktoxy_sub(struct mob_data *md);

/*==========================================
 *
 *------------------------------------------
 */
static void mob_walk(struct mob_data *md, unsigned int tick, int data) {

	int moveblock;
	int i;
	static int dirx[8] = {0, -1, -1, -1,  0,  1, 1, 1};
	static int diry[8] = {1,  1,  0, -1, -1, -1, 0, 1};
	int x,y,dx,dy;

	md->state.state = MS_IDLE;
	if (md->walkpath.path_pos >= md->walkpath.path_len || md->walkpath.path_pos != data)
		return;

	md->walkpath.path_half ^= 1;
	if (md->walkpath.path_half == 0) {
		md->walkpath.path_pos++;
		if (md->state.change_walk_target){
			mob_walktoxy_sub(md);
			return;
		}
	} else {
		if (md->walkpath.path[md->walkpath.path_pos] >= 8)
			return;

		x = md->bl.x;
		y = md->bl.y;
		if (map_getcell(md->bl.m, x, y, CELL_CHKNOPASS)) {
			mob_stop_walking(md, 1);
			return;
		}
		md->dir = md->walkpath.path[md->walkpath.path_pos];
		dx = dirx[md->dir];
		dy = diry[md->dir];

		if (!(status_get_mode(&md->bl)&0x20) && map_getcell(md->bl.m, x + dx, y + dy, CELL_CHKBASILICA)) {
			mob_stop_walking(md, 1);
			return;
		}

		if (map_getcell(md->bl.m, x + dx, y + dy, CELL_CHKNOPASS)) {
			mob_walktoxy_sub(md);
			return;
		}

		moveblock = (x / BLOCK_SIZE != (x + dx) / BLOCK_SIZE || y / BLOCK_SIZE != (y + dy) / BLOCK_SIZE);

		md->state.state = MS_WALK;
		map_foreachinmovearea(clif_moboutsight, md->bl.m, x-AREA_SIZE, y-AREA_SIZE, x+AREA_SIZE, y+AREA_SIZE, dx,dy, BL_PC, md);

		x += dx;
		y += dy;
		if (md->min_chase > 13)
			md->min_chase--;

		skill_unit_move(&md->bl, tick, 0);
		if (moveblock) map_delblock(&md->bl);
		md->bl.x = x;
		md->bl.y = y;
		if (moveblock) map_addblock(&md->bl);
		skill_unit_move(&md->bl, tick, 1);

		map_foreachinmovearea(clif_mobinsight, md->bl.m, x-AREA_SIZE, y-AREA_SIZE, x+AREA_SIZE, y+AREA_SIZE, -dx, -dy, BL_PC, md);
		md->state.state = MS_IDLE;

		if (md->option&4)
			skill_check_cloaking(&md->bl);
	}
	if ((i = calc_next_walk_step(md)) > 0) {
		i = i >> 1;
		if (i < 1 && md->walkpath.path_half == 0)
			i = 1;
		md->timer = add_timer(tick + i, mob_timer, md->bl.id, md->walkpath.path_pos);
		md->state.state = MS_WALK;

		if (md->walkpath.path_pos >= md->walkpath.path_len)
			clif_fixmobpos(md);
	}

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
static void mob_attack(struct mob_data *md, unsigned int tick, int data) {

	struct block_list *tbl = NULL;
	struct map_session_data *tsd = NULL;
	struct mob_data *tmd = NULL;

	int mode, race, range, unlock_target = 0;

	md->min_chase = 13;
	md->state.state = MS_IDLE;
	md->state.skillstate = MSS_IDLE;

	if (md->skilltimer != -1)
		return;

	if (md->opt1 > 0 || md->option & 2)
		return;

	if (md->sc_data[SC_AUTOCOUNTER].timer != -1)
		return;

	if (md->sc_data[SC_BLADESTOP].timer != -1)
		return;

	if ((tbl = map_id2bl(md->target_id)) == NULL) {
		md->target_id = 0;
		md->state.targettype = NONE_ATTACKABLE;
		return;
	}

	if (tbl->type == BL_PC)
		tsd = (struct map_session_data *)tbl;
	else if (tbl->type == BL_MOB)
		tmd = (struct mob_data *)tbl;
	else
		return;

	if (!md->mode)
		mode = mob_db[md->class].mode;
	else
		mode = md->mode;

	race = mob_db[md->class].race;

	if (!(mode & 0x80)) {
		md->target_id = 0;
		md->state.targettype = NONE_ATTACKABLE;
		return;
	}

	if (md->bl.m != tbl->m || tbl->prev == NULL || distance(md->bl.x, md->bl.y, tbl->x, tbl->y) >= 13) {
		unlock_target = 1;
	} else if (tsd) {
		if (pc_isdead(tsd) || pc_isinvisible(tsd) || tsd->invincible_timer != -1 || tsd->sc_data[SC_TRICKDEAD].timer != -1)
			unlock_target = 1;
		if (!(mode & 0x20) && ((tsd->sc_data[SC_BASILICA].timer != -1 || tsd->perfect_hiding) && ((race != 4 || race != 6) && (pc_ishiding(tsd) || pc_iscloaking(tsd) || pc_ischasewalk(tsd) || tsd->state.gangsterparadise))))
			unlock_target = 1;
	}

	if (unlock_target == 1) {
		md->target_id = 0;
		md->attacked_id = 0;
		md->state.targettype = NONE_ATTACKABLE;
		mob_stop_walking(md, 1);
		return;
	}

	range = mob_db[md->class].range;
	if (mode & 1)
		range++;
	if (distance(md->bl.x, md->bl.y, tbl->x, tbl->y) > range)
		return;
	if (battle_config.monster_attack_direction_change)
		md->dir = map_calc_dir(&md->bl, tbl->x, tbl->y);

	md->state.skillstate = MSS_ATTACK;
	if (mobskill_use(md, tick, -2))
		return;

	md->target_lv = battle_weapon_attack(&md->bl, tbl, tick, 0);

	if (!(battle_config.monster_cloak_check_type & 2) && md->sc_data[SC_CLOAKING].timer != -1)
		status_change_end(&md->bl, SC_CLOAKING, -1);

	md->attackabletime = tick + status_get_adelay(&md->bl);

	md->timer = add_timer(md->attackabletime, mob_timer, md->bl.id, 0);
	md->state.state = MS_ATTACK;

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
int mob_stopattacked(struct map_session_data *sd, va_list ap)
{
	int id;

	nullpo_retr(0, ap);

	id = va_arg(ap, int);
	if (sd->attacktarget == id)
		pc_stopattack(sd);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
void mob_changestate(struct mob_data *md, int state, int type) {

	int i;

	nullpo_retv(md);

	if (md->timer != -1) {
		delete_timer(md->timer, mob_timer);
		md->timer = -1;
	}
	md->state.state = state;

	switch(state) {
	case MS_WALK:
		if ((i = calc_next_walk_step(md)) > 0) {
			i = i>>2;
			md->timer = add_timer(gettick_cache + i, mob_timer, md->bl.id, 0);
		}
		else
			md->state.state = MS_IDLE;
		break;
	case MS_ATTACK:
		i = DIFF_TICK(md->attackabletime, gettick_cache);
		if (i > 0 && i < 2000)
			md->timer = add_timer(md->attackabletime, mob_timer, md->bl.id, 0);
		else if (type) {
			md->attackabletime = gettick_cache + status_get_amotion(&md->bl);
			md->timer = add_timer(md->attackabletime, mob_timer, md->bl.id, 0);
		}
		else {
			md->attackabletime = gettick_cache + 1;
			md->timer = add_timer(md->attackabletime, mob_timer, md->bl.id, 0);
		}
		break;
	case MS_DELAY:
		md->timer = add_timer(gettick_cache + type, mob_timer, md->bl.id, 0);
		break;
	case MS_DEAD:
		skill_castcancel(&md->bl, 0, 0);
		md->state.skillstate = MSS_DEAD;
		md->last_deadtime = gettick_cache;
		clif_foreachclient(mob_stopattacked, md->bl.id);
		skill_unit_move(&md->bl, gettick_cache, 0);
		status_change_clear(&md->bl,2);
		skill_clear_unitgroup(&md->bl);
		skill_cleartimerskill(&md->bl);
		if (md->deletetimer != -1) {
			delete_timer(md->deletetimer, mob_timer_delete);
			md->deletetimer = -1;
		}
		md->hp = md->target_id = md->attacked_id = md->attacked_count = 0;
		md->state.targettype = NONE_ATTACKABLE;
		break;
	}

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
static int mob_timer(int tid, unsigned int tick, int id, int data) {

	struct mob_data *md;
	struct block_list *bl;

	if ((bl = map_id2bl(id)) == NULL)
		return 1;

	if (bl->type != BL_MOB)
		return 1;

	md = (struct mob_data*)bl;

	if (md->timer != tid) {
		if (battle_config.error_log)
			printf("mob_timer %d != %d\n", md->timer, tid);
		return 0;
	}

	md->timer = -1;
	if (md->bl.prev == NULL || md->state.state == MS_DEAD)
		return 1;

	map_freeblock_lock();
	switch(md->state.state) {
	case MS_WALK:
		mob_walk(md, tick, data);
		break;
	case MS_ATTACK:
		mob_attack(md, tick, data);
		break;
	case MS_DELAY:
		mob_changestate(md, MS_IDLE, 0);
		break;
	default:
		if (battle_config.error_log)
			printf("mob_timer : state: %d ?\n", md->state.state);
		break;
	}
	map_freeblock_unlock();
	if (md->timer == -1)
		mob_changestate(md, MS_WALK, 0);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
static int mob_walktoxy_sub(struct mob_data *md) {

	struct walkpath_data wpd;
	int x, y;
	static int dirx[8] = {0,-1,-1,-1,0,1,1,1};
	static int diry[8] = {1,1,0,-1,-1,-1,0,1};

	nullpo_retr(0, md);

	if (path_search(&wpd,md->bl.m,md->bl.x,md->bl.y,md->to_x,md->to_y,md->state.walk_easy))
		return 1;

	x = md->bl.x + dirx[wpd.path[0]];
	y = md->bl.y + diry[wpd.path[0]];
	if (map_getcell(md->bl.m, x, y, CELL_CHKBASILICA) && !(status_get_mode(&md->bl) & 0x20)) {
		md->state.change_walk_target = 0;
		return 1;
	}

	memcpy(&md->walkpath, &wpd, sizeof(wpd));

	md->state.change_walk_target = 0;
	mob_changestate(md, MS_WALK, 0);
	clif_movemob(md);

	return 0;
}

/*==========================================
*
*------------------------------------------
*/
static int mob_randomxy(struct mob_data *md) {

	int i, x, y, d;

	d = rand() % 7 + 2;
	for(i = 0; i < 3; i++) {
		int r = rand();
		x = md->bl.x + r % (d * 2 + 1) - d;
		y = md->bl.y + r / (d * 2 + 1) % (d * 2 + 1) - d;
		if (map_getcell(md->bl.m, x, y, CELL_CHKPASS)) {
			md->to_x = x;
			md->to_y = y;
			return 0;
		}
	}
	
	return 1;
}

/*==========================================
 *
 *------------------------------------------
 */
int mob_walktoxy(struct mob_data *md,int x,int y,int easy) {

	struct walkpath_data wpd;

	nullpo_retr(0, md);

	if (md->state.state == MS_WALK && path_search(&wpd,md->bl.m,md->bl.x,md->bl.y,x,y,easy) )
		return 1;

	if (md->sc_data[SC_CONFUSION].timer != -1) {
		if (mob_randomxy(md))
			return 1;
	} else {
		md->to_x = x;
		md->to_y = y;
	}

	md->state.walk_easy = easy;

	if (md->state.state == MS_WALK)
		md->state.change_walk_target = 1;
	else
		return mob_walktoxy_sub(md);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
static int mob_delayspawn(int tid,unsigned int tick,int m,int n) {

	mob_spawn(m);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int mob_setdelayspawn(int id) {

	unsigned int spawntime, spawntime1, spawntime2, spawntime3;
	struct mob_data *md;
	struct block_list *bl;

	if ((bl = map_id2bl(id)) == NULL)
		return -1;

	if (!bl || !bl->type || bl->type != BL_MOB)
		return -1;

	nullpo_retr(-1, md = (struct mob_data*)bl);

	if (!md || md->bl.type != BL_MOB)
		return -1;

	// Processing of mob which is not revitalized
	if (md->spawndelay1 == -1 && md->spawndelay2 == -1 && md->n == 0) {
		map_deliddb(&md->bl);
		if (md->lootitem) {
			FREE(md->lootitem);
		}
		map_freeblock(md); // Instead of [ of free ]
		return 0;
	}

	spawntime1 = md->last_spawntime + md->spawndelay1;
	spawntime2 = md->last_deadtime + md->spawndelay2;
	spawntime3 = gettick_cache + 5000;
	if (DIFF_TICK(spawntime1,spawntime2) > 0)
		spawntime = spawntime1;
	else
		spawntime = spawntime2;
	if (DIFF_TICK(spawntime3, spawntime) > 0)
		spawntime = spawntime3;

	add_timer(spawntime, mob_delayspawn, id, 0);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int mob_spawn(int id) {

	int x = 0, y = 0, i;
	unsigned int skill_delay;
	struct mob_data *md;
	struct block_list *bl;

	nullpo_retr(-1, bl = map_id2bl(id));

	if (!bl || !bl->type || bl->type != BL_MOB)
		return -1;

	nullpo_retr(-1, md = (struct mob_data*)bl);

	if (!md || !md->bl.type || md->bl.type != BL_MOB)
		return -1;

	md->last_spawntime = gettick_cache;
	if (md->bl.prev != NULL) {
		map_delblock(&md->bl);
	} else {
		if (md->class != md->base_class) {
			if (strncmp(md->name, mob_db[md->class].jname, 24) == 0) {
				memset(md->name, 0, sizeof(md->name));
				strncpy(md->name, mob_db[md->base_class].jname, 24);
			}
			md->speed = mob_db[md->base_class].speed;
		}
		md->class = md->base_class;
	}

	md->bl.m = md->m;
	i = 0;
	do {
		if (md->x0 == 0 && md->y0 == 0) {
			x = rand() % (map[md->bl.m].xs-2) + 1;
			y = rand() % (map[md->bl.m].ys-2) + 1;
		} else {
			x = md->x0 + rand() % (md->xs + 1) - md->xs / 2;
			y = md->y0 + rand() % (md->ys + 1) - md->ys / 2;
			if ((md->xs < i / 4 || md->ys < i / 4) && map_getcell(md->bl.m, x, y, CELL_CHKNOPASS_NPC)) {
				if (md->xs < i / 4)
					x = md->x0 + rand() % (i / 4 + 1) - (i / 4) / 2;
				if (md->ys < i / 4)
					y = md->y0 + rand() % (i / 4 + 1) - (i / 4) / 2;
			}
		}
		i++;
	} while (map_getcell(md->bl.m, x, y, CELL_CHKNOPASS_NPC) && i < 50);

	if (i >= 50) {
		add_timer(gettick_cache + 5000, mob_delayspawn, id, 0);
		return 1;
	}

	md->to_x = md->bl.x = x;
	md->to_y = md->bl.y = y;
	md->dir = 0;
	md->target_dir = 0;

	memset(&md->state, 0, sizeof(md->state));
	md->attacked_id = 0;
	md->attacked_count = 0;
	md->target_id = 0;
	md->move_fail_count = 0;

	if (!md->speed)
		md->speed = mob_db[md->class].speed;
	md->def_ele = mob_db[md->class].element;

	if (!md->level)
		md->level = mob_db[md->class].lv;

	md->master_id = 0;
	md->master_dist = 0;

	md->state.state = MS_IDLE;
	md->state.skillstate = MSS_IDLE;
	md->timer = -1;
	md->last_thinktime = gettick_cache;
	md->next_walktime = gettick_cache + rand() % 50 + 5000;
	md->attackabletime = gettick_cache;
	md->canmove_tick = gettick_cache;

	md->guild_id = 0;
	if (md->class >= 1285 && md->class <= 1288) {
		struct guild_castle *gc = guild_mapname2gc(map[md->bl.m].name);
		if (gc)
			md->guild_id = gc->guild_id;
	}

	md->deletetimer = -1;

	md->skilltimer = -1;
	skill_delay = gettick_cache - 1000 * 3600 * 10;
	for(i = 0; i < MAX_MOBSKILL; i++)
		md->skilldelay[i] = skill_delay;
	md->skillid = 0;
	md->skilllv = 0;

	memset(md->dmglog, 0, sizeof(md->dmglog));
	if (md->lootitem)
		memset(md->lootitem, 0, sizeof(md->lootitem));
	md->lootitem_count = 0;

	for(i = 0; i < MAX_MOBSKILLTIMERSKILL; i++)
		md->skilltimerskill[i].timer = -1;

	for(i = 0; i < SC_MAX; i++) {
		md->sc_data[i].timer = -1;
		md->sc_data[i].val1 = md->sc_data[i].val2 = md->sc_data[i].val3 = md->sc_data[i].val4 = 0;
	}
	md->sc_count = 0;
	md->opt1 = md->opt2 = md->opt3 = md->option = 0;

	memset(md->skillunit, 0, sizeof(md->skillunit));
	memset(md->skillunittick, 0, sizeof(md->skillunittick));

	md->hp = status_get_max_hp(&md->bl);
	if (md->hp <= 0) {
		mob_makedummymobdb(md->class);
		md->hp = status_get_max_hp(&md->bl);
	}

	map_addblock(&md->bl);
	skill_unit_move(&md->bl, gettick_cache, 1);

	clif_spawnmob(md);
	
	mobskill_use(md, gettick_cache, MSC_SPAWN);

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
void mob_stopattack(struct mob_data *md) {

	md->target_id = 0;
	md->state.targettype = NONE_ATTACKABLE;
	md->attacked_id = 0;
	md->attacked_count = 0;

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void mob_stop_walking(struct mob_data *md, int type) {

	nullpo_retv(md);

	if (md->state.state == MS_WALK || md->state.state == MS_IDLE) {
		int dx = 0, dy = 0;

		md->walkpath.path_len = 0;
		if (type & 4){
			dx = md->to_x-md->bl.x;
			if (dx < 0)
				dx = -1;
			else if (dx > 0)
				dx = 1;
			dy = md->to_y-md->bl.y;
			if (dy < 0)
				dy = -1;
			else if (dy > 0)
				dy = 1;
		}
		md->to_x = md->bl.x + dx;
		md->to_y = md->bl.y + dy;
		if (dx != 0 || dy != 0){
			mob_walktoxy_sub(md);
			return;
		}
		mob_changestate(md, MS_IDLE, 0);
	}
	if (type & 0x01)
		clif_fixmobpos(md);
	if (type & 0x02) {
		if (md->canmove_tick < gettick_cache)
			md->canmove_tick = gettick_cache + status_get_dmotion(&md->bl);
	}

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
int mob_can_reach(struct mob_data *md,struct block_list *bl,int range) {

	int dx,dy;
	struct walkpath_data wpd;
	int i;

	nullpo_retr(0, md);
	nullpo_retr(0, bl);

	dx = abs(bl->x - md->bl.x);
	dy = abs(bl->y - md->bl.y);

	//=========== Guildcastle Guardian no search start ===========
	// When players are the guild castle member not attack them
	if (md->class >= 1285 && md->class <= 1287) {
		struct map_session_data *sd;
		struct guild *g;
		struct guild_castle *gc = guild_mapname2gc(map[bl->m].name);

		if (gc && agit_flag == 0) // Guardians will not attack during non-woe time
			return 0; // End addition

		if (bl->type == BL_PC) {
			if (!gc)
				return 0;
			sd = (struct map_session_data *)bl;
			if (sd->status.guild_id > 0) {
				g = guild_search(sd->status.guild_id); // Don't attack guild members
				if (g && g->guild_id == gc->guild_id)
					return 0;
				if (g && guild_isallied(g, gc))
					return 0;
			}
		}
	}
	//========== Guild castle Guardian no search eof ==============

	if (bl->type == BL_PC && battle_config.monsters_ignore_gm) // Option to have monsters ignore GMs
	{
		if (((struct map_session_data *)bl)->GM_level >= battle_config.monsters_ignore_gm)
			return 0;
	}

	if ( md->bl.m != bl->m)
		return 0;

	if ( range > 0 && range < ((dx>dy)?dx:dy))
		return 0;

	if ( md->bl.x==bl->x && md->bl.y==bl->y)
		return 1;

	// Obstacle judging
	wpd.path_len	= 0;
	wpd.path_pos	= 0;
	wpd.path_half	= 0;
	if (path_search(&wpd, md->bl.m, md->bl.x, md->bl.y, bl->x, bl->y, 0) != -1)
		return 1;

	if (bl->type != BL_PC && bl->type != BL_MOB)
		return 0;

	// It judges whether it can adjoin or not
	dx=(dx>0)?1:((dx<0)?-1:0);
	dy=(dy>0)?1:((dy<0)?-1:0);
	if (path_search(&wpd, md->bl.m, md->bl.x, md->bl.y, bl->x-dx, bl->y-dy, 0) != -1)
		return 1;
	for(i = -1; i < 2; i++) {
		// If was if(path_search(&wpd,md->bl.m,md->bl.x,md->bl.y,bl->x-1+i/3,bl->y-1+i%3,0)!=-1)
		if (path_search(&wpd, md->bl.m, md->bl.x, md->bl.y, bl->x-1, bl->y+i,0)!= -1)
			return 1;
		if (path_search(&wpd, md->bl.m, md->bl.x, md->bl.y, bl->x, bl->y+i, 0) != -1)
			return 1;
		if (path_search(&wpd, md->bl.m, md->bl.x, md->bl.y, bl->x+1, bl->y+i, 0)!= -1)
			return 1;
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int mob_target(struct mob_data *md,struct block_list *bl,int dist) {

	struct map_session_data *sd;
	struct status_change *sc_data;
	short *option;
	int mode,race;

	nullpo_retr(0, md);
	nullpo_retr(0, bl);

	sc_data	= status_get_sc_data(bl);
	option	= status_get_option(bl);
	race	= mob_db[md->class].race;

	if (!md->mode)
		mode = mob_db[md->class].mode;
	else
		mode = md->mode;
	if (!(mode & 0x80)) {
		md->target_id = 0;
		return 0;
	}
	if ((md->target_id > 0 && md->state.targettype == ATTACKABLE) && (!(mode & 0x04) || rand() % 100 > 25) &&
	    !(md->state.provoke_flag && md->state.provoke_flag == bl->id))
		return 0;

	if (mode & 0x20 ||
	    (sc_data && sc_data[SC_TRICKDEAD].timer == -1 && sc_data[SC_BASILICA].timer == -1 &&
	     ((option && !(*option & 0x06)) || race == 4 || race == 6 || mode & 0x100))) {
		if (bl->type == BL_PC) {
			nullpo_retr(0, sd = (struct map_session_data *)bl);
			if (sd->invincible_timer != -1 || pc_isinvisible(sd))
				return 0;
			if (!(mode & 0x20) && race != 4 && race != 6 && !(mode & 0x100) && sd->state.gangsterparadise)
				return 0;
		}

		md->target_id = bl->id;
		if (bl->type == BL_PC || bl->type == BL_MOB)
			md->state.targettype = ATTACKABLE;
		else
			md->state.targettype = NONE_ATTACKABLE;
		if (md->state.provoke_flag)
			md->state.provoke_flag = 0;
		md->min_chase = dist + 13;
		if (md->min_chase > 26)
			md->min_chase = 26;
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
static int mob_special_ai_sub_hard_activesearch(struct block_list *bl, va_list ap) {

	struct map_session_data *tsd = NULL;
	struct mob_data *smd, *tmd = NULL;
	struct block_list *tbl;
	int mode, race, dist, dist2, *pcc;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);

	smd = va_arg(ap, struct mob_data *);
	pcc = va_arg(ap, int *);

	if (bl->type == BL_PC) {
		tsd = (struct map_session_data *)bl;
	} else if (bl->type == BL_MOB) {
		tmd = (struct mob_data *)bl;
	} else
		return 0;

	if (battle_check_target(&smd->bl, bl, BCT_ENEMY) <= 0)
		return 0;

	if (!smd->mode)
		mode = mob_db[smd->class].mode;
	else
		mode = smd->mode;

	race = mob_db[smd->class].race;

	if (tsd &&
	    tsd->bl.m == smd->bl.m &&
	    !pc_isdead(tsd) &&
	    tsd->invincible_timer == -1 &&
	    !pc_isinvisible(tsd)) {
		if (mode & 0x20 ||
		    (tsd->sc_data[SC_TRICKDEAD].timer == -1 && tsd->sc_data[SC_BASILICA].timer == -1 &&
		     ((!pc_ishiding(tsd) && !tsd->state.gangsterparadise) || ((race == 4 || race == 6) && !tsd->perfect_hiding)))) {
			// Not already a valid target
			if (!smd->target_id || smd->state.targettype == NONE_ATTACKABLE) {
				if (mob_can_reach(smd, bl, 12)) {
					smd->target_id = bl->id;
					smd->state.targettype = ATTACKABLE;
					smd->min_chase = 13;
					(*pcc) = 1;
				}
			} else { // Already a target
				tbl = map_id2bl(smd->target_id);
				dist = distance(smd->bl.x, smd->bl.y, tbl->x, tbl->y);
				if (dist < 2) // If item is just near the monster, don't change
					return 0;
				dist2 = distance(smd->bl.x, smd->bl.y, bl->x, bl->y);
				if (dist2 < dist) { // Check if new target is nearest than the first
					if (mob_can_reach(smd, bl, 12)) {
						smd->target_id = bl->id;
						smd->state.targettype = ATTACKABLE;
						smd->min_chase = 13;
						(*pcc) = 1;
					}
				} else if (dist2 == dist && // Check if new target is nearest than the first
				           rand() % 32768 < 32768 / (++(*pcc))) { // It is made a probability, such as within the limits PC
					if (mob_can_reach(smd, bl, 12)) { // Reachability judging
						smd->target_id = bl->id; // targettype and min_chase stay same
					}
				}
			}
		}
	} else if (tmd &&
	           tmd->bl.m == smd->bl.m) { // Necessary?
		if (!smd->target_id || smd->state.targettype == NONE_ATTACKABLE) { // Not already a valid target
			if (mob_can_reach(smd, bl, 12)) {
				smd->target_id = bl->id;
				smd->state.targettype = ATTACKABLE;
				smd->min_chase = 13;
				(*pcc) = 1;
			}
		} else { // Already a target
			tbl = map_id2bl(smd->target_id);
			dist = distance(smd->bl.x, smd->bl.y, tbl->x, tbl->y);
			if (dist < 2) // If item is just near the monster, don't change
				return 0;
			dist2 = distance(smd->bl.x, smd->bl.y, bl->x, bl->y);
			if (dist2 < dist) { // Check if new target is nearest than the first
				if (mob_can_reach(smd, bl, 12)) {
					smd->target_id = bl->id;
					smd->state.targettype = ATTACKABLE;
					smd->min_chase = 13;
					(*pcc) = 1;
				}
			} else if (dist2 == dist && // Check if new target is nearest than the first
			           rand() % 32768 < 32768 / (++(*pcc))) { // It is made a probability, such as within the limits PC
				if (mob_can_reach(smd, bl, 12)) { // Reachability judging
					smd->target_id = bl->id; // targettype and min_chase stay same
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
static int mob_ai_sub_hard_activesearch(struct block_list *bl, va_list ap) {

	struct map_session_data *tsd;
	struct mob_data *smd;
	struct block_list *tbl;
	int mode, race, dist, dist2, *pcc;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);

	smd = va_arg(ap, struct mob_data *);
	pcc = va_arg(ap, int *);

	if ((tsd = (struct map_session_data *)bl) == NULL)
		return 0;

	if (battle_check_target(&smd->bl, bl, BCT_ENEMY) <= 0)
		return 0;

	if (!smd->mode)
		mode = mob_db[smd->class].mode;
	else
		mode = smd->mode;

	race = mob_db[smd->class].race;

	if (tsd->bl.m == smd->bl.m && // Necessary?
	    !pc_isdead(tsd) &&
	    tsd->invincible_timer == -1 &&
	    !pc_isinvisible(tsd)) {
		if (mode & 0x20 ||
		    (tsd->sc_data[SC_TRICKDEAD].timer == -1 && tsd->sc_data[SC_BASILICA].timer == -1 &&
		     ((!pc_ishiding(tsd) && !tsd->state.gangsterparadise) || ((race == 4 || race == 6) && !tsd->perfect_hiding)))) {
			if (!smd->target_id || smd->state.targettype == NONE_ATTACKABLE) { // Not already a valid target
				if (mob_can_reach(smd, bl, 12)) {
					smd->target_id = bl->id;
					smd->state.targettype = ATTACKABLE;
					smd->min_chase = 13;
					(*pcc) = 1;
				}
			} else { // Already a target
				tbl = map_id2bl(smd->target_id);
				dist = distance(smd->bl.x, smd->bl.y, tbl->x, tbl->y);
				if (dist < 2) // If item is just near the monster, don't change
					return 0;
				dist2 = distance(smd->bl.x, smd->bl.y, bl->x, bl->y);
				if (dist2 < dist) { // Check if new target is nearest than the first
					if (mob_can_reach(smd, bl, 12)) {
						smd->target_id = bl->id;
						smd->state.targettype = ATTACKABLE;
						smd->min_chase = 13;
						(*pcc) = 1;
					}
				} else if (dist2 == dist && // Check if new target is nearest than the first
				           rand() % 32768 < 32768 / (++(*pcc))) { // It is made a probability, such as within the limits PC
					if (mob_can_reach(smd, bl, 12)) { // Reachability judging
						smd->target_id = bl->id; // targettype and min_chase stay same
					}
				}
			}
		}
	}

	return 0;
}

/*==========================================
 * loot monster item search
 * in main loop, first time we call this function, the monster have no target.
 * if after it have a target in same loop, that is always an item.
 *------------------------------------------
 */
static int mob_ai_sub_hard_lootsearch(struct block_list *bl, va_list ap) {

	struct mob_data* md;
	struct block_list *tbl;
	int dist, dist2, *itc;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);

	md = va_arg(ap, struct mob_data *);
	itc = va_arg(ap, int *);

	if (bl->m == md->bl.m) { // Necessary?
		if (!md->target_id) { // Not already an item
			if (mob_can_reach(md, bl, 12)) { // Reachability judging
				md->target_id = bl->id;
				md->state.targettype = NONE_ATTACKABLE;
				md->min_chase = 13;
				(*itc) = 1;
			}
		} else { // Already an item
			tbl = map_id2bl(md->target_id);
			dist = distance(md->bl.x, md->bl.y, tbl->x, tbl->y);
			if (dist < 2) // If item is just near the monster, don't change
				return 0;
			dist2 = distance(md->bl.x, md->bl.y, bl->x, bl->y);
			if (dist2 < dist) { // Check if new item is nearest than the first
				if (mob_can_reach(md, bl, 12)) { // Reachability judging
					md->target_id = bl->id; // targettype and min_chase stay same
					(*itc) = 1;
				}
			} else if (dist2 == dist && // Check if new item is nearest than the first
			           rand() % 32768 < 32768 / (++(*itc))) { // It is made a probability, such as within the limits PC
				if (mob_can_reach(md, bl, 12)) { // Reachability judging
					md->target_id = bl->id; // targettype and min_chase stay same
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
static int mob_ai_sub_hard_linksearch(struct block_list *bl, va_list ap) {

	struct mob_data *tmd;
	struct mob_data* md;
	struct block_list *target;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);
	tmd = (struct mob_data *)bl;
	nullpo_retr(0, md = va_arg(ap, struct mob_data *));
	nullpo_retr(0, target = va_arg(ap, struct block_list *));

	// Same family free in a range at a link monster -- It will be made to lock if MOB is
	if (tmd->class == md->class && tmd->bl.m == md->bl.m && (!tmd->target_id || md->state.targettype == NONE_ATTACKABLE)) {
		if (mob_can_reach(tmd, target, 12)) { // Reachability judging
			tmd->target_id = md->attacked_id;
			tmd->state.targettype = ATTACKABLE;
			tmd->min_chase = 13;
		}
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
static int mob_ai_sub_hard_slavemob(struct mob_data *md, unsigned int tick) {

	struct mob_data *mmd=NULL;
	struct block_list *bl;
	int mode,race,old_dist;

	nullpo_retr(0, md);

	if ((bl = map_id2bl(md->master_id)) == NULL)
		return 0;

	mmd = (struct mob_data *)bl;
	// It is not main monster/leader
	if (mmd->bl.type != BL_MOB || mmd->bl.id != md->master_id)
		return 0;

	// Since it is in the map on which the master is not, Teleport is carried out and it pursues
	if (mmd->bl.m != md->bl.m)
	{
		mob_warp(md,mmd->bl.m,mmd->bl.x,mmd->bl.y,3);
		md->state.master_check = 1;
		return 0;
	}

	mode = mob_db[md->class].mode;

	// Distance with between slave and master is measured
	old_dist = md->master_dist;
	md->master_dist = distance(md->bl.x,md->bl.y,mmd->bl.x,mmd->bl.y);

	// Since the master was in near immediately before, teleport is carried out and it pursues
	if (old_dist < 10 && md->master_dist > 18) {
		mob_warp(md, -1, mmd->bl.x, mmd->bl.y, 3);
		md->state.master_check = 1;
		return 0;
	}

	// Although there is the master, since it is somewhat far, it approaches
	if ((!md->target_id || md->state.targettype == NONE_ATTACKABLE) && unit_can_move(&md->bl) &&
	    (md->walkpath.path_pos>=md->walkpath.path_len || md->walkpath.path_len==0) && md->master_dist<15){
		int i=0,dx,dy,ret;
		if (md->master_dist>4) {
			do {
				if (i<=5){
					dx=mmd->bl.x - md->bl.x;
					dy=mmd->bl.y - md->bl.y;
					if (dx < 0) dx += (rand() % ((dx < -3) ? 3 : -dx) + 1);
					else if (dx > 0) dx-=(rand() % ((dx > 3) ? 3 : dx) + 1);
					if (dy < 0) dy += (rand() % ((dy < -3) ? 3 : -dy) + 1);
					else if (dy > 0) dy-=(rand() % ((dy > 3) ? 3 : dy) + 1);
				}else{
					dx=mmd->bl.x - md->bl.x + rand()%7 - 3;
					dy=mmd->bl.y - md->bl.y + rand()%7 - 3;
				}

				ret=mob_walktoxy(md,md->bl.x+dx,md->bl.y+dy,0);
				i++;
			} while(ret && i<10);
		}
		else {
			do {
				dx = rand()%9 - 5;
				dy = rand()%9 - 5;
				if ( dx == 0 && dy == 0) {
					dx = (rand()%1)? 1:-1;
					dy = (rand()%1)? 1:-1;
				}
				dx += mmd->bl.x;
				dy += mmd->bl.y;

				ret=mob_walktoxy(md,mmd->bl.x+dx,mmd->bl.y+dy,0);
				i++;
			} while(ret && i<10);
		}

		md->next_walktime=tick+500;
		md->state.master_check = 1;
	}

	// There is the master, the master locks a target and he does not lock
	if ( (mmd->target_id>0 && mmd->state.targettype == ATTACKABLE) && (!md->target_id || md->state.targettype == NONE_ATTACKABLE) ){
		struct map_session_data *sd=map_id2sd(mmd->target_id);
		if (sd != NULL && !pc_isdead(sd) && sd->invincible_timer == -1 && !pc_isinvisible(sd)) {

			race = mob_db[md->class].race;
			if (mode&0x20 ||
				(sd->sc_data[SC_TRICKDEAD].timer == -1 && sd->sc_data[SC_BASILICA].timer == -1 &&
				((!pc_ishiding(sd) && !sd->state.gangsterparadise) || ((race == 4 || race == 6) && !sd->perfect_hiding)))) {

				md->target_id=sd->bl.id;
				md->state.targettype = ATTACKABLE;
				md->min_chase = 5 + distance(md->bl.x,md->bl.y,sd->bl.x,sd->bl.y);
				md->state.master_check = 1;
			}
		}
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
static int mob_unlocktarget(struct mob_data *md, unsigned int tick) {

	nullpo_retr(0, md);

	md->target_id = 0;
	md->state.targettype = NONE_ATTACKABLE;
	md->state.skillstate = MSS_IDLE;
	md->next_walktime = tick + rand() % 3000 + 3000;

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
static int mob_randomwalk(struct mob_data *md, unsigned int tick) {

	const int retrycount=20;
	int speed;

	nullpo_retr(0, md);

	status_calc_speed(&md->bl);
	speed = md->speed;
	if (DIFF_TICK(md->next_walktime,tick)<0){
		int i, x, y, c, d = 12 - md->move_fail_count;
		int mask[8][2] = {{0,1},{-1,1},{-1,0},{-1,-1},{0,-1},{1,-1},{1,0},{1,1}};
		if (d<5) d=5;
		for(i=0;i<retrycount;i++) {
			int r = rand();
			x = r % (d * 2 + 1) - d;
			y = r / (d * 2 + 1) % (d * 2 + 1) - d;
			if (md->target_dir) {
				if (x < 0) x = 0 - x;
				if (y < 0) y = 0 - y;
				x *= mask[md->target_dir-1][0];
				y *= mask[md->target_dir-1][1];
			}
			x += md->bl.x;
			y += md->bl.y;

			if (map_getcell(md->bl.m, x, y, CELL_CHKPASS) && mob_walktoxy(md, x, y, 1) == 0){
				md->move_fail_count=0;
				break;
			}
			if (i+1>=retrycount){
				md->move_fail_count++;
				md->target_dir = 0;
				if (md->move_fail_count > 1000) {
					if (battle_config.error_log)
						printf("MOB cant move. random spawn %d, class = %d\n", md->bl.id, md->class);
					md->move_fail_count = 0;
					mob_spawn(md->bl.id);
				}
			}
		}
		for(i=c=0;i<md->walkpath.path_len;i++) {
			if (md->walkpath.path[i]&1)
				c += speed * 14 / 10;
			else
				c += speed;
		}
		md->next_walktime = tick + rand() % 3000 + 3000 + c;
		md->state.skillstate = MSS_WALK;
		return 1;
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
static int mob_ai_sub_hard(struct block_list *bl, va_list ap) {

	struct mob_data *md,*tmd = NULL;
	struct map_session_data *tsd = NULL;
	struct block_list *tbl = NULL;
	struct flooritem_data *fitem;
	unsigned int tick;
	int i, dx, dy, ret, dist;
	int attack_type = 0;
	int mode, race;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);
	md = (struct mob_data*)bl;

	tick = va_arg(ap, unsigned int);

	if (DIFF_TICK(tick, md->last_thinktime) < MIN_MOBTHINKTIME)
		return 0;
	md->last_thinktime = tick;

	if (md->skilltimer != -1 || md->bl.prev == NULL) { // Under a skill aria and death
		if (DIFF_TICK(tick, md->next_walktime) > MIN_MOBTHINKTIME)
			md->next_walktime = tick;
		return 0;
	}

	// Abnormalities
	if ((md->opt1 > 0 && md->opt1 != 6) || md->state.state == MS_DELAY || md->sc_data[SC_BLADESTOP].timer != -1)
		return 0;
		
	if (!md->mode)
		mode = mob_db[md->class].mode;
	else
		mode = md->mode;

	race = mob_db[md->class].race;

	if (!(mode & 0x80) && md->target_id > 0)
		md->target_id = 0;

	if (md->attacked_id > 0 && mode & 0x08) {
		struct map_session_data *asd = map_id2sd(md->attacked_id);
		if (asd) {
			if (asd->invincible_timer == -1 && !pc_isinvisible(asd)) {
				map_foreachinarea(mob_ai_sub_hard_linksearch, md->bl.m,
				                  md->bl.x - 13, md->bl.y - 13,
				                  md->bl.x + 13, md->bl.y + 13,
				                  BL_MOB, md, &asd->bl);
			}
		}
	}
	
	if (mode > 0 && md->attacked_id > 0 && !md->target_id && md->state.targettype == NONE_ATTACKABLE && md->attacked_count++ > 3) {
		if (mobskill_use(md, tick, MSC_RUDEATTACKED) == 0 && unit_can_move(&md->bl)) {
			struct block_list *abl = map_id2bl(md->attacked_id);
			int dist = rand() % 10 + 1;
			int dir = abl ? map_calc_dir(abl, bl->x, bl->y) : rand() % 8;
			int mask[8][2] = {{0,1},{-1,1},{-1,0},{-1,-1},{0,-1},{1,-1},{1,0},{1,1}};
			mob_walktoxy(md, md->bl.x + dist * mask[dir][0], md->bl.y + dist * mask[dir][1], 0);
			md->next_walktime = tick;
		}
	}

	// It checks to see it was attacked first (If active, it is target change at 25% of probability)
	if (mode > 0 && md->attacked_id > 0 && (!md->target_id || md->state.targettype == NONE_ATTACKABLE ||
	    (mode & 0x04 && rand() % 100 < 25))) {
		struct block_list *abl = map_id2bl(md->attacked_id);
		struct map_session_data *asd = NULL;
		if (abl) {
			if (abl->type == BL_PC)
				asd = (struct map_session_data *)abl;
			if (asd == NULL || md->bl.m != abl->m || abl->prev == NULL || asd->invincible_timer != -1 || pc_isinvisible(asd) ||
			    (dist = distance(md->bl.x, md->bl.y, abl->x, abl->y)) >= 32 || battle_check_target(bl, abl, BCT_ENEMY) <= 0)
				md->attacked_id = 0;
			else {
				if (!md->target_id || (distance(md->bl.x, md->bl.y, abl->x, abl->y) < 3)) {
					md->target_id = md->attacked_id; // Set target
					md->state.targettype = ATTACKABLE;
					attack_type = 1;
					md->attacked_id = 0;
					md->min_chase = dist + 13;
					if (md->min_chase > 26)
						md->min_chase = 26;
				}
			}
		}
	}

	md->state.master_check = 0;
	// Processing of slave monster
	if (md->master_id > 0 && md->state.special_mob_ai == 0) // 0: Nothing, 1: Cannibalize, 2-3: Spheremine
		mob_ai_sub_hard_slavemob(md, tick);

	// ?? of a bitter taste TIVU monster
	if ((!md->target_id || md->state.targettype == NONE_ATTACKABLE) && mode & 0x04 && !md->state.master_check &&
		battle_config.monster_active_enable) {
		if (md->state.special_mob_ai) { // 0: Nothing, 1: Cannibalize, 2-3: Spheremine
			i = 0;
			map_foreachinarea(mob_special_ai_sub_hard_activesearch, md->bl.m,
			                  md->bl.x - 8, md->bl.y - 8, // Calculated distance in sub function was < 9 -> +/- 8 and no calculation inside the sub (and not AREA_SIZE * 2 = 40)
			                  md->bl.x + 8, md->bl.y + 8,
			                  0, md, &i);
		} else { // Most of agressive monsters are normal, so use a different function with no test about the type
			i = 0;
			map_foreachinarea(mob_ai_sub_hard_activesearch, md->bl.m,
			                  md->bl.x - 8, md->bl.y - 8, // Calculated distance in sub function was < 9 -> +/- 8 and no calculation inside the sub (and not AREA_SIZE * 2 = 40)
			                  md->bl.x + 8, md->bl.y + 8,
			                  BL_PC, md, &i);
		}
	}

	// The item search of a route monster
	if (!md->target_id && (mode & 0x02) && !md->state.master_check) {
		if (md->lootitem && (battle_config.monster_loot_type == 0 || md->lootitem_count < LOOTITEM_SIZE)) {
			i = 0;
			map_foreachinarea(mob_ai_sub_hard_lootsearch, md->bl.m,
			                  md->bl.x - 8, md->bl.y - 8, // Calculated distance in sub function was < 9 -> +/- 8 and only calculation inside the sub to compare distance of all items (and not AREA_SIZE * 2 = 40)
			                  md->bl.x + 8, md->bl.y + 8,
			                  BL_ITEM, md, &i);
		}
	}

	// It will attack, if the candidate for an attack is
	if (md->target_id > 0) {
		if ((tbl = map_id2bl(md->target_id)) && md->target_id == tbl->id && tbl->m == md->bl.m && tbl->prev != NULL) {
			if (tbl->type == BL_PC)
				tsd = (struct map_session_data *)tbl;
			else if (tbl->type == BL_MOB)
				tmd = (struct mob_data *)tbl;
			if (tsd || tmd) {
				if ((dist = distance(md->bl.x, md->bl.y, tbl->x, tbl->y)) >= md->min_chase)
					mob_unlocktarget(md, tick);
				else if (tsd && !(mode&0x20) && (tsd->sc_data[SC_TRICKDEAD].timer != -1 || tsd->sc_data[SC_BASILICA].timer != -1 ||
				          ((pc_ishiding(tsd) || tsd->state.gangsterparadise) &&
				          !((race == 4 || race == 6) && !tsd->perfect_hiding))))
					mob_unlocktarget(md, tick);
				else if (!battle_check_range(&md->bl, tbl, mob_db[md->class].range)) {
					if (!(mode & 1)) {
						mob_unlocktarget(md, tick);
						return 0;
					}
					if (!unit_can_move(&md->bl))
						return 0;
					md->state.skillstate = MSS_CHASE;
					mobskill_use(md, tick, -1);
					if (md->timer != -1 && md->state.state != MS_ATTACK && (DIFF_TICK(md->next_walktime, tick) < 0 || distance(md->to_x, md->to_y, tbl->x, tbl->y) < 2))
						return 0;
					if (!mob_can_reach(md, tbl, (md->min_chase > 13) ? md->min_chase : 13))
						mob_unlocktarget(md, tick);
					else {
						md->next_walktime = tick + 500;
						i = 0;
						do {
							if (i == 0) {
								dx = tbl->x - md->bl.x;
								dy = tbl->y - md->bl.y;
								if (dx < 0) dx++;
								else if (dx > 0) dx--;
								if (dy < 0) dy++;
								else if (dy > 0) dy--;
							} else {
								dx = tbl->x - md->bl.x + rand() % 3 - 1;
								dy = tbl->y - md->bl.y + rand() % 3 - 1;
							}
							ret = mob_walktoxy(md, md->bl.x+dx, md->bl.y+dy, 0);
							i++;
						} while (ret && i < 5);

						if (ret) {
							if (dx < 0) dx = 2;
							else if (dx > 0) dx = -2;
							if (dy < 0) dy = 2;
							else if (dy > 0) dy = -2;
							mob_walktoxy(md, md->bl.x + dx, md->bl.y + dy, 0);
						}
					}
				} else {
					md->state.skillstate = MSS_ATTACK;
					if (md->state.state == MS_WALK)
						mob_stop_walking(md, 1);
					if (md->state.state == MS_ATTACK)
						return 0;
					mob_changestate(md, MS_ATTACK, attack_type);
				}
				return 0;
			} else {
				if (tbl->type != BL_ITEM || (dist = distance(md->bl.x, md->bl.y, tbl->x, tbl->y)) >= md->min_chase || !md->lootitem) {
					mob_unlocktarget(md, tick);
					if (md->state.state == MS_WALK)
						mob_stop_walking(md, 1);
				} else if (dist > 0) {
					if (!(mode & 1)) {
						mob_unlocktarget(md, tick);
						return 0;
					}
					if (!unit_can_move(&md->bl))
						return 0;
					md->state.skillstate = MSS_LOOT;
					mobskill_use(md, tick, -1);
					if (md->timer != -1 && md->state.state != MS_ATTACK && (md->next_walktime < tick || distance(md->to_x, md->to_y, tbl->x, tbl->y) <= 0))
						return 0;
					if (md->next_walktime < tick)
						md->next_walktime = tick + 500;
					if (mob_walktoxy(md, tbl->x, tbl->y, 0))
						mob_unlocktarget(md, tick);
				} else {
					if (md->state.state == MS_ATTACK)
						return 0;
					if (md->state.state == MS_WALK)
						mob_stop_walking(md, 1);
					fitem = (struct flooritem_data *)tbl;
					if (md->lootitem_count < LOOTITEM_SIZE)
						memcpy(&md->lootitem[md->lootitem_count++], &fitem->item_data, sizeof(md->lootitem[0]));
					else if (battle_config.monster_loot_type == 1) {
						mob_unlocktarget(md,tick);
						return 0;
					} else {
						if (md->lootitem[0].card[0] == (short)0xff00)
							intif_delete_petdata(*((long *)(&md->lootitem[0].card[1])));
						for(i = 0; i < LOOTITEM_SIZE - 1; i++)
							memcpy(&md->lootitem[i], &md->lootitem[i + 1], sizeof(md->lootitem[0]));
						memcpy(&md->lootitem[LOOTITEM_SIZE - 1], &fitem->item_data, sizeof(md->lootitem[0]));
					}
					map_clearflooritem(tbl->id);
					mob_unlocktarget(md, tick);
					md->next_walktime -= 1500; // They can restart speedly and that doesn't stop all monsters in same moment when item is loot
				}
				return 0;
			}
		} else {
			mob_unlocktarget(md, tick);
			md->next_walktime -= 2500; // They can restart speedly and that doesn't stop all monsters in same moment when target is lost
			if (md->state.state == MS_WALK)
				mob_stop_walking(md, 4);
			return 0;
		}
	}

	// It is skill use at the time of /standby at the time of a walk
	if (mobskill_use(md, tick, -1))
		return 0;

	if (mode&1 && unit_can_move(&md->bl) &&
	    (md->master_id == 0 || md->state.special_mob_ai || md->master_dist > 10)) { // 0: Nothing, 1: Cannibalize, 2-3: Spheremine

		if (DIFF_TICK(md->next_walktime, tick) > + 7000 &&
		    (md->walkpath.path_len == 0 || md->walkpath.path_pos>=md->walkpath.path_len)) {
			md->next_walktime = tick + 3000 * rand() % 2000;
		}

		// Random movement
		if (mob_randomwalk(md, tick))
			return 0;
	}

	// Since he has finished walking, it stands by
	if (md->walkpath.path_len == 0 || md->walkpath.path_pos >= md->walkpath.path_len)
		md->state.skillstate = MSS_IDLE;

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
static int mob_ai_sub_foreachclient(struct map_session_data *sd, va_list ap) {

	unsigned int tick;

	nullpo_retr(0, ap);

	tick = va_arg(ap, unsigned int);
	map_foreachinarea(mob_ai_sub_hard, sd->bl.m,
	                  sd->bl.x - AREA_SIZE * 2, sd->bl.y - AREA_SIZE * 2,
	                  sd->bl.x + AREA_SIZE * 2, sd->bl.y + AREA_SIZE * 2,
	                  BL_MOB, tick);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
static int mob_ai_hard(int tid, unsigned int tick, int id, int data) {

	clif_foreachclient(mob_ai_sub_foreachclient, tick);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
static int mob_ai_sub_lazy(void * key, void * data, va_list app) {

	struct mob_data *md = data;
	unsigned int tick;
	va_list ap;
	
	nullpo_retr(0, md);
	nullpo_retr(0, app);
	ap = va_arg(app, va_list);
	nullpo_retr(0, ap);

	if (md->bl.type != BL_MOB)
		return 0;

	tick = va_arg(ap, unsigned int);
	
	if (DIFF_TICK(tick, md->last_thinktime) < MIN_MOBTHINKTIME * 10)
		return 0;

	md->last_thinktime = tick;

	if (md->bl.prev == NULL || md->skilltimer != -1) {
		if (DIFF_TICK(tick, md->next_walktime) > MIN_MOBTHINKTIME * 10)
			md->next_walktime = tick;
		return 0;
	}

	if (DIFF_TICK(md->next_walktime, tick) < 0 &&
	    (mob_db[md->class].mode & 1) && unit_can_move(&md->bl)) {

		if (map[md->bl.m].users > 0) {
			if (rand() % 1000 < MOB_LAZYMOVEPERC)
				mob_randomwalk(md, tick);

			else if (rand() % 1000 < MOB_LAZYWARPPERC && md->x0 <= 0 && md->master_id != 0 &&
				mob_db[md->class].mexp <= 0 && !(mob_db[md->class].mode&0x20))
				mob_spawn(md->bl.id);

		} else {
			if (rand() % 1000 < MOB_LAZYWARPPERC && md->x0 <= 0 && md->master_id != 0 &&
			    mob_db[md->class].mexp <= 0 && !(mob_db[md->class].mode & 0x20))
				mob_warp(md,-1,-1,-1,-1);
		}

		md->next_walktime = tick + rand() % 10000 + 5000;
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
static int mob_ai_lazy(int tid, unsigned int tick, int id, int data) {

	map_foreachiddb(mob_ai_sub_lazy, tick);

	return 0;
}


/*==========================================
 * The structure object for item drop with delay
 * Since it is only two being able to pass [ int ] a timer function
 * Data is put in and passed to this structure object
 *------------------------------------------
 */
struct delay_item_drop {
	int m, x, y;
	int nameid, amount;
	struct map_session_data *first_sd, *second_sd, *third_sd;
};

struct delay_item_drop2 {
	int m, x, y;
	struct item item_data;
	struct map_session_data *first_sd, *second_sd, *third_sd;
};

/*==========================================
 *
 *------------------------------------------
 */
static int mob_delay_item_drop(int tid, unsigned int tick, int id, int data) {

	struct delay_item_drop *ditem;
	struct item temp_item;
	int flag, drop_flag = 1;

	nullpo_retr(0, ditem = (struct delay_item_drop *)id);

	memset(&temp_item, 0, sizeof(temp_item));
	temp_item.nameid = ditem->nameid;
	temp_item.amount = ditem->amount;
	temp_item.identify = !itemdb_isequip3(temp_item.nameid);

	if (ditem->first_sd) {
		if ((int)ditem->first_sd->state.autoloot_rate >= data && // Drop rate of items concerned by the autoloot (from 0% to x%)
		    distance(ditem->x, ditem->y, ditem->first_sd->bl.x, ditem->first_sd->bl.y) <= battle_config.item_auto_get_distance) {
			drop_flag = 0;
			if ((flag = pc_additem(ditem->first_sd, &temp_item, ditem->amount))) {
				clif_additem(ditem->first_sd, 0, 0, flag);
				drop_flag = 1;
			}
		}
		if (drop_flag == 1) {
			if (((int)ditem->first_sd->state.displaydrop_rate) >= data) {
				char message[MAX_MSG_LEN + 100]; // Max size of msg_txt + security (char name, char id, etc...) (100)
				sprintf(message, msg_txt(656), ditem->amount, itemdb_exists(ditem->nameid)->jname, (float)data / 100.); // Monster dropped %d %s (drop rate: %%%0.02f).
				clif_displaymessage(ditem->first_sd->fd, message);
			}
		}
	}

	if (drop_flag)
		map_addflooritem(&temp_item, 1, ditem->m, ditem->x, ditem->y, ditem->first_sd, ditem->second_sd, ditem->third_sd, 0, 0);

	FREE(ditem);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
static int mob_delay_item_drop2(int tid, unsigned int tick, int id, int data) {

	struct delay_item_drop2 *ditem;
	int flag, drop_flag = 1;

	nullpo_retr(0, ditem = (struct delay_item_drop2 *)id);

	if (ditem->first_sd){
		if (ditem->first_sd->state.autolootloot_flag && // 0: No autoloot, 1: Autoloot (For looted items)
		    distance(ditem->x, ditem->y, ditem->first_sd->bl.x, ditem->first_sd->bl.y) <= battle_config.item_auto_get_distance) {
			drop_flag = 0;
			if ((flag = pc_additem(ditem->first_sd, &ditem->item_data, ditem->item_data.amount))) {
				clif_additem(ditem->first_sd, 0, 0, flag);
				drop_flag = 1;
			}
		}
		if (drop_flag == 1) {
			if (ditem->first_sd->state.displaylootdrop) {
				char message[MAX_MSG_LEN + 100]; // max size of msg_txt + security (char name, char id, etc...) (100)
				sprintf(message, msg_txt(660), ditem->item_data.amount, itemdb_exists(ditem->item_data.nameid)->jname); // Monster dropped %d %s (it was a loot).
				clif_displaymessage(ditem->first_sd->fd, message);
			}
		}
	}

	if (drop_flag)
		map_addflooritem(&ditem->item_data, ditem->item_data.amount, ditem->m, ditem->x, ditem->y, ditem->first_sd, ditem->second_sd, ditem->third_sd, 0, 0);

	FREE(ditem);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int mob_delete(struct mob_data *md) {

	nullpo_retr(1, md);

	if (md->bl.prev == NULL)
		return 1;

	mob_changestate(md, MS_DEAD, 0);
	clif_clearchar_area(&md->bl, 1);
	if (md->level)
		md->level = 0;
	map_delblock(&md->bl);
	if (mob_get_viewclass(md->class) <= 1000)
		clif_clearchar_delay(gettick_cache + 3000, &md->bl, 0);
	mob_deleteslave(md);
	mob_setdelayspawn(md->bl.id);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
void mob_catch_delete(struct mob_data *md, int type) {

	if (md->bl.prev == NULL)
		return;

	mob_changestate(md, MS_DEAD, 0);
	clif_clearchar_area(&md->bl, type);
	map_delblock(&md->bl);
	mob_setdelayspawn(md->bl.id);

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
int mob_timer_delete(int tid, unsigned int tick, int id, int data) {

	struct block_list *bl = map_id2bl(id);
	struct mob_data *md;

	nullpo_retr(0, bl);

	md = (struct mob_data *)bl;

	md->deletetimer = -1;

	mob_catch_delete(md, 3);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int mob_deleteslave_sub(struct block_list *bl,va_list ap) {

	struct mob_data *md;
	int id;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);
	nullpo_retr(0, md = (struct mob_data *)bl);

	id = va_arg(ap,int);
	if (md->master_id > 0 && md->master_id == id)
		mob_damage(NULL, md, md->hp, 1);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int mob_deleteslave(struct mob_data *md) {

	nullpo_retr(0, md);

	map_foreachinarea(mob_deleteslave_sub, md->bl.m,
	                  0, 0, map[md->bl.m].xs, map[md->bl.m].ys,
	                  BL_MOB, md->bl.id);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int mob_damage(struct block_list *src, struct mob_data *md, int damage, int type) {

	int i, count, minpos, mindmg;
	struct map_session_data *sd = NULL, *tmpsd[DAMAGELOG_SIZE];
	struct {
		struct party *p;
		int id;
		int base_exp, job_exp, zeny;
	} pt[DAMAGELOG_SIZE];
	int pnum;
	int mvp_damage[3], max_hp;
	struct map_session_data *mvp_sd = NULL, *second_sd = NULL,*third_sd = NULL;
	struct block_list *master = NULL;
	double tdmg, temp;
	struct item item;
	int ret;
	int drop_rate;
	int race;

	nullpo_retr(0, md);

	max_hp = status_get_max_hp(&md->bl);

	if (src && src->type == BL_PC) {
		sd = (struct map_session_data *)src;
		mvp_sd = sd;
	}

	if (md->bl.prev == NULL) {
		if (battle_config.error_log)
			printf("mob_damage : BlockError!!\n");
		return 0;
	}

	race = mob_db[md->class].race;

	if (md->state.state == MS_DEAD || md->hp <= 0) {
		if (md->bl.prev != NULL) {
			mob_changestate(md, MS_DEAD, 0);
			mobskill_use(md, gettick_cache, -1); // It is skill at the time of death
			clif_clearchar_area(&md->bl, 1);
			if (md->level)
				md->level = 0;
			map_delblock(&md->bl);
			mob_setdelayspawn(md->bl.id);
		}
		return 0;
	}

	if (md->sc_data[SC_ENDURE].timer == -1)
		mob_stop_walking(md, 3);
	if (damage > max_hp >> 2)
		skill_stop_dancing(&md->bl, 0);

	if (md->hp > max_hp)
		md->hp = max_hp;

	if (damage > md->hp)
		damage = md->hp;

	if (!(type & 2)) {
		if (sd != NULL) {
			for(i = 0, minpos = 0, mindmg = 0x7fffffff; i < DAMAGELOG_SIZE; i++) {
				if (md->dmglog[i].id == sd->status.char_id) // char_id
					break;
				if (md->dmglog[i].id == 0) { // char_id
					minpos = i;
					mindmg = 0;
				}
				else if (md->dmglog[i].dmg < mindmg) {
					minpos = i;
					mindmg = md->dmglog[i].dmg;
				}
			}
			if (i < DAMAGELOG_SIZE)
				md->dmglog[i].dmg += damage;
			else {
				md->dmglog[minpos].id = sd->status.char_id; // char_id
				md->dmglog[minpos].dmg = damage;
			}

			if (md->attacked_id <= 0 && md->state.special_mob_ai == 0) // 0: Nothing, 1: Cannibalize, 2-3: Spheremine
				md->attacked_id = sd->bl.id;
		}
		if (src && src->type == BL_PET && battle_config.pet_attack_exp_to_master == 1) {
			struct pet_data *pd = (struct pet_data *)src;
			nullpo_retr(0, pd);
			for(i = 0, minpos = 0, mindmg = 0x7fffffff; i < DAMAGELOG_SIZE; i++) {
				if (md->dmglog[i].id == pd->msd->status.char_id) // char_id
					break;
				if (md->dmglog[i].id == 0) { // char_id
					minpos = i;
					mindmg = 0;
				} else if (md->dmglog[i].dmg < mindmg) {
					minpos = i;
					mindmg = md->dmglog[i].dmg;
				}
			}
			if (i < DAMAGELOG_SIZE)
				md->dmglog[i].dmg += (damage * battle_config.pet_attack_exp_rate) / 100;
			else {
				md->dmglog[minpos].id = pd->msd->status.char_id; // char_id
				md->dmglog[minpos].dmg = (damage * battle_config.pet_attack_exp_rate) / 100;
			}
		}
		if (src && src->type == BL_MOB && ((struct mob_data*)src)->state.special_mob_ai) { // 0: Nothing, 1: Cannibalize, 2-3: Spheremine
			struct mob_data *md2 = (struct mob_data *)src;
			struct map_session_data *sdmaster;
			nullpo_retr(0, md2);
			sdmaster = map_id2sd(md2->master_id);
			if (sdmaster && sdmaster->state.auth) {
				for(i = 0, minpos = 0, mindmg = 0x7fffffff; i < DAMAGELOG_SIZE; i++) {
					if (md->dmglog[i].id == sdmaster->status.char_id) // char_id
						break;
					if (md->dmglog[i].id == 0) { // char_id
						minpos = i;
						mindmg = 0;
					} else if (md->dmglog[i].dmg < mindmg) {
						minpos = i;
						mindmg = md->dmglog[i].dmg;
					}
				}
				if (i < DAMAGELOG_SIZE)
					md->dmglog[i].dmg += damage;
				else {
					md->dmglog[minpos].id = sdmaster->status.char_id; // char_id
					md->dmglog[minpos].dmg = damage;
			}

			if (md->attacked_id <= 0 && md->state.special_mob_ai == 0) // 0: Nothing, 1: Cannibalize, 2-3: Spheremine
				md->attacked_id = md2->master_id;
			}
		}
	}

	md->hp -= damage;

	if (md->class >= 1285 && md->class <= 1287) {	// Guardian HP update
		struct guild_castle *gc = guild_mapname2gc(map[md->bl.m].name);
		if (gc) {
			if (md->bl.id == gc->GID0) {
				gc->Ghp0 = md->hp;
				if (gc->Ghp0 <= 0) {
					guild_castledatasave(gc->castle_id, 10, 0);
					guild_castledatasave(gc->castle_id, 18, 0);
				}
			} else if (md->bl.id == gc->GID1) {
				gc->Ghp1 = md->hp;
				if (gc->Ghp1 <= 0) {
					guild_castledatasave(gc->castle_id, 11, 0);
					guild_castledatasave(gc->castle_id, 19, 0);
				}
			} else if (md->bl.id == gc->GID2) {
				gc->Ghp2 = md->hp;
				if (gc->Ghp2 <= 0) {
					guild_castledatasave(gc->castle_id, 12, 0);
					guild_castledatasave(gc->castle_id, 20, 0);
				}
			} else if (md->bl.id == gc->GID3) {
				gc->Ghp3 = md->hp;
				if (gc->Ghp3 <= 0) {
					guild_castledatasave(gc->castle_id, 13, 0);
					guild_castledatasave(gc->castle_id, 21, 0);
				}
			} else if (md->bl.id == gc->GID4) {
				gc->Ghp4 = md->hp;
				if (gc->Ghp4 <= 0) {
					guild_castledatasave(gc->castle_id, 14, 0);
					guild_castledatasave(gc->castle_id, 22, 0);
				}
			} else if (md->bl.id == gc->GID5) {
				gc->Ghp5 = md->hp;
				if (gc->Ghp5 <= 0) {
					guild_castledatasave(gc->castle_id, 15, 0);
					guild_castledatasave(gc->castle_id, 23, 0);
				}
			} else if (md->bl.id == gc->GID6) {
				gc->Ghp6 = md->hp;
				if (gc->Ghp6 <= 0) {
					guild_castledatasave(gc->castle_id, 16, 0);
					guild_castledatasave(gc->castle_id, 24, 0);
				}
			} else if (md->bl.id == gc->GID7) {
				gc->Ghp7 = md->hp;
				if (gc->Ghp7 <= 0) {
					guild_castledatasave(gc->castle_id, 17, 0);
					guild_castledatasave(gc->castle_id, 25, 0);
				}
			}
		}
	}

	if (md->option & 2)
		status_change_end(&md->bl, SC_HIDING, -1);
	if (md->option & 4)
		status_change_end(&md->bl, SC_CLOAKING, -1);

	if (md->state.special_mob_ai == 2) { // 0: Nothing, 1: Cannibalize, 2-3: Spheremine
		int skillidx;
		if ((skillidx = mob_skillid2skillidx(md->class, NPC_SELFDESTRUCTION)) >= 0) {
			md->mode |= 0x1;
			md->next_walktime = gettick_cache;
			mobskill_use_id(md, &md->bl, skillidx);
			md->state.special_mob_ai++; // 0: Nothing, 1: Cannibalize, 2-3: Spheremine
		}
		if (src && md->master_id == src->id)
			md->target_dir = map_calc_dir(src, md->bl.x, md->bl.y) + 1;
	}

	if (md->hp > 0) {
		if (battle_config.show_mob_hp)
			clif_update_mobhp(md);
		return 0;
	}

	map_freeblock_lock();
	mob_changestate(md, MS_DEAD, 0);
	mobskill_use(md, gettick_cache, -1);

	if (src && src->type == BL_MOB)
		mob_unlocktarget((struct mob_data *)src, gettick_cache);

	// Taekwon Mission
	if (sd && sd->status.class == JOB_TAEKWON && md->class == sd->tk_mission_target_id) {
		sd->tk_mission_count++;
		pc_setglobalreg(sd, "TK_MISSION_COUNT", sd->tk_mission_count);

		if (sd->tk_mission_count >= 100) {
			ranking_gain_point(sd, RK_TAEKWON, 1);

			do {
				sd->tk_mission_target_id = rand() % MAX_MOB_DB;
			} while(sd->tk_mission_target_id == 1182 || sd->tk_mission_target_id == 1183 || sd->tk_mission_target_id == 1184 || mob_db[sd->tk_mission_target_id].max_hp <= 0 || mob_db[sd->tk_mission_target_id].summonper[0] == 0 || mob_db[sd->tk_mission_target_id].mode & 0x20);

			sd->tk_mission_count = 0;

			pc_setglobalreg(sd, "TK_MISSION_ID", sd->tk_mission_target_id);

			clif_mission_mob(sd, sd->tk_mission_target_id, 0);
		}
	}

	if (sd) {
		int sp = 0, hp = 0;
		if (sd->state.attack_type == BF_MAGIC && sd->skilltarget == md->bl.id && (i = pc_checkskill(sd, HW_SOULDRAIN)) > 0) {
			clif_skill_nodamage(src, &md->bl, HW_SOULDRAIN, i, 1);
			sp += (status_get_lv(&md->bl)) * (65 + 15 * i) / 100;
		}
		sp += sd->sp_gain_value;
		sp += sd->sp_gain_race[race];
		hp += sd->hp_gain_value;
		if (sp > 0) {
			if (sd->status.sp + sp > sd->status.max_sp)
				sp = sd->status.max_sp - sd->status.sp;
			sd->status.sp += sp;
			clif_heal(sd->fd, SP_SP, sp);
		}
		if (hp > 0) {
			if (sd->status.hp + hp > sd->status.max_hp)
				hp = sd->status.max_hp - sd->status.hp;
			sd->status.hp += hp;
			clif_heal(sd->fd, SP_HP, hp);
		}
	}

	tdmg = 0.;
	count = 0;
	memset(tmpsd, 0, sizeof(tmpsd));
	memset(mvp_damage, 0, sizeof(mvp_damage));
	for(i = 0; i < DAMAGELOG_SIZE; i++) {
		if (md->dmglog[i].id == 0) // char_id
			continue;
		tdmg += (double)md->dmglog[i].dmg; // Exploit fix: Add all damages done by all, if a player damages a monster and a novice arrives after, tmdg must not give to him all xp if first is disconnected
		tmpsd[i] = map_charid2sd(md->dmglog[i].id); // char_id
		if (mvp_damage[0] < md->dmglog[i].dmg) { // Don't give MvP exp if player is disconnected -> conserv mvp_sd if NULL
			if (mvp_sd == NULL) { // Even if tmpsd[i] is NULL, we save it to avoid that mvp exp will be given to a little player
				mvp_sd = tmpsd[i];
				mvp_damage[0] = md->dmglog[i].dmg;
			} else {
				third_sd = second_sd;
				second_sd = mvp_sd;
				mvp_sd = tmpsd[i];
				mvp_damage[2] = mvp_damage[1];
				mvp_damage[1] = mvp_damage[0];
				mvp_damage[0] = md->dmglog[i].dmg;
			}
		} else if (mvp_damage[1] < md->dmglog[i].dmg) {
			third_sd = second_sd;
			second_sd = tmpsd[i];
			mvp_damage[2] = mvp_damage[1];
			mvp_damage[1] = md->dmglog[i].dmg;
		} else if (mvp_damage[2] < md->dmglog[i].dmg) {
			third_sd = tmpsd[i];
			mvp_damage[2] = md->dmglog[i].dmg;
		}
		if (tmpsd[i] == NULL)
			continue;
		if (tmpsd[i]->bl.m != md->bl.m || pc_isdead(tmpsd[i]))
			continue;
		count++;
	}

	if (tdmg < (double)max_hp)
		tdmg = (double)max_hp;

	memset(pt, 0, sizeof(pt));

	if (map[md->bl.m].flag.pvp == 0 || battle_config.pvp_exp == 1) {

		if (md->master_id)
			master = map_id2bl(md->master_id);

		pnum = 0;
		for(i = 0; i < DAMAGELOG_SIZE; i++) {
			int pid;
			char flag;
			double per, base_exp, job_exp, zeny = 0.;
			struct party *p;
			if (tmpsd[i] == NULL || tmpsd[i]->bl.m != md->bl.m || pc_isdead(tmpsd[i]))
				continue;
			per = ((double)md->dmglog[i].dmg) * ((double)(9 + ((count > 6) ? 6 : count))) / 10. / tdmg;

			if (sd && sd->expaddrace[race])
				per += per*sd->expaddrace[race]/100;

			if (per > 1.5)
				per = 1.5;
			else if (per < 0.)
				per = 0.;
			if (mob_db[md->class].base_exp > 0) {
				base_exp = ((double)mob_db[md->class].base_exp) * per + .5;
				if (base_exp < 1.)
					base_exp = 1.;
			} else
				base_exp = 0.;
			if (mob_db[md->class].job_exp > 0) {
				job_exp = ((double)mob_db[md->class].job_exp) * per + .5;
				if (job_exp < 1.)
					job_exp = 1.;
			} else
				job_exp = 0.;

			if (battle_config.pk_mode && sd && (mob_db[md->class].lv - sd->status.base_level >= 20)) {
				job_exp *= 1.15; // pk_mode additional exp if monster >= 20 levels
				base_exp *= 1.15; // pk_mode additional exp if monster >= 20 levels
			}

			if (md->master_id &&
			    ((master && status_get_mode(master) & 0x20) || // Check if its master is a Boss
			     (md->state.special_mob_ai >= 1 && battle_config.alchemist_summon_reward != 1))) { // For summoned creatures // 0: Nothing, 1: Cannibalize, 2-3: Spheremine
				base_exp = 0;
				job_exp = 0;
			} else {
				if (battle_config.zeny_from_mobs) {
					if (md->level > 0)
						zeny = ((double)(md->level + rand() % md->level)) * per + .5; // Zeny calculation moblv + random moblv
					if (mob_db[md->class].mexp > 0)
						zeny *= (rand() % 250);
				}
			}

			// Mapflags: noexp check
			if (map[md->bl.m].flag.nobaseexp == 1)
				base_exp = 0;
			if (map[md->bl.m].flag.nojobexp == 1)
				job_exp = 0;

			flag = 1;
			if ((pid = tmpsd[i]->status.party_id) > 0) {
				int j;
				for(j = 0; j < pnum; j++)
					if (pt[j].id == pid)
						break;
				if (j == pnum) {
					if ((p = party_search(pid)) != NULL && p->exp != 0) {
						pt[pnum].id = pid;
						pt[pnum].p = p;
						pt[pnum].base_exp = (int)base_exp;
						pt[pnum].job_exp = (int)job_exp;
						if (battle_config.zeny_from_mobs)
							pt[pnum].zeny = (int)zeny; // Zeny share
						pnum++;
						flag = 0;
					}
				} else {
					pt[j].base_exp += (int)base_exp;
					pt[j].job_exp += (int)job_exp;
					if (battle_config.zeny_from_mobs)
						pt[j].zeny += (int)zeny; // Zeny share
					flag = 0;
				}
			}
			if (flag) {
				if (base_exp > 0 || job_exp > 0)
					pc_gainexp(tmpsd[i], base_exp, job_exp);
				if (battle_config.zeny_from_mobs && zeny > 0) {
					pc_getzeny(tmpsd[i], zeny); // Zeny from mobs
				}
			}
		}

		for(i = 0; i < pnum; i++)
			party_exp_share(pt[i].p, md->bl.m, pt[i].base_exp, pt[i].job_exp, pt[i].zeny);

		// Item drop
		if (!(type & 1)) {
			// Mapflag: nodrop check
			if (!map[md->bl.m].flag.nomobdrop) {
				for(i = 0; i < 10; i++) {
					struct delay_item_drop *ditem;
					double temp_rate, rate;
					int ratemin, ratemax;
					if (md->master_id &&
					    ((master && status_get_mode(master) & 0x20) || // Check if its master is a Boss
					     (md->state.special_mob_ai >= 1 && battle_config.alchemist_summon_reward != 1))) // 0: Nothing, 1: Cannibalize, 2-3: Spheremine
						break;
					if (mob_db[md->class].dropitem[i].nameid <= 0)
						continue;
					// Get value for calculation of drop rate
					switch (itemdb_type(mob_db[md->class].dropitem[i].nameid)) {
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
					case 6:
						rate = (double)battle_config.item_rate_card;
						ratemin = battle_config.item_drop_card_min;
						ratemax = battle_config.item_drop_card_max;
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
					// Check for special bonus
					if (battle_config.drops_by_luk > 0 && sd) // Drops affected by luk
						drop_rate += (sd->status.luk * battle_config.drops_by_luk) / 100;
					if (battle_config.pk_mode && sd && (mob_db[md->class].lv - sd->status.base_level) >= 20) // pk_mode increase drops if 20 level difference
						drop_rate = drop_rate * 5 / 4; // *1.25
					if (drop_rate > 10000)
						drop_rate = 10000;
					else if (drop_rate <= 0 && !battle_config.drop_rate0item)
						drop_rate = 1;
					// Check drop success
					if (drop_rate <= rand() % 10000)
						continue;

					CALLOC(ditem, struct delay_item_drop, 1);
					ditem->nameid = mob_db[md->class].dropitem[i].nameid;
					ditem->amount = 1;
					ditem->m = md->bl.m;
					ditem->x = md->bl.x;
					ditem->y = md->bl.y;
					if (mvp_sd != NULL) {
						ditem->first_sd = mvp_sd;
						if (second_sd != NULL) {
							ditem->second_sd = second_sd;
							ditem->third_sd = third_sd;
						} else {
							ditem->second_sd = third_sd;
							ditem->third_sd = NULL;
						}
					} else if (second_sd != NULL) {
						ditem->first_sd = second_sd;
						ditem->second_sd = third_sd;
						ditem->third_sd = NULL;
					} else {
						ditem->first_sd = third_sd;
						ditem->second_sd = NULL;
						ditem->third_sd = NULL;
					}
					add_timer(gettick_cache + 500 + i, mob_delay_item_drop, (int)ditem, drop_rate); // data = drop rate (loot = 1000000000)
				}

				// Ore Discovery
				if (sd == mvp_sd && pc_checkskill(sd, BS_FINDINGORE) > 0 && battle_config.finding_ore_rate >= rand() % 10000) { // battle_config.finding_ore_rate / 100 >= rand() % 100
					struct delay_item_drop *ditem;
					int itemid[17] = { 714, 756, 757, 969, 984, 985, 990, 991, 992, 993, 994, 995, 996, 997, 998, 999, 1002 };
					CALLOC(ditem, struct delay_item_drop, 1);
					ditem->nameid = itemid[rand() % 17];
					ditem->amount = 1;
					ditem->m = md->bl.m;
					ditem->x = md->bl.x;
					ditem->y = md->bl.y;
					if (mvp_sd != NULL) {
						ditem->first_sd = mvp_sd;
						if (second_sd != NULL) {
							ditem->second_sd = second_sd;
							ditem->third_sd = third_sd;
						} else {
							ditem->second_sd = third_sd;
							ditem->third_sd = NULL;
						}
					} else if (second_sd != NULL) {
						ditem->first_sd = second_sd;
						ditem->second_sd = third_sd;
						ditem->third_sd = NULL;
					} else {
						ditem->first_sd = third_sd;
						ditem->second_sd = NULL;
						ditem->third_sd = NULL;
					}
					add_timer(gettick_cache + 500 + i, mob_delay_item_drop, (int)ditem, battle_config.finding_ore_rate); // data = drop rate (loot = 1000000000)
				}
			}

			if (sd) {
				for(i = 0; i < sd->monster_drop_item_count && i < MAX_PC_BONUS; i++) {
					struct delay_item_drop *ditem;
					int race = status_get_race(&md->bl);
					if (sd->monster_drop[i].id < 0)
						continue;
					if (sd->monster_drop[i].race & (1 << race) ||
					    (mob_db[md->class].mode & 0x20 && sd->monster_drop[i].race & 1 << 10) ||
					    (!(mob_db[md->class].mode & 0x20) && sd->monster_drop[i].race & 1 << 11)) {
						if (sd->monster_drop[i].rate <= rand() % 10000)
							continue;
						CALLOC(ditem, struct delay_item_drop, 1);

						if (sd->monster_drop[i].id > 0)
							ditem->nameid = sd->monster_drop[i].id;
						else
							ditem->nameid = itemdb_searchrandomgroup(sd->monster_drop[i].group);
						ditem->amount = 1;
						ditem->m = md->bl.m;
						ditem->x = md->bl.x;
						ditem->y = md->bl.y;
						if (mvp_sd != NULL) {
							ditem->first_sd = mvp_sd;
							if (second_sd != NULL) {
								ditem->second_sd = second_sd;
								ditem->third_sd = third_sd;
							} else {
								ditem->second_sd = third_sd;
								ditem->third_sd = NULL;
							}
						} else if (second_sd != NULL) {
							ditem->first_sd = second_sd;
							ditem->second_sd = third_sd;
							ditem->third_sd = NULL;
						} else {
							ditem->first_sd = third_sd;
							ditem->second_sd = NULL;
							ditem->third_sd = NULL;
						}
						add_timer(gettick_cache + 520 + i, mob_delay_item_drop, (int)ditem, sd->monster_drop[i].rate); // data = drop rate (loot = 1000000000)
					}
				}
				if (sd->get_zeny_num && rand()%100 < sd->get_zeny_rate)
					pc_getzeny(sd, rand() % (sd->get_zeny_num + 1));
			}

			if (md->lootitem) {
				for(i = 0; i < md->lootitem_count; i++) {
					struct delay_item_drop2 *ditem;
					CALLOC(ditem, struct delay_item_drop2, 1);
					memcpy(&ditem->item_data, &md->lootitem[i], sizeof(md->lootitem[0]));
					ditem->m = md->bl.m;
					ditem->x = md->bl.x;
					ditem->y = md->bl.y;
					if (mvp_sd != NULL) {
						ditem->first_sd = mvp_sd;
						if (second_sd != NULL) {
							ditem->second_sd = second_sd;
							ditem->third_sd = third_sd;
						} else {
							ditem->second_sd = third_sd;
							ditem->third_sd = NULL;
						}
					} else if (second_sd != NULL) {
						ditem->first_sd = second_sd;
						ditem->second_sd = third_sd;
						ditem->third_sd = NULL;
					} else {
						ditem->first_sd = third_sd;
						ditem->second_sd = NULL;
						ditem->third_sd = NULL;
					}
					add_timer(gettick_cache + 540 + i, mob_delay_item_drop2, (int)ditem, 1000000000); // data = drop rate (loot = 1000000000)
				}
			}
		}

		if (mvp_sd && mob_db[md->class].mexp > 0 &&
		// If player who have do max damages is not here, remove all mvp experience
		    mvp_sd->bl.m == md->bl.m && !pc_isdead(mvp_sd)) {
			int mexp;
			int j;
			temp = ((double)mob_db[md->class].mexp * (9.+(double)count)/10.);
			mexp = (temp > 2147483647. || temp < 0) ? 0x7fffffff : (int)temp;
			if (mexp < 1) mexp = 1;
			// mapflag: noexp check
			if (map[md->bl.m].flag.nobaseexp == 1 || map[md->bl.m].flag.nojobexp == 1)
				mexp = 1;
			clif_mvp_effect(mvp_sd);
			clif_mvp_exp(mvp_sd, mexp);
			pc_gainexp(mvp_sd, mexp, 0);

			// MvP item drops
			if (!map[md->bl.m].flag.nomvpdrop)
			{
				for(j = 0; j < 3; j++)
				{
					double temp_rate;

					i = rand() % 3;

					// Skip invalid items
					if (mob_db[md->class].mvpitem[i].nameid <= 0)
						continue;

					// Calculate drop rate
					if (mob_db[md->class].mvpitem[i].p <= 0)
						drop_rate = 0;
					else {
						// Item type specific droprate modifier
						switch(itemdb_type(mob_db[md->class].mvpitem[i].nameid))
						{
							case 0:		// Healing items
								temp_rate = ((double)mob_db[md->class].mvpitem[i].p) * ((double)battle_config.mvp_healing_rate) / (double)100;
								break;
							case 2:		// Usable items
								temp_rate = ((double)mob_db[md->class].mvpitem[i].p) * ((double)battle_config.mvp_usable_rate) / (double)100;
								break;
							case 4:
							case 5:
							case 8: 	// Equipable items
								temp_rate = ((double)mob_db[md->class].mvpitem[i].p) * ((double)battle_config.mvp_equipable_rate) / (double)100;
								break;
							case 6:		// Cards
								temp_rate = ((double)mob_db[md->class].mvpitem[i].p) * ((double)battle_config.mvp_card_rate) / (double)100;
								break;
							default:	// Common items
								temp_rate = ((double)mob_db[md->class].mvpitem[i].p) * ((double)battle_config.mvp_common_rate) / (double)100;
								break;
						}
						if (temp_rate > 10000. || temp_rate < 0 || ((int)temp_rate) > 10000)
							drop_rate = 10000;
						else
							drop_rate = (int)temp_rate;
					}

					// Check minimum/maximum with configuration
					if (drop_rate < battle_config.item_drop_mvp_min)
						drop_rate = battle_config.item_drop_mvp_min;
					else if (drop_rate > battle_config.item_drop_mvp_max)
						drop_rate = battle_config.item_drop_mvp_max;
					if (drop_rate <= 0 && !battle_config.drop_rate0item)
						drop_rate = 1;
					// Check drop success
					if (drop_rate <= rand() % 10000)
						continue;
					memset(&item, 0, sizeof(item));
					item.nameid = mob_db[md->class].mvpitem[i].nameid;
					item.identify = !itemdb_isequip3(item.nameid);
					clif_mvp_item(mvp_sd, item.nameid);
					if (mvp_sd->weight * 2 > mvp_sd->max_weight)
						map_addflooritem(&item, 1, mvp_sd->bl.m, mvp_sd->bl.x, mvp_sd->bl.y, mvp_sd, second_sd, third_sd, mvp_sd->bl.id, 1);
					else if ((ret = pc_additem(mvp_sd, &item, 1))) {
						clif_additem(sd, 0, 0, ret);
						map_addflooritem(&item, 1, mvp_sd->bl.m, mvp_sd->bl.x, mvp_sd->bl.y, mvp_sd, second_sd, third_sd, mvp_sd->bl.id, 1);
					}
					break;
				}
			}
		}

	}

	// <Agit> NPC Event [OnAgitBreak]
	if (md->npc_event[0] && strcmp(((md->npc_event) + strlen(md->npc_event) - 13), "::OnAgitBreak") == 0) {
		printf("MOB.C: Run NPC_Event[OnAgitBreak].\n");
		if (agit_flag == 1) // Call to Run NPC_Event[OnAgitBreak]
			guild_agit_break(md);
	}

	if (md->npc_event[0]) {
		// if(battle_config.battle_log)
		//	printf("mob_damage : run event : %s\n", md->npc_event);
		if (src && src->type == BL_PET)
			sd = ((struct pet_data *)src)->msd;
		if (sd == NULL) {
			if (mvp_sd != NULL)
				sd = mvp_sd;
			else if (second_sd != NULL)
				sd = second_sd;
			else if (third_sd != NULL)
				sd = third_sd;
			else {
				struct map_session_data *tmp_sd;
				for(i = 0; i < fd_max; i++) {
					if (session[i] && (tmp_sd = session[i]->session_data) && tmp_sd->state.auth) {
						if (md->bl.m == tmp_sd->bl.m) {
							sd = tmp_sd;
							break;
						}
					}
				}
			}
		}
		if (sd)
			npc_event(sd, md->npc_event, 0);
	}

	clif_clearchar_area(&md->bl, 1);
	if (md->level)
		md->level = 0;
	map_delblock(&md->bl);
	if (mob_get_viewclass(md->class) <= 1000)
		clif_clearchar_delay(gettick_cache + 3000, &md->bl, 0);
	mob_deleteslave(md);
	mob_setdelayspawn(md->bl.id);
	map_freeblock_unlock();

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int mob_class_change(struct mob_data *md, int *value, int count) {

	unsigned int skill_delay;
	int i, hp_rate, max_hp, class_ = 0;

	nullpo_retr(0, md);
	nullpo_retr(0, value);

	if (md->bl.prev == NULL) return 0;
	if (count < 1) return 0;

	if (md->class == class_) return 0; // If the previous class is the same
	if (md->class >= 1285 && md->class <= 1288) return 0; // Emperium and Guardians should be ignored
	if (md->class >= 1324 && md->class <= 1363) return 0; // Treasure Chests also invalid

	i = rand() % count;
	if (value[i] && value[i] > 1000 && value[i] < MAX_MOB_DB)
		class_ = value[i];
	else
		return 0;

	max_hp = status_get_max_hp(&md->bl);
	hp_rate = md->hp * 100 / max_hp;
	clif_mob_class_change(md, class_);
	md->class = class_;
	max_hp = status_get_max_hp(&md->bl);
	if (battle_config.monster_class_change_full_recover == 1) {
		md->hp = max_hp;
		memset(md->dmglog, 0, sizeof(md->dmglog));
	} else
		md->hp = max_hp * hp_rate / 100;
	if (md->hp > max_hp) md->hp = max_hp;
	else if (md->hp < 1) md->hp = 1;

	memset(md->name, 0, sizeof(md->name));
	strncpy(md->name, mob_db[class_].jname, 24);
	memset(&md->state,0,sizeof(md->state));
	md->attacked_id = 0;
	md->target_id = 0;
	md->move_fail_count = 0;

	md->speed = mob_db[md->class].speed;
	md->def_ele = mob_db[md->class].element;

	mob_changestate(md,MS_IDLE, 0);
	skill_castcancel(&md->bl, 0, 0);
	md->state.skillstate = MSS_IDLE;
	md->last_thinktime = gettick_cache;
	md->next_walktime = gettick_cache + rand() % 50 + 5000;
	md->attackabletime = gettick_cache;
	md->canmove_tick = gettick_cache;

	skill_delay = gettick_cache - 1000 * 3600 * 10;
	for(i = 0; i < MAX_MOBSKILL; i++)
		md->skilldelay[i] = skill_delay;
	md->skillid = 0;
	md->skilllv = 0;

	if (md->lootitem == NULL && mob_db[class_].mode & 0x02) {
		CALLOC(md->lootitem, struct item, LOOTITEM_SIZE);
	}

	skill_clear_unitgroup(&md->bl);
	skill_cleartimerskill(&md->bl);

	clif_clearchar_area(&md->bl, 0);
	clif_spawnmob(md);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int mob_heal(struct mob_data *md, int heal) {

	int max_hp;

	nullpo_retr(0, md);

	max_hp = status_get_max_hp(&md->bl);

	md->hp += heal;
	if (md->class == 1288) {
		struct guild_castle *gc = guild_mapname2gc(map[md->bl.m].name);
		if (gc) {
			if (mob_db[1288].max_hp + 2000 * gc->defense < md->hp)
				md->hp = mob_db[1288].max_hp + 2000 * gc->defense;
		} else if (max_hp < md->hp)
			md->hp = max_hp;
	} else if (max_hp < md->hp)
		md->hp = max_hp;

	if (md->class >= 1285 && md->class <= 1287) {
		struct guild_castle *gc = guild_mapname2gc(map[md->bl.m].name);
		if (gc) {
			if      (md->bl.id == gc->GID0) {
				if (15670 + 2000 * gc->defense < md->hp)
					md->hp = 15670 + 2000 * gc->defense;
				gc->Ghp0 = md->hp;
			} else if (md->bl.id == gc->GID1) {
				if (15670 + 2000 * gc->defense < md->hp)
					md->hp = 15670 + 2000 * gc->defense;
				gc->Ghp1 = md->hp;
			} else if (md->bl.id == gc->GID2) {
				if (15670 + 2000 * gc->defense < md->hp)
					md->hp = 15670 + 2000 * gc->defense;
				gc->Ghp2 = md->hp;
			} else if (md->bl.id == gc->GID3) {
				if (30214 + 2000 * gc->defense < md->hp)
					md->hp = 30214 + 2000 * gc->defense;
				gc->Ghp3 = md->hp;
			} else if (md->bl.id == gc->GID4) {
				if (30214 + 2000 * gc->defense < md->hp)
					md->hp = 30214 + 2000 * gc->defense;
				gc->Ghp4 = md->hp;
			} else if (md->bl.id == gc->GID5) {
				if (28634 + 2000 * gc->defense < md->hp)
					md->hp = 28634 + 2000 * gc->defense;
				gc->Ghp5 = md->hp;
			} else if (md->bl.id == gc->GID6) {
				if (28634 + 2000 * gc->defense < md->hp)
					md->hp = 28634 + 2000 * gc->defense;
				gc->Ghp6 = md->hp;
			} else if (md->bl.id == gc->GID7) {
				if (28634 + 2000 * gc->defense < md->hp)
					md->hp = 28634 + 2000 * gc->defense;
				gc->Ghp7 = md->hp;
			}
		}
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int mob_warpslave_sub(struct block_list *bl, va_list ap) {

	struct mob_data *md = (struct mob_data *)bl;
	int x, y, id, tid;

	id = va_arg(ap, int);
	x = va_arg(ap, int);
	y = va_arg(ap, int);

	if (md->master_id == id) {
		tid = md->target_id;
		mob_warp(md, -1, x, y, 2);
		md->target_id = tid;
		mob_attack(md, 0, 0);
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int mob_warpslave(struct mob_data *md, int x, int y) {

	map_foreachinarea(mob_warpslave_sub, md->bl.m,
	                  0, 0,
	                  map[md->bl.m].xs - 1, map[md->bl.m].ys - 1, BL_MOB,
	                  md->bl.id, md->bl.x, md->bl.y);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int mob_warp(struct mob_data *md, int m, int x, int y, int type) {

	int i, xs = 0, ys = 0, bx = x, by = y;

	nullpo_retr(0, md);

	if (md->bl.prev == NULL)
		return 0;

	if (m < 0) m = md->bl.m;

	if (type >= 0) {
		if (map[md->bl.m].flag.monster_noteleport)
			return 0;
		clif_clearchar_area(&md->bl, type);
	}
	skill_unit_move(&md->bl, gettick_cache, 0);
	map_delblock(&md->bl);

	if (bx > 0 && by > 0) {
		xs = 9;
		ys = 9;
	}

	i = 0;
	while(map_getcell(m, x, y, CELL_CHKNOPASS_NPC) && i < 1000) {
		if (xs > 0 && ys > 0 && i < 250) {
			x = bx + rand() % xs - xs / 2;
			y = by + rand() % ys - ys / 2;
		} else {
			x = rand() % (map[m].xs - 2) + 1;
			y = rand() % (map[m].ys - 2) + 1;
		}
		i++;
	}
	md->dir = 0;
	if (i < 1000) {
		md->bl.x = md->to_x = x;
		md->bl.y = md->to_y = y;
		md->bl.m = m;
	} else {
		m = md->bl.m;
		if (battle_config.error_log == 1)
			printf("MOB %d warp failed, class = %d\n", md->bl.id, md->class);
	}

	md->target_id = 0;
	md->state.targettype = NONE_ATTACKABLE;
	md->attacked_id = 0;
	md->state.skillstate = MSS_IDLE;
	mob_changestate(md, MS_IDLE, 0);

	map_addblock(&md->bl);
	skill_unit_move(&md->bl, gettick_cache, 1);
	if (type > 0) {
		clif_spawnmob(md);
		mob_warpslave(md, md->bl.x, md->bl.y);
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int mob_countslave_sub(struct block_list *bl,va_list ap) {

	int id,*c;
	struct mob_data *md;

	id=va_arg(ap,int);

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);
	nullpo_retr(0, c=va_arg(ap,int *));
	nullpo_retr(0, md = (struct mob_data *)bl);

	if (md->master_id == id)
		(*c)++;

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int mob_countslave(struct mob_data *md) {

	int c = 0;

	nullpo_retr(0, md);

	map_foreachinarea(mob_countslave_sub, md->bl.m,
	                  0, 0, map[md->bl.m].xs - 1, map[md->bl.m].ys - 1,
	                  BL_MOB, md->bl.id, &c);

	return c;
}

/*==========================================
 *
 *------------------------------------------
 */
int mob_summonslave(struct mob_data *md2, int *value, int amount, int flag) {

	struct mob_data *md;
	int bx,by,m,count = 0, class, k, a;
	char monster_name[25];
	char * name_pos;

	nullpo_retr(0, md2);
	nullpo_retr(0, value);

	bx = md2->bl.x;
	by = md2->bl.y;
	m = md2->bl.m;

	if (value[0] <= 1000 || value[0] >= MAX_MOB_DB)
		return 0;
	while(count < 21 && value[count] > 1000 && value[count] < MAX_MOB_DB)
		count++;
	if (count < 1) return 0;

	for(k = 0; k < count; k++) {
		class = value[k];
		if (class <= 1000 || class >= MAX_MOB_DB)
			continue;
		for(a = amount; a > 0; a--) {
			int x, y, i;
			CALLOC(md, struct mob_data, 1);
			if (mob_db[class].mode & 0x02) {
				CALLOC(md->lootitem, struct item, LOOTITEM_SIZE);
			} else
				md->lootitem = NULL;

			i = 0;
			do {
				x = rand() % 9 - 4 + bx;
				y = rand() % 9 - 4 + by;
				i++;
			} while(map_getcell(m, x, y, CELL_CHKNOPASS_NPC) && i < 100);
			if (i >= 100) {
				x = bx;
				y = by;
			}

			memset(monster_name, 0, sizeof(monster_name));
			// If leader name is included in follower name, change leader name to display name
			if ((name_pos = strstr(mob_db[class].jname, mob_db[md2->class].jname)) != NULL) {
				int count2;
				count2 = 0;
				if (name_pos != mob_db[class].jname) {
					count2 = name_pos - mob_db[class].jname;
					strncpy(monster_name, mob_db[class].jname, count2);
				}
				if (24 - count2 > 0) {
					strncpy(monster_name + count2, md2->name, 24 - count2);
					count2 += ((int)strlen(md2->name) < 24 - count2) ? (int)strlen(md2->name) : 24 - count2;
				}
				if (24 - count2 > 0) {
					strncpy(monster_name + count2, mob_db[class].jname + (name_pos - mob_db[class].jname + strlen(mob_db[md2->class].jname)), 24 - count2);
				}
			} else
				strncpy(monster_name, mob_db[class].jname, 24);

			mob_spawn_dataset(md, monster_name, class);
			md->bl.m = m;
			md->bl.x = x;
			md->bl.y = y;

			md->m = m;
			md->x0 = x;
			md->y0 = y;
			md->xs = 0;
			md->ys = 0;
			md->speed = md2->speed;
			md->spawndelay1 = -1;
			md->spawndelay2 = -1;
			md->size = md2->size; // Slaves have same size as leader

			memset(md->npc_event,0,sizeof(md->npc_event));
			md->bl.type = BL_MOB;
			map_addiddb(&md->bl);
			mob_spawn(md->bl.id);
			clif_skill_nodamage(&md->bl, &md->bl, (flag) ? NPC_SUMMONSLAVE : NPC_SUMMONMONSTER, amount, 1); // Amount ???

			if (flag)
				md->master_id = md2->bl.id;
		}
	}

	return 0;
}

/*==========================================
 * mob_counttargeted_sub Function
 *------------------------------------------
 */
static int mob_counttargeted_sub(struct block_list *bl,va_list ap) {

	int id,*c,target_lv;
	struct block_list *src;

	id=va_arg(ap,int);
	nullpo_retr(0, bl);
	nullpo_retr(0, ap);
	nullpo_retr(0, c=va_arg(ap,int *));

	src=va_arg(ap,struct block_list *);
	target_lv=va_arg(ap,int);
	if (id == bl->id || (src && id == src->id)) return 0;
	if (bl->type == BL_PC) {
		struct map_session_data *sd = (struct map_session_data *)bl;
		if (sd && sd->attacktarget == id && sd->attacktimer != -1 && sd->attacktarget_lv >= target_lv)
			(*c)++;
	} else if (bl->type == BL_MOB) {
		struct mob_data *md = (struct mob_data *)bl;
		if (md && md->target_id == id && md->timer != -1 && md->state.state == MS_ATTACK && md->target_lv >= target_lv)
			(*c)++;
	} else if (bl->type == BL_PET) {
		struct pet_data *pd = (struct pet_data *)bl;
		if (pd->target_id == id && pd->timer != -1 && pd->state.state == MS_ATTACK && pd->target_lv >= target_lv)
			(*c)++;
	}

	return 0;
}

/*==========================================
 * mob_counttargeted
 *------------------------------------------
 */
int mob_counttargeted(struct mob_data *md, struct block_list *src, int target_lv) {

	int c = 0;

	map_foreachinarea(mob_counttargeted_sub, md->bl.m,
	                  md->bl.x - AREA_SIZE, md->bl.y - AREA_SIZE,
	                  md->bl.x + AREA_SIZE, md->bl.y + AREA_SIZE, 0, md->bl.id, &c, src, target_lv);

	return c;
}

/*==========================================
 *
 *------------------------------------------
 */
int mob_skillid2skillidx(int class, int skillid) {

	int i;
	struct mob_skill *ms = mob_db[class].skill;

	if (ms==NULL)
		return -1;

	for(i=0;i<mob_db[class].maxskill;i++) {
		if (ms[i].skill_id == skillid)
			return i;
	}

	return -1;
}

/*==========================================
 *
 *------------------------------------------
 */
int mobskill_castend_id(int tid, unsigned int tick, int id, int data) {

	struct mob_data* md = NULL;
	struct block_list *bl;
	struct block_list *mbl;
	int range;

	if ((mbl = map_id2bl(id)) == NULL)
		return 0;
	if ((md=(struct mob_data *)mbl) == NULL) {
		printf("mobskill_castend_id nullpo mbl->id:%d\n",mbl->id);
		return 0;
	}

	if (md->bl.type != BL_MOB || md->bl.prev == NULL)
		return 0;

	if (md->skilltimer != tid)
		return 0;

	md->skilltimer = -1;

	if (md->sc_count) {
		if (md->opt1>0 || md->sc_data[SC_SILENCE].timer != -1 || md->sc_data[SC_ROKISWEIL].timer != -1 || md->sc_data[SC_STEELBODY].timer != -1)
			return 0;
		if (md->sc_data[SC_AUTOCOUNTER].timer != -1 && md->skillid != KN_AUTOCOUNTER)
			return 0;
		if (md->sc_data[SC_BLADESTOP].timer != -1)
			return 0;
		if (md->sc_data[SC_BERSERK].timer != -1)
			return 0;
	}
	if (md->skillid != NPC_EMOTION)
		md->last_thinktime = tick + status_get_adelay(&md->bl);

	if ((bl = map_id2bl(md->skilltarget)) == NULL || bl->prev==NULL) {
		return 0;
	}
	if (md->bl.m != bl->m)
		return 0;

	if (md->skillid == PR_LEXAETERNA) {
		struct status_change *sc_data = status_get_sc_data(bl);
		if (sc_data && (sc_data[SC_FREEZE].timer != -1 || (sc_data[SC_STONE].timer != -1 && sc_data[SC_STONE].val2 == 0)))
			return 0;
	} else if (md->skillid == RG_BACKSTAP) {
		int dir = map_calc_dir(&md->bl,bl->x,bl->y);
		int t_dir = status_get_dir(bl);
		int dist = distance(md->bl.x,md->bl.y,bl->x,bl->y);
		if (bl->type != BL_SKILL && (dist == 0 || map_check_dir(dir,t_dir)))
			return 0;
	}
	if (((skill_get_inf(md->skillid)&1) || (skill_get_inf2(md->skillid)&4)) &&
		battle_check_target(&md->bl, bl, BCT_ENEMY) <= 0)
		return 0;
	range = skill_get_range(md->skillid, md->skilllv, 0, 0);
	if (range < 0)
		range = status_get_range(&md->bl) - (range + 1);
	if (range + battle_config.mob_skill_add_range < distance(md->bl.x,md->bl.y,bl->x,bl->y))
		return 0;

	md->skilldelay[md->skillidx]=tick;

	switch(skill_get_nk(md->skillid)) {
	case 0:	
		skill_castend_damage_id(&md->bl, bl, md->skillid, md->skilllv, tick, 0);
		break;
	case 1:
		skill_castend_nodamage_id(&md->bl, bl, md->skillid, md->skilllv, tick, 0);
		break;
	case 2:
		skill_castend_damage_id(&md->bl, bl, md->skillid, md->skilllv, tick, 0);
		break;
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int mobskill_castend_pos(int tid, unsigned int tick, int id, int data) {

	struct mob_data* md = NULL;
	struct block_list *bl;
	int range,maxcount;

	if ((bl = map_id2bl(id)) == NULL)
		return 0;

	nullpo_retr(0, md = (struct mob_data *)bl);

	if (md->bl.type != BL_MOB || md->bl.prev == NULL)
		return 0;

	if (md->skilltimer != tid)
		return 0;

	md->skilltimer = -1;

	if (md->sc_count) {
		if (md->opt1>0 || md->sc_data[SC_SILENCE].timer != -1 || md->sc_data[SC_ROKISWEIL].timer != -1 || md->sc_data[SC_STEELBODY].timer != -1)
			return 0;
		if (md->sc_data[SC_AUTOCOUNTER].timer != -1 && md->skillid != KN_AUTOCOUNTER)
			return 0;
		if (md->sc_data[SC_BLADESTOP].timer != -1)
			return 0;
		if (md->sc_data[SC_BERSERK].timer != -1)
			return 0;
	}

	if (!battle_config.monster_skill_reiteration &&
	    skill_get_unit_flag(md->skillid) & UF_NOREITERATION &&
	    skill_check_unit_range(md->bl.m, md->skillx, md->skilly, md->skillid, md->skilllv))
		return 0;

	if (battle_config.monster_land_skill_limit) {
		maxcount = skill_get_maxcount(md->skillid);
		if (maxcount > 0) {
			int i,c;
			for(i=c=0;i<MAX_MOBSKILLUNITGROUP;i++) {
				if (md->skillunit[i].alive_count > 0 && md->skillunit[i].skill_id == md->skillid)
					c++;
			}
			if (c >= maxcount)
				return 0;
		}
	}

	range = skill_get_range(md->skillid, md->skilllv, 0, 0);
	if (range < 0)
		range = status_get_range(&md->bl) - (range + 1);
	if (range + battle_config.mob_skill_add_range < distance(md->bl.x,md->bl.y,md->skillx,md->skilly))
		return 0;
	md->skilldelay[md->skillidx]=tick;

	skill_castend_pos2(&md->bl, md->skillx, md->skilly, md->skillid, md->skilllv, tick, 0);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int mobskill_use_id(struct mob_data *md, struct block_list *target, int skill_idx) {

	int casttime,range;
	struct mob_skill *ms;
	int skill_id, skill_lv, forcecast = 0;

	nullpo_retr(0, md);
	nullpo_retr(0, ms = &mob_db[md->class].skill[skill_idx]);

	if (target == NULL && (target = map_id2bl(md->target_id)) == NULL)
		return 0;

	if (target->prev == NULL || md->bl.prev == NULL)
		return 0;

	skill_id = ms->skill_id;
	skill_lv = ms->skill_lv;

	if (md->sc_count) {
		if (md->opt1 > 0 || md->sc_data[SC_SILENCE].timer != -1 || md->sc_data[SC_ROKISWEIL].timer != -1 || md->sc_data[SC_STEELBODY].timer != -1)
			return 0;
		if (md->sc_data[SC_LANDPROTECTOR].timer != -1 && (skill_id == AL_TELEPORT || skill_id == AL_WARP))
			return 0;
		if (md->sc_data[SC_AUTOCOUNTER].timer != -1 && skill_id != KN_AUTOCOUNTER)
			return 0;
		if (md->sc_data[SC_BLADESTOP].timer != -1)
			return 0;
		if (md->sc_data[SC_BERSERK].timer != -1)
			return 0;
	}

	if (md->option & 4 && skill_id == TF_HIDING)
		return 0;

	if (md->option & 2 && skill_id != TF_HIDING && skill_id != AS_GRIMTOOTH && skill_id != RG_BACKSTAP && skill_id != RG_RAID)
		return 0;

	if (map[md->bl.m].flag.gvg && (skill_id == SM_ENDURE || skill_id == AL_TELEPORT || skill_id == AL_WARP ||
		skill_id == WZ_ICEWALL || skill_id == TF_BACKSLIDING))
		return 0;

	if (skill_get_inf2(skill_id)&0x200 && md->bl.id == target->id)
		return 0;

	range = skill_get_range(skill_id, skill_lv, 0, 0);
	if (range < 0)
		range = status_get_range(&md->bl) - (range + 1);
	if (!battle_check_range(&md->bl, target, range))
		return 0;

	casttime = skill_castfix(&md->bl, ms->casttime);

	md->state.skillcastcancel = ms->cancel;
	md->skilldelay[skill_idx] = gettick_cache;

	switch(skill_id) {
	case ALL_RESURRECTION:
		if (target->type != BL_PC && battle_check_undead(status_get_race(target), status_get_elem_type(target))){	/* 敵がアンデッドなら */
			forcecast=1;
			casttime=skill_castfix(&md->bl, skill_get_cast(PR_TURNUNDEAD,skill_lv) );
		}
		break;
	case MO_EXTREMITYFIST:
	case SA_MAGICROD:
	case SA_SPELLBREAKER:
		forcecast=1;
		break;
	case NPC_SUMMONSLAVE:
	case NPC_SUMMONMONSTER:
		if (md->master_id != 0)
			return 0;
		break;
	}

	if (casttime > 0 || forcecast) {
		mob_stop_walking(md,0);
		clif_skillcasting( &md->bl,
			md->bl.id, target->id, 0,0, skill_id,casttime);
	}

	if (casttime <= 0)
		md->state.skillcastcancel=0;

	md->skilltarget	= target->id;
	md->skillx		= 0;
	md->skilly		= 0;
	md->skillid		= skill_id;
	md->skilllv		= skill_lv;
	md->skillidx	= skill_idx;

	if (!(battle_config.monster_cloak_check_type & 2) && md->sc_data[SC_CLOAKING].timer != -1 && md->skillid != AS_CLOAKING)
		status_change_end(&md->bl, SC_CLOAKING, -1);

	if (md->skilltimer != -1) {
		int inf;
		if ((inf = skill_get_inf(md->skillid)) == 2 || inf == 32)
			delete_timer(md->skilltimer, mobskill_castend_pos);
		else
			delete_timer(md->skilltimer, mobskill_castend_id);
		md->skilltimer = -1;
	}

	// Skill has cast time
	if (casttime > 0) {
		md->skilltimer = add_timer(gettick_cache + casttime, mobskill_castend_id, md->bl.id, 0);
	// Cast skill immediately
	} else {
		mobskill_castend_id(md->skilltimer, gettick_cache, md->bl.id, 0);
	}

	return 1;
}

/*==========================================
 *
 *------------------------------------------
 */
int mobskill_use_pos(struct mob_data *md, int skill_x, int skill_y, int skill_idx) {

	int casttime = 0, range;
	struct mob_skill *ms;
	struct block_list bl;
	int skill_id, skill_lv;

	nullpo_retr(0, md);
	nullpo_retr(0, ms = &mob_db[md->class].skill[skill_idx]);

	if (md->bl.prev == NULL)
		return 0;

	skill_id = ms->skill_id;
	skill_lv = ms->skill_lv;

	if (md->sc_count) {
		if (md->opt1 > 0 || md->sc_data[SC_SILENCE].timer != -1 ||
		    (!(mob_db[md->class].mode & 0x20) && md->sc_data[SC_ROKISWEIL].timer != -1) ||
		    md->sc_data[SC_STEELBODY].timer != -1)
			return 0;
		if (md->sc_data[SC_LANDPROTECTOR].timer != -1 && (md->skillid == AL_TELEPORT || md->skillid == AL_WARP))
			return 0;
		if (md->sc_data[SC_AUTOCOUNTER].timer != -1 && md->skillid != KN_AUTOCOUNTER)
			return 0;
		if (md->sc_data[SC_BLADESTOP].timer != -1)
			return 0;
		if (md->sc_data[SC_BERSERK].timer != -1)
			return 0;
	}

	if (md->option & 2)
		return 0;

	if (map[md->bl.m].flag.gvg && (skill_id == SM_ENDURE || skill_id == AL_TELEPORT || skill_id == AL_WARP ||
	    skill_id == WZ_ICEWALL || skill_id == TF_BACKSLIDING))
		return 0;

	bl.type = BL_NUL;
	bl.m = md->bl.m;
	bl.x = skill_x;
	bl.y = skill_y;
	range = skill_get_range(skill_id, skill_lv, 0, 0);
	if (range < 0)
		range = status_get_range(&md->bl) - (range + 1);
	if (!battle_check_range(&md->bl, &bl, range))
		return 0;

	casttime = skill_castfix(&md->bl, ms->casttime);
	md->skilldelay[skill_idx] = gettick_cache;
	md->state.skillcastcancel = ms->cancel;

	if (casttime > 0) {
		mob_stop_walking(md, 0);
		clif_skillcasting(&md->bl, md->bl.id, 0, skill_x, skill_y, skill_id, casttime);
	}

	if (casttime <= 0)
		md->state.skillcastcancel = 0;

	md->skillx      = skill_x;
	md->skilly      = skill_y;
	md->skilltarget = 0;
	md->skillid     = skill_id;
	md->skilllv     = skill_lv;
	md->skillidx    = skill_idx;

	if (!(battle_config.monster_cloak_check_type & 2) && md->sc_data[SC_CLOAKING].timer != -1)
		status_change_end(&md->bl, SC_CLOAKING, -1);

	if (md->skilltimer != -1) {
		int inf;
		if ((inf = skill_get_inf(md->skillid)) == 2 || inf == 32)
			delete_timer(md->skilltimer, mobskill_castend_pos);
		else
			delete_timer(md->skilltimer, mobskill_castend_id);
		md->skilltimer = -1;
	}

	// Skill has cast time
	if (casttime > 0) {
		md->skilltimer = add_timer(gettick_cache + casttime, mobskill_castend_pos, md->bl.id, 0);
	// Cast skill immediately
	} else {
		mobskill_castend_pos(md->skilltimer, gettick_cache, md->bl.id, 0);
	}

	return 1;
}


/*==========================================
 *
 *------------------------------------------
 */
int mob_getfriendhpltmaxrate_sub(struct block_list *bl,va_list ap) {

	int rate;
	struct mob_data **fr, *md, *mmd;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);
	nullpo_retr(0, mmd = va_arg(ap,struct mob_data *));

	md = (struct mob_data *)bl;

	if (mmd->bl.id == bl->id)
		return 0;
	if (battle_check_target(&mmd->bl, bl, BCT_ENEMY) > 0)
		return 0;
	
	rate = va_arg(ap, int);
	fr = va_arg(ap, struct mob_data **);
	if (md->hp < mob_db[md->class].max_hp * rate / 100)
		(*fr) = md;

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
struct mob_data *mob_getfriendhpltmaxrate(struct mob_data *md,int rate) {

	struct mob_data *fr = NULL;
	const int r = 8;

	nullpo_retr(NULL, md);

	map_foreachinarea(mob_getfriendhpltmaxrate_sub, md->bl.m,
	                  md->bl.x - r ,md->bl.y - r, md->bl.x + r, md->bl.y + r,
	                  BL_MOB, md, rate, &fr);

	return fr;
}

/*==========================================
 *
 *------------------------------------------
 */
struct block_list *mob_getmasterhpltmaxrate(struct mob_data *md, int rate) {

	if (md && md->master_id > 0) {
		struct block_list *bl = map_id2bl(md->master_id);
		if (status_get_hp(bl) < status_get_max_hp(bl) * rate / 100)
			return bl;
	}

	return NULL;
}

/*==========================================
 *
 *------------------------------------------
 */
int mob_getfriendstatus_sub(struct block_list *bl,va_list ap) {

	int cond1,cond2;
	struct mob_data **fr, *md, *mmd;
	int flag=0;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);
	nullpo_retr(0, md=(struct mob_data *)bl);
	nullpo_retr(0, mmd=va_arg(ap,struct mob_data *));

	if (mmd->bl.id == bl->id)
		return 0;
	if (battle_check_target(&mmd->bl, bl, BCT_ENEMY) > 0)
		return 0;
		
	cond1=va_arg(ap,int);
	cond2=va_arg(ap,int);
	fr=va_arg(ap,struct mob_data **);
	if (cond2==-1) {
		int j;
		for(j=SC_STONE;j<=SC_BLIND && !flag;j++){
			flag=(md->sc_data[j].timer!=-1);
		}
	} else
		flag=(md->sc_data[cond2].timer!=-1);
	if (flag^(cond1==MSC_FRIENDSTATUSOFF))
		(*fr)=md;

	return 0;
}

/*==========================================
 * What a status state suits by nearby MOB is looked for
 *------------------------------------------
 */
struct mob_data *mob_getfriendstatus(struct mob_data *md, int cond1, int cond2) {

	struct mob_data *fr = NULL;
	const int r = 8;

	nullpo_retr(0, md);

	map_foreachinarea(mob_getfriendstatus_sub, md->bl.m,
	                  md->bl.x - r ,md->bl.y - r, md->bl.x + r, md->bl.y + r,
	                  BL_MOB, md, cond1, cond2, &fr);

	return fr;
}

/*==========================================
 * Skill use judging
 *------------------------------------------
 */
int mobskill_use(struct mob_data *md, unsigned int tick, int event) {

	struct mob_skill *ms;
	int i;

	nullpo_retr(0, md);
	nullpo_retr(0, ms = mob_db[md->class].skill);

	// If mob skills are disabled or if mob is already casting a skill, cancel
	if (battle_config.mob_skill_use == 0 || md->skilltimer != -1)
		return 0;

	// If Self Destruction status is active, cancel
	if (md->sc_data[SC_SELFDESTRUCTION].timer != -1)
		return 0;

	// If using Spheremine AI, cancel
	if (md->state.special_mob_ai >= 2) // 0: Nothing, 1: Cannibalize, 2-3: Spheremine
		return 0;

	// If mob is dead, set mob as dead but continue skill as well
	if (md->hp <= 0)
		md->state.skillstate = MSS_DEAD;

	for(i=0;i<mob_db[md->class].maxskill;i++) {
		int c2=ms[i].cond2,flag=0;
		struct mob_data *fmd = NULL;
		struct map_session_data *fsd = NULL;

		if (DIFF_TICK(tick,md->skilldelay[i])<ms[i].delay)
			continue;

		if (ms[i].state>=0 && ms[i].state!=md->state.skillstate)
			continue;

		flag=(event==ms[i].cond1);
		if (!flag) {
			switch(ms[i].cond1){
			case MSC_ALWAYS:
				flag=1; break;
			case MSC_MYHPLTMAXRATE:
				{
					int max_hp = status_get_max_hp(&md->bl);	
					flag = (md->hp < max_hp * c2 / 100); break;
				}
			case MSC_MYSTATUSON:
			case MSC_MYSTATUSOFF:
				if (ms[i].cond2==-1) {
					int j;
					for(j=SC_STONE;j<=SC_BLIND && !flag;j++) {
						flag=(md->sc_data[j].timer!=-1 );
					}
				} else
					flag=(md->sc_data[ms[i].cond2].timer!=-1);
				flag ^= (ms[i].cond1 == MSC_MYSTATUSOFF);
				break;
			case MSC_FRIENDHPLTMAXRATE:
				flag = ((fmd = mob_getfriendhpltmaxrate(md, ms[i].cond2)) != NULL);
				break;
			case MSC_FRIENDSTATUSON:
			case MSC_FRIENDSTATUSOFF:
				flag = ((fmd = mob_getfriendstatus(md, ms[i].cond1, ms[i].cond2)) != NULL);
				break;
			case MSC_SLAVELT:
				flag = (mob_countslave(md) < c2);
				break;
			case MSC_ATTACKPCGT:
				flag = (mob_counttargeted(md, NULL, 0) > c2);
				break;
			case MSC_SLAVELE:
				flag = (mob_countslave(md) <= c2);
				break;
			case MSC_ATTACKPCGE:
				flag = (mob_counttargeted(md, NULL, 0) >= c2);
				break;
			case MSC_AFTERSKILL:
				flag = (md->skillid == c2);
				break;
			case MSC_SKILLUSED:
				flag = ((event & 0xffff) == MSC_SKILLUSED && ((event >> 16) == c2 || c2 == 0));
				break;
			case MSC_RUDEATTACKED:
				flag = (!md->attacked_id && md->attacked_count > 0);
				if (flag) md->attacked_count = 0;
				break;
			case MSC_MASTERATTACKED:
				flag = (md->master_id > 0 && battle_counttargeted(map_id2bl(md->master_id), NULL, 0) > 0);
				break;
			case MSC_MASTERHPLTMAXRATE:
				{
					struct block_list *bl = mob_getmasterhpltmaxrate(md, ms[i].cond2);
					if (bl) {
						if (bl->type == BL_MOB)
							fmd = (struct mob_data *)bl;
						else if (bl->type == BL_PC)
							fsd = (struct map_session_data *)bl;
					}
					flag = (fmd || fsd);
					break;
				}
			}
		}

		if (flag && rand() % 10000 < ms[i].permillage) {
			int inf;
			if ((inf = skill_get_inf(ms[i].skill_id)) == 2 || inf == 32) {
				struct block_list *bl = NULL;
				int x=0,y=0;
				if (ms[i].target<=MST_AROUND) {
					switch (ms[i].target) {
						case MST_TARGET:
						case MST_AROUND5:
							bl = map_id2bl(md->target_id);
							break;
						case MST_FRIEND:
							if (fmd) {
								bl = &fmd->bl;
								break;
							} else if (fsd) {
								bl = &fsd->bl;
								break;
							}
						default:
							bl = &md->bl;
							break;
					}
					if (bl != NULL) {
						x = bl->x;
						y = bl->y;
					}
				}
				if (x <= 0 || y <= 0)
					continue;
				if (ms[i].target >= MST_AROUND1) {
					int bx = x, by = y, j, m = bl->m, r = ms[i].target - MST_AROUND1;
					j = 0;
					do {
						bx = x + rand() % (r * 2 + 3) - r;
						by = y + rand() % (r * 2 + 3) - r;
						j++;
					} while(map_getcell(m, bx, by, CELL_CHKNOPASS) && j < 1000);
					if (j < 1000) {
						x = bx;
						y = by;
					}
				}
				if (ms[i].target >= MST_AROUND5) {
					int bx=x, by=y, j, m=bl->m, r=(ms[i].target-MST_AROUND5) + 1;
					j = 0;
					do {
						bx = x + rand() % (r*2+1) - r;
						by = y + rand() % (r*2+1) - r;
						j++;
					} while(map_getcell(m, bx, by, CELL_CHKNOPASS) && j < 1000);
					if (j < 1000) {
						x = bx;
						y = by;
					}
				}
				if (!mobskill_use_pos(md,x,y,i))
					return 0;

			} else {
				if (ms[i].target<=MST_FRIEND) {
					struct block_list *bl;
					switch (ms[i].target)
					{
						case MST_TARGET:
							bl = map_id2bl(md->target_id);
							break;
						case MST_FRIEND:
							if (fmd) {
								bl = &fmd->bl;
								break;
							} else if (fsd) {
								bl = &fsd->bl;
								break;
							}
						default:
							bl = &md->bl;
							break;
					}
					if (bl && !mobskill_use_id(md,bl,i))
						return 0;
				}
			}
			if (ms[i].emotion >= 0)
				clif_emotion(&md->bl,ms[i].emotion);
			return 1;
		}
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int mobskill_event(struct mob_data *md, int flag) {

	nullpo_retr(0, md);

	if (flag == -1 && mobskill_use(md, gettick_cache, MSC_CASTTARGETED))
		return 1;
	if ((flag & BF_SHORT) && mobskill_use(md, gettick_cache, MSC_CLOSEDATTACKED))
		return 1;
	if ((flag & BF_LONG) && mobskill_use(md, gettick_cache, MSC_LONGRANGEATTACKED))
		return 1;

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int mob_gvmobcheck(struct map_session_data *sd, struct block_list *bl) {

	struct mob_data *md = NULL;

	nullpo_retr(0, sd);
	nullpo_retr(0, bl);

	if (bl->type == BL_MOB && (md = (struct mob_data *)bl) && (md->class <= 1288 && md->class >= 1285)) {
		// 1288 = Emperium, 1287 = guardian, 1286 = guardian, 1285 = guardian
		struct guild_castle *gc = guild_mapname2gc(map[sd->bl.m].name);
		struct guild *g = guild_search(sd->status.guild_id);

		if (g == NULL)
			return 0; // return 0, player has no guild
		else if (gc != NULL && !map[sd->bl.m].flag.gvg)
			return 0; // return 0, player is in castle and GvG is disabled
		else if (gc != NULL && g->guild_id == gc->guild_id)
			return 0; // return 0, player is in a castle, and his guild is the same as the mob's guild
		else if (g && guild_checkskill(g, GD_APPROVAL) <= 0)
			return 0;// return 0, player is in a guild, player's guild does not have GD_APPROVAL guild skill
	}

	return 1;
}

/*==========================================
 *
 *------------------------------------------
 */
void mob_makedummymobdb(int class) {

	int i;

	sprintf(mob_db[class].name, "mob%d", class);
	sprintf(mob_db[class].jname, "mob%d", class);
	mob_db[class].lv = 1;
	mob_db[class].max_hp = 1000;
	mob_db[class].max_sp = 1;
	mob_db[class].base_exp = 2;
	mob_db[class].job_exp = 1;
	mob_db[class].range = 1;
	mob_db[class].atk1 = 7;
	mob_db[class].atk2 = 10;
	mob_db[class].def = 0;
	mob_db[class].mdef = 0;
	mob_db[class].str = 1;
	mob_db[class].agi = 1;
	mob_db[class].vit = 1;
	mob_db[class].int_ = 1;
	mob_db[class].dex = 6;
	mob_db[class].luk = 2;
	mob_db[class].range2 = 10;
	mob_db[class].range3 = 10;
	mob_db[class].size = 0;
	mob_db[class].race = 0;
	mob_db[class].element = 0;
	mob_db[class].mode = 0;
	mob_db[class].speed = 300;
	mob_db[class].adelay = 1000;
	mob_db[class].amotion = 500;
	mob_db[class].dmotion = 500;
	mob_db[class].dropitem[0].nameid = 909;
	mob_db[class].dropitem[0].p = 1000;
	for(i = 1; i < 10; i++) {
		mob_db[class].dropitem[i].nameid = 0;
		mob_db[class].dropitem[i].p = 0;
	}
	mob_db[class].mexp = 0;
	mob_db[class].mexpper = 0;
	for(i = 0; i < 3; i++) {
		mob_db[class].mvpitem[i].nameid = 0;
		mob_db[class].mvpitem[i].p = 0;
	}
	for(i = 0; i < MAX_RANDOMMONSTER; i++)
		mob_db[class].summonper[i] = 0;

	return;
}

/*==========================================
 * db/mob_db.txt reading
 *------------------------------------------
 */
static int mob_readdb(void) {

	FILE *fp;
	char line[1024];
	char *filename[] = { "db/mob_db.txt", "db/mob_db2.txt" };
	int i, ln;
	double temp_exp;
	FILE *mob_db_fp = NULL;
	#define MOB_DB_SQL_NAME "mob_db_map.sql"

	memset(mob_db, 0, sizeof(mob_db));

	// Code to create SQL mob db
	if (create_mob_db_script) {
		// If file exists (Doesn't exist, was renamed!... not normal that continue to exist)
		if (access(MOB_DB_SQL_NAME, 0) == 0) // 0: file exist or not, 0 = success
			remove(MOB_DB_SQL_NAME); // Delete the file, return value = 0 (success), return value = -1 (Access denied or file not found)
		if ((mob_db_fp = fopen(MOB_DB_SQL_NAME, "a")) != NULL) {
			printf(CL_GREEN "Loaded: " CL_RESET "'" CL_WHITE MOB_DB_SQL_NAME CL_RESET "' file generated.\n");
			fprintf(mob_db_fp, "# You can regenerate this file with an option in inter_freya.conf" RETCODE);
			fprintf(mob_db_fp, RETCODE);
			fprintf(mob_db_fp, "CREATE TABLE `%s` (" RETCODE, mob_db_db);
			fprintf(mob_db_fp, "  `ID` mediumint(9) NOT NULL default '0'," RETCODE);
			fprintf(mob_db_fp, "  `Name` text NOT NULL," RETCODE);
			fprintf(mob_db_fp, "  `Name2` text NOT NULL," RETCODE);
			fprintf(mob_db_fp, "  `LV` smallint(6) NOT NULL default '0'," RETCODE);
			fprintf(mob_db_fp, "  `HP` mediumint(9) NOT NULL default '0'," RETCODE);
			fprintf(mob_db_fp, "  `SP` mediumint(9) NOT NULL default '0'," RETCODE);
			fprintf(mob_db_fp, "  `EXP` mediumint(9) NOT NULL default '0'," RETCODE);
			fprintf(mob_db_fp, "  `JEXP` mediumint(9) NOT NULL default '0'," RETCODE);
			fprintf(mob_db_fp, "  `Range1` tinyint(4) NOT NULL default '0'," RETCODE);
			fprintf(mob_db_fp, "  `ATK1` smallint(6) NOT NULL default '0'," RETCODE);
			fprintf(mob_db_fp, "  `ATK2` smallint(6) NOT NULL default '0'," RETCODE);
			fprintf(mob_db_fp, "  `DEF` smallint(6) NOT NULL default '0'," RETCODE);
			fprintf(mob_db_fp, "  `MDEF` smallint(6) NOT NULL default '0'," RETCODE);
			fprintf(mob_db_fp, "  `STR` tinyint(4) UNSIGNED NOT NULL default '0'," RETCODE);
			fprintf(mob_db_fp, "  `AGI` tinyint(4) UNSIGNED NOT NULL default '0'," RETCODE);
			fprintf(mob_db_fp, "  `VIT` tinyint(4) UNSIGNED NOT NULL default '0'," RETCODE);
			fprintf(mob_db_fp, "  `INT` tinyint(4) UNSIGNED NOT NULL default '0'," RETCODE);
			fprintf(mob_db_fp, "  `DEX` tinyint(4) UNSIGNED NOT NULL default '0'," RETCODE);
			fprintf(mob_db_fp, "  `LUK` tinyint(4) UNSIGNED NOT NULL default '0'," RETCODE);
			fprintf(mob_db_fp, "  `Range2` tinyint(4) NOT NULL default '0'," RETCODE);
			fprintf(mob_db_fp, "  `Range3` tinyint(4) NOT NULL default '0'," RETCODE);
			fprintf(mob_db_fp, "  `Scale` tinyint(4) NOT NULL default '0'," RETCODE);
			fprintf(mob_db_fp, "  `Race` tinyint(4) NOT NULL default '0'," RETCODE);
			fprintf(mob_db_fp, "  `Element` tinyint(4) NOT NULL default '0'," RETCODE);
			fprintf(mob_db_fp, "  `Mode` smallint(6) NOT NULL default '0'," RETCODE);
			fprintf(mob_db_fp, "  `Speed` smallint(6) NOT NULL default '0'," RETCODE);
			fprintf(mob_db_fp, "  `ADelay` smallint(6) NOT NULL default '0'," RETCODE);
			fprintf(mob_db_fp, "  `aMotion` smallint(6) NOT NULL default '0'," RETCODE);
			fprintf(mob_db_fp, "  `dMotion` smallint(6) NOT NULL default '0'," RETCODE);
			fprintf(mob_db_fp, "  `Drop1id` mediumint(9) NOT NULL default '0'," RETCODE);
			fprintf(mob_db_fp, "  `Drop1per` mediumint(9) NOT NULL default '0'," RETCODE);
			fprintf(mob_db_fp, "  `Drop2id` mediumint(9) NOT NULL default '0'," RETCODE);
			fprintf(mob_db_fp, "  `Drop2per` mediumint(9) NOT NULL default '0'," RETCODE);
			fprintf(mob_db_fp, "  `Drop3id` mediumint(9) NOT NULL default '0'," RETCODE);
			fprintf(mob_db_fp, "  `Drop3per` mediumint(9) NOT NULL default '0'," RETCODE);
			fprintf(mob_db_fp, "  `Drop4id` mediumint(9) NOT NULL default '0'," RETCODE);
			fprintf(mob_db_fp, "  `Drop4per` mediumint(9) NOT NULL default '0'," RETCODE);
			fprintf(mob_db_fp, "  `Drop5id` mediumint(9) NOT NULL default '0'," RETCODE);
			fprintf(mob_db_fp, "  `Drop5per` mediumint(9) NOT NULL default '0'," RETCODE);
			fprintf(mob_db_fp, "  `Drop6id` mediumint(9) NOT NULL default '0'," RETCODE);
			fprintf(mob_db_fp, "  `Drop6per` mediumint(9) NOT NULL default '0'," RETCODE);
			fprintf(mob_db_fp, "  `Drop7id` mediumint(9) NOT NULL default '0'," RETCODE);
			fprintf(mob_db_fp, "  `Drop7per` mediumint(9) NOT NULL default '0'," RETCODE);
			fprintf(mob_db_fp, "  `Drop8id` mediumint(9) NOT NULL default '0'," RETCODE);
			fprintf(mob_db_fp, "  `Drop8per` mediumint(9) NOT NULL default '0'," RETCODE);
			fprintf(mob_db_fp, "  `Drop9id` mediumint(9) NOT NULL default '0'," RETCODE);
			fprintf(mob_db_fp, "  `Drop9per` mediumint(9) NOT NULL default '0'," RETCODE);
			fprintf(mob_db_fp, "  `DropCardid` mediumint(9) NOT NULL default '0'," RETCODE);
			fprintf(mob_db_fp, "  `DropCardper` mediumint(9) NOT NULL default '0'," RETCODE);
			fprintf(mob_db_fp, "  `MEXP` mediumint(9) NOT NULL default '0'," RETCODE);
			fprintf(mob_db_fp, "  `ExpPer` mediumint(9) NOT NULL default '0'," RETCODE);
			fprintf(mob_db_fp, "  `MVP1id` mediumint(9) NOT NULL default '0'," RETCODE);
			fprintf(mob_db_fp, "  `MVP1per` mediumint(9) NOT NULL default '0'," RETCODE);
			fprintf(mob_db_fp, "  `MVP2id` mediumint(9) NOT NULL default '0'," RETCODE);
			fprintf(mob_db_fp, "  `MVP2per` mediumint(9) NOT NULL default '0'," RETCODE);
			fprintf(mob_db_fp, "  `MVP3id` mediumint(9) NOT NULL default '0'," RETCODE);
			fprintf(mob_db_fp, "  `MVP3per` mediumint(9) NOT NULL default '0'" RETCODE);
			fprintf(mob_db_fp, ") TYPE = MyISAM;" RETCODE);
			fprintf(mob_db_fp, RETCODE);
		} else {
			printf(CL_RED "Error: " CL_RESET "Failed to generate the '" MOB_DB_SQL_NAME "' file." CL_RESET"\n");
			create_mob_db_script = 0; // Don't continue to try to create file after
		}
	}

	for(i = 0; i < sizeof(filename) / sizeof(filename[0]); i++) {

		ln = 0;
		fp = fopen(filename[i], "r");
		if (fp == NULL){
			if (i > 0) {
				continue;
			} else {
				// Code to create SQL mob db
				if (create_mob_db_script && mob_db_fp != NULL) {
					fclose(mob_db_fp);
				}
				return -1;
			}
		}
		while(fgets(line, sizeof(line), fp)) { // fgets reads until maximum one less than size and add '\0' -> so, it's not necessary to add -1
			int class, j;
			char *str[57], *p, *np;

			if ((line[0] == '/' && line[1] == '/') || line[0] == '\0' || line[0] == '\n' || line[0] == '\r') {
				// Code to create SQL mob db
				if (create_mob_db_script && mob_db_fp != NULL) {
					// Remove carriage return if exist
					while(line[0] != '\0' && (line[(j = strlen(line) - 1)] == '\n' || line[j] == '\r'))
						line[j] = '\0';
					// Add comments in the SQL script
					if ((line[0] == '/' && line[1] == '/' && strlen(line) > 2))
					{
						fprintf(mob_db_fp, "#%s" RETCODE, line + 2);
					} else {
						fprintf(mob_db_fp, RETCODE);
					}
				}
				continue;
			}

			// Remove carriage return if exists
			while(line[0] != '\0' && (line[(j = strlen(line) - 1)] == '\n' || line[j] == '\r'))
				line[j] = '\0';

			for(j = 0, p = line; j < 57; j++) {
				if ((np = strchr(p, ',')) != NULL) {
					str[j] = p;
					*np = 0;
					p = np + 1;
					if (j == 56) {
						printf(CL_YELLOW "WARNING: Invalid monster line (more parameters than required)" CL_RESET ": %s\n", line);
					}
				} else {
					str[j] = p;
					if (j < 56) {
						printf(CL_YELLOW "WARNING: Invalid mob_db.txt line (not enough parameters: %d instead of 56)" CL_RESET ": %s\n", j, line);
					}
				}
			}

			class = atoi(str[0]);
			if (class <= 1000 || class >= MAX_MOB_DB)
				continue;

			ln++;
			if (ln % 20 == 19) {
				printf("Reading mob #%d (class id: %d)...\r", ln, class);
				fflush(stdout);
			}
			mob_db[class].view_class = class;
			strncpy(mob_db[class].name, str[1], 24);
			strncpy(mob_db[class].jname, str[2], 24);
			mob_db[class].lv = atoi(str[3]);
			mob_db[class].max_hp = atoi(str[4]);
			mob_db[class].max_sp = atoi(str[5]);

			mob_db[class].base_exp = atoi(str[6]);
			if (mob_db[class].base_exp <= 0)
				mob_db[class].base_exp = 0;
			else {
				temp_exp = ((double)mob_db[class].base_exp) * ((double)battle_config.base_exp_rate) / 100.;
				if (temp_exp > ((double)0x7fffffff) || temp_exp < 0 || ((int)temp_exp) > 0x7fffffff)
					mob_db[class].base_exp = 0x7fffffff;
				else {
					mob_db[class].base_exp = (int)temp_exp;
					if (mob_db[class].base_exp < 1)
						mob_db[class].base_exp = 1;
				}
			}

			mob_db[class].job_exp = atoi(str[7]);
			if (mob_db[class].job_exp <= 0)
				mob_db[class].job_exp = 0;
			else {
				temp_exp = ((double)mob_db[class].job_exp) * ((double)battle_config.job_exp_rate) / 100.;
				if (temp_exp > ((double)0x7fffffff) || temp_exp < 0 || ((int)temp_exp) > 0x7fffffff)
					mob_db[class].job_exp = 0x7fffffff;
				else {
					mob_db[class].job_exp = (int)temp_exp;
					if (mob_db[class].job_exp < 1)
						mob_db[class].job_exp = 1;
				}
			}

			mob_db[class].range = atoi(str[8]);
			mob_db[class].atk1 = atoi(str[9]);
			mob_db[class].atk2 = atoi(str[10]);
			mob_db[class].def = atoi(str[11]);
			mob_db[class].mdef = atoi(str[12]);
			mob_db[class].str = atoi(str[13]);
			mob_db[class].agi = atoi(str[14]);
			mob_db[class].vit = atoi(str[15]);
			mob_db[class].int_ = atoi(str[16]);
			mob_db[class].dex = atoi(str[17]);
			mob_db[class].luk = atoi(str[18]);
			mob_db[class].range2 = atoi(str[19]);
			mob_db[class].range3 = atoi(str[20]);
			mob_db[class].size = atoi(str[21]);
			mob_db[class].race = atoi(str[22]);
			mob_db[class].element = atoi(str[23]);
			mob_db[class].mode = atoi(str[24]);
			mob_db[class].speed = atoi(str[25]);
			mob_db[class].adelay = atoi(str[26]);
			mob_db[class].amotion = atoi(str[27]);
			mob_db[class].dmotion = atoi(str[28]);

		// If the attack animation is longer than the delay, the client crops the attack animation
		if (mob_db[class].adelay < 250 && (mob_db[class].adelay < mob_db[class].amotion))
			mob_db[class].adelay = mob_db[class].amotion;

			for(j = 0; j < 10; j++) {
				int nameid, percent;
				nameid = atoi(str[29+j*2]);
				if (nameid == 0) // nameid = 0, it's for no item
					continue;
				else if (nameid < 0 || itemdb_search(nameid) == NULL)
					printf("drop item error: unknown item id %d (%d: %s).\n", nameid, class, mob_db[class].name);
				else if (atoi(str[30+j*2]) > 0) { // nameid > 0 &&
					mob_db[class].dropitem[j].nameid = nameid;
					percent = atoi(str[30+j*2]);
					if (percent < 0) {
						printf("drop item error: item id %d, invalid percent: %d -> 0 (%d: %s).\n", nameid, percent, class, mob_db[class].name);
						percent = 0;
					} else if (percent > 10000) { // 100%
						printf("drop item warning: item id %d, too big percent: %d -> 10000 (%d: %s).\n", nameid, percent, class, mob_db[class].name);
						percent = 10000;
					}
					mob_db[class].dropitem[j].p = percent;
				}
			}

			mob_db[class].mexp = atoi(str[49]) * battle_config.mvp_exp_rate / 100;
			mob_db[class].mexpper = atoi(str[50]);

			for(j = 0; j < 3; j++) {
				int nameid, percent;
				nameid = atoi(str[51+j*2]);
				if (nameid == 0) // nameid = 0, it's for no item
					continue;
				else if (nameid < 0 || itemdb_search(nameid) == NULL)
					printf("mvp item error: unknown item id %d (%d: %s).\n", nameid, class, mob_db[class].name);
				else if (atoi(str[52+j*2]) > 0) { // nameid > 0 &&
					mob_db[class].mvpitem[j].nameid = nameid;
					percent = atoi(str[52+j*2]);
					if (percent < 0) {
						printf("mvp item error: item id %d, invalid percent: %d -> 0 (%d: %s).\n", nameid, percent, class, mob_db[class].name);
						percent = 0;
					} else if (percent > 10000) { // 100%
						printf("mvp item warning: item id %d, too big percent: %d -> 10000 (%d: %s).\n", nameid, percent, class, mob_db[class].name);
						percent = 10000;
					}
					mob_db[class].mvpitem[j].p = percent;
				}
			}
			// Code to create SQL mob db
			if (create_mob_db_script && mob_db_fp != NULL) {
				char mob_name[49]; // 24 * 2 + NULL
				char mob_jname[49]; // 24 * 2 + NULL

				struct mob_db *actual_mob;
				actual_mob = &mob_db[class];
				// Escape mob names
				memset(mob_name, 0, sizeof(mob_name));
				// mysql_escape_string(mob_name, actual_mob->name, strlen(actual_mob->name));
				mysql_escape_string(mob_name, str[1], strlen(str[1]));
				memset(mob_jname, 0, sizeof(mob_jname));
				// mysql_escape_string(mob_jname, actual_mob->jname, strlen(actual_mob->jname));
				mysql_escape_string(mob_jname, str[2], strlen(str[2]));
				// Create request
				fprintf(mob_db_fp, "INSERT INTO `%s` VALUES (%d, '%s', '%s', %d, %d, %d,"
				                                             " %d, %d,"
				                                             " %d, %d, %d, %d, %d,"
				                                             " %d, %d, %d, %d, %d, %d,"
				                                             " %d, %d, %d, %d, %d,"
				                                             " %d, %d, %d, %d, %d,"
				                                             " %d, %d, %d, %d,"
				                                             " %d, %d, %d, %d,"
				                                             " %d, %d, %d, %d,"
				                                             " %d, %d, %d, %d,"
				                                             " %d, %d, %d, %d,"
				                                             " %d, %d,"
				                                             " %d, %d, %d, %d,"
				                                             " %d, %d);" RETCODE,
				                    mob_db_db, class, mob_name, mob_jname, actual_mob->lv, actual_mob->max_hp, actual_mob->max_sp,
				                    atoi(str[6]), atoi(str[7]), // actual_mob->base_exp, actual_mob->job_exp: Not modified
				                    actual_mob->range, actual_mob->atk1, actual_mob->atk2, actual_mob->def, actual_mob->mdef,
				                    actual_mob->str, actual_mob->agi, actual_mob->vit, actual_mob->int_, actual_mob->dex, actual_mob->luk,
				                    actual_mob->range2, actual_mob->range3, actual_mob->size, actual_mob->race, actual_mob->element,
				                    actual_mob->mode, actual_mob->speed, actual_mob->adelay, actual_mob->amotion, actual_mob->dmotion,
				                    actual_mob->dropitem[0].nameid, actual_mob->dropitem[0].p, actual_mob->dropitem[1].nameid, actual_mob->dropitem[1].p,
				                    actual_mob->dropitem[2].nameid, actual_mob->dropitem[2].p, actual_mob->dropitem[3].nameid, actual_mob->dropitem[3].p,
				                    actual_mob->dropitem[4].nameid, actual_mob->dropitem[4].p, actual_mob->dropitem[5].nameid, actual_mob->dropitem[5].p,
				                    actual_mob->dropitem[6].nameid, actual_mob->dropitem[6].p, actual_mob->dropitem[7].nameid, actual_mob->dropitem[7].p,
				                    actual_mob->dropitem[8].nameid, actual_mob->dropitem[8].p, actual_mob->dropitem[9].nameid, actual_mob->dropitem[9].p,
				                    atoi(str[49]), actual_mob->mexpper, // actual_mob->mexp: Not modified
				                    actual_mob->mvpitem[0].nameid, actual_mob->mvpitem[0].p, actual_mob->mvpitem[1].nameid, actual_mob->mvpitem[1].p,
				                    actual_mob->mvpitem[2].nameid, actual_mob->mvpitem[2].p);
			}

			for(j = 0; j < MAX_RANDOMMONSTER; j++)
				mob_db[class].summonper[j] = 0;
			mob_db[class].maxskill = 0;
			mob_db[class].sex = 0;
			mob_db[class].hair = 0;
			mob_db[class].hair_color = 0;
			mob_db[class].weapon = 0;
			mob_db[class].shield = 0;
			mob_db[class].head_top = 0;
			mob_db[class].head_mid = 0;
			mob_db[class].head_buttom = 0;
			mob_db[class].clothes_color = 0;
		}
		fclose(fp);
		printf(CL_GREEN "Loaded: " CL_RESET "'" CL_WHITE "%s" CL_RESET "' read ('" CL_WHITE "%d" CL_RESET "' entrie%s).\n", filename[i], ln, (ln > 1) ? "s" : "");
	}

	// Code to create SQL mob db
	if (create_mob_db_script && mob_db_fp != NULL) {
		fclose(mob_db_fp);
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
static int mob_readdb_mobavail(void) {

	FILE *fp;
	char line[1024];
	int ln = 0;
	int class, j, k;
	char *str[20],*p,*np;

	if ((fp = fopen("db/mob_avail.txt", "r")) == NULL) {
		printf(CL_RED "Error:" CL_RESET " Failed to load db/mob_avail.txt.\n");
		return -1;
	}

	while(fgets(line, sizeof(line), fp)) { // fgets reads until maximum one less than size and add '\0' -> so, it's not necessary to add -1
		if ((line[0] == '/' && line[1] == '/') || line[0] == '\0' || line[0] == '\n' || line[0] == '\r')
			continue;
		// It's not necessary to remove 'carriage return ('\n' or '\r')
		memset(str, 0, sizeof(str));

		for(j = 0, p = line; j < 12; j++) {
			if ((np = strchr(p, ',')) != NULL) {
				str[j] = p;
				*np = 0;
				p = np + 1;
			} else
				str[j] = p;
			}

		if (str[0] == NULL)
			continue;

		class = atoi(str[0]);

		if (class <= 1000 || class >= MAX_MOB_DB)
			continue;
		k = atoi(str[1]);
		if (j > 3 && k > 26 && k < 69)
			k += 3974; // Advanced Class/Baby Class
		if (k >= 0)
			mob_db[class].view_class = k;

		if (mob_db[class].view_class < 27 || mob_db[class].view_class > 4000) {
			mob_db[class].sex = atoi(str[2]);
			mob_db[class].hair = atoi(str[3]);
			mob_db[class].hair_color = atoi(str[4]);
			mob_db[class].weapon = atoi(str[5]);
			mob_db[class].shield = atoi(str[6]);
			mob_db[class].head_top = atoi(str[7]);
			mob_db[class].head_mid = atoi(str[8]);
			mob_db[class].head_buttom = atoi(str[9]);
			mob_db[class].option = atoi(str[10]) & ~0x46;
			mob_db[class].clothes_color = atoi(str[11]);
		} else if (atoi(str[2]) > 0)
			mob_db[class].equip = atoi(str[2]);

		ln++;
	}
	fclose(fp);
	printf(CL_GREEN "Loaded: " CL_RESET "'" CL_WHITE "db/mob_avail.txt" CL_RESET "' read ('" CL_WHITE "%d" CL_RESET "' entrie%s).\n", ln, (ln > 1) ? "s" : "");

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
static int mob_read_randommonster(void) {

	FILE *fp;
	char line[1024];
	char *str[10], *p;
	int i, j;

	const char* mobfile[] = {
		"db/random/mob_deadbranch.txt",
		"db/random/mob_poringbox.txt",
		"db/random/mob_bloodybranch.txt" };

	for(i=0;i<MAX_RANDOMMONSTER;i++){
		mob_db[0].summonper[i] = 1002;
		fp = fopen(mobfile[i],"r");
		if (fp== NULL){
			printf(CL_RED "Error:" CL_RESET " Failed to load %s\n",mobfile[i]);
			return -1;
		}
		while(fgets(line, sizeof(line), fp)) { // fgets reads until maximum one less than size and add '\0' -> so, it's not necessary to add -1
			int class, per;
			if ((line[0] == '/' && line[1] == '/') || line[0] == '\0' || line[0] == '\n' || line[0] == '\r')
				continue;
			// It's not necessary to remove 'carriage return ('\n' or '\r')
			memset(str, 0, sizeof(str));
			for(j=0,p=line;j<3 && p;j++){
				str[j]=p;
				p=strchr(p,',');
				if (p) *p++=0;
			}

			if (str[0] == NULL || str[2] == NULL)
				continue;

			class = atoi(str[0]);
			per=atoi(str[2]);
			if ((class > 1000 && class < MAX_MOB_DB) || class == 0)
				mob_db[class].summonper[i] = per;
		}
		fclose(fp);
		printf(CL_GREEN "Loaded: " CL_RESET "'" CL_WHITE "%s" CL_RESET "' read.\n", mobfile[i]);
	}

	return 0;
}

/*==========================================
 * db/mob_skill_db.txt reading
 *------------------------------------------
 */
static void mob_readskilldb(void) {

	FILE *fp;
	char line[1024], line2[1024];
	int i;

	const struct {
		char str[32];
		int id;
	} cond1[] = { // Do not add conditions if code does not support them
		{ "always",            MSC_ALWAYS            },
		{ "myhpltmaxrate",     MSC_MYHPLTMAXRATE     },
		{ "friendhpltmaxrate", MSC_FRIENDHPLTMAXRATE },
		{ "mystatuson",        MSC_MYSTATUSON        },
		{ "mystatusoff",       MSC_MYSTATUSOFF       },
		{ "friendstatuson",    MSC_FRIENDSTATUSON    },
		{ "friendstatusoff",   MSC_FRIENDSTATUSOFF   },
		{ "attackpcgt",        MSC_ATTACKPCGT        },
		{ "attackpcge",        MSC_ATTACKPCGE        },
		{ "slavelt",           MSC_SLAVELT           },
		{ "slavele",           MSC_SLAVELE           },
		{ "closedattacked",    MSC_CLOSEDATTACKED    },
		{ "longrangeattacked", MSC_LONGRANGEATTACKED },
		{ "skillused",         MSC_SKILLUSED         },
		{ "afterskill",        MSC_AFTERSKILL        },
		{ "casttargeted",      MSC_CASTTARGETED      },
		{ "rudeattacked",      MSC_RUDEATTACKED      },
		{ "masterattacked",    MSC_MASTERATTACKED    },
		{ "masterhpltmaxrate", MSC_MASTERHPLTMAXRATE },
		{ "onspawn",           MSC_SPAWN             },
	}, cond2[] ={
		{ "anybad",    -1           },
		{ "stone",     SC_STONE     },
		{ "freeze",    SC_FREEZE    },
		{ "stun",      SC_STUN      },
		{ "sleep",     SC_SLEEP     },
		{ "poison",    SC_POISON    },
		{ "curse",     SC_CURSE     },
		{ "silence",   SC_SILENCE   },
		{ "confusion", SC_CONFUSION },
		{ "blind",     SC_BLIND     },
		{ "hiding",    SC_HIDING    },
		{ "sight",     SC_SIGHT     },
	}, state[] = {
		{ "any",    -1         },
		{ "idle",   MSS_IDLE   },
		{ "walk",   MSS_WALK   },
		{ "attack", MSS_ATTACK },
		{ "dead",   MSS_DEAD   },
		{ "loot",   MSS_LOOT   },
		{ "chase",  MSS_CHASE  },
	}, target[] = {
		{ "target",  MST_TARGET  },
		{ "self",    MST_SELF    },
		{ "friend",  MST_FRIEND  },
		{ "around5", MST_AROUND5 },
		{ "around6", MST_AROUND6 },
		{ "around7", MST_AROUND7 },
		{ "around8", MST_AROUND8 },
		{ "around1", MST_AROUND1 },
		{ "around2", MST_AROUND2 },
		{ "around3", MST_AROUND3 },
		{ "around4", MST_AROUND4 },
		{ "around",  MST_AROUND  },
	};

	int x;
	char *filename[] = { "db/mob_skill_db.txt", "db/mob_skill_db2.txt" };

	for(x = 0; x < 2; x++) {

		fp = fopen(filename[x], "r");
		if (fp == NULL){
			if (x == 0)
				printf(CL_RED "Error:" CL_RESET " Failed to load %s\n", filename[x]);
			continue;
		}
		memset(line, 0, sizeof(line));
		while(fgets(line, sizeof(line), fp)) { // fgets reads until maximum one less than size and add '\0' -> so, it's not necessary to add -1
			char *sp[20], *p;
			int mob_id;
			struct mob_skill *ms;
			int j = 0;

			if ((line[0] == '/' && line[1] == '/') || line[0] == '\0' || line[0] == '\n' || line[0] == '\r')
				continue;
			// Remove carriage return if exist
			while(line[0] != '\0' && (line[(i = strlen(line) - 1)] == '\n' || line[i] == '\r'))
				line[i] = '\0';

			strcpy(line2, line); // To display right error

			memset(sp, 0, sizeof(sp));
			for(i = 0, p = line; i < 18 && p; i++) {
				sp[i] = p;
				if ((p = strchr(p,',')) != NULL)
					*p++ = 0;
			}
			if ((mob_id = atoi(sp[0])) <= 0)
				continue;

			if (strcmp(sp[1], "clear") == 0) {
				memset(mob_db[mob_id].skill, 0, sizeof(mob_db[mob_id].skill));
				mob_db[mob_id].maxskill = 0;
				continue;
			}

			for(i = 0; i < MAX_MOBSKILL; i++)
				if ((ms = &mob_db[mob_id].skill[i])->skill_id == 0)
					break;
			if (i == MAX_MOBSKILL) {
				printf("mob_skill: readdb: too many skill ! [%s] in %d[%s]\n",
					sp[1], mob_id, mob_db[mob_id].jname);
				continue;
			}

			ms->state = atoi(sp[2]);
			for(j = 0; j < sizeof(state) / sizeof(state[0]); j++) {
				if (strcmp(sp[2], state[j].str) == 0) {
					ms->state = state[j].id;
					break;
				}
			}
			if (j == sizeof(state) / sizeof(state[0]) && sp[2][0] != '\0' && (sp[2][0] < '0' || sp[2][0] > '9')) {
				printf("DB (%s): " CL_RED "Unknown state '%s' line:" CL_RESET "\n%s\n", filename[x], sp[2], line2);
			}
			ms->skill_id = atoi(sp[3]);
			j = atoi(sp[4]);
			if (j <= 0 || j > MAX_SKILL_LEVEL)
				continue;
			ms->skill_lv = j;
			ms->permillage = atoi(sp[5]);
			ms->casttime = atoi(sp[6]);
			ms->delay = atoi(sp[7]);
			ms->cancel = atoi(sp[8]);
			if (strcmp(sp[8], "yes") == 0)
				ms->cancel = 1;
			ms->target = atoi(sp[9]);
			for(j = 0; j < sizeof(target) / sizeof(target[0]); j++) {
				if (strcmp(sp[9], target[j].str) == 0) {
					ms->target = target[j].id;
					break;
				}
			}
			if (j == sizeof(target) / sizeof(target[0]) && sp[9][0] != '\0' && (sp[9][0] < '0' || sp[9][0] > '9')) {
				printf("DB (%s): " CL_RED "Unknown target '%s' line:" CL_RESET "\n%s\n", filename[x], sp[9], line2);
			}
			ms->cond1 = -1;
			for(j = 0; j < sizeof(cond1) / sizeof(cond1[0]); j++) {
				if (strcmp(sp[10], cond1[j].str) == 0) {
					ms->cond1 = cond1[j].id;
					break;
				}
			}
			if (j == sizeof(cond1) / sizeof(cond1[0]) && sp[10][0] != '\0' && (sp[10][0] < '0' || sp[10][0] > '9')) {
				printf("DB (%s): " CL_RED "Unknown condition 1 '%s' line:" CL_RESET "\n%s\n", filename[x], sp[10], line2);
			}
			ms->cond2 = atoi(sp[11]);
			for(j = 0; j < sizeof(cond2) / sizeof(cond2[0]); j++) {
				if (strcmp(sp[11], cond2[j].str) == 0) {
					ms->cond2 = cond2[j].id;
					break;
				}
			}
			if (j == sizeof(cond2) / sizeof(cond2[0]) && sp[11][0] != '\0' && (sp[11][0] < '0' || sp[11][0] > '9')) {
				printf("DB (%s): " CL_RED "Unknown condition 2 '%s' line:" CL_RESET "\n%s\n", filename[x], sp[11], line2);
			}
			ms->val[0] = atoi(sp[12]);
			ms->val[1] = atoi(sp[13]);
			ms->val[2] = atoi(sp[14]);
			ms->val[3] = atoi(sp[15]);
			ms->val[4] = atoi(sp[16]);
			if (sp[17] != NULL && strlen(sp[17]) > 2)
				ms->emotion = atoi(sp[17]);
			else
				ms->emotion = -1;
			mob_db[mob_id].maxskill = i + 1;

			memset(line, 0, sizeof(line));
		}
		fclose(fp);
		printf(CL_GREEN "Loaded: " CL_RESET "'" CL_WHITE "%s" CL_RESET "' read.\n", filename[x]);
	}

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
static void mob_readdb_race(void) {

	FILE *fp;
	char line[1024];
	int race, j, k;
	char *str[20], *p, *np;

	if ((fp = fopen("db/mob_race2_db.txt", "r")) == NULL) {
		printf(CL_RED "Error:" CL_RESET " Failed to load db/mob_race2_db.txt\n");
		return;
	}

	while(fgets(line, sizeof(line), fp)) { // fgets reads until maximum one less than size and add '\0' -> so, it's not necessary to add -1
		if ((line[0] == '/' && line[1] == '/') || line[0] == '\0' || line[0] == '\n' || line[0] == '\r')
			continue;
		// It's not necessary to remove 'carriage return ('\n' or '\r')

		memset(str, 0, sizeof(str));
		for(j = 0, p = line; j < 12; j++) {
			if ((np = strchr(p, ',')) != NULL) {
				str[j] = p;
				*np = 0;
				p = np + 1;
			} else
				str[j] = p;
		}
		if (str[0] == '\0')
			continue;

		race = atoi(str[0]);
		if (race < 0 || race >= MAX_MOB_RACE_DB)
			continue;

		for (j = 1; j < 20; j++) {
			if (!str[j])
				break;
			k = atoi(str[j]);
			if (k < 1000 || k > MAX_MOB_DB)
				continue;
			mob_db[k].race2 = race;
		}
	}
	fclose(fp);
	printf(CL_GREEN "Loaded: " CL_RESET "'" CL_WHITE "db/mob_race2_db.txt" CL_RESET "' read.\n");

	return;
}

void mob_reload(void) {

	do_init_mob();

	return;
}

#ifndef TXT_ONLY
/*==========================================
 * SQL reading
 *------------------------------------------
 */
static int mob_read_sqldb(void) {

	char line[1024];
	int i, class, ln = 0;
	char *str[57], *p, *np;
	double temp_exp;

	memset(mob_db, 0, sizeof(mob_db));

	sql_request("SELECT * FROM `%s`", mob_db_db);
	while(sql_get_row()) {
		sprintf(line, "%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s",
		             sql_get_string( 0), sql_get_string( 1), sql_get_string( 2), sql_get_string( 3), sql_get_string( 4),
		             sql_get_string( 5), sql_get_string( 6), sql_get_string( 7), sql_get_string( 8), sql_get_string( 9),
		             sql_get_string(10), sql_get_string(11), sql_get_string(12), sql_get_string(13), sql_get_string(14),
		             sql_get_string(15), sql_get_string(16), sql_get_string(17), sql_get_string(18), sql_get_string(19),
		             sql_get_string(20), sql_get_string(21), sql_get_string(22), sql_get_string(23), sql_get_string(24),
		             sql_get_string(25), sql_get_string(26), sql_get_string(27), sql_get_string(28), sql_get_string(29),
		             sql_get_string(30), sql_get_string(31), sql_get_string(32), sql_get_string(33), sql_get_string(34),
		             sql_get_string(35), sql_get_string(36), sql_get_string(37), sql_get_string(38), sql_get_string(39),
		             sql_get_string(40), sql_get_string(41), sql_get_string(42), sql_get_string(43), sql_get_string(44),
		             sql_get_string(45), sql_get_string(46), sql_get_string(47), sql_get_string(48), sql_get_string(49),
		             sql_get_string(50), sql_get_string(51), sql_get_string(52), sql_get_string(53), sql_get_string(54),
		             sql_get_string(55), sql_get_string(56));

		for(i = 0, p = line; i < 57; i++) {
			if ((np = strchr(p, ',')) != NULL) {
				str[i] = p;
				*np = 0;
				p = np + 1;
			} else
				str[i] = p;
		}

		class = atoi(str[0]);
		if (class <= 1000 || class >= MAX_MOB_DB)
			continue;

		ln++;
		if (ln % 20 == 19) {
			printf("Reading mob #%d (class id: %d)...\r", ln, class);
			fflush(stdout);
		}

		mob_db[class].view_class = class;
		strncpy(mob_db[class].name, str[1], 24);
		strncpy(mob_db[class].jname, str[2], 24);
		mob_db[class].lv = atoi(str[3]);
		mob_db[class].max_hp = atoi(str[4]);
		mob_db[class].max_sp = atoi(str[5]);

		mob_db[class].base_exp = atoi(str[6]);
		if (mob_db[class].base_exp <= 0)
			mob_db[class].base_exp = 0;
		else {
			temp_exp = ((double)mob_db[class].base_exp) * ((double)battle_config.base_exp_rate) / 100.;
			if (temp_exp > ((double)0x7fffffff) || temp_exp < 0 || ((int)temp_exp) > 0x7fffffff)
				mob_db[class].base_exp = 0x7fffffff;
			else {
				mob_db[class].base_exp = (int)temp_exp;
				if (mob_db[class].base_exp < 1)
					mob_db[class].base_exp = 1;
			}
		}

		mob_db[class].job_exp = atoi(str[7]);
		if (mob_db[class].job_exp <= 0)
			mob_db[class].job_exp = 0;
		else {
			temp_exp = ((double)mob_db[class].job_exp) * ((double)battle_config.job_exp_rate) / 100.;
			if (temp_exp > ((double)0x7fffffff) || temp_exp < 0 || ((int)temp_exp) > 0x7fffffff)
				mob_db[class].job_exp = 0x7fffffff;
			else {
				mob_db[class].job_exp = (int)temp_exp;
				if (mob_db[class].job_exp < 1)
					mob_db[class].job_exp = 1;
			}
		}

		mob_db[class].range = atoi(str[8]);
		mob_db[class].atk1 = atoi(str[9]);
		mob_db[class].atk2 = atoi(str[10]);
		mob_db[class].def = atoi(str[11]);
		mob_db[class].mdef = atoi(str[12]);
		mob_db[class].str = atoi(str[13]);
		mob_db[class].agi = atoi(str[14]);
		mob_db[class].vit = atoi(str[15]);
		mob_db[class].int_ = atoi(str[16]);
		mob_db[class].dex = atoi(str[17]);
		mob_db[class].luk = atoi(str[18]);
		mob_db[class].range2 = atoi(str[19]);
		mob_db[class].range3 = atoi(str[20]);
		mob_db[class].size = atoi(str[21]);
		mob_db[class].race = atoi(str[22]);
		mob_db[class].element = atoi(str[23]);
		mob_db[class].mode = atoi(str[24]);
		mob_db[class].speed = atoi(str[25]);
		mob_db[class].adelay = atoi(str[26]);
		mob_db[class].amotion = atoi(str[27]);
		mob_db[class].dmotion = atoi(str[28]);

		for(i = 0; i < 10; i++) {
			int nameid, percent;
			nameid = atoi(str[29+i*2]);
			if (nameid == 0) // nameid = 0, it's for no item
				continue;
			else if (nameid < 0 || itemdb_search(nameid) == NULL)
				printf("drop item error: unknown item id %d (%d: %s).\n", nameid, class, mob_db[class].name);
			else if (atoi(str[30+i*2]) > 0) { // nameid > 0 &&
				mob_db[class].dropitem[i].nameid = nameid;
				percent = atoi(str[30+i*2]);
				if (percent < 0) {
					printf("drop item error: item id %d, invalid percent: %d -> 0 (%d: %s).\n", nameid, percent, class, mob_db[class].name);
					percent = 0;
				} else if (percent > 10000) { // 100%
					printf("drop item warning: item id %d, too big percent: %d -> 10000 (%d: %s).\n", nameid, percent, class, mob_db[class].name);
					percent = 10000;
				}
				mob_db[class].dropitem[i].p = percent;
			}
		}

		mob_db[class].mexp = atoi(str[49]) * battle_config.mvp_exp_rate / 100;
		mob_db[class].mexpper = atoi(str[50]);

		for(i = 0; i < 3; i++) {
			int nameid, percent;
			nameid = atoi(str[51+i*2]);
			if (nameid == 0) // nameid = 0, it's for no item
				continue;
			else if (nameid < 0 || itemdb_search(nameid) == NULL)
				printf("mvp item error: unknown item id %d (%d: %s).\n", nameid, class, mob_db[class].name);
			else if (atoi(str[52+i*2]) > 0) { // nameid > 0 &&
				mob_db[class].mvpitem[i].nameid = nameid;
				percent = atoi(str[52+i*2]);
				if (percent < 0) {
					printf("mvp item error: item id %d, invalid percent: %d -> 0 (%d: %s).\n", nameid, percent, class, mob_db[class].name);
					percent = 0;
				} else if (percent > 10000) { // 100%
					printf("mvp item warning: item id %d, too big percent: %d -> 10000 (%d: %s).\n", nameid, percent, class, mob_db[class].name);
					percent = 10000;
				}
				mob_db[class].mvpitem[i].p = percent;
			}
		}

		for(i = 0; i < MAX_RANDOMMONSTER; i++)
			mob_db[class].summonper[i] = 0;
		mob_db[class].maxskill = 0;

		mob_db[class].sex = 0;
		mob_db[class].hair = 0;
		mob_db[class].hair_color = 0;
		mob_db[class].weapon = 0;
		mob_db[class].shield = 0;
		mob_db[class].head_top = 0;
		mob_db[class].head_mid = 0;
		mob_db[class].head_buttom = 0;
	}
	printf(CL_GREEN "Loaded: " CL_RESET "'" CL_WHITE "%s" CL_RESET "' read ('" CL_WHITE "%d" CL_RESET "' entrie%s).\n", mob_db_db, ln, (ln > 1) ? "s" : "");

	return 0;
}
#endif /* Not TXT_ONLY */

/*==========================================
 * Circumference initialization of mob
 *------------------------------------------
 */
int do_init_mob(void)
{
#ifndef TXT_ONLY
	if (db_use_sqldbs)
		mob_read_sqldb();
	else
#endif /* Not TXT_ONLY */
	mob_readdb();

	mob_readdb_mobavail();
	mob_read_randommonster();
	mob_readskilldb();
	mob_readdb_race();

	add_timer_func_list(mob_timer, "mob_timer");
	add_timer_func_list(mob_delayspawn, "mob_delayspawn");
	add_timer_func_list(mob_delay_item_drop, "mob_delay_item_drop");
	add_timer_func_list(mob_delay_item_drop2, "mob_delay_item_drop2"); // for looted items
	add_timer_func_list(mob_ai_hard, "mob_ai_hard");
	add_timer_func_list(mob_ai_lazy, "mob_ai_lazy");
	add_timer_func_list(mobskill_castend_pos, "mobskill_castend_pos");
	add_timer_func_list(mobskill_castend_id, "mobskill_castend_id");
	add_timer_func_list(mob_timer_delete, "mob_timer_delete");
	add_timer_interval(gettick_cache + MIN_MOBTHINKTIME, mob_ai_hard, 0, 0, MIN_MOBTHINKTIME);
	add_timer_interval(gettick_cache + MIN_MOBTHINKTIME * 10, mob_ai_lazy, 0, 0, MIN_MOBTHINKTIME * 10);

	return 0;
}
