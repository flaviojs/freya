// Copyright (c) Freya Development Team - Licensed under GNU GPL
// For more information, see License.txt in the main folder

#include <config.h>

#include <stdio.h>
#include <string.h>

#include "nullpo.h"

#if NULLPO_CHECK

void nullpo_info_core_simple(const char *file, int line, const char *func) {
	printf("--- nullpo info --------------------------------------------\n");
	printf("%s:%d: in func `%s'\n",
	       (file == NULL) ? "??" : file,
	       line,
	       (func == NULL || func[0] == '\0') ? "unknown": func);
	printf("--- end nullpo info ----------------------------------------\n");

	return;
}

#endif /* NULLPO_CHECK */

