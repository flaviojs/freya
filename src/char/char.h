// Copyright (c) Freya Development Team - Licensed under GNU GPL
// For more information, see License.txt in the main folder

#ifndef _CHAR_H_
#define _CHAR_H_

#define MAX_SERVERS 30 // max number of char-servers (and account_id values: 0 to max-1)
#define MAX_MAP_SERVERS 30

extern struct mmo_charstatus *char_dat;
extern signed char *online_chars; // same size of char_dat, and id value of current server (or 0)
extern int char_num;

struct mmo_map_server{ // used only in char.c
	long ip;
	unsigned short port;
	unsigned short map_num;
	unsigned char agit_flag; // 0: WoE not starting, Woe is running
	int users;
	char *map; // [MAX_MAP_PER_SERVER][17]; // 16 + NULL
};
extern int server_fd[MAX_MAP_SERVERS];

#ifdef USE_SQL
enum {
	TABLE_INVENTORY,
	TABLE_CART,
	TABLE_STORAGE,
	TABLE_GUILD_STORAGE
};
int memitemdata_to_sql(struct item *itemlist, int list_id, int tableswitch);
#endif /* USE_SQL */

int char_nick2idx(char* character_name); // return -1 if not found, index in char_dat[] if found

unsigned char mapif_sendall(unsigned int len);
unsigned char mapif_sendallwos(int fd, unsigned int len);
void mapif_send(int fd, unsigned int len);

void char_log(char *fmt, ...);

#ifdef USE_SQL
extern char char_db[256];
extern char cart_db[256];
extern char inventory_db[256];
extern char charlog_db[256];
extern char storage_db[256];
extern char interlog_db[256];
extern char global_reg_value[256];
extern char skill_db[256];
extern char memo_db[256];
extern char guild_db[256];
extern char guild_alliance_db[256];
extern char guild_castle_db[256];
extern char guild_expulsion_db[256];
extern char guild_member_db[256];
extern char guild_position_db[256];
extern char guild_skill_db[256];
extern char guild_storage_db[256];
extern char party_db[256];
extern char pet_db[256];
extern char friends_db[256];
extern char statuschange_db[256];
extern char rank_db[256];
#endif /* USE_SQL */

#endif // _CHAR_H_
