// $Id: lutils.h 305 2006-02-23 22:34:31Z DarkRaven $
#ifndef _LUTILS_H_
#define _LUTILS_H_

char login_log_filename[512];
int log_login;
unsigned char log_file_date;

void write_log(char *fmt, ...);
void close_log(void);

#endif // _LUTILS_H_
