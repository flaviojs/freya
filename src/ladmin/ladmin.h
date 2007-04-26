// Copyright (c) Freya Development Team - Licensed under GNU GPL
// For more information, see License.txt in the main folder

#ifndef _LADMIN_H_
#define _LADMIN_H_

#define LADMIN_CONF_NAME	"conf/ladmin_freya.conf"
#define PASSWORDENC		3	// A definition is given when making an encryption password correspond.
							// It is 1 at the time of passwordencrypt.
							// It is made into 2 at the time of passwordencrypt2.
							// When it is made 3, it corresponds to both.

int parse_fromlogin(int fd); // global definition (called by socket.c)

#endif // _LADMIN_H_
