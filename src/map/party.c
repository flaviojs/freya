#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "party.h"
#include "../common/db.h"
#include "../common/timer.h"
#include "../common/socket.h"
#include "nullpo.h"
#include "../common/malloc.h"
#include "pc.h"
#include "map.h"
#include "battle.h"
#include "intif.h"
#include "clif.h"

#ifdef MEMWATCH
#include "memwatch.h"
#endif

#define PARTY_SEND_XYHP_INVERVAL 1000 // 座標やＨＰ送信の間隔

static struct dbt* party_db;

int party_send_xyhp_timer(int tid, unsigned int tick, int id, int data);

/*==========================================
 * 終了
 *------------------------------------------
 */
static int party_db_final(void *key,void *data,va_list ap)
{
	FREE(data);

	return 0;
}

void do_final_party(void)
{
	if(party_db)
		numdb_final(party_db,party_db_final);
}

// 初期化
void do_init_party(void)
{
	party_db = numdb_init();
	add_timer_func_list(party_send_xyhp_timer, "party_send_xyhp_timer");
	add_timer_interval(gettick_cache + PARTY_SEND_XYHP_INVERVAL, party_send_xyhp_timer, 0, 0, PARTY_SEND_XYHP_INVERVAL);
}

// 検索
struct party *party_search(int party_id)
{
	return numdb_search(party_db,party_id);
}

int party_searchname_sub(void *key,void *data,va_list ap)
{
	struct party *p=(struct party *)data,**dst;
	char *str;

	str = va_arg(ap,char *);
	dst = va_arg(ap,struct party **);
	if (strcasecmp(p->name, str) == 0)
		*dst = p;

	return 0;
}

// パーティ名検索
struct party* party_searchname(char *str)
{
	struct party *p = NULL;

	numdb_foreach(party_db, party_searchname_sub, str, &p);

	return p;
}

// 作成要求
void party_create(struct map_session_data *sd, char *name, short item, short item2) {
	int i;
	char party_name[25]; // 24 + NULL

//	nullpo_retv(sd); // checked before to call function

	if (sd->status.party_id == 0) {
		memset(party_name, 0, sizeof(party_name)); // 24 + NULL
		strncpy(party_name, name, 24); // 24 + NULL
		// if too short name (hacker)
		if (party_name[0] == '\0') {
			clif_party_created(sd, 1); // 0xfa <flag>.B: 0: Party has successfully been organized, 1: That Party Name already exists., 2: The Character is already in a party.
			return;
		}
		// check control chars and del (was checked in cahr-server, check before tor send to chas server)
		for(i = 0; party_name[i]; i++) {
			if (!(party_name[i] & 0xe0) || party_name[i] == 0x7f) {
				clif_party_created(sd, 1); // 0xfa <flag>.B: 0: Party has successfully been organized, 1: That Party Name already exists., 2: The Character is already in a party.
				return;
			}
		}
		// check bad word
		if (check_bad_word(party_name, strlen(party_name), sd))
			return; // Check_bad_word function display message if necessary
		intif_create_party(sd, party_name, item, item2);
	} else
		clif_party_created(sd, 2); // 0xfa <flag>.B: 0: Party has successfully been organized, 1: That Party Name already exists., 2: The Character is already in a party.

	return;
}

// 作成可否
int party_created(int account_id, int fail, int party_id, char *name) {
	struct map_session_data *sd;

	sd = map_id2sd(account_id);

	nullpo_retr(0, sd);

	if (fail == 0) {
		struct party *p;
		sd->status.party_id = party_id;
		if ((p = numdb_search(party_db, party_id)) != NULL) {
			printf("party: id already exists!\n");
			exit(1);
		}
		CALLOC(p, struct party, 1);
		p->party_id = party_id;
		strncpy(p->name, name, 24);
		numdb_insert(party_db,party_id, p);
		clif_party_created(sd, 0); // 0xfa <flag>.B: 0: Party has successfully been organized, 1: That Party Name already exists., 2: The Character is already in a party.
	} else {
		clif_party_created(sd, 1); // 0xfa <flag>.B: 0: Party has successfully been organized, 1: That Party Name already exists., 2: The Character is already in a party.
	}

	return 0;
}

// 情報要求
int party_request_info(int party_id) {
	return intif_request_partyinfo(party_id);
}

// 所属キャラの確認
int party_check_member(struct party *p)
{
	int i;
	struct map_session_data *sd;

	nullpo_retr(0, p);

	for(i=0;i<fd_max;i++){
		if(session[i] && (sd=session[i]->session_data) && sd->state.auth){
			if(sd->status.party_id==p->party_id){
				int j,f=1;
				for(j=0;j<MAX_PARTY;j++){	// パーティにデータがあるか確認
					if (p->member[j].account_id == sd->status.account_id) {
						if (strncmp(p->member[j].name, sd->status.name, 24) == 0)
							f=0;	// データがある
						else
							p->member[j].sd=NULL;	// 同垢別キャラだった
					}
				}
				if(f){
					sd->status.party_id=0;
					if(battle_config.error_log)
						printf("party: check_member %d[%s] is not member\n",sd->status.account_id,sd->status.name);
				}
			}
		}
	}

	return 0;
}

// 情報所得失敗（そのIDのキャラを全部未所属にする）
int party_recv_noinfo(int party_id) { // 0x3821 <size>.W (<party_id>.L | <struct party>.?B) - answer of 0x3021 - if size = 8: party doesn't exist - otherwise: party structure
	int i;
	struct map_session_data *sd;

	for(i = 0; i < fd_max; i++) {
		if (session[i] && (sd = session[i]->session_data) && sd->state.auth) {
			if (sd->status.party_id == party_id)
				sd->status.party_id = 0;
		}
	}

	return 0;
}

// 情報所得
int party_recv_info(struct party *sp) { // 0x3821 <size>.W (<party_id>.L | <struct party>.?B) - answer of 0x3021 - if size = 8: party doesn't exist - otherwise: party structure
	struct party *p;
	int i;

	nullpo_retr(0, sp);

	if ((p = numdb_search(party_db, sp->party_id)) == NULL) {
		CALLOC(p, struct party, 1);
		numdb_insert(party_db, sp->party_id, p);

		// 最初のロードなのでユーザーのチェックを行う
		party_check_member(sp);
	}
	memcpy(p, sp, sizeof(struct party));

	for(i = 0; i < MAX_PARTY; i++) { // sdの設定
		struct map_session_data *sd = map_id2sd(p->member[i].account_id);
		p->member[i].sd = (sd != NULL && sd->status.party_id == p->party_id) ? sd : NULL;
	}

	clif_party_info(p, -1);

	for(i = 0; i < MAX_PARTY; i++) { // 設定情報の送信
//		struct map_session_data *sd = map_id2sd(p->member[i].account_id);
		struct map_session_data *sd = p->member[i].sd;
		if (sd != NULL && sd->party_sended == 0) {
			clif_party_option(p, sd, 0x100);
			sd->party_sended = 1;
		}
	}

	return 0;
}

// パーティへの勧誘
void party_invite(struct map_session_data *sd, int account_id) {
	struct map_session_data *tsd;
	struct party *p;
	int i;

//	nullpo_retv(sd); // checked before to call function

	tsd = map_id2sd(account_id);
	if (tsd == NULL) // not: nullpo_retr(0, tsd); --> if invited player is disconnected, it's not an error (not display a message)
		return;

	p = party_search(sd->status.party_id);
	if (p == NULL)
		return;

	if (!battle_config.invite_request_check) { // Are other requests accepted during a request or not?
		if (tsd->guild_invite > 0 || tsd->trade_partner) { // 相手が取引中かどうか
			clif_party_inviteack(sd, tsd->status.name, 0); // 0: the player is already in other party, 1: invitation was denied, 2: success to invite, 4: character in the same account already joined
			return;
		}
	}
	if (tsd->status.party_id > 0 || tsd->party_invite > 0) { // 相手の所属確認
		clif_party_inviteack(sd, tsd->status.name, 0); // 0: the player is already in other party, 1: invitation was denied, 2: success to invite, 4: character in the same account already joined
		return;
	}
	for(i = 0; i < MAX_PARTY; i++) { // 同アカウント確認
		if (p->member[i].account_id == account_id) {
			if (battle_config.party_invite_same_account == 0 || strncmp(p->member[i].name, tsd->status.name, 24) == 0) {
				clif_party_inviteack(sd, tsd->status.name, 4); // 0: the player is already in other party, 1: invitation was denied, 2: success to invite, 4: character in the same account already joined
				return;
			}
		}
	}

	tsd->party_invite = sd->status.party_id;
	tsd->party_invite_account = sd->status.account_id;

	clif_party_invite(sd, tsd, p);

	return;
}

// パーティ勧誘への返答
void party_reply_invite(struct map_session_data *sd, int account_id, int flag) { // 0: invitation was denied, 1: accepted
	struct map_session_data *tsd;

//	nullpo_retv(sd); // checked before to call function

	if (flag == 1) { // 承諾 // 0: invitation was denied, 1: accepted
		//inter鯖へ追加要求
		intif_party_addmember(sd->party_invite, sd);
	} else { // 拒否
		sd->party_invite = 0;
		sd->party_invite_account = 0;
		tsd = map_id2sd(account_id);
		if (tsd == NULL)
			return;
		clif_party_inviteack(tsd, sd->status.name, 1); // 0: the player is already in other party, 1: invitation was denied, 2: success to invite, 4: character in the same account already joined
	}

	return;
}

// パーティが追加された
int party_member_added(int party_id, int account_id, int flag)
{
	struct map_session_data *sd = map_id2sd(account_id), *sd2;
	struct party *p;

	if (sd == NULL) {
		if (flag == 0) {
			if (battle_config.error_log)
				printf("party: member added error %d is not online\n", account_id);
			intif_party_leave(party_id, account_id); // キャラ側に登録できなかったため脱退要求を出す
		}
		return 0;
	}
	sd2 = map_id2sd(sd->party_invite_account);
	sd->party_invite = 0;
	sd->party_invite_account = 0;

	if (flag == 1) { // 失敗
		if (sd2 != NULL)
			clif_party_inviteack(sd2, sd->status.name, 0); // 0: the player is already in other party, 1: invitation was denied, 2: success to invite, 4: character in the same account already joined
		return 0;
	}

	// 成功
	sd->party_sended = 0;
	sd->status.party_id = party_id;

	if (sd2 != NULL)
		clif_party_inviteack(sd2, sd->status.name, 2); // 0: the player is already in other party, 1: invitation was denied, 2: success to invite, 4: character in the same account already joined

	// いちおう競合確認
	party_check_conflict(sd);

	if ((p = party_search(sd->status.party_id)) != NULL) {
		if (p->exp && !party_check_share_range(p, sd->status.base_level))
			intif_party_changeoption(sd->status.party_id, sd->status.account_id, 0, p->item);
	}

	return 0;
}

// パーティ除名要求
void party_removemember(struct map_session_data *sd, int account_id, char *name) {
	struct party *p;
	int i;

//	nullpo_retv(sd); // checked before to call function

	if ((p = party_search(sd->status.party_id)) == NULL)
		return;

	for(i = 0; i < MAX_PARTY; i++) { // リーダーかどうかチェック
		if (p->member[i].account_id == sd->status.account_id &&
		    strncmp(p->member[i].name, sd->status.name, 24) == 0)
			if (p->member[i].leader == 0)
				return;
	}

	for(i = 0; i < MAX_PARTY; i++) { // 所属しているか調べる
		if (p->member[i].account_id == account_id &&
		    strncmp(p->member[i].name, name, 24) == 0){
			intif_party_leave(p->party_id, account_id);
			return;
		}
	}

	return;
}

// パーティ脱退要求
void party_leave(struct map_session_data *sd) {
	struct party *p;
	int i;

//	nullpo_retv(sd); // checked before to call function

	if ((p = party_search(sd->status.party_id)) == NULL )
		return;

	for(i = 0; i < MAX_PARTY; i++) { // 所属しているか
		if (p->member[i].account_id == sd->status.account_id &&
		    strncmp(p->member[i].name, sd->status.name, 24) == 0) {
			intif_party_leave(p->party_id, sd->status.account_id);
			return;
		}
	}

	return;
}

// パーティメンバが脱退した
void party_member_leaved(int party_id, int account_id, char *name) {
	struct map_session_data *sd = map_id2sd(account_id);
	struct party * p = party_search(party_id);

	if (p != NULL) {
		int i;
		for(i = 0; i < MAX_PARTY; i++)
			if (p->member[i].account_id == account_id &&
			    strncmp(p->member[i].name, name, 24) == 0) {
				clif_party_leaved(p, sd, account_id, name, 0x00);
				memset(&p->member[i], 0, sizeof(struct party_member));
			}
	}
	if (sd != NULL && sd->status.party_id == party_id) {
		sd->status.party_id = 0;
		sd->party_sended = 0;
	}

	return;
}

// パーティ解散通知
int party_broken(int party_id) {
	struct party *p;
	int i;

	if ((p = party_search(party_id)) == NULL)
		return 0;

	for(i = 0; i < MAX_PARTY; i++){
		if (p->member[i].sd != NULL){
			clif_party_leaved(p, p->member[i].sd, p->member[i].account_id, p->member[i].name, 0x10);
			p->member[i].sd->status.party_id = 0;
			p->member[i].sd->party_sended = 0;
		}
	}
	numdb_erase(party_db, party_id);

	return 0;
}

int party_check_share_range(struct party *p, int level) {
	int i;

	for(i = 0; i < MAX_PARTY; i++) {
		if (p->member[i].online) {
			int diff = abs(p->member[i].lv - level);
			if (diff > MAX_PARTY_SHARE)
				return 0;
		}
	}

	return 1;
}

// パーティの設定変更要求
void party_changeoption(struct map_session_data *sd, unsigned short party_exp, unsigned short item) {
	struct party *p;
	int maxlv = 0, minlv = 0x7fffffff;
	int i, lv;

//	nullpo_retv(sd); // checked before to call function

	if (sd->status.party_id == 0 || (p = party_search(sd->status.party_id)) == NULL)
		return;

	// ONLY the party leader can choose either 'Each Take' or 'Even Share' for experience points.
	for(i = 0; i < MAX_PARTY; i++) { // リーダーかどうかチェック
		if (p->member[i].account_id == sd->status.account_id &&
		    strncmp(p->member[i].name, sd->status.name, 24) == 0)
			if (p->member[i].leader)
				break;
	}
	if (i == MAX_PARTY)
		return;

	for(i = 0; i < MAX_PARTY; i++) {
		if (p->member[i].online) {
			lv = p->member[i].lv;
			if (lv < minlv)
				minlv = lv;
			if (maxlv < lv)
				maxlv = lv;
		}
	}

	if (party_exp > 0 && maxlv != 0 && (maxlv - minlv > MAX_PARTY_SHARE))
		return;

	intif_party_changeoption(sd->status.party_id, sd->status.account_id, party_exp, item);

	return;
}

// パーティの設定変更通知
int party_optionchanged(int party_id, int account_id, unsigned short party_exp, unsigned char item, unsigned char flag)
{
	struct party *p;
	struct map_session_data *sd;

	if ((p = party_search(party_id)) == NULL)
		return 0;

	sd = map_id2sd(account_id);

	if (!(flag&0x01)) p->exp = party_exp;
	if (!(flag&0x10)) p->item = item;
	clif_party_option(p, sd, flag);

	return 0;
}

// パーティメンバの移動通知
void party_recv_movemap(int party_id, int account_id, char *mapname, int online, int lv) {
	struct party *p;
	int i;

	if( (p=party_search(party_id))==NULL)
		return;
	for(i=0;i<MAX_PARTY;i++){
		struct party_member *m=&p->member[i];
		if( m == NULL ){
			printf("party_recv_movemap nullpo?\n");
			return;
		}
		if (m->account_id == account_id){
			memset(m->map, 0, sizeof(m->map));
			strncpy(m->map, mapname, 16); // 17 - NULL
			m->online=online;
			m->lv=lv;
			break;
		}
	}
	if(i==MAX_PARTY){
		if(battle_config.error_log)
			printf("party: not found member %d on %d[%s]",account_id,party_id,p->name);
		return;
	}

	for(i=0;i<MAX_PARTY;i++){	// sd再設定
		struct map_session_data *sd= map_id2sd(p->member[i].account_id);
		p->member[i].sd=(sd!=NULL && sd->status.party_id==p->party_id)?sd:NULL;
	}

	party_send_xy_clear(p);	// 座標再通知要請

	clif_party_info(p,-1);

	return;
}

// パーティメンバの移動
void party_send_movemap(struct map_session_data *sd) {
	struct party *p;

//	nullpo_retv(sd); // checked before to call function

	if (sd->status.party_id == 0)
		return;

	intif_party_changemap(sd, 1); // flag: 0: offline, 1:online

	if (sd->party_sended != 0) // もうパーティデータは送信済み
		return;

	// 競合確認
	party_check_conflict(sd);

	// あるならパーティ情報送信
	if ((p = party_search(sd->status.party_id)) != NULL) {
		party_check_member(p); // 所属を確認する
		if (sd->status.party_id == p->party_id){
			clif_party_info(p, sd->fd);
			clif_party_option(p, sd, 0x100);
			sd->party_sended = 1;
		}
	}

	return;
}

// パーティメンバのログアウト
int party_send_logout(struct map_session_data *sd)
{
	struct party *p;

	nullpo_retr(0, sd);

	if (sd->status.party_id > 0) {
		intif_party_changemap(sd,0); // flag: 0: offline, 1:online

		// sdが無効になるのでパーティ情報から削除
		if ((p = party_search(sd->status.party_id)) != NULL) {
			int i;
			for(i = 0; i < MAX_PARTY; i++)
				if (p->member[i].sd == sd) {
					p->member[i].sd = NULL;
					p->member[i].online = 0;
				}
		}
	}

	return 0;
}

// パーティメッセージ送信
void party_send_message(struct map_session_data *sd, char *mes, int len) {
//	nullpo_retv(sd); // checked before to call function

//	if (sd->status.party_id == 0) // checked before to call function
//		return;

	// send message (if multi-servers)
	if (!map_is_alone)
		intif_party_message(sd->status.party_id, sd->status.account_id, mes, len);
	// send to local players
	party_recv_message(sd->status.party_id, sd->status.account_id, mes, len);

	return;
}

// パーティメッセージ受信
int party_recv_message(int party_id, int account_id, char *mes, int len) {
	struct party *p;

	if ((p = party_search(party_id)) == NULL)
		return 0;

	clif_party_message(p, account_id, mes, len);

	return 0;
}

// パーティ競合確認
void party_check_conflict(struct map_session_data *sd) {
//	nullpo_retv(sd); // checked before to call function

	intif_party_checkconflict(sd->status.party_id, sd->status.account_id, sd->status.name);

	return;
}

// 位置やＨＰ通知用
int party_send_xyhp_timer_sub(void *key,void *data,va_list ap)
{
	struct party *p=(struct party *)data;
	int i;

	nullpo_retr(0, p);

	for(i=0;i<MAX_PARTY;i++){
		struct map_session_data *sd;
		if((sd=p->member[i].sd)!=NULL){
			// 座標通知
			if(sd->party_x!=sd->bl.x || sd->party_y!=sd->bl.y){
				clif_party_xy(p,sd);
				sd->party_x=sd->bl.x;
				sd->party_y=sd->bl.y;
			}
			// ＨＰ通知
			if(sd->party_hp!=sd->status.hp){
				clif_party_hp(p,sd);
				sd->party_hp=sd->status.hp;
			}
		}
	}

	return 0;
}

// 位置やＨＰ通知
int party_send_xyhp_timer(int tid,unsigned int tick,int id,int data)
{
	numdb_foreach(party_db,party_send_xyhp_timer_sub,tick);

	return 0;
}

// 位置通知クリア
int party_send_xy_clear(struct party *p)
{
	int i;

	nullpo_retr(0, p);

	for(i=0;i<MAX_PARTY;i++){
		struct map_session_data *sd;
		if((sd=p->member[i].sd)!=NULL){
			sd->party_x=-1;
			sd->party_y=-1;
			sd->party_hp=-1;
		}
	}

	return 0;
}

// HP通知の必要性検査用（map_foreachinmoveareaから呼ばれる）
int party_send_hp_check(struct block_list *bl,va_list ap)
{
	int party_id;
	int *flag;
	struct map_session_data *sd;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);
	nullpo_retr(0, sd=(struct map_session_data *)bl);

	party_id=va_arg(ap,int);
	flag=va_arg(ap,int *);

	if(sd->status.party_id==party_id){
		*flag=1;
		sd->party_hp=-1;
	}

	return 0;
}

// 経験値公平分配
// exp share and added zeny share [Valaris]
void party_exp_share(struct party *p, short map_id, int base_exp, int job_exp, int zeny) {
	struct map_session_data *sd;
	struct map_session_data *sdlist[MAX_PARTY];
	int i, c;

//	nullpo_retv(p); // checked before to call function

	c = 0;
	for(i = 0; i < MAX_PARTY; i++) {
		// note: Characters that die during battle will not receive any experience distributed.
		if ((sd = p->member[i].sd) != NULL && sd->fd > 0 && session[sd->fd] != NULL && !pc_isdead(sd) && sd->bl.m == map_id) {
			if (battle_config.idle_no_share == 2 && (sd->idletime < (gettick_cache - battle_config.idle_delay_no_share))) // 2 minutes idle by default
				continue;
			if (battle_config.chat_no_share == 2 && sd->chatID)
				continue;
			if (battle_config.npc_chat_no_share == 2 && sd->npc_id)
				continue;
			if (battle_config.shop_no_share == 2 && sd->vender_id)
				continue;
			if (battle_config.trade_no_share == 2 && sd->trade_partner)
				continue;
			sdlist[c++] = sd;
		}
	}

	if (c == 0)
		return;

	if (c > 1) {
		if (base_exp > 0) {
			base_exp = (base_exp + (c - 1)) / c;
			if (base_exp < 1)
				base_exp = 1;
		}
		if (job_exp > 0) {
			job_exp = (job_exp + (c - 1)) / c;
			if (job_exp < 1)
				job_exp = 1;
		}
		if (zeny > 0) {
			zeny = (zeny + (c - 1)) / c;
			if (zeny < 1)
				zeny = 1;
		}
	}

	for(i = 0; i < c; i++) {
		sd = sdlist[i];
		if (battle_config.idle_no_share == 1 && (sd->idletime < (gettick_cache - battle_config.idle_delay_no_share))) // 2 minutes idle by default
			continue;
		if (battle_config.chat_no_share == 1 && sd->chatID)
			continue;
		if (battle_config.npc_chat_no_share == 1 && sd->npc_id)
			continue;
		if (battle_config.shop_no_share == 1 && sd->vender_id)
			continue;
		if (battle_config.trade_no_share == 1 && sd->trade_partner)
			continue;
		pc_gainexp(sd, base_exp, job_exp);
		if (battle_config.zeny_from_mobs) // zeny from mobs [Valaris]
			pc_getzeny(sd, zeny);
	}

	return;
}

int party_sub_count(struct block_list *bl, va_list ap)
{
	int *c;
	struct map_session_data *sd;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);

	sd = (struct map_session_data*)bl;
	c = va_arg(ap, int *);

	if (sd) (*c)++;

	return 0;
}

// 同じマップのパーティメンバー全体に処理をかける
// type==0 同じマップ
//     !=0 画面内
void party_foreachsamemap(int (*func)(struct block_list*,va_list),
	struct map_session_data *sd,int type,...)
{
	struct party *p;
	va_list ap;
	int i;
	int x0, y_0, x1, y_1;
	struct block_list *list[MAX_PARTY];
	int blockcount = 0;

	nullpo_retv(sd);

	if((p=party_search(sd->status.party_id))==NULL)
		return;

	x0  = sd->bl.x - AREA_SIZE;
	y_0 = sd->bl.y - AREA_SIZE;
	x1  = sd->bl.x + AREA_SIZE;
	y_1 = sd->bl.y + AREA_SIZE;

	va_start(ap,type);

	for(i=0;i<MAX_PARTY;i++){
		struct party_member *m=&p->member[i];
		if(m->sd!=NULL){
			if(sd->bl.m!=m->sd->bl.m)
				continue;
			if (type != 0 &&
			    (m->sd->bl.x < x0 || m->sd->bl.y < y_0 ||
			     m->sd->bl.x > x1 || m->sd->bl.y > y_1))
				continue;
			list[blockcount++]=&m->sd->bl;
		}
	}

	map_freeblock_lock(); // メモリからの解放を禁止する

	for(i=0;i<blockcount;i++)
		if(list[i]->prev) // 有効かどうかチェック
			func(list[i],ap);

	map_freeblock_unlock(); // 解放を許可する

	va_end(ap);
}
