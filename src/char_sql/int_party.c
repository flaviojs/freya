// Copyright (c) Freya Development Team - Licensed under GNU GPL
// For more information, see License.txt in the main folder

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "char.h"
#include "../common/socket.h"
#include "../common/malloc.h"

static struct party *partys;
static int party_num, party_max;
static struct party party_tmp;
static int party_newid = 100;

void mapif_party_broken(int party_id);

//--------------------
// Save party to mysql
//--------------------
void inter_party_tosql(int party_id, struct party *p) {
	char t_name[48];
	char t_member[48];
	int party_member, party_online_member;
	int party_exist;
	int i;

	// Check arguments
//	if (p == NULL || party_id == 0 || party_id != p->party_id) {
	if (p->party_id == 0)
		return;

	//printf("(" CL_BOLD "Party %d" CL_RESET "). Request to save.\n", party_id);

	// Searching if party is already in memory (to call this function, it's creation of party or modification. In the last case, party is in memory)
	party_exist = -1;
	for(i = 0; i < party_num; i++)
		if (partys[i].party_id == party_id) { // if found
			party_exist = i;
			break;
		}
	//printf("Check if party #%d exists: %s.\n", party_id, (party_exist == -1) ? "No" : "Yes");

	// if party exists
	if (party_exist != -1) {
		// Check members in the party
		party_member = 0;
		sql_request("SELECT count(1) FROM `%s` WHERE `party_id`='%d'", char_db, party_id);
		if (sql_get_row())
			party_member = sql_get_integer(0);
		/*switch(party_member) {
		case 0:
			printf("No member found in the party #%d.\n", party_id);
			break;
		case 1:
			printf("One member found in the party #%d.\n", party_id);
			break;
		default:
			printf("%d members found in the party #%d.\n", party_member, party_id);
			break;
		}*/

		party_online_member = 0;
		for(i = 0; i < char_num; i++)
			if (char_dat[i].party_id == party_id && online_chars[i] != 0) // to know the number of online players, we check visible and hidden members
				party_online_member++;
		/*switch(party_online_member) {
		case 0:
			printf("No member online in the party #%d.\n", party_id);
			break;
		case 1:
			printf("One member is online in the party #%d.\n", party_id);
			break;
		default:
			printf("%d members are online in the party #%d.\n", party_online_member, party_id);
			break;
		}*/

		// if there is no member and no online member (no member and 1 online, it's probably creation)
		if (party_member == 0 && party_online_member == 0) {
			// Deletion of the party.
			sql_request("DELETE FROM `%s` WHERE `party_id`='%d'", party_db, party_id);
			//printf("No member in party %d, break it.\n", party_id);
			memset(p, 0, sizeof(struct party));
			// remove party from memory
			if (party_exist != (party_num - 1))
				memcpy(&partys[party_exist], &partys[party_num - 1], sizeof(struct party));
			party_num--;
			return;

		// Party exists
		} else {
			// Update party information (members)
			memset(tmp_sql, 0, sizeof(tmp_sql));
			for(i = 0; i < MAX_PARTY; i++) {
				if (p->member[i].account_id > 0 &&
				// Check if it's necessary to save
				    (p->member[i].account_id != partys[party_exist].member[i].account_id ||
				     strcmp(p->member[i].name, partys[party_exist].member[i].name) != 0)) {
					mysql_escape_string(t_member, p->member[i].name, strlen(p->member[i].name));
					if (tmp_sql[0] == '\0')
						tmp_sql_len = sprintf(tmp_sql, "UPDATE `%s` SET `party_id`='%d' WHERE (`account_id` = '%d' AND `name` = '%s')",
						                      char_db, party_id, p->member[i].account_id, t_member);
					else
						tmp_sql_len += sprintf(tmp_sql + tmp_sql_len, " OR (`account_id` = '%d' AND `name` = '%s')",
						                       p->member[i].account_id, t_member);
					// don't check length here, 65536 is enough for all information.
				}
			}
			if (tmp_sql[0] != '\0') {
				//printf("%s\n", tmp_sql);
				sql_request(tmp_sql);
			}
			// Update party information (general informations)
			if (p->exp != partys[party_exist].exp || p->item != partys[party_exist].item) {
				sql_request("UPDATE `%s` SET `exp`='%d', `item`='%d' WHERE `party_id`='%d'", party_db, p->exp, p->item, party_id);
				//printf("Update party #%d information.\n", party_id);
			}

			// Save party in memory
			memcpy(&partys[party_exist], p, sizeof(struct party));
		}

	// new party
	} else {
		// Add new party, if it not exists (or change leader)
		i = 0;
		while(i < MAX_PARTY && ((p->member[i].account_id > 0 && p->member[i].leader == 0) || (p->member[i].account_id < 0)))
			i++;
		if (i < MAX_PARTY) {
			mysql_escape_string(t_name, p->name, strlen(p->name));
			sql_request("INSERT INTO `%s` (`party_id`, `name`, `exp`, `item`, `leader_id`) VALUES ('%d', '%s', '%d', '%d', '%d')",
			            party_db, party_id, t_name, p->exp, p->item, p->member[i].account_id);
			mysql_escape_string(t_member, p->member[i].name, strlen(p->member[i].name));
			sql_request("UPDATE `%s` SET `party_id`='%d' WHERE `account_id`='%d' AND `name`='%s'",
			            char_db, party_id, p->member[i].account_id, t_member);
			//printf("Creation of the party #%d.\n", party_id);
			// Save party in memory
			if (party_num >= party_max) {
				party_max += 256;
				REALLOC(partys, struct party, party_max);
			}
			memcpy(&partys[party_num], p, sizeof(struct party));
			party_num++;
		}
	}

	//printf("Party save success.\n");

	return;
}

//------------------------
// Read a party from mysql
//------------------------
void inter_party_fromsql(int party_id) { // fill party_tmp with informations of the party (or set all values to 0)
	int leader_id;
	int i;
	struct party_member *m;

	memset(&party_tmp, 0, sizeof(struct party));

	if (party_id <= 0)
		return;

	// searching if character is already in memory
	for(i = 0; i < party_num; i++)
		if (partys[i].party_id == party_id) { // if found
			memcpy(&party_tmp, &partys[i], sizeof(struct party));
			printf("(" CL_BOLD "Party #%d" CL_RESET ") Request load - load success [Already in memory].\n", party_id);
			return;
		}

	printf("(" CL_BOLD "Party #%d" CL_RESET ") Request load - read party from sqlDB.\n", party_id);

	// load general informations
	leader_id = 0;
	sql_request("SELECT `name`, `exp`, `item`, `leader_id` FROM `%s` WHERE `party_id`='%d'", party_db, party_id);
	if (sql_get_row()) {
		party_tmp.party_id = party_id;
		strncpy(party_tmp.name, sql_get_string(0), sizeof(party_tmp.name) - 1);
		party_tmp.exp = sql_get_integer(1);
		party_tmp.item = sql_get_integer(2);
		leader_id = sql_get_integer(3);
	} else {
		//printf("Cannot find the party #%d.\n", party_id);
		return;
	}

	// Load members
	sql_request("SELECT `account_id`, `name`, `base_level`, `last_map`, `online` FROM `%s` WHERE `party_id`='%d'", char_db, party_id);
	i = 0;
	while(sql_get_row() && i < MAX_PARTY) {
		m = &party_tmp.member[i];
		m->account_id = sql_get_integer(0);
		if (m->account_id == leader_id)
			m->leader = 1;
		else
			m->leader = 0;
		strncpy(m->name, sql_get_string(1), sizeof(m->name) - 1);
		m->lv = sql_get_integer(2);
		strncpy(m->map, sql_get_string(3), 16); // 17 - NULL
		m->online = (sql_get_integer(4) > 0) ? 1 : 0; // get only online (and not hidden) character
		i++;
	}
	if (i == 0) {
		memset(&party_tmp, 0, sizeof(struct party));
		return;
	}
	/*switch(i) {
	case 0:
		printf("No member found in the party #%d.\n", party_id);
		break;
	case 1:
		printf("One member found in the party #%d.\n", party_id);
		break;
	default:
		printf("%d members found in the party #%d.\n", i, party_id);
		break;
	}*/

	printf("Party #%d load success.\n", party_id);

	// save ine memory the party
	if (party_num >= party_max) {
		party_max += 256;
		REALLOC(partys, struct party, party_max);
	}
	memcpy(&partys[party_num], &party_tmp, sizeof(struct party));
	party_num++;

	return;
}

int party_sync_timer(int tid, unsigned int tick, int id, int data) {
	// here, we remove parties that have no character online
	int i, j;
	int party_id; // speed up

	// searching if a character is in memory. if not, remove party information from memory
	for(i = 0; i < party_num; i++) {
		party_id = partys[i].party_id; // speed up
		for(j = 0; j < char_num; j++)
			if (char_dat[j].party_id == party_id) // if a player is in memory, party must stay in memory too
				break;
		if (j == char_num) {
			if (i != (party_num - 1))
				memcpy(&partys[i], &partys[party_num - 1], sizeof(struct party));
			party_num--;
			i--; // to continue with actual 'new' party
		}
	}

	return 0;
}

//------------------------------
// Init inter-server for parties
//------------------------------
void inter_party_init() {
	int i;
	int total_party;

	total_party = 0;
	sql_request("SELECT count(1) FROM `%s`", party_db);
	if (sql_get_row()) {
		total_party = sql_get_integer(0);
		if (total_party > 0) {
			// Searching for party_newid
			sql_request("SELECT max(`party_id`) FROM `%s`", party_db);
			if (sql_get_row())
				party_newid = sql_get_integer(0) + 1;
		}
	}

	printf("Total parties found: %d.\n", total_party);
	printf("Set party_newid: #%d.\n", party_newid);

	party_num = 0;
	party_max = 256;
	CALLOC(partys, struct party, 256);

	add_timer_func_list(party_sync_timer, "party_sync_timer");
	i = add_timer_interval(gettick_cache + 60 * 1000, party_sync_timer, 0, 0, 60 * 1000); // to check parties in memory and free if not used

	return;
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

//	if (p == NULL || p->party_id == 0)
	if (p->party_id == 0)
		return 1;

	//printf("Party #%d: check empty.\n", p->party_id);
	for(i = 0; i < MAX_PARTY; i++) {
		//printf("Member #%d account: %d.\n", i, p->member[i].account_id);
		if (p->member[i].account_id > 0)
			return 0;
	}
	// If there is no member, then break the party
	mapif_party_broken(p->party_id);
	inter_party_tosql(p->party_id, p);

	return 1;
}


//---------------------------------------------------
// Check if a member is in two parties: not necessary
//---------------------------------------------------
void party_check_conflict(int party_id, int account_id, char *nick) {
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
	char t_name[49]; // 24 * 2 + NULL
	int i;

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

	// search if a party exists with the same name
	mysql_escape_string(t_name, party_name, strlen(party_name));
	sql_request("SELECT count(1) FROM `%s` WHERE `name`='%s'", party_db, t_name);
	if (sql_get_row()) {
		if (sql_get_integer(0) > 0) {
			//printf("int_party: same name party exists [%s].\n", name);
			mapif_party_created(fd, account_id, NULL);
			return;
		}
	}

	memset(&party_tmp, 0, sizeof(struct party));
	party_tmp.party_id = party_newid++;
	strncpy(party_tmp.name, party_name, 24);
	//party_tmp.exp = 0;
	party_tmp.item = item; // <item1>ÉAÉCÉeÉÄé˚èWï˚ñ@ÅB0Ç≈å¬êlï ÅA1Ç≈ÉpÅ[ÉeÉBåˆóL
	party_tmp.itemc = item2; // <item2>ÉAÉCÉeÉÄï™îzï˚ñ@ÅB0Ç≈å¬êlï ÅA1Ç≈ÉpÅ[ÉeÉBÇ…ãœìôï™îz

	party_tmp.member[0].account_id = account_id;
	strncpy(party_tmp.member[0].name, nick, 24);
	strncpy(party_tmp.member[0].map, map, 16); // 17 - NULL
	party_tmp.member[0].leader = 1;
	party_tmp.member[0].online = 1;
	party_tmp.member[0].lv = lv;

	inter_party_tosql(party_tmp.party_id, &party_tmp);

	mapif_party_created(fd, account_id, &party_tmp);
	mapif_party_info(fd, &party_tmp);

	// Update character in memory
	for(i = 0; i < char_num; i++)
		if (char_dat[i].account_id == account_id && memcmp(char_dat[i].name, nick, 24) == 0) {
			char_dat[i].party_id = party_tmp.party_id;
			break;
		}

	return;
}

//----------------------------------
// Request to have party information
//----------------------------------
void mapif_parse_PartyInfo(int fd, int party_id) { // 0x3021 <party_id>.L - ask for party
	inter_party_fromsql(party_id); // fill party_tmp with informations of the party (or set all values to 0)

	if (party_tmp.party_id >= 0)
		mapif_party_info(fd, &party_tmp);
	else
		mapif_party_noinfo(fd, party_id); // party doesn't exist

	return;
}

//---------------------------
// Adding a member in a party
//---------------------------
void mapif_parse_PartyAddMember(int fd, int party_id, int account_id, char *nick, char *map, int lv) {
	int i;

	inter_party_fromsql(party_id); // fill party_tmp with informations of the party (or set all values to 0)

	if (party_tmp.party_id <= 0) { // party doesn't exist
		mapif_party_memberadded(fd, party_id, account_id, 1);
		return;
	}

	for(i = 0; i < MAX_PARTY; i++) {
		if (party_tmp.member[i].account_id == 0) { // must we check if an other character of same account is in the party?
			party_tmp.member[i].account_id = account_id;
			memset(party_tmp.member[i].name, 0, sizeof(party_tmp.member[i].name));
			strncpy(party_tmp.member[i].name, nick, 24);
			memset(party_tmp.member[i].map, 0, sizeof(party_tmp.member[i].map));
			strncpy(party_tmp.member[i].map, map, 16); // 17 - NULL
			party_tmp.member[i].leader = 0;
			party_tmp.member[i].online = 1;
			party_tmp.member[i].lv = lv;
			mapif_party_memberadded(fd, party_id, account_id, 0);
			mapif_party_info(-1, &party_tmp);
			// Update character in memory
			for(i = 0; i < char_num; i++)
				if (char_dat[i].account_id == account_id && memcmp(char_dat[i].name, nick, 24) == 0) {
					char_dat[i].party_id = party_tmp.party_id;
					break;
				}

			if (party_tmp.exp > 0 && !party_check_exp_share(&party_tmp)) {
				party_tmp.exp = 0;
				mapif_party_optionchanged(fd, &party_tmp, 0, 0);
			}
			inter_party_tosql(party_id, &party_tmp);
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
	int flag;

	inter_party_fromsql(party_id); // fill party_tmp with informations of the party (or set all values to 0)

	if (party_tmp.party_id <= 0) // party doesn't exist
		return;

	flag = 0;
	party_tmp.exp = exp;
	if (exp > 0 && !party_check_exp_share(&party_tmp)) {
		flag = 1;
		party_tmp.exp = 0;
	}

	party_tmp.item = item;

	mapif_party_optionchanged(fd, &party_tmp, account_id, flag);
	inter_party_tosql(party_id, &party_tmp);

	return;
}

//------------------------
// A member leaves a party
//------------------------
int mapif_parse_PartyLeave(int fd, int party_id, int account_id) {
	char t_member[49]; // 24 * 2 + NULL
	int i, j;
	int flag;

	inter_party_fromsql(party_id); // fill party_tmp with informations of the party (or set all values to 0)

	if (party_tmp.party_id > 0) {
		for(i = 0; i < MAX_PARTY; i++) {
			if (party_tmp.member[i].account_id == account_id) {
				//printf("member leave the party #%d: account_id = %d.\n", party_id, account_id);
				mapif_party_leaved(party_id, account_id, party_tmp.member[i].name);

				// Update character information
				mysql_escape_string(t_member, party_tmp.member[i].name, strlen(party_tmp.member[i].name));
				sql_request("UPDATE `%s` SET `party_id`='0' WHERE `account_id`='%d' AND `name`='%s'", char_db, account_id, t_member);
				// Update character in memory
				for(j = 0; j < char_num; j++)
					if (char_dat[j].account_id == account_id && strncmp(char_dat[j].name, party_tmp.member[i].name, 24) == 0) {
						char_dat[j].party_id = 0;
						break;
					}
				//printf("Delete member %s from party #%d.\n", p->member[i].name, party_id);

				// if it's leader, remove all other members
				if (party_tmp.member[i].leader == 1) {
					flag = 0;
					for(j = 0; j < MAX_PARTY; j++) {
						if (party_tmp.member[j].account_id > 0 && j != i) {
							mapif_party_leaved(party_id, party_tmp.member[j].account_id, party_tmp.member[j].name);
							flag++;
							//printf("Delete member %s from party #%d (Leader breaks party).\n", party_tmp->member[j].name, party_id);
						}
					}
					if (flag > 0) {
						// Update char information in database
						sql_request("UPDATE `%s` SET `party_id`='0' WHERE `party_id`='%d'", char_db, party_id);
					}
					// Update characters in memory
					for(j = 0; j < char_num; j++)
						if (char_dat[j].party_id == party_id)
							char_dat[j].party_id = 0;
					// Delete the party.
					sql_request("DELETE FROM `%s` WHERE `party_id`='%d'", party_db, party_id);
					//printf("Leader breaks party #%d.\n", party_id);
					memset(&party_tmp, 0, sizeof(struct party));
					// remove party from memory
					for(j = 0; j < party_num; j++)
						if (partys[j].party_id == party_id) { // if found
							if (j != (party_num - 1))
								memcpy(&partys[j], &partys[party_num - 1], sizeof(struct party));
							party_num--;
							break;
						}
				} else {
					memset(&party_tmp.member[i], 0, sizeof(struct party_member));
					// Update party in memory (party_tmp is not pointer on &partys[j])
					for(j = 0; j < party_num; j++) {
						if (partys[j].party_id == party_id) { // if found
							memcpy(&partys[j], &party_tmp, sizeof(struct party));
							break;
						}
					}
				}
				// leader is always the last member, but with deletion of char, that can be different
				if (party_check_empty(&party_tmp) == 0)
					mapif_party_info(-1, &party_tmp); // Sending party information to map-server // Ç‹ÇæêlÇ™Ç¢ÇÈÇÃÇ≈ÉfÅ[É^ëóêM
				break;
			}
		}

	// party not found, suppress characters with this party
	} else {
		sql_request("UPDATE `%s` SET `party_id`='0' WHERE `party_id`='%d'", char_db, party_id);
		// Update character in memory
		for(i = 0; i < char_num; i++)
			if (char_dat[i].party_id == party_id)
				char_dat[i].party_id = 0;
	}

	return 0;
}

//-----------------------
// A member change of map
//-----------------------
void mapif_parse_PartyChangeMap(int fd, int party_id, int account_id, char *map, unsigned char online, int lv) { // online: 0: offline, 1:online
	int i;

	inter_party_fromsql(party_id); // fill party_tmp with informations of the party (or set all values to 0)

	if (party_tmp.party_id <= 0) // party doesn't exist
		return;

	for(i = 0; i < MAX_PARTY; i++) {
		if (party_tmp.member[i].account_id == account_id) { // same account can have more than character in same party. we must check name here too!
			memset(party_tmp.member[i].map, 0, sizeof(party_tmp.member[i].map));
			strncpy(party_tmp.member[i].map, map, 16); // 17 - NULL
			party_tmp.member[i].online = online; // online: 0: offline, 1:online
			party_tmp.member[i].lv = lv;
			mapif_party_membermoved(&party_tmp, i);

			if (party_tmp.exp > 0 && !party_check_exp_share(&party_tmp)) {
				party_tmp.exp = 0;
				mapif_party_optionchanged(fd, &party_tmp, 0, 0);
			}
			break;
		}
	}
	inter_party_tosql(party_id, &party_tmp);

	return;
}

//-----------------
// Break of a party
//-----------------
void mapif_parse_BreakParty(int fd, int party_id) {
	inter_party_fromsql(party_id); // fill party_tmp with informations of the party (or set all values to 0)

	if (party_tmp.party_id <= 0) // party doesn't exist
		return;

	inter_party_tosql(party_id, &party_tmp);

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

