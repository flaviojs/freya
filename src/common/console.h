/* console.h : console control includes
 * $Id: console.h 1 2006-01-13 22:47:50Z MagicalTux $
 */

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

