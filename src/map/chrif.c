// Copyright (c) Freya Development Team - Licensed under GNU GPL
// For more information, see License.txt in the main folder

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
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

#include "../common/socket.h"
#include "../common/timer.h"
#include "../common/malloc.h"
#include "map.h"
#include "battle.h"
#include "chrif.h"
#include "clif.h"
#include "intif.h"
#include "npc.h"
#include "pc.h"
#include "nullpo.h"
#include "atcommand.h"
#include "script.h"
#include "status.h"
#include "trade.h"
#include "vending.h"
#include "ranking.h"

#ifdef MEMWATCH
#include "memwatch.h"
#endif

static const int packet_len_table[0x30] = {
	56, 3,-1,30,23,-1, 6,-1,	// 2af8-2aff
	 6,-1,19, 6,-1,49,44, 6,	// 2b00-2b07
	 6,30,-1, 8,86,11,44,34,	// 2b08-2b0f
	-1,-1,10, 6,11,-1,-1, 7,	// 2b10-2b17

	46, 6,-1,-1, 6,70,38, 7,	// 2b18-2b1f
	-1,11,10,10,-1,-1,10,-1,	// 2b20-2b27
};

int char_fd = -1;
int srvinfo;
static char char_ip_str[16];
static int char_ip;
static int char_port = 6121;
static char userid[25], passwd[25]; // 24 + NULL
static unsigned char chrif_state = 0;
static unsigned char map_init_done = 0;
unsigned char map_is_alone; // Define communication usage of map-server if it is alone

/*==========================================
 *
 *------------------------------------------
 */
void chrif_setuserid(char *id) {

	strncpy(userid, id, 24);
}

/*==========================================
 *
 *------------------------------------------
 */
void chrif_setpasswd(char *pwd) {

	memset(passwd, 0, sizeof(passwd));
	strncpy(passwd, pwd, 24);
}

/*==========================================
 *
 *------------------------------------------
 */
void chrif_setip(char *ip) {

	strncpy(char_ip_str, ip, 16);
	char_ip = inet_addr(char_ip_str);
}

/*==========================================
 *
 *------------------------------------------
 */
void chrif_setport(int port) {

	char_port = port;
}

/*==========================================
 *
 *------------------------------------------
 */
inline int chrif_isconnect(void) {

	return chrif_state == 2;
}

/*==========================================
 *
 *------------------------------------------
 */
void chrif_save(struct map_session_data *sd) {

	int manner, clothes_color; // Values are modified by pc_makesavestatus
	struct skill skill_cpy[MAX_SKILL]; // Values are modified by pc_makesavestatus (Cloneskill flag)

	nullpo_retv(sd);

	// Save value before modification by pc_makesavestatus
	clothes_color = sd->status.clothes_color;
	manner = sd->status.manner;
	memcpy(&skill_cpy, &sd->status.skill, sizeof(skill_cpy));

	pc_makesavestatus(sd);

	WPACKETW(0) = 0x2b01;
	WPACKETW(2) = sizeof(struct mmo_charstatus) + 12;
	WPACKETL(4) = sd->bl.id;
	WPACKETL(8) = sd->char_id;
	memcpy(WPACKETP(12), &sd->status, sizeof(struct mmo_charstatus));
	SENDPACKET(char_fd, sizeof(struct mmo_charstatus) + 12);

	// Don't send storage if it is was not modified (saving of brandwidth)
	if (sd->state.modified_storage_flag) { // 0: not modified, 1: modified, 2: modified and sended to char-server for saving
		storage_storage_save(sd); // To synchronise storage with character
		sd->state.modified_storage_flag = 2; // 0: not modified, 1: modified, 2: modified and sended to char-server for saving
	}

	// If it's a guild member, save guild storage if the player has opened it (and not close)
	if (sd->state.storage_flag)
		storage_guild_storagesave(sd);

	// Save global registers if necessary (and answer will remove flag)
	if (sd->global_reg_dirty) // must be saved or not
		chrif_saveglobalreg(sd);

	// Save account registers if necessary (and answer will remove flag)
	if (sd->account_reg_dirty) // must be saved or not
		intif_saveaccountreg(sd);

	// Save account registers 2 if necessary (and answer will remove flag)
	if (sd->account_reg2_dirty) // must be saved or not
		chrif_saveaccountreg2(sd);

	// Restore previous values
	sd->status.clothes_color = clothes_color;
	sd->status.manner = manner;
	memcpy(&sd->status.skill, &skill_cpy, sizeof(skill_cpy));

	sd->last_saving = gettick_cache; // To limit successive savings with auto-save

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
int chrif_connect(int fd) {

	WPACKETW( 0) = 0x2af8;
	strncpy(WPACKETP( 2), userid, 24);
	strncpy(WPACKETP(26), passwd, 24);
	WPACKETL(50) = clif_getip();
	WPACKETW(54) = clif_getport();
	SENDPACKET(fd, 56);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int chrif_sendmap(int fd) {

	int i;

	WPACKETW(0) = 0x2afa;
	for(i = 0; i < map_num; i++) {
		if (map[i].alias != NULL)
			strncpy(WPACKETP(4 + i * 16), map[i].alias, 16);
		else
			strncpy(WPACKETP(4 + i * 16), map[i].name, 16);
	}
	WPACKETW(2) = 4 + i * 16;
	SENDPACKET(fd, WPACKETW(2));

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int chrif_recvmap(int fd) {

	int i, j, ip, port;
	unsigned char *p = (unsigned char *)&ip;

	if (chrif_state < 2)
		return -1;

	ip = RFIFOL(fd,4);
	port = RFIFOW(fd,8);
	j = 0;
	for(i = 10; i < RFIFOW(fd,2); i += 16) {
		map_setipport(RFIFOP(fd,i), ip, port);
		j++;
	}
	if (battle_config.etc_log)
		printf("recv map on %d.%d.%d.%d:%d (%d maps)\n", p[0], p[1], p[2], p[3], port, j);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int chrif_changemapserver(struct map_session_data *sd, char *name, int x, int y, int ip, short port) {

	WPACKETW( 0) = 0x2b05;
	WPACKETL( 2) = sd->bl.id;
	WPACKETL( 6) = sd->login_id1;
	WPACKETL(10) = sd->login_id2;
	WPACKETL(14) = sd->status.char_id;
	strncpy(WPACKETP(18), name, 16);
	WPACKETW(34) = x;
	WPACKETW(36) = y;
	WPACKETL(38) = ip;
	WPACKETW(42) = port;
	WPACKETB(44) = sd->status.sex;
	WPACKETL(45) = session[sd->fd]->client_addr.sin_addr.s_addr;
	SENDPACKET(char_fd, 49);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int chrif_changemapserverack(int fd) {

	struct map_session_data *sd = map_id2sd(RFIFOL(fd,2));

	if (sd == NULL || sd->status.char_id != RFIFOL(fd,14))
		return -1;

	if (RFIFOL(fd,6) == 1) {
		if (battle_config.error_log)
			printf("map server change failed.\n");
		pc_authfail(sd->bl.id);
		return 0;
	}
	clif_changemapserver(sd, RFIFOP(fd,18), RFIFOW(fd,34), RFIFOW(fd,36), RFIFOL(fd,38), RFIFOW(fd,42));

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int chrif_connectack(int fd) {

	if (RFIFOB(fd,2)) {
		printf("Connected to char-server failed %d.\n", RFIFOB(fd,2));
		exit(1);
	}
	printf(CL_CYAN "Successfully connected to char-server" CL_RESET " (connection " CL_WHITE "#%d" CL_RESET ").\n", fd);
	chrif_state = 1;

	chrif_sendmap(fd);

	printf("chrif: '" CL_WHITE "OnCharIfInit" CL_RESET "' event done. (" CL_WHITE "%d" CL_RESET " events).\n", npc_event_doall("OnCharIfInit"));
	printf("chrif: '" CL_WHITE "OnInterIfInit" CL_RESET "' event done. (" CL_WHITE "%d" CL_RESET " events).\n", npc_event_doall("OnInterIfInit"));
	if (!map_init_done) {
		map_init_done = 1;
		printf("chrif: '" CL_WHITE "OnInterIfInitOnce" CL_RESET "' event done. (" CL_WHITE "%d" CL_RESET " events).\n", npc_event_doall("OnInterIfInitOnce"));
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int chrif_sendmapack(int fd) {

	memset(wisp_server_name, 0, sizeof(wisp_server_name));
	strncpy(wisp_server_name, RFIFOP(fd,2), 24);
	server_char_id = RFIFOL(fd, 26);
	server_fake_mob_id = npc_get_new_npc_id(); // Mob ID of an invisible mob to check for bots

	chrif_state = 2;

	// Ask login-server for gm_level of all online GM
	// if char-server has crashed, it send to map-server only 0 as GM level for all players
	chrif_reloadGMdb();

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int chrif_authreq(struct map_session_data *sd) {

	if (!sd->bl.id || !sd->login_id1)
		return -1;

	if (sd->sex > 1)
		return -1;

	WPACKETW( 0) = 0x2afc;
	WPACKETL( 2) = sd->bl.id;
	WPACKETL( 6) = sd->char_id;
	WPACKETL(10) = sd->login_id1;
	WPACKETL(14) = sd->login_id2;
	WPACKETL(18) = session[sd->fd]->client_addr.sin_addr.s_addr;
	WPACKETB(22) = sd->sex;
	SENDPACKET(char_fd, 23);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int chrif_charselectreq(struct map_session_data *sd) {

	if (char_fd < 0) // Refuse character select menu if char-server is disconnected
		return -1;

	WPACKETW( 0) = 0x2b02;
	WPACKETL( 2) = sd->bl.id;
	WPACKETL( 6) = sd->login_id1;
	WPACKETL(10) = sd->login_id2;
	WPACKETL(14) = session[sd->fd]->client_addr.sin_addr.s_addr;
	WPACKETB(18) = sd->sex;
	SENDPACKET(char_fd, 19);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int chrif_searchcharid(int char_id) {

	if (!char_id)
		return -1;

	WPACKETW(0) = 0x2b08;
	WPACKETL(2) = char_id;
	SENDPACKET(char_fd, 6);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int chrif_changegm(int id, const char *pass, int len) {

	if (battle_config.etc_log)
		printf("chrif_changegm: account: %d, password: '%s'.\n", id, pass);

	WPACKETW(0) = 0x2b0a;
	WPACKETW(2) = len + 8;
	WPACKETL(4) = id;
	strncpy(WPACKETP(8), pass, len);
	SENDPACKET(char_fd, len + 8);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int chrif_changeemail(int id, const char *actual_email, const char *new_email) {

	if (battle_config.etc_log)
		printf("chrif_changeemail: account: %d, actual_email: '%s', new_email: '%s'.\n", id, actual_email, new_email);

	WPACKETW(0) = 0x2b0c;
	WPACKETL(2) = id;
	strncpy(WPACKETP( 6), actual_email, 40);
	strncpy(WPACKETP(46), new_email, 40);
	SENDPACKET(char_fd, 86);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int chrif_changepassword(int id, const char *old_password, const char *new_password) {

	if (battle_config.etc_log)
		printf("chrif_changepassword: account: %d, old_password: '%s', new_password: '%s'.\n", id, old_password, new_password);

	WPACKETW(0) = 0x2b1d;
	WPACKETL(2) = id;
	strncpy(WPACKETP( 6), old_password, 32);
	strncpy(WPACKETP(38), new_password, 32);
	SENDPACKET(char_fd, 70);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
void chrif_updatefame(int char_id, unsigned char rank_id, int points) {

	WPACKETW(0) = 0x2b2c;
	WPACKETL(2) = char_id;
	WPACKETL(6) = points;
	WPACKETB(10)= rank_id;
	SENDPACKET(char_fd, 11);

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
int chrif_char_ask_name(int id, char * character_name, short operation_type, int year_gmlvl, int month, int day, int hour, int minute, int second) {

	memset(WPACKETP(0), 0, 44);

	WPACKETW( 0) = 0x2b0e;
	WPACKETL( 2) = id; // account_id of who asks (For answer) -1 if nobody
	strncpy(WPACKETP(6), character_name, 24);
	WPACKETW(30) = operation_type; // type of operation
	switch(operation_type) {
	case 2:
		// Check values...
		if (second > 32767) {
			minute = minute + (second / 60);
			second = second % 60;
		}
		if (minute > 32767) {
			hour = hour + (minute / 60);
			minute = minute % 60;
		}
		if (hour > 32767) {
			day = day + (hour / 24);
			hour = hour % 24;
		}
		// Don't check days, because days are not same in all months
		if (month > 32767) {
			year_gmlvl = year_gmlvl + (month / 12);
			month = month % 12;
		}
		// If no ban, don't send packet
		if (year_gmlvl == 0 && month == 0 && day == 0 && hour == 0 && minute == 0 && second == 0)
			return 0;
		WPACKETW(32) = year_gmlvl;
		WPACKETW(34) = month;
		WPACKETW(36) = day;
		WPACKETW(38) = hour;
		WPACKETW(40) = minute;
		WPACKETW(42) = second;
		break;
	case 6:
		WPACKETB(32) = year_gmlvl;
		break;
	}
	SENDPACKET(char_fd, 44);

	return 0;
}

/*==========================================
 * Answer after a request about a character name to do some operations
 * Used to answer of chrif_char_ask_name.
 * type of operation:
 *   1: block
 *   2: ban
 *   3: unblock
 *   4: unban
 *   5: changesex
 *   6: change GM level
 * type of answer:
 *   0: login-server resquest done
 *   1: player not found
 *   2: gm level too low
 *   3: login-server offline
 *------------------------------------------
 */
int chrif_char_ask_name_answer(int fd) {

	int acc;
	struct map_session_data *sd;
	char output[256];
	char player_name[25]; // 24 + NULL

	acc = RFIFOL(fd,2); // account_id of who has asked (-1 if nobody)
	memset(player_name, 0, sizeof(player_name));
	strncpy(player_name, RFIFOP(fd,6), 24);

	if (acc >= 0) {
		sd = map_id2sd(acc);
		if (sd != NULL) {
			if (RFIFOW(fd, 32) == 1) // Player not found
				sprintf(output, msg_txt(542), player_name); // The player '%s' doesn't exist.
			else {
				switch(RFIFOW(fd, 30)) {
				case 1: // Block
					switch(RFIFOW(fd, 32)) {
					case 0: // Login-server resquest done
						sprintf(output, msg_txt(543), player_name); // Login-server has been asked to block the player '%s'.
						break;
					case 2: // GMlevel too low
						sprintf(output, msg_txt(544), player_name); // Your GM level doesn't authorize you to block the player '%s'.
						break;
					case 3: // Login-server offline
						sprintf(output, msg_txt(545), player_name); // Login-server is offline. Impossible to block the player '%s'.
						break;
					}
					break;
				case 2: // Ban
					switch(RFIFOW(fd, 32)) {
					case 0: // Login-server resquest done
						sprintf(output, msg_txt(546), player_name); // Login-server has been asked to ban the player '%s'.
						break;
					case 2: // GM level too low
						sprintf(output, msg_txt(547), player_name); // Your GM level doesn't authorize you to ban the player '%s'.
						break;
					case 3: // Login-server offline
						sprintf(output, msg_txt(548), player_name); // Login-server is offline. Impossible to ban the player '%s'.
						break;
					}
					break;
				case 3: // Unblock
					switch(RFIFOW(fd, 32)) {
					case 0: // Login-server resquest done
						sprintf(output, msg_txt(549), player_name); // Login-server has been asked to unblock the player '%s'.
						break;
					case 2: // gm level too low
						sprintf(output, msg_txt(550), player_name); // Your GM level doesn't authorize you to unblock the player '%s'.
						break;
					case 3: // login-server offline
						sprintf(output, msg_txt(551), player_name); // Login-server is offline. Impossible to unblock the player '%s'.
						break;
					}
					break;
				case 4: // Unban
					switch(RFIFOW(fd, 32)) {
					case 0: // Login-server resquest done
						sprintf(output, msg_txt(552), player_name); // Login-server has been asked to unban the player '%s'.
						break;
					case 2: // GM level too low
						sprintf(output, msg_txt(553), player_name); // Your GM level doesn't authorize you to unban the player '%s'.
						break;
					case 3: // Login-server offline
						sprintf(output, msg_txt(554), player_name); // Login-server is offline. Impossible to unban the player '%s'.
						break;
					}
					break;
				case 5: // Change Sex
					switch(RFIFOW(fd, 32)) {
					case 0: // Login-server resquest done
						sprintf(output, msg_txt(555), player_name); // Login-server has been asked to change the sex of the player '%s'.
						break;
					case 2: // GM level too low
						sprintf(output, msg_txt(556), player_name); // Your GM level don't authorize you to change the sex of the player '%s'.
						break;
					case 3: // Login-server offline
						sprintf(output, msg_txt(557), player_name); // Login-server is offline. Impossible to change the sex of the player '%s'.
						break;
					}
					break;
				case 6: // Change GM level
					switch(RFIFOW(fd, 32)) {
					case 0: // Login-server resquest done
						sprintf(output, msg_txt(675), player_name); // Login-server has been asked to change the GM level of the player '%s'.
						break;
					case 2: // GM level too low
						sprintf(output, msg_txt(676), player_name); // Your GM level don't authorize you to change the GM level of the player '%s'.
						break;
					case 3: // Login-server offline
						sprintf(output, msg_txt(677), player_name); // Login-server is offline. Impossible to change the GM level of the player '%s'.
						break;
					}
					break;
				}
			}
			if (output[0] != '\0') {
				output[sizeof(output)-1] = '\0';
				clif_displaymessage(sd->fd, output);
			}
		}
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int chrif_changedgm(int fd) {

	int acc, level;
	struct map_session_data *sd = NULL;

	acc = RFIFOL(fd,2);

	sd = map_id2sd(acc);

	if (sd != NULL) { // == NULL is not an error. another map-server can be source of original answer
		level = RFIFOW(fd,6);
		if (battle_config.etc_log)
			printf("chrif_changedgm: account: %d, GM level 0 -> %d.\n", acc, level);
		if (level > 0)
			clif_displaymessage(sd->fd, msg_txt(558)); // GM modification success.
		else
			clif_displaymessage(sd->fd, msg_txt(559)); // Failure of GM modification.
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
// 0x2b0d <account_id>.L <sex>.B <account_id_of_GM>.L (sex = -1 -> failed; account_id_of_GM = -1 -> ladmin)
int chrif_changedsex(int fd) {

	int acc, sex, i;
	int acc_asker;
	struct map_session_data *sd;
	struct map_session_data *sd_asker;
	struct pc_base_job s_class;
	char output[MAX_MSG_LEN + 100]; // max size of msg_txt + security (char name, char id, etc...) (100)

	acc = RFIFOL(fd,2);
	sex = RFIFOB(fd,6);
	acc_asker = RFIFOL(fd,7); // Who wants do operation: -1, ladmin. other: account_id of GM
	if (battle_config.etc_log)
		printf("chrif_changedsex %d.\n", acc);
	sd = map_id2sd(acc);
	if (acc > 0 && sex != -1 && sex != 255) { // sex == -1 -> not found
		if (sd != NULL && sd->status.sex != sex) {
			s_class = pc_calc_base_job(sd->status.class);
			if (sd->status.sex == 0) {
				sd->status.sex = 1;
				sd->sex = 1;
			} else if (sd->status.sex == 1) {
				sd->status.sex = 0;
				sd->sex = 0;
			}
			// To avoid any problem with equipment and invalid sex, equipment is unequipped
			for (i = 0; i < MAX_INVENTORY; i++) {
				if (sd->status.inventory[i].nameid && sd->status.inventory[i].equip)
					pc_unequipitem((struct map_session_data*)sd, i, 3);
			}
			// Reset skills of some jobs (Bard / Dancer)
			if (s_class.job == 19 || s_class.job == 4020 || s_class.job == 4042 ||
			    s_class.job == 20 || s_class.job == 4021 || s_class.job == 4043) {
				// Remove specific skills of classes 19, 4020 and 4042
				for(i = 315; i <= 322; i++) {
					if (sd->status.skill[i].id > 0 && sd->status.skill[i].flag != 1 && sd->status.skill[i].flag != 13) { // flag: 0 (normal), 1 (only card), 2-12 (card and skill (skill level +2)), 13 (cloneskill)
						if (sd->status.skill[i].flag >= 2) {
							sd->status.skill_point += (sd->status.skill[i].flag - 2);
							sd->status.skill[i].flag = 1;
							sd->status.skill[i].lv = 1;
						} else {
							sd->status.skill_point += sd->status.skill[i].lv;
							sd->status.skill[i].id = 0;
							sd->status.skill[i].lv = 0;
						}
					}
				}
				// Remove specific skills of classes 20, 4021 and 4043
				for(i = 323; i <= 330; i++) {
					if (sd->status.skill[i].id > 0 && sd->status.skill[i].flag != 1 && sd->status.skill[i].flag != 13) { // flag: 0 (normal), 1 (only card), 2-12 (card and skill (skill level +2)), 13 (cloneskill)
						if (sd->status.skill[i].flag >= 2) {
							sd->status.skill_point += (sd->status.skill[i].flag - 2);
							sd->status.skill[i].flag = 1;
							sd->status.skill[i].lv = 1;
						} else {
							sd->status.skill_point += sd->status.skill[i].lv;
							sd->status.skill[i].id = 0;
							sd->status.skill[i].lv = 0;
						}
					}
				}
				clif_updatestatus(sd, SP_SKILLPOINT);
				// Change job if necessary
				if (s_class.job == 20 || s_class.job == 4021 || s_class.job == 4043)
					sd->status.class -= 1;
				else if (s_class.job == 19 || s_class.job == 4020 || s_class.job == 4042)
					sd->status.class += 1;
			}
			// Save character
			chrif_save(sd); // Do pc_makesavestatus and save storage + account_reg/account_reg2 too
			sd->login_id1++; // Change identify, because if player come back in char within the 5 seconds, he can change its characters
			                 // Do same modify in login-server for the account, but no in char-server (it ask again login_id1 to login, and don't remember it)
			clif_displaymessage(sd->fd, msg_txt(560)); // Your sex has been changed (need disconnection by the server)...
			clif_setwaitclose(sd->fd); // Forced disconnect for the change
		}
	} else {
		if (sd != NULL)
			printf("chrif_changedsex failed.\n");
	}

	// Display message to GM
	if (acc_asker != -1) {
		sd_asker = map_id2sd(acc_asker);
		if (acc != acc_asker) // No display if GM changes its own sex. Message will display before (always success)
			if (sd_asker != NULL) {
				if (sex == -1 || sex == 255)
					sprintf(output, msg_txt(526), acc); // Change sex failed. Account %d not found.
				else if (sd != NULL)
					sprintf(output, msg_txt(527), sd->status.name); // Sex of '%s' changed.
				else
					sprintf(output, msg_txt(528), acc); // Sex of account %d changed.
				clif_displaymessage(sd_asker->fd, output);
			}
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
void chrif_saveaccountreg2(struct map_session_data *sd) {

	int size;

	size = sizeof(struct global_reg) * sd->account_reg2_num;
	memset(WPACKETP(8), 0, size);

	WPACKETW(0) = 0x2b10; // Send structure of account_reg2 from map-server to char-server
	WPACKETW(2) = 8 + size;
	WPACKETL(4) = sd->bl.id;

	if (sd->account_reg2_num > 0)
		memcpy(WPACKETP(8), sd->account_reg2, size);

	SENDPACKET(char_fd, 8 + size);

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void chrif_accountreg2(int fd) {

	struct map_session_data *sd;

	if ((sd = map_id2sd(RFIFOL(fd,4))) == NULL)
		return;

	FREE(sd->account_reg2);
	sd->account_reg2_num = (RFIFOW(fd,2) - 8) / sizeof(struct global_reg);
	sd->account_reg2_dirty = 0;
	if (sd->account_reg2_num > 0) {
		MALLOC(sd->account_reg2, struct global_reg, sd->account_reg2_num);
		memcpy(sd->account_reg2, RFIFOP(fd,8), RFIFOW(fd,2) - 8);
	}

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
// 0x2b19 <account_id>.L
void chrif_accountreg2Ack(int fd) {

	struct map_session_data *sd;

	if ((sd = map_id2sd(RFIFOL(fd,2))) == NULL)
		return;

	sd->account_reg2_dirty = 0;

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
// 0x2b1a <packet_len>.w <account_id>.L account_reg.structure.*B
void chrif_globalreg(int fd) {

	struct map_session_data *sd;

	if ((sd = map_id2sd(RFIFOL(fd,4))) == NULL) // Player is not yet authenticated
		return;

	sd->global_reg_num = (RFIFOW(fd,2) - 8) / sizeof(struct global_reg);
	sd->global_reg_dirty = 0;
	if (sd->global_reg_num > 0) {
		MALLOC(sd->global_reg, struct global_reg, sd->global_reg_num);
		memcpy(sd->global_reg, RFIFOP(fd,8), RFIFOW(fd,2) - 8);
	}

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void chrif_saveglobalreg(struct map_session_data *sd) {

	int size;

	size = sizeof(struct global_reg) * sd->global_reg_num;
	memset(WPACKETP(8), 0, size);

	WPACKETW(0) = 0x2b1b; // Send structure of global_reg from map-server to char-server
	WPACKETW(2) = 8 + size;
	WPACKETL(4) = sd->status.char_id;

	if (sd->global_reg_num > 0)
		memcpy(WPACKETP(8), sd->global_reg, size);

	SENDPACKET(char_fd, 8 + size);

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
// 0x2b1c <char_id>.L
void chrif_globalregAck(int fd) {

	struct map_session_data *sd;

	if ((sd = map_charid2sd(RFIFOL(fd,2))) == NULL)
		return;

	sd->global_reg_dirty = 0;

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
int chrif_divorce(int char_id, int partner_id) {

	struct map_session_data *sd = NULL;

	if (!char_id || !partner_id)
		return 0;

	nullpo_retr(0, sd = map_nick2sd(map_charid2nick(partner_id)));
	if (sd->status.partner_id == char_id) {
		int i;
		sd->status.partner_id = 0;
		for(i = 0; i < MAX_INVENTORY; i++)
			if (sd->status.inventory[i].nameid == WEDDING_RING_M || sd->status.inventory[i].nameid == WEDDING_RING_F)
				pc_delitem(sd, i, 1, 0);
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int chrif_accountdeletion(int fd) {

	int acc;
	struct map_session_data *sd;

	acc = RFIFOL(fd,2);
	if (battle_config.etc_log)
		printf("chrif_accountdeletion %d.\n", acc);
	if (acc > 0) {
		sd = map_id2sd(acc);
		if (sd) {
			sd->login_id1++; // Change identify, because if player comes back in char within the 5 seconds, he can change its characters
			if (sd->state.auth)
				clif_displaymessage(sd->fd, msg_txt(561)); // Your account has been deleted (disconnection)...
			clif_setwaitclose(sd->fd); // Forced to disconnect for the change
		}
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int chrif_accountban(int fd) {

	int acc;
	struct map_session_data *sd;

	acc = RFIFOL(fd,2);
	if (battle_config.etc_log)
		printf("chrif_accountban %d.\n", acc);
	if (acc > 0) {
		sd = map_id2sd(acc);
		if (sd) {
			sd->login_id1++; // Change identify, because if player come back in char within the 5 seconds, he can change its characters
			if (sd->state.auth) {
				if (RFIFOB(fd,6) == 0) { // 0: change of status, 1: ban
					switch (RFIFOL(fd,7)) { // Status or final date of a banishment
					case 1:   // 0 = Unregistered ID
						clif_displaymessage(sd->fd, msg_txt(562)); // Your account has 'Unregistered'.
						break;
					case 2:   // 1 = Incorrect Password
						clif_displaymessage(sd->fd, msg_txt(563)); // Your account has an 'Incorrect Password'...
						break;
					case 3:   // 2 = This ID is expired
						clif_displaymessage(sd->fd, msg_txt(564)); // Your account has expired.
						break;
					case 4:   // 3 = Rejected from Server
						clif_displaymessage(sd->fd, msg_txt(565)); // Your account has been rejected from server.
						break;
					case 5:   // 4 = You have been blocked by the GM Team
						clif_displaymessage(sd->fd, msg_txt(566)); // Your account has been blocked by the GM Team.
						break;
					case 6:   // 5 = Your Game's EXE file is not the latest version
						clif_displaymessage(sd->fd, msg_txt(567)); // Your Game's EXE file is not the latest version.
						break;
					case 7:   // 6 = Your are Prohibited to log in until %s
						clif_displaymessage(sd->fd, msg_txt(568)); // Your account has been prohibited to log in.
						break;
					case 8:   // 7 = Server is jammed due to over populated
						clif_displaymessage(sd->fd, msg_txt(569)); // Server is jammed due to over populated.
						break;
					case 9:   // 8 = No MSG (actually, all states after 9 except 99 are No MSG, use only this)
						clif_displaymessage(sd->fd, msg_txt(570)); // Your account has not more authorized.
						break;
					case 100: // 99 = This ID has been totally erased
						clif_displaymessage(sd->fd, msg_txt(571)); // Your account has been totally erased.
						break;
					default:
						clif_displaymessage(sd->fd, msg_txt(570)); // Your account has not more authorized.
						break;
					}
				} else if (RFIFOB(fd,6) == 1) { // 0: change of status, 1: ban
					time_t timestamp;
					char tmpstr[2048];
					memset(tmpstr, 0, sizeof(tmpstr));
					timestamp = (time_t)RFIFOL(fd,7); // Status or final date of a banishment
					strcpy(tmpstr, msg_txt(572)); // Your account has been banished until
					strftime(tmpstr + strlen(tmpstr), 24, msg_txt(573), localtime(&timestamp)); // %m-%d-%Y %H:%M:%S
					clif_displaymessage(sd->fd, tmpstr);
				}
			}
			clif_setwaitclose(sd->fd); // Forced to disconnect for the change
		}
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int chrif_recvgmaccounts(int fd) {

	printf("From login-server: receiving information of " CL_WHITE "%d GM accounts" CL_RESET ".\n", pc_read_gm_account(fd));

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int chrif_recv_alone_map_server_flag(int fd) {

	map_is_alone      = RFIFOB(fd,2);
	log_gm_level      = RFIFOB(fd,3);
	log_trade_level   = RFIFOB(fd,4);
	log_script_level  = RFIFOB(fd,5);
	log_vending_level = RFIFOB(fd,6);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int chrif_reloadGMdb(void) {

	struct map_session_data *sd;
	int gm, i;

	WPACKETW(0) = 0x2af7; // 0x2af7 (map->char) -> 0x2709 (char->login)
	gm = 0;
	for(i = 0; i < fd_max; i++) {
		if (session[i] && (sd = session[i]->session_data) && sd->state.auth && sd->GM_level) {
			WPACKETL(6 + 4 * gm) = sd->status.account_id;
			gm++;
		}
	}
	if (gm > 0) { // Don't ask if there is no online GM
		WPACKETW(4) = gm;
		WPACKETW(2) = 6 + (4 * gm); // size
		SENDPACKET(char_fd, WPACKETW(2));
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
// 0x272b/0x2b18 <account_id>.L <new_e-mail>.40B
int chrif_recv_email_change_answer(int fd) {

	int acc;
	struct map_session_data *sd;
	char *tmpstr;
	char new_email[41]; // 40 + NULL

	acc = RFIFOL(fd,2);
	if (battle_config.etc_log)
		printf("chrif_recv_email_change_answer: account: %d.\n", acc);
	if (acc > 0) {
		if ((sd = map_id2sd(acc)) && sd->state.auth) {
			memset(new_email, 0, sizeof(new_email));
			strncpy(new_email, RFIFOP(fd,6), 40);
			if (new_email[0] == '\0') {
				CALLOC(tmpstr, char, strlen(msg_txt(619)) + 1); // Message length + NULL // Your e-mail has NOT been changed (impossible to change it).
				sprintf(tmpstr, msg_txt(619)); // Your e-mail has NOT been changed (impossible to change it).
				clif_wis_message(sd->fd, wisp_server_name, tmpstr, strlen(tmpstr) + 1);
				FREE(tmpstr);
			} else {
				CALLOC(tmpstr, char, strlen(msg_txt(620)) + 40 + 1); // Message length + email length + NULL // Your e-mail has been changed to '%s'.
				sprintf(tmpstr, msg_txt(620), new_email); // Your e-mail has been changed to '%s'.
				clif_wis_message(sd->fd, wisp_server_name, tmpstr, strlen(tmpstr) + 1);
				FREE(tmpstr);
			}
		}
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
// 0x272e/0x2b1e <account_id>.L <new_password>.32B
int chrif_recv_password_change_answer(int fd) {

	int acc;
	struct map_session_data *sd;
	char *tmpstr;
	char new_password[33]; // 32 + NULL

	acc = RFIFOL(fd,2);
	if (battle_config.etc_log)
		printf("chrif_recv_password_change_answer: account: %d.\n", acc);
	if (acc > 0) {
		if ((sd = map_id2sd(acc)) && sd->state.auth) {
			memset(new_password, 0, sizeof(new_password));
			strncpy(new_password, RFIFOP(fd,6), 32);
			if (new_password[0] == '\0') {
				CALLOC(tmpstr, char, strlen(msg_txt(673)) + 1); // Message length + NULL // Your password has NOT been changed (impossible to change it).
				sprintf(tmpstr, msg_txt(673)); // Your password has NOT been changed (impossible to change it).
				clif_wis_message(sd->fd, wisp_server_name, tmpstr, strlen(tmpstr) + 1);
				FREE(tmpstr);
			} else {
				CALLOC(tmpstr, char, strlen(msg_txt(674)) + 32 + 1); // Message length + password length + NULL // Your password has been changed to '%s'.
				sprintf(tmpstr, msg_txt(674), new_password); // Your password has been changed to '%s'.
				clif_wis_message(sd->fd, wisp_server_name, tmpstr, strlen(tmpstr) + 1);
				FREE(tmpstr);
			}
		}
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int chrif_ragsrvinfo(int base_rate, int job_rate, int drop_rate) {

	char line[256], buf[256];
	FILE *fp;
	int i;

	memset(buf, 0, sizeof(buf));
	if ((fp = fopen(motd_txt, "r")) != NULL) {
		while (fgets(line, sizeof(line), fp) != NULL && strlen(buf) < 255) {
			for(i = 0; line[i]; i++) {
				if (line[i] == '\r' || line[i] == '\n') {
					line[i] = '|';
					break;
				}
			}
			strncpy(buf + strlen(buf), line, 255 - strlen(buf));
		}
		fclose(fp);
	} else {
		strcpy(buf, msg_txt(611)); // Unable to read motd.txt file.
	}

	WPACKETW(0) = 0x2b16;
	WPACKETW(2) = strlen(buf) + 11; // 10 + 1 (NULL)
	WPACKETW(4) = base_rate;
	WPACKETW(6) = job_rate;
	WPACKETW(8) = drop_rate;
	strncpy(WPACKETP(10), buf, strlen(buf));
	WPACKETB(WPACKETW(2) - 1) = 0; // WPACKETB(strlen(buf) + 10) = 0; // Set NULL
	SENDPACKET(char_fd, WPACKETW(2));

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
// 0x2b24 <friend_char_id1>.L <friend_char_id2>.L
void chrif_send_friends(int friend_char_id1, int friend_char_id2) {

	WPACKETW(0) = 0x2b24; // 0x2b24 <friend_char_id1>.L <friend_char_id2>.L
	WPACKETL(2) = friend_char_id1;
	WPACKETL(6) = friend_char_id2;
	SENDPACKET(char_fd, 10);

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
// 0x2b25 <size>.W <char_id>.L <friend_num>.W {<friend_account_id>.L <friend_char_id>.L <friend_name>.24B}.x
int chrif_recv_friends_list(int fd) {

	struct map_session_data *sd;
	int friend_num, i;
	unsigned short len;

	sd = map_charid2sd(RFIFOL(fd,4));
	if (sd == NULL)
		return 0;

	friend_num = RFIFOW(fd,8);

	sd->friend_num = friend_num;

	if (friend_num > 0) {
		CALLOC(sd->friends, struct friend, friend_num);
		len = 10;
		for (i = 0; i < friend_num; i++) {
			sd->friends[i].account_id =  RFIFOL(fd, len);
			sd->friends[i].char_id    =  RFIFOL(fd, len + 4);
			strncpy(sd->friends[i].name, RFIFOP(fd, len + 8), 24);
			len += 32;
		}
		// Send Friends info after pc_authok_final_step
		clif_friend_send_info(sd); // Send friends list to player.
		clif_friend_send_online(sd, 1); // Send online/offline msg of friend to player.
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
void chrif_friend_delete(int friend_list_char_id, int deleted_char_id, unsigned char flag) {

	WPACKETW( 0) = 0x2b23; // 0x2b23 <owner_char_id_list>.L <deleted_char_id>.L <flag>.B (flag: 0: do nothing, 1: search friend to remove player from its list)
	WPACKETL( 2) = friend_list_char_id;
	WPACKETL( 6) = deleted_char_id;
	WPACKETB(10) = flag;
	SENDPACKET(char_fd, 11);

	return;
}

#ifdef USE_SQL // TXT version of this is still in dev
/*==========================================
 *
 *------------------------------------------
 */
int chrif_load_scdata(int fd) {

	struct map_session_data *sd;
	struct status_change_data data;
	int char_id, i, count;

	char_id = RFIFOL(fd,4); // Player char ID

	sd = map_charid2sd(char_id);
	if (!sd || sd->status.char_id != char_id) {
		if (battle_config.error_log)
			printf("chrif_load_scdata: Unable to load sc_data for charID: %d.\n", char_id);
		return -1;
	}

	count = RFIFOW(fd,8); // sc_count

	for (i = 0; i < count; i++) {
		memcpy(&data, RFIFOP(fd,14 + i*sizeof(struct status_change_data)), sizeof(struct status_change_data));
		if (data.tick < 1) {
			if(battle_config.error_log)
				printf("chrif_load_scdata: Received invalid duration (%d ms) for status change %d (character %s)\n", data.tick, data.type, sd->status.name);
			continue;
		}
		status_change_start(&sd->bl, data.type, data.val1, data.val2, data.val3, data.val4, data.tick, 7);
		// Flag 3 is 1&2, 1: Force status start, 2: Do not modify the tick value sent.
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int chrif_save_scdata(struct map_session_data *sd) {

	int i, count = 0;
	struct status_change_data data;
	struct TimerData *timer;

	if(!chrif_isconnect())
		return -1;

	WPACKETW( 0) = 0x2b2b;
	WPACKETL( 4) = sd->status.char_id;
	for (i = 0; i < SC_MAX; i++) {
		if (sd->sc_data[i].timer == -1)
			continue;
		timer = get_timer(sd->sc_data[i].timer);
		if (timer == NULL || timer->func != status_change_timer || DIFF_TICK(timer->tick, gettick_cache) < 0)
			continue;
		data.tick = DIFF_TICK(timer->tick, gettick_cache); // Duration that is left before ending.
		data.type = i;
		data.val1 = sd->sc_data[i].val1;
		data.val2 = sd->sc_data[i].val2;
		data.val3 = sd->sc_data[i].val3;
		data.val4 = sd->sc_data[i].val4;

		memcpy(WPACKETP(14 + count * sizeof(struct status_change_data)),
		       &data, sizeof(struct status_change_data));
		count++;
	}
	if (count == 0)
		return 0; // Nothing to save.
	WPACKETW(8) = count;
	WPACKETW(2) = 14 +count*sizeof(struct status_change_data); // Total packet size
	SENDPACKET(char_fd, WPACKETW(2));
	return 0;
}
#endif

/*==========================================
 *
 *------------------------------------------
 */
// 0x2b22 <owner_char_id_list>.L <deleted_char_id>.L
int chrif_parse_friend_delete(int fd) {

	struct map_session_data* sd;
	int char_id, friend_char_id;
	int i, j;

	char_id = RFIFOL(fd,2); // List owner to update

	sd = map_charid2sd(char_id);
	if (sd == NULL)
		return 0;

	friend_char_id = RFIFOL(fd,6); // To delete

	// Search friend
	for (i = 0; i < sd->friend_num; i ++)
		if (sd->friends[i].char_id == friend_char_id) {
			int account_id;
			// Get account_id of the deleted friend to send answer
			account_id = sd->friends[i].account_id;
			// Move all chars down
			for(j = i + 1; j < sd->friend_num; j++)
				memcpy(&sd->friends[j - 1], &sd->friends[j], sizeof(struct friend));
			sd->friend_num--;
			// If at least 1 friend
			if (sd->friend_num > 0) {
				REALLOC(sd->friends, struct friend, sd->friend_num); // Realloc destroy last friend
			} else {
				FREE(sd->friends);
			}
			// Send answer to player
			clif_friend_del_ack(sd->fd, account_id, char_id); // account_id, char_id
			// Don't save friend list, already save in char-server
			break;
		}

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
// 0x2b20
void chrif_recv_top10rank(int fd) {

	memcpy(ranking_data, RFIFOP(fd, 4), sizeof(ranking_data));

	return;
}

int check_connect_char_server(int tid, unsigned int tick, int id, int data);

/*==========================================
 *
 *------------------------------------------
 */
int chrif_parse(int fd) {

	int packet_len, cmd, i;

	// Only char-server can have an access to here.
	// So, if it isn't the char-server, we disconnect the session (fd != char_fd).
	if (fd != char_fd || session[fd]->eof) {
		if (fd == char_fd) {
			printf("Map-server can't connect to char-server (connection #%d).\n", fd);
			char_fd = -1;
			chrif_state = 0;
			map_eraseallipport();
		}
#ifdef __WIN32
		shutdown(fd, SD_BOTH);
		closesocket(fd);
#else
		close(fd);
#endif
		delete_session(fd);
		// If char-server has been disconnected
		if (char_fd == -1) {
			// If we disconnect all people
			if (battle_config.char_disconnect_mode == 0) {
				flush_fifos();
				for(i = 0; i < fd_max; i++)
					if (session[i] && session[i]->session_data)
						session[fd]->eof = 1;
			}
		}
		return 0;
	}

	while (RFIFOREST(fd) >= 2 && !session[fd]->eof) {

		cmd = RFIFOW(fd,0);
		if (cmd < 0x2af8 || cmd >= 0x2af8 + (sizeof(packet_len_table) / sizeof(packet_len_table[0])) ||
		    packet_len_table[cmd-0x2af8] == 0) {

			int r = intif_parse(fd);

			if (r == 1) continue;
			if (r == 2) return 0;

			printf("chrif_parse : parse error on char (%d %d)\n", fd, cmd);
			session[fd]->eof = 1;
			return 0;
		}
		packet_len = packet_len_table[cmd-0x2af8];
		if (packet_len == -1) {
			if (RFIFOREST(fd) < 4)
				return 0;
			packet_len = RFIFOW(fd,2);
		}
		if (RFIFOREST(fd) < packet_len)
			return 0;

		switch(cmd) {
		case 0x2af9: chrif_connectack(fd); break;
		case 0x2afb: chrif_sendmapack(fd); break;
		case 0x2afd: pc_authok(RFIFOL(fd,4), RFIFOL(fd,8), (struct mmo_charstatus*)RFIFOP(fd,12)); break;
		case 0x2afe: pc_authfail(RFIFOL(fd,2)); break;
		case 0x2b00: map_setusers(RFIFOL(fd,2)); break;
		case 0x2b03: clif_charselectok(RFIFOL(fd,2)); break;
		case 0x2b04: chrif_recvmap(fd); break;
		case 0x2b06: chrif_changemapserverack(fd); break;
		case 0x2b07: pc_doubble_connection(RFIFOL(fd,2)); break;
		case 0x2b09: map_addchariddb(RFIFOL(fd,2), RFIFOP(fd,6)); break;
		case 0x2b0b: chrif_changedgm(fd); break;
		case 0x2b0d: chrif_changedsex(fd); break; // 0x2b0d <account_id>.L <sex>.B <account_id_of_GM>.L (sex = -1 -> failed; account_id_of_GM = -1 -> ladmin)
		case 0x2b0f: chrif_char_ask_name_answer(fd); break;
		case 0x2b11: chrif_accountreg2(fd); break; // Send structure of account_reg2 from char-server to map-server
		case 0x2b12: chrif_divorce(RFIFOL(fd,2), RFIFOL(fd,6)); break;
		case 0x2b13: chrif_accountdeletion(fd); break;
		case 0x2b14: chrif_accountban(fd); break;
		case 0x2b15: chrif_recvgmaccounts(fd); break;
		case 0x2b17: chrif_recv_alone_map_server_flag(fd); break;
		case 0x2b18: chrif_recv_email_change_answer(fd); break; // 0x272b/0x2b18 <account_id>.L <new_e-mail>.40B
		case 0x2b19: chrif_accountreg2Ack(fd); break; // 0x2b19 <account_id>.L
		case 0x2b1a: chrif_globalreg(fd); break; // 0x2b1a <packet_len>.w <account_id>.L account_reg.structure.*B
		case 0x2b1c: chrif_globalregAck(fd); break; // 0x2b1c <char_id>.L
		case 0x2b1e: chrif_recv_password_change_answer(fd); break; // 0x272e/0x2b1e <account_id>.L <new_password>.32B
		case 0x2b1f: pc_set_gm_level(RFIFOL(fd,2), RFIFOB(fd,6)); break; // 0x2b1f <account_id>.L <GM_Level>.B

		case 0x2b20: chrif_recv_top10rank(fd); break;

		case 0x2b21: pc_set_gm_level_by_gm(RFIFOL(fd,2), (signed char)RFIFOB(fd,6), RFIFOL(fd,7)); break; // 0x272e/0x2b1e <account_id>.L <GM_level>.B <accound_id_of_GM>.L (GM_level = -1 -> player not found, -2: gm level doesn't authorise you, -3: already right value; account_id_of_GM = -1 -> script)

		case 0x2b22: chrif_parse_friend_delete(fd); break; // 0x2b22 <owner_char_id_list>.L <deleted_char_id>.L
		case 0x2b25: chrif_recv_friends_list(fd); break; // 0x2b25 <size>.W <char_id>.L <friend_num>.W {<friend_account_id>.L <friend_char_id>.L <friend_name>.24B}.x
		
		case 0x2b26: pc_authok_final_step(RFIFOL(fd,2), (time_t)RFIFOL(fd,6)); break; // 0x2b26 <account_id>.L <connect_until_time>.L
#ifdef USE_SQL
		case 0x2b27: chrif_load_scdata(fd); break;
#endif
		default:
			if (battle_config.error_log)
				printf("chrif_parse : unknown packet %d %d\n", fd, RFIFOW(fd,0));
			session[fd]->eof = 1;
			return 0;
		}
		RFIFOSKIP(fd, packet_len);
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int send_users_tochar(int tid, unsigned int tick, int id, int data) {

	int users, hidden_users, i;
	struct map_session_data *sd;

	if (!chrif_isconnect())
		return 0; // Check to see if char-server is connected and initialisation is finished

	// Set online not hidden players
	users = 0;
	for(i = 0; i < fd_max; i++) {
		if (session[i] && (sd = session[i]->session_data) && sd->state.auth &&
		    !(sd->GM_level >= battle_config.hide_GM_session || (sd->status.option & OPTION_HIDE))) {
			WPACKETL(9 + 4 * users) = sd->status.char_id;
			users++;
		}
	}
	// Set online not hidden players
	hidden_users = 0;
	for(i = 0; i < fd_max; i++) {
		if (session[i] && (sd = session[i]->session_data) && sd->state.auth &&
		    (sd->GM_level >= battle_config.hide_GM_session || (sd->status.option & OPTION_HIDE))) {
			WPACKETL(9 + 4 * (users + hidden_users)) = sd->status.char_id;
			hidden_users++;
		}
	}
	// Finish packet
	WPACKETW(0) = 0x2aff;
	WPACKETW(2) = 9 + 4 * (users + hidden_users); // size
	WPACKETW(4) = users;
	WPACKETW(6) = hidden_users;
	WPACKETB(8) = agit_flag; // 0: WoE not starting, Woe is running
	SENDPACKET(char_fd, WPACKETW(2));

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int check_connect_char_server(int tid, unsigned int tick, int id, int data) {

	if (char_fd < 0 || session[char_fd] == NULL) {
		printf("Attempting to connect to char-server (%s:%d). Please wait...\n", char_ip_str, char_port);
		chrif_state = 0;
		char_fd = make_connection(char_ip, char_port);
		if (char_fd != -1) {
			session[char_fd]->func_parse = chrif_parse;
			realloc_fifo(char_fd, RFIFOSIZE_SERVER, WFIFOSIZE_SERVER);

			chrif_connect(char_fd);
		} else
			printf("Can not connect to char-server.\n");
#ifndef TXT_ONLY
		srvinfo = 0;
	} else {
		if (srvinfo == 0) {
			chrif_ragsrvinfo(battle_config.base_exp_rate, battle_config.job_exp_rate, battle_config.item_rate_common);
			srvinfo = 1;
		}
#endif /* not TXT_ONLY */
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int do_final_chrif(void) {

	if (char_fd >= 0) {
#ifdef __WIN32
		shutdown(char_fd, SD_BOTH);
		closesocket(char_fd);
#else
		close(char_fd);
#endif
		delete_session(char_fd);
		char_fd = -1;
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int do_init_chrif(void) {

	char_fd = -1;
	chrif_state = 0;

	add_timer_func_list(check_connect_char_server, "check_connect_char_server");
	add_timer_func_list(send_users_tochar, "send_users_tochar");
	add_timer_interval(gettick_cache + 1000, check_connect_char_server, 0, 0, 5 * 1000);
	add_timer_interval(gettick_cache + 1000, send_users_tochar, 0, 0, 5 * 1000);

	return 0;
}
