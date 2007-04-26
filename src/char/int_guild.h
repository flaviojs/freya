// Copyright (c) Freya Development Team - Licensed under GNU GPL
// For more information, see License.txt in the main folder

#ifndef _INT_GUILD_H_
#define _INT_GUILD_H_

void inter_guild_init();
int inter_guild_parse_frommap(int fd);
void inter_guild_mapif_init(int fd);
void inter_guild_leave(int guild_id, int account_id, int char_id);

#ifdef TXT_ONLY
void inter_guild_save();
#endif /* TXT_ONLY */

#ifdef TXT_ONLY
extern char guild_txt[1024];
extern char castle_txt[1024];
#endif /* TXT_ONLY */

#endif // _INT_GUILD_H_
