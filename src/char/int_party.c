// Copyright (c) Freya Development Team - Licensed under GNU GPL
// For more information, see License.txt in the main folder

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "inter.h"
#include "int_party.h"
#include <mmo.h>
#include "char.h"
#include "../common/socket.h"
#include "../common/db.h"
#include "../common/lock.h"
#include "../common/malloc.h"

char party_txt[1024] = "save/party.txt";

static struct dbt *party_db;
static int party_newid = 100;

void mapif_party_broken(int party_id);
int party_check_empty(struct party *p);
int mapif_parse_PartyLeave(int fd, int party_id, int account_id);

// ÉpÅ[ÉeÉBÉfÅ[É^ÇÃï∂éöóÒÇ÷ÇÃïœä∑
int inter_party_tostr(char *str, struct party *p) {
	int i, len;

	len = sprintf(str, "%d\t%s\t%d,%d\t", p->party_id, p->name, p->exp, p->item);
	for(i = 0; i < MAX_PARTY; i++) {
		struct party_member *m = &p->member[i];
		len += sprintf(str + len, "%d,%d\t%s\t", m->account_id, m->leader, ((m->account_id > 0) ? m->name : "NoMember"));
	}

	return 0;
}

// ÉpÅ[ÉeÉBÉfÅ[É^ÇÃï∂éöóÒÇ©ÇÁÇÃïœä∑
int inter_party_fromstr(char *str, struct party *p) {
	int i, j;
	int tmp_int[16];
	char tmp_str[256];

	memset(p, 0, sizeof(struct party));

//	printf("sscanf party main info\n");
	if (sscanf(str, "%d\t%[^\t]\t%d,%d\t", &tmp_int[0], tmp_str, &tmp_int[1], &tmp_int[2]) != 4)
		return 1;

	p->party_id = tmp_int[0];
	strncpy(p->name, tmp_str, 24);
	p->exp = tmp_int[1];
	p->item = tmp_int[2];
//	printf("%d [%s] %d %d\n", tmp_int[0], tmp_str[0], tmp_int[1], tmp_int[2]);

	for(j = 0; j < 3 && str != NULL; j++)
		str = strchr(str + 1, '\t');

	for(i = 0; i < MAX_PARTY; i++) {
		struct party_member *m = &p->member[i];
		if (str == NULL)
			return 1;
//		printf("sscanf party member info %d\n", i);

		if (sscanf(str + 1, "%d,%d\t%[^\t]\t", &tmp_int[0], &tmp_int[1], tmp_str) != 3)
			return 1;

		m->account_id = tmp_int[0];
		m->leader = tmp_int[1];
		strncpy(m->name, tmp_str, 24);
//		printf(" %d %d [%s]\n", tmp_int[0], tmp_int[1], tmp_str);

		for(j = 0; j < 2 && str != NULL; j++)
			str = strchr(str + 1, '\t');
	}

	return 0;
}

//------------------------------
// Init inter-server for parties
//------------------------------
void inter_party_init() {
	char line[8192];
	struct party *p;
	FILE *fp;
	int c = 0;
	int i, j;

	party_db = numdb_init();

	if ((fp = fopen(party_txt, "r")) == NULL)
		return;

	while(fgets(line, sizeof(line), fp)) { // fgets reads until maximum one less than size and add '\0' -> so, it's not necessary to add -1
		if ((line[0] == '/' && line[1] == '/') || line[0] == '\0' || line[0] == '\n' || line[0] == '\r')
			continue;
		// it's not necessary to remove 'carriage return ('\n' or '\r')
		j = 0;
		if (sscanf(line, "%d\t%%newid%%\n%n", &i, &j) == 1 && j > 0) {
			if (party_newid < i)
				party_newid = i;
			continue;
		}

		CALLOC(p, struct party, 1);
		if (inter_party_fromstr(line, p) == 0 && p->party_id > 0) {
			if (p->party_id >= party_newid)
				party_newid = p->party_id + 1;
			numdb_insert(party_db, (CPU_INT)p->party_id, p);
			party_check_empty(p);
		} else {
			printf("int_party: broken data [%s] line %d\n", party_txt, c + 1);
			FREE(p);
		}
		c++;
	}
	fclose(fp);
//	printf("int_party: %s read done (%d parties)\n", party_txt, c);

	return;
}

// ÉpÅ[ÉeÉBÅ[ÉfÅ[É^ÇÃÉZÅ[Éuóp
int inter_party_save_sub(void *key, void *data, va_list ap) {
	char line[8192];
	FILE *fp;

	inter_party_tostr(line, (struct party *)data);
	fp = va_arg(ap, FILE *);
	fprintf(fp, "%s" RETCODE, line);

	return 0;
}

// ÉpÅ[ÉeÉBÅ[ÉfÅ[É^ÇÃÉZÅ[Éu
void inter_party_save() {
	FILE *fp;
	int lock;

	if ((fp = lock_fopen(party_txt, &lock)) == NULL) {
		printf("int_party: cant write [%s] !!! data is lost !!!\n", party_txt);
		return;
	}

	numdb_foreach(party_db, inter_party_save_sub, fp);
//	fprintf(fp, "%d\t%%newid%%" RETCODE, party_newid);
	lock_fclose(fp,party_txt, &lock);
//	printf("int_party: %s saved.\n", party_txt);

	return;
}

// ÉpÅ[ÉeÉBñºåüçıóp
int search_partyname_sub(void *key,void *data,va_list ap) {
	struct party *p = (struct party *)data,**dst;
	char *str;

	str = va_arg(ap, char *);
	dst = va_arg(ap, struct party **);
	if (strcasecmp(p->name, str) == 0)
		*dst = p;

	return 0;
}

// ÉpÅ[ÉeÉBñºåüçı
struct party* search_partyname(char *str) {
	struct party *p = NULL;
	numdb_foreach(party_db, search_partyname_sub, str, &p);

	return p;
}

//----------------------------
// Check whether EXP can share
//----------------------------
int party_check_exp_share(struct party *p) {
	int i;
	int maxlv = 0, minlv = 0x7fffffff;
	int lv;

	for(i = 0; i < MAX_PARTY; i++) {
		if (p->member[i].online) {
			lv = p->member[i].lv;
			if (lv < minlv)
				minlv = lv;
			if (maxlv < lv)
				maxlv = lv;
		}
	}

	return (maxlv == 0 || maxlv - minlv <= party_share_level);
}

//-------------------------------
// Is there members in the party?
//-------------------------------
int party_check_empty(struct party *p) {
	int i;

	if (p == NULL || p->party_id == 0)
		return 1;

	//printf("Party #%d: check empty.\n", p->party_id);
	for(i = 0; i < MAX_PARTY; i++) {
		//printf("Member #%d account: %d.\n", i, p->member[i].account_id);
		if (p->member[i].account_id > 0)
			return 0;
	}
	// If there is no member, then break the party
	mapif_party_broken(p->party_id);
	numdb_erase(party_db, (CPU_INT)p->party_id);
	FREE(p);

	return 1;
}

// ÉLÉÉÉâÇÃã£çáÇ™Ç»Ç¢Ç©É`ÉFÉbÉNóp
int party_check_conflict_sub(void *key, void *data, va_list ap) {
	struct party *p = (struct party *)data;
	int party_id, account_id, i;
	char *nick;

	party_id = va_arg(ap, int);

	if (p->party_id == party_id) // ñ{óàÇÃèäëÆÇ»ÇÃÇ≈ñ‚ëËÇ»Çµ
		return 0;

	account_id = va_arg(ap, int);
	nick = va_arg(ap, char *);

	for(i = 0; i < MAX_PARTY; i++) {
		if (p->member[i].account_id == account_id && strcmp(p->member[i].name, nick) == 0) {
			// ï ÇÃÉpÅ[ÉeÉBÇ…ãUÇÃèäëÆÉfÅ[É^Ç™Ç†ÇÈÇÃÇ≈íEëﬁ
			printf("int_party: party conflict! %d %d %d\n", account_id, party_id, p->party_id);
			mapif_parse_PartyLeave(-1, p->party_id, account_id);
		}
	}

	return 0;
}

// ÉLÉÉÉâÇÃã£çáÇ™Ç»Ç¢Ç©É`ÉFÉbÉN
void party_check_conflict(int party_id, int account_id, char *nick) {
	numdb_foreach(party_db, party_check_conflict_sub, party_id, account_id, nick);

	return;
}

//**************************************
// The communication with the map-server
//**************************************

//----------------------
// Party creation answer
//----------------------
void mapif_party_created(int fd, int account_id, struct party *p) {
	WPACKETW(0) = 0x3820;
	WPACKETL(2) = account_id;
	if (p != NULL) {
		WPACKETB(6) = 0; // created
		WPACKETL(7) = p->party_id;
		strncpy(WPACKETP(11), p->name, 24);
		//printf("int_party: created (party #%d, name: '%s')!\n", p->party_id, p->name);
	} else {
		WPACKETB(6) = 1; // not created
		WPACKETL(7) = 0;
		strncpy(WPACKETP(11), "error", 24);
	}
	SENDPACKET(fd, 35);

	return;
}

//----------------------------
// Party information not found
//----------------------------
void mapif_party_noinfo(int fd, int party_id) {
	WPACKETW(0) = 0x3821; // 0x3821 <size>.W (<party_id>.L | <struct party>.?B) - answer of 0x3021 - if size = 8: party doesn't exist - otherwise: party structure
	WPACKETW(2) = 8;
	WPACKETL(4) = party_id;
	SENDPACKET(fd, 8);
	printf("int_party: info not found (party: #%d).\n", party_id);

	return;
}

//-------------------------------
// Sending of party to map-server
//-------------------------------
void mapif_party_info(int fd, struct party *p) {
	WPACKETW(0) = 0x3821; // 0x3821 <size>.W (<party_id>.L | <struct party>.?B) - answer of 0x3021 - if size = 8: party doesn't exist - otherwise: party structure
	WPACKETW(2) = 4 + sizeof(struct party);
	memcpy(WPACKETP(4), p, sizeof(struct party));
	if (fd < 0)
		mapif_sendall(4 + sizeof(struct party));
	else
		mapif_send(fd, 4 + sizeof(struct party));
	//printf("int_party: info (party: #%d, name: '%s').\n", p->party_id, p->name);

	return;
}

//-----------------------------
// Answer to add a party member
//-----------------------------
void mapif_party_memberadded(int fd, int party_id, int account_id, int flag) {
	WPACKETW( 0) = 0x3822;
	WPACKETL( 2) = party_id;
	WPACKETL( 6) = account_id;
	WPACKETB(10) = flag;
	SENDPACKET(fd, 11);

	return;
}

//-------------------------------------------
// Sending party information (after a change)
//-------------------------------------------
void mapif_party_optionchanged(int fd, struct party *p, int account_id, int flag) {
	WPACKETW( 0) = 0x3823;
	WPACKETL( 2) = p->party_id;
	WPACKETL( 6) = account_id;
	WPACKETW(10) = p->exp;
	WPACKETW(12) = p->item;
	WPACKETB(14) = flag;
	if (flag == 0)
		mapif_sendall(15);
	else
		mapif_send(fd, 15);
	//printf("int_party: option changed: party #%d, account:%d, exp:%d, item:%d, flag:%d.\n", p->party_id, account_id, p->exp, p->item, flag);

	return;
}

//------------------------------------------------------
// Sending information about a member that leave a party
//------------------------------------------------------
int mapif_party_leaved(int party_id, int account_id, char *name) {
	WPACKETW(0) = 0x3824;
	WPACKETL(2) = party_id;
	WPACKETL(6) = account_id;
	strncpy(WPACKETP(10), name, 24);
	mapif_sendall(34);
	//printf("int_party: party leaved: party:#%d, account:%d, name: '%s'.\n", party_id, account_id, name);

	return 0;
}

//-----------------------------------------------
// New information about a character (map change)
//-----------------------------------------------
int mapif_party_membermoved(struct party *p, int idx) {
	WPACKETW( 0) = 0x3825;
	WPACKETL( 2) = p->party_id;
	WPACKETL( 6) = p->member[idx].account_id;
	strncpy(WPACKETP(10), p->member[idx].map, 16);
	WPACKETB(26) = p->member[idx].online;
	WPACKETW(27) = p->member[idx].lv;
	mapif_sendall(29);

	return 0;
}

//--------------------
// Deletion of a party
//--------------------
void mapif_party_broken(int party_id) {
	WPACKETW(0) = 0x3826;
	WPACKETL(2) = party_id;
	mapif_sendall(6);
	//printf("int_party: broken party #%d.\n", party_id);

	return;
}

//**************************************
// The communication from the map-server
//**************************************

//--------------------
// Creation of a party
//--------------------
void mapif_parse_CreateParty(int fd, int account_id, char *party_name, char *nick, char *map, int lv, unsigned char item, unsigned char item2) {
	struct party *p;

// check of structure of party_name is done in map-server

/* moved to map-server------------
	// check control chars and del
	for(i = 0; i < 24 && party_name[i]; i++) {
		if (!(party_name[i] & 0xe0) || party_name[i] == 0x7f) {
			printf("int_party: illegal party name [%s]\n", party_name);
			mapif_party_created(fd, account_id, NULL);
			return;
		}
	}--------------------*/

	if ((p = search_partyname(party_name)) != NULL) {
		printf("int_party: same name party exists [%s]\n", party_name);
		mapif_party_created(fd, account_id, NULL);
		return;
	}

	CALLOC(p, struct party, 1);
	p->party_id = party_newid++;
	strncpy(p->name, party_name, 24);
	//p->exp = 0;
	// <item1>ÉAÉCÉeÉÄé˚èWï˚ñ@ÅB0Ç≈å¬êlï ÅA1Ç≈ÉpÅ[ÉeÉBåˆóL
	// <item2>ÉAÉCÉeÉÄï™îzï˚ñ@ÅB0Ç≈å¬êlï ÅA1Ç≈ÉpÅ[ÉeÉBÇ…ãœìôï™îz
	p->item = item; // item 1
	p->itemc = item2; // item 2

	p->member[0].account_id = account_id;
	strncpy(p->member[0].name, nick, 24);
	strncpy(p->member[0].map, map, 16); // 17 - NULL
	p->member[0].leader = 1;
	p->member[0].online = 1;
	p->member[0].lv = lv;

	numdb_insert(party_db, (CPU_INT)p->party_id, p);

	mapif_party_created(fd, account_id, p);
	mapif_party_info(fd, p);

	return;
}

//----------------------------------
// Request to have party information
//----------------------------------
void mapif_parse_PartyInfo(int fd, int party_id) { // 0x3021 <party_id>.L - ask for party
	struct party *p;

	p = numdb_search(party_db, (CPU_INT)party_id);
	if (p != NULL)
		mapif_party_info(fd, p);
	else
		mapif_party_noinfo(fd, party_id);

	return;
}

//---------------------------
// Adding a member in a party
//---------------------------
void mapif_parse_PartyAddMember(int fd, int party_id, int account_id, char *nick, char *map, int lv) {
	struct party *p;
	int i;

	p = numdb_search(party_db, (CPU_INT)party_id);
	if (p == NULL) {
		mapif_party_memberadded(fd, party_id, account_id, 1);
		return;
	}

	for(i = 0; i < MAX_PARTY; i++) {
		if (p->member[i].account_id == 0) { // must we check if an other character of same account is in the party?
			p->member[i].account_id = account_id;
			memset(p->member[i].name, 0, sizeof(p->member[i].name));
			strncpy(p->member[i].name, nick, 24);
			memset(p->member[i].map, 0, sizeof(p->member[i].map));
			strncpy(p->member[i].map, map, 16); // 17 - NULL
			p->member[i].leader = 0;
			p->member[i].online = 1;
			p->member[i].lv = lv;
			mapif_party_memberadded(fd, party_id, account_id, 0);
			mapif_party_info(-1, p);

			if (p->exp > 0 && !party_check_exp_share(p)) {
				p->exp = 0;
				mapif_party_optionchanged(fd, p, 0, 0);
			}
			return;
		}
	}
	mapif_party_memberadded(fd, party_id, account_id, 1);

	return;
}

//--------------------------------------
// Request to change a option in a party
//--------------------------------------
void mapif_parse_PartyChangeOption(int fd, int party_id, int account_id, unsigned short exp, unsigned char item) {
	struct party *p;
	int flag;

	p = numdb_search(party_db, (CPU_INT)party_id);
	if (p == NULL)
		return;

	flag = 0;
	p->exp = exp;
	if (exp > 0 && !party_check_exp_share(p)) {
		flag = 1;
		p->exp = 0;
	}

	p->item = item;

	mapif_party_optionchanged(fd, p, account_id, flag);

	return;
}

//------------------------
// A member leaves a party
//------------------------
int mapif_parse_PartyLeave(int fd, int party_id, int account_id) {
	struct party *p;
	int i;

	p = numdb_search(party_db, (CPU_INT)party_id);
	if (p != NULL) {
		for(i = 0; i < MAX_PARTY; i++) {
			if (p->member[i].account_id == account_id) {
				mapif_party_leaved(party_id, account_id, p->member[i].name);

				memset(&p->member[i], 0, sizeof(struct party_member));
				if (party_check_empty(p) == 0)
					mapif_party_info(-1, p); // Sending party information to map-server // Ç‹ÇæêlÇ™Ç¢ÇÈÇÃÇ≈ÉfÅ[É^ëóêM
				break;
			}
		}
	}

	return 0;
}

//-----------------------
// A member change of map
//-----------------------
void mapif_parse_PartyChangeMap(int fd, int party_id, int account_id, char *map, unsigned char online, int lv) { // online: 0: offline, 1:online
	struct party *p;
	int i;

	p = numdb_search(party_db, (CPU_INT)party_id);
	if (p == NULL)
		return;

	for(i = 0; i < MAX_PARTY; i++) {
		if (p->member[i].account_id == account_id) { // same account can have more than character in same party. we must check name here too!
			memset(p->member[i].map, 0, sizeof(p->member[i].map));
			strncpy(p->member[i].map, map, 16); // 17 - NULL
			p->member[i].online = online; // online: 0: offline, 1:online
			p->member[i].lv = lv;
			mapif_party_membermoved(p, i);

			if (p->exp > 0 && !party_check_exp_share(p)) {
				p->exp = 0;
				mapif_party_optionchanged(fd, p, 0, 0);
			}
			break;
		}
	}

	return;
}

//-----------------
// Break of a party
//-----------------
void mapif_parse_BreakParty(int fd, int party_id) {
	struct party *p;

	p = numdb_search(party_db, (CPU_INT)party_id);
	if (p == NULL)
		return;

	numdb_erase(party_db, (CPU_INT)party_id);
	mapif_party_broken(party_id);

	return;
}

//--------------------------------
// Transmission of a party message
//--------------------------------
void mapif_parse_PartyMessage(int fd, int party_id, int account_id, char *mes, int len) {
	WPACKETW(0) = 0x3827;
	WPACKETW(2) = len + 12;
	WPACKETL(4) = party_id;
	WPACKETL(8) = account_id;
	strncpy(WPACKETP(12), mes, len);
	mapif_sendallwos(fd, len + 12);

	return;
}

//--------------------
// Party check request
//--------------------
void mapif_parse_PartyCheck(int fd, int party_id, int account_id, char *nick) {
	party_check_conflict(party_id, account_id, nick);
	return;
}

//-----------------------------------------------------------------------------------
// Packet parser from of map-server
//   You will found here only packet analyser
//   Packet length check is done before (inter.c) a,d inter.c includes RFIFOSKIP too.
//   Result is: 0 (invalid packet), 1Å(right packet)
//-----------------------------------------------------------------------------------
int inter_party_parse_frommap(int fd) {
	switch(RFIFOW(fd,0)) {
	case 0x3020:
		mapif_parse_CreateParty(fd, RFIFOL(fd,2), RFIFOP(fd,6), RFIFOP(fd,30), RFIFOP(fd,54), RFIFOW(fd,70), RFIFOB(fd,72), RFIFOB(fd,73));
		return 1;
	case 0x3021: // 0x3021 <party_id>.L - ask for party
		mapif_parse_PartyInfo(fd, RFIFOL(fd,2));
		return 1;
	case 0x3022:
		mapif_parse_PartyAddMember(fd, RFIFOL(fd,2), RFIFOL(fd,6), RFIFOP(fd,10), RFIFOP(fd,34), RFIFOW(fd,50));
		return 1;
	case 0x3023:
		mapif_parse_PartyChangeOption(fd, RFIFOL(fd,2), RFIFOL(fd,6), RFIFOW(fd,10), RFIFOW(fd,12));
		return 1;
	case 0x3024:
		mapif_parse_PartyLeave(fd, RFIFOL(fd,2), RFIFOL(fd,6));
		return 1;
	case 0x3025:
		mapif_parse_PartyChangeMap(fd, RFIFOL(fd,2), RFIFOL(fd,6), RFIFOP(fd,10), RFIFOB(fd,26), RFIFOW(fd,27));
		return 1;
	case 0x3026:
		mapif_parse_BreakParty(fd, RFIFOL(fd,2));
		return 1;
	case 0x3027:
		mapif_parse_PartyMessage(fd, RFIFOL(fd,4), RFIFOL(fd,8), RFIFOP(fd,12), RFIFOW(fd,2)-12);
		return 1;
	case 0x3028:
		mapif_parse_PartyCheck(fd, RFIFOL(fd,2), RFIFOL(fd,6), RFIFOP(fd,10));
		return 1;
	default:
		return 0;
	}

	return 0;
}

//---------------------------------------------------------
// Request from map-server about a member that leaves party
//---------------------------------------------------------
void inter_party_leave(int party_id, int account_id) {
	mapif_parse_PartyLeave(-1, party_id, account_id);

	return;
}

