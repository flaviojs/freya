#ifndef _ATNWINSVC_H_
#define _ATNWINSVC_H_

#if defined(_WIN32) && defined(WIN_SERVICE)

#ifndef _WIN32_WINNT
#	define _WIN32_WINNT	0x0500
#endif
#ifndef WINVER
#	define WINVER 0x500
#endif

#include <windows.h>

void atnwinsvc_setname( const char *name, const char *disp, const char *desc );
void atnwinsvc_setlogfile( const char* sout, const char* serr );
void atnwinsvc_logflush();
int atnwinsvc_main( int argc, char **argv );

int atnwinsvc_notify_start();
int atnwinsvc_notify_ready();
int atnwinsvc_notify_stop();
int atnwinsvc_notify_finish();

#else

#define atnwinsvc_setname(a,b,c)
#define atnwinsvc_setlogfile(a,b)
#define atnwinsvc_logflush()
#define atnwinsvc_main(a,b)
#define atnwinsvc_notify_start()
#define atnwinsvc_notify_ready()
#define atnwinsvc_notify_stop()
#define atnwinsvc_notify_finish()

#endif

#endif
