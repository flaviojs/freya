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

#define LADMIN_CONF_NAME	"conf/ladmin_freya.conf"
#define PASSWORDENC		3	// A definition is given when making an encryption password correspond.
							// It is 1 at the time of passwordencrypt.
							// It is made into 2 at the time of passwordencrypt2.
							// When it is made 3, it corresponds to both.

int parse_fromlogin(int fd); // global definition (called by socket.c)

#endif // _LADMIN_H_
