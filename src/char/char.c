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
#include <sys/time.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <netdb.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h> // tolower
#include <sys/stat.h> // for stat/lstat/fstat

#include "../common/core.h"
#include "../common/socket.h"
#include "../common/timer.h"
#include <mmo.h>
#include "../common/version.h"
#include "../common/lock.h"
#include "char.h"
#include "../common/malloc.h"
#include "../common/utils.h"
#include "../common/addons.h"
#include "../common/console.h"

#include "inter.h"
#include "int_pet.h"
#include "int_guild.h"
#include "int_party.h"
#include "int_storage.h"

#ifdef MEMWATCH
#include "memwatch.h"
#endif

#ifdef USE_SQL
char char_db[256] = "char";
char cart_db[256] = "cart_inventory";
char inventory_db[256] = "inventory";
char charlog_db[256] = "charlog";
char storage_db[256] = "storage";
char interlog_db[256] = "interlog";
char global_reg_value[256] = "global_reg_value";
char skill_db[256] = "skill";
char memo_db[256] = "memo";
char guild_db[256] = "guild";
char guild_alliance_db[256] = "guild_alliance";
char guild_castle_db[256] = "guild_castle";
char guild_expulsion_db[256] = "guild_expulsion";
char guild_member_db[256] = "guild_member";
char guild_position_db[256] = "guild_position";
char guild_skill_db[256] = "guild_skill";
char guild_storage_db[256] = "guild_storage";
char party_db[256] = "party";
char pet_db[256] = "pet";
char friends_db[256] = "friends";
char statuschange_db[256] = "statuschange";
char rank_db[256] = "ranking";

char *SQL_CONF_NAME = "conf/inter_freya.conf";
#endif /* USE_SQL */

struct mmo_map_server server[MAX_MAP_SERVERS];
int server_fd[MAX_MAP_SERVERS];
unsigned char server_freezeflag[MAX_MAP_SERVERS]; // Map-server anti-freeze system. Counter. 6 ok, 5...0 frozen
int anti_freeze_counter = 12;
int anti_freeze_interval = 0;

int login_fd = -1, char_fd = -1;
char userid[24];
char passwd[25];
char server_name[21]; // 20 + NULL
char wisp_server_name[25] = "Server";
int server_char_id;
int login_ip_set_ = 0;
char login_ip_str[16];
int login_ip;
int login_port = 6900;
int char_ip_set_ = 0;
char char_ip_str[16];
int char_ip;
int char_port = 6121;
int char_maintenance;
int char_new;
int email_creation = 0; // disabled by default
#ifdef TXT_ONLY
char char_txt[1024] = "save/athena.txt";
char friends_txt[1024] = "save/friends.txt";
unsigned char friends_txt_flag = 0; // must we save friends file or not? (0: not, x: yes)
#endif /* TXT_ONLY */
char unknown_char_name[1024] = "Unknown";
char char_log_filename[1024] = "log/char.log";
static unsigned char log_file_date = 3; /* year + month (example: log/login-2006-12.log) */
char temp_char_buffer[1024]; // temporary buffer of type char (see php_addslashes)
FILE *log_fp = NULL;

int log_char = 0;

// Maps-servers connection security
#define ACO_STRSIZE 32 // max normal value: 255.255.255.255/255.255.255.255 + NULL, so 15 + 1 + 15 + 1 = 32
static int access_map_allownum = 0;
static char *access_map_allow = NULL;

// Added for lan support
char lan_map_ip[128];
int subnet[4];
int subnetmask[4];

int name_ignoring_case = 0; // Allow or not identical name for characters but with a different case by [Yor]
int char_name_option = 0; // Option to know which letters/symbols are authorized in the name of a character (0: all, 1: only those in char_name_letters, 2: all EXCEPT those in char_name_letters) by [Yor]
char char_name_letters[1024] = ""; // list of letters/symbols used to authorize or not a name of a character. by [Yor]
int char_name_with_spaces = 1; // option to authorize spaces in a character name
int char_name_double_spaces = 0; // option to authorize double spaces in a character name
int char_name_space_at_first = 0; // option to authorize a space as first char in a character name
int char_name_space_at_last = 0; // option to authorize a space as last char in a character name
char char_name_language[6] = "en_US"; // Language used by the server (for manner)
int char_name_language_check = 1; // option to authorize bad words checks in a character name
int chars_per_account = 0; // Maximum characters per account (0 = disabled, 1 to 9)

struct char_session_data{
	int account_id, login_id1, login_id2;
	int found_char[9]; // found_char = index in db (TXT), char_id (SQL), -1: void
	char email[41]; // e-mail (default: a@a.com) by [Yor] // 40 + NULL
	time_t connect_until_time; // # of seconds 1/1/1970 (timestamp): Validity limit of the account (0 = unlimited)
	unsigned char sex;
	unsigned auth : 1; // 0: not already authentified, 1: authentified
};

#define AUTH_FIFO_SIZE ((FD_SETSIZE > 1024) ? FD_SETSIZE : 1024) // max possible connections (previously: 1024)
struct {
	int account_id, char_id, login_id1, login_id2, ip, char_pos;
	unsigned char delflag; // 0: auth_fifo canceled/void, 2: auth_fifo received from login/map server in memory, 1: connection authentified
	unsigned char sex;
	time_t connect_until_time; // # of seconds 1/1/1970 (timestamp): Validity limit of the account (0 = unlimited)
	int map_auth; // 1 if it's map authentification (to avoid "Server still recognises your last login")
} auth_fifo[AUTH_FIFO_SIZE];
int auth_fifo_pos = 0;

unsigned char check_ip_flag = 1; // It's to check IP of a player between char-server and other servers (part of anti-hacking system)
unsigned char check_authfifo_login2 = 1; // It's to check the login2 value (part of anti-hacking system) - related to the client's versions higher than 18.

int char_id_count = 500000; // before: 150000, now 500000 because floor items have id from 0 to 499999
struct mmo_charstatus *char_dat;
struct global_reg_db { // same size of char_dat, to conserv global reg
	unsigned short global_reg_num; // 0-700 (GLOBAL_REG_NUM)
	struct global_reg *global_reg; // same size of char_dat, to conserv global reg
} *global_reg_db = NULL;
time_t *char_dat_timer; // save last modification of a character
int char_num, char_max;
int max_connect_user = 0;
unsigned char min_level_to_bypass = 99; // Sets default min_level_to_bypass
int autosave_interval = 300 * 1000;

// Initial position (it's possible to set it in conf file)
struct point start_point = {"new_1-1.gat", 53, 111};
int start_weapon = 1201;
int start_armor = 2301;
int start_item = 0;
int start_item_quantity = 0;
int start_zeny = 0;
int max_hair_style = 24; // added by [Yor]
int max_hair_color = 9; // added by [Yor]

struct gm_account *gm_account = NULL;
int GM_num = 0;

// online players by [Yor]
char online_txt_filename[1024] = "online.txt";
char online_html_filename[1024] = "online.html";
char online_php_filename[1024] = "online.php";
int online_sorting_option = 0; // sorting option to display online players in online files
int online_display_option_txt = 0; // display options: to know which columns must be displayed
#ifdef TXT_ONLY
int online_display_option_html = 1; // display options: to know which columns must be displayed (default: TXT 1, SQL 0)
#else // TXT_ONLY -> USE_SQL
int online_display_option_html = 0; // display options: to know which columns must be displayed (default: TXT 1, SQL 0)
#endif // USE_SQL
int online_display_option_php = 0; // display options: to know which columns must be displayed
int online_refresh_html = 20; // refresh time (in sec) of the html file in the explorer
unsigned char online_gm_display_min_level = 20; // minimum GM level to display 'GM' when we want to display it

#ifdef TXT_ONLY
struct friend_dat { // same size of char_dat, to conserv friends
	unsigned char friend_num; // 0-40 (MAX_FRIENDS)
	struct friend *friends; // same size of char_dat, to conserv friends
} *friend_dat = NULL;
#endif // TXT_ONLY
signed char *online_chars; // same size of char_dat, and id value of current server (or -1)
time_t update_online; // to update online files when we receiving information from a server (not less than 8 seconds)

struct account_reg2_db { // same size of char_dat, to conserv friends
	int account_id;
	time_t update_time; // last update (to remove from memory)
	unsigned short num; // 0 - 700 (ACCOUNT_REG2_NUM)
	struct global_reg *reg2;
} *account_reg2_db = NULL;
int account_reg2_db_num = 0;

int console = 0;
char console_pass[1024] = "consoleon"; /* password to enable console */

//------------------------------
// Writing function of logs file
//------------------------------
void char_log(char *fmt, ...)
{
	if(log_char != 1 && log_char != 2)
		return;

	va_list ap;
	struct timeval tv;
	time_t now;
	char tmpstr[2048];
	static char log_filename_used[sizeof(char_log_filename) + 64] = "1"; // +64 for date size
	char log_filename_to_use[sizeof(char_log_filename) + 64]; // must be different to log_filename_used

	// get time for file name and logs
	gettimeofday(&tv, NULL);
	now = time(NULL);

	// create file name
	memset(log_filename_to_use, 0, sizeof(log_filename_to_use));
	if (log_file_date == 0) {
		strcpy(log_filename_to_use, char_log_filename);
	} else {
		char* last_point;
		char* last_slash; // to avoid name like ../log_file_name_without_point
		// search position of '.'
		last_point = strrchr(char_log_filename, '.');
		last_slash = strrchr(char_log_filename, '/');
		if (last_point == NULL || (last_slash != NULL && last_slash > last_point))
			last_point = char_log_filename + strlen(char_log_filename);
		strncpy(log_filename_to_use, char_log_filename, last_point - char_log_filename);
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
			strcpy(log_filename_to_use, char_log_filename);
			break;
		}
	}

	// if previously used file has different name from the file to use
	if (strcmp(log_filename_used, log_filename_to_use) != 0) {
		if (log_fp != NULL) {
			fclose(log_fp);
			log_fp = NULL; // it'll be checked down there...
		}
//		counter = 0;
		memcpy(log_filename_used, log_filename_to_use, sizeof(log_filename_used));
	}

	// if not already open, try to open log
	if (log_fp == NULL)
		log_fp = fopen(log_filename_used, "a");

	if (log_fp) {
		if (fmt[0] == '\0') // jump a line if no message
			fprintf(log_fp, RETCODE);
		else {
			va_start(ap, fmt);
			memset(tmpstr, 0, sizeof(tmpstr));
			strftime(tmpstr, 24, "%d-%m-%Y %H:%M:%S", localtime(&now));
			sprintf(tmpstr + strlen(tmpstr), ".%03d: %s", (int)tv.tv_usec / 1000, fmt);
			vfprintf(log_fp, tmpstr, ap);
			va_end(ap);
		}
//		counter++;
//		if ((counter % 10) == 9) {
			fflush(log_fp); // under cygwin or windows, if software is stopped, data are not written in the file -> fflush at every line
//			counter = 0;
//		}
	}

	return;
}

/*==========================================
 * manner system (by [Yor])
 *------------------------------------------
 */
// manner system - variables definition
//------------------------------------------
#define MANNER_SIZE 80 // fix max size of sentence/word in manner
char *MANNER_CONF_NAME = "conf/manner.txt";
char *manner_db = NULL;
unsigned int manner_counter = 0;
long creation_time_manner_file = -1;

// manner system - remove manner table from memory
//------------------------------------------
void delete_manner() {
	// clean up manner table
	FREE(manner_db);
	manner_counter = 0;

	return;
}

// manner system - file reading
//------------------------------------------
void read_manner() {
	struct stat file_stat;
	FILE *fp;
	char line[MANNER_SIZE], w1[MANNER_SIZE], w2[MANNER_SIZE];
	int add_word, i;

	delete_manner();

	// get last modify time/date
	if (stat(MANNER_CONF_NAME, &file_stat))
		creation_time_manner_file = 0; // error
	else
		creation_time_manner_file = file_stat.st_mtime;

	fp = fopen(MANNER_CONF_NAME, "r");
	if (fp == NULL) {
		printf("Can't read manner file: %s.\n", MANNER_CONF_NAME);
		return;
	}

	add_word = 0; // Are we in right language? (0: no, 1: yes)
	while(fgets(line, sizeof(line), fp)) { // fgets reads until maximum one less than size and add '\0' -> so, it's not necessary to add -1
		// remove spaces at begining
		while (isspace(line[0]))
			for (i = 0; line[i]; i++)
				line[i] = line[i + 1];
		// is it valid line?
		if ((line[0] == '/' && line[1] == '/') || line[0] == '\0' || line[0] == '\n' || line[0] == '\r')
			continue;
		// remove spaces at end
		while ((i = strlen(line)) > 0 && isspace(line[i - 1]))
			line[i - 1] = '\0';
		// check for configuration
		memset(w1, 0, sizeof(w1));
		if (sscanf(line, "%[^:]:%s", w1, w2) == 2) {
			if (strcasecmp(w1, "language") == 0) {
				if (strcmp(w2, char_name_language) == 0)
					add_word = 1; // Are we in right language? (0: no, 1: yes)
				else
					add_word = 0; // Are we in right language? (0: no, 1: yes)
				continue;
			}
		}
		// check for word/sentence
		if (add_word) {
			if (manner_db == NULL) {
				CALLOC(manner_db, char, MANNER_SIZE);
			} else {
				REALLOC(manner_db, char, (manner_counter + 1) * MANNER_SIZE);
				memset(manner_db + (manner_counter * MANNER_SIZE), 0, sizeof(char) * MANNER_SIZE);
			}
			i = 0;
			while (line[i]) { // to do a correct check in messages, we need lower or upper
				line[i] = tolower((unsigned char)line[i]); // tolower needs unsigned char
				i++;
			}
			strncpy(manner_db + (manner_counter++) * MANNER_SIZE, line, MANNER_SIZE - 1); // -1: NULL
			manner_db[manner_counter * MANNER_SIZE - 1] = '\0';
		}
	}
	fclose(fp);
	printf("DB '" CL_WHITE "%s" CL_RESET "' read ('" CL_WHITE "%d" CL_RESET "' entrie%s).\n", MANNER_CONF_NAME, manner_counter, (manner_counter > 1) ? "s" : "");

	return;
}

// manner system - Check if manner file have been changed
//------------------------------------------
int check_manner_file(int tid, unsigned int tick, int id, int data) {
	struct stat file_stat;
	long new_time;

	// get last modify time/date
	if (stat(MANNER_CONF_NAME, &file_stat))
		new_time = 0; // error
	else
		new_time = file_stat.st_mtime;

	if (new_time != creation_time_manner_file)
		read_manner();

	return 0;
}

// manner system - Check bad words in a sentence
// return values:
//   0: no bad word
//  ~0: bad word
//------------------------------------------
char check_bad_word(const char* sentence, const int sentence_len) {
	char *sentence_to_check, *to_check, *p;
	int i;

	// if no check
	if (!char_name_language_check)
		return 0;

	// if no bad words exist
	if (manner_counter == 0)
		return 0;

	CALLOC(sentence_to_check, char, sentence_len + 1);

	// copy sentence in lower case
	i = 0;
	while (sentence[i]) { // to do a correct check in messages, we need lower or upper
		if (iscntrl((int)sentence[i]))
			sentence_to_check[i] = '_';
		else
			sentence_to_check[i] = tolower((unsigned char)sentence[i]); // tolower needs unsigned char
		i++;
	}

	for (i = 0; i < manner_counter; i++) {
		to_check = sentence_to_check;
		while ((p = strstr(to_check, manner_db + i * MANNER_SIZE)) != NULL) {
			//if (p == sentence_to_check || *p < 'a' || *p > 'z') { // if before word/sentence, it's a void char
			//if (p == sentence_to_check || !isalpha((int)*p)) { // if before word/sentence, it's a void char
			if (p == sentence_to_check || isspace((int)*(p - 1)) || ispunct((int)*(p - 1)) || isdigit((int)*(p - 1))) { // if before word/sentence, it's a void char
				p = p + strlen(manner_db + i * MANNER_SIZE);
				//if (*p == '\0' || !isalpha((int)*p)) { // if after word/sentence, it's a void char
				if (*p == '\0' || isspace((int)*p) || ispunct((int)*p) || isdigit((int)*p)) { // if after word/sentence, it's a void char
					//printf("Bad word: '%s' in %s at %d.\n", manner_db + i * MANNER_SIZE, sentence, (p - sentence) + 1);
					FREE(sentence_to_check);
					return 1; // block
				}
			}
			to_check++; // to check multiple bad words in single sentence
		}
	}

	FREE(sentence_to_check);

	return 0; // don't block
}

// manner system - END
//------------------------------------------

//----------------------------------------------------------------------
// Determine if an account (id) is a GM account
// and returns its level (or 0 if it isn't a GM account or if not found)
//----------------------------------------------------------------------
int isGM(int account_id) {
	int i;

	for(i = 0; i < GM_num; i++)
		if (gm_account[i].account_id == account_id)
			return gm_account[i].level;

	return 0;
}

/*==========================================
 * Search an character id
 *   Return character index in memory or -1 (if not found) or -2 (if found, but not with right case sensitive name))
 *------------------------------------------
 */
int char_nick2idx(char* character_name) {
	int i, quantity, idx;

	quantity = 0;
	idx = -1;
	for(i = 0; i < char_num; i++) {
		// Without case sensitive check (increase the number of similar character names found)
		if (strcasecmp(char_dat[i].name, character_name) == 0) {
			// Strict comparison (if found, we finish the function immediatly with correct value)
			if (strcmp(char_dat[i].name, character_name) == 0)
				return i;
			quantity++;
			idx = i;
		}
	}

	// if no name found
	if (quantity == 0)
		return -1;
// in SQL, all database is not in memory. So, we must search in SQL
#ifndef USE_SQL
	// Here, the exact character name is not found
	// We return the found index of a similar account ONLY if there is 1 similar character
	else if (quantity == 1)
		return idx;
#endif /* not USE_SQL */
	// Exact character name is not found and more than 1 similar characters have been found
	else
		return -2;
}

#ifdef TXT_ONLY
//-------------------------
// quick sorting (by char_id) (by [Yor])
//-------------------------
void tri_rapide_char_id(int tableau[], int premier, int dernier) {
	int temp, vmin, vmax, separateur_de_listes;

	vmin = premier;
	vmax = dernier;
	separateur_de_listes = char_dat[tableau[(premier + dernier) / 2]].char_id;

	do {
		while(char_dat[tableau[vmin]].char_id < separateur_de_listes)
			vmin++;
		while(char_dat[tableau[vmax]].char_id > separateur_de_listes)
			vmax--;

		if (vmin <= vmax) {
			temp = tableau[vmin];
			tableau[vmin++] = tableau[vmax];
			tableau[vmax--] = temp;
		}
	} while(vmin <= vmax);

	if (premier < vmax)
		tri_rapide_char_id(tableau, premier, vmax);
	if (vmin < dernier)
		tri_rapide_char_id(tableau, vmin, dernier);
}

//----------------------------------
// Function to save friends in files
//----------------------------------
void mmo_friend_sync(void) { // only for TXT version
	char line[65536];
	int line_len;
	int i, j, idx;
	int lock;
	FILE *fp;
	int *id;
	struct friend_dat *frds;

	CALLOC(id, int, char_num);

	// Sorting before save (by [Yor])
	for(i = 0; i < char_num; i++)
		id[i] = i;
	if (char_num > 1)
		tri_rapide_char_id(id, 0, char_num - 1);

	// Friends List data save
	if ((fp = lock_fopen(friends_txt, &lock)) == NULL) {
		printf("WARNING: Server can't not save friends.\n");
		char_log("WARNING: Server can't not save friends." RETCODE);
	} else {
		fprintf(fp, "// Friends file" RETCODE
		            "// Structure:" RETCODE
		            "// char_id<tab>friend1_id,friend2_id..." RETCODE
		            RETCODE
		            "// Set if check of 'a friend must be friend of its friend' is already done" RETCODE
		            "friend_of_friend_checked: 1" RETCODE
		            RETCODE);
		line_len = 0;
		for(i = 0; i < char_num; i++) {
			idx = id[i]; // speed up
			frds = &friend_dat[idx];
			for(j = 0; j < frds->friend_num; j++) {
				if (line_len == 0)
					line_len = sprintf(line, "%d\t%d", char_dat[idx].char_id, frds->friends[j].char_id);
				else
					line_len += sprintf(line + line_len, ",%d", frds->friends[j].char_id);
			}
			if (line_len != 0) {
				fprintf(fp, "%s" RETCODE, line);
				line_len = 0;
			}
		}
		lock_fclose(fp, friends_txt, &lock);
		friends_txt_flag = 0; // must we save friends file or not? (0: not, x: yes)
	}

	FREE(id);

	return;
}
#endif // TXT_ONLY

//-------------------------------------------------
// Function to create the character line (for save)
//-------------------------------------------------
inline int mmo_char_tostr(char *str, struct mmo_charstatus *p, int idx) {
	int i;
	char *str_p = str;
	struct item *item_p, *item_end;
	struct global_reg *reg;

	// on multi-map server, sometimes it's posssible that last_point become void. (reason???) We check that to not lost character at restart.
	if (p->last_point.map[0] == '\0') {
		memset(p->last_point.map, 0, sizeof(p->last_point.map));
		strncpy(p->last_point.map, "prontera.gat", 16); // 17 - NULL
		p->last_point.x = 273;
		p->last_point.y = 354;
	}

	str_p += int2string(str_p, p->char_id);
	*(str_p++) = '\t';

	str_p += int2string(str_p, p->account_id);
	*(str_p++) = ',';
	str_p += int2string(str_p, p->char_num);
	*(str_p++) = '\t';

	strcpy(str_p, p->name);
	str_p += strlen(p->name);
	*(str_p++) = '\t';

	str_p += int2string(str_p, p->class);
	*(str_p++) = ',';
	str_p += int2string(str_p, p->base_level);
	*(str_p++) = ',';
	str_p += int2string(str_p, p->job_level);
	*(str_p++) = '\t';

	str_p += int2string(str_p, p->base_exp);
	*(str_p++) = ',';
	str_p += int2string(str_p, p->job_exp);
	*(str_p++) = ',';
	str_p += int2string(str_p, p->zeny);
	*(str_p++) = '\t';

	str_p += int2string(str_p, p->hp);
	*(str_p++) = ',';
	str_p += int2string(str_p, p->max_hp);
	*(str_p++) = ',';
	str_p += int2string(str_p, p->sp);
	*(str_p++) = ',';
	str_p += int2string(str_p, p->max_sp);
	*(str_p++) = '\t';

	str_p += int2string(str_p, p->str);
	*(str_p++) = ',';
	str_p += int2string(str_p, p->agi);
	*(str_p++) = ',';
	str_p += int2string(str_p, p->vit);
	*(str_p++) = ',';
	str_p += int2string(str_p, p->int_);
	*(str_p++) = ',';
	str_p += int2string(str_p, p->dex);
	*(str_p++) = ',';
	str_p += int2string(str_p, p->luk);
	*(str_p++) = '\t';

	str_p += int2string(str_p, p->status_point);
	*(str_p++) = ',';
	str_p += int2string(str_p, p->skill_point);
	*(str_p++) = '\t';

	str_p += int2string(str_p, p->option);
	*(str_p++) = ',';
	str_p += int2string(str_p, p->karma);
	*(str_p++) = ',';
	str_p += int2string(str_p, p->manner);
	*(str_p++) = '\t';

	str_p += int2string(str_p, p->party_id);
	*(str_p++) = ',';
	str_p += int2string(str_p, p->guild_id);
	*(str_p++) = ',';
	str_p += int2string(str_p, p->pet_id);
	*(str_p++) = '\t';

	str_p += int2string(str_p, p->hair);
	*(str_p++) = ',';
	str_p += int2string(str_p, p->hair_color);
	*(str_p++) = ',';
	str_p += int2string(str_p, p->clothes_color);
	*(str_p++) = '\t';

	str_p += int2string(str_p, p->weapon);
	*(str_p++) = ',';
	str_p += int2string(str_p, p->shield);
	*(str_p++) = ',';
	str_p += int2string(str_p, p->head_top);
	*(str_p++) = ',';
	str_p += int2string(str_p, p->head_mid);
	*(str_p++) = ',';
	str_p += int2string(str_p, p->head_bottom);
	*(str_p++) = '\t';

	strcpy(str_p, p->last_point.map);
	str_p += strlen(p->last_point.map);
	*(str_p++) = ',';
	str_p += int2string(str_p, p->last_point.x);
	*(str_p++) = ',';
	str_p += int2string(str_p, p->last_point.y);
	*(str_p++) = '\t';

	strcpy(str_p, p->save_point.map);
	str_p += strlen(p->save_point.map);
	*(str_p++) = ',';
	str_p += int2string(str_p, p->save_point.x);
	*(str_p++) = ',';
	str_p += int2string(str_p, p->save_point.y);
	*(str_p++) = ',';
	str_p += int2string(str_p, p->partner_id);
	*(str_p++) = ',';
	str_p += int2string(str_p, p->die_counter);
	*(str_p++) = '\t';

	
	for(i = 0; i < MAX_PORTAL_MEMO; i++)
		if (p->memo_point[i].map[0]) {
			strcpy(str_p, p->memo_point[i].map);
			str_p += strlen(p->memo_point[i].map);
			*(str_p++) = ',';
			//str_p += sprintf(str_p, "%s,", p->memo_point[i].map);
			str_p += int2string(str_p, p->memo_point[i].x);
			*(str_p++) = ',';
			str_p += int2string(str_p, p->memo_point[i].y);
		}
	*(str_p++) = '\t';

	item_end = &p->inventory[MAX_INVENTORY];
	for(item_p = &p->inventory[0]; item_p < item_end; item_p++) {
		if (item_p->nameid != 0) {
			str_p += int2string(str_p, item_p->id);
			*(str_p++) = ',';
			str_p += int2string(str_p, item_p->nameid);
			*(str_p++) = ',';
			str_p += int2string(str_p, item_p->amount);
			*(str_p++) = ',';
			str_p += int2string(str_p, item_p->equip);
			*(str_p++) = ',';
			str_p += int2string(str_p, item_p->identify);
			*(str_p++) = ',';
			str_p += int2string(str_p, item_p->refine);
			*(str_p++) = ',';
			str_p += int2string(str_p, item_p->attribute);
			*(str_p++) = ',';
			str_p += int2string(str_p, item_p->card[0]);
			*(str_p++) = ',';
			str_p += int2string(str_p, item_p->card[1]);
			*(str_p++) = ',';
			str_p += int2string(str_p, item_p->card[2]);
			*(str_p++) = ',';
			str_p += int2string(str_p, item_p->card[3]);
			*(str_p++) = ' ';
		}
	}
	*(str_p++) = '\t';

	item_end = &p->cart[MAX_CART];
	for(item_p = &p->cart[0]; item_p < item_end; item_p++) {
		if (item_p->nameid != 0) {
			str_p += int2string(str_p, item_p->id);
			*(str_p++) = ',';
			str_p += int2string(str_p, item_p->nameid);
			*(str_p++) = ',';
			str_p += int2string(str_p, item_p->amount);
			*(str_p++) = ',';
			str_p += int2string(str_p, item_p->equip);
			*(str_p++) = ',';
			str_p += int2string(str_p, item_p->identify);
			*(str_p++) = ',';
			str_p += int2string(str_p, item_p->refine);
			*(str_p++) = ',';
			str_p += int2string(str_p, item_p->attribute);
			*(str_p++) = ',';
			str_p += int2string(str_p, item_p->card[0]);
			*(str_p++) = ',';
			str_p += int2string(str_p, item_p->card[1]);
			*(str_p++) = ',';
			str_p += int2string(str_p, item_p->card[2]);
			*(str_p++) = ',';
			str_p += int2string(str_p, item_p->card[3]);
			*(str_p++) = ' ';
		}
	}
	*(str_p++) = '\t';

	for(i = 0; i < MAX_SKILL; i++) {
		if (p->skill[i].lv != 0 && p->skill[i].id == 0)
			p->skill[i].id = i; // Fix skill tree
		if (p->skill[i].id && p->skill[i].flag != 1 && p->skill[i].flag != 13) { // flag: 0 (normal), 1 (only card), 2-12 (card and skill (skill level +2)), 13 (cloneskill)
			str_p += int2string(str_p, p->skill[i].id);
			*(str_p++) = ',';
			str_p += int2string(str_p, (p->skill[i].flag == 0) ? p->skill[i].lv : p->skill[i].flag - 2);
			*(str_p++) = ' ';
		}
	}
	*(str_p++) = '\t';

	reg = global_reg_db[idx].global_reg;
	for(i = 0; i < global_reg_db[idx].global_reg_num; i++) {
		strncpy(str_p, reg[i].str, 32);
		str_p += strlen(reg[i].str);
		*(str_p++) = ',';
		str_p += int2string(str_p, reg[i].value);
		*(str_p++) = ' ';
	}
	*(str_p++) = '\t';

	*str_p = '\0';

	return 0;
}

//-------------------------------------------------------------------------
// Function to set the character from the line (at read of characters file)
//-------------------------------------------------------------------------
static inline int mmo_char_fromstr(char *str, struct mmo_charstatus *p, int idx) {
	int tmp_int[256];
	char tmp_str[256];
	struct global_reg reg[GLOBAL_REG_NUM];
	int set, next, len, i, j;

	// initilialise character
	memset(p, 0, sizeof(struct mmo_charstatus));
	// If its not the new char structure ver 4
	if ((set = sscanf(str, "%d\t%d,%d\t%[^\t]\t%d,%d,%d\t%d,%d,%d\t%d,%d,%d,%d\t%d,%d,%d,%d,%d,%d\t%d,%d"
	   "\t%d,%d,%d\t%d,%d,%d\t%d,%d,%d\t%d,%d,%d,%d,%d"
	   "\t%[^,],%d,%d\t%[^,],%d,%d,%d,%d%n",
	   &tmp_int[0], &tmp_int[1], &tmp_int[2], p->name, //
	   &tmp_int[3], &tmp_int[4], &tmp_int[5],
	   &tmp_int[6], &tmp_int[7], &tmp_int[8],
	   &tmp_int[9], &tmp_int[10], &tmp_int[11], &tmp_int[12],
	   &tmp_int[13], &tmp_int[14], &tmp_int[15], &tmp_int[16], &tmp_int[17], &tmp_int[18],
	   &tmp_int[19], &tmp_int[20],
	   &tmp_int[21], &tmp_int[22], &tmp_int[23], //
	   &tmp_int[24], &tmp_int[25], &tmp_int[26],
	   &tmp_int[27], &tmp_int[28], &tmp_int[29],
	   &tmp_int[30], &tmp_int[31], &tmp_int[32], &tmp_int[33], &tmp_int[34],
	   p->last_point.map, &tmp_int[35], &tmp_int[36], //
	   p->save_point.map, &tmp_int[37], &tmp_int[38], &tmp_int[39], &tmp_int[40], &next)) != 44) {
		tmp_int[40] = 0; // die_counter
	// If it's not char structure of version 1008 and after
	if ((set = sscanf(str, "%d\t%d,%d\t%[^\t]\t%d,%d,%d\t%d,%d,%d\t%d,%d,%d,%d\t%d,%d,%d,%d,%d,%d\t%d,%d"
	   "\t%d,%d,%d\t%d,%d,%d\t%d,%d,%d\t%d,%d,%d,%d,%d"
	   "\t%[^,],%d,%d\t%[^,],%d,%d,%d%n",
	   &tmp_int[0], &tmp_int[1], &tmp_int[2], p->name, //
	   &tmp_int[3], &tmp_int[4], &tmp_int[5],
	   &tmp_int[6], &tmp_int[7], &tmp_int[8],
	   &tmp_int[9], &tmp_int[10], &tmp_int[11], &tmp_int[12],
	   &tmp_int[13], &tmp_int[14], &tmp_int[15], &tmp_int[16], &tmp_int[17], &tmp_int[18],
	   &tmp_int[19], &tmp_int[20],
	   &tmp_int[21], &tmp_int[22], &tmp_int[23], //
	   &tmp_int[24], &tmp_int[25], &tmp_int[26],
	   &tmp_int[27], &tmp_int[28], &tmp_int[29],
	   &tmp_int[30], &tmp_int[31], &tmp_int[32], &tmp_int[33], &tmp_int[34],
	   p->last_point.map, &tmp_int[35], &tmp_int[36], //
	   p->save_point.map, &tmp_int[37], &tmp_int[38], &tmp_int[39], &next)) != 43) {
		tmp_int[39] = 0; // partner id
		// If not char structure from version 384 to 1007
		if ((set = sscanf(str, "%d\t%d,%d\t%[^\t]\t%d,%d,%d\t%d,%d,%d\t%d,%d,%d,%d\t%d,%d,%d,%d,%d,%d\t%d,%d"
		   "\t%d,%d,%d\t%d,%d,%d\t%d,%d,%d\t%d,%d,%d,%d,%d"
		   "\t%[^,],%d,%d\t%[^,],%d,%d%n",
		   &tmp_int[0], &tmp_int[1], &tmp_int[2], p->name, //
		   &tmp_int[3], &tmp_int[4], &tmp_int[5],
		   &tmp_int[6], &tmp_int[7], &tmp_int[8],
		   &tmp_int[9], &tmp_int[10], &tmp_int[11], &tmp_int[12],
		   &tmp_int[13], &tmp_int[14], &tmp_int[15], &tmp_int[16], &tmp_int[17], &tmp_int[18],
		   &tmp_int[19], &tmp_int[20],
		   &tmp_int[21], &tmp_int[22], &tmp_int[23], //
		   &tmp_int[24], &tmp_int[25], &tmp_int[26],
		   &tmp_int[27], &tmp_int[28], &tmp_int[29],
		   &tmp_int[30], &tmp_int[31], &tmp_int[32], &tmp_int[33], &tmp_int[34],
		   p->last_point.map, &tmp_int[35], &tmp_int[36], //
		   p->save_point.map, &tmp_int[37], &tmp_int[38], &next)) != 42) {
			// It's char structure of a version before 384
			tmp_int[26] = 0; // pet id
			set = sscanf(str, "%d\t%d,%d\t%[^\t]\t%d,%d,%d\t%d,%d,%d\t%d,%d,%d,%d\t%d,%d,%d,%d,%d,%d\t%d,%d"
			      "\t%d,%d,%d\t%d,%d\t%d,%d,%d\t%d,%d,%d,%d,%d"
			      "\t%[^,],%d,%d\t%[^,],%d,%d%n",
			      &tmp_int[0], &tmp_int[1], &tmp_int[2], p->name, //
			      &tmp_int[3], &tmp_int[4], &tmp_int[5],
			      &tmp_int[6], &tmp_int[7], &tmp_int[8],
			      &tmp_int[9], &tmp_int[10], &tmp_int[11], &tmp_int[12],
			      &tmp_int[13], &tmp_int[14], &tmp_int[15], &tmp_int[16], &tmp_int[17], &tmp_int[18],
			      &tmp_int[19], &tmp_int[20],
			      &tmp_int[21], &tmp_int[22], &tmp_int[23], //
			      &tmp_int[24], &tmp_int[25], //
			      &tmp_int[27], &tmp_int[28], &tmp_int[29],
			      &tmp_int[30], &tmp_int[31], &tmp_int[32], &tmp_int[33], &tmp_int[34],
			      p->last_point.map, &tmp_int[35], &tmp_int[36], //
			      p->save_point.map, &tmp_int[37], &tmp_int[38], &next);
			set += 2;
			//printf("char: old char data ver.1\n");
		// Char structure of version 1007 or older
		} else {
			set++;
			//printf("char: old char data ver.2\n");
		}
	// Char structure of version 1008+
	} else {
		set++;
		//printf("char: new char data ver.3\n");
	}
	// Char structture of version 1363+
	} else {
		//printf("char: new char data ver.4\n");
	}
	if (set != 44)
		return 0;

	p->char_id = tmp_int[0];
	p->account_id = tmp_int[1];
	p->char_num = tmp_int[2];
	p->class = tmp_int[3];
	p->base_level = tmp_int[4];
	p->job_level = tmp_int[5];
	p->base_exp = tmp_int[6];
	p->job_exp = tmp_int[7];
	p->zeny = tmp_int[8];
	p->hp = tmp_int[9];
	p->max_hp = tmp_int[10];
	p->sp = tmp_int[11];
	p->max_sp = tmp_int[12];
	p->str = tmp_int[13];
	p->agi = tmp_int[14];
	p->vit = tmp_int[15];
	p->int_ = tmp_int[16];
	p->dex = tmp_int[17];
	p->luk = tmp_int[18];
	p->status_point = tmp_int[19];
	p->skill_point = tmp_int[20];
	p->option = tmp_int[21];
	p->karma = tmp_int[22];
	p->manner = tmp_int[23];
	p->party_id = tmp_int[24];
	p->guild_id = tmp_int[25];
	p->pet_id = tmp_int[26];
	p->hair = tmp_int[27];
	if (p->hair > max_hair_style) {
		p->hair = max_hair_style;
	}
	p->hair_color = tmp_int[28];
	if (p->hair_color > max_hair_color) {
		p->hair_color = max_hair_color;
	}
	p->clothes_color = tmp_int[29];
	p->weapon = tmp_int[30];
	p->shield = tmp_int[31];
	p->head_top = tmp_int[32];
	p->head_mid = tmp_int[33];
	p->head_bottom = tmp_int[34];
	p->last_point.x = tmp_int[35];
	p->last_point.y = tmp_int[36];
	p->save_point.x = tmp_int[37];
	p->save_point.y = tmp_int[38];
	p->partner_id = tmp_int[39];
	p->die_counter = tmp_int[40];
	
	// Some checks
	for(i = 0; i < char_num; i++) {
		if (char_dat[i].char_id == p->char_id) {
			printf(CL_RED "mmo_auth_init: ******Error: a character has an identical id to another.\n");
			printf(       "               character id #%d -> new character not read.\n", p->char_id);
			printf(       "               Character saved in log file." CL_RESET "\n");
			return -1;
		} else if (strcmp(char_dat[i].name, p->name) == 0) {
			printf(CL_RED "mmo_auth_init: ******Error: character name already exists.\n");
			printf(       "               character name '%s' -> new character not read.\n", p->name);
			printf(       "               Character saved in log file." CL_RESET "\n");
			return -2;
		}
	}

	if (str[next] == '\n' || str[next] == '\r')
		return 1;	// �V�K�f�[�^

	next++;

	// read memo data
	i = 0;
	while(str[next] && str[next] != '\t' && i < MAX_PORTAL_MEMO) {
		if (sscanf(str + next, "%[^,],%d,%d%n", p->memo_point[i].map, &tmp_int[0], &tmp_int[1], &len) != 3)
			return -3;
		p->memo_point[i].x = tmp_int[0];
		p->memo_point[i].y = tmp_int[1];
		i++;
		next += len;
		if (str[next] == ' ')
			next++;
	}

	next++;

	// read inventory
	for(i = 0; str[next] && str[next] != '\t'; i++) {
		if (sscanf(str + next, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d%n",
		    &tmp_int[0], &tmp_int[1], &tmp_int[2], &tmp_int[3],
		    &tmp_int[4], &tmp_int[5], &tmp_int[6],
		    &tmp_int[7], &tmp_int[8], &tmp_int[9], &tmp_int[10], &len) != 11) {
			// invalid structure
			return -4;
		}
		p->inventory[i].id = tmp_int[0];
		p->inventory[i].nameid = tmp_int[1];
		p->inventory[i].amount = tmp_int[2];
		p->inventory[i].equip = tmp_int[3];
		p->inventory[i].identify = tmp_int[4];
		p->inventory[i].refine = tmp_int[5];
		p->inventory[i].attribute = tmp_int[6];
		p->inventory[i].card[0] = tmp_int[7];
		p->inventory[i].card[1] = tmp_int[8];
		p->inventory[i].card[2] = tmp_int[9];
		p->inventory[i].card[3] = tmp_int[10];
		if (p->inventory[i].refine > 10)
			p->inventory[i].refine = 10;
		else if (p->inventory[i].refine < 0)
			p->inventory[i].refine = 0;
		next += len;
		if (str[next] == ' ')
			next++;
	}

	next++;

	// read cart.
	for(i = 0; str[next] && str[next] != '\t'; i++) {
		if (sscanf(str + next, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d%n",
		    &tmp_int[0], &tmp_int[1], &tmp_int[2], &tmp_int[3],
		    &tmp_int[4], &tmp_int[5], &tmp_int[6],
		    &tmp_int[7], &tmp_int[8], &tmp_int[9], &tmp_int[10], &len) != 11) {
			// invalid structure
			return -5;
		}
		p->cart[i].id = tmp_int[0];
		p->cart[i].nameid = tmp_int[1];
		p->cart[i].amount = tmp_int[2];
		p->cart[i].equip = tmp_int[3];
		p->cart[i].identify = tmp_int[4];
		p->cart[i].refine = tmp_int[5];
		p->cart[i].attribute = tmp_int[6];
		p->cart[i].card[0] = tmp_int[7];
		p->cart[i].card[1] = tmp_int[8];
		p->cart[i].card[2] = tmp_int[9];
		p->cart[i].card[3] = tmp_int[10];
		if (p->cart[i].refine > 10)
			p->cart[i].refine = 10;
		else if (p->cart[i].refine < 0)
			p->cart[i].refine = 0;
		next += len;
		if (str[next] == ' ')
			next++;
	}

	next++;

	// read skill
	while(str[next] && str[next] != '\t') {
		if (sscanf(str + next, "%d,%d%n", &i, &tmp_int[1], &len) != 2)
			return -6;
		if (i > 0 && i < MAX_SKILL && tmp_int[1] > 0) { // no skill 0 -> >0
			p->skill[i].id = i;
			p->skill[i].lv = tmp_int[1];
		}
		next += len;
		if (str[next] == ' ')
			next++;
	}

	next++;

	i = 0;
	while(str[next] && str[next] != '\t' && str[next] != '\n' && str[next] != '\r' && i < GLOBAL_REG_NUM) {
		memset(tmp_str, 0, sizeof(tmp_str));
		if (sscanf(str + next, "%[^,],%d%n", tmp_str, &tmp_int[0], &len) != 2) {
			// because some scripts are not correct, the str can be "". So, we must check that.
			// If it's, we must not refuse the character, but just this REG value.
			// Character line will have something like: nov_2nd_cos,9 ,9 nov_1_2_cos_c,1 (here, ,9 is not good)
			if (str[next] != ',' || sscanf(str + next, ",%d%n", &tmp_int[0], &len) != 1)
				return -7;
		}
		if (tmp_str[0] && tmp_int[0] != 0) {
			// check if global_reg already exists
			for (j = 0; j < i; j++)
				if (strcmp(reg[j].str, tmp_str) == 0)
					break;
			// if not found
			if (j == i) {
				strncpy(reg[i].str, tmp_str, 32);
				reg[i].str[32] = '\0';
				reg[i].value = tmp_int[0];
//				printf("reg #%d: %s=%d\n", i,reg[i].str, reg[i].value);
				i++;
			}
		}
		next += len;
		if (str[next] == ' ')
			next++;
	}
	if (i > 0) {
		global_reg_db[idx].global_reg_num = i;
		MALLOC(global_reg_db[idx].global_reg, struct global_reg, i);
		memcpy(global_reg_db[idx].global_reg, reg, sizeof(struct global_reg) * i);
	}

	return 1;
}

#ifdef TXT_ONLY
//-----------------------------
// Function to read friend list
//-----------------------------
int parse_friend_txt() {
	char line[1024];
	int i, j, k, l, char_id;
	int friend_id;
	char temp[1024]; // 24 + NULL - but same size of line -> 1024
	int next, len, counter, already_checked;
	FILE *fp;
	struct friend_dat *frds;
	struct friend *tmp_friends;

	// Open the file
	if ((fp = fopen(friends_txt, "r")) == NULL) {
		printf("Friends file not found: %s.\n", friends_txt);
		char_log("Friends file not found: %s." RETCODE, friends_txt);
		return 0;
	}

	printf("Reading friends file '%s'...\r", friends_txt);
	fflush(stdout);

	// create temp structure
	CALLOC(tmp_friends, struct friend, MAX_FRIENDS);

	counter = 0;
	already_checked = 0; // to know if friend of friend have been already checked
	// Look for the ID
	while(fgets(line, sizeof(line), fp)) { // fgets reads until maximum one less than size and add '\0' -> so, it's not necessary to add -1
		if ((line[0] == '/' && line[1] == '/') || line[0] == '\0' || line[0] == '\n' || line[0] == '\r')
			continue;
		// remove carriage return if exist
		while(line[0] != '\0' && (line[(i = strlen(line) - 1)] == '\n' || line[i] == '\r'))
			line[i] = '\0';

		// Search if it's a configuration value
		if (already_checked == 0 && sscanf(line, "%[^:]:%n", temp, &next) == 1 && strcasecmp(temp, "friend_of_friend_checked") == 0) {
			if (atoi(line + next) == 1)
				already_checked = 1; // to know if friend of friend have been already checked
		} else if (sscanf(line, "%d%n", &char_id, &next) == 1) { // get char_id
			for(i = 0; i < char_num; i++) {
				if (char_dat[i].char_id == char_id) {
					// Fill in the list
					FREE(friend_dat[i].friends);
					// check old version
					if (line[next] != '\t') {
						// structure: char_id,friend_char_id0,friend_name0,friend_char_id1,friend_name1
						j = 0;
						while(line[next] && j < MAX_FRIENDS) {
							if (sscanf(line + next, ",%d,%[^,]%n", &friend_id, temp, &len) != 2)
								break;
							// checking if not already registered
							for (k = 0; k < j; k++)
								if (tmp_friends[k].char_id == friend_id)
									break;
							// if not already regitered (and not yourself)
							if (k == j && char_id != friend_id) {
								// searching if friend always exists in database
								for (k = 0; k < char_num; k++)
									if (char_dat[k].char_id == friend_id) {
										tmp_friends[j].account_id = char_dat[k].account_id;
										tmp_friends[j].char_id = friend_id;
										strncpy(tmp_friends[j].name, char_dat[k].name, 24); // to be sure of the name, use actual name of the player
										j++;
										counter++;
										if (counter % 20 == 0) {
											printf("Reading friends file '%s'... %d\r", friends_txt, counter);
											fflush(stdout);
										}
										break;
									}
							} else {
								// set flag of friends file
								friends_txt_flag = 1; // must we save friends file or not? (0: not, x: yes)
							}
							next += len;
						}
					} else {
						// structure: char_id'TAB'friend_char_id0,friend_char_id1
						j = 0;
						while(line[next] && j < MAX_FRIENDS) {
							next++; // remove the 'TAB' or ','
							if (sscanf(line + next, "%[^,]%n", temp, &len) != 1)
								break;
							friend_id = atoi(temp);
							// checking if not already registered
							for (k = 0; k < j; k++)
								if (tmp_friends[k].char_id == friend_id)
									break;
							// if not already regitered (and not yourself)
							if (k == j && char_id != friend_id) {
								// searching if friend always exists in database
								for (k = 0; k < char_num; k++)
									if (char_dat[k].char_id == friend_id) {
										tmp_friends[j].account_id = char_dat[k].account_id;
										tmp_friends[j].char_id = friend_id;
										strncpy(tmp_friends[j].name, char_dat[k].name, 24);
										j++;
										counter++;
										if (counter % 20 == 0) {
											printf("Reading friends file '%s'... %d\r", friends_txt, counter);
											fflush(stdout);
										}
										break;
									}
							} else {
								// set flag of friends file
								friends_txt_flag = 1; // must we save friends file or not? (0: not, x: yes)
							}
							next += len;
						}
					}
					if (j != 0) {
						MALLOC(friend_dat[i].friends, struct friend, j);
						memcpy(friend_dat[i].friends, tmp_friends, sizeof(struct friend) * j);
					}
					friend_dat[i].friend_num = j;
					break;
				}
			}
		}
	}
	fclose(fp);

	// Free temp structure
	FREE(tmp_friends);

	printf("Reading friends file '%s'... done.      \n", friends_txt);

	if (already_checked == 0) { // to know if friend of friend have been already checked
		printf("Checking if a friend is the friend of its friend too.\n");
		// check if a character with a friend is also friend for its friend
		for (i = 0; i < char_num; i++) {
			char_id = char_dat[i].char_id;
			// if character has some friends do loop of friends
			for (j = 0; j < friend_dat[i].friend_num; j++) {
				friend_id = friend_dat[i].friends[j].char_id;
				// search friend in list of characters
				for (k = 0; k < char_num; k++) {
					if (char_dat[k].char_id == friend_id) {
						frds = &friend_dat[k];
						// check all friends of this character
						for (l = 0; l < frds->friend_num; l++) {
							if (frds->friends[l].char_id == char_id)
								break;
						}
						// if not found in the list, add it
						if (l == frds->friend_num && l < MAX_FRIENDS) {
							if (frds->friend_num == 0) {
								CALLOC(frds->friends, struct friend, 1);
							} else {
								REALLOC(frds->friends, struct friend, frds->friend_num + 1);
							}
							frds->friends[frds->friend_num].account_id = char_dat[i].account_id;
							frds->friends[frds->friend_num].char_id = char_id;
							strncpy(frds->friends[frds->friend_num].name, char_dat[i].name, 24);
							frds->friend_num++;
							// set flag of friends file
							friends_txt_flag = 1; // must we save friends file or not? (0: not, x: yes)
						}
						break;
					}
				}
			}
		}
	}

	return 0;
}
#endif // TXT_ONLY

/*==========================================
 * Send friends list to map-server [Yor]
 *------------------------------------------
 */
inline void chrif_send_friends(int idx, int char_fd) {
	int i, len;

#ifdef TXT_ONLY
	struct friend_dat *frds;

	frds = &friend_dat[idx];
	if (frds->friend_num > 0) {
		len = 10;
		for(i = 0; i < frds->friend_num; i++) {
			WPACKETL(len    ) = frds->friends[i].account_id;
			WPACKETL(len + 4) = frds->friends[i].char_id;
			strncpy(WPACKETP(len + 8), frds->friends[i].name, 24);
			len += 32;
		}
		WPACKETW(0) = 0x2b25; // 0x2b25 <size>.W <char_id>.L <friend_num>.W {<friend_account_id>.L <friend_char_id>.L <friend_name>.24B}.x
		WPACKETW(2) = len; // size
		WPACKETL(4) = char_dat[idx].char_id;
		WPACKETW(8) = frds->friend_num;
		SENDPACKET(char_fd, len);
	}
#endif // TXT_ONLY

#ifdef USE_SQL
	//`friends` (`char_id`, `friend_id`)
	sql_request("SELECT SQL_SMALL_RESULT b.`account_id`, b.`char_id`, b.`name` FROM `%s` AS a LEFT JOIN `%s` AS b ON a.`friend_id` = b.`char_id` WHERE a.`char_id`='%d'",
	            friends_db, char_db, char_dat[idx].char_id);
	i = 0;
	len = 10;
	while(sql_get_row() && i < MAX_FRIENDS) {
		if (sql_get_integer(0) > 0 && sql_get_integer(1) > 0 && sql_get_string(2) != NULL && sql_get_string(2)[0]) {
			WPACKETL(len    ) = sql_get_integer(0);
			WPACKETL(len + 4) = sql_get_integer(1);
			strncpy(WPACKETP(len + 8), sql_get_string(2), 24);
			i++;
			len += 32;
		}
	}
	if (i > 0) {
		WPACKETW(0) = 0x2b25; // 0x2b25 <size>.W <char_id>.L <friend_num>.W {<friend_account_id>.L <friend_char_id>.L <friend_name>.24B}.x
		WPACKETW(2) = len; // size
		WPACKETL(4) = char_dat[idx].char_id;
		WPACKETW(8) = i;
		SENDPACKET(char_fd, len);
	}
#endif // USE_SQL

	return;
}

/*====================================
 * Send global reg to map-server [Yor]
 *----------------=-------------------
 */
inline void chrif_send_global_reg(int idx, int char_fd) { // 0x2b1a <packet_len>.w <account_id>.L account_reg.structure.*B
	int size;

	if (global_reg_db[idx].global_reg_num > 0) {
		size = sizeof(struct global_reg) * global_reg_db[idx].global_reg_num;
		WPACKETW(0) = 0x2b1a; // 0x2b1a <packet_len>.w <account_id>.L account_reg.structure.*B
		WPACKETW(2) = 8 + size;
		WPACKETL(4) = char_dat[idx].account_id;
		memcpy(WPACKETP(8), global_reg_db[idx].global_reg, size);
		SENDPACKET(char_fd, 8 + size);
	}

	return;
}

//-------------------------------------
// Function to read characters database
//-------------------------------------
static inline void mmo_char_init(void) {
#ifdef USE_SQL
	sql_request("SELECT count(1) FROM `%s`", char_db);
	if (sql_get_row()) {
		char_num = sql_get_integer(0);

		if (char_num != 0) {
			sql_request("SELECT max(`char_id`) FROM `%s`", char_db);
			if (sql_get_row()) {
				if (char_id_count <= sql_get_integer(0))
					char_id_count = sql_get_integer(0) + 1;
			}
		}
	}

	if (char_num == 0) {
		printf("No character found in database.\n");
		char_log("No character found in database." RETCODE);
	} else if (char_num == 1) {
		printf("1 character found in database.\n");
		char_log("1 character found in database." RETCODE);
	} else {
		printf("%d characters found in database.\n", char_num);
		char_log("%d characters found in database." RETCODE, char_num);
	}

	sql_request("UPDATE `%s` SET `online`=0", char_db);
#endif // USE_SQL

#ifdef TXT_ONLY
	char line[65536];
	int i, j;
	int ret, line_count;
	FILE *fp;
#endif // TXT_ONLY

	char_max = 256;
	CALLOC(char_dat, struct mmo_charstatus, 256);
	CALLOC(global_reg_db, struct global_reg_db, 256);
#ifdef TXT_ONLY
	CALLOC(friend_dat, struct friend_dat, 256);
#endif // TXT_ONLY
	CALLOC(char_dat_timer, time_t, 256);
	CALLOC(online_chars, signed char, 256);

	char_num = 0; // TXT: init / SQL: used before, so init it here

#ifdef TXT_ONLY
	if ((fp = fopen(char_txt, "r")) == NULL) {
		printf("Characters file not found: %s.\n", char_txt);
		char_log("Characters file not found: %s. Id for the next created character: %d." RETCODE, char_txt, char_id_count);
		return;
	}

	line_count = 0;
	while(fgets(line, sizeof(line), fp)) { // fgets reads until maximum one less than size and add '\0' -> so, it's not necessary to add -1
		line_count++;
		if ((line[0] == '/' && line[1] == '/') || line[0] == '\0' || line[0] == '\n' || line[0] == '\r')
			continue;
		// it's not necessary to remove 'carriage return ('\n' or '\r')

		j = 0;
		if (sscanf(line, "%d\t%%newid%%%n", &i, &j) == 1 && j > 0) {
			if (char_id_count < i)
				char_id_count = i;
			continue;
		}

		if (char_num >= char_max) {
			char_max += 256;
			REALLOC(char_dat, struct mmo_charstatus, char_max);
			REALLOC(global_reg_db, struct global_reg_db, char_max);
			REALLOC(friend_dat, struct friend_dat, char_max);
			REALLOC(char_dat_timer, time_t, char_max);
			REALLOC(online_chars, signed char, char_max);
			for(i = char_max - 256; i < char_max; i++) {
				global_reg_db[i].global_reg_num = 0;
				global_reg_db[i].global_reg = NULL;
				friend_dat[i].friend_num = 0;
				friend_dat[i].friends = NULL;
				online_chars[i] = 0;
				char_dat_timer[i] = 0;
			}
		}

		if (char_num % 20 == 19) {
			printf("Reading character #%d...\r", char_num + 1);
			fflush(stdout);
		}
		ret = mmo_char_fromstr(line, &char_dat[char_num], char_num);

		if (ret > 0) { // negative value or zero for errors
			if (strcasecmp(wisp_server_name, char_dat[char_num].name) == 0) {
				for (j = 0; j < global_reg_db[char_num].global_reg_num; j++)
					if (strcmp(global_reg_db[char_num].global_reg[j].str, "Server_name") == 0)
						break;
				if (j == global_reg_db[char_num].global_reg_num) {
					printf("mmo_auth_init: ******WARNING: character name has wisp server name.\n");
					printf("               Character name '%s' = wisp server name '%s'.\n", char_dat[char_num].name, wisp_server_name);
					printf("               Character read. Suggestion: change the wisp server name.\n");
					char_log("mmo_auth_init: ******WARNING: character name has wisp server name: Character name '%s' = wisp server name '%s'." RETCODE,
					          char_dat[char_num].name, wisp_server_name);
				}
			}
			if (char_id_count <= char_dat[char_num].char_id)
				char_id_count = char_dat[char_num].char_id + 1;
			char_num++;
		} else {
			memset(&char_dat[char_num], 0, sizeof(struct mmo_charstatus));
			// don't re-init global_reg_db[char_num], because it's always read if player datas are correct
			printf("mmo_char_init: in characters file, unable to read the line #%d.\n", line_count);
			printf("               -> Character saved in log file.\n");
			switch (ret) {
			case -1:
				char_log("Duplicate character id in the next character line (character not read):" RETCODE);
				break;
			case -2:
				char_log("Duplicate character name in the next character line (character not read):" RETCODE);
				break;
			case -3:
				char_log("Invalid memo point structure in the next character line (character not read):" RETCODE);
				break;
			case -4:
				char_log("Invalid inventory item structure in the next character line (character not read):" RETCODE);
				break;
			case -5:
				char_log("Invalid cart item structure in the next character line (character not read):" RETCODE);
				break;
			case -6:
				char_log("Invalid skill structure in the next character line (character not read):" RETCODE);
				break;
			case -7:
				char_log("Invalid register structure in the next character line (character not read):" RETCODE);
				break;
			default: // 0
				char_log("Unabled to get a character in the next line - Basic structure of line (before inventory) is incorrect (character not read):" RETCODE);
				break;
			}
			char_log("%s", line);
		}
	}
	fclose(fp);

	if (char_num == 0) {
		printf("No character found in %s.\n", char_txt);
		char_log("No character found in %s." RETCODE, char_txt);
	} else if (char_num == 1) {
		printf("1 character read in %s.\n", char_txt);
		char_log("1 character read in %s." RETCODE, char_txt);
	} else {
		printf("%d characters read in %s.\n", char_num, char_txt);
		char_log("%d characters read in %s." RETCODE, char_num, char_txt);
	}

	// Initialize friends list
	parse_friend_txt();
#endif // TXT_ONLY

	char_log("Id for the next created character: %d." RETCODE, char_id_count);

	return;
}

#ifdef TXT_ONLY
//-------------------------
// quick sorting (by account_id) (by [Yor])
//-------------------------
void tri_rapide(int tableau[], int premier, int dernier) {
	int temp, vmin, vmax, separateur_de_listes, separateur_de_listes2;
	int account_id;

	vmin = premier;
	vmax = dernier;
	separateur_de_listes = char_dat[tableau[(premier + dernier) / 2]].account_id;
	separateur_de_listes2 = char_dat[tableau[(premier + dernier) / 2]].char_num;

	do {
		while((account_id = char_dat[tableau[vmin]].account_id) < separateur_de_listes ||
		      // if same account id, we sort by slot.
		      (account_id == separateur_de_listes &&
		       char_dat[tableau[vmin]].char_num < separateur_de_listes2))
			vmin++;
		while((account_id = char_dat[tableau[vmax]].account_id) > separateur_de_listes ||
		      // if same account id, we sort by slot.
		      (account_id == separateur_de_listes &&
		       char_dat[tableau[vmax]].char_num > separateur_de_listes2))
			vmax--;

		if (vmin <= vmax) {
			temp = tableau[vmin];
			tableau[vmin++] = tableau[vmax];
			tableau[vmax--] = temp;
		}
	} while(vmin <= vmax);

	if (premier < vmax)
		tri_rapide(tableau, premier, vmax);
	if (vmin < dernier)
		tri_rapide(tableau, vmin, dernier);
}

//---------------------------------------------------------
// Function to save characters in files (speed up by [Yor])
//---------------------------------------------------------
void mmo_char_sync(void) { // only for TXT version
	char line[65536];
	int i;
	int lock;
	FILE *fp;
	int *id;

	CALLOC(id, int, char_num);

	// Sorting before save (by [Yor])
	for(i = 0; i < char_num; i++)
		id[i] = i;
	if (char_num > 1)
		tri_rapide(id, 0, char_num - 1);

	// Data save
	if ((fp = lock_fopen(char_txt, &lock)) == NULL) {
		printf("WARNING: Server can't not save characters.\n");
		char_log("WARNING: Server can't not save characters." RETCODE);
	} else {
		for(i = 0; i < char_num; i++) {
			mmo_char_tostr(line, &char_dat[id[i]], id[i]); // use of sorted index
			fprintf(fp, "%s" RETCODE, line);
		}
		fprintf(fp, "%d\t%%newid%%" RETCODE, char_id_count);
		lock_fclose(fp, char_txt, &lock);
	}

	FREE(id);

	// save friends file
	if (friends_txt_flag)
		mmo_friend_sync();

	return;
}
#endif /* TXT_ONLY */

//-------------------------------------------------------------------------
// Function to save (in a periodic way) datas in files (TXT)
// Function to remove (in a periodic way) offline players from memory (SQL)
//-------------------------------------------------------------------------
int mmo_char_sync_timer(int tid, unsigned int tick, int id, int data) {
#ifdef TXT_ONLY
	mmo_char_sync();
	inter_save();
#endif /* TXT_ONLY */

#ifdef USE_SQL
	// here, we remove older characters from memory
	time_t actual_time; // speed up
	int i;

	// remove character from memory
	//actual_time = time(NULL) - (anti_freeze_interval * (anti_freeze_counter + 1)); // not remove before that the map server is really frozen if it is (anti_freeze_system = 12 times 10 secs)
	actual_time = time(NULL) - 15; // conserv 15 sec more the char in memory before to delete it
	for (i = 0; i < char_num; i++)
		if (char_dat_timer[i] < actual_time) {
			if (i != (char_num - 1)) {
				memcpy(&char_dat[i], &char_dat[char_num - 1], sizeof(struct mmo_charstatus));
				char_dat_timer[i] = char_dat_timer[char_num - 1];
				online_chars[i] = online_chars[char_num - 1];
			}
			i--; // retest same (new) value
			char_num--;
		}
#endif /* USE_SQL */

	return 0;
}

//-------------------------------------------------------------
// searching char_id for the server (based on wisp_server_name)
//-------------------------------------------------------------
void found_server_char_id() {
	int i;
	int pos_id;
#ifdef USE_SQL
	char t_character_name[50];
	char character_name_found[25];
#endif /* USE_SQL */

#ifdef TXT_ONLY
	int j;
	pos_id = char_nick2idx(wisp_server_name);
#else /* TXT_ONLY -> USE_SQL*/
	mysql_escape_string(t_character_name, wisp_server_name, strlen(wisp_server_name));
	sql_request("SELECT `char_id`, `name` FROM `%s` WHERE `name` = '%s'", char_db, t_character_name);
	pos_id = -1;
	while (sql_get_row()) {
		memset(character_name_found, 0, sizeof(character_name_found));
		strncpy(character_name_found, sql_get_string(1), 24);
		// Strict comparison (if found, we finish the function immediatly with correct value)
		if (strcmp(character_name_found, wisp_server_name) == 0) {
			pos_id = mmo_char_fromsql(sql_get_integer(0)); // return -1 if not found, index in char_dat[] if found
			break;
		}
	}
#endif /* USE_SQL */

	// server name has been found in database
	if (pos_id >= 0) { // -1: no char with this name, -2: exact sensitive name not found
		for(i = 0; i < global_reg_db[pos_id].global_reg_num; i++) {
			if (strcmp(global_reg_db[pos_id].global_reg[i].str, "Server_name") == 0) {
				// if found and char was specialy created...
				server_char_id = char_dat[pos_id].char_id;
				printf("Char id used to represent server: %d.\n", server_char_id);
				char_log("Char id used to represent server: %d." RETCODE, server_char_id);
				return;
			}
		}
		// if server name is found, and name is not the server char_id, stop program and say why
		printf(CL_RED "ERROR: wisp server name is name of a player." CL_RESET "\n");
		printf(CL_RED "       Freya has been stopped." CL_RESET "\n");
		printf(CL_RED "       Change wisp server name in char_freya.conf." CL_RESET "\n");
		char_log("ERROR: wisp server name is name of a player. Freya has been stopped. Change wisp server name in char_freya.conf." RETCODE);
		exit(1);
	}

	char_log("Creation of Server Character: '%s', stats: %d+%d+%d+%d+%d+%d=%d, hair: %d, hair color: %d." RETCODE,
	         wisp_server_name, 5, 5, 5, 5, 5, 5, 30, 1, 1);

#ifdef TXT_ONLY
	// server name has not been found -> create new char to have a valid char_id
	if (char_num >= char_max) {
		char_max += 256;
		REALLOC(char_dat, struct mmo_charstatus, char_max);
		REALLOC(global_reg_db, struct global_reg_db, char_max);
		REALLOC(friend_dat, struct friend_dat, char_max);
		REALLOC(char_dat_timer, time_t, char_max);
		REALLOC(online_chars, signed char, char_max);
		for(i = char_max - 256; i < char_max; i++) {
			global_reg_db[i].global_reg_num = 0;
			global_reg_db[i].global_reg = NULL;
			friend_dat[i].friend_num = 0;
			friend_dat[i].friends = NULL;
			online_chars[i] = 0;
			char_dat_timer[i] = 0;
		}
	}

	i = char_num;

	server_char_id = char_id_count++;

	memset(&char_dat[i], 0, sizeof(struct mmo_charstatus));
	global_reg_db[i].global_reg_num = 0;
	global_reg_db[i].global_reg = NULL;
	friend_dat[i].friend_num = 0;
	friend_dat[i].friends = NULL;

	char_dat[i].char_id = server_char_id;
//	char_dat[i].account_id = 0;
	char_dat[i].char_num = 1;
	strncpy(char_dat[i].name, wisp_server_name, 24);
//	char_dat[i].class = 0;
	char_dat[i].base_level = 1;
	char_dat[i].job_level = 1;
//	char_dat[i].base_exp = 0;
//	char_dat[i].job_exp = 0;
	char_dat[i].zeny = start_zeny;
	char_dat[i].str = 5;
	char_dat[i].agi = 5;
	char_dat[i].vit = 5;
	char_dat[i].int_ = 5;
	char_dat[i].dex = 5;
	char_dat[i].luk = 5;
	char_dat[i].max_hp = 40 * (100 + 5) / 100;
	char_dat[i].max_sp = 11 * (100 + 5) / 100;
	char_dat[i].hp = char_dat[i].max_hp;
	char_dat[i].sp = char_dat[i].max_sp;
//	char_dat[i].status_point = 0;
//	char_dat[i].skill_point = 0;
//	char_dat[i].option = 0;
//	char_dat[i].karma = 0;
//	char_dat[i].manner = 0;
//	char_dat[i].party_id = 0;
//	char_dat[i].guild_id = 0;
	char_dat[i].hair = 1;
	char_dat[i].hair_color = 1;
//	char_dat[i].clothes_color = 0;
	j = 0;
	if (start_weapon > 0) {
		char_dat[i].inventory[j].nameid = start_weapon; // Knife
		char_dat[i].inventory[j].amount = 1;
		char_dat[i].inventory[j].equip = 0x02;
		char_dat[i].inventory[j].identify = 1;
//		char_dat[i].inventory[j].attribute = 0; // if broken = 1
		j++;
	}
	if (start_armor > 0) {
		char_dat[i].inventory[j].nameid = start_armor; // Cotton Shirt
		char_dat[i].inventory[j].amount = 1;
		char_dat[i].inventory[j].equip = 0x10;
		char_dat[i].inventory[j].identify = 1;
//		char_dat[i].inventory[j].attribute = 0; // if broken = 1
		j++;
	}
	if (start_item > 0 || start_item_quantity > 0) {
		char_dat[i].inventory[j].nameid = start_item;
		char_dat[i].inventory[j].amount = start_item_quantity;
		char_dat[i].inventory[j].identify = 1;
//		char_dat[i].inventory[j].attribute = 0; // if broken = 1
		j++;
	}
	if (start_weapon > 0)
		char_dat[i].weapon = 1;
//	char_dat[i].shield = 0;
//	char_dat[i].head_top = 0;
//	char_dat[i].head_mid = 0;
//	char_dat[i].head_bottom = 0;
	memcpy(&char_dat[i].last_point, &start_point, sizeof(start_point));
	memcpy(&char_dat[i].save_point, &start_point, sizeof(start_point));
	char_dat[i].skill[1].id = 1; // basic skill
//	char_dat[i].skill[1].lv = 0; // it's normally added by map-server, so, just add it for the principe

	MALLOC(global_reg_db[i].global_reg, struct global_reg, 1);
	strncpy(global_reg_db[i].global_reg[0].str, "Server_name", 32);
	global_reg_db[i].global_reg[0].str[32] = '\0';
	global_reg_db[i].global_reg[0].value = 1;
	global_reg_db[i].global_reg_num = 1;

	char_num++;

	mmo_char_sync();
#endif /* TXT_ONLY */

#ifdef USE_SQL
	server_char_id = char_id_count++;

	// make new char.
	if (!sql_request("INSERT INTO `%s` (`char_id`,`account_id`,`char_num`,`name`,`zeny`,`str`,`agi`,`vit`,`int`,`dex`,`luk`,`max_hp`,`hp`,`max_sp`,`sp`,`hair`,`hair_color`,"
	                 "`last_map`,`last_x`,`last_y`,`save_map`,`save_x`,`save_y`) "
	                 " VALUES ('%d', '%d', '%d', '%s', '%d', '%d', '%d', '%d', '%d', '%d', '%d', '%d', '%d','%d', '%d','%d', '%d',"
	                 " '%s', '%d', '%d', '%s', '%d', '%d')", char_db,
	                 server_char_id, 0, 1, t_character_name, start_zeny, 5, 5, 5, 5, 5, 5,
	                 (40 * (100 + 5) / 100), (40 * (100 + 5) / 100), (11 * (100 + 5) / 100), (11 * (100 + 5) / 100), 1, 1,
	                 start_point.map, start_point.x, start_point.y, start_point.map, start_point.x, start_point.y)) {
		// if server name can not be created
		printf(CL_RED "ERROR: wisp server name can not be added in SQL database." CL_RESET "\n");
		printf(CL_RED "       Freya has been stopped." CL_RESET "\n");
		char_log("ERROR: wisp server name can not be added in SQL database. Freya has been stopped." RETCODE);
		exit(1);
	}

	// `inventory` (`id`, `char_id`, `nameid`, `amount`, `equip`, `identify`, `refine`, `attribute`, `card0`, `card1`, `card2`, `card3`)
	memset(tmp_sql, 0, sizeof(tmp_sql));
	if (start_weapon > 0)
		tmp_sql_len = sprintf(tmp_sql, "INSERT INTO `%s` (`char_id`, `nameid`, `amount`, `equip`, `identify`) VALUES ('%d', '%d', '%d', '%d', '%d')",
		                      inventory_db, server_char_id, start_weapon, 1, 0x02, 1); // Knife
	if (start_armor > 0) {
		if (tmp_sql[0] == '\0')
			tmp_sql_len = sprintf(tmp_sql, "INSERT INTO `%s` (`char_id`, `nameid`, `amount`, `equip`, `identify`) VALUES ('%d', '%d', '%d', '%d', '%d')",
			                      inventory_db, server_char_id, start_armor, 1, 0x10, 1); // Cotton Shirt
		else
			tmp_sql_len += sprintf(tmp_sql + tmp_sql_len, ", ('%d', '%d', '%d', '%d', '%d')",
			                       server_char_id, start_armor, 1, 0x10, 1); // Cotton Shirt
	}
	if (start_item > 0 || start_item_quantity > 0) {
		if (tmp_sql[0] == '\0')
			tmp_sql_len = sprintf(tmp_sql, "INSERT INTO `%s` (`char_id`, `nameid`, `amount`, `equip`, `identify`) VALUES ('%d', '%d', '%d', '%d', '%d')",
			                      inventory_db, server_char_id, start_item, start_item_quantity, 0, 1);
		else
			tmp_sql_len += sprintf(tmp_sql + tmp_sql_len, ", ('%d', '%d', '%d', '%d', '%d')",
			                       server_char_id, start_item, start_item_quantity, 0, 1);
	}
	if (tmp_sql[0] != '\0') {
		//printf("%s\n", tmp_sql);
		sql_request(tmp_sql);
	}

	// `skill` (`char_id`, `id`, `lv`) // it's normally added by map-server, so, just add it for the principe
	sql_request("INSERT INTO `%s`(`char_id`, `id`, `lv`) VALUES ('%d', '%d', '%d')",
	            skill_db, server_char_id, 1, 0); // basic skill

	//`global_reg_value` (`char_id`, `str`, `value`)
	sql_request("INSERT INTO `%s` (`char_id`, `str`, `value`) VALUES ('%d', '%s', '%d')",
	            global_reg_value, server_char_id, "Server_name", 1);
#endif /* USE_SQL */

	printf("Char id used to represent server: %d.\n", server_char_id);
	char_log("Char id used to represent server: %d." RETCODE, server_char_id);

	return;
}

//-----------------------------------
// Function to create a new character
//-----------------------------------
int make_new_char(int fd, char *dat) {
	struct char_session_data *sd;
	char name[25];
#ifdef USE_SQL
	char t_name[50];
#endif /* USE_SQL */
	int i, j;

	sd = session[fd]->session_data;

	memset(name, 0, sizeof(name));
	strncpy(name, dat, 24);

	// remove control characters from the name
	if (remove_control_chars(name)) {
		char_log("Make new char error (control char received in the name): connection #%d, account: %d." RETCODE, fd, sd->account_id);
		return -1; // This character name already exists
	}

	// replace insecable space by normal space (for GM work, private message, etc...)
	// note: it seems that in trade or vending some exploits can be done when name has insecable space
	for (i = 0; name[i]; i++)
		if (name[i] == (char)160)
			name[i] = 32;

	// check spaces
	if (!char_name_with_spaces) // if no space authorized
		for (i = 0; name[i]; i++)
			if (name[i] == 32) {
				char_log("Make new char error (name with space): connection #%d, account: %d, name: %s." RETCODE, fd, sd->account_id, name);
				return -1; // This character name already exists
			}

	// check double spaces
	if (!char_name_double_spaces) // if no double space authorized
		for (i = 0; name[i]; i++)
			if (name[i] == 32 && name[i + 1] == 32) {
				for (j = i; name[j]; j++)
					name[j] = name[j + 1];
				i--; // test again same i
			}

	// check space at first char
	if (!char_name_space_at_first) // if no space at first authorized
		while (name[0] == 32)
			for (i = 0; name[i]; i++)
				name[i] = name[i + 1];

	// check space at last char
	if (!char_name_space_at_last) // if no space at last authorized
		while ((i = strlen(name)) > 0 && name[i - 1] == 32)
			name[i - 1] = '\0';

	// check length of character name
	if (strlen(name) < 4) {
		char_log("Make new char error (character name too small): connection #%d, account: %d, name: '%s'." RETCODE, fd, sd->account_id, name);
		return -1; // This character name already exists
	}

	// Check authorized letters/symbols in the name of the character
	if (char_name_option == 1) { // only letters/symbols in char_name_letters are authorized
		for (i = 0; name[i]; i++)
			if (strchr(char_name_letters, name[i]) == NULL) {
				char_log("Make new char error (invalid letter in the name): connection #%d, account: %d, name: %s, invalid letter: %c." RETCODE,
				         fd, sd->account_id, name, name[i]);
				return -1; // This character name already exists
			}
	} else if (char_name_option == 2) { // letters/symbols in char_name_letters are forbidden
		for (i = 0; name[i]; i++)
			if (strchr(char_name_letters, name[i]) != NULL) {
				char_log("Make new char error (invalid letter in the name): connection #%d, account: %d, name: %s, invalid letter: %c." RETCODE,
				         fd, sd->account_id, name, name[i]);
				return -1; // This character name already exists
			}
	} // else, all letters/symbols are authorized (except control char removed before)

	// check stat error
	if (dat[24] + dat[25] + dat[26] + dat[27] + dat[28] + dat[29] != 30 || // stats (6*5)
	    dat[30] < 0 || dat[30] >= 9 || // slots (0-8)
	    dat[33] <= 0 || dat[33] > max_hair_style || // hair style
	    dat[31] < 0 || dat[31] > max_hair_color) { // hair color
		char_log("Make new char error (invalid values): connection #%d, account: %d, slot %d, name: %s, stats: %d+%d+%d+%d+%d+%d=%d, hair: %d, hair color: %d" RETCODE,
		         fd, sd->account_id, dat[30], name, dat[24], dat[25], dat[26], dat[27], dat[28], dat[29], dat[24] + dat[25] + dat[26] + dat[27] + dat[28] + dat[29], dat[33], dat[31]);
		return -2; // Character creation is denied
	}

	// check individual stat value
	for(i = 24; i <= 29; i++) {
		if (dat[i] < 1 || dat[i] > 9) {
			char_log("Make new char error (invalid stat value: not between 1 to 9): connection #%d, account: %d, slot %d, name: %s, stats: %d+%d+%d+%d+%d+%d=%d, hair: %d, hair color: %d" RETCODE,
			         fd, sd->account_id, dat[30], name, dat[24], dat[25], dat[26], dat[27], dat[28], dat[29], dat[24] + dat[25] + dat[26] + dat[27] + dat[28] + dat[29], dat[33], dat[31]);
			return -2; // Character creation is denied
		}
	}

	if (strcmp(wisp_server_name, name) == 0) {
		char_log("Make new char error (used name is wisp name for server: %s): connection #%d, account: %d, slot %d, name: %s, stats: %d+%d+%d+%d+%d+%d=%d, hair: %d, hair color: %d." RETCODE,
		         wisp_server_name, fd, sd->account_id, dat[30], name, dat[24], dat[25], dat[26], dat[27], dat[28], dat[29], dat[24] + dat[25] + dat[26] + dat[27] + dat[28] + dat[29], dat[33], dat[31]);
		return -1; // This character name already exists
	}

	if (check_bad_word(name, strlen(name))) {
		char_log("Make new char error (name uses a bad word): connection #%d, account: %d, slot %d, name: %s, stats: %d+%d+%d+%d+%d+%d=%d, hair: %d, hair color: %d." RETCODE,
		         fd, sd->account_id, dat[30], name, dat[24], dat[25], dat[26], dat[27], dat[28], dat[29], dat[24] + dat[25] + dat[26] + dat[27] + dat[28] + dat[29], dat[33], dat[31]);
		return -2; // Character creation is denied
	}

	j = 0;
	for(i = 0; i < char_num; i++) {
		if (strcmp(char_dat[i].name, name) == 0 ||
		    (name_ignoring_case == 0 && strcasecmp(char_dat[i].name, name) == 0)) {
			char_log("Make new char error (name already exists): connection #%d, account: %d, slot %d, name: %s, stats: %d+%d+%d+%d+%d+%d=%d, hair: %d, hair color: %d." RETCODE,
			         fd, sd->account_id, dat[30], name, dat[24], dat[25], dat[26], dat[27], dat[28], dat[29], dat[24] + dat[25] + dat[26] + dat[27] + dat[28] + dat[29], dat[33], dat[31]);
			return -1; // This character name already exists
		}
		if (char_dat[i].account_id == sd->account_id) {
			if (char_dat[i].char_num == dat[30]) {
				char_log("Make new char error (slot already used): connection #%d, account: %d, slot %d, name: %s, stats: %d+%d+%d+%d+%d+%d=%d, hair: %d, hair color: %d." RETCODE,
				         fd, sd->account_id, dat[30], name, dat[24], dat[25], dat[26], dat[27], dat[28], dat[29], dat[24] + dat[25] + dat[26] + dat[27] + dat[28] + dat[29], dat[33], dat[31]);
				return -2; // Character creation is denied
			}
			j++;
		}
	}

	// Charlimit per account (adapted from a post found on freya's forum) [Yor] (thanks to Sirius_Black)
	if (chars_per_account != 0 && j >= chars_per_account) {
		char_log("Make new char error (charlimit per account exceeded): connection #%d, account: %d, slot %d, name: %s, stats: %d+%d+%d+%d+%d+%d=%d, hair: %d, hair color: %d." RETCODE,
		         fd, sd->account_id, dat[30], name, dat[24], dat[25], dat[26], dat[27], dat[28], dat[29], dat[24] + dat[25] + dat[26] + dat[27] + dat[28] + dat[29], dat[33], dat[31]);
		return -2; // Character creation is denied
	}

	if (char_num >= char_max) {
		char_max += 256;
		REALLOC(char_dat, struct mmo_charstatus, char_max);
		REALLOC(global_reg_db, struct global_reg_db, char_max);
#ifdef TXT_ONLY
		REALLOC(friend_dat, struct friend_dat, char_max);
#endif // TXT_ONLY
		REALLOC(char_dat_timer, time_t, char_max);
		REALLOC(online_chars, signed char, char_max);
		for(j = char_max - 256; j < char_max; j++) {
			global_reg_db[j].global_reg_num = 0;
			global_reg_db[j].global_reg = NULL;
#ifdef TXT_ONLY
			friend_dat[j].friend_num = 0;
			friend_dat[j].friends = NULL;
#endif // TXT_ONLY
			online_chars[j] = 0;
			char_dat_timer[j] = 0;
		}
	}

	char_log("Creation of New Character: connection #%d, account: %d, slot %d, character Name: %s, stats: %d+%d+%d+%d+%d+%d=%d, hair: %d, hair color: %d." RETCODE,
	         fd, sd->account_id, dat[30], name, dat[24], dat[25], dat[26], dat[27], dat[28], dat[29], dat[24] + dat[25] + dat[26] + dat[27] + dat[28] + dat[29], dat[33], dat[31]);

	i = char_num;

	memset(&char_dat[i], 0, sizeof(struct mmo_charstatus));
	global_reg_db[i].global_reg_num = 0;
	global_reg_db[i].global_reg = NULL;
	friend_dat[i].friend_num = 0;
	friend_dat[i].friends = NULL;

	char_dat[i].char_id = char_id_count++;
	char_dat[i].account_id = sd->account_id;
	char_dat[i].char_num = dat[30];
	strncpy(char_dat[i].name, name, 24);
//	char_dat[i].class = 0;
	char_dat[i].base_level = 1;
	char_dat[i].job_level = 1;
//	char_dat[i].base_exp = 0;
//	char_dat[i].job_exp = 0;
	char_dat[i].zeny = start_zeny;
	char_dat[i].str = dat[24];
	char_dat[i].agi = dat[25];
	char_dat[i].vit = dat[26];
	char_dat[i].int_ = dat[27];
	char_dat[i].dex = dat[28];
	char_dat[i].luk = dat[29];
	char_dat[i].max_hp = 40 * (100 + char_dat[i].vit) / 100;
	char_dat[i].max_sp = 11 * (100 + char_dat[i].int_) / 100;
	char_dat[i].hp = char_dat[i].max_hp;
	char_dat[i].sp = char_dat[i].max_sp;
//	char_dat[i].status_point = 0;
//	char_dat[i].skill_point = 0;
//	char_dat[i].option = 0;
//	char_dat[i].karma = 0;
//	char_dat[i].manner = 0;
//	char_dat[i].party_id = 0;
//	char_dat[i].guild_id = 0;
	char_dat[i].hair = dat[33];
	char_dat[i].hair_color = dat[31];
//	char_dat[i].clothes_color = 0;
	j = 0;
	if (start_weapon > 0) {
		char_dat[i].inventory[j].nameid = start_weapon; // Knife
		char_dat[i].inventory[j].amount = 1;
		char_dat[i].inventory[j].equip = 0x02;
		char_dat[i].inventory[j].identify = 1;
//		char_dat[i].inventory[j].attribute = 0; // if broken = 1
		j++;
	}
	if (start_armor > 0) {
		char_dat[i].inventory[j].nameid = start_armor; // Cotton Shirt
		char_dat[i].inventory[j].amount = 1;
		char_dat[i].inventory[j].equip = 0x10;
		char_dat[i].inventory[j].identify = 1;
//		char_dat[i].inventory[j].attribute = 0; // if broken = 1
		j++;
	}
	if (start_item > 0 || start_item_quantity > 0) {
		char_dat[i].inventory[j].nameid = start_item;
		char_dat[i].inventory[j].amount = start_item_quantity;
		char_dat[i].inventory[j].identify = 1;
//		char_dat[i].inventory[j].attribute = 0; // if broken = 1
		j++;
	}
	if (start_weapon > 0)
		char_dat[i].weapon = 1;
//	char_dat[i].shield = 0;
//	char_dat[i].head_top = 0;
//	char_dat[i].head_mid = 0;
//	char_dat[i].head_bottom = 0;
	memcpy(&char_dat[i].last_point, &start_point, sizeof(start_point));
	memcpy(&char_dat[i].save_point, &start_point, sizeof(start_point));
	char_dat[i].skill[1].id = 1; // basic skill
//	char_dat[i].skill[1].lv = 0; // it's normally added by map-server, so, just add it for the principe

	char_num++;

	mmo_char_sync();

	return i; // found_char = index in db (TXT), char_id (SQL), -1: void
}

//----------------------------------------------------
// This function return the name of the job (by [Yor])
//----------------------------------------------------
char * job_name(int class) {
	switch (class) {
	case 0:    return "Novice";
	case 1:    return "Swordsman";
	case 2:    return "Mage";
	case 3:    return "Archer";
	case 4:    return "Acolyte";
	case 5:    return "Merchant";
	case 6:    return "Thief";
	case 7:    return "Knight";
	case 8:    return "Priest";
	case 9:    return "Wizard";
	case 10:   return "Blacksmith";
	case 11:   return "Hunter";
	case 12:   return "Assassin";
	case 13:   return "Peco knight";
	case 14:   return "Crusader";
	case 15:   return "Monk";
	case 16:   return "Sage";
	case 17:   return "Rogue";
	case 18:   return "Alchemist";
	case 19:   return "Bard";
	case 20:   return "Dancer";
	case 21:   return "Peco crusader";
	case 22:   return "Wedding";
	case 23:   return "Super Novice";
	case 24:   return "Gunslinger";
	case 25:   return "Ninja";
	case 26:   return "Xmas";
	case 4001: return "Novice High";
	case 4002: return "Swordsman High";
	case 4003: return "Mage High";
	case 4004: return "Archer High";
	case 4005: return "Acolyte High";
	case 4006: return "Merchant High";
	case 4007: return "Thief High";
	case 4008: return "Lord Knight";
	case 4009: return "High Priest";
	case 4010: return "High Wizard";
	case 4011: return "Whitesmith";
	case 4012: return "Sniper";
	case 4013: return "Assassin Cross";
	case 4014: return "Peco Knight";
	case 4015: return "Paladin";
	case 4016: return "Champion";
	case 4017: return "Professor";
	case 4018: return "Stalker";
	case 4019: return "Creator";
	case 4020: return "Clown";
	case 4021: return "Gypsy";
	case 4022: return "Peco Paladin";
	case 4023: return "Baby Novice";
	case 4024: return "Baby Swordsman";
	case 4025: return "Baby Mage";
	case 4026: return "Baby Archer";
	case 4027: return "Baby Acolyte";
	case 4028: return "Baby Merchant";
	case 4029: return "Baby Thief";
	case 4030: return "Baby Knight";
	case 4031: return "Baby Priest";
	case 4032: return "Baby Wizard";
	case 4033: return "Baby Blacksmith";
	case 4034: return "Baby Hunter";
	case 4035: return "Baby Assassin";
	case 4036: return "Baby Peco Knight";
	case 4037: return "Baby Crusader";
	case 4038: return "Baby Monk";
	case 4039: return "Baby Sage";
	case 4040: return "Baby Rogue";
	case 4041: return "Baby Alchemist";
	case 4042: return "Baby Bard";
	case 4043: return "Baby Dancer";
	case 4044: return "Baby Peco Crusader";
	case 4045: return "Super Baby";
	case 4046: return "Taekwon Kid";
	case 4047: return "Taekwon Master";
	case 4048: return "Taekwon Master";
	case 4049: return "Soul Linker";
	case 4050: return "Bon Gun";
	case 4051: return "Death Knight";
	case 4052: return "Dark Collector";
	case 4053: return "Munak";
	}

	return "Unknown Job";
}

//-------------------------
// quick sorting (by [Yor])
//-------------------------
void quick_sorting(int tableau[], int premier, int dernier) {
	int temp, vmin, vmax, separateur_de_listes, separateur_de_listes2, separateur_de_listes3;
	int cpm_result;
	char *p_name, *p_map;

	vmin = premier;
	vmax = dernier;

	switch (online_sorting_option) {
	case 1: // by name (without case sensitive)
		p_name = char_dat[tableau[(premier + dernier) / 2]].name;
		do {
			while((cpm_result = strcasecmp(char_dat[tableau[vmin]].name, p_name)) < 0 ||
			      // if same name, we sort with case sensitive.
			      (cpm_result == 0 &&
			       strcmp(char_dat[tableau[vmin]].name, p_name) < 0))
				vmin++;
			while((cpm_result = strcasecmp(char_dat[tableau[vmax]].name, p_name)) > 0 ||
			      // if same name, we sort with case sensitive.
			      (cpm_result == 0 &&
			       strcmp(char_dat[tableau[vmax]].name, p_name) > 0))
				vmax--;

			if (vmin <= vmax) {
				temp = tableau[vmin];
				tableau[vmin++] = tableau[vmax];
				tableau[vmax--] = temp;
			}
		} while(vmin <= vmax);
		break;

	case 2: // by zeny
		separateur_de_listes = char_dat[tableau[(premier + dernier) / 2]].zeny;
		p_name = char_dat[tableau[(premier + dernier) / 2]].name;
		do {
			while((cpm_result = char_dat[tableau[vmin]].zeny) < separateur_de_listes ||
			      // if same number of zenys, we sort by name.
			      (cpm_result == separateur_de_listes &&
			       strcasecmp(char_dat[tableau[vmin]].name, p_name) < 0))
				vmin++;
			while((cpm_result = char_dat[tableau[vmax]].zeny) > separateur_de_listes ||
			      // if same number of zenys, we sort by name.
			      (cpm_result == separateur_de_listes &&
			       strcasecmp(char_dat[tableau[vmax]].name, p_name) > 0))
				vmax--;

			if (vmin <= vmax) {
				temp = tableau[vmin];
				tableau[vmin++] = tableau[vmax];
				tableau[vmax--] = temp;
			}
		} while(vmin <= vmax);
		break;

	case 3: // by base level
		separateur_de_listes = char_dat[tableau[(premier + dernier) / 2]].base_level;
		separateur_de_listes2 = char_dat[tableau[(premier + dernier) / 2]].base_exp;
		do {
			while((cpm_result = char_dat[tableau[vmin]].base_level) < separateur_de_listes ||
			      // if same base level, we sort by base exp.
			      (cpm_result == separateur_de_listes &&
			       char_dat[tableau[vmin]].base_exp < separateur_de_listes2))
				vmin++;
			while((cpm_result = char_dat[tableau[vmax]].base_level) > separateur_de_listes ||
			      // if same base level, we sort by base exp.
			      (cpm_result == separateur_de_listes &&
			       char_dat[tableau[vmax]].base_exp > separateur_de_listes2))
				vmax--;

			if (vmin <= vmax) {
				temp = tableau[vmin];
				tableau[vmin++] = tableau[vmax];
				tableau[vmax--] = temp;
			}
		} while(vmin <= vmax);
		break;

	case 4: // by job (and job level)
		separateur_de_listes = char_dat[tableau[(premier + dernier) / 2]].class;
		separateur_de_listes2 = char_dat[tableau[(premier + dernier) / 2]].job_level;
		separateur_de_listes3 = char_dat[tableau[(premier + dernier) / 2]].job_exp;
		do {
			while((cpm_result = char_dat[tableau[vmin]].class) < separateur_de_listes ||
			      // if same job, we sort by job level.
			      (cpm_result == separateur_de_listes &&
			       char_dat[tableau[vmin]].job_level < separateur_de_listes2) ||
			      // if same job and job level, we sort by job exp.
			      (cpm_result == separateur_de_listes &&
			       char_dat[tableau[vmin]].job_level == separateur_de_listes2 &&
			       char_dat[tableau[vmin]].job_exp < separateur_de_listes3))
				vmin++;
			while((cpm_result = char_dat[tableau[vmax]].class) > separateur_de_listes ||
			      // if same job, we sort by job level.
			      (cpm_result == separateur_de_listes &&
			       char_dat[tableau[vmax]].job_level > separateur_de_listes2) ||
			      // if same job and job level, we sort by job exp.
			      (cpm_result == separateur_de_listes &&
			       char_dat[tableau[vmax]].job_level == separateur_de_listes2 &&
			       char_dat[tableau[vmax]].job_exp > separateur_de_listes3))
				vmax--;

			if (vmin <= vmax) {
				temp = tableau[vmin];
				tableau[vmin++] = tableau[vmax];
				tableau[vmax--] = temp;
			}
		} while(vmin <= vmax);
		break;

	case 5: // by location map name
		p_map = char_dat[tableau[(premier + dernier) / 2]].last_point.map;
		p_name = char_dat[tableau[(premier + dernier) / 2]].name;
		do {
			while((cpm_result = strcmp(char_dat[tableau[vmin]].last_point.map, p_map)) < 0 || // no map is identical and with upper cases (not use strcasecmp)
			      // if same map name, we sort by name.
			      (cpm_result == 0 &&
			       strcasecmp(char_dat[tableau[vmin]].name, p_name) < 0))
				vmin++;
			while((cpm_result = strcmp(char_dat[tableau[vmax]].last_point.map, p_map)) > 0 || // no map is identical and with upper cases (not use strcasecmp)
			      // if same map name, we sort by name.
			      (cpm_result == 0 &&
			       strcasecmp(char_dat[tableau[vmax]].name, p_name) > 0))
				vmax--;

			if (vmin <= vmax) {
				temp = tableau[vmin];
				tableau[vmin++] = tableau[vmax];
				tableau[vmax--] = temp;
			}
		} while(vmin <= vmax);
		break;

	default: // 0 or invalid value: no sorting (by account)
		separateur_de_listes = char_dat[tableau[(premier + dernier) / 2]].account_id;
		separateur_de_listes2 = char_dat[tableau[(premier + dernier) / 2]].char_num;
		do {
			while((cpm_result = char_dat[tableau[vmin]].account_id) < separateur_de_listes ||
			      // if same account id, we sort by slot.
			      (cpm_result == separateur_de_listes &&
			       char_dat[tableau[vmin]].char_num < separateur_de_listes2))
				vmin++;
			while((cpm_result = char_dat[tableau[vmax]].account_id) > separateur_de_listes ||
			      // if same account id, we sort by slot.
			      (cpm_result == separateur_de_listes &&
			       char_dat[tableau[vmax]].char_num > separateur_de_listes2))
				vmax--;

			if (vmin <= vmax) {
				temp = tableau[vmin];
				tableau[vmin++] = tableau[vmax];
				tableau[vmax--] = temp;
			}
		} while(vmin <= vmax);
		break;
	}

	if (premier < vmax)
		quick_sorting(tableau, premier, vmax);
	if (vmin < dernier)
		quick_sorting(tableau, vmin, dernier);
}

char *php_addslashes(const char *original_string) {
	const char *pointer1;
	char *pointer2;

	pointer1 = original_string;
	pointer2 = temp_char_buffer;
	while(*pointer1) {
		if ((*pointer1 == 92) || (*pointer1 == 39) || (*pointer1 == 34)) *(pointer2++) = 92;
		*(pointer2++) = *(pointer1++);
		if ((void *)(&temp_char_buffer + 1022) <= (void *)pointer2) break; // overflow?
	}
	*pointer2 = 0;

	return temp_char_buffer;
}

//-------------------------------------------------------------
// Function to create the online files (txt and html). by [Yor]
//-------------------------------------------------------------
void create_online_files(void) {
	int i, j, k, l; // for loops
	int players;    // count the number of players
	FILE *fp;       // for the txt file
	char temp[256];       // to prepare what we must display
	time_t time_server;   // for number of seconds
	struct tm *datetime;  // variable for time in structure ->tm_mday, ->tm_sec, ...
	int *id;
	unsigned char agit_flag; // 0: WoE not starting, Woe is running
	int txt_temp, html_temp; // used to change value when war of emperium

	// something to display in normal mode?
	if ((online_display_option_txt & (1 + 2 + 4 + 8 + 16 + 32 + 64)) == 0 &&
	    (online_display_option_html & (1 + 2 + 4 + 8 + 16 + 32 + 64)) == 0 &&
	    online_display_option_php == 0) // we display nothing, so return
		return;

	// check for agit
	txt_temp = online_display_option_txt;
	html_temp = online_display_option_html;
	agit_flag = 0;
	for(i = 0; i < MAX_MAP_SERVERS; i++)
		if (server[i].agit_flag) {
			// check if we don't display coordinates (then display only map name)
			if ((online_display_option_txt & 128) && (online_display_option_txt & 16)) { // 128: not display coordinates when it's War of Emperium (map is displayed)
				txt_temp = txt_temp & ~16; // 16: mapname and coordinates
				txt_temp = txt_temp | 8; // 8: map name
			}
			// check if we don't display map name (no coordinates)
			if (online_display_option_txt & 256) // 256: not display map and coordinates when it's War of Emperium
				txt_temp = txt_temp & ~(8 + 16); // 8: map name // 16: mapname and coordinates
			// check if we don't display name
			if (online_display_option_txt & 512) // 512: not display name when it's War of Emperium
				txt_temp = txt_temp & ~(1 + 64); // 1: name (just the name, no function like 'GM') // 64: name (with 'GM' if the player is a GM)
			// check if we don't display coordinates (then display only map name)
			if ((online_display_option_html & 128) && (online_display_option_html & 16)) { // 128: not display coordinates when it's War of Emperium (map is displayed)
				html_temp = html_temp & ~16; // 16: mapname and coordinates
				html_temp = html_temp | 8; // 8: map name
			}
			// check if we don't display map name (no coordinates)
			if (online_display_option_html & 256) // 256: not display map and coordinates when it's War of Emperium
				html_temp = html_temp & ~(8 + 16); // 8: map name // 16: mapname and coordinates
			// check if we don't display name
			if (online_display_option_html & 512) // 512: not display name when it's War of Emperium
				html_temp = html_temp & ~(1 + 64); // 1: name (just the name, no function like 'GM') // 64: name (with 'GM' if the player is a GM)
			agit_flag = 1;
			break;
		}

	// fix max values of online_display_option... (need to check == 0 after)
	// (if value is 0, no file is done)
	// 1: name (just the name, no function like 'GM')
	// 2: job
	// 4: levels
	// 8: map name
	// 16: mapname and coordinates
	// 32: zenys
	// 64: name (with 'GM' if the player is a GM)
	// 128: not display coordinates when it's War of Emperium (map is displayed) (suppress option 16 and replace by option 8 when WoE)
	// 256: not display map and coordinates when it's War of Emperium (suppress options 8 and 16 when WoE)
	// 512: not display name when it's War of Emperium (suppress options 1 and 64 when WoE)
	txt_temp  = txt_temp  & (1 + 2 + 4 + 8 + 16 + 32 + 64);
	html_temp = html_temp & (1 + 2 + 4 + 8 + 16 + 32 + 64);

	//char_log("Creation of online players files." RETCODE);

	CALLOC(id, int, char_num);

	// Get number of online players, id of each online players
	players = 0;
	// sort online characters.
	for(i = 0; i < char_num; i++) {
		if (online_chars[i] > 0) { // get only online (and not hidden) character
			id[players] = i;
			players++;
		}
	}
	if (online_sorting_option && players > 1)
		quick_sorting(id, 0, players - 1);

	// get time
	time(&time_server); // get time in seconds since 1/1/1970
	datetime = localtime(&time_server); // convert seconds in structure

	// write txt files
	if ((online_display_option_txt & (1 + 2 + 4 + 8 + 16 + 32 + 64)) != 0) {
		if ((fp = fopen(online_txt_filename, "w")) != NULL) {
			// get time
			memset(temp, 0, sizeof(temp));
			strftime(temp, sizeof(temp) - 1, "%d %b %Y %X", datetime); // like sprintf, but only for date/time (05 dec 2003 15:12:52)
			// write heading
			fprintf(fp, "Online Players on %s (%s):" RETCODE, server_name, temp);
			if (agit_flag)
				fprintf(fp, "Actually, it's War of Emperium." RETCODE);
			fprintf(fp, RETCODE);

			if (txt_temp != 0) {
				// If we display at least 1 player
				if (players > 0) {
					j = 0; // count the number of characters for the txt version and to set the separate line
					if ((txt_temp & 1) || (txt_temp & 64)) {
						if (txt_temp & 64) {
							fprintf(fp, "Name                          "); // 30
							j += 30;
						} else {
							fprintf(fp, "Name                     "); // 25
							j += 25;
						}
					}
					if ((txt_temp & 6) == 6) {
						fprintf(fp, "Job                 Levels "); // 27
						j += 27;
					} else if (txt_temp & 2) {
						fprintf(fp, "Job                "); // 19
						j += 19;
					} else if (txt_temp & 4) {
						fprintf(fp, " Levels "); // 8
						j += 8;
					}
					if (txt_temp & 24) { // 8 or 16
						if (txt_temp & 16) {
							fprintf(fp, "Location     ( x , y ) "); // 23
							j += 23;
						} else {
							fprintf(fp, "Location     "); // 13
							j += 13;
						}
					}
					if (txt_temp & 32) {
						fprintf(fp, "          Zenys "); // 16
						j += 16;
					}
					fprintf(fp, RETCODE);
					for (k = 0; k < j; k++)
						fprintf(fp, "-");
					fprintf(fp, RETCODE);

					// display each player.
					for (i = 0; i < players; i++) {
						// get id of the character (more speed)
						j = id[i];
						// displaying the character name
						if ((txt_temp & 1) || (txt_temp & 64)) { // without/with 'GM' display
							l = isGM(char_dat[j].account_id);
							if (txt_temp & 64) {
								if (l >= online_gm_display_min_level)
									fprintf(fp, "%-24s (GM) ", char_dat[j].name);
								else
									fprintf(fp, "%-24s      ", char_dat[j].name);
							} else
								fprintf(fp, "%-24s ", char_dat[j].name);
							// name of the character in the html (no < >, because that create problem in html code)
						}
						// displaying of the job
						if (txt_temp & 6) {
							if ((txt_temp & 6) == 6)
								fprintf(fp, "%-18s %3d/%3d ", job_name(char_dat[j].class), char_dat[j].base_level, char_dat[j].job_level);
							else if (txt_temp & 2)
								fprintf(fp, "%-18s ", job_name(char_dat[j].class));
							else if (txt_temp & 4)
								fprintf(fp, "%3d/%3d ", char_dat[j].base_level, char_dat[j].job_level);
						}
						// displaying of the map
						if (txt_temp & 24) { // 8 or 16
							// prepare map name
							memset(temp, 0, sizeof(temp));
							strncpy(temp, char_dat[j].last_point.map, 16);
							if (strchr(temp, '.') != NULL)
								temp[strchr(temp, '.') - temp] = '\0'; // suppress the '.gat'
							// write map name
							if (txt_temp & 16) // map-name AND coordinates
								fprintf(fp, "%-12s (%3d,%3d) ", temp, char_dat[j].last_point.x, char_dat[j].last_point.y);
							else
								fprintf(fp, "%-12s ", temp);
						}
						// displaying number of zenys
						if (txt_temp & 32) {
							// write number of zenys
							if (char_dat[j].zeny == 0) // if no zeny
								fprintf(fp, "        no zeny ");
							else
								fprintf(fp, "%13d z ", char_dat[j].zeny);
						}
						fprintf(fp, RETCODE);
					}
					fprintf(fp, RETCODE);
				}
			}

			// Displaying number of online players
			if (players == 0) {
				fprintf(fp, "No user is online." RETCODE);
			} else if (players == 1) {
				fprintf(fp, "1 user is online." RETCODE);
			} else {
				fprintf(fp, "%d users are online." RETCODE, players);
			}
		}
		fclose(fp);
	}

	// write html files
	if ((online_display_option_html & (1 + 2 + 4 + 8 + 16 + 32 + 64)) != 0) {
		if ((fp = fopen(online_html_filename, "w")) != NULL) {
			// get time
			memset(temp, 0, sizeof(temp));
			strftime(temp, sizeof(temp) - 1, "%d %b %Y %X", datetime); // like sprintf, but only for date/time (05 dec 2003 15:12:52)
			// write heading
			fprintf(fp, "<HTML>" RETCODE
			            "  <HEAD>" RETCODE
			            "    <META http-equiv=\"Refresh\" content=\"%d\">" RETCODE, online_refresh_html); // update on client explorer every x seconds
			fprintf(fp, "    <TITLE>Online Players on %s</TITLE>" RETCODE, server_name);
			fprintf(fp, "  </HEAD>" RETCODE
			            "  <BODY>" RETCODE
			            "    <H3>Online Players on %s (%s):</H3>" RETCODE, server_name, temp);
			if (agit_flag)
				fprintf(fp, "    <H4>Actually, it's War of Emperium.</H4>" RETCODE);

			if (html_temp != 0) {
				// If we display at least 1 player
				if (players > 0) {
					fprintf(fp, "    <table border=\"1\" cellspacing=\"1\">" RETCODE
					            "      <tr>" RETCODE);
					if ((html_temp & 1) || (html_temp & 64))
						fprintf(fp, "        <td><b>Name</b></td>" RETCODE);
					if ((html_temp & 6) == 6)
						fprintf(fp, "        <td><b>Job (levels)</b></td>" RETCODE);
					else if (html_temp & 2)
						fprintf(fp, "        <td><b>Job</b></td>" RETCODE);
					else if (html_temp & 4)
						fprintf(fp, "        <td><b>Levels</b></td>" RETCODE);
					if (html_temp & 24) // 8 or 16
						fprintf(fp, "        <td><b>Location</b></td>" RETCODE);
					if (html_temp & 32)
						fprintf(fp, "        <td ALIGN=CENTER><b>zenys</b></td>" RETCODE);
					fprintf(fp, "      </tr>" RETCODE);

					// display each player.
					for (i = 0; i < players; i++) {
						// get id of the character (more speed)
						j = id[i];
						fprintf(fp, "      <tr>" RETCODE);
						// displaying the character name
						if ((html_temp & 1) || (html_temp & 64)) { // without/with 'GM' display
							strcpy(temp, char_dat[j].name);
							l = isGM(char_dat[j].account_id);
							// name of the character in the html (no < >, because that create problem in html code)
							fprintf(fp, "        <td>");
							if ((html_temp & 64) && l >= online_gm_display_min_level)
								fprintf(fp, "<b>");
							for (k = 0; temp[k]; k++) {
								switch(temp[k]) {
								case '<': // <
									fprintf(fp, "&lt;");
									break;
								case '>': // >
									fprintf(fp, "&gt;");
									break;
								default:
									fprintf(fp, "%c", temp[k]);
									break;
								};
							}
							if ((html_temp & 64) && l >= online_gm_display_min_level)
								fprintf(fp, "</b> (GM)");
							fprintf(fp, "</td>" RETCODE);
						}
						// displaying of the job
						if (html_temp & 6) {
							if ((html_temp & 6) == 6)
								fprintf(fp, "        <td>%s %d/%d</td>" RETCODE, job_name(char_dat[j].class), char_dat[j].base_level, char_dat[j].job_level);
							else if (html_temp & 2)
								fprintf(fp, "        <td>%s</td>" RETCODE, job_name(char_dat[j].class));
							else if (html_temp & 4)
								fprintf(fp, "        <td>%d/%d</td>" RETCODE, char_dat[j].base_level, char_dat[j].job_level);
						}
						// displaying of the map
						if (html_temp & 24) { // 8 or 16
							// prepare map name
							memset(temp, 0, sizeof(temp));
							strncpy(temp, char_dat[j].last_point.map, 16);
							if (strchr(temp, '.') != NULL)
								temp[strchr(temp, '.') - temp] = '\0'; // suppress the '.gat'
							// write map name
							if (html_temp & 16) // map-name AND coordinates
								fprintf(fp, "        <td>%s (%d, %d)</td>" RETCODE, temp, char_dat[j].last_point.x, char_dat[j].last_point.y);
							else
								fprintf(fp, "        <td>%s</td>" RETCODE, temp);
						}
						// displaying number of zenys
						if (html_temp & 32) {
							// write number of zenys
							if (char_dat[j].zeny == 0) // if no zeny
								fprintf(fp, "        <td ALIGN=RIGHT>no zeny</td>" RETCODE);
							else
								fprintf(fp, "        <td ALIGN=RIGHT>%d z</td>" RETCODE, char_dat[j].zeny);
						}
						fprintf(fp, "      </tr>" RETCODE);
					}
					fprintf(fp, "    </table>" RETCODE);
				}
			}

			// Displaying number of online players
			if (players == 0) {
				fprintf(fp, "    <p>No user is online.</p>" RETCODE);
			} else if (players == 1) {
				fprintf(fp, "    <p>1 user is online.</p>" RETCODE);
			} else {
				fprintf(fp, "    <p>%d users are online.</p>" RETCODE, players);
			}
			fprintf(fp, "  </BODY>" RETCODE
			            "</HTML>" RETCODE);
			fclose(fp);
		}
	}

	// write php files
	if (online_display_option_php != 0) {
		if ((fp = fopen(online_php_filename, "w")) != NULL) {
			// get time
			memset(temp, 0, sizeof(temp));
			strftime(temp, sizeof(temp) - 1, "%d %b %Y %X", datetime); // like sprintf, but only for date/time (05 dec 2003 15:12:52)
			// write heading
			fprintf(fp, "<?php" RETCODE
			            "// File generated on %s" RETCODE, temp);
			fprintf(fp, "$freya = array(" RETCODE
			            "\t'server_name'=>'%s'," RETCODE, php_addslashes(server_name));
			fprintf(fp, "\t'time'=>%d," RETCODE, (int)time_server);
			fprintf(fp, "\t'agit_flag'=>%d," RETCODE, agit_flag);

			// If we display at least 1 player
			if (players > 0) {
				fprintf(fp, "\t'players'=>array(" RETCODE);

				// display each player.
				for (i = 0; i < players; i++) {
					// get id of the character (more speed)
					j = id[i];
					fprintf(fp, "\t\t%d=>array(" RETCODE, char_dat[j].account_id);
					fprintf(fp, "\t\t\t'charname'=>'%s'," RETCODE, php_addslashes(char_dat[j].name));
					k = isGM(char_dat[j].account_id);
					fprintf(fp, "\t\t\t'isgm'=>%s," RETCODE, (k >= online_gm_display_min_level) ? "true" : "false");
					fprintf(fp, "\t\t\t'gmlevel'=>%d," RETCODE, k);
					fprintf(fp, "\t\t\t'class'=>%d," RETCODE, char_dat[j].class);
					fprintf(fp, "\t\t\t'baselvl'=>%d," RETCODE, char_dat[j].base_level);
					fprintf(fp, "\t\t\t'joblvl'=>%d," RETCODE, char_dat[j].job_level);
					fprintf(fp, "\t\t\t'map'=>'%s'," RETCODE, php_addslashes(char_dat[j].last_point.map));
					fprintf(fp, "\t\t\t'mapx'=>'%d'," RETCODE, char_dat[j].last_point.x);
					fprintf(fp, "\t\t\t'mapy'=>'%d'," RETCODE, char_dat[j].last_point.y);
					fprintf(fp, "\t\t\t'zeny'=>%d," RETCODE, char_dat[j].zeny);
					fprintf(fp, "\t\t)," RETCODE);
				}
				fprintf(fp, "\t)," RETCODE);
			} else {
				fprintf(fp, "\t'players'=>array()," RETCODE);
			}

			// Displaying number of online players
			fprintf(fp, "\t'playercount'=>%d," RETCODE, players);
			fprintf(fp, ");" RETCODE);
			fclose(fp);
		}
	}

	FREE(id);

	return;
}

//--------------------------------------------------------------------------------------
// This function return the number of online (and not hidden) players in all map-servers
//--------------------------------------------------------------------------------------
int count_users(void) {
	int i, users;

	users = 0;
	for(i = 0; i < MAX_MAP_SERVERS; i++)
		if (server_fd[i] >= 0)
			users += server[i].users;

	return users;
}

//----------------------------------------
// Function to send characters to a player
//----------------------------------------
int mmo_char_send006b(int fd, struct char_session_data *sd) {
#ifdef TXT_ONLY
	int i;
	struct mmo_charstatus *p;
#endif // TXT_ONLY
	int j, found_num;

//	printf("mmo_char_send006b start (" CL_BOLD "%d" CL_RESET ") Request Char Data:\n", sd->account_id);

	memset(WPACKETP(0), 0, 24);
	WPACKETW(0) = 0x6b;

	found_num = 0;
	memset(sd->found_char, 0, sizeof(sd->found_char)); // found_char = index in db (TXT), char_id (SQL), -1: void
#ifdef TXT_ONLY
	for(i = 0; i < char_num && found_num < 9; i++) {
		if (char_dat[i].account_id == sd->account_id) {
			p = &char_dat[i];
//			printf("(" CL_GREEN "%d" CL_RESET ")" CL_GREEN "%s" CL_RESET "\t[char]\n", p->char_id, p->name);
			sd->found_char[found_num] = i; // found_char = index in db (TXT), char_id (SQL), -1: void

			j = 24 + (found_num * 108); // increase speed of code
			memset(WPACKETP(j), 0, 108);

			WPACKETL(j    ) = p->char_id;
			WPACKETL(j+  4) = p->base_exp;
			WPACKETL(j+  8) = p->zeny;
//			printf("char #%d: char_id %d, base_exp: %d, zeny: %d\n", found_num, WPACKETL(j), WPACKETL(j+4), WPACKETL(j+8));
			WPACKETL(j+ 12) = p->job_exp;
			WPACKETL(j+ 16) = p->job_level;
//			printf("char #%d: job_exp: %d, job_level: %d\n", found_num, WPACKETL(j+12), WPACKETL(j+16));

			WPACKETL(j+ 20) = 0;
			WPACKETL(j+ 24) = 0;
			WPACKETL(j+ 28) = p->option;

			WPACKETL(j+ 32) = p->karma;
			WPACKETL(j+ 36) = p->manner;
//			printf("char #%d: opt1 %d, opt2: %d, option: %d, karma: %d, manner: %d\n", found_num, WPACKETL(j+20), WPACKETL(j+24), WPACKETL(j+28), WPACKETL(j+32), WPACKETL(j+36));

			WPACKETW(j+ 40) = p->status_point;
			WPACKETW(j+ 42) = (p->hp > 0x7fff) ? 0x7fff : p->hp;
			WPACKETW(j+ 44) = (p->max_hp > 0x7fff) ? 0x7fff : p->max_hp;
			WPACKETW(j+ 46) = (p->sp > 0x7fff) ? 0x7fff : p->sp;
			WPACKETW(j+ 48) = (p->max_sp > 0x7fff) ? 0x7fff : p->max_sp;
			WPACKETW(j+ 50) = DEFAULT_WALK_SPEED; // p->speed;
//			printf("char #%d: status_point %d, hp: %d/%d, sp: %d/%d, speed: %d\n", found_num, WPACKETW(j+40), WPACKETW(j+42), WPACKETW(j+44), WPACKETW(j+46), WPACKETW(j+48), WPACKETW(j+50));
			WPACKETW(j+ 52) = p->class;
			WPACKETW(j+ 54) = p->hair;
			/* Peco crash fix*/
			if (p->class ==   13 || p->class ==   21 ||
			    p->class == 4014 || p->class == 4022 ||
			    p->class == 4036 || p->class == 4044)
				WPACKETW(j+ 56) = 0;
			else
				WPACKETW(j+ 56) = p->weapon;
			WPACKETW(j+ 58) = p->base_level;
			WPACKETW(j+ 60) = p->skill_point;
//			printf("char #%d: class %d, hair: %d, weapon: %d, base_level: %d, skill_point: %d\n", found_num, WPACKETW(j+52), WPACKETW(j+54), WPACKETW(j+56), WPACKETW(j+58), WPACKETW(j+60));
			WPACKETW(j+ 62) = p->head_bottom;
			WPACKETW(j+ 64) = p->shield;
			WPACKETW(j+ 66) = p->head_top;
			WPACKETW(j+ 68) = p->head_mid;
//			printf("char #%d: head_bottom %d, shield: %d, head_top: %d, head_mid: %d\n", found_num, WPACKETW(j+62), WPACKETW(j+64), WPACKETW(j+66), WPACKETW(j+68));
			WPACKETW(j+ 70) = p->hair_color;
			WPACKETW(j+ 72) = p->clothes_color;

			strncpy(WPACKETP(j+74), p->name, 24);
//			printf("char #%d: hair_color: %d, clothes_color: %d, name: %s\n", found_num, WPACKETW(j+70), WPACKETW(j+72), p->name);

			WPACKETB(j+ 98) = (p->str > 255) ? 255 : p->str;
			WPACKETB(j+ 99) = (p->agi > 255) ? 255 : p->agi;
			WPACKETB(j+100) = (p->vit > 255) ? 255 : p->vit;
			WPACKETB(j+101) = (p->int_ > 255) ? 255 : p->int_;
			WPACKETB(j+102) = (p->dex > 255) ? 255 : p->dex;
			WPACKETB(j+103) = (p->luk > 255) ? 255 : p->luk;
			WPACKETW(j+104) = p->char_num;
			WPACKETW(j+106) = 1;
//			printf("char #%d: str %d, agi: %d, vit: %d, int_: %d, dex: %d, luk: %d, char_num: %d\n", found_num, WPACKETB(j+98), WPACKETB(j+99), WPACKETB(j+100), WPACKETB(j+101), WPACKETB(j+102), WPACKETB(j+103), WPACKETB(j+104));

			found_num++;
		}
	}
#else // TXT_ONLY -> USE_SQL
	sql_request("SELECT `char_id`,`base_exp`,`zeny`,`job_exp`,`job_level`,`option`,`karma`,`manner`,"
	            "`status_point`,`hp`,`max_hp`,`sp`,`max_sp`,`class`,`hair`,`weapon`,`base_level`,`skill_point`,"
	            "`head_bottom`,`shield`,`head_top`,`head_mid`,`hair_color`,`clothes_color`,"
	            "`name`,`str`,`agi`,`vit`,`int`,`dex`,`luk`,`char_num` FROM `%s` WHERE `account_id` = '%d'", char_db, sd->account_id);
	while(sql_get_row() && found_num < 9) {
//		printf("(" CL_GREEN "%d" CL_RESET ")" CL_GREEN "%s" CL_RESET "\t[char]\n", sql_get_integer(0), sql_get_string(24));
		sd->found_char[found_num] = sql_get_integer(0); // found_char = index in db (TXT), char_id (SQL), -1: void

		j = 24 + (found_num * 108); // increase speed of code
		memset(WPACKETP(j), 0, 108);

		WPACKETL(j    ) = sql_get_integer(0); // char_id
		WPACKETL(j+  4) = sql_get_integer(1); // base_exp
		WPACKETL(j+  8) = sql_get_integer(2); // zeny
//		printf("char #%d: char_id %d, base_exp: %d, zeny: %d\n", found_num, WPACKETL(j), WPACKETL(j+4), WPACKETL(j+8));
		WPACKETL(j+ 12) = sql_get_integer(3); // job_exp
		WPACKETL(j+ 16) = sql_get_integer(4); // job_level
//		printf("char #%d: job_exp: %d, job_level: %d\n", found_num, WPACKETL(j+12), WPACKETL(j+16));

		WPACKETL(j+ 20) = 0; // opt1
		WPACKETL(j+ 24) = 0; // opt2
		WPACKETL(j+ 28) = sql_get_integer(5); // option

		WPACKETL(j+ 32) = sql_get_integer(6); // karma
		WPACKETL(j+ 36) = sql_get_integer(7); // manner
//		printf("char #%d: opt1 %d, opt2: %d, option: %d, karma: %d, manner: %d\n", found_num, WPACKETL(j+20), WPACKETL(j+24), WPACKETL(j+28), WPACKETL(j+32), WPACKETL(j+36));

		WPACKETW(j+ 40) = sql_get_integer(8); // status_point
		WPACKETW(j+ 42) = (sql_get_integer( 9) > 0x7fff) ? 0x7fff : sql_get_integer( 9); // hp
		WPACKETW(j+ 44) = (sql_get_integer(10) > 0x7fff) ? 0x7fff : sql_get_integer(10); // max_hp
		WPACKETW(j+ 46) = (sql_get_integer(11) > 0x7fff) ? 0x7fff : sql_get_integer(11); // sp
		WPACKETW(j+ 48) = (sql_get_integer(12) > 0x7fff) ? 0x7fff : sql_get_integer(12); // max_sp
		WPACKETW(j+ 50) = DEFAULT_WALK_SPEED; // speed
//		printf("char #%d: status_point %d, hp: %d/%d, sp: %d/%d, speed: %d\n", found_num, WPACKETW(j+40), WPACKETW(j+42), WPACKETW(j+44), WPACKETW(j+46), WPACKETW(j+48), WPACKETW(j+50));
		WPACKETW(j+ 52) = sql_get_integer(13); // class
		WPACKETW(j+ 54) = sql_get_integer(14); // hair
		if (sql_get_integer(14) > max_hair_style) {
			WPACKETW(j+ 54) = max_hair_style; // hair
		}
		/* Peco crash fix*/
		if (sql_get_integer(13) ==   13 || sql_get_integer(13) ==   21 ||
		    sql_get_integer(13) == 4014 || sql_get_integer(13) == 4022 ||
		    sql_get_integer(13) == 4036 || sql_get_integer(13) == 4044)
			WPACKETW(j+ 56) = 0;
		else
			WPACKETW(j+ 56) = sql_get_integer(15); // weapon
		WPACKETW(j+ 58) = sql_get_integer(16); // base_level
		WPACKETW(j+ 60) = sql_get_integer(17); // skill_point
//		printf("char #%d: class %d, hair: %d, weapon: %d, base_level: %d, skill_point: %d\n", found_num, WPACKETW(j+52), WPACKETW(j+54), WPACKETW(j+56), WPACKETW(j+58), WPACKETW(j+60));
		WPACKETW(j+ 62) = sql_get_integer(18); // head_bottom
		WPACKETW(j+ 64) = sql_get_integer(19); // shield
		WPACKETW(j+ 66) = sql_get_integer(20); // head_top
		WPACKETW(j+ 68) = sql_get_integer(21); // head_mid
//		printf("char #%d: head_bottom %d, shield: %d, head_top: %d, head_mid: %d\n", found_num, WPACKETW(j+62), WPACKETW(j+64), WPACKETW(j+66), WPACKETW(j+68));
		WPACKETW(j+ 70) = sql_get_integer(22); // hair_color
		if (sql_get_integer(22) > max_hair_color) {
			WPACKETW(j+ 70) = max_hair_color;
		}
		WPACKETW(j+ 72) = sql_get_integer(23); // clothes_color

		strncpy(WPACKETP(j+74), sql_get_string(24), 24); // name
//		printf("char #%d: hair_color: %d, clothes_color: %d, name: %s\n", found_num, WPACKETW(j+70), WPACKETW(j+72), sql_get_string(24));

		WPACKETB(j+ 98) = (sql_get_integer(25) > 255) ? 255 : sql_get_integer(25); // str
		WPACKETB(j+ 99) = (sql_get_integer(26) > 255) ? 255 : sql_get_integer(26); // agi
		WPACKETB(j+100) = (sql_get_integer(27) > 255) ? 255 : sql_get_integer(27); // vit
		WPACKETB(j+101) = (sql_get_integer(28) > 255) ? 255 : sql_get_integer(28); // int_
		WPACKETB(j+102) = (sql_get_integer(29) > 255) ? 255 : sql_get_integer(29); // dex
		WPACKETB(j+103) = (sql_get_integer(30) > 255) ? 255 : sql_get_integer(30); // luk
		WPACKETW(j+104) = sql_get_integer(31); // char_num
		WPACKETW(j+106) = 1;
//		printf("char #%d: str %d, agi: %d, vit: %d, int_: %d, dex: %d, luk: %d, char_num: %d\n", found_num, WPACKETB(j+98), WPACKETB(j+99), WPACKETB(j+100), WPACKETB(j+101), WPACKETB(j+102), WPACKETB(j+103), WPACKETB(j+104));

		found_num++;
	}
#endif

//	if (found_num == 0)
//		printf("No character found.\n");

	for(j = found_num; j < 9; j++)
		sd->found_char[j] = -1; // found_char = index in db (TXT), char_id (SQL), -1: void

	WPACKETW(2) = 24 + found_num * 108;

	SENDPACKET(fd, WPACKETW(2));

	return 0;
}

// check account reg2 to removed them from memory
int check_account_reg2(int tid, unsigned int tick, int id, int data) {
	time_t actual_time; // speed up
	int i, j, acc;
	struct char_session_data *sd;

	actual_time = time(NULL) - (15 * 60); // conserv 15 min account reg2

	// for each account_reg2
	for(i = 0; i < account_reg2_db_num; i++) {
		acc = account_reg2_db[i].account_id;
		// search online player
		for(j = 0; j < char_num; j++) {
			if (char_dat[j].account_id == acc && online_chars[j] != 0) {
				// update timer
				account_reg2_db[i].update_time = time(NULL);
				break;
			}
		}

		// if player is not online on map-server
		if (j == char_num) {
			// check if player is connected to char-server
			for(j = 0; j < fd_max; j++) {
				if (session[j] && (sd = session[j]->session_data) && sd->account_id == acc) {
					// update timer
					account_reg2_db[i].update_time = time(NULL);
					break;
				}
			}

			// if player is not online, check if it must be conserved
			if (j == fd_max) {
				if (account_reg2_db[i].update_time < actual_time) { // conserv 15 min account reg2
					FREE(account_reg2_db[i].reg2);
					// delete it
					if (account_reg2_db_num > 1) {
						if (i != account_reg2_db_num - 1)
							memcpy(&account_reg2_db[i], &account_reg2_db[account_reg2_db_num - 1], sizeof(struct account_reg2_db));
						REALLOC(account_reg2_db, struct account_reg2_db, account_reg2_db_num - 1);
						account_reg2_db_num--;
					} else {
						FREE(account_reg2_db);
						account_reg2_db_num = 0;
					}
					i--; // to recheck new account_reg2_db
				}
			}
		}
	}

	return 0;
}

// send account reg2 to map-server
void send_account_reg2(int acc, int fd) { // only send when character is send to map-server -> not send void account_reg2)
	int i, size;

	for(i = 0; i < account_reg2_db_num; i++) {
		if (account_reg2_db[i].account_id == acc) { // account_reg2 is not void
			size = sizeof(struct global_reg) * account_reg2_db[i].num;
			WPACKETW(0) = 0x2b11; // send structure of account_reg2 from char-server to map-server
			WPACKETW(2) = 8 + size;
			WPACKETL(4) = acc;
			memcpy(WPACKETP(8), account_reg2_db[i].reg2, size);
			SENDPACKET(fd, 8 + size);
			break;
		}
	}

	return;
}

// ����(char�폜���Ɏg�p)
int char_divorce(struct mmo_charstatus *cs) {
	int i, j;

	if (cs == NULL)
		return 0;

	if (cs->partner_id > 0) {
		for(i = 0; i < char_num; i++) {
			if (char_dat[i].char_id == cs->partner_id && char_dat[i].partner_id == cs->char_id) {
				cs->partner_id = 0;
				char_dat[i].partner_id = 0;
				for(j = 0; j < MAX_INVENTORY; j++)
					if (char_dat[i].inventory[j].nameid == WEDDING_RING_M || char_dat[i].inventory[j].nameid == WEDDING_RING_F)
						memset(&char_dat[i].inventory[j], 0, sizeof(char_dat[i].inventory[0]));
					if (cs->inventory[j].nameid == WEDDING_RING_M || cs->inventory[j].nameid == WEDDING_RING_F)
						memset(&cs->inventory[j], 0, sizeof(cs->inventory[0]));
				return 0;
			}
		}
	}

	return 0;
}

//-----------------------------------
// Say if a player is online (or was online 15 sec before) by [Yor]
//-----------------------------------
int always_online_timer(int account_id) {
	int i;

	for(i = 0; i < char_num; i++)
		if (char_dat[i].account_id == account_id &&
		    (online_chars[i] != 0 || char_dat_timer[i] > time(NULL))) // to check online here, check visible and hidden characters
			return 1;

	return 0;
}

//----------------------------------------------------------------------
// Force disconnection of an online player (with account value) by [Yor]
//----------------------------------------------------------------------
int disconnect_player(int account_id) {
	int i;
	struct char_session_data *sd;

	// disconnect player if online on char-server
	for(i = 0; i < fd_max; i++) {
		if (session[i] && (sd = session[i]->session_data)) {
			if (sd->account_id == account_id) {
				session[i]->eof = 1;
				return 1;
			}
		}
	}

	return 0;
}

// �L�����폜�ɔ����f�[�^�폜
static int char_delete(struct mmo_charstatus *cs) {
	int j;

	// �y�b�g�폜
	if (cs->pet_id)
		inter_pet_delete(cs->pet_id);
	for (j = 0; j < MAX_INVENTORY; j++)
		if (cs->inventory[j].card[0] == (short)0xff00)
			inter_pet_delete(*((long *)(&cs->inventory[j].card[2])));
	for (j = 0; j < MAX_CART; j++)
		if (cs->cart[j].card[0] == (short)0xff00)
			inter_pet_delete(*((long *)(&cs->cart[j].card[2])));
	// �M���h�E��
	if (cs->guild_id)
		inter_guild_leave(cs->guild_id, cs->account_id, cs->char_id);
	// �p�[�e�B�[�E��
	if (cs->party_id)
		inter_party_leave(cs->party_id, cs->account_id);
	// ����
	if (cs->partner_id) {
		// ��������map�ɒʒm
		WPACKETW(0) = 0x2b12;
		WPACKETL(2) = cs->char_id;
		WPACKETL(6) = cs->partner_id;
		mapif_sendall(10);
		// ����
		char_divorce(cs);
	}

	return 0;
}

int check_connect_login_server(int tid, unsigned int tick, int id, int data);

int parse_tologin(int fd) {
	int i;
	struct char_session_data *sd;

	// only login-server can have an access to here.
	// so, if it isn't the login-server, we disconnect the session (fd != login_fd).
	if (fd != login_fd || session[fd]->eof) {
		if (fd == login_fd) {
			printf("Char-server can't connect to login-server (connection #%d).\n", fd);
			login_fd = -1;
		}
#ifdef __WIN32
		shutdown(fd, SD_BOTH);
		closesocket(fd);
#else
		close(fd);
#endif
		delete_session(fd);
		// try immediatly to reconnect if it was the login-server
//		if (login_fd == -1)
//			check_connect_login_server(0, 0, 0, 0);
		return 0;
	}

	while(RFIFOREST(fd) >= 2 && !session[fd]->eof) {
//		printf("parse_tologin: connection #%d, packet: 0x%x (with being read: %d bytes).\n", fd, RFIFOW(fd,0), RFIFOREST(fd));

		sd = session[fd]->session_data;

		switch(RFIFOW(fd,0)) {
		case 0x2711:
			if (RFIFOREST(fd) < 3)
				return 0;
			if (RFIFOB(fd,2)) {
				printf("Can not connect to login-server.\n");
				printf("The server communication passwords (default s1/p1) is probably invalid.\n");
#ifdef TXT_ONLY
				printf("Also, please make sure your accounts file (default: accounts.txt) has those values present.\n");
#endif /* TXT_ONLY */
#ifdef USE_SQL
				printf("Also, please make sure your accounts database (default: login) has those values present.\n");
#endif /* USE_SQL */
				printf("If you changed the communication passwords, change them back at map_freya.conf and char_freya.conf\n");
#ifdef __WIN32
				Sleep(2000); // 2 seconds to display message
#else
				sleep(2); // 2 seconds to display message
#endif
				exit(1);
			} else {
				printf(CL_CYAN "Successfully connected to login-server" CL_RESET " (connection " CL_WHITE "#%d" CL_RESET ").\n", fd);
				// if no map-server already connected, display a message...
				for(i = 0; i < MAX_MAP_SERVERS; i++)
					if (server_fd[i] >= 0 && server[i].map && server[i].map[0]) // server[i].map[0][0]) // if map-server online and at least 1 map
						break;
				if (i == MAX_MAP_SERVERS)
					printf("Awaiting maps from map-server.\n");
			}
			RFIFOSKIP(fd, 3);
			break;

		case 0x2713:
			if (RFIFOREST(fd) < 51)
				return 0;
//			printf("parse_tologin 2713 : %d\n", RFIFOB(fd,6));
			for(i = 0; i < fd_max; i++) {
				if (session[i] && (sd = session[i]->session_data) && sd->account_id == (int)RFIFOL(fd,2)) {
					int gm_level;
					if (RFIFOB(fd,6) != 0) {
						WPACKETW(0) = 0x6c;
						WPACKETB(2) = 0x42; // like 00 = Rejected from Server
						SENDPACKET(i, 3);
					} else if (max_connect_user == 0 ||
					           count_users() < max_connect_user ||
					           min_level_to_bypass <= (gm_level = isGM(RFIFOL(fd,2)))) { // Checks min_level_bypass to allow max_connect_user bypass [PoW]
//						if (max_connect_user == 0)
//							printf("max_connect_user (unlimited) -> accepted.\n");
//						else (count_users() < max_connect_user)
//							printf("count_users(): %d < max_connect_user (%d) -> accepted.\n", count_users(), max_connect_user);
//						else
//							printf("User limit bypass of the GM (level:%d) account '%d' accepted.\n", gm_level, RFIFOL(fd,2));
						// check duplicated connection
						if (always_online_timer(sd->account_id)) {
							WPACKETW(0) = 0x81;
							WPACKETB(2) = 8; // 08 = Server still recognises your last login
							SENDPACKET(i, 3);
							sd->auth = 0; // 0: not already authentified, 1: authentified
							WPACKETW(0) = 0x2b07;
							WPACKETL(2) = sd->account_id;
							mapif_sendall(6);
						} else {
							memset(sd->email, 0, sizeof(sd->email)); // 40 + NULL
							strncpy(sd->email, RFIFOP(fd, 7), 40);
							if (e_mail_check(sd->email) == 0) {
								memset(sd->email, 0, sizeof(sd->email));
								strncpy(sd->email, "a@a.com", 40); // default e-mail
							}
							sd->connect_until_time = (time_t)RFIFOL(fd,47);
							// send characters to player
							mmo_char_send006b(i, sd);
							sd->auth = 1; // 0: not already authentified, 1: authentified
						}
					} else {
						// refuse connection: too much online players
//						printf("count_users(): %d < max_connect_use (%d) (and no bypass) -> fail...\n", count_users(), max_connect_user);
						WPACKETW(0) = 0x6c;
						WPACKETB(2) = 0; // 00 = Rejected from Server
						SENDPACKET(i, 3);
					}
					break;
				}
			}
			RFIFOSKIP(fd,51);
			break;

		// Receiving of an e-mail/time limit from the login-server (answer of a request because a player comes back from map-server to char-server) by [Yor]
		case 0x2717:
			if (RFIFOREST(fd) < 50)
				return 0;
		  {
			int acc;
			acc = (int)RFIFOL(fd,2); // speed up
			for(i = 0; i < fd_max; i++) {
				if (session[i] && (sd = session[i]->session_data)) {
					if (sd->account_id == acc) {
						memset(sd->email, 0, sizeof(sd->email)); // 40 + NULL
						strncpy(sd->email, RFIFOP(fd,6), 40);
						if (e_mail_check(sd->email) == 0) {
							memset(sd->email, 0, sizeof(sd->email));
							strncpy(sd->email, "a@a.com", 40); // default e-mail
						}
						sd->connect_until_time = (time_t)RFIFOL(fd,46);
						break;
					}
				}
			}
		  }
			RFIFOSKIP(fd,50);
			break;

		// alive packet of login-server: to check connection by [Yor]
		case 0x2718:
			RFIFOSKIP(fd,2);
			break;

		// Receiving authentification from login server (to avoid char->login->char)
		case 0x2719:
			if (RFIFOREST(fd) < 19)
				return 0;
			// to conserv a maximum of authentification, search if account is already authentified and replace it
			// that will reduce multiple connection too
			for(i = 0; i < AUTH_FIFO_SIZE; i++)
				if (auth_fifo[i].account_id == (int)RFIFOL(fd,2))
					break;
			// if not found, use next value
			if (i == AUTH_FIFO_SIZE) {
				if (auth_fifo_pos >= AUTH_FIFO_SIZE)
					auth_fifo_pos = 0;
				i = auth_fifo_pos;
				auth_fifo_pos++;
			}
			//printf("auth_fifo set (auth #%d) - account: %d, secure: %08x-%08x\n", i, RFIFOL(fd,2), RFIFOL(fd,6), RFIFOL(fd,10));
			auth_fifo[i].account_id = RFIFOL(fd,2);
			auth_fifo[i].char_id = 0;
			auth_fifo[i].login_id1 = RFIFOL(fd,6);
			auth_fifo[i].login_id2 = RFIFOL(fd,10);
			auth_fifo[i].delflag = 2; // 0: auth_fifo canceled/void, 2: auth_fifo received from login/map server in memory, 1: connection authentified
			auth_fifo[i].char_pos = 0;
			auth_fifo[i].sex = RFIFOB(fd,18);
			auth_fifo[i].connect_until_time = 0; // unlimited/unknown time by default (not display in map-server)
			auth_fifo[i].ip = RFIFOL(fd,14);
			auth_fifo[i].map_auth = 0;
			RFIFOSKIP(fd,19);
			break;

		// Receiving online accounts of other char-servers (to avoid multiple connection)
		case 0x271a:
			if (RFIFOREST(fd) < 6 || RFIFOREST(fd) < RFIFOW(fd,2))
				return 0;
		  {
			int j, acc;
			for (j = 6; j < RFIFOW(fd,2); j = j + 4) {
				acc = (int)RFIFOL(fd,j);
				// check if someone if already connected with same id
				for(i = 0; i < fd_max; i++) {
					struct char_session_data *sd2;
					if (session[i] && (sd2 = session[i]->session_data) && sd2->account_id == acc) {
						WPACKETW(0) = 0x81;
						WPACKETB(2) = 2; // 02 = Someone else is logged in on this ID
						SENDPACKET(i, 3);
						sd2->auth = 0; // 0: not already authentified, 1: authentified
						// no break... multiple connections
					}
				}
				// Check people on map servers
				for(i = 0; i < char_num; i++)
					if (online_chars[i] != 0 && char_dat[i].account_id == acc) {
						// send information to all map-servers for disconnection
						WPACKETW(0) = 0x2b07;
						WPACKETL(2) = acc;
						mapif_sendall(6);
						break;
					}
			}
		  }
			RFIFOSKIP(fd,RFIFOW(fd,2));
			break;

		case 0x2721:	// gm reply
			if (RFIFOREST(fd) < 8)
				return 0;
			WPACKETW(0) = 0x2b0b;
			WPACKETL(2) = RFIFOL(fd,2); // account
			WPACKETW(6) = RFIFOW(fd,6); // GM level
			mapif_sendall(8);
//			printf("parse_tologin: To become GM answer: char -> map.\n");
			RFIFOSKIP(fd,8);
			break;

		case 0x2723:	// changesex reply (modified by [Yor]) // 0x2723 <account_id>.L <sex>.B <account_id_of_GM>.L (sex = -1 -> failed; account_id_of_GM = -1 -> ladmin)
			if (RFIFOREST(fd) < 11)
				return 0;
		  {
			int acc, sex, j;
			acc = RFIFOL(fd,2);
			sex = RFIFOB(fd,6);
			if (acc > 0 && sex != -1 && sex != 255) { // sex == -1 -> not found
				for (i = 0; i < char_num; i++) {
					if (char_dat[i].account_id == acc) {
						int jobclass = char_dat[i].class;
						char_dat[i].sex = sex;
						auth_fifo[i].sex = sex;
						if (jobclass == 19 || jobclass == 20 ||
						    jobclass == 4020 || jobclass == 4021 ||
						    jobclass == 4042 || jobclass == 4043) {
							// job modification
							if (jobclass == 19 || jobclass == 20) {
								char_dat[i].class = (sex) ? 19 : 20;
							} else if (jobclass == 4020 || jobclass == 4021) {
								char_dat[i].class = (sex) ? 4020 : 4021;
							} else if (jobclass == 4042 || jobclass == 4043) {
								char_dat[i].class = (sex) ? 4042 : 4043;
							}
							// remove specifical skills of classes 19, 4020 and 4042
							for(j = 315; j <= 322; j++) {
								if (char_dat[i].skill[j].id > 0 && char_dat[i].skill[j].flag != 1 && char_dat[i].skill[j].flag != 13) { // flag: 0 (normal), 1 (only card), 2-12 (card and skill (skill level +2)), 13 (cloneskill)
									if (char_dat[i].skill[j].flag >= 2) {
										char_dat[i].skill_point += (char_dat[i].skill[j].flag - 2);
										char_dat[i].skill[j].flag = 1;
										char_dat[i].skill[j].lv = 1;
									} else {
										char_dat[i].skill_point += char_dat[i].skill[j].lv;
										char_dat[i].skill[j].id = 0;
										char_dat[i].skill[j].lv = 0;
									}
								}
							}
							// remove specifical skills of classes 20, 4021 and 4043
							for(j = 323; j <= 330; j++) {
								if (char_dat[i].skill[j].id > 0 && char_dat[i].skill[j].flag != 1 && char_dat[i].skill[j].flag != 13) { // flag: 0 (normal), 1 (only card), 2-12 (card and skill (skill level +2)), 13 (cloneskill)
									if (char_dat[i].skill[j].flag >= 2) {
										char_dat[i].skill_point += (char_dat[i].skill[j].flag - 2);
										char_dat[i].skill[j].flag = 1;
										char_dat[i].skill[j].lv = 1;
									} else {
										char_dat[i].skill_point += char_dat[i].skill[j].lv;
										char_dat[i].skill[j].id = 0;
										char_dat[i].skill[j].lv = 0;
									}
								}
							}
						}
						// to avoid any problem with equipment and invalid sex, equipment is unequiped.
						for (j = 0; j < MAX_INVENTORY; j++) {
							if (char_dat[i].inventory[j].nameid && char_dat[i].inventory[j].equip)
								char_dat[i].inventory[j].equip = 0;
						}
						char_dat[i].weapon = 0;
						char_dat[i].shield = 0;
						char_dat[i].head_top = 0;
						char_dat[i].head_mid = 0;
						char_dat[i].head_bottom = 0;
					}
				}
				// disconnect player if online on char-server
				disconnect_player(acc);
			}
			WPACKETW(0) = 0x2b0d; // 0x2b0d <account_id>.L <sex>.B <account_id_of_GM>.L (sex = -1 -> failed; account_id_of_GM = -1 -> ladmin)
			WPACKETL(2) = acc;
			WPACKETB(6) = sex; // sex == -1 -> not found
			WPACKETL(7) = RFIFOL(fd,7); // who want do operation: -1, ladmin. other: account_id of GM
			mapif_sendall(11);
		  }
			RFIFOSKIP(fd, 11);
			break;

		case 0x2726:	// Request to send a broadcast message (no answer)
			if (RFIFOREST(fd) < 6 || RFIFOREST(fd) < RFIFOW(fd,2))
				return 0;
			if (RFIFOW(fd,2) < 7)
				char_log("Receiving a message for broadcast, but message is void." RETCODE);
			else {
				// at least 1 map-server
				for(i = 0; i < MAX_MAP_SERVERS; i++)
					if (server_fd[i] >= 0 && server[i].map && server[i].map[0])
						break;
				if (i == MAX_MAP_SERVERS)
					char_log("'ladmin': Receiving a message for broadcast, but no map-server is online." RETCODE);
				else {
					char message[RFIFOW(fd,2) - 6 + 1]; // +1 to add a null terminated if not exist in the packet
					int lp;
					char *p;
					memset(message, 0, sizeof(message));
					memcpy(message, RFIFOP(fd,6), RFIFOW(fd,2) - 6);
					message[sizeof(message)-1] = '\0';
					remove_control_chars(message);
					// remove all first spaces
					p = message;
					while(p[0] == ' ')
						p++;
					// if message is only composed of spaces
					if (p[0] == '\0')
						char_log("Receiving a message for broadcast, but message is only a lot of spaces." RETCODE);
					// else send message to all map-servers
					else {
						if (RFIFOW(fd,4) == 0) {
							char_log("'ladmin': Receiving a message for broadcast (message (in yellow): %s)" RETCODE, message);
							lp = 4;
						} else {
							char_log("'ladmin': Receiving a message for broadcast (message (in blue): %s)" RETCODE, message);
							lp = 8;
						}
						// split message to max 80 char
						while(p[0] != '\0') { // if not finish
							if (p[0] == ' ') // jump if first char is a space
								p++;
							else {
								char split[80];
								char* last_space;
								memset(split, 0, sizeof(split));
								sscanf(p, "%79[^\t]", split); // max 79 char, any char (\t is control char and control char was removed before)
								split[sizeof(split)-1] = '\0'; // last char always \0
								if (strlen(split) != strlen(p) && (last_space = strrchr(split, ' ')) != NULL) { // searching space from end of the string
									last_space[0] = '\0'; // replace it by NULL to have correct length of split
									p++; // to jump the new NULL
								}
								p += strlen(split);
								// send broadcast to all map-servers
								WPACKETW(0) = 0x3800; // 0x3000/0x3800 <packet_len>.w <message>.?B
								WPACKETW(2) = lp + strlen(split) + 1;
								WPACKETL(4) = 0x65756c62; // only write if in blue (lp = 8) // eulb
								memcpy(WPACKETP(lp), split, strlen(split) + 1); // +1 to copy NULL too (because short mesage can have part of the 'blue')
								mapif_sendall(WPACKETW(2));
							}
						}
					}
				}
			}
			RFIFOSKIP(fd,RFIFOW(fd,2));
			break;

		// receiving account_reg2 from login-server
		case 0x2729:
			if (RFIFOREST(fd) < 8 || RFIFOREST(fd) < RFIFOW(fd,2))
				return 0;
		  {
			int j, p, acc;
			acc = RFIFOL(fd,4);
			// if some values to conserv
			if (RFIFOW(fd,2) > 8) {
				for(i = 0; i < account_reg2_db_num; i++) {
					// if found, resize actual structure
					if (account_reg2_db[i].account_id == acc) {
						account_reg2_db[i].update_time = time(NULL);
						if (account_reg2_db[i].num != (RFIFOW(fd,2) - 8) / 36) {
							account_reg2_db[i].num = (RFIFOW(fd,2) - 8) / 36;
							REALLOC(account_reg2_db[i].reg2, struct global_reg, account_reg2_db[i].num);
						}
						break;
					}
				}
				// if not found, create a new structure
				if (i == account_reg2_db_num) {
					if (account_reg2_db_num == 0) {
						MALLOC(account_reg2_db, struct account_reg2_db, 1);
					} else {
						REALLOC(account_reg2_db, struct account_reg2_db, account_reg2_db_num + 1);
					}
					account_reg2_db[i].account_id = acc;
					account_reg2_db[i].update_time = time(NULL);
					account_reg2_db[i].num = (RFIFOW(fd,2) - 8) / 36;
					MALLOC(account_reg2_db[i].reg2, struct global_reg, account_reg2_db[i].num);
					account_reg2_db_num++;
				}
				// save reg2
				j = 0;
				for(p = 8; p < RFIFOW(fd,2); p += 36) {
					strncpy(account_reg2_db[i].reg2[j].str, RFIFOP(fd,p), 32);
					account_reg2_db[i].reg2[j].str[32] = '\0';
					account_reg2_db[i].reg2[j].value = RFIFOL(fd,p + 32);
					j++;
				}
				// Send informations to all map-servers
				p = sizeof(struct global_reg) * account_reg2_db[i].num;
				WPACKETW(0) = 0x2b11; // send structure of account_reg2 from char-server to map-server
				WPACKETW(2) = 8 + p;
				WPACKETL(4) = acc;
				memcpy(WPACKETP(8), account_reg2_db[i].reg2, p);
				mapif_sendall(8 + p);
			// no value to save
			} else {
				for(i = 0; i < account_reg2_db_num; i++) {
					if (account_reg2_db[i].account_id == acc) {
						// if account reg2 exist
						FREE(account_reg2_db[i].reg2);
						// delete it
						if (account_reg2_db_num > 1) {
							if (i != account_reg2_db_num - 1)
								memcpy(&account_reg2_db[i], &account_reg2_db[account_reg2_db_num - 1], sizeof(struct account_reg2_db));
							REALLOC(account_reg2_db, struct account_reg2_db, account_reg2_db_num - 1);
							account_reg2_db_num--;
						} else {
							FREE(account_reg2_db);
							account_reg2_db_num = 0;
						}
						break;
					}
				}
				// Send informations to all map-servers
				WPACKETW(0) = 0x2b11; // send structure of account_reg2 from char-server to map-server
				WPACKETW(2) = 8;
				WPACKETL(4) = acc;
				mapif_sendall(8);
			}
//			printf("char: save_account_reg2_reply\n");
			RFIFOSKIP(fd, RFIFOW(fd,2));
		  }
			break;

		// Email modification answer from login-server (map->char->login->char - now -> map)
		case 0x272b: // 0x272b/0x2b18 <account_id>.L <new_e-mail>.40B
			if (RFIFOREST(fd) < 46)
				return 0;
			WPACKETW(0) = 0x2b18; // 0x272b/0x2b18 <account_id>.L <new_e-mail>.40B
			memcpy(WPACKETP(2), RFIFOP(fd,2), 46 - 2);
			mapif_sendall(46);
			RFIFOSKIP(fd, 46);
			break;

		// ACK about account_reg2 saving (send back to all map-servers; we don't know which map-server has send the save)
		case 0x272c: // 0x272c <account_id>.L
			if (RFIFOREST(fd) < 6)
				return 0;
			WPACKETW(0) = 0x2b19; // 0x2b19 <account_id>.L
			WPACKETL(2) = RFIFOL(fd,2);
			mapif_sendall(6);
			RFIFOSKIP(fd, 6);
			break;

		// Password modification answer from login-server (map->char->login->char - now -> map)
		case 0x272e: // 0x272e <account_id>.L <new_password>.32B
			if (RFIFOREST(fd) < 38)
				return 0;
			WPACKETW(0) = 0x2b1e; // 0x272e/0x2b1e <account_id>.L <new_password>.32B
			memcpy(WPACKETP(2), RFIFOP(fd,2), 38 - 2);
			mapif_sendall(38);
			RFIFOSKIP(fd, 38);
			break;

		// Answer of login-server after a request to change a gm level from a map-server
		case 0x272f: // 0x272f/0x2b21 <account_id>.L <GM_level>.B <accound_id_of_GM>.L (GM_level = -1 -> player not found, -2: gm level doesn't authorise you, -3: already right value; account_id_of_GM = -1 -> script)
			if (RFIFOREST(fd) < 11)
				return 0;
			// transmit answer to all map-servers
			WPACKETW(0) = 0x2b21; // 0x272f/0x2b21 <account_id>.L <GM_level>.B <accound_id_of_GM>.L (GM_level = -1 -> player not found, -2: gm level doesn't authorise you, -3: already right value; account_id_of_GM = -1 -> script)
			memcpy(WPACKETP(2), RFIFOP(fd,2), 11 - 2);
			mapif_sendall(11);
			RFIFOSKIP(fd, 11);
			break;

		// Account deletion notification (from login-server)
		case 0x2730:
			if (RFIFOREST(fd) < 6)
				return 0;
			// Deletion of all characters of the account
			for(i = 0; i < char_num; i++) {
				if (char_dat[i].account_id == RFIFOL(fd,2)) {
					char_delete(&char_dat[i]);
					if (i < char_num - 1) {
						memcpy(&char_dat[i], &char_dat[char_num - 1], sizeof(struct mmo_charstatus));
						FREE(friend_dat[i].friends);
						friend_dat[i].friend_num = friend_dat[char_num - 1].friend_num;
						friend_dat[i].friends    = friend_dat[char_num - 1].friends;
						friend_dat[char_num - 1].friend_num = 0;
						friend_dat[char_num - 1].friends    = NULL;
						FREE(global_reg_db[i].global_reg);
						global_reg_db[i].global_reg_num = global_reg_db[char_num - 1].global_reg_num;
						global_reg_db[i].global_reg     = global_reg_db[char_num - 1].global_reg;
						global_reg_db[char_num - 1].global_reg_num = 0;
						global_reg_db[char_num - 1].global_reg     = NULL;
						char_dat_timer[i] = char_dat_timer[char_num - 1];
						online_chars[i] = online_chars[char_num - 1];
						// if moved character owns to deleted account, check again it's character
						if (char_dat[i].account_id == RFIFOL(fd,2)) {
							i--;
						// Correct moved character reference in the character's owner by [Yor]
						} else {
							int j, k;
							struct char_session_data *sd2;
							for (j = 0; j < fd_max; j++) {
								if (session[j] && (sd2 = session[j]->session_data) &&
								    sd2->account_id == char_dat[char_num - 1].account_id) {
									for (k = 0; k < 9; k++) {
										if (sd2->found_char[k] == char_num - 1) {
											sd2->found_char[k] = i; // found_char = index in db (TXT), char_id (SQL), -1: void
											break;
										}
									}
									break;
								}
							}
						}
					}
					char_num--;
				}
			}
			// Deletion of the storage
			inter_storage_delete(RFIFOL(fd,2));
			// send to all map-servers to disconnect the player
			WPACKETW(0) = 0x2b13;
			WPACKETL(2) = RFIFOL(fd,2);
			mapif_sendall(6);
			// disconnect player if online on char-server
			disconnect_player(RFIFOL(fd,2));
			RFIFOSKIP(fd,6);
			break;

		// State change of account/ban notification (from login-server) by [Yor]
		case 0x2731:
			if (RFIFOREST(fd) < 11)
				return 0;
			// send to all map-servers to disconnect the player
			WPACKETW(0) = 0x2b14;
			WPACKETL(2) = RFIFOL(fd,2);
			WPACKETB(6) = RFIFOB(fd,6); // 0: change of statut, 1: ban
			WPACKETL(7) = RFIFOL(fd,7); // status or final date of a banishment
			mapif_sendall(11);
			// disconnect player if online on char-server
			disconnect_player(RFIFOL(fd,2));
			RFIFOSKIP(fd,11);
			break;

		/* Receiving a new connection of an account from the login-server */
		case 0x2732:
			if (RFIFOREST(fd) < 6)
				return 0;
			// check if someone if already connected with same id
			for(i = 0; i < fd_max; i++) {
				struct char_session_data *sd2;
				if (session[i] && (sd2 = session[i]->session_data) && sd2->account_id == (int)RFIFOL(fd,2)) {
					WPACKETW(0) = 0x81;
					WPACKETB(2) = 2; // 02 = Someone else is logged in on this ID
					SENDPACKET(i, 3);
					sd2->auth = 0; // 0: not already authentified, 1: authentified
					// no break... multiple connections
				}
			}
			/* send information to all map-servers */
			WPACKETW(0) = 0x2b07;
			WPACKETL(2) = RFIFOL(fd,2);
			mapif_sendall(6);
			RFIFOSKIP(fd,6);
			break;

		/* Receiving a GM account info from login-server (by [Yor]) */
		case 0x2733:
			if (RFIFOREST(fd) < 7)
				return 0;
		  {
			int new_level = 0;
			for(i = 0; i < GM_num; i++)
				if (gm_account[i].account_id == (int)RFIFOL(fd,2)) {
					if (gm_account[i].level != RFIFOB(fd,6)) {
						gm_account[i].level = RFIFOB(fd,6);
						new_level = 1;
					}
					break;
				}
			// if not found, add it
			if (i == GM_num) {
				if (((int)RFIFOB(fd,6)) > 0) {
					if (GM_num == 0) {
						MALLOC(gm_account, struct gm_account, 1);
					} else {
						REALLOC(gm_account, struct gm_account, GM_num + 1);
					}
					gm_account[GM_num].account_id = RFIFOL(fd,2);
					gm_account[GM_num].level = RFIFOB(fd,6);
					new_level = 1;
					GM_num++;
				}
			}
			if (new_level == 1) {
				printf("From login-server: receiving a GM account information (%d: level %d).\n", RFIFOL(fd,2), (int)RFIFOB(fd,6));
				char_log("From login-server: receiving a GM account information (%d: level %d)." RETCODE, RFIFOL(fd,2), (int)RFIFOB(fd,6));
			}
			//create_online_files(); // not change online file for only 1 player (in next timer, that will be done
			// send gm acccounts level to map-servers to update value if necessary (send anyway to avoid error when char-server was crashed)
			WPACKETW(0) = 0x2b1f; // 0x2b1f <account_id>.L <GM_Level>.B
			WPACKETL(2) = RFIFOL(fd,2);
			WPACKETB(6) = RFIFOB(fd,6);
			mapif_sendall(7);
		  }
			RFIFOSKIP(fd,7);
			break;

		default:
			session[fd]->eof = 1;
			return 0;
		}
	}

	return 0;
}

int send_users_tologin(int tid, unsigned int tick, int id, int data) {
	int users = count_users();
	int i;
	struct char_session_data *sd;

	// send number of players to all map-servers
	WPACKETW(0) = 0x2b00;
	WPACKETL(2) = users;
	mapif_sendall(6);

	// send number of user to login server
	WPACKETW(0) = 0x2714;
	WPACKETW(4) = users;
	users = 0; // can be different to count_users() that doesn't count hidden people
	// send people online in map-servers
	for(i = 0; i < char_num; i++)
		if (online_chars[i] != 0) {
			WPACKETL(6 + users * 4) = char_dat[i].account_id;
			users++;
		}
	// send people online in this server
	for (i = 0; i < fd_max; i++)
		if (session[i] && (sd = session[i]->session_data)) {
			WPACKETL(6 + users * 4) = sd->account_id;
			users++;
		}
	WPACKETW(2) = 6 + users * 4;
	SENDPACKET(login_fd, 6 + users * 4);

	return 0;
}

//--------------------------------
// Map-server anti-freeze system
//--------------------------------
int map_anti_freeze_system(int tid, unsigned int tick, int id, int data) {
	int i;
	int users = count_users();

	//printf("[%d] Entering in map_anti_freeze_system function to check freeze of servers.\n", tick);
	for(i = 0; i < MAX_MAP_SERVERS; i++) {
		if (server_fd[i] >= 0) { // if map-server is online
			//printf("map_anti_freeze_system: server #%d, flag: %d.\n", i, server_freezeflag[i]);
			if (anti_freeze_interval != 0 && server_freezeflag[i]-- < 1) { // Map-server anti-freeze system. Counter. 6 ok, 5...0 frozen
				printf("Anti-freeze system: Map-server #%d is frozen.\n", i);
				printf("              It hasn't sended informations since 50 seconds -> disconnection.\n");
				char_log("Anti-freeze system: Map-server #%d is frozen -> disconnection." RETCODE, i);
				session[server_fd[i]]->eof = 1;
			} else {
				// send number of users to check connection
				WPACKETW(0) = 0x2b00;
				WPACKETL(2) = users;
				SENDPACKET(server_fd[i], 6);
			}
		}
	}

	return 0;
}

int parse_frommap(int fd) {
	int i, j;
	int id;

	for(id = 0; id < MAX_MAP_SERVERS; id++)
		if (server_fd[id] == fd)
			break;
	if (id == MAX_MAP_SERVERS || session[fd]->eof) {
		if (id < MAX_MAP_SERVERS) {
			printf("Map-server %d (session #%d) has disconnected.\n", id, fd);
			FREE(server[id].map);
			memset(&server[id], 0, sizeof(struct mmo_map_server));
#ifdef USE_SQL
			sql_request("DELETE FROM `ragsrvinfo` WHERE `index`='%d'", server_fd[id]);
#endif /* USE_SQL */
			server_fd[id] = -1;
#ifdef USE_SQL
			sql_request("UPDATE `%s` SET `online`=0 WHERE `online`=%d OR `online`=%d", char_db, id + 1, -(id + 1));
#endif /* USE_SQL */
			for(j = 0; j < char_num; j++)
				if (online_chars[j] == id + 1 || online_chars[j] == -(id + 1)) {
					online_chars[j] = 0;
					char_dat_timer[j] = 0; /* must be get timer for char_dat_timer ? */
				}
			send_users_tologin(0, 0, 0, 0); // update number of players to login-server and map-servers
			create_online_files(); // update online players files (to remove all online players of this server)
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

	while(RFIFOREST(fd) >= 2 && !session[fd]->eof) {
//		printf("parse_frommap: connection #%d, packet: 0x%x (with being read: %d bytes).\n", fd, RFIFOW(fd,0), RFIFOREST(fd));

		switch(RFIFOW(fd,0)) {

		/* request from map-server to reload GM accounts. Transmission to login-server (by Yor) */
		case 0x2af7: // 0x2af7 (map->char) -> 0x2709 (char->login)
			if (RFIFOREST(fd) < 6 || RFIFOREST(fd) < RFIFOW(fd,2))
				return 0;
			server_freezeflag[id] = anti_freeze_counter; // Map anti-freeze system. Counter. 6 ok, 5...0 frozen
			if (login_fd == -1) // try connection to login server if not yet done
				check_connect_login_server(0, 0, 0, 0);
			if (login_fd > 0) { // don't send request if no login-server
				WPACKETW(0) = 0x2709;
				memcpy(WPACKETP(2), RFIFOP(fd,2), RFIFOW(fd,2)-2);
				SENDPACKET(login_fd, WPACKETW(2));
			}
			RFIFOSKIP(fd,RFIFOW(fd,2));
			break;

		// Receiving map names list from the map-server
		case 0x2afa:
			if (RFIFOREST(fd) < 4 || RFIFOREST(fd) < RFIFOW(fd,2))
				return 0;
			server_freezeflag[id] = anti_freeze_counter; // Map anti-freeze system. Counter. 6 ok, 5...0 frozen
			FREE(server[id].map);
			server[id].map_num = (RFIFOW(fd,2) - 4) / 16; // MAX_MAP_PER_SERVER
			if (server[id].map_num > 0) // MAX_MAP_PER_SERVER
				CALLOC(server[id].map, char, server[id].map_num * 17);
			j = 0;
			for(i = 4; i < RFIFOW(fd,2); i += 16) {
				// if it's a name
				if (RFIFOP(fd,i)[0]) {
					int k, m;
					// get map name
					strncpy(server[id].map + j * 17, RFIFOP(fd,i), 16);
					// searching if map is not on another map-server
					for(k = 0; k < MAX_MAP_SERVERS; k++) {
						if (k == id)
							continue;
						if (server_fd[k] >= 0) { // if map-server online
							for(m = 0; m < server[k].map_num; m++) // MAX_MAP_PER_SERVER
								if (server[k].map[m * 17] && strcmp(server[k].map + (m * 17), server[id].map + (j * 17)) == 0) {
									printf("Map '%s' from server #%d already on server #%d -> not loaded.\n", server[id].map + (j * 17), id, k);
									memset(server[id].map + (j * 17), 0, 16); // reset map name
									break;
								}
							if (m != server[k].map_num) // MAX_MAP_PER_SERVER
								break;
						}
					}
					// if not found in another map-server
					if (k == MAX_MAP_SERVERS) {
//						printf("set map %d.%d: %s\n", id, j, server[id].map + (j * 17));
						j++;
					}
				} else
					memset(server[id].map + j * 17, 0, 16);
			}
		  {
			unsigned char *p = (unsigned char *)&server[id].ip;
			printf(CL_CYAN "Map-Server %d connected" CL_RESET ": " CL_WHITE "%d maps, from IP %d.%d.%d.%d port %hu" CL_RESET ".\n", id, j, p[0], p[1], p[2], p[3], server[id].port);
			printf("Map-server %d loading complete.\n", id);
			char_log("Map-Server %d connected: %d maps, from IP %d.%d.%d.%d port %hu. Map-server %d loading complete." RETCODE,
			         id, j, p[0], p[1], p[2], p[3], server[id].port, id);
		  }
			send_alone_map_server_flag(); // send flag 'map_is_alone' to all map-servers
			WPACKETW(0) = 0x2afb;
			strncpy(WPACKETP(2), wisp_server_name, 24); // name for wisp to player
			WPACKETL(26) = server_char_id;
			SENDPACKET(fd, 30);
		  {
			int x;
			if (j == 0) {
				printf(CL_YELLOW "WARNING: Map-Server %d has NO map" CL_RESET ".\n", id);
				char_log("WARNING: Map-Server %d has NO map." RETCODE, id);
				// clean up map list
				FREE(server[id].map);
				server[id].map_num = 0; // MAX_MAP_PER_SERVER
			} else {
				// exchange map informations only if ALL maps were read
				if (j == server[id].map_num) {
//					printf("Check... no map removed.\n");
					// Transmitting maps information to the other map-servers
					WPACKETW(0) = 0x2b04;
					WPACKETW(2) = j * 16 + 10;
					WPACKETL(4) = server[id].ip;
					WPACKETW(8) = server[id].port;
					for(i = 0; i < j; i++) {
						strncpy(WPACKETP(10 + i * 16), server[id].map + (i * 17), 16);
					}
					mapif_sendallwos(fd, WPACKETW(2));
					// Transmitting the maps of the other map-servers to the new map-server (note: send information of others servers if we can connect to the map-server (at least 1 map))
					for(x = 0; x < MAX_MAP_SERVERS; x++) {
						if (server_fd[x] >= 0 && x != id) {
							j = 0;
							for(i = 0; i < server[x].map_num; i++) // MAX_MAP_PER_SERVER
								if (server[x].map[i * 17]) {
									strncpy(WPACKETP(10 + j * 16), server[x].map + (i * 17), 16);
									j++;
								}
							if (j > 0) {
								WPACKETW(0) = 0x2b04;
								WPACKETW(2) = j * 16 + 10;
								WPACKETL(4) = server[x].ip;
								WPACKETW(8) = server[x].port;
								SENDPACKET(fd, WPACKETW(2));
							}
						}
					}
					// check start_point
					j = 0;
					for(x = 0; x < MAX_MAP_SERVERS; x++) {
						if (server_fd[x] >= 0) {
							for(i = 0; i < server[x].map_num; i++) // MAX_MAP_PER_SERVER
								if (server[x].map[i * 17] && strcasecmp(server[x].map + (i * 17), start_point.map) == 0) {
									j = 1;
									break;
								}
						}
					}
					if (!j) {
						printf(CL_YELLOW "WARNING: start point map (%s) doesn't exist" CL_RESET " in actual map-servers. ", start_point.map);
						printf("If you have multi map-servers, perhaps start point map is on not yet connected a map-server.\n");
					}
				// not same number of map
				} else {
					printf(CL_YELLOW "WARNING: Map-Server %d has %d valid (duplicated?) maps on %d -> no map loaded" CL_RESET ".\n", id, j, server[id].map_num);
					char_log("WARNING: Map-Server %d has %d valid (duplicated?) maps on %d -> no map loaded" RETCODE, id, j, server[id].map_num);
					// clean up map list
					FREE(server[id].map);
					server[id].map_num = 0; // MAX_MAP_PER_SERVER
				}
			}
		  }
			RFIFOSKIP(fd,RFIFOW(fd,2));
			break;

		// auth request, then send character data to map-server
		case 0x2afc:
			if (RFIFOREST(fd) < 23)
				return 0;
			server_freezeflag[id] = anti_freeze_counter; // Map anti-freeze system. Counter. 6 ok, 5...0 frozen
			//printf("auth_fifo search: account: %d, char: %d, secure: %08x-%08x\n", RFIFOL(fd,2), RFIFOL(fd,6), RFIFOL(fd,10), RFIFOL(fd,14));
			for(i = 0; i < AUTH_FIFO_SIZE; i++) {
				if (auth_fifo[i].account_id == (int)RFIFOL(fd,2) &&
				    auth_fifo[i].char_id == (int)RFIFOL(fd,6) &&
				    auth_fifo[i].login_id1 == (int)RFIFOL(fd,10) &&
				    auth_fifo[i].sex == RFIFOB(fd,22) &&
				// here, it's the only area where it's possible that we doesn't know login_id2 (map-server asks just after 0x72 packet, that doesn't given the value)
				    (!check_authfifo_login2 || auth_fifo[i].login_id2 == (int)RFIFOL(fd,14) || RFIFOL(fd,14) == 0) && // relate to the versions higher than 18
				    (!check_ip_flag || auth_fifo[i].ip == RFIFOL(fd,18)) &&
				    !auth_fifo[i].delflag) { // 0: auth_fifo canceled/void, 2: auth_fifo received from login/map server in memory, 1: connection authentified
					int idx;
					auth_fifo[i].delflag = 1; // 0: auth_fifo canceled/void, 2: auth_fifo received from login/map server in memory, 1: connection authentified
#ifdef USE_SQL
					idx = mmo_char_fromsql(auth_fifo[i].char_id); // return -1 if not found, index in char_dat[] if found
					if (idx == -1) {
						i = AUTH_FIFO_SIZE;
						break;
					}
#else // USE_SQL -> TXT_ONLY
					idx = auth_fifo[i].char_pos;
#endif // TXT_ONLY
					WPACKETW( 0) = 0x2afd;
					WPACKETW( 2) = 12 + sizeof(struct mmo_charstatus);
					WPACKETL( 4) = RFIFOL(fd,2); // auth_fifo[i].account_id
					WPACKETL( 8) = auth_fifo[i].login_id2;
					char_dat[idx].sex = auth_fifo[i].sex;
					memcpy(WPACKETP(12), &char_dat[idx], sizeof(struct mmo_charstatus));
					SENDPACKET(fd, 12 + sizeof(struct mmo_charstatus));
					// send global_reg of the player
					chrif_send_global_reg(idx, fd); // index,server_fd.
					// send storage of the player too
					mapif_get_storage(fd, RFIFOL(fd,2));
					// send account_reg of the player too
					mapif_parse_AccRegRequest(fd, RFIFOL(fd,2));
					// send account_reg2 of the player too
					send_account_reg2(fd, RFIFOL(fd,2));
					// send: we can use the character now, all informations sended
					WPACKETW(0) = 0x2b26; // 0x2b26 <account_id>.L <connect_until_time>.L
					WPACKETL(2) = RFIFOL(fd,2);
					WPACKETL(6) = (unsigned long)auth_fifo[i].connect_until_time;
					SENDPACKET(fd, 10);
					// Send Friends info after pc_authok_final_step (player is finally authenticated)
					chrif_send_friends(idx, fd); // index,server_fd.
					// ---------- here the character is completly sended
					// send pet if necessary
					if (char_dat[idx].pet_id > 0)
						mapif_load_pet(fd, RFIFOL(fd,2), RFIFOL(fd,6), char_dat[idx].pet_id); // int account_id, int char_id, int pet_id
					//printf("auth_fifo search success (auth #%d, account %d, character: %d).\n", i, RFIFOL(fd,2), RFIFOL(fd,6));
					break;
				}
			}
			if (i == AUTH_FIFO_SIZE) {
				WPACKETW(0) = 0x2afe;
				WPACKETL(2) = RFIFOL(fd,2);
				SENDPACKET(fd, 6);
				printf("auth_fifo search error! account %d not authentified.\n", RFIFOL(fd,2));
			}
			RFIFOSKIP(fd,23);
			break;

		// MAP�T�[�o�[��̃��[�U�[����M
		// Receive alive message (online players) from map-server (no answer)
		case 0x2aff:
			if (RFIFOREST(fd) < 9 || RFIFOREST(fd) < RFIFOW(fd,2))
				return 0;
		  {
			time_t next_alive_time; // speed up
			server_freezeflag[id] = anti_freeze_counter; // Map anti-freeze system. Counter. 6 ok, 5...0 frozen
			if (RFIFOW(fd,4) != server[id].users)
				printf("User count changing: %d->%d (server: %d).\n", server[id].users, RFIFOW(fd,4), id);
			server[id].users = RFIFOW(fd,4);
			server[id].agit_flag = RFIFOB(fd,8); // 0: WoE not starting, Woe is running
			//printf("users: %d, hidden users: %d, agit: %d.\n", server[id].users, RFIFOW(fd,6 + (4 * server[id].users)), (server[id].agit_flag != 0) ? "On" : "Off");
			// remove all previously online players of the server
			for(i = 0; i < char_num; i++)
				if (online_chars[i] == id + 1 || online_chars[i] == -(id + 1))
					online_chars[i] = 0;
#ifdef USE_SQL
			sql_request("UPDATE `%s` SET `online`=0 WHERE `online`=%d OR `online`=%d", char_db, id + 1, -(id + 1));
#endif /* USE_SQL */
			// add online players in the list by [Yor]
#ifdef USE_SQL
			memset(tmp_sql, 0, sizeof(tmp_sql));
#endif /* USE_SQL */
			next_alive_time = time(NULL) + 15; // 15 seconds alive
			for(i = 0; i < server[id].users; i++) {
				int char_id = RFIFOL(fd,9 + i * 4);
				for(j = 0; j < char_num; j++)
					if (char_dat[j].char_id == char_id) {
						char_dat_timer[j] = next_alive_time;
						online_chars[j] = id + 1;
						//printf("online: %d\n", char_id);
						break;
					}
#ifdef USE_SQL
				// if player is not in memory, reload it (char-server crashes and map-server doesn't crash for example)
				if (j == char_num) {
					j = mmo_char_fromsql(char_id); // return -1 if not found, index in char_dat[] if found
					if (j != -1) {
						char_dat_timer[j] = next_alive_time;
						online_chars[j] = id + 1;
						//printf("online (was not in memory): %d\n", char_id);
					}
				}
				// update sql database
				if (tmp_sql[0] == '\0')
					tmp_sql_len = sprintf(tmp_sql, "UPDATE `%s` SET `online`=%d WHERE `char_id` IN ('%d'", char_db, id + 1, char_id);
				else
					tmp_sql_len += sprintf(tmp_sql + tmp_sql_len, ", '%d'", char_id);
				// don't check length here, 65536 is enough for all information.
#endif /* USE_SQL */
			}
#ifdef USE_SQL
			if (tmp_sql[0] != '\0') {
				sprintf(tmp_sql + tmp_sql_len, ")");
				//printf("%s\n", tmp_sql);
				sql_request(tmp_sql);
			}
#endif /* USE_SQL */
			// add hidden players in the list by [Yor]
#ifdef USE_SQL
			memset(tmp_sql, 0, sizeof(tmp_sql));
#endif /* USE_SQL */
			for(i = 0; i < RFIFOW(fd,6); i++) {
				int char_id = RFIFOL(fd,9 + (server[id].users + i) * 4);
				for(j = 0; j < char_num; j++)
					if (char_dat[j].char_id == char_id) {
						char_dat_timer[j] = next_alive_time;
						online_chars[j] = -(id + 1);
						//printf("hidden: %d\n", char_id);
						break;
					}
#ifdef USE_SQL
				// if player is not in memory, reload it (char-server crashes and map-server doesn't crash for example)
				if (j == char_num) {
					j = mmo_char_fromsql(char_id); // return -1 if not found, index in char_dat[] if found
					if (j != -1) {
						char_dat_timer[j] = next_alive_time;
						online_chars[j] = -(id + 1);
						//printf("hidden (was not in memory): %d\n", char_id);
					}
				}
				// update sql database
				if (tmp_sql[0] == '\0')
					tmp_sql_len = sprintf(tmp_sql, "UPDATE `%s` SET `online`=%d WHERE `char_id` IN ('%d'", char_db, -(id + 1), char_id);
				else
					tmp_sql_len += sprintf(tmp_sql + tmp_sql_len, ", '%d'", char_id);
				// don't check length here, 65536 is enough for all information.
#endif /* USE_SQL */
			}
#ifdef USE_SQL
			if (tmp_sql[0] != '\0') {
				sprintf(tmp_sql + tmp_sql_len, ")");
				//printf("%s\n", tmp_sql);
				sql_request(tmp_sql);
			}
#endif /* USE_SQL */
			if (update_online < time(NULL)) { // Time is done
				update_online = time(NULL) + 8;
				create_online_files(); // only every 8 sec. (normally, 1 server send users every 5 sec.) Don't update every time, because that takes time, but only every 2 connection.
				                       // it set to 8 sec because is more than 5 (sec) and if we have more than 1 map-server, informations can be received in shifted.
			}
			send_users_tologin(0, 0, 0, 0); // update number of players to login-server and map-servers
		  }
			RFIFOSKIP(fd,RFIFOW(fd,2));
			break;

		// �L�����f�[�^�ۑ�
		// Receive character data from map-server (no answer)
		case 0x2b01:
			if (RFIFOREST(fd) < 12 || RFIFOREST(fd) < RFIFOW(fd,2))
				return 0;
			server_freezeflag[id] = anti_freeze_counter; // Map anti-freeze system. Counter. 6 ok, 5...0 frozen
#ifdef USE_SQL
			mmo_char_tosql((struct mmo_charstatus*)RFIFOP(fd,12));
#endif // USE_SQL
			for(i = 0; i < char_num; i++) {
				if (char_dat[i].char_id == RFIFOL(fd,8)) { // char_id is unique
#ifdef TXT_ONLY
					memcpy(&char_dat[i], RFIFOP(fd,12), sizeof(struct mmo_charstatus));
#endif // TXT_ONLY
					char_dat_timer[i] = time(NULL) + 15; // 15 seconds alive
					break;
				}
			}
			RFIFOSKIP(fd,RFIFOW(fd,2));
			break;

		// req char selection
		// player comes back from map-server to char-server
		case 0x2b02:
			if (RFIFOREST(fd) < 19)
				return 0;
			server_freezeflag[id] = anti_freeze_counter; // Map anti-freeze system. Counter. 6 ok, 5...0 frozen
			// to conserv a maximum of authentification, search if account is already authentified and replace it
			// that will reduce multiple connection too
			for(i = 0; i < AUTH_FIFO_SIZE; i++)
				if (auth_fifo[i].account_id == RFIFOL(fd,2))
					break;
			// if not found, use next value
			if (i == AUTH_FIFO_SIZE) {
				if (auth_fifo_pos >= AUTH_FIFO_SIZE)
					auth_fifo_pos = 0;
				i = auth_fifo_pos;
				auth_fifo_pos++;
			}
			//printf("auth_fifo set (auth #%d) - account: %d, secure: %08x-%08x\n", i, RFIFOL(fd,2), RFIFOL(fd,6), RFIFOL(fd,10));
			auth_fifo[i].account_id = RFIFOL(fd,2);
			auth_fifo[i].char_id = 0;
			auth_fifo[i].login_id1 = RFIFOL(fd,6);
			auth_fifo[i].login_id2 = RFIFOL(fd,10);
			auth_fifo[i].delflag = 2; // 0: auth_fifo canceled/void, 2: auth_fifo received from login/map server in memory, 1: connection authentified
			auth_fifo[i].char_pos = 0;
			auth_fifo[i].sex = RFIFOB(fd,18);
			auth_fifo[i].connect_until_time = 0; // unlimited/unknown time by default (not display in map-server)
			auth_fifo[i].ip = RFIFOL(fd,14);
			auth_fifo[i].map_auth = 1;
			WPACKETW(0) = 0x2b03;
			WPACKETL(2) = RFIFOL(fd,2);
			SENDPACKET(fd, 6);
			RFIFOSKIP(fd,19);
			break;

		// �}�b�v�T�[�o�[�Ԉړ��v��
		// request "change map server"
		case 0x2b05:
			if (RFIFOREST(fd) < 49)
				return 0;
			server_freezeflag[id] = anti_freeze_counter; // Map anti-freeze system. Counter. 6 ok, 5...0 frozen
			for(j = 0; j < char_num; j++)
				if (char_dat[j].char_id == RFIFOL(fd,14)) { // char_id is unique
					// to conserv a maximum of authentification, search if account is already authentified and replace it
					// that will reduce multiple connection too
					for(i = 0; i < AUTH_FIFO_SIZE; i++)
						if (auth_fifo[i].account_id == RFIFOL(fd,2))
							break;
					// if not found, use next value
					if (i == AUTH_FIFO_SIZE) {
						if (auth_fifo_pos >= AUTH_FIFO_SIZE)
							auth_fifo_pos = 0;
						i = auth_fifo_pos;
						auth_fifo_pos++;
					}
					//printf("auth_fifo set (auth#%d) - account: %d, secure: 0x%08x-0x%08x\n", i, RFIFOL(fd,2), RFIFOL(fd,6), RFIFOL(fd,10));
					auth_fifo[i].account_id = RFIFOL(fd,2);
					auth_fifo[i].login_id1 = RFIFOL(fd,6);
					auth_fifo[i].login_id2 = RFIFOL(fd,10);
					auth_fifo[i].char_id = RFIFOL(fd,14);
					auth_fifo[i].delflag = 0; // 0: auth_fifo canceled/void, 2: auth_fifo received from login/map server in memory, 1: connection authentified
					auth_fifo[i].sex = RFIFOB(fd,44);
					auth_fifo[i].connect_until_time = 0; // unlimited/unknown time by default (not display in map-server)
					auth_fifo[i].ip = RFIFOL(fd,45);
					auth_fifo[i].map_auth = 1;
					auth_fifo[i].char_pos = j;
					WPACKETW(0) = 0x2b06;
					memcpy(WPACKETP(2), RFIFOP(fd,2), 42);
					WPACKETL(6) = 0; // correct
					SENDPACKET(fd,44);
					break;
				}
			if (j == char_num) {
				WPACKETW(0) = 0x2b06;
				memcpy(WPACKETP(2), RFIFOP(fd,2), 42);
				WPACKETL(6) = 1; // fail
				SENDPACKET(fd, 44);
			}
			RFIFOSKIP(fd,49);
			break;

		// char name check // �L����������
		case 0x2b08:
			if (RFIFOREST(fd) < 6)
				return 0;
			server_freezeflag[id] = anti_freeze_counter; // Map anti-freeze system. Counter. 6 ok, 5...0 frozen
		  {
			int char_id;
			char_id = RFIFOL(fd,2);
			WPACKETW(0) = 0x2b09;
			WPACKETL(2) = char_id;
			for(i = 0; i < char_num; i++) {
				if (char_dat[i].char_id == char_id)
					break;
			}
			if (i != char_num)
				strncpy(WPACKETP(6), char_dat[i].name, 24);
			else {
#ifdef USE_SQL
				sql_request("SELECT `name` FROM `%s` WHERE `char_id`='%d'", char_db, char_id);
				if (sql_get_row())
					strncpy(WPACKETP(6), sql_get_string(0), 24);
				else
#endif // USE_SQL
					strncpy(WPACKETP(6), unknown_char_name, 24);
			}
			SENDPACKET(fd, 30);
		  }
			RFIFOSKIP(fd,6);
			break;

		// it is a request to become GM
		case 0x2b0a:
			if (RFIFOREST(fd) < 8 || RFIFOREST(fd) < RFIFOW(fd,2))
				return 0;
			server_freezeflag[id] = anti_freeze_counter; // Map anti-freeze system. Counter. 6 ok, 5...0 frozen
//			printf("parse_frommap: change gm -> login, account: %d, pass: '%s'.\n", RFIFOL(fd,4), RFIFOP(fd,8));
			if (login_fd > 0) { // don't send request if no login-server
				WPACKETW(0) = 0x2720;
				memcpy(WPACKETP(2), RFIFOP(fd,2), RFIFOW(fd,2)-2);
				SENDPACKET(login_fd, WPACKETW(2));
			} else {
				WPACKETW(0) = 0x2b0b;
				WPACKETL(2) = RFIFOL(fd,4);
				WPACKETW(6) = 0;
				SENDPACKET(fd, 8);
			}
			RFIFOSKIP(fd, RFIFOW(fd,2));
			break;

		// Map server send information to change an email of an account -> login-server (actually, no answer)
		case 0x2b0c:
			if (RFIFOREST(fd) < 86)
				return 0;
			server_freezeflag[id] = anti_freeze_counter; // Map anti-freeze system. Counter. 6 ok, 5...0 frozen
			WPACKETW(0) = 0x2722; // 0x2722 <account_id>.L <actual_e-mail>.40B <new_e-mail>.40B
			memcpy(WPACKETP(2), RFIFOP(fd,2), 84);
			SENDPACKET(login_fd, 86);
			RFIFOSKIP(fd, 86);
			break;

		// Map server send information to change a password of an account -> login-server (actually, no answer)
		case 0x2b1d:
			if (RFIFOREST(fd) < 70)
				return 0;
			server_freezeflag[id] = anti_freeze_counter; // Map anti-freeze system. Counter. 6 ok, 5...0 frozen
			WPACKETW(0) = 0x272d; // 0x272d <account_id>.L <old_password>.32B <new_password>.32B
			memcpy(WPACKETP(2), RFIFOP(fd,2), 68);
			SENDPACKET(login_fd, 70);
			RFIFOSKIP(fd, 70);
			break;

		// Map server ask char-server about a character name to do some operations (all operations are transmitted to login-server)
		case 0x2b0e:
			if (RFIFOREST(fd) < 44)
				return 0;
			server_freezeflag[id] = anti_freeze_counter; // Map anti-freeze system. Counter. 6 ok, 5...0 frozen
		  {
			char character_name[25], character_name_found[25];
#ifdef USE_SQL
			char t_character_name[50];
#endif /* USE_SQL */
			int acc = RFIFOL(fd,2); // account_id of who ask (-1 if nobody)
			int account_id;
			strncpy(character_name, RFIFOP(fd,6), 24);
			character_name[24] = '\0';
			account_id = atoi(character_name);
			// search character in memory
			i = char_nick2idx(character_name);
			// if not found, we search if the name is an account id and if a character exists
			if (i < 0) { // -1: no char with this name, -2: exact sensitive name not found
				char *p_character_name;
				i = 0;
				p_character_name = character_name;
				while(isspace(*p_character_name))
					p_character_name++;
				if (*p_character_name == '+' || *p_character_name == '-')
					p_character_name++;
				// if character_name is a number
				if (*p_character_name >= '0' && *p_character_name <= '9') {
					for(j = 0; j < char_num; j++)
						if (char_dat[j].account_id == account_id && char_dat[j].sex != 2) { // if not a server account
							strncpy(character_name_found, char_dat[j].name, 24);
							character_name_found[24] = '\0';
							i = 1;
							break;
						}
				}
			} else {
				if (char_dat[i].sex != 2) { // if not a server account
					account_id = char_dat[i].account_id;
					strncpy(character_name_found, char_dat[i].name, 24);
					character_name_found[24] = '\0';
					i = 1;
				} else
					i = 0;
			}
#ifdef USE_SQL
			// if not found in memory
			if (i != 1) {
				// search character in SQL
				i = 0;
				mysql_escape_string(t_character_name, character_name, strlen(character_name));
				sql_request("SELECT `account_id`, `name` FROM `%s` WHERE `name` = '%s'", char_db, t_character_name);
				while (sql_get_row()) {
					i++;
					account_id = sql_get_integer(0);
					strncpy(character_name_found, sql_get_string(1), 24);
					character_name_found[24] = '\0';
					// Strict comparison (if found, we finish the function immediatly with correct value)
					if (strcmp(character_name_found, character_name) == 0) {
						i = 1;
						break;
					}
				}
				// if not found (unique or not), we search if the name is an account id and if a character exists
				if (i == 0) {
					char *p_character_name;
					p_character_name = character_name;
					while(isspace(*p_character_name))
						p_character_name++;
					if (*p_character_name == '+' || *p_character_name == '-')
						p_character_name++;
					// if character_name is a number
					if (*p_character_name >= '0' && *p_character_name <= '9') {
						// try to found player with account id
						sql_request("SELECT `account_id` FROM `%s` WHERE `account_id` = '%d'", char_db, atoi(character_name));
						if (sql_get_row()) {
							account_id = atoi(character_name); // sql_get_integer(0);
							strncpy(character_name_found, character_name, 24);
							character_name_found[24] = '\0';
							i = 1;
						}
					}
				}
			}
#endif /* USE_SQL */
			if (i == 1) {
				switch(RFIFOW(fd, 30)) {
				case 1: // block
					if (acc == -1 || isGM(acc) >= isGM(account_id)) {
						if (login_fd > 0) { // don't send request if no login-server
							WPACKETW(0) = 0x2724;
							WPACKETL(2) = account_id; // account value
							WPACKETL(6) = 5; // status of the account
							SENDPACKET(login_fd, 10);
//							printf("char : status -> login: account %d, status: %d \n", account_id, 5);
							// send answer if a player ask, not if the server ask
							if (acc != -1) {
								WPACKETW( 0) = 0x2b0f; // answer
								WPACKETL( 2) = acc; // who want do operation
								memcpy(WPACKETP(6), character_name_found, 24); // put correct name if found
								WPACKETW(30) = RFIFOW(fd, 30); // type of operation: 1-block, 2-ban, 3-unblock, 4-unban, 5-changesex, 6-changeGMlevel
								WPACKETW(32) = 0; // answer: 0-login-server resquest done, 1-player not found, 2-gm level too low, 3-login-server offline
								SENDPACKET(fd, 34);
							}
						} else
							// send answer if a player ask, not if the server ask
							if (acc != -1) {
								WPACKETW( 0) = 0x2b0f; // answer
								WPACKETL( 2) = acc; // who want do operation
								memcpy(WPACKETP(6), character_name_found, 24); // put correct name if found
								WPACKETW(30) = RFIFOW(fd, 30); // type of operation: 1-block, 2-ban, 3-unblock, 4-unban, 5-changesex, 6-changeGMlevel
								WPACKETW(32) = 3; // answer: 0-login-server resquest done, 1-player not found, 2-gm level too low, 3-login-server offline
								SENDPACKET(fd, 34);
							}
					} else
						// send answer if a player ask, not if the server ask
						if (acc != -1) {
							WPACKETW( 0) = 0x2b0f; // answer
							WPACKETL( 2) = acc; // who want do operation
							memcpy(WPACKETP(6), character_name_found, 24); // put correct name if found
							WPACKETW(30) = RFIFOW(fd, 30); // type of operation: 1-block, 2-ban, 3-unblock, 4-unban, 5-changesex, 6-changeGMlevel
							WPACKETW(32) = 2; // answer: 0-login-server resquest done, 1-player not found, 2-gm level too low, 3-login-server offline
							SENDPACKET(fd, 34);
						}
					break;
				case 2: // ban
					if (acc == -1 || isGM(acc) >= isGM(account_id)) {
						if (login_fd > 0) { // don't send request if no login-server
							WPACKETW( 0) = 0x2725;
							WPACKETL( 2) = account_id; // account value
							WPACKETW( 6) = RFIFOW(fd,32); // year
							WPACKETW( 8) = RFIFOW(fd,34); // month
							WPACKETW(10) = RFIFOW(fd,36); // day
							WPACKETW(12) = RFIFOW(fd,38); // hour
							WPACKETW(14) = RFIFOW(fd,40); // minute
							WPACKETW(16) = RFIFOW(fd,42); // second
							SENDPACKET(login_fd, 18);
//							printf("char : status -> login: account %d, ban: %dy %dm %dd %dh %dmn %ds\n",
//							       account_id, (short)RFIFOW(fd,32), (short)RFIFOW(fd,34), (short)RFIFOW(fd,36), (short)RFIFOW(fd,38), (short)RFIFOW(fd,40), (short)RFIFOW(fd,42));
							// send answer if a player ask, not if the server ask
							if (acc != -1) {
								WPACKETW( 0) = 0x2b0f; // answer
								WPACKETL( 2) = acc; // who want do operation
								memcpy(WPACKETP(6), character_name_found, 24); // put correct name if found
								WPACKETW(30) = RFIFOW(fd, 30); // type of operation: 1-block, 2-ban, 3-unblock, 4-unban, 5-changesex, 6-changeGMlevel
								WPACKETW(32) = 0; // answer: 0-login-server resquest done, 1-player not found, 2-gm level too low, 3-login-server offline
								SENDPACKET(fd, 34);
							}
						} else
							// send answer if a player ask, not if the server ask
							if (acc != -1) {
								WPACKETW( 0) = 0x2b0f; // answer
								WPACKETL( 2) = acc; // who want do operation
								memcpy(WPACKETP(6), character_name_found, 24); // put correct name if found
								WPACKETW(30) = RFIFOW(fd, 30); // type of operation: 1-block, 2-ban, 3-unblock, 4-unban, 5-changesex, 6-changeGMlevel
								WPACKETW(32) = 3; // answer: 0-login-server resquest done, 1-player not found, 2-gm level too low, 3-login-server offline
								SENDPACKET(fd, 34);
							}
					} else
						// send answer if a player ask, not if the server ask
						if (acc != -1) {
							WPACKETW( 0) = 0x2b0f; // answer
							WPACKETL( 2) = acc; // who want do operation
							memcpy(WPACKETP(6), character_name_found, 24); // put correct name if found
							WPACKETW(30) = RFIFOW(fd, 30); // type of operation: 1-block, 2-ban, 3-unblock, 4-unban, 5-changesex, 6-changeGMlevel
							WPACKETW(32) = 2; // answer: 0-login-server resquest done, 1-player not found, 2-gm level too low, 3-login-server offline
							SENDPACKET(fd, 34);
						}
					break;
				case 3: // unblock
					if (acc == -1 || isGM(acc) >= isGM(account_id)) {
						if (login_fd > 0) { // don't send request if no login-server
							WPACKETW(0) = 0x2724;
							WPACKETL(2) = account_id; // account value
							WPACKETL(6) = 0; // status of the account
							SENDPACKET(login_fd, 10);
//							printf("char : status -> login: account %d, status: %d \n", account_id, 0);
							// send answer if a player ask, not if the server ask
							if (acc != -1) {
								WPACKETW( 0) = 0x2b0f; // answer
								WPACKETL( 2) = acc; // who want do operation
								memcpy(WPACKETP(6), character_name_found, 24); // put correct name if found
								WPACKETW(30) = RFIFOW(fd, 30); // type of operation: 1-block, 2-ban, 3-unblock, 4-unban, 5-changesex, 6-changeGMlevel
								WPACKETW(32) = 0; // answer: 0-login-server resquest done, 1-player not found, 2-gm level too low, 3-login-server offline
								SENDPACKET(fd, 34);
							}
						} else
							// send answer if a player ask, not if the server ask
							if (acc != -1) {
								WPACKETW( 0) = 0x2b0f; // answer
								WPACKETL( 2) = acc; // who want do operation
								memcpy(WPACKETP(6), character_name_found, 24); // put correct name if found
								WPACKETW(30) = RFIFOW(fd, 30); // type of operation: 1-block, 2-ban, 3-unblock, 4-unban, 5-changesex, 6-changeGMlevel
								WPACKETW(32) = 3; // answer: 0-login-server resquest done, 1-player not found, 2-gm level too low, 3-login-server offline
								SENDPACKET(fd, 34);
							}
					} else
						// send answer if a player ask, not if the server ask
						if (acc != -1) {
							WPACKETW( 0) = 0x2b0f; // answer
							WPACKETL( 2) = acc; // who want do operation
							memcpy(WPACKETP(6), character_name_found, 24); // put correct name if found
							WPACKETW(30) = RFIFOW(fd, 30); // type of operation: 1-block, 2-ban, 3-unblock, 4-unban, 5-changesex, 6-changeGMlevel
							WPACKETW(32) = 2; // answer: 0-login-server resquest done, 1-player not found, 2-gm level too low, 3-login-server offline
							SENDPACKET(fd, 34);
						}
					break;
				case 4: // unban
					if (acc == -1 || isGM(acc) >= isGM(account_id)) {
						if (login_fd > 0) { // don't send request if no login-server
							WPACKETW(0) = 0x272a;
							WPACKETL(2) = account_id; // account value
							SENDPACKET(login_fd, 6);
//							printf("char : status -> login: account %d, unban request\n", account_id);
							// send answer if a player ask, not if the server ask
							if (acc != -1) {
								WPACKETW( 0) = 0x2b0f; // answer
								WPACKETL( 2) = acc; // who want do operation
								memcpy(WPACKETP(6), character_name_found, 24); // put correct name if found
								WPACKETW(30) = RFIFOW(fd, 30); // type of operation: 1-block, 2-ban, 3-unblock, 4-unban, 5-changesex, 6-changeGMlevel
								WPACKETW(32) = 0; // answer: 0-login-server resquest done, 1-player not found, 2-gm level too low, 3-login-server offline
								SENDPACKET(fd, 34);
							}
						} else
							// send answer if a player ask, not if the server ask
							if (acc != -1) {
								WPACKETW( 0) = 0x2b0f; // answer
								WPACKETL( 2) = acc; // who want do operation
								memcpy(WPACKETP(6), character_name_found, 24); // put correct name if found
								WPACKETW(30) = RFIFOW(fd, 30); // type of operation: 1-block, 2-ban, 3-unblock, 4-unban, 5-changesex, 6-changeGMlevel
								WPACKETW(32) = 3; // answer: 0-login-server resquest done, 1-player not found, 2-gm level too low, 3-login-server offline
								SENDPACKET(fd, 34);
							}
					} else
						// send answer if a player ask, not if the server ask
						if (acc != -1) {
							WPACKETW( 0) = 0x2b0f; // answer
							WPACKETL( 2) = acc; // who want do operation
							memcpy(WPACKETP(6), character_name_found, 24); // put correct name if found
							WPACKETW(30) = RFIFOW(fd, 30); // type of operation: 1-block, 2-ban, 3-unblock, 4-unban, 5-changesex, 6-changeGMlevel
							WPACKETW(32) = 2; // answer: 0-login-server resquest done, 1-player not found, 2-gm level too low, 3-login-server offline
							SENDPACKET(fd, 34);
						}
					break;
				case 5: // changesex
					if (acc == -1 || isGM(acc) >= isGM(account_id)) {
						if (login_fd > 0) { // don't send request if no login-server
							WPACKETW(0) = 0x2727;
							WPACKETL(2) = account_id; // account value
							WPACKETL(6) = acc; // who want do operation
							SENDPACKET(login_fd, 10);
//							printf("char : status -> login: account %d, change sex request\n", account_id);
							// send answer if a player ask, not if the server ask
							if (acc != -1) {
								WPACKETW( 0) = 0x2b0f; // answer
								WPACKETL( 2) = acc; // who want do operation
								memcpy(WPACKETP(6), character_name_found, 24); // put correct name if found
								WPACKETW(30) = RFIFOW(fd, 30); // type of operation: 1-block, 2-ban, 3-unblock, 4-unban, 5-changesex, 6-changeGMlevel
								WPACKETW(32) = 0; // answer: 0-login-server resquest done, 1-player not found, 2-gm level too low, 3-login-server offline
								SENDPACKET(fd, 34);
							}
						} else
							// send answer if a player ask, not if the server ask
							if (acc != -1) {
								WPACKETW( 0) = 0x2b0f; // answer
								WPACKETL( 2) = acc; // who want do operation
								memcpy(WPACKETP(6), character_name_found, 24); // put correct name if found
								WPACKETW(30) = RFIFOW(fd, 30); // type of operation: 1-block, 2-ban, 3-unblock, 4-unban, 5-changesex, 6-changeGMlevel
								WPACKETW(32) = 3; // answer: 0-login-server resquest done, 1-player not found, 2-gm level too low, 3-login-server offline
								SENDPACKET(fd, 34);
							}
					} else
						// send answer if a player ask, not if the server ask
						if (acc != -1) {
							WPACKETW( 0) = 0x2b0f; // answer
							WPACKETL( 2) = acc; // who want do operation
							memcpy(WPACKETP(6), character_name_found, 24); // put correct name if found
							WPACKETW(30) = RFIFOW(fd, 30); // type of operation: 1-block, 2-ban, 3-unblock, 4-unban, 5-changesex, 6-changeGMlevel
							WPACKETW(32) = 2; // answer: 0-login-server resquest done, 1-player not found, 2-gm level too low, 3-login-server offline
							SENDPACKET(fd, 34);
						}
					break;
				case 6: // changeGMlevel
					if (acc == -1 || isGM(acc) >= isGM(account_id)) {
						if (login_fd > 0) { // don't send request if no login-server
							WPACKETW(0) = 0x272f; // 0x272f <account_id>.L <accound_id_of_GM>.L <GM_level>.B (account_id_of_GM = -1 -> script)
							WPACKETL(2) = account_id; // account value
							WPACKETL(6) = acc; // who want do operation
							WPACKETB(10) = RFIFOB(fd, 32); // New GM level
							SENDPACKET(login_fd, 11);
//							printf("char : GM level change -> login: account %d, new level: %d \n", account_id, (int)RFIFOB(fd, 32));
							// send answer if a player ask, not if the server ask
							if (acc != -1) {
								WPACKETW( 0) = 0x2b0f; // answer
								WPACKETL( 2) = acc; // who want do operation
								memcpy(WPACKETP(6), character_name_found, 24); // put correct name if found
								WPACKETW(30) = RFIFOW(fd, 30); // type of operation: 1-block, 2-ban, 3-unblock, 4-unban, 5-changesex, 6-changeGMlevel
								WPACKETW(32) = 0; // answer: 0-login-server resquest done, 1-player not found, 2-gm level too low, 3-login-server offline
								SENDPACKET(fd, 34);
							}
						} else
							// send answer if a player ask, not if the server ask
							if (acc != -1) {
								WPACKETW( 0) = 0x2b0f; // answer
								WPACKETL( 2) = acc; // who want do operation
								memcpy(WPACKETP(6), character_name_found, 24); // put correct name if found
								WPACKETW(30) = RFIFOW(fd, 30); // type of operation: 1-block, 2-ban, 3-unblock, 4-unban, 5-changesex, 6-changeGMlevel
								WPACKETW(32) = 3; // answer: 0-login-server resquest done, 1-player not found, 2-gm level too low, 3-login-server offline
								SENDPACKET(fd, 34);
							}
					} else
						// send answer if a player ask, not if the server ask
						if (acc != -1) {
							WPACKETW( 0) = 0x2b0f; // answer
							WPACKETL( 2) = acc; // who want do operation
							memcpy(WPACKETP(6), character_name_found, 24); // put correct name if found
							WPACKETW(30) = RFIFOW(fd, 30); // type of operation: 1-block, 2-ban, 3-unblock, 4-unban, 5-changesex, 6-changeGMlevel
							WPACKETW(32) = 2; // answer: 0-login-server resquest done, 1-player not found, 2-gm level too low, 3-login-server offline
							SENDPACKET(fd, 34);
						}
					break;
				}
			// character name not found
			} else {
				// send answer if a player ask, not if the server ask
				if (acc != -1) {
					WPACKETW( 0) = 0x2b0f; // answer
					WPACKETL( 2) = acc; // who want do operation
					memcpy(WPACKETP(6), character_name, 24);
					WPACKETW(30) = RFIFOW(fd, 30); // type of operation: 1-block, 2-ban, 3-unblock, 4-unban, 5-changesex, 6-changeGMlevel
					WPACKETW(32) = 1; // answer: 0-login-server resquest done, 1-player not found, 2-gm level too low, 3-login-server offline
					SENDPACKET(fd, 34);
				}
			}
		  }
			RFIFOSKIP(fd, 44);
			break;

		// Receiving account_reg2 from map-server
		case 0x2b10: // send structure of account_reg2 from map-server to char-server
			if (RFIFOREST(fd) < 8 || RFIFOREST(fd) < RFIFOW(fd,2))
				return 0;
			server_freezeflag[id] = anti_freeze_counter; // Map anti-freeze system. Counter. 6 ok, 5...0 frozen
		  {
			int acc;
			acc = RFIFOL(fd,4);
			// if something to save
			if (RFIFOW(fd,2) > 8) {
				for(i = 0; i < account_reg2_db_num; i++) {
					if (account_reg2_db[i].account_id == acc) {
						// if found, free actual structure
						account_reg2_db[i].update_time = time(NULL);
						if (account_reg2_db[i].num != (RFIFOW(fd,2) - 8) / sizeof(struct global_reg)) {
							account_reg2_db[i].num = (RFIFOW(fd,2) - 8) / sizeof(struct global_reg);
							REALLOC(account_reg2_db[i].reg2, struct global_reg, account_reg2_db[i].num);
						}
						break;
					}
				}
				// if not found, create a new structure
				if (i == account_reg2_db_num) {
					if (account_reg2_db_num == 0) {
						MALLOC(account_reg2_db, struct account_reg2_db, 1);
					} else {
						REALLOC(account_reg2_db, struct account_reg2_db, account_reg2_db_num + 1);
					}
					account_reg2_db[i].account_id = acc;
					account_reg2_db[i].update_time = time(NULL);
					account_reg2_db[i].num = (RFIFOW(fd,2) - 8) / sizeof(struct global_reg);
					MALLOC(account_reg2_db[i].reg2, struct global_reg, account_reg2_db[i].num);
					account_reg2_db_num++;
				}
				// save reg2
				memcpy(account_reg2_db[i].reg2, RFIFOP(fd,8), RFIFOW(fd,2) - 8);
			// if nothing to save
			} else {
				for(i = 0; i < account_reg2_db_num; i++) {
					if (account_reg2_db[i].account_id == acc) {
						// if account reg2 exist
						FREE(account_reg2_db[i].reg2);
						// delete it
						if (account_reg2_db_num > 1) {
							if (i != account_reg2_db_num - 1)
								memcpy(&account_reg2_db[i], &account_reg2_db[account_reg2_db_num - 1], sizeof(struct account_reg2_db));
							REALLOC(account_reg2_db, struct account_reg2_db, account_reg2_db_num - 1);
							account_reg2_db_num--;
						} else {
							FREE(account_reg2_db);
							account_reg2_db_num = 0;
						}
						break;
					}
				}
			}
			// Send account register 2 to login server
			WPACKETW(0) = 0x2728; // send structure of account_reg2 from char-server to login-server
			memcpy(WPACKETP(2), RFIFOP(fd,2), RFIFOW(fd,2) - 2);
			SENDPACKET(login_fd, WPACKETW(2));
			// login-server will send back ACK (0x272c)
//			printf("char: save_account_reg2 (from map)\n");
			RFIFOSKIP(fd, RFIFOW(fd,2));
		  }
			break;

		// Recieve rates [Wizputer]
		case 0x2b16: // read by TXT version to have compatibility
			if (RFIFOREST(fd) < 10 || RFIFOREST(fd) < RFIFOW(fd,2))
				return 0;
			server_freezeflag[id] = anti_freeze_counter; // Map anti-freeze system. Counter. 6 ok, 5...0 frozen
#ifndef TXT_ONLY
		  {
			char bslash_server_name[42], bslash_motd[512];
			mysql_escape_string(bslash_server_name, server_name, strlen(server_name));
			mysql_escape_string(bslash_motd, RFIFOP(fd,10), strlen(RFIFOP(fd,10)));
			sql_request("INSERT INTO `ragsrvinfo` SET `index`='%d',`name`='%s',`exp`='%d',`jexp`='%d',`drop`='%d',`motd`='%s'",
			            fd, bslash_server_name, RFIFOW(fd,4), RFIFOW(fd,6), RFIFOW(fd,8), bslash_motd);
		  }
#endif /* not TXT_ONLY */
			RFIFOSKIP(fd, RFIFOW(fd,2));
			break;

		// Recieve global register from map-server
		case 0x2b1b: // send structure of global_reg from map-server to char-server
			if (RFIFOREST(fd) < 8 || RFIFOREST(fd) < RFIFOW(fd,2))
				return 0;
			server_freezeflag[id] = anti_freeze_counter; // Map anti-freeze system. Counter. 6 ok, 5...0 frozen
		  {
			int char_id;
			char_id = RFIFOL(fd,4);
			for(i = 0; i < char_num; i++) {
				if (char_dat[i].char_id == char_id) {
					// if something to save
					if (RFIFOW(fd,2) > 8) {
						// if actually no global register
						if (global_reg_db[i].global_reg_num == 0) {
							global_reg_db[i].global_reg_num = (RFIFOW(fd,2) - 8) / sizeof(struct global_reg);
							MALLOC(global_reg_db[i].global_reg, struct global_reg, global_reg_db[i].global_reg_num);
						// if actually some global register
						} else {
							if (global_reg_db[i].global_reg_num != (RFIFOW(fd,2) - 8) / sizeof(struct global_reg)) {
								global_reg_db[i].global_reg_num = (RFIFOW(fd,2) - 8) / sizeof(struct global_reg);
								REALLOC(global_reg_db[i].global_reg, struct global_reg, global_reg_db[i].global_reg_num);
							}
						}
						memcpy(global_reg_db[i].global_reg, RFIFOP(fd,8), sizeof(struct global_reg) * global_reg_db[i].global_reg_num);
					// if nothing to save
					} else {
						if (global_reg_db[i].global_reg_num > 0) {
							FREE(global_reg_db[i].global_reg);
							global_reg_db[i].global_reg_num = 0;
						}
					}
					break;
				}
			}
#ifdef USE_SQL
		  {
			struct global_reg *reg;
			char temp_str[65]; // 32 * 2 + NULL
//			printf("- Save global_reg_value data to MySQL!\n");
			//`global_reg_value` (`char_id`, `str`, `value`)
			sql_request("DELETE FROM `%s` WHERE `char_id`='%d'", global_reg_value, char_id);
			//insert here.
			tmp_sql_len = 0;
			reg = (struct global_reg*)RFIFOP(fd,8);
			j = 0;
			for(i = 8; i < RFIFOW(fd,2); i += sizeof(struct global_reg)) {
				mysql_escape_string(temp_str, reg[j].str, strlen(reg[j].str));
				if (tmp_sql_len == 0) {
					tmp_sql_len = sprintf(tmp_sql, "INSERT INTO `%s` (`char_id`, `str`, `value`) VALUES ('%d', '%s','%d')",
					              global_reg_value, char_id, temp_str, reg[j].value);
				} else
					tmp_sql_len += sprintf(tmp_sql + tmp_sql_len, ", ('%d', '%s','%d')",
					               char_id, temp_str, reg[j].value);
				// don't check length here, 65536 is enough for all information.
				j++;
			}
			if (tmp_sql_len != 0) {
				//printf("%s\n", tmp_sql);
				sql_request(tmp_sql);
			}
		  }
#endif /* USE_SQL */
			// Sending ACK to the char-server
			WPACKETW(0) = 0x2b1c; // 0x2b1c <char_id>.L
			WPACKETL(2) = char_id;
			SENDPACKET(fd, 6);
//			printf("char: save_global_reg (from map)\n");
			RFIFOSKIP(fd, RFIFOW(fd,2));
		  }
			break;

		// Receive deletion of a friend from a list
		case 0x2b23: // 0x2b23 <owner_char_id_list>.L <deleted_char_id>.L <flag>.B (flag: 0: do nothing, 1: search friend to remove player from its list)
			if (RFIFOREST(fd) < 11)
				return 0;
			server_freezeflag[id] = anti_freeze_counter; // Map anti-freeze system. Counter. 6 ok, 5...0 frozen
		  {
			int char_id, friend_char_id;
#ifdef TXT_ONLY
			struct friend_dat *frds;
#endif /* TXT_ONLY */
			char_id = RFIFOL(fd,2); // list owner to update
			friend_char_id = RFIFOL(fd,6); // to delete
#ifdef TXT_ONLY
			// change friend list of the character in memory (TXT)
			for(i = 0; i < char_num; i++) {
				// check character 1
				if (char_dat[i].char_id == char_id) {
					frds = &friend_dat[i];
					for (j = 0; j < frds->friend_num; j++) {
						if (frds->friends[j].char_id == friend_char_id) {
							if (frds->friend_num < 2) {
								FREE(frds->friends);
								frds->friend_num = 0;
							} else {
								if (j != frds->friend_num - 1)
									memcpy(&frds->friends[j], &frds->friends[frds->friend_num - 1], sizeof(struct friend));
								frds->friend_num--;
								REALLOC(frds->friends, struct friend, frds->friend_num);
							}
							// set flag of friends file
							friends_txt_flag = 1; // must we save friends file or not? (0: not, x: yes)
							break;
						}
					}
				}
				// check character 2
				if (char_dat[i].char_id == friend_char_id) {
					frds = &friend_dat[i];
					for (j = 0; j < frds->friend_num; j++) {
						if (frds->friends[j].char_id == char_id) {
							if (frds->friend_num < 2) {
								FREE(frds->friends);
								frds->friend_num = 0;
							} else {
								if (j != frds->friend_num - 1)
									memcpy(&frds->friends[j], &frds->friends[frds->friend_num - 1], sizeof(struct friend));
								frds->friend_num--;
								REALLOC(frds->friends, struct friend, frds->friend_num);
							}
							// set flag of friends file
							friends_txt_flag = 1; // must we save friends file or not? (0: not, x: yes)
							break;
						}
					}
				}
			}
#endif /* TXT_ONLY */
#ifdef USE_SQL
			sql_request("DELETE FROM `%s` WHERE `char_id`='%d' AND `friend_id`=%d", friends_db, char_id, friend_char_id);
			sql_request("DELETE FROM `%s` WHERE `char_id`='%d' AND `friend_id`=%d", friends_db, friend_char_id, char_id);
#endif /* USE_SQL */
			// if friend list not updated
			if (RFIFOB(fd,10)) {
				// ask other map-server to delete player from the list
				WPACKETW(0) = 0x2b22; // 0x2b22 <owner_char_id_list>.L <deleted_char_id>.L
				WPACKETL(2) = char_id; // list owner to update
				WPACKETL(6) = friend_char_id; // to delete
				mapif_sendall(10); // ask also source map: perahps player was waiting authentification on map-server
			}
		  }
			RFIFOSKIP(fd, 11);
			break;

		// Receive added friends in list of players
		case 0x2b24: // 0x2b24 <friend_char_id1>.L <friend_char_id2>.L
			if (RFIFOREST(fd) < 10)
				return 0;
			server_freezeflag[id] = anti_freeze_counter; // Map anti-freeze system. Counter. 6 ok, 5...0 frozen
		  {
			int friend_char_id1, friend_char_id2;
#ifdef TXT_ONLY
			int index1, index2;
#endif /* TXT_ONLY */
			friend_char_id1 = RFIFOL(fd,2);
			friend_char_id2 = RFIFOL(fd,6);
#ifdef TXT_ONLY
			// search the 2 characters in memory
			index1 = -1;
			index2 = -1;
			for(i = 0; i < char_num; i++) {
				if (char_dat[i].char_id == friend_char_id1) {
					index1 = i;
					if (index2 != -1) // if other index found, stop now the research
						break;
				}
				if (char_dat[i].char_id == friend_char_id2) {
					index2 = i;
					if (index1 != -1) // if other index found, stop now the research
						break;
				}
			}
			// change friend list of the character (TXT)
			if (i < char_num && // if the 2 characters have been found
			    friend_dat[index1].friend_num < MAX_FRIENDS &&
			    friend_dat[index2].friend_num < MAX_FRIENDS) {
				struct friend_dat *frds;

				// add in character 1
				frds = &friend_dat[index1];
				if (frds->friend_num == 0) {
					CALLOC(frds->friends, struct friend, 1);
				} else {
					REALLOC(frds->friends, struct friend, frds->friend_num + 1);
				}
				frds->friends[frds->friend_num].account_id =  char_dat[index2].account_id;
				frds->friends[frds->friend_num].char_id    =  char_dat[index2].char_id;
				strncpy(frds->friends[frds->friend_num].name, char_dat[index2].name, 24);
				frds->friend_num++;

				// add in character 2
				frds = &friend_dat[index2];
				if (frds->friend_num == 0) {
					CALLOC(frds->friends, struct friend, 1);
				} else {
					REALLOC(frds->friends, struct friend, frds->friend_num + 1);
				}
				frds->friends[frds->friend_num].account_id =  char_dat[index1].account_id;
				frds->friends[frds->friend_num].char_id    =  char_dat[index1].char_id;
				strncpy(frds->friends[frds->friend_num].name, char_dat[index1].name, 24);
				frds->friend_num++;
				// set flag of friends file
				friends_txt_flag = 1; // must we save friends file or not? (0: not, x: yes)
			}
#endif /* TXT_ONLY */
#ifdef USE_SQL
			sql_request("INSERT INTO `%s` (`char_id`, `friend_id`) VALUES ('%d', '%d'), ('%d', '%d')", friends_db, friend_char_id1, friend_char_id2, friend_char_id2, friend_char_id1);
#endif /* USE_SQL */
		  }
			RFIFOSKIP(fd, 10);
			break;

		case 0x2b2b:	// save player's status changes
			if (RFIFOREST(fd) < 4 || RFIFOREST(fd) < RFIFOW(fd,2) || RFIFOW(fd, 8) == 0)
				return 0;
			server_freezeflag[id] = anti_freeze_counter; // Map anti-freeze system. Counter. 6 ok, 5...0 frozen
		  {
//			int count, charid, i;
//			struct status_change_data data;
//			charid = RFIFOL(fd, 4);
//			count = RFIFOW(fd, 8);
			// TODO: management of player's status
		  }
			RFIFOSKIP(fd, RFIFOW(fd, 2));
			break;

		//Update of fame points
		case 0x2b2c: // 0x2b2c <char_id>.L <points>.L <rank_id>.B
			if (RFIFOREST(fd) < 11) //check packet length
				return 0;
			server_freezeflag[id] = anti_freeze_counter; // Map anti-freeze system. Counter. 6 ok, 5...0 frozen
		  {
//			int char_id = RFIFOL(fd, 2);
//			int points = RFIFOL(fd, 6);
//			unsigned int update_flag = 0, rank_id = RFIFOB(fd, 10);
//			unsigned int i;
//			if (rank_id >= RK_MAX)
//				return 0;
			// TODO: management of fame points
		  }
			RFIFOSKIP(fd, 11);
			break;

		default:
			// inter server - packet
		  {
			int r = inter_parse_frommap(fd);
			if (r == 1) { // processed
				server_freezeflag[id] = anti_freeze_counter; // Map anti-freeze system. Counter. 6 ok, 5...0 frozen
				break;
			}
			if (r == 2) // need more packet
				return 0;
		  }

			// no inter server packet. no char server packet -> disconnect
			printf("char: unknown packet 0x%04x (%d bytes to read in buffer)! (from map).\n", RFIFOW(fd,0), RFIFOREST(fd));
			session[fd]->eof = 1;
			return 0;
		}
	}

	return 0;
}

int search_mapserver(char *map) {
	int i, j;
	char temp_map[17]; // 16 + NULL
	int temp_map_len;

//	printf("Searching the map-server for map '%s'... ", map);
	strncpy(temp_map, map, 16); // 17 - NULL
	temp_map[16] = '\0';
	if (strchr(temp_map, '.') != NULL)
		temp_map[strchr(temp_map, '.') - temp_map + 1] = '\0'; // suppress the '.gat', but conserve the '.' to be sure of the name of the map

	temp_map_len = strlen(temp_map);
	for(i = 0; i < MAX_MAP_SERVERS; i++)
		if (server_fd[i] >= 0)
			for(j = 0; j < server[i].map_num; j++) // MAX_MAP_PER_SERVER
				if (server[i].map[j * 17]) {
					//printf("%s : %s = %d\n", server[i].map + (j * 17), map, strncmp(server[i].map + (j * 17), temp_map, temp_map_len));
					if (strncmp(server[i].map + (j * 17), temp_map, temp_map_len) == 0) {
//						printf("found -> server #%d.\n", i);
						return i;
					}
				}

//	printf("not found.\n");

	return -1;
}

//--------------------------------------------------------------
// Test of the IP mask
// (ip: IP to be tested, str: mask x.x.x.x/# or x.x.x.x/y.y.y.y)
//--------------------------------------------------------------
static inline int check_ipmask(unsigned int ip, const char *str) {
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

//-------------------------------------
// Access control by IP for map-servers
//-------------------------------------
static inline int check_mapip(unsigned int ip) {
	int i;
	unsigned char *p = (unsigned char *)&ip;
	char buf[17];
	char * access_ip;

	if (access_map_allownum == 0)
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

	for(i = 0; i < access_map_allownum; i++) {
		access_ip = access_map_allow + (i * ACO_STRSIZE);
		if (strncmp(access_ip, buf, strlen(access_ip)) == 0 || check_ipmask(ip, access_ip))
			return 1;
	}

	return 0;
}

//-----------------------------------------------------
// Test to know if an IP come from LAN or WAN. by [Yor]
//-----------------------------------------------------
int lan_ip_check(unsigned char *p) {
	int i;
	int lancheck = 1;

//	printf("lan_ip_check: to compare: %d.%d.%d.%d, network: %d.%d.%d.%d/%d.%d.%d.%d\n",
//	       p[0], p[1], p[2], p[3],
//	       subnet[0], subnet[1], subnet[2], subnet[3],
//	       subnetmask[0], subnetmask[1], subnetmask[2], subnetmask[3]);
	for(i = 0; i < 4; i++) {
		if ((subnet[i] & subnetmask[i]) != (p[i] & subnetmask[i])) {
			lancheck = 0;
			break;
		}
	}
	printf("LAN test (result): %s source" CL_RESET ".\n", (lancheck) ? CL_CYAN "LAN" : CL_GREEN "WAN");

	return lancheck;
}

//-----------------------------------------------------
// Main parse function
//-----------------------------------------------------
int parse_char(int fd) {
	int i, ch;
	char email[41]; // 40 + NULL
#ifdef USE_SQL
	char t_name[49];
#endif // USE_SQL
	struct char_session_data *sd;
	unsigned char *p = (unsigned char *) &session[fd]->client_addr.sin_addr;

	if ((login_fd <= 0 && // disconnect any player (already connected to char-server or coming back from map-server) if login-server is diconnected.
	     !(RFIFOREST(fd) >= 2 && (RFIFOW(fd, 0) == 0x7530 || RFIFOW(fd, 0) == 0x7532 || RFIFOW(fd, 0) == 0x7533 || RFIFOW(fd, 0) == 0x7535)) && // except to ask version or uptime, or ask for disconnection
	     !(RFIFOREST(fd) >= 56 && (RFIFOW(fd, 0) == 0x2af8))) || // except login as map-server
	     session[fd]->eof) {
#ifdef __WIN32
		shutdown(fd, SD_BOTH);
		closesocket(fd);
#else
		close(fd);
#endif
		delete_session(fd);
		return 0;
	}

	sd = session[fd]->session_data;

	while(RFIFOREST(fd) >= 2 && !session[fd]->eof) {
//		if (RFIFOW(fd,0) < 30000)
//			printf("parse_char: connection #%d, packet: 0x%x (with being read: %d bytes).\n", fd, RFIFOW(fd,0), RFIFOREST(fd));

		// check some packets (some packets can only be done if session data exist!)
		if (sd == NULL) {
			switch(RFIFOW(fd,0)) {
			case 0x66: // char select
			case 0x67: // make new
			case 0x68: // delete char
				// we refuse connection
				WPACKETW(0) = 0x6c;
				WPACKETB(2) = 0; // 00 = Rejected from Server
				SENDPACKET(fd, 3);
				session[fd]->eof = 1;
				return 0;
			}
		} else if (sd->auth == 0) { // 0: not already authentified, 1: authentified
			switch(RFIFOW(fd,0)) {
			case 0x66: // char select
			case 0x67: // make new
			case 0x68: // delete char
			case 0x2af8: // login as map-server
				// we refuse connection
				WPACKETW(0) = 0x6c;
				WPACKETB(2) = 0; // 00 = Rejected from Server
				SENDPACKET(fd, 3);
				session[fd]->eof = 1;
				return 0;
			}
		} else { // sd->auth != 0 // 0: not already authentified, 1: authentified
			switch(RFIFOW(fd,0)) {
			case 0x65: // request to connect
			case 0x2af8: // login as map-server
				// we refuse connection
				WPACKETW(0) = 0x6c;
				WPACKETB(2) = 0; // 00 = Rejected from Server
				SENDPACKET(fd, 3);
				session[fd]->eof = 1;
				return 0;
			}
		}

		switch(RFIFOW(fd,0)) {
		case 0x20b: //20040622 encryption ragexe correspondence
			if (RFIFOREST(fd) < 19)
				return 0;
			RFIFOSKIP(fd,19);
			break;

		case 0x65: // request to connect
			if (RFIFOREST(fd) < 17)
				return 0;
		  {
			int account_id; // speed up
			account_id = RFIFOL(fd,2);
			// check if it's a hacker (using server account)
			if (account_id < MAX_SERVERS) { // max number of char-servers (and account_id values: 0 to max-1)
				WPACKETW(0) = 0x6c;
				WPACKETB(2) = 0; // 00 = Rejected from Server
				SENDPACKET(fd, 3);
				session[fd]->eof = 1;
			} else {
				int GM_value;
//				printf("request connect - account_id:%d/login_id1:%d/login_id2:%d\n", account_id, RFIFOL(fd, 6), RFIFOL(fd, 10));
				// send back account_id
				WPACKETL(0) = account_id;
				SENDPACKET(fd, 4);
				// check if someone if already connected with same id
				for(i = 0; i < fd_max; i++) {
					struct char_session_data *sd2;
					if (i != fd && session[i] && (sd2 = session[i]->session_data) && sd2->account_id == account_id) {
						WPACKETW(0) = 0x81;
						WPACKETB(2) = 2; // 02 = Someone else is logged in on this ID
						SENDPACKET(fd, 3);
						session[fd]->eof = 1;
						WPACKETW(0) = 0x81;
						WPACKETB(2) = 2; // 02 = Someone else is logged in on this ID
						SENDPACKET(i, 3);
						session[i]->eof = 1;
						sd2->auth = 0; // 0: not already authentified, 1: authentified
						break;
					}
				}
				if (i == fd_max) {
					if ((GM_value = isGM(account_id)))
						printf("Account Logged On; Account ID: %d (GM level %d).\n", account_id, GM_value);
					else
						printf("Account Logged On; Account ID: %d.\n", account_id);
					if (sd == NULL) {
						CALLOC(session[fd]->session_data, struct char_session_data, 1);
						sd = session[fd]->session_data;
						strncpy(sd->email, "no mail", 40); // put here a mail without '@' to refuse deletion if we don't receive the e-mail
						sd->connect_until_time = 0; // unknow or illimited (not displaying on map-server)
					}
					sd->account_id = account_id; // RFIFOL(fd,2);
					sd->login_id1 = RFIFOL(fd,6);
					sd->login_id2 = RFIFOL(fd,10);
					sd->sex = RFIFOB(fd,16);
					sd->auth = 0; // 0: not already authentified, 1: authentified
					// search authentification
					for(i = 0; i < AUTH_FIFO_SIZE; i++) {
						if (auth_fifo[i].account_id == account_id &&
						    auth_fifo[i].login_id1 == sd->login_id1 &&
						    auth_fifo[i].sex == sd->sex &&
						    (!check_authfifo_login2 || auth_fifo[i].login_id2 == sd->login_id2) && // relate to the versions higher than 18
						    (!check_ip_flag || auth_fifo[i].ip == session[fd]->client_addr.sin_addr.s_addr) &&
						    auth_fifo[i].delflag == 2) { // 0: auth_fifo canceled/void, 2: auth_fifo received from login/map server in memory, 1: connection authentified
							auth_fifo[i].delflag = 1; // 0: auth_fifo canceled/void, 2: auth_fifo received from login/map server in memory, 1: connection authentified
							if (max_connect_user == 0 ||
							    count_users() < max_connect_user ||
							    min_level_to_bypass <= GM_value) { // Checks min_level_bypass to allow max_connect_user bypass [PoW]
								// check duplicated connection
								if (auth_fifo[i].map_auth == 0 && always_online_timer(account_id)) {
									WPACKETW(0) = 0x81;
									WPACKETB(2) = 8; // 08 = Server still recognises your last login
									SENDPACKET(fd, 3);
									session[fd]->eof = 1;
									//sd->auth = 0; // 0: not already authentified, 1: authentified
									WPACKETW(0) = 0x2b07;
									WPACKETL(2) = account_id;
									mapif_sendall(6);
								} else {
									// request to login-server to obtain e-mail/time limit
									WPACKETW(0) = 0x2716;
									WPACKETL(2) = account_id;
									SENDPACKET(login_fd, 6);
									// send characters to player
									mmo_char_send006b(fd, sd);
									sd->auth = 1; // 0: not already authentified, 1: authentified
								}
							} else {
								// refuse connection (over populated)
								WPACKETW(0) = 0x6c;
								WPACKETB(2) = 0; // 00 = Rejected from Server
								SENDPACKET(fd, 3);
								session[fd]->eof = 1;
							}
							break;
						}
					}
					// authentification not found
					if (i == AUTH_FIFO_SIZE) {
						if (login_fd > 0) { // don't send request if no login-server
							WPACKETW( 0) = 0x2712; // ask login-server to authentify an account
							WPACKETL( 2) = account_id; // sd->account_id;
							WPACKETL( 6) = sd->login_id1;
							WPACKETL(10) = sd->login_id2; // relate to the versions higher than 18
							WPACKETB(14) = sd->sex;
							WPACKETL(15) = session[fd]->client_addr.sin_addr.s_addr;
							SENDPACKET(login_fd, 19);
						} else { // if no login-server, we must refuse connection
							WPACKETW(0) = 0x6c;
							WPACKETB(2) = 0; // 00 = Rejected from Server
							SENDPACKET(fd, 3);
							session[fd]->eof = 1;
						}
					}
				}
			}
		  }
			RFIFOSKIP(fd,17);
			break;

		case 0x66:	// char select
			if (RFIFOREST(fd) < 3)
				return 0;

			// if we activated email creation and email is default email
			if (email_creation != 0 && strcmp(sd->email, "a@a.com") == 0 && login_fd > 0) { // to modify an e-mail, login-server must be online
				WPACKETW(0) = 0x70;
				WPACKETB(2) = 0; // 00 = Incorrect Email address
				SENDPACKET(fd, 3);

			// otherwise, load the character
			} else {
				char char_slot; // speeder
				char_slot = RFIFOB(fd,2);
				for (ch = 0; ch < 9; ch++)
					if (sd->found_char[ch] >= 0 && char_dat[sd->found_char[ch]].char_num == char_slot) // found_char = index in db (TXT), char_id (SQL), -1: void
						break;
				if (ch != 9) {
					ch = sd->found_char[ch];
					// selected char is server name?
					if (strcmp(char_dat[ch].name, wisp_server_name) == 0) {
						WPACKETW(0) = 0x81;
						WPACKETB(2) = 1; // 01 = Server closed
						SENDPACKET(fd, 3);
						session[fd]->eof = 1;
					} else {
						char_log("Character Selected, Account ID: %d, Character Slot: %d, Character Name: %s." RETCODE,
						         sd->account_id, char_slot, char_dat[ch].name);
						// searching map server
						i = search_mapserver(char_dat[ch].last_point.map);
						// if map is not found, we check major cities
						if (i < 0) {
							if ((i = search_mapserver("prontera.gat")) >= 0) { // check is done without 'gat'
								strncpy(char_dat[ch].last_point.map, "prontera.gat", 16); // 17 - NULL
								char_dat[ch].last_point.map[16] = '\0';
								char_dat[ch].last_point.x = 273; // savepoint coordinates
								char_dat[ch].last_point.y = 354;
							} else if ((i = search_mapserver("geffen.gat")) >= 0) { // check is done without 'gat'
								strncpy(char_dat[ch].last_point.map, "geffen.gat", 16); // 17 - NULL
								char_dat[ch].last_point.map[16] = '\0';
								char_dat[ch].last_point.x = 120; // savepoint coordinates
								char_dat[ch].last_point.y = 100;
							} else if ((i = search_mapserver("morocc.gat")) >= 0) { // check is done without 'gat'
								strncpy(char_dat[ch].last_point.map, "morocc.gat", 16); // 17 - NULL
								char_dat[ch].last_point.map[16] = '\0';
								char_dat[ch].last_point.x = 160; // savepoint coordinates
								char_dat[ch].last_point.y = 94;
							} else if ((i = search_mapserver("alberta.gat")) >= 0) { // check is done without 'gat'
								strncpy(char_dat[ch].last_point.map, "alberta.gat", 16); // 17 - NULL
								char_dat[ch].last_point.map[16] = '\0';
								char_dat[ch].last_point.x = 116; // savepoint coordinates
								char_dat[ch].last_point.y = 57;
							} else if ((i = search_mapserver("payon.gat")) >= 0) { // check is done without 'gat'
								strncpy(char_dat[ch].last_point.map, "payon.gat", 16); // 17 - NULL
								char_dat[ch].last_point.map[16] = '\0';
								char_dat[ch].last_point.x = 87; // savepoint coordinates
								char_dat[ch].last_point.y = 117;
							} else if ((i = search_mapserver("izlude.gat")) >= 0) { // check is done without 'gat'
								strncpy(char_dat[ch].last_point.map, "izlude.gat", 16); // 17 - NULL
								char_dat[ch].last_point.map[16] = '\0';
								char_dat[ch].last_point.x = 94; // savepoint coordinates
								char_dat[ch].last_point.y = 103;
							} else {
								int j;
								// get first online server (with a map)
								i = 0;
								for(j = 0; j < MAX_MAP_SERVERS; j++)
									if (server_fd[j] >= 0 && server[j].map && server[j].map[0]) { // change save point to one of map found on the server (the first) // MAX_MAP_PER_SERVER
										i = j;
										strncpy(char_dat[ch].last_point.map, server[j].map, 16); // 17 - NULL
										char_dat[ch].last_point.map[16] = '\0';
										printf("Map-server #%d found with a map: '%s'.\n", j, server[j].map);
										// coordinates are unknown
										break;
									}
								// if no map-server is connected, we send: server closed
								if (j == MAX_MAP_SERVERS) {
									WPACKETW(0) = 0x81;
									WPACKETB(2) = 1; // 01 = Server closed
									SENDPACKET(fd, 3);
									session[fd]->eof = 1;
									sd->auth = 0; // 0: not already authentified, 1: authentified
									RFIFOSKIP(fd,3);
									break;
								}
							}
						}
						// if a server has been found
						char_dat_timer[ch] = time(NULL) + 15 + 5; // 15 seconds alive (+5 for the authentification time between char server and map server)
						WPACKETW( 0) = 0x71;
						WPACKETL( 2) = char_dat[ch].char_id;
						strncpy(WPACKETP(6), char_dat[ch].last_point.map, 16);
						printf("Character selection '%s' (account: %d, slot: %d).\n", char_dat[ch].name, sd->account_id, RFIFOB(fd,2));
						printf("--Send IP of map-server. ");
						if (lan_ip_check(p))
							WPACKETL(22) = inet_addr(lan_map_ip);
						else
							WPACKETL(22) = server[i].ip;
						WPACKETW(26) = server[i].port;
						SENDPACKET(fd, 28);
						// to conserv a maximum of authentification, search if account is already authentified and replace it
						// that will reduce multiple connection too
						for(i = 0; i < AUTH_FIFO_SIZE; i++)
							if (auth_fifo[i].account_id == sd->account_id)
								break;
						// if not found, use next value
						if (i == AUTH_FIFO_SIZE) {
							if (auth_fifo_pos >= AUTH_FIFO_SIZE)
								auth_fifo_pos = 0;
							i = auth_fifo_pos;
							auth_fifo_pos++;
						}
						//printf("auth_fifo set #%d - account %d, char: %d, secure: %08x-%08x\n", i, sd->account_id, char_dat[ch].char_id, sd->login_id1, sd->login_id2);
						auth_fifo[i].account_id = sd->account_id;
						auth_fifo[i].char_id = char_dat[ch].char_id;
						auth_fifo[i].login_id1 = sd->login_id1;
						auth_fifo[i].login_id2 = sd->login_id2;
						auth_fifo[i].delflag = 0; // 0: auth_fifo canceled/void, 2: auth_fifo received from login/map server in memory, 1: connection authentified
						auth_fifo[i].char_pos = ch; // found_char = index in db (TXT), char_id (SQL), -1: void
						auth_fifo[i].sex = sd->sex;
						auth_fifo[i].connect_until_time = sd->connect_until_time;
						auth_fifo[i].ip = session[fd]->client_addr.sin_addr.s_addr;
						auth_fifo[i].map_auth = 1;
					}
				}
				// remove authentification (you can not do more)
				sd->auth = 0; // 0: not already authentified, 1: authentified
			}
			RFIFOSKIP(fd,3);
			break;

		case 0x67:	// make new
			if (RFIFOREST(fd) < 37)
				return 0;
		  {
			int counter;
			// check if it exist empty slot
			counter = 0;
			for(ch = 0; ch < 9; ch++)
				if (sd->found_char[ch] != -1) // found_char = index in db (TXT), char_id (SQL), -1: void
					counter++;
			// doesn't exist empty slot
			if (counter == 9) {
				WPACKETW(0) = 0x6e;
				WPACKETB(2) = 0x02; // Character creation is denied
				SENDPACKET(fd, 3);
			// an empty slot is available
			} else {
				// if we have at least 1 character (with more, we have spoof name detection) AND
				// if we activated email creation and email is default email
				if (counter && email_creation != 0 && strcmp(sd->email, "a@a.com") == 0 && login_fd > 0) { // to modify an e-mail, login-server must be online
					WPACKETW(0) = 0x6e;
					WPACKETB(2) = 0x02; // Character creation is denied
					SENDPACKET(fd, 3);
					RFIFOSKIP(fd, 37);
					break;

				// otherwise, create the character
				} else {
					i = make_new_char(fd, RFIFOP(fd,2)); // found_char = index in db (TXT), char_id (SQL), -1: void
					if (i == -2) {
						WPACKETW(0) = 0x6e;
						WPACKETB(2) = 0x02; // Character creation is denied
						SENDPACKET(fd, 3);
						RFIFOSKIP(fd, 37);
						break;
					} else if (i < 0) {
						WPACKETW(0) = 0x6e;
						WPACKETB(2) = 0x00; // This character name already exists
						SENDPACKET(fd, 3);
						RFIFOSKIP(fd, 37);
						break;
					}
					for(ch = 0; ch < 9; ch++) {
						if (sd->found_char[ch] == -1) {
							sd->found_char[ch] = i; // found_char = index in db (TXT), char_id (SQL), -1: void
							break;
						}
					}

#ifdef TXT_ONLY
					memset(WPACKETP(0), 0, 110);

					WPACKETW(0    ) = 0x6d;

					WPACKETL(2    ) = char_dat[i].char_id; // char_id
					WPACKETL(2+  4) = char_dat[i].base_exp; // base_exp
					WPACKETL(2+  8) = char_dat[i].zeny; // zeny
					WPACKETL(2+ 12) = char_dat[i].job_exp; // job_exp
					WPACKETL(2+ 16) = char_dat[i].job_level; // job_level

					//WPACKETL(2+ 20) = 0; // opt1
					//WPACKETL(2+ 24) = 0; // opt2
					WPACKETL(2+ 28) = char_dat[i].option; // option

					WPACKETL(2+ 28) = char_dat[i].karma; // karma
					WPACKETL(2+ 32) = char_dat[i].manner; // manner

					WPACKETW(2+ 40) = 0x30; // status_point
					WPACKETW(2+ 42) = (char_dat[i].hp > 0x7fff) ? 0x7fff : char_dat[i].hp; // hp
					WPACKETW(2+ 44) = (char_dat[i].max_hp > 0x7fff) ? 0x7fff : char_dat[i].max_hp; // max_hp
					WPACKETW(2+ 46) = (char_dat[i].sp > 0x7fff) ? 0x7fff : char_dat[i].sp; // sp
					WPACKETW(2+ 48) = (char_dat[i].max_sp > 0x7fff) ? 0x7fff : char_dat[i].max_sp; // max_sp
					WPACKETW(2+ 50) = DEFAULT_WALK_SPEED; // speed
					WPACKETW(2+ 52) = char_dat[i].class; // class
					WPACKETW(2+ 54) = char_dat[i].hair; // hair

					WPACKETW(2+ 56) = char_dat[i].weapon; // weapon
					WPACKETW(2+ 58) = char_dat[i].base_level; // base_level
					WPACKETW(2+ 60) = char_dat[i].skill_point; // skill_point
					WPACKETW(2+ 62) = char_dat[i].head_bottom; // head_bottom
					WPACKETW(2+ 64) = char_dat[i].shield; // shield
					WPACKETW(2+ 66) = char_dat[i].head_top; // head_top
					WPACKETW(2+ 68) = char_dat[i].head_mid; // head_mid
					WPACKETW(2+ 70) = char_dat[i].hair_color; // hair_color
					WPACKETW(2+ 72) = char_dat[i].clothes_color; // clothes_color

					strncpy(WPACKETP(2+74), char_dat[i].name, 24); // name

					WPACKETB(2+ 98) = (char_dat[i].str > 255) ? 255 : char_dat[i].str; // str
					WPACKETB(2+ 99) = (char_dat[i].agi > 255) ? 255 : char_dat[i].agi; // agi
					WPACKETB(2+100) = (char_dat[i].vit > 255) ? 255 : char_dat[i].vit; // vit
					WPACKETB(2+101) = (char_dat[i].int_ > 255) ? 255 : char_dat[i].int_; // int_
					WPACKETB(2+102) = (char_dat[i].dex > 255) ? 255 : char_dat[i].dex; // dex
					WPACKETB(2+103) = (char_dat[i].luk > 255) ? 255 : char_dat[i].luk; // luk
					WPACKETB(2+104) = char_dat[i].char_num; // char_num
					WPACKETW(2+104) = char_dat[i].char_num; // char_num
					WPACKETB(2+106) = 1; //Rename bit.
					SENDPACKET(fd, 110);
#else // TXT_ONLY -> USE_SQL
					sql_request("SELECT `char_id`,`base_exp`,`zeny`,`job_exp`,`job_level`,`option`,`karma`,`manner`,"
					            "`status_point`,`hp`,`max_hp`,`sp`,`max_sp`,`class`,`hair`,`weapon`,`base_level`,`skill_point`,"
					            "`head_bottom`,`shield`,`head_top`,`head_mid`,`hair_color`,`clothes_color`,"
					            "`name`,`str`,`agi`,`vit`,`int`,`dex`,`luk`,`char_num` FROM `%s` WHERE `char_id`='%d'", char_db, i);
					if (sql_get_row()) {
						printf("(" CL_GREEN "%d" CL_RESET ")" CL_GREEN "%s" CL_RESET "\t[char creation]\n", sql_get_integer(0), sql_get_string(24));

						memset(WPACKETP(0), 0, 110);
						WPACKETW(0    ) = 0x6d;

						WPACKETL(2    ) =  sql_get_integer( 0); // char_id
						WPACKETL(2+  4) =  sql_get_integer( 1); // base_exp
						WPACKETL(2+  8) =  sql_get_integer( 2); // zeny
						WPACKETL(2+ 12) =  sql_get_integer( 3); // job_exp
						WPACKETL(2+ 16) =  sql_get_integer( 4); // job_level

						//WPACKETL(2+ 20) =  0; // opt1
						//WPACKETL(2+ 24) =  0; // opt2
						WPACKETL(2+ 28) =  sql_get_integer( 5); // option

						WPACKETL(2+ 32) =  sql_get_integer( 6); // karma
						WPACKETL(2+ 36) =  sql_get_integer( 7); // manner

						WPACKETW(2+ 40) =  sql_get_integer( 8); // status_point
						WPACKETW(2+ 42) = (sql_get_integer( 9) > 0x7fff) ? 0x7fff : sql_get_integer( 9); // hp
						WPACKETW(2+ 44) = (sql_get_integer(10) > 0x7fff) ? 0x7fff : sql_get_integer(10); // max_hp
						WPACKETW(2+ 46) = (sql_get_integer(11) > 0x7fff) ? 0x7fff : sql_get_integer(11); // sp
						WPACKETW(2+ 48) = (sql_get_integer(12) > 0x7fff) ? 0x7fff : sql_get_integer(12); // max_sp
						WPACKETW(2+ 50) = DEFAULT_WALK_SPEED; // speed
						WPACKETW(2+ 52) =  sql_get_integer(13); // class
						WPACKETW(2+ 54) =  sql_get_integer(14); // hair
						if (sql_get_integer(14) > max_hair_style) {
							WPACKETW(2+ 54) = max_hair_style; // hair
						}

						WPACKETW(2+ 56) =  sql_get_integer(15); // weapon
						WPACKETW(2+ 58) =  sql_get_integer(16); // base_level
						WPACKETW(2+ 60) =  sql_get_integer(17); // skill_point
						WPACKETW(2+ 62) =  sql_get_integer(18); // head_bottom
						WPACKETW(2+ 64) =  sql_get_integer(19); // shield
						WPACKETW(2+ 66) =  sql_get_integer(20); // head_top
						WPACKETW(2+ 68) =  sql_get_integer(21); // head_mid
						WPACKETW(2+ 70) =  sql_get_integer(22); // hair_color
						if (sql_get_integer(22) > max_hair_color) {
							WPACKETW(2+ 70) = max_hair_color;
						}
						WPACKETW(2+ 72) =  sql_get_integer(23); // clothes_color

						strncpy(WPACKETP(2+74), sql_get_string(24), 24); // name

						WPACKETB(2+ 98) = (sql_get_integer(25) > 255) ? 255 : sql_get_integer(25); // str
						WPACKETB(2+ 99) = (sql_get_integer(26) > 255) ? 255 : sql_get_integer(26); // agi
						WPACKETB(2+100) = (sql_get_integer(27) > 255) ? 255 : sql_get_integer(27); // vit
						WPACKETB(2+101) = (sql_get_integer(28) > 255) ? 255 : sql_get_integer(28); // int_
						WPACKETB(2+102) = (sql_get_integer(29) > 255) ? 255 : sql_get_integer(29); // dex
						WPACKETB(2+103) = (sql_get_integer(30) > 255) ? 255 : sql_get_integer(30); // luk
						WPACKETW(2+104) =  sql_get_integer(31); // char_num
						WPACKETB(2+106) = 1; //Rename bit.
						SENDPACKET(fd, 110);
					}
#endif // USE_SQL
				}
			}
		  }
			RFIFOSKIP(fd,37);
			break;

		case 0x68:	// delete char //Yor's Fix
			if (RFIFOREST(fd) < 46)
				return 0;
			//printf(CL_RED " Request Char Del: " CL_RESET CL_GREEN "%d" CL_RESET "(" CL_GREEN "%d" CL_RESET ")\n", sd->account_id, RFIFOL(fd, 2));
			memset(email, 0, sizeof(email));
			strncpy(email, RFIFOP(fd,6), 40);
			if (e_mail_check(email) == 0) {
				memset(email, 0, sizeof(email));
				strncpy(email, "a@a.com", 40); // default e-mail
			}
		  {
			int char_id;
			char_id = RFIFOL(fd,2);

			// if we activated email creation and email is default email
			if (email_creation != 0 && strcmp(sd->email, "a@a.com") == 0 && login_fd > 0) { // to modify an e-mail, login-server must be online
				// if sended email is incorrect e-mail
				if (strcmp(email, "a@a.com") == 0) {
					WPACKETW(0) = 0x70;
					WPACKETB(2) = 0; // 00 = Incorrect Email address
					SENDPACKET(fd, 3);
					RFIFOSKIP(fd,46);
				// we act like we have selected a character
				} else {
					// we change the packet to set it like selection.
#ifdef TXT_ONLY
					for (i = 0; i < 9; i++)
						if (sd->found_char[i] >= 0 && char_dat[sd->found_char[i]].char_id == char_id) { // found_char = index in db (TXT), char_id (SQL), -1: void
							// we save new e-mail
							memset(sd->email, 0, sizeof(sd->email));
							strncpy(sd->email, email, 40);
							// we send new e-mail to login-server ('online' login-server is checked before)
							WPACKETW(0) = 0x2715;
							WPACKETL(2) = sd->account_id;
							strncpy(WPACKETP(6), email, 40);
							SENDPACKET(login_fd, 46);
							// skip part of the packet! (46, but leave the size of select packet: 3)
							RFIFOSKIP(fd,43);
							// change value to put new packet (char selection)
							RFIFOW(fd, 0) = 0x66;
							RFIFOB(fd, 2) = char_dat[sd->found_char[i]].char_num; // found_char = index in db (TXT), char_id (SQL), -1: void
							// resent all characters, otherwise client think that character has been destroyed
//							mmo_char_send006b(fd, sd);
							// send: incorrect email to be sure that client doesn't delete character
							WPACKETW(0) = 0x70;
							WPACKETB(2) = 1; // 01-FF = Character deletion denied
							SENDPACKET(fd, 3);
							break;
						}
					if (i == 9) {
#endif /* TXT_ONLY */
#ifdef USE_SQL
					sql_request("SELECT `char_num` FROM `%s` WHERE `char_id`='%d'", char_db, char_id);
					if (sql_get_row()) {
						// we save new e-mail
						memset(sd->email, 0, sizeof(sd->email));
						strncpy(sd->email, email, 40);
						// we send new e-mail to login-server ('online' login-server is checked before)
						WPACKETW(0) = 0x2715;
						WPACKETL(2) = sd->account_id;
						strncpy(WPACKETP(6), email, 40);
						SENDPACKET(login_fd, 46);
						// skip part of the packet! (46, but leave the size of select packet: 3)
						RFIFOSKIP(fd,43);
						// change value to put new packet (char selection)
						RFIFOW(fd, 0) = 0x66;
						RFIFOB(fd, 2) = sql_get_integer(0);
						// resent all characters, otherwise client think that character has been destroyed
//						mmo_char_send006b(fd, sd);
						// send: incorrect email to be sure that client doesn't delete character
						WPACKETW(0) = 0x70;
						WPACKETB(2) = 1; // 01-FF = Character deletion denied
						SENDPACKET(fd, 3);
					} else {
#endif /* USE_SQL */
						WPACKETW(0) = 0x70;
						WPACKETB(2) = 0; // 00 = Incorrect Email address
						SENDPACKET(fd, 3);
						RFIFOSKIP(fd,46);
					}
				}

			// otherwise, we delete the character
			} else {
				if (strcasecmp(email, sd->email) != 0) { // if it's an invalid email
					WPACKETW(0) = 0x70;
					WPACKETB(2) = 0; // 00 = Incorrect Email address
					SENDPACKET(fd, 3);
				// if mail is correct
				} else {
					for (i = 0; i < 9; i++) {
						struct mmo_charstatus *cs;
						if (sd->found_char[i] >= 0 && (cs = &char_dat[sd->found_char[i]])->char_id == char_id) { // found_char = index in db (TXT), char_id (SQL), -1: void
							char_delete(cs); // deletion process

							if (sd->found_char[i] != char_num - 1) {
								int j, k;
								struct char_session_data *sd2;
								memcpy(&char_dat[sd->found_char[i]], &char_dat[char_num - 1], sizeof(struct mmo_charstatus));
								FREE(friend_dat[sd->found_char[i]].friends);
								friend_dat[sd->found_char[i]].friend_num = friend_dat[char_num - 1].friend_num;
								friend_dat[sd->found_char[i]].friends    = friend_dat[char_num - 1].friends;
								friend_dat[char_num - 1].friend_num = 0;
								friend_dat[char_num - 1].friends    = NULL;
								FREE(global_reg_db[sd->found_char[i]].global_reg);
								global_reg_db[sd->found_char[i]].global_reg_num = global_reg_db[char_num - 1].global_reg_num;
								global_reg_db[sd->found_char[i]].global_reg     = global_reg_db[char_num - 1].global_reg;
								global_reg_db[char_num - 1].global_reg_num = 0;
								global_reg_db[char_num - 1].global_reg     = NULL;
								char_dat_timer[sd->found_char[i]] = char_dat_timer[char_num - 1];
								online_chars[sd->found_char[i]] = online_chars[char_num - 1];
								// Correct moved character reference in the character's owner
								for (j = 0; j < fd_max; j++) {
									if (session[j] && (sd2 = session[j]->session_data) &&
										sd2->account_id == char_dat[char_num - 1].account_id) {
										for (k = 0; k < 9; k++) {
											if (sd2->found_char[k] == char_num - 1) {
												sd2->found_char[k] = sd->found_char[i]; // found_char = index in db (TXT), char_id (SQL), -1: void
												break;
											}
										}
										break;
									}
								}
							}

							char_num--;
							for(ch = i; ch < 9 - 1; ch++)
								sd->found_char[ch] = sd->found_char[ch + 1];
							sd->found_char[8] = -1;
							WPACKETW(0) = 0x6f; // deleted!
							SENDPACKET(fd, 2);
							break;
						}
					}
					if (i == 9) { // reject
						WPACKETW(0) = 0x70;
						WPACKETB(2) = 0;
						SENDPACKET(fd, 3);
					}
				}
				RFIFOSKIP(fd, 46);
			}
		  }
			break;

		case 0x2af8: // login as map-server
			if (RFIFOREST(fd) < 56)
				return 0;
			if (!check_mapip(session[fd]->client_addr.sin_addr.s_addr)) {
				unsigned char *p = (unsigned char *) &session[fd]->client_addr.sin_addr;
				printf("Connection of a map-server REFUSED (map_allow, ip: %d.%d.%d.%d).\n", p[0], p[1], p[2], p[3]);
				printf("   Check your char_freya.conf (option: mapallowip)\n");
				printf("   if connection must be authorised.\n");
				WPACKETW(0) = 0x2af9;
				WPACKETB(2) = 3;
				SENDPACKET(fd, 3);
				/* set eof */
				session[fd]->eof = 1;
				RFIFOSKIP(fd,56);
			} else {
				for(i = 0; i < MAX_MAP_SERVERS; i++)
					if (server_fd[i] < 0)
						break;
				if (i == MAX_MAP_SERVERS || strncmp(RFIFOP(fd,2), userid, 24) || memcmp(RFIFOP(fd,26), passwd, 24)) {
					WPACKETW(0) = 0x2af9;
					WPACKETB(2) = 3;
					SENDPACKET(fd, 3);
					/* set eof */
					session[fd]->eof = 1;
					RFIFOSKIP(fd,56);
				} else {
					int len;
					WPACKETW(0) = 0x2af9;
					WPACKETB(2) = 0;
					SENDPACKET(fd, 3);
					session[fd]->func_parse = parse_frommap;
					server_fd[i] = fd;
					server_freezeflag[i] = anti_freeze_counter; // Map anti-freeze system. Counter. 6 ok, 5...0 frozen
					server[i].ip = RFIFOL(fd,50);
					server[i].port = RFIFOW(fd,54);
					server[i].users = 0;
					printf("User count: 0 (server: %d)\n", i);
					FREE(server[i].map);
					server[i].map_num = 0; // MAX_MAP_PER_SERVER
					RFIFOSKIP(fd,56);
					realloc_fifo(fd, RFIFOSIZE_SERVER, WFIFOSIZE_SERVER);
					inter_mapif_init(fd);
					// send gm acccounts level to map-servers
					len = 4;
					WPACKETW(0) = 0x2b15;
					for(i = 0; i < GM_num && len < 32760; i++) { // max size of packet = 32767
						WPACKETL(len    ) = gm_account[i].account_id;
						WPACKETB(len + 4) = gm_account[i].level;
						len += 5;
					}
					WPACKETW(2) = len;
					SENDPACKET(fd, len);
					// continue with one to one packet if quantity is too important
					while(i < GM_num) {
						WPACKETW(0) = 0x2b1f; // 0x2b1f <account_id>.L <GM_Level>.B
						WPACKETL(2) = gm_account[i].account_id;
						WPACKETB(6) = gm_account[i].level;
						SENDPACKET(fd, 7);
						i++;
					}
					return 0;
				}
			}
			break;

		case 0x187:	// Alive�M���H
			if (RFIFOREST(fd) < 6)
				return 0;
			RFIFOSKIP(fd, 6);
			break;

		case 0x7530:	// Request of the server version
			WPACKETW(0) = 0x7531;
			WPACKETB(2) = FREYA_MAJORVERSION;
			WPACKETB(3) = FREYA_MINORVERSION;
			WPACKETB(4) = FREYA_REVISION;
			WPACKETB(5) = FREYA_STATE;
			WPACKETB(6) = 0;
			WPACKETB(7) = FREYA_INTERVERSION | FREYA_CHARVERSION;
			WPACKETW(8) = 0;
			SENDPACKET(fd, 10);
			session[fd]->eof = 1;
			RFIFOSKIP(fd, 2);
			return 0;

		case 0x7532:	// Request of end of connection
			session[fd]->eof = 1;
			RFIFOSKIP(fd, 2);
			return 0;

		case 0x7533:	// Request of the server uptime
			WPACKETW(0) = 0x7534;
			WPACKETL(2) = time(NULL) - start_time;
			WPACKETW(6) = count_users();
			SENDPACKET(fd, 8);
			session[fd]->eof = 1;
			RFIFOSKIP(fd, 2);
			return 0;

		case 0x7535:	// Request of the server version (freya version)
			WPACKETW(0) = 0x7536;
			WPACKETB(2) = FREYA_MAJORVERSION;
			WPACKETB(3) = FREYA_MINORVERSION;
			WPACKETB(4) = FREYA_REVISION;
			WPACKETB(5) = FREYA_STATE;
			WPACKETB(6) = 0;
			WPACKETB(7) = FREYA_INTERVERSION | FREYA_CHARVERSION;
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
			RFIFOSKIP(fd, 2);
			return 0;

		default:
			session[fd]->eof = 1;
			return 0;
		}
	}

	return 0;
}

//---------------------------------
// Console Commands Parser by [Yor]
//---------------------------------
int parse_console(char *buf) {
	static int console_on = 1;
	char command[4096], param[4096];

	memset(command, 0, sizeof(command));
	memset(param, 0, sizeof(param));

	/* get param of command */
	sscanf(buf, "%s %[^\n]", command, param);

/*	printf("Console command: %s %s\n", command, param); */

	if (!console_on) {

		char_log("Console command - disabled: %s %s" RETCODE, command, param);

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

		char_log("Console command: %s %s" RETCODE, command, param);

		if (strcasecmp("shutdown", command) == 0 ||
		    strcasecmp("exit", command) == 0 ||
		    strcasecmp("quit", command) == 0 ||
		    strcasecmp("end", command) == 0) {
			exit(0);
		} else if (strcasecmp("alive", command) == 0 ||
		           strcasecmp("status", command) == 0 ||
		           strcasecmp("uptime", command) == 0) {
			printf(CL_DARK_CYAN "Console: " CL_RESET CL_BOLD "I'm Alive for %u seconds." CL_RESET "\n", (int)(time(NULL) - start_time));
			printf(CL_DARK_CYAN "Console: " CL_RESET CL_BOLD "Number of online player%s: %d." CL_RESET "\n", (count_users() > 1) ? "s" : "", count_users());
#ifdef TXT_ONLY
		} else if (strcasecmp("save", command) == 0) {
			mmo_char_sync();
			printf(CL_DARK_CYAN "Console: " CL_RESET CL_BOLD "Char database saved." CL_RESET "\n");
#endif /* TXT_ONLY */
		} else if (strcasecmp("online_files", command) == 0 ||
		           strcasecmp("online_file", command) == 0) {
			create_online_files();
			printf(CL_DARK_CYAN "Console: " CL_RESET CL_BOLD "Online files created." CL_RESET "\n");
		} else if (strcasecmp("?", command) == 0 ||
		           strcasecmp("h", command) == 0 ||
		           strcasecmp("help", command) == 0 ||
		           strcasecmp("aide", command) == 0) {
			printf(CL_DARK_GREEN "Help of commands:" CL_RESET "\n");
			printf("  '" CL_DARK_CYAN "shutdown|exit|qui|end" CL_RESET "': To shutdown the server.\n");
			printf("  '" CL_DARK_CYAN "alive|status|uptime" CL_RESET "': To know if server is alive.\n");
#ifdef TXT_ONLY
			printf("  '" CL_DARK_CYAN "save" CL_RESET "': To save database.\n");
#endif /* TXT_ONLY */
			printf("  '" CL_DARK_CYAN "online_file" CL_RESET "': To recreate online files.\n");
			printf("  '" CL_DARK_CYAN "?|h|help|aide" CL_RESET "': To display help.\n");
			printf("  '" CL_DARK_CYAN "consoleoff" CL_RESET "': To disable console commands.\n");
			printf("  '" CL_DARK_CYAN "version" CL_RESET "': To display version of the server.\n");
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

//-------------------------------
// Packet send to all map-servers
//-------------------------------
unsigned char mapif_sendall(unsigned int len) {
	int i;
	unsigned char c;
	int fd;

	c = 0;
	for(i = 0; i < MAX_MAP_SERVERS; i++) {
		if ((fd = server_fd[i]) >= 0) {
			SENDPACKET(fd, len);
			c++;
		}
	}

	return c;
}

//-------------------------------------------------------------------
// Packet send to all map-servers, except one (wos: without our self)
//-------------------------------------------------------------------
unsigned char mapif_sendallwos(int sfd, unsigned int len) {
	int i;
	unsigned char c;
	int fd;

	c = 0;
	for(i = 0; i < MAX_MAP_SERVERS; i++) {
		if ((fd = server_fd[i]) >= 0 && fd != sfd) {
			SENDPACKET(fd, len);
			c++;
		}
	}

	return c;
}

//---------------------------------
// Packet send to only 1 map-server
//---------------------------------
void mapif_send(int fd, unsigned int len) {
	int i;

	if (fd >= 0) {
		for(i = 0; i < MAX_MAP_SERVERS; i++) {
			if (fd == server_fd[i]) {
				SENDPACKET(fd, len);
				return;
			}
		}
	}

	return;
}

int check_connect_login_server(int tid, unsigned int tick, int id, int data) {
	if (login_fd <= 0 || session[login_fd] == NULL) {
		printf("Attempting to connect to login-server (%s:%d). Please wait...\n", login_ip_str, login_port);
		login_fd = make_connection(login_ip, login_port);
		if (login_fd != -1) {
			session[login_fd]->func_parse = parse_tologin;
			realloc_fifo(login_fd, RFIFOSIZE_SERVER, WFIFOSIZE_SERVER);
			WPACKETW( 0) = 0x2710;
			strncpy(WPACKETP( 2), userid, 24);
			strncpy(WPACKETP(26), passwd, 24);
			WPACKETL(50) = 0;
			WPACKETL(54) = char_ip;
			WPACKETW(58) = char_port;
			strncpy(WPACKETP(60), server_name, 20);
			WPACKETW(80) = 0;
			WPACKETW(82) = char_maintenance;
			WPACKETW(84) = char_new;
			SENDPACKET(login_fd, 86);
		} else
			printf("Can not connect to login-server.\n");
	}

	return 0;
}

#ifdef USE_SQL
void sql_config_read(const char *cfgName) {
	char line[1024], w1[1024], w2[1024];
	FILE *fp;

	printf("Reading config file: %s\n", cfgName);

	if ((fp = fopen(cfgName, "r")) == NULL) {
//		if ((fp = fopen("conf/inter_freya.conf", "r")) == NULL) { // not try default, possible infinite loop with import
			printf("File not found: %s\n", cfgName);
			exit(1);
//		}
	}

	while(fgets(line, sizeof(line), fp)) { // fgets reads until maximum one less than size and add '\0' -> so, it's not necessary to add -1
		if ((line[0] == '/' && line[1] == '/') || line[0] == '\0' || line[0] == '\n' || line[0] == '\r')
			continue;
		// it's not necessary to remove 'carriage return ('\n' or '\r')

		memset(w2, 0, sizeof(w2));
		if (sscanf(line, "%[^:]: %[^\r\n]", w1, w2) != 2)
			continue;

		if (strcasecmp(w1, "char_db") == 0) {
			memset(char_db, 0, sizeof(char_db));
			strncpy(char_db, w2, sizeof(char_db) - 1);
		} else if (strcasecmp(w1, "cart_db") == 0) {
			memset(cart_db, 0, sizeof(cart_db));
			strncpy(cart_db, w2, sizeof(cart_db) - 1);
		} else if (strcasecmp(w1, "inventory_db") == 0) {
			memset(inventory_db, 0, sizeof(inventory_db));
			strncpy(inventory_db, w2, sizeof(inventory_db) - 1);
		} else if (strcasecmp(w1, "charlog_db") == 0) {
			memset(charlog_db, 0, sizeof(charlog_db));
			strncpy(charlog_db, w2, sizeof(charlog_db) - 1);
		} else if (strcasecmp(w1, "storage_db") == 0) {
			memset(storage_db, 0, sizeof(storage_db));
			strncpy(storage_db, w2, sizeof(storage_db) - 1);
		} else if (strcasecmp(w1, "reg_db") == 0) {
			memset(global_reg_value, 0, sizeof(global_reg_value));
			strncpy(global_reg_value, w2, sizeof(global_reg_value) - 1);
		} else if (strcasecmp(w1, "skill_db") == 0) {
			memset(skill_db, 0, sizeof(skill_db));
			strncpy(skill_db, w2, sizeof(skill_db) - 1);
		} else if (strcasecmp(w1, "interlog_db") == 0) {
			memset(interlog_db, 0, sizeof(interlog_db));
			strncpy(interlog_db, w2, sizeof(interlog_db) - 1);
		} else if (strcasecmp(w1, "memo_db") == 0) {
			memset(memo_db, 0, sizeof(memo_db));
			strncpy(memo_db, w2, sizeof(memo_db) - 1);
		} else if (strcasecmp(w1, "guild_db") == 0) {
			memset(guild_db, 0, sizeof(guild_db));
			strncpy(guild_db, w2, sizeof(guild_db) - 1);
		} else if (strcasecmp(w1, "guild_alliance_db") == 0) {
			memset(guild_alliance_db, 0, sizeof(guild_alliance_db));
			strncpy(guild_alliance_db, w2, sizeof(guild_alliance_db) - 1);
		} else if (strcasecmp(w1, "guild_castle_db") == 0) {
			memset(guild_castle_db, 0, sizeof(guild_castle_db));
			strncpy(guild_castle_db, w2, sizeof(guild_castle_db) - 1);
		} else if (strcasecmp(w1, "guild_expulsion_db") == 0) {
			memset(guild_expulsion_db, 0, sizeof(guild_expulsion_db));
			strncpy(guild_expulsion_db, w2, sizeof(guild_expulsion_db) - 1);
		} else if (strcasecmp(w1, "guild_member_db") == 0) {
			memset(guild_member_db, 0, sizeof(guild_member_db));
			strncpy(guild_member_db, w2, sizeof(guild_member_db) - 1);
		} else if (strcasecmp(w1, "guild_skill_db") == 0) {
			memset(guild_skill_db, 0, sizeof(guild_skill_db));
			strncpy(guild_skill_db, w2, sizeof(guild_skill_db) - 1);
		} else if (strcasecmp(w1, "guild_position_db") == 0) {
			memset(guild_position_db, 0, sizeof(guild_position_db));
			strncpy(guild_position_db, w2, sizeof(guild_position_db) - 1);
		} else if (strcasecmp(w1, "guild_storage_db") == 0) {
			memset(guild_storage_db, 0, sizeof(guild_storage_db));
			strncpy(guild_storage_db, w2, sizeof(guild_storage_db) - 1);
		} else if (strcasecmp(w1, "party_db") == 0) {
			memset(party_db, 0, sizeof(party_db));
			strncpy(party_db, w2, sizeof(party_db) - 1);
		} else if (strcasecmp(w1, "pet_db") == 0) {
			memset(pet_db, 0, sizeof(pet_db));
			strncpy(pet_db, w2, sizeof(pet_db) - 1);
		} else if(strcasecmp(w1, "statuschange_db") == 0) {
			memset(statuschange_db, 0, sizeof(statuschange_db));
			strncpy(statuschange_db, w2, sizeof(statuschange_db) - 1);
		} else if (strcasecmp(w1, "rank_db") == 0) {
			memset(rank_db, 0, sizeof(rank_db));
			strncpy(rank_db, w2, sizeof(rank_db) - 1);
// import
		} else if (strcasecmp(w1, "import") == 0) {
			printf("sql_config_read: Import file: %s.\n", w2);
			sql_config_read(w2);
		}
	}
	fclose(fp);

	printf("reading configure done.\n");
}
#endif /* USE_SQL */

//-----------------------------------
// Reading general configuration file
//-----------------------------------
static void char_config_read(const char *cfgName) { // not inline, called too often
	int j;
	struct hostent *h = NULL;
	char line[1024], w1[1024], w2[1024];
	FILE *fp;

	// set default configuration
	memset(lan_map_ip, 0, sizeof(lan_map_ip));
	strncpy(lan_map_ip, "127.0.0.1", sizeof(lan_map_ip) - 1);
	subnet[0] = 127;
	subnet[1] = 0;
	subnet[2] = 0;
	subnet[3] = 1;
	for(j = 0; j < 4; j++)
		subnetmask[j] = 255;

	if ((fp = fopen(cfgName, "r")) == NULL) {
//		if ((fp = fopen("conf/char_freya.conf", "r")) == NULL) { // not try default, possible infinite loop with import
			printf("Configuration file not found: %s.\n", cfgName);
			exit(1);
//		}
	}

	while(fgets(line, sizeof(line), fp)) { // fgets reads until maximum one less than size and add '\0' -> so, it's not necessary to add -1
		if ((line[0] == '/' && line[1] == '/') || line[0] == '\0' || line[0] == '\n' || line[0] == '\r')
			continue;
		// it's not necessary to remove 'carriage return ('\n' or '\r')

		memset(w2, 0, sizeof(w2));
		if (sscanf(line, "%[^:]: %[^\r\n]", w1, w2) != 2)
			continue;

		remove_control_chars(w1);
		remove_control_chars(w2);
		if (strcasecmp(w1, "userid") == 0) {
			memset(userid, 0, sizeof(userid));
			memcpy(userid, w2, 24);
		} else if (strcasecmp(w1, "passwd") == 0) {
			memset(passwd, 0, sizeof(passwd));
			memcpy(passwd, w2, 24);
		} else if (strcasecmp(w1, "server_name") == 0) {
			memset(server_name, 0, sizeof(server_name));
			strncpy(server_name, w2, 20);
			printf(CL_WHITE "%s" CL_RESET " server has been initialized.\n", w2);
		} else if (strcasecmp(w1, "wisp_server_name") == 0) {
			if (strlen(w2) >= 4) {
				memset(wisp_server_name, 0, sizeof(wisp_server_name));
				strncpy(wisp_server_name, w2, 24);
			}
		} else if (strcasecmp(w1, "login_ip") == 0) {
			memset(login_ip_str, 0, sizeof(login_ip_str));
			login_ip_set_ = 1;
			h = gethostbyname(w2);
			if (h != NULL) {
				printf("Login server IP address : %s -> %d.%d.%d.%d\n", w2, (unsigned char)h->h_addr[0], (unsigned char)h->h_addr[1], (unsigned char)h->h_addr[2], (unsigned char)h->h_addr[3]);
				sprintf(login_ip_str, "%d.%d.%d.%d", (unsigned char)h->h_addr[0], (unsigned char)h->h_addr[1], (unsigned char)h->h_addr[2], (unsigned char)h->h_addr[3]);
			} else
				memcpy(login_ip_str, w2, 16);
		} else if (strcasecmp(w1, "login_port") == 0) {
			login_port = atoi(w2);
		} else if (strcasecmp(w1, "char_ip") == 0) {
			memset(char_ip_str, 0, sizeof(char_ip_str));
			char_ip_set_ = 1;
			h = gethostbyname(w2);
			if (h != NULL) {
				printf("Character server IP address : %s -> %d.%d.%d.%d\n", w2, (unsigned char)h->h_addr[0], (unsigned char)h->h_addr[1], (unsigned char)h->h_addr[2], (unsigned char)h->h_addr[3]);
				sprintf(char_ip_str, "%d.%d.%d.%d", (unsigned char)h->h_addr[0], (unsigned char)h->h_addr[1], (unsigned char)h->h_addr[2], (unsigned char)h->h_addr[3]);
			} else
				memcpy(char_ip_str, w2, 16);
		} else if (strcasecmp(w1, "char_port") == 0) {
			char_port = atoi(w2);
		} else if (strcasecmp(w1, "listen_ip") == 0) {
			memset(listen_ip, 0, sizeof(listen_ip));
			h = gethostbyname(w2);
			if (h != NULL) {
				sprintf(listen_ip, "%d.%d.%d.%d", (unsigned char)h->h_addr[0], (unsigned char)h->h_addr[1], (unsigned char)h->h_addr[2], (unsigned char)h->h_addr[3]);
			} else {
				strncpy(listen_ip, w2, 15); // 16 - NULL
			}
		} else if (strcasecmp(w1, "char_maintenance") == 0) {
			char_maintenance = atoi(w2);
		} else if (strcasecmp(w1, "char_new") == 0) {
			char_new = atoi(w2);
		} else if (strcasecmp(w1, "email_creation") == 0) {
			email_creation = config_switch(w2);
#ifdef TXT_ONLY
		} else if (strcasecmp(w1, "char_txt") == 0) {
			strcpy(char_txt, w2);
		} else if (strcasecmp(w1, "friends_txt") == 0) {
			strcpy(friends_txt, w2);
#endif /* TXT_ONLY */
		} else if (strcasecmp(w1, "max_connect_user") == 0) {
			max_connect_user = atoi(w2);
			if (max_connect_user < 0)
				max_connect_user = 0; // unlimited online players
		} else if (strcasecmp(w1, "min_level_to_bypass") == 0) { // max_connect_user bypass [PoW]
			min_level_to_bypass = atoi(w2);
//			if (min_level_to_bypass <= 0 || min_level_to_bypass > 99)
			if (min_level_to_bypass > 99)
				min_level_to_bypass = 100; // Disallow min_level_to_bypass to be set at 100 [PoW]
		} else if (strcasecmp(w1, "check_ip_flag") == 0) {
			check_ip_flag = config_switch(w2);
		} else if (strcasecmp(w1, "check_authfifo_login2") == 0) {
			check_authfifo_login2 = config_switch(w2);
		} else if (strcasecmp(w1, "autosave_time") == 0) {
			if (atoi(w2) > 0)
				autosave_interval = atoi(w2) * 1000;
		} else if (strcasecmp(w1, "log_char") == 0) {
			log_char = config_switch(w2);
		} else if (strcasecmp(w1, "char_log_filename") == 0) {
			memset(char_log_filename, 0, sizeof(char_log_filename));
			strcpy(char_log_filename, w2);
		} else if (strcasecmp(w1, "log_file_date") == 0) {
			log_file_date = atoi(w2);
			if (log_file_date > 4)
				log_file_date = 3; // default
		} else if (strcasecmp(w1, "chars_per_account") == 0) {
			if (atoi(w2) >= 0 && atoi(w2) <= 9)
				chars_per_account = atoi(w2);

/* Anti-freeze options */
		} else if (strcasecmp(w1, "anti_freeze_counter") == 0) {
			if (atoi(w2) > 1)
				anti_freeze_counter = atoi(w2);
		} else if (strcasecmp(w1, "anti_freeze_interval") == 0) {
			if (atoi(w2) >= 0)
				anti_freeze_interval = atoi(w2);

/* Characters' creation options */
		} else if (strcasecmp(w1, "start_point") == 0) {
			char map[1024]; // same size of line to avoid overflow
			int x, y;
			if (sscanf(w2, "%[^,],%d,%d", map, &x, &y) < 3) {
				printf(CL_YELLOW "WARNING: Invalid start point in configuration file (invalid number of parameters):" CL_RESET " %s.\n", w2);
				continue;
			}
			if ((strstr(map, ".gat") != NULL || strlen(map) > 16)) { // Verify at least if '.gat' is in the map name
				if (x > 0 && x < 32767 && y > 0 && y < 32767) { // verify at least a minimum for the coordinates
					memset(start_point.map, 0, sizeof(start_point.map));
					strncpy(start_point.map, map, 16); // 17 - NULL
					start_point.x = x;
					start_point.y = y;
				} else
					printf(CL_YELLOW "WARNING: Invalid start point in configuration file (invalid coordinates):" CL_RESET " %s.\n", w2);
			} else
				printf(CL_YELLOW "WARNING: Invalid start point in configuration file (invalid map name):" CL_RESET " %s.\n", w2);
		} else if (strcasecmp(w1, "start_zeny") == 0) {
			start_zeny = atoi(w2);
			if (start_zeny < 0)
				start_zeny = 0;
		} else if (strcasecmp(w1, "start_weapon") == 0) {
			start_weapon = atoi(w2);
			if (start_weapon < 0)
				start_weapon = 0;
		} else if (strcasecmp(w1, "start_armor") == 0) {
			start_armor = atoi(w2);
			if (start_armor < 0)
				start_armor = 0;
		} else if (strcasecmp(w1, "start_item") == 0) {
			start_item = atoi(w2);
			if (start_item < 0)
				start_item = 0;
		} else if (strcasecmp(w1, "start_item_quantity") == 0) {
			start_item_quantity = atoi(w2);
			if (start_item_quantity < 0)
				start_item_quantity = 0;
		} else if (strcasecmp(w1, "max_hair_style") == 0) {
			max_hair_style = atoi(w2);
			if (max_hair_style < 1)
				max_hair_style = 1;
		} else if (strcasecmp(w1, "max_hair_color") == 0) {
			max_hair_color = atoi(w2);
			if (max_hair_color < 1)
				max_hair_color = 1;

/* Characters names options */
		} else if (strcasecmp(w1, "unknown_char_name") == 0) {
			memset(unknown_char_name, 0, sizeof(unknown_char_name));
			strncpy(unknown_char_name, w2, 24);
		} else if (strcasecmp(w1, "name_ignoring_case") == 0) {
			name_ignoring_case = config_switch(w2);
		} else if (strcasecmp(w1, "char_name_option") == 0) {
			char_name_option = atoi(w2);
		} else if (strcasecmp(w1, "char_name_letters") == 0) {
			memset(char_name_letters, 0, sizeof(char_name_letters));
			strcpy(char_name_letters, w2);
		} else if (strcasecmp(w1, "char_name_with_spaces") == 0) {
			char_name_with_spaces = config_switch(w2);
		} else if (strcasecmp(w1, "char_name_double_spaces") == 0) {
			char_name_double_spaces = config_switch(w2);
		} else if (strcasecmp(w1, "char_name_space_at_first") == 0) {
			char_name_space_at_first = config_switch(w2);
		} else if (strcasecmp(w1, "char_name_space_at_last") == 0) {
			char_name_space_at_last = config_switch(w2);
		} else if (strcasecmp(w1, "char_name_language") == 0) {
			if (strlen(w2) > 5) {
				printf("Manner language not changed (bad value detected: len > 5).\n");
			} else {
				memset(char_name_language, 0, sizeof(char_name_language));
				strcpy(char_name_language, w2);
				//printf("Set manner language to '%s'.\n", char_name_language);
			}
		} else if (strcasecmp(w1, "char_name_language_check") == 0) {
			char_name_language_check = config_switch(w2);

/* online files options */
		} else if (strcasecmp(w1, "online_txt_filename") == 0) {
			memset(online_txt_filename, 0, sizeof(online_txt_filename));
			strcpy(online_txt_filename, w2);
		} else if (strcasecmp(w1, "online_html_filename") == 0) {
			memset(online_html_filename, 0, sizeof(online_html_filename));
			strcpy(online_html_filename, w2);
		} else if (strcasecmp(w1, "online_php_filename") == 0) {
			memset(online_php_filename, 0, sizeof(online_php_filename));
			strcpy(online_php_filename, w2);
		} else if (strcasecmp(w1, "online_sorting_option") == 0) {
			online_sorting_option = atoi(w2);
		} else if (strcasecmp(w1, "online_display_option_txt") == 0) {
			online_display_option_txt = atoi(w2);
		} else if (strcasecmp(w1, "online_display_option_html") == 0) {
			online_display_option_html = atoi(w2);
		} else if (strcasecmp(w1, "online_display_option_php") == 0) {
			online_display_option_php = config_switch(w2);
		} else if (strcasecmp(w1, "online_gm_display_min_level") == 0) { // minimum GM level to display 'GM' when we want to display it
			online_gm_display_min_level = atoi(w2);
			if (online_gm_display_min_level < 1)
				online_gm_display_min_level = 1;
		} else if (strcasecmp(w1, "online_refresh_html") == 0) {
			online_refresh_html = atoi(w2);
			if (online_refresh_html < 5) // send online file every 5 seconds to player is enough
				online_refresh_html = 5;
		} else if (strcasecmp(w1, "console") == 0) {
			console = config_switch(w2);
		} else if (strcasecmp(w1, "console_pass") == 0) {
			memset(console_pass, 0, sizeof(console_pass));
			strncpy(console_pass, w2, sizeof(console_pass) - 1);

/* Maps-servers connection security */
		} else if (strcasecmp(w1, "mapallowip") == 0) {
			if (strcasecmp(w2, "clear") == 0) {
				FREE(access_map_allow);
				access_map_allownum = 0;
			} else {
				if (strcasecmp(w2, "all") == 0) {
					/* reset all previous values */
					FREE(access_map_allow);
					/* set to all */
					CALLOC(access_map_allow, char, ACO_STRSIZE);
					access_map_allownum = 1;
					//access_map_allow[0] = '\0';
				} else if (w2[0] && !(access_map_allownum == 1 && access_map_allow[0] == '\0')) { /* don't add IP if already 'all' */
					if (access_map_allow) {
						REALLOC(access_map_allow, char, (access_map_allownum + 1) * ACO_STRSIZE);
						memset(access_map_allow + (access_map_allownum * ACO_STRSIZE), 0, sizeof(char) * ACO_STRSIZE);
					} else {
						CALLOC(access_map_allow, char, ACO_STRSIZE);
					}
					strncpy(access_map_allow + (access_map_allownum++) * ACO_STRSIZE, w2, ACO_STRSIZE - 1); // 32 - NULL
					access_map_allow[access_map_allownum * ACO_STRSIZE - 1] = '\0';
				}
			}

/* lan options */
		} else if (strcasecmp(w1, "lan_map_ip") == 0) { // Read map-server Lan IP Address
			memset(lan_map_ip, 0, sizeof(lan_map_ip));
			h = gethostbyname(w2);
			if (h != NULL) {
				sprintf(lan_map_ip, "%d.%d.%d.%d", (unsigned char)h->h_addr[0], (unsigned char)h->h_addr[1], (unsigned char)h->h_addr[2], (unsigned char)h->h_addr[3]);
			} else {
				strncpy(lan_map_ip, w2, sizeof(lan_map_ip));
				lan_map_ip[sizeof(lan_map_ip)-1] = 0;
			}
		} else if (strcasecmp(w1, "subnet") == 0) { // Read Subnetwork
			for(j = 0; j < 4; j++)
				subnet[j] = 0;
			h = gethostbyname(w2);
			if (h != NULL) {
				for(j = 0; j < 4; j++)
					subnet[j] = (unsigned char)h->h_addr[j];
			} else {
				sscanf(w2, "%d.%d.%d.%d", &subnet[0], &subnet[1], &subnet[2], &subnet[3]);
			}
		} else if (strcasecmp(w1, "subnetmask") == 0) { // Read Subnetwork Mask
			for(j = 0; j < 4; j++)
				subnetmask[j] = 255;
			h = gethostbyname(w2);
			if (h != NULL) {
				for(j = 0; j < 4; j++)
					subnetmask[j] = (unsigned char)h->h_addr[j];
			} else {
				sscanf(w2, "%d.%d.%d.%d", &subnetmask[0], &subnetmask[1], &subnetmask[2], &subnetmask[3]);
			}

// addon
		} else if (strcasecmp(w1, "addon") == 0) {
			addons_load(w2, ADDONS_CHAR);

// import
		} else if (strcasecmp(w1, "import") == 0) {
			printf("char_config_read: Import file: %s.\n", w2);
			char_config_read(w2);
		}
	}
	fclose(fp);

	// display lan options
	printf("---Lan Support configuration...\n");
	printf("LAN IP of map-server: %s.\n", lan_map_ip);
	printf("Sub-network of the map-server: %d.%d.%d.%d.\n", subnet[0], subnet[1], subnet[2], subnet[3]);
	printf("Sub-network mask of the map-server: %d.%d.%d.%d.\n", subnetmask[0], subnetmask[1], subnetmask[2], subnetmask[3]);

	// sub-network check of the map-server
  {
	unsigned int a0, a1, a2, a3;
	unsigned char p[4];
	if (sscanf(lan_map_ip, "%u.%u.%u.%u", &a0, &a1, &a2, &a3) < 4 || a0 > 255 || a1 > 255 || a2 > 255 || a3 > 255) // < 0 is always correct (unsigned)
		printf(CL_RED "***ERROR: Incorrect LAN IP of the map-server (a value is < 0 or > 255)." CL_RESET "\n");
	else {
		p[0] = a0; p[1] = a1; p[2] = a2; p[3] = a3;
		printf("LAN test of LAN IP of the map-server: ");
		if (lan_ip_check(p) == 0)
			printf(CL_RED "***ERROR: LAN IP of the map-server doesn't belong to the specified Sub-network." CL_RESET "\n");
	}
  }
	printf("---End of Lan Support configuration.\n");

	return;
}

//--------------------------------------
// Function called at exit of the server
//--------------------------------------
void do_final(void) {
	int i;

	printf("Terminating...\n");

	/* send all packets not sended */
	flush_fifos();

	// write online players files with no player
	for(i = 0; i < char_num; i++)
		online_chars[i] = 0;
	create_online_files();
	FREE(online_chars);
#ifdef USE_SQL
	// write online players files with no player
	sql_request("UPDATE `%s` SET `online`=0", char_db);
#endif /* USE_SQL */

#ifdef TXT_ONLY
	mmo_char_sync();
#endif // TXT_ONLY
	inter_save();

#ifdef USE_SQL
	sql_request("DELETE FROM `ragsrvinfo`");
#endif /* USE_SQL */

	FREE(gm_account);

#ifdef TXT_ONLY
	for(i = 0; i < char_num; i++) {
		FREE(friend_dat[i].friends);
	}
	FREE(friend_dat);
#endif // TXT_ONLY
	for(i = 0; i < char_num; i++) {
		FREE(global_reg_db[i].global_reg);
	}
	FREE(global_reg_db);
	FREE(char_dat_timer);
	FREE(char_dat);

	for(i = 0; i < account_reg2_db_num; i++) {
		FREE(account_reg2_db[i].reg2);
	}
	FREE(account_reg2_db);
	account_reg2_db_num = 0;

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

	if (char_fd != -1) {
#ifdef __WIN32
		shutdown(char_fd, SD_BOTH);
		closesocket(char_fd);
#else
		close(char_fd);
#endif
		delete_session(char_fd);
		char_fd = -1;
	}

	for(i = 0; i < fd_max; i++)
		if (session[i]) {
#ifdef __WIN32
			if (i > 0) { // not console
				shutdown(i, SD_BOTH);
				closesocket(i);
			}
#else
			close(i);
#endif
			delete_session(i);
		}

	delete_manner();

	for(i = 0; i < MAX_MAP_SERVERS; i++) {
		FREE(server[i].map);
		server[i].map_num = 0; // MAX_MAP_PER_SERVER
	}

	// Maps-servers connection security
	FREE(access_map_allow);
	access_map_allownum = 0;

	/* restore console parameters */
	term_input_disable();

#ifdef USE_SQL
	sql_close();
#endif /* USE_SQL */

#ifdef __WIN32
	// close windows sockets
	WSACleanup();
#endif /* __WIN32 */

	char_log("----End of char-server (normal end with closing of all files)." RETCODE);

	if (log_fp != NULL)
		fclose(log_fp);

	printf("Finished.\n");
}

//-----------------------------
// Main function of char-server
//-----------------------------
void do_init(const int argc, char **argv) {
	int i;

	// a newline in the log...
	char_log("");
	char_log("The char-server is starting..." RETCODE);
	printf("The char-server is starting...\n");

	char_config_read((argc < 2) ? "conf/char_freya.conf" : argv[1]);
#ifdef USE_SQL
	sql_config_read(SQL_CONF_NAME);
#endif /* USE_SQL */

	if (login_ip_set_ == 0 || char_ip_set_ == 0) {
		// The char server should know what IP address it is running on
		int localaddr = ntohl(addr_[0]);
		unsigned char *ptr = (unsigned char *) &localaddr;
		char buf[16];
		sprintf(buf, "%d.%d.%d.%d", ptr[0], ptr[1], ptr[2], ptr[3]);
		if (naddr_ != 1)
			printf("Multiple interfaces detected. Using %s as our IP address.\n", buf);
		else
			printf("Defaulting to %s as our IP address.\n", buf);
		if (login_ip_set_ == 0)
			strcpy(login_ip_str, buf);
		if (char_ip_set_ == 0)
			strcpy(char_ip_str, buf);
	}

	printf("charserver configuration reading done.\n");

	login_ip = inet_addr(login_ip_str);
	char_ip = inet_addr(char_ip_str);

	login_fd = -1;
	for(i = 0; i < MAX_MAP_SERVERS; i++) {
		memset(&server[i], 0, sizeof(struct mmo_map_server));
		server_fd[i] = -1;
	}

	inter_init((argc > 2) ? argv[2] : "conf/inter_freya.conf"); // Inter server initialization

#ifdef TXT_ONLY
	friends_txt_flag = 0; // must we save friends file or not? (0: not, x: yes)
#endif /* TXT_ONLY */
	mmo_char_init();

	// searching char_id for the server (based on wisp_server_name)
	found_server_char_id();

	update_online = time(NULL);
	create_online_files(); // update online players files at start of the server

	atexit(do_final);
//	set_termfunc(do_final);
	set_defaultparse(parse_char);

	gm_account = NULL;
	GM_num = 0;

	read_manner(); // read forbidden words

	char_fd = make_listen_port(char_port);

	add_timer_func_list(check_connect_login_server, "check_connect_login_server");
	add_timer_func_list(send_users_tologin, "send_users_tologin");
	add_timer_func_list(mmo_char_sync_timer, "mmo_char_sync_timer");
	add_timer_func_list(map_anti_freeze_system, "map_anti_freeze_system");
	add_timer_func_list(check_manner_file, "check_manner_file");
	add_timer_func_list(check_account_reg2, "check_account_reg2");

	i = add_timer_interval(gettick_cache + 1000, check_connect_login_server, 0, 0, 5 * 1000);
	i = add_timer_interval(gettick_cache + 1000, send_users_tologin, 0, 0, 5 * 1000);
#ifdef TXT_ONLY
	i = add_timer_interval(gettick_cache + autosave_interval, mmo_char_sync_timer, 0, 0, autosave_interval);
#else
	i = add_timer_interval(gettick_cache + 30 * 1000, mmo_char_sync_timer, 0, 0, 30 * 1000); // to check characters in memory and free if not used for 15 seconds
#endif
	if (anti_freeze_interval == 0)
		i = add_timer_interval(gettick_cache + 6000, map_anti_freeze_system, 0, 0, 6 * 1000); // every 6 sec (users are sended every 5 sec)
	else
		i = add_timer_interval(gettick_cache + anti_freeze_interval * 1000, map_anti_freeze_system, 0, 0, anti_freeze_interval * 1000); // every 6 sec (users are sended every 5 sec)
	i = add_timer_interval(gettick_cache + 60000, check_manner_file, 0, 0, 60000); // every 60 sec we check if manner file has been changed
	i = add_timer_interval(gettick_cache + 300000, check_account_reg2, 0, 0, 300000); // every 300 sec (5 minutes) we check account reg2 to clean up it

#ifdef DYNAMIC_LINKING
	addons_enable_all();
#endif

	if (strcmp(listen_ip, "0.0.0.0") == 0) {
		char_log("The char-server is ready (listening on the port %d - from any ip)." RETCODE, char_port);
		printf("The char-server is " CL_GREEN "ready" CL_RESET " (listening on the port " CL_WHITE "%d" CL_RESET " - from any ip).\n", char_port);
	} else {
		char_log("The char-server is ready (listening on %s:%d)." RETCODE, listen_ip, char_port);
		printf("The char-server is " CL_GREEN "ready" CL_RESET " (listening on " CL_WHITE "%s:%d" CL_RESET ").\n", listen_ip, char_port);
	}

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
