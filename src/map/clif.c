// $Id: clif.c 579 2005-12-08 11:02:24Z Yor $

#define DUMP_UNKNOWN_PACKET 1
#include <config.h>

#include <stdio.h>
#include <ctype.h> // tolower
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#ifdef __WIN32
#define __USE_W32_SOCKETS
#include <windows.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif
#include <time.h>
#include <sys/stat.h> // for stat/lstat/fstat

#include "../common/socket.h"
#include "../common/core.h"
#include "../common/timer.h"
#include "../common/malloc.h"
#include "../common/version.h"
#include "nullpo.h"

#include "map.h"
#include "chrif.h"
#include "clif.h"
#include "pc.h"
#include "npc.h"
#include "itemdb.h"
#include "chat.h"
#include "trade.h"
#include "storage.h"
#include "script.h"
#include "skill.h"
#include "atcommand.h"
#include "intif.h"
#include "battle.h"
#include "mob.h"
#include "party.h"
#include "guild.h"
#include "vending.h"
#include "pet.h"
#include "status.h"
#include "ranking.h"

#ifdef MEMWATCH
#include "memwatch.h"
#endif

#define STATE_BLIND 0x10

#ifdef ANTIBOT_SYSTEM
int fake_mob_list[] = { // set here mobs that do not sound when they don't move
	1001, // scorpion
	1002, // poring
	1007, // fabre
	1031, // poporing
	1022  // werewolf
};
#endif

#define MAX_PACKET_DB   0x260
#define MAX_PACKET_VERSION 14
#define PACKETVER           6 //default protocol version

static const int packet_len_table[MAX_PACKET_DB] = {
   10,  0,  0,  0,  0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,
//#0x0040
    0,  0,  0,  0,  0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0, 55, 17,  3, 37,  46, -1, 23, -1,  3,108,  3,  2,
    3, 28, 19, 11,  3, -1,  9,  5,  54, 53, 58, 60, 41,  2,  6,  6,
//#0x0080
    7,  3,  2,  2,  2,  5, 16, 12,  10,  7, 29, 23, -1, -1, -1,  0, // 0x8b unknown... size 2 or 23?
    7, 22, 28,  2,  6, 30, -1, -1,   3, -1, -1,  5,  9, 17, 17,  6,
   23,  6,  6, -1, -1, -1, -1,  8,   7,  6,  7,  4,  7,  0, -1,  6,
    8,  8,  3,  3, -1,  6,  6, -1,   7,  6,  2,  5,  6, 44,  5,  3,
//#0x00C0
    7,  2,  6,  8,  6,  7, -1, -1,  -1, -1,  3,  3,  6,  6,  2, 27, // 0xcd: 3 or 6 (account_id = 4, so probably 6)
    3,  4,  4,  2, -1, -1,  3, -1,   6, 14,  3, -1, 28, 29, -1, -1,
   30, 30, 26,  2,  6, 26,  3,  3,   8, 19,  5,  2,  3,  2,  2,  2,
    3,  2,  6,  8, 21,  8,  8,  2,   2, 26,  3, -1,  6, 27, 30, 10,

//#0x0100
    2,  6,  6, 30, 79, 31, 10, 10,  -1, -1,  4,  6,  6,  2, 11, -1,
   10, 39,  4, 10, 31, 35, 10, 18,   2, 13, 15, 20, 68,  2,  3, 16,
    6, 14, -1, -1, 21,  8,  8,  8,   8,  8,  2,  2,  3,  4,  2, -1,
    6, 86,  6, -1, -1,  7, -1,  6,   3, 16,  4,  4,  4,  6, 24, 26,
//#0x0140
   22, 14,  6, 10, 23, 19,  6, 39,   8,  9,  6, 27, -1,  2,  6,  6,
  110,  6, -1, -1, -1, -1, -1,  6,  -1, 54, 66, 54, 90, 42,  6, 42,
   -1, -1, -1, -1, -1, 30, -1,  3,  14,  3, 30, 10, 43, 14,186,182,
   14, 30, 10,  3, -1,  6,106, -1,   4,  5,  4, -1,  6,  7, -1, -1,
//#0x0180
    6,  3,106, 10, 10, 34,  0,  6,   8,  4,  4,  4, 29, -1, 10,  6,
   90, 86, 24,  6, 30,102,  9,  4,   8,  4, 14, 10, -1,  6,  2,  6,
    3,  3, 35,  5, 11, 26, -1,  4,   4,  6, 10, 12,  6, -1,  4,  4,
   11,  7, -1, 67, 12, 18,114,  6,   3,  6, 26, 26, 26, 26,  2,  3,
//#0x01C0,   Set 0x1d5=-1
    2, 14, 10, -1, 22, 22,  4,  2,  13, 97,  3,  9,  9, 30,  6, 28,
    8, 14, 10, 35,  6, -1,  4, 11,  54, 53, 60,  2, -1, 47, 33,  6,
   30,  8, 34, 14,  2,  6, 26,  2,  28, 81,  6, 10, 26,  2, -1, -1,
   -1, -1, 20, 10, 32,  9, 34, 14,   2,  6, 48, 56, -1,  4,  5, 10,
//#0x200
   26, -1,  26, 10, 18, 26, 11, 34,  14, 36, 10,  0,  0, -1, 24,  0,
    0,  0,   0,  0,  0,  0,  0,  0,   0,282,282,  0,  0,  0,  0,  0,
    0,  0,   0,  0,  0,  0,282,  0,   0, 15, 58, 57, 64,  0,  0,  0,
    0,  0,   0,  0,  0,  0,  0,  0, 282,  0,  0,  0,  0,  0,  0,  0,
//#0x240
    0,  0,   0,  0,  0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,   0,  0,  0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,
};

// size list for each packet version after packet version 4.
static int packet_size_table[MAX_PACKET_VERSION][MAX_PACKET_DB];

// local define
enum {
	ALL_CLIENT,
	ALL_SAMEMAP,
	AREA,
	AREA_WOS,
	AREA_WOC,
	AREA_WOSC,
	AREA_PET, // to send pet information (hair style) with packet 0x0078 and 0x007b
	AREA_CHAT_WOC,
	CHAT,
	CHAT_WOS,
	PARTY,
	PARTY_WOS,
	PARTY_SAMEMAP,
	PARTY_SAMEMAP_WOS,
	PARTY_AREA,
	PARTY_AREA_WOS,
	GUILD,
	GUILD_WOS,
	GUILD_SAMEMAP,
	GUILD_SAMEMAP_WOS,
	GUILD_AREA,
	GUILD_AREA_WOS,
	SELF
};

#define WBUFPOS(p,pos,x,y) { unsigned char *__p = (unsigned char *)(p); __p += (pos); __p[0] = (x) >> 2; __p[1] = ((x) << 6) | (((y) >> 4) & 0x3f); __p[2] = (y) << 4; }
#define WBUFPOS2(p,pos,x0,y0,x1,y1) { unsigned char *__p = (unsigned char *)(p); __p += (pos); __p[0] = (x0) >> 2; __p[1] = ((x0) << 6) | (((y0) >> 4) & 0x3f); __p[2] = ((y0) << 4) | (((x1) >> 6) & 0x0f); __p[3] = ((x1) << 2) | (((y1) >> 8) & 0x03); __p[4] = (y1); }

#define WPACKETPOS(pos, x, y) { WBUFPOS (WPACKETP(pos), 0, x, y); }
#define WPACKETPOS2(pos, x0, y0, x1, y1) { WBUFPOS2(WPACKETP(pos), 0, x0, y0, x1, y1); }

static char map_ip_str[16];
static in_addr_t map_ip;
static int map_port = 5121;
int map_fd;
char talkie_mes[81]; // 80 + 1 NULL
unsigned char display_unknown_packet = 0;

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
				if (strcmp(w2, npc_language) == 0)
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
	printf("DB '" CL_WHITE "%s" CL_RESET "' readed ('" CL_WHITE "%d" CL_RESET "' entrie%s).\n", MANNER_CONF_NAME, manner_counter, (manner_counter > 1) ? "s" : "");

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
char check_bad_word(const char* sentence, const int sentence_len, const struct map_session_data *sd) {
	char *sentence_to_check, *to_check, *p;
	int i;

	// if no check
	if (battle_config.manner_action == 0)
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
					switch (battle_config.manner_action) {
					case 1: // display a simple message to the player (but doesn't block message)
						clif_displaymessage(sd->fd, msg_txt(651)); // Rudeness is not a good way to have fun in this game.
						return 0; // don't block message
					case 2: // block message and do nothing
						return 1; // block message
					case 3: // block message and send warning message to player
						clif_displaymessage(sd->fd, msg_txt(650)); // Rudeness is not authorized in this game.
						return 1; // block message
					default: // do nothing (invalid option)
						return 0; // don't block message
					}
				}
			}
			to_check++; // to check multiple bad words in single sentence
		}
	}

	FREE(sentence_to_check);

	return 0; // don't block message
}

// manner system - END
//------------------------------------------

/*==========================================
 * map鯖のip設定
 *------------------------------------------
 */
void clif_setip(char *ip) {
	memcpy(map_ip_str, ip, 16);
	map_ip = inet_addr(map_ip_str);
}

/*==========================================
 * map鯖のport設定
 *------------------------------------------
 */
void clif_setport(int port) {
	map_port = port;
}

/*==========================================
 * map鯖のip読み出し
 *------------------------------------------
 */
in_addr_t clif_getip(void) {
	return map_ip;
}

/*==========================================
 * map鯖のport読み出し
 *------------------------------------------
 */
int clif_getport(void) {
	return map_port;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_countusers(void) {
	int users, i;
/*	struct map_session_data *sd;

	users = 0;
	for(i = 0; i < fd_max; i++) {
		if (session[i] && (sd = session[i]->session_data) && sd->state.auth)
			users++;
	}*/
	// speeder method in big servers:
	// method with check of fd_max and session_data need 3 test and 1 affectation
	// method with map counter need just addition on all maps.
	users = 0;
	for(i = 0; i < map_num; i++)
		users = users + map[i].users;
	// fix possible bug
	if (users < 0)
		users = 0;

	return users;
}

/*==========================================
 * 全てのclientに対してfunc()実行
 *------------------------------------------
 */
int clif_foreachclient(int (*func)(struct map_session_data*, va_list),...) {
	int i;
	va_list ap;
	struct map_session_data *sd;

	va_start(ap, func);
	for(i = 0; i < fd_max; i++) {
		if (session[i] && (sd = session[i]->session_data) && sd->state.auth)
			func(sd, ap);
	}
	va_end(ap);

	return 0;
}

/*==========================================
 * clif_sendでAREA*指定時用
 *------------------------------------------
 */
int clif_send_sub(struct block_list *bl, va_list ap) {
	int len, type;
	struct block_list *src_bl;
	struct map_session_data *sd;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);
	nullpo_retr(0, (sd = (struct map_session_data *)bl));

	len = va_arg(ap, int);
	nullpo_retr(0, (src_bl = va_arg(ap, struct block_list*)));
	type = va_arg(ap, int);

	switch(type) {
	case AREA_WOS:
		if (bl == src_bl)
			return 0;
		break;
	case AREA_WOC:
		//if ((sd && sd->chatID) || bl == src_bl)
		if ((sd->chatID) || bl == src_bl)
			return 0;
		break;
	case AREA_WOSC:
		//if (sd && sd->chatID && sd->chatID == ((struct map_session_data*)src_bl)->chatID)
		if (sd->chatID && sd->chatID == ((struct map_session_data*)src_bl)->chatID)
			return 0;
		break;
	case AREA_PET: // to send pet information (hair style) with packet 0x0078 and 0x007b
		if (!(WPACKETW(14) < 24 || WPACKETW(14) > 4000)) {
			if (sd->packet_ver < 12) { // 0: old, 1: 7july04, 2: 13july04, 3: 26july04, 4: 9aug04/16aug04/17aug04, 5: 6sept04, 6: 21sept04, 7: 18oct04, 8: 25oct04/08nov04, 9: 6dec04, 10: 10jan05, 11: 9may05, 12: 28jun05, 13: 4april06 (by [Yor])
				WPACKETW(16) = 0x14; // pet_hair_style
			} else {
				// note: version of 0614 is not used in freya (value = 24)
				WPACKETW(16) = 100; // pet_hair_style
			}
		}
		break;
	}

	if (sd->fd >= 0 && session[sd->fd] != NULL && sd->state.auth) {
		if (packet_size_table[sd->packet_ver][WPACKETW(0)]) { // packet must exist for the client version
			SENDPACKET(sd->fd, len);
		}
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_send(int len, struct block_list *bl, int type) {
	int i;
	struct map_session_data *sd;
	struct map_session_data *sd2;
	struct chat_data *cd;
	struct party *p = NULL;
	struct guild *g = NULL;
	int x0 = 0, x1 = 0, y0 = 0, y1 = 0;

	if (type != ALL_CLIENT) {
		nullpo_retr(0, bl);
	}

	switch(type) {
	case ALL_CLIENT: // 全クライアントに送信
		for(i = 0; i < fd_max; i++) {
			if (session[i] && (sd = session[i]->session_data) != NULL && sd->state.auth) {
				if (packet_size_table[sd->packet_ver][WPACKETW(0)]) { // packet must exist for the client version
					SENDPACKET(i, len);
				}
			}
		}
		break;
	case ALL_SAMEMAP: // 同じマップの全クライアントに送信
	  {
		short map_id;
		map_id = bl->m; // speed up
		for(i = 0; i < fd_max; i++) {
			if (session[i] && (sd = session[i]->session_data) != NULL && sd->state.auth && sd->bl.m == map_id) {
				if (packet_size_table[sd->packet_ver][WPACKETW(0)]) { // packet must exist for the client version
					SENDPACKET(i, len);
				}
			}
		}
	  }
		break;
	case AREA:
	case AREA_WOS:
	case AREA_WOC:
	case AREA_WOSC:
	case AREA_PET: // to send pet information (hair style) with packet 0x0078 and 0x007b
		map_foreachinarea(clif_send_sub, bl->m, bl->x-AREA_SIZE, bl->y-AREA_SIZE, bl->x+AREA_SIZE, bl->y+AREA_SIZE, BL_PC, len, bl, type);
		break;
	case AREA_CHAT_WOC:
		map_foreachinarea(clif_send_sub, bl->m, bl->x - (AREA_SIZE-5), bl->y - (AREA_SIZE-5), bl->x + (AREA_SIZE-5), bl->y + (AREA_SIZE-5), BL_PC, len, bl, AREA_WOC);
		break;
	case CHAT:
	case CHAT_WOS:
		cd = (struct chat_data*)bl;
		if (bl->type == BL_PC) {
			sd = (struct map_session_data*)bl;
			cd = (struct chat_data*)map_id2bl(sd->chatID);
		} else if (bl->type != BL_CHAT)
			break;
		if (cd == NULL)
			break;
		for(i = 0; i < cd->users; i++) {
			sd2 = cd->usersd[i];
			if (type == CHAT_WOS && sd2 == (struct map_session_data*)bl)
				continue;
			if (packet_size_table[sd2->packet_ver][WPACKETW(0)]) { // packet must exist for the client version
				if (sd2->fd >= 0 && session[sd2->fd]) {// Added check to see if session exists
					SENDPACKET(sd2->fd, len);
				}
			}
		}
		break;

	case PARTY_AREA:		// 同じ画面内の全パーティーメンバに送信
	case PARTY_AREA_WOS:	// 自分以外の同じ画面内の全パーティーメンバに送信
		x0 = bl->x - AREA_SIZE;
		y0 = bl->y - AREA_SIZE;
		x1 = bl->x + AREA_SIZE;
		y1 = bl->y + AREA_SIZE;
	case PARTY:				// 全パーティーメンバに送信
	case PARTY_WOS:			// 自分以外の全パーティーメンバに送信
	case PARTY_SAMEMAP:		// 同じマップの全パーティーメンバに送信
	case PARTY_SAMEMAP_WOS:	// 自分以外の同じマップの全パーティーメンバに送信
		if (bl->type == BL_PC) {
			sd = (struct map_session_data *)bl;
			if (sd->partyspy > 0) {
				p = party_search(sd->partyspy);
			} else {
				if (sd->status.party_id > 0)
					p = party_search(sd->status.party_id);
			}
		}
		if (p) {
			for(i = 0; i < MAX_PARTY; i++) {
				if ((sd = p->member[i].sd) != NULL && sd->fd >= 0) {
					if (session[sd->fd] == NULL || session[sd->fd]->session_data == NULL || !sd->state.auth)
						continue;
					if (sd->bl.id == bl->id && (type == PARTY_WOS ||
					    type == PARTY_SAMEMAP_WOS || type == PARTY_AREA_WOS))
						continue;
					if (type != PARTY && type != PARTY_WOS && bl->m != sd->bl.m) // マップチェック
						continue;
					if ((type == PARTY_AREA || type == PARTY_AREA_WOS) &&
					    (sd->bl.x < x0 || sd->bl.y < y0 ||
					     sd->bl.x > x1 || sd->bl.y > y1))
						continue;
					if (packet_size_table[sd->packet_ver][WPACKETW(0)]) { // packet must exist for the client version
						SENDPACKET(sd->fd, len);
					}
//					if (battle_config.etc_log)
//						printf("send party %d %d %d\n",p->party_id,i,flag)
				}
			}
			for (i = 0; i < fd_max; i++) {
				if (session[i] && (sd = session[i]->session_data) != NULL && sd->state.auth) {
					if (sd->partyspy == p->party_id) {
						if (packet_size_table[sd->packet_ver][WPACKETW(0)]) { // packet must exist for the client version
							SENDPACKET(i, len);
						}
					}
				}
			}
		}
		break;
	case SELF:
		sd = (struct map_session_data *)bl;
		if (packet_size_table[sd->packet_ver][WPACKETW(0)]) { // packet must exist for the client version
			SENDPACKET(sd->fd, len);
		}
		break;

	case GUILD_AREA:
	case GUILD_AREA_WOS:
		x0 = bl->x - AREA_SIZE;
		y0 = bl->y - AREA_SIZE;
		x1 = bl->x + AREA_SIZE;
		y1 = bl->y + AREA_SIZE;
	case GUILD:
	case GUILD_WOS:
		if (bl && bl->type == BL_PC) { // guildspy [Syrus22]
			sd = (struct map_session_data *)bl;
			if (sd->guildspy > 0) {
				g = guild_search(sd->guildspy);
			} else {
				if (sd->status.guild_id > 0)
					g = guild_search(sd->status.guild_id);
			}
		}
		if (g) {
			for(i = 0; i < g->max_member; i++) {
				if ((sd = g->member[i].sd) != NULL) {
					if (type == GUILD_WOS && sd->bl.id == bl->id)
						continue;
					if (packet_size_table[sd->packet_ver][WPACKETW(0)]) { // packet must exist for the client version
						SENDPACKET(sd->fd, len);
					}
				}
			}
			for (i = 0; i < fd_max; i++) {
				if (session[i] && (sd = session[i]->session_data) != NULL && sd->state.auth) {
					if (sd->guildspy == g->guild_id) {
						if (packet_size_table[sd->packet_ver][WPACKETW(0)]) { // packet must exist for the client version
							SENDPACKET(i, len);
						}
					}
				}
			}
		}
		break;
	case GUILD_SAMEMAP:
	case GUILD_SAMEMAP_WOS:
		if (bl->type == BL_PC) {
			sd = (struct map_session_data *)bl;
			if (sd->status.guild_id > 0)
				g = guild_search(sd->status.guild_id);
		}
		if (g) {
			for(i = 0; i < g->max_member; i++) {
				if ((sd = g->member[i].sd) != NULL) {
					if (sd->bl.id == bl->id && (type == GUILD_WOS ||
					    type == GUILD_SAMEMAP_WOS || type == GUILD_AREA_WOS))
						continue;
					if (type != GUILD && type != GUILD_WOS && bl->m != sd->bl.m) // マップチェック
						continue;
					if ((type == GUILD_AREA || type == GUILD_AREA_WOS) &&
					    (sd->bl.x < x0 || sd->bl.y < y0 ||
					     sd->bl.x > x1 || sd->bl.y > y1))
						continue;
					if (packet_size_table[sd->packet_ver][WPACKETW(0)]) { // packet must exist for the client version
						SENDPACKET(sd->fd, len);
					}
				}
			}
		}
		break;

	default:
		if (battle_config.error_log)
			printf("clif_send まだ作ってないよー\n");
		return -1;
	}

	return 0;
}

//
// パケット作って送信
//
/*==========================================
 *
 *------------------------------------------
 */
int clif_authok(struct map_session_data *sd) {
	nullpo_retr(0, sd);

	WPACKETW( 0) = 0x73;
	WPACKETL( 2) = gettick();
	WPACKETPOS(6, sd->bl.x, sd->bl.y);
	WPACKETB( 9) = 5;
	WPACKETB(10) = 5;
	SENDPACKET(sd->fd, packet_len_table[0x73]);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_authfail_fd(int fd, int type) {
	WPACKETW(0) = 0x81;
	WPACKETL(2) = type; // 00 = Disconnected from server, 2 = same id
	SENDPACKET(fd, packet_len_table[0x81]);

	clif_setwaitclose(fd);

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_charselectok(int id) {
	struct map_session_data *sd;

	if ((sd = map_id2sd(id)) == NULL)
		return 1;

	WPACKETW(0) = 0xb3;
	WPACKETB(2) = 1;
	SENDPACKET(sd->fd, packet_len_table[0xb3]);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_dropflooritem(struct flooritem_data *fitem) { //009e <ID>.l <name ID>.w <identify flag>.B <X>.w <Y>.w <subX>.B <subY>.B <amount>.w
	int view;

	nullpo_retr(0, fitem);

	if (fitem->item_data.nameid <= 0)
		return 0;

	WPACKETW( 0) = 0x9e;
	WPACKETL( 2) = fitem->bl.id;
	if ((view = itemdb_viewid(fitem->item_data.nameid)) > 0)
		WPACKETW(6) = view;
	else
		WPACKETW(6) = fitem->item_data.nameid;
	WPACKETB( 8) = fitem->item_data.identify;
	WPACKETW( 9) = fitem->bl.x;
	WPACKETW(11) = fitem->bl.y;
	WPACKETB(13) = fitem->subx;
	WPACKETB(14) = fitem->suby;
	WPACKETW(15) = fitem->item_data.amount;
	clif_send(packet_len_table[0x9e], &fitem->bl, AREA);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_clearflooritem(struct flooritem_data *fitem, int fd) {
	nullpo_retr(0, fitem);

	WPACKETW(0) = 0xa1;
	WPACKETL(2) = fitem->bl.id;
	if (fd == 0)
		clif_send(packet_len_table[0xa1], &fitem->bl, AREA);
	else
		SENDPACKET(fd, packet_len_table[0xa1]);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_clearchar(struct block_list *bl, int type) {
// type:
// 00 = Walked out of visible range; dead corpse disappeared, etc. (Fade out)
// 01 = Died
// 02 = Teleported via teleport, fly wing or butterfly wing; logout, etc. (Golden aura)
// 03 = Special? (Blue aura)
	nullpo_retr(0, bl);

	WPACKETW(0) = 0x80;
	WPACKETL(2) = bl->id;
	if (type == 9) {
		WPACKETB(6) = 0;
		clif_send(packet_len_table[0x80], bl, AREA);
	} else {
		WPACKETB(6) = type;
		clif_send(packet_len_table[0x80], bl, type == 1 ? AREA : AREA_WOS);
	}

	return 0;
}

static int clif_clearchar_delay_sub(int tid, unsigned int tick, int id, int data) {
	struct block_list *bl = (struct block_list *)id;

	clif_clearchar(bl,data);
	map_freeblock(bl);

	return 0;
}

int clif_clearchar_delay(unsigned int tick, struct block_list *bl, int type) {
	struct block_list *tmpbl;

	CALLOC(tmpbl, struct block_list, 1);

	memcpy(tmpbl, bl, sizeof(struct block_list));
	add_timer(tick, clif_clearchar_delay_sub, (int)tmpbl, type);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_clearchar_id(int id, unsigned char type, int fd) {
	WPACKETW(0) = 0x80;
	WPACKETL(2) = id;
	WPACKETB(6) = type;
	SENDPACKET(fd, packet_len_table[0x80]);

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
static int clif_set0078(struct map_session_data *sd) {

	nullpo_retr(0, sd);

	if (sd->disguise > 23 && sd->disguise < 4001) { // mob disguises
		WPACKETW( 0) = 0x78;
		WPACKETL( 2) = sd->bl.id;
		WPACKETW( 6) = status_get_speed(&sd->bl);
		WPACKETW( 8) = sd->opt1;
		WPACKETW(10) = sd->opt2;
		WPACKETW(12) = sd->status.option & ~0x0020; // fix peco
		WPACKETW(14) = sd->disguise;
		WPACKETW(42) = 0;
		WPACKETB(44) = 0;
		WBUFPOS(WPACKETP(0), 46, sd->bl.x, sd->bl.y);
		WPACKETB(48) |= sd->dir & 0x0f;
		WPACKETB(49) = 5;
		WPACKETB(50) = 5;
		WPACKETB(51) = 0;
		WPACKETW(52) = (sd->status.base_level > battle_config.max_lv) ? battle_config.max_lv : sd->status.base_level;

		return packet_len_table[0x78];
	}

#if PACKETVER > 6
	WPACKETW( 0) = 0x22a;
	WPACKETL( 2) = sd->bl.id;
	WPACKETW( 6) = sd->speed;
	WPACKETW( 8) = sd->opt1;
	WPACKETW(10) = sd->opt2;
	WPACKETL(12) = sd->status.option;
	WPACKETW(16) = sd->status.class;
	WPACKETW(18) = sd->status.hair;
	if (sd->equip_index[9] >= 0 && sd->inventory_data[sd->equip_index[9]] && sd->view_class != 22) {
		if (sd->inventory_data[sd->equip_index[9]]->view_id > 0)
			WPACKETW(20) = sd->inventory_data[sd->equip_index[9]]->view_id; //weapon
		else
			WPACKETW(20) = sd->status.inventory[sd->equip_index[9]].nameid; //weapon
	} else
		WPACKETW(20) = 0; //weapon
	if (sd->equip_index[8] >= 0 && sd->equip_index[8] != sd->equip_index[9] &&
		 sd->inventory_data[sd->equip_index[8]] && sd->view_class != 22) {
		if (sd->inventory_data[sd->equip_index[8]]->view_id > 0)
			WPACKETW(22) = sd->inventory_data[sd->equip_index[8]]->view_id; //shield
		else
			WPACKETW(22) = sd->status.inventory[sd->equip_index[8]].nameid; //shield
	} else
		WPACKETW(22) = 0; //shield
	WPACKETW(24) = sd->status.head_bottom;
	WPACKETW(26) = sd->status.head_top;
	WPACKETW(28) = sd->status.head_mid;
	WPACKETW(30) = sd->status.hair_color;
	WPACKETW(32) = sd->status.clothes_color;
	WPACKETW(34) = sd->head_dir;
	WPACKETL(36) = sd->status.guild_id;
	WPACKETW(40) = sd->guild_emblem_id;
	WPACKETW(42) = sd->status.manner;
	WPACKETL(44) = sd->opt3;
	WPACKETB(48) = sd->status.karma;
	WPACKETB(49) = sd->sex;
	WBUFPOS(WPACKETP(0), 50, sd->bl.x, sd->bl.y);
	WPACKETB(52)|= sd->dir&0x0f;
	WPACKETB(53) = 5;
	WPACKETB(54) = 5;
	WPACKETB(55) = sd->state.dead_sit;
	WPACKETW(56) = (sd->status.base_level > battle_config.max_lv) ? battle_config.max_lv : sd->status.base_level;
	
#else
	WPACKETW( 0) = 0x1d8;
	WPACKETL( 2) = sd->bl.id;
	WPACKETW( 6) = sd->speed;
	WPACKETW( 8) = sd->opt1;
	WPACKETW(10) = sd->opt2;
	WPACKETW(12) = sd->status.option;
	WPACKETW(14) = sd->view_class;
	WPACKETW(16) = sd->status.hair;
	if (sd->equip_index[9] >= 0 && sd->inventory_data[sd->equip_index[9]] && sd->view_class != 22) {
		if (sd->inventory_data[sd->equip_index[9]]->view_id > 0)
			WPACKETW(18) = sd->inventory_data[sd->equip_index[9]]->view_id;
		else
			WPACKETW(18) = sd->status.inventory[sd->equip_index[9]].nameid;
	} else
		WPACKETW(18) = 0;
	if (sd->equip_index[8] >= 0 && sd->equip_index[8] != sd->equip_index[9] &&
		sd->inventory_data[sd->equip_index[8]] && sd->view_class != 22) {
		if (sd->inventory_data[sd->equip_index[8]]->view_id > 0)
			WPACKETW(20) = sd->inventory_data[sd->equip_index[8]]->view_id;
		else
			WPACKETW(20) = sd->status.inventory[sd->equip_index[8]].nameid;
	} else
		WPACKETW(20) = 0;
	WPACKETW(22) = sd->status.head_bottom;
	WPACKETW(24) = sd->status.head_top;
	WPACKETW(26) = sd->status.head_mid;
	WPACKETW(28) = sd->status.hair_color;
	WPACKETW(30) = sd->status.clothes_color;
	WPACKETW(32) = sd->head_dir;
	WPACKETL(34) = sd->status.guild_id;
	WPACKETW(38) = sd->guild_emblem_id;
	WPACKETW(40) = sd->status.manner;
	WPACKETW(42) = sd->opt3;
	WPACKETB(44) = sd->status.karma;
	WPACKETB(45) = sd->sex;
	WBUFPOS(WPACKETP(0), 46, sd->bl.x, sd->bl.y);
	WPACKETB(48) |= sd->dir & 0x0f;
	WPACKETB(49) = 5;
	WPACKETB(50) = 5;
	WPACKETB(51) = sd->state.dead_sit; // 0: standup, 1: dead, 2: sit
	WPACKETW(52) = (sd->status.base_level > battle_config.max_lv) ? battle_config.max_lv : sd->status.base_level;
#endif

	return packet_len_table[WPACKETW(0)];

}

/*==========================================
 *
 *------------------------------------------
 */
static int clif_set007b(struct map_session_data *sd) {

	nullpo_retr(0, sd);

	if (sd->disguise > 23 && sd->disguise < 4001) { // mob disguises
		WPACKETW( 0) = 0x7b;
		WPACKETL( 2) = sd->bl.id;
		WPACKETW( 6) = status_get_speed(&sd->bl);
		WPACKETW( 8) = sd->opt1;
		WPACKETW(10) = sd->opt2;
		WPACKETW(12) = sd->status.option & ~0x0020; // fix peco;
		WPACKETW(14) = sd->disguise;
		WPACKETL(22) = gettick();
		WPACKETW(46) = 0;
		WPACKETB(48) = 0;
		WBUFPOS2(WPACKETP(0), 50, sd->bl.x, sd->bl.y, sd->to_x, sd->to_y);
		WPACKETB(55) = 0;
		WPACKETB(56) = 5;
		WPACKETB(57) = 5;
		WPACKETW(58) = (sd->status.base_level > battle_config.max_lv) ? battle_config.max_lv : sd->status.base_level;

		return packet_len_table[0x7b];
	}

#if PACKETVER > 6
	WPACKETW( 0) = 0x22c;
	WPACKETL( 2) = sd->bl.id;
	WPACKETW( 6) = sd->speed;
	WPACKETW( 8) = sd->opt1;
	WPACKETW(10) = sd->opt2;
	WPACKETL(12) = sd->status.option;
	WPACKETW(16) = sd->status.class;
	WPACKETW(18) = sd->status.hair;

	if (sd->equip_index[9] >= 0 && sd->inventory_data[sd->equip_index[9]] && sd->view_class != 22) {
		if (sd->inventory_data[sd->equip_index[9]]->view_id > 0)
			WPACKETW(20) = sd->inventory_data[sd->equip_index[9]]->view_id; //weapon
		else
			WPACKETW(20) = sd->status.inventory[sd->equip_index[9]].nameid; //weapon
	}	else
		WPACKETW(20) = 0; //weapon

	if (sd->equip_index[8] >= 0 && sd->equip_index[8] != sd->equip_index[9] && 
		sd->inventory_data[sd->equip_index[8]] && sd->view_class != 22) {
		if (sd->inventory_data[sd->equip_index[8]]->view_id > 0)
			WPACKETW(22) = sd->inventory_data[sd->equip_index[8]]->view_id; //shield
		else
			WPACKETW(22) = sd->status.inventory[sd->equip_index[8]].nameid; //shield
	}	else
		WPACKETW(22) = 0; //shield

	WPACKETW(24) = sd->status.head_bottom;
	WPACKETL(26) = gettick();
	WPACKETW(30) = sd->status.head_top;
	WPACKETW(32) = sd->status.head_mid;
	WPACKETW(34) = sd->status.hair_color;
	WPACKETW(36) = sd->status.clothes_color;
	WPACKETW(38) = sd->head_dir;
	WPACKETL(40) = sd->status.guild_id;
	WPACKETW(44) = sd->guild_emblem_id;
	WPACKETW(46) = sd->status.manner;
	WPACKETL(48) = sd->opt3;
	WPACKETB(52) = sd->status.karma;
	WPACKETB(53) = sd->sex;
	WPACKETPOS2(54, sd->bl.x, sd->bl.y, sd->to_x, sd->to_y);
	WPACKETB(59) = 0x88; // Deals with acceleration in directions. [Valaris]
	WPACKETB(60) = 0;
	WPACKETB(61) = 0;
	WPACKETW(62) = (sd->status.base_level > battle_config.max_lv) ? battle_config.max_lv : sd->status.base_level;

#else
	WPACKETW( 0) = 0x1da;
	WPACKETL( 2) = sd->bl.id;
	WPACKETW( 6) = sd->speed;
	WPACKETW( 8) = sd->opt1;
	WPACKETW(10) = sd->opt2;
	WPACKETW(12) = sd->status.option;
	WPACKETW(14) = sd->view_class;
	WPACKETW(16) = sd->status.hair;

	if (sd->equip_index[9] >= 0 && sd->inventory_data[sd->equip_index[9]] && sd->view_class != 22) {
		if (sd->inventory_data[sd->equip_index[9]]->view_id > 0)
			WPACKETW(18) = sd->inventory_data[sd->equip_index[9]]->view_id; //weapon
		else
			WPACKETW(18) = sd->status.inventory[sd->equip_index[9]].nameid; //weapon
	}	else
		WPACKETW(18) = 0; //weapon

	if (sd->equip_index[8] >= 0 && sd->equip_index[8] != sd->equip_index[9] && 
		sd->inventory_data[sd->equip_index[8]] && sd->view_class != 22) {
		if (sd->inventory_data[sd->equip_index[8]]->view_id > 0)
			WPACKETW(20) = sd->inventory_data[sd->equip_index[8]]->view_id; //shield
		else
			WPACKETW(20) = sd->status.inventory[sd->equip_index[8]].nameid; //shield
	}	else
		WPACKETW(20) = 0; //shield

	WPACKETW(22) = sd->status.head_bottom;
	WPACKETL(24) = gettick();
	WPACKETW(28) = sd->status.head_top;
	WPACKETW(30) = sd->status.head_mid;
	WPACKETW(32) = sd->status.hair_color;
	WPACKETW(34) = sd->status.clothes_color;
	WPACKETW(36) = sd->head_dir;
	WPACKETL(38) = sd->status.guild_id;
	WPACKETW(42) = sd->guild_emblem_id;
	WPACKETW(44) = sd->status.manner;
	WPACKETW(46) = sd->opt3;
	WPACKETB(48) = sd->status.karma;
	WPACKETB(49) = sd->sex;
	WBUFPOS2(WPACKETP(0), 50, sd->bl.x, sd->bl.y, sd->to_x, sd->to_y);
	WPACKETB(55) = 0;
	WPACKETB(56) = 5;
	WPACKETB(57) = 5;
	WPACKETW(58) = (sd->status.base_level > battle_config.max_lv) ? battle_config.max_lv : sd->status.base_level;	
#endif

	return packet_len_table[WPACKETW(0)];

}

/*==========================================
 * クラスチェンジ typeはMobの場合は1で他は0？
 *------------------------------------------
 */
int clif_class_change(struct block_list *bl, int class, int type)
{
	nullpo_retr(0, bl);

	if (class >= MAX_PC_CLASS) {
		WPACKETW(0) = 0x1b0;
		WPACKETL(2) = bl->id;
		WPACKETB(6) = type;
		WPACKETL(7) = class;
		clif_send(packet_len_table[0x1b0], bl, AREA);
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_mob_class_change(struct mob_data *md, int class) {
	int view = mob_get_viewclass(class);

	nullpo_retr(0, md);

	if (view >= MAX_PC_CLASS) {
		WPACKETW(0) = 0x1b0;
		WPACKETL(2) = md->bl.id;
		WPACKETB(6) = 1;
		WPACKETL(7) = view;
		clif_send(packet_len_table[0x1b0], &md->bl, AREA);
	}

	return 0;
}

// mob equipment [Valaris]
int clif_mob_equip(struct mob_data *md, int nameid) {
	nullpo_retr(0, md);

	WPACKETW(0) = 0x1a4;
	WPACKETB(2) = 3;
	WPACKETL(3) = md->bl.id;
	WPACKETL(7) = nameid;
	clif_send(packet_len_table[0x1a4], &md->bl, AREA);

	return 0;
}

/*==========================================
 * MOB表示1
 *------------------------------------------
 */
static int clif_mob0078(struct mob_data *md) {
	int level;

	nullpo_retr(0, md);

	memset(WPACKETP(0), 0, packet_len_table[0x78]);

	WPACKETW( 0) = 0x78;
	WPACKETL( 2) = md->bl.id;
	WPACKETW( 6) = status_get_speed(&md->bl);
	WPACKETW( 8) = md->opt1;
	WPACKETW(10) = md->opt2;
	WPACKETW(12) = md->option;
	WPACKETW(14) = mob_get_viewclass(md->class);
	if ((mob_get_viewclass(md->class) <= 23) || (mob_get_viewclass(md->class) == 812) || (mob_get_viewclass(md->class) >= 4001)) {
		WPACKETW(12) |= mob_db[md->class].option;
		WPACKETW(16) = mob_get_hair(md->class);
		WPACKETW(18) = mob_get_weapon(md->class);
		WPACKETW(20) = mob_get_head_buttom(md->class);
		WPACKETW(22) = mob_get_shield(md->class);
		WPACKETW(24) = mob_get_head_top(md->class);
		WPACKETW(26) = mob_get_head_mid(md->class);
		WPACKETW(28) = mob_get_hair_color(md->class);
		WPACKETW(30) = mob_get_clothes_color(md->class); //Add for player monster dye - Valaris
		if (md->guild_id) { // Guilds' gardians & emperiums, otherwize = 0
			struct guild *g = guild_search(md->guild_id);
			WPACKETL(34) = md->guild_id;
			if (g)
				WPACKETL(38) = g->emblem_id;
		}
		WPACKETB(45) = mob_get_sex(md->class);
	} else if (md->guild_id) { // Guilds' gardians & emperiums, otherwize = 0
		struct guild *g = guild_search(md->guild_id);
		if (g) {
			WPACKETL(22) = g->emblem_id;
			WPACKETL(26) = md->guild_id;
		}
	} // End addition

	WBUFPOS(WPACKETP(0), 46, md->bl.x, md->bl.y);
	WPACKETB(48) |= md->dir & 0x0f;
	WPACKETB(49) = 5;
	WPACKETB(50) = 5;
	WPACKETW(52) = ((level = status_get_lv(&md->bl)) > battle_config.max_lv) ? battle_config.max_lv : level;

	return packet_len_table[0x78];
}

/*==========================================
 * MOB表示2
 *------------------------------------------
 */
static int clif_mob007b(struct mob_data *md) {
	int level;

	nullpo_retr(0, md);

	memset(WPACKETP(0), 0, packet_len_table[0x7b]);

	WPACKETW( 0) = 0x7b;
	WPACKETL( 2) = md->bl.id;
	WPACKETW( 6) = status_get_speed(&md->bl);
	WPACKETW( 8) = md->opt1;
	WPACKETW(10) = md->opt2;
	WPACKETW(12) = md->option;
	WPACKETW(14) = mob_get_viewclass(md->class);
	if ((mob_get_viewclass(md->class) < 24) || (mob_get_viewclass(md->class) > 4000)) {
		WPACKETW(12) |= mob_db[md->class].option;
		WPACKETW(16) = mob_get_hair(md->class);
		WPACKETW(18) = mob_get_weapon(md->class);
		WPACKETW(20) = mob_get_head_buttom(md->class);
		WPACKETL(22) = gettick();
		WPACKETW(26) = mob_get_shield(md->class);
		WPACKETW(28) = mob_get_head_top(md->class);
		WPACKETW(30) = mob_get_head_mid(md->class);
		WPACKETW(32) = mob_get_hair_color(md->class);
		WPACKETW(34) = mob_get_clothes_color(md->class); //Add for player monster dye - Valaris
		if (md->guild_id) { // Guilds' gardians & emperiums, otherwize = 0
			struct guild *g = guild_search(md->guild_id);
			WPACKETL(38) = md->guild_id;
			if (g)
				WPACKETL(42) = g->emblem_id;
		}
		WPACKETB(49) = mob_get_sex(md->class);
	} else {
		WPACKETL(22) = gettick();
		if (md->guild_id) { // Guilds' gardians & emperiums, otherwize = 0
			struct guild *g = guild_search(md->guild_id);
			if (g) {
				WPACKETL(26) = g->emblem_id;
				WPACKETL(30) = md->guild_id;
			}
		}
	}

	WBUFPOS2(WPACKETP(0), 50, md->bl.x, md->bl.y, md->to_x, md->to_y);
	WPACKETB(56) = 5;
	WPACKETB(57) = 5;
	WPACKETW(58) = ((level = status_get_lv(&md->bl)) > battle_config.max_lv) ? battle_config.max_lv : level;

	return packet_len_table[0x7b];
}

/*==========================================
 *
 *------------------------------------------
 */
static int clif_npc0078(struct npc_data *nd) {
	struct guild *g;

	nullpo_retr(0, nd);

	memset(WPACKETP(0), 0, packet_len_table[0x78]);

	WPACKETW( 0) = 0x78;
	WPACKETL( 2) = nd->bl.id;
	WPACKETW( 6) = nd->speed;
//	WPACKETW(12) = nd->option;
	WPACKETW(14) = nd->class;
	if ((nd->class == 722) && (nd->u.scr.guild_id > 0) && ((g=guild_search(nd->u.scr.guild_id)) != NULL)) {
//	if ((nd->bl.subtype != WARP) && (nd->class == 722) && (nd->u.scr.guild_id > 0) && ((g = guild_search(nd->u.scr.guild_id)))) {
		WPACKETL(22) = g->emblem_id;
		WPACKETL(26) = g->guild_id;
	}
	WBUFPOS(WPACKETP(0), 46, nd->bl.x, nd->bl.y);
	WPACKETB(48) |= nd->dir & 0x0f;
	WPACKETB(49) = 5;
	WPACKETB(50) = 5;

	return packet_len_table[0x78];
}

// NPC Walking [Valaris]
static int clif_npc007b(struct npc_data *nd) {
	struct guild *g;

	nullpo_retr(0, nd);

	memset(WPACKETP(0), 0, packet_len_table[0x7b]);

	WPACKETW( 0) = 0x7b;
	WPACKETL( 2) = nd->bl.id;
	WPACKETW( 6) = nd->speed;
//	WPACKETW(12) = nd->option;
	WPACKETW(14) = nd->class;
	if ((nd->class == 722) && (nd->u.scr.guild_id > 0) && ((g=guild_search(nd->u.scr.guild_id)) != NULL)) {
//	if ((nd->bl.subtype!=WARP) && (nd->class == 722) && (nd->u.scr.guild_id > 0) && ((g=guild_search(nd->u.scr.guild_id)))) {
		WPACKETL(22) = g->emblem_id;
		WPACKETL(26) = g->guild_id;
	}

	WPACKETL(22) = gettick();
	WBUFPOS2(WPACKETP(0), 50, nd->bl.x, nd->bl.y, nd->to_x, nd->to_y);
	WPACKETB(56) = 5;
	WPACKETB(57) = 5;

	return packet_len_table[0x7b];
}

/*==========================================
 *
 *------------------------------------------
 */
static int clif_pet0078(struct pet_data *pd) {
	int view, level;

	nullpo_retr(0, pd);

	memset(WPACKETP(0), 0, packet_len_table[0x78]);

	WPACKETW( 0) = 0x78;
	WPACKETL( 2) = pd->bl.id;
	WPACKETW( 6) = pd->speed;
	WPACKETW(14) = mob_get_viewclass(pd->class);
	if (WPACKETW(14) < 24 || WPACKETW(14) > 4000) {
		WPACKETW(12) = mob_db[pd->class].option;
		WPACKETW(16) = mob_get_hair(pd->class);
		WPACKETW(18) = mob_get_weapon(pd->class);
		WPACKETW(20) = mob_get_head_buttom(pd->class);
		WPACKETW(22) = mob_get_shield(pd->class);
		WPACKETW(24) = mob_get_head_top(pd->class);
		WPACKETW(26) = mob_get_head_mid(pd->class);
		WPACKETW(28) = mob_get_hair_color(pd->class);
		WPACKETW(30) = mob_get_clothes_color(pd->class); //Add for player pet dye - Valaris
		WPACKETB(45) = mob_get_sex(pd->class);
	} else {
		//WPACKETW(16) = 0x14; // old versions
		WPACKETW(16) = 100; // pet_hair_style
		if ((view = itemdb_viewid(pd->equip)) > 0)
			WPACKETW(20) = view;
		else
			WPACKETW(20) = pd->equip;
	}
	WBUFPOS(WPACKETP(0), 46, pd->bl.x, pd->bl.y);
	WPACKETB(48) |= pd->dir & 0x0f;
	WPACKETB(49) = 0;
	WPACKETB(50) = 0;
	WPACKETW(52) = ((level = status_get_lv(&pd->bl)) > battle_config.max_lv) ? battle_config.max_lv : level;

	return packet_len_table[0x78];
}

/*==========================================
 *
 *------------------------------------------
 */
static int clif_pet007b(struct pet_data *pd) {
	int view, level;

	nullpo_retr(0, pd);

	memset(WPACKETP(0), 0, packet_len_table[0x7b]);

	WPACKETW( 0) = 0x7b;
	WPACKETL( 2) = pd->bl.id;
	WPACKETW( 6) = pd->speed;
	WPACKETW(14) = mob_get_viewclass(pd->class);
	if (WPACKETW(14) < 24 || WPACKETW(14) > 4000) {
		WPACKETW(12) = mob_db[pd->class].option;
		WPACKETW(16) = mob_get_hair(pd->class);
		WPACKETW(18) = mob_get_weapon(pd->class);
		WPACKETW(20) = mob_get_head_buttom(pd->class);
		WPACKETL(22) = gettick();
		WPACKETW(26) = mob_get_shield(pd->class);
		WPACKETW(28) = mob_get_head_top(pd->class);
		WPACKETW(30) = mob_get_head_mid(pd->class);
		WPACKETW(32) = mob_get_hair_color(pd->class);
		WPACKETW(34) = mob_get_clothes_color(pd->class); //Add for player pet dye - Valaris
		WPACKETB(49) = mob_get_sex(pd->class);
	} else {
		//WPACKETW(16) = 0x14; // old versions
		WPACKETW(16) = 100; // pet_hair_style
		if ((view = itemdb_viewid(pd->equip)) > 0)
			WPACKETW(20) = view;
		else
			WPACKETW(20) = pd->equip;
		WPACKETL(22) = gettick();
	}
	WBUFPOS2(WPACKETP(0), 50, pd->bl.x, pd->bl.y, pd->to_x, pd->to_y);
//	WPACKETB(56) = 0;
//	WPACKETB(57) = 0;
	WPACKETW(58) = ((level = status_get_lv(&pd->bl)) > battle_config.max_lv) ? battle_config.max_lv : level;

	return packet_len_table[0x7b];
}

/*==========================================
 *
 *------------------------------------------
 */
static int clif_set0192(int fd, int m, int x, int y, int type) {
	WPACKETW(0) = 0x192;
	WPACKETW(2) = x;
	WPACKETW(4) = y;
	WPACKETW(6) = type;
	strncpy(WPACKETP(8), map[m].name, 16);
	SENDPACKET(fd, packet_len_table[0x192]);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_spawnpc(struct map_session_data *sd) {
	nullpo_retr(0, sd);

	clif_set0078(sd);

#if PACKETVER > 6
	WPACKETW( 0) = 0x22b;
	WPACKETW(55) = (sd->status.base_level > battle_config.max_lv) ? battle_config.max_lv : sd->status.base_level;
#else
	WPACKETW( 0) = 0x1d9;
	WPACKETW(51) = (sd->status.base_level > battle_config.max_lv) ? battle_config.max_lv : sd->status.base_level;
#endif

	clif_send(packet_len_table[WPACKETW(0)], &sd->bl, AREA_WOS);

	if (sd->disguise > 23 && sd->disguise < 4001) { // mob disguises
		clif_clearchar(&sd->bl, 9);

		memset(WPACKETP(0), 0, packet_len_table[0x119]);
		WPACKETW( 0) = 0x119;
		WPACKETL( 2) = sd->bl.id;
		WPACKETW(10) = 0x40;
		clif_send(packet_len_table[0x119], &sd->bl, SELF);

		memset(WPACKETP(0), 0, packet_len_table[0x7c]);
		WPACKETW( 0) = 0x7c;
		WPACKETL( 2) = sd->bl.id;
		WPACKETW( 6) = sd->speed;
		WPACKETW( 8) = sd->opt1;
		WPACKETW(10) = sd->opt2;
		WPACKETW(12) = sd->status.option & ~0x0020; // fix peco;
		WPACKETW(20) = sd->disguise;
		WBUFPOS(WPACKETP(0), 36, sd->bl.x, sd->bl.y);
		clif_send(packet_len_table[0x7c], &sd->bl, AREA);
	}

	if (sd->spiritball > 0)
		clif_spiritball(sd);

	if (sd->status.guild_id > 0) { // force display of guild emblem [Valaris]
		struct guild *g = guild_search(sd->status.guild_id);
		if (g)
			clif_guild_emblem(sd, g);
	}	// end addition [Valaris]

	if (sd->status.class ==   13 || sd->status.class ==   21 ||
	    sd->status.class == 4014 || sd->status.class == 4022 ||
	    sd->status.class == 4036 || sd->status.class == 4044)
		pc_setoption(sd, sd->status.option | 0x0020);

	if ((pc_isriding(sd) && pc_checkskill(sd, KN_RIDING) > 0) &&
	    (sd->status.class ==    7 || sd->status.class ==   14 ||
	     sd->status.class == 4008 || sd->status.class == 4015 ||
	     sd->status.class == 4030 || sd->status.class == 4037))
		pc_setriding(sd); // update peco riders for people upgrading athena

	if (map[sd->bl.m].flag.snow)
		clif_specialeffect(&sd->bl, 162, 1); // flag: 0: player see in the area (normal), 1: only player see only by player, 2: all players in a map that see only their (not see others), 3: all players that see only their (not see others)
	if (map[sd->bl.m].flag.fog)
		clif_specialeffect(&sd->bl, 233, 1); // flag: 0: player see in the area (normal), 1: only player see only by player, 2: all players in a map that see only their (not see others), 3: all players that see only their (not see others)
	if (map[sd->bl.m].flag.sakura)
		clif_specialeffect(&sd->bl, 163, 1); // flag: 0: player see in the area (normal), 1: only player see only by player, 2: all players in a map that see only their (not see others), 3: all players that see only their (not see others)
	if (map[sd->bl.m].flag.leaves)
		clif_specialeffect(&sd->bl, 333, 1); // flag: 0: player see in the area (normal), 1: only player see only by player, 2: all players in a map that see only their (not see others), 3: all players that see only their (not see others)
	if (map[sd->bl.m].flag.rain)
		clif_specialeffect(&sd->bl, 161, 1); // flag: 0: player see in the area (normal), 1: only player see only by player, 2: all players in a map that see only their (not see others), 3: all players that see only their (not see others)
	if (night_flag && !map[sd->bl.m].flag.indoors) {
		if (!sd->state.night) {
			sd->state.night = 1;
		}
		clif_status_change(&sd->bl, ICO_NIGHT, 0);
		clif_status_change(&sd->bl, ICO_NIGHT, 1);
	} else if (sd->state.night) {
			clif_status_change(&sd->bl, ICO_NIGHT, 0);
			sd->state.night = 0;
	}
	
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_spawnnpc(struct npc_data *nd)
{
	int len;

	nullpo_retr(0, nd);

//	if (nd->class < 0 || (nd->flag & 1 && nd->option != 0x0002) || nd->class == INVISIBLE_CLASS)
	if (nd->class < 0 || nd->flag & 1 || nd->class == INVISIBLE_CLASS)
		return 0;

	memset(WPACKETP(0), 0, packet_len_table[0x7c]);

	WPACKETW( 0) = 0x7c;
	WPACKETL( 2) = nd->bl.id;
	WPACKETW( 6) = nd->speed;
//	WPACKETW(12) = nd->option;
	WPACKETW(20) = nd->class;
	WBUFPOS(WPACKETP(0), 36, nd->bl.x, nd->bl.y);

	clif_send(packet_len_table[0x7c], &nd->bl, AREA);

	len = clif_npc0078(nd);
	clif_send(len, &nd->bl, AREA);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_spawnmob(struct mob_data *md)
{
	int len;

	nullpo_retr(0, md);

	if (mob_get_viewclass(md->class) > 23 ) {
		memset(WPACKETP(0), 0, packet_len_table[0x7c]);

		WPACKETW( 0) = 0x7c;
		WPACKETL( 2) = md->bl.id;
		WPACKETW( 6) = md->speed;
		WPACKETW( 8) = md->opt1;
		WPACKETW(10) = md->opt2;
		WPACKETW(12) = md->option;
		WPACKETW(20) = mob_get_viewclass(md->class);
		WBUFPOS(WPACKETP(0), 36, md->bl.x, md->bl.y);
		clif_send(packet_len_table[0x7c], &md->bl, AREA);
	}

	len = clif_mob0078(md);
	clif_send(len, &md->bl, AREA);

	if (mob_get_equip(md->class) > 0) // mob equipment [Valaris]
		clif_mob_equip(md, mob_get_equip(md->class));

	if (md->size == 2) // tiny/big mobs [Valaris]
		clif_specialeffect(&md->bl, 423, 0); // flag: 0: player see in the area (normal), 1: only player see only by player, 2: all players in a map that see only their (not see others), 3: all players that see only their (not see others)
	else if (md->size == 1)
		clif_specialeffect(&md->bl, 421, 0); // flag: 0: player see in the area (normal), 1: only player see only by player, 2: all players in a map that see only their (not see others), 3: all players that see only their (not see others)

	return 0;
}

// pet

/*==========================================
 *
 *------------------------------------------
 */
int clif_spawnpet(struct pet_data *pd)
{
	int len;

	nullpo_retr(0, pd);

	if (mob_get_viewclass(pd->class) >= MAX_PC_CLASS) {
		memset(WPACKETP(0), 0, packet_len_table[0x7c]);

		WPACKETW( 0) = 0x7c;
		WPACKETL( 2) = pd->bl.id;
		WPACKETW( 6) = pd->speed;
		WPACKETW(20) = mob_get_viewclass(pd->class);
		WBUFPOS(WPACKETP(0), 36, pd->bl.x, pd->bl.y);
		clif_send(packet_len_table[0x7c], &pd->bl, AREA);
	}

	len = clif_pet0078(pd);
	clif_send(len, &pd->bl, AREA_PET);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_movepet(struct pet_data *pd) {
	int len;

	nullpo_retr(0, pd);

	len = clif_pet007b(pd);
	clif_send(len, &pd->bl, AREA_PET);

	return 0;
}

/*==========================================
 * npc walking [Valaris]
 *------------------------------------------
 */
int clif_movenpc(struct npc_data *nd) {
	int len;

	nullpo_retr(0, nd);

	len = clif_npc007b(nd);
	clif_send(len, &nd->bl, AREA);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
/*int clif_servertick(struct map_session_data *sd)
{
	nullpo_retr(0, sd);

	WPACKETW(0) = 0x7f;
	WPACKETL(2) = gettick();
	SENDPACKET(sd->fd, packet_len_table[0x7f]);

	return 0;
}*/

/*==========================================
 *
 *------------------------------------------
 */
int clif_walkok(struct map_session_data *sd)
{
	nullpo_retr(0, sd);

	WPACKETW( 0) = 0x87;
	WPACKETL( 2) = gettick();
	WPACKETPOS2(6, sd->bl.x, sd->bl.y, sd->to_x, sd->to_y);
	WPACKETB(11) = 0;
	SENDPACKET(sd->fd, packet_len_table[0x87]);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_movechar(struct map_session_data *sd) {
	int len;

	nullpo_retr(0, sd);

	len = clif_set007b(sd);

	if (sd->disguise > 23 && sd->disguise < 4001) {
		clif_send(len, &sd->bl, AREA);
		return 0;
	} else
		clif_send(len, &sd->bl, AREA_WOS);

	if (battle_config.save_clothcolor && sd->status.clothes_color > 0)
		clif_changelook(&sd->bl, LOOK_CLOTHES_COLOR, sd->status.clothes_color);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
static int clif_waitclose(int tid, unsigned int tick, int id, int data) {
	struct map_session_data *sd;

	if (session[id]) {
		// check if the session is always the same [Yor]
		if (((sd = session[id]->session_data) == NULL && data == 0) ||
		    (sd != NULL && sd->bl.id == data))
			session[id]->eof = 1;
		// else session was closed and an other player have connected [Yor]
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_setwaitclose(int fd) {
	struct map_session_data *sd;

	// if player is not already in the game (double connection probably) [yor]
	if ((sd = session[fd]->session_data) == NULL) {
		// limited timer, just to send information.
		add_timer(gettick() + 1000, clif_waitclose, fd, 0);
	} else
		add_timer(gettick() + 5000, clif_waitclose, fd, sd->bl.id);
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_changemap(struct map_session_data *sd, char *mapname, int x, int y) {
	nullpo_retr(0, sd);

	WPACKETW( 0) = 0x91;
	strncpy(WPACKETP(2), mapname, 16);
	WPACKETW(18) = x;
	WPACKETW(20) = y;
	SENDPACKET(sd->fd, packet_len_table[0x91]);

	if (sd->disguise > 23 && sd->disguise < 4001) // mob disguises [Valaris]
		clif_spawnpc(sd);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_changemapserver(struct map_session_data *sd, char *mapname, int x, int y, int ip, int port) {
	nullpo_retr(0, sd);

	WPACKETW( 0) = 0x92;
	strncpy(WPACKETP(2), mapname, 16);
	WPACKETB(17) = 0; //Null term.
	WPACKETW(18) = x;
	WPACKETW(20) = y;
	WPACKETL(22) = ip;
	WPACKETW(26) = port;
	SENDPACKET(sd->fd, packet_len_table[0x92]);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_fixpos(struct block_list *bl) {
	nullpo_retr(0, bl);

	WPACKETW(0) = 0x88;
	WPACKETL(2) = bl->id;
	WPACKETW(6) = bl->x;
	WPACKETW(8) = bl->y;

	clif_send(packet_len_table[0x88], bl, AREA);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_npcbuysell(struct map_session_data* sd, int id) {
	nullpo_retr(0, sd);

	WPACKETW(0) = 0xc4;
	WPACKETL(2) = id;
	SENDPACKET(sd->fd, packet_len_table[0xc4]);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_buylist(struct map_session_data *sd, struct npc_data *nd) {
	struct item_data *id;
	int i, val;

//	nullpo_retv(sd); // checked before to call function
//	nullpo_retv(nd); // checked before to call function

	WPACKETW(0) = 0xc6;
	for(i = 0; nd->u.shop_item[i].nameid > 0; i++) {
		id = itemdb_search(nd->u.shop_item[i].nameid);
		val = nd->u.shop_item[i].value;
		WPACKETL( 4 + i * 11) = val;
		if (!id->flag.value_notdc)
			WPACKETL( 8 + i * 11) = pc_modifybuyvalue(sd, val);
		else
			WPACKETL( 8 + i * 11) = val;
		WPACKETB(12 + i * 11) = id->type;
		if (id->view_id > 0)
			WPACKETW(13 + i * 11) = id->view_id;
		else
			WPACKETW(13 + i * 11) = nd->u.shop_item[i].nameid;
	}
	WPACKETW(2) = i * 11 + 4;
	SENDPACKET(sd->fd, WPACKETW(2));

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_selllist(struct map_session_data *sd) {
	int i, c, val;

//	nullpo_retr(0, sd); // checked before to call function

	WPACKETW(0) = 0xc7;
	c = 0;
	for(i = 0; i < MAX_INVENTORY; i++) {
		if (sd->status.inventory[i].nameid > 0 && sd->inventory_data[i]) {
			if (!itemdb_cansell(sd->status.inventory[i].nameid, sd->GM_level))
				continue;

			val = sd->inventory_data[i]->value_sell;
			if (val < 0)
				continue;
			WPACKETW( 4 + c * 10) = i + 2;
			WPACKETL( 6 + c * 10) = val;
			if (!sd->inventory_data[i]->flag.value_notoc)
				WPACKETL(10 + c * 10) = pc_modifysellvalue(sd, val);
			else
				WPACKETL(10 + c * 10) = val;
			c++;
		}
	}
	WPACKETW(2) = c * 10 + 4;
	SENDPACKET(sd->fd, WPACKETW(2));

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_scriptmes(struct map_session_data *sd, int npcid, char *mes) {
	int len;

	nullpo_retr(0, sd);

	len = strlen(mes);

	WPACKETW(0) = 0xb4;
	WPACKETW(2) = len + 9; // len + 8 + NULL
	WPACKETL(4) = npcid;
	strncpy(WPACKETP(8), mes, len);
	WPACKETB(len + 9 - 1) = 0; // NULL
	SENDPACKET(sd->fd, WPACKETW(2));

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_scriptnext(struct map_session_data *sd, int npcid) {
	nullpo_retr(0, sd);

	WPACKETW(0) = 0xb5;
	WPACKETL(2) = npcid;
	SENDPACKET(sd->fd, packet_len_table[0xb5]);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_scriptclose(struct map_session_data *sd, int npcid) {
	nullpo_retr(0, sd);

	WPACKETW(0) = 0xb6;
	WPACKETL(2) = npcid;
	SENDPACKET(sd->fd, packet_len_table[0xb6]);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_scriptmenu(struct map_session_data *sd, int npcid, char *mes) {
	int len;

	nullpo_retr(0, sd);

	len = strlen(mes);

	WPACKETW(0) = 0xb7;
	WPACKETW(2) = len + 9;
	WPACKETL(4) = npcid;
	strncpy(WPACKETP(8), mes, len);
	WPACKETB(len + 9 - 1) = 0; // NULL
	SENDPACKET(sd->fd, WPACKETW(2));

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_scriptinput(struct map_session_data *sd, int npcid) {
	nullpo_retr(0, sd);

	WPACKETW(0) = 0x142;
	WPACKETL(2) = npcid;
	SENDPACKET(sd->fd, packet_len_table[0x142]);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_scriptinputstr(struct map_session_data *sd, int npcid) {
	nullpo_retr(0, sd);

	WPACKETW(0) = 0x1d4;
	WPACKETL(2) = npcid;
	SENDPACKET(sd->fd, packet_len_table[0x1d4]);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_viewpoint(struct map_session_data *sd, int npc_id, int type, int x, int y, int id, int color) {
	nullpo_retr(0, sd);

	WPACKETW( 0) = 0x144;
	WPACKETL( 2) = npc_id;
	WPACKETL( 6) = type;
	WPACKETL(10) = x;
	WPACKETL(14) = y;
	WPACKETB(18) = id;
	WPACKETL(19) = color;
	SENDPACKET(sd->fd, packet_len_table[0x144]);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_cutin(struct map_session_data *sd, char *image, int type) {
	nullpo_retr(0, sd);

	WPACKETW( 0) = 0x1b3;
	strncpy(WPACKETP(2), image, 64);
	WPACKETB(66) = type;
	SENDPACKET(sd->fd, packet_len_table[0x1b3]);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_additem(struct map_session_data *sd, int n, int amount, int fail) {
// fail:
//    0: you got...
//    1: you cannot get the item.
//    2: you can not carry more items because because you are overweight.
//    3: you got...
//    4: you can not have this item because you are over the weight limit.
//    5: you can not carry more than 30,000 of one king of item.
//    6: you cannot get the item.
	int j;

	nullpo_retr(0, sd);

	if (fail) {
		WPACKETW( 0) = 0xa0;
		WPACKETW( 2) = n + 2;
		WPACKETW( 4) = amount;
		WPACKETW( 6) = 0;
		WPACKETB( 8) = 0;
		WPACKETB( 9) = 0;
		WPACKETB(10) = 0;
		WPACKETW(11) = 0;
		WPACKETW(13) = 0;
		WPACKETW(15) = 0;
		WPACKETW(17) = 0;
		WPACKETW(19) = 0;
		WPACKETB(21) = 0;
		WPACKETB(22) = fail;
	} else {
		if (n < 0 || n >= MAX_INVENTORY || sd->status.inventory[n].nameid <= 0 || sd->inventory_data[n] == NULL)
			return 1;

		WPACKETW( 0) = 0xa0;
		WPACKETW( 2) = n + 2;
		WPACKETW( 4) = amount;
		if (sd->inventory_data[n]->view_id > 0)
			WPACKETW(6) = sd->inventory_data[n]->view_id;
		else
			WPACKETW(6) = sd->status.inventory[n].nameid;
		WPACKETB( 8) = sd->status.inventory[n].identify;
		WPACKETB( 9) = sd->status.inventory[n].attribute;
		WPACKETB(10) = sd->status.inventory[n].refine;
		if (sd->status.inventory[n].card[0] == 0x00ff || sd->status.inventory[n].card[0] == 0x00fe || sd->status.inventory[n].card[0] == (short)0xff00) {
			WPACKETW(11) = sd->status.inventory[n].card[0];
			WPACKETW(13) = sd->status.inventory[n].card[1];
			WPACKETW(15) = sd->status.inventory[n].card[2];
			WPACKETW(17) = sd->status.inventory[n].card[3];
		} else {
			if (sd->status.inventory[n].card[0] > 0 && (j = itemdb_viewid(sd->status.inventory[n].card[0])) > 0)
				WPACKETW(11) = j;
			else
				WPACKETW(11) = sd->status.inventory[n].card[0];
			if (sd->status.inventory[n].card[1] > 0 && (j = itemdb_viewid(sd->status.inventory[n].card[1])) > 0)
				WPACKETW(13) = j;
			else
				WPACKETW(13) = sd->status.inventory[n].card[1];
			if (sd->status.inventory[n].card[2] > 0 && (j = itemdb_viewid(sd->status.inventory[n].card[2])) > 0)
				WPACKETW(15) = j;
			else
				WPACKETW(15) = sd->status.inventory[n].card[2];
			if (sd->status.inventory[n].card[3] > 0 && (j = itemdb_viewid(sd->status.inventory[n].card[3])) > 0)
				WPACKETW(17) = j;
			else
				WPACKETW(17) = sd->status.inventory[n].card[3];
		}
		WPACKETW(19) = pc_equippoint(sd,n);
		WPACKETB(21) = (sd->inventory_data[n]->type == 7)? 4 : sd->inventory_data[n]->type;
		WPACKETB(22) = fail;
	}
	SENDPACKET(sd->fd, packet_len_table[0xa0]);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_delitem(struct map_session_data *sd, int n, int amount) {
	nullpo_retr(0, sd);

	WPACKETW(0) = 0xaf;
	WPACKETW(2) = n + 2;
	WPACKETW(4) = amount;
	SENDPACKET(sd->fd, packet_len_table[0xaf]);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_itemlist(struct map_session_data *sd)
{
	int i, n, arrow = -1;

	nullpo_retr(0, sd);

	n = 0;
	for(i = 0; i < MAX_INVENTORY; i++) {
		if (sd->status.inventory[i].nameid <= 0 || sd->inventory_data[i] == NULL || itemdb_isequip2(sd->inventory_data[i]))
			continue;
		WPACKETW(n*18+ 4) = i + 2;
		if (sd->inventory_data[i]->view_id > 0)
			WPACKETW(n*18+6) = sd->inventory_data[i]->view_id;
		else
			WPACKETW(n*18+6) = sd->status.inventory[i].nameid;
		WPACKETB(n*18+ 8) = sd->inventory_data[i]->type;
		WPACKETB(n*18+ 9) = sd->status.inventory[i].identify;
		WPACKETW(n*18+10) = sd->status.inventory[i].amount;
		if (sd->inventory_data[i]->equip == 0x8000) {
			WPACKETW(n*18+12) = 0x8000;
			if (sd->status.inventory[i].equip)
				arrow = i; // ついでに矢装備チェック
		} else
			WPACKETW(n*18+12) = 0;
		WPACKETW(n*18+14) = sd->status.inventory[i].card[0];
		WPACKETW(n*18+16) = sd->status.inventory[i].card[1];
		WPACKETW(n*18+18) = sd->status.inventory[i].card[2];
		WPACKETW(n*18+20) = sd->status.inventory[i].card[3];
		n++;
	}
	if (n) {
		WPACKETW(0) = 0x1ee;
		WPACKETW(2) = 4 + n * 18;
		SENDPACKET(sd->fd, WPACKETW(2));
	}

	if (arrow >= 0)
		clif_arrowequip(sd, arrow);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_equiplist(struct map_session_data *sd)
{
	int i, j, n;

	nullpo_retr(0, sd);

	for(i = 0, n = 0; i < MAX_INVENTORY; i++) {
		if (sd->status.inventory[i].nameid <= 0 || sd->inventory_data[i] == NULL || !itemdb_isequip2(sd->inventory_data[i]))
			continue;
		WPACKETW(n*20+ 4) = i + 2;
		if (sd->inventory_data[i]->view_id > 0)
			WPACKETW(n*20+6) = sd->inventory_data[i]->view_id;
		else
			WPACKETW(n*20+6) = sd->status.inventory[i].nameid;
		WPACKETB(n*20+ 8) = (sd->inventory_data[i]->type == 7)? 4:sd->inventory_data[i]->type;
		WPACKETB(n*20+ 9) = sd->status.inventory[i].identify;
		WPACKETW(n*20+10) = pc_equippoint(sd,i);
		WPACKETW(n*20+12) = sd->status.inventory[i].equip;
		WPACKETB(n*20+14) = sd->status.inventory[i].attribute;
		WPACKETB(n*20+15) = sd->status.inventory[i].refine;
		if (sd->status.inventory[i].card[0] == 0x00ff || sd->status.inventory[i].card[0] == 0x00fe || sd->status.inventory[i].card[0] == (short)0xff00) {
			WPACKETW(n*20+16) = sd->status.inventory[i].card[0];
			WPACKETW(n*20+18) = sd->status.inventory[i].card[1];
			WPACKETW(n*20+20) = sd->status.inventory[i].card[2];
			WPACKETW(n*20+22) = sd->status.inventory[i].card[3];
		} else {
			if (sd->status.inventory[i].card[0] > 0 && (j=itemdb_viewid(sd->status.inventory[i].card[0])) > 0)
				WPACKETW(n*20+16) = j;
			else
				WPACKETW(n*20+16) = sd->status.inventory[i].card[0];
			if (sd->status.inventory[i].card[1] > 0 && (j = itemdb_viewid(sd->status.inventory[i].card[1])) > 0)
				WPACKETW(n*20+18) = j;
			else
				WPACKETW(n*20+18) = sd->status.inventory[i].card[1];
			if (sd->status.inventory[i].card[2] > 0 && (j = itemdb_viewid(sd->status.inventory[i].card[2])) > 0)
				WPACKETW(n*20+20) = j;
			else
				WPACKETW(n*20+20) = sd->status.inventory[i].card[2];
			if (sd->status.inventory[i].card[3] > 0 && (j = itemdb_viewid(sd->status.inventory[i].card[3])) > 0)
				WPACKETW(n*20+22) = j;
			else
				WPACKETW(n*20+22) = sd->status.inventory[i].card[3];
		}
		n++;
	}
	if (n) {
		WPACKETW(0) = 0xa4;
		WPACKETW(2) = 4 + n * 20;
		SENDPACKET(sd->fd, WPACKETW(2));
	}

	return 0;
}

/*==========================================
 * カプラさんに預けてある消耗品&収集品リスト
 *------------------------------------------
 */
int clif_storageitemlist(struct map_session_data *sd, struct storage *stor)
{
	struct item_data *id;
	int i, n;

	nullpo_retr(0, sd);
	nullpo_retr(0, stor);

	n = 0;
	for(i = 0; i < MAX_STORAGE; i++) {
		if (stor->storage[i].nameid <= 0)
			continue;
		nullpo_retr(0, id = itemdb_search(stor->storage[i].nameid));
		if (itemdb_isequip2(id))
			continue;

		WPACKETW(n*18+ 4) = i+1;
		if (id->view_id > 0)
			WPACKETW(n*18+ 6) = id->view_id;
		else
			WPACKETW(n*18+ 6) = stor->storage[i].nameid;
		WPACKETB(n*18+ 8) = id->type;
		WPACKETB(n*18+ 9) = stor->storage[i].identify;
		WPACKETW(n*18+10) = stor->storage[i].amount;
		WPACKETW(n*18+12) = 0;
		WPACKETW(n*18+14) = stor->storage[i].card[0];
		WPACKETW(n*18+16) = stor->storage[i].card[1];
		WPACKETW(n*18+18) = stor->storage[i].card[2];
		WPACKETW(n*18+20) = stor->storage[i].card[3];
		n++;
	}
	if (n) {
		WPACKETW(0) = 0x1f0;
		WPACKETW(2) = 4 + n * 18;
		SENDPACKET(sd->fd, WPACKETW(2));
	}

	return 0;
}

/*==========================================
 * カプラさんに預けてある装備リスト
 *------------------------------------------
 */
int clif_storageequiplist(struct map_session_data *sd, struct storage *stor)
{
	struct item_data *id;
	int i, j, n;

	nullpo_retr(0, sd);
	nullpo_retr(0, stor);

	n = 0;
	for(i = 0; i < MAX_STORAGE; i++) {
		if (stor->storage[i].nameid <= 0)
			continue;
		nullpo_retr(0, id = itemdb_search(stor->storage[i].nameid));
		if (!itemdb_isequip2(id))
			continue;
		WPACKETW(n*20+ 4) = i + 1;
		if (id->view_id > 0)
			WPACKETW(n*20+ 6) = id->view_id;
		else
			WPACKETW(n*20+ 6) = stor->storage[i].nameid;
		WPACKETB(n*20+ 8) = id->type;
		WPACKETB(n*20+ 9) = stor->storage[i].identify;
		WPACKETW(n*20+10) = id->equip;
		WPACKETW(n*20+12) = stor->storage[i].equip;
		WPACKETB(n*20+14) = stor->storage[i].attribute;
		WPACKETB(n*20+15) = stor->storage[i].refine;
		if (stor->storage[i].card[0] == 0x00ff || stor->storage[i].card[0] == 0x00fe || stor->storage[i].card[0] == (short)0xff00) {
			WPACKETW(n*20+16) = stor->storage[i].card[0];
			WPACKETW(n*20+18) = stor->storage[i].card[1];
			WPACKETW(n*20+20) = stor->storage[i].card[2];
			WPACKETW(n*20+22) = stor->storage[i].card[3];
		} else {
			if (stor->storage[i].card[0] > 0 && (j = itemdb_viewid(stor->storage[i].card[0])) > 0)
				WPACKETW(n*20+16) = j;
			else
				WPACKETW(n*20+16) = stor->storage[i].card[0];
			if (stor->storage[i].card[1] > 0 && (j = itemdb_viewid(stor->storage[i].card[1])) > 0)
				WPACKETW(n*20+18) = j;
			else
				WPACKETW(n*20+18) = stor->storage[i].card[1];
			if (stor->storage[i].card[2] > 0 && (j = itemdb_viewid(stor->storage[i].card[2])) > 0)
				WPACKETW(n*20+20) = j;
			else
				WPACKETW(n*20+20) = stor->storage[i].card[2];
			if (stor->storage[i].card[3] > 0 && (j = itemdb_viewid(stor->storage[i].card[3])) > 0)
				WPACKETW(n*20+22) = j;
			else
				WPACKETW(n*20+22) = stor->storage[i].card[3];
		}
		n++;
	}
	if (n) {
		WPACKETW(0) = 0xa6;
		WPACKETW(2) = 4 + n * 20;
		SENDPACKET(sd->fd, WPACKETW(2));
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_guildstorageitemlist(struct map_session_data *sd, struct guild_storage *stor)
{
	struct item_data *id;
	int i, n;

	nullpo_retr(0, sd);
	nullpo_retr(0, stor);

	n = 0;
	for(i = 0; i < MAX_GUILD_STORAGE; i++) {
		if (stor->storage[i].nameid <= 0)
			continue;
		nullpo_retr(0, id = itemdb_search(stor->storage[i].nameid));
		if (itemdb_isequip2(id))
			continue;

		WPACKETW(n*18+ 4) = i + 1;
		if (id->view_id > 0)
			WPACKETW(n*18+6) = id->view_id;
		else
			WPACKETW(n*18+6) = stor->storage[i].nameid;
		WPACKETB(n*18+ 8) = id->type;
		WPACKETB(n*18+ 9) = stor->storage[i].identify;
		WPACKETW(n*18+10) = stor->storage[i].amount;
		WPACKETW(n*18+12) = 0;
		WPACKETW(n*18+14) = stor->storage[i].card[0];
		WPACKETW(n*18+16) = stor->storage[i].card[1];
		WPACKETW(n*18+18) = stor->storage[i].card[2];
		WPACKETW(n*18+20) = stor->storage[i].card[3];
		n++;
	}
	if (n) {
		WPACKETW(0) = 0x1f0;
		WPACKETW(2) = 4 + n * 18;
		SENDPACKET(sd->fd, WPACKETW(2));
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_guildstorageequiplist(struct map_session_data *sd, struct guild_storage *stor)
{
	struct item_data *id;
	int i,j,n;

	nullpo_retr(0, sd);

	n = 0;
	for(i = 0; i < MAX_GUILD_STORAGE; i++) {
		if (stor->storage[i].nameid <= 0)
			continue;
		nullpo_retr(0, id = itemdb_search(stor->storage[i].nameid));
		if (!itemdb_isequip2(id))
			continue;
		WPACKETW(n*20+4) = i + 1;
		if (id->view_id > 0)
			WPACKETW(n*20+6) = id->view_id;
		else
			WPACKETW(n*20+6) = stor->storage[i].nameid;
		WPACKETB(n*20+ 8) = id->type;
		WPACKETB(n*20+ 9) = stor->storage[i].identify;
		WPACKETW(n*20+10) = id->equip;
		WPACKETW(n*20+12) = stor->storage[i].equip;
		WPACKETB(n*20+14) = stor->storage[i].attribute;
		WPACKETB(n*20+15) = stor->storage[i].refine;
		if (stor->storage[i].card[0] == 0x00ff || stor->storage[i].card[0] == 0x00fe || stor->storage[i].card[0] == (short)0xff00) {
			WPACKETW(n*20+16) = stor->storage[i].card[0];
			WPACKETW(n*20+18) = stor->storage[i].card[1];
			WPACKETW(n*20+20) = stor->storage[i].card[2];
			WPACKETW(n*20+22) = stor->storage[i].card[3];
		} else {
			if (stor->storage[i].card[0] > 0 && (j = itemdb_viewid(stor->storage[i].card[0])) > 0)
				WPACKETW(n*20+16) = j;
			else
				WPACKETW(n*20+16) = stor->storage[i].card[0];
			if (stor->storage[i].card[1] > 0 && (j = itemdb_viewid(stor->storage[i].card[1])) > 0)
				WPACKETW(n*20+18) = j;
			else
				WPACKETW(n*20+18)=stor->storage[i].card[1];
			if (stor->storage[i].card[2] > 0 && (j = itemdb_viewid(stor->storage[i].card[2])) > 0)
				WPACKETW(n*20+20) = j;
			else
				WPACKETW(n*20+20) = stor->storage[i].card[2];
			if (stor->storage[i].card[3] > 0 && (j = itemdb_viewid(stor->storage[i].card[3])) > 0)
				WPACKETW(n*20+22) = j;
			else
				WPACKETW(n*20+22) = stor->storage[i].card[3];
		}
		n++;
	}
	if (n) {
		WPACKETW(0) = 0xa6;
		WPACKETW(2) = 4 + n * 20;
		SENDPACKET(sd->fd, WPACKETW(2));
	}

	return 0;
}

/*==========================================
 * ステータスを送りつける
 * 表示専用数字はこの中で計算して送る
 *------------------------------------------
 */
int clif_updatestatus(struct map_session_data *sd, int type)
{
	nullpo_retr(0, sd);

	switch(type) {
		// 00b0
	case SP_WEIGHT:
		pc_checkweighticon(sd);
		WPACKETW(0) = 0xb0;
		WPACKETW(2) = type;
		WPACKETL(4) = sd->weight;
		SENDPACKET(sd->fd, 8);
		break;
	case SP_MAXWEIGHT:
		WPACKETW(0) = 0xb0;
		WPACKETW(2) = type;
		WPACKETL(4) = sd->max_weight;
		SENDPACKET(sd->fd, 8);
		break;
	case SP_SPEED:
		WPACKETW(0) = 0xb0;
		WPACKETW(2) = type;
		WPACKETL(4) = sd->speed;
		SENDPACKET(sd->fd, 8);
		break;
	case SP_BASELEVEL:
		WPACKETW(0) = 0xb0;
		WPACKETW(2) = type;
		WPACKETL(4) = sd->status.base_level;
		SENDPACKET(sd->fd, 8);
		break;
	case SP_JOBLEVEL:
		WPACKETW(0) = 0xb0;
		WPACKETW(2) = type;
		WPACKETL(4) = sd->status.job_level;
		SENDPACKET(sd->fd, 8);
		break;
	case SP_MANNER:
		WPACKETW(0) = 0xb0;
		WPACKETW(2) = type;
		WPACKETL(4) = sd->status.manner;
		SENDPACKET(sd->fd, 8);
		clif_changestatus(&sd->bl, SP_MANNER, sd->status.manner);
		break;
	case SP_STATUSPOINT:
		WPACKETW(0) = 0xb0;
		WPACKETW(2) = type;
		WPACKETL(4) = sd->status.status_point;
		SENDPACKET(sd->fd, 8);
		break;
	case SP_SKILLPOINT:
		WPACKETW(0) = 0xb0;
		WPACKETW(2) = type;
		WPACKETL(4) = sd->status.skill_point;
		SENDPACKET(sd->fd, 8);
		break;
	case SP_HIT:
		WPACKETW(0) = 0xb0;
		WPACKETW(2) = type;
		WPACKETL(4) = sd->hit;
		SENDPACKET(sd->fd, 8);
		break;
	case SP_FLEE1:
		WPACKETW(0) = 0xb0;
		WPACKETW(2) = type;
		WPACKETL(4) = sd->flee;
		SENDPACKET(sd->fd, 8);
		break;
	case SP_FLEE2:
		WPACKETW(0) = 0xb0;
		WPACKETW(2) = type;
		WPACKETL(4) = sd->flee2 / 10;
		SENDPACKET(sd->fd, 8);
		break;
	case SP_MAXHP:
		WPACKETW(0) = 0xb0;
		WPACKETW(2) = type;
		WPACKETL(4) = sd->status.max_hp;
		SENDPACKET(sd->fd, 8);
		break;
	case SP_MAXSP:
		WPACKETW(0) = 0xb0;
		WPACKETW(2) = type;
		WPACKETL(4) = sd->status.max_sp;
		SENDPACKET(sd->fd, 8);
		break;
	case SP_HP:
		WPACKETW(0) = 0xb0;
		WPACKETW(2) = type;
		WPACKETL(4) = sd->status.hp;
		SENDPACKET(sd->fd, 8);
		if(battle_config.display_hpmeter) // display hp of the player if necessary
			clif_hpmeter(sd, NULL); // NULL: send to any people, other: send to only 1 player
		break;
	case SP_SP:
		WPACKETW(0) = 0xb0;
		WPACKETW(2) = type;
		WPACKETL(4) = sd->status.sp;
		SENDPACKET(sd->fd, 8);
		break;
	case SP_ASPD:
		WPACKETW(0) = 0xb0;
		WPACKETW(2) = type;
		WPACKETL(4) = sd->aspd;
		SENDPACKET(sd->fd, 8);
		break;
	case SP_ATK1:
		WPACKETW(0) = 0xb0;
		WPACKETW(2) = type;
		WPACKETL(4) = sd->base_atk + sd->watk;
		SENDPACKET(sd->fd, 8);
		break;
	case SP_DEF1:
		WPACKETW(0) = 0xb0;
		WPACKETW(2) = type;
		WPACKETL(4) = sd->def;
		SENDPACKET(sd->fd, 8);
		break;
	case SP_MDEF1:
		WPACKETW(0) = 0xb0;
		WPACKETW(2) = type;
		WPACKETL(4) = sd->mdef;
		SENDPACKET(sd->fd, 8);
		break;
	case SP_ATK2:
		WPACKETW(0) = 0xb0;
		WPACKETW(2) = type;
		WPACKETL(4) = sd->watk2;
		SENDPACKET(sd->fd, 8);
		break;
	case SP_DEF2:
		WPACKETW(0) = 0xb0;
		WPACKETW(2) = type;
		WPACKETL(4) = sd->def2;
		SENDPACKET(sd->fd, 8);
		break;
	case SP_MDEF2:
		WPACKETW(0) = 0xb0;
		WPACKETW(2) = type;
		WPACKETL(4) = sd->mdef2;
		SENDPACKET(sd->fd, 8);
		break;
	case SP_CRITICAL:
		WPACKETW(0) = 0xb0;
		WPACKETW(2) = type;
		WPACKETL(4) = sd->critical / 10;
		SENDPACKET(sd->fd, 8);
		break;
	case SP_MATK1:
		WPACKETW(0) = 0xb0;
		WPACKETW(2) = type;
		WPACKETL(4) = sd->matk1;
		SENDPACKET(sd->fd, 8);
		break;
	case SP_MATK2:
		WPACKETW(0) = 0xb0;
		WPACKETW(2) = type;
		WPACKETL(4) = sd->matk2;
		SENDPACKET(sd->fd, 8);
		break;

	// 00b1 終了
	case SP_ZENY:
		WPACKETW(0) = 0xb1;
		WPACKETW(2) = type;
		if (sd->status.zeny < 0)
			sd->status.zeny = 0;
		WPACKETL(4) = sd->status.zeny;
		SENDPACKET(sd->fd, 8);
		break;
	case SP_BASEEXP:
		WPACKETW(0) = 0xb1;
		WPACKETW(2) = type;
		WPACKETL(4) = sd->status.base_exp;
		SENDPACKET(sd->fd, 8);
		break;
	case SP_JOBEXP:
		WPACKETW(0) = 0xb1;
		WPACKETW(2) = type;
		WPACKETL(4) = sd->status.job_exp;
		SENDPACKET(sd->fd, 8);
		break;
	case SP_NEXTBASEEXP:
		WPACKETW(0) = 0xb1;
		WPACKETW(2) = type;
		WPACKETL(4) = pc_nextbaseexp(sd);
		SENDPACKET(sd->fd, 8);
		break;
	case SP_NEXTJOBEXP:
		WPACKETW(0) = 0xb1;
		WPACKETW(2) = type;
		WPACKETL(4) = pc_nextjobexp(sd);
		SENDPACKET(sd->fd, 8);
		break;

		// 00be 終了
	case SP_USTR:
	case SP_UAGI:
	case SP_UVIT:
	case SP_UINT:
	case SP_UDEX:
	case SP_ULUK:
		WPACKETW(0) = 0xbe;
		WPACKETW(2) = type;
		WPACKETB(4) = pc_need_status_point(sd, type - SP_USTR + SP_STR);
		SENDPACKET(sd->fd, 5);
		break;

		// 013a 終了
	case SP_ATTACKRANGE:
		WPACKETW(0) = 0x13a;
		WPACKETW(2) = sd->attackrange;
		SENDPACKET(sd->fd, 4);
		break;

		// 0141 終了
	case SP_STR:
		WPACKETW( 0) = 0x141;
		WPACKETL( 2) = type;
		WPACKETL( 6) = sd->status.str;
		WPACKETL(10) = sd->paramb[0] + sd->parame[0];
		SENDPACKET(sd->fd, 14);
		break;
	case SP_AGI:
		WPACKETW( 0) = 0x141;
		WPACKETL( 2) = type;
		WPACKETL( 6) = sd->status.agi;
		WPACKETL(10) = sd->paramb[1] + sd->parame[1];
		SENDPACKET(sd->fd, 14);
		break;
	case SP_VIT:
		WPACKETW( 0) = 0x141;
		WPACKETL( 2) = type;
		WPACKETL( 6) = sd->status.vit;
		WPACKETL(10) = sd->paramb[2] + sd->parame[2];
		SENDPACKET(sd->fd, 14);
		break;
	case SP_INT:
		WPACKETW( 0) = 0x141;
		WPACKETL( 2) = type;
		WPACKETL( 6) = sd->status.int_;
		WPACKETL(10) = sd->paramb[3] + sd->parame[3];
		SENDPACKET(sd->fd, 14);
		break;
	case SP_DEX:
		WPACKETW( 0) = 0x141;
		WPACKETL( 2) = type;
		WPACKETL( 6) = sd->status.dex;
		WPACKETL(10) = sd->paramb[4] + sd->parame[4];
		SENDPACKET(sd->fd, 14);
		break;
	case SP_LUK:
		WPACKETW( 0) = 0x141;
		WPACKETL( 2) = type;
		WPACKETL( 6) = sd->status.luk;
		WPACKETL(10) = sd->paramb[5] + sd->parame[5];
		SENDPACKET(sd->fd, 14);
		break;

	case SP_CARTINFO:
		WPACKETW( 0) = 0x121;
		WPACKETW( 2) = sd->cart_num;
		WPACKETW( 4) = MAX_CART; // sd->cart_max_num; it's always MAX_CART... removed
		WPACKETL( 6) = sd->cart_weight;
		WPACKETL(10) = sd->cart_max_weight;
		SENDPACKET(sd->fd, 14);
		break;

	default:
		if (battle_config.error_log)
			printf("clif_updatestatus : make %d routine\n", type);
		return 1;
	}

	return 0;
}

int clif_changestatus(struct block_list *bl, int type, int val)
{
	struct map_session_data *sd = NULL;

	nullpo_retr(0, bl);

	if (bl->type == BL_PC)
		sd = (struct map_session_data *)bl;

//printf("clif_changestatus id:%d type:%d val:%d\n", bl->id, type, val);
	if (sd) {
		WPACKETW(0) = 0x1ab;
		WPACKETL(2) = bl->id;
		WPACKETW(6) = type;
		switch(type) {
		case SP_MANNER:
			WPACKETL(8) = val;
			break;
		default:
			if (battle_config.error_log)
				printf("clif_changestatus : make %d routine\n", type);
			return 1;
		}
		clif_send(packet_len_table[0x1ab], bl, AREA_WOS);
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_changelook(struct block_list *bl, int type, int val)
{
	struct map_session_data *sd = NULL;

	nullpo_retr(0, bl);

	if (bl->type == BL_PC)
		sd = (struct map_session_data *)bl;

	if (sd && sd->disguise > 23 && sd->disguise < 4001) // mob disguises
		return 0;

	if (sd && (type == LOOK_WEAPON || type == LOOK_SHIELD || type == LOOK_SHOES)) {
		WPACKETW(0) = 0x1d7;
		WPACKETL(2) = bl->id;
		if (type == LOOK_SHOES) {
			WPACKETB(6) = 9;
			if (sd->equip_index[2] >= 0 && sd->inventory_data[sd->equip_index[2]]) {
				if (sd->inventory_data[sd->equip_index[2]]->view_id > 0)
					WPACKETW(7) = sd->inventory_data[sd->equip_index[2]]->view_id;
				else
					WPACKETW(7) = sd->status.inventory[sd->equip_index[2]].nameid;
			} else
				WPACKETW(7) = 0;
			WPACKETW(9) = 0;
		} else {
			WPACKETB(6) = 2;
		if (sd->equip_index[9] >= 0 && sd->inventory_data[sd->equip_index[9]] && sd->view_class != 22) {
			if (sd->inventory_data[sd->equip_index[9]]->view_id > 0)
				WPACKETW(7) = sd->inventory_data[sd->equip_index[9]]->view_id;
			else
				WPACKETW(7) = sd->status.inventory[sd->equip_index[9]].nameid;
		} else
			WPACKETW(7) = 0;

		if (sd->equip_index[8] >= 0 && sd->equip_index[8] != sd->equip_index[9] && sd->inventory_data[sd->equip_index[8]] &&
			sd->view_class != 22) {
			if (sd->inventory_data[sd->equip_index[8]]->view_id > 0)
				WPACKETW(9) = sd->inventory_data[sd->equip_index[8]]->view_id;
			else
				WPACKETW(9) = sd->status.inventory[sd->equip_index[8]].nameid;
		} else
			WPACKETW(9) = 0;
		}
		clif_send(packet_len_table[0x1d7], bl, AREA);
	} else if (sd && (type == LOOK_BASE) && (val > 255)) {
		WPACKETW(0) = 0x1d7;
		WPACKETL(2) = bl->id;
		WPACKETB(6) = type;
		WPACKETW(7) = val;
		WPACKETW(9) = 0;
		clif_send(packet_len_table[0x1d7], bl, AREA);
	} else {
		WPACKETW(0) = 0xc3;
		WPACKETL(2) = bl->id;
		WPACKETB(6) = type;
		WPACKETB(7) = val;
		clif_send(packet_len_table[0xc3], bl, AREA);
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_initialstatus(struct map_session_data *sd)
{
	nullpo_retr(0, sd);

	WPACKETW( 0) = 0xbd;
	WPACKETW( 2) = sd->status.status_point;
	WPACKETB( 4) = (sd->status.str > 255) ? 255 : sd->status.str;
	WPACKETB( 5) = pc_need_status_point(sd, SP_STR);
	WPACKETB( 6) = (sd->status.agi > 255) ? 255 : sd->status.agi;
	WPACKETB( 7) = pc_need_status_point(sd, SP_AGI);
	WPACKETB( 8) = (sd->status.vit > 255) ? 255 : sd->status.vit;
	WPACKETB( 9) = pc_need_status_point(sd, SP_VIT);
	WPACKETB(10) = (sd->status.int_ > 255) ? 255 : sd->status.int_;
	WPACKETB(11) = pc_need_status_point(sd, SP_INT);
	WPACKETB(12) = (sd->status.dex > 255) ? 255 : sd->status.dex;
	WPACKETB(13) = pc_need_status_point(sd, SP_DEX);
	WPACKETB(14) = (sd->status.luk > 255) ? 255 : sd->status.luk;
	WPACKETB(15) = pc_need_status_point(sd, SP_LUK);

	WPACKETW(16) = sd->base_atk + sd->watk;
	WPACKETW(18) = sd->watk2; //atk bonus
	WPACKETW(20) = sd->matk1;
	WPACKETW(22) = sd->matk2;
	WPACKETW(24) = sd->def; // def
	WPACKETW(26) = sd->def2;
	WPACKETW(28) = sd->mdef; // mdef
	WPACKETW(30) = sd->mdef2;
	WPACKETW(32) = sd->hit;
	WPACKETW(34) = sd->flee;
	WPACKETW(36) = sd->flee2/10;
	WPACKETW(38) = sd->critical/10;
	WPACKETW(40) = sd->status.karma;
	WPACKETW(42) = sd->status.manner;

	SENDPACKET(sd->fd, packet_len_table[0xbd]);

	clif_updatestatus(sd, SP_STR);
	clif_updatestatus(sd, SP_AGI);
	clif_updatestatus(sd, SP_VIT);
	clif_updatestatus(sd, SP_INT);
	clif_updatestatus(sd, SP_DEX);
	clif_updatestatus(sd, SP_LUK);

	clif_updatestatus(sd, SP_ATTACKRANGE);
	clif_updatestatus(sd, SP_ASPD);

	return 0;
}

/*==========================================
 *矢装備
 *------------------------------------------
 */
int clif_arrowequip(struct map_session_data *sd, int val)
{
	nullpo_retr(0, sd);

	if (sd->attacktarget && sd->attacktarget > 0) // [Valaris]
		sd->attacktarget = 0;

	WPACKETW(0) = 0x013c;
	WPACKETW(2) = val + 2;//矢のアイテムID
	SENDPACKET(sd->fd, packet_len_table[0x013c]);

	return 0;
}

/*==========================================
 * type:
 * 0: Please equip arrows first. (red message)
 * 1: You can't attack or use skills because Weight Limit has been exceeded.
 * 2: You can't use skill because Weight Limit has been exceeded
 * 3: Arrow have been equiped.
 *------------------------------------------
 */
int clif_arrow_fail(struct map_session_data *sd, int type)
{
	nullpo_retr(0, sd);

	WPACKETW(0) = 0x013b;
	WPACKETW(2) = type;
	SENDPACKET(sd->fd, packet_len_table[0x013b]);

	return 0;
}

/*==========================================
 * 作成可能 矢リスト送信
 *------------------------------------------
 */
void clif_arrow_create_list(struct map_session_data *sd) {
	int i, c, view;

//	nullpo_retv(sd); // checked before to call function

	c = 4;
	for(i = 0; i < num_skill_arrow_db; i++) { // MAX_SKILL_ARROW_DB -> dynamic now
		if (pc_search_inventory(sd, skill_arrow_db[i].nameid) >= 0) { // skill_arrow_db[i].nameid > 0 && -> always > 0 at reading of DB
			if ((view = itemdb_viewid(skill_arrow_db[i].nameid)) > 0)
				WPACKETW(c) = view;
			else
				WPACKETW(c) = skill_arrow_db[i].nameid;
			c += 2;
		}
	}
	if (c > 4) {
		WPACKETW(0) = 0x1ad;
		WPACKETW(2) = c;
		SENDPACKET(sd->fd, c);
		sd->state.make_arrow_flag = 1;
	}

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_statusupack(struct map_session_data *sd, int type, int ok, int val)
{
	nullpo_retr(0, sd);

	WPACKETW(0) = 0xbc;
	WPACKETW(2) = type;
	WPACKETB(4) = ok;
	WPACKETB(5) = val;
	SENDPACKET(sd->fd, packet_len_table[0xbc]);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_equipitemack(struct map_session_data *sd, int n, int pos, int ok)
{
	nullpo_retr(0, sd);

	WPACKETW(0) = 0xaa;
	WPACKETW(2) = n + 2;
	WPACKETW(4) = pos;
	WPACKETB(6) = ok;
	SENDPACKET(sd->fd, packet_len_table[0xaa]);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_unequipitemack(struct map_session_data *sd, int n, int pos, int ok)
{
	nullpo_retr(0, sd);

	WPACKETW(0) = 0xac;
	WPACKETW(2) = n + 2;
	WPACKETW(4) = pos;
	WPACKETB(6) = ok;
	SENDPACKET(sd->fd, packet_len_table[0xac]);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_misceffect(struct block_list* bl, int type)
{
	nullpo_retr(0, bl);

	WPACKETW(0) = 0x19b;
	WPACKETL(2) = bl->id;
	WPACKETL(6) = type;
	clif_send(packet_len_table[0x19b], bl, AREA);

	return 0;
}

int clif_misceffect2(struct block_list *bl, int type) {
	nullpo_retr(0, bl);

	WPACKETW(0) = 0x1f3;
	WPACKETL(2) = bl->id;
	WPACKETL(6) = type;
	clif_send( packet_len_table[0x1f3], bl, AREA);

	return 0;
}

/*==========================================
 * 表示オプション変更
 *------------------------------------------
 */
int clif_changeoption(struct block_list* bl) {
	short option;
	struct status_change *sc_data;
//	struct map_session_data *sd;
	static const int omask[] = { 0x10,0x20 };
	static const int scnum[] = { SC_FALCON, SC_RIDING };
	int i;

	nullpo_retr(0, bl);

	option = *status_get_option(bl);
	sc_data = status_get_sc_data(bl);

#if PACKETVER > 6
	WPACKETW( 0) = 0x229;
	WPACKETL( 2) = bl->id;
	WPACKETW( 6) = *status_get_opt1(bl);
	WPACKETW( 8) = *status_get_opt2(bl);
	WPACKETL(10) = option;
	WPACKETB(14) = 0;
#else
	WPACKETW( 0) = 0x119;
	WPACKETL( 2) = bl->id;
	WPACKETW( 6) = *status_get_opt1(bl);
	WPACKETW( 8) = *status_get_opt2(bl);
	WPACKETW(10) = option;
	WPACKETB(12) = 0; // ??
#endif

	if (bl->type == BL_PC) { // disguises
		struct map_session_data *sd = ((struct map_session_data *)bl);
		if (sd && sd->disguise > 23 && sd->disguise < 4001) {
			clif_send(packet_len_table[WPACKETW(0)], bl, AREA_WOS);
			clif_spawnpc(sd);
		} else
			clif_send(packet_len_table[WPACKETW(0)], bl, AREA);
	} else
		clif_send(packet_len_table[WPACKETW(0)], bl, AREA);

	// アイコンの表示
//	if (sc_data) {
		for(i = 0; i < sizeof(omask) / sizeof(omask[0]); i++) {
			if (option & omask[i]) {
				if (sc_data[scnum[i]].timer == -1)
					status_change_start(bl, scnum[i], 0, 0, 0, 0, 0, 0);
			} else {
				status_change_end(bl, scnum[i], -1);
			}
		}
/*	}

	if (bl->type == BL_PC && (sd = (struct map_session_data *)bl))
		clif_changelook(bl, LOOK_BASE, sd->view_class);*/

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_useitemack(struct map_session_data *sd, int idx, int amount, int ok)
{
	nullpo_retr(0, sd);

	if (!ok) {
		WPACKETW(0) = 0xa8;
		WPACKETW(2) = idx + 2;
		WPACKETW(4) = amount;
		WPACKETB(6) = ok;
		SENDPACKET(sd->fd, packet_len_table[0xa8]);
	} else {
		WPACKETW(0) = 0x1c8;
		WPACKETW(2) = idx + 2;
		if (sd->inventory_data[idx] && sd->inventory_data[idx]->view_id > 0)
			WPACKETW(4) = sd->inventory_data[idx]->view_id;
		else
			WPACKETW(4) = sd->status.inventory[idx].nameid;
		WPACKETL( 6) = sd->bl.id;
		WPACKETW(10) = amount;
		WPACKETB(12) = ok;
		clif_send(packet_len_table[0x1c8], &sd->bl, AREA);
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_createchat(struct map_session_data *sd, int fail)
{
	nullpo_retr(0, sd);

	WPACKETW(0) = 0xd6;
	WPACKETB(2) = fail; // 0: ok, 1: room limit exceeded., 2: same room exists.
	SENDPACKET(sd->fd, packet_len_table[0xd6]);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_dispchat(struct chat_data *cd, int fd)
{
	if (cd == NULL || *cd->owner == NULL)
		return 1;

	WPACKETW( 0) = 0xd7;
	WPACKETW( 2) = strlen(cd->title) + 18; // add NULL [Yor]
	WPACKETL( 4) = (*cd->owner)->id;
	WPACKETL( 8) = cd->bl.id;
	WPACKETW(12) = cd->limit;
	WPACKETW(14) = cd->users;
	WPACKETB(16) = cd->pub;
	strcpy(WPACKETP(17), cd->title);
	if (fd)
		SENDPACKET(fd, WPACKETW(2));
	else
		clif_send(WPACKETW(2), *cd->owner, AREA_WOSC);

	return 0;
}

/*==========================================
 * chatの状態変更成功
 * 外部の人用と命令コード(d7->df)が違うだけ
 *------------------------------------------
 */
int clif_changechatstatus(struct chat_data *cd)
{
	if (cd == NULL || cd->usersd[0] == NULL)
		return 1;

	WPACKETW( 0) = 0xdf;
	WPACKETW( 2) = strlen(cd->title) + 18; // add NULL [Yor]
	WPACKETL( 4) = cd->usersd[0]->bl.id;
	WPACKETL( 8) = cd->bl.id;
	WPACKETW(12) = cd->limit;
	WPACKETW(14) = cd->users;
	WPACKETB(16) = cd->pub;
	strcpy(WPACKETP(17), cd->title);
	clif_send(WPACKETW(2), &cd->usersd[0]->bl, CHAT);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_clearchat(struct chat_data *cd, int fd)
{
	nullpo_retr(0, cd);

	WPACKETW(0) = 0xd8;
	WPACKETL(2) = cd->bl.id;
	if (fd)
		SENDPACKET(fd, packet_len_table[0xd8]);
	else
		clif_send(packet_len_table[0xd8], *cd->owner, AREA_WOSC);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_joinchatfail(struct map_session_data *sd, int fail)
{
	nullpo_retr(0, sd);

	WPACKETW(0) = 0xda;
	WPACKETB(2) = fail; // 0: The room is full, 1: Incorrect User ID or Password. Please, try again, 2: You have been kicked off this room, 3: (void), 4: You do not have enough zeny., 5: You are not the required level.
	SENDPACKET(sd->fd, packet_len_table[0xda]);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_joinchatok(struct map_session_data *sd, struct chat_data* cd)
{
	int i;

	nullpo_retr(0, sd);
	nullpo_retr(0, cd);

	WPACKETW(0) = 0xdb;
	WPACKETW(2) = 8 + (28 * cd->users);
	WPACKETL(4) = cd->bl.id;
	for(i = 0;i < cd->users; i++) {
		WPACKETL(8+i*28) = (i != 0) || ((*cd->owner)->type == BL_NPC);
		strncpy(WPACKETP(8+i*28+4), cd->usersd[i]->status.name, 24);
	}
	SENDPACKET(sd->fd, WPACKETW(2));

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_addchat(struct chat_data* cd, struct map_session_data *sd) {
	nullpo_retr(0, sd);
	nullpo_retr(0, cd);

	WPACKETW( 0) = 0x0dc;
	WPACKETW( 2) = cd->users;
	strncpy(WPACKETP(4), sd->status.name, 24);
	clif_send(packet_len_table[0xdc], &sd->bl, CHAT_WOS);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_changechatowner(struct chat_data* cd, struct map_session_data *sd)
{
	nullpo_retr(0, sd);
	nullpo_retr(0, cd);

	WPACKETW( 0) = 0xe1;
	WPACKETL( 2) = 1;
	strncpy(WPACKETP(6), cd->usersd[0]->status.name, 24);
	WPACKETW(30) = 0xe1;
	WPACKETL(32) = 0;
	strncpy(WPACKETP(36), sd->status.name, 24);

	clif_send(packet_len_table[0xe1] * 2, &sd->bl, CHAT);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_leavechat(struct chat_data* cd, struct map_session_data *sd) {
	nullpo_retr(0, sd);
	nullpo_retr(0, cd);

	WPACKETW( 0) = 0xdd;
	WPACKETW( 2) = cd->users - 1;
	strncpy(WPACKETP(4), sd->status.name, 24);
	WPACKETB(28) = 0;

	clif_send(packet_len_table[0xdd], &sd->bl, CHAT);

	return 0;
}

/*==========================================
 * 取り引き要請受け
 *------------------------------------------
 */
int clif_traderequest(struct map_session_data *sd, char *name)
{
//	struct map_session_data *target_sd;

	nullpo_retr(0, sd);
//	nullpo_retr(0, (target_sd = map_id2sd(sd->trade_partner)));

//#if PACKETVER < 6
	WPACKETW(0) = 0xe5;
	strncpy(WPACKETP(2),name, 24);
	SENDPACKET(sd->fd, packet_len_table[0xe5]);
/*#else
	WPACKETW(0) = 0x1f4;
	strncpy(WPACKETP(2), name, 24);
	WPACKETL(26) = target_sd->status.char_id; //良く分からないからとりあえずchar_id
	WPACKETW(30) = target_sd->status.base_level;
	SENDPACKET(sd->fd, packet_db[0x1f4]);*/
//#endif

	return 0;
}

/*==========================================
 * 取り引き要求応答
 *------------------------------------------
 */
int clif_tradestart(struct map_session_data *sd, unsigned char type) { // 00e7 <fail>.B: response to requesting trade: 0: You are too far away from the person to trade., 1: This Character is not currently online or does not exist, 2: The person is in another trade., 3: (trade ok->open the trade window)., 4: The deal has been rejected.
//	struct map_session_data *target_sd;

	nullpo_retr(0, sd);
//	nullpo_retr(-1, (target_sd = map_id2sd(sd->trade_partner)));

//#if PACKETVER < 6
	WPACKETW(0) = 0xe7; // 00e7 <fail>.B: response to requesting trade: 0: You are too far away from the person to trade., 1: This Character is not currently online or does not exist, 2: The person is in another trade., 3: (trade ok->open the trade window)., 4: The deal has been rejected.
	WPACKETB(2) = type;
	SENDPACKET(sd->fd, packet_len_table[0xe7]);
/*#else
	WPACKETW(0) = 0x1f5;
	WPACKETB(2) = type;
	WPACKETL(3) = target_sd->status.char_id;	//良く分からないからとりあえずchar_id
	WPACKETW(7) = target_sd->status.base_level;
	SENDPACKET(sd->fd, packet_db[0x1f5]);*/
//#endif

	return 0;
}

/*==========================================
 * 相手方からのアイテム追加
 *------------------------------------------
 */
int clif_tradeadditem(struct map_session_data *sd, struct map_session_data *tsd, int idx, int amount)
{
	int j;

	nullpo_retr(0, sd);
	nullpo_retr(0, tsd);

	WPACKETW( 0) = 0xe9;
	WPACKETL( 2) = amount;
	if (idx == 0) {
		WPACKETW( 6) = 0; // type id
		WPACKETB( 8) = 0; //identify flag
		WPACKETB( 9) = 0; // attribute
		WPACKETB(10) = 0; //refine
		WPACKETW(11) = 0; //card (4w)
		WPACKETW(13) = 0; //card (4w)
		WPACKETW(15) = 0; //card (4w)
		WPACKETW(17) = 0; //card (4w)
	} else {
		idx -= 2;
		if (sd->inventory_data[idx] && sd->inventory_data[idx]->view_id > 0)
			WPACKETW( 6) = sd->inventory_data[idx]->view_id;
		else
			WPACKETW( 6) = sd->status.inventory[idx].nameid; // type id
		WPACKETB( 8) = sd->status.inventory[idx].identify; //identify flag
		WPACKETB( 9) = sd->status.inventory[idx].attribute; // attribute
		WPACKETB(10) = sd->status.inventory[idx].refine; //refine
		if (sd->status.inventory[idx].card[0] == 0x00ff || sd->status.inventory[idx].card[0] == 0x00fe || sd->status.inventory[idx].card[0] == (short)0xff00) {
			WPACKETW(11) = sd->status.inventory[idx].card[0]; //card (4w)
			WPACKETW(13) = sd->status.inventory[idx].card[1]; //card (4w)
			WPACKETW(15) = sd->status.inventory[idx].card[2]; //card (4w)
			WPACKETW(17) = sd->status.inventory[idx].card[3]; //card (4w)
		} else {
			if (sd->status.inventory[idx].card[0] > 0 && (j = itemdb_viewid(sd->status.inventory[idx].card[0])) > 0)
				WPACKETW(11) = j;
			else
				WPACKETW(11) = sd->status.inventory[idx].card[0];
			if (sd->status.inventory[idx].card[1] > 0 && (j = itemdb_viewid(sd->status.inventory[idx].card[1])) > 0)
				WPACKETW(13) = j;
			else
				WPACKETW(13) = sd->status.inventory[idx].card[1];
			if (sd->status.inventory[idx].card[2] > 0 && (j = itemdb_viewid(sd->status.inventory[idx].card[2])) > 0)
				WPACKETW(15) = j;
			else
				WPACKETW(15) = sd->status.inventory[idx].card[2];
			if (sd->status.inventory[idx].card[3] > 0 && (j = itemdb_viewid(sd->status.inventory[idx].card[3])) > 0)
				WPACKETW(17) = j;
			else
				WPACKETW(17) = sd->status.inventory[idx].card[3];
		}
	}
	SENDPACKET(tsd->fd, packet_len_table[0xe9]);

	return 0;
}

/*==========================================
 * アイテム追加成功/失敗
 *------------------------------------------
 */
int clif_tradeitemok(struct map_session_data *sd, int idx, int fail)
{
	nullpo_retr(0, sd);

	WPACKETW(0) = 0xea;
	WPACKETW(2) = idx;
	WPACKETB(4) = fail;
	SENDPACKET(sd->fd, packet_len_table[0xea]);

	return 0;
}

/*==========================================
 * 取り引きok押し
 *------------------------------------------
 */
int clif_tradedeal_lock(struct map_session_data *sd, int fail)
{
	nullpo_retr(0, sd);

	WPACKETW(0) = 0xec;
	WPACKETB(2) = fail; // 0=you 1=the other person
	SENDPACKET(sd->fd, packet_len_table[0xec]);

	return 0;
}

/*==========================================
 * 取り引きがキャンセルされました
 *------------------------------------------
 */
int clif_tradecancelled(struct map_session_data *sd)
{
	nullpo_retr(0, sd);

	WPACKETW(0) = 0xee;
	SENDPACKET(sd->fd, packet_len_table[0xee]);

	return 0;
}

/*==========================================
 * 取り引き完了
 *------------------------------------------
 */
int clif_tradecompleted(struct map_session_data *sd, int fail)
{
	nullpo_retr(0, sd);

	WPACKETW(0) = 0xf0;
	WPACKETB(2) = fail;
	SENDPACKET(sd->fd, packet_len_table[0xf0]);

	return 0;
}

/*==========================================
 * カプラ倉庫のアイテム数を更新
 *------------------------------------------
 */
int clif_updatestorageamount(struct map_session_data *sd, struct storage *stor)
{
	nullpo_retr(0, sd);
	nullpo_retr(0, stor);

	WPACKETW(0) = 0xf2; // update storage amount
	WPACKETW(2) = stor->storage_amount;  //items
	WPACKETW(4) = MAX_STORAGE; //items max
	SENDPACKET(sd->fd, packet_len_table[0xf2]);

	return 0;
}

/*==========================================
 * カプラ倉庫にアイテムを追加する
 *------------------------------------------
 */
int clif_storageitemadded(struct map_session_data *sd, struct storage *stor, int idx, int amount)
{
	int view, j;

	nullpo_retr(0, sd);
	nullpo_retr(0, stor);

	WPACKETW( 0) = 0xf4; // Storage item added
	WPACKETW( 2) = idx + 1; // index
	WPACKETL( 4) = amount; // amount
	if ((view = itemdb_viewid(stor->storage[idx].nameid)) > 0)
		WPACKETW( 8) = view;
	else
		WPACKETW( 8) = stor->storage[idx].nameid; // id
	WPACKETB(10) = stor->storage[idx].identify; //identify flag
	WPACKETB(11) = stor->storage[idx].attribute; // attribute
	WPACKETB(12) = stor->storage[idx].refine; //refine
	if (stor->storage[idx].card[0]==0x00ff || stor->storage[idx].card[0]==0x00fe || stor->storage[idx].card[0]==(short)0xff00) {
		WPACKETW(13) = stor->storage[idx].card[0]; //card (4w)
		WPACKETW(15) = stor->storage[idx].card[1]; //card (4w)
		WPACKETW(17) = stor->storage[idx].card[2]; //card (4w)
		WPACKETW(19) = stor->storage[idx].card[3]; //card (4w)
	} else {
		if (stor->storage[idx].card[0] > 0 && (j = itemdb_viewid(stor->storage[idx].card[0])) > 0)
			WPACKETW(13) = j;
		else
			WPACKETW(13) = stor->storage[idx].card[0];
		if (stor->storage[idx].card[1] > 0 && (j = itemdb_viewid(stor->storage[idx].card[1])) > 0)
			WPACKETW(15) = j;
		else
			WPACKETW(15) = stor->storage[idx].card[1];
		if (stor->storage[idx].card[2] > 0 && (j = itemdb_viewid(stor->storage[idx].card[2])) > 0)
			WPACKETW(17) = j;
		else
			WPACKETW(17) = stor->storage[idx].card[2];
		if (stor->storage[idx].card[3] > 0 && (j = itemdb_viewid(stor->storage[idx].card[3])) > 0)
			WPACKETW(19) = j;
		else
			WPACKETW(19) = stor->storage[idx].card[3];
	}
	SENDPACKET(sd->fd, packet_len_table[0xf4]);

	sd->state.modified_storage_flag = 1; // 0: not modified, 1: modified, 2: modified and sended to char-server for saving

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_updateguildstorageamount(struct map_session_data *sd, struct guild_storage *stor)
{
	nullpo_retr(0, sd);
	nullpo_retr(0, stor);

	WPACKETW(0) = 0xf2;  // update storage amount
	WPACKETW(2) = stor->storage_amount;  //items
	WPACKETW(4) = MAX_GUILD_STORAGE; //items max
	SENDPACKET(sd->fd, packet_len_table[0xf2]);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_guildstorageitemadded(struct map_session_data *sd, struct guild_storage *stor, int idx, int amount)
{
	int view, j;

	nullpo_retr(0, sd);
	nullpo_retr(0, stor);

	WPACKETW( 0) = 0xf4; // Storage item added
	WPACKETW( 2) = idx + 1; // index
	WPACKETL( 4) = amount; // amount
	if ((view = itemdb_viewid(stor->storage[idx].nameid)) > 0)
		WPACKETW( 8) = view;
	else
		WPACKETW( 8) = stor->storage[idx].nameid; // id
	WPACKETB(10) = stor->storage[idx].identify; //identify flag
	WPACKETB(11) = stor->storage[idx].attribute; // attribute
	WPACKETB(12) = stor->storage[idx].refine; //refine
	if (stor->storage[idx].card[0] == 0x00ff || stor->storage[idx].card[0] == 0x00fe || stor->storage[idx].card[0] == (short)0xff00) {
		WPACKETW(13) = stor->storage[idx].card[0]; //card (4w)
		WPACKETW(15) = stor->storage[idx].card[1]; //card (4w)
		WPACKETW(17) = stor->storage[idx].card[2]; //card (4w)
		WPACKETW(19) = stor->storage[idx].card[3]; //card (4w)
	} else {
		if (stor->storage[idx].card[0] > 0 && (j = itemdb_viewid(stor->storage[idx].card[0])) > 0)
			WPACKETW(13) = j;
		else
			WPACKETW(13) = stor->storage[idx].card[0];
		if (stor->storage[idx].card[1] > 0 && (j = itemdb_viewid(stor->storage[idx].card[1])) > 0)
			WPACKETW(15) = j;
		else
			WPACKETW(15) = stor->storage[idx].card[1];
		if (stor->storage[idx].card[2] > 0 && (j = itemdb_viewid(stor->storage[idx].card[2])) > 0)
			WPACKETW(17) = j;
		else
			WPACKETW(17) = stor->storage[idx].card[2];
		if (stor->storage[idx].card[3] > 0 && (j = itemdb_viewid(stor->storage[idx].card[3])) > 0)
			WPACKETW(19) = j;
		else
			WPACKETW(19) = stor->storage[idx].card[3];
	}
	SENDPACKET(sd->fd, packet_len_table[0xf4]);

	stor->modified_storage_flag = 1; // Guild storage is modified

	return 0;
}

/*==========================================
 * カプラ倉庫からアイテムを取り去る
 *------------------------------------------
 */
int clif_storageitemremoved(struct map_session_data *sd, int idx, int amount)
{
	nullpo_retr(0, sd);

	WPACKETW(0) = 0xf6; // Storage item removed
	WPACKETW(2) = idx + 1;
	WPACKETL(4) = amount;
	SENDPACKET(sd->fd, packet_len_table[0xf6]);

	return 0;
}

/*==========================================
 * カプラ倉庫を閉じる
 *------------------------------------------
 */
int clif_storageclose(struct map_session_data *sd)
{
	nullpo_retr(0, sd);

	WPACKETW(0) = 0xf8; // Storage Closed
	SENDPACKET(sd->fd, packet_len_table[0xf8]);

	return 0;
}

//
// callback系 ?
//
/*==========================================
 * PC表示
 *------------------------------------------
 */
void clif_getareachar_pc(struct map_session_data* sd, struct map_session_data* dstsd)
{
	int len;

	nullpo_retv(sd);
	nullpo_retv(dstsd);

	if (dstsd->walktimer != -1) {
		len = clif_set007b(dstsd);
		SENDPACKET(sd->fd, len);
	} else {
		len = clif_set0078(dstsd);
		SENDPACKET(sd->fd, len);
	}

	if (dstsd->chatID) {
		struct chat_data *cd;
		cd = (struct chat_data*)map_id2bl(dstsd->chatID);
		if (cd->usersd[0] == dstsd)
			clif_dispchat(cd, sd->fd);
	}
	if (dstsd->vender_id) {
		clif_showvendingboard(&dstsd->bl, dstsd->message, sd->fd); // 80 + NULL (shop title)
	}
	if (dstsd->spiritball > 0) {
		WPACKETW(0) = 0x1e1;
		WPACKETL(2) = dstsd->bl.id;
		WPACKETW(6) = dstsd->spiritball;
		SENDPACKET(sd->fd, packet_len_table[0x1e1]);
	}
	if (battle_config.save_clothcolor && dstsd->status.clothes_color > 0)
		clif_changelook(&dstsd->bl, LOOK_CLOTHES_COLOR, dstsd->status.clothes_color);
	if (sd->status.manner < 0)
		clif_changestatus(&sd->bl, SP_MANNER, sd->status.manner); // not dstsd instead of sd ???

	// display hp of the player if necessary
	if(battle_config.display_hpmeter)
		clif_hpmeter(dstsd, sd); // NULL: send to any people, other: send to only 1 player
}

/*==========================================
 * NPC表示
 *------------------------------------------
 */
void clif_getareachar_npc(struct map_session_data* sd, struct npc_data* nd) {
	int len;

	nullpo_retv(sd);
	nullpo_retv(nd);

	if (nd->class < 0 || nd->flag & 1 || nd->class == INVISIBLE_CLASS)
		return;

	if (nd->state.state == MS_WALK) {
		len = clif_npc007b(nd);
	} else {
		len = clif_npc0078(nd);
	}
	SENDPACKET(sd->fd, len);

	if (nd->chat_id)
		clif_dispchat((struct chat_data*)map_id2bl(nd->chat_id), sd->fd);
}

/*==========================================
 * 移動停止
 *------------------------------------------
 */
int clif_movemob(struct mob_data *md)
{
	int len;

	nullpo_retr(0, md);

	len = clif_mob007b(md);
	clif_send(len, &md->bl, AREA);

	if (mob_get_equip(md->class) > 0) // mob equipment [Valaris]
		clif_mob_equip(md, mob_get_equip(md->class));

	if (md->size == 2) // tiny/big mobs [Valaris]
		clif_specialeffect(&md->bl, 423, 0); // flag: 0: player see in the area (normal), 1: only player see only by player, 2: all players in a map that see only their (not see others), 3: all players that see only their (not see others)
	else if (md->size == 1)
		clif_specialeffect(&md->bl, 421, 0); // flag: 0: player see in the area (normal), 1: only player see only by player, 2: all players in a map that see only their (not see others), 3: all players that see only their (not see others)

	return 0;
}

/*==========================================
 * モンスターの位置修正
 *------------------------------------------
 */
int clif_fixmobpos(struct mob_data *md)
{
	int len;

	nullpo_retr(0, md);

	if (md->state.state == MS_WALK) {
		len = clif_mob007b(md);
		clif_send(len, &md->bl, AREA);
	} else {
		len = clif_mob0078(md);
		clif_send(len, &md->bl, AREA);
	}

	return 0;
}

/*==========================================
 * PCの位置修正
 *------------------------------------------
 */
int clif_fixpcpos(struct map_session_data *sd)
{
	int len;

	nullpo_retr(0, sd);

	if (sd->walktimer != -1) {
		len = clif_set007b(sd);
		clif_send(len, &sd->bl, AREA);
	} else {
		len = clif_set0078(sd);
		clif_send(len, &sd->bl, AREA);
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_fixpetpos(struct pet_data *pd)
{
	int len;

	nullpo_retr(0, pd);

	if (pd->state.state == MS_WALK) {
		len = clif_pet007b(pd);
		clif_send(len, &pd->bl, AREA_PET);
	} else {
		len = clif_pet0078(pd);
		clif_send(len, &pd->bl, AREA_PET);
	}

	return 0;
}

// npc walking [Valaris]
int clif_fixnpcpos(struct npc_data *nd)
{
	int len;

	nullpo_retr(0, nd);

	if (nd->state.state == MS_WALK) {
		len = clif_npc007b(nd);
		clif_send(len, &nd->bl, AREA);
	} else {
		len = clif_npc0078(nd);
		clif_send(len, &nd->bl, AREA);
	}

	return 0;
}

/*==========================================
 * 通常攻撃エフェクト＆ダメージ
 *------------------------------------------
 */
int clif_damage(struct block_list *src, struct block_list *dst, unsigned int tick, int sdelay, int ddelay, int damage, int div_, int type, int damage2)
{
	struct status_change *sc_data;

	nullpo_retr(0, src);
	nullpo_retr(0, dst);

	sc_data = status_get_sc_data(dst);

	if (type != 4 && dst->type == BL_PC && !map[dst->m].flag.gvg && ((struct map_session_data *)dst)->special_state.infinite_endure)
		type = 9;
	if (sc_data) {
		if (type != 4 && !map[dst->m].flag.gvg && (sc_data[SC_ENDURE].timer != -1 ||
			sc_data[SC_CONCENTRATION].timer != -1 || sc_data[SC_BERSERK].timer != -1))
			type = 9;
		if (sc_data[SC_HALLUCINATION].timer != -1) {
			if (damage > 0)
				damage = damage * (5+sc_data[SC_HALLUCINATION].val1) + rand() % 100;
			if (damage2 > 0)
				damage2 = damage2 * (5+sc_data[SC_HALLUCINATION].val1) + rand() % 100;
		}
	}

	WPACKETW( 0) = 0x8a;
	WPACKETL( 2) = src->id;
	WPACKETL( 6) = dst->id;
	WPACKETL(10) = tick;
	WPACKETL(14) = sdelay;
	WPACKETL(18) = ddelay;
	WPACKETW(22) = (damage > 0x7fff) ? 0x7fff : damage;
	WPACKETW(24) = div_;
	WPACKETB(26) = type;
	WPACKETW(27) = damage2;
	clif_send(packet_len_table[0x8a], src, AREA);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_getareachar_mob(struct map_session_data* sd, struct mob_data* md)
{
	int len;

	nullpo_retv(sd);
	nullpo_retv(md);

	if (md->state.state == MS_WALK) {
		len = clif_mob007b(md);
		SENDPACKET(sd->fd, len);
	} else {
		len = clif_mob0078(md);
		SENDPACKET(sd->fd, len);
	}

	if (mob_get_equip(md->class) > 0) // mob equipment [Valaris]
		clif_mob_equip(md,mob_get_equip(md->class));

	if (md->size == 2) // tiny/big mobs [Valaris]
		clif_specialeffect(&md->bl, 423, 0); // flag: 0: player see in the area (normal), 1: only player see only by player, 2: all players in a map that see only their (not see others), 3: all players that see only their (not see others)
	else if (md->size == 1)
		clif_specialeffect(&md->bl, 421, 0); // flag: 0: player see in the area (normal), 1: only player see only by player, 2: all players in a map that see only their (not see others), 3: all players that see only their (not see others)
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_getareachar_pet(struct map_session_data* sd, struct pet_data* pd) {
	int len;

	nullpo_retv(sd);
	nullpo_retv(pd);

	if (pd->state.state == MS_WALK) {
		len = clif_pet007b(pd);
		if (!(WPACKETW(14) < 24 || WPACKETW(14) > 4000)) {
			if (sd->packet_ver < 12) { // 0: old, 1: 7july04, 2: 13july04, 3: 26july04, 4: 9aug04/16aug04/17aug04, 5: 6sept04, 6: 21sept04, 7: 18oct04, 8: 25oct04/08nov04, 9: 6dec04, 10: 10jan05, 11: 9may05, 12: 28jun05, 13: 4april06 (by [Yor])
				WPACKETW(16) = 0x14; // pet_hair_style
			} else {
				// note: version of 0614 is not used in freya (value = 24)
				WPACKETW(16) = 100; // pet_hair_style
			}
		}
		SENDPACKET(sd->fd, len);
	} else {
		len = clif_pet0078(pd);
		if (!(WPACKETW(14) < 24 || WPACKETW(14) > 4000)) {
			if (sd->packet_ver < 12) { // 0: old, 1: 7july04, 2: 13july04, 3: 26july04, 4: 9aug04/16aug04/17aug04, 5: 6sept04, 6: 21sept04, 7: 18oct04, 8: 25oct04/08nov04, 9: 6dec04, 10: 10jan05, 11: 9may05, 12: 28jun05, 13: 4april06 (by [Yor])
				WPACKETW(16) = 0x14; // pet_hair_style
			} else {
				// note: version of 0614 is not used in freya (value = 24)
				WPACKETW(16) = 100; // pet_hair_style
			}
		}
		SENDPACKET(sd->fd, len);
	}
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_getareachar_item(struct map_session_data* sd, struct flooritem_data* fitem)
{
	int view;

	nullpo_retv(sd);
	nullpo_retv(fitem);

	//009d <ID>.l <item ID>.w <identify flag>.B <X>.w <Y>.w <amount>.w <subX>.B <subY>.B
	WPACKETW( 0) = 0x9d;
	WPACKETL( 2) = fitem->bl.id;
	if ((view = itemdb_viewid(fitem->item_data.nameid)) > 0)
		WPACKETW(6) = view;
	else
		WPACKETW(6) = fitem->item_data.nameid;
	WPACKETB( 8) = fitem->item_data.identify;
	WPACKETW( 9) = fitem->bl.x;
	WPACKETW(11) = fitem->bl.y;
	WPACKETW(13) = fitem->item_data.amount;
	WPACKETB(15) = fitem->subx;
	WPACKETB(16) = fitem->suby;

	SENDPACKET(sd->fd, packet_len_table[0x9d]);
}

/*==========================================
 * 場所スキルエフェクトが視界に入る
 *------------------------------------------
 */
int clif_getareachar_skillunit(struct map_session_data *sd, struct skill_unit *unit)
{
	//struct block_list *bl;

	nullpo_retr(0, unit);

	//bl = map_id2bl(unit->group->src_id);
	memset(WPACKETP(0), 0, packet_len_table[0x11f]);
	WPACKETW( 0) = 0x11f;
	WPACKETL( 2) = unit->bl.id;
	WPACKETL( 6) = unit->group->src_id;
	WPACKETW(10) = unit->bl.x;
	WPACKETW(12) = unit->bl.y;
	WPACKETB(14) = unit->group->unit_id;
	WPACKETB(15) = 0;

	//Use invisible unit id for traps.
	/*if (battle_config.traps_setting&1 && skill_get_inf2(unit->group->skill_id)&INF2_TRAP)
		WFIFOB(fd,14)=UNT_ATTACK_SKILLS;
	else*/

/*
	if(unit->group->skill_id == WZ_ICEWALL)
		clif_set0192(fd,unit->bl.m,unit->bl.x,unit->bl.y,5);
	return 0;

	memset(WPACKETP(0), 0, packet_len_table[0x1c9]);
	WPACKETW( 0) = 0x1c9;
	WPACKETL( 2) = unit->bl.id;
	WPACKETL( 6) = unit->group->src_id;
	WPACKETW(10) = unit->bl.x;
	WPACKETW(12) = unit->bl.y;
	WPACKETB(14) = unit->group->unit_id;
	WPACKETB(15) = 1;
//	if (unit->group->unit_id==0xb0) { //グラフィティ
//		WPACKETB(15+ 1) = 1;
//		memcpy(WPACKETP(15+ 2), unit->group->valstr, 80);
//	}else{
		WPACKETL(15+ 1) = 0;                	//1-4調べた限り固定
		WPACKETL(15+ 5) = 0;                	//5-8調べた限り固定
		                                    	//9-12マップごとで一定の77-80とはまた違う4バイトのかなり大きな数字
		WPACKETL(15+13) = unit->bl.y - 0x12;	//13-16ユニットのY座標-18っぽい(Y:17でFF FF FF FF)
		WPACKETL(15+17) = 0x004f37dd;       	//17-20調べた限り固定
		WPACKETL(15+21) = 0x0012f674;       	//21-24調べた限り固定
		WPACKETL(15+25) = 0x0012f664;       	//25-28調べた限り固定
		WPACKETL(15+29) = 0x0012f654;       	//29-32調べた限り固定
		WPACKETL(15+33) = 0x77527bbc;       	//33-36調べた限り固定
		                                    	//37-39
		WPACKETB(15+40) = 0x2d;             	//40調べた限り固定
		WPACKETL(15+41) = 0;                	//41-44調べた限り0固定
		WPACKETL(15+45) = 0;                	//45-48調べた限り0固定
		WPACKETL(15+49) = 0;                	//49-52調べた限り0固定
		WPACKETL(15+53) = 0x0048d919;       	//53-56調べた限り固定
		WPACKETL(15+57) = 0x0000003e;       	//57-60調べた限り固定
		WPACKETL(15+61) = 0x0012f66c;       	//61-64調べた限り固定
		                                    	//65-68
		                                    	//69-72
		if (bl) WPACKETL(15+73) = bl->y;    	//73-76術者のY座標
			WPACKETL(15+77) = unit->bl.m;    	//77-80マップIDかなぁ？かなり2バイトで足りそうな数字
		WPACKETB(15+81) = 0xaa;             	//81終端文字0xaa
//	}


	if (unit->group->unit_id == 0xb0) {
		WPACKETL(15) = 1;
		WPACKETL(16) = 1;
		memcpy(WPACKETP(17), unit->group->valstr, 80);
	}
*/
	SENDPACKET(sd->fd, packet_len_table[0x11f]);

	if (unit->group->skill_id == WZ_ICEWALL)
		clif_set0192(sd->fd, unit->bl.m, unit->bl.x, unit->bl.y, 5);

	return 0;
}

/*==========================================
 * 場所スキルエフェクトが視界から消える
 *------------------------------------------
 */
int clif_clearchar_skillunit(struct skill_unit *unit, int fd)
{
	nullpo_retr(0, unit);

	WPACKETW(0) = 0x120;
	WPACKETL(2) = unit->bl.id;
	SENDPACKET(fd, packet_len_table[0x120]);
	if (unit->group && unit->group->skill_id == WZ_ICEWALL)
		clif_set0192(fd, unit->bl.m, unit->bl.x, unit->bl.y, unit->val2);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_01ac(struct block_list *bl)
{
	nullpo_retr(0, bl);

	WPACKETW(0) = 0x1ac;
	WPACKETL(2) = bl->id;
	clif_send(packet_len_table[0x1ac], bl, AREA);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_getareachar(struct block_list* bl, va_list ap)
{
	struct map_session_data *sd;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);

	sd = va_arg(ap, struct map_session_data*);

	if (sd == NULL || session[sd->fd] == NULL)
		return 0;

	switch(bl->type) {
	case BL_PC:
		if (sd == (struct map_session_data*)bl)
			break;
		clif_getareachar_pc(sd, (struct map_session_data*)bl);
		break;
	case BL_NPC:
		clif_getareachar_npc(sd, (struct npc_data*)bl);
		break;
	case BL_MOB:
		clif_getareachar_mob(sd, (struct mob_data*)bl);
		break;
	case BL_PET:
		clif_getareachar_pet(sd, (struct pet_data*)bl);
		break;
	case BL_ITEM:
		clif_getareachar_item(sd, (struct flooritem_data*)bl);
		break;
	case BL_SKILL:
		clif_getareachar_skillunit(sd, (struct skill_unit *)bl);
		break;
	default:
		if (battle_config.error_log)
			printf("get area char ??? %d\n", bl->type);
		break;
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_pcoutsight(struct block_list *bl, va_list ap)
{
	struct map_session_data *sd, *dstsd;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);
	nullpo_retr(0, sd = va_arg(ap, struct map_session_data*));

	switch(bl->type) {
	case BL_PC:
		dstsd = (struct map_session_data*) bl;
		if (sd != dstsd) {
			clif_clearchar_id(dstsd->bl.id, 0, sd->fd);
			clif_clearchar_id(sd->bl.id, 0, dstsd->fd);
			if (dstsd->chatID) {
				struct chat_data *cd;
				cd = (struct chat_data*)map_id2bl(dstsd->chatID);
				if (cd->usersd[0] == dstsd)
					clif_dispchat(cd, sd->fd);
			}
			if (dstsd->vender_id)
				clif_closevendingboard(&dstsd->bl, sd->fd);
		}
		break;
	case BL_NPC:
		if (((struct npc_data *)bl)->class != INVISIBLE_CLASS)
			clif_clearchar_id(bl->id, 0, sd->fd);
		break;
	case BL_MOB:
	case BL_PET:
		clif_clearchar_id(bl->id, 0, sd->fd);
		break;
	case BL_ITEM:
		clif_clearflooritem((struct flooritem_data*)bl,sd->fd);
		break;
	case BL_SKILL:
		clif_clearchar_skillunit((struct skill_unit *)bl,sd->fd);
		break;
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_pcinsight(struct block_list *bl, va_list ap)
{
	struct map_session_data *sd, *dstsd;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);
	nullpo_retr(0, sd = va_arg(ap, struct map_session_data*));

	switch(bl->type) {
	case BL_PC:
		dstsd = (struct map_session_data *)bl;
		if (sd != dstsd) {
			clif_getareachar_pc(sd, dstsd);
			clif_getareachar_pc(dstsd, sd);
		}
		break;
	case BL_NPC:
		clif_getareachar_npc(sd, (struct npc_data*)bl);
		break;
	case BL_MOB:
		clif_getareachar_mob(sd, (struct mob_data*)bl);
		break;
	case BL_PET:
		clif_getareachar_pet(sd, (struct pet_data*)bl);
		break;
	case BL_ITEM:
		clif_getareachar_item(sd, (struct flooritem_data*)bl);
		break;
	case BL_SKILL:
		clif_getareachar_skillunit(sd, (struct skill_unit *)bl);
		break;
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_moboutsight(struct block_list *bl, va_list ap)
{
	struct map_session_data *sd;
	struct mob_data *md;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);
	nullpo_retr(0, md = va_arg(ap, struct mob_data*));

	if (bl->type == BL_PC && ((sd = (struct map_session_data*)bl) != NULL) && sd->fd >= 0 && session[sd->fd] != NULL) {
		clif_clearchar_id(md->bl.id, 0, sd->fd);
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_mobinsight(struct block_list *bl, va_list ap)
{
	struct map_session_data *sd;
	struct mob_data *md;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);

	md = va_arg(ap, struct mob_data*);
	if (bl->type == BL_PC && ((sd = (struct map_session_data*)bl) != NULL) && sd->fd >= 0 && session[sd->fd] != NULL) {
		clif_getareachar_mob(sd, md);
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_petoutsight(struct block_list *bl, va_list ap)
{
	struct map_session_data *sd;
	struct pet_data *pd;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);
	nullpo_retr(0, pd = va_arg(ap, struct pet_data*));

	if (bl->type == BL_PC && ((sd = (struct map_session_data*)bl) != NULL) && sd->fd >= 0 && session[sd->fd] != NULL) {
		clif_clearchar_id(pd->bl.id, 0, sd->fd);
	}

	return 0;
}

// npc walking [Valaris]
int clif_npcoutsight(struct block_list *bl, va_list ap)
{
	struct map_session_data *sd;
	struct npc_data *nd;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);
	nullpo_retr(0, nd = va_arg(ap, struct npc_data*));

	if (bl->type == BL_PC && ((sd = (struct map_session_data*)bl) != NULL) && sd->fd >= 0 && session[sd->fd] != NULL) {
		clif_clearchar_id(nd->bl.id, 0, sd->fd);
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_petinsight(struct block_list *bl, va_list ap)
{
	struct map_session_data *sd;
	struct pet_data *pd;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);

	pd = va_arg(ap, struct pet_data*);
	if (bl->type == BL_PC && ((sd = (struct map_session_data*)bl) != NULL) && sd->fd >= 0 && session[sd->fd] != NULL) {
		clif_getareachar_pet(sd, pd);
	}

	return 0;
}

// npc walking [Valaris]
int clif_npcinsight(struct block_list *bl, va_list ap)
{
	struct map_session_data *sd;
	struct npc_data *nd;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);

	nd = va_arg(ap, struct npc_data*);
	if (bl->type == BL_PC && ((sd = (struct map_session_data*)bl) != NULL) && sd->fd >= 0 && session[sd->fd] != NULL) {
		clif_getareachar_npc(sd, nd);
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_skillinfo(struct map_session_data *sd, int skillid, int type, int range)
{
	int id;

	nullpo_retr(0, sd);

	if ((id = sd->status.skill[skillid].id) <= 0)
		return 0;

	WPACKETW( 0) = 0x147;
	WPACKETW( 2) = id;
	if (type < 0)
		WPACKETW(4) = skill_get_inf(id);
	else
		WPACKETW(4) = type;
	WPACKETW( 6) = 0;
	WPACKETW( 8) = sd->status.skill[skillid].lv;
	WPACKETW(10) = skill_get_sp(id,sd->status.skill[skillid].lv);
	if (range < 0) {
		range = skill_get_range(id, sd->status.skill[skillid].lv);
		if (range < 0)
			range = status_get_range(&sd->bl) - (range + 1);
		WPACKETW(12) = range;
	} else
		WPACKETW(12) = range;
	memset(WPACKETP(14), 0, 24);
	if (sd->GM_level >= battle_config.gm_allskill)
		WPACKETB(38) = (sd->status.skill[skillid].flag == 0 && sd->status.skill[skillid].lv < skill_get_max(id)) ? 1 : 0;
	else if ((sd->GM_level >= battle_config.gm_all_skill_job && skill_tree_get_max(id, sd->status.class) > 0) ||
	         !(skill_get_inf2(id) & 0x01) || battle_config.quest_skill_learn)
		//WPACKETB(38) = (sd->status.skill[skillid].lv < skill_get_max(id) && sd->status.skill[skillid].flag == 0) ? 1 : 0;
		WPACKETB(38) = (sd->status.skill[skillid].flag == 0 && sd->status.skill[skillid].lv < skill_tree_get_max(id, sd->status.class)) ? 1 : 0;
	else
		WPACKETB(38) = 0;
	SENDPACKET(sd->fd, packet_len_table[0x147]);

	return 0;
}

/*==========================================
 * スキルリストを送信する
 *------------------------------------------
 */
int clif_skillinfoblock(struct map_session_data *sd)
{
	int i, len = 4, id, range;

	nullpo_retr(0, sd);

	WPACKETW(0) = 0x10f;
	for (i = 0; i < MAX_SKILL; i++) {
		if ((id = sd->status.skill[i].id) != 0) {
			WPACKETW(len     ) = id;
			WPACKETW(len +  2) = skill_get_inf(id);
			WPACKETW(len +  4) = 0;
			WPACKETW(len +  6) = sd->status.skill[i].lv;
			WPACKETW(len +  8) = skill_get_sp(id, sd->status.skill[i].lv);
			range = skill_get_range(id, sd->status.skill[i].lv);
			if (range < 0)
				range = status_get_range(&sd->bl) - (range + 1);
			WPACKETW(len + 10)= range;
			memset(WPACKETP(len+12), 0, 24);
			if (sd->GM_level >= battle_config.gm_allskill)
				WPACKETB(len + 36) = (sd->status.skill[i].lv < skill_get_max(id)) ? 1 : 0; // flag: 0 (normal), 1 (only card), 2-12 (card and skill (skill level +2)), 13 (cloneskill)
			else if ((sd->GM_level >= battle_config.gm_all_skill_job && skill_tree_get_max(id, sd->status.class) > 0) ||
			         !(skill_get_inf2(id) & 0x01) || battle_config.quest_skill_learn)
				//WPACKETB(len+36) = (sd->status.skill[i].lv < skill_get_max(id) && sd->status.skill[i].flag == 0)? 1:0;
				WPACKETB(len + 36) = (sd->status.skill[i].flag == 0 && sd->status.skill[i].lv < skill_tree_get_max(id, sd->status.class)) ? 1 : 0; // flag: 0 (normal), 1 (only card), 2-12 (card and skill (skill level +2)), 13 (cloneskill)
			else
				WPACKETB(len + 36) = 0;
			len += 37;
		}
	}
	WPACKETW(2) = len;
	SENDPACKET(sd->fd, len);

	return 0;
}

/*==========================================
 * スキル割り振り通知
 *------------------------------------------
 */
int clif_skillup(struct map_session_data *sd, int skill_num)
{
	int range;

	nullpo_retr(0, sd);

	WPACKETW( 0) = 0x10e;
	WPACKETW( 2) = skill_num;
	WPACKETW( 4) = sd->status.skill[skill_num].lv;
	WPACKETW( 6) = skill_get_sp(skill_num, sd->status.skill[skill_num].lv);
	range = skill_get_range(skill_num, sd->status.skill[skill_num].lv);
	if (range < 0)
		range = status_get_range(&sd->bl) - (range + 1);
	WPACKETW( 8) = range;
	//WPACKETB(10) = (sd->status.skill[skill_num].lv < skill_get_max(sd->status.skill[skill_num].id)) ? 1 : 0;
	WPACKETB(10) = (sd->status.skill[skill_num].lv < skill_tree_get_max(sd->status.skill[skill_num].id, sd->status.class)) ? 1 : 0;
	SENDPACKET(sd->fd, packet_len_table[0x10e]);

	return 0;
}

/*==========================================
 * スキル詠唱エフェクトを送信する
 *------------------------------------------
 */
int clif_skillcasting(struct block_list* bl,
	int src_id, int dst_id, int dst_x, int dst_y, int skill_num, int casttime)
{
	WPACKETW( 0) = 0x13e;
	WPACKETL( 2) = src_id;
	WPACKETL( 6) = dst_id;
	WPACKETW(10) = dst_x;
	WPACKETW(12) = dst_y;
	WPACKETW(14) = skill_num;//魔法詠唱スキル
	WPACKETL(16) = skill_get_pl(skill_num);//属性
	WPACKETL(20) = casttime;//skill詠唱時間
	clif_send(packet_len_table[0x13e], bl, AREA);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_skillcastcancel(struct block_list* bl)
{
	nullpo_retr(0, bl);

	WPACKETW(0) = 0x1b9;
	WPACKETL(2) = bl->id;
	clif_send(packet_len_table[0x1b9], bl, AREA);

	return 0;
}

/*==========================================
 * スキル詠唱失敗
 * skill 1, type 0:
 *   btype: 0: You haven't learned enough skills to Trade.
 *   btype: 1: You haven't learned enough skills to use Emotion icons.
 *   btype: 2: You haven't learned enough skills to Sit.
 *   btype: 3: You haven't learned enough skills to create a chat room.
 *   btype: 4: You haven't learned enough skills to Party.
 *   btype: 5: You haven't learned enough skills to Shout.
 *   btype: 6: You haven't learned enough skills for Pking.
 *   btype: 7: You haven't learned enough skills for alignning.
 * skill 1, type 1:
 *   btype: any: not enough SP (red)
 * skill 1, type 2:
 *   btype: any: not enough HP (red)
 * skill 1, type 3:
 *   btype: any: You don't have enough ingredients. (red)
 * skill 1, type 4:
 *   btype: any: There is a delay after using a skill. (red)
 * skill 1, type 5:
 *   btype: any: Skill has failed because you do not have enough Zeny. (red)
 * skill 1, type 6:
 *   btype: any: The skill can not be used with this weapon. (red)
 * skill 1, type 7:
 *   btype: any: Red Gemstone needed. (red)
 * skill 1, type 8:
 *   btype: any: Blue Gemstone needed. (red)
 * skill 1, type 9:
 *   btype: any: You can't use this Skill because you are over your Weight Limit. (red)
 * skill 1, type 10:
 *   btype: any: Skill has failed. (red)
 * skill 1, type 11 or more: no message
 *------------------------------------------
 */
int clif_skill_fail(struct map_session_data *sd, int skill_id, int type, int btype)
{
	nullpo_retr(0, sd);

	// reset all variables
	sd->skillx = sd->skilly = -1;
	sd->skillid = sd->skilllv = -1;
	sd->skillitem = sd->skillitemlv = -1;

	if (type == 0x4 && battle_config.display_delay_skill_fail == 0)
		return 0;

	WPACKETW(0) = 0x110;
	WPACKETW(2) = skill_id;
	WPACKETW(4) = btype;
	WPACKETW(6) = 0;
	WPACKETB(8) = 0;
	WPACKETB(9) = type;
	SENDPACKET(sd->fd, packet_len_table[0x110]);

	return 0;
}

/*==========================================
 * スキル攻撃エフェクト＆ダメージ
 *------------------------------------------
 */
int clif_skill_damage(struct block_list *src, struct block_list *dst,
	unsigned int tick, int sdelay, int ddelay, int damage, int div_, int skill_id, int skill_lv, int type)
{
	struct status_change *sc_data;

	nullpo_retr(0, src);
	nullpo_retr(0, dst);

	sc_data = status_get_sc_data(dst);

	if (type != 5 && dst->type == BL_PC && !map[dst->m].flag.gvg && ((struct map_session_data *)dst)->special_state.infinite_endure)
		type = 9;
	if (sc_data) {
		if (type != 5 && !map[dst->m].flag.gvg && (sc_data[SC_ENDURE].timer != -1 ||
			sc_data[SC_CONCENTRATION].timer != -1 || sc_data[SC_BERSERK].timer != -1))
			type = 9;
		if (sc_data[SC_HALLUCINATION].timer != -1 && damage > 0)
			damage = damage * (5+sc_data[SC_HALLUCINATION].val1) + rand() % 100;
	}

	WPACKETW( 0) = 0x1de;
	WPACKETW( 2) = skill_id;
	WPACKETL( 4) = src->id;
	WPACKETL( 8) = dst->id;
	WPACKETL(12) = tick;
	WPACKETL(16) = sdelay;
	WPACKETL(20) = ddelay;
	WPACKETL(24) = damage;
	WPACKETW(28) = skill_lv;
	WPACKETW(30) = div_;
	WPACKETB(32) = (type > 0) ? type : skill_get_hit(skill_id);
	clif_send(packet_len_table[0x1de], src, AREA);

	return 0;
}

/*==========================================
 * 吹き飛ばしスキル攻撃エフェクト＆ダメージ
 *------------------------------------------
 */
int clif_skill_damage2(struct block_list *src, struct block_list *dst,
	unsigned int tick, int sdelay, int ddelay, int damage, int div_, int skill_id, int skill_lv, int type)
{
	struct status_change *sc_data;

	nullpo_retr(0, src);
	nullpo_retr(0, dst);

	sc_data = status_get_sc_data(dst);

	if (type != 5 && dst->type == BL_PC && !map[dst->m].flag.gvg && ((struct map_session_data *)dst)->special_state.infinite_endure)
		type = 9;
	if (sc_data) {
		if (type != 5 && !map[dst->m].flag.gvg && (sc_data[SC_ENDURE].timer != -1 ||
			sc_data[SC_CONCENTRATION].timer != -1 || sc_data[SC_BERSERK].timer != -1))
			type = 9;
		if (sc_data[SC_HALLUCINATION].timer != -1 && damage > 0)
			damage = damage * (5+sc_data[SC_HALLUCINATION].val1) + rand() % 100;
	}

	WPACKETW( 0) = 0x115;
	WPACKETW( 2) = skill_id;
	WPACKETL( 4) = src->id;
	WPACKETL( 8) = dst->id;
	WPACKETL(12) = tick;
	WPACKETL(16) = sdelay;
	WPACKETL(20) = ddelay;
	WPACKETW(24) = dst->x;
	WPACKETW(26) = dst->y;
	WPACKETW(28) = damage;
	WPACKETW(30) = skill_lv;
	WPACKETW(32) = div_;
	WPACKETB(34) = (type > 0) ? type : skill_get_hit(skill_id);
	clif_send(packet_len_table[0x115], src, AREA);

	return 0;
}

/*==========================================
 * 支援/回復スキルエフェクト
 *------------------------------------------
 */
int clif_skill_nodamage(struct block_list *src, struct block_list *dst,
	int skill_id, int heal, int fail)
{
	nullpo_retr(0, src);
	nullpo_retr(0, dst);

	WPACKETW( 0) = 0x11a;
	WPACKETW( 2) = skill_id;
	WPACKETW( 4) = (heal > 0x7fff) ? 0x7fff : heal;
	WPACKETL( 6) = dst->id;
	WPACKETL(10) = src->id;
	WPACKETB(14) = fail;
	clif_send(packet_len_table[0x11a], src, AREA);

	return 0;
}

/*==========================================
 * 場所スキルエフェクト
 *------------------------------------------
 */
int clif_skill_poseffect(struct block_list *src, int skill_id, int val, int x, int y, int tick)
{
	nullpo_retr(0, src);

	WPACKETW( 0) = 0x117;
	WPACKETW( 2) = skill_id;
	WPACKETL( 4) = src->id;
	WPACKETW( 8) = val;
	WPACKETW(10) = x;
	WPACKETW(12) = y;
	WPACKETL(14) = tick;
	clif_send(packet_len_table[0x117], src, AREA);

	return 0;
}

/*==========================================
 * 場所スキルエフェクト表示
 *------------------------------------------
 */
int clif_skill_setunit(struct skill_unit *unit)
{
	//struct block_list *bl;

	nullpo_retr(0, unit);

	if(unit->group->unit_id == 0xb0) { //Graffiti [Valaris]
		memset(WPACKETP(0), 0, packet_len_table[0x1c9]);
		WPACKETW( 0) = 0x1c9;
		WPACKETL( 2) = unit->bl.id;
		WPACKETL( 6) = unit->group->src_id;
		WPACKETW(10) = unit->bl.x;
		WPACKETW(12) = unit->bl.y;
		WPACKETB(14) = unit->group->unit_id;
		WPACKETB(15) = 1;
		WPACKETB(16) = 1;
		memcpy(WPACKETP(17), unit->group->valstr, 80);
	} else {
		memset(WPACKETP(0), 0, packet_len_table[0x11f]);
		WPACKETW( 0) = 0x11f;
		WPACKETL( 2) = unit->bl.id;
		WPACKETL( 6) = unit->group->src_id;
		WPACKETW(10) = unit->bl.x;
		WPACKETW(12) = unit->bl.y;
		WPACKETB(14) = unit->group->unit_id;
		WPACKETB(15) = 1;
	}

	clif_send(packet_len_table[WPACKETW(0)], &unit->bl, AREA);
	return 0;

	/* Old implementation
	memset(WPACKETP(0), 0, packet_len_table[0x1c9]);
	WPACKETW( 0) = 0x1c9;
	WPACKETL( 2) = unit->bl.id;
	WPACKETL( 6) = unit->group->src_id;
	WPACKETW(10) = unit->bl.x;
	WPACKETW(12) = unit->bl.y;
	WPACKETB(14) = unit->group->unit_id;
	WPACKETB(15) = 1;
	*/
//	if (unit->group->unit_id == 0xb0) { //グラフィティ
//		WPACKETB(15+1) = 1;
//		memcpy(WPACKETP(15+2), unit->group->valstr, 80);
//	} else {
/*
		WPACKETL(15+ 1) = 0;                	//1-4調べた限り固定
		WPACKETL(15+ 5) = 0;                	//5-8調べた限り固定
		                                    	//9-12マップごとで一定の77-80とはまた違う4バイトのかなり大きな数字
		WPACKETL(15+13) = unit->bl.y - 0x12;	//13-16ユニットのY座標-18っぽい(Y:17でFF FF FF FF)
		WPACKETL(15+17) = 0x004f37dd;       	//17-20調べた限り固定(0x1b2で0x004fdbddだった)
		WPACKETL(15+21) = 0x0012f674;       	//21-24調べた限り固定
		WPACKETL(15+25) = 0x0012f664;       	//25-28調べた限り固定
		WPACKETL(15+29) = 0x0012f654;       	//29-32調べた限り固定
		WPACKETL(15+33) = 0x77527bbc;       	//33-36調べた限り固定
		                                    	//37-39
		WPACKETB(15+40) = 0x2d;             	//40調べた限り固定
		WPACKETL(15+41) = 0;                	//41-44調べた限り0固定
		WPACKETL(15+45) = 0;                	//45-48調べた限り0固定
		WPACKETL(15+49) = 0;                	//49-52調べた限り0固定
		WPACKETL(15+53) = 0x0048d919;       	//53-56調べた限り固定(0x01b2で0x00495119だった)
		WPACKETL(15+57) = 0x0000003e;       	//57-60調べた限り固定
		WPACKETL(15+61) = 0x0012f66c;       	//61-64調べた限り固定
		                                    	//65-68
		                                    	//69-72
		if (bl) WPACKETL(15+73) = bl->y;    	//73-76術者のY座標
		WPACKETL(15+77) = unit->bl.m;       	//77-80マップIDかなぁ？かなり2バイトで足りそうな数字
		WPACKETB(15+81) = 0xaa;             	//81終端文字0xaa
//	}

	if (unit->group->unit_id == 0xb0) {
		WPACKETL(15) = 1;
		WPACKETL(16) = 1;
		memcpy(WPACKETP(17), unit->group->valstr, 80);
	}

	clif_send(packet_len_table[0x1c9], &unit->bl, AREA);

	return 0; */
}


/*==========================================
 * 場所スキルエフェクト削除
 *------------------------------------------
 */
int clif_skill_delunit(struct skill_unit *unit)
{
	nullpo_retr(0, unit);

	WPACKETW(0) = 0x120;
	WPACKETL(2) = unit->bl.id;
	clif_send(packet_len_table[0x120], &unit->bl, AREA);

	return 0;
}

/*==========================================
 * ワープ場所選択
 *------------------------------------------
 */
void clif_skill_warppoint(struct map_session_data *sd, unsigned short skill_num,
	const char *map1, const char *map2, const char *map3, const char *map4) {
	nullpo_retv(sd);

	WPACKETW(0) = 0x11c;
	WPACKETW(2) = skill_num;
	strncpy(WPACKETP( 4), map1, 16);
	strncpy(WPACKETP(20), map2, 16);
	strncpy(WPACKETP(36), map3, 16);
	strncpy(WPACKETP(52), map4, 16);
	SENDPACKET(sd->fd, packet_len_table[0x11c]);

	return;
}

/*==========================================
 * メモ応答
 *------------------------------------------
 */
void clif_skill_memo(struct map_session_data *sd, unsigned char flag) { // 00: success to take memo., 01: insuffisant skill level., 02: You don't know warp skill.
//	nullpo_retv(sd); // checked before to call function

	WPACKETW(0) = 0x11e;
	WPACKETB(2) = flag;
	SENDPACKET(sd->fd, packet_len_table[0x11e]);

	return;
}

int clif_skill_teleportmessage(struct map_session_data *sd, int flag)
{
	nullpo_retr(0, sd);

	WPACKETW(0) = 0x189;
	WPACKETW(2) = flag;
	SENDPACKET(sd->fd, packet_len_table[0x189]);

	return 0;
}

/*==========================================
 * モンスター情報
 *------------------------------------------
 */
int clif_skill_estimation(struct map_session_data *sd, struct block_list *bl)
{
	struct mob_data *md;
	int i;

//nullpo_retr(0, sd); // checked before to call function
	nullpo_retr(0, bl);

	if (bl->type != BL_MOB)
		return 0;
	if ((md = (struct mob_data *)bl) == NULL)
		return 0;

	WPACKETW( 0) = 0x18c;
	WPACKETW( 2) = mob_get_viewclass(md->class);
	WPACKETW( 4) = md->level;
	WPACKETW( 6) = mob_db[md->class].size;
	WPACKETL( 8) = md->hp;
	WPACKETW(12) = status_get_def2(&md->bl);
	WPACKETW(14) = mob_db[md->class].race;
	WPACKETW(16) = status_get_mdef2(&md->bl) - (mob_db[md->class].vit >> 1);
	WPACKETW(18) = status_get_elem_type(&md->bl);
	for(i = 0; i < 9; i++)
		WPACKETB(20+i) = battle_attr_fix(100, i+1, md->def_ele);

	if (sd->status.party_id > 0)
		clif_send(packet_len_table[0x18c], &sd->bl, PARTY_AREA);
	else {
		SENDPACKET(sd->fd, packet_len_table[0x18c]);
	}

	return 0;
}

/*==========================================
 * アイテム合成可能リスト
 *------------------------------------------
 */
int clif_skill_produce_mix_list(struct map_session_data *sd, int trigger)
{
	int i, c, view;

	nullpo_retr(0, sd);

	WPACKETW(0) = 0x18d;
	c = 0;
	for(i = 0; i < MAX_SKILL_PRODUCE_DB; i++) {
		if (skill_can_produce_mix(sd, skill_produce_db[i].nameid, trigger, 1)) {
			if ((view = itemdb_viewid(skill_produce_db[i].nameid)) > 0)
				WPACKETW(c*8+ 4) = view;
			else
				WPACKETW(c*8+ 4) = skill_produce_db[i].nameid;
			WPACKETW(c*8+ 6) = 0x0012;
			WPACKETL(c*8+ 8) = sd->status.char_id;
			c++;
		}
	}
	if (c > 0) {
		sd->state.produce_flag = 1;
		WPACKETW(2) = c * 8 + 8;
		SENDPACKET(sd->fd, WPACKETW(2));
	}

	return 0;
}

#ifdef USE_SQL //TXT version is still in dev
/*==========================================
 * Sends a status change packet to the player only, used for loading status changes. [Skotlex]
 *------------------------------------------
 */
int clif_status_load(struct map_session_data *sd, int type) {
	if(!sd) 
		return 0;

	if(type == ICO_BLANK)  //It shows nothing on the client...
		return 0;

	WPACKETW( 0) = 0x0196;
	WPACKETW( 2) = type;
	WPACKETL( 4) = sd->bl.id;
	WPACKETB( 8) = 1; //Status start
	SENDPACKET(sd->fd, packet_len_table[0x196]);
	return 0;
}
#endif

/*==========================================
 * 状態異常アイコン/メッセージ表示
 *------------------------------------------
 */
int clif_status_change(struct block_list *bl, int type, int flag)
{
	nullpo_retr(0, bl);

	WPACKETW(0) = 0x0196;
	WPACKETW(2) = type;
	WPACKETL(4) = bl->id;
	WPACKETB(8) = flag;
	clif_send(packet_len_table[0x196], bl, AREA);

	return 0;
}

/*==========================================
 * Send message (modified by [Yor])
 *------------------------------------------
 */
int clif_displaymessage(const int fd, char* mes)
{
	// Console
	if (fd == 0)
		printf(CL_DARK_CYAN "Console: " CL_RESET CL_BOLD "%s" CL_RESET "\n", mes);
	else {
		int len_mes = strlen(mes);

		if (len_mes > 0) { // don't send a void message (it's not displaying on the client chat). @help can send void line.
			WPACKETW(0) = 0x8e;
			WPACKETW(2) = len_mes + 5; // 4 + len + NULL teminate
			strncpy(WPACKETP(4), mes, len_mes);
			WPACKETB(len_mes + 5 - 1) = 0; // NULL
			SENDPACKET(fd, len_mes + 5);
		}
	}

	return 0;
}

/*==========================================
 * 天の声を送信する
 *------------------------------------------
 */
int clif_GMmessage(struct block_list *bl, char* mes, short len, int flag) { // 0x3000/0x3800 <packet_len>.w <message>.?B
	unsigned char lp;

	if (len > 0 && mes[len - 1] == '\0') // if len including NULL, count len corectly
		len--;
	if (len < 1)
		return 0;

	lp = (flag & 0x10) ? 8 : 4;

	WPACKETW(0) = 0x9a;
	WPACKETW(2) = len + lp + 1; // + 1 for NULL
	WPACKETL(4) = 0x65756c62; // eulb
	strncpy(WPACKETP(lp), mes, len);
	WPACKETB(len + lp) = 0; // be sure to have NULL
	flag &= 0x07;
	clif_send(WPACKETW(2), bl,
	          (flag == 1) ? ALL_SAMEMAP :
	          (flag == 2) ? AREA :
	          (flag == 3) ? SELF :
	          ALL_CLIENT);

	return 0;
}

/*==========================================
 * グローバルメッセージ
 *------------------------------------------
 */
void clif_GlobalMessage(struct block_list *bl, char *message) {
	int len;

	nullpo_retv(bl);

	if (!message)
		return;

	len = strlen(message) + 1; // + NULL

	WPACKETW(0) = 0x8d;
	WPACKETW(2) = len + 8;
	WPACKETL(4) = bl->id;
	strncpy(WPACKETP(8), message, len);
	clif_send(len + 8, bl, AREA_CHAT_WOC);

	return;
}

/*==========================================
 * Does an announce message in the given color. 
 *------------------------------------------
 */
void clif_announce(char* mes, unsigned int color, unsigned int flag) {
	WPACKETW( 0) = 0x1c3;
	WPACKETW( 2) = 16 + strlen(mes) + 1;
	WPACKETL( 4) = color;
	WPACKETW( 8) = 0x190; // Font style? Type?
	WPACKETW(10) = 0x0c; // 12? Font size?
	WPACKETL(12) = 0; // Unknown!
	strcpy(WPACKETP(16), mes);

	flag &= 0x07;
	clif_send(WPACKETW(2), NULL,
	          (flag == 1) ? ALL_SAMEMAP :
	          (flag == 2) ? AREA :
	          (flag == 3) ? SELF :
	          ALL_CLIENT);

	return;
}

/*==========================================
 * HPSP回復エフェクトを送信する
 *------------------------------------------
 */
int clif_heal(int fd, int type, int val) {
	WPACKETW(0) = 0x13d;
	WPACKETW(2) = type;
	WPACKETW(4) = val;
	SENDPACKET(fd, packet_len_table[0x13d]);

	return 0;
}

/*==========================================
 * 復活する
 *------------------------------------------
 */
int clif_resurrection(struct block_list *bl, int type)
{
	nullpo_retr(0, bl);

	if (bl->type == BL_PC) { // disguises
		struct map_session_data *sd = ((struct map_session_data *)bl);
		if (sd && sd->disguise > 23 && sd->disguise < 4001)
			clif_spawnpc(sd);
	}

	WPACKETW(0) = 0x148;
	WPACKETL(2) = bl->id;
	WPACKETW(6) = type;

	clif_send(packet_len_table[0x148], bl,type == 1 ? AREA : AREA_WOS);

	return 0;
}

/*==========================================
 * PVP実装？（仮）
 *------------------------------------------
 */
int clif_set0199(int fd, int type)
{
	WPACKETW(0) = 0x199;
	WPACKETW(2) = type;
	SENDPACKET(fd, packet_len_table[0x199]);

	return 0;
}

/*==========================================
 * PVP実装？(仮)
 *------------------------------------------
 */
int clif_pvpset(struct map_session_data *sd, int pvprank, int pvpnum, int type)
{
	nullpo_retr(0, sd);

//	if (!map[sd->bl.m].flag.pvp)
//		return 0;

	if (type == 2) {
		if (pvprank <= 0)
			pc_calc_pvprank(sd);
		WPACKETW( 0) = 0x19a;
		WPACKETL( 2) = sd->bl.id;
		WPACKETL( 6) = pvprank;
		WPACKETL(10) = pvpnum;
		SENDPACKET(sd->fd, packet_len_table[0x19a]);
	} else {
		WPACKETW(0) = 0x19a;
		WPACKETL(2) = sd->bl.id;
		if (sd->status.option & 0x46)
			WPACKETL(6) = -1;
		else
			if (pvprank <= 0)
				pc_calc_pvprank(sd);
			WPACKETL(6) = pvprank;
		WPACKETL(10) = pvpnum;
		if (!type)
			clif_send(packet_len_table[0x19a], &sd->bl, AREA);
		else
			clif_send(packet_len_table[0x19a], &sd->bl, ALL_SAMEMAP);
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_send0199(int map_id, int type)
{
	struct block_list bl;

	bl.m = map_id;
	WPACKETW(0) = 0x199;
	WPACKETW(2) = type;
	clif_send(packet_len_table[0x199], &bl, ALL_SAMEMAP);

	return 0;
}

/*==========================================
 * 精錬エフェクトを送信する
 *------------------------------------------
 */
int clif_refine(int fd, struct map_session_data *sd, int fail, int idx, int val)
{
	WPACKETW(0) = 0x188;
	WPACKETW(2) = fail;
	WPACKETW(4) = idx + 2;
	WPACKETW(6) = val;
	SENDPACKET(fd, packet_len_table[0x188]);

	return 0;
}

/*==========================================
 * Wisp/page is transmitted to the destination player
 *------------------------------------------
 */
int clif_wis_message(int fd, char *nick, char *mes, int mes_len) // R 0097 <len>.w <nick>.24B <message>.?B
{
//	printf("clif_wis_message(fd:%d, nick:%s, mes:%s).\n", fd, nick, mes);

	WPACKETW(0) = 0x97;
	WPACKETW(2) = mes_len + 28;
	strncpy(WPACKETP( 4), nick, 24);
	strncpy(WPACKETP(28), mes, mes_len);
	SENDPACKET(fd, mes_len + 28);

	return 0;
}

/*==========================================
 * The transmission result of Wisp/page is transmitted to the source player
 *------------------------------------------
 */
int clif_wis_end(int fd, int flag) { // R 0098 <type>.B: 0: success to send wisper, 1: target character is not loged in?, 2: ignored by target
	WPACKETW(0) = 0x98;
	WPACKETW(2) = flag;
	SENDPACKET(fd, packet_len_table[0x98]);

	return 0;
}

/*==========================================
 * キャラID名前引き結果を送信する
 *------------------------------------------
 */
void clif_solved_charname(struct map_session_data *sd, int char_id) {
	char *p = map_charid2nick(char_id);

//	nullpo_retv(sd); // checked before to call function

	if (p != NULL) {
		WPACKETW(0) = 0x194;
		WPACKETL(2) = char_id;
		strncpy(WPACKETP(6), p, 24);
		SENDPACKET(sd->fd, packet_len_table[0x194]);
	} else {
		map_reqchariddb(sd, char_id);
		chrif_searchcharid(char_id);
	}

	return;
}

/*==========================================
 * カードの挿入可能リストを返す
 *------------------------------------------
 */
void clif_use_card(struct map_session_data *sd, short idx) {
	int i, j, c;

//	nullpo_retv(sd); // checked before to call function

	if (idx < 0 || idx >= MAX_INVENTORY)
		return;

	if (sd->inventory_data[idx]) {
		int ep = sd->inventory_data[idx]->equip;
		WPACKETW(0) = 0x017b;
		
		c = 0;
		for(i = 0; i < MAX_INVENTORY; i++) {
			if (sd->inventory_data[i] == NULL)
				continue;
			if (sd->inventory_data[i]->type != 4 && sd->inventory_data[i]->type != 5) // 武器防具じゃない
				continue;
			if (sd->status.inventory[i].card[0] == 0x00ff) // 製造武器
				continue;
			if (sd->status.inventory[i].card[0] == (short)0xff00 || sd->status.inventory[i].card[0] == 0x00fe)
				continue;
			if (sd->status.inventory[i].identify == 0) // 未鑑定
				continue;

			if ((sd->inventory_data[i]->equip&ep) == 0) // 装備個所が違う
				continue;
			if (sd->inventory_data[i]->type == 4 && ep == 32) // 盾カードと両手武器
				continue;

			for (j = 0; j < sd->inventory_data[i]->slot; j++) {
				if (sd->status.inventory[i].card[j] == 0)
					break;
			}
			if (j == sd->inventory_data[i]->slot) // すでにカードが一杯
				continue;

			WPACKETW(4+c*2) = i + 2;
			c++;
		}
		//Only send if found equipment to compound
		if(c > 0) {
			WPACKETW(2) = 4 + c * 2;
			SENDPACKET(sd->fd, WPACKETW(2));
		}
	}

	return;
}

/*==========================================
 * カードの挿入終了
 *------------------------------------------
 */
void clif_insert_card(struct map_session_data *sd, short idx_equip, short idx_card, unsigned char flag) { // flag: 1=fail, 0:success
//	nullpo_retv(sd); // checked before to call function

	WPACKETW(0) = 0x17d;
	WPACKETW(2) = idx_equip + 2;
	WPACKETW(4) = idx_card + 2;
	WPACKETB(6) = flag; // flag: 1=fail, 0:success
	SENDPACKET(sd->fd, packet_len_table[0x17d]);

	return;
}

/*==========================================
 * 鑑定可能アイテムリスト送信
 *------------------------------------------
 */
int clif_item_identify_list(struct map_session_data *sd)
{
	int i, c;

	nullpo_retr(0, sd);

	c = 0;
	for(i = 0; i < MAX_INVENTORY; i++) {
		if (sd->status.inventory[i].nameid > 0 && sd->status.inventory[i].identify != 1) {
			WPACKETW(c * 2 + 4) = i + 2;
			c++;
		}
	}
	if (c > 0) {
		WPACKETW(0) = 0x177;
		WPACKETW(2) = c * 2 + 4;
		SENDPACKET(sd->fd, WPACKETW(2));
	}

	return 0;
}

/*==========================================
 * 鑑定結果
 *------------------------------------------
 */
void clif_item_identified(struct map_session_data *sd, short idx, unsigned char flag) { // flag: 0: success, 1: fail
//	nullpo_retv(sd); // checked before to call function

	WPACKETW(0) = 0x179;
	WPACKETW(2) = idx + 2;
	WPACKETB(4) = flag; // flag: 0: success, 1: fail
	SENDPACKET(sd->fd, packet_len_table[0x179]);

	return;
}

/*==========================================
 * 修理可能アイテムリスト送信
 * ※実際のパケットがわからないので動作しません
 *------------------------------------------
 */
void clif_item_repair_list(struct map_session_data *sd)
{
  //nullpo_retv(sd); //checked before to call function
 	int i, c;
 	c = 0;

 	for(i = 0; i < MAX_INVENTORY; i++) {
		if (sd->status.inventory[i].nameid > 0 && sd->status.inventory[i].attribute == 1) {
			WPACKETW(c * 2 + 4) = i + 2;
 			c++;
 		}
 	}
	if (c > 0) {
		WPACKETW(0) = 0x177; // temporarily use same packet as clif_item_identify
		WPACKETW(2) = c * 2 + 4;
		SENDPACKET(sd->fd, WPACKETW(2));
	}
   
	return;
}

/*==========================================
 * Weapon Refining [Celest]
 *------------------------------------------
 */
int clif_item_refine_list(struct map_session_data *sd)
{
	int i, c;
	nullpo_retr(0, sd);

	c = 0;
	for(i = 0; i < MAX_INVENTORY; i++) {
		if (sd->status.inventory[i].nameid > 0 && itemdb_type(sd->status.inventory[i].nameid) == 4) {
			WPACKETW(c * 2 + 4) = i + 2;
			c++;
		}
	}
	if (c > 0) {
		WPACKETW(0) = 0x177; // temporarily use same packet as clif_item_identify
		WPACKETW(2) = c * 2 + 4;
		SENDPACKET(sd->fd, WPACKETW(2));
	}

	return 0;
}

/*==========================================
 * アイテムによる一時的なスキル効果
 *------------------------------------------
 */
int clif_item_skill(struct map_session_data *sd, int skillid, int skilllv, const char *name)
{
	int range;

	nullpo_retr(0, sd);

	WPACKETW( 0) = 0x147;
	WPACKETW( 2) = skillid;
	WPACKETW( 4) = skill_get_inf(skillid);
	WPACKETW( 6) = 0;
	WPACKETW( 8) = skilllv;
	WPACKETW(10) = skill_get_sp(skillid, skilllv);
	range = skill_get_range(skillid, skilllv);
	if (range < 0)
		range = status_get_range(&sd->bl) - (range + 1);
	WPACKETW(12) = range;
	strncpy(WPACKETP(14), name, 24);
	WPACKETB(38) = 0;
	SENDPACKET(sd->fd, packet_len_table[0x147]);

	return 0;
}

/*==========================================
 * カートにアイテム追加
 *------------------------------------------
 */
int clif_cart_additem(struct map_session_data *sd, int n, int amount)
{
	int view, j;

	WPACKETW( 0) = 0x124;
	WPACKETW( 2) = n + 2;
	WPACKETL( 4) = amount;
	if ((view = itemdb_viewid(sd->status.cart[n].nameid)) > 0)
		WPACKETW( 8) = view;
	else
		WPACKETW( 8) = sd->status.cart[n].nameid;
	WPACKETB(10) = sd->status.cart[n].identify;
	WPACKETB(11) = sd->status.cart[n].attribute;
	WPACKETB(12) = sd->status.cart[n].refine;
	if (sd->status.cart[n].card[0] == 0x00ff || sd->status.cart[n].card[0] == 0x00fe || sd->status.cart[n].card[0] == (short)0xff00) {
		WPACKETW(13) = sd->status.cart[n].card[0];
		WPACKETW(15) = sd->status.cart[n].card[1];
		WPACKETW(17) = sd->status.cart[n].card[2];
		WPACKETW(19) = sd->status.cart[n].card[3];
	} else {
		if (sd->status.cart[n].card[0] > 0 && (j=itemdb_viewid(sd->status.cart[n].card[0])) > 0)
			WPACKETW(13) = j;
		else
			WPACKETW(13) = sd->status.cart[n].card[0];
		if (sd->status.cart[n].card[1] > 0 && (j=itemdb_viewid(sd->status.cart[n].card[1])) > 0)
			WPACKETW(15) = j;
		else
			WPACKETW(15) = sd->status.cart[n].card[1];
		if (sd->status.cart[n].card[2] > 0 && (j=itemdb_viewid(sd->status.cart[n].card[2])) > 0)
			WPACKETW(17) = j;
		else
			WPACKETW(17) = sd->status.cart[n].card[2];
		if (sd->status.cart[n].card[3] > 0 && (j=itemdb_viewid(sd->status.cart[n].card[3])) > 0)
			WPACKETW(19) = j;
		else
			WPACKETW(19) = sd->status.cart[n].card[3];
	}
	SENDPACKET(sd->fd, packet_len_table[0x124]);

	return 0;
}

int clif_cart_addequipitem(struct map_session_data *sd, int n, int amount)
{
	struct item_data *id;
	int j;

	id = itemdb_search(sd->status.cart[n].nameid);

	// if equip item, use packet of equiplist (to display all information)
	WPACKETW( 0) = 0x122;
	WPACKETW( 2) = 24;
	WPACKETW( 4) = n + 2;
	if (id->view_id > 0)
		WPACKETW( 6) = id->view_id;
	else
		WPACKETW( 6) = sd->status.cart[n].nameid;
	WPACKETB( 8) = id->type;
	WPACKETB( 9) = sd->status.cart[n].identify;
	WPACKETW(10) = id->equip;
	WPACKETW(12) = sd->status.cart[n].equip;
	WPACKETB(14) = sd->status.cart[n].attribute;
	WPACKETB(15) = sd->status.cart[n].refine;
	if (sd->status.cart[n].card[0] == 0x00ff || sd->status.cart[n].card[0] == 0x00fe || sd->status.cart[n].card[0] == (short)0xff00) {
		WPACKETW(16) = sd->status.cart[n].card[0];
		WPACKETW(18) = sd->status.cart[n].card[1];
		WPACKETW(20) = sd->status.cart[n].card[2];
		WPACKETW(22) = sd->status.cart[n].card[3];
	} else {
		if (sd->status.cart[n].card[0] > 0 && (j = itemdb_viewid(sd->status.cart[n].card[0])) > 0)
			WPACKETW(16) = j;
		else
			WPACKETW(16) = sd->status.cart[n].card[0];
		if (sd->status.cart[n].card[1] > 0 && (j = itemdb_viewid(sd->status.cart[n].card[1])) > 0)
			WPACKETW(18) = j;
		else
			WPACKETW(18) = sd->status.cart[n].card[1];
		if (sd->status.cart[n].card[2] > 0 && (j = itemdb_viewid(sd->status.cart[n].card[2])) > 0)
			WPACKETW(20) = j;
		else
			WPACKETW(20) = sd->status.cart[n].card[2];
		if (sd->status.cart[n].card[3] > 0 && (j = itemdb_viewid(sd->status.cart[n].card[3])) > 0)
			WPACKETW(22) = j;
		else
			WPACKETW(22) = sd->status.cart[n].card[3];
	}
	SENDPACKET(sd->fd, 4 + 20);

	return 0;
}

/*==========================================
 * カートからアイテム削除
 *------------------------------------------
 */
int clif_cart_delitem(struct map_session_data *sd, int n, int amount)
{
	WPACKETW(0) = 0x125;
	WPACKETW(2) = n + 2;
	WPACKETL(4) = amount;
	SENDPACKET(sd->fd, packet_len_table[0x125]);

	return 0;
}

/*==========================================
 * カートのアイテムリスト
 *------------------------------------------
 */
int clif_cart_itemlist(struct map_session_data *sd)
{
	struct item_data *id;
	int i, n;

	nullpo_retr(0, sd);

	n = 0;
	for(i = 0; i < MAX_CART; i++) {
		if (sd->status.cart[i].nameid <= 0)
			continue;
		id = itemdb_search(sd->status.cart[i].nameid);
		if (itemdb_isequip2(id))
			continue;
		WPACKETW(n*18+ 4) = i + 2;
		if (id->view_id > 0)
			WPACKETW(n*18+ 6) = id->view_id;
		else
			WPACKETW(n*18+ 6) = sd->status.cart[i].nameid;
		WPACKETB(n*18+ 8) = id->type;
		WPACKETB(n*18+ 9) = sd->status.cart[i].identify;
		WPACKETW(n*18+10) = sd->status.cart[i].amount;
		WPACKETW(n*18+12) = 0;
		WPACKETW(n*18+14) = sd->status.cart[i].card[0];
		WPACKETW(n*18+16) = sd->status.cart[i].card[1];
		WPACKETW(n*18+18) = sd->status.cart[i].card[2];
		WPACKETW(n*18+20) = sd->status.cart[i].card[3];
		n++;
	}
	if (n) {
		WPACKETW(0) = 0x1ef;
		WPACKETW(2) = 4 + n * 18;
		SENDPACKET(sd->fd, WPACKETW(2));
	}

	return 0;
}

/*==========================================
 * カートの装備品リスト
 *------------------------------------------
 */
int clif_cart_equiplist(struct map_session_data *sd)
{
	struct item_data *id;
	int i, j, n;

	nullpo_retr(0, sd);

	WPACKETW(0) = 0x122;
	n = 0;
	for(i = 0; i < MAX_CART; i++) {
		if (sd->status.cart[i].nameid <= 0)
			continue;
		id = itemdb_search(sd->status.cart[i].nameid);
		if (!itemdb_isequip2(id))
			continue;
		WPACKETW(n*20+ 4) = i + 2;
		if (id->view_id > 0)
			WPACKETW(n*20+ 6) = id->view_id;
		else
			WPACKETW(n*20+ 6) = sd->status.cart[i].nameid;
		WPACKETB(n*20+ 8) = id->type;
		WPACKETB(n*20+ 9) = sd->status.cart[i].identify;
		WPACKETW(n*20+10) = id->equip;
		WPACKETW(n*20+12) = sd->status.cart[i].equip;
		WPACKETB(n*20+14) = sd->status.cart[i].attribute;
		WPACKETB(n*20+15) = sd->status.cart[i].refine;
		if (sd->status.cart[i].card[0] == 0x00ff || sd->status.cart[i].card[0] == 0x00fe || sd->status.cart[i].card[0] == (short)0xff00) {
			WPACKETW(n*20+16) = sd->status.cart[i].card[0];
			WPACKETW(n*20+18) = sd->status.cart[i].card[1];
			WPACKETW(n*20+20) = sd->status.cart[i].card[2];
			WPACKETW(n*20+22) = sd->status.cart[i].card[3];
		} else {
			if (sd->status.cart[i].card[0] > 0 && (j = itemdb_viewid(sd->status.cart[i].card[0])) > 0)
				WPACKETW(n*20+16) = j;
			else
				WPACKETW(n*20+16) = sd->status.cart[i].card[0];
			if (sd->status.cart[i].card[1] > 0 && (j = itemdb_viewid(sd->status.cart[i].card[1])) > 0)
				WPACKETW(n*20+18) = j;
			else
				WPACKETW(n*20+18) = sd->status.cart[i].card[1];
			if (sd->status.cart[i].card[2] > 0 && (j = itemdb_viewid(sd->status.cart[i].card[2])) > 0)
				WPACKETW(n*20+20) = j;
			else
				WPACKETW(n*20+20) = sd->status.cart[i].card[2];
			if (sd->status.cart[i].card[3] > 0 && (j = itemdb_viewid(sd->status.cart[i].card[3])) > 0)
				WPACKETW(n*20+22) = j;
			else
				WPACKETW(n*20+22) = sd->status.cart[i].card[3];
		}
		n++;
	}
	if (n) {
		WPACKETW(2) = 4 + n * 20;
		SENDPACKET(sd->fd, WPACKETW(2));
	}

	return 0;
}

/*==========================================
 * 露店開設
 *------------------------------------------
 */
void clif_openvendingreq(struct map_session_data *sd, int num)
{
//nullpo_retv(sd); //checked before to call function

	WPACKETW(0) = 0x12d;
	WPACKETW(2) = num;
	SENDPACKET(sd->fd, packet_len_table[0x12d]);

	return;
}

/*==========================================
 * 露店看板表示
 *------------------------------------------
 */
int clif_showvendingboard(struct block_list* bl, char *message, int fd)
{
	nullpo_retr(0, bl);

	WPACKETW(0) = 0x131;
	WPACKETL(2) = bl->id;
	strncpy(WPACKETP(6), message, 80);
	if (fd)
		SENDPACKET(fd, packet_len_table[0x131]);
	else
		clif_send(packet_len_table[0x131], bl, AREA_WOS);

	return 0;
}

/*==========================================
 * 露店看板消去
 *------------------------------------------
 */
void clif_closevendingboard(struct block_list* bl, int fd) {
//	nullpo_retv(bl); // checked before to call function

	WPACKETW(0) = 0x132;
	WPACKETL(2) = bl->id;
	if (fd)
		SENDPACKET(fd, packet_len_table[0x132]);
	else
		clif_send(packet_len_table[0x132], bl, AREA_WOS);

	return;
}

/*==========================================
 * 露店アイテムリスト
 *------------------------------------------
 */
int clif_vendinglist(struct map_session_data *sd, int id, struct vending *vending)
{
	struct item_data *data;
	int i, j, n, idx;
	struct map_session_data *vsd;

	nullpo_retr(0, sd);
	nullpo_retr(0, vending);
	nullpo_retr(0, vsd = map_id2sd(id));

	n = 0;
	for(i = 0; i < vsd->vend_num; i++) {
		if (vending[i].amount <= 0)
			continue;
		WPACKETL( 8 + n * 22) = vending[i].value;
		WPACKETW(12 + n * 22) = vending[i].amount;
		WPACKETW(14 + n * 22) = (idx = vending[i].index) + 2;
		if (vsd->status.cart[idx].nameid <= 0 || vsd->status.cart[idx].amount <= 0)
			continue;
		data = itemdb_search(vsd->status.cart[idx].nameid);
		WPACKETB(16 + n * 22) = data->type;
		if (data->view_id > 0)
			WPACKETW(17 + n * 22) = data->view_id;
		else
			WPACKETW(17 + n * 22) = vsd->status.cart[idx].nameid;
		WPACKETB(19 + n * 22) = vsd->status.cart[idx].identify;
		WPACKETB(20 + n * 22) = vsd->status.cart[idx].attribute;
		WPACKETB(21 + n * 22) = vsd->status.cart[idx].refine;
		if (vsd->status.cart[idx].card[0] == 0x00ff || vsd->status.cart[idx].card[0] == 0x00fe || vsd->status.cart[idx].card[0] == (short)0xff00) {
			WPACKETW(22 + n * 22) = vsd->status.cart[idx].card[0];
			WPACKETW(24 + n * 22) = vsd->status.cart[idx].card[1];
			WPACKETW(26 + n * 22) = vsd->status.cart[idx].card[2];
			WPACKETW(28 + n * 22) = vsd->status.cart[idx].card[3];
		} else {
			if (vsd->status.cart[idx].card[0] > 0 && (j = itemdb_viewid(vsd->status.cart[idx].card[0])) > 0)
				WPACKETW(22 + n * 22) = j;
			else
				WPACKETW(22 + n * 22) = vsd->status.cart[idx].card[0];
			if (vsd->status.cart[idx].card[1] > 0 && (j = itemdb_viewid(vsd->status.cart[idx].card[1])) > 0)
				WPACKETW(24 + n * 22) = j;
			else
				WPACKETW(24 + n * 22) = vsd->status.cart[idx].card[1];
			if (vsd->status.cart[idx].card[2] > 0 && (j = itemdb_viewid(vsd->status.cart[idx].card[2])) > 0)
				WPACKETW(26 + n * 22) = j;
			else
				WPACKETW(26 + n * 22) = vsd->status.cart[idx].card[2];
			if (vsd->status.cart[idx].card[3] > 0 && (j = itemdb_viewid(vsd->status.cart[idx].card[3])) > 0)
				WPACKETW(28 + n * 22) = j;
			else
				WPACKETW(28 + n * 22) = vsd->status.cart[idx].card[3];
		}
		n++;
	}
	if (n) {
		WPACKETW(0) = 0x133;
		WPACKETW(2) = 8 + n * 22;
		WPACKETL(4) = id;
		SENDPACKET(sd->fd, WPACKETW(2));
	}

	return 0;
}

/*==========================================
 * 露店アイテム購入失敗
 *------------------------------------------
*/
int clif_buyvending(struct map_session_data *sd, int idx, int amount, int fail)
{
	nullpo_retr(0, sd);

	WPACKETW(0) = 0x135;
	WPACKETW(2) = idx + 2;
	WPACKETW(4) = amount;
	WPACKETB(6) = fail;
	SENDPACKET(sd->fd, packet_len_table[0x135]);

	return 0;
}

/*==========================================
 * 露店開設成功
 *------------------------------------------
*/
int clif_openvending(struct map_session_data *sd, int id, struct vending *vending)
{
	struct item_data *data;
	int i, j, n, idx;

	nullpo_retr(0, sd);

	n = 0;
	for(i = 0; i < sd->vend_num; i++) {
		if (sd->vend_num > 2 + pc_checkskill(sd, MC_VENDING))
			return 0;
		WPACKETL( 8 + n * 22) = vending[i].value;
		WPACKETW(12 + n * 22) = (idx = vending[i].index) + 2;
		WPACKETW(14 + n * 22) = vending[i].amount;
		if (sd->status.cart[idx].nameid <= 0 || sd->status.cart[idx].amount <= 0 || sd->status.cart[idx].identify == 0 ||
			sd->status.cart[idx].attribute == 1) // Prevent unidentified and broken items from being sold [Valaris]
			continue;
		data = itemdb_search(sd->status.cart[idx].nameid);
		WPACKETB(16 + n * 22) = data->type;
		if (data->view_id > 0)
			WPACKETW(17 + n * 22) = data->view_id;
		else
			WPACKETW(17 + n * 22) = sd->status.cart[idx].nameid;
		WPACKETB(19 + n * 22) = sd->status.cart[idx].identify;
		WPACKETB(20 + n * 22) = sd->status.cart[idx].attribute;
		WPACKETB(21 + n * 22) = sd->status.cart[idx].refine;
		if (sd->status.cart[idx].card[0] == 0x00ff || sd->status.cart[idx].card[0] == 0x00fe || sd->status.cart[idx].card[0] == (short)0xff00) {
			WPACKETW(22 + n * 22) = sd->status.cart[idx].card[0];
			WPACKETW(24 + n * 22) = sd->status.cart[idx].card[1];
			WPACKETW(26 + n * 22) = sd->status.cart[idx].card[2];
			WPACKETW(28 + n * 22) = sd->status.cart[idx].card[3];
		} else {
			if (sd->status.cart[idx].card[0] > 0 && (j = itemdb_viewid(sd->status.cart[idx].card[0])) > 0)
				WPACKETW(22 + n * 22) = j;
			else
				WPACKETW(22 + n * 22) = sd->status.cart[idx].card[0];
			if (sd->status.cart[idx].card[1] > 0 && (j = itemdb_viewid(sd->status.cart[idx].card[1])) > 0)
				WPACKETW(24 + n * 22) = j;
			else
				WPACKETW(24 + n * 22) = sd->status.cart[idx].card[1];
			if (sd->status.cart[idx].card[2] > 0 && (j = itemdb_viewid(sd->status.cart[idx].card[2])) > 0)
				WPACKETW(26 + n * 22) = j;
			else
				WPACKETW(26 + n * 22) = sd->status.cart[idx].card[2];
			if (sd->status.cart[idx].card[3] > 0 && (j = itemdb_viewid(sd->status.cart[idx].card[3])) > 0)
				WPACKETW(28 + n * 22) = j;
			else
				WPACKETW(28 + n * 22) = sd->status.cart[idx].card[3];
		}
		n++;
	}
	if (n) {
		WPACKETW(0) = 0x136;
		WPACKETW(2) = 8 + n * 22;
		WPACKETL(4) = id;
		SENDPACKET(sd->fd, WPACKETW(2));
	}

	return n;
}

/*==========================================
 * 露店アイテム販売報告
 *------------------------------------------
*/
int clif_vendingreport(struct map_session_data *sd, int idx, int amount)
{
	nullpo_retr(0, sd);

	WPACKETW(0) = 0x137;
	WPACKETW(2) = idx + 2;
	WPACKETW(4) = amount;
	SENDPACKET(sd->fd, packet_len_table[0x137]);

	return 0;
}

/*==========================================
 * パーティ作成完了
 *------------------------------------------
 */
int clif_party_created(struct map_session_data *sd, int flag)
{
	nullpo_retr(0, sd);

	WPACKETW(0) = 0xfa;
	WPACKETB(2) = flag; // 0xfa <flag>.B: 0: Party has successfully been organized, 1: That Party Name already exists., 2: The Character is already in a party.
	SENDPACKET(sd->fd, packet_len_table[0xfa]);

	return 0;
}

/*==========================================
 * パーティ情報送信
 *------------------------------------------
 */
int clif_party_info(struct party *p, int fd) {
	int i, c;
	struct map_session_data *sd = NULL;

	nullpo_retr(0, p);

	WPACKETW(0) = 0xfb;
	strncpy(WPACKETP(4), p->name, 24);
	c = 0;
	for(i = 0; i < MAX_PARTY; i++) {
		struct party_member *m = &p->member[i];
		if (m->account_id > 0) {
			if (sd == NULL)
				sd = m->sd;
			WPACKETL(28 + c * 46) = m->account_id;
			strncpy(WPACKETP(28 + c * 46 +  4), m->name, 24);
			strncpy(WPACKETP(28 + c * 46 + 28), m->map, 16);
			WPACKETB(28 + c * 46 + 44) = (m->leader) ? 0 : 1;
			WPACKETB(28 + c * 46 + 45) = (m->online) ? 0 : 1;
			c++;
		}
	}
	WPACKETW(2) = 28 + c * 46;
	if (fd >= 0) { // fdが設定されてるならそれに送る
		SENDPACKET(fd, WPACKETW(2));
		return 9;
	}
	if (sd != NULL)
		clif_send(WPACKETW(2), &sd->bl, PARTY);

	return 0;
}

/*==========================================
 * パーティ勧誘
 *------------------------------------------
 */
void clif_party_invite(struct map_session_data *sd, struct map_session_data *tsd, struct party *p) {
//	nullpo_retv(sd); // checked before to call function
//	nullpo_retv(tsd); // checked before to call function
//	nullpo_retv(p); // checked before to call function

	WPACKETW(0) = 0xfe;
	WPACKETL(2) = sd->status.account_id;
	strncpy(WPACKETP(6), p->name, 24);
	SENDPACKET(tsd->fd, packet_len_table[0xfe]);

	return;
}

/*==========================================
 * パーティ勧誘結果
 *------------------------------------------
 */
int clif_party_inviteack(struct map_session_data *sd, char *nick, int flag) {
	nullpo_retr(0, sd);

	WPACKETW( 0) = 0xfd;
	strncpy(WPACKETP(2), nick, 24);
	WPACKETB(26) = flag;
	SENDPACKET(sd->fd, packet_len_table[0xfd]);

	return 0;
}

/*==========================================
 * パーティ設定送信
 * flag & 0x001=exp変更ミス
 *        0x010=item変更ミス
 *        0x100=一人にのみ送信
 *------------------------------------------
 */
int clif_party_option(struct party *p, struct map_session_data *sd, int flag)
{
	nullpo_retr(0, p);

//	if (battle_config.etc_log)
//		printf("clif_party_option: %d %d %d\n", p->exp, p->item, flag);
	if (sd == NULL && flag == 0) {
		int i;
		for(i = 0; i < MAX_PARTY; i++)
			if ((sd = map_id2sd(p->member[i].account_id)) != NULL)
				break;
	}
	if (sd == NULL)
		return 0;

	WPACKETW(0) = 0x101;
	WPACKETW(2) = ((flag & 0x01) ? 2 : p->exp);
	WPACKETW(4) = ((flag & 0x10) ? 2 : p->item);
	if (flag == 0) {
		clif_send(packet_len_table[0x101], &sd->bl, PARTY);
	} else {
		SENDPACKET(sd->fd, packet_len_table[0x101]);
	}

	return 0;
}

/*==========================================
 * パーティ脱退（脱退前に呼ぶこと）
 *------------------------------------------
 */
int clif_party_leaved(struct party *p, struct map_session_data *sd, int account_id, char *name, int flag)
{
	int i;

	nullpo_retr(0, p);

	if ((flag & 0xf0) == 0) {
		if (sd == NULL)
			for(i = 0; i < MAX_PARTY; i++)
				if ((sd = p->member[i].sd) != NULL)
					break;
		if (sd != NULL) {
			WPACKETW( 0) = 0x105;
			WPACKETL( 2) = account_id;
			strncpy(WPACKETP(6), name, 24);
			WPACKETB(30) = flag & 0x0f;
			clif_send(packet_len_table[0x105], &sd->bl, PARTY);
		}
	} else if (sd != NULL) {
		WPACKETW( 0) = 0x105;
		WPACKETL( 2) = account_id;
		strncpy(WPACKETP(6), name, 24);
		WPACKETB(30) = flag & 0x0f;
		SENDPACKET(sd->fd, packet_len_table[0x105]);
	}

	return 0;
}

/*==========================================
 * パーティメッセージ送信
 *------------------------------------------
 */
int clif_party_message(struct party *p, int account_id, char *mes, int len)
{
	struct map_session_data *sd;
	int i;

	nullpo_retr(0, p);

//	printf("clif_party_message(p->name:%s, account_id:%d, mes:%s).\n", p->name, account_id, mes);

	for(i = 0; i < MAX_PARTY; i++) {
		if ((sd = p->member[i].sd) != NULL)
			break;
	}
	if (sd != NULL) {
		WPACKETW(0) = 0x109;
		WPACKETW(2) = len + 8;
		WPACKETL(4) = account_id;
		strncpy(WPACKETP(8), mes, len);
		clif_send(len + 8, &sd->bl, PARTY);
	}

	return 0;
}

/*==========================================
 * パーティ座標通知
 *------------------------------------------
 */
int clif_party_xy(struct party *p, struct map_session_data *sd)
{
	nullpo_retr(0, sd);

	WPACKETW(0) = 0x107;
	WPACKETL(2) = sd->status.account_id;
	WPACKETW(6) = sd->bl.x;
	WPACKETW(8) = sd->bl.y;
	clif_send(packet_len_table[0x107], &sd->bl, PARTY_SAMEMAP_WOS);
//	if (battle_config.etc_log)
//		printf("clif_party_xy %d\n", sd->status.account_id);

	return 0;
}

/*==========================================
 * パーティHP通知
 *------------------------------------------
 */
int clif_party_hp(struct party *p, struct map_session_data *sd)
{
	nullpo_retr(0, sd);

	WPACKETW(0) = 0x106;
	WPACKETL(2) = sd->status.account_id;
	WPACKETW(6) = (sd->status.hp > 0x7fff) ? 0x7fff : sd->status.hp;
	WPACKETW(8) = (sd->status.max_hp > 0x7fff) ? 0x7fff : sd->status.max_hp;
	clif_send(packet_len_table[0x106], &sd->bl, PARTY_AREA_WOS);
//	if (battle_config.etc_log)
//		printf("clif_party_hp %d\n", sd->status.account_id);

	return 0;
}

/*==========================================
 * パーティ場所移動（未使用）
 *------------------------------------------
 */
int clif_party_move(struct party *p, struct map_session_data *sd, int online)
{
	nullpo_retr(0, sd);
	nullpo_retr(0, p);

	WPACKETW( 0) = 0x104;
	WPACKETL( 2) = sd->status.account_id;
	WPACKETL( 6) = 0;
	WPACKETW(10) = sd->bl.x;
	WPACKETW(12) = sd->bl.y;
	WPACKETB(14) = !online;
	strncpy(WPACKETP(15), p->name, 24);
	strncpy(WPACKETP(39), sd->status.name, 24);
	strncpy(WPACKETP(63), map[sd->bl.m].name, 16);
	clif_send(packet_len_table[0x104], &sd->bl, PARTY);

	return 0;
}

/*==========================================
 * 攻撃するために移動が必要
 *------------------------------------------
 */
int clif_movetoattack(struct map_session_data *sd, struct block_list *bl)
{
	nullpo_retr(0, sd);
	nullpo_retr(0, bl);

	WPACKETW( 0) = 0x139;
	WPACKETL( 2) = bl->id;
	WPACKETW( 6) = bl->x;
	WPACKETW( 8) = bl->y;
	WPACKETW(10) = sd->bl.x;
	WPACKETW(12) = sd->bl.y;
	WPACKETW(14) = sd->attackrange;
	SENDPACKET(sd->fd, packet_len_table[0x139]);

	return 0;
}

/*==========================================
 * 製造エフェクト
 *------------------------------------------
 */
int clif_produceeffect(struct map_session_data *sd, int flag, int nameid) {
	int view;

	nullpo_retr(0, sd);

	// 名前の登録と送信を先にしておく
	if (map_charid2nick(sd->status.char_id) == NULL)
		map_addchariddb(sd->status.char_id, sd->status.name);
	clif_solved_charname(sd, sd->status.char_id);

	WPACKETW(0) = 0x18f;
	WPACKETW(2) = flag;
	if ((view = itemdb_viewid(nameid)) > 0)
		WPACKETW(4) = view;
	else
		WPACKETW(4) = nameid;
	SENDPACKET(sd->fd, packet_len_table[0x18f]);

	return 0;
}

// pet
int clif_catch_process(struct map_session_data *sd)
{
	nullpo_retr(0, sd);

	WPACKETW(0) = 0x19e;
	SENDPACKET(sd->fd, packet_len_table[0x19e]);

	return 0;
}

void clif_pet_rulet(struct map_session_data *sd, unsigned char flag) { // flag = 0: fail, 1: success
//	nullpo_retv(sd); // checked before to call function

	WPACKETW(0) = 0x1a0;
	WPACKETB(2) = flag; // flag = 0: fail, 1: success
	SENDPACKET(sd->fd, packet_len_table[0x1a0]);

	return;
}

/*==========================================
 * pet卵リスト作成
 *------------------------------------------
 */
int clif_sendegg(struct map_session_data *sd)
{
	int i, n;

	nullpo_retr(0, sd);

	WPACKETW(0) = 0x1a6; //R 01a6 <len>.w <index>.w*
	n = 0;
	if (sd->status.pet_id <= 0) {
		for(i = 0; i < MAX_INVENTORY; i++) {
			if (sd->status.inventory[i].nameid <= 0 || sd->inventory_data[i] == NULL ||
			    sd->inventory_data[i]->type != 7 ||
			    sd->status.inventory[i].amount <= 0)
				continue;
			WPACKETW(n * 2 + 4) = i + 2;
			n++;
		}
	}
	WPACKETW(2) = 4 + n * 2;
	SENDPACKET(sd->fd, WPACKETW(2));

	return 0;
}

int clif_send_petdata(struct map_session_data *sd, int type, int param)
{
	nullpo_retr(0, sd);
	nullpo_retr(0, sd->pd);

	if (type == 5) {
		WPACKETW(0) = 0x1a4;
		WPACKETB(2) = type;
		WPACKETL(3) = sd->pd->bl.id;
		if (sd->packet_ver < 12) { // 0: old, 1: 7july04, 2: 13july04, 3: 26july04, 4: 9aug04/16aug04/17aug04, 5: 6sept04, 6: 21sept04, 7: 18oct04, 8: 25oct04/08nov04, 9: 6dec04, 10: 10jan05, 11: 9may05, 12: 28jun05, 13: 4april06 (by [Yor])
			WPACKETL(7) = 20; // pet_hair_style
		} else {
			// note: version of 0614 is not used in freya (value = 24)
			WPACKETL(7) = 100; // pet_hair_style
		}
		SENDPACKET(sd->fd, packet_len_table[0x1a4]);
	} else {
		WPACKETW(0) = 0x1a4;
		WPACKETB(2) = type;
		WPACKETL(3) = sd->pd->bl.id;
		WPACKETL(7) = param;
		SENDPACKET(sd->fd, packet_len_table[0x1a4]);
	}

	return 0;
}

void clif_send_petstatus(struct map_session_data *sd) {
//	nullpo_retv(sd); // checked before to call function

	WPACKETW( 0) = 0x1a2;
	strncpy(WPACKETP( 2), sd->pet.name, 24);
	WPACKETB(26) = (battle_config.pet_rename == 1) ? 0 : sd->pet.rename_flag;
	WPACKETW(27) = sd->pet.level;
	WPACKETW(29) = sd->pet.hungry;
	WPACKETW(31) = sd->pet.intimate;
	WPACKETW(33) = sd->pet.equip;
	SENDPACKET(sd->fd, packet_len_table[0x1a2]);

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
static void clif_pet_emotion(struct pet_data *pd, int param) {
	struct map_session_data *sd;

	nullpo_retv(pd);
	nullpo_retv(sd = pd->msd);

	if (param >= 100 && sd->petDB->talk_convert_class) {
		if (sd->petDB->talk_convert_class < 0)
			return;
		else if (sd->petDB->talk_convert_class > 0) {
			param -= (pd->class - 100) * 100;
			param += (sd->petDB->talk_convert_class - 100) * 100;
		}
	}

	WPACKETW(0) = 0x1aa;
	WPACKETL(2) = pd->bl.id;
	WPACKETL(6) = param;
	clif_send(packet_len_table[0x1aa], &pd->bl, AREA);

	return;
}

int clif_pet_performance(struct block_list *bl, int param) {
	nullpo_retr(0, bl);

	WPACKETW(0) = 0x1a4;
	WPACKETB(2) = 4;
	WPACKETL(3) = bl->id;
	WPACKETL(7) = param;

	clif_send(packet_len_table[0x1a4], bl, AREA);

	return 0;
}

int clif_pet_equip(struct pet_data *pd, int nameid) {
	int view;

	nullpo_retr(0, pd);

	WPACKETW(0) = 0x1a4;
	WPACKETB(2) = 3;
	WPACKETL(3) = pd->bl.id;
	if ((view = itemdb_viewid(nameid)) > 0)
		WPACKETL(7) = view;
	else
		WPACKETL(7) = nameid;

	clif_send(packet_len_table[0x1a4], &pd->bl, AREA);

	return 0;
}

int clif_pet_food(struct map_session_data *sd, int foodid, int fail)
{
	nullpo_retr(0, sd);

	WPACKETW(0) = 0x1a3;
	WPACKETB(2) = fail;
	WPACKETW(3) = foodid;
	SENDPACKET(sd->fd, packet_len_table[0x1a3]);

	return 0;
}

/*==========================================
 * オートスペル リスト送信
 *------------------------------------------
 */
int clif_autospell(struct map_session_data *sd, int skilllv)
{
	nullpo_retr(0, sd);

	WPACKETW( 0) = 0x1cd;

	if (skilllv > 0 && pc_checkskill(sd, MG_NAPALMBEAT) > 0)
		WPACKETL( 2) = MG_NAPALMBEAT;
	else
		WPACKETL( 2) = 0x00000000;
	if (skilllv > 1 && pc_checkskill(sd, MG_COLDBOLT) > 0)
		WPACKETL( 6) = MG_COLDBOLT;
	else
		WPACKETL( 6) = 0x00000000;
	if (skilllv > 1 && pc_checkskill(sd, MG_FIREBOLT) > 0)
		WPACKETL(10) = MG_FIREBOLT;
	else
		WPACKETL(10) = 0x00000000;
	if (skilllv > 1 && pc_checkskill(sd, MG_LIGHTNINGBOLT) > 0)
		WPACKETL(14) = MG_LIGHTNINGBOLT;
	else
		WPACKETL(14) = 0x00000000;
	if (skilllv > 4 && pc_checkskill(sd, MG_SOULSTRIKE) > 0)
		WPACKETL(18) = MG_SOULSTRIKE;
	else
		WPACKETL(18) = 0x00000000;
	if (skilllv > 7 && pc_checkskill(sd, MG_FIREBALL) > 0)
		WPACKETL(22) = MG_FIREBALL;
	else
		WPACKETL(22) = 0x00000000;
	if (skilllv > 9 && pc_checkskill(sd, MG_FROSTDIVER) > 0)
		WPACKETL(26) = MG_FROSTDIVER;
	else
		WPACKETL(26) = 0x00000000;

	SENDPACKET(sd->fd, packet_len_table[0x1cd]);

	return 0;
}

/*==========================================
 * ディボーションの青い糸
 *------------------------------------------
 */
int clif_devotion(struct map_session_data *sd, int target)
{
	int n;

	nullpo_retr(0, sd);

	WPACKETW( 0) = 0x1cf;
	WPACKETL( 2) = sd->bl.id;
//	WPACKETL( 6) = target;
	for(n = 0; n < 5; n++)
		WPACKETL( 6+4*n) = sd->dev.val2[n];
//		WPACKETL(10+4*n) = 0;
	WPACKETB(26) = 8;
	WPACKETB(27) = 0;

	clif_send(packet_len_table[0x1cf], &sd->bl, AREA);

	return 0;
}

/*==========================================
 * Gospel client packet messages for support effects by [Proximus]
 * type number, message
 * 21: All abnormal status effects have been removed.
 * 22: You will be immune to abnormal status effects for the next minute.
 * 23: Your Max HP will stay increased for the next minute.
 * 24: Your Max SP will stay increased for the next minute.
 * 25: All of your Stats will stay increased for the next minute.
 * 28: Your weapon will remain blessed with Holy power for the next minute.
 * 29: Your armor will remain blessed with Holy power for the next minute.
 * 30: Your Defense will stay increased for the next 10 seconds.
 * 31: Your Attack strength will stay increased for the next minute.
 * 32: Your Accuracy and Flee Rate will stay increased for the next minute.
 *------------------------------------------
 */
void clif_gospel(struct map_session_data *sd, int type) {
	if(!sd) 
		return;

	WPACKETW( 0) = 0x215;
	WPACKETL( 2) = 20 + type;
	SENDPACKET(sd->fd, 6);
	
	return;
}

/*==========================================
 * 氣球
 *------------------------------------------
 */
int clif_spiritball(struct map_session_data *sd)
{
	nullpo_retr(0, sd);

	WPACKETW(0) = 0x1d0;
	WPACKETL(2) = sd->bl.id;
	WPACKETW(6) = sd->spiritball;
	clif_send(packet_len_table[0x1d0], &sd->bl, AREA);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_combo_delay(struct block_list *bl, int wait)
{
	nullpo_retr(0, bl);

	WPACKETW(0) = 0x1d2;
	WPACKETL(2) = bl->id;
	WPACKETL(6) = wait;
	clif_send(packet_len_table[0x1d2], bl, AREA);

	return 0;
}

/*==========================================
 *白刃取り
 *------------------------------------------
 */
int clif_bladestop(struct block_list *src, struct block_list *dst, int bool)
{
	nullpo_retr(0, src);
	nullpo_retr(0, dst);

	switch(src->type) {
		case BL_PC:
		case BL_MOB:
			break;
		default:
			return 0;
	}
	
	switch(dst->type) {
		case BL_PC:
		case BL_MOB:
			break;
		default:
			return 0;
	}
	
	WPACKETW( 0) = 0x1d1;
	WPACKETL( 2) = src->id;
	WPACKETL( 6) = dst->id;
	WPACKETL(10) = bool;

	clif_send(packet_len_table[0x1d1], src, AREA);

	return 0;
}

/*==========================================
 * GMへ場所とHP通知
 *------------------------------------------
 */
inline void clif_load_hpsp(struct map_session_data *sd) {

	WPACKETW(0) = 0x107;
	WPACKETL(2) = sd->bl.id;
	WPACKETW(6) = sd->bl.x;
	WPACKETW(8) = sd->bl.y;
	
	WPACKETW(0 + packet_len_table[0x107]) = 0x106;
	WPACKETL(2 + packet_len_table[0x107]) = sd->status.account_id;
	WPACKETW(6 + packet_len_table[0x107]) = (sd->status.hp > 0x7fff) ? 0x7fff : sd->status.hp;
	WPACKETW(8 + packet_len_table[0x107]) = (sd->status.max_hp > 0x7fff) ? 0x7fff : sd->status.max_hp;

	return;
}

void clif_hpmeter(struct map_session_data *sd, struct map_session_data *dstsd) { // NULL: send to any people, other: send to only 1 player
	struct map_session_data *sd2;

	//nullpo_retr(0, sd); //checked before to call

	if(dstsd != NULL) {
		if (dstsd->state.auth && dstsd->GM_level >= battle_config.display_hpmeter) {
			if(dstsd->GM_level >= sd->GM_level) {
				clif_load_hpsp(sd); //Store hp/sp info in buffer
				SENDPACKET(dstsd->fd, packet_len_table[0x107] + packet_len_table[0x106]);
			}
		}
	} else {
		int i, x0, y0, x1, y1;

		x0 = sd->bl.x - AREA_SIZE;
		y0 = sd->bl.y - AREA_SIZE;
		x1 = sd->bl.x + AREA_SIZE;
		y1 = sd->bl.y + AREA_SIZE;
		clif_load_hpsp(sd); //Store hp/sp info in buffer

		for (i = 0; i < fd_max; i++) {
			if (session[i] && (sd2 = session[i]->session_data) && /*sd != sd2 &&*/ sd2->state.auth) {	
				if (sd2->bl.m != sd->bl.m ||
					sd2->bl.x < x0 || sd2->bl.y < y0 ||
					sd2->bl.x > x1 || sd2->bl.y > y1 ||
					sd2->GM_level < sd->GM_level ||
					sd2->GM_level < battle_config.display_hpmeter)
					continue;
				SENDPACKET(i, packet_len_table[0x107] + packet_len_table[0x106]);
			}
		}
	}

	return;
}

/*==================================================
 * Update monster hp view if it has changed [Celest]
 * Implemented in Freya from peterxxl & Sirius_Black comments by MagicalTux
 *--------------------------------------------------
 */

int clif_update_mobhp(struct mob_data *md) {
	struct guild *g;
	struct guild_castle *gc;

//	nullpo_retr(0, md); // checked before to call function

	if (md->guild_id && (g = guild_search(md->guild_id)) != NULL && // Guilds' gardians & emperiums, otherwize = 0
	    (gc = guild_mapname2gc(map[md->bl.m].name))) {
		if (battle_config.show_mob_hp > 1) {
			memset(WPACKETP(0), 0, packet_len_table[0x195]);
			WPACKETW( 0) = 0x195;
			WPACKETL( 2) = md->bl.id;
			strncpy(WPACKETP( 6), md->name, 24);
			//sprintf(WPACKETP(30), "hp: %d/%d", md->hp, mob_db[md->class].max_hp); // maximum is variable -> don't show max HP
			sprintf(WPACKETP(30), "Actual hp: %d", md->hp);
			strncpy(WPACKETP(54), g->name, 24);
			strncpy(WPACKETP(78), gc->castle_name, 24);
			clif_send(packet_len_table[0x195], &md->bl, AREA);
		}
	} else { // check of battle_config.show_mob_hp os done before to call function
		memset(WPACKETP(0), 0, packet_len_table[0x195]);
		WPACKETW( 0) = 0x195;
		WPACKETL( 2) = md->bl.id;
		strncpy(WPACKETP( 6), md->name, 24);
		//memset(WPACKETP(30), 0, 24);
		//memset(WPACKETP(54), 0, 24);
		WPACKETB(54) = ' '; // to force display with new clients
		sprintf(WPACKETP(78), "hp: %d/%d", md->hp, mob_db[md->class].max_hp);
		clif_send(packet_len_table[0x195], &md->bl, AREA);
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_changemapcell(int m, int x, int y, int cell_type, int type)
{
	struct block_list bl;

	bl.m = m;
	bl.x = x;
	bl.y = y;
	WPACKETW(0) = 0x192;
	WPACKETW(2) = x;
	WPACKETW(4) = y;
	WPACKETW(6) = cell_type;
	strncpy(WPACKETP(8), map[m].name, 16);
	if (!type)
		clif_send(packet_len_table[0x192], &bl, AREA);
	else
		clif_send(packet_len_table[0x192], &bl, ALL_SAMEMAP);

	return 0;
}

/*==========================================
 * MVPエフェクト
 *------------------------------------------
 */
int clif_mvp_effect(struct map_session_data *sd)
{
	nullpo_retr(0, sd);

	WPACKETW(0) = 0x10c;
	WPACKETL(2) = sd->bl.id;
	clif_send(packet_len_table[0x10c], &sd->bl, AREA);

	return 0;
}

/*==========================================
 * MVPアイテム所得
 *------------------------------------------
 */
int clif_mvp_item(struct map_session_data *sd, int nameid)
{
	int view;

	nullpo_retr(0, sd);

	WPACKETW(0) = 0x10a;
	if ((view = itemdb_viewid(nameid)) > 0)
		WPACKETW(2) = view;
	else
		WPACKETW(2) = nameid;
	SENDPACKET(sd->fd, packet_len_table[0x10a]);

	return 0;
}

/*==========================================
 * MVP経験値所得
 *------------------------------------------
 */
int clif_mvp_exp(struct map_session_data *sd, int mexp)
{
	nullpo_retr(0, sd);

	WPACKETW(0) = 0x10b;
	WPACKETL(2) = mexp;
	SENDPACKET(sd->fd, packet_len_table[0x10b]);

	return 0;
}

/*==========================================
 * ギルド作成可否通知
 *------------------------------------------
 */
void clif_guild_created(struct map_session_data *sd, int flag) {
//	nullpo_retv(sd); // checked before to call function

	WPACKETW(0) = 0x167;
	WPACKETB(2) = flag; // 0x167 <flag>.b: 0: Guild has been created., 1: You are already in a guild., 2: That Guild Name already exists., 3: You need the necessary item to create a Guild.
	SENDPACKET(sd->fd, packet_len_table[0x167]);

	return;
}

/*==========================================
 * ギルド所属通知
 *------------------------------------------
 */
int clif_guild_belonginfo(struct map_session_data *sd, struct guild *g)
{
	int ps;

	nullpo_retr(0, sd);
	nullpo_retr(0, g);

	ps = guild_getposition(sd, g);

	memset(WPACKETP(0), 0, packet_len_table[0x16c]);

	WPACKETW( 0) = 0x16c;
	WPACKETL( 2) = g->guild_id;
	WPACKETL( 6) = g->emblem_id;
	WPACKETL(10) = g->position[ps].mode;
	strncpy(WPACKETP(19), g->name, 24);
	SENDPACKET(sd->fd, packet_len_table[0x16c]);

	return 0;
}

/*==========================================
 * ギルドメンバログイン通知
 *------------------------------------------
 */
int clif_guild_memberlogin_notice(struct guild *g, int idx, int flag)
{
	nullpo_retr(0, g);

	WPACKETW( 0) = 0x16d;
	WPACKETL( 2) = g->member[idx].account_id;
	WPACKETL( 6) = g->member[idx].char_id;
	WPACKETL(10) = flag;
	if (g->member[idx].sd == NULL) {
		struct map_session_data *sd = guild_getavailablesd(g);
		if (sd != NULL)
			clif_send(packet_len_table[0x16d], &sd->bl, GUILD);
	} else
		clif_send(packet_len_table[0x16d], &g->member[idx].sd->bl, GUILD_WOS);

	return 0;
}

/*==========================================
 * ギルドマスター通知(14dへの応答)
 *------------------------------------------
 */
void clif_guild_masterormember(struct map_session_data *sd) { // R 014E <type?>.l

//	nullpo_retv(sd); // checked before to call function

	WPACKETW(0) = 0x14e;
	WPACKETL(2) = (sd->status.guild_id > 0 && sd->state.gmaster_flag)? 0xd7 : 0x57; //guildmaster : member
	SENDPACKET(sd->fd, packet_len_table[0x14e]);

	return;
}

/*==========================================
 * Basic Info (Territories [Valaris])
 *------------------------------------------
 */
void clif_guild_basicinfo(struct map_session_data *sd) {
	int i, t;
	struct guild *g;
	struct guild_castle *gc;

//	nullpo_retv(sd); // checked before to call function

	g = guild_search(sd->status.guild_id);
	if (g == NULL)
		return;

	t = 0;
	for(i = 0; i < MAX_GUILDCASTLE; i++) {
		gc = guild_castle_search(i);
		if (!gc)
			continue;
		if (g->guild_id == gc->guild_id)
			t++;
	}

	memset(WPACKETP(0), 0, packet_len_table[0x1b6]);

	// R 01B6 <guildID>.l <guildLv>.l <connum>.l <定員>.l <Avl.lvl>.l <now_exp>.l <next_exp>.l <上納ポイント>.l <性向F-V>.l <性向R-W>.l <members>.l <guild name>.24B <guild master>.24B <agit?>.20B
	// R 0150 <guildID>.l <guildLv>.l <接続数>.l <定員>.l <Avl.lvl>.l <経験値>.l <next_exp>.l <上納ポイント>.l <性向左右VW>.l <性向上下RF>.l <人数？>.l <guild name>.24B <guild master>.24B <本拠地>.16B
	WPACKETW( 0) = 0x1b6; // 0x150;
	WPACKETL( 2) = g->guild_id;
	WPACKETL( 6) = g->guild_lv;
	WPACKETL(10) = g->connect_member;
	WPACKETL(14) = g->max_member;
	WPACKETL(18) = g->average_lv;
	WPACKETL(22) = g->exp;
	WPACKETL(26) = g->next_exp;
//	WPACKETL(30) = 0; // 上納
//	WPACKETL(34) = 0; // VW（性格の悪さ？：性向グラフ左右）
//	WPACKETL(38) = 0; // RF（正義の度合い？：性向グラフ上下）
//	WPACKETL(42) = 0; // 人数？
	strncpy(WPACKETP(46), g->name, 24);
	strncpy(WPACKETP(70), g->master, 24);
//	memcpy(WPACKETP(94),"",20); // 本拠地

	if (t == MAX_GUILDCASTLE) 
		strncpy(WPACKETP(94), "Total Domination", 20);
	else {
		switch(t) {
			case 0:  strncpy(WPACKETP(94), "None Taken",           20); break;
			case 1:  strncpy(WPACKETP(94), "One Castle",           20); break;
			case 2:  strncpy(WPACKETP(94), "Two Castles",          20); break;
			case 3:	 strncpy(WPACKETP(94), "Three Castles",        20); break;
			case 4:	 strncpy(WPACKETP(94), "Four Castles",         20); break;
			case 5:	 strncpy(WPACKETP(94), "Five Castles",         20); break;
			case 6:	 strncpy(WPACKETP(94), "Six Castles",          20); break;
			case 7:	 strncpy(WPACKETP(94), "Seven Castles",        20); break;
			case 8:	 strncpy(WPACKETP(94), "Eight Castles",        20); break;
			case 9:	 strncpy(WPACKETP(94), "Nine Castles",         20); break;
			case 10: strncpy(WPACKETP(94), "Ten Castles",          20); break;
			case 11: strncpy(WPACKETP(94), "Eleven Castles",       20); break;
			case 12: strncpy(WPACKETP(94), "Twelve Castles",       20); break;
			case 13: strncpy(WPACKETP(94), "Thirteen Castles",     20); break;
			case 14: strncpy(WPACKETP(94), "Fourteen Castles",     20); break;
			case 15: strncpy(WPACKETP(94), "Fifteen Castles",      20); break;
			case 16: strncpy(WPACKETP(94), "Sixteen Castles",      20); break;
			case 17: strncpy(WPACKETP(94), "Seventeen Castles",    20); break;
			case 18: strncpy(WPACKETP(94), "Eighteen Castles",     20); break;
			case 19: strncpy(WPACKETP(94), "Nineteen Castles",     20); break;
			case 20: strncpy(WPACKETP(94), "Twenty Castles",       20); break;
			case 21: strncpy(WPACKETP(94), "Twenty One Castles",   20); break;
			case 22: strncpy(WPACKETP(94), "Twenty Two Castles",   20); break;
			case 23: strncpy(WPACKETP(94), "Twenty Three Castles", 20); break;
			case 24: strncpy(WPACKETP(94), "Twenty Four Castles",  20); break;
		}
	}

	SENDPACKET(sd->fd, packet_len_table[0x1b6]);
	clif_guild_emblem(sd, g); // Guild emblem vanish fix

	return;
}

/*==========================================
 * ギルド同盟/敵対情報
 *------------------------------------------
 */
void clif_guild_allianceinfo(struct map_session_data *sd) {
	int i, len;
	struct guild *g;

//	nullpo_retv(sd); // checked before to call function

	g = guild_search(sd->status.guild_id);
	if (g == NULL)
		return;

	WPACKETW(0) = 0x14c;
	len = 4;
	for(i = 0; i < MAX_GUILDALLIANCE; i++) {
		struct guild_alliance *a = &g->alliance[i];
		if (a->guild_id > 0) {
			WPACKETL(len    ) = a->opposition;
			WPACKETL(len + 4) = a->guild_id;
			strncpy(WPACKETP(len + 8), a->name, 24);
			len += 32;
		}
	}
	WPACKETW(2) = len;
	SENDPACKET(sd->fd, len);

	return;
}

/*==========================================
 * ギルドメンバーリスト
 *------------------------------------------
 */
void clif_guild_memberlist(struct map_session_data *sd) {
	int i, len;
	struct guild *g;

	nullpo_retv(sd);

	g = guild_search(sd->status.guild_id);
	if (g == NULL)
		return;

	memset(WPACKETP(0), 0, 104 * g->max_member + 4);

	WPACKETW(0) = 0x154; // R 0154 <len>.w {<accID>.l <charactorID>.l <髪型>.w <髪の色>.w <性別？>.w <job>.w <lvl?>.w <上納経験値>.l <online>.l <Position>.l <メモ？>.50B <nick>.24B}*
	len = 4;
	for(i = 0; i < g->max_member; i++) {
		struct guild_member *m = &g->member[i];
		if (m->account_id == 0)
			continue;
		WPACKETL(len     ) = m->account_id;
		WPACKETL(len +  4) = m->char_id;
		WPACKETW(len +  8) = m->hair;
		WPACKETW(len + 10) = m->hair_color;
		WPACKETW(len + 12) = m->gender;
		WPACKETW(len + 14) = m->class;
		WPACKETW(len + 16) = m->lv;
		WPACKETL(len + 18) = m->exp;
		WPACKETL(len + 22) = m->online;
		WPACKETL(len + 26) = m->position;
		//memset(WPACKETP(len + 30), 0, 50); // メモ？
		strncpy(WPACKETP(len + 80), m->name, 24);
		len += 104;
	}
	WPACKETW(2) = len;
	SENDPACKET(sd->fd, len);

	return;
}

/*==========================================
 * ギルド役職名リスト
 *------------------------------------------
 */
void clif_guild_positionnamelist(struct map_session_data *sd) {
	int i, len;
	struct guild *g;

//	nullpo_retv(sd); // checked before to call function

	g = guild_search(sd->status.guild_id);
	if (g == NULL)
		return;

	WPACKETW(0) = 0x166;
	len = 4;
	for(i = 0; i < MAX_GUILDPOSITION; i++) {
		WPACKETL(len) = i;
		strncpy(WPACKETP(len + 4), g->position[i].name, 24);
		len += 28;
	}
	WPACKETW(2) = len;
	SENDPACKET(sd->fd, len);

	return;
}

/*==========================================
 * ギルド役職情報リスト
 *------------------------------------------
 */
void clif_guild_positioninfolist(struct map_session_data *sd) {
	int i, len;
	struct guild *g;

//	nullpo_retv(sd); // checked before to call function

	g = guild_search(sd->status.guild_id);
	if (g == NULL)
		return;

	WPACKETW(0) = 0x160;
	len = 4;
	for(i = 0; i < MAX_GUILDPOSITION; i++) {
		struct guild_position *p = &g->position[i];
		WPACKETL(len     ) = i;
		WPACKETL(len +  4) = p->mode;
		WPACKETL(len +  8) = i;
		WPACKETL(len + 12) = p->exp_mode;
		len += 16;
	}
	WPACKETW(2) = len;
	SENDPACKET(sd->fd, len);

	return;
}

/*==========================================
 * ギルド役職変更通知
 *------------------------------------------
 */
int clif_guild_positionchanged(struct guild *g, int idx)
{
	struct map_session_data *sd;

	nullpo_retr(0, g);

	if ((sd = guild_getavailablesd(g)) != NULL) {
		WPACKETW( 0) = 0x174;
		WPACKETW( 2) = 44;
		WPACKETL( 4) = idx;
		WPACKETL( 8) = g->position[idx].mode;
		WPACKETL(12) = idx;
		WPACKETL(16) = g->position[idx].exp_mode;
		strncpy(WPACKETP(20), g->position[idx].name, 24);
		clif_send(44, &sd->bl, GUILD);
	}

	return 0;
}

/*==========================================
 * ギルドメンバ変更通知
 *------------------------------------------
 */
int clif_guild_memberpositionchanged(struct guild *g, int idx)
{
	struct map_session_data *sd;

	nullpo_retr(0, g);

	WPACKETW( 0) = 0x156;
	WPACKETW( 2) = 16;
	WPACKETL( 4) = g->member[idx].account_id;
	WPACKETL( 8) = g->member[idx].char_id;
	WPACKETL(12) = g->member[idx].position;
	if ((sd = guild_getavailablesd(g)) != NULL)
		clif_send(16, &sd->bl, GUILD);

	return 0;
}

/*==========================================
 * ギルドエンブレム送信
 *------------------------------------------
 */
void clif_guild_emblem(struct map_session_data *sd, struct guild *g) {
//	nullpo_retv(sd); // checked before to call function
//	nullpo_retv(g); // checked before to call function

	if (g->emblem_len <= 0)
		return;

	WPACKETW(0) = 0x152;
	WPACKETW(2) = g->emblem_len + 12;
	WPACKETL(4) = g->guild_id;
	WPACKETL(8) = g->emblem_id;
	memcpy(WPACKETP(12), g->emblem_data, g->emblem_len);
	SENDPACKET(sd->fd, g->emblem_len + 12);

	return;
}

/*==========================================
 * ギルドスキル送信
 *------------------------------------------
 */
void clif_guild_skillinfo(struct map_session_data *sd) {
	int i, id, len, up = 1;
	struct guild *g;

//	nullpo_retv(sd); // checked before to call function

	g = guild_search(sd->status.guild_id);
	if (g == NULL)
		return;

	memset(WPACKETP(0), 0, MAX_GUILDSKILL * 24 + 6);

	WPACKETW(0) = 0x0162; // R 0162 <len>.w <skill_point>.w {<skill ID>.w <target type>.w ?.w <lv>.w <sp>.w <range>.w <skill name>.24B <up>.B}.37B*
	WPACKETW(4) = g->skill_point;
	len = 6;
	for(i = 0; i < MAX_GUILDSKILL; i++) {
			if((id = g->skill[i].id) > 0 && guild_check_skill_require(g, g->skill[i].id)){
			WPACKETW(len     ) = id;
			WPACKETW(len +  2) = guild_skill_get_inf(id);
			WPACKETW(len +  4) = 0;
			WPACKETW(len +  6) = g->skill[i].lv;
			WPACKETW(len +  8) = guild_skill_get_sp(id, g->skill[i].lv);
			WPACKETW(len + 10) = guild_skill_get_range(id);
			//strncpy(WPACKETP(len + 12), SKILL_NAME, 24);
			if(g->skill[i].lv < guild_skill_get_max(id) && (sd == g->member[0].sd))
				up = 1;
			else
				up = 0;
			WPACKETB(len + 36) = up;
			len += 37;
		}
	}
	WPACKETW(2) = len;
	SENDPACKET(sd->fd, len);

	return;
}

/*==========================================
 * ギルド告知送信
 *------------------------------------------
 */
int clif_guild_notice(struct map_session_data *sd, struct guild *g)
{
	nullpo_retr(0, sd);
	nullpo_retr(0, g);

	if (*g->mes1 == 0 && *g->mes2 == 0)
		return 0;

	WPACKETW(0) = 0x16f;
	strncpy(WPACKETP( 2), g->mes1,  60);
	strncpy(WPACKETP(62), g->mes2, 120);
	SENDPACKET(sd->fd, packet_len_table[0x16f]);

	return 0;
}

/*==========================================
 * ギルドメンバ勧誘
 *------------------------------------------
 */
void clif_guild_invite(struct map_session_data *sd, struct guild *g) {
//	nullpo_retv(sd); // checked before to call function
//	nullpo_retv(g); // checked before to call function

	WPACKETW(0) = 0x16a;
	WPACKETL(2) = g->guild_id;
	strncpy(WPACKETP(6), g->name, 24);
	SENDPACKET(sd->fd, packet_len_table[0x16a]);

	return;
}

/*==========================================
 * ギルドメンバ勧誘結果
 *------------------------------------------
 */
void clif_guild_inviteack(struct map_session_data *sd, int flag) {
//	nullpo_retv(sd); // checked before to call function

	WPACKETW(0) = 0x169; // R 0169 <type>.B (Types: 0: in an other guild, 1: it was denied, 2: join, 3: guild has no more available place)
	WPACKETB(2) = flag;
	SENDPACKET(sd->fd, packet_len_table[0x169]);

	return;
}

/*==========================================
 * ギルドメンバ脱退通知
 *------------------------------------------
 */
int clif_guild_leave(struct map_session_data *sd,const char *name,const char *mes)
{
	nullpo_retr(0, sd);

	WPACKETW(0) = 0x15a;
	strncpy(WPACKETP( 2), name, 24);
	strncpy(WPACKETP(26), mes, 40);
	clif_send(packet_len_table[0x15a], &sd->bl, GUILD);

	return 0;
}

/*==========================================
 * ギルドメンバ追放通知
 *------------------------------------------
 */
void clif_guild_explusion(struct map_session_data *sd, const char *name, const char *mes, int account_id) {
	nullpo_retv(sd);

	WPACKETW(0) = 0x15c;
	strncpy(WPACKETP( 2), name, 24);
	strncpy(WPACKETP(26), mes, 40);
	strncpy(WPACKETP(66), "dummy", 24);
	clif_send(packet_len_table[0x15c], &sd->bl, GUILD);

	return;
}

/*==========================================
 * ギルド追放メンバリスト
 *------------------------------------------
 */
void clif_guild_explusionlist(struct map_session_data *sd) {
	int i, len;
	struct guild *g;

//	nullpo_retv(sd); // checked before to call function

	g = guild_search(sd->status.guild_id);
	if (g == NULL)
		return;

	WPACKETW(0) = 0x163; // R 0163 <len>.w { <nick>.24B <アカウントＩＤ>.24B <追放理由>.40B }.88B*
	len = 4;
	for(i = 0; i < MAX_GUILDEXPLUSION; i++) {
		struct guild_explusion *e = &g->explusion[i];
		if (e->account_id > 0) {
			strncpy(WPACKETP(len     ), e->name, 24);
			strncpy(WPACKETP(len + 24), e->acc,  24);
			strncpy(WPACKETP(len + 48), e->mes,  40);
			len += 88;
		}
	}
	WPACKETW(2) = len;
	SENDPACKET(sd->fd, len);

	return;
}

/*==========================================
 * ギルド会話
 *------------------------------------------
 */
int clif_guild_message(struct guild *g, int account_id, const char *mes, short len) {
	struct map_session_data *sd;

	if (len < 1 || (sd = guild_getavailablesd(g)) == NULL)
		return 0;

//	printf("clif_guild_message(g->name:%s, account_id:%d, mes:%s).\n", g->name, account_id, mes);

	WPACKETW(0) = 0x17f;
	WPACKETW(2) = len + 5; // 4 + 1 for NULL
	strncpy(WPACKETP(4), mes, len);
	WPACKETB(len + 4) = 0; // NULL
	clif_send(len + 5, &sd->bl, GUILD);

	return 0;
}

/*==========================================
 * ギルドスキル割り振り通知
 *------------------------------------------
 */
int clif_guild_skillup(struct map_session_data *sd, int skill_num, int lv)
{
	nullpo_retr(0, sd);

	WPACKETW( 0) = 0x10e;
	WPACKETW( 2) = skill_num;
	WPACKETW( 4) = lv;
	WPACKETW( 6) = guild_skill_get_sp(skill_num, lv);
	WPACKETW( 8) = guild_skill_get_range(skill_num);
	WPACKETB(10) = 1;
	SENDPACKET(sd->fd, 11);

	return 0;
}

/*==========================================
 * ギルド同盟要請
 *------------------------------------------
 */
int clif_guild_reqalliance(struct map_session_data *sd, int account_id, const char *name)
{
	nullpo_retr(0, sd);

	WPACKETW(0) = 0x171;
	WPACKETL(2) = account_id;
	strncpy(WPACKETP(6), name, 24);
	SENDPACKET(sd->fd, packet_len_table[0x171]);

	return 0;
}

/*==========================================
 * ギルド同盟結果
 *------------------------------------------
 */
int clif_guild_allianceack(struct map_session_data *sd, int flag)
{
	nullpo_retr(0, sd);

	WPACKETW(0) = 0x173;
	WPACKETL(2) = flag;
	SENDPACKET(sd->fd, packet_len_table[0x173]);

	return 0;
}

/*==========================================
 * ギルド関係解消通知
 *------------------------------------------
 */
int clif_guild_delalliance(struct map_session_data *sd, int guild_id, int flag)
{
	nullpo_retr(0, sd);

	WPACKETW(0) = 0x184;
	WPACKETL(2) = guild_id;
	WPACKETL(6) = flag;
	SENDPACKET(sd->fd, packet_len_table[0x184]);

	return 0;
}

/*==========================================
 * ギルド敵対結果
 *------------------------------------------
 */
void clif_guild_oppositionack(struct map_session_data *sd, unsigned char flag) { // flag: 0: success, 1: (failed)opposition guild list is full, 2: (failed)already in opposition guild list
//	nullpo_retv(sd); // checked before to call function

	WPACKETW(0) = 0x181;
	WPACKETB(2) = flag; // flag: 0: success, 1: (failed)opposition guild list is full, 2: (failed)already in opposition guild list
	SENDPACKET(sd->fd, packet_len_table[0x181]);

	return;
}

/*==========================================
 * ギルド関係追加
 *------------------------------------------
 */
/*int clif_guild_allianceadded(struct guild *g, int idx)
{
	WPACKETW(0) = 0x185;
	WPACKETL(2) = g->alliance[idx].opposition;
	WPACKETL(6) = g->alliance[idx].guild_id;
	strncpy(WPACKETP(10), g->alliance[idx].name, 24);
	clif_send(packet_len_table[0x185], guild_getavailablesd(g), GUILD);

	return 0;
}*/

/*==========================================
 * ギルド解散通知
 *------------------------------------------
 */
void clif_guild_broken(struct map_session_data *sd, int flag) {
//	nullpo_retv(sd); // checked before to call function

	WPACKETW(0) = 0x15e;
	WPACKETL(2) = flag;
	SENDPACKET(sd->fd, packet_len_table[0x15e]);

	return;
}

/*==========================================
 * エモーション
 *------------------------------------------
 */
void clif_emotion(struct block_list *bl, int type)
{
	nullpo_retv(bl);

	WPACKETW(0) = 0xc0;
	WPACKETL(2) = bl->id;
	WPACKETB(6) = type;
	clif_send(packet_len_table[0xc0], bl, AREA);
}

/*==========================================
 * トーキーボックス
 *------------------------------------------
 */
void clif_talkiebox(struct block_list *bl, char* talkie)
{
	nullpo_retv(bl);

	WPACKETW(0) = 0x191;
	WPACKETL(2) = bl->id;
	strncpy(WPACKETP(6), talkie, 80);
	clif_send(packet_len_table[0x191], bl, AREA);
}

/*==========================================
 * 結婚エフェクト
 *------------------------------------------
 */
void clif_wedding_effect(struct block_list *bl) {
	nullpo_retv(bl);

	WPACKETW(0) = 0x1ea;
	WPACKETL(2) = bl->id;
	clif_send(packet_len_table[0x1ea], bl, AREA);
}

/*==========================================
 * あなたに逢いたい使用時名前叫び
 *------------------------------------------

void clif_callpartner(struct map_session_data *sd)
{
	char *p;

	nullpo_retv(sd);

	if (sd->status.partner_id) {
		p = map_charid2nick(sd->status.partner_id);
		if (p) {
			WPACKETW(0) = 0x1e6;
			strncpy(WPACKETP(2), p, 24);
		} else {
			map_reqchariddb(sd, sd->status.partner_id);
			chrif_searchcharid(sd->status.partner_id);
			WPACKETW(0) = 0x1e6;
			memset(WPACKETP(2), 0, 24);
		}
		clif_send(packet_len_table[0x1e6], &sd->bl, AREA);
	}

	return;
}*/

/*==========================================
 * Adopt baby [Celest]
 *------------------------------------------
 */
void clif_adopt_process(struct map_session_data *sd) {
	nullpo_retv(sd);

	WPACKETW(0) = 0x1f8;
	SENDPACKET(sd->fd, packet_len_table[0x1f8]);

	return;
}

/*==========================================
 * Marry [DracoRPG]
 *------------------------------------------
 */
void clif_marriage_process(struct map_session_data *sd) {
	nullpo_retv(sd);

	WPACKETW(0) = 0x1e4;
	SENDPACKET(sd->fd, packet_len_table[0x1e4]);

	return;
}


/*==========================================
 * Notice of divorce
 *------------------------------------------
 */
void clif_divorced(struct map_session_data *sd, char *name) {
	nullpo_retv(sd);

	WPACKETW(0) = 0x205;
	strncpy(WPACKETP(2), name, 24);
	SENDPACKET(sd->fd, packet_len_table[0x205]);

	return;
}

/* Future adoption packet
void clif_adoption_invite(struct map_session_data *sd, struct map_session_data *tsd) { //0x1f6 <src_accid>.L, <dst_accid>.L, <name>.24

	WPACKETW(0) = 0x1f6;
	WPACKETL(2) = sd->status.account_id;
	WPACKETL(6) = tsd->status.account_id;
	strncpy(WPACKETP(10), sd->status.name, 24);
	SENDPACKET(tsd->fd, packet_len_table[0xfe]);

	return;
}*/

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_ReqAdopt(int fd, struct map_session_data *sd) { // S 0x01f9 <ID>.l
//	nullpo_retv(sd); // checked before to call function

//	SENDPACKET(fd, packet_len_table[0x1f6]);

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_ReqMarriage(int fd, struct map_session_data *sd) {
	nullpo_retv(sd);

	WPACKETW(0) = 0x1e2;
	SENDPACKET(fd, packet_len_table[0x1e2]);

	return;
}

/*==========================================
 * 座る
 *------------------------------------------
 */
void clif_sitting(struct map_session_data *sd) {
	nullpo_retv(sd);

	WPACKETW( 0) = 0x8a;
	WPACKETL( 2) = sd->bl.id;
	WPACKETB(26) = 2;
	clif_send(packet_len_table[0x8a], &sd->bl, AREA);
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_disp_onlyself(struct map_session_data *sd, char *mes) {
	int len;

	nullpo_retv(sd);

	len = strlen(mes);

	WPACKETW(0) = 0x17f;
	WPACKETW(2) = len + 5; // 4 + 1 for NULL
	strncpy(WPACKETP(4), mes, len);
	WPACKETB(len + 4) = 0; // NULL
	SENDPACKET(sd->fd, len + 5);

	return;
}

/*==========================================
 *
 *------------------------------------------
 */

int clif_GM_kickack(struct map_session_data *sd, int id)
{
	nullpo_retr(0, sd);

	WPACKETW(0) = 0xcd;
	WPACKETL(2) = id;
	SENDPACKET(sd->fd, packet_len_table[0xcd]);

	return 0;
}

int clif_GM_kick(struct map_session_data *sd, struct map_session_data *tsd, int type)
{
	nullpo_retr(0, tsd);

	if (type)
		clif_GM_kickack(sd, tsd->status.account_id);
	tsd->opt1 = tsd->opt2 = 0;
	WPACKETW(0) = 0x18b;
	WPACKETW(2) = 0; // flag= 0: success, 1: failure (please wait 15 sec...)
	SENDPACKET(tsd->fd, packet_len_table[0x18b]);
	clif_setwaitclose(tsd->fd);

	return 0;
}

/*==========================================
 * Wis拒否許可応答
 *------------------------------------------
 */
int clif_wisexin(struct map_session_data *sd, int type, int flag)
{
	nullpo_retr(0, sd);

	WPACKETW(0) = 0xd1;
	WPACKETB(2) = type;
	WPACKETB(3) = flag;
	SENDPACKET(sd->fd, packet_len_table[0xd1]);

	return 0;
}

/*==========================================
 * Wis全拒否許可応答
 *------------------------------------------
 */
int clif_wisall(struct map_session_data *sd, int type, int flag)
{
	nullpo_retr(0, sd);

	WPACKETW(0) = 0xd2;
	WPACKETB(2) = type;
	WPACKETB(3) = flag;
	SENDPACKET(sd->fd, packet_len_table[0xd2]);

	return 0;
}

/*==========================================
 * サウンドエフェクト
 *------------------------------------------
 */
void clif_soundeffect(struct map_session_data *sd, struct block_list *bl, char *name, int type)
{
	nullpo_retv(sd);
	nullpo_retv(bl);

	WPACKETW( 0) = 0x1d3;
	strncpy(WPACKETP(2), name, 24);
	WPACKETB(26) = type;
	WPACKETL(27) = 0;
	WPACKETL(31) = bl->id;
	SENDPACKET(sd->fd, packet_len_table[0x1d3]);

	return;
}

int clif_soundeffectall(struct block_list *bl, char *name, int type)
{
	nullpo_retr(0, bl);

	WPACKETW(0) = 0x1d3;
	strncpy(WPACKETP(2), name, 24);
	WPACKETB(26) = type;
	WPACKETL(27) = 0;
	WPACKETL(31) = bl->id;
	clif_send(packet_len_table[0x1d3], bl, AREA);

	return 0;
}

// displaying special effects (npcs, weather, etc) [Valaris]
int clif_specialeffect(struct block_list *bl, int type, int flag) { // flag: 0: player see in the area (normal), 1: only player see only by player, 2: all players in a map that see only their (not see others), 3: all players that see only their (not see others)
	struct map_session_data *pl_sd;
	int i;

	nullpo_retr(0, bl);

	if (flag == 3) {
		for(i = 0; i < fd_max; i++) {
			if (session[i] && (pl_sd = session[i]->session_data) != NULL &&
			    pl_sd->state.auth &&
			    (((struct map_session_data *)&bl)->GM_level > ((struct map_session_data *)&pl_sd->bl)->GM_level))
				clif_specialeffect(&pl_sd->bl, type, 1); // flag: 0: player see in the area (normal), 1: only player see only by player, 2: all players in a map that see only their (not see others), 3: all players that see only their (not see others)
		}
	} else if (flag == 2) {
		for(i = 0; i < fd_max; i++) {
			if (session[i] && (pl_sd = session[i]->session_data) != NULL && pl_sd->state.auth && pl_sd->bl.m == bl->m)
				clif_specialeffect(&pl_sd->bl, type, 1); // flag: 0: player see in the area (normal), 1: only player see only by player, 2: all players in a map that see only their (not see others), 3: all players that see only their (not see others)
		}
	} else if (flag == 1) {
		struct map_session_data *sd = (struct map_session_data *)bl;
		WPACKETW(0) = 0x1f3;
		WPACKETL(2) = bl->id;
		WPACKETL(6) = type;
		SENDPACKET(sd->fd, packet_len_table[0x1f3]);
	} else if (!flag) {
		WPACKETW(0) = 0x1f3;
		WPACKETL(2) = bl->id;
		WPACKETL(6) = type;
		clif_send(packet_len_table[0x1f3], bl, AREA);
	}

	return 0;
}

// ------------
// timer for no answer from char-server (disconnected?) about a connection
// ------------
int clif_waitauth(int tid, unsigned int tick, int id, int data) { // id = sd->fd, data = sd->bl.id
	struct map_session_data *sd;

	// check if the session is always the same [Yor]
	if (session[id] != NULL && (sd = session[id]->session_data) != NULL && sd->fd == id && sd->bl.id == data) {
		// if player is not authentified (and we have not received answer of char-server)
		if (!sd->state.auth) {
			printf("Char-server doesn't give an answer about connection of account #%d.\n", data);
			clif_authfail_fd(id, 0); // Disconnected from server
		}
	}

	return 0;
}

/*==========================================
 * notification of friend list
 *------------------------------------------
 */
void clif_friend_send_info(struct map_session_data *sd) {
	int i;
	unsigned short len;

//	nullpo_retv(sd); // checked before to call function

	WPACKETW(0) = 0x201;
	len = 4;
	for(i = 0; i < sd->friend_num; i++) {
		struct friend *frd = &sd->friends[i];
		if (frd->char_id) {
			WPACKETL(len    ) = frd->account_id;
			WPACKETL(len + 4) = frd->char_id;
			strncpy(WPACKETP(len + 8), frd->name, 24);
			len += 32;
		}
	}
	WPACKETW(2) = len;
	SENDPACKET(sd->fd, len);

	return;
}

/*==========================================
 * Online information notification of friend list
 *------------------------------------------
 */
void clif_friend_send_online(struct map_session_data *sd, int online) {
	int i;

	for (i = 0; i < sd->friend_num; i++) {
		if (sd->friends[i].char_id) {
			struct map_session_data *fsd = map_charid2sd(sd->friends[i].char_id);
			if (fsd){

				// Notify character's friends
				WPACKETW( 0) = 0x206;
				WPACKETL( 2) = sd->status.account_id;
				WPACKETL( 6) = sd->status.char_id;
				WPACKETB(10) = !online;
				SENDPACKET(fsd->fd, packet_len_table[0x206]);

				// Notify character
				if (online) {
					WPACKETW( 0) = 0x206;
					WPACKETL( 2) = sd->friends[i].account_id;
					WPACKETL( 6) = sd->friends[i].char_id;
					WPACKETB(10) = !online;
					SENDPACKET(sd->fd, packet_len_table[0x206]);
				}
			}
		}
	}
	return;
}

/*==========================================
 * Friend list add request
 *------------------------------------------
 */
void clif_friend_add_request(const int fd, struct map_session_data *from_sd) {
//	nullpo_retv(from_sd); // checked before to call function

	WPACKETW(0) = 0x207;
	WPACKETL(2) = from_sd->bl.id;
	WPACKETL(6) = from_sd->status.char_id;
	strncpy(WPACKETP(10), from_sd->status.name, 24);
	SENDPACKET(fd, packet_len_table[0x207]);

	return;
}

/*==========================================
 * Friend list add answer
 * flag-> 0: accepted/all ok, 1: refused, 2: your friend list is full, 3: the friend list of partner is full
 *------------------------------------------
 */
void clif_friend_add_ack(const int fd, int account_id, int char_id, char* name, int flag) {
	WPACKETW(0) = 0x209;
	WPACKETW(2) = flag;
	WPACKETL(4) = account_id;
	WPACKETL(8) = char_id;
	strncpy(WPACKETP(12), name, 24);
	SENDPACKET(fd, packet_len_table[0x209]);

	return;
}

/*==========================================
 * Friend list deletion notification
 *------------------------------------------
 */
void clif_friend_del_ack(const int fd, int account_id, int char_id) { // R 020A <id1>.l <id2>.l
	WPACKETW(0) = 0x20a;
	WPACKETL(2) = account_id;
	WPACKETL(6) = char_id;
	SENDPACKET(fd, packet_len_table[0x20a]);

	return;
}

int clif_message(struct block_list *bl, char* msg) {
	unsigned short msg_len;

	nullpo_retr(0, bl);

	msg_len = strlen(msg) + 1;
	if (msg_len < 2) // message + 1
		return 0;

	WPACKETW(0) = 0x8d;
	WPACKETW(2) = msg_len + 8;
	WPACKETL(4) = bl->id;
	strcpy(WPACKETP(8), msg);

	clif_send(msg_len + 8, bl, AREA_CHAT_WOC);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_changed_dir(struct block_list *bl) {

	if (bl->type == BL_PC) {
		struct map_session_data *sd = (struct map_session_data *)bl;
		WPACKETW(0) = 0x9c;
		WPACKETL(2) = bl->id;
		WPACKETW(6) = sd->head_dir;
		WPACKETB(8) = status_get_dir(bl);
		if (sd->disguise > 23 && sd->disguise < 4001) // mob disguises [Valaris]
			clif_send(packet_len_table[0x9c], bl, AREA);
		else
			clif_send(packet_len_table[0x9c], bl, AREA_WOS);
	} else {
		WPACKETW(0) = 0x9c;
		WPACKETL(2) = bl->id;
		WPACKETW(6) = map_calc_dir(bl, bl->x, bl->y); // work for player too if necessary, but slower.
		WPACKETB(8) = status_get_dir(bl);
		clif_send(packet_len_table[0x9c], bl, AREA_WOS);
	}

	return;
}

void clif_slide(struct block_list *bl, int x, int y) {
	nullpo_retv(bl);

	WPACKETW(0) = 0x01ff;
	WPACKETL(2) = bl->id;
	WPACKETW(6) = x;
	WPACKETW(8) = y;

	clif_send(packet_len_table[0x1ff], bl, AREA);
	return;
}

/*==========================================
 * To send fame points gained [Proximus]
 *------------------------------------------
 */
void clif_fame_point(struct map_session_data *sd, unsigned char type, unsigned int points) {

	switch(type) {
		case RK_BLACKSMITH:	WPACKETW(0) = 0x21b; break;
		case RK_ALCHEMIST:  WPACKETW(0) = 0x21c; break;
		case RK_TAEKWON:    WPACKETW(0) = 0x224; break;
		case RK_PK:         WPACKETW(0) = 0x236; break;
		default: return; //invalid type
	}

	WPACKETL(2) = points;
	WPACKETL(6) = sd->status.fame_point[type];
	SENDPACKET(sd->fd, 10);

	return;
}

// ------------
// clif_parse_*
// ------------
// パケット読み取って色々操作
/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_WantToConnection(int fd, struct map_session_data *sd) { // S 0x0072 <account ID>.l <charactor ID>.l <login ID1>.l <login ID2>.l <sex>.b
	struct map_session_data *old_sd;
	int account_id;
	unsigned char packet_ver;

/*	if (sd) { // checked before
		if (battle_config.error_log)
			printf("clif_parse_WantToConnection : invalid request?\n");
		return;
	}*/

	switch(RFIFOW(fd,0)) { // 0: old, 1: 7july04, 2: 13july04, 3: 26july04, 4: 9aug04/16aug04/17aug04, 5: 6sept04, 6: 21sept04, 7: 18oct04, 8: 25oct04/08nov04, 9: 6dec04, 10: 10jan05, 11: 9may05, 12: 28jun05, 13: 4april06 (by [Yor])
	case 0x72:
		//printf("Received bytes %d with packet 0x72.\n", RFIFOREST(fd));
		if (RFIFOREST(fd) >= 39 && (RFIFOB(fd,38) == 0 || RFIFOB(fd,38) == 1)) { // 00 = Female, 01 = Male
			account_id = RFIFOL(fd,12);
			packet_ver = 2;
		} else if (RFIFOREST(fd) >= 22 && (RFIFOB(fd,21) == 0 || RFIFOB(fd,21) == 1)) { // 00 = Female, 01 = Male
			account_id = RFIFOL(fd,5);
			packet_ver = 1;
		} else { // old packet version
			account_id = RFIFOL(fd,2);
			packet_ver = 0;
		}
		break;
	case 0x7E:
		//printf("Received bytes %d with packet 0x7E.\n", RFIFOREST(fd));
		if (RFIFOREST(fd) >= 37 && (RFIFOB(fd,36) == 0 || RFIFOB(fd,36) == 1)) { // 00 = Female, 01 = Male
			account_id = RFIFOL(fd,9);
			packet_ver = 4;
		} else {
			account_id = RFIFOL(fd,12);
			packet_ver = 3;
		}
		break;
	case 0xF5:
		//printf("Received bytes %d with packet 0xF5.\n", RFIFOREST(fd));
		if (RFIFOREST(fd) >= 34 && (RFIFOB(fd,33) == 0 || RFIFOB(fd,33) == 1)) { // 00 = Female, 01 = Male
			account_id = RFIFOL(fd,7);
			packet_ver = 5;
		} else if (RFIFOREST(fd) >= 33 && (RFIFOB(fd,32) == 0 || RFIFOB(fd,32) == 1)) { // 00 = Female, 01 = Male
			account_id = RFIFOL(fd,12);
			packet_ver = 7;
		} else if (RFIFOREST(fd) >= 32 && (RFIFOB(fd,31) == 0 || RFIFOB(fd,31) == 1)) { // 00 = Female, 01 = Male
			account_id = RFIFOL(fd,10);
			packet_ver = 6;
		} else {
			// 2 possibilities
			if ((battle_config.packet_ver_flag & 256) != 0 && (battle_config.packet_ver_flag & 512) == 0) {
				account_id = RFIFOL(fd,5);
				packet_ver = 8;
			} else if ((battle_config.packet_ver_flag & 256) == 0 && (battle_config.packet_ver_flag & 512) != 0) {
				account_id = RFIFOL(fd,3);
				packet_ver = 9;
			} else { // search correct value
				// if account id and char id of packet_ver 9
				if ((battle_config.packet_ver_flag & 512) != 0 &&
				    RFIFOL(fd,3) >= 2000000 && RFIFOL(fd,3) < 1000000000 && // account id (more than 1,000,000,000 accounts?)
				    RFIFOL(fd,10) >= 150000 && RFIFOL(fd,10) < 5000000) { // char id (more than 5,000,000 characters?) // note: Old versions have id begining at 150000 (now, 500000)
					account_id = RFIFOL(fd,3);
					packet_ver = 9;
				// if account id and char id of packet_ver 8
				} else /*if ((battle_config.packet_ver_flag & 256) != 0 &&
				         RFIFOL(fd,5) >= 2000000 && RFIFOL(fd,5) < 1000000000 && // account id (more than 1,000,000,000 accounts?)
				         RFIFOL(fd,14) >= 150000 && RFIFOL(fd,14) < 5000000) */ { // char id (more than 5,000,000 characters?) // note: Old versions have id begining at 150000 (now, 500000)
					account_id = RFIFOL(fd,5);
					packet_ver = 8;
				}
			}
		}
		break;
	//case 0x9B:
	default: // 0x9B
		//printf("Received bytes %d with packet 0x9B.\n", RFIFOREST(fd));
		if (RFIFOREST(fd) >= 37 && (RFIFOB(fd,36) == 0 || RFIFOB(fd,36) == 1)) { // 00 = Female, 01 = Male
			account_id = RFIFOL(fd,9);
			packet_ver = 13;
		} else if (RFIFOREST(fd) >= 32 && (RFIFOB(fd,31) == 0 || RFIFOB(fd,31) == 1)) { // 00 = Female, 01 = Male
			// 2 possibilities
			if ((battle_config.packet_ver_flag & 1024) != 0 && (battle_config.packet_ver_flag & 4096) == 0) {
				account_id = RFIFOL(fd,3);
				packet_ver = 10;
			} else if ((battle_config.packet_ver_flag & 1024) == 0 && (battle_config.packet_ver_flag & 4096) != 0) {
				account_id = RFIFOL(fd,9);
				packet_ver = 12;
			} else { // search correct value
				// if account id and char id of packet_ver 10
				if ((battle_config.packet_ver_flag & 1024) != 0 &&
				    RFIFOL(fd,3) >= 2000000 && RFIFOL(fd,3) < 1000000000 && // account id (more than 1,000,000,000 accounts?)
				    RFIFOL(fd,12) >= 150000 && RFIFOL(fd,12) < 5000000) { // char id (more than 5,000,000 characters?) // note: Old versions have id begining at 150000 (now, 500000)
					account_id = RFIFOL(fd,3);
					packet_ver = 10;
				// if account id and char id of packet_ver 12
				} else /*if ((battle_config.packet_ver_flag & 4096) != 0 &&
				         RFIFOL(fd,9) >= 2000000 && RFIFOL(fd,9) < 1000000000 && // account id (more than 1,000,000,000 accounts?)
				         RFIFOL(fd,15) >= 150000 && RFIFOL(fd,15) < 5000000) */ { // char id (more than 5,000,000 characters?) // note: Old versions have id begining at 150000 (now, 500000)
					account_id = RFIFOL(fd,9);
					packet_ver = 12;
				}
			}
		} else { //if (RFIFOREST(fd) >= 26 && (RFIFOB(fd,25) == 0 || RFIFOB(fd,25) == 1)) { // 00 = Female, 01 = Male
			account_id = RFIFOL(fd,4);
			packet_ver = 11;
		}
		break;
	}

	// if same account already connected, we disconnect the 2 sessions
	if ((old_sd = map_id2sd(account_id)) != NULL) {
		clif_authfail_fd(fd, 2); // same id
		clif_authfail_fd(old_sd->fd, 2); // same id
		printf("clif_parse_WantToConnection: Double connection for account %d (sessions: #%d (new) and #%d (old)).\n", account_id, fd, old_sd->fd);
	} else {
		CALLOC(session[fd]->session_data, struct map_session_data, 1); // init with 0/NULL

		sd = session[fd]->session_data;
		sd->fd = fd;
		sd->packet_ver = packet_ver; // 0: old, 1: 7july04, 2: 13july04, 3: 26july04, 4: 9aug04/16aug04/17aug04, 5: 6sept04, 6: 21sept04, 7: 18oct04, 8: 25oct04/08nov04, 9: 6dec04, 10: 10jan05, 11: 9may05, 12: 28jun05, 13: 4april06 (by [Yor])

		switch(packet_ver) { // 0: old, 1: 7july04, 2: 13july04, 3: 26july04, 4: 9aug04/16aug04/17aug04, 5: 6sept04, 6: 21sept04, 7: 18oct04, 8: 25oct04/08nov04, 9: 6dec04, 10: 10jan05, 11: 9may05, 12: 28jun05, 13: 4april06 (by [Yor])
		case 13:
			pc_setnewpc(sd, account_id, RFIFOL(fd,21), RFIFOL(fd,28), RFIFOL(fd,32), RFIFOB(fd,36), fd);
			break;
		case 12:
			pc_setnewpc(sd, account_id, RFIFOL(fd,15), RFIFOL(fd,23), RFIFOL(fd,27), RFIFOB(fd,31), fd);
			break;
		case 11:
			pc_setnewpc(sd, account_id, RFIFOL(fd,9), RFIFOL(fd,17), RFIFOL(fd,21), RFIFOB(fd,25), fd);
			break;
		case 10:
			pc_setnewpc(sd, account_id, RFIFOL(fd,12), RFIFOL(fd,23), RFIFOL(fd,27), RFIFOB(fd,31), fd);
			break;
		case 9:
			pc_setnewpc(sd, account_id, RFIFOL(fd,10), RFIFOL(fd,20), RFIFOL(fd,24), RFIFOB(fd,28), fd);
			break;
		case 8:
			pc_setnewpc(sd, account_id, RFIFOL(fd,14), RFIFOL(fd,20), RFIFOL(fd,24), RFIFOB(fd,28), fd);
			break;
		case 7:
			pc_setnewpc(sd, account_id, RFIFOL(fd,18), RFIFOL(fd,24), RFIFOL(fd,28), RFIFOB(fd,32), fd);
			break;
		case 6:
			pc_setnewpc(sd, account_id, RFIFOL(fd,17), RFIFOL(fd,23), RFIFOL(fd,27), RFIFOB(fd,31), fd);
			break;
		case 5:
			pc_setnewpc(sd, account_id, RFIFOL(fd,15), RFIFOL(fd,25), RFIFOL(fd,29), RFIFOB(fd,33), fd);
			break;
		case 4:
			pc_setnewpc(sd, account_id, RFIFOL(fd,21), RFIFOL(fd,28), RFIFOL(fd,32), RFIFOB(fd,36), fd);
			break;
		case 3:
			pc_setnewpc(sd, account_id, RFIFOL(fd,18), RFIFOL(fd,24), RFIFOL(fd,28), RFIFOB(fd,32), fd);
			break;
		case 2:
			pc_setnewpc(sd, account_id, RFIFOL(fd,22), RFIFOL(fd,30), RFIFOL(fd,34), RFIFOB(fd,38), fd);
			break;
		case 1:
			pc_setnewpc(sd, account_id, RFIFOL(fd,9), RFIFOL(fd,13), RFIFOL(fd,17), RFIFOB(fd,21), fd);
			break;
		case 0:
			pc_setnewpc(sd, account_id, RFIFOL(fd,6), RFIFOL(fd,10), RFIFOL(fd,14), RFIFOB(fd,18), fd);
			break;
		}

		WPACKETL(0) = account_id; // = sd->bl.id;
		SENDPACKET(fd, 4);

		map_addiddb(&sd->bl);
		add_timer(gettick() + 40000, clif_waitauth, fd, account_id); // wait 40 sec authentification, and then disconnect player

		if (chrif_authreq(sd))
			session[fd]->eof = 1;
	}

	return;
}

#ifdef ANTIBOT_SYSTEM
/*==========================================
 * antibot actions
 *------------------------------------------
 */
void antibot_action(struct map_session_data *sd) {
	int fake_mob, i, x, y;
	int level;

	//printf("antibot_action: Account %d, char %d (%s), action: %d.\n", sd->status.account_id, sd->status.char_id, sd->status.name, sd->state.bot_flag);
	switch (sd->state.bot_flag) { // 0: no bot action done, 1: far fake player sended, 2: far fake player deleted, 3: hiden fake player sended, 4: hiden fake player deleted, 5: like 1, 6: like 2
	case 0:
	case 4:
		// if player moves -> do another time
		if (sd->walktimer != -1)
			return;
		// not immediatly
		if ((rand() % 10) != 0) // 0: no bot action done, 1: far fake player sended, 2: far fake player deleted, 3: hiden fake player sended, 4: hiden fake player deleted, 5: like 1, 6: like 2
			return;
		// send fake player (outside of view and char_id of the wisp server name)
		memset(WPACKETP(0), 0, packet_len_table[0x1d8]);
		WPACKETW( 0) = 0x1d8;
		WPACKETL( 2) = server_char_id;
		WPACKETW( 6) = DEFAULT_WALK_SPEED; // sd->speed;
		//WPACKETW( 8) = 0; // opt1
		//WPACKETW(10) = 0; // opt2
		//WPACKETW(12) = 0; // status.option
		WPACKETW(14) = (rand() % 13); // class: 0-12
		WPACKETW(16) = (rand() % 19) + 1; // hair style: 1 - 19
		//WPACKETW(18) = 0; // sd->inventory_data[sd->equip_index[9]]->view_id
		//WPACKETW(20) = 0; // sd->inventory_data[sd->equip_index[8]]->view_id
		WPACKETW(22) = sd->status.head_bottom;
		WPACKETW(24) = sd->status.head_top;
		WPACKETW(26) = sd->status.head_mid;
		WPACKETW(28) = (rand() % 8) + 1; // hair color: 1-8
		//WPACKETW(30) = 0; // clothes color
		WPACKETW(32) = sd->head_dir;
		//WPACKETL(34) = 0; // guild_id
		//WPACKETW(38) = 0; // guild_emblem_id
		//WPACKETW(40) = 0; // manner
		//WPACKETW(42) = 0; // sd->opt3
		//WPACKETB(44) = 0; // karma
		WPACKETB(45) = (rand() % 2); // sex
		// show it anywhere outside the 'normal' screen (normal user can not look it) 30 tiles are visible on screen of big resolution -> 38 (+8 for security)
		i = 0;
		do {
			if ((rand() % 2) == 0) {
				x = sd->bl.x + 38 + (rand() % AREA_SIZE); // outside area limit (not visible in normal client)
			} else {
				x = sd->bl.x - 38 - (rand() % AREA_SIZE); // outside area limit (not visible in normal client)
			}
			if ((rand() % 2) == 0) {
				y = sd->bl.y + 38 + (rand() % AREA_SIZE); // outside area limit (not visible in normal client)
			} else {
				y = sd->bl.y - 38 - (rand() % AREA_SIZE); // outside area limit (not visible in normal client)
			}
			i++;
		} while ((x < 0 || x >= map[sd->bl.m].xs || y < 0 || y >= map[sd->bl.m].ys) && i < 100);
		// don't send it if not in correct coordinates
		if (i < 100) {
			//printf("antibot_action: char coord %d,%d, fake char coord %d,%d.\n", sd->bl.x, sd->bl.y, x, y);
			WBUFPOS(WPACKETP(0), 46, x, y);
			WPACKETB(48) |= sd->dir & 0x0f;
			WPACKETB(49) = 5;
			WPACKETB(50) = 5;
			WPACKETB(51) = (rand() % 2) * 2; // dead_sit: 0: standup, 1: dead, 2: sit
			WPACKETW(52) = (rand() % 98) + 1; // level -> not do 99 to avoid 'lightning area'
			SENDPACKET(sd->fd, packet_len_table[0x1d8]);
			// update flag
			sd->state.bot_flag++; // 0: no bot action done, 1: far fake player sended, 2: far fake player deleted, 3: hiden fake player sended, 4: hiden fake player deleted, 5: like 1, 6: like 2
		// not send it -> not removed it after
		} else {
			// update flag
			sd->state.bot_flag += 2; // 0: no bot action done, 1: far fake player sended, 2: far fake player deleted, 3: hiden fake player sended, 4: hiden fake player deleted, 5: like 1, 6: like 2
		}
		break;

	case 1:
	case 5:
		// if player not walks -> wait a moment
		if (sd->walktimer == -1 && (rand() % 10) != 0)
			return;
		// just after:
		// take fake player out of screen
		clif_clearchar_id(server_char_id, 0, sd->fd);
		// update flag
		sd->state.bot_flag++; // 0: no bot action done, 1: far fake player sended, 2: far fake player deleted, 3: hiden fake player sended, 4: hiden fake player deleted, 5: like 1, 6: like 2
		break;

	case 2:
		// if player moves -> do another time
		if (sd->walktimer != -1)
			return;
		// not immediatly
		if ((rand() % 10) != 0)
			return;
		// send fake player (exactly same of the player, with HIDE option and char_id of the wisp server name)
		memset(WPACKETP(0), 0, packet_len_table[0x1d8]);
		WPACKETW( 0) = 0x1d8;
		WPACKETL( 2) = server_char_id;
		WPACKETW( 6) = DEFAULT_WALK_SPEED; // sd->speed;
		//WPACKETW( 8) = 0; // sd->opt1
		//WPACKETW(10) = 0; // sd->opt2
		WPACKETW(12) = OPTION_HIDE;
		WPACKETW(14) = sd->view_class;
		WPACKETW(16) = sd->status.hair;
		if (sd->equip_index[9] >= 0 && sd->inventory_data[sd->equip_index[9]] && sd->view_class != 22) {
			if (sd->inventory_data[sd->equip_index[9]]->view_id > 0)
				WPACKETW(18) = sd->inventory_data[sd->equip_index[9]]->view_id;
			else
				WPACKETW(18) = sd->status.inventory[sd->equip_index[9]].nameid;
//		} else {
//			WPACKETW(18) = 0;
		}
		if (sd->equip_index[8] >= 0 && sd->equip_index[8] != sd->equip_index[9] && sd->inventory_data[sd->equip_index[8]] && sd->view_class != 22) {
			if (sd->inventory_data[sd->equip_index[8]]->view_id > 0)
				WPACKETW(20) = sd->inventory_data[sd->equip_index[8]]->view_id;
			else
				WPACKETW(20) = sd->status.inventory[sd->equip_index[8]].nameid;
//		} else {
//			WPACKETW(20) = 0;
		}
		WPACKETW(22) = sd->status.head_bottom;
		WPACKETW(24) = sd->status.head_top;
		WPACKETW(26) = sd->status.head_mid;
		WPACKETW(28) = sd->status.hair_color;
		WPACKETW(30) = sd->status.clothes_color;
		WPACKETW(32) = sd->head_dir;
		WPACKETL(34) = sd->status.guild_id;
		WPACKETW(38) = sd->guild_emblem_id;
		//WPACKETW(40) = 0; // sd->status.manner
		//WPACKETW(42) = 0; // sd->opt3
		//WPACKETB(44) = 0; // sd->status.karma
		WPACKETB(45) = sd->sex;
		// show it anywhere because it's removed in next loop (normal user can not look it)
		i = 0;
		do {
			x = sd->bl.x + rand() % AREA_SIZE - AREA_SIZE / 2; // not in area limit, near the player
			y = sd->bl.y + rand() % AREA_SIZE - AREA_SIZE / 2; // not in area limit, near the player
			i++;
		} while ((x < 0 || x >= map[sd->bl.m].xs || y < 0 || y >= map[sd->bl.m].ys || (x == sd->bl.x && y == sd->bl.y)) && i < 100);
		if (i < 100) {
			WBUFPOS(WPACKETP(0), 46, x, y);
		} else {
			WBUFPOS(WPACKETP(0), 46, sd->bl.x, sd->bl.y);
		}
		WPACKETB(48) |= sd->dir & 0x0f;
		WPACKETB(49) = 5;
		WPACKETB(50) = 5;
		WPACKETB(51) = sd->state.dead_sit; // 0: standup, 1: dead, 2: sit
		WPACKETW(52) = ((level = sd->status.base_level) > battle_config.max_lv) ? battle_config.max_lv : level;
		SENDPACKET(sd->fd, packet_len_table[0x1d8]);

		// send a fake monster
		memset(WPACKETP(0), 0, packet_len_table[0x7c]);
		WPACKETW( 0) = 0x7c;
		WPACKETL( 2) = server_fake_mob_id;
		WPACKETW( 6) = sd->speed;
		//WPACKETW( 8) = 0;
		//WPACKETW(10) = 0;
		WPACKETW(12) = OPTION_HIDE;
		fake_mob = fake_mob_list[(sd->bl.m + sd->fd + sd->status.char_id) % (sizeof(fake_mob_list) / sizeof(fake_mob_list[0]))]; // never same mob
		if (mobdb_checkid(fake_mob) == 0)
			fake_mob = 1002; // poring (default)
		WPACKETW(20) = fake_mob;
		// show it anywhere because it's removed in next loop (normal user can not look it)
		i = 0;
		do {
			x = sd->bl.x + rand() % AREA_SIZE - AREA_SIZE / 2; // not in area limit, near the player
			y = sd->bl.y + rand() % AREA_SIZE - AREA_SIZE / 2; // not in area limit, near the player
			i++;
		} while ((x < 0 || x >= map[sd->bl.m].xs || y < 0 || y >= map[sd->bl.m].ys) && i < 100);
		if (i < 100) {
			WBUFPOS(WPACKETP(0), 36, x, y);
		} else {
			WBUFPOS(WPACKETP(0), 36, sd->bl.x, sd->bl.y);
		}
		SENDPACKET(sd->fd, packet_len_table[0x7c]);

		// update flag
		sd->state.bot_flag++; // 0: no bot action done, 1: far fake player sended, 2: far fake player deleted, 3: hiden fake player sended, 4: hiden fake player deleted, 5: like 1, 6: like 2
		break;

	case 3:
		// not immediatly
		if ((rand() % 10) != 0)
			return;
		// take fake mob out of screen
		clif_clearchar_id(server_fake_mob_id, 0, sd->fd);
		// take fake player out of screen
		clif_clearchar_id(server_char_id, 0, sd->fd);
		// update flag
		sd->state.bot_flag++; // 0: no bot action done, 1: far fake player sended, 2: far fake player deleted, 3: hiden fake player sended, 4: hiden fake player deleted, 5: like 1, 6: like 2
		break;
	}
	//printf("antibot_action: account %d, char %d (%s), new flag: %d.\n", sd->status.account_id, sd->status.char_id, sd->status.name, sd->state.bot_flag);

	return;
}
#endif

/*==========================================
 * 007d クライアント側マップ読み込み完了
 * map侵入時に必要なデータを全て送りつける
 *------------------------------------------
 */
void clif_parse_LoadEndAck(int fd, struct map_session_data *sd) { // S 0x007d
	struct npc_data *npc;

//	nullpo_retv(sd); // checked before to call function

	if (sd->bl.prev != NULL)
		return;

	if(sd->state.relocate != 0) {
		sd->state.relocate = 0;
		clif_changemap(sd, map[sd->bl.m].name, sd->bl.x, sd->bl.y);
		return;
	}

#ifdef ANTIBOT_SYSTEM
	// restart all anti-bot system every time you change of map
	// remove fake people from screen, because with flying wings, you can se them
	if ((sd->state.bot_flag % 2) == 1) { // 0: no bot action done, 1: far fake player sended, 2: far fake player deleted, 3: hiden fake player sended, 4: hiden fake player deleted, 5: like 1, 6: like 2
		// remove fake people from screen, because with flying wings, you can se them
		// take fake mob out of screen
		clif_clearchar_id(server_fake_mob_id, 0, sd->fd);
		// take fake player out of screen
		clif_clearchar_id(server_char_id, 0, sd->fd);
	}
	if (!battle_config.ban_bot) // if ban bot destect disabled
		sd->state.bot_flag = 6; // 0: no bot action done, 1: far fake player sended, 2: far fake player deleted, 3: hiden fake player sended, 4: hiden fake player deleted, 5: like 1, 6: like 2
	else
		sd->state.bot_flag = 0; // 0: no bot action done, 1: far fake player sended, 2: far fake player deleted, 3: hiden fake player sended, 4: hiden fake player deleted, 5: like 1, 6: like 2
#endif

	// 接続ok時
	//clif_authok();
	if (sd->npc_id)
		npc_event_dequeue(sd);

	if(sd->state.connect_new) {
		clif_skillinfoblock(sd);
		//next exp
		clif_updatestatus(sd, SP_NEXTBASEEXP);
		clif_updatestatus(sd, SP_NEXTJOBEXP);
		//skill points
		clif_updatestatus(sd, SP_SKILLPOINT);
		// param all
		clif_initialstatus(sd);
	} else {
		//stats
		clif_updatestatus(sd, SP_STR);
		clif_updatestatus(sd, SP_AGI);
		clif_updatestatus(sd, SP_VIT);
		clif_updatestatus(sd, SP_INT);
		clif_updatestatus(sd, SP_DEX);
		clif_updatestatus(sd, SP_LUK);
	}

	pc_checkitem(sd);

	//Inventory list
	clif_itemlist(sd);
	clif_equiplist(sd);

	if (pc_iscarton(sd)) { //Cart inventory info
		clif_cart_itemlist(sd);
		clif_cart_equiplist(sd);
		clif_updatestatus(sd, SP_CARTINFO);
	}
	
	party_send_movemap(sd);
	guild_send_memberinfoshort(sd, 1);

	if (battle_config.pc_invincible_time > 0) {
		if (map[sd->bl.m].flag.gvg)
			pc_setinvincibletimer(sd, battle_config.pc_invincible_time << 1);
		else
			pc_setinvincibletimer(sd, battle_config.pc_invincible_time);
	}

	map_addblock(&sd->bl); // ブロック登録
	clif_spawnpc(sd); // spawn

	// weight max , now
	clif_updatestatus(sd, SP_MAXWEIGHT);
	clif_updatestatus(sd, SP_WEIGHT);

	// pvp
	if (map[sd->bl.m].flag.pvp) {
		if (!battle_config.pk_mode) { // remove pvp stuff for pk_mode [Valaris]
			sd->pvp_timer = add_timer(gettick() + 200, pc_calc_pvprank_timer, sd->bl.id, 0);
			sd->pvp_rank = 0;
			sd->pvp_lastusers = 0;
			sd->pvp_point = 5;
		}
		clif_set0199(fd, 1);
	}
	
	// pet
	if (sd->status.pet_id > 0 && sd->pd && sd->pet.intimate > 0) {
		map_addblock(&sd->pd->bl);
		clif_spawnpet(sd->pd);
		clif_send_petdata(sd, 0, 0);
		clif_send_petdata(sd, 5, 0x14);
		clif_send_petstatus(sd);
	}

	if (sd->state.connect_new) {
		struct guild_castle *gc = guild_mapname2gc(map[sd->bl.m].name);
		sd->state.connect_new = 0;
		if (sd->status.class != sd->view_class)
			clif_changelook(&sd->bl, LOOK_BASE, sd->view_class);
		if (sd->status.pet_id > 0 && sd->pd && sd->pet.intimate > 900)
			clif_pet_emotion(sd->pd, (sd->pd->class - 100) * 100 + 50 + pet_hungry_val(sd));

/*		Stop players from spawning inside castles [Valaris]	*/
		if (gc)
			pc_setpos(sd, sd->status.save_point.map, sd->status.save_point.x, sd->status.save_point.y, 2, 0);
/*		End Addition [Valaris]	*/
	}

	if (map[sd->bl.m].flag.gvg)
		clif_set0199(fd, 3);
 	else if (map[sd->bl.m].flag.gvg_dungeon)
 		clif_set0199(sd->fd, 1);
	
	// view equipment item
	clif_changelook(&sd->bl, LOOK_WEAPON, 0);
	if (battle_config.save_clothcolor && sd->status.clothes_color > 0)
		clif_changelook(&sd->bl, LOOK_CLOTHES_COLOR, sd->status.clothes_color);

	if (sd->status.hp < sd->status.max_hp >> 2 && pc_checkskill(sd, SM_AUTOBERSERK) > 0 &&
	    (sd->sc_data[SC_PROVOKE].timer == -1 || sd->sc_data[SC_PROVOKE].val2 == 0))
		// オートバーサーク発動
		status_change_start(&sd->bl, SC_PROVOKE, 10, 1, 0, 0, 0, 0);

	if (battle_config.muting_players && sd->status.manner < 0)
		status_change_start(&sd->bl, SC_NOCHAT, 0, 0, 0, 0, 0, 0);

	if (sd->sc_count) {
		if (sd->sc_data[SC_TRICKDEAD].timer != -1)
			status_change_end(&sd->bl, SC_TRICKDEAD, -1);
		if (sd->sc_data[SC_SIGNUMCRUCIS].timer != -1 && !battle_check_undead(7, sd->def_ele))
			status_change_end(&sd->bl, SC_SIGNUMCRUCIS, -1);
	}
	
	if (sd->special_state.infinite_endure && sd->sc_data[SC_ENDURE].timer == -1)
		status_change_start(&sd->bl, SC_ENDURE, 10, 1, 0, 0, 0, 0);
	/*
	for(i = 0; i < MAX_INVENTORY; i++) {
		if (sd->status.inventory[i].equip && sd->status.inventory[i].equip & 0x0002 && sd->status.inventory[i].attribute == 1)
			status_change_start(&sd->bl, SC_BROKNWEAPON, 0, 0, 0, 0, 0, 0);
		if (sd->status.inventory[i].equip && sd->status.inventory[i].equip & 0x0010 && sd->status.inventory[i].attribute == 1)
			status_change_start(&sd->bl, SC_BROKNARMOR, 0, 0, 0, 0, 0, 0);
	}*/

	if ((npc = npc_name2id(script_config.loadmap_event_name))) {
		if(npc->bl.m == sd->bl.m) {
			run_script(npc->u.scr.script, 0, sd->bl.id, npc->bl.id); // PCLoadMapEvent
	#ifdef __DEBUG
			printf("Event '" CL_WHITE "%s" CL_RESET "' executed.\n", script_config.loadmap_event_name);
	#endif
		}
	}

	map_foreachinarea(clif_getareachar, sd->bl.m, sd->bl.x - AREA_SIZE, sd->bl.y - AREA_SIZE, sd->bl.x + AREA_SIZE, sd->bl.y + AREA_SIZE, 0, sd);

	// display hp of the player if necessary
	if(battle_config.display_hpmeter != 0)
		clif_hpmeter(sd, NULL); // NULL: send to any people, other: send to only 1 player

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_TickSend(int fd, struct map_session_data *sd) { // S 0x007e <client tick>.l
//	nullpo_retv(sd); // checked before to call function

	switch (sd->packet_ver) { // 0: old, 1: 7july04, 2: 13july04, 3: 26july04, 4: 9aug04/16aug04/17aug04, 5: 6sept04, 6: 21sept04, 7: 18oct04, 8: 25oct04/08nov04, 9: 6dec04, 10: 10jan05, 11: 9may05, 12: 28jun05, 13: 4april06 (by [Yor])
	case 13:
		sd->client_tick = RFIFOL(fd,7);
		break;
	case 12:
		sd->client_tick = RFIFOL(fd,9);
		break;
	case 11:
		sd->client_tick = RFIFOL(fd,4);
		break;
	case 10:
		sd->client_tick = RFIFOL(fd,5);
		break;
	case 9:
		sd->client_tick = RFIFOL(fd,3);
		break;
	case 8:
		sd->client_tick = RFIFOL(fd,5);
		break;
	case 7:
		sd->client_tick = RFIFOL(fd,6);
		break;
	case 6:
		sd->client_tick = RFIFOL(fd,10);
		break;
	case 5:
		sd->client_tick = RFIFOL(fd,7);
		break;
	case 4:
		sd->client_tick = RFIFOL(fd,9);
		break;
	case 3:
		sd->client_tick = RFIFOL(fd,6);
		break;
	default: // old version by default (and version 1 + 2)
		sd->client_tick = RFIFOL(fd,2);
		break;
	}
/*
	if (battle_config.speed_hack_info_GM_level < 100) {
		// tick changed
		unsigned int tick = gettick();
		if (sd->client_tick < sd->first_client_tick || tick < sd->tick_at_start || sd->first_client_tick == 0) {
			sd->first_client_tick = sd->client_tick; // to check speed hack
			sd->tick_at_start     = tick; // to check speed hack
			sd->first_check_done  = 0; // to check speed hack (to avoid lag when we set value, so don't considere first check)
			//clif_servertick(sd);
			WPACKETW(0) = 0x7f;
			WPACKETL(2) = tick;
			SENDPACKET(fd, packet_len_table[0x7f]);
		} else {
			double diff_tick, diff_tick2;
			diff_tick  = (double)(sd->client_tick - sd->first_client_tick);
			diff_tick2 = (double)(tick - sd->tick_at_start);
			// check speed hack usage (if lesser value, no problem (slower player, player will have lag or lag of server))
			if (diff_tick - 5000. > diff_tick2 && (diff_tick - diff_tick2) / diff_tick2 > .1) { // with a marge of 5 sec and more than 10%
				// if it just after to have set the reference, don't display message
				if (!sd->first_check_done || diff_tick2 < 15000.) { // don't display in the fisrt 15 seconds (to parse all cumulativ lags of client). note: tick is sended every 12 sec
					sd->first_client_tick = sd->client_tick; // to check speed hack
					sd->tick_at_start     = tick; // to check speed hack
					sd->first_check_done  = 1; // to check speed hack (to avoid lag when we set value, so don't considere first check)
				} else {
					char message_to_gm[MAX_MSG_LEN + 100]; // max size of msg_txt + security (char name, char id, etc...) (100)
					sprintf(message_to_gm, "Possible speed hack usage with player '%s' (account: %d; GM lvl: %d), speed: +%0.02lf%% of normal.", sd->status.name, sd->status.account_id, sd->GM_level, (diff_tick - diff_tick2) / diff_tick2 * 100.);
					intif_wis_message_to_gm(wisp_server_name, battle_config.speed_hack_info_GM_level, message_to_gm);
					//printf("Speed hack usage on connection %d: player '%s' (account: %d; GM lvl: %d), speed: +%0.02lf%% of normal.\n", fd, sd->status.name, sd->status.account_id, sd->GM_level, (diff_tick - diff_tick2) / diff_tick2 * 100.);
				}
			}
			//printf("Elapsed tick: %lu (Elapsed server tick: %lu).\n", (unsigned long)diff_tick, (unsigned long)diff_tick2);
			// init again (every 10 min) first_client_tick to avoid player that connect a long time and after use speed hack
			if (diff_tick > (10 * 60 * 1000) && diff_tick2 > (10 * 60 * 1000)) { // check client AND server tick to avoid problem with some possible lag (on client OR server)
				sd->first_client_tick = sd->client_tick; // to check speed hack
				sd->tick_at_start     = tick; // to check speed hack
				sd->first_check_done  = 0; // to check speed hack (to avoid lag when we set value, so don't considere first check)
				//clif_servertick(sd);
				WPACKETW(0) = 0x7f;
				WPACKETL(2) = tick;
				SENDPACKET(fd, packet_len_table[0x7f]);
			} else {
				// send client diff of client tick -> client can not adjust tick with server tick
				//clif_servertick(sd);
				WPACKETW(0) = 0x7f;
				WPACKETL(2) = (sd->tick_at_start + (unsigned int)diff_tick);
				SENDPACKET(fd, packet_len_table[0x7f]);
			}
		}
	} else {*/
		//clif_servertick(sd);
		WPACKETW(0) = 0x7f;
		WPACKETL(2) = gettick();
		SENDPACKET(fd, packet_len_table[0x7f]);
	//}

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_WalkToXY(int fd, struct map_session_data *sd) { // S 0x0085 <X_Y>.3B
	int x, y;

//	nullpo_retv(sd); // checked before to call function

	if (pc_isdead(sd)) {
		clif_clearchar_area(&sd->bl, 1);
		return;
	}

	if (pc_issit(sd)) // client can not send walk packet if player is sit down. But with hacker and latency... do a check
		return; // to avoid: have regeneration of a sitting man, but not be sit down.

	if (sd->bl.prev == NULL || sd->npc_id != 0|| sd->vender_id != 0 || sd->trade_partner != 0 || sd->chatID != 0)
		return;

	if (sd->skilltimer != -1 && pc_checkskill(sd, SA_FREECAST) <= 0) // フリーキャスト
		return;

	if(pc_cant_move(sd))
		return;

	if((sd->status.option&2) && pc_checkskill(sd, RG_TUNNELDRIVE) <= 0)
		return;

	pc_delinvincibletimer(sd);

	pc_stopattack(sd);

	switch (sd->packet_ver) { // 0: old, 1: 7july04, 2: 13july04, 3: 26july04, 4: 9aug04/16aug04/17aug04, 5: 6sept04, 6: 21sept04, 7: 18oct04, 8: 25oct04/08nov04, 9: 6dec04, 10: 10jan05, 11: 9may05, 12: 28jun05, 13: 4april06 (by [Yor])
	case 13:
		x = RFIFOB(fd,12) * 4 + (RFIFOB(fd,13) >> 6);
		y = ((RFIFOB(fd,13) & 0x3f) << 4) + (RFIFOB(fd,14) >> 4);
		break;
	case 12:
		x = RFIFOB(fd,8) * 4 + (RFIFOB(fd,9) >> 6);
		y = ((RFIFOB(fd,9) & 0x3f) << 4) + (RFIFOB(fd,10) >> 4);
		break;
	case 11:
		x = RFIFOB(fd,5) * 4 + (RFIFOB(fd,6) >> 6);
		y = ((RFIFOB(fd,6) & 0x3f) << 4) + (RFIFOB(fd,7) >> 4);
		break;
	case 10:
		x = RFIFOB(fd,10) * 4 + (RFIFOB(fd,11) >> 6);
		y = ((RFIFOB(fd,11) & 0x3f) << 4) + (RFIFOB(fd,12) >> 4);
		break;
	case 9:
		x = RFIFOB(fd,4) * 4 + (RFIFOB(fd,5) >> 6);
		y = ((RFIFOB(fd,5) & 0x3f) << 4) + (RFIFOB(fd,6) >> 4);
		break;
	case 8:
		x = RFIFOB(fd,3) * 4 + (RFIFOB(fd,4) >> 6);
		y = ((RFIFOB(fd,4) & 0x3f) << 4) + (RFIFOB(fd,5) >> 4);
		break;
	case 7:
		x = RFIFOB(fd,3) * 4 + (RFIFOB(fd,4) >> 6);
		y = ((RFIFOB(fd,4) & 0x3f) << 4) + (RFIFOB(fd,5) >> 4);
		break;
	case 6:
		x = RFIFOB(fd,11) * 4 + (RFIFOB(fd,12) >> 6);
		y = ((RFIFOB(fd,12) & 0x3f) << 4) + (RFIFOB(fd,13) >> 4);
		break;
	case 5:
		x = RFIFOB(fd,6) * 4 + (RFIFOB(fd,7) >> 6);
		y = ((RFIFOB(fd,7) & 0x3f) << 4) + (RFIFOB(fd,8) >> 4);
		break;
	case 4:
		x = RFIFOB(fd,12) * 4 + (RFIFOB(fd,13) >> 6);
		y = ((RFIFOB(fd,13) & 0x3f) << 4) + (RFIFOB(fd,14) >> 4);
		break;
	case 3:
		x = RFIFOB(fd,3) * 4 + (RFIFOB(fd,4) >> 6);
		y = ((RFIFOB(fd,4) & 0x3f) << 4) + (RFIFOB(fd,5) >> 4);
		break;
	case 2:
		x = RFIFOB(fd,6) * 4 + (RFIFOB(fd,7) >> 6);
		y = ((RFIFOB(fd,7) & 0x3f) << 4) + (RFIFOB(fd,8) >> 4);
		break;
	case 1:
		x = RFIFOB(fd,5) * 4 + (RFIFOB(fd,6) >> 6);
		y = ((RFIFOB(fd,6) & 0x3f) << 4) + (RFIFOB(fd,7) >> 4);
		break;
	default: // old version by default
		x = RFIFOB(fd,2) * 4 + (RFIFOB(fd,3) >> 6);
		y = ((RFIFOB(fd,3) & 0x3f) << 4) + (RFIFOB(fd,4) >> 4);
		break;
	}
	// if player don't click always the same position
	if (sd->to_x != x || sd->to_y != y)
		pc_walktoxy(sd, x, y);

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_QuitGame(int fd, struct map_session_data *sd) { // S 0x018a ?.w
	unsigned int tick = gettick();
	struct skill_unit_group* sg;

//	nullpo_retv(sd); // checked before to call function

	if ((!pc_isdead(sd) && (sd->opt1 || (sd->opt2 && !(night_flag == 1 && (sd->opt2 & STATE_BLIND) == STATE_BLIND)))) ||
	    sd->skilltimer != -1 ||
	    (DIFF_TICK(tick, sd->canact_tick) < 0) ||
	    (sd->sc_data[SC_DANCING].timer!=-1 && sd->sc_data[SC_DANCING].val4 && (sg = (struct skill_unit_group *)sd->sc_data[SC_DANCING].val2) && sg->src_id == sd->bl.id)) {
		WPACKETW(0) = 0x18b;
		WPACKETW(2) = 1; // flag= 0: success, 1: failure (please wait 15 sec...)
		SENDPACKET(fd, packet_len_table[0x18b]);
		return;
	}

	// Rovert's prevent logout option fixed [Valaris]
	if ((battle_config.prevent_logout && (tick - sd->canlog_tick) >= 10000) || (!battle_config.prevent_logout)) {
		clif_setwaitclose(fd);
		WPACKETW(0) = 0x18b;
		WPACKETW(2) = 0; // flag= 0: success, 1: failure (please wait 15 sec...)
		SENDPACKET(fd, packet_len_table[0x18b]);
	} else {
		WPACKETW(0) = 0x18b;
		WPACKETW(2) = 1; // flag= 0: success, 1: failure (please wait 15 sec...)
		SENDPACKET(fd, packet_len_table[0x18b]);
	}

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void check_fake_id(struct map_session_data *sd, int target_id) {
#ifdef ANTIBOT_SYSTEM	//Lagged clients could request names of already gone mobs/players = misban!	
	// if player asks for the fake player (only bot and modified client can see a hiden player)
	if (target_id == server_char_id) {
		char message_to_gm[MAX_MSG_LEN + 100]; // max size of msg_txt + security (char name, char id, etc...) (100)
		// to avoid some possible problem with players, don't accept detection if player is not in the process of detection
		// note: always detect if it's for a HIDDEN id.
		if (sd->state.bot_flag == 0 || sd->state.bot_flag == 2 || sd->state.bot_flag == 6) // 0: no bot action done, 1: far fake player sended, 2: far fake player deleted, 3: hiden fake player sended, 4: hiden fake player deleted, 5: like 1, 6: like 2
			return;

		sprintf(message_to_gm, msg_txt(622), sd->status.name, sd->status.account_id); // Character '%s' (account: %d) try to use a bot (it tries to detect a fake player).
		intif_wis_message_to_gm(wisp_server_name, battle_config.hack_info_GM_level, message_to_gm);
		// if we block people
		if (battle_config.ban_bot < 0) {
			chrif_char_ask_name(-1, sd->status.name, 1, 0, 0, 0, 0, 0, 0); // type: 1 - block
			clif_setwaitclose(sd->fd); // forced to disconnect because of the hack
			// message about the ban
			sprintf(message_to_gm, msg_txt(540)); //  This player has been definitivly blocked.
		// if we ban people
		} else if (battle_config.ban_bot > 0) {
			chrif_char_ask_name(-1, sd->status.name, 2, 0, 0, 0, 0, battle_config.ban_bot, 0); // type: 2 - ban (year, month, day, hour, minute, second)
			clif_setwaitclose(sd->fd); // forced to disconnect because of the hack
			// message about the ban
			sprintf(message_to_gm, msg_txt(507), battle_config.ban_bot); //  This player has been banned for %d minute(s).
		} else { // impossible to display: we don't send fake player if battle_config.ban_bot is == 0
			// message about the ban
			sprintf(message_to_gm, msg_txt(508)); //  This player hasn't been banned (Ban option is disabled).
		}
		intif_wis_message_to_gm(wisp_server_name, battle_config.hack_info_GM_level, message_to_gm);
		// send this info cause the bot ask until get an answer, damn spam
		WPACKETW(0) = 0x95;
		WPACKETL(2) = server_char_id;
		strncpy(WPACKETP(6), sd->status.name, 24);
		SENDPACKET(sd->fd, packet_len_table[0x95]);
		// take fake player out of screen
		clif_clearchar_id(server_char_id, 0, sd->fd);
		// take fake mob out of screen
		clif_clearchar_id(server_fake_mob_id, 0, sd->fd);
		// update flag
		sd->state.bot_flag = 6; // 0: no bot action done, 1: far fake player sended, 2: far fake player deleted, 3: hiden fake player sended, 4: hiden fake player deleted, 5: like 1, 6: like 2
		// if player asks for the fake mob (only bot and modified client can see a hiden mob)

	} else if (target_id == server_fake_mob_id) {
		int fake_mob;
		char message_to_gm[MAX_MSG_LEN + 100]; // max size of msg_txt + security (char name, char id, etc...) (100)
		sprintf(message_to_gm, msg_txt(623), sd->status.name, sd->status.account_id); // Character '%s' (account: %d) try to use a bot (it tries to detect a fake mob).
		intif_wis_message_to_gm(wisp_server_name, battle_config.hack_info_GM_level, message_to_gm);
		// if we block people
		if (battle_config.ban_bot < 0) {
			chrif_char_ask_name(-1, sd->status.name, 1, 0, 0, 0, 0, 0, 0); // type: 1 - block
			clif_setwaitclose(sd->fd); // forced to disconnect because of the hack
			// message about the ban
			sprintf(message_to_gm, msg_txt(540)); //  This player has been definitivly blocked.
		// if we ban people
		} else if (battle_config.ban_bot > 0) {
			chrif_char_ask_name(-1, sd->status.name, 2, 0, 0, 0, 0, battle_config.ban_bot, 0); // type: 2 - ban (year, month, day, hour, minute, second)
			clif_setwaitclose(sd->fd); // forced to disconnect because of the hack
			// message about the ban
			sprintf(message_to_gm, msg_txt(507), battle_config.ban_bot); //  This player has been banned for %d minute(s).
		} else { // impossible to display: we don't send fake player if battle_config.ban_bot is == 0
			// message about the ban
			sprintf(message_to_gm, msg_txt(508)); //  This player hasn't been banned (Ban option is disabled).
		}
		intif_wis_message_to_gm(wisp_server_name, battle_config.hack_info_GM_level, message_to_gm);
		// send this info cause the bot ask until get an answer, damn spam
		WPACKETW(0) = 0x95;
		WPACKETL(2) = server_fake_mob_id;
		fake_mob = fake_mob_list[(sd->bl.m + sd->fd + sd->status.char_id) % (sizeof(fake_mob_list) / sizeof(fake_mob_list[0]))]; // never same mob
		if (mobdb_checkid(fake_mob) == 0)
			fake_mob = 1002; // poring (default)
		strncpy(WPACKETP(6), mob_db[fake_mob].name, 24);
		SENDPACKET(sd->fd, packet_len_table[0x95]);
		// take fake mob out of screen
		clif_clearchar_id(server_fake_mob_id, 0, sd->fd);
		// take fake player out of screen
		clif_clearchar_id(server_char_id, 0, sd->fd);
		// update flag
		sd->state.bot_flag = 6; // 0: no bot action done, 1: far fake player sended, 2: far fake player deleted, 3: hiden fake player sended, 4: hiden fake player deleted, 5: like 1, 6: like 2
	}
#endif

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_GetCharNameRequest(int fd, struct map_session_data *sd) { // S 0x0094 <ID>.l
	struct block_list *bl;
	int account_id;

//	nullpo_retv(sd); // checked before to call function

	switch (sd->packet_ver) { // 0: old, 1: 7july04, 2: 13july04, 3: 26july04, 4: 9aug04/16aug04/17aug04, 5: 6sept04, 6: 21sept04, 7: 18oct04, 8: 25oct04/08nov04, 9: 6dec04, 10: 10jan05, 11: 9may05, 12: 28jun05, 13: 4april06 (by [Yor])
	case 13:
		account_id = RFIFOL(fd,8);
		break;
	case 12:
		account_id = RFIFOL(fd,4);
		break;
	case 11:
		account_id = RFIFOL(fd,7);
		break;
	case 10:
		account_id = RFIFOL(fd,4);
		break;
	case 9:
		account_id = RFIFOL(fd,9);
		break;
	case 8:
		account_id = RFIFOL(fd,6);
		break;
	case 7:
		account_id = RFIFOL(fd,11);
		break;
	case 6:
		account_id = RFIFOL(fd,6);
		break;
	case 5:
		account_id = RFIFOL(fd,10);
		break;
	case 4:
		account_id = RFIFOL(fd,8);
		break;
	case 3:
		account_id = RFIFOL(fd,11);
		break;
	default: // old version by default (+ packet version 1 and 2)
		account_id = RFIFOL(fd,2);
		break;
	}
	bl = map_id2bl(account_id);

	if (bl == NULL) {
		// if player asks for the fake mob/player (only bot and modified client can see a hiden mob/player)
		check_fake_id(sd, account_id);
		return;
	}

	switch(bl->type) {
	case BL_PC:
	  {
		struct map_session_data *ssd = (struct map_session_data *)bl;
		struct party *p = NULL;
		struct guild *g = NULL;
	
		if(ssd->status.party_id > 0) //for party name
			p = party_search(ssd->status.party_id);
		if(ssd->status.guild_id > 0) //for guild name
			g = guild_search(ssd->status.guild_id);
			
		if (p || g) {
			memset(WPACKETP(0), 0, packet_len_table[0x195]);
			WPACKETW(0) = 0x195;
			WPACKETL(2) = account_id;
			strncpy(WPACKETP( 6), ssd->status.name, 24);
			
			if (p != NULL)
				strncpy(WPACKETP(30), p->name, 24);
			if (g != NULL) {
				int i;
				strncpy(WPACKETP(54), g->name,24);
				// ギルド所属ならパケット0195を返す
				for(i = 0; i < g->max_member; i++) {
					// just check char_id (char_id is unique)
					if (g->member[i].char_id == ssd->status.char_id) {
						int ps = g->member[i].position;
						if (ps >= 0 && ps < MAX_GUILDPOSITION)
							strncpy(WPACKETP(78), g->position[ps].name, 24);
						break;
					}
				}
			}
			SENDPACKET(fd, packet_len_table[0x195]);
		} else {
			WPACKETW(0) = 0x95;
			WPACKETL(2) = account_id;
			strncpy(WPACKETP(6), ssd->status.name, 24);
			SENDPACKET(fd, packet_len_table[0x95]);
		}
	  }
		break;
	case BL_PET:
		WPACKETW(0) = 0x95;
		WPACKETL(2) = account_id;
		strncpy(WPACKETP(6), ((struct pet_data*)bl)->name, 24);
		SENDPACKET(fd, packet_len_table[0x95]);
		break;
	case BL_NPC:
		WPACKETW(0) = 0x95;
		WPACKETL(2) = account_id;
		strncpy(WPACKETP(6), ((struct npc_data*)bl)->name, 24);
		SENDPACKET(fd, packet_len_table[0x95]);
		break;
	case BL_MOB:
	  {
		struct guild *g;
		struct guild_castle *gc;
		struct mob_data *md = (struct mob_data *)bl;

//		if (md == NULL) // not: nullpo_retv(md); --> if mob was spawned and killed, it's not an error (not display a message)
//			return;

 		if (md->guild_id && (g = guild_search(md->guild_id)) != NULL && // Guilds' gardians & emperiums, otherwize = 0
	    (gc = guild_mapname2gc(map[md->bl.m].name))) {
			memset(WPACKETP(0), 0, packet_len_table[0x195]);
			WPACKETW( 0) = 0x195;
			WPACKETL( 2) = account_id;
			strncpy(WPACKETP( 6), md->name, 24);
			if (battle_config.show_mob_hp > 1) { // display HP only if asked for
				//sprintf(WPACKETP(30), "hp: %d/%d", md->hp, mob_db[md->class].max_hp); // maximum is variable -> don't show max HP
				sprintf(WPACKETP(30), "Actual hp: %d", md->hp);
			}
			strncpy(WPACKETP(54), g->name, 24);
			strncpy(WPACKETP(78), gc->castle_name, 24);
			SENDPACKET(fd, packet_len_table[0x195]);
		} else if (battle_config.show_mob_hp) {
			memset(WPACKETP(0), 0, packet_len_table[0x195]);
			WPACKETW( 0) = 0x195;
			WPACKETL( 2) = account_id;
			strncpy(WPACKETP( 6), md->name, 24);
			//memset(WPACKETP(30), 0, 24);
			//memset(WPACKETP(54), 0, 24);
			WPACKETB(54) = ' '; // to force display with new clients
			sprintf(WPACKETP(78), "hp: %d/%d", md->hp, mob_db[md->class].max_hp);
			SENDPACKET(fd, packet_len_table[0x195]);
		} else {
			WPACKETW(0) = 0x95;
			WPACKETL(2) = account_id;
			strncpy(WPACKETP(6), md->name, 24);
			SENDPACKET(fd, packet_len_table[0x95]);
		}
	  }
		break;
	default:
		if (battle_config.error_log)
			printf("clif_parse_GetCharNameRequest : bad type %d(%d)\n", bl->type, account_id);
		break;
	}

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_GlobalMessage(int fd, struct map_session_data *sd) { // S 0x008c <len>.w <str>.?B
	char *message;
	char *message_to_gm;
	char *p_message;
	int i, packet_size, message_size, name_length;
	int min_length, max_length;

//	nullpo_retv(sd); // checked before to call function

	if (sd->sc_data[SC_BERSERK].timer != -1 || //バーサーク時は会話も不可
	    sd->sc_data[SC_NOCHAT].timer != -1) //チャット禁止
		return;

	// get sizes
	packet_size = RFIFOW(fd,2);
	message_size = packet_size - 4;

	// check size of message
	if (message_size < 1)
		return;

	// get message
	CALLOC(message, char, message_size + 1); // +1 for null terminated if necessary
	strncpy(message, RFIFOP(fd,4), message_size);
	// remove final NULL from message_size
	i = strlen(message);
	if (i < message_size) // note: normal client always sends final NULL, so i = message_size - 1. If more, it's abnormal
		message_size = i;

	min_length = 1; // normal client can not sends (void) message if it's not a skill
	max_length = battle_config.max_message_length;
	if (sd->skillid == BA_FROSTJOKE || sd->skillid == DC_SCREAM) { // these skills can sends message
		for(i = 0; i < MAX_SKILLTIMERSKILL; i++) {
			if (sd->skilltimerskill[i].timer != -1 &&
			    (sd->skilltimerskill[i].skill_id == BA_FROSTJOKE || sd->skilltimerskill[i].skill_id == DC_SCREAM)) {
				min_length = 0; // can send (void) message
				max_length = battle_config.max_global_message_length; // not use general configuration message length
				break;
			}
		}
	}

	// check size of message again
	if (message_size < 1) { // send a void message?
		FREE(message);
		return;
	} else if (message_size > max_length) { // more than the authorized?
		printf("Possible hack on global message (normal message): character '%s' (account: %d) tries to send a big message (%d characters).\n", sd->status.name, sd->status.account_id, message_size);
		CALLOC(message_to_gm, char, message_size + MAX_MSG_LEN + 100); // message_size + max size of msg_txt + security (char name, char id, etc...) (100)
		// information is sended to all online GM
		sprintf(message_to_gm, msg_txt(642), sd->status.name, sd->status.account_id, message_size); // Possible hack on global message (normal message): character '%s' (account: %d) tries to send a big message (%d characters).
		intif_wis_message_to_gm(wisp_server_name, battle_config.hack_info_GM_level, message_to_gm);
		intif_wis_message_to_gm(wisp_server_name, battle_config.hack_info_GM_level, msg_txt(641)); //  This player hasn't been banned.
		FREE(message_to_gm);
		FREE(message);
		return;
	}

	//printf("clif_parse_GlobalMessage: message: '%s'.\n", message); // structure of message: <player_name> : <message>
	name_length = strlen(sd->status.name);
	p_message = message + name_length;
	if (strncmp(message, sd->status.name, name_length) != 0 || // check player name
	    message_size < name_length + 3 + min_length || // check void message (at least 1 char) - normal client refuse to send void message
	    *p_message != ' ' || *(p_message + 1) != ':' || *(p_message + 2) != ' ') { // check ' : '
		CALLOC(message_to_gm, char, message_size + MAX_MSG_LEN + 100); // message_size + max size of msg_txt + security (char name, char id, etc...) (100)
		printf("Hack on global message: character '%s' (account: %d), use an other name to send a (normal) message.\n", sd->status.name, sd->status.account_id);

		// information is sended to all online GM
		sprintf(message_to_gm, msg_txt(504), sd->status.name, sd->status.account_id); // Hack on global message (normal message): character '%s' (account: %d) uses an other name.
		intif_wis_message_to_gm(wisp_server_name, battle_config.hack_info_GM_level, message_to_gm);
		if (message[0] == '\0')
			strcpy(message_to_gm, msg_txt(505)); //  This player sends a void name and a void message.
		else
			sprintf(message_to_gm, msg_txt(506), message); //  This player sends (name:message): '%s'.
		intif_wis_message_to_gm(wisp_server_name, battle_config.hack_info_GM_level, message_to_gm);
		// if we block people
		if (battle_config.ban_spoof_namer < 1) {
			chrif_char_ask_name(-1, sd->status.name, 1, 0, 0, 0, 0, 0, 0); // type: 1 - block
			clif_setwaitclose(sd->fd); // forced to disconnect because of the hack
			// message about the ban
			sprintf(message_to_gm, msg_txt(540)); //  This player has been definitivly blocked.
		// if we ban people
		} else if (battle_config.ban_spoof_namer > 0) {
			chrif_char_ask_name(-1, sd->status.name, 2, 0, 0, 0, 0, battle_config.ban_spoof_namer, 0); // type: 2 - ban (year, month, day, hour, minute, second)
			clif_setwaitclose(fd); // forced to disconnect because of the hack
			// message about the ban
			sprintf(message_to_gm, msg_txt(507), battle_config.ban_spoof_namer); //  This player has been banned for %d minute(s).
		} else {
			// message about the ban
			sprintf(message_to_gm, msg_txt(508)); //  This player hasn't been banned (Ban option is disabled).
		}
		intif_wis_message_to_gm(wisp_server_name, battle_config.hack_info_GM_level, message_to_gm);
		FREE(message_to_gm);
		// but for the hacker, we display on his screen (he sees/looks no difference).
	} else {
		// check GM command
		if (is_atcommand(fd, sd, message, sd->GM_level) != AtCommand_None) {
			FREE(message);
			return;
		}

		// remove space at start for check
		p_message = p_message + 3; // position to message (+ ' : ')
		while(*p_message && isspace(*p_message))
			p_message++;

		// check bad words (after is_atcommand check, because player can have a bad name)
		if (check_bad_word(p_message, message_size, sd)) {
			FREE(message);
			return;
		}

		// send message to others
		memset(WPACKETP(0), 0, message_size + 9); // 8 + len of message + 1 (NULL)
		WPACKETW(0) = 0x8d;
		WPACKETW(2) = message_size + 9; // 8 + len of message + 1 (NULL)
		WPACKETL(4) = sd->bl.id;
		memcpy(WPACKETP(8), message, message_size);
		clif_send(message_size + 9, &sd->bl, sd->chatID ? CHAT_WOS : AREA_CHAT_WOC);
	}

	// send back message to the speaker
	memcpy(WPACKETP(0), RFIFOP(fd,0), packet_size);
	WPACKETW(0) = 0x8e;
	SENDPACKET(fd, packet_size);

	// Super Novice Angel
	if(pc_calc_base_job2(sd->status.class) == JOB_SUPER_NOVICE)
	{
		int next = pc_nextbaseexp(sd);

		if(next <= 0)
			next = sd->status.base_exp;

		if((next > 0 && (sd->status.base_exp * 100 / next) % 10 == 0) || sd->status.base_level >= 99)
		{
			char *message2;
			// lower case for message
			for(i = 0; p_message[i]; i++)
				p_message[i] = tolower((unsigned char)p_message[i]); // tolower needs unsigned char
			CALLOC(message2, char, MAX_MSG_LEN + 100); // max size of msg_txt + security (char name, char id, etc...) (100)
			switch(sd->state.snovice_flag) {
			// first message
			case 0:
				strcpy(message2, msg_txt(605)); // Guardian Angel, can you hear my voice? ^^;
				// message is already lowered when loading
				if(strstr(p_message, message2))
					sd->state.snovice_flag = 1;
				break;
			// second message
			case 1:
				sprintf(message2, msg_txt(606), sd->status.name); // My name is %s, and I'm a Super Novice~
				for(i = 0; message2[i]; i++)
					message2[i] = tolower((unsigned char)message2[i]); // tolower needs unsigned char
				if(strstr(p_message, message2))
					sd->state.snovice_flag = 2;
				break;
			// third message
			case 2:
				strcpy(message2, msg_txt(607)); // Please help me~ T.T
				// message is already lowered when loading
				if(strstr(p_message, message2))
				{
					// explosion spirits level 17 because 'critical_change = 500% = 75 + 25 * 17' [Harbin]
					status_change_start(&sd->bl, SkillStatusChangeTable[MO_EXPLOSIONSPIRITS], 17, 0, 0, 0, skill_get_time(MO_EXPLOSIONSPIRITS, 1), 0);

					// no spirit spheres anymore? [Harbin]
					// for(i = 0; i < 5; i++)
					//	 pc_addspiritball(sd, skill_get_time(MO_CALLSPIRITS, 1), 5);

					sd->state.snovice_flag = 0;
				}
				break;
			}
			FREE(message2);
		}
	}

	FREE(message);

	return;
}

/*==========================================
 * /mm /mapmove (as @rura GM command)
 *------------------------------------------
 */
void clif_parse_MapMove(int fd, struct map_session_data *sd) { // S 0x140 <map_name>.16B <X>.W <Y>.W
	char output[70]; // 24 (name) + 1 + 1 + 1 + 5 (@rura) + 1 + 16 + 1 + 6 (-32767) + 1 + 6 + NULL = 64
	char mapname[17]; // 16 + NULL

//	nullpo_retv(sd); // checked before to call function

	memset(mapname, 0, sizeof(mapname));
	strncpy(mapname, RFIFOP(fd,2), 16); // 17 - NULL
	
	if (mapname[0] != '\0') {
		memset(output, 0, sizeof(output));
		sprintf(output, "%s : %crura %s %d %d", sd->status.name, GM_Symbol(), mapname, RFIFOW(fd,18), RFIFOW(fd,20));
//		printf("/mm command: %s\n", output);
		is_atcommand(fd, sd, output, sd->GM_level); // do nothing if command doesn't work
	}

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_ChangeDir(int fd, struct map_session_data *sd) { // S 0x009b <head dir>.w <dir>.B
//	nullpo_retv(sd); // checked before to call function

	switch (sd->packet_ver) { // 0: old, 1: 7july04, 2: 13july04, 3: 26july04, 4: 9aug04/16aug04/17aug04, 5: 6sept04, 6: 21sept04, 7: 18oct04, 8: 25oct04/08nov04, 9: 6dec04, 10: 10jan05, 11: 9may05, 12: 28jun05, 13: 4april06 (by [Yor])
	case 13:
		sd->head_dir = RFIFOW(fd,7);
		sd->dir = RFIFOB(fd,11);
		break;
	case 12:
		sd->head_dir = RFIFOW(fd,8);
		sd->dir = RFIFOB(fd,16);
		break;
	case 11:
		sd->head_dir = RFIFOW(fd,7);
		sd->dir = RFIFOB(fd,10);
		break;
	case 10:
		sd->head_dir = RFIFOW(fd,12);
		sd->dir = RFIFOB(fd,22);
		break;
	case 9:
		sd->head_dir = RFIFOW(fd,3);
		sd->dir = RFIFOB(fd,7);
		break;
	case 8:
		sd->head_dir = RFIFOW(fd,6);
		sd->dir = RFIFOB(fd,14);
		break;
	case 7:
		sd->head_dir = RFIFOW(fd,5);
		sd->dir = RFIFOB(fd,12);
		break;
	case 6:
		sd->head_dir = RFIFOW(fd,8);
		sd->dir = RFIFOB(fd,17);
		break;
	case 5:
		sd->head_dir = RFIFOW(fd,4);
		sd->dir = RFIFOB(fd,9);
		break;
	case 4:
		sd->head_dir = RFIFOW(fd,7);
		sd->dir = RFIFOB(fd,11);
		break;
	case 3:
		sd->head_dir = RFIFOW(fd,5);
		sd->dir = RFIFOB(fd,12);
		break;
	case 2:
		sd->head_dir = RFIFOW(fd,5);
		sd->dir = RFIFOB(fd,12);
		break;
	default: // old version by default (and packet version 1)
		sd->head_dir = RFIFOW(fd,2);
		sd->dir = RFIFOB(fd,4);
		break;
	}

	//pc_setdir(sd, dir, headdir);

	clif_changed_dir(&sd->bl);

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_Emotion(int fd, struct map_session_data *sd) { // S 0x00bf <type>.B
	unsigned int tick;
//	nullpo_retv(sd); // checked before to call function

//	if (battle_config.basic_skill_check == 0 || pc_checkskill(sd, NV_BASIC) >= 2) { // other solution speeder
	if (battle_config.basic_skill_check == 0 || (sd->status.skill[NV_BASIC].id == NV_BASIC && sd->status.skill[NV_BASIC].lv >= 2)) {
		if (RFIFOB(fd,2) == 34) { // prevent use of the mute emote [Valaris]
			clif_skill_fail(sd, 1, 0, 1);
			return;
		}
		// fix flood of emotion icon (ro-proxy): flood only the hacker player
		if (sd->emotionlasttime >= (tick = gettick())) {
			sd->emotionlasttime = tick + 1000; // not more than 1 every second (normal client is every 3-4 seconds)
			clif_displaymessage(fd, msg_txt(591)); // Don't flood server with emotion icons, please.
			return;
		}
		sd->emotionlasttime = tick + 1000; // not more than 1 every second (normal client is every 3-4 seconds)

		WPACKETW(0) = 0xc0;
		WPACKETL(2) = sd->bl.id;
		WPACKETB(6) = RFIFOB(fd,2);
		clif_send(packet_len_table[0xc0], &sd->bl, AREA);
	} else
		clif_skill_fail(sd, 1, 0, 1);

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_HowManyConnections(int fd, struct map_session_data *sd) { // S 0x00c1
//	nullpo_retv(sd); // checked before to call function

	if (map_is_alone) { // not in multi-servers
		// calculation like @who (don't show hidden GM) and don't wait update from char-server (realtime calculation)
		int i, count;
		struct map_session_data *pl_sd;

		count = 0;
		for (i = 0; i < fd_max; i++)
			if (session[i] && (pl_sd = session[i]->session_data) && pl_sd->state.auth)
				if (!((pl_sd->GM_level >= battle_config.hide_GM_session || (pl_sd->status.option & OPTION_HIDE)) && (pl_sd->GM_level > sd->GM_level))) // only lower or same level
					count++;
		WPACKETW(0) = 0xc2;
		WPACKETL(2) = count;
		SENDPACKET(fd, packet_len_table[0xc2]);
	} else {
		WPACKETW(0) = 0xc2;
		WPACKETL(2) = map_getusers();
		SENDPACKET(fd, packet_len_table[0xc2]);
	}

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_ActionRequest(int fd, struct map_session_data *sd) { // S 0x0089 <target ID>.l <type>.B
	int target_id;
	unsigned char action_type;

//	nullpo_retv(sd); // checked before to call function

	if (pc_isdead(sd)) {
		clif_clearchar_area(&sd->bl, 1);
		return;
	}

	if (sd->bl.prev == NULL || sd->npc_id != 0 || sd->opt1 > 0 || sd->status.option & 2 ||
	    sd->sc_data[SC_TRICKDEAD].timer != -1 ||
	    sd->sc_data[SC_AUTOCOUNTER].timer != -1 || //オートカウンター
	    sd->sc_data[SC_BLADESTOP].timer != -1 || //白刃取り
	   (sd->sc_data[SC_GOSPEL].timer != -1 && sd->sc_data[SC_GOSPEL].val4 == BCT_SELF) ||	// cannot attack while in GOSPEL area
	   (sd->sc_data[SC_DANCING].timer != -1 && sd->sc_data[SC_LONGING].timer == -1)) //ダンス中
		return;

	pc_stop_walking(sd, 0);
	pc_stopattack(sd);

	switch (sd->packet_ver) { // 0: old, 1: 7july04, 2: 13july04, 3: 26july04, 4: 9aug04/16aug04/17aug04, 5: 6sept04, 6: 21sept04, 7: 18oct04, 8: 25oct04/08nov04, 9: 6dec04, 10: 10jan05, 11: 9may05, 12: 28jun05, 13: 4april06 (by [Yor])
	case 13:
		target_id = RFIFOL(fd,7);
		action_type = RFIFOB(fd,17);
		break;
	case 12:
		target_id = RFIFOL(fd,11);
		action_type = RFIFOB(fd,23);
		break;
	case 11:
		target_id = RFIFOL(fd,5);
		action_type = RFIFOB(fd,18);
		break;
	case 10:
		target_id = RFIFOL(fd,9);
		action_type = RFIFOB(fd,19);
		break;
	case 9:
		target_id = RFIFOL(fd,6);
		action_type = RFIFOB(fd,17);
		break;
	case 8:
		target_id = RFIFOL(fd,4);
		action_type = RFIFOB(fd,14);
		break;
	case 7:
		target_id = RFIFOL(fd,3);
		action_type = RFIFOB(fd,8);
		break;
	case 6:
		target_id = RFIFOL(fd,3);
		action_type = RFIFOB(fd,8);
		break;
	case 5:
		target_id = RFIFOL(fd,9);
		action_type = RFIFOB(fd,22);
		break;
	case 4:
		target_id = RFIFOL(fd,7);
		action_type = RFIFOB(fd,17);
		break;
	case 3:
		target_id = RFIFOL(fd,3);
		action_type = RFIFOB(fd,8);
		break;
	default: // old version by default (and packet version 1 and 2)
		target_id = RFIFOL(fd,2);
		action_type = RFIFOB(fd,6);
		break;
	}

	switch(action_type) {
	case 0x00: // once attack
	case 0x07: // continuous attack
		if (sd->sc_data[SC_WEDDING].timer != -1 || sd->view_class == 22)
			return;
		if (sd->vender_id != 0)
			return;
		if (!battle_config.sdelay_attack_enable && pc_checkskill(sd, SA_FREECAST) <= 0) {
			if (DIFF_TICK(gettick(), sd->canact_tick) < 0) {
				clif_skill_fail(sd, 1, 4, 0);
				return;
			}
		}
		pc_delinvincibletimer(sd);
		if (sd->attacktarget > 0) // [Valaris]
			sd->attacktarget = 0;
		pc_attack(sd, target_id, action_type != 0);
		break;
	case 0x02: // sitdown
		// can not sit down when casting
		if (sd->skilltimer != -1 || sd->sc_data[SC_GRAVITATION].timer != -1)
			return;
//		if (battle_config.basic_skill_check == 0 || pc_checkskill(sd, NV_BASIC) >= 3) { // other solution speeder
		if (battle_config.basic_skill_check == 0 || (sd->status.skill[NV_BASIC].id == NV_BASIC && sd->status.skill[NV_BASIC].lv >= 3)) {
			pc_stop_walking(sd, 1);
			skill_gangsterparadise(sd, 1); // ギャングスターパラダイス設定
			pc_setsit(sd); // (sd)->state.dead_sit = 2 // 0: standup, 1: dead, 2: sit
			skill_rest(sd, 1);
			clif_sitting(sd);
		} else
			clif_skill_fail(sd, 1, 0, 2);
		break;
	case 0x03: // standup
		skill_gangsterparadise(sd, 0); // ギャングスターパラダイス解除
		skill_rest(sd, 0); // TK_HPTIME standing up mode [Dralnu]
		pc_setstand(sd); // sd->state.dead_sit = 0 // 0: standup, 1: dead, 2: sit
		WPACKETW( 0) = 0x8a;
		WPACKETL( 2) = sd->bl.id;
		WPACKETB(26) = 3;
		clif_send(packet_len_table[0x8a], &sd->bl, AREA);
		break;
	}

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_Restart(int fd, struct map_session_data *sd) { // S 00b2 <type>.B
//	nullpo_retv(sd); // checked before to call function

	switch(RFIFOB(fd,2)) {
	case 0x00: // restart game when character died
		if (pc_isdead(sd)) {
			pc_setstand(sd);
			pc_setrestartvalue(sd, 3);
			pc_setpos(sd, sd->status.save_point.map, sd->status.save_point.x, sd->status.save_point.y, 2, 0);
		// in case the player's status somehow wasn't updated yet [Celest]
		} else if (sd->status.hp <= 0)
			pc_setdead(sd);
		break;
	case 0x01: // request character select
		if (!pc_isdead(sd) && (sd->opt1 || (sd->opt2 && !(night_flag == 1 && (sd->opt2 & STATE_BLIND) == STATE_BLIND))))
			return;

		// Rovert's Prevent logout option - Fixed [Valaris]
		if ((gettick() - sd->canlog_tick) >= 10000 || (!battle_config.prevent_logout)) {
			if (chrif_charselectreq(sd)) { // refuse disconnection if char-server is disconnected
				WPACKETW(0) = 0x18b;
				WPACKETW(2) = 1; // flag= 0: success, 1: failure (please wait 15 sec...)
				SENDPACKET(fd, packet_len_table[0x018b]);
			} // do nothing if accepted. Client will automatically connect to char-server
		} else {
			WPACKETW(0) = 0x18b;
			WPACKETW(2) = 1; // flag= 0: success, 1: failure (please wait 15 sec...)
			SENDPACKET(fd, packet_len_table[0x018b]);
		}
		break;
	}

	return;
}

/*==========================================
 * Transmission of a wisp (S 0096 <len>.w <nick>.24B <message>.?B)
 *------------------------------------------
 */
void clif_parse_Wis(int fd, struct map_session_data *sd) { // S 0x0096 <len>.w <nick>.24B <message>.?B
	char *message; // dynamic allocation
	char *message_to_gm;
	char *gm_command; // dynamic allocation
	char player_name[25]; // 24 + NULL
	struct map_session_data *dstsd;
	int i, message_size;

//	nullpo_retv(sd); // checked before to call function

	//printf("clif_parse_Wis: message: '%s'.\n", RFIFOP(fd,28));

	if (sd->sc_data[SC_BERSERK].timer != -1 || //バーサーク時は会話も不可
	    sd->sc_data[SC_NOCHAT].timer != -1) //チャット禁止
		return;

	// get size
	message_size = RFIFOW(fd,2) - 28;

	// check size of message
	if (message_size < 1)
		return;

	// get message
	CALLOC(message, char, message_size + 1); // +1 for null terminated if necessary
	strncpy(message, RFIFOP(fd,28), message_size);
	// remove final NULL from message_size
	i = strlen(message);
	if (i < message_size) // note: normal client always sends final NULL, so i = message_size - 1. If more, it's abnormal
		message_size = i;

	// check size of message again
	if (message_size < 1) { // send a void message?
		FREE(message);
		return;
	} else if (message_size > battle_config.max_message_length) { // more than the authorized?
		printf("Possible hack on wisp message (PM message): character '%s' (account: %d) tries to send a big message (%d characters).\n", sd->status.name, sd->status.account_id, message_size);
		CALLOC(message_to_gm, char, message_size + MAX_MSG_LEN + 100); // message_size + max size of msg_txt + security (char name, char id, etc...) (100)
		// information is sended to all online GM
		sprintf(message_to_gm, msg_txt(643), sd->status.name, sd->status.account_id, message_size); // Possible hack on wisp message (PM message): character '%s' (account: %d) tries to send a big message (%d characters).
		intif_wis_message_to_gm(wisp_server_name, battle_config.hack_info_GM_level, message_to_gm);
		intif_wis_message_to_gm(wisp_server_name, battle_config.hack_info_GM_level, msg_txt(641)); //  This player hasn't been banned.
		FREE(message_to_gm);
		FREE(message);
		return;
	}

	//printf("clif_parse_Wis: message: '%s'.\n", message); // structure of message: <message>
	// check GM command
	CALLOC(gm_command, char, message_size + 28); // (name) 24 + " : " (3) + message + NULL (1)
	sprintf(gm_command, "%s : %s", sd->status.name, message);
	if (is_atcommand(fd, sd, gm_command, sd->GM_level) != AtCommand_None) {
		FREE(gm_command);
		FREE(message);
		return;
	}
	FREE(gm_command);

	// check bad words (after is_atcommand check, because player can have a bad name)
	if (check_bad_word(message, message_size, sd)) {
		FREE(message);
		return;
	}

	// searching destination character
	memset(player_name, 0, sizeof(player_name)); // 24 + NULL
	strncpy(player_name, RFIFOP(fd,4), 24);
	dstsd = map_nick2sd(player_name);
	// player is not on this map-server
	if (dstsd == NULL) {
		if (!map_is_alone) {// if multi-map server
			// send message to inter-server
			intif_wis_message(sd, player_name, message, message_size); // 0x3001/0x3801 <packet_len>.w (<w_id_0x3801>.L) <sender_GM_level>.B <sender_name>.24B <nick_name>.24B <message>.?B
		} else { // unique map-server: not neessary to send to interserver
			clif_wis_end(fd, 1); // type: 0: success to send wisper, 1: target character is not loged in?, 2: ignored by target
		}

	// player is on this map-server
	} else {
		// if you send to your self, don't send anything to others
		if (dstsd->fd == fd) // but, normaly, it's impossible!
			clif_wis_message(fd, wisp_server_name, msg_txt(509), strlen(msg_txt(509)) + 1); // You can not page yourself. Sorry.
		// otherwise, send message and answer immediatly
		else {
			// if GM pm, not ignore it
			if (sd->GM_level >= battle_config.pm_gm_not_ignored && sd->GM_level >= dstsd->GM_level) {
				clif_wis_message(dstsd->fd, sd->status.name, message, message_size + 1);
				clif_wis_end(fd, 0); // type: 0: success to send wisper, 1: target character is not loged in?, 2: ignored by target
			// if normal message
			} else {
				if (dstsd->ignoreAll == 1)
					clif_wis_end(fd, 2); // type: 0: success to send wisper, 1: target character is not loged in?, 2: ignored by target
				else {
					// if player ignore the source character
					for(i = 0; i < dstsd->ignore_num; i++)
						if (strcmp(dstsd->ignore[i].name, sd->status.name) == 0) {
							clif_wis_end(fd, 2); // flag: 0: success to send wisper, 1: target character is not loged in?, 2: ignored by target
							break;
						}
					// if source player not found in ignore list
					if (i == dstsd->ignore_num) {
						clif_wis_message(dstsd->fd, sd->status.name, message, message_size + 1);
						clif_wis_end(fd, 0); // type: 0: success to send wisper, 1: target character is not loged in?, 2: ignored by target
					}
				}
			}
		}
	}

	FREE(message);

	return;
}

/*==========================================
 * /b /nb /bb
 *------------------------------------------
 */
void clif_parse_GMmessage(int fd, struct map_session_data *sd) { // S 0x0099 <len>.w <message>.?B
	char *message;
	char *message_to_gm;
	char *output;
	int i, message_size;

//	nullpo_retv(sd); // checked before to call function

	// get size
	message_size = RFIFOW(fd,2) - 4;

	// check size of message
	if (message_size < 1)
		return;

	// get message
	CALLOC(message, char, message_size + 1);
	strncpy(message, RFIFOP(fd,4), message_size);
	// remove final NULL from message_size
	i = strlen(message);
	if (i < message_size) // note: normal client always sends final NULL, so i = message_size - 1. If more, it's abnormal
		message_size = i;

	// check size of message again
	if (message_size < 1) { // send a void message?
		FREE(message);
		return;
	} else if (message_size > battle_config.max_message_length) { // more than the authorized?
		printf("Possible hack on GM message: character '%s' (account: %d) tries to send a big message (%d characters).\n", sd->status.name, sd->status.account_id, message_size);
		CALLOC(message_to_gm, char, message_size + MAX_MSG_LEN + 100); // message_size + max size of msg_txt + security (char name, char id, etc...) (100)
		// information is sended to all online GM
		sprintf(message_to_gm, msg_txt(644), sd->status.name, sd->status.account_id, message_size); // Possible hack on GM message: character '%s' (account: %d) tries to send a big message (%d characters).
		intif_wis_message_to_gm(wisp_server_name, battle_config.hack_info_GM_level, message_to_gm);
		intif_wis_message_to_gm(wisp_server_name, battle_config.hack_info_GM_level, msg_txt(641)); //  This player hasn't been banned.
		FREE(message_to_gm);
		FREE(message);
		return;
	}

	//printf("clif_parse_GMmessage: message: '%s'.\n", message); // structure of message: <player_name>: <message> or <message>

	// don't check bad words (is_atcommand already do the check)
/*	if (check_bad_word(message, message_size, sd)) {
		FREE(message);
		return;
	}*/

	// do GM command
	CALLOC(output, char, message_size + 53); // name(24) + " : "(3) + @nb(4) + " "(1) + message(x) + NULL(1) + security (20) = 53 + length message
	sprintf(output, "%s : %cnb %s", sd->status.name, GM_Symbol(), message);
	//printf("clif_parse_GMmessage: %s\n", output);
	is_atcommand(fd, sd, output, sd->GM_level); // do nothing if command doesn't work
	FREE(output);

	FREE(message);

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_TakeItem(int fd, struct map_session_data *sd) { // S 0x009f <ID>.l
	struct flooritem_data *fitem;
	int map_object_id;

//	nullpo_retv(sd); // checked before to call function

	if (pc_isdead(sd)) {
		clif_clearchar_area(&sd->bl, 1);
		return;
	}

	if (sd->npc_id != 0 || sd->vender_id != 0 || sd->opt1 > 0 || sd->trade_partner != 0 ||
	    sd->sc_data[SC_TRICKDEAD].timer != -1 || //死んだふり
	    sd->sc_data[SC_BLADESTOP].timer != -1 || //白刃取り
	    sd->sc_data[SC_BERSERK].timer != -1 || //バーサーク
	    sd->sc_data[SC_NOCHAT].timer != -1) //会話禁止
		return;

	switch (sd->packet_ver) { // 0: old, 1: 7july04, 2: 13july04, 3: 26july04, 4: 9aug04/16aug04/17aug04, 5: 6sept04, 6: 21sept04, 7: 18oct04, 8: 25oct04/08nov04, 9: 6dec04, 10: 10jan05, 11: 9may05, 12: 28jun05, 13: 4april06 (by [Yor])
	case 13:
		map_object_id = RFIFOL(fd,9);
		break;
	case 12:
		map_object_id = RFIFOL(fd,9);
		break;
	case 11:
		map_object_id = RFIFOL(fd,4);
		break;
	case 10:
		map_object_id = RFIFOL(fd,5);
		break;
	case 9:
		map_object_id = RFIFOL(fd,3);
		break;
	case 8:
		map_object_id = RFIFOL(fd,5);
		break;
	case 7:
		map_object_id = RFIFOL(fd,6);
		break;
	case 6:
		map_object_id = RFIFOL(fd,10);
		break;
	case 5:
		map_object_id = RFIFOL(fd,7);
		break;
	case 4:
		map_object_id = RFIFOL(fd,9);
		break;
	case 3:
		map_object_id = RFIFOL(fd,6);
		break;
	case 2:
		map_object_id = RFIFOL(fd,6);
		break;
	default: // old version by default (and packet version 1)
		map_object_id = RFIFOL(fd,2);
		break;
	}
	fitem = (struct flooritem_data*)map_id2bl(map_object_id);

	if (fitem == NULL || fitem->bl.type != BL_ITEM || fitem->bl.m != sd->bl.m)
		return;

	// check against looters (cloaking and chasewalk users)
	if (fitem->owner != sd->bl.id && // if player is not owner of the item, he can not get it AND
	    fitem->first_get_id != sd->bl.id) { // if it's not 'get' first of the item (not check get 'get' second/third, because he can get before the first)
		if (//pc_iscloaking(sd) || //Disable cloaking characters from looting [Skotlex] // incorrect check. use timer (sd->sc_data[SC_CLOAKING].timer != -1)
		    sd->sc_data[SC_CLOAKING].timer != -1 || // Cloaking [Yor]
		    sd->sc_data[SC_CHASEWALK].timer != -1) { // Chasewalk [Aru]
			clif_additem(sd, 0, 0, 6); // 6: you cannot get the item.
			return;
		}
	}

	pc_takeitem(sd, fitem);
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_DropItem(int fd, struct map_session_data *sd) { // S 0x00a2 <index>.w <amount>.w
//	nullpo_retv(sd); // checked before to call function

	if (pc_isdead(sd)) {
		clif_clearchar_area(&sd->bl, 1);
		return;
	}
	if (sd->npc_id != 0 || sd->vender_id != 0 || sd->opt1 > 0 ||
	    sd->sc_data[SC_AUTOCOUNTER].timer != -1 || //オートカウンター
	    sd->sc_data[SC_BLADESTOP].timer != -1 || //白刃取り
	    sd->sc_data[SC_BERSERK].timer != -1) //バーサーク
		return;

	switch (sd->packet_ver) { // 0: old, 1: 7july04, 2: 13july04, 3: 26july04, 4: 9aug04/16aug04/17aug04, 5: 6sept04, 6: 21sept04, 7: 18oct04, 8: 25oct04/08nov04, 9: 6dec04, 10: 10jan05, 11: 9may05, 12: 28jun05, 13: 4april06 (by [Yor])
	case 13:
		pc_dropitem(sd, RFIFOW(fd,8) - 2, RFIFOW(fd,15)); // item_index, item_amount
		break;
	case 12:
		pc_dropitem(sd, RFIFOW(fd,3) - 2, RFIFOW(fd,10)); // item_index, item_amount
		break;
	case 11:
		pc_dropitem(sd, RFIFOW(fd,5) - 2, RFIFOW(fd,8)); // item_index, item_amount
		break;
	case 10:
		pc_dropitem(sd, RFIFOW(fd,15) - 2, RFIFOW(fd,18)); // item_index, item_amount
		break;
	case 9:
		pc_dropitem(sd, RFIFOW(fd,4) - 2, RFIFOW(fd,10)); // item_index, item_amount
		break;
	case 8:
		pc_dropitem(sd, RFIFOW(fd,6) - 2, RFIFOW(fd,10)); // item_index, item_amount
		break;
	case 7:
		pc_dropitem(sd, RFIFOW(fd,5) - 2, RFIFOW(fd,12)); // item_index, item_amount
		break;
	case 6:
		pc_dropitem(sd, RFIFOW(fd,12) - 2, RFIFOW(fd,17)); // item_index, item_amount
		break;
	case 5:
		pc_dropitem(sd, RFIFOW(fd,6) - 2, RFIFOW(fd,15)); // item_index, item_amount
		break;
	case 4:
		pc_dropitem(sd, RFIFOW(fd,8) - 2, RFIFOW(fd,15)); // item_index, item_amount
		break;
	case 3:
		pc_dropitem(sd, RFIFOW(fd,5) - 2, RFIFOW(fd,12)); // item_index, item_amount
		break;
	default: // old version by default (+ packet version 1 and 2)
		pc_dropitem(sd, RFIFOW(fd,2) - 2, RFIFOW(fd,4)); // item_index, item_amount
		break;
	}

	// Anti Crash-Dupe Protection:
	// ----------------------------------------
	// Save 2 characters in same moment (with a trade for example)
	// drop an item on floor
	// other player take it and disconnect (with item saving)
	// remaining character crash server (and can be not saved -> conserv item) - Hacker found always a solution to crash a server :(
	// to avoid that:
	// when voluntary drop an item on the floor (so only here, not in pc_dropitem function) we save player
	// Note: If a player want to give an item to another, he can trade too (secured transaction - and the 2 players are saved in same time).
	chrif_save(sd); // do pc_makesavestatus and save storage + account_reg/account_reg2 too

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_UseItem(int fd, struct map_session_data *sd) { // S 0x00a7 <index>.w <ID>.l
//	nullpo_retv(sd); // checked before to call function

	if (pc_isdead(sd)) {
		clif_clearchar_area(&sd->bl, 1);
		return;
	}
	if (sd->npc_id != 0 || sd->vender_id != 0 || (sd->opt1 > 0 && sd->opt1 != 6) ||
	    sd->sc_data[SC_TRICKDEAD].timer != -1 || //死んだふり
	    sd->sc_data[SC_BLADESTOP].timer != -1 || //白刃取り
	    sd->sc_data[SC_BERSERK].timer != -1 || 
		sd->sc_data[SC_GRAVITATION].timer !=- 1 ||
	    sd->sc_data[SC_NOCHAT].timer != -1) //会話禁止
		return;

	pc_delinvincibletimer(sd);

	switch (sd->packet_ver) { // 0: old, 1: 7july04, 2: 13july04, 3: 26july04, 4: 9aug04/16aug04/17aug04, 5: 6sept04, 6: 21sept04, 7: 18oct04, 8: 25oct04/08nov04, 9: 6dec04, 10: 10jan05, 11: 9may05, 12: 28jun05, 13: 4april06 (by [Yor])
	case 13:
		pc_useitem(sd, RFIFOW(fd,9) - 2);
		break;
	case 12:
		pc_useitem(sd, RFIFOW(fd,9) - 2);
		break;
	case 11:
		pc_useitem(sd, RFIFOW(fd,4) - 2);
		break;
	case 10:
		pc_useitem(sd, RFIFOW(fd,5) - 2);
		break;
	case 9:
		pc_useitem(sd, RFIFOW(fd,3) - 2);
		break;
	case 8:
		pc_useitem(sd, RFIFOW(fd,5) - 2);
		break;
	case 7:
		pc_useitem(sd, RFIFOW(fd,6) - 2);
		break;
	case 6:
		pc_useitem(sd, RFIFOW(fd,10) - 2);
		break;
	case 5:
		pc_useitem(sd, RFIFOW(fd,7) - 2);
		break;
	case 4:
		pc_useitem(sd, RFIFOW(fd,9) - 2);
		break;
	case 3:
		pc_useitem(sd, RFIFOW(fd,6) - 2);
		break;
	case 2:
		pc_useitem(sd, RFIFOW(fd,6) - 2);
		break;
	case 1:
		pc_useitem(sd, RFIFOW(fd,5) - 2);
		break;
	default: // old version by default
		pc_useitem(sd, RFIFOW(fd,2) - 2);
		break;
	}

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_EquipItem(int fd, struct map_session_data *sd) { // S 0x00a9 <index>.w <equip type>.w
	int idx;

//	nullpo_retv(sd); // checked before to call function

	idx = RFIFOW(fd, 2) - 2;
	if (idx < 0 || idx >= MAX_INVENTORY)
		return;

	if (sd->npc_id != 0 || sd->vender_id != 0)
		return;
	if (sd->sc_data[SC_BLADESTOP].timer != -1 || sd->sc_data[SC_BERSERK].timer != -1)
		return;

	if (pc_isdead(sd)) {
		clif_clearchar_area(&sd->bl, 1);
		return;
	}

	if (sd->status.inventory[idx].identify != 1) { // 未鑑定
		clif_equipitemack(sd, idx, 0, 0); // fail
		return;
	}

/*	if (sd->status.inventory[idx].attribute != 0) { // 破壊されている
		clif_equipitemack(sd, idx, 0, 0); // fail
		return;
	}*/
	//ペット用装備であるかないか
	if (sd->inventory_data[idx]) {
		if (sd->inventory_data[idx]->type != 8) { // 8: petequip
			if (sd->inventory_data[idx]->type == 10) // 10: arrow
				pc_equipitem(sd, idx, 0x8000); // 矢を無理やり装備できるように（－－；
			else
				pc_equipitem(sd, idx, RFIFOW(fd, 4));
		} else
			pet_equipitem(sd, idx);
	}

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_UnequipItem(int fd, struct map_session_data *sd) { // S 0x00ab <index>.w
	int idx;

//	nullpo_retv(sd); // checked before to call function

	idx = RFIFOW(fd, 2) - 2;
	if (idx < 0 || idx >= MAX_INVENTORY)
		return;

	if (pc_isdead(sd)) {
		clif_clearchar_area(&sd->bl, 1);
		return;
	}

	if (sd->npc_id != 0 || sd->vender_id != 0 || sd->opt1 > 0)
		return;

/*	if (sd->status.inventory[idx].attribute == 1 && sd->sc_data[SC_BROKNWEAPON].timer != -1)
		status_change_end(&sd->bl, SC_BROKNWEAPON, -1);
	if (sd->status.inventory[idx].attribute == 1 && sd->sc_data[SC_BROKNARMOR].timer != -1)
		status_change_end(&sd->bl, SC_BROKNARMOR, -1);
	if (sd->sc_count && (sd->sc_data[SC_BLADESTOP].timer != -1 || sd->sc_data[SC_BERSERK].timer != -1))
		return;*/

	pc_unequipitem(sd, idx, 1);

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_NpcClicked(int fd, struct map_session_data *sd) { // S 0x0090 <ID>.l <type?>.B
//	nullpo_retv(sd); // checked before to call function

	if (pc_isdead(sd)) {
		clif_clearchar_area(&sd->bl, 1);
		return;
	}

	if (sd->npc_id != 0 || sd->vender_id != 0)
		return;

	npc_click(sd, RFIFOL(fd,2));
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_NpcBuySellSelected(int fd, struct map_session_data *sd) { // S 0x00c5 <ID>.l <type>.B (type: 00 is buy, 01 is sell).
//	nullpo_retv(sd); // checked before to call function

	npc_buysellsel(sd, RFIFOL(fd,2), RFIFOB(fd,6));

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_NpcBuyListSend(int fd, struct map_session_data *sd) { // S 0x00c8 <len>.w {<amount>.w <item ID>.w}.4B*
	int fail, n;

//	nullpo_retv(sd); // checked before to call function

	n = (RFIFOW(fd,2) - 4) / 4;

	if (n <= 0) // max is checked in npc_buylist function
		return;

	fail = npc_buylist(sd, n, (unsigned short*)RFIFOP(fd,4));

	WPACKETW(0) = 0xca;
	WPACKETB(2) = fail; // 0: The deal has successfully completed., 1: You dont have enough zeny., 2: you are overcharged!, 3: You are over your weight limit.
	SENDPACKET(fd, packet_len_table[0xca]);

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_NpcSellListSend(int fd, struct map_session_data *sd) { // S 0x00c9 <len>.w {<index>.w <amount>.w}.4B*
	int fail, n;

//	nullpo_retv(sd); // checked before to call function

	n = (RFIFOW(fd,2) - 4) /4;
	if (n <= 0 || n > MAX_INVENTORY) // max is checked in npc_selllist function, but normal client never send more items that total in inventory
		return;

	fail = npc_selllist(sd, n, (unsigned short*)RFIFOP(fd,4));

	WPACKETW(0) = 0xcb;
	WPACKETB(2) = fail;
	SENDPACKET(fd, packet_len_table[0xcb]);

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_CreateChatRoom(int fd, struct map_session_data *sd) { // S 0x00d5 <len>.w <limit>.w <pub>.B <passwd>.8B <title>.?B
//	nullpo_retv(sd); // checked before to call function

	if (sd->sc_data[SC_BERSERK].timer != -1 || //バーサーク時は会話も不可
	    sd->sc_data[SC_NOCHAT].timer != -1) { //チャット禁止
		// no message
		return;
	}

	// a vender can not create chat [NO-IP]
	if (sd->vender_id != 0)
		return;

//	if (battle_config.basic_skill_check == 0 || pc_checkskill(sd, NV_BASIC) >= 4) { // other solution speeder
	if (battle_config.basic_skill_check == 0 || (sd->status.skill[NV_BASIC].id == NV_BASIC && sd->status.skill[NV_BASIC].lv >= 4)) {
		char *chat_title;
		int chat_title_len;
		chat_title_len = RFIFOW(fd,2) - 15;
		CALLOC(chat_title, char, chat_title_len + 1); // (title) + NULL
		strncpy(chat_title, RFIFOP(fd,15), chat_title_len);
		// check void chat title
		if (chat_title[0] == '\0') {
			FREE(chat_title);
			return;
		}
		chat_title_len = strlen(chat_title); // be sure to have right length
		// check bad words
		if (check_bad_word(chat_title, chat_title_len, sd)) {
			FREE(chat_title);
			return;
		}
		chat_createchat(sd, RFIFOW(fd,4), RFIFOB(fd,6), RFIFOP(fd,7), chat_title, chat_title_len);
		FREE(chat_title);
	} else
		clif_skill_fail(sd, 1, 0, 3); // You've not learned enough Basic Skill to create a chat room

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_ChatAddMember(int fd, struct map_session_data *sd) { // S 0x00d9 <chat ID>.l <passwd>.8B
//	nullpo_retv(sd); // checked before to call function

	chat_joinchat(sd, RFIFOL(fd,2), RFIFOP(fd,6));

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_ChatRoomStatusChange(int fd, struct map_session_data *sd) { // S 0x00de <len>.w <limit>.w <pub>.B <passwd>.8B <title>.?B
//	nullpo_retv(sd); // checked before to call function

	chat_changechatstatus(sd, RFIFOW(fd,4), RFIFOB(fd,6), RFIFOP(fd,7), RFIFOP(fd,15), RFIFOW(fd,2) - 15);

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_ChangeChatOwner(int fd, struct map_session_data *sd) { // S 0x00e0 ?.l <nick>.24B
//	nullpo_retv(sd); // checked before to call function

	chat_changechatowner(sd, RFIFOP(fd,6));

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_KickFromChat(int fd, struct map_session_data *sd) { // S 0x00e2 <nick>.24B
//	nullpo_retv(sd); // checked before to call function

	chat_kickchat(sd, RFIFOP(fd,2));

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_ChatLeave(int fd, struct map_session_data *sd) { // S 0x00e3
//	nullpo_retv(sd); // checked before to call function

	chat_leavechat(sd);

	return;
}

/*==========================================
 * 取引要請を相手に送る
 *------------------------------------------
 */
void clif_parse_TradeRequest(int fd, struct map_session_data *sd) { // 0x00e4 <ID>.L
//	nullpo_retv(sd); // checked before to call function

	if (map[sd->bl.m].flag.notrade) {
		clif_displaymessage(fd, msg_txt(655)); // Sorry, but trade is not allowed on this map.
		return;
	}

//	if (battle_config.basic_skill_check == 0 || pc_checkskill(sd, NV_BASIC) >= 1) { // other solution speeder
	if (battle_config.basic_skill_check == 0 || (sd->status.skill[NV_BASIC].id == NV_BASIC && sd->status.skill[NV_BASIC].lv >= 1)) {
		trade_traderequest(sd, RFIFOL(fd, 2), 1); // 1: check if near the other player, 0: doesnt check
	} else
		clif_skill_fail(sd, 1, 0, 0);

	return;
}

/*==========================================
 * 取引要請
 *------------------------------------------
 */
void clif_parse_TradeAck(int fd, struct map_session_data *sd) { // S 0x00e6 <type>.B: 3: trade ok., 4: trade canceled. (no other reception with this packet)
//	nullpo_retv(sd); // checked before to call function

	trade_tradeack(sd, RFIFOB(fd,2)); // 0x00e6 <type>.B: 3: trade ok., 4: trade canceled. (no other reception with this packet)

	return;
}

/*==========================================
 * アイテム追加
 *------------------------------------------
 */
void clif_parse_TradeAddItem(int fd, struct map_session_data *sd) { // S 0x00e8 <index>.w <amount>.l
//	nullpo_retv(sd); // checked before to call function

	trade_tradeadditem(sd, RFIFOW(fd,2), RFIFOL(fd,4));

	return;
}

/*==========================================
 * アイテム追加完了(ok押し)
 *------------------------------------------
 */
void clif_parse_TradeOk(int fd, struct map_session_data *sd) { // S 0x00eb
//	nullpo_retv(sd); // checked before to call function

	trade_tradeok(sd);

	return;
}

/*==========================================
 * 取引キャンセル
 *------------------------------------------
 */
void clif_parse_TradeCancel(int fd, struct map_session_data *sd) { // S 0x00ed
//	nullpo_retv(sd); // checked before to call function

	trade_tradecancel(sd);

	return;
}

/*==========================================
 * 取引許諾(trade押し)
 *------------------------------------------
 */
void clif_parse_TradeCommit(int fd, struct map_session_data *sd) { // S 0x00ef
//	nullpo_retv(sd); // checked before to call function

	trade_tradecommit(sd);

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_StopAttack(int fd, struct map_session_data *sd) { // S 0x0118
//	nullpo_retv(sd); // checked before to call function

	pc_stopattack(sd);

	return;
}

/*==========================================
 * カートへアイテムを移す
 *------------------------------------------
 */
void clif_parse_PutItemToCart(int fd, struct map_session_data *sd) { // S 0x0126 <index>.w <amount>.l
//	nullpo_retv(sd); // checked before to call function

	if (sd->npc_id != 0 || sd->vender_id != 0 || sd->trade_partner != 0 || !pc_iscarton(sd))
		return;

	pc_putitemtocart(sd, RFIFOW(fd,2) - 2, RFIFOL(fd,4));

	return;
}

/*==========================================
 * カートからアイテムを出す
 *------------------------------------------
 */
void clif_parse_GetItemFromCart(int fd, struct map_session_data *sd) { // S 0x0127 <index>.w <amount>.l
//	nullpo_retv(sd); // checked before to call function

	if (sd->npc_id != 0 || sd->vender_id != 0 || sd->trade_partner != 0 || !pc_iscarton(sd))
		return;

	pc_getitemfromcart(sd, RFIFOW(fd,2) - 2, RFIFOL(fd,4));

	return;
}

/*==========================================
 * 付属品(鷹,ペコ,カート)をはずす
 *------------------------------------------
 */
void clif_parse_RemoveOption(int fd, struct map_session_data *sd) { // S 0x012a
	int flag = 0;

//	nullpo_retv(sd); // checked before to call function

	if (pc_isriding(sd)) { // jobchange when removing peco [Valaris]
		switch(sd->status.class) { //Peco Classes
			case JOB_KNIGHT2:
				sd->status.class = sd->view_class = JOB_KNIGHT;
				break;
			case JOB_CRUSADER2:
				sd->status.class = sd->view_class = JOB_CRUSADER;
				break;
			case JOB_LORD_KNIGHT2:
				sd->status.class = sd->view_class = JOB_LORD_KNIGHT;
				break;
			case JOB_PALADIN2:
				sd->status.class = sd->view_class = JOB_PALADIN;
				break;
			case JOB_BABY_KNIGHT2:
				sd->status.class = sd->view_class = JOB_BABY_KNIGHT;
				break;
			case JOB_BABY_CRUSADER2:
				sd->status.class = sd->view_class = JOB_BABY_CRUSADER;
				break;
		}
		pc_setoption(sd, sd->status.option & ~0x0020);
		flag = 1;
	}

	if (pc_iscarton(sd)) {
		pc_setoption(sd, sd->status.option & ~CART_MASK);
		flag = 1;
	}

	if (pc_isfalcon(sd)) {
		pc_setoption(sd, sd->status.option & ~0x0010);
		flag = 1;
	}

	if (flag == 0)
		pc_setoption(sd, 0);

	clif_changelook(&sd->bl, LOOK_BASE, sd->view_class); /* for button in the equip window */
	if (sd->status.clothes_color > 0)
		clif_changelook(&sd->bl, LOOK_CLOTHES_COLOR, sd->status.clothes_color);

	return;
}

/*==========================================
 * チェンジカート
 *------------------------------------------
 */
void clif_parse_ChangeCart(int fd, struct map_session_data *sd) { // S 0x01af <type>.w
//	nullpo_retv(sd); // checked before to call function

	pc_setcart(sd, RFIFOW(fd,2));

	return;
}

/*==========================================
 * ステータスアップ
 *------------------------------------------
 */
void clif_parse_StatusUp(int fd, struct map_session_data *sd) { // S 0x00bb <type>.w <amount>.B
//	nullpo_retv(sd); // checked before to call function

	pc_statusup(sd, RFIFOW(fd,2));

	return;
}

/*==========================================
 * スキルレベルアップ
 *------------------------------------------
 */
void clif_parse_SkillUp(int fd, struct map_session_data *sd) { // S 0x0112 <skill ID>.w
//	nullpo_retv(sd); // checked before to call function

	pc_skillup(sd, RFIFOW(fd,2));

	return;
}

/*==========================================
 * スキル使用（ID指定）
 *------------------------------------------
 */
void clif_parse_UseSkillToId(int fd, struct map_session_data *sd) { // S 0x0113 <level>.w <skill ID>.w <ID>.l
	short skillnum, skilllv;
	int lv, target_id;
//	struct block_list *bl;

//	nullpo_retv(sd); // checked before to call function

//	if (map[sd->bl.m].flag.noskill) return;
	if (sd->bl.prev == NULL || sd->chatID || sd->npc_id || sd->vender_id)
		return;

	if (pc_issit(sd)) // client can not send useskill packet if player is sit down. But with hacker and latency... do a check
		return; // to avoid: have regeneration of a sitting man, but not be sit down.

	switch (sd->packet_ver) { // 0: old, 1: 7july04, 2: 13july04, 3: 26july04, 4: 9aug04/16aug04/17aug04, 5: 6sept04, 6: 21sept04, 7: 18oct04, 8: 25oct04/08nov04, 9: 6dec04, 10: 10jan05, 11: 9may05, 12: 28jun05, 13: 4april06 (by [Yor])
		case 13:
			skilllv = RFIFOW(fd,11);
			skillnum = RFIFOW(fd,18);
			target_id = RFIFOL(fd,22);
			break;
		case 12:
			skilllv = RFIFOW(fd,6);
			skillnum = RFIFOW(fd,17);
			target_id = RFIFOL(fd,30);
			break;
		case 11:
			skilllv = RFIFOW(fd,6);
			skillnum = RFIFOW(fd,10);
			target_id = RFIFOL(fd,21);
			break;
		case 10:
			skilllv = RFIFOW(fd,8);
			skillnum = RFIFOW(fd,16);
			target_id = RFIFOL(fd,22);
			break;
		case 9:
			skilllv = RFIFOW(fd,8);
			skillnum = RFIFOW(fd,12);
			target_id = RFIFOL(fd,18);
			break;
		case 8:
			skilllv = RFIFOW(fd,4);
			skillnum = RFIFOW(fd,10);
			target_id = RFIFOL(fd,22);
			break;
		case 7:
			skilllv = RFIFOW(fd,7);
			skillnum = RFIFOW(fd,12);
			target_id = RFIFOL(fd,16);
			break;
		case 6:
			skilllv = RFIFOW(fd,4);
			skillnum = RFIFOW(fd,7);
			target_id = RFIFOL(fd,10);
			break;
		case 5:
			skilllv = RFIFOW(fd,9);
			skillnum = RFIFOW(fd,15);
			target_id = RFIFOL(fd,18);
			break;
		case 4:
			skilllv = RFIFOW(fd,11);
			skillnum = RFIFOW(fd,18);
			target_id = RFIFOL(fd,22);
			break;
		case 3:
			skilllv = RFIFOW(fd,7);
			skillnum = RFIFOW(fd,12);
			target_id = RFIFOL(fd,16);
			break;
		case 2:
			skilllv = RFIFOW(fd,7);
			skillnum = RFIFOW(fd,9);
			target_id = RFIFOL(fd,15);
			break;
		case 1:
			skilllv = RFIFOW(fd,4);
			skillnum = RFIFOW(fd,9);
			target_id = RFIFOL(fd,11);
			break;
		default: // old version by default
			skilllv = RFIFOW(fd,2);
			skillnum = RFIFOW(fd,4);
			target_id = RFIFOL(fd,6);
			break;
	}

	if (skillnotok(skillnum, sd))
		return;

	if (sd->skilltimer != -1) {
		if (skillnum != SA_CASTCANCEL)
			return;
	} else if (DIFF_TICK(gettick(), sd->canact_tick) < 0 &&
	           // allow monk combos to ignore this delay [celest]
	           !(sd->sc_count && sd->sc_data[SC_COMBO].timer != -1 &&
	            (skillnum == MO_EXTREMITYFIST ||
	             skillnum == MO_CHAINCOMBO ||
	             skillnum == MO_COMBOFINISH ||
	             skillnum == CH_PALMSTRIKE ||
	             skillnum == CH_TIGERFIST ||
	             skillnum == CH_CHAINCRUSH))) {
		clif_skill_fail(sd, skillnum, 4, 0);
		return;
	}

	if ((sd->sc_data[SC_TRICKDEAD].timer != -1 && skillnum != NV_TRICKDEAD) ||
	     sd->sc_data[SC_BERSERK].timer != -1 || sd->sc_data[SC_NOCHAT].timer != -1 ||
	    (sd->sc_data[SC_GOSPEL].timer != -1 && sd->sc_data[SC_GOSPEL].val4 == BCT_SELF && skillnum != PA_GOSPEL) ||
	     sd->sc_data[SC_WEDDING].timer != -1 || sd->view_class == 22)
		return;

	pc_delinvincibletimer(sd);
	if (sd->skillitem >= 0 && sd->skillitem == skillnum) {
		if (skilllv != sd->skillitemlv)
			skilllv = sd->skillitemlv;
		skill_use_id(sd, target_id, skillnum, skilllv);
	} else {
		sd->skillitem = sd->skillitemlv = -1;
		if (skillnum == MO_EXTREMITYFIST) {
				if ((sd->sc_data[SC_COMBO].timer == -1 || 
				(sd->sc_data[SC_COMBO].val1 != MO_COMBOFINISH && sd->sc_data[SC_COMBO].val1 != CH_TIGERFIST && sd->sc_data[SC_COMBO].val1 != CH_CHAINCRUSH))) {
				if (!sd->state.skill_flag) {
					sd->state.skill_flag = 1;
					clif_skillinfo(sd, MO_EXTREMITYFIST, 1, -1);
					return;
				} else if (sd->bl.id == target_id) {
					clif_skillinfo(sd, MO_EXTREMITYFIST, 1, -1);
					return;
				}
			}
		} 
		else if (skillnum == TK_JUMPKICK) {
			if (sd->sc_data[SC_COMBO].timer == -1 || sd->sc_data[SC_COMBO].val1 != TK_JUMPKICK) {
				if (!sd->state.skill_flag) {
					sd->state.skill_flag = 1;
					clif_skillinfo(sd, TK_JUMPKICK, 1, -1);
					return;
				} else if (sd->bl.id == target_id) {
					clif_skillinfo(sd, TK_JUMPKICK, 1, -1);
					return;
				}
			}
		}
		if ((lv = pc_checkskill(sd, skillnum)) > 0) {
			if (skilllv > lv)
				skilllv = lv;
			skill_use_id(sd, target_id, skillnum, skilllv);
			if (sd->state.skill_flag)
				sd->state.skill_flag = 0;
		}
	}

	return;
}

/*==========================================
 * スキル使用（場所指定）
 *------------------------------------------
 */
void clif_parse_UseSkillToPos(int fd, struct map_session_data *sd) { // S 0x0116 <level>.w <skill ID>.w <X>.w <Y>.w
                                                                     // S 0x0190 <skill lv>.w <skill code>.w <X>.w <Y>.w <message>.80b
	short skillnum, skilllv, x, y;
	int lv, skillmoreinfo;

//	nullpo_retv(sd); // checked before to call function

//	if (map[sd->bl.m].flag.noskill) return;
	if (sd->bl.prev == NULL || sd->chatID || sd->npc_id || sd->vender_id)
		return;

	if (pc_issit(sd)) // client can not send useskill packet if player is sit down. But with hacker and latency... do a check
		return; // to avoid: have regeneration of a sitting man, but not be sit down.

	skillmoreinfo = -1;
	switch (sd->packet_ver) { // 0: old, 1: 7july04, 2: 13july04, 3: 26july04, 4: 9aug04/16aug04/17aug04, 5: 6sept04, 6: 21sept04, 7: 18oct04, 8: 25oct04/08nov04, 9: 6dec04, 10: 10jan05, 11: 9may05, 12: 28jun05, 13: 4april06 (by [Yor])
	case 13:
		skilllv = RFIFOW(fd,5);
		skillnum = RFIFOW(fd,15);
		x = RFIFOW(fd,29);
		y = RFIFOW(fd,38);
		if (RFIFOW(fd,0) == 0x07e)
			skillmoreinfo = 40;
		break;
	case 12:
		skilllv = RFIFOW(fd,12);
		skillnum = RFIFOW(fd,15);
		x = RFIFOW(fd,18);
		y = RFIFOW(fd,31);
		if (RFIFOW(fd,0) == 0x07e)
			skillmoreinfo = 33;
		break;
	case 11:
		skilllv = RFIFOW(fd,5);
		skillnum = RFIFOW(fd,9);
		x = RFIFOW(fd,12);
		y = RFIFOW(fd,20);
		if (RFIFOW(fd,0) == 0x07e)
			skillmoreinfo = 22;
		break;
	case 10:
		skilllv = RFIFOW(fd,10);
		skillnum = RFIFOW(fd,18);
		x = RFIFOW(fd,22);
		y = RFIFOW(fd,32);
		if (RFIFOW(fd,0) == 0x07e)
			skillmoreinfo = 34;
		break;
	case 9:
		skilllv = RFIFOW(fd,4);
		skillnum = RFIFOW(fd,9);
		x = RFIFOW(fd,22);
		y = RFIFOW(fd,28);
		if (RFIFOW(fd,0) == 0x113)
			skillmoreinfo = 30;
		break;
	case 8:
		skilllv = RFIFOW(fd,6);
		skillnum = RFIFOW(fd,9);
		x = RFIFOW(fd,23);
		y = RFIFOW(fd,26);
		if (RFIFOW(fd,0) == 0x08c)
			skillmoreinfo = 28;
		break;
	case 7:
		skilllv = RFIFOW(fd,3); // 2? to check.
		skillnum = RFIFOW(fd,6);
		x = RFIFOW(fd,17);
		y = RFIFOW(fd,21);
		if (RFIFOW(fd,0) == 0x08c)
			skillmoreinfo = 23;
		break;
	case 6:
		skilllv = RFIFOW(fd,6); // 16? to check.
		skillnum = RFIFOW(fd,20);
		x = RFIFOW(fd,23);
		y = RFIFOW(fd,27);
		if (RFIFOW(fd,0) == 0x08c)
			skillmoreinfo = 29;
		break;
	case 5:
		skilllv = RFIFOW(fd,10);
		skillnum = RFIFOW(fd,14);
		x = RFIFOW(fd,18);
		y = RFIFOW(fd,23);
		if (RFIFOW(fd,0) == 0x08c)
			skillmoreinfo = 25;
		break;
	case 4:
		skilllv = RFIFOW(fd,5);
		skillnum = RFIFOW(fd,15);
		x = RFIFOW(fd,29);
		y = RFIFOW(fd,38);
		if (RFIFOW(fd,0) == 0x0a2)
			skillmoreinfo = 40;
		break;
	case 3:
		skilllv = RFIFOW(fd,3);
		skillnum = RFIFOW(fd,6);
		x = RFIFOW(fd,17);
		y = RFIFOW(fd,21);
		if (RFIFOW(fd,0) == 0x0a2)
			skillmoreinfo = 23;
		break;
	case 2:
		skilllv = RFIFOW(fd,7);
		skillnum = RFIFOW(fd,9);
		x = RFIFOW(fd,15);
		y = RFIFOW(fd,17);
		if (RFIFOW(fd,0) == 0x190)
			skillmoreinfo = 19;
		break;
	case 1:
		skilllv = RFIFOW(fd,4);
		skillnum = RFIFOW(fd,9);
		x = RFIFOW(fd,11);
		y = RFIFOW(fd,13);
		if (RFIFOW(fd,0) == 0x190)
			skillmoreinfo = 15;
		break;
	default: // old version by default
		skilllv = RFIFOW(fd,2);
		skillnum = RFIFOW(fd,4);
		x = RFIFOW(fd,6);
		y = RFIFOW(fd,8);
		if (RFIFOW(fd,0) == 0x190)
			skillmoreinfo = 10;
		break;
	}

	if (skillnotok(skillnum, sd))
		return;

	if (skillmoreinfo != -1) {
		if (pc_issit(sd)) {
			clif_skill_fail(sd, skillnum, 0, 0);
			return;
		}
		memset(talkie_mes, 0, sizeof(talkie_mes)); // 80 + NULL
		strncpy(talkie_mes, RFIFOP(fd, skillmoreinfo), 80); // 80 + NULL
	}

	if (sd->skilltimer != -1)
		return;
	else if (DIFF_TICK(gettick(), sd->canact_tick) < 0 &&
	         // allow monk combos to ignore this delay [celest]
	         !(sd->sc_count && sd->sc_data[SC_COMBO].timer!=-1 &&
	          (skillnum == MO_EXTREMITYFIST ||
	           skillnum == MO_CHAINCOMBO ||
	           skillnum == MO_COMBOFINISH ||
	           skillnum == CH_PALMSTRIKE ||
	           skillnum == CH_TIGERFIST ||
	           skillnum == CH_CHAINCRUSH))) {
		clif_skill_fail(sd, skillnum, 4, 0);
		return;
	}

	if ((sd->sc_data[SC_TRICKDEAD].timer != -1 && skillnum != NV_TRICKDEAD) ||
	    sd->sc_data[SC_BERSERK].timer != -1 || sd->sc_data[SC_NOCHAT].timer != -1 ||
	    sd->sc_data[SC_WEDDING].timer != -1 || sd->view_class == 22)
		return;

	pc_delinvincibletimer(sd);
	if (sd->skillitem >= 0 && sd->skillitem == skillnum) {
		if (skilllv != sd->skillitemlv)
			skilllv = sd->skillitemlv;
		skill_use_pos(sd, x, y, skillnum, skilllv);
	} else {
		sd->skillitem = sd->skillitemlv = -1;
		if ((lv = pc_checkskill(sd, skillnum)) > 0) {
			if (skilllv > lv)
				skilllv = lv;
			skill_use_pos(sd, x, y, skillnum, skilllv);
		}
	}

	return;
}

/*==========================================
 * スキル使用（map指定）
 *------------------------------------------
 */
void clif_parse_UseSkillMap(int fd, struct map_session_data *sd) { // S 0x011b <skill ID>.w <map name>.16B
	char mapname[17]; // 16 + NULL

//	nullpo_retv(sd); // checked before to call function

//	if (map[sd->bl.m].flag.noskill) return;

	if (sd->bl.prev == NULL || sd->chatID != 0 || sd->npc_id != 0 || sd->vender_id != 0 ||
	    sd->sc_data[SC_TRICKDEAD].timer != -1 ||
	    sd->sc_data[SC_BERSERK].timer != -1 ||
	    sd->sc_data[SC_NOCHAT].timer != -1 ||
	    sd->sc_data[SC_WEDDING].timer != -1 ||
	    sd->view_class == 22)
		return;

	pc_delinvincibletimer(sd);

	memset(mapname, 0, sizeof(mapname));
	strncpy(mapname, RFIFOP(fd,4), 16);
	skill_castend_map(sd, RFIFOW(fd,2), mapname);

	return;
}

/*==========================================
 * メモ要求
 *------------------------------------------
 */
void clif_parse_RequestMemo(int fd, struct map_session_data *sd) { // S 0x011d
//	nullpo_retv(sd); // checked before to call function

	pc_memo(sd); // ,-1); // always -1

	return;
}

/*==========================================
 * アイテム合成
 *------------------------------------------
 */
void clif_parse_ProduceMix(int fd, struct map_session_data *sd) { // S 0x018e <MakeItemID>.w <slot1ItemID>.w <slot2ItemID>,w <slot3.ItemID>.w
//	nullpo_retv(sd); // checked before to call function

	if (!sd->state.produce_flag) //Check if player casted the required skill, to avoid macros
		return;

	skill_produce_mix(sd, RFIFOW(fd,2), RFIFOW(fd,4), RFIFOW(fd,6), RFIFOW(fd,8));
	sd->state.produce_flag = 0;

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_NpcSelectMenu(int fd, struct map_session_data *sd) { // S 0x00b8 <ID>.l <select>.B
//	nullpo_retv(sd); // checked before to call function

	sd->npc_menu = RFIFOB(fd,6);
	npc_scriptcont(sd, RFIFOL(fd,2));

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_NpcNextClicked(int fd, struct map_session_data *sd) { // S 0x00b9 <ID>.l
//	nullpo_retv(sd); // checked before to call function

	npc_scriptcont(sd, RFIFOL(fd,2));

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_NpcAmountInput(int fd, struct map_session_data *sd) { // S 0143 <ID>.l <amount>.l
//	nullpo_retv(sd); // checked before to call function

#define RFIFOL_(fd, pos) (*(int*)(session[fd]->rdata + session[fd]->rdata_pos + (pos)))
	// Input Value overflow Exploit FIX
	sd->npc_amount = RFIFOL_(fd,6); // fixed by Lupus. npc_amount is (int) but was RFIFOL changing it to (unsigned int)
#undef RFIFOL_

	npc_scriptcont(sd, RFIFOL(fd,2));

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_NpcStringInput(int fd, struct map_session_data *sd) { // S 0x01d5 <len>.w <ID>.l <input>.?B 00
	char *input_str;
	unsigned short input_length;

//	nullpo_retv(sd); // checked before to call function

	input_length = RFIFOW(fd,2) - 8; // in packet, included NULL, but hacker can not
	CALLOC(input_str, char, input_length + 1); // + NULL
	strncpy(input_str, RFIFOP(fd,8), input_length);
	input_length = strlen(input_str);
	memset(sd->npc_str, 0, sizeof(sd->npc_str)); // npc_str = 255 + NULL
	if (input_length > 255) // npc_str = 255 + NULL
		input_length = 255; // npc_str = 255 + NULL
	strncpy(sd->npc_str, input_str, input_length); // npc_str = 255 + NULL
	FREE(input_str);
	npc_scriptcont(sd, RFIFOL(fd,4));

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_parse_NpcCloseClicked(int fd, struct map_session_data *sd) { // S 0146 <ID>.l
//	nullpo_retv(sd); // checked before to call function

	npc_scriptcont(sd, RFIFOL(fd,2));

	return;
}

/*==========================================
 * アイテム鑑定
 *------------------------------------------
 */
void clif_parse_ItemIdentify(int fd, struct map_session_data *sd) { // S 0x0178 <index>.w
//	nullpo_retv(sd); // checked before to call function

	pc_item_identify(sd, RFIFOW(fd,2) - 2);

	return;
}

/*==========================================
 * 矢作成
 *------------------------------------------
 */
void clif_parse_SelectArrow(int fd, struct map_session_data *sd) { // S 0x01ae <itemID>.w
//	nullpo_retv(sd); // checked before to call function

	sd->state.make_arrow_flag = 0;
	skill_arrow_create(sd, RFIFOW(fd,2));

	return;
}

/*==========================================
 * オートスペル受信
 *------------------------------------------
 */
void clif_parse_AutoSpell(int fd, struct map_session_data *sd) { // S 0x01ce <sid>.l
//	nullpo_retv(sd); // checked before to call function

	skill_autospell(sd, RFIFOL(fd,2));

	return;
}

/*==========================================
 * カード使用
 *------------------------------------------
 */
void clif_parse_UseCard(int fd, struct map_session_data *sd) { // S 0x017A <index>.w
//	nullpo_retv(sd); // checked before to call function

	clif_use_card(sd, RFIFOW(fd,2) - 2);

	return;
}

/*==========================================
 * カード挿入装備選択
 *------------------------------------------
 */
void clif_parse_InsertCard(int fd, struct map_session_data *sd) { // S 0x017C <SrcIndex>.w <DescIndex>.w
//	nullpo_retv(sd); // checked before to call function

	pc_insert_card(sd, RFIFOW(fd,2) - 2, RFIFOW(fd,4) - 2);

	return;
}

/*==========================================
 * 0193 キャラID名前引き
 *------------------------------------------
 */
void clif_parse_SolveCharName(int fd, struct map_session_data *sd) { // S 0x0193 <ID>.l
//	nullpo_retv(sd); // checked before to call function

	switch (sd->packet_ver) { // 0: old, 1: 7july04, 2: 13july04, 3: 26july04, 4: 9aug04/16aug04/17aug04, 5: 6sept04, 6: 21sept04, 7: 18oct04, 8: 25oct04/08nov04, 9: 6dec04, 10: 10jan05, 11: 9may05, 12: 28jun05, 13: 4april06 (by [Yor])
	case 13:
		clif_solved_charname(sd, RFIFOL(fd,7));
		break;
	case 12:
		clif_solved_charname(sd, RFIFOL(fd,5));
		break;
	case 11:
		clif_solved_charname(sd, RFIFOL(fd,11));
		break;
	case 10:
		clif_solved_charname(sd, RFIFOL(fd,7));
		break;
	case 9:
		clif_solved_charname(sd, RFIFOL(fd,10));
		break;
	case 8:
		clif_solved_charname(sd, RFIFOL(fd,12));
		break;
	case 7:
		clif_solved_charname(sd, RFIFOL(fd,8));
		break;
	case 6:
		clif_solved_charname(sd, RFIFOL(fd,6));
		break;
	case 5:
		clif_solved_charname(sd, RFIFOL(fd,10));
		break;
	case 4:
		clif_solved_charname(sd, RFIFOL(fd,7));
		break;
	case 3:
		clif_solved_charname(sd, RFIFOL(fd,8));
		break;
	default: // old version by default (+ packet version 1 and 2)
		clif_solved_charname(sd, RFIFOL(fd,2));
		break;
	}

	return;
}

/*==========================================
 * 0197 /resetskill /resetstate
 *------------------------------------------
 */
void clif_parse_ResetChar(int fd, struct map_session_data *sd) { // S 0x0197 <type>.w
	char output[50]; // 24 (name) + 1 + 1 + 1 + 11 (@resetstate) + NULL = 39

//	nullpo_retv(sd); // checked before to call function

	memset(output, 0, sizeof(output));

	switch(RFIFOW(fd,2)) {
	case 0:
		sprintf(output, "%s : %cresetstate", sd->status.name, GM_Symbol());
		is_atcommand(fd, sd, output, sd->GM_level); // no nothing if command doesn't work
		break;
	case 1:
		sprintf(output, "%s : %cresetskill", sd->status.name, GM_Symbol());
		is_atcommand(fd, sd, output, sd->GM_level); // no nothing if command doesn't work
		break;
	}

	return;
}

/*==========================================
 * 019c /lb /nlb /mb (/mb adds 'blue')
 *------------------------------------------
 */
void clif_parse_LGMmessage(int fd, struct map_session_data *sd) { // S 0c019c <len>.w <message>.?B
	char *message;
	char *message_to_gm;
	char *output;
	char *p_message;
	int i, message_size, name_length;

//	nullpo_retv(sd); // checked before to call function

	// get size
	message_size = RFIFOW(fd,2) - 4;

	// check size of message
	if (message_size < 1)
		return;

	// get message
	CALLOC(message, char, message_size + 1); // (message) + NULL
	strncpy(message, RFIFOP(fd,4), message_size);
	// remove final NULL from message_size
	i = strlen(message);
	if (i < message_size) // note: normal client always sends final NULL, so i = message_size - 1. If more, it's abnormal
		message_size = i;

	// check size of message again
	if (message_size < 1) { // send a void message?
		FREE(message);
		return;
	} else if (message_size > battle_config.max_message_length) { // more than the authorized?
		printf("Possible hack on local GM message: character '%s' (account: %d) tries to send a big message (%d characters).\n", sd->status.name, sd->status.account_id, message_size);
		CALLOC(message_to_gm, char, message_size + MAX_MSG_LEN + 100); // message_size + max size of msg_txt + security (char name, char id, etc...) (100)
		// information is sended to all online GM
		sprintf(message_to_gm, msg_txt(645), sd->status.name, sd->status.account_id, message_size); // Possible hack on local GM message: character '%s' (account: %d) tries to send a big message (%d characters).
		intif_wis_message_to_gm(wisp_server_name, battle_config.hack_info_GM_level, message_to_gm);
		intif_wis_message_to_gm(wisp_server_name, battle_config.hack_info_GM_level, msg_txt(641)); //  This player hasn't been banned.
		FREE(message_to_gm);
		FREE(message);
		return;
	}

	//printf("clif_parse_LGMmessage: message: '%s'.\n", message); // structure of message: <player_name>: <message> or <message> or blue<message>

	// works on /mb and remove 'blue'
	name_length = strlen(sd->status.name);
	p_message = message;
	// if not /lb
	if (strncmp(message, sd->status.name, name_length) != 0 || // check player name
	    message_size <= name_length + 3 || // check void message (at least 1 char) - normal client refuse to send void message
	    *(message + name_length) != ':' || *(message + name_length + 1) != ' ') { // check ': '
		// so, it's /nlb or /mb -- > check 'blue'
		if (strncmp(message, "blue", 4) == 0) {
			//printf("It's /mb\n");
			p_message = p_message + 4;
			message_size = message_size - 4;
			if (message_size < 1) {
				FREE(message);
				return;
			}
		}
	}

	// do GM command
	CALLOC(output, char, message_size + 53); // name(24) + " : "(3) + @nlb(4) + " "(1) + message(x) + NULL(1) + security(20) = 53 + len of message
	sprintf(output, "%s : %cnlb %s", sd->status.name, GM_Symbol(), p_message);
	//printf("clif_parse_LGMmessage: %s\n", output);
	is_atcommand(fd, sd, output, sd->GM_level); // no nothing if command doesn't work
	FREE(output);

	// don't check bad words (is_atcommand already do the check)
/*	if (check_bad_word(p_message, message_size, sd)) {
		FREE(message);
		return;
	}*/

	FREE(message);

	return;
}

/*==========================================
 * カプラ倉庫へ入れる
 *------------------------------------------
 */
void clif_parse_MoveToKafra(int fd, struct map_session_data *sd) { // S 0x00f3 <index>.w <amount>.l
	short item_index;
	int item_amount;

//	nullpo_retv(sd); // checked before to call function

	if (sd->npc_id != 0 || sd->vender_id != 0)
		return;

	switch (sd->packet_ver) { // 0: old, 1: 7july04, 2: 13july04, 3: 26july04, 4: 9aug04/16aug04/17aug04, 5: 6sept04, 6: 21sept04, 7: 18oct04, 8: 25oct04/08nov04, 9: 6dec04, 10: 10jan05, 11: 9may05, 12: 28jun05, 13: 4april06 (by [Yor])
	case 13:
		item_index = RFIFOW(fd,5) - 2;
		item_amount = RFIFOL(fd,19);
		break;
	case 12:
		item_index = RFIFOW(fd,16) - 2;
		item_amount = RFIFOL(fd,27);
		break;
	case 11:
		item_index = RFIFOW(fd,7) - 2;
		item_amount = RFIFOL(fd,10);
		break;
	case 10:
		item_index = RFIFOW(fd,10) - 2;
		item_amount = RFIFOL(fd,16);
		break;
	case 9:
		item_index = RFIFOW(fd,4) - 2;
		item_amount = RFIFOL(fd,10);
		break;
	case 8:
		item_index = RFIFOW(fd,6) - 2;
		item_amount = RFIFOL(fd,9);
		break;
	case 7:
		item_index = RFIFOW(fd,5) - 2;
		item_amount = RFIFOL(fd,12);
		break;
	case 6:
		item_index = RFIFOW(fd,6) - 2;
		item_amount = RFIFOL(fd,21);
		break;
	case 5:
		item_index = RFIFOW(fd,3) - 2;
		item_amount = RFIFOL(fd,15);
		break;
	case 4:
		item_index = RFIFOW(fd,5) - 2;
		item_amount = RFIFOL(fd,19);
		break;
	case 3:
		item_index = RFIFOW(fd,5) - 2;
		item_amount = RFIFOL(fd,12);
		break;
	default: // old version by default (+ packet version 1 and 2)
		item_index = RFIFOW(fd,2) - 2;
		item_amount = RFIFOL(fd,4);
		break;
	}

	if (item_index < 0 || item_index >= MAX_INVENTORY)
		return;
	if (item_amount <= 0 || item_amount > sd->status.inventory[item_index].amount)
		return;

	if (sd->state.storage_flag)
		storage_guild_storageadd(sd, item_index, item_amount);
	else
		storage_storageadd(sd, item_index, item_amount);

	return;
}

/*==========================================
 * カプラ倉庫から出す
 *------------------------------------------
 */
void clif_parse_MoveFromKafra(int fd, struct map_session_data *sd) { // S 0x00f5 <index>.w <amount>.l
	short item_index;
	int item_amount;

//	nullpo_retv(sd); // checked before to call function

	if (sd->npc_id != 0 || sd->vender_id != 0)
		return;

	switch (sd->packet_ver) { // 0: old, 1: 7july04, 2: 13july04, 3: 26july04, 4: 9aug04/16aug04/17aug04, 5: 6sept04, 6: 21sept04, 7: 18oct04, 8: 25oct04/08nov04, 9: 6dec04, 10: 10jan05, 11: 9may05, 12: 28jun05, 13: 4april06 (by [Yor])
	case 13:
		item_index = RFIFOW(fd,11) - 1;
		item_amount = RFIFOL(fd,22);
		break;
	case 12:
		item_index = RFIFOW(fd,11) - 1;
		item_amount = RFIFOL(fd,14);
		break;
	case 11:
		item_index = RFIFOW(fd,14) - 1;
		item_amount = RFIFOL(fd,18);
		break;
	case 10:
		item_index = RFIFOW(fd,11) - 1;
		item_amount = RFIFOL(fd,17);
		break;
	case 9:
		item_index = RFIFOW(fd,4) - 1;
		item_amount = RFIFOL(fd,17);
		break;
	case 8:
		item_index = RFIFOW(fd,12) - 1;
		item_amount = RFIFOL(fd,18);
		break;
	case 7:
		item_index = RFIFOW(fd,10) - 1;
		item_amount = RFIFOL(fd,22);
		break;
	case 6:
		item_index = RFIFOW(fd,4) - 1;
		item_amount = RFIFOL(fd,8);
		break;
	case 5:
		item_index = RFIFOW(fd,3) - 1;
		item_amount = RFIFOL(fd,13);
		break;
	case 4:
		item_index = RFIFOW(fd,11) - 1;
		item_amount = RFIFOL(fd,22);
		break;
	case 3:
		item_index = RFIFOW(fd,10) - 1;
		item_amount = RFIFOL(fd,22);
		break;
	default: // old version by default (+ packet version 1 and 2)
		item_index = RFIFOW(fd,2) - 1;
		item_amount = RFIFOL(fd,4);
		break;
	}

	if (sd->state.storage_flag) {
		// check index and quantity
		if (item_index < 0 || item_index >= MAX_GUILD_STORAGE ||
		    item_amount <= 0)
			return;
		storage_guild_storageget(sd, item_index, item_amount);
	} else {
		// check index and quantity
		if (item_index < 0 || item_index >= MAX_STORAGE ||
		    item_amount <= 0)
			return;
		storage_storageget(sd, item_index, item_amount);
	}

	return;
}

/*==========================================
 * カプラ倉庫へカートから入れる
 *------------------------------------------
 */
void clif_parse_MoveToKafraFromCart(int fd, struct map_session_data *sd) { // S 0x0129 <index>.w <amount>.l
//	nullpo_retv(sd); // checked before to call function

	if (sd->npc_id != 0 || sd->vender_id != 0 || sd->trade_partner != 0)
		return;

	if (sd->state.storage_flag)
		storage_guild_storageaddfromcart(sd, RFIFOW(fd,2) - 2, RFIFOL(fd,4));
	else
		storage_storageaddfromcart(sd, RFIFOW(fd,2) - 2, RFIFOL(fd,4));

	return;
}

/*==========================================
 * カプラ倉庫から出す
 *------------------------------------------
 */
void clif_parse_MoveFromKafraToCart(int fd, struct map_session_data *sd) { // S 0x0128 <index>.w <amount>.l
//	nullpo_retv(sd); // checked before to call function

	if (sd->npc_id != 0 || sd->vender_id != 0)
		return;

	if (sd->state.storage_flag)
		storage_guild_storagegettocart(sd, RFIFOW(fd,2) - 1, RFIFOL(fd,4));
	else
		storage_storagegettocart(sd, RFIFOW(fd,2) - 1, RFIFOL(fd,4));

	return;
}

/*==========================================
 * カプラ倉庫を閉じる
 *------------------------------------------
 */
void clif_parse_CloseKafra(int fd, struct map_session_data *sd) { // S 0x00f7
//	nullpo_retv(sd); // checked before to call function

	if (sd->state.storage_flag)
		storage_guild_storageclose(sd);
	else
		storage_storageclose(sd);

	return;
}

/*==========================================
 * パーティを作る
 *------------------------------------------
 */
void clif_parse_CreateParty(int fd, struct map_session_data *sd) { // S 0x00f9 <party name>.24B
//	nullpo_retv(sd); // checked before to call function

//	if (battle_config.basic_skill_check == 0 || pc_checkskill(sd, NV_BASIC) >= 7) { // other solution speeder
	if (battle_config.basic_skill_check == 0 || (sd->status.skill[NV_BASIC].id == NV_BASIC && sd->status.skill[NV_BASIC].lv >= 7)) {
		party_create(sd, RFIFOP(fd,2), 0, 0);
	} else
		clif_skill_fail(sd, 1, 0, 4);

	return;
}

/*==========================================
 * パーティを作る
 *------------------------------------------
 */
void clif_parse_CreateParty2(int fd, struct map_session_data *sd) { // S 0x01e8 <party name>.24B <item1>B <item2>B
//	nullpo_retv(sd); // checked before to call function

//	if (battle_config.basic_skill_check == 0 || pc_checkskill(sd, NV_BASIC) >= 7) { // other solution speeder
	if (battle_config.basic_skill_check == 0 || (sd->status.skill[NV_BASIC].id == NV_BASIC && sd->status.skill[NV_BASIC].lv >= 7)) {
		party_create(sd, RFIFOP(fd,2), RFIFOB(fd,26), RFIFOB(fd,27));
	} else
		clif_skill_fail(sd, 1, 0, 4);

	return;
}

/*==========================================
 * パーティに勧誘
 *------------------------------------------
 */
void clif_parse_PartyInvite(int fd, struct map_session_data *sd) { // S 0x00fc <ID>.l
//	nullpo_retv(sd); // checked before to call function

	party_invite(sd, RFIFOL(fd,2));

	return;
}

/*==========================================
 * パーティ勧誘返答
 *------------------------------------------
 */
void clif_parse_ReplyPartyInvite(int fd, struct map_session_data *sd) { // S 0x00ff <ID>.l <fail>.l
//	nullpo_retv(sd); // checked before to call function

//	if (battle_config.basic_skill_check == 0 || pc_checkskill(sd, NV_BASIC) >= 5) { // other solution speeder
	if (battle_config.basic_skill_check == 0 || (sd->status.skill[NV_BASIC].id == NV_BASIC && sd->status.skill[NV_BASIC].lv >= 5)) {
		party_reply_invite(sd, RFIFOL(fd,2), RFIFOL(fd,6)); // 0: invitation was denied, 1: accepted
	} else {
		party_reply_invite(sd, RFIFOL(fd,2), 0); // 0: invitation was denied, 1: accepted
		clif_skill_fail(sd, 1, 0, 4);
	}

	return;
}

/*==========================================
 * パーティ脱退要求
 *------------------------------------------
 */
void clif_parse_LeaveParty(int fd, struct map_session_data *sd) { // S 0x0100
//	nullpo_retv(sd); // checked before to call function

	party_leave(sd);

	return;
}

/*==========================================
 * パーティ除名要求
 *------------------------------------------
 */
void clif_parse_RemovePartyMember(int fd, struct map_session_data *sd) { // S 0x0103 <??>.l <nick>.24B
//	nullpo_retv(sd); // checked before to call function

	party_removemember(sd, RFIFOL(fd,2), RFIFOP(fd,6));

	return;
}

/*==========================================
 * パーティ設定変更要求
 *------------------------------------------
 */
void clif_parse_PartyChangeOption(int fd, struct map_session_data *sd) { // S 0102 <exp>.w <item?>.w
//	nullpo_retv(sd); // checked before to call function

	party_changeoption(sd, RFIFOW(fd,2), RFIFOW(fd,4));

	return;
}

/*==========================================
 * パーティメッセージ送信要求
 *------------------------------------------
 */
void clif_parse_PartyMessage(int fd, struct map_session_data *sd) { // S 0x0108 <len>.w <message>.?B
	char *message;
	char *message_to_gm;
	char *p_message;
	int i, message_size, name_length;

//	nullpo_retv(sd); // checked before to call function

	if (sd->sc_data[SC_BERSERK].timer != -1 || //バーサーク時は会話も不可
	    sd->sc_data[SC_NOCHAT].timer != -1) //チャット禁止
		return;

	// normal client can not send party message if player is not in a party
	if (sd->status.party_id == 0)
		return;

	// get size
	message_size = RFIFOW(fd,2) - 4;

	// check size of message
	if (message_size < 1)
		return;

	// get message
	CALLOC(message, char, message_size + 1); // +1 for null terminated if necessary
	strncpy(message, RFIFOP(fd, 4), message_size);
	// remove final NULL from message_size
	i = strlen(message);
	if (i < message_size) // note: normal client always sends final NULL, so i = message_size - 1. If more, it's abnormal
		message_size = i;

	// check size of message again
	if (message_size < 1) { // send a void message?
		FREE(message);
		return;
	} else if (message_size > battle_config.max_message_length) { // more than the authorized?
		printf("Possible hack on party message: character '%s' (account: %d) tries to send a big message (%d characters).\n", sd->status.name, sd->status.account_id, message_size);
		CALLOC(message_to_gm, char, message_size + MAX_MSG_LEN + 100); // message_size + max size of msg_txt + security (char name, char id, etc...) (100)
		// information is sended to all online GM
		sprintf(message_to_gm, msg_txt(646), sd->status.name, sd->status.account_id, message_size); // Possible hack on party message: character '%s' (account: %d) tries to send a big message (%d characters).
		intif_wis_message_to_gm(wisp_server_name, battle_config.hack_info_GM_level, message_to_gm);
		intif_wis_message_to_gm(wisp_server_name, battle_config.hack_info_GM_level, msg_txt(641)); //  This player hasn't been banned.
		FREE(message_to_gm);
		FREE(message);
		return;
	}

	//printf("clif_parse_PartyMessage: message: '%s'.\n", message); // structure of message: <player_name> : <message>
	name_length = strlen(sd->status.name);
	p_message = message + name_length;
	if (strncmp(message, sd->status.name, name_length) != 0 || // check player name
	    message_size <= name_length + 3 || // check void message (at least 1 char) - normal client refuse to send void message
	    *p_message != ' ' || *(p_message + 1) != ':' || *(p_message + 2) != ' ') { // check ' : '
		CALLOC(message_to_gm, char, message_size + MAX_MSG_LEN + 100); // message_size + max size of msg_txt + security (char name, char id, etc...) (100)
		printf("Hack on party message: character '%s' (account: %d), use an other name to send a (party) message.\n", sd->status.name, sd->status.account_id);

		// information is sended to all online GM
		sprintf(message_to_gm, msg_txt(652), sd->status.name, sd->status.account_id); // Hack on party message: character '%s' (account: %d) uses an other name.
		intif_wis_message_to_gm(wisp_server_name, battle_config.hack_info_GM_level, message_to_gm);
		if (message[0] == '\0')
			strcpy(message_to_gm, msg_txt(505)); //  This player sends a void name and a void message.
		else
			sprintf(message_to_gm, msg_txt(506), message); //  This player sends (name:message): '%s'.
		intif_wis_message_to_gm(wisp_server_name, battle_config.hack_info_GM_level, message_to_gm);
		// if we block people
		if (battle_config.ban_spoof_namer < 1) {
			chrif_char_ask_name(-1, sd->status.name, 1, 0, 0, 0, 0, 0, 0); // type: 1 - block
			clif_setwaitclose(fd); // forced to disconnect because of the hack
			// message about the ban
			sprintf(message_to_gm, msg_txt(540)); //  This player has been definitivly blocked.
		// if we ban people
		} else if (battle_config.ban_spoof_namer > 0) {
			chrif_char_ask_name(-1, sd->status.name, 2, 0, 0, 0, 0, battle_config.ban_spoof_namer, 0); // type: 2 - ban (year, month, day, hour, minute, second)
			clif_setwaitclose(fd); // forced to disconnect because of the hack
			// message about the ban
			sprintf(message_to_gm, msg_txt(507), battle_config.ban_spoof_namer); //  This player has been banned for %d minute(s).
		} else {
			// message about the ban
			sprintf(message_to_gm, msg_txt(508)); //  This player hasn't been banned (Ban option is disabled).
		}
		intif_wis_message_to_gm(wisp_server_name, battle_config.hack_info_GM_level, message_to_gm);
		FREE(message_to_gm);
		// but for the hacker, we display on his screen (he sees/looks no difference).
		WPACKETW(0) = 0x109;
		WPACKETW(2) = message_size + 8 + 1; // +1 (NULL)
		WPACKETL(4) = sd->status.account_id;
		strncpy(WPACKETP(8), message, message_size);
		WPACKETB(message_size + 8) = 0; // NULL
		SENDPACKET(sd->fd, message_size + 8 + 1); // only for the hacker // +1 (NULL)
	} else {
		// check GM command
		if (is_atcommand(fd, sd, message, sd->GM_level) != AtCommand_None) {
			FREE(message);
			return;
		}

		// remove space at start for check
		p_message = p_message + 3; // position to message (+ ' : ')
		while(*p_message && isspace(*p_message))
			p_message++;

		// check bad words (after is_atcommand check, because player can have a bad name)
		if (check_bad_word(p_message, message_size, sd)) {
			FREE(message);
			return;
		}

		party_send_message(sd, message, message_size + 1);
	}

	FREE(message);

	return;
}

/*==========================================
 * 露店閉鎖
 *------------------------------------------
 */
void clif_parse_CloseVending(int fd, struct map_session_data *sd) { // S 012e
//	nullpo_retv(sd); // checked before to call function

	vending_closevending(sd);

	return;
}

/*==========================================
 * 露店アイテムリスト要求
 *------------------------------------------
 */
void clif_parse_VendingListReq(int fd, struct map_session_data *sd) { // S 0x0130 <ID>.l
//	nullpo_retv(sd); // checked before to call function

	vending_vendinglistreq(sd, RFIFOL(fd,2));
	if (sd->npc_id)
		npc_event_dequeue(sd);

	return;
}

/*==========================================
 * 露店アイテム購入
 *------------------------------------------
 */
void clif_parse_PurchaseReq(int fd, struct map_session_data *sd) { // S 0x0134 <len>.w <ID>.l {<amount>.w <index>.w}.4B*
//	nullpo_retv(sd); // checked before to call function

	if(sd->npc_id != 0 || sd->trade_partner != 0)
		return;

	vending_purchasereq(sd, RFIFOW(fd,2), RFIFOL(fd,4), RFIFOP(fd,8));

	return;
}

/*==========================================
 * 露店開設
 *------------------------------------------
 */
void clif_parse_OpenVending(int fd, struct map_session_data *sd) { // S 0x01b2 <len>.w <message>.80B <flag>.B {<index>.w <amount>.w <value>.l}.8B*
//	nullpo_retv(sd); // checked before to call function

	vending_openvending(sd, RFIFOW(fd,2), RFIFOP(fd,4), RFIFOB(fd,84), RFIFOP(fd,85));

	return;
}

/*==========================================
 * /monster /item
 *------------------------------------------
 */
void clif_parse_GM_Monster_Item(int fd, struct map_session_data *sd) { // S 0x13f <item|mob_name>.24B
	char monster_item_name[25]; // 24 + NULL
	char output[70]; // 24 (name) + 1 + 1 + 1 + 6 (@spawn/@item) + 1 + 24 + 1 + NULL = 60

//	nullpo_retv(sd); // checked before to call function

	memset(monster_item_name, 0, sizeof(monster_item_name));
	memset(output, 0, sizeof(output));

	strncpy(monster_item_name, RFIFOP(fd,2), 24);
	if (itemdb_searchname(monster_item_name) != NULL) { // item first for emperium (monster and item, and monster can not be spawned)
		sprintf(output, "%s : %citem %s", sd->status.name, GM_Symbol(), monster_item_name); // as @item
		is_atcommand(fd, sd, output, sd->GM_level); // do nothing if command doesn't work
	} else if (mobdb_searchname(monster_item_name) != 0) {
		sprintf(output, "%s : %cspawn %s", sd->status.name, GM_Symbol(), monster_item_name); // as @spawn
		is_atcommand(fd, sd, output, sd->GM_level); // do nothing if command doesn't work
	} else {
		// display only if GM can do the GM command
		if (get_atcommand_level(AtCommand_Item) <= sd->GM_level || get_atcommand_level(AtCommand_Spawn) <= sd->GM_level)
			clif_displaymessage(fd, msg_txt(510)); // Unknown monster or item.
	}

	return;
}

/*==========================================
 * ギルドを作る
 *------------------------------------------
 */
void clif_parse_CreateGuild(int fd, struct map_session_data *sd) { // S 0x0165 <myaccID>.l <guild name>.24B
//	nullpo_retv(sd); // checked before to call function

	guild_create(sd, RFIFOP(fd,6));

	return;
}

/*==========================================
 * ギルドマスターかどうか確認
 *------------------------------------------
 */
void clif_parse_GuildCheckMaster(int fd, struct map_session_data *sd) { // S 0x014D
//	nullpo_retv(sd); // checked before to call function

	clif_guild_masterormember(sd);

	return;
}

/*==========================================
 * ギルド情報要求
 *------------------------------------------
 */
void clif_parse_GuildRequestInfo(int fd, struct map_session_data *sd) { // S 014F <page>.l
//	nullpo_retv(sd); // checked before to call function

	switch(RFIFOL(fd,2)) {
	case 0: // ギルド基本情報、同盟敵対情報
		clif_guild_basicinfo(sd);
		clif_guild_allianceinfo(sd);
		break;
	case 1: // メンバーリスト、役職名リスト
		clif_guild_positionnamelist(sd);
		clif_guild_memberlist(sd);
		break;
	case 2: // 役職名リスト、役職情報リスト
		clif_guild_positionnamelist(sd);
		clif_guild_positioninfolist(sd);
		break;
	case 3: // スキルリスト
		clif_guild_skillinfo(sd);
		break;
	case 4: // 追放リスト
		clif_guild_explusionlist(sd);
		break;
	default:
		if (battle_config.error_log)
			printf("clif: guild request info: unknown page %d\n", RFIFOL(fd,2));
		break;
	}

	return;
}

/*==========================================
 * ギルド役職変更
 *------------------------------------------
 */
void clif_parse_GuildChangePositionInfo(int fd, struct map_session_data *sd) { // S 0x0161 <len>.w {<index>.l <mode>.l <index>.l <exp_mode>.l <nickname>.24B}.40B*
	unsigned int i;
//	nullpo_retv(sd); // checked before to call function

	if(sd->state.gmaster_flag == NULL)
		return; //Only the guild leader should be able to perform such action, but its also possible with packet hacking

	for(i = 4; i < RFIFOW(fd,2); i += 40)
		//sd->status.guild_id, guild position index, mode (17:invite+expel, 1:invite, 16:expel), exp rate, position name
		guild_change_position(sd, RFIFOL(fd, i), RFIFOL(fd, i + 4), RFIFOL(fd, i + 12), RFIFOP(fd, i + 16));

	return;
}

/*==========================================
 * ギルドメンバ役職変更
 *------------------------------------------
 */
void clif_parse_GuildChangeMemberPosition(int fd, struct map_session_data *sd) { // S 0x0155 <len>.w {<accID>.l <charaID>.l <index>.l}.12B*
	int i;

//	nullpo_retv(sd); // checked before to call function

	if(sd->state.gmaster_flag == NULL)
		return; //Only the guild leader should be able to perform such action, but its also possible with packet hacking

	for(i = 4; i < RFIFOW(fd,2); i += 12) {
		//params: src guild_id, target account_id, target char_id, position
		guild_change_memberposition(sd->status.guild_id, RFIFOL(fd,i), RFIFOL(fd,i+4), RFIFOL(fd,i+8));
	}

	return;
}

/*==========================================
 * ギルドエンブレム要求
 *------------------------------------------
 */
void clif_parse_GuildRequestEmblem(int fd, struct map_session_data *sd) { // S 0151 <guild ID>.l
//	nullpo_retv(sd); // checked before to call function

	struct guild *g = guild_search(RFIFOL(fd,2));

	if (g != NULL)
		clif_guild_emblem(sd, g);

	return;
}

/*==========================================
 * ギルドエンブレム変更
 *------------------------------------------
 */
void clif_parse_GuildChangeEmblem(int fd, struct map_session_data *sd) { // S 0x0153 <len>.w <emblem data>.?B
//	nullpo_retv(sd); // checked before to call function

	if(sd->state.gmaster_flag == NULL)
		return; //Only the guild leader should be able to perform such action, but its also possible with packet hacking

	guild_change_emblem(sd, RFIFOW(fd,2) - 4, RFIFOP(fd,4));

	return;
}

/*==========================================
 * ギルド告知変更
 *------------------------------------------
 */
void clif_parse_GuildChangeNotice(int fd, struct map_session_data *sd) { // S 0x016E <guildID>.l <mess1>.60B <mess2>.120B
//	nullpo_retv(sd); // checked before to call function

	if(sd->state.gmaster_flag == NULL)
		return; //Only the guild leader should be able to perform such action, but its also possible with packet hacking

	guild_change_notice(sd, RFIFOL(fd,2), RFIFOP(fd,6), RFIFOP(fd,66));

	return;
}

/*==========================================
 * ギルド勧誘
 *------------------------------------------
 */
void clif_parse_GuildInvite(int fd, struct map_session_data *sd) { // S 0x0168 <TargetAccID>.l <sourceAccID>.l <myCharactorID>.l
//	nullpo_retv(sd); // checked before to call function

	guild_invite(sd, RFIFOL(fd,2));

	return;
}

/*==========================================
 * ギルド勧誘返信
 *------------------------------------------
 */
void clif_parse_GuildReplyInvite(int fd, struct map_session_data *sd) { // S 0x016B <guild ID>.l <type>.l
//	nullpo_retv(sd); // checked before to call function

	guild_reply_invite(sd, RFIFOL(fd,2), RFIFOL(fd,6));

	return;
}

/*==========================================
 * ギルド脱退
 *------------------------------------------
 */
void clif_parse_GuildLeave(int fd, struct map_session_data *sd) { // S 0x0159 <guildID>.l <accID>.l <charID>.l <mess>.40B
//	nullpo_retv(sd); // checked before to call function

	//We dont use the values sent by the client, we can just take them from sd
	guild_leave(sd, RFIFOP(fd,14)); // S 0159 <guildID>.l <accID>.l <charID>.l <mess>.40B

	return;
}

/*==========================================
 * ギルド追放
 *------------------------------------------
 */
void clif_parse_GuildExplusion(int fd, struct map_session_data *sd) { // S 0x015B <guildID>.l <accID>.l <charID>.l <mess>.40B
//	nullpo_retv(sd); // checked before to call function

	guild_explusion(sd, RFIFOL(fd,2), RFIFOL(fd,6), RFIFOL(fd,10), RFIFOP(fd,14)); // S 015B <guildID>.l <accID>.l <charID>.l <mess>.40B

	return;
}

/*==========================================
 * ギルド会話
 *------------------------------------------
 */
void clif_parse_GuildMessage(int fd, struct map_session_data *sd) { // S 0x017e <len>.w <message>.?B
	char *message;
	char *message_to_gm;
	char *p_message;
	int i, message_size, name_length;

//	nullpo_retv(sd); // checked before to call function

	if (sd->sc_data[SC_BERSERK].timer != -1 || //バーサーク時は会話も不可
	    sd->sc_data[SC_NOCHAT].timer != -1) //チャット禁止
		return;

	// normal client can not send guild message if player is not in a guild
	if (sd->status.guild_id == 0)
		return;

	// get size
	message_size = RFIFOW(fd,2) - 4;

	// check size of message
	if (message_size < 1)
		return;

	// get message
	CALLOC(message, char, message_size + 1); // +1 for null terminated if necessary
	strncpy(message, RFIFOP(fd, 4), message_size);
	// remove final NULL from message_size
	i = strlen(message);
	if (i < message_size) // note: normal client always sends final NULL, so i = message_size - 1. If more, it's abnormal
		message_size = i;

	// check size of message again
	if (message_size < 1) { // send a void message?
		FREE(message);
		return;
	} else if (message_size > battle_config.max_message_length) { // more than the authorized?
		printf("Possible hack on guild message: character '%s' (account: %d) tries to send a big message (%d characters).\n", sd->status.name, sd->status.account_id, message_size);
		CALLOC(message_to_gm, char, message_size + MAX_MSG_LEN + 100); // message_size + max size of msg_txt + security (char name, char id, etc...) (100)
		// information is sended to all online GM
		sprintf(message_to_gm, msg_txt(647), sd->status.name, sd->status.account_id, message_size); // Possible hack on guild message: character '%s' (account: %d) tries to send a big message (%d characters).
		intif_wis_message_to_gm(wisp_server_name, battle_config.hack_info_GM_level, message_to_gm);
		intif_wis_message_to_gm(wisp_server_name, battle_config.hack_info_GM_level, msg_txt(641)); //  This player hasn't been banned.
		FREE(message_to_gm);
		FREE(message);
		return;
	}

	//printf("clif_parse_GuildMessage: message: '%s'.\n", message); // structure of message: <player_name> : <message>
	name_length = strlen(sd->status.name);
	p_message = message + name_length;
	if (strncmp(message, sd->status.name, name_length) != 0 || // check player name
	    message_size <= name_length + 3 || // check void message (at least 1 char) - normal client refuse to send void message
	    *p_message != ' ' || *(p_message + 1) != ':' || *(p_message + 2) != ' ') { // check ' : '
		CALLOC(message_to_gm, char, message_size + MAX_MSG_LEN + 100); // message_size + max size of msg_txt + security (char name, char id, etc...) (100)
		printf("Hack on guild message: character '%s' (account: %d), use an other name to send a (guild) message.\n", sd->status.name, sd->status.account_id);

		// information is sended to all online GM
		sprintf(message_to_gm, msg_txt(653), sd->status.name, sd->status.account_id); // Hack on guild message: character '%s' (account: %d) uses an other name.
		intif_wis_message_to_gm(wisp_server_name, battle_config.hack_info_GM_level, message_to_gm);
		if (message[0] == '\0')
			strcpy(message_to_gm, msg_txt(505)); //  This player sends a void name and a void message.
		else
			sprintf(message_to_gm, msg_txt(506), message); //  This player sends (name:message): '%s'.
		intif_wis_message_to_gm(wisp_server_name, battle_config.hack_info_GM_level, message_to_gm);
		// if we block people
		if (battle_config.ban_spoof_namer < 1) {
			chrif_char_ask_name(-1, sd->status.name, 1, 0, 0, 0, 0, 0, 0); // type: 1 - block
			clif_setwaitclose(fd); // forced to disconnect because of the hack
			// message about the ban
			sprintf(message_to_gm, msg_txt(540)); //  This player has been definitivly blocked.
		// if we ban people
		} else if (battle_config.ban_spoof_namer > 0) {
			chrif_char_ask_name(-1, sd->status.name, 2, 0, 0, 0, 0, battle_config.ban_spoof_namer, 0); // type: 2 - ban (year, month, day, hour, minute, second)
			clif_setwaitclose(fd); // forced to disconnect because of the hack
			// message about the ban
			sprintf(message_to_gm, msg_txt(507), battle_config.ban_spoof_namer); //  This player has been banned for %d minute(s).
		} else {
			// message about the ban
			sprintf(message_to_gm, msg_txt(508)); //  This player hasn't been banned (Ban option is disabled).
		}
		intif_wis_message_to_gm(wisp_server_name, battle_config.hack_info_GM_level, message_to_gm);
		FREE(message_to_gm);
		// but for the hacker, we display on his screen (he sees/looks no difference).
		WPACKETW(0) = 0x17f;
		WPACKETW(2) = message_size + 5; // 4 + 1 for NULL
		strncpy(WPACKETP(4), message, message_size);
		WPACKETB(message_size + 5 - 1) = 0; // NULL
		SENDPACKET(sd->fd, message_size + 5); // only for the hacker // +1 (NULL)
	} else {
		// check GM command
		if (is_atcommand(fd, sd, message, sd->GM_level) != AtCommand_None) {
			FREE(message);
			return;
		}

		// remove space at start for check
		p_message = p_message + 3; // position to message (+ ' : ')
		while(*p_message && isspace(*p_message))
			p_message++;

		// check bad words (after is_atcommand check, because player can have a bad name)
		if (check_bad_word(p_message, message_size, sd)) {
			FREE(message);
			return;
		}

		guild_send_message(sd, message, message_size + 1);
	}

	FREE(message);

	return;
}

/*==========================================
 * ギルド同盟要求
 *------------------------------------------
 */
void clif_parse_GuildRequestAlliance(int fd, struct map_session_data *sd) { // S 0170 <TargetAccID>.l <sourceAccID>.l <myCharactorID>.l
//	nullpo_retv(sd); // checked before to call function

	if(sd->state.gmaster_flag == NULL)
		return; //Only the guild leader should be able to perform such action, but its also possible with packet hacking

	guild_reqalliance(sd, RFIFOL(fd,2));

	return;
}

/*==========================================
 * ギルド同盟要求返信
 *------------------------------------------
 */
void clif_parse_GuildReplyAlliance(int fd, struct map_session_data *sd) { // S 0x0172 <SorceAccID>.l <type>.l (type: 0 deny, 1:accepted)
//	nullpo_retv(sd); // checked before to call function

	guild_reply_reqalliance(sd, RFIFOL(fd,2), RFIFOL(fd,6)); // flag: 0 deny, 1:accepted

	return;
}

/*==========================================
 * ギルド関係解消
 *------------------------------------------
 */
void clif_parse_GuildDelAlliance(int fd, struct map_session_data *sd) { // S 0x0183 <guild_id>.l <type>.l
//	nullpo_retv(sd); // checked before to call function

	if(sd->state.gmaster_flag == NULL)
		return; //Only the guild leader should be able to perform such action, but its also possible with packet hacking

	guild_delalliance(sd, RFIFOL(fd,2), RFIFOL(fd,6));

	return;
}

/*==========================================
 * ギルド敵対
 *------------------------------------------
 */
void clif_parse_GuildOpposition(int fd, struct map_session_data *sd) { // S 0x0180 <charactorID>.l
//	nullpo_retv(sd); // checked before to call function

	if(sd->state.gmaster_flag == NULL)
		return; //Only the guild leader should be able to perform such action, but its also possible with packet hacking

	guild_opposition(sd, RFIFOL(fd,2));

	return;
}

/*==========================================
 * ギルド解散
 *------------------------------------------
 */
void clif_parse_GuildBreak(int fd, struct map_session_data *sd) { // S 0x015D <guild name>.24B <?>.16B
//	nullpo_retv(sd); // checked before to call function

	guild_break(sd, RFIFOP(fd,2));

	return;
}

// pet
void clif_parse_CatchPet(int fd, struct map_session_data *sd) { // S 0x019f <ID>.l
//	nullpo_retv(sd); // checked before to call function

	pet_catch_process2(sd, RFIFOL(fd,2));

	return;
}

void clif_parse_PetMenu(int fd, struct map_session_data *sd) { // S 0x01a1 <param>.1B
//	nullpo_retv(sd); // checked before to call function

	pet_menu(sd, RFIFOB(fd,2));

	return;
}

void clif_parse_ChangePetName(int fd, struct map_session_data *sd) { // S 0x01a5 <pet name>.24B
//	nullpo_retv(sd); // checked before to call function

	pet_change_name(sd, RFIFOP(fd,2));

	return;
}

void clif_parse_SelectEgg(int fd, struct map_session_data *sd) { // S 0x01a7 <index>.w
//	nullpo_retv(sd); // checked before to call function

	pet_select_egg(sd, RFIFOW(fd,2) - 2);

	return;
}

void clif_parse_PetEmotion(int fd, struct map_session_data *sd) { // S 0x01a9 <emotion>.l
//	nullpo_retv(sd); // checked before to call function

	if (sd->pd)
		clif_pet_emotion(sd->pd, RFIFOL(fd,2));

	return;
}

// Kick (right click menu for GM "(name) force to quit")
void clif_parse_GMKick(int fd, struct map_session_data *sd) { // S 0x00cc <ID>.l
	struct block_list *target;

//	nullpo_retv(sd); // checked before to call function

	if ((battle_config.atc_gmonly == 0 || sd->GM_level) &&
	    (sd->GM_level >= get_atcommand_level(AtCommand_Kick))) {
		target = map_id2bl(RFIFOL(fd,2));
		if (target) {
			if (target->type == BL_PC) {
				struct map_session_data *tsd = (struct map_session_data *)target;
				if (sd->GM_level > tsd->GM_level)
					clif_GM_kick(sd, tsd, 1);
				else
					clif_GM_kickack(sd, 0);
			} else if (target->type == BL_MOB) {
				struct mob_data *md = (struct mob_data *)target;
				sd->state.attack_type = 0;
				mob_damage(&sd->bl, md, md->hp, 2);
			} else
				clif_GM_kickack(sd, 0);
		} else
			clif_GM_kickack(sd, 0);
	}

	return;
}

/*==========================================
 * /remove (like /shift) (note: in official: /remove is for account_name, /shift for character name)
 *------------------------------------------
 */
void clif_parse_GMRemove(int fd, struct map_session_data *sd) { // S 0x1ba <player_name>.24b
	char output[70]; // 24 (name) + 1 + 1 + 1 + 7 (@jumpto) + 1 + 24 (player_name) + NULL = 60
	char player_name[25];

//	nullpo_retv(sd); // checked before to call function

	memset(output, 0, sizeof(output));
	memset(player_name, 0, sizeof(player_name));

	strncpy(player_name, RFIFOP(fd,2), 24);
	sprintf(output, "%s : %cjumpto %s", sd->status.name, GM_Symbol(), player_name);
	is_atcommand(fd, sd, output, sd->GM_level); // no nothing if command doesn't work

	return;
}

/*==========================================
 * /shift (note: in official: /remove is for account_name, /shift for character name)
 *------------------------------------------
 */
void clif_parse_GMShift(int fd, struct map_session_data *sd) { // S 0x1bb <player_name>.24b
	char output[70]; // 24 (name) + 1 + 1 + 1 + 7 (@jumpto) + 1 + 24 (player_name) + NULL = 60
	char player_name[25];

//	nullpo_retv(sd); // checked before to call function

	memset(output, 0, sizeof(output));
	memset(player_name, 0, sizeof(player_name));

	strncpy(player_name, RFIFOP(fd,2), 24);
	sprintf(output, "%s : %cjumpto %s", sd->status.name, GM_Symbol(), player_name);
	is_atcommand(fd, sd, output, sd->GM_level); // no nothing if command doesn't work

	return;
}

/*==========================================
 * /recall (note: in official: /recall is for account_name, /summon for character name)
 *------------------------------------------
 */
void clif_parse_GMrecall(int fd, struct map_session_data *sd) { // S 0x1bc <player_name>.24b
	char output[70]; // 24 (name) + 1 + 1 + 1 + 6 (@recall) + 1 + 24 (player_name) + NULL = 59
	char player_name[25];

//	nullpo_retv(sd); // checked before to call function

	memset(output, 0, sizeof(output));
	memset(player_name, 0, sizeof(player_name));

	strncpy(player_name, RFIFOP(fd,2), 24);
	sprintf(output, "%s : %crecall %s", sd->status.name, GM_Symbol(), player_name);
	is_atcommand(fd, sd, output, sd->GM_level); // no nothing if command doesn't work

	return;
}

/*==========================================
 * /summon (like /recall) (note: in official: /recall is for account_name, /summon for character name)
 *------------------------------------------
 */
void clif_parse_GMsummon(int fd, struct map_session_data *sd) { // S 0x1bd <player_name>.24b
	char output[70]; // 24 (name) + 1 + 1 + 1 + 6 (@recall) + 1 + 24 (player_name) + NULL = 59
	char player_name[25];

//	nullpo_retv(sd); // checked before to call function

	memset(output, 0, sizeof(output));
	memset(player_name, 0, sizeof(player_name));

	strncpy(player_name, RFIFOP(fd,2), 24);
	sprintf(output, "%s : %crecall %s", sd->status.name, GM_Symbol(), player_name);
	is_atcommand(fd, sd, output, sd->GM_level); // no nothing if command doesn't work

	return;
}

/*==========================================
 * /hide
 *------------------------------------------
 */
void clif_parse_GMHide(int fd, struct map_session_data *sd) { // S 0x019d <?>.4B
	char output[40]; // 24 (name) + 1 + 1 + 1 + 5 (@hide) + NULL = 33

//	nullpo_retv(sd); // checked before to call function

	memset(output, 0, sizeof(output));
	sprintf(output, "%s : %chide", sd->status.name, GM_Symbol());
	is_atcommand(fd, sd, output, sd->GM_level); // no nothing if command doesn't work

	return;
}

/*==========================================
 * GM Mute Request packet from the client itself
 * type:
 *   00 - reduce prohibition time
 *   01 - increase prohibition time
 *   02 - speak too much
 *        time/amount: 10 speak to much
 *        time/amount: 60 move without move mouse/send move packet (bot)
 *------------------------------------------
 */
void clif_parse_GMReqNoChat(int fd, struct map_session_data *sd) { // S 0149 <ID>.l <type>.B <time>.w
	int tid;
	int type;
	unsigned short limit;
	struct block_list *bl;
	struct map_session_data *dstsd = NULL;

//	nullpo_retv(sd); // checked before to call function

	tid = RFIFOL(fd, 2);
	type = RFIFOB(fd, 6);
	limit = RFIFOW(fd, 7);
	bl = map_id2bl(tid);

	if (!bl || bl->type != BL_PC)
		return;
	nullpo_retv(dstsd =(struct map_session_data *)bl);
	
	//printf("player id:%d, type:%d, limit:%d.\n", tid, type, limit);

	if (type == 2 && limit == 60 && battle_config.check_ban_bot < 100) {
		if (tid == dstsd->bl.id && dstsd->followtimer == -1) { // if not follow someone (@follow GM command)
			char message_to_gm[MAX_MSG_LEN + 100]; // max size of msg_txt + security (char name, char id, etc...) (100)
			sprintf(message_to_gm, "Possible BOT usage with player '%s' (account: %d; GM lvl: %d).", dstsd->status.name, dstsd->status.account_id, sd->GM_level);
			intif_wis_message_to_gm(wisp_server_name, battle_config.check_ban_bot, message_to_gm);
			//printf("Bot usage on connection %d: player '%s' (account: %d; GM lvl: %d).\n", fd, sd->status.name, sd->status.account_id, sd->GM_level);
		}
	}

	if (!battle_config.muting_players) {
		// don't display message if player follows another
		if (!(type == 2 && limit == 60 && tid == dstsd->bl.id && dstsd->followtimer != -1))
			clif_displaymessage(fd, msg_txt(511)); // Muting is disabled.
		return;
	}

	if ((type == 2 && !sd->GM_level && tid == sd->bl.id) || 
		  (sd->GM_level > dstsd->GM_level && sd->GM_level >= get_atcommand_level(AtCommand_Mute))) {
		if (type == 0)
			limit = 0 - limit;
		if((dstsd->status.manner - limit) < (-32768)) //Negative overflow fix [Proximus]
			return;
		WPACKETW(0) = 0x14b;
		WPACKETB(2) = (type == 2) ? 1 : type;
		strncpy(WPACKETP(3), sd->status.name, 24);
		SENDPACKET(dstsd->fd, packet_len_table[0x14b]);
		dstsd->status.manner -= limit;
		if (dstsd->status.manner < 0)
			status_change_start(bl, SC_NOCHAT, 0, 0, 0, 0, 0, 0);
		else {
			dstsd->status.manner = 0;
			status_change_end(bl, SC_NOCHAT, -1);
		}
		if (battle_config.etc_log)
			printf("name:%s type:%d limit:%d manner:%d.\n", dstsd->status.name, type, limit, dstsd->status.manner);
	}

	return;
}

/*==========================================
 * GMによるチャット禁止時間参照（？） (from GM menu click)
 *------------------------------------------
 */
void clif_parse_GMReqNoChatCount(int fd, struct map_session_data *sd) { // S 0x01df <ID>.|
//	nullpo_retv(sd); // checked before to call function

	WPACKETW(0) = 0x1e0;
	WPACKETL(2) = RFIFOL(fd,2);
	memset(WPACKETP(6), 0, 24);
	sprintf(WPACKETP(6), "%d", RFIFOL(fd,2)); // ?????????????
//	memcpy(WPACKETP(6), "TESTNAME", 24);
	SENDPACKET(fd, packet_len_table[0x1e0]);

	if (battle_config.etc_log)
		printf("clif_parse_GMReqNoChatCount: from %s about ID %d.\n", sd->status.name, RFIFOL(fd,2));

	return;
}

void clif_parse_PMIgnore(int fd, struct map_session_data *sd) { // S 0x00cf <nick>.24B <type>.B
	char nick[25]; // S 00cf <nick>.24B <type>.B: 00 (/ex nick) deny speech from nick, 01 (/in nick) allow speech from nick
	int i, pos;

//	nullpo_retv(sd); // checked before to call function

	memset(nick, 0, sizeof(nick));
	strncpy(nick, RFIFOP(fd,2), 24); // speed up
	//printf("Ignore: char '%s' state: %d\n", nick, RFIFOB(fd,26));

	// do nothing only if nick can not exist
	if (strlen(nick) < 4) {
		WPACKETW(0) = 0x0d1; // R 00d1 <type>.B <fail>.B: type: 0: deny, 1: allow, fail: 0: success, 1: fail
		WPACKETB(2) = RFIFOB(fd,26);
		WPACKETB(3) = 1; // fail
		SENDPACKET(fd, packet_len_table[0x0d1]);
		if (RFIFOB(fd,26) == 0) // type
			clif_wis_message(fd, wisp_server_name, msg_txt(512), strlen(msg_txt(512)) + 1); // It's impossible to block this player.
		else
			clif_wis_message(fd, wisp_server_name, msg_txt(513), strlen(msg_txt(513)) + 1); // It's impossible to unblock this player.
		return;
	// name can exist
	} else {
		// deny action (we add nick only if it's not already exist
		if (RFIFOB(fd,26) == 0) { // type
			for(i = 0; i < sd->ignore_num; i++) {
				if (strcmp(sd->ignore[i].name, nick) == 0) {
					WPACKETW(0) = 0x0d1; // R 00d1 <type>.B <fail>.B: type: 0: deny, 1: allow, fail: 0: success, 1: fail
					WPACKETB(2) = RFIFOB(fd,26);
					WPACKETB(3) = 1; // fail
					SENDPACKET(fd, packet_len_table[0x0d1]);
					clif_wis_message(fd, wisp_server_name, msg_txt(514), strlen(msg_txt(514)) + 1); // This player is already blocked.
					if (strcmp(wisp_server_name, nick) == 0) { // to found possible bot users who automatically ignore people.
						char output[MAX_MSG_LEN + 100]; // max size of msg_txt + security (char name, char id, etc...) (100)
						sprintf(output, msg_txt(515), sd->status.name, sd->status.account_id, wisp_server_name); // Character '%s' (account: %d) has tried AGAIN to block wisps from '%s' (wisp name of the server). Bot user? Check please.
						intif_wis_message_to_gm(wisp_server_name, battle_config.hack_info_GM_level, output);
					}
					return;
				}
			}
			// if not already blocked
			// if list is full
			if (sd->ignore_num == MAX_IGNORE_LIST) {
				WPACKETW(0) = 0x0d1; // R 00d1 <type>.B <fail>.B: type: 0: deny, 1: allow, fail: 0: success, 1: fail
				WPACKETB(2) = RFIFOB(fd,26);
				WPACKETB(3) = 1; // fail
				SENDPACKET(fd, packet_len_table[0x0d1]);
				clif_wis_message(fd, wisp_server_name, msg_txt(518), strlen(msg_txt(518)) + 1); // You can not block more people.
				if (strcmp(wisp_server_name, nick) == 0) { // to found possible bot users who automatically ignore people.
					char output[MAX_MSG_LEN + 100]; // max size of msg_txt + security (char name, char id, etc...) (100)
					sprintf(output, msg_txt(516), sd->status.name, sd->status.account_id, wisp_server_name); // Character '%s' (account: %d) has tried to block wisps from '%s' (wisp name of the server). Bot user?
					intif_wis_message_to_gm(wisp_server_name, battle_config.hack_info_GM_level, output);
				}
			// if list is not full
			} else {
				// add player in the list
				if (sd->ignore_num == 0) {
					MALLOC(sd->ignore, struct ignore, 1);
				} else {
					REALLOC(sd->ignore, struct ignore, sd->ignore_num + 1);
				}
				strncpy(sd->ignore[sd->ignore_num].name, nick, 24);
				sd->ignore[sd->ignore_num].name[24] = '\0';
				sd->ignore_num++;
				// send message
				WPACKETW(0) = 0x0d1; // R 00d1 <type>.B <fail>.B: type: 0: deny, 1: allow, fail: 0: success, 1: fail
				WPACKETB(2) = RFIFOB(fd,26);
				WPACKETB(3) = 0; // success
				SENDPACKET(fd, packet_len_table[0x0d1]);
				// check bot about 'wisp_server_name'
				if (strcmp(wisp_server_name, nick) == 0) { // to found possible bot users who automatically ignore people.
					char output[MAX_MSG_LEN + 100]; // max size of msg_txt + security (char name, char id, etc...) (100)
					sprintf(output, msg_txt(516), sd->status.name, sd->status.account_id, wisp_server_name); // Character '%s' (account: %d) has tried to block wisps from '%s' (wisp name of the server). Bot user?
					intif_wis_message_to_gm(wisp_server_name, battle_config.hack_info_GM_level, output);
					// send something to be informed and force bot to ignore twice... If GM receiving block + block again, it's a bot :)
					clif_wis_message(fd, wisp_server_name, msg_txt(517), strlen(msg_txt(517)) + 1); // Add me in your ignore list, doesn't block my wisps.
				}
			}
		// allow action (we remove all same nicks if they exist)
		} else {
			pos = -1;
			for(i = 0; i < sd->ignore_num; i++)
				if (strcmp(sd->ignore[i].name, nick) == 0) {
					if (sd->ignore_num == 1) {
						FREE(sd->ignore);
					} else {
						if (i != sd->ignore_num - 1)
							memcpy(sd->ignore[i].name, sd->ignore[sd->ignore_num - 1].name, sizeof(sd->ignore[i].name));
						REALLOC(sd->ignore, struct ignore, sd->ignore_num - 1);
					}
					sd->ignore_num--;
					i--; // to recheck same id
					if (pos == -1) {
						WPACKETW(0) = 0x0d1; // R 00d1 <type>.B <fail>.B: type: 0: deny, 1: allow, fail: 0: success, 1: fail
						WPACKETB(2) = RFIFOB(fd,26);
						WPACKETB(3) = 0; // success
						SENDPACKET(fd, packet_len_table[0x0d1]);
						pos = i; // don't break, to remove ALL same nick
					}
				}
			if (pos == -1) {
				WPACKETW(0) = 0x0d1; // R 00d1 <type>.B <fail>.B: type: 0: deny, 1: allow, fail: 0: success, 1: fail
				WPACKETB(2) = RFIFOB(fd,26);
				WPACKETB(3) = 1; // fail
				SENDPACKET(fd, packet_len_table[0x0d1]);
				clif_wis_message(fd, wisp_server_name, msg_txt(519), strlen(msg_txt(519)) + 1); // This player is not blocked by you.
			}
		}
	}

//	for(i = 0; i < sd->ignore_num; i++) // for debug only
//		printf("Ignored player: '%s'\n", sd->ignore[i].name);

	return;
}

void clif_parse_PMIgnoreAll(int fd, struct map_session_data *sd) { // S 0x00d0 <type>.B
//	nullpo_retv(sd); // checked before to call function

	//printf("Ignore all: state: %d\n", RFIFOB(fd,2));

	if (RFIFOB(fd,2) == 0) {// S 00d0 <type>len.B: 00 (/exall) deny all speech, 01 (/inall) allow all speech
		if (sd->ignoreAll == 0) {
			sd->ignoreAll = 1;
			WPACKETW(0) = 0x0d2; // R 00d2 <type>.B <fail>.B: type: 0: deny, 1: allow, fail: 0: success, 1: fail
			WPACKETB(2) = 0;
			WPACKETB(3) = 0; // success
			SENDPACKET(fd, packet_len_table[0x0d2]);
		} else {
			WPACKETW(0) = 0x0d2; // R 00d2 <type>.B <fail>.B: type: 0: deny, 1: allow, fail: 0: success, 1: fail
			WPACKETB(2) = 0;
			WPACKETB(3) = 1; // fail
			SENDPACKET(fd, packet_len_table[0x0d2]);
			clif_wis_message(fd, wisp_server_name, msg_txt(520), strlen(msg_txt(520)) + 1); // You already block everyone.
		}
	} else {
		if (sd->ignoreAll == 1) {
			sd->ignoreAll = 0;
			WPACKETW(0) = 0x0d2; // R 00d2 <type>.B <fail>.B: type: 0: deny, 1: allow, fail: 0: success, 1: fail
			WPACKETB(2) = 1;
			WPACKETB(3) = 0; // success
			SENDPACKET(fd, packet_len_table[0x0d2]);
		} else {
			WPACKETW(0) = 0x0d2; // R 00d2 <type>.B <fail>.B: type: 0: deny, 1: allow, fail: 0: success, 1: fail
			WPACKETB(2) = 1;
			WPACKETB(3) = 1; // fail
			SENDPACKET(fd, packet_len_table[0x0d2]);
			clif_wis_message(fd, wisp_server_name, msg_txt(521), strlen(msg_txt(521)) + 1); // You already allow everyone.
		}
	}

	return;
}

/*==========================================
 * Wis拒否リスト
 *------------------------------------------
 */
int pstrcmp(const void *a, const void *b) {
	return strcasecmp((char *)a, (char *)b);
}

void clif_parse_PMIgnoreList(int fd, struct map_session_data *sd) { // S 0x00d3
	int i;

//	nullpo_retv(sd); // checked before to call function

	if (sd->ignore_num > 1)
		qsort(sd->ignore, sd->ignore_num, sizeof(sd->ignore[0].name), pstrcmp);

	WPACKETW(0) = 0xd4; // /ex
	WPACKETW(2) = 4 + (24 * sd->ignore_num);
	for(i = 0; i < sd->ignore_num; i++)
		strncpy(WPACKETP(4 + i * 24), sd->ignore[i].name, 24);
	SENDPACKET(fd, 4 + (24 * sd->ignore_num));

	return;
}

/*
void clif_parse_skillMessage(int fd, struct map_session_data *sd) {	// Added by RoVeRT
	int skillid,skilllv, x, y;
	char *mes;

	skilllv = RFIFOW(fd,2);
	skillid = RFIFOW(fd,4);

	y = RFIFOB(fd,6);
	x = RFIFOB(fd,8);

	mes = RFIFOP(fd,10);

	// skill 220 = graffiti
//	printf("skill: %d %d location: %3d %3d message: %s\n", skillid, skilllv, x, y, (char*)mes);
}*/

/*==========================================
 * スパノビの/doridoriによるSPR2倍
 *------------------------------------------
 */
void clif_parse_sn_doridori(int fd, struct map_session_data *sd) { // S 0x01e7
//	nullpo_retv(sd); // checked before to call function

	if (sd)
		sd->doridori_counter = 1;

	return;
}

/*==========================================
 * スパノビの爆裂波動
 *------------------------------------------
 */
void clif_parse_sn_explosionspirits(int fd, struct map_session_data *sd) { // S 0x01ed
	double nextbaseexp;
	short s_class;

//	nullpo_retv(sd); // checked before to call function

	nextbaseexp = (double)pc_nextbaseexp(sd);
	s_class = pc_calc_base_job2(sd->status.class);
	if (s_class == JOB_SUPER_NOVICE && sd->status.base_exp > 0 && nextbaseexp > 0 && (int)(1000. * (double)sd->status.base_exp / nextbaseexp) % 100 == 0) {
		if (battle_config.etc_log) {
			if (nextbaseexp != 0)
				printf("SuperNovice explosionspirits!! %d %d %d %d\n", sd->bl.id, s_class, sd->status.base_exp, (int)((double)1000 * sd->status.base_exp / nextbaseexp));
			else
				printf("SuperNovice explosionspirits!! %d %d %d 000\n", sd->bl.id, s_class, sd->status.base_exp);
		}
		clif_skill_nodamage(&sd->bl, &sd->bl, MO_EXPLOSIONSPIRITS, 5, 1);
		status_change_start(&sd->bl, SkillStatusChangeTable[MO_EXPLOSIONSPIRITS], 5, 0, 0, 0, skill_get_time(MO_EXPLOSIONSPIRITS, 5), 0);
	}

	return;
}

/*==========================================
 * Friend list additional request
 *------------------------------------------
 */
void clif_parse_FriendAddRequest(int fd, struct map_session_data *sd) { // S 0x0202 <nick name>.24B
	char player_name[25]; // 24 + NULL
	struct map_session_data *f_sd;
	int i;

//	nullpo_retv(sd); // checked before to call function

	memset(player_name, 0, sizeof(player_name));
	strncpy(player_name, RFIFOP(fd,2), 24);
	f_sd = map_nick2sd(player_name);

	// Friend doesn't exist (no player with this name)
	if (f_sd == NULL) {
		clif_displaymessage(fd, msg_txt(522)); // This name (for a friend) doesn't exist.
		return;
	}

	// your self? -> return, normal client doesn't accept to add yoursalf as friend
	if (f_sd == sd)
		return;

	// Friend already exists
	for (i = 0; i < sd->friend_num; i++)
		if (sd->friends[i].char_id == f_sd->status.char_id) {
			clif_displaymessage(fd, msg_txt(523)); // This player is already a friend.
			return;
		}

	// no empty slot?
	if (sd->friend_num == MAX_FRIENDS) {
		clif_friend_add_ack(fd, f_sd->bl.id, f_sd->status.char_id, f_sd->status.name, 2); // flag-> 0: accepted/all ok, 1: refused, 2: your friend list is full, 3: the friend list of partner is full
		return;
	}

	// no empty slot?
	if (f_sd->friend_num == MAX_FRIENDS) {
		clif_friend_add_ack(fd, f_sd->bl.id, f_sd->status.char_id, f_sd->status.name, 3); // flag-> 0: accepted/all ok, 1: refused, 2: your friend list is full, 3: the friend list of partner is full
		return;
	}

	// is player alredy invited by someone?
	if (f_sd->friend_invite > 0) {
		clif_displaymessage(fd, msg_txt(524)); // Please wait. This player must already answer to an invitation.
		return;
	}

	f_sd->friend_invite = sd->status.char_id;

	// ask other player
	clif_friend_add_request(f_sd->fd, sd);

	return;
}

/*==========================================
 * Friend list additional request answer
 *------------------------------------------
 */
void clif_parse_FriendAddReply(int fd, struct map_session_data *sd) { // S 0x0208 <id1>.l <id2>.l <flag>.l - Flag: 0: deny, 1: accept
	//int account_id;
	int char_id;
	struct map_session_data *tsd;

//	nullpo_retv(sd); // checked before to call function

	// player was invited?
	if (sd->friend_invite == 0)
		return;

	//account_id = RFIFOL(fd,2);
	char_id = RFIFOL(fd,6);

	// is right anwer?
	if (sd->friend_invite != char_id)
		return;

	sd->friend_invite = 0;

	tsd = map_charid2sd(char_id);

	// player that has invited is not more present
	if (tsd == NULL)
		return;

	// refused?
	if (RFIFOL(fd, 10) == 0) { // flag
		clif_friend_add_ack(tsd->fd, sd->bl.id, sd->status.char_id, sd->status.name, 1); // flag-> 0: accepted/all ok, 1: refused, 2: your friend list is full, 3: the friend list of partner is full
		return;
	}

	// no empty slot?
	if (sd->friend_num == MAX_FRIENDS) {
		clif_friend_add_ack(fd, tsd->bl.id, tsd->status.char_id, tsd->status.name, 2); // flag-> 0: accepted/all ok, 1: refused, 2: your friend list is full, 3: the friend list of partner is full
		clif_friend_add_ack(tsd->fd, sd->bl.id, sd->status.char_id, sd->status.name, 3); // flag-> 0: accepted/all ok, 1: refused, 2: your friend list is full, 3: the friend list of partner is full
		return;
	}

	// no empty slot?
	if (tsd->friend_num == MAX_FRIENDS) {
		clif_friend_add_ack(fd, tsd->bl.id, tsd->status.char_id, tsd->status.name, 3); // flag-> 0: accepted/all ok, 1: refused, 2: your friend list is full, 3: the friend list of partner is full
		clif_friend_add_ack(tsd->fd, sd->bl.id, sd->status.char_id, sd->status.name, 2); // flag-> 0: accepted/all ok, 1: refused, 2: your friend list is full, 3: the friend list of partner is full
		return;
	}

	// add friend in the list of the invited
	if (sd->friend_num == 0) {
		CALLOC(sd->friends, struct friend, 1);
	} else {
		REALLOC(sd->friends, struct friend, sd->friend_num + 1);
	}
	memset(&sd->friends[sd->friend_num], 0, sizeof(struct friend));
	sd->friends[sd->friend_num].account_id  = tsd->status.account_id;
	sd->friends[sd->friend_num].char_id     = tsd->status.char_id;
	strncpy(sd->friends[sd->friend_num].name, tsd->status.name, 24);
	sd->friend_num++;
	// send acceptation of friend
	clif_friend_add_ack(sd->fd, tsd->bl.id, tsd->status.char_id, tsd->status.name, 0); // flag-> 0: accepted/all ok, 1: refused, 2: your friend list is full, 3: the friend list of partner is full

	// add friend in the list of the player that has invited
	if (tsd->friend_num == 0) {
		CALLOC(tsd->friends, struct friend, 1);
	} else {
		REALLOC(tsd->friends, struct friend, tsd->friend_num + 1);
	}
	memset(&tsd->friends[tsd->friend_num], 0, sizeof(struct friend));
	tsd->friends[tsd->friend_num].account_id  = sd->status.account_id;
	tsd->friends[tsd->friend_num].char_id     = sd->status.char_id;
	strncpy(tsd->friends[tsd->friend_num].name, sd->status.name, 24);
	tsd->friend_num++;
	// send acceptation of friend
	clif_friend_add_ack(tsd->fd, sd->bl.id, sd->status.char_id, sd->status.name, 0); // flag-> 0: accepted/all ok, 1: refused, 2: your friend list is full, 3: the friend list of partner is full

	// Add friend in the 2 lists (on char-server)
	chrif_send_friends(sd->status.char_id, tsd->status.char_id); // 0x2b24 <friend_char_id1>.L <friend_char_id2>.L

	return;
}

/*==========================================
 * Friend list deletion request
 *------------------------------------------
 */
void clif_parse_FriendDeleteRequest(int fd, struct map_session_data *sd) { // S 0x0203 <id1>.l <id2>.l
	//int account_id;
	struct map_session_data *tsd;
	int char_id;
	int i, j, k;

//	nullpo_retv(sd); // checked before to call function

	//account_id = RFIFOL(fd,2);
	char_id = RFIFOL(fd,6);

	// Search friend
	for (i = 0; i < sd->friend_num; i ++)
		if (sd->friends[i].char_id == char_id) {
			// move all chars down
			for(j = i + 1; j < sd->friend_num; j++)
				memcpy(&sd->friends[j - 1], &sd->friends[j], sizeof(struct friend));
			sd->friend_num--;
			// if at least 1 friend
			if (sd->friend_num > 0) {
				REALLOC(sd->friends, struct friend, sd->friend_num); // realloc destroy last friend
			} else {
				FREE(sd->friends);
			}
			// send answer to player
			clif_friend_del_ack(fd, RFIFOL(fd,2), char_id); // account_id, char_id
			// work on friend list
			tsd = map_charid2sd(char_id);
			if (tsd == NULL) {
				// delete friend from list and ask char_server
				chrif_friend_delete(char_id, sd->status.char_id, 1); // owner of friend list, deleted friend (flag: 0: done on map-server, 1: not done for friend)
			} else {
				// if present on server, do immediatly
				for (k = 0; k < tsd->friend_num; k ++)
					if (tsd->friends[k].char_id == sd->status.char_id) {
						// move all chars down
						for(j = k + 1; j < tsd->friend_num; j++)
							memcpy(&tsd->friends[j - 1], &tsd->friends[j], sizeof(struct friend));
						tsd->friend_num--;
						// if at least 1 friend
						if (tsd->friend_num > 0) {
							REALLOC(tsd->friends, struct friend, tsd->friend_num); // realloc destroy last friend
						} else {
							FREE(tsd->friends);
						}
						// send answer to player
						clif_friend_del_ack(tsd->fd, sd->status.account_id, sd->status.char_id);
						break;
					}
				// delete friend from list and ask char_server
				chrif_friend_delete(char_id, sd->status.char_id, 0); // owner of friend list, deleted friend (flag: 0: done on map-server, 1: not done for friend)
			}
			return;
		}

	clif_displaymessage(fd, msg_txt(525)); // Friend name not found in list.

	return;
}

/*==========================================
 * /killall
 *------------------------------------------
 */
void clif_parse_GMkillall(int fd, struct map_session_data *sd) { // S 0x00ce
	char message[40]; // 24 (name) + 1 + 1 + 1 + 8 (@kickall) + NULL = 36

//	nullpo_retv(sd); // checked before to call function

	memset(message, 0, sizeof(message));

	sprintf(message, "%s : %ckickall", sd->status.name, GM_Symbol());
	is_atcommand(fd, sd, message, sd->GM_level);

	return;
}

/*==========================================
 * 0x021d
 *------------------------------------------
 */
void clif_parse_noaction(int fd, struct map_session_data *sd) { // S 0x021D <0000>.L ??
//	nullpo_retv(sd); // checked before to call function

	return;
}

/*==========================================
 * To send top 10 fame list [Proximus]
 * types: RK_BLACKSMITH, RK_ALCHEMISH, RK_TAEKWON, RK_PK
 *------------------------------------------
 */
void clif_parse_Ranking(int fd) { //0x217, 0x218, 0x225, 0x237
	unsigned int i, type;
	char *name;

	switch(RFIFOW(fd, 0)) {
		case 0x217: type = RK_BLACKSMITH;	WPACKETW(0) = 0x219; break;
		case 0x218: type = RK_ALCHEMIST;  WPACKETW(0) = 0x21a; break;
		case 0x225: type = RK_TAEKWON;    WPACKETW(0) = 0x226; break;
		case 0x237: type = RK_PK;         WPACKETW(0) = 0x238; break;
		default: return;
	}

	for (i = 0; i < MAX_RANKER; i++) {
		if (ranking_data[type][i].char_id > 0) {
			if (strcmp(ranking_data[type][i].name, "") == 0 && (name = map_charid2nick(ranking_data[type][i].char_id)) != NULL)
				memcpy(WPACKETP(2 + 24 * i), name, 24);
			else
				memcpy(WPACKETP(2 + 24 * i), ranking_data[type][i].name, 24);
		} else
			strncpy(WPACKETP(2 + 24 * i), "", 24);
		WPACKETL(242 + i * 4) = ranking_data[type][i].point;
	}
	SENDPACKET(fd, packet_len_table[WPACKETW(0)]);

	return;
}

/*==========================================
 * just to avoid disconnection of the player
 *------------------------------------------
 */
// S 0x012f <len>.w <message>.80B {<index>.w <amount>.w <value>.l}.8B* // ???? -> Street stall establishment, street stall name & item and price list
// S 0x0157 <Account_id of the player>.l - ask information of a player in guild windows (answer 0x0158)
// S 0x0175 <??>.l -> ??? size 6
// S 0x0176 ?? -> ??? size 106
// S 0x01c1 ?? -> ??? size 14
void clif_parse_Unknown(int fd, struct map_session_data *sd) {
	int cmd, packet_len, i;

//	nullpo_retv(sd); // checked before to call function

	if (display_unknown_packet) {
		cmd = RFIFOW(fd,0);

		packet_len = packet_size_table[sd->packet_ver][cmd];
		if (packet_len == -1)
			packet_len = RFIFOW(fd, 2);

		printf("clif_parse: Unknown packet structure: packet 0x%x, length %d\n", cmd, packet_len);
		printf("            Packet version %d, Account ID %d\n", sd->packet_ver, sd->bl.id);
		printf("Account ID %d, character ID %d, player name: '%s'.\n", sd->status.account_id, sd->status.char_id, sd->status.name);
		printf("---- 00-01-02-03-04-05-06-07-08-09-0A-0B-0C-0D-0E-0F");
		for(i = 0; i < packet_len; i++) {
			if ((i & 15) == 0)
				printf("\n%04X ",i);
			printf("%02X ", RFIFOB(fd,i));
		}
		printf("\n");
	}

	return;
}

// functions list
static void (*clif_parse_func_table[MAX_PACKET_VERSION][MAX_PACKET_DB])() = {
	{
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,

	// 40
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,

	// 70
	NULL, NULL, clif_parse_WantToConnection, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, clif_parse_LoadEndAck, clif_parse_TickSend, NULL,

	// 80
	NULL, NULL, NULL, NULL, NULL, clif_parse_WalkToXY, NULL, NULL,
	NULL, clif_parse_ActionRequest, NULL, NULL, clif_parse_GlobalMessage, NULL, NULL, NULL,
	// 90
	clif_parse_NpcClicked, NULL, NULL, NULL, clif_parse_GetCharNameRequest, NULL, clif_parse_Wis, NULL,
	NULL, clif_parse_GMmessage, NULL, clif_parse_ChangeDir, NULL, NULL, NULL, clif_parse_TakeItem,
	// a0
	NULL, NULL, clif_parse_DropItem, NULL, NULL, NULL, NULL, clif_parse_UseItem,
	NULL, clif_parse_EquipItem, NULL, clif_parse_UnequipItem, NULL, NULL, NULL, NULL,
	// b0
	NULL, NULL, clif_parse_Restart, NULL, NULL, NULL, NULL, NULL,
	clif_parse_NpcSelectMenu, clif_parse_NpcNextClicked, NULL, clif_parse_StatusUp, NULL, NULL, NULL, clif_parse_Emotion,

	// c0
	NULL, clif_parse_HowManyConnections, NULL, NULL, NULL, clif_parse_NpcBuySellSelected, NULL, NULL,
	clif_parse_NpcBuyListSend, clif_parse_NpcSellListSend, NULL, NULL, clif_parse_GMKick, NULL, clif_parse_GMkillall, clif_parse_PMIgnore,
	// d0
	clif_parse_PMIgnoreAll, NULL, NULL, clif_parse_PMIgnoreList, NULL, clif_parse_CreateChatRoom, NULL, NULL,
	NULL, clif_parse_ChatAddMember, NULL, NULL, NULL, NULL, clif_parse_ChatRoomStatusChange, NULL,
	// e0
	clif_parse_ChangeChatOwner, NULL, clif_parse_KickFromChat, clif_parse_ChatLeave, clif_parse_TradeRequest, NULL, clif_parse_TradeAck, NULL,
	clif_parse_TradeAddItem, NULL, NULL, clif_parse_TradeOk, NULL, clif_parse_TradeCancel, NULL, clif_parse_TradeCommit,
	// f0
	NULL, NULL, NULL, clif_parse_MoveToKafra, NULL, clif_parse_MoveFromKafra, NULL, clif_parse_CloseKafra,
	NULL, clif_parse_CreateParty, NULL, NULL, clif_parse_PartyInvite, NULL, NULL, clif_parse_ReplyPartyInvite,

	// 100
	clif_parse_LeaveParty, NULL, clif_parse_PartyChangeOption, clif_parse_RemovePartyMember, NULL, NULL, NULL, NULL,
	clif_parse_PartyMessage, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	// 110
	NULL, NULL, clif_parse_SkillUp, clif_parse_UseSkillToId, NULL, NULL, clif_parse_UseSkillToPos, NULL,
	clif_parse_StopAttack, NULL, NULL, clif_parse_UseSkillMap, NULL, clif_parse_RequestMemo, NULL, NULL,
	// 120
	NULL, NULL, NULL, NULL, NULL, NULL, clif_parse_PutItemToCart, clif_parse_GetItemFromCart,
	clif_parse_MoveFromKafraToCart, clif_parse_MoveToKafraFromCart, clif_parse_RemoveOption, NULL, NULL, NULL, clif_parse_CloseVending, clif_parse_Unknown,
	// 130
	clif_parse_VendingListReq, NULL, NULL, NULL, clif_parse_PurchaseReq, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, clif_parse_GM_Monster_Item,

	// 140
	clif_parse_MapMove, NULL, NULL, clif_parse_NpcAmountInput, NULL, NULL, clif_parse_NpcCloseClicked, NULL,
	NULL, clif_parse_GMReqNoChat, NULL, NULL, NULL, clif_parse_GuildCheckMaster, NULL, clif_parse_GuildRequestInfo,
	// 150
	NULL, clif_parse_GuildRequestEmblem, NULL, clif_parse_GuildChangeEmblem, NULL, clif_parse_GuildChangeMemberPosition, NULL, clif_parse_Unknown,
	NULL, clif_parse_GuildLeave, NULL, clif_parse_GuildExplusion, NULL, clif_parse_GuildBreak, NULL, NULL,
	// 160
	NULL, clif_parse_GuildChangePositionInfo, NULL, NULL, NULL, clif_parse_CreateGuild, NULL, NULL,
	clif_parse_GuildInvite, NULL, NULL, clif_parse_GuildReplyInvite, NULL, NULL, clif_parse_GuildChangeNotice, NULL,
	// 170
	clif_parse_GuildRequestAlliance, NULL, clif_parse_GuildReplyAlliance, NULL, NULL, clif_parse_Unknown, NULL, NULL,
	clif_parse_ItemIdentify, NULL, clif_parse_UseCard, NULL, clif_parse_InsertCard, NULL, clif_parse_GuildMessage, NULL,

	// 180
	clif_parse_GuildOpposition, NULL, NULL, clif_parse_GuildDelAlliance, NULL, NULL, NULL, NULL,
	NULL, NULL, clif_parse_QuitGame, NULL, NULL, NULL, clif_parse_ProduceMix, NULL,
	// 190
	clif_parse_UseSkillToPos, NULL, NULL, clif_parse_SolveCharName, NULL, NULL, NULL, clif_parse_ResetChar,
	NULL, NULL, NULL, NULL, clif_parse_LGMmessage, clif_parse_GMHide, NULL, clif_parse_CatchPet,
	// 1a0
	NULL, clif_parse_PetMenu, NULL, NULL, NULL, clif_parse_ChangePetName, NULL, clif_parse_SelectEgg,
	NULL, clif_parse_PetEmotion, NULL, NULL, NULL, NULL, clif_parse_SelectArrow, clif_parse_ChangeCart,
	// 1b0
	NULL, NULL, clif_parse_OpenVending, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, clif_parse_GMRemove, clif_parse_GMShift, clif_parse_GMrecall, clif_parse_GMsummon, NULL, NULL,

	// 1c0
	clif_parse_Unknown, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, clif_parse_AutoSpell,
	NULL,
	// 1d0
	NULL, NULL, NULL, NULL, NULL, clif_parse_NpcStringInput, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, clif_parse_GMReqNoChatCount,
	// 1e0
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, clif_parse_sn_doridori,
	clif_parse_CreateParty2, NULL, NULL, NULL, NULL, clif_parse_sn_explosionspirits, NULL, NULL,
	// 1f0
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, clif_parse_ReqAdopt, NULL, NULL, NULL, NULL, NULL, NULL,

	// 200
	NULL, NULL, clif_parse_FriendAddRequest, clif_parse_FriendDeleteRequest, NULL, NULL, NULL, NULL,
	clif_parse_FriendAddReply, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	// 210
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	// 220
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	// 230
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,

	// 240
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	// 250
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
#if 0
	case 0x0157: ???? (size 6, 0x157, Account_id of the player.4L) - give information of a player in guild windows (answer 0x0158)
	case 0x01c0: ???? (size 2, 0x1c0)
#endif
	},
	{NULL},
	{NULL},
	{NULL},
	{NULL},
	{NULL},
	{NULL},
	{NULL},
	{NULL},
	{NULL},
	{NULL}
};

/*==========================================
 * クライアントからのパケット解析
 * socket.cのdo_parsepacketから呼び出される
 *------------------------------------------
 */
static int clif_parse(int fd) {
	int packet_len, cmd;
	unsigned char packet_ver;
	struct map_session_data *sd;

	sd = session[fd]->session_data;

	// 接続が切れてるので後始末
	if (session[fd]->eof ||
	    (battle_config.char_disconnect_mode < 2 && !chrif_isconnect())) { // char鯖に繋がってない間は接続禁止 (!chrif_isconnect())
		if (sd != NULL) {
			if (sd->state.auth) {
				map_quit(sd); // the function doesn't send to inter-server/char-server if it is not connected
				if (!chrif_isconnect()) {
//					if (sd->status.name != NULL) {
						if (sd->GM_level)
							printf("Char-server disconnected: Player [" CL_WHITE "%s" CL_RESET "(gm lvl:" CL_WHITE "%d" CL_RESET ")] has been disconnected.\n", sd->status.name, sd->GM_level); // Player logout display [Valaris]
						else
							printf("Char-server disconnected: Player [" CL_WHITE "%s" CL_RESET "] has been disconnected.\n", sd->status.name); // Player logout display [Valaris]
//					} else
//						printf("Char-server disconnected: Player with account [" CL_WHITE "%d" CL_RESET "] has been disconnected.\n", sd->bl.id); // Player logout display [Yor]
				} else {
//					if (sd->status.name != NULL) {
						if (sd->GM_level)
							printf("Player [" CL_WHITE "%s" CL_RESET "(gm lvl:" CL_WHITE "%d" CL_RESET ")] has logged off your server.\n", sd->status.name, sd->GM_level); // Player logout display [Valaris]
						else
							printf("Player [" CL_WHITE "%s" CL_RESET "] has logged off your server.\n", sd->status.name); // Player logout display [Valaris]
//					} else
//						printf("Player with account [" CL_WHITE "%d" CL_RESET "] has logged off your server.\n", sd->bl.id); // Player logout display [Yor]
				}
			} else { // not authentified! (refused by char-server or disconnect before to be authentified)
				if (!chrif_isconnect())
					printf("Char-server disconnected: Player with account [" CL_WHITE "%d" CL_RESET "] has been disconnected (before to be authentified by char-server).\n", sd->bl.id); // Player logout display [Yor]
				else
					printf("Player with account [" CL_WHITE "%d" CL_RESET "] has logged off your server (before to be authentified by char-server).\n", sd->bl.id); // Player logout display [Yor]
				map_deliddb(&sd->bl); // account_id has been included in the DB before auth answer
			}
			map_quit2(sd); // to free memory of dynamic allocation in session (even if player is auth or not)
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

	if (RFIFOREST(fd) < 2)
		return 0;

	if (sd == NULL) {
		//printf("received packet %d\n", RFIFOW(fd,0));
		// check authentification packet to know packet version
		switch(RFIFOW(fd,0)) {

		// 管理用パケット処理 - administration packets
		case 0x7530: // Athena情報所得
			WPACKETW(0) = 0x7531;
			WPACKETB(2) = ATHENA_MAJOR_VERSION;
			WPACKETB(3) = ATHENA_MINOR_VERSION;
			WPACKETB(4) = ATHENA_REVISION;
			WPACKETB(5) = ATHENA_RELEASE_FLAG;
			WPACKETB(6) = ATHENA_OFFICIAL_FLAG;
			WPACKETB(7) = ATHENA_SERVER_MAP;
			WPACKETW(8) = ATHENA_MOD_VERSION;
			SENDPACKET(fd, 10);
			session[fd]->eof = 1;
			RFIFOSKIP(fd,2);
			return 0;
		case 0x7532: // 接続の切断
			session[fd]->eof = 1;
			RFIFOSKIP(fd,2);
			return 0;
		// Request of the server uptime
		case 0x7533:
			WPACKETW(0) = 0x7534;
			WPACKETL(2) = time(NULL) - start_time;
			if (map_is_alone) { // not in multi-servers
				// calculation like @who (don't show hidden GM) and don't wait update from char-server (realtime calculation)
				int i, count;
				struct map_session_data *pl_sd;
				count = 0;
				for (i = 0; i < fd_max; i++)
					if (session[i] && (pl_sd = session[i]->session_data) && pl_sd->state.auth)
						if (!(pl_sd->GM_level >= battle_config.hide_GM_session || (pl_sd->status.option & OPTION_HIDE)))
							count++;
				WPACKETW(6) = count;
			} else
				WPACKETW(6) = map_getusers();
			SENDPACKET(fd, 8);
			session[fd]->eof = 1;
			RFIFOSKIP(fd, 2);
			return 0;
		case 0x7535: // Request of the server version (freya version)
			WPACKETW(0) = 0x7536;
			WPACKETB(2) = ATHENA_MAJOR_VERSION;
			WPACKETB(3) = ATHENA_MINOR_VERSION;
			WPACKETB(4) = ATHENA_REVISION;
			WPACKETB(5) = ATHENA_RELEASE_FLAG;
			WPACKETB(6) = ATHENA_OFFICIAL_FLAG;
			WPACKETB(7) = ATHENA_SERVER_MAP;
			WPACKETW(8) = ATHENA_MOD_VERSION;
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
			return 0;

		// Authentification packets
		// 0: old, 1: 7july04, 2: 13july04, 3: 26july04, 4: 9aug04/16aug04/17aug04, 5: 6sept04, 6: 21sept04, 7: 18oct04, 8: 25oct04/08nov04, 9: 6dec04, 10: 10jan05, 11: 9may05, 12: 28jun05, 13: 4april06 (by [Yor])
		case 0x72:
			// packet_ver = 2
			if (RFIFOREST(fd) >= 39 && (RFIFOB(fd,38) == 0 || RFIFOB(fd,38) == 1)) { // 00 = Female, 01 = Male
				if ((battle_config.packet_ver_flag & 4) != 0) {
					clif_parse_WantToConnection(fd, sd);
					RFIFOSKIP(fd, 39); // packet_size_table[2][0x72]
					return 0;
				}
			// packet_ver = 1
			} else if (RFIFOREST(fd) >= 22 && (RFIFOB(fd,21) == 0 || RFIFOB(fd,21) == 1)) { // 00 = Female, 01 = Male
				if ((battle_config.packet_ver_flag & 2) != 0) {
					clif_parse_WantToConnection(fd, sd);
					RFIFOSKIP(fd, 22); // packet_size_table[1][0x72]
					return 0;
				}
			// packet_ver = 0
			} else if (RFIFOREST(fd) >= 19 && (RFIFOB(fd,18) == 0 || RFIFOB(fd,18) == 1)) { // 00 = Female, 01 = Male
				if ((battle_config.packet_ver_flag & 1) != 0) {
					clif_parse_WantToConnection(fd, sd);
					RFIFOSKIP(fd, 19); // packet_size_table[0][0x72]
					return 0;
				}
			// else probably incomplete packet
			} else if (RFIFOREST(fd) < 19) // lesser packet
				return 0;
			// else invalid
			else if (RFIFOREST(fd) >= 39) { // bigest packet (and not correct/valid)
				session[fd]->eof = 1;
				return 0;
			}
			break;
		case 0x7E:
			// packet_ver = 4
			if (RFIFOREST(fd) >= 37 && (RFIFOB(fd,36) == 0 || RFIFOB(fd,36) == 1)) { // 00 = Female, 01 = Male
				if ((battle_config.packet_ver_flag & 16) != 0) {
					clif_parse_WantToConnection(fd, sd);
					RFIFOSKIP(fd, 37); // packet_size_table[4][0x7E]
					return 0;
				}
			// packet_ver = 3
			} else if (RFIFOREST(fd) >= 33 && (RFIFOB(fd,32) == 0 || RFIFOB(fd,32) == 1)) { // 00 = Female, 01 = Male
				if ((battle_config.packet_ver_flag & 8) != 0) {
					clif_parse_WantToConnection(fd, sd);
					RFIFOSKIP(fd, 33); // packet_size_table[3][0x7E]
					return 0;
				}
			// else probably incomplete packet
			} else if (RFIFOREST(fd) < 33) // lesser packet
				return 0;
			// else invalid
			else if (RFIFOREST(fd) >= 37) { // bigest packet (and not correct/valid)
				session[fd]->eof = 1;
				return 0;
			}
			break;
		case 0xF5:
			// packet_ver = 5
			if (RFIFOREST(fd) >= 34 && (RFIFOB(fd,33) == 0 || RFIFOB(fd,33) == 1)) { // 00 = Female, 01 = Male
				if ((battle_config.packet_ver_flag & 32) != 0) {
					clif_parse_WantToConnection(fd, sd);
					RFIFOSKIP(fd, 34); // packet_size_table[5][0xF5]
					return 0;
				}
			// packet_ver = 7
			} else if (RFIFOREST(fd) >= 33 && (RFIFOB(fd,32) == 0 || RFIFOB(fd,32) == 1)) { // 00 = Female, 01 = Male
				if ((battle_config.packet_ver_flag & 128) != 0) {
					clif_parse_WantToConnection(fd, sd);
					RFIFOSKIP(fd, 33); // packet_size_table[7][0xF5]
					return 0;
				}
			// packet_ver = 6
			} else if (RFIFOREST(fd) >= 32 && (RFIFOB(fd,31) == 0 || RFIFOB(fd,31) == 1)) { // 00 = Female, 01 = Male
				if ((battle_config.packet_ver_flag & 64) != 0) {
					clif_parse_WantToConnection(fd, sd);
					RFIFOSKIP(fd, 32); // packet_size_table[6][0xF5]
					return 0;
				}
			// packet_ver = 8 or packet_ver = 9
			} else if (RFIFOREST(fd) >= 29 && (RFIFOB(fd,28) == 0 || RFIFOB(fd,28) == 1)) { // 00 = Female, 01 = Male
				if (((battle_config.packet_ver_flag & 256) != 0 && (battle_config.packet_ver_flag & 512) == 0) || // only version 8
				    ((battle_config.packet_ver_flag & 256) == 0 && (battle_config.packet_ver_flag & 512) != 0) || // only version 9
					// if account id and char id of packet_ver 9
				    ((battle_config.packet_ver_flag & 512) != 0 &&
				     RFIFOL(fd,3) >= 2000000 && RFIFOL(fd,3) < 1000000000 && // account id (more than 1,000,000,000 accounts?)
				     RFIFOL(fd,10) >= 150000 && RFIFOL(fd,10) < 5000000) || // char id (more than 5,000,000 characters?) // note: Old versions have id begining at 150000 (now, 500000)
					// if account id and char id of packet_ver 8
				    ((battle_config.packet_ver_flag & 256) != 0 &&
				     RFIFOL(fd,5) >= 2000000 && RFIFOL(fd,5) < 1000000000 && // account id (more than 1,000,000,000 accounts?)
				     RFIFOL(fd,14) >= 150000 && RFIFOL(fd,14) < 5000000)) { // char id (more than 5,000,000 characters?) // note: Old versions have id begining at 150000 (now, 500000)
					clif_parse_WantToConnection(fd, sd);
					RFIFOSKIP(fd, 29); // packet_size_table[8][0xF5] or packet_size_table[9][0xF5]
					return 0;
				}
			// else probably incomplete packet
			} else if (RFIFOREST(fd) < 29) // lesser packet
				return 0;
			// else invalid
			else if (RFIFOREST(fd) >= 34) { // bigest packet (and not correct/valid)
				session[fd]->eof = 1;
				return 0;
			}
			break;
		case 0x9B:
			// packet_ver = 13
			if (RFIFOREST(fd) >= 37 && (RFIFOB(fd,36) == 0 || RFIFOB(fd,36) == 1)) { // 00 = Female, 01 = Male
				if ((battle_config.packet_ver_flag & 8192) != 0) {
					clif_parse_WantToConnection(fd, sd);
					RFIFOSKIP(fd, 37); // packet_size_table[13][0x9B]
					return 0;
				}
			// packet_ver = 10 or packet_ver = 12
			} else if (RFIFOREST(fd) >= 32 && (RFIFOB(fd,31) == 0 || RFIFOB(fd,31) == 1)) { // 00 = Female, 01 = Male
				if (((battle_config.packet_ver_flag & 1024) != 0 && (battle_config.packet_ver_flag & 4096) == 0) || // only version 10
				    ((battle_config.packet_ver_flag & 1024) == 0 && (battle_config.packet_ver_flag & 4096) != 0) || // only version 12
					// if account id and char id of packet_ver 12
				    ((battle_config.packet_ver_flag & 4096) != 0 &&
				      RFIFOL(fd,9) >= 2000000 && RFIFOL(fd,9) < 1000000000 && // account id (more than 1,000,000,000 accounts?)
				      RFIFOL(fd,15) >= 150000 && RFIFOL(fd,15) < 5000000) || // char id (more than 5,000,000 characters?) // note: Old versions have id begining at 150000 (now, 500000)
					// if account id and char id of packet_ver 10
				    ((battle_config.packet_ver_flag & 1024) != 0 &&
				     RFIFOL(fd,3) >= 2000000 && RFIFOL(fd,3) < 1000000000 && // account id (more than 1,000,000,000 accounts?)
				     RFIFOL(fd,12) >= 150000 && RFIFOL(fd,12) < 5000000)) { // char id (more than 5,000,000 characters?) // note: Old versions have id begining at 150000 (now, 500000)
					clif_parse_WantToConnection(fd, sd);
					RFIFOSKIP(fd, 32); // packet_size_table[10][0x9B] or packet_size_table[12][0x9B]
					return 0;
				}
			// packet_ver = 11
			} else if (RFIFOREST(fd) >= 26 && (RFIFOB(fd,25) == 0 || RFIFOB(fd,25) == 1)) { // 00 = Female, 01 = Male
				if ((battle_config.packet_ver_flag & 2048) != 0) {
					clif_parse_WantToConnection(fd, sd);
					RFIFOSKIP(fd, 26); // packet_size_table[11][0x9B]
					return 0;
				}
			// else probably incomplete packet
			} else if (RFIFOREST(fd) < 26) // lesser packet
				return 0;
			// else invalid
			else if (RFIFOREST(fd) >= 37) { // biggest packet (and not correct/valid)
				session[fd]->eof = 1;
				return 0;
			}
			break;

		// unknown packet
		default:
			session[fd]->eof = 1;
			return 0;
		}

		// if we arrive here, the packet_verflag was not authorized.
		WPACKETW(0) = 0x6a;
		WPACKETB(2) = 5; // 05 = Game's EXE is not the latest version
		SENDPACKET(fd, 23);
		session[fd]->eof = 1;
		return 0;

	} else {
		// get packet version before to parse
		packet_ver = sd->packet_ver;

		// ゲーム用以外パケットか、認証を終える前に0072以外が来たら、切断する
		if (packet_ver >= MAX_PACKET_VERSION) { // if packet is not inside these values: session is incorrect?? or auth packet is unknown
			session[fd]->eof = 1;
			//printf("clif_parse: session #%d, packet 0x%x (%d bytes received) unknown version -> disconnected.\n", fd, RFIFOW(fd,0), RFIFOREST(fd));
			return 0;
		}

		while(RFIFOREST(fd) >= 2 && !session[fd]->eof) {

			//printf("clif_parse: connection #%d, packet (ver %d): 0x%x (with being read: %d bytes).\n", fd, packet_ver, RFIFOW(fd,0), RFIFOREST(fd));

			cmd = RFIFOW(fd,0);

			// ゲーム用以外パケットか、認証を終える前に0072以外が来たら、切断する
			if (cmd < 0 || cmd >= MAX_PACKET_DB || (packet_len = packet_size_table[packet_ver][cmd]) == 0) {
				session[fd]->eof = 1;
				printf("clif_parse: session #%d, packet 0x%x (%d bytes received) -> disconnected.\n", fd, cmd, RFIFOREST(fd));
				return 0;
			}

			// パケット長を計算
			if (packet_len == -1) {
				if (RFIFOREST(fd) < 4)
					return 0; // 可変長パケットで長さの所までデータが来てない
				packet_len = RFIFOW(fd,2);
				if (packet_len < 4 || packet_len > 32768) {
					session[fd]->eof = 1;
					return 0;
				}
			}
			if (RFIFOREST(fd) < packet_len)
				return 0; // まだ1パケット分データが揃ってない

			if (!sd->state.auth || sd->state.waitingdisconnect) { // 切断待ちの場合パケットを処理しない

			} else if (clif_parse_func_table[packet_ver][cmd] != NULL) {
				if (clif_parse_func_table[packet_ver][cmd] == clif_parse_WantToConnection) { //Alt+F4
					session[fd]->eof = 1;
					return 0;
				} else {
					// パケット処理
					clif_parse_func_table[packet_ver][cmd](fd, sd);
					// if not tick (automatic packet)
					if (clif_parse_func_table[packet_ver][cmd] != clif_parse_TickSend) {
#ifdef ANTIBOT_SYSTEM						
						// check antibot
						if (sd->state.bot_flag < 6) // 0: no bot action done, 1: far fake player sended, 2: far fake player deleted, 3: hiden fake player sended, 4: hiden fake player deleted, 5: like 1, 6: like 2
							antibot_action(sd); // IMPORTANT: after parse, to know if player is moving
#endif
						// check inactiv player
						if (battle_config.idle_disconnect != 0 && 
							clif_parse_func_table[packet_ver][cmd] != clif_parse_GetCharNameRequest) // not charname solve (mouse can stay in front, and player doesn't ask for)
							sd->lastpackettime = time(NULL); // for disconnection if player is inactive
					}
				}
			} else {
				// 不明なパケット
				if (battle_config.error_log) {
					printf("\nclif_parse: session #%d, packet 0x%x, length %d\n", fd, cmd, packet_len);
#ifdef DUMP_UNKNOWN_PACKET
				  {
					int i;
					FILE *fp;
					char packet_txt[256] = "save/packet.txt";
					time_t now;
					printf("---- 00-01-02-03-04-05-06-07-08-09-0A-0B-0C-0D-0E-0F");
					for(i = 0; i < packet_len; i++) {
						if ((i & 15) == 0)
							printf("\n%04X ",i);
						printf("%02X ", RFIFOB(fd,i));
					}
					if (sd->state.auth) {
						if (sd->status.name != NULL)
							printf("\nAccount ID %d, character ID %d, player name %s.\n",
							       sd->status.account_id, sd->status.char_id, sd->status.name);
						else
							printf("\nAccount ID %d.\n", sd->bl.id);
					} else // not authentified! (refused by char-server or disconnect before to be authentified)
						printf("\nAccount ID %d.\n", sd->bl.id);

					if ((fp = fopen(packet_txt, "a")) == NULL) {
						printf("clif.c: cant write [%s] !!! data is lost !!!\n", packet_txt);
						return 1;
					} else {
						time(&now);
						if (sd->state.auth) {
							if (sd->status.name != NULL)
								fprintf(fp, "%sPlayer with account ID %d (character ID %d, player name %s) sent wrong packet:" RETCODE,
								        asctime(localtime(&now)), sd->status.account_id, sd->status.char_id, sd->status.name);
							else
								fprintf(fp, "%sPlayer with account ID %d sent wrong packet:" RETCODE, asctime(localtime(&now)), sd->bl.id);
						} else // not authentified! (refused by char-server or disconnect before to be authentified)
							fprintf(fp, "%sPlayer with account ID %d sent wrong packet:" RETCODE, asctime(localtime(&now)), sd->bl.id);

						fprintf(fp, "\t---- 00-01-02-03-04-05-06-07-08-09-0A-0B-0C-0D-0E-0F");
						for(i = 0; i < packet_len; i++) {
							if ((i & 15) == 0)
								fprintf(fp, RETCODE "\t%04X ", i);
							fprintf(fp, "%02X ", RFIFOB(fd,i));
						}
						fprintf(fp, RETCODE RETCODE);
						fclose(fp);
					}
				  }
#endif
				}
				session[fd]->eof = 1;
				return 0;
			}

			RFIFOSKIP(fd, packet_len);
		}
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int do_init_clif(void) {

	// functions of packet version 0-1-2 are same, but size are different
	memcpy(&clif_parse_func_table[1], &clif_parse_func_table[0], sizeof(clif_parse_func_table[0]));
	memcpy(&clif_parse_func_table[2], &clif_parse_func_table[1], sizeof(clif_parse_func_table[0]));

	// init packet function calls for packet ver 3
	memcpy(&clif_parse_func_table[3], &clif_parse_func_table[2], sizeof(clif_parse_func_table[0]));
	clif_parse_func_table[3][0x072] = clif_parse_DropItem;
	clif_parse_func_table[3][0x07e] = clif_parse_WantToConnection;
	clif_parse_func_table[3][0x085] = clif_parse_UseSkillToId;
	clif_parse_func_table[3][0x089] = clif_parse_GetCharNameRequest;
	clif_parse_func_table[3][0x08c] = clif_parse_UseSkillToPos;
	clif_parse_func_table[3][0x094] = clif_parse_TakeItem;
	clif_parse_func_table[3][0x09b] = clif_parse_WalkToXY;
	clif_parse_func_table[3][0x09f] = clif_parse_ChangeDir;
	clif_parse_func_table[3][0x0a2] = clif_parse_UseSkillToPos;
	clif_parse_func_table[3][0x0a7] = clif_parse_SolveCharName;
	clif_parse_func_table[3][0x0f3] = clif_parse_GlobalMessage;
	clif_parse_func_table[3][0x0f5] = clif_parse_UseItem;
	clif_parse_func_table[3][0x0f7] = clif_parse_TickSend;
	clif_parse_func_table[3][0x113] = clif_parse_MoveToKafra;
	clif_parse_func_table[3][0x116] = clif_parse_CloseKafra;
	clif_parse_func_table[3][0x190] = clif_parse_MoveFromKafra;
	clif_parse_func_table[3][0x193] = clif_parse_ActionRequest;

	// init packet function calls for packet ver 4 (same function of packet version 3, but size are different)
	memcpy(&clif_parse_func_table[4], &clif_parse_func_table[3], sizeof(clif_parse_func_table[0]));
	// new packet
	clif_parse_func_table[4][0x20f] = clif_parse_Unknown; // just to avoid disconnection of the player
	clif_parse_func_table[4][0x210] = clif_parse_Unknown; // just to avoid disconnection of the player
	clif_parse_func_table[4][0x212] = clif_parse_Unknown; // just to avoid disconnection of the player
	clif_parse_func_table[4][0x213] = clif_parse_Unknown; // just to avoid disconnection of the player
	clif_parse_func_table[4][0x214] = clif_parse_Unknown; // just to avoid disconnection of the player

	// init packet function calls for packet ver 5
	memcpy(&clif_parse_func_table[5], &clif_parse_func_table[4], sizeof(clif_parse_func_table[0]));
	clif_parse_func_table[5][0x072] = clif_parse_UseItem;
	clif_parse_func_table[5][0x07e] = clif_parse_MoveToKafra;
	clif_parse_func_table[5][0x085] = clif_parse_ActionRequest;
	clif_parse_func_table[5][0x089] = clif_parse_WalkToXY;
	clif_parse_func_table[5][0x08c] = clif_parse_UseSkillToPos;
	clif_parse_func_table[5][0x094] = clif_parse_DropItem;
	clif_parse_func_table[5][0x09b] = clif_parse_GetCharNameRequest;
	clif_parse_func_table[5][0x09f] = clif_parse_GlobalMessage;
	clif_parse_func_table[5][0x0a2] = clif_parse_SolveCharName;
	clif_parse_func_table[5][0x0a7] = clif_parse_UseSkillToPos;
	clif_parse_func_table[5][0x0f3] = clif_parse_ChangeDir;
	clif_parse_func_table[5][0x0f5] = clif_parse_WantToConnection;
	clif_parse_func_table[5][0x0f7] = clif_parse_CloseKafra;
	clif_parse_func_table[5][0x113] = clif_parse_TakeItem;
	clif_parse_func_table[5][0x116] = clif_parse_TickSend;
	clif_parse_func_table[5][0x190] = clif_parse_UseSkillToId;
	clif_parse_func_table[5][0x193] = clif_parse_MoveFromKafra;
	// new packet
//	clif_parse_func_table[5][0x20f] = clif_parse_Unknown; // just to avoid disconnection of the player
//	clif_parse_func_table[5][0x210] = clif_parse_Unknown; // just to avoid disconnection of the player
//	clif_parse_func_table[5][0x212] = clif_parse_Unknown; // just to avoid disconnection of the player
//	clif_parse_func_table[5][0x213] = clif_parse_Unknown; // just to avoid disconnection of the player
//	clif_parse_func_table[5][0x214] = clif_parse_Unknown; // just to avoid disconnection of the player

	// init packet function calls for packet ver 6 (same function of packet version 5, but size are different)
	memcpy(&clif_parse_func_table[6], &clif_parse_func_table[5], sizeof(clif_parse_func_table[0]));

	// init packet function calls for packet ver 7 (same function of packet version 6, but size are different)
	memcpy(clif_parse_func_table[7], &clif_parse_func_table[6], sizeof(clif_parse_func_table[0]));

	// init packet function calls for packet ver 8 (same function of packet version 7, but size are different)
	memcpy(&clif_parse_func_table[8], &clif_parse_func_table[7], sizeof(clif_parse_func_table[0]));
	// new packets from 2004-11-08aSakexe.rgz client (always packet version 8)
	clif_parse_func_table[8][0x215] = clif_parse_Unknown; // just to avoid disconnection of the player
	clif_parse_func_table[8][0x216] = clif_parse_Unknown; // just to avoid disconnection of the player
	clif_parse_func_table[8][0x217] = clif_parse_Unknown; // just to avoid disconnection of the player
	clif_parse_func_table[8][0x218] = clif_parse_Unknown; // just to avoid disconnection of the player
	clif_parse_func_table[8][0x219] = clif_parse_Unknown; // just to avoid disconnection of the player
	clif_parse_func_table[8][0x21a] = clif_parse_Unknown; // just to avoid disconnection of the player
	clif_parse_func_table[8][0x21b] = clif_parse_Unknown; // just to avoid disconnection of the player
	clif_parse_func_table[8][0x21c] = clif_parse_Unknown; // just to avoid disconnection of the player

	// init packet function calls for packet ver 9
	memcpy(&clif_parse_func_table[9], &clif_parse_func_table[8], sizeof(clif_parse_func_table[0]));
	clif_parse_func_table[9][0x072] = clif_parse_UseSkillToId;
	clif_parse_func_table[9][0x07e] = clif_parse_UseSkillToPos;
	clif_parse_func_table[9][0x085] = clif_parse_GlobalMessage;
	clif_parse_func_table[9][0x089] = clif_parse_TickSend;
	clif_parse_func_table[9][0x08c] = clif_parse_GetCharNameRequest;
	clif_parse_func_table[9][0x094] = clif_parse_MoveToKafra;
	clif_parse_func_table[9][0x09b] = clif_parse_CloseKafra;
	clif_parse_func_table[9][0x09f] = clif_parse_ActionRequest;
	clif_parse_func_table[9][0x0a2] = clif_parse_TakeItem;
	clif_parse_func_table[9][0x0a7] = clif_parse_WalkToXY;
	clif_parse_func_table[9][0x0f3] = clif_parse_ChangeDir;
	clif_parse_func_table[9][0x0f5] = clif_parse_WantToConnection;
	clif_parse_func_table[9][0x0f7] = clif_parse_SolveCharName;
	clif_parse_func_table[9][0x113] = clif_parse_UseSkillToPos;
	clif_parse_func_table[9][0x116] = clif_parse_DropItem;
	clif_parse_func_table[9][0x190] = clif_parse_UseItem;
	clif_parse_func_table[9][0x193] = clif_parse_MoveFromKafra;
	// new packet
	clif_parse_func_table[9][0x21d] = clif_parse_noaction;
	clif_parse_func_table[9][0x21e] = clif_parse_Unknown; // just to avoid disconnection of the player
	clif_parse_func_table[9][0x221] = clif_parse_Unknown; // just to avoid disconnection of the player
	clif_parse_func_table[9][0x222] = clif_parse_Unknown; // just to avoid disconnection of the player
	clif_parse_func_table[9][0x223] = clif_parse_Unknown; // just to avoid disconnection of the player

	// init packet function calls for packet ver 10
	memcpy(&clif_parse_func_table[10], &clif_parse_func_table[9], sizeof(clif_parse_func_table[0]));
	clif_parse_func_table[10][0x072] = clif_parse_UseSkillToId;
	clif_parse_func_table[10][0x07e] = clif_parse_UseSkillToPos;
	clif_parse_func_table[10][0x085] = clif_parse_ChangeDir;
	clif_parse_func_table[10][0x089] = clif_parse_TickSend;
	clif_parse_func_table[10][0x08c] = clif_parse_GetCharNameRequest;
	clif_parse_func_table[10][0x094] = clif_parse_MoveToKafra;
	clif_parse_func_table[10][0x09b] = clif_parse_WantToConnection;
	clif_parse_func_table[10][0x09f] = clif_parse_UseItem;
	clif_parse_func_table[10][0x0a2] = clif_parse_SolveCharName;
	clif_parse_func_table[10][0x0a7] = clif_parse_WalkToXY;
	clif_parse_func_table[10][0x0f3] = clif_parse_GlobalMessage;
	clif_parse_func_table[10][0x0f5] = clif_parse_TakeItem;
	clif_parse_func_table[10][0x0f7] = clif_parse_MoveFromKafra;
	clif_parse_func_table[10][0x113] = clif_parse_UseSkillToPos;
	clif_parse_func_table[10][0x116] = clif_parse_DropItem;
	clif_parse_func_table[10][0x190] = clif_parse_ActionRequest;
	clif_parse_func_table[10][0x193] = clif_parse_CloseKafra;
	// new packet
	clif_parse_func_table[10][0x21f] = clif_parse_Unknown; // just to avoid disconnection of the player
	clif_parse_func_table[10][0x220] = clif_parse_Unknown; // just to avoid disconnection of the player

	// init packet function calls for packet ver 11 (same function of packet version 10, but size are different)
	memcpy(&clif_parse_func_table[11], &clif_parse_func_table[10], sizeof(clif_parse_func_table[0]));
	// newest packet
	clif_parse_func_table[11][0x224] = clif_parse_Unknown; // just to avoid disconnection of the player
	clif_parse_func_table[11][0x225] = clif_parse_Unknown; // just to avoid disconnection of the player
	clif_parse_func_table[11][0x226] = clif_parse_Unknown; // just to avoid disconnection of the player
	clif_parse_func_table[11][0x227] = clif_parse_Unknown; // just to avoid disconnection of the player
	clif_parse_func_table[11][0x228] = clif_parse_Unknown; // just to avoid disconnection of the player
	clif_parse_func_table[11][0x229] = clif_parse_Unknown; // just to avoid disconnection of the player
	clif_parse_func_table[11][0x22a] = clif_parse_Unknown; // just to avoid disconnection of the player
	clif_parse_func_table[11][0x22b] = clif_parse_Unknown; // just to avoid disconnection of the player
	clif_parse_func_table[11][0x22c] = clif_parse_Unknown; // just to avoid disconnection of the player
	clif_parse_func_table[11][0x22d] = clif_parse_Unknown; // just to avoid disconnection of the player
	clif_parse_func_table[11][0x232] = clif_parse_Unknown; // just to avoid disconnection of the player
	clif_parse_func_table[11][0x233] = clif_parse_Unknown; // just to avoid disconnection of the player
	clif_parse_func_table[11][0x234] = clif_parse_Unknown; // just to avoid disconnection of the player

	// init packet function calls for packet ver 12 (same function of packet version 12, but size are different)
	memcpy(&clif_parse_func_table[12], &clif_parse_func_table[11], sizeof(clif_parse_func_table[0]));
	// newest packet
	clif_parse_func_table[12][0x217] = clif_parse_Ranking;
	clif_parse_func_table[12][0x218] = clif_parse_Ranking;
	clif_parse_func_table[12][0x225] = clif_parse_Ranking;
	clif_parse_func_table[12][0x237] = clif_parse_Ranking;

	// init packet function calls for packet ver 13 (same function of packet version 12, but size are different)
	memcpy(&clif_parse_func_table[13], &clif_parse_func_table[12], sizeof(clif_parse_func_table[0]));	// 0: old, 1: 7july04, 2: 13july04, 3: 26july04, 4: 9aug04/16aug04/17aug04, 5: 6sept04, 6: 21sept04, 7: 18oct04, 8: 25oct04/08nov04, 9: 6dec04, 10: 10jan05, 11: 9may05, 12: 28jun05, 13: 4april06 (by [Yor])

	// size of packet version 0
	memcpy(&packet_size_table[0], &packet_len_table, sizeof(packet_len_table));

	// 0: old, 1: 7july04, 2: 13july04, 3: 26july04, 4: 9aug04/16aug04/17aug04, 5: 6sept04, 6: 21sept04, 7: 18oct04, 8: 25oct04/08nov04, 9: 6dec04, 10: 10jan05, 11: 9may05, 12: 28jun05, 13: 4april06 (by [Yor])
	// size of packet version 1
	memcpy(&packet_size_table[1], &packet_size_table[0], sizeof(packet_len_table));
	packet_size_table[1][0x072] = 22;
	packet_size_table[1][0x085] = 8;
	packet_size_table[1][0x0a7] = 13;
	packet_size_table[1][0x113] = 15;
	packet_size_table[1][0x116] = 15;
	packet_size_table[1][0x190] = 95;

	// size of packet version 2
	memcpy(&packet_size_table[2], &packet_size_table[1], sizeof(packet_len_table));
	packet_size_table[2][0x072] = 39;
	packet_size_table[2][0x085] = 9;
	packet_size_table[2][0x09b] = 13;
	packet_size_table[2][0x09f] = 10;
	packet_size_table[2][0x0a7] = 17;
	packet_size_table[2][0x113] = 19;
	packet_size_table[2][0x116] = 19;
	packet_size_table[2][0x190] = 99;

	// size of packet version 3
	memcpy(&packet_size_table[3], &packet_size_table[2], sizeof(packet_len_table));
	packet_size_table[3][0x072] = 14;
	packet_size_table[3][0x07e] = 33;
	packet_size_table[3][0x085] = 20;
	packet_size_table[3][0x089] = 15;
	packet_size_table[3][0x08c] = 23;
	packet_size_table[3][0x094] = 10;
	packet_size_table[3][0x09b] = 6;
	packet_size_table[3][0x09f] = 13;
	packet_size_table[3][0x0a2] = 103;
	packet_size_table[3][0x0a7] = 12;
	packet_size_table[3][0x0f3] = -1;
	packet_size_table[3][0x0f5] = 17;
	packet_size_table[3][0x0f7] = 10;
	packet_size_table[3][0x113] = 16;
	packet_size_table[3][0x116] = 2;
	packet_size_table[3][0x190] = 26;
	packet_size_table[3][0x193] = 9;

	// size of packet version 4
	memcpy(&packet_size_table[4], &packet_size_table[3], sizeof(packet_len_table));
	packet_size_table[4][0x072] = 17;
	packet_size_table[4][0x07e] = 37;
	packet_size_table[4][0x085] = 26;
	packet_size_table[4][0x089] = 12;
	packet_size_table[4][0x08c] = 40;
	packet_size_table[4][0x094] = 13;
	packet_size_table[4][0x09b] = 15;
	packet_size_table[4][0x09f] = 12;
	packet_size_table[4][0x0a2] = 120;
	packet_size_table[4][0x0a7] = 11;
//	packet_size_table[4][0x0f3] = -1;
	packet_size_table[4][0x0f5] = 24;
	packet_size_table[4][0x0f7] = 13;
	packet_size_table[4][0x113] = 23;
//	packet_size_table[4][0x116] = 2;
	packet_size_table[4][0x190] = 26;
	packet_size_table[4][0x193] = 18;
	// new packet
	packet_size_table[4][0x20f] = 10;
	packet_size_table[4][0x210] = 22;
	packet_size_table[4][0x212] = 26;
	packet_size_table[4][0x213] = 26;
	packet_size_table[4][0x214] = 42;

	// size of packet version 5
	memcpy(&packet_size_table[5], &packet_size_table[4], sizeof(packet_len_table));
	packet_size_table[5][0x072] = 20;
	packet_size_table[5][0x07e] = 19;
	packet_size_table[5][0x085] = 23;
	packet_size_table[5][0x089] = 9;
	packet_size_table[5][0x08c] = 105;
	packet_size_table[5][0x094] = 17;
	packet_size_table[5][0x09b] = 14;
	packet_size_table[5][0x09f] = -1;
	packet_size_table[5][0x0a2] = 14;
	packet_size_table[5][0x0a7] = 25;
	packet_size_table[5][0x0f3] = 10;
	packet_size_table[5][0x0f5] = 34;
	packet_size_table[5][0x0f7] = 2;
	packet_size_table[5][0x113] = 11;
	packet_size_table[5][0x116] = 11;
	packet_size_table[5][0x190] = 22;
	packet_size_table[5][0x193] = 17;

	// size of packet version 6
	memcpy(&packet_size_table[6], &packet_size_table[5], sizeof(packet_len_table));
	packet_size_table[6][0x072] = 18;
	packet_size_table[6][0x07e] = 25;
	packet_size_table[6][0x085] = 9;
	packet_size_table[6][0x089] = 14;
	packet_size_table[6][0x08c] = 109;
	packet_size_table[6][0x094] = 19;
	packet_size_table[6][0x09b] = 10;
//	packet_size_table[6][0x09f] = -1;
	packet_size_table[6][0x0a2] = 10;
	packet_size_table[6][0x0a7] = 29;
	packet_size_table[6][0x0f3] = 18;
	packet_size_table[6][0x0f5] = 32;
//	packet_size_table[6][0x0f7] = 2;
	packet_size_table[6][0x113] = 14;
	packet_size_table[6][0x116] = 14;
	packet_size_table[6][0x190] = 14;
	packet_size_table[6][0x193] = 12;

	// size of packet version 7
	memcpy(&packet_size_table[7], &packet_size_table[6], sizeof(packet_len_table));
	packet_size_table[7][0x072] = 17;
	packet_size_table[7][0x07e] = 16;
//	packet_size_table[7][0x085] = 9;
	packet_size_table[7][0x089] = 6;
	packet_size_table[7][0x08c] = 103;
	packet_size_table[7][0x094] = 14;
	packet_size_table[7][0x09b] = 15;
//	packet_size_table[7][0x09f] = -1;
	packet_size_table[7][0x0a2] = 12;
	packet_size_table[7][0x0a7] = 23;
	packet_size_table[7][0x0f3] = 13;
	packet_size_table[7][0x0f5] = 33;
//	packet_size_table[7][0x0f7] = 2;
	packet_size_table[7][0x113] = 10;
	packet_size_table[7][0x116] = 10;
	packet_size_table[7][0x190] = 20;
	packet_size_table[7][0x193] = 26;

	// size of packet version 8
	memcpy(&packet_size_table[8], &packet_size_table[7], sizeof(packet_len_table));
	packet_size_table[8][0x072] = 13;
	packet_size_table[8][0x07e] = 13;
	packet_size_table[8][0x085] = 15;
//	packet_size_table[8][0x089] = 6;
	packet_size_table[8][0x08c] = 108;
	packet_size_table[8][0x094] = 12;
	packet_size_table[8][0x09b] = 10;
//	packet_size_table[8][0x09f] = -1;
	packet_size_table[8][0x0a2] = 16;
	packet_size_table[8][0x0a7] = 28;
	packet_size_table[8][0x0f3] = 15;
	packet_size_table[8][0x0f5] = 29;
//	packet_size_table[8][0x0f7] = 2;
	packet_size_table[8][0x113] = 9;
	packet_size_table[8][0x116] = 9;
	packet_size_table[8][0x190] = 26;
	packet_size_table[8][0x193] = 22;
	// new packet
	packet_size_table[8][0x215] = 6;
	packet_size_table[8][0x216] = 6;
	packet_size_table[8][0x217] = 2;
	packet_size_table[8][0x218] = 2;
	packet_size_table[8][0x219] = 282;
	packet_size_table[8][0x21a] = 282;
	packet_size_table[8][0x21b] = 10;
	packet_size_table[8][0x21c] = 10;

	// size of packet version 9
	memcpy(&packet_size_table[9], &packet_size_table[8], sizeof(packet_len_table));
	packet_size_table[9][0x072] = 22;
	packet_size_table[9][0x07e] = 30;
	packet_size_table[9][0x085] = -1;
	packet_size_table[9][0x089] = 7;
	packet_size_table[9][0x08c] = 13;
	packet_size_table[9][0x094] = 14;
	packet_size_table[9][0x09b] = 2;
	packet_size_table[9][0x09f] = 18;
	packet_size_table[9][0x0a2] = 7;
	packet_size_table[9][0x0a7] = 7;
	packet_size_table[9][0x0f3] = 8;
	packet_size_table[9][0x0f5] = 29;
	packet_size_table[9][0x0f7] = 14;
	packet_size_table[9][0x113] = 110;
	packet_size_table[9][0x116] = 12;
	packet_size_table[9][0x190] = 15;
	packet_size_table[9][0x193] = 21;
	// new packet
	packet_size_table[9][0x21d] = 6;
	packet_size_table[9][0x21e] = 6;
	packet_size_table[9][0x221] = -1;
	packet_size_table[9][0x222] = 6;
	packet_size_table[9][0x223] = 8;

	// size of packet version 10
	memcpy(&packet_size_table[10], &packet_size_table[9], sizeof(packet_len_table));
	packet_size_table[10][0x072] = 26;
	packet_size_table[10][0x07e] = 114;
	packet_size_table[10][0x085] = 23;
	packet_size_table[10][0x089] = 9;
	packet_size_table[10][0x08c] = 8;
	packet_size_table[10][0x094] = 20;
	packet_size_table[10][0x09b] = 32;
	packet_size_table[10][0x09f] = 17;
	packet_size_table[10][0x0a2] = 11;
	packet_size_table[10][0x0a7] = 13;
	packet_size_table[10][0x0f3] = -1;
	packet_size_table[10][0x0f5] = 9;
	packet_size_table[10][0x0f7] = 21;
	packet_size_table[10][0x113] = 34;
	packet_size_table[10][0x116] = 20;
	packet_size_table[10][0x190] = 20;
	packet_size_table[10][0x193] = 2;
	// new packet
	packet_size_table[10][0x21f] = 66;
	packet_size_table[10][0x220] = 10;

	// size of packet version 11
	memcpy(&packet_size_table[11], &packet_size_table[10], sizeof(packet_len_table));
	packet_size_table[11][0x072] = 25;
	packet_size_table[11][0x07e] = 102;
	packet_size_table[11][0x085] = 11;
	packet_size_table[11][0x089] = 8;
	packet_size_table[11][0x08c] = 11;
	packet_size_table[11][0x094] = 14;
	packet_size_table[11][0x09b] = 26;
	packet_size_table[11][0x09f] = 14;
	packet_size_table[11][0x0a2] = 15;
	packet_size_table[11][0x0a7] = 8;
//	packet_size_table[11][0x0f3] = -1;
	packet_size_table[11][0x0f5] = 8;
	packet_size_table[11][0x0f7] = 22;
	packet_size_table[11][0x113] = 22;
	packet_size_table[11][0x116] = 10;
	packet_size_table[11][0x190] = 19;
//	packet_size_table[11][0x193] = 2;
	// new packet
	packet_size_table[11][0x224] = 10;
	packet_size_table[11][0x225] = 2;
	packet_size_table[11][0x226] = 282;
	packet_size_table[11][0x227] = 18;
	packet_size_table[11][0x228] = 18;
	packet_size_table[11][0x229] = 15;
	packet_size_table[11][0x22a] = 58;
	packet_size_table[11][0x22b] = 57;
	packet_size_table[11][0x22c] = 64;
	packet_size_table[11][0x22d] = 5;
	packet_size_table[11][0x22e] = 69;
	packet_size_table[11][0x22f] = 5;
	packet_size_table[11][0x230] = 12;
	packet_size_table[11][0x231] = 26;
	packet_size_table[11][0x232] = 9;
	packet_size_table[11][0x233] = 11;
	packet_size_table[11][0x234] = -1;
	packet_size_table[11][0x235] = -1;
	packet_size_table[11][0x236] = 10;

	// size of packet version 12
	memcpy(&packet_size_table[12], &packet_size_table[11], sizeof(packet_len_table));
	packet_size_table[12][0x072] = 34;
	packet_size_table[12][0x07e] = 113;
	packet_size_table[12][0x085] = 17;
	packet_size_table[12][0x089] = 13;
	packet_size_table[12][0x08c] = 8;
	packet_size_table[12][0x094] = 31;
	packet_size_table[12][0x09b] = 32;
	packet_size_table[12][0x09f] = 19;
	packet_size_table[12][0x0a2] = 9;
	packet_size_table[12][0x0a7] = 11;
//	packet_size_table[12][0x0f3] = -1;
	packet_size_table[12][0x0f5] = 13;
	packet_size_table[12][0x0f7] = 18;
	packet_size_table[12][0x113] = 33;
	packet_size_table[12][0x116] = 12;
	packet_size_table[12][0x190] = 24;
//	packet_size_table[12][0x193] = 2;
	// new packet
	packet_size_table[12][0x237] = 2;
	packet_size_table[12][0x238] = 282;
	packet_size_table[12][0x239] = 11;
	packet_size_table[12][0x23a] = 4;
	packet_size_table[12][0x23b] = 36;
	packet_size_table[12][0x23c] = -1;
	packet_size_table[12][0x23d] = -1;
	packet_size_table[12][0x23e] = 4;
	packet_size_table[12][0x23f] = 2;
	packet_size_table[12][0x240] = -1;
	packet_size_table[12][0x241] = -1;
	packet_size_table[12][0x242] = -1;
	packet_size_table[12][0x243] = -1;
	packet_size_table[12][0x244] = -1;
	packet_size_table[12][0x245] = 3;
	packet_size_table[12][0x246] = 4;
	packet_size_table[12][0x247] = 8;
	packet_size_table[12][0x248] = -1;
	packet_size_table[12][0x249] = 3;
	packet_size_table[12][0x24a] = 70;
	packet_size_table[12][0x24b] = 4;
	packet_size_table[12][0x24c] = 8;
	packet_size_table[12][0x24d] = 12;
	packet_size_table[12][0x24e] = 4;
	packet_size_table[12][0x24f] = 10;
	packet_size_table[12][0x250] = 3;
	packet_size_table[12][0x251] = 32;
	packet_size_table[12][0x252] = -1;
	packet_size_table[12][0x253] = 3;
	packet_size_table[12][0x254] = 3;
	packet_size_table[12][0x255] = 5;
	packet_size_table[12][0x256] = 5;
	packet_size_table[12][0x257] = 8;
	packet_size_table[12][0x258] = 2;
	packet_size_table[12][0x259] = 3;
	packet_size_table[12][0x25a] = -1;
	packet_size_table[12][0x25b] = -1;
	packet_size_table[12][0x25c] = 4;
	packet_size_table[12][0x25d] = -1;
	packet_size_table[12][0x25e] = 4;

	// size of packet version 13
	memcpy(&packet_size_table[13], &packet_size_table[12], sizeof(packet_len_table));
	packet_size_table[13][0x072] = 26;
	packet_size_table[13][0x07e] = 120;
	packet_size_table[13][0x085] = 12;
//	packet_size_table[13][0x089] = 13;
	packet_size_table[13][0x08c] = 12;
	packet_size_table[13][0x094] = 23;
	packet_size_table[13][0x09b] = 37;
	packet_size_table[13][0x09f] = 24;
	packet_size_table[13][0x0a2] = 11;
	packet_size_table[13][0x0a7] = 15;
//	packet_size_table[13][0x0f3] = -1;
//	packet_size_table[13][0x0f5] = 13;
	packet_size_table[13][0x0f7] = 26;
	packet_size_table[13][0x113] = 40;
	packet_size_table[13][0x116] = 17;
	packet_size_table[13][0x190] = 18;
//	packet_size_table[13][0x193] = 2;

	set_defaultparse(clif_parse);

	read_manner(); // read forbidden words

	make_listen_port(map_port);

	add_timer_func_list(check_manner_file, "check_manner_file");
	add_timer_func_list(clif_waitclose, "clif_waitclose");
	add_timer_func_list(clif_waitauth, "clif_waitauth");
	add_timer_func_list(clif_clearchar_delay_sub, "clif_clearchar_delay_sub");

	add_timer_interval(gettick() + 60000, check_manner_file, 0, 0, 60000); // every 60 sec we check if manner file has been changed

	return 0;
}

