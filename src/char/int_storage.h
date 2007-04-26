// Copyright (c) Freya Development Team - Licensed under GNU GPL
// For more information, see License.txt in the main folder

#ifndef _INT_STORAGE_H_
#define _INT_STORAGE_H_

void inter_storage_init();
#ifdef TXT_ONLY
void inter_storage_save();
void inter_guild_storage_save();
#endif /* TXT_ONLY */
void inter_storage_delete(const int account_id);
int inter_guild_storage_delete(const int guild_id);
void mapif_get_storage(const int fd, const int account_id);

int inter_storage_parse_frommap(int fd);

#ifdef TXT_ONLY
extern char storage_txt[1024];
extern char guild_storage_txt[1024];
#endif /* TXT_ONLY */

#endif // _INT_STORAGE_H_
