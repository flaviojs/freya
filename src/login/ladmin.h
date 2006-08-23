// $Id: ladmin.h 464 2005-10-30 22:27:54Z Yor $
#ifndef _LADMIN_H_
#define _LADMIN_H_

int display_parse_admin; /* 0: no, 1: yes */
int add_to_unlimited_account; /* Give possibility or not to adjust (ladmin command: timeadd) the time of an unlimited account. */

int parse_admin(int fd);

#endif // _LADMIN_H_
