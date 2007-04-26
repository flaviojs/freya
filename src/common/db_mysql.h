// Copyright (c) Freya Development Team - Licensed under GNU GPL
// For more information, see License.txt in the main folder

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

