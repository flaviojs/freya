// $Id: trade.h 1 2006-01-13 22:47:50Z MagicalTux $
#ifndef	_TRADE_H_
#define	_TRADE_H_

#include "map.h"

extern unsigned char log_trade_level;

void trade_traderequest(struct map_session_data *sd, int target_id, int near_check);
void trade_tradeack(struct map_session_data *sd, unsigned char type);
void trade_tradeadditem(struct map_session_data *sd, int idx, int amount);
void trade_tradeok(struct map_session_data *sd);
void trade_tradecancel(struct map_session_data *sd);
void trade_tradecommit(struct map_session_data *sd);

#endif	// _TRADE_H_
