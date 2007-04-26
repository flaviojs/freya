// Copyright (c) Freya Development Team - Licensed under GNU GPL
// For more information, see License.txt in the main folder

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
