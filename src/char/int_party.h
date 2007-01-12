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

#ifndef _INT_PARTY_H_
#define _INT_PARTY_H_

void inter_party_init();
void inter_party_save();

int inter_party_parse_frommap(int fd);

void inter_party_leave(int party_id, int account_id);

#ifdef TXT_ONLY
extern char party_txt[1024];
#endif /* TXT_ONLY */

#endif // _INT_PARTY_H_
