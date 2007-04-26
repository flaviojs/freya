// Copyright (c) Freya Development Team - Licensed under GNU GPL
// For more information, see License.txt in the main folder

#ifndef _INTER_H_
#define _INTER_H_

#include <config.h>

void inter_init(const char *file);
int check_ttl_wisdata(int tid, unsigned int tick, int id, int data);
void inter_save();
int inter_parse_frommap(int fd);
void inter_mapif_init(int fd);

void mapif_parse_AccRegRequest(int fd, int account_id);

void inter_log(char *fmt, ...);

extern short party_share_level;
extern short guild_extension_bonus;

void send_alone_map_server_flag(); // send flag 'map_is_alone' to all map-servers

#ifdef USE_SQL
//add include for DBMS(mysql)
#include "../common/db_mysql.h"

extern char tmp_sql[MAX_SQL_BUFFER];
extern int tmp_sql_len;
#endif /* USE_SQL */

#endif // _INTER_H_
