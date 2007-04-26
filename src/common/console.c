// Copyright (c) Freya Development Team - Licensed under GNU GPL
// For more information, see License.txt in the main folder

#include <config.h>

#include <stdio.h>

#if defined( __WIN32 )
#include <windows.h> /* STD_INPUT_HANDLE, DWORD */
HANDLE hStdin;
HANDLE hStdout;
DWORD savemode;
#endif

#include "console.h"
#include "keycodes.h"

unsigned char term_input_status = 0;

void term_input_enable() {
#ifdef __WIN32
	DWORD retval;
	hStdin = GetStdHandle(STD_INPUT_HANDLE);
	term_input_status = 0;
	if (hStdin == INVALID_HANDLE_VALUE || !GetNumberOfConsoleInputEvents(hStdin, &retval)) {
		//printf("console can not be enabled !\n");
	} else {
		if (GetConsoleMode(hStdin, &savemode) != 0)
			if (SetConsoleMode(hStdin, savemode & ~(ENABLE_MOUSE_INPUT | ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT)) != 0) {
				// try so open a output handle (to display characters)
				hStdout = CreateFile("CONOUT$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0);
				if (hStdout == INVALID_HANDLE_VALUE)
					SetConsoleMode(hStdin, savemode); // restore previous value of the keyboard imput
				else
					term_input_status = 1;
			}
	}
#else /* __WIN32 */

#ifdef HAVE_TERMIOS
	struct termios tio_new;
#if defined(__NetBSD__) || defined(__svr4__) || defined(__CYGWIN__) || defined(__OS2__) || defined(__GLIBC__)
	tcgetattr(0, &tio_orig);
	tcgetattr(0, &tio_new);
#elif defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__bsdi__) || defined(__APPLE__)
	ioctl(0, TIOCGETA, &tio_orig);
	ioctl(0, TIOCGETA, &tio_new);
#else
	ioctl(0, TCGETS, &tio_orig);
	ioctl(0, TCGETS, &tio_new);
#endif
	/* character by character --------------------------*/
//	tio_new.c_lflag &= ~(ICANON|ECHO); // Clear ICANON and ECHO.
//	tio_new.c_cc[VMIN] = 1;
//	tio_new.c_cc[VTIME] = 0;
	/*--------------------------------------------------*/
	/* line by line ------------------------------------*/
//	tio_new.c_lflag |= (ICANON|ECHO);
//	tio_new.c_cc[VMIN] = 0;
//	tio_new.c_cc[VTIME] = 0;
	/*--------------------------------------------------*/
#if defined(__NetBSD__) || defined(__svr4__) || defined(__CYGWIN__) || defined(__OS2__) || defined(__GLIBC__)
//	tcsetattr(0, TCSANOW, &tio_new);
#elif defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__bsdi__) || defined(__APPLE__)
//	ioctl(0, TIOCSETA, &tio_new);
#else
//	ioctl(0, TCSETS, &tio_new);
#endif
#endif /* HAVE_TERMIOS */
	term_input_status = 1;
#endif /* __WIN32 */
}

void term_input_disable() {
	if (!term_input_status)
		return; // already disabled / never enabled

#ifdef __WIN32
	SetConsoleMode(hStdin, savemode);
	CloseHandle(hStdout);
#else /* __WIN32 */
#ifdef HAVE_TERMIOS
#if defined(__NetBSD__) || defined(__svr4__) || defined(__CYGWIN__) || defined(__OS2__) || defined(__GLIBC__)
	tcsetattr(0, TCSANOW, &tio_orig);
#elif defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__bsdi__) || defined(__APPLE__)
	ioctl(0, TIOCSETA, &tio_orig);
#else
	ioctl(0, TCSETS, &tio_orig);
#endif
#endif /* HAVE_TERMIOS */
#endif /* NOT __WIN32 */
	term_input_status = 0;
}
