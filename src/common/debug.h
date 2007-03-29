// Copyright (c) Freya Development Team - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#ifndef _DEBUG_H_
#define _DEBUG_H_

#ifdef __DEBUG

#define MALLOC(result, type, number) if(!((result) = (type *)malloc((number) * sizeof(type)))) { printf("malloc failure at %s:%d \n", __FILE__, __LINE__); abort(); }
#define CALLOC(result, type, number) if(!((result) = (type *)calloc((number), sizeof(type)))) { printf("calloc failure at %s:%d \n", __FILE__, __LINE__); abort(); }
#define REALLOC(result, type, number) if(!((result) = (type *)realloc((result), sizeof(type) * (number)))) { printf("realloc failure at %s:%d \n", __FILE__, __LINE__); abort(); }
#define FREE(result) if(result) { free(result); result = NULL; }
#define ASSERT(var, ret) if(!var) { printf("assertion check failed at %s:%d \n", __FILE__, __LINE__); return ret; }
#define ASSERTV(var) if(!var) { printf("assertion check failed at %s:%d \n", __FILE__, __LINE__); return; }

#else /* __DEBUG -> !__DEBUG */

#define MALLOC(result, type, number) (result) = (type *)malloc((number) * sizeof(type))
#define CALLOC(result, type, number) (result) = (type *)calloc((number), sizeof(type))
#define REALLOC(result, type, number) (result) = (type *)realloc((result), sizeof(type) * (number))
#define FREE(result) if(result) { free(result); result = NULL; }
#define ASSERT(var, ret) /* this is a dummy comment */
#define ASSERTV(var) /* this is a dummy comment */


#endif /* !__DEBUG */
#endif /* DEBUG_H_ */
