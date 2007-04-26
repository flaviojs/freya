// Copyright (c) Freya Development Team - Licensed under GNU GPL
// For more information, see License.txt in the main folder

#ifndef _UTILS_H_
#define _UTILS_H_

#ifndef NULL
#define NULL (void *)0
#endif

extern int config_switch(const char *str);
extern int e_mail_check(const char *email);
extern int remove_control_chars(char *str);
extern inline int int2string(char *buffer, int val);
extern inline int lint2string(char *buffer, long int val);
#ifndef USE_MYSQL
unsigned long mysql_escape_string(char * to, const char * from, unsigned long length);
#endif /* not USE_MYSQL */


// for help with the console colors look here:
// http://www.edoceo.com/liberum/?l=console-output-in-color
// some code explanation (used here):
// \033[2J : clear screen and go up/left (0, 0 position)
// \033[K  : clear line from actual position to end of the line
// \033[0m : reset color parameter
// \033[1m : use bold for font

#ifdef __WIN32
	#define CL_RESET        ""
	#define CL_CLS          ""
	#define CL_CLL          ""

	#define CL_BOLD         ""

	#define CL_BLACK        ""
	#define CL_DARK_RED     ""
	#define CL_DARK_GREEN   ""
	#define CL_DARK_YELLOW  ""
	#define CL_DARK_BLUE    ""
	#define CL_DARK_MAGENTA ""
	#define CL_DARK_CYAN    ""
	#define CL_DARK_WHITE   ""

	#define CL_GRAY         ""
	#define CL_RED          ""
	#define CL_GREEN        ""
	#define CL_YELLOW       ""
	#define CL_BLUE         ""
	#define CL_MAGENTA      ""
	#define CL_CYAN         ""
	#define CL_WHITE        ""

	#define CL_BG_GRAY      ""
	#define CL_BG_RED       ""
	#define CL_BG_GREEN     ""
	#define CL_BG_YELLOW    ""
	#define CL_BG_BLUE      ""
	#define CL_BG_MAGENTA   ""
	#define CL_BG_CYAN      ""
	#define CL_BG_WHITE     ""
#else
	#define CL_RESET        "\033[0;0m"
	#define CL_CLS          "\033[2J"
	#define CL_CLL          "\033[K"

	// font settings
	#define CL_BOLD         "\033[1m"

	#define CL_BLACK        "\033[0;30m" // 0 will reset all value, so re-add background if necessary
	#define CL_DARK_RED     "\033[0;31m" // 0 will reset all value, so re-add background if necessary
	#define CL_DARK_GREEN   "\033[0;32m" // 0 will reset all value, so re-add background if necessary
	#define CL_DARK_YELLOW  "\033[0;33m" // 0 will reset all value, so re-add background if necessary
	#define CL_DARK_BLUE    "\033[0;34m" // 0 will reset all value, so re-add background if necessary
	#define CL_DARK_MAGENTA "\033[0;35m" // 0 will reset all value, so re-add background if necessary
	#define CL_DARK_CYAN    "\033[0;36m" // 0 will reset all value, so re-add background if necessary
	#define CL_DARK_WHITE   "\033[0;37m" // 0 will reset all value, so re-add background if necessary

	#define CL_GRAY         "\033[1;30m"
	#define CL_RED          "\033[1;31m"
	#define CL_GREEN        "\033[1;32m"
	#define CL_YELLOW       "\033[1;33m"
	#define CL_BLUE         "\033[1;34m"
	#define CL_MAGENTA      "\033[1;35m"
	#define CL_CYAN         "\033[1;36m"
	#define CL_WHITE        "\033[1;37m"

	#define CL_BG_GRAY      "\033[40m"
	#define CL_BG_RED       "\033[41m"
	#define CL_BG_GREEN     "\033[42m"
	#define CL_BG_YELLOW    "\033[43m"
	#define CL_BG_BLUE      "\033[44m"
	#define CL_BG_MAGENTA   "\033[45m"
	#define CL_BG_CYAN      "\033[46m"
	#define CL_BG_WHITE     "\033[47m"
#endif

/* replaced all strcasecmp and strncasecmp definition by soft code to be compatible with any system
// fixed strcasecmp and strncasecmp definition
// Most systems: int strcasecmp() / int strncasecmp()
// -------------------------------------------
// Convenience defines for Mac platforms
#if defined(MAC_OS_CLASSIC) || defined(MAC_OS_X)
// Any OS on Mac platform
#define strcasecmp strcmp
#define strncasecmp strncmp
//#endif // MAC_OS_CLASSIC || MAC_OS_X

// Convenience defines for Windows platforms
#elif defined(WINDOWS) || defined(_WIN32)
#if defined(__BORLANDC__)
#define strcasecmp stricmp
#define strncasecmp strncmpi
#else // __BORLANDC__
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
#endif // ! __BORLANDC__
//#endif // WINDOWS || __WIN32

// Convenience defines for MS-DOS platforms
#elif defined(MSDOS)
#if defined(__TURBOC__)
#define strcasecmp stricmp
#define strncasecmp strncmpi
#else // __TURBOC__ */ /* MSC ?
#define strcasecmp stricmp
#define strncasecmp strnicmp
#endif // MSC ?
//#endif // MSDOS

// Convenience defines for OS/2 + icc/gcc platforms
#elif defined(AMIGA) || defined(OS2) || defined(__OS2__) || defined(__EMX__) || defined(__PUREC__)
#define strcasecmp stricmp
#define strncasecmp strnicmp
//#endif // OS2 || __OS2__ || __EMX__

// Convenience defines for other platforms
#elif defined(AMIGA) || defined(__PUREC__)
#define strcasecmp stricmp
#define strncasecmp strnicmp
//#endif // AMIGA || __PUREC__

// Convenience defines with internal command of GCC
#elif defined(__GCC__)
#define strcasecmp __builtin_strcasecmp
#define strncasecmp __builtin_strncasecmp
#endif // __GCC__
*/

#define strcasecmp stringcasecmp
#define strncasecmp stringncasecmp
extern int stringcasecmp(const char *s1, const char *s2);
extern int stringncasecmp(const char *s1, const char *s2, size_t n);

#endif // _UTILS_H_
