// Copyright (c) Freya Development Team - Licensed under GNU GPL
// For more information, see License.txt in the main folder

#ifndef _INT_PET_H_
#define _INT_PET_H_

void inter_pet_init();
void inter_pet_save();
int inter_pet_delete(int pet_id);
void mapif_load_pet(int fd, int account_id, int char_id, int pet_id);

int inter_pet_parse_frommap(int fd);

#ifdef TXT_ONLY
extern char pet_txt[1024];
#endif /* TXT_ONLY */

#endif // _INT_PET_H_
