// $Id: malloc.h 269 2005-09-12 20:41:10Z Yor $
#ifndef _MALLOC_H_
#define _MALLOC_H_

#include <stdlib.h> // malloc, calloc, free

#define MALLOC(result, type, number) if (!((result) = (type *)malloc((number) * sizeof(type)))) \
      printf("SYSERR: malloc failure at %s:%d.\n", __FILE__, __LINE__), abort()

#define CALLOC(result, type, number) if (!((result) = (type *)calloc((number), sizeof(type)))) \
      printf("SYSERR: calloc failure at %s:%d.\n", __FILE__, __LINE__), abort()

#define REALLOC(result, type, number) if (!((result) = (type *)realloc((result), sizeof(type) * (number)))) \
      printf("SYSERR: realloc failure at %s:%d.\n", __FILE__, __LINE__), abort()

#define FREE(result) if (result) free(result), result = NULL

#endif // _MALLOC_H_
