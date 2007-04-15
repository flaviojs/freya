/* Copyright (C) 2007 Freya Development Team

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA. */

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
