// $Id: addons.c 495 2006-04-07 09:02:46Z Harbin $

#include <config.h>

#include <stdlib.h>
#include <string.h>

#include "../common/addons.h"
#include "addons.h"

void init_localcalltable(void)
{
#ifdef DYNAMIC_LINKING
	local_table = malloc(LFNC_COUNT * 4);
#endif
}

