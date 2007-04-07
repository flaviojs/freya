// $Id: nezumi_sql.h 206 2006-02-06 15:20:53Z MagicalTux $
// Global SQL functions
// Based on Gabuzomeu's work

#include <config.h>

#ifndef _SQL_H_
#define _SQL_H_

#ifdef USE_SQL

#if 0
#include "utils.h"
#ifndef __WIN32
# include <mysql.h>
#else
# include <mysql/mysql.h>
#endif
#endif

#define MAX_SQL_BUFFER 65535 * 2 // memo is limited to 60000, x2 for escape code

#ifdef USE_MYSQL
// MySQL variables
unsigned int db_mysql_server_port;
char db_mysql_server_ip[1024]; /* configuration line are read for 1024 char */
char db_mysql_server_id[32];
char db_mysql_server_pw[32];
char db_mysql_server_db[32];
#endif /* USE_MYSQL */

#ifdef USE_SQLITE
char db_sqlite_database_file[256]; /* where the SQLite database will be stored */
#endif /* USE_SQLITE */

#ifdef USE_PGSQL
// PgSQL variables
char db_pgsql_conninfo[1024]; 
#endif

// Our functions
int sql_request(const char *format, ...);
void sql_init();
void sql_close(void);
int sql_get_row(void);
char *sql_get_string(int num_col);
int sql_get_integer(int num_col);
unsigned long sql_num_rows(void);
unsigned long db_sql_escape_string(char *to, const char *from, unsigned long from_length);

#endif /* USE_SQL */

#endif /* _SQL_H_ */

