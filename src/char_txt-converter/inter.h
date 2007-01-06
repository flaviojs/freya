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

#ifndef _INTER_H_
#define _INTER_H_

//add include for DBMS(mysql)
#ifndef __WIN32
# include <mysql.h>
#else
# include <mysql/mysql.h>
#endif

extern char tmp_sql[65535];

extern int db_conv_server_port;
extern char db_conv_server_ip[16];
extern char db_conv_server_id[32];
extern char db_conv_server_pw[32];
extern char db_conv_server_logindb[32];

#endif // _INTER_H_
