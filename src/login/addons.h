// Copyright (c) Freya Development Team - Licensed under GNU GPL
// For more information, see License.txt in the main folder

#ifdef DYNAMIC_LINKING
 #ifdef __ADDON
  #define LOCAL_EX_SYMBOL(_symbol,_offset,_var) memcpy(&_var,local_table+(_offset*4),4)
 #else
  #define LOCAL_EX_SYMBOL(_symbol,_offset,_var) addon_tmp_pointer=_symbol; memcpy(local_table+(_offset*4),&addon_tmp_pointer,4)
 #endif
#else
 #define LOCAL_EX_SYMBOL(_symbol,_offset,_var) _var=_symbol
#endif

#define LFNC_COUNT 4

/* common export table */

#define LFNC_SERVER(_var) LOCAL_EX_SYMBOL(&server, 0, _var)
#define LFNC_SERVER_FD(_var) LOCAL_EX_SYMBOL(&server_fd, 1, _var)
#define LFNC_LOGIN_PORT(_var) LOCAL_EX_SYMBOL(&login_port, 2, _var)
#define LFNC_GETTICK_CACHE(_var) LOCAL_EX_SYMBOL(&gettick_cache, 3, _var)

