// Copyright (c) Freya Development Team - Licensed under GNU GPL
// For more information, see License.txt in the main folder

#ifndef	_VENDING_H_
#define	_VENDING_H_

#include "map.h"

extern unsigned char log_vending_level;

void vending_closevending(struct map_session_data *sd);
void vending_openvending(struct map_session_data *sd, unsigned short len, char *message, unsigned char flag, char *p);
void vending_vendinglistreq(struct map_session_data *sd, int id);
void vending_purchasereq(struct map_session_data *sd, short len, int id, char *p);

#endif	// _VENDING_H_
