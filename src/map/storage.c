// Copyright (c) Freya Development Team - Licensed under GNU GPL
// For more information, see License.txt in the main folder

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../common/socket.h"
#include "../common/db.h"
#include "itemdb.h"
#include "clif.h"
#include "chrif.h"
#include "intif.h"
#include "pc.h"
#include "storage.h"
#include "guild.h"
#include "atcommand.h"
#include "nullpo.h"
#include "../common/malloc.h"

#ifdef MEMWATCH
#include "memwatch.h"
#endif

static struct dbt *storage_db;
static struct dbt *guild_storage_db;

/*==========================================
 * 倉庫内アイテムソート
 *------------------------------------------
 */
int storage_comp_item(const void *_i1, const void *_i2) {
	struct item *i1 = (struct item *)_i1;
	struct item *i2 = (struct item *)_i2;

	if (i1->nameid == i2->nameid) {
		return 0;
	} else if (!(i1->nameid) || !(i1->amount)) {
		return 1;
	} else if (!(i2->nameid) || !(i2->amount)) {
		return -1;
	} else {
		return i1->nameid - i2->nameid;
	}
}

void sortage_sortitem(struct storage* stor) {
	nullpo_retv(stor);

	qsort(stor->storage, MAX_STORAGE, sizeof(struct item), storage_comp_item);
}

void sortage_gsortitem(struct guild_storage* gstor) {
	nullpo_retv(gstor);

	qsort(gstor->storage, MAX_GUILD_STORAGE, sizeof(struct item), storage_comp_item);
}

/*==========================================
 * 初期化とか
 *------------------------------------------
 */
int do_init_storage(void) // map.c::do_init()から呼ばれる
{
	storage_db=numdb_init();
	guild_storage_db=numdb_init();

	return 1;
}

int guild_storage_db_final(void *key, void *data, va_list ap) {
	FREE(data);

	return 0;
}

int storage_db_final(void *key, void *data, va_list ap) {
	FREE(data);

	return 0;
}
void do_final_storage(void) { // map.c::do_final()から呼ばれる
	if (storage_db)
		numdb_final(storage_db, storage_db_final);
	if (guild_storage_db)
		numdb_final(guild_storage_db, guild_storage_db_final);
}

struct storage *account2storage(int account_id) {
	struct storage *stor;

	stor=numdb_search(storage_db, account_id);
	if (stor == NULL) {
		CALLOC(stor, struct storage, 1);
		stor->account_id = account_id;
		numdb_insert(storage_db, stor->account_id, stor);
	}

	return stor;
}

// Just to ask storage, without creation
struct storage *account2storage2(int account_id) {
	return numdb_search(storage_db, account_id);
}

void storage_delete(int account_id) {
	struct storage *stor = numdb_search(storage_db, account_id);

	if (stor) {
		numdb_erase(storage_db, account_id);
		FREE(stor);
		//printf("storage_delete called for account %d - storage deleted.\n", account_id);
	} else {
		//printf("storage_delete called for account %d - storage not found.\n", account_id);
	}

	return;
}

/*==========================================
 * カプラ倉庫を開く
 *------------------------------------------
 */
int storage_storageopen(struct map_session_data *sd)
{
	struct storage *stor;

	nullpo_retr(0, sd);

	if((stor = numdb_search(storage_db,sd->status.account_id)) != NULL) {
		if (stor->storage_status == 0) { // not already opened
			stor->storage_status = 1;
			sd->state.storage_flag = 0;
			clif_storageitemlist(sd, stor);
			clif_storageequiplist(sd, stor);
			clif_updatestorageamount(sd, stor);
			return 0;
		}
	} else
		intif_request_storage(sd->status.account_id);

	return 1;
}

/*==========================================
 * カプラ倉庫へアイテム追加
 *------------------------------------------
 */
int storage_additem(struct map_session_data *sd, struct storage *stor, struct item *item_data, int amount)
{
	struct item_data *data;
	int i;

	nullpo_retr(1, sd);
	nullpo_retr(1, stor);
	nullpo_retr(1, item_data);

	if (item_data->nameid <= 0 || amount <= 0)
		return 1;
	nullpo_retr(1, data = itemdb_search(item_data->nameid));

	if (!itemdb_canstore(item_data->nameid, sd->GM_level)) { // check if item can be stored in storage
		clif_displaymessage (sd->fd, msg_txt(286));
		return 1;
	}

	i = MAX_STORAGE;
	if (!itemdb_isequip2(data)) {
		// 装備品ではないので、既所有品なら個数のみ変化させる
		for(i = 0; i < MAX_STORAGE; i++) {
			if (stor->storage[i].nameid == item_data->nameid &&
			    stor->storage[i].identify == item_data->identify &&
			    stor->storage[i].refine == item_data->refine &&
			    stor->storage[i].attribute == item_data->attribute &&
			    stor->storage[i].card[0] == item_data->card[0] &&
			    stor->storage[i].card[1] == item_data->card[1] &&
			    stor->storage[i].card[2] == item_data->card[2] &&
			    stor->storage[i].card[3] == item_data->card[3]) {
				if (stor->storage[i].amount + amount > MAX_AMOUNT)
					return 1;
				stor->storage[i].amount += amount;
				clif_storageitemadded(sd, stor, i, amount);
				break;
			}
		}
	}
	if (i >= MAX_STORAGE) {
		// 装備品か未所有品だったので空き欄へ追加
		for(i = 0; i < MAX_STORAGE; i++) {
			if (stor->storage[i].nameid == 0) {
				memcpy(&stor->storage[i], item_data, sizeof(stor->storage[0]));
				stor->storage[i].amount = amount;
				stor->storage_amount++;
				clif_storageitemadded(sd,stor,i,amount);
				clif_updatestorageamount(sd,stor);
				break;
			}
		}
		if(i>=MAX_STORAGE)
			return 1;
	}

	return 0;
}

/*==========================================
 * カプラ倉庫アイテムを減らす
 *------------------------------------------
 */
int storage_delitem(struct map_session_data *sd, struct storage *stor, int n, int amount)
{
	nullpo_retr(1, sd);
	nullpo_retr(1, stor);

	if (stor->storage[n].nameid == 0 || stor->storage[n].amount < amount)
		return 1;

	stor->storage[n].amount -= amount;
	if (stor->storage[n].amount == 0) {
		memset(&stor->storage[n], 0, sizeof(stor->storage[0]));
		stor->storage_amount--;
		clif_updatestorageamount(sd, stor);
	}
	clif_storageitemremoved(sd, n, amount);

	sd->state.modified_storage_flag = 1; // 0: not modified, 1: modified, 2: modified and sended to char-server for saving

	return 0;
}

/*==========================================
 * カプラ倉庫へ入れる
 *------------------------------------------
 */
void storage_storageadd(struct map_session_data *sd, short idx, int amount) {
	struct storage *stor;

//	nullpo_retv(sd); // checked before to call function

	if ((stor = account2storage(sd->status.account_id)) != NULL) {
		if (stor->storage_amount <= MAX_STORAGE && stor->storage_status == 1) { // storage not full & storage open
//			if (idx >= 0 && idx < MAX_INVENTORY) { // valid index // checked before to call function
//				if (amount <= sd->status.inventory[idx].amount && amount > 0) { //valid amount // checked before to call function
					if (storage_additem(sd, stor, &sd->status.inventory[idx], amount) == 0)
						// remove item from inventory
						pc_delitem(sd, idx, amount, 0);
//				} // valid amount
//			} // valid index
		} // storage not full & storage open
	}

	return;
}

/*==========================================
 * カプラ倉庫から出す
 *------------------------------------------
 */
void storage_storageget(struct map_session_data *sd, short idx, int amount) {
	struct storage *stor;
	int flag;

//	nullpo_retv(sd); // checked before to call function

	if ((stor = account2storage(sd->status.account_id)) != NULL) {
		if (stor->storage_status == 1) { // storage open
//			if (idx >= 0 && idx < MAX_STORAGE) { // valid index // checked before to call function
//				if ((amount > 0) { // valid amount // checked before to call function
				if (amount <= stor->storage[idx].amount) { // valid amount
					if ((flag = pc_additem(sd, &stor->storage[idx], amount)) == 0)
						storage_delitem(sd, stor, idx, amount);
					else
						clif_additem(sd, 0, 0, flag);
				} // valid amount
//			} // valid index
		} // storage open
	}

	return;
}

/*==========================================
 * カプラ倉庫へカートから入れる
 *------------------------------------------
 */
void storage_storageaddfromcart(struct map_session_data *sd, short idx, int amount) {
	struct storage *stor;

//	nullpo_retv(sd); // checked before to call function
	nullpo_retv(stor = account2storage(sd->status.account_id));

	if (stor->storage_amount <= MAX_STORAGE && stor->storage_status == 1) { // storage not full & storage open
		if (idx >= 0 && idx < MAX_CART) { // valid index
			if (amount <= sd->status.cart[idx].amount && amount > 0) { //valid amount
				if (storage_additem(sd, stor, &sd->status.cart[idx], amount) == 0)
					pc_cart_delitem(sd, idx, amount, 0);
			} // valid amount
		}// valid index
	}// storage not full & storage open

	return;
}

/*==========================================
 * カプラ倉庫からカートへ出す
 *------------------------------------------
 */
void storage_storagegettocart(struct map_session_data *sd, short idx, int amount) {
	struct storage *stor;

//	nullpo_retv(sd); // checked before to call function
	nullpo_retv(stor = account2storage(sd->status.account_id));

	if (stor->storage_status == 1) { // storage open
		if (idx >= 0 && idx < MAX_STORAGE) { // valid index
			if (amount <= stor->storage[idx].amount && amount > 0) { //valid amount
				if (pc_cart_additem(sd, &stor->storage[idx], amount) == 0) {
					storage_delitem(sd, stor, idx, amount);
				}
			} // valid amount
		} // valid index
	} // storage open

	return;
}

/*==========================================
 * カプラ倉庫を閉じる
 *------------------------------------------
 */
void storage_storageclose(struct map_session_data *sd) {
	struct storage *stor;

//	nullpo_retv(sd); // checked before to call function
	nullpo_retv(stor = account2storage(sd->status.account_id));

	stor->storage_status = 0;
	sd->state.storage_flag = 0;
	clif_storageclose(sd);

	sortage_sortitem(stor);

	chrif_save(sd); // do pc_makesavestatus and save storage + account_reg/account_reg2 too

	return;
}

/*==========================================
 * ログアウト時開いているカプラ倉庫の保存
 *------------------------------------------
 */
int storage_storage_quit(struct map_session_data *sd)
{
	struct storage *stor;

	nullpo_retr(0, sd);

	stor = numdb_search(storage_db, sd->status.account_id);
	if (stor)
		stor->storage_status = 0;

	return 0;
}

int storage_storage_save(struct map_session_data *sd)
{
	struct storage *stor;

	nullpo_retr(0, sd);

	stor = numdb_search(storage_db,sd->status.account_id);
	if (stor) intif_send_storage(stor);

	return 0;
}

struct guild_storage *guild2storage(int guild_id) {
	struct guild_storage *gs = NULL;

	if (guild_search(guild_id) != NULL) {
		gs = numdb_search(guild_storage_db, guild_id);
		if (gs == NULL) {
			CALLOC(gs, struct guild_storage, 1);
			gs->guild_id = guild_id;
			numdb_insert(guild_storage_db, gs->guild_id, gs);
		}
	}

	return gs;
}

int guild_storage_delete(int guild_id)
{
	struct guild_storage *gstor = numdb_search(guild_storage_db,guild_id);
	if(gstor) {
		numdb_erase(guild_storage_db,guild_id);
		FREE(gstor);
	}

	return 0;
}

int storage_guild_storageopen(struct map_session_data *sd)
{
	struct guild_storage *gstor;

	nullpo_retr(0, sd);

	if (sd->status.guild_id <= 0)
		return 2;
	if ((gstor = numdb_search(guild_storage_db, sd->status.guild_id)) != NULL) {
		if (gstor->storage_status)
			return 1;
		gstor->storage_status = 1;
		sd->state.storage_flag = 1;
		clif_guildstorageitemlist(sd, gstor);
		clif_guildstorageequiplist(sd, gstor);
		clif_updateguildstorageamount(sd, gstor);
		return 0;
	} else {
		gstor = guild2storage(sd->status.guild_id);
		if (gstor == NULL) // guild has been deleted?
			return 2;
		gstor->storage_status = 1;
		intif_request_guild_storage(sd->status.account_id, sd->status.guild_id);
	}

	return 0;
}

int guild_storage_additem(struct map_session_data *sd,struct guild_storage *stor,struct item *item_data,int amount)
{
	struct item_data *data;
	int i;

	nullpo_retr(1, sd);
	nullpo_retr(1, stor);
	nullpo_retr(1, item_data);

	if (item_data->nameid <= 0 || amount <= 0)
		return 1;
	nullpo_retr(1, data = itemdb_search(item_data->nameid));

	if (!itemdb_canguildstore(item_data->nameid, sd->GM_level)) { // check if item can be stored in guild storage
		clif_displaymessage (sd->fd, msg_txt(286));
		return 1;
	}

	i = MAX_GUILD_STORAGE;
	if (!itemdb_isequip2(data)) {
		// 装備品ではないので、既所有品なら個数のみ変化させる
		for(i = 0; i < MAX_GUILD_STORAGE; i++) {
			if (stor->storage[i].nameid == item_data->nameid &&
			    stor->storage[i].identify == item_data->identify &&
			    stor->storage[i].refine == item_data->refine &&
			    stor->storage[i].attribute == item_data->attribute &&
			    stor->storage[i].card[0] == item_data->card[0] &&
			    stor->storage[i].card[1] == item_data->card[1] &&
			    stor->storage[i].card[2] == item_data->card[2] &&
			    stor->storage[i].card[3] == item_data->card[3]) {
				if (stor->storage[i].amount + amount > MAX_AMOUNT)
					return 1;
				stor->storage[i].amount += amount;
				clif_guildstorageitemadded(sd, stor, i, amount);
				break;
			}
		}
	}
	if (i >= MAX_GUILD_STORAGE) {
		// 装備品か未所有品だったので空き欄へ追加
		for(i = 0; i < MAX_GUILD_STORAGE; i++) {
			if (stor->storage[i].nameid == 0) {
				memcpy(&stor->storage[i], item_data, sizeof(stor->storage[0]));
				stor->storage[i].amount = amount;
				stor->storage_amount++;
				clif_guildstorageitemadded(sd, stor, i, amount);
				clif_updateguildstorageamount(sd, stor);
				break;
			}
		}
		if (i >= MAX_GUILD_STORAGE)
			return 1;
	}

	return 0;
}

int guild_storage_delitem(struct map_session_data *sd, struct guild_storage *stor, int n, int amount)
{
	nullpo_retr(1, sd);
	nullpo_retr(1, stor);

	if (stor->storage[n].nameid == 0 || stor->storage[n].amount < amount)
		return 1;

	stor->storage[n].amount -= amount;
	if (stor->storage[n].amount == 0) {
		memset(&stor->storage[n], 0, sizeof(stor->storage[0]));
		stor->storage_amount--;
		clif_updateguildstorageamount(sd, stor);
	}
	clif_storageitemremoved(sd, n, amount);

	stor->modified_storage_flag = 1; // Guild storage is modified

	return 0;
}

void storage_guild_storageadd(struct map_session_data *sd, short idx, int amount) {
	struct guild_storage *stor;

//	nullpo_retv(sd); // checked before to call function

	if ((stor = guild2storage(sd->status.guild_id)) != NULL) {
		if (stor->storage_amount <= MAX_GUILD_STORAGE && stor->storage_status == 1) { // storage not full & storage open
//			if (idx >= 0 && idx < MAX_INVENTORY) { // valid index // checked before to call function
//				if ((amount <= sd->status.inventory[idx].amount) && (amount > 0)) { // valid amount // checked before to call function
					if (guild_storage_additem(sd, stor, &sd->status.inventory[idx], amount) == 0)
					// remove item from inventory
						pc_delitem(sd, idx, amount, 0);
//				} // valid amount
//			} // valid index
		} // storage not full & storage open
	}

	return;
}

void storage_guild_storageget(struct map_session_data *sd, short idx, int amount) {
	struct guild_storage *stor;
	int flag;

//	nullpo_retv(sd); // checked before to call function

	if ((stor = guild2storage(sd->status.guild_id)) != NULL) {
		if (stor->storage_status == 1) { // storage open
//			if (idx >= 0 && idx < MAX_GUILD_STORAGE) { // valid index // checked before to call function
//				if (amount > 0) { //valid amount // checked before to call function
				if (amount <= stor->storage[idx].amount) { //valid amount
					if ((flag = pc_additem(sd, &stor->storage[idx], amount)) == 0)
						guild_storage_delitem(sd, stor, idx, amount);
					else
						clif_additem(sd, 0, 0, flag);
				} // valid amount
//			} // valid index
		} // storage open
	}

	return;
}

void storage_guild_storageaddfromcart(struct map_session_data *sd, short idx, int amount) {
	struct guild_storage *stor;

//	nullpo_retv(sd); // checked before to call function

	if ((stor = guild2storage(sd->status.guild_id)) != NULL) {
		if (stor->storage_amount <= MAX_GUILD_STORAGE && stor->storage_status == 1) { // storage not full & storage open
			if (idx >= 0 && idx < MAX_CART) { // valid index
				if (amount <= sd->status.cart[idx].amount && amount > 0) { //valid amount
					if (guild_storage_additem(sd, stor, &sd->status.cart[idx], amount) == 0)
						pc_cart_delitem(sd, idx, amount, 0);
				} // valid amount
			} // valid index
		} // storage not full & storage open
	}

	return;
}

void storage_guild_storagegettocart(struct map_session_data *sd, short idx, int amount) {
	struct guild_storage *stor;

//	nullpo_retv(sd); // checked before to call function

	if ((stor = guild2storage(sd->status.guild_id)) != NULL) {
		if (stor->storage_status == 1) { // storage open
			if (idx >= 0 && idx < MAX_GUILD_STORAGE) { // valid index
				if (amount <= stor->storage[idx].amount && amount > 0) { //valid amount
					if (pc_cart_additem(sd, &stor->storage[idx], amount) == 0) {
						guild_storage_delitem(sd, stor, idx, amount);
					}
				} // valid amount
			} // valid index
		} // storage open
	}

	return;
}

void storage_guild_storageclose(struct map_session_data *sd) {
	struct guild_storage *stor;

//	nullpo_retv(sd); // checked before to call function

	if ((stor = guild2storage(sd->status.guild_id)) != NULL) {
		if (stor->modified_storage_flag) {
			intif_send_guild_storage(sd->status.account_id, stor);
			//stor->modified_storage_flag = 0; // because char-server can crash, once modified, guild storage is always saved --> don't change flag
		}
		stor->storage_status = 0;
		sd->state.storage_flag = 0;
		chrif_save(sd); // do pc_makesavestatus and save storage + account_reg/account_reg2 too (after storage_flag = 0 to not save guild storage twice)
		sortage_gsortitem(stor);
	}
	clif_storageclose(sd);

	return;
}

int storage_guild_storage_quit(struct map_session_data *sd)
{
	struct guild_storage *stor;

	nullpo_retr(0, sd);

	stor = numdb_search(guild_storage_db, sd->status.guild_id);
	if (stor) {
		if (stor->modified_storage_flag) {
			intif_send_guild_storage(sd->status.account_id, stor);
			//stor->modified_storage_flag = 0; // because char-server can crash, once modified, guild storage is always saved --> don't change flag
		}
		stor->storage_status = 0;
		sd->state.storage_flag = 0;
		// save player to avoid difference if crash
		chrif_save(sd); // do pc_makesavestatus and save storage + account_reg/account_reg2 too (after storage_flag = 0 to not save guild storage twice)
	}

	return 0;
}

// just to save guild storage is necessary (when a player is saved)
int storage_guild_storagesave(struct map_session_data *sd)
{
	struct guild_storage *stor;

	nullpo_retr(0, sd);

	if ((stor = guild2storage(sd->status.guild_id)) != NULL) {
		if (stor->modified_storage_flag) {
			intif_send_guild_storage(sd->status.account_id, stor);
			//stor->modified_storage_flag = 0; // because char-server can crash, once modified, guild storage is always saved --> don't change flag
		}
	}

	return 0;
}

