// Copyright (c) Freya Development Team - Licensed under GNU GPL
// For more information, see License.txt in the main folder

#ifndef _MALLOC_H_
#define _MALLOC_H_

#include <stdlib.h>

#define MALLOC(result, type, number) (result) = (type *)malloc((number) * sizeof(type))
#define CALLOC(result, type, number) (result) = (type *)calloc((number), sizeof(type))
#define REALLOC(result, type, number) (result) = (type *)realloc((result), sizeof(type) * (number))
#define FREE(result) if(result) { free(result); result = NULL; }

#endif /* _MALLOC_H_ */
