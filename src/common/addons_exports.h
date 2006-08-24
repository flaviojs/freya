// $Id: addons_exports.h 502 2005-11-11 20:52:23Z Yor $
// Addons exports : exported functions to addons...

#ifdef DYNAMIC_LINKING
 #ifdef __ADDON
  #define EXPORTED_SYMBOL(_symbol,_offset,_var) memcpy(&_var,call_table+(_offset*4),4)
 #else
  #define EXPORTED_SYMBOL(_symbol,_offset,_var) addon_tmp_pointer=_symbol; memcpy(call_table+(_offset*4),&addon_tmp_pointer,4)
 #endif
#else
 #define EXPORTED_SYMBOL(_symbol,_offset,_var) _var=_symbol
#endif

#define MFNC_COUNT 8

// common export table

#define MFNC_LOCAL_TABLE(_var) EXPORTED_SYMBOL(local_table, 0, _var)
#define MFNC_DISPLAY_TITLE(_var) EXPORTED_SYMBOL(display_title, 1, _var)
#define MFNC_GETTICK(_var) EXPORTED_SYMBOL(gettick, 2, _var)
#define MFNC_ADD_TIMER(_var) EXPORTED_SYMBOL(add_timer, 2, _var)
#define MFNC_ADD_TIMER_INTERVAL(_var) EXPORTED_SYMBOL(add_timer_interval, 3, _var)
#define MFNC_DELETE_TIMER(_var) EXPORTED_SYMBOL(delete_timer, 4, _var)
#define MFNC_ADDTICK_TIMER(_var) EXPORTED_SYMBOL(addtick_timer, 5, _var)
#define MFNC_GET_VERSION(_var) EXPORTED_SYMBOL(get_version, 6, _var)

