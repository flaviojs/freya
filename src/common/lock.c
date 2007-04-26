// Copyright (c) Freya Development Team - Licensed under GNU GPL
// For more information, see License.txt in the main folder

#include <config.h>

#include <stdio.h>
#include <unistd.h> // for access function
#include <sys/stat.h> // low level access
#include <fcntl.h> // O_CREAT
#include <errno.h> // errno
#include <string.h> // strerror

#include "lock.h"
#include "utils.h"

// 書き込みファイルの保護処理
// （書き込みが終わるまで、旧ファイルを保管しておく）

// 新しいファイルの書き込み開始
FILE* lock_fopen(const char* filename, int *info) {
	char newfile[512];
	FILE *fp;
	int no = 0;

	// 安全なファイル名を得る（手抜き）
	do {
		sprintf(newfile, "%s_%04d.tmp", filename, ++no);
	} while((fp = fopen(newfile, "r")) && (fclose(fp), no < 9999));
	*info = no;

	return fopen(newfile, "w");
}

// The old (previous) file deletion, the renaming of the old file to previous file and the creation of the new
int lock_fclose(FILE *fp, const char* filename, int *info) {
	int  ret = 0;
	char newfile[512];
	char backupfile[512];

	if (fp != NULL) {
		ret = fclose(fp);
		if (ret == 0) {
			sprintf(newfile, "%s_%04d.tmp", filename, *info);
			sprintf(backupfile, "%s.previous", filename);
			// if a previous file exists
			if (access(backupfile, 0) == 0) // 0: file exist or not, 0 = success
				remove(backupfile); // delete the file. return value = 0 (success), return value = -1 (denied access or not found file)
			rename(filename, backupfile); // always try to rename
			// if file exists (doesn't exist, was renamed!... unnormal that continue to exist)
			if (access(filename, 0) == 0) // 0: file exist or not, 0 = success
				remove(filename); // delete the file. return value = 0 (success), return value = -1 (denied access or not found file)
			// When it's fail at this point, it's not good :(
			errno = 0;
			if ((ret = rename(newfile, filename))) // always try to rename
				printf("%s - rename '" CL_WHITE "%s" CL_RESET "' to '" CL_WHITE "%s" CL_RESET "'.\n", strerror(errno), filename, newfile);
		}
		return ret; // if 0: normal (no error)
	}

	return 1;
}

int lock_open(const char* filename, int *info) {
	char newfile[512];
	int fp;
	int no = 0;

	// 安全なファイル名を得る（手抜き）
	do {
		sprintf(newfile, "%s_%04d.tmp", filename, ++no);
	} while((fp = open(newfile, O_WRONLY | O_CREAT | O_EXCL, 0644)) == -1 && no < 9999);
	*info = no;

	return fp;
}

// The old (previous) file deletion, the renaming of the old file to previous file and the creation of the new
int lock_close(int fp, const char* filename, int *info) {
	int  ret = 0;
	char newfile[512];
	char backupfile[512];

	if (fp != -1) {
		ret = close(fp);
		if (ret == 0) {
			sprintf(newfile, "%s_%04d.tmp", filename, *info);
			sprintf(backupfile, "%s.previous", filename);
			// if a previous file exists
			if (access(backupfile, 0) == 0) // 0: file exist or not, 0 = success
				remove(backupfile); // delete the file. return value = 0 (success), return value = -1 (denied access or not found file)
			rename(filename, backupfile); // always try to rename
			// if file exists (doesn't exist, was renamed!... unnormal that continue to exist)
			if (access(filename, 0) == 0) // 0: file exist or not, 0 = success
				remove(filename); // delete the file. return value = 0 (success), return value = -1 (denied access or not found file)
			// When it's fail at this point, it's not good :(
			errno = 0;
			if ((ret = rename(newfile, filename))) // always try to rename
				printf("%s - rename '" CL_WHITE "%s" CL_RESET "' to '" CL_WHITE "%s" CL_RESET "'.\n", strerror(errno), filename, newfile);
		}
		return ret; // if 0: normal (no error)
	}

	return 1;
}

