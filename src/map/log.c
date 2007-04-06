// $Id: npc.c 460 2005-10-27 22:22:48Z Yor $
#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h> // isspace
#include <string.h>
#include <math.h>
#include <time.h>
#include <stdint.h>

#include "../common/socket.h"
#include "../common/timer.h"
#include "../common/malloc.h"
#include "../common/lock.h"
#include "../common/db.h"

#include "map.h"
#include "clif.h"
#include "chrif.h"
#include "itemdb.h"
#include "pc.h"
#include "script.h"
#include "storage.h"
#include "mob.h"
#include "npc.h"
#include "pet.h"
#include "intif.h"
#include "skill.h"
#include "chat.h"
#include "battle.h"
#include "party.h"
#include "guild.h"
#include "atcommand.h"
#include "status.h"
#include "log.h"

#ifdef MEMWATCH
#include "memwatch.h"
#endif

struct Log_Config log_config;

//log_config->refine_level = 0;

short log_refine_level  = 0;
short log_npcsell_level = 0;
short log_pcdrop_level  = 0;
short log_pcpick_level  = 0;


/*===========================================
 * pick by pc logs   [zug]
 */
int log_pcpick(struct map_session_data *sd, struct item *item, int amount) {
	char log_mesg[MAX_MSG_LEN + 512], * itemname;
	unsigned char *sin_addr;

	//if (zeny < log_pcdrop_level)
	//	return 0;
	//format : MM/DD/YYYY HH:MM:SS.mss: Charname [charid(lvl:gmlevel)] (ip:71.137.161.25)refine item +n () : success
	sin_addr = (unsigned char *)&session[sd->fd]->client_addr.sin_addr;

	if (item != NULL) {
		itemname = log_itemgetname(item); // max length: 250
	} else {
		CALLOC(itemname, char, 18);
		sprintf(itemname, "null item (hack?)");
	}

	sprintf(log_mesg, "%s [%d(%s:%d)] (ip:%d.%d.%d.%d) pick : %d %s" RETCODE,
	        sd->status.name, (int)sd->status.account_id, msg_txt(597), sd->GM_level,
	        sin_addr[0], sin_addr[1], sin_addr[2], sin_addr[3], amount, itemname);

	FREE(itemname);
	// send log to inter-server
	intif_send_log(8, log_mesg);

	return 1;
}


/*===========================================
 * drop by pc logs   [zug]
 */
int log_pcdrop(struct map_session_data *sd, int idx, int amount) {
	char log_mesg[MAX_MSG_LEN + 512], *itemname;
	unsigned char *sin_addr;

	//if (zeny < log_pcdrop_level) return 0;
	//format : MM/DD/YYYY HH:MM:SS.mss: Charname [charid(lvl:gmlevel)] (ip:71.137.161.25)refine item +n () : success
	sin_addr = (unsigned char *) & session[sd->fd]->client_addr.sin_addr;

	if (sd->inventory_data[idx] != NULL) {
		itemname = log_itemgetname(&sd->status.inventory[idx]); // max length: 250
	} else {
		CALLOC(itemname, char, 40);
		sprintf(itemname, "inventory slot %d was empty (hack?)", idx);
	}

	sprintf(log_mesg, "%s [%d(%s:%d)] (ip:%d.%d.%d.%d) drop : %d %s" RETCODE,
	        sd->status.name, (int)sd->status.account_id, msg_txt(597), sd->GM_level,
	        sin_addr[0], sin_addr[1], sin_addr[2], sin_addr[3], amount, itemname);
	
	FREE(itemname);
	// send log to inter-server
	intif_send_log(7, log_mesg);

	return 1;
}


/* ========================================
 * NPC sales logs	[zug]
 */
int log_npcsell(struct map_session_data *sd, int n, unsigned short *item_list, int zeny) {
	char *log_mesg, * itemname;
	int log_mesglen = 0, base_price, idx, i, log_mesg_allocd = MAX_MSG_LEN + 512;
	unsigned char *sin_addr;
	struct item_data * item_data;

	MALLOC(log_mesg, char, log_mesg_allocd);

	if (zeny < log_npcsell_level)
		return 0;
	//format : MM/DD/YYYY HH:MM:SS.mss: Charname [charid(lvl:gmlevel)] (ip:71.137.161.25)refine item +n () : success
	sin_addr = (unsigned char *) &session[sd->fd]->client_addr.sin_addr;

	log_mesglen = sprintf(log_mesg, "%s [%d(%s:%d)] (ip:%d.%d.%d.%d) sold for %d zeny:" RETCODE,
	                      sd->status.name, (int)sd->status.account_id, msg_txt(597), sd->GM_level,
	                      sin_addr[0], sin_addr[1], sin_addr[2], sin_addr[3], zeny);

	for(i = 0; i < n; i++) {
		idx = item_list[i * 2] - 2;
		if (sd->inventory_data[idx] != NULL) {
			itemname = log_itemgetname(&sd->status.inventory[idx]); // max length: 250
			base_price = 0;
			if (sd->status.inventory[idx].nameid > 0 && (item_data = itemdb_search(sd->status.inventory[idx].nameid)) != NULL)
				base_price = item_data->value_sell;
			if (log_mesglen + 128 > log_mesg_allocd) {
				/* less than 128 bytes remaining, allocate 512 other bytes */
				log_mesg_allocd += 512;
				REALLOC(log_mesg, char, log_mesg_allocd);
			}
			log_mesglen += sprintf(log_mesg + log_mesglen, " - %d %s (base price %dz)" RETCODE, item_list[i*2+1], itemname, base_price);
			FREE(itemname);
		}
	}
	// send log to inter-server
	intif_send_log(6, log_mesg);
	FREE(log_mesg);

	return 1;
}

/* =========================================
 *  Log refine [zug]
 */

int log_refine(struct map_session_data *sd, int equipid, unsigned short state) { //state : 0 refine failed, 1 refine sucess
	char log_mesg[MAX_MSG_LEN + 512];
	char * itemname;
	int log_mesglen = 0;
	unsigned char *sin_addr;

	// get item info and refine level, decide to log or not
	if ((sd->status.inventory[equipid].refine + 1) <= log_config.refine_level)
		return 0;

	//format : MM/DD/YYYY HH:MM:SS.mss: Charname [charid(lvl:gmlevel)] (ip:71.137.161.25)refine item +n () : success
	sin_addr = (unsigned char *) &session[sd->fd]->client_addr.sin_addr;

	itemname = log_itemgetname(&sd->status.inventory[equipid]); // max length: 250
	log_mesglen = sprintf(log_mesg, "%s [%d(%s:%d)] (ip:%d.%d.%d.%d) refined a %s:" RETCODE,
	                      sd->status.name, (int)sd->status.account_id, msg_txt(597), sd->GM_level,
	                      sin_addr[0], sin_addr[1], sin_addr[2], sin_addr[3], itemname);

	//refine level & state
	log_mesglen += sprintf(log_mesg + log_mesglen, (state) ? "success" RETCODE : "failed" RETCODE);

	// send log to inter-server
	intif_send_log(5, log_mesg);

	FREE(itemname);

	return 1;
}

/*========================
 * format item name [zug]
 */
char * log_itemgetname(struct item *item) {
	// longest name: (24x2x4)(cart names)+4(cart #)+5(amount)+46(other) = 247
	char * name;

	int j, name_len = 0, counter= 0;
	struct item_data * item_data, * card;

	CALLOC(name, char, 250);

	if ((item_data = itemdb_search(item->nameid)) != NULL) {
		name_len = sprintf(name, "%s", item_data->jname);
	} else {
		name_len = sprintf(name, "Unknow item");
	}

	name_len += (item->refine)
	            ? sprintf(name + name_len, " %+d (id: %d)", item->refine, item->nameid)
	            : sprintf(name + name_len, " (id: %d)", item->nameid);

	//cards ?
	for (j = 0; j < item_data->slot; j++) {
		if (item->card[j] > 0 && ((card = itemdb_search(item->card[j])) != NULL)) {
			if (counter == 0)
				name_len += sprintf(name + name_len, "(cards: %s (%d)",card->jname, card->nameid);
			else
				name_len += sprintf(name + name_len, ", %s (%d)", card->jname, card->nameid);
			counter++;
		}
	}
	if (counter != 0) {
		name[strlen(name)] = ')';
		name[strlen(name)+1] = '\0';
		name_len++;
	}

	return name;
}

