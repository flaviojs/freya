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
