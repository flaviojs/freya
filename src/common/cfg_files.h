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

#ifndef _CFG_FILES_H_
#define _CFG_FILES_H_

#define CONF_VAR_CHAR 0
#define CONF_VAR_SHORT 1
#define CONF_VAR_INT 2
#define CONF_VAR_STRING 3
#define CONF_VAR_CALLBACK 4

#define CONF_OK 0
#define CONF_ERROR 1

struct conf_entry {
	char *param_name; // 4 bytes + malloc'd string
	void *save_pointer; // 4 bytes
	int min_limit,max_limit; // 8 bytes : min. and max limit of the value (zero = not used)
	char type; // 1 byte
	// 3 bytes lost
}

#endif // _CFG_FILES_H_
