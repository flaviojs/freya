/*	This file is a part of Freya.
		Freya is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	any later version.
		Freya is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.
		You should have received a copy of the GNU General Public License
	along with Freya; if not, write to the Free Software
	Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA */

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
