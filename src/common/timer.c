// Copyright (c) Freya Development Team - Licensed under GNU GPL
// For more information, see License.txt in the main folder

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#ifdef __WIN32
#define __USE_W32_SOCKETS
#include <windows.h>
#else
#include <sys/socket.h>
#include <sys/time.h>
#endif

#include "timer.h"
#include "malloc.h"
#include "console.h"

#ifdef MEMWATCH
#include "memwatch.h"
#endif

#define TIMER_ONCE_AUTODEL 1
#define TIMER_INTERVAL 2
//#define TIMER_REMOVE_HEAP 16

static struct TimerData* timer_data = NULL;
static int timer_data_max, timer_data_num;
static int* free_timer_list = NULL;
static int free_timer_list_max = 0, free_timer_list_pos = 0;

static int timer_heap_num = 0, timer_heap_max = 0;
static int* timer_heap = NULL;

// for debug
struct timer_func_list {
	int (*func)(int,unsigned int,int,int);
	struct timer_func_list* next;
	char* name;
};
static struct timer_func_list* tfl_root;

#ifdef __WIN32
/* Modified struct timezone to void - we pass NULL anyway */
int gettimeofday(struct timeval *t, void *dummy) {
	DWORD millisec = GetTickCount();

	t->tv_sec = (int) (millisec / 1000);
	t->tv_usec = (millisec % 1000) * 1000;

	return 0;
}
#endif

//
int add_timer_func_list(int (*func)(int,unsigned int,int,int), char* name) {
	struct timer_func_list* tfl;

	CALLOC(tfl, struct timer_func_list, 1);
	CALLOC(tfl->name, char, strlen(name) + 1); // + NULL

	tfl->next = tfl_root;
	tfl->func = func;
	strcpy(tfl->name, name);
	tfl_root = tfl;

	return 0;
}

char* search_timer_func_list(int (*func)(int,unsigned int,int,int)) {
	struct timer_func_list* tfl;

	for(tfl = tfl_root; tfl; tfl = tfl->next) {
		if (func == tfl->func)
			return tfl->name;
	}

	return "???";
}

/*----------------------------
 * Get tick time
 *----------------------------*/
static unsigned int tick_start;
unsigned int gettick_cache;

void init_gettick(void) {
	struct timeval tval;

	gettimeofday(&tval, NULL);
	tick_start = tval.tv_sec; // get fisrt value to have difference and reduce overflow value

	gettick_nocache(); // init gettick_cache

	return;
}

unsigned int gettick_nocache(void) {
	struct timeval tval;

	gettimeofday(&tval, NULL);

	return gettick_cache = ((unsigned int)tval.tv_sec - tick_start) * ((unsigned int)1000) + (unsigned int)(tval.tv_usec / 1000);
}

/*======================================
 * CORE : Timer Heap
 *--------------------------------------
 */
static void push_timer_heap(int idx) {
	int i;
	unsigned int j;
	int min, max, pivot; // for sorting

	// check number of element
	if (timer_heap_num >= timer_heap_max) {
		if (timer_heap_max == 0) {
			timer_heap_max = 256;
			CALLOC(timer_heap, int, 256);
		} else {
			timer_heap_max += 256;
			REALLOC(timer_heap, int, timer_heap_max);
			memset(timer_heap + (timer_heap_max - 256), 0, sizeof(int) * 256);
		}
	}

	// do a sorting from higher to lower
	j = timer_data[idx].tick; // speed up
	// with less than 4 values, it's speeder to use simple loop
	if (timer_heap_num < 4) {
		for(i = timer_heap_num; i > 0; i--)
			if (j < timer_data[timer_heap[i - 1]].tick)
				break;
			else
				timer_heap[i] = timer_heap[i - 1];
		timer_heap[i] = idx;
	// searching by dichotomie
	} else {
		// if lower actual item is higher than new
		if (j < timer_data[timer_heap[timer_heap_num - 1]].tick)
			timer_heap[timer_heap_num] = idx;
		else {
			// searching position
			min = 0;
			max = timer_heap_num - 1;
			while (min < max) {
				pivot = (min + max) / 2;
				if (j < timer_data[timer_heap[pivot]].tick)
					min = pivot + 1;
				else
					max = pivot;
			}
			// move elements - do loop if there are a little number of elements to move
			if (timer_heap_num - min < 5) {
				for(i = timer_heap_num; i > min; i--)
					timer_heap[i] = timer_heap[i - 1];
			// move elements - else use memmove (speeder for a lot of elements)
			} else
				memmove(&timer_heap[min + 1], &timer_heap[min], sizeof(int) * (timer_heap_num - min));
			// save new element
			timer_heap[min] = idx;
		}
	}

	timer_heap_num++;
}

int add_timer(unsigned int tick, int (*func)(int,unsigned int,int,int), int id, int data) {
	struct TimerData* td;
	int i;

	if (free_timer_list_pos) {
		do {
			i = free_timer_list[--free_timer_list_pos];
		} while(i >= timer_data_num && free_timer_list_pos > 0);
	} else
		i = timer_data_num;
	if (i >= timer_data_num)
		for (i = timer_data_num; i < timer_data_max && timer_data[i].type; i++)
			;
	if (i >= timer_data_num && i >= timer_data_max) {
		if (timer_data_max == 0) {
			timer_data_max = 256;
			CALLOC(timer_data, struct TimerData, timer_data_max);
		} else {
			timer_data_max += 256;
			REALLOC(timer_data, struct TimerData, timer_data_max);
			memset(timer_data + (timer_data_max - 256), 0, sizeof(struct TimerData) * 256);
		}
	}
	td = &timer_data[i];
	td->tick = tick;
	td->func = func;
	td->id = id;
	td->data = data;
	td->type = TIMER_ONCE_AUTODEL;
	td->interval = 1000;
	push_timer_heap(i);
	if (i >= timer_data_num)
		timer_data_num = i + 1;

	return i;
}

int add_timer_interval(unsigned int tick, int (*func)(int,unsigned int,int,int), int id, int data, int interval) {
	struct TimerData* td;
	int i;

	if (free_timer_list_pos) {
		do {
			i = free_timer_list[--free_timer_list_pos];
		} while(i >= timer_data_num && free_timer_list_pos > 0);
	} else
		i = timer_data_num;
	if (i >= timer_data_num)
		for (i = timer_data_num; i < timer_data_max && timer_data[i].type; i++)
			;
	if (i >= timer_data_num && i >= timer_data_max) {
		if (timer_data_max == 0) {
			timer_data_max = 256;
			CALLOC(timer_data, struct TimerData, timer_data_max);
		} else {
			timer_data_max += 256;
			REALLOC(timer_data, struct TimerData, timer_data_max);
			memset(timer_data + (timer_data_max - 256), 0, sizeof(struct TimerData) * 256);
		}
	}
	td = &timer_data[i];
	td->tick = tick;
	td->func = func;
	td->id = id;
	td->data = data;
	td->type = TIMER_INTERVAL;
	td->interval = interval;
	push_timer_heap(i);
	if (i >= timer_data_num)
		timer_data_num = i + 1;

	return i;
}

int delete_timer(int id, int (*func)(int, unsigned int, int, int)) {
	if (id < 0 || id >= timer_data_max) {
		printf("delete_timer error : no such timer %d.\n", id);
		return -1;
	}

	if (timer_data[id].func != func) {
		printf("delete_timer error : function dismatch %p(%s) != %p(%s)\n",
		       timer_data[id].func, search_timer_func_list(timer_data[id].func),
		       func, search_timer_func_list(func));
		return -2;
	}

	// そのうち消えるにまかせる
	timer_data[id].func = NULL;
	timer_data[id].type = TIMER_ONCE_AUTODEL;
//	timer_data[id].tick -= 60 * 60 * 1000;

	return 0;
}

unsigned int addtick_timer(int tid, int added_tick) {
	int i, j;

	if (tid < 0 || tid >= timer_data_max) {
		printf("addtick_timer error : no such timer %d.\n", tid);
		return -1;
	}

	// search timer in the list (don't use quick search, it's not used often)
	for(i = 0; i < timer_heap_num; i++)
		if (timer_heap[i] == tid)
			break;

	// if not found
	if (i == timer_heap_num) {
		// we are probably executing the timer (inside the timer function) -> just add the tick
		timer_data[tid].tick += added_tick;
	// if found
	} else {
		// put it at first in the list
		for(j = i; j < timer_heap_num - 1; j++)
			timer_heap[j] = timer_heap[j + 1];
		// remove it from list
		timer_heap_num--;
		// change tick value
		timer_data[tid].tick += added_tick;
		// re-add it in the list
		push_timer_heap(tid);
	}

	return timer_data[tid].tick;
}

struct TimerData* get_timer(int tid) {
	return &timer_data[tid];
}

int do_timer(void) {
	int i, nextmin;

	while(timer_heap_num) {
		i = timer_heap[timer_heap_num - 1]; // next shorter element
		if ((nextmin = DIFF_TICK(timer_data[i].tick, gettick_cache)) > 0 && timer_data[i].func != NULL) { // if we must wait, and timer have a function (not deleted)
			if (nextmin < 50)
				return 50;
			else if (nextmin > 1000) {/* perhaps some parses of clients can create timers before 1 sec */
				if (nextmin > 1800000) // 1800 sec (30 min) // for debug
					printf("Warning: next timer (function: %s) will be done in %d ms.\n", search_timer_func_list(timer_data[i].func), nextmin);
#ifdef __WIN32
				if (term_input_status) // to speed up console echo
					return 300;
				else
#endif /* __WIN32 */
					return 1000;
			} else {
#ifdef __WIN32
				if (term_input_status && nextmin > 300) // to speed up console echo
					return 300;
				else
#endif /* __WIN32 */
					return nextmin;
			}
		}
//		if (timer_heap_num > 0)
		timer_heap_num--; // suppress the actual element from the table
//		timer_data[i].type |= TIMER_REMOVE_HEAP;
		if (timer_data[i].func) {
			if (nextmin < -1000) {
				// 1秒以上の大幅な遅延が発生しているので、
				// timer処理タイミングを現在値とする事で
				// 呼び出し時タイミング(引数のtick)相対で処理してる
				// timer関数の次回処理タイミングを遅らせる
				timer_data[i].func(i, gettick_cache, timer_data[i].id, timer_data[i].data);
			} else {
				timer_data[i].func(i, timer_data[i].tick, timer_data[i].id, timer_data[i].data);
			}
		}
		// this test is not necessary, because it's correctly done in the software (27 may 2005), but if someone change it, it's to protect against a possible futur bug
		if (!timer_data[i].func) // don't set with previous test (else): The function can change the value of the timer.
			timer_data[i].type = TIMER_ONCE_AUTODEL; // no function -> delete the timer (normally, not need to be set, but who knows)
//		if (timer_data[i].type & TIMER_REMOVE_HEAP) {
//			switch(timer_data[i].type & ~TIMER_REMOVE_HEAP) {
			switch(timer_data[i].type) {
			case TIMER_ONCE_AUTODEL:
				timer_data[i].type = 0;
				if (free_timer_list_pos >= free_timer_list_max) {
					if (free_timer_list_max == 0) {
						free_timer_list_max = 256;
						CALLOC(free_timer_list, int, 256);
					} else {
						free_timer_list_max += 256;
						REALLOC(free_timer_list, int, free_timer_list_max);
						memset(free_timer_list + (free_timer_list_max - 256), 0, 256 * sizeof(int));
					}
				}
				free_timer_list[free_timer_list_pos++] = i;
				break;
			case TIMER_INTERVAL:
				if (DIFF_TICK(timer_data[i].tick, gettick_cache) < -1000) {
					timer_data[i].tick = gettick_cache + timer_data[i].interval;
				} else {
					timer_data[i].tick += timer_data[i].interval;
				}
//				timer_data[i].type &= ~TIMER_REMOVE_HEAP;
				push_timer_heap(i);
				break;
			}
//		}
	}

#ifdef __WIN32
	if (term_input_status) // to speed up console echo
		return 300;
	else
#endif /* __WIN32 */
		return 1000;
}

void timer_final() {
	struct timer_func_list *tfl, *tfl2;

	tfl = tfl_root;
	while (tfl) {
		tfl2 = tfl->next; // copy next pointer
		FREE(tfl->name); // free structures
		FREE(tfl);
		tfl = tfl2; // use copied pointer for next cycle
	}

	FREE(timer_data);
	FREE(timer_heap);
	FREE(free_timer_list);

	return;
}

