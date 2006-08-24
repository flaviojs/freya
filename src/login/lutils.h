// $Id: lutils.h 464 2005-10-30 22:27:54Z Yor $
#ifndef _LUTILS_H_
#define _LUTILS_H_

char login_log_filename[512];
int log_login;

void write_log(char *fmt, ...);
void close_log(void);

#endif // _LUTILS_H_
