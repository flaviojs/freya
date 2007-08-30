// Copyright (c) Freya Development Team - Licensed under GNU GPL
// For more information, see License.txt in the main folder

#include <config.h>

#include <stdlib.h>
#include <string.h>

#include "../common/addons.h"
#include "addons.h"

void init_localcalltable(void) {

#ifdef DYNAMIC_LINKING
	local_table = malloc(LFNC_COUNT * 4);
	// Put here list of exported functions...
#endif
}
