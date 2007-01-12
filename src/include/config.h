/* Configuration file for Freya
 * $Id: configure 570 2005-12-01 23:15:33Z Yor $
 *
 * This file is generated. Do not edit it.
 */

#ifndef __FREYA_CONFIG_H_
#define __FREYA_CONFIG_H_

/* MySQL support disabled */
#define TXT_ONLY
#undef USE_SQL
#undef USE_MYSQL
#undef USE_SQLITE

/* GetText support disabled */
#undef ENABLE_NLS

/* FreyaPro disabled */
#undef FREYA_PRO

/* Cygwin environement */
#define __CYGWIN

/* reset CYGWIN:s FD_SETSIZE value */
#ifdef FD_SETSIZE
#undef FD_SETSIZE
#endif
#define FD_SETSIZE 1024

/* Win32 support disabled */
#undef __WIN32

/* debian linux (and some other linux) need that */
#ifdef FD_SETSIZE
#undef FD_SETSIZE
#endif
#define FD_SETSIZE 1024

/* enable dynamic addon linking */
#define DYNAMIC_LINKING

/* Debug mode enabled */
#define __DEBUG

/* Experimental Anti-Bot System Disabled */
#undef ANTIBOT_SYSTEM

/* CPU INT detected. This int is equal in term of size to the void * pointer size */
#define CPU_INT int

#define ADDONS_EXT ".dll"
/* configure misc */
#define CONF_PLATFORM "CYGWIN_NT-5.1"
#define CONF_PREFIX "/home/Owner"
#define CONF_DATE "Sun, 07 Jan 2007 18:37:19 -0800"
#define CONF_CONFIGURE "'./configure' '-without-gettext'"
#define CONF_INCLUDES "-I../include"
#define CONF_LIBS "-L../lib -lz -lm"

#endif

