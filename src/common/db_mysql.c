// Copyright (c) Freya Development Team - Licensed under GNU GPL
// For more information, see License.txt in the main folder

#include <config.h>

#ifdef USE_MYSQL
	#undef MYSQL_DEBUG // Define that to get console output of queries

#include <stdio.h>
#include <time.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#ifdef __WIN32
	#define __USE_W32_SOCKETS
	#include <windows.h>
#endif

#include "utils.h"
#include "db_mysql.h"

static char last_request[MAX_SQL_BUFFER];

static MYSQL mysql_handle;
static MYSQL_RES* mysql_db_res = NULL;
static MYSQL_ROW mysql_db_row = NULL;

void sql_init() {

	memset(last_request, 0, sizeof(last_request));
	mysql_init(&mysql_handle);
	if (!mysql_real_connect(&mysql_handle, db_mysql_server_ip, db_mysql_server_id, db_mysql_server_pw,
	    db_mysql_server_db, db_mysql_server_port, (char *)NULL, 0)) {
		/* pointer check */
		printf(CL_RED "[SQLERR]" CL_RESET " Can't connect to the database: %s.\n", mysql_error(&mysql_handle));
		exit(1);
	}

	return;
}

void sql_close(void) {

	mysql_close(&mysql_handle);

	return;
}

inline int sql_request(const char *format, ...) {
	char inbuf[MAX_SQL_BUFFER];
	int request_with_result;
	va_list args;

	if (format == NULL || *format == '\0')
		return 0;

	va_start(args, format);
	vsprintf(inbuf, format, args);
	va_end(args);

	request_with_result = (strncasecmp(inbuf, "SELECT", 6) == 0 ||
	                       strncasecmp(inbuf, "OPTIMIZE", 8) == 0 ||
	                       strncasecmp(inbuf, "SHOW", 4) == 0);

	if (request_with_result) {
		if (mysql_db_res) {
			mysql_free_result(mysql_db_res);
			mysql_db_res = NULL;
			mysql_db_row = NULL;
		}
	}

	strcpy(last_request, inbuf);

#ifdef MYSQL_DEBUG
	printf("Query: %s\n", inbuf);
#endif

	if (mysql_query(&mysql_handle, inbuf)) {

#ifdef MYSQL_DEBUG // Error logging on mysql_error.log
		FILE *stderr;
		stderr = fopen("log/mysql_error.log", "a");
		if (stderr != NULL) {
			fprintf(stderr, "[SQLERR] %s, Error: %s \n", last_request, mysql_error(&mysql_handle));
			fclose(stderr);
		}
#endif

		printf(CL_RED "[SQLERR]" CL_RESET " %s, Error: %s \n", last_request, mysql_error(&mysql_handle));
		return 0;
	}

	if (request_with_result)
		mysql_db_res = mysql_store_result(&mysql_handle);

	return 1;
}

int sql_get_row(void) {
	if (!mysql_db_res)
		return 0;

	mysql_db_row = mysql_fetch_row(mysql_db_res);

	if (!mysql_db_row)
		return 0;

	return 1;
}

char *sql_get_string(int num_col) {

	if (!mysql_db_res)
		return NULL;

	if (!mysql_db_row) {
		printf(CL_RED "[SQLERR]" CL_RESET " Access to null sql row ? (col: %d), last req:%s\n", num_col, last_request);
		return NULL;
	}

	if (mysql_db_row[num_col])
		return mysql_db_row[num_col];

	return NULL;
}

int sql_get_integer(int num_col) {

	if (!mysql_db_res)
		return -1;

	if (!mysql_db_row) {
		printf(CL_RED "[SQLERR]" CL_RESET " Access to null sql row ? (col: %d), last req:%s\n", num_col, last_request);
		return -1;
	}

	if (mysql_db_row[num_col])
		return atoi(mysql_db_row[num_col]);

	return 0;
}

unsigned long sql_num_rows(void) {
	if (mysql_db_res)
		return mysql_num_rows(mysql_db_res);

	return 0;
}
#endif /* USE_MYSQL */

