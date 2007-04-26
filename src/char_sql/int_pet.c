// Copyright (c) Freya Development Team - Licensed under GNU GPL
// For more information, see License.txt in the main folder

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "char.h"
#include "../common/malloc.h"

#ifdef TXT_ONLY
char pet_txt[1024] = "save/pet.txt";
#endif /* TXT_ONLY */

struct s_pet *pet_pt;
static int pet_newid = 100;

//---------------------------------------------------------
static void inter_pet_tosql(int pet_id, struct s_pet *p) { // not inline, called too often
	//`pet` (`pet_id`, `class`, `name`, `account_id`, `char_id`, `level`, `egg_id`, `equip`, `intimate`, `hungry`, `rename_flag`, `incuvate`)
	char t_name[49]; // (24 * 2) + NULL

//	printf("request save pet: %d...\n", pet_id);

	if (p->hungry < 0)
		p->hungry = 0;
	else if (p->hungry > 100)
		p->hungry = 100;
	if (p->intimate < 0)
		p->intimate = 0;
	else if (p->intimate > 1000)
		p->intimate = 1000;
	mysql_escape_string(t_name, p->name, strlen(p->name));
	sql_request("REPLACE INTO `%s` (`pet_id`, `class`,`name`,`account_id`,`char_id`,`level`,`egg_id`,`equip`,`intimate`,`hungry`,`rename_flag`,`incuvate`) VALUES ('%d', '%d', '%s', '%d', '%d', '%d', '%d', '%d', '%d', '%d', '%d', '%d')",
	            pet_db, pet_id, p->class, t_name, p->account_id, p->char_id, p->level, p->egg_id,
	            p->equip, p->intimate, p->hungry, p->rename_flag, p->incuvate);

//	printf("pet save success.\n");

	return;
}

void inter_pet_fromsql(int pet_id, struct s_pet *p) {
//	printf("request load pet: %d...\n", pet_id);

	memset(p, 0, sizeof(struct s_pet));

	//`pet` (`pet_id`, `class`,`name`,`account_id`,`char_id`,`level`,`egg_id`,`equip`,`intimate`,`hungry`,`rename_flag`,`incuvate`)
	sql_request("SELECT `class`,`name`,`account_id`,`char_id`,`level`,`egg_id`,`equip`,`intimate`,`hungry`,`rename_flag`,`incuvate` FROM `%s` WHERE `pet_id`='%d'",pet_db, pet_id);
	if (sql_get_row()) {
		p->pet_id = pet_id;
		p->class = sql_get_integer(0);
		strncpy(p->name, sql_get_string(1), 24);
		p->account_id = sql_get_integer(2);
		p->char_id = sql_get_integer(3);
		p->level = sql_get_integer(4);
		p->egg_id = sql_get_integer(5);
		p->equip = sql_get_integer(6);
		p->intimate = sql_get_integer(7);
		p->hungry = sql_get_integer(8);
		p->rename_flag = sql_get_integer(9);
		p->incuvate = sql_get_integer(10);
		if (p->hungry < 0)
			p->hungry = 0;
		else if (p->hungry > 100)
			p->hungry = 100;
		if (p->intimate < 0)
			p->intimate = 0;
		else if (p->intimate > 1000)
			p->intimate = 1000;
//		printf("Pet load success.\n");
//	} else {
//		printf("Pet load failed.\n");
	}

	return;
}

//----------------------------------------------

void inter_pet_init() {
	int i;

	//memory alloc
	printf("interserver pet memory initialize... (%d byte)\n", sizeof(struct s_pet));
	CALLOC(pet_pt, struct s_pet, 1);

	sql_request("SELECT count(1) FROM `%s`", pet_db);
	if (!sql_get_row())
		exit(0);
	i = sql_get_integer(0);
	printf("Total pet datas -> '%d'...\n", i);

	if (i > 0) {
		// set pet_newid
		sql_request("SELECT max(`pet_id`) FROM `%s`", pet_db);
		if (sql_get_row())
			pet_newid = sql_get_integer(0) + 1;
	}

	printf("set pet_newid: %d.\n", pet_newid);

	return;
}

//----------------------------------
int inter_pet_delete(int pet_id) {
	printf("request delete pet #%d.\n", pet_id);

	sql_request("DELETE FROM `%s` WHERE `pet_id`='%d'", pet_db, pet_id);

	return 0;
}

//------------------------------------------------------
int mapif_pet_created(int fd, int account_id, struct s_pet *p) {
	WPACKETW(0) = 0x3880;
	WPACKETL(2) = account_id;
	if (p != NULL) {
		WPACKETB(6) = 0;
		WPACKETL(7) = p->pet_id;
		printf("int_pet: created! %d %s.\n", p->pet_id, p->name);
	} else {
		WPACKETB(6) = 1;
		WPACKETL(7) = 0;
	}
	SENDPACKET(fd, 11);

	return 0;
}

int mapif_pet_info(int fd, int account_id, struct s_pet *p) {
	WPACKETW(0) = 0x3881;
	WPACKETW(2) = sizeof(struct s_pet) + 9;
	WPACKETL(4) = account_id;
	WPACKETB(8) = 0;
	memcpy(WPACKETP(9), p, sizeof(struct s_pet));
	SENDPACKET(fd, sizeof(struct s_pet) + 9);

	return 0;
}

int mapif_pet_noinfo(int fd, int account_id) {
	WPACKETW(0) = 0x3881;
	WPACKETW(2) = sizeof(struct s_pet) + 9;
	WPACKETL(4) = account_id;
	WPACKETB(8) = 1;
	memset(WPACKETP(9), 0, sizeof(struct s_pet));
	SENDPACKET(fd, sizeof(struct s_pet) + 9);

	return 0;
}

int mapif_save_pet_ack(int fd, int account_id, int flag) {
	WPACKETW(0) = 0x3882;
	WPACKETL(2) = account_id;
	WPACKETB(6) = flag;
	SENDPACKET(fd, 7);

	return 0;
}

int mapif_delete_pet_ack(int fd, int flag) {
	WPACKETW(0) = 0x3883;
	WPACKETB(2) = flag;
	SENDPACKET(fd, 3);

	return 0;
}

int mapif_create_pet(int fd, int account_id, int char_id, short pet_class, short pet_lv, short pet_egg_id,
                     short pet_equip, short intimate, short hungry, char rename_flag, char incuvate, char *pet_name) {

	memset(pet_pt, 0, sizeof(struct s_pet));
	pet_pt->pet_id = pet_newid++;
	strncpy(pet_pt->name, pet_name, 24);
	if (incuvate == 1)
		pet_pt->account_id = pet_pt->char_id = 0;
	else {
		pet_pt->account_id = account_id;
		pet_pt->char_id = char_id;
	}
	pet_pt->class = pet_class;
	pet_pt->level = pet_lv;
	pet_pt->egg_id = pet_egg_id;
	pet_pt->equip = pet_equip;
	pet_pt->intimate = intimate;
	pet_pt->hungry = hungry;
	pet_pt->rename_flag = rename_flag;
	pet_pt->incuvate = incuvate;

	if (pet_pt->hungry < 0)
		pet_pt->hungry = 0;
	else if (pet_pt->hungry > 100)
		pet_pt->hungry = 100;
	if (pet_pt->intimate < 0)
		pet_pt->intimate = 0;
	else if (pet_pt->intimate > 1000)
		pet_pt->intimate = 1000;

	inter_pet_tosql(pet_pt->pet_id, pet_pt);

	mapif_pet_created(fd, account_id, pet_pt);

	return 0;
}

void mapif_load_pet(int fd, int account_id, int char_id, int pet_id) {
	memset(pet_pt, 0, sizeof(struct s_pet));

	inter_pet_fromsql(pet_id, pet_pt);

	if (pet_pt != NULL) {
		if (pet_pt->incuvate == 1) {
			pet_pt->account_id = 0;
			pet_pt->char_id = 0;
			mapif_pet_info(fd, account_id, pet_pt);
		} else if (account_id == pet_pt->account_id && char_id == pet_pt->char_id)
			mapif_pet_info(fd, account_id, pet_pt);
		else
			mapif_pet_noinfo(fd, account_id);
	} else
		mapif_pet_noinfo(fd, account_id);

	return;
}

// process pet save request.
int mapif_save_pet(int fd, int account_id, struct s_pet *data) {
	int len = RFIFOW(fd, 2);

	if (sizeof(struct s_pet) != len - 8) {
		printf("inter pet: data size error %d %d.\n", (int)sizeof(struct s_pet), len - 8);
	} else {
		if (data->hungry < 0)
			data->hungry = 0;
		else if (data->hungry > 100)
			data->hungry = 100;
		if (data->intimate < 0)
			data->intimate = 0;
		else if (data->intimate > 1000)
			data->intimate = 1000;
		if (data->incuvate == 1)
			data->account_id = data->char_id = 0;
		inter_pet_tosql(data->pet_id, data);
		mapif_save_pet_ack(fd, account_id, 0);
	}

	return 0;
}

int mapif_delete_pet(int fd, int pet_id) {
	mapif_delete_pet_ack(fd, inter_pet_delete(pet_id));

	return 0;
}

int mapif_parse_CreatePet(int fd) {
	mapif_create_pet(fd, RFIFOL(fd,2), RFIFOL(fd,6), RFIFOW(fd,10), RFIFOW(fd,12), RFIFOW(fd,14), RFIFOW(fd,16), RFIFOL(fd,18),
	                 RFIFOL(fd,20), RFIFOB(fd,22), RFIFOB(fd,23), RFIFOP(fd,24));

	return 0;
}

int mapif_parse_LoadPet(int fd) {
	mapif_load_pet(fd, RFIFOL(fd,2), RFIFOL(fd,6), RFIFOL(fd,10));

	return 0;
}

int mapif_parse_SavePet(int fd) {
	mapif_save_pet(fd, RFIFOL(fd,4), (struct s_pet *)RFIFOP(fd,8));

	return 0;
}

int mapif_parse_DeletePet(int fd) {
	mapif_delete_pet(fd, RFIFOL(fd,2));

	return 0;
}

// map server からの通信
// ・１パケットのみ解析すること
// ・パケット長データはinter.cにセットしておくこと
// ・パケット長チェックや、RFIFOSKIPは呼び出し元で行われるので行ってはならない
// ・エラーなら0(false)、そうでないなら1(true)をかえさなければならない
int inter_pet_parse_frommap(int fd) {
	switch(RFIFOW(fd,0)) {
	case 0x3080: mapif_parse_CreatePet(fd); return 1;
	case 0x3081: mapif_parse_LoadPet(fd); return 1;
	case 0x3082: mapif_parse_SavePet(fd); return 1;
	case 0x3083: mapif_parse_DeletePet(fd); return 1;
	default:
		return 0;
	}

	return 0;
}

