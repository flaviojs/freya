// $Id: lock.h 464 2005-10-30 22:27:54Z Yor $
#ifndef _LOCK_H_
#define _LOCK_H_

FILE* lock_fopen(const char* filename, int *info);
int   lock_fclose(FILE *fp, const char* filename, int *info);
int lock_open(const char* filename, int *info);
int lock_close(int fp, const char* filename, int *info);

#endif // _LOCK_H_

