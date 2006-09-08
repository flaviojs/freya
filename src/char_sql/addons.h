// $Id: addons.h 502 2005-11-11 20:52:23Z Yor $
// Addons local exports : exported functions to addons...

#ifdef DYNAMIC_LINKING
 #ifdef __ADDON
  #define LOCAL_EX_SYMBOL(_symbol, _offset, _var) memcpy(&_var, call_table + (_offset * 4), 4)
 #else
  #define LOCAL_EX_SYMBOL(_symbol, _offset, _var) addon_tmp_pointer=_symbol; memcpy(call_table + (_offset * 4), &addon_tmp_pointer, 4)
 #endif
#else
 #define LOCAL_EX_SYMBOL(_symbol, _offset,_var) _var = _symbol
#endif

#define LFNC_COUNT 0

// common export table

// #define LFNC_xxx(_var) LOCAL_EX_SYMBOL(xxx,0,_var)

