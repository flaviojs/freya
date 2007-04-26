// Copyright (c) Freya Development Team - Licensed under GNU GPL
// For more information, see License.txt in the main folder

#ifndef _CHAT_H_
#define _CHAT_H_

#include "map.h"

void chat_createchat(struct map_session_data *, unsigned short, int, char*, char*, int);
void chat_joinchat(struct map_session_data *, int, char*);
void chat_leavechat(struct map_session_data*);
void chat_changechatowner(struct map_session_data *, char *);
void chat_changechatstatus(struct map_session_data *, unsigned short, int, char*, char*, int);
void chat_kickchat(struct map_session_data *,char *);

int chat_createnpcchat(struct npc_data *nd, unsigned short limit, int pub, int trigger, char* title, int titlelen, const char *ev);
int chat_deletenpcchat(struct npc_data *nd);
int chat_enableevent(struct chat_data *cd);
int chat_disableevent(struct chat_data *cd);
int chat_npckickall(struct chat_data *cd);

int do_final_chat(void);

#endif // _CHAT_H_
