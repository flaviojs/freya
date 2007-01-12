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

#include <config.h>

#ifndef _SQL_H_
#define _SQL_H_

#ifdef USE_MYSQL

#include "utils.h"
#ifndef __WIN32
# include <mysql.h>
#else
# include <mysql/mysql.h>
#endif

#define MAX_SQL_BUFFER 65535 * 2 // memo is limited to 60000, x2 for escape code


// MySQL variables
unsigned int db_mysql_server_port;
char db_mysql_server_ip[1024]; /* configuration line are read for 1024 char */
char db_mysql_server_id[32];
char db_mysql_server_pw[32];
char db_mysql_server_db[32];

// Our functions
int sql_request(const char *format, ...);
void sql_init();
void sql_close(void);
int sql_get_row(void);
char *sql_get_string(int num_col);
int sql_get_integer(int num_col);
unsigned long sql_num_rows(void);
#endif /* USE_MYSQL */

#endif /* _SQL_H_ */

