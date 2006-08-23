// $Id: timer.h 513 2005-11-14 10:37:49Z Yor $
// original : core.h 2003/03/14 11:55:25 Rev 1.4

#ifndef _TIMER_H_
#define _TIMER_H_

#ifdef __WIN32
/* We need winsock lib to have timeval struct - windows is weirdo */
#define __USE_W32_SOCKETS
#include <windows.h>
#endif

#define DIFF_TICK(a,b) ((int)((a)-(b)))

// Struct declaration

struct TimerData {
	unsigned int tick;
	int (*func)(int, unsigned int, int, int);
	int id;
	int data;
	char type; // 0 or TIMER_ONCE_AUTODEL or TIMER_INTERVAL or TIMER_REMOVE_HEAP
	int interval;
};

// Function prototype declaration

#ifdef __WIN32
void gettimeofday(struct timeval *t, void *dummy);
#endif

void init_gettick(void);
unsigned int gettick_nocache(void);
unsigned int gettick(void);
//extern unsigned int gettick_cache;

int add_timer(unsigned int, int (*)(int,unsigned int,int,int), int, int);
int add_timer_interval(unsigned int, int (*)(int,unsigned int,int,int), int, int, int);
int delete_timer(int, int (*)(int,unsigned int, int, int));

unsigned int addtick_timer(int tid, int added_tick);
struct TimerData *get_timer(int tid);

int do_timer(unsigned int tick);

int add_timer_func_list(int (*)(int,unsigned int,int,int), char*);

void timer_final();

#endif	// _TIMER_H_
