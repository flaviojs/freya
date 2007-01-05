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

