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
