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
#endif
#include <stdio.h>
#include <stdlib.h>
#ifndef LCCWIN32
#include <unistd.h>
#endif
#include <signal.h>
#include <fcntl.h>
#include <string.h>

#include "../common/socket.h"
#include "../common/timer.h"
#include "map.h"
#include "battle.h"
#include "chrif.h"
#include "clif.h"
#include "pc.h"
#include "intif.h"
#include "storage.h"
#include "party.h"
#include "guild.h"
#include "pet.h"
#include "nullpo.h"
#include "atcommand.h"
#include "../common/malloc.h"

#ifdef MEMWATCH
#include "memwatch.h"
#endif

void mapif_parse_WisToGM(char *Wisp_name, char* mes, short len, short min_gm_level); // 0x3003/0x3803 <packet_len>.w <wispname>.24B <min_gm_level>.w <message>.?B
void mapif_parse_MainMessage(char *Wisp_name, char* mes, short len); // 0x3006/0x3806 <packet_len>.w <wispname>.24B <message>.?B
int mapif_parse_MessageToGM(char *Wisp_name, char* mes, short len); // 0x3007/0x3807 <packet_len>.w <wispname>.24B <message>.?B

static const int packet_len_table[] = {
	-1,-1,27,-1, -1, 6, -1, -1,  0,-1, 0, 0,  0, 0,  0, 0, // 0x3800-0x380f
	-1, 6,-1, 0,  0, 0,  0,  0, -1, 0, 0, 0,  0, 0,  0, 0, // 0x3810-0x381f
	35,-1,11,15, 34,29,  6, -1,  0, 0, 0, 0,  0, 0,  0, 0, // 0x3820-0x382f
	10,-1,15, 0, 79,19,  7, -1,  0,-1,-1,-1, 14,67,186,-1, // 0x3830-0x383f
	 9, 9,-1, 0,  0, 0,  0,  0,  0, 0, 0, 0,  0, 0,  0, 0, // 0x3840-0x384f
	 0, 0, 0, 0,  0, 0,  0,  0,  0, 0, 0, 0,  0, 0,  0, 0, // 0x3850-0x385f
	 0, 0, 0, 0,  0, 0,  0,  0,  0, 0, 0, 0,  0, 0,  0, 0, // 0x3860-0x386f
	 0, 0, 0, 0,  0, 0,  0,  0,  0, 0, 0, 0,  0, 0,  0, 0, // 0x3870-0x387f
	11,-1, 7, 3,  0, 0,  0,  0,  0, 0, 0, 0,  0, 0,  0, 0, // 0x3880-0x388f
};

extern int char_fd; // inter serverのfdはchar_fdを使う
#define inter_fd (char_fd) // エイリアス

//-----------------------------------------------------------------
// inter serverへの送信

// pet
void intif_create_pet(int account_id, int char_id, short pet_class, short pet_lv, short pet_egg_id,
                      short pet_equip, short intimate, short hungry, char rename_flag, char incuvate, char *pet_name) {
	WPACKETW( 0) = 0x3080;
	WPACKETL( 2) = account_id;
	WPACKETL( 6) = char_id;
	WPACKETW(10) = pet_class;
	WPACKETW(12) = pet_lv;
	WPACKETW(14) = pet_egg_id;
	WPACKETW(16) = pet_equip;
	WPACKETW(18) = intimate;
	WPACKETW(20) = hungry;
	WPACKETB(22) = rename_flag;
	WPACKETB(23) = incuvate;
	strncpy(WPACKETP(24), pet_name, 24);
	SENDPACKET(inter_fd, 48);

	return;
}

void intif_request_petdata(int account_id, int char_id, int pet_id) {
	WPACKETW(0) = 0x3081;
	WPACKETL(2) = account_id;
	WPACKETL(6) = char_id;
	WPACKETL(10) = pet_id;
	SENDPACKET(inter_fd, 14);

	return;
}

void intif_save_petdata(int account_id, struct s_pet *p) {
	WPACKETW(0) = 0x3082;
	WPACKETW(2) = sizeof(struct s_pet) + 8;
	WPACKETL(4) = account_id;
	memcpy(WPACKETP(8), p, sizeof(struct s_pet));
	SENDPACKET(inter_fd, sizeof(struct s_pet) + 8);

	return;
}

void intif_delete_petdata(int pet_id) {
	WPACKETW(0) = 0x3083;
	WPACKETL(2) = pet_id;
	SENDPACKET(inter_fd, 6);

	return;
}

// GMメッセージを送信
int intif_GMmessage(char* mes, int flag) { // 0x3000/0x3800 <packet_len>.w <message>.?B
	short msg_len;
	char *buf; // we can not send WPACKETP(4). clif_GMmessage overwrite it

	msg_len = strlen(mes) + 1; // +1: NULL // +1 to copy NULL too (because short mesage can have part of the 'blue')

	if (msg_len > 1) { // at least 1 char (without the NULL)
		// prepare message
		if (flag & 0x10) {
			CALLOC(buf, char, msg_len + 4);
			WBUFL(buf, 0) = 0x65756c62; // eulb
			memcpy(WBUFP(buf, 4), mes, msg_len); // +1 to copy NULL too (because short mesage can have part of the 'blue')
			msg_len = msg_len + 4;
		} else {
			CALLOC(buf, char, msg_len);
			memcpy(WBUFP(buf, 0), mes, msg_len); // +1 to copy NULL too (because short mesage can have part of the 'blue')
		}
		// send message (if multi-servers)
		if (!map_is_alone) {
			WPACKETW(0) = 0x3000; // 0x3000/0x3800 <packet_len>.w <message>.?B
			WPACKETW(2) = 4 + msg_len;
			memcpy(WPACKETP(4), mes, msg_len);
			SENDPACKET(inter_fd, WPACKETW(2));
		}
		// send to local players
		clif_GMmessage(NULL, buf, msg_len, 0);
		FREE(buf);
	}

	return 0;
}

void intif_announce(char* mes, unsigned int color, unsigned int flag) {
	clif_announce(NULL, mes, color, flag);

		// send message (if multi-servers)
	if (!map_is_alone) {
		WPACKETW(0) = 0x3009; // 0x3009/0x3809 <packet_len>.w <color>.L <flag>.L <message>.?B
		WPACKETW(2) = 12 + strlen(mes) + 1;
		WPACKETL(4) = color;
		WPACKETL(8) = flag;
		strcpy(WPACKETP(12), mes);
		SENDPACKET(inter_fd, WPACKETW(2));
		}

	return;
}

// The transmission of Wisp/Page to inter-server (player not found on this server)
int intif_wis_message(struct map_session_data *sd, char *nick, char *mes, int mes_len) {
	nullpo_retr(0, sd);

	WPACKETW(0) = 0x3001; // 0x3001/0x3801 <packet_len>.w (<w_id_0x3801>.L) <sender_GM_level>.B <sender_name>.24B <nick_name>.24B <message>.?B
	WPACKETW(2) = mes_len + 53 + 1; // + NULL
	WPACKETB(4) = sd->GM_level; // need for pm_gm_not_ignored option
	strncpy(WPACKETP( 5), sd->status.name, 24);
	strncpy(WPACKETP(29), nick, 24);
	strncpy(WPACKETP(53), mes, mes_len);
	WPACKETB(53 + mes_len) = 0;
	SENDPACKET(inter_fd, mes_len + 53 + 1); // + NULL

	if (battle_config.etc_log)
		printf("intif_wis_message from %s to %s (message: '%s')\n", sd->status.name, nick, mes);

	return 0;
}

// The reply of Wisp/page
int intif_wis_replay(int id, int flag) {
	WPACKETW(0) = 0x3002; // 0x3002 <Wis_id>.L <flag>.B (flag: 0: success to send wisper, 1: target character is not loged in?, 2: ignored by target)
	WPACKETL(2) = id;
	WPACKETB(6) = flag; // flag: 0: success to send wisper, 1: target character is not loged in?, 2: ignored by target
	SENDPACKET(inter_fd, 7);

	if (battle_config.etc_log)
		printf("intif_wis_replay: id: %d, flag:%d\n", id, flag);

	return 0;
}

// The transmission of GM only Wisp/Page from server to inter-server
void intif_wis_message_to_gm(char *Wisp_name, short min_gm_level, char *mes) {
	int mes_len;

	mes_len = strlen(mes) + 1; // + null

	// send message (if multi-servers)
	if (!map_is_alone) {
		WPACKETW( 0) = 0x3003; // 0x3003/0x3803 <packet_len>.w <wispname>.24B <min_gm_level>.w <message>.?B
		WPACKETW( 2) = mes_len + 30;
		strncpy(WPACKETP( 4), Wisp_name, 24);
		WPACKETW(28) = min_gm_level;
		strncpy(WPACKETP(30), mes, mes_len);
		SENDPACKET(inter_fd, mes_len + 30);
	}

	// send to local players
	mapif_parse_WisToGM(Wisp_name, mes, mes_len, min_gm_level);

	if (battle_config.etc_log)
		printf("intif_wis_message_to_gm: from: '%s', min level: %d, message: '%s'.\n", Wisp_name, min_gm_level, mes);

	return;
}

// The transmission of message on main channel from server to inter-server
void intif_main_message(char *Wisp_name, char *mes) {
	int mes_len;

	mes_len = strlen(mes) + 1; // + null

	// send message (if multi-servers)
	if (!map_is_alone) {
		WPACKETW( 0) = 0x3006; // 0x3006/0x3806 <packet_len>.w <wispname>.24B <message>.?B
		WPACKETW( 2) = mes_len + 28;
		strncpy(WPACKETP( 4), Wisp_name, 24);
		strncpy(WPACKETP(28), mes, mes_len);
		SENDPACKET(inter_fd, mes_len + 28);
	}

	// send to local players
	mapif_parse_MainMessage(Wisp_name, mes, mes_len);

	if (battle_config.etc_log)
		printf("intif_main_message: from: '%s', message: '%s'.\n", Wisp_name, mes);

	return;
}

// The transmission of message to GMs from server to inter-server
int intif_gm_message(char *Wisp_name, char *mes) {
	int mes_len;

	if (battle_config.etc_log)
		printf("intif_gm_message: from: '%s', message: '%s'.\n", Wisp_name, mes);

	mes_len = strlen(mes) + 1; // + null

	// send message (if multi-servers)
	if (!map_is_alone) {
		WPACKETW( 0) = 0x3007; // 0x3007/0x3807 <packet_len>.w <wispname>.24B <message>.?B
		WPACKETW( 2) = mes_len + 28;
		strncpy(WPACKETP( 4), Wisp_name, 24);
		strncpy(WPACKETP(28), mes, mes_len);
		SENDPACKET(inter_fd, mes_len + 28);
	}

	// send to local players (and return number of online GMs on this server that have receive the message)
	return mapif_parse_MessageToGM(Wisp_name, mes, mes_len);
}

// The transmission of logs to inter-server
void intif_send_log(unsigned char log_type, char *log_msg) {
	int log_len;

	log_len = strlen(log_msg) + 1; // + null

	WPACKETW( 0) = 0x3008; // 0x3008 <packet_len>.w <log_type>.B <message>.?B (types: 1 GM commands, 2: Trades, 3: Scripts, 4: Vending)
	WPACKETW( 2) = log_len + 5 + 1; // NULL
	WPACKETB( 4) = log_type;
	memcpy(WPACKETP(5), log_msg, log_len + 1); // NULL
	SENDPACKET(inter_fd, log_len + 5 + 1);

	return;
}


// アカウント変数送信
void intif_saveaccountreg(struct map_session_data *sd) {
	int size;

//	nullpo_retv(sd); // checked before to call function

	//printf("intif_saveaccountreg: account %d, num of account_reg: %d.\n", sd->bl.id, sd->account_reg_num);
	if (sd->account_reg_num > 0) {
		size = sizeof(struct global_reg) * sd->account_reg_num;
		WPACKETW(0) = 0x3004; // 0x3004 <packet_len>.w <account_id>.L account_reg.structure.*B
		WPACKETW(2) = 8 + size;
		WPACKETL(4) = sd->bl.id; // account_id
		memcpy(WPACKETP(8), sd->account_reg, size);
		SENDPACKET(inter_fd, 8 + size);
	} else {
		WPACKETW(0) = 0x3004; // 0x3004 <packet_len>.w <account_id>.L account_reg.structure.*B
		WPACKETW(2) = 8;
		WPACKETL(4) = sd->bl.id; // account_id
		SENDPACKET(inter_fd, 8);
	}

	return;
}

// アカウント変数要求
/*int intif_request_accountreg(struct map_session_data *sd) // now send at same moment of character (synchronized)
{
	nullpo_retr(0, sd);

	WPACKETW(0) = 0x3005; // 0x3005 <account_id>.L
	WPACKETL(2) = sd->bl.id;
	SENDPACKET(inter_fd, 6);

	return 0;
}*/

// 倉庫データ要求
int intif_request_storage(int account_id)
{
	WPACKETW(0) = 0x3010;
	WPACKETL(2) = account_id;
	SENDPACKET(inter_fd, 6);

	return 0;
}

// 倉庫データ送信
int intif_send_storage(struct storage *stor)
{
	nullpo_retr(0, stor);

	WPACKETW(0) = 0x3011;
	WPACKETW(2) = sizeof(struct storage) + 8;
	WPACKETL(4) = stor->account_id;
	memcpy(WPACKETP(8), stor, sizeof(struct storage));
	SENDPACKET(inter_fd, sizeof(struct storage) + 8);

	return 0;
}

int intif_request_guild_storage(int account_id, int guild_id)
{
	WPACKETW(0) = 0x3018;
	WPACKETL(2) = account_id;
	WPACKETL(6) = guild_id;
	SENDPACKET(inter_fd, 10);

	return 0;
}

int intif_send_guild_storage(int account_id, struct guild_storage *gstor)
{
	WPACKETW(0) = 0x3019;
	WPACKETW(2) = sizeof(struct guild_storage) + 12;
	WPACKETL(4) = account_id;
	WPACKETL(8) = gstor->guild_id;
	memcpy(WPACKETP(12), gstor, sizeof(struct guild_storage));
	SENDPACKET(inter_fd, sizeof(struct guild_storage) + 12);

	return 0;
}

// パーティ作成要求
void intif_create_party(struct map_session_data *sd, char *party_name, short item, short item2) {
//	nullpo_retv(sd); // checked before to call function

	WPACKETW( 0) = 0x3020;
	WPACKETL( 2) = sd->status.account_id;
	strncpy(WPACKETP( 6), party_name, 24);
	strncpy(WPACKETP(30), sd->status.name, 24);
	strncpy(WPACKETP(54), map[sd->bl.m].name, 16);
	WPACKETW(70) = sd->status.base_level;
	WPACKETB(72) = item;
	WPACKETB(73) = item2;
	SENDPACKET(inter_fd, 74);

	return;
}

// パーティ情報要求
int intif_request_partyinfo(int party_id) {
	WPACKETW(0) = 0x3021; // 0x3021 <party_id>.L - ask for party
	WPACKETL(2) = party_id;
	SENDPACKET(inter_fd, 6);
//	if (battle_config.etc_log)
//		printf("intif: request party info (party id: %d).\n", party_id);

	return 0;
}

// パーティ追加要求
void intif_party_addmember(int party_id, struct map_session_data *sd) {
//	nullpo_retv(sd); // checked before to call function

	WPACKETW( 0) = 0x3022;
	WPACKETL( 2) = party_id;
	WPACKETL( 6) = sd->status.account_id;
	strncpy(WPACKETP(10), sd->status.name, 24);
	strncpy(WPACKETP(34), map[sd->bl.m].name, 16);
	WPACKETW(50) = sd->status.base_level;
	SENDPACKET(inter_fd, 52);

	return;
}

// パーティ設定変更
void intif_party_changeoption(int party_id, int account_id, unsigned short party_exp, unsigned short item) {
	WPACKETW( 0) = 0x3023;
	WPACKETL( 2) = party_id;
	WPACKETL( 6) = account_id;
	WPACKETW(10) = party_exp;
	WPACKETW(12) = item;
	SENDPACKET(inter_fd, 14);

	return;
}

// パーティ脱退要求
void intif_party_leave(int party_id, int account_id) {

	WPACKETW(0) = 0x3024;
	WPACKETL(2) = party_id;
	WPACKETL(6) = account_id;
	SENDPACKET(inter_fd, 10);

	return;
}

// パーティ移動要求
int intif_party_changemap(struct map_session_data *sd, unsigned char online) { // flag: 0: offline, 1:online
	if (sd != NULL) {
		WPACKETW( 0) = 0x3025;
		WPACKETL( 2) = sd->status.party_id;
		WPACKETL( 6) = sd->status.account_id;
		strncpy(WPACKETP(10), map[sd->bl.m].name, 16);
		WPACKETB(26) = online; // flag: 0: offline, 1:online
		WPACKETW(27) = sd->status.base_level;
		SENDPACKET(inter_fd, 29);
	}
//	if (battle_config.etc_log)
//		printf("party: change map\n");

	return 0;
}

// パーティー解散要求
int intif_break_party(int party_id)
{
	WPACKETW(0) = 0x3026;
	WPACKETL(2) = party_id;
	SENDPACKET(inter_fd, 6);

	return 0;
}

// パーティ会話送信
void intif_party_message(int party_id, int account_id, char *mes, int len) {
	WPACKETW(0) = 0x3027;
	WPACKETW(2) = len + 12;
	WPACKETL(4) = party_id;
	WPACKETL(8) = account_id;
	strncpy(WPACKETP(12), mes, len);
	SENDPACKET(inter_fd, len + 12);

	return;
}

// パーティ競合チェック要求
int intif_party_checkconflict(int party_id, int account_id, char *nick) {
	WPACKETW(0) = 0x3028;
	WPACKETL(2) = party_id;
	WPACKETL(6) = account_id;
	strncpy(WPACKETP(10), nick, 24);
	SENDPACKET(inter_fd, 34);

	return 0;
}

// ギルド作成要求
void intif_guild_create(const char *name, const struct guild_member *master) {
//	nullpo_retv(sd); // checked before to call function

	WPACKETW(0) = 0x3030;
	WPACKETW(2) = sizeof(struct guild_member) + 32;
	WPACKETL(4) = master->account_id;
	strncpy(WPACKETP(8), name, 24);
	memcpy(WPACKETP(32), master, sizeof(struct guild_member));
	SENDPACKET(inter_fd, sizeof(struct guild_member) + 32);

	return;
}

// ギルド情報要求
int intif_guild_request_info(int guild_id) {
	WPACKETW(0) = 0x3031; // 0x3031 <guild_id>.L - ask for guild
	WPACKETL(2) = guild_id;
	SENDPACKET(inter_fd, 6);

	return 0;
}

// ギルドメンバ追加要求
int intif_guild_addmember(int guild_id, struct guild_member *m)
{
	WPACKETW(0) = 0x3032;
	WPACKETW(2) = sizeof(struct guild_member) + 8;
	WPACKETL(4) = guild_id;
	memcpy(WPACKETP(8), m, sizeof(struct guild_member));
	SENDPACKET(inter_fd, sizeof(struct guild_member) + 8);

	return 0;
}

// ギルドメンバ脱退/追放要求
void intif_guild_leave(int guild_id, int account_id, int char_id, int flag, const char *mes) {
	WPACKETW( 0) = 0x3034;
	WPACKETL( 2) = guild_id;
	WPACKETL( 6) = account_id;
	WPACKETL(10) = char_id;
	WPACKETB(14) = flag;
	strncpy(WPACKETP(15), mes, 40);
	SENDPACKET(inter_fd, 55);

	return;
}

// ギルドメンバのオンライン状況/Lv更新要求
int intif_guild_memberinfoshort(int guild_id,
	int account_id, int char_id, int online, int lv, int class)
{
	WPACKETW( 0) = 0x3035;
	WPACKETL( 2) = guild_id;
	WPACKETL( 6) = account_id;
	WPACKETL(10) = char_id;
	WPACKETB(14) = online;
	WPACKETW(15) = lv;
	WPACKETW(17) = class;
	SENDPACKET(inter_fd, 19);

	return 0;
}

int intif_guild_break(int guild_id)
{
	WPACKETW(0) = 0x3036;
	WPACKETL(2) = guild_id;
	SENDPACKET(inter_fd, 6);

	return 0;
}

// ギルド会話送信
int intif_guild_message(int guild_id, int account_id, char *mes, int len) {
	WPACKETW(0) = 0x3037;
	WPACKETW(2) = len + 12;
	WPACKETL(4) = guild_id;
	WPACKETL(8) = account_id;
	strncpy(WPACKETP(12), mes, len);
	SENDPACKET(inter_fd, len + 12);

	return 0;
}

// ギルド競合チェック要求
int intif_guild_checkconflict(int guild_id, int account_id, int char_id)
{
	WPACKETW( 0) = 0x3038;
	WPACKETL( 2) = guild_id;
	WPACKETL( 6) = account_id;
	WPACKETL(10) = char_id;
	SENDPACKET(inter_fd, 14);

	return 0;
}

// ギルド基本情報変更要求
int intif_guild_change_basicinfo(int guild_id, int type, const void *data, int len)
{
	WPACKETW(0) = 0x3039;
	WPACKETW(2) = len + 10;
	WPACKETL(4) = guild_id;
	WPACKETW(8) = type;
	memcpy(WPACKETP(10), data, len);
	SENDPACKET(inter_fd, len + 10);

	return 0;
}

// ギルドメンバ情報変更要求
void intif_guild_change_memberinfo(int guild_id, int account_id, int char_id,
	int type, const void *data, int len)
{
	WPACKETW( 0) = 0x303a;
	WPACKETW( 2) = len + 18;
	WPACKETL( 4) = guild_id;
	WPACKETL( 8) = account_id;
	WPACKETL(12) = char_id;
	WPACKETW(16) = type;
	memcpy(WPACKETP(18), data, len);
	SENDPACKET(inter_fd, len + 18);

	return;
}

// ギルド役職変更要求
void intif_guild_position(int guild_id, int idx, struct guild_position *p) {
	WPACKETW(0) = 0x303b;
	WPACKETW(2) = sizeof(struct guild_position) + 12;
	WPACKETL(4) = guild_id;
	WPACKETL(8) = idx;
	memcpy(WPACKETP(12), p, sizeof(struct guild_position));
	SENDPACKET(inter_fd, sizeof(struct guild_position) + 12);

	return;
}

// ギルドスキルアップ要求
int intif_guild_skillup(int guild_id, int skill_num, int account_id, int flag)
{
	WPACKETW( 0) = 0x303c;
	WPACKETL( 2) = guild_id;
	WPACKETL( 6) = skill_num;
	WPACKETL(10) = account_id;
	WPACKETL(14) = flag;
	SENDPACKET(inter_fd, 18);

	return 0;
}

// ギルド同盟/敵対要求
void intif_guild_alliance(int guild_id1, int guild_id2, int account_id1, int account_id2, unsigned char flag) {
	WPACKETW( 0) = 0x303d;
	WPACKETL( 2) = guild_id1;
	WPACKETL( 6) = guild_id2;
	WPACKETL(10) = account_id1;
	WPACKETL(14) = account_id2;
	WPACKETB(18) = flag;
	SENDPACKET(inter_fd, 19);

	return;
}

// ギルド告知変更要求
void intif_guild_notice(int guild_id, const char *mes1, const char *mes2) {
	WPACKETW(0) = 0x303e;
	WPACKETL(2) = guild_id;
	strncpy(WPACKETP( 6), mes1, 60);
	strncpy(WPACKETP(66), mes2, 120);
	SENDPACKET(inter_fd, 186);

	return;
}

// ギルドエンブレム変更要求
void intif_guild_emblem(int guild_id, unsigned short len, const char *data) {
//	if (guild_id <= 0 || len < 0 || len > 2000) // checked before to call function
//		return;

	WPACKETW(0) = 0x303f;
	WPACKETW(2) = len + 12;
	WPACKETL(4) = guild_id;
	WPACKETL(8) = 0;
	memcpy(WPACKETP(12), data, len);
	SENDPACKET(inter_fd, len + 12);

	return;
}

//現在のギルド城占領ギルドを調べる
int intif_guild_castle_dataload(int castle_id, int idx)
{
	WPACKETW(0) = 0x3040;
	WPACKETW(2) = castle_id;
	WPACKETB(4) = idx;
	SENDPACKET(inter_fd, 5);

	return 0;
}

//ギルド城占領ギルド変更要求
int intif_guild_castle_datasave(int castle_id, int idx, int value)
{
	WPACKETW(0) = 0x3041;
	WPACKETW(2) = castle_id;
	WPACKETB(4) = idx;
	WPACKETL(5) = value;
	SENDPACKET(inter_fd, 9);

	return 0;
}

/*==========================================
 * 指定した名前のキャラの場所要求
 *------------------------------------------
 */
int intif_charposreq(int account_id, char *name, int flag) {
	WPACKETW( 0) = 0x3090;
	WPACKETL( 2) = account_id;
	strncpy(WPACKETP(6), name, 24);
	WPACKETB(30) = flag;
	SENDPACKET(inter_fd, 31);

	return 0;
}

/*==========================================
 * 指定した名前のキャラの場所に移動する
 * @jumpto
 *------------------------------------------
 */
int intif_jumpto(int account_id, char *name) {
	intif_charposreq(account_id, name, 1);
	//printf("intif_jumpto: %d %s\n", account_id,name);

	return 0;
}

/*==========================================
 * 指定した名前のキャラの場所表示する
 * @where
 *------------------------------------------
 */
int intif_where(int account_id, char *name)
{
	intif_charposreq(account_id, name, 0);
	//printf("intif_where: %d %s\n", account_id,name);

	return 0;
}

/*==========================================
 * 指定した名前のキャラを呼び寄せる
 * flag=0 あなたに逢いたい
 * flag=1 @recall
 *------------------------------------------
 */
int intif_charmovereq(struct map_session_data *sd, char *name, int flag)
{
	nullpo_retr(0, sd);

	//printf("intif_charmovereq: %d %s\n", sd->status.account_id, name);
	if (name == NULL)
		return -1;

	WPACKETW( 0) = 0x3092;
	WPACKETL( 2) = sd->status.account_id;
	strncpy(WPACKETP( 6), name, 24);
	WPACKETB(30) = flag;
	strncpy(WPACKETP(31), sd->mapname, 16);
	WPACKETW(47) = sd->bl.x;
	WPACKETW(49) = sd->bl.y;
	SENDPACKET(inter_fd, 51);

	return 0;
}

/*==========================================
 * 対象IDにメッセージを送信
 *------------------------------------------
 */
int intif_displaymessage(int account_id, char* mes)
{
	int len = strlen(mes);

	WPACKETW(0) = 0x3093;
	WPACKETW(2) = len + 9; // 8 + NULL
	WPACKETL(4) = account_id;
	strncpy(WPACKETP(8), mes, len);
	WPACKETB(len + 9 - 1) = 0; // NULL
	SENDPACKET(inter_fd, len + 9);

	return 0;
}
//-----------------------------------------------------------------
// Packets receive from inter server

// Wisp/Page reception
int intif_parse_WisMessage(int fd) { // 0x3001/0x3801 <packet_len>.w (<w_id_0x3801>.L) <sender_GM_level>.B <sender_name>.24B <nick_name>.24B <message>.?B
	struct map_session_data* sd;
	char wisp_source[25];
	char nick_name[25];
	int i;

	memset(wisp_source, 0, sizeof(wisp_source));
	strncpy(wisp_source, RFIFOP(fd,9), 24);
	memset(nick_name, 0, sizeof(nick_name));
	strncpy(nick_name, RFIFOP(fd,33), 24);

	if (battle_config.etc_log)
		printf("intif_parse_wismessage: id: %d, from: %s, to: %s, message: '%s'\n", RFIFOL(fd,4), wisp_source, nick_name, RFIFOP(fd,57));
	sd = map_nick2sd(nick_name); // Searching destination player
	if (sd != NULL && strcmp(sd->status.name, nick_name) == 0) { // exactly same name (inter-server have checked the name before)
		// if GM pm, not ignore it
		if (RFIFOB(fd,8) >= battle_config.pm_gm_not_ignored && RFIFOB(fd,8) >= sd->GM_level) {
			clif_wis_message(sd->fd, wisp_source, RFIFOP(fd,57), RFIFOW(fd,2) - 57);
			intif_wis_replay(RFIFOL(fd,4), 0); // flag: 0: success to send wisper, 1: target character is not loged in?, 2: ignored by target
		// if normal message
		} else {
			// if player ignore all
			if (sd->ignoreAll == 1)
				intif_wis_replay(RFIFOL(fd,4), 2); // flag: 0: success to send wisper, 1: target character is not loged in?, 2: ignored by target
			else {
				// if player ignore the source character
				for(i = 0; i < sd->ignore_num; i++)
					if (strcmp(sd->ignore[i].name, wisp_source) == 0) {
						intif_wis_replay(RFIFOL(fd,4), 2); // flag: 0: success to send wisper, 1: target character is not loged in?, 2: ignored by target
						break;
					}
				// if source player not found in ignore list
				if (i == sd->ignore_num) {
					clif_wis_message(sd->fd, wisp_source, RFIFOP(fd,57), RFIFOW(fd,2) - 57);
					intif_wis_replay(RFIFOL(fd,4), 0); // flag: 0: success to send wisper, 1: target character is not loged in?, 2: ignored by target
				}
			}
		}
	} else
		intif_wis_replay(RFIFOL(fd,4), 1); // flag: 0: success to send wisper, 1: target character is not loged in?, 2: ignored by target

	return 0;
}

// Wisp/page transmission result reception
int intif_parse_WisEnd(int fd) { // 0x3802 <sender_name>.24B <flag>.B (flag: 0: success to send wisper, 1: target character is not loged in?, 2: ignored by target)
	struct map_session_data* sd;

	if (battle_config.etc_log)
		printf("intif_parse_wisend: player: %s, flag: %d\n", RFIFOP(fd,2), RFIFOB(fd,26)); // flag: 0: success to send wisper, 1: target character is not loged in?, 2: ignored by target
	sd = map_nick2sd(RFIFOP(fd,2));
	if (sd != NULL)
		clif_wis_end(sd->fd, RFIFOB(fd,26));

	return 0;
}

// Received wisp message from map-server via char-server for ALL gm
void mapif_parse_WisToGM(char *Wisp_name, char* mes, short len, short min_gm_level) { // 0x3003/0x3803 <packet_len>.w <wispname>.24B <min_gm_level>.w <message>.?B
	int i;
	struct map_session_data *pl_sd;
	char *message;
	char Wispname[25];

	if (min_gm_level < 0)
		min_gm_level = 0;
	else if (min_gm_level > 99)
		return;

	memset(Wispname, 0, sizeof(Wispname));
	strncpy(Wispname, Wisp_name, 24);

	CALLOC(message, char, len + strlen(msg_txt(593)) + strlen(msg_txt(541)) + 10 + 1); // (to gm >= %d) (+10 numeric: %d->22222) + (1) NULL terminated
	if (min_gm_level == 0)
		sprintf(message, msg_txt(593), min_gm_level); // (to all players) 
	else
		sprintf(message, msg_txt(541), min_gm_level); // (to GM >= %d) 
	strncpy(message + strlen(message), mes, len);

	// information is sended to all online GM
	for (i = 0; i < fd_max; i++)
		if (session[i] && (pl_sd = session[i]->session_data) && pl_sd->state.auth)
			if (pl_sd->GM_level >= min_gm_level)
				clif_wis_message(i, Wispname, message, strlen(message) + 1);

	FREE(message);

	return;
}

// アカウント変数通知
void intif_parse_AccountReg(int fd) { // 0x3804 <packet_len>.w <account_id>.L {<account_reg_str>.32B <account_reg_value>.L}.x
	int j, p;
	struct map_session_data *sd;

	if ((sd = map_id2sd(RFIFOL(fd,4))) == NULL)
		return;

	//FREE(sd->account_reg); // not already initialized
	j = 0;
	if (RFIFOW(fd,2) > 8) {
		MALLOC(sd->account_reg, struct global_reg, (RFIFOW(fd,2) - 8) / 36);
		for(p = 8; p < RFIFOW(fd,2); p += 36) {
			memcpy(sd->account_reg[j].str, RFIFOP(fd,p), 32); // sended with completed by 0 strings -> memcpy speeder than strncpy
			sd->account_reg[j].str[32] = '\0';
			sd->account_reg[j].value = RFIFOL(fd, p + 32);
//			printf("intif: accountreg #%d: %s: %d.\n", j, sd->account_reg[j].str, sd->account_reg[j].value);
			j++;
		}
	}
	sd->account_reg_num = j;
//	printf("intif: accountreg (account_reg_num: %d).\n", j);

	return;
}

// Account registers saved. remove flag
void intif_parse_AccountRegAck(int fd) { // 0x3805 <account_id>.L
	struct map_session_data *sd;

	if ((sd = map_id2sd(RFIFOL(fd,2))) == NULL)
		return;

	sd->account_reg_dirty = 0; // must be saved or not

	return;
}

// Received main message from map-server via char-server for ALL players
void mapif_parse_MainMessage(char *Wisp_name, char* mes, short len) { // 0x3006/0x3806 <packet_len>.w <wispname>.24B <message>.?B
	int i;
	struct map_session_data *pl_sd;
	char *message;
	char Wispname[25];

	memset(Wispname, 0, sizeof(Wispname));
	strncpy(Wispname, Wisp_name, 24);

	CALLOC(message, char, len + strlen(msg_txt(603)) + 25 + 1); // msgtxt + Wispname (24) + len + (1) NULL terminated
	sprintf(message, msg_txt(603), Wispname); // (Main) '%s': 
	strncpy(message + strlen(message), mes, len);

	// information is sended to all online player
	for (i = 0; i < fd_max; i++)
		if (session[i] && (pl_sd = session[i]->session_data) && pl_sd->state.auth)
			if (pl_sd->state.main_flag) { // if main is activated
				if (battle_config.atcommand_main_channel_type == -5) { // -5: like a GM message (in blue)
					clif_GMmessage(&pl_sd->bl, message, strlen(message) + 1, 3 | 0x10); // 3 -> SELF + 0x10 for blue
				} else if (battle_config.atcommand_main_channel_type == -4) { // -4: like a GM message (in yellow)
					clif_GMmessage(&pl_sd->bl, message, strlen(message) + 1, 3); // 3 -> SELF
//				} else if (battle_config.atcommand_main_channel_type == -3) { // -3: like a guild message (default)
//					clif_disp_onlyself(pl_sd, message);
				} else if (battle_config.atcommand_main_channel_type == -2) { // -2: like a party message
					clif_party_message_self(pl_sd, message, strlen(message) + 1);
				} else if (battle_config.atcommand_main_channel_type == -1) { // -1: like a chat message
					clif_displaymessage(pl_sd->fd, message);
				} else if (battle_config.atcommand_main_channel_type >= 0 && battle_config.atcommand_main_channel_type <= 0xFFFFFF) { // 0 to 16777215 (0xFFFFFF): like a colored GM message (set the color of the GM message; each basic color from 0 to 255 -> (65536 * Red + 256 * Green + Blue))
					clif_announce(&pl_sd->bl, message, battle_config.atcommand_main_channel_type, 3); // flag = 3 = SELF
				} else { // -3: like a guild message (default)
					clif_disp_onlyself(pl_sd, message);
				}
			}

	FREE(message);

	return;
}

// Received message from map-server via char-server for ALL GMs
int mapif_parse_MessageToGM(char *Wisp_name, char* mes, short len) { // 0x3007/0x3807 <packet_len>.w <wispname>.24B <message>.?B
	int i, gm;
	struct map_session_data *pl_sd;
	char *message;
	char Wispname[25];

	memset(Wispname, 0, sizeof(Wispname));
	strncpy(Wispname, Wisp_name, 24);

	CALLOC(message, char, len + strlen(msg_txt(604)) + 25 + 1); // msgtxt + Wispname (24) + len + (1) NULL terminated
	sprintf(message, msg_txt(604), Wispname); // (Request from '%s'): 
	strncpy(message + strlen(message), mes, len);

	// information is sended to all online player (same test in @request GM command)
	gm = 0;
	for (i = 0; i < fd_max; i++)
		if (session[i] && (pl_sd = session[i]->session_data) && pl_sd->state.auth)
			if (pl_sd->GM_level >= battle_config.atcommand_min_GM_level_for_request) // if it is authorized GM
				if (pl_sd->state.refuse_request_flag == 0) // if GM doesn't refuse requests
					if (strcmp(Wispname, pl_sd->status.name) != 0) { // if not the request people
						clif_displaymessage(pl_sd->fd, message);
						gm++;
					}

	FREE(message);

	return gm; // return number of online GMs on this server that have receive the message
}

// loading of storage when character is loaded
int intif_parse_GetStorage(int fd) {
	struct storage *stor;
	struct map_session_data *sd;

	sd = map_id2sd(RFIFOL(fd,4));
	if (sd == NULL) {
		if (battle_config.error_log)
			printf("intif_parse_GetStorage: user not found %d\n", RFIFOL(fd,4));
		return 1;
	}

	if (RFIFOW(fd,2) - 8 != sizeof(struct storage)) {
		if (battle_config.error_log) {
			printf("intif_parse_GetStorage: data size error %d != %d.\n", RFIFOW(fd,2) - 8, sizeof(struct storage));
			printf("                        Please use same version of char-server and map-server.\n");
		}
		return 1;
	}

	stor = account2storage(RFIFOL(fd,4));
	memcpy(stor, RFIFOP(fd,8), sizeof(struct storage));

	return 0;
}

// 倉庫データ受信
int intif_parse_LoadStorage(int fd) {
	struct storage *stor;
	struct map_session_data *sd;

	stor = account2storage(RFIFOL(fd,4));

	if (stor->storage_status == 1) { // Already open. lets ignore this update
		if (battle_config.error_log)
			printf("intif_parse_LoadStorage: storage received for a client already open.\n");
		return 0;
	}

	if (RFIFOW(fd,2)-8 != sizeof(struct storage)) {
		if (battle_config.error_log)
			printf("intif_parse_LoadStorage: data size error %d %d\n", RFIFOW(fd,2) - 8, sizeof(struct storage));
		return 1;
	}
	sd = map_id2sd(RFIFOL(fd,4));
	if (sd == NULL) {
		if (battle_config.error_log)
			printf("intif_parse_LoadStorage: user not found %d\n", RFIFOL(fd,4));
		return 1;
	}
	if (battle_config.save_log)
		printf("intif_parse_LoadStorage: %d\n", RFIFOL(fd,4));
	memcpy(stor, RFIFOP(fd,8), sizeof(struct storage));
	stor->storage_status = 1;
	sd->state.storage_flag = 0;
	clif_storageitemlist(sd, stor);
	clif_storageequiplist(sd, stor);
	clif_updatestorageamount(sd, stor);

	return 0;
}

// 倉庫データ送信成功
int intif_parse_SaveStorage(int fd) { // need to remove storage's flag of saving
	struct map_session_data *sd;
	int i, account_id;

	account_id = RFIFOL(fd,2);

	if (battle_config.save_log)
		printf("intif_savestorage: done %d.\n", account_id);

	for(i = 0; i < fd_max; i++) {
		if (session[i] && (sd = session[i]->session_data) && sd->state.auth && sd->status.account_id == account_id) {
			if (sd->state.modified_storage_flag == 2) { // if not modified after sending
				sd->state.modified_storage_flag = 0; // 0: not modified, 1: modified, 2: modified and sended to char-server for saving
			}
		}
	}

	return 0;
}

int intif_parse_LoadGuildStorage(int fd) {
	struct guild_storage *gstor;
	struct map_session_data *sd;
	int guild_id = RFIFOL(fd,8);

	if (guild_id > 0) {
		gstor = guild2storage(guild_id);
		if (!gstor) {
			if (battle_config.error_log)
				printf("intif_parse_LoadGuildStorage: error guild_id %d not exist\n", guild_id);
			return 1;
		}
		if (RFIFOW(fd,2) - 12 != sizeof(struct guild_storage)) {
			gstor->storage_status = 0;
			if (battle_config.error_log)
				printf("intif_parse_LoadGuildStorage: data size error %d %d\n", RFIFOW(fd,2) - 12 , sizeof(struct guild_storage));
			return 1;
		}
		sd = map_id2sd(RFIFOL(fd,4));
		if (sd == NULL) {
			if (battle_config.error_log)
				printf("intif_parse_LoadGuildStorage: user not found %d\n", RFIFOL(fd,4));
			return 1;
		}
		if (battle_config.save_log)
			printf("intif_open_guild_storage: %d\n", RFIFOL(fd,4));
		memcpy(gstor, RFIFOP(fd,12), sizeof(struct guild_storage));
		gstor->storage_status = 1;
		gstor->modified_storage_flag = 0;
		sd->state.storage_flag = 1;
		clif_guildstorageitemlist(sd, gstor);
		clif_guildstorageequiplist(sd, gstor);
		clif_updateguildstorageamount(sd, gstor);
	}

	return 0;
}

/*int intif_parse_SaveGuildStorage(int fd) // no real usage for this packet
{
	if (battle_config.save_log) {
		printf("intif_save_guild_storage: done %d %d %d\n", RFIFOL(fd,2), RFIFOL(fd,6), RFIFOB(fd,10));
	}

	return 0;
}*/

// パーティ作成可否
int intif_parse_PartyCreated(int fd) {
	if (battle_config.etc_log)
		printf("intif: party created.\n");
	party_created(RFIFOL(fd,2), RFIFOB(fd,6), RFIFOL(fd,7), RFIFOP(fd,11));

	return 0;
}

// パーティ情報
int intif_parse_PartyInfo(int fd) { // 0x3821 <size>.W (<party_id>.L | <struct party>.?B) - answer of 0x3021 - if size = 8: party doesn't exist - otherwise: party structure
	if (RFIFOW(fd,2) == 8) {
		if (battle_config.error_log)
			printf("intif: party noinfo %d\n", RFIFOL(fd,4));
		party_recv_noinfo(RFIFOL(fd,4));
		return 0;
	}

//	printf("intif: party info %d\n", RFIFOL(fd,4));
	if (RFIFOW(fd,2) != sizeof(struct party) + 4) {
		if (battle_config.error_log)
			printf("intif: party info : data size error %d %d %d\n", RFIFOL(fd,4), RFIFOW(fd,2), sizeof(struct party) + 4);
	}
	party_recv_info((struct party *)RFIFOP(fd,4));

	return 0;
}

// パーティ追加通知
int intif_parse_PartyMemberAdded(int fd)
{
	if (battle_config.etc_log)
		printf("intif: party member added %d %d %d\n", RFIFOL(fd,2), RFIFOL(fd,6), RFIFOB(fd,10));
	party_member_added(RFIFOL(fd,2), RFIFOL(fd,6), RFIFOB(fd,10));

	return 0;
}

// パーティ設定変更通知
int intif_parse_PartyOptionChanged(int fd)
{
	party_optionchanged(RFIFOL(fd,2), RFIFOL(fd,6), RFIFOW(fd,10), RFIFOW(fd,12), RFIFOB(fd,14));

	return 0;
}

// パーティ脱退通知
int intif_parse_PartyMemberLeaved(int fd) {
	if (battle_config.etc_log)
		printf("intif: party member leaved %d %d %s\n", RFIFOL(fd,2), RFIFOL(fd,6), RFIFOP(fd,10));
	party_member_leaved(RFIFOL(fd,2), RFIFOL(fd,6), RFIFOP(fd,10));

	return 0;
}

// パーティ解散通知
int intif_parse_PartyBroken(int fd)
{
	party_broken(RFIFOL(fd,2));

	return 0;
}

// パーティ移動通知
int intif_parse_PartyMove(int fd)
{
//	if (battle_config.etc_log)
//		printf("intif: party move %d %d %s %d %d\n", RFIFOL(fd,2), RFIFOL(fd,6), RFIFOP(fd,10), RFIFOB(fd,26), RFIFOW(fd,27));
	party_recv_movemap(RFIFOL(fd,2), RFIFOL(fd,6), RFIFOP(fd,10), RFIFOB(fd,26), RFIFOW(fd,27));

	return 0;
}

// パーティメッセージ
int intif_parse_PartyMessage(int fd)
{
//	if (battle_config.etc_log)
//		printf("intif_parse_PartyMessage: %s\n", RFIFOP(fd,12));
	party_recv_message(RFIFOL(fd,4), RFIFOL(fd,8), RFIFOP(fd,12), RFIFOW(fd,2) - 12);

	return 0;
}

// ギルド作成可否
int intif_parse_GuildCreated(int fd)
{
	guild_created(RFIFOL(fd,2), RFIFOL(fd,6));

	return 0;
}

// ギルド情報
int intif_parse_GuildInfo(int fd) // 0x3831 <size>.W (<guild_id>.L | <struct guild>.?B) - answer of 0x3031 - if size = 8: guild doesn't exist - otherwise: guild structure
{
	if (RFIFOW(fd,2) == 8) {
		if (battle_config.error_log)
			printf("intif: guild noinfo %d\n", RFIFOL(fd,4));
		guild_recv_noinfo(RFIFOL(fd,4));
		return 0;
	}

//	if (battle_config.etc_log)
//		printf("intif: guild info %d\n", RFIFOL(fd,4));
	if (RFIFOW(fd,2) != sizeof(struct guild) + 4) {
		if (battle_config.error_log)
			printf("intif: guild info : data size error\n %d %d %d", RFIFOL(fd,4), RFIFOW(fd,2), sizeof(struct guild) + 4);
	}
	guild_recv_info((struct guild *)RFIFOP(fd,4));

	return 0;
}

// ギルドメンバ追加通知
int intif_parse_GuildMemberAdded(int fd)
{
	if (battle_config.etc_log)
		printf("intif: guild member added %d %d %d %d\n", RFIFOL(fd,2), RFIFOL(fd,6), RFIFOL(fd,10), RFIFOB(fd,14));
	guild_member_added(RFIFOL(fd,2), RFIFOL(fd,6), RFIFOL(fd,10), RFIFOB(fd,14));

	return 0;
}

// ギルドメンバ脱退/追放通知
int intif_parse_GuildMemberLeaved(int fd)
{
	guild_member_leaved(RFIFOL(fd,2), RFIFOL(fd,6), RFIFOL(fd,10), RFIFOB(fd,14), RFIFOP(fd,55), RFIFOP(fd,15));

	return 0;
}

// ギルドメンバオンライン状態/Lv変更通知
int intif_parse_GuildMemberInfoShort(int fd)
{
	guild_recv_memberinfoshort(RFIFOL(fd,2), RFIFOL(fd,6), RFIFOL(fd,10), RFIFOB(fd,14), RFIFOW(fd,15), RFIFOW(fd,17));

	return 0;
}

// ギルド解散通知
int intif_parse_GuildBroken(int fd)
{
	guild_broken(RFIFOL(fd,2), RFIFOB(fd,6));

	return 0;
}

// ギルド基本情報変更通知
int intif_parse_GuildBasicInfoChanged(int fd)
{
	int type = RFIFOW(fd,8), guild_id = RFIFOL(fd,4);
	void *data = RFIFOP(fd,10);
	struct guild *g = guild_search(guild_id);
	short dw = *((short *)data);
	int dd = *((int *)data);
	if (g == NULL)
		return 0;
	switch(type) {
	case GBI_EXP:        g->exp = dd; break;
	case GBI_GUILDLV:    g->guild_lv = dw; break;
	case GBI_SKILLPOINT: g->skill_point = dd; break;
	}

	return 0;
}

// ギルドメンバ情報変更通知
int intif_parse_GuildMemberInfoChanged(int fd)
{
	int type = RFIFOW(fd,16), guild_id = RFIFOL(fd,4);
	int account_id = RFIFOL(fd,8), char_id = RFIFOL(fd,12);
	void *data = RFIFOP(fd,18);
	struct guild *g = guild_search(guild_id);
	int idx, dd = *((int *)data);

	if (g == NULL)
		return 0;

	idx = guild_getindex(g, account_id, char_id);
	switch(type) {
	case GMI_POSITION:
		g->member[idx].position = dd;
		guild_memberposition_changed(g, idx, dd);
		break;
	case GMI_EXP:
		g->member[idx].exp = dd;
		break;
	}

	return 0;
}

// ギルド役職変更通知
int intif_parse_GuildPosition(int fd)
{
	if (RFIFOW(fd,2) != sizeof(struct guild_position) + 12) {
		if (battle_config.error_log)
			printf("intif: guild info : data size error\n %d %d %d", RFIFOL(fd,4), RFIFOW(fd,2), sizeof(struct guild_position) + 12);
	}
	guild_position_changed(RFIFOL(fd,4), RFIFOL(fd,8), (struct guild_position *)RFIFOP(fd,12));

	return 0;
}

// ギルドスキル割り振り通知
int intif_parse_GuildSkillUp(int fd)
{
	guild_skillupack(RFIFOL(fd,2), RFIFOL(fd,6), RFIFOL(fd,10));

	return 0;
}

// ギルド同盟/敵対通知
int intif_parse_GuildAlliance(int fd)
{
	guild_allianceack(RFIFOL(fd,2), RFIFOL(fd,6), RFIFOL(fd,10), RFIFOL(fd,14), RFIFOB(fd,18), RFIFOP(fd,19), RFIFOP(fd,43));

	return 0;
}

// ギルド告知変更通知
int intif_parse_GuildNotice(int fd)
{
	guild_notice_changed(RFIFOL(fd,2), RFIFOP(fd,6), RFIFOP(fd,66));

	return 0;
}

// ギルドエンブレム変更通知
int intif_parse_GuildEmblem(int fd)
{
	guild_emblem_changed(RFIFOW(fd,2) - 12, RFIFOL(fd,4), RFIFOL(fd,8), RFIFOP(fd,12));

	return 0;
}

// ギルド会話受信
int intif_parse_GuildMessage(int fd)
{
	guild_recv_message(RFIFOL(fd,4), RFIFOL(fd,8), RFIFOP(fd,12), RFIFOW(fd,2) - 12);

	return 0;
}

// ギルド城データ要求返信
int intif_parse_GuildCastleDataLoad(int fd)
{
	return guild_castledataloadack(RFIFOW(fd,2), RFIFOB(fd,4), RFIFOL(fd,5));
}

// ギルド城データ変更通知
int intif_parse_GuildCastleDataSave(int fd)
{
	return guild_castledatasaveack(RFIFOW(fd,2), RFIFOB(fd,4), RFIFOL(fd,5));
}

// ギルド城データ一括受信(初期化時)
int intif_parse_GuildCastleAllDataLoad(int fd)
{
	return guild_castlealldataload(RFIFOW(fd,2), (struct guild_castle *)RFIFOP(fd,4));
}

// pet
int intif_parse_CreatePet(int fd)
{
	pet_get_egg(RFIFOL(fd,2), RFIFOL(fd,7), RFIFOB(fd,6));

	return 0;
}

int intif_parse_RecvPetData(int fd)
{
	struct s_pet p;
	int len = RFIFOW(fd,2);

	if (sizeof(struct s_pet) != len - 9) {
		if (battle_config.etc_log)
			printf("intif: pet data: data size error %d %d\n", sizeof(struct s_pet), len - 9);
	} else {
		memcpy(&p, RFIFOP(fd,9), sizeof(struct s_pet));
		pet_recv_petdata(RFIFOL(fd,4), &p, RFIFOB(fd,8));
	}

	return 0;
}

int intif_parse_SavePetOk(int fd)
{
	if (RFIFOB(fd,6) == 1) {
		if (battle_config.error_log)
			printf("pet data save failure.\n");
	}

	return 0;
}

int intif_parse_DeletePetOk(int fd)
{
	if (RFIFOB(fd,2) == 1) {
		if (battle_config.error_log)
			printf("pet data delete failure.\n");
	}

	return 0;
}

//-----------------------------------------------------------------
// inter serverからの通信
// エラーがあれば0(false)を返すこと
// パケットが処理できれば1,パケット長が足りなければ2を返すこと
int intif_parse(int fd) {
	int packet_len;
	int cmd = RFIFOW(fd,0);

	// パケットのID確認
	if (cmd < 0x3800 || cmd >= 0x3800 + (sizeof(packet_len_table) / sizeof(packet_len_table[0])) ||
	    packet_len_table[cmd - 0x3800] == 0) {
		return 0;
	}
	// パケットの長さ確認
	packet_len = packet_len_table[cmd - 0x3800];
	if (packet_len == -1) {
		if (RFIFOREST(fd) < 4)
			return 2;
		packet_len = RFIFOW(fd,2);
	}
//	if(battle_config.etc_log)
//		printf("intif_parse %d %x %d %d\n", fd, cmd, packet_len, RFIFOREST(fd));
	if (RFIFOREST(fd) < packet_len) {
		return 2;
	}

	// 処理分岐
	switch(cmd) {
	case 0x3800:	clif_GMmessage(NULL, RFIFOP(fd,4), packet_len - 4, 0); break; // 0x3000/0x3800 <packet_len>.w <message>.?B
	case 0x3801:	intif_parse_WisMessage(fd); break; // 0x3001/0x3801 <packet_len>.w (<w_id_0x3801>.L) <sender_GM_level>.B <sender_name>.24B <nick_name>.24B <message>.?B
	case 0x3802:	intif_parse_WisEnd(fd); break; // 0x3802 <sender_name>.24B <flag>.B (flag: 0: success to send wisper, 1: target character is not loged in?, 2: ignored by target)
	case 0x3803:	mapif_parse_WisToGM(RFIFOP(fd,4), RFIFOP(fd,30), RFIFOW(fd,2) - 30, RFIFOW(fd,28)); break; // 0x3003/0x3803 <packet_len>.w <wispname>.24B <min_gm_level>.w <message>.?B
	case 0x3804:	intif_parse_AccountReg(fd); break; // 0x3804 <packet_len>.w <account_id>.L {<account_reg_str>.32B <account_reg_value>.L}.x
	case 0x3805:	intif_parse_AccountRegAck(fd); break; // 0x3805 <account_id>.L
	case 0x3806:	mapif_parse_MainMessage(RFIFOP(fd,4), RFIFOP(fd,28), RFIFOW(fd,2) - 28); break; // 0x3006/0x3806 <packet_len>.w <wispname>.24B <message>.?B
	case 0x3807:	mapif_parse_MessageToGM(RFIFOP(fd,4), RFIFOP(fd,28), RFIFOW(fd,2) - 28); break; // 0x3007/0x3807 <packet_len>.w <wispname>.24B <message>.?B

	case 0x3809:	clif_announce(NULL, RFIFOP(fd,12), RFIFOL(fd,4), RFIFOL(fd,8)); break; // 0x3009/0x3809 <packet_len>.w <color>.L <flag>.L <message>.?B

	case 0x3810:	intif_parse_LoadStorage(fd); break;
	case 0x3811:	intif_parse_SaveStorage(fd); break; // need to remove storage's flag of saving
	case 0x3812:	intif_parse_GetStorage(fd); break;
	case 0x3818:	intif_parse_LoadGuildStorage(fd); break;
//	case 0x3819:	intif_parse_SaveGuildStorage(fd); break; // no real usage for this packet
	case 0x3820:	intif_parse_PartyCreated(fd); break;
	case 0x3821:	intif_parse_PartyInfo(fd); break; // 0x3821 <size>.W (<party_id>.L | <struct party>.?B) - answer of 0x3021 - if size = 8: party doesn't exist - otherwise: party structure
	case 0x3822:	intif_parse_PartyMemberAdded(fd); break;
	case 0x3823:	intif_parse_PartyOptionChanged(fd); break;
	case 0x3824:	intif_parse_PartyMemberLeaved(fd); break;
	case 0x3825:	intif_parse_PartyMove(fd); break;
	case 0x3826:	intif_parse_PartyBroken(fd); break;
	case 0x3827:	intif_parse_PartyMessage(fd); break;
	case 0x3830:	intif_parse_GuildCreated(fd); break;
	case 0x3831:	intif_parse_GuildInfo(fd); break; // 0x3831 <size>.W (<guild_id>.L | <struct guild>.?B) - answer of 0x3031 - if size = 8: guild doesn't exist - otherwise: guild structure
	case 0x3832:	intif_parse_GuildMemberAdded(fd); break;
	case 0x3834:	intif_parse_GuildMemberLeaved(fd); break;
	case 0x3835:	intif_parse_GuildMemberInfoShort(fd); break;
	case 0x3836:	intif_parse_GuildBroken(fd); break;
	case 0x3837:	intif_parse_GuildMessage(fd); break;
	case 0x3839:	intif_parse_GuildBasicInfoChanged(fd); break;
	case 0x383a:	intif_parse_GuildMemberInfoChanged(fd); break;
	case 0x383b:	intif_parse_GuildPosition(fd); break;
	case 0x383c:	intif_parse_GuildSkillUp(fd); break;
	case 0x383d:	intif_parse_GuildAlliance(fd); break;
	case 0x383e:	intif_parse_GuildNotice(fd); break;
	case 0x383f:	intif_parse_GuildEmblem(fd); break;
	case 0x3840:	intif_parse_GuildCastleDataLoad(fd); break;
	case 0x3841:	intif_parse_GuildCastleDataSave(fd); break;
	case 0x3842:	intif_parse_GuildCastleAllDataLoad(fd); break;
	case 0x3880:	intif_parse_CreatePet(fd); break;
	case 0x3881:	intif_parse_RecvPetData(fd); break;
	case 0x3882:	intif_parse_SavePetOk(fd); break;
	case 0x3883:	intif_parse_DeletePetOk(fd); break;
	default:
		if (battle_config.error_log)
			printf("intif_parse : unknown packet %d %x\n", fd, RFIFOW(fd,0));
		return 0;
	}

	// パケット読み飛ばし
	RFIFOSKIP(fd, packet_len);

	return 1;
}
