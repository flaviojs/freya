/* Copyright (C) 2007 Freya Development Team

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA. */

#ifdef DYNAMIC_LINKING
 #ifdef __ADDON
  #define LOCAL_EX_SYMBOL(_symbol,_offset,_var) memcpy(&_var,call_table+(_offset*4),4)
 #else
  #define LOCAL_EX_SYMBOL(_symbol,_offset,_var) addon_tmp_pointer=_symbol; memcpy(call_table+(_offset*4),&addon_tmp_pointer,4)
 #endif
#else
 #define LOCAL_EX_SYMBOL(_symbol,_offset,_var) _var=_symbol
#endif

#define LFNC_COUNT 0

// Common export table

// #define LFNC_xxx(_var) LOCAL_EX_SYMBOL(xxx,0,_var)
