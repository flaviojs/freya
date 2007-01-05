/*	This file is a part of Freya.
		Freya is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	any later version.
		Freya is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.
		You should have received a copy of the GNU General Public License
	along with Freya; if not, write to the Free Software
	Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA */

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

