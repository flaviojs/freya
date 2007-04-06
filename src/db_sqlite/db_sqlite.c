/* db_sqlite.c : interface to SQLite for Nezumi
 * $Id: db_sqlite.c 474 2006-03-27 22:03:19Z MagicalTux $
 * Partially based on Gabuzomeu's work
 */

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../common/utils.h"
#include <nezumi_sql.h>
#include "sqlite3.h"

struct _sqlite_result {
	struct _sqlite_result *next;
	char **colname;
	char **content;
};

static sqlite3 *sqlite_handle;
static char inbuf[MAX_SQL_BUFFER];
static char last_request[MAX_SQL_BUFFER];
static struct _sqlite_result *sqlite_result = NULL;
static int sqlite_first_row;
static int sqlite_num_rows;

#ifdef __DEBUG
static void priv_sqlite_log_bad_query(char *query, char *msg) {
	FILE *out;

	out = fopen("log/sqlite_query.log", "a");
	if (!out) return;
	fprintf(out, "Query: %s\nError: %s\n\n", query, msg);
	fclose(out);
}
#endif

static void priv_sqlite_return_now(sqlite3_context *context, int argn, sqlite3_value **rc) {
	char date_res[24]; // 0000-00-00 00:00:00 = 19 char - but we never know :)
	struct tm *tm;
	int len;
	time_t now;

	time(&now);
	tm = localtime(&now);
	len = sprintf((char *)&date_res, "%4d-%2d-%2d %2d:%2d:%2d", tm->tm_year, tm->tm_mon+1, tm->tm_mday,
			tm->tm_hour, tm->tm_min, tm->tm_sec);
	sqlite3_result_text(context, (char *)&date_res, len, SQLITE_TRANSIENT);
}

void sql_init() {
	int res;

	if (db_sqlite_database_file[0] == 0) {
		strcpy((char *)&db_sqlite_database_file, "save/nezumi.db");
	}
#ifdef __DEBUG
	printf("Loading SQLite database from %s\n", db_sqlite_database_file);
#endif
	res = sqlite3_open(db_sqlite_database_file, &sqlite_handle);
	if (res != 0) {
		printf("Could not open SQLite database: %s\n", sqlite3_errmsg(sqlite_handle));
		sqlite3_close(sqlite_handle);
		exit(1);
	}
	sqlite3_create_function(sqlite_handle, "NOW", 0, SQLITE_ANY, NULL, priv_sqlite_return_now, NULL, NULL);
}

void sql_close(void) {
#ifdef __DEBUG
	printf("Closing SQLite. Bye!\n");
#endif
	sqlite3_close(sqlite_handle);
}

static int priv_sqlite_callback(void *NotUsed, int argc, char **argv, char **azColName) {
	// insert result into set...
	struct _sqlite_result *new;
	if (sqlite_result) {
		new = sqlite_result;
		while(new->next) new=new->next;
		new->next = malloc(sizeof(struct _sqlite_result));
		new = new->next;
	} else {
		new = malloc(sizeof(struct _sqlite_result));
		sqlite_result = new;
	}
	new->next = NULL;
	new->colname = malloc((argc+1)*sizeof(void*));
	new->content = malloc(argc*sizeof(void*));
	for(int i=0;i<argc;i++) {
		new->colname[i] = strdup(azColName[i]);
		new->content[i] = (argv[i]==NULL?NULL:strdup(argv[i]));
	}
	new->colname[argc] = NULL; // "End of resultset" marker
	sqlite_num_rows++;
	return 0;
}

static int priv_sqlite_check_insert(char *inbuf) {
	char tmp_query[MAX_SQL_BUFFER];
	char base_query[MAX_SQL_BUFFER], tmp_query_2[MAX_SQL_BUFFER];
	int pos_inbuf, begin_pos, in_quotes, priv_result;
	char *sqlite_errmsg;
	/* first, scan the query
	 * It can have various forms :
	 * INSERT INTO ... (...) VALUES (...), (...), ...
	 * INSERT INTO ... VALUES (...), (...), ...
	 * INSERT INTO ... SET ...
	 */
	if (strncasecmp(inbuf, "INSERT INTO", 11) != 0) return 0; /* shouldn't have been called in that case */
	pos_inbuf = 12;
	while(*(inbuf+pos_inbuf)!=' ') {
		if (*(inbuf+pos_inbuf)==0) return 0; /* premature "end of query" */
		if (*(inbuf+pos_inbuf)=='(') break;
		pos_inbuf++;
	}
	while(*(inbuf+pos_inbuf)==' ') {
		if (*(inbuf+pos_inbuf)==0) return 0; /* premature "end of query" */
		if (*(inbuf+pos_inbuf)=='(') break;
		pos_inbuf++;
	}
	if (*(inbuf+pos_inbuf)=='(') {
		/* good boy, field list is present... well in fact it's annoying :p */
		while(*(inbuf+pos_inbuf)!=')') {
			if (*(inbuf+pos_inbuf)==0) return 0; /* premature "end of query" */
			pos_inbuf++;
		}
		pos_inbuf++;
		while(*(inbuf+pos_inbuf)==' ') {
			if (*(inbuf+pos_inbuf)==0) return 0; /* premature "end of query" */
			pos_inbuf++;
		}
	}
	/* now, we should be able to read "VALUES" */
	if (strncasecmp(inbuf+pos_inbuf, "VALUES", 6) != 0) return 0; /* wasn't this kind of query ? */
	pos_inbuf += 7;
	if (*(inbuf+pos_inbuf)==0) return 0; /* premature "end of query" */
	/* ok, now we have our base query... copy it to our buffer ! */
	memcpy(&base_query, inbuf, pos_inbuf);
	base_query[pos_inbuf] = 0; // NULL (end of string)
	/* now, read each element of the query ... */
	priv_result = 1;
	while(1) {
		/* move begin_pos to first ( */
		while(*(inbuf+pos_inbuf)!='(') {
			if (*(inbuf+pos_inbuf)==0) return priv_result; /* normal "end of query" */
			pos_inbuf++;
		}
		begin_pos = pos_inbuf;
		/* move to end pos */
		in_quotes = 0;
		while((in_quotes != 0) || (*(inbuf+pos_inbuf)!=')')) {
			if (*(inbuf+pos_inbuf)==0) return 0; /* premature "end of query" */
			if (in_quotes == 0) {
				if ((*(inbuf+pos_inbuf)=='\'') || (*(inbuf+pos_inbuf)=='"')) in_quotes = *(inbuf+pos_inbuf);
			} else if (*(inbuf+pos_inbuf) == in_quotes) {
				if (*(inbuf+pos_inbuf-1)!=in_quotes) in_quotes = 0; /* in sqlite, quote escaping is obtained by doubling the quote */
				/* TODO: this is not a perfect code... improve it in some way so it can handle cases such as '''' */
			}
			pos_inbuf++;
		}
		pos_inbuf++;
		/* copy the query part */
		memcpy(&tmp_query, (inbuf+begin_pos), pos_inbuf-begin_pos);
		tmp_query[pos_inbuf-begin_pos]=0;
		sprintf(tmp_query_2, "%s%s", base_query, tmp_query);
		if (sqlite3_exec(sqlite_handle, tmp_query_2, priv_sqlite_callback, NULL, &sqlite_errmsg) != SQLITE_OK) {
			printf("SQLERR[bulk]: Req: %s, Error: %s \n", last_request, sqlite_errmsg);
#ifdef __DEBUG
			priv_sqlite_log_bad_query(tmp_query_2, sqlite_errmsg);
#endif
			sqlite3_free(sqlite_errmsg);
			priv_result = 0;
		}
	}
	return priv_result;
}

inline int sql_request(const char *format, ...) {
	int request_with_result;
	va_list args;
	char *sqlite_errmsg;

	if (format == NULL || *format == '\0') return 0;

	va_start(args, format);
	vsprintf(inbuf, format, args);
	va_end(args);

	request_with_result = (strncasecmp(inbuf, "SELECT", 6) == 0 ||
			strncasecmp(inbuf, "OPTIMIZE", 8) == 0 ||
			strncasecmp(inbuf, "SHOW", 4) == 0);
	if (request_with_result) {
		while(sqlite_result != NULL) {
			struct _sqlite_result *tmp = sqlite_result->next;
			for(int i=0;sqlite_result->colname[i] != NULL;i++) {
				free(sqlite_result->colname[i]);
				free(sqlite_result->content[i]);
			}
			free(sqlite_result->colname);
			free(sqlite_result->content);
			free(sqlite_result);
			sqlite_result = tmp;
		}
		sqlite_first_row = 1;
		sqlite_num_rows = 0;
	}
	strcpy(last_request, inbuf);
	if (strncasecmp(inbuf, "INSERT INTO", 11) == 0) {
		/* Handle special case : many inserts in one query */
		if (priv_sqlite_check_insert(inbuf) != 0) return 1;
	}
	if (sqlite3_exec(sqlite_handle, inbuf, priv_sqlite_callback, NULL, &sqlite_errmsg) != SQLITE_OK) {
		printf("SQLERR: Req: %s, Error: %s \n", inbuf, sqlite_errmsg);
#ifdef __DEBUG
		priv_sqlite_log_bad_query(inbuf, sqlite_errmsg);
#endif
		sqlite3_free(sqlite_errmsg);
		return 0;
	}
	return 1;
}

unsigned long db_sql_escape_string(char *to, const char *from, unsigned long from_length) {
	char *pos=to;
	for (int i=0;i<from_length;i++) {
		*(pos++)=*(from+i);
		if (*(from+i)=='\'') *(pos++)='\'';
	}
	*pos=0;
	return pos-to;
}

int sql_get_row(void) {
	struct _sqlite_result *tmp;
	if (sqlite_result == NULL) return 0;
	if (sqlite_first_row != 0) {
		sqlite_first_row = 0;
		return 1;
	}
	tmp = sqlite_result->next;
	for(int i=0;sqlite_result->colname[i] != NULL;i++) {
		free(sqlite_result->colname[i]);
		free(sqlite_result->content[i]);
	}
	free(sqlite_result->colname);
	free(sqlite_result->content);
	free(sqlite_result);
	sqlite_result = tmp;
	if (sqlite_result == NULL) return 0;
	return 1;
}

char *sql_get_string(int num_col) {
	if (sqlite_result == NULL) return NULL;
	return sqlite_result->content[num_col];
}

int sql_get_integer(int num_col) {
	char *str = sql_get_string(num_col);
	if (str == NULL) return -1;
	return atoi(str);
}

unsigned long sql_num_rows(void) {
	return sqlite_num_rows;
}

