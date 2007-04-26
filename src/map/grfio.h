// Copyright (c) Freya Development Team - Licensed under GNU GPL
// For more information, see License.txt in the main folder

#ifndef _GRFIO_H_
#define _GRFIO_H_

void grfio_init(char*);         // GRFIO Initialize
int grfio_add(char*);           // GRFIO Resource file add
void* grfio_read(char*);        // GRFIO data file read
void* grfio_reads(char*, int*); // GRFIO data file read & size get
int grfio_size(char*);          // GRFIO data file size get

int decode_zip(char *dest, unsigned long* destLen, const char* source, unsigned long sourceLen);
int encode_zip(char *dest, unsigned long* destLen, const char* source, unsigned long sourceLen);

// Accessor to GRF filenames
char *grfio_setdatafile(const char *str);
char *grfio_setadatafile(const char *str);
char *grfio_setsdatafile(const char *str);

typedef struct {
	int   srclen; // compressed size
	int   srclen_aligned;
	int   declen; // original size
	int   srcpos;
	int   next; // -1: no next, 0+: pointer to index of next value
	char  cycle;
	char  type;
	char  fn[128-4*5]; // file name
	char  gentry; // read grf file select
} FILELIST;
// to search if a file exists
FILELIST *filelist_find(char *fname);

#endif // _GRFIO_H_
