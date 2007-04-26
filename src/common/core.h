// Copyright (c) Freya Development Team - Licensed under GNU GPL
// For more information, see License.txt in the main folder

#ifndef _CORE_H_
#define _CORE_H_

#include <time.h> // time_t

int runflag;

void do_init(const int, char**);

void set_termfunc(void (*termfunc)(void));

void versionscreen(void);
void display_title(void);

extern time_t start_time; // time of start for uptime

int get_version(char);
#define VERSION_FLAG_MAJOR 0
#define VERSION_FLAG_MINOR 1
#define VERSION_FLAG_REVISION 2
#define VERSION_FLAG_RELEASE 3
#define VERSION_FLAG_OFFICIAL 4
#define VERSION_FLAG_MOD 5

#define MAX_TERMFUNC 10

#endif // _CORE_H_
