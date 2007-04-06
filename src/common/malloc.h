// $Id: malloc.h 187 2006-02-04 00:00:27Z MagicalTux $
#ifndef _MALLOC_H_
#define _MALLOC_H_

#include <config.h>
#include <stdlib.h> // malloc, calloc, free

#ifdef __LEAK_TRACER

void *internal_malloc(size_t size, char *file, int line);
void *internal_calloc(size_t size, char *file, int line);
void *internal_realloc(void *old, size_t size, char *file, int line);
void internal_free(void *ptr, char *file, int line);

#define MALLOC(result, type, number) result = (type *)internal_malloc((number) * sizeof(type), __FILE__, __LINE__)
#define CALLOC(result, type, number) result = (type *)internal_calloc((number) * sizeof(type), __FILE__, __LINE__)
#define REALLOC(result, type, number) result = (type *)internal_realloc(result, (number) * sizeof(type), __FILE__, __LINE__)
#define FREE(result) if (result != NULL) internal_free(result, __FILE__, __LINE__), result = NULL

#else

#define MALLOC(result, type, number) if (!((result) = (type *)malloc((number) * sizeof(type)))) \
      printf("SYSERR: malloc failure at %s:%d.\n", __FILE__, __LINE__), abort()

#define CALLOC(result, type, number) if (!((result) = (type *)calloc((number), sizeof(type)))) \
      printf("SYSERR: calloc failure at %s:%d.\n", __FILE__, __LINE__), abort()

#define REALLOC(result, type, number) if (!((result) = (type *)realloc((result), sizeof(type) * (number)))) \
      printf("SYSERR: realloc failure at %s:%d.\n", __FILE__, __LINE__), abort()

#define FREE(result) if (result) free(result), result = NULL

#endif

#endif // _MALLOC_H_
