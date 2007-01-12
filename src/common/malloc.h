/*	This file is a part of Freya.
		Freya is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	any later version.
		Freya is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.
		You should have received a copy of the GNU General Public License
	along with Freya; if not, write to the Free Software
	Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA */

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
