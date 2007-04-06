/* cfg_files.h : config files parser - header
 * $Id: cfg_files.h 464 2005-10-30 22:27:54Z Yor $
 * Made by MagicalTux
 */

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
