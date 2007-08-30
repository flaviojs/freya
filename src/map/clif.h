// Copyright (c) Freya Development Team - Licensed under GNU GPL
// For more information, see License.txt in the main folder

#ifndef _CLIF_H_
#define _CLIF_H_

#ifdef __WIN32
typedef unsigned int in_addr_t;
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#include "map.h"

extern unsigned char display_unknown_packet;

enum {
	ALL_CLIENT,
	ALL_SAMEMAP,
	AREA,
	AREA_WOS,
	AREA_WOC,
	AREA_WOSC,
	AREA_PET,
	AREA_CHAT_WOC,
	CHAT,
	CHAT_WOS,
	PARTY,
	PARTY_WOS,
	PARTY_SAMEMAP,
	PARTY_SAMEMAP_WOS,
	PARTY_AREA,
	PARTY_AREA_WOS,
	GUILD,
	GUILD_WOS,
	GUILD_SAMEMAP,
	GUILD_SAMEMAP_WOS,
	GUILD_AREA,
	GUILD_AREA_WOS,
	SELF
};

void delete_manner();
char check_bad_word(const char* sentence, const int sentence_len, const struct map_session_data *sd);

void clif_mission_mob(struct map_session_data *sd, short mob_id, short progress);

void clif_setip(char*);
void clif_setport(int);

in_addr_t clif_getip(void);
int clif_getport(void);
int clif_countusers(void);
void clif_setwaitclose(int);

int clif_authok(struct map_session_data *);
void clif_authfail_fd(int, int);
int clif_charselectok(int);
void check_fake_id(struct map_session_data *sd, int target_id);
int clif_dropflooritem(struct flooritem_data *);
int clif_clearflooritem(struct flooritem_data *,int);
int clif_clearchar(struct block_list*, int);
int clif_clearchar_delay(unsigned int, struct block_list *,int);
#define clif_clearchar_area(bl,type) clif_clearchar(bl, type)
void clif_clearchar_id(int, unsigned char, int);
int clif_spawnpc(struct map_session_data*);
int clif_spawnnpc(struct npc_data*);
int clif_spawnmob(struct mob_data*);
int clif_spawnpet(struct pet_data*);
int clif_walkok(struct map_session_data*);
int clif_movechar(struct map_session_data*);
int clif_movemob(struct mob_data*);
int clif_movepet(struct pet_data *pd);
int clif_movenpc(struct npc_data *nd);
int clif_changemap(struct map_session_data*, char*, int, int);
int clif_changemapserver(struct map_session_data*, char*, int, int, int, int);
int clif_fixpos(struct block_list *); 
int clif_fixmobpos(struct mob_data *md);
int clif_fixpcpos(struct map_session_data *sd);
int clif_fixpetpos(struct pet_data *pd);
int clif_fixnpcpos(struct npc_data *nd); 
int clif_npcbuysell(struct map_session_data*, int);	
void clif_buylist(struct map_session_data*, struct npc_data*); 
void clif_selllist(struct map_session_data*); 
int clif_scriptmes(struct map_session_data*, int, char*); 
int clif_scriptnext(struct map_session_data*, int); 
int clif_scriptclose(struct map_session_data*, int); 
int clif_scriptmenu(struct map_session_data*, int, char*); 
int clif_scriptinput(struct map_session_data*,int); 
int clif_scriptinputstr(struct map_session_data *sd, int npcid); 
int clif_cutin(struct map_session_data*, char*, int); 
int clif_viewpoint(struct map_session_data*,int,int,int,int,int,int);	
int clif_additem(struct map_session_data*,int,int,int);	
int clif_delitem(struct map_session_data*,int,int);	
int clif_updatestatus(struct map_session_data*, int); 
int clif_changestatus(struct block_list*, int, int); 
int clif_damage(struct block_list *, struct block_list *, unsigned int, int, int, int, int, int, int); 
#define clif_takeitem(src, dst) clif_damage(src, dst, 0, 0, 0, 0, 0, 1, 0)
int clif_changelook(struct block_list *, int, int); 
int clif_arrowequip(struct map_session_data *sd, int val); 
int clif_arrow_fail(struct map_session_data *sd, int type); 
void clif_arrow_create_list(struct map_session_data *sd); 
int clif_statusupack(struct map_session_data *, int, int, int); 
int clif_equipitemack(struct map_session_data *, int, int, int); 
int clif_unequipitemack(struct map_session_data *, int, int, int); 
int clif_misceffect(struct block_list*, int); 
int clif_misceffect2(struct block_list *bl, int type);
int clif_changeoption(struct block_list*); 
int clif_useitemack(struct map_session_data*, int, int, int); 
void clif_GlobalMessage(struct block_list *bl, char *message);
int clif_createchat(struct map_session_data*, int); 
int clif_dispchat(struct chat_data*, int); 
int clif_joinchatfail(struct map_session_data*, int); 
int clif_joinchatok(struct map_session_data*, struct chat_data*); 
int clif_addchat(struct chat_data*, struct map_session_data*); 
int clif_changechatowner(struct chat_data*, struct map_session_data*);
int clif_clearchat(struct chat_data*, int);
int clif_leavechat(struct chat_data*, struct map_session_data*);
int clif_changechatstatus(struct chat_data*);

void clif_emotion(struct block_list *bl, int type);
void clif_talkiebox(struct block_list *bl, char* talkie);
void clif_wedding_effect(struct block_list *bl);
void clif_sitting(struct map_session_data *sd);
void clif_standing(struct map_session_data *sd);
void clif_soundeffect(struct map_session_data *sd, struct block_list *bl, char *name, int type);
int clif_soundeffectall(struct block_list *bl, char *name, int type);

void clif_adopt_process(struct map_session_data *sd);
void clif_adoption_invite(struct map_session_data *sd, struct map_session_data *tsd);

int clif_traderequest(struct map_session_data *sd, char *name);
int clif_tradestart(struct map_session_data *sd, unsigned char type);
int clif_tradeadditem(struct map_session_data *sd, struct map_session_data *tsd, int idx, int amount);
int clif_tradeitemok(struct map_session_data *sd, int idx, int fail);
int clif_tradedeal_lock(struct map_session_data *sd, int fail);
int clif_tradecancelled(struct map_session_data *sd);
int clif_tradecompleted(struct map_session_data *sd, int fail);

#include "storage.h"
int clif_storageitemlist(struct map_session_data *sd,struct storage *stor);
int clif_storageequiplist(struct map_session_data *sd,struct storage *stor);
int clif_updatestorageamount(struct map_session_data *sd,struct storage *stor);
int clif_storageitemadded(struct map_session_data *sd,struct storage *stor,int idx,int amount);
int clif_storageitemremoved(struct map_session_data *sd,int idx,int amount);
int clif_storageclose(struct map_session_data *sd);
int clif_guildstorageitemlist(struct map_session_data *sd,struct guild_storage *stor);
int clif_guildstorageequiplist(struct map_session_data *sd,struct guild_storage *stor);
int clif_updateguildstorageamount(struct map_session_data *sd,struct guild_storage *stor);
int clif_guildstorageitemadded(struct map_session_data *sd,struct guild_storage *stor,int idx,int amount);

int clif_pcinsight(struct block_list *,va_list);
int clif_pcoutsight(struct block_list *,va_list);
int clif_mobinsight(struct block_list *,va_list);
int clif_moboutsight(struct block_list *,va_list);
int clif_petoutsight(struct block_list *bl,va_list ap);
int clif_petinsight(struct block_list *bl,va_list ap);
int clif_npcoutsight(struct block_list *bl,va_list ap);
int clif_npcinsight(struct block_list *bl,va_list ap);

int clif_class_change(struct block_list *bl,int class,int type);
int clif_mob_class_change(struct mob_data *md,int class);
int clif_mob_equip(struct mob_data *md,int nameid);

int clif_skillinfo(struct map_session_data *sd,int skillid,int type,int range);
int clif_skillinfoblock(struct map_session_data *sd);
int clif_skillup(struct map_session_data *sd,int skill_num);

int clif_skillcasting(struct block_list* bl,
	int src_id, int dst_id, int dst_x, int dst_y, int skill_num, int casttime);
int clif_skillcastcancel(struct block_list* bl);
int clif_skill_fail(struct map_session_data *sd, int skill_id, int type, int btype);
int clif_skill_damage(struct block_list *src, struct block_list *dst,
	unsigned int tick, int sdelay, int ddelay, int damage, int div_,
	int skill_id, int skill_lv, int type);
int clif_skill_damage2(struct block_list *src,struct block_list *dst,
	unsigned int tick, int sdelay, int ddelay, int damage, int div_,
	int skill_id, int skill_lv, int type);
int clif_skill_nodamage(struct block_list *src, struct block_list *dst,
	int skill_id, int heal, int fail);
int clif_skill_poseffect(struct block_list *src,int skill_id,
	int val,int x,int y,int tick);
int clif_skill_estimation(struct map_session_data *sd, struct block_list *dst);
void clif_skill_warppoint(struct map_session_data *sd, unsigned short skill_num, const char *map1,const char *map2, const char *map3, const char *map4);
void clif_skill_memo(struct map_session_data *sd, unsigned char flag);
int clif_skill_teleportmessage(struct map_session_data *sd, int flag);
int clif_skill_produce_mix_list(struct map_session_data *sd, int trigger);

int clif_produceeffect(struct map_session_data *sd,int flag,int nameid);

int clif_skill_setunit(struct skill_unit *unit);
int clif_skill_delunit(struct skill_unit *unit);

int clif_01ac(struct block_list *bl);

int clif_autospell(struct map_session_data *sd,int skilllv);
int clif_devotion(struct map_session_data *sd,int target);
void clif_gospel(struct map_session_data *sd, int type);
int clif_spiritball(struct map_session_data *sd);
int clif_combo_delay(struct block_list *src, int wait);
int clif_bladestop(struct block_list *src, struct block_list *dst, int bool);
int clif_update_mobhp(struct mob_data *md);
int clif_changemapcell(int m,int x,int y,int cell_type,int type);
#ifdef USE_SQL // TXT version is still in dev
int clif_status_load(struct map_session_data *sd, int type);
#endif
int clif_status_change(struct block_list *bl,int type,int flag);

int clif_wis_message(int fd,char *nick,char *mes,int mes_len);
int clif_wis_end(int fd,int flag);

void clif_solved_charname(struct map_session_data *sd,int char_id);

void clif_insert_card(struct map_session_data *sd, short idx_equip, short idx_card, unsigned char flag);

int clif_itemlist(struct map_session_data *sd);
int clif_equiplist(struct map_session_data *sd);

int clif_cart_additem(struct map_session_data*, int n, int amount);
int clif_cart_addequipitem(struct map_session_data *sd, int n, int amount);
int clif_cart_delitem(struct map_session_data*, int, int);
int clif_cart_itemlist(struct map_session_data *sd);
int clif_cart_equiplist(struct map_session_data *sd);

int clif_item_identify_list(struct map_session_data *sd);
void clif_item_identified(struct map_session_data *sd, short idx, unsigned char flag);
void clif_item_repair_list(struct map_session_data *sd,struct map_session_data *dstsd);
int clif_item_refine_list(struct map_session_data *sd);

int clif_item_skill(struct map_session_data *sd,int skillid,int skilllv,const char *name);

int clif_mvp_effect(struct map_session_data *sd);
int clif_mvp_item(struct map_session_data *sd, int nameid);
int clif_mvp_exp(struct map_session_data *sd, int mexp);
void clif_changed_dir(struct block_list *bl);
void clif_slide(struct block_list *,int, int);
void clif_fame_point(struct map_session_data *sd, unsigned char type, unsigned int points);

void clif_openvendingreq(struct map_session_data *sd, int num);
int clif_showvendingboard(struct block_list* bl,char *message, int fd);
void clif_closevendingboard(struct block_list* bl, int fd);
void clif_vendinglist(struct map_session_data *sd, struct map_session_data *vsd);
int clif_buyvending(struct map_session_data *sd, int idx, int amount, int fail);
int clif_openvending(struct map_session_data *sd, int id, struct vending *vending);
int clif_vendingreport(struct map_session_data *sd, int idx, int amount);

int clif_movetoattack(struct map_session_data *sd,struct block_list *bl);

int clif_party_created(struct map_session_data *sd, int flag);
int clif_party_info(struct party *p, int fd);
void clif_party_invite(struct map_session_data *sd, struct map_session_data *tsd, struct party *p);
int clif_party_inviteack(struct map_session_data *sd, char *nick, int flag);
int clif_party_option(struct party *p, struct map_session_data *sd, int flag);
int clif_party_leaved(struct party *p, struct map_session_data *sd, int account_id, char *name, int flag);
void clif_party_message_self(struct map_session_data *sd, char *mes, int len);
int clif_party_message(struct party *p, int account_id,char *mes, int len);
int clif_party_move(struct party *p, struct map_session_data *sd, int online);
int clif_party_xy(struct party *p, struct map_session_data *sd);
int clif_party_hp(struct party *p, struct map_session_data *sd);
void clif_hpmeter(struct map_session_data *sd, struct map_session_data *dstsd);

void clif_guild_created(struct map_session_data *sd, int flag);
int clif_guild_belonginfo(struct map_session_data *sd, struct guild *g);
void clif_guild_basicinfo(struct map_session_data *sd);
void clif_guild_allianceinfo(struct map_session_data *sd);
void clif_guild_memberlist(struct map_session_data *sd);
void clif_guild_skillinfo(struct map_session_data *sd);
int clif_guild_memberlogin_notice(struct guild *g, int idx, int flag);
void clif_guild_invite(struct map_session_data *sd, struct guild *g);
void clif_guild_inviteack(struct map_session_data *sd, int flag);
int clif_guild_leave(struct map_session_data *sd, const char *name, const char *mes);
void clif_guild_explusion(struct map_session_data *sd, const char *name, const char *mes, int account_id);
int clif_guild_positionchanged(struct guild *g, int idx);
int clif_guild_memberpositionchanged(struct guild *g, int idx);
void clif_guild_emblem(struct map_session_data *sd, struct guild *g);
int clif_guild_notice(struct map_session_data *sd, struct guild *g);
int clif_guild_message(struct guild *g, int account_id, const char *mes, short len);
int clif_guild_skillup(struct map_session_data *sd, int skill_num, int lv);
int clif_guild_reqalliance(struct map_session_data *sd, int account_id, const char *name);
int clif_guild_allianceack(struct map_session_data *sd, int flag);
int clif_guild_delalliance(struct map_session_data *sd, int guild_id, int flag);
void clif_guild_oppositionack(struct map_session_data *sd, unsigned char flag);
void clif_guild_broken(struct map_session_data *sd, int flag);


int clif_displaymessage(const int fd, char* mes);
void clif_disp_onlyself(struct map_session_data *sd, char *mes);
int clif_GMmessage(struct block_list *bl, char* mes, short len, int flag);
void clif_announce(struct block_list *bl, char* mes, unsigned int color, unsigned int flag);
int clif_heal(int fd, int type, int val);
int clif_resurrection(struct block_list *bl, int type);
int clif_set0199(int fd, int type);
int clif_pvpset(struct map_session_data *sd, int pvprank, int pvpnum, int type);
int clif_send0199(int map_id, int type);
int clif_refine(int fd, struct map_session_data *sd, int fail, int idx, int val);

int clif_catch_process(struct map_session_data *sd);
void clif_pet_rulet(struct map_session_data *sd, unsigned char flag);
int clif_sendegg(struct map_session_data *sd);
int clif_send_petdata(struct map_session_data *sd, int type, int param);
void clif_send_petstatus(struct map_session_data *sd);
int clif_pet_performance(struct block_list *bl, int param);
int clif_pet_equip(struct pet_data *pd, int nameid);
int clif_pet_food(struct map_session_data *sd, int foodid, int fail);

void clif_friend_send_info(struct map_session_data *sd);
void clif_friend_add_ack(const int fd, int account_id, int char_id, char* name, int flag);
void clif_friend_del_ack(const int fd, int account_id, int char_id);
void clif_friend_send_online(struct map_session_data *sd, int online);

void clif_openmailbox(const int fd);
void clif_send_mailbox(struct map_session_data *sd,int num,struct mail_data *md[MAIL_STORE_MAX]);
void clif_receive_mail(struct map_session_data *sd,struct mail_data *md);
void clif_res_sendmail(const int fd,int flag);
void clif_arrive_newmail(const int fd,struct mail_data *md);
void clif_deletemail_res(const int fd,int mail_num,int flag);

int clif_specialeffect(struct block_list *bl,int type, int flag);
int clif_message(struct block_list *bl, char* msg);

int clif_GM_kickack(struct map_session_data *sd, int id);
int clif_GM_kick(struct map_session_data *sd, struct map_session_data *tsd, int type);

int clif_foreachclient(int (*)(struct map_session_data*, va_list), ...);

int do_final_clif(void);
int do_init_clif(void);

#endif // _CLIF_H_
