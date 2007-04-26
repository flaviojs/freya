// Copyright (c) Freya Development Team - Licensed under GNU GPL
// For more information, see License.txt in the main folder

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h> // toupper, iscntrl
#ifdef __WIN32
// time_t location :
#include <sys/types.h>
#endif

#include <time.h> // time_t

#include "../common/utils.h" // for 'colors' in printf

#if defined __CYGWIN || defined __WIN32
// txtやlogなどの書き出すファイルの改行コード
#define RETCODE "\r\n" // (CR/LF：Windows系)
#else
#define RETCODE "\n" // (LF：Unix系）
#endif

char account_txt[1024] = "../save/account.txt";

struct auth_dat {
	char userid[24];
} *auth_dat = NULL;

//-----------------------------------------------------
// Function to suppress control characters in a string.
//-----------------------------------------------------
int remove_control_chars(char *str) {
	int change = 0;

	while(*str) {
		if (iscntrl((int)(*str))) { // if (*str > 0 && *str < 32) {
			*str = '_';
			change++;
		}
		str++;
	}

	return change;
}

//------------------------------------------------
// Function to remove all escape char in a string.
//------------------------------------------------
static void remove_escape_chars(char *buf) { // not inline, called too often
	int i, j;

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

	return;
}

int main(int argc, char *argv[]) {
	char username[24];
	char password[25];
	char accountsex[2];
	char *p;
	FILE *FPaccin;
	int next_id;
	char line[2048], buf[1024];
	int auth_num, auth_max;
	int server_count;
	FILE *FPaccout;

	// to read account line
	int account_id, logincount, state, i;
	char userid[2048], pass[2048], lastlogin[2048], sex, email[2048], error_message[2048], last_ip[2048], memo[2048];
	time_t ban_until_time;
	time_t connect_until_time;

	printf("FREYA simple accounts creation tool for TXT version\n"
	       "===================================================\n\n");

	printf("Don't create an account if the login-server is online!!!\n");
	printf("If the login-server is online, press ctrl+C now to stop this software.\n\n");

	// get file account file name
	printf(CL_DARK_CYAN "Enter the file name and path ('enter' = '%s') \n-> " CL_RESET, account_txt);
	memset(buf, 0, sizeof(buf));
	fgets(buf, sizeof(buf), stdin); // fgets reads until maximum one less than size and add '\0' -> so, it's not necessary to add -1
	if ((p = strrchr(buf, '\n')) != NULL)
		p[0] = '\0';
	remove_escape_chars(buf);
	if (buf[0] != '\0') {
		memcpy(account_txt, buf, sizeof(account_txt));
		account_txt[sizeof(account_txt) - 1] = '\0';
		remove_control_chars(account_txt);
	}

	// Check to see if account.txt exists.
	printf("Checking if '%s' file exists... ", account_txt);
	if ((FPaccin = fopen(account_txt, "r")) == NULL) {
		printf("File not found!\n");
		printf("Run the setup wizard please.\n");
		exit(0);
	}
	printf("File exists.\n");

	auth_num = 0;
	server_count = 0;
	auth_max = 256;
	auth_dat = (struct auth_dat*)calloc(sizeof(struct auth_dat), 256);

	printf("Checking for next id... ");
	fflush(stdout);
	next_id = 2000001;
	while(fgets(line, sizeof(line), FPaccin)) { // fgets reads until maximum one less than size and add '\0' -> so, it's not necessary to add -1
		if ((line[0] == '/' && line[1] == '/') || line[0] == '\0' || line[0] == '\n' || line[0] == '\r')
			continue;
		// remove carriage return if exist
		while(line[0] != '\0' && (line[strlen(line)-1] == '\n' || line[strlen(line)-1] == '\r'))
			line[strlen(line)-1] = '\0';

		memset(userid, 0, sizeof(userid));

		// database version reading (v3)
		if ((i = sscanf(line, "%d\t%[^\t]\t%[^\t]\t%[^\t]\t%c\t%d\t%d\t%[^\t]\t%[^\t]\t%ld\t%[^\t]\t%[^\t]\t%ld\t",
		                &account_id, userid, pass, lastlogin, &sex, &logincount, &state,
		                email, error_message, &connect_until_time, last_ip, memo, &ban_until_time)) == 13 ||
		// database version reading (v2)
		    (i = sscanf(line, "%d\t%[^\t]\t%[^\t]\t%[^\t]\t%c\t%d\t%d\t%[^\t]\t%[^\t]\t%ld\t%[^\t]\t%[^\t]\t",
		                &account_id, userid, pass, lastlogin, &sex, &logincount, &state,
		                email, error_message, &connect_until_time, last_ip, memo)) == 12 ||
		// Old athena database version reading (v1)
		    (i = sscanf(line, "%d\t%[^\t]\t%[^\t]\t%[^\t]\t%c\t%d\t%d\t",
		         &account_id, userid, pass, lastlogin, &sex, &logincount, &state)) >= 5) {
			userid[23] = '\0';
			remove_control_chars(userid);
			if (auth_num >= auth_max) {
				auth_max += 256;
				auth_dat = (struct auth_dat*)realloc(auth_dat, sizeof(struct auth_dat) * auth_max);
				memset(auth_dat + (auth_max - 256), 0, sizeof(struct auth_dat) * 256);
			}
			strncpy(auth_dat[auth_num].userid, userid, 24);
			auth_num++;
			if (sex == 'S' || sex == 's')
				server_count++;
			if (account_id >= next_id)
				next_id = account_id + 1;
		} else {
			i = 0;
			if (sscanf(line, "%d\t%%newid%%\n%n", &account_id, &i) == 1 && i > 0 && account_id > next_id)
				next_id = account_id;
		}
	}
	fclose(FPaccin);
	printf("Id for next account: %d.\n", next_id);

	if (auth_num == 0) {
		printf("No account found.\n");
	} else {
		if (auth_num == 1) {
			printf("1 account read, ");
		} else {
			printf("%d accounts read, ", auth_num);
		}
		if (server_count == 0) {
			printf("of which is no server account ('S').\n");
		} else if (server_count == 1) {
			printf("of which is 1 server account ('S').\n");
		} else {
			printf("of which are %d server accounts ('S').\n", server_count);
		}
	}
	printf("\n");

	memset(username, 0, sizeof(username));
	while(strlen(username) < 4 || strlen(username) > 23) {
		printf(CL_DARK_CYAN "Enter an username (4-23 characters): " CL_RESET);
		memset(buf, 0, sizeof(buf));
		fgets(buf, sizeof(buf), stdin); // fgets reads until maximum one less than size and add '\0' -> so, it's not necessary to add -1
		if ((p = strrchr(buf, '\n')) != NULL)
			p[0] = '\0';
		remove_escape_chars(buf);
		memcpy(username, buf, sizeof(username));
		username[sizeof(username) - 1] = '\0';
		remove_control_chars(username);
		// check if name already exist
		for(i = 0; i < auth_num; i++)
			if (strcmp(auth_dat[i].userid, username) == 0) {
				printf(CL_RED "******Error: username already exists." CL_RESET "\n");
				break;
			}
		if (i != auth_num)
			memset(username, 0, sizeof(username));
	}

	memset(password, 0, sizeof(password));
	while(strlen(password) < 4 || strlen(password) > 24) {
		printf(CL_DARK_CYAN "Enter a password (4-24 characters): " CL_RESET);
		memset(buf, 0, sizeof(buf));
		fgets(buf, sizeof(buf), stdin); // fgets reads until maximum one less than size and add '\0' -> so, it's not necessary to add -1
		if ((p = strrchr(buf, '\n')) != NULL)
			p[0] = '\0';
		remove_escape_chars(buf);
		memcpy(password, buf, sizeof(password));
		password[sizeof(password) - 1] = '\0';
		remove_control_chars(password);
	}

	memset(accountsex, 0, sizeof(accountsex));
	while(strcmp(accountsex, "F") != 0 && strcmp(accountsex, "M") != 0) {
		printf(CL_DARK_CYAN "Enter a gender (M for male, F for female): " CL_RESET);
		memset(buf, 0, sizeof(buf));
		fgets(buf, sizeof(buf), stdin); // fgets reads until maximum one less than size and add '\0' -> so, it's not necessary to add -1
		if ((p = strrchr(buf, '\n')) != NULL)
			p[0] = '\0';
		remove_escape_chars(buf);
		accountsex[0] = toupper((unsigned char)buf[0]); // toupper needs unsigned char
		accountsex[1] = '\0';
	}

	FPaccout = fopen(account_txt, "r+");

	fseek(FPaccout, 0, SEEK_END);
	fprintf(FPaccout, "%i	%s	%s	-	%s	-" RETCODE, next_id, username, password, accountsex);
	fclose(FPaccout);

	printf("Account added.\n");

	return 0;
}

