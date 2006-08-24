// $Id: map.c 577 2005-12-03 18:06:48Z Yor $
#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <math.h> // floor
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
#include <gettext.h>

#ifdef MEMWATCH
#include "memwatch.h"
#endif

#ifndef TXT_ONLY

#include "mail.h" // mail system [Valaris]

int db_use_sqldbs = 0;

char char_db[32] = "char";

unsigned char optimize_table = 0;

#endif /* not TXT_ONLY */

char item_db_db[32] = "item_db"; // need in TXT too to generate SQL DB script (not change the default name in this case)
char mob_db_db[32] = "mob_db"; // need in TXT too to generate SQL DB script (not change the default name in this case)

char create_item_db_script = 0; // generate a script file to create SQL item_db (0: no, 1: yes)
char create_mob_db_script = 0; // generate a script file to create SQL mob_db (0: no, 1: yes)

// 極力 staticでローカルに収める
static struct dbt * id_db;
static struct dbt * map_db;
//static struct dbt * nick_db; // not used
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

static char afm_dir[1024] = ""; // [Valaris]

//struct map_data map[MAX_MAP_PER_SERVER];
struct map_data *map = NULL;
int map_num = 0;

int map_port;

int autosave_interval = DEFAULT_AUTOSAVE_INTERVAL;
unsigned char agit_flag = 0; // 0: WoE not starting, Woe is running
unsigned char night_flag = 0; // 0=day, 1=night [Yor]

struct charid2nick {
	char nick[25]; // 24 + NULL
	int req_id;
};

// マップキャッシュ利用フラグ,どっちを使うかはmap_athana.conf内のread_map_from_bitmapで指定
// 0ならば利用しない、1だと非圧縮保存、2だと圧縮して保存
int map_read_flag = READ_FROM_BITMAP_COMPRESSED;
char map_cache_file[1024] = "db/mapinfo.txt"; //ビットマップファイルのデフォルトパス

char motd_txt[1024] = "conf/motd.txt";
char help_txt[1024] = "conf/help.txt";

char wisp_server_name[25] = "Server"; // can be modified in char-server configuration file
int server_char_id; // char id used by server
int server_fake_mob_id; // mob id of a invisible mod to check bots
char npc_language[6] = "en_US"; // Language used by the server (for GetText and manner)
char npc_charset[20] = "iso-8859-1"; // Charset for GetText

int console = 0;
char console_pass[1024] = "consoleon"; /* password to enable console */

/*==========================================
 * 全map鯖総計での接続数設定
 * (char鯖から送られてくる)
 *------------------------------------------
 */
void map_setusers(int n) {
	users = n;
}

/*==========================================
 * 全map鯖総計での接続数取得 (/wへの応答用)
 *------------------------------------------
 */
int map_getusers(void) {
	return users;
}

//
// block削除の安全性確保処理
//

/*==========================================
 * blockをfreeするときfreeの変わりに呼ぶ
 * ロックされているときはバッファにためる
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
 * blockのfreeを一時的に禁止する
 *------------------------------------------
 */
void map_freeblock_lock(void) {
	++block_free_lock;

	return;
}

/*==========================================
 * blockのfreeのロックを解除する
 * このとき、ロックが完全になくなると
 * バッファにたまっていたblockを全部削除
 *------------------------------------------
 */
void map_freeblock_unlock(void) {
	int i;

	if ((--block_free_lock) == 0) {
//		printf("map_freeblock_unlock for %d blocks.\n", block_free_count);
		for(i = 0; i < block_free_count; i++) {
//			if (i != 0)
//				printf("map_freeblock_unlock for %d blocks.\n", block_free_count);
			FREE(block_free[i]);
		}
		block_free_count = 0;
	}else if (block_free_lock < 0){
		// error in code! always display //
		printf("map_freeblock_unlock: lock count < 0 !\n");
	}

	return;
}

//
// block化処理
//
/*==========================================
 * map[]のblock_listから繋がっている場合に
 * bl->prevにbl_headのアドレスを入れておく
 *------------------------------------------
 */
static struct block_list bl_head;

/*==========================================
 * map[]のblock_listに追加
 * mobは数が多いので別リスト
 *
 * 既にlink済みかの確認が無い。危険かも
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
 * map[]のblock_listから外す
 * prevがNULLの場合listに繋がってない
 *------------------------------------------
 */
int map_delblock(struct block_list *bl) {

	nullpo_retr(0, bl);

	// 既にblocklistから抜けている
	if (bl->prev == NULL) {
		if (bl->next != NULL) {
			// prevがNULLでnextがNULLでないのは有ってはならない
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
		// リストの頭なので、map[]のblock_listを更新する
		if(bl->type == BL_MOB){
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
 * 周囲のPC人数を数える (現在未使用)
 *------------------------------------------
 */
/*int map_countnearpc(int m, int x, int y) {
	int bx, by, bx_max, by_max, c;
	struct block_list *bl;

	if (map[m].users == 0)
		return 0;

	c = 0;
	bx_max = x / BLOCK_SIZE + AREA_SIZE / BLOCK_SIZE + 1;
	if (bx_max >= map[m].bxs)
		bx_max = map[m].bxs - 1;
	by_max = y / BLOCK_SIZE + AREA_SIZE / BLOCK_SIZE + 1;
	if (by_max >= map[m].bys)
		by_max = map[m].bys - 1;
	for(by = y / BLOCK_SIZE - AREA_SIZE / BLOCK_SIZE - 1; by < by_max; by++) {
		if (by < 0)
			continue;
		for(bx = x / BLOCK_SIZE - AREA_SIZE / BLOCK_SIZE - 1; bx < bx_max; bx++) {
			if (bx < 0)
				continue;
			for(bl = map[m].block[bx + by * map[m].bxs]; bl; bl = bl->next){
				if (bl->type == BL_PC)
					c++;
			}
		}
	}

	return c;
}*/

/*==========================================
 * セル上のPCとMOBの数を数える (グランドクロス用)
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

/*
 * ｫｻｫ�ﾟｾｪﾎｪﾋﾌｸｪﾄｪｱｪｿｫｹｫｭｫ�ｫ讚ﾋｫﾃｫﾈｪｹ
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
 * map m (x0,y0)-(x1,y1)内の全objに対して
 * funcを呼ぶ
 * type!=0 ならその種類のみ
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

	map_freeblock_lock(); // メモリからの解放を禁止する

	for(i = blockcount; i < bl_list_count; i++)
		if (bl_list[i]->prev) // 有効かどうかチェック
			func(bl_list[i], ap);

	map_freeblock_unlock(); // 解放を許可する

	va_end(ap);
	bl_list_count = blockcount;
}

/*==========================================
 * 矩形(x0,y0)-(x1,y1)が(dx,dy)移動した時の
 * 領域外になる領域(矩形かL字形)内のobjに
 * 対してfuncを呼ぶ
 *
 * dx,dyは-1,0,1のみとする（どんな値でもいいっぽい？）
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
		// 矩形領域の場合
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
		// L字領域の場合
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

	map_freeblock_lock(); // メモリからの解放を禁止する

	for(i = blockcount; i < bl_list_count; i++)
		if (bl_list[i]->prev) // 有効かどうかチェック
			func(bl_list[i], ap);

	map_freeblock_unlock(); // 解放を許可する

	va_end(ap);
	bl_list_count = blockcount;
}

/*// -- moonsoul	(added map_foreachincell which is a rework of map_foreachinarea but
//			 which only checks the exact single x/y passed to it rather than an
//			 area radius - may be more useful in some instances)
//
void map_foreachincell(int (*func)(struct block_list*,va_list), int m, int x, int y, int type,...) {
	int bx, by;
	struct block_list *bl;
	va_list ap;
	int blockcount = bl_list_count, i;

	va_start(ap,type);

	by = y / BLOCK_SIZE;
	bx = x / BLOCK_SIZE;

	// any blocks
	if (type == 0) {
		for(bl = map[m].block[bx + by * map[m].bxs]; bl; bl = bl->next) {
			if (bl->x == x && bl->y == y && bl_list_count < BL_LIST_MAX)
				bl_list[bl_list_count++] = bl;
		}
		for(bl = map[m].block_mob[bx + by * map[m].bxs]; bl; bl = bl->next) {
			if (bl->x == x && bl->y == y && bl_list_count < BL_LIST_MAX)
				bl_list[bl_list_count++] = bl;
		}
	// only monsters
	} else if (type == BL_MOB) {
		for(bl = map[m].block_mob[bx + by * map[m].bxs]; bl; bl = bl->next) {
			if (bl->x == x && bl->y == y && bl_list_count < BL_LIST_MAX)
				bl_list[bl_list_count++] = bl;
		}
	// only 1 type, except monsters
	} else {
		for(bl = map[m].block[bx + by * map[m].bxs]; bl; bl = bl->next) {
			if (bl->type != type)
				continue;
			if (bl->x == x && bl->y == y && bl_list_count < BL_LIST_MAX)
				bl_list[bl_list_count++] = bl;
		}
	}

	map_freeblock_lock(); // メモリからの解放を禁止する

	for(i = blockcount; i < bl_list_count; i++)
		if (bl_list[i]->prev) // 有効かどうかチェック
			func(bl_list[i],ap);

	map_freeblock_unlock(); // 解放を許可する

	va_end(ap);
	bl_list_count = blockcount;
}*/

/*============================================================
* For checking a path between two points (x0, y0) and (x1, y1)
*------------------------------------------------------------
 */
void map_foreachinpath(int (*func)(struct block_list*, va_list), int m, int x0, int y_0, int x1, int y_1, int range, int type, ...) { // range not used
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
	map_freeblock_lock(); // メモリからの解放を禁止する

	for(i = blockcount; i < bl_list_count; i++)
		if (bl_list[i]->prev) // 有効かどうかチェック
			func(bl_list[i], ap);

	map_freeblock_unlock(); // 解放を許可する

	va_end(ap);
	bl_list_count = blockcount;
}

/*==========================================
 * 床アイテムやエフェクト用の一時obj割り当て
 * object[]への保存とid_db登録まで
 *
 * bl->idもこの中で設定して問題無い?
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
 * 一時objectの解放
 *	map_delobjectのfreeしないバージョン
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
 * 一時objectの解放
 * block_listからの削除、id_dbからの削除
 * object dataのfree、object[]へのNULL代入
 *
 * addとの対称性が無いのが気になる
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
 * 全一時obj相手にfuncを呼ぶ
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
//		if (bl_list[i]->prev || bl_list[i]->next)
		if (bl_list[i]->prev)
			func(bl_list[i], ap);

	map_freeblock_unlock();

	va_end(ap);
	bl_list_count = blockcount;
}

/*==========================================
 * 床アイテムを消す
 *
 * data==0の時はtimerで消えた時
 * data!=0の時は拾う等で消えた時として動作
 *
 * 後者は、map_clearflooritem(id)へ
 * map.h内で#defineしてある
 *------------------------------------------
 */
int map_clearflooritem_timer(int tid, unsigned int tick, int id, int data) { // if data is set, timer is called before to be executed -> delete it
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
 * (m,x,y)の周囲rangeマス内の空き(=侵入可能)cellの
 * 内から適当なマス目の座標をx+(y<<16)で返す
 *
 * 現状range=1でアイテムドロップ用途のみ
 *------------------------------------------
 */
int map_searchrandfreecell(int m, int x, int y, int range) {
	int free_cell, i, j;

	for(free_cell = 0, i = -range; i <= range; i++) {
		if(i + y < 0 || i + y >=map[m].ys)
			continue;
		for(j = - range; j <= range; j++){
			if (j + x < 0 || j + x >= map[m].xs)
				continue;
			if (map_getcell(m, j + x, i + y, CELL_CHKNOPASS))
				continue;
			free_cell++;
		}
	}
	if(free_cell==0)
		return -1;
	free_cell=rand()%free_cell;
	for(i=-range;i<=range;i++){
		if(i+y<0 || i+y>=map[m].ys)
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
 * (m,x,y)を中心に3x3以内に床アイテム設置
 *
 * item_dataはamount以外をcopyする
 *------------------------------------------
 */
int map_addflooritem(struct item *item_data, int amount, int m, int x, int y, struct map_session_data *first_sd,
    struct map_session_data *second_sd, struct map_session_data *third_sd, int owner_id, int type) {
	int xy, r;
	unsigned int tick;
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
	
	tick = gettick();
	if (first_sd) {
		fitem->first_get_id = first_sd->bl.id;
		if (type)
			fitem->first_get_tick = tick + battle_config.mvp_item_first_get_time;
		else
			fitem->first_get_tick = tick + battle_config.item_first_get_time;
	}
	if (second_sd) {
		fitem->second_get_id = second_sd->bl.id;
		if (type)
			fitem->second_get_tick = tick + battle_config.mvp_item_first_get_time + battle_config.mvp_item_second_get_time;
		else
			fitem->second_get_tick = tick + battle_config.item_first_get_time + battle_config.item_second_get_time;
	}
	if (third_sd) {
		fitem->third_get_id = third_sd->bl.id;
		if(type)
			fitem->third_get_tick = tick + battle_config.mvp_item_first_get_time + battle_config.mvp_item_second_get_time + battle_config.mvp_item_third_get_time;
		else
			fitem->third_get_tick = tick + battle_config.item_first_get_time + battle_config.item_second_get_time + battle_config.item_third_get_time;
	}
	fitem->owner = owner_id; // who drop the item (to authorize to get item at any moment)

	memcpy(&fitem->item_data, item_data, sizeof(*item_data));
	fitem->item_data.amount = amount;
	fitem->subx = (r & 3) * 3 + 3;
	fitem->suby = ((r >> 2) & 3) * 3 + 3;
	//fitem->cleartimer = add_timer(gettick() + battle_config.flooritem_lifetime, map_clearflooritem_timer, fitem->bl.id, 0);
	fitem->cleartimer = add_timer(tick + battle_config.flooritem_lifetime, map_clearflooritem_timer, fitem->bl.id, 0);

	map_addblock(&fitem->bl);
	clif_dropflooritem(fitem);

	return fitem->bl.id;
}

/*==========================================
 * charid_dbへ追加(返信待ちがあれば返信)
 *------------------------------------------
 */
void map_addchariddb(int charid, char *name) {
	struct charid2nick *p;
	int req;

	p = numdb_search(charid_db, charid);
	if (p == NULL) { // データベースにない
		CALLOC(p, struct charid2nick, 1);
		p->req_id = 0;
	} else
		numdb_erase(charid_db, charid);

	req = p->req_id;
	memset(p->nick, 0, sizeof(p->nick));
	strncpy(p->nick, name, 24);
	p->req_id = 0;
	numdb_insert(charid_db, charid, p);
	if (req) { // 返信待ちがあれば返信
		struct map_session_data *sd = map_id2sd(req);
		if (sd != NULL)
			clif_solved_charname(sd, charid);
	}

	return;
}

/*==========================================
 * charid_dbへ追加（返信要求のみ）
 *------------------------------------------
 */
int map_reqchariddb(struct map_session_data * sd,int charid) {
	struct charid2nick *p;

	nullpo_retr(0, sd);

	p = numdb_search(charid_db, charid);
	if(p!=NULL) // データベースにすでにある
		return 0;
	CALLOC(p, struct charid2nick, 1);
	p->req_id=sd->bl.id;
	numdb_insert(charid_db, charid, p);

	return 0;
}

/*==========================================
 * id_dbへblを追加
 *------------------------------------------
 */
void map_addiddb(struct block_list *bl) {
	nullpo_retv(bl);

	numdb_insert(id_db, bl->id, bl);
}

/*==========================================
 * id_dbからblを削除
 *------------------------------------------
 */
void map_deliddb(struct block_list *bl) {
	nullpo_retv(bl);

	numdb_erase(id_db, bl->id);
}

/*==========================================
 * nick_dbへsdを追加
 *------------------------------------------
 */
/*void map_addnickdb(struct map_session_data *sd) {
	nullpo_retv(sd);

	strdb_insert(nick_db, sd->status.name, sd); // not used
}*/

/*==========================================
 * PCのquit処理 map.c内分
 *
 * quit処理の主体が違うような気もしてきた
 *------------------------------------------
 */
void map_quit(struct map_session_data *sd) {
	nullpo_retv(sd);

	if (sd->state.event_disconnect) {
		struct npc_data *npc;
		if ((npc = npc_name2id(script_config.logout_event_name))) {
			run_script(npc->u.scr.script, 0, sd->bl.id, npc->bl.id); // PCLogoutNPC
#ifdef __DEBUG
			printf("Event '" CL_WHITE "%s" CL_RESET "' executed.\n", script_config.logout_event_name);
#endif
		}
	}

	if (sd->chatID) // チャットから出る
		chat_leavechat(sd);

	if (sd->trade_partner) // 取引を中断する
		trade_tradecancel(sd);

	if (sd->friend_num > 0)
		clif_friend_send_online(sd, 0);

	if (sd->party_invite > 0) // パーティ勧誘を拒否する
		party_reply_invite(sd, sd->party_invite_account, 0); // 0: invitation was denied, 1: accepted

	if (sd->guild_invite > 0) // ギルド勧誘を拒否する
		guild_reply_invite(sd, sd->guild_invite, 0);
	if (sd->guild_alliance > 0) // ギルド同盟勧誘を拒否する
		guild_reply_reqalliance(sd, sd->guild_alliance_account, 0); // flag: 0 deny, 1:accepted

	party_send_logout(sd); // パーティのログアウトメッセージ送信

	guild_send_memberinfoshort(sd, 0); // ギルドのログアウトメッセージ送信

	pc_cleareventtimer(sd); // イベントタイマを破棄する

	if (sd->state.storage_flag)
		storage_guild_storage_quit(sd);
	else
		storage_storage_quit(sd); // 倉庫を開いてるなら保存する

	skill_castcancel(&sd->bl, 0); // 詠唱を中断する
	skill_stop_dancing(&sd->bl, 1);// ダンス/演奏中断

	//Status that are not saved...
	if(sd->sc_count) {
		if(sd->sc_data[SC_HIDING].timer != -1)
			status_change_end(&sd->bl, SC_HIDING, -1);
		if(sd->sc_data[SC_CLOAKING].timer != -1)
			status_change_end(&sd->bl, SC_CLOAKING, -1);
		if (sd->sc_data[SC_BLADESTOP].timer != -1)
			status_change_end(&sd->bl, SC_BLADESTOP, -1);
		if(sd->sc_data[SC_RUN].timer != -1)
			status_change_end(&sd->bl, SC_RUN,-1);
		if(sd->sc_data[SC_SPURT].timer != -1)
			status_change_end(&sd->bl, SC_SPURT, -1);
		if(sd->sc_data[SC_CONCENTRATION].timer != -1) //Aegis X.2 does not saves it when logging off
			status_change_end(&sd->bl, SC_CONCENTRATION, -1);
		if(sd->sc_data[SC_BERSERK].timer != -1) {
			status_change_end(&sd->bl, SC_BERSERK, -1);
			sd->status.hp = 100;
		}
		if(sd->sc_data[SC_RIDING].timer != -1)
			status_change_end(&sd->bl, SC_RIDING, -1);
		if(sd->sc_data[SC_FALCON].timer != -1)
			status_change_end(&sd->bl, SC_FALCON, -1);
		if(sd->sc_data[SC_WEIGHT50].timer != -1)
			status_change_end(&sd->bl, SC_WEIGHT50, -1);
		if(sd->sc_data[SC_WEIGHT90].timer != -1)
			status_change_end(&sd->bl, SC_WEIGHT90, -1);
		if(sd->sc_data[SC_BROKNWEAPON].timer != -1)
			status_change_end(&sd->bl, SC_BROKNWEAPON, -1);
		if(sd->sc_data[SC_BROKNARMOR].timer != -1)
			status_change_end(&sd->bl, SC_BROKNARMOR, -1);
		if(sd->sc_data[SC_GUILDAURA].timer != -1)
			status_change_end(&sd->bl, SC_GUILDAURA, -1);
		if(sd->sc_data[SC_GOSPEL].timer != -1)
			status_change_end(&sd->bl, SC_GOSPEL, -1);
		// Marionette Control: Remove the effect off your target when logging out - [Aalye]
		if(sd->sc_data[SC_MARIONETTE].timer != -1) {
			struct block_list *bl = map_id2bl(sd->sc_data[SC_MARIONETTE].val3);
			status_change_end(bl, SC_MARIONETTE2, -1);
		}
	}

	skill_clear_unitgroup(&sd->bl); // スキルユニットグループの削除
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
	skill_unit_move(&sd->bl, gettick() ,0);

	status_calc_pc(sd, 4);
//	skill_clear_unitgroup(&sd->bl);

	if (sd->status.option & OPTION_HIDE)
		clif_clearchar_area(&sd->bl, 4); // no display // need to send information to authorize people in area to go in the square
	else
		clif_clearchar_area(&sd->bl, 2); // warp display

#ifdef USE_SQL //TXT version of this is still in dev
	chrif_save_scdata(sd); // save status changes
#endif
	status_change_clear(&sd->bl, 1); // ステータス異常を解除する
	
	if (sd->status.pet_id > 0 && sd->pd) {
		pet_lootitem_drop(sd->pd, sd);
		if (sd->pet.intimate <= 0) {
			intif_delete_petdata(sd->status.pet_id);
			pet_remove_map(sd);
			//sd->pd = NULL; // done in pet_remove_map
			sd->status.pet_id = 0;
			sd->petDB = NULL;
		} else {
			intif_save_petdata(sd->status.account_id, &sd->pet);
			pet_remove_map(sd);
			//sd->pd = NULL; // done in pet_remove_map
			//sd->status.pet_id = 0; // DON'T reset this value, to save pet with Char (chrif_save)
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
//	strdb_erase(nick_db, sd->status.name); // not used
	if (numdb_search(charid_db, sd->status.char_id) != NULL) /* from freya' forum by LebrEf[TaVu] */
		numdb_erase(charid_db, sd->status.char_id);

	return;
}

/*==========================================
 * to free memory of dynamic allocation in session (even if player is auth or not)
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
 * id番号のPCを探す。居なければNULL
 *------------------------------------------
 */
struct map_session_data * map_id2sd(int id) {
// remove search from db, because:
// 1 - all players, npc, items and mob are in this db (to search, it's not speed, and search in session is more sure)
// 2 - DB seems not always correct. Sometimes, when a player disconnects, its id (account value) is not removed and structure
//     point to a memory area that is not more a session_data and value are incorrect (or out of available memory) -> crash
// replaced by searching in all session.
// by searching in session, we are sure that fd, session, and account exist.
/*
	struct block_list *bl;

	bl = numdb_search(id_db, id);
	if (bl && bl->type == BL_PC)
		return (struct map_session_data*)bl;

	return NULL;
*/
	int i;
	struct map_session_data *sd;

	for(i = 0; i < fd_max; i++)
		if (session[i] && (sd = session[i]->session_data) && sd->bl.id == id)
			return sd;

	return NULL;
}

/*==========================================
 * char_id番号の名前を探す
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
 * Search session data from a nick name
 * (without sensitive case if necessary)
 * return map_session_data pointer or NULL
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
 * id番号の物を探す
 * 一時objectの場合は配列を引くのみ
 *------------------------------------------
 */
struct block_list * map_id2bl(int id) {
	if (id >= 0 && id < sizeof(object) / sizeof(object[0]))
		return object[id];
	else
		return numdb_search(id_db, id);
}

/*==========================================
 * id_db内の全てにfuncを実行
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
 * map.npcへ追加 (warp等の領域持ちのみ)
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

void map_removenpc(void) {
	int i, m, n = 0;

	for(m = 0; m < map_num; m++) {
		for(i = 0; i < map[m].npc_num && i < MAX_NPC_PER_MAP; i++) {
			if (map[m].npc[i] != NULL) {
				clif_clearchar_area(&map[m].npc[i]->bl, 2);
				map_delblock(&map[m].npc[i]->bl);
				numdb_erase(id_db, map[m].npc[i]->bl.id);
				if (map[m].npc[i]->bl.subtype == SCRIPT) {
//					FREE(map[m].npc[i]->u.scr.script);
//					FREE(map[m].npc[i]->u.scr.label_list);
				}
				FREE(map[m].npc[i]);
				n++;
			}
		}
	}
	printf("Successfully removed and freed from memory: '" CL_WHITE "%d" CL_RESET "' NPCs.\n", n);

	return;
}

/*==========================================
 * map名からmap番号へ変換
 *------------------------------------------
 */
int map_mapname2mapid(const char *name) { // map id on this server (m == -1 if not in actual map-server)
	struct map_data *md;

	if (name == NULL || name[0] == '\0')
		return -1;

	md = strdb_search(map_db, name);

	// If we can't find the .gat map try .afm instead [celest]
	if (md == NULL && strstr(name, ".gat") != NULL) {
		char afm_name[strlen(name) + 1]; // +1 NULL
		memset(afm_name, 0, strlen(name) + 1); // +1 NULL
		strncpy(afm_name, name, strlen(name) - 4);
		strcat(afm_name, ".afm");
		md = strdb_search(map_db, afm_name);
	}

	if (md == NULL || md->gat == NULL)
		return -1;

	return md->m;
}

/*==========================================
 * 他鯖map名からip,port変換
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
 * Check if a map is in game (on any map-servers)
 * result:
 *        0+: id of server on actual map-server
 *        -1: doesn't exist
 *        -2: exist on another map-server
 *------------------------------------------
 */
short map_checkmapname(const char *mapname) {
	int m;
	struct map_data_other_server *mdos;

	if ((m = map_mapname2mapid(mapname)) >= 0) // map id on this server (m == -1 if not in actual map-server)
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
	if(s_dir == t_dir)
		return 0;
	switch(s_dir) {
		case 0:
			if(t_dir == 7 || t_dir == 1 || t_dir == 0)
				return 0;
			break;
		case 1:
			if(t_dir == 0 || t_dir == 2 || t_dir == 1)
				return 0;
			break;
		case 2:
			if(t_dir == 1 || t_dir == 3 || t_dir == 2)
				return 0;
			break;
		case 3:
			if(t_dir == 2 || t_dir == 4 || t_dir == 3)
				return 0;
			break;
		case 4:
			if(t_dir == 3 || t_dir == 5 || t_dir == 4)
				return 0;
			break;
		case 5:
			if(t_dir == 4 || t_dir == 6 || t_dir == 5)
				return 0;
			break;
		case 6:
			if(t_dir == 5 || t_dir == 7 || t_dir == 6)
				return 0;
			break;
		case 7:
			if(t_dir == 6 || t_dir == 0 || t_dir == 7)
				return 0;
			break;
	}

	return 1;
}

/*==========================================
 * 彼我の方向を計算
 *------------------------------------------
 */
int map_calc_dir(struct block_list *src, int x, int y) {
	int dir = 0;
	int dx, dy;

	nullpo_retr(0, src);

	dx = x - src->x;
	dy = y - src->y;
	if (dx == 0 && dy == 0) { // 彼我の場所一致
		dir = 0; // 上
	} else if (dx >= 0 && dy >= 0) { // 方向的に右上
		dir = 7; // 右上
		if (dx * 3 - 1 < dy) dir = 0; // 上
		if (dx > dy * 3) dir = 6; // 右
	} else if (dx >= 0 && dy <= 0) { // 方向的に右下
		dir = 5; // 右下
		if (dx * 3 - 1 < -dy) dir = 4; // 下
		if (dx > -dy * 3) dir = 6; // 右
	} else if (dx <= 0 && dy <= 0) { // 方向的に左下
		dir = 3; // 左下
		if (dx * 3 + 1 > dy) dir = 4; // 下
		if (dx < dy * 3) dir = 2; // 左
	} else { // 方向的に左上
		dir = 1; // 左上
		if (-dx * 3 - 1 < dy) dir = 0; // 上
		if (-dx > dy * 3) dir = 2; // 左
	}

	return dir;
}

// gat系
/*==========================================
 * (m,x,y)の状態を調べる
 *------------------------------------------
 */
int map_getcell(int m, int x, int y, cell_t cellchk) {
	//return (m < 0 || m > MAX_MAP_PER_SERVER) ? 0 : map_getcellp(&map[m], x, y, cellchk);
	return (m < 0 || m >= map_num) ?
	                                 ((cellchk == CELL_CHKNOPASS || cellchk == CELL_CHKNOPASS_NPC) ? 1 : 0) :
	                                 map_getcellp(&map[m], x, y, cellchk);
}

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
 * (m,x,y)の状態をtにする
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
 * 他鯖管理のマップをdbに追加
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
			// 読み込んでいたけど、担当外になったマップ
			CALLOC(mdos, struct map_data_other_server, 1);
			strncpy(mdos->name, name, 24);
			mdos->gat  = NULL;
			mdos->ip   = ip;
			mdos->port = port;
			mdos->map  = md;
			strdb_insert(map_db, mdos->name, mdos);
			// printf("from char server : %s -> %08lx:%d\n",name,ip,port);
		} else {
			// 読み込んでいて、担当になったマップ（何もしない）
			;
		}
	} else {
		mdos = (struct map_data_other_server *)md;
		if (ip == clif_getip() && port == clif_getport()) {
			// 自分の担当になったマップ
			if (mdos->map == NULL) {
				// 読み込んでいないので終了する
				printf("map_setipport : %s is not loaded.\n", name);
				exit(1);
			} else {
				// 読み込んでいるので置き換える
				md = mdos->map;
				FREE(mdos);
				strdb_insert(map_db, md->name, md);
			}
		} else {
			// 他の鯖の担当マップなので置き換えるだけ
			mdos->ip   = ip;
			mdos->port = port;
		}
	}

	return 0;
}

/*==========================================
 * 他鯖管理のマップを全て削除
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

int map_eraseallipport(void) {
	strdb_foreach(map_db, map_eraseallipport_sub);

	return 1;
}

/*==========================================
 * 他鯖管理のマップをdbから削除
 *------------------------------------------
 */
int map_eraseipport(char *name, unsigned long ip, int port) {
	struct map_data *md;
	struct map_data_other_server *mdos;
//	unsigned char *p = (unsigned char *)&ip;

	md=strdb_search(map_db, name);
	if (md) {
		if (md->gat) // local -> check data
			return 0;
		else {
			mdos = (struct map_data_other_server *)md;
			if (mdos->ip == ip && mdos->port == port) {
				if (mdos->map) {
					// このマップ鯖でも読み込んでいるので移動できる
					return 1; // 呼び出し元で chrif_sendmap() をする
				} else {
					strdb_erase(map_db, name);
					FREE(mdos);
				}
//				if (battle_config.etc_log)
//					printf("erase map %s %d.%d.%d.%d:%d\n", name, p[0], p[1], p[2], p[3], port);
			}
		}
	}

	return 0;
}

// 初期化周り
/*==========================================
 * 水場高さ設定
 *------------------------------------------
 */
static struct waterlist {
	char mapname[17]; // 16 + NULL
	int waterheight;
} *waterlist = NULL;
int waterlist_num = 0;
#define NO_WATER 1000000

static int map_waterheight(char *mapname) {
	int i;

	for(i = 0; i < waterlist_num; i++)
		if (strcmp(waterlist[i].mapname, mapname) == 0)
			return waterlist[i].waterheight;

	return NO_WATER;
}

static void map_readwater(char *watertxt) {
	char line[1024], w1[1024];
	FILE *fp;

	fp = fopen(watertxt, "r");
	if (fp == NULL) {
		printf("map_readwater: file not found: %s.\n", watertxt);
		return;
	}

	while(fgets(line, sizeof(line), fp)) { // fgets reads until maximum one less than size and add '\0' -> so, it's not necessary to add -1
		int wh, count;
		if ((line[0] == '/' && line[1] == '/') || line[0] == '\0' || line[0] == '\n' || line[0] == '\r')
			continue;
		// it's not necessary to remove 'carriage return ('\n' or '\r')
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

	printf("File '" CL_WHITE "%s" CL_RESET "' readed ('" CL_WHITE "%d" CL_RESET "' entrie%s).\n", watertxt, waterlist_num, (waterlist_num > 1) ? "s" : "");
}

/*==========================================
* マップキャッシュに追加する
*===========================================*/

// マップキャッシュの最大値
#define MAX_MAP_CACHE 768

//各マップごとの最小限情報を入れるもの、READ_FROM_BITMAP用
struct map_cache_info {
	char fn[32];//ファイル名
	int xs, ys; //幅と高さ
	int water_height;
	int pos; // データが入れてある場所
	int compressed; // zilb通せるようにする為の予約
	int compressed_len; // zilb通せるようにする為の予約
}; // 56 byte

struct map_cache_head {
	int sizeof_header;
	int sizeof_map;
	// 上の２つ改変不可
	int nmaps; // マップの個数
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
			// キャッシュ読み込み成功
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

	// 読み込みに失敗したので新規に作成する
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

int map_cache_read(struct map_data *m) {
	int i;

	if(!map_cache.fp)
		return 0;

	for(i = 0; i < map_cache.head.nmaps; i++) {
		if (!strcmp(m->name, map_cache.map[i].fn)) {
			if (map_cache.map[i].water_height != map_waterheight(m->name)) {
				// 水場の高さが違うので読み直し
				return 0;
			} else if (map_cache.map[i].compressed == 0) {
				int size = map_cache.map[i].xs * map_cache.map[i].ys;
				m->xs = map_cache.map[i].xs;
				m->ys = map_cache.map[i].ys;
				CALLOC(m->gat, char, m->xs * m->ys);
				fseek(map_cache.fp, map_cache.map[i].pos, SEEK_SET);
				if (fread(m->gat, 1, size,map_cache.fp) == size) {
					// 成功
					return 1;
				} else {
					// なぜかファイル後半が欠けてるので読み直し
					m->xs = 0;
					m->ys = 0;
					FREE(m->gat);
					return 0;
				}
			} else if (map_cache.map[i].compressed == 1) {
				// 圧縮フラグ=1 : zlib
				char *buf;
				unsigned long dest_len;
				int size_compress = map_cache.map[i].compressed_len;
				m->xs = map_cache.map[i].xs;
				m->ys = map_cache.map[i].ys;
				MALLOC(m->gat, char, m->xs * m->ys);
				MALLOC(buf, char, size_compress);
				fseek(map_cache.fp, map_cache.map[i].pos, SEEK_SET);
				if (fread(buf, 1, size_compress,map_cache.fp) != size_compress) {
					// なぜかファイル後半が欠けてるので読み直し
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
					// 正常に解凍が出来てない
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

static int map_cache_write(struct map_data *m) {
	int i;
	unsigned long len_new, len_old;
	char *write_buf;

	if (!map_cache.fp)
		return 0;

	for(i = 0; i < map_cache.head.nmaps; i++) {
		if (!strcmp(m->name, map_cache.map[i].fn)) {
			// 同じエントリーがあれば上書き
			if (map_cache.map[i].compressed == 0) {
				len_old = map_cache.map[i].xs * map_cache.map[i].ys;
			} else if (map_cache.map[i].compressed == 1) {
				len_old = map_cache.map[i].compressed_len;
			} else {
				// サポートされてない形式なので長さ０
				len_old = 0;
			}
			if (map_read_flag >= READ_FROM_BITMAP_COMPRESSED) { // if (map_read_flag == 2) { ???
				// 圧縮保存
				// さすがに２倍に膨れる事はないという事で
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
				// サイズが同じか小さくなったので場所は変わらない
				fseek(map_cache.fp, map_cache.map[i].pos, SEEK_SET);
				fwrite(write_buf, 1, len_new, map_cache.fp);
			} else {
				// 新しい場所に登録
				fseek(map_cache.fp, map_cache.head.filesize, SEEK_SET);
				fwrite(write_buf, 1, len_new, map_cache.fp);
				map_cache.map[i].pos = map_cache.head.filesize;
				map_cache.head.filesize += len_new;
			}
			map_cache.map[i].xs  = m->xs;
			map_cache.map[i].ys  = m->ys;
			map_cache.map[i].water_height = map_waterheight(m->name);
			map_cache.dirty = 1;
			if (map_read_flag >= READ_FROM_BITMAP_COMPRESSED) { // if (map_read_flag == 2) { ???
				FREE(write_buf);
			}
			return 0;
		}
	}
	// 同じエントリが無ければ書き込める場所を探す
	for(i = 0; i < map_cache.head.nmaps; i++) {
		if (map_cache.map[i].fn[0] == 0) {
			// 新しい場所に登録
			if (map_read_flag >= READ_FROM_BITMAP_COMPRESSED) { // if (map_read_flag == 2) { ???
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
			if (map_read_flag >= READ_FROM_BITMAP_COMPRESSED) { // if (map_read_flag == 2) { ???
				FREE(write_buf);
			}
			return 0;
		}
	}

	// 書き込めなかった
	return 1;
}

static int map_readafm(int m, char *fn) {
	char line[65536];
	int x, xs = 0;
	int y, ys = 0;
	size_t size;
	int tmp_int[256];
	char *str;
	FILE *fp;

	fp = fopen(fn, "r");
	if (fp == NULL) {
		printf(CL_RED "Error: can't open file: %s." CL_RESET "\n",fn);
		return 0;
	} else {

		printf("Loading Maps [%d/%d]: " CL_WHITE "%-30s  " CL_RESET "\r", m, map_num, fn);
		fflush(stdout);

		fgets(line, sizeof(line), fp); // fgets reads until maximum one less than size and add '\0' -> so, it's not necessary to add -1
//		if (strcmp(line,"ADVANCED FUSION MAP") != 0) {
//			break;
//		}

		fgets(line, sizeof(line), fp); // fgets reads until maximum one less than size and add '\0' -> so, it's not necessary to add -1
//		if (strcmp(line,fn) < 0) {
//			break;
//		}

		map[m].m = m;

		str = fgets(line, sizeof(line), fp); // fgets reads until maximum one less than size and add '\0' -> so, it's not necessary to add -1
		sscanf(str , "%d%d", &tmp_int[0], &tmp_int[1]);
		xs = tmp_int[0];
		ys = tmp_int[1];
		if (xs == 0 || ys == 0) // ?? but possible
			return -1;

		map[m].xs = xs;
		map[m].ys = ys;

		CALLOC(map[m].gat, char, xs * ys);

		map[m].npc_num = map[m].users = 0;

		memset(&map[m].flag, 0, sizeof(map[m].flag));
		if (battle_config.pk_mode) // make all maps pvp for pk_mode [Valaris]
			map[m].flag.pvp = 1;
		map[m].flag.nogmcmd = 100; // All gm commands can be used

		fgets(line, sizeof(line), fp); // fgets reads until maximum one less than size and add '\0' -> so, it's not necessary to add -1

		for (y = 0; y < ys; y++) {
			for (x = 0; x < xs; x++) {
				map[m].gat[x+y] = getc(fp) - 48;
			}
		}
	}

	fclose(fp);

	map[m].bxs = (xs + BLOCK_SIZE - 1) / BLOCK_SIZE;
	map[m].bys = (ys + BLOCK_SIZE - 1) / BLOCK_SIZE;

	size = map[m].bxs * map[m].bys;

	CALLOC(map[m].block, struct block_list*, size);
	CALLOC(map[m].block_mob, struct block_list*, size);

	strdb_insert(map_db, map[m].name, &map[m]);

	return 0;
}

/*==========================================
 * マップ1枚読み込み
 *------------------------------------------
 */
static int map_readmap(int m, char *fn, char *alias, int *map_cache_count) {
	unsigned char *gat;
	int x, y, xs, ys, y_xs;
	struct gat_1cell {float high[4]; int type;} *p;
	int wh;
	size_t size;

	if (map_cache_read(&map[m])) {
		// キャッシュから読み込めた
		(*map_cache_count)++;
	} else {
		// read & convert fn
		gat = grfio_read(fn);
		if (gat == NULL) {
			printf("Map '%s' not found: " CL_WHITE "removed" CL_RESET " from maplist.\n", fn);
			return -1;
		}
		xs = *(int*)(gat + 6);
		ys = *(int*)(gat + 10);
		if (xs == 0 || ys == 0) { // ?? but possible
			printf(CL_YELLOW "Invalid size for map '%s'" CL_RESET " (xs,ys: %d,%d): " CL_WHITE "removed" CL_RESET " from maplist.\n", fn, xs, ys);
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
					// 水場判定
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

	printf("Loading Maps [%d/%d]: " CL_WHITE "%-30s  " CL_RESET "\r", m, map_num, fn);
	fflush(stdout);

	map[m].m = m;
	map[m].npc_num = 0;
	map[m].users = 0;
	memset(&map[m].flag, 0, sizeof(map[m].flag));
	if (battle_config.pk_mode) // make all maps pvp for pk_mode [Valaris]
		map[m].flag.pvp = 1;
	map[m].flag.nogmcmd = 100; // All gm commands can be used

	map[m].bxs = (map[m].xs + BLOCK_SIZE - 1) / BLOCK_SIZE;
	map[m].bys = (map[m].ys + BLOCK_SIZE - 1) / BLOCK_SIZE;

	size = map[m].bxs * map[m].bys;

	CALLOC(map[m].block, struct block_list *, size);
	CALLOC(map[m].block_mob, struct block_list *, size);

	if (alias != NULL) { // [MouseJstr]
		strdb_insert(map_db, alias, &map[m]);
	} else {
		strdb_insert(map_db, map[m].name, &map[m]);
	}

//	printf("%s read done.\n",fn);

	return 0;
}

/*==========================================
 * 読み込むmapを削除する
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
			//printf("Removing map [ %s ] from maplist.\n", map[i].name);
			FREE(map[i].alias); // [Yor]
			memmove(map + i, map + i + 1, sizeof(struct map_data) * (map_num - i - 1));
			map_num--;
			if (map_num == 0) {
				FREE(map);
			} else {
				REALLOC(map, struct map_data, map_num);
			}
		}
	}

	return;
}

/*==========================================
 * 全てのmapデータを読み込む
 *------------------------------------------
 */
int map_readallmap(void) {
	int i, maps_removed = 0;
	char fn[256];
	FILE *afm_file;
	int map_cache_count = 0;
	char *p;

	// マップキャッシュを開く
	if (map_read_flag >= READ_FROM_BITMAP)
		map_cache_open(map_cache_file);

	printf("Loading Maps%s...\n",
	       (map_read_flag == CREATE_BITMAP_COMPRESSED ? " (Generating Map Cache w/ Compression)" :
	        map_read_flag == CREATE_BITMAP ? " (Generating Map Cache)" :
	        map_read_flag >= READ_FROM_BITMAP ? " (w/ Map Cache)" :
	        map_read_flag == READ_FROM_AFM ? " (w/ AFM)" : ""));

	// 先に全部のャbプの存在を確認
	for(i = 0; i < map_num; i++) {
		char afm_name[256] = "";
		if (!strstr(map[i].name, ".afm")) {
			// check if it's necessary to replace the extension - speeds up loading abit
			strncpy(afm_name, map[i].name, strlen(map[i].name) - 4);
			strcat(afm_name, ".afm");
		}
		map[i].alias = NULL;

		sprintf(fn, "%s\\%s", afm_dir, afm_name);
		for(p = &fn[0]; *p != 0; p++) // * At the time of Unix
			if (*p == '\\')
				*p = '/';

		afm_file = fopen(fn, "r");
		if (afm_file != NULL) {
			map_readafm(i, fn);
			fclose(afm_file);
		} else if (strstr(map[i].name, ".gat") != NULL) {
			p = strstr(map[i].name, "<"); // [MouseJstr]
			if (p != NULL) {
				char buf[64];
				*p++ = '\0';
				sprintf(buf, "data\\%s", p);
				CALLOC(map[i].alias, char, strlen(buf) + 1); // +1 for NULL
				strcpy(map[i].alias, buf);
			}

			sprintf(fn, "data\\%s", map[i].name);
			//printf("map to read: %s\n", fn);
			if (map_readmap(i, fn, p, &map_cache_count) == -1) {
				map_delmap(map[i].name);
				maps_removed++;
				i--;
			}
		}
	}

	FREE(waterlist);

	if (maps_removed) {
		printf("Successfully loaded " CL_WHITE "%d maps" CL_RESET " on %d %40s\n", map_num, map_num + maps_removed, "");
		if (map_cache_count > 1)
			printf("of which were " CL_WHITE "%d maps" CL_RESET " in cache file.\n", map_cache_count);
		else
			printf("of which was " CL_WHITE "%d map" CL_RESET " in cache file.\n", map_cache_count);
		printf("Maps Removed: " CL_WHITE "%d" CL_RESET ".\n", maps_removed);
	} else {
		printf("Successfully loaded " CL_WHITE "%d maps" CL_RESET "%40s\n", map_num, "");
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
 * 読み込むmapを追加する
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

	memset(map[map_num].name, 0, sizeof(map[map_num].name));
	strncpy(map[map_num].name, mapname, 16); // 17 - NULL
	map_num++;

	return;
}

/*================================
 * Console Command Parser by [Yor]
 *--------------------------------
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

	/* get param of command */
	sscanf(buf, "%s %[^\n]", command, param);

/*	printf("Console command: %s %s\n", command, param); */

	if (!console_on) {

		if (strcasecmp("?", command) == 0 ||
		    strcasecmp("h", command) == 0 ||
		    strcasecmp("help", command) == 0 ||
		    strcasecmp("aide", command) == 0) {
			printf(CL_DARK_GREEN "Help of commands:" CL_RESET "\n");
			printf("  '" CL_DARK_CYAN "?|h|help|aide" CL_RESET "': Display this help.\n");
			printf("  '" CL_DARK_CYAN "<console password>" CL_RESET "': enable console mode.\n");
		} else if (strcmp(console_pass, command) == 0) {
			printf(CL_DARK_CYAN "Console commands are now enabled." CL_RESET "\n");
			console_on = 1;
		} else {
			printf(CL_DARK_CYAN "Console commands are disabled. Please enter the password." CL_RESET "\n");
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
			printf(CL_DARK_CYAN "Console: " CL_RESET CL_BOLD "I'm Alive for %u seconds." CL_RESET "\n", (int)(time(NULL) - start_time));
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
			printf("  To use GM commands: ");
			printf("  " CL_DARK_CYAN "<gm command>:<map_of_\"gm\"> <x> <y>" CL_RESET "\n");
			printf("  You can use any GM command that doesn't require the GM.\n");
			printf("  No using @item or @warp however you can use @charwarp.\n");
			printf("  The <map_of_\"gm\"> <x> <y> is for commands that need coords of the GM.\n");
			printf("  IE: @spawn\n");
			printf("  '" CL_DARK_CYAN "shutdown|exit|qui|end" CL_RESET "': To shutdown the server.\n");
			printf("  '" CL_DARK_CYAN "alive|status|uptime" CL_RESET "': To know if server is alive.\n");
			printf("  '" CL_DARK_CYAN "consoleoff" CL_RESET "': To disable console commands.\n");
			printf("  '" CL_DARK_CYAN "?|h|help|aide" CL_RESET "': To display help.\n");
			printf("  '" CL_DARK_CYAN "version" CL_RESET "': To display version of the server.\n");
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
				printf(CL_DARK_CYAN "Console commands are now disabled." CL_RESET "\n");
				console_on = 0;
			}
		} else {
			n = sscanf(buf, "%[^:]:%99s %d %d[^\n]", command, mapname, &x, &y);

//			printf("Command: %s || Map: %s Coords: %d %d\n", command, mapname, x, y);

			m = 0;
			if (n > 1) {
				if (strstr(mapname, ".gat") == NULL && strstr(mapname, ".afm") == NULL && strlen(mapname) < 13) // 16 - 4 (.gat)
					strcat(mapname, ".gat");
				if ((m = map_mapname2mapid(mapname)) < 0) { // map id on this server (m == -1 if not in actual map-server)
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
 * remove ended comments
 *------------------------------------------
 */
void remove_ended_comments(char *line) {
	int i;

	// removed endline comments
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
 * 設定ファイルを読み込む
 *------------------------------------------
 */
int map_config_read(char *cfgName) {
	char line[1024], w1[1024], w2[1024];
	FILE *fp;
	struct hostent *h = NULL;

	if ((fp = fopen(cfgName, "r")) == NULL) {
//		if ((fp = fopen("conf/map_athena.conf", "r")) == NULL) { // not try default, possible infinite loop with import
			printf("Map configuration file not found at: %s\n", cfgName);
			exit(1);
//		}
	}

	while(fgets(line, sizeof(line), fp)) { // fgets reads until maximum one less than size and add '\0' -> so, it's not necessary to add -1
		if ((line[0] == '/' && line[1] == '/') || line[0] == '\0' || line[0] == '\n' || line[0] == '\r')
			continue;
		// it's not necessary to remove 'carriage return ('\n' or '\r')

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
					printf("Character server IP address: " CL_WHITE "%s" CL_RESET " -> " CL_WHITE "%d.%d.%d.%d" CL_RESET "\n", w2, (unsigned char)h->h_addr[0], (unsigned char)h->h_addr[1], (unsigned char)h->h_addr[2], (unsigned char)h->h_addr[3]);
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
					printf("Map server IP address: " CL_WHITE "%s" CL_RESET " -> " CL_WHITE "%d.%d.%d.%d" CL_RESET "\n", w2, (unsigned char)h->h_addr[0], (unsigned char)h->h_addr[1], (unsigned char)h->h_addr[2], (unsigned char)h->h_addr[3]);
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
				printf("Removing map [ %s ] from maplist.\n", w2);
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
					printf("NPC language not changed (bad value detected : len > 5).\n");
				} else {
					memset(npc_language, 0, sizeof(npc_language));
					strcpy(npc_language, w2);
					//printf("Set NPC language to '%s'.\n", npc_language);
				}
			} else if (strcasecmp(w1, "npc_charset") == 0) {
				remove_ended_comments(w2); // remove ended comments
				if (strlen(w2) > 19) {
					printf("NPC charset not changed (bad value detected : len > 19)\n");
				} else {
					memset(npc_charset, 0, sizeof(npc_charset));
					strcpy(npc_charset, w2);
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
// import
			} else if (strcasecmp(w1, "import") == 0) {
				remove_ended_comments(w2); // remove ended comments
				printf("map_config_read: Import file: %s.\n", w2);
				map_config_read(w2);
			}
		}
	}
	fclose(fp);

	return 0;
}

#ifndef TXT_ONLY
/*=======================================
 *  MySQL Init
 *---------------------------------------
 */
int map_sql_init(void){
	// DB connection start
	sql_init();

	return 0;
}

/*=======================================
 *  MySQL Close
 *---------------------------------------
 */
int map_sql_close(void){
	sql_close();
	printf("Close Map DB Connection...\n");

	return 0;
}

/*=======================================
 *  MySQL Configuration
 *---------------------------------------
 */
int sql_config_read(char *cfgName) {
	char line[1024], w1[1024], w2[1024];
	FILE *fp;

	if ((fp = fopen(cfgName, "r")) == NULL) {
//		if ((fp = fopen("conf/inter_athena.conf", "r")) == NULL) { // not try default, possible infinite loop with import
			printf("File not found: %s.\n", cfgName);
			return 1;
//		}
	}
	while(fgets(line, sizeof(line), fp)) { // fgets reads until maximum one less than size and add '\0' -> so, it's not necessary to add -1
		if ((line[0] == '/' && line[1] == '/') || line[0] == '\0' || line[0] == '\n' || line[0] == '\r')
			continue;
		// it's not necessary to remove 'carriage return ('\n' or '\r')

		memset(w2, 0, sizeof(w2));
		if (sscanf(line, "%[^:]: %[^\r\n]", w1, w2) != 2)
			continue;

		if (strcasecmp(w1, "item_db_db") == 0) {
			strcpy(item_db_db, w2);
		} else if (strcasecmp(w1, "mob_db_db") == 0) {
			strcpy(mob_db_db, w2);
		} else if (strcasecmp(w1, "char_db") == 0) {
			strcpy(char_db, w2);

		//Map Server SQL DB
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

		//Map server option to use SQL db or not
		} else if (strcasecmp(w1, "use_sql_db") == 0) {
			db_use_sqldbs = config_switch(w2);
			printf ("Using SQL dbs: " CL_WHITE "%s" CL_RESET ".\n", w2);

// import
		} else if (strcasecmp(w1, "import") == 0) {
			printf("sql_config_read: Import file: %s.\n", w2);
			sql_config_read(w2);
		}
	}
	fclose(fp);

	printf("File '" CL_WHITE "%s" CL_RESET "' readed.\n", cfgName);

	return 0;
}
#endif /* not TXT_ONLY */

int id_db_final(void *k, void *d, va_list ap){ return 0; }
int map_db_final(void *k, void *d, va_list ap){ return 0; }

/*int nick_db_final(void *k, void *d, va_list ap) { // not used
	FREE(d);

	return 0;
}*/

int charid_db_final(void *k, void *d, va_list ap) {
	FREE(d);

	return 0;
}

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
 * map鯖終了時処理
 *------------------------------------------
 */
void do_final(void) {
	int map_id, i, j;
	struct map_session_data *sd;
	unsigned int tick_cache;
	
	printf("Terminating...\n");

	map_cache_close();

	pc_guardiansave(); // save guardians (if necessary)

	// save all online players
	j = 0;
	tick_cache = gettick();
	for (i = 0; i < fd_max; i++) {
		if (session[i] && (sd = session[i]->session_data) && sd->state.auth) {
			if (sd->last_saving + 30000 < tick_cache) { // not save if previous saving was done recently // to limit successive savings with auto-save
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
//	map_removenpc();
	timer_final();

	do_final_msg_config();
	do_final_script();
	do_final_itemdb();
	do_final_storage();
	do_final_guild();
	do_final_pet();

	for(i = 0; i < map_num; i++) {
		FREE(map[i].gat);
		FREE(map[i].block);
		FREE(map[i].block_mob);
		FREE(map[i].drop_list);
	}
	FREE(map);

	numdb_final(id_db, id_db_final);
	strdb_final(map_db, map_db_final);
//	strdb_final(nick_db, nick_db_final); // not used
	numdb_final(charid_db, charid_db_final);

	/* restore console parameters */
	term_input_disable();

#ifndef TXT_ONLY
	if (battle_config.mail_system || db_use_sqldbs)
		map_sql_close();
#endif /* not TXT_ONLY */

#ifdef __WIN32
	// close windows sockets
	WSACleanup();
#endif /* __WIN32 */

	printf("Finished.\n");
}

/*======================================================
 * Map-Server Init and Command-line Arguments
 *------------------------------------------------------
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

#if ENABLE_NLS
/* GetText init
 *
 * GetText is used for NPCs internationalisation.
 * It may also be used later for help, etc...
 *
 * NOTE: setlocale() does not work on cygwin. However we
 * can use putenv() to force the locale. We'll will
 * still prefer using setlocale() if available since
 * it's the standard way of changing locale.
 */
void gettext_language_init() {
#ifndef WITH_C99
	char env_var[50]; // max -> 'LC_CTYPE=' (9) + strlen(npc_language) (6) + NULL (1)
#endif
	char *cur_locale;
	cur_locale = setlocale(LC_ALL, ""); // use local definition instead of "C" definition
	cur_locale = setlocale(LC_CTYPE, ""); // use local definition instead of "C" definition
	//cur_locale = setlocale(LANG, ""); // LANG can not be changed by setlocale function.

#ifndef WITH_C99
	sprintf(env_var, "LANG=%s", npc_language);
	putenv(env_var);
	sprintf(env_var, "LC_ALL=%s", npc_language);
	putenv(env_var);
	sprintf(env_var, "LC_CTYPE=%s", npc_language); // for string comparaison, char sorting...
	putenv(env_var);
#endif

#if !defined __CYGWIN || defined WITH_C99
	cur_locale = setlocale(LC_ALL, npc_language);
	if (!cur_locale) {
		printf("Could not change system locale to : " CL_WHITE "%s" CL_RESET ".\n", npc_language);
		printf("This locale is probably not in your localedef.\n");
		return;
	}
	printf("Initializing GetText to use language " CL_WHITE "%s" CL_RESET ".\n", cur_locale);
	cur_locale = setlocale(LC_CTYPE, npc_language); // for string comparaison, char sorting...
#else /* NOT __CYGWIN */
	printf("Forcing GetText to use language " CL_WHITE "%s" CL_RESET ".\n", npc_language);
#endif /* NOT __CYGWIN */

	bindtextdomain("npcs", "locales");
	bind_textdomain_codeset("npcs", npc_charset);
	textdomain("npcs");
}
#endif /* ENABLE_NLS */

/*======================================================
 * Map-Server Init and Command-line Arguments [Valaris]
 *------------------------------------------------------
 */
void do_init(const int argc, char *argv[]) {
	int i;

#ifndef TXT_ONLY
	char *SQL_CONF_NAME = "conf/inter_athena.conf";
#endif
	char *MAP_CONF_NAME = "conf/map_athena.conf";
	char *BATTLE_CONF_FILENAME = "conf/battle_athena.conf";
	char *ATCOMMAND_CONF_FILENAME = "conf/atcommand_athena.conf";
	char *SCRIPT_CONF_NAME = "conf/script_athena.conf";
	char *GRF_PATH_FILENAME = "conf/grf-files.txt";
	FILE *data_conf;
	char line[1024], w1[1024], w2[1024];

	printf("The map-server is starting...\n");

	memset(messages_filename, 0, sizeof(messages_filename));
	strncpy(messages_filename, "conf/msg_athena.conf", sizeof(messages_filename) - 1);

	srand(gettick());

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
		else if (strcmp(argv[i], "--run_once") == 0) // close the map-server as soon as its done.. for testing [Celest]
			runflag = 0;
	}

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
#if ENABLE_NLS
	gettext_language_init();
#endif /* ENABLE_NLS */
#ifdef DYNAMIC_LINKING
	/* load all addons */
	addons_enable_all();
#endif /* DYNAMIC_LINKING */

	atexit(do_final);

	id_db = numdb_init();
	map_db = strdb_init(17); // 16 + NULL
//	nick_db = strdb_init(25); // 24 + NULL // not used
	charid_db = numdb_init();
#ifdef USE_SQL
	if (battle_config.mail_system || db_use_sqldbs) {
		map_sql_init();
#ifdef USE_MYSQL
		if (optimize_table) {
			printf("The map-server optimizes SQL tables: `%s` and `%s`...\r", item_db_db, mob_db_db);
			sql_request("OPTIMIZE TABLE `%s`, `%s`", item_db_db, mob_db_db);
			printf("SQL tables (`%s` and `%s`) have been optimized.        \n", item_db_db, mob_db_db);
		}
#endif /* USE_MYSQL */
	} else
		printf("Mysql connection not initialized (you don't use mob/item db or mail system).\n");
#endif /* USE_SQL */

	grfio_init(GRF_PATH_FILENAME);

	// check battle option in command line (--<battle_option>)
	for (i = 1; i < argc; i++) {
		if (argv[i + 1] != NULL)
			if (argv[i][0] == '-' && argv[i][1] == '-')
				battle_set_value(argv[i] + 2, argv[i+1]);
	}

	data_conf = fopen(GRF_PATH_FILENAME, "r");

	// It will read, if there is grf-files.txt.
	if (data_conf) {
		while(fgets(line, sizeof(line), data_conf)) { // fgets reads until maximum one less than size and add '\0' -> so, it's not necessary to add -1
			memset(w2, 0, sizeof(w2));
			if (sscanf(line, "%[^:]: %[^\r\n]", w1, w2) == 2) {
				if (strcmp(w1, "afm_dir") == 0)
					strcpy(afm_dir, w2);
			}
		}
		fclose(data_conf);
	} // end of reading grf-files.txt

	map_readallmap();

	add_timer_func_list(map_clearflooritem_timer, "map_clearflooritem_timer");

	do_init_chrif();
	do_init_clif();
	do_init_itemdb();
	do_init_mob(); // npcの初期化時内でmob_spawnして、mob_dbを参照するのでinit_npcより先
	do_init_script();
	do_init_pc();
	do_init_status();
	do_init_party();
	do_init_guild();
	do_init_storage();
	do_init_skill();
	do_init_pet();
	do_init_npc();

#ifdef USE_SQL /* mail system [Valaris] */
	if (battle_config.mail_system)
		do_init_mail();
#endif /* USE_SQL */

	npc_event_do_oninit(); // npcのOnInitイベント実行

	if (battle_config.pk_mode)
		printf("The server is running in " CL_RED "PK Mode" CL_RESET ".\n");

#ifdef __DEBUG
	printf("The map-server is running in " CL_WHITE "Debug Mode" CL_RESET ".\n");
#endif

	if (strcmp(listen_ip, "0.0.0.0") == 0) {
		//map_log("The map-server is ready (and is listening on the port %d - from any ip)." RETCODE, map_port);
		printf("The map-server is " CL_GREEN "ready" CL_RESET " (and is listening on the port " CL_WHITE "%d" CL_RESET " - from any ip).\n", map_port);
	} else {
		//map_log("The map-server is ready (and is listening on %s:%d)." RETCODE, listen_ip, map_port);
		printf("The map-server is " CL_GREEN "ready" CL_RESET " (and is listening on " CL_WHITE "%s:%d" CL_RESET ").\n", listen_ip, map_port);
	}

	/* console */
	if (console) {
		start_console(parse_console);
		if (term_input_status == 0) {
			printf("Sorry, but console commands can not be initialized -> disabled.\n\n");
			console = 0;
		} else {
			printf("Console commands are enabled. Type " CL_BOLD "help" CL_RESET " to have a little help.\n\n");
		}
	} else
		printf(CL_DARK_CYAN "Console commands are OFF/disactivated. You can not enter any console commands." CL_RESET "\n\n");

	return;
}
