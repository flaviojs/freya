// $Id: ladmin.h 1 2006-01-13 22:47:50Z MagicalTux $
#ifndef _LADMIN_H_
#define _LADMIN_H_

int display_parse_admin; /* 0: no, 1: yes */
int add_to_unlimited_account; /* Give possibility or not to adjust (ladmin command: timeadd) the time of an unlimited account. */

int parse_admin(int fd);

#endif // _LADMIN_H_
