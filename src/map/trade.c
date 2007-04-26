// Copyright (c) Freya Development Team - Licensed under GNU GPL
// For more information, see License.txt in the main folder

#include <config.h>

#include <stdio.h>
#include <string.h>

#include "../common/socket.h"

#include "clif.h"
#include "itemdb.h"
#include "map.h"
#include "trade.h"
#include "pc.h"
#include "npc.h"
#include "battle.h"
#include "chrif.h"
#include "intif.h"
#include "atcommand.h"

#include "nullpo.h"

unsigned char log_trade_level = 0;

/*===============================================
 * This function logs all trades
 *-----------------------------------------------
 */
void log_trade(struct map_session_data *sd, struct map_session_data *target_sd) {
	int flag, trade_i, amount;
	int j, counter;
	char tmpstr[8192]; // max lines: trade (1) + players names (2) + 10x2 items + total (2) = 25
	                   // longest line: (24x2x4)(cart names)+4(cart #)+5(amount)+46(other) = 247
	                   // 247 * 25 = 6175 (8192)
	int tmpstr_len;
	unsigned char *sin_addr;
	struct item_data *item_data, *item_temp;

	// don't log if GM level is not enough
	if (sd->GM_level < log_trade_level && target_sd->GM_level < log_trade_level)
		return;

	// log title
	tmpstr_len = sprintf(tmpstr, "%s:" RETCODE, msg_txt(595)); // trade done

	// log first player
	sin_addr = (unsigned char *)&session[sd->fd]->client_addr.sin_addr;
	tmpstr_len += sprintf(tmpstr + tmpstr_len, "* %s [%d(%s:%d)] (ip:%d.%d.%d.%d) %s:" RETCODE, sd->status.name, sd->status.account_id, msg_txt(597), sd->GM_level, sin_addr[0], sin_addr[1], sin_addr[2], sin_addr[3], msg_txt(596)); // lvl // gives

	// log items of the first player
	flag = 0;
	for(trade_i = 0; trade_i < 10; trade_i++) {
		amount = sd->deal_item_amount[trade_i];
		if (amount > 0) {
			int n = sd->deal_item_index[trade_i] - 2;
			if (sd->status.inventory[n].amount < amount)
				amount = sd->status.inventory[n].amount;
			if (amount > 0 && (item_data = itemdb_search(sd->status.inventory[n].nameid)) != NULL) {
				if (sd->status.inventory[n].refine)
					tmpstr_len += sprintf(tmpstr + tmpstr_len, " - %d %s %+d (%s %+d, id: %d)" RETCODE, amount, item_data->name, sd->status.inventory[n].refine, item_data->jname, sd->status.inventory[n].refine, sd->status.inventory[n].nameid);
				else
					tmpstr_len += sprintf(tmpstr + tmpstr_len, " - %d %s (%s, id: %d)" RETCODE, amount, item_data->name, item_data->jname, sd->status.inventory[n].nameid);
				counter = 0;
				for (j = 0; j < item_data->slot; j++) {
					if (sd->status.inventory[n].card[j] && (item_temp = itemdb_search(sd->status.inventory[n].card[j])) != NULL) {
						if (counter == 0)
							tmpstr_len += sprintf(tmpstr + tmpstr_len, "   -> (%s: #%d %s (%s), ", msg_txt(598), ++counter, item_temp->name, item_temp->jname); // card(s)
						else
							tmpstr_len += sprintf(tmpstr + tmpstr_len, "#%d %s (%s), ", ++counter, item_temp->name, item_temp->jname);
					}
				}
				if (counter != 0) {
					tmpstr[strlen(tmpstr) - 2] = ')';
					tmpstr[strlen(tmpstr) - 1] = '\0';
					tmpstr_len--;
					tmpstr_len += sprintf(tmpstr + tmpstr_len, RETCODE);
				}
				flag = 1;
			}
		}
	}
	// log zenys of the first player
	if (sd->deal_zeny > 0) {
		if (sd->deal_zeny == 1)
			tmpstr_len += sprintf(tmpstr + tmpstr_len, " - %s" RETCODE, msg_txt(599)); // 1 zeny trades by this player.
		else
			tmpstr_len += sprintf(tmpstr + tmpstr_len, " - %d %s" RETCODE, sd->deal_zeny, msg_txt(600)); // zenys trade by this player.
	} else {
		if (flag)
			tmpstr_len += sprintf(tmpstr + tmpstr_len, " - %s" RETCODE, msg_txt(601)); // No zeny trades by this player.
		else
			tmpstr_len += sprintf(tmpstr + tmpstr_len, " - %s" RETCODE, msg_txt(602)); // Nothing trades by this player.
	}

	// log second player
	sin_addr = (unsigned char *)&session[target_sd->fd]->client_addr.sin_addr;
	tmpstr_len += sprintf(tmpstr + tmpstr_len, "* %s [%d(%s:%d)] (ip:%d.%d.%d.%d) %s:" RETCODE, target_sd->status.name, target_sd->status.account_id, msg_txt(597), target_sd->GM_level, sin_addr[0], sin_addr[1], sin_addr[2], sin_addr[3], msg_txt(596)); // lvl // gives

	// log items of the second player
	flag = 0;
	for(trade_i = 0; trade_i < 10; trade_i++) {
		amount = target_sd->deal_item_amount[trade_i];
		if (amount > 0) {
			int n = target_sd->deal_item_index[trade_i] - 2;
			if (target_sd->status.inventory[n].amount < amount)
				amount = target_sd->status.inventory[n].amount;
			if (amount > 0 && (item_data = itemdb_search(target_sd->status.inventory[n].nameid)) != NULL) {
				if (target_sd->status.inventory[n].refine)
					tmpstr_len += sprintf(tmpstr + tmpstr_len, " - %d %s %+d (%s %+d, id: %d)" RETCODE, amount, item_data->name, target_sd->status.inventory[n].refine, item_data->jname, target_sd->status.inventory[n].refine, target_sd->status.inventory[n].nameid);
				else
					tmpstr_len += sprintf(tmpstr + tmpstr_len, " - %d %s (%s, id: %d)" RETCODE, amount, item_data->name, item_data->jname, target_sd->status.inventory[n].nameid);
				counter = 0;
				for (j = 0; j < item_data->slot; j++) {
					if (target_sd->status.inventory[n].card[j] && (item_temp = itemdb_search(target_sd->status.inventory[n].card[j])) != NULL) {
						if (counter == 0)
							tmpstr_len += sprintf(tmpstr + tmpstr_len, "   -> (%s: #%d %s (%s), ", msg_txt(598), ++counter, item_temp->name, item_temp->jname); // card(s)
						else
							tmpstr_len += sprintf(tmpstr + tmpstr_len, "#%d %s (%s), ", ++counter, item_temp->name, item_temp->jname);
					}
				}
				if (counter != 0) {
					tmpstr[strlen(tmpstr) - 2] = ')';
					tmpstr[strlen(tmpstr) - 1] = '\0';
					tmpstr_len--;
					tmpstr_len += sprintf(tmpstr + tmpstr_len, RETCODE);
				}
				flag = 1;
			}
		}
	}
	// log zenys of the second player
	if (target_sd->deal_zeny > 0) {
		if (target_sd->deal_zeny == 1)
			tmpstr_len += sprintf(tmpstr + tmpstr_len, " - %s" RETCODE RETCODE, msg_txt(599)); // 1 zeny trades by this player.
		else
			tmpstr_len += sprintf(tmpstr + tmpstr_len, " - %d %s" RETCODE RETCODE, target_sd->deal_zeny, msg_txt(600)); // zenys trade by this player.
	} else {
		if (flag)
			tmpstr_len += sprintf(tmpstr + tmpstr_len, " - %s" RETCODE RETCODE, msg_txt(601)); // No zeny trades by this player.
		else
			tmpstr_len += sprintf(tmpstr + tmpstr_len, " - %s" RETCODE RETCODE, msg_txt(602)); // Nothing trades by this player.
	}

	// send log to inter-server
	intif_send_log(LOG_TRADE, tmpstr); // 0x3008 <packet_len>.w <log_type>.B <message>.?B (types: 0 GM commands, 1: Trades, 2: Scripts, 3: Vending)

	return;
}

/*==========================================
 * 取引要請を相手に送る
 *------------------------------------------
 */
void trade_traderequest(struct map_session_data *sd, int target_id, int near_check) { // 1: check if near the other player, 0: doesnt check
	struct map_session_data *target_sd;

//	nullpo_retv(sd); // checked before to call function

	if ((target_sd = map_id2sd(target_id)) != NULL) {
		if (!battle_config.invite_request_check) { // Are other requests accepted during a request or not?
			if (target_sd->guild_invite > 0 || target_sd->party_invite > 0 || // 相手はPT要請中かGuild要請中
			           sd->guild_invite > 0 ||        sd->party_invite > 0) {
				clif_tradestart(sd, 2); // 00e7 <fail>.B: response to requesting trade: 0: You are too far away from the person to trade., 1: This Character is not currently online or does not exist, 2: The person is in another trade., 3: (trade ok->open the trade window)., 4: The deal has been rejected.
				return;
			}
		}
		if (target_sd->trade_partner != 0 || target_sd->npc_id != 0) {
			clif_tradestart(sd, 2); // 00e7 <fail>.B: response to requesting trade: 0: You are too far away from the person to trade., 1: This Character is not currently online or does not exist, 2: The person is in another trade., 3: (trade ok->open the trade window)., 4: The deal has been rejected.
		} else if (sd->trade_partner != 0 || sd->npc_id != 0) {
			trade_tradecancel(sd); // person is in another trade
		} else if (near_check && (sd->bl.m != target_sd->bl.m ||
		                          (sd->bl.x - target_sd->bl.x <= -5 || sd->bl.x - target_sd->bl.x >= 5) ||
		                          (sd->bl.y - target_sd->bl.y <= -5 || sd->bl.y - target_sd->bl.y >= 5))) {
			clif_tradestart(sd, 0); // 00e7 <fail>.B: response to requesting trade: 0: You are too far away from the person to trade., 1: This Character is not currently online or does not exist, 2: The person is in another trade., 3: (trade ok->open the trade window)., 4: The deal has been rejected.
		// if on a gvg map and not in same guild (that can block other player)
		} else if (map[sd->bl.m].flag.gvg && sd->status.guild_id != target_sd->status.guild_id) {
			clif_tradestart(sd, 4); // 00e7 <fail>.B: response to requesting trade: 0: You are too far away from the person to trade., 1: This Character is not currently online or does not exist, 2: The person is in another trade., 3: (trade ok->open the trade window)., 4: The deal has been rejected.
		} else if (sd == target_sd) {
			clif_tradestart(sd, 1); // 00e7 <fail>.B: response to requesting trade: 0: You are too far away from the person to trade., 1: This Character is not currently online or does not exist, 2: The person is in another trade., 3: (trade ok->open the trade window)., 4: The deal has been rejected.
		} else {
			target_sd->trade_partner = sd->status.account_id;
			sd->trade_partner = target_sd->status.account_id;
			sd->deal_locked = 1; // 1: Deal request accepted
			clif_traderequest(target_sd, sd->status.name);
		}
	} else {
		clif_tradestart(sd, 1);  // 00e7 <fail>.B: response to requesting trade: 0: You are too far away from the person to trade., 1: This Character is not currently online or does not exist, 2: The person is in another trade., 3: (trade ok->open the trade window)., 4: The deal has been rejected.
	}

	return;
}

/*==========================================
 * 取引要請
 *------------------------------------------
 */
void trade_tradeack(struct map_session_data *sd, unsigned char type) { // 0x00e6 <type>.B: 3: trade ok., 4: trade canceled. (no other reception with this packet)
	struct map_session_data *target_sd;

//	nullpo_retv(sd); // checked before to call function

	// check type
	if (type != 3 && type != 4)
		return;

	if ((target_sd = map_id2sd(sd->trade_partner)) != NULL && sd->deal_locked == 0) { // 0: no trade, 1: Deal request accepted, 2: Trade accepted, 3: Trade valided (button 'Ok' pressed)
		clif_tradestart(target_sd, type); // 00e7 <fail>.B: response to requesting trade: 0: You are too far away from the person to trade., 1: This Character is not currently online or does not exist, 2: The person is in another trade., 3: (trade ok->open the trade window)., 4: The deal has been rejected.
		clif_tradestart(sd, type); // 00e7 <fail>.B: response to requesting trade: 0: You are too far away from the person to trade., 1: This Character is not currently online or does not exist, 2: The person is in another trade., 3: (trade ok->open the trade window)., 4: The deal has been rejected.
		sd->deal_locked = 1; //1: Deal request accepted
		if (type == 4) { // Cancel
			sd->deal_locked = 0; // 0: no trade, 1: Deal request accepted, 2: Trade accepted, 3: Trade valided (button 'Ok' pressed)
			sd->trade_partner = 0;
			target_sd->deal_locked = 0; // 0: no trade, 1: Deal request accepted, 2: Trade accepted, 3: Trade valided (button 'Ok' pressed)
			target_sd->trade_partner = 0;
		}
		if (sd->npc_id != 0)
			npc_event_dequeue(sd);
		if (target_sd->npc_id != 0)
			npc_event_dequeue(target_sd);
	}

	return;
}

/*==========================================
 * Check here hacker for duplicate item in trade
 * normal client refuse to have 2 same types of item (except equipment) in same trade window
 * normal client authorize only no equiped item and only from inventory
 *------------------------------------------
 */
int impossible_trade_check(struct map_session_data *sd) {
	struct item inventory[MAX_INVENTORY];
	int i, idx;

	nullpo_retr(0, sd);

	// get inventory of player
	memcpy(&inventory, &sd->status.inventory, sizeof(struct item) * MAX_INVENTORY);

/* remove this part: arrows can be trade and equiped
	// remove equiped items (they can not be trade)
	for (i = 0; i < MAX_INVENTORY; i++)
		if (inventory[i].nameid > 0 && inventory[i].equip)
			memset(&inventory[i], 0, sizeof(struct item));
*/

	// check items in player inventory
	for(i = 0; i < 10; i++)
		if (sd->deal_item_amount[i] < 0) { // negativ? -> hack
//			printf("Negativ amount in trade, by hack!\n"); // normal client send cancel when we type negativ amount
			return -1;
		} else if (sd->deal_item_amount[i] > 0) {
			idx = sd->deal_item_index[i] - 2;
			inventory[idx].amount -= sd->deal_item_amount[i]; // remove item from inventory
//			printf("%d items left\n", inventory[idx].amount);
			if (inventory[idx].amount < 0) { // if more than the player have -> hack
				char message_to_gm[strlen(msg_txt(538)) + strlen(msg_txt(539)) + strlen(msg_txt(540)) + strlen(msg_txt(507)) + strlen(msg_txt(508))];
//				printf("A player try to trade more items that he has: hack!\n");
				sprintf(message_to_gm, msg_txt(538), sd->status.name, sd->status.account_id); // Hack on trade: character '%s' (account: %d) try to trade more items that he has.
				intif_wis_message_to_gm(wisp_server_name, battle_config.hack_info_GM_level, message_to_gm);
				sprintf(message_to_gm, msg_txt(539), sd->status.inventory[idx].amount, sd->status.inventory[idx].nameid, sd->status.inventory[idx].amount - inventory[idx].amount); // This player has %d of a kind of item (id: %d), and try to trade %d of them.
				intif_wis_message_to_gm(wisp_server_name, battle_config.hack_info_GM_level, message_to_gm);
				// if we block people
				if (battle_config.ban_hack_trade < 0) {
					chrif_char_ask_name(-1, sd->status.name, 1, 0, 0, 0, 0, 0, 0); // type: 1 - block
					clif_setwaitclose(sd->fd); // forced to disconnect because of the hack
					// message about the ban
					sprintf(message_to_gm, msg_txt(540)); //  This player has been definitivly blocked.
				// if we ban people
				} else if (battle_config.ban_hack_trade > 0) {
					chrif_char_ask_name(-1, sd->status.name, 2, 0, 0, 0, 0, battle_config.ban_hack_trade, 0); // type: 2 - ban (year, month, day, hour, minute, second)
					clif_setwaitclose(sd->fd); // forced to disconnect because of the hack
					// message about the ban
					sprintf(message_to_gm, msg_txt(507), battle_config.ban_spoof_namer); //  This player has been banned for %d minute(s).
				} else {
					// message about the ban
					sprintf(message_to_gm, msg_txt(508)); //  This player hasn't been banned (Ban option is disabled).
				}
				intif_wis_message_to_gm(wisp_server_name, battle_config.hack_info_GM_level, message_to_gm);
				return 1;
			}
		}

	return 0;
}

/*==========================================
 * Check here if we can add item in inventory (against full inventory)
 *------------------------------------------
 */
int trade_check(struct map_session_data *sd) {
	struct item inventory[MAX_INVENTORY];
	struct item inventory2[MAX_INVENTORY];
	struct item_data *data;
	struct map_session_data *target_sd;
	int trade_i, i, amount;

	target_sd = map_id2sd(sd->trade_partner);

	// get inventory of player
	memcpy(&inventory, &sd->status.inventory, sizeof(struct item) * MAX_INVENTORY);
	memcpy(&inventory2, &target_sd->status.inventory, sizeof(struct item) * MAX_INVENTORY);

	// check free slot in both inventory
	for(trade_i = 0; trade_i < 10; trade_i++) {
		amount = sd->deal_item_amount[trade_i];
		if (amount > 0) {
			int n = sd->deal_item_index[trade_i] - 2;
			// check quantity
			if (amount > inventory[n].amount)
				amount = inventory[n].amount;
			if (amount > 0) {
				data = itemdb_search(inventory[n].nameid);
				i = MAX_INVENTORY;
				// check for non-equipement item
				if (!itemdb_isequip2(data)) {
					for(i = 0; i < MAX_INVENTORY; i++)
						if (inventory2[i].nameid == inventory[n].nameid &&
							inventory2[i].card[0] == inventory[n].card[0] && inventory2[i].card[1] == inventory[n].card[1] &&
							inventory2[i].card[2] == inventory[n].card[2] && inventory2[i].card[3] == inventory[n].card[3]) {
							if (inventory2[i].amount + amount > MAX_AMOUNT) {
								clif_displaymessage(sd->fd, msg_txt(592)); // Trade can not be done, because one of your doesn't have enough free slots in its inventory.
								clif_displaymessage(target_sd->fd, msg_txt(592)); // Trade can not be done, because one of your doesn't have enough free slots in its inventory.
								return 0;
							}
							inventory2[i].amount += amount;
							inventory[n].amount -= amount;
							if (inventory[n].amount <= 0)
								memset(&inventory[n], 0, sizeof(struct item));
							break;
						}
				}
				// check for equipement
				if (i == MAX_INVENTORY) {
					for(i = 0; i < MAX_INVENTORY; i++) {
						if (inventory2[i].nameid == 0) {
							memcpy(&inventory2[i], &inventory[n], sizeof(struct item));
							inventory2[i].amount = amount;
							inventory[n].amount -= amount;
							if (inventory[n].amount <= 0)
								memset(&inventory[n], 0, sizeof(struct item));
							break;
						}
					}
					if (i == MAX_INVENTORY) {
						clif_displaymessage(sd->fd, msg_txt(592)); // Trade can not be done, because one of your doesn't have enough free slots in its inventory.
						clif_displaymessage(target_sd->fd, msg_txt(592)); // Trade can not be done, because one of your doesn't have enough free slots in its inventory.
						return 0;
					}
				}
			}
		}
		amount = target_sd->deal_item_amount[trade_i];
		if (amount > 0) {
			int n = target_sd->deal_item_index[trade_i] - 2;
			// check quantity
			if (amount > inventory2[n].amount)
				amount = inventory2[n].amount;
			if (amount > 0) {
				// search if it's possible to add item (for full inventory)
				data = itemdb_search(inventory2[n].nameid);
				i = MAX_INVENTORY;
				if (!itemdb_isequip2(data)) {
					for(i = 0; i < MAX_INVENTORY; i++)
						if (inventory[i].nameid == inventory2[n].nameid &&
							inventory[i].card[0] == inventory2[n].card[0] && inventory[i].card[1] == inventory2[n].card[1] &&
							inventory[i].card[2] == inventory2[n].card[2] && inventory[i].card[3] == inventory2[n].card[3]) {
							if (inventory[i].amount + amount > MAX_AMOUNT) {
								clif_displaymessage(sd->fd, msg_txt(592)); // Trade can not be done, because one of your doesn't have enough free slots in its inventory.
								clif_displaymessage(target_sd->fd, msg_txt(592)); // Trade can not be done, because one of your doesn't have enough free slots in its inventory.
								return 0;
							}
							inventory[i].amount += amount;
							inventory2[n].amount -= amount;
							if (inventory2[n].amount <= 0)
								memset(&inventory2[n], 0, sizeof(struct item));
							break;
						}
				}
				if (i == MAX_INVENTORY) {
					for(i = 0; i < MAX_INVENTORY; i++) {
						if (inventory[i].nameid == 0) {
							memcpy(&inventory[i], &inventory2[n], sizeof(struct item));
							inventory[i].amount = amount;
							inventory2[n].amount -= amount;
							if (inventory2[n].amount <= 0)
								memset(&inventory2[n], 0, sizeof(struct item));
							break;
						}
					}
					if (i == MAX_INVENTORY) {
						clif_displaymessage(sd->fd, msg_txt(592)); // Trade can not be done, because one of your doesn't have enough free slots in its inventory.
						clif_displaymessage(target_sd->fd, msg_txt(592)); // Trade can not be done, because one of your doesn't have enough free slots in its inventory.
						return 0;
					}
				}
			}
		}
	}

	return 1;
}

/*==========================================
 * アイテム追加
 *------------------------------------------
 */
void trade_tradeadditem(struct map_session_data *sd, int idx, int amount) { // S 0x00e8 <index>.w <amount>.l
	struct map_session_data *target_sd;
	int trade_i;
	int trade_weight = 0;
	int c, nameid;

//	nullpo_retv(sd); // checked before to call function
	if ((target_sd = map_id2sd(sd->trade_partner)) == NULL || sd->deal_locked > 1)
		return; //Can't add stuff.

	if (idx < 2 || idx >= MAX_INVENTORY + 2) {
		if (idx == 0) {
			if (amount > 0 && amount <= MAX_ZENY && amount <= sd->status.zeny && // check amount
				(target_sd->status.zeny + amount) <= MAX_ZENY) { // Fix positive overflow
				sd->deal_zeny = amount;
				clif_tradeadditem(sd, target_sd, 0, amount);
			} else {
				if (amount != 0) {
					trade_tradecancel(sd);
					return;
				}
			}
		}
	} else if (amount > 0 && amount <= sd->status.inventory[idx-2].amount) {
		for(trade_i = 0; trade_i < 10; trade_i++) {
			nameid = sd->inventory_data[idx-2]->nameid;
			if (!itemdb_cantrade(nameid, sd->GM_level, target_sd->GM_level) ||	//Can't trade
				(pc_get_partner(sd) != target_sd && itemdb_canonlypartnertrade(nameid, sd->GM_level, target_sd->GM_level))) {	//Can't partner-trade
				clif_tradeitemok(sd, idx, 1); // fail to add item -- the player was over weighted.
				amount = 0;
				break;
			}
			else if (sd->deal_item_amount[trade_i] == 0) {
				trade_weight += sd->inventory_data[idx-2]->weight * amount;
				if (target_sd->weight + trade_weight > target_sd->max_weight) {
					clif_tradeitemok(sd, idx, 1); // fail to add item -- the player was over weighted.
					amount = 0;
				} else {
					for(c = 0; c == trade_i - 1; c++) { // re-deal exploit protection [Valaris]
						if (sd->deal_item_index[c] == idx) {
							trade_tradecancel(sd);
							return;
						}
					}
					sd->deal_item_index[trade_i] = idx;
					sd->deal_item_amount[trade_i] += amount;
					if (impossible_trade_check(sd)) { // check exploit (trade more items that you have)
						trade_tradecancel(sd);
						return;
					}
					clif_tradeitemok(sd, idx, 0); // success to add item
					clif_tradeadditem(sd, target_sd, idx, amount);
				}
				break;
			} else {
				trade_weight += sd->inventory_data[sd->deal_item_index[trade_i]-2]->weight * sd->deal_item_amount[trade_i];
			}
		}
	}

	return;
}

/*==========================================
 * アイテム追加完了(ok押し)
 *------------------------------------------
 */
void trade_tradeok(struct map_session_data *sd) {
	struct map_session_data *target_sd;
	int trade_i, idx;

//	nullpo_retv(sd); // checked before to call function

	// check items
	for(trade_i = 0; trade_i < 10; trade_i++) {
		idx = sd->deal_item_index[trade_i] - 2;
		if ((idx >= 0 && sd->deal_item_amount[trade_i] > sd->status.inventory[idx].amount) ||
		    sd->deal_item_amount[trade_i] < 0) {
			trade_tradecancel(sd);
			return;
		}
	}

	// check exploit (trade more items that you have)
	if (impossible_trade_check(sd)) {
		trade_tradecancel(sd);
		return;
	}

	// check zeny
	if (sd->deal_zeny < 0 || sd->deal_zeny > MAX_ZENY || sd->deal_zeny > sd->status.zeny) { // check amount
		trade_tradecancel(sd);
		return;
	}

	if ((target_sd = map_id2sd(sd->trade_partner)) != NULL) {
		sd->deal_locked = 2; // 0: no trade, 1: Deal request accepted, 2: Trade accepted, 3: Trade valided (button 'Ok' pressed)
		clif_tradeitemok(sd, 0, 0);
		clif_tradedeal_lock(sd, 0);
		clif_tradedeal_lock(target_sd, 1);
	}

	return;
}

/*==========================================
 * 取引キャンセル
 *------------------------------------------
 */
void trade_tradecancel(struct map_session_data *sd) {
	struct map_session_data *target_sd;
	int trade_i;

//	nullpo_retv(sd); // checked before to call function

	if ((target_sd = map_id2sd(sd->trade_partner)) != NULL) {
		for(trade_i = 0; trade_i < 10; trade_i++) { // give items back (only virtual)
			if (sd->deal_item_amount[trade_i] != 0) {
				clif_additem(sd, sd->deal_item_index[trade_i] - 2, sd->deal_item_amount[trade_i], 0); // 0: you got...
				sd->deal_item_index[trade_i] = 0;
				sd->deal_item_amount[trade_i] = 0;
			}
			if (target_sd->deal_item_amount[trade_i] != 0) {
				clif_additem(target_sd, target_sd->deal_item_index[trade_i] - 2, target_sd->deal_item_amount[trade_i], 0); // 0: you got...
				target_sd->deal_item_index[trade_i] = 0;
				target_sd->deal_item_amount[trade_i] = 0;
			}
		}
		if (sd->deal_zeny) {
			clif_updatestatus(sd, SP_ZENY);
			sd->deal_zeny = 0;
		}
		if (target_sd->deal_zeny) {
			clif_updatestatus(target_sd, SP_ZENY);
			target_sd->deal_zeny = 0;
		}
		sd->deal_locked = 0; // 0: no trade, 1: Deal request accepted, 2: Trade accepted, 3: Trade valided (button 'Ok' pressed)
		sd->trade_partner = 0;
		target_sd->deal_locked = 0; // 0: no trade, 1: Deal request accepted, 2: Trade accepted, 3: Trade valided (button 'Ok' pressed)
		target_sd->trade_partner = 0;
		clif_tradecancelled(sd);
		clif_tradecancelled(target_sd);
	}

	return;
}

/*==========================================
 * 取引許諾(trade押し)
 *------------------------------------------
 */
void trade_tradecommit(struct map_session_data *sd) {
	struct map_session_data *target_sd;
	int trade_i;
	int flag;

//	nullpo_retv(sd); // checked before to call function

	if ((target_sd = map_id2sd(sd->trade_partner)) != NULL) {
		if (sd->deal_locked > 1 && target_sd->deal_locked > 1) { // 0: no trade, 1: Deal request accepted, 2: Trade accepted, 3: Trade valided (button 'Ok' pressed)
			sd->deal_locked = 3; // 0: no trade, 1: Deal request accepted, 2: Trade accepted, 3: Trade valided (button 'Ok' pressed)
			if (target_sd->deal_locked == 3) { // the other one pressed 'trade' too
				// check exploit (trade more items that you have)
				if (impossible_trade_check(sd)) {
					trade_tradecancel(sd);
					return;
				}
				// check exploit (trade more items that you have)
				if (impossible_trade_check(target_sd)) {
					trade_tradecancel(target_sd);
					return;
				}
				// check zenys value against hackers
				if (sd->deal_zeny >= 0 && sd->deal_zeny <= MAX_ZENY && sd->deal_zeny <= sd->status.zeny && // check amount
				    (target_sd->status.zeny + sd->deal_zeny) <= MAX_ZENY && // Fix positive overflow
				    target_sd->deal_zeny >= 0 && target_sd->deal_zeny <= MAX_ZENY && target_sd->deal_zeny <= target_sd->status.zeny && // check amount
				    (sd->status.zeny + target_sd->deal_zeny) <= MAX_ZENY) { // Fix positive overflow

					// check for full inventory (can not add traded items)
					if (!trade_check(sd)) { // check the both players
						trade_tradecancel(sd);
						return;
					}

					// trade is accepted
					log_trade(sd, target_sd);
					for(trade_i = 0; trade_i < 10; trade_i++) {
						if (sd->deal_item_amount[trade_i] > 0) {
							int n = sd->deal_item_index[trade_i] - 2;

							if (sd->status.inventory[n].amount < sd->deal_item_amount[trade_i])
								sd->deal_item_amount[trade_i] = sd->status.inventory[n].amount;

							if (sd->deal_item_amount[trade_i] > 0) {
								flag = pc_additem(target_sd, &sd->status.inventory[n], sd->deal_item_amount[trade_i]);
								if (flag == 0)
									pc_delitem(sd, n, sd->deal_item_amount[trade_i], 1);
								else
									clif_additem(sd, n, sd->deal_item_amount[trade_i], 0); // 0: you got...
								sd->deal_item_index[trade_i] = 0;
								sd->deal_item_amount[trade_i] = 0;
							}
						}
						if (target_sd->deal_item_amount[trade_i] > 0) {
							int n = target_sd->deal_item_index[trade_i] - 2;

							if (target_sd->status.inventory[n].amount < target_sd->deal_item_amount[trade_i])
								target_sd->deal_item_amount[trade_i] = target_sd->status.inventory[n].amount;

							if (target_sd->deal_item_amount[trade_i] > 0) {
								flag = pc_additem(sd, &target_sd->status.inventory[n], target_sd->deal_item_amount[trade_i]);
								if (flag == 0)
									pc_delitem(target_sd, n, target_sd->deal_item_amount[trade_i], 1);
								else
									clif_additem(target_sd, n, target_sd->deal_item_amount[trade_i], 0); // 0: you got...
								target_sd->deal_item_index[trade_i] = 0;
								target_sd->deal_item_amount[trade_i] = 0;
							}
						}
					}
					if (sd->deal_zeny) {
						sd->status.zeny -= sd->deal_zeny;
						target_sd->status.zeny += sd->deal_zeny;
					}
					if (target_sd->deal_zeny) {
						target_sd->status.zeny -= target_sd->deal_zeny;
						sd->status.zeny += target_sd->deal_zeny;
					}
					if (sd->deal_zeny || target_sd->deal_zeny) {
						clif_updatestatus(sd, SP_ZENY);
						sd->deal_zeny = 0;
						clif_updatestatus(target_sd, SP_ZENY);
						target_sd->deal_zeny = 0;
					}
					sd->deal_locked = 0; // 0: no trade, 1: Deal request accepted, 2: Trade accepted, 3: Trade valided (button 'Ok' pressed)
					sd->trade_partner = 0;
					target_sd->deal_locked = 0; // 0: no trade, 1: trade accepted, 2: trade valided (button 'Ok' pressed)
					target_sd->trade_partner = 0;
					clif_tradecompleted(sd, 0);
					clif_tradecompleted(target_sd, 0);
					// save both player to avoid crash: they always have no advantage/disadvantage between the 2 players
					chrif_save(sd); // do pc_makesavestatus and save storage + account_reg/account_reg2 too
					chrif_save(target_sd); // do pc_makesavestatus and save storage + account_reg/account_reg2 too
				// zeny value was modified!!!! hacker with packet modified
				} else {
					trade_tradecancel(sd);
				}
			}
		}
	}

	return;
}
