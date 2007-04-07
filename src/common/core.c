// $Id: core.c 575 2006-05-06 06:45:17Z MagicalTux $

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <stdint.h>

#ifndef __WIN32
#include <sys/time.h>
#include <sys/resource.h>
#endif

#include "core.h"
#include "socket.h"
#include "timer.h"
#include "version.h"
#include "console.h"
#include "utils.h"

#ifdef MEMWATCH
#include "memwatch.h"
#endif

static void (*(term_func[MAX_TERMFUNC]))(void);

time_t start_time;

static char *nezumi_logo[] = {
	"      __                         _",
	"   /\\ \\ \\___ _____   _ _ __ ___ (_)",
	"  /  \\/ / _ \\_  / | | | '_ ` _ \\| |",
	" / /\\  /  __// /| |_| | | | | | | |",
	" \\_\\ \\/ \\___/___|\\__,_|_| |_| |_|_|",
	"          http://ro-freya.net",
	NULL
};

#ifndef __WIN32
/* void test_fdlimit(void)
 * This function will check if the NOFILE limit of the system allows to handle the required
 * amount of file descriptors. Note that if Nezumi is running as root, it will set the limit
 * by itself. If the limit is higher than FD_SETSIZE, no warning will be displayed.
 */
void test_fdlimit(void)
{
	struct rlimit limit,limit2;
	fprintf(stderr, CL_WHITE "Info: " CL_RESET "compiled to handle %d connections \n\n", FD_SETSIZE);
	if (getrlimit(RLIMIT_NOFILE, &limit) == 0)
	{
		if (limit.rlim_cur == RLIM_INFINITY)
			return;
		if (limit.rlim_cur < FD_SETSIZE) {
			if ((limit.rlim_max < FD_SETSIZE) && (limit.rlim_max != RLIM_INFINITY))
			{
				limit2.rlim_cur = FD_SETSIZE;
				limit2.rlim_max = FD_SETSIZE;
				if (setrlimit(RLIMIT_NOFILE, &limit2) == 0)
					return;
			}
			if ((limit.rlim_cur < limit.rlim_max) || (limit.rlim_max == RLIM_INFINITY)) {
				limit2.rlim_max = limit.rlim_max;
				limit2.rlim_cur = limit.rlim_max;
				if (setrlimit(RLIMIT_NOFILE,&limit2) != 0)
				{
					printf(CL_YELLOW "Warning: " CL_RESET "could not set the fdsize to '%d' \n", (int)limit.rlim_max);
					return;
				} else {
					limit.rlim_cur = limit.rlim_max;
					if ((limit.rlim_cur >= FD_SETSIZE) || (limit.rlim_cur == RLIM_INFINITY))
						return;
				}
			}
			printf(CL_YELLOW "Warning: " CL_RESET "could not set fdsize to '%d'. limited to '%d' \n", (int)limit.rlim_max, (int)limit.rlim_cur);
		}
	}
	return;
}
#endif

/*======================================
 *	CORE : Set function
 *--------------------------------------
 */
void set_termfunc(void (*termfunc)(void)) {
	int i;

	for(i = 0; i < MAX_TERMFUNC; i++) {
		if (!term_func[i]) {
			term_func[i] = termfunc;
			return;
		}
	}
	puts("Could not allocate a term function! Please increase MAX_TERMFUNC in core.h !!");
}

/*======================================
 *	CORE : Signal Sub Function
 *--------------------------------------
 */
static void sig_proc(int sn) {
//	int i;
	static int is_called = 0;

	if (is_called++)
		return;

	switch(sn) {
	case SIGINT:
	case SIGTERM:
#ifdef __WIN32
	case SIGBREAK:
#endif
/*		for (i = 0; i < MAX_TERMFUNC; i++) {
			if (term_func[i]) (*term_func[i])();
		}
		for(i = 0; i < fd_max; i++) {
			if (!session[i])
				continue;
#ifdef __WIN32
			if (i > 0) { // not console
				shutdown(i, SD_BOTH);
				closesocket(i);
			}
#else
			close(i);
#endif
			delete_session(i);
		}
		term_input_disable();*/
		exit(0);
	}

	is_called--;
}

/*======================================================
 * Server Version Screen
 *------------------------------------------------------
 */
void versionscreen()
{
	printf("\n");
#ifdef SVN_REVISION
	if (SVN_REVISION >= 1)
	{
#ifdef TXT_ONLY
		printf(CL_BOLD CL_BG_BLUE "Nezumi version %d.%d.%d, Athena Mod version %d (SVN rev.: %d) (TXT version)" CL_RESET "\n", ATHENA_MAJOR_VERSION, ATHENA_MINOR_VERSION, ATHENA_REVISION, ATHENA_MOD_VERSION, (int)SVN_REVISION);
#else
		printf(CL_BOLD CL_BG_BLUE "Nezumi version %d.%d.%d, Athena Mod version %d (SVN rev.: %d) (SQL version)" CL_RESET "\n", ATHENA_MAJOR_VERSION, ATHENA_MINOR_VERSION, ATHENA_REVISION, ATHENA_MOD_VERSION, (int)SVN_REVISION);
#endif /* TXT_ONLY */
	} else {
#ifdef TXT_ONLY
		printf(CL_BOLD CL_BG_BLUE "Nezumi version %d.%d.%d, Athena Mod version %d (TXT version)" CL_RESET "\n", ATHENA_MAJOR_VERSION, ATHENA_MINOR_VERSION, ATHENA_REVISION, ATHENA_MOD_VERSION);
#else
		printf(CL_BOLD CL_BG_BLUE "Nezumi version %d.%d.%d, Athena Mod version %d (SQL version)" CL_RESET "\n", ATHENA_MAJOR_VERSION, ATHENA_MINOR_VERSION, ATHENA_REVISION, ATHENA_MOD_VERSION);
#endif /* TXT_ONLY */
	}
#else
#ifdef TXT_ONLY
	printf(CL_BOLD CL_BG_BLUE "Nezumi version %d.%d.%d, Athena Mod version %d (TXT version)" CL_RESET "\n", ATHENA_MAJOR_VERSION, ATHENA_MINOR_VERSION, ATHENA_REVISION, ATHENA_MOD_VERSION);
#else
	printf(CL_BOLD CL_BG_BLUE "Nezumi version %d.%d.%d, Athena Mod version %d (SQL version)" CL_RESET "\n", ATHENA_MAJOR_VERSION, ATHENA_MINOR_VERSION, ATHENA_REVISION, ATHENA_MOD_VERSION);
#endif /* TXT_ONLY */
#endif /* SVN_REVISION */
	puts(CL_GREEN "Website/Forum" CL_RESET ": \thttp://ro-freya.net/");
	puts(CL_GREEN "Download URL" CL_RESET ": \thttp://ookoo.org/svn/freya/");
	puts(CL_GREEN "IRC Channel" CL_RESET ": \tirc://irc.deltaanime.net/freya");
	if (ATHENA_RELEASE_FLAG)
		puts("This version is not for release.\n");

	return;
}

/*======================================
 *	CORE : Display title
 *--------------------------------------
 * Code totally rewritten by MagicalTux
 */
void display_title(void) {
	int title_size=0, logo_width=0, extra_width=0, mid=0, prefix_size=0, total_width=0, x=0;
	char extra_1[32], extra_2[16], format[64], format2[64], top_bot[81];
	while (nezumi_logo[title_size]!=NULL) {
		int tmplen = strlen(nezumi_logo[title_size]);
		if (tmplen>logo_width) logo_width=tmplen;
		title_size++;
	}
	extra_width = strlen(extra_1);
	total_width = logo_width+extra_width;
	if (total_width>78) total_width=78;
	prefix_size = (80-(total_width))/2-4;
	for(int i=0;i<total_width;i++) {
		x=1-x;
		top_bot[i]=(x?'-':'=');
	}
	top_bot[total_width]=0;
	sprintf(format, CL_DARK_WHITE CL_BG_BLUE "%%%ds(" CL_BOLD "%%-%ds%%-%ds" CL_DARK_WHITE CL_BG_BLUE ")" CL_CLL CL_RESET "\n", prefix_size, logo_width, extra_width);
	sprintf(format2, CL_DARK_WHITE CL_BG_BLUE "%%%ds(%%-%ds)" CL_CLL CL_RESET "\n", prefix_size, total_width);
	mid = title_size/2;
	fprintf(stderr, format2, "", top_bot);
	for (int i=0;i<title_size;i++) {
		char *tmpextra="";
		if (i==mid-1) {
			tmpextra=(char *)&extra_1;
		} else if (i==mid) {
			tmpextra=(char *)&extra_2;
		}
		fprintf(stderr, format, "", nezumi_logo[i], tmpextra);
	}
	fprintf(stderr, format2, "", top_bot);
	printf("\n");	// newline to make it look better
#if ATHENA_RELEASE_FLAG == 0
	fprintf(stderr, CL_WHITE "Info: " CL_RESET "Nezumi Version %d.%d.%d %s", ATHENA_MAJOR_VERSION, ATHENA_MINOR_VERSION, ATHENA_REVISION,
#ifdef USE_SQL
		"SQL \n");
#else /* USE_SQL */
		"TXT \n");
#endif /* USE_SQL */
#else /* ATHENA_RELEASE_FLAG == 0 */
	fprintf(stderr, CL_WHITE "Info: " CL_RESET "Nezumi Revision %d %s", (int)SVN_REVISION,
#ifdef USE_SQL
		"SQL \n");
#else /* USE_SQL */
		"TXT \n");
#endif /* USE_SQL */
#endif /* ATHENA_RELEASE_FLAG == 0 */
}

// This is an implementation of signal() using sigaction() for portability.
// (sigaction() is POSIX; signal() is not.)  Taken from Stevens' _Advanced
// Programming in the UNIX Environment_.
//
#ifndef POSIX
#define compat_signal(signo, func) signal(signo, func)
#else
sigfunc *compat_signal(int signo, sigfunc *func) {
	struct sigaction sact, oact;

	sact.sa_handler = func;
	sigemptyset(&sact.sa_mask);
	sact.sa_flags = 0;
#ifdef SA_INTERRUPT
	sact.sa_flags |= SA_INTERRUPT; // SunOS
#endif
	if (sigaction(signo, &sact, &oact) < 0) // The return value from sigaction is zero if it succeeds, and -1 on failure.
		return (SIG_ERR);

	return (oact.sa_handler);
}
#endif

int get_version(char flag) {
	switch(flag) {
	case VERSION_FLAG_MAJOR:
		return ATHENA_MAJOR_VERSION;
	case VERSION_FLAG_MINOR:
		return ATHENA_MINOR_VERSION;
	case VERSION_FLAG_REVISION:
		return ATHENA_REVISION;
	case VERSION_FLAG_RELEASE:
		return ATHENA_RELEASE_FLAG;
	case VERSION_FLAG_OFFICIAL:
		return ATHENA_OFFICIAL_FLAG;
	case VERSION_FLAG_MOD:
		return ATHENA_MOD_VERSION;
	}

	return 0;
}


#ifdef __LEAK_TRACER

struct leak_tracer_entry {
	void *ptr;
	size_t size;
	char *file;
	int line;
	time_t ts;
	void *next;
};

static struct leak_tracer_entry *first_ltrace[LEAK_TRACER_SIZE];

void leak_tracer_end(void)
{
	int i = 0;
    struct leak_tracer_entry *tmp1, *tmp2;

    for(i = 0; i < LEAK_TRACER_SIZE; i++)
    {
        tmp1 = first_ltrace[i];
        while(tmp1 != NULL)
        {
            printf(CL_YELLOW "Warning: " CL_RESET "possible memory leak. %s %ld bytes of memory (allocated at %s.%d.%ld) weren't freed \n", tmp1->size, tmp1->ptr, tmp1->file, tmp1->line, tmp1->ts);
            tmp2 = tmp1->next;
            free(tmp1->ptr);
            free(tmp1->file);
            free(tmp1);
            tmp1 = tmp2;
        }
    }

    memset(first_ltrace, 0, sizeof(void *)*LEAK_TRACER_SIZE);
}

void leak_tracer_init(void) {
	atexit(leak_tracer_end);
	memset(first_ltrace, 0, sizeof(void *)*LEAK_TRACER_SIZE);
}

void leak_tracer_reg(void *ptr, size_t size, char *file, int line) {
	struct leak_tracer_entry *tmp;
	size_t len = strlen(file)+1;
	tmp = calloc(1, sizeof(struct leak_tracer_entry));
	tmp->ptr = ptr;
	tmp->size = size;
	tmp->file = malloc(len);
	memcpy(tmp->file, file, len);
	tmp->line = line;
	tmp->ts = time(NULL) - start_time;
	tmp->next = first_ltrace[(intptr_t)tmp->ptr%LEAK_TRACER_SIZE];
	first_ltrace[(intptr_t)tmp->ptr%LEAK_TRACER_SIZE] = tmp;
}

int leak_tracer_free(void *ptr, char *file, int line) {
	struct leak_tracer_entry *tmp, *prev;
	if (first_ltrace[(intptr_t)ptr%LEAK_TRACER_SIZE] == NULL)
	{
		printf(CL_YELLOW "Warning: " CL_RESET "possible segmentation fault. server tried to free non-allocated block '%p' at %s.%d \n", ptr, file, line);
		return 1;
	}
	tmp = first_ltrace[(intptr_t)ptr%LEAK_TRACER_SIZE];
	prev = NULL;
	while(tmp != NULL) {
		if (tmp->ptr == ptr) {
			if (prev == NULL) {
				first_ltrace[(intptr_t)ptr%LEAK_TRACER_SIZE] = tmp->next;
			} else {
				prev->next = tmp->next;
			}
			free(tmp->file);
			free(tmp);
			return 0;
		}
		prev = tmp;
		tmp = tmp->next;
	}
	printf(CL_YELLOW "Warning: " CL_RESET "possible segmentation fault. server tried to free non-allocated block '%p' at %s.%d \n", ptr, file, line);
	return 2;
}

inline void *internal_malloc(size_t size, char *file, int line) {
	void *ptr = malloc(size);
	leak_tracer_reg(ptr, size, file, line);
	return ptr;
}

inline void *internal_calloc(size_t size, char *file, int line) {
	void *ptr = calloc(1, size);
	leak_tracer_reg(ptr, size, file, line);
	return ptr;
}

inline void *internal_realloc(void *old, size_t size, char *file, int line) {
	void *ptr;
	if (old != NULL) if (leak_tracer_free(old, file, line) != 0) return NULL;
	ptr = realloc(old, size);
	leak_tracer_reg(ptr, size, file, line);
	return ptr;
}

inline void internal_free(void *ptr, char *file, int line) {
	if (leak_tracer_free(ptr, file, line) != 0) return;
	free(ptr);
	return;
}

#endif

/*======================================
 *	CORE : MAINROUTINE
 *--------------------------------------
 */
int main(int argc, char **argv) {
	int next;

	start_time = time(NULL); // time of start for uptime

#ifdef __LEAK_TRACER
	leak_tracer_init();
#endif

	memset(&term_func, 0, sizeof(term_func));
	display_title();

	init_gettick(); // init timer values, and calculate first value to do difference

#ifndef __WIN32
	test_fdlimit();
#endif

	Net_Init();
	do_socket();

#ifndef __WIN32
	if(compat_signal(SIGUSR1, SIG_IGN) == SIG_ERR)
		printf(CL_YELLOW "Warning: " CL_RESET "could not initialize signal handler for 'SIGUSR1' \n");
#ifdef SIGPIPE
	if(compat_signal(SIGPIPE, SIG_IGN) == SIG_ERR)
		printf(CL_YELLOW "Warning: " CL_RESET "could not initialize signal handler for 'SIGPIPE' \n");
#else
	printf(CL_YELLOW "Warning: " CL_RESET "could not recognize signal handler for 'SIGPIPE' \n");
#endif
#endif

	if (compat_signal(SIGTERM, sig_proc) == SIG_ERR)
		printf(CL_YELLOW "Warning: " CL_RESET "could not initialize signal handler for 'SIGTERM' \n");
	if (compat_signal(SIGINT, sig_proc) == SIG_ERR)
		printf(CL_YELLOW "Warning: " CL_RESET "could not initialize signal handler for 'SIGINT' \n");
#ifdef __WIN32
	if (compat_signal(SIGBREAK, sig_proc) == SIG_ERR)
		printf(CL_YELLOW "Warning: " CL_RESET "could not initialize signal handler for 'SIGBREAK' \n");
#endif

	if (compat_signal(SIGFPE, SIG_DFL) == SIG_ERR)
		printf(CL_YELLOW "Warning: " CL_RESET "could not initialize signal handler for 'SIGFPE' \n");
	if (compat_signal(SIGSEGV, SIG_DFL) == SIG_ERR)
		printf(CL_YELLOW "Warning: " CL_RESET "could not initialize signal handler for 'SIGSEGV' \n");
	if (compat_signal(SIGILL, SIG_DFL) == SIG_ERR)
		printf(CL_YELLOW "Warning: " CL_RESET "could not initialize signal handler for 'SIGILL' \n");
	if (compat_signal(SIGABRT, SIG_DFL) == SIG_ERR)
		printf(CL_YELLOW "Warning: " CL_RESET "could not initialize signal handler for 'SIGABRT' \n");
#ifndef __WIN32
	if (compat_signal(SIGBUS, SIG_DFL) == SIG_ERR)
		printf(CL_YELLOW "Warning: " CL_RESET "could not initialize signal handler for 'SIGBUS' \n");
	if (compat_signal(SIGTRAP, SIG_DFL) == SIG_ERR)
		printf(CL_YELLOW "Warning: " CL_RESET "could not initialize signal handler for 'SIGTRAP' \n");
#endif

	do_init(argc, argv);
	runflag = 1;
	while(runflag)
	{
		gettick_nocache();
		next = do_timer();
		do_sendrecv(next);
		gettick_nocache();
		do_parsepacket();
	}

	return 0;
}

