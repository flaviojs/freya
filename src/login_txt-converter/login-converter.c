// Copyright (c) Freya Development Team - Licensed under GNU GPL
// For more information, see License.txt in the main folder

// Include of configuration script
#include <config.h>

// Include of dependances
#ifdef __WIN32
#define __USE_W32_SOCKETS
#include <windows.h>
#else
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef __WIN32
# include <mysql.h>
#else
# include <mysql/mysql.h>
#endif

#include "../common/db_mysql.h"
#include <mmo.h>
#include "../common/version.h"
#include "../common/malloc.h"
#include "../common/utils.h"

/*--------------
  from login.h
--------------*/
#define LOGIN_CONF_NAME "conf/login_freya.conf"

#define START_ACCOUNT_NUM 2000001
#define END_ACCOUNT_NUM 100000000

struct auth_dat {
	int account_id;
	unsigned sex : 3; /* 0/1, 2: server */
	char userid[25], pass[33], lastlogin[24]; // 24 + NULL, 32 + NULL, 2005-06-16 23:59:59.999 + NULL = 23+1 = 24
	int logincount;
	short state; /* packet 0x006a value + 1 (0: compte OK) */
	char email[41]; // e-mail (by default: a@a.com) 40 + NULL
	char error_message[20]; // Message of error code #6 = Your are Prohibited to log in until %s (packet 0x006a) // 19 + NULL
	time_t ban_until_time; /* # of seconds 1/1/1970 (timestamp): ban time limit of the account (0 = no ban) */
	time_t connect_until_time; /* # of seconds 1/1/1970 (timestamp): Validity limit of the account (0 = unlimited) */
	char last_ip[16]; /* save of last IP of connection */
	char *memo; /* a memo field */
	unsigned char level; /* GM level */
	unsigned short account_reg2_num; // 0 - 700 (ACCOUNT_REG2_NUM)
	struct global_reg *account_reg2;
} *auth_dat = NULL;

int auth_num = 0;

char login_db[256];
char login_db_account_id[256];
char login_db_userid[256];
char login_db_user_pass[256];
char login_db_level[256];

/*--------------
  from login.c
--------------*/
int auth_max = 0;

/* *** TXT *** CONFIGURATION */
char account_filename[1024];
char GM_account_filename[1024];

/*-------------------------
  Adding an account in db
-------------------------*/
int add_account(struct auth_dat * account, const char *memo) {
	int memo_size;

	/* increase size of db if necessary */
	if (auth_num >= auth_max) {
		auth_max += 256;
		REALLOC(auth_dat, struct auth_dat, auth_max);
		memset(auth_dat + (auth_max - 256), 0, sizeof(struct auth_dat) * 256);
	}

	memcpy(&auth_dat[auth_num], account, sizeof(struct auth_dat));
	if (memo != NULL && ((memo_size = strlen(memo)) > 2 || (memo_size == 1 && memo[0] != '-'))) {
		// limit size of memo to 60000
		if (memo_size > 60000)
			memo_size = 60000;
		CALLOC(auth_dat[auth_num].memo, char, memo_size + 1); // + NULL
		memcpy(auth_dat[auth_num].memo, memo, memo_size);
	}

	auth_num++;

	return auth_num - 1;
}

/*--------------------------------------------------------------------------
  Reading of GM accounts file (and their level) - to conserv compatibility
--------------------------------------------------------------------------*/
static inline void read_gm_file(void) {
	char line[512];
	FILE *fp;
	int account_id, level;
	int i;
	int range, start_range, end_range;

	if ((fp = fopen(GM_account_filename, "r")) == NULL)
		return;

	while(fgets(line, sizeof(line), fp)) { // fgets reads until maximum one less than size and add '\0' -> so, it's not necessary to add -1
		if ((line[0] == '/' && line[1] == '/') || line[0] == '\0' || line[0] == '\n' || line[0] == '\r')
			continue;
		// it's not necessary to remove 'carriage return ('\n' or '\r')

		if ((range = sscanf(line, "%d%*[-~]%d %d", &start_range, &end_range, &level)) == 3 ||
		    (range = sscanf(line, "%d%*[-~]%d:%d", &start_range, &end_range, &level)) == 3 ||
		    (range = sscanf(line, "%d %d", &start_range, &level)) == 2 ||
		    (range = sscanf(line, "%d:%d", &start_range, &level)) == 2 ||
		    (range = sscanf(line, "%d: %d", &start_range, &level)) == 2) {
			if (level < 0)
				level = 0;
			else if (level > 99)
				level = 99;
			if (range == 2)
				end_range = start_range;
			else if (end_range < start_range) {
				i = end_range;
				end_range = start_range;
				start_range = i;
			}
			for (account_id = start_range; account_id <= end_range; account_id++) {
				for(i = 0; i < auth_num; i++)
					if (auth_dat[i].account_id == account_id) {
						auth_dat[i].level = level;
						break;
					}
			}
		}
	}
	fclose(fp);

	return;
}

/*-----------------------------------------------
  Initialisation of accounts database in memory
-----------------------------------------------*/
static inline void init_db(void) {
#define LINE_SIZE 65535
	int server_count = 0, GM_counter = 0;
	char *line;
	char *str;
	int i;

	FILE *fp;
	int account_id, logincount, state, level, n, j, v;
	char *p, *userid, *pass, *lastlogin, sex, *email, *error_message, *last_ip, *memo;
	long int ban_until_time, connect_until_time; // don't use time_t to avoid compilation error on some system
	struct auth_dat tmp_account;

	/* use dynamic allocation to reduce memory use when server is working */
	CALLOC(line, char, LINE_SIZE);
	CALLOC(str, char, LINE_SIZE);

	auth_num = 0;
	auth_max = 256;
	CALLOC(auth_dat, struct auth_dat, 256);

	if ((fp = fopen(account_filename, "r")) == NULL) {
		/* no account file -> no account -> no login, including char-server (ERROR) */
		printf(CL_RED "init_db: Accounts file [%s] not found." CL_RESET "\n", account_filename);
		FREE(line);
		FREE(str);
		return;
	}

	/* use dynamic allocation to reduce memory use when server is working */
	CALLOC(userid, char, LINE_SIZE);
	CALLOC(pass, char, LINE_SIZE);
	CALLOC(lastlogin, char, LINE_SIZE);
	CALLOC(email, char, LINE_SIZE);
	CALLOC(error_message, char, LINE_SIZE);
	CALLOC(last_ip, char, LINE_SIZE);
	CALLOC(memo, char, LINE_SIZE);

	/* loop to read all accounts */
	while(fgets(line, LINE_SIZE, fp) != NULL) { // fgets reads until maximum one less than size and add '\0' -> so, it's not necessary to add -1
		if ((line[0] == '/' && line[1] == '/') || line[0] == '\0' || line[0] == '\n' || line[0] == '\r')
			continue;

		/* remove carriage return if exist */
		while(line[0] != '\0' && (line[(i = strlen(line) - 1)] == '\n' || line[i] == '\r'))
			line[i] = '\0';

		p = line;

		memset(userid, 0, LINE_SIZE);
		memset(pass, 0, LINE_SIZE);
		memset(lastlogin, 0, LINE_SIZE);
		memset(email, 0, LINE_SIZE);
		memset(error_message, 0, LINE_SIZE);
		memset(last_ip, 0, LINE_SIZE);
		memset(memo, 0, LINE_SIZE);

		/* database version reading (v4) */
		if ((i = sscanf(line, "%d\t%[^\t]\t%[^\t]\t%[^\t]\t%c\t%d\t%d\t%[^\t]\t%[^\t]\t%ld\t%[^\t]\t%[^\t]\t%ld\t%d\t%n",
		                &account_id, userid, pass, lastlogin, &sex, &logincount, &state,
		                email, error_message, &connect_until_time, last_ip, memo, &ban_until_time, &level, &n)) == 14 ||
		/* database version reading (v3) */
		    (i = sscanf(line, "%d\t%[^\t]\t%[^\t]\t%[^\t]\t%c\t%d\t%d\t%[^\t]\t%[^\t]\t%ld\t%[^\t]\t%[^\t]\t%ld\t%n",
		                &account_id, userid, pass, lastlogin, &sex, &logincount, &state,
		                email, error_message, &connect_until_time, last_ip, memo, &ban_until_time, &n)) == 13 ||
		/* database version reading (v2) */
		    (i = sscanf(line, "%d\t%[^\t]\t%[^\t]\t%[^\t]\t%c\t%d\t%d\t%[^\t]\t%[^\t]\t%ld\t%[^\t]\t%[^\t]\t%n",
		                &account_id, userid, pass, lastlogin, &sex, &logincount, &state,
		                email, error_message, &connect_until_time, last_ip, memo, &n)) == 12 ||
		/* Old athena database version reading (v1) */
		    (i = sscanf(line, "%d\t%[^\t]\t%[^\t]\t%[^\t]\t%c\t%d\t%d\t%n",
		         &account_id, userid, pass, lastlogin, &sex, &logincount, &state, &n)) >= 5) {

			/* Some checks */
			if (account_id > END_ACCOUNT_NUM) {
				printf(CL_RED "init_db: ******Error: an account has an id higher than %d\n", END_ACCOUNT_NUM);
				printf(       "         account id #%d -> account not read (saved in log file)." CL_RESET "\n", account_id);
				continue;
			} else if (account_id < 0) {
				printf(CL_RED "init_db: ******Error: an account has an invalid id (%d)\n", account_id);
				printf(       "         account id #%d -> account not read (saved in log file)." CL_RESET "\n", account_id);
				continue;
			}
			userid[24] = '\0';
			remove_control_chars(userid);
			for(j = 0; j < auth_num; j++) {
				if (auth_dat[j].account_id == account_id) {
					printf(CL_RED "init_db: ******Error: an account has an identical id to another.\n");
					printf(       "         account id #%d -> new account not read (saved in log file)." CL_RESET "\n", account_id);
					break;
				} else if (strcmp(auth_dat[j].userid, userid) == 0) {
					printf(CL_RED "init_db: ******Error: account name already exists.\n");
					printf(       "         account name '%s' -> new account not read.\n", userid); /* 2 lines, account name can be long. */
					printf(       "         Account saved in log file." CL_RESET "\n");
					break;
				}
			}
			if (j != auth_num)
				continue;

			if (auth_num % 20 == 19) {
				printf("Reading account #%d...\r", auth_num + 1);
				fflush(stdout);
			}

			/* init account values */
			memset(&tmp_account, 0, sizeof(tmp_account)); // sizeof(tmp_account) = sizeof(struct auth_dat)

			/* set values of v1 */
			tmp_account.account_id = account_id;

			strncpy(tmp_account.userid, userid, 24); // 25 - NULL

			remove_control_chars(pass);
			strncpy(tmp_account.pass, pass, 32); // 33 - NULL

			remove_control_chars(lastlogin);
			strncpy(tmp_account.lastlogin, lastlogin, 23); // 24 - NULL

			tmp_account.sex = (sex == 'S' || sex == 's') ? 2 : (sex == 'M' || sex == 'm');

			if (i >= 6) {
				if (logincount >= 0)
					tmp_account.logincount = logincount;
				/* else
					tmp_account.logincount = 0; already to 0 */
			} /* else
				tmp_account.logincount = 0; already to 0 */

			if (i >= 7) {
				if (state > 255)
					tmp_account.state = 100;
				else if (state < 0) {
					/* tmp_account.state = 0; already to 0 */
				} else
					tmp_account.state = state;
			} /* else
				tmp_account.state = 0; already to 0 */

			/* set values of v2 */
			if (i < 8) {
				/* Initialization of new data */
				strcpy(tmp_account.email, "a@a.com");
				tmp_account.error_message[0] = '-';
				/* tmp_account.connect_until_time = 0; already to 0 */
				tmp_account.last_ip[0] = '-';
				/* strncpy(tmp_account.memo, "-", 65535); already to 0 */
			} else {
				if (e_mail_check(email) == 0) {
					printf("Account %s (%d): invalid e-mail (replaced by a@a.com).\n", tmp_account.userid, tmp_account.account_id);
					strcpy(tmp_account.email, "a@a.com");
				} else {
					remove_control_chars(email);
					strncpy(tmp_account.email, email, 40); // 41 - NULL
				}

				remove_control_chars(error_message);
				if (error_message[0] == '\0' || state != 7) /* 7, because state is packet 0x006a value + 1 */
					tmp_account.error_message[0] = '-';
				else
					strncpy(tmp_account.error_message, error_message, 19); // 20 - NULL

				tmp_account.connect_until_time = (time_t)connect_until_time;

				remove_control_chars(last_ip);
				strncpy(tmp_account.last_ip, last_ip, 15); // 16 - NULL

				/* memo[65535] = '\0'; not necessary to limit memo */
				remove_control_chars(memo);
				/* added in ad_account in db function */
			}

			/* set values of v3 */
			if (i >= 13)
				tmp_account.ban_until_time = (time_t)ban_until_time;
			/* else
				tmp_account.ban_until_time = 0; already to 0 */

			/* set values of v4 */
			if (i >= 14) {
				if (level > 99)
					tmp_account.level = 99;
				else if (level < 0) {
					/* tmp_account.level = 0; already to 0 */
				} else
					tmp_account.level = level;
			} /* else
				tmp_account.level = 0; already to 0 */

			for(j = 0; j < ACCOUNT_REG2_NUM; j++) {
				p += n;
				if (sscanf(p, "%[^\t,],%d %n", str, &v, &n) != 2) {
					/* We must check if a str is void. If it's, we can continue to read other REG2.
					   Account line will have something like: str2,9 ,9 str3,1 (here, ,9 is not good) */
					if (p[0] == ',' && sscanf(p, ",%d %n", &v, &n) == 1) {
						j--;
						continue;
					} else
						break;
				}
				remove_control_chars(str);
				if (j == 0) {
					MALLOC(tmp_account.account_reg2, struct global_reg, 1);
				} else {
					REALLOC(tmp_account.account_reg2, struct global_reg, j + 1);
				}
				strncpy(tmp_account.account_reg2[j].str, str, 32); // 33 - NULL
				tmp_account.account_reg2[j].str[32] = '\0';
				tmp_account.account_reg2[j].value = v;
			}
			tmp_account.account_reg2_num = j;

			/* add account in the database */
			add_account(&tmp_account, memo);

			/* count the number of server accounts */
			if (tmp_account.sex == 2)
				server_count++;
		}
	}
	fclose(fp);

	/* free dynamic allocation */
	FREE(userid);
	FREE(pass);
	FREE(lastlogin);
	FREE(email);
	FREE(error_message);
	FREE(last_ip);
	FREE(memo);

	read_gm_file();

	GM_counter = 0;
	for(i = 0; i < auth_num; i++)
		if (auth_dat[i].level > 0)
			GM_counter++;

	/* display informations */
	if (auth_num == 0) {
		printf("No account found in '%s'.\n", account_filename);
	} else {
		if (auth_num == 1) {
			printf("1 account found in '%s',\n", account_filename);
			sprintf(line, "1 account found in '%s',", account_filename);
		} else {
			printf("%d accounts found in '%s',\n", auth_num, account_filename);
			sprintf(line, "%d accounts found in '%s',", auth_num, account_filename);
		}
		if (GM_counter == 0) {
			printf("  of which is no GM account, and ");
			sprintf(str, "%s of which is no GM account and", line);
		} else if (GM_counter == 1) {
			printf("  of which is 1 GM account, and ");
			sprintf(str, "%s of which is 1 GM account and", line);
		} else {
			printf("  of which are %d GM accounts, and ", GM_counter);
			sprintf(str, "%s of which are %d GM accounts and", line, GM_counter);
		}
		if (server_count == 0) {
			printf("of which is no server account ('S').\n");
		} else if (server_count == 1) {
			printf("of which is 1 server account ('S').\n");
		} else {
			printf("of which are %d server accounts ('S').\n", server_count);
		}
	}

	/* free dynamic allocation */
	FREE(line);
	FREE(str);

	return;
}

/*---------------------------------------
  Writing of the accounts database file
---------------------------------------*/
static inline void write_account(void) {
	char line[65536];
	int tmp_sql_len;
	struct auth_dat *p;
	char account_name[49], pass[65], email[81], error_message[39]; // 24*2+NULL, 32*2+NULL, 40*2+NULL, 19*2+NULL
	char t_reg2[65]; // (32 * 2) + 1
	int i, j;

	t_reg2[64] = '\0';

	for (i = 0; i < auth_num; i++) {
		if (i % 20 == 19) {
			printf("Saving account #%d in SQL database...\r", i + 1);
			fflush(stdout);
		}
		p = &auth_dat[i];
		mysql_escape_string(account_name, p->userid, strlen(p->userid));
		mysql_escape_string(pass, p->pass, strlen(p->pass));
		mysql_escape_string(email, p->email, strlen(p->email));
		mysql_escape_string(error_message, p->error_message, strlen(p->error_message));
		if (strcmp(p->lastlogin, "-") == 0) {
			strcpy(p->lastlogin, "0000-00-00 00:00:00");
		}
		if (p->memo == NULL) {
			sql_request("REPLACE INTO `%s` (`%s`, `%s`, `%s`, `lastlogin`, `sex`, `logincount`, `email`, `%s`, "
			                               "`error_message`, `connect_until`, `last_ip`, `memo`, `ban_until`, `state`) "
			                       " VALUES ('%d', '%s', '%s', '%s', '%c', '%d', '%s', '%d', "
			                               "'%s', '%ld', '%s', '%s', '%ld', '%d')",
			login_db, login_db_account_id, login_db_userid, login_db_user_pass, login_db_level,
			p->account_id, account_name, pass, p->lastlogin, (p->sex == 2) ? 'S' : (p->sex ? 'M' : 'F'), p->logincount, email, p->level,
			error_message, (long int)p->connect_until_time, p->last_ip, "-", (long int)p->ban_until_time, p->state);
		} else {
			mysql_escape_string(line, p->memo, strlen(p->memo));
			sql_request("REPLACE INTO `%s` (`%s`, `%s`, `%s`, `lastlogin`, `sex`, `logincount`, `email`, `%s`, "
			                               "`error_message`, `connect_until`, `last_ip`, `memo`, `ban_until`, `state`) "
			                       " VALUES ('%d', '%s', '%s', '%s', '%c', '%d', '%s', '%d', "
			                               "'%s', '%ld', '%s', '%s', '%ld', '%d')",
			login_db, login_db_account_id, login_db_userid, login_db_user_pass, login_db_level,
			p->account_id, account_name, pass, p->lastlogin, (p->sex == 2) ? 'S' : (p->sex ? 'M' : 'F'), p->logincount, email, p->level,
			error_message, (long int)p->connect_until_time, p->last_ip, line, (long int)p->ban_until_time, p->state);
		}
		// write account_reg2
		tmp_sql_len = 0;
		for(j = 0; j < p->account_reg2_num; j++) {
			mysql_escape_string(t_reg2, p->account_reg2[j].str, strlen(p->account_reg2[j].str));
			if (tmp_sql_len == 0)
				tmp_sql_len = sprintf(line, "INSERT INTO `account_reg2_db`(`account_id`,`str`,`value`) VALUES ('%d', '%s', '%d')",
				                      p->account_id, t_reg2, p->account_reg2[j].value);
			else
				tmp_sql_len += sprintf(line + tmp_sql_len, ", ('%d', '%s', '%d')", p->account_id, t_reg2, p->account_reg2[j].value);
		}
		if (tmp_sql_len != 0) {
			//printf("%s\n", line);
			sql_request(line);
		}
	}

	if (auth_num == 0)
		printf("No account saved in SQL database.                             \n");
	else if (auth_num == 1)
		printf("1 account saved in SQL database.                              \n");
	else
		printf("%d accounts saved in SQL database.                            \n", auth_num);

	return;
}

/*------------------------------------
  Reading general configuration file
------------------------------------*/
static void login_config_read(const char *cfgName) { // not inline, called too often
	char line[1024], w1[1024], w2[1024];
	FILE *fp;

	if ((fp = fopen(cfgName, "r")) == NULL) {
//		if ((fp = fopen("conf/login_freya.conf", "r")) == NULL) { // not try default, possible infinite loop with import
			printf("Configuration file (%s) not found.\n", cfgName);
			return;
//		}
	}

	memset(line, 0, sizeof(line));
	while(fgets(line, sizeof(line), fp)) { // fgets reads until maximum one less than size and add '\0' -> so, it's not necessary to add -1
		if ((line[0] == '/' && line[1] == '/') || line[0] == '\0' || line[0] == '\n' || line[0] == '\r')
			continue;
		// it's not necessary to remove 'carriage return ('\n' or '\r')

		memset(w1, 0, sizeof(w1));
		memset(w2, 0, sizeof(w2));
		if (sscanf(line, "%[^:]: %[^\r\n]", w1, w2) == 2) {
			remove_control_chars(w1);
			remove_control_chars(w2);

			/* *** TXT *** CONFIGURATION */
			if (strcasecmp(w1, "account_filename") == 0) {
				memset(account_filename, 0, sizeof(account_filename));
				strncpy(account_filename, w2, sizeof(account_filename) - 1);
			} else if (strcasecmp(w1, "GM_account_filename") == 0) {
				memset(GM_account_filename, 0, sizeof(GM_account_filename));
				strncpy(GM_account_filename, w2, sizeof(GM_account_filename) - 1);

			/* *** SQL *** CONFIGURATION */
			} else if (strcasecmp(w1, "login_db") == 0) {
				memset(login_db, 0, sizeof(login_db));
				strncpy(login_db, w2, sizeof(login_db) - 1);
#ifdef USE_MYSQL
			/* add for DB connection */
			} else if (strcasecmp(w1, "mysql_server_ip") == 0) {
				memset(db_mysql_server_ip, 0, sizeof(db_mysql_server_ip));
				strncpy(db_mysql_server_ip, w2, sizeof(db_mysql_server_ip) - 1);
			} else if (strcasecmp(w1, "mysql_server_port") == 0) {
				db_mysql_server_port = atoi(w2);
			} else if (strcasecmp(w1, "mysql_server_id") == 0) {
				memset(db_mysql_server_id, 0, sizeof(db_mysql_server_id));
				strncpy(db_mysql_server_id, w2, sizeof(db_mysql_server_id) - 1);
			} else if (strcasecmp(w1, "mysql_server_pw") == 0) {
				memset(db_mysql_server_pw, 0, sizeof(db_mysql_server_pw));
				strncpy(db_mysql_server_pw, w2, sizeof(db_mysql_server_pw) - 1);
			} else if (strcasecmp(w1, "mysql_login_db") == 0) {
				memset(db_mysql_server_db, 0, sizeof(db_mysql_server_db));
				strncpy(db_mysql_server_db, w2, sizeof(db_mysql_server_db) - 1);
#endif /* USE_MYSQL */
			/* added for custom column names for custom login table */
			} else if (strcasecmp(w1, "login_db_account_id") == 0) {
				memset(login_db_account_id, 0, sizeof(login_db_account_id));
				strncpy(login_db_account_id, w2, sizeof(login_db_account_id) - 1);
			} else if (strcasecmp(w1, "login_db_userid") == 0) {
				memset(login_db_userid, 0, sizeof(login_db_userid));
				strncpy(login_db_userid, w2, sizeof(login_db_userid) - 1);
			} else if (strcasecmp(w1, "login_db_user_pass") == 0) {
				memset(login_db_user_pass, 0, sizeof(login_db_user_pass));
				strncpy(login_db_user_pass, w2, sizeof(login_db_user_pass) - 1);
			} else if (strcasecmp(w1, "login_db_level") == 0) {
				memset(login_db_level, 0, sizeof(login_db_level));
				strncpy(login_db_level, w2, sizeof(login_db_level) - 1);

			/* IMPORT FILES */
			} else if (strcasecmp(w1, "import") == 0) {
				printf("login_config_read: Import file: %s.\n", w2);
				login_config_read(w2);
			}
		}
	}
	fclose(fp);

	return;
}

/*-------------------------------------------
  Initialisation of configuration variables
--------------------------------------------*/
static inline void init_conf_variables(void) {
	/* *** TXT *** CONFIGURATION */
	memset(account_filename, 0, sizeof(account_filename));
	strcpy(account_filename, "save/account.txt");
	memset(GM_account_filename, 0, sizeof(GM_account_filename));
	strcpy(GM_account_filename, "conf/GM_account.txt");

	/* *** SQL *** CONFIGURATION */
	memset(login_db, 0, sizeof(login_db));
	strcpy(login_db, "login");
#ifdef USE_MYSQL
	/* add for DB connection */
	memset(db_mysql_server_ip, 0, sizeof(db_mysql_server_ip));
	strcpy(db_mysql_server_ip, "127.0.0.1");
	db_mysql_server_port = 3306;
	memset(db_mysql_server_id, 0, sizeof(db_mysql_server_id));
	strcpy(db_mysql_server_id, "ragnarok");
	memset(db_mysql_server_pw, 0, sizeof(db_mysql_server_pw));
	strcpy(db_mysql_server_pw, "ragnarok");
	memset(db_mysql_server_db, 0, sizeof(db_mysql_server_db));
	strcpy(db_mysql_server_db, "ragnarok");
#endif /* USE_MYSQL */
	/* added for custom column names for custom login table */
	memset(login_db_account_id, 0, sizeof(login_db_account_id));
	strcpy(login_db_account_id, "account_id");
	memset(login_db_userid, 0, sizeof(login_db_userid));
	strcpy(login_db_userid, "userid");
	memset(login_db_user_pass, 0, sizeof(login_db_user_pass));
	strcpy(login_db_user_pass, "user_pass");
	memset(login_db_level, 0, sizeof(login_db_level));
	strcpy(login_db_level, "level");

	return;
}

/*--------------------------------
  Main function of this software
--------------------------------*/
void do_init(const int argc, char **argv) {
	char input;
	int i;

	init_conf_variables(); /* init before to read file to avoid recursiv calls */
	login_config_read((argc > 1) ? argv[1] : LOGIN_CONF_NAME);

	printf("\nWarning : Make sure you backup your databases before continuing!\n");
	printf("\nDo you wish to convert your Login Database to SQL? (y/n) : ");
	input = getchar();

	if (input == 'y' || input == 'Y') {
		/* init db */
		init_db();
		/* open sql connection */
		sql_init();
		/* save db */
		write_account();
		/* close connections */
		sql_close();
		/* free memory */
		if (auth_dat != NULL) {
			for (i = 0; i < auth_num; i++) {
				FREE(auth_dat[i].memo);
				FREE(auth_dat[i].account_reg2);
			}
			FREE(auth_dat);
		}
	}

	exit(0);
}
