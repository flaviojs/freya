// $Id: db_pgsql.c 209 2006-02-06 16:32:24Z MagicalTux $
// MySQL management functions
// Initial sourcecode by Gabuzomeu

#include <config.h>

#ifdef USE_PGSQL

#include <stdio.h>
#include <time.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#ifdef __WIN32
#define __USE_W32_SOCKETS
#include <windows.h>
#endif

#ifndef __WIN32
# include <postgresql/postgres_fe.h>
# include <postgresql/libpq-fe.h>
#else
# include <postgres_fe.h>
# include <libpq-fe.h>
#endif

#include "../common/utils.h"
#include <nezumi_sql.h>

static char last_request[MAX_SQL_BUFFER];

static PGconn* pgsql_handle;
static PGresult* pgsql_db_res = NULL;
static int pgsql_db_row = 0;

void sql_init() {
#ifdef __DEBUG
	printf("Init pgsql connection.\n");
#endif
	memset(last_request, 0, sizeof(last_request));
	pgsql_handle = PQconnectdb(db_pgsql_conninfo);
	if (pgsql_handle == NULL) {
		printf("Connect DB server error: internal error. Check available memory!\n");
		exit(1);
	}
	if (PQstatus(pgsql_handle)!=CONNECTION_OK) {
		printf("Connect DB server error: %s.\n", PQerrorMessage(pgsql_handle));
		PQfinish(pgsql_handle);
		exit(1);
	}

	return;
}

void sql_close(void) {
#ifdef __DEBUG
	printf("Closing pgsql connection.\n");
#endif
	if (pgsql_db_res != NULL) {
		PQclear(pgsql_db_res);
		pgsql_db_res = NULL;
		pgsql_db_row = 0;
	}
	PQfinish(pgsql_handle);

	return;
}

inline void priv_pgsql_adapt(char *req) {
	/* replace ` with " in req ... */
	while (*req) {
		if (*req == '`') *req='"';
		req++;
	}
}

char inbuf[MAX_SQL_BUFFER];
inline int sql_request(const char *format, ...) {
	va_list args;
	PGresult *res;

	if (format == NULL || *format == '\0')
		return 0;

	va_start(args, format);
	vsprintf(inbuf, format, args);
	va_end(args);

	priv_pgsql_adapt((char *)&inbuf);
	strcpy(last_request, inbuf);

#ifdef MYSQL_DEBUG
	printf("Query: %s\n", inbuf);
#endif
	res = PQexec(pgsql_handle, inbuf);
	if (PQresultStatus(res) == PGRES_COMMAND_OK) return 1; /* no data returned */
	if (PQresultStatus(res) != PGRES_TUPLES_OK) { /* some error occured */
		printf("SQLERR: Req: %s, Error: %s \n", last_request, PQresultErrorMessage(res));
		PQclear(res);
		return 0;
	}
	if (pgsql_db_res) {
		PQclear(pgsql_db_res);
		pgsql_db_res = NULL;
		pgsql_db_row = 0;
	}
	pgsql_db_res = res;

	return 1;
}

int sql_get_row(void) {
	if (! pgsql_db_res)
		return 0;

	if (pgsql_db_row >= PQntuples(pgsql_db_res)) return 0;
	pgsql_db_row++;

	return 1;
}

char *sql_get_string(int num_col) {
	if (! pgsql_db_res)
		return NULL;

	return PQgetvalue(pgsql_db_res, pgsql_db_row-1, num_col);
}

unsigned long db_sql_escape_string(char *to, const char *from, unsigned long from_length) {
	return PQescapeString(to, from, from_length);
}

int sql_get_integer(int num_col) {
	if (! pgsql_db_res)
		return -1;

	return atoi(PQgetvalue(pgsql_db_res, pgsql_db_row-1, num_col));
}

unsigned long sql_num_rows(void) {
	if (pgsql_db_res)
		return PQntuples(pgsql_db_res);

	return 0;
}
#endif /* USE_MYSQL */

