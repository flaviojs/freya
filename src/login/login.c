// Copyright (c) Freya Development Team - Licensed under GNU GPL
// For more information, see License.txt in the main folder

// Include of configuration script
#include <config.h>

#include <sys/types.h>
// Include of dependances
#ifdef __WIN32
#define __USE_W32_SOCKETS
#include <windows.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <netdb.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h> // isprint

/* Includes of software */
#include "../common/core.h"
#include "../common/socket.h"
#include "../common/timer.h"
#include "../common/version.h"
#include "../common/malloc.h"
#include "../common/lock.h"
#include "../common/addons.h"
#include "../common/console.h"
#include "../common/utils.h"
#include "../common/md5calc.h"
#include "login.h"
#include "ladmin.h"
#include "lutils.h"

/* add include for DBMS (mysql) */
#ifdef USE_SQL
#ifdef USE_MYSQL
#include <../common/db_mysql.h>
#endif /* USE_MYSQL */
#endif /* USE_SQL */

/*------------------
  global variables
------------------*/
int account_id_count = START_ACCOUNT_NUM;
int login_port;
static char lan_char_ip[16]; // 15 + NULL
static unsigned long int lan_char_ip_addr;
static unsigned char subnet[4];
static unsigned char subnetmask[4];
static unsigned char new_account_flag;

char date_format[32];

static unsigned char save_unknown_packets;

static unsigned char display_parse_login; /* 0: no, 1: yes */
static unsigned char display_parse_fromchar; /* 0: no, 1: yes (without packet 0x2714), 2: all packets */

static unsigned char server_freezeflag[MAX_SERVERS]; /* Char-server anti-freeze system. Counter. 6 ok, 5...0 frozen */
static int anti_freeze_counter;
static int anti_freeze_interval;

static int login_fd = -1; /* init with -1 to avoid problem in do_final function if socket is not already opened */

enum {
	ACO_DENY_ALLOW = 0,
	ACO_ALLOW_DENY,
	ACO_MUTUAL_FAILTURE,
	ACO_STRSIZE = 32 // max normal value: 255.255.255.255/255.255.255.255 + NULL, so 15 + 1 + 15 + 1 = 32
};

static unsigned char access_order; // enum: ACO_DENY_ALLOW, ACO_ALLOW_DENY, ACO_MUTUAL_FAILTURE
static int access_allownum;
static int access_denynum;
static char *access_allow;
static char *access_deny;

static int start_limited_time; /* Starting additional sec from now for the limited time at creation of accounts (-1: unlimited time, 0 or more: additional sec from now) */
static unsigned char check_ip_flag; /* It's to check IP of a player between login-server and char-server (part of anti-hacking system) */
static unsigned char check_authfifo_login2; // It's to check the login2 value (part of anti-hacking system) - related to the client's versions higher than 18.

struct login_session_data {
	int md5keylen;
	char md5key[20];
};

struct auth_dat *auth_dat = NULL;
static short auth_fifo_pos = 0;
int auth_num = 0, auth_max = 0;

static unsigned char level_new_account = 0; // GM level of a new account

/* Logs options */
static unsigned char log_request_connection; // Enable/disable logs of 'Request for connection' message (packet 0x64/0x1dd/0x27c)
static unsigned char log_request_version; // Enable/disable logs of 'Request of the server version' (athena version) message (packet 0x7530 with 'normal' connection)
static unsigned char log_request_freya_version; // Enable/disable logs of 'Request of the server version' (freya version) message (packet 0x7535 with 'normal' connection)
static unsigned char log_request_uptime; // Enable/disable logs of 'Request of the server uptime' message (packet 0x7533 with 'normal' connection)

/* sstatus files name */
static char temp_char_buffer[1024]; /* temporary buffer of type char (see php_addslashes) */
static char sstatus_txt_filename[512];
static char sstatus_html_filename[512];
static char sstatus_php_filename[512];
static int sstatus_refresh_html = 20; /* refresh time (in sec) of the html file in the explorer */
static unsigned char sstatus_txt_enable;
static unsigned char sstatus_html_enable;
static unsigned char sstatus_php_enable;

/* define the number of times that some players must authentify them before to save account file.
   it's just about normal authentification. If an account is created or modified, save is immediatly done.
   An authentification just change last connected IP and date. It already save in log file.
   set minimum auth change before save: */
#define AUTH_BEFORE_SAVE_FILE 10
/* set divider of auth_num to found number of change before save */
#define AUTH_SAVE_FILE_DIVIDER 50
#ifdef TXT_ONLY
static int auth_before_save_file = 0; /* Counter. First save when 1st char-server do connection. */
#endif /* TXT_ONLY */

/* all variables about char-servers connection security */
static char *access_char_allow = NULL;
static int access_char_allownum = 0;

/* all variables about admin */
static unsigned char admin_state; /* authorize or not connection in admin mode */
static char admin_pass[25];
static int access_ladmin_allownum = 0;
static char *access_ladmin_allow = NULL;

/* all variables about GM */
static char gm_pass[64];
static unsigned char level_new_gm;
static unsigned char min_level_to_connect; /* minimum level of player/GM (0: player, 1-99: gm) to connect on the server */

static int client_version_to_connect = 0; /* Client version needed to connect: 0: any client, otherwise client version */

static unsigned char dynamic_pass_failure_ban;
static int dynamic_pass_failure_ban_time;
static int dynamic_pass_failure_ban_how_many;
static int dynamic_pass_failure_ban_how_long;
static unsigned char dynamic_pass_failure_save_in_account;
static int pass_failure_list_max; /* list of invalid password */
static struct pass_failure_list{
	unsigned int ip;
	time_t ban_time; /* # of seconds 1/1/1970 (timestamp): ban hour of the invalid check */
} *pass_failure_list;
static int ban_list_max; /* list of actual ban */
static struct ban_list{
	unsigned int ip;
	time_t ban_time; /* # of seconds 1/1/1970 (timestamp): ban hour of the invalid check */
} *ban_list;

static unsigned char strict_account_name_compare;

static unsigned char console;
static char console_pass[512]; /* password to enable console */

/* online players variables */
static struct online_db{
	int account_id;
	char server;
} *online_db = NULL;
static unsigned short online_num = 0, online_max = 0;


/* *** TXT *** CONFIGURATION */
#ifdef TXT_ONLY
static char account_filename[512];
static char GM_account_filename[512];
static unsigned char save_GM_level_with_accounts;
#endif /* TXT_ONLY */

/* *** SQL *** CONFIGURATION */
#ifdef USE_SQL
#ifdef USE_MYSQL
static unsigned char optimize_table = 0;
#endif /* USE_MYSQL */
#endif /* USE_SQL */

/*---------------------------------------------------------------
  Test of the IP mask
  (ip: IP to be tested, str: mask x.x.x.x/# or x.x.x.x/y.y.y.y)
---------------------------------------------------------------*/
static int check_ipmask(unsigned int ip, const char *str) { // not inline, called too often
	int i = 0;
	unsigned int mask = 0, m, ip2;
	unsigned short a0, a1, a2, a3;
	unsigned char *p = (unsigned char *)&ip2, *p2 = (unsigned char *)&mask;

	if (sscanf(str, "%hu.%hu.%hu.%hu/%n", &a0, &a1, &a2, &a3, &i) != 4 || i == 0 ||
	    a0 > 255 || a1 > 255 || a2 > 255 || a3 > 255)
		return 0;
	p[0] = a0; p[1] = a1; p[2] = a2; p[3] = a3;

	if (sscanf(str + i, "%hu.%hu.%hu.%hu", &a0, &a1, &a2, &a3) == 4 &&
	    a0 <= 255 && a1 <= 255 && a2 <= 255 && a3 <= 255) {
		p2[0] = a0; p2[1] = a1; p2[2] = a2; p2[3] = a3;
		mask = ntohl(mask);
	} else if (sscanf(str + i, "%u", &m) == 1 && m <= 32) { // m unsigned, not need to check m >= 0
		for(i = 0; i < m; i++)
			mask = (mask >> 1) | 0x80000000;
	} else {
		printf("check_ipmask: invalid mask [%s].\n", str);
		return 0;
	}

/*	printf("Tested IP: %08x, network: %08x, network mask: %08x\n", (unsigned int)ntohl(ip), (unsigned int)ntohl(ip2), mask); */
	return ((ntohl(ip) & mask) == (ntohl(ip2) & mask));
}

/*----------------------
  Access control by IP
----------------------*/
static int check_ip(unsigned int ip) { // not inline, called too often
	int i;
	unsigned char *p = (unsigned char *)&ip;
	char buf[17]; // 255.255.255.255. + NULL (16 + NULL)
	char * access_ip;
	enum { ACF_DEF, ACF_ALLOW, ACF_DENY } flag = ACF_DEF;

	if (access_allownum == 0 && access_denynum == 0)
		return 1; /* When there is no restriction, all IP are authorized. */

/*	+   012.345.: front match form, or
	    all: all IP are matched, or
	    012.345.678.901/24: network form (mask with # of bits), or
	    012.345.678.901/255.255.255.0: network form (mask with ip mask)
	+   Note about the DNS resolution (like www.ne.jp, etc.):
	    There is no guarantee to have an answer.
	    If we have an answer, there is no guarantee to have a 100% correct value.
	    And, the waiting time (to check) can be long (over 1 minute to a timeout). That can block the software.
	    So, DNS notation isn't authorized for ip checking.*/
	sprintf(buf, "%d.%d.%d.%d.", p[0], p[1], p[2], p[3]);

	for(i = 0; i < access_allownum; i++) {
		access_ip = access_allow + i * ACO_STRSIZE;
		if (strncmp(access_ip, buf, strlen(access_ip)) == 0 || check_ipmask(ip, access_ip)) {
			if (access_order == ACO_ALLOW_DENY)
				return 1; /* With 'allow, deny' (deny if not allow), allow has priority */
			flag = ACF_ALLOW;
			break;
		}
	}

	for(i = 0; i < access_denynum; i++) {
		access_ip = access_deny + i * ACO_STRSIZE;
		if (memcmp(access_ip, buf, strlen(access_ip)) == 0 || check_ipmask(ip, access_ip)) {
			/*flag = ACF_DENY; not necessary to define flag */
			return 0; /* At this point, if it's 'deny', we refuse connection. */
		}
	}

	return (flag == ACF_ALLOW || access_order == ACO_DENY_ALLOW) ? 1 : 0;
	/* With 'mutual-failture', only 'allow' and non 'deny' IP are authorized.
	     A non 'allow' (even non 'deny') IP is not authorized. It's like: if allowed and not denied, it's authorized.
	     So, it's disapproval if you have no description at the time of 'mutual-failture'.
	   With 'deny,allow' (allow if not deny), because here it's not deny, we authorize.*/
}

/*---------------------------------
  Access control by IP for ladmin
---------------------------------*/
static inline int check_ladminip(unsigned int ip) {
	int i;
	unsigned char *p = (unsigned char *)&ip;
	char buf[17];
	char * access_ip;

	if (access_ladmin_allownum == 0)
		return 1; /* When there is no restriction, all IP are authorized. */

/*	+   012.345.: front match form, or
	    all: all IP are matched, or
	    012.345.678.901/24: network form (mask with # of bits), or
	    012.345.678.901/255.255.255.0: network form (mask with ip mask)
	+   Note about the DNS resolution (like www.ne.jp, etc.):
	    There is no guarantee to have an answer.
	    If we have an answer, there is no guarantee to have a 100% correct value.
	    And, the waiting time (to check) can be long (over 1 minute to a timeout). That can block the software.
	    So, DNS notation isn't authorized for ip checking.*/
	sprintf(buf, "%d.%d.%d.%d.", p[0], p[1], p[2], p[3]);

	for(i = 0; i < access_ladmin_allownum; i++) {
		access_ip = access_ladmin_allow + (i * ACO_STRSIZE);
		if (strncmp(access_ip, buf, strlen(access_ip)) == 0 || check_ipmask(ip, access_ip))
			return 1;
	}

	return 0;
}

/*---------------------------------
  Access control by IP for char-servers
---------------------------------*/
static inline int check_charip(unsigned int ip) {
	int i;
	unsigned char *p = (unsigned char *)&ip;
	char buf[17];
	char * access_ip;

	if (access_char_allownum == 0)
		return 1; /* When there is no restriction, all IP are authorized. */

/*	+   012.345.: front match form, or
	    all: all IP are matched, or
	    012.345.678.901/24: network form (mask with # of bits), or
	    012.345.678.901/255.255.255.0: network form (mask with ip mask)
	+   Note about the DNS resolution (like www.ne.jp, etc.):
	    There is no guarantee to have an answer.
	    If we have an answer, there is no guarantee to have a 100% correct value.
	    And, the waiting time (to check) can be long (over 1 minute to a timeout). That can block the software.
	    So, DNS notation isn't authorized for ip checking.*/
	sprintf(buf, "%d.%d.%d.%d.", p[0], p[1], p[2], p[3]);

	for(i = 0; i < access_char_allownum; i++) {
		access_ip = access_char_allow + (i * ACO_STRSIZE);
		if (strncmp(access_ip, buf, strlen(access_ip)) == 0 || check_ipmask(ip, access_ip))
			return 1;
	}

	return 0;
}

/*---------------------------------------------
  Test to know if an IP come from LAN or WAN.
---------------------------------------------*/
static int lan_ip_check(unsigned char *p) { // not inline, called too often
	int i;
	int lancheck = 1;

	for(i = 0; i < 4; i++) {
		if ((subnet[i] & subnetmask[i]) != (p[i] & subnetmask[i])) {
			lancheck = 0;
			break;
		}
	}
/*	printf("LAN test (result): %s source" CL_RESET ".\n", (lancheck) ? CL_CYAN "LAN" : CL_GREEN "WAN"); */

	return lancheck;
}

/*------------------------------------
  Add a password failure in the list
------------------------------------*/
static void add_pass_failure_ban(unsigned int ip, int account) { // not inline, called too often
	int i, counter, add_done;
	time_t actual_time;
	unsigned char *p = (unsigned char *)&ip;
	char tmpstr[256];
	int tmpstr_len;

	/* set the actual */
	actual_time = time(NULL) - dynamic_pass_failure_ban_time; /* speed up */

	counter = 0;
	add_done = 0;
	for (i = 0; i < pass_failure_list_max; i++)
		if (pass_failure_list[i].ip) {
			/* if time is over */
			if (pass_failure_list[i].ban_time < actual_time) {
				if (add_done == 0) {
					/* add the new value */
					pass_failure_list[i].ip = ip;
					pass_failure_list[i].ban_time = time(NULL);
					add_done = 1;
					counter++;
				} else {
					/* reset the value */
					pass_failure_list[i].ip = 0;
					pass_failure_list[i].ban_time = 0;
				}
			/* if it's same ip */
			} else if (pass_failure_list[i].ip == ip)
				counter++;
		} else if (add_done == 0) {
			/* add the new value */
			pass_failure_list[i].ip = ip;
			pass_failure_list[i].ban_time = time(NULL);
			add_done = 1;
			counter++;
		}

	/* if not added */
	if (add_done == 0) {
		pass_failure_list_max += 256;
		REALLOC(pass_failure_list, struct pass_failure_list, pass_failure_list_max);
		memset(pass_failure_list + (pass_failure_list_max - 256), 0, sizeof(struct pass_failure_list) * 256);
		/* add the new value */
		pass_failure_list[pass_failure_list_max - 256].ip = ip;
		pass_failure_list[pass_failure_list_max - 256].ban_time = time(NULL);
		counter++;
	}

	/* if ban counter is arrived, we add the ip is the ban list */
	if (counter == dynamic_pass_failure_ban_how_many) {
		write_log("Too many invalid passwords: IP banned (ip: %d.%d.%d.%d)" RETCODE, p[0], p[1], p[2], p[3]);
		actual_time = time(NULL); /* speed up */
		add_done = 0;
		for (i = 0; i < ban_list_max; i++)
			if (ban_list[i].ip) {
				/* if time is over */
				if (ban_list[i].ban_time < actual_time) {
					if (add_done == 0) {
						/* add the new value */
						ban_list[i].ip = ip;
						ban_list[i].ban_time = time(NULL) + dynamic_pass_failure_ban_how_long;
						add_done = 1;
					} else {
						/* reset the value */
						ban_list[i].ip = 0;
						ban_list[i].ban_time = 0;
					}
				}
			} else if (add_done == 0) {
				/* add the new value */
				ban_list[i].ip = ip;
				ban_list[i].ban_time = time(NULL) + dynamic_pass_failure_ban_how_long;
				add_done = 1;
			}
		/* if not added */
		if (add_done == 0) {
			ban_list_max += 256;
			REALLOC(ban_list, struct ban_list, ban_list_max);
			memset(ban_list + (ban_list_max - 256), 0, sizeof(struct ban_list) * 256);
			/* add the next value */
			ban_list[ban_list_max - 256].ip = ip;
			ban_list[ban_list_max - 256].ban_time = time(NULL) + dynamic_pass_failure_ban_how_long;
		}
		/* add comment in memo field */
		if (dynamic_pass_failure_save_in_account) {
			memset(tmpstr, 0, sizeof(tmpstr));
			tmpstr_len = strftime(tmpstr, 20, date_format, localtime(&actual_time)); // 19 + NULL
			sprintf(tmpstr + tmpstr_len, ": %d invalid passwords: IP (%d.%d.%d.%d) banned for %d seconds.", dynamic_pass_failure_ban_how_many, p[0], p[1], p[2], p[3], dynamic_pass_failure_ban_how_long);
			add_text_in_memo(account, tmpstr);
			save_account(account, 1);
		}
	}

	return;
}

/*----------------------------------------------
  Check ban ip from invalid repeated passwords
-----------------------------------------------*/
static inline time_t check_banlist(unsigned int ip) {
	int i;
	time_t actual_time;

	/* set the actual */
	actual_time = time(NULL); /* speed up */

	for (i = 0; i < ban_list_max; i++)
		if (ban_list[i].ip) {
			/* if time is over */
			if (ban_list[i].ban_time < actual_time) {
				/* reset the value */
				ban_list[i].ip = 0;
				ban_list[i].ban_time = 0;
			/* if it's same ip */
			} else if (ban_list[i].ip == ip && dynamic_pass_failure_ban)
				return ban_list[i].ban_time; /* banned */
		}

	return 0;
}

/*----------------------------------------------
  Check if a account is already online
-----------------------------------------------*/
unsigned char check_online_player(const int account_id) {
	int i;

	for (i = 0; i < online_num; i++)
		if (online_db[i].account_id == account_id)
			return 1;

	return 0;
}

/*---------------------------------
  Packet send to all char-servers
---------------------------------*/
void charif_sendall(unsigned int len) {
	int i, fd;

	for(i = 0; i < MAX_SERVERS; i++) { // max number of char-servers (and account_id values: 0 to max-1)
		if ((fd = server_fd[i]) >= 0) {
			SENDPACKET(fd, len);
		}
	}

	return;
}

/*---------------------------------------------------------------------
  Packet send to all char-servers, except one (wos: without our self)
---------------------------------------------------------------------*/
static void charif_sendallwos(int sfd, unsigned int len) { // not inline, called too often
	int i, fd;

	for(i = 0; i < MAX_SERVERS; i++) { // max number of char-servers (and account_id values: 0 to max-1)
		if ((fd = server_fd[i]) >= 0 && fd != sfd) {
			SENDPACKET(fd, len);
		}
	}

	return;
}

/*--------------------------------------
  Send a GM account to all char-server
--------------------------------------*/
void send_GM_account(const int account_id, const unsigned char level) {
	WPACKETW(0) = 0x2733;
	WPACKETL(2) = account_id;
	WPACKETB(6) = level;
	charif_sendall(7);

	return;
}

/*----------------------------------------
  Check/authentification of a connection
----------------------------------------*/
static int mmo_auth(struct mmo_account* account, int fd) { // not inline, called too often
	int i;
	struct timeval tv;
	time_t now;
	char tmpstr[64];
	int len, newaccount = 0;
	struct login_session_data *ld;
	char ip[16];
	unsigned char *sin_addr = (unsigned char *)&session[fd]->client_addr.sin_addr;
	struct auth_dat new_account;

	sprintf(ip, "%d.%d.%d.%d", sin_addr[0], sin_addr[1], sin_addr[2], sin_addr[3]);

	len = strlen(account->userid) - 2;
	/* Account creation with _M/_F */
	if (account->passwdenc == 0 && len >= 4 && len <= (24 - 2) && account->userid[len] == '_' &&
	    (account->userid[len + 1] == 'F' || account->userid[len + 1] == 'M') && new_account_flag != 0 &&
	    account_id_count <= END_ACCOUNT_NUM && strlen(account->passwd) >= 4 && strlen(account->passwd) <= 24) {
		newaccount = 1;
		account->userid[len] = '\0';
	}

	/* account search */
	i = search_account_index(account->userid);

	/* check version of client */
	if (client_version_to_connect != 0 &&
	    account->version != client_version_to_connect && (i == -1 || auth_dat[i].sex != 2)) {
		write_log("Attempt of connection of an unauthorized client version (client version: %d, authorized client version: %d, ip: %s)" RETCODE,
		          account->version, client_version_to_connect, ip);
		return 5; /* 5 = Your Game's EXE file is not the latest version */
	}

	/* if asked for creation */
	if (newaccount == 1) {
		/* if account exists (strict comparaison) */
		if (i != -1 && strcmp(account->userid, auth_dat[i].userid) == 0) {
			/* set unregisted ID, because name was given with _F/_M */
			write_log("Attempt of creation of an already existant account (account: %s_%c, ip: %s)" RETCODE,
			          account->userid, account->userid[len + 1], ip);
			return 0; /* 0 = Unregistered ID */
		}
		/* if we refuse creation of account with different case - don't check if we authorize, because strict comparaison was done before */
		if (unique_case_account_name_creation) {
			if (i != -1 || check_existing_account_name_for_creation(account->userid)) {
				/* set unregisted ID, because name was given with _F/_M */
				write_log("Attempt of creation of an account with different case (account: %s_%c, ip: %s)" RETCODE,
				          account->userid, account->userid[len + 1], ip);
				return 0; /* 0 = Unregistered ID */
			}
		}
		/* do some checks */
		if (account_id_count > END_ACCOUNT_NUM) {
			write_log("Request for an account creation: no more available id number (account: %s, sex: %c, ip: %s)" RETCODE,
			          account->userid, account->userid[len + 1], ip);
			return 0; /* 0 = Unregistered ID */
		/* create the account */
		} else {
			/* init account structure */
			init_new_account(&new_account);
			/* copy values */
			strncpy(new_account.userid, account->userid, 24);
			if (use_md5_passwds)
				MD5_String(account->passwd, new_account.pass);
			else
				strncpy(new_account.pass, account->passwd, 32);
			new_account.sex = ((account->userid[len + 1] == 'S' || account->userid[len + 1] == 's') ? 2 : (account->userid[len + 1] == 'M' || account->userid[len + 1] == 'm'));
			/* Added new account */
			i = add_account(&new_account, NULL);
			write_log("Account creation and authentification accepted (account %s (id: %d), sex: %c, connection with _F/_M, ip: %s)" RETCODE,
			          account->userid, account->account_id, account->userid[len + 1], ip);
/*			#ifdef TXT_ONLY
				Note: save is done at end of the function.
				save_account(i, 1); // in SQL, account is already save when we call add_account()
			#endif // TXT_ONLY */
		}
	/* no request for creation - account exists*/
	} else if (i != -1) {
		/* for the possible tests/checks afterwards (copy correct sensitive case) */
		memset(account->userid, 0, sizeof(account->userid));
		strncpy(account->userid, auth_dat[i].userid, 24);
		/* check password */
		if (account->passwdenc > 0) {
			ld = session[fd]->session_data;
			if (!ld) {
				write_log("Md5 key not created (account: %s, ip: %s)" RETCODE, account->userid, ip);
				return 1; /* 1 = Incorrect Password */
			}
			if (password_check(auth_dat[i].pass, account->passwd, ld->md5key) == 0) {
				write_log("Invalid password (account: %s, encrypted password, ip: %s)" RETCODE, account->userid, ip);
				add_pass_failure_ban(session[fd]->client_addr.sin_addr.s_addr, i);
				return 1; /* 1 = Incorrect Password */
			}
		} else {
			if (password_check(auth_dat[i].pass, account->passwd, "") == 0) {
				write_log("Invalid password (account: %s, non encrypted password, ip: %s)" RETCODE, account->userid, ip);
				add_pass_failure_ban(session[fd]->client_addr.sin_addr.s_addr, i);
				return 1; /* 1 = Incorrect Password */
			}
		}
		/* check state */
		if (auth_dat[i].state) {
			write_log("Connection refused (account: %s, state: %d, ip: %s)" RETCODE,
			          account->userid, auth_dat[i].state, ip);
			switch(auth_dat[i].state) { /* packet 0x006a value + 1 */
			case 1:   /* 0 = Unregistered ID */
			case 2:   /* 1 = Incorrect Password */
			case 3:   /* 2 = This ID is expired */
			case 4:   /* 3 = Rejected from Server */
			case 5:   /* 4 = You have been blocked by the GM Team */
			case 6:   /* 5 = Your Game's EXE file is not the latest version */
			case 7:   /* 6 = Your are Prohibited to log in until %s */
			case 8:   /* 7 = Server is jammed due to over populated */
			case 9:   /* 8 = No MSG (actually, all states after 9 except 99 are No MSG, use only this) */
			case 100: /* 99 = This ID has been totally erased */
				return auth_dat[i].state - 1;
			default:
				return 99; /* 99 = ID has been totally erased */
			}
		}
		/* check ban time */
		if (auth_dat[i].ban_until_time != 0) { /* if account is banned */
			memset(tmpstr, 0, sizeof(tmpstr));
			strftime(tmpstr, 20, date_format, localtime(&auth_dat[i].ban_until_time)); // 19 + NULL
			if (auth_dat[i].ban_until_time > time(NULL)) { /* always banned */
				write_log("Connection refused (account: %s, banned until %s, ip: %s)" RETCODE,
				          account->userid, tmpstr, ip);
				return 6; /* 6 = Your are Prohibited to log in until %s */
			} else { /* ban is finished */
				write_log("End of ban (account: %s, previously banned until %s -> not more banned, ip: %s)" RETCODE,
				          account->userid, tmpstr, ip);
				auth_dat[i].ban_until_time = 0; /* reset the ban time */
			}
		}
		/* check limited time */
		if (auth_dat[i].connect_until_time != 0 && auth_dat[i].connect_until_time < time(NULL)) {
			write_log("Connection refused (account: %s, expired ID, ip: %s)" RETCODE, account->userid, ip);
			return 2; /* 2 = This ID is expired */
		}
		/* connection is accepted */
		write_log("Authentification accepted (account: %s (id: %d), ip: %s)" RETCODE, account->userid, auth_dat[i].account_id, ip);
	/* account doesn't exist */
	} else {
		if (account->passwdenc > 0) {
			ld = session[fd]->session_data;
			if (!ld)
				write_log("Unknown account (account: %s, Md5 key not created for encrypted password, ip: %s)" RETCODE, account->userid, ip);
			else
				write_log("Unknown account (account: %s, encrypted password, ip: %s)" RETCODE, account->userid, ip);
		} else
			write_log("Unknown account (account: %s, non encrypted password, ip: %s)" RETCODE, account->userid, ip);
		return 0; /* 0 = Unregistered ID */
	}

	/* update value of authorized account */
	gettimeofday(&tv, NULL);
	now = time(NULL);
	memset(tmpstr, 0, sizeof(tmpstr));
	strftime(tmpstr, 20, date_format, localtime(&now)); // 19 + NULL
	sprintf(tmpstr + strlen(tmpstr), ".%03d", (int)tv.tv_usec / 1000);

	account->account_id = auth_dat[i].account_id;
	account->login_id1 = rand();
	account->login_id2 = rand();
	memset(account->lastlogin, 0, 24);
	strncpy(account->lastlogin, auth_dat[i].lastlogin, 23);
	memset(auth_dat[i].lastlogin, 0, 24);
	strncpy(auth_dat[i].lastlogin, tmpstr, 23);
	account->sex = auth_dat[i].sex;
	strncpy(auth_dat[i].last_ip, ip, 16);
	auth_dat[i].logincount++;

	/* save update */
	save_account(i, newaccount); /* if new account, force save. Otherwise, doesn't force save */

	/* send GM level to char-server and map server */
	send_GM_account(auth_dat[i].account_id, auth_dat[i].level);

	return -1; /* account OK */
}

/*--------------------
  Added text in memo
--------------------*/
void add_text_in_memo(int account, const char* text) {
	int len;
	unsigned int size_of_memo;

	len = strlen(text);
	if (len < 1)
		return;

	if (auth_dat[account].memo == NULL) {
		/* limit size of memo to 60000 */
		if (len > 60000)
			len = 60000;
		CALLOC(auth_dat[account].memo, char, len + 1);
		strncpy(auth_dat[account].memo, text, len);
	} else {
		size_of_memo = strlen(auth_dat[account].memo);
		/* limit size of memo to 60000 */
		if (size_of_memo + 1 + len > 60000) /* with space between the 2 strings */
			len = 60000 - 1 - size_of_memo;
		if (len > 0) {
			REALLOC(auth_dat[account].memo, char, size_of_memo + 1 + len + 1); /* add space and NULL */
			auth_dat[account].memo[size_of_memo] = ' ';
			strncpy(auth_dat[account].memo + (size_of_memo + 1), text, len);
			auth_dat[account].memo[size_of_memo + 1 + len] = 0;
		}
	}

	return;
}


static inline char *php_addslashes(const char *original_string) {
	const char *pointer1;
	char *pointer2;

	pointer1 = original_string;
	pointer2 = temp_char_buffer;
	while(*pointer1) {
		if ((*pointer1 == 92) || (*pointer1 == 39) || (*pointer1 == 34))
			*(pointer2++) = 92;
		*(pointer2++) = *(pointer1++);
		if ((void *)(&temp_char_buffer + 1022) <= (void *)pointer2) // overflow?
			break;
	}
	*pointer2 = 0;

	return temp_char_buffer;
}

/*------------------------
  Sstatus files creation
------------------------*/
static void create_sstatus_files() { // not inline, called too often
	int i, j;             /* for loops */
	FILE *fp;             /* for the file */
	time_t time_server;   /* for number of seconds */
	char temp_time[20];  /* to prepare how time must be displayed */
	int servers;          /* count the number of online servers */
	int players;          /* count the number of players */

	/* Get number of online servers */
	servers = 0;
	players = 0;
	for(i = 0; i < MAX_SERVERS; i++) // max number of char-servers (and account_id values: 0 to max-1)
		if (server_fd[i] > 0) {
			servers++;
			players += server[i].users;
		}

	/* get time */
	time(&time_server); /* get time in seconds since 1/1/1970 */
	memset(temp_time, 0, sizeof(temp_time));
	strftime(temp_time, 20, date_format, localtime(&time_server)); /* like sprintf, but only for date/time (05 dec 2003 15:12:52) */

	/* write TXT file */
	if (sstatus_txt_enable != 0) { /* 0: no, 1 yes, 2: yes with ID of servers */
		if ((fp = fopen(sstatus_txt_filename, "w")) != NULL) {
			/* If we display at least 1 server */
			if (servers > 0) {
				/* write heading */
				if (servers < 2)
					fprintf(fp, "Online Server (%s):" RETCODE, temp_time);
				else
					fprintf(fp, "Online Servers (%s):" RETCODE, temp_time);
				fprintf(fp, RETCODE);

				if (sstatus_txt_enable == 1) { /* 0: no, 1 yes, 2: yes with ID of servers */
					fprintf(fp, "Name                 Counter" RETCODE /* 20 + 1 + 7 */
					            "----------------------------" RETCODE);
				} else {
					fprintf(fp, "  ID Name                 Counter" RETCODE /* 4 + 1 + 20 + 1 + 7 */
					            "---------------------------------" RETCODE);
				}
				/* display each server. */
				for(i = 0; i < MAX_SERVERS; i++) // max number of char-servers (and account_id values: 0 to max-1)
					if (server_fd[i] > 0) {
						if (sstatus_txt_enable == 1) { /* 0: no, 1 yes, 2: yes with ID of servers */
							fprintf(fp, "%-20s %7d" RETCODE, server[i].name, server[i].users);
						} else {
							fprintf(fp, "%4d %-20s %7d" RETCODE, i, server[i].name, server[i].users);
						}
					}
				/* no display if only 1 server */
				if (servers > 1)
					fprintf(fp, "%d servers are online." RETCODE, servers);
				/* display number of online players */
				if (players == 0)
					fprintf(fp, "No player is online." RETCODE);
				else if (players == 1)
					fprintf(fp, "1 player is online." RETCODE);
				else
					fprintf(fp, "%d players are online." RETCODE, players);
			} else {
				fprintf(fp, "No server is online (%s)." RETCODE, temp_time);
			}
			fclose(fp);
		}
	}

	/* write HTML file */
	if (sstatus_html_enable != 0) { /* 0: no, 1 yes, 2: yes with ID of servers */
		if ((fp = fopen(sstatus_html_filename, "w")) != NULL) {
			/* write heading */
			fprintf(fp, "<HTML>" RETCODE
			            "  <HEAD>" RETCODE
			            "    <META http-equiv=\"Refresh\" content=\"%d\">" RETCODE, sstatus_refresh_html); /* update on client explorer every x seconds */
			if (servers < 2)
				fprintf(fp, "    <TITLE>Online Server</TITLE>" RETCODE);
			else
				fprintf(fp, "    <TITLE>Online Servers</TITLE>" RETCODE);
			fprintf(fp, "  </HEAD>" RETCODE
			            "  <BODY>" RETCODE);

			/* If we display at least 1 server */
			if (servers > 0) {
				if (servers < 2)
					fprintf(fp, "    <H3>Online Server (%s):</H3>" RETCODE, temp_time);
				else
					fprintf(fp, "    <H3>Online Servers (%s):</H3>" RETCODE, temp_time);

				fprintf(fp, "    <table border=\"1\" cellspacing=\"1\">" RETCODE
				            "      <tr>" RETCODE);
				if (sstatus_html_enable != 1) /* 0: no, 1 yes, 2: yes with ID of servers */
					fprintf(fp, "        <td><b>Id</b></td>" RETCODE);
				fprintf(fp, "        <td><b>Name</b></td>" RETCODE
				            "        <td><b>Counter</b></td>" RETCODE
				            "      </tr>" RETCODE);
				/* display each server. */
				for(i = 0; i < MAX_SERVERS; i++) { // max number of char-servers (and account_id values: 0 to max-1)
					if (server_fd[i] > 0) {
						fprintf(fp, "      <tr>" RETCODE);
						if (sstatus_html_enable != 1) /* 0: no, 1 yes, 2: yes with ID of servers */
							fprintf(fp, "        <td>%d</td>" RETCODE, i);
						/* name of the server in the html (no < >, because that create problem in html code) */
						fprintf(fp, "        <td>");
						for (j = 0; server[i].name[j]; j++) {
							switch(server[i].name[j]) {
							case '<':
								fprintf(fp, "&lt;");
								break;
							case '>':
								fprintf(fp, "&gt;");
								break;
							default:
								fprintf(fp, "%c", server[i].name[j]);
								break;
							};
						}
						fprintf(fp, "</td>" RETCODE
						            "        <td>%d</td>" RETCODE, server[i].users);
						fprintf(fp, "      </tr>" RETCODE);
					}
				}
				fprintf(fp, "    </table>" RETCODE);
				/* no display if only 1 server */
				if (servers > 1)
					fprintf(fp, "    <p>%d servers are online.</p>" RETCODE, servers);
				/* display number of online players */
				if (players == 0)
					fprintf(fp, "    <p>No player is online.</p>" RETCODE);
				else if (players == 1)
					fprintf(fp, "    <p>1 player is online.</p>" RETCODE);
				else
					fprintf(fp, "    <p>%d players are online.</p>" RETCODE, players);
			} else {
				fprintf(fp, "    <p>No server is online (%s).</p>" RETCODE, temp_time);
			}
			fprintf(fp, "  </BODY>" RETCODE);
			fprintf(fp, "</HTML>" RETCODE);
			fclose(fp);
		}
	}

	/* write PHP file */
	if (sstatus_php_enable != 0) { /* 0: no, 1 yes */
		if ((fp = fopen(sstatus_php_filename, "w")) != NULL) {
			/* write heading */
			fprintf(fp, "<?php" RETCODE
			            "// File generated on %s" RETCODE, temp_time);
			fprintf(fp, "$server = array(" RETCODE
			            "\t'time'=>%d," RETCODE, (int)time_server);

			/* If we display at least 1 server */
			if (servers > 0) {
				fprintf(fp, "\t'servers'=>array(" RETCODE);
				/* display each server. */
				for(i = 0; i < MAX_SERVERS; i++) { // max number of char-servers (and account_id values: 0 to max-1)
					if (server_fd[i] > 0) {
						fprintf(fp, "\t\t%d=>array(" RETCODE, i);
						fprintf(fp, "\t\t\t'name'=>'%s'," RETCODE, php_addslashes(server[i].name));
						fprintf(fp, "\t\t\t'counter'=>%d," RETCODE, server[i].users);
						fprintf(fp, "\t\t)," RETCODE);
					}
				}
				fprintf(fp, "\t)," RETCODE);
			} else {
				fprintf(fp, "\t'servers'=>array()," RETCODE);
			}
			fprintf(fp, "\t'playercount'=>%d," RETCODE, players);
			fprintf(fp, ");" RETCODE);
			fclose(fp);
		}
	}

	return;
}

/*---------------------------------
  Packet parsing for char-servers
---------------------------------*/
int parse_fromchar(int fd) {
	int i, j, id;
	unsigned char *p = (unsigned char *) &session[fd]->client_addr.sin_addr;
	char ip[16];
	int acc;

	sprintf(ip, "%d.%d.%d.%d", p[0], p[1], p[2], p[3]);

	for(id = 0; id < MAX_SERVERS; id++) // max number of char-servers (and account_id values: 0 to max-1)
		if (server_fd[id] == fd)
			break;
	if (id == MAX_SERVERS || session[fd]->eof) { // max number of char-servers (and account_id values: 0 to max-1)
		if (id < MAX_SERVERS) { // max number of char-servers (and account_id values: 0 to max-1)
			printf("Char-server '" CL_CYAN "%s" CL_RESET "' has disconnected.\n", server[id].name);
			write_log("Char-server '%s' has disconnected (ip: %s)." RETCODE, server[id].name, ip);
			server_fd[id] = -1;
			memset(&server[id], 0, sizeof(struct mmo_char_server));
			/* remove online players from db */
			for (i = 0; i < online_num; i++)
				if (online_db[i].server == id) {
					if (i < online_num - 1)
						memcpy(&online_db[i], &online_db[online_num - 1], sizeof(struct online_db));
					online_db[online_num - 1].server = -1;
					online_db[online_num - 1].account_id = 0;
					online_num--;
					i--; /* to check new online[i] */
				}
			/* update sstatus */
			create_sstatus_files();
#ifdef USE_SQL
			/* server delete status */
			sql_request("DELETE FROM `sstatus` WHERE `index` = '%d'", id);
#endif /* USE_SQL */
		}
#ifdef __WIN32
		shutdown(fd, SD_BOTH);
		closesocket(fd);
#else
		close(fd);
#endif
		delete_session(fd);
		return 0;
	}

	while (RFIFOREST(fd) >= 2 && !session[fd]->eof) {

		if (display_parse_fromchar == 2 || (display_parse_fromchar == 1 && RFIFOW(fd,0) != 0x2714)) /* 0x2714 is sended very often (number of players) */
			printf("parse_fromchar: connection #%d, packet: 0x%x (with being read: %d bytes).\n", fd, RFIFOW(fd,0), RFIFOREST(fd));

		switch (RFIFOW(fd,0)) {

		/* request from map-server to reload GM accounts. Transmission to login-server via char-server */
		case 0x2709: // 0x2af7 (map->char) -> 0x2709 (char->login)
			if (RFIFOREST(fd) < 6 || RFIFOREST(fd) < RFIFOW(fd,2))
				return 0;
			server_freezeflag[id] = anti_freeze_counter; // Char anti-freeze system. Counter. 6 ok, 5...0 frozen - only on valid packet
			// check for all GM
			for (j = 0; j < RFIFOW(fd,4); j++) {
				acc = RFIFOL(fd,6 + (j * 4));
				i = search_account_index2(acc);
				// if account found
				if (i != -1)
					if (auth_dat[i].level != 0) {
						// send new level to all char-servers
						send_GM_account(acc, auth_dat[i].level);
//						printf("Account: %d, gm level: %d\n", acc, auth_dat[i].level);
					}
			}
			RFIFOSKIP(fd,RFIFOW(fd,2));
			break;

		/* request from char-server to authentify an account */
		case 0x2712:
			if (RFIFOREST(fd) < 19)
				return 0;
			server_freezeflag[id] = anti_freeze_counter; // Char anti-freeze system. Counter. 6 ok, 5...0 frozen - only on valid packet
		  {
			unsigned int acc_ip; /* speed up */
			acc = RFIFOL(fd,2); /* speed up */
			acc_ip = RFIFOL(fd,15);
			for(i = 0; i < AUTH_FIFO_SIZE; i++) {
				if (auth_fifo[i].account_id == acc &&
				    auth_fifo[i].login_id1 == (int)RFIFOL(fd,6) &&
				    (!check_authfifo_login2 || auth_fifo[i].login_id2 == (int)RFIFOL(fd,10)) && // related to the versions higher than 18
				    auth_fifo[i].sex == RFIFOB(fd,14) &&
				    (!check_ip_flag || auth_fifo[i].ip == acc_ip || lan_ip_check((unsigned char *)&auth_fifo[i].ip) || lan_ip_check((unsigned char *)&acc_ip)) && /* check lan ip to avoid some bad routers */
				    !auth_fifo[i].delflag) { // 0: accepted, 1: not valid/not init
					int k;
					k = search_account_index2(acc);
					/* if account found */
					if (k != -1) {
						int size;
						auth_fifo[i].delflag = 1; // 0: accepted, 1: not valid/not init
						write_log("Char-server '%s': authentification of the account %d accepted (ip: %s)." RETCODE, server[id].name, acc, ip);
						// Sending of the account_reg2 to char-server (Must be sended before ACK)
						size = 8;
#ifdef TXT_ONLY
						for(j = 0; j < auth_dat[k].account_reg2_num; j++) {
							strncpy(WPACKETP(size), auth_dat[k].account_reg2[j].str, 32);
							WPACKETL(size + 32) = auth_dat[k].account_reg2[j].value;
							k += 36;
						}
#else // TXT_ONLY -> USE_SQL
						sql_request("SELECT `str`,`value` FROM `account_reg2_db` WHERE `account_id` = '%d'", acc);
						while (sql_get_row()) {
							strncpy(WPACKETP(size), sql_get_string(0), 32);
							WPACKETL(size + 32) = sql_get_integer(1);
							size += 36;
						}
#endif // USE_SQL
						WPACKETW(0) = 0x2729; // Sending of the account_reg2 to char-server
						WPACKETW(2) = size;
						WPACKETL(4) = acc;
						SENDPACKET(fd, size);
//						printf("parse_fromchar: Sending of account_reg2: login->char (auth fifo)\n");
						WPACKETW( 0) = 0x2713;
						WPACKETL( 2) = acc;
						WPACKETB( 6) = 0;
						strncpy(WPACKETP(7), auth_dat[k].email, 40);
						WPACKETL(47) = (unsigned long)auth_dat[k].connect_until_time;
						SENDPACKET(fd, 51);
						break;
					}
				}
			}
			/* authentification not found */
			if (i == AUTH_FIFO_SIZE) {
				write_log("Char-server '%s': authentification of the account %d REFUSED (ip: %s)." RETCODE, server[id].name, acc, ip);
				memset(WPACKETP(0), 0, 51);
				WPACKETW(0) = 0x2713;
				WPACKETL(2) = acc;
				WPACKETB(6) = 1;
				/* It is unnecessary to send email */
				/* It is unnecessary to send validity date of the account */
				SENDPACKET(fd, 51);
			}
		  }
			RFIFOSKIP(fd,19);
			break;

		/* we receive the number of users (no answer) */
		case 0x2714:
			if (RFIFOREST(fd) < 6 || RFIFOREST(fd) < RFIFOW(fd,2))
				return 0;
			server_freezeflag[id] = anti_freeze_counter; /* Char anti-freeze system. Counter. 6 ok, 5...0 frozen - only on valid packet*/
			//printf("parse_fromchar: Receiving of the users number of the server '%s': %d\n", server[id].name, RFIFOL(fd,4));
			server[id].users = RFIFOW(fd,4); /* it's the number of non-hidden players on all map-servers */
			/* remove previous online players from db */
			for (i = 0; i < online_num; i++)
				if (online_db[i].server == id) {
					if (i < online_num - 1)
						memcpy(&online_db[i], &online_db[online_num - 1], sizeof(struct online_db));
					online_db[online_num - 1].server = -1;
					online_db[online_num - 1].account_id = 0;
					online_num--;
					i--; /* to check new online[i] */
				}
			/* add new online players in db */
			for (i = 6; i < RFIFOW(fd,2); i = i + 4) {
				if (online_num >= online_max) {
					online_max += 256;
					REALLOC(online_db, struct online_db, online_max);
					for(j = online_max - 256; j < online_max; j++) {
						online_db[j].server = -1;
						online_db[j].account_id = 0;
					}
				}
				online_db[online_num].server = id;
				online_db[online_num].account_id = RFIFOL(fd,i);
//				printf("online: %d\n", online_db[online_num].account_id);
				online_num++;
			}
			// send to all OTHER char-servers list of connected accounts (to avoid multiple connections)
			if (server[id].users > 0) {
				WPACKETW(0) = 0x271a;
				WPACKETW(2) = 6 + 4 * server[id].users;
				WPACKETW(4) = server[id].users;
				j = 6;
				for (i = 0; i < online_num; i++)
					if (online_db[i].server == id) {
						WPACKETL(j) = online_db[i].account_id;
						j = j + 4;
					}
				charif_sendallwos(fd, WPACKETW(2));
			}
			RFIFOSKIP(fd,RFIFOW(fd,2));
			/* update sstatus */
			create_sstatus_files();
#ifdef USE_SQL
			/* update db sstatus */
			sql_request("UPDATE `sstatus` SET `user` = '%d' WHERE `index` = '%d'", server[id].users, id);
#endif /* USE_SQL */
			break;

		/* we receive an e-mail creation of an account with a default e-mail (no answer) */
		case 0x2715:
			if (RFIFOREST(fd) < 46)
				return 0;
			server_freezeflag[id] = anti_freeze_counter; /* Char anti-freeze system. Counter. 6 ok, 5...0 frozen - only on valid packet*/
		  {
			char email[41]; // 40 + NULL
			acc = RFIFOL(fd,2); /* speed up */
			strncpy(email, RFIFOP(fd,6), 40);
			email[40] = '\0';
			remove_control_chars(email);
			//printf("parse_fromchar: an e-mail creation of an account with a default e-mail: server '%s', account: %d, e-mail: '%s'.\n", server[id].name, acc, RFIFOP(fd,6));
			if (e_mail_check(email) == 0)
				write_log("Char-server '%s': Creation of an e-mail on an account with a default e-mail: REFUSED - invalid e-mail (account: %d, ip: %s)" RETCODE,
				          server[id].name, acc, ip);
			else {
				i = search_account_index2(acc);
				/* if account found */
				if (i != -1) {
					if (auth_dat[i].sex == 2)
							write_log("Char-server '%s': Creation of an e-mail on an account with a default e-mail: REFUSED - account is Server account (account: %d, ip: %s)." RETCODE,
							          server[id].name, acc, ip);
					else {
						if (strcmp(auth_dat[i].email, "a@a.com") == 0 || auth_dat[i].email[0] == '\0') {
							strncpy(auth_dat[i].email, email, 40);
							auth_dat[i].email[40] = '\0';
							write_log("Char-server '%s': Creation of an e-mail on an account with a default e-mail: ok (account: %d, new e-mail: %s, ip: %s)." RETCODE,
							          server[id].name, acc, email, ip);
							/* Save */
							save_account(i, 1);
						} else
							write_log("Char-server '%s': Creation of an e-mail on an account with a default e-mail: REFUSED - e-mail of account isn't default e-mail (account: %d, ip: %s)." RETCODE,
							          server[id].name, acc, ip);
					}
				/* if account not found */
				} else
					write_log("Char-server '%s': Creation of an e-mail on an account with a default e-mail: REFUSED - unknown account (account: %d, ip: %s)." RETCODE,
					          server[id].name, acc, ip);
			}
		  }
			RFIFOSKIP(fd,46);
			break;

		/* We receive an e-mail/limited time request, because a player comes back from a map-server to the char-server */
		case 0x2716:
			if (RFIFOREST(fd) < 6)
				return 0;
			server_freezeflag[id] = anti_freeze_counter; /* Char anti-freeze system. Counter. 6 ok, 5...0 frozen - only on valid packet*/
			acc = RFIFOL(fd,2);
			//printf("parse_fromchar: E-mail/limited time request from '%s' server (concerned account: %d)\n", server[id].name, acc);
			i = search_account_index2(acc);
			/* if account found */
			if (i != -1) {
				write_log("Char-server '%s': Request of an e-mail/limited time: ok (account: %d, ip: %s)." RETCODE, server[id].name, acc, ip);
				WPACKETW( 0) = 0x2717;
				WPACKETL( 2) = acc;
				strncpy(WPACKETP(6), auth_dat[i].email, 40);
				WPACKETL(46) = (unsigned long)auth_dat[i].connect_until_time;
				SENDPACKET(fd, 50);
				// send gm level to update it in char-server and map-server if necessary
				send_GM_account(acc, auth_dat[i].level);
			/* if account not found */
			} else
				write_log("Char-server '%s': Request of an e-mail/limited time: unknown account (account: %d, ip: %s)." RETCODE, server[id].name, acc, ip);
			RFIFOSKIP(fd,6);
			break;

		/* To become GM request */
		case 0x2720:
			if (RFIFOREST(fd) < 8 || RFIFOREST(fd) < RFIFOW(fd,2))
				return 0;
			server_freezeflag[id] = anti_freeze_counter; /* Char anti-freeze system. Counter. 6 ok, 5...0 frozen - only on valid packet*/
		  {
			char *received_pass;
			acc = RFIFOL(fd,4);
			CALLOC(received_pass, char, RFIFOW(fd,2) - 8 + 1); // size of pass + NULL
			strncpy(received_pass, RFIFOP(fd,8), RFIFOW(fd,2) - 8);
			//printf("parse_fromchar: Request to become a GM acount from account #%d.\n", acc);
			WPACKETW(0) = 0x2721;
			WPACKETL(2) = acc;
			WPACKETW(6) = 0;
			if (strcmp(received_pass, gm_pass) == 0) {
				i = search_account_index2(acc);
				if (i != -1) {
					if (auth_dat[i].sex == 2)
							write_log("Char-server '%s': Request to become GM: account is Server account (suggested account: %d, correct password, ip: %s)." RETCODE,
							          server[id].name, acc, ip);
					else {
						/* only non-GM can become GM */
						if (auth_dat[i].level == 0) {
							/* if we autorise creation */
							if (level_new_gm > 0) {
								time_t now;
								char tmpstr[256];
								int tmpstr_len;
								write_log("Char-server '%s': Request to become GM: ok (account: %d, ip: %s)." RETCODE, server[id].name, acc, ip);
								WPACKETW(6) = level_new_gm;
								/* add comment in memo field */
								now = time(NULL);
								memset(tmpstr, 0, sizeof(tmpstr));
								tmpstr_len = strftime(tmpstr, 20, date_format, localtime(&now)); // 19 + NULL
								sprintf(tmpstr + tmpstr_len, ": @GM command - level changing %d->%d.", auth_dat[i].level, level_new_gm);
								add_text_in_memo(i, tmpstr);
								/* save level */
								auth_dat[i].level = level_new_gm;
#ifdef TXT_ONLY
								/* need to save GM level file if we save as old version type */
								GM_level_need_save_flag = 1;
#endif /* TXT_ONLY */
								/* Save */
								save_account(i, 1);
								/* send new level to all char-servers */
								send_GM_account(auth_dat[i].account_id, level_new_gm);
							} else
								write_log("Char-server '%s': Request to become GM: all is correct, but GM creation is disable (level_new_gm = 0) (account: %d, correct password, ip: %s)." RETCODE,
								          server[id].name, acc, ip);
						} else
							write_log("Char-server '%s': Request to become GM: account is already GM (suggested account: %d, correct password, ip: %s)." RETCODE,
							          server[id].name, acc, ip);
					}
				} else
					write_log("Char-server '%s': Request to become GM: unknown account (account %d, ip: %s)." RETCODE,
					          server[id].name, acc, ip);
			} else
				write_log("Char-server '%s': Request to become GM: invalid password (suggested account: %d, ip: %s)." RETCODE, server[id].name, acc, ip);
			charif_sendall(10);
			FREE(received_pass);
		  }
			RFIFOSKIP(fd, RFIFOW(fd,2));
			return 0;

		// Map server send information to change an email of an account via char-server
		case 0x2722: // 0x2722 <account_id>.L <actual_e-mail>.40B <new_e-mail>.40B
			if (RFIFOREST(fd) < 86)
				return 0;
			server_freezeflag[id] = anti_freeze_counter; /* Char anti-freeze system. Counter. 6 ok, 5...0 frozen - only on valid packet*/
		  {
			char actual_email[41], new_email[41]; // 40 + NULL
			acc = RFIFOL(fd,2);
			strncpy(actual_email, RFIFOP(fd,6), 40);
			actual_email[40] = '\0';
			remove_control_chars(actual_email);
			strncpy(new_email, RFIFOP(fd,46), 40);
			new_email[40] = '\0';
			remove_control_chars(new_email);
			// send answer
			memset(WPACKETP(0), 0, 46);
			WPACKETW(0) = 0x272b; // 0x272b/0x2b18 <account_id>.L <new_e-mail>.40B
			WPACKETL(2) = acc;
			//strncpy(WPACKETP(6), new_email, 40); // set to NULL to say: not changed
			if (e_mail_check(actual_email) == 0) {
				write_log("Char-server '%s': Request to modify an e-mail on an account (@email GM command): actual email is invalid (account: %d, ip: %s)" RETCODE,
				          server[id].name, acc, ip);
			} else if (e_mail_check(new_email) == 0) {
				write_log("Char-server '%s': Request to modify an e-mail on an account (@email GM command): invalid new e-mail (account: %d, ip: %s)" RETCODE,
				          server[id].name, acc, ip);
			} else if (strcasecmp(new_email, "a@a.com") == 0) {
				write_log("Char-server '%s': Request to modify an e-mail on an account (@email GM command): new email is the default e-mail (account: %d, ip: %s)" RETCODE,
				          server[id].name, acc, ip);
			} else {
				i = search_account_index2(acc);
				/* if account found */
				if (i != -1) {
					if (auth_dat[i].sex == 2)
							write_log("Char-server '%s': Request to modify an e-mail on an account (@email GM command): account is Server account (account: %d (%s), actual e-mail: %s, proposed e-mail: %s, ip: %s)." RETCODE,
							          server[id].name, acc, auth_dat[i].userid, auth_dat[i].email, actual_email, ip);
					else {
						if (strcasecmp(auth_dat[i].email, actual_email) == 0) {
							time_t now;
							char tmpstr[256];
							int tmpstr_len;
							strncpy(auth_dat[i].email, new_email, 40);
							auth_dat[i].email[40] = '\0';
							write_log("Char-server '%s': Request to modify an e-mail on an account (@email GM command): ok (account: %d (%s), new e-mail: %s, ip: %s)." RETCODE,
							          server[id].name, acc, auth_dat[i].userid, new_email, ip);
							// add comment in memo field
							now = time(NULL);
							memset(tmpstr, 0, sizeof(tmpstr));
							tmpstr_len = strftime(tmpstr, 20, date_format, localtime(&now)); // 19 + NULL
							sprintf(tmpstr + tmpstr_len, ": Email change from player %s->%s.", actual_email, new_email);
							add_text_in_memo(i, tmpstr);
							// Save
							save_account(i, 1);
							// send answer
							strncpy(WPACKETP(6), new_email, 40);
						} else {
							write_log("Char-server '%s': Request to modify an e-mail on an account (@email GM command): actual e-mail is incorrect (account: %d (%s), actual e-mail: %s, proposed e-mail: %s, ip: %s)." RETCODE,
							          server[id].name, acc, auth_dat[i].userid, auth_dat[i].email, actual_email, ip);
						}
					}
				/* if account not found */
				} else {
					write_log("Char-server '%s': Request to modify an e-mail on an account (@email GM command): unknown account (account: %d, ip: %s)." RETCODE,
					          server[id].name, acc, ip);
				}
			}
			charif_sendall(46);
		  }
			RFIFOSKIP(fd, 86);
			break;

		/* Receiving of map-server via char-server a status change resquest */
		case 0x2724:
			if (RFIFOREST(fd) < 10)
				return 0;
			server_freezeflag[id] = anti_freeze_counter; /* Char anti-freeze system. Counter. 6 ok, 5...0 frozen - only on valid packet*/
		  {
			int statut;
			acc = RFIFOL(fd,2);
			statut = RFIFOL(fd,6);
			i = search_account_index2(acc);
			/* if account found */
			if (i != -1) {
				if (auth_dat[i].sex == 2)
					write_log("Char-server '%s': Request to change a status: account is Server account (account: %d, suggested status %d, ip: %s)." RETCODE,
					          server[id].name, acc, statut, ip);
				else {
					if (auth_dat[i].state != statut) {
						write_log("Char-server '%s': Request to change a status: ok (account: %d, new status %d, ip: %s)." RETCODE,
						          server[id].name, acc, statut, ip);
						if (statut != 0) {
							WPACKETW(0) = 0x2731;
							WPACKETL(2) = acc;
							WPACKETB(6) = 0; /* 0: change of statut, 1: ban */
							WPACKETL(7) = statut; /* status or final date of a banishment */
							charif_sendall(11);
							for(j = 0; j < AUTH_FIFO_SIZE; j++)
								if (auth_fifo[j].account_id == acc)
									auth_fifo[j].login_id1++; /* to avoid reconnection error when come back from map-server (char-server will ask again the authentification) */
						}
						auth_dat[i].state = statut;
						/* Save */
						save_account(i, 1);
					} else
						write_log("Char-server '%s': Request to change a status: actual status is already the good status (account: %d, status %d, ip: %s)." RETCODE,
						          server[id].name, acc, statut, ip);
				}
			/* if account not found */
			} else
				write_log("Char-server '%s': Request to change a status: unknown account (account: %d, suggested status %d, ip: %s)." RETCODE,
				          server[id].name, acc, statut, ip);
			RFIFOSKIP(fd,10);
		  }
			return 0;

		/* Receiving of map-server via char-server a ban resquest */
		case 0x2725:
			if (RFIFOREST(fd) < 18)
				return 0;
			server_freezeflag[id] = anti_freeze_counter; /* Char anti-freeze system. Counter. 6 ok, 5...0 frozen - only on valid packet*/
			acc = RFIFOL(fd,2);
			i = search_account_index2(acc);
			/* if account found */
			if (i != -1) {
				if (auth_dat[i].sex == 2)
					write_log("Char-server '%s': ban request: account is Server account (account: %d, ip: %s)." RETCODE, server[id].name, acc, ip);
				else {
					time_t timestamp;
					struct tm *tmtime;
					if (auth_dat[i].ban_until_time == 0 || auth_dat[i].ban_until_time < time(NULL))
						timestamp = time(NULL);
					else
						timestamp = auth_dat[i].ban_until_time;
					tmtime = localtime(&timestamp);
					tmtime->tm_year = tmtime->tm_year + (short)RFIFOW(fd,6);
					tmtime->tm_mon = tmtime->tm_mon + (short)RFIFOW(fd,8);
					tmtime->tm_mday = tmtime->tm_mday + (short)RFIFOW(fd,10);
					tmtime->tm_hour = tmtime->tm_hour + (short)RFIFOW(fd,12);
					tmtime->tm_min = tmtime->tm_min + (short)RFIFOW(fd,14);
					tmtime->tm_sec = tmtime->tm_sec + (short)RFIFOW(fd,16);
					timestamp = mktime(tmtime);
					if (timestamp != -1) {
						if (timestamp <= time(NULL))
							timestamp = 0;
						if (auth_dat[i].ban_until_time != timestamp) {
							if (timestamp != 0) {
								char tmpstr[24];
								memset(tmpstr, 0, sizeof(tmpstr));
								strftime(tmpstr, 20, date_format, localtime(&timestamp)); // 19 + NULL
								write_log("Char-server '%s': ban request: ok (account: %d, new final date of banishment: %d (%s), ip: %s)." RETCODE,
								          server[id].name, acc, timestamp, (timestamp == 0 ? "no banishment" : tmpstr), ip);
								WPACKETW(0) = 0x2731;
								WPACKETL(2) = auth_dat[i].account_id;
								WPACKETB(6) = 1; /* 0: change of statut, 1: ban */
								WPACKETL(7) = timestamp; /* status or final date of a banishment */
								charif_sendall(11);
								for(j = 0; j < AUTH_FIFO_SIZE; j++)
									if (auth_fifo[j].account_id == acc)
										auth_fifo[j].login_id1++; /* to avoid reconnection error when come back from map-server (char-server will ask again the authentification) */
							} else
								write_log("Char-server '%s': ban request: new date unbans the account (account: %d, ip: %s)." RETCODE,
								          server[id].name, acc, ip);
							auth_dat[i].ban_until_time = timestamp;
							/* Save */
							save_account(i, 1);
						} else {
							write_log("Char-server '%s': ban request: no change for ban date (account: %d, ip: %s)." RETCODE,
							          server[id].name, acc, ip);
						}
					} else
						write_log("Char-server '%s': ban request: invalid date (account: %d, ip: %s)." RETCODE, server[id].name, acc, ip);
				}
			/* if account not found */
			} else
				write_log("Char-server '%s': ban request: unknown account (account: %d, ip: %s)." RETCODE, server[id].name, acc, ip);
			RFIFOSKIP(fd,18);
			return 0;

		/* Change of sex (sex is reversed) */
		case 0x2727:
			if (RFIFOREST(fd) < 10)
				return 0;
			server_freezeflag[id] = anti_freeze_counter; /* Char anti-freeze system. Counter. 6 ok, 5...0 frozen - only on valid packet*/
		  {
			char sex;
			acc = RFIFOL(fd,2);
			i = search_account_index2(acc);
			/* if account found */
			if (i != -1) {
				if (auth_dat[i].sex == 2) {
					write_log("Char-server '%s': Request of sex change: account is Server account (suggested account: %d, actual sex %d (Server), ip: %s)." RETCODE,
					          server[id].name, acc, auth_dat[i].sex, ip);
					WPACKETW(0) = 0x2723; // 0x2723 <account_id>.L <sex>.B <accound_it_of_GM>.L (sex = -1 -> failed; account_id_of_GM = -1 -> ladmin)
					WPACKETL(2) = acc;
					WPACKETB(6) = -1; // sex == -1 -> not found
					WPACKETL(7) = RFIFOL(fd,6); // who want do operation: -1, ladmin. other: account_id of GM
					charif_sendall(11);
				} else {
					if (auth_dat[i].sex == 0)
						sex = 1;
					else
						sex = 0;
					write_log("Char-server '%s': Request of sex change: ok (account: %d, new sex %c, ip: %s)." RETCODE,
					          server[id].name, acc, (sex == 2) ? 'S' : (sex ? 'M' : 'F'), ip);
					for(j = 0; j < AUTH_FIFO_SIZE; j++)
						if (auth_fifo[j].account_id == acc)
							auth_fifo[j].login_id1++; /* to avoid reconnection error when come back from map-server (char-server will ask again the authentification) */
					auth_dat[i].sex = sex;
					WPACKETW(0) = 0x2723; // 0x2723 <account_id>.L <sex>.B <account_id_of_GM>.L (sex = -1 -> failed; account_id_of_GM = -1 -> ladmin)
					WPACKETL(2) = acc;
					WPACKETB(6) = sex; // sex == -1 -> not found
					WPACKETL(7) = RFIFOL(fd,6); // who want do operation: -1, ladmin. other: account_id of GM
					charif_sendall(11);
					/* Save */
					save_account(i, 1);
				}
			/* if account not found */
			} else {
				write_log("Char-server '%s': Request of sex change: unknown account (account: %d, ip: %s)." RETCODE, server[id].name, acc, ip);
				WPACKETW(0) = 0x2723; // 0x2723 <account_id>.L <sex>.B <accound_it_of_GM>.L (sex = -1 -> failed; account_id_of_GM = -1 -> ladmin)
				WPACKETL(2) = acc;
				WPACKETB(6) = -1; // sex == -1 -> not found
				WPACKETL(7) = RFIFOL(fd,6); // who want do operation: -1, ladmin. other: account_id of GM
				charif_sendall(11);
			}
		  }
			RFIFOSKIP(fd,10);
			return 0;

		// We receive account_reg2 from a char-server, and we send them to other char-servers.
		case 0x2728: // send structure of account_reg2 from char-server to login-server
			if (RFIFOREST(fd) < 8 || RFIFOREST(fd) < RFIFOW(fd,2))
				return 0;
			server_freezeflag[id] = anti_freeze_counter; /* Char anti-freeze system. Counter. 6 ok, 5...0 frozen - only on valid packet*/
			acc = RFIFOL(fd,4);
			i = search_account_index2(acc);
			/* if account found */
			if (i != -1) {
				if (auth_dat[i].sex == 2)
					write_log("Char-server '%s': receiving (from the char-server) of account_reg2: account is Server account (account: %d, ip: %s)." RETCODE,
					          server[id].name, acc, ip);
				else {
					write_log("Char-server '%s': receiving (from the char-server) of account_reg2: ok (account: %d, ip: %s)." RETCODE,
					          server[id].name, acc, ip);
#ifdef TXT_ONLY
					FREE(auth_dat[i].account_reg2);
					auth_dat[i].account_reg2_num = (RFIFOW(fd,2) - 8) / sizeof(struct global_reg);
					if (auth_dat[i].account_reg2_num > 0) {
						MALLOC(auth_dat[i].account_reg2, struct global_reg, auth_dat[i].account_reg2_num);
						memcpy(auth_dat[i].account_reg2, RFIFOP(fd,8), RFIFOW(fd,2) - 8);
					}
					/* Save */
					save_account(i, 1);
#else // TXT_ONLY -> USE_SQL
					sql_request("DELETE FROM `account_reg2_db` WHERE `account_id` = '%d'", acc);
					if (RFIFOW(fd,2) > 8) {
						char *tmp_sql;
						int size, tmp_sql_len;
						struct global_reg *reg2;
						char t_reg2[65]; // (32 * 2) + 1
						t_reg2[64] = '\0';
						MALLOC(tmp_sql, char, 100 + ((RFIFOW(fd,2) - 8) / sizeof(struct global_reg)) * 70); // each element (str(33) + value(11) + (,,) (14) + security (2) = 60 ->70) + INSERT... (65)->100
						tmp_sql_len = 0;
						for(size = 8; size < RFIFOW(fd,2); size += sizeof(struct global_reg)) {
							reg2 = (struct global_reg*)RFIFOP(fd, size);
							mysql_escape_string(t_reg2, reg2->str, strlen(reg2->str));
							if (tmp_sql_len == 0)
								tmp_sql_len = sprintf(tmp_sql, "INSERT INTO `account_reg2_db`(`account_id`,`str`,`value`) VALUES ('%d', '%s', '%d')",
								                      acc, t_reg2, reg2->value);
							else
								tmp_sql_len += sprintf(tmp_sql + tmp_sql_len, ", ('%d', '%s', '%d')", acc, t_reg2, reg2->value);
						}
						if (tmp_sql_len != 0) {
							//printf("%s\n", tmp_sql);
							sql_request(tmp_sql);
						}
						FREE(tmp_sql);
					}
#endif // USE_SQL
					// Sending ACK to the char-server
					WPACKETW(0) = 0x272c; // 0x272c <account_id>.L
					WPACKETL(2) = acc;
					SENDPACKET(fd, 6);
//					printf("parse_fromchar: receiving (from the char-server) of account_reg2 (account id: %d).\n", acc);
				}
			/* if account not found */
			} else {
//				printf("parse_fromchar: receiving (from the char-server) of account_reg2 (unknwon account id: %d).\n", acc);
				write_log("Char-server '%s': receiving (from the char-server) of account_reg2: unknown account (account: %d, ip: %s)." RETCODE,
				          server[id].name, acc, ip);
			}
			RFIFOSKIP(fd,RFIFOW(fd,2));
			break;

		/* Receiving of map-server via char-server a unban resquest (no answer) */
		case 0x272a:
			if (RFIFOREST(fd) < 6)
				return 0;
			server_freezeflag[id] = anti_freeze_counter; /* Char anti-freeze system. Counter. 6 ok, 5...0 frozen - only on valid packet*/
			acc = RFIFOL(fd,2);
			i = search_account_index2(acc);
			/* if account found */
			if (i != -1) {
				if (auth_dat[i].sex == 2)
					write_log("Char-server '%s': UnBan request: account is Server account (account: %d, no change for unban date, ip: %s)." RETCODE, server[id].name, acc, ip);
				else {
					if (auth_dat[i].ban_until_time != 0) {
						auth_dat[i].ban_until_time = 0;
						write_log("Char-server '%s': UnBan request: ok (account: %d, ip: %s)." RETCODE, server[id].name, acc, ip);
						/* Save */
						save_account(i, 1);
					} else
						write_log("Char-server '%s': UnBan request: account is not banished (account: %d, no change for unban date, ip: %s)." RETCODE, server[id].name, acc, ip);
				}
			/* account doesn't exist */
			} else
				write_log("Char-server '%s': UnBan request: unknown account (account: %d, ip: %s)." RETCODE, server[id].name, acc, ip);
			RFIFOSKIP(fd,6);
			return 0;

		// Map server send information to change a password of an account via char-server
		case 0x272d: // 0x272d <account_id>.L <old_password>.32B <new_password>.32B
			if (RFIFOREST(fd) < 70)
				return 0;
			server_freezeflag[id] = anti_freeze_counter; /* Char anti-freeze system. Counter. 6 ok, 5...0 frozen - only on valid packet*/
		  {
			char old_password[33], new_password[33]; // 32 + NULL
			acc = RFIFOL(fd,2);
			strncpy(old_password, RFIFOP(fd,6), 32);
			old_password[32] = '\0';
			remove_control_chars(old_password);
			strncpy(new_password, RFIFOP(fd,38), 32);
			new_password[32] = '\0';
			remove_control_chars(new_password);
			// send answer
			memset(WPACKETP(0), 0, 38);
			WPACKETW(0) = 0x272e; // 0x272e/0x2b1e <account_id>.L <new_password>.32B
			WPACKETL(2) = acc;
			i = search_account_index2(acc);
			/* if account found */
			if (i != -1) {
				if (auth_dat[i].sex == 2)
						write_log("Char-server '%s': Request to modify a password on an account (@password GM command): account is Server account (account: %d (%s), old password: %s, proposed password: %s, ip: %s)." RETCODE,
							        server[id].name, acc, auth_dat[i].userid, auth_dat[i].pass, old_password, ip);
				else {
					if (strcasecmp(auth_dat[i].pass, old_password) == 0) {
						time_t now;
						char tmpstr[256];
						int tmpstr_len;
						strncpy(auth_dat[i].pass, new_password, 32);
						auth_dat[i].pass[32] = '\0';
						write_log("Char-server '%s': Request to modify a password on an account (@password GM command): ok (account: %d (%s), new password: %s, ip: %s)." RETCODE,
							        server[id].name, acc, auth_dat[i].userid, new_password, ip);
						// add comment in memo field
						now = time(NULL);
						memset(tmpstr, 0, sizeof(tmpstr));
						tmpstr_len = strftime(tmpstr, 20, date_format, localtime(&now)); // 19 + NULL
						sprintf(tmpstr + tmpstr_len, ": Password change from player %s->%s.", old_password, new_password);
						add_text_in_memo(i, tmpstr);
						// Save
						save_account(i, 1);
						// send answer
						strncpy(WPACKETP(6), new_password, 32);
					} else {
						write_log("Char-server '%s': Request to modify a password on an account (@password GM command): old password is incorrect (account: %d (%s), old password: %s, proposed password: %s, ip: %s)." RETCODE,
							        server[id].name, acc, auth_dat[i].userid, auth_dat[i].pass, old_password, ip);
					}
				}
			/* if account not found */
			} else {
				write_log("Char-server '%s': Request to modify a password on an account (@password GM command): unknown account (account: %d, ip: %s)." RETCODE,
					        server[id].name, acc, ip);
			}
			charif_sendall(38);
		  }
			RFIFOSKIP(fd, 70);
			break;

		/* Receiving of map-server via char-server a GM level change resquest */
		case 0x272f: // 0x272f <account_id>.L <accound_id_of_GM>.L <GM_level>.B (account_id_of_GM = -1 -> script)
			if (RFIFOREST(fd) < 11)
				return 0;
			server_freezeflag[id] = anti_freeze_counter; /* Char anti-freeze system. Counter. 6 ok, 5...0 frozen - only on valid packet*/
		  {
			char gm_level, gm_level_of_who_ask = -1;
			acc = RFIFOL(fd,2);
			gm_level = (char)RFIFOB(fd,10);
			// get GM level of who ask
			if (RFIFOL(fd,6) == -1) { // if script
				gm_level_of_who_ask = 100;
			} else {
				i = search_account_index2(RFIFOL(fd,6));
				if (i != -1)
					gm_level_of_who_ask = auth_dat[i].level;
			}
			if (gm_level_of_who_ask == -1) { // if nobody ask????
				write_log("Char-server '%s': Request of GM level change: GM who asks doesn't exist (account of the GM: %d, ip: %s)." RETCODE,
				          server[id].name, (int)RFIFOL(fd,6), ip);
				// note: no answer because no GM
			} else {
				// work on player to modify
				i = search_account_index2(acc);
				/* if account found */
				if (i != -1) {
					// check if it's a server account
					if (auth_dat[i].sex == 2) {
						write_log("Char-server '%s': Request of GM level change: account is Server account (Account: %d, suggested Level %d, ip: %s)." RETCODE,
						          server[id].name, acc, (int)RFIFOB(fd,10), ip);
						// do answer only if not a script
						if (RFIFOL(fd,6) != -1) {
							WPACKETW(0) = 0x272f; // 0x272f/0x2b21 <account_id>.L <GM_level>.B <accound_id_of_GM>.L (GM_level = -1 -> player not found, -2: gm level doesn't authorise you, -3: already right value; account_id_of_GM = -1 -> script)
							WPACKETL(2) = acc;
							WPACKETB(6) = -1; // GM level == -1 -> not found
							WPACKETL(7) = RFIFOL(fd,6); // who want do operation: -1, script. other: account_id of GM
							charif_sendall(11);
						}
					// check difference of levels
					} else if (auth_dat[i].level >= gm_level_of_who_ask) {
						write_log("Char-server '%s': Request of GM level change: GM doesn't have authorisation to modify this player (Player's account: %d (lvl: %d), suggested level %d, GM's account: %d (lvl: %d), ip: %s)." RETCODE,
						          server[id].name, acc, (int)auth_dat[i].level, (int)RFIFOB(fd,10), RFIFOL(fd,6), (int)gm_level_of_who_ask, ip);
						// do answer only if not a script
						if (RFIFOL(fd,6) != -1) {
							WPACKETW(0) = 0x272f; // 0x272f/0x2b21 <account_id>.L <GM_level>.B <accound_id_of_GM>.L (GM_level = -1 -> player not found, -2: gm level doesn't authorise you, -3: already right value; account_id_of_GM = -1 -> script)
							WPACKETL(2) = acc;
							WPACKETB(6) = -2; // GM level == -2: gm level doesn't authorise you
							WPACKETL(7) = RFIFOL(fd,6); // who want do operation: -1, script. other: account_id of GM
							charif_sendall(11);
						}
					} else if (auth_dat[i].level == gm_level || gm_level < 0 || gm_level > 99) {
						write_log("Char-server '%s': Request of GM level change: Player already have the right value (Player's account: %d, level %d, ip: %s)." RETCODE,
						          server[id].name, acc, (int)gm_level, ip);
						// do answer only if not a script
						if (RFIFOL(fd,6) != -1) {
							WPACKETW(0) = 0x272f; // 0x272f/0x2b21 <account_id>.L <GM_level>.B <accound_id_of_GM>.L (GM_level = -1 -> player not found, -2: gm level doesn't authorise you, -3: already right value; account_id_of_GM = -1 -> script)
							WPACKETL(2) = acc;
							WPACKETB(6) = -3; // GM level == -3: already right value
							WPACKETL(7) = RFIFOL(fd,6); // who want do operation: -1, script. other: account_id of GM
							charif_sendall(11);
						}
					} else {
						write_log("Char-server '%s': Request of GM level change: ok (Player's account: %d, level %d->%d, ip: %s)." RETCODE,
						          server[id].name, acc, (int)auth_dat[i].level, (int)gm_level, ip);
						// do answer only if not a script
						if (RFIFOL(fd,6) != -1) {
							WPACKETW(0) = 0x272f; // 0x272f/0x2b21 <account_id>.L <GM_level>.B <accound_id_of_GM>.L (GM_level = -1 -> player not found, -2: gm level doesn't authorise you, -3: already right value; account_id_of_GM = -1 -> script)
							WPACKETL(2) = acc;
							WPACKETB(6) = gm_level; // GM level
							WPACKETL(7) = RFIFOL(fd,6); // who want do operation: -1, script. other: account_id of GM
							charif_sendall(11);
						}
						/* send GM level to char-server and map server */
						send_GM_account(acc, gm_level);
						// Change GM level
						auth_dat[i].level = gm_level;
						// Save
						save_account(i, 1);
					}
				/* if account not found */
				} else {
					write_log("Char-server '%s': Request of GM level change: unknown account (account: %d, ip: %s)." RETCODE, server[id].name, acc, ip);
					// do answer only if not a script
					if (RFIFOL(fd,6) != -1) {
						WPACKETW(0) = 0x272f; // 0x272f/0x2b21 <account_id>.L <GM_level>.B <accound_id_of_GM>.L (GM_level = -1 -> player not found, -2: gm level doesn't authorise you, -3: already right value; account_id_of_GM = -1 -> script)
						WPACKETL(2) = acc;
						WPACKETB(6) = -1; // GM level == -1 -> not found
						WPACKETL(7) = RFIFOL(fd,6); // who want do operation: -1, script. other: account_id of GM
						charif_sendall(11);
					}
				}
			}
		  }
			RFIFOSKIP(fd,11);
			return 0;

		default:
		  {
			FILE *logfp;
			char tmpstr[24];
			struct timeval tv;
			time_t now;
			logfp = fopen(login_log_unknown_packets_filename, "a"); // it is not necessary to create a specifical file with the date (like done for log/login-2006.log)
			if (logfp) {
				gettimeofday(&tv, NULL);
				now = time(NULL);
				memset(tmpstr, 0, sizeof(tmpstr));
				strftime(tmpstr, 20, date_format, localtime(&now)); // 19 + NULL
				fprintf(logfp, "%s.%03d: receiving of an unknown packet -> disconnection" RETCODE, tmpstr, (int)tv.tv_usec / 1000);
				fprintf(logfp, "parse_fromchar: connection #%d (ip: %s), packet: 0x%x (with being read: %d)." RETCODE, fd, ip, RFIFOW(fd,0), RFIFOREST(fd));
				fprintf(logfp, "Detail (in hex):" RETCODE
				               "---- 00-01-02-03-04-05-06-07  08-09-0A-0B-0C-0D-0E-0F" RETCODE);
				memset(tmpstr, 0, sizeof(tmpstr));
				for(i = 0; i < RFIFOREST(fd); i++) {
					if ((i & 15) == 0)
						fprintf(logfp, "%04X ", i);
					fprintf(logfp, "%02x ", RFIFOB(fd,i));
					if (RFIFOB(fd,i) > 0x1f)
						tmpstr[i % 16] = RFIFOB(fd,i);
					else
						tmpstr[i % 16] = '.';
					if ((i - 7) % 16 == 0) /* -8 + 1 */
						fprintf(logfp, " ");
					else if ((i + 1) % 16 == 0) {
						fprintf(logfp, " %s" RETCODE, tmpstr);
						memset(tmpstr, 0, sizeof(tmpstr));
					}
				}
				if (i % 16 != 0) {
					for(j = i; j % 16 != 0; j++) {
						fprintf(logfp, "   ");
						if ((j - 7) % 16 == 0) /* -8 + 1 */
							fprintf(logfp, " ");
					}
					fprintf(logfp, " %s" RETCODE, tmpstr);
				}
				fprintf(logfp, RETCODE);
				fclose(logfp);
			}
		  }
			printf("parse_fromchar: Unknown packet 0x%x (from a char-server)! -> disconnection.\n", RFIFOW(fd,0));
			session[fd]->eof = 1;
			printf("Char-server has been disconnected (unknown packet).\n");
			return 0;
		}
	}

	return 0;
}

/*------------------------------------------------------------------------------------------
  Default packet parsing (normal players or administation/char-server connection requests)
------------------------------------------------------------------------------------------*/
int parse_login(int fd) {
	struct mmo_account account;
	int result, i, j;
	unsigned char *p = (unsigned char *) &session[fd]->client_addr.sin_addr;
	char ip[16];

	sprintf(ip, "%d.%d.%d.%d", p[0], p[1], p[2], p[3]);

	/* if not eof, we check ip and ban list */
	if (session[fd]->eof == 0) {
		time_t ban_time;

		/* check ip */
		if (!check_ip(session[fd]->client_addr.sin_addr.s_addr)) {
			write_log("Connection refused: IP isn't authorized (deny/allow, ip: %s)." RETCODE, ip);
			/* set eof */
			session[fd]->eof = 1;
			/* send special message if it's a connection/authentification request */
			if (RFIFOREST(fd) >= 2 && (RFIFOW(fd,0) == 0x64 || RFIFOW(fd,0) == 0x01dd || RFIFOW(fd,0) == 0x027c)) {
				memset(WPACKETP(0), 0, 23);
				WPACKETW(0) = 0x6a;
				WPACKETB(2) = 3; /* 3 = Rejected from Server */
				/* not need to add message of error 6 */
				SENDPACKET(fd, 23);
				return 0; /* do an additional loop to send the message if necessary */
			}
		}

		/* check password ban */
		if ((ban_time = check_banlist(session[fd]->client_addr.sin_addr.s_addr)) != 0) {
			/* don't write log, it was already writen when ban was decided */
			/* set eof */
			session[fd]->eof = 1;
			/* send special message if it's a connection/authentification request */
			if (RFIFOREST(fd) >= 2 && (RFIFOW(fd,0) == 0x64 || RFIFOW(fd,0) == 0x01dd || RFIFOW(fd,0) == 0x027c)) {
				memset(WPACKETP(0), 0, 23);
				WPACKETW( 0) = 0x6a;
				WPACKETB( 2) = 6; /* 6 = Your are Prohibited to log in until %s */
				strftime(WPACKETP(3), 20, date_format, localtime(&ban_time)); // 19 + NULL
				WPACKETB(22) = 0; /* force NULL */
				SENDPACKET(fd, 23);
				return 0; /* do an additional loop to send the message if necessary */
			}
		}
	}

	if (session[fd]->eof) {
#ifdef __WIN32
		shutdown(fd, SD_BOTH);
		closesocket(fd);
#else
		close(fd);
#endif
		delete_session(fd);
		return 0;
	}

	while(RFIFOREST(fd) >= 2 && !session[fd]->eof) {

		/* display parse if asked */
		if (display_parse_login == 1) {
			if (RFIFOW(fd,0) == 0x64 || RFIFOW(fd,0) == 0x01dd || RFIFOW(fd,0) == 0x027c) {
				if (RFIFOREST(fd) >= ((RFIFOW(fd,0) == 0x64) ? 55 : ((RFIFOW(fd,0) == 0x01dd) ? 47 : 60)))
					printf("parse_login: connection #%d, packet: 0x%x (with being read: %d), account: %s.\n", fd, RFIFOW(fd,0), RFIFOREST(fd), RFIFOP(fd,6));
			} else if (RFIFOW(fd,0) == 0x2710) {
				if (RFIFOREST(fd) >= 86)
					printf("parse_login: connection #%d, packet: 0x%x (with being read: %d), server: %s.\n", fd, RFIFOW(fd,0), RFIFOREST(fd), RFIFOP(fd,60));
			} else
				printf("parse_login: connection #%d, packet: 0x%x (with being read: %d).\n", fd, RFIFOW(fd,0), RFIFOREST(fd));
		}

		switch(RFIFOW(fd,0)) {

		/* New alive packet: structure: 0x200 <account.userid>.24B. used to verify if client is always alive. */
		case 0x200:
			if (RFIFOREST(fd) < 26)
				return 0;
			RFIFOSKIP(fd,26);
			break;

		/* New alive packet: structure: 0x204 <encrypted.account.userid>.16B. (new ragexe from 22 june 2004) */
		case 0x204:
			if (RFIFOREST(fd) < 18)
				return 0;
			RFIFOSKIP(fd,18);
			break;

		// 20051214 nProtect Part 1
		case 0x258:
			memset(WPACKETP(0), 0, 18);
			WPACKETW(0) = 0x0227;
			SENDPACKET(fd,18);
			RFIFOSKIP(fd,2);
			break;

		// 20051214 nProtect Part 2
		case 0x228:
			if (RFIFOREST(fd) < 18)
				return 0;
			WPACKETW(0) = 0x0259;
			WPACKETB(2) = 1;
			SENDPACKET(fd,3);
			RFIFOSKIP(fd,18);
			break;

		/* Ask connection of a client */
		case 0x64:
		/* Ask connection of a client (encryption mode) */
		case 0x01dd:
		/* new packet of client login connection (non encrypted) */
		case 0x027c:
		  {
			int length = 55; // default: 0x64
			switch(RFIFOW(fd,0)) {
			//case 0x64: length = 55; break;
			case 0x01dd: length = 47; break;
			case 0x027c: length = 60; break;
			}
			if (RFIFOREST(fd) < length)
				return 0;
			/* get parameters */
			account.version = RFIFOL(fd,2);
			memset(account.userid, 0, sizeof(account.userid));
			strncpy(account.userid, RFIFOP(fd,6), 24);
			remove_control_chars(account.userid);
			memset(account.passwd, 0, sizeof(account.passwd));
			if (length - 31 > sizeof(account.passwd) - 1)
				strncpy(account.passwd, RFIFOP(fd,30), sizeof(account.passwd) - 1);
			else
				memcpy(account.passwd, RFIFOP(fd,30), length - 31); // 0x64 (24), 0x1dd (16), 0x27c (how exactly??)
			remove_control_chars(account.passwd);
			account.passwdenc = (RFIFOW(fd,0) == 0x64) ? 0 : 3; /* A definition is given when making an encryption password correspond.
			                                                       It is 1 at the time of passwordencrypt.
			                                                       It is made into 2 at the time of passwordencrypt2.
			                                                       When it is made 3, it corresponds to both.*/
			/* log request */
			if (log_request_connection) {
				if (RFIFOW(fd,0) == 0x64)
					write_log("Request for connection (non encryption mode) of %s (ip: %s)." RETCODE, account.userid, ip);
				else
					write_log("Request for connection (encryption mode) of %s (ip: %s)." RETCODE, account.userid, ip);
			}
			/* check authentification */
			result = mmo_auth(&account, fd);
			/* if account is authorized */
			if (result == -1) {
				i = search_account_index(account.userid); /* SQL: mmo_auth have conserv data in memory, and there will be no other request */
				/* send information to all char-servers for disconnection (if account is online) */
				if (account.sex != 2) {
					WPACKETW(0) = 0x2732;
					WPACKETL(2) = account.account_id;
					charif_sendall(6);
				} // else don't disconnect. it's necessary for scripts asking about servers list
				if (min_level_to_connect > auth_dat[i].level) {
					write_log("Connection refused: the minimum GM level for connection is %d (account: %s, GM level: %d, ip: %s)." RETCODE,
					          min_level_to_connect, account.userid, auth_dat[i].level, ip);
					WPACKETW(0) = 0x81;
					WPACKETL(2) = 1; /* 01 = Server closed */
					SENDPACKET(fd,3);
					/* set eof */
					session[fd]->eof = 1;
				} else {
					/* if if account is already online */
					if (check_online_player(account.account_id)) {
						write_log("Connection refused: Account is already online (account: %s (%d), GM level: %d, ip: %s)." RETCODE,
						          account.userid, account.account_id, auth_dat[i].level, ip);
						WPACKETW(0) = 0x81;
						WPACKETL(2) = 8; /* 08 = Server still recognises your last login */
						SENDPACKET(fd,3);
						/* set eof */
						session[fd]->eof = 1;
					} else {
						int k;
						if (auth_dat[i].level)
							printf("Connection of the GM (level:" CL_BOLD "%d" CL_RESET ") account '" CL_GREEN "%s" CL_RESET "' accepted.\n", auth_dat[i].level, account.userid);
						else
							printf("Connection of the account '" CL_GREEN "%s" CL_RESET "' accepted.\n", account.userid);
						j = 47;
						for(k = 0; k < MAX_SERVERS; k++) { // max number of char-servers (and account_id values: 0 to max-1)
							if (server_fd[k] >= 0) {
								if (lan_ip_check(p))
									WPACKETL(j)  = lan_char_ip_addr; // pre calculated to speed up software (and check value before)
								else
									WPACKETL(j)  = server[k].ip;
								WPACKETW(j +  4) = server[k].port;
								strncpy(WPACKETP(j + 6), server[k].name, 20);
								WPACKETW(j + 26) = server[k].users;
								WPACKETW(j + 28) = server[k].maintenance;
								WPACKETW(j + 30) = server[k].new;
								j += 32;
							}
						}
						/* if at least 1 char-server */
						if (j > 47) {
							memset(WPACKETP(0), 0, 47);
							WPACKETW( 0) = 0x69;
							WPACKETW( 2) = j;
							WPACKETL( 4) = account.login_id1;
							WPACKETL( 8) = account.account_id;
							WPACKETL(12) = account.login_id2;
							WPACKETL(16) = 0; /* in old version, that was for ip (not more used) */
							strncpy(WPACKETP(20), account.lastlogin, 23); /* in old version, that was for name (not more used) */
							WPACKETB(46) = account.sex;
							SENDPACKET(fd, j);
							/* to conserv a maximum of authentification, search if account is already authentified and replace it
							   that will reduce multiple connection too */
							for(j = 0; j < AUTH_FIFO_SIZE; j++)
								if (auth_fifo[j].account_id == account.account_id)
									break;
							/* if not found, use next value */
							if (j == AUTH_FIFO_SIZE) {
								if (auth_fifo_pos >= AUTH_FIFO_SIZE)
									auth_fifo_pos = 0;
								j = auth_fifo_pos;
								auth_fifo_pos++;
							}
							auth_fifo[j].account_id = account.account_id;
							auth_fifo[j].login_id1 = account.login_id1;
							auth_fifo[j].login_id2 = account.login_id2;
							auth_fifo[j].sex = account.sex;
							auth_fifo[j].delflag = 0; // 0: accepted, 1: not valid/not init
							auth_fifo[j].ip = session[fd]->client_addr.sin_addr.s_addr;
							// Sending of the account_reg2 to all char-servers
							k = 8;
#ifdef TXT_ONLY
							for(j = 0; j < auth_dat[i].account_reg2_num; j++) {
								strncpy(WPACKETP(k), auth_dat[i].account_reg2[j].str, 32);
								WPACKETL(k + 32) = auth_dat[i].account_reg2[j].value;
								k += 36;
							}
#else // TXT_ONLY -> USE_SQL
							sql_request("SELECT `str`,`value` FROM `account_reg2_db` WHERE `account_id` = '%d'", account.account_id);
							while (sql_get_row()) {
								strncpy(WPACKETP(k), sql_get_string(0), 32);
								WPACKETL(k + 32) = sql_get_integer(1);
								k += 36;
							}
#endif // USE_SQL
							WPACKETW(0) = 0x2729; // Sending of the account_reg2 to char-server (must be sended before ACK)
							WPACKETW(2) = k;
							WPACKETL(4) = account.account_id;
							charif_sendall(k);
							/* send to all char-servers auth information (to avoid char->login->char about auth) */
							WPACKETW( 0) = 0x2719;
							WPACKETL( 2) = account.account_id;
							WPACKETL( 6) = account.login_id1;
							WPACKETL(10) = account.login_id2;
							WPACKETL(14) = session[fd]->client_addr.sin_addr.s_addr;
							WPACKETB(18) = account.sex;
							charif_sendall(19);
						/* if no char-server, don't send void list of servers, just disconnect the player with proper message */
						} else {
							write_log("Connection refused: there is no char-server online (account: %s, ip: %s)." RETCODE, account.userid, ip);
							WPACKETW(0) = 0x81;
							WPACKETL(2) = 1; /* 01 = Server closed */
							SENDPACKET(fd, 3);
							/* set eof */
							session[fd]->eof = 1;
						}
					}
				}
			/* account is not authorized */
			} else {
				memset(WPACKETP(0), 0, 23);
				WPACKETW( 0) = 0x6a;
				WPACKETB( 2) = result;
				if (result == 6) { /* 6 = Your are Prohibited to log in until %s */
					i = search_account_index(account.userid);
					if (i != -1) {
						if (auth_dat[i].ban_until_time != 0) /* if account is banned, we send ban timestamp */
							strftime(WPACKETP(3), 20, date_format, localtime(&auth_dat[i].ban_until_time)); // 19 + NULL
						else /* we send error message */
							strncpy(WPACKETP(3), auth_dat[i].error_message, 19); // 19 + NULL
					}
				}
				WPACKETB(22) = 0;
				SENDPACKET(fd,23);
				/* set eof */
				session[fd]->eof = 1;
			}
			RFIFOSKIP(fd,length);
		  }
			break;

		/* Sending request of the coding key */
		case 0x01db:
		/* Sending request of the coding key (administration packet) */
		case 0x791a:
			if (session[fd]->session_data) {
				printf("login: abnormal request of MD5 key (already opened session).\n");
				/* set eof */
				session[fd]->eof = 1;
			} else {
				struct login_session_data *ld;
				CALLOC(session[fd]->session_data, struct login_session_data, 1);
				ld = session[fd]->session_data;
				if (RFIFOW(fd,0) == 0x01db)
					write_log("Sending request of the coding key (ip: %s)" RETCODE, ip);
				else
					write_log("'ladmin': Sending request of the coding key (ip: %s)" RETCODE, ip);
				/* Creation of the coding key */
				memset(ld->md5key, 0, sizeof(ld->md5key));
				if (use_md5_passwds) {
					ld->md5keylen = 0;
				} else {
					ld->md5keylen = rand() % 4 + 12;
					for(i = 0; i < ld->md5keylen; i++)
						ld->md5key[i] = rand() % 255 + 1;
				}
				WPACKETW(0) = 0x01dc;
				WPACKETW(2) = 4 + ld->md5keylen;
				if (ld->md5keylen > 0)
					memcpy(WPACKETP(4), ld->md5key, ld->md5keylen);
				SENDPACKET(fd, 4 + ld->md5keylen);
			}
			RFIFOSKIP(fd,2);
			break;

		/* Connection request of a char-server */
		case 0x2710:
			if (RFIFOREST(fd) < 86)
				return 0;
			if (!check_charip(session[fd]->client_addr.sin_addr.s_addr)) {
				printf("Connection of a char-server REFUSED (char_allow, ip: %s).\n", ip);
				printf("   Check your login_freya.conf (option: charallowip)\n");
				printf("   if connection must be authorised.\n");
				write_log("Connection of a char-server REFUSED (char_allow, ip: %s)" RETCODE, ip);
				/* send answer */
				WPACKETW(0) = 0x2711;
				WPACKETB(2) = 3; // 0: accepted, 3: refused
				SENDPACKET(fd, 3);
				/* set eof */
				session[fd]->eof = 1;
			} else {
				char server_name[21]; // 20 + NULL
#ifdef USE_SQL
				char t_uid[41]; // 20 * 2 + NULL
#endif /* USE_SQL */
				memset(account.userid, 0, sizeof(account.userid));
				strncpy(account.userid, RFIFOP(fd,2), 24);
				remove_control_chars(account.userid);
				memset(account.passwd, 0, sizeof(account.passwd));
				strncpy(account.passwd, RFIFOP(fd,26), 24);
				remove_control_chars(account.passwd);
				account.passwdenc = 0;
				memset(server_name, 0, sizeof(server_name));
				strncpy(server_name, RFIFOP(fd,60), 20);
				remove_control_chars(server_name);
				write_log("Connection request of the char-server '%s' @ %d.%d.%d.%d:%d (ip: %s)" RETCODE,
				          server_name, RFIFOB(fd,54), RFIFOB(fd,55), RFIFOB(fd,56), RFIFOB(fd,57), RFIFOW(fd,58), ip);
				result = mmo_auth(&account, fd);
				if (result == -1 && account.sex == 2 && account.account_id >= 0 && account.account_id < MAX_SERVERS && server_fd[account.account_id] == -1) {
					write_log("Connection of the char-server '%s' accepted (account: %s, ip: %s)" RETCODE,
					          server_name, account.userid, ip);
					printf("Connection of the char-server '" CL_CYAN "%s" CL_RESET "' (" CL_WHITE "%d.%d.%d.%d:%d" CL_RESET ") accepted.\n",
					       server_name, RFIFOB(fd,54), RFIFOB(fd,55), RFIFOB(fd,56), RFIFOB(fd,57), RFIFOW(fd,58));
					memset(&server[account.account_id], 0, sizeof(struct mmo_char_server));
					server[account.account_id].ip          = RFIFOL(fd,54);
					server[account.account_id].port        = RFIFOW(fd,58);
					strncpy(server[account.account_id].name, server_name, 20);
					server[account.account_id].users       = 0;
					server[account.account_id].maintenance = RFIFOW(fd,82);
					server[account.account_id].new         = RFIFOW(fd,84);
					server_fd[account.account_id]          = fd;
					server_freezeflag[account.account_id]  = anti_freeze_counter; /* Char anti-freeze system. Counter. 6 ok, 5...0 frozen */
					WPACKETW(0) = 0x2711;
					WPACKETB(2) = 0; // 0: accepted, 3: refused
					SENDPACKET(fd, 3);
					session[fd]->func_parse = parse_fromchar;
					realloc_fifo(fd, RFIFOSIZE_SERVER, WFIFOSIZE_SERVER);
					create_sstatus_files();
#ifdef USE_SQL
					// better: use REPLACE, but there is no KEY in the table
					sql_request("DELETE FROM `sstatus` WHERE `index` = '%d'", account.account_id);
					mysql_escape_string(t_uid, server[account.account_id].name, strlen(server[account.account_id].name));
					sql_request("INSERT INTO `sstatus`(`index`, `name`, `user`) VALUES ('%d', '%s', '%d')", account.account_id, t_uid, 0);
#endif /* USE_SQL */
				} else {
					if (server_fd[account.account_id] != -1) {
						printf("Connection of the char-server '%s' REFUSED - already connected (account: %d-%s, ip: %s)\n",
						        server_name, account.account_id, account.userid, ip);
						printf("You must probably wait that the freeze system detects the disconnection.\n");
						write_log("Connection of the char-server '%s' REFUSED - already connected (account: %d-%s, ip: %s)" RETCODE,
						          server_name, account.account_id, account.userid, ip);
						write_log("You must probably wait that the freeze system detects the disconnection." RETCODE);
					} else {
						printf("Connection of the char-server '%s' REFUSED (account: %s, ip: %s).\n", server_name, account.userid, ip);
						write_log("Connection of the char-server '%s' REFUSED (account: %s, ip: %s)" RETCODE, server_name, account.userid, ip);
					}
					WPACKETW(0) = 0x2711;
					WPACKETB(2) = 3; // 0: accepted, 3: refused
					SENDPACKET(fd, 3);
					/* set eof */
					session[fd]->eof = 1;
				}
			}
			RFIFOSKIP(fd,86);
			return 0;

		/* Request of the server version */
		case 0x7530:
			if (log_request_version)
				write_log("Request of the server version: ok (ip: %s)" RETCODE, ip);
			WPACKETW(0) = 0x7531;
			WPACKETB(2) = FREYA_MAJORVERSION;
			WPACKETB(3) = FREYA_MINORVERSION;
			WPACKETB(4) = FREYA_REVISION;
			WPACKETB(5) = FREYA_STATE;
			WPACKETB(6) = 0;
			WPACKETB(7) = FREYA_LOGINVERSION;
			WPACKETW(8) = 0;
			SENDPACKET(fd, 10);
			session[fd]->eof = 1;
			RFIFOSKIP(fd,2);
			break;

		/* Request to end connection */
		case 0x7532:
			write_log("Request of end of connection: ok (ip: %s)" RETCODE, ip);
			/* set eof */
			session[fd]->eof = 1;
			RFIFOSKIP(fd,2);
			return 0;

		// Request of the server uptime
		case 0x7533:
			j = 0;
			for(i = 0; i < MAX_SERVERS; i++)
				if (server_fd[i] >= 0)
					j = j + server[i].users;
			if (log_request_uptime)
				write_log("Request of the server uptime: ok (ip: %s)" RETCODE, ip);
			WPACKETW(0) = 0x7534;
			WPACKETL(2) = time(NULL) - start_time;
			WPACKETW(6) = j;
			SENDPACKET(fd, 8);
			session[fd]->eof = 1;
			RFIFOSKIP(fd, 2);
			return 0;

		/* Request of the server version (freya version) */
		case 0x7535:
			if (log_request_freya_version)
				write_log("Request of the server version: ok (ip: %s)" RETCODE, ip);
			WPACKETW(0) = 0x7536;
			WPACKETB(2) = FREYA_MAJORVERSION;
			WPACKETB(3) = FREYA_MINORVERSION;
			WPACKETB(4) = FREYA_REVISION;
			WPACKETB(5) = FREYA_STATE;
			WPACKETB(6) = 0;
			WPACKETB(7) = FREYA_LOGINVERSION;
			WPACKETW(8) = 0;
#ifdef SVN_REVISION
			if (SVN_REVISION >= 1) // in case of .svn directories have been deleted
				WPACKETW(10) = SVN_REVISION;
			else
				WPACKETW(10) = 0;
#else
				WPACKETW(10) = 0;
#endif /* SVN_REVISION */
#ifdef TXT_ONLY
			WPACKETB(12) = 0;
#else
			WPACKETB(12) = 1;
#endif /* TXT_ONLY */
			SENDPACKET(fd, 13);
			session[fd]->eof = 1;
			RFIFOSKIP(fd,2);
			break;

		/* Request for administation login */
		case 0x7918:
			if (RFIFOREST(fd) < 20 || RFIFOREST(fd) < ((RFIFOW(fd,2) == 0) ? 28 : 20))
				return 0;
			if (!check_ladminip(session[fd]->client_addr.sin_addr.s_addr)) {
				write_log("'ladmin'-login: Connection in administration mode refused: IP isn't authorized (ladmin_allow, ip: %s)." RETCODE, ip);
				/* set eof */
				session[fd]->eof = 1;
				/* send answer */
				WPACKETW(0) = 0x7919;
				WPACKETB(2) = 1;
				SENDPACKET(fd, 3);
			} else {
				char password[25]; // 24 + NULL
				memset(password, 0, sizeof(password));
				if (RFIFOW(fd,2) == 0) { /* non encrypted password */
					memset(password, 0, sizeof(password));
					strncpy(password, RFIFOP(fd,4), 24);
					remove_control_chars(password);
					/* If remote administration is enabled and password sent by client matches */
					if (admin_state != 0 && password_check(admin_pass, password, "")) {
						write_log("'ladmin'-login: Connection in administration mode accepted (non encrypted password, ip: %s)" RETCODE, ip);
						printf("Connection of a remote administration accepted (non encrypted password).\n");
						session[fd]->func_parse = parse_admin;
						realloc_fifo(fd, RFIFOSIZE_SERVER, WFIFOSIZE_SERVER); /* read: biggest possible packet (FIFOW = 65536 or memo (limited by software to 60000)). write: list/ls of memo (limited by software to 60000) */
						/* send answer */
						WPACKETW(0) = 0x7919;
						WPACKETB(2) = 0;
						SENDPACKET(fd, 3);
					} else {
						if (admin_state == 0)
							write_log("'ladmin'-login: Connection in administration mode REFUSED - remote administration is disabled (non encrypted password, ip: %s)" RETCODE, ip);
						else
							write_log("'ladmin'-login: Connection in administration mode REFUSED - invalid password (non encrypted password, ip: %s)" RETCODE, ip);
						/* set eof */
						session[fd]->eof = 1;
						/* send answer */
						WPACKETW(0) = 0x7919;
						WPACKETB(2) = 1;
						SENDPACKET(fd, 3);
					}
				} else { /* encrypted password */
					struct login_session_data *ld = session[fd]->session_data;
					if (!ld) {
						printf("'ladmin'-login: error! MD5 key not created/requested for an administration login.\n");
						/* set eof */
						session[fd]->eof = 1;
						/* send answer */
						WPACKETW(0) = 0x7919;
						WPACKETB(2) = 1;
						SENDPACKET(fd, 3);
					} else {
						memcpy(password, RFIFOP(fd,4), 16);
						/* If remote administration is enabled and password hash sent by client matches */
						if (admin_state != 0 && password_check(admin_pass, password, ld->md5key)) {
							write_log("'ladmin'-login: Connection in administration mode accepted (encrypted password, ip: %s)" RETCODE, ip);
							printf("Connection of a remote administration accepted (encrypted password).\n");
							session[fd]->func_parse = parse_admin;
							realloc_fifo(fd, RFIFOSIZE_SERVER, WFIFOSIZE_SERVER); /* read: biggest possible packet (FIFOW = 65536 or memo (limited by software to 60000)). write: list/ls of memo (limited by software to 60000) */
							/* send answer */
							WPACKETW(0) = 0x7919;
							WPACKETB(2) = 0;
							SENDPACKET(fd, 3);
						} else {
							if (admin_state == 0)
								write_log("'ladmin'-login: Connection in administration mode REFUSED - remote administration is disabled (encrypted password, ip: %s)" RETCODE, ip);
							else
								write_log("'ladmin'-login: Connection in administration mode REFUSED - invalid password (encrypted password, ip: %s)" RETCODE, ip);
							/* set eof */
							session[fd]->eof = 1;
							/* send answer */
							WPACKETW(0) = 0x7919;
							WPACKETB(2) = 1;
							SENDPACKET(fd, 3);
						}
					}
				}
			}
			RFIFOSKIP(fd, (RFIFOW(fd,2) == 0) ? 28 : 20);
			break;

		/* Unknown packet */
		default:
			if (save_unknown_packets) {
				FILE *logfp;
				char tmpstr[24];
				struct timeval tv;
				time_t now;
				logfp = fopen(login_log_unknown_packets_filename, "a"); // it is not necessary to create a specifical file with the date (like done for log/login-2006.log)
				if (logfp) {
					gettimeofday(&tv, NULL);
					now = time(NULL);
					memset(tmpstr, 0, sizeof(tmpstr));
					strftime(tmpstr, 20, date_format, localtime(&now)); // 19 + NULL
					fprintf(logfp, "%s.%03d: receiving of an unknown packet -> disconnection" RETCODE, tmpstr, (int)tv.tv_usec / 1000);
					fprintf(logfp, "parse_login: connection #%d (ip: %s), packet: 0x%x (with being read: %d)." RETCODE, fd, ip, RFIFOW(fd,0), RFIFOREST(fd));
					fprintf(logfp, "Detail (in hex):" RETCODE
					               "---- 00-01-02-03-04-05-06-07  08-09-0A-0B-0C-0D-0E-0F" RETCODE);
					memset(tmpstr, 0, sizeof(tmpstr));
					for(i = 0; i < RFIFOREST(fd); i++) {
						if ((i & 15) == 0)
							fprintf(logfp, "%04X ", i);
						fprintf(logfp, "%02x ", RFIFOB(fd,i));
						if (RFIFOB(fd,i) > 0x1f)
							tmpstr[i % 16] = RFIFOB(fd,i);
						else
							tmpstr[i % 16] = '.';
						if ((i - 7) % 16 == 0) /* -8 + 1 */
							fprintf(logfp, " ");
						else if ((i + 1) % 16 == 0) {
							fprintf(logfp, " %s" RETCODE, tmpstr);
							memset(tmpstr, 0, sizeof(tmpstr));
						}
					}
					if (i % 16 != 0) {
						for(j = i; j % 16 != 0; j++) {
							fprintf(logfp, "   ");
							if ((j - 7) % 16 == 0) /* -8 + 1 */
								fprintf(logfp, " ");
						}
						fprintf(logfp, " %s" RETCODE, tmpstr);
					}
					fprintf(logfp, RETCODE);
					fclose(logfp);
				}
			}
			write_log("End of connection, unknown packet (ip: %s)" RETCODE, ip);
			/* set eof */
			session[fd]->eof = 1;
			return 0;
		}
	}

	return 0;
}

/*---------------------------------------
  password checking
  return 1 if correct, 0 if not correct
---------------------------------------*/
int password_check(const char * password_db, const char * pass, const char * key) {
	char md5str[64] = ""; // 32 + NULL or 16 + NULL
	unsigned char md5bin[33]; // 32 + NULL or 16 + NULL

	/* plain text compare */
	if (strcmp(password_db, pass) == 0)
		return 1;

	/* md5 compare without key - binary */
	MD5_String2binary(pass, md5bin);
	if (memcmp(md5bin, password_db, 16) == 0)
		return 1;

	/* md5 compare without key - HEX */
	MD5_String(pass, md5str);
	if (memcmp(md5str, password_db, 32) == 0)
		return 1;

	if (key[0] != '\0') { /* do compare with the key only if the key exists */
		/* md5 compare with key - HEX: pass + key */
		sprintf(md5str, "%s%s", password_db, key);
		MD5_String2binary(md5str, md5bin);
		if (memcmp(pass, md5bin, 16) == 0)
			return 1;

		/* md5 compare with key - HEX: key + pass */
		sprintf(md5str, "%s%s", key, password_db);
		MD5_String2binary(md5str, md5bin);
		if (memcmp(pass, md5bin, 16) == 0)
			return 1;
	}

	return 0;
}

/*---------------------------------------------------------------------
  Check a account name in database to know if already existing or not
---------------------------------------------------------------------*/
int check_existing_account_name_for_creation(const char *name) {
#ifdef TXT_ONLY
	int i;

	/* if we refuse creation of account with different case */
	if (unique_case_account_name_creation) {
		for(i = 0; i < auth_num; i++)
			if (strcasecmp(auth_dat[i].userid, name) == 0)
				return 1;
	} else {
		for(i = 0; i < auth_num; i++)
			if (strcmp(auth_dat[i].userid, name) == 0)
				return 1;
	}

#endif /* TXT_ONLY */

#ifdef USE_SQL
	char account_name[49];

	mysql_escape_string(account_name, name, strlen(name));
	sql_request("SELECT `%s` FROM `%s` WHERE `%s` = '%s'", login_db_userid, login_db, login_db_userid, account_name);
	/* if we refuse creation of account with different case */
	if (unique_case_account_name_creation) {
		while (sql_get_row())
			if (sql_get_string(0) != NULL && strcasecmp(sql_get_string(0), name) == 0)
				return 1;
	} else {
		while (sql_get_row())
			if (sql_get_string(0) != NULL && strcmp(sql_get_string(0), name) == 0)
				return 1;
	}
#endif /* USE_SQL */

	return 0;
}

#ifdef USE_SQL
/*-----------------------------------------------
  Fix incompatibility about GM level with eAthena
  (always auth_dat[0])
-------------------------------------------------*/
static void fix_gm_level(void) { // not inline, called too often
	// check GM level: must be done for eAthena compatibility about an incorrect usage of GM level
	if (auth_dat[0].level > 99) { // note: unsigned char -> can not be negativ
		time_t now;
		char tmpstr[256];
		int tmpstr_len;
		// set state to 5 (You have been blocked by the GM Team.) and level to 0
		sql_request("UPDATE `%s` SET `state` = 5, `%s` = 0 WHERE `%s` = %d", login_db, login_db_level, login_db_account_id, auth_dat[0].account_id);
		write_log("search_account_index: account #%d with an invalid GM level (%d) -> GM level set to 0 and state set to 5 (You have been blocked by the GM Team.)." RETCODE, auth_dat[0].account_id, auth_dat[0].level);
		// add comment in memo field
		now = time(NULL);
		memset(tmpstr, 0, sizeof(tmpstr));
		tmpstr_len = strftime(tmpstr, 20, date_format, localtime(&now)); // 19 + NULL
		sprintf(tmpstr + tmpstr_len, ": invalid GM level (%d) -> GM level set to 0 and state set to 5 (You have been blocked by the GM Team.).", auth_dat[0].level);
		add_text_in_memo(0, tmpstr);
		// set new values
		auth_dat[0].level = 0;
		auth_dat[0].state = 5;
		// save memo and new values
		save_account(0, 0); // not necessary to force save, because values are changed when account is loaded
	}

	return;
}
#endif /* USE_SQL */

/*------------------------------------------------
  Search an account index by name of account
    (return account index or -1 (if not found))
    If exact account name is not found,
    the function checks without case sensitive
    and returns index if only 1 account is found
    and similar to the searched name.
 -----------------------------------------------*/
int search_account_index(const char* name) {
	int quantity;

#ifdef TXT_ONLY
	int i, idx;

	if (strict_account_name_compare) {
		for(i = 0; i < auth_num; i++) {
			if (strcmp(auth_dat[i].userid, name) == 0)
				return i;
		}
	} else {
		idx = -1;
		quantity = 0;
		for(i = 0; i < auth_num; i++) {
			/* Without case sensitive check (increase the number of similar account names found) */
			if (strcasecmp(auth_dat[i].userid, name) == 0) {
				/* Strict comparison (if found, we finish the function immediatly with correct value) */
				if (strcmp(auth_dat[i].userid, name) == 0)
					return i;
				quantity++;
				idx = i;
			}
		}
		/* Here, the exact account name is not found
		   We return the found index of a similar account ONLY if there is 1 similar account */
		if (quantity == 1)
			return idx;
	}
#endif /* TXT_ONLY */

#ifdef USE_SQL
	char account_name[49]; // 24 * 2 + NULL
	char *temp_data;
	int memo_size;
	static time_t last_call = 0;

	/* if already in memory -> reduction of requests */
	if (strcmp(auth_dat[0].userid, name) == 0 && last_call > time(NULL)) /* don't conserv a read after 10 sec */
		return 0;

	/* set lastcall for 10 seconds */
	last_call = time(NULL) + 10;

	mysql_escape_string(account_name, name, strlen(name));
	sql_request("SELECT `%s`, `%s`, `%s`, `lastlogin`, `sex`, `logincount`, `email`, `%s`, "
	            "`error_message`, `connect_until`, `last_ip`, `memo`, `ban_until`, `state` "
	            "FROM `%s` WHERE `%s` = '%s'",
	            login_db_account_id, login_db_userid, login_db_user_pass, login_db_level,
	            login_db, login_db_userid, account_name);
	quantity = 0;
	while (sql_get_row()) {
		/* init data */
		FREE(auth_dat[0].memo);
		memset(&auth_dat[0], 0, sizeof(struct auth_dat));
		/* get data */
		auth_dat[0].account_id = sql_get_integer(0);
		strncpy(auth_dat[0].userid, sql_get_string(1), 24); // 25 - NULL
		strncpy(auth_dat[0].pass, sql_get_string(2), 32); // 33 - NULL
		strncpy(auth_dat[0].lastlogin, sql_get_string(3), 23); // 24 - NULL
		if (auth_dat[0].lastlogin[0] == '\0' || strcmp(auth_dat[0].lastlogin, "0000-00-00 00:00:00") == 0 || strcmp(auth_dat[0].lastlogin, "0000-00-00 00:00:00.000") == 0) {
			memset(auth_dat[0].lastlogin, 0, 24);
			strcpy(auth_dat[0].lastlogin, "-");
		}
		temp_data = sql_get_string(4);
		auth_dat[0].sex = (temp_data[0] == 'S' || temp_data[0] == 's') ? 2 : (temp_data[0] == 'M' || temp_data[0] == 'm');
		auth_dat[0].logincount = sql_get_integer(5);
		strncpy(auth_dat[0].email, sql_get_string(6), 40); // 41 - NULL
		if (e_mail_check(auth_dat[0].email) == 0) {
			memset(auth_dat[0].email, 0, sizeof(auth_dat[0].email));
			strcpy(auth_dat[0].email, "a@a.com");
		}
		auth_dat[0].level = sql_get_integer(7);
		strncpy(auth_dat[0].error_message, sql_get_string(8), 19); // 19 + NULL
		if (auth_dat[0].error_message[0] == '\0')
			strcpy(auth_dat[0].error_message, "-");
		auth_dat[0].connect_until_time = (time_t)sql_get_integer(9);
		strncpy(auth_dat[0].last_ip, sql_get_string(10), 15); // 16 - NULL
		if (auth_dat[0].last_ip[0] == '\0')
			strcpy(auth_dat[0].last_ip, "-");
		if ((temp_data = sql_get_string(11)) != NULL && ((memo_size = strlen(temp_data)) > 2 || (memo_size == 1 && temp_data[0] != '-'))) {
			/* limit size of memo to 60000 */
			if (memo_size > 60000)
				memo_size = 60000;
			CALLOC(auth_dat[0].memo, char, memo_size + 1);
			strncpy(auth_dat[0].memo, temp_data, memo_size);
		}
		auth_dat[0].ban_until_time = (time_t)sql_get_integer(12);
		auth_dat[0].state = sql_get_integer(13);
		/* Strict comparison (if found, we finish the function immediatly with correct value) */
		if (strcmp(auth_dat[0].userid, name) == 0) {
			// check GM level: must be done for eAthena compatibility about an incorrect usage of GM level
			fix_gm_level();
			return 0;
		}
		quantity++;
	}
	/* Here, the exact account name is not found
	   We return the found index of a similar account ONLY if there is 1 similar account */
	if (!strict_account_name_compare && quantity == 1) { // strict_account_name_compare == 0
		// check GM level: must be done for eAthena compatibility about an incorrect usage of GM level
		fix_gm_level();
		return 0;
	}

	/* not found, so init value */
	FREE(auth_dat[0].memo);
	memset(&auth_dat[0], 0, sizeof(struct auth_dat));
#endif /* USE_SQL */

	/* Exact account name is not found and 0 or more than 1 similar accounts have been found ==> we say not found */
	return -1;
}

/*------------------------------------------------
  Search an account index by id of account
    (return account index or -1 (if not found))
 -----------------------------------------------*/
int search_account_index2(const int account_id) {
#ifdef TXT_ONLY
	int i;

	for(i = 0; i < auth_num; i++)
		if (auth_dat[i].account_id == account_id)
			return i;
#endif /* TXT_ONLY */

#ifdef USE_SQL
	char *temp_data;
	int memo_size;
	static time_t last_call = 0;

	/* if already in memory -> reduction of requests */
	if (auth_dat[0].account_id == account_id && last_call > time(NULL)) /* don't conserv a read after 10 sec */
		return 0;

	/* set lastcall for 10 seconds */
	last_call = time(NULL) + 10;

	sql_request("SELECT `%s`, `%s`, `%s`, `lastlogin`, `sex`, `logincount`, `email`, `%s`, "
	            "`error_message`, `connect_until`, `last_ip`, `memo`, `ban_until`, `state` "
	            "FROM `%s` WHERE `%s` = '%d'",
	            login_db_account_id, login_db_userid, login_db_user_pass, login_db_level,
	            login_db, login_db_account_id, account_id);
	if (sql_get_row()) {
		/* init data */
		FREE(auth_dat[0].memo);
		memset(&auth_dat[0], 0, sizeof(struct auth_dat));
		/* get data */
		auth_dat[0].account_id = sql_get_integer(0);
		strncpy(auth_dat[0].userid, sql_get_string(1), 24); // 25 - NULL
		strncpy(auth_dat[0].pass, sql_get_string(2), 32); // 33 - NULL
		strncpy(auth_dat[0].lastlogin, sql_get_string(3), 23); // 24 - NULL
		if (auth_dat[0].lastlogin[0] == '\0' || strcmp(auth_dat[0].lastlogin, "0000-00-00 00:00:00") == 0 || strcmp(auth_dat[0].lastlogin, "0000-00-00 00:00:00.000") == 0) {
			memset(auth_dat[0].lastlogin, 0, 24);
			strcpy(auth_dat[0].lastlogin, "-");
		}
		temp_data = sql_get_string(4);
		auth_dat[0].sex = (temp_data[0] == 'S' || temp_data[0] == 's') ? 2 : (temp_data[0] == 'M' || temp_data[0] == 'm');
		auth_dat[0].logincount = sql_get_integer(5);
		strncpy(auth_dat[0].email, sql_get_string(6), 40); // 41 - NULL
		if (e_mail_check(auth_dat[0].email) == 0) {
			memset(auth_dat[0].email, 0, sizeof(auth_dat[0].email));
			strcpy(auth_dat[0].email, "a@a.com");
		}
		auth_dat[0].level = sql_get_integer(7);
		strncpy(auth_dat[0].error_message, sql_get_string(8), 19); // 19 + NULL
		if (auth_dat[0].error_message[0] == '\0')
			strcpy(auth_dat[0].error_message, "-");
		auth_dat[0].connect_until_time = (time_t)sql_get_integer(9);
		strncpy(auth_dat[0].last_ip, sql_get_string(10), 15); // 16 - NULL
		if (auth_dat[0].last_ip[0] == '\0')
			strcpy(auth_dat[0].last_ip, "-");
		if ((temp_data = sql_get_string(11)) != NULL && ((memo_size = strlen(temp_data)) > 2 || (memo_size == 1 && temp_data[0] != '-'))) {
			/* limit size of memo to 60000 */
			if (memo_size > 60000)
				memo_size = 60000;
			CALLOC(auth_dat[0].memo, char, memo_size + 1);
			memcpy(auth_dat[0].memo, temp_data, memo_size);
		}
		auth_dat[0].ban_until_time = (time_t)sql_get_integer(12);
		auth_dat[0].state = sql_get_integer(13);
		// check GM level: must be done for eAthena compatibility about an incorrect usage of GM level
		fix_gm_level();
		return 0;
	}

	/* not found, so init value */
	FREE(auth_dat[0].memo);
	memset(&auth_dat[0], 0, sizeof(struct auth_dat));
#endif /* USE_SQL */

	/* account_id is not found */
	return -1;
}

/*---------------------------------------------------------
  Create a string to save the account in the account file
---------------------------------------------------------*/
void account_to_str(char *str, struct auth_dat *p) {
#ifdef TXT_ONLY
	int i;
#endif // TXT_ONLY
	int memo_size;
	char *str_p = str;

	str_p += int2string(str_p, p->account_id);
	*(str_p++) = '\t';

	strcpy(str_p, p->userid);
	str_p += strlen(p->userid);
	*(str_p++) = '\t';

	strcpy(str_p, p->pass);
	str_p += strlen(p->pass);
	*(str_p++) = '\t';

	if (p->lastlogin[0]) {
		strcpy(str_p, p->lastlogin);
		str_p += strlen(p->lastlogin);
	} else
		*(str_p++) = '-';
	*(str_p++) = '\t';

	*(str_p++) = (p->sex == 2) ? 'S' : (p->sex ? 'M' : 'F');
	*(str_p++) = '\t';

	str_p += int2string(str_p, p->logincount);
	*(str_p++) = '\t';

	str_p += int2string(str_p, p->state);
	*(str_p++) = '\t';

	if (p->email[0]) {
		strcpy(str_p, p->email);
		str_p += strlen(p->email);
	} else
		*(str_p++) = '-';
	*(str_p++) = '\t';

	if (p->error_message[0]) {
		strcpy(str_p, p->error_message);
		str_p += strlen(p->error_message);
	} else
		*(str_p++) = '-';
	*(str_p++) = '\t';

	str_p += lint2string(str_p, (long int)p->connect_until_time);
	*(str_p++) = '\t';

	if (p->last_ip[0]) {
		strcpy(str_p, p->last_ip);
		str_p += strlen(p->last_ip);
	} else
		*(str_p++) = '-';
	*(str_p++) = '\t';

	if (p->memo == NULL)
		*(str_p++) = '-';
	else {
		memo_size = strlen(p->memo);
		/* limit size of memo to 60000 */
		if (memo_size > 60000)
			memo_size = 60000;
		strcpy(str_p, p->memo);
		str_p += memo_size;
	}
	*(str_p++) = '\t';

	str_p += lint2string(str_p, (long int)p->ban_until_time);
	*(str_p++) = '\t';

#ifdef TXT_ONLY
	if (save_GM_level_with_accounts) {
		str_p += int2string(str_p, p->level);
		*(str_p++) = '\t';
	}
#endif // TXT_ONLY

#ifdef USE_SQL
	str_p += int2string(str_p, p->level);
	*(str_p++) = '\t';
#endif /* USE_SQL */

#ifdef TXT_ONLY
	for(i = 0; i < p->account_reg2_num; i++)
		if (p->account_reg2[i].str[0]) {
			strcpy(str_p, p->account_reg2[i].str);
			str_p += strlen(p->account_reg2[i].str);
			*(str_p++) = ',';
			str_p += int2string(str_p, p->account_reg2[i].value);
			*(str_p++) = ' ';
		}
#else // TXT_ONLY -> USE_SQL
	sql_request("SELECT `str`,`value` FROM `account_reg2_db` WHERE `account_id` = '%d'", p->account_id);
	while (sql_get_row()) {
		strcpy(str_p, sql_get_string(0));
		str_p += strlen(sql_get_string(0));
		*(str_p++) = ',';
		str_p += int2string(str_p, sql_get_integer(1));
		*(str_p++) = ' ';
	}
#endif // USE_SQL

	*str_p = '\0';

	return;
}

/*--------------------------
  quick sorting
--------------------------*/
void accounts_speed_sorting(int tableau[], int premier, int dernier) {
	int temp, vmin, vmax, separateur_de_listes;

	vmin = premier;
	vmax = dernier;
	separateur_de_listes = auth_dat[tableau[(premier + dernier) / 2]].account_id;

	do {
		while(auth_dat[tableau[vmin]].account_id < separateur_de_listes)
			vmin++;
		while(auth_dat[tableau[vmax]].account_id > separateur_de_listes)
			vmax--;

		if (vmin <= vmax) {
			temp = tableau[vmin];
			tableau[vmin++] = tableau[vmax];
			tableau[vmax--] = temp;
		}
	} while(vmin <= vmax);

	if (premier < vmax)
		accounts_speed_sorting(tableau, premier, vmax);
	if (vmin < dernier)
		accounts_speed_sorting(tableau, vmin, dernier);
}

/*-----------------------------------------
  Writing of the accounts database file
  (accounts are sorted by id before save)
-----------------------------------------*/
void save_account(int idx, unsigned char forced) { /* forced: 1 = must be save, 0 = information is not important */
#ifdef TXT_ONLY
	char line[65536];
	FILE *fp;
	int i, j, lock;
	int *id;

	// don't save if server was not open. Perahps we have not already read all accounts, so we can lost information
	if (login_fd == -1)
		return;

	/* Save until for change ip/time of auth is not very useful => limited save for that
	   Save there informations isnot necessary, because they are saved in log file. */
	if (--auth_before_save_file <= 0 || forced) { /* Reduce counter. 0 or less, we save */
		CALLOC(id, int, auth_num);

		/* Sorting before save */
		for(i = 0; i < auth_num; i++)
			id[i] = i;
		if (auth_num > 1)
			accounts_speed_sorting(id, 0, auth_num - 1);

		/* Data save */
		if ((fp = lock_fopen(account_filename, &lock)) != NULL) {

			fprintf(fp, "// Accounts file" RETCODE
			            "//   This file contents all information about the accounts." RETCODE);
			if (save_GM_level_with_accounts)
				fprintf(fp, "// Structure: ID, account name, password, last login time, sex, # of logins, state, email, error message for state 7, validity time, last (accepted) login ip, memo field, ban timestamp, GM level, repeated(register text, register value)" RETCODE);
			else
				fprintf(fp, "// Structure: ID, account name, password, last login time, sex, # of logins, state, email, error message for state 7, validity time, last (accepted) login ip, memo field, ban timestamp, repeated(register text, register value)" RETCODE);
			fprintf(fp, "// Some explanations:" RETCODE
			            "//   ID              : unique numeric value of each account. Server ID MUST be between 0 to 29 (MAX_SERVERS)." RETCODE
			            "//   account name    : between 4 to 24 char for a normal account (standard client can't send less than 4 char)." RETCODE
			            "//   account password: between 4 to 24 char (exception: Server accounts can have password with less than 4 char)" RETCODE
			            "//   sex             : M or F for normal accounts, S for server accounts" RETCODE
			            "//   state           : 0: account is ok, 1 to 256: error code of packet 0x006a + 1" RETCODE
			            "//   email           : between 3 to 40 char (a@a.com is like no email)" RETCODE
			            "//   error message   : text for the state #7: 'Your are Prohibited to login until <text>'. Max 19 char" RETCODE
			            "//   valitidy time   : 0: unlimited account, <other value>: date calculated by addition of 1/1/1970 + value (number of seconds since the 1/1/1970)" RETCODE
			            "//   memo field      : max 60000 char" RETCODE
			            "//   ban time        : 0: no ban, <other value>: banned until the date: date calculated by addition of 1/1/1970 + value (number of seconds since the 1/1/1970)" RETCODE);
			if (save_GM_level_with_accounts)
				fprintf(fp, "//   GM level        : 0: normal player, <other value>: GM level" RETCODE);

			for(i = 0; i < auth_num; i++) {
				j = id[i]; /* use of sorted index */
				if (auth_dat[j].account_id < 0)
					continue;

				account_to_str(line, &auth_dat[j]);
				fputs(line, fp);
				fputs(RETCODE, fp);
			}
			fprintf(fp, "%d\t%%newid%%" RETCODE, account_id_count);

			lock_fclose(fp, account_filename, &lock);
		}

		/* Save GM levels in a different file if necessary */
		if (!save_GM_level_with_accounts && GM_level_need_save_flag == 1) {
			if ((fp = lock_fopen(GM_account_filename, &lock)) != NULL) {

				fprintf(fp, "// GM levels file" RETCODE
				            "// Setting the account ID which you recognize as GM" RETCODE
				            "//   Level: 0 (normal player), and from 1 to 99 maximum level." RETCODE
				            "//          Note here are only levels 1 to 99. Default level of an account is level 0." RETCODE
				            "//          It's not necessary to set a level to 0, except if you want to cancel a GM level at start." RETCODE
				            "//   Usage #1(Standard): <account id> <level>" RETCODE
				            "//   Usage #2(Range):    <beginning of range>-<end of range> <level>" RETCODE
				            "// Examples:" RETCODE
				            "//   2000002 99" RETCODE
				            "//   2000003-2000005 99" RETCODE);

				for(i = 0; i < auth_num; i++) {
					j = id[i]; /* use of sorted index */
					if (auth_dat[j].account_id < 0 || auth_dat[j].level < 1)
						continue;

					fprintf(fp, RETCODE "// %s" RETCODE "%d %d" RETCODE, auth_dat[j].userid, auth_dat[j].account_id, auth_dat[j].level);
				}
				lock_fclose(fp, GM_account_filename, &lock);
				GM_level_need_save_flag = 0;
			}
		}

		/* set new counter to minimum number of auth before save */
		auth_before_save_file = auth_num / AUTH_SAVE_FILE_DIVIDER; /* Re-initialise counter. We have save.*/
		if (auth_before_save_file < AUTH_BEFORE_SAVE_FILE)
			auth_before_save_file = AUTH_BEFORE_SAVE_FILE;

		FREE(id);
	}
#endif /* TXT_ONLY */

#ifdef USE_SQL
	struct auth_dat *p;
	char account_name[49], pass[65], email[81], error_message[39]; // 24*2 + NULL, 32*2 + NULL, 40*2+NULL, 19*2+NULL
	char *memo;

	// don't save if server was not open. No account have been loaded
	if (login_fd == -1)
		return;

	p = &auth_dat[idx];
	if (p->userid[0] != '\0') { // if we have loaded an account (example: 'save' console command without loaded account)
		mysql_escape_string(account_name, p->userid, strlen(p->userid));
		mysql_escape_string(pass, p->pass, strlen(p->pass));
		mysql_escape_string(email, p->email, strlen(p->email));
		mysql_escape_string(error_message, p->error_message, strlen(p->error_message));
		if (p->memo == NULL) {
			sql_request("REPLACE INTO `%s` (`%s`, `%s`, `%s`, `lastlogin`, `sex`, `logincount`, `email`, `%s`, "
			            "`error_message`, `connect_until`, `last_ip`, `memo`, `ban_until`, `state`) "
			            " VALUES ('%d', '%s', '%s', '%s', '%c', '%d', '%s', '%d', "
			            "'%s', '%ld', '%s', '%s', '%ld', '%d')",
			            login_db, login_db_account_id, login_db_userid, login_db_user_pass, login_db_level,
			            p->account_id, account_name, pass, p->lastlogin, (p->sex == 2) ? 'S' : (p->sex ? 'M' : 'F'), p->logincount, email, p->level,
			            error_message, (long int)p->connect_until_time, p->last_ip, "-", (long int)p->ban_until_time, p->state);
		} else {
			CALLOC(memo, char, strlen(p->memo) * 2 + 1);
			mysql_escape_string(memo, p->memo, strlen(p->memo));
			sql_request("REPLACE INTO `%s` (`%s`, `%s`, `%s`, `lastlogin`, `sex`, `logincount`, `email`, `%s`, "
			            "`error_message`, `connect_until`, `last_ip`, `memo`, `ban_until`, `state`) "
			            " VALUES ('%d', '%s', '%s', '%s', '%c', '%d', '%s', '%d', "
			            "'%s', '%ld', '%s', '%s', '%ld', '%d')",
			            login_db, login_db_account_id, login_db_userid, login_db_user_pass, login_db_level,
			            p->account_id, account_name, pass, p->lastlogin, (p->sex == 2) ? 'S' : (p->sex ? 'M' : 'F'), p->logincount, email, p->level,
			            error_message, (long int)p->connect_until_time, p->last_ip, memo, (long int)p->ban_until_time, p->state);
			FREE(memo);
		}
	}
#endif /* USE_SQL */

	return;
}

#ifdef TXT_ONLY
/*-----------------------------------------------------
  Check if we must save accounts file or not.
    We check if we must save because we have
    do some authentifications without arrive to
    the minimum of authentifications for the save.
  Note: all other modifications of accounts (deletion,
        change of some informations excepted lastip/
        lastlogintime, creation) are always save
        immediatly and set the minimum of
        authentifications to its initialization value.
------------------------------------------------------*/
int check_account_sync(int tid, unsigned int tick, int id, int data) {
	/* we only save if necessary: we have do some authentifications without do saving */
	if (auth_before_save_file < AUTH_BEFORE_SAVE_FILE ||
	    auth_before_save_file < (auth_num / AUTH_SAVE_FILE_DIVIDER))
		save_account(0, 1);

	return 0;
}
#endif /* TXT_ONLY */

/*-------------------------
  Adding an account in db
-------------------------*/
int add_account(struct auth_dat * account, const char *memo) {
	int memo_size;

#ifdef TXT_ONLY
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

	if (account->account_id >= account_id_count)
		account_id_count = account->account_id + 1;

	auth_num++;

	return auth_num - 1;
#endif /* TXT_ONLY */

#ifdef USE_SQL
	char account_name[49], pass[65], email[81], error_message[39]; // 24*2 + NULL, 32*2 + NULL, 40*2+NULL, 19*2+NULL
	char *line; /* add account is not often called: dynamic memory is better to reduce memory usage and don't reduce speed, because it's not often called */

	mysql_escape_string(account_name, account->userid, strlen(account->userid));
	mysql_escape_string(pass, account->pass, strlen(account->pass));
	mysql_escape_string(email, account->email, strlen(account->email));
	mysql_escape_string(error_message, account->error_message, strlen(account->error_message));
	if (memo == NULL) {
		if (sql_request("INSERT INTO `%s` (`%s`, `%s`, `lastlogin`, `sex`, `logincount`, `email`, `%s`, " /* login_db_account_id: AUTO_INCREMENT */
		                "`error_message`, `connect_until`, `last_ip`, `memo`, `ban_until`, `state`) "
		                " VALUES ('%s', '%s', '%s', '%c', '%d', '%s', '%d', "
		                "'%s', '%ld', '%s', '%s', '%ld', '%d')",
		                login_db, login_db_userid, login_db_user_pass, login_db_level,
		                account_name, pass, account->lastlogin, (account->sex == 2) ? 'S' : (account->sex ? 'M' : 'F'), account->logincount, email, account->level,
		                error_message, (long int)account->connect_until_time, account->last_ip, "-", (long int)account->ban_until_time, account->state)) {
#ifdef USE_MYSQL
			sql_request("SELECT LAST_INSERT_ID()");
#else /* USE_MYSQL -> other SQL */
			sql_request("SELECT `%s` FROM `%s` WHERE `%s` = '%s'", login_db_account_id, login_db, login_db_userid, account_name);
#endif /* other SQL */
			if (sql_get_row()) {
				account->account_id = sql_get_integer(0);
				if (account->account_id >= account_id_count)
					account_id_count = account->account_id + 1;
			} else
				account->account_id = account_id_count++;
		}
	} else {
		CALLOC(line, char, strlen(memo) * 2 + 1);
		mysql_escape_string(line, memo, strlen(memo));
		if (sql_request("INSERT INTO `%s` (`%s`, `%s`, `%s`, `lastlogin`, `sex`, `logincount`, `email`, `%s`, " /* login_db_account_id: AUTO_INCREMENT */
		                "`error_message`, `connect_until`, `last_ip`, `memo`, `ban_until`, `state`) "
		                " VALUES ('%s', '%s', '%s', '%c', '%d', '%s', '%d', "
		                "'%s', '%ld', '%s', '%s', '%ld', '%d')",
		                login_db, login_db_userid, login_db_user_pass, login_db_level,
		                account_name, pass, account->lastlogin, (account->sex == 2) ? 'S' : (account->sex ? 'M' : 'F'), account->logincount, email, account->level,
		                error_message, (long int)account->connect_until_time, account->last_ip, line, (long int)account->ban_until_time, account->state)) {
		FREE(line);
#ifdef USE_MYSQL
			sql_request("SELECT LAST_INSERT_ID()");
#else /* USE_MYSQL -> other SQL */
			sql_request("SELECT `%s` FROM `%s` WHERE `%s` = '%s'", login_db_account_id, login_db, login_db_userid, account_name);
#endif /* other SQL */
			if (sql_get_row()) {
				account->account_id = sql_get_integer(0);
				if (account->account_id >= account_id_count)
					account_id_count = account->account_id + 1;
			} else
				account->account_id = account_id_count++;
		}
	}

	FREE(auth_dat[0].memo);
	memcpy(&auth_dat[0], account, sizeof(struct auth_dat));
	auth_dat[0].memo = NULL;

	if (memo != NULL && ((memo_size = strlen(memo)) > 2 || (memo_size == 1 && memo[0] != '-'))) {
		// limit size of memo to 60000
		if (memo_size > 60000)
			memo_size = 60000;
		CALLOC(auth_dat[0].memo, char, memo_size + 1); // + NULL
		memcpy(auth_dat[0].memo, memo, memo_size);
	}

	return 0;
#endif /* USE_SQL */
}

/*-------------------------------------------------
  Init default values of an account
  Init all values, except: name, password and sex
-------------------------------------------------*/
void init_new_account(struct auth_dat * account) {
#ifdef TXT_ONLY
	int i;
#endif /* TXT_ONLY */
	time_t timestamp, timestamp_temp;
	struct tm *tmtime;
//	char password[25]; // 24 + NULL

	/* init structure */
	memset(account, 0, sizeof(struct auth_dat));

	/* init each value */
#ifdef TXT_ONLY
	/* searching first account id that is not a GM account */
	do {
		for(i = 0; i < auth_num; i++)
			if (auth_dat[i].account_id == account_id_count && auth_dat[i].level > 0) {
				account_id_count++;
				break;
			}
	} while (i != auth_num);
#endif /* TXT_ONLY */
	account->account_id = account_id_count++;
/*	strncpy(account->userid, "", 24); */
/*	if (use_md5_passwds) {
		memset(password, 0, sizeof(password));
		MD5_String(account->passwd, password);
	} else {
		strncpy(account->passwd, "", 24);
	} */
/*	account->sex = (sex == 'M'); */
	strcpy(account->email, "a@a.com");
	strcpy(account->lastlogin, "-");
/*	account->logincount = 0; */
/*	account->state = 0; */
	strcpy(account->error_message, "-");
/*	account->ban_until_time = 0; */
	if (start_limited_time < 0) {
/*		account->connect_until_time = 0; // unlimited */
	} else { /* limited time */
		timestamp = time(NULL) + start_limited_time;
		/* double conversion to be sure that it is possible */
		tmtime = localtime(&timestamp);
		timestamp_temp = mktime(tmtime);
		if (timestamp_temp != -1 && (timestamp_temp + 3600) >= timestamp) /* check possible value and overflow (and avoid summer/winter hour) */
			account->connect_until_time = timestamp_temp;
		else {
/*			account->connect_until_time = 0; // unlimited */
		}
	}
	strcpy(account->last_ip, "-");
/*	account->memo = NULL; */
	account->level = level_new_account;
#ifdef TXT_ONLY
/*	account->account_reg2_num = 0; */
/*	account->account_reg2 = NULL; */
#endif // TXT_ONLY

	return;
}

/*--------------------------------------------------------
  Deletion of an account in db (only ladmin can do that)
--------------------------------------------------------*/
void delete_account(const char * account_name) {
	int i;

	i = search_account_index(account_name);
	if (i != -1) {
		/* Char-server is notified of deletion (for characters deletion). */
		WPACKETW(0) = 0x2730;
		WPACKETL(2) = auth_dat[i].account_id;
		charif_sendall(6);
		/* delete account */
#ifdef TXT_ONLY
		FREE(auth_dat[i].memo);
		FREE(auth_dat[i].account_reg2);
		memset(auth_dat[i].userid, 0, sizeof(auth_dat[i].userid));
		auth_dat[i].account_id = -1;
		save_account(i, 1);
#endif /* TXT_ONLY */
#ifdef USE_SQL
		sql_request("DELETE FROM `%s` WHERE `%s` = '%d'", login_db, login_db_account_id, auth_dat[i].account_id); /* account id speeder than account name */
		sql_request("DELETE FROM `account_reg2_db` WHERE `account_id` = '%d'", auth_dat[i].account_id); // delete account_reg2
		FREE(auth_dat[i].memo);
		memset(&auth_dat[i], 0, sizeof(struct auth_dat));
#endif /* USE_SQL */
	}

	return;
}

#ifdef TXT_ONLY
/*--------------------------------------------------------------------------
  Reading of GM accounts file (and their level) - to conserv compatibility
--------------------------------------------------------------------------*/
static inline void read_gm_file(void) {
	char line[512];
	FILE *fp;
	int account_id, level; // level is not unsigned char to read file
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
						auth_dat[i].level = (unsigned char)level;
						break;
					}
			}
		}
	}
	fclose(fp);

	/* if we save GM levels with accounts */
	if (save_GM_level_with_accounts) {
		save_account(0, 1);
	/* we have read a new version type of account, and we save GM account file */
	} else if (GM_level_need_save_flag == 1) {
		save_account(0, 1); // save accounts and GM file (old structure)
	}

	return;
}
#endif /* TXT_ONLY */

/*-----------------------------------------------
  Initialisation of accounts database in memory
-----------------------------------------------*/
static inline void init_db(void) {
#define LINE_SIZE 65535
	int num_accounts = 0, server_count = 0, GM_counter = 0;
	char *line, *str;
	int i;

#ifdef TXT_ONLY
	FILE *fp;
	int account_id, logincount, state, level, n, j, v;
	char *p, *userid, *pass, *lastlogin, sex, *email, *error_message, *last_ip, *memo;
	long int ban_until_time, connect_until_time; // don't use time_t to avoid compilation error on some system
	struct auth_dat tmp_account;
#endif /* TXT_ONLY */

#ifdef USE_SQL
	char * temp_sex;
#endif /* USE_SQL */

	/* use dynamic allocation to reduce memory use when server is working */
	CALLOC(line, char, LINE_SIZE);
	CALLOC(str, char, LINE_SIZE);

	/* create online database */
	online_num = 0;
	online_max = 256;
	CALLOC(online_db, struct online_db, 256);
	for (i = 0; i < online_max; i++) {
		online_db[i].server = -1;
		//online_db[i].account_id = 0;
	}

#ifdef TXT_ONLY
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
				write_log("init_db: ******Error: an account has an id (%d) higher than %d -> account not read (saved in next line):" RETCODE, account_id, END_ACCOUNT_NUM);
				write_log("%s", line);
				continue;
			} else if (account_id < 0) {
				printf(CL_RED "init_db: ******Error: an account has an invalid id (%d)\n", account_id);
				printf(       "         account id #%d -> account not read (saved in log file)." CL_RESET "\n", account_id);
				write_log("init_db: ******Error: an account has an invalid id (%d) -> account not read (saved in next line):" RETCODE, account_id);
				write_log("%s", line);
				continue;
			}
			userid[24] = '\0';
			remove_control_chars(userid);
			for(j = 0; j < auth_num; j++) {
				if (auth_dat[j].account_id == account_id) {
					printf(CL_RED "init_db: ******Error: an account has an identical id to another.\n");
					printf(       "         account id #%d -> new account not read (saved in log file)." CL_RESET "\n", account_id);
					write_log("init_db: ******Error: an account has an identical id (%d) to another -> new account not read (saved in next line):" RETCODE, account_id);
					write_log("%s", line);
					break;
				} else if (strcmp(auth_dat[j].userid, userid) == 0) {
					printf(CL_RED "init_db: ******Error: account name already exists.\n");
					printf(       "         account name '%s' -> new account not read.\n", userid); /* 2 lines, account name can be long. */
					printf(       "         Account saved in log file." CL_RESET "\n");
					write_log("init_db: ******Error: an account has an identical name ('%s') to another -> new account not read (saved in next line):" RETCODE, userid);
					write_log("%s", line);
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
					tmp_account.level = (unsigned char)level;
				/* need to save GM level file if we save as old version type */
				GM_level_need_save_flag = 1;
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
				if (j == 0) {
					MALLOC(tmp_account.account_reg2, struct global_reg, 1);
				} else {
					REALLOC(tmp_account.account_reg2, struct global_reg, j + 1);
				}
				remove_control_chars(str);
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

		} else {
			i = 0;
			if (sscanf(line, "%d\t%%newid%%\n%n", &account_id, &i) == 1 && i > 0 && account_id > account_id_count)
				account_id_count = account_id;
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

	num_accounts = auth_num;
#endif /* TXT_ONLY */

#ifdef USE_SQL
	sql_init();

	auth_num = 1;
	auth_max = 1;
	CALLOC(auth_dat, struct auth_dat, 1);

	sql_request("SELECT COUNT(1), MAX(`%s`), `sex` FROM `%s` GROUP BY `sex`", login_db_account_id, login_db);
	while(sql_get_row()) {
		num_accounts += sql_get_integer(0);
		if (account_id_count <= sql_get_integer(1)) /* not the better way to found next id, but one solution */
			account_id_count = sql_get_integer(1) + 1;
		if ((temp_sex = sql_get_string(2)) != NULL && temp_sex[0] == 'S')
			server_count = sql_get_integer(0);
	}
	sql_request("SELECT COUNT(1) FROM `%s` WHERE `%s` > 0", login_db, login_db_level);
	if (sql_get_row())
		GM_counter = sql_get_integer(0);
#endif /* USE_SQL */

	/* display informations */
#ifdef TXT_ONLY
	if (num_accounts == 0) {
		printf("No account found in '%s'.\n", account_filename);
		write_log("No account found in '%s'." RETCODE, account_filename);
	} else {
		if (num_accounts == 1) {
			printf("1 account found in '%s',\n", account_filename);
			sprintf(line, "1 account found in '%s',", account_filename);
		} else {
			printf("%d accounts found in '%s',\n", num_accounts, account_filename);
			sprintf(line, "%d accounts found in '%s',", num_accounts, account_filename);
		}
#endif /* TXT_ONLY */
#ifdef USE_SQL
	if (num_accounts == 0) {
		printf("No account found in database '%s'.\n", login_db);
		write_log("No account found in database '%s'." RETCODE, login_db);
	} else {
		if (num_accounts == 1) {
			printf("1 account found in database '%s',\n", login_db);
			sprintf(line, "1 account found in database '%s',", login_db);
		} else {
			printf("%d accounts found in database '%s',\n", num_accounts, login_db);
			sprintf(line, "%d accounts found in database '%s',", num_accounts, login_db);
		}
#endif /* USE_SQL */
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
			write_log("%s of which is no server account ('S')." RETCODE, str);
		} else if (server_count == 1) {
			printf("of which is 1 server account ('S').\n");
			write_log("%s of which is 1 server account ('S')." RETCODE, str);
		} else {
			printf("of which are %d server accounts ('S').\n", server_count);
			write_log("%s of which are %d server accounts ('S')." RETCODE, str, server_count);
		}
	}

	/* free dynamic allocation */
	FREE(line);
	FREE(str);

	return;
}

/*--------------------------------
  Char-server anti-freeze system
--------------------------------*/
int char_anti_freeze_system(int tid, unsigned int tick, int id, int data) {
	int i;

	//printf("Entering in char_anti_freeze_system function to check freeze of servers.\n");
	for(i = 0; i < MAX_SERVERS; i++) { // max number of char-servers (and account_id values: 0 to max-1)
		if (server_fd[i] >= 0) { /* if char-server is online */
			//printf("char_anti_freeze_system: server #%d '%s', flag: %d.\n", i, server[i].name, server_freezeflag[i]);
			if (anti_freeze_interval != 0 && server_freezeflag[i]-- < 1) { /* Char-server anti-freeze system. Counter. 6 ok, 5...0 frozen */
				printf("Anti-freeze system: Char-server #%d '%s' is frozen -> disconnection.\n", i, server[i].name);
				printf("              It hasn't sended informations since %d seconds -> disconnection.\n", anti_freeze_interval * 1000 * anti_freeze_counter);
				write_log("Anti-freeze system: Char-server #%d '%s' is frozen -> disconnection." RETCODE, i, server[i].name);
				session[server_fd[i]]->eof = 1;
			} else {
				/* send alive packet to check connection */
				WPACKETW(0) = 0x2718;
				SENDPACKET(server_fd[i], 2);
			}
		}
	}

	return 0;
}

/*----------------------------------
  Console Commands Parser
----------------------------------*/
int parse_console(char *buf) {
	static int console_on = 1;
	int i, j;
	char command[4096], param[4096];

	memset(command, 0, sizeof(command));
	memset(param, 0, sizeof(param));

	/* get param of command */
	sscanf(buf, "%s %[^\n]", command, param);

/*	printf("Console command: %s %s\n", command, param); */

	if (!console_on) {

		write_log("Console command - disabled: %s %s" RETCODE, command, param);

		if (strcasecmp("?", command) == 0 ||
		    strcasecmp("h", command) == 0 ||
		    strcasecmp("help", command) == 0 ||
		    strcasecmp("aide", command) == 0) {
			printf(CL_DARK_GREEN "Help of commands:" CL_RESET "\n");
			printf("  '" CL_DARK_CYAN "?|h|help|aide" CL_RESET "': Display this help.\n");
			printf("  '" CL_DARK_CYAN "<console password>" CL_RESET "': enable console mode.\n");
		} else if (strcmp(console_pass, command) == 0) {
			printf(CL_DARK_CYAN "Console commands are now enabled." CL_RESET "\n");
			console_on = 1;
		} else {
			printf(CL_DARK_CYAN "Console commands are disabled. Please enter the password." CL_RESET "\n");
		}

	} else {

		write_log("Console command: %s %s" RETCODE, command, param);

		if (strcasecmp("shutdown", command) == 0 ||
		    strcasecmp("exit", command) == 0 ||
		    strcasecmp("quit", command) == 0 ||
		    strcasecmp("end", command) == 0) {
			exit(0);

		} else if (strcasecmp("alive", command) == 0 ||
		           strcasecmp("status", command) == 0 ||
		           strcasecmp("uptime", command) == 0) {
			j = 0;
			for(i = 0; i < MAX_SERVERS; i++)
				if (server_fd[i] >= 0)
					j = j + server[i].users;
			printf(CL_DARK_CYAN "Console: " CL_RESET CL_BOLD "I'm Alive for %u seconds." CL_RESET "\n", (int)(time(NULL) - start_time));
			printf(CL_DARK_CYAN "Console: " CL_RESET CL_BOLD "Number of online player%s: %d." CL_RESET "\n", (j > 1) ? "s" : "", j);

		} else if (strcasecmp("ladmin", command) == 0 ||
		           strcasecmp("admin", command) == 0) {
			/* display status of ladmin */
			if (param[0] == '\0') {
				if (admin_state != 0)
					printf("  " CL_BOLD "Authorization for ladmin is ON." CL_RESET "\n");
				else
					printf("  " CL_BOLD "Authorization for ladmin is OFF." CL_RESET "\n");
			} else {
				i = config_switch(param);
				/* parameter error */
				if (i < 0 || i > 1)
					printf("  " CL_RED "***ERROR: use 'ladmin|admin on|off' please." CL_RESET "\n");
				/* we want authorized ladmin connection */
				else if (i == 1) {
					if (admin_state != 0)
						printf("  " CL_BOLD "Authorization for ladmin is already ON." CL_RESET "\n");
					else {
						admin_state = 1;
						printf("  " CL_BOLD "Ladmin connection is now authorized." CL_RESET "\n");
						/* display warning if necessary */
						if (admin_pass[0] == '\0') {
							printf("  ***WARNING: Administrator password is void (admin_pass).\n");
						} else if (strcmp(admin_pass, "admin") == 0) {
							printf("  ***WARNING: You are using the default administrator password (admin_pass).\n");
							printf("              We highly recommend that you change it.\n");
						}
					}
				/* we want disable ladmin connection */
				} else {
					if (admin_state == 0)
						printf("  " CL_BOLD "Authorization for ladmin is already OFF." CL_RESET "\n");
					else {
						admin_state = 0;
						printf("  " CL_BOLD "Ladmin connection is now disabled." CL_RESET "\n");
						/* disconnect actual ladmin connection */
						j = 0;
						for(i = 0; i < fd_max; i++)
							if (session[i] && session[i]->func_parse == parse_admin) {
#ifdef __WIN32
								shutdown(i, SD_BOTH);
								closesocket(i);
#else
								close(i);
#endif
								delete_session(i);
								j++;
							}
						if (j > 0)
							printf("  Ladmin connections have been closed!\n");
					}
				}
			}

#ifdef TXT_ONLY
		} else if (strcasecmp("save", command) == 0) {
			save_account(0, 1);
			printf(CL_DARK_CYAN "Console: " CL_RESET CL_BOLD "Accounts database saved." CL_RESET "\n");
#endif /* TXT_ONLY */

		} else if (strcasecmp("?", command) == 0 ||
		           strcasecmp("h", command) == 0 ||
		           strcasecmp("help", command) == 0 ||
		           strcasecmp("aide", command) == 0) {
			printf(CL_DARK_GREEN"Help of commands:" CL_RESET "\n");
			printf("  '" CL_DARK_CYAN "?|h|help|aide" CL_RESET "': Display this help.\n");
			printf("  '" CL_DARK_CYAN "shutdown|exit|quit|end" CL_RESET "': Shutdown the server.\n");
			printf("  '" CL_DARK_CYAN "alive|status|uptime" CL_RESET "': Check the server.\n");
			printf("  '" CL_DARK_CYAN "ladmin|admin" CL_RESET "': Display authorization of ladmin.\n");
			printf("  '" CL_DARK_CYAN "ladmin|admin on|off" CL_RESET "': Enable/Disable authorization for ladmin connection.\n");
#ifdef TXT_ONLY
			printf("  '" CL_DARK_CYAN "save" CL_RESET "': Save accounts files.\n");
#endif /* TXT_ONLY */
			printf("  '" CL_DARK_CYAN "consoleoff" CL_RESET "': Disable console commands.\n");
			printf("  '" CL_DARK_CYAN "version" CL_RESET "': Display version of the server.\n");

		} else if (strcasecmp("version", command) == 0) {
			versionscreen();

		} else if (strcasecmp("consoleoff", command) == 0 ||
		           strcasecmp("console_off", command) == 0 ||
		           strcasecmp("consoloff", command) == 0 ||
		           strcasecmp("consol_off", command) == 0||
		           strcasecmp("console", command) == 0) {
			if (strcasecmp("console", command) == 0 && strcasecmp("off", param) != 0) {
				printf(CL_RED "ERROR: Unknown parameter." CL_RESET "\n");
			} else {
				printf(CL_DARK_CYAN "Console commands are now disabled." CL_RESET "\n");
				console_on = 0;
			}
		} else {
			printf(CL_DARK_CYAN "Console: " CL_RESET CL_RED "ERROR - Unknown command [%s]." CL_RESET "\n", command);
		}

	}

	return 0;
}

/*------------------------------------
  Reading general configuration file
------------------------------------*/
static void login_config_read(const char *cfgName) { // not inline, called too often
	char line[512], w1[512], w2[512];
	struct hostent * h = NULL;
	int i;
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

			/* General options */
			if (strcasecmp(w1, "login_port") == 0) {
				login_port = atoi(w2);
			} else if (strcasecmp(w1, "listen_ip") == 0) {
				memset(listen_ip, 0, sizeof(listen_ip));
				h = gethostbyname(w2);
				if (h != NULL) {
					sprintf(listen_ip, "%d.%d.%d.%d", (unsigned char)h->h_addr[0], (unsigned char)h->h_addr[1], (unsigned char)h->h_addr[2], (unsigned char)h->h_addr[3]);
				} else {
					strncpy(listen_ip, w2, 15); // 16 - NULL
				}
			} else if (strcasecmp(w1, "console") == 0) {
				console = config_switch(w2);
			} else if (strcasecmp(w1, "console_pass") == 0) {
				memset(console_pass, 0, sizeof(console_pass));
				strncpy(console_pass, w2, sizeof(console_pass) - 1);
			} else if (strcasecmp(w1, "strict_account_name_compare") == 0) {
				strict_account_name_compare = config_switch(w2);
			} else if (strcasecmp(w1, "use_MD5_passwords") == 0) {
				use_md5_passwds = config_switch(w2);
			} else if (strcasecmp(w1, "new_account") == 0) {
				new_account_flag = config_switch(w2);
			} else if (strcasecmp(w1, "unique_case_account_name_creation") == 0) {
				unique_case_account_name_creation = config_switch(w2);
			} else if (strcasecmp(w1, "level_new_account") == 0) {
				level_new_account = atoi(w2);
			} else if (strcasecmp(w1, "min_level_to_connect") == 0) {
				min_level_to_connect = atoi(w2);
			} else if (strcasecmp(w1, "client_version_to_connect") == 0) {
				client_version_to_connect = atoi(w2); /* Client version needed to connect: 0: any client, otherwise client version */
			} else if (strcasecmp(w1, "date_format") == 0) { /* note: never have more than 19 char for the date! */
				switch (atoi(w2)) {
				case 0:
					memset(date_format, 0, sizeof(date_format));
					strncpy(date_format, "%d-%m-%Y %H:%M:%S", sizeof(date_format) - 1); // 31-12-2004 23:59:59 // never set with more than 23 char + NULL
					break;
				case 1:
					memset(date_format, 0, sizeof(date_format));
					strncpy(date_format, "%m-%d-%Y %H:%M:%S", sizeof(date_format) - 1); // 12-31-2004 23:59:59 // never set with more than 23 char + NULL
					break;
				case 2:
					memset(date_format, 0, sizeof(date_format));
					strncpy(date_format, "%Y-%d-%m %H:%M:%S", sizeof(date_format) - 1); // 2004-31-12 23:59:59 // never set with more than 23 char + NULL
					break;
				case 3:
					memset(date_format, 0, sizeof(date_format));
					strncpy(date_format, "%Y-%m-%d %H:%M:%S", sizeof(date_format) - 1); // 2004-12-31 23:59:59 // never set with more than 23 char + NULL
					break;
				}
			} else if (strcasecmp(w1, "gm_pass") == 0) {
				memset(gm_pass, 0, sizeof(gm_pass));
				strncpy(gm_pass, w2, sizeof(gm_pass) - 1);
			} else if (strcasecmp(w1, "level_new_gm") == 0) {
				level_new_gm = atoi(w2);

			/* Logs options */
			} else if (strcasecmp(w1, "login_log_filename") == 0) {
				memset(login_log_filename, 0, sizeof(login_log_filename));
				strncpy(login_log_filename, w2, sizeof(login_log_filename) - 1);
			} else if (strcasecmp(w1, "log_login") == 0) {
				log_login = config_switch(w2);
			} else if (strcasecmp(w1, "log_file_date") == 0) {
				log_file_date = atoi(w2);
			} else if (strcasecmp(w1, "log_request_connection") == 0) {
				log_request_connection = config_switch(w2);
			} else if (strcasecmp(w1, "log_request_version") == 0) {
				log_request_version = config_switch(w2);
			} else if (strcasecmp(w1, "log_request_freya_version") == 0) {
				log_request_freya_version = config_switch(w2);
			} else if (strcasecmp(w1, "log_request_uptime") == 0) {
				log_request_uptime = config_switch(w2);

			/* Anti-freeze options */
			} else if (strcasecmp(w1, "anti_freeze_counter") == 0) {
				anti_freeze_counter = atoi(w2);
			} else if (strcasecmp(w1, "anti_freeze_interval") == 0) {
				anti_freeze_interval = atoi(w2);

			/* sstatus files options */
			} else if (strcasecmp(w1, "sstatus_txt_filename") == 0) {
				memset(sstatus_txt_filename, 0, sizeof(sstatus_txt_filename));
				strncpy(sstatus_txt_filename, w2, sizeof(sstatus_txt_filename) - 1);
			} else if (strcasecmp(w1, "sstatus_html_filename") == 0) {
				memset(sstatus_html_filename, 0, sizeof(sstatus_html_filename));
				strncpy(sstatus_html_filename, w2, sizeof(sstatus_html_filename) - 1);
			} else if (strcasecmp(w1, "sstatus_php_filename") == 0) {
				memset(sstatus_php_filename, 0, sizeof(sstatus_php_filename));
				strncpy(sstatus_php_filename, w2, sizeof(sstatus_php_filename) - 1);
			} else if (strcasecmp(w1, "sstatus_refresh_html") == 0) {
				sstatus_refresh_html = atoi(w2);
				if (sstatus_refresh_html < 5) /* send online file every 5 seconds to player is enough */
					sstatus_refresh_html = 5;
			} else if (strcasecmp(w1, "sstatus_txt_enable") == 0) {
				sstatus_txt_enable = atoi(w2); /* 0: no, 1 yes, 2: yes with ID of servers */
			} else if (strcasecmp(w1, "sstatus_html_enable") == 0) {
				sstatus_html_enable = atoi(w2); /* 0: no, 1 yes, 2: yes with ID of servers */
			} else if (strcasecmp(w1, "sstatus_php_enable") == 0) {
				sstatus_php_enable = config_switch(w2); /* 0: no, 1 yes */

			/* Lan support options */
			} else if (strcasecmp(w1, "lan_char_ip") == 0) { /* Read Char-Server Lan IP Address */
				memset(lan_char_ip, 0, sizeof(lan_char_ip));
				h = gethostbyname(w2);
				if (h != NULL) {
					sprintf(lan_char_ip, "%d.%d.%d.%d", (unsigned char)h->h_addr[0], (unsigned char)h->h_addr[1], (unsigned char)h->h_addr[2], (unsigned char)h->h_addr[3]);
				} else {
					strncpy(lan_char_ip, w2, 15); // 16 - NULL
				}
			} else if (strcasecmp(w1, "subnet") == 0) { /* Read Subnetwork */
				h = gethostbyname(w2);
				if (h != NULL) {
					for(i = 0; i < 4; i++)
						subnet[i] = (unsigned char)h->h_addr[i];
				} else {
					unsigned short a0, a1, a2, a3;
					if (sscanf(w2, "%hu.%hu.%hu.%hu", &a0, &a1, &a2, &a3) == 4 &&
					    a0 <= 255 && a1 <= 255 && a2 <= 255 && a3 <= 255) {
						subnet[0] = a0;
						subnet[1] = a1;
						subnet[2] = a2;
						subnet[3] = a3;
					}
				}
			} else if (strcasecmp(w1, "subnetmask") == 0) { /* Read Subnetwork Mask */
				h = gethostbyname(w2);
				if (h != NULL) {
					for(i = 0; i < 4; i++)
						subnetmask[i] = (unsigned char)h->h_addr[i];
				} else {
					unsigned short a0, a1, a2, a3;
					if (sscanf(w2, "%hu.%hu.%hu.%hu", &a0, &a1, &a2, &a3) == 4 &&
					    a0 <= 255 && a1 <= 255 && a2 <= 255 && a3 <= 255) {
						subnetmask[0] = a0;
						subnetmask[1] = a1;
						subnetmask[2] = a2;
						subnetmask[3] = a3;
					}
				}

			/* Char-servers connection security */
			} else if (strcasecmp(w1, "charallowip") == 0) {
				if (strcasecmp(w2, "clear") == 0) {
					FREE(access_char_allow);
					access_char_allownum = 0;
				} else {
					if (strcasecmp(w2, "all") == 0) {
						/* reset all previous values */
						FREE(access_char_allow);
						/* set to all */
						CALLOC(access_char_allow, char, ACO_STRSIZE);
						access_char_allownum = 1;
						//access_char_allow[0] = '\0';
					} else if (w2[0] && !(access_char_allownum == 1 && access_char_allow[0] == '\0')) { /* don't add IP if already 'all' */
						if (access_char_allow) {
							REALLOC(access_char_allow, char, (access_char_allownum + 1) * ACO_STRSIZE);
							memset(access_char_allow + (access_char_allownum * ACO_STRSIZE), 0, sizeof(char) * ACO_STRSIZE);
						} else {
							CALLOC(access_char_allow, char, ACO_STRSIZE);
						}
						strncpy(access_char_allow + (access_char_allownum++) * ACO_STRSIZE, w2, ACO_STRSIZE - 1); // 32 - NULL
						access_char_allow[access_char_allownum * ACO_STRSIZE - 1] = '\0';
					}
				}

			/* Network security */
			} else if (strcasecmp(w1, "check_ip_flag") == 0) {
				check_ip_flag = config_switch(w2);
			} else if (strcasecmp(w1, "check_authfifo_login2") == 0) {
				check_authfifo_login2 = config_switch(w2);
			} else if (strcasecmp(w1, "order") == 0) {
				access_order = atoi(w2);
				if (strcasecmp(w2, "deny,allow") == 0 ||
				    strcasecmp(w2, "deny, allow") == 0)
					access_order = ACO_DENY_ALLOW;
				else if (strcasecmp(w2, "allow,deny") == 0 ||
				         strcasecmp(w2, "allow, deny") == 0)
					access_order = ACO_ALLOW_DENY;
				else if (strcasecmp(w2, "mutual-failture") == 0 ||
				         strcasecmp(w2, "mutual-failure") == 0)
					access_order = ACO_MUTUAL_FAILTURE;
			} else if (strcasecmp(w1, "allow") == 0) {
				if (strcasecmp(w2, "clear") == 0) {
					FREE(access_allow);
					access_allownum = 0;
				} else {
					if (strcasecmp(w2, "all") == 0) {
						/* reset all previous values */
						FREE(access_allow);
						/* set to all */
						CALLOC(access_allow, char, ACO_STRSIZE);
						access_allownum = 1;
						//access_allow[0] = '\0';
					} else if (w2[0] && !(access_allownum == 1 && access_allow[0] == '\0')) { /* don't add IP if already 'all' */
						if (access_allow) {
							REALLOC(access_allow, char, (access_allownum + 1) * ACO_STRSIZE);
							memset(access_allow + (access_allownum * ACO_STRSIZE), 0, sizeof(char) * ACO_STRSIZE);
						} else {
							CALLOC(access_allow, char, ACO_STRSIZE);
						}
						strncpy(access_allow + (access_allownum++) * ACO_STRSIZE, w2, ACO_STRSIZE - 1); // 32 - NULL
						access_allow[access_allownum * ACO_STRSIZE - 1] = '\0';
					}
				}
			} else if (strcasecmp(w1, "deny") == 0) {
				if (strcasecmp(w2, "clear") == 0) {
					FREE(access_deny);
					access_denynum = 0;
				} else {
					if (strcasecmp(w2, "all") == 0) {
						/* reset all previous values */
						FREE(access_deny);
						/* set to all */
						CALLOC(access_deny, char, ACO_STRSIZE);
						access_denynum = 1;
						//access_deny[0] = '\0';
					} else if (w2[0] && !(access_denynum == 1 && access_deny[0] == '\0')) { /* don't add IP if already 'all' */
						if (access_deny) {
							REALLOC(access_deny, char, (access_denynum + 1) * ACO_STRSIZE);
							memset(access_deny + (access_denynum * ACO_STRSIZE), 0, sizeof(char) * ACO_STRSIZE);
						} else {
							CALLOC(access_deny, char, ACO_STRSIZE);
						}
						strncpy(access_deny + (access_denynum++) * ACO_STRSIZE, w2, ACO_STRSIZE - 1); // 32 - NULL
						access_deny[access_denynum * ACO_STRSIZE - 1] = '\0';
					}
				}

			/* dynamic password error ban */
			} else if (strcasecmp(w1, "dynamic_pass_failure_ban") == 0) {
				dynamic_pass_failure_ban = config_switch(w2);
			} else if (strcasecmp(w1, "dynamic_pass_failure_ban_time") == 0) {
				dynamic_pass_failure_ban_time = atoi(w2);
			} else if (strcasecmp(w1, "dynamic_pass_failure_ban_how_many") == 0) {
				dynamic_pass_failure_ban_how_many = atoi(w2);
			} else if (strcasecmp(w1, "dynamic_pass_failure_ban_how_long") == 0) {
				dynamic_pass_failure_ban_how_long = atoi(w2);
			} else if (strcasecmp(w1, "dynamic_pass_failure_save_in_account") == 0) {
				dynamic_pass_failure_save_in_account = config_switch(w2);

			/* Remote administration system (ladmin) */
			} else if (strcasecmp(w1, "admin_state") == 0) {
				admin_state = config_switch(w2);
			} else if (strcasecmp(w1, "admin_pass") == 0) {
				memset(admin_pass, 0, sizeof(admin_pass));
				strncpy(admin_pass, w2, sizeof(admin_pass) - 1);
			} else if (strcasecmp(w1, "ladminallowip") == 0) {
				if (strcasecmp(w2, "clear") == 0) {
					FREE(access_ladmin_allow);
					access_ladmin_allownum = 0;
				} else {
					if (strcasecmp(w2, "all") == 0) {
						/* reset all previous values */
						FREE(access_ladmin_allow);
						/* set to all */
						CALLOC(access_ladmin_allow, char, ACO_STRSIZE);
						access_ladmin_allownum = 1;
						//access_ladmin_allow[0] = '\0';
					} else if (w2[0] && !(access_ladmin_allownum == 1 && access_ladmin_allow[0] == '\0')) { /* don't add IP if already 'all' */
						if (access_ladmin_allow) {
							REALLOC(access_ladmin_allow, char, (access_ladmin_allownum + 1) * ACO_STRSIZE);
							memset(access_ladmin_allow + (access_ladmin_allownum * ACO_STRSIZE), 0, sizeof(char) * ACO_STRSIZE);
						} else {
							CALLOC(access_ladmin_allow, char, ACO_STRSIZE);
						}
						strncpy(access_ladmin_allow + (access_ladmin_allownum++) * ACO_STRSIZE, w2, ACO_STRSIZE - 1); // 32 - NULL
						access_ladmin_allow[access_ladmin_allownum * ACO_STRSIZE - 1] = '\0';
					}
				}
			} else if (strcasecmp(w1, "add_to_unlimited_account") == 0) {
				add_to_unlimited_account = config_switch(w2);
			} else if (strcasecmp(w1, "start_limited_time") == 0) {
				start_limited_time = atoi(w2);
			} else if (strcasecmp(w1, "ladmin_min_GM_level") == 0) {
				ladmin_min_GM_level = atoi(w2);

			/* Debug options */
			} else if (strcasecmp(w1, "save_unknown_packets") == 0) {
				save_unknown_packets = config_switch(w2);
			} else if (strcasecmp(w1, "login_log_unknown_packets_filename") == 0) {
				memset(login_log_unknown_packets_filename, 0, sizeof(login_log_unknown_packets_filename));
				strncpy(login_log_unknown_packets_filename, w2, sizeof(login_log_unknown_packets_filename) - 1);
			} else if (strcasecmp(w1, "display_parse_login") == 0) {
				display_parse_login = config_switch(w2);
			} else if (strcasecmp(w1, "display_parse_admin") == 0) {
				display_parse_admin = config_switch(w2);
			} else if (strcasecmp(w1, "display_parse_fromchar") == 0) {
				display_parse_fromchar = atoi(w2); /* 0: no, 1: yes (without packet 0x2714), 2: all packets */

			/* Addons loaded in login-server */
			} else if (strcasecmp(w1, "addon") == 0) {
				addons_load(w2, ADDONS_LOGIN);

			/* *** TXT *** CONFIGURATION */
#ifdef TXT_ONLY
			} else if (strcasecmp(w1, "account_filename") == 0) {
				memset(account_filename, 0, sizeof(account_filename));
				strncpy(account_filename, w2, sizeof(account_filename) - 1);
			} else if (strcasecmp(w1, "GM_account_filename") == 0) {
				memset(GM_account_filename, 0, sizeof(GM_account_filename));
				strncpy(GM_account_filename, w2, sizeof(GM_account_filename) - 1);
			} else if (strcasecmp(w1, "save_GM_level_with_accounts") == 0) {
				save_GM_level_with_accounts = config_switch(w2);
#endif /* TXT_ONLY */

			/* *** SQL *** CONFIGURATION */
#ifdef USE_SQL
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
#ifdef USE_MYSQL
			} else if (strcasecmp(w1, "optimize_table") == 0) {
				optimize_table = config_switch(w2);
#endif /* USE_MYSQL */
#endif /* USE_SQL */

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

/*--------------------------------------
  Displaying of configuration warnings
---------------------------------------*/
static inline void display_conf_warnings(void) {

/* General options */
	if (login_port < 1024 || login_port > 65535) {
		printf("***WARNING: Invalid value for login_port parameter -> set to 6900 (default).\n");
		login_port = 6900;
	}

	if (inet_addr(listen_ip) == INADDR_NONE) { // INADDR_NONE is not always -1
		printf("***WARNING: Invalid listen IP parameter -> set to 0.0.0.0 (default).\n");
		memset(listen_ip, 0, sizeof(listen_ip));
		strcpy(listen_ip, "0.0.0.0");
	}

	if (console) {
		if (console_pass[0] == '\0') {
			printf("***WARNING: The console commands password is void (console_pass).\n");
			printf("            We highly recommend that you set one password.\n");
		} else if (strcmp(console_pass, "consoleon") == 0) {
			printf("***WARNING: You are using the default console commands password (console_pass).\n");
			printf("            We highly recommend that you change it.\n");
		}
	}

/*	if (level_new_account < 0) {
		printf("***WARNING: value of level_new_account (%d) is invalid.\n", level_new_account);
		printf("            -> Using default value (0).\n");
		level_new_account = 0;
	} else*/ if (level_new_account > 99) {
		printf("***WARNING: value of level_new_account (%d) is invalid.\n", level_new_account);
		printf("            -> Using default value (0).\n");
		level_new_account = 0;
	}

/*	if (min_level_to_connect < 0) { // 0: all players, 1-99 at least gm level x
		printf("***WARNING: Invalid value for min_level_to_connect (%d) parameter\n", min_level_to_connect);
		printf("            -> set to 0 (any player).\n");
		min_level_to_connect = 0;
	} else*/ if (min_level_to_connect > 99) { /* 0: all players, 1-99 at least gm level x */
		printf("***WARNING: Invalid value for min_level_to_connect (%d) parameter\n", min_level_to_connect);
		printf("            -> set to 99 (only GM level 99).\n");
		min_level_to_connect = 99;
	}

	if (client_version_to_connect < 0) { /* Client version needed to connect: 0: any client, otherwise client version */
		printf("***WARNING: Invalid value for client_version_to_connect (%d) parameter\n", client_version_to_connect);
		printf("            -> set to 0 (any client version).\n");
		client_version_to_connect = 0;
	}

	if (gm_pass[0] == '\0') {
		printf("***WARNING: 'To GM become' password is void (gm_pass).\n");
		printf("            We highly recommend that you set one password.\n");
	} else if (strcmp(gm_pass, "gm") == 0) {
		printf("***WARNING: You are using the default GM password (gm_pass).\n");
		printf("            We highly recommend that you change it.\n");
	}

	if (level_new_gm > 99) {
		printf("***WARNING: Invalid value for level_new_gm parameter -> set to 60 (default).\n");
		level_new_gm = 60;
	}

/* Logs options */
	if (log_file_date > 4) {
		printf("***WARNING: Invalid value for log_file_date parameter -> set to 3 (default).\n");
		log_file_date = 3;
	}

/* Anti-freeze options */
	if (anti_freeze_counter < 2) {
		printf("***WARNING: Invalid value for anti_freeze_counter option-> set to 12 (default).\n");
		anti_freeze_counter = 12;
	}
	if (anti_freeze_interval < 0) {
		printf("***WARNING: Invalid value for anti_freeze_interval option->set to 10 (default).\n");
		anti_freeze_interval = 10;
	} else if (anti_freeze_interval != 0 && (anti_freeze_counter * anti_freeze_interval) < 6) {
		printf("***WARNING: Invalid values for anti_freeze_counter and anti_freeze_counter\n");
		printf("            parameters. Char-servers send number of users every 5 seconds.\n");
		printf("            Interval and counter must be upper than this value.\n");
		printf("            Set to default: anti_freeze_counter = 12.\n");
		anti_freeze_counter = 12;
	}

/* Lan support options */
	if (inet_addr(lan_char_ip) == INADDR_NONE) { // INADDR_NONE is not always -1
		printf("***WARNING: Invalid LAN IP parameter -> set to 127.0.0.1 (default).\n");
		memset(lan_char_ip, 0, sizeof(lan_char_ip));
		strcpy(lan_char_ip, "127.0.0.1");
	} else {
		unsigned int a0, a1, a2, a3;
		unsigned char p[4];
		if (sscanf(lan_char_ip, "%u.%u.%u.%u", &a0, &a1, &a2, &a3) < 4 || a0 > 255 || a1 > 255 || a2 > 255 || a3 > 255) // < 0 is always correct (unsigned)
			printf(CL_RED "***ERROR: Incorrect LAN IP of the char-server (a value is < 0 or > 255)." CL_RESET "\n");
		else {
			p[0] = a0; p[1] = a1; p[2] = a2; p[3] = a3;
			if (lan_ip_check(p) == 0)
				printf(CL_RED "***ERROR: LAN IP of the char-server doesn't belong to the specified Sub-network." CL_RESET "\n");
		}
	}

/* Char-servers connection security */

/* Network security */
	if (access_order == ACO_DENY_ALLOW) {
		if (access_denynum == 1 && access_deny[0] == '\0') {
			printf("***WARNING: The IP security order is 'deny,allow' (allow if not deny).\n");
			printf("            And you refuse ALL IP.\n");
		}
	} else if (access_order == ACO_ALLOW_DENY) {
		if (access_allownum == 0) {
			printf("***WARNING: The IP security order is 'allow,deny' (deny if not allow).\n");
			printf("            But, NO IP IS AUTHORIZED!\n");
		}
	} else { /* ACO_MUTUAL_FAILTURE */
		if (access_allownum == 0) {
			printf("***WARNING: The IP security order is 'mutual-failture'\n");
			printf("            (allow if in the allow list and not in the deny list).\n");
			printf("            But, NO IP IS AUTHORIZED!\n");
		} else if (access_denynum == 1 && access_deny[0] == '\0') {
			printf("***WARNING: The IP security order is mutual-failture\n");
			printf("            (allow if in the allow list and not in the deny list).\n");
			printf("            But, you refuse ALL IP!\n");
		}
	}

	if (dynamic_pass_failure_ban) {
		if (dynamic_pass_failure_ban_time < 1) {
			printf("***WARNING: Invalid value for dynamic_pass_failure_ban_time (%d) parameter\n", dynamic_pass_failure_ban_time);
			printf("            -> set to 60 (1 minute to look number of invalid passwords.\n");
			dynamic_pass_failure_ban_time = 60;
		}
		if (dynamic_pass_failure_ban_how_many < 1) {
			printf("***WARNING: Invalid value for dynamic_pass_failure_ban_how_many (%d) parameter\n", dynamic_pass_failure_ban_how_many);
			printf("            -> set to 3 (3 invalid passwords before to temporarily ban.\n");
			dynamic_pass_failure_ban_how_many = 3;
		}
		if (dynamic_pass_failure_ban_how_long < 1) {
			printf("***WARNING: Invalid value for dynamic_pass_failure_ban_how_long (%d) parameter\n", dynamic_pass_failure_ban_how_long);
			printf("            -> set to 300 (5 minutes of temporarily ban.\n");
			dynamic_pass_failure_ban_how_long = 300;
		}
	}

/* Remote administration system (ladmin) */
	if (admin_state) {
		if (admin_pass[0] == '\0') {
			printf("***WARNING: Administrator password is void (admin_pass).\n");
		} else if (strcmp(admin_pass, "admin") == 0) {
			printf("***WARNING: You are using the default administrator password (admin_pass).\n");
			printf("            We highly recommend that you change it.\n");
		}
	}

	if (start_limited_time < -1) { /* -1: create unlimited account, 0 or more: additionnal sec from now to create limited time */
		printf("***WARNING: Invalid value for start_limited_time parameter\n");
		printf("            -> set to -1 (new accounts are created with unlimited time).\n");
		start_limited_time = -1;
	}

	if (ladmin_min_GM_level < 1) { /* if 0, it's like ls ladmin command */
		printf("***WARNING: Invalid value for ladmin_min_GM_level (%d) parameter\n", ladmin_min_GM_level);
		printf("            -> set to 20 (mediators and upper accounts).\n");
		ladmin_min_GM_level = 20;
	} else if (ladmin_min_GM_level > 99) { /* minimum GM level of a account for listGM/lsGM ladmin command */
		printf("***WARNING: Invalid value for ladmin_min_GM_level (%d) parameter\n", ladmin_min_GM_level);
		printf("            -> set to 20 (mediators and upper accounts).\n");
		ladmin_min_GM_level = 20;
	}

/* Debug options */
if (display_parse_fromchar > 2) { /* 0: no, 1: yes (without packet 0x2714), 2: all packets */
		printf("***WARNING: Invalid value (%d) for display_parse_fromchar parameter\n", display_parse_fromchar);
		printf("            -> set to 2 (display all packets).\n");
		display_parse_fromchar = 2;
	}

/* Addons loaded in login-server */

/* *** TXT *** CONFIGURATION */
#ifdef TXT_ONLY
#endif /* TXT_ONLY */

/* *** SQL *** CONFIGURATION */
#ifdef USE_SQL
#ifdef USE_MYSQL
	if (db_mysql_server_port < 1024 || db_mysql_server_port > 65535) {
		printf("***WARNING: Invalid value for db_mysql_server_port parameter -> set to 3306 (default).\n");
		db_mysql_server_port = 3306;
	}
#endif /* USE_MYSQL */
#endif /* USE_SQL */

/* IMPORT FILES */

	return;
}

/*---------------------------------
  Save configuration in log file
---------------------------------*/
static inline void save_config_in_log(void) {
	int i;

	write_log(""); /* a newline in the log... */
#ifdef TXT_ONLY
	#ifdef SVN_REVISION
		if (SVN_REVISION >= 1) // in case of .svn directories have been deleted
			write_log("The login-server (v%1d.%1d.%1d %s %s, SVN rev. %d) is starting..." RETCODE, FREYA_MAJORVERSION, FREYA_MINORVERSION, FREYA_REVISION, "TXT", FREYA_STATE ? "beta" : "final", (int)SVN_REVISION);
		else
			write_log("The login-server (v%1d.%1d.%1d %s %s) is starting..." RETCODE, FREYA_MAJORVERSION, FREYA_MINORVERSION, FREYA_REVISION, "TXT", FREYA_STATE ? "beta" : "final");
	#else
		write_log("The login-server (v%1d.%1d.%1d %s %s) is starting..." RETCODE, FREYA_MAJORVERSION, FREYA_MINORVERSION, FREYA_REVISION, "TXT", FREYA_STATE ? "beta" : "final");
	#endif /* SVN_REVISION */
#else
	#ifdef SVN_REVISION
		if (SVN_REVISION >= 1) // in case of .svn directories have been deleted
			write_log("The login-server (v%1d.%1d.%1d %s %s, SVN rev. %d) is starting..." RETCODE, FREYA_MAJORVERSION, FREYA_MINORVERSION, FREYA_REVISION, "SQL", FREYA_STATE ? "beta" : "final", (int)SVN_REVISION);
		else
			write_log("The login-server (v%1d.%1d.%1d %s %s) is starting..." RETCODE, FREYA_MAJORVERSION, FREYA_MINORVERSION, FREYA_REVISION, "SQL", FREYA_STATE ? "beta" : "final");
	#else
		write_log("The login-server (v%1d.%1d.%1d %s %s) is starting..." RETCODE, FREYA_MAJORVERSION, FREYA_MINORVERSION, FREYA_REVISION, "SQL", FREYA_STATE ? "beta" : "final");
	#endif /* SVN_REVISION */
#endif /* USE_SQL */
	write_log("The configuration of the server is set:" RETCODE);

	/* General options */
	write_log("* General options *" RETCODE);
	write_log("- with port: %d." RETCODE, login_port);
	if (strcmp(listen_ip, "0.0.0.0") == 0)
		write_log("- to listen on any IP." RETCODE);
	else
		write_log("- to listen only on IP '%s'." RETCODE, listen_ip);
	if (console) {
		write_log("- with console commands enabled." RETCODE);
		if (console_pass[0] == '\0')
			write_log("- with a VOID console commands password (console_pass)." RETCODE);
		else if (strcmp(console_pass, "consoleon") == 0)
			write_log("- with the DEFAULT console commands password (console_pass)." RETCODE);
		else
			write_log("- with a console commands password (console_pass) of %d character%s." RETCODE, strlen(console_pass), (strlen(console_pass) > 1) ? "s" : "");
	} else
		write_log("- with console commands disabled." RETCODE);

	if (strict_account_name_compare == 0)
		write_log("- to accept any case inside account name when a player or the remote administrator ask for." RETCODE);
	else
		write_log("- to force players and remote administrator to use correct case in account name." RETCODE);
	if (use_md5_passwds == 0)
		write_log("- to save password in plain text." RETCODE);
	else
		write_log("- to save password with MD5 encrypting." RETCODE);
	if (new_account_flag != 0)
		write_log("- to ALLOW new accounts (with _F/_M)." RETCODE);
	else
		write_log("- to NOT ALLOW new accounts (with _F/_M)." RETCODE);
	if (unique_case_account_name_creation == 1)
		write_log("- to REFUSE creation of accounts with different case name." RETCODE);
	else
		write_log("- to ALLOW creation of accounts with different case name." RETCODE);
	write_log("- New accounts are created with level: %d." RETCODE, level_new_account);
	if (min_level_to_connect == 0) /* 0: all players, 1-99 at least gm level x */
		write_log("- with no minimum level for connection." RETCODE);
	else if (min_level_to_connect == 99)
		write_log("- to accept only GM with level 99." RETCODE);
	else
		write_log("- to accept only GM with level %d or more." RETCODE, min_level_to_connect);
	if (client_version_to_connect == 0) /* Client version needed to connect: 0: any client, otherwise client version */
		write_log("- to accept any version of client." RETCODE);
	else
		write_log("- to accept only version '%d' of client." RETCODE, client_version_to_connect);
	/* not necessary to log the 'date_format': it's displayed in the log file */
	if (level_new_gm == 0)
		write_log("- to refuse any creation of GM with @gm." RETCODE);
	else {
		write_log("- to create GM with level '%d' when @gm is used." RETCODE, level_new_gm);
		if (gm_pass[0] == '\0')
			write_log("- with a VOID 'To GM become' password (gm_pass)." RETCODE);
		else if (strcmp(gm_pass, "gm") == 0)
			write_log("- with the DEFAULT 'To GM become' password (gm_pass)." RETCODE);
		else
			write_log("- with a 'To GM become' password (gm_pass) of %d character%s." RETCODE, strlen(gm_pass), (strlen(gm_pass) > 1) ? "s" : "");
	}

	/* Logs options */
	if (log_login) {
		write_log("* Logs options *" RETCODE);
		/* not necessary to log the 'login_log_filename', we are inside */
		/* not necessary to log the 'log_login', we are inside */
		switch (log_file_date) {
		case 1:
			write_log("- to add the year to the log file name (example: log/login-2006.log)." RETCODE);
			break;
		case 2:
			write_log("- to add the month to the log file name (example: log/login-12.log)." RETCODE);
			break;
		case 3:
			write_log("- to add the year and the month to the log file name (example: log/login-2006-12.log)." RETCODE);
			break;
		case 4:
			write_log("- to add the date to the log file name (example: log/login-2006-12-25.log)." RETCODE);
			break;
		default: // case 0:
			write_log("- to not change the log file name with a date." RETCODE);
			break;
		}
		if (log_request_connection)
			write_log("- to log 'Request for connection' message (packet 0x64/0x1dd/0x27c)." RETCODE);
		else
			write_log("- to NOT log 'Request for connection' message (packet 0x64/0x1dd/0x27c)." RETCODE);
		if (log_request_version)
			write_log("- to log 'Request of the server version' (athena version) message (packet 0x7530)." RETCODE);
		else
			write_log("- to NOT log 'Request of the server version' (athena version) message (packet 0x7530)." RETCODE);
		if (log_request_freya_version)
			write_log("- to log 'Request of the server version' (freya version) message (packet 0x7535)." RETCODE);
		else
			write_log("- to NOT log 'Request of the server version' (freya version) message (packet 0x7535)." RETCODE);
		if (log_request_uptime)
			write_log("- to log 'Request of the server uptime' message (packet 0x7533)." RETCODE);
		else
			write_log("- to NOT log 'Request of the server uptime' message (packet 0x7533)." RETCODE);
	}

	/* Anti-freeze options */
	write_log("* Anti-freeze options *" RETCODE);
	if (anti_freeze_interval == 0)
		write_log("- with no anti-freeze system." RETCODE);
	else {
		write_log("- to check char-servers every %d second%s." RETCODE, anti_freeze_interval, (anti_freeze_interval > 1) ? "s" : "");
		write_log("- to disconnect char-servers after %d invalid successive check%s." RETCODE, anti_freeze_counter, (anti_freeze_counter > 1) ? "s" : "");
	}

	/* sstatus files options */
	write_log("* sstatus files options *" RETCODE);
	if (sstatus_txt_enable == 0)
		write_log("- with non sstatus TXT file." RETCODE);
	else if (sstatus_txt_enable == 1)
		write_log("- with the sstatus (without ID of servers) TXT file name: '%s'." RETCODE, sstatus_txt_filename);
	else
		write_log("- with the sstatus TXT (with ID of servers) file name: '%s'." RETCODE, sstatus_txt_filename);
	if (sstatus_html_enable == 0)
		write_log("- with non sstatus HTML file." RETCODE);
	else {
		if (sstatus_html_enable == 1)
			write_log("- with the sstatus (without ID of servers) HTML file name: '%s'." RETCODE, sstatus_html_filename);
		else
			write_log("- with the sstatus HTML (with ID of servers) file name: '%s'." RETCODE, sstatus_html_filename);
		write_log("- with a refresh time of %d seconds for the sstatus HTML file name'." RETCODE, sstatus_refresh_html);
	}
	if (sstatus_php_enable == 0)
		write_log("- with non sstatus PHP file." RETCODE);
	else
		write_log("- with the sstatus PHP file name: '%s'." RETCODE, sstatus_php_filename);

	/* Lan support options */
	write_log("* Lan support options *" RETCODE);
	write_log("- with LAN IP of char-server: %s." RETCODE, lan_char_ip);
	write_log("- with sub-network of the char-server: %hd.%hd.%hd.%hd." RETCODE, subnet[0], subnet[1], subnet[2], subnet[3]);
	write_log("- with sub-network mask of the char-server: %hd.%hd.%hd.%hd." RETCODE, subnetmask[0], subnetmask[1], subnetmask[2], subnetmask[3]);

	/* Char-servers connection security */
	if (access_char_allownum == 0 || (access_char_allownum == 1 && access_char_allow[0] == '\0')) {
		write_log("- to accept any IP for all char-servers connetions" RETCODE);
	} else {
		write_log("- to accept following IP for all char-servers connetions:" RETCODE);
		for(i = 0; i < access_char_allownum; i++)
			write_log("  %s" RETCODE, (char *)(access_char_allow + i * ACO_STRSIZE));
	}

	/* Network security */
	write_log("* Network security *" RETCODE);
	if (check_ip_flag)
		write_log("- with control of players IP between login-server and char-server." RETCODE);
	else
		write_log("- to not check players IP between login-server and char-server." RETCODE);
	if (check_authfifo_login2)
		write_log("- with control of players login2 between login-server and char-server." RETCODE);
	else
		write_log("- to not check players login2 between login-server and char-server." RETCODE);
	if (access_order == ACO_DENY_ALLOW) {
		if (access_denynum == 0) {
			write_log("- with the IP security order: 'deny,allow' (allow if not deny). You refuse no IP." RETCODE);
		} else if (access_denynum == 1 && access_deny[0] == '\0') {
			write_log("- with the IP security order: 'deny,allow' (allow if not deny). You refuse ALL IP! -> No connection is possible." RETCODE);
		} else {
			write_log("- with the IP security order: 'deny,allow' (allow if not deny). Refused IP are:" RETCODE);
			for(i = 0; i < access_denynum; i++)
				write_log("  %s" RETCODE, (char *)(access_deny + i * ACO_STRSIZE));
		}
	} else if (access_order == ACO_ALLOW_DENY) {
		if (access_allownum == 0) {
			write_log("- with the IP security order: 'allow,deny' (deny if not allow). But, NO IP IS AUTHORIZED! -> No connection is possible." RETCODE);
		} else if (access_allownum == 1 && access_allow[0] == '\0') {
			write_log("- with the IP security order: 'allow,deny' (deny if not allow). You authorize ALL IP." RETCODE);
		} else {
			write_log("- with the IP security order: 'allow,deny' (deny if not allow). Authorized IP are:" RETCODE);
			for(i = 0; i < access_allownum; i++)
				write_log("  %s" RETCODE, (char *)(access_allow + i * ACO_STRSIZE));
		}
	} else { /* ACO_MUTUAL_FAILTURE */
		write_log("- with the IP security order: 'mutual-failture' (allow if in the allow list and not in the deny list)." RETCODE);
		if (access_allownum == 0) {
			write_log("  But, NO IP IS AUTHORIZED! -> No connection is possible." RETCODE);
		} else if (access_denynum == 1 && access_deny[0] == '\0') {
			write_log("  But, you refuse ALL IP! -> No connection is possible." RETCODE);
		} else {
			if (access_allownum == 1 && access_allow[0] == '\0') {
				write_log("  You authorize ALL IP." RETCODE);
			} else {
				write_log("  Authorized IP are:" RETCODE);
				for(i = 0; i < access_allownum; i++)
					write_log("    %s" RETCODE, (char *)(access_allow + i * ACO_STRSIZE));
			}
			write_log("  Refused IP are:" RETCODE);
			for(i = 0; i < access_denynum; i++)
				write_log("    %s" RETCODE, (char *)(access_deny + i * ACO_STRSIZE));
		}
	}

	/* dynamic password error ban */
	if (dynamic_pass_failure_ban == 0)
		write_log("- with NO dynamic password error ban." RETCODE);
	else {
		write_log("- with a dynamic password error ban:" RETCODE);
		write_log("  After %d invalid password%s in %d second%s" RETCODE, dynamic_pass_failure_ban_how_many, (dynamic_pass_failure_ban_how_many > 1) ? "s" : "", dynamic_pass_failure_ban_time, (dynamic_pass_failure_ban_time > 1) ? "s" : "");
		write_log("  IP is banned for %d second%s" RETCODE, dynamic_pass_failure_ban_how_long, (dynamic_pass_failure_ban_how_long > 1) ? "s" : "");
		if (dynamic_pass_failure_save_in_account)
			write_log("  Information about dynamic password ban are saved in the memo of the last concerned account" RETCODE);
		else
			write_log("  Information about dynamic password ban are NOT saved in any memo" RETCODE);
	}

	/* Remote administration system (ladmin) */
	write_log("* Remote administration system (ladmin) *" RETCODE);
	if (admin_state == 0)
		write_log("- with no remote administration." RETCODE);
	else if (admin_pass[0] == '\0')
		write_log("- with a remote administration with a VOID password." RETCODE);
	else if (strcmp(admin_pass, "admin") == 0)
		write_log("- with a remote administration with the DEFAULT password." RETCODE);
	else
		write_log("- with a remote administration with the password of %d character%s." RETCODE, strlen(admin_pass), (strlen(admin_pass) > 1) ? "s" : "");
	if (access_ladmin_allownum == 0 || (access_ladmin_allownum == 1 && access_ladmin_allow[0] == '\0')) {
		write_log("- to accept any IP for remote administration" RETCODE);
	} else {
		write_log("- to accept following IP for remote administration:" RETCODE);
		for(i = 0; i < access_ladmin_allownum; i++)
			write_log("  %s" RETCODE, (char *)(access_ladmin_allow + i * ACO_STRSIZE));
	}
	if (add_to_unlimited_account)
		write_log("- to authorize adjustment (with timeadd ladmin) on an unlimited account." RETCODE);
	else
		write_log("- to refuse adjustment (with timeadd ladmin) on an unlimited account. You must use timeset (ladmin command) before." RETCODE);
	if (start_limited_time < 0)
		write_log("- to create new accounts with an unlimited time." RETCODE);
	else if (start_limited_time == 0)
		write_log("- to create new accounts with a limited time: time of creation." RETCODE);
	else
		write_log("- to create new accounts with a limited time: time of creation + %d second(s)." RETCODE, start_limited_time);
	if (ladmin_min_GM_level == 99)
		write_log("- listGM/lsGM ladmin command will display GM accounts of level 99 (only)." RETCODE);
	else
		write_log("- listGM/lsGM ladmin command will display GM accounts from level %d to 99." RETCODE, ladmin_min_GM_level);

	/* Debug options */
	write_log("* Debug options *" RETCODE);
	if (save_unknown_packets)
		write_log("- to SAVE all unkown packets." RETCODE);
	else
		write_log("- to SAVE only unkown packets sending by a char-server or a remote administration." RETCODE);
	write_log("- with the unknown packets file name: '%s'." RETCODE, login_log_unknown_packets_filename);
	if (display_parse_login)
		write_log("- to display normal parse packets on console." RETCODE);
	else
		write_log("- to NOT display normal parse packets on console." RETCODE);
	if (display_parse_admin)
		write_log("- to display administration parse packets on console." RETCODE);
	else
		write_log("- to NOT display administration parse packets on console." RETCODE);
	if (display_parse_fromchar == 2)
		write_log("- to display char-server parse packets on console." RETCODE);
	else if (display_parse_fromchar == 1)
		write_log("- to NOT display char-server parse packets on console (without packet 0x2714)." RETCODE);
	else
		write_log("- to NOT display char-server parse packets on console." RETCODE);

	/* Addons loaded in login-server */

	/* *** TXT *** CONFIGURATION */
#ifdef TXT_ONLY
	write_log("* *** TXT *** CONFIGURATION *" RETCODE);
	write_log("- with the accounts file name: '%s'." RETCODE, account_filename);
	if (save_GM_level_with_accounts) {
		write_log("- to modify the GM levels at start with the file: '%s'." RETCODE, GM_account_filename);
		write_log("- to save the GM levels in the accounts file." RETCODE);
	} else
		write_log("- to save the GM levels in a specifical file: '%s'." RETCODE, GM_account_filename);
#endif /* TXT_ONLY */

	/* *** SQL *** CONFIGURATION */
#ifdef USE_SQL
	write_log("* *** SQL *** CONFIGURATION *" RETCODE);
	write_log("- to use login database: '%s'." RETCODE, login_db);
#ifdef USE_MYSQL
	write_log("- to use MYSQL server on IP: '%s'." RETCODE, db_mysql_server_ip);
	write_log("- to use MYSQL server on port: '%d'." RETCODE, db_mysql_server_port);
	write_log("- to use MYSQL server with id: '%s'." RETCODE, db_mysql_server_id);
	write_log("- to use MYSQL server with a password of %d character%s." RETCODE, strlen(db_mysql_server_pw), (strlen(db_mysql_server_pw) > 1) ? "s" : "");
	write_log("- to use MYSQL server with db: '%s'." RETCODE, db_mysql_server_db);
#endif /* USE_MYSQL */
	write_log("- to use name field of account id: '%s'." RETCODE, login_db_account_id);
	write_log("- to use name field of account name: '%s'." RETCODE, login_db_userid);
	write_log("- to use name field of account password: '%s'." RETCODE, login_db_user_pass);
	write_log("- to use name field of account level: '%s'." RETCODE, login_db_level);
#ifdef USE_MYSQL
	if (optimize_table)
		write_log("- to optimize tables at start." RETCODE);
	else
		write_log("- to NOT optimize tables at start." RETCODE);
#endif /* USE_MYSQL */
#endif /* USE_SQL */

	return;
}

/*---------------------------------------
  Function called at exit of the server
---------------------------------------*/
void do_final(void) {
	int i, fd;

	printf("Terminating...\n");

	/* send all packets not sended */
	flush_fifos();

	for (i = 0; i < MAX_SERVERS; i++) { // max number of char-servers (and account_id values: 0 to max-1)
		if ((fd = server_fd[i]) >= 0) {
			server_fd[i] = -1;
			memset(&server[i], 0, sizeof(struct mmo_char_server));
#ifdef __WIN32
			if (fd > 0) { // not console
				shutdown(fd, SD_BOTH);
				closesocket(fd);
			}
#else
			close(fd);
#endif
			delete_session(fd);
		}
	}

#ifdef TXT_ONLY
	save_account(0, 1);
#endif /* TXT_ONLY */

	create_sstatus_files();
#ifdef USE_SQL
	/* delete all server status */
	sql_request("DELETE FROM `sstatus`");

	sql_close();
#endif /* USE_SQL */

	if (auth_dat != NULL) {
		for (i = 0; i < auth_num; i++) {
			FREE(auth_dat[i].memo);
#ifdef TXT_ONLY
			FREE(auth_dat[i].account_reg2);
#endif // TXT_ONLY
		}
		FREE(auth_dat);
		auth_num = 0;
		auth_max = 0;
	}

	/* free online database */
	FREE(online_db);
	online_num = 0;
	online_max = 0;

	FREE(access_allow);
	access_allownum = 0;
	FREE(access_deny);
	access_denynum = 0;
	FREE(access_char_allow);
	access_char_allownum = 0;
	FREE(access_ladmin_allow);
	access_ladmin_allownum = 0;

	/* free the ban list */
	ban_list_max = 0;
	FREE(ban_list);
	pass_failure_list_max = 0;
	FREE(pass_failure_list);

	if (login_fd != -1) {
#ifdef __WIN32
		shutdown(login_fd, SD_BOTH);
		closesocket(login_fd);
#else
		close(login_fd);
#endif
		delete_session(login_fd);
		login_fd = -1;
	}

	/* restore console parameters */
	term_input_disable();

	for(fd = 0; fd < fd_max; fd++)
		if (session[fd]) {
#ifdef __WIN32
			if (fd > 0) { // not console
				shutdown(fd, SD_BOTH);
				closesocket(fd);
			}
#else
			close(fd);
#endif
			delete_session(fd);
		}

#ifdef __WIN32
	// close windows sockets
	WSACleanup();
#endif /* __WIN32 */

	write_log("----End of login-server (normal end with closing of all files)." RETCODE);

	close_log();

	printf("Finished.\n");
}

/*-------------------------------------------
  Initialisation of configuration variables
--------------------------------------------*/
static inline void init_conf_variables(void) {
	int i;

	/* General options */
	login_port = 6900;
	memset(listen_ip, 0, sizeof(listen_ip));
	strcpy(listen_ip, "0.0.0.0");
	console = 0; /* off */
	memset(console_pass, 0, sizeof(console_pass));
	strcpy(console_pass, "consoleon");
	strict_account_name_compare = 1; /* yes */
	use_md5_passwds = 0; /* no */
	new_account_flag = 1; /* yes */
	unique_case_account_name_creation = 1; /* yes */
	level_new_account = 0; /* level 0 */
	min_level_to_connect = 0;
	client_version_to_connect = 0; /* Client version needed to connect: 0: any client, otherwise client version */
	memset(date_format, 0, sizeof(date_format));
	strcpy(date_format, "%Y-%m-%d %H:%M:%S"); // 2004-12-31 23:59:59 // never set with more than 23 char + NULL
	memset(gm_pass, 0, sizeof(gm_pass));
	strcpy(gm_pass, "gm");
	level_new_gm = 60;

	/* Logs options */
	memset(login_log_filename, 0, sizeof(login_log_filename));
	strcpy(login_log_filename, "log/login.log");
	log_login = 1; /* yes */
	log_file_date = 3; /* year + month (example: log/login-2006-12.log) */
	log_request_connection = 1; /* yes */
	log_request_version = 0; /* no */
	log_request_freya_version = 0; /* no */
	log_request_uptime = 0; /* no */

	/* Anti-freeze options */
	anti_freeze_counter = 12;
	anti_freeze_interval = 0;

	/* sstatus files options */
	memset(sstatus_txt_filename, 0, sizeof(sstatus_txt_filename));
	strcpy(sstatus_txt_filename, "sstatus.txt");
	memset(sstatus_html_filename, 0, sizeof(sstatus_html_filename));
	strcpy(sstatus_html_filename, "sstatus.html");
	memset(sstatus_php_filename, 0, sizeof(sstatus_php_filename));
	strcpy(sstatus_php_filename, "sstatus.php");
	sstatus_refresh_html = 20;
	sstatus_txt_enable = 0; /* 0: no, 1 yes, 2: yes with ID of servers */
	sstatus_html_enable = 1; /* 0: no, 1 yes, 2: yes with ID of servers */
	sstatus_php_enable = 0; /* 0: no, 1 yes */

	/* Char-servers connection security */
	access_char_allow = NULL;
	access_char_allownum = 0;

	/* Lan support options */
	memset(lan_char_ip, 0, sizeof(lan_char_ip));
	strcpy(lan_char_ip, "127.0.0.1");
	subnet[0] = 127;
	subnet[1] = 0;
	subnet[2] = 0;
	subnet[3] = 1;
	for(i = 0; i < 4; i++)
		subnetmask[i] = 255;

	/* Network security */
	check_ip_flag = 1; /* yes */
	check_authfifo_login2 = 1; /* yes */
	access_order = ACO_DENY_ALLOW;
	access_allow = NULL;
	access_allownum = 0;
	access_deny = NULL;
	access_denynum = 0;

	/* dynamic password error ban */
	dynamic_pass_failure_ban = 1; /* yes */
	dynamic_pass_failure_ban_time = 60;
	dynamic_pass_failure_ban_how_many = 3;
	dynamic_pass_failure_ban_how_long = 300;
	dynamic_pass_failure_save_in_account = 0; /* 0: no, 1 yes */

	/* Remote administration system (ladmin) */
	admin_state = 0; /* off */
	memset(admin_pass, 0, sizeof(admin_pass));
	strcpy(admin_pass, "admin");
	access_ladmin_allow = NULL;
	access_ladmin_allownum = 0;
	add_to_unlimited_account = 0; /* no */
	start_limited_time = -1;
	ladmin_min_GM_level = 20; /* minimum GM level of a account for listGM/lsGM ladmin command */

	/* Debug options */
	save_unknown_packets = 0; /* no */
	memset(login_log_unknown_packets_filename, 0, sizeof(login_log_unknown_packets_filename));
	strcpy(login_log_unknown_packets_filename, "log/login_unknown_packets.log");
	display_parse_login = 0; /* no */
	display_parse_admin = 0; /* no */
	display_parse_fromchar = 0; /* 0: no, 1: yes (without packet 0x2714), 2: all packets */

	/* Addons loaded in login-server */

	/* *** TXT *** CONFIGURATION */
#ifdef TXT_ONLY
	memset(account_filename, 0, sizeof(account_filename));
	strcpy(account_filename, "save/account.txt");
	memset(GM_account_filename, 0, sizeof(GM_account_filename));
	strcpy(GM_account_filename, "conf/GM_account.txt");
	save_GM_level_with_accounts = 1; /* yes */
#endif /* TXT_ONLY */

	/* *** SQL *** CONFIGURATION */
#ifdef USE_SQL
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
#ifdef USE_MYSQL
	optimize_table = 0;
#endif /* USE_MYSQL */
#endif /* USE_SQL */

	/* IMPORT FILES */

	return;
}

/*--------------------------------
  Main function of this software
--------------------------------*/
void do_init(const int argc, char **argv) {
	int i;

	printf("The login-server is starting...\n");

	/* Init variables */
	for(i = 0; i < AUTH_FIFO_SIZE; i++)
		auth_fifo[i].delflag = 1; // 0: accepted, 1: not valid/not init
	for(i = 0; i < MAX_SERVERS; i++) // max number of char-servers (and account_id values: 0 to max-1)
		server_fd[i] = -1;
	login_fd = -1;

	/* must be init here for the atexit function */
	access_allow = NULL;
	access_deny = NULL;
	access_char_allow = NULL;
	access_ladmin_allow = NULL;

	/* init the ban list */
	pass_failure_list_max = 256;
	CALLOC(pass_failure_list, struct pass_failure_list, 256);
	ban_list_max = 256;
	CALLOC(ban_list, struct ban_list, 256);

	/* Define final function */
	atexit(do_final); /* clean up CALLOC of configuration if necessary */
/*	set_termfunc(do_final); when called, after exit is done, and atexit repeat same thing. so removed */

	/* read configuration */
	printf("---Start reading of Login Server configuration file (%s).\n", (argc > 1) ? argv[1] : LOGIN_CONF_NAME);
	init_conf_variables(); /* init before to read file to avoid recursiv calls */
	login_config_read((argc > 1) ? argv[1] : LOGIN_CONF_NAME);
	display_conf_warnings(); /* not in login_config_read, because we can use 'import' option, and display same message twice or more */
	save_config_in_log(); /* not before, because log file name can be changed */
	printf("---End reading of Login Server configuration file.\n");

	// initialise calculation of lan_char_ip_addr (check before in display_conf_warnings() function)
	lan_char_ip_addr = inet_addr(lan_char_ip); // to speed up after

	/* if we doesn't save logs */
	if (!log_login)
		printf("---Warning: Log file system is disabled.\n");

	/* Initialise random */
	srand(time(NULL));

	/* set default parser as parse_login function */
	set_defaultparse(parse_login);

	/* init db */
#ifdef TXT_ONLY
	GM_level_need_save_flag = 0; /* not need to save GM level file */
#endif /* TXT_ONLY */
	init_db();

	/* SQL UPDATE */
#ifdef USE_SQL
#ifdef USE_MYSQL
	// create `account_reg2_db` table if it not exists
	sql_request("SHOW TABLES");
	i = 0; // flag
	while (sql_get_row()) {
		if (strcmp(sql_get_string(0), "account_reg2_db") == 0) {
			i = 1;
			break;
		}
	}
	// create `account_reg2_db` table is not exist
	if (i == 0) {
		printf("The login-server has created the `account_reg2_db` SQL table.\n");
		sql_request("CREATE TABLE IF NOT EXISTS `account_reg2_db` ("
		            "`account_id` int(11) NOT NULL default '0',"
		            "`str` varchar(32) NOT NULL default '',"
		            "`value` int(11) NOT NULL default '0',"
		            "PRIMARY KEY (`account_id`, `str`)"
		            ") TYPE = MyISAM");
	}
#endif /* USE_MYSQL */
#endif /* USE_SQL */

	/* SQL MAINTENANCE */
#ifdef USE_SQL
#ifdef USE_MYSQL
	// execute optimize table if user has configured to do
	if (optimize_table) {
		sql_request("OPTIMIZE TABLE `sstatus`, `account_reg2_db`, `%s`", login_db);
		printf("SQL tables (`sstatus`, `account_reg2_db` and `%s`) have been optimized.\n", login_db);
	}
#endif /* USE_MYSQL */
#endif /* USE_SQL */

	/* update sstatus files and db */
	create_sstatus_files();
#ifdef USE_SQL
	/* delete all server status */
	sql_request("DELETE FROM `sstatus`");
#endif /* USE_SQL */

#ifdef DYNAMIC_LINKING
	addons_enable_all();
#endif

	/* server port open & binding */
	login_fd = make_listen_port(login_port);
	if (strcmp(listen_ip, "0.0.0.0") == 0) {
		write_log("The login-server is ready (listening on the port %d - from any ip)." RETCODE, login_port);
		printf("The login-server is " CL_GREEN "ready" CL_RESET " (listening on the port " CL_WHITE "%d" CL_RESET " - from any ip).\n", login_port);
	} else {
		write_log("The login-server is ready (listening on %s:%d)." RETCODE, listen_ip, login_port);
		printf("The login-server is " CL_GREEN "ready" CL_RESET " (listening on " CL_WHITE "%s:%d" CL_RESET ").\n", listen_ip, login_port);
	}

	add_timer_func_list(char_anti_freeze_system, "char_anti_freeze_system");
	if (anti_freeze_interval == 0)
		i = add_timer_interval(gettick_cache + 6000, char_anti_freeze_system, 0, 0, 6000); /* every 6 sec (users are sended every 5 sec) */
	else
		i = add_timer_interval(gettick_cache + anti_freeze_interval * 1000, char_anti_freeze_system, 0, 0, anti_freeze_interval * 1000); /* every 10 sec (users are sended every 5 sec) */
#ifdef TXT_ONLY
	add_timer_func_list(check_account_sync, "check_account_sync");
	i = add_timer_interval(gettick_cache + 60000, check_account_sync, 0, 0, 60000); /* every minute we check if we must save accounts file (only if necessary to save) */
#endif /* TXT_ONLY */

	/* console */
	if (console) {
		start_console(parse_console);
		if (term_input_status == 0) {
			printf("Sorry, but console commands can not be initialized -> disabled.\n\n");
			console = 0;
		} else {
			printf("Console commands are enabled. Type " CL_BOLD "help" CL_RESET " to have a little help.\n\n");
		}
	} else
		printf(CL_DARK_CYAN "Console commands are OFF/disactivated. You can not enter any console commands." CL_RESET "\n\n");

	return;
}

