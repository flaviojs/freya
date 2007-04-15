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
int gettimeofday(struct timeval *t, void *dummy);
#endif

void init_gettick(void);
unsigned int gettick_nocache(void);
extern unsigned int gettick_cache;

int add_timer(unsigned int, int (*)(int,unsigned int,int,int), int, int);
int add_timer_interval(unsigned int, int (*)(int,unsigned int,int,int), int, int, int);
int delete_timer(int, int (*)(int,unsigned int, int, int));

unsigned int addtick_timer(int tid, int added_tick);
struct TimerData *get_timer(int tid);

int do_timer(void);

int add_timer_func_list(int (*)(int,unsigned int,int,int), char*);

void timer_final();

#endif	// _TIMER_H_
