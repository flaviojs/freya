// Copyright (c) Freya Development Team - Licensed under GNU GPL
// For more information, see License.txt in the main folder

#ifndef _LOCK_H_
#define _LOCK_H_

FILE* lock_fopen(const char* filename, int *info);
int   lock_fclose(FILE *fp, const char* filename, int *info);
int lock_open(const char* filename, int *info);
int lock_close(int fp, const char* filename, int *info);

#endif // _LOCK_H_

