// Copyright (c) Freya Development Team - Licensed under GNU GPL
// For more information, see License.txt in the main folder

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
