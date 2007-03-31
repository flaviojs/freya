// Copyright (c) Freya Development Team - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

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
