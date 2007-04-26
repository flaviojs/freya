// Copyright (c) Freya Development Team - Licensed under GNU GPL
// For more information, see License.txt in the main folder

#ifndef _LUTILS_H_
#define _LUTILS_H_

char login_log_filename[512];
int log_login;
unsigned char log_file_date;

void write_log(char *fmt, ...);
void close_log(void);

#endif // _LUTILS_H_
