// Copyright (c) Freya Development Team - Licensed under GNU GPL
// For more information, see License.txt in the main folder

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <math.h>
#include <locale.h>
#ifdef __WIN32
#define __USE_W32_SOCKETS
#include <windows.h>
#else
#include <netdb.h>
#endif

#include "../common/core.h"
#include "../common/timer.h"
#include "../common/db.h"
#include "../common/malloc.h"
#include "../common/socket.h"
#include "../common/addons.h"
#include "../common/console.h"
#include "../common/version.h"
#include "grfio.h"
#include "map.h"
#include "chrif.h"
#include "clif.h"
#include "intif.h"
#include "npc.h"
#include "pc.h"
#include "mob.h"
#include "chat.h"
#include "itemdb.h"
#include "storage.h"
#include "skill.h"
#include "trade.h"
#include "party.h"
#include "battle.h"
#include "script.h"
#include "guild.h"
#include "pet.h"
#include "atcommand.h"
#include "nullpo.h"
#include "status.h"

#ifdef MEMWATCH
#include "memwatch.h"
#endif

#ifndef TXT_ONLY

int db_use_sqldbs = 0;

char char_db[32] = "char";

unsigned char optimize_table = 0;

#endif /* not TXT_ONLY */

char item_db_db[32] = "item_db"; // need in TXT too to generate SQL DB script (not change the default name in this case)
char mob_db_db[32] = "mob_db"; // need in TXT too to generate SQL DB script (not change the default name in this case)

char create_item_db_script = 0; // generate a script file to create SQL item_db (0: no, 1: yes)
char create_mob_db_script = 0; // generate a script file to create SQL mob_db (0: no, 1: yes)

static struct dbt * id_db;
static struct dbt * map_db;
static struct dbt * charid_db;

static int users;
static struct block_list *object[MAX_FLOORITEM];
static int first_free_object_id, last_object_id;

#define block_free_max 1048576
static void *block_free[block_free_max];
static int block_free_count = 0, block_free_lock = 0;

#define BL_LIST_MAX 1048576
static struct block_list *bl_list[BL_LIST_MAX];
static int bl_list_count = 0;

struct map_data *map = NULL;
int map_num = 0;

int map_port;

int autosave_interval = DEFAULT_AUTOSAVE_INTERVAL;
unsigned char agit_flag = 0; // 0: WoE not starting, Woe is running
unsigned char night_flag = 0; // 0=day, 1=night

struct charid2nick {
	char nick[25]; // 24 + NULL
	int req_id;
};

int map_read_flag = READ_FROM_BITMAP_COMPRESSED;
char map_cache_file[1024] = "db/map.info";

char motd_txt[1024] = "conf/motd.txt";
char help_txt[1024] = "conf/help.txt";
char extra_add_file_txt[1024] = "map_extra_add.txt"; // to add items from external software (use append to add a line)

char wisp_server_name[25] = "Server"; // can be modified in char-server configuration file
int server_char_id; // char id used by server
int server_fake_mob_id; // mob id of a invisible mod to check bots
char npc_language[6] = "en_US"; // Language used by word filter

int console = 0;
char console_pass[1024] = "consoleon"; /* password to enable console */

/*==========================================
 *
 *------------------------------------------
 */
void map_setusers(int n) {

	users = n;
}

/*==========================================
 *
 *------------------------------------------
 */
int map_getusers(void) {

	return users;
}

/*==========================================
 *
 *------------------------------------------
 */
int map_freeblock(void *bl) {

	if (block_free_lock == 0) {
		FREE(bl);
	} else {
		if (block_free_count >= block_free_max) {
			if (battle_config.error_log)
				printf("map_freeblock: *WARNING* too many free block! %d %d\n", block_free_count, block_free_lock);
		} else
			block_free[block_free_count++] = bl;
	}

	return block_free_lock;
}

/*==========================================
 *
 *------------------------------------------
 */
void map_freeblock_lock(void) {

	++block_free_lock;

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void map_freeblock_unlock(void) {

	int i;

	if ((--block_free_lock) == 0) {
		for(i = 0; i < block_free_count; i++) {
			FREE(block_free[i]);
		}
		block_free_count = 0;
	} else if (block_free_lock < 0){
		printf("map_freeblock_unlock: lock count < 0 !\n");
	}

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
static struct block_list bl_head;

/*==========================================
 *
 *------------------------------------------
 */
int map_addblock(struct block_list *bl) {

	int m, x, y;
	int b;

	nullpo_retr(0, bl);

	if (bl->prev != NULL) {
		if (battle_config.error_log)
			printf("map_addblock error : bl->prev!=NULL\n");
		return 0;
	}

	m = bl->m;
	x = bl->x;
	y = bl->y;
	if (m < 0 || m >= map_num ||
	    x < 0 || x >= map[m].xs ||
	    y < 0 || y >= map[m].ys)
		return 1;

	b = x / BLOCK_SIZE + (y / BLOCK_SIZE) * map[m].bxs;
	if (bl->type == BL_MOB) {
		bl->next = map[m].block_mob[b];
		bl->prev = &bl_head;
		if (bl->next)
			bl->next->prev = bl;
		map[m].block_mob[b] = bl;
	} else {
		bl->next = map[m].block[b];
		bl->prev = &bl_head;
		if (bl->next)
			bl->next->prev = bl;
		map[m].block[b] = bl;
		if (bl->type == BL_PC)
			map[m].users++;
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int map_delblock(struct block_list *bl) {

	nullpo_retr(0, bl);

	if (bl->prev == NULL) {
		if (bl->next != NULL) {
			if (battle_config.error_log)
				printf("map_delblock error : bl->next!=NULL\n");
		}
		return 0;
	}

	if (bl->type == BL_PC)
		map[bl->m].users--;
	if (bl->next)
		bl->next->prev = bl->prev;
	if (bl->prev == &bl_head) {
		if (bl->type == BL_MOB){
			map[bl->m].block_mob[bl->x / BLOCK_SIZE + (bl->y / BLOCK_SIZE) * map[bl->m].bxs] = bl->next;
		} else {
			map[bl->m].block[bl->x / BLOCK_SIZE + (bl->y / BLOCK_SIZE) * map[bl->m].bxs] = bl->next;
		}
	} else {
		bl->prev->next = bl->next;
	}
	bl->next = NULL;
	bl->prev = NULL;

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int map_count_oncell(int m, int x, int y) {

	int b;
	struct block_list *bl;
	int count = 0;

	if (x < 0 || y < 0 || (x >= map[m].xs) || (y >= map[m].ys))
		return 1;

	b = x / BLOCK_SIZE + (y / BLOCK_SIZE) * map[m].bxs;

	for(bl = map[m].block[b]; bl; bl = bl->next)
		if (bl->x == x && bl->y == y && bl->type == BL_PC)
			count++;
	for(bl = map[m].block_mob[b]; bl; bl = bl->next)
		if (bl->x == x && bl->y == y)
			count++;

	if (!count)
		return 1;

	return count;
}

/*==========================================
 *
 *------------------------------------------
 */
struct skill_unit *map_find_skill_unit_oncell(struct block_list *target, int x, int y, int skill_id, struct skill_unit *out_unit) {

	int m, b;
	struct block_list *bl;
	struct skill_unit *unit;

	m = target->m;
	if (x < 0 || y < 0 || (x >= map[m].xs) || (y >= map[m].ys))
		return NULL;

	b = x / BLOCK_SIZE + (y / BLOCK_SIZE) * map[m].bxs;

	for(bl = map[m].block[b]; bl; bl = bl->next){
		if (bl->x != x || bl->y != y || bl->type != BL_SKILL)
			continue;
		unit = (struct skill_unit *)bl;
		if (unit == out_unit || !unit->alive ||
		    !unit->group || unit->group->skill_id != skill_id)
			continue;
		if (battle_check_target(&unit->bl, target, unit->group->target_flag) > 0)
			return unit;
	}

	return NULL;
}

/*==========================================
 *
 *------------------------------------------
 */
void map_foreachinarea(int (*func)(struct block_list*, va_list), int m, int x0, int y_0, int x1, int y_1, int type, ...) {

	struct map_data *md;
	int bx, by;
	struct block_list *bl;
	va_list ap;
	int blockcount = bl_list_count, i;

	if (m < 0)
		return;

	va_start(ap, type);

	md = &map[m];

	if (x0  < 0) x0  = 0;
	if (y_0 < 0) y_0 = 0;
	if (x1  >= md->xs) x1  = md->xs - 1;
	if (y_1 >= md->ys) y_1 = md->ys - 1;

	// any blocks
	if (type == 0) {
		for (by = y_0 / BLOCK_SIZE; by <= y_1 / BLOCK_SIZE; by++) {
			for(bx = x0 / BLOCK_SIZE; bx <= x1 / BLOCK_SIZE; bx++) {
				for(bl = md->block[bx + by * md->bxs]; bl; bl = bl->next)
					if (bl->x >= x0 && bl->x <= x1 && bl->y >= y_0 && bl->y <= y_1 && bl_list_count < BL_LIST_MAX)
						bl_list[bl_list_count++] = bl;
				for(bl = md->block_mob[bx + by * md->bxs]; bl; bl = bl->next)
					if (bl->x >= x0 && bl->x <= x1 && bl->y >= y_0 && bl->y <= y_1 && bl_list_count < BL_LIST_MAX)
						bl_list[bl_list_count++] = bl;
			}
		}
	// only monsters
	} else if (type == BL_MOB) {
		for(by = y_0 / BLOCK_SIZE; by <= y_1 / BLOCK_SIZE; by++) {
			for(bx= x0 / BLOCK_SIZE; bx <= x1 / BLOCK_SIZE; bx++) {
				for(bl = md->block_mob[bx + by * md->bxs]; bl; bl = bl->next)
					if (bl->x >= x0 && bl->x <= x1 && bl->y >= y_0 && bl->y <= y_1 && bl_list_count < BL_LIST_MAX)
						bl_list[bl_list_count++] = bl;
			}
		}
	// only 1 type, except monsters
	} else {
		for (by = y_0 / BLOCK_SIZE; by <= y_1 / BLOCK_SIZE; by++) {
			for(bx = x0 / BLOCK_SIZE; bx <= x1 / BLOCK_SIZE; bx++) {
				for(bl = md->block[bx + by * md->bxs]; bl; bl = bl->next) {
					if (bl->type != type)
						continue;
					if (bl->x >= x0 && bl->x <= x1 && bl->y >= y_0 && bl->y <= y_1 && bl_list_count < BL_LIST_MAX)
						bl_list[bl_list_count++] = bl;
				}
			}
		}
	}

	map_freeblock_lock();

	for(i = blockcount; i < bl_list_count; i++)
		if (bl_list[i]->prev)
			func(bl_list[i], ap);

	map_freeblock_unlock();

	va_end(ap);
	bl_list_count = blockcount;
}

/*==========================================
 *
 *------------------------------------------
 */
void map_foreachinmovearea(int (*func)(struct block_list*,va_list), int m, int x0, int y_0, int x1, int y_1, int dx, int dy, int type, ...) {

	struct map_data *md;
	int bx, by;
	struct block_list *bl;
	va_list ap;
	int blockcount = bl_list_count, i;

	va_start(ap, type);

	md = &map[m];

	if (dx == 0 || dy == 0) {
		if (dx == 0) {
			if (dy < 0) {
				y_0 = y_1 + dy + 1;
			} else {
				y_1 = y_0 + dy - 1;
			}
		} else if (dy == 0) {
			if (dx < 0) {
				x0 = x1 + dx + 1;
			} else {
				x1 = x0 + dx - 1;
			}
		}
		if (x0  < 0) x0  = 0;
		if (y_0 < 0) y_0 = 0;
		if (x1  >= md->xs) x1  = md->xs - 1;
		if (y_1 >= md->ys) y_1 = md->ys - 1;

		// any blocks
		if (type == 0) {
			for(by = y_0 / BLOCK_SIZE; by <= y_1 / BLOCK_SIZE; by++) {
				for(bx = x0 / BLOCK_SIZE; bx <= x1 / BLOCK_SIZE; bx++) {
					for(bl = md->block[bx + by * md->bxs]; bl; bl = bl->next)
						if (bl->x >= x0 && bl->x <= x1 && bl->y >= y_0 && bl->y <= y_1 && bl_list_count < BL_LIST_MAX)
							bl_list[bl_list_count++] = bl;
					for(bl = md->block_mob[bx + by * md->bxs];  bl; bl = bl->next)
						if (bl->x >= x0 && bl->x <= x1 && bl->y >= y_0 && bl->y <= y_1 && bl_list_count < BL_LIST_MAX)
							bl_list[bl_list_count++] = bl;
				}
			}
		// only monsters
		} else if (type == BL_MOB) {
			for(by = y_0 / BLOCK_SIZE; by <= y_1 / BLOCK_SIZE; by++) {
				for(bx = x0 / BLOCK_SIZE; bx <= x1 / BLOCK_SIZE; bx++) {
					for(bl = md->block_mob[bx + by * md->bxs];  bl; bl = bl->next)
						if (bl->x >= x0 && bl->x <= x1 && bl->y >= y_0 && bl->y <= y_1 && bl_list_count < BL_LIST_MAX)
							bl_list[bl_list_count++] = bl;
				}
			}
		// only 1 type, except monsters
		} else {
			for(by = y_0 / BLOCK_SIZE; by <= y_1 / BLOCK_SIZE; by++) {
				for(bx = x0 / BLOCK_SIZE; bx <= x1 / BLOCK_SIZE; bx++) {
					for(bl = md->block[bx + by * md->bxs]; bl; bl = bl->next) {
						if (bl->type != type)
							continue;
						if (bl->x >= x0 && bl->x <= x1 && bl->y >= y_0 && bl->y <= y_1 && bl_list_count < BL_LIST_MAX)
							bl_list[bl_list_count++] = bl;
					}
				}
			}
		}
	} else {
		if (x0  < 0) x0  = 0;
		if (y_0 < 0) y_0 = 0;
		if (x1  >= md->xs) x1  = md->xs - 1;
		if (y_1 >= md->ys) y_1 = md->ys - 1;

		// any blocks
		if (type == 0) {
			for(by = y_0 / BLOCK_SIZE; by <= y_1 / BLOCK_SIZE; by++) {
				for(bx = x0 / BLOCK_SIZE; bx <= x1 / BLOCK_SIZE; bx++) {
					for(bl = md->block[bx + by * md->bxs]; bl; bl = bl->next) {
						if (bl->x < x0 || bl->x > x1 || bl->y < y_0 || bl->y > y_1)
							continue;
						if (((dx > 0 && bl->x < x0  + dx) || (dx < 0 && bl->x > x1  + dx) ||
						     (dy > 0 && bl->y < y_0 + dy) || (dy < 0 && bl->y > y_1 + dy)) &&
						    bl_list_count < BL_LIST_MAX)
							bl_list[bl_list_count++] = bl;
					}
					for(bl = md->block_mob[bx + by * md->bxs]; bl; bl = bl->next) {
						if (bl->x < x0 || bl->x > x1 || bl->y < y_0 || bl->y > y_1)
							continue;
						if (((dx > 0 && bl->x < x0  + dx) || (dx < 0 && bl->x > x1  + dx) ||
						     (dy > 0 && bl->y < y_0 + dy) || (dy < 0 && bl->y > y_1 + dy)) &&
						    bl_list_count < BL_LIST_MAX)
							bl_list[bl_list_count++] = bl;
					}
				}
			}
		// only monsters
		} else if (type == BL_MOB) {
			for(by = y_0 / BLOCK_SIZE; by <= y_1 / BLOCK_SIZE; by++) {
				for(bx = x0 / BLOCK_SIZE; bx <= x1 / BLOCK_SIZE; bx++) {
					for(bl = md->block_mob[bx + by * md->bxs]; bl; bl = bl->next) {
						if (bl->x < x0 || bl->x > x1 || bl->y < y_0 || bl->y > y_1)
							continue;
						if (((dx > 0 && bl->x < x0  + dx) || (dx < 0 && bl->x > x1  + dx) ||
						     (dy > 0 && bl->y < y_0 + dy) || (dy < 0 && bl->y > y_1 + dy)) &&
						    bl_list_count < BL_LIST_MAX)
							bl_list[bl_list_count++] = bl;
					}
				}
			}
		// only 1 type, except monsters
		} else {
			for(by = y_0 / BLOCK_SIZE; by <= y_1 / BLOCK_SIZE; by++) {
				for(bx = x0 / BLOCK_SIZE; bx <= x1 / BLOCK_SIZE; bx++) {
					for(bl = md->block[bx + by * md->bxs]; bl; bl = bl->next) {
						if (bl->type != type)
							continue;
						if (bl->x < x0 || bl->x > x1 || bl->y < y_0 || bl->y > y_1)
							continue;
						if (((dx > 0 && bl->x < x0  + dx) || (dx < 0 && bl->x > x1  + dx) ||
						     (dy > 0 && bl->y < y_0 + dy) || (dy < 0 && bl->y > y_1 + dy)) &&
						    bl_list_count < BL_LIST_MAX)
							bl_list[bl_list_count++] = bl;
					}
				}
			}
		}
	}

	map_freeblock_lock();

	for(i = blockcount; i < bl_list_count; i++)
		if (bl_list[i]->prev)
			func(bl_list[i], ap);

	map_freeblock_unlock();

	va_end(ap);
	bl_list_count = blockcount;
}

/*==========================================
 *
 *------------------------------------------
 */
void map_foreachinpath(int (*func)(struct block_list*, va_list), int m, int x0, int y_0, int x1, int y_1, int range, int type, ...) {

	va_list ap;
	int i, blockcount = bl_list_count;
	struct block_list *bl;

	//////////////////////////////////////////////////////////////
	// linear parametric equation
	// x = (x1 - x0) * t + x0; y = (y_1 - y_0) * t + y_0; t = [0,1]
	//////////////////////////////////////////////////////////////
	// linear equation for finding a single line between (x0,y_0)->(x1,y_1)
	// independent of the given xy-values
	double dx, dy;
	int bx, by;

	int t, tmax;

	if (x0  < 0) x0  = 0;
	if (y_0 < 0) y_0 = 0;
	if (x1  >= map[m].xs) x1  = map[m].xs - 1;
	if (y_1 >= map[m].ys) y_1 = map[m].ys - 1;

	///////////////////////////////
	// find maximum runindex
	tmax = abs(y_1 - y_0);
	if (tmax  < abs(x1 - x0))
		tmax = abs(x1 - x0);
	// pre-calculate delta values for x and y destination
	// should speed up cause you don't need to divide in the loop
	dx = 0.0;
	dy = 0.0;
	if (tmax > 0) {
		dx = ((double)(x1  - x0 )) / ((double)tmax);
		dy = ((double)(y_1 - y_0)) / ((double)tmax);
	}

	// go along the index
	bx = -1; // initialize block coords to some impossible value
	by = -1;
	for(t = 0; t <= tmax; t++) { // xy-values of the line including start and end point
		int x = (int)floor(dx * (double)t + 0.5) + x0;
		int y = (int)floor(dy * (double)t + 0.5) + y_0;

		// check the block index of the calculated xy
		if ((bx != x / BLOCK_SIZE) || (by != y / BLOCK_SIZE)) {
			// we have reached a new block
			// so we store the current block coordinates
			bx = x / BLOCK_SIZE;
			by = y / BLOCK_SIZE;
			if (type == 0) {
				for(bl = map[m].block[bx + by * map[m].bxs]; bl; bl = bl->next) {
					if (bl_list_count < BL_LIST_MAX) {
						// check if block xy is on the line
						if ((bl->x - x0) * (y_1 - y_0) == (bl->y - y_0) * (x1 - x0))
							// and if it is within start and end point
							if ((((x0  <= x1)  && (x0  <= bl->x) && (bl->x <= x1))  || ((x0  >= x1)  && (x0  >= bl->x) && (bl->x >= x1))) &&
							    (((y_0 <= y_1) && (y_0 <= bl->y) && (bl->y <= y_1)) || ((y_0 >= y_1) && (y_0 >= bl->y) && (bl->y >= y_1))))
								bl_list[bl_list_count++] = bl;
					}
				}
				for(bl = map[m].block_mob[bx + by * map[m].bxs]; bl; bl = bl->next) {
					if (bl_list_count < BL_LIST_MAX) {
						// check if block xy is on the line
						if ((bl->x - x0) * (y_1 - y_0) == (bl->y - y_0) * (x1 - x0))
							// and if it is within start and end point
							if ((((x0  <= x1)  && (x0  <= bl->x) && (bl->x <= x1))  || ((x0  >= x1)  && (x0  >= bl->x) && (bl->x >= x1))) &&
							    (((y_0 <= y_1) && (y_0 <= bl->y) && (bl->y <= y_1)) || ((y_0 >= y_1) && (y_0 >= bl->y) && (bl->y >= y_1))))
								bl_list[bl_list_count++] = bl;
					}
				}
			} else if (type != BL_MOB) {
				for(bl = map[m].block[bx + by * map[m].bxs]; bl; bl = bl->next) {
					if (bl_list_count < BL_LIST_MAX) {
						// check if block xy is on the line
						if ((bl->x - x0) * (y_1 - y_0) == (bl->y - y_0) * (x1 - x0))
							// and if it is within start and end point
							if ((((x0  <= x1)  && (x0  <= bl->x) && (bl->x <= x1))  || ((x0  >= x1)  && (x0  >= bl->x) && (bl->x >= x1))) &&
							    (((y_0 <= y_1) && (y_0 <= bl->y) && (bl->y <= y_1)) || ((y_0 >= y_1) && (y_0 >= bl->y) && (bl->y >= y_1))))
								bl_list[bl_list_count++] = bl;
					}
				}
			} else /*if (type == BL_MOB)*/ {
				for(bl = map[m].block_mob[bx + by * map[m].bxs]; bl; bl = bl->next) {
					if (bl_list_count < BL_LIST_MAX) {
						// check if block xy is on the line
						if ((bl->x - x0) * (y_1 - y_0) == (bl->y - y_0) * (x1 - x0))
							// and if it is within start and end point
							if ((((x0  <= x1)  && (x0  <= bl->x) && (bl->x <= x1))  || ((x0  >= x1)  && (x0  >= bl->x) && (bl->x >= x1))) &&
							    (((y_0 <= y_1) && (y_0 <= bl->y) && (bl->y <= y_1)) || ((y_0 >= y_1) && (y_0 >= bl->y) && (bl->y >= y_1))))
								bl_list[bl_list_count++] = bl;
					}
				}
			}
		}
	}

	va_start(ap,type);
	map_freeblock_lock();

	for(i = blockcount; i < bl_list_count; i++)
		if (bl_list[i]->prev)
			func(bl_list[i], ap);

	map_freeblock_unlock();

	va_end(ap);
	bl_list_count = blockcount;
}

/*==========================================
 *
 *------------------------------------------
 */
int map_addobject(struct block_list *bl) {

	int i;

	if (bl == NULL) {
		printf("map_addobject nullpo?\n");
		return 0;
	}
	if (first_free_object_id < 2 || first_free_object_id >= MAX_FLOORITEM)
		first_free_object_id = 2;
	for(i = first_free_object_id; i < MAX_FLOORITEM; i++)
		if (object[i] == NULL)
			break;
	if (i >= MAX_FLOORITEM) {
		if (battle_config.error_log)
			printf("no free object id\n");
		return 0;
	}
	first_free_object_id = i;
	if (last_object_id < i)
		last_object_id = i;
	object[i] = bl;
	numdb_insert(id_db, i, bl);

	return i;
}

/*==========================================
 *
 *------------------------------------------
 */
int map_delobjectnofree(int id) {

	struct block_list *obj = object[id];

	if (obj == NULL)
		return 0;

	map_delblock(obj);
	numdb_erase(id_db, id);
	// no free block
	object[id] = NULL;

	if (first_free_object_id > id)
		first_free_object_id = id;

	while(last_object_id > 2 && object[last_object_id] == NULL)
		last_object_id--;

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int map_delobject(int id) {

	struct block_list *obj = object[id];

	if (obj == NULL)
		return 0;

	map_delblock(obj);
	numdb_erase(id_db, id);
	map_freeblock(obj); // free block
	object[id] = NULL;

	if (first_free_object_id > id)
		first_free_object_id = id;

	while(last_object_id > 2 && object[last_object_id] == NULL)
		last_object_id--;

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
void map_foreachobject(int (*func)(struct block_list*, va_list), int type, ...) {

	int i;
	int blockcount = bl_list_count;
	va_list ap;

	va_start(ap, type);

	for(i = 2; i <= last_object_id; i++) {
		if (object[i]){
			if (type && object[i]->type != type)
				continue;
			if (bl_list_count >= BL_LIST_MAX) {
				if (battle_config.error_log)
					printf("map_foreachobject: too many block !\n");
			} else
				bl_list[bl_list_count++] = object[i];
		}
	}

	map_freeblock_lock();

	for(i = blockcount; i < bl_list_count; i++)
		if (bl_list[i]->prev)
			func(bl_list[i], ap);

	map_freeblock_unlock();

	va_end(ap);
	bl_list_count = blockcount;
}

/*==========================================
 *
 *------------------------------------------
 */
int map_clearflooritem_timer(int tid, unsigned int tick, int id, int data) {

	struct flooritem_data *fitem;

	fitem = (struct flooritem_data *)object[id];
	if (fitem == NULL || fitem->bl.type != BL_ITEM || (!data && fitem->cleartimer != tid)){
		if (battle_config.error_log)
			printf("map_clearflooritem_timer : error\n");
		return 1;
	}
	if (data)
		delete_timer(fitem->cleartimer, map_clearflooritem_timer);
	else if (fitem->item_data.card[0] == (short)0xff00)
		intif_delete_petdata(*((long *)(&fitem->item_data.card[1])));
	clif_clearflooritem(fitem, 0);
	map_delobject(fitem->bl.id);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int map_searchrandfreecell(int m, int x, int y, int range) {

	int free_cell, i, j;

	for(free_cell = 0, i = -range; i <= range; i++) {
		if (i + y < 0 || i + y >=map[m].ys)
			continue;
		for(j = - range; j <= range; j++){
			if (j + x < 0 || j + x >= map[m].xs)
				continue;
			if (map_getcell(m, j + x, i + y, CELL_CHKNOPASS))
				continue;
			free_cell++;
		}
	}
	if (free_cell==0)
		return -1;
	free_cell=rand()%free_cell;
	for(i=-range;i<=range;i++){
		if (i+y<0 || i+y>=map[m].ys)
			continue;
		for(j=-range;j<=range;j++){
			if (j + x < 0 || j + x >= map[m].xs)
				continue;
			if (map_getcell(m, j + x, i + y, CELL_CHKNOPASS))
				continue;
			if (free_cell == 0) {
				x += j;
				y += i;
				i = range + 1;
				break;
			}
			free_cell--;
		}
	}

	return x + (y << 16);
}

/*==========================================
 *
 *------------------------------------------
 */
int map_addflooritem(struct item *item_data, int amount, int m, int x, int y, struct map_session_data *first_sd,
    struct map_session_data *second_sd, struct map_session_data *third_sd, int owner_id, int type) {

	int xy, r;
	struct flooritem_data *fitem;

	nullpo_retr(0, item_data);

	if ((xy = map_searchrandfreecell(m, x, y, 1)) < 0)
		return 0;
	r = rand();

	CALLOC(fitem, struct flooritem_data, 1);
	fitem->bl.type = BL_ITEM;
	fitem->bl.prev = fitem->bl.next = NULL;
	fitem->bl.m = m;
	fitem->bl.x = xy & 0xffff;
	fitem->bl.y = (xy >> 16) & 0xffff;
	fitem->first_get_id = 0;
	fitem->first_get_tick = 0;
	fitem->second_get_id = 0;
	fitem->second_get_tick = 0;
	fitem->third_get_id = 0;
	fitem->third_get_tick = 0;

	fitem->bl.id = map_addobject(&fitem->bl);
	if (fitem->bl.id == 0) {
		FREE(fitem);
		return 0;
	}

	if (first_sd) {
		fitem->first_get_id = first_sd->bl.id;
		if (type)
			fitem->first_get_tick = gettick_cache + battle_config.mvp_item_first_get_time;
		else
			fitem->first_get_tick = gettick_cache + battle_config.item_first_get_time;
	}
	if (second_sd) {
		fitem->second_get_id = second_sd->bl.id;
		if (type)
			fitem->second_get_tick = gettick_cache + battle_config.mvp_item_first_get_time + battle_config.mvp_item_second_get_time;
		else
			fitem->second_get_tick = gettick_cache + battle_config.item_first_get_time + battle_config.item_second_get_time;
	}
	if (third_sd) {
		fitem->third_get_id = third_sd->bl.id;
		if (type)
			fitem->third_get_tick = gettick_cache + battle_config.mvp_item_first_get_time + battle_config.mvp_item_second_get_time + battle_config.mvp_item_third_get_time;
		else
			fitem->third_get_tick = gettick_cache + battle_config.item_first_get_time + battle_config.item_second_get_time + battle_config.item_third_get_time;
	}
	fitem->owner = owner_id; // who drop the item (to authorize to get item at any moment)

	memcpy(&fitem->item_data, item_data, sizeof(*item_data));
	fitem->item_data.amount = amount;
	fitem->subx = (r & 3) * 3 + 3;
	fitem->suby = ((r >> 2) & 3) * 3 + 3;
	fitem->cleartimer = add_timer(gettick_cache + battle_config.flooritem_lifetime, map_clearflooritem_timer, fitem->bl.id, 0);

	map_addblock(&fitem->bl);
	clif_dropflooritem(fitem);

	return fitem->bl.id;
}

/*==========================================
 *
 *------------------------------------------
 */
void map_addchariddb(int charid, char *name) {

	struct charid2nick *p;
	int req;

	p = numdb_search(charid_db, charid);
	if (p == NULL) {
		CALLOC(p, struct charid2nick, 1);
		p->req_id = 0;
	} else
		numdb_erase(charid_db, charid);

	req = p->req_id;
	memset(p->nick, 0, sizeof(p->nick));
	strncpy(p->nick, name, 24);
	p->req_id = 0;
	numdb_insert(charid_db, charid, p);
	if (req) {
		struct map_session_data *sd = map_id2sd(req);
		if (sd != NULL)
			clif_solved_charname(sd, charid);
	}

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
int map_reqchariddb(struct map_session_data * sd,int charid) {

	struct charid2nick *p;

	nullpo_retr(0, sd);

	p = numdb_search(charid_db, charid);
	if (p!=NULL)
		return 0;
	CALLOC(p, struct charid2nick, 1);
	p->req_id=sd->bl.id;
	numdb_insert(charid_db, charid, p);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
void map_addiddb(struct block_list *bl) {

	nullpo_retv(bl);

	numdb_insert(id_db, bl->id, bl);
}

/*==========================================
 *
 *------------------------------------------
 */
void map_deliddb(struct block_list *bl) {

	nullpo_retv(bl);

	numdb_erase(id_db, bl->id);
}

/*==========================================
 *
 *------------------------------------------
 */
void map_quit(struct map_session_data *sd) {

	nullpo_retv(sd);

	if (sd->state.event_disconnect) {
		struct npc_data *npc;
		if ((npc = npc_name2id(script_config.logout_event_name))) {
			run_script(npc->u.scr.script, 0, sd->bl.id, npc->bl.id);
		}
	}

	if (sd->chatID)
		chat_leavechat(sd);

	if (sd->trade_partner)
		trade_tradecancel(sd);

	if (sd->friend_num > 0)
		clif_friend_send_online(sd, 0);

	if (sd->party_invite > 0)
		party_reply_invite(sd, sd->party_invite_account, 0); // 0: invitation was denied, 1: accepted

	if (sd->guild_invite > 0)
		guild_reply_invite(sd, sd->guild_invite, 0);
	if (sd->guild_alliance > 0)
		guild_reply_reqalliance(sd, sd->guild_alliance_account, 0); // flag: 0 deny, 1:accepted

	party_send_logout(sd);

	guild_send_memberinfoshort(sd, 0);

	pc_cleareventtimer(sd);

	if (sd->state.storage_flag)
		storage_guild_storage_quit(sd);
	else
		storage_storage_quit(sd);

	skill_castcancel(&sd->bl, 0, 0);
	skill_stop_dancing(&sd->bl, 1);

	// Statuses that are not saved...
	if (sd->sc_count) {
		if (sd->sc_data[SC_HIDING].timer != -1)
			status_change_end(&sd->bl, SC_HIDING, -1);
		if (sd->sc_data[SC_CLOAKING].timer != -1)
			status_change_end(&sd->bl, SC_CLOAKING, -1);
		if (sd->sc_data[SC_BLADESTOP].timer != -1)
			status_change_end(&sd->bl, SC_BLADESTOP, -1);
		if (sd->sc_data[SC_RUN].timer != -1)
			status_change_end(&sd->bl, SC_RUN,-1);
		if (sd->sc_data[SC_SPURT].timer != -1)
			status_change_end(&sd->bl, SC_SPURT, -1);
		if (sd->sc_data[SC_CONCENTRATION].timer != -1)
			status_change_end(&sd->bl, SC_CONCENTRATION, -1);
		if (sd->sc_data[SC_BERSERK].timer != -1) {
			status_change_end(&sd->bl, SC_BERSERK, -1);
			sd->status.hp = 100;
		}
		if (sd->sc_data[SC_RIDING].timer != -1)
			status_change_end(&sd->bl, SC_RIDING, -1);
		if (sd->sc_data[SC_FALCON].timer != -1)
			status_change_end(&sd->bl, SC_FALCON, -1);
		if (sd->sc_data[SC_WEIGHT50].timer != -1)
			status_change_end(&sd->bl, SC_WEIGHT50, -1);
		if (sd->sc_data[SC_WEIGHT90].timer != -1)
			status_change_end(&sd->bl, SC_WEIGHT90, -1);
		if (sd->sc_data[SC_BROKENWEAPON].timer != -1)
			status_change_end(&sd->bl, SC_BROKENWEAPON, -1);
		if (sd->sc_data[SC_BROKENARMOR].timer != -1)
			status_change_end(&sd->bl, SC_BROKENARMOR, -1);
		if (sd->sc_data[SC_GUILDAURA].timer != -1)
			status_change_end(&sd->bl, SC_GUILDAURA, -1);
		if (sd->sc_data[SC_GOSPEL].timer != -1)
			status_change_end(&sd->bl, SC_GOSPEL, -1);
		if (sd->sc_data[SC_MARIONETTE].timer != -1) {
			struct block_list *bl = map_id2bl(sd->sc_data[SC_MARIONETTE].val3);
			status_change_end(bl, SC_MARIONETTE2, -1);
		}
	}

	skill_clear_unitgroup(&sd->bl);
	skill_cleartimerskill(&sd->bl);
	
	if (sd->followtimer != -1) {
		delete_timer(sd->followtimer, pc_follow_timer);
		sd->followtimer = -1;
	}
	if (sd->pvp_timer != -1) {
		delete_timer(sd->pvp_timer, pc_calc_pvprank_timer);
		sd->pvp_timer = -1;
	}
	if (sd->jailtimer != -1) {
		delete_timer(sd->jailtimer, pc_jail_timer);
		sd->jailtimer = -1;
	}
	
	pc_stop_walking(sd, 0);
	pc_stopattack(sd);
	pc_delinvincibletimer(sd);
	pc_delspiritball(sd, sd->spiritball, 1);
	skill_gangsterparadise(sd, 0);
	skill_unit_move(&sd->bl, gettick_cache, 0);

	status_calc_pc(sd, 4);

	if (sd->status.option & OPTION_HIDE)
		clif_clearchar_area(&sd->bl, 4); // no display // need to send information to authorize people in area to go in the square
	else
		clif_clearchar_area(&sd->bl, 2); // warp display

#ifdef USE_SQL //TXT version of this is still in dev
	chrif_save_scdata(sd); // save status changes
#endif
	status_change_clear(&sd->bl, 1);
	
	if (sd->status.pet_id > 0 && sd->pd) {
		pet_lootitem_drop(sd->pd, sd);
		if (sd->pet.intimate <= 0) {
			intif_delete_petdata(sd->status.pet_id);
			pet_remove_map(sd);
			sd->status.pet_id = 0;
			sd->petDB = NULL;
		} else {
			intif_save_petdata(sd->status.account_id, &sd->pet);
			pet_remove_map(sd);
			sd->petDB = NULL;
		}
	}

	if (pc_isdead(sd))
		pc_setrestartvalue(sd, 2);
	chrif_save(sd); // do pc_makesavestatus and save storage + account_reg/account_reg2 too

	map_delblock(&sd->bl);

	numdb_erase(id_db, sd->bl.id);

  	{
		void *p = numdb_search(charid_db, sd->status.char_id);
		if (p) {
			numdb_erase(charid_db, sd->status.char_id);
			FREE(p);
		}
  	}

	if (numdb_search(charid_db, sd->status.char_id) != NULL)
		numdb_erase(charid_db, sd->status.char_id);

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void map_quit2(struct map_session_data *sd) {

	nullpo_retv(sd);

	storage_delete(sd->bl.id);

	FREE(sd->npc_stackbuf);
	FREE(sd->ignore);
	FREE(sd->global_reg);
	FREE(sd->account_reg);
	FREE(sd->account_reg2);
	FREE(sd->friends);
	FREE(sd->reg);
	FREE(sd->regstr);

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
struct map_session_data * map_charid2sd(int char_id) {

	int i;
	struct map_session_data *sd;

	for(i = 0; i < fd_max; i++)
		if (session[i] && (sd = session[i]->session_data) && sd->state.auth && sd->status.char_id == char_id)
			return sd;

	return NULL;
}

/*==========================================
 *
 *------------------------------------------
 */
struct map_session_data * map_id2sd(int id) {

	int i;
	struct map_session_data *sd;

	for(i = 0; i < fd_max; i++)
		if (session[i] && (sd = session[i]->session_data) && sd->bl.id == id)
			return sd;

	return NULL;
}

/*==========================================
 *
 *------------------------------------------
 */
char * map_charid2nick(int id) {

	struct charid2nick *p = numdb_search(charid_db, id);

	if (p == NULL)
		return NULL;
	if (p->req_id != 0)
		return NULL;

	return p->nick;
}

/*==========================================
 *
 *------------------------------------------
 */
struct map_session_data * map_nick2sd(char *nick) {

	int i, quantity;
	struct map_session_data *sd;
	struct map_session_data *pl_sd;

	if (nick == NULL)
		return NULL;

	quantity = 0;
	sd = NULL;
	for (i = 0; i < fd_max; i++) {
		if (session[i] && (pl_sd = session[i]->session_data) && pl_sd->state.auth)
			// Without case sensitive check (increase the number of similar character names found)
			if (strcasecmp(pl_sd->status.name, nick) == 0) {
				// Strict comparison (if found, we finish the function immediatly with correct value)
				if (strcmp(pl_sd->status.name, nick) == 0)
					return pl_sd;
				quantity++;
				sd = pl_sd;
			}
	}
	// Here, the exact character name is not found
	// We return the found index of a similar account ONLY if there is 1 similar character
	// and if we are with an unique Map-server
	if (quantity == 1 && map_is_alone)
		return sd;

	// Exact character name is not found and 0 or more than 1 similar characters have been found ==> we say not found
	return NULL;
}

/*==========================================
 *
 *------------------------------------------
 */
struct block_list * map_id2bl(int id) {

	if (id >= 0 && id < sizeof(object) / sizeof(object[0]))
		return object[id];
	else
		return numdb_search(id_db, id);
}

/*==========================================
 *
 *------------------------------------------
 */
void map_foreachiddb(int (*func)(void*,void*,va_list),...) {

	va_list ap;

	va_start(ap, func);
	numdb_foreach(id_db, func, ap);
	va_end(ap);

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
int map_addnpc(int m, struct npc_data *nd) {

	int i;

	if (m < 0 || m >= map_num)
		return -1;
	for(i = 0; i < map[m].npc_num && i < MAX_NPC_PER_MAP; i++)
		if (map[m].npc[i] == NULL)
			break;
	if (i == MAX_NPC_PER_MAP){
		if (battle_config.error_log)
			printf("too many NPCs in one map %s\n", map[m].name);
		return -1;
	}
	if (i == map[m].npc_num)
		map[m].npc_num++;

	nullpo_retr(0, nd);

	map[m].npc[i] = nd;
	nd->n = i;
	numdb_insert(id_db, nd->bl.id, nd);

	return i;
}

/*==========================================
 *
 *------------------------------------------
 */
void map_removenpc(void) {

	int i, m, n = 0;

	for(m = 0; m < map_num; m++) {
		for(i = 0; i < map[m].npc_num && i < MAX_NPC_PER_MAP; i++) {
			if (map[m].npc[i] != NULL) {
				clif_clearchar_area(&map[m].npc[i]->bl, 2);
				map_delblock(&map[m].npc[i]->bl);
				numdb_erase(id_db, map[m].npc[i]->bl.id);
				FREE(map[m].npc[i]);
				n++;
			}
		}
	}
	printf("Successfully removed and freed from memory: '" CL_WHITE "%d" CL_RESET "' NPCs.\n", n);

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
int map_mapname2mapid(const char *name) {

	struct map_data *md;

	if (name == NULL || name[0] == '\0')
		return -1;

	md = strdb_search(map_db, name);

	if (md == NULL || md->gat == NULL)
		return -1;

	return md->m;
}

/*==========================================
 *
 *------------------------------------------
 */
int map_mapname2ipport(const char *name, int *ip, int *port) {

	struct map_data_other_server *mdos;

	mdos = strdb_search(map_db, name);
	if (mdos == NULL || mdos->gat)
		return -1;
	*ip = mdos->ip;
	*port = mdos->port;

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
short map_checkmapname(const char *mapname) {

	int m;
	struct map_data_other_server *mdos;

	if ((m = map_mapname2mapid(mapname)) >= 0)
		return m;

	mdos = strdb_search(map_db, mapname);
	if (mdos == NULL || mdos->gat)
		return -1;

	return -2;
}

/*==========================================
 *
 *------------------------------------------
 */
int map_check_dir(int s_dir,int t_dir) {

	if (s_dir == t_dir)
		return 0;
	switch(s_dir) {
		case 0:
			if (t_dir == 7 || t_dir == 1 || t_dir == 0)
				return 0;
			break;
		case 1:
			if (t_dir == 0 || t_dir == 2 || t_dir == 1)
				return 0;
			break;
		case 2:
			if (t_dir == 1 || t_dir == 3 || t_dir == 2)
				return 0;
			break;
		case 3:
			if (t_dir == 2 || t_dir == 4 || t_dir == 3)
				return 0;
			break;
		case 4:
			if (t_dir == 3 || t_dir == 5 || t_dir == 4)
				return 0;
			break;
		case 5:
			if (t_dir == 4 || t_dir == 6 || t_dir == 5)
				return 0;
			break;
		case 6:
			if (t_dir == 5 || t_dir == 7 || t_dir == 6)
				return 0;
			break;
		case 7:
			if (t_dir == 6 || t_dir == 0 || t_dir == 7)
				return 0;
			break;
	}

	return 1;
}

/*==========================================
 *
 *------------------------------------------
 */
int map_calc_dir(struct block_list *src, int x, int y) {

	int dir = 0;
	int dx, dy;

	nullpo_retr(0, src);

	dx = x - src->x;
	dy = y - src->y;
	if (dx == 0 && dy == 0) {
		dir = 0;
	} else if (dx >= 0 && dy >= 0) {
		dir = 7;
		if (dx * 3 - 1 < dy) dir = 0;
		if (dx > dy * 3) dir = 6;
	} else if (dx >= 0 && dy <= 0) {
		dir = 5;
		if (dx * 3 - 1 < -dy) dir = 4;
		if (dx > -dy * 3) dir = 6;
	} else if (dx <= 0 && dy <= 0) {
		dir = 3;
		if (dx * 3 + 1 > dy) dir = 4;
		if (dx < dy * 3) dir = 2;
	} else {
		dir = 1;
		if (-dx * 3 - 1 < dy) dir = 0;
		if (-dx > dy * 3) dir = 2;
	}

	return dir;
}

/*==========================================
 *
 *------------------------------------------
 */
int map_getcell(int m, int x, int y, cell_t cellchk) {

	return (m < 0 || m >= map_num) ?
	                                 ((cellchk == CELL_CHKNOPASS || cellchk == CELL_CHKNOPASS_NPC) ? 1 : 0) :
	                                 map_getcellp(&map[m], x, y, cellchk);
}

/*==========================================
 *
 *------------------------------------------
 */
int map_getcellp(struct map_data* m, int x, int y, cell_t cellchk) {

	int type;

	nullpo_ret(m);

	if (x < 0 || x >= m->xs - 1 || y < 0 || y >= m->ys - 1) {
		if (cellchk == CELL_CHKNOPASS || cellchk == CELL_CHKNOPASS_NPC)
			return 1;
		return 0;
	}
	type = m->gat[x + y * m->xs];
	if (cellchk < 0x10)
		type &= CELL_MASK;

	switch(cellchk) {
	case CELL_CHKPASS:
		return (type != 1 && type != 5);
	case CELL_CHKNOPASS:
		return (type == 1 || type == 5);
	case CELL_CHKWALL:
		return (type == 1);
	case CELL_CHKWATER:
		return (type == 3);
	case CELL_CHKGROUND:
		return (type == 5);
	case CELL_GETTYPE:
		return type;
	case CELL_CHKNOPASS_NPC:
		return (type == 1 || type == 5 || m->gat[x + y * m->xs] & CELL_NPC);
	case CELL_CHKNPC:
		return (type & CELL_NPC);
	case CELL_CHKBASILICA:
		return (type & CELL_BASILICA);
	default:
		return 0;
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
void map_setcell(int m, int x, int y, int cell) {

	int j;

	if (x < 0 || x >= map[m].xs || y < 0 || y >= map[m].ys)
		return;

	j = x + y * map[m].xs;

	switch (cell) {
	case CELL_SETNPC:
		map[m].gat[j] |= CELL_NPC;
		break;
	case CELL_SETBASILICA:
		map[m].gat[j] |= CELL_BASILICA;
		break;
	case CELL_CLRBASILICA:
		map[m].gat[j] &= ~CELL_BASILICA;
		break;
	default:
		map[m].gat[j] = (map[m].gat[j] & ~CELL_MASK) + cell;
		break;
	}

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
int map_setipport(char *name, unsigned long ip, int port) {

	struct map_data *md;
	struct map_data_other_server *mdos;

	md = strdb_search(map_db, name);
	if (md == NULL) { // not exist -> add new data
		CALLOC(mdos, struct map_data_other_server, 1);
		strncpy(mdos->name, name, 24);
		mdos->gat  = NULL;
		mdos->ip   = ip;
		mdos->port = port;
		mdos->map  = NULL;
		strdb_insert(map_db, mdos->name, mdos);
	} else if (md->gat){
		if (ip != clif_getip() || port != clif_getport()){
			CALLOC(mdos, struct map_data_other_server, 1);
			strncpy(mdos->name, name, 24);
			mdos->gat  = NULL;
			mdos->ip   = ip;
			mdos->port = port;
			mdos->map  = md;
			strdb_insert(map_db, mdos->name, mdos);
		} else {
			;
		}
	} else {
		mdos = (struct map_data_other_server *)md;
		if (ip == clif_getip() && port == clif_getport()) {
			if (mdos->map == NULL) {
				printf("map_setipport : %s is not loaded.\n", name);
				exit(1);
			} else {
				md = mdos->map;
				FREE(mdos);
				strdb_insert(map_db, md->name, md);
			}
		} else {
			mdos->ip   = ip;
			mdos->port = port;
		}
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int map_eraseallipport_sub(void *key, void *data, va_list va) {

	struct map_data_other_server *mdos = (struct map_data_other_server*)data;

	if (mdos->gat == NULL && mdos->map == NULL) {
		strdb_erase(map_db, key);
		FREE(mdos);
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int map_eraseallipport(void) {

	strdb_foreach(map_db, map_eraseallipport_sub);

	return 1;
}

/*==========================================
 *
 *------------------------------------------
 */
int map_eraseipport(char *name, unsigned long ip, int port) {

	struct map_data *md;
	struct map_data_other_server *mdos;

	md=strdb_search(map_db, name);
	if (md) {
		if (md->gat) // local -> check data
			return 0;
		else {
			mdos = (struct map_data_other_server *)md;
			if (mdos->ip == ip && mdos->port == port) {
				if (mdos->map) {
					return 1;
				} else {
					strdb_erase(map_db, name);
					FREE(mdos);
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
static struct waterlist {
	char mapname[17]; // 16 + NULL
	int waterheight;
} *waterlist = NULL;
int waterlist_num = 0;
#define NO_WATER 1000000

/*==========================================
 *
 *------------------------------------------
 */
static int map_waterheight(char *mapname) {

	int i;

	for(i = 0; i < waterlist_num; i++)
		if (strcmp(waterlist[i].mapname, mapname) == 0)
			return waterlist[i].waterheight;

	return NO_WATER;
}

/*==========================================
 *
 *------------------------------------------
 */
static void map_readwater(char *watertxt) {

	char line[1024], w1[1024];
	FILE *fp;

	fp = fopen(watertxt, "r");
	if (fp == NULL) {
		printf("map_readwater: file not found: %s.\n", watertxt);
		return;
	}

	while(fgets(line, sizeof(line), fp)) {
		int wh, count;
		if ((line[0] == '/' && line[1] == '/') || line[0] == '\0' || line[0] == '\n' || line[0] == '\r')
			continue;
		if ((count = sscanf(line, "%s%d", w1, &wh)) < 1)
			continue;
		if (waterlist_num == 0) {
			CALLOC(waterlist, struct waterlist, 1);
		} else {
			REALLOC(waterlist, struct waterlist, waterlist_num + 1);
			memset(waterlist + waterlist_num, 0, sizeof(struct waterlist));
		}
		memset(waterlist[waterlist_num].mapname, 0, sizeof(waterlist[waterlist_num].mapname));
		strncpy(waterlist[waterlist_num].mapname, w1, 16); // 17 - NULL
		if (count >= 2)
			waterlist[waterlist_num].waterheight = wh;
		else
			waterlist[waterlist_num].waterheight = 3;
		waterlist_num++;
	}
	fclose(fp);

	printf(CL_GREEN "Loaded: " CL_RESET "File '" CL_WHITE "%s" CL_RESET "' read ('" CL_WHITE "%d" CL_RESET "' entrie%s).\n", watertxt, waterlist_num, (waterlist_num > 1) ? "s" : "");
}

/*==========================================
 *
 *------------------------------------------
 */
#define MAX_MAP_CACHE 768

struct map_cache_info {
	char fn[32];
	int xs, ys;
	int water_height;
	int pos;
	int compressed;
	int compressed_len;
};

struct map_cache_head {
	int sizeof_header;
	int sizeof_map;
	int nmaps;
	int filesize;
};
struct {
	struct map_cache_head head;
	struct map_cache_info *map;
	FILE *fp;
	int dirty;
} map_cache;

static int map_cache_open(char *fn);
static void map_cache_close(void);
static int map_cache_read(struct map_data *m);
static int map_cache_write(struct map_data *m);

/*==========================================
 *
 *------------------------------------------
 */
static int map_cache_open(char *fn) {

	atexit(map_cache_close);
	if (map_cache.fp) {
		map_cache_close();
	}
	map_cache.fp = fopen(fn, "r+b");
	if (map_cache.fp) {
		fread(&map_cache.head, 1, sizeof(struct map_cache_head), map_cache.fp);
		fseek(map_cache.fp, 0, SEEK_END);
		if (map_cache.head.sizeof_header == sizeof(struct map_cache_head) &&
		    map_cache.head.sizeof_map    == sizeof(struct map_cache_info) &&
		    map_cache.head.filesize      == ftell(map_cache.fp)) {
			CALLOC(map_cache.map, struct map_cache_info, map_cache.head.nmaps);
			fseek(map_cache.fp, sizeof(struct map_cache_head), SEEK_SET);
			fread(map_cache.map, sizeof(struct map_cache_info), map_cache.head.nmaps, map_cache.fp);
			return 1;
		}
		fclose(map_cache.fp);
	} else if (map_read_flag == READ_FROM_BITMAP) {
		map_read_flag = CREATE_BITMAP; // READ_FROM_BITMAP + 1
	} else if (map_read_flag == READ_FROM_BITMAP_COMPRESSED)
		map_read_flag = CREATE_BITMAP_COMPRESSED; // READ_FROM_BITMAP_COMPRESSED + 1

	map_cache.fp = fopen(fn, "wb");
	if (map_cache.fp) {
		memset(&map_cache.head, 0, sizeof(struct map_cache_head));
		CALLOC(map_cache.map, struct map_cache_info, MAX_MAP_CACHE);
		map_cache.head.nmaps         = MAX_MAP_CACHE;
		map_cache.head.sizeof_header = sizeof(struct map_cache_head);
		map_cache.head.sizeof_map    = sizeof(struct map_cache_info);

		map_cache.head.filesize  = sizeof(struct map_cache_head);
		map_cache.head.filesize += sizeof(struct map_cache_info) * map_cache.head.nmaps;

		map_cache.dirty = 1;
		return 1;
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
static void map_cache_close(void) {

	if (!map_cache.fp)
		return;

	if (map_cache.dirty) {
		fseek(map_cache.fp, 0, SEEK_SET);
		fwrite(&map_cache.head, 1, sizeof(struct map_cache_head), map_cache.fp);
		fwrite(map_cache.map, map_cache.head.nmaps, sizeof(struct map_cache_info), map_cache.fp);
	}
	fclose(map_cache.fp);
	FREE(map_cache.map);
	map_cache.fp = NULL;

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
int map_cache_read(struct map_data *m) {

	int i;

	if (!map_cache.fp)
		return 0;

	for(i = 0; i < map_cache.head.nmaps; i++) {
		if (!strcmp(m->name, map_cache.map[i].fn)) {
			if (map_cache.map[i].water_height != map_waterheight(m->name)) {
				return 0;
			} else if (map_cache.map[i].compressed == 0) {
				int size = map_cache.map[i].xs * map_cache.map[i].ys;
				m->xs = map_cache.map[i].xs;
				m->ys = map_cache.map[i].ys;
				CALLOC(m->gat, char, m->xs * m->ys);
				fseek(map_cache.fp, map_cache.map[i].pos, SEEK_SET);
				if (fread(m->gat, 1, size,map_cache.fp) == size) {
					return 1;
				} else {
					m->xs = 0;
					m->ys = 0;
					FREE(m->gat);
					return 0;
				}
			} else if (map_cache.map[i].compressed == 1) {
				char *buf;
				unsigned long dest_len;
				int size_compress = map_cache.map[i].compressed_len;
				m->xs = map_cache.map[i].xs;
				m->ys = map_cache.map[i].ys;
				MALLOC(m->gat, char, m->xs * m->ys);
				MALLOC(buf, char, size_compress);
				fseek(map_cache.fp, map_cache.map[i].pos, SEEK_SET);
				if (fread(buf, 1, size_compress,map_cache.fp) != size_compress) {
					printf("fread error\n");
					m->xs = 0;
					m->ys = 0;
					FREE(m->gat);
					FREE(buf);
					return 0;
				}
				dest_len = m->xs * m->ys;
				decode_zip(m->gat, &dest_len, buf, size_compress);
				if (dest_len != map_cache.map[i].xs * map_cache.map[i].ys) {
					m->xs = 0;
					m->ys = 0;
					FREE(m->gat);
					FREE(buf);
					return 0;
				}
				FREE(buf);
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
static int map_cache_write(struct map_data *m) {

	int i;
	unsigned long len_new, len_old;
	char *write_buf;

	if (!map_cache.fp)
		return 0;

	for(i = 0; i < map_cache.head.nmaps; i++) {
		if (!strcmp(m->name, map_cache.map[i].fn)) {
			if (map_cache.map[i].compressed == 0) {
				len_old = map_cache.map[i].xs * map_cache.map[i].ys;
			} else if (map_cache.map[i].compressed == 1) {
				len_old = map_cache.map[i].compressed_len;
			} else {
				len_old = 0;
			}
			if (map_read_flag >= READ_FROM_BITMAP_COMPRESSED) {
				MALLOC(write_buf, char, m->xs * m->ys * 2);
				len_new = m->xs * m->ys * 2;
				encode_zip(write_buf, &len_new, m->gat, m->xs * m->ys);
				map_cache.map[i].compressed     = 1;
				map_cache.map[i].compressed_len = len_new;
			} else {
				len_new = m->xs * m->ys;
				write_buf = m->gat;
				map_cache.map[i].compressed     = 0;
				map_cache.map[i].compressed_len = 0;
			}
			if (len_new <= len_old) {
				fseek(map_cache.fp, map_cache.map[i].pos, SEEK_SET);
				fwrite(write_buf, 1, len_new, map_cache.fp);
			} else {
				fseek(map_cache.fp, map_cache.head.filesize, SEEK_SET);
				fwrite(write_buf, 1, len_new, map_cache.fp);
				map_cache.map[i].pos = map_cache.head.filesize;
				map_cache.head.filesize += len_new;
			}
			map_cache.map[i].xs  = m->xs;
			map_cache.map[i].ys  = m->ys;
			map_cache.map[i].water_height = map_waterheight(m->name);
			map_cache.dirty = 1;
			if (map_read_flag >= READ_FROM_BITMAP_COMPRESSED) {
				FREE(write_buf);
			}
			return 0;
		}
	}
	for(i = 0; i < map_cache.head.nmaps; i++) {
		if (map_cache.map[i].fn[0] == 0) {
			if (map_read_flag >= READ_FROM_BITMAP_COMPRESSED) {
				MALLOC(write_buf, char, m->xs * m->ys * 2);
				len_new = m->xs * m->ys * 2;
				encode_zip(write_buf, &len_new, m->gat, m->xs * m->ys);
				map_cache.map[i].compressed     = 1;
				map_cache.map[i].compressed_len = len_new;
			} else {
				len_new = m->xs * m->ys;
				write_buf = m->gat;
				map_cache.map[i].compressed     = 0;
				map_cache.map[i].compressed_len = 0;
			}
			strncpy(map_cache.map[i].fn, m->name, sizeof(map_cache.map[0].fn));
			fseek(map_cache.fp, map_cache.head.filesize, SEEK_SET);
			fwrite(write_buf, 1, len_new, map_cache.fp);
			map_cache.map[i].pos = map_cache.head.filesize;
			map_cache.map[i].xs  = m->xs;
			map_cache.map[i].ys  = m->ys;
			map_cache.map[i].water_height = map_waterheight(m->name);
			map_cache.head.filesize += len_new;
			map_cache.dirty = 1;
			if (map_read_flag >= READ_FROM_BITMAP_COMPRESSED) {
				FREE(write_buf);
			}
			return 0;
		}
	}

	return 1;
}

/*==========================================
 *
 *------------------------------------------
 */
static int map_readmap(int m, char *fn, char *alias, int *map_cache_count) {

	unsigned char *gat;
	int x, y, xs, ys, y_xs;
	struct gat_1cell {float high[4]; int type;} *p;
	int wh;
	size_t size;

	if (map_cache_read(&map[m])) {
		(*map_cache_count)++;
	} else {
		// read & convert fn
		gat = grfio_read(fn);
		if (gat == NULL) {
			printf(CL_YELLOW "Warning: " CL_RESET "Map '%s' not found: " CL_WHITE "removed" CL_RESET " from maplist.\n", fn);
			return -1;
		}
		xs = *(int*)(gat + 6);
		ys = *(int*)(gat + 10);
		if (xs == 0 || ys == 0) { // ?? but possible
			printf(CL_YELLOW "Warning: " CL_RESET "Invalid size for map '%s'" CL_RESET " (xs,ys: %d,%d): " CL_WHITE "removed" CL_RESET " from maplist.\n", fn, xs, ys);
			return -1;
		}
		map[m].xs = xs;
		map[m].ys = ys;
		CALLOC(map[m].gat, char, xs * ys);
		wh = map_waterheight(map[m].name);
		for(y = 0; y < ys; y++) {
			y_xs = y * xs; // speed up
			p = (struct gat_1cell*)(gat + y_xs * 20 + 14);
			for(x = 0; x < xs; x++) {
				if (wh != NO_WATER && p->type == 0) {
					map[m].gat[x + y_xs] = (p->high[0] > wh || p->high[1] > wh || p->high[2] > wh || p->high[3] > wh) ? 3 : 0;
				} else {
					map[m].gat[x + y_xs] = p->type;
				}
				p++;
			}
		}
		map_cache_write(&map[m]);
		FREE(gat);
	}

	printf(CL_WHITE "Server: " CL_RESET "Loading Maps [%d/%d]: " CL_WHITE "%-30s  " CL_RESET "\r", m, map_num, fn);
	fflush(stdout);

	map[m].m = m;
	map[m].npc_num = 0;
	map[m].users = 0;
	memset(&map[m].flag, 0, sizeof(map[m].flag));
	if (battle_config.pk_mode)
		map[m].flag.pvp = 1;
	map[m].flag.nogmcmd = 100;

	map[m].bxs = (map[m].xs + BLOCK_SIZE - 1) / BLOCK_SIZE;
	map[m].bys = (map[m].ys + BLOCK_SIZE - 1) / BLOCK_SIZE;

	size = map[m].bxs * map[m].bys;

	CALLOC(map[m].block, struct block_list *, size);
	CALLOC(map[m].block_mob, struct block_list *, size);

	if (alias != NULL) {
		strdb_insert(map_db, alias, &map[m]);
	} else {
		strdb_insert(map_db, map[m].name, &map[m]);
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
void map_delmap(char *mapname) {

	int i;

	if (strcasecmp(mapname, "all") == 0) {
		FREE(map);
		map_num = 0;
		return;
	}

	for(i = 0; i < map_num; i++) {
		if (strcmp(map[i].name, mapname) == 0) {
			FREE(map[i].alias);
			if (i < map_num - 1)
				memmove(map + i, map + (i + 1), sizeof(struct map_data) * (map_num - i - 1));
			map_num--;
			if (map_num == 0) {
				FREE(map);
				return;
			} else {
				REALLOC(map, struct map_data, map_num);
			}
			i--;
		}
	}

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
int map_readallmap(void) {

	int i, maps_removed = 0;
	char fn[256];
	int map_cache_count = 0;
	char *p;

	if (map_read_flag >= READ_FROM_BITMAP)
		map_cache_open(map_cache_file);

	printf(CL_WHITE "Server: " CL_RESET "Loading Maps%s...\n",
	       (map_read_flag == CREATE_BITMAP_COMPRESSED ? " (Generating Compressed Map Cache)" :
	        map_read_flag == CREATE_BITMAP ? " (Generating Map Cache)" :
	        map_read_flag >= READ_FROM_BITMAP ? " (Reading Map Cache)" : ""));

	for(i = 0; i < map_num; i++) {
		map[i].alias = NULL;

		for(p = &fn[0]; *p != 0; p++) // * At the time of Unix
			if (*p == '\\')
				*p = '/';

		if (strstr(map[i].name, ".gat") != NULL) {
			p = strstr(map[i].name, "<");
			if (p != NULL) {
				char buf[64];
				*p++ = '\0';
				sprintf(buf, "data\\%s", p);
				CALLOC(map[i].alias, char, strlen(buf) + 1); // +1 for NULL
				strcpy(map[i].alias, buf);
			}

			sprintf(fn, "data\\%s", map[i].name);
			if (map_readmap(i, fn, p, &map_cache_count) == -1) {
				map_delmap(map[i].name);
				maps_removed++;
				i--;
			}
		}
	}

	FREE(waterlist);

	if (maps_removed) {
		printf(CL_GREEN "Loaded: " CL_RESET "Successfully loaded " CL_WHITE "%d maps" CL_RESET " on %d %40s\n", map_num, map_num + maps_removed, "");
		if (map_cache_count > 1)
			printf("of which were " CL_WHITE "%d maps" CL_RESET " in cache file.\n", map_cache_count);
		else
			printf("of which was " CL_WHITE "%d map" CL_RESET " in cache file.\n", map_cache_count);
		printf(CL_WHITE "Server: " CL_RESET "Maps Removed: " CL_WHITE "%d" CL_RESET ".\n", maps_removed);
	} else {
		printf(CL_GREEN "Loaded: " CL_RESET "Successfully loaded " CL_WHITE "%d maps" CL_RESET "%40s\n", map_num, "");
		if (map_cache_count > 1)
			printf("of which were " CL_WHITE "%d maps" CL_RESET " in cache file.\n", map_cache_count);
		else
			printf("of which was " CL_WHITE "%d map" CL_RESET " in cache file.\n", map_cache_count);
	}

	map_cache_close();
	if (map_read_flag == CREATE_BITMAP || map_read_flag == CREATE_BITMAP_COMPRESSED)
		--map_read_flag;

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
void map_addmap(char *mapname) {

	if (strcasecmp(mapname, "clear") == 0) {
		FREE(map);
		map_num = 0;
		return;
	}

	if (map_num == 0) {
		CALLOC(map, struct map_data, 1);
	} else {
		REALLOC(map, struct map_data, map_num + 1);
		memset(map + map_num, 0, sizeof(struct map_data));
	}

	strncpy(map[map_num].name, mapname, 16); // 17 - NULL
	map_num++;

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
int parse_console(char *buf) {

	static int console_on = 1;
	char command[4096], param[4096], mapname[4096], buf2[4096];
	int x = 0, y = 0;
	int m, n;
	struct map_session_data *sd;

	memset(command, 0, sizeof(command));
	memset(param, 0, sizeof(param));
	memset(mapname, 0, sizeof(mapname));
	memset(buf2, 0, sizeof(buf2));

	sscanf(buf, "%s %[^\n]", command, param);

	if (!console_on) {

		if (strcasecmp("?", command) == 0 ||
		    strcasecmp("h", command) == 0 ||
		    strcasecmp("help", command) == 0 ||
		    strcasecmp("aide", command) == 0) {
			printf(CL_DARK_CYAN "Console: " CL_RESET  "Help commands:" CL_RESET "\n");
			printf(CL_DARK_CYAN "Console: " CL_RESET "  '" CL_DARK_CYAN "?|h|help|aide" CL_RESET "': Display this help.\n");
			printf(CL_DARK_CYAN "Console: " CL_RESET "  '" CL_DARK_CYAN "<console password>" CL_RESET "': enable console mode.\n");
		} else if (strcmp(console_pass, command) == 0) {
			printf(CL_DARK_CYAN "Console: " CL_RESET "Console commands are now enabled." CL_RESET "\n");
			console_on = 1;
		} else {
			printf(CL_DARK_CYAN "Console: " CL_RESET "Console commands are disabled. Please enter the password." CL_RESET "\n");
		}

	} else {

		if (strcasecmp("shutdown", command) == 0 ||
		    strcasecmp("exit", command) == 0 ||
		    strcasecmp("quit", command) == 0 ||
		    strcasecmp("end", command) == 0) {
			exit(0);
		} else if (strcasecmp("alive", command) == 0 ||
		           strcasecmp("status", command) == 0 ||
		           strcasecmp("uptime", command) == 0) {
			int count;
			printf(CL_DARK_CYAN "Console: " CL_RESET CL_BOLD "Uptime - %u seconds." CL_RESET "\n", (int)(time(NULL) - start_time));
			if (map_is_alone) { // not in multi-servers
				// calculation like @who (don't show hidden GM) and don't wait update from char-server (realtime calculation)
				int i;
				struct map_session_data *pl_sd;
				count = 0;
				for (i = 0; i < fd_max; i++)
					if (session[i] && (pl_sd = session[i]->session_data) && pl_sd->state.auth)
						if (!(pl_sd->GM_level >= battle_config.hide_GM_session || (pl_sd->status.option & OPTION_HIDE)))
							count++;
			} else
				count = map_getusers();
			printf(CL_DARK_CYAN "Console: " CL_RESET CL_BOLD "Number of online player%s: %d." CL_RESET "\n", (count > 1) ? "s" : "", count);
		} else if (strcasecmp("?", command) == 0 ||
		           strcasecmp("h", command) == 0 ||
		           strcasecmp("help", command) == 0 ||
		           strcasecmp("aide", command) == 0) {
			printf(CL_DARK_GREEN "Help of commands:" CL_RESET "\n");
			printf(CL_DARK_CYAN "Console: " CL_RESET "  To use GM commands: ");
			printf(CL_DARK_CYAN "Console: " CL_RESET "  " CL_DARK_CYAN "<gm command>:<map_of_\"gm\"> <x> <y>" CL_RESET "\n");
			printf(CL_DARK_CYAN "Console: " CL_RESET "  You can use any GM command that doesn't require the GM.\n");
			printf(CL_DARK_CYAN "Console: " CL_RESET "  No using @item or @warp however you can use @charwarp.\n");
			printf(CL_DARK_CYAN "Console: " CL_RESET "  The <map_of_\"gm\"> <x> <y> is for commands that need coords of the GM.\n");
			printf(CL_DARK_CYAN "Console: " CL_RESET "  IE: @spawn\n");
			printf(CL_DARK_CYAN "Console: " CL_RESET "  '" CL_DARK_CYAN "shutdown|exit|qui|end" CL_RESET "': To shutdown the server.\n");
			printf(CL_DARK_CYAN "Console: " CL_RESET "  '" CL_DARK_CYAN "alive|status|uptime" CL_RESET "': To know if server is alive.\n");
			printf(CL_DARK_CYAN "Console: " CL_RESET "  '" CL_DARK_CYAN "consoleoff" CL_RESET "': To disable console commands.\n");
			printf(CL_DARK_CYAN "Console: " CL_RESET "  '" CL_DARK_CYAN "?|h|help|aide" CL_RESET "': To display help.\n");
			printf(CL_DARK_CYAN "Console: " CL_RESET "  '" CL_DARK_CYAN "version" CL_RESET "': To display version of the server.\n");
		} else if (strcasecmp("version", command) == 0) {
			versionscreen();
		} else if (strcasecmp("consoleoff", command) == 0 ||
		           strcasecmp("console_off", command) == 0 ||
		           strcasecmp("consoloff", command) == 0 ||
		           strcasecmp("consol_off", command) == 0||
		           strcasecmp("console", command) == 0) {
			if (strcasecmp("console", command) == 0 && strcasecmp("off", param) != 0) {
				printf(CL_RED "ERROR: Unknown parameter." CL_RESET "\n");
			} else {
				printf(CL_DARK_CYAN "Console: " CL_RESET "Console commands are now disabled." CL_RESET "\n");
				console_on = 0;
			}
		} else {
			n = sscanf(buf, "%[^:]:%99s %d %d[^\n]", command, mapname, &x, &y);

			m = 0;
			if (n > 1) {
				if (strstr(mapname, ".gat") == NULL && strlen(mapname) < 13) // 16 - 4 (.gat)
					strcat(mapname, ".gat");
				if ((m = map_mapname2mapid(mapname)) < 0) {
					printf(CL_DARK_CYAN "Console: " CL_RESET CL_BOLD "Unknown map on this map-server." CL_RESET "\n");
					return 0;
				}
			}
			CALLOC(sd, struct map_session_data, 1);
			sd->fd = 0;
			strcpy(sd->status.name, "Console");
			sd->bl.m = m;
			sd->GM_level = 99;
			if (x <= 0)
				sd->bl.x = rand() % (map[m].xs - 2) + 1;
			else
				sd->bl.x = x;
			if (y <= 0)
				sd->bl.y = rand() % (map[m].ys - 2) + 1;
			else
				sd->bl.y = y;
			sprintf(buf2, "Console: %s", command);
			if (is_atcommand(sd->fd, sd, buf2, 99) == AtCommand_None)
				printf(CL_DARK_CYAN "Console: " CL_RESET CL_RED "ERROR - Unknown command [%s]." CL_RESET "\n", command);
			FREE(sd);
		}
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
void remove_ended_comments(char *line) {

	int i;

	i = 0;
	while(line[i] && line[i + 1]) {
		if (line[i] == '/' && line[i + 1] == '/') {
			line[i] = '\0';
			break;
		}
		i++;
	}

	return;
}

static int map_ip_set_ = 0;
static int char_ip_set_ = 0;

/*==========================================
 *
 *------------------------------------------
 */
int map_config_read(char *cfgName) {

	char line[1024], w1[1024], w2[1024];
	FILE *fp;
	struct hostent *h = NULL;

	if ((fp = fopen(cfgName, "r")) == NULL) {
			printf("Map configuration file not found at: %s\n", cfgName);
			exit(1);
	}

	while(fgets(line, sizeof(line), fp)) {
		if ((line[0] == '/' && line[1] == '/') || line[0] == '\0' || line[0] == '\n' || line[0] == '\r')
			continue;

		memset(w2, 0, sizeof(w2));
		if (sscanf(line, "%[^:]: %[^\r\n]", w1, w2) == 2) {
			// check options.
			if (strcasecmp(w1, "userid") == 0) {
				// don't remove ended comments, that can be part of this option
				chrif_setuserid(w2);
			} else if (strcasecmp(w1, "passwd") == 0) {
				// don't remove ended comments, that can be part of this option
				chrif_setpasswd(w2);
			} else if (strcasecmp(w1, "char_ip") == 0) {
				remove_ended_comments(w2); // remove ended comments
				char_ip_set_ = 1;
				h = gethostbyname(w2);
				if (h != NULL) {
				printf(CL_WHITE "Server: " CL_RESET "Character server IP address: " CL_WHITE "%s" CL_RESET " -> " CL_WHITE "%d.%d.%d.%d" CL_RESET "\n", w2, (unsigned char)h->h_addr[0], (unsigned char)h->h_addr[1], (unsigned char)h->h_addr[2], (unsigned char)h->h_addr[3]);
					sprintf(w2,"%d.%d.%d.%d", (unsigned char)h->h_addr[0], (unsigned char)h->h_addr[1], (unsigned char)h->h_addr[2], (unsigned char)h->h_addr[3]);
				}
				chrif_setip(w2);
			} else if (strcasecmp(w1, "char_port") == 0) {
				remove_ended_comments(w2); // remove ended comments
				chrif_setport(atoi(w2));
			} else if (strcasecmp(w1, "map_ip") == 0) {
				remove_ended_comments(w2); // remove ended comments
				map_ip_set_ = 1;
				h = gethostbyname(w2);
				if (h != NULL) {
				printf(CL_WHITE "Server: " CL_RESET "Map server IP address: " CL_WHITE "%s" CL_RESET " -> " CL_WHITE "%d.%d.%d.%d" CL_RESET "\n", w2, (unsigned char)h->h_addr[0], (unsigned char)h->h_addr[1], (unsigned char)h->h_addr[2], (unsigned char)h->h_addr[3]);
					sprintf(w2, "%d.%d.%d.%d", (unsigned char)h->h_addr[0], (unsigned char)h->h_addr[1], (unsigned char)h->h_addr[2], (unsigned char)h->h_addr[3]);
				}
				clif_setip(w2);
			} else if (strcasecmp(w1, "map_port") == 0) {
				remove_ended_comments(w2); // remove ended comments
				clif_setport(atoi(w2));
				map_port = atoi(w2);
			} else if (strcasecmp(w1, "listen_ip") == 0) {
				remove_ended_comments(w2); // remove ended comments
				memset(listen_ip, 0, sizeof(listen_ip));
				h = gethostbyname(w2);
				if (h != NULL) {
					sprintf(listen_ip, "%d.%d.%d.%d", (unsigned char)h->h_addr[0], (unsigned char)h->h_addr[1], (unsigned char)h->h_addr[2], (unsigned char)h->h_addr[3]);
				} else {
					strncpy(listen_ip, w2, 15); // 16 - NULL
				}
			} else if (strcasecmp(w1, "water_height") == 0) {
				remove_ended_comments(w2); // remove ended comments
				map_readwater(w2);
			} else if (strcasecmp(w1, "map") == 0) {
				remove_ended_comments(w2); // remove ended comments
				map_addmap(w2);
			} else if (strcasecmp(w1, "delmap") == 0) {
				remove_ended_comments(w2); // remove ended comments
				printf(CL_WHITE "Server: " CL_RESET "Removing map [ %s ] from maplist.\n", w2);
				map_delmap(w2);
			} else if (strcasecmp(w1, "npc") == 0) {
				remove_ended_comments(w2); // remove ended comments
				npc_addsrcfile(w2);
			} else if (strcasecmp(w1, "delnpc") == 0) {
				remove_ended_comments(w2); // remove ended comments
				npc_delsrcfile(w2);
			} else if (strcasecmp(w1, "data_grf") == 0) {
				remove_ended_comments(w2); // remove ended comments
				grfio_setdatafile(w2);
			} else if (strcasecmp(w1, "sdata_grf") == 0) {
				remove_ended_comments(w2); // remove ended comments
				grfio_setsdatafile(w2);
			} else if (strcasecmp(w1, "adata_grf") == 0) {
				remove_ended_comments(w2); // remove ended comments
				grfio_setadatafile(w2);
			} else if (strcasecmp(w1, "autosave_time") == 0) {
				remove_ended_comments(w2); // remove ended comments
				autosave_interval = atoi(w2) * 1000;
				if (autosave_interval <= 0)
					autosave_interval = DEFAULT_AUTOSAVE_INTERVAL;
			} else if (strcasecmp(w1, "display_unknown_packet") == 0) {
				remove_ended_comments(w2); // remove ended comments
				display_unknown_packet = config_switch(w2);
			} else if (strcasecmp(w1, "motd_txt") == 0) {
				remove_ended_comments(w2); // remove ended comments
				memset(motd_txt, 0, sizeof(motd_txt));
				strcpy(motd_txt, w2);
			} else if (strcasecmp(w1, "help_txt") == 0) {
				remove_ended_comments(w2); // remove ended comments
				memset(help_txt, 0, sizeof(help_txt));
				strcpy(help_txt, w2);
			} else if (strcasecmp(w1, "mapreg_txt") == 0) {
				remove_ended_comments(w2); // remove ended comments
				memset(mapreg_txt, 0, sizeof(mapreg_txt));
				strncpy(mapreg_txt, w2, sizeof(mapreg_txt) - 1);
			} else if (strcasecmp(w1, "mapreg_txt_timer") == 0) {
				remove_ended_comments(w2); // remove ended comments
				if (atoi(w2) >= 10 && atoi(w2) <= 3600)
					mapreg_txt_timer = atoi(w2);
			} else if(strcasecmp(w1, "extra_add_file_txt") == 0) {
				remove_ended_comments(w2); // remove ended comments
				memset(extra_add_file_txt, 0, sizeof(extra_add_file_txt));
				strncpy(extra_add_file_txt, w2, sizeof(extra_add_file_txt) - 1);
			} else if (strcasecmp(w1, "read_map_from_cache") == 0) {
				remove_ended_comments(w2); // remove ended comments
				if (atoi(w2) == 2)
					map_read_flag = READ_FROM_BITMAP_COMPRESSED;
				else if (atoi(w2) == 1)
					map_read_flag = READ_FROM_BITMAP;
				else
					map_read_flag = READ_FROM_GAT;
			}else if (strcasecmp(w1, "map_cache_file") == 0){
				remove_ended_comments(w2); // remove ended comments
				memset(map_cache_file, 0, sizeof(map_cache_file));
				strncpy(map_cache_file, w2, sizeof(map_cache_file) - 1);
			} else if (strcasecmp(w1, "npc_language") == 0) {
				remove_ended_comments(w2); // remove ended comments
				if (strlen(w2) > 5) {
					printf(CL_YELLOW "Warning: " CL_RESET "NPC language not changed (bad value detected : len > 5).\n");
				} else {
					memset(npc_language, 0, sizeof(npc_language));
					strcpy(npc_language, w2);
				}
			} else if (strcasecmp(w1, "console") == 0) {
				remove_ended_comments(w2); // remove ended comments
				console = config_switch(w2);
			} else if (strcasecmp(w1, "console_pass") == 0) {
				// don't remove ended comments, that can be part of this option
				memset(console_pass, 0, sizeof(console_pass));
				strncpy(console_pass, w2, sizeof(console_pass) - 1);

			} else if (strcasecmp(w1, "create_item_db_script") == 0) {
				remove_ended_comments(w2); // remove ended comments
				create_item_db_script = config_switch(w2); // generate a script file to create SQL item_db (0: no, 1: yes)
			} else if (strcasecmp(w1, "create_mob_db_script") == 0) {
				remove_ended_comments(w2); // remove ended comments
				create_mob_db_script = config_switch(w2); // generate a script file to create SQL mob_db (0: no, 1: yes)
#ifndef TXT_ONLY
			} else if (strcasecmp(w1, "optimize_table") == 0) {
				remove_ended_comments(w2); // remove ended comments
				optimize_table = config_switch(w2); // optimize mob/item SQL db
#endif /* not TXT_ONLY */

			} else if (strcasecmp(w1, "addon") == 0) {
				remove_ended_comments(w2); // remove ended comments
				addons_load(w2, ADDONS_MAP);
			} else if (strcasecmp(w1, "import") == 0) {
				remove_ended_comments(w2); // remove ended comments
				printf(CL_GREEN "Loaded: " CL_RESET "map_config_read: Import file: %s.\n", w2);
				map_config_read(w2);
			}
		}
	}
	fclose(fp);

	return 0;
}

#ifndef TXT_ONLY
/*==========================================
 *
 *------------------------------------------
 */
int map_sql_init(void){

	sql_init();

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int map_sql_close(void){

	sql_close();
	printf("Close Map DB Connection...\n");

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int sql_config_read(char *cfgName) {

	char line[1024], w1[1024], w2[1024];
	FILE *fp;

	if ((fp = fopen(cfgName, "r")) == NULL) {
			printf("File not found: %s.\n", cfgName);
			return 1;
	}
	while(fgets(line, sizeof(line), fp)) {
		if ((line[0] == '/' && line[1] == '/') || line[0] == '\0' || line[0] == '\n' || line[0] == '\r')
			continue;

		memset(w2, 0, sizeof(w2));
		if (sscanf(line, "%[^:]: %[^\r\n]", w1, w2) != 2)
			continue;

		if (strcasecmp(w1, "item_db_db") == 0) {
			strcpy(item_db_db, w2);
		} else if (strcasecmp(w1, "mob_db_db") == 0) {
			strcpy(mob_db_db, w2);
		} else if (strcasecmp(w1, "char_db") == 0) {
			strcpy(char_db, w2);

		// Map Server SQL DB
		} else if (strcasecmp(w1, "mysql_server_ip") == 0) {
			strcpy(db_mysql_server_ip, w2);
		} else if (strcasecmp(w1, "mysql_server_port") == 0) {
			db_mysql_server_port = atoi(w2);
		} else if (strcasecmp(w1, "mysql_server_id") == 0) {
			strcpy(db_mysql_server_id, w2);
		} else if (strcasecmp(w1, "mysql_server_pw") == 0) {
			strcpy(db_mysql_server_pw, w2);
		} else if (strcasecmp(w1, "mysql_map_db") == 0) {
			strcpy(db_mysql_server_db, w2);

		// Map server option to use SQL db or not
		} else if (strcasecmp(w1, "use_sql_db") == 0) {
			db_use_sqldbs = config_switch(w2);
			printf ("Using SQL dbs: " CL_WHITE "%s" CL_RESET ".\n", w2);

		} else if (strcasecmp(w1, "import") == 0) {
			printf("sql_config_read: Import file: %s.\n", w2);
			sql_config_read(w2);
		}
	}
	fclose(fp);

	printf(CL_GREEN "Loaded: " CL_RESET "File '" CL_WHITE "%s" CL_RESET "' read.\n", cfgName);

	return 0;
}
#endif /* not TXT_ONLY */

/*==========================================
 *
 *------------------------------------------
 */
int id_db_final(void *k, void *d, va_list ap){ return 0; }
int map_db_final(void *k, void *d, va_list ap){ return 0; }

/*==========================================
 *
 *------------------------------------------
 */
int charid_db_final(void *k, void *d, va_list ap) {
	FREE(d);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
static int cleanup_sub(struct block_list *bl, va_list ap) {
	nullpo_retr(0, bl);

	switch(bl->type) {
	case BL_PC:
		if (((struct map_session_data *)bl)->state.auth)
			map_quit((struct map_session_data *)bl);
		map_quit2((struct map_session_data *)bl); // to free memory of dynamic allocation in session (even if player is auth or not)
		break;
	case BL_NPC:
		npc_delete((struct npc_data *)bl);
		break;
	case BL_MOB:
		mob_delete((struct mob_data *)bl);
		break;
	case BL_PET:
		pet_remove_map((struct map_session_data *)bl);
		break;
	case BL_ITEM:
		map_clearflooritem(bl->id);
		break;
	case BL_SKILL:
		skill_delunit((struct skill_unit *) bl);
		break;
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
void do_final(void) {

	int map_id, i, j;
	struct map_session_data *sd;

	printf("Terminating...\n");

	map_cache_close();

	pc_guardiansave(); // save guardians (if necessary)

	// save all online players
	j = 0;
	for (i = 0; i < fd_max; i++) {
		if (session[i] && (sd = session[i]->session_data) && sd->state.auth) {
			if (sd->last_saving + 30000 < gettick_cache) { // not save if previous saving was done recently // to limit successive savings with auto-save
				chrif_save(sd); // do pc_makesavestatus and save storage + account_reg/account_reg2 too
				j++;
				if (j % 3 == 0) // send to char-server only every 3 characters
					flush_fifos(); // flush all packets of all connections (including char-server connection)
			}
		}
	}

	if (j == 1)
		printf("1 player sended to char-server (previous saving: more than 30 sec).\n");
	else if (j > 1)
		printf("%d player(s) sended to char-server (previous saving: more than 30 sec).\n", j);

	for (map_id = 0; map_id < map_num; map_id++) {
		if (map[map_id].m)
			map_foreachinarea(cleanup_sub, map_id, 0, 0, map[map_id].xs, map[map_id].ys, 0, 0);
	}

	/* send all packets not sended */
	flush_fifos();

	// close all connections
	do_final_chrif(); // reset char_fd and close connection
	for (i = 0; i < fd_max; i++) {
		if (session[i] && session[i]->session_data) {
#ifdef __WIN32
			if (i > 0) { // not console
				shutdown(i, SD_BOTH);
				closesocket(i);
			}
#else
			close(i);
#endif
			delete_session(i);
		}
	}

	delete_manner();

	do_final_npc();
	timer_final();

	do_final_atcommand();
	do_final_script();
	do_final_itemdb();
	do_final_storage();
	do_final_guild();
	do_final_pet();
	do_final_pc();

	for(i = 0; i < map_num; i++) {
		FREE(map[i].gat);
		FREE(map[i].block);
		FREE(map[i].block_mob);
		FREE(map[i].drop_list);
	}
	FREE(map);

	numdb_final(id_db, id_db_final);
	strdb_final(map_db, map_db_final);
	numdb_final(charid_db, charid_db_final);

	/* restore console parameters */
	term_input_disable();

#ifndef TXT_ONLY
	if (db_use_sqldbs)
		map_sql_close();
#endif /* not TXT_ONLY */

#ifdef __WIN32
	// close windows sockets
	WSACleanup();
#endif /* __WIN32 */

	printf("Finished.\n");
}

/*==========================================
 *
 *------------------------------------------
 */
void map_helpscreen() {

	printf("\n");
	puts(CL_DARK_GREEN "Usage: map-server [options]" CL_RESET "");
	puts("  " CL_DARK_CYAN "Options                  " CL_RESET " Description");
	puts("-------------------------------------------------------------------------------");
	puts("  " CL_DARK_CYAN "--help, --h, --?, /?     " CL_RESET " Displays this help screen");
	puts("  " CL_DARK_CYAN "--map-config <file>      " CL_RESET " Load map-server configuration from <file>");
	puts("  " CL_DARK_CYAN "--battle-config <file>   " CL_RESET " Load battle configuration from <file>");
	puts("  " CL_DARK_CYAN "--atcommand-config <file>" CL_RESET " Load atcommand configuration from <file>");
	puts("  " CL_DARK_CYAN "--script-config <file>   " CL_RESET " Load script configuration from <file>");
	puts("  " CL_DARK_CYAN "--msg-config <file>      " CL_RESET " Load message configuration from <file>");
	puts("  " CL_DARK_CYAN "--grf-path-file <file>   " CL_RESET " Load grf path file configuration from <file>");
	puts("  " CL_DARK_CYAN "--version, --v, -v, /v   " CL_RESET " Displays the server's version");
	puts("  " CL_DARK_CYAN "--sql-config <file>      " CL_RESET " Load inter-server configuration from <file>");
	puts("                            (SQL Only)");
	printf("\n");

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void do_init(const int argc, char *argv[]) {
	register int i;

#ifndef TXT_ONLY
	char *SQL_CONF_NAME = "conf/inter_freya.conf";
#endif
	char *MAP_CONF_NAME = "conf/map_freya.conf";
	char *BATTLE_CONF_FILENAME = "conf/battle_freya.conf";
	char *ATCOMMAND_CONF_FILENAME = "conf/atcommand_freya.conf";
	char *SCRIPT_CONF_NAME = "conf/script_freya.conf";
	char *GRF_PATH_FILENAME = "conf/grf-files.txt";

	memset(messages_filename, 0, sizeof(messages_filename));
	strncpy(messages_filename, "conf/msg_freya.conf", sizeof(messages_filename) - 1);

	srand(gettick_cache);

	for (i = 1; i < argc; i++) {

		if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "--h") == 0 || strcmp(argv[i], "--?") == 0 || strcmp(argv[i], "/?") == 0) {
			map_helpscreen();
			exit(1);
		} else if (strcmp(argv[i], "--version") == 0 || strcmp(argv[i], "--v") == 0 || strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "/v") == 0) {
			versionscreen();
			exit(1);
		} else if (strcmp(argv[i], "--map_config") == 0 || strcmp(argv[i], "--map-config") == 0)
			MAP_CONF_NAME = argv[i + 1];
		else if (strcmp(argv[i], "--battle_config") == 0 || strcmp(argv[i],"--battle-config") == 0)
			BATTLE_CONF_FILENAME = argv[i + 1];
		else if (strcmp(argv[i], "--atcommand_config") == 0 || strcmp(argv[i],"--atcommand-config") == 0)
			ATCOMMAND_CONF_FILENAME = argv[i + 1];
		else if (strcmp(argv[i], "--script_config") == 0 || strcmp(argv[i],"--script-config") == 0)
			SCRIPT_CONF_NAME = argv[i + 1];
		else if (strcmp(argv[i], "--msg_config") == 0 || strcmp(argv[i],"--msg-config") == 0) {
			memset(messages_filename, 0, sizeof(messages_filename));
			strncpy(messages_filename, argv[i + 1], sizeof(messages_filename) - 1);
		} else if (strcmp(argv[i], "--grf_path_file") == 0 || strcmp(argv[i],"--grf-path-file") == 0)
			GRF_PATH_FILENAME = argv[i + 1];
#ifndef TXT_ONLY
		else if (strcmp(argv[i], "--sql_config") == 0 || strcmp(argv[i],"--sql-config") == 0)
			SQL_CONF_NAME = argv[i + 1];
#endif /* not TXT_ONLY */
		else if (strcmp(argv[i], "--run_once") == 0) // close the map-server as soon as its done.. for testing
			runflag = 0;
	}

	printf(CL_WHITE "\nServer: Freya Configuration Settings:\n" CL_RESET);
	map_config_read(MAP_CONF_NAME);

	if (map_ip_set_ == 0 || char_ip_set_ == 0) {
		// The map server should know what IP address it is running on
		int localaddr = ntohl(addr_[0]);
		unsigned char *ptr = (unsigned char *) &localaddr;
		char buf[16];
		sprintf(buf, "%d.%d.%d.%d", ptr[0], ptr[1], ptr[2], ptr[3]);
		if (naddr_ != 1)
			printf("Multiple interfaces detected. Using %s as our IP address.\n", buf);
		else
			printf("Defaulting to %s as our IP address.\n", buf);
		if (map_ip_set_ == 0)
			clif_setip(buf);
		if (char_ip_set_ == 0)
			chrif_setip(buf);
	}

	battle_config_read(BATTLE_CONF_FILENAME);
	atcommand_config_read(ATCOMMAND_CONF_FILENAME);
	script_config_read(SCRIPT_CONF_NAME);
	msg_config_read(messages_filename);
#ifndef TXT_ONLY
	sql_config_read(SQL_CONF_NAME); // set 'db_use_sqldbs' flag
#endif /* not TXT_ONLY */
#ifdef DYNAMIC_LINKING
	/* load all addons */
	addons_enable_all();
#endif /* DYNAMIC_LINKING */

	atexit(do_final);

	id_db = numdb_init();
	map_db = strdb_init(17); // 16 + NULL
	charid_db = numdb_init();
#ifdef USE_SQL
	if (db_use_sqldbs) {
		map_sql_init();
#ifdef USE_MYSQL
		if (optimize_table) {
			printf("The map-server optimizes SQL tables: `%s` and `%s`...\r", item_db_db, mob_db_db);
			sql_request("OPTIMIZE TABLE `%s`, `%s`", item_db_db, mob_db_db);
			printf("SQL tables (`%s` and `%s`) have been optimized.        \n", item_db_db, mob_db_db);
		}
#endif /* USE_MYSQL */
	} else
		printf("Mysql connection not initialized (you don't use mob/item db system).\n");
#endif /* USE_SQL */

	grfio_init(GRF_PATH_FILENAME);

	// check battle option in command line (--<battle_option>)
	for (i = 1; i < argc; i++) {
		if (argv[i + 1] != NULL)
			if (argv[i][0] == '-' && argv[i][1] == '-')
				battle_set_value(argv[i] + 2, argv[i+1]);
	}

	map_readallmap();

	add_timer_func_list(map_clearflooritem_timer, "map_clearflooritem_timer");

	do_init_chrif();
	do_init_clif();

	// Begin loading databases
	printf(CL_WHITE "\nServer: Freya Databases:\n" CL_RESET);

	do_init_itemdb();
	do_init_mob();
	do_init_script();
	do_init_pc();
	do_init_status();
	do_init_party();
	do_init_guild();
	do_init_storage();
	do_init_skill();
	do_init_pet();

	// Begin loading scripts
	do_init_npc();

	npc_event_do_oninit();

	if (battle_config.pk_mode)
		printf(CL_WHITE "Server: " CL_RESET "The server is running in " CL_RED "PK Mode" CL_RESET ".\n");

	if (strcmp(listen_ip, "0.0.0.0") == 0) {
		printf(CL_WHITE "Server: " CL_RESET "The map-server is " CL_GREEN "ready" CL_RESET " (listening on the port " CL_WHITE "%d" CL_RESET " - from any ip).\n", map_port);
	} else {
		printf(CL_WHITE "Server: " CL_RESET "The map-server is " CL_GREEN "ready" CL_RESET " (listening on " CL_WHITE "%s:%d" CL_RESET ").\n", listen_ip, map_port);
	}

	if (console) {
		start_console(parse_console);
		if (term_input_status == 0) {
			printf(CL_WHITE "Server: " CL_RESET "Sorry, but console commands can not be initialized -> disabled.\n\n");
			console = 0;
		} else {
			printf(CL_WHITE "Server: " CL_RESET "Console commands are enabled. Type " CL_BOLD "help" CL_RESET " to have a little help.\n\n");
		}
	} else
		printf(CL_DARK_CYAN "Console: Console commands are disabled. You cannot enter any console commands." CL_RESET "\n\n");

	return;
}
