// Copyright (c) Freya Development Team - Licensed under GNU GPL
// For more information, see License.txt in the main folder

#ifdef DYNAMIC_LINKING
 #ifdef __ADDON
  #define LOCAL_EX_SYMBOL(_symbol, _offset,_var) memcpy(&_var, call_table + (_offset * 4), 4)
 #else
  #define LOCAL_EX_SYMBOL(_symbol, _offset,_var) addon_tmp_pointer = _symbol; memcpy(call_table + (_offset * 4), &addon_tmp_pointer, 4)
 #endif
#else
 #define LOCAL_EX_SYMBOL(_symbol, _offset,_var) _var = _symbol
#endif

#define LFNC_COUNT 0

// common export table

// #define LFNC_xxx(_var) LOCAL_EX_SYMBOL(xxx,0,_var)
