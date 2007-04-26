// Copyright (c) Freya Development Team - Licensed under GNU GPL
// For more information, see License.txt in the main folder

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../common/db.h"
#include "../common/timer.h"
#include "../common/socket.h"
#include "../common/malloc.h"
#include "../common/utils.h"
#include "guild.h"
#include "storage.h"
#include "nullpo.h"
#include "battle.h"
#include "npc.h"
#include "pc.h"
#include "map.h"
#include "mob.h"
#include "intif.h"
#include "clif.h"
#include "skill.h"
#include "atcommand.h"
#include "chrif.h"
#include "status.h"

#ifdef MEMWATCH
#include "memwatch.h"
#endif

static struct dbt *guild_db;
static struct dbt *castle_db;
static struct dbt *guild_expcache_db;
static struct dbt *guild_infoevent_db;
static struct dbt *guild_castleinfoevent_db;

struct eventlist {
	char name[50];
	struct eventlist *next;
};

struct {
	int id;
	int max;
	struct {
		short id;
		short lv;
	} need[6];
} guild_skill_tree[MAX_GUILDSKILL];

#define GUILD_PAYEXP_INVERVAL 10000
#define GUILD_PAYEXP_LIST 8192

struct guild_expcache {
	int guild_id, account_id, char_id, exp;
};

int guild_skill_get_inf(int id)
{
	if (id == GD_BATTLEORDER) return 4;
	else if (id == GD_REGENERATION) return 4;
	else if (id == GD_RESTORE) return 4;
	else if (id == GD_EMERGENCYCALL) return 4;
	else return 0;
}

int guild_skill_get_sp(int id,int lv){ return 0; }

int guild_skill_get_range(int id){ return 0; }

int guild_skill_get_max(int id)
{
	if (id  < GD_SKILLBASE || id > GD_SKILLBASE+MAX_GUILDSKILL)
		return 0;
	return guild_skill_tree[id-GD_SKILLBASE].max;
}

int guild_checkskill(struct guild *g, int id)
{
	if (g==NULL) return 0;
	int idx = id - GD_SKILLBASE;

	if (idx < 0 || idx >= MAX_GUILDSKILL)
		return 0;

	if (id == GD_GLORYGUILD && battle_config.no_guilds_glory)
		return 1;

	if (g->skill[idx].lv < 0)
		return 0;

	return g->skill[idx].lv;
}

int guild_read_guildskill_tree_db(void)
{
	int i,k, id = 0, ln = 0;
	FILE *fp;
	char line[1024], *p;

	memset(guild_skill_tree,0,sizeof(guild_skill_tree));
	fp = fopen("db/guild_skill_tree.txt","r");
	if(fp == NULL){
		printf(CL_RED "Error:" CL_RESET " Failed to load db/guild_skill_tree.txt\n");
		return 1;
	}
	while(fgets(line, sizeof(line), fp)){
		char *split[50];
		if ((line[0] == '/' && line[1] == '/') || line[0] == '\0' || line[0] == '\n' || line[0] == '\r')
			continue;
		for(i=0,p=line;i<12 && p;i++){
			split[i]=p;
			p=strchr(p,',');
			if(p) *p++=0;
		}
		if(i<12)
			continue;
		id = atoi(split[0]) - GD_SKILLBASE;
		if(id<0 || id>=MAX_GUILDSKILL)
			continue;
		guild_skill_tree[id].id=atoi(split[0]);
		guild_skill_tree[id].max=atoi(split[1]);
		if (guild_skill_tree[id].id==GD_GLORYGUILD && battle_config.no_guilds_glory && guild_skill_tree[id].max==0)
			guild_skill_tree[id].max=1;
		for(k=0;k<5;k++){
			guild_skill_tree[id].need[k].id=atoi(split[k*2+2]);
			guild_skill_tree[id].need[k].lv=atoi(split[k*2+3]);
		}
	ln++;
	}
	fclose(fp);
	printf(CL_GREEN "Loaded: " CL_RESET "'" CL_WHITE "db/guild_skill_tree.txt" CL_RESET "' read.\n");

	return 0;
}

int guild_check_skill_require(struct guild *g,int id)
{
	int i;
	int idx = id-GD_SKILLBASE;

	if(g == NULL)
		return 0;

	if (idx < 0 || idx >= MAX_GUILDSKILL)
		return 0;

	for(i=0;i<5;i++)
	{
		if(guild_skill_tree[idx].need[i].id == 0)
			break;
		if(guild_skill_tree[idx].need[i].lv > guild_checkskill(g,guild_skill_tree[idx].need[i].id))
			return 0;
	}
	return 1;
}

int guild_payexp_timer(int tid, unsigned int tick, int id, int data);
int guild_gvg_eliminate_timer(int tid, unsigned int tick, int id, int data);

static int guild_read_castledb(void)
{
	FILE *fp;
	char line[1024];
	int j, ln = 0;
	char *str[32], *p;
	struct guild_castle *gc;

	if ((fp = fopen("db/castle_db.txt", "r")) == NULL)
	{
		printf(CL_RED "Error:" CL_RESET " Failed to load db/castle_db.txt\n");
		return -1;
	}

	while(fgets(line,sizeof(line),fp)) { // fgets reads until maximum one less than size and add '\0' -> so, it's not necessary to add -1
		if ((line[0] == '/' && line[1] == '/') || line[0] == '\0' || line[0] == '\n' || line[0] == '\r')
			continue;
		// It's not necessary to remove 'carriage return ('\n' or '\r')
		memset(str, 0, sizeof(str));
		CALLOC(gc, struct guild_castle, 1);
		for(j = 0, p = line; j < 6 && p; j++) {
			str[j] = p;
			p = strchr(p,',');
			if (p) *p++ = 0;
		}

		gc->guild_id = 0; // <Agit> Clear Data for Initialize
		gc->economy=0; gc->defense=0; gc->triggerE=0; gc->triggerD=0; gc->nextTime=0; gc->payTime=0;
		gc->createTime=0; gc->visibleC=0; gc->visibleG0=0; gc->visibleG1=0; gc->visibleG2=0;
		gc->visibleG3=0; gc->visibleG4=0; gc->visibleG5=0; gc->visibleG6=0; gc->visibleG7=0;
		gc->Ghp0=0; gc->Ghp1=0; gc->Ghp2=0; gc->Ghp3=0; gc->Ghp4=0; gc->Ghp5=0; gc->Ghp6=0; gc->Ghp7=0; // guardian HP [Valaris]

		gc->castle_id = atoi(str[0]);
		strncpy(gc->map_name, str[1], 16); // 17 - NULL
		strncpy(gc->castle_name, str[2], 24);
		strncpy(gc->castle_event, str[3], 24);

		numdb_insert(castle_db, gc->castle_id, gc);

		//intif_guild_castle_info(gc->castle_id);

		ln++;
	}
	fclose(fp);
	printf(CL_GREEN "Loaded: " CL_RESET "'" CL_WHITE "db/castle_db.txt" CL_RESET "' read ('" CL_WHITE "%d" CL_RESET "' entrie%s).\n", ln, (ln > 1) ? "s" : "");

	return 0;
}

void do_init_guild(void)
{
	guild_db = numdb_init();
	castle_db = numdb_init();
	guild_expcache_db = numdb_init();
	guild_infoevent_db = numdb_init();
	guild_castleinfoevent_db = numdb_init();

	guild_read_castledb();
	
	guild_read_guildskill_tree_db();

	add_timer_func_list(guild_gvg_eliminate_timer, "guild_gvg_eliminate_timer");
	add_timer_func_list(guild_payexp_timer, "guild_payexp_timer");
	add_timer_interval(gettick_cache + GUILD_PAYEXP_INVERVAL, guild_payexp_timer, 0, 0, GUILD_PAYEXP_INVERVAL);
}

struct guild *guild_search(int guild_id)
{
	return numdb_search(guild_db, guild_id);
}

int guild_searchname_sub(void *key, void *data, va_list ap)
{
	struct guild *g = (struct guild *)data, **dst;
	char *str;

	str = va_arg(ap,char *);
	dst = va_arg(ap,struct guild **);
	if (strcasecmp(g->name, str) == 0)
		*dst = g;

	return 0;
}

struct guild* guild_searchname(char *str)
{
	struct guild *g = NULL;

	numdb_foreach(guild_db, guild_searchname_sub, str, &g);

	return g;
}

struct guild_castle *guild_castle_search(int gcid)
{
	return numdb_search(castle_db,gcid);
}

struct guild_castle *guild_mapname2gc(char *mapname)
{
	int i;
	struct guild_castle *gc = NULL;

	for(i = 0; i < MAX_GUILDCASTLE; i++) {
		gc = guild_castle_search(i);
		if (!gc) continue;
		if (strcmp(gc->map_name, mapname) == 0) return gc;
	}

	return NULL;
}

struct map_session_data *guild_getavailablesd(struct guild *g)
{
	int i;

	nullpo_retr(NULL, g);

	for(i = 0; i < g->max_member; i++)
		if (g->member[i].sd != NULL)
			return g->member[i].sd;

	return NULL;
}

int guild_getindex(struct guild *g, int account_id, int char_id)
{
	int i;

	if (g == NULL)
		return -1;

	for(i = 0; i < g->max_member; i++)
		// Just check char_id (char_id is unique)
		if (g->member[i].char_id == char_id)
			return i;

	return -1;
}

int guild_getposition(struct map_session_data *sd, struct guild *g)
{
	int i;

	nullpo_retr(-1, sd);

	if (g == NULL && (g = guild_search(sd->status.guild_id)) == NULL)
		return -1;

	for(i = 0; i < g->max_member; i++)
		// Just check char_id (char_id is unique)
		if (g->member[i].char_id == sd->status.char_id)
			return g->member[i].position;

	return -1;
}

void guild_makemember(struct guild_member *m, struct map_session_data *sd)
{
//	nullpo_retv(sd); // checked before to call function

	memset(m, 0, sizeof(struct guild_member));
	m->account_id = sd->status.account_id;
	m->char_id    = sd->status.char_id;
	m->hair       = sd->status.hair;
	m->hair_color = sd->status.hair_color;
	m->gender     = sd->sex;
	m->class      = sd->status.class;
	m->lv         = sd->status.base_level;
	m->exp        = 0;
	m->exp_payper = 0;
	m->online     = 1;
	m->position   = MAX_GUILDPOSITION - 1;
	strncpy(m->name, sd->status.name, 24);

	return;
}

int guild_check_conflict(struct map_session_data *sd)
{
	nullpo_retr(0, sd);

	intif_guild_checkconflict(sd->status.guild_id, sd->status.account_id, sd->status.char_id);

	return 0;
}

int guild_payexp_timer_sub(void *key, void *data, va_list ap)
{
	int i, *dellist,*delp, dataid=(int)key;
	struct guild_expcache *c;
	struct guild *g;

	nullpo_retr(0, ap);
	nullpo_retr(0, c = (struct guild_expcache *)data);
	nullpo_retr(0, dellist = va_arg(ap, int *));
	nullpo_retr(0, delp = va_arg(ap, int *));

	if (*delp >= GUILD_PAYEXP_LIST || (g = guild_search(c->guild_id)) == NULL)
		return 0;
	if ((i = guild_getindex(g, c->account_id, c->char_id)) < 0)
		return 0;

	g->member[i].exp += c->exp;
	if (g->member[i].exp < 0)
		g->member[i].exp = 0x7FFFFFFF;

	intif_guild_change_memberinfo(g->guild_id, c->account_id, c->char_id, GMI_EXP, &c->exp, sizeof(c->exp));
	c->exp = 0;

	dellist[(*delp)++] = dataid;
	FREE(c);

	return 0;
}

int guild_payexp_timer(int tid,unsigned int tick,int id,int data)
{
	int dellist[GUILD_PAYEXP_LIST],delp=0,i;

	numdb_foreach(guild_expcache_db, guild_payexp_timer_sub, dellist, &delp);
	for(i=0;i<delp;i++)
		numdb_erase(guild_expcache_db,dellist[i]);
//	if(battle_config.etc_log)
//		printf("guild exp %d charactor's exp flushed !\n",delp);

	return 0;
}

//------------------------------------------------------------------------

void guild_create(struct map_session_data *sd, char *guildname)
{
	char guild_name[25]; // 24 + NULL

//	nullpo_retv(sd); // checked before to call function

	if (sd->status.guild_id == 0)
	{
		if (!battle_config.guild_emperium_check || pc_search_inventory(sd, 714) >= 0)
		{
			struct guild_member m;
			memset(guild_name, 0, sizeof(guild_name)); // 24 + NULL
			strncpy(guild_name, guildname, 24); // 24 + NULL
			// If too short name (hacker)
			if (guild_name[0] == '\0') {
				clif_guild_created(sd, 2); // 0x167 <flag>.b: 0: Guild has been created., 1: You are already in a guild., 2: That Guild Name already exists., 3: You need the necessary item to create a Guild.
				return;
			}
			// Check bad word
			if (check_bad_word(guild_name, strlen(guild_name), sd))
				return; // Check_bad_word function display message if necessary
			// check '"' in the name
			if (strchr(guild_name, '\"') != NULL)
			{
				clif_displaymessage(sd->fd, msg_txt(654)); // You are not authorized to use '"' in a guild name. // if you use ", /breakguild will not work
				return;
			}
			guild_makemember(&m, sd);
			m.position = 0;
			intif_guild_create(guild_name, &m);
		} else
			clif_guild_created(sd, 3); // 0x167 <flag>.b: 0: Guild has been created., 1: You are already in a guild., 2: That Guild Name already exists., 3: You need the necessary item to create a Guild.
	} else
		clif_guild_created(sd, 1); // 0x167 <flag>.b: 0: Guild has been created., 1: You are already in a guild., 2: That Guild Name already exists., 3: You need the necessary item to create a Guild.

	return;
}

int guild_created(int account_id,int guild_id)
{
	struct map_session_data *sd=map_id2sd(account_id);

	if (sd == NULL)
		return 0;

	if (guild_id > 0)
	{
			struct guild *g;
			sd->status.guild_id = guild_id;
			sd->guild_sended = 0;
			if ((g = numdb_search(guild_db, guild_id)) != NULL)
			{
				printf("guild: id already exists!\n");
				exit(1);
			}
			clif_guild_created(sd, 0); // 0x167 <flag>.b: 0: Guild has been created., 1: You are already in a guild., 2: That Guild Name already exists., 3: You need the necessary item to create a Guild.
			if (battle_config.guild_emperium_check)
				pc_delitem(sd, pc_search_inventory(sd,714), 1, 0);
	} else
	{
		clif_guild_created(sd, 2); // 0x167 <flag>.b: 0: Guild has been created., 1: You are already in a guild., 2: That Guild Name already exists., 3: You need the necessary item to create a Guild.
	}

	return 0;
}

int guild_request_info(int guild_id)
{
//	if(battle_config.etc_log)
//		printf("guild_request_info\n");

	return intif_guild_request_info(guild_id);
}

int guild_npc_request_info(int guild_id,const char *event)
{
	struct eventlist *ev;

	if (guild_search(guild_id))
	{
		if (event && *event)
			npc_event_do(event);
		return 0;
	}

	if (event == NULL || *event == 0)
		return guild_request_info(guild_id);

	CALLOC(ev, struct eventlist, 1);
	memcpy(ev->name, event, (sizeof(ev->name) > strlen(event)) ? strlen(event) : sizeof(ev->name));
	ev->next = (struct eventlist *)numdb_search(guild_infoevent_db, guild_id);
	numdb_insert(guild_infoevent_db, guild_id, ev);

	return guild_request_info(guild_id);
}

int guild_check_member(const struct guild *g)
{
	int i;
	struct map_session_data *sd;

	nullpo_retr(0, g);

	for(i = 0; i < fd_max; i++)
	{
		if (session[i] && (sd = session[i]->session_data) && sd->state.auth)
		{
			if (sd->status.guild_id == g->guild_id) {
				int j;
				for(j = 0; j < MAX_GUILD; j++)
				{
					// Just check char_id (char_id is unique)
					if (g->member[j].char_id == sd->status.char_id)
						break;
				}
				if (j == MAX_GUILD)
				{
					sd->status.guild_id = 0;
					sd->guild_sended = 0;
					sd->guild_emblem_id = 0;
					if (battle_config.error_log)
						printf("guild: check_member %d[%s] is not member\n",sd->status.account_id,sd->status.name);
				}
			}
		}
	}

	return 0;
}

int guild_recv_noinfo(int guild_id) // 0x3831 <size>.W (<guild_id>.L | <struct guild>.?B) - answer of 0x3031 - if size = 8: guild doesn't exist - otherwise: guild structure
{
	int i;
	struct map_session_data *sd;

	for(i = 0; i < fd_max; i++)
	{
		if (session[i] && (sd = session[i]->session_data) && sd->state.auth)
		{
			if (sd->status.guild_id == guild_id)
				sd->status.guild_id = 0;
		}
	}

	return 0;
}

int guild_recv_info(struct guild *sg) // 0x3831 <size>.W (<guild_id>.L | <struct guild>.?B) - answer of 0x3031 - if size = 8: guild doesn't exist - otherwise: guild structure
{
	struct guild *g, before;
	int i, bm, m;
	struct eventlist *ev,*ev2;

	nullpo_retr(0, sg);

	if ((g = numdb_search(guild_db, sg->guild_id)) == NULL)
	{
		struct map_session_data *sd;
		CALLOC(g, struct guild, 1);
		numdb_insert(guild_db, sg->guild_id, g);
		before = *sg;

		guild_check_member(sg);
		if ((sd = map_nick2sd(sg->master)) != NULL)
			sd->state.gmaster_flag = g;	// To update gmaster_flag when the guild master is the first player to join the map-server

	} else
		before = *g;
	memcpy(g, sg, sizeof(struct guild));

	for(i = bm = m = 0; i < g->max_member; i++)
	{
		if (g->member[i].account_id > 0)
		{
			struct map_session_data *sd = map_id2sd(g->member[i].account_id);
			g->member[i].sd = (sd != NULL &&
			                   sd->status.char_id == g->member[i].char_id &&
			                   sd->status.guild_id == g->guild_id) ? sd : NULL;
			m++;
		} else
			g->member[i].sd = NULL;
		if (before.member[i].account_id > 0)
			bm++;
	}

	for(i = 0; i < g->max_member; i++)
	{
		struct map_session_data *sd = g->member[i].sd;
		if (sd == NULL)
			continue;

		if (before.guild_lv != g->guild_lv || bm != m || before.max_member != g->max_member) {
			clif_guild_basicinfo(sd);
			clif_guild_emblem(sd, g);
		}

		if (bm != m)
			clif_guild_memberlist(g->member[i].sd);

		if (before.skill_point != g->skill_point)
			clif_guild_skillinfo(sd);

		if (sd->guild_sended == 0)
		{
			clif_guild_belonginfo(sd, g);
			clif_guild_notice(sd, g);
			sd->guild_emblem_id = g->emblem_id;
			sd->guild_sended = 1;
		}
	}

	if ((ev = numdb_search(guild_infoevent_db, sg->guild_id)) != NULL)
	{
		numdb_erase(guild_infoevent_db, sg->guild_id);
		while(ev) {
			npc_event_do(ev->name);
			ev2 = ev->next;
			FREE(ev);
			ev = ev2;
		}
	}

	return 0;
}

void guild_invite(struct map_session_data *sd, int account_id)
{
	struct map_session_data *tsd;
	struct guild *g;
	int i;

//	nullpo_retv(sd); // Checked before to call function

	if (map[sd->bl.m].flag.gvg) {	// Cannot leave/join guild on GvG maps
		clif_guild_inviteack(sd, 1); // R 0169 <type>.B (Types: 0: in an other guild, 1: it was denied, 2: join, 3: guild has no more available place)
		return;
	}

	tsd = map_id2sd(account_id);
	if (tsd == NULL)
		return;

	g = guild_search(sd->status.guild_id);
	if (g == NULL)
		return;

	if (!battle_config.invite_request_check) // Are other requests accepted during a request or not?
	{
		if (tsd->party_invite > 0 || tsd->trade_partner)
		{
			clif_guild_inviteack(sd, 0); // R 0169 <type>.B (Types: 0: in an other guild, 1: it was denied, 2: join, 3: guild has no more available place)
			return;
		}
	}
	if (tsd->status.guild_id > 0 || tsd->guild_invite > 0)
	{
		clif_guild_inviteack(sd, 0); // R 0169 <type>.B (Types: 0: in an other guild, 1: it was denied, 2: join, 3: guild has no more available place)
		return;
	}

	if (sd->state.gmaster_flag == NULL && ((i = guild_getposition(sd, g)) < 0 || !(g->position[i].mode & 0x0001)))
		return; // Only members who have permission should be able to expel people. Modes: invite 0x0001, expel 0x0010

	for(i = 0; i < g->max_member; i++)
	{
		if (g->member[i].account_id == 0)
			break;
	}
	if (i == g->max_member)
	{
		clif_guild_inviteack(sd, 3); // R 0169 <type>.B (Types: 0: in an other guild, 1: it was denied, 2: join, 3: guild has no more available place)
		return;
	}

	tsd->guild_invite = sd->status.guild_id;
	tsd->guild_invite_account = sd->status.account_id;

	clif_guild_invite(tsd, g);

	return;
}

void guild_reply_invite(struct map_session_data *sd, int guild_id, int flag)
{
	struct map_session_data *tsd;

//	nullpo_retv(sd); // checked before to call function
	nullpo_retv(tsd = map_id2sd(sd->guild_invite_account));

	if (sd->guild_invite != guild_id)
		return;

	if (flag == 1)
	{
		struct guild_member m;
		struct guild *g;
		int i;

		if ((g = guild_search(tsd->status.guild_id)) == NULL)
		{
			sd->guild_invite = 0;
			sd->guild_invite_account = 0;
			return;
		}
		for(i = 0; i < g->max_member; i++)
			if (g->member[i].account_id == 0)
				break;
		if (i == g->max_member)
		{
			sd->guild_invite = 0;
			sd->guild_invite_account = 0;
			clif_guild_inviteack(tsd,3); // R 0169 <type>.B (Types: 0: in an other guild, 1: it was denied, 2: join, 3: guild has no more available place)
			return;
		}

		guild_makemember(&m, sd);
		intif_guild_addmember(sd->guild_invite, &m);
	}
	else
	{
		sd->guild_invite = 0;
		sd->guild_invite_account = 0;
		clif_guild_inviteack(tsd, 1); // R 0169 <type>.B (Types: 0: in an other guild, 1: it was denied, 2: join, 3: guild has no more available place)
	}

	return;
}

int guild_member_added(int guild_id, int account_id, int char_id, int flag) // flag: 0: ok, 1: no guild/guild full
{
	struct map_session_data *sd= map_id2sd(account_id),*sd2;
	struct guild *g;

	if( (g=guild_search(guild_id))==NULL )
		return 0;

	if (sd == NULL || sd->guild_invite == 0)
	{
		if (flag == 0) // flag: 0: ok, 1: no guild/guild full
		{
			if (battle_config.error_log)
				printf("guild: member added error %d is not online.\n", account_id);
			intif_guild_leave(guild_id, account_id, char_id, 0, "* Failure of register *"); // mes is correctly managed in intif_guild_leave
		}
		return 0;
	}
	sd2 = map_id2sd(sd->guild_invite_account);

	sd->guild_invite = 0;
	sd->guild_invite_account = 0;

	if (flag == 1) // flag: 0: ok, 1: no guild/guild full
	{
		if (sd2 != NULL)
			clif_guild_inviteack(sd2, 3); // R 0169 <type>.B (Types: 0: in an other guild, 1: it was denied, 2: join, 3: guild has no more available place)
		return 0;
	}

	sd->guild_sended = 0;
	sd->status.guild_id = guild_id;

	if (sd2 != NULL)
		clif_guild_inviteack(sd2, 2); // R 0169 <type>.B (Types: 0: in an other guild, 1: it was denied, 2: join, 3: guild has no more available place)

	guild_check_conflict(sd);

	return 0;
}

void guild_leave(struct map_session_data *sd, const char *mes) // S 0159 <guildID>.l <accID>.l <charID>.l <mess>.40B
{
	//We dont use the values sent by the client, we can just take them from sd
	struct guild *g;
	int i;

//	nullpo_retv(sd); // checked before to call function

	if (map[sd->bl.m].flag.gvg)	// cannot leave/join guild on GvG maps
		return;

	g = guild_search(sd->status.guild_id);

	if (g == NULL)
		return;

	for(i = 0; i < g->max_member; i++)
	{
		// Just check char_id (char_id is unique)
		if (g->member[i].char_id == sd->status.char_id)
		{
			intif_guild_leave(sd->status.guild_id, sd->status.account_id, sd->status.char_id, 0, mes); // mes is correctly managed in intif_guild_leave
			return;
		}
	}

	return;
}

void guild_explusion(struct map_session_data *sd, int guild_id, int account_id, int char_id, const char *mes) // S 015B <guildID>.l <accID>.l <charID>.l <mess>.40B
{
	struct guild *g;
	int i, ps;

//	nullpo_retv(sd); // checked before to call function

	if (sd->status.guild_id != guild_id)
		return;

	g = guild_search(guild_id);

	if (g == NULL)
		return;

	if (sd->state.gmaster_flag == NULL && ((ps = guild_getposition(sd, g)) < 0 || !(g->position[ps].mode & 0x0010)))
		return;

	for(i = 0; i < g->max_member; i++)
	{
		// Just check char_id (char_id is unique)
		if (g->member[i].char_id == char_id)
		{
			intif_guild_leave(g->guild_id, account_id, char_id, 1, mes); // mes is correctly managed in intif_guild_leave
			memset(&g->member[i], 0, sizeof(struct guild_member));
			return;
		}
	}

	return;
}

int guild_member_leaved(int guild_id, int account_id, int char_id, int flag,
	const char *name, const char *mes)
{
	struct map_session_data *sd = map_id2sd(account_id);
	struct guild *g = guild_search(guild_id);

	if (g != NULL) {
		unsigned int i;
		for(i = 0; i < g->max_member; i++)
		{
			// just check char_id (char_id is unique)
			if (g->member[i].char_id == char_id)
			{
				struct map_session_data *sd2 = sd;
				if (sd2 == NULL)
					sd2 = guild_getavailablesd(g);
				else {
					if (flag == 0)
						clif_guild_leave(sd2, name, mes);
					else
						clif_guild_explusion(sd2, name, mes, account_id);
				}
				memset(&g->member[i], 0, sizeof(struct guild_member)); // Completely erase the member
			}
		}

		// Send all members to all online members
		for(i = 0; i < g->max_member; i++) {
			if (g->member[i].sd != NULL)
				clif_guild_memberlist(g->member[i].sd);
		}
	}

	// If player is on the map, reset his guild values
	if (sd != NULL && sd->status.guild_id == guild_id && sd->status.char_id == char_id) { // sd->status.account_id was found with account_id
		if (sd->state.storage_flag)
			storage_guild_storage_quit(sd); // Force close guild storage to avoid duplication method [Proximus]
		sd->status.guild_id = 0;
		sd->guild_emblem_id = 0;
		sd->guild_sended = 0;
	}

	return 0;
}

int guild_send_memberinfoshort(struct map_session_data *sd,int online)
{
	struct guild *g;

	nullpo_retr(0, sd);

	if(sd->status.guild_id<=0)
		return 0;

	g=guild_search(sd->status.guild_id);
	if(g==NULL)
		return 0;

	intif_guild_memberinfoshort(g->guild_id,
		sd->status.account_id,sd->status.char_id,online,sd->status.base_level,sd->status.class);

	if(!online)
	{
		int i=guild_getindex(g,sd->status.account_id,sd->status.char_id);
		if(i>=0)
			g->member[i].sd=NULL;
		return 0;
	}

	if(sd->guild_sended != 0)
		return 0;

	guild_check_conflict(sd);

	if((g=guild_search(sd->status.guild_id)) != NULL)
	{
		guild_check_member(g);
		if(sd->status.guild_id==g->guild_id){
			clif_guild_belonginfo(sd,g);
			clif_guild_notice(sd,g);
			sd->guild_sended=1;
			sd->guild_emblem_id=g->emblem_id;
		}
	}

	return 0;
}

int guild_recv_memberinfoshort(int guild_id,int account_id,int char_id,int online,int lv,int class)
{
	int i,alv,c,idx=0,om=0,oldonline=-1;
	struct guild *g = guild_search(guild_id);

	if(g==NULL)
		return 0;

	for(i=0,alv=0,c=0,om=0;i<g->max_member;i++)
	{
		struct guild_member *m=&g->member[i];
		// Just check char_id (char_id is unique)
		if (m->char_id == char_id)
		{
			oldonline=m->online;
			m->online=online;
			m->lv=lv;
			m->class=class;
			idx=i;
		}
		if(m->account_id>0)
		{
			alv+=m->lv;
			c++;
		}
		if(m->online)
			om++;
	}
	if(idx==g->max_member)
	{
		if(battle_config.error_log)
			printf("guild: not found member %d,%d on %d[%s]\n",	account_id,char_id,guild_id,g->name);
		return 0;
	}
	g->average_lv=c?alv/c:0;
	g->connect_member=om;

	if(oldonline!=online)
		clif_guild_memberlogin_notice(g,idx,online);

	for(i=0;i<g->max_member;i++)
	{
		struct map_session_data *sd= map_id2sd(g->member[i].account_id);
		g->member[i].sd=(sd!=NULL &&
			sd->status.char_id==g->member[i].char_id &&
			sd->status.guild_id==guild_id)?sd:NULL;
	}

	return 0;
}

void guild_send_message(struct map_session_data *sd, char *mes, int len)
{
//	nullpo_retv(sd); // checked before to call function

//	if (sd->status.guild_id == 0) // checked before to call function
//		return;

	// Send message (if multi-servers)
	if (!map_is_alone)
		intif_guild_message(sd->status.guild_id, sd->status.account_id, mes, len);
	// Send to local players
	guild_recv_message(sd->status.guild_id, sd->status.account_id, mes, len);

	return;
}

int guild_recv_message(int guild_id, int account_id, char *mes, int len)
{
	struct guild *g;

	if ((g = guild_search(guild_id)) == NULL)
		return 0;

	clif_guild_message(g, account_id, mes, len);

	return 0;
}

void guild_change_memberposition(int guild_id, int account_id, int char_id, int idx)
{
	intif_guild_change_memberinfo(guild_id, account_id, char_id, GMI_POSITION, &idx, sizeof(idx));

	return;
}

int guild_memberposition_changed(struct guild *g,int idx,int pos)
{
	nullpo_retr(0, g);

	g->member[idx].position=pos;
	clif_guild_memberpositionchanged(g,idx);

	return 0;
}

void guild_change_position(struct map_session_data *sd, int idx, int mode, int exp_mode, const char *name)
{
	struct guild_position p;

//	nullpo_retv(sd); // checked before to call function

	memset(&p, 0, sizeof(struct guild_position));

	p.mode = mode;
	if (exp_mode < 0) {
		// p.exp_mode = 0;
	} else if (exp_mode > battle_config.guild_exp_limit)
		p.exp_mode = battle_config.guild_exp_limit;
	else
		p.exp_mode = exp_mode;
	strncpy(p.name, name, 24);

	intif_guild_position(sd->status.guild_id, idx, &p);

	return;
}

int guild_position_changed(int guild_id, int idx, struct guild_position *p)
{
	struct guild *g=guild_search(guild_id);

	if(g==NULL)
		return 0;

	memcpy(&g->position[idx],p,sizeof(struct guild_position));
	clif_guild_positionchanged(g,idx);

	return 0;
}

void guild_change_notice(struct map_session_data *sd, int guild_id, const char *mes1, const char *mes2)
{
//	nullpo_retv(sd); // checked before to call function

	if (guild_id != sd->status.guild_id)
		return;

	intif_guild_notice(guild_id, mes1, mes2); // mes1 and mes2 correctly managed in intif_guild_notice

	return;
}

int guild_notice_changed(int guild_id,const char *mes1,const char *mes2)
{
	int i;
	struct map_session_data *sd;
	struct guild *g=guild_search(guild_id);

	if (g == NULL)
		return 0;

	memset(g->mes1, 0, sizeof(g->mes1));
	strncpy(g->mes1, mes1, 60);
	memset(g->mes2, 0, sizeof(g->mes2));
	strncpy(g->mes2, mes2, 120);

	for(i=0;i<g->max_member;i++)
	{
		if((sd=g->member[i].sd) != NULL)
			clif_guild_notice(sd,g);
	}

	return 0;
}

void guild_change_emblem(struct map_session_data *sd, unsigned short len, const char *data)
{
	struct guild *g;

//	nullpo_retv(sd); // checked before to call function

//	if (len < 0 || len > 2000) // unsigned short (cannot be negative)
	if (len > 2000)
		return;

	if ((g = guild_search(sd->status.guild_id)) && guild_checkskill(g, GD_GLORYGUILD) > 0)
	{
		intif_guild_emblem(sd->status.guild_id, len, data);
		return;
	}

	clif_skill_fail(sd, GD_GLORYGUILD, 0, 0);

	return;
}

int guild_emblem_changed(int len,int guild_id,int emblem_id,const char *data)
{
	int i;
	struct map_session_data *sd;
	struct guild *g=guild_search(guild_id);

	if(g==NULL)
		return 0;

	memcpy(g->emblem_data,data,len);
	g->emblem_len=len;
	g->emblem_id=emblem_id;

	for(i=0;i<g->max_member;i++)
	{
		if((sd = g->member[i].sd)!=NULL)
		{
			sd->guild_emblem_id=emblem_id;
			clif_guild_belonginfo(sd, g);
			clif_guild_emblem(sd, g);
		}
	}

	return 0;
}

int guild_payexp(struct map_session_data *sd, int guild_exp)
{
	struct guild *g;
	struct guild_expcache *c;
	int per, exp2;

	nullpo_retr(0, sd);

	if (sd->status.guild_id == 0 || (g = guild_search(sd->status.guild_id)) == NULL)
		return 0;
	if ((per = g->position[guild_getposition(sd,g)].exp_mode) <= 0)
		return 0;
	if (per > 100) per = 100;

	if ((exp2 = guild_exp * per / 100) <= 0)
		return 0;

	if ((c = numdb_search(guild_expcache_db, sd->status.char_id)) == NULL)
	{
		CALLOC(c, struct guild_expcache, 1);
		c->guild_id = sd->status.guild_id;
		c->account_id = sd->status.account_id;
		c->char_id = sd->status.char_id;
		c->exp = exp2;
		numdb_insert(guild_expcache_db, c->char_id, c);
	}
	else
	{
		c->exp += exp2;
	}

	return exp2;
}

int guild_getexp(struct map_session_data *sd, int guild_exp)
{
	struct guild *g;
	struct guild_expcache *c;

	nullpo_retr(0, sd);

	if (sd->status.guild_id == 0 || (g = guild_search(sd->status.guild_id)) == NULL)
		return 0;

	if ((c = (struct guild_expcache *) numdb_search(guild_expcache_db, sd->status.char_id)) == NULL)
	{
		CALLOC(c, struct guild_expcache, 1);
		c->guild_id = sd->status.guild_id;
		c->account_id = sd->status.account_id;
		c->char_id = sd->status.char_id;
		c->exp = guild_exp;
		numdb_insert(guild_expcache_db, c->char_id, c);
	}
	else
	{
		c->exp += guild_exp;
	}

	return guild_exp;
}

void guild_skillup(struct map_session_data *sd, short skill_num, int flag)
{
	struct guild *g;
	short idx = skill_num - GD_SKILLBASE;

	nullpo_retv(sd);

	if (idx < 0 || idx >= MAX_GUILDSKILL)
		return;

	if (sd->status.guild_id == 0 || (g = guild_search(sd->status.guild_id)) == NULL)
		return;
	if (strcmp(sd->status.name, g->master))
		return;

	if ((g->skill_point > 0 || flag & 1) &&
	    g->skill[idx].id != 0 &&
	    g->skill[idx].lv < guild_skill_get_max(skill_num))
	{
		intif_guild_skillup(g->guild_id, skill_num, sd->status.account_id, flag);
	}
	status_calc_pc(sd, 0);

	return;
}

int guild_skillupack(int guild_id,int skill_num,int account_id)
{
	struct map_session_data *sd=map_id2sd(account_id);
	struct guild *g=guild_search(guild_id);
	int i;

	if(g==NULL)
		return 0;

	if(sd!=NULL)
		clif_guild_skillup(sd,skill_num,g->skill[skill_num-GD_SKILLBASE].lv);

	for(i=0;i<g->max_member;i++)
		if((sd=g->member[i].sd)!=NULL)
			clif_guild_skillinfo(sd);

	return 0;
}

int guild_get_alliance_count(struct guild *g,int flag)
{
	int i,c;

	nullpo_retr(0, g);

	for(i=c=0;i<MAX_GUILDALLIANCE;i++)
	{
		if(g->alliance[i].guild_id>0 &&
			g->alliance[i].opposition==flag )
			c++;
	}

	return c;
}

void guild_reqalliance(struct map_session_data *sd, int account_id)
{
	struct map_session_data *tsd;
	struct guild *g[2];
	int i;

//	nullpo_retv(sd); // checked before to call function

	if (sd->status.guild_id <= 0)
		return;

	if (agit_flag) {
		clif_displaymessage(sd->fd, msg_txt(535)); // Alliances cannot be made during Guild Wars!
		return;
	}

	tsd= map_id2sd(account_id);
	if (tsd == NULL || tsd->status.guild_id <= 0)
		return;

	g[0] = guild_search(sd->status.guild_id);
	g[1] = guild_search(tsd->status.guild_id);

	if (g[0] == NULL || g[1] == NULL)
		return;

	if (guild_get_alliance_count(g[0], 0) >= 3)
		clif_guild_allianceack(sd, 4);
	if (guild_get_alliance_count(g[1], 0) >= 3)
		clif_guild_allianceack(sd, 3);

	if (tsd->guild_alliance > 0)
	{
		clif_guild_allianceack(sd, 1);
		return;
	}

	for(i = 0; i < MAX_GUILDALLIANCE; i++)
	{
		if (g[0]->alliance[i].guild_id == tsd->status.guild_id && g[0]->alliance[i].opposition == 0)
		{
			clif_guild_allianceack(sd, 0);
			return;
		}
	}

	tsd->guild_alliance = sd->status.guild_id;
	tsd->guild_alliance_account = sd->status.account_id;

	clif_guild_reqalliance(tsd, sd->status.account_id, g[0]->name);

	return;
}

void guild_reply_reqalliance(struct map_session_data *sd, int account_id, int flag) // flag: 0 deny, 1:accepted
{
	struct map_session_data *tsd;

//	nullpo_retv(sd); // checked before to call function
	nullpo_retv(tsd = map_id2sd(account_id));

	if (sd->guild_alliance != tsd->status.guild_id)
		return;

	if (flag == 1)
	{
		int i;
		struct guild *g1, *g2;

		if ((g1 = guild_search(sd->status.guild_id)) == NULL || guild_get_alliance_count(g1, 0) >= 3)
		{
			clif_guild_allianceack(sd, 4);
			clif_guild_allianceack(tsd, 3);
			return;
		}
		if ((g2 = guild_search(tsd->status.guild_id)) == NULL || guild_get_alliance_count(g2, 0) >= 3)
		{
			clif_guild_allianceack(sd, 3);
			clif_guild_allianceack(tsd, 4);
			return;
		}

		for(i = 0; i < MAX_GUILDALLIANCE; i++)
		{
			if (g1->alliance[i].guild_id == tsd->status.guild_id && g1->alliance[i].opposition == 1)
				intif_guild_alliance(sd->status.guild_id, tsd->status.guild_id, sd->status.account_id, tsd->status.account_id, 9);
		}
		for(i = 0; i < MAX_GUILDALLIANCE; i++)
		{
			if (g2->alliance[i].guild_id == sd->status.guild_id && g2->alliance[i].opposition == 1)
				intif_guild_alliance(tsd->status.guild_id, sd->status.guild_id, tsd->status.account_id, sd->status.account_id, 9);
		}

		intif_guild_alliance(sd->status.guild_id, tsd->status.guild_id, sd->status.account_id, tsd->status.account_id, 0);
	}
	else
	{
		sd->guild_alliance = 0;
		sd->guild_alliance_account = 0;
		clif_guild_allianceack(tsd, 3);
	}

	return;
}

void guild_delalliance(struct map_session_data *sd, int guild_id, int flag) {
//	nullpo_retv(sd); // checked before to call function

	if (agit_flag)
	{
		clif_displaymessage(sd->fd, msg_txt(536)); // Alliances cannot be broken during Guild Wars!
		return;
	}

	intif_guild_alliance(sd->status.guild_id, guild_id, sd->status.account_id, 0, flag | 8);

	return;
}

void guild_opposition(struct map_session_data *sd, int char_id)
{
	struct map_session_data *tsd;
	struct guild *g;
	int i;

//	nullpo_retv(sd); // checked before to call function

	g = guild_search(sd->status.guild_id);
	if (g == NULL)
		return;

	tsd = map_id2sd(char_id);
	if (tsd == NULL)
		return;

	if (guild_get_alliance_count(g, 1) >= 3)
		clif_guild_oppositionack(sd, 1); // flag: 0: success, 1: (failed)opposition guild list is full, 2: (failed)already in opposition guild list

	for(i = 0; i < MAX_GUILDALLIANCE; i++)
	{
		if (g->alliance[i].guild_id == tsd->status.guild_id)
		{
			if (g->alliance[i].opposition == 1)
			{
				clif_guild_oppositionack(sd, 2); // flag: 0: success, 1: (failed)opposition guild list is full, 2: (failed)already in opposition guild list
				return;
			}
			else
				intif_guild_alliance(sd->status.guild_id, tsd->status.guild_id, sd->status.account_id, tsd->status.account_id, 8);
		}
	}

	intif_guild_alliance(sd->status.guild_id, tsd->status.guild_id, sd->status.account_id, tsd->status.account_id, 1);

	return;
}

int guild_allianceack(int guild_id1, int guild_id2, int account_id1, int account_id2,
	int flag,const char *name1, const char *name2)
{
	struct guild *g[2];
	int guild_id[2];
	const char *guild_name[2];
	struct map_session_data *sd[2];
	int j, i;

	guild_id[0] = guild_id1;
	guild_id[1] = guild_id2;
	guild_name[0] = name1;
	guild_name[1] = name2;
	sd[0] = map_id2sd(account_id1);
	sd[1] = map_id2sd(account_id2);

	g[0] = guild_search(guild_id1);
	g[1] = guild_search(guild_id2);

	if (sd[0] != NULL && (flag & 0x0f) == 0)
	{
		sd[0]->guild_alliance = 0;
		sd[0]->guild_alliance_account = 0;
	}

	if (flag & 0x70)
	{
		for(i = 0; i < 2 - (flag & 1); i++)
			if (sd[i] != NULL)
				clif_guild_allianceack(sd[i], ((flag >> 4) == i + 1) ? 3 : 4);
		return 0;
	}
//	if (battle_config.etc_log)
//		printf("guild alliance_ack %d %d %d %d %d %s %s\n", guild_id1, guild_id2, account_id1, account_id2, flag, name1, name2);

	if (!(flag & 0x08))
	{
		for(i = 0; i < 2 - (flag & 1); i++)
			if(g[i]!=NULL)
				for(j=0;j<MAX_GUILDALLIANCE;j++)
					if(g[i]->alliance[j].guild_id==0)
					{
						g[i]->alliance[j].guild_id=guild_id[1-i];
						memset(g[i]->alliance[j].name, 0, sizeof(g[i]->alliance[j].name));
						strncpy(g[i]->alliance[j].name,guild_name[1-i],24);
						g[i]->alliance[j].opposition=flag&1;
						break;
					}
	}
	else
	{
		for(i = 0; i < 2 - (flag & 1); i++){
			if(g[i]!=NULL)
				for(j=0;j<MAX_GUILDALLIANCE;j++)
					if(	g[i]->alliance[j].guild_id==guild_id[1-i] &&
						g[i]->alliance[j].opposition==(flag&1)){
						g[i]->alliance[j].guild_id=0;
						break;
					}
			if(sd[i] != NULL)
				clif_guild_delalliance(sd[i],guild_id[1-i],(flag&1));
		}
	}

	if((flag&0x0f) == 0)
	{
		if(sd[1] != NULL)
			clif_guild_allianceack(sd[1],2);
	}
	else if((flag&0x0f) == 1)
	{
		if(sd[0] != NULL)
			clif_guild_oppositionack(sd[0], 0); // flag: 0: success, 1: (failed)opposition guild list is full, 2: (failed)already in opposition guild list
	}


	for(i = 0; i < 2 - (flag & 1); i++)
	{
		struct map_session_data *p_sd;
		if (g[i] != NULL)
			for(j = 0; j < g[i]->max_member; j++)
				if ((p_sd = g[i]->member[j].sd) != NULL)
					clif_guild_allianceinfo(p_sd);
	}

	return 0;
}

int guild_broken_sub(void *key,void *data,va_list ap)
{
	struct guild *g=(struct guild *)data;
	int guild_id=va_arg(ap,int);
	int i,j;
	struct map_session_data *sd=NULL;

	nullpo_retr(0, g);

	for(i=0;i<MAX_GUILDALLIANCE;i++)
	{
		if(g->alliance[i].guild_id==guild_id)
		{
			for(j=0;j<g->max_member;j++)
				if( (sd=g->member[j].sd)!=NULL )
					clif_guild_delalliance(sd,guild_id,g->alliance[i].opposition);
			g->alliance[i].guild_id=0;
		}
	}

	return 0;
}

int guild_broken(int guild_id,int flag)
{
	struct guild *g=guild_search(guild_id);
	struct map_session_data *sd;
	int i;

	if(flag!=0 || g==NULL)
		return 0;

	for(i=0;i<g->max_member;i++)
	{
		if ((sd = g->member[i].sd) != NULL)
		{
			if (sd->state.storage_flag)
				storage_guild_storage_quit(sd);
			sd->status.guild_id = 0;
			sd->guild_sended = 0;
			sd->state.gmaster_flag = NULL;
			clif_guild_broken(g->member[i].sd, 0);
		}
	}

	numdb_foreach(guild_db,guild_broken_sub,guild_id);
	numdb_erase(guild_db,guild_id);
	guild_storage_delete(guild_id);
	FREE(g);

	return 0;
}

void guild_break(struct map_session_data *sd, char *name)
{
	struct guild *g;
	register int i;

//	nullpo_retv(sd);

	if((g = guild_search(sd->status.guild_id)) == NULL)
		return;
	if(strcasecmp(g->name, name) != 0)
		return;
	if(strcasecmp(sd->status.name, g->master) != 0)
		return;

	for(i = 0; i < g->max_member; i++)
	{
		if(g->member[i].account_id > 0 && g->member[i].char_id != sd->status.char_id)
			break;
	}

	if(i != g->max_member)
	{
		clif_guild_broken(sd, 2);
		return;
	}

	intif_guild_break(g->guild_id);

	return;
}

int guild_castledataload(int castle_id,int idx)
{
	return intif_guild_castle_dataload(castle_id,idx);
}

int guild_addcastleinfoevent(int castle_id,int idx,const char *name)
{
	struct eventlist *ev;
	int code=castle_id|(idx<<16);

	if(name==NULL || *name==0)
		return 0;

	CALLOC(ev, struct eventlist, 1);
	strncpy(ev->name, name, sizeof(ev->name) - 1);
	ev->next=numdb_search(guild_castleinfoevent_db,code);
	numdb_insert(guild_castleinfoevent_db,code,ev);

	return 0;
}

int guild_castledataloadack(int castle_id,int idx,int value)
{
	struct guild_castle *gc=guild_castle_search(castle_id);
	int code=castle_id|(idx<<16);
	struct eventlist *ev,*ev2;

	if (gc == NULL)
		return 0;

	switch(idx)
	{
	case 1: gc->guild_id = value; break;
	case 2: gc->economy = value; break;
	case 3: gc->defense = value; break;
	case 4: gc->triggerE = value; break;
	case 5: gc->triggerD = value; break;
	case 6: gc->nextTime = value; break;
	case 7: gc->payTime = value; break;
	case 8: gc->createTime = value; break;
	case 9: gc->visibleC = value; break;
	case 10: gc->visibleG0 = value; break;
	case 11: gc->visibleG1 = value; break;
	case 12: gc->visibleG2 = value; break;
	case 13: gc->visibleG3 = value; break;
	case 14: gc->visibleG4 = value; break;
	case 15: gc->visibleG5 = value; break;
	case 16: gc->visibleG6 = value; break;
	case 17: gc->visibleG7 = value; break;
	case 18: gc->Ghp0 = value; break; // guardian HP [Valaris]
	case 19: gc->Ghp1 = value; break;
	case 20: gc->Ghp2 = value; break;
	case 21: gc->Ghp3 = value; break;
	case 22: gc->Ghp4 = value; break;
	case 23: gc->Ghp5 = value; break;
	case 24: gc->Ghp6 = value; break;
	case 25: gc->Ghp7 = value; break; // end additions [Valaris]
	default:
		printf("guild_castledataloadack ERROR!! (Not found index=%d)\n", idx);
		return 0;
	}
	if((ev=numdb_search(guild_castleinfoevent_db,code)) != NULL)
	{
		numdb_erase(guild_castleinfoevent_db,code);
		while(ev) {
			npc_event_do(ev->name);
			ev2 = ev->next;
			FREE(ev);
			ev = ev2;
		}
	}

	return 1;
}

int guild_castledatasave(int castle_id,int idx,int value)
{
	return intif_guild_castle_datasave(castle_id,idx,value);
}

int guild_castledatasaveack(int castle_id,int idx,int value)
{
	struct guild_castle *gc=guild_castle_search(castle_id);

	if (gc == NULL)
		return 0;

	switch(idx)
	{
	case 1: gc->guild_id = value; break;
	case 2: gc->economy = value; break;
	case 3: gc->defense = value; break;
	case 4: gc->triggerE = value; break;
	case 5: gc->triggerD = value; break;
	case 6: gc->nextTime = value; break;
	case 7: gc->payTime = value; break;
	case 8: gc->createTime = value; break;
	case 9: gc->visibleC = value; break;
	case 10: gc->visibleG0 = value; break;
	case 11: gc->visibleG1 = value; break;
	case 12: gc->visibleG2 = value; break;
	case 13: gc->visibleG3 = value; break;
	case 14: gc->visibleG4 = value; break;
	case 15: gc->visibleG5 = value; break;
	case 16: gc->visibleG6 = value; break;
	case 17: gc->visibleG7 = value; break;
	case 18: gc->Ghp0 = value; break;	// guardian HP [Valaris]
	case 19: gc->Ghp1 = value; break;
	case 20: gc->Ghp2 = value; break;
	case 21: gc->Ghp3 = value; break;
	case 22: gc->Ghp4 = value; break;
	case 23: gc->Ghp5 = value; break;
	case 24: gc->Ghp6 = value; break;
	case 25: gc->Ghp7 = value; break;	// end additions [Valaris]
	default:
		printf("guild_castledatasaveack ERROR!! (Not found index=%d)\n", idx);
		return 0;
	}

	return 1;
}

int guild_castlealldataload(int len,struct guild_castle *gc)
{
	int i;
	int n = (len - 4) / sizeof(struct guild_castle), ev = -1;

	nullpo_retr(0, gc);

//	// if first loading
//	/* char_init_done was removed as no longer used [MagicalTux] */
//	if (!char_init_done) {
		for(i = 0; i < n; i++)
		{
			if ((gc + i)->guild_id)
				ev = i;
		}
		for(i = 0; i < n; i++, gc++)
		{
			struct guild_castle *c = guild_castle_search(gc->castle_id);
			if (!c)
			{
				printf("guild_castlealldataload ??\n");
				continue;
			}
			memcpy(&c->guild_id, &gc->guild_id, sizeof(struct guild_castle) - ((int)&c->guild_id - (int)c));
			if (c->guild_id)
			{
				if (i != ev)
					guild_request_info(c->guild_id);
				else
					guild_npc_request_info(c->guild_id, "::OnAgitInit");
			}
		}
		if (ev == -1)
			npc_event_doall("OnAgitInit");
//	// if already loaded, just copy updated castles and ask for guild.
//	} else {
//		// èÈÉfÅ[É^äiî[Ç∆ÉMÉãÉhèÓïÒóvãÅ
//		for(i = 0; i < n; i++, gc++) {
//			struct guild_castle *c = guild_castle_search(gc->castle_id);
//			memcpy(&c->guild_id, &gc->guild_id, sizeof(struct guild_castle) - ((int)&c->guild_id - (int)c));
//			if (c->guild_id)
//				guild_request_info(c->guild_id);
//		}
//	} // must the if (ev == -1) npc_event_doall("OnAgitInit"); executed too ?

	printf("chrif: " CL_WHITE "%d castles" CL_RESET " received.\n", n);

	return 0;
}

// Run All NPC_Event[OnAgitStart]
void guild_agit_start(void)
{
	printf("NPC_Event:[OnAgitStart] Run (%d) Events by @AgitStart.\n", npc_event_doall("OnAgitStart"));

	return;
}

int guild_agit_end(void) // Run All NPC_Event[OnAgitEnd]
{
	int c = npc_event_doall("OnAgitEnd");

	printf("NPC_Event:[OnAgitEnd] Run (%d) Events by @AgitEnd.\n", c);

	return 0;
}

int guild_gvg_eliminate_timer(int tid, unsigned int tick, int id, int data)
{
// Run One NPC_Event[OnAgitEliminate]
	char *name = (char*)data;
	size_t len;
	char *evname;
	int c;

	if (name != NULL)
	{
		len = strlen(name);
		CALLOC(evname, char, len + 5); // 4 + final NULL

		if (agit_flag)
		{ // Agit not already End
			memcpy(evname, name, len - 5);
			strcpy(evname + len - 5, "Eliminate");
			c = npc_event_do(evname);
			printf("NPC_Event:[%s] Run (%d) Events.\n", evname, c);
		}
		FREE(name);
	}

	return 0;
}

int guild_agit_break(struct mob_data *md) // Run One NPC_Event[OnAgitBreak]
{
	char *evname;

	nullpo_retr(0, md);

	CALLOC(evname, char, strlen(md->npc_event) + 1); // + NULL

	strcpy(evname,md->npc_event);
// Now By User to Run [OnAgitBreak] NPC Event...
// It's a little impossible to null point with player disconnect in this!
// But Script will be stop, so nothing...
// Maybe will be changed in the futher..
//      int c = npc_event_do(evname);
	if (!agit_flag) // Agit already End
		return 0;
	add_timer(gettick_cache + battle_config.gvg_eliminate_time, guild_gvg_eliminate_timer, md->bl.m, (int)evname);

	return 0;
}

// [MouseJstr]
//   How many castles does this guild have?
int guild_checkcastles(struct guild *g)
{
	int i,nb_cas=0, id,cas_id=0;
	struct guild_castle *gc;

	id=g->guild_id;
	for(i=0;i<MAX_GUILDCASTLE;i++)
	{
		gc=guild_castle_search(i);
		cas_id=gc->guild_id;
		if(g->guild_id==cas_id)
			nb_cas=nb_cas+1;
	}

	return nb_cas;
}

//  Is this guild allied with this castle?
int guild_isallied(struct guild *g, struct guild_castle *gc)
{
	int i;

	nullpo_retr(0, g);

	if(g->guild_id == gc->guild_id)
		return 1;
	if (gc->guild_id == 0)
		return 0;

	for(i=0;i<MAX_GUILDALLIANCE;i++)
		if(g->alliance[i].guild_id == gc->guild_id)
		{
			if(g->alliance[i].opposition == 0)
				return 1;
			else
				return 0;
		}

	return 0;
}

int guild_idisallied(int guild_id, int guild_id2)
{
	int i;
	struct guild *g = NULL;

	if (guild_id <= 0 || guild_id2 <= 0)
		return 0;
	
	if(guild_id == guild_id2)
		return 1;

	if ((g = guild_search(guild_id)))
	{
		for(i = 0; i < MAX_GUILDALLIANCE; i++)
			if(g->alliance[i].guild_id == guild_id2)
			{
				if(g->alliance[i].opposition == 0)
					return 1;
				else
					return 0;
			}
	}
	return 0;
}

static int guild_db_final(void *key, void *data, va_list ap)
{
	struct guild *g = data;

	FREE(g);

	return 0;
}

static int castle_db_final(void *key, void *data, va_list ap)
{
	struct guild_castle *gc = data;

	FREE(gc);

	return 0;
}

static int guild_expcache_db_final(void *key, void *data, va_list ap)
{
	struct guild_expcache *c = data;

	FREE(c);

	return 0;
}

static int guild_infoevent_db_final(void *key,void *data,va_list ap)
{
	struct eventlist *ev=data;

	FREE(ev);

	return 0;
}

void do_final_guild(void)
{
	if(guild_db)
		numdb_final(guild_db,guild_db_final);
	if(castle_db)
		numdb_final(castle_db,castle_db_final);
	if(guild_expcache_db)
		numdb_final(guild_expcache_db,guild_expcache_db_final);
	if(guild_infoevent_db)
		numdb_final(guild_infoevent_db,guild_infoevent_db_final);
	if(guild_castleinfoevent_db)
		numdb_final(guild_castleinfoevent_db,guild_infoevent_db_final);
}
