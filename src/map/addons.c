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

#include <stdlib.h>
#include <string.h>

#include "../common/addons.h"
#include "addons.h"

void init_localcalltable(void) {
#ifdef DYNAMIC_LINKING
	local_table = malloc(LFNC_COUNT * 4);
	// put here list of exported functions...
#endif
}

