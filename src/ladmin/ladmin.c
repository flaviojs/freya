// Copyright (c) Freya Development Team - Licensed under GNU GPL
// For more information, see License.txt in the main folder

#include <config.h>

#include <sys/types.h>
#ifdef __WIN32
#define __USE_W32_SOCKETS
#include <windows.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h> // gettimeofday
#include <sys/ioctl.h>
#include <arpa/inet.h> // inet_addr
#include <netdb.h> // gethostbyname
#endif
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h> // close
#include <signal.h>
#include <fcntl.h>
#include <string.h> // str*
#include <stdarg.h> // va_list
#include <ctype.h> // tolower, toupper

#include "../common/core.h"
#include "../common/timer.h"
#include "../common/socket.h"
#include "ladmin.h"
#include "../common/version.h"
#include <mmo.h>

#ifdef PASSWORDENC
#include "../common/md5calc.h"
#endif

#ifdef MEMWATCH
#include "memwatch.h"
#endif

//-------------------------------INSTRUCTIONS------------------------------
// Set the variables below:
//   IP of the login server.
//   Port where the login-server listens incoming packets.
//   Password of administration (same of config_freya.conf).
//   Displayed language of the sofware (if not correct, english is used).
// IMPORTANT:
//   Be sure that you authorize remote administration in login-server
//   (see login_freya.conf, 'admin_state' parameter)
//-------------------------------------------------------------------------
char loginserverip[16] = "127.0.0.1";        // IP of login-server
int loginserverport = 6900;                  // Port of login-server
char loginserveradminpassword[25] = "admin"; // Administration password
#ifdef PASSWORDENC
int passenc = 2;                             // Encoding type of the password
#else
int passenc = 0;                             // Encoding type of the password
#endif
char defaultlanguage = 'E';                  // Default language (F: Français/E: English)
                                             // (if it's not 'F', default is English)
char ladmin_log_filename[1024] = "log/ladmin.log";
static unsigned char log_file_date = 3; /* year + month (example: log/login-2006-12.log) */
char date_format[32] = "%Y-%m-%d %H:%M:%S";
//-------------------------------------------------------------------------
//  LIST of COMMANDs that you can type at the prompt:
//    To use these commands you can only type only the first letters.
//    You must type a minimum of letters (you can not type 'a',
//      because ladmin doesn't know if it's for 'aide' or for 'add')
//    <Example> q <= quit, li <= list, pass <= passwd, etc.
//
//  Note: every time you must give a account_name, you can use "" or '' (spaces can be included)
//
//  aide/help/?
//    Display the description of the commands
//  aide/help/? [command]
//    Display the description of the specified command
//
//  add <account_name> <sex> <password>
//    Create an account with the default email (a@a.com).
//    Concerning the sex, only the first letter is used (F or M).
//    The e-mail is set to a@a.com (default e-mail). It's like to have no e-mail.
//    When the password is omitted, the input is done without displaying of the pressed keys.
//    <example> add testname Male testpass
//
//  ban/banish yyyy/mm/dd hh:mm:ss <account name>
//    Changes the final date of a banishment of an account.
//    Like banset, but <account name> is at end.
//
//  banadd <account_name> <modifier>
//    Adds or substracts time from the final date of a banishment of an account.
//    Modifier is done as follows:
//      Adjustment value (-1, 1, +1, etc...)
//      Modified element:
//        a or y: year
//        m:  month
//        j or d: day
//        h:  hour
//        mn: minute
//        s:  second
//    <example> banadd testname +1m-2mn1s-6y
//              this example adds 1 month and 1 second, and substracts 2 minutes and 6 years at the same time.
//  NOTE: If you modify the final date of a non-banished account,
//        you fix the final date to (actual time +- adjustments)
//
//  banset <account_name> yyyy/mm/dd [hh:mm:ss]
//    Changes the final date of a banishment of an account.
//    Default time [hh:mm:ss]: 23:59:59.
//  banset <account_name> 0
//    Set a non-banished account (0 = unbanished).
//
//  block <account name>
//    Set state 5 (You have been blocked by the GM Team) to an account.
//    Like state <account name> 5.
//
//  check <account_name> <password>
//    Check the validity of a password for an account
//    NOTE: Server will never sends back a password.
//          It's the only method you have to know if a password is correct.
//          The other method is to have a ('physical') access to the accounts file.
//
//  create <account_name> <sex> <email> <password>
//    Like the 'add' command, but with e-mail moreover.
//    <example> create testname Male my@mail.com testpass
//
//  del <account name>
//    Remove an account.
//    This order requires confirmation. After confirmation, the account is deleted.
//
//  email <account_name> <email>
//    Modify the e-mail of an account.
//
//  getcount
//    Give the number of players online on all char-servers.
//
//  gm <account_name> [GM_level]
//    Modify the GM level of an account.
//    Default value remove GM level (GM level = 0).
//    <example> gm testname 80
//
//  id <account name>
//    Give the id of an account.
//
//  info <account_id>
//    Display complete information of an account.
//
//  kami <message>
//    Sends a broadcast message on all map-server (in yellow).
//  kamib <message>
//    Sends a broadcast message on all map-server (in blue).
//
//  language <language>
//    Change the language of displaying.
//
//  list/ls [start_id [end_id]]
//    Display a list of accounts.
//    'start_id', 'end_id': indicate end and start identifiers.
//    Research by name is not possible with this command.
//    <example> list 10 9999999
//
//  listBan/lsBan [start_id [end_id]]
//    Like list/ls, but only for accounts with state or banished
//
//  listGM/lsGM [start_id [end_id]]
//    Like list/ls, but only for GM accounts
//
//  listOK/lsOK [start_id [end_id]]
//    Like list/ls, but only for accounts without state and not banished
//
//  listOnline/lsOnline [start_id [end_id]]
//    Like list/ls, but only for online accounts
//
//  memo <account_name> <memo>
//    Modify the memo of an account.
//    'memo': it can have until 60000 characters (with spaces or not).
//
//  memoadd <account_name> <memo text>
//    Add <memo text> to the memo of an account.
//    The 'memo' can have until 60000 characters (with spaces or not).
//
//  name <account_id>
//    Give the name of an account.
//
//  passwd <account_name> <new_password>
//    Change the password of an account.
//    When new password is omitted, the input is done without displaying of the pressed keys.
//
//  quit/end/exit
//    End of the program of administration
//
//  search <expression>
//    Seek accounts.
//    Displays the accounts whose names correspond.
//  search -r/-e/--expr/--regex <expression>
//    Seek accounts by regular expression.
//    Displays the accounts whose names correspond.
//
//  sex <account_name> <sex>
//    Modify the sex of an account.
//    <example> sex testname Male
//
//  state <account_name> <new_state> <error_message_#7>
//    Change the state of an account.
//    'new_state': state is the state of the packet 0x006a + 1. The possibilities are:
//                 0 = Account ok            6 = Your Game's EXE file is not the latest version
//                 1 = Unregistered ID       7 = You are Prohibited to log in until %s
//                 2 = Incorrect Password    8 = Server is jammed due to over populated
//                 3 = This ID is expired    9 = No MSG
//                 4 = Rejected from Server  100 = This ID has been totally erased
//                 5 = You have been blocked by the GM Team
//                 all other values are 'No MSG', then use state 9 please.
//    'error_message_#7': message of the code error 6 = Your are Prohibited to log in until %s (packet 0x006a)
//
//  timeadd <account_name> <modifier>
//    Adds or substracts time from the validity limit of an account.
//    Modifier is done as follows:
//      Adjustment value (-1, 1, +1, etc...)
//      Modified element:
//        a or y: year
//        m:  month
//        j or d: day
//        h:  hour
//        mn: minute
//        s:  second
//    <example> timeadd testname +1m-2mn1s-6y
//              this example adds 1 month and 1 second, and substracts 2 minutes and 6 years at the same time.
//  NOTE: You can not modify a unlimited validity limit.
//        If you want modify it, you want probably create a limited validity limit.
//        So, at first, you must set the validity limit to a date/time.
//
//  timeset <account_name> yyyy/mm/dd [hh:mm:ss]
//    Changes the validity limit of an account.
//    Default time [hh:mm:ss]: 23:59:59.
//  timeset <account_name> 0
//    Gives an unlimited validity limit (0 = unlimited).
//
//  unban/unbanish <account name>
//    Unban an account.
//    Like banset <account name> 0.
//
//  unblock <account name>
//    Set state 0 (Account ok) to an account.
//    Like state <account name> 0.
//
//  uptime
//    Display the uptime of the login-server.
//
//  version
//    Display the version of the login-server.
//
//  who <account name>
//    Displays complete information of an account.
//
//-------------------------------------------------------------------------
int login_fd = -1;
int login_ip;
unsigned char init_flag = 0; // flag to know if init is terminated (0: no, 1: yes)
unsigned char bytes_to_read = 0; // flag to know if we waiting bytes from login-server
char command[1024];
char parameters[1024];
int list_first, list_last, list_type, list_count; // parameter to display a list of accounts
int already_exit_function = 0; // sometimes, the exit function is called twice... so, don't log twice the message

//------------------------------
// Writing function of logs file
//------------------------------
static void ladmin_log(char *fmt, ...) { // not inline, called too often
	FILE *logfp;
	va_list ap;
	struct timeval tv;
	time_t now;
	char tmpstr[2048];
	char log_filename_to_use[sizeof(ladmin_log_filename) + 64];

	va_start(ap, fmt);

	// get time for file name and logs
	gettimeofday(&tv, NULL);
	now = time(NULL);

	// create file name
	memset(log_filename_to_use, 0, sizeof(log_filename_to_use));
	if (log_file_date == 0) {
		strcpy(log_filename_to_use, ladmin_log_filename);
	} else {
		char* last_point;
		char* last_slash; // to avoid name like ../log_file_name_without_point
		// search position of '.'
		last_point = strrchr(ladmin_log_filename, '.');
		last_slash = strrchr(ladmin_log_filename, '/');
		if (last_point == NULL || (last_slash != NULL && last_slash > last_point))
			last_point = ladmin_log_filename + strlen(ladmin_log_filename);
		strncpy(log_filename_to_use, ladmin_log_filename, last_point - ladmin_log_filename);
		switch (log_file_date) {
		case 1:
			strftime(log_filename_to_use + strlen(log_filename_to_use), 63, "-%Y", localtime(&now));
			strcat(log_filename_to_use, last_point);
			break;
		case 2:
			strftime(log_filename_to_use + strlen(log_filename_to_use), 63, "-%m", localtime(&now));
			strcat(log_filename_to_use, last_point);
			break;
		case 3:
			strftime(log_filename_to_use + strlen(log_filename_to_use), 63, "-%Y-%m", localtime(&now));
			strcat(log_filename_to_use, last_point);
			break;
		case 4:
			strftime(log_filename_to_use + strlen(log_filename_to_use), 63, "-%Y-%m-%d", localtime(&now));
			strcat(log_filename_to_use, last_point);
			break;
		default: // case 0: 
			strcpy(log_filename_to_use, ladmin_log_filename);
			break;
		}
	}

	logfp = fopen(log_filename_to_use, "a");
	if (logfp) {
		if (fmt[0] == '\0') // jump a line if no message
			fprintf(logfp, RETCODE);
		else {
			memset(tmpstr, 0, sizeof(tmpstr));
			strftime(tmpstr, 20, date_format, localtime(&now));
			sprintf(tmpstr + strlen(tmpstr), ".%03d: %s", (int)tv.tv_usec / 1000, fmt);
			vfprintf(logfp, tmpstr, ap);
		}
		fclose(logfp);
	}

	va_end(ap);

	return;
}

//---------------------------------------------
// Function to return ordonal text of a number.
//---------------------------------------------
static char* makeordinal(int number) {
	if (defaultlanguage == 'F') {
		if (number == 0)
			return "";
		else if (number == 1)
			return "er";
		else
			return "ème";
	} else {
		if ((number % 10) < 4 && (number % 10) != 0 && (number < 10 || number > 20)) {
			if ((number % 10) == 1)
				return "st";
			else if ((number % 10) == 2)
				return "nd";
			else
				return "rd";
		} else {
			return "th";
		}
	}

	return "";
}

//-----------------------------------------------------------------------------------------
// Function to test of the validity of an account name (return 0 if incorrect, and 1 if ok)
//-----------------------------------------------------------------------------------------
static int verify_accountname(char* account_name) {
	int i;

	for(i = 0; account_name[i]; i++) {
		if (iscntrl((int)account_name[i])) { // if (account_name[i] < 32) {
			if (defaultlanguage == 'F') {
				printf("Caractère interdit trouvé dans le nom du compte (%d%s caractère).\n", i+1, makeordinal(i+1));
				ladmin_log("Caractère interdit trouvé dans le nom du compte (%d%s caractère)." RETCODE, i+1, makeordinal(i+1));
			} else {
				printf("Illegal character found in the account name (%d%s character).\n", i+1, makeordinal(i+1));
				ladmin_log("Illegal character found in the account name (%d%s character)." RETCODE, i+1, makeordinal(i+1));
			}
			return 0;
		}
	}

	if (strlen(account_name) < 4) {
		if (defaultlanguage == 'F') {
			printf("Nom du compte trop court. Entrez un nom de compte de 4-23 caractères.\n");
			ladmin_log("Nom du compte trop court. Entrez un nom de compte de 4-23 caractères." RETCODE);
		} else {
			printf("Account name is too short. Please input an account name of 4-23 bytes.\n");
			ladmin_log("Account name is too short. Please input an account name of 4-23 bytes." RETCODE);
		}
		return 0;
	}

	if (strlen(account_name) > 23) {
		if (defaultlanguage == 'F') {
			printf("Nom du compte trop long. Entrez un nom de compte de 4-23 caractères.\n");
			ladmin_log("Nom du compte trop long. Entrez un nom de compte de 4-23 caractères." RETCODE);
		} else {
			printf("Account name is too long. Please input an account name of 4-23 bytes.\n");
			ladmin_log("Account name is too long. Please input an account name of 4-23 bytes." RETCODE);
		}
		return 0;
	}

	return 1;
}

//----------------------------------
// Sub-function: Input of a password
//----------------------------------
static int typepasswd(char * password) {
	char password1[1023], password2[1023];
	int letter;
	int i;

	if (defaultlanguage == 'F') {
		ladmin_log("Aucun mot de passe n'a été donné. Demande d'un mot de passe." RETCODE);
	} else {
		ladmin_log("No password was given. Request to obtain a password." RETCODE);
	}

	memset(password1, 0, sizeof(password1));
	memset(password2, 0, sizeof(password2));
	if (defaultlanguage == 'F')
		printf(CL_CYAN " Entrez le mot de passe > " CL_DARK_GREEN CL_BG_GREEN);
	else
		printf(CL_CYAN " Type the password > " CL_DARK_GREEN CL_BG_GREEN);
		i = 0;
		while ((letter = getchar()) != '\n')
			password1[i++] = letter;
	if (defaultlanguage == 'F')
		printf(CL_RESET CL_CYAN " Ré-entrez le mot de passe > " CL_DARK_GREEN CL_BG_GREEN);
	else
		printf(CL_RESET CL_CYAN " Verify the password > " CL_DARK_GREEN CL_BG_GREEN);
		i = 0;
		while ((letter = getchar()) != '\n')
			password2[i++] = letter;

#ifndef __WIN32
	printf(CL_RESET);
#endif
	fflush(stdout);
	fflush(stdin);

	if (strcmp(password1, password2) != 0) {
		if (defaultlanguage == 'F') {
			printf("Erreur de vérification du mot de passe: Saisissez le même mot de passe svp.\n");
			ladmin_log("Erreur de vérification du mot de passe: Saisissez le même mot de passe svp." RETCODE);
			ladmin_log("  Premier mot de passe: %s, second mot de passe: %s." RETCODE, password1, password2);
		} else {
			printf("Password verification failed. Please input same password.\n");
			ladmin_log("Password verification failed. Please input same password." RETCODE);
			ladmin_log("  First password: %s, second password: %s." RETCODE, password1, password2);
		}
		return 0;
	}
	if (defaultlanguage == 'F') {
		ladmin_log("Mot de passe saisi: %s." RETCODE, password1);
	} else {
		ladmin_log("Typed password: %s." RETCODE, password1);
	}
	strcpy(password, password1);

	return 1;
}

//------------------------------------------------------------------------------------
// Sub-function: Test of the validity of password (return 0 if incorrect, and 1 if ok)
//------------------------------------------------------------------------------------
static int verify_password(char * password) {
	int i;

	for(i = 0; password[i]; i++) {
		if (iscntrl((int)password[i])) { // if (password[i] < 32) {
			if (defaultlanguage == 'F') {
				printf("Caractère interdit trouvé dans le mot de passe (%d%s caractère).\n", i+1, makeordinal(i+1));
				ladmin_log("Caractère interdit trouvé dans le nom du compte (%d%s caractère)." RETCODE, i+1, makeordinal(i+1));
			} else {
				printf("Illegal character found in the password (%d%s character).\n", i+1, makeordinal(i+1));
				ladmin_log("Illegal character found in the password (%d%s character)." RETCODE, i+1, makeordinal(i+1));
			}
			return 0;
		}
	}

	if (strlen(password) < 4) {
		if (defaultlanguage == 'F') {
			printf("Nom du compte trop court. Entrez un nom de compte de 4-23 caractères.\n");
			ladmin_log("Nom du compte trop court. Entrez un nom de compte de 4-23 caractères." RETCODE);
		} else {
			printf("Account name is too short. Please input an account name of 4-23 bytes.\n");
			ladmin_log("Account name is too short. Please input an account name of 4-23 bytes." RETCODE);
		}
		return 0;
	}

	if (strlen(password) > 24) {
		if (defaultlanguage == 'F') {
			printf("Mot de passe trop long. Entrez un mot de passe de 4-24 caractères.\n");
			ladmin_log("Mot de passe trop long. Entrez un mot de passe de 4-24 caractères." RETCODE);
		} else {
			printf("Password is too long. Please input a password of 4-24 bytes.\n");
			ladmin_log("Password is too long. Please input a password of 4-24 bytes." RETCODE);
		}
		return 0;
	}

	return 1;
}

//------------------------------------------------------------------
// Sub-function: Check the name of a command (return complete name)
//-----------------------------------------------------------------
static void check_command(char * cmd_name) { // not inline, called too often
// help
	if (strncmp(cmd_name, "aide", 2) == 0 && strncmp(cmd_name, "aide", strlen(cmd_name)) == 0) // not 1 letter command: 'aide' or 'add'?
		strcpy(cmd_name, "aide");
	else if (strncmp(cmd_name, "help", 1) == 0 && strncmp(cmd_name, "help", strlen(cmd_name)) == 0)
		strcpy(cmd_name, "help");
// general commands
	else if (strncmp(cmd_name, "add", 2) == 0 && strncmp(cmd_name, "add", strlen(cmd_name)) == 0) // not 1 letter command: 'aide' or 'add'?
		strcpy(cmd_name, "add");
	else if ((strncmp(cmd_name, "ban", 3) == 0 && strncmp(cmd_name, "ban", strlen(cmd_name)) == 0) ||
	         (strncmp(cmd_name, "banish", 4) == 0 && strncmp(cmd_name, "banish", strlen(cmd_name)) == 0))
		strcpy(cmd_name, "ban");
	else if ((strncmp(cmd_name, "banadd", 4) == 0 && strncmp(cmd_name, "banadd", strlen(cmd_name)) == 0) || // not 1 letter command: 'ba' or 'bs'? 'banadd' or 'banset' ?
	         strcmp(cmd_name,   "ba") == 0)
		strcpy(cmd_name, "banadd");
	else if ((strncmp(cmd_name, "banset", 4) == 0 && strncmp(cmd_name, "banset", strlen(cmd_name)) == 0) || // not 1 letter command: 'ba' or 'bs'? 'banadd' or 'banset' ?
	         strcmp(cmd_name,   "bs") == 0)
		strcpy(cmd_name, "banset");
	else if (strncmp(cmd_name, "block", 2) == 0 && strncmp(cmd_name, "block", strlen(cmd_name)) == 0)
		strcpy(cmd_name, "block");
	else if (strncmp(cmd_name, "check", 2) == 0 && strncmp(cmd_name, "check", strlen(cmd_name)) == 0) // not 1 letter command: 'check' or 'create'?
		strcpy(cmd_name, "check");
	else if (strncmp(cmd_name, "create", 2) == 0 && strncmp(cmd_name, "create", strlen(cmd_name)) == 0) // not 1 letter command: 'check' or 'create'?
		strcpy(cmd_name, "create");
	else if (strncmp(cmd_name, "delete", 1) == 0 && strncmp(cmd_name, "delete", strlen(cmd_name)) == 0)
		strcpy(cmd_name, "delete");
	else if ((strncmp(cmd_name, "email", 2) == 0 && strncmp(cmd_name, "email", strlen(cmd_name)) == 0) || // not 1 letter command: 'email', 'end' or 'exit'?
	         (strncmp(cmd_name, "e-mail", 2) == 0 && strncmp(cmd_name, "e-mail", strlen(cmd_name)) == 0))
		strcpy(cmd_name, "email");
	else if (strncmp(cmd_name, "getcount", 2) == 0 && strncmp(cmd_name, "getcount", strlen(cmd_name)) == 0) // not 1 letter command: 'getcount' or 'gm'?
		strcpy(cmd_name, "getcount");
//	else if (strncmp(cmd_name, "gm", 2) == 0 && strncmp(cmd_name, "gm", strlen(cmd_name)) == 0) // not 1 letter command: 'getcount' or 'gm'?
//		strcpy(cmd_name, "gm");
//	else if (strncmp(cmd_name, "id", 2) == 0 && strncmp(cmd_name, "id", strlen(cmd_name)) == 0) // not 1 letter command: 'id' or 'info'?
//		strcpy(cmd_name, "id");
	else if (strncmp(cmd_name, "info", 2) == 0 && strncmp(cmd_name, "info", strlen(cmd_name)) == 0) // not 1 letter command: 'id' or 'info'?
		strcpy(cmd_name, "info");
//	else if (strncmp(cmd_name, "kami", 4) == 0 && strncmp(cmd_name, "kami", strlen(cmd_name)) == 0) // only all letters command: 'kami' or 'kamib'?
//		strcpy(cmd_name, "kami");
//	else if (strncmp(cmd_name, "kamib", 5) == 0 && strncmp(cmd_name, "kamib", strlen(cmd_name)) == 0) // only all letters command: 'kami' or 'kamib'?
//		strcpy(cmd_name, "kamib");
	else if ((strncmp(cmd_name, "language", 2) == 0 && strncmp(cmd_name, "language", strlen(cmd_name)) == 0)) // not 1 letter command: 'language' or 'list'?
		strcpy(cmd_name, "language");
	else if ((strncmp(cmd_name, "list", 2) == 0 && strncmp(cmd_name, "list", strlen(cmd_name)) == 0) || // 'list' is default list command // not 1 letter command: 'language' or 'list'?
	         strcmp(cmd_name,   "ls") == 0)
		strcpy(cmd_name, "list");
	else if ((strncmp(cmd_name, "listban", 5) == 0 && strncmp(cmd_name, "listban", strlen(cmd_name)) == 0) ||
	         (strncmp(cmd_name, "lsban", 3) == 0 && strncmp(cmd_name, "lsban", strlen(cmd_name)) == 0) ||
	         strcmp(cmd_name, "lb") == 0)
		strcpy(cmd_name, "listban");
	else if ((strncmp(cmd_name, "listgm", 5) == 0 && strncmp(cmd_name, "listgm", strlen(cmd_name)) == 0) ||
	         (strncmp(cmd_name, "lsgm", 3) == 0 && strncmp(cmd_name, "lsgm", strlen(cmd_name)) == 0) ||
	         strcmp(cmd_name,  "lg") == 0)
		strcpy(cmd_name, "listgm");
	else if ((strncmp(cmd_name, "listok", 6) == 0 && strncmp(cmd_name, "listok", strlen(cmd_name)) == 0) ||
	         (strncmp(cmd_name, "lsok", 4) == 0 && strncmp(cmd_name, "lsok", strlen(cmd_name)) == 0))
		strcpy(cmd_name, "listok");
	else if ((strncmp(cmd_name, "listonline", 6) == 0 && strncmp(cmd_name, "listonline", strlen(cmd_name)) == 0) ||
	         (strncmp(cmd_name, "lsonline", 4) == 0 && strncmp(cmd_name, "lsonline", strlen(cmd_name)) == 0))
		strcpy(cmd_name, "listonline");
	else if (strcmp(cmd_name, "memo") == 0)
		strcpy(cmd_name, "memo");
	else if (strncmp(cmd_name, "memoadd", 5) == 0 && strncmp(cmd_name, "memoadd", strlen(cmd_name)) == 0)
		strcpy(cmd_name, "memoadd");
	else if (strncmp(cmd_name, "name", 1) == 0 && strncmp(cmd_name, "name", strlen(cmd_name)) == 0)
		strcpy(cmd_name, "name");
	else if ((strncmp(cmd_name, "password", 1) == 0 && strncmp(cmd_name, "password", strlen(cmd_name)) == 0) ||
	         strcmp(cmd_name, "passwd") == 0)
		strcpy(cmd_name, "password");
	else if (strncmp(cmd_name, "search", 3) == 0 && strncmp(cmd_name, "search", strlen(cmd_name)) == 0) // not 1 letter command: 'search', 'state' or 'sex'?
		strcpy(cmd_name, "search"); // not 2 letters command: 'search' or 'sex'?
//	else if (strncmp(cmd_name, "sex", 3) == 0 && strncmp(cmd_name, "sex", strlen(cmd_name)) == 0) // not 1 letter command: 'search', 'state' or 'sex'?
//		strcpy(cmd_name, "sex"); // not 2 letters cmd_name: 'search' or 'sex'?
	else if (strncmp(cmd_name, "state", 2) == 0 && strncmp(cmd_name, "state", strlen(cmd_name)) == 0) // not 1 letter command: 'search', 'state' or 'sex'?
		strcpy(cmd_name, "state");
	else if ((strncmp(cmd_name, "timeadd", 5) == 0 && strncmp(cmd_name, "timeadd", strlen(cmd_name)) == 0) || // not 1 letter command: 'ta' or 'ts'? 'timeadd' or 'timeset'?
	         strcmp(cmd_name,  "ta") == 0)
		strcpy(cmd_name, "timeadd");
	else if ((strncmp(cmd_name, "timeset", 5) == 0 && strncmp(cmd_name, "timeset", strlen(cmd_name)) == 0) || // not 1 letter command: 'ta' or 'ts'? 'timeadd' or 'timeset'?
	         strcmp(cmd_name,  "ts") == 0)
		strcpy(cmd_name, "timeset");
	else if ((strncmp(cmd_name, "unban", 5) == 0 && strncmp(cmd_name, "unban", strlen(cmd_name)) == 0) ||
	         (strncmp(cmd_name, "unbanish", 4) == 0 && strncmp(cmd_name, "unbanish", strlen(cmd_name)) == 0))
		strcpy(cmd_name, "unban");
	else if (strncmp(cmd_name, "unblock", 4) == 0 && strncmp(cmd_name, "unblock", strlen(cmd_name)) == 0)
		strcpy(cmd_name, "unblock");
	else if (strncmp(cmd_name, "uptime", 2) == 0 && strncmp(cmd_name, "uptime", strlen(cmd_name)) == 0)
		strcpy(cmd_name, "uptime");
	else if (strncmp(cmd_name, "version", 1) == 0 && strncmp(cmd_name, "version", strlen(cmd_name)) == 0)
		strcpy(cmd_name, "version");
	else if (strncmp(cmd_name, "who", 1) == 0 && strncmp(cmd_name, "who", strlen(cmd_name)) == 0)
		strcpy(cmd_name, "who");
// quit
	else if (strncmp(cmd_name, "quit", 1) == 0 && strncmp(cmd_name, "quit", strlen(cmd_name)) == 0)
		strcpy(cmd_name, "quit");
	else if (strncmp(cmd_name, "exit", 2) == 0 && strncmp(cmd_name, "exit", strlen(cmd_name)) == 0) // not 1 letter command: 'email', 'end' or 'exit'?
		strcpy(cmd_name, "exit");
	else if (strncmp(cmd_name, "end", 2) == 0 && strncmp(cmd_name, "end", strlen(cmd_name)) == 0) // not 1 letter command: 'email', 'end' or 'exit'?
		strcpy(cmd_name, "end");

	return;
}

//-----------------------------------------
// Sub-function: Display commands of ladmin
//-----------------------------------------
static void display_help(char* param, int language) { // not inline, called too often
	char cmd_name[1023];
	int i;

	memset(cmd_name, 0, sizeof(cmd_name));

	if (sscanf(param, "%s ", cmd_name) < 1 || cmd_name[0] == '\0')
		strcpy(cmd_name, ""); // any value that is not a command

	if (cmd_name[0] == '?') {
		if (defaultlanguage == 'F')
			strcpy(cmd_name, "aide");
		else
			strcpy(cmd_name, "help");
	}

	// lowercase for command
	for (i = 0; cmd_name[i]; i++)
		cmd_name[i] = tolower((unsigned char)cmd_name[i]); // tolower needs unsigned char

	// Analyse of the command
	check_command(cmd_name); // give complete name to the command

	if (defaultlanguage == 'F') {
		ladmin_log("Affichage des commandes ou d'une commande." RETCODE);
	} else {
		ladmin_log("Displaying of the commands or a command." RETCODE);
	}

	if (language == 1) {
		if (strcmp(cmd_name, "aide") == 0) {
			printf("aide/help/?\n");
			printf("  Affiche la description des commandes.\n");
			printf("aide/help/? [commande]\n");
			printf("  Affiche la description de la commande specifiée.\n");
		} else if (strcmp(cmd_name, "help") == 0) {
			printf("aide/help/?\n");
			printf("  Display the description of the commands.\n");
			printf("aide/help/? [command]\n");
			printf("  Display the description of the specified command.\n");
// general commands
		} else if (strcmp(cmd_name, "add") == 0) {
			printf("add <nomcompte> <sexe> <motdepasse>\n");
			printf("  Crée un compte avec l'email par défaut (a@a.com).\n");
			printf("  Concernant le sexe, seule la première lettre compte (F ou M).\n");
			printf("  L'e-mail est a@a.com (e-mail par défaut). C'est comme n'avoir aucun e-mail.\n");
			printf("  Lorsque motdepasse est omis, la saisie se fait sans que la frappe se voit.\n");
			printf("  <exemple> add testname Male testpass\n");
		} else if (strcmp(cmd_name, "ban") == 0) {
			printf("ban/banish aaaa/mm/jj hh:mm:ss <nom compte>\n");
			printf("  Change la date de fin de bannissement d'un compte.\n");
			printf("  Comme banset, mais <nom compte> est à la fin.\n");
		} else if (strcmp(cmd_name, "banadd") == 0) {
			printf("banadd <nomcompte> <Modificateur>\n");
			printf("  Ajoute ou soustrait du temps à la date de banissement d'un compte.\n");
			printf("  Les modificateurs sont construits comme suit:\n");
			printf("    Valeur d'ajustement (-1, 1, +1, etc...)\n");
			printf("    Elément modifié:\n");
			printf("      a ou y: année\n");
			printf("      m:      mois\n");
			printf("      j ou d: jour\n");
			printf("      h:      heure\n");
			printf("      mn:     minute\n");
			printf("      s:      seconde\n");
			printf("  <exemple> banadd testname +1m-2mn1s-6a\n");
			printf("            Cette exemple ajoute 1 mois et une seconde, et soustrait 2 minutes\n");
			printf("            et 6 ans dans le même temps.\n");
			printf("NOTE: Si vous modifez la date de banissement d'un compte non bani,\n");
			printf("      vous indiquez comme date (le moment actuel +- les ajustements).\n");
		} else if (strcmp(cmd_name, "banset") == 0) {
			printf("banset <nomcompte> aaaa/mm/jj [hh:mm:ss]\n");
			printf("  Change la date de fin de bannissement d'un compte.\n");
			printf("  Heure par défaut [hh:mm:ss]: 23:59:59.\n");
			printf("banset <nomcompte> 0\n");
			printf("  Débanni un compte (0 = de-banni).\n");
		} else if (strcmp(cmd_name, "block") == 0) {
			printf("block <nom compte>\n");
			printf("  Place le status d'un compte à 5 (You have been blocked by the GM Team).\n");
			printf("  La commande est l'équivalent de state <nom_compte> 5.\n");
		} else if (strcmp(cmd_name, "check") == 0) {
			printf("check <nomcompte> <motdepasse>\n");
			printf("  Vérifie la validité d'un mot de passe pour un compte.\n");
			printf("  NOTE: Le serveur n'enverra jamais un mot de passe.\n");
			printf("        C'est la seule méthode que vous possédez pour savoir\n");
			printf("        si un mot de passe est le bon. L'autre méthode est\n");
			printf("        d'avoir un accès ('physique') au fichier des comptes.\n");
		} else if (strcmp(cmd_name, "create") == 0) {
			printf("create <nomcompte> <sexe> <email> <motdepasse>\n");
			printf("  Comme la commande add, mais avec l'e-mail en plus.\n");
			printf("  <exemple> create testname Male mon@mail.com testpass\n");
		} else if (strcmp(cmd_name, "delete") == 0) {
			printf("del <nom compte>\n");
			printf("  Supprime un compte.\n");
			printf("  La commande demande confirmation. Après confirmation, le compte est détruit.\n");
		} else if (strcmp(cmd_name, "email") == 0) {
			printf("email <nomcompte> <email>\n");
			printf("  Modifie l'e-mail d'un compte.\n");
		} else if (strcmp(cmd_name, "getcount") == 0) {
			printf("getcount\n");
			printf("  Donne le nombre de joueurs en ligne par serveur de char.\n");
		} else if (strcmp(cmd_name, "gm") == 0) {
			printf("gm <nomcompte> [Niveau_GM]\n");
			printf("  Modifie le niveau de GM d'un compte.\n");
			printf("  Valeur par défaut: 0 (suppression du niveau de GM).\n");
			printf("  <exemple> gm nomtest 80\n");
		} else if (strcmp(cmd_name, "id") == 0) {
			printf("id <nom compte>\n");
			printf("  Donne l'id d'un compte.\n");
		} else if (strcmp(cmd_name, "info") == 0) {
			printf("info <idcompte>\n");
			printf("  Affiche les informations sur un compte.\n");
		} else if (strcmp(cmd_name, "kami") == 0) {
			printf("kami <message>\n");
			printf("  Envoi un message général sur tous les serveurs de map (en jaune).\n");
		} else if (strcmp(cmd_name, "kamib") == 0) {
			printf("kamib <message>\n");
			printf("  Envoi un message général sur tous les serveurs de map (en bleu).\n");
		} else if (strcmp(cmd_name, "language") == 0) {
			printf("language <langue>\n");
			printf("  Change la langue d'affichage.\n");
			printf("  Langues possibles: 'Français' ou 'English'.\n");
		} else if (strcmp(cmd_name, "list") == 0) {
			printf("list/ls [Premier_id [Dernier_id]]\n");
			printf("  Affiche une liste de comptes.\n");
			printf("  'Premier_id', 'Dernier_id': indique les identifiants de départ et de fin.\n");
			printf("  La recherche par nom n'est pas possible avec cette commande.\n");
			printf("  <example> list 10 9999999\n");
		} else if (strcmp(cmd_name, "listban") == 0) {
			printf("listBan/lsBan [Premier_id [Dernier_id]]\n");
			printf("  Comme list/ls, mais seulement pour les comptes avec statut ou bannis.\n");
		} else if (strcmp(cmd_name, "listgm") == 0) {
			printf("listGM/lsGM [Premier_id [Dernier_id]]\n");
			printf("  Comme list/ls, mais seulement pour les comptes GM.\n");
		} else if (strcmp(cmd_name, "listok") == 0) {
			printf("listOK/lsOK [Premier_id [Dernier_id]]\n");
			printf("  Comme list/ls, mais seulement pour les comptes sans statut et non bannis.\n");
		} else if (strcmp(cmd_name, "listonline") == 0) {
			printf("listOnline/lsOnline [Premier_id [Dernier_id]]\n");
			printf("  Comme list/ls, mais seulement pour les comptes online.\n");
		} else if (strcmp(cmd_name, "memo") == 0) {
			printf("memo <nomcompte> <memo>\n");
			printf("  Modifie le mémo d'un compte.\n");
			printf("  'memo': Il peut avoir jusqu'à 60000 caractères (avec des espaces ou non).\n");
		} else if (strcmp(cmd_name, "memoadd") == 0) {
			printf("memoadd <nomcompte> <texte du memo>\n");
			printf("  Ajoute <texte du memo> au mémo d'un compte.\n");
			printf("  Le 'memo' peut avoir jusqu'à 60000 caractères (avec des espaces ou non).\n");
		} else if (strcmp(cmd_name, "name") == 0) {
			printf("name <idcompte>\n");
			printf("  Donne le nom d'un compte.\n");
		} else if (strcmp(cmd_name, "password") == 0) {
			printf("passwd <nomcompte> <nouveaumotdepasse>\n");
			printf("  Change le mot de passe d'un compte.\n");
			printf("  Lorsque nouveaumotdepasse est omis,\n");
			printf("  la saisie se fait sans que la frappe ne se voit.\n");
		} else if (strcmp(cmd_name, "search") == 0) {
			printf("search <expression>\n");
			printf("  Cherche des comptes.\n");
			printf("  Affiche les comptes dont les noms correspondent.\n");
//			printf("search -r/-e/--expr/--regex <expression>\n");
//			printf("  Cherche des comptes par expression regulière.\n");
//			printf("  Affiche les comptes dont les noms correspondent.\n");
		} else if (strcmp(cmd_name, "sex") == 0) {
			printf("sex <nomcompte> <sexe>\n");
			printf("  Modifie le sexe d'un compte.\n");
			printf("  <exemple> sex testname Male\n");
		} else if (strcmp(cmd_name, "state") == 0) {
			printf("state <nomcompte> <nouveaustatut> <message_erreur_7>\n");
			printf("  Change le statut d'un compte.\n");
			printf("  'nouveaustatut': Le statut est le même que celui du packet 0x006a + 1.\n");
			printf("               les possibilités sont:\n");
			printf("               0 = Compte ok\n");
			printf("               1 = Unregistered ID\n");
			printf("               2 = Incorrect Password\n");
			printf("               3 = This ID is expired\n");
			printf("               4 = Rejected from Server\n");
			printf("               5 = You have been blocked by the GM Team\n");
			printf("               6 = Your Game's EXE file is not the latest version\n");
			printf("               7 = You are Prohibited to log in until...\n");
			printf("               8 = Server is jammed due to over populated\n");
			printf("               9 = No MSG\n");
			printf("               100 = This ID has been totally erased\n");
			printf("               all other values are 'No MSG', then use state 9 please.\n");
			printf("  'message_erreur_7': message du code erreur 6 =\n");
			printf("                      Your are Prohibited to log in until... (packet 0x006a).\n");
		} else if (strcmp(cmd_name, "timeadd") == 0) {
			printf("timeadd <nomcompte> <modificateur>\n");
			printf("  Ajoute/soustrait du temps à la limite de validité d'un compte.\n");
			printf("  Le modificateur est composé comme suit:\n");
			printf("    Valeur modificatrice (-1, 1, +1, etc...)\n");
			printf("    Elément modifié:\n");
			printf("      a ou y: année\n");
			printf("      m:      mois\n");
			printf("      j ou d: jour\n");
			printf("      h:      heure\n");
			printf("      mn:     minute\n");
			printf("      s:      seconde\n");
			printf("  <exemple> timeadd testname +1m-2mn1s-6a\n");
			printf("            Cette exemple ajoute 1 mois et une seconde, et soustrait 2 minutes\n");
			printf("            et 6 ans dans le même temps.\n");
			printf("NOTE: Vous ne pouvez pas modifier une limite de validité illimitée. Si vous\n");
			printf("      désirez le faire, c'est que vous voulez probablement créer un limite de\n");
			printf("      validité limitée. Donc, en premier, fixé une limite de valitidé.\n");
		} else if (strcmp(cmd_name, "timeadd") == 0) {
			printf("timeset <nomcompte> aaaa/mm/jj [hh:mm:ss]\n");
			printf("  Change la limite de validité d'un compte.\n");
			printf("  Heure par défaut [hh:mm:ss]: 23:59:59.\n");
			printf("timeset <nomcompte> 0\n");
			printf("  Donne une limite de validité illimitée (0 = illimitée).\n");
		} else if (strcmp(cmd_name, "unban") == 0) {
			printf("unban/unbanish <nom compte>\n");
			printf("  Ote le banissement d'un compte.\n");
			printf("  La commande est l'équivalent de banset <nom_compte> 0.\n");
		} else if (strcmp(cmd_name, "unblock") == 0) {
			printf("unblock <nom compte>\n");
			printf("  Place le status d'un compte à 0 (Compte ok).\n");
			printf("  La commande est l'équivalent de state <nom_compte> 0.\n");
		} else if (strcmp(cmd_name, "uptime") == 0) {
			printf("uptime\n");
			printf("  Affiche l'uptime du login-serveur.\n");
		} else if (strcmp(cmd_name, "version") == 0) {
			printf("version\n");
			printf("  Affiche la version du login-serveur.\n");
		} else if (strcmp(cmd_name, "who") == 0) {
			printf("who <nom compte>\n");
			printf("  Affiche les informations sur un compte.\n");
// quit
		} else if (strcmp(cmd_name, "quit") == 0 ||
		           strcmp(cmd_name, "exit") == 0 ||
		           strcmp(cmd_name, "end") == 0) {
			printf("quit/end/exit\n");
			printf("  Fin du programme d'administration.\n");
// unknown command
		} else {
			if (cmd_name[0] != '\0')
				printf("Commande inconnue [%s] pour l'aide. Affichage de toutes les commandes.\n", cmd_name);
			printf(" aide/help/?                             -- Affiche cet aide\n");
			printf(" aide/help/? [commande]                  -- Affiche l'aide de la commande\n");
			printf(" add <nomcompte> <sexe> <motdepasse>     -- Crée un compte (sans email)\n");
			printf(" ban/banish aaaa/mm/jj hh:mm:ss <nom compte> -- Fixe la date finale de banismnt\n");
			printf(" banadd/ba <nomcompte> <modificateur>    -- Ajout/soustrait du temps à la\n");
			printf("   exemple: ba moncompte +1m-2mn1s-2y       date finale de banissement\n");
			printf(" banset/bs <nomcompte> aaaa/mm/jj [hh:mm:ss] -- Change la date fin de banisemnt\n");
			printf(" banset/bs <nomcompte> 0                 -- Dé-banis un compte.\n");
			printf(" block <nom compte>  -- Mets le status d'un compte à 5 (blocked by the GM Team)\n");
			printf(" check <nomcompte> <motdepasse>          -- Vérifie un mot de passe d'un compte\n");
			printf(" create <nomcompte> <sexe> <email> <motdepasse> -- Crée un compte (avec email)\n");
			printf(" del <nom compte>                        -- Supprime un compte\n");
			printf(" email <nomcompte> <email>               -- Modifie l'e-mail d'un compte\n");
			printf(" getcount                                -- Donne le nb de joueurs en ligne\n");
			printf("  gm <nomcompte> [Niveau_GM]              -- Modifie le niveau de GM d'un compte\n");
			printf(" id <nom compte>                         -- Donne l'id d'un compte\n");
			printf(" info <idcompte>                         -- Affiche les infos sur un compte\n");
			printf(" kami <message>                          -- Envoi un message général (en jaune)\n");
			printf(" kamib <message>                         -- Envoi un message général (en bleu)\n");
			printf(" language <langue>                       -- Change la langue d'affichage.\n");
			printf(" list/ls [Premier_id [Dernier_id] ]      -- Affiche une liste de comptes\n");
			printf(" listBan/lsBan [Premier_id [Dernier_id] ] -- Affiche une liste de comptes\n");
			printf("                                             avec un statut ou bannis\n");
			printf(" listGM/lsGM [Premier_id [Dernier_id] ]  -- Affiche une liste de comptes GM\n");
			printf(" listOK/lsOK [Premier_id [Dernier_id] ]  -- Affiche une liste de comptes\n");
			printf("                                            sans status et non bannis\n");
			printf(" listOnline/lsOnline [Pr_id [Dern_id] ]  -- Affiche les comptes online\n");
			printf(" memo <nomcompte> <memo>                 -- Modifie le memo d'un compte\n");
			printf(" memoadd <nomcompte> <texte du memo>     -- Ajoute <texte> au memo d'un compte\n");
			printf(" name <idcompte>                         -- Donne le nom d'un compte\n");
			printf(" passwd <nomcompte> <nouveaumotdepasse>  -- Change le mot de passe d'un compte\n");
			printf(" quit/end/exit                           -- Fin du programme d'administation\n");
			printf(" search <expression>                     -- Cherche des comptes\n");
//			printf(" search -e/-r/--expr/--regex <expression> -- Cherche des comptes par REGEX\n");
			printf(" sex <nomcompte> <sexe>                  -- Modifie le sexe d'un compte\n");
			printf(" state <nomcompte> <nouveaustatut> <messageerr7> -- Change le statut d'1 compte\n");
			printf(" timeadd/ta <nomcompte> <modificateur>   -- Ajout/soustrait du temps à la\n");
			printf("   exemple: ta moncompte +1m-2mn1s-2y       limite de validité\n");
			printf(" timeset/ts <nomcompte> aaaa/mm/jj [hh:mm:ss] -- Change la limite de validité\n");
			printf(" timeset/ts <nomcompte> 0                -- limite de validité = illimitée\n");
			printf(" unban/unbanish <nom compte>             -- Ote le banissement d'un compte\n");
			printf(" unblock <nom compte>             -- Mets le status d'un compte à 0 (Compte ok)\n");
			printf(" uptime                                  -- Donne l'uptime du login-serveur\n");
			printf(" version                                 -- Donne la version du login-serveur\n");
			printf(" who <nom compte>                        -- Affiche les infos sur un compte\n");
			printf(" Note: Pour les noms de compte avec des espaces, tapez \"<nom compte>\" (ou ').\n");
		}
	} else {
		if (strcmp(cmd_name, "aide") == 0) {
			printf("aide/help/?\n");
			printf("  Display the description of the commands.\n");
			printf("aide/help/? [command]\n");
			printf("  Display the description of the specified command.\n");
		} else if (strcmp(cmd_name, "help") == 0) {
			printf("aide/help/?\n");
			printf("  Display the description of the commands.\n");
			printf("aide/help/? [command]\n");
			printf("  Display the description of the specified command.\n");
// general commands
		} else if (strcmp(cmd_name, "add") == 0) {
			printf("add <account_name> <sex> <password>\n");
			printf("  Create an account with the default email (a@a.com).\n");
			printf("  Concerning the sex, only the first letter is used (F or M).\n");
			printf("  The e-mail is set to a@a.com (default e-mail). It's like to have no e-mail.\n");
			printf("  When the password is omitted,\n");
			printf("  the input is done without displaying of the pressed keys.\n");
			printf("  <example> add testname Male testpass\n");
		} else if (strcmp(cmd_name, "ban") == 0) {
			printf("ban/banish yyyy/mm/dd hh:mm:ss <account name>\n");
			printf("  Changes the final date of a banishment of an account.\n");
			printf("  Like banset, but <account name> is at end.\n");
		} else if (strcmp(cmd_name, "banadd") == 0) {
			printf("banadd <account_name> <modifier>\n");
			printf("  Adds or substracts time from the final date of a banishment of an account.\n");
			printf("  Modifier is done as follows:\n");
			printf("    Adjustment value (-1, 1, +1, etc...)\n");
			printf("    Modified element:\n");
			printf("      a or y: year\n");
			printf("      m:  month\n");
			printf("      j or d: day\n");
			printf("      h:  hour\n");
			printf("      mn: minute\n");
			printf("      s:  second\n");
			printf("  <example> banadd testname +1m-2mn1s-6y\n");
			printf("            this example adds 1 month and 1 second, and substracts 2 minutes\n");
			printf("            and 6 years at the same time.\n");
			printf("NOTE: If you modify the final date of a non-banished account,\n");
			printf("      you fix the final date to (actual time +- adjustments).\n");
		} else if (strcmp(cmd_name, "banset") == 0) {
			printf("banset <account_name> yyyy/mm/dd [hh:mm:ss]\n");
			printf("  Changes the final date of a banishment of an account.\n");
			printf("  Default time [hh:mm:ss]: 23:59:59.\n");
			printf("banset <account_name> 0\n");
			printf("  Set a non-banished account (0 = unbanished).\n");
		} else if (strcmp(cmd_name, "block") == 0) {
			printf("block <account name>\n");
			printf("  Set state 5 (You have been blocked by the GM Team) to an account.\n");
			printf("  This command works like state <account_name> 5.\n");
		} else if (strcmp(cmd_name, "check") == 0) {
			printf("check <account_name> <password>\n");
			printf("  Check the validity of a password for an account.\n");
			printf("  NOTE: Server will never sends back a password.\n");
			printf("        It's the only method you have to know if a password is correct.\n");
			printf("        The other method is to have a ('physical') access to the accounts file.\n");
		} else if (strcmp(cmd_name, "create") == 0) {
			printf("create <account_name> <sex> <email> <password>\n");
			printf("  Like the 'add' command, but with e-mail moreover.\n");
			printf("  <example> create testname Male my@mail.com testpass\n");
		} else if (strcmp(cmd_name, "delete") == 0) {
			printf("del <account name>\n");
			printf("  Remove an account.\n");
			printf("  This order requires confirmation. After confirmation, the account is deleted.\n");
		} else if (strcmp(cmd_name, "email") == 0) {
			printf("email <account_name> <email>\n");
			printf("  Modify the e-mail of an account.\n");
		} else if (strcmp(cmd_name, "getcount") == 0) {
			printf("getcount\n");
			printf("  Give the number of players online on all char-servers.\n");
		} else if (strcmp(cmd_name, "gm") == 0) {
			printf("gm <account_name> [GM_level]\n");
			printf("  Modify the GM level of an account.\n");
			printf("  Default value remove GM level (GM level = 0).\n");
			printf("  <example> gm testname 80\n");
		} else if (strcmp(cmd_name, "id") == 0) {
			printf("id <account name>\n");
			printf("  Give the id of an account.\n");
		} else if (strcmp(cmd_name, "info") == 0) {
			printf("info <account_id>\n");
			printf("  Display complete information of an account.\n");
		} else if (strcmp(cmd_name, "kami") == 0) {
			printf("kami <message>\n");
			printf("  Sends a broadcast message on all map-server (in yellow).\n");
		} else if (strcmp(cmd_name, "kamib") == 0) {
			printf("kamib <message>\n");
			printf("  Sends a broadcast message on all map-server (in blue).\n");
		} else if (strcmp(cmd_name, "language") == 0) {
			printf("language <language>\n");
			printf("  Change the language of displaying.\n");
			printf("  Possible languages: Français or English.\n");
		} else if (strcmp(cmd_name, "list") == 0) {
			printf("list/ls [start_id [end_id]]\n");
			printf("  Display a list of accounts.\n");
			printf("  'start_id', 'end_id': indicate end and start identifiers.\n");
			printf("  Research by name is not possible with this command.\n");
			printf("  <example> list 10 9999999\n");
		} else if (strcmp(cmd_name, "listban") == 0) {
			printf("listBan/lsBan [start_id [end_id]]\n");
			printf("  Like list/ls, but only for accounts with state or banished.\n");
		} else if (strcmp(cmd_name, "listgm") == 0) {
			printf("listGM/lsGM [start_id [end_id]]\n");
			printf("  Like list/ls, but only for GM accounts.\n");
		} else if (strcmp(cmd_name, "listok") == 0) {
			printf("listOK/lsOK [start_id [end_id]]\n");
			printf("  Like list/ls, but only for accounts without state and not banished.\n");
		} else if (strcmp(cmd_name, "listonline") == 0) {
			printf("listOnline/lsOnline [start_id [end_id]]\n");
			printf("  Like list/ls, but only for online accounts.\n");
		} else if (strcmp(cmd_name, "memo") == 0) {
			printf("memo <account_name> <memo>\n");
			printf("  Modify the memo of an account.\n");
			printf("  'memo': it can have until 60000 characters (with spaces or not).\n");
		} else if (strcmp(cmd_name, "memoadd") == 0) {
			printf("memoadd <account_name> <memo text>\n");
			printf("  Add <memo text> to memo of an account.\n");
			printf("  The 'memo' can have until 60000 characters (with spaces or not).\n");
		} else if (strcmp(cmd_name, "name") == 0) {
			printf("name <account_id>\n");
			printf("  Give the name of an account.\n");
		} else if (strcmp(cmd_name, "password") == 0) {
			printf("passwd <account_name> <new_password>\n");
			printf("  Change the password of an account.\n");
			printf("  When new password is omitted,\n");
			printf("  the input is done without displaying of the pressed keys.\n");
		} else if (strcmp(cmd_name, "search") == 0) {
			printf("search <expression>\n");
			printf("  Seek accounts.\n");
			printf("  Displays the accounts whose names correspond.\n");
//			printf("search -r/-e/--expr/--regex <expression>\n");
//			printf("  Seek accounts by regular expression.\n");
//			printf("  Displays the accounts whose names correspond.\n");
		} else if (strcmp(cmd_name, "sex") == 0) {
			printf("sex <account_name> <sex>\n");
			printf("  Modify the sex of an account.\n");
			printf("  <example> sex testname Male\n");
		} else if (strcmp(cmd_name, "state") == 0) {
			printf("state <account_name> <new_state> <error_message_#7>\n");
			printf("  Change the state of an account.\n");
			printf("  'new_state': state is the state of the packet 0x006a + 1.\n");
			printf("               The possibilities are:\n");
			printf("               0 = Account ok\n");
			printf("               1 = Unregistered ID\n");
			printf("               2 = Incorrect Password\n");
			printf("               3 = This ID is expired\n");
			printf("               4 = Rejected from Server\n");
			printf("               5 = You have been blocked by the GM Team\n");
			printf("               6 = Your Game's EXE file is not the latest version\n");
			printf("               7 = You are Prohibited to log in until...\n");
			printf("               8 = Server is jammed due to over populated\n");
			printf("               9 = No MSG\n");
			printf("               100 = This ID has been totally erased\n");
			printf("               all other values are 'No MSG', then use state 9 please.\n");
			printf("  'error_message_#7': message of the code error 6 =\n");
			printf("                      Your are Prohibited to log in until... (packet 0x006a).\n");
		} else if (strcmp(cmd_name, "timeadd") == 0) {
			printf("timeadd <account_name> <modifier>\n");
			printf("  Adds or substracts time from the validity limit of an account.\n");
			printf("  Modifier is done as follows:\n");
			printf("    Adjustment value (-1, 1, +1, etc...)\n");
			printf("    Modified element:\n");
			printf("      a or y: year\n");
			printf("      m:  month\n");
			printf("      j or d: day\n");
			printf("      h:  hour\n");
			printf("      mn: minute\n");
			printf("      s:  second\n");
			printf("  <example> timeadd testname +1m-2mn1s-6y\n");
			printf("            this example adds 1 month and 1 second, and substracts 2 minutes\n");
			printf("            and 6 years at the same time.\n");
			printf("NOTE: You can not modify a unlimited validity limit.\n");
			printf("      If you want modify it, you want probably create a limited validity limit.\n");
			printf("      So, at first, you must set the validity limit to a date/time.\n");
		} else if (strcmp(cmd_name, "timeadd") == 0) {
			printf("timeset <account_name> yyyy/mm/dd [hh:mm:ss]\n");
			printf("  Changes the validity limit of an account.\n");
			printf("  Default time [hh:mm:ss]: 23:59:59.\n");
			printf("timeset <account_name> 0\n");
			printf("  Gives an unlimited validity limit (0 = unlimited).\n");
		} else if (strcmp(cmd_name, "unban") == 0) {
			printf("unban/unbanish <account name>\n");
			printf("  Remove the banishment of an account.\n");
			printf("  This command works like banset <account_name> 0.\n");
		} else if (strcmp(cmd_name, "unblock") == 0) {
			printf("unblock <account name>\n");
			printf("  Set state 0 (Account ok) to an account.\n");
			printf("  This command works like state <account_name> 0.\n");
		} else if (strcmp(cmd_name, "uptime") == 0) {
			printf("uptime\n");
			printf("  Display the uptime of the login-server.\n");
		} else if (strcmp(cmd_name, "version") == 0) {
			printf("version\n");
			printf("  Display the version of the login-server.\n");
		} else if (strcmp(cmd_name, "who") == 0) {
			printf("who <account name>\n");
			printf("  Displays complete information of an account.\n");
// quit
		} else if (strcmp(cmd_name, "quit") == 0 ||
		           strcmp(cmd_name, "exit") == 0 ||
		           strcmp(cmd_name, "end") == 0) {
			printf("quit/end/exit\n");
			printf("  End of the program of administration.\n");
// unknown command
		} else {
			if (cmd_name[0] != '\0')
				printf("Unknown command [%s] for help. Displaying of all commands.\n", cmd_name);
			printf(" aide/help/?                          -- Display this help\n");
			printf(" aide/help/? [command]                -- Display the help of the command\n");
			printf(" add <account_name> <sex> <password>  -- Create an account with default email\n");
			printf(" ban/banish yyyy/mm/dd hh:mm:ss <account name> -- Change final date of a ban\n");
			printf(" banadd/ba <account_name> <modifier>  -- Add or substract time from the final\n");
			printf("   example: ba apple +1m-2mn1s-2y        date of a banishment of an account\n");
			printf(" banset/bs <account_name> yyyy/mm/dd [hh:mm:ss] -- Change final date of a ban\n");
			printf(" banset/bs <account_name> 0           -- Un-banish an account\n");
			printf(" block <account name>     -- Set state 5 (blocked by the GM Team) to an account\n");
			printf(" check <account_name> <password>      -- Check the validity of a password\n");
			printf(" create <account_name> <sex> <email> <passwrd> -- Create an account with email\n");
			printf(" del <account name>                   -- Remove an account\n");
			printf(" email <account_name> <email>         -- Modify an email of an account\n");
			printf(" getcount                             -- Give the number of players online\n");
			printf(" gm <account_name> [GM_level]         -- Modify the GM level of an account\n");
			printf(" id <account name>                    -- Give the id of an account\n");
			printf(" info <account_id>                    -- Display all information of an account\n");
			printf(" kami <message>                       -- Sends a broadcast message (in yellow)\n");
			printf(" kamib <message>                      -- Sends a broadcast message (in blue)\n");
			printf(" language <language>                  -- Change the language of displaying.\n");
			printf(" list/ls [First_id [Last_id]]         -- Display a list of accounts\n");
			printf(" listBan/lsBan [First_id [Last_id] ]  -- Display a list of accounts\n");
			printf("                                         with state or banished\n");
			printf(" listGM/lsGM [First_id [Last_id]]     -- Display a list of GM accounts\n");
			printf(" listOK/lsOK [First_id [Last_id] ]    -- Display a list of accounts\n");
			printf("                                         without state and not banished\n");
			printf(" listOnline/lsOnline [1_id [Lst_id] ] -- Display online accounts\n");
			printf(" memo <account_name> <memo>           -- Modify the memo of an account\n");
			printf(" memoadd <account_name> <memo text>   -- Add <text> to the memo of an account\n");
			printf(" name <account_id>                    -- Give the name of an account\n");
			printf(" passwd <account_name> <new_password> -- Change the password of an account\n");
			printf(" quit/end/exit                        -- End of the program of administation\n");
			printf(" search <expression>                  -- Seek accounts\n");
//			printf(" search -e/-r/--expr/--regex <expressn> -- Seek accounts by regular-expression\n");
			printf(" sex <nomcompte> <sexe>               -- Modify the sex of an account\n");
			printf(" state <account_name> <new_state> <error_message_#7> -- Change the state\n");
			printf(" timeadd/ta <account_name> <modifier> -- Add or substract time from the\n");
			printf("   example: ta apple +1m-2mn1s-2y        validity limit of an account\n");
			printf(" timeset/ts <account_name> yyyy/mm/dd [hh:mm:ss] -- Change the validify limit\n");
			printf(" timeset/ts <account_name> 0          -- Give a unlimited validity limit\n");
			printf(" unban/unbanish <account name>        -- Remove the banishment of an account\n");
			printf(" unblock <account name>               -- Set state 0 (Account ok) to an account\n");
			printf(" uptime                               -- Gives the uptime of the login-server\n");
			printf(" version                              -- Gives the version of the login-server\n");
			printf(" who <account name>                   -- Display all information of an account\n");
			printf(" Note: To use spaces in an account name, type \"<account name>\" (or ').\n");
		}
	}

	return;
}

//-----------------------------
// Sub-function: add an account
//-----------------------------
static void addaccount(char* param, int emailflag) { // not inline, called too often
	char name[1023], sex[1023], email[1023], password[1023];
//	int i;

	memset(name, 0, sizeof(name));
	memset(sex, 0, sizeof(sex));
	memset(email, 0, sizeof(email));
	memset(password, 0, sizeof(password));

	if (emailflag == 0) { // add command
		if (sscanf(param, "\"%[^\"]\" %s %[^\r\n]", name, sex, password) < 2 && // password can be void
		    sscanf(param, "'%[^']' %s %[^\r\n]", name, sex, password) < 2 && // password can be void
		    sscanf(param, "%s %s %[^\r\n]", name, sex, password) < 2) { // password can be void
			if (defaultlanguage == 'F') {
				printf("Entrez un nom de compte, un sexe et un mot de passe svp.\n");
				printf("<exemple> add nomtest Male motdepassetest\n");
				ladmin_log("Nombre incorrect de paramètres pour créer un compte (commande 'add')." RETCODE);
			} else {
				printf("Please input an account name, a sex and a password.\n");
				printf("<example> add testname Male testpass\n");
				ladmin_log("Incomplete parameters to create an account ('add' command)." RETCODE);
			}
			return;
		}
		strcpy(email, "a@a.com"); // default email
	} else { // 1: create command
		if (sscanf(param, "\"%[^\"]\" %s %s %[^\r\n]", name, sex, email, password) < 3 && // password can be void
		    sscanf(param, "'%[^']' %s %s %[^\r\n]", name, sex, email, password) < 3 && // password can be void
		    sscanf(param, "%s %s %s %[^\r\n]", name, sex, email, password) < 3) { // password can be void
			if (defaultlanguage == 'F') {
				printf("Entrez un nom de compte, un sexe et un mot de passe svp.\n");
				printf("<exemple> create nomtest Male mo@mail.com motdepassetest\n");
				ladmin_log("Nombre incorrect de paramètres pour créer un compte (commande 'create')." RETCODE);
			} else {
				printf("Please input an account name, a sex and a password.\n");
				printf("<example> create testname Male my@mail.com testpass\n");
				ladmin_log("Incomplete parameters to create an account ('create' command)." RETCODE);
			}
			return;
		}
	}
	if (verify_accountname(name) == 0) {
		return;
	}

/*	for(i = 0; name[i]; i++) {
		if (strchr("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz-_", name[i]) == NULL) {
			if (defaultlanguage == 'F') {
				printf("Caractère interdit (%c) trouvé dans le nom du compte (%d%s caractère).\n", name[i], i+1, makeordinal(i+1));
				ladmin_log("Caractère interdit (%c) trouvé dans le nom du compte (%d%s caractère)." RETCODE, name[i], i+1, makeordinal(i+1));
			} else {
				printf("Illegal character (%c) found in the account name (%d%s character).\n", name[i], i+1, makeordinal(i+1));
				ladmin_log("Illegal character (%c) found in the account name (%d%s character)." RETCODE, name[i], i+1, makeordinal(i+1));
			}
			return;
		}
	}*/

	sex[0] = toupper((unsigned char)sex[0]); // toupper needs unsigned char
	if (strchr("MF", sex[0]) == NULL) {
		if (defaultlanguage == 'F') {
			printf("Sexe incorrect [%s]. Entrez M ou F svp.\n", sex);
			ladmin_log("Sexe incorrect [%s]. Entrez M ou F svp." RETCODE, sex);
		} else {
			printf("Illegal gender [%s]. Please input M or F.\n", sex);
			ladmin_log("Illegal gender [%s]. Please input M or F." RETCODE, sex);
		}
		return;
	}

	if (strlen(email) < 3) {
		if (defaultlanguage == 'F') {
			printf("Email trop courte [%s]. Entrez une e-mail valide svp.\n", email);
			ladmin_log("Email trop courte [%s]. Entrez une e-mail valide svp." RETCODE, email);
		} else {
			printf("Email is too short [%s]. Please input a valid e-mail.\n", email);
			ladmin_log("Email is too short [%s]. Please input a valid e-mail." RETCODE, email);
		}
		return;
	}
	if (strlen(email) > 40) {
		if (defaultlanguage == 'F') {
			printf("Email trop longue [%s]. Entrez une e-mail de 40 caractères maximum svp.\n", email);
			ladmin_log("Email trop longue [%s]. Entrez une e-mail de 40 caractères maximum svp." RETCODE, email);
		} else {
			printf("Email is too long [%s]. Please input an e-mail with 40 bytes at the most.\n", email);
			ladmin_log("Email is too long [%s]. Please input an e-mail with 40 bytes at the most." RETCODE, email);
		}
		return;
	}
	if (e_mail_check(email) == 0) {
		if (defaultlanguage == 'F') {
			printf("Email incorrecte [%s]. Entrez une e-mail valide svp.\n", email);
			ladmin_log("Email incorrecte [%s]. Entrez une e-mail valide svp." RETCODE, email);
		} else {
			printf("Invalid email [%s]. Please input a valid e-mail.\n", email);
			ladmin_log("Invalid email [%s]. Please input a valid e-mail." RETCODE, email);
		}
		return;
	}

	if (password[0] == '\0') {
		if (typepasswd(password) == 0)
			return;
	}
	if (verify_password(password) == 0)
		return;

	if (defaultlanguage == 'F') {
		ladmin_log("Envoi d'un requête au serveur de logins pour créer un compte." RETCODE);
	} else {
		ladmin_log("Request to login-server to create an account." RETCODE);
	}

	WPACKETW(0) = 0x7930;
	strncpy(WPACKETP( 2), name, 24);
	strncpy(WPACKETP(26), password, 24);
	WPACKETB(50) = sex[0];
	strncpy(WPACKETP(51), email, 40);
	SENDPACKET(login_fd, 91);
	bytes_to_read = 1;

	return;
}

//---------------------------------------------------------------------------------
// Sub-function: Add/substract time to the final date of a banishment of an account
//---------------------------------------------------------------------------------
static inline void banaddaccount(char* param) {
	char name[1023], modif[1023];
	int year, month, day, hour, minute, second;
	char * p_modif;
	int value, i;

	memset(name, 0, sizeof(name));
	memset(modif, 0, sizeof(modif));
	year = month = day = hour = minute = second = 0;

	if (sscanf(param, "\"%[^\"]\" %[^\r\n]", name, modif) < 2 &&
	    sscanf(param, "'%[^']' %[^\r\n]", name, modif) < 2 &&
	    sscanf(param, "%s %[^\r\n]", name, modif) < 2) {
		if (defaultlanguage == 'F') {
			printf("Entrez un nom de compte et un modificateur svp.\n");
			printf("  <exemple> banadd nomtest +1m-2mn1s-6y\n");
			printf("            Cette exemple ajoute 1 mois et 1 seconde, et soustrait 2 minutes\n");
			printf("            et 6 ans dans le même temps.\n");
			ladmin_log("Nombre incorrect de paramètres pour modifier la fin de ban d'un compte (commande 'banadd')." RETCODE);
		} else {
			printf("Please input an account name and a modifier.\n");
			printf("  <example>: banadd testname +1m-2mn1s-6y\n");
			printf("             this example adds 1 month and 1 second, and substracts 2 minutes\n");
			printf("             and 6 years at the same time.\n");
			ladmin_log("Incomplete parameters to modify the ban date/time of an account ('banadd' command)." RETCODE);
		}
		return;
	}
	if (verify_accountname(name) == 0) {
		return;
	}

	// lowercase for modif
	for (i = 0; modif[i]; i++)
		modif[i] = tolower((unsigned char)modif[i]); // tolower needs unsigned char
	p_modif = modif;
	while (p_modif[0] != '\0') {
		value = atoi(p_modif);
		if (value == 0) {
			p_modif++;
		} else {
			if (p_modif[0] == '-' || p_modif[0] == '+')
				p_modif++;
			while (p_modif[0] != '\0' && p_modif[0] >= '0' && p_modif[0] <= '9') {
				p_modif++;
			}
			if (p_modif[0] == 's') {
				second = value;
				p_modif++;
			} else if (p_modif[0] == 'm' && p_modif[1] == 'n') {
				minute = value;
				p_modif += 2;
			} else if (p_modif[0] == 'h') {
				hour = value;
				p_modif++;
			} else if (p_modif[0] == 'd' || p_modif[0] == 'j') {
				day = value;
				p_modif += 2;
			} else if (p_modif[0] == 'm') {
				month = value;
				p_modif++;
			} else if (p_modif[0] == 'y' || p_modif[0] == 'a') {
				year = value;
				p_modif++;
			} else {
				p_modif++;
			}
		}
	}

	if (defaultlanguage == 'F') {
		printf(" année:   %d\n", year);
		printf(" mois:    %d\n", month);
		printf(" jour:    %d\n", day);
		printf(" heure:   %d\n", hour);
		printf(" minute:  %d\n", minute);
		printf(" seconde: %d\n", second);
	} else {
		printf(" year:   %d\n", year);
		printf(" month:  %d\n", month);
		printf(" day:    %d\n", day);
		printf(" hour:   %d\n", hour);
		printf(" minute: %d\n", minute);
		printf(" second: %d\n", second);
	}

	if (year == 0 && month == 0 && day == 0 && hour == 0 && minute == 0 && second == 0) {
		if (defaultlanguage == 'F') {
			printf("Vous devez entrer un ajustement avec cette commande, svp:\n");
			printf("  Valeur d'ajustement (-1, 1, +1, etc...)\n");
			printf("  Element modifié:\n");
			printf("    a ou y: année\n");
			printf("    m:      mois\n");
			printf("    j ou d: jour\n");
			printf("    h:      heure\n");
			printf("    mn:     minute\n");
			printf("    s:      seconde\n");
			printf("  <exemple> banadd nomtest +1m-2mn1s-6y\n");
			printf("            Cette exemple ajoute 1 mois et 1 seconde, et soustrait 2 minutes\n");
			printf("            et 6 ans dans le même temps.\n");
			ladmin_log("Aucun ajustement n'est pas un ajustement (commande 'banadd')." RETCODE);
		} else {
			printf("Please give an adjustment with this command:\n");
			printf("  Adjustment value (-1, 1, +1, etc...)\n");
			printf("  Modified element:\n");
			printf("    a or y: year\n");
			printf("    m: month\n");
			printf("    j or d: day\n");
			printf("    h: hour\n");
			printf("    mn: minute\n");
			printf("    s: second\n");
			printf("  <example> banadd testname +1m-2mn1s-6y\n");
			printf("            this example adds 1 month and 1 second, and substracts 2 minutes\n");
			printf("            and 6 years at the same time.\n");
			ladmin_log("No adjustment isn't an adjustment ('banadd' command)." RETCODE);
		}
		return;
	}
	if (year > 127 || year < -127) {
		if (defaultlanguage == 'F') {
			printf("Entrez un ajustement d'années correct (de -127 à 127), svp.\n");
			ladmin_log("Ajustement de l'année hors norme (commande 'banadd')." RETCODE);
		} else {
			printf("Please give a correct adjustment for the years (from -127 to 127).\n");
			ladmin_log("Abnormal adjustement for the year ('banadd' command)." RETCODE);
		}
		return;
	}
	if (month > 255 || month < -255) {
		if (defaultlanguage == 'F') {
			printf("Entrez un ajustement de mois correct (de -255 à 255), svp.\n");
			ladmin_log("Ajustement du mois hors norme (commande 'banadd')." RETCODE);
		} else {
			printf("Please give a correct adjustment for the months (from -255 to 255).\n");
			ladmin_log("Abnormal adjustement for the month ('banadd' command)." RETCODE);
		}
		return;
	}
	if (day > 32767 || day < -32767) {
		if (defaultlanguage == 'F') {
			printf("Entrez un ajustement de jours correct (de -32767 à 32767), svp.\n");
			ladmin_log("Ajustement des jours hors norme (commande 'banadd')." RETCODE);
		} else {
			printf("Please give a correct adjustment for the days (from -32767 to 32767).\n");
			ladmin_log("Abnormal adjustement for the days ('banadd' command)." RETCODE);
		}
		return;
	}
	if (hour > 32767 || hour < -32767) {
		if (defaultlanguage == 'F') {
			printf("Entrez un ajustement d'heures correct (de -32767 à 32767), svp.\n");
			ladmin_log("Ajustement des heures hors norme (commande 'banadd')." RETCODE);
		} else {
			printf("Please give a correct adjustment for the hours (from -32767 to 32767).\n");
			ladmin_log("Abnormal adjustement for the hours ('banadd' command)." RETCODE);
		}
		return;
	}
	if (minute > 32767 || minute < -32767) {
		if (defaultlanguage == 'F') {
			printf("Entrez un ajustement de minutes correct (de -32767 à 32767), svp.\n");
			ladmin_log("Ajustement des minutes hors norme (commande 'banadd')." RETCODE);
		} else {
			printf("Please give a correct adjustment for the minutes (from -32767 to 32767).\n");
			ladmin_log("Abnormal adjustement for the minutes ('banadd' command)." RETCODE);
		}
		return;
	}
	if (second > 32767 || second < -32767) {
		if (defaultlanguage == 'F') {
			printf("Entrez un ajustement de secondes correct (de -32767 à 32767), svp.\n");
			ladmin_log("Ajustement des secondes hors norme (commande 'banadd')." RETCODE);
		} else {
			printf("Please give a correct adjustment for the seconds (from -32767 to 32767).\n");
			ladmin_log("Abnormal adjustement for the seconds ('banadd' command)." RETCODE);
		}
		return;
	}

	if (defaultlanguage == 'F') {
		ladmin_log("Envoi d'un requête au serveur de logins pour modifier la date d'un bannissement." RETCODE);
	} else {
		ladmin_log("Request to login-server to modify a ban date/time." RETCODE);
	}

	WPACKETW( 0) = 0x794c;
	strncpy(WPACKETP(2), name, 24);
	WPACKETW(26) = (short)year;
	WPACKETW(28) = (short)month;
	WPACKETW(30) = (short)day;
	WPACKETW(32) = (short)hour;
	WPACKETW(34) = (short)minute;
	WPACKETW(36) = (short)second;
	SENDPACKET(login_fd, 38);
	bytes_to_read = 1;

	return;
}

//-----------------------------------------------------------------------
// Sub-function of sub-function banaccount, unbanaccount or bansetaccount
// Set the final date of a banishment of an account
//-----------------------------------------------------------------------
static void bansetaccountsub(char* name, char* date, char* time_txt) { // not inline, called too often
	int year, month, day, hour, minute, second;
	time_t ban_until_time; // # of seconds 1/1/1970 (timestamp): ban time limit of the account (0 = no ban)
	struct tm *tmtime;

	year = month = day = hour = minute = second = 0;
	ban_until_time = 0;
	tmtime = localtime(&ban_until_time); // initialize

	if (verify_accountname(name) == 0) {
		return;
	}

	if (atoi(date) != 0 &&
	    ((sscanf(date, "%d/%d/%d", &year, &month, &day) < 3 &&
	      sscanf(date, "%d-%d-%d", &year, &month, &day) < 3 &&
	      sscanf(date, "%d.%d.%d", &year, &month, &day) < 3) ||
	     sscanf(time_txt, "%d:%d:%d", &hour, &minute, &second) < 3)) {
		if (defaultlanguage == 'F') {
			printf("Entrez une date et une heure svp (format: aaaa/mm/jj hh:mm:ss).\n");
			printf("Vous pouvez aussi mettre 0 à la place si vous utilisez la commande 'banset'.\n");
			ladmin_log("Format incorrect pour la date/heure (commande'banset' ou 'ban')." RETCODE);
		} else {
			printf("Please input a date and a time (format: yyyy/mm/dd hh:mm:ss).\n");
			printf("You can imput 0 instead of if you use 'banset' command.\n");
			ladmin_log("Invalid format for the date/time ('banset' or 'ban' command)." RETCODE);
		}
		return;
	}

	if (atoi(date) == 0) {
		ban_until_time = 0;
	} else {
		if (year < 70) {
			year = year + 100;
		}
		if (year >= 1900) {
			year = year - 1900;
		}
		if (month < 1 || month > 12) {
			if (defaultlanguage == 'F') {
				printf("Entrez un mois correct svp (entre 1 et 12).\n");
				ladmin_log("Mois incorrect pour la date (command 'banset' ou 'ban')." RETCODE);
			} else {
				printf("Please give a correct value for the month (from 1 to 12).\n");
				ladmin_log("Invalid month for the date ('banset' or 'ban' command)." RETCODE);
			}
			return;
		}
		month = month - 1;
		if (day < 1 || day > 31) {
			if (defaultlanguage == 'F') {
				printf("Entrez un jour correct svp (entre 1 et 31).\n");
				ladmin_log("Jour incorrect pour la date (command 'banset' ou 'ban')." RETCODE);
			} else {
				printf("Please give a correct value for the day (from 1 to 31).\n");
				ladmin_log("Invalid day for the date ('banset' or 'ban' command)." RETCODE);
			}
			return;
		}
		if (((month == 3 || month == 5 || month == 8 || month == 10) && day > 30) ||
		    (month == 1 && day > 29)) {
			if (defaultlanguage == 'F') {
				printf("Entrez un jour correct en fonction du mois (%d) svp.\n", month);
				ladmin_log("Jour incorrect pour ce mois correspondant (command 'banset' ou 'ban')." RETCODE);
			} else {
				printf("Please give a correct value for a day of this month (%d).\n", month);
				ladmin_log("Invalid day for this month ('banset' or 'ban' command)." RETCODE);
			}
			return;
		}
		if (hour < 0 || hour > 23) {
			if (defaultlanguage == 'F') {
				printf("Entrez une heure correcte svp (entre 0 et 23).\n");
				ladmin_log("Heure incorrecte pour l'heure (command 'banset' ou 'ban')." RETCODE);
			} else {
				printf("Please give a correct value for the hour (from 0 to 23).\n");
				ladmin_log("Invalid hour for the time ('banset' or 'ban' command)." RETCODE);
			}
			return;
		}
		if (minute < 0 || minute > 59) {
			if (defaultlanguage == 'F') {
				printf("Entrez des minutes correctes svp (entre 0 et 59).\n");
				ladmin_log("Minute incorrecte pour l'heure (command 'banset' ou 'ban')." RETCODE);
			} else {
				printf("Please give a correct value for the minutes (from 0 to 59).\n");
				ladmin_log("Invalid minute for the time ('banset' or 'ban' command)." RETCODE);
			}
			return;
		}
		if (second < 0 || second > 59) {
			if (defaultlanguage == 'F') {
				printf("Entrez des secondes correctes svp (entre 0 et 59).\n");
				ladmin_log("Seconde incorrecte pour l'heure (command 'banset' ou 'ban')." RETCODE);
			} else {
				printf("Please give a correct value for the seconds (from 0 to 59).\n");
				ladmin_log("Invalid second for the time ('banset' or 'ban' command)." RETCODE);
			}
			return;
		}
		tmtime->tm_year = year;
		tmtime->tm_mon = month;
		tmtime->tm_mday = day;
		tmtime->tm_hour = hour;
		tmtime->tm_min = minute;
		tmtime->tm_sec = second;
		tmtime->tm_isdst = -1; // -1: no winter/summer time modification
		ban_until_time = mktime(tmtime);
		if (ban_until_time == -1) {
			if (defaultlanguage == 'F') {
				printf("Date incorrecte.\n");
				printf("Entrez une date et une heure svp (format: aaaa/mm/jj hh:mm:ss).\n");
				printf("Vous pouvez aussi mettre 0 à la place si vous utilisez la commande 'banset'.\n");
				ladmin_log("Date incorrecte. (command 'banset' ou 'ban')." RETCODE);
			} else {
				printf("Invalid date.\n");
				printf("Please input a date and a time (format: yyyy/mm/dd hh:mm:ss).\n");
				printf("You can imput 0 instead of if you use 'banset' command.\n");
				ladmin_log("Invalid date. ('banset' or 'ban' command)." RETCODE);
			}
			return;
		}
	}

	if (defaultlanguage == 'F') {
		ladmin_log("Envoi d'un requête au serveur de logins pour fixer un ban." RETCODE);
	} else {
		ladmin_log("Request to login-server to set a ban." RETCODE);
	}

	WPACKETW(0) = 0x794a;
	strncpy(WPACKETP(2), name, 24);
	WPACKETL(26) = (int)ban_until_time;
	SENDPACKET(login_fd, 30);
	bytes_to_read = 1;

	return;
}

//---------------------------------------------------------------------
// Sub-function: Set the final date of a banishment of an account (ban)
//---------------------------------------------------------------------
static inline void banaccount(char* param) {
	char name[1023], date[1023], time_txt[1023];

	memset(name, 0, sizeof(name));
	memset(date, 0, sizeof(date));
	memset(time_txt, 0, sizeof(time_txt));

	if (sscanf(param, "%s %s \"%[^\"]\"", date, time_txt, name) < 3 &&
	    sscanf(param, "%s %s '%[^']'", date, time_txt, name) < 3 &&
	    sscanf(param, "%s %s %[^\r\n]", date, time_txt, name) < 3) {
		if (defaultlanguage == 'F') {
			printf("Entrez un nom de compte, une date et une heure svp.\n");
			printf("<exemple>: banset <nom_du_compte> aaaa/mm/jj [hh:mm:ss]\n");
			printf("           banset <nom_du_compte> 0    (0 = dé-bani)\n");
			printf("           ban/banish aaaa/mm/jj hh:mm:ss <nom du compte>\n");
			printf("           unban/unbanish <nom du compte>\n");
			printf("           Heure par défaut [hh:mm:ss]: 23:59:59.\n");
			ladmin_log("Nombre incorrect de paramètres pour fixer un ban (commande 'banset' ou 'ban')." RETCODE);
		} else {
			printf("Please input an account name, a date and a hour.\n");
			printf("<example>: banset <account_name> yyyy/mm/dd [hh:mm:ss]\n");
			printf("           banset <account_name> 0   (0 = un-banished)\n");
			printf("           ban/banish yyyy/mm/dd hh:mm:ss <account name>\n");
			printf("           unban/unbanish <account name>\n");
			printf("           Default time [hh:mm:ss]: 23:59:59.\n");
			ladmin_log("Incomplete parameters to set a ban ('banset' or 'ban' command)." RETCODE);
		}
		return;
	}

	bansetaccountsub(name, date, time_txt);

	return;
}

//------------------------------------------------------------------------
// Sub-function: Set the final date of a banishment of an account (banset)
//------------------------------------------------------------------------
static inline void bansetaccount(char* param) {
	char name[1023], date[1023], time_txt[1023];

	memset(name, 0, sizeof(name));
	memset(date, 0, sizeof(date));
	memset(time_txt, 0, sizeof(time_txt));

	if (sscanf(param, "\"%[^\"]\" %s %[^\r\n]", name, date, time_txt) < 2 && // if date = 0, time can be void
	    sscanf(param, "'%[^']' %s %[^\r\n]", name, date, time_txt) < 2 && // if date = 0, time can be void
	    sscanf(param, "%s %s %[^\r\n]", name, date, time_txt) < 2) { // if date = 0, time can be void
		if (defaultlanguage == 'F') {
			printf("Entrez un nom de compte, une date et une heure svp.\n");
			printf("<exemple>: banset <nom_du_compte> aaaa/mm/jj [hh:mm:ss]\n");
			printf("           banset <nom_du_compte> 0    (0 = dé-bani)\n");
			printf("           ban/banish aaaa/mm/jj hh:mm:ss <nom du compte>\n");
			printf("           unban/unbanish <nom du compte>\n");
			printf("           Heure par défaut [hh:mm:ss]: 23:59:59.\n");
			ladmin_log("Nombre incorrect de paramètres pour fixer un ban (commande 'banset' ou 'ban')." RETCODE);
		} else {
			printf("Please input an account name, a date and a hour.\n");
			printf("<example>: banset <account_name> yyyy/mm/dd [hh:mm:ss]\n");
			printf("           banset <account_name> 0   (0 = un-banished)\n");
			printf("           ban/banish yyyy/mm/dd hh:mm:ss <account name>\n");
			printf("           unban/unbanish <account name>\n");
			printf("           Default time [hh:mm:ss]: 23:59:59.\n");
			ladmin_log("Incomplete parameters to set a ban ('banset' or 'ban' command)." RETCODE);
		}
		return;
	}

	if (time_txt[0] == '\0')
		strcpy(time_txt, "23:59:59");

	bansetaccountsub(name, date, time_txt);

	return;
}

//-------------------------------------------------
// Sub-function: unbanishment of an account (unban)
//-------------------------------------------------
static inline void unbanaccount(char* param) {
	char name[1023];

	memset(name, 0, sizeof(name));

	if (param[0] == '\0' ||
	    (sscanf(param, "\"%[^\"]\"", name) < 1 &&
	     sscanf(param, "'%[^']'", name) < 1 &&
	     sscanf(param, "%[^\r\n]", name) < 1) ||
	     name[0] == '\0') {
		if (defaultlanguage == 'F') {
			printf("Entrez un nom de compte svp.\n");
			printf("<exemple>: banset <nom_du_compte> aaaa/mm/jj [hh:mm:ss]\n");
			printf("           banset <nom_du_compte> 0    (0 = dé-bani)\n");
			printf("           ban/banish aaaa/mm/jj hh:mm:ss <nom du compte>\n");
			printf("           unban/unbanish <nom du compte>\n");
			printf("           Heure par défaut [hh:mm:ss]: 23:59:59.\n");
			ladmin_log("Nombre incorrect de paramètres pour fixer un ban (commande 'unban')." RETCODE);
		} else {
			printf("Please input an account name.\n");
			printf("<example>: banset <account_name> yyyy/mm/dd [hh:mm:ss]\n");
			printf("           banset <account_name> 0   (0 = un-banished)\n");
			printf("           ban/banish yyyy/mm/dd hh:mm:ss <account name>\n");
			printf("           unban/unbanish <account name>\n");
			printf("           Default time [hh:mm:ss]: 23:59:59.\n");
			ladmin_log("Incomplete parameters to set a ban ('unban' command)." RETCODE);
		}
		return;
	}

	bansetaccountsub(name, "0", "");

	return;
}

//---------------------------------------------------------
// Sub-function: Asking to check the validity of a password
// (Note: never send back a password with login-server!! security of passwords)
//---------------------------------------------------------
static inline void checkaccount(char* param) {
	char name[1023], password[1023];

	memset(name, 0, sizeof(name));
	memset(password, 0, sizeof(password));

	if (sscanf(param, "\"%[^\"]\" %[^\r\n]", name, password) < 1 && // password can be void
	    sscanf(param, "'%[^']' %[^\r\n]", name, password) < 1 && // password can be void
	    sscanf(param, "%s %[^\r\n]", name, password) < 1) { // password can be void
		if (defaultlanguage == 'F') {
			printf("Entrez un nom de compte svp.\n");
			printf("<exemple> check testname motdepasse\n");
			ladmin_log("Nombre incorrect de paramètres pour tester le mot d'un passe d'un compte (commande 'check')." RETCODE);
		} else {
			printf("Please input an account name.\n");
			printf("<example> check testname password\n");
			ladmin_log("Incomplete parameters to check the password of an account ('check' command)." RETCODE);
		}
		return;
	}

	if (verify_accountname(name) == 0) {
		return;
	}

	if (password[0] == '\0') {
		if (typepasswd(password) == 0)
			return;
	}
	if (verify_password(password) == 0)
		return;

	if (defaultlanguage == 'F') {
		ladmin_log("Envoi d'un requête au serveur de logins pour test un mot de passe." RETCODE);
	} else {
		ladmin_log("Request to login-server to check a password." RETCODE);
	}

	WPACKETW(0) = 0x793a;
	strncpy(WPACKETP( 2), name, 24);
	strncpy(WPACKETP(26), password, 24);
	SENDPACKET(login_fd, 50);
	bytes_to_read = 1;

	return;
}

//------------------------------------------------
// Sub-function: Asking for deletion of an account
//------------------------------------------------
static inline void delaccount(char* param) {
	char name[1023];
	char letter;
	char confirm[1023];
	int i;

	memset(name, 0, sizeof(name));

	if (param[0] == '\0' ||
	    (sscanf(param, "\"%[^\"]\"", name) < 1 &&
	     sscanf(param, "'%[^']'", name) < 1 &&
	     sscanf(param, "%[^\r\n]", name) < 1) ||
	     name[0] == '\0') {
		if (defaultlanguage == 'F') {
			printf("Entrez un nom de compte svp.\n");
			printf("<exemple> del nomtestasupprimer\n");
			ladmin_log("Aucun nom donné pour supprimer un compte (commande 'delete')." RETCODE);
		} else {
			printf("Please input an account name.\n");
			printf("<example> del testnametodelete\n");
			ladmin_log("No name given to delete an account ('delete' command)." RETCODE);
		}
		return;
	}

	if (verify_accountname(name) == 0) {
		return;
	}

	memset(confirm, 0, sizeof(confirm));
	while ((confirm[0] != 'o' || defaultlanguage != 'F') && confirm[0] != 'n' && (confirm[0] != 'y' || defaultlanguage == 'F')) {
		if (defaultlanguage == 'F')
			printf(CL_CYAN " ** Etes-vous vraiment sûr de vouloir SUPPRIMER le compte [%s]? (o/n) > " CL_RESET, name);
		else
			printf(CL_CYAN " ** Are you really sure to DELETE account [%s]? (y/n) > " CL_RESET, name);
		fflush(stdout);
		memset(confirm, 0, sizeof(confirm));
		i = 0;
		while ((letter = getchar()) != '\n')
			confirm[i++] = letter;
	}

	if (confirm[0] == 'n') {
		if (defaultlanguage == 'F') {
			printf("Suppression annulée.\n");
			ladmin_log("Suppression annulée par l'utilisateur (commande 'delete')." RETCODE);
		} else {
			printf("Deletion canceled.\n");
			ladmin_log("Deletion canceled by user ('delete' command)." RETCODE);
		}
		return;
	}

	if (defaultlanguage == 'F') {
		ladmin_log("Envoi d'un requête au serveur de logins pour détruire un compte." RETCODE);
	} else {
		ladmin_log("Request to login-server to delete an acount." RETCODE);
	}

	WPACKETW(0) = 0x7932;
	strncpy(WPACKETP(2), name, 24);
	SENDPACKET(login_fd, 26);
	bytes_to_read = 1;

	return;
}

//----------------------------------------------------------
// Sub-function: Asking to modification of an account e-mail
//----------------------------------------------------------
static inline void changeemail(char* param) {
	char name[1023], email[1023];

	memset(name, 0, sizeof(name));
	memset(email, 0, sizeof(email));

	if (sscanf(param, "\"%[^\"]\" %[^\r\n]", name, email) < 2 &&
	    sscanf(param, "'%[^']' %[^\r\n]", name, email) < 2 &&
	    sscanf(param, "%s %[^\r\n]", name, email) < 2) {
		if (defaultlanguage == 'F') {
			printf("Entrez un nom de compte et une email svp.\n");
			printf("<exemple> email testname nouveauemail\n");
			ladmin_log("Nombre incorrect de paramètres pour changer l'email d'un compte (commande 'email')." RETCODE);
		} else {
			printf("Please input an account name and an email.\n");
			printf("<example> email testname newemail\n");
			ladmin_log("Incomplete parameters to change the email of an account ('email' command)." RETCODE);
		}
		return;
	}

	if (verify_accountname(name) == 0) {
		return;
	}

	if (strlen(email) < 3) {
		if (defaultlanguage == 'F') {
			printf("Email trop courte [%s]. Entrez une e-mail valide svp.\n", email);
			ladmin_log("Email trop courte [%s]. Entrez une e-mail valide svp." RETCODE, email);
		} else {
			printf("Email is too short [%s]. Please input a valid e-mail.\n", email);
			ladmin_log("Email is too short [%s]. Please input a valid e-mail." RETCODE, email);
		}
		return;
	}
	if (strlen(email) > 40) {
		if (defaultlanguage == 'F') {
			printf("Email trop longue [%s]. Entrez une e-mail de 40 caractères maximum svp.\n", email);
			ladmin_log("Email trop longue [%s]. Entrez une e-mail de 40 caractères maximum svp." RETCODE, email);
		} else {
			printf("Email is too long [%s]. Please input an e-mail with 40 bytes at the most.\n", email);
			ladmin_log("Email is too long [%s]. Please input an e-mail with 40 bytes at the most." RETCODE, email);
		}
		return;
	}
	if (e_mail_check(email) == 0) {
		if (defaultlanguage == 'F') {
			printf("Email incorrecte [%s]. Entrez une e-mail valide svp.\n", email);
			ladmin_log("Email incorrecte [%s]. Entrez une e-mail valide svp." RETCODE, email);
		} else {
			printf("Invalid email [%s]. Please input a valid e-mail.\n", email);
			ladmin_log("Invalid email [%s]. Please input a valid e-mail." RETCODE, email);
		}
		return;
	}

	if (defaultlanguage == 'F') {
		ladmin_log("Envoi d'un requête au serveur de logins pour changer une email." RETCODE);
	} else {
		ladmin_log("Request to login-server to change an email." RETCODE);
	}

	WPACKETW(0) = 0x7940;
	strncpy(WPACKETP( 2), name, 24);
	strncpy(WPACKETP(26), email, 40);
	SENDPACKET(login_fd, 66);
	bytes_to_read = 1;

	return;
}

//-----------------------------------------------------
// Sub-function: Asking of the number of online players
//-----------------------------------------------------
static inline void getlogincount() {
	if (defaultlanguage == 'F') {
		ladmin_log("Envoi d'un requête au serveur de logins pour obtenir le nombre de joueurs en jeu." RETCODE);
	} else {
		ladmin_log("Request to login-server to obtain the # of online players." RETCODE);
	}

	WPACKETW(0) = 0x7938;
	SENDPACKET(login_fd, 2);
	bytes_to_read = 1;

	return;
}

//----------------------------------------------------------
// Sub-function: Asking to modify the GM level of an account
//----------------------------------------------------------
static inline void changegmlevel(char* param) {
	char name[1023];
	int GM_level;

	memset(name, 0, sizeof(name));
	GM_level = 0;

	if (sscanf(param, "\"%[^\"]\" %d", name, &GM_level) < 1 &&
	    sscanf(param, "'%[^']' %d", name, &GM_level) < 1 &&
	    sscanf(param, "%s %d", name, &GM_level) < 1) {
		if (defaultlanguage == 'F') {
			printf("Entrez un nom de compte et un niveau de GM svp.\n");
			printf("<exemple> gm nomtest 80\n");
			ladmin_log("Nombre incorrect de paramètres pour changer le Niveau de GM d'un compte (commande 'gm')." RETCODE);
		} else {
			printf("Please input an account name and a GM level.\n");
			printf("<example> gm testname 80\n");
			ladmin_log("Incomplete parameters to change the GM level of an account ('gm' command)." RETCODE);
		}
		return;
	}

	if (verify_accountname(name) == 0) {
		return;
	}

	if (GM_level < 0 || GM_level > 99) {
		if (defaultlanguage == 'F') {
			printf("Niveau de GM incorrect [%d]. Entrez une valeur de 0 à 99 svp.\n", GM_level);
			ladmin_log("Niveau de GM incorrect [%d]. La valeur peut être de 0 à 99." RETCODE, GM_level);
		} else {
			printf("Illegal GM level [%d]. Please input a value from 0 to 99.\n", GM_level);
			ladmin_log("Illegal GM level [%d]. The value can be from 0 to 99." RETCODE, GM_level);
		}
		return;
	}

	if (defaultlanguage == 'F') {
		ladmin_log("Envoi d'un requête au serveur de logins pour changer un niveau de GM." RETCODE);
	} else {
		ladmin_log("Request to login-server to change a GM level." RETCODE);
	}

	WPACKETW( 0) = 0x793e;
	strncpy(WPACKETP(2), name, 24);
	WPACKETB(26) = GM_level;
	SENDPACKET(login_fd, 27);
	bytes_to_read = 1;

	return;
}

//---------------------------------------------
// Sub-function: Asking to obtain an account id
//---------------------------------------------
static inline void idaccount(char* param) {
	char name[1023];

	memset(name, 0, sizeof(name));

	if (param[0] == '\0' ||
	    (sscanf(param, "\"%[^\"]\"", name) < 1 &&
	     sscanf(param, "'%[^']'", name) < 1 &&
	     sscanf(param, "%[^\r\n]", name) < 1) ||
	     name[0] == '\0') {
		if (defaultlanguage == 'F') {
			printf("Entrez un nom de compte svp.\n");
			printf("<exemple> id nomtest\n");
			ladmin_log("Aucun nom donné pour rechecher l'id d'un compte (commande 'id')." RETCODE);
		} else {
			printf("Please input an account name.\n");
			printf("<example> id testname\n");
			ladmin_log("No name given to search an account id ('id' command)." RETCODE);
		}
		return;
	}

	if (verify_accountname(name) == 0) {
		return;
	}

	if (defaultlanguage == 'F') {
		ladmin_log("Envoi d'un requête au serveur de logins pour connaître l'id d'un compte." RETCODE);
	} else {
		ladmin_log("Request to login-server to know an account id." RETCODE);
	}

	WPACKETW(0) = 0x7944;
	strncpy(WPACKETP(2), name, 24);
	SENDPACKET(login_fd, 26);
	bytes_to_read = 1;

	return;
}

//----------------------------------------------------------------------------
// Sub-function: Asking to displaying information about an account (by its id)
//----------------------------------------------------------------------------
static inline void infoaccount(char* param) {
	int account_id;

	if (sscanf(param, "%d", &account_id) < 1) {
		if (defaultlanguage == 'F') {
			printf("Entrez un id de compte derrière la commande svp.\n");
			printf("Utilisez la commande 'who' si vous cherchez avec un nom.\n");
			ladmin_log("Une valeur n'étant pas un id de compte a été donnée pour la commande 'info'." RETCODE);
		} else {
			printf("Please input a account id as parameter.\n");
			printf("Use 'who' command if you search with a name.\n");
			ladmin_log("A non account id value was given for the 'info' command." RETCODE);
		}
		return;
	}

	if (account_id < 0) {
		if (defaultlanguage == 'F') {
			printf("Entrez un id ayant une valeur positive svp.\n");
			ladmin_log("Une valeur négative a été donné pour trouver le compte." RETCODE);
		} else {
			printf("Please input a positive value for the id.\n");
			ladmin_log("Negative value was given to found the account." RETCODE);
		}
		return;
	}

	if (defaultlanguage == 'F') {
		ladmin_log("Envoi d'un requête au serveur de logins pour obtenir le information d'un compte (par l'id)." RETCODE);
	} else {
		ladmin_log("Request to login-server to obtain information about an account (by its id)." RETCODE);
	}

	WPACKETW(0) = 0x7954;
	WPACKETL(2) = account_id;
	SENDPACKET(login_fd, 6);
	bytes_to_read = 1;

	return;
}

//---------------------------------------
// Sub-function: Send a broadcast message
//---------------------------------------
static void sendbroadcast(short type, char* message) { // not inline, called too often
	if (message[0] == '\0') {
		if (defaultlanguage == 'F') {
			printf("Entrez un message svp.\n");
			if (type == 0) {
				printf("<exemple> kami un message\n");
			} else {
				printf("<exemple> kamib un message\n");
			}
			ladmin_log("Le message est vide (commande 'kami(b)')." RETCODE);
		} else {
			printf("Please input a message.\n");
			if (type == 0) {
				printf("<example> kami a message\n");
			} else {
				printf("<example> kamib a message\n");
			}
			ladmin_log("The message is void ('kami(b)' command)." RETCODE);
		}
		return;
	}

	WPACKETW(0) = 0x794e;
	WPACKETW(2) = 6 + strlen(message) + 1; /* + NULL */
	WPACKETW(4) = type;
	strncpy(WPACKETP(6), message, strlen(message));
	WPACKETB(6 + strlen(message) + 1 - 1) = 0; // NULL
	SENDPACKET(login_fd, 6 + strlen(message) + 1); /* + NULL */
	bytes_to_read = 1;

	return;
}

//--------------------------------------------
// Sub-function: Change language of displaying
//--------------------------------------------
static inline void changelanguage(char* language) {
	if (language[0] == '\0') {
		if (defaultlanguage == 'F') {
			printf("Entrez une langue svp.\n");
			printf("<exemple> language english\n");
			printf("          language français\n");
			ladmin_log("La langue est vide (commande 'language')." RETCODE);
		} else {
			printf("Please input a language.\n");
			printf("<example> language english\n");
			printf("          language français\n");
			ladmin_log("The language is void ('language' command)." RETCODE);
		}
		return;
	}

	language[0] = toupper((unsigned char)language[0]); // toupper needs unsigned char
	if (language[0] == 'F' || language[0] == 'E') {
		defaultlanguage = language[0];
		if (defaultlanguage == 'F') {
			printf("Changement de la langue d'affichage en Français.\n");
			ladmin_log("Changement de la langue d'affichage en Français." RETCODE);
		} else {
			printf("Displaying language changed to English.\n");
			ladmin_log("Displaying language changed to English." RETCODE);
		}
	} else {
		if (defaultlanguage == 'F') {
			printf("Langue non paramétrée (langues possibles: 'Français' ou 'English').\n");
			ladmin_log("Langue non paramétrée (Français ou English nécessaire)." RETCODE);
		} else {
			printf("Undefined language (possible languages: Français or English).\n");
			ladmin_log("Undefined language (must be Français or English)." RETCODE);
		}
	}

	return;
}

//--------------------------------------------------------
// Sub-function: Asking to Displaying of the accounts list
//--------------------------------------------------------
static void listaccount(char* param, int type) { // not inline, called too often
//int list_first, list_last, list_type; // parameter to display a list of accounts
	int i;

	list_type = type;

	// set default values
	list_first = 0;
	list_last = 2147483647;

	if (list_type == 2) { // if search
		for (i = 0; param[i]; i++)
			param[i] = tolower((unsigned char)param[i]); // tolower needs unsigned char
		// get all accounts = use default
	} else { // if list (list_type == 0)
	         // if listgm (list_type == 1)
	         // if listban (list_type == 3)
	         // if listok (list_type == 4)
	         // if listonline (list_type == 5)
		switch(sscanf(param, "%d %d", &list_first, &list_last)) {
		case 0:
			// get all accounts = use default
			break;
		case 1:
			list_last = 2147483647;
			// use tests of the following value
		default:
			if (list_first < 0)
				list_first = 0;
			if (list_last < list_first || list_last < 0)
				list_last = 2147483647;
			break;
		}
	}

	if (defaultlanguage == 'F') {
		ladmin_log("Envoi d'un requête au serveur de logins pour obtenir la liste des comptes de %d à %d." RETCODE, list_first, list_last);
	} else {
		ladmin_log("Request to login-server to obtain the list of accounts from %d to %d." RETCODE, list_first, list_last);
	}

	WPACKETW( 0) = 0x7920;
	WPACKETL( 2) = list_first;
	WPACKETL( 6) = list_last;
	WPACKETB(10) = (list_type == 2) ? 0 : list_type; /* 0: any accounts, 1: gm only, 3: accounts with state or bannished, 4: accounts without state and not bannished */
	SENDPACKET(login_fd, 11);
	bytes_to_read = 1;

	//          0 0123456789 01 012345678901234567890123012 012345 0123456789012345678901234567
	if (defaultlanguage == 'F') {
		printf("S'il y a une '*', le joueur est online.\n");
		printf("|\n");
		printf("*  id_compte GM nom_utilisateur         sex  count statut\n");
	} else {
		printf("If a '*' is set, the player is online.\n");
		printf("|\n");
		printf("* account_id GM user_name               sex  count state\n");
	}
	printf("-------------------------------------------------------------------------------\n");
	list_count = 0;

	return;
}

//--------------------------------------------
// Sub-function: Asking to modify a memo field
//--------------------------------------------
static inline void changememo(char* param) {
	char name[1023], memo[65536];
	int memo_length; /* speed up */

	memset(name, 0, sizeof(name));
	memset(memo, 0, sizeof(memo));

	if (sscanf(param, "\"%[^\"]\" %[^\r\n]", name, memo) < 1 && // memo can be void
	    sscanf(param, "'%[^']' %[^\r\n]", name, memo) < 1 && // memo can be void
	    sscanf(param, "%s %[^\r\n]", name, memo) < 1) { // memo can be void
		if (defaultlanguage == 'F') {
			printf("Entrez un nom de compte et un mémo svp.\n");
			printf("<exemple> memo nomtest nouveau memo\n");
			ladmin_log("Nombre incorrect de paramètres pour changer le mémo d'un compte (commande 'email')." RETCODE);
		} else {
			printf("Please input an account name and a memo.\n");
			printf("<example> memo testname new memo\n");
			ladmin_log("Incomplete parameters to change the memo of an account ('email' command)." RETCODE);
		}
		return;
	}

	if (verify_accountname(name) == 0) {
		return;
	}

	memo_length = strlen(memo);

	if (memo_length > 60000) {
		if (defaultlanguage == 'F') {
			printf("Mémo trop long (%d caractères).\n", memo_length);
			printf("Entrez un mémo de 60000 caractères maximum svp.\n");
			ladmin_log("Mémo trop long (%d caractères). Entrez un mémo de 60000 caractères maximum svp." RETCODE, memo_length);
		} else {
			printf("Memo is too long (%d characters).\n", memo_length);
			printf("Please input a memo of 60000 bytes at the maximum.\n");
			ladmin_log("Memo is too long (%d characters). Please input a memo of 60000 bytes at the maximum." RETCODE, memo_length);
		}
		return;
	}

	if (defaultlanguage == 'F') {
		ladmin_log("Envoi d'un requête au serveur de logins pour changer un mémo." RETCODE);
	} else {
		ladmin_log("Request to login-server to change a memo." RETCODE);
	}

	WPACKETW(0) = 0x7942;
	WPACKETW(2) = 28 + memo_length;
	strncpy(WPACKETP(4), name, 24);
	if (memo_length > 0)
		strncpy(WPACKETP(28), memo, memo_length);
	SENDPACKET(login_fd, 28 + memo_length);
	bytes_to_read = 1;

	return;
}

//---------------------------------------------------
// Sub-function: Asking to add a text to a memo field
//---------------------------------------------------
static inline void addmemo(char* param) {
	char name[1023], memo[65536];
	int memo_length; /* speed up */

	memset(name, 0, sizeof(name));
	memset(memo, 0, sizeof(memo));

	if (sscanf(param, "\"%[^\"]\" %[^\r\n]", name, memo) < 1 && // memo can be void
	    sscanf(param, "'%[^']' %[^\r\n]", name, memo) < 1 && // memo can be void
	    sscanf(param, "%s %[^\r\n]", name, memo) < 1) { // memo can be void
		if (defaultlanguage == 'F') {
			printf("Entrez un nom de compte et un mémo à ajouter svp.\n");
			printf("<exemple> memoadd nomtest texte du memo à ajouter\n");
			ladmin_log("Nombre incorrect de paramètres pour ajouter un texte au mémo d'un compte (commande 'email')." RETCODE);
		} else {
			printf("Please input an account name and a memo to add.\n");
			printf("<example> memoadd testname memo text to add\n");
			ladmin_log("Incomplete parameters to add a text to the memo of an account ('email' command)." RETCODE);
		}
		return;
	}

	if (verify_accountname(name) == 0) {
		return;
	}

	memo_length = strlen(memo);

	if (memo_length > 60000) {
		if (defaultlanguage == 'F') {
			printf("Mémo trop long (%d caractères).\n", memo_length);
			printf("Entrez un mémo de 60000 caractères maximum svp.\n");
			ladmin_log("Mémo trop long (%d caractères). Entrez un mémo de 60000 caractères maximum svp." RETCODE, memo_length);
		} else {
			printf("Memo is too long (%d characters).\n", memo_length);
			printf("Please input a memo of 60000 bytes at the maximum.\n");
			ladmin_log("Memo is too long (%d characters). Please input a memo of 60000 bytes at the maximum." RETCODE, memo_length);
		}
		return;
	} else if (memo_length == 0) {
		if (defaultlanguage == 'F') {
			printf("Mémo à ajouter trop court (0 caractères).\n");
			printf("Entrez un mémo à ajouter svp.\n");
			ladmin_log("Mémo à ajouter trop court (0 caractères). Entrez un mémo à ajouter svp." RETCODE);
		} else {
			printf("Memo to add is too short (0 character).\n");
			printf("Please input a memo to add.\n");
			ladmin_log("Memo to add is too short (0 character). Please input a memo to add." RETCODE);
		}
		return;
	}

	if (defaultlanguage == 'F') {
		ladmin_log("Envoi d'un requête au serveur de logins pour ajouter un texte à un mémo." RETCODE);
	} else {
		ladmin_log("Request to login-server to add a text to a memo." RETCODE);
	}

	WPACKETW(0) = 0x7955;
	WPACKETW(2) = 28 + memo_length;
	strncpy(WPACKETP(4), name, 24);
	if (memo_length > 0)
		strncpy(WPACKETP(28), memo, memo_length);
	SENDPACKET(login_fd, 28 + memo_length);
	bytes_to_read = 1;

	return;
}

//-----------------------------------------------
// Sub-function: Asking to obtain an account name
//-----------------------------------------------
static inline void nameaccount(int id) {
	if (id < 0) {
		if (defaultlanguage == 'F') {
			printf("Entrez un id ayant une valeur positive svp.\n");
			ladmin_log("Id négatif donné pour rechecher le nom d'un compte (commande 'name')." RETCODE);
		} else {
			printf("Please input a positive value for the id.\n");
			ladmin_log("Negativ id given to search an account name ('name' command)." RETCODE);
		}
		return;
	}

	if (defaultlanguage == 'F')
		ladmin_log("Envoi d'un requête au serveur de logins pour connaître le nom d'un compte." RETCODE);
	else
		ladmin_log("Request to login-server to know an account name." RETCODE);

	WPACKETW(0) = 0x7946;
	WPACKETL(2) = id;
	SENDPACKET(login_fd, 6);
	bytes_to_read = 1;

	return;
}

//------------------------------------------
// Sub-function: Asking to modify a password
// (Note: never send back a password with login-server!! security of passwords)
//------------------------------------------
static inline void changepasswd(char* param) {
	char name[1023], password[1023];

	memset(name, 0, sizeof(name));
	memset(password, 0, sizeof(password));

	if (sscanf(param, "\"%[^\"]\" %[^\r\n]", name, password) < 1 &&
	    sscanf(param, "'%[^']' %[^\r\n]", name, password) < 1 &&
	    sscanf(param, "%s %[^\r\n]", name, password) < 1) {
		if (defaultlanguage == 'F') {
			printf("Entrez un nom de compte svp.\n");
			printf("<exemple> passwd nomtest nouveaumotdepasse\n");
			ladmin_log("Nombre incorrect de paramètres pour changer le mot d'un passe d'un compte (commande 'password')." RETCODE);
		} else {
			printf("Please input an account name.\n");
			printf("<example> passwd testname newpassword\n");
			ladmin_log("Incomplete parameters to change the password of an account ('password' command)." RETCODE);
		}
		return;
	}

	if (verify_accountname(name) == 0) {
		return;
	}

	if (password[0] == '\0') {
		if (typepasswd(password) == 0)
			return;
	}
	if (verify_password(password) == 0)
		return;

	if (defaultlanguage == 'F') {
		ladmin_log("Envoi d'un requête au serveur de logins pour changer un mot de passe." RETCODE);
	} else {
		ladmin_log("Request to login-server to change a password." RETCODE);
	}

	WPACKETW(0) = 0x7934;
	strncpy(WPACKETP( 2), name, 24);
	strncpy(WPACKETP(26), password, 24);
	SENDPACKET(login_fd, 50);
	bytes_to_read = 1;

	return;
}

//-----------------------------------------------------
// Sub-function: Asking to modify the sex of an account
//-----------------------------------------------------
static inline void changesex(char* param) {
	char name[1023], sex[1023];

	memset(name, 0, sizeof(name));
	memset(sex, 0, sizeof(sex));

	if (sscanf(param, "\"%[^\"]\" %[^\r\n]", name, sex) < 2 &&
	    sscanf(param, "'%[^']' %[^\r\n]", name, sex) < 2 &&
	    sscanf(param, "%s %[^\r\n]", name, sex) < 2) {
		if (defaultlanguage == 'F') {
			printf("Entrez un nom de compte et un sexe svp.\n");
			printf("<exemple> sex nomtest Male\n");
			ladmin_log("Nombre incorrect de paramètres pour changer le sexe d'un compte (commande 'sex')." RETCODE);
		} else {
			printf("Please input an account name and a sex.\n");
			printf("<example> sex testname Male\n");
			ladmin_log("Incomplete parameters to change the sex of an account ('sex' command)." RETCODE);
		}
		return;
	}

	if (verify_accountname(name) == 0) {
		return;
	}

	sex[0] = toupper((unsigned char)sex[0]); // toupper needs unsigned char
	if (strchr("MF", sex[0]) == NULL) {
		if (defaultlanguage == 'F') {
			printf("Sexe incorrect [%s]. Entrez M ou F svp.\n", sex);
			ladmin_log("Sexe incorrect [%s]. Entrez M ou F svp." RETCODE, sex);
		} else {
			printf("Illegal gender [%s]. Please input M or F.\n", sex);
			ladmin_log("Illegal gender [%s]. Please input M or F." RETCODE, sex);
		}
		return;
	}

	if (defaultlanguage == 'F') {
		ladmin_log("Envoi d'un requête au serveur de logins pour changer un sexe." RETCODE);
	} else {
		ladmin_log("Request to login-server to change a sex." RETCODE);
	}

	WPACKETW( 0) = 0x793c;
	strncpy(WPACKETP(2), name, 24);
	WPACKETB(26) = sex[0];
	SENDPACKET(login_fd, 27);
	bytes_to_read = 1;

	return;
}

//-------------------------------------------------------------------------
// Sub-function of sub-function changestate, blockaccount or unblockaccount
// Asking to modify the state of an account
//-------------------------------------------------------------------------
static void changestatesub(char* name, int state, char* error_message7) { // not inline, called too often
	char error_message[1023]; // need to use, because we can modify error_message7

	memset(error_message, 0, sizeof(error_message));
	strncpy(error_message, error_message7, sizeof(error_message) - 1);

	if ((state < 0 || state > 9) && state != 100) { // Valid values: 0: ok, or value of the 0x006a packet + 1
		if (defaultlanguage == 'F') {
			printf("Entrez une des statuts suivantes svp:\n");
			printf("  0 = Compte ok             6 = Your Game's EXE file is not the latest version\n");
		} else {
			printf("Please input one of these states:\n");
			printf("  0 = Account ok            6 = Your Game's EXE file is not the latest version\n");
		}
		printf("  1 = Unregistered ID       7 = You are Prohibited to log in until + message\n");
		printf("  2 = Incorrect Password    8 = Server is jammed due to over populated\n");
		printf("  3 = This ID is expired    9 = No MSG\n");
		printf("  4 = Rejected from Server  100 = This ID has been totally erased\n");
		printf("  5 = You have been blocked by the GM Team\n");
		if (defaultlanguage == 'F') {
			printf("<exemples> state nomtest 5\n");
			printf("           state nomtest 7 fin de votre ban\n");
			printf("           block <nom compte>\n");
			printf("           unblock <nom compte>\n");
			ladmin_log("Valeur incorrecte pour le statut d'un compte (commande 'state', 'block' ou 'unblock')." RETCODE);
		} else {
			printf("<examples> state testname 5\n");
			printf("           state testname 7 end of your ban\n");
			printf("           block <account name>\n");
			printf("           unblock <account name>\n");
			ladmin_log("Invalid value for the state of an account ('state', 'block' or 'unblock' command)." RETCODE);
		}
		return;
	}

	if (verify_accountname(name) == 0) {
		return;
	}

	if (state != 7) {
		strcpy(error_message, "-");
	} else {
		if (error_message[0] == '\0') {
			if (defaultlanguage == 'F') {
				printf("Message d'erreur trop court. Entrez un message de 1-19 caractères.\n");
				ladmin_log("Message d'erreur trop court. Entrez un message de 1-19 caractères." RETCODE);
			} else {
				printf("Error message is too short. Please input a message of 1-19 bytes.\n");
				ladmin_log("Error message is too short. Please input a message of 1-19 bytes." RETCODE);
			}
			return;
		}
		if (strlen(error_message) > 19) {
			if (defaultlanguage == 'F') {
				printf("Message d'erreur trop long. Entrez un message de 1-19 caractères.\n");
				ladmin_log("Message d'erreur trop long. Entrez un message de 1-19 caractères." RETCODE);
			} else {
				printf("Error message is too long. Please input a message of 1-19 bytes.\n");
				ladmin_log("Error message is too long. Please input a message of 1-19 bytes." RETCODE);
			}
			return;
		}
	}

	if (defaultlanguage == 'F') {
		ladmin_log("Envoi d'un requête au serveur de logins pour changer un statut." RETCODE);
	} else {
		ladmin_log("Request to login-server to change a state." RETCODE);
	}

	WPACKETW( 0) = 0x7936;
	strncpy(WPACKETP( 2), name, 24);
	WPACKETL(26) = state;
	strncpy(WPACKETP(30), error_message, 20); // 19 + NULL
	SENDPACKET(login_fd, 50);
	bytes_to_read = 1;

	return;
}

//-------------------------------------------------------
// Sub-function: Asking to modify the state of an account
//-------------------------------------------------------
static inline void changestate(char* param) {
	char name[1023], error_message[1023];
	int state;

	memset(name, 0, sizeof(name));
	memset(error_message, 0, sizeof(error_message));

	if (sscanf(param, "\"%[^\"]\" %d %[^\r\n]", name, &state, error_message) < 2 &&
	    sscanf(param, "'%[^']' %d %[^\r\n]", name, &state, error_message) < 2 &&
	    sscanf(param, "%s %d %[^\r\n]", name, &state, error_message) < 2) {
		if (defaultlanguage == 'F') {
			printf("Entrez un nom de compte et un statut svp.\n");
			printf("<exemples> state nomtest 5\n");
			printf("           state nomtest 7 fin de votre ban\n");
			printf("           block <nom compte>\n");
			printf("           unblock <nom compte>\n");
			ladmin_log("Nombre incorrect de paramètres pour changer le statut d'un compte (commande 'state')." RETCODE);
		} else {
			printf("Please input an account name and a state.\n");
			printf("<examples> state testname 5\n");
			printf("           state testname 7 end of your ban\n");
			printf("           block <account name>\n");
			printf("           unblock <account name>\n");
			ladmin_log("Incomplete parameters to change the state of an account ('state' command)." RETCODE);
		}
		return;
	}

	changestatesub(name, state, error_message);

	return;
}

//-------------------------------------------
// Sub-function: Asking to unblock an account
//-------------------------------------------
static inline void unblockaccount(char* param) {
	char name[1023];

	memset(name, 0, sizeof(name));

	if (param[0] == '\0' ||
	    (sscanf(param, "\"%[^\"]\"", name) < 1 &&
	     sscanf(param, "'%[^']'", name) < 1 &&
	     sscanf(param, "%[^\r\n]", name) < 1) ||
	     name[0] == '\0') {
		if (defaultlanguage == 'F') {
			printf("Entrez un nom de compte svp.\n");
			printf("<exemples> state nomtest 5\n");
			printf("           state nomtest 7 fin de votre ban\n");
			printf("           block <nom compte>\n");
			printf("           unblock <nom compte>\n");
			ladmin_log("Nombre incorrect de paramètres pour changer le statut d'un compte (commande 'unblock')." RETCODE);
		} else {
			printf("Please input an account name.\n");
			printf("<examples> state testname 5\n");
			printf("           state testname 7 end of your ban\n");
			printf("           block <account name>\n");
			printf("           unblock <account name>\n");
			ladmin_log("Incomplete parameters to change the state of an account ('unblock' command)." RETCODE);
		}
		return;
	}

	changestatesub(name, 0, "-"); // state 0, no error message

	return;
}

//-------------------------------------------
// Sub-function: Asking to unblock an account
//-------------------------------------------
static inline void blockaccount(char* param) {
	char name[1023];

	memset(name, 0, sizeof(name));

	if (param[0] == '\0' ||
	    (sscanf(param, "\"%[^\"]\"", name) < 1 &&
	     sscanf(param, "'%[^']'", name) < 1 &&
	     sscanf(param, "%[^\r\n]", name) < 1) ||
	     name[0] == '\0') {
		if (defaultlanguage == 'F') {
			printf("Entrez un nom de compte svp.\n");
			printf("<exemples> state nomtest 5\n");
			printf("           state nomtest 7 fin de votre ban\n");
			printf("           block <nom compte>\n");
			printf("           unblock <nom compte>\n");
			ladmin_log("Nombre incorrect de paramètres pour changer le statut d'un compte (commande 'block')." RETCODE);
		} else {
			printf("Please input an account name.\n");
			printf("<examples> state testname 5\n");
			printf("           state testname 7 end of your ban\n");
			printf("           block <account name>\n");
			printf("           unblock <account name>\n");
			ladmin_log("Incomplete parameters to change the state of an account ('block' command)." RETCODE);
		}
		return;
	}

	changestatesub(name, 5, "-"); // state 5, no error message

	return;
}

//---------------------------------------------------------------------
// Sub-function: Add/substract time to the validity limit of an account
//---------------------------------------------------------------------
static inline void timeaddaccount(char* param) {
	char name[1023], modif[1023];
	int year, month, day, hour, minute, second;
	char * p_modif;
	int value, i;

	memset(name, 0, sizeof(name));
	memset(modif, 0, sizeof(modif));
	year = month = day = hour = minute = second = 0;

	if (sscanf(param, "\"%[^\"]\" %[^\r\n]", name, modif) < 2 &&
	    sscanf(param, "'%[^']' %[^\r\n]", name, modif) < 2 &&
	    sscanf(param, "%s %[^\r\n]", name, modif) < 2) {
		if (defaultlanguage == 'F') {
			printf("Entrez un nom de compte et un modificateur svp.\n");
			printf("  <exemple> timeadd nomtest +1m-2mn1s-6y\n");
			printf("            Cette exemple ajoute 1 mois et 1 seconde, et soustrait 2 minutes\n");
			printf("            et 6 ans dans le même temps.\n");
			ladmin_log("Nombre incorrect de paramètres pour modifier une date limite d'utilisation (commande 'timeadd')." RETCODE);
		} else {
			printf("Please input an account name and a modifier.\n");
			printf("  <example>: timeadd testname +1m-2mn1s-6y\n");
			printf("             this example adds 1 month and 1 second, and substracts 2 minutes\n");
			printf("             and 6 years at the same time.\n");
			ladmin_log("Incomplete parameters to modify a limit time ('timeadd' command)." RETCODE);
		}
		return;
	}
	if (verify_accountname(name) == 0) {
		return;
	}

	// lowercase for modif
	for (i = 0; modif[i]; i++)
		modif[i] = tolower((unsigned char)modif[i]); // tolower needs unsigned char
	p_modif = modif;
	while (strlen(p_modif) > 0) {
		value = atoi(p_modif);
		if (value == 0) {
			p_modif++;
		} else {
			if (p_modif[0] == '-' || p_modif[0] == '+')
				p_modif++;
			while (strlen(p_modif) > 0 && p_modif[0] >= '0' && p_modif[0] <= '9') {
				p_modif++;
			}
			if (p_modif[0] == 's') {
				second = value;
				p_modif++;
			} else if (p_modif[0] == 'm' && p_modif[1] == 'n') {
				minute = value;
				p_modif += 2;
			} else if (p_modif[0] == 'h') {
				hour = value;
				p_modif++;
			} else if (p_modif[0] == 'd' || p_modif[0] == 'j') {
				day = value;
				p_modif += 2;
			} else if (p_modif[0] == 'm') {
				month = value;
				p_modif++;
			} else if (p_modif[0] == 'y' || p_modif[0] == 'a') {
				year = value;
				p_modif++;
			} else {
				p_modif++;
			}
		}
	}

	if (defaultlanguage == 'F') {
		printf(" année:   %d\n", year);
		printf(" mois:    %d\n", month);
		printf(" jour:    %d\n", day);
		printf(" heure:   %d\n", hour);
		printf(" minute:  %d\n", minute);
		printf(" seconde: %d\n", second);
	} else {
		printf(" year:   %d\n", year);
		printf(" month:  %d\n", month);
		printf(" day:    %d\n", day);
		printf(" hour:   %d\n", hour);
		printf(" minute: %d\n", minute);
		printf(" second: %d\n", second);
	}

	if (year == 0 && month == 0 && day == 0 && hour == 0 && minute == 0 && second == 0) {
		if (defaultlanguage == 'F') {
			printf("Vous devez entrer un ajustement avec cette commande, svp:\n");
			printf("  Valeur d'ajustement (-1, 1, +1, etc...)\n");
			printf("  Elément modifié:\n");
			printf("    a ou y: année\n");
			printf("    m:      mois\n");
			printf("    j ou d: jour\n");
			printf("    h:      heure\n");
			printf("    mn:     minute\n");
			printf("    s:      seconde\n");
			printf("  <exemple> timeadd nomtest +1m-2mn1s-6y\n");
			printf("            Cette exemple ajoute 1 mois et 1 seconde, et soustrait 2 minutes\n");
			printf("            et 6 ans dans le même temps.\n");
			ladmin_log("Aucun ajustement n'est pas un ajustement (commande 'timeadd')." RETCODE);
		} else {
			printf("Please give an adjustment with this command:\n");
			printf("  Adjustment value (-1, 1, +1, etc...)\n");
			printf("  Modified element:\n");
			printf("    a or y: year\n");
			printf("    m:      month\n");
			printf("    j or d: day\n");
			printf("    h:      hour\n");
			printf("    mn:     minute\n");
			printf("    s:      second\n");
			printf("  <example> timeadd testname +1m-2mn1s-6y\n");
			printf("            this example adds 1 month and 1 second, and substracts 2 minutes\n");
			printf("            and 6 years at the same time.\n");
			ladmin_log("No adjustment isn't an adjustment ('timeadd' command)." RETCODE);
		}
		return;
	}
	if (year > 127 || year < -127) {
		if (defaultlanguage == 'F') {
			printf("Entrez un ajustement d'années correct (de -127 à 127), svp.\n");
			ladmin_log("Ajustement de l'année hors norme ('timeadd' command)." RETCODE);
		} else {
			printf("Please give a correct adjustment for the years (from -127 to 127).\n");
			ladmin_log("Abnormal adjustement for the year ('timeadd' command)." RETCODE);
		}
		return;
	}
	if (month > 255 || month < -255) {
		if (defaultlanguage == 'F') {
			printf("Entrez un ajustement de mois correct (de -255 à 255), svp.\n");
			ladmin_log("Ajustement du mois hors norme ('timeadd' command)." RETCODE);
		} else {
			printf("Please give a correct adjustment for the months (from -255 to 255).\n");
			ladmin_log("Abnormal adjustement for the month ('timeadd' command)." RETCODE);
		}
		return;
	}
	if (day > 32767 || day < -32767) {
		if (defaultlanguage == 'F') {
			printf("Entrez un ajustement de jours correct (de -32767 à 32767), svp.\n");
			ladmin_log("Ajustement des jours hors norme ('timeadd' command)." RETCODE);
		} else {
			printf("Please give a correct adjustment for the days (from -32767 to 32767).\n");
			ladmin_log("Abnormal adjustement for the days ('timeadd' command)." RETCODE);
		}
		return;
	}
	if (hour > 32767 || hour < -32767) {
		if (defaultlanguage == 'F') {
			printf("Entrez un ajustement d'heures correct (de -32767 à 32767), svp.\n");
			ladmin_log("Ajustement des heures hors norme ('timeadd' command)." RETCODE);
		} else {
			printf("Please give a correct adjustment for the hours (from -32767 to 32767).\n");
			ladmin_log("Abnormal adjustement for the hours ('timeadd' command)." RETCODE);
		}
		return;
	}
	if (minute > 32767 || minute < -32767) {
		if (defaultlanguage == 'F') {
			printf("Entrez un ajustement de minutes correct (de -32767 à 32767), svp.\n");
			ladmin_log("Ajustement des minutes hors norme ('timeadd' command)." RETCODE);
		} else {
			printf("Please give a correct adjustment for the minutes (from -32767 to 32767).\n");
			ladmin_log("Abnormal adjustement for the minutes ('timeadd' command)." RETCODE);
		}
		return;
	}
	if (second > 32767 || second < -32767) {
		if (defaultlanguage == 'F') {
			printf("Entrez un ajustement de secondes correct (de -32767 à 32767), svp.\n");
			ladmin_log("Ajustement des secondes hors norme ('timeadd' command)." RETCODE);
		} else {
			printf("Please give a correct adjustment for the seconds (from -32767 to 32767).\n");
			ladmin_log("Abnormal adjustement for the seconds ('timeadd' command)." RETCODE);
		}
		return;
	}

	if (defaultlanguage == 'F') {
		ladmin_log("Envoi d'un requête au serveur de logins pour modifier une date limite d'utilisation." RETCODE);
	} else {
		ladmin_log("Request to login-server to modify a time limit." RETCODE);
	}

	WPACKETW( 0) = 0x7950;
	strncpy(WPACKETP(2), name, 24);
	WPACKETW(26) = (short)year;
	WPACKETW(28) = (short)month;
	WPACKETW(30) = (short)day;
	WPACKETW(32) = (short)hour;
	WPACKETW(34) = (short)minute;
	WPACKETW(36) = (short)second;
	SENDPACKET(login_fd, 38);
	bytes_to_read = 1;

	return;
}

//-------------------------------------------------
// Sub-function: Set a validity limit of an account
//-------------------------------------------------
static inline void timesetaccount(char* param) {
	char name[1023], date[1023], time_txt[1023];
	int year, month, day, hour, minute, second;
	time_t connect_until_time; // # of seconds 1/1/1970 (timestamp): Validity limit of the account (0 = unlimited)
	struct tm *tmtime;

	memset(name, 0, sizeof(name));
	memset(date, 0, sizeof(date));
	memset(time_txt, 0, sizeof(time_txt));
	year = month = day = hour = minute = second = 0;
	connect_until_time = 0;
	tmtime = localtime(&connect_until_time); // initialize

	if (sscanf(param, "\"%[^\"]\" %s %[^\r\n]", name, date, time_txt) < 2 && // if date = 0, time can be void
	    sscanf(param, "'%[^']' %s %[^\r\n]", name, date, time_txt) < 2 && // if date = 0, time can be void
	    sscanf(param, "%s %s %[^\r\n]", name, date, time_txt) < 2) { // if date = 0, time can be void
		if (defaultlanguage == 'F') {
			printf("Entrez un nom de compte, une date et une heure svp.\n");
			printf("<exemple>: timeset <nom_du_compte> aaaa/mm/jj [hh:mm:ss]\n");
			printf("           timeset <nom_du_compte> 0    (0 = illimité)\n");
			printf("           Heure par défaut [hh:mm:ss]: 23:59:59.\n");
			ladmin_log("Nombre incorrect de paramètres pour fixer une date limite d'utilisation (commande 'timeset')." RETCODE);
		} else {
			printf("Please input an account name, a date and a hour.\n");
			printf("<example>: timeset <account_name> yyyy/mm/dd [hh:mm:ss]\n");
			printf("           timeset <account_name> 0   (0 = unlimited)\n");
			printf("           Default time [hh:mm:ss]: 23:59:59.\n");
			ladmin_log("Incomplete parameters to set a limit time ('timeset' command)." RETCODE);
		}
		return;
	}
	if (verify_accountname(name) == 0) {
		return;
	}

	if (time_txt[0] == '\0')
		strcpy(time_txt, "23:59:59");

	if (atoi(date) != 0 &&
	    ((sscanf(date, "%d/%d/%d", &year, &month, &day) < 3 &&
	      sscanf(date, "%d-%d-%d", &year, &month, &day) < 3 &&
	      sscanf(date, "%d.%d.%d", &year, &month, &day) < 3 &&
	      sscanf(date, "%d'%d'%d", &year, &month, &day) < 3) ||
	     sscanf(time_txt, "%d:%d:%d", &hour, &minute, &second) < 3)) {
		if (defaultlanguage == 'F') {
			printf("Entrez 0 ou une date et une heure svp (format: 0 ou aaaa/mm/jj hh:mm:ss).\n");
			ladmin_log("Format incorrect pour la date/heure ('timeset' command)." RETCODE);
		} else {
			printf("Please input 0 or a date and a time (format: 0 or yyyy/mm/dd hh:mm:ss).\n");
			ladmin_log("Invalid format for the date/time ('timeset' command)." RETCODE);
		}
		return;
	}

	if (atoi(date) == 0) {
		connect_until_time = 0;
	} else {
		if (year < 70) {
			year = year + 100;
		}
		if (year >= 1900) {
			year = year - 1900;
		}
		if (month < 1 || month > 12) {
			if (defaultlanguage == 'F') {
				printf("Entrez un mois correct svp (entre 1 et 12).\n");
				ladmin_log("Mois incorrect pour la date ('timeset' command)." RETCODE);
			} else {
				printf("Please give a correct value for the month (from 1 to 12).\n");
				ladmin_log("Invalid month for the date ('timeset' command)." RETCODE);
			}
			return;
		}
		month = month - 1;
		if (day < 1 || day > 31) {
			if (defaultlanguage == 'F') {
				printf("Entrez un jour correct svp (entre 1 et 31).\n");
				ladmin_log("Jour incorrect pour la date ('timeset' command)." RETCODE);
			} else {
				printf("Please give a correct value for the day (from 1 to 31).\n");
				ladmin_log("Invalid day for the date ('timeset' command)." RETCODE);
			}
			return;
		}
		if (((month == 3 || month == 5 || month == 8 || month == 10) && day > 30) ||
		    (month == 1 && day > 29)) {
			if (defaultlanguage == 'F') {
				printf("Entrez un jour correct en fonction du mois (%d) svp.\n", month);
				ladmin_log("Jour incorrect pour ce mois correspondant ('timeset' command)." RETCODE);
			} else {
				printf("Please give a correct value for a day of this month (%d).\n", month);
				ladmin_log("Invalid day for this month ('timeset' command)." RETCODE);
			}
			return;
		}
		if (hour < 0 || hour > 23) {
			if (defaultlanguage == 'F') {
				printf("Entrez une heure correcte svp (entre 0 et 23).\n");
				ladmin_log("Heure incorrecte pour l'heure ('timeset' command)." RETCODE);
			} else {
				printf("Please give a correct value for the hour (from 0 to 23).\n");
				ladmin_log("Invalid hour for the time ('timeset' command)." RETCODE);
			}
			return;
		}
		if (minute < 0 || minute > 59) {
			if (defaultlanguage == 'F') {
				printf("Entrez des minutes correctes svp (entre 0 et 59).\n");
				ladmin_log("Minute incorrecte pour l'heure ('timeset' command)." RETCODE);
			} else {
				printf("Please give a correct value for the minutes (from 0 to 59).\n");
				ladmin_log("Invalid minute for the time ('timeset' command)." RETCODE);
			}
			return;
		}
		if (second < 0 || second > 59) {
			if (defaultlanguage == 'F') {
				printf("Entrez des secondes correctes svp (entre 0 et 59).\n");
				ladmin_log("Seconde incorrecte pour l'heure ('timeset' command)." RETCODE);
			} else {
				printf("Please give a correct value for the seconds (from 0 to 59).\n");
				ladmin_log("Invalid second for the time ('timeset' command)." RETCODE);
			}
			return;
		}
		tmtime->tm_year = year;
		tmtime->tm_mon = month;
		tmtime->tm_mday = day;
		tmtime->tm_hour = hour;
		tmtime->tm_min = minute;
		tmtime->tm_sec = second;
		tmtime->tm_isdst = -1; // -1: no winter/summer time modification
		connect_until_time = mktime(tmtime);
		if (connect_until_time == -1) {
			if (defaultlanguage == 'F') {
				printf("Date incorrecte.\n");
				printf("Ajoutez 0 ou une date et une heure svp (format: 0 ou aaaa/mm/jj hh:mm:ss).\n");
				ladmin_log("Date incorrecte. ('timeset' command)." RETCODE);
			} else {
				printf("Invalid date.\n");
				printf("Please add 0 or a date and a time (format: 0 or yyyy/mm/dd hh:mm:ss).\n");
				ladmin_log("Invalid date. ('timeset' command)." RETCODE);
			}
			return;
		}
	}

	if (defaultlanguage == 'F') {
		ladmin_log("Envoi d'un requête au serveur de logins pour fixer une date limite d'utilisation." RETCODE);
	} else {
		ladmin_log("Request to login-server to set a time limit." RETCODE);
	}

	WPACKETW( 0) = 0x7948;
	strncpy(WPACKETP(2), name, 24);
	WPACKETL(26) = (int)connect_until_time;
	SENDPACKET(login_fd,30);
	bytes_to_read = 1;

	return;
}

//------------------------------------------------------------------------------
// Sub-function: Asking to displaying information about an account (by its name)
//------------------------------------------------------------------------------
static inline void whoaccount(char* param) {
	char name[1023];

	memset(name, 0, sizeof(name));

	if (param[0] == '\0' ||
	    (sscanf(param, "\"%[^\"]\"", name) < 1 &&
	     sscanf(param, "'%[^']'", name) < 1 &&
	     sscanf(param, "%[^\r\n]", name) < 1) ||
	     name[0] == '\0') {
		if (defaultlanguage == 'F') {
			printf("Entrez un nom de compte svp.\n");
			printf("<exemple> who nomtest\n");
			ladmin_log("Aucun nom n'a été donné pour trouver le compte." RETCODE);
		} else {
			printf("Please input an account name.\n");
			printf("<example> who testname\n");
			ladmin_log("No name was given to found the account." RETCODE);
		}
		return;
	}
	if (verify_accountname(name) == 0) {
		return;
	}

	if (defaultlanguage == 'F') {
		ladmin_log("Envoi d'un requête au serveur de logins pour obtenir le information d'un compte (par le nom)." RETCODE);
	} else {
		ladmin_log("Request to login-server to obtain information about an account (by its name)." RETCODE);
	}

	WPACKETW(0) = 0x7952;
	strncpy(WPACKETP(2), name, 24);
	SENDPACKET(login_fd, 26);
	bytes_to_read = 1;

	return;
}

//--------------------------------------------------------
// Sub-function: Asking of the version of the login-server
//--------------------------------------------------------
static void checkloginversion() { // not inline, called too often
	if (defaultlanguage == 'F')
		ladmin_log("Envoi d'un requête au serveur de logins pour obtenir sa version." RETCODE);
	else
		ladmin_log("Request to login-server to obtain its version." RETCODE);

	WPACKETW(0) = 0x7535;
	SENDPACKET(login_fd, 2);
	bytes_to_read = 1;

	return;
}

//-------------------------------------------------------
// Sub-function: Asking of the uptime of the login-server
//-------------------------------------------------------
static void checkloginuptime() { // not inline, called too often
	if (defaultlanguage == 'F')
		ladmin_log("Envoi d'un requête au serveur de logins pour obtenir son uptime." RETCODE);
	else
		ladmin_log("Request to login-server to obtain its uptime." RETCODE);

	WPACKETW(0) = 0x7533;
	SENDPACKET(login_fd, 2);
	bytes_to_read = 1;

	return;
}

//---------------------------------------------
// Prompt function
// this function wait until user type a command
// and analyse the command.
//---------------------------------------------
static inline void prompt() {
	int i, j;
	char buf[1024];
	char *p;

	// while we don't wait new packets
	while (bytes_to_read == 0) {
		printf("\n");
		if (defaultlanguage == 'F')
			printf(CL_DARK_GREEN "Pour afficher les commandes, tapez 'Entrée'." CL_RESET "\n");
		else
			printf(CL_DARK_GREEN "To list the commands, type 'enter'." CL_RESET "\n");
		printf(CL_DARK_CYAN "Ladmin-> " CL_RESET CL_BOLD);
		fflush(stdout);

		// get command and parameter
		memset(buf, 0, sizeof(buf));
		fflush(stdin);
		fgets(buf, sizeof(buf), stdin); // fgets reads until maximum one less than size and add '\0' -> so, it's not necessary to add -1

#ifndef __WIN32
		printf(CL_RESET);
#endif
		fflush(stdout);

		// remove final \n
		if((p = strrchr(buf, '\n')) != NULL)
			p[0] = '\0';
		// remove all control char
		for (i = 0; buf[i]; i++)
			if (iscntrl((int)buf[i])) { // if (buf[i] < 32) {
				// remove cursor control.
				if (buf[i] == 27 && buf[i+1] == '[' &&
				    (buf[i+2] == 'H' || // home position (cursor)
				     buf[i+2] == 'J' || // clear screen
				     buf[i+2] == 'A' || // up 1 line
				     buf[i+2] == 'B' || // down 1 line
				     buf[i+2] == 'C' || // right 1 position
				     buf[i+2] == 'D' || // left 1 position
				     buf[i+2] == 'G')) { // center cursor (windows)
					for (j = i; buf[j]; j++)
						buf[j] = buf[j+3];
				} else if (buf[i] == 27 && buf[i+1] == '[' && buf[i+2] == '2' && buf[i+3] == 'J') { // clear screen
					for (j = i; buf[j]; j++)
						buf[j] = buf[j+4];
				} else if (buf[i] == 27 && buf[i+1] == '[' && buf[i+3] == '~' &&
				           (buf[i+2] == '1' || // home (windows)
				            buf[i+2] == '2' || // insert (windows)
				            buf[i+2] == '3' || // del (windows)
				            buf[i+2] == '4' || // end (windows)
				            buf[i+2] == '5' || // pgup (windows)
				            buf[i+2] == '6')) { // pgdown (windows)
					for (j = i; buf[j]; j++)
						buf[j] = buf[j+4];
				} else if (buf[i] == 8 && i > 0) { // backspace (ctrl+H) (if no previous char, next 'else' will remove backspace)
					for (j = i - 1; buf[j]; j++) // i - 1, to remove previous character
						buf[j] = buf[j+2];
						i--; // recheck previous value
				} else {
					// remove other control char.
					for (j = i; buf[j]; j++)
						buf[j] = buf[j+1];
				}
				i--;
			}

		// extract command name and parameters
		memset(command, 0, sizeof(command));
		memset(parameters, 0, sizeof(parameters));
		sscanf(buf, "%1023s %[^\n]", command, parameters);
		command[1023] = '\0';
		parameters[1023] = '\0';

		// lowercase for command line
		for (i = 0; command[i]; i++)
			command[i] = tolower((unsigned char)command[i]); // tolower needs unsigned char

		if (command[0] == '?' || command[0] == '\0') {
			if (defaultlanguage == 'F') {
				strcpy(buf, "aide");
				strcpy(command, "aide");
			} else {
				strcpy(buf, "help");
				strcpy(command, "help");
			}
		}

		// Analyse of the command
		check_command(command); // give complete name to the command

		if (parameters[0] == '\0') {
			if (defaultlanguage == 'F') {
				ladmin_log("Commande: '%s' (sans paramètre)" RETCODE, command, parameters);
			} else {
				ladmin_log("Command: '%s' (without parameters)" RETCODE, command, parameters);
			}
		} else {
			if (defaultlanguage == 'F') {
				ladmin_log("Commande: '%s', paramètres: '%s'" RETCODE, command, parameters);
			} else {
				ladmin_log("Command: '%s', parameters: '%s'" RETCODE, command, parameters);
			}
		}

		// Analyse of the command
// help
		if (strcmp(command, "aide") == 0) {
			display_help(parameters, 1); // 1: french
		} else if (strcmp(command, "help") == 0) {
			display_help(parameters, 0); // 0: english
// general commands
		} else if (strcmp(command, "add") == 0) {
			addaccount(parameters, 0); // 0: no email
		} else if (strcmp(command, "ban") == 0) {
			banaccount(parameters);
		} else if (strcmp(command, "banadd") == 0) {
			banaddaccount(parameters);
		} else if (strcmp(command, "banset") == 0) {
			bansetaccount(parameters);
		} else if (strcmp(command, "block") == 0) {
			blockaccount(parameters);
		} else if (strcmp(command, "check") == 0) {
			checkaccount(parameters);
		} else if (strcmp(command, "create") == 0) {
			addaccount(parameters, 1); // 1: with email
		} else if (strcmp(command, "delete") == 0) {
			delaccount(parameters);
		} else if (strcmp(command, "email") == 0) {
			changeemail(parameters);
		} else if (strcmp(command, "getcount") == 0) {
			getlogincount();
		} else if (strcmp(command, "gm") == 0) {
			changegmlevel(parameters);
		} else if (strcmp(command, "id") == 0) {
			idaccount(parameters);
		} else if (strcmp(command, "info") == 0) {
			infoaccount(parameters);
		} else if (strcmp(command, "kami") == 0) {
			sendbroadcast(0, parameters); // flag for normal
		} else if (strcmp(command, "kamib") == 0) {
			sendbroadcast(0x10, parameters); // flag for blue
		} else if (strcmp(command, "language") == 0) {
			changelanguage(parameters);
		} else if (strcmp(command, "list") == 0) {
			listaccount(parameters, 0); // 0: to list all
		} else if (strcmp(command, "listban") == 0) {
			listaccount(parameters, 3); // 3: to list only accounts with state or bannished
		} else if (strcmp(command, "listgm") == 0) {
			listaccount(parameters, 1); // 1: to list only GM
		} else if (strcmp(command, "listok") == 0) {
			listaccount(parameters, 4); // 4: to list only accounts without state and not bannished
		} else if (strcmp(command, "listonline") == 0) {
			listaccount(parameters, 5); // 5: to list only online accounts
		} else if (strcmp(command, "memo") == 0) {
			changememo(parameters);
		} else if (strcmp(command, "memoadd") == 0) {
			addmemo(parameters);
		} else if (strcmp(command, "name") == 0) {
			nameaccount(atoi(parameters));
		} else if (strcmp(command, "password") == 0) {
			changepasswd(parameters);
		} else if (strcmp(command, "search") == 0) { // no regex in C version
			listaccount(parameters, 2); // 2: to list with pattern
		} else if (strcmp(command, "sex") == 0) {
			changesex(parameters);
		} else if (strcmp(command, "state") == 0) {
			changestate(parameters);
		} else if (strcmp(command, "timeadd") == 0) {
			timeaddaccount(parameters);
		} else if (strcmp(command, "timeset") == 0) {
			timesetaccount(parameters);
		} else if (strcmp(command, "unban") == 0) {
			unbanaccount(parameters);
		} else if (strcmp(command, "unblock") == 0) {
			unblockaccount(parameters);
		} else if (strcmp(command, "uptime") == 0) {
			checkloginuptime();
		} else if (strcmp(command, "version") == 0) {
			checkloginversion();
		} else if (strcmp(command, "who") == 0) {
			whoaccount(parameters);
// quit
		} else if (strcmp(command, "quit") == 0 ||
		           strcmp(command, "exit") == 0 ||
		           strcmp(command, "end") == 0) {
			if (defaultlanguage == 'F') {
				printf("Au revoir.\n");
			} else {
				printf("Bye.\n");
			}
			exit(0);
// unknown command
		} else {
			if (defaultlanguage == 'F') {
				printf("Commande inconnue [%s].\n", buf);
				ladmin_log("Commande inconnue [%s]." RETCODE, buf);
			} else {
				printf("Unknown command [%s].\n", buf);
				ladmin_log("Unknown command [%s]." RETCODE, buf);
			}
		}
	}

	return;
}

//-------------------------------------------------------------
// Function: Parse receiving informations from the login-server
//-------------------------------------------------------------
int parse_fromlogin(int fd) { // global definition (called by socket.c)
	char account_name[25];

	if (session[fd]->eof) {
		if (defaultlanguage == 'F') {
			printf("Impossible de se connecter au serveur de login [%s:%d] !\n", loginserverip, loginserverport);
			ladmin_log("Impossible de se connecter au serveur de login [%s:%d] !" RETCODE, loginserverip, loginserverport);
		} else {
			printf("Impossible to have a connection with the login-server [%s:%d] !\n", loginserverip, loginserverport);
			ladmin_log("Impossible to have a connection with the login-server [%s:%d] !" RETCODE, loginserverip, loginserverport);
		}
#ifdef __WIN32
		shutdown(fd, SD_BOTH);
		closesocket(fd);
#else
		close(fd);
#endif
		delete_session(fd);
		exit (0);
	}

//	printf("parse_fromlogin : %d %d %d\n", fd, RFIFOREST(fd), RFIFOW(fd,0));

	while(RFIFOREST(fd) >= 2) {
		switch(RFIFOW(fd,0)) {
		case 0x7919:	// answer of a connection request
			if (RFIFOREST(fd) < 3)
				return 0;
			if (RFIFOB(fd,2) != 0) {
				if (defaultlanguage == 'F') {
					printf("Erreur de login:\n");
					printf(" - mot de passe incorrect,\n");
					printf(" - système d'administration non activé, ou\n");
					printf(" - IP non autorisée.\n");
					ladmin_log("Erreur de login: mot de passe incorrect, système d'administration non activé, ou IP non autorisée." RETCODE);
				} else {
					printf("Error at login:\n");
					printf(" - incorrect password,\n");
					printf(" - administration system not activated, or\n");
					printf(" - unauthorized IP.\n");
					ladmin_log("Error at login: incorrect password, administration system not activated, or unauthorized IP." RETCODE);
				}
				session[fd]->eof = 1;
				//bytes_to_read = 1; // not stop at prompt
			} else {
				if (defaultlanguage == 'F') {
					printf("Connexion établie.\n");
					ladmin_log("Connexion établie." RETCODE);
					printf("Lecture de la version du serveur de login...\n");
					ladmin_log("Lecture de la version du serveur de login..." RETCODE);
				} else {
					printf("Connection established.\n");
					ladmin_log("Connection established." RETCODE);
					printf("Fetching the login-server version...\n");
					ladmin_log("Fetching the login-server version..." RETCODE);
				}
				//bytes_to_read = 1; // unchanged
				checkloginversion();
				checkloginuptime();
			}
			RFIFOSKIP(fd,3);
			break;

#ifdef PASSWORDENC
		case 0x01dc:	// answer of a coding key request
			if (RFIFOREST(fd) < 4 || RFIFOREST(fd) < RFIFOW(fd,2))
				return 0;
		  {
			char md5str[64] = "";
//			unsigned char md5bin[32];
			if (RFIFOW(fd,2) == 4) {
				memset(md5str, 0, sizeof(md5str));
				strncpy(md5str, loginserveradminpassword, sizeof(md5str) - 1);
			} else {
				if (passenc == 1) {
					memset(md5str, 0, sizeof(md5str));
					memcpy(md5str, RFIFOP(fd,4), RFIFOW(fd,2) - 4);
					strncpy(md5str + (RFIFOW(fd,2) - 4), loginserveradminpassword, strlen(loginserveradminpassword));
				} else if (passenc == 2) {
					strncpy(md5str, loginserveradminpassword, strlen(loginserveradminpassword));
					memcpy(md5str + strlen(loginserveradminpassword), RFIFOP(fd,4), RFIFOW(fd,2) - 4);
				}
			}
//			MD5_String2binary(md5str, md5bin);
			WPACKETW(0) = 0x7918; // Request for administation login (encrypted password)
			WPACKETW(2) = passenc; // Encrypted type
//			memcpy(WPACKETP(4), md5bin, 16);
			MD5_String2binary(md5str, (unsigned char*)WPACKETP(4));
			SENDPACKET(login_fd, 20);
			if (defaultlanguage == 'F') {
				printf("Réception de la clef MD5.\n");
				ladmin_log("Réception de la clef MD5." RETCODE);
				printf("Envoi du mot de passe crypté...\n");
				ladmin_log("Envoi du mot de passe crypté..." RETCODE);
			} else {
				printf("Got a MD5 challenge.\n");
				ladmin_log("Got a MD5 challenge." RETCODE);
				printf("Sending the encrypted password...\n");
				ladmin_log("Sending the encrypted password..." RETCODE);
			}
		  }
			bytes_to_read = 1;
			RFIFOSKIP(fd,RFIFOW(fd,2));
			break;
#endif

		case 0x7531:	// Displaying of the version of the login-server
			if (RFIFOREST(fd) < 10)
				return 0;
			printf("  Login-Server [%s:%d]\n", loginserverip, loginserverport);
			if (((int)RFIFOB(login_fd,5)) == 0) {
				printf("  Freya version stable-%d.%d", (int)RFIFOB(login_fd,2), (int)RFIFOB(login_fd,3));
			} else {
				printf("  Freya version dev-%d.%d", (int)RFIFOB(login_fd,2), (int)RFIFOB(login_fd,3));
			}
			if (((int)RFIFOB(login_fd,4)) == 0)
				printf(" revision %d", (int)RFIFOB(login_fd,4));
			if (((int)RFIFOB(login_fd,6)) == 0)
				printf("%d.\n", RFIFOW(login_fd,8));
			else if (((int)RFIFOB(login_fd,6)) == 2)
				printf("-Freya mod%d.\n", RFIFOW(login_fd,8));
			else
				printf("-mod%d.\n", RFIFOW(login_fd,8));
			if (init_flag == 1) // flag to know if init is terminated (0: no, 1: yes)
				bytes_to_read = 0; // if in initialisation, wait for uptime too
			RFIFOSKIP(fd,10);
			break;

		case 0x7534:	// Displaying of the uptime of the login-server
			if (RFIFOREST(fd) < 8)
				return 0;
			if (defaultlanguage == 'F') {
				printf("  Uptime du login-server: %d secondes.\n", RFIFOL(fd,2));
				printf("  Nombre de joueur%s en ligne: %d.\n", (RFIFOW(fd,6) > 1) ? "s" : "", RFIFOW(fd,6));
			} else {
				printf("  Uptime of the login-server: %d seconds.\n", RFIFOL(fd,2));
				printf("  Number of online player%s: %d.\n", (RFIFOW(fd,6) > 1) ? "s" : "", RFIFOW(fd,6));
			}
			bytes_to_read = 0;
			RFIFOSKIP(fd,8);
			break;

		case 0x7536:	// Displaying of the version of the login-server
			if (RFIFOREST(fd) < 13)
				return 0;
			printf("  Login-Server [%s:%d]\n", loginserverip, loginserverport);
			if (((int)RFIFOB(login_fd,5)) == 0) {
				printf("  Freya version stable-%d.%d", (int)RFIFOB(login_fd,2), (int)RFIFOB(login_fd,3));
			} else {
				printf("  Freya version dev-%d.%d", (int)RFIFOB(login_fd,2), (int)RFIFOB(login_fd,3));
			}
			if (((int)RFIFOB(login_fd,4)) == 0)
				printf(" revision %d", (int)RFIFOB(login_fd,4));
			if (((int)RFIFOB(login_fd,6)) == 0)
				printf("%d", RFIFOW(login_fd,8));
			else if (((int)RFIFOB(login_fd,6)) == 2)
				printf("-Freya mod%d", RFIFOW(login_fd,8));
			else
				printf("-mod%d", RFIFOW(login_fd,8));
			if (RFIFOW(login_fd,10) > 0)
				printf(" SVN rev. %u", RFIFOW(login_fd,10));
			if (RFIFOB(login_fd,12) == 0)
				printf(" (TXT version).\n");
			else
				printf(" (SQL version).\n");
			if (init_flag == 1) // flag to know if init is terminated (0: no, 1: yes)
				bytes_to_read = 0; // if in initialisation, wait for uptime too
			RFIFOSKIP(fd,13);
			break;

		case 0x7921:	// Displaying of the list of accounts
			if (RFIFOREST(fd) < 4 || RFIFOREST(fd) < RFIFOW(fd,2))
				return 0;
			if (RFIFOW(fd,2) < 6) {
				if (defaultlanguage == 'F') {
					ladmin_log("  Réception d'une liste des comptes vide." RETCODE);
					if (list_count == 0)
						printf("Aucun compte trouvé.\n");
					else if (list_count == 1)
						printf("1 compte trouvé.\n");
					else
						printf("%d comptes trouvés.\n", list_count);
				} else {
					ladmin_log("  Receiving of a void accounts list." RETCODE);
					if (list_count == 0)
						printf("No account found.\n");
					else if (list_count == 1)
						printf("1 account found.\n");
					else
						printf("%d accounts found.\n", list_count);
				}
				bytes_to_read = 0;
			} else {
				int i;
				if (defaultlanguage == 'F')
					ladmin_log("  Réception d'une liste des comptes." RETCODE);
				else
					ladmin_log("  Receiving of a accounts list." RETCODE);
				for(i = 5; i < RFIFOW(fd,2); i += 37) {
					int j;
					char userid[25];
					char lower_userid[25];
					memset(userid, 0, sizeof(userid));
					strncpy(userid, RFIFOP(fd,i + 5), 24);
					memset(lower_userid, 0, sizeof(lower_userid));
					for (j = 0; userid[j]; j++)
						lower_userid[j] = tolower((unsigned char)userid[j]); // tolower needs unsigned char
					list_first = RFIFOL(fd,i) + 1;
					// here are checks...
					if (list_type == 0 ||
					    (list_type == 1 && RFIFOB(fd,i + 4) > 0) ||
					    (list_type == 2 && strstr(lower_userid, parameters) != NULL) ||
					    (list_type == 3 && RFIFOW(fd,i + 34) != 0) ||
					    (list_type == 4 && RFIFOW(fd,i + 34) == 0) ||
					    (list_type == 5 && RFIFOW(fd,i + 36) != 0)) {
						printf("%s ", RFIFOB(fd,i + 36) ? "*" : " ");
						printf("%10d ", RFIFOL(fd,i));
						if (RFIFOB(fd,i+4) == 0)
							printf("   ");
						else
							printf("%2d ", (int)RFIFOB(fd,i+4));
						printf("%-24s", userid);
						if (RFIFOB(fd,i + 29) == 0)
							printf("%-3s ", "Fem");
						else if (RFIFOB(fd,i + 29) == 1)
							printf("%-3s ", "Mal");
						else
							printf("%-3s ", "Svr");
						printf("%6d ", RFIFOL(fd,i + 30));
						switch(RFIFOW(fd,i + 34)) {
						case 0:
							if (defaultlanguage == 'F')
								printf("%-27s\n", "Compte Ok");
							else
								printf("%-27s\n", "Account OK");
							break;
						case 1:
							printf("%-27s\n", "Unregistered ID");
							break;
						case 2:
							printf("%-27s\n", "Incorrect Password");
							break;
						case 3:
							printf("%-27s\n", "This ID is expired");
							break;
						case 4:
							printf("%-27s\n", "Rejected from Server");
							break;
						case 5:
							printf("%-27s\n", "Blocked by the GM Team"); // You have been blocked by the GM Team
							break;
						case 6:
							printf("%-27s\n", "Your EXE file is too old"); // Your Game's EXE file is not the latest version
							break;
						case 7:
							printf("%-27s\n", "Banishement or");
							printf("                                                   Prohibited to login until...\n"); // You are Prohibited to log in until %s
							break;
						case 8:
							printf("%-27s\n", "Server is over populated");
							break;
						case 9:
							printf("%-27s\n", "No MSG");
							break;
						default: // 100
							printf("%-27s\n", "This ID is totally erased"); // This ID has been totally erased
							break;
						}
						list_count++;
					}
				}
				if (list_first < list_last) {
					// asking of the following acounts
					if (defaultlanguage == 'F')
						ladmin_log("Envoi d'un requête au serveur de logins pour obtenir la liste des comptes de %d à %d (complément)." RETCODE, list_first, list_last);
					else
						ladmin_log("Request to login-server to obtain the list of accounts from %d to %d (complement)." RETCODE, list_first, list_last);
					WPACKETW( 0) = 0x7920;
					WPACKETL( 2) = list_first;
					WPACKETL( 6) = list_last;
					WPACKETB(10) = (list_type == 2) ? 0 : list_type; /* 0: any accounts, 1: gm only, 3: accounts with state or bannished, 4: accounts without state and not bannished */
					SENDPACKET(login_fd, 11);
					bytes_to_read = 1;
				} else
					bytes_to_read = 0;
			}
			RFIFOSKIP(fd,RFIFOW(fd,2));
			break;

		case 0x7931:	// Answer of login-server about an account creation
			if (RFIFOREST(fd) < 30)
				return 0;
			memset(account_name, 0, sizeof(account_name));
			strncpy(account_name, RFIFOP(fd,6), 24);
			if (RFIFOL(fd,2) == -1) {
				if (defaultlanguage == 'F') {
					printf("Echec à la création du compte [%s]. Un compte identique existe déjà.\n", account_name);
					ladmin_log("Echec à la création du compte [%s]. Un compte identique existe déjà." RETCODE, account_name);
				} else {
					printf("Account [%s] creation failed. Same account already exists.\n", account_name);
					ladmin_log("Account [%s] creation failed. Same account already exists." RETCODE, account_name);
				}
			} else {
				if (defaultlanguage == 'F') {
					printf("Compte [%s] créé avec succès [id: %d].\n", account_name, RFIFOL(fd,2));
					ladmin_log("Compte [%s] créé avec succès [id: %d]." RETCODE, account_name, RFIFOL(fd,2));
				} else {
					printf("Account [%s] is successfully created [id: %d].\n", account_name, RFIFOL(fd,2));
					ladmin_log("Account [%s] is successfully created [id: %d]." RETCODE, account_name, RFIFOL(fd,2));
				}
			}
			bytes_to_read = 0;
			RFIFOSKIP(fd,30);
			break;

		case 0x7933:	// Answer of login-server about an account deletion
			if (RFIFOREST(fd) < 30)
				return 0;
			memset(account_name, 0, sizeof(account_name));
			strncpy(account_name, RFIFOP(fd,6), 24);
			if (RFIFOL(fd,2) == -1) {
				if (defaultlanguage == 'F') {
					printf("Echec de la suppression du compte [%s]. Le compte n'existe pas.\n", account_name);
					ladmin_log("Echec de la suppression du compte [%s]. Le compte n'existe pas." RETCODE, account_name);
				} else {
					printf("Account [%s] deletion failed. Account doesn't exist.\n", account_name);
					ladmin_log("Account [%s] deletion failed. Account doesn't exist." RETCODE, account_name);
				}
			} else {
				if (defaultlanguage == 'F') {
					printf("Compte [%s][id: %d] SUPPRIME avec succès.\n", account_name, RFIFOL(fd,2));
					ladmin_log("Compte [%s][id: %d] SUPPRIME avec succès." RETCODE, account_name, RFIFOL(fd,2));
				} else {
					printf("Account [%s][id: %d] is successfully DELETED.\n", account_name, RFIFOL(fd,2));
					ladmin_log("Account [%s][id: %d] is successfully DELETED." RETCODE, account_name, RFIFOL(fd,2));
				}
			}
			bytes_to_read = 0;
			RFIFOSKIP(fd,30);
			break;

		case 0x7935:	// answer of the change of an account password
			if (RFIFOREST(fd) < 30)
				return 0;
			memset(account_name, 0, sizeof(account_name));
			strncpy(account_name, RFIFOP(fd,6), 24);
			if (RFIFOL(fd,2) == -1) {
				if (defaultlanguage == 'F') {
					printf("Echec de la modification du mot de passe du compte [%s].\n", account_name);
					printf("Le compte [%s] n'existe pas.\n", account_name);
					ladmin_log("Echec de la modification du mot de passe du compte. Le compte [%s] n'existe pas." RETCODE, account_name);
				} else {
					printf("Account [%s] password changing failed.\n", account_name);
					printf("Account [%s] doesn't exist.\n", account_name);
					ladmin_log("Account password changing failed. The compte [%s] doesn't exist." RETCODE, account_name);
				}
			} else {
				if (defaultlanguage == 'F') {
					printf("Modification du mot de passe du compte [%s][id: %d] réussie.\n", account_name, RFIFOL(fd,2));
					ladmin_log("Modification du mot de passe du compte [%s][id: %d] réussie." RETCODE, account_name, RFIFOL(fd,2));
				} else {
					printf("Account [%s][id: %d] password successfully changed.\n", account_name, RFIFOL(fd,2));
					ladmin_log("Account [%s][id: %d] password successfully changed." RETCODE, account_name, RFIFOL(fd,2));
				}
			}
			bytes_to_read = 0;
			RFIFOSKIP(fd,30);
			break;

		case 0x7937:	// answer of the change of an account state
			if (RFIFOREST(fd) < 34)
				return 0;
			memset(account_name, 0, sizeof(account_name));
			strncpy(account_name, RFIFOP(fd,6), 24);
			if (RFIFOL(fd,2) == -1) {
				if (defaultlanguage == 'F') {
					printf("Echec du changement du statut du compte [%s]. Le compte n'existe pas.\n", account_name);
					ladmin_log("Echec du changement du statut du compte [%s]. Le compte n'existe pas." RETCODE, account_name);
				} else {
					printf("Account [%s] state changing failed. Account doesn't exist.\n", account_name);
					ladmin_log("Account [%s] state changing failed. Account doesn't exist." RETCODE, account_name);
				}
			} else {
				char tmpstr[256];
				if (defaultlanguage == 'F') {
					sprintf(tmpstr, "Statut du compte [%s] changé avec succès en [", account_name);
				} else {
					sprintf(tmpstr, "Account [%s] state successfully changed in [", account_name);
				}
				switch(RFIFOL(fd,30)) {
				case 0:
					if (defaultlanguage == 'F')
						strcat(tmpstr, "0: Compte Ok");
					else
						strcat(tmpstr, "0: Account OK");
					break;
				case 1:
					strcat(tmpstr, "1: Unregistered ID");
					break;
				case 2:
					strcat(tmpstr, "2: Incorrect Password");
					break;
				case 3:
					strcat(tmpstr, "3: This ID is expired");
					break;
				case 4:
					strcat(tmpstr, "4: Rejected from Server");
					break;
				case 5:
					strcat(tmpstr, "5: You have been blocked by the GM Team");
					break;
				case 6:
					strcat(tmpstr, "6: [Your Game's EXE file is not the latest version");
					break;
				case 7:
					strcat(tmpstr, "7: You are Prohibited to log in until...");
					break;
				case 8:
					strcat(tmpstr, "8: Server is jammed due to over populated");
					break;
				case 9:
					strcat(tmpstr, "9: No MSG");
					break;
				default: // 100
					strcat(tmpstr, "100: This ID is totally erased");
					break;
				}
				strcat(tmpstr, "]");
				printf("%s\n", tmpstr);
				ladmin_log("%s%s", tmpstr, RETCODE);
			}
			bytes_to_read = 0;
			RFIFOSKIP(fd,34);
			break;

		case 0x7939:	// answer of the number of online players
			if (RFIFOREST(fd) < 4 || RFIFOREST(fd) < RFIFOW(fd,2))
				return 0;
		  {
			// Get length of the received packet
			int i;
			char name[21];
			if (defaultlanguage == 'F') {
				ladmin_log("  Réception du nombre de joueurs en ligne." RETCODE);
			} else {
				ladmin_log("  Receiving of the number of online players." RETCODE);
			}
			// Read information of the servers
			if (RFIFOW(fd,2) < 5) {
				if (defaultlanguage == 'F') {
					printf("  Aucun serveur n'est connecté au login serveur.\n");
				} else {
					printf("  No server is connected to the login-server.\n");
				}
			} else {
				if (defaultlanguage == 'F') {
					printf("  Nombre de joueurs en ligne (serveur: nb):\n");
				} else {
					printf("  Number of online players (server: number):\n");
				}
				// Displaying of result
				for(i = 4; i < RFIFOW(fd,2); i += 32) {
					memcpy(name, RFIFOP(fd,i+6), 20);
					name[sizeof(name) - 1] = '\0';
					printf("    %-20s : %5d\n", name, RFIFOW(fd,i+26));
				}
			}
		  }
			bytes_to_read = 0;
			RFIFOSKIP(fd,RFIFOW(fd,2));
			break;

		case 0x793b:	// answer of the check of a password
			if (RFIFOREST(fd) < 30)
				return 0;
			memset(account_name, 0, sizeof(account_name));
			strncpy(account_name, RFIFOP(fd,6), 24);
			if (RFIFOL(fd,2) == -1) {
				if (defaultlanguage == 'F') {
					printf("Le compte [%s] n'existe pas ou le mot de passe est incorrect.\n", account_name);
					ladmin_log("Le compte [%s] n'existe pas ou le mot de passe est incorrect." RETCODE, account_name);
				} else {
					printf("The account [%s] doesn't exist or the password is incorrect.\n", account_name);
					ladmin_log("The account [%s] doesn't exist or the password is incorrect." RETCODE, account_name);
				}
			} else {
				if (defaultlanguage == 'F') {
					printf("Le mot de passe donné correspond bien au compte [%s][id: %d].\n", account_name, RFIFOL(fd,2));
					ladmin_log("Le mot de passe donné correspond bien au compte [%s][id: %d]." RETCODE, account_name, RFIFOL(fd,2));
				} else {
					printf("The proposed password is correct for the account [%s][id: %d].\n", account_name, RFIFOL(fd,2));
					ladmin_log("The proposed password is correct for the account [%s][id: %d]." RETCODE, account_name, RFIFOL(fd,2));
				}
			}
			bytes_to_read = 0;
			RFIFOSKIP(fd,30);
			break;

		case 0x793d:	// answer of the change of an account sex
			if (RFIFOREST(fd) < 30)
				return 0;
			memset(account_name, 0, sizeof(account_name));
			strncpy(account_name, RFIFOP(fd,6), 24);
			if (RFIFOL(fd,2) == -1) {
				if (defaultlanguage == 'F') {
					printf("Echec de la modification du sexe du compte [%s].\n", account_name);
					printf("Le compte [%s] n'existe pas ou le sexe est déjà celui demandé.\n", account_name);
					ladmin_log("Echec de la modification du sexe du compte. Le compte [%s] n'existe pas ou le sexe est déjà celui demandé." RETCODE, account_name);
				} else {
					printf("Account [%s] sex changing failed.\n", account_name);
					printf("Account [%s] doesn't exist or the sex is already the good sex.\n", account_name);
					ladmin_log("Account sex changing failed. The compte [%s] doesn't exist or the sex is already the good sex." RETCODE, account_name);
				}
			} else {
				if (defaultlanguage == 'F') {
					printf("Sexe du compte [%s][id: %d] changé avec succès.\n", account_name, RFIFOL(fd,2));
					ladmin_log("Sexe du compte [%s][id: %d] changé avec succès." RETCODE, account_name, RFIFOL(fd,2));
				} else {
					printf("Account [%s][id: %d] sex successfully changed.\n", account_name, RFIFOL(fd,2));
					ladmin_log("Account [%s][id: %d] sex successfully changed." RETCODE, account_name, RFIFOL(fd,2));
				}
			}
			bytes_to_read = 0;
			RFIFOSKIP(fd,30);
			break;

		case 0x793f:	// answer of the change of an account GM level
			if (RFIFOREST(fd) < 32)
				return 0;
			memset(account_name, 0, sizeof(account_name));
			strncpy(account_name, RFIFOP(fd,6), 24);
			if (RFIFOL(fd,2) == -1) {
				if (defaultlanguage == 'F') {
					printf("Echec de la modification du niveau de GM du compte [%s].\n", account_name);
					printf("Le compte [%s] n'existe pas, le niveau de GM est déjà celui demandé\n", account_name);
					printf("ou il est impossible de modifier le fichier des comptes GM.\n");
					ladmin_log("Echec de la modification du niveau de GM du compte. Le compte [%s] n'existe pas, le niveau de GM est déjà celui demandé ou il est impossible de modifier le fichier des comptes GM." RETCODE, RFIFOP(fd,6));
				} else {
					printf("Account [%s] GM level changing failed.\n", account_name);
					printf("Account [%s] doesn't exist, the GM level is already the good GM level\n", account_name);
					printf("or it's impossible to modify the GM accounts file.\n");
					ladmin_log("Account GM level changing failed. The compte [%s] doesn't exist, the GM level is already the good sex or it's impossible to modify the GM accounts file." RETCODE, RFIFOP(fd,6));
				}
			} else {
				if (defaultlanguage == 'F') {
					printf("Niveau de GM du compte [%s][id: %d] changé avec succès.\n", account_name, RFIFOL(fd,2));
					printf("Niveau de GM: %d -> %d.\n", RFIFOB(fd,30), RFIFOB(fd,31));
					ladmin_log("Niveau de GM du compte [%s][id: %d] changé avec succès: %d -> %d." RETCODE, account_name, RFIFOL(fd,2), RFIFOB(fd,30), RFIFOB(fd,31));
				} else {
					printf("Account [%s][id: %d] GM level successfully changed.\n", account_name, RFIFOL(fd,2));
					printf("GM level: %d -> %d.\n", RFIFOB(fd,30), RFIFOB(fd,31));
					ladmin_log("Account [%s][id: %d] GM level successfully changed: %d -> %d." RETCODE, account_name, RFIFOL(fd,2), RFIFOB(fd,30), RFIFOB(fd,31));
				}
			}
			bytes_to_read = 0;
			RFIFOSKIP(fd,32);
			break;

		case 0x7941:	// answer of the change of an account email
			if (RFIFOREST(fd) < 30)
				return 0;
			memset(account_name, 0, sizeof(account_name));
			strncpy(account_name, RFIFOP(fd,6), 24);
			if (RFIFOL(fd,2) == -1) {
				if (defaultlanguage == 'F') {
					printf("Echec de la modification de l'e-mail du compte [%s].\n", account_name);
					printf("Le compte [%s] n'existe pas.\n", account_name);
					ladmin_log("Echec de la modification de l'e-mail du compte. Le compte [%s] n'existe pas." RETCODE, account_name);
				} else {
					printf("Account [%s] e-mail changing failed.\n", account_name);
					printf("Account [%s] doesn't exist.\n", account_name);
					ladmin_log("Account e-mail changing failed. The compte [%s] doesn't exist." RETCODE, account_name);
				}
			} else {
				if (defaultlanguage == 'F') {
					printf("Modification de l'e-mail du compte [%s][id: %d] réussie.\n", account_name, RFIFOL(fd,2));
					ladmin_log("Modification de l'e-mail du compte [%s][id: %d] réussie." RETCODE, account_name, RFIFOL(fd,2));
				} else {
					printf("Account [%s][id: %d] e-mail successfully changed.\n", account_name, RFIFOL(fd,2));
					ladmin_log("Account [%s][id: %d] e-mail successfully changed." RETCODE, account_name, RFIFOL(fd,2));
				}
			}
			bytes_to_read = 0;
			RFIFOSKIP(fd,30);
			break;

		case 0x7943:	// answer of the change of an account memo
			if (RFIFOREST(fd) < 30)
				return 0;
			memset(account_name, 0, sizeof(account_name));
			strncpy(account_name, RFIFOP(fd,6), 24);
			if (RFIFOL(fd,2) == -1) {
				if (defaultlanguage == 'F') {
					printf("Echec du changement du mémo du compte [%s]. Le compte n'existe pas.\n", account_name);
					ladmin_log("Echec du changement du mémo du compte [%s]. Le compte n'existe pas." RETCODE, account_name);
				} else {
					printf("Account [%s] memo changing failed. Account doesn't exist.\n", account_name);
					ladmin_log("Account [%s] memo changing failed. Account doesn't exist." RETCODE, account_name);
				}
			} else {
				if (defaultlanguage == 'F') {
					printf("Mémo du compte [%s][id: %d] changé avec succès.\n", account_name, RFIFOL(fd,2));
					ladmin_log("Mémo du compte [%s][id: %d] changé avec succès." RETCODE, account_name, RFIFOL(fd,2));
				} else {
					printf("Account [%s][id: %d] memo successfully changed.\n", account_name, RFIFOL(fd,2));
					ladmin_log("Account [%s][id: %d] memo successfully changed." RETCODE, account_name, RFIFOL(fd,2));
				}
			}
			bytes_to_read = 0;
			RFIFOSKIP(fd,30);
			break;

		case 0x7945:	// answer of an account id search
			if (RFIFOREST(fd) < 30)
				return 0;
			memset(account_name, 0, sizeof(account_name));
			strncpy(account_name, RFIFOP(fd,6), 24);
			if (RFIFOL(fd,2) == -1) {
				if (defaultlanguage == 'F') {
					printf("Impossible de trouver l'id du compte [%s]. Le compte n'existe pas.\n", account_name);
					ladmin_log("Impossible de trouver l'id du compte [%s]. Le compte n'existe pas." RETCODE, account_name);
				} else {
					printf("Unable to find the account [%s] id. Account doesn't exist.\n", account_name);
					ladmin_log("Unable to find the account [%s] id. Account doesn't exist." RETCODE, account_name);
				}
			} else {
				if (defaultlanguage == 'F') {
					printf("Le compte [%s] a pour id: %d.\n", account_name, RFIFOL(fd,2));
					ladmin_log("Le compte [%s] a pour id: %d." RETCODE, account_name, RFIFOL(fd,2));
				} else {
					printf("The account [%s] have the id: %d.\n", account_name, RFIFOL(fd,2));
					ladmin_log("The account [%s] have the id: %d." RETCODE, account_name, RFIFOL(fd,2));
				}
			}
			bytes_to_read = 0;
			RFIFOSKIP(fd,30);
			break;

		case 0x7947:	// answer of an account name search
			if (RFIFOREST(fd) < 30)
				return 0;
			memset(account_name, 0, sizeof(account_name));
			strncpy(account_name, RFIFOP(fd,6), 24);
			if (strcmp(account_name, "") == 0) {
				if (defaultlanguage == 'F') {
					printf("Impossible de trouver le nom du compte [%d]. Le compte n'existe pas.\n", RFIFOL(fd,2));
					ladmin_log("Impossible de trouver le nom du compte [%d]. Le compte n'existe pas." RETCODE, RFIFOL(fd,2));
				} else {
					printf("Unable to find the account [%d] name. Account doesn't exist.\n", RFIFOL(fd,2));
					ladmin_log("Unable to find the account [%d] name. Account doesn't exist." RETCODE, RFIFOL(fd,2));
				}
			} else {
				if (defaultlanguage == 'F') {
					printf("Le compte [id: %d] a pour nom: %s.\n", RFIFOL(fd,2), account_name);
					ladmin_log("Le compte [id: %d] a pour nom: %s." RETCODE, RFIFOL(fd,2), account_name);
				} else {
					printf("The account [id: %d] have the name: %s.\n", RFIFOL(fd,2), account_name);
					ladmin_log("The account [id: %d] have the name: %s." RETCODE, RFIFOL(fd,2), account_name);
				}
			}
			bytes_to_read = 0;
			RFIFOSKIP(fd,30);
			break;

		case 0x7949:	// answer of an account validity limit set
			if (RFIFOREST(fd) < 34)
				return 0;
			memset(account_name, 0, sizeof(account_name));
			strncpy(account_name, RFIFOP(fd,6), 24);
			if (RFIFOL(fd,2) == -1) {
				if (defaultlanguage == 'F') {
					printf("Echec du changement de la validité du compte [%s].\n", account_name);
					printf("Le compte n'existe pas ou a déjà la bonne date de validité.\n");
					ladmin_log("Echec du changement de la validité du compte [%s]. Le compte n'existe pas ou a déjà la bonne date de validité." RETCODE, account_name);
				} else {
					printf("Account [%s] validity limit changing failed.\n", account_name);
					printf("Account doesn't exist or already has the good validity date.\n");
					ladmin_log("Account [%s] validity limit changing failed. Account doesn't exist or already has the good validity date." RETCODE, account_name);
				}
			} else {
				time_t timestamp = RFIFOL(fd,30);
				if (timestamp == 0) {
					if (defaultlanguage == 'F') {
						printf("Limite de validité du compte [%s][id: %d] changée avec succès en [illimité].\n", account_name, RFIFOL(fd,2));
						ladmin_log("Limite de validité du compte [%s][id: %d] changée avec succès en [illimité]." RETCODE, account_name, RFIFOL(fd,2));
					} else {
						printf("Validity Limit of the account [%s][id: %d] successfully changed to [unlimited].\n", account_name, RFIFOL(fd,2));
						ladmin_log("Validity Limit of the account [%s][id: %d] successfully changed to [unlimited]." RETCODE, account_name, RFIFOL(fd,2));
					}
				} else {
					char tmpstr[64];
					memset(tmpstr, 0, sizeof(tmpstr));
					strftime(tmpstr, 20, date_format, localtime(&timestamp));
					if (defaultlanguage == 'F') {
						printf("Limite de validité du compte [%s][id: %d] changée avec succès pour être jusqu'au %s.\n", account_name, RFIFOL(fd,2), tmpstr);
						ladmin_log("Limite de validité du compte [%s][id: %d] changée avec succès pour être jusqu'au %s." RETCODE, account_name, RFIFOL(fd,2), tmpstr);
					} else {
						printf("Validity Limit of the account [%s][id: %d] successfully changed to be until %s.\n", account_name, RFIFOL(fd,2), tmpstr);
						ladmin_log("Validity Limit of the account [%s][id: %d] successfully changed to be until %s." RETCODE, account_name, RFIFOL(fd,2), tmpstr);
					}
				}
			}
			bytes_to_read = 0;
			RFIFOSKIP(fd,34);
			break;

		case 0x794b:	// answer of an account ban set
			if (RFIFOREST(fd) < 34)
				return 0;
			memset(account_name, 0, sizeof(account_name));
			strncpy(account_name, RFIFOP(fd,6), 24);
			if (RFIFOL(fd,2) == -1) {
				if (defaultlanguage == 'F') {
					printf("Echec du changement de la date finale de banissement du compte [%s].\n", account_name);
					printf("Le compte n'existe pas ou a déjà la bonne date finale de banissement.\n");
					ladmin_log("Echec du changement de la date finale de banissement du compte [%s]. Le compte n'existe pas ou a déjà la bonne date finale de banissement." RETCODE, account_name);
				} else {
					printf("Account [%s] final date of banishment changing failed.\n", account_name);
					printf("Account doesn't exist or already has the good final date of banishment.\n");
					ladmin_log("Account [%s] final date of banishment changing failed. Account doesn't exist or already has the good final date of banishment." RETCODE, account_name);
				}
			} else {
				time_t timestamp = RFIFOL(fd,30);
				if (timestamp == 0) {
					if (defaultlanguage == 'F') {
						printf("Date finale de banissement du compte [%s][id: %d] changée avec succès en [dé-bannie].\n", account_name, RFIFOL(fd,2));
						ladmin_log("Date finale de banissement du compte [%s][id: %d] changée avec succès en [dé-bannie]." RETCODE, account_name, RFIFOL(fd,2));
					} else {
						printf("Final date of banishment of the account [%s][id: %d] successfully changed to [unbanished].\n", account_name, RFIFOL(fd,2));
						ladmin_log("Final date of banishment of the account [%s][id: %d] successfully changed to [unbanished]." RETCODE, account_name, RFIFOL(fd,2));
					}
				} else {
					char tmpstr[64];
					memset(tmpstr, 0, sizeof(tmpstr));
					strftime(tmpstr, 20, date_format, localtime(&timestamp));
					if (defaultlanguage == 'F') {
						printf("Date finale de banissement du compte [%s][id: %d] changée avec succès pour être jusqu'au %s.\n", account_name, RFIFOL(fd,2), tmpstr);
						ladmin_log("Date finale de banissement du compte [%s][id: %d] changée avec succès pour être jusqu'au %s." RETCODE, account_name, RFIFOL(fd,2), tmpstr);
					} else {
						printf("Final date of banishment of the account [%s][id: %d] successfully changed to be until %s.\n", account_name, RFIFOL(fd,2), tmpstr);
						ladmin_log("Final date of banishment of the account [%s][id: %d] successfully changed to be until %s." RETCODE, account_name, RFIFOL(fd,2), tmpstr);
					}
				}
			}
			bytes_to_read = 0;
			RFIFOSKIP(fd,34);
			break;

		case 0x794d:	// answer of an account ban date/time changing
			if (RFIFOREST(fd) < 34)
				return 0;
			memset(account_name, 0, sizeof(account_name));
			strncpy(account_name, RFIFOP(fd,6), 24);
			if (RFIFOL(fd,2) == -1) {
				if (defaultlanguage == 'F') {
					printf("Echec du changement de la date finale de banissement du compte [%s]. Le compte n'existe pas.\n", account_name);
					ladmin_log("Echec du changement de la date finale de banissement du compte [%s]. Le compte n'existe pas." RETCODE, account_name);
				} else {
					printf("Account [%s] final date of banishment changing failed. Account doesn't exist.\n", account_name);
					ladmin_log("Account [%s] final date of banishment changing failed. Account doesn't exist." RETCODE, account_name);
				}
			} else {
				time_t timestamp = RFIFOL(fd,30);
				if (timestamp == 0) {
					if (defaultlanguage == 'F') {
						printf("Date finale de banissement du compte [%s][id: %d] changée avec succès en [dé-bannie].\n", account_name, RFIFOL(fd,2));
						ladmin_log("Date finale de banissement du compte [%s][id: %d] changée avec succès en [dé-bannie]." RETCODE, account_name, RFIFOL(fd,2));
					} else {
						printf("Final date of banishment of the account [%s][id: %d] successfully changed to [unbanished].\n", account_name, RFIFOL(fd,2));
						ladmin_log("Final date of banishment of the account [%s][id: %d] successfully changed to [unbanished]." RETCODE, account_name, RFIFOL(fd,2));
					}
				} else {
					char tmpstr[64];
					memset(tmpstr, 0, sizeof(tmpstr));
					strftime(tmpstr, 20, date_format, localtime(&timestamp));
					if (defaultlanguage == 'F') {
						printf("Date finale de banissement du compte [%s][id: %d] changée avec succès pour être jusqu'au %s.\n", account_name, RFIFOL(fd,2), tmpstr);
						ladmin_log("Date finale de banissement du compte [%s][id: %d] changée avec succès pour être jusqu'au %s." RETCODE, account_name, RFIFOL(fd,2), tmpstr);
					} else {
						printf("Final date of banishment of the account [%s][id: %d] successfully changed to be until %s.\n", account_name, RFIFOL(fd,2), tmpstr);
						ladmin_log("Final date of banishment of the account [%s][id: %d] successfully changed to be until %s." RETCODE, account_name, RFIFOL(fd,2), tmpstr);
					}
				}
			}
			bytes_to_read = 0;
			RFIFOSKIP(fd,34);
			break;

		case 0x794f:	// answer of a broadcast
			if (RFIFOREST(fd) < 4)
				return 0;
			if (RFIFOW(fd,2) == (unsigned short)-1) {
				if (defaultlanguage == 'F') {
					printf("Echec de l'envoi du message. Aucun serveur de char en ligne.\n");
					ladmin_log("Echec de l'envoi du message. Aucun serveur de char en ligne." RETCODE);
				} else {
					printf("Message sending failed. No online char-server.\n");
					ladmin_log("Message sending failed. No online char-server." RETCODE);
				}
			} else {
				if (defaultlanguage == 'F') {
					printf("Message transmis au serveur de logins avec succès.\n");
					ladmin_log("Message transmis au serveur de logins avec succès." RETCODE);
				} else {
					printf("Message successfully sended to login-server.\n");
					ladmin_log("Message successfully sended to login-server." RETCODE);
				}
			}
			bytes_to_read = 0;
			RFIFOSKIP(fd,4);
			break;

		case 0x7951:	// answer of an account validity limit changing
			if (RFIFOREST(fd) < 34)
				return 0;
			memset(account_name, 0, sizeof(account_name));
			strncpy(account_name, RFIFOP(fd,6), 24);
			if (RFIFOL(fd,2) == -1) {
				if (defaultlanguage == 'F') {
					printf("Echec du changement de la validité du compte [%s]. Le compte n'existe pas.\n", account_name);
					ladmin_log("Echec du changement de la validité du compte [%s]. Le compte n'existe pas." RETCODE, account_name);
				} else {
					printf("Account [%s] validity limit changing failed. Account doesn't exist.\n", account_name);
					ladmin_log("Account [%s] validity limit changing failed. Account doesn't exist." RETCODE, account_name);
				}
			} else {
				time_t timestamp = RFIFOL(fd,30);
				if (timestamp == 0) {
					if (defaultlanguage == 'F') {
						printf("Limite de validité du compte [%s][id: %d] inchangée.\n", account_name, RFIFOL(fd,2));
						printf("Le compte a une validité illimitée ou\n");
						printf("la modification est impossible avec les ajustements demandés.\n");
						ladmin_log("Limite de validité du compte [%s][id: %d] inchangée. Le compte a une validité illimitée ou la modification est impossible avec les ajustements demandés." RETCODE, RFIFOP(fd,6), RFIFOL(fd,2));
					} else {
						printf("Validity limit of the account [%s][id: %d] unchanged.\n", account_name, RFIFOL(fd,2));
						printf("The account have an unlimited validity limit or\n");
						printf("the changing is impossible with the proposed adjustments.\n");
						ladmin_log("Validity limit of the account [%s][id: %d] unchanged. The account have an unlimited validity limit or the changing is impossible with the proposed adjustments." RETCODE, RFIFOP(fd,6), RFIFOL(fd,2));
					}
				} else {
					char tmpstr[64];
					memset(tmpstr, 0, sizeof(tmpstr));
					strftime(tmpstr, 20, date_format, localtime(&timestamp));
					if (defaultlanguage == 'F') {
						printf("Limite de validité du compte [%s][id: %d] changée avec succès pour être jusqu'au %s.\n", account_name, RFIFOL(fd,2), tmpstr);
						ladmin_log("Limite de validité du compte [%s][id: %d] changée avec succès pour être jusqu'au %s." RETCODE, account_name, RFIFOL(fd,2), tmpstr);
					} else {
						printf("Validity limit of the account [%s][id: %d] successfully changed to be until %s.\n", account_name, RFIFOL(fd,2), tmpstr);
						ladmin_log("Validity limit of the account [%s][id: %d] successfully changed to be until %s." RETCODE, account_name, RFIFOL(fd,2), tmpstr);
					}
				}
			}
			bytes_to_read = 0;
			RFIFOSKIP(fd,34);
			break;

		case 0x7953:	// answer of a request about informations of an account (by account name/id)
			if (RFIFOREST(fd) < 150 || RFIFOREST(fd) < (150 + RFIFOW(fd,148)))
				return 0;
		  {
			char userid[25], error_message[20], lastlogin[24], last_ip[16], email[41], memo[65536];
			time_t ban_until_time; // # of seconds 1/1/1970 (timestamp): ban time limit of the account (0 = no ban)
			time_t connect_until_time; // # of seconds 1/1/1970 (timestamp): Validity limit of the account (0 = unlimited)
			memset(userid, 0, sizeof(userid)); /* 25 */
			strncpy(userid, RFIFOP(fd,7), 24);
			memset(error_message, 0, sizeof(error_message));
			strncpy(error_message, RFIFOP(fd,40), 19);
			memset(lastlogin, 0, 24);
			strncpy(lastlogin, RFIFOP(fd,60), 23);
			memcpy(last_ip, RFIFOP(fd,84), sizeof(last_ip));
			last_ip[sizeof(last_ip)-1] = '\0';
			memset(email, 0, sizeof(email)); // 40 + NULL
			strncpy(email, RFIFOP(fd,100), 40);
			connect_until_time = (time_t)RFIFOL(fd,140);
			ban_until_time = (time_t)RFIFOL(fd,144);
			memset(memo, 0, sizeof(memo));
			strncpy(memo, RFIFOP(fd,150), RFIFOW(fd,148));
			if (RFIFOL(fd,2) == -1) {
				if (defaultlanguage == 'F') {
					printf("Impossible de trouver le compte [%s]. Le compte n'existe pas.\n", parameters);
					ladmin_log("Impossible de trouver le compte [%s]. Le compte n'existe pas." RETCODE, parameters);
				} else {
					printf("Unabled to find the account [%s]. Account doesn't exist.\n", parameters);
					ladmin_log("Unabled to find the account [%s]. Account doesn't exist." RETCODE, parameters);
				}
			} else if (userid[0] == '\0') {
				if (defaultlanguage == 'F') {
					printf("Impossible de trouver le compte [id: %s]. Le compte n'existe pas.\n", parameters);
					ladmin_log("Impossible de trouver le compte [id: %s]. Le compte n'existe pas." RETCODE, parameters);
				} else {
					printf("Unabled to find the account [id: %s]. Account doesn't exist.\n", parameters);
					ladmin_log("Unabled to find the account [id: %s]. Account doesn't exist." RETCODE, parameters);
				}
			} else {
				if (defaultlanguage == 'F') {
					ladmin_log("Réception d'information concernant un compte." RETCODE);
					printf("Le compte a les caractéristiques suivantes:\n");
				} else {
					ladmin_log("Receiving information about an account." RETCODE);
					printf("The account is set with:\n");
				}
				if (RFIFOB(fd,6) == 0) {
					printf(" Id:     %d (non-GM)\n", RFIFOL(fd,2));
				} else {
					if (defaultlanguage == 'F') {
						printf(" Id:     %d (GM niveau %d)\n", RFIFOL(fd,2), (int)RFIFOB(fd,6));
					} else {
						printf(" Id:     %d (GM level %d)\n", RFIFOL(fd,2), (int)RFIFOB(fd,6));
					}
				}
				if (defaultlanguage == 'F') {
					printf(" Nom:    '%s'\n", userid);
					if (RFIFOB(fd,31) == 0)
						printf(" Sexe:   Femme\n");
					else if (RFIFOB(fd,31) == 1)
						printf(" Sexe:   Male\n");
					else
						printf(" Sexe:   Serveur\n");
				} else {
					printf(" Name:   '%s'\n", userid);
					if (RFIFOB(fd,31) == 0)
						printf(" Sex:    Female\n");
					else if (RFIFOB(fd,31) == 1)
						printf(" Sex:    Male\n");
					else
						printf(" Sex:    Server\n");
				}
				printf(" E-mail: %s\n", email);
				switch(RFIFOW(fd,36)) {
				case 0:
					if (defaultlanguage == 'F')
						printf(" Statut: 0 [Compte Ok]\n");
					else
						printf(" Statut: 0 [Account OK]\n");
					break;
				case 1:
					printf(" Statut: 1 [Unregistered ID]\n");
					break;
				case 2:
					printf(" Statut: 2 [Incorrect Password]\n");
					break;
				case 3:
					printf(" Statut: 3 [This ID is expired]\n");
					break;
				case 4:
					printf(" Statut: 4 [Rejected from Server]\n");
					break;
				case 5:
					printf(" Statut: 5 [You have been blocked by the GM Team]\n");
					break;
				case 6:
					printf(" Statut: 6 [Your Game's EXE file is not the latest version]\n");
					break;
				case 7:
					printf(" Statut: 7 [You are Prohibited to log in until %s]\n", error_message);
					break;
				case 8:
					printf(" Statut: 8 [Server is jammed due to over populated]\n");
					break;
				case 9:
					printf(" Statut: 9 [No MSG]\n");
					break;
				default: // 100
					printf(" Statut: %d [This ID is totally erased]\n", RFIFOW(fd,36));
					break;
				}
				printf("       : %s\n", (RFIFOB(fd,38)) ? "ONLINE" : "OFFline");
				if (defaultlanguage == 'F') {
					if (ban_until_time == 0) {
						printf(" Banissement: non banni.\n");
					} else {
						char tmpstr[64];
						memset(tmpstr, 0, sizeof(tmpstr));
						strftime(tmpstr, 20, date_format, localtime(&ban_until_time));
						printf(" Banissement: jusqu'au %s.\n", tmpstr);
					}
					if (RFIFOL(fd,32) > 1)
						printf(" Compteur: %d connexions.\n", RFIFOL(fd,32));
					else
						printf(" Compteur: %d connexion.\n", RFIFOL(fd,32));
					printf(" Dernière connexion le: %s (ip: %s)\n", lastlogin, last_ip);
					if (connect_until_time == 0) {
						printf(" Limite de validité: illimité.\n");
					} else {
						char tmpstr[64];
						memset(tmpstr, 0, sizeof(tmpstr));
						strftime(tmpstr, 20, date_format, localtime(&connect_until_time));
						printf(" Limite de validité: jusqu'au %s.\n", tmpstr);
					}
				} else {
					if (ban_until_time == 0) {
						printf(" Banishment: not banished.\n");
					} else {
						char tmpstr[64];
						memset(tmpstr, 0, sizeof(tmpstr));
						strftime(tmpstr, 20, date_format, localtime(&ban_until_time));
						printf(" Banishment: until %s.\n", tmpstr);
					}
					if (RFIFOL(fd,32) > 1)
						printf(" Count:  %d connections.\n", RFIFOL(fd,32));
					else
						printf(" Count:  %d connection.\n", RFIFOL(fd,32));
					printf(" Last connection at: %s (ip: %s).\n", lastlogin, last_ip);
					if (connect_until_time == 0) {
						printf(" Validity limit: unlimited.\n");
					} else {
						char tmpstr[64];
						memset(tmpstr, 0, sizeof(tmpstr));
						strftime(tmpstr, 20, date_format, localtime(&connect_until_time));
						printf(" Validity limit: until %s.\n", tmpstr);
					}
				}
				if (memo[0] == '\0')
					printf(" Memo:   '-'\n");
				else
					printf(" Memo:   '%s'\n", memo);
			}
		  }
			bytes_to_read = 0;
			RFIFOSKIP(fd,150 + RFIFOW(fd,148));
			break;

		case 0x7956:	// answer of the addition of a text to an account memo
			if (RFIFOREST(fd) < 30)
				return 0;
			memset(account_name, 0, sizeof(account_name));
			strncpy(account_name, RFIFOP(fd,6), 24);
			if (RFIFOL(fd,2) == -1) {
				if (defaultlanguage == 'F') {
					printf("Echec de l'ajout d'un texte au mémo du compte [%s].\n", account_name);
					printf("Le compte n'existe pas.\n");
					ladmin_log("Echec de l'ajout d'un texte au mémo du compte [%s]. Le compte n'existe pas." RETCODE, account_name);
				} else {
					printf("Text's addition to an account [%s] memo to failed.\n", account_name);
					printf("Account doesn't exist.\n");
					ladmin_log("Text's addition to an account [%s] memo to failed. Account doesn't exist." RETCODE, account_name);
				}
			} else {
				if (defaultlanguage == 'F') {
					printf("Ajout d'un texte au mémo du compte [%s][id: %d] réussi.\n", account_name, RFIFOL(fd,2));
					ladmin_log("Ajout d'un texte au mémo du compte [%s][id: %d] réussi." RETCODE, account_name, RFIFOL(fd,2));
				} else {
					printf("Addition of a text to a account [%s][id: %d] memo successfully done.\n", account_name, RFIFOL(fd,2));
					ladmin_log("Addition of a text to a account [%s][id: %d] memo successfully done." RETCODE, account_name, RFIFOL(fd,2));
				}
			}
			bytes_to_read = 0;
			RFIFOSKIP(fd,30);
			break;

		default:
			printf("Remote administration has been disconnected (unknown packet).\n");
			ladmin_log("'End of connection, unknown packet." RETCODE);
			session[fd]->eof = 1;
			return 0;
		}
	}

	// if we don't wait new packets, do the prompt
	prompt();

	return 0;
}

//------------------------------------
// Function to connect to login-server
//------------------------------------
static inline void Connect_login_server() {
	if (defaultlanguage == 'F') {
		printf("Essai de connexion au serveur de logins (%s:%d)...\n", loginserverip, loginserverport);
		ladmin_log("Essai de connexion au serveur de logins (%s:%d)..." RETCODE, loginserverip, loginserverport);
	} else {
		printf("Connecting to login-server (%s:%d)...\n", loginserverip, loginserverport);
		ladmin_log("Connecting to login-server (%s:%d)..." RETCODE, loginserverip, loginserverport);
	}

	login_fd = make_connection(login_ip, loginserverport);
	if (login_fd == -1) {
		if (defaultlanguage == 'F') {
			printf("Impossible de se connecter au serveur de logins.\n");
			ladmin_log("Impossible de se connecter au serveur de logins." RETCODE);
		} else {
			printf("Unable to connect to login-server.\n");
			ladmin_log("Unable to connect to login-server." RETCODE);
		}
		exit(1);
	}
	realloc_fifo(login_fd, RFIFOSIZE_SERVER, WFIFOSIZE_SERVER);

#ifdef PASSWORDENC
	if (passenc == 0) {
#endif
		WPACKETW(0) = 0x7918; // Request for administation login
		WPACKETW(2) = 0; // no encrypted
		memcpy(WPACKETP(4), loginserveradminpassword, 24);
		SENDPACKET(login_fd, 28);
		bytes_to_read = 1;
		if (defaultlanguage == 'F') {
			printf("Envoi du mot de passe...\n");
			ladmin_log("Envoi du mot de passe..." RETCODE);
		} else {
			printf("Sending password...\n");
			ladmin_log("Sending password..." RETCODE);
		}
#ifdef PASSWORDENC
	} else {
		WPACKETW(0) = 0x791a; // Sending request about the coding key
		SENDPACKET(login_fd, 2);
		bytes_to_read = 1;
		if (defaultlanguage == 'F') {
			printf("Demande de la clef MD5...\n");
			ladmin_log("Demande de la clef MD5..." RETCODE);
		} else {
			printf("Requesting the MD5 challenge...\n");
			ladmin_log("Requesting the MD5 challenge..." RETCODE);
		}
	}
#endif

	return;
}

//-----------------------------------
// Reading general configuration file
//-----------------------------------
static void ladmin_config_read(const char *cfgName) { // not inline, called too often
	char line[1024], w1[1024], w2[1024];
	FILE *fp;

	fp = fopen(cfgName, "r");
	if ((fp = fopen(cfgName, "r")) == NULL) {
//		if ((fp = fopen("conf/ladmin_freya.conf", "r")) == NULL) { // not try default, possible infinite loop with import
			if (defaultlanguage == 'F') {
				printf(CL_RESET "Fichier de configuration (%s) non trouvé.\n", cfgName);
			} else {
				printf(CL_RESET "Configuration file (%s) not found.\n", cfgName);
			}
			return;
//		}
	}

	if (defaultlanguage == 'F') {
		printf(CL_RESET "---Début de lecture du fichier de configuration Ladmin (%s).\n", cfgName);
	} else {
		printf(CL_RESET "---Start reading of Ladmin configuration file (%s).\n", cfgName);
	}
	while(fgets(line, sizeof(line) - 1, fp)) { // fgets reads until maximum one less than size and add '\0' -> so, it's not necessary to add -1
		if ((line[0] == '/' && line[1] == '/') || line[0] == '\0' || line[0] == '\n' || line[0] == '\r')
			continue;
		// it's not necessary to remove 'carriage return ('\n' or '\r')

		memset(w2, 0, sizeof(w2));
		if (sscanf(line, "%[^:]: %[^\r\n]", w1, w2) == 2) {
			remove_control_chars(w1);
			remove_control_chars(w2);

			if (strcasecmp(w1, "login_ip") == 0) {
				struct hostent *h = gethostbyname(w2);
				if (h != NULL) {
					if (defaultlanguage == 'F') {
						printf("Adresse du serveur de logins: %s -> %d.%d.%d.%d.\n", w2, (unsigned char)h->h_addr[0], (unsigned char)h->h_addr[1], (unsigned char)h->h_addr[2], (unsigned char)h->h_addr[3]);
					} else {
						printf("Login server IP address: %s -> %d.%d.%d.%d.\n", w2, (unsigned char)h->h_addr[0], (unsigned char)h->h_addr[1], (unsigned char)h->h_addr[2], (unsigned char)h->h_addr[3]);
					}
					sprintf(loginserverip, "%d.%d.%d.%d", (unsigned char)h->h_addr[0], (unsigned char)h->h_addr[1], (unsigned char)h->h_addr[2], (unsigned char)h->h_addr[3]);
				} else
					memcpy(loginserverip, w2, 16);
			} else if (strcasecmp(w1, "login_port") == 0) {
				loginserverport = atoi(w2);
			} else if (strcasecmp(w1, "admin_pass") == 0) {
				memset(loginserveradminpassword, 0, sizeof(loginserveradminpassword));
				strncpy(loginserveradminpassword, w2, sizeof(loginserveradminpassword) - 1);
#ifdef PASSWORDENC
			} else if (strcasecmp(w1, "passenc") == 0) {
				passenc = atoi(w2);
				if (passenc < 0 || passenc > 2)
					passenc = 0;
#endif
			} else if (strcasecmp(w1, "defaultlanguage") == 0) {
				if (w2[0] == 'F' || w2[0] == 'E')
					defaultlanguage = w2[0];
			} else if (strcasecmp(w1, "ladmin_log_filename") == 0) {
				memset(ladmin_log_filename, 0, sizeof(ladmin_log_filename));
				strncpy(ladmin_log_filename, w2, sizeof(ladmin_log_filename) - 1);
			} else if (strcasecmp(w1, "log_file_date") == 0) {
				log_file_date = atoi(w2);
				if (log_file_date > 4)
					log_file_date = 3; // default
			} else if (strcasecmp(w1, "date_format") == 0) { // note: never have more than 19 char for the date!
				switch (atoi(w2)) {
				case 0:
					strcpy(date_format, "%d-%m-%Y %H:%M:%S"); // 31-12-2004 23:59:59
					break;
				case 1:
					strcpy(date_format, "%m-%d-%Y %H:%M:%S"); // 12-31-2004 23:59:59
					break;
				case 2:
					strcpy(date_format, "%Y-%d-%m %H:%M:%S"); // 2004-31-12 23:59:59
					break;
				case 3:
					strcpy(date_format, "%Y-%m-%d %H:%M:%S"); // 2004-12-31 23:59:59
					break;
				}
// import
			} else if (strcasecmp(w1, "import") == 0) {
				printf("ladmin_config_read: Import file: %s.\n", w2);
				ladmin_config_read(w2);
			}
		}
	}
	fclose(fp);

	login_ip = inet_addr(loginserverip);

	if (defaultlanguage == 'F') {
		printf("---Lecture du fichier de configuration Ladmin terminée.\n");
	} else {
		printf("---End reading of Ladmin configuration file.\n");
	}

	return;
}

//--------------------------------------
// Function called at exit of the server
//--------------------------------------
void do_final(void) {

	if (already_exit_function == 0) {
		int fd;

		if (login_fd >= 0) {
#ifdef __WIN32
			shutdown(login_fd, SD_BOTH);
			closesocket(login_fd);
#else
			close(login_fd);
#endif
			delete_session(login_fd);
			login_fd = -1;
		}

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

		if (defaultlanguage == 'F') {
			printf(CL_RESET "----Fin de Ladmin (fin normale avec fermeture de tous les fichiers).\n");
			ladmin_log("----Fin de Ladmin (fin normale avec fermeture de tous les fichiers)." RETCODE);
		} else {
			printf(CL_RESET "----End of Ladmin (normal end with closing of all files).\n");
			ladmin_log("----End of Ladmin (normal end with closing of all files)." RETCODE);
		}

		already_exit_function = 1;
	}
}

//------------------------
// Main function of ladmin
//------------------------
void do_init(const int argc, char **argv) {
	init_flag = 0; // flag to know if init is terminated (0: no, 1: yes)

	// read ladmin configuration
	ladmin_config_read((argc > 1) ? argv[1] : LADMIN_CONF_NAME);

	ladmin_log("");
	if (defaultlanguage == 'F') {
		ladmin_log("Fichier de configuration lu." RETCODE);
	} else {
		ladmin_log("Configuration file read." RETCODE);
	}

	srand(time(NULL));

	atexit(do_final);
//	set_termfunc(do_final);
	set_defaultparse(parse_fromlogin);

	if (defaultlanguage == 'F') {
		printf("Outil d'administration à distance de Freya.\n");
		printf("(pour Freya version %d.%d.%d.).\n", FREYA_MAJORVERSION, FREYA_MINORVERSION, FREYA_REVISION);
	} else {
		printf("Freya login-server administration tool.\n");
		printf("(for Freya version %d.%d.%d.).\n", FREYA_MAJORVERSION, FREYA_MINORVERSION, FREYA_REVISION);
	}

	Connect_login_server();

	if (defaultlanguage == 'F') {
		ladmin_log("Ladmin est prêt." RETCODE);
		printf("Ladmin est " CL_GREEN "prêt" CL_RESET ".\n\n");
	} else {
		ladmin_log("Ladmin is ready." RETCODE);
		printf("Ladmin is " CL_GREEN "ready" CL_RESET ".\n\n");
	}

	init_flag = 1; // flag to know if init is terminated (0: no, 1: yes)

	return;
}
