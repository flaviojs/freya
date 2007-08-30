// Copyright (c) Freya Development Team - Licensed under GNU GPL
// For more information, see License.txt in the main folder

#include <config.h>

#include <stdio.h>
#include <string.h>

#include "../common/socket.h"
#include "../common/malloc.h"

#include "clif.h"
#include "itemdb.h"
#include "map.h"
#include "vending.h"
#include "pc.h"
#include "skill.h"
#include "battle.h"
#include "nullpo.h"
#include "chrif.h"
#include "intif.h"
#include "atcommand.h"

unsigned char log_vending_level = 0;

/*==========================================
 *
 *------------------------------------------
*/
void vending_closevending(struct map_session_data *sd) {

	sd->vender_id = 0;

	// If player close shop (damage, auto-close, all items sold), reset disconnection timer to not disconnect the player just after last selled item
	sd->lastpackettime = time(NULL); // For disconnection if player is inactive

	clif_closevendingboard(&sd->bl, 0);

	return;
}

/*==========================================
 *
 *------------------------------------------
*/
void vending_vendinglistreq(struct map_session_data *sd, int id) {

	struct map_session_data *vsd;

	if ((vsd = map_id2sd(id)) == NULL || !vsd->state.auth)
		return;
	if (vsd->vender_id == 0)
		return;

	clif_vendinglist(sd, vsd);

	return;
}

/*==========================================
 *
 *------------------------------------------
*/
void vending_purchasereq(struct map_session_data *sd, short len, int id, char *p) {

	int i, j, w, new = 0, blank, vend_list[MAX_VENDING];
	double z;
	unsigned short amount;
	short idx;
	struct map_session_data *vsd;
	struct vending vending[MAX_VENDING]; // Against duplicate packets

	vsd = map_id2sd(id);
	if (vsd == NULL)
		return;
	if (vsd->vender_id == 0)
		return;
	if (vsd->vender_id == sd->bl.id)
		return;

	// Check number of buying items
	if (len < 8 + 4 || len > 8 + 4 * MAX_VENDING) {
		clif_buyvending(sd, 0, 32767, 4); //Not enough quantity (index and amount are unknown)
		return;
	}

	blank = pc_inventoryblank(sd);

	// Duplicate item in vending to check hacker with multiple packets
	memcpy(&vending, &vsd->vending, sizeof(struct vending) * MAX_VENDING); // Copy vending list

	// Some checks
	z = 0.;
	w = 0;
	for(i = 0; 8 + 4 * i < len; i++) {
		amount = *(unsigned short*)(p + 4 * i);
		idx = *(short*)(p + 2 + 4 * i) - 2;

		if (amount <= 0)
			return;

		// Check of index
		if (idx < 0 || idx >= MAX_CART)
			return;

		for(j = 0; j < vsd->vend_num; j++) {
			if (vsd->vending[j].index == idx) {
				vend_list[i] = j;
				break;
			}
		}
		if (j == vsd->vend_num)
			return; // ”„‚èØ‚ê

		z += ((double)vsd->vending[j].value * (double)amount);
		if (z > (double)sd->status.zeny || z < 0. || z > (double)MAX_ZENY) {
			clif_buyvending(sd, idx, amount, 1); // You don't have enough zeny
			return; // zeny•s‘«
		}
		if (z + (double)vsd->status.zeny > (double)MAX_ZENY) {
			clif_buyvending(sd, idx, vsd->vending[j].amount, 4); // Not enough quantity
			return; // zeny•s‘«
		}
		w += itemdb_weight(vsd->status.cart[idx].nameid) * amount;
		if (w + sd->weight > sd->max_weight) {
			clif_buyvending(sd, idx, amount, 2); // You cannot buy, because overweight
			return;
		}
		// If they try to add packets (example: get twice or more 2 apples if marchand has only 3 apples).
		// Here, we check cumulative amounts
		if (vending[j].amount < amount) { // Send more quantity is not a hack (an other player can have buy items just before)
			clif_buyvending(sd, idx, vsd->vending[j].amount, 4); // Not enough quantity
			return;
		} else
			vending[j].amount -= amount;

		switch(pc_checkadditem(sd, vsd->status.cart[idx].nameid, amount)) {
		case ADDITEM_EXIST:
			break;
		case ADDITEM_NEW:
			new++;
			if (new > blank)
				return;
			break;
		case ADDITEM_OVERAMOUNT:
			return;
		}
	}

  { // For logging variables
	char tmpstr[8192]; // max lines: trade (1) + players names (2) + 10x2 items + total (2) = 25
	                   // longest line: (24x2x4)(cart names)+4(cart #)+5(amount)+46(other) = 247
	                   // 247 * 25 = 6175 (8192)
	int tmpstr_len;
	unsigned char *sin_addr;
	struct item_data *item_data, *item_temp;
	int counter;

	// log title
	tmpstr_len = sprintf(tmpstr, msg_txt(614), (int)z); // Vending done for %d z
	tmpstr_len += sprintf(tmpstr + tmpstr_len, ":" RETCODE);
	// log vendor
	sin_addr = (unsigned char *)&session[vsd->fd]->client_addr.sin_addr;
	tmpstr_len += sprintf(tmpstr + tmpstr_len, "* %s: %s [%d(%s:%d)] (ip:%d.%d.%d.%d)." RETCODE, msg_txt(615), vsd->status.name, vsd->status.account_id, msg_txt(597), vsd->GM_level, sin_addr[0], sin_addr[1], sin_addr[2], sin_addr[3]); // Vendor, lvl
	// log buyer
	sin_addr = (unsigned char *)&session[sd->fd]->client_addr.sin_addr;
	tmpstr_len += sprintf(tmpstr + tmpstr_len, "* %s: %s [%d(%s:%d)] (ip:%d.%d.%d.%d)." RETCODE, msg_txt(616), sd->status.name, sd->status.account_id, msg_txt(597), sd->GM_level, sin_addr[0], sin_addr[1], sin_addr[2], sin_addr[3]); // Buyer, lvl

	if(pc_payzeny(sd, (int)z) != 0) {
		WPACKETW(0) = 0xca;
		WPACKETB(2) = 1; // 0: The deal has successfully completed., 1: You dont have enough zeny., 2: you are overcharged!, 3: You are over your weight limit.
		SENDPACKET(sd->fd, 3/*packet_len_table[0xca]*/);

		return;
	}
	pc_getzeny(vsd, (int)z);
	for(i = 0; 8 + 4 * i < len; i++) {
		amount = *(unsigned short*)(p + 4 * i);
		idx = *(short*)(p + 2 + 4 * i) - 2;

		// log item
		if (amount > 0 && (item_data = itemdb_search(vsd->status.cart[idx].nameid)) != NULL) {
			if (vsd->status.cart[idx].refine)
				tmpstr_len += sprintf(tmpstr + tmpstr_len, " - %d %s %+d (%s %+d, id: %d), ", amount, item_data->name, vsd->status.cart[idx].refine, item_data->jname, vsd->status.cart[idx].refine, vsd->status.cart[idx].nameid);
			else
				tmpstr_len += sprintf(tmpstr + tmpstr_len, " - %d %s (%s, id: %d), ", amount, item_data->name, item_data->jname, vsd->status.cart[idx].nameid);
			if (amount > 1)
				tmpstr_len += sprintf(tmpstr + tmpstr_len, msg_txt(617), vsd->vending[vend_list[i]].value, amount, vsd->vending[vend_list[i]].value * amount); // price: %d x %d z = %d z.
			else
				tmpstr_len += sprintf(tmpstr + tmpstr_len, msg_txt(618), vsd->vending[vend_list[i]].value); // price: %d z.
			tmpstr_len += sprintf(tmpstr + tmpstr_len, RETCODE);
			counter = 0;
			for (j = 0; j < item_data->slot; j++) {
				if (vsd->status.cart[idx].card[j] && (item_temp = itemdb_search(vsd->status.cart[idx].card[j])) != NULL) {
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
		}

		// vending item
		pc_additem(sd, &vsd->status.cart[idx], amount);
		vsd->vending[vend_list[i]].amount -= amount;
		pc_cart_delitem(vsd, idx, amount, 0);
		clif_vendingreport(vsd, idx, amount);
	}

	// send log to inter-server
	tmpstr_len += sprintf(tmpstr + tmpstr_len, RETCODE);
	if (vsd->GM_level >= log_vending_level || sd->GM_level >= log_vending_level)
		intif_send_log(LOG_VENDING, tmpstr); // 0x3008 <packet_len>.w <log_type>.B <message>.?B (types: 0 GM commands, 1: Trades, 2: Scripts, 3: Vending)
  }

	// save both players to avoid crash: they always have no advantage/disadvantage between the 2 players
	chrif_save(sd); // do pc_makesavestatus and save storage + account_reg/account_reg2 too
	chrif_save(vsd); // do pc_makesavestatus and save storage + account_reg/account_reg2 too

	return;
}

/*==========================================
 *
 *------------------------------------------
*/
void vending_openvending(struct map_session_data *sd, unsigned short len, char *message, unsigned char flag, char *p) {

	char *shop_title;
	int shop_title_len;
	int vending_skill_lvl;
	int i, j;

	vending_skill_lvl = pc_checkskill(sd, MC_VENDING);
	if (vending_skill_lvl < 1 || !pc_iscarton(sd)) {
		clif_skill_fail(sd, MC_VENDING, 0, 0);
		return;
	}

	CALLOC(shop_title, char, 80 + 1); // (title) + NULL
	strncpy(shop_title, message, 80);
	shop_title_len = strlen(shop_title);
	// check bad words and shopname len
	if (shop_title[0] == '\0' || check_bad_word(shop_title, shop_title_len, sd)) {
		FREE(shop_title);
		return;
	}

	if (flag) {
		len -= 85;
		// check number of items in shop
		if (len < 8 || len > 8 * MAX_VENDING || len > 8 * (2 + vending_skill_lvl)) {
			clif_skill_fail(sd, MC_VENDING, 0, 0);
			FREE(shop_title);
			return;
		}
		// check values before to continue
		for(i = 0; (8 * i) < len; i++) {
			short idx;
			idx = *(short*)(p + 8 * i) - 2;
			if (idx < 0 || idx >= MAX_CART || // index
			    (*(unsigned short*)(p + 2 + 8 * i)) <= 0 /*|| // quantity
			    (*(unsigned int*)(p + 4 + 8 * i)) < 0*/) { // price (price == 0 -> max_value)
				clif_skill_fail(sd, MC_VENDING, 0, 0);
				FREE(shop_title);
				return;
			}
		}
		memset(&sd->vending[0], 0, sizeof(struct vending) * MAX_VENDING);
//		for(i = 0; (8 * i) < len && i < MAX_VENDING; i++) { // Check on MAX_VENDING not more necessary (check with 'len')
		for(i = 0, j = 0; (8 * j) < len; i++, j++) {
			sd->vending[i].index = *(short*)(p + 8 * j) - 2;
			if (sd->vending[i].index < 0 || sd->vending[i].index >= MAX_CART ||
				(!itemdb_cantrade(sd->status.cart[sd->vending[i].index].nameid, sd->GM_level, sd->GM_level) ||
				itemdb_canonlypartnertrade(sd->status.cart[sd->vending[i].index].nameid, sd->GM_level, sd->GM_level))) {
				i--; // Preserve the vending index, skip to the next item.
				continue;
			}
			sd->vending[i].amount = *(unsigned short*)(p + 2 + 8 * j);
			sd->vending[i].value = *(unsigned int*)(p + 4 + 8 * j);
			if (sd->vending[i].value > (unsigned int)battle_config.vending_max_value)
				sd->vending[i].value = (unsigned int)battle_config.vending_max_value;
			else if (sd->vending[i].value == 0)
				sd->vending[i].value = battle_config.vending_max_value; // Auto set to max value
			if (pc_cartitem_amount(sd, sd->vending[i].index, sd->vending[i].amount) < 0) { //  || sd->vending[i].value < 0: it's unsigned int value. never <0
				clif_skill_fail(sd, MC_VENDING, 0, 0);
				FREE(shop_title);
				return;
			}
		}
		if (i != j) {	
			if (i == 0) { // All items were not allowed to be vended
				clif_skill_fail(sd, MC_VENDING, 0, 0);
				FREE(shop_title);
				return;
			}
			clif_displaymessage (sd->fd, msg_txt(287));
		}
		sd->vender_id = sd->bl.id;
		sd->vend_num = i;
		memset(sd->message, 0, sizeof(sd->message)); // 80 + NULL (shop title)
		strcpy(sd->message, shop_title); // 80 + NULL (shop title)
		if (clif_openvending(sd, sd->vender_id, sd->vending) > 0)
			clif_showvendingboard(&sd->bl, shop_title, 0); // 80 + NULL (shop title)
		else
			sd->vender_id = 0;
	}

	FREE(shop_title);

	return;
}
