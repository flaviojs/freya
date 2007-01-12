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

#ifndef MODULES_H
#define MODULES_H

#ifdef __WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

#define MOD_VERSION	"1.0"

#ifdef __WIN32
 #define DLLFUNC	_declspec(dllexport)
 #define mod_dlopen(x,y) LoadLibrary(x)
 #define mod_dlclose FreeLibrary
 #define mod_dlsym(x,y,z) z = (void *)GetProcAddress(x,y)
 #define mod_dlerror our_dlerror
#else
 #define mod_dlopen dlopen
 #define mod_dlclose dlclose
 #define mod_dlsym(x,y,z) z = dlsym(x,y)
 #define mod_dlerror dlerror
 #define DLLFUNC
#endif

#ifndef RTLD_NOW
#define RTLD_NOW 0
#endif

/* used for various temporary usages */
void *addon_tmp_pointer;
/* used to contain the common calls table */
void *call_table;
/* used to contain the local calls table */
void *local_table;

typedef struct _Module Module;

/*
 * Module header that every module must include, with the name of
 * mod_header
*/

typedef struct _ModuleHeader {
	char *name;
	char *version;
	char *description;
	char *modversion;
	char addon_target;
} ModuleHeader;

typedef struct {
	int size;
	int module_load;
	Module *handle;
} ModuleInfo;


struct _Module {
	struct _Module *prev, *next;
	ModuleHeader *header; /* The module's header */
	char *filename; /* filename of the module file */
#ifdef __WIN32
	HMODULE dll;	/* Return value of LoadLibrary */
#else
	void *dll;	/* Return value of dlopen */
#endif
	int state; /* state of the addon (loaded or not, etc...) */
};

/* Module function return values */
#define MOD_SUCCESS 0
#define MOD_FAILED -1

/* Addons states */
#define MOD_STATE_EMPTY 0 /* No addon here, empty space */
#define MOD_STATE_DISABLED 1 /* addon currently disabled */
#define MOD_STATE_ENABLED 2 /* addon currently enabled */

#define ADDONS_ALL 0
#define ADDONS_LOGIN 1
#define ADDONS_CHAR 2
#define ADDONS_MAP 3

#ifdef DYNAMIC_LINKING
 #define MOD_HEADER(name) Mod_Header
 #define MOD_TEST(name) Mod_Test
 #define MOD_INIT(name) Mod_Init
 #define MOD_LOAD(name) Mod_Load
 #define MOD_UNLOAD(name) Mod_Unload
#else
 #define MOD_HEADER(name) name##_Header
 #define MOD_TEST(name) name##_Test
 #define MOD_INIT(name) name##_Init
 #define MOD_LOAD(name) name##_Load
 #define MOD_UNLOAD(name) name##_Unload
#endif

int addons_enable_all(void);
int addons_load(char *, char);
int addons_unload(struct _Module *, int);
void addons_unload_all(void);

#endif // MODULES_H

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

#ifdef DYNAMIC_LINKING
 #ifdef __ADDON
  #define EXPORTED_SYMBOL(_symbol,_offset,_var) memcpy(&_var,call_table+(_offset*sizeof(void *)),sizeof(void *))
 #else
  #define EXPORTED_SYMBOL(_symbol,_offset,_var) addon_tmp_pointer=_symbol; memcpy(call_table+(_offset*sizeof(void *)),&addon_tmp_pointer,sizeof(void *))
 #endif
#else
 #define EXPORTED_SYMBOL(_symbol,_offset,_var) _var=_symbol
#endif

#define MFNC_COUNT 7

// common export table

#define MFNC_LOCAL_TABLE(_var) EXPORTED_SYMBOL(local_table, 0, _var)
#define MFNC_DISPLAY_TITLE(_var) EXPORTED_SYMBOL(display_title, 1, _var)
#define MFNC_ADD_TIMER(_var) EXPORTED_SYMBOL(add_timer, 2, _var)
#define MFNC_ADD_TIMER_INTERVAL(_var) EXPORTED_SYMBOL(add_timer_interval, 3, _var)
#define MFNC_DELETE_TIMER(_var) EXPORTED_SYMBOL(delete_timer, 4, _var)
#define MFNC_ADDTICK_TIMER(_var) EXPORTED_SYMBOL(addtick_timer, 5, _var)
#define MFNC_GET_VERSION(_var) EXPORTED_SYMBOL(get_version, 6, _var)

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

#ifndef _CFG_FILES_H_
#define _CFG_FILES_H_

#define CONF_VAR_CHAR 0
#define CONF_VAR_SHORT 1
#define CONF_VAR_INT 2
#define CONF_VAR_STRING 3
#define CONF_VAR_CALLBACK 4

#define CONF_OK 0
#define CONF_ERROR 1

struct conf_entry {
	char *param_name; // 4 bytes + malloc'd string
	void *save_pointer; // 4 bytes
	int min_limit,max_limit; // 8 bytes : min. and max limit of the value (zero = not used)
	char type; // 1 byte
	// 3 bytes lost
}

#endif // _CFG_FILES_H_
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

#ifndef _CORE_H_
#define _CORE_H_

#include <time.h> // time_t

int runflag;

void do_init(const int, char**);

void set_termfunc(void (*termfunc)(void));

void versionscreen(void);
void display_title(void);

extern time_t start_time; // time of start for uptime

int get_version(char);
#define VERSION_FLAG_MAJOR 0
#define VERSION_FLAG_MINOR 1
#define VERSION_FLAG_REVISION 2
#define VERSION_FLAG_RELEASE 3
#define VERSION_FLAG_OFFICIAL 4
#define VERSION_FLAG_MOD 5

#define MAX_TERMFUNC 10

#endif // _CORE_H_
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

#ifndef _DB_H_
#define _DB_H_

#include <stdarg.h>
#include <config.h>

#define HASH_SIZE (256 + 27)

#define RED 0
#define BLACK 1

struct dbn {
	struct dbn *parent, *left, *right;
	int color;
	void *key;
	void *data;
	int deleted; // 削除済みフラグ(db_foreach)
};

struct dbt {
	int (*cmp)(struct dbt*, void*, void*);
	unsigned CPU_INT (*hash)(struct dbt*, void*);
	// which 1 - key,   2 - data,  3 - both
	void (*release)(struct dbn*, int which);
	int maxlen;
	struct dbn *ht[HASH_SIZE];
	int item_count; // vf?
	const char* alloc_file; // DB?t@C
	int alloc_line; // DB?s
	// db_foreach 内部でdb_erase される対策として、
	// db_foreach が終わるまでロックすることにする
	struct db_free {
		struct dbn *z;
		struct dbn **root;
	} *free_list;
	int free_count;
	int free_max;
	int free_lock;
};

#define strdb_search(t,k)   db_search((t), (void*)(k))
#define strdb_insert(t,k,d) db_insert((t), (void*)(k), (void*)(d))
#define strdb_erase(t,k)    db_erase((t), (void*)(k))
#define strdb_foreach       db_foreach
#define strdb_final         db_final
#define numdb_search(t,k)   db_search((t), (void*)(k))
#define numdb_insert(t,k,d) db_insert((t), (void*)(k), (void*)(d))
#define numdb_erase(t,k)    db_erase((t), (void*)(k))
#define numdb_foreach       db_foreach
#define numdb_final         db_final

#define strdb_init(a)       strdb_init_(a,__FILE__, __LINE__)
#define numdb_init()        numdb_init_(__FILE__, __LINE__)

struct dbt* strdb_init_(int maxlen, const char *file, int line);
struct dbt* numdb_init_(const char *file, int line);

void* db_search(struct dbt *table, void* key);
struct dbn* db_insert(struct dbt *table, void* key, void* data);
void* db_erase(struct dbt *table, void* key);
void db_foreach(struct dbt*, int(*)(void*, void*, va_list), ...);
void db_final(struct dbt*, int(*)(void*, void*, va_list), ...);

#endif // _DB_H_
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

#include <config.h>

#ifndef _SQL_H_
#define _SQL_H_

#ifdef USE_MYSQL

#include "utils.h"
#ifndef __WIN32
# include <mysql.h>
#else
# include <mysql/mysql.h>
#endif

#define MAX_SQL_BUFFER 65535 * 2 // memo is limited to 60000, x2 for escape code


// MySQL variables
unsigned int db_mysql_server_port;
char db_mysql_server_ip[1024]; /* configuration line are read for 1024 char */
char db_mysql_server_id[32];
char db_mysql_server_pw[32];
char db_mysql_server_db[32];

// Our functions
int sql_request(const char *format, ...);
void sql_init();
void sql_close(void);
int sql_get_row(void);
char *sql_get_string(int num_col);
int sql_get_integer(int num_col);
unsigned long sql_num_rows(void);
#endif /* USE_MYSQL */

#endif /* _SQL_H_ */

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

#ifndef _KEYCODES_H_
#define _KEYCODES_H_

#define KEY_ENTER 13
#define KEY_TAB 9

#define KEY_BASE 0x100

/*  Function keys  */
#define KEY_F (KEY_BASE+64)

/* Control keys */
#define KEY_CTRL (KEY_BASE)
#define KEY_BACKSPACE (KEY_CTRL+0)
#define KEY_DELETE (KEY_CTRL+1)
#define KEY_INSERT (KEY_CTRL+2)
#define KEY_HOME (KEY_CTRL+3)
#define KEY_END (KEY_CTRL+4)
#define KEY_PAGE_UP (KEY_CTRL+5)
#define KEY_PAGE_DOWN (KEY_CTRL+6)
#define KEY_ESC (KEY_CTRL+7)

/* Control keys short name */
#define KEY_BS KEY_BACKSPACE
#define KEY_DEL KEY_DELETE
#define KEY_INS KEY_INSERT
#define KEY_PGUP KEY_PAGE_UP
#define KEY_PGDOWN KEY_PAGE_DOWN
#define KEY_PGDWN KEY_PAGE_DOWN

/* Cursor movement */
#define KEY_CRSR (KEY_BASE+16)
#define KEY_RIGHT (KEY_CRSR+0)
#define KEY_LEFT (KEY_CRSR+1)
#define KEY_DOWN (KEY_CRSR+2)
#define KEY_UP (KEY_CRSR+3)

/* XF86 Multimedia keyboard keys */
#define KEY_XF86_BASE (0x100+384)
#define KEY_XF86_PAUSE (KEY_XF86_BASE+1)
#define KEY_XF86_STOP (KEY_XF86_BASE+2)
#define KEY_XF86_PREV (KEY_XF86_BASE+3)
#define KEY_XF86_NEXT (KEY_XF86_BASE+4)

/* Keypad keys */
#define KEY_KEYPAD (KEY_BASE+32)
#define KEY_KP0 (KEY_KEYPAD+0)
#define KEY_KP1 (KEY_KEYPAD+1)
#define KEY_KP2 (KEY_KEYPAD+2)
#define KEY_KP3 (KEY_KEYPAD+3)
#define KEY_KP4 (KEY_KEYPAD+4)
#define KEY_KP5 (KEY_KEYPAD+5)
#define KEY_KP6 (KEY_KEYPAD+6)
#define KEY_KP7 (KEY_KEYPAD+7)
#define KEY_KP8 (KEY_KEYPAD+8)
#define KEY_KP9 (KEY_KEYPAD+9)
#define KEY_KPDEC (KEY_KEYPAD+10)
#define KEY_KPINS (KEY_KEYPAD+11)
#define KEY_KPDEL (KEY_KEYPAD+12)
#define KEY_KPENTER (KEY_KEYPAD+13)

#endif // _KEYCODES_H_
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

#ifndef _LOCK_H_
#define _LOCK_H_

FILE* lock_fopen(const char* filename, int *info);
int   lock_fclose(FILE *fp, const char* filename, int *info);
int lock_open(const char* filename, int *info);
int lock_close(int fp, const char* filename, int *info);

#endif // _LOCK_H_

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

#ifndef _MALLOC_H_
#define _MALLOC_H_

#include <stdlib.h>

#ifdef __DEBUG

#define MALLOC(result, type, number) if(!((result) = (type *)malloc((number) * sizeof(type)))) { printf("malloc failure at %s:%d \n", __FILE__, __LINE__); abort(); }
#define CALLOC(result, type, number) if(!((result) = (type *)calloc((number), sizeof(type)))) { printf("calloc failure at %s:%d \n", __FILE__, __LINE__); abort(); }
#define REALLOC(result, type, number) if(!((result) = (type *)realloc((result), sizeof(type) * (number)))) { printf("realloc failure at %s:%d \n", __FILE__, __LINE__); abort(); }
#define FREE(result) if(result) { free(result); result = NULL; }

#else

#define MALLOC(result, type, number) (result) = (type *)malloc((number) * sizeof(type))
#define CALLOC(result, type, number) (result) = (type *)calloc((number), sizeof(type))
#define REALLOC(result, type, number) (result) = (type *)realloc((result), sizeof(type) * (number))
#define FREE(result) if(result) { free(result); result = NULL; }

#endif /* __DEBUG */
#endif /* _MALLOC_H_ */
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

#ifndef _MD5CALC_H_
#define _MD5CALC_H_

void MD5_String(const char * string, char * output);
void MD5_String2binary(const char * string, unsigned char * output);

#endif // _MD5CALC_H_
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

#ifndef _SOCKET_H_
#define _SOCKET_H_

#include <stdio.h>

#ifdef __WIN32
#define __USE_W32_SOCKETS
#include <windows.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#endif

// define declaration

#define RFIFOP(fd,pos) (session[fd]->rdata + session[fd]->rdata_pos + (pos))
#define RFIFOB(fd,pos) (*(unsigned char*)(session[fd]->rdata + session[fd]->rdata_pos + (pos)))
#define RFIFOW(fd,pos) (*(unsigned short*)(session[fd]->rdata + session[fd]->rdata_pos + (pos)))
#define RFIFOL(fd,pos) (*(unsigned int*)(session[fd]->rdata + session[fd]->rdata_pos + (pos)))
#ifdef __DEBUG
#define RFIFOSKIP(fd,len) ((session[fd]->rdata_size - session[fd]->rdata_pos - (len) < 0) ? (fprintf(stderr, "Too many skips at %s:%d.\n", __FILE__, __LINE__), exit(1)) : (session[fd]->rdata_pos += (len)))
#else
#define RFIFOSKIP(fd,len) (session[fd]->rdata_pos += (len))
#endif
#define RFIFOREST(fd) (session[fd]->rdata_size - session[fd]->rdata_pos)
#define RFIFOFLUSH(fd) (memmove(session[fd]->rdata, RFIFOP(fd,0), RFIFOREST(fd)), session[fd]->rdata_size = RFIFOREST(fd), session[fd]->rdata_pos = 0)
#define RFIFOSPACE(fd) (session[fd]->max_rdata - session[fd]->rdata_size)
#define RBUFP(p,pos) (((unsigned char*)(p)) + (pos))
#define RBUFB(p,pos) (*(unsigned char*)((p) + (pos)))
#define RBUFW(p,pos) (*(unsigned short*)((p) + (pos)))
#define RBUFL(p,pos) (*(unsigned int*)((p) + (pos)))

#define WFIFOSPACE(fd) (session[fd]->max_wdata - session[fd]->wdata_size)
char WPACKETBUF[65536]; /* biggest paket: Guild storage with only equipement: 4 + 22 * 1000 = 22004, biggest have size in WPACKETW(2) */
#define WPACKETP(pos) (WPACKETBUF + (pos))
#define WPACKETB(pos) (*(unsigned char*)(WPACKETBUF + (pos)))
#define WPACKETW(pos) (*(unsigned short*)(WPACKETBUF + (pos)))
#define WPACKETL(pos) (*(unsigned int*)(WPACKETBUF + (pos)))
// use function instead of macro.
//#define SENDPACKET(fd,len) (memcpy(session[fd]->wdata + session[fd]->wdata_size, WFIFOBUF, len); session[fd]->wdata_size = session[fd]->wdata_size + (len);)
// These MACRO, under, are obsolet
//#define WFIFOP(fd,pos) (session[fd]->wdata + session[fd]->wdata_size + (pos))
//#define WFIFOB(fd,pos) (*(unsigned char*)(session[fd]->wdata + session[fd]->wdata_size + (pos)))
//#define WFIFOW(fd,pos) (*(unsigned short*)(session[fd]->wdata + session[fd]->wdata_size + (pos)))
//#define WFIFOL(fd,pos) (*(unsigned int*)(session[fd]->wdata + session[fd]->wdata_size + (pos)))
// use function instead of macro.
//#define WFIFOSET(fd,len) (session[fd]->wdata_size = (session[fd]->wdata_size + (len) + 2048 < session[fd]->max_wdata) ? session[fd]->wdata_size + len : session[fd]->wdata_size)
#define WBUFP(p,pos) (((unsigned char*)(p)) + (pos))
#define WBUFB(p,pos) (*(unsigned char*)((p) + (pos)))
#define WBUFW(p,pos) (*(unsigned short*)((p) + (pos)))
#define WBUFL(p,pos) (*(unsigned int*)((p) + (pos)))

#ifdef __INTERIX
#define FD_SETSIZE 4096
#endif // __INTERIX

/* Removed Cygwin FD_SETSIZE declarations, now are directly passed on to the compiler through Makefile [Valaris] */
/* Removed LCCWIN32 FD_SETSIZE declarations, must be passed directly to the compiler while compiling [MagicalTux] */

// Struct declaration

struct socket_data{
	unsigned char eof; /* 0 or 1 */
	unsigned char not_used_big_rdata, not_used_big_wdata; /* counter to reduce max_r/wdata of a socket when it was increased */
	char *rdata, *wdata;
	int max_rdata, max_wdata;
	int rdata_size, wdata_size;
	int rdata_pos;
	struct sockaddr_in client_addr;
	int (*func_recv)(int);
	int (*func_send)(int);
	int (*func_parse)(int);
	int (*func_console)(char*);
	void* session_data;
};

// Data prototype declaration

extern struct socket_data *session[FD_SETSIZE];

int fd_max;

extern char listen_ip[16]; // 15 + NULL

// Function prototype declaration

int make_listen_port(int);
int make_connection(long, int);
int delete_session(int);
int realloc_fifo(int fd, int rfifo_size, int wfifo_size);
void SENDPACKET(const int fd, const int len);
//int WFIFOSET(int fd, unsigned int len);
//int RFIFOSKIP(int fd, int len);

void flush_fifos(void);
int do_sendrecv(int next);
int do_parsepacket(void);
void do_socket(void);

int start_console(int (*parse_console_func)(char*));

void set_defaultparse(int (*defaultparse)(int));

int Net_Init(void);

unsigned int addr_[16]; /* ip addresses of local host (host byte order) */
unsigned int naddr_; /* # of ip addresses */

#endif // _SOCKET_H_
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

#ifndef _TIMER_H_
#define _TIMER_H_

#ifdef __WIN32
/* We need winsock lib to have timeval struct - windows is weirdo */
#define __USE_W32_SOCKETS
#include <windows.h>
#endif

#define DIFF_TICK(a,b) ((int)((a)-(b)))

// Struct declaration

struct TimerData {
	unsigned int tick;
	int (*func)(int, unsigned int, int, int);
	int id;
	int data;
	char type; // 0 or TIMER_ONCE_AUTODEL or TIMER_INTERVAL or TIMER_REMOVE_HEAP
	int interval;
};

// Function prototype declaration

#ifdef __WIN32
int gettimeofday(struct timeval *t, void *dummy);
#endif

void init_gettick(void);
unsigned int gettick_nocache(void);
extern unsigned int gettick_cache;

int add_timer(unsigned int, int (*)(int,unsigned int,int,int), int, int);
int add_timer_interval(unsigned int, int (*)(int,unsigned int,int,int), int, int, int);
int delete_timer(int, int (*)(int,unsigned int, int, int));

unsigned int addtick_timer(int tid, int added_tick);
struct TimerData *get_timer(int tid);

int do_timer(void);

int add_timer_func_list(int (*)(int,unsigned int,int,int), char*);

void timer_final();

#endif	// _TIMER_H_
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

#ifndef _VERSION_H_
#define _VERSION_H_

#define FREYA_STATE				1		// 0 = Release, 1 = Development

#define FREYA_MAJORVERSION		3		// Major version number
#define FREYA_MINORVERSION		1		// Minor version number
#define FREYA_REVISION			0		// Revision number

#define FREYA_LOGINVERSION		1		// Login-server version
#define FREYA_CHARVERSION		2		// Character-server version
#define FREYA_INTERVERSION		4		// Inter-server version
#define FREYA_MAPVERSION		8		// Map-server version

#endif // _VERSION_H_
