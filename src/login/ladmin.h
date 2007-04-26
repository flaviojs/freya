// Copyright (c) Freya Development Team - Licensed under GNU GPL
// For more information, see License.txt in the main folder

#ifndef _LADMIN_H_
#define _LADMIN_H_

int display_parse_admin; /* 0: no, 1: yes */
int add_to_unlimited_account; /* Give possibility or not to adjust (ladmin command: timeadd) the time of an unlimited account. */
unsigned char ladmin_min_GM_level; /* minimum GM level of a account for listGM/lsGM ladmin command */

int parse_admin(int fd);

#endif // _LADMIN_H_
