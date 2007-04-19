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

#ifndef _NULLPO_H_
#define _NULLPO_H_

#define NULLPO_CHECK 1

#if __STDC_VERSION__ < 199901L
# if __GNUC__ >= 2
#  define __func__ __FUNCTION__
# else
#  define __func__ ""
# endif
#endif

#ifdef LCCWIN32
/* This is only required with LCCWIN32 and not with gcc Win32 */
#define __attribute__(x)	// nothing
#endif

#define NLP_MARK __FILE__, __LINE__, __func__

/*----------------------------------------------------------------------------
 * Macros
 *----------------------------------------------------------------------------
 */
#if NULLPO_CHECK

void nullpo_info_core_simple(const char *file, int line, const char *func);

#define nullpo_ret(t) if (!(t)) { nullpo_info_core_simple(NLP_MARK); return 0; }
#define nullpo_retv(t) if (!(t)) { nullpo_info_core_simple(NLP_MARK); return; }
#define nullpo_retr(ret, t) if (!(t)) { nullpo_info_core_simple(NLP_MARK); return (ret); }

#else /* NULLPO_CHECK */
/* No Nullpo check */

#define nullpo_ret(t) if ((t)) { ; }
#define nullpo_retv(t) if ((t)) { ;}
#define nullpo_retr(ret, t) if ((t)) { ; }

#endif /* NULLPO_CHECK */

#endif // _NULLPO_H_
