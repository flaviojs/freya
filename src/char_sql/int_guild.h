/*	This file is a part of Freya.
		Freya is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	any later version.
		Freya is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.
		You should have received a copy of the GNU General Public License
	along with Freya; if not, write to the Free Software
	Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA */

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
