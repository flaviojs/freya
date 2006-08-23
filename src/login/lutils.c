// $Id: lutils.c 573 2005-12-02 22:21:10Z Yor $

/*------------------------------------------------------------------------
 Module:        Version 1.0.0 - Yor
 Author:        Freya Team Copyrights (c) 2004-2005
 Project:       Project Freya Account Server
 Creation Date: December 6, 2004
 Modified Date: January 8, 2005
 Description:   Ragnarok Online Server Emulator - General functions of login-server
------------------------------------------------------------------------*/

/* Include of configuration script */
#include <config.h>

/* Include of dependances */
#ifdef __WIN32
#define __USE_W32_SOCKETS
#else
#include <sys/time.h>
#endif

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdarg.h> // va_list

/* Includes of software */
#include <mmo.h>
#include "../common/timer.h" /* gettimeofday for win32 */
#include "lutils.h"
#include "login.h"

/*-----------------
  local variables
-----------------*/
static int log_fp = -1;

/*-----------------
  Close logs file
-----------------*/
void close_log(void) {
	if (log_fp != -1) {
		close(log_fp);
		log_fp = -1;
	}

	return;
}

/*-------------------------------
  Writing function of logs file
-------------------------------*/
void write_log(char *fmt, ...) {
	static int counter = 0;
	va_list ap;
	struct timeval tv;
	time_t now;
	char tmpstr[2048];
	int tmpstr_len;

	/* if we doesn't want to log */
	if (!log_login)
		return;

	/* if not already open, try to open log */
	if (log_fp == -1)
		log_fp = open(login_log_filename, O_WRONLY | O_CREAT | O_APPEND, 0644);

	if (log_fp != -1) {
		if (fmt[0] == '\0') /* jump a line if no message */
			write(log_fp, RETCODE, strlen(RETCODE));
		else {
			va_start(ap, fmt);
			gettimeofday(&tv, NULL);
			now = time(NULL);
			memset(tmpstr, 0, sizeof(tmpstr));
			tmpstr_len = strftime(tmpstr, 20, date_format, localtime(&now)); // 19 + NULL
			tmpstr_len += sprintf(tmpstr + tmpstr_len, ".%03d: ", (int)tv.tv_usec / 1000);
			tmpstr_len += vsprintf(tmpstr + tmpstr_len, fmt, ap);
			write(log_fp, tmpstr, tmpstr_len);
			va_end(ap);
		}
		counter++;
		if ((counter % 100) == 99) {
			close_log(); /* under cygwin or windows, if software is stopped, data are not written in the file -> periodicaly close file */
			counter = 0;
		}
	}

	return;
}
