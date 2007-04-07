// $Id: timer.h 699 2006-07-12 22:37:46Z Yor $
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

#define TIMER_FUNC(_x) int _x(int tid, unsigned int tick, intptr_t id, intptr_t data)

struct TimerData {
	unsigned int tick;
	int (*func)(int, unsigned int, intptr_t, intptr_t);
	intptr_t id;
	intptr_t data;
	char type; // 0 or TIMER_ONCE_AUTODEL or TIMER_INTERVAL or TIMER_REMOVE_HEAP
	int interval;
};

// Function prototype declaration

#ifdef __WIN32
int gettimeofday(struct timeval *t, void *dummy);
#endif

void init_gettick(void);
unsigned int gettick_nocache(void);
extern unsigned int gettick_cache;

int add_timer(unsigned int, int (*)(int,unsigned int,intptr_t,intptr_t), intptr_t, intptr_t);
int add_timer_interval(unsigned int, int (*)(int,unsigned int,intptr_t,intptr_t), intptr_t, intptr_t, int);
int delete_timer(int, int (*)(int,unsigned int, intptr_t, intptr_t));

unsigned int addtick_timer(int tid, int added_tick);
struct TimerData *get_timer(int tid);

int do_timer(void);

int add_timer_func_list(int (*)(int,unsigned int,intptr_t,intptr_t), char*);

void timer_final();

#endif	// _TIMER_H_
