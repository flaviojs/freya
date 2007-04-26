// Copyright (c) Freya Development Team - Licensed under GNU GPL
// For more information, see License.txt in the main folder

/* Include of configuration script */
#include <config.h>

/* Include of dependances */
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#ifndef __WIN32
/* gettimeofday for linux */
#include <sys/time.h>
#include <time.h>
#endif
#include <ctype.h> // isprint

/* Includes of software */
#include "../common/socket.h"
#include "../common/core.h"
#include "../common/timer.h" /* gettimeofday for win32 */
#include <mmo.h>
#include "../common/version.h"
#include "../common/lock.h"
#include "../common/malloc.h"
#include "../common/md5calc.h"
#include "login.h"
#include "lutils.h"

#ifdef USE_SQL
#ifdef USE_MYSQL
#include <../common/db_mysql.h>
#endif /* USE_MYSQL */
#endif /* USE_SQL */

int display_parse_admin = 0; /* 0: no, 1: yes */

/*----------------------------------------
  ladmin function:
  0x7530 - Request of the server version
  0x7535 - Request of the server version (freya version)
----------------------------------------*/
static void ladmin_packet_version(const int fd, const char* ip) {
	write_log("'ladmin': Request of the server version (ip: %s)" RETCODE, ip);

	if (RFIFOW(fd,0) == 0x7530) {
		WPACKETW(0) = 0x7531;
		WPACKETB(2) = FREYA_MAJORVERSION;
		WPACKETB(3) = FREYA_MINORVERSION;
		WPACKETB(4) = FREYA_REVISION;
		WPACKETB(5) = FREYA_STATE;
		WPACKETB(6) = 0;
		WPACKETB(7) = FREYA_LOGINVERSION;
		WPACKETW(8) = 0;
		SENDPACKET(fd, 10);
	} else {
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
	}

	return;
}

/*---------------------------------------
  ladmin function:
  0x7532 - Request of end of connection
---------------------------------------*/
static void ladmin_end_of_connection(const int fd, const char* ip) {
	write_log("'ladmin': Request of end of connection (ip: %s)" RETCODE, ip);

	session[fd]->eof = 1;

	return;
}

/*---------------------------------------
  ladmin function:
  0x7533 - Request of the server uptime
---------------------------------------*/
static void ladmin_packet_uptime(const int fd, const char* ip) {
	int i, j;

	write_log("'ladmin': Request of the server uptime (ip: %s)" RETCODE, ip);

	j = 0;
	for(i = 0; i < MAX_SERVERS; i++)
		if (server_fd[i] >= 0)
			j = j + server[i].users;

	WPACKETW(0) = 0x7534;
	WPACKETL(2) = time(NULL) - start_time;
	WPACKETW(6) = j;
	SENDPACKET(fd, 8);

	return;
}

/*---------------------------------------------
  ladmin function:
  0x791f - Check to know if ready (no answer)
---------------------------------------------*/
static void ladmin_ready(const int fd, const char* ip) {
	write_log("'ladmin': Check to know if ready (ip: %s)" RETCODE, ip);

	return;
}

/*--------------------------------------
  ladmin function:
  0x7920 - Request of an accounts list
--------------------------------------*/
static void ladmin_list_of_accounts(const int fd, const char* ip) {
	int start, end;
	unsigned short len;
	unsigned char type, online = 0; // init online to avoid an incorrect warning of some compilators
#ifdef TXT_ONLY
	int i, j;
	int account_id; /* speed up */
	int *id;
#endif /* TXT_ONLY */
#ifdef USE_SQL
	int end2;
#endif /* USE_SQL */

	/* get parameters */
	start = RFIFOL(fd,2);
	end = RFIFOL(fd,6);
	type = RFIFOB(fd,10); // 0: any accounts, 1: gm only, 3: accounts with state or bannished, 4: accounts without state and not bannished, 5: online accounts
	if (start < 0)
		start = 0;
	if (end > END_ACCOUNT_NUM || end < start || end <= 0)
		end = END_ACCOUNT_NUM;
	write_log("'ladmin': Request of an accounts list (from %d to %d, ip: %s)" RETCODE, start, end, ip);

#ifdef TXT_ONLY
	CALLOC(id, int, auth_num);
	/* Sort before send */
	for(i = 0; i < auth_num; i++)
		id[i] = i;
	if (auth_num > 1)
		accounts_speed_sorting(id, 0, auth_num - 1);

	/* answer preparation */
	WPACKETW(0) = 0x7921;
	WPACKETB(4) = type;
	len = 5;

	/* Sending accounts information */
	for(i = 0; i < auth_num && len < 30000; i++) {
		j = id[i]; /* use sorted index */
		account_id = auth_dat[j].account_id;
		if (account_id >= start && account_id <= end &&
		    (type == 0 || // 0: any accounts, 1: gm only, 3: accounts with state or bannished, 4: accounts without state and not bannished, 5: online accounts
		     (type == 1 && auth_dat[j].level >= ladmin_min_GM_level) ||
		     (type == 3 && (auth_dat[j].state > 0 || auth_dat[j].ban_until_time != 0)) ||
		     (type == 4 && (auth_dat[j].state == 0 && auth_dat[j].ban_until_time == 0)) ||
		     (type == 5 && (online = check_online_player(account_id)) != 0))) {
			WPACKETL(len     ) = account_id;
			WPACKETB(len +  4) = auth_dat[j].level;
			strncpy(WPACKETP(len + 5), auth_dat[j].userid, 24);
			WPACKETB(len + 29) = auth_dat[j].sex;
			WPACKETL(len + 30) = auth_dat[j].logincount;
			if (auth_dat[j].state == 0 && auth_dat[j].ban_until_time != 0) /* if no state and banished */
				WPACKETW(len + 34) = 7; /* 6 = Your are Prohibited to log in until %s */
			else
				WPACKETW(len + 34) = auth_dat[j].state;
			if (type == 5) // don't repeat check_online_player twice
				WPACKETB(len + 36) = online;
			else
				WPACKETB(len + 36) = check_online_player(account_id);
			len += 37;
		}
	}
	WPACKETW(2) = len;
	SENDPACKET(fd, len);

	FREE(id);
#endif /* TXT_ONLY */

#ifdef USE_SQL
	switch(type) { // 0: any accounts, 1: gm only, 3: accounts with state or bannished, 4: accounts without state and not bannished, 5: online accounts
	case 1:
		sql_request("SELECT COUNT(1), MIN(`%s`) FROM `%s` WHERE `%s` BETWEEN '%d' AND '%d' AND `%s` >= '%d'",
		    login_db_account_id, login_db, login_db_account_id, start, end, login_db_level, ladmin_min_GM_level);
		break;
	case 3:
		sql_request("SELECT COUNT(1), MIN(`%s`) FROM `%s` WHERE `%s` BETWEEN '%d' AND '%d' AND (`state` > '0' OR `ban_until` <> '0')",
		    login_db_account_id, login_db, login_db_account_id, start, end);
		break;
	case 4:
		sql_request("SELECT COUNT(1), MIN(`%s`) FROM `%s` WHERE `%s` BETWEEN '%d' AND '%d' AND `state` = '0' AND `ban_until` = '0'",
		    login_db_account_id, login_db, login_db_account_id, start, end);
		break;
	default:
		sql_request("SELECT COUNT(1), MIN(`%s`) FROM `%s` WHERE `%s` BETWEEN '%d' AND '%d'",
		    login_db_account_id, login_db, login_db_account_id, start, end);
		break;
	}

	/* answer preparation */
	WPACKETW(0) = 0x7921;
	WPACKETB(4) = type;
	len = 5;

	while (sql_get_row() && sql_get_integer(0) > 0) {
		if (sql_get_integer(0) > 600) {
			start = sql_get_integer(1);
			end2 = start + 600; /* max 600, perhaps not 600 accounts */
		} else
			end2 = end;
		switch(type) { /* 0: any accounts, 1: gm only, 3: accounts with state or bannished, 4: accounts without state and not bannished */
		case 1:
			sql_request("SELECT `%s`, `%s`, `%s`, `sex`, `logincount`, `ban_until`, `state` "
			            "FROM `%s` WHERE `%s` BETWEEN '%d' AND '%d' AND `%s` >= '%d'",
			             login_db_account_id, login_db_level, login_db_userid,
			             login_db, login_db_account_id, start, end2, login_db_level, ladmin_min_GM_level);
			break;
		case 3:
			sql_request("SELECT `%s`, `%s`, `%s`, `sex`, `logincount`, `ban_until`, `state` "
			            "FROM `%s` WHERE `%s` BETWEEN '%d' AND '%d' AND (`state` > '0' OR `ban_until` <> '0')",
			             login_db_account_id, login_db_level, login_db_userid,
			             login_db, login_db_account_id, start, end2);
			break;
		case 4:
			sql_request("SELECT `%s`, `%s`, `%s`, `sex`, `logincount`, `ban_until`, `state` "
			            "FROM `%s` WHERE `%s` BETWEEN '%d' AND '%d' AND `state` = '0' AND `ban_until` = '0'",
			             login_db_account_id, login_db_level, login_db_userid,
			             login_db, login_db_account_id, start, end2);
			break;
		default:
			sql_request("SELECT `%s`, `%s`, `%s`, `sex`, `logincount`, `ban_until`, `state` "
			            "FROM `%s` WHERE `%s` BETWEEN '%d' AND '%d'",
			             login_db_account_id, login_db_level, login_db_userid,
			             login_db, login_db_account_id, start, end2);
			break;
		}
		while (sql_get_row()) {
			char * temp_sex;
			online = check_online_player(sql_get_integer(0));
			if (type != 5 || online) {
				WPACKETL(len     ) = sql_get_integer(0);
				WPACKETB(len +  4) = (unsigned char)sql_get_integer(1);
				strncpy(WPACKETP(len + 5), sql_get_string(2), 24);
				temp_sex = sql_get_string(3);
				WPACKETB(len + 29) = (temp_sex[0] == 'S' || temp_sex[0] == 'S') ? 2 : (temp_sex[0] == 'M' || temp_sex[0] == 'M');
				WPACKETL(len + 30) = sql_get_integer(4);
				if (sql_get_integer(6) == 0 && sql_get_integer(5) != 0) /* if no state and banished */
					WPACKETW(len + 34) = 7; /* 6 = Your are Prohibited to log in until %s */
				else
					WPACKETW(len + 34) = sql_get_integer(6);
				WPACKETB(len + 36) = check_online_player(sql_get_integer(0));
				len += 37;
			}
		}
		// if no online player in the list, and maximum id not found
		if (len == 5 && type == 5 && end2 < end) {
			start = end2 + 1;
			// ask again
			sql_request("SELECT COUNT(1), MIN(`%s`) FROM `%s` WHERE `%s` BETWEEN '%d' AND '%d'",
			    login_db_account_id, login_db, login_db_account_id, start, end);
		}
	}
	WPACKETW(2) = len;
	SENDPACKET(fd, len);
#endif /* USE_SQL */

	return;
}

/*------------------------------------------
  ladmin function:
  0x7930 - Request for an account creation
------------------------------------------*/
static void ladmin_account_creation(const int fd, const char* ip) {
	struct auth_dat account;
	int i, new_id;
	char userid[25], pass[25], sex, email[41]; // 24 + NULL, 24 + NULL, 40 + NULL

	/* get parameters */
	memset(userid, 0, sizeof(userid));
	strncpy(userid, RFIFOP(fd, 2), 24);
	remove_control_chars(userid);
	memset(pass, 0, sizeof(pass));
	strncpy(pass, RFIFOP(fd, 26), 24);
	remove_control_chars(pass);
	sex = RFIFOB(fd,50);
	memset(email, 0, sizeof(email));
	strncpy(email, RFIFOP(fd, 51), 40);
	remove_control_chars(email);
	if (e_mail_check(email) == 0) {
		memset(email, 0, sizeof(email));
		strcpy(email, "a@a.com");
	}

	/* do some checks */
	if (strlen(userid) < 4) {
		write_log("'ladmin': Request for an account creation: invalid account (account name is too short, ip: %s)" RETCODE, ip);
		/* send answer */
		WPACKETW(0) = 0x7931;
		WPACKETL(2) = -1;
		strncpy(WPACKETP(6), RFIFOP(fd,2), 24);
		SENDPACKET(fd, 30);
	} else if (strlen(pass) < 4) {
		write_log("'ladmin': Request for an account creation: invalid account (account: %s, password is too short, ip: %s)" RETCODE, userid, ip);
		/* send answer */
		WPACKETW(0) = 0x7931;
		WPACKETL(2) = -1;
		strncpy(WPACKETP(6), RFIFOP(fd,2), 24);
		SENDPACKET(fd, 30);
	} else if (sex != 'F' && sex != 'M') {
		write_log("'ladmin': Request for an account creation: invalid sex (account: %s, ip: %s)" RETCODE, userid, ip);
		/* send answer */
		WPACKETW(0) = 0x7931;
		WPACKETL(2) = -1;
		strncpy(WPACKETP(6), RFIFOP(fd,2), 24);
		SENDPACKET(fd, 30);
	} else if (account_id_count > END_ACCOUNT_NUM) {
		write_log("'ladmin': Request for an account creation: no more available id number (account: %s, sex: %c, ip: %s)" RETCODE,
		          userid, sex, ip);
		/* send answer */
		WPACKETW(0) = 0x7931;
		WPACKETL(2) = -1;
		strncpy(WPACKETP(6), RFIFOP(fd,2), 24);
		SENDPACKET(fd, 30);
	} else {
		i = search_account_index(userid);
		if (i != -1 && strncmp(auth_dat[i].userid, userid, 24) == 0) {
			write_log("'ladmin': Request for an account creation: account already exists (account: %s, ip: %s)" RETCODE,
			          auth_dat[i].userid, ip);
			/* send answer */
			WPACKETW(0) = 0x7931;
			WPACKETL(2) = -1;
			strncpy(WPACKETP(6), RFIFOP(fd,2), 24);
			SENDPACKET(fd, 30);
		/* if we refuse creation of account with different case - don't check if we authorize, because strict comparaison was done before */
		} else if (unique_case_account_name_creation && (i != -1 || check_existing_account_name_for_creation(userid))) {
			write_log("'ladmin': Request for an account creation: account already exists with different case (account: %s, ip: %s)" RETCODE,
			          userid, ip);
			/* send answer */
			WPACKETW(0) = 0x7931;
			WPACKETL(2) = -1;
			strncpy(WPACKETP(6), RFIFOP(fd,2), 24);
			SENDPACKET(fd, 30);
		} else {
			/* init account structure */
			init_new_account(&account);
			/* copy values */
			strncpy(account.userid, userid, 24);
			if (use_md5_passwds)
				MD5_String(pass, account.pass);
			else
				strncpy(account.pass, pass, 32);
			account.sex = ((sex == 'S' || sex == 's') ? 2 : (sex == 'M' || sex == 'm'));
			strncpy(account.email, email, 40);
			/* Added new account */
			new_id = add_account(&account, NULL);
			write_log("'ladmin': Request for an account creation: ok (account: %s (id: %d), sex: %c, email: %s, ip: %s)" RETCODE,
			          account.userid, auth_dat[new_id].account_id, (account.sex == 2) ? 'S' : (account.sex ? 'M' : 'F'), account.email, ip);
			/* send answer */
			WPACKETW(0) = 0x7931;
			WPACKETL(2) = auth_dat[new_id].account_id;
			strncpy(WPACKETP(6), RFIFOP(fd,2), 24);
			SENDPACKET(fd, 30);
#ifdef TXT_ONLY
			save_account(new_id, 1); /* in SQL, account is already save when we call add_account() */
#endif /* TXT_ONLY */
		}
	}

	return;
}

/*------------------------------------------
  ladmin function:
  0x7932 - Request for an account deletion
------------------------------------------*/
static void ladmin_account_deletion(const int fd, const char* ip) {
	int i;
	char account_name[25], buf[65535];

	/* get parameters */
	memset(account_name, 0, sizeof(account_name));
	strncpy(account_name, RFIFOP(fd, 2), 24);
	remove_control_chars(account_name);

	/* search if account exists */
	i = search_account_index(account_name);
	if (i != -1) {
		/* send answer */
		WPACKETW(0) = 0x7933;
		WPACKETL(2) = auth_dat[i].account_id;
		strncpy(WPACKETP(6), auth_dat[i].userid, 24);
		SENDPACKET(fd, 30);
		/* save deleted account in log file */
		write_log("'ladmin': Request for an account deletion: ok (account: %s, id: %d, ip: %s) - saved in next line:" RETCODE,
		          auth_dat[i].userid, auth_dat[i].account_id, ip);
		write_log("Structure: ID, account name, password, last login time, sex, # of logins, state, email, error message for state 7, validity time, last (accepted) login ip, memo field, ban timestamp, GM level, repeated(register text, register value)" RETCODE);
		account_to_str(buf, &auth_dat[i]);
		write_log("%s" RETCODE, buf);
		/* delete account - send information to all char servers */
		delete_account(account_name);
	} else {
		/* send answer */
		WPACKETW(0) = 0x7933;
		WPACKETL(2) = -1;
		strncpy(WPACKETP(6), account_name, 24);
		SENDPACKET(fd, 30);
		write_log("'ladmin': Request for an account deletion: unknown account (account: %s, ip: %s)" RETCODE, account_name, ip);
	}

	return;
}

/*---------------------------------------
  ladmin function:
  0x7934 - Request to change a password
---------------------------------------*/
static void ladmin_change_password(const int fd, const char* ip) {
	int i;
	char account_name[25], pass[25];

	/* get parameters */
	memset(account_name, 0, sizeof(account_name));
	strncpy(account_name, RFIFOP(fd, 2), 24);
	remove_control_chars(account_name);
	memset(pass, 0, sizeof(pass));
	strncpy(pass, RFIFOP(fd,26), 24);
	remove_control_chars(pass);

	/* search if account exists */
	i = search_account_index(account_name);
	if (i != -1) {
		/* send answer */
		WPACKETW(0) = 0x7935;
		WPACKETL(2) = auth_dat[i].account_id;
		strncpy(WPACKETP(6), auth_dat[i].userid, 24);
		SENDPACKET(fd, 30);
		/* change password */
		memset(auth_dat[i].pass, 0, sizeof(auth_dat[i].pass));
		if (use_md5_passwds)
			MD5_String(pass, auth_dat[i].pass);
		else
			strncpy(auth_dat[i].pass, pass, 32);
		write_log("'ladmin': Request to change a password: ok (account: %s, ip: %s)" RETCODE, auth_dat[i].userid, ip);
		save_account(i, 1);
	} else {
		/* send answer */
		WPACKETW(0) = 0x7935;
		WPACKETL(2) = -1;
		strncpy(WPACKETP(6), account_name, 24);
		SENDPACKET(fd, 30);
		write_log("'ladmin': Request to change a password: unknown account (account: %s, ip: %s)" RETCODE, account_name, ip);
	}

	return;
}

/*------------------------------------
  ladmin function:
  0x7936 - Request to change a state
------------------------------------*/
static void ladmin_change_state(const int fd, const char* ip) {
	int i, j, statut;
	char account_name[25], error_message[20]; // 24 + NULL, 19 + NULL

	/* get parameters */
	memset(account_name, 0, sizeof(account_name));
	strncpy(account_name, RFIFOP(fd, 2), 24);
	remove_control_chars(account_name);
	statut = RFIFOL(fd,26);
	memset(error_message, 0, sizeof(error_message));
	strncpy(error_message, RFIFOP(fd, 30), 19);
	remove_control_chars(error_message);

	/* check logic of paramaters */
	if (statut != 7 || error_message[0] == '\0') { /* 7: 6 = Your are Prohibited to log in until %s */
		memset(error_message, 0, sizeof(error_message));
		strcpy(error_message, "-");
	}

	/* search if account exists */
	i = search_account_index(account_name);
	if (i != -1) {
		/* send answer */
		WPACKETW( 0) = 0x7937;
		WPACKETL( 2) = auth_dat[i].account_id;
		strncpy(WPACKETP(6), auth_dat[i].userid, 24);
		WPACKETL(30) = statut;
		SENDPACKET(fd, 34);
		/* if it's already the good state */
		if (auth_dat[i].state == statut && strcmp(auth_dat[i].error_message, error_message) == 0)
			write_log("'ladmin': Request to change a state: state of the account is already the right state (account: %s, received state: %d, ip: %s)" RETCODE,
			          account_name, statut, ip);
		/* if it's not already the good state */
		else {
			if (statut == 7)
				write_log("'ladmin': Request to change a state: ok (account: %s, new state: %d - prohibited to login until '%s', ip: %s)" RETCODE,
				          auth_dat[i].userid, statut, error_message, ip);
			else
				write_log("'ladmin': Request to change a state: ok (account: %s, new state: %d, ip: %s)" RETCODE, auth_dat[i].userid, statut, ip);
			/* new statut is not 'no status/ok' */
			if (statut != 0) {
				WPACKETW(0) = 0x2731;
				WPACKETL(2) = auth_dat[i].account_id;
				WPACKETB(6) = 0; /* 0: change of statut, 1: ban */
				WPACKETL(7) = statut; /* status or final date of a banishment */
				charif_sendall(11); /* send information to all char servers */
				for(j = 0; j < AUTH_FIFO_SIZE; j++)
					if (auth_fifo[j].account_id == auth_dat[i].account_id)
						auth_fifo[j].login_id1++; /* to avoid reconnection error when come back from map-server (char-server will ask again the authentification) */
			}
			/* save state */
			auth_dat[i].state = statut;
			memset(auth_dat[i].error_message, 0, sizeof(auth_dat[i].error_message));
			strncpy(auth_dat[i].error_message, error_message, 19); // 19 + NULL
			save_account(i, 1);
		}
	/* account is not found */
	} else {
		/* send answer */
		WPACKETW( 0) = 0x7937;
		WPACKETL( 2) = -1;
		strncpy(WPACKETP(6), account_name, 24);
		WPACKETL(30) = statut;
		SENDPACKET(fd, 34);
		write_log("'ladmin': Request to change a state: unknown account (account: %s, received state: %d, ip: %s)" RETCODE,
		          account_name, statut, ip);
	}

	return;
}

/*-----------------------------------------------------------
  ladmin function:
  0x7938 - Request for servers list and # of online players
-----------------------------------------------------------*/
static void ladmin_list_of_servers(const int fd, const char* ip) {
	int i, j;

	write_log("'ladmin': Request of a servers list (ip: %s)" RETCODE, ip);
	j = 0;
	for(i = 0; i < MAX_SERVERS; i++) { // max number of char-servers (and account_id values: 0 to max-1)
		if (server_fd[i] >= 0) {
			WPACKETL(4 + j * 32     ) = server[i].ip;
			WPACKETW(4 + j * 32 +  4) = server[i].port;
			strncpy(WPACKETP(4 + j * 32 + 6), server[i].name, 20);
			WPACKETW(4 + j * 32 + 26) = server[i].users;
			WPACKETW(4 + j * 32 + 28) = server[i].maintenance;
			WPACKETW(4 + j * 32 + 30) = server[i].new;
			j++;
		}
	}
	WPACKETW(0) = 0x7939;
	WPACKETW(2) = 4 + 32 * j;
	SENDPACKET(fd, 4 + 32 * j);

	return;
}

/*--------------------------------------
  ladmin function:
  0x793a - Request to check a password
--------------------------------------*/
static void ladmin_check_password(const int fd, const char* ip) {
	int i;
	char account_name[25], pass[25];

	/* get parameters */
	memset(account_name, 0, sizeof(account_name));
	strncpy(account_name, RFIFOP(fd, 2), 24);
	remove_control_chars(account_name);
	memset(pass, 0, sizeof(pass));
	strncpy(pass, RFIFOP(fd, 26), 24);
	remove_control_chars(pass);

	/* search if account exists */
	i = search_account_index(account_name);
	if (i != -1) {
		if (password_check(auth_dat[i].pass, pass, "")) {
			/* answer */
			WPACKETW(0) = 0x793b;
			WPACKETL(2) = auth_dat[i].account_id;
			strncpy(WPACKETP(6), auth_dat[i].userid, 24);
			SENDPACKET(fd, 30);
			write_log("'ladmin': Request to check a password: ok (account: %s, ip: %s)" RETCODE,
			          auth_dat[i].userid, ip);
		} else {
			/* answer */
			WPACKETW(0) = 0x793b;
			WPACKETL(2) = -1;
			strncpy(WPACKETP(6), auth_dat[i].userid, 24);
			SENDPACKET(fd, 30);
			write_log("'ladmin': Request to check a password: invalid password (account: %s, ip: %s)" RETCODE,
			          auth_dat[i].userid, ip);
		}
	} else {
		/* answer */
		WPACKETW(0) = 0x793b;
		WPACKETL(2) = -1;
		strncpy(WPACKETP(6), account_name, 24);
		SENDPACKET(fd, 30);
		write_log("'ladmin': Request to check a password: unknown account (account: %s, ip: %s)" RETCODE, account_name, ip);
	}

	return;
}

/*----------------------------------
  ladmin function:
  0x793c - Request to change a sex
----------------------------------*/
static void ladmin_change_sex(const int fd, const char* ip) {
	int i, j;
	char account_name[25], sex;

	/* get parameters */
	memset(account_name, 0, sizeof(account_name));
	strncpy(account_name, RFIFOP(fd, 2), 24);
	remove_control_chars(account_name);
	sex = RFIFOB(fd, 26);

	/* Check value of the sex */
	if (sex != 'F' && sex != 'M') {
		/* answer */
		WPACKETW(0) = 0x793d;
		WPACKETL(2) = -1;
		strncpy(WPACKETP(6), account_name, 24);
		SENDPACKET(fd, 30);
		if (isprint((int)sex) > 31)
			write_log("'ladmin': Request to change a sex: invalid sex (account: %s, received sex: %c, ip: %s)" RETCODE,
			          account_name, sex, ip);
		else
			write_log("'ladmin': Request to change a sex: invalid sex (account: %s, received sex: 'control char', ip: %s)" RETCODE,
			          account_name, ip);
	} else {
		/* search if account exists */
		i = search_account_index(account_name);
		if (i != -1) {
			/* if it's not the good sex */
			if (auth_dat[i].sex != ((sex == 'S' || sex == 's') ? 2 : (sex == 'M' || sex == 'm'))) {
				/* answer */
				WPACKETW(0) = 0x793d;
				WPACKETL(2) = auth_dat[i].account_id;
				strncpy(WPACKETP(6), auth_dat[i].userid, 24);
				SENDPACKET(fd, 30);
				for(j = 0; j < AUTH_FIFO_SIZE; j++)
					if (auth_fifo[j].account_id == auth_dat[i].account_id)
						auth_fifo[j].login_id1++; /* to avoid reconnection error when come back from map-server (char-server will ask again the authentification) */
				write_log("'ladmin': Request to change a sex: ok (account: %s, new sex: %c, ip: %s)" RETCODE,
				          auth_dat[i].userid, sex, ip);
				/* save sex */
				auth_dat[i].sex = (sex == 'S' || sex == 's') ? 2 : (sex == 'M' || sex == 'm');
				save_account(i, 1);
				/* send to all char-server the change */
				WPACKETW(0) = 0x2723; // 0x2723 <account_id>.L <sex>.B <account_id_of_GM>.L (sex = -1 -> failed; account_id_of_GM = -1 -> ladmin)
				WPACKETL(2) = auth_dat[i].account_id;
				WPACKETB(6) = auth_dat[i].sex;
				WPACKETL(7) = -1; // who want do operation: -1, ladmin. other: account_id of GM
				charif_sendall(11); /* send information to all char servers */
			} else {
				/* answer */
				WPACKETW(0) = 0x793d;
				WPACKETL(2) = -1;
				strncpy(WPACKETP(6), auth_dat[i].userid, 24);
				SENDPACKET(fd, 30);
				write_log("'ladmin': Request to change a sex: sex is already the good sex (account: %s, sex: %c, ip: %s)" RETCODE,
				          auth_dat[i].userid, sex, ip);
			}
		/* account is not found */
		} else {
			/* answer */
			WPACKETW(0) = 0x793d;
			WPACKETL(2) = -1;
			strncpy(WPACKETP(6), account_name, 24);
			SENDPACKET(fd, 30);
			write_log("'ladmin': Request to change a sex: unknown account (account: %s, received sex: %c, ip: %s)" RETCODE,
			          account_name, sex, ip);
		}
	}

	return;
}

/*------------------------------------
  ladmin function:
  0x793e - Request to change a level
------------------------------------*/
static void ladmin_change_level(const int fd, const char* ip) {
	int i;
	char account_name[25];
	unsigned char level;
	time_t now;
	char tmpstr[128];
	int tmpstr_len;

	/* get parameters */
	memset(account_name, 0, sizeof(account_name));
	strncpy(account_name, RFIFOP(fd, 2), 24);
	remove_control_chars(account_name);
	level = RFIFOB(fd, 26);

	/* Check value of the level */
	if (level > 99) { /* not need to check < 0, it's unsigned value */
		/* answer */
		WPACKETW( 0) = 0x793f;
		WPACKETL( 2) = -1;
		strncpy(WPACKETP(6), account_name, 24);
		WPACKETB(30) = 0;
		WPACKETB(31) = level;
		SENDPACKET(fd, 32);
		write_log("'ladmin': Request to change a level: invalid level (account: %s, received level: %d, ip: %s)" RETCODE,
		          account_name, (int)level, ip);
	} else {
		/* search if account exists */
		i = search_account_index(account_name);
		if (i != -1) {
			/* if it's not the good level */
			if (auth_dat[i].level != level) {
				/* answer */
				WPACKETW( 0) = 0x793f;
				WPACKETL( 2) = auth_dat[i].account_id;
				strncpy(WPACKETP(6), auth_dat[i].userid, 24);
				WPACKETB(30) = auth_dat[i].level;
				WPACKETB(31) = level;
				SENDPACKET(fd, 32);
				write_log("'ladmin': Request to change a level: ok (account: %s, new level: %d, ip: %s)" RETCODE,
				          auth_dat[i].userid, (int)level, ip);
				/* add comment in memo field */
				now = time(NULL);
				memset(tmpstr, 0, sizeof(tmpstr));
				tmpstr_len = strftime(tmpstr, 20, date_format, localtime(&now)); // 19 + NULL
				sprintf(tmpstr + tmpstr_len, ": Level changing %d->%d.", (int)auth_dat[i].level, (int)level);
				add_text_in_memo(i, tmpstr);
				/* save level */
				auth_dat[i].level = level;
#ifdef TXT_ONLY
				/* need to save GM level file if we save as old version type */
				GM_level_need_save_flag = 1;
#endif /* TXT_ONLY */
				save_account(i, 1);
				/* send new level to all char-servers */
				send_GM_account(auth_dat[i].account_id, level);
			} else {
				/* answer */
				WPACKETW( 0) = 0x793f;
				WPACKETL( 2) = -1;
				strncpy(WPACKETP(6), auth_dat[i].userid, 24);
				WPACKETB(30) = auth_dat[i].level;
				WPACKETB(31) = level;
				SENDPACKET(fd, 32);
				write_log("'ladmin': Request to change a level: level is already the good level (account: %s, level: %d, ip: %s)" RETCODE,
				          auth_dat[i].userid, (int)level, ip);
			}
		/* account is not found */
		} else {
			/* answer */
			WPACKETW( 0) = 0x793f;
			WPACKETL( 2) = -1;
			strncpy(WPACKETP(6), account_name, 24);
			WPACKETB(30) = 0;
			WPACKETB(31) = level;
			SENDPACKET(fd, 32);
			write_log("'ladmin': Request to change a level: unknown account (account: %s, received level: %d, ip: %s)" RETCODE,
			          account_name, (int)level, ip);
		}
	}

	return;
}

/*-----------------------------------
  ladmin function:
  0x7940 - Request to change a mail
-----------------------------------*/
static void ladmin_change_mail(const int fd, const char* ip) {
	int i;
	char account_name[25], mail[41]; // 24 + NULL, 40 + NULL

	/* get parameters */
	memset(account_name, 0, sizeof(account_name));
	strncpy(account_name, RFIFOP(fd, 2), 24);
	remove_control_chars(account_name);
	memset(mail, 0, sizeof(mail));
	strncpy(mail, RFIFOP(fd, 26), 40);
	remove_control_chars(mail);

	/* check the mail */
	if (e_mail_check(mail) == 0) {
		/* send answer */
		WPACKETW(0) = 0x7941;
		WPACKETL(2) = -1;
		strncpy(WPACKETP(6), account_name, 24);
		SENDPACKET(fd, 30);
		write_log("'ladmin': Request to change a mail: invalid mail (account: %s, ip: %s)" RETCODE, account_name, ip);
	} else {
		/* search if account exists */
		i = search_account_index(account_name);
		if (i != -1) {
			/* send answer */
			WPACKETW(0) = 0x7941;
			WPACKETL(2) = auth_dat[i].account_id;
			strncpy(WPACKETP(6), auth_dat[i].userid, 24);
			SENDPACKET(fd, 30);
			write_log("'ladmin': Request to change a mail: ok (account: %s, new mail: %s, ip: %s)" RETCODE, auth_dat[i].userid, mail, ip);
			/* save mail */
			memset(auth_dat[i].email, 0, sizeof(auth_dat[i].email));
			strncpy(auth_dat[i].email, mail, 40);
			save_account(i, 1);
			// send change to all map-servers
			WPACKETW(0) = 0x272b; // 0x272b/0x2b18 <account_id>.L <new_e-mail>.40B
			WPACKETL(2) = auth_dat[i].account_id;
			strncpy(WPACKETP(6), mail, 40); // set to NULL to say: not changed
			charif_sendall(46);
		} else {
			WPACKETW(0) = 0x7941;
			WPACKETL(2) = -1;
			strncpy(WPACKETP(6), account_name, 24);
			SENDPACKET(fd, 30);
			write_log("'ladmin': Request to change a mail: unknown account (account: %s, received mail: %s, ip: %s)" RETCODE,
			          account_name, mail, ip);
		}
	}

	return;
}

/*-----------------------------------
  ladmin function:
  0x7942 - Request to change a memo
-----------------------------------*/
static void ladmin_change_memo(const int fd, const char* ip) {
	int i;
	char account_name[25];
	unsigned int size_of_memo;
	char * memo;

	/* get parameters */
	memset(account_name, 0, sizeof(account_name));
	strncpy(account_name, RFIFOP(fd, 4), 24);
	remove_control_chars(account_name);
	size_of_memo = RFIFOW(fd, 2) - 28;
	/* limit size of memo to 60000 */
	if (size_of_memo > 60000)
		size_of_memo = 60000;
	CALLOC(memo, char, size_of_memo + 1); // + NULL
	strncpy(memo, RFIFOP(fd, 28), size_of_memo);
	remove_control_chars(memo);
	size_of_memo = strlen(memo); /* recalculate to have correct length */

	/* search if account exists */
	i = search_account_index(account_name);
	if (i != -1) {
		/* send answer */
		WPACKETW(0) = 0x7943;
		WPACKETL(2) = auth_dat[i].account_id;
		strncpy(WPACKETP(6), auth_dat[i].userid, 24);
		SENDPACKET(fd, 30);
		FREE(auth_dat[i].memo);
		if (size_of_memo > 0) {
			write_log("'ladmin': Request to change a memo: ok (account: %s, new memo: %s, ip: %s)" RETCODE, auth_dat[i].userid, memo, ip);
			CALLOC(auth_dat[i].memo, char, size_of_memo + 1); // + NULL
			strncpy(auth_dat[i].memo, memo, size_of_memo);
		} else
			write_log("'ladmin': Request to change a memo: ok (account: %s, new memo: (void), ip: %s)" RETCODE, auth_dat[i].userid, ip);
		/* save memo */
		save_account(i, 1);
	} else {
		/* send answer */
		WPACKETW(0) = 0x7943;
		WPACKETL(2) = -1;
		strncpy(WPACKETP(6), account_name, 24);
		SENDPACKET(fd, 30);
		if (size_of_memo > 0)
			write_log("'ladmin': Request to change a memo: unknown account (account: %s, received memo: %s, ip: %s)" RETCODE,
			          account_name, memo, ip);
		else
			write_log("'ladmin': Request to change a memo: unknown account (account: %s, received memo: (void), ip: %s)" RETCODE,
			          account_name, ip);
	}
	FREE(memo);

	return;
}

/*-----------------------------------------
  ladmin function:
  0x7944 - Request to found an account id
-----------------------------------------*/
static void ladmin_search_account_id(const int fd, const char* ip) {
	int i;
	char account_name[25]; // 24 + NULL

	/* get parameters */
	memset(account_name, 0, sizeof(account_name));
	strncpy(account_name, RFIFOP(fd, 2), 24);
	remove_control_chars(account_name);

	/* search if account exists */
	i = search_account_index(account_name);
	if (i != -1) {
		/* send answer */
		WPACKETW(0) = 0x7945;
		WPACKETL(2) = auth_dat[i].account_id;
		strncpy(WPACKETP(6), auth_dat[i].userid, 24);
		SENDPACKET(fd, 30);
		write_log("'ladmin': Request to found an account id: ok (account: %s, id: %d, ip: %s)" RETCODE, auth_dat[i].userid, auth_dat[i].account_id, ip);
	} else {
		/* send answer */
		WPACKETW(0) = 0x7945;
		WPACKETL(2) = -1;
		strncpy(WPACKETP(6), account_name, 24);
		SENDPACKET(fd, 30);
		write_log("'ladmin': Request to found an account id: unknown account (account: %s, ip: %s)" RETCODE, account_name, ip);
	}

	return;
}

/*-------------------------------------------
  ladmin function:
  0x7946 - Request to found an account name
-------------------------------------------*/
static void ladmin_search_account_name(const int fd, const char* ip) {
	int i, account_id;

	/* get parameters */
	account_id = RFIFOL(fd,2);

	/* search if account exists */
	i = search_account_index2(account_id);
	if (i != -1) {
		/* send answer */
		WPACKETW(0) = 0x7947;
		WPACKETL(2) = account_id;
		strncpy(WPACKETP(6), auth_dat[i].userid, 24);
		SENDPACKET(fd, 30);
		write_log("'ladmin': Request to found an account name: ok (account: %s, id: %d, ip: %s)" RETCODE, auth_dat[i].userid, account_id, ip);
	} else {
		/* send answer */
		WPACKETW(0) = 0x7947;
		WPACKETL(2) = account_id;
		memset(WPACKETP(6), 0, 24);
		SENDPACKET(fd, 30);
		write_log("'ladmin': Request to found an account name: unknown account (id: %d, ip: %s)" RETCODE, account_id, ip);
	}

	return;
}

/*-----------------------------------------------------------
  ladmin function:
  0x7948 - Request to set a validity limit (absolute value)
-----------------------------------------------------------*/
static void ladmin_set_validity_limit(const int fd, const char* ip) {
	int i;
	char account_name[25], tmpstr[25];
	time_t timestamp;

	/* get parameters */
	memset(account_name, 0, sizeof(account_name));
	strncpy(account_name, RFIFOP(fd, 2), 24);
	remove_control_chars(account_name);
	timestamp = (time_t)RFIFOL(fd, 26);

	/* prepare possible string for date */
	memset(tmpstr, 0, sizeof(tmpstr));
	strftime(tmpstr, 20, date_format, localtime(&timestamp)); // 19 + NULL

	/* search if account exists */
	i = search_account_index(account_name);
	if (i != -1) {
		/* if it's not the good validity limit */
		if (auth_dat[i].connect_until_time != timestamp) { /* specialy for unlimited on an unlimited account */
			/* answer */
			WPACKETW( 0) = 0x7949;
			WPACKETL( 2) = auth_dat[i].account_id;
			strncpy(WPACKETP(6), auth_dat[i].userid, 24);
			WPACKETL(30) = timestamp;
			SENDPACKET(fd, 34);
			write_log("'ladmin': Request to set a validity limit: ok (account: %s, new validity: %d (%s), ip: %s)" RETCODE,
			          auth_dat[i].userid, timestamp, ((timestamp == 0) ? "unlimited" : tmpstr), ip);
			/* save validity limit */
			auth_dat[i].connect_until_time = timestamp;
			save_account(i, 1);
		} else {
			/* answer */
			WPACKETW( 0) = 0x7949;
			WPACKETL( 2) = -1;
			strncpy(WPACKETP(6), auth_dat[i].userid, 24);
			WPACKETL(30) = timestamp;
			SENDPACKET(fd, 34);
			write_log("'ladmin': Request to set a validity limit: validity limit is already the good value (account: %s, validity: %d (%s), ip: %s)" RETCODE,
			          account_name, timestamp, ((timestamp == 0) ? "unlimited" : tmpstr), ip);
		}
	/* account is not found */
	} else {
		/* answer */
		WPACKETW( 0) = 0x7949;
		WPACKETL( 2) = -1;
		strncpy(WPACKETP(6), account_name, 24);
		WPACKETL(30) = timestamp;
		SENDPACKET(fd, 34);
		write_log("'ladmin': Request to set a validity limit: unknown account (account: %s, received validity: %d (%s), ip: %s)" RETCODE,
		          account_name, timestamp, ((timestamp == 0) ? "unlimited" : tmpstr), ip);
	}

	return;
}

/*---------------------------------------------------------------------
  ladmin function:
  0x794a - Request to set a final date of banishment (absolute value)
---------------------------------------------------------------------*/
static void ladmin_set_ban_limit(const int fd, const char* ip) {
	int i, j;
	char account_name[25], tmpstr[25];
	time_t timestamp;

	/* get parameters */
	memset(account_name, 0, sizeof(account_name));
	strncpy(account_name, RFIFOP(fd, 2), 24);
	remove_control_chars(account_name);
	timestamp = (time_t)RFIFOL(fd, 26);

	/* Check parameter */
	if (timestamp <= time(NULL))
		timestamp = 0;

	/* prepare possible string for date */
	memset(tmpstr, 0, sizeof(tmpstr));
	strftime(tmpstr, 20, date_format, localtime(&timestamp)); // 19 + NULL

	/* search if account exists */
	i = search_account_index(account_name);
	if (i != -1) {
		/* if it's not the good final date of banishment */
		if (auth_dat[i].ban_until_time != timestamp) { /* specialy for not banished on not banished account */
			/* answer */
			WPACKETW( 0) = 0x794b;
			WPACKETL( 2) = auth_dat[i].account_id;
			strncpy(WPACKETP(6), auth_dat[i].userid, 24);
			WPACKETL(30) = timestamp;
			SENDPACKET(fd, 34);
			write_log("'ladmin': Request to set a final date of banishment: ok (account: %s, new final date of banishment: %d (%s), ip: %s)" RETCODE,
			          auth_dat[i].userid, timestamp, ((timestamp == 0) ? "no banishment" : tmpstr), ip);
			/* if new banishement */
			if (auth_dat[i].ban_until_time == 0) { /* was not banished, is now banished */
				WPACKETW(0) = 0x2731;
				WPACKETL(2) = auth_dat[i].account_id;
				WPACKETB(6) = 1; /* 0: change of statut, 1: ban */
				WPACKETL(7) = timestamp; /* status or final date of a banishment */
				charif_sendall(11); /* send information to all char servers */
				for(j = 0; j < AUTH_FIFO_SIZE; j++)
					if (auth_fifo[j].account_id == auth_dat[i].account_id)
						auth_fifo[j].login_id1++; /* to avoid reconnection error when come back from map-server (char-server will ask again the authentification) */
			}
			/* save final date of banishment */
			auth_dat[i].ban_until_time = timestamp;
			save_account(i, 1);
		} else {
			/* answer */
			WPACKETW( 0) = 0x794b;
			WPACKETL( 2) = -1;
			strncpy(WPACKETP(6), auth_dat[i].userid, 24);
			WPACKETL(30) = timestamp;
			SENDPACKET(fd, 34);
			write_log("'ladmin': Request to set a final date of banishment: final date of banishment is already the good value (account: %s, final date of banishment: %d (%s), ip: %s)" RETCODE,
			          account_name, timestamp, ((timestamp == 0) ? "no banishment" : tmpstr), ip);
		}
	/* account is not found */
	} else {
		/* answer */
		WPACKETW( 0) = 0x794b;
		WPACKETL( 2) = -1;
		strncpy(WPACKETP(6), account_name, 24);
		WPACKETL(30) = timestamp;
		SENDPACKET(fd, 34);
		write_log("'ladmin': Request to set a final date of banishment: unknown account (account: %s, received final date of banishment: %d (%s), ip: %s)" RETCODE,
		          account_name, timestamp, ((timestamp == 0) ? "no banishment" : tmpstr), ip);
	}

	return;
}

/*-----------------------------------------------------------------------
  ladmin function:
  0x794c - Request to adjust a final date of banishment (relativ value)
-----------------------------------------------------------------------*/
static void ladmin_adjust_ban_limit(const int fd, const char* ip) {
	int i, j;
	char account_name[25], tmpstr[25];
	struct tm *tmtime;
	time_t timestamp;
	short year, month, day, hour, min, sec;

	/* get parameters */
	memset(account_name, 0, sizeof(account_name));
	strncpy(account_name, RFIFOP(fd, 2), 24);
	remove_control_chars(account_name);
	year  = (short)RFIFOW(fd,26);
	month = (short)RFIFOW(fd,28);
	day   = (short)RFIFOW(fd,30);
	hour  = (short)RFIFOW(fd,32);
	min   = (short)RFIFOW(fd,34);
	sec   = (short)RFIFOW(fd,36);

	/* search if account exists */
	i = search_account_index(account_name);
	if (i != -1) {
		/* Calculation of the new final date of banishement */
		if (auth_dat[i].ban_until_time == 0 || auth_dat[i].ban_until_time < time(NULL))
			timestamp = time(NULL);
		else
			timestamp = auth_dat[i].ban_until_time;
		tmtime = localtime(&timestamp);
		tmtime->tm_year = tmtime->tm_year + year;
		tmtime->tm_mon  = tmtime->tm_mon  + month;
		tmtime->tm_mday = tmtime->tm_mday + day;
		tmtime->tm_hour = tmtime->tm_hour + hour;
		tmtime->tm_min  = tmtime->tm_min  + min;
		tmtime->tm_sec  = tmtime->tm_sec  + sec;
		timestamp = mktime(tmtime);
		/* if the new final date is possible */
		if (timestamp != -1) {
			if (timestamp <= time(NULL))
				timestamp = 0;
			/* answer */
			WPACKETW( 0) = 0x794d;
			WPACKETL( 2) = auth_dat[i].account_id;
			strncpy(WPACKETP(6), auth_dat[i].userid, 24);
			WPACKETL(30) = (unsigned long)timestamp;
			SENDPACKET(fd, 34);
			/* preparation of the string */
			memset(tmpstr, 0, sizeof(tmpstr));
			strftime(tmpstr, 20, date_format, localtime(&timestamp)); // 19 + NULL
			/* if it's not the good final date of banishment */
			if (auth_dat[i].ban_until_time != timestamp) { /* final date is modified ? */
				write_log("'ladmin': Request to adjust a final date of banishment: ok (account: %s, (%+d y %+d m %+d d %+d h %+d mn %+d s) -> new final date: %d (%s), ip: %s)" RETCODE,
				          auth_dat[i].userid, year, month, day, hour, min, sec, timestamp, (timestamp == 0 ? "no banishment" : tmpstr), ip);
				/* if new banishement */
				if (auth_dat[i].ban_until_time == 0) { /* was not banished, is now banished */
					WPACKETW(0) = 0x2731;
					WPACKETL(2) = auth_dat[i].account_id;
					WPACKETB(6) = 1; /* 0: change of statut, 1: ban */
					WPACKETL(7) = timestamp; /* status or final date of a banishment */
					charif_sendall(11); /* send information to all char servers */
					for(j = 0; j < AUTH_FIFO_SIZE; j++)
						if (auth_fifo[j].account_id == auth_dat[i].account_id)
							auth_fifo[j].login_id1++; /* to avoid reconnection error when come back from map-server (char-server will ask again the authentification) */
				}
				/* save final date of banishment */
				auth_dat[i].ban_until_time = timestamp;
				save_account(i, 1);
			} else {
				write_log("'ladmin': Request to adjust a final date of banishment: no adjustment (account: %s, final date of banishment: %d (%s), ip: %s)" RETCODE,
				          account_name, timestamp, ((timestamp == 0) ? "no banishment" : tmpstr), ip);
			}
		/* so, new final date of banishement is impossible */
		} else {
			/* answer */
			WPACKETW( 0) = 0x794d;
			WPACKETL( 2) = auth_dat[i].account_id;
			strncpy(WPACKETP(6), auth_dat[i].userid, 24);
			WPACKETL(30) = (unsigned long)auth_dat[i].ban_until_time;
			SENDPACKET(fd, 34);
			/* preparation of the string */
			memset(tmpstr, 0, sizeof(tmpstr));
			strftime(tmpstr, 20, date_format, localtime(&auth_dat[i].ban_until_time)); // 19 + NULL
			write_log("'ladmin': Request to adjust a final date of banishment: impossible to adjust (account: %s, %d (%s) + (%+d y %+d m %+d d %+d h %+d mn %+d s) -> ???, ip: %s)" RETCODE,
			          account_name, auth_dat[i].ban_until_time, (auth_dat[i].ban_until_time == 0 ? "no banishment" : tmpstr), year, month, day, hour, min, sec, ip);
		}
	/* account is not found */
	} else {
		/* answer */
		WPACKETW( 0) = 0x794d;
		WPACKETL( 2) = -1;
		strncpy(WPACKETP(6), account_name, 24);
		WPACKETL(30) = 0;
		SENDPACKET(fd, 34);
		write_log("'ladmin': Request to adjust a final date of banishment: unknown account (account: %s, ip: %s)" RETCODE, account_name, ip);
	}

	return;
}

/*----------------------------------
  ladmin function:
  0x794e - Request for a broadcast
----------------------------------*/
static void ladmin_broadcast(const int fd, const char* ip) {
	int i, packet_size;

	/* get parameters */
	packet_size = RFIFOW(fd,2);

	/* search if there is a message */
	if (packet_size < 7) {
		/* answer */
		WPACKETW(0) = 0x794f;
		WPACKETW(2) = -1;
		SENDPACKET(fd, 4);
		write_log("'ladmin': Request for a broadcast: no message (ip: %s)" RETCODE, ip);
	} else {
		/* at least 1 char-server */
		for(i = 0; i < MAX_SERVERS; i++) // max number of char-servers (and account_id values: 0 to max-1)
			if (server_fd[i] >= 0)
				break;
		if (i == MAX_SERVERS) { // max number of char-servers (and account_id values: 0 to max-1)
			/* answer */
			WPACKETW(0) = 0x794f;
			WPACKETW(2) = -1;
			SENDPACKET(fd, 4);
			write_log("'ladmin': Request for a broadcast: no char-server is online (ip: %s)" RETCODE, ip);
		} else {
			/* answer */
			WPACKETW(0) = 0x794f;
			WPACKETW(2) = 0;
			SENDPACKET(fd, 4);
			/* preparation of packet for char-servers */
			WPACKETW(0) = 0x2726;
			memcpy(WPACKETP(2), RFIFOP(fd,2), packet_size - 2);
			WPACKETB(packet_size) = '\0'; /* for the remove control_char after */
			remove_control_chars(WPACKETP(6));
			/* log */
			if (WPACKETW(4) == 0)
				write_log("'ladmin': Request for a broadcast: ok (message (in yellow): %s, ip: %s)" RETCODE, WPACKETP(6), ip);
			else
				write_log("'ladmin': Request for a broadcast: ok (message (in blue): %s, ip: %s)" RETCODE, WPACKETP(6), ip);
			/* send same message to all char-servers (no answer) */
			charif_sendall(packet_size); /* send information to all char servers */
		}
	}

	return;
}

/*-------------------------------------------------------------
  ladmin function:
  0x7950 - Request to adjust a validity limit (relativ value)
-------------------------------------------------------------*/
static void ladmin_adjust_validity_limit(const int fd, const char* ip) {
	int i;
	char account_name[25], tmpstr[25], tmpstr2[25];
	struct tm *tmtime;
	time_t timestamp;
	short year, month, day, hour, min, sec;

	/* get parameters */
	memset(account_name, 0, sizeof(account_name));
	strncpy(account_name, RFIFOP(fd, 2), 24);
	remove_control_chars(account_name);
	year  = (short)RFIFOW(fd,26);
	month = (short)RFIFOW(fd,28);
	day   = (short)RFIFOW(fd,30);
	hour  = (short)RFIFOW(fd,32);
	min   = (short)RFIFOW(fd,34);
	sec   = (short)RFIFOW(fd,36);

	/* search if account exists */
	i = search_account_index(account_name);
	if (i != -1) {
		/* check if we authorize modification of unlimited account */
		if (add_to_unlimited_account == 0 && auth_dat[i].connect_until_time == 0) {
			/* answer */
			WPACKETW( 0) = 0x7951;
			WPACKETL( 2) = auth_dat[i].account_id;
			strncpy(WPACKETP(6), auth_dat[i].userid, 24);
			WPACKETL(30) = 0;
			SENDPACKET(fd, 34);
			write_log("'ladmin': Request to adjust a validity limit: adjustement of unlimited account is not authorized (account: %s, ip: %s)" RETCODE,
			          auth_dat[i].userid, ip);
		} else {
			/* Calculation of the new validity limit */
			if (auth_dat[i].connect_until_time == 0 || auth_dat[i].connect_until_time < time(NULL))
				timestamp = time(NULL);
			else
				timestamp = auth_dat[i].connect_until_time;
			tmtime = localtime(&timestamp);
			tmtime->tm_year = tmtime->tm_year + year;
			tmtime->tm_mon  = tmtime->tm_mon  + month;
			tmtime->tm_mday = tmtime->tm_mday + day;
			tmtime->tm_hour = tmtime->tm_hour + hour;
			tmtime->tm_min  = tmtime->tm_min  + min;
			tmtime->tm_sec  = tmtime->tm_sec  + sec;
			timestamp = mktime(tmtime);
			/* if the validity limit is possible */
			if (timestamp != -1) {
				/* answer */
				WPACKETW( 0) = 0x7951;
				WPACKETL( 2) = auth_dat[i].account_id;
				strncpy(WPACKETP(6), auth_dat[i].userid, 24);
				WPACKETL(30) = (unsigned long)timestamp;
				SENDPACKET(fd, 34);
				/* preparation of the string */
				memset(tmpstr, 0, sizeof(tmpstr));
				strftime(tmpstr, 20, date_format, localtime(&timestamp)); // 19 + NULL
				memset(tmpstr2, 0, sizeof(tmpstr2));
				strftime(tmpstr2, 20, date_format, localtime(&auth_dat[i].connect_until_time)); // 19 + NULL
				/* if it's not already the good validity limit */
				if (auth_dat[i].connect_until_time != timestamp) { /* validity limit is modified ? */
					write_log("'ladmin': Request to adjust a validity limit: ok (account: %s, %d (%s) + (%+d y %+d m %+d d %+d h %+d mn %+d s) -> new validity limit: %d (%s), ip: %s)" RETCODE,
					          auth_dat[i].userid, auth_dat[i].connect_until_time, (auth_dat[i].connect_until_time == 0 ? "unlimited" : tmpstr2), year, month, day, hour, min, sec, timestamp, (timestamp == 0 ? "unlimited" : tmpstr), ip);
					/* save validity limit */
					auth_dat[i].connect_until_time = timestamp;
					save_account(i, 1);
				} else {
					write_log("'ladmin': Request to adjust a validity limit: no adjustment (account: %s, validity limit: %d (%s), ip: %s)" RETCODE,
					          account_name, timestamp, ((timestamp == 0) ? "unlimited" : tmpstr), ip);
				}
			/* so, new validity limit is impossible */
			} else {
				/* answer */
				WPACKETW( 0) = 0x7951;
				WPACKETL( 2) = auth_dat[i].account_id;
				strncpy(WPACKETP(6), auth_dat[i].userid, 24);
				WPACKETL(30) = (unsigned long)auth_dat[i].connect_until_time;
				SENDPACKET(fd, 34);
				/* preparation of the string */
				memset(tmpstr, 0, sizeof(tmpstr));
				strftime(tmpstr, 20, date_format, localtime(&auth_dat[i].connect_until_time)); // 19 + NULL
				write_log("'ladmin': Request to adjust a validity limit: impossible to adjust (account: %s, %d (%s) + (%+d y %+d m %+d d %+d h %+d mn %+d s) -> ???, ip: %s)" RETCODE,
				          account_name, auth_dat[i].connect_until_time, (auth_dat[i].connect_until_time == 0 ? "unlimited" : tmpstr), year, month, day, hour, min, sec, ip);
			}
		}
	/* account is not found */
	} else {
		/* answer */
		WPACKETW( 0) = 0x7951;
		WPACKETL( 2) = -1;
		strncpy(WPACKETP(6), account_name, 24);
		WPACKETL(30) = 0;
		SENDPACKET(fd, 34);
		write_log("'ladmin': Request to adjust a validity limit: unknown account (account: %s, ip: %s)" RETCODE, account_name, ip);
	}

	return;
}

/*--------------------------------------------------------------
  ladmin function:
  0x7952 - Request to obtain account information (by the name)
--------------------------------------------------------------*/
static void ladmin_account_information(const int fd, const char* ip) {
	int i;
	char account_name[25];

	/* get parameters */
	memset(account_name, 0, sizeof(account_name));
	strncpy(account_name, RFIFOP(fd, 2), 24);
	remove_control_chars(account_name);

	/* search if account exists */
	i = search_account_index(account_name);
	if (i != -1) {
		write_log("'ladmin': Request to obtain account information (by the name): ok (account: %s, id: %d, ip: %s)" RETCODE, auth_dat[i].userid, auth_dat[i].account_id, ip);
		/* send answer */
		memset(WPACKETP(0), 0, 150);
		WPACKETW(  0) = 0x7953;
		WPACKETL(  2) = auth_dat[i].account_id;
		WPACKETB(  6) = auth_dat[i].level;
		strncpy(WPACKETP(  7), auth_dat[i].userid, 24);
		WPACKETB( 31) = auth_dat[i].sex;
		WPACKETL( 32) = auth_dat[i].logincount;
		WPACKETW( 36) = auth_dat[i].state;
		WPACKETB( 38) = check_online_player(auth_dat[i].account_id);
		WPACKETB( 39) = 0; // not used
		strncpy(WPACKETP( 40), auth_dat[i].error_message, 20); // 19 + NULL
		strncpy(WPACKETP( 60), auth_dat[i].lastlogin, 23);
		strncpy(WPACKETP( 84), auth_dat[i].last_ip, 16);
		strncpy(WPACKETP(100), auth_dat[i].email, 40);
		WPACKETL(140) = (unsigned long)auth_dat[i].connect_until_time;
		WPACKETL(144) = (unsigned long)auth_dat[i].ban_until_time;
		if (auth_dat[i].memo == NULL) {
			WPACKETW(148) = 0;
			SENDPACKET(fd, 150);
		} else {
			WPACKETW(148) = strlen(auth_dat[i].memo);
			if (auth_dat[i].memo[0])
				memcpy(WPACKETP(150), auth_dat[i].memo, strlen(auth_dat[i].memo));
			SENDPACKET(fd, 150 + strlen(auth_dat[i].memo));
		}
	} else {
		write_log("'ladmin': Request to obtain account information (by the name): unknown account (account: %s, ip: %s)" RETCODE, account_name, ip);
		/* send answer */
		memset(WPACKETP(0), 0, 150);
		WPACKETW(0) = 0x7953;
		WPACKETL(2) = -1;
		strncpy(WPACKETP(7), account_name, 24);
		SENDPACKET(fd, 150);
	}

	return;
}

/*------------------------------------------------------------
  ladmin function:
  0x7954 - Request to obtain account information (by the id)
------------------------------------------------------------*/
static void ladmin_account_information2(const int fd, const char* ip) {
	int i, account_id;

	/* get parameters */
	account_id = RFIFOL(fd,2);

	/* search if account exists */
	i = search_account_index2(account_id);
	if (i != -1) {
		write_log("'ladmin': Request to obtain account information (by the id): ok (account: %s, id: %d, ip: %s)" RETCODE, auth_dat[i].userid, auth_dat[i].account_id, ip);
		/* send answer */
		memset(WPACKETP(0), 0, 150);
		WPACKETW(  0) = 0x7953;
		WPACKETL(  2) = auth_dat[i].account_id;
		WPACKETB(  6) = auth_dat[i].level;
		strncpy(WPACKETP(  7), auth_dat[i].userid, 24);
		WPACKETB( 31) = auth_dat[i].sex;
		WPACKETL( 32) = auth_dat[i].logincount;
		WPACKETW( 36) = auth_dat[i].state;
		WPACKETB( 38) = check_online_player(auth_dat[i].account_id);
		WPACKETB( 39) = 0; // not used
		strncpy(WPACKETP( 40), auth_dat[i].error_message, 20); // 19 + NULL
		strncpy(WPACKETP( 60), auth_dat[i].lastlogin, 23);
		strncpy(WPACKETP( 84), auth_dat[i].last_ip, 16);
		strncpy(WPACKETP(100), auth_dat[i].email, 40);
		WPACKETL(140) = (unsigned long)auth_dat[i].connect_until_time;
		WPACKETL(144) = (unsigned long)auth_dat[i].ban_until_time;
		if (auth_dat[i].memo == NULL) {
			WPACKETW(148) = 0;
			SENDPACKET(fd, 150);
		} else {
			WPACKETW(148) = strlen(auth_dat[i].memo);
			if (auth_dat[i].memo[0])
				memcpy(WPACKETP(150), auth_dat[i].memo, strlen(auth_dat[i].memo));
			SENDPACKET(fd, 150 + strlen(auth_dat[i].memo));
		}
	} else {
		write_log("'ladmin': Request to obtain account information (by the id): unknown account (id: %d, ip: %s)" RETCODE, account_id, ip);
		/* send answer */
		memset(WPACKETP(0), 0, 150);
		WPACKETW(0) = 0x7953;
		WPACKETL(2) = account_id;
		SENDPACKET(fd, 150);
	}

	return;
}

/*------------------------------------------
  ladmin function:
  0x7955 - Request to add a text to a memo
------------------------------------------*/
static void ladmin_add_memo(const int fd, const char* ip) {
	int i;
	char account_name[25];
	unsigned int size_of_memo;
	char * added_memo;

	/* get parameters */
	memset(account_name, 0, sizeof(account_name));
	strncpy(account_name, RFIFOP(fd, 4), 24);
	remove_control_chars(account_name);
	size_of_memo = RFIFOW(fd, 2) - 28;
	/* limit size of memo to 60000 */
	if (size_of_memo > 60000)
		size_of_memo = 60000;
	CALLOC(added_memo, char, size_of_memo + 1); // + NULL
	strncpy(added_memo, RFIFOP(fd, 28), size_of_memo);
	remove_control_chars(added_memo);
	size_of_memo = strlen(added_memo); /* recalculate to have correct length */

	/* search if account exists */
	i = search_account_index(account_name);
	if (i != -1) {
		/* if memo to add is void */
		if (size_of_memo == 0) {
			/* send answer */
			WPACKETW(0) = 0x7956;
			WPACKETL(2) = -1;
			strncpy(WPACKETP(6), account_name, 24);
			SENDPACKET(fd, 30);
			write_log("'ladmin': Request to add a text to a memo: void text to add (account: %s, ip: %s)" RETCODE, account_name, ip);
		} else {
			/* send answer */
			WPACKETW(0) = 0x7956;
			WPACKETL(2) = auth_dat[i].account_id;
			strncpy(WPACKETP(6), auth_dat[i].userid, 24);
			SENDPACKET(fd, 30);
			write_log("'ladmin': Request to add a text to a memo: ok (account: %s, added text: %s, ip: %s)" RETCODE, auth_dat[i].userid, added_memo, ip);
			/* add text to memo */
			add_text_in_memo(i, added_memo);
			/* save memo */
			save_account(i, 1);
		}
	} else {
		/* send answer */
		WPACKETW(0) = 0x7956;
		WPACKETL(2) = -1;
		strncpy(WPACKETP(6), account_name, 24);
		SENDPACKET(fd, 30);
		if (size_of_memo > 0)
			write_log("'ladmin': Request to add a text to a memo: unknown account (account: %s, received text: %s, ip: %s)" RETCODE,
			          account_name, added_memo, ip);
		else
			write_log("'ladmin': Request to add a text to a memo: unknown account (account: %s, received text: (void), ip: %s)" RETCODE,
			          account_name, ip);
	}
	FREE(added_memo);

	return;
}

/*--------------
  Packet table
---------------*/
static struct parse_func_table {
	int packet;
	signed short length; /* signed, because -1 */
	void (*proc)(const int fd, const char * ip);
} parse_func_table[] = {
	/* general functions */
	{ 0x7530,   2, ladmin_packet_version },
	{ 0x7532,   2, ladmin_end_of_connection },
	{ 0x7533,   2, ladmin_packet_uptime },
	{ 0x7535,   2, ladmin_packet_version },
	/* pure ladmin functions*/
	{ 0x791f,   2, ladmin_ready }, /* no answer */
	{ 0x7920,  11, ladmin_list_of_accounts },
	{ 0x7930,  91, ladmin_account_creation },
	{ 0x7932,  26, ladmin_account_deletion },
	{ 0x7934,  50, ladmin_change_password },
	{ 0x7936,  50, ladmin_change_state },
	{ 0x7938,   2, ladmin_list_of_servers },
	{ 0x793a,  50, ladmin_check_password },
	{ 0x793c,  27, ladmin_change_sex },
	{ 0x793e,  27, ladmin_change_level },
	{ 0x7940,  66, ladmin_change_mail },
	{ 0x7942,  -1, ladmin_change_memo },
	{ 0x7944,  26, ladmin_search_account_id },
	{ 0x7946,   6, ladmin_search_account_name },
	{ 0x7948,  30, ladmin_set_validity_limit },
	{ 0x794a,  30, ladmin_set_ban_limit },
	{ 0x794c,  38, ladmin_adjust_ban_limit },
	{ 0x794e,  -1, ladmin_broadcast },
	{ 0x7950,  38, ladmin_adjust_validity_limit },
	{ 0x7952,  26, ladmin_account_information }, /* by name */
	{ 0x7954,   6, ladmin_account_information2 }, /* by id */
	{ 0x7955,  -1, ladmin_add_memo },
	/* add functions before this line */
	{      0,   0, NULL },
};

/*----------------------------------------
  Packet parsing for administation login
----------------------------------------*/
int parse_admin(int fd) {
	int i, j, packet_len;
	unsigned char *p = (unsigned char *) &session[fd]->client_addr.sin_addr;
	char ip[16];

	sprintf(ip, "%d.%d.%d.%d", p[0], p[1], p[2], p[3]);

	if (session[fd]->eof) {
#ifdef __WIN32
		shutdown(fd, SD_BOTH);
		closesocket(fd);
#else
		close(fd);
#endif
		delete_session(fd);
		printf("Remote administration has disconnected (session #%d).\n", fd);
		write_log("'ladmin': Disconnection (session #%d, ip: %s)." RETCODE, fd, ip);
		return 0;
	}

	while(RFIFOREST(fd) >= 2 && !session[fd]->eof) {

		if (display_parse_admin)
			printf("parse_admin: session #%d, packet: 0x%x (total to read: %d).\n", fd, RFIFOW(fd,0), RFIFOREST(fd));

		i = 0;
		while(parse_func_table[i].packet != 0) {
			if (parse_func_table[i].packet == RFIFOW(fd,0)) {
				packet_len = parse_func_table[i].length;
				if (packet_len == -1) {
					if (RFIFOREST(fd) < 4)
						return 0;
					packet_len = RFIFOW(fd,2);
					if (packet_len < 4) {
						session[fd]->eof = 1;
						return 0;
					}
				}
				if (RFIFOREST(fd) < packet_len)
					return 0;
				parse_func_table[i].proc(fd, ip);
				RFIFOSKIP(fd,packet_len);
				if (session[fd]->eof)
					return 0;
				break;
			}
			i++;
		}

		/* if packet was not found */
		if (parse_func_table[i].packet == 0) {
			FILE *logfp;
			char tmpstr[24];
			struct timeval tv;
			time_t now;
			logfp = fopen(login_log_unknown_packets_filename, "a");
			if (logfp) {
				gettimeofday(&tv, NULL);
				now = time(NULL);
				memset(tmpstr, 0, sizeof(tmpstr));
				strftime(tmpstr, 20, date_format, localtime(&now)); // 19 + NULL
				fprintf(logfp, "%s.%03d: receiving of an unknown packet -> disconnection" RETCODE, tmpstr, (int)tv.tv_usec / 1000);
				fprintf(logfp, "parse_admin: connection #%d (ip: %s), packet: 0x%x (with being read: %d)." RETCODE, fd, ip, RFIFOW(fd,0), RFIFOREST(fd));
				fprintf(logfp, "Detail (in hex):" RETCODE);
				fprintf(logfp, "---- 00-01-02-03-04-05-06-07  08-09-0A-0B-0C-0D-0E-0F" RETCODE);
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
			write_log("'ladmin': End of connection, unknown packet (ip: %s)" RETCODE, ip);
			session[fd]->eof = 1;
			printf("Remote administration has been disconnected (unknown packet).\n");
			return 0;
		}

	}

	return 0;
}
