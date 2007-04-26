// Copyright (c) Freya Development Team - Licensed under GNU GPL
// For more information, see License.txt in the main folder

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h> // for close()
#include <string.h> // strstr
#include <time.h> // time()

#ifndef __WIN32
// Needed for getrlimit() call
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

time_t start_time; // time of start for uptime

#ifndef __WIN32
/* void test_fdlimit(void)
 * This function will check if the NOFILE limit of the system allows to handle the required
 * amount of file descriptors. Note that if Freya is running as root, it will set the limit
 * by itself. If the limit is higher than FD_SETSIZE, no warning will be displayed.
 */
void test_fdlimit(void) {
	struct rlimit limit,limit2;
	/* check if FD_SETSIZE matches the system limit */
	printf("Compiled to handle %d connections !\n", FD_SETSIZE);
	if (getrlimit(RLIMIT_NOFILE, &limit) == 0) {
		if (limit.rlim_cur == RLIM_INFINITY) // unlimited!
			return;
		if (limit.rlim_cur < FD_SETSIZE) {
			if ((limit.rlim_max < FD_SETSIZE) && (limit.rlim_max != RLIM_INFINITY)) {
				// attempt to set the limit to higher values
				limit2.rlim_cur = FD_SETSIZE;
				limit2.rlim_max = FD_SETSIZE;
				if (setrlimit(RLIMIT_NOFILE, &limit2) == 0)
					return; // success!
			}
			if ((limit.rlim_cur < limit.rlim_max) || (limit.rlim_max == RLIM_INFINITY)) {
				limit2.rlim_max = limit.rlim_max;
				limit2.rlim_cur = limit.rlim_max;
				if (setrlimit(RLIMIT_NOFILE,&limit2) != 0) {
					// failed (should not fail however Oo)
					printf("ARGH: Couldn't set the soft NOFILE limit to %d !\n", (int)limit.rlim_max);
				} else {
					limit.rlim_cur = limit.rlim_max;
					if ((limit.rlim_cur >= FD_SETSIZE) || (limit.rlim_cur == RLIM_INFINITY)) // got infinity
						return;
				}
			}
			puts(   " ****");
			puts(  " * " CL_YELLOW "WARNING!!!" CL_RESET);
			printf(" * Your system is limiting Freya from using more than %d fds.\n", (int)limit.rlim_cur);
			puts(  " * In order to fix that on Linux you'll have to edit the file /etc/security/limits.conf");
			puts(  " * and add lines for the user running Freya :");
			printf(" * ");
			printf(" * username<tab>soft<tab>nofile<tab>%d\n", FD_SETSIZE);
			printf(" * username<tab>hard<tab>nofile<tab>%d\n * \n", FD_SETSIZE);
			puts(  " * Make sure pam_limits.so is enabled in /etc/pam.d/su and /etc/pam.d/ssh");
			puts(  " ****");
		}
	}
}
#endif /* __WIN32 */

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

//
// --- signal handler
//
static void sig_proc(int sn)
{
	static int is_called = 0;

	if(is_called++)
		return;

	switch(sn)
	{
#ifdef __WIN32
		case SIGBREAK:
#endif
		case SIGINT:
		case SIGTERM:
			exit(0);
	}

	is_called--;

	return;
}

/*======================================================
 * Server Version Screen
 *------------------------------------------------------
 */
void versionscreen() {
	printf("\n");
#ifdef SVN_REVISION
	if (SVN_REVISION >= 1) { // in case of .svn directories have been deleted
#ifdef TXT_ONLY
		printf(CL_BOLD CL_BG_BLUE "Freya version %d.%d.%d (SVN rev.: %d) (TXT version)" CL_RESET "\n", FREYA_MAJORVERSION, FREYA_MINORVERSION, FREYA_REVISION, (int)SVN_REVISION);
#else
		printf(CL_BOLD CL_BG_BLUE "Freya version %d.%d.%d (SVN rev.: %d) (SQL version)" CL_RESET "\n", FREYA_MAJORVERSION, FREYA_MINORVERSION, FREYA_REVISION, (int)SVN_REVISION);
#endif /* TXT_ONLY */
	} else {
#ifdef TXT_ONLY
		printf(CL_BOLD CL_BG_BLUE "Freya version %d.%d.%d (TXT version)" CL_RESET "\n", FREYA_MAJORVERSION, FREYA_MINORVERSION, FREYA_REVISION);
#else
		printf(CL_BOLD CL_BG_BLUE "Freya version %d.%d.%d (SQL version)" CL_RESET "\n", FREYA_MAJORVERSION, FREYA_MINORVERSION, FREYA_REVISION);
#endif /* TXT_ONLY */
	}
#else
#ifdef TXT_ONLY
	printf(CL_BOLD CL_BG_BLUE "Freya version %d.%d.%d (TXT version)" CL_RESET "\n", FREYA_MAJORVERSION, FREYA_MINORVERSION, FREYA_REVISION);
#else
	printf(CL_BOLD CL_BG_BLUE "Freya version %d.%d.%d (SQL version)" CL_RESET "\n", FREYA_MAJORVERSION, FREYA_MINORVERSION, FREYA_REVISION);
#endif /* TXT_ONLY */
#endif /* SVN_REVISION */
	puts(CL_GREEN "Website/Forum" CL_RESET ": \thttp://www.project-freya.net/");
	puts(CL_GREEN "Download URL" CL_RESET ": \thttp://wiki.ro-freya.net/DownloadFreya");
	puts(CL_GREEN "IRC Channel" CL_RESET ": \tirc://irc.deltaanime.net/freya");
	puts("Open '" CL_DARK_CYAN "readme.html" CL_RESET "' for more information.");
	if(FREYA_STATE)
		puts("This version is not for release.\n");

	return;
}

//
// --- display freya title (move into main() maybe?)
//
void display_title(void)
{
	printf(CL_CLS CL_DARK_WHITE CL_BG_BLUE "          (=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=)" CL_CLL CL_RESET "\n");
	printf(CL_DARK_WHITE CL_BG_BLUE "          (" CL_BOLD "        (c)2004-2007 Freya Team Presents:                " CL_DARK_WHITE CL_BG_BLUE ")" CL_CLL CL_RESET "\n");
	printf(CL_DARK_WHITE CL_BG_BLUE "          (" CL_BOLD "         ___   ___    ___   _  _   __                    " CL_DARK_WHITE CL_BG_BLUE ")" CL_CLL CL_RESET "\n");
	printf(CL_DARK_WHITE CL_BG_BLUE "          (" CL_BOLD "        (  _) (  ,)  (  _) ( \\/ ) (  )                   " CL_DARK_WHITE CL_BG_BLUE ")" CL_CLL CL_RESET "\n");
	printf(CL_DARK_WHITE CL_BG_BLUE "          (" CL_BOLD "        (  _)  )  \\   ) _)  \\  /  /__\\  v%1d.%1d.%1d %3s %5s " CL_DARK_WHITE CL_BG_BLUE ")" CL_CLL CL_RESET "\n", FREYA_MAJORVERSION, FREYA_MINORVERSION, FREYA_REVISION,
#ifdef USE_SQL
	"SQL", FREYA_STATE ? "dev  " : "     ");
#else
	"TXT", FREYA_STATE ? "dev  " : "     ");
#endif /* USE_SQL */
#ifdef SVN_REVISION
	if (SVN_REVISION >= 1) // in case of .svn directories have been deleted
			printf(CL_DARK_WHITE CL_BG_BLUE "          (" CL_BOLD "        (_)   (_)\\_) (___) (__/  (_)(_) SVN rev. %-5d   " CL_DARK_WHITE CL_BG_BLUE ")" CL_CLL CL_RESET "\n", (int)SVN_REVISION);
	else
			printf(CL_DARK_WHITE CL_BG_BLUE "          (" CL_BOLD "        (_)   (_)\\_) (___) (__/  (_)(_)                  " CL_DARK_WHITE CL_BG_BLUE ")" CL_CLL CL_RESET "\n");
#else
		printf(CL_DARK_WHITE CL_BG_BLUE "          (" CL_BOLD "        (_)   (_)\\_) (___) (__/  (_)(_)                  " CL_DARK_WHITE CL_BG_BLUE ")" CL_CLL CL_RESET "\n");
#endif /* SVN_REVISION */
	printf(CL_DARK_WHITE CL_BG_BLUE "          (" CL_BOLD "        http://www.ro-freya.net                          " CL_DARK_WHITE CL_BG_BLUE ")" CL_CLL CL_RESET "\n");
	printf(CL_DARK_WHITE CL_BG_BLUE "          (=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=)" CL_CLL CL_RESET "\n\n"); // reset color
}

int get_version(char flag)
{
	switch(flag)
	{
		case VERSION_FLAG_MAJOR:
			return FREYA_MAJORVERSION;
		case VERSION_FLAG_MINOR:
			return FREYA_MINORVERSION;
		case VERSION_FLAG_REVISION:
			return FREYA_REVISION;
		case VERSION_FLAG_RELEASE:
			return FREYA_STATE;
		case VERSION_FLAG_OFFICIAL:
			return 0;
		case VERSION_FLAG_MOD:
			return 0;
	}
	return 0;
}

//
// --- the main function
//
int main(int argc, char **argv)
{
	int next;

	start_time = time(NULL); // time of start for uptime

	memset(&term_func, 0, sizeof(term_func));
	display_title();

	init_gettick(); // init timer values, and calculate first value to do difference

#ifndef __WIN32
	test_fdlimit();
#endif

	Net_Init();
	do_socket();

	/* signal handling */
#ifndef __WIN32
	signal(SIGUSR1, SIG_IGN);		// Online Flag. No Action Required
	signal(SIGPIPE, SIG_IGN);		// Network Error. No Action Required
#endif /* !__WIN32 */
	signal(SIGINT, sig_proc);		// Process Interrupted
	signal(SIGTERM, sig_proc);		// Termination Request
#ifdef __WIN32
	signal(SIGBREAK, sig_proc);		// Termination Request (Windows)
#endif /* _WIN32 */
	signal(SIGFPE, SIG_DFL);		// Floating Point Error
	signal(SIGILL, SIG_DFL);		// Illegal Instruction
	signal(SIGSEGV, SIG_DFL);		// Segmentation Fault
	signal(SIGABRT, SIG_DFL);		// Abort
#ifndef __WIN32
	signal(SIGBUS, SIG_DFL);		// Improper Memory Handling
	signal(SIGTRAP, SIG_DFL);		// Trace/Breakpoint Trap
#endif /* !__WIN32 */

	do_init(argc, argv);
	runflag = 1;
	while(runflag) {
		gettick_nocache();
		next = do_timer();
		do_sendrecv(next);
		gettick_nocache();
		do_parsepacket();
	}

	return 0;
}
