// Copyright (c) Freya Development Team - Licensed under GNU GPL
// For more information, see License.txt in the main folder

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "inter.h"
#include "int_pet.h"
#include <mmo.h>
#include "char.h"
#include "../common/socket.h"
#include "../common/db.h"
#include "../common/lock.h"
#include "../common/malloc.h"

#ifdef TXT_ONLY
char pet_txt[1024] = "save/pet.txt";
#endif /* TXT_ONLY */

static struct dbt *pet_db;
static int pet_newid = 100;

//---------------------------------------------------------
int inter_pet_tostr(char *str, struct s_pet *p) {
	int len;

	if (p->hungry < 0)
		p->hungry = 0;
	else if (p->hungry > 100)
		p->hungry = 100;
	if (p->intimate < 0)
		p->intimate = 0;
	else if (p->intimate > 1000)
		p->intimate = 1000;

	len = sprintf(str, "%d,%d,%s\t%d,%d,%d,%d,%d,%d,%d,%d,%d",
	              p->pet_id, p->class, p->name, p->account_id, p->char_id, p->level, p->egg_id,
	              p->equip, p->intimate, p->hungry, p->rename_flag, p->incuvate);

	return 0;
}

static inline int inter_pet_fromstr(char *str,struct s_pet *p) {
	int s;
	int tmp_int[16];
	char tmp_str[256];

	memset(p, 0, sizeof(struct s_pet));

//	printf("sscanf pet main info\n");
	s = sscanf(str, "%d,%d,%[^\t]\t%d,%d,%d,%d,%d,%d,%d,%d,%d", &tmp_int[0], &tmp_int[1], tmp_str, &tmp_int[2],
	           &tmp_int[3], &tmp_int[4], &tmp_int[5], &tmp_int[6], &tmp_int[7], &tmp_int[8], &tmp_int[9], &tmp_int[10]);

	if (s != 12)
		return 1;

	p->pet_id = tmp_int[0];
	p->class = tmp_int[1];
	strncpy(p->name, tmp_str, 24);
	p->account_id = tmp_int[2];
	p->char_id = tmp_int[3];
	p->level = tmp_int[4];
	p->egg_id = tmp_int[5];
	p->equip = tmp_int[6];
	p->intimate = tmp_int[7];
	p->hungry = tmp_int[8];
	p->rename_flag = tmp_int[9];
	p->incuvate = tmp_int[10];

	if (p->hungry < 0)
		p->hungry = 0;
	else if (p->hungry > 100)
		p->hungry = 100;
	if (p->intimate < 0)
		p->intimate = 0;
	else if (p->intimate > 1000)
		p->intimate = 1000;

	return 0;
}

//----------------------------------------------

void inter_pet_init() {
	char line[8192];
	struct s_pet *p;
	FILE *fp;
	int c;

	pet_db = numdb_init();

	if ((fp = fopen(pet_txt, "r")) == NULL)
		return;

	c = 0;
	while(fgets(line, sizeof(line), fp)) { // fgets reads until maximum one less than size and add '\0' -> so, it's not necessary to add -1
		if ((line[0] == '/' && line[1] == '/') || line[0] == '\0' || line[0] == '\n' || line[0] == '\r')
			continue;
		// it's not necessary to remove 'carriage return ('\n' or '\r')
		CALLOC(p, struct s_pet, 1);
		if (inter_pet_fromstr(line, p) == 0 && p->pet_id > 0) {
			if (p->pet_id >= pet_newid)
				pet_newid = p->pet_id + 1;
			numdb_insert(pet_db, (CPU_INT)p->pet_id, p);
		} else {
			printf("int_pet: broken data [%s] line %d.\n", pet_txt, c);
			FREE(p);
		}
		c++;
	}
	fclose(fp);
//	printf("int_pet: %s read done (%d pets)\n", pet_txt, c);

	return;
}

int inter_pet_save_sub(void *key, void *data, va_list ap) {
	char line[8192];
	FILE *fp;

	inter_pet_tostr(line,(struct s_pet *)data);
	fp = va_arg(ap, FILE *);
	fprintf(fp,"%s" RETCODE,line);

	return 0;
}

void inter_pet_save() {
	FILE *fp;
	int lock;

	if ((fp = lock_fopen(pet_txt, &lock)) == NULL) {
		printf("int_pet: cant write [%s] !!! data is lost !!!\n", pet_txt);
		return;
	}

	numdb_foreach(pet_db, inter_pet_save_sub, fp);
	lock_fclose(fp, pet_txt, &lock);
//	printf("int_pet: %s saved.\n", pet_txt);

	return;
}

//----------------------------------
int inter_pet_delete(int pet_id) {
	struct s_pet *p;

	printf("request delete pet #%d.\n", pet_id);

	p = numdb_search(pet_db, (CPU_INT)pet_id);

	if (p == NULL)
		return 1;
	else {
		numdb_erase(pet_db, (CPU_INT)pet_id);
	}

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
	struct s_pet *p;

	CALLOC(p, struct s_pet, 1);
	p->pet_id = pet_newid++;
	strncpy(p->name, pet_name, 24);
	if (incuvate == 1)
		p->account_id = p->char_id = 0;
	else {
		p->account_id = account_id;
		p->char_id = char_id;
	}
	p->class = pet_class;
	p->level = pet_lv;
	p->egg_id = pet_egg_id;
	p->equip = pet_equip;
	p->intimate = intimate;
	p->hungry = hungry;
	p->rename_flag = rename_flag;
	p->incuvate = incuvate;

	if (p->hungry < 0)
		p->hungry = 0;
	else if (p->hungry > 100)
		p->hungry = 100;
	if (p->intimate < 0)
		p->intimate = 0;
	else if (p->intimate > 1000)
		p->intimate = 1000;

	numdb_insert(pet_db, (CPU_INT)p->pet_id,p);

	mapif_pet_created(fd, account_id, p);

	return 0;
}

void mapif_load_pet(int fd,int account_id, int char_id, int pet_id) {
	struct s_pet *p;

	p = numdb_search(pet_db, (CPU_INT)pet_id);

	if (p != NULL) {
		if (p->incuvate == 1) {
			p->account_id = 0;
			p->char_id = 0;
			mapif_pet_info(fd, account_id, p);
		} else if (account_id == p->account_id && char_id == p->char_id)
			mapif_pet_info(fd, account_id, p);
		else
			mapif_pet_noinfo(fd, account_id);
	} else
		mapif_pet_noinfo(fd, account_id);

	return;
}

// process pet save request.
int mapif_save_pet(int fd, int account_id, struct s_pet *data) {
	struct s_pet *p;
	int pet_id;
	int len = RFIFOW(fd, 2);

	if (sizeof(struct s_pet) != len - 8) {
		printf("inter pet: data size error %d %d.\n", (int)sizeof(struct s_pet), len - 8);
	} else {
		pet_id = data->pet_id;
		p = numdb_search(pet_db, (CPU_INT)pet_id);
		if (p == NULL) {
			CALLOC(p, struct s_pet, 1);
			p->pet_id = data->pet_id;
			if (p->pet_id == 0)
				data->pet_id = p->pet_id = pet_newid++;
			numdb_insert(pet_db, (CPU_INT)p->pet_id, p);
		}
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
		memcpy(p, data, sizeof(struct s_pet));
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

