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

#ifndef _LADMIN_H_
#define _LADMIN_H_

int display_parse_admin; /* 0: no, 1: yes */
int add_to_unlimited_account; /* Give possibility or not to adjust (ladmin command: timeadd) the time of an unlimited account. */
unsigned char ladmin_min_GM_level; /* minimum GM level of a account for listGM/lsGM ladmin command */

int parse_admin(int fd);

#endif // _LADMIN_H_
