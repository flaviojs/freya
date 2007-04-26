// Copyright (c) Freya Development Team - Licensed under GNU GPL
// For more information, see License.txt in the main folder

#ifndef _LOGIN_H_
#define _LOGIN_H_

#include <mmo.h>

#define MAX_SERVERS 30 // max number of char-servers (and account_id values: 0 to max-1)

#define LOGIN_CONF_NAME "conf/login_freya.conf"

#define START_ACCOUNT_NUM 2000001
#define END_ACCOUNT_NUM 100000000

#define AUTH_FIFO_SIZE ((FD_SETSIZE > 1024) ? FD_SETSIZE : 1024) // max possible connections (previously: 1024)

struct {
	int account_id;
	int login_id1;
	int login_id2;
	unsigned int ip;
	unsigned sex : 3; /* 0/1, 2: server */
	unsigned delflag : 1; // 0: accepted, 1: not valid/not init
} auth_fifo[AUTH_FIFO_SIZE];

struct mmo_char_server {
	char name[21]; // 20 + NULL
	unsigned int ip;
	unsigned short port;
	unsigned short users;
	unsigned short maintenance;
	unsigned short new;
};

struct mmo_account {
	char userid[25]; // 24 + NULL
	char passwd[33]; // 32 + NULL
	unsigned passwdenc : 2; /* 0, 1, 2, 3 */
	unsigned sex : 3; /* 0/1, 2: server */
	int version; /* Added for version check */

	int account_id;
	int login_id1;
	int login_id2;
	char lastlogin[24]; // 2005-06-16 23:59:59.999 + NULL = 23+1 = 24
};

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
#ifdef TXT_ONLY
	unsigned short account_reg2_num; // 0 - 700 (ACCOUNT_REG2_NUM)
	struct global_reg *account_reg2;
#endif // TXT_ONLY
} *auth_dat;

extern char date_format[32];

int login_port;

int account_id_count;
int auth_num;
extern int add_to_unlimited_account; /* Give possibility or not to adjust (ladmin command: timeadd) the time of an unlimited account. */
extern unsigned char ladmin_min_GM_level; /* minimum GM level of a account for listGM/lsGM ladmin command */
unsigned char use_md5_passwds;
unsigned char unique_case_account_name_creation;

char login_log_unknown_packets_filename[512];

/* *** TXT *** CONFIGURATION */
#ifdef TXT_ONLY
unsigned char GM_level_need_save_flag;
#endif /* TXT_ONLY */

/* *** SQL *** CONFIGURATION */
#ifdef USE_SQL
char login_db[256];
#ifdef USE_MYSQL
#endif /* USE_MYSQL */
char login_db_account_id[256];
char login_db_userid[256];
char login_db_user_pass[256];
char login_db_level[256];
#endif /* USE_SQL */

int password_check(const char * password_db, const char * pass, const char * key);
int check_existing_account_name_for_creation(const char *name);
int search_account_index(const char* account_name);
int search_account_index2(const int account_id);
unsigned char check_online_player(const int account_id);
void accounts_speed_sorting(int tableau[], int premier, int dernier);
void save_account(int idx, unsigned char forced); /* forced: 1 = must be save, 0 = information is not important */
void delete_account(const char * account_name);
void send_GM_account(const int account_id, const unsigned char level);
void add_text_in_memo(int account, const char* text);
void charif_sendall(unsigned int len);
void account_to_str(char *str, struct auth_dat *p);
int add_account(struct auth_dat * account, const char *memo);
void init_new_account(struct auth_dat * account);

#ifndef __ADDON
struct mmo_char_server server[MAX_SERVERS]; // max number of char-servers (and account_id values: 0 to max-1)
int server_fd[MAX_SERVERS]; // max number of char-servers (and account_id values: 0 to max-1)
#endif

#endif // _LOGIN_H_
