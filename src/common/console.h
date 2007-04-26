// Copyright (c) Freya Development Team - Licensed under GNU GPL
// For more information, see License.txt in the main folder

#ifndef _CONSOLE_H_
#define _CONSOLE_H_

extern unsigned char term_input_status;

#ifdef __WIN32
extern HANDLE hStdin;
extern HANDLE hStdout;
#ifdef HAVE_TERMIOS
struct termios tio_orig;
#endif /* HAVE_TERMIOS */
#endif

void term_input_enable();
void term_input_disable();

#endif // _CONSOLE_H_

