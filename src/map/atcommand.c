// Copyright (c) Freya Development Team - Licensed under GNU GPL
// For more information, see License.txt in the main folder

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#include "../common/socket.h"
#include "../common/timer.h"
#include "../common/malloc.h"
#include "../common/core.h"
#include "../common/version.h"
#include "nullpo.h"

#include "clif.h"
#include "chrif.h"
#include "intif.h"
#include "itemdb.h"
#include "map.h"
#include "pc.h"
#include "skill.h"
#include "mob.h"
#include "pet.h"
#include "battle.h"
#include "party.h"
#include "guild.h"
#include "atcommand.h"
#include "script.h"
#include "npc.h"
#include "trade.h"
#include "grfio.h"
#include "status.h"

#define STATE_BLIND 0x10

short log_gm_level = 40;

static char command_symbol = '@'; /* first char of the commands */
static char char_command_symbol = '#'; /* first char of the remote commands */
static char main_channel_symbol = '~'; /* first char of the main channel */

time_t last_spawn; /* # of seconds 1/1/1970 (timestamp): to limit number of spawn at 1 every 2 seconds (reduction of lag) */

#define MSG_NUMBER 1000
static char *msg_table[MSG_NUMBER]; /* Server messages (0-499 reserved for GM commands, 500-999 reserved for others) */
static int msg_config_read_done = 0; /* for multiple configuration read */

static struct synonym_table { /* table for GM command synonyms */
	char* synonym;
	char* command;
} *synonym_table = NULL;
static int synonym_count = 0; /* number of synonyms */

#define ATCOMMAND_FUNC(x) int atcommand_ ## x (const int fd, struct map_session_data* sd, const char* original_command, const char* command, const char* message)
ATCOMMAND_FUNC(broadcast);
ATCOMMAND_FUNC(localbroadcast);
ATCOMMAND_FUNC(localbroadcast2);
ATCOMMAND_FUNC(rurap);
ATCOMMAND_FUNC(rura);
ATCOMMAND_FUNC(where);
ATCOMMAND_FUNC(jumpto);
ATCOMMAND_FUNC(jump);
ATCOMMAND_FUNC(users);
ATCOMMAND_FUNC(who);
ATCOMMAND_FUNC(who2);
ATCOMMAND_FUNC(who3);
ATCOMMAND_FUNC(whomap);
ATCOMMAND_FUNC(whomap2);
ATCOMMAND_FUNC(whomap3);
ATCOMMAND_FUNC(whogm);
ATCOMMAND_FUNC(whohas);
ATCOMMAND_FUNC(whohasmap);
ATCOMMAND_FUNC(save);
ATCOMMAND_FUNC(load);
ATCOMMAND_FUNC(charload);
ATCOMMAND_FUNC(charloadmap);
ATCOMMAND_FUNC(charloadall);
ATCOMMAND_FUNC(speed);
ATCOMMAND_FUNC(charspeed);
ATCOMMAND_FUNC(charspeedmap);
ATCOMMAND_FUNC(charspeedall);
ATCOMMAND_FUNC(storage);
ATCOMMAND_FUNC(charstorage);
ATCOMMAND_FUNC(guildstorage);
ATCOMMAND_FUNC(charguildstorage);
ATCOMMAND_FUNC(option);
ATCOMMAND_FUNC(optionadd);
ATCOMMAND_FUNC(optionremove);
ATCOMMAND_FUNC(hide);
ATCOMMAND_FUNC(jobchange);
ATCOMMAND_FUNC(die);
ATCOMMAND_FUNC(kill);
ATCOMMAND_FUNC(alive);
ATCOMMAND_FUNC(kami);
ATCOMMAND_FUNC(kamib);
ATCOMMAND_FUNC(kamiGM);
ATCOMMAND_FUNC(heal);
ATCOMMAND_FUNC(item);
ATCOMMAND_FUNC(charitem);
ATCOMMAND_FUNC(charitemall);
ATCOMMAND_FUNC(item2);
ATCOMMAND_FUNC(itemreset);
ATCOMMAND_FUNC(charitemreset);
ATCOMMAND_FUNC(itemcheck);
ATCOMMAND_FUNC(charitemcheck);
ATCOMMAND_FUNC(baselevelup);
ATCOMMAND_FUNC(joblevelup);
ATCOMMAND_FUNC(help);
ATCOMMAND_FUNC(gm);
ATCOMMAND_FUNC(pvpoff);
ATCOMMAND_FUNC(pvpon);
ATCOMMAND_FUNC(gvgoff);
ATCOMMAND_FUNC(gvgon);
ATCOMMAND_FUNC(style);
ATCOMMAND_FUNC(go);
ATCOMMAND_FUNC(go2);
ATCOMMAND_FUNC(spawn);
ATCOMMAND_FUNC(spawnmap);
ATCOMMAND_FUNC(spawnall);
ATCOMMAND_FUNC(killmonster);
ATCOMMAND_FUNC(killmonster2);
ATCOMMAND_FUNC(killmonsterarea);
ATCOMMAND_FUNC(killmonster2area);
ATCOMMAND_FUNC(refine);
ATCOMMAND_FUNC(refineall);
ATCOMMAND_FUNC(produce);
ATCOMMAND_FUNC(memo);
ATCOMMAND_FUNC(gat);
ATCOMMAND_FUNC(statusicon);
ATCOMMAND_FUNC(statuspoint);
ATCOMMAND_FUNC(skillpoint);
ATCOMMAND_FUNC(zeny);
ATCOMMAND_FUNC(param);
ATCOMMAND_FUNC(guildlevelup);
ATCOMMAND_FUNC(charguildlevelup);
ATCOMMAND_FUNC(petfriendly);
ATCOMMAND_FUNC(pethungry);
ATCOMMAND_FUNC(petrename);
ATCOMMAND_FUNC(charpetrename);
ATCOMMAND_FUNC(recall);
ATCOMMAND_FUNC(recallall);
ATCOMMAND_FUNC(character_job);
ATCOMMAND_FUNC(revive);
ATCOMMAND_FUNC(charheal);
ATCOMMAND_FUNC(character_stats);
ATCOMMAND_FUNC(character_stats_all);
ATCOMMAND_FUNC(character_option);
ATCOMMAND_FUNC(character_optionadd);
ATCOMMAND_FUNC(character_optionremove);
ATCOMMAND_FUNC(character_save);
ATCOMMAND_FUNC(night);
ATCOMMAND_FUNC(day);
ATCOMMAND_FUNC(doommap);
ATCOMMAND_FUNC(doom);
ATCOMMAND_FUNC(raisemap);
ATCOMMAND_FUNC(raise);
ATCOMMAND_FUNC(character_baselevel);
ATCOMMAND_FUNC(character_joblevel);
ATCOMMAND_FUNC(change_level);
ATCOMMAND_FUNC(kick);
ATCOMMAND_FUNC(kickmap);
ATCOMMAND_FUNC(kickall);
ATCOMMAND_FUNC(allskill);
ATCOMMAND_FUNC(questskill);
ATCOMMAND_FUNC(charquestskill);
ATCOMMAND_FUNC(lostskill);
ATCOMMAND_FUNC(charlostskill);
ATCOMMAND_FUNC(spiritball);
ATCOMMAND_FUNC(charspiritball);
ATCOMMAND_FUNC(guild);
ATCOMMAND_FUNC(resetstate);
ATCOMMAND_FUNC(resetskill);
ATCOMMAND_FUNC(charskreset);
ATCOMMAND_FUNC(charstreset);
ATCOMMAND_FUNC(charreset);
ATCOMMAND_FUNC(charstpoint);
ATCOMMAND_FUNC(charstyle);
ATCOMMAND_FUNC(charskpoint);
ATCOMMAND_FUNC(charzeny);
ATCOMMAND_FUNC(agitstart);
ATCOMMAND_FUNC(agitend);
ATCOMMAND_FUNC(reloaditemdb);
ATCOMMAND_FUNC(reloadmobdb);
ATCOMMAND_FUNC(reloadskilldb);
ATCOMMAND_FUNC(reloadscript);
ATCOMMAND_FUNC(mapexit);
ATCOMMAND_FUNC(idsearch);
ATCOMMAND_FUNC(whodrops);
ATCOMMAND_FUNC(mapinfo);
ATCOMMAND_FUNC(mobinfo);
ATCOMMAND_FUNC(dye);
ATCOMMAND_FUNC(hair_style);
ATCOMMAND_FUNC(hair_color);
ATCOMMAND_FUNC(stat_all);
ATCOMMAND_FUNC(change_sex);
ATCOMMAND_FUNC(char_change_sex);
ATCOMMAND_FUNC(char_block);
ATCOMMAND_FUNC(char_ban);
ATCOMMAND_FUNC(char_unblock);
ATCOMMAND_FUNC(char_unban);
ATCOMMAND_FUNC(mount_peco);
ATCOMMAND_FUNC(char_mount_peco);
ATCOMMAND_FUNC(falcon);
ATCOMMAND_FUNC(char_falcon);
ATCOMMAND_FUNC(cart);
ATCOMMAND_FUNC(char_cart);
ATCOMMAND_FUNC(remove_cart);
ATCOMMAND_FUNC(char_remove_cart);
ATCOMMAND_FUNC(guildspy);
ATCOMMAND_FUNC(partyspy);
ATCOMMAND_FUNC(repairall);
ATCOMMAND_FUNC(guildrecall);
ATCOMMAND_FUNC(partyrecall);
ATCOMMAND_FUNC(nuke);
ATCOMMAND_FUNC(enablenpc);
ATCOMMAND_FUNC(disablenpc);
ATCOMMAND_FUNC(servertime);
ATCOMMAND_FUNC(chardelitem);
ATCOMMAND_FUNC(jail);
ATCOMMAND_FUNC(unjail);
ATCOMMAND_FUNC(jailtime);
ATCOMMAND_FUNC(charjailtime);
ATCOMMAND_FUNC(disguise);
ATCOMMAND_FUNC(undisguise);
ATCOMMAND_FUNC(chardisguise);
ATCOMMAND_FUNC(charundisguise);
ATCOMMAND_FUNC(changelook);
ATCOMMAND_FUNC(charchangelook);
ATCOMMAND_FUNC(ignorelist);
ATCOMMAND_FUNC(charignorelist);
ATCOMMAND_FUNC(inall);
ATCOMMAND_FUNC(exall);
ATCOMMAND_FUNC(email);
ATCOMMAND_FUNC(password);
ATCOMMAND_FUNC(effect);
ATCOMMAND_FUNC(character_item_list);
ATCOMMAND_FUNC(character_storage_list);
ATCOMMAND_FUNC(character_cart_list);
ATCOMMAND_FUNC(addwarp);
ATCOMMAND_FUNC(follow);
ATCOMMAND_FUNC(unfollow);
ATCOMMAND_FUNC(skillon);
ATCOMMAND_FUNC(skilloff);
ATCOMMAND_FUNC(nospell);
ATCOMMAND_FUNC(killer);
ATCOMMAND_FUNC(charkiller);
ATCOMMAND_FUNC(npcmove);
ATCOMMAND_FUNC(killable);
ATCOMMAND_FUNC(charkillable);
ATCOMMAND_FUNC(chareffect);
ATCOMMAND_FUNC(chardye);
ATCOMMAND_FUNC(charhairstyle);
ATCOMMAND_FUNC(charhaircolor);
ATCOMMAND_FUNC(dropall);
ATCOMMAND_FUNC(chardropall);
ATCOMMAND_FUNC(storeall);
ATCOMMAND_FUNC(charstoreall);
ATCOMMAND_FUNC(skillid);
ATCOMMAND_FUNC(useskill);
ATCOMMAND_FUNC(snow);
ATCOMMAND_FUNC(sakura);
ATCOMMAND_FUNC(fog);
ATCOMMAND_FUNC(leaves);
ATCOMMAND_FUNC(clsweather);
ATCOMMAND_FUNC(mobsearch);
ATCOMMAND_FUNC(cleanmap);
ATCOMMAND_FUNC(cleanarea);
ATCOMMAND_FUNC(adjgmlvl);
ATCOMMAND_FUNC(adjgmlvl2);
ATCOMMAND_FUNC(adjcmdlvl);
ATCOMMAND_FUNC(trade);
ATCOMMAND_FUNC(send);
ATCOMMAND_FUNC(setbattleflag);
ATCOMMAND_FUNC(setmapflag);
ATCOMMAND_FUNC(unmute);
ATCOMMAND_FUNC(uptime);
ATCOMMAND_FUNC(clock);
ATCOMMAND_FUNC(mute);
ATCOMMAND_FUNC(refresh);
ATCOMMAND_FUNC(petid);
ATCOMMAND_FUNC(identify);
ATCOMMAND_FUNC(misceffect);
ATCOMMAND_FUNC(skilltree);
ATCOMMAND_FUNC(marry);
ATCOMMAND_FUNC(divorce);
ATCOMMAND_FUNC(rings);
ATCOMMAND_FUNC(sound);
ATCOMMAND_FUNC(mailbox);
ATCOMMAND_FUNC(autoloot);
ATCOMMAND_FUNC(autolootloot);
ATCOMMAND_FUNC(displayexp);
ATCOMMAND_FUNC(displaydrop);
ATCOMMAND_FUNC(displaylootdrop);
ATCOMMAND_FUNC(invincible);
ATCOMMAND_FUNC(charinvincible);
ATCOMMAND_FUNC(sc_start);
ATCOMMAND_FUNC(main);
ATCOMMAND_FUNC(request);
ATCOMMAND_FUNC(version);

/*==========================================
 *AtCommandInfo atcommand_info[]\‘¢‘Ì‚Ì’è‹`
 *------------------------------------------
 */
// First char of commands is configured in atcommand_freya.conf. Leave @ in this list for default value.
// to set default level, read atcommand_freya.conf first please.
// Note: Be sure that all commands are in lower case in this structure
static struct AtCommandInfo {
	AtCommandType type;
	const char* command;
	unsigned char level;
	int (*proc)(const int, struct map_session_data*, const char* original_command, const char* command, const char* message);
} atcommand_info[] = {
	{ AtCommand_RuraP,                 "@rura+",                60, atcommand_rurap },
	{ AtCommand_Rura,                  "@rura",                 40, atcommand_rura }, // /mm /mapmove command
	{ AtCommand_Where,                 "@where",                 3, atcommand_where },
	{ AtCommand_JumpTo,                "@jumpto",               20, atcommand_jumpto }, // + /shift and /remove
	{ AtCommand_Jump,                  "@jump",                 40, atcommand_jump },
	{ AtCommand_Users,                 "@users",                10, atcommand_users },
	{ AtCommand_Who,                   "@who",                  20, atcommand_who },
	{ AtCommand_Who2,                  "@who2",                 20, atcommand_who2 },
	{ AtCommand_Who3,                  "@who3",                 20, atcommand_who3 },
	{ AtCommand_WhoMap,                "@whomap",               20, atcommand_whomap },
	{ AtCommand_WhoMap2,               "@whomap2",              20, atcommand_whomap2 },
	{ AtCommand_WhoMap3,               "@whomap3",              20, atcommand_whomap3 },
	{ AtCommand_WhoGM,                 "@whogm",                20, atcommand_whogm },
	{ AtCommand_Save,                  "@save",                 40, atcommand_save },
	{ AtCommand_Load,                  "@load",                 40, atcommand_load },
	{ AtCommand_CharLoad,              "@charload",             60, atcommand_charload },
	{ AtCommand_CharLoadMap,           "@charloadmap",          60, atcommand_charloadmap },
	{ AtCommand_CharLoadAll,           "@charloadall",          80, atcommand_charloadall },
	{ AtCommand_Speed,                 "@speed",                40, atcommand_speed },
	{ AtCommand_CharSpeed,             "@charspeed",            60, atcommand_charspeed },
	{ AtCommand_CharSpeedMap,          "@charspeedmap",         60, atcommand_charspeedmap },
	{ AtCommand_CharSpeedAll,          "@charspeedall",         80, atcommand_charspeedall },
	{ AtCommand_Storage,               "@storage",               3, atcommand_storage },
	{ AtCommand_CharStorage,           "@charstorage",          20, atcommand_charstorage },
	{ AtCommand_GuildStorage,          "@gstorage",             50, atcommand_guildstorage },
	{ AtCommand_CharGuildStorage,      "@chargstorage",         60, atcommand_charguildstorage },
	{ AtCommand_Option,                "@option",               40, atcommand_option },
	{ AtCommand_OptionAdd,             "@optionadd",            40, atcommand_optionadd },
	{ AtCommand_OptionRemove,          "@optionremove",         40, atcommand_optionremove },
	{ AtCommand_Hide,                  "@hide",                 20, atcommand_hide }, // + /hide
	{ AtCommand_JobChange,             "@jobchange",            40, atcommand_jobchange },
	{ AtCommand_Die,                   "@die",                   2, atcommand_die },
	{ AtCommand_Kill,                  "@kill",                 60, atcommand_kill },
	{ AtCommand_Alive,                 "@alive",                60, atcommand_alive },
	{ AtCommand_Heal,                  "@heal",                 40, atcommand_heal },
	{ AtCommand_Kami,                  "@kami",                 40, atcommand_kami }, // /b, /nb and /bb command
	{ AtCommand_KamiB,                 "@kamib",                40, atcommand_kamib },
	{ AtCommand_KamiC,                 "@kamic",                40, atcommand_kami },
	{ AtCommand_KamiGM,                "@kamigm",               20, atcommand_kamiGM },
	{ AtCommand_Item,                  "@item",                 60, atcommand_item }, // + /item
	{ AtCommand_CharItem,              "@charitem",             60, atcommand_charitem },
	{ AtCommand_CharItemAll,           "@charitemall",          80, atcommand_charitemall },
	{ AtCommand_Item2,                 "@item2",                60, atcommand_item2 },
	{ AtCommand_ItemReset,             "@itemreset",            40, atcommand_itemreset },
	{ AtCommand_CharItemReset,         "@charitemreset",        60, atcommand_charitemreset },
	{ AtCommand_ItemCheck,             "@itemcheck",            60, atcommand_itemcheck },
	{ AtCommand_CharItemCheck,         "@charitemcheck",        60, atcommand_charitemcheck },
	{ AtCommand_BaseLevelUp,           "@lvup",                 60, atcommand_baselevelup },
	{ AtCommand_JobLevelUp,            "@jlvl",                 60, atcommand_joblevelup },
	{ AtCommand_Help,                  "@help",                  2, atcommand_help },
	{ AtCommand_GM,                    "@gm",                  100, atcommand_gm },
	{ AtCommand_PvPOff,                "@pvpoff",               40, atcommand_pvpoff },
	{ AtCommand_PvPOn,                 "@pvpon",                40, atcommand_pvpon },
	{ AtCommand_GvGOff,                "@gvgoff",               40, atcommand_gvgoff },
	{ AtCommand_GvGOn,                 "@gvgon",                40, atcommand_gvgon },
	{ AtCommand_Style,                 "@style",                20, atcommand_style },
	{ AtCommand_Go,                    "@go",                   10, atcommand_go },
	{ AtCommand_Go2,                   "@go2",                  10, atcommand_go2 },
	{ AtCommand_Spawn,                 "@spawn",                50, atcommand_spawn }, // + /monster
	{ AtCommand_Spawn,                 "@spawnsmall",           50, atcommand_spawn },
	{ AtCommand_Spawn,                 "@spawnbig",             50, atcommand_spawn },
	{ AtCommand_Spawn,                 "@spawnagro",            60, atcommand_spawn },
	{ AtCommand_Spawn,                 "@spawnneutral",         60, atcommand_spawn },
	{ AtCommand_Spawn,                 "@spawnagrosmall",       60, atcommand_spawn },
	{ AtCommand_Spawn,                 "@spawnneutralsmall",    60, atcommand_spawn },
	{ AtCommand_Spawn,                 "@spawnagrobig",         60, atcommand_spawn },
	{ AtCommand_Spawn,                 "@spawnneutralbig",      60, atcommand_spawn },
	{ AtCommand_SpawnMap,              "@spawnmap",             50, atcommand_spawnmap },
	{ AtCommand_SpawnMap,              "@spawnmapsmall",        50, atcommand_spawnmap },
	{ AtCommand_SpawnMap,              "@spawnmapbig",          50, atcommand_spawnmap },
	{ AtCommand_SpawnMap,              "@spawnmapagro",         60, atcommand_spawnmap },
	{ AtCommand_SpawnMap,              "@spawnmapneutral",      60, atcommand_spawnmap },
	{ AtCommand_SpawnMap,              "@spawnmapagrosmall",    60, atcommand_spawnmap },
	{ AtCommand_SpawnMap,              "@spawnmapneutralsmall", 60, atcommand_spawnmap },
	{ AtCommand_SpawnMap,              "@spawnmapagrobig",      60, atcommand_spawnmap },
	{ AtCommand_SpawnMap,              "@spawnmapneutralbig",   60, atcommand_spawnmap },
	{ AtCommand_SpawnAll,              "@spawnall",             60, atcommand_spawnall },
	{ AtCommand_SpawnAll,              "@spawnallsmall",        60, atcommand_spawnall },
	{ AtCommand_SpawnAll,              "@spawnallbig",          60, atcommand_spawnall },
	{ AtCommand_SpawnAll,              "@spawnallagro",         70, atcommand_spawnall },
	{ AtCommand_SpawnAll,              "@spawnallneutral",      70, atcommand_spawnall },
	{ AtCommand_SpawnAll,              "@spawnallagrosmall",    70, atcommand_spawnall },
	{ AtCommand_SpawnAll,              "@spawnallneutralsmall", 70, atcommand_spawnall },
	{ AtCommand_SpawnAll,              "@spawnallagrobig",      70, atcommand_spawnall },
	{ AtCommand_SpawnAll,              "@spawnallneutralbig",   70, atcommand_spawnall },
	{ AtCommand_KillMonster,           "@killmonster",          60, atcommand_killmonster },
	{ AtCommand_KillMonster2,          "@killmonster2",         40, atcommand_killmonster2 },
	{ AtCommand_KillMonsterArea,       "@killmonsterarea",      60, atcommand_killmonsterarea },
	{ AtCommand_KillMonster2Area,      "@killmonster2area",     40, atcommand_killmonster2area },
	{ AtCommand_Refine,                "@refine",               60, atcommand_refine },
	{ AtCommand_RefineAll,             "@refineall",            60, atcommand_refineall },
	{ AtCommand_Produce,               "@produce",              60, atcommand_produce },
	{ AtCommand_Memo,                  "@memo",                 40, atcommand_memo },
	{ AtCommand_GAT,                   "@gat",                  99, atcommand_gat },
	{ AtCommand_StatusIcon,            "@statusicon",           99, atcommand_statusicon },
	{ AtCommand_StatusPoint,           "@stpoint",              60, atcommand_statuspoint },
	{ AtCommand_SkillPoint,            "@skpoint",              60, atcommand_skillpoint },
	{ AtCommand_Zeny,                  "@zeny",                 60, atcommand_zeny },
	{ AtCommand_Strength,              "@str",                  60, atcommand_param },
	{ AtCommand_Agility,               "@agi",                  60, atcommand_param },
	{ AtCommand_Vitality,              "@vit",                  60, atcommand_param },
	{ AtCommand_Intelligence,          "@int",                  60, atcommand_param },
	{ AtCommand_Dexterity,             "@dex",                  60, atcommand_param },
	{ AtCommand_Luck,                  "@luk",                  60, atcommand_param },
	{ AtCommand_GuildLevelUp,          "@guildlvup",            60, atcommand_guildlevelup },
	{ AtCommand_CharGuildLevelUp,      "@charguildlvup",        60, atcommand_charguildlevelup },
	{ AtCommand_PetFriendly,           "@petfriendly",          40, atcommand_petfriendly },
	{ AtCommand_PetHungry,             "@pethungry",            40, atcommand_pethungry },
	{ AtCommand_PetRename,             "@petrename",             2, atcommand_petrename },
	{ AtCommand_CharPetRename,         "@charpetrename",        50, atcommand_charpetrename },
	{ AtCommand_Recall,                "@recall",               60, atcommand_recall }, // + /recall and /summon
	{ AtCommand_CharacterJob,          "@charjob",              60, atcommand_character_job },
	{ AtCommand_ChangeLevel,           "@charchangelevel",      60, atcommand_change_level },
	{ AtCommand_Revive,                "@charalive",            60, atcommand_revive },
	{ AtCommand_CharacterHeal,         "@charheal",             60, atcommand_charheal },
	{ AtCommand_CharacterStats,        "@charstats",            20, atcommand_character_stats },
	{ AtCommand_CharacterStatsAll,     "@charstatsall",         40, atcommand_character_stats_all },
	{ AtCommand_CharacterOption,       "@charoption",           60, atcommand_character_option },
	{ AtCommand_CharacterOptionAdd,    "@charoptionadd",        60, atcommand_character_optionadd },
	{ AtCommand_CharacterOptionRemove, "@charoptionremove",     60, atcommand_character_optionremove },
	{ AtCommand_CharacterSave,         "@charsave",             60, atcommand_character_save },
	{ AtCommand_Night,                 "@night",                80, atcommand_night },
	{ AtCommand_Day,                   "@day",                  80, atcommand_day },
	{ AtCommand_Doom,                  "@doom",                 80, atcommand_doom },
	{ AtCommand_DoomMap,               "@doommap",              70, atcommand_doommap },
	{ AtCommand_Raise,                 "@raise",                80, atcommand_raise },
	{ AtCommand_RaiseMap,              "@raisemap",             70, atcommand_raisemap },
	{ AtCommand_CharacterBaseLevel,    "@charlvup",             60, atcommand_character_baselevel },
	{ AtCommand_CharacterJobLevel,     "@charjlvl",             60, atcommand_character_joblevel },
	{ AtCommand_Kick,                  "@kick",                 20, atcommand_kick }, // + right click menu for GM "(name) force to quit"
	{ AtCommand_KickMap,               "@kickmap",              70, atcommand_kickmap },
	{ AtCommand_KickAll,               "@kickall",              99, atcommand_kickall },
	{ AtCommand_AllSkill,              "@allskill",             60, atcommand_allskill },
	{ AtCommand_QuestSkill,            "@questskill",           40, atcommand_questskill },
	{ AtCommand_CharQuestSkill,        "@charquestskill",       60, atcommand_charquestskill },
	{ AtCommand_LostSkill,             "@lostskill",            40, atcommand_lostskill },
	{ AtCommand_CharLostSkill,         "@charlostskill",        60, atcommand_charlostskill },
	{ AtCommand_SpiritBall,            "@spiritball",           40, atcommand_spiritball },
	{ AtCommand_CharSpiritBall,        "@charspiritball",       60, atcommand_charspiritball },
	{ AtCommand_Guild,                 "@guild",                50, atcommand_guild },
	{ AtCommand_AgitStart,             "@agitstart",            60, atcommand_agitstart },
	{ AtCommand_AgitEnd,               "@agitend",              60, atcommand_agitend },
	{ AtCommand_MapExit,               "@mapexit",              99, atcommand_mapexit },
	{ AtCommand_IDSearch,              "@idsearch",             60, atcommand_idsearch },
	{ AtCommand_WhoDrops,              "@whodrops",              1, atcommand_whodrops },
	{ AtCommand_Broadcast,             "@broadcast",            40, atcommand_broadcast },
	{ AtCommand_LocalBroadcast,        "@localbroadcast",       20, atcommand_localbroadcast },
	{ AtCommand_LocalBroadcast2,       "@nlb",                  40, atcommand_localbroadcast2 }, // /lb, /nlb and /mb commands
	{ AtCommand_LocalBroadcast2,       "@mb",                   40, atcommand_localbroadcast2 }, // /lb, /nlb and /mb commands
	{ AtCommand_RecallAll,             "@recallall",            80, atcommand_recallall },
	{ AtCommand_ResetState,            "@resetstate",           40, atcommand_resetstate }, // + /resetstate
	{ AtCommand_ResetSkill,            "@resetskill",           40, atcommand_resetskill }, // + /resetskill
	{ AtCommand_CharStReset,           "@charstreset",          60, atcommand_charstreset },
	{ AtCommand_CharSkReset,           "@charskreset",          60, atcommand_charskreset },
	{ AtCommand_ReloadItemDB,          "@reloaditemdb",         99, atcommand_reloaditemdb },
	{ AtCommand_ReloadMobDB,           "@reloadmobdb",          99, atcommand_reloadmobdb },
	{ AtCommand_ReloadSkillDB,         "@reloadskilldb",        99, atcommand_reloadskilldb },
	{ AtCommand_ReloadScript,          "@reloadscript",         99, atcommand_reloadscript },
	{ AtCommand_CharReset,             "@charreset",            60, atcommand_charreset },
	{ AtCommand_CharStyle,             "@charstyle",            50, atcommand_charstyle },
	{ AtCommand_CharSKPoint,           "@charskpoint",          60, atcommand_charskpoint },
	{ AtCommand_CharSTPoint,           "@charstpoint",          60, atcommand_charstpoint },
	{ AtCommand_CharZeny,              "@charzeny",             60, atcommand_charzeny },
	{ AtCommand_MapInfo,               "@mapinfo",              40, atcommand_mapinfo },
	{ AtCommand_MobInfo,               "@mobinfo",              20, atcommand_mobinfo },
	{ AtCommand_Dye,                   "@dye",                  20, atcommand_dye },
	{ AtCommand_Hstyle,                "@hairstyle",            20, atcommand_hair_style },
	{ AtCommand_Hcolor,                "@haircolor",            20, atcommand_hair_color },
	{ AtCommand_StatAll,               "@statall",              60, atcommand_stat_all },
	{ AtCommand_ChangeSex,             "@changesex",            20, atcommand_change_sex },
	{ AtCommand_CharChangeSex,         "@charchangesex",        60, atcommand_char_change_sex },
	{ AtCommand_CharBlock,             "@charblock",            60, atcommand_char_block },
	{ AtCommand_CharBan,               "@charban",              60, atcommand_char_ban },
	{ AtCommand_CharUnBlock,           "@charunblock",          80, atcommand_char_unblock },
	{ AtCommand_CharUnBan,             "@charunban",            80, atcommand_char_unban },
	{ AtCommand_MountPeco,             "@peco",                 20, atcommand_mount_peco },
	{ AtCommand_CharMountPeco,         "@charpeco",             50, atcommand_char_mount_peco },
	{ AtCommand_Falcon,                "@falcon",               20, atcommand_falcon },
	{ AtCommand_CharFalcon,            "@charfalcon",           50, atcommand_char_falcon },
	{ AtCommand_Cart,                  "@cart",                 20, atcommand_cart },
	{ AtCommand_Cart,                  "@cart0",                20, atcommand_cart },
	{ AtCommand_Cart,                  "@cart1",                20, atcommand_cart },
	{ AtCommand_Cart,                  "@cart2",                20, atcommand_cart },
	{ AtCommand_Cart,                  "@cart3",                20, atcommand_cart },
	{ AtCommand_Cart,                  "@cart4",                20, atcommand_cart },
	{ AtCommand_Cart,                  "@cart5",                20, atcommand_cart },
	{ AtCommand_RemoveCart,            "@removecart",           20, atcommand_remove_cart },
	{ AtCommand_CharCart,              "@charcart",             50, atcommand_char_cart },
	{ AtCommand_CharCart,              "@charcart0",            50, atcommand_char_cart },
	{ AtCommand_CharCart,              "@charcart1",            50, atcommand_char_cart },
	{ AtCommand_CharCart,              "@charcart2",            50, atcommand_char_cart },
	{ AtCommand_CharCart,              "@charcart3",            50, atcommand_char_cart },
	{ AtCommand_CharCart,              "@charcart4",            50, atcommand_char_cart },
	{ AtCommand_CharCart,              "@charcart5",            50, atcommand_char_cart },
	{ AtCommand_CharRemoveCart,        "@charremovecart",       50, atcommand_char_remove_cart },
	{ AtCommand_GuildSpy,              "@guildspy",             60, atcommand_guildspy },
	{ AtCommand_PartySpy,              "@partyspy",             60, atcommand_partyspy },
	{ AtCommand_RepairAll,             "@repairall",            60, atcommand_repairall },
	{ AtCommand_GuildRecall,           "@guildrecall",          60, atcommand_guildrecall },
	{ AtCommand_PartyRecall,           "@partyrecall",          60, atcommand_partyrecall },
	{ AtCommand_Nuke,                  "@nuke",                 60, atcommand_nuke },
	{ AtCommand_Enablenpc,             "@enablenpc",            80, atcommand_enablenpc },
	{ AtCommand_Disablenpc,            "@disablenpc",           80, atcommand_disablenpc },
	{ AtCommand_ServerTime,            "@time",                  1, atcommand_servertime },
	{ AtCommand_CharDelItem,           "@chardelitem",          60, atcommand_chardelitem },
	{ AtCommand_Jail,                  "@jail",                 60, atcommand_jail },
	{ AtCommand_UnJail,                "@unjail",               60, atcommand_unjail },
	{ AtCommand_JailTime,              "@jailtime",              1, atcommand_jailtime },
	{ AtCommand_CharJailTime,          "@charjailtime",         20, atcommand_charjailtime },
	{ AtCommand_Disguise,              "@disguise",             20, atcommand_disguise },
	{ AtCommand_UnDisguise,            "@undisguise",           20, atcommand_undisguise },
	{ AtCommand_CharDisguise,          "@chardisguise",         60, atcommand_chardisguise },
	{ AtCommand_CharUnDisguise,        "@charundisguise",       60, atcommand_charundisguise },
	{ AtCommand_ChangeLook,            "@changelook",           20, atcommand_changelook },
	{ AtCommand_CharChangeLook,        "@charchangelook",       60, atcommand_charchangelook },
	{ AtCommand_IgnoreList,            "@ignorelist",            1, atcommand_ignorelist },
	{ AtCommand_CharIgnoreList,        "@charignorelist",       20, atcommand_charignorelist },
	{ AtCommand_InAll,                 "@inall",                20, atcommand_inall },
	{ AtCommand_ExAll,                 "@exall",                20, atcommand_exall },
	{ AtCommand_EMail,                 "@email",                 1, atcommand_email },
	{ AtCommand_Password,              "@password",             20, atcommand_password },
	{ AtCommand_Effect,                "@effect",               40, atcommand_effect },
	{ AtCommand_Char_Item_List,        "@charitemlist",         20, atcommand_character_item_list },
	{ AtCommand_Char_Storage_List,     "@charstoragelist",      20, atcommand_character_storage_list },
	{ AtCommand_Char_Cart_List,        "@charcartlist",         20, atcommand_character_cart_list },
	{ AtCommand_Follow,                "@follow",               20, atcommand_follow },
	{ AtCommand_UnFollow,              "@unfollow",             20, atcommand_unfollow },
	{ AtCommand_AddWarp,               "@addwarp",              60, atcommand_addwarp },
	{ AtCommand_SkillOn,               "@skillon",              80, atcommand_skillon },
	{ AtCommand_SkillOff,              "@skilloff",             80, atcommand_skilloff },
	{ AtCommand_NoSpell,               "@nospell",              80, atcommand_nospell },
	{ AtCommand_Killer,                "@killer",               60, atcommand_killer },
	{ AtCommand_CharKiller,            "@charkiller",           60, atcommand_charkiller },
	{ AtCommand_NpcMove,               "@npcmove",              80, atcommand_npcmove },
	{ AtCommand_Killable,              "@killable",             40, atcommand_killable },
	{ AtCommand_CharKillable,          "@charkillable",         60, atcommand_charkillable },
	{ AtCommand_Chareffect,            "@chareffect",           40, atcommand_chareffect },
	{ AtCommand_Chardye,               "@chardye",              50, atcommand_chardye },
	{ AtCommand_Charhairstyle,         "@charhairstyle",        50, atcommand_charhairstyle },
	{ AtCommand_Charhaircolor,         "@charhaircolor",        50, atcommand_charhaircolor },
	{ AtCommand_Dropall,               "@dropall",              40, atcommand_dropall },
	{ AtCommand_Chardropall,           "@chardropall",          60, atcommand_chardropall },
	{ AtCommand_Storeall,              "@storeall",             40, atcommand_storeall },
	{ AtCommand_Charstoreall,          "@charstoreall",         60, atcommand_charstoreall },
	{ AtCommand_Skillid,               "@skillid",              40, atcommand_skillid },
	{ AtCommand_Useskill,              "@useskill",             40, atcommand_useskill },
	{ AtCommand_Snow,                  "@snow",                 80, atcommand_snow },
	{ AtCommand_Sakura,                "@sakura",               80, atcommand_sakura },
	{ AtCommand_Fog,                   "@fog",                  80, atcommand_fog },
	{ AtCommand_Leaves,                "@leaves",               80, atcommand_leaves },
	{ AtCommand_Clsweather,            "@clsweather",           80, atcommand_clsweather },
	{ AtCommand_MobSearch,             "@mobsearch",            20, atcommand_mobsearch },
	{ AtCommand_CleanMap,              "@cleanmap",             40, atcommand_cleanmap },
	{ AtCommand_CleanArea,             "@cleanarea",            40, atcommand_cleanarea },
	{ AtCommand_MiscEffect,            "@misceffect",           60, atcommand_misceffect },
	{ AtCommand_AdjGmLvl,              "@adjgmlvl",             80, atcommand_adjgmlvl },
	{ AtCommand_AdjGmLvl2,             "@adjgmlvl2",            99, atcommand_adjgmlvl2 },
	{ AtCommand_AdjGmLvl2,             "@adjgm0",               20, atcommand_adjgmlvl2 },
	{ AtCommand_AdjGmLvl2,             "@adjgm1",               20, atcommand_adjgmlvl2 },
	{ AtCommand_AdjGmLvl2,             "@adjgm2",               60, atcommand_adjgmlvl2 },
	{ AtCommand_AdjGmLvl2,             "@adjgm3",               60, atcommand_adjgmlvl2 },
	{ AtCommand_AdjGmLvl2,             "@adjgm4",               60, atcommand_adjgmlvl2 },
	{ AtCommand_AdjGmLvl2,             "@adjgm5",               60, atcommand_adjgmlvl2 },
	{ AtCommand_AdjGmLvl2,             "@adjgm6",               60, atcommand_adjgmlvl2 },
	{ AtCommand_AdjGmLvl2,             "@adjgm7",               60, atcommand_adjgmlvl2 },
	{ AtCommand_AdjGmLvl2,             "@adjgm8",               60, atcommand_adjgmlvl2 },
	{ AtCommand_AdjGmLvl2,             "@adjgm9",               60, atcommand_adjgmlvl2 },
	{ AtCommand_AdjGmLvl2,             "@adjgm10",              60, atcommand_adjgmlvl2 },
	{ AtCommand_AdjCmdLvl,             "@adjcmdlvl",            99, atcommand_adjcmdlvl },
	{ AtCommand_Trade,                 "@trade",                60, atcommand_trade },
	{ AtCommand_Send,                  "@send",                 60, atcommand_send },
	{ AtCommand_SetBattleFlag,         "@setbattleflag",        99, atcommand_setbattleflag },
	{ AtCommand_SetMapFlag,            "@setmapflag",           99, atcommand_setmapflag },
	{ AtCommand_UnMute,                "@unmute",               60, atcommand_unmute },
	{ AtCommand_UpTime,                "@uptime",                1, atcommand_uptime },
	{ AtCommand_Clock,                 "@clock",                 1, atcommand_clock },
	{ AtCommand_Mute,                  "@mute",                 99, atcommand_mute },
	{ AtCommand_Mute,                  "@red",                  99, atcommand_mute },
	{ AtCommand_WhoHas,                "@whohas",               20, atcommand_whohas },
	{ AtCommand_WhoHasMap,             "@whohasmap",            20, atcommand_whohasmap },
	{ AtCommand_Refresh,               "@refresh",              40, atcommand_refresh },
	{ AtCommand_PetId,                 "@petid",                40, atcommand_petid },
	{ AtCommand_Identify,              "@identify",             40, atcommand_identify },
	{ AtCommand_SkillTree,             "@skilltree",            40, atcommand_skilltree },
	{ AtCommand_Marry,                 "@marry",                40, atcommand_marry },
	{ AtCommand_Divorce,               "@divorce",              40, atcommand_divorce },
	{ AtCommand_Rings,                 "@rings",                40, atcommand_rings },
	{ AtCommand_Sound,                 "@sound",                40, atcommand_sound },
	{ AtCommand_MailBox,               "@mailbox",               1, atcommand_mailbox	},
	{ AtCommand_AutoLoot,              "@autoloot",              1, atcommand_autoloot },
	{ AtCommand_AutoLootLoot,          "@autolootloot",          1, atcommand_autolootloot },
	{ AtCommand_Displayexp,            "@displayexp",            1, atcommand_displayexp },
	{ AtCommand_DisplayDrop,           "@displaydrop",           1, atcommand_displaydrop },
	{ AtCommand_DisplayLootDrop,       "@displaylootdrop",       1, atcommand_displaylootdrop },
	{ AtCommand_Invincible,            "@invincible",           60, atcommand_invincible },
	{ AtCommand_CharInvincible,        "@charinvincible",       60, atcommand_charinvincible },
	{ AtCommand_SC_Start,              "@sc_start",             60, atcommand_sc_start },
	{ AtCommand_Main,                  "@main",                  1, atcommand_main },
	{ AtCommand_Request,               "@request",               1, atcommand_request },
	{ AtCommand_Version,               "@version",               1, atcommand_version },

// Add new commands before this line
	{ AtCommand_Unknown,               NULL,                     1, NULL }
};

/*=========================================
 * Generic variables
 *-----------------------------------------
 */
char atcmd_output[MAX_MSG_LEN + 512]; // at least maximum of message length + some variables (character' name, remaining time, map, etc...)
char atcmd_name[100];
char atcmd_mapname[100];

/*=========================================
 * This function returns the name of the user's class
 *-----------------------------------------
 */
char * job_name(int class) {
	switch (class) {
	case 0:    return "Novice";
	case 1:    return "Swordsman";
	case 2:    return "Mage";
	case 3:    return "Archer";
	case 4:    return "Acolyte";
	case 5:    return "Merchant";
	case 6:    return "Thief";
	case 7:    return "Knight";
	case 8:    return "Priest";
	case 9:    return "Wizard";
	case 10:   return "Blacksmith";
	case 11:   return "Hunter";
	case 12:   return "Assassin";
	case 13:   return "Peco knight";
	case 14:   return "Crusader";
	case 15:   return "Monk";
	case 16:   return "Sage";
	case 17:   return "Rogue";
	case 18:   return "Alchemist";
	case 19:   return "Bard";
	case 20:   return "Dancer";
	case 21:   return "Peco crusader";
	case 22:   return "Wedding";
	case 23:   return "Super Novice";
	case 24:   return "Gunslinger";
	case 25:   return "Ninja";
	case 26:   return "Xmas";
	case 4001: return "Novice High";
	case 4002: return "Swordsman High";
	case 4003: return "Mage High";
	case 4004: return "Archer High";
	case 4005: return "Acolyte High";
	case 4006: return "Merchant High";
	case 4007: return "Thief High";
	case 4008: return "Lord Knight";
	case 4009: return "High Priest";
	case 4010: return "High Wizard";
	case 4011: return "Whitesmith";
	case 4012: return "Sniper";
	case 4013: return "Assassin Cross";
	case 4014: return "Peco Lord Knight";
	case 4015: return "Paladin";
	case 4016: return "Champion";
	case 4017: return "Professor";
	case 4018: return "Stalker";
	case 4019: return "Creator";
	case 4020: return "Clown";
	case 4021: return "Gypsy";
	case 4022: return "Peco Paladin";
	case 4023: return "Baby Novice";
	case 4024: return "Baby Swordsman";
	case 4025: return "Baby Mage";
	case 4026: return "Baby Archer";
	case 4027: return "Baby Acolyte";
	case 4028: return "Baby Merchant";
	case 4029: return "Baby Thief";
	case 4030: return "Baby Knight";
	case 4031: return "Baby Priest";
	case 4032: return "Baby Wizard";
	case 4033: return "Baby Blacksmith";
	case 4034: return "Baby Hunter";
	case 4035: return "Baby Assassin";
	case 4036: return "Baby Peco Knight";
	case 4037: return "Baby Crusader";
	case 4038: return "Baby Monk";
	case 4039: return "Baby Sage";
	case 4040: return "Baby Rogue";
	case 4041: return "Baby Alchemist";
	case 4042: return "Baby Bard";
	case 4043: return "Baby Dancer";
	case 4044: return "Baby Peco Crusader";
	case 4045: return "Super Baby";
	case 4046: return "Taekwon Kid";
	case 4047: return "Taekwon Master";
	case 4048: return "Taekwon Master";
	case 4049: return "Soul Linker";
	case 4050: return "Bon Gun";
	case 4051: return "Death Knight";
	case 4052: return "Dark Collector";
	case 4053: return "Munak";
	}

	return "Unknown Job";
}

/*===============================================
 * This function logs all valid GM commands
 *-----------------------------------------------
 */
void log_atcommand(struct map_session_data *sd, const char *command, const char * param) {
	char tmpstr[200];
	unsigned char *sin_addr;

	sin_addr = (unsigned char *)&session[sd->fd]->client_addr.sin_addr;
	if (!param || !*param)
		sprintf(tmpstr, "%s [%d(lvl:%d)] (ip:%d.%d.%d.%d): %s" RETCODE,
		        sd->status.name, sd->status.account_id, sd->GM_level, sin_addr[0], sin_addr[1], sin_addr[2], sin_addr[3], command);
	else
		sprintf(tmpstr, "%s [%d(lvl:%d)] (ip:%d.%d.%d.%d): %s %s" RETCODE,
		        sd->status.name, sd->status.account_id, sd->GM_level, sin_addr[0], sin_addr[1], sin_addr[2], sin_addr[3], command, param);

	// send log to inter-server
	intif_send_log(LOG_ATCOMMAND, tmpstr); // 0x3008 <packet_len>.w <log_type>.B <message>.?B (types: 1 GM commands, 2: Trades, 3: Scripts, 4: Vending)

	return;
}

/*===============================================
 * This function returns the GM command symbol
 *-----------------------------------------------
 */
char GM_Symbol() {
	return command_symbol;
}

/*===============================================
 * This function returns the level of a GM command
 *-----------------------------------------------
 */
int get_atcommand_level(const AtCommandType type) {
	int i;

	for (i = 0; atcommand_info[i].type != AtCommand_Unknown; i++)
		if (atcommand_info[i].type == type)
			return atcommand_info[i].level;

	return 100; /* 100: command can not be used */
}

/*==========================================
 * This function checks chat for @commands
 *------------------------------------------
 */
AtCommandType is_atcommand(const int fd, struct map_session_data* sd, const char* message, unsigned char gmlvl) {
	const char* str;
	const char* p;
	int s_flag;
	int i;
	char command[100];
	char received_command[100]; // to display on error

	nullpo_retr(AtCommand_None, sd);

/* 	if muted, don't use GM command, commented for now */
/*	if (!battle_config.allow_atcommand_when_mute && sd->sc_count && sd->sc_data[SC_NOCHAT].timer != -1)
		return AtCommand_Unknown;*/

	if (!message || !*message || strncmp(message, sd->status.name, strlen(sd->status.name)) != 0)
		return AtCommand_None;

	/* search start of GM command */
	str = message + strlen(sd->status.name);
	s_flag = 0;
	while (*str && (isspace(*str) || (s_flag == 0 && *str == ':'))) {
		if (*str == ':')
			s_flag = 1;
		str++;
	}
	if (!*str || s_flag == 0) /* if no message or no ':' found */
		return AtCommand_None;

	/* check first char. */
	if (*str != command_symbol && *str != char_command_symbol && *str != main_channel_symbol)
		return AtCommand_None;

	/* get GM level */
	if (battle_config.atc_gmonly != 0 && !gmlvl)
		return AtCommand_None;

	/* extract gm command */
	p = str;
	while (*p && !isspace(*p))
		p++;
	i = p - str; /* speed up */
	if (i >= sizeof(command) - 5 || (i < 2 && *str != main_channel_symbol)) /* too long (-4 for @char + NULL, remote commands), or too short */
		return AtCommand_Unknown;
	memset(command, 0, sizeof(command));
	if (*str == char_command_symbol) {
		command[0] = command_symbol;
		command[1] = 'c';
		command[2] = 'h';
		command[3] = 'a';
		command[4] = 'r';
		strncpy(command + 5, str + 1, i - 1);
	} else if (*str == main_channel_symbol) {
		command[0] = command_symbol;
		command[1] = 'm';
		command[2] = 'a';
		command[3] = 'i';
		command[4] = 'n';
		p = str + 1;
	} else
		strncpy(command, str, i);
	// conserv received command to display on error
	memset(received_command, 0, sizeof(received_command));
	strncpy(received_command, str, i);
	/* for compare, reduce command in lowercase */
	for (i = 0; command[i]; i++)
		command[i] = tolower((unsigned char)command[i]); // tolower needs unsigned char

	// check for synonym here
	for (i = 0; i < synonym_count; i++) {
		if (strcmp(command + 1, synonym_table[i].synonym) == 0) {
			memset(command + 1, 0, sizeof(command) - 1); // don't change command_symbol (+1)
			strcpy(command + 1, synonym_table[i].command);
			break;
		}
	}

	/* search gm command type and check level*/
	i = 0;
	while (atcommand_info[i].type != AtCommand_Unknown) {
		if (strcmp(command + 1, atcommand_info[i].command + 1) == 0) {
			if (gmlvl < atcommand_info[i].level) {
				/* do loop until end for speed up */
				while (atcommand_info[i].type != AtCommand_Unknown)
					i++;
			}
			break;
		}
		i++;
	}
	/* if not found */
	if (atcommand_info[i].type == AtCommand_Unknown || atcommand_info[i].proc == NULL) {
		/* doesn't return Unknown if player is normal player (display the text, not display: unknown command) */
		if (gmlvl <= battle_config.atcommand_max_player_gm_level)
			return AtCommand_None;
		else {
			sprintf(atcmd_output, msg_txt(153), received_command); // %s is Unknown Command.
			clif_displaymessage(fd, atcmd_output);
			return AtCommand_Unknown;
		}
	}

	// check mingmlvl map flag
	if (gmlvl < (unsigned char)map[sd->bl.m].flag.mingmlvl) {
		switch (battle_config.mingmlvl_message) { // Which message do we send when a GM can use a command, but mingmlvl map flag block it?
		case 0: // Send no message (GM command is displayed like when GM speaks).
			return AtCommand_None;
		case 1: // Send 'unknown command'.
			sprintf(atcmd_output, msg_txt(153), received_command); // %s is Unknown Command.
			clif_displaymessage(fd, atcmd_output);
			return AtCommand_Unknown;
		case 2: // Send a special message (like: you are not authorized...).
		default:
			clif_displaymessage(fd, msg_txt(268)); // You're not authorized to use this command on this map. Sorry.
			return AtCommand_Unknown;
		}
	}

	/* search start of parameters */
	while (isspace(*p))
		p++;

	/* check parameter length */
	if (strlen(p) > 99) {
		clif_displaymessage(fd, msg_txt(49)); // Command with too long parameters.
		sprintf(atcmd_output, msg_txt(154), received_command); // %s failed.
		clif_displaymessage(fd, atcmd_output);
	} else if (atcommand_info[i].level >= map[sd->bl.m].flag.nogmcmd) {
		clif_displaymessage(fd, msg_txt(269)); // This map can't perform this GM command, sorry.
	/* do GM command */
	} else {
		command[0] = atcommand_info[i].command[0]; /* set correct first symbol for after (inside the function). */
		if (atcommand_info[i].proc(fd, sd, received_command, command, p) == 0) {
			// log command if necessary
			if (log_gm_level <= atcommand_info[i].level) {
				command[0] = command_symbol; /* to have correct display */
				log_atcommand(sd, command, p);
			}
		} else {
			/* Command can not be executed */
			command[0] = command_symbol; /* to have correct display */
			sprintf(atcmd_output, msg_txt(154), received_command); // %s failed.
			clif_displaymessage(fd, atcmd_output);
		}
	}

	return atcommand_info[i].type;
}

/*==========================================
 *
 *------------------------------------------
 */
static void atcommand_synonym_free(void) {
	int i;

	for (i = 0; i < synonym_count; i++) {
		FREE(synonym_table[i].synonym);
		FREE(synonym_table[i].command);
	}
	if (synonym_table != NULL) {
		FREE(synonym_table);
	}
	synonym_count = 0;

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
int atcommand_config_read(const char *cfgName) {
	static int read_counter = 0; // to count every time we enter in the function (for 'import' option)
	char line[512], w1[512], w2[512];
	int i, level;
	FILE* fp;

	/* init time of last spawn */
	last_spawn = time(NULL); /* # of seconds 1/1/1970 (timestamp): to limit number of spawn at 1 every 2 seconds (reduction of lag) */

	if ((fp = fopen(cfgName, "r")) == NULL) {
//		if ((fp = fopen("conf/atcommand_freya.conf", "r")) == NULL) { // not try default, possible infinite loop with import
			printf("At commands configuration file not found: %s\n", cfgName);
			return 1;
//		}
	}

	if (read_counter == 0) {
		atcommand_synonym_free();
		command_symbol = '@'; /* first char of the commands */
		char_command_symbol = '#'; /* first char of the remote commands */
		main_channel_symbol = '~'; /* first char of the main channel */
	}
	read_counter++;

	while (fgets(line, sizeof(line), fp)) { // fgets reads until maximum one less than size and add '\0' -> so, it's not necessary to add -1
		if ((line[0] == '/' && line[1] == '/') || line[0] == '\0' || line[0] == '\n' || line[0] == '\r')
			continue;
		// it's not necessary to remove 'carriage return ('\n' or '\r')

		memset(w2, 0, sizeof(w2));

		if (sscanf(line, "%[^:]:%s", w1, w2) == 2) {
			for (i = 0; w1[i]; i++) /* speed up, only 1 lowercase for all loops */
				w1[i] = tolower((unsigned char)w1[i]); // tolower needs unsigned char
			/* searching gm command */
			for (i = 0; atcommand_info[i].type != AtCommand_Unknown; i++)
				if (strcmp(atcommand_info[i].command + 1, w1) == 0)
					break;
// GM commands
			if (atcommand_info[i].type != AtCommand_Unknown) {
				level = atoi(w2);
				if (level > 100)
					level = 100;
				else if (level < 0)
					level = 0;
				atcommand_info[i].level = level;
			} else if (strcmp(w1, "command_symbol") == 0) {
				if (!iscntrl((int)w2[0]) && // w2[0] > 31 &&
				    w2[0] != '/' && // symbol of standard ragnarok GM commands
				    w2[0] != '%' && // symbol of party chat speaking
				    w2[0] != '$' && // symbol of guild chat speaking
				    w2[0] != char_command_symbol && // symbol of 'remote' GM commands
				    w2[0] != main_channel_symbol) // symbol of the Main channel
					command_symbol = w2[0];
			} else if (strcmp(w1, "char_command_symbol") == 0) {
				if (!iscntrl((int)w2[0]) && // w2[0] > 31 &&
				    w2[0] != '/' && // symbol of standard ragnarok GM commands
				    w2[0] != '%' && // symbol of party chat speaking
				    w2[0] != '$' && // symbol of guild chat speaking
				    w2[0] != command_symbol && // symbol of 'normal' GM commands
				    w2[0] != main_channel_symbol) // symbol of the Main channel
					char_command_symbol = w2[0];
			} else if (strcmp(w1, "main_channel_symbol") == 0) {
				if (!iscntrl((int)w2[0]) && // w2[0] > 31 &&
				    w2[0] != '/' && // symbol of standard ragnarok GM commands
				    w2[0] != '%' && // symbol of party chat speaking
				    w2[0] != '$' && // symbol of guild chat speaking
				    w2[0] != command_symbol && // symbol of 'normal' GM commands
				    w2[0] != char_command_symbol) // symbol of 'remote' GM commands
					main_channel_symbol = w2[0];
			} else if (strcmp(w1, "messages_filename") == 0) {
				memset(messages_filename, 0, sizeof(messages_filename));
				strncpy(messages_filename, w2, sizeof(messages_filename) - 1);
// import
			} else if (strcmp(w1, "import") == 0) {
				printf("atcommand_config_read: Import file: %s.\n", w2);
				atcommand_config_read(w2);
// unknown option/command
			} else {
				printf("Unknown GM command: '%s'.\n", w1);
			}

// Synonym commands
		} else if (sscanf(line, "%[^=]=%s", w1, w2) == 2) { // synonym
			for (i = 0; w1[i]; i++) /* speed up, only 1 lowercase for all loops */
				w1[i] = tolower((unsigned char)w1[i]); // tolower needs unsigned char
			/* searching if synonym is not a gm command */
			for (i = 0; atcommand_info[i].type != AtCommand_Unknown; i++)
				if (strcmp(atcommand_info[i].command + 1, w1) == 0) {
					printf("Error in %s file: GM synonym '%s' is not a synonym, but a GM command.\n", cfgName, w1);
					break;
				}
			// if synonym is ok
			if (atcommand_info[i].type == AtCommand_Unknown) {
				for (i = 0; w2[i]; i++) /* speed up, only 1 lowercase for all loops */
					w2[i] = tolower((unsigned char)w2[i]); // tolower needs unsigned char
				/* searching if gm command exists */
				for (i = 0; atcommand_info[i].type != AtCommand_Unknown; i++)
					if (strcmp(atcommand_info[i].command + 1, w2) == 0) {
						// GM command found, create synonym
						//printf("new synonym: %s->%s\n", w1, w2);
						if (synonym_count == 0) {
							CALLOC(synonym_table, struct synonym_table, 1);
						} else {
							REALLOC(synonym_table, struct synonym_table, synonym_count + 1);
						}
						for (i = 0; w1[i]; i++) /* speed up, only 1 lowercase for all loops */
							w1[i] = tolower((unsigned char)w1[i]); // tolower needs unsigned char
						CALLOC(synonym_table[synonym_count].synonym, char, strlen(w1) + 1);
						strcpy(synonym_table[synonym_count].synonym, w1);
						for (i = 0; w2[i]; i++) /* speed up, only 1 lowercase for all loops */
							w2[i] = tolower((unsigned char)w2[i]); // tolower needs unsigned char
						CALLOC(synonym_table[synonym_count].command, char, strlen(w2) + 1);
						strcpy(synonym_table[synonym_count].command, w2);
						synonym_count++;
						break;
					}
				if (atcommand_info[i].type == AtCommand_Unknown)
					printf("Error in %s file: GM command '%s' of synonym '%s' doesn't exist.\n", cfgName, w2, w1);
			}
		}
	}
	fclose(fp);

	printf(CL_GREEN "Loaded: " CL_RESET "File '" CL_WHITE "%s" CL_RESET "' read.\n", cfgName);

	read_counter--;
	if (read_counter == 0) {
		printf(CL_WHITE "Server: " CL_RESET "Atcommand Symbols:\n");
		printf(CL_WHITE "Server: " CL_RESET "'" CL_WHITE "%c" CL_RESET "' for GM commands.\n", command_symbol);
		printf(CL_WHITE "Server: " CL_RESET "'" CL_WHITE "%c" CL_RESET "' for char GM commands.\n", char_command_symbol);
		printf(CL_WHITE "Server: " CL_RESET "'" CL_WHITE "%c" CL_RESET "' for main channel.\n", main_channel_symbol);
	}

	return 0;
}

//--------------------------------------------------
// Return the message string of the specified number
//--------------------------------------------------
char * msg_txt(int msg_number) {
	if (msg_number >= 0 && msg_number < MSG_NUMBER && msg_table[msg_number] != NULL && msg_table[msg_number][0] != '\0')
		return msg_table[msg_number];

	return "<no message>";
}

/*==========================================
 * Add Message in table
 *------------------------------------------
 */
void add_msg(int msg_number, const char *Message) {
	int len, i, var1, var2;

	len = strlen(Message);
	if (len > 0) {
		if (len > MAX_MSG_LEN)
			len = MAX_MSG_LEN;
		if (msg_config_read_done) { /* if already read, some checks */
			if (msg_table[msg_number] == NULL) {
				printf(CL_YELLOW "Warning: Added the message #%d. By default this message doesn't exist." CL_RESET"\n", msg_number);
			/* else, check number of % */
			} else {
				var1 = 0;
				for(i = 0; msg_table[msg_number][i]; i++)
					if (msg_table[msg_number][i] == '%') {
						if (msg_table[msg_number][i + 1] == '%') { /* if %% to display % */
							i++;
						} else {
							var1++;
						}
					}
				var2 = 0;
				for(i = 0; Message[i] && i <= len; i++)
					if (Message[i] == '%') {
						if (Message[i + 1] == '%') { /* if %% to display % */
							i++;
						} else {
							var2++;
						}
					}
				if (var1 != var2) {
					printf(CL_YELLOW "Warning: Added the message #%d with %d variable%s and replaced the message have %d variable%s." CL_RESET" Note: a variable begin by %%.\n", msg_number, var2, (var2 > 1) ? "s" : "", var1, (var1 > 1) ? "s" : "");
				}
			}
		}
		/* add new message */
		FREE(msg_table[msg_number]);
		CALLOC(msg_table[msg_number], char, len + 1); // + NULL
		strncpy(msg_table[msg_number], Message, len);
	} else {
		printf(CL_RED "Error in conf file: Attempted to add the void message #%d." CL_RESET"\n", msg_number);
	}

	return;
}

/*==========================================
 * Set default messages
 *------------------------------------------
 */
void set_default_msg() {
	// Messages of GM commands
	add_msg(  0, "Warped.");
	add_msg(  1, "Map not found.");
	add_msg(  2, "Coordinates out of range.");
	add_msg(  3, "Player not found.");
	add_msg(  4, "Jumped to '%s'.");
	add_msg(  5, "Jumped to %d %d");
	add_msg(  6, "Player data respawn point saved.");
	add_msg(  7, "Warping to respawn point.");
	add_msg(  8, "Speed changed.");
	add_msg(  9, "Options changed.");
	add_msg( 10, "Invisible: Off.");
	add_msg( 11, "Invisible: On.");
	add_msg( 12, "Your job has been changed.");
	add_msg( 13, "What a pity! You've died.");
	add_msg( 14, "Player killed.");
	add_msg( 15, "Player warped (message sends to player too).");
	add_msg( 16, "You've been revived! It's a miracle!");
	add_msg( 17, "HP, SP recovered.");
	add_msg( 18, "Item created.");
	add_msg( 19, "Invalid item ID or name.");
	add_msg( 20, "All of your inventory items have been removed.");
	add_msg( 21, "Base level raised.");
	add_msg( 22, "Base level lowered.");
	add_msg( 23, "Job level can't go any higher.");
	add_msg( 24, "Job level raised.");
	add_msg( 25, "Job level lowered.");
	add_msg( 26, "Help commands:");
	add_msg( 27, "File 'help.txt' not found.");
	add_msg( 28, "No players found.");
	add_msg( 29, "1 player found.");
	add_msg( 30, "%d players found.");
	add_msg( 31, "PvP: Off.");
	add_msg( 32, "PvP: On.");
	add_msg( 33, "GvG: Off.");
	add_msg( 34, "GvG: On.");
	add_msg( 35, "Unable to use palette with class.");
	add_msg( 36, "Appearance changed.");
	add_msg( 37, "Invalid number specified.");
	add_msg( 38, "Invalid location number or name.");
	add_msg( 39, "Monsters summoned!");
	add_msg( 40, "Invalid monster ID or name.");
	add_msg( 41, "Unable to decrease the number/value.");
	add_msg( 42, "Stat(s) changed.");
	add_msg( 43, "Player is not in a guild.");
	add_msg( 44, "Player is not the master of the guild.");
	add_msg( 45, "Guild level change failed.");
	add_msg( 46, "'%s' recalled!");
	add_msg( 47, "Base level unable to go any higher.");
	add_msg( 48, "Player's job changed.");
	add_msg( 49, "Command has too long of parameters.");
	add_msg( 50, "You already have some GM powers.");
	add_msg( 51, "Player revived.");
	add_msg( 52, "This option cannot be used in PK Mode.");
	add_msg( 53, "'%s' (account: %d) stats:");
	add_msg( 54, "No player(s) found in map '%s'.");
	add_msg( 55, "1 player found in map '%s'.");
	add_msg( 56, "%d players found in map '%s'.");
	add_msg( 57, "Player's respawn point changed.");
	add_msg( 58, "Player's options changed.");
	add_msg( 59, "Night has fallen.");
	add_msg( 60, "Day has arrived.");
	add_msg( 61, "The holy messenger has given judgement.");
	add_msg( 62, "Judgement was made.");
	add_msg( 63, "Mercy has been shown.");
	add_msg( 64, "Mercy has been granted.");
	add_msg( 65, "Character's base level raised.");
	add_msg( 66, "Character's base level lowered.");
	add_msg( 67, "Character's job level is unable go any higher.");
	add_msg( 68, "character's job level raised.");
	add_msg( 69, "Character's job level lowered.");
	add_msg( 70, "Skill has been learned.");
	add_msg( 71, "Skill has been forgotten.");
	add_msg( 72, "Guild siege warfare started!");
	add_msg( 73, "Guild siege warfare has already been started.");
	add_msg( 74, "Guild siege warfare has ended!");
	add_msg( 75, "Guild siege warfare has not started yet.");
	add_msg( 76, "You have received all skills.");
	add_msg( 77, "The reference result of '%s' (name: id):");
	add_msg( 78, "%s: %d");
	add_msg( 79, "It is %d affair above.");
	add_msg( 80, "Enter a proper display name and monster name/id.");
	add_msg( 81, "GM level does not authorize you to do action.");
	add_msg( 82, "Enter one of these numbers/names:");
	add_msg( 83, "Cannot spawn Emperium.");
	add_msg( 84, "All stats changed!");
	add_msg( 85, "Invalid time for ban command.");
	add_msg( 86, "Sorry, but a player name must have at least 4 characters.");
	add_msg( 87, "Sorry, but a player name can only have 23 characters maximum.");
	add_msg( 88, "Char-server checking character name..");
	add_msg( 89, "Night-mode is already active.");
	add_msg( 90, "Day-mode is already active.");
	add_msg( 91, "Character's base level is at its maximum.");
	add_msg( 92, "All online players recalled!");
	add_msg( 93, "All online players of the '%s' guild recalled.");
	add_msg( 94, "Incorrect name/ID, or no one from the guild is online.");
	add_msg( 95, "All online players of the '%s' party recalled.");
	add_msg( 96, "Incorrect name or ID, or no one from the party is online.");
	add_msg( 97, "Item database reloaded.");
	add_msg( 98, "Monster database reloaded.");
	add_msg( 99, "Skill database reloaded.");
	add_msg(100, "Npc scripts reloaded.");
	add_msg(101, "Login-server requested to reload GM accounts and corresponding GM levels.");
	add_msg(102, "Mounted Peco.");
	add_msg(103, "No longer spying on the '%s' guild.");
	add_msg(104, "Initialized spying on the '%s' guild.");
	add_msg(105, "No longer spying on the '%s' party.");
	add_msg(106, "Initialized spying on the '%s' party.");
	add_msg(107, "All broken items have been repaired.");
	add_msg(108, "No broken items present.");
	add_msg(109, "Player has been nuked!");
	add_msg(110, "Npc Enabled.");
	add_msg(111, "Npc does not exist.");
	add_msg(112, "Npc Disabled.");
	add_msg(113, "%d item(s) removed by GM.");
	add_msg(114, "%d item(s) removed from the player.");
	add_msg(115, "%d item(s) removed. Player had only %d on %d items.");
	add_msg(116, "Character does not have the item.");
	add_msg(117, "A GM has jailed you for %s.");
	add_msg(118, "Player warped to jail for %s.");
	add_msg(119, "Player is not currently in jail.");
	add_msg(120, "A GM has discharged you from jail.");
	add_msg(121, "Player warped to Prontera.");
	add_msg(122, "Disguise applied.");
	add_msg(123, "Monster/NPC name/id not found.");
	add_msg(124, "Disguise removed.");
	add_msg(125, "Disguise is not currently in effect.");
	add_msg(126, "You accept all whispers (no whisper is refused).");
	add_msg(127, "You accept all whispers, except thoses from %d player(s):");
	add_msg(128, "You refuse all whispers (no specific whisper is refused).");
	add_msg(129, "You refuse all whispers, AND refuse whispers from %d player(s):");
	add_msg(130, "'%s' accept all whispers (no wisper is refused).");
	add_msg(131, "'%s' accept all whispers, except thoses from %d player(s):");
	add_msg(132, "'%s' refuse all whispers (no specific whisper is refused).");
	add_msg(133, "'%s' refuse all whispers, AND refuse whispers from %d player(s):");
	add_msg(134, "'%s' already accepts all whispers.");
	add_msg(135, "'%s' now accepts all whispers.");
	add_msg(136, "A GM has authorized all whispers for you.");
	add_msg(137, "'%s' already blocks all whispers.");
	add_msg(138, "'%s' now blocks all whispers.");
	add_msg(139, "A GM has blocked all whispers for you.");
	add_msg(140, "Disguise applied.");
	add_msg(141, "Disguise removed.");
	add_msg(142, "Disguisse is not currently in effect.");
	add_msg(143, "Enter a proper monster name/id.");
	add_msg(144, "Invalid e-mail. If you have default e-mail, type a@a.com.");
	add_msg(145, "Invalid new e-mail. Please enter a correct e-mail.");
	add_msg(146, "New e-mail must be a real e-mail.");
	add_msg(147, "New e-mail must be different from the original e-mail.");
	add_msg(148, "Information sent to login-server via char-server.");
	add_msg(149, "Unable to increase the number/value.");
	add_msg(150, "No GM found.");
	add_msg(151, "1 GM found.");
	add_msg(152, "%d GMs found.");
	add_msg(153, "%s is an unknown command.");
	add_msg(154, "%s failed.");
	add_msg(155, "Unable to change job.");
	add_msg(156, "HP and/or SP modified.");
	add_msg(157, "HP and SP are currently full.");
	add_msg(158, "Base level is at its minimum.");
	add_msg(159, "Job level is at its minimum.");
	add_msg(160, "PvP is already Off.");
	add_msg(161, "PvP is already On.");
	add_msg(162, "GvG is already Off.");
	add_msg(163, "GvG is already On.");
	add_msg(164, "Memo point #%d doesn't exist.");
	add_msg(165, "Monsters killed!");
	add_msg(166, "No items have been refined!");
	add_msg(167, "1 item has been refined!");
	add_msg(168, "%d items have been refined!");
	add_msg(169, "Item (%d: '%s') is not an equipment.");
	add_msg(170, "Item is not an equipment.");
	add_msg(171, "%d - void");
	add_msg(172, "Previous memo point replaced: %d - %s (%d,%d).");
	add_msg(173, "Warp skill not present for memo command.");
	add_msg(174, "Number of status points changed!");
	add_msg(175, "Number of skill points changed!");
	add_msg(176, "Zeny amount changed!");
	add_msg(177, "Unable to decrease stat.");
	add_msg(178, "Unable to increase stat.");
	add_msg(179, "Guild level modified.");
	add_msg(180, "Monter/egg name/id doesn't exist.");
	add_msg(181, "You already have a pet.");
	add_msg(182, "Pet friendliness value changed!");
	add_msg(183, "Pet friendliness is already the good value.");
	add_msg(184, "Pet is not present.");
	add_msg(185, "Pet hunger value changed!");
	add_msg(186, "Pet hunger is already the good value.");
	add_msg(187, "Pet is now rename-able.");
	add_msg(188, "Pet is already name-able.");
	add_msg(189, "Pet is now rename-able.");
	add_msg(190, "Pet is already name-able.");
	add_msg(191, "Pet is not present.");
	add_msg(192, "Unable to change the character's job.");
	add_msg(193, "Character's base level is at its minimum.");
	add_msg(194, "Character's job level is at its minimum.");
	add_msg(195, "All players have been kicked!");
	add_msg(196, "Quest skills has already been obtained.");
	add_msg(197, "Skill number doesn't exist or isn't a quest skill.");
	add_msg(198, "Skill number doesn't exist.");
	add_msg(199, "Character has learned skill.");
	add_msg(200, "Character has already learned skill.");
	add_msg(201, "Skill is not present.");
	add_msg(202, "Skill has been removed.");
	add_msg(203, "Skill is not present");
	add_msg(204, "WARNING: More than 400 Spirit Balls can CRASH your client!");
	add_msg(205, "You already have this number of Spirit Balls.");
	add_msg(206, "'%s' skill points reset!");
	add_msg(207, "'%s' stats points reset!");
	add_msg(208, "'%s' skill and stats points reseted!");
	add_msg(209, "Character's number of skill points modified!");
	add_msg(210, "Character's number of status points modified!");
	add_msg(211, "Character's zeny amount changed!");
	add_msg(212, "Cannot mount Peco while in disguise.");
	add_msg(213, "Cannot mount peco with current job.");
	add_msg(214, "Unmounted Peco.");
	add_msg(215, "Player cannot mount a Peco while in disguise.");
	add_msg(216, "Player has mounted a Peco.");
	add_msg(217, "Player cannot mount a Peco with his/her job.");
	add_msg(218, "Player has already mounted a Peco.");
	add_msg(219, "%d day");
	add_msg(220, "%d days");
	add_msg(221, "%s %d hour");
	add_msg(222, "%s %d hours");
	add_msg(223, "%s %d minute");
	add_msg(224, "%s %d minutes");
	add_msg(225, "%s and %d second");
	add_msg(226, "%s and %d seconds");
	add_msg(227, "Cannot be disguised when riding a Peco.");
	add_msg(228, "Character cannot be disguised when riding a Peco.");
	add_msg(229, "Effect has changed.");
	add_msg(230, "Server time (normal time): %A, %B %d %Y %X.");
	add_msg(231, "Game time: The game is in permanent daylight.");
	add_msg(232, "Game time: The game is in permanent night.");
	add_msg(233, "Game time: The game is actualy in night for %s.");
	add_msg(234, "Game time: After, the game will be in permanent daylight.");
	add_msg(235, "Game time: The game is actualy in daylight for %s.");
	add_msg(236, "Game time: After, the game will be in permanent night.");
	add_msg(237, "Game time: After, the game will be in night for %s.");
	add_msg(238, "Game time: A day cycle has a normal duration of %s.");
	add_msg(239, "Game time: After, the game will be in daylight for %s.");
	add_msg(240, "%d monster(s) summoned!");
	add_msg(241, "All inventory items of the character have been removed.");
	add_msg(242, "All inventory items have been checked.");
	add_msg(243, "Skills can no longer be used on this map.");
	add_msg(244, "Skills can now be used on this map.");
	add_msg(245, "Server Uptime: %s.");
	add_msg(246, "Message of the day:");
	add_msg(247, "Display option (HP of players) is now set to OFF.");
	add_msg(248, "Display option (HP of players) is now set to ON.");
	add_msg(249, "Display experience option is now set to OFF.");
	add_msg(250, "Display experience option is now set to ON.");
	add_msg(251, "Request has not been sent. Request system is disabled.");
	add_msg(252, "Request has been sent. If there are no GMs is online, request will be lost.");
	add_msg(253, "(Map Message)");

	add_msg(254, "List of monsters (with current drop rate) that drop '%s (%s)' (id: %d):");
	add_msg(255, "No monster(s) drops item '%s (%s)' (id: %d).");
	add_msg(256, "1 monster drops item '%s (%s)' (id: %d).");
	add_msg(257, "%d monster(s) drop item '%s (%s)' (id: %d).");

	add_msg(258, "Invalid party name. Party name must have between 1 to 24 characters.");
	add_msg(259, "Invalid guild name. Guild name must have between 1 to 24 characters.");

	add_msg(260, "You're now a killer.");
	add_msg(261, "You're no longer a killer.");
	add_msg(262, "Player is now a killer.");
	add_msg(263, "Player is no longer a killer.");
	add_msg(264, "You're now killable.");
	add_msg(265, "You're no longer killable.");
	add_msg(266, "Player is now killable.");
	add_msg(267, "Player is no longer killable.");

	add_msg(268, "You're not authorized to use this command on this map.");
	add_msg(269, "Current map restricts you from performing this GM command.");

	add_msg(270, "Current autoloot option is disabled.");
	add_msg(271, "Current autoloot option is set to get rate between 0 to %0.02f%%.");
	add_msg(272, "Autoloot option for looted items is now set to OFF.");
	add_msg(273, "Autoloot option for looted items is now set to ON.");
	add_msg(274, "You now receive drops of looted items.");
	add_msg(275, "You no longer receive drops looted items.");
	add_msg(276, "Invalid drop rate. Value must be between 0 (no autoloot) to 100 (autoloot all drops).");
	add_msg(277, "Autoloot of monster drops disabled.");
	add_msg(278, "Set autoloot of monster drops when they are between 0 to %0.02f%%.");

	add_msg(279, "Invalid coordinates.");
	add_msg(280, "Invalid coordinates (an Npc is already at this position).");
	add_msg(281, "Npc's coordinates have been changed.");

	add_msg(282, "Invalid color.");

	// To-Do: Remove..
	add_msg(283, "Shuffle done!");

	add_msg(284, "This item cannot be traded.");
	add_msg(285, "This item cannot be dropped.");
	add_msg(286, "This item cannot be stored.");
	add_msg(287, "Some of your items cannot be vended and were removed from the shop.");

	// Messages of others (not for GM commands)
	add_msg(500, "It's night...");
	add_msg(501, "Your account time limit is: %d-%m-%Y %H:%M:%S.");
	add_msg(502, "The day has arrived!");
	add_msg(503, "The night has fallen...");
	add_msg(504, "Hack on global message (normal message): character '%s' (account: %d) uses an other name.");
	add_msg(505, " This player sends a void name and a void message.");
	add_msg(506, " This player sends (name:message): '%s'.");
	add_msg(507, " This player has been banned for %d minute(s).");
	add_msg(508, " This player hasn't been banned (Ban option is disabled).");
	add_msg(509, "You can not page yourself. Sorry.");
	add_msg(510, "Unknown monster or item.");
	add_msg(511, "Muting is disabled.");
	add_msg(512, "It's impossible to block this player.");
	add_msg(513, "It's impossible to unblock this player.");
	add_msg(514, "This player is already blocked.");
	add_msg(515, "Character '%s' (account: %d) has tried AGAIN to block wisps from '%s' (wisp name of the server). Bot user? Check please.");
	add_msg(516, "Character '%s' (account: %d) has tried to block wisps from '%s' (wisp name of the server). Bot user?");
	add_msg(517, "Add me in your ignore list, doesn't block my wisps.");
	add_msg(518, "You can not block more people.");
	add_msg(519, "This player is not blocked by you.");
	add_msg(520, "You already block everyone.");
	add_msg(521, "You already allow everyone.");
	add_msg(522, "This name (for a friend) doesn't exist.");
	add_msg(523, "This player is already a friend.");
	add_msg(524, "Please wait. This player must already answer to an invitation.");
	add_msg(525, "Friend name not found in list.");
	add_msg(526, "Change sex failed. Account %d not found.");
	add_msg(527, "Sex of '%s' changed.");
	add_msg(528, "Sex of account %d changed.");
	add_msg(529, "You cannot wear disguise when riding a Peco.");
	add_msg(530, "Item has been repaired.");
	add_msg(531, "%s stole an unknown item.");
	add_msg(532, "%s stole a(n) %s.");
	add_msg(533, "%s has not stolen the item because of being overweight.");
	add_msg(534, "You cannot mount a peco while in disguise.");
	add_msg(535, "Alliances cannot be made during Guild Wars!");
	add_msg(536, "Alliances cannot be broken during Guild Wars!");
	add_msg(537, "You broke target's weapon.");
	add_msg(538, "Hack on trade: character '%s' (account: %d) try to trade more items that he has.");
	add_msg(539, "This player has %d of a kind of item (id: %d), and try to trade %d of them.");
	add_msg(540, " This player has been definitivly blocked.");
	add_msg(541, "(to GM >= %d) ");
	add_msg(542, "The player '%s' doesn't exist.");
	add_msg(543, "Login-server has been asked to block the player '%s'.");
	add_msg(544, "Your GM level don't authorize you to block the player '%s'.");
	add_msg(545, "Login-server is offline. Impossible to block the player '%s'.");
	add_msg(546, "Login-server has been asked to ban the player '%s'.");
	add_msg(547, "Your GM level don't authorize you to ban the player '%s'.");
	add_msg(548, "Login-server is offline. Impossible to ban the player '%s'.");
	add_msg(549, "Login-server has been asked to unblock the player '%s'.");
	add_msg(550, "Your GM level don't authorize you to unblock the player '%s'.");
	add_msg(551, "Login-server is offline. Impossible to unblock the player '%s'.");
	add_msg(552, "Login-server has been asked to unban the player '%s'.");
	add_msg(553, "Your GM level don't authorize you to unban the player '%s'.");
	add_msg(554, "Login-server is offline. Impossible to unban the player '%s'.");
	add_msg(555, "Login-server has been asked to change the sex of the player '%s'.");
	add_msg(556, "Your GM level don't authorize you to change the sex of the player '%s'.");
	add_msg(557, "Login-server is offline. Impossible to change the sex of the player '%s'.");
	add_msg(558, "GM modification success.");
	add_msg(559, "Failure of GM modification.");
	add_msg(560, "Your sex has been changed (need disconnection by the server)...");
	add_msg(561, "Your account has been deleted (disconnection)...");
	add_msg(562, "Your account has 'Unregistered'.");
	add_msg(563, "Your account has an 'Incorrect Password'...");
	add_msg(564, "Your account has expired.");
	add_msg(565, "Your account has been rejected from server.");
	add_msg(566, "Your account has been blocked by the GM Team.");
	add_msg(567, "Your Game's EXE file is not the latest version.");
	add_msg(568, "Your account has been prohibited to log in.");
	add_msg(569, "Server is jammed due to over populated.");
	add_msg(570, "Your account has not more authorized.");
	add_msg(571, "Your account has been totally erased.");
	add_msg(572, "Your account has been banished until ");
	add_msg(573, "%m-%d-%Y %H:%M:%S");

	// Mail System (Need to remove)
	//----------------------
	add_msg(574, "You have no message.");
	add_msg(575, "%d - From : %s (New - Priority)");
	add_msg(576, "%d - From : %s (New)");
	add_msg(577, "%d - From : %s");
	add_msg(578, "You have %d new messages.");
	add_msg(579, "You have %d unread priority messages.");
	add_msg(580, "You have no new message.");
	add_msg(581, "Message not found.");
	add_msg(582, "Reading message from %s");
	add_msg(583, "Cannot delete unread priority message.");
	add_msg(584, "You have recieved new messages, use @listmail before deleting.");
	add_msg(585, "Message deleted.");
	add_msg(586, "You must wait 10 minutes before sending another message.");
	add_msg(587, "Access Denied.");
	add_msg(588, "Character does not exist.");
	add_msg(589, "Message has been sent.");
	add_msg(590, "You have new message.");

	add_msg(591, "Don't flood server with emotion icons, please.");
	add_msg(592, "Trade can not be done, because one of your doesn't have enough free slots in its inventory.");
	add_msg(593, "(to all players) ");
	add_msg(594, "Experience gained: Base:%d, Job:%d.");
	add_msg(595, "trade done");
	add_msg(596, "gives");
	add_msg(597, "lvl");
	add_msg(598, "card(s)");
	add_msg(599, "1 zeny trades by this player.");
	add_msg(600, "zenys trade by this player.");
	add_msg(601, "No zeny trades by this player.");
	add_msg(602, "Nothing trades by this player.");
	add_msg(603, "[Main] '%s': ");
	add_msg(604, "(Request from '%s'): ");

	// Supernovice's Guardian Angel
	// actually. new client msgtxt file contains these 3 lines...
	add_msg(605, "Guardian Angel, can you hear my voice? ^^;");
	add_msg(606, "My name is %s, and I'm a Super Novice~");
	add_msg(607, "Please help me~ T.T");

	add_msg(608, "Main channel is now OFF.");
	add_msg(609, "Main channel is now ON.");
	add_msg(610, "Experience gained: Base:%d (%2.02f%%), Job:%d (%2.02f%%).");
	add_msg(611, "Unable to read motd.txt file.");
	add_msg(612, "You are not active. In order to provide better service to other players, you will be disconnected in 30 seconds.");
	add_msg(613, "Disconnected due to inactivity.");

	add_msg(614, "Vending done for %d z");
	add_msg(615, "Vender");
	add_msg(616, "Buyer");
	add_msg(617, "price: %d x %d z = %d z.");
	add_msg(618, "price: %d z.");

	add_msg(619, "Your e-mail has NOT been changed (impossible to change it).");
	add_msg(620, "Your e-mail has been changed to '%s'.");

	add_msg(621, "Possible use of BOT (99%% of chance) or modified client by '%s' (account: %d, char_id: %d). This player ask your name when you are hidden.");
	add_msg(622, "Character '%s' (account: %d) try to use a bot (it tries to detect a fake player).");
	add_msg(623, "Character '%s' (account: %d) try to use a bot (it tries to detect a fake mob).");

	add_msg(624, "You have been discharged.");
	add_msg(625, "Invalid time for jail command.");
	add_msg(626, "Remember, you are in jail.");
	add_msg(627, "Invalid final time for jail command.");
	add_msg(628, "No change done for jail time of this player.");
	add_msg(629, "Jail time of the player mofified by %+d second%s.");
	add_msg(630, "Map error, please reconnect.");
	add_msg(631, "%s has been discharged from jail.");
	add_msg(632, "A GM has discharged %s from jail.");
	add_msg(633, "Character is now in jail for %s.");
	add_msg(634, "Your jail time has been modified by %+d second%s.");
	add_msg(635, "You are now in jail for %s.");
	add_msg(636, "You are actually in jail for %s.");
	add_msg(637, "You are not in jail.");
	add_msg(638, "This player is actually in jail for %s.");
	add_msg(639, "This player is not in jail.");
	add_msg(640, "%s has been put in jail for %s.");

	add_msg(641, "Player hasn't been banned.");
	add_msg(642, "Possible hack on global message (normal message): character '%s' (account: %d) tries to send a big message (%d characters).");
	add_msg(643, "Possible hack on wisp message (PM): character '%s' (account: %d) tries to send a big message (%d characters).");
	add_msg(644, "Possible hack on GM message: character '%s' (account: %d) tries to send a big message (%d characters).");
	add_msg(645, "Possible hack on local GM message: character '%s' (account: %d) tries to send a big message (%d characters).");
	add_msg(646, "Possible hack on party message: character '%s' (account: %d) tries to send a big message (%d characters).");
	add_msg(647, "Possible hack on guild message: character '%s' (account: %d) tries to send a big message (%d characters).");

	add_msg(648, "A merchant with a vending shop can not join a chat, sorry.");

	add_msg(649, "You have too much zeny. You can not get more zeny.");

	add_msg(650, "Rudeness is not authorized in this game.");
	add_msg(651, "Rudeness is not a good way to have fun in this game.");

	add_msg(652, "Hack on party message: character '%s' (account: %d) uses an other name.");
	add_msg(653, "Hack on guild message: character '%s' (account: %d) uses an other name.");

	add_msg(654, "You are not authorized to use '\"' in a guild name.");

	add_msg(655, "Sorry, but trade is not allowed on this map.");

	add_msg(656, "Monster dropped %d %s (drop rate: %%%0.02f).");
	add_msg(657, "Invalid drop rate. Value must be between 0 (no display) to 100 (display all drops).");
	add_msg(658, "Set displaying of monster drops when they are between 0 to %0.02f%%.");
	add_msg(659, "Displaying of monster drops disabled.");
	add_msg(660, "Monster dropped %d %s (it was a loot).");
	add_msg(661, "Your current display drop option is disabled.");
	add_msg(662, "Your current display drop option is set to display rate between 0 to %0.02f%%.");

	add_msg(663, "You display now drops of looted items.");
	add_msg(664, "You don't more display drops of looted items.");
	add_msg(665, "And you display drops of looted items.");
	add_msg(666, "And you don't display drops of looted items.");

	add_msg(667, "Server has adjusted your speed to your follow target.");

	add_msg(668, "(From %s) The War Of Emperium is running...");

	add_msg(669, "%s has broken.");
	
	add_msg(670, "You can not create a chat when you are in one.");
	add_msg(671, "Hack on NPC: Player %s (%d:%d) tried to buy %d of nonstackable item %d!.");

	add_msg(672, "New password must be different from the old password.");
	add_msg(673, "Your password has NOT been changed (impossible to change it).");
	add_msg(674, "Your password has been changed to '%s'.");

	add_msg(675, "Login-server has been asked to change the GM level of the player '%s'.");
	add_msg(676, "Your GM level don't authorize you to change the GM level of the player '%s'.");
	add_msg(677, "Login-server is offline. Impossible to change the GM level of the player '%s'.");
	add_msg(678, "Player (account: %d) that you want to change the GM level doesn't exist.");
	add_msg(679, "You are not authorised to change the GM level of this player (account: %d).");
	add_msg(680, "The player (account: %d) already has the specified GM level.");
	add_msg(681, "GM level of the player (account: %d) changed to %d.");

	add_msg(682, "For the record: War of Emperium is actually running. Because you are member of a guild, you can not use 'Main channel'.");
	add_msg(683, "War of Emperium is actually running. Because you are member of a guild, you can not use 'Main channel'.");
	add_msg(684, "For the record: War of Emperium is actually running. Because you are member of a guild, you can not use 'Main channel' on GvG maps.");
	add_msg(685, "War of Emperium is actually running. Because you are member of a guild, you can not use 'Main channel' on GvG maps.");

	add_msg(686, "Server (special action): you lost %ld zenys.");
	add_msg(687, "Server (special action): you gain %ld zenys.");
	add_msg(688, "Server (special action): you lost %ld %s.");
	add_msg(689, "Server (special action): you obtain %ld %s.");

	return;
}

/*==========================================
 * Read Message Data
 *------------------------------------------
 */
int msg_config_read(const char *cfgName) {
	int msg_number;
	char line[MAX_MSG_LEN], w1[MAX_MSG_LEN], w2[MAX_MSG_LEN];
	int i;
	FILE *fp;

	if (msg_config_read_done == 0) {
		memset(&msg_table[0], 0, sizeof(msg_table[0]) * MSG_NUMBER);
		set_default_msg();
		msg_config_read_done = 1;
	}

	if ((fp = fopen(cfgName, "r")) == NULL) {
//		if ((fp = fopen("conf/msg_freya.conf", "r")) == NULL) { // not try default, possible infinite loop with import
			printf("Messages file not found: %s.\n", cfgName);
			// prepare check for supernovice angel
			for (i = 0; msg_table[605][i]; i++) // Guardian Angel, can you hear my voice? ^^;
				msg_table[605][i] = tolower((unsigned char)msg_table[605][i]); // tolower needs unsigned char
			for (i = 0; msg_table[607][i]; i++) // Please help me~ T.T
				msg_table[607][i] = tolower((unsigned char)msg_table[607][i]); // tolower needs unsigned char
			return 1;
//		}
	}

	while(fgets(line, sizeof(line), fp)) { // fgets reads until maximum one less than size and add '\0' -> so, it's not necessary to add -1
		if ((line[0] == '/' && line[1] == '/') || line[0] == '\0' || line[0] == '\n' || line[0] == '\r')
			continue;
		/* remove carriage return if exist */
		while(line[0] != '\0' && (line[(i = strlen(line) - 1)] == '\n' || line[i] == '\r'))
			line[i] = '\0';
		// it's not necessary to remove 'carriage return ('\n' or '\r')

		memset(w2, 0, sizeof(w2));
		if (sscanf(line, "%[^:]: %[^\r\n]", w1, w2) == 2) {
// import
			if (strcasecmp(w1, "import") == 0) {
				printf("msg_config_read: Import file: %s.\n", w2);
				msg_config_read(w2);
			} else {
				msg_number = atoi(w1);
				if (msg_number >= 0 && msg_number < MSG_NUMBER) {
					add_msg(msg_number, w2);
				//	printf("Message #%d: '%s'.\n", msg_number, msg_table[msg_number]);
				}
			}
		}
	}
	fclose(fp);

	// prepare check for supernovice angel
	for (i = 0; msg_table[605][i]; i++) // Guardian Angel, can you hear my voice? ^^;
		msg_table[605][i] = tolower((unsigned char)msg_table[605][i]); // tolower needs unsigned char
	for (i = 0; msg_table[607][i]; i++) // Please help me~ T.T
		msg_table[607][i] = tolower((unsigned char)msg_table[607][i]); // tolower needs unsigned char

	printf(CL_GREEN "Loaded: " CL_RESET "File '" CL_WHITE "%s" CL_RESET "' read.\n", cfgName);

	return 0;
}

/*==========================================
 * Free Message Data
 *------------------------------------------
 */
void do_final_msg_config() {
	int msg_number;

	for (msg_number = 0; msg_number < MSG_NUMBER; msg_number++) {
		FREE(msg_table[msg_number]);
	}

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void do_final_atcommand(void) {
	do_final_msg_config();
	atcommand_synonym_free();

	return;
}

/*==========================================
// @ command processing functions
 *------------------------------------------
 */

/*==========================================
 *
 *------------------------------------------
 */
static int atkillmonster_sub(struct block_list *bl, va_list ap) {
	int flag = va_arg(ap, int);

	nullpo_retr(0, bl);

	if (flag)
		mob_damage(NULL, (struct mob_data *)bl, ((struct mob_data *)bl)->hp, 2);
	else
		mob_delete((struct mob_data *)bl);

	return 0;
}

/*==========================================
 * to send usage of a GM command to a player
 *------------------------------------------
 */
void send_usage(struct map_session_data *sd, char *fmt, ...) {
	va_list ap;
	char mes[2048];

	va_start(ap, fmt);

	vsprintf(mes, fmt, ap);

	if (battle_config.atcommand_send_usage_type == -5) { // -5: like a GM message (in blue)
		clif_GMmessage(&sd->bl, mes, strlen(mes) + 1, 3 | 0x10); // 3 -> SELF + 0x10 for blue
	} else if (battle_config.atcommand_send_usage_type == -4) { // -4: like a GM message (in yellow)
		clif_GMmessage(&sd->bl, mes, strlen(mes) + 1, 3); // 3 -> SELF
	} else if (battle_config.atcommand_send_usage_type == -3) { // -3: like a guild message
		clif_disp_onlyself(sd, mes);
	} else if (battle_config.atcommand_send_usage_type == -2) { // -2: like a party message
		clif_party_message_self(sd, mes, strlen(mes) + 1);
	//} else if (battle_config.atcommand_send_usage_type == -1) { // -1: like a chat message (default)
	} else if (battle_config.atcommand_send_usage_type >= 0 && battle_config.atcommand_send_usage_type <= 0xFFFFFF) { // 0 to 16777215 (0xFFFFFF): like a colored GM message (set the color of the GM message; each basic color from 0 to 255 -> (65536 * Red + 256 * Green + Blue))
		clif_announce(&sd->bl, mes, battle_config.atcommand_send_usage_type, 3); // flag = 3 = SELF
	} else { // -1: like a chat message (default)
		clif_displaymessage(sd->fd, mes);
	}

	va_end(ap);

	return;
}

/*==========================================
 * @rura+
 *------------------------------------------
 */
ATCOMMAND_FUNC(rurap) {
	int x = 0, y = 0;
	struct map_session_data *pl_sd;
	int m;

	if (!message || !*message || (sscanf(message, "%s %d %d %[^\n]", atcmd_mapname, &x, &y, atcmd_name) < 4 &&
	                              sscanf(message, "%[^, ],%d,%d %[^\n]", atcmd_mapname, &x, &y, atcmd_name) < 4)) {
		send_usage(sd, "Usage: %s <mapname> <x> <y> <char name|account_id>", original_command);
		return -1;
	}

	if (strstr(atcmd_mapname, ".gat") == NULL && strlen(atcmd_mapname) < 13) // 16 - 4 (.gat)
		strcat(atcmd_mapname, ".gat");

	if ((pl_sd = map_nick2sd(atcmd_name)) != NULL || ((pl_sd = map_id2sd(atoi(atcmd_name))) != NULL && pl_sd->state.auth)) {
		if (sd->GM_level >= pl_sd->GM_level) { // Only lower or same GM level
			if ((m = map_checkmapname(atcmd_mapname)) == -1) { // If map doesn't exist in all map-servers
				clif_displaymessage(fd, msg_txt(1)); // Map not found.
				return -1;
			}
			if (m >= 0) { // if on this map-server
				if (map[m].flag.nowarpto && battle_config.any_warp_GM_min_level > sd->GM_level) {
					clif_displaymessage(fd, "You are not authorized to warp someone to this map.");
					return -1;
				}
				if (x < 1)
					x = rand() % (map[m].xs - 2) + 1;
				else if (x >= map[m].xs)
					x = map[m].xs - 1;
				if (y < 1)
					y = rand() % (map[m].ys - 2) + 1;
				else if (y >= map[m].ys)
					y = map[m].ys - 1;
			}
			if (pl_sd->bl.m >= 0 && map[pl_sd->bl.m].flag.nowarp && battle_config.any_warp_GM_min_level > sd->GM_level) {
				clif_displaymessage(fd, "You are not authorized to warp this player from its actual map.");
				return -1;
			}
			if (pc_setpos(pl_sd, atcmd_mapname, x, y, 3, 1) == 0) {
				clif_displaymessage(pl_sd->fd, msg_txt(0)); // Warped.
				if (pl_sd != sd)
					clif_displaymessage(fd, msg_txt(15)); // Player warped (message sends to player too).
			} else {
				clif_displaymessage(fd, msg_txt(1)); // Map not found.
				return -1;
			}
		} else {
			clif_displaymessage(fd, msg_txt(81)); // Your GM level don't authorize you to do this action on this player.
			return -1;
		}
	} else {
		clif_displaymessage(fd, msg_txt(3)); // Character not found.
		return -1;
	}

	return 0;
}

/*==========================================
 * @rura
 *------------------------------------------
 */
ATCOMMAND_FUNC(rura) {
	int x = 0, y = 0;
	int m;

	if (!message || !*message || (sscanf(message, "%[^, ],%d,%d", atcmd_mapname, &x, &y) < 3 &&
	                              sscanf(message, "%s %d %d", atcmd_mapname, &x, &y) < 1)) {
		send_usage(sd, "Please, enter a map (usage: %s <mapname> <x> <y>).", original_command);
		return -1;
	}

	if (strstr(atcmd_mapname, ".gat") == NULL && strlen(atcmd_mapname) < 13) // 16 - 4 (.gat)
		strcat(atcmd_mapname, ".gat");

	if ((m = map_checkmapname(atcmd_mapname)) == -1) { // If map doesn't exist in all map-servers
		clif_displaymessage(fd, msg_txt(1)); // Map not found.
		return -1;
	}

	if (m >= 0) { // if on this map-server
		if (x < 1)
			x = rand() % (map[m].xs - 2) + 1;
		else if (x >= map[m].xs)
			x = map[m].xs - 1;
		if (y < 1)
			y = rand() % (map[m].ys - 2) + 1;
		else if (y >= map[m].ys)
			y = map[m].ys - 1;

		if (map[m].flag.nowarpto && battle_config.any_warp_GM_min_level > sd->GM_level) {
			clif_displaymessage(fd, "You are not authorized to warp you to this map.");
			return -1;
		}
	}

	if (sd->bl.m >= 0 && map[sd->bl.m].flag.nowarp && battle_config.any_warp_GM_min_level > sd->GM_level) {
		clif_displaymessage(fd, "You are not authorized to warp you from your actual map.");
		return -1;
	}

	if (pc_setpos(sd, atcmd_mapname, x, y, 3, 1) == 0)
		clif_displaymessage(fd, msg_txt(0)); // Warped.
	else {
		clif_displaymessage(fd, msg_txt(1)); // Map not found.
		return -1;
	}

	return 0;
}

/*==========================================
 * @where - /where
 *------------------------------------------
 */
ATCOMMAND_FUNC(where) {
	struct map_session_data *pl_sd;

	if (!message || !*message || sscanf(message, "%[^\n]", atcmd_name) < 1) {
		memset(atcmd_name, 0, sizeof(atcmd_name));
		strncpy(atcmd_name, sd->status.name, 24);
	}

	if (((pl_sd = map_nick2sd(atcmd_name)) != NULL || ((pl_sd = map_id2sd(atoi(atcmd_name))) != NULL && pl_sd->state.auth)) &&
	    !((pl_sd->GM_level >= battle_config.hide_GM_session || (pl_sd->status.option & OPTION_HIDE)) && (pl_sd->GM_level > sd->GM_level))) { // Only lower or same level
		sprintf(atcmd_output, "%s: %s (%d,%d)", pl_sd->status.name, pl_sd->mapname, pl_sd->bl.x, pl_sd->bl.y);
		clif_displaymessage(fd, atcmd_output);
	} else {
		clif_displaymessage(fd, msg_txt(3)); // Character not found.
		return -1;
	}

	return 0;
}

/*==========================================
 * @jumpto - Jumps to a target player
 *------------------------------------------
 */
ATCOMMAND_FUNC(jumpto) {
	struct map_session_data *pl_sd;

	if (!message || !*message || sscanf(message, "%[^\n]", atcmd_name) < 1) {
		send_usage(sd, "Please, enter a player name (usage: %s <char name|account_id>).", original_command);
		return -1;
	}

	if ((pl_sd = map_nick2sd(atcmd_name)) != NULL || ((pl_sd = map_id2sd(atoi(atcmd_name))) != NULL && pl_sd->state.auth)) {
		if (!((pl_sd->GM_level >= battle_config.hide_GM_session || (pl_sd->status.option & OPTION_HIDE)) && (pl_sd->GM_level > sd->GM_level))) { // Only lower or same level
			if (pl_sd->bl.m >= 0 && map[pl_sd->bl.m].flag.nowarpto && battle_config.any_warp_GM_min_level > sd->GM_level) {
				clif_displaymessage(fd, "You are not authorized to warp you to the map of this player.");
				return -1;
			}
			if (sd->bl.m >= 0 && map[sd->bl.m].flag.nowarp && battle_config.any_warp_GM_min_level > sd->GM_level) {
				clif_displaymessage(fd, "You are not authorized to warp you from your actual map.");
				return -1;
			}
			if (pc_setpos(sd, pl_sd->mapname, pl_sd->bl.x, pl_sd->bl.y, 3, 1) == 0) { // Not found?
				sprintf(atcmd_output, msg_txt(4), pl_sd->status.name); // Jump to '%s'.
				clif_displaymessage(fd, atcmd_output);
			} else {
				clif_displaymessage(fd, msg_txt(1)); // Map not found.
				return -1;
			}
		} else {
			clif_displaymessage(fd, msg_txt(3)); // Character not found.
			return -1;
		}
	} else {
		clif_displaymessage(fd, msg_txt(3)); // Character not found.
		return -1;
	}

	return 0;
}

/*==========================================
 * @jump - Teleport
 *------------------------------------------
 */
ATCOMMAND_FUNC(jump) {
	int x = 0, y = 0;

	sscanf(message, "%d %d", &x, &y);

	if (x < 1)
		x = rand() % (map[sd->bl.m].xs - 2) + 1;
	else if (x >= map[sd->bl.m].xs)
		x = map[sd->bl.m].xs - 1;
	if (y < 1)
		y = rand() % (map[sd->bl.m].ys - 2) + 1;
	else if (y >= map[sd->bl.m].ys)
		y = map[sd->bl.m].ys - 1;

	if (sd->bl.m >= 0 && map[sd->bl.m].flag.nowarpto && battle_config.any_warp_GM_min_level > sd->GM_level) {
		clif_displaymessage(fd, "You are not authorized to warp you to your actual map.");
		return -1;
	}
	if (sd->bl.m >= 0 && map[sd->bl.m].flag.nowarp && battle_config.any_warp_GM_min_level > sd->GM_level) {
		clif_displaymessage(fd, "You are not authorized to warp you from your actual map.");
		return -1;
	}

	pc_setpos(sd, sd->mapname, x, y, 3, 1);
	sprintf(atcmd_output, msg_txt(5), x, y); // Jump to %d %d
	clif_displaymessage(fd, atcmd_output);

	return 0;
}

/*==========================================
 * @users - simplify
 *------------------------------------------
 */
ATCOMMAND_FUNC(users) {
	char buf[50];
	int i, m, users_all;
	struct map_session_data *pl_sd;
	int *map_users;

	// Addition of all players
	users_all = 0;
	CALLOC(map_users, int, map_num);
/* --> Incorrect calculation: A player can know when a hidden GM is present
	for(m = 0; m < map_num; m++)
		if (map[m].users > 0)
			users_all = users_all + map[m].users;
<-- */
	for (i = 0; i < fd_max; i++)
		if (session[i] && (pl_sd = session[i]->session_data) && pl_sd->state.auth)
			if (!((pl_sd->GM_level >= battle_config.hide_GM_session || (pl_sd->status.option & OPTION_HIDE)) && (pl_sd->GM_level > sd->GM_level))) // Only lower or same level
				if ((m = pl_sd->bl.m) >= 0 && m < map_num) {
					map_users[m]++;
					users_all++;
				}

	// Display number of players in all map
	for(m = 0; m < map_num; m++)
		if (map_users[m] > 0) {
			if (m == sd->bl.m)
				sprintf(buf, "%s : %d (%02.02f%%) - your map", map[m].name, map_users[m], ((float)map_users[m] * 100. / (float)users_all)); // 16 + 3 + 4 (1000) + 2 + 6 (100.00) + 13 + 1 (NULL) = 45
			else
				sprintf(buf, "%s : %d (%02.02f%%)", map[m].name, map_users[m], ((float)map_users[m] * 100. / (float)users_all)); // 16 + 3 + 4 (1000) + 2 + 6 (100.00) + 2 + 1 (NULL) = 34
			clif_displaymessage(fd, buf);
		}

	FREE(map_users);

	// Display total number of players
	sprintf(buf, "all : %d", users_all);
	clif_displaymessage(fd, buf);

	return 0;
}

/*==========================================
 * @who - Shows all users on a server
 *------------------------------------------
 */
ATCOMMAND_FUNC(who) {
	struct map_session_data *pl_sd;
	int i, j, count;
	char match_text[100];

	memset(match_text, 0, sizeof(match_text));

	if (sscanf(message, "%[^\n]", match_text) < 1)
		strcpy(match_text, "");
	for (j = 0; match_text[j]; j++)
		match_text[j] = tolower((unsigned char)match_text[j]); // tolower needs unsigned char

	count = 0;
	for (i = 0; i < fd_max; i++) {
		if (session[i] && (pl_sd = session[i]->session_data) && pl_sd->state.auth) {
			if (!((pl_sd->GM_level >= battle_config.hide_GM_session || (pl_sd->status.option & OPTION_HIDE)) && (pl_sd->GM_level > sd->GM_level))) { // Only lower or same level
				memset(atcmd_name, 0, sizeof(atcmd_name));
				for (j = 0; pl_sd->status.name[j]; j++)
					atcmd_name[j] = tolower((unsigned char)pl_sd->status.name[j]); // tolower needs unsigned char
				if (strstr(atcmd_name, match_text) != NULL) { // Search with no case sensitive
					if (pl_sd->GM_level > 0)
						sprintf(atcmd_output, "Name: %s (GM:%d) | Location: %s %d %d", pl_sd->status.name, pl_sd->GM_level, pl_sd->mapname, pl_sd->bl.x, pl_sd->bl.y);
					else
						sprintf(atcmd_output, "Name: %s | Location: %s %d %d", pl_sd->status.name, pl_sd->mapname, pl_sd->bl.x, pl_sd->bl.y);
					clif_displaymessage(fd, atcmd_output);
					count++;
				}
			}
		}
	}

	if (count == 0)
		clif_displaymessage(fd, msg_txt(28)); // No player found.
	else if (count == 1)
		clif_displaymessage(fd, msg_txt(29)); // 1 player found.
	else {
		sprintf(atcmd_output, msg_txt(30), count); // %d players found.
		clif_displaymessage(fd, atcmd_output);
	}

	return 0;
}

/*==========================================
 * @who2 - Shows all users on a server with more detailed info
 *------------------------------------------
 */
ATCOMMAND_FUNC(who2) {
	struct map_session_data *pl_sd;
	int i, j, count;
	char match_text[100];

	memset(match_text, 0, sizeof(match_text));

	if (sscanf(message, "%[^\n]", match_text) < 1)
		strcpy(match_text, "");
	for (j = 0; match_text[j]; j++)
		match_text[j] = tolower((unsigned char)match_text[j]); // tolower needs unsigned char

	count = 0;
	for (i = 0; i < fd_max; i++) {
		if (session[i] && (pl_sd = session[i]->session_data) && pl_sd->state.auth) {
			if (!((pl_sd->GM_level >= battle_config.hide_GM_session || (pl_sd->status.option & OPTION_HIDE)) && (pl_sd->GM_level > sd->GM_level))) { // Only lower or same level
				memset(atcmd_name, 0, sizeof(atcmd_name));
				for (j = 0; pl_sd->status.name[j]; j++)
					atcmd_name[j] = tolower((unsigned char)pl_sd->status.name[j]); // tolower needs unsigned char
				if (strstr(atcmd_name, match_text) != NULL) { // Search with no case sensitive
					if (pl_sd->GM_level > 0)
						sprintf(atcmd_output, "Name: %s (GM:%d) | BLvl: %d | Job: %s (Lvl: %d)", pl_sd->status.name, pl_sd->GM_level, pl_sd->status.base_level, job_name(pl_sd->status.class), pl_sd->status.job_level);
					else
						sprintf(atcmd_output, "Name: %s | BLvl: %d | Job: %s (Lvl: %d)", pl_sd->status.name, pl_sd->status.base_level, job_name(pl_sd->status.class), pl_sd->status.job_level);
					clif_displaymessage(fd, atcmd_output);
					count++;
				}
			}
		}
	}

	if (count == 0)
		clif_displaymessage(fd, msg_txt(28)); // No player found.
	else if (count == 1)
		clif_displaymessage(fd, msg_txt(29)); // 1 player found.
	else {
		sprintf(atcmd_output, msg_txt(30), count); // %d players found.
		clif_displaymessage(fd, atcmd_output);
	}

	return 0;
}

/*==========================================
 * @who2 - Shows all users on a server with extra info
 *------------------------------------------
 */
ATCOMMAND_FUNC(who3) {
	struct map_session_data *pl_sd;
	int i, j, count;
	char match_text[100];
	struct guild *g;
	struct party *p;

	memset(match_text, 0, sizeof(match_text));

	if (sscanf(message, "%[^\n]", match_text) < 1)
		strcpy(match_text, "");
	for (j = 0; match_text[j]; j++)
		match_text[j] = tolower((unsigned char)match_text[j]); // tolower needs unsigned char

	count = 0;
	for (i = 0; i < fd_max; i++) {
		if (session[i] && (pl_sd = session[i]->session_data) && pl_sd->state.auth) {
			if (!((pl_sd->GM_level >= battle_config.hide_GM_session || (pl_sd->status.option & OPTION_HIDE)) && (pl_sd->GM_level > sd->GM_level))) { // Only lower or same level
				memset(atcmd_name, 0, sizeof(atcmd_name));
				for (j = 0; pl_sd->status.name[j]; j++)
					atcmd_name[j] = tolower((unsigned char)pl_sd->status.name[j]); // tolower needs unsigned char
				if (strstr(atcmd_name, match_text) != NULL) { // Search with no case sensitive
					p = party_search(pl_sd->status.party_id);
					g = guild_search(pl_sd->status.guild_id);
					if (pl_sd->GM_level > 0)
						sprintf(atcmd_output, "Name: %s (GM:%d) | Party: '%s' | Guild: '%s'", pl_sd->status.name, pl_sd->GM_level, (p == NULL) ? "None" : p->name, (g == NULL) ? "None" : g->name);
					else
						sprintf(atcmd_output, "Name: %s | Party: '%s' | Guild: '%s'", pl_sd->status.name, (p == NULL) ? "None" : p->name, (g == NULL) ? "None" : g->name);
					clif_displaymessage(fd, atcmd_output);
					count++;
				}
			}
		}
	}

	if (count == 0)
		clif_displaymessage(fd, msg_txt(28)); // No player found.
	else if (count == 1)
		clif_displaymessage(fd, msg_txt(29)); // 1 player found.
	else {
		sprintf(atcmd_output, msg_txt(30), count); // %d players found.
		clif_displaymessage(fd, atcmd_output);
	}

	return 0;
}

/*==========================================
 * @whomap - Same as @who only for a map
 *------------------------------------------
 */
ATCOMMAND_FUNC(whomap) {
	struct map_session_data *pl_sd;
	int i, count;
	int map_id;

	if (!message || !*message || sscanf(message, "%s", atcmd_mapname) < 1)
		map_id = sd->bl.m;
	else {
		if (strstr(atcmd_mapname, ".gat") == NULL && strlen(atcmd_mapname) < 13) // 16 - 4 (.gat)
			strcat(atcmd_mapname, ".gat");
		if ((map_id = map_mapname2mapid(atcmd_mapname)) < 0) // Only from actual map-server
			map_id = sd->bl.m;
	}

	count = 0;
	for (i = 0; i < fd_max; i++) {
		if (session[i] && (pl_sd = session[i]->session_data) && pl_sd->state.auth) {
			if (pl_sd->bl.m == map_id) {
				if (!((pl_sd->GM_level >= battle_config.hide_GM_session || (pl_sd->status.option & OPTION_HIDE)) && (pl_sd->GM_level > sd->GM_level))) { // Only lower or same level
					if (pl_sd->GM_level > 0)
						sprintf(atcmd_output, "Name: %s (GM:%d) | Location: %s %d %d", pl_sd->status.name, pl_sd->GM_level, pl_sd->mapname, pl_sd->bl.x, pl_sd->bl.y);
					else
						sprintf(atcmd_output, "Name: %s | Location: %s %d %d", pl_sd->status.name, pl_sd->mapname, pl_sd->bl.x, pl_sd->bl.y);
					clif_displaymessage(fd, atcmd_output);
					count++;
				}
			}
		}
	}

	if (count == 0)
		sprintf(atcmd_output, msg_txt(54), map[map_id].name); // No player found in map '%s'.
	else if (count == 1)
		sprintf(atcmd_output, msg_txt(55), map[map_id].name); // 1 player found in map '%s'.
	else
		sprintf(atcmd_output, msg_txt(56), count, map[map_id].name); // %d players found in map '%s'.
	clif_displaymessage(fd, atcmd_output);

	return 0;
}

/*==========================================
 * @whomap2 - Same as @who2 only for a map
 *------------------------------------------
 */
ATCOMMAND_FUNC(whomap2) {
	struct map_session_data *pl_sd;
	int i, count;
	int map_id = 0;

	if (!message || !*message || sscanf(message, "%s", atcmd_mapname) < 1)
		map_id = sd->bl.m;
	else {
		if (strstr(atcmd_mapname, ".gat") == NULL && strlen(atcmd_mapname) < 13) // 16 - 4 (.gat)
			strcat(atcmd_mapname, ".gat");
		if ((map_id = map_mapname2mapid(atcmd_mapname)) < 0) // Only from actual map-server
			map_id = sd->bl.m;
	}

	count = 0;
	for (i = 0; i < fd_max; i++) {
		if (session[i] && (pl_sd = session[i]->session_data) && pl_sd->state.auth) {
			if (pl_sd->bl.m == map_id) {
				if (!((pl_sd->GM_level >= battle_config.hide_GM_session || (pl_sd->status.option & OPTION_HIDE)) && (pl_sd->GM_level > sd->GM_level))) { // Only lower or same level
					if (pl_sd->GM_level > 0)
						sprintf(atcmd_output, "Name: %s (GM:%d) | BLvl: %d | Job: %s (Lvl: %d)", pl_sd->status.name, pl_sd->GM_level, pl_sd->status.base_level, job_name(pl_sd->status.class), pl_sd->status.job_level);
					else
						sprintf(atcmd_output, "Name: %s | BLvl: %d | Job: %s (Lvl: %d)", pl_sd->status.name, pl_sd->status.base_level, job_name(pl_sd->status.class), pl_sd->status.job_level);
					clif_displaymessage(fd, atcmd_output);
					count++;
				}
			}
		}
	}

	if (count == 0)
		sprintf(atcmd_output, msg_txt(54), map[map_id].name); // No player found in map '%s'.
	else if (count == 1)
		sprintf(atcmd_output, msg_txt(55), map[map_id].name); // 1 player found in map '%s'.
	else
		sprintf(atcmd_output, msg_txt(56), count, map[map_id].name); // %d players found in map '%s'.
	clif_displaymessage(fd, atcmd_output);

	return 0;
}

/*==========================================
 * @whomap3 - Same as @who3 only for a map
 *------------------------------------------
 */
ATCOMMAND_FUNC(whomap3) {
	struct map_session_data *pl_sd;
	int i, count;
	int map_id = 0;
	struct guild *g;
	struct party *p;

	if (!message || !*message || sscanf(message, "%s", atcmd_mapname) < 1)
		map_id = sd->bl.m;
	else {
		if (strstr(atcmd_mapname, ".gat") == NULL && strlen(atcmd_mapname) < 13) // 16 - 4 (.gat)
			strcat(atcmd_mapname, ".gat");
		if ((map_id = map_mapname2mapid(atcmd_mapname)) < 0) // Only from actual map-server
			map_id = sd->bl.m;
	}

	count = 0;
	for (i = 0; i < fd_max; i++) {
		if (session[i] && (pl_sd = session[i]->session_data) && pl_sd->state.auth) {
			if (pl_sd->bl.m == map_id) {
				if (!((pl_sd->GM_level >= battle_config.hide_GM_session || (pl_sd->status.option & OPTION_HIDE)) && (pl_sd->GM_level > sd->GM_level))) { // Only lower or same level
					p = party_search(pl_sd->status.party_id);
					g = guild_search(pl_sd->status.guild_id);
					if (pl_sd->GM_level > 0)
						sprintf(atcmd_output, "Name: %s (GM:%d) | Party: '%s' | Guild: '%s'", pl_sd->status.name, pl_sd->GM_level, (p == NULL) ? "None" : p->name, (g == NULL) ? "None" : g->name);
					else
						sprintf(atcmd_output, "Name: %s | Party: '%s' | Guild: '%s'", pl_sd->status.name, (p == NULL) ? "None" : p->name, (g == NULL) ? "None" : g->name);
					clif_displaymessage(fd, atcmd_output);
					count++;
				}
			}
		}
	}

	if (count == 0)
		sprintf(atcmd_output, msg_txt(54), map[map_id].name); // No player found in map '%s'.
	else if (count == 1)
		sprintf(atcmd_output, msg_txt(55), map[map_id].name); // 1 player found in map '%s'.
	else
		sprintf(atcmd_output, msg_txt(56), count, map[map_id].name); // %d players found in map '%s'.
	clif_displaymessage(fd, atcmd_output);

	return 0;
}

/*==========================================
 * @whogm - Shows GMs
 *------------------------------------------
 */
ATCOMMAND_FUNC(whogm) {
	struct map_session_data *pl_sd;
	int i, j, count;
	char match_text[100];
	struct guild *g;
	struct party *p;

	memset(match_text, 0, sizeof(match_text));

	if (sscanf(message, "%[^\n]", match_text) < 1)
		strcpy(match_text, "");
	for (j = 0; match_text[j]; j++)
		match_text[j] = tolower((unsigned char)match_text[j]); // tolower needs unsigned char

	count = 0;
	for (i = 0; i < fd_max; i++) {
		if (session[i] && (pl_sd = session[i]->session_data) && pl_sd->state.auth) {
			if (pl_sd->GM_level > battle_config.atcommand_max_player_gm_level) {
				if (!((pl_sd->GM_level >= battle_config.hide_GM_session || (pl_sd->status.option & OPTION_HIDE)) && (pl_sd->GM_level > sd->GM_level))) { // Only lower or same level
					memset(atcmd_name, 0, sizeof(atcmd_name));
					for (j = 0; pl_sd->status.name[j]; j++)
						atcmd_name[j] = tolower((unsigned char)pl_sd->status.name[j]); // tolower needs unsigned char
					if (strstr(atcmd_name, match_text) != NULL) { // Search with no case sensitive
						sprintf(atcmd_output, "Name: %s (GM:%d) | Location: %s %d %d", pl_sd->status.name, pl_sd->GM_level, pl_sd->mapname, pl_sd->bl.x, pl_sd->bl.y);
						clif_displaymessage(fd, atcmd_output);
						sprintf(atcmd_output, "       BLvl: %d | Job: %s (Lvl: %d)", pl_sd->status.base_level, job_name(pl_sd->status.class), pl_sd->status.job_level);
						clif_displaymessage(fd, atcmd_output);
						p = party_search(pl_sd->status.party_id);
						g = guild_search(pl_sd->status.guild_id);
						sprintf(atcmd_output, "       Party: '%s' | Guild: '%s'", (p == NULL) ? "None" : p->name, (g == NULL) ? "None" : g->name);
						clif_displaymessage(fd, atcmd_output);
						count++;
					}
				}
			}
		}
	}

	if (count == 0)
		clif_displaymessage(fd, msg_txt(150)); // No GM found.
	else if (count == 1)
		clif_displaymessage(fd, msg_txt(151)); // 1 GM found.
	else {
		sprintf(atcmd_output, msg_txt(152), count); // %d GMs found.
		clif_displaymessage(fd, atcmd_output);
	}

	return 0;
}

/*==========================================
 * @whohas
 *------------------------------------------
 */
ATCOMMAND_FUNC(whohas) {
	char item_name[100];
	char position[100];
	int i, j, k, item_id, players;
	struct map_session_data *pl_sd;
	struct item_data *item_data;
	struct storage *stor;

	memset(item_name, 0, sizeof(item_name));

	if (!message || !*message || sscanf(message, "%s", item_name) < 1) {
		send_usage(sd, "Please, enter an item name/id (usage: %s <item_name|ID>).", original_command);
		return -1;
	}

	item_id = 0;
	if ((item_data = itemdb_searchname(item_name)) != NULL ||
	    (item_data = itemdb_exists(atoi(item_name))) != NULL)
		item_id = item_data->nameid;

	if (item_id >= 501) {
		// Get number of online players, id of each online players
		players = 0;
		// Sort online characters.
		for(i = 0; i < fd_max; i++) {
			if (session[i] && (pl_sd = session[i]->session_data) && pl_sd->state.auth) {
				if (!((pl_sd->GM_level >= battle_config.hide_GM_session || (pl_sd->status.option & OPTION_HIDE)) && (pl_sd->GM_level > sd->GM_level))) { // Only lower or same level
					memset(position, 0, sizeof(position));
					// Search in inventory
					for (j = 0; j < MAX_INVENTORY; j++) {
						if (pl_sd->status.inventory[j].nameid == item_id)
							break;
						// Search for cards
						if (pl_sd->status.inventory[j].nameid > 0 && (item_data = itemdb_search(pl_sd->status.inventory[j].nameid)) != NULL) {
							for (k = 0; k < item_data->slot; k++)
								if (pl_sd->status.inventory[j].card[k] == item_id)
									break;
							if (k != item_data->slot)
								break;
						}
					}
					if (j != MAX_INVENTORY)
						strcpy(position, "inventory, ");
					// Search in cart
					for (j = 0; j < MAX_CART; j++) {
						if (pl_sd->status.cart[j].nameid == item_id)
							break;
						// Search for cards
						if (pl_sd->status.cart[j].nameid > 0 && (item_data = itemdb_search(pl_sd->status.cart[j].nameid)) != NULL) {
							for (k = 0; k < item_data->slot; k++)
								if (pl_sd->status.cart[j].card[k] == item_id)
									break;
							if (k != item_data->slot)
								break;
						}
					}
					if (j != MAX_CART)
						strcat(position, "cart, ");
					// Search in storage
					if ((stor = account2storage2(pl_sd->status.account_id)) != NULL) {
						for (j = 0; j < MAX_STORAGE; j++) {
							if (stor->storage[j].nameid == item_id)
								break;
							// Search for cards
							if (stor->storage[j].nameid > 0 && (item_data = itemdb_search(stor->storage[j].nameid)) != NULL) {
								for (k = 0; k < item_data->slot; k++)
									if (stor->storage[j].card[k] == item_id)
										break;
								if (k != item_data->slot)
									break;
							}
						}
					}
					if (j != MAX_STORAGE)
						strcat(position, "storage, ");
					// If found in one of inventory system of the player
					if (position[0]) {
						position[strlen(position)-2] = '\0'; // Remove last ', '
						if (pl_sd->GM_level > 0)
							sprintf(atcmd_output, "Name: %s (GM:%d) | %s (%d/%d) | found in %s", pl_sd->status.name, pl_sd->GM_level, job_name(pl_sd->status.class), pl_sd->status.base_level, pl_sd->status.job_level, position);
						else
							sprintf(atcmd_output, "Name: %s | %s (%d/%d) | found in %s", pl_sd->status.name, job_name(pl_sd->status.class), pl_sd->status.base_level, pl_sd->status.job_level, position);
						clif_displaymessage(fd, atcmd_output);
						players++;
					}
				}
			}
		}
	} else {
		clif_displaymessage(fd, msg_txt(19)); // Invalid item ID or name.
		return -1;
	}

	if (players == 0)
		clif_displaymessage(fd, "No player has this item.");
	else if (players == 1)
		clif_displaymessage(fd, "1 player has this item.");
	else {
		sprintf(atcmd_output, "%d players have this item.", players);
		clif_displaymessage(fd, atcmd_output);
	}

	return 0;
}

/*==========================================
 * @whohasmap
 *------------------------------------------
 */
ATCOMMAND_FUNC(whohasmap) {
	char item_name[100];
	char position[100];
	int i, j, k, map_id, item_id, players;
	struct map_session_data *pl_sd;
	struct item_data *item_data;
	struct storage *stor;

	memset(item_name, 0, sizeof(item_name));
	memset(atcmd_mapname, 0, sizeof(atcmd_mapname));

	if (!message || !*message || sscanf(message, "%s %s", item_name, atcmd_mapname) < 1) {
		send_usage(sd, "Please, enter an item name/id (usage: %s <item_name|ID> [map]).", original_command);
		return -1;
	}

	if (!atcmd_mapname[0])
		map_id = sd->bl.m;
	else {
		if (strstr(atcmd_mapname, ".gat") == NULL && strlen(atcmd_mapname) < 13) // 16 - 4 (.gat)
			strcat(atcmd_mapname, ".gat");
		if ((map_id = map_mapname2mapid(atcmd_mapname)) < 0) // Only from actual map-server
			map_id = sd->bl.m;
	}

	item_id = 0;
	if ((item_data = itemdb_searchname(item_name)) != NULL ||
	    (item_data = itemdb_exists(atoi(item_name))) != NULL)
		item_id = item_data->nameid;

	if (item_id >= 501) {
		// Get number of online players, id of each online players
		players = 0;
		// Sort online characters
		for(i = 0; i < fd_max; i++) {
			if (session[i] && (pl_sd = session[i]->session_data) && pl_sd->state.auth) {
				if (!((pl_sd->GM_level >= battle_config.hide_GM_session || (pl_sd->status.option & OPTION_HIDE)) && (pl_sd->GM_level > sd->GM_level))) { // Only lower or same level
					if (pl_sd->bl.m == map_id) {
						memset(position, 0, sizeof(position));
						// Search in inventory
						for (j = 0; j < MAX_INVENTORY; j++) {
							if (pl_sd->status.inventory[j].nameid == item_id)
								break;
							// Search for cards
							if (pl_sd->status.inventory[j].nameid > 0 && (item_data = itemdb_search(pl_sd->status.inventory[j].nameid)) != NULL) {
								for (k = 0; k < item_data->slot; k++)
									if (pl_sd->status.inventory[j].card[k] == item_id)
										break;
								if (k != item_data->slot)
									break;
							}
						}
						if (j != MAX_INVENTORY)
							strcpy(position, "inventory, ");
						// Search in cart
						for (j = 0; j < MAX_CART; j++) {
							if (pl_sd->status.cart[j].nameid == item_id)
								break;
							// Search for cards
							if (pl_sd->status.cart[j].nameid > 0 && (item_data = itemdb_search(pl_sd->status.cart[j].nameid)) != NULL) {
								for (k = 0; k < item_data->slot; k++)
									if (pl_sd->status.cart[j].card[k] == item_id)
										break;
								if (k != item_data->slot)
									break;
							}
						}
						if (j != MAX_CART)
							strcat(position, "cart, ");
						// Search in storage
						if ((stor = account2storage2(pl_sd->status.account_id)) != NULL) {
							for (j = 0; j < MAX_STORAGE; j++) {
								if (stor->storage[j].nameid == item_id)
									break;
								// Search for cards
								if (stor->storage[j].nameid > 0 && (item_data = itemdb_search(stor->storage[j].nameid)) != NULL) {
									for (k = 0; k < item_data->slot; k++)
										if (stor->storage[j].card[k] == item_id)
											break;
									if (k != item_data->slot)
										break;
								}
							}
						}
						if (j != MAX_STORAGE)
							strcat(position, "storage, ");
						// If found in one of inventory system of the player
						if (position[0]) {
							position[strlen(position)-2] = '\0'; // Remove last ', '
							if (pl_sd->GM_level > 0)
								sprintf(atcmd_output, "Name: %s (GM:%d) | %s (%d/%d) | found in %s", pl_sd->status.name, pl_sd->GM_level, job_name(pl_sd->status.class), pl_sd->status.base_level, pl_sd->status.job_level, position);
							else
								sprintf(atcmd_output, "Name: %s | %s (%d/%d) | found in %s", pl_sd->status.name, job_name(pl_sd->status.class), pl_sd->status.base_level, pl_sd->status.job_level, position);
							clif_displaymessage(fd, atcmd_output);
							players++;
						}
					}
				}
			}
		}
	} else {
		clif_displaymessage(fd, msg_txt(19)); // Invalid item ID or name.
		return -1;
	}

	if (players == 0)
		sprintf(atcmd_output, "No player has this item on map '%s'.", map[map_id].name);
	else if (players == 1)
		sprintf(atcmd_output, "1 player has this item on map '%s'.", map[map_id].name);
	else
		sprintf(atcmd_output, "%d players have this item on map '%s'.", players, map[map_id].name);
	clif_displaymessage(fd, atcmd_output);

	return 0;
}

/*==========================================
 * @save - Makes current position the user's save point
 *------------------------------------------
 */
ATCOMMAND_FUNC(save) {
	pc_setsavepoint(sd, sd->mapname, sd->bl.x, sd->bl.y);
	if (sd->status.pet_id > 0 && sd->pd)
		intif_save_petdata(sd->status.account_id, &sd->pet);
	chrif_save(sd); // Do pc_makesavestatus and save storage + account_reg/account_reg2 too
	clif_displaymessage(fd, msg_txt(6)); // Character data respawn point saved.

	return 0;
}

/*==========================================
 * @load - Sends user to respawn point
 *------------------------------------------
 */
ATCOMMAND_FUNC(load) {
	int m;

	if ((m = map_checkmapname(sd->status.save_point.map)) == -1) { // If map doesn't exist in all map-servers
		clif_displaymessage(fd, "Save map not found.");
		return -1;
	}

	if (m >= 0) { // If on this map-server
		if (map[m].flag.nowarpto && battle_config.any_warp_GM_min_level > sd->GM_level) {
			clif_displaymessage(fd, "You are not authorized to warp you to your save map.");
			return -1;
		}
	}
	if (sd->bl.m >= 0 && map[sd->bl.m].flag.nowarp && battle_config.any_warp_GM_min_level > sd->GM_level) {
		clif_displaymessage(fd, "You are not authorized to warp you from your actual map.");
		return -1;
	}
	pc_setpos(sd, sd->status.save_point.map, sd->status.save_point.x, sd->status.save_point.y, 0, 0);
	clif_displaymessage(fd, msg_txt(7)); // Warping to respawn point.

	return 0;
}

/*==========================================
 * @charload - Sends target to respawn point
 *------------------------------------------
 */
ATCOMMAND_FUNC(charload) {
	struct map_session_data *pl_sd;
	int m;

	if (!message || !*message || sscanf(message, "%[^\n]", atcmd_name) < 1) {
		send_usage(sd, "Please, enter a player name (usage: %s <char name|account_id>).", original_command);
		return -1;
	}

	if ((pl_sd = map_nick2sd(atcmd_name)) != NULL || ((pl_sd = map_id2sd(atoi(atcmd_name))) != NULL && pl_sd->state.auth)) {
		if (sd->GM_level >= pl_sd->GM_level) { // Only lower or same level
			if ((m = map_checkmapname(pl_sd->status.save_point.map)) == -1) { // If map doesn't exist in all map-servers
				clif_displaymessage(fd, "Save map not found.");
				return -1;
			}
			if (m >= 0) { // If on this map-server
				if (map[m].flag.nowarpto && battle_config.any_warp_GM_min_level > sd->GM_level) {
					clif_displaymessage(fd, "You are not authorized to warp this character to its save map.");
					return -1;
				}
			}
			if (pl_sd->bl.m >= 0 && map[pl_sd->bl.m].flag.nowarp && battle_config.any_warp_GM_min_level > sd->GM_level) {
				clif_displaymessage(fd, "You are not authorized to warp this character from its actual map.");
				return -1;
			}
			pc_setpos(pl_sd, pl_sd->status.save_point.map, pl_sd->status.save_point.x, pl_sd->status.save_point.y, 0, 0);
			clif_displaymessage(fd, "Character warped to its respawn point.");
		} else {
			clif_displaymessage(fd, msg_txt(81)); // Your GM level don't authorize you to do this action on this player.
			return -1;
		}
	} else {
		clif_displaymessage(fd, msg_txt(3)); // Character not found.
		return -1;
	}

	return 0;
}

/*==========================================
 * @charloadmap - Sends all chars on a map to their respawn point
 *------------------------------------------
 */
ATCOMMAND_FUNC(charloadmap) {
	struct map_session_data *pl_sd;
	int i, count;
	int map_id;

	if (!message || !*message || sscanf(message, "%s", atcmd_mapname) < 1)
		map_id = sd->bl.m;
	else {
		if (strstr(atcmd_mapname, ".gat") == NULL && strlen(atcmd_mapname) < 13) // 16 - 4 (.gat)
			strcat(atcmd_mapname, ".gat");
		if ((map_id = map_mapname2mapid(atcmd_mapname)) < 0) // Only from actual map-server
			map_id = sd->bl.m;
	}

	count = 0;
	for (i = 0; i < fd_max; i++) {
		if (session[i] && (pl_sd = session[i]->session_data) && pl_sd->state.auth) {
			if (pl_sd->bl.m == map_id) {
				if (sd->GM_level >= pl_sd->GM_level) { // Only lower or same level
					if (pl_sd != sd) { // Prevent from loading yourself
						pc_setpos(pl_sd, pl_sd->status.save_point.map, pl_sd->status.save_point.x, pl_sd->status.save_point.y, 0, 0);
						count++;
					}
				}
			}
		}
	}

	if (count == 0)
		sprintf(atcmd_output, "No player recalled from map '%s'.", map[map_id].name);
	else if (count == 1)
		sprintf(atcmd_output, "1 player recalled from map '%s'.", map[map_id].name);
	else
		sprintf(atcmd_output, "%d players recalled from map '%s'.", count, map[map_id].name);
	clif_displaymessage(fd, atcmd_output);

	return 0;
}

/*==========================================
 * @charloadall - Sends all chars to their respawn point
 *------------------------------------------
 */
ATCOMMAND_FUNC(charloadall) {
	struct map_session_data *pl_sd;
	int i, count;

	count = 0;
	for (i = 0; i < fd_max; i++) {
		if (session[i] && (pl_sd = session[i]->session_data) && pl_sd->state.auth) {
			if (sd->GM_level >= pl_sd->GM_level) { // Only lower or same level
				if (pl_sd != sd) { // Prevent from loading yourself
					pc_setpos(pl_sd, pl_sd->status.save_point.map, pl_sd->status.save_point.x, pl_sd->status.save_point.y, 0, 0);
					count++;
				}
			}
		}
	}

	if (count == 0)
		clif_displaymessage(fd, "No player recalled.");
	else if (count == 1)
		clif_displaymessage(fd, "1 player recalled.");
	else {
		sprintf(atcmd_output, "%d players recalled.", count);
		clif_displaymessage(fd, atcmd_output);
	}

	return 0;
}

/*==========================================
 * @speed - Changes user's speed value
 *------------------------------------------
 */
ATCOMMAND_FUNC(speed) {
	int speed = 0;

	sscanf(message, "%d", &speed);

	if (speed != 0 && (speed < MIN_WALK_SPEED || speed > MAX_WALK_SPEED)) {
		send_usage(sd, "Please, enter a valid speed value (usage:%s 0(restore normal speed)|%s <%d-%d>).", original_command, original_command, MIN_WALK_SPEED, MAX_WALK_SPEED);
		return -1;
	}

	if (speed == 0)
		status_calc_pc(sd, 3);
	else {
		sd->speed = speed;
		clif_updatestatus(sd, SP_SPEED);
	}
	clif_displaymessage(fd, msg_txt(8)); // Speed changed.

	return 0;
}

/*==========================================
 * @charspeed - Changes target's speed value
 *------------------------------------------
 */
ATCOMMAND_FUNC(charspeed) {
	struct map_session_data *pl_sd;
	int speed = 0;

	if (!message || !*message || sscanf(message, "%d %[^\n]", &speed, atcmd_name) < 2) {
		send_usage(sd, "Please, enter a speed value and a player name (usage: %s 0(restore normal speed)|<%d-%d> <player name>).", original_command, MIN_WALK_SPEED, MAX_WALK_SPEED);
		return -1;
	}

	if (speed != 0 && (speed < MIN_WALK_SPEED || speed > MAX_WALK_SPEED)) {
		send_usage(sd, "Please, enter a valid speed value (usage: %s 0(restore normal speed)|<%d-%d> <player name>).", original_command, MIN_WALK_SPEED, MAX_WALK_SPEED);
		return -1;
	}

	if ((pl_sd = map_nick2sd(atcmd_name)) != NULL || ((pl_sd = map_id2sd(atoi(atcmd_name))) != NULL && pl_sd->state.auth)) {
		if (sd->GM_level >= pl_sd->GM_level) { // Only lower or same level
			if (speed == 0)
				status_calc_pc(pl_sd, 3);
			else {
				pl_sd->speed = speed;
				clif_updatestatus(pl_sd, SP_SPEED);
			}
			clif_displaymessage(fd, "Character speed changed.");
		} else {
			clif_displaymessage(fd, msg_txt(81)); // Your GM level don't authorize you to do this action on this player.
			return -1;
		}
	} else {
		clif_displaymessage(fd, msg_txt(3)); // Character not found.
		return -1;
	}

	return 0;
}

/*==========================================
 * @charspeedmap - Changes all users on a map's speed values
 *------------------------------------------
 */
ATCOMMAND_FUNC(charspeedmap) {
	struct map_session_data *pl_sd;
	int i, count;
	int speed = 0;
	int map_id;

	memset(atcmd_mapname, 0, sizeof(atcmd_mapname));

	sscanf(message, "%d %[^\n]", &speed, atcmd_mapname);

	if (speed != 0 && (speed < MIN_WALK_SPEED || speed > MAX_WALK_SPEED)) {
		send_usage(sd, "Please, enter a valid speed value (usage: %s [0(restore normal speed)|<%d-%d>] [map name]).", original_command, MIN_WALK_SPEED, MAX_WALK_SPEED);
		return -1;
	}

	if (!atcmd_mapname[0])
		map_id = sd->bl.m;
	else {
		if (strstr(atcmd_mapname, ".gat") == NULL && strlen(atcmd_mapname) < 13) // 16 - 4 (.gat)
			strcat(atcmd_mapname, ".gat");
		if ((map_id = map_mapname2mapid(atcmd_mapname)) < 0) // Only from actual map-server
			map_id = sd->bl.m;
	}

	count = 0;
	for (i = 0; i < fd_max; i++) {
		if (session[i] && (pl_sd = session[i]->session_data) && pl_sd->state.auth) {
			if (pl_sd->bl.m == map_id) {
				if (sd->GM_level >= pl_sd->GM_level) { // Only lower or same level
					if (speed == 0)
						status_calc_pc(pl_sd, 3);
					else {
						pl_sd->speed = speed;
						clif_updatestatus(pl_sd, SP_SPEED);
					}
					count++;
				}
			}
		}
	}

	if (count == 0)
		sprintf(atcmd_output, "Speed not changed - no player found on map '%s'.", map[map_id].name);
	else if (count == 1)
		sprintf(atcmd_output, "Speed changed at 1 player on map '%s'.", map[map_id].name);
	else
		sprintf(atcmd_output, "Speed changed at %d players on map '%s'.", count, map[map_id].name);
	clif_displaymessage(fd, atcmd_output);

	return 0;
}

/*==========================================
 * @charspeedall - Changes all users on a server's speed values
 *------------------------------------------
 */
ATCOMMAND_FUNC(charspeedall) {
	struct map_session_data *pl_sd;
	int i, count;
	int speed = 0;

	sscanf(message, "%d", &speed);

	if (speed != 0 && (speed < MIN_WALK_SPEED || speed > MAX_WALK_SPEED)) {
		send_usage(sd, "Please, enter a valid speed value (usage: %s [0(restore normal speed)|<%d-%d>]).", original_command, MIN_WALK_SPEED, MAX_WALK_SPEED);
		return -1;
	}

	count = 0;
	for (i = 0; i < fd_max; i++) {
		if (session[i] && (pl_sd = session[i]->session_data) && pl_sd->state.auth) {
			if (sd->GM_level >= pl_sd->GM_level) { // Only lower or same level
				if (speed == 0)
					status_calc_pc(pl_sd, 3);
				else {
					pl_sd->speed = speed;
					clif_updatestatus(pl_sd, SP_SPEED);
				}
				count++;
			}
		}
	}

	if (count == 0)
		clif_displaymessage(fd, "Speed not changed - no player found.");
	else if (count == 1)
		clif_displaymessage(fd, "Speed changed at 1 player.");
	else {
		sprintf(atcmd_output, "Speed changed at %d players.", count);
		clif_displaymessage(fd, atcmd_output);
	}

	return 0;
}

/*==========================================
 * @storage - Opens user's storage
 *------------------------------------------
 */
ATCOMMAND_FUNC(storage) {
	struct storage *stor;

	if (sd->state.storage_flag == 1) {
		clif_displaymessage(fd, "You have opened your guild storage. Close it before.");
		return -1;
	}

	if ((stor = account2storage2(sd->status.account_id)) != NULL && stor->storage_status == 1) {
		clif_displaymessage(fd, "You have already opened your storage.");
		return -1;
	}

	if (!battle_config.atcommand_storage_on_pvp_map && map[sd->bl.m].flag.pvp) {
		clif_displaymessage(fd, "You can not open your storage on a PVP map.");
		return -1;
	}

	storage_storageopen(sd);

	return 0;
}

/*==========================================
 * @charstorage - Opens target's storage
 *------------------------------------------
 */
ATCOMMAND_FUNC(charstorage) {
	struct map_session_data *pl_sd;
	struct storage *stor;

	if (!message || !*message || sscanf(message, "%[^\n]", atcmd_name) < 1) {
		send_usage(sd, "Please, enter a player name (usage: %s <char name|account_id>).", original_command);
		return -1;
	}

	if ((pl_sd = map_nick2sd(atcmd_name)) != NULL || ((pl_sd = map_id2sd(atoi(atcmd_name))) != NULL && pl_sd->state.auth)) {
		if (sd->GM_level >= pl_sd->GM_level) { // Only lower or same level
			if (pl_sd->state.storage_flag == 1) {
				clif_displaymessage(fd, "The player has opened its guild storage. He/She must close it before.");
				return -1;
			}
			if ((stor = account2storage2(pl_sd->status.account_id)) != NULL && stor->storage_status == 1) {
				clif_displaymessage(fd, "The player have already opened his/her storage.");
				return -1;
			}
			if (!battle_config.atcommand_storage_on_pvp_map && map[pl_sd->bl.m].flag.pvp) {
				clif_displaymessage(fd, "This player is on a pvp map. You can not open his/her storage.");
				return -1;
			}
			storage_storageopen(pl_sd);
			if (pl_sd != sd)
				clif_displaymessage(fd, "Player's storage is now opened (for the player).");
		} else {
			clif_displaymessage(fd, msg_txt(81)); // Your GM level don't authorize you to do this action on this player.
			return -1;
		}
	} else {
		clif_displaymessage(fd, msg_txt(3)); // Character not found.
		return -1;
	}

	return 0;
}

/*==========================================
 * @guildstorage - Opens user's guild storage
 *------------------------------------------
 */
ATCOMMAND_FUNC(guildstorage) {
	struct storage *stor;

	if (sd->status.guild_id > 0) {
		if (sd->state.storage_flag == 1) {
			clif_displaymessage(fd, "You have already opened your guild storage.");
			return -1;
		}
		if ((stor = account2storage2(sd->status.account_id)) != NULL && stor->storage_status == 1) {
			clif_displaymessage(fd, "Your storage is opened. Close it before.");
			return -1;
		}
		if (!battle_config.atcommand_gstorage_on_pvp_map && map[sd->bl.m].flag.pvp) {
			clif_displaymessage(fd, "You can not open your guild storage on a PVP map.");
			return -1;
		}
		storage_guild_storageopen(sd);
	} else {
		clif_displaymessage(fd, "You are not in a guild.");
		return -1;
	}

	return 0;
}

/*==========================================
 * @charguildstorage - Opens target's guild storage
 *------------------------------------------
 */
ATCOMMAND_FUNC(charguildstorage) {
	struct map_session_data *pl_sd;
	struct storage *stor;

	if (!message || !*message || sscanf(message, "%[^\n]", atcmd_name) < 1) {
		send_usage(sd, "Please, enter a player name (usage: %s <char name|account_id>).", original_command);
		return -1;
	}

	if ((pl_sd = map_nick2sd(atcmd_name)) != NULL || ((pl_sd = map_id2sd(atoi(atcmd_name))) != NULL && pl_sd->state.auth)) {
		if (sd->GM_level >= pl_sd->GM_level) { // Only lower or same level
			if (pl_sd->status.guild_id > 0) {
				if (pl_sd->state.storage_flag == 1) {
					clif_displaymessage(fd, "The guild storage of the player is already opened.");
					return -1;
				}
				if ((stor = account2storage2(pl_sd->status.account_id)) != NULL && stor->storage_status == 1) {
					clif_displaymessage(fd, "The player's storage is opened. He/She must close it before.");
					return -1;
				}
				if (!battle_config.atcommand_gstorage_on_pvp_map && map[pl_sd->bl.m].flag.pvp) {
					clif_displaymessage(fd, "This player is on a pvp map. You can not open his/her guild storage.");
					return -1;
				}
				storage_guild_storageopen(pl_sd);
				if (pl_sd != sd)
					clif_displaymessage(fd, "Player's guild storage is now opened (for the player).");
			} else {
				clif_displaymessage(fd, "This player is not in a guild.");
				return -1;
			}
		} else {
			clif_displaymessage(fd, msg_txt(81)); // Your GM level don't authorize you to do this action on this player.
			return -1;
		}
	} else {
		clif_displaymessage(fd, msg_txt(3)); // Character not found.
		return -1;
	}

	return 0;
}

/*==========================================
 * @option
 *------------------------------------------
 */
ATCOMMAND_FUNC(option) {
	int opt1 = 0, opt2 = 0, opt3 = 0;

	if (!message || !*message || sscanf(message, "%d %d %d", &opt1, &opt2, &opt3) < 1 || opt1 < 0 || opt2 < 0 || opt3 < 0) {
		send_usage(sd, "Please, enter at least a option (usage: %s <opt1:0+> <opt2:0+> <opt3:0+>).", original_command);
		return -1;
	}

	sd->opt1 = opt1;
	sd->opt2 = opt2;
	if (!(sd->status.option & CART_MASK) && (opt3 & CART_MASK)) {
		clif_cart_itemlist(sd);
		clif_cart_equiplist(sd);
		clif_updatestatus(sd, SP_CARTINFO);
	}
	sd->status.option = opt3;
	// Fix Peco display
	if (sd->status.class ==   13 || sd->status.class ==   21 ||
	    sd->status.class == 4014 || sd->status.class == 4022 ||
	    sd->status.class == 4036 || sd->status.class == 4044) {
		if (!pc_isriding(sd)) { // sd has the new value...
			// Normal classes
			if (sd->status.class == 13)
				sd->status.class = sd->view_class = 7;
			else if (sd->status.class == 21)
				sd->status.class = sd->view_class = 14;
			// Advanced classes
			else if (sd->status.class == 4014)
				sd->status.class = sd->view_class = 4008;
			else if (sd->status.class == 4022)
				sd->status.class = sd->view_class = 4015;
			// Baby classes
			else if (sd->status.class == 4036)
				sd->status.class = sd->view_class = 4030;
			else if (sd->status.class == 4044)
				sd->status.class = sd->view_class = 4037;
		}
	} else {
		if (pc_isriding(sd)) { // sd has the new value...
			if (sd->disguise > 0) { // Temporary prevention of crash caused by Peco + disguise
				sd->status.option &= ~0x0020;
			} else {
				// Normal classes
				if (sd->status.class == 7)
					sd->status.class = sd->view_class = 13;
				else if (sd->status.class == 14)
					sd->status.class = sd->view_class = 21;
				// Advanced classes
				else if (sd->status.class == 4008)
					sd->status.class = sd->view_class = 4014;
				else if (sd->status.class == 4015)
					sd->status.class = sd->view_class = 4022;
				// Baby classes
				else if (sd->status.class == 4030)
					sd->status.class = sd->view_class = 4036;
				else if (sd->status.class == 4037)
					sd->status.class = sd->view_class = 4044;
				else
					sd->status.option &= ~0x0020;
			}
		}
	}

	clif_changeoption(&sd->bl);
	status_calc_pc(sd, 0);
	clif_displaymessage(fd, msg_txt(9)); // Options changed.
	if (pc_isfalcon(sd) && pc_iscarton(sd) && (sd->status.option & 0x0008) != 0x0008)
		clif_displaymessage(fd, "Falcon can display all carts like a basic cart (type 1).");

	return 0;
}

/*==========================================
 * @optionadd
 *------------------------------------------
 */
ATCOMMAND_FUNC(optionadd) {
	int opt1 = 0, opt2 = 0, opt3 = 0;

	if (!message || !*message || sscanf(message, "%d %d %d", &opt1, &opt2, &opt3) < 1 || opt1 < 0 || opt2 < 0 || opt3 < 0 || (opt1 == 0 && opt2 == 0 && opt3 == 0)) {
		send_usage(sd, "Please, enter at least a option (usage: %s <opt1:0+> <opt2:0+> <opt3:0+>).", original_command);
		return -1;
	}

	sd->opt1 |= opt1;
	sd->opt2 |= opt2;
	if (!(sd->status.option & CART_MASK) && (opt3 & CART_MASK)) {
		clif_cart_itemlist(sd);
		clif_cart_equiplist(sd);
		clif_updatestatus(sd, SP_CARTINFO);
	}
	sd->status.option |= opt3;
	// Fix Peco display
	if (sd->status.class ==   13 || sd->status.class ==   21 ||
	    sd->status.class == 4014 || sd->status.class == 4022 ||
	    sd->status.class == 4036 || sd->status.class == 4044) {
		if (!pc_isriding(sd)) { // sd has the new value...
			// Normal classes
			if (sd->status.class == 13)
				sd->status.class = sd->view_class = 7;
			else if (sd->status.class == 21)
				sd->status.class = sd->view_class = 14;
			// Advanced classes
			else if (sd->status.class == 4014)
				sd->status.class = sd->view_class = 4008;
			else if (sd->status.class == 4022)
				sd->status.class = sd->view_class = 4015;
			// Baby classes
			else if (sd->status.class == 4036)
				sd->status.class = sd->view_class = 4030;
			else if (sd->status.class == 4044)
				sd->status.class = sd->view_class = 4037;
		}
	} else {
		if (pc_isriding(sd)) { // sd has the new value...
			if (sd->disguise > 0) { // Temporary prevention of crash caused by Peco + disguise
				sd->status.option &= ~0x0020;
			} else {
				// Normal classes
				if (sd->status.class == 7)
					sd->status.class = sd->view_class = 13;
				else if (sd->status.class == 14)
					sd->status.class = sd->view_class = 21;
				// Advanced classes
				else if (sd->status.class == 4008)
					sd->status.class = sd->view_class = 4014;
				else if (sd->status.class == 4015)
					sd->status.class = sd->view_class = 4022;
				// Baby classes
				else if (sd->status.class == 4030)
					sd->status.class = sd->view_class = 4036;
				else if (sd->status.class == 4037)
					sd->status.class = sd->view_class = 4044;
				else
					sd->status.option &= ~0x0020;
			}
		}
	}

	clif_changeoption(&sd->bl);
	status_calc_pc(sd, 0);
	clif_displaymessage(fd, msg_txt(9)); // Options changed.
	if (pc_isfalcon(sd) && pc_iscarton(sd) && (sd->status.option & 0x0008) != 0x0008)
		clif_displaymessage(fd, "Falcon can display all carts like a basic cart (type 1).");

	return 0;
}

/*==========================================
 * @optionremove
 *------------------------------------------
 */
ATCOMMAND_FUNC(optionremove) {
	int opt1 = 0, opt2 = 0, opt3 = 0;

	if (!message || !*message || sscanf(message, "%d %d %d", &opt1, &opt2, &opt3) < 1 || opt1 < 0 || opt2 < 0 || opt3 < 0 || (opt1 == 0 && opt2 == 0 && opt3 == 0)) {
		send_usage(sd, "Please, enter at least a option (usage: %s <opt1:0+> <opt2:0+> <opt3:0+>).", original_command);
		return -1;
	}

	sd->opt1 &= ~opt1;
	sd->opt2 &= ~opt2;
	sd->status.option &= ~opt3;
	// Fix Peco display
	if (sd->status.class ==   13 || sd->status.class ==   21 ||
	    sd->status.class == 4014 || sd->status.class == 4022 ||
	    sd->status.class == 4036 || sd->status.class == 4044) {
		if (!pc_isriding(sd)) { // sd has the new value...
			// Normal classes
			if (sd->status.class == 13)
				sd->status.class = sd->view_class = 7;
			else if (sd->status.class == 21)
				sd->status.class = sd->view_class = 14;
			// Advanced classes
			else if (sd->status.class == 4014)
				sd->status.class = sd->view_class = 4008;
			else if (sd->status.class == 4022)
				sd->status.class = sd->view_class = 4015;
			// Baby classes
			else if (sd->status.class == 4036)
				sd->status.class = sd->view_class = 4030;
			else if (sd->status.class == 4044)
				sd->status.class = sd->view_class = 4037;
		}
	} else {
		if (pc_isriding(sd)) { // sd has the new value...
			if (sd->disguise > 0) { // Temporary prevention of crash caused by Peco + disguise
				sd->status.option &= ~0x0020;
			} else {
				// Normal classes
				if (sd->status.class == 7)
					sd->status.class = sd->view_class = 13;
				else if (sd->status.class == 14)
					sd->status.class = sd->view_class = 21;
				// Advanced classes
				else if (sd->status.class == 4008)
					sd->status.class = sd->view_class = 4014;
				else if (sd->status.class == 4015)
					sd->status.class = sd->view_class = 4022;
				// Baby classes
				else if (sd->status.class == 4030)
					sd->status.class = sd->view_class = 4036;
				else if (sd->status.class == 4037)
					sd->status.class = sd->view_class = 4044;
				else
					sd->status.option &= ~0x0020;
			}
		}
	}

	clif_changeoption(&sd->bl);
	status_calc_pc(sd, 0);
	clif_displaymessage(fd, msg_txt(9)); // Options changed.

	return 0;
}

/*==========================================
 * @hide - Makes user invisible
 *------------------------------------------
 */
ATCOMMAND_FUNC(hide) {
	if (sd->status.option & OPTION_HIDE) {
		sd->status.option &= ~OPTION_HIDE;
		clif_displaymessage(fd, msg_txt(10)); // Invisible: Off.
	} else {
		sd->status.option |= OPTION_HIDE;
		clif_displaymessage(fd, msg_txt(11)); // Invisible: On.
	}
	clif_changeoption(&sd->bl);

	return 0;
}

/*==========================================
 * @jobchange - Changes user's current job
 *------------------------------------------
 */
ATCOMMAND_FUNC(jobchange) {
	int job = 0, upper = -1;
	int flag, i;
	char jobname[100];

	const struct {
		char name[20]; 
		int id; 
	} jobs[] = {
		{ "novice",                    0 },
		{ "swordsman",                 1 },
		{ "swordman",                  1 },
		{ "mage",                      2 },
		{ "archer",                    3 },
		{ "acolyte",                   4 },
		{ "merchant",                  5 },
		{ "thief",                     6 },
		{ "knight",                    7 },
		{ "priest",                    8 },
		{ "priestess",                 8 },
		{ "wizard",                    9 },
		{ "blacksmith",               10 },
		{ "hunter",                   11 },
		{ "assassin",                 12 },
		{ "peco knight",              13 },
		{ "peco-knight",              13 },
		{ "peco_knight",              13 },
		{ "knight peco",              13 },
		{ "knight-peco",              13 },
		{ "knight_peco",              13 },
		{ "crusader",                 14 },
		{ "defender",                 14 },
		{ "monk",                     15 },
		{ "sage",                     16 },
		{ "rogue",                    17 },
		{ "alchemist",                18 },
		{ "bard",                     19 },
		{ "dancer",                   20 },
		{ "peco crusader",            21 },
		{ "peco-crusader",            21 },
		{ "peco_crusader",            21 },
		{ "crusader peco",            21 },
		{ "crusader-peco",            21 },
		{ "crusader_peco",            21 },
		{ "super novice",             23 },
		{ "super-novice",             23 },
		{ "super_novice",             23 },
		{ "supernovice",              23 },
		{ "gunslinger",               24 },
		{ "gunner",                   24 },
		{ "ninja",                    25 },
		{ "xmas",                     26 },
		{ "santa",                    26 },
		{ "novice high",            4001 },
		{ "novice_high",            4001 },
		{ "high novice",            4001 },
		{ "high_novice",            4001 },
		{ "swordsman high",         4002 },
		{ "swordsman_high",         4002 },
		{ "high swordsman",         4002 },
		{ "high_swordsman",         4002 },
		{ "swordman high",          4002 },
		{ "swordman_high",          4002 },
		{ "high swordman",          4002 },
		{ "high_swordman",          4002 },
		{ "mage high",              4003 },
		{ "mage_high",              4003 },
		{ "high mage",              4003 },
		{ "high_mage",              4003 },
		{ "archer high",            4004 },
		{ "archer_high",            4004 },
		{ "high archer",            4004 },
		{ "high_archer",            4004 },
		{ "acolyte high",           4005 },
		{ "acolyte_high",           4005 },
		{ "high acolyte",           4005 },
		{ "high_acolyte",           4005 },
		{ "merchant high",          4006 },
		{ "merchant_high",          4006 },
		{ "high merchant",          4006 },
		{ "high_merchant",          4006 },
		{ "thief high",             4007 },
		{ "thief_high",             4007 },
		{ "high thief",             4007 },
		{ "high_thief",             4007 },
		{ "lord knight",            4008 },
		{ "lord_knight",            4008 },
		{ "high priest",            4009 },
		{ "high_priest",            4009 },
		{ "high priestess",         4009 },
		{ "high_priestess",         4009 },
		{ "high wizard",            4010 },
		{ "high_wizard",            4010 },
		{ "whitesmith",             4011 },
		{ "white smith",            4011 },
		{ "white_smith",            4011 },
		{ "white-smith",            4011 },
		{ "mastersmith",            4011 },
		{ "master smith",           4011 },
		{ "master_smith",           4011 },
		{ "master-smith",           4011 },
		{ "sniper",                 4012 },
		{ "assassin cross",         4013 },
		{ "assassin_cross",         4013 },
		{ "assassin-cross",         4013 },
		{ "peco lord knight",       4014 },
		{ "peco-lord-knight",       4014 },
		{ "peco_lord_knight",       4014 },
		{ "lord knight peco",       4014 },
		{ "lord-knight-peco",       4014 },
		{ "lord_knight_peco",       4014 },
		{ "paladin",                4015 },
		{ "champion",               4016 },
		{ "scholar",                4017 },
		{ "professor",              4017 },
		{ "stalker",                4018 },
		{ "biochemist",             4019 },
		{ "creator",                4019 },
		{ "clown",                  4020 },
		{ "minstrel",               4020 },
		{ "gypsy",                  4021 },
		{ "peco paladin",           4022 },
		{ "peco-paladin",           4022 },
		{ "peco_paladin",           4022 },
		{ "paladin peco",           4022 },
		{ "paladin-peco",           4022 },
		{ "paladin_peco",           4022 },
		{ "baby novice",            4023 },
		{ "baby_novice",            4023 },
		{ "baby swordsman",         4024 },
		{ "baby_swordsman",         4024 },
		{ "baby swordman",          4024 },
		{ "baby_swordman",          4024 },
		{ "baby mage",              4025 },
		{ "baby_mage",              4025 },
		{ "baby archer",            4026 },
		{ "baby_archer",            4026 },
		{ "baby acolyte",           4027 },
		{ "baby_acolyte",           4027 },
		{ "baby merchant",          4028 },
		{ "baby_merchant",          4028 },
		{ "baby thief",             4029 },
		{ "baby_thief",             4029 },
		{ "baby knight",            4030 },
		{ "baby_knight",            4030 },
		{ "baby priest",            4031 },
		{ "baby_priest",            4031 },
		{ "baby priestess",         4031 },
		{ "baby_priestess",         4031 },
		{ "baby wizard",            4032 },
		{ "baby_wizard",            4032 },
		{ "baby blacksmith",        4033 },
		{ "baby_blacksmith",        4033 },
		{ "baby hunter",            4034 },
		{ "baby_hunter",            4034 },
		{ "baby assassin",          4035 },
		{ "baby_assassin",          4035 },
		{ "baby peco knight",       4036 },
		{ "baby peco-knight",       4036 },
		{ "baby peco_knight",       4036 },
		{ "baby_peco_knight",       4036 },
		{ "baby crusader",          4037 },
		{ "baby_crusader",          4037 },
		{ "baby defender",          4037 },
		{ "baby_defender",          4037 },
		{ "baby monk",              4038 },
		{ "baby_monk",              4038 },
		{ "baby sage",              4039 },
		{ "baby_sage",              4039 },
		{ "baby rogue",             4040 },
		{ "baby_rogue",             4040 },
		{ "baby alchemist",         4041 },
		{ "baby_alchemist",         4041 },
		{ "baby bard",              4042 },
		{ "baby_bard",              4042 },
		{ "baby dancer",            4043 },
		{ "baby_dancer",            4043 },
		{ "baby peco crusader",     4044 },
		{ "baby peco-crusader",     4044 },
		{ "baby peco_crusader",     4044 },
		{ "baby_peco_crusader",     4044 },
		{ "super baby",             4045 },
		{ "super-baby",             4045 },
		{ "super_baby",             4045 },
		{ "baby supernovice",       4045 },
		{ "baby super-novice",      4045 },
		{ "baby super novice",      4045 },
		{ "baby_supernovice",       4045 },
		{ "baby_super_novice",      4045 },
		{ "taekwon",                4046 },
		{ "taekwon kid",            4046 },
		{ "taekwon_kid",            4046 },
		{ "taekwon boy",            4046 },
		{ "taekwon_boy",            4046 },
		{ "taekwon girl",           4046 },
		{ "taekwon_girl",           4046 },
		{ "star gladiator",         4047 },
		{ "star_gladiator",         4047 },
		{ "star knight",            4047 },
		{ "star_knight",            4047 },
		{ "taekwon master",         4047 },
		{ "taekwon_master",         4047 },
		{ "star gladiator2",        4048 },
		{ "star_gladiator2",        4048 },
		{ "taekwon master2",        4048 },
		{ "taekwon_master2",        4048 },
		{ "soul linker",            4049 },
		{ "soul_linker",            4049 },
		{ "bon gun",                4050 },
		{ "bon_gun",                4050 },
		{ "bongun",                 4050 },
		{ "death knight",           4051 },
		{ "death_knight",           4051 },
		{ "dark collector",         4052 },
		{ "dark_collector",         4052 },
		{ "monster breeder",        4052 },
		{ "monster_breeder",        4052 },
		{ "munak",                  4053 },
	};

	if (!message || !*message || sscanf(message, "%u %u", &job, &upper) < 1 || job < 0 || job >= MAX_PC_CLASS || (job > 26 && job < 4001)) {
		/* search class name */
		i = (int)(sizeof(jobs) / sizeof(jobs[0]));
		if (sscanf(message, "\"%[^\"]\" %d", jobname, &upper) >= 1 ||
		    sscanf(message, "%s %d", jobname, &upper) >= 1) {
			for (i = 0; i < (int)(sizeof(jobs) / sizeof(jobs[0])); i++) {
				if (strcasecmp(jobname, jobs[i].name) == 0 || strcasecmp(message, jobs[i].name) == 0) {
					job = jobs[i].id;
					break;
				}
			}
		}
	/* if class name not found */
	if ((i == (int)(sizeof(jobs) / sizeof(jobs[0])))) {
			send_usage(sd, "Please, enter a valid job ID (usage: %s <job ID|\"job name\"> [upper]).", original_command);
			send_usage(sd, "   0 Novice            7 Knight           14 Crusader         21 Peco Crusader");
			send_usage(sd, "   1 Swordman          8 Priest           15 Monk             22 N/A");
			send_usage(sd, "   2 Mage              9 Wizard           16 Sage             23 Super Novice");
			send_usage(sd, "   3 Archer           10 Blacksmith       17 Rogue            24 Gunslinger");
			send_usage(sd, "   4 Acolyte          11 Hunter           18 Alchemist        25 Ninja");
			send_usage(sd, "   5 Merchant         12 Assassin         19 Bard             26 N/A");
			send_usage(sd, "   6 Thief            13 Peco Knight      20 Dancer");
			send_usage(sd, "4001 Novice High    4008 Lord Knight      4015 Paladin        4022 Peco Paladin");
			send_usage(sd, "4002 Swordman High  4009 High Priest      4016 Champion");
			send_usage(sd, "4003 Mage High      4010 High Wizard      4017 Professor");
			send_usage(sd, "4004 Archer High    4011 Whitesmith       4018 Stalker");
			send_usage(sd, "4005 Acolyte High   4012 Sniper           4019 Creator");
			send_usage(sd, "4006 Merchant High  4013 Assassin Cross   4020 Clown");
			send_usage(sd, "4007 Thief High     4014 Peco Lord Knight 4021 Gypsy");
			send_usage(sd, "4023 Baby Novice    4030 Baby Knight      4037 Baby Crusader  4044 Baby Peco Crusader");
			send_usage(sd, "4024 Baby Swordsman 4031 Baby Priest      4038 Baby Monk      4045 Super Baby");
			send_usage(sd, "4025 Baby Mage      4032 Baby Wizard      4039 Baby Sage      4046 Taekwon Kid");
			send_usage(sd, "4026 Baby Archer    4033 Baby Blacksmith  4040 Baby Rogue     4047 Taekwon Master");
			send_usage(sd, "4027 Baby Acolyte   4034 Baby Hunter      4041 Baby Alchemist 4048 N/A");
			send_usage(sd, "4028 Baby Merchant  4035 Baby Assassin    4042 Baby Bard      4049 Soul Linker");
			send_usage(sd, "4029 Baby Thief     4036 Baby Peco-Knight 4043 Baby Dancer");
			send_usage(sd, "[upper]: -1 (default) to automatically determine the 'level', 0 to force normal job, 1 to force high job.");
			return -1;
		}
	}

	if (job == sd->status.class) {
		clif_displaymessage(fd, "Your job is already the wished job.");
		return -1;
	}

	// Fix Peco display
	if ((job != 13 && job != 21 && job != 4014 && job != 4022 && job != 4036 && job != 4044)) {
		if (pc_isriding(sd)) {
			// Normal classes
			if (sd->status.class == 13)
				sd->status.class = sd->view_class = 7;
			else if (sd->status.class == 21)
				sd->status.class = sd->view_class = 14;
			// Advanced classes
			else if (sd->status.class == 4014)
				sd->status.class = sd->view_class = 4008;
			else if (sd->status.class == 4022)
				sd->status.class = sd->view_class = 4015;
			// Baby classes
			else if (sd->status.class == 4036)
				sd->status.class = sd->view_class = 4030;
			else if (sd->status.class == 4044)
				sd->status.class = sd->view_class = 4037;
			sd->status.option &= ~0x0020;
			clif_changeoption(&sd->bl);
			status_calc_pc(sd, 0);
		}
	} else {
		if (!pc_isriding(sd)) {
			// Normal classes
			if (job == 13)
				job = 7;
			else if (job == 21)
				job = 14;
			// Advanced classes
			else if (job == 4014)
				job = 4008;
			else if (job == 4022)
				job = 4015;
			// Baby classes
			else if (job == 4036)
				job = 4030;
			else if (job == 4044)
				job = 4037;
		}
	}

	flag = 0;
	for (i = 0; i < MAX_INVENTORY; i++) {
		if (sd->status.inventory[i].nameid && (sd->status.inventory[i].equip & 34) != 0) { // Right hand (2) + left hand (32)
			pc_unequipitem(sd, i, 3); // Unequip weapon to avoid sprite error
			flag = 1;
		}
	}
	if (flag)
		clif_displaymessage(fd, "Weapon unequiped to avoid sprite error.");

	if (pc_jobchange(sd, job, upper) == 0)
		clif_displaymessage(fd, msg_txt(12)); // Your job has been changed.
	else {
		clif_displaymessage(fd, msg_txt(155)); // Impossible to change your job.
		return -1;
	}

	return 0;
}

/*==========================================
 * @die - Kills current user
 *------------------------------------------
 */
ATCOMMAND_FUNC(die) {
	/* if dead */
	if (pc_isdead(sd)) {
		clif_displaymessage(fd, "You are dead. You must be alive to kill yourself.");
		return -1;
	}

	clif_specialeffect(&sd->bl, 450, 0); // Flag: 0: Player see in the area (Normal), 1: Only player see only by player, 2: All players in a map that see only their (Not see others), 3: All players that see only their (Not see others)
	pc_damage(NULL, sd, sd->status.hp + 1);
	clif_displaymessage(fd, msg_txt(13)); // A pity! You've died.

	return 0;
}

/*==========================================
 * @kill - Kills a target
 *------------------------------------------
 */
ATCOMMAND_FUNC(kill) {
	struct map_session_data *pl_sd;

	if (!message || !*message || sscanf(message, "%[^\n]", atcmd_name) < 1) {
		send_usage(sd, "Please, enter a player name (usage: %s <char name|account_id>).", original_command);
		return -1;
	}

	if ((pl_sd = map_nick2sd(atcmd_name)) != NULL || ((pl_sd = map_id2sd(atoi(atcmd_name))) != NULL && pl_sd->state.auth)) {
		if (sd->GM_level >= pl_sd->GM_level) { // Only lower or same level
			pc_damage(NULL, pl_sd, pl_sd->status.hp + 1);
			clif_displaymessage(fd, msg_txt(14)); // Character killed.
		} else {
			clif_displaymessage(fd, msg_txt(81)); // Your GM level don't authorize you to do this action on this player.
			return -1;
		}
	} else {
		clif_displaymessage(fd, msg_txt(3)); // Character not found.
		return -1;
	}

	return 0;
}

/*==========================================
 * @alive - Revives user
 *------------------------------------------
 */
ATCOMMAND_FUNC(alive) {
	// If not dead, call @heal command
	if (!pc_isdead(sd))
		return atcommand_heal(fd, sd, original_command, "@heal", "");

	sd->status.hp = sd->status.max_hp;
	sd->status.sp = sd->status.max_sp;
	clif_skill_nodamage(&sd->bl, &sd->bl, ALL_RESURRECTION, 4, 1);
	pc_setstand(sd);
	if (battle_config.pc_invincible_time > 0)
		pc_setinvincibletimer(sd, battle_config.pc_invincible_time);
	clif_updatestatus(sd, SP_HP);
	clif_updatestatus(sd, SP_SP);
	clif_resurrection(&sd->bl, 1);
	clif_displaymessage(fd, msg_txt(16)); // You've been revived! It's a miracle!

	return 0;
}

/*==========================================
 * @heal - Heals yourself
 *------------------------------------------
 */
ATCOMMAND_FUNC(heal) {
	int hp = 0, sp = 0;

	// If dead
	if (pc_isdead(sd)) {
		clif_displaymessage(fd, "You are dead. You must be resurrected before you can be healed.");
		return -1;
	}

	sscanf(message, "%d %d", &hp, &sp);

	if (hp == 0 && sp == 0) {
		hp = sd->status.max_hp - sd->status.hp;
		sp = sd->status.max_sp - sd->status.sp;
	} else {
		if (hp > 0 && (hp > sd->status.max_hp || hp > (sd->status.max_hp - sd->status.hp))) // Fix positive overflow
			hp = sd->status.max_hp - sd->status.hp;
		else if (hp < 0 && (hp < -sd->status.max_hp || hp < (1 - sd->status.hp))) // Fix negative overflow
			hp = 1 - sd->status.hp;
		if (sp > 0 && (sp > sd->status.max_sp || sp > (sd->status.max_sp - sd->status.sp))) // Fix positive overflow
			sp = sd->status.max_sp - sd->status.sp;
		else if (sp < 0 && (sp < -sd->status.max_sp || sp < (1 - sd->status.sp))) // Fix negative overflow
			sp = 1 - sd->status.sp;
	}

	if (hp > 0) // Display like heal
		clif_heal(fd, SP_HP, hp);
	else if (hp < 0) // Display like damage
		clif_damage(&sd->bl,&sd->bl, gettick_cache, 0, 0, -hp, 0 , 4, 0);
	if (sp > 0) // No display if we lost SP
		clif_heal(fd, SP_SP, sp);

	if (hp != 0 || sp != 0) {
		pc_heal(sd, hp, sp);
		if (hp >= 0 && sp >= 0)
			clif_displaymessage(fd, msg_txt(17)); // HP, SP recovered.
		else
			clif_displaymessage(fd, msg_txt(156)); // HP or/and SP modified.
	} else {
		clif_displaymessage(fd, msg_txt(157)); // HP and SP are already with the good values.
		return -1;
	}

	return 0;
}

/*==========================================
 * @kami
 *------------------------------------------
 */
ATCOMMAND_FUNC(kami) {
	int color;

	switch (*(command + 5)) {
	case 'c': // Command is converted to lower case before to be called
		if (!message || !*message || sscanf(message, "%x %[^\n]", &color, atcmd_output) < 2) {
			send_usage(sd, "Please, enter color and message (usage: %s <hex_color> <message>).", original_command);
			return -1;
		}
		if (color < 0 || color > 0xFFFFFF) {
			clif_displaymessage(fd, msg_txt(282)); // Invalid color.
			return -1;
		}
		if (check_bad_word(message, strlen(message), sd))
			return -1; // Check_bad_word function display message if necessary
		intif_announce(atcmd_output, color, 0);
		break;

	default:
		if (!message || !*message) {
			send_usage(sd, "Please, enter a message (usage: %s <message>).", original_command);
			return -1;
		}

		// Because /b give gm name, we check which sentence we propose for bad words
		if (strncmp(message, sd->status.name, strlen(sd->status.name)) == 0 && message[strlen(sd->status.name)] == ':') {
			// printf("/b: Check with GM name: %s\n", message);
			if (check_bad_word(message + strlen(sd->status.name) + 1, strlen(message), sd))
				return -1; // Check_bad_word function display message if necessary
		} else {
			// printf("/b: Check without GM name: %s\n", message);
			if (check_bad_word(message, strlen(message), sd))
				return -1; // Check_bad_word function display message if necessary
		}

		intif_GMmessage((char*)message, 0);
		break;
	}

	return 0;
}

/*==========================================
 * @kamib
 *------------------------------------------
 */
ATCOMMAND_FUNC(kamib) {
	if (!message || !*message) {
		send_usage(sd, "Please, enter a message (usage: %s <message>).", original_command);
		return -1;
	}

	if (check_bad_word(message, strlen(message), sd))
		return -1; // Check_bad_word function display message if necessary

	intif_GMmessage((char*)message, 0x10);

	return 0;
}

/*==========================================
 * @kamiGM
 *------------------------------------------
 */
ATCOMMAND_FUNC(kamiGM) {
	int min_level;
	char message_to_gm[100];
	char message_to_gm2[125];

	memset(message_to_gm2, 0, sizeof(message_to_gm2));

	if (!message || !*message || sscanf(message, "%d %[^\n]", &min_level, message_to_gm) < 2) {
		send_usage(sd, "Please, enter a level and a message (usage: %s <min_gm_level> <message>).", original_command);
		return -1;
	}

	if (check_bad_word(message_to_gm, strlen(message_to_gm), sd))
		return -1; // Check_bad_word function display message if necessary

	intif_wis_message_to_gm(sd->status.name, min_level, message_to_gm);

	if (min_level == 0)
		sprintf(message_to_gm2, msg_txt(593), min_level); // To all players
	else
		sprintf(message_to_gm2, msg_txt(541), min_level); // To GM >= %d
	strncpy(message_to_gm2 + strlen(message_to_gm2), message_to_gm, strlen(message_to_gm));
	clif_wis_message(fd, "You", message_to_gm2, strlen(message_to_gm2) + 1); // R 0097 <len>.w <nick>.24B <message>.?B

	return 0;
}



/*==========================================
 * Function to check if an item is not forbidden for creation
 * return 0: not authorized
 * return not 0: authorized
 *------------------------------------------
 */
int check_item_authorization(const int item_id, const unsigned char gm_level) {
	FILE *fp;
	char line[512];
	int id, level;
	const char *filename = "db/item_deny.txt";

	if ((fp = fopen(filename, "r")) == NULL) {
		// printf("Items deny file not found: %s.\n", filename); // Not display every time
		return 1; // By default: Not denied -> Authorized
	}

	while(fgets(line, sizeof(line), fp)) { // fgets reads until maximum one less than size and add '\0' -> so, it's not necessary to add -1
		if ((line[0] == '/' && line[1] == '/') || line[0] == '\0' || line[0] == '\n' || line[0] == '\r')
			continue;
		// It's not necessary to remove 'carriage return ('\n' or '\r')
		level = 100; // Default: If no mentioned GM level, no creation -> 100
		if (sscanf(line, "%d,%d", &id, &level) < 1)
			continue;
		if (id == item_id) {
			fclose(fp);
			if (gm_level < level)
				return 0; // Not authorized
			else
				return 1; // Authorized
		}
	}
	fclose(fp);

	return 1; // Autorized
}

/*==========================================
 * @item command
 *------------------------------------------
 */
ATCOMMAND_FUNC(item) {
	char item_name[100];
	int number = 0, item_id, flag;
	struct item item_tmp;
	struct item_data *item_data;
	int get_count, i, pet_id;

	memset(item_name, 0, sizeof(item_name));

	if (!message || !*message || sscanf(message, "%s %d", item_name, &number) < 1) {
		send_usage(sd, "Please, enter an item name/id (usage: %s <item name | ID> [quantity]).", original_command);
		return -1;
	}

	if (number <= 0)
		number = 1;

	if(number > MAX_AMOUNT)
		number = MAX_AMOUNT;

	item_id = 0;
	if ((item_data = itemdb_searchname(item_name)) != NULL ||
	    (item_data = itemdb_exists(atoi(item_name))) != NULL)
		item_id = item_data->nameid;

	if (item_id >= 501) {
		if (check_item_authorization(item_id, sd->GM_level)) {
			get_count = number;
			// Check Pet Egg
			pet_id = search_petDB_index(item_id, PET_EGG);
			if (item_data->type == 4 || item_data->type == 5 || // 4 = Weapons, 5 = Armors
				item_data->type == 7 || item_data->type == 8) { // 7 = Eggs, 8 = Pet Accessories
				get_count = 1;
			}
			for (i = 0; i < number; i += get_count) {
				// If Pet Egg
				if (pet_id >= 0) {
					sd->catch_target_class = pet_db[pet_id].class;
					intif_create_pet(sd->status.account_id, sd->status.char_id,
					                 pet_db[pet_id].class, mob_db[pet_db[pet_id].class].lv,
					                 pet_db[pet_id].EggID, 0, pet_db[pet_id].intimate,
					                 100, 0, 1, pet_db[pet_id].jname);
				// If not Pet Egg
				} else {
					memset(&item_tmp, 0, sizeof(item_tmp));
					item_tmp.nameid = item_id;
					item_tmp.identify = 1;
					if ((battle_config.atcommand_item_creation_name_input && !item_data->slot) ||
					    battle_config.atcommand_item_creation_name_input > 1) {
						item_tmp.card[0] = 0x00fe; // Add GM name
						item_tmp.card[1] = 0;
						*((unsigned long *)(&item_tmp.card[2])) = sd->char_id;
					}
					if ((flag = pc_additem(sd, &item_tmp, get_count)))
						clif_additem(sd, 0, 0, flag);
				}
			}
			clif_displaymessage(fd, msg_txt(18)); // Item created.
		} else {
			clif_displaymessage(fd, "You are not authorized to create this item.");
			return -1;
		}
	} else {
		clif_displaymessage(fd, msg_txt(19)); // Invalid item ID or name.
		return -1;
	}

	return 0;
}

/*==========================================
 * @charitem command
 *------------------------------------------
 */
ATCOMMAND_FUNC(charitem) {
	struct map_session_data *pl_sd;
	char item_name[100];
	int number = 0, item_id, flag;
	struct item item_tmp;
	struct item_data *item_data;
	int get_count, i, pet_id;

	memset(item_name, 0, sizeof(item_name));

	if (!message || !*message || sscanf(message, "%s %d %[^\n]", item_name, &number, atcmd_name) < 3) {
		send_usage(sd, "Please, enter an item name/id and a player name (usage: %s <item name | ID> <quantity> <char name|account_id>).", original_command);
		return -1;
	}

	if (number <= 0)
		number = 1;

	if(number > MAX_AMOUNT)
		number = MAX_AMOUNT;

	item_id = 0;
	if ((item_data = itemdb_searchname(item_name)) != NULL ||
	    (item_data = itemdb_exists(atoi(item_name))) != NULL)
		item_id = item_data->nameid;

	if (item_id >= 501) {
		if (check_item_authorization(item_id, sd->GM_level)) {
			get_count = number;
			// Check Pet Egg
			pet_id = search_petDB_index(item_id, PET_EGG);
			if (item_data->type == 4 || item_data->type == 5 || // 4 = Weapons, 5 = Armors
				item_data->type == 7 || item_data->type == 8) { // 7 = Eggs, 8 = Pet Accessories
				get_count = 1;
			}
			if ((pl_sd = map_nick2sd(atcmd_name)) != NULL || ((pl_sd = map_id2sd(atoi(atcmd_name))) != NULL && pl_sd->state.auth)) {
				for (i = 0; i < number; i += get_count) {
					// If Pet Egg
					if (pet_id >= 0) {
						pl_sd->catch_target_class = pet_db[pet_id].class;
						intif_create_pet(pl_sd->status.account_id, pl_sd->status.char_id,
						                 pet_db[pet_id].class, mob_db[pet_db[pet_id].class].lv,
						                 pet_db[pet_id].EggID, 0, pet_db[pet_id].intimate,
						                 100, 0, 1, pet_db[pet_id].jname);
					// If not Pet Egg
					} else {
						memset(&item_tmp, 0, sizeof(item_tmp));
						item_tmp.nameid = item_id;
						item_tmp.identify = 1;
						if ((battle_config.atcommand_item_creation_name_input && !item_data->slot) ||
						    battle_config.atcommand_item_creation_name_input > 1) {
							item_tmp.card[0] = 0x00fe; // Add GM name
							item_tmp.card[1] = 0;
							*((unsigned long *)(&item_tmp.card[2])) = sd->char_id;
						}
						if ((flag = pc_additem(pl_sd, &item_tmp, get_count)))
							clif_additem(pl_sd, 0, 0, flag);
					}
				}
				sprintf(atcmd_output, "Player received %d %s.", number, item_data->jname);
				clif_displaymessage(fd, atcmd_output);
			} else {
				clif_displaymessage(fd, msg_txt(3)); // Character not found.
				return -1;
			}
		} else {
			clif_displaymessage(fd, "You are not authorized to create this item.");
			return -1;
		}
	} else {
		clif_displaymessage(fd, msg_txt(19)); // Invalid item ID or name.
		return -1;
	}

	return 0;
}

/*==========================================
 * @itemall/@charitemall/@giveitemall command
 *------------------------------------------
 */
ATCOMMAND_FUNC(charitemall) {
	struct map_session_data *pl_sd;
	char item_name[100];
	int number = 0, item_id, flag;
	struct item item_tmp;
	struct item_data *item_data;
	int get_count, i, j, pet_id, c;

	memset(item_name, 0, sizeof(item_name));

	if (!message || !*message || sscanf(message, "%s %d", item_name, &number) < 1) {
		send_usage(sd, "Please, enter an item name/id (usage: %s <item name | ID> [quantity]).", original_command);
		return -1;
	}

	if (number <= 0)
		number = 1;

	if(number > MAX_AMOUNT)
		number = MAX_AMOUNT;

	item_id = 0;
	if ((item_data = itemdb_searchname(item_name)) != NULL ||
	    (item_data = itemdb_exists(atoi(item_name))) != NULL)
		item_id = item_data->nameid;

	if (item_id >= 501) {
		if (check_item_authorization(item_id, sd->GM_level)) {
			get_count = number;
			// Check Pet Egg
			pet_id = search_petDB_index(item_id, PET_EGG);
			if (item_data->type == 4 || item_data->type == 5 || // 4 = Weapons, 5 = Armors
				item_data->type == 7 || item_data->type == 8) { // 7 = Eggs, 8 = Pet Accessories
				get_count = 1;
			}
			c = 0;
			for (j = 0; j < fd_max; j++) {
				if (session[j] && (pl_sd = session[j]->session_data) && pl_sd->state.auth) {
					for (i = 0; i < number; i += get_count) {
						// If Pet Egg
						if (pet_id >= 0) {
							pl_sd->catch_target_class = pet_db[pet_id].class;
							intif_create_pet(pl_sd->status.account_id, pl_sd->status.char_id,
							                 pet_db[pet_id].class, mob_db[pet_db[pet_id].class].lv,
							                 pet_db[pet_id].EggID, 0, pet_db[pet_id].intimate,
							                 100, 0, 1, pet_db[pet_id].jname);
						// If not Pet Egg
						} else {
							memset(&item_tmp, 0, sizeof(item_tmp));
							item_tmp.nameid = item_id;
							item_tmp.identify = 1;
							if ((battle_config.atcommand_item_creation_name_input && !item_data->slot) ||
							    battle_config.atcommand_item_creation_name_input > 1) {
								item_tmp.card[0] = 0x00fe; // Add GM name
								item_tmp.card[1] = 0;
								*((unsigned long *)(&item_tmp.card[2])) = sd->char_id;
							}
							if ((flag = pc_additem(pl_sd, &item_tmp, get_count)))
								clif_additem(pl_sd, 0, 0, flag);
						}
					}
					sprintf(atcmd_output, "You got %d %s.", number, item_data->jname);
					clif_displaymessage(pl_sd->fd, atcmd_output);
					c++;
				}
			}
			sprintf(atcmd_output, "Everyone received %d %s.", number, item_data->jname);
			clif_displaymessage(fd, atcmd_output);
		} else {
			clif_displaymessage(fd, "You are not authorized to create this item.");
			return -1;
		}
	} else {
		clif_displaymessage(fd, msg_txt(19)); // Invalid item ID or name.
		return -1;
	}

	return 0;
}

/*==========================================
 * @item2
 *------------------------------------------
 */
ATCOMMAND_FUNC(item2) {
	struct item item_tmp;
	struct item_data *item_data;
	char item_name[100];
	int item_id, number = 0;
	int identify = 0, refine = 0, attr = 0;
	int c1 = 0, c2 = 0, c3 = 0, c4 = 0;
	int flag;
	int loop, get_count, i;

	memset(item_name, 0, sizeof(item_name));

	if (!message || !*message || sscanf(message, "%s %d %d %d %d %d %d %d %d", item_name, &number, &identify, &refine, &attr, &c1, &c2, &c3, &c4) < 9) {
		send_usage(sd, "Please, enter all informations (usage: %s <item name or ID> <quantity>", original_command);
		send_usage(sd, "  <Identify_flag> <refine> <attribut> <Card1> <Card2> <Card3> <Card4>).");
		return -1;
	}

	if (number <= 0)
		number = 1;

	if(number > MAX_AMOUNT)
		number = MAX_AMOUNT;

	item_id = 0;
	if ((item_data = itemdb_searchname(item_name)) != NULL ||
	    (item_data = itemdb_exists(atoi(item_name))) != NULL)
		item_id = item_data->nameid;

	if (item_id > 500) {
		if (check_item_authorization(item_id, sd->GM_level)) {
			loop = 1;
			get_count = number;
			if (item_data->type == 4 || item_data->type == 5 || // 4 = Weapons, 5 = Armors
				item_data->type == 7 || item_data->type == 8) { // 7 = Eggs, 8 = Pet Accessories
				loop = number;
				get_count = 1;
				if (item_data->type == 7) { // 7 = Eggs
					identify = 1;
					refine = 0;
				}
				if (item_data->type == 8) // 8 = Pet Accessories
					refine = 0;
			} else {
				identify = 1;
				refine = 0;
				attr = 0;
			}
			if (refine > 10)
				refine = 10;
			else if (refine < 0)
				refine = 0;
			for (i = 0; i < loop; i++) {
				memset(&item_tmp, 0, sizeof(item_tmp));
				item_tmp.nameid = item_id;
				item_tmp.identify = identify;
				item_tmp.refine = refine;
				item_tmp.attribute = attr;
				item_tmp.card[0] = c1;
				item_tmp.card[1] = c2;
				item_tmp.card[2] = c3;
				item_tmp.card[3] = c4;
				if ((flag = pc_additem(sd, &item_tmp, get_count)))
					clif_additem(sd, 0, 0, flag);
			}
			clif_displaymessage(fd, msg_txt(18)); // Item created.
		} else {
			clif_displaymessage(fd, "You are not authorized to create this item.");
			return -1;
		}
	} else {
		clif_displaymessage(fd, msg_txt(19)); // Invalid item ID or name.
		return -1;
	}

	return 0;
}

/*==========================================
 * @itemreset - Removes all user's items in inventory
 *------------------------------------------
 */
ATCOMMAND_FUNC(itemreset) {
	int i;

	for (i = 0; i < MAX_INVENTORY; i++) {
		if (sd->status.inventory[i].amount && sd->status.inventory[i].equip == 0)
			pc_delitem(sd, i, sd->status.inventory[i].amount, 0);
	}
	clif_displaymessage(fd, msg_txt(20)); // All of your inventory items have been removed.

	return 0;
}

/*==========================================
 * @charitemreset - Removes all target's items in inventory
 *------------------------------------------
 */
ATCOMMAND_FUNC(charitemreset) {
	struct map_session_data *pl_sd;
	int i;

	if (!message || !*message || sscanf(message, "%[^\n]", atcmd_name) < 1) {
		send_usage(sd, "Please, enter a player name (usage: %s <char name|account_id>).", original_command);
		return -1;
	}

	if ((pl_sd = map_nick2sd(atcmd_name)) != NULL || ((pl_sd = map_id2sd(atoi(atcmd_name))) != NULL && pl_sd->state.auth)) {
		if (sd->GM_level >= pl_sd->GM_level) { // Only lower or same level
			for (i = 0; i < MAX_INVENTORY; i++) {
				if (pl_sd->status.inventory[i].amount && pl_sd->status.inventory[i].equip == 0)
					pc_delitem(pl_sd, i, pl_sd->status.inventory[i].amount, 0);
			}
			clif_displaymessage(pl_sd->fd, msg_txt(20)); // All of your inventory items have been removed.
			clif_displaymessage(fd, msg_txt(241)); // All inventory items of the player have been removed.
		} else {
			clif_displaymessage(fd, msg_txt(81)); // Your GM level don't authorize you to do this action on this player.
			return -1;
		}
	} else {
		clif_displaymessage(fd, msg_txt(3)); // Character not found.
		return -1;
	}

	return 0;
}

/*==========================================
 * @itemcheck
 *------------------------------------------
 */
ATCOMMAND_FUNC(itemcheck) {
	pc_checkitem(sd);
	clif_displaymessage(fd, msg_txt(242)); // All your items have been checked.

	return 0;
}

/*==========================================
 * @charitemcheck
 *------------------------------------------
 */
ATCOMMAND_FUNC(charitemcheck) {
	struct map_session_data *pl_sd;

	if (!message || !*message || sscanf(message, "%[^\n]", atcmd_name) < 1) {
		send_usage(sd, "Please, enter a player name (usage: %s <char name|account_id>).", original_command);
		return -1;
	}

	if ((pl_sd = map_nick2sd(atcmd_name)) != NULL || ((pl_sd = map_id2sd(atoi(atcmd_name))) != NULL && pl_sd->state.auth)) {
		if (sd->GM_level >= pl_sd->GM_level) { // Only lower or same level
			pc_checkitem(pl_sd);
			if (pl_sd != sd)
				clif_displaymessage(pl_sd->fd, "All your items have been checked by a GM.");
			clif_displaymessage(fd, "Item check on the player done.");
		} else {
			clif_displaymessage(fd, msg_txt(81)); // Your GM level don't authorize you to do this action on this player.
			return -1;
		}
	} else {
		clif_displaymessage(fd, msg_txt(3)); // Character not found.
		return -1;
	}

	return 0;
}

/*==========================================
 * @baselevelup - Levels up user's base level
 *------------------------------------------
 */
ATCOMMAND_FUNC(baselevelup) {
	int level, i, modified_stat;
	short modified_status[6]; // Need to update only modifed stats

	if (!message || !*message || sscanf(message, "%d", &level) < 1 || level == 0) {
		send_usage(sd, "Please, enter a level adjustement (usage: %s <number of levels>).", original_command);
		return -1;
	}

	if (level > 0) {
		if (sd->status.base_level == battle_config.maximum_level) { // Check for max level
			clif_displaymessage(fd, msg_txt(47)); // Base level can't go any higher.
			return -1;
		}
		if (level > battle_config.maximum_level || level > (battle_config.maximum_level - sd->status.base_level)) // Fix positive overflow
			level = battle_config.maximum_level - sd->status.base_level;
		for (i = 1; i <= level; i++)
			sd->status.status_point += (sd->status.base_level + i + 14) / 5;
		// if player have max in all stats, don't give status_point
		if (sd->status.str  >= battle_config.max_parameter &&
		    sd->status.agi  >= battle_config.max_parameter &&
		    sd->status.vit  >= battle_config.max_parameter &&
		    sd->status.int_ >= battle_config.max_parameter &&
		    sd->status.dex  >= battle_config.max_parameter &&
		    sd->status.luk  >= battle_config.max_parameter)
			sd->status.status_point = 0;
		sd->status.base_level += level;
		clif_updatestatus(sd, SP_BASELEVEL);
		clif_updatestatus(sd, SP_NEXTBASEEXP);
		clif_updatestatus(sd, SP_STATUSPOINT);
		status_calc_pc(sd, 0);
		pc_heal(sd, sd->status.max_hp, sd->status.max_sp);
		clif_misceffect(&sd->bl, 0);
		clif_displaymessage(fd, msg_txt(21)); // Base level raised.
	} else {
		if (sd->status.base_level == 1) {
			clif_displaymessage(fd, msg_txt(158)); // Base level can't go any lower.
			return -1;
		}
		if (level < -battle_config.maximum_level || level < (1 - sd->status.base_level)) // Fix negative overflow
			level = 1 - sd->status.base_level;
		for (i = 0; i > level; i--)
			sd->status.status_point -= (sd->status.base_level + i + 14) / 5;
		if (sd->status.status_point < 0) {
			short* status[] = {
				&sd->status.str,  &sd->status.agi, &sd->status.vit,
				&sd->status.int_, &sd->status.dex, &sd->status.luk
			};
			// Remove points from stats beginning by the upper
			for (i = 0; i <= MAX_STATUS_TYPE; i++)
				modified_status[i] = 0;
			while (sd->status.status_point < 0 &&
			       sd->status.str > 0 &&
			       sd->status.agi > 0 &&
			       sd->status.vit > 0 &&
			       sd->status.int_ > 0 &&
			       sd->status.dex > 0 &&
			       sd->status.luk > 0 &&
			       (sd->status.str + sd->status.agi + sd->status.vit + sd->status.int_ + sd->status.dex + sd->status.luk > 6)) {
				modified_stat = 0;
				for (i = 1; i <= MAX_STATUS_TYPE; i++)
					if (*status[modified_stat] < *status[i])
						modified_stat = i;
				sd->status.status_point += (*status[modified_stat] + 8) / 10 + 1; // ((val - 1) + 9) / 10 + 1
				*status[modified_stat] = *status[modified_stat] - ((short)1);
				modified_status[modified_stat] = 1; // Flag
			}
			for (i = 0; i <= MAX_STATUS_TYPE; i++)
				if (modified_status[i]) {
					clif_updatestatus(sd, SP_STR + i);
					clif_updatestatus(sd, SP_USTR + i);
				}
		}
		clif_updatestatus(sd, SP_STATUSPOINT);
		sd->status.base_level += level; // Note: Here, level is negative
		clif_updatestatus(sd, SP_BASELEVEL);
		clif_updatestatus(sd, SP_NEXTBASEEXP);
		status_calc_pc(sd, 0);
		clif_displaymessage(fd, msg_txt(22)); // Base level lowered.
	}

	return 0;
}

/*==========================================
 * @joblevelup - Levels up user's job level
 *------------------------------------------
 */
ATCOMMAND_FUNC(joblevelup) {
	int up_level, level;

	if (!message || !*message || sscanf(message, "%d", &level) < 1 || level == 0) {
		send_usage(sd, "Please, enter a level adjustment (usage: %s <number of levels>).", original_command);
		return -1;
	}

// Maximum job level checking for each class

    // Standard classes can go to job 50
    up_level = 50;
    // Novice can go to job 10
    if (sd->status.class == 0 || sd->status.class == 4001 || sd->status.class == 4023)
        up_level = 10;
    // Super Novice can go to job 99
    if (sd->status.class == 23 || sd->status.class == 4045)
        up_level = 99;
    // Advanced classes can go up to level 70
    if ((sd->status.class >= 4008) && (sd->status.class <= 4022))
        up_level = 70;
    // Ninja and Gunslingers should be able to go to job level 70
    if ((sd->status.class == 24) || (sd->status.class == 25))
        up_level = 70;

	if (level > 0) {
		if (sd->status.job_level == up_level) {
			clif_displaymessage(fd, msg_txt(23)); // Job level can't go any higher
			return -1;
		}
		if (level > up_level || level > (up_level - sd->status.job_level)) // Fix positive overflow
			level = up_level - sd->status.job_level;
		// Check maximum authorized job level
		if (sd->status.class == 0) { // Novice
			if (sd->status.job_level >= battle_config.atcommand_max_job_level_novice) {
				clif_displaymessage(fd, "You're not authorized to increase more your job level.");
				return -1;
			} else if (sd->status.job_level + level >= battle_config.atcommand_max_job_level_novice)
				level = battle_config.atcommand_max_job_level_novice - sd->status.job_level;
		} else if (sd->status.class <= 6 || sd->status.class == 4046) { // First Job/Taekwon
			if (sd->status.job_level >= battle_config.atcommand_max_job_level_job1) {
				clif_displaymessage(fd, "You're not authorized to increase more your job level.");
				return -1;
			} else if (sd->status.job_level + level >= battle_config.atcommand_max_job_level_job1)
				level = battle_config.atcommand_max_job_level_job1 - sd->status.job_level;
		} else if (sd->status.class <= 22 || (sd->status.class >= 4047 && sd->status.class <= 4049)) { // Second Job/Star Gladiator/Soul Linker
			if (sd->status.job_level >= battle_config.atcommand_max_job_level_job2) {
				clif_displaymessage(fd, "You're not authorized to increase more your job level.");
				return -1;
			} else if (sd->status.job_level + level >= battle_config.atcommand_max_job_level_job2)
				level = battle_config.atcommand_max_job_level_job2 - sd->status.job_level;
		} else if (sd->status.class == 23) { // Super Novice
			if (sd->status.job_level >= battle_config.atcommand_max_job_level_supernovice) {
				clif_displaymessage(fd, "You're not authorized to increase more your job level.");
				return -1;
			} else if (sd->status.job_level + level >= battle_config.atcommand_max_job_level_supernovice)
				level = battle_config.atcommand_max_job_level_supernovice - sd->status.job_level;
		} else if (sd->status.class == 24) { // Gunslinger
			if (sd->status.job_level >= battle_config.atcommand_max_job_level_gunslinger) {
				clif_displaymessage(fd, "You're not authorized to increase more your job level.");
				return -1;
			} else if (sd->status.job_level + level >= 70)
				level = 70 - sd->status.job_level;
		} else if (sd->status.class == 25) { // Ninja
			if (sd->status.job_level >= battle_config.atcommand_max_job_level_ninja) {
				clif_displaymessage(fd, "You're not authorized to increase more your job level.");
				return -1;
			} else if (sd->status.job_level + level >= 70)
				level = 70 - sd->status.job_level;
		} else if (sd->status.class == 4001) { // High Novice
			if (sd->status.job_level >= battle_config.atcommand_max_job_level_highnovice) {
				clif_displaymessage(fd, "You're not authorized to increase more your job level.");
				return -1;
			} else if (sd->status.job_level + level >= battle_config.atcommand_max_job_level_highnovice)
				level = battle_config.atcommand_max_job_level_highnovice - sd->status.job_level;
		} else if (sd->status.class <= 4007) { // High First Job
			if (sd->status.job_level >= battle_config.atcommand_max_job_level_highjob1) {
				clif_displaymessage(fd, "You're not authorized to increase more your job level.");
				return -1;
			} else if (sd->status.job_level + level >= battle_config.atcommand_max_job_level_highjob1)
				level = battle_config.atcommand_max_job_level_highjob1 - sd->status.job_level;
		} else if (sd->status.class <= 4022) { // High Second Job
			if (sd->status.job_level >= battle_config.atcommand_max_job_level_highjob2) {
				clif_displaymessage(fd, "You're not authorized to increase more your job level.");
				return -1;
			} else if (sd->status.job_level + level >= battle_config.atcommand_max_job_level_highjob2)
				level = battle_config.atcommand_max_job_level_highjob2 - sd->status.job_level;
		} else if (sd->status.class == 4023) { // Baby Novice
			if (sd->status.job_level >= battle_config.atcommand_max_job_level_babynovice) {
				clif_displaymessage(fd, "You're not authorized to increase more your job level.");
				return -1;
			} else if (sd->status.job_level + level >= battle_config.atcommand_max_job_level_babynovice)
				level = battle_config.atcommand_max_job_level_babynovice - sd->status.job_level;
		} else if (sd->status.class <= 4029) { // Baby First Job
			if (sd->status.job_level >= battle_config.atcommand_max_job_level_babyjob1) {
				clif_displaymessage(fd, "You're not authorized to increase more your job level.");
				return -1;
			} else if (sd->status.job_level + level >= battle_config.atcommand_max_job_level_babyjob1)
				level = battle_config.atcommand_max_job_level_babyjob1 - sd->status.job_level;
		} else if (sd->status.class <= 4044) { // Baby Second Job
			if (sd->status.job_level >= battle_config.atcommand_max_job_level_babyjob2) {
				clif_displaymessage(fd, "You're not authorized to increase more your job level.");
				return -1;
			} else if (sd->status.job_level + level >= battle_config.atcommand_max_job_level_babyjob2)
				level = battle_config.atcommand_max_job_level_babyjob2 - sd->status.job_level;
		} else if (sd->status.class == 4045) { // Super Baby
			if (sd->status.job_level >= battle_config.atcommand_max_job_level_superbaby) {
				clif_displaymessage(fd, "You're not authorized to increase more your job level.");
				return -1;
			} else if (sd->status.job_level + level >= battle_config.atcommand_max_job_level_superbaby)
				level = battle_config.atcommand_max_job_level_superbaby - sd->status.job_level;
		}

		sd->status.job_level += level;
		clif_updatestatus(sd, SP_JOBLEVEL);
		clif_updatestatus(sd, SP_NEXTJOBEXP);
		sd->status.skill_point += level;
		clif_updatestatus(sd, SP_SKILLPOINT);
		status_calc_pc(sd, 0);
		clif_misceffect(&sd->bl, 1);
		clif_displaymessage(fd, msg_txt(24)); // Job level raised
	} else {
		if (sd->status.job_level == 1) {
			clif_displaymessage(fd, msg_txt(159)); // Job level can't go any lower
			return -1;
		}
		if (level < -up_level || level < (1 - sd->status.job_level)) // Fix negative overflow
			level = 1 - sd->status.job_level;
		// Don't check maximum authorized if we reduce level
		sd->status.job_level += level;
		clif_updatestatus(sd, SP_JOBLEVEL);
		clif_updatestatus(sd, SP_NEXTJOBEXP);
		if (sd->status.skill_point > 0) {
			sd->status.skill_point += level; // Note: Here, level is negative
			if (sd->status.skill_point < 0) {
				level = sd->status.skill_point;
				sd->status.skill_point = 0;
			}
			clif_updatestatus(sd, SP_SKILLPOINT);
		}
		if (level < 0) { // If always negative, skill points must be removed from skills
			// To Add: Remove skill points from skills
		}
		status_calc_pc(sd, 0);
		clif_displaymessage(fd, msg_txt(25)); // Job level lowered
	}

	return 0;
}

/*==========================================
 * Replace all @ from help.txt to command_symbol before display
 *------------------------------------------
 */
void change_arrobas_to_symbol(char* line) {
	char * p;
	char buf[256];
	int i;

	if (command_symbol == '@')
		return;

	p = line;
	while ((p = strchr(p, '@')) != NULL) {
		memcpy(buf, p + 1, 4);
		buf[4] = 0;
		// Search for @char (A lot of GM commands begins with @char)
		if (strcasecmp(buf, "char") == 0) { // strncmpi will be better, but doesn't work under win32 (bcc32 don't recognize strncasecmp)
			*p = command_symbol;
		} else {
			// Search for synonyms commands
			for (i = 0; i < synonym_count; i++) {
				memcpy(buf, p + 1, strlen(synonym_table[i].synonym));
				buf[strlen(synonym_table[i].synonym)] = 0;
				if (strcasecmp(buf, synonym_table[i].synonym) == 0) { // strncmpi will be better, but doesn't work under win32 (bcc32 don't recognize strncasecmp)
					*p = command_symbol;
					break;
				}
			}
			if (i == synonym_count) {
				// Search for normal commands
				for (i = 0; atcommand_info[i].type != AtCommand_Unknown; i++) {
					memcpy(buf, p + 1, strlen(atcommand_info[i].command + 1));
					buf[strlen(atcommand_info[i].command + 1)] = 0;
					if (strcasecmp(buf, atcommand_info[i].command + 1) == 0) { // strncmpi will be better, but doesn't work under win32 (bcc32 don't recognize strncasecmp)
						*p = command_symbol;
						break;
					}
				}
			}
		}
		p++;
	}

	return;
}

/*==========================================
 * @help
 *------------------------------------------
 */
ATCOMMAND_FUNC(help) {
	char buf[256], w1[256], w2[256];
	char cmd_line[1000][256];
	char key[256];
	struct {
		char key_name[256];
		int key_line;
	} keys[200];
	int i, j, k, counter;
	int counter_keys, founded_key;
	int page, first_line, last_line;
	char *p;
	FILE* fp;

	memset(buf, 0, sizeof(buf));
	memset(w1, 0, sizeof(w1));
	memset(w2, 0, sizeof(w2));
	memset(cmd_line, 0, sizeof(cmd_line));
	memset(key, 0, sizeof(key));
	memset(keys, 0, sizeof(keys));

	if ((fp = fopen(help_txt, "r")) == NULL) {
		// If not found, try default help file name */
		if ((fp = fopen("conf/help.txt", "r")) == NULL) {
			clif_displaymessage(fd, msg_txt(27)); // File 'help.txt' not found.
			return -1;
		}
	}

	if ((page = atoi(message)) < 0) {
		page = 0;
		strcpy(key, "0");
	} else {
		for (i = 0; message[i]; i++)
			key[i] = tolower((unsigned char)message[i]); // tolower needs unsigned char
	}

	counter = 0;
	counter_keys = 0;
	founded_key = -1;
	for (i = 0; i < (int)(sizeof(keys) / sizeof(keys[0])); i++)
		keys[i].key_line = sizeof(cmd_line) / sizeof(cmd_line[0]); // Init finish to max value
	// Get lines to display
	while(fgets(buf, sizeof(buf), fp) != NULL && counter < (int)(sizeof(cmd_line) / sizeof(cmd_line[0]))) { // fgets reads until maximum one less than size and add '\0' -> so, it's not necessary to add -1
		if (buf[0] == '/' && buf[1] == '/')
			continue;
		for (i = 0; buf[i] != '\0'; i++) {
			if (buf[i] == '\r' || buf[i] == '\n') {
				buf[i] = '\0';
				break;
			}
		}
		memset(w2, 0, sizeof(w2));
		// Help line without GM level
		if (sscanf(buf, "%255[^:]:%255[^\n]", w1, w2) < 2) {
			strcpy(cmd_line[counter], buf);
			counter++;
		} else {
			// for comparison, reduce in lowercase
			for (i = 0; w1[i]; i++)
				w1[i] = tolower((unsigned char)w1[i]); // tolower needs unsigned char
			// Remove space at start
			while (w1[0] == 32)
				for (i = 0; w1[i]; i++)
					w1[i] = w1[i + 1];
			// Check space at last
			while ((i = strlen(w1)) > 0 && w1[i - 1] == 32)
				w1[i - 1] = '\0';
			// Keys saving
			if (strcmp(w1, "key") == 0) {
				if (counter_keys < (int)(sizeof(keys) / sizeof(keys[0]))) {
					for (i = 0; w2[i]; i++)
						w2[i] = tolower((unsigned char)w2[i]); // tolower needs unsigned char
					sprintf(keys[counter_keys].key_name, "%s,", w2); // Add a final ',' to check key+','
					keys[counter_keys].key_line = counter;
					sprintf(w2, "%s,", key);
					if (strstr(keys[counter_keys].key_name, w2) != NULL) // Search without case sensitivity
						founded_key = counter_keys;
					counter_keys++;
				}
			// Help line with GM level
			} else {
/*
// To save in a help file with level of each function and without comments
#include <fcntl.h> // for open, and O_WRONLY | O_CREAT | O_APPEND
#include <unistd.h> // for write and close
static int log_fp;
char tmpstr[256];
log_fp = open("help_save.txt", O_WRONLY | O_CREAT | O_APPEND, 0644);
*/
				// Check for synonym here
				for (i = 0; i < synonym_count; i++) {
					if (strcmp(w1 + 1, synonym_table[i].synonym) == 0) {
						memset(w1 + 1, 0, sizeof(w1) - 1); // Don't change command_symbol (+1)
						strcpy(w1 + 1, synonym_table[i].command);
						break;
					}
				}

				// Search GM command type
				i = 0;
				while (atcommand_info[i].type != AtCommand_Unknown) {
					if (strcmp(w1 + 1, atcommand_info[i].command + 1) == 0) {
/*
if (log_fp != -1) {
	sprintf(tmpstr, "%03d: ", atcommand_info[i].level);
	write(log_fp, tmpstr, strlen(tmpstr));
	write(log_fp, w2, strlen(w2));
	write(log_fp, RETCODE, strlen(RETCODE));
	close(log_fp);
}
*/
						if (sd->GM_level >= atcommand_info[i].level) {
							strcpy(cmd_line[counter], w2);
							counter++;
						}
						break;
					}
					i++;
				}
				// If not found
				if (atcommand_info[i].type == AtCommand_Unknown)
/*
if (log_fp != -1) {
	sprintf(tmpstr, "%03d: ", atoi(w1));
	write(log_fp, tmpstr, strlen(tmpstr));
	write(log_fp, w2, strlen(w2));
	write(log_fp, RETCODE, strlen(RETCODE));
	close(log_fp);
}
*/
					if (sd->GM_level >= atoi(w1)) {
						strcpy(cmd_line[counter], w2);
						counter++;
					}
			}
		}
	}

	fclose(fp);

	// Displaying of lines (pages method, or all pages)
	if (page > 0 || !message || !*message || strcmp(key, "0") == 0) {
		// Calculation of first and last line
		if (page == 0) { // We display all lines
			first_line = 0;
			last_line = counter;
		} else {
			if ((page - 1) * 50 > counter)
				page = (counter / 50) + 1;
			first_line = (page - 1) * 50;
			last_line = first_line + 50;
			if ((counter - 10) < last_line) // Display all rest of GM command if there is less 10 GM command in last page
				last_line = counter;
			if ((counter - 10) < first_line && first_line >= 50) {
				first_line = first_line - 50;
				page--;
			}
		}
		// Displaying of the help
		if (first_line == 0 && last_line == counter)
			clif_displaymessage(fd, msg_txt(26)); // Help commands:
		else {
			sprintf(buf, "----- Help commands (page %d):", page);
			clif_displaymessage(fd, buf);
		}
		for(i = first_line; i < last_line; i++) {
			change_arrobas_to_symbol(cmd_line[i]);
			clif_displaymessage(fd, cmd_line[i]);
		}

	// Displaying of lines (key method)
	} else {
		// If asked key not found or display of all keywords
		if (founded_key == -1 || strncmp(key, "key", 3) == 0) { // 'key' or 'keyword'
			if (strncmp(key, "key", 3) != 0) {
				sprintf(buf, "----- Help commands keys (key: %s) - key not found!", key);
				clif_displaymessage(fd, buf);
			} else {
				sprintf(buf, "----- Help command keys (%s <key_word>):", original_command);
				clif_displaymessage(fd, buf);
			}
			memset(cmd_line, 0, sizeof(cmd_line)); // So save all keywords and not repeat them
			counter = 0;
			for(i = 0; i < counter_keys; i++) {
				p = keys[i].key_name;
				while(sscanf(p, "%255[^,],%n", w2, &j) == 1 && counter < (int)(sizeof(cmd_line) / sizeof(cmd_line[0]))) {
					if (strcmp(w2, "all") != 0) { // All is a special key
						// Check duplicate keywords
						for(k = 0; k < counter; k++)
							if (strcmp(cmd_line[k], w2) == 0)
								break;
						if (k == counter) {
							strcpy(cmd_line[counter], w2);
							counter++;
						}
					}
					p = p + j;
				}
			}
			if (counter == 0)
				clif_displaymessage(fd, "There is no keyword in the help file.");
			else {
				for(i = 0; i < counter; i++) {
					change_arrobas_to_symbol(cmd_line[i]);
					clif_displaymessage(fd, cmd_line[i]);
				}
			}
		// If key found
		} else {
			sprintf(buf, "----- Help commands (key: %s):", key);
			clif_displaymessage(fd, buf);
			strcat(key, ",");
			for(j = 0; j < counter_keys; j++) {
				if (strstr(keys[j].key_name, "all,") != NULL || strstr(keys[j].key_name, key) != NULL) {
					first_line = keys[j].key_line;
					if ((j - 1) < counter_keys) {
						last_line = keys[j+1].key_line;
						if (last_line > counter)
							last_line = counter;
					} else
						last_line = counter;
					for(i = first_line; i < last_line; i++) {
						change_arrobas_to_symbol(cmd_line[i]);
						clif_displaymessage(fd, cmd_line[i]);
					}
					if (last_line == counter)
						break;
				}
			}
		}
	}

	return 0;
}

/*==========================================
 * @gm - Makes you a GM (Password-Protected)
 *------------------------------------------
 */
ATCOMMAND_FUNC(gm) {
	char password[100];

	memset(password, 0, sizeof(password));

	if (!message || !*message || sscanf(message, "%[^\n]", password) < 1) {
		send_usage(sd, "Please, enter a password (usage: %s <password>).", original_command);
		return -1;
	}

	if (sd->GM_level > battle_config.atcommand_max_player_gm_level) { // A GM can not use this function. only a normal player (Become GM is not for GM!)
		clif_displaymessage(fd, msg_txt(50)); // You already have some GM powers.
		return -1;
	} else
		chrif_changegm(sd->status.account_id, password, strlen(password) + 1);

	return 0;
}

/*==========================================
 * @pvpoff - Turns PvP off for current map
 *------------------------------------------
 */
ATCOMMAND_FUNC(pvpoff) {
	struct map_session_data *pl_sd;
	int i;

	if (battle_config.pk_mode) { // Disable command if server is in PK mode
		clif_displaymessage(fd, msg_txt(52)); // This option cannot be used in PK Mode.
		return -1;
	}

	if (map[sd->bl.m].flag.pvp) {
		map[sd->bl.m].flag.pvp = 0;
		clif_send0199(sd->bl.m, 0);
		for (i = 0; i < fd_max; i++) {
			if (session[i] && (pl_sd = session[i]->session_data) && pl_sd->state.auth) {
				if (sd->bl.m == pl_sd->bl.m) {
					clif_pvpset(pl_sd, 0, 0, 2); // Send packet to player to say: Pvp area
					if (pl_sd->pvp_timer != -1) {
						delete_timer(pl_sd->pvp_timer, pc_calc_pvprank_timer);
						pl_sd->pvp_timer = -1;
					}
				}
			}
		}
		clif_displaymessage(fd, msg_txt(31)); // PvP: Off.
	} else {
		clif_displaymessage(fd, msg_txt(160)); // PvP is already Off.
		return -1;
	}

	return 0;
}

/*==========================================
 * @pvpon - Turns PvP on for current map
 *------------------------------------------
 */
ATCOMMAND_FUNC(pvpon) {
	struct map_session_data *pl_sd;
	int i;

	if (battle_config.pk_mode) { // Disable command if server is in PK mode
		clif_displaymessage(fd, msg_txt(52)); // This option cannot be used in PK Mode.
		return -1;
	}

	if (!map[sd->bl.m].flag.pvp) {
		map[sd->bl.m].flag.pvp = 1;
		clif_send0199(sd->bl.m, 1);
		for (i = 0; i < fd_max; i++) {
			if (session[i] && (pl_sd = session[i]->session_data) && pl_sd->state.auth) {
				if (sd->bl.m == pl_sd->bl.m && pl_sd->pvp_timer == -1) {
					pl_sd->pvp_timer = add_timer(gettick_cache + 200, pc_calc_pvprank_timer, pl_sd->bl.id, 0);
					pl_sd->pvp_rank = 0;
					pl_sd->pvp_lastusers = 0;
					pl_sd->pvp_point = 5;
				}
			}
		}
		clif_displaymessage(fd, msg_txt(32)); // PvP: On.
	} else {
		clif_displaymessage(fd, msg_txt(161)); // PvP is already On.
		return -1;
	}

	return 0;
}

/*==========================================
 * @gvgoff - Turns GvG off for current map
 *------------------------------------------
 */
ATCOMMAND_FUNC(gvgoff) {
	if (map[sd->bl.m].flag.gvg) {
		map[sd->bl.m].flag.gvg = 0;
		clif_send0199(sd->bl.m, 0);
		clif_displaymessage(fd, msg_txt(33)); // GvG: Off.
	} else {
		clif_displaymessage(fd, msg_txt(162)); // GvG is already Off.
		return -1;
	}

	return 0;
}

/*==========================================
 * @gvgon - Turns GvG on for current map
 *------------------------------------------
 */
ATCOMMAND_FUNC(gvgon) {
	if (!map[sd->bl.m].flag.gvg) {
		map[sd->bl.m].flag.gvg = 1;
		clif_send0199(sd->bl.m, 3);
		clif_displaymessage(fd, msg_txt(34)); // GvG: On.
	} else {
		clif_displaymessage(fd, msg_txt(163)); // GvG is already On.
		return -1;
	}

	return 0;
}

/*==========================================
 * @style - Changes user's palette/hair styles
 *------------------------------------------
 */
ATCOMMAND_FUNC(style) {
	int hair_style = 0, hair_color = 0, cloth_color = 0;

	if (!message || !*message || sscanf(message, "%d %d %d", &hair_style, &hair_color, &cloth_color) < 1) {
		send_usage(sd, "Please, enter at least a value (usage: %s <hair ID: %d-%d> <hair color: %d-%d> <clothes color: %d-%d>).", original_command,
		           battle_config.min_hair_style, battle_config.max_hair_style, battle_config.min_hair_color, battle_config.max_hair_color, battle_config.min_cloth_color, battle_config.max_cloth_color);
		return -1;
	}

	if (hair_style >= battle_config.min_hair_style && hair_style <= battle_config.max_hair_style &&
		hair_color >= battle_config.min_hair_color && hair_color <= battle_config.max_hair_color &&
		cloth_color >= battle_config.min_cloth_color && cloth_color <= battle_config.max_cloth_color) {
		if (!battle_config.clothes_color_for_assassin &&
		    cloth_color != 0 && sd->status.sex == 1 && (sd->status.class == 12 ||  sd->status.class == 17)) {
			clif_displaymessage(fd, msg_txt(35)); // You can't use this clothes color with this class.
			return -1;
		} else {
			pc_changelook(sd, LOOK_HAIR, hair_style);
			pc_changelook(sd, LOOK_HAIR_COLOR, hair_color);
			pc_changelook(sd, LOOK_CLOTHES_COLOR, cloth_color);
			clif_displaymessage(fd, msg_txt(36)); // Appearence changed.
		}
	} else {
		clif_displaymessage(fd, msg_txt(37)); // An invalid number was specified.
		send_usage(sd, "Please, enter a valid value (usage: %s <hair ID: %d-%d> <hair color: %d-%d> <clothes color: %d-%d>).", original_command,
		           battle_config.min_hair_style, battle_config.max_hair_style, battle_config.min_hair_color, battle_config.max_hair_color, battle_config.min_cloth_color, battle_config.max_cloth_color);
		return -1;
	}

	return 0;
}

/*==========================================
 * @dye - Changes user's palettes
 *------------------------------------------
 */
ATCOMMAND_FUNC(dye) {

	int cloth_color = 0;

	if (!message || !*message || sscanf(message, "%d", &cloth_color) < 1) {
		send_usage(sd, "Please, enter a clothes color (usage: %s <clothes color: %d-%d>).", original_command, battle_config.min_cloth_color, battle_config.max_cloth_color);
		return -1;
	}

	if (cloth_color >= battle_config.min_cloth_color && cloth_color <= battle_config.max_cloth_color) {
		if (!battle_config.clothes_color_for_assassin &&
		    cloth_color != 0 && sd->status.sex == 1 && (sd->status.class == 12 || sd->status.class == 17)) {
			clif_displaymessage(fd, msg_txt(35)); // You can't use this clothes color with this class.
			return -1;
		} else {
			// Dye crash/exploit fix for Illusionary Shadow (Need to finish) [Tsuyuki]
			// if (sd->sc_data[SC_BUNSINJYUTSU].timer != -1)
			//	status_change_end(sd, SC_BUNSINJYUTSU, -1);
			pc_changelook(sd, LOOK_CLOTHES_COLOR, cloth_color);
			clif_displaymessage(fd, msg_txt(36)); // Appearence changed.
		}
	} else {
		clif_displaymessage(fd, msg_txt(37)); // An invalid number was specified.
		send_usage(sd, "Please, enter a valid clothes color (usage: %s <clothes color: %d-%d>).", original_command, battle_config.min_cloth_color, battle_config.max_cloth_color);
		return -1;
	}

	return 0;
}

/*==========================================
 * @chardye - Changes target's palettes
 *------------------------------------------
 */
ATCOMMAND_FUNC(chardye) {
	int cloth_color = 0;
	struct map_session_data *pl_sd;

	if (!message || !*message || sscanf(message, "%d %[^\n]", &cloth_color, atcmd_name) < 2) {
		send_usage(sd, "Please, enter a clothes color and a player name (usage: %s <clothes color: %d-%d> <char name|account_id>).", original_command, battle_config.min_cloth_color, battle_config.max_cloth_color);
		return -1;
	}

	if ((pl_sd = map_nick2sd(atcmd_name)) != NULL || ((pl_sd = map_id2sd(atoi(atcmd_name))) != NULL && pl_sd->state.auth)) {
		if (sd->GM_level >= pl_sd->GM_level) { // Only lower or same level
			if (cloth_color >= battle_config.min_cloth_color && cloth_color <= battle_config.max_cloth_color) {
				if (!battle_config.clothes_color_for_assassin &&
				    cloth_color != 0 && pl_sd->status.sex == 1 && (pl_sd->status.class == 12 || pl_sd->status.class == 17)) {
					clif_displaymessage(fd, "You can't use this clothes color with the class of this player.");
					return -1;
				} else {
			// Dye crash/exploit fix for Illusionary Shadow (Need to finish) [Tsuyuki]
			//		if (pl_sd->sc_data[SC_BUNSINJYUTSU].timer != -1)
			//			status_change_end(&pl_sd->src, SC_BUNSINJYUTSU, -1);
					pc_changelook(pl_sd, LOOK_CLOTHES_COLOR, cloth_color);
					clif_displaymessage(fd, "Player's appearence changed.");
				}
			} else {
				clif_displaymessage(fd, msg_txt(37)); // An invalid number was specified.
				send_usage(sd, "Please, enter a valid clothes color (usage: %s <clothes color: %d-%d> <char name|account_id>).", original_command, battle_config.min_cloth_color, battle_config.max_cloth_color);
				return -1;
			}
		} else {
			clif_displaymessage(fd, msg_txt(81)); // Your GM level don't authorize you to do this action on this player.
			return -1;
		}
	} else {
		clif_displaymessage(fd, msg_txt(3)); // Character not found.
		return -1;
	}

	return 0;
}

/*==========================================
 * @hairstyle - Changes user's hairstyle
 *------------------------------------------
 */
ATCOMMAND_FUNC(hair_style) {
	int hair_style = 0;

	if (!message || !*message || sscanf(message, "%d", &hair_style) < 1) {
		send_usage(sd, "Please, enter a hair style (usage: %s <hair ID: %d-%d>).", original_command, battle_config.min_hair_style, battle_config.max_hair_style);
		return -1;
	}

	if (hair_style >= battle_config.min_hair_style && hair_style <= battle_config.max_hair_style) {
		pc_changelook(sd, LOOK_HAIR, hair_style);
		clif_displaymessage(fd, msg_txt(36)); // Appearence changed.
	} else {
		clif_displaymessage(fd, msg_txt(37)); // An invalid number was specified.
		send_usage(sd, "Please, enter a valid hair style (usage: %s <hair ID: %d-%d>).", original_command, battle_config.min_hair_style, battle_config.max_hair_style);
		return -1;
	}

	return 0;
}

/*==========================================
 * @charhairstyle - Changes target's hairstyle
 *------------------------------------------
 */
ATCOMMAND_FUNC(charhairstyle) {
	int hair_style = 0;
	struct map_session_data *pl_sd;

	if (!message || !*message || sscanf(message, "%d %[^\n]", &hair_style, atcmd_name) < 2) {
		send_usage(sd, "Please, enter a hair style and a player name (usage: %s <hair ID: %d-%d> <char name|account_id>).", original_command, battle_config.min_hair_style, battle_config.max_hair_style);
		return -1;
	}

	if ((pl_sd = map_nick2sd(atcmd_name)) != NULL || ((pl_sd = map_id2sd(atoi(atcmd_name))) != NULL && pl_sd->state.auth)) {
		if (sd->GM_level >= pl_sd->GM_level) { // Only lower or same level
			if (hair_style >= battle_config.min_hair_style && hair_style <= battle_config.max_hair_style) {
				pc_changelook(pl_sd, LOOK_HAIR, hair_style);
				clif_displaymessage(fd, "Player's appearence changed.");
			} else {
				clif_displaymessage(fd, msg_txt(37)); // An invalid number was specified.
				send_usage(sd, "Please, enter a valid hair style and a player name (usage: %s <hair ID: %d-%d> <char name|account_id>).", original_command, battle_config.min_hair_style, battle_config.max_hair_style);
				return -1;
			}
		} else {
			clif_displaymessage(fd, msg_txt(81)); // Your GM level don't authorize you to do this action on this player.
			return -1;
		}
	} else {
		clif_displaymessage(fd, msg_txt(3)); // Character not found.
		return -1;
	}

	return 0;
}

/*==========================================
 * @haircolor - Changes user's hair color
 *------------------------------------------
 */
ATCOMMAND_FUNC(hair_color) {
	int hair_color = 0;

	if (!message || !*message || sscanf(message, "%d", &hair_color) < 1) {
		send_usage(sd, "Please, enter a hair color (usage: %s <hair color: %d-%d>).", original_command, battle_config.min_hair_color, battle_config.max_hair_color);
		return -1;
	}

	if (hair_color >= battle_config.min_hair_color && hair_color <= battle_config.max_hair_color) {
		pc_changelook(sd, LOOK_HAIR_COLOR, hair_color);
		clif_displaymessage(fd, msg_txt(36)); // Appearence changed.
	} else {
		clif_displaymessage(fd, msg_txt(37)); // An invalid number was specified.
		send_usage(sd, "Please, enter a valid hair color (usage: %s <hair color: %d-%d>).", original_command, battle_config.min_hair_color, battle_config.max_hair_color);
		return -1;
	}

	return 0;
}

/*==========================================
 * @charhaircolor - Changes target's hair color
 *------------------------------------------
 */
ATCOMMAND_FUNC(charhaircolor) {
	int hair_color = 0;
	struct map_session_data *pl_sd;

	if (!message || !*message || sscanf(message, "%d %[^\n]", &hair_color, atcmd_name) < 2) {
		send_usage(sd, "Please, enter a hair color and a player name (usage: %s <hair color: %d-%d> <char name|account_id>).", original_command, battle_config.min_hair_color, battle_config.max_hair_color);
		return -1;
	}

	if ((pl_sd = map_nick2sd(atcmd_name)) != NULL || ((pl_sd = map_id2sd(atoi(atcmd_name))) != NULL && pl_sd->state.auth)) {
		if (sd->GM_level >= pl_sd->GM_level) { // Only lower or same level
			if (hair_color >= battle_config.min_hair_color && hair_color <= battle_config.max_hair_color) {
				pc_changelook(pl_sd, LOOK_HAIR_COLOR, hair_color);
				clif_displaymessage(fd, "Player's appearence changed.");
			} else {
				clif_displaymessage(fd, msg_txt(37)); // An invalid number was specified.
				send_usage(sd, "Please, enter a valid hair color and a player name (usage: %s <hair color: %d-%d> <char name|account_id>).", original_command, battle_config.min_hair_color, battle_config.max_hair_color);
				return -1;
			}
		} else {
			clif_displaymessage(fd, msg_txt(81)); // Your GM level don't authorize you to do this action on this player.
			return -1;
		}
	} else {
		clif_displaymessage(fd, msg_txt(3)); // Character not found.
		return -1;
	}

	return 0;
}

/*==========================================
 * @go [city_number/city_name]
 *------------------------------------------
 */
ATCOMMAND_FUNC(go) {
	int i;
	int town;
	int m;

	const struct { char map[17];  int x,   y; } data[] = { // 16 + NULL
	             { "prontera.gat",    157, 191 },	//	 0=Prontera
	             { "morocc.gat",      156,  93 },	//	 1=Morroc
	             { "geffen.gat",      119,  59 },	//	 2=Geffen
	             { "payon.gat",       162, 233 },	//	 3=Payon
	             { "alberta.gat",     192, 147 },	//	 4=Alberta
	             { "izlude.gat",      128, 114 },	//	 5=Izlude
	             { "aldebaran.gat",   140, 131 },	//	 6=Al de Baran
	             { "xmas.gat",        147, 134 },	//	 7=Lutie
	             { "comodo.gat",      209, 143 },	//	 8=Comodo
	             { "yuno.gat",        157,  51 },	//	 9=Yuno
	             { "amatsu.gat",      198,  84 },	//	10=Amatsu
	             { "gonryun.gat",     160, 120 },	//	11=Kunlun
	             { "umbala.gat",       89, 157 },	//	12=Umbala
	             { "niflheim.gat",     21, 153 },	//	13=Niflheim
	             { "louyang.gat",     217,  40 },	//	14=Louyang
	             { "new_1-1.gat",      53, 111 },	//	15=Novice Grounds (Starting Point)
	             { "sec_pri.gat",      23,  61 },	//	16=Jail
	             { "valkyrie.gat",     49,  48 },	//	17=Valkyrie Realm
	             { "pay_arche.gat",    56, 132 },	//	18=Archer's Village
	             { "jawaii.gat",      249, 127 },	//	19=Jawaii
	             { "ayothaya.gat",    151, 117 },	//	20=Ayothaya
	             { "einbroch.gat",     64, 200 },	//	21=Einbroch
	             { "lighthalzen.gat", 159,  93 },	//	22=Lighthalzen
               { "hugel.gat",        96, 145 },	//	23=Hugel
               { "rachel.gat",      130, 116 },	//	24=Rachel
               { "veins.gat",				181,	84 }, //	25=Veins
	};

	if (map[sd->bl.m].flag.nogo && battle_config.any_warp_GM_min_level > sd->GM_level) {
		sprintf(atcmd_output, "You can not use %s on this map.", original_command);
		clif_displaymessage(fd, atcmd_output);
		return -1;
	}

	// Get the number
	town = atoi(message);

	// If value is invalid, display all values
	if (!message || !*message || sscanf(message, "%s", atcmd_mapname) < 1 || town < -3 || town >= (int)(sizeof(data) / sizeof(data[0]))) {
		send_usage(sd, msg_txt(38)); // Invalid location number or name.
		send_usage(sd, msg_txt(82)); // Please, use one of this numbers/names:
		send_usage(sd, "-3=(Memo Point 2)   7=Lutie           17=Valkyrie Realm");
		send_usage(sd, "-2=(Memo Point 1)   8=Comodo          18=Archer's Village");
		send_usage(sd, "-1=(Memo Point 0)   9=Juno            19=Jawaii");
		send_usage(sd, " 0=Prontera        10=Amatsu          20=Ayothaya");
		send_usage(sd, " 1=Morroc          11=Kunlun      	  21=Einbroch");
		send_usage(sd, " 2=Geffen          12=Umbala          22=Lighthalzen");
		send_usage(sd, " 3=Payon           13=Niflheim        23=Hugel");
		send_usage(sd, " 4=Alberta         14=Louyang      	  24=Rachel");
		send_usage(sd, " 5=Izlude          15=Start Point     25=Veins");
		send_usage(sd, " 6=Al de Baran     16=Jail");
		return -1;
	} else {
		// Get possible name of the city and add .gat if not in the name
		for (i = 0; atcmd_mapname[i]; i++)
			atcmd_mapname[i] = tolower((unsigned char)atcmd_mapname[i]); // tolower needs unsigned char
		if (strstr(atcmd_mapname, ".gat") == NULL && strlen(atcmd_mapname) < 13) // 16 - 4 (.gat)
			strcat(atcmd_mapname, ".gat");
		// Try to see if it's a name, and not a number (try a lot of possibilities, write errors and abbreviations too)
		if (strncmp(atcmd_mapname, "prontera.gat", 3) == 0) // 3 first characters
			town = 0;
		else if (strncmp(atcmd_mapname, "morocc.gat", 3) == 0)
			town = 1;
		else if (strncmp(atcmd_mapname, "geffen.gat", 3) == 0) // 3 first characters
			town = 2;
		else if (strncmp(atcmd_mapname, "payon.gat", 3) == 0 || // 3 first characters
		           strncmp(atcmd_mapname, "paion.gat", 3) == 0) // Writing error (3 first characters)
			town = 3;
		else if (strncmp(atcmd_mapname, "alberta.gat", 3) == 0) // 3 first characters
			town = 4;
		else if (strncmp(atcmd_mapname, "izlude.gat", 3) == 0 || // 3 first characters
		           strncmp(atcmd_mapname, "islude.gat", 3) == 0) // Writing error (3 first characters)
			town = 5;
		else if (strncmp(atcmd_mapname, "aldebaran.gat", 3) == 0 || // 3 first characters
		           strcmp(atcmd_mapname,  "al.gat") == 0) // al (de baran)
			town = 6;
		else if (strncmp(atcmd_mapname, "lutie.gat", 3) == 0 || // Name of the city, not name of the map (3 first characters)
		           strcmp(atcmd_mapname,  "christmas.gat") == 0 || // Name of the symbol
		           strncmp(atcmd_mapname, "xmas.gat", 3) == 0 || // 3 first characters
		           strncmp(atcmd_mapname, "x-mas.gat", 3) == 0) // Writing error (3 first characters)
			town = 7;
		else if (strncmp(atcmd_mapname, "comodo.gat", 3) == 0) // 3 first characters
			town = 8;
		else if (strncmp(atcmd_mapname, "yuno.gat", 3) == 0) // 3 first characters
			town = 9;
		else if (strncmp(atcmd_mapname, "amatsu.gat", 3) == 0 || // 3 first characters
		           strncmp(atcmd_mapname, "ammatsu.gat", 3) == 0) // Writing error (3 first characters)
			town = 10;
		else if (strncmp(atcmd_mapname, "gonryun.gat", 3) == 0) // 3 first characters
			town = 11;
		else if (strncmp(atcmd_mapname, "umbala.gat", 3) == 0) // 3 first characters
			town = 12;
		else if (strncmp(atcmd_mapname, "niflheim.gat", 3) == 0) // 3 first characters
			town = 13;
		else if (strncmp(atcmd_mapname, "louyang.gat", 3) == 0) // 3 first characters
			town = 14;
		else if (strncmp(atcmd_mapname, "new_1-1.gat", 3) == 0 || // 3 first characters (or "newbies")
		           strncmp(atcmd_mapname, "startpoint.gat", 3) == 0 || // Name of the position (3 first characters)
		           strncmp(atcmd_mapname, "begining.gat", 3) == 0) // Name of the position (3 first characters)
			town = 15;
		else if (strncmp(atcmd_mapname, "sec_pri.gat", 3) == 0 || // 3 first characters
		           strncmp(atcmd_mapname, "prison.gat", 3) == 0 || // Name of the position (3 first characters)
		           strncmp(atcmd_mapname, "jails.gat", 3) == 0) // Name of the position
			town = 16;
		else if (strncmp(atcmd_mapname, "valkyrie.gat", 3) == 0) // 3 first characters
			town = 17;
		else if (strncmp(atcmd_mapname, "pay_arche.gat", 4) == 0 || // 4 first characters
		           strncmp(atcmd_mapname, "village.gat", 3) == 0 || // Name of the position (3 first characters)
		           strncmp(atcmd_mapname, "archer.gat", 3) == 0) // Name of the position (3 first characters)
			town = 18;
		else if (strncmp(atcmd_mapname, "jawaii.gat", 3) == 0) // 3 first characters
			town = 19;
		else if (strncmp(atcmd_mapname, "ayothaya.gat", 3) == 0) // 3 first characters
			town = 20;
		else if (strncmp(atcmd_mapname, "einbroch.gat", 3) == 0) // 3 first characters
			town = 21;
		else if (strncmp(atcmd_mapname, "lighthalzen.gat", 3) == 0) // 3 first characters
			town = 22;
		else if (strncmp(atcmd_mapname, "hugel.gat", 3) == 0) // 3 first characters
			town = 23;
		else if (strncmp(atcmd_mapname, "rachel.gat", 3) == 0 || // Name of the position (3 first characters)
		           strncmp(atcmd_mapname, "rachael.gat", 3) == 0) // Name of the position (3 first characters)
			town = 24;
		else if (strncmp(atcmd_mapname, "veins.gat", 3) == 0) // 3 first characters
			town = 25;
		else if (sscanf(message, "%d", &i) < 1) { // Not a number
			send_usage(sd, msg_txt(38)); // Invalid location number or name.
			send_usage(sd, msg_txt(82)); // Please, use one of this numbers/names:
			send_usage(sd, "-3=(Memo Point 2)   7=Lutie           17=Valkyrie Realm");
			send_usage(sd, "-2=(Memo Point 1)   8=Comodo          18=Archer's Village");
			send_usage(sd, "-1=(Memo Point 0)   9=Juno            19=Jawaii");
			send_usage(sd, " 0=Prontera        10=Amatsu          20=Ayothaya");
			send_usage(sd, " 1=Morroc          11=Kunlun      	  21=Einbroch");
			send_usage(sd, " 2=Geffen          12=Umbala          22=Lighthalzen");
			send_usage(sd, " 3=Payon           13=Niflheim        23=Hugel");
			send_usage(sd, " 4=Alberta         14=Louyang      	  24=Rachel");
			send_usage(sd, " 5=Izlude          15=Start Point     25=Veins");
			send_usage(sd, " 6=Al de Baran     16=Jail");
			return -1;
		}

		if (town >= -3 && town <= -1) { // MAX_PORTAL_MEMO
			if (sd->status.memo_point[-town-1].map[0]) {
				if ((m = map_checkmapname(sd->status.memo_point[-town-1].map)) == -1) { // If map doesn't exist in all map-servers
					clif_displaymessage(fd, msg_txt(1)); // Map not found.
					return -1;
				}
				if (m >= 0) { // If on this map-server
					if (map[m].flag.nowarpto && battle_config.any_warp_GM_min_level > sd->GM_level) {
						clif_displaymessage(fd, "You are not authorized to warp you to this memo map.");
						return -1;
					}
				}
				if (sd->bl.m >= 0 && map[sd->bl.m].flag.nowarp && battle_config.any_warp_GM_min_level > sd->GM_level) {
					clif_displaymessage(fd, "You are not authorized to warp you from your actual map.");
					return -1;
				}
				if (pc_setpos(sd, sd->status.memo_point[-town-1].map, sd->status.memo_point[-town-1].x, sd->status.memo_point[-town-1].y, 3, 0) == 0) {
					clif_displaymessage(fd, msg_txt(0)); // Warped.
				} else {
					clif_displaymessage(fd, msg_txt(1)); // Map not found.
					return -1;
				}
			} else {
				sprintf(atcmd_output, msg_txt(164), -town-1); // Your memo point #%d doesn't exist.
				clif_displaymessage(fd, atcmd_output);
				return -1;
			}
		} else if (town >= 0 && town < (int)(sizeof(data) / sizeof(data[0]))) {
			if ((m = map_checkmapname((char*)data[town].map)) == -1) { // If map doesn't exist in all map-servers
				clif_displaymessage(fd, msg_txt(1)); // Map not found.
				return -1;
			}
			if (m >= 0) { // If on this map-server
				if (map[m].flag.nowarpto && battle_config.any_warp_GM_min_level > sd->GM_level) {
					clif_displaymessage(fd, "You are not authorized to warp you to this destination map.");
					return -1;
				}
			}
			if (sd->bl.m >= 0 && map[sd->bl.m].flag.nowarp && battle_config.any_warp_GM_min_level > sd->GM_level) {
				clif_displaymessage(fd, "You are not authorized to warp you from your actual map.");
				return -1;
			}
			if (pc_setpos(sd, (char*)data[town].map, data[town].x, data[town].y, 3, 0) == 0) {
				clif_displaymessage(fd, msg_txt(0)); // Warped.
			} else {
				clif_displaymessage(fd, msg_txt(1)); // Map not found.
				return -1;
			}
		} else { // If you arrive here, you have an error in town variable when reading of names
			clif_displaymessage(fd, msg_txt(38)); // Invalid location number or name.
			return -1;
		}
	}

	return 0;
}

/*==========================================
 * @go2 Dungeon Warp Spots
 *------------------------------------------
*/
ATCOMMAND_FUNC(go2) {
	int i;
	int dungeon;
	int m;

	const struct { char map[17];  int x,   y; } data[] = { // 16 + NULL
	             { "hu_fild05.gat",     192, 207 },	//	 0=Abyss Lake
	             { "ama_in02.gat",      213, 161 },	//	 1=Amatsu Dungeon
	             { "moc_fild15.gat",    243, 246 },	//	 2=Ant Hell
	             { "ayo_fild02.gat",    273, 149 },	//	 3=Ayothaya Dungeon
	             { "izlu2dun.gat",      106,  88 },	//	 4=Byalan Dungeon
	             { "prt_fild05.gat",    273, 210 },	//	 5=Culvert Sewers
	             { "mjolnir_02.gat",     81, 359 },	//	 6=Coal Mine
	             { "einbech.gat",       138, 247 },	//	 7=Einbroch Dungeon
	             { "prt_fild01.gat",    136, 360 },	//	 8=Hidden Temple
	             { "glast_01.gat",      368, 303 },	//	 9=Glast Heim
	             { "gonryun.gat",       160, 195 },	//	10=Kunlun Dungeon
	             { "ra_fild01.gat",     234, 327 },	//	11=Ice Dungeon
	             { "yuno_fild07.gat",   219, 176 },	//	12=Juperos Ruins
	             { "yuno_fild08.gat",    69, 170 },	//	13=Kiel Hyre
	             { "yuno_fild03.gat",    39, 140 },	//	14=Magma Dungeon
	             { "lighthalzen.gat",   338, 229 },	//	15=Biolabs
	             { "gef_fild10.gat",     70, 332 },	//	16=Orc Dungeon
	             { "pay_arche.gat",      43, 132 },	//	17=Payon Dungeon
	             { "moc_ruins.gat",      62, 162 },	//	18=Pyramids
	             { "moc_fild19.gat",    107, 100 },	//	19=Sphinx
	             { "alb2trea.gat",       75,  98 },	//	20=Sunken Ship
	             { "hu_fild01.gat",     139, 152 },	//	21=Thanatos Tower
	             { "ve_fild03.gat",     168, 234 },	//	22=Thor Dungeon
               { "tur_dun01.gat",     149, 238 },	//	23=Turtle Dungeon
	};

	if (map[sd->bl.m].flag.nogo && battle_config.any_warp_GM_min_level > sd->GM_level) {
		sprintf(atcmd_output, "You can not use %s on this map.", original_command);
		clif_displaymessage(fd, atcmd_output);
		return -1;
	}

	// Get the number
	dungeon = atoi(message);

	// If no value, display all values
	if (!message || !*message || sscanf(message, "%s", atcmd_mapname) < 1 || dungeon < 0 || dungeon >= (int)(sizeof(data) / sizeof(data[0]))) {
		send_usage(sd, msg_txt(38)); // Invalid location number or name.
		send_usage(sd, msg_txt(82)); // Please, use one of this numbers/names:
		send_usage(sd, " 0=Abyss Lake         10=Kunlun Dungeon 20=Sunken Ship");
		send_usage(sd, " 1=Amatsu Dungeon     11=Ice Dungeon    21=Thanatos Tower");
		send_usage(sd, " 2=Ant Hell           12=Juperos Ruins  22=Thor Dungeon");
		send_usage(sd, " 3=Ayothaya Dungeon   13=Kiel Hyre      23=Turtle Dungeon");
		send_usage(sd, " 4=Byalan Dungeon     14=Magma Dungeon");
		send_usage(sd, " 5=Culvert Sewers     15=Biolabs");
		send_usage(sd, " 6=Coal Mine          16=Orc Dungeon");
		send_usage(sd, " 7=Einbroch Dungeon   17=Payon Dungeon");
		send_usage(sd, " 8=Hidden Temple      18=Pyramids");
		send_usage(sd, " 9=Glast Heim         19=Sphinx");
		return -1;
	} else {
		// Get possible name of the dungeon and add .gat if not in the name
		for (i = 0; atcmd_mapname[i]; i++)
			atcmd_mapname[i] = tolower((unsigned char)atcmd_mapname[i]); // To lower needs unsigned char
		if (strstr(atcmd_mapname, ".gat") == NULL && strlen(atcmd_mapname) < 13) // 16 - 4 (.gat)
			strcat(atcmd_mapname, ".gat");
		// Try to see if it's a name, and not a number (try a lot of possibilities, write errors and abbreviations too)
		if (strncmp(atcmd_mapname, "hu_fild05.gat", 3) == 0) // 3 first characters
			dungeon = 0;
		else if (strncmp(atcmd_mapname, "ama_in02.gat", 3) == 0)
			dungeon = 1;
		else if (strncmp(atcmd_mapname, "moc_fild15.gat", 3) == 0) // 3 first characters
			dungeon = 2;
		else if (strncmp(atcmd_mapname, "ayo_fild02.gat", 3) == 0) // 3 first characters
			dungeon = 3;
		else if (strncmp(atcmd_mapname, "izlu2dun.gat", 3) == 0) // 3 first characters
			dungeon = 4;
		else if (strncmp(atcmd_mapname, "prt_fild05.gat", 3) == 0) // 3 first characters
			dungeon = 5;
		else if (strncmp(atcmd_mapname, "mjolnir_02.gat", 3) == 0) // 3 first characters
			dungeon = 6;
		else if (strncmp(atcmd_mapname, "einbech.gat", 3) == 0) // 3 first characters
			dungeon = 7;
		else if (strncmp(atcmd_mapname, "prt_fild01.gat", 3) == 0) // 3 first characters
			dungeon = 8;
		else if (strncmp(atcmd_mapname, "glast_01.gat", 3) == 0) // 3 first characters
			dungeon = 9;
		else if (strncmp(atcmd_mapname, "gonryun.gat", 3) == 0) // 3 first characters
			dungeon = 10;
		else if (strncmp(atcmd_mapname, "ra_fild01.gat", 3) == 0) // 3 first characters
			dungeon = 11;
		else if (strncmp(atcmd_mapname, "yuno_fild07.gat", 3) == 0) // 3 first characters
			dungeon = 12;
		else if (strncmp(atcmd_mapname, "yuno_fild08.gat", 3) == 0) // 3 first characters
			dungeon = 13;
		else if (strncmp(atcmd_mapname, "yuno_fild03.gat", 3) == 0) // 3 first characters
			dungeon = 14;
		else if (strncmp(atcmd_mapname, "lighthalzen.gat", 3) == 0) // 3 first characters
			dungeon = 15;
		else if (strncmp(atcmd_mapname, "gef_fild10.gat", 3) == 0) // 3 first characters
			dungeon = 16;
		else if (strncmp(atcmd_mapname, "pay_arche.gat", 3) == 0) // 3 first characters
			dungeon = 17;
		else if (strncmp(atcmd_mapname, "moc_ruins.gat", 4) == 0) // 4 first characters
			dungeon = 18;
		else if (strncmp(atcmd_mapname, "moc_fild19.gat", 3) == 0) // 3 first characters
			dungeon = 19;
		else if (strncmp(atcmd_mapname, "alb2trea.gat", 3) == 0) // 3 first characters
			dungeon = 20;
		else if (strncmp(atcmd_mapname, "hu_fild01.gat", 3) == 0) // 3 first characters
			dungeon = 21;
		else if (strncmp(atcmd_mapname, "ve_fild03.gat", 3) == 0) // 3 first characters
			dungeon = 22;
		else if (strncmp(atcmd_mapname, "tur_dun01.gat", 3) == 0) // 3 first characters
			dungeon = 23;
		else if (sscanf(message, "%d", &i) < 1) { // Not a number
			send_usage(sd, msg_txt(38)); // Invalid location number or name
			send_usage(sd, msg_txt(82)); // Please, use one of these numbers/names:
			send_usage(sd, " 0=Abyss Lake         10=Kunlun Dungeon 20=Sunken Ship");
			send_usage(sd, " 1=Amatsu Dungeon     11=Ice Dungeon    21=Thanatos Tower");
			send_usage(sd, " 2=Ant Hell           12=Juperos Ruins  22=Thor Dungeon");
			send_usage(sd, " 3=Ayothaya Dungeon   13=Kiel Hyre      23=Turtle Dungeon");
			send_usage(sd, " 4=Byalan Dungeon     14=Magma Dungeon");
			send_usage(sd, " 5=Culvert Sewers     15=Biolabs");
			send_usage(sd, " 6=Coal Mine          16=Orc Dungeon");
			send_usage(sd, " 7=Einbroch Dungeon   17=Payon Dungeon");
			send_usage(sd, " 8=Hidden Temple      18=Pyramids");
			send_usage(sd, " 9=Glast Heim         19=Sphinx");
			return -1;
		}

	 if (dungeon >= 0 && dungeon < (int)(sizeof(data) / sizeof(data[0]))) {
			if ((m = map_checkmapname((char*)data[dungeon].map)) == -1) { // If map doesn't exist in all map-servers
				clif_displaymessage(fd, msg_txt(1)); // Map not found
				return -1;
			}
			if (m >= 0) { // If on this map-server
				if (map[m].flag.nowarpto && battle_config.any_warp_GM_min_level > sd->GM_level) {
					clif_displaymessage(fd, "You are not authorized to warp you to this destination map.");
					return -1;
				}
			}
			if (sd->bl.m >= 0 && map[sd->bl.m].flag.nowarp && battle_config.any_warp_GM_min_level > sd->GM_level) {
				clif_displaymessage(fd, "You are not authorized to warp you from your actual map.");
				return -1;
			}
			if (pc_setpos(sd, (char*)data[dungeon].map, data[dungeon].x, data[dungeon].y, 3, 0) == 0) {
				clif_displaymessage(fd, msg_txt(0)); // Warped
			} else {
				clif_displaymessage(fd, msg_txt(1)); // Map not found
				return -1;
			}
		} else { // If you arrive here, you have an error in dungeon variable when reading of names
			clif_displaymessage(fd, msg_txt(38)); // Invalid location number or name
			return -1;
		}
	}

	return 0;
}

/*==========================================
 * Function to check if a mob is not forbidden for spawn
 * return 0: Not authorized
 * return not 0: Authorized
 *------------------------------------------
 */
int check_mob_authorization(const int mob_id, const unsigned char gm_level) {
	FILE *fp;
	char line[512];
	int id, level;
	const char *filename = "db/mob_deny.txt";

	if ((fp = fopen(filename, "r")) == NULL) {
		// printf("Mob deny file not found: %s.\n", filename); // not display every time
		return 1; // By default: Not deny -> Authorized
	}

	while(fgets(line, sizeof(line), fp)) { // fgets reads until maximum one less than size and add '\0' -> So, it's not necessary to add -1
		if ((line[0] == '/' && line[1] == '/') || line[0] == '\0' || line[0] == '\n' || line[0] == '\r')
			continue;
		// It's not necessary to remove 'carriage return ('\n' or '\r')
		level = 100; // Default: If no mentioned GM level, no spawn -> 100
		if (sscanf(line, "%d,%d", &id, &level) < 1)
			continue;
		if (id == mob_id) {
			fclose(fp);
			if (gm_level < level)
				return 0; // Not authorized
			else
				return 1; // Authorized
		}
	}
	fclose(fp);

	return 1; // Autorized
}

/*==========================================
 * Calculation of number of monsters that the player can look aroung of him
 *------------------------------------------
 */
int quantity_visible_monster(struct map_session_data* sd) {
	int m, bx, by;
	int mob_num;
	struct block_list *bl;

	mob_num = 0;
	m = sd->bl.m;
	for(by = (sd->bl.y - AREA_SIZE) / BLOCK_SIZE - 1; by <= (sd->bl.y + AREA_SIZE) / BLOCK_SIZE + 1; by++) {
		if (by < 0)
			continue;
		else if (by >= map[m].bys)
			break;
		for(bx = (sd->bl.x - AREA_SIZE) / BLOCK_SIZE - 1; bx <= (sd->bl.x + AREA_SIZE) / BLOCK_SIZE + 1; bx++) {
			if (bx < 0)
				continue;
			else if (bx >= map[m].bxs)
				break;
			for (bl = map[m].block_mob[bx + by * map[m].bxs]; bl; bl = bl->next)
				mob_num++;
		}
	}

	return mob_num;
}

/*==========================================
 * @spawn - Spawns monsters
 *------------------------------------------
 */
ATCOMMAND_FUNC(spawn) {
	char name[100];
	char monster[100];
	int mob_id;
	int number = 0;
	int limited_number; // For correct display
	int x = 0, y = 0;
	int count;
	int c, i, j, id;
	int mx, my, range;
	struct mob_data *md;
	struct block_list *bl;
	int b, mob_num, slave_num;
	int size, agro;

	memset(name, 0, sizeof(name));
	memset(monster, 0, sizeof(monster));

	if (!message || !*message ||
	    (sscanf(message, "\"%[^\"]\" %s %d %d %d", name, monster, &number, &x, &y) < 2 &&
	     sscanf(message, "%s \"%[^\"]\" %d %d %d", monster, name, &number, &x, &y) < 2 &&
	     sscanf(message, "%s %d \"%[^\"]\" %d %d", monster, &number, name, &x, &y) < 3 &&
	     sscanf(message, "%s %d %s %d %d", monster, &number, name, &x, &y) < 1)) {
		clif_displaymessage(fd, msg_txt(143)); // Give a monster name/id please.
		return -1;
	}

	// If monster identifier/name argument is a name
	if ((mob_id = mobdb_searchname(monster)) == 0) // Check name first (To avoid possible name begining by a number)
		mob_id = mobdb_checkid(atoi(monster));

	if (mob_id == 0) {
		clif_displaymessage(fd, msg_txt(40)); // Invalid monster ID or name.
		return -1;
	}

	if (mob_id == 1288) {
		clif_displaymessage(fd, msg_txt(83)); // Cannot spawn emperium.
		return -1;
	}

	if (!check_mob_authorization(mob_id, sd->GM_level)) {
		clif_displaymessage(fd, "You are not authorized to spawn this monster.");
		return -1;
	}

	if (number <= 0)
		number = 1;

	if (name[0] == '\0')
		strcpy(name, "--ja--");

	// If value of atcommand_spawn_quantity_limit directive is greater than or equal to 1 and quantity of monsters is greater than value of the directive
	limited_number = number;
	if (battle_config.atc_spawn_quantity_limit > 0 && number > battle_config.atc_spawn_quantity_limit)
		limited_number = battle_config.atc_spawn_quantity_limit;

	if (battle_config.atc_map_mob_limit > 0) {
		mob_num = 0;
		for (b = 0; b < map[sd->bl.m].bxs * map[sd->bl.m].bys; b++)
			for (bl = map[sd->bl.m].block_mob[b]; bl; bl = bl->next)
				mob_num++;
		if (mob_num >= battle_config.atc_map_mob_limit) {
			clif_displaymessage(fd, "There is too many monsters on the map. You can not spawn more monsters.");
			return -1;
		} else if (mob_num + limited_number > battle_config.atc_map_mob_limit) {
			sprintf(atcmd_output, "Due to a density of monsters on the map, spawn has been limited.");
			clif_displaymessage(fd, atcmd_output);
			limited_number = battle_config.atc_map_mob_limit - mob_num;
		}
	}

	if ((count = quantity_visible_monster(sd)) >= 2000) {
		clif_displaymessage(fd, "There is too many monsters around of you. Move to a sparsely populated area to spawn.");
		return -1;
	} else if (count + limited_number > 2000) {
		sprintf(atcmd_output, "Due to a density of monsters around of you, spawn has been limited.");
		clif_displaymessage(fd, atcmd_output);
		limited_number = 2000 - count;
	}

	if (battle_config.etc_log)
		printf("%s monster='%s' name='%s' id=%d count=%d (%d,%d)\n", command, monster, name, mob_id, limited_number, x, y);

	// Check latest spawn time
	if (battle_config.atc_local_spawn_interval) {
		if (last_spawn > time(NULL)) { // Number of seconds 1/1/1970 (timestamp): to limit number of spawn at 1 every 2 seconds (Reduction of lag)
			sprintf(atcmd_output, "Please wait %d second(s) before to spawn a monster to avoid lag around of you.", (int)(last_spawn - time(NULL)));
			clif_displaymessage(fd, atcmd_output);
			return -1;
		}
		last_spawn = time(NULL) + battle_config.atc_local_spawn_interval;
	}

	// Check for monster size
	if (strstr(command, "small") != NULL)
		size = MAX_MOB_DB; // +2000 small
	else if (strstr(command, "big") != NULL)
		size = (MAX_MOB_DB * 2); // +4000 big
	else
		size = 0; // normal

	// Check for agressive monster
	if (strstr(command, "agro") != NULL)
		agro = 1; // Agressive
	else if (strstr(command, "neutral") != NULL)
		agro = -1; // Not Agressive
	else
		agro = 0; // Normal

	count = 0;
	range = sqrt(limited_number) * 2 + 1; // Calculation of an odd number
	for (i = 0; i < limited_number; i++) {
		j = 0;
		id = 0;
		while(j++ < 64 && id == 0) { // Try 64 times to spawn the monster (Needed for close area)
			do {
				if (x <= 0)
					mx = sd->bl.x + (rand() % (range + j * 2) - ((range + j * 2) / 2));
				else
					mx = x;
				if (y <= 0)
					my = sd->bl.y + (rand() % (range + j * 2) - ((range + j * 2) / 2));
				else
					my = y;
			} while ((c = map_getcell(sd->bl.m, mx, my, CELL_CHKNOPASS)) && j++ < 64);
			if (!c) {
				id = mob_once_spawn(sd, "this", mx, my, name, mob_id + size, 1, "");
				if (agro == 1 && (md = (struct mob_data *)map_id2bl(id)))
					md->mode = mob_db[mob_id].mode | (0x1 + 0x4 + 0x80); // Like Dead Branch
				else if (agro == -1 && (md = (struct mob_data *)map_id2bl(id)))
					md->mode = mob_db[mob_id].mode & ~0x4; // Remove agressive flag
			}
		}
		count += (id != 0) ? 1 : 0;
	}

	if (count != 0) {
		if (number == count)
			clif_displaymessage(fd, msg_txt(39)); // All monsters summoned!
		else {
			sprintf(atcmd_output, msg_txt(240), count); // %d monster(s) summoned!
			clif_displaymessage(fd, atcmd_output);
		}
		slave_num = 0;
		mob_num = 0;
		for (b = 0; b < map[sd->bl.m].bxs * map[sd->bl.m].bys; b++)
			for (bl = map[sd->bl.m].block_mob[b]; bl; bl = bl->next) {
				mob_num++;
				if (((struct mob_data *)bl)->master_id)
					slave_num++;
			}
		if (slave_num == 0)
			sprintf(atcmd_output, "Total mobs in map: %d (of which is no slave).", mob_num);
		else
			sprintf(atcmd_output, "Total mobs in map: %d (of which are %d slaves).", mob_num, slave_num);
		clif_displaymessage(fd, atcmd_output);
	} else {
		clif_displaymessage(fd, msg_txt(40)); // Invalid monster ID or name.
		return -1;
	}

	return 0;
}

/*==========================================
 * @spawnmap - Spawns monsters across a map
 *------------------------------------------
 */
ATCOMMAND_FUNC(spawnmap) {
	char name[100];
	char monster[100];
	int mob_id;
	int number = 0;
	int limited_number; // For correct display
	int count;
	int c, i, j, id;
	int x, y;
	struct mob_data *md;
	struct block_list *bl;
	int b, mob_num, slave_num;
	int size, agro;

	memset(name, 0, sizeof(name));
	memset(monster, 0, sizeof(monster));

	if (!message || !*message ||
	    (sscanf(message, "\"%[^\"]\" %s %d", name, monster, &number) < 2 &&
	     sscanf(message, "%s \"%[^\"]\" %d", monster, name, &number) < 2 &&
	     sscanf(message, "%s %d \"%[^\"]\"", monster, &number, name) < 3 &&
	     sscanf(message, "%s %d %s", monster, &number, name) < 1)) {
		clif_displaymessage(fd, msg_txt(143)); // Give a monster name/id please.
		return -1;
	}

	// If monster identifier/name argument is a name
	if ((mob_id = mobdb_searchname(monster)) == 0) // Check name first (To avoid possible name begining by a number)
		mob_id = mobdb_checkid(atoi(monster));

	if (mob_id == 0) {
		clif_displaymessage(fd, msg_txt(40)); // Invalid monster ID or name.
		return -1;
	}

	if (mob_id == 1288) {
		clif_displaymessage(fd, msg_txt(83)); // Cannot spawn emperium.
		return -1;
	}

	if (!check_mob_authorization(mob_id, sd->GM_level)) {
		clif_displaymessage(fd, "You are not authorized to spawn this monster.");
		return -1;
	}

	if (number <= 0)
		number = 1;

	if (name[0] == '\0')
		strcpy(name, "--ja--");

	// If value of atcommand_spawn_quantity_limit directive is greater than or equal to 1 and quantity of monsters is greater than value of the directive
	limited_number = number;
	if (battle_config.atc_spawn_quantity_limit > 0 && number > battle_config.atc_spawn_quantity_limit)
		limited_number = battle_config.atc_spawn_quantity_limit;

	if (battle_config.atc_map_mob_limit > 0) {
		mob_num = 0;
		for (b = 0; b < map[sd->bl.m].bxs * map[sd->bl.m].bys; b++)
			for (bl = map[sd->bl.m].block_mob[b]; bl; bl = bl->next)
				mob_num++;
		if (mob_num >= battle_config.atc_map_mob_limit) {
			clif_displaymessage(fd, "There is too many monsters on the map. You can not spawn more monsters.");
			return -1;
		} else if (mob_num + limited_number > battle_config.atc_map_mob_limit) {
			sprintf(atcmd_output, "Due to a density of monsters on the map, spawn has been limited.");
			clif_displaymessage(fd, atcmd_output);
			limited_number = battle_config.atc_map_mob_limit - mob_num;
		}
	}

	if (battle_config.etc_log)
		printf("%s monster='%s' name='%s' id=%d count=%d\n", command, monster, name, mob_id, limited_number);

	// Check for monster size
	if (strstr(command, "small") != NULL)
		size = MAX_MOB_DB; // +2000 small
	else if (strstr(command, "big") != NULL)
		size = (MAX_MOB_DB * 2); // +4000 big
	else
		size = 0; // Normal

	// Check for agressive monster
	if (strstr(command, "agro") != NULL)
		agro = 1; // Agressive
	else if (strstr(command, "neutral") != NULL)
		agro = -1; // Not agressive
	else
		agro = 0; // Normal

	count = 0;
	for (i = 0; i < limited_number; i++) {
		j = 0;
		id = 0;
		while(j++ < 64 && id == 0) { // Try 64 times to spawn the monster (Needed for close area)
			do {
				x = rand() % (map[sd->bl.m].xs - 2) + 1;
				y = rand() % (map[sd->bl.m].ys - 2) + 1;
			} while ((c = map_getcell(sd->bl.m, x, y, CELL_CHKNOPASS)) && j++ < 64);
			if (!c) {
				id = mob_once_spawn(sd, "this", x, y, name, mob_id + size, 1, "");
				if (agro == 1 && (md = (struct mob_data *)map_id2bl(id)))
					md->mode = mob_db[mob_id].mode | (0x1 + 0x4 + 0x80); // Like Dead Branch
				else if (agro == -1 && (md = (struct mob_data *)map_id2bl(id)))
					md->mode = mob_db[mob_id].mode & ~0x4; // Remove agressive flag
			}
		}
		count += (id != 0) ? 1 : 0;
	}

	if (count != 0) {
		if (number == count)
			clif_displaymessage(fd, msg_txt(39)); // All monsters summoned!
		else {
			sprintf(atcmd_output, msg_txt(240), count); // %d monster(s) summoned!
			clif_displaymessage(fd, atcmd_output);
		}
		slave_num = 0;
		mob_num = 0;
		for (b = 0; b < map[sd->bl.m].bxs * map[sd->bl.m].bys; b++)
			for (bl = map[sd->bl.m].block_mob[b]; bl; bl = bl->next) {
				mob_num++;
				if (((struct mob_data *)bl)->master_id)
					slave_num++;
			}
		if (slave_num == 0)
			sprintf(atcmd_output, "Total mobs in map: %d (of which is no slave).", mob_num);
		else
			sprintf(atcmd_output, "Total mobs in map: %d (of which are %d slaves).", mob_num, slave_num);
		clif_displaymessage(fd, atcmd_output);
	} else {
		clif_displaymessage(fd, msg_txt(40)); // Invalid monster ID or name.
		return -1;
	}

	return 0;
}

/*==========================================
 * @spawnall - Spawns all monsters
 *------------------------------------------
 */
ATCOMMAND_FUNC(spawnall) {
	char name[100];
	char monster[100];
	int mob_id;
	int number = 0;
	int limited_number; // For correct display
	int count;
	int c, i, j, k, id;
	int x, y, range;
	struct mob_data *md;
	int size, agro;
	struct map_session_data *pl_sd;

	memset(name, 0, sizeof(name));
	memset(monster, 0, sizeof(monster));

	if (!message || !*message ||
	    (sscanf(message, "\"%[^\"]\" %s %d", name, monster, &number) < 2 &&
	     sscanf(message, "%s \"%[^\"]\" %d", monster, name, &number) < 2 &&
	     sscanf(message, "%s %d \"%[^\"]\"", monster, &number, name) < 3 &&
	     sscanf(message, "%s %d %s", monster, &number, name) < 1)) {
		clif_displaymessage(fd, msg_txt(143)); // Give a monster name/id please.
		return -1;
	}

	// If monster identifier/name argument is a name
	if ((mob_id = mobdb_searchname(monster)) == 0) // Check name first (To avoid possible name begining by a number)
		mob_id = mobdb_checkid(atoi(monster));

	if (mob_id == 0) {
		clif_displaymessage(fd, msg_txt(40)); // Invalid monster ID or name.
		return -1;
	}

	if (mob_id == 1288) {
		clif_displaymessage(fd, msg_txt(83)); // Cannot spawn emperium.
		return -1;
	}

	if (!check_mob_authorization(mob_id, sd->GM_level)) {
		clif_displaymessage(fd, "You are not authorized to spawn this monster.");
		return -1;
	}

	if (number <= 0)
		number = 1;

	if (name[0] == '\0')
		strcpy(name, "--ja--");

	// If value of atcommand_spawn_quantity_limit directive is greater than or equal to 1 and quantity of monsters is greater than value of the directive
	limited_number = number;
	if (battle_config.atc_spawn_quantity_limit > 0 && number > battle_config.atc_spawn_quantity_limit)
		limited_number = battle_config.atc_spawn_quantity_limit;

	if (battle_config.etc_log)
		printf("%s monster='%s' name='%s' id=%d count=%d\n", command, monster, name, mob_id, limited_number);

	// Check for monster size
	if (strstr(command, "small") != NULL)
		size = MAX_MOB_DB; // +2000 small
	else if (strstr(command, "big") != NULL)
		size = (MAX_MOB_DB * 2); // +4000 big
	else
		size = 0; // Normal

	// Check for agressive monster
	if (strstr(command, "agro") != NULL)
		agro = 1; // Agressive
	else if (strstr(command, "neutral") != NULL)
		agro = -1; // Not agressive
	else
		agro = 0; // Normal

	count = 0;
	range = sqrt(limited_number) * 2 + 1; // Calculation of an odd number
	for (i = 0; i < fd_max; i++) {
		if (session[i] && (pl_sd = session[i]->session_data) && pl_sd->state.auth) {
			if (!((pl_sd->GM_level >= battle_config.hide_GM_session || (pl_sd->status.option & OPTION_HIDE)) && (pl_sd->GM_level > sd->GM_level))) { // Only lower or same level
				for (k = 0; k < limited_number; k++) {
					j = 0;
					id = 0;
					while(j++ < 64 && id == 0) { // Try 64 times to spawn the monster (Needed for close area)
						do {
							x = pl_sd->bl.x + (rand() % (range + j * 2) - ((range + j * 2) / 2));
							y = pl_sd->bl.y + (rand() % (range + j * 2) - ((range + j * 2) / 2));
						} while ((c = map_getcell(pl_sd->bl.m, x, y, CELL_CHKNOPASS)) && j++ < 64);
						if (!c) {
							id = mob_once_spawn(pl_sd, "this", x, y, name, mob_id + size, 1, "");
							if (agro == 1 && (md = (struct mob_data *)map_id2bl(id)))
								md->mode = mob_db[mob_id].mode | (0x1 + 0x4 + 0x80); // Like Dead Branch
							else if (agro == -1 && (md = (struct mob_data *)map_id2bl(id)))
								md->mode = mob_db[mob_id].mode & ~0x4; // Remove agressive flag
						}
					}
				}
				count++;
			}
		}
	}

	if (count == 0) // Impossible (GM is online)
		clif_displaymessage(fd, "No monster has been spawned!");
	else {
		if (count == 1)
			sprintf(atcmd_output, "%d monster%s summoned on 1 player!", limited_number, (limited_number > 1) ? "s" : "");
		else
			sprintf(atcmd_output, "%d monster%s summoned on each player (%d players)!", limited_number, (limited_number > 1) ? "s" : "", count);
		clif_displaymessage(fd, atcmd_output);
	}

	return 0;
}

/*==========================================
 * @killmonster - Kills all monsters on a map
 *------------------------------------------
 */
ATCOMMAND_FUNC(killmonster) {
	int map_id;

	if (!message || !*message || sscanf(message, "%s", atcmd_mapname) < 1)
		map_id = sd->bl.m;
	else {
		if (strstr(atcmd_mapname, ".gat") == NULL && strlen(atcmd_mapname) < 13) // 16 - 4 (.gat)
			strcat(atcmd_mapname, ".gat");
		if ((map_id = map_mapname2mapid(atcmd_mapname)) < 0) // Only from actual map-server
			map_id = sd->bl.m;
	}

	map_foreachinarea(atkillmonster_sub, map_id, 0, 0, map[map_id].xs, map[map_id].ys, BL_MOB, 1); // 0: No drop, 1: Drop

	clif_displaymessage(fd, msg_txt(165)); // All monsters killed!


	return 0;
}

/*==========================================
 * @killmonster2 - Kills all monsters on a map without leaving drops
 *------------------------------------------
 */
ATCOMMAND_FUNC(killmonster2) {
	int map_id;

	if (!message || !*message || sscanf(message, "%s", atcmd_mapname) < 1)
		map_id = sd->bl.m;
	else {
		if (strstr(atcmd_mapname, ".gat") == NULL && strlen(atcmd_mapname) < 13) // 16 - 4 (.gat)
			strcat(atcmd_mapname, ".gat");
		if ((map_id = map_mapname2mapid(atcmd_mapname)) < 0) // Only from actual map-server
			map_id = sd->bl.m;
	}

	map_foreachinarea(atkillmonster_sub, map_id, 0, 0, map[map_id].xs, map[map_id].ys, BL_MOB, 0); // 0: No drop, 1: Drop

	clif_displaymessage(fd, msg_txt(165)); // All monsters killed!

	return 0;
}

/*==========================================
 * @killmonsterarea - Kills all monsters in a specified area
 *------------------------------------------
 */
ATCOMMAND_FUNC(killmonsterarea) {
	int area_size;

	area_size = AREA_SIZE;
	sscanf(message, "%d", &area_size);
	if (area_size < 1)
		area_size = 1;

	map_foreachinarea(atkillmonster_sub, sd->bl.m, sd->bl.x - area_size, sd->bl.y - area_size, sd->bl.x + area_size, sd->bl.y + area_size, BL_MOB, 1); // 0: No drop, 1: Drop

	clif_displaymessage(fd, msg_txt(165)); // All monsters killed!


	return 0;
}

/*==========================================
 * @killmonster2area - Kills all monsters in a specified area without drops
 *------------------------------------------
 */
ATCOMMAND_FUNC(killmonster2area) {
	int area_size;

	area_size = AREA_SIZE;
	sscanf(message, "%d", &area_size);
	if (area_size < 1)
		area_size = 1;

	map_foreachinarea(atkillmonster_sub, sd->bl.m, sd->bl.x - area_size, sd->bl.y - area_size, sd->bl.x + area_size, sd->bl.y + area_size, BL_MOB, 0); // 0: No drop, 1: Drop

	clif_displaymessage(fd, msg_txt(165)); // All monsters killed!

	return 0;
}

/*==========================================
 * @refine - Refines all equipment, regardless of refinery capability
 *------------------------------------------
 */
ATCOMMAND_FUNC(refine) {
	int i, position = 0, refine = 0, current_position, final_refine;
	int count;

	sscanf(message, "%d %d", &position, &refine);
	if (position < 0) {
		send_usage(sd, "Please, enter a position and a amount (usage: %s [equip position [+/- amount]]).", original_command);
		send_usage(sd, "(%s 0 [+/- amount] -> refine all items.)", original_command);
		return -1;
	}

	if (refine < -10)
		refine = -10;
	else if (refine > 10)
		refine = 10;
	else if (refine == 0)
		refine = 1;

	count = 0;
	for (i = 0; i < MAX_INVENTORY; i++) {
		if (sd->status.inventory[i].nameid &&
		    (sd->status.inventory[i].equip & position ||
			(sd->status.inventory[i].equip && !position))) {
			final_refine = sd->status.inventory[i].refine + refine;
			if (final_refine > 10)
				final_refine = 10;
			else if (final_refine < 0)
				final_refine = 0;
			if (sd->status.inventory[i].refine != final_refine) {
				sd->status.inventory[i].refine = final_refine;
				current_position = sd->status.inventory[i].equip;
				pc_unequipitem(sd, i, 3);
				clif_refine(fd, sd, 0, i, sd->status.inventory[i].refine);
				clif_delitem(sd, i, 1);
				clif_additem(sd, i, 1, 0); // 0: You got...
				pc_equipitem(sd, i, current_position);
				count++;
			}
		}
	}
	// Do effect if at least 1 item is modified
	if (count > 0)
		clif_misceffect((struct block_list*)&sd->bl, 3);

	if (count == 0)
		clif_displaymessage(fd, msg_txt(166)); // No item has been refined!
	else if (count == 1)
		clif_displaymessage(fd, msg_txt(167)); // 1 item has been refined!
	else {
		sprintf(atcmd_output, msg_txt(168), count); // %d items have been refined!
		clif_displaymessage(fd, atcmd_output);
	}

	return 0;
}

/*==========================================
 * @refineall - Refines all items
 *------------------------------------------
 */
ATCOMMAND_FUNC(refineall) {
	char message2[103]; // '0 ' (2) + message (max 100) + NULL (1) = 103

	// Preparation of message
	sprintf(message2, "0 %s", message);

	// Call refine function
	return atcommand_refine(fd, sd, original_command, "@refine", message2); // Use local buffer to avoid problem (We call another function that can use global variables)
}

/*==========================================
 * @produce
 *------------------------------------------
 */
ATCOMMAND_FUNC(produce) {
	char item_name[100];
	int item_id, attribute = 0, star = 0;
	int flag = 0;
	struct item_data *item_data;
	struct item tmp_item;

	memset(item_name, 0, sizeof(item_name));

	if (!message || !*message || sscanf(message, "%s %d %d", item_name, &attribute, &star) < 1) {
		send_usage(sd, "Please, enter at least an item name/id (usage: %s <equip name or equip ID> <element> <# of very's>).", original_command);
		return -1;
	}

	item_id = 0;
	if ((item_data = itemdb_searchname(item_name)) != NULL ||
	    (item_data = itemdb_exists(atoi(item_name))) != NULL)
		item_id = item_data->nameid;

	if (itemdb_exists(item_id) &&
	    (item_id <  501 || item_id > 1099) && // Weapons: 1100-1999, Armors/Equipment: 2000-2999
	    (item_id < 4001 || item_id > 4407) && // Cards: 4000-4999, Headgears: 5000-6000
	    (item_id < 7001 || item_id > 10019) && // Etc: 7000-7999, Eggs: 9000-9999, Pet Accessories: 10000-10999, History Books: 11000-11999, Scrolls/Quivers: 12000-12999
	    itemdb_isequip(item_id)) {
		if (check_item_authorization(item_id, sd->GM_level)) {
			if (attribute < MIN_ATTRIBUTE || attribute > MAX_ATTRIBUTE)
				attribute = ATTRIBUTE_NORMAL;
			if (star < MIN_STAR || star > MAX_STAR)
				star = 0;
			memset(&tmp_item, 0, sizeof tmp_item);
			tmp_item.nameid = item_id;
			tmp_item.amount = 1;
			tmp_item.identify = 1;
			tmp_item.card[0] = 0x00ff;
			tmp_item.card[1] = ((star * 5) << 8) + attribute;
			*((unsigned long *)(&tmp_item.card[2])) = sd->char_id;
			clif_produceeffect(sd, 0, item_id);
			clif_misceffect(&sd->bl, 3);
			if ((flag = pc_additem(sd, &tmp_item, 1)))
				clif_additem(sd, 0, 0, flag);
			// Display is not necessary
		} else {
			clif_displaymessage(fd, "You are not authorized to create this item.");
			return -1;
		}
	} else {
		if (battle_config.error_log)
			printf("%s NOT WEAPON [%d]\n", original_command, item_id);
		if (item_id != 0 && itemdb_exists(item_id)) {
			sprintf(atcmd_output, msg_txt(169), item_id, item_data->name); // This item (%d: '%s') is not an equipment.
			clif_displaymessage(fd, atcmd_output);
		} else
			clif_displaymessage(fd, msg_txt(170)); // This item is not an equipment.
		return -1;
	}

	return 0;
}

/*==========================================
 * Sub-function to display actual memo points
 *------------------------------------------
 */
void atcommand_memo_sub(struct map_session_data* sd) {
	int i;

	clif_displaymessage(sd->fd, "Your actual memo positions are (except respawn point):");
	for (i = 0; i < MAX_PORTAL_MEMO; i++) {
		if (sd->status.memo_point[i].map[0])
			sprintf(atcmd_output, "%d - %s (%d,%d)", i, sd->status.memo_point[i].map, sd->status.memo_point[i].x, sd->status.memo_point[i].y);
		else
			sprintf(atcmd_output, msg_txt(171), i); // %d - void
		clif_displaymessage(sd->fd, atcmd_output);
	}

	return;
}

/*==========================================
 * @memo - Memos a map like /memo
 *------------------------------------------
 */
ATCOMMAND_FUNC(memo) {
	int position = 0;

	if (!message || !*message || sscanf(message, "%d", &position) < 1)
		atcommand_memo_sub(sd);
	else {
		if (position >= 0 && position < MAX_PORTAL_MEMO) {
			if (sd->bl.m >= 0 && map[sd->bl.m].flag.nowarpto && battle_config.any_warp_GM_min_level > sd->GM_level) {
				clif_displaymessage(fd, "You are not authorized to memo this map.");
				return -1;
			}
			if (sd->status.memo_point[position].map[0]) {
				sprintf(atcmd_output, msg_txt(172), position, sd->status.memo_point[position].map, sd->status.memo_point[position].x, sd->status.memo_point[position].y); // You replace previous memo position %d - %s (%d,%d).
				clif_displaymessage(fd, atcmd_output);
			}
			memset(sd->status.memo_point[position].map, 0, sizeof(sd->status.memo_point[position].map));
			strncpy(sd->status.memo_point[position].map, map[sd->bl.m].name, 16); // 17 - NULL
			sd->status.memo_point[position].x = sd->bl.x;
			sd->status.memo_point[position].y = sd->bl.y;
			clif_skill_memo(sd, 0); // 00: Success to take memo., 01: insuffisant skill level., 02: You don't know warp skill.
			if (pc_checkskill(sd, AL_WARP) <= (position + 1))
				clif_displaymessage(fd, msg_txt(173)); // Note: you don't have the 'Warp' skill level to use it.
			atcommand_memo_sub(sd);
		} else {
			send_usage(sd, "Please, enter a valid position (usage: %s <memo_position:%d-%d>).", original_command, 0, MAX_PORTAL_MEMO - 1);
			atcommand_memo_sub(sd);
			return -1;
		}
	}

	return 0;
}

/*==========================================
 * @gat - Displays gat map information
 *------------------------------------------
 */
ATCOMMAND_FUNC(gat) {
	int y, x;

	clif_displaymessage(fd, "Ground informations around the player:");
	for (y = sd->bl.y + 2; y >= sd->bl.y - 2; y--) {
		if (y >= 0 && y < map[sd->bl.m].ys) {
			if (sd->bl.x < 2)
				x = 2;
			else if (sd->bl.x >= map[sd->bl.m].xs - 2)
				x = map[sd->bl.m].xs - 3;
			else
				x = sd->bl.x;
			sprintf(atcmd_output, "%s (x= %d-%d, y= %d) %02X %02X %02X %02X %02X",
			         map[sd->bl.m].name,   x - 2, x + 2, y,
			         map_getcell(sd->bl.m, x - 2, y, CELL_GETTYPE),
			         map_getcell(sd->bl.m, x - 1, y, CELL_GETTYPE),
			         map_getcell(sd->bl.m, x    , y, CELL_GETTYPE),
			         map_getcell(sd->bl.m, x + 1, y, CELL_GETTYPE),
			         map_getcell(sd->bl.m, x + 2, y, CELL_GETTYPE));
			clif_displaymessage(fd, atcmd_output);
		}
	}

	return 0;
}

/*==========================================
 * @send - Used for testing packet sends from the client
 *------------------------------------------
 */
ATCOMMAND_FUNC(send) {
	unsigned char buf[1024];
	int a = 0;

	memset(buf, 0, sizeof(buf));

	if (!message || !*message || sscanf(message, "%d", &a) < 1)
		return -1;

	clif_misceffect(&sd->bl, a);

	return 0;
}

/*==========================================
 * @statusicon
 *------------------------------------------
 */
ATCOMMAND_FUNC(statusicon) {
	int x, y;

	if (!message || !*message || sscanf(message, "%d %d", &x, &y) < 2) {
		send_usage(sd, "Please, enter a status type/flag (usage: %s <status type> <flag>).", original_command);
		return -1;
	}

	clif_status_change(&sd->bl, x, y);

	return 0;
}

/*==========================================
 * @stpoint
 *------------------------------------------
 */
ATCOMMAND_FUNC(statuspoint) {
	int point, new_status_point;

	if (!message || !*message || sscanf(message, "%d", &point) < 1 || point == 0) {
		send_usage(sd, "Please, enter a number (usage: %s <number of points>).", original_command);
		return -1;
	}

	new_status_point = (int)sd->status.status_point + point;
	if (point > 0 && (point > 0x7FFF || new_status_point > 0x7FFF)) // Fix positive overflow
		new_status_point = 0x7FFF;
	else if (point < 0 && (point < -0x7FFF || new_status_point < 0)) // Fix negative overflow
		new_status_point = 0;

	if (new_status_point != (int)sd->status.status_point) {
		// If player have max in all stats, don't give status_point
		if (sd->status.str  >= battle_config.max_parameter &&
		    sd->status.agi  >= battle_config.max_parameter &&
		    sd->status.vit  >= battle_config.max_parameter &&
		    sd->status.int_ >= battle_config.max_parameter &&
		    sd->status.dex  >= battle_config.max_parameter &&
		    sd->status.luk  >= battle_config.max_parameter &&
		    (short)new_status_point != 0) {
			sd->status.status_point = 0;
			clif_updatestatus(sd, SP_STATUSPOINT);
			clif_displaymessage(fd, "You have max in each stat -> status points set to 0.");
			return -1;
		} else {
			sd->status.status_point = (short)new_status_point;
			clif_updatestatus(sd, SP_STATUSPOINT);
			clif_displaymessage(fd, msg_txt(174)); // Number of status points changed!
		}
	} else {
		if (point < 0)
			clif_displaymessage(fd, msg_txt(41)); // Impossible to decrease the number/value.
		else
			clif_displaymessage(fd, msg_txt(149)); // Impossible to increase the number/value.
		return -1;
	}

	return 0;
}

/*==========================================
 * @skpoint
 *------------------------------------------
 */
ATCOMMAND_FUNC(skillpoint) {
	int point, new_skill_point;

	if (!message || !*message || sscanf(message, "%d", &point) < 1 || point == 0) {
		send_usage(sd, "Please, enter a number (usage: %s <number of points>).", original_command);
		return -1;
	}

	new_skill_point = (int)sd->status.skill_point + point;
	if (point > 0 && (point > 0x7FFF || new_skill_point > 0x7FFF)) // Fix positive overflow
		new_skill_point = 0x7FFF;
	else if (point < 0 && (point < -0x7FFF || new_skill_point < 0)) // Fix negative overflow
		new_skill_point = 0;

	if (new_skill_point != (int)sd->status.skill_point) {
		sd->status.skill_point = (short)new_skill_point;
		clif_updatestatus(sd, SP_SKILLPOINT);
		clif_displaymessage(fd, msg_txt(175)); // Number of skill points changed!
	} else {
		if (point < 0)
			clif_displaymessage(fd, msg_txt(41)); // Impossible to decrease the number/value.
		else
			clif_displaymessage(fd, msg_txt(149)); // Impossible to increase the number/value.
		return -1;
	}

	return 0;
}

/*==========================================
 * @zeny
 *------------------------------------------
 */
ATCOMMAND_FUNC(zeny) {
	int zeny, new_zeny;

	if (!message || !*message || sscanf(message, "%d", &zeny) < 1 || zeny == 0) {
		send_usage(sd, "Please, enter an amount (usage: %s <amount>).", original_command);
		return -1;
	}

	new_zeny = sd->status.zeny + zeny;
	if (zeny > 0 && (zeny > MAX_ZENY || new_zeny > MAX_ZENY)) // Fix positive overflow
		new_zeny = MAX_ZENY;
	else if (zeny < 0 && (zeny < -MAX_ZENY || new_zeny < 0)) // Fix negative overflow
		new_zeny = 0;

	if (new_zeny != sd->status.zeny) {
		sd->status.zeny = new_zeny;
		clif_updatestatus(sd, SP_ZENY);
		clif_displaymessage(fd, msg_txt(176)); // Number of zenys changed!
	} else {
		if (zeny < 0)
			clif_displaymessage(fd, msg_txt(41)); // Impossible to decrease the number/value.
		else
			clif_displaymessage(fd, msg_txt(149)); // Impossible to increase the number/value.
		return -1;
	}

	return 0;
}

/*==========================================
 * @param
 *------------------------------------------
 */
ATCOMMAND_FUNC(param) {
	int i, idx, value = 0, new_value;
	const char* param[] = { "@str", "@agi", "@vit", "@int", "@dex", "@luk", NULL };
	short* status[] = {
		&sd->status.str,  &sd->status.agi, &sd->status.vit,
		&sd->status.int_, &sd->status.dex, &sd->status.luk
	};

	if (!message || !*message || sscanf(message, "%d", &value) < 1 || value == 0) {
		send_usage(sd, "Please, enter a valid value (usage: %s <+/-adjustement>).", original_command);
		return -1;
	}

	idx = -1;
	for (i = 0; param[i] != NULL; i++) {
		if (strcasecmp(command + 1, param[i] + 1) == 0) { // To suppress symbol of GM command
			idx = i;
			break;
		}
	}
	if (idx < 0 || idx > MAX_STATUS_TYPE) { // Normaly impossible...
		send_usage(sd, "Please, enter a valid value (usage: %s <+/-adjustement>).", original_command);
		return -1;
	}

	new_value = (int)*status[idx] + value;
	if (value > 0 && (value > battle_config.max_parameter || new_value > battle_config.max_parameter)) // Fix positive overflow
		new_value = battle_config.max_parameter;
	else if (value < 0 && (value < -battle_config.max_parameter || new_value < 1)) // Fix negative overflow
		new_value = 1;

	if (new_value != (int)*status[idx]) {
		*status[idx] = ((short)new_value);
		clif_updatestatus(sd, SP_STR + idx);
		clif_updatestatus(sd, SP_USTR + idx);
		status_calc_pc(sd, 0);
		clif_displaymessage(fd, msg_txt(42)); // Stat changed.
		// If player have max in all stats, don't give status_point
		if (sd->status.str  >= battle_config.max_parameter &&
		    sd->status.agi  >= battle_config.max_parameter &&
		    sd->status.vit  >= battle_config.max_parameter &&
		    sd->status.int_ >= battle_config.max_parameter &&
		    sd->status.dex  >= battle_config.max_parameter &&
		    sd->status.luk  >= battle_config.max_parameter &&
		    sd->status.status_point != 0) {
			sd->status.status_point = 0;
			clif_updatestatus(sd, SP_STATUSPOINT);
			clif_displaymessage(fd, "You have max in each stat -> status points set to 0.");
		}
	} else {
		if (value < 0)
			clif_displaymessage(fd, msg_txt(41)); // Impossible to decrease the number/value.
		else
			clif_displaymessage(fd, msg_txt(149)); // Impossible to increase the number/value.
		return -1;
	}

	return 0;
}

/*==========================================
 * @statsall
 *------------------------------------------
 */
ATCOMMAND_FUNC(stat_all) {
	int idx, count, value = 0, new_value;
	short* status[] = {
		&sd->status.str,  &sd->status.agi, &sd->status.vit,
		&sd->status.int_, &sd->status.dex, &sd->status.luk
	};

	if (!message || !*message || sscanf(message, "%d", &value) < 1 || value == 0)
		value = battle_config.max_parameter;

	count = 0;
	for (idx = 0; idx < (int)(sizeof(status) / sizeof(status[0])); idx++) {

		new_value = (int)*status[idx] + value;
		if (value > 0 && (value > battle_config.max_parameter || new_value > battle_config.max_parameter)) // Fix positive overflow
			new_value = battle_config.max_parameter;
		else if (value < 0 && (value < -battle_config.max_parameter || new_value < 1)) // Fix negative overflow
			new_value = 1;

		if (new_value != (int)*status[idx]) {
			*status[idx] = ((short)new_value);
			clif_updatestatus(sd, SP_STR + idx);
			clif_updatestatus(sd, SP_USTR + idx);
			status_calc_pc(sd, 0);
			count++;
		}
	}

	if (count > 0) { // If at least 1 stat modified
		clif_displaymessage(fd, msg_txt(84)); // All stats changed!
		// If player have max in all stats, don't give status_point
		if (sd->status.str  >= battle_config.max_parameter &&
		    sd->status.agi  >= battle_config.max_parameter &&
		    sd->status.vit  >= battle_config.max_parameter &&
		    sd->status.int_ >= battle_config.max_parameter &&
		    sd->status.dex  >= battle_config.max_parameter &&
		    sd->status.luk  >= battle_config.max_parameter &&
		    sd->status.status_point != 0) {
			sd->status.status_point = 0;
			clif_updatestatus(sd, SP_STATUSPOINT);
			clif_displaymessage(fd, "You have max in each stat -> status points set to 0.");
		}
	} else {
		if (value < 0)
			clif_displaymessage(fd, msg_txt(177)); // Impossible to decrease a stat.
		else
			clif_displaymessage(fd, msg_txt(178)); // Impossible to increase a stat.
		return -1;
	}

	return 0;
}

/*==========================================
 * @guildlevelup - Levels up user's guild
 *------------------------------------------
 */
ATCOMMAND_FUNC(guildlevelup) {
	int level = 0;
	short added_level;
	struct guild *guild_info;

	if (!message || !*message || sscanf(message, "%d", &level) < 1 || level == 0) {
		send_usage(sd, "Please, enter a valid level (usage: %s <# of levels>).", original_command);
		return -1;
	}

	if (sd->status.guild_id <= 0 || (guild_info = guild_search(sd->status.guild_id)) == NULL) {
		clif_displaymessage(fd, msg_txt(43)); // You're not in a guild.
		return -1;
	}
	if (strcmp(sd->status.name, guild_info->master) != 0) {
		clif_displaymessage(fd, msg_txt(44)); // You're not the master of your guild.
		return -1;
	}

	added_level = (short)level;
	if (level > 0 && (level > MAX_GUILDLEVEL || added_level > ((short)MAX_GUILDLEVEL - guild_info->guild_lv))) // Fix positive overflow
		added_level = (short)MAX_GUILDLEVEL - guild_info->guild_lv;
	else if (level < 0 && (level < -MAX_GUILDLEVEL || added_level < (1 - guild_info->guild_lv))) // Fix negative overflow
		added_level = 1 - guild_info->guild_lv;

	if (added_level != 0) {
		intif_guild_change_basicinfo(guild_info->guild_id, GBI_GUILDLV, &added_level, 2); // 2= sizeof(added_level)
		clif_displaymessage(fd, msg_txt(179)); // Guild level changed.
	} else {
		clif_displaymessage(fd, msg_txt(45)); // Guild level change failed.
		return -1;
	}

	return 0;
}

/*==========================================
 * @charguildlevelup - Level's up a target's guild
 *------------------------------------------
 */
ATCOMMAND_FUNC(charguildlevelup) {
	int level;
	short added_level;
	struct guild *guild_info;
	struct map_session_data *pl_sd;

	if (!message || !*message || sscanf(message, "%d %[^\n]", &level, atcmd_name) < 2 || level == 0) {
		send_usage(sd, "Please, enter a valid level (usage: %s <# of levels> <char name|account_id>).", original_command);
		return -1;
	}

	if ((pl_sd = map_nick2sd(atcmd_name)) != NULL || ((pl_sd = map_id2sd(atoi(atcmd_name))) != NULL && pl_sd->state.auth)) {
		if (sd->GM_level >= pl_sd->GM_level) { // Only lower or same level
			if (pl_sd->status.guild_id <= 0 || (guild_info = guild_search(pl_sd->status.guild_id)) == NULL) {
				clif_displaymessage(fd, "This player is not in a guild.");
				return -1;
			}
			if (strcmp(pl_sd->status.name, guild_info->master) != 0) {
				clif_displaymessage(fd, "This player is not the master of its guild.");
				return -1;
			}

			added_level = (short)level;
			if (level > 0 && (level > MAX_GUILDLEVEL || added_level > ((short)MAX_GUILDLEVEL - guild_info->guild_lv))) // Fix positive overflow
				added_level = (short)MAX_GUILDLEVEL - guild_info->guild_lv;
			else if (level < 0 && (level < -MAX_GUILDLEVEL || added_level < (1 - guild_info->guild_lv))) // Fix negative overflow
				added_level = 1 - guild_info->guild_lv;

			if (added_level != 0) {
				intif_guild_change_basicinfo(guild_info->guild_id, GBI_GUILDLV, &added_level, 2); // 2= sizeof(added_level)
				clif_displaymessage(fd, msg_txt(179)); // Guild level changed.
			} else {
				clif_displaymessage(fd, msg_txt(45)); // Guild level change failed.
				return -1;
			}
		} else {
			clif_displaymessage(fd, msg_txt(81)); // Your GM level don't authorize you to do this action on this player.
			return -1;
		}
	} else {
		clif_displaymessage(fd, msg_txt(3)); // Character not found.
		return -1;
	}

	return 0;
}

/*==========================================
 * @petfriendly - Changes user's pet's intimacy value
 *------------------------------------------
 */
ATCOMMAND_FUNC(petfriendly) {
	int friendly;
	int t;

	if (!message || !*message || sscanf(message, "%d", &friendly) < 1 || friendly < 0 || friendly > 1000) {
		send_usage(sd, "Please, enter a valid value (usage: %s <0-1000>).", original_command);
		return -1;
	}

	if (sd->status.pet_id > 0 && sd->pd) {
		if (friendly != sd->pet.intimate) {
			t = sd->pet.intimate;
			sd->pet.intimate = friendly;
			clif_send_petstatus(sd);
			if (battle_config.pet_status_support) {
				if ((sd->pet.intimate > 0 && t <= 0) ||
				    (sd->pet.intimate <= 0 && t > 0)) {
					if (sd->bl.prev != NULL)
						status_calc_pc(sd, 0);
					else
						status_calc_pc(sd, 2);
				}
			}
			clif_displaymessage(fd, msg_txt(182)); // Pet friendly value changed!
		} else {
			clif_displaymessage(fd, msg_txt(183)); // Pet friendly is already the good value.
			return -1;
		}
	} else {
		clif_displaymessage(fd, msg_txt(184)); // Sorry, but you have no pet.
		return -1;
	}

	return 0;
}

/*==========================================
 * @pethungry - Changes a user's pet's hunger value
 *------------------------------------------
 */
ATCOMMAND_FUNC(pethungry) {
	int hungry;

	if (!message || !*message || sscanf(message, "%d", &hungry) < 1 || hungry < 0 || hungry > 100) {
		send_usage(sd, "Please, enter a valid number (usage: %s <0-100>).", original_command);
		return -1;
	}

	if (sd->status.pet_id > 0 && sd->pd) {
		if (hungry != sd->pet.hungry) {
			sd->pet.hungry = hungry;
			clif_send_petstatus(sd);
			clif_displaymessage(fd, msg_txt(185)); // Pet hungry value changed!
		} else {
			clif_displaymessage(fd, msg_txt(186)); // Pet hungry is already the good value.
			return -1;
		}
	} else {
		clif_displaymessage(fd, msg_txt(184)); // Sorry, but you have no pet.
		return -1;
	}

	return 0;
}

/*==========================================
 * @petrename - Renames user's pet
 *------------------------------------------
 */
ATCOMMAND_FUNC(petrename) {
	if (sd->status.pet_id > 0 && sd->pd) {
		if (sd->pet.rename_flag != 0) {
			sd->pet.rename_flag = 0;
			intif_save_petdata(sd->status.account_id, &sd->pet);
			clif_send_petstatus(sd);
			clif_displaymessage(fd, msg_txt(187)); // You can now rename your pet.
		} else {
			clif_displaymessage(fd, msg_txt(188)); // You can already rename your pet.
			return -1;
		}
	} else {
		clif_displaymessage(fd, msg_txt(184)); // Sorry, but you have no pet.
		return -1;
	}

	return 0;
}

/*==========================================
 * @charpetrename - Renames a target's pet
 *------------------------------------------
 */
ATCOMMAND_FUNC(charpetrename) {
	struct map_session_data *pl_sd;

	if (!message || !*message || sscanf(message, "%[^\n]", atcmd_name) < 1) {
		send_usage(sd, "Please, enter a player name (usage: %s <char name|account_id>).", original_command);
		return -1;
	}

	if ((pl_sd = map_nick2sd(atcmd_name)) != NULL || ((pl_sd = map_id2sd(atoi(atcmd_name))) != NULL && pl_sd->state.auth)) {
		if (pl_sd->status.pet_id > 0 && pl_sd->pd) {
			if (pl_sd->pet.rename_flag != 0) {
				pl_sd->pet.rename_flag = 0;
				intif_save_petdata(pl_sd->status.account_id, &pl_sd->pet);
				clif_send_petstatus(pl_sd);
				clif_displaymessage(fd, msg_txt(189)); // This player can now rename his/her pet.
			} else {
				clif_displaymessage(fd, msg_txt(190)); // This player can already rename his/her pet.
				return -1;
			}
		} else {
			clif_displaymessage(fd, msg_txt(191)); // Sorry, but this player has no pet.
			return -1;
		}
	} else {
		clif_displaymessage(fd, msg_txt(3)); // Character not found.
		return -1;
	}

	return 0;
}

/*==========================================
 * @recall - Warps a target to you
 *------------------------------------------
 */
ATCOMMAND_FUNC(recall) {
	struct map_session_data *pl_sd;

	if (!message || !*message || sscanf(message, "%[^\n]", atcmd_name) < 1) {
		send_usage(sd, "Please, enter a player name (usage: %s <char name|account_id>).", original_command);
		return -1;
	}

	if ((pl_sd = map_nick2sd(atcmd_name)) != NULL || ((pl_sd = map_id2sd(atoi(atcmd_name))) != NULL && pl_sd->state.auth)) {
		if (sd->GM_level >= pl_sd->GM_level) { // Only lower or same level
			if (sd->bl.m >= 0 && map[sd->bl.m].flag.nowarpto && battle_config.any_warp_GM_min_level > sd->GM_level) {
				clif_displaymessage(fd, "You are not authorized to warp somenone to your actual map.");
				return -1;
			}
			if (pl_sd->bl.m >= 0 && map[pl_sd->bl.m].flag.nowarp && battle_config.any_warp_GM_min_level > sd->GM_level) {
				clif_displaymessage(fd, "You are not authorized to warp this player from its actual map.");
				return -1;
			}
			pc_setpos(pl_sd, sd->mapname, sd->bl.x, sd->bl.y, 2, 0);
			sprintf(atcmd_output, msg_txt(46), pl_sd->status.name); // '%s' recalled!
			clif_displaymessage(fd, atcmd_output);
		} else {
			clif_displaymessage(fd, msg_txt(81)); // Your GM level don't authorize you to do this action on this player.
			return -1;
		}
	} else {
		clif_displaymessage(fd, msg_txt(3)); // Character not found.
		return -1;
	}

	return 0;
}

/*==========================================
 * @charjob - Changes a target's job
 *------------------------------------------
 */
ATCOMMAND_FUNC(character_job) {
	struct map_session_data* pl_sd;
	int job = 0, upper = -1;
	int flag, i;
	char jobname[100];

	const struct { char name[20]; int id; } jobs[] = {
		{ "novice",                    0 },
		{ "swordsman",                 1 },
		{ "swordman",                  1 },
		{ "mage",                      2 },
		{ "archer",                    3 },
		{ "acolyte",                   4 },
		{ "merchant",                  5 },
		{ "thief",                     6 },
		{ "knight",                    7 },
		{ "priest",                    8 },
		{ "priestess",                 8 },
		{ "wizard",                    9 },
		{ "blacksmith",               10 },
		{ "hunter",                   11 },
		{ "assassin",                 12 },
		{ "peco knight",              13 },
		{ "peco-knight",              13 },
		{ "peco_knight",              13 },
		{ "knight peco",              13 },
		{ "knight-peco",              13 },
		{ "knight_peco",              13 },
		{ "crusader",                 14 },
		{ "defender",                 14 },
		{ "monk",                     15 },
		{ "sage",                     16 },
		{ "rogue",                    17 },
		{ "alchemist",                18 },
		{ "bard",                     19 },
		{ "dancer",                   20 },
		{ "peco crusader",            21 },
		{ "peco-crusader",            21 },
		{ "peco_crusader",            21 },
		{ "crusader peco",            21 },
		{ "crusader-peco",            21 },
		{ "crusader_peco",            21 },
		{ "super novice",             23 },
		{ "super-novice",             23 },
		{ "super_novice",             23 },
		{ "supernovice",              23 },
		{ "gunslinger",               24 },
		{ "gunner",                   24 },
		{ "ninja",                    25 },
		{ "xmas",                     26 },
		{ "santa",                    26 },
		{ "novice high",            4001 },
		{ "novice_high",            4001 },
		{ "high novice",            4001 },
		{ "high_novice",            4001 },
		{ "swordsman high",         4002 },
		{ "swordsman_high",         4002 },
		{ "high swordsman",         4002 },
		{ "high_swordsman",         4002 },
		{ "swordman high",          4002 },
		{ "swordman_high",          4002 },
		{ "high swordman",          4002 },
		{ "high_swordman",          4002 },
		{ "mage high",              4003 },
		{ "mage_high",              4003 },
		{ "high mage",              4003 },
		{ "high_mage",              4003 },
		{ "archer high",            4004 },
		{ "archer_high",            4004 },
		{ "high archer",            4004 },
		{ "high_archer",            4004 },
		{ "acolyte high",           4005 },
		{ "acolyte_high",           4005 },
		{ "high acolyte",           4005 },
		{ "high_acolyte",           4005 },
		{ "merchant high",          4006 },
		{ "merchant_high",          4006 },
		{ "high merchant",          4006 },
		{ "high_merchant",          4006 },
		{ "thief high",             4007 },
		{ "thief_high",             4007 },
		{ "high thief",             4007 },
		{ "high_thief",             4007 },
		{ "lord knight",            4008 },
		{ "lord_knight",            4008 },
		{ "high priest",            4009 },
		{ "high_priest",            4009 },
		{ "high priestess",         4009 },
		{ "high_priestess",         4009 },
		{ "high wizard",            4010 },
		{ "high_wizard",            4010 },
		{ "whitesmith",             4011 },
		{ "white smith",            4011 },
		{ "white_smith",            4011 },
		{ "white-smith",            4011 },
		{ "mastersmith",            4011 },
		{ "master smith",           4011 },
		{ "master_smith",           4011 },
		{ "master-smith",           4011 },
		{ "sniper",                 4012 },
		{ "assassin cross",         4013 },
		{ "assassin_cross",         4013 },
		{ "assassin-cross",         4013 },
		{ "peco lord knight",       4014 },
		{ "peco-lord-knight",       4014 },
		{ "peco_lord_knight",       4014 },
		{ "lord knight peco",       4014 },
		{ "lord-knight-peco",       4014 },
		{ "lord_knight_peco",       4014 },
		{ "paladin",                4015 },
		{ "champion",               4016 },
		{ "scholar",                4017 },
		{ "professor",              4017 },
		{ "stalker",                4018 },
		{ "biochemist",             4019 },
		{ "creator",                4019 },
		{ "clown",                  4020 },
		{ "minstrel",               4020 },
		{ "gypsy",                  4021 },
		{ "peco paladin",           4022 },
		{ "peco-paladin",           4022 },
		{ "peco_paladin",           4022 },
		{ "paladin peco",           4022 },
		{ "paladin-peco",           4022 },
		{ "paladin_peco",           4022 },
		{ "baby novice",            4023 },
		{ "baby_novice",            4023 },
		{ "baby swordsman",         4024 },
		{ "baby_swordsman",         4024 },
		{ "baby swordman",          4024 },
		{ "baby_swordman",          4024 },
		{ "baby mage",              4025 },
		{ "baby_mage",              4025 },
		{ "baby archer",            4026 },
		{ "baby_archer",            4026 },
		{ "baby acolyte",           4027 },
		{ "baby_acolyte",           4027 },
		{ "baby merchant",          4028 },
		{ "baby_merchant",          4028 },
		{ "baby thief",             4029 },
		{ "baby_thief",             4029 },
		{ "baby knight",            4030 },
		{ "baby_knight",            4030 },
		{ "baby priest",            4031 },
		{ "baby_priest",            4031 },
		{ "baby priestess",         4031 },
		{ "baby_priestess",         4031 },
		{ "baby wizard",            4032 },
		{ "baby_wizard",            4032 },
		{ "baby blacksmith",        4033 },
		{ "baby_blacksmith",        4033 },
		{ "baby hunter",            4034 },
		{ "baby_hunter",            4034 },
		{ "baby assassin",          4035 },
		{ "baby_assassin",          4035 },
		{ "baby peco knight",       4036 },
		{ "baby peco-knight",       4036 },
		{ "baby peco_knight",       4036 },
		{ "baby_peco_knight",       4036 },
		{ "baby crusader",          4037 },
		{ "baby_crusader",          4037 },
		{ "baby defender",          4037 },
		{ "baby_defender",          4037 },
		{ "baby monk",              4038 },
		{ "baby_monk",              4038 },
		{ "baby sage",              4039 },
		{ "baby_sage",              4039 },
		{ "baby rogue",             4040 },
		{ "baby_rogue",             4040 },
		{ "baby alchemist",         4041 },
		{ "baby_alchemist",         4041 },
		{ "baby bard",              4042 },
		{ "baby_bard",              4042 },
		{ "baby dancer",            4043 },
		{ "baby_dancer",            4043 },
		{ "baby peco crusader",     4044 },
		{ "baby peco-crusader",     4044 },
		{ "baby peco_crusader",     4044 },
		{ "baby_peco_crusader",     4044 },
		{ "super baby",             4045 },
		{ "super-baby",             4045 },
		{ "super_baby",             4045 },
		{ "baby supernovice",       4045 },
		{ "baby super-novice",      4045 },
		{ "baby super novice",      4045 },
		{ "baby_supernovice",       4045 },
		{ "baby_super_novice",      4045 },
		{ "taekwon",                4046 },
		{ "taekwon kid",            4046 },
		{ "taekwon_kid",            4046 },
		{ "taekwon boy",            4046 },
		{ "taekwon_boy",            4046 },
		{ "taekwon girl",           4046 },
		{ "taekwon_girl",           4046 },
		{ "star gladiator",         4047 },
		{ "star_gladiator",         4047 },
		{ "star knight",            4047 },
		{ "star_knight",            4047 },
		{ "taekwon master",         4047 },
		{ "taekwon_master",         4047 },
		{ "star gladiator2",        4048 },
		{ "star_gladiator2",        4048 },
		{ "taekwon master2",        4048 },
		{ "taekwon_master2",        4048 },
		{ "soul linker",            4049 },
		{ "soul_linker",            4049 },
		{ "bon gun",                4050 },
		{ "bon_gun",                4050 },
		{ "bongun",                 4050 },
		{ "death knight",           4051 },
		{ "death_knight",           4051 },
		{ "dark collector",         4052 },
		{ "dark_collector",         4052 },
		{ "monster breeder",        4052 },
		{ "monster_breeder",        4052 },
		{ "munak",                  4053 },
	};

	if (!message || !*message || sscanf(message, "%d %d %[^\n]", &job, &upper, atcmd_name) < 3 || job < 0 || job >= MAX_PC_CLASS || (job > 26 && job < 4001)) {
		upper = -1;
		if (!message || !*message || sscanf(message, "%d %[^\n]", &job, atcmd_name) < 2 || job < 0 || job >= MAX_PC_CLASS || (job > 26 && job < 4001)) {
			// Search class name
			i = (int)(sizeof(jobs) / sizeof(jobs[0]));
			if (sscanf(message, "\"%[^\"]\" %d %[^\n]", jobname, &upper, atcmd_name) == 3 ||
			    sscanf(message, "%s %d %[^\n]", jobname, &upper, atcmd_name) == 3) {
				for (i = 0; i < (int)(sizeof(jobs) / sizeof(jobs[0])); i++) {
					if (strcasecmp(jobname, jobs[i].name) == 0) {
						job = jobs[i].id;
						break;
					}
				}
			}
			// If class name not found (With 3 parameters), try with 2 parameters
			if ((i == (int)(sizeof(jobs) / sizeof(jobs[0])))) {
				upper = -1;
				if (sscanf(message, "\"%[^\"]\" %[^\n]", jobname, atcmd_name) == 2 ||
				    sscanf(message, "%s %[^\n]", jobname, atcmd_name) == 2) {
					for (i = 0; i < (int)(sizeof(jobs) / sizeof(jobs[0])); i++) {
						if (strcasecmp(jobname, jobs[i].name) == 0) {
							job = jobs[i].id;
							break;
						}
					}
				}
	// If class name not found
	if ((i == (int)(sizeof(jobs) / sizeof(jobs[0])))) {
		send_usage(sd, "Please, enter a valid job ID (usage: %s <job ID|\"job name\"> [upper]).", original_command);
		send_usage(sd, "   0 Novice            7 Knight           14 Crusader         21 Peco Crusader");
		send_usage(sd, "   1 Swordman          8 Priest           15 Monk             22 N/A");
		send_usage(sd, "   2 Mage              9 Wizard           16 Sage             23 Super Novice");
		send_usage(sd, "   3 Archer           10 Blacksmith       17 Rogue            24 Gunslinger");
		send_usage(sd, "   4 Acolyte          11 Hunter           18 Alchemist        25 Ninja");
		send_usage(sd, "   5 Merchant         12 Assassin         19 Bard             26 N/A");
		send_usage(sd, "   6 Thief            13 Peco Knight      20 Dancer");
		send_usage(sd, "4001 Novice High    4008 Lord Knight      4015 Paladin        4022 Peco Paladin");
		send_usage(sd, "4002 Swordman High  4009 High Priest      4016 Champion");
		send_usage(sd, "4003 Mage High      4010 High Wizard      4017 Professor");
		send_usage(sd, "4004 Archer High    4011 Whitesmith       4018 Stalker");
		send_usage(sd, "4005 Acolyte High   4012 Sniper           4019 Creator");
		send_usage(sd, "4006 Merchant High  4013 Assassin Cross   4020 Clown");
		send_usage(sd, "4007 Thief High     4014 Peco Lord Knight 4021 Gypsy");
		send_usage(sd, "4023 Baby Novice    4030 Baby Knight      4037 Baby Crusader  4044 Baby Peco Crusader");
		send_usage(sd, "4024 Baby Swordsman 4031 Baby Priest      4038 Baby Monk      4045 Super Baby");
		send_usage(sd, "4025 Baby Mage      4032 Baby Wizard      4039 Baby Sage      4046 Taekwon Kid");
		send_usage(sd, "4026 Baby Archer    4033 Baby Blacksmith  4040 Baby Rogue     4047 Taekwon Master");
		send_usage(sd, "4027 Baby Acolyte   4034 Baby Hunter      4041 Baby Alchemist 4048 N/A");
		send_usage(sd, "4028 Baby Merchant  4035 Baby Assassin    4042 Baby Bard      4049 Soul Linker");
		send_usage(sd, "4029 Baby Thief     4036 Baby Peco-Knight 4043 Baby Dancer");
		send_usage(sd, "[upper]: -1 (default) to automatically determine the 'level', 0 to force normal job, 1 to force high job.");
			return -1;
				}
			}
		}
	}

	if ((pl_sd = map_nick2sd(atcmd_name)) != NULL || ((pl_sd = map_id2sd(atoi(atcmd_name))) != NULL && pl_sd->state.auth)) {
		if (sd->GM_level >= pl_sd->GM_level) { // Only lower or same GM level
			if (job == pl_sd->status.class) {
				clif_displaymessage(fd, "The character's job is already the wished job.");
				return -1;
			}

			// Fix Peco display
			if ((job != 13 && job != 21 && job != 4014 && job != 4022 && job != 4036 && job != 4044)) {
				if (pc_isriding(sd)) {
					// Normal classes
					if (pl_sd->status.class == 13)
						pl_sd->status.class = pl_sd->view_class = 7;
					else if (pl_sd->status.class == 21)
						pl_sd->status.class = pl_sd->view_class = 14;
					// Advanced classes
					else if (pl_sd->status.class == 4014)
						pl_sd->status.class = pl_sd->view_class = 4008;
					else if (pl_sd->status.class == 4022)
						pl_sd->status.class = pl_sd->view_class = 4015;
					// Baby classes
					else if (pl_sd->status.class == 4036)
						pl_sd->status.class = pl_sd->view_class = 4030;
					else if (pl_sd->status.class == 4044)
						pl_sd->status.class = pl_sd->view_class = 4037;
					pl_sd->status.option &= ~0x0020;
					clif_changeoption(&pl_sd->bl);
					status_calc_pc(pl_sd, 0);
				}
			} else {
				if (!pc_isriding(sd)) {
					// Normal classes
					if (job == 13)
						job = 7;
					else if (job == 21)
						job = 14;
					// Advanced classes
					else if (job == 4014)
						job = 4008;
					else if (job == 4022)
						job = 4015;
					// Baby classes
					else if (job == 4036)
						job = 4030;
					else if (job == 4044)
						job = 4037;
				}
			}

			flag = 0;
			for (i = 0; i < MAX_INVENTORY; i++)
				if (pl_sd->status.inventory[i].nameid && (pl_sd->status.inventory[i].equip & 34) != 0) { // Right hand (2) + left hand (32)
					pc_unequipitem(pl_sd, i, 3); // Unequip weapon to avoid sprite error
					flag = 1;
				}
			if (flag)
				clif_displaymessage(pl_sd->fd, "Weapon unequiped to avoid sprite error.");

			if (pc_jobchange(pl_sd, job, upper) == 0) {
				if (pl_sd != sd)
					clif_displaymessage(pl_sd->fd, "Your job has been changed by a GM.");
				clif_displaymessage(fd, msg_txt(48)); // Character's job changed.
			} else {
				clif_displaymessage(fd, msg_txt(192)); // Impossible to change the character's job.
				return -1;
			}
		} else {
			clif_displaymessage(fd, msg_txt(81)); // Your GM level don't authorize you to do this action on this player.
			return -1;
		}
	} else {
		clif_displaymessage(fd, msg_txt(3)); // Character not found.
		return -1;
	}

	return 0;
}

/*==========================================
 * @charchangelevel - Character set the level when the player changed of job (job 1 -> job 2)
 *------------------------------------------
 */
ATCOMMAND_FUNC(change_level) {
	struct map_session_data *pl_sd;
	int level;

	if (!message || !*message || sscanf(message, "%d %[^\n]", &level, atcmd_name) < 2 || level < 40 || level > 50) {
		send_usage(sd, "Please, enter a right level and a player name (usage: %s <lvl:40-50> <char name|account_id>).", original_command);
		return -1;
	}

	if ((pl_sd = map_nick2sd(atcmd_name)) != NULL || ((pl_sd = map_id2sd(atoi(atcmd_name))) != NULL && pl_sd->state.auth)) {
		if (sd->GM_level >= pl_sd->GM_level) { // Only lower or same gm level
			if (pl_sd->change_level != level) {
				pl_sd->change_level = level;
				pc_setglobalreg(pl_sd, "jobchange_level", pl_sd->change_level);
				// Save player immediatly (Synchronize with global_reg)
				chrif_save(pl_sd); // do pc_makesavestatus and save storage + account_reg/account_reg2 too
				// Recalculate skill tree
				status_calc_pc(pl_sd, 0);
				clif_displaymessage(fd, "Player's end level of job 1 changed!");
			} else {
				clif_displaymessage(fd, "Player already had this level when he became job 2.");
				return -1;
			}
		} else {
			clif_displaymessage(fd, msg_txt(81)); // Your GM level don't authorize you to do this action on this player.
			return -1;
		}
	} else {
		clif_displaymessage(fd, msg_txt(3)); // Character not found.
		return -1;
	}

	return 0;
}

/*==========================================
 * @revive - Revives a target
 *------------------------------------------
 */
ATCOMMAND_FUNC(revive) {
	struct map_session_data *pl_sd;

	if (!message || !*message || sscanf(message, "%[^\n]", atcmd_name) < 1) {
		send_usage(sd, "Please, enter a player name (usage: %s <char name|account_id>).", original_command);
		return -1;
	}

	if ((pl_sd = map_nick2sd(atcmd_name)) != NULL || ((pl_sd = map_id2sd(atoi(atcmd_name))) != NULL && pl_sd->state.auth)) {
		// If not dead, call @charheal command
		if (!pc_isdead(pl_sd)) {
			char message2[105]; // '0 0 ' (4) + message (max 100) + NULL (1) = 105
			// Preparation of message
			sprintf(message2, "0 0 %s", message);
			// Call refine function
			return atcommand_charheal(fd, sd, original_command, "@charheal", message2); // Use local buffer to avoid problem (We call another function that can use global variables)
		}
		pl_sd->status.hp = pl_sd->status.max_hp;
		pl_sd->status.sp = pl_sd->status.max_sp;
		clif_skill_nodamage(&pl_sd->bl, &pl_sd->bl, ALL_RESURRECTION, 4, 1);
		pc_setstand(pl_sd);
		if (battle_config.pc_invincible_time > 0)
			pc_setinvincibletimer(sd, battle_config.pc_invincible_time);
		clif_updatestatus(pl_sd, SP_HP);
		clif_updatestatus(pl_sd, SP_SP);
		clif_resurrection(&pl_sd->bl, 1);
		clif_displaymessage(pl_sd->fd, msg_txt(16)); // You've been revived! It's a miracle!
		clif_displaymessage(fd, msg_txt(51)); // Character revived.
	} else {
		clif_displaymessage(fd, msg_txt(3)); // Character not found.
		return -1;
	}

	return 0;
}

/*==========================================
 * @charheal - Fully heals a target
 *------------------------------------------
 */
ATCOMMAND_FUNC(charheal) {
	int hp = 0, sp = 0;
	struct map_session_data *pl_sd;

	if (!message || !*message || sscanf(message, "%d %d %[^\n]", &hp, &sp, atcmd_name) < 3) {
		sp = 0;
		if (!message || !*message || sscanf(message, "%d %[^\n]", &hp, atcmd_name) < 2) {
			hp = 0;
			if (!message || !*message || sscanf(message, "%[^\n]", atcmd_name) < 1) {
				send_usage(sd, "Please, enter a player name (usage: %s [<HP> [<SP>]] <char name|account_id>).", original_command);
				return -1;
			}
		}
	}

	if ((pl_sd = map_nick2sd(atcmd_name)) != NULL || ((pl_sd = map_id2sd(atoi(atcmd_name))) != NULL && pl_sd->state.auth)) {
		// If dead
		if (pc_isdead(pl_sd)) {
			clif_displaymessage(fd, "This player is dead. You must resurrect him/her before to heal him/her.");
			return -1;
		}

		if (hp == 0 && sp == 0) {
			hp = pl_sd->status.max_hp - pl_sd->status.hp;
			sp = pl_sd->status.max_sp - pl_sd->status.sp;
		} else {
			if (hp > 0 && (hp > pl_sd->status.max_hp || hp > (pl_sd->status.max_hp - pl_sd->status.hp))) // Fix positive overflow
				hp = sd->status.max_hp - sd->status.hp;
			else if (hp < 0 && (hp < -pl_sd->status.max_hp || hp < (1 - pl_sd->status.hp))) // Fix negative overflow
				hp = 1 - pl_sd->status.hp;
			if (sp > 0 && (sp > pl_sd->status.max_sp || sp > (pl_sd->status.max_sp - pl_sd->status.sp))) // Fix positive overflow
				sp = pl_sd->status.max_sp - pl_sd->status.sp;
			else if (sp < 0 && (sp < -pl_sd->status.max_sp || sp < (1 - pl_sd->status.sp))) // Fix negative overflow
				sp = 1 - pl_sd->status.sp;
		}

		if (hp > 0) // Display like heal
			clif_heal(pl_sd->fd, SP_HP, hp);
		else if (hp < 0) // Display like damage
			clif_damage(&pl_sd->bl, &pl_sd->bl, gettick_cache, 0, 0, -hp, 0 , 4, 0);
		if (sp > 0) // No display if we lost SP
			clif_heal(pl_sd->fd, SP_SP, sp);

		if (hp != 0 || sp != 0) {
			pc_heal(pl_sd, hp, sp);
			if (hp >= 0 && sp >= 0)
				clif_displaymessage(fd, "HP, SP of the player recovered.");
			else
				clif_displaymessage(fd, "HP or/and SP of the player modified.");
		} else {
			clif_displaymessage(fd, "HP and SP of the player are already with the good value.");
			return -1;
		}

	} else {
		clif_displaymessage(fd, msg_txt(3)); // Character not found.
		return -1;
	}

	return 0;
}

/*==========================================
 * @charstats - Displays a target's status information
 *------------------------------------------
 */
ATCOMMAND_FUNC(character_stats) {
	struct map_session_data *pl_sd;
	struct guild *g;
	struct party *p;
	int i;

	if (!message || !*message || sscanf(message, "%[^\n]", atcmd_name) < 1) {
		send_usage(sd, "Please, enter a player name (usage: %s <char name|account_id>).", original_command);
		return -1;
	}

	if ((pl_sd = map_nick2sd(atcmd_name)) != NULL || ((pl_sd = map_id2sd(atoi(atcmd_name))) != NULL && pl_sd->state.auth)) {
		struct {
			const char* format;
			int value;
		} output_table[] = {
			{ "Str: %d",  pl_sd->status.str },
			{ "Agi: %d",  pl_sd->status.agi },
			{ "Vit: %d",  pl_sd->status.vit },
			{ "Int: %d",  pl_sd->status.int_ },
			{ "Dex: %d",  pl_sd->status.dex },
			{ "Luk: %d",  pl_sd->status.luk },
			{ "Zeny: %d", pl_sd->status.zeny },
			{ NULL, 0 }
		};

		if (pl_sd->GM_level > 0)
			sprintf(atcmd_output, "'%s' (GM:%d) (account: %d) stats:",   pl_sd->status.name, pl_sd->GM_level, pl_sd->status.account_id);
		else
			sprintf(atcmd_output, msg_txt(53),             pl_sd->status.name, pl_sd->status.account_id); // '%s' (account: %d) stats:
		clif_displaymessage(fd, atcmd_output);
		
		sprintf(atcmd_output, "Job: %s (level %d/%d)",     job_name(pl_sd->status.class), pl_sd->status.base_level, pl_sd->status.job_level);
		clif_displaymessage(fd, atcmd_output);
		
		sprintf(atcmd_output, "Location: %s %d %d",        pl_sd->mapname, pl_sd->bl.x, pl_sd->bl.y);
		clif_displaymessage(fd, atcmd_output);
		
		p = party_search(pl_sd->status.party_id);
		g = guild_search(pl_sd->status.guild_id);
		sprintf(atcmd_output, "Party: '%s' | Guild: '%s'", (p == NULL) ? "None" : p->name, (g == NULL) ? "None" : g->name);
		clif_displaymessage(fd, atcmd_output);
		
		sprintf(atcmd_output, "Hp: %d/%d",                 pl_sd->status.hp, pl_sd->status.max_hp);
		clif_displaymessage(fd, atcmd_output);
		
		sprintf(atcmd_output, "Sp: %d/%d",                 pl_sd->status.sp, pl_sd->status.max_sp);
		clif_displaymessage(fd, atcmd_output);
		
		for (i = 0; output_table[i].format != NULL; i++) {
			sprintf(atcmd_output, output_table[i].format,  output_table[i].value);
			clif_displaymessage(fd, atcmd_output);
		}
		
		sprintf(atcmd_output, "Speed: %d",                 pl_sd->speed);
		clif_displaymessage(fd, atcmd_output);
	
	} else {
		clif_displaymessage(fd, msg_txt(3)); // Character not found.
		return -1;
	}

	return 0;
}

/*==========================================
 * @charstatsall - Character Stats All
 *------------------------------------------
 */
ATCOMMAND_FUNC(character_stats_all) {
	int i, count;
	struct map_session_data *pl_sd;

	count = 0;
	for(i = 0; i < fd_max; i++) {
		if (session[i] && (pl_sd = session[i]->session_data) && pl_sd->state.auth) {
			sprintf(atcmd_output, "Name: %s | BLvl: %d | Job: %s (Lvl: %d) | HP: %d/%d | SP: %d/%d", pl_sd->status.name, pl_sd->status.base_level, job_name(pl_sd->status.class), pl_sd->status.job_level, pl_sd->status.hp, pl_sd->status.max_hp, pl_sd->status.sp, pl_sd->status.max_sp);
			clif_displaymessage(fd, atcmd_output);
			if (pl_sd->GM_level > 0)
				sprintf(atcmd_output, "STR: %d | AGI: %d | VIT: %d | INT: %d | DEX: %d | LUK: %d | Zeny: %d | GM Lvl: %d", pl_sd->status.str, pl_sd->status.agi, pl_sd->status.vit, pl_sd->status.int_, pl_sd->status.dex, pl_sd->status.luk, pl_sd->status.zeny, pl_sd->GM_level);
			else
				sprintf(atcmd_output, "STR: %d | AGI: %d | VIT: %d | INT: %d | DEX: %d | LUK: %d | Zeny: %d", pl_sd->status.str, pl_sd->status.agi, pl_sd->status.vit, pl_sd->status.int_, pl_sd->status.dex, pl_sd->status.luk, pl_sd->status.zeny);
			clif_displaymessage(fd, atcmd_output);
			clif_displaymessage(fd, "--------");
			count++;
		}
	}

	if (count == 0)
		clif_displaymessage(fd, msg_txt(28)); // No player found.
	else if (count == 1)
		clif_displaymessage(fd, msg_txt(29)); // 1 player found.
	else {
		sprintf(atcmd_output, msg_txt(30), count); // %d players found.
		clif_displaymessage(fd, atcmd_output);
	}

	return 0;
}

/*==========================================
 * @charoption
 *------------------------------------------
 */
ATCOMMAND_FUNC(character_option) {
	int opt1 = 0, opt2 = 0, opt3 = 0;
	struct map_session_data* pl_sd;

	if (!message || !*message || sscanf(message, "%d %d %d %[^\n]", &opt1, &opt2, &opt3, atcmd_name) < 4 || opt1 < 0 || opt2 < 0 || opt3 < 0) {
		send_usage(sd, "Please, enter valid options and a player name (usage: %s <opt1:0+> <opt2:0+> <opt3:0+> <char name|account_id>).", original_command);
		return -1;
	}

	if ((pl_sd = map_nick2sd(atcmd_name)) != NULL || ((pl_sd = map_id2sd(atoi(atcmd_name))) != NULL && pl_sd->state.auth)) {
		if (sd->GM_level >= pl_sd->GM_level) { // Only lower or same GM level
			pl_sd->opt1 = opt1;
			pl_sd->opt2 = opt2;
			if (!(pl_sd->status.option & CART_MASK) && (opt3 & CART_MASK)) {
				clif_cart_itemlist(pl_sd);
				clif_cart_equiplist(pl_sd);
				clif_updatestatus(pl_sd, SP_CARTINFO);
			}
			pl_sd->status.option = opt3;
			// Fix Peco display
			if (pl_sd->status.class ==   13 || pl_sd->status.class ==   21 ||
			    pl_sd->status.class == 4014 || pl_sd->status.class == 4022 ||
			    pl_sd->status.class == 4036 || pl_sd->status.class == 4044) {
				if (!pc_isriding(pl_sd)) { // pl_sd has the new value...
					// Normal classes
					if (pl_sd->status.class == 13)
						pl_sd->status.class = pl_sd->view_class = 7;
					else if (pl_sd->status.class == 21)
						pl_sd->status.class = pl_sd->view_class = 14;
					// Advanced classes
					else if (pl_sd->status.class == 4014)
						pl_sd->status.class = pl_sd->view_class = 4008;
					else if (pl_sd->status.class == 4022)
						pl_sd->status.class = pl_sd->view_class = 4015;
					// Baby classes
					else if (pl_sd->status.class == 4036)
						pl_sd->status.class = pl_sd->view_class = 4030;
					else if (pl_sd->status.class == 4044)
						pl_sd->status.class = pl_sd->view_class = 4037;
				}
			} else {
				if (pc_isriding(pl_sd)) { // pl_sd has the new value...
					if (pl_sd->disguise > 0) { // Temporary prevention of crash caused by Peco + disguise
						pl_sd->status.option &= ~0x0020;
					} else {
						// Normal classes
						if (pl_sd->status.class == 7)
							pl_sd->status.class = pl_sd->view_class = 13;
						else if (pl_sd->status.class == 14)
							pl_sd->status.class = pl_sd->view_class = 21;
						// Advanced classes
						else if (pl_sd->status.class == 4008)
							pl_sd->status.class = pl_sd->view_class = 4014;
						else if (pl_sd->status.class == 4015)
							pl_sd->status.class = pl_sd->view_class = 4022;
						// Baby classes
						else if (pl_sd->status.class == 4030)
							pl_sd->status.class = pl_sd->view_class = 4036;
						else if (pl_sd->status.class == 4037)
							pl_sd->status.class = pl_sd->view_class = 4044;
						else
							pl_sd->status.option &= ~0x0020;
					}
				}
			}
			clif_changeoption(&pl_sd->bl);
			status_calc_pc(pl_sd, 0);
			clif_displaymessage(fd, msg_txt(58)); // Character's options changed.
			if (pc_isfalcon(pl_sd) && pc_iscarton(pl_sd) && (pl_sd->status.option & 0x0008) != 0x0008)
				clif_displaymessage(fd, "Falcon can display all carts like a basic cart (type 1).");
		} else {
			clif_displaymessage(fd, msg_txt(81)); // Your GM level don't authorize you to do this action on this player.
			return -1;
		}
	} else {
		clif_displaymessage(fd, msg_txt(3)); // Character not found.
		return -1;
	}

	return 0;
}

/*==========================================
 * @charoptionadd
 *------------------------------------------
 */
ATCOMMAND_FUNC(character_optionadd) {
	int opt1 = 0, opt2 = 0, opt3 = 0;
	struct map_session_data* pl_sd;

	if (!message || !*message || sscanf(message, "%d %d %d %[^\n]", &opt1, &opt2, &opt3, atcmd_name) < 4 || opt1 < 0 || opt2 < 0 || opt3 < 0 || (opt1 == 0 && opt2 == 0 && opt3 == 0)) {
		send_usage(sd, "Please, enter valid options and a player name (usage: %s <opt1:0+> <opt2:0+> <opt3:0+> <char name|account_id>).", original_command);
		return -1;
	}

	if ((pl_sd = map_nick2sd(atcmd_name)) != NULL || ((pl_sd = map_id2sd(atoi(atcmd_name))) != NULL && pl_sd->state.auth)) {
		if (sd->GM_level >= pl_sd->GM_level) { // Only lower or same GM level
			pl_sd->opt1 |= opt1;
			pl_sd->opt2 |= opt2;
			if (!(pl_sd->status.option & CART_MASK) && (opt3 & CART_MASK)) {
				clif_cart_itemlist(pl_sd);
				clif_cart_equiplist(pl_sd);
				clif_updatestatus(pl_sd, SP_CARTINFO);
			}
			pl_sd->status.option |= opt3;
			// Fix Peco display
			if (pl_sd->status.class ==   13 || pl_sd->status.class ==   21 ||
			    pl_sd->status.class == 4014 || pl_sd->status.class == 4022 ||
			    pl_sd->status.class == 4036 || pl_sd->status.class == 4044) {
				if (!pc_isriding(pl_sd)) { // pl_sd has the new value...
					// Normal classes
					if (pl_sd->status.class == 13)
						pl_sd->status.class = pl_sd->view_class = 7;
					else if (pl_sd->status.class == 21)
						pl_sd->status.class = pl_sd->view_class = 14;
					// High classes
					else if (pl_sd->status.class == 4014)
						pl_sd->status.class = pl_sd->view_class = 4008;
					else if (pl_sd->status.class == 4022)
						pl_sd->status.class = pl_sd->view_class = 4015;
					// Baby classes
					else if (pl_sd->status.class == 4036)
						pl_sd->status.class = pl_sd->view_class = 4030;
					else if (pl_sd->status.class == 4044)
						pl_sd->status.class = pl_sd->view_class = 4037;
				}
			} else {
				if (pc_isriding(pl_sd)) { // pl_sd has the new value...
					if (pl_sd->disguise > 0) { // Temporary prevention of crash caused by peco + disguise
						pl_sd->status.option &= ~0x0020;
					} else {
						// Normal classes
						if (pl_sd->status.class == 7)
							pl_sd->status.class = pl_sd->view_class = 13;
						else if (pl_sd->status.class == 14)
							pl_sd->status.class = pl_sd->view_class = 21;
						// High classes
						else if (pl_sd->status.class == 4008)
							pl_sd->status.class = pl_sd->view_class = 4014;
						else if (pl_sd->status.class == 4015)
							pl_sd->status.class = pl_sd->view_class = 4022;
						// Baby classes
						else if (pl_sd->status.class == 4030)
							pl_sd->status.class = pl_sd->view_class = 4036;
						else if (pl_sd->status.class == 4037)
							pl_sd->status.class = pl_sd->view_class = 4044;
						else
							pl_sd->status.option &= ~0x0020;
					}
				}
			}
			clif_changeoption(&pl_sd->bl);
			status_calc_pc(pl_sd, 0);
			clif_displaymessage(fd, msg_txt(58)); // Character's options changed.
			if (pc_isfalcon(pl_sd) && pc_iscarton(pl_sd) && (pl_sd->status.option & 0x0008) != 0x0008)
				clif_displaymessage(fd, "Falcon can display all carts like a basic cart (type 1).");
		} else {
			clif_displaymessage(fd, msg_txt(81)); // Your GM level don't authorize you to do this action on this player.
			return -1;
		}
	} else {
		clif_displaymessage(fd, msg_txt(3)); // Character not found.
		return -1;
	}

	return 0;
}

/*==========================================
 * @charoptionremove
 *------------------------------------------
 */
ATCOMMAND_FUNC(character_optionremove) {
	int opt1 = 0, opt2 = 0, opt3 = 0;
	struct map_session_data* pl_sd;

	if (!message || !*message || sscanf(message, "%d %d %d %[^\n]", &opt1, &opt2, &opt3, atcmd_name) < 4 || opt1 < 0 || opt2 < 0 || opt3 < 0 || (opt1 == 0 && opt2 == 0 && opt3 == 0)) {
		send_usage(sd, "Please, enter valid options and a player name (usage: %s <opt1:0+> <opt2:0+> <opt3:0+> <char name|account_id>).", original_command);
		return -1;
	}

	if ((pl_sd = map_nick2sd(atcmd_name)) != NULL || ((pl_sd = map_id2sd(atoi(atcmd_name))) != NULL && pl_sd->state.auth)) {
		if (sd->GM_level >= pl_sd->GM_level) { // Only lower or same GM level
			pl_sd->opt1 &= ~opt1;
			pl_sd->opt2 &= ~opt2;
			pl_sd->status.option &= ~opt3;
			// Fix Peco display
			if (pl_sd->status.class ==   13 || pl_sd->status.class ==   21 ||
			    pl_sd->status.class == 4014 || pl_sd->status.class == 4022 ||
			    pl_sd->status.class == 4036 || pl_sd->status.class == 4044) {
				if (!pc_isriding(pl_sd)) { // pl_sd has the new value...
					// Normal classes
					if (pl_sd->status.class == 13)
						pl_sd->status.class = pl_sd->view_class = 7;
					else if (pl_sd->status.class == 21)
						pl_sd->status.class = pl_sd->view_class = 14;
					// Advanced classes
					else if (pl_sd->status.class == 4014)
						pl_sd->status.class = pl_sd->view_class = 4008;
					else if (pl_sd->status.class == 4022)
						pl_sd->status.class = pl_sd->view_class = 4015;
					// Baby classes
					else if (pl_sd->status.class == 4036)
						pl_sd->status.class = pl_sd->view_class = 4030;
					else if (pl_sd->status.class == 4044)
						pl_sd->status.class = pl_sd->view_class = 4037;
				}
			} else {
				if (pc_isriding(pl_sd)) { // pl_sd has the new value...
					if (pl_sd->disguise > 0) { // Temporary prevention of crash caused by Peco + disguise
						pl_sd->status.option &= ~0x0020;
					} else {
						// Normal classes
						if (pl_sd->status.class == 7)
							pl_sd->status.class = pl_sd->view_class = 13;
						else if (pl_sd->status.class == 14)
							pl_sd->status.class = pl_sd->view_class = 21;
						// Advanced classes
						else if (pl_sd->status.class == 4008)
							pl_sd->status.class = pl_sd->view_class = 4014;
						else if (pl_sd->status.class == 4015)
							pl_sd->status.class = pl_sd->view_class = 4022;
						// Baby classes
						else if (pl_sd->status.class == 4030)
							pl_sd->status.class = pl_sd->view_class = 4036;
						else if (pl_sd->status.class == 4037)
							pl_sd->status.class = pl_sd->view_class = 4044;
						else
							pl_sd->status.option &= ~0x0020;
					}
				}
			}
			clif_changeoption(&pl_sd->bl);
			status_calc_pc(pl_sd, 0);
			clif_displaymessage(fd, msg_txt(58)); // Character's options changed.
		} else {
			clif_displaymessage(fd, msg_txt(81)); // Your GM level don't authorize you to do this action on this player.
			return -1;
		}
	} else {
		clif_displaymessage(fd, msg_txt(3)); // Character not found.
		return -1;
	}

	return 0;
}

/*==========================================
 * @changesex - Change user's gender
 *------------------------------------------
 */
ATCOMMAND_FUNC(change_sex) {
	chrif_char_ask_name(sd->status.account_id, sd->status.name, 5, 0, 0, 0, 0, 0, 0); // Type: 5 - changesex
	clif_displaymessage(fd, "Your character name has been sent to char-server to ask it.");

	return 0;
}

/*==========================================
 * @charchangesex - Changes target's gender
 *------------------------------------------
 */
ATCOMMAND_FUNC(char_change_sex) {
	struct map_session_data* pl_sd;

	if (!message || !*message || sscanf(message, "%[^\n]", atcmd_name) < 1) {
		send_usage(sd, "Please, enter a player name (usage: %s <char name|account_id>).", original_command);
		return -1;
	}

	if ((pl_sd = map_id2sd(atoi(atcmd_name))) != NULL && pl_sd->state.auth) {
		memset(atcmd_name, 0, sizeof(atcmd_name));
		strncpy(atcmd_name, pl_sd->status.name, 24);
	}

	// Check player name
	if (strlen(atcmd_name) < 4) {
		clif_displaymessage(fd, msg_txt(86)); // Sorry, but a player name has at least 4 characters.
		return -1;
	} else if (strlen(atcmd_name) > 23) {
		clif_displaymessage(fd, msg_txt(87)); // Sorry, but a player name has 23 characters maximum.
		return -1;
	} else {
		chrif_char_ask_name(sd->status.account_id, atcmd_name, 5, 0, 0, 0, 0, 0, 0); // Type: 5 - changesex
		clif_displaymessage(fd, msg_txt(88)); // Character name sends to char-server to ask it.
	}

	return 0;
}

/*==========================================
 * charblock command (Usage: charblock <player_name>)
 * This command do a definitive ban on a player
 *------------------------------------------
 */
ATCOMMAND_FUNC(char_block) {
	struct map_session_data* pl_sd;

	if (!message || !*message || sscanf(message, "%[^\n]", atcmd_name) < 1) {
		send_usage(sd, "Please, enter a player name (usage: %s <char name|account_id>).", original_command);
		return -1;
	}

	if ((pl_sd = map_id2sd(atoi(atcmd_name))) != NULL && pl_sd->state.auth) {
		memset(atcmd_name, 0, sizeof(atcmd_name));
		strncpy(atcmd_name, pl_sd->status.name, 24);
	}

	// check player name
	if (strlen(atcmd_name) < 4) {
		clif_displaymessage(fd, msg_txt(86)); // Sorry, but a player name has at least 4 characters.
		return -1;
	} else if (strlen(atcmd_name) > 23) {
		clif_displaymessage(fd, msg_txt(87)); // Sorry, but a player name has 23 characters maximum.
		return -1;
	} else {
		chrif_char_ask_name(sd->status.account_id, atcmd_name, 1, 0, 0, 0, 0, 0, 0); // Type: 1 - block
		clif_displaymessage(fd, msg_txt(88)); // Character name sends to char-server to ask it.
	}

	return 0;
}

/*==========================================
 * charban command (Usage: charban <time> <player_name>)
 * This command do a limited ban on a player
 * Time is done as follows:
 *   Adjustment value (-1, 1, +1, etc...)
 *   Modified element:
 *     a or y: year
 *     m:  month
 *     j or d: day
 *     h:  hour
 *     mn: minute
 *     s:  second
 * <example> @ban +1m-2mn1s-6y test_player
 *           this example adds 1 month and 1 second, and substracts 2 minutes and 6 years at the same time.
 *------------------------------------------
 */
ATCOMMAND_FUNC(char_ban) {
	struct map_session_data* pl_sd;
	char modif[100];
	char * modif_p;
	int year, month, day, hour, minute, second, value;

	memset(modif, 0, sizeof(modif));

	if (!message || !*message || sscanf(message, "%s %[^\n]", modif, atcmd_name) < 2) {
		send_usage(sd, "Please, enter ban time and a player name (usage: %s <time> <char name|account_id>).", original_command);
		send_usage(sd, "time usage: adjustement (+/- value) and element (y/a, m, d/j, h, mn, s)");
		send_usage(sd, "Example: %s +1m-2mn1s-6y testplayer", original_command);
		return -1;
	}

	modif_p = modif;
	year = month = day = hour = minute = second = 0;
	while (modif_p[0] != '\0') {
		value = atoi(modif_p);
		if (value == 0)
			modif_p++;
		else {
			if (modif_p[0] == '-' || modif_p[0] == '+')
				modif_p++;
			while (modif_p[0] >= '0' && modif_p[0] <= '9')
				modif_p++;
			if (modif_p[0] == 's') {
				second += value;
				modif_p++;
			} else if (modif_p[0] == 'm' && modif_p[1] == 'n') {
				minute += value;
				modif_p = modif_p + 2;
			} else if (modif_p[0] == 'h') {
				hour += value;
				modif_p++;
			} else if (modif_p[0] == 'd' || modif_p[0] == 'j') {
				day += value;
				modif_p++;
			} else if (modif_p[0] == 'm') {
				month += value;
				modif_p++;
			} else if (modif_p[0] == 'y' || modif_p[0] == 'a') {
				year += value;
				modif_p++;
			} else if (modif_p[0] != '\0') {
				modif_p++;
			}
		}
	}
	if (year == 0 && month == 0 && day == 0 && hour == 0 && minute == 0 && second == 0) {
		clif_displaymessage(fd, msg_txt(85)); // Invalid time for ban command.
		return -1;
	}

	if ((pl_sd = map_id2sd(atoi(atcmd_name))) != NULL && pl_sd->state.auth) {
		memset(atcmd_name, 0, sizeof(atcmd_name));
		strncpy(atcmd_name, pl_sd->status.name, 24);
	}

	// Check player name
	if (strlen(atcmd_name) < 4) {
		clif_displaymessage(fd, msg_txt(86)); // Sorry, but a player name has at least 4 characters.
		return -1;
	} else if (strlen(atcmd_name) > 23) {
		clif_displaymessage(fd, msg_txt(87)); // Sorry, but a player name has 23 characters maximum.
		return -1;
	} else {
		chrif_char_ask_name(sd->status.account_id, atcmd_name, 2, year, month, day, hour, minute, second); // Type: 2 - ban
		clif_displaymessage(fd, msg_txt(88)); // Character name sends to char-server to ask it.
	}

	return 0;
}

/*==========================================
 * charunblock command (Usage: charunblock <player_name>)
 *------------------------------------------
 */
ATCOMMAND_FUNC(char_unblock) {
	struct map_session_data* pl_sd;

	if (!message || !*message || sscanf(message, "%[^\n]", atcmd_name) < 1) {
		send_usage(sd, "Please, enter a player name (usage: %s <char name|account_id>).", original_command);
		return -1;
	}

	if ((pl_sd = map_id2sd(atoi(atcmd_name))) != NULL && pl_sd->state.auth) {
		memset(atcmd_name, 0, sizeof(atcmd_name));
		strncpy(atcmd_name, pl_sd->status.name, 24);
	}

	// Check player name
	if (strlen(atcmd_name) < 4) {
		clif_displaymessage(fd, msg_txt(86)); // Sorry, but a player name has at least 4 characters.
		return -1;
	} else if (strlen(atcmd_name) > 23) {
		clif_displaymessage(fd, msg_txt(87)); // Sorry, but a player name has 23 characters maximum.
		return -1;
	} else {
		// Send answer to login server via char-server
		chrif_char_ask_name(sd->status.account_id, atcmd_name, 3, 0, 0, 0, 0, 0, 0); // Type: 3 - unblock
		clif_displaymessage(fd, msg_txt(88)); // Character name sends to char-server to ask it.
	}

	return 0;
}

/*==========================================
 * charunban command (Usage: charunban <player_name>)
 *------------------------------------------
 */
ATCOMMAND_FUNC(char_unban) {
	struct map_session_data* pl_sd;

	if (!message || !*message || sscanf(message, "%[^\n]", atcmd_name) < 1) {
		send_usage(sd, "Please, enter a player name (usage: %s <char name|account_id>).", original_command);
		return -1;
	}

	if ((pl_sd = map_id2sd(atoi(atcmd_name))) != NULL && pl_sd->state.auth) {
		memset(atcmd_name, 0, sizeof(atcmd_name));
		strncpy(atcmd_name, pl_sd->status.name, 24);
	}

	// Check player name
	if (strlen(atcmd_name) < 4) {
		clif_displaymessage(fd, msg_txt(86)); // Sorry, but a player name has at least 4 characters.
		return -1;
	} else if (strlen(atcmd_name) > 23) {
		clif_displaymessage(fd, msg_txt(87)); // Sorry, but a player name has 23 characters maximum.
		return -1;
	} else {
		// Send answer to login server via char-server
		chrif_char_ask_name(sd->status.account_id, atcmd_name, 4, 0, 0, 0, 0, 0, 0); // Type: 4 - unban
		clif_displaymessage(fd, msg_txt(88)); // Character name sends to char-server to ask it.
	}

	return 0;
}

/*==========================================
 * @charsave - Changes a target's save point
 *------------------------------------------
 */
ATCOMMAND_FUNC(character_save) {
	struct map_session_data* pl_sd;
	int x = 0, y = 0;
	int m;

	if (!message || !*message || sscanf(message, "%s %d %d %[^\n]", atcmd_mapname, &x, &y, atcmd_name) < 4 || x < 0 || y < 0) {
		send_usage(sd, "Please, enter a valid save point and a player name (usage: %s <map> <x> <y> <char name|account_id>).", original_command);
		return -1;
	}

	if (strstr(atcmd_mapname, ".gat") == NULL && strlen(atcmd_mapname) < 13) // 16 - 4 (.gat)
		strcat(atcmd_mapname, ".gat");

	if ((pl_sd = map_nick2sd(atcmd_name)) != NULL || ((pl_sd = map_id2sd(atoi(atcmd_name))) != NULL && pl_sd->state.auth)) {
		if (sd->GM_level >= pl_sd->GM_level) { // Only lower or same GM level
			if ((m = map_checkmapname(atcmd_mapname)) == -1) { // If map doesn't exist in all map-servers
				clif_displaymessage(fd, msg_txt(1)); // Map not found.
				return -1;
			}
			if (x < 0 || y < 0) {
				clif_displaymessage(fd, msg_txt(2)); // Coordinates out of range.
				return -1;
			}
			if (m >= 0) {
				if (map[m].flag.nowarpto && battle_config.any_warp_GM_min_level > sd->GM_level) {
					clif_displaymessage(fd, "You are not authorized to set this map as a save map.");
					return -1;
				}
				if (x >= map[m].xs || y >= map[m].ys) {
					clif_displaymessage(fd, msg_txt(2)); // Coordinates out of range.
					return -1;
				}
				if (map_getcell(m, x, y, CELL_CHKNOPASS)) {
					clif_displaymessage(fd, "Invalid coordinates for a respawn point.");
					return -1;
				}
			}
			pc_setsavepoint(pl_sd, atcmd_mapname, x, y);
			clif_displaymessage(fd, msg_txt(57)); // Character's respawn point changed.
		} else {
			clif_displaymessage(fd, msg_txt(81)); // Your GM level don't authorize you to do this action on this player.
			return -1;
		}
	} else {
		clif_displaymessage(fd, msg_txt(3)); // Character not found.
		return -1;
	}

	return 0;
}

/*==========================================
 * @night - Makes the server night-mode
 *------------------------------------------
 */
ATCOMMAND_FUNC(night) {
	struct map_session_data *pl_sd;
	int i, msglen;

	if (night_flag != 1) {
		night_flag = 1; // 0=Day, 1=Night
		msglen = strlen(msg_txt(59)); // Night has fallen.
		for(i = 0; i < fd_max; i++) {
			if (session[i] && (pl_sd = session[i]->session_data) && pl_sd->state.auth && !map[pl_sd->bl.m].flag.indoors) {
				clif_status_change(&pl_sd->bl, ICO_NIGHT, 0);
				clif_status_change(&pl_sd->bl, ICO_NIGHT, 1);
				clif_wis_message(pl_sd->fd, wisp_server_name, msg_txt(59), msglen + 1); // Night has fallen.
			}
		}
	} else {
		clif_displaymessage(fd, msg_txt(89)); // Sorry, it's already the night. Impossible to execute the command.
		return -1;
	}

	return 0;
}

/*==========================================
 * @day - Makes the server day-mode
 *------------------------------------------
 */
ATCOMMAND_FUNC(day) {
	struct map_session_data *pl_sd;
	int i, msglen;

	if (night_flag != 0) {
		night_flag = 0; // 0=Day, 1=Night
		msglen = strlen(msg_txt(60)); // Day has arrived.
		for(i = 0; i < fd_max; i++) {
			if (session[i] && (pl_sd = session[i]->session_data) && pl_sd->state.auth) {
				clif_status_change(&pl_sd->bl, ICO_NIGHT, 0);
				clif_wis_message(pl_sd->fd, wisp_server_name, msg_txt(60), msglen + 1); // Day has arrived.
			}
		}
	} else {
		clif_displaymessage(fd, msg_txt(90)); // Sorry, it's already the day. Impossible to execute the command.
		return -1;
	}

	return 0;
}

/*==========================================
 * @doommap - Kills all players on a map
 *------------------------------------------
 */
ATCOMMAND_FUNC(doommap) {
	struct map_session_data *pl_sd;
	int i, count;
	int map_id;

	if (!message || !*message || sscanf(message, "%s", atcmd_mapname) < 1)
		map_id = sd->bl.m;
	else {
		if (strstr(atcmd_mapname, ".gat") == NULL && strlen(atcmd_mapname) < 13) // 16 - 4 (.gat)
			strcat(atcmd_mapname, ".gat");
		if ((map_id = map_mapname2mapid(atcmd_mapname)) < 0) // Only from actual map-server
			map_id = sd->bl.m;
	}

	clif_specialeffect(&sd->bl, 450, 2); // Dark cross display around each player // Flag: 0: Player see in the area (Normal), 1: Only player see only by player, 2: All players in a map that see only their (Not see others), 3: All players that see only their (Not see others)
	count = 0;
	for (i = 0; i < fd_max; i++) {
		if (session[i] && (pl_sd = session[i]->session_data) && pl_sd->state.auth && i != fd && map_id == pl_sd->bl.m &&
		    sd->GM_level >= pl_sd->GM_level) { // Only lower or same GM level
			pc_damage(NULL, pl_sd, pl_sd->status.hp + 1);
			clif_displaymessage(pl_sd->fd, msg_txt(61)); // The holy messenger has given judgement.
			count++;
		}
	}
	clif_displaymessage(fd, msg_txt(62)); // Judgement was made.
	if (count == 0)
		sprintf(atcmd_output, "No player killed on the map '%s'.", map[map_id].name);
	else if (count == 1)
		sprintf(atcmd_output, "1 player killed on the map '%s'.", map[map_id].name);
	else
		sprintf(atcmd_output, "%d players killed on the map '%s'.", count, map[map_id].name);
	clif_displaymessage(fd, atcmd_output);

	return 0;
}

/*==========================================
 * @doom - Kills all players on a server
 *------------------------------------------
 */
ATCOMMAND_FUNC(doom) {
	struct map_session_data *pl_sd;
	int i, count;

	clif_specialeffect(&sd->bl, 450, 3); // Dark cross display around each player // Flag: 0: Player see in the area (Normal), 1: Only player see only by player, 2: All players in a map that see only their (Not see others), 3: All players that see only their (Not see others)
	count = 0;
	for(i = 0; i < fd_max; i++) {
		if (session[i] && (pl_sd = session[i]->session_data) && pl_sd->state.auth && i != fd &&
		    sd->GM_level >= pl_sd->GM_level) { // Only lower or same GM level
			pc_damage(NULL, pl_sd, pl_sd->status.hp + 1);
			clif_displaymessage(pl_sd->fd, msg_txt(61)); // The holy messenger has given judgement.
			count++;
		}
	}
	clif_displaymessage(fd, msg_txt(62)); // Judgement was made.
	if (count == 0)
		clif_displaymessage(fd, "No player killed.");
	else if (count == 1)
		clif_displaymessage(fd, "1 player killed.");
	else {
		sprintf(atcmd_output, "%d players killed.", count);
		clif_displaymessage(fd, atcmd_output);
	}

	return 0;
}

/*==========================================
 * atcommand_raise_sub Function
 *------------------------------------------
 */
static void atcommand_raise_sub(struct map_session_data* sd) {
	clif_skill_nodamage(&sd->bl, &sd->bl, ALL_RESURRECTION, 4, 1);
	sd->status.hp = sd->status.max_hp;
	sd->status.sp = sd->status.max_sp;
	pc_setstand(sd);
	clif_updatestatus(sd, SP_HP);
	clif_updatestatus(sd, SP_SP);
	clif_resurrection(&sd->bl, 1);
	if (battle_config.pc_invincible_time > 0)
		pc_setinvincibletimer(sd, battle_config.pc_invincible_time);
	clif_displaymessage(sd->fd, msg_txt(63)); // Mercy has been shown.

	return;
}

/*==========================================
 * @raisemap - Revives all players on a map
 *------------------------------------------
 */
ATCOMMAND_FUNC(raisemap) {
	struct map_session_data *pl_sd;
	int i, count;
	int map_id;

	if (!message || !*message || sscanf(message, "%s", atcmd_mapname) < 1)
		map_id = sd->bl.m;
	else {
		if (strstr(atcmd_mapname, ".gat") == NULL && strlen(atcmd_mapname) < 13) // 16 - 4 (.gat)
			strcat(atcmd_mapname, ".gat");
		if ((map_id = map_mapname2mapid(atcmd_mapname)) < 0) // Only from actual map-server
			map_id = sd->bl.m;
	}

	count = 0;
	for (i = 0; i < fd_max; i++) {
		if (session[i] && (pl_sd = session[i]->session_data) && pl_sd->state.auth && pc_isdead(pl_sd) && map_id == pl_sd->bl.m) {
			atcommand_raise_sub(pl_sd);
			count++;
		}
	}
	clif_displaymessage(fd, msg_txt(64)); // Mercy has been granted.
	if (count == 0)
		sprintf(atcmd_output, "No player raised on the map '%s'.", map[map_id].name);
	else if (count == 1)
		sprintf(atcmd_output, "1 player raised on the map '%s'.", map[map_id].name);
	else
		sprintf(atcmd_output, "%d players raised on the map '%s'.", count, map[map_id].name);
	clif_displaymessage(fd, atcmd_output);

	return 0;
}

/*==========================================
 * @raise - Revives all players on a server
 *------------------------------------------
 */
ATCOMMAND_FUNC(raise) {
	struct map_session_data *pl_sd;
	int i, count;

	count = 0;
	for (i = 0; i < fd_max; i++) {
		if (session[i] && (pl_sd = session[i]->session_data) && pl_sd->state.auth && pc_isdead(pl_sd)) {
			atcommand_raise_sub(pl_sd);
			count++;
		}
	}
	clif_displaymessage(fd, msg_txt(64)); // Mercy has been granted.
	if (count == 0)
		sprintf(atcmd_output, "No player raised.");
	else if (count == 1)
		sprintf(atcmd_output, "1 player raised.");
	else
		sprintf(atcmd_output, "%d players raised.", count);
	clif_displaymessage(fd, atcmd_output);

	return 0;
}

/*==========================================
 * @charbaselvl - Changes a target's base level
 *------------------------------------------
*/
ATCOMMAND_FUNC(character_baselevel) {
	struct map_session_data *pl_sd;
	int level = 0, i, modified_stat;
	short modified_status[6]; // Need to update only modifed stats

	if (!message || !*message || sscanf(message, "%d %[^\n]", &level, atcmd_name) < 2 || level == 0) {
		send_usage(sd, "Please, enter a level adjustement and a player name (usage: %s <#> <nickname>).", original_command);
		return -1;
	}

	if ((pl_sd = map_nick2sd(atcmd_name)) != NULL || ((pl_sd = map_id2sd(atoi(atcmd_name))) != NULL && pl_sd->state.auth)) {
		if (sd->GM_level >= pl_sd->GM_level) { // Only lower or same GM level
			if (level > 0) {
				if (pl_sd->status.base_level == battle_config.maximum_level) { // check for max level
					clif_displaymessage(fd, msg_txt(91)); // Character's base level can't go any higher.
					return 0;
				}
				if (level > battle_config.maximum_level || level > (battle_config.maximum_level - pl_sd->status.base_level)) // Fix positive overflow
					level = battle_config.maximum_level - pl_sd->status.base_level;
				for (i = 1; i <= level; i++)
					pl_sd->status.status_point += (pl_sd->status.base_level + i + 14) / 5;
				// If player have max in all stats, don't give status_point
				if (pl_sd->status.str  >= battle_config.max_parameter &&
				    pl_sd->status.agi  >= battle_config.max_parameter &&
				    pl_sd->status.vit  >= battle_config.max_parameter &&
				    pl_sd->status.int_ >= battle_config.max_parameter &&
				    pl_sd->status.dex  >= battle_config.max_parameter &&
				    pl_sd->status.luk  >= battle_config.max_parameter)
					pl_sd->status.status_point = 0;
				pl_sd->status.base_level += level;
				clif_updatestatus(pl_sd, SP_BASELEVEL);
				clif_updatestatus(pl_sd, SP_NEXTBASEEXP);
				clif_updatestatus(pl_sd, SP_STATUSPOINT);
				status_calc_pc(pl_sd, 0);
				pc_heal(pl_sd, pl_sd->status.max_hp, pl_sd->status.max_sp);
				clif_misceffect(&pl_sd->bl, 0);
				clif_displaymessage(fd, msg_txt(65)); // Character's base level raised.
			} else {
				if (pl_sd->status.base_level == 1) {
					clif_displaymessage(fd, msg_txt(193)); // Character's base level can't go any lower.
					return -1;
				}
				if (level < -battle_config.maximum_level || level < (1 - pl_sd->status.base_level)) // Fix negative overflow
					level = 1 - pl_sd->status.base_level;
				for (i = 0; i > level; i--)
					pl_sd->status.status_point -= (pl_sd->status.base_level + i + 14) / 5;
				if (pl_sd->status.status_point < 0) {
					short* status[] = {
						&pl_sd->status.str,  &pl_sd->status.agi, &pl_sd->status.vit,
						&pl_sd->status.int_, &pl_sd->status.dex, &pl_sd->status.luk
					};
					// Remove points from stats begining by the upper
					for (i = 0; i <= MAX_STATUS_TYPE; i++)
						modified_status[i] = 0;
					while (pl_sd->status.status_point < 0 &&
					       pl_sd->status.str > 0 &&
					       pl_sd->status.agi > 0 &&
					       pl_sd->status.vit > 0 &&
					       pl_sd->status.int_ > 0 &&
					       pl_sd->status.dex > 0 &&
					       pl_sd->status.luk > 0 &&
					       (pl_sd->status.str + pl_sd->status.agi + pl_sd->status.vit + pl_sd->status.int_ + pl_sd->status.dex + pl_sd->status.luk > 6)) {
						modified_stat = 0;
						for (i = 1; i <= MAX_STATUS_TYPE; i++)
							if (*status[modified_stat] < *status[i])
								modified_stat = i;
						pl_sd->status.status_point += (*status[modified_stat] + 8) / 10 + 1; // ((val - 1) + 9) / 10 + 1
						*status[modified_stat] = *status[modified_stat] - ((short)1);
						modified_status[modified_stat] = 1; // Flag
					}
					for (i = 0; i <= MAX_STATUS_TYPE; i++)
						if (modified_status[i]) {
							clif_updatestatus(pl_sd, SP_STR + i);
							clif_updatestatus(pl_sd, SP_USTR + i);
						}
				}
				clif_updatestatus(pl_sd, SP_STATUSPOINT);
				pl_sd->status.base_level += level; // Note: Here, level is negative
				clif_updatestatus(pl_sd, SP_BASELEVEL);
				clif_updatestatus(pl_sd, SP_NEXTBASEEXP);
				status_calc_pc(pl_sd, 0);
				clif_displaymessage(fd, msg_txt(66)); // Character's base level lowered.
			}
		} else {
			clif_displaymessage(fd, msg_txt(81)); // Your GM level don't authorize you to do this action on this player.
			return -1;
		}
	} else {
		clif_displaymessage(fd, msg_txt(3)); // Character not found.
		return -1;
	}

	return 0;
}

/*==========================================
 * @charjoblvl - Changes a target's job level
 *------------------------------------------
 */
ATCOMMAND_FUNC(character_joblevel) {
	struct map_session_data *pl_sd;
	int max_level, level = 0;

	if (!message || !*message || sscanf(message, "%d %[^\n]", &level, atcmd_name) < 2 || level == 0) {
		send_usage(sd, "Please, enter a level adjustement and a player name (usage: %s <#> <char name|account_id>).", original_command);
		return -1;
	}

	if ((pl_sd = map_nick2sd(atcmd_name)) != NULL || ((pl_sd = map_id2sd(atoi(atcmd_name))) != NULL && pl_sd->state.auth)) {
		if (sd->GM_level >= pl_sd->GM_level) { // Only lower or same gm level

		// Maximum job level checking for each class

    // Standard classes can go to job 50
    max_level = 50;
    // Novice can go to job 10
    if (pl_sd->status.class == 0 || pl_sd->status.class == 4001 || pl_sd->status.class == 4023)
        max_level = 10;
    // Super Novice can go to job 99
    if (pl_sd->status.class == 23 || pl_sd->status.class == 4045)
        max_level = 99;
    // Advanced classes can go up to level 70
    if ((pl_sd->status.class >= 4008) && (pl_sd->status.class <= 4022))
        max_level = 70;
    // Ninja and Gunslingers should be able to go to job level 70
    if ((pl_sd->status.class == 24) || (pl_sd->status.class == 25))
        max_level = 70;

			if (level > 0) {
				if (pl_sd->status.job_level == max_level) {
					clif_displaymessage(fd, msg_txt(67)); // Character's job level can't go any higher
					return -1;
				}
				if (level > max_level || level > (max_level - pl_sd->status.job_level)) // Fix positive overflow
					level = max_level - pl_sd->status.job_level;
				// Check with maximum authorized level
				if (pl_sd->status.class == 0) { // Novice
					if (pl_sd->status.job_level >= battle_config.atcommand_max_job_level_novice) {
						clif_displaymessage(fd, "You're not authorized to increase more the job level of this player.");
						return -1;
					} else if (pl_sd->status.job_level + level >= battle_config.atcommand_max_job_level_novice)
						level = battle_config.atcommand_max_job_level_novice - pl_sd->status.job_level;
				} else if (pl_sd->status.class <= 6 || pl_sd->status.class == 4046) { // First Job/Taekwon Kid
					if (pl_sd->status.job_level >= battle_config.atcommand_max_job_level_job1) {
						clif_displaymessage(fd, "You're not authorized to increase more the job level of this player.");
						return -1;
					} else if (pl_sd->status.job_level + level >= battle_config.atcommand_max_job_level_job1)
						level = battle_config.atcommand_max_job_level_job1 - pl_sd->status.job_level;
				} else if (pl_sd->status.class <= 22 || (sd->status.class >= 4047 && sd->status.class <= 4049)) { // Second Job/Star Gladiator/Soul Linker
					if (pl_sd->status.job_level >= battle_config.atcommand_max_job_level_job2) {
						clif_displaymessage(fd, "You're not authorized to increase more the job level of this player.");
						return -1;
					} else if (pl_sd->status.job_level + level >= battle_config.atcommand_max_job_level_job2)
						level = battle_config.atcommand_max_job_level_job2 - pl_sd->status.job_level;
				} else if (pl_sd->status.class == 23) { // Super Novice
					if (pl_sd->status.job_level >= battle_config.atcommand_max_job_level_supernovice) {
						clif_displaymessage(fd, "You're not authorized to increase more the job level of this player.");
						return -1;
					} else if (pl_sd->status.job_level + level >= battle_config.atcommand_max_job_level_supernovice)
						level = battle_config.atcommand_max_job_level_supernovice - pl_sd->status.job_level;
				} else if (pl_sd->status.class == 24) { // Gunslinger
					if (pl_sd->status.job_level >= battle_config.atcommand_max_job_level_gunslinger) {
						clif_displaymessage(fd, "You're not authorized to increase more the job level of this player.");
						return -1;
					} else if (pl_sd->status.job_level + level >= 70)
						level = 70 - pl_sd->status.job_level;
				} else if (pl_sd->status.class == 25) { // Ninja
					if (pl_sd->status.job_level >= battle_config.atcommand_max_job_level_ninja) {
						clif_displaymessage(fd, "You're not authorized to increase more the job level of this player.");
						return -1;
					} else if (pl_sd->status.job_level + level >= 70)
						level = 70 - pl_sd->status.job_level;
				} else if (pl_sd->status.class == 4001) { // High Novice
					if (pl_sd->status.job_level >= battle_config.atcommand_max_job_level_highnovice) {
						clif_displaymessage(fd, "You're not authorized to increase more the job level of this player.");
						return -1;
					} else if (pl_sd->status.job_level + level >= battle_config.atcommand_max_job_level_highnovice)
						level = battle_config.atcommand_max_job_level_highnovice - pl_sd->status.job_level;
				} else if (pl_sd->status.class <= 4007) { // High First Job
					if (pl_sd->status.job_level >= battle_config.atcommand_max_job_level_highjob1) {
						clif_displaymessage(fd, "You're not authorized to increase more the job level of this player.");
						return -1;
					} else if (pl_sd->status.job_level + level >= battle_config.atcommand_max_job_level_highjob1)
						level = battle_config.atcommand_max_job_level_highjob1 - pl_sd->status.job_level;
				} else if (pl_sd->status.class <= 4022) { // High Second Job
					if (pl_sd->status.job_level >= battle_config.atcommand_max_job_level_highjob2) {
						clif_displaymessage(fd, "You're not authorized to increase more the job level of this player.");
						return -1;
					} else if (pl_sd->status.job_level + level >= battle_config.atcommand_max_job_level_highjob2)
						level = battle_config.atcommand_max_job_level_highjob2 - pl_sd->status.job_level;
				} else if (pl_sd->status.class == 4023) { // Baby Novice
					if (pl_sd->status.job_level >= battle_config.atcommand_max_job_level_babynovice) {
						clif_displaymessage(fd, "You're not authorized to increase more the job level of this player.");
						return -1;
					} else if (pl_sd->status.job_level + level >= battle_config.atcommand_max_job_level_babynovice)
						level = battle_config.atcommand_max_job_level_babynovice - pl_sd->status.job_level;
				} else if (pl_sd->status.class <= 4029) { // Baby First Job
					if (pl_sd->status.job_level >= battle_config.atcommand_max_job_level_babyjob1) {
						clif_displaymessage(fd, "You're not authorized to increase more the job level of this player.");
						return -1;
					} else if (pl_sd->status.job_level + level >= battle_config.atcommand_max_job_level_babyjob1)
						level = battle_config.atcommand_max_job_level_babyjob1 - pl_sd->status.job_level;
				} else if (pl_sd->status.class <= 4044) { // Baby Second Job
					if (pl_sd->status.job_level >= battle_config.atcommand_max_job_level_babyjob2) {
						clif_displaymessage(fd, "You're not authorized to increase more the job level of this player.");
						return -1;
					} else if (pl_sd->status.job_level + level >= battle_config.atcommand_max_job_level_babyjob2)
						level = battle_config.atcommand_max_job_level_babyjob2 - pl_sd->status.job_level;
				} else if (pl_sd->status.class == 4045) { // Super Baby
					if (pl_sd->status.job_level >= battle_config.atcommand_max_job_level_superbaby) {
						clif_displaymessage(fd, "You're not authorized to increase more the job level of this player.");
						return -1;
					} else if (pl_sd->status.job_level + level >= battle_config.atcommand_max_job_level_superbaby)
						level = battle_config.atcommand_max_job_level_superbaby - pl_sd->status.job_level;
				}
				pl_sd->status.job_level += level;
				clif_updatestatus(pl_sd, SP_JOBLEVEL);
				clif_updatestatus(pl_sd, SP_NEXTJOBEXP);
				pl_sd->status.skill_point += level;
				clif_updatestatus(pl_sd, SP_SKILLPOINT);
				status_calc_pc(pl_sd, 0);
				clif_misceffect(&pl_sd->bl, 1);
				clif_displaymessage(fd, msg_txt(68)); // Character's job level raised
			} else {
				if (pl_sd->status.job_level == 1) {
					clif_displaymessage(fd, msg_txt(194)); // Character's job level can't go any lower
					return -1;
				}
				if (level < -max_level || level < (1 - pl_sd->status.job_level)) // Fix negative overflow
					level = 1 - pl_sd->status.job_level;
				// Don't check maximum authorized if we reduce level
				pl_sd->status.job_level += level;
				clif_updatestatus(pl_sd, SP_JOBLEVEL);
				clif_updatestatus(pl_sd, SP_NEXTJOBEXP);
				if (pl_sd->status.skill_point > 0) {
					pl_sd->status.skill_point += level; // Note: Here, level is negative
					if (pl_sd->status.skill_point < 0) {
						level = pl_sd->status.skill_point;
						pl_sd->status.skill_point = 0;
					}
					clif_updatestatus(pl_sd, SP_SKILLPOINT);
				}
				if (level < 0) { // If always negative, skill points must be removed from skills
					// To add: Remove skill points from skills
				}
				status_calc_pc(pl_sd, 0);
				clif_displaymessage(fd, msg_txt(69)); // Character's job level lowered
			}
		} else {
			clif_displaymessage(fd, msg_txt(81)); // Your GM level don't authorize you to do this action on this player
			return -1;
		}
	} else {
		clif_displaymessage(fd, msg_txt(3)); // Character not found
		return -1;
	}

	return 0;
}

/*==========================================
 * @kick - Kicks a target off a server
 *------------------------------------------
 */
ATCOMMAND_FUNC(kick) {
	struct map_session_data *pl_sd;

	if (!message || !*message || sscanf(message, "%[^\n]", atcmd_name) < 1) {
		send_usage(sd, "Please, enter a player name (usage: %s <char name|account_id>).", original_command);
		return -1;
	}

	if ((pl_sd = map_nick2sd(atcmd_name)) != NULL || ((pl_sd = map_id2sd(atoi(atcmd_name))) != NULL && pl_sd->state.auth)) {
		if (sd->GM_level >= pl_sd->GM_level) // Only lower or same GM level
			clif_GM_kick(sd, pl_sd, 1);
		else {
			clif_displaymessage(fd, msg_txt(81)); // Your GM level don't authorize you to do this action on this player.
			return -1;
		}
	} else {
		clif_displaymessage(fd, msg_txt(3)); // Character not found.
		return -1;
	}

	return 0;
}

/*==========================================
 * @kickmap - Kicks all players on a map off a server
 *------------------------------------------
 */
ATCOMMAND_FUNC(kickmap) {
	struct map_session_data *pl_sd;
	int i, count;
	int map_id;

	if (!message || !*message || sscanf(message, "%s", atcmd_mapname) < 1)
		map_id = sd->bl.m;
	else {
		if (strstr(atcmd_mapname, ".gat") == NULL && strlen(atcmd_mapname) < 13) // 16 - 4 (.gat)
			strcat(atcmd_mapname, ".gat");
		if ((map_id = map_mapname2mapid(atcmd_mapname)) < 0) // Only from actual map-server
			map_id = sd->bl.m;
	}

	count = 0;
	for (i = 0; i < fd_max; i++) {
		if (session[i] && (pl_sd = session[i]->session_data) && pl_sd->state.auth && map_id == pl_sd->bl.m &&
		    sd->GM_level >= pl_sd->GM_level) { // Only lower or same GM level
			if (fd != i)
				clif_GM_kick(sd, pl_sd, 0);
				count++;
			}
		}

	if (count == 0)
		sprintf(atcmd_output, "No player kicked from the map '%s'.", map[map_id].name);
	else if (count == 1)
		sprintf(atcmd_output, "1 player kicked from the map '%s'.", map[map_id].name);
	else
		sprintf(atcmd_output, "%d players kicked from the map '%s'.", count, map[map_id].name);
	clif_displaymessage(fd, atcmd_output);

	return 0;
}

/*==========================================
 * @kickall - Kicks all players off a server
 *------------------------------------------
 */
ATCOMMAND_FUNC(kickall) {
	struct map_session_data *pl_sd;
	int i, count;

	count = 0;
	for (i = 0; i < fd_max; i++) {
		if (session[i] && (pl_sd = session[i]->session_data) && pl_sd->state.auth &&
		    sd->GM_level >= pl_sd->GM_level) { // Only lower or same GM level
			if (fd != i)
				clif_GM_kick(sd, pl_sd, 0);
				count++;
			}
		}

	if (count == 0)
		clif_displaymessage(fd, msg_txt(28)); // No player found.
	else if (count == 1)
		clif_displaymessage(fd, msg_txt(29)); // 1 player found.
	else {
		sprintf(atcmd_output, msg_txt(30), count); // %d players found.
		clif_displaymessage(fd, atcmd_output);
	}
	clif_displaymessage(fd, msg_txt(195)); // All players have been kicked!

	return 0;
}

/*==========================================
 * @allskill - Gives user all available skills
 *------------------------------------------
 */
ATCOMMAND_FUNC(allskill) {
	pc_allskillup(sd); // All skills
	clif_displaymessage(fd, msg_txt(76)); // You have received all skills.

	return 0;
}

/*==========================================
 * @questskill - Gives a user all available quest skills
 *------------------------------------------
 */
ATCOMMAND_FUNC(questskill) {
	int skill_id;

	if (!message || !*message || sscanf(message, "%d", &skill_id) < 1 || skill_id < 0) {
		send_usage(sd, "Please, enter a quest skill number (usage: %s <#:0+>).", original_command);
		send_usage(sd, "Novice                 Swordsman                  Thief                Merchant");
		send_usage(sd, "142 = Emergency Care   144 = Moving HP Recovery   149 = Throw Sand     153 = Cart Revolution");
		send_usage(sd, "143 = Act dead         145 = Attack Weak Point    150 = Back Sliding   154 = Change Cart");
		send_usage(sd, "Archer                 146 = Auto Berserk         151 = Take Stone     155 = Crazy Uproar/Loud Voice");
		send_usage(sd, "147 = Arrow Creation   Acolyte                    152 = Stone Throw    Magician");
		send_usage(sd, "148 = Charge Arrows    156 = Holy Light                                157 = Energy Coat");
		send_usage(sd, "Knight                 Blacksmith                 Priest");
		send_usage(sd, "1001 = Charge Attack   1013 = Greed               1014 = Redemptio");
		send_usage(sd, "Rogue                  Monk                       Sage");
		send_usage(sd, "1005 = Close Confine   1015 = Ki Translation      1008 = Elemental Change Water");
		send_usage(sd, "Dancer                 1016 = Ki Explosion        1017 = Elemental Change Earth");
		send_usage(sd, "1011 = Wink of Charm                              1018 = Elemental Change Fire");
		send_usage(sd, "                                                  1019 = Elemental Change Wind");
		return -1;
	}

	if (skill_id >= 0 && skill_id < MAX_SKILL_DB) {
		if (skill_get_inf2(skill_id) & 0x01) {
			if (pc_checkskill(sd, skill_id) == 0) {
				pc_skill(sd, skill_id, 1, 0);
				if (pc_checkskill(sd, skill_id) == 0) { // If always unknown
					clif_displaymessage(fd, "You can not learn this skill quest (incompatible job?).");
				} else {
					clif_displaymessage(fd, msg_txt(70)); // You have learned the skill.
				}
			} else {
				clif_displaymessage(fd, msg_txt(196)); // You already have this quest skill.
				return -1;
			}
		} else {
			clif_displaymessage(fd, msg_txt(197)); // This skill number doesn't exist or isn't a quest skill.
			return -1;
		}
	} else {
		clif_displaymessage(fd, msg_txt(198)); // This skill number doesn't exist.
		return -1;
	}

	return 0;
}

/*==========================================
 * @charquestskill - Gives a target all available quest skills
 *------------------------------------------
 */
ATCOMMAND_FUNC(charquestskill) {
	struct map_session_data *pl_sd;
	int skill_id;

	if (!message || !*message || sscanf(message, "%d %[^\n]", &skill_id, atcmd_name) < 2 || skill_id < 0) {
		send_usage(sd, "Please, enter a quest skill number and a player name (usage: %s <#:0+> <char name|account_id>).", original_command);
		send_usage(sd, "Novice                 Swordsman                  Thief                Merchant");
		send_usage(sd, "142 = Emergency Care   144 = Moving HP Recovery   149 = Throw Sand     153 = Cart Revolution");
		send_usage(sd, "143 = Act dead         145 = Attack Weak Point    150 = Back Sliding   154 = Change Cart");
		send_usage(sd, "Archer                 146 = Auto Berserk         151 = Take Stone     155 = Crazy Uproar/Loud Voice");
		send_usage(sd, "147 = Arrow Creation   Acolyte                    152 = Stone Throw    Magician");
		send_usage(sd, "148 = Charge Arrows    156 = Holy Light                                157 = Energy Coat");
		send_usage(sd, "Knight                 Blacksmith                 Priest");
		send_usage(sd, "1001 = Charge Attack   1013 = Greed               1014 = Redemptio");
		send_usage(sd, "Rogue                  Monk                       Sage");
		send_usage(sd, "1005 = Close Confine   1015 = Ki Translation      1008 = Elemental Change Water");
		send_usage(sd, "Dancer                 1016 = Ki Explosion        1017 = Elemental Change Earth");
		send_usage(sd, "1011 = Wink of Charm                              1018 = Elemental Change Fire");
		send_usage(sd, "                                                  1019 = Elemental Change Wind");
		return -1;
	}

	if (skill_id >= 0 && skill_id < MAX_SKILL_DB) {
		if (skill_get_inf2(skill_id) & 0x01) {
			if ((pl_sd = map_nick2sd(atcmd_name)) != NULL || ((pl_sd = map_id2sd(atoi(atcmd_name))) != NULL && pl_sd->state.auth)) {
				if (pc_checkskill(pl_sd, skill_id) == 0) {
					pc_skill(pl_sd, skill_id, 1, 0);
					if (pc_checkskill(pl_sd, skill_id) == 0) { // If always unknown
						clif_displaymessage(fd, "This player can not learn this skill quest (incompatible job?).");
					} else {
						clif_displaymessage(fd, msg_txt(199)); // This player has learned the skill.
					}
				} else {
					clif_displaymessage(fd, msg_txt(200)); // This player already has this quest skill.
					return -1;
				}
			} else {
				clif_displaymessage(fd, msg_txt(3)); // Character not found.
				return -1;
			}
		} else {
			clif_displaymessage(fd, msg_txt(197)); // This skill number doesn't exist or isn't a quest skill.
			return -1;
		}
	} else {
		clif_displaymessage(fd, msg_txt(198)); // This skill number doesn't exist.
		return -1;
	}

	return 0;
}

/*==========================================
 * @lostskill
 *------------------------------------------
 */
ATCOMMAND_FUNC(lostskill) {
	int skill_id;

	if (!message || !*message || sscanf(message, "%d", &skill_id) < 1 || skill_id < 0) {
		send_usage(sd, "Please, enter a quest skill number (usage: %s <#:0+>).", original_command);
		return -1;
	}

	if (skill_id >= 0 && skill_id < MAX_SKILL) {
		if (skill_get_inf2(skill_id) & 0x01) {
			if (pc_checkskill(sd, skill_id) > 0) {
				sd->status.skill[skill_id].id = 0;
				sd->status.skill[skill_id].lv = 0;
				sd->status.skill[skill_id].flag = 0;
				clif_skillinfoblock(sd);
				clif_displaymessage(fd, msg_txt(71)); // You have forgotten the skill.
			} else {
				clif_displaymessage(fd, msg_txt(201)); // You don't have this quest skill.
				return -1;
			}
		} else {
			clif_displaymessage(fd, msg_txt(197)); // This skill number doesn't exist or isn't a quest skill.
			return -1;
		}
	} else {
		clif_displaymessage(fd, msg_txt(198)); // This skill number doesn't exist.
		return -1;
	}

	return 0;
}

/*==========================================
 * @charlostskill
 *------------------------------------------
 */
ATCOMMAND_FUNC(charlostskill) {
	struct map_session_data *pl_sd;
	int skill_id;

	if (!message || !*message || sscanf(message, "%d %[^\n]", &skill_id, atcmd_name) < 2 || skill_id < 0) {
		send_usage(sd, "Please, enter a quest skill number and a player name (usage: %s <#:0+> <char name|account_id>).", original_command);
		return -1;
	}

	if (skill_id >= 0 && skill_id < MAX_SKILL) {
		if (skill_get_inf2(skill_id) & 0x01) {
			if ((pl_sd = map_nick2sd(atcmd_name)) != NULL || ((pl_sd = map_id2sd(atoi(atcmd_name))) != NULL && pl_sd->state.auth)) {
				if (pc_checkskill(pl_sd, skill_id) > 0) {
					pl_sd->status.skill[skill_id].id = 0;
					pl_sd->status.skill[skill_id].lv = 0;
					pl_sd->status.skill[skill_id].flag = 0;
					clif_skillinfoblock(pl_sd);
					clif_displaymessage(fd, msg_txt(202)); // This player has forgotten the skill.
				} else {
					clif_displaymessage(fd, msg_txt(203)); // This player doesn't have this quest skill.
					return -1;
				}
			} else {
				clif_displaymessage(fd, msg_txt(3)); // Character not found.
				return -1;
			}
		} else {
			clif_displaymessage(fd, msg_txt(197)); // This skill number doesn't exist or isn't a quest skill.
			return -1;
		}
	} else {
		clif_displaymessage(fd, msg_txt(198)); // This skill number doesn't exist.
		return -1;
	}

	return 0;
}

/*==========================================
 * @spiritball - Summons spirit balls on the user
 *------------------------------------------
 */
ATCOMMAND_FUNC(spiritball) {
	int number;

	if (!message || !*message || sscanf(message, "%d", &number) < 1 || number < 0) {
		send_usage(sd, "Please, enter a valid spirit ball number (usage: %s <number: 0-500>).", original_command);
		return -1;
	}

	// Set max number to avoid server/client crash (500 create big spirit balls of several spirit balls: No visual difference with more)
	if (number > 500)
		number = 500;

	if (sd->spiritball != number) {
		if (sd->spiritball > 0)
			pc_delspiritball(sd, sd->spiritball, 1);
			sd->spiritball = number;
			clif_spiritball(sd);
			// No message, player can look the difference
			if (number > 400)
				clif_displaymessage(fd, msg_txt(204)); // WARNING: more than 400 spirit balls can CRASH your client!
	} else {
		clif_displaymessage(fd, msg_txt(205)); // You already have this number of spiritballs.
		return -1;
	}

	return 0;
}

/*==========================================
 * @charspiritball - Summons spirit balls on a target
 *------------------------------------------
 */
ATCOMMAND_FUNC(charspiritball) {
	struct map_session_data *pl_sd;
	int number;

	if (!message || !*message || sscanf(message, "%d %[^\n]", &number, atcmd_name) < 2 || number < 0) {
		send_usage(sd, "Please, enter a spirit ball number and a player name (usage: %s <number: 0-500> <char name|account_id>).", original_command);
		return -1;
	}

	if ((pl_sd = map_nick2sd(atcmd_name)) != NULL || ((pl_sd = map_id2sd(atoi(atcmd_name))) != NULL && pl_sd->state.auth)) {
		if (sd->GM_level >= pl_sd->GM_level) { // Only lower or same GM level
			// Set max number to avoid server/client crash (500 create big spirit balls of several spirit balls: No visual difference with more)
			if (number > 500)
				number = 500;
			if (pl_sd->spiritball != number) {
				if (pl_sd->spiritball > 0)
					pc_delspiritball(pl_sd, pl_sd->spiritball, 1);
					pl_sd->spiritball = number;
					clif_spiritball(pl_sd);
				if (number > 0) {
					sprintf(atcmd_output, "The player has now %d spirit ball(s).", number);
					clif_displaymessage(fd, atcmd_output);
					if (number > 400)
						clif_displaymessage(fd, "WARNING: more than 400 spiritballs can CRASH the player's client!");
				} else
					clif_displaymessage(fd, "The player has no more spirit ball.");
			} else {
				clif_displaymessage(fd, "The player already has this number of spiritballs.");
				return -1;
			}
		} else {
			clif_displaymessage(fd, msg_txt(81)); // Your GM level don't authorize you to do this action on this player.
			return -1;
		}
	} else {
		clif_displaymessage(fd, msg_txt(3)); // Character not found.
		return -1;
	}

	return 0;
}

/*==========================================
 * @guild - Creates a guild
 *------------------------------------------
 */
ATCOMMAND_FUNC(guild) {
	char guild_name[100];
	int prev;

	memset(guild_name, 0, sizeof(guild_name));

	if (!message || !*message || (sscanf(message, "\"%[^\"]\"", guild_name) < 1 && sscanf(message, "%[^\n]", guild_name) < 1) || guild_name[0] == '\0') {
		send_usage(sd, "Please, enter a guild name (usage: %s \"guild_name\"|<guild_name>).", original_command);
		return -1;
	}

	if (strchr(guild_name, '\"') != NULL) {
		clif_displaymessage(fd, msg_txt(654)); // You are not authorized to use '"' in a guild name. // If you use ", /breakguild will not work
		return -1;
	}

	if (strlen(guild_name) <= 24) {
		prev = battle_config.guild_emperium_check;
		battle_config.guild_emperium_check = 0;
		guild_create(sd, guild_name);
		battle_config.guild_emperium_check = prev;
	} else {
		clif_displaymessage(fd, msg_txt(259)); // Invalid guild name. Guild name must have between 1 to 24 characters.
		return -1;
	}

	return 0;
}

/*==========================================
 * @agitstart - Starts the War of Emperium
 *------------------------------------------
 */
ATCOMMAND_FUNC(agitstart) {
	if (agit_flag == 1) {
		clif_displaymessage(fd, msg_txt(73)); // Already it has started siege warfare.
		return -1;
	}

	agit_flag = 1; // 0: WoE not starting, Woe is running
	guild_agit_start();
	clif_displaymessage(fd, msg_txt(72)); // Guild siege warfare start!

	return 0;
}

/*==========================================
 * @agitend - Ends the War of Emperium
 *------------------------------------------
 */
ATCOMMAND_FUNC(agitend) {
	if (agit_flag == 0) {
		clif_displaymessage(fd, msg_txt(75)); // Siege warfare hasn't started yet.
		return -1;
	}

	pc_guardiansave(); // Save guardians (If necessary)
	agit_flag = 0; // 0: WoE not starting, Woe is running
	guild_agit_end();
	clif_displaymessage(fd, msg_txt(74)); // Guild siege warfare end!

	return 0;
}

/*==========================================
 * @mapexit
 *------------------------------------------
 */
ATCOMMAND_FUNC(mapexit) {
	struct map_session_data *pl_sd;
	int i;

	for (i = 0; i < fd_max; i++) {
		if (session[i] && (pl_sd = session[i]->session_data) && pl_sd->state.auth) {
			if (fd != i)
				clif_GM_kick(sd, pl_sd, 0);
		}
	}
	clif_GM_kick(sd, sd, 0);

	runflag = 0; // Terminate the main loop

	// Send all packets not sent
	flush_fifos();

	return 0;
}

/*==========================================
 * @idsearch/@itemsearch <part_of_name>
 *------------------------------------------
 */
ATCOMMAND_FUNC(idsearch) {
	char item_name[100];
	char temp[100];
	int i, j, match;
	struct item_data *item;

	memset(item_name, 0, sizeof(item_name));

	if (!message || !*message || sscanf(message, "%s", item_name) < 0) {
		send_usage(sd, "Please, enter a part of item name (usage: %s <part_of_item_name>).", original_command);
		return -1;
	}

	sprintf(atcmd_output, msg_txt(77), item_name); // The reference result of '%s' (name: id):
	clif_displaymessage(fd, atcmd_output);
	match = 0;
	for(i = 0; item_name[i]; i++)
		item_name[i] = tolower((unsigned char)item_name[i]); // tolower needs unsigned char
	for(i = 0; i < 20000; i++) {
		if ((item = itemdb_exists(i)) != NULL) {
			memset(temp, 0, sizeof(temp));
			for(j = 0; item->jname[j]; j++)
				temp[j] = tolower((unsigned char)item->jname[j]); // tolower needs unsigned char
			if (strstr(temp, item_name) != NULL) {
				match++;
				sprintf(atcmd_output, msg_txt(78), item->jname, item->nameid); // %s: %d
				clif_displaymessage(fd, atcmd_output);
			}
		}
	}
	sprintf(atcmd_output, msg_txt(79), match); // It is %d affair above.
	clif_displaymessage(fd, atcmd_output);

	return 0;
}

/*==========================================
 * whodrops <name/id_of_item>
 *------------------------------------------
 */
ATCOMMAND_FUNC(whodrops) {
	char item_name[100];
	int i;
	int item_id, mob_id, count;
	struct item_data *item;
	struct mob_db *mob;
	double temp_rate, rate;
	int drop_rate;
	int ratemin, ratemax;

	memset(item_name, 0, sizeof(item_name));

	if (!message || !*message || sscanf(message, "%s", item_name) < 0) {
		send_usage(sd, "Please, enter an item name/id (usage: %s <name/id_of_item>).", original_command);
		return -1;
	}

	item_id = 0;
	if ((item = itemdb_searchname(item_name)) != NULL ||
	    (item = itemdb_exists(atoi(item_name))) != NULL)
		item_id = item->nameid;

	if (item_id >= 501) {
		count = 0;
		for(mob_id = 1000; mob_id < MAX_MOB_DB; mob_id++) {
			if (!mobdb_checkid(mob_id)) // Mob doesn't exist
				continue;
			mob = &mob_db[mob_id];
			// Search within normal drops
			for (i = 0; i < 10; i++) {
				if (mob->dropitem[i].nameid != item_id)
					continue;
				// Get value for calculation of drop rate
				switch (itemdb_type(mob->dropitem[i].nameid)) {
				case 0:
					rate = (double)battle_config.item_rate_heal;
					ratemin = battle_config.item_drop_heal_min;
					ratemax = battle_config.item_drop_heal_max;
					break;
				case 2:
					rate = (double)battle_config.item_rate_use;
					ratemin = battle_config.item_drop_use_min;
					ratemax = battle_config.item_drop_use_max;
					break;
				case 4:
				case 5:
				case 8: // Changed to include Pet Equip
					rate = (double)battle_config.item_rate_equip;
					ratemin = battle_config.item_drop_equip_min;
					ratemax = battle_config.item_drop_equip_max;
					break;
				case 6:
					rate = (double)battle_config.item_rate_card;
					ratemin = battle_config.item_drop_card_min;
					ratemax = battle_config.item_drop_card_max;
					break;
				default:
					rate = (double)battle_config.item_rate_common;
					ratemin = battle_config.item_drop_common_min;
					ratemax = battle_config.item_drop_common_max;
					break;
				}
				// Calculation of drop rate
				if (mob->dropitem[i].p <= 0)
					drop_rate = 0;
				else {
					temp_rate = ((double)mob->dropitem[i].p) * rate / 100.;
					if (temp_rate > 10000. || temp_rate < 0 || ((int)temp_rate) > 10000)
						drop_rate = 10000;
					else
						drop_rate = (int)temp_rate;
				}
				// Check minimum/maximum with configuration
				if (drop_rate < ratemin)
					drop_rate = ratemin;
				else if (drop_rate > ratemax)
					drop_rate = ratemax;
				if (drop_rate <= 0 && !battle_config.drop_rate0item)
					drop_rate = 1;
				// Display drops
				if (drop_rate > 0) {
					count++;
					if (count == 1) {
						sprintf(atcmd_output, msg_txt(254), item->jname, item->name, item_id); // List of monsters (With current drop rate) that drop '%s (%s)' (id: %d):
						clif_displaymessage(fd, atcmd_output);
					}
					sprintf(atcmd_output, " %s (%s)  %02.02f%%", mob->jname, mob->name, ((float)drop_rate) / 100.);
					clif_displaymessage(fd, atcmd_output);
					break;
				}
			}
			// If item was found
			//if (i != 10) // If an administrator sets same item in normal drop and MvP drop, don't stop here to display MvP drop rate
			//	continue;
			// Within MvP drops
			if (mob->mexp) {
				for (i = 0; i < 3; i++) {
					if (mob->mvpitem[i].nameid != item_id)
						continue;
					// Calculation of drop rate
					if (mob->mvpitem[i].p <= 0)
						drop_rate = 0;
					else {
						// Item type specific droprate modifier
						switch(itemdb_type(mob->mvpitem[i].nameid))
						{
							case 0:		// Healing items
								temp_rate = ((double)mob->mvpitem[i].p) * ((double)battle_config.mvp_healing_rate) / (double)100;
								break;
							case 2:		// Usable items
								temp_rate = ((double)mob->mvpitem[i].p) * ((double)battle_config.mvp_usable_rate) / (double)100;
								break;
							case 4:
							case 5:
							case 8: 	// Equipable items
								temp_rate = ((double)mob->mvpitem[i].p) * ((double)battle_config.mvp_equipable_rate) / (double)100;
								break;
							case 6:		// Cards
								temp_rate = ((double)mob->mvpitem[i].p) * ((double)battle_config.mvp_card_rate) / (double)100;
								break;
							default:	// Common items
								temp_rate = ((double)mob->mvpitem[i].p) * ((double)battle_config.mvp_common_rate) / (double)100;
								break;
						}
						if(temp_rate > 10000. || temp_rate < 0 || ((int)temp_rate) > 10000)
							drop_rate = 10000;
						else
							drop_rate = (int)temp_rate;
					}
					// Check minimum/maximum with configuration
					if (drop_rate < battle_config.item_drop_mvp_min)
						drop_rate = battle_config.item_drop_mvp_min;
					else if (drop_rate > battle_config.item_drop_mvp_max)
						drop_rate = battle_config.item_drop_mvp_max;
					if (drop_rate <= 0 && !battle_config.drop_rate0item)
						drop_rate = 1;
					// Display drops
					if (drop_rate > 0) {
						count++;
						if (count == 1) {
							sprintf(atcmd_output, msg_txt(254), item->jname, item->name, item_id); // List of monsters (with current drop rate) that drop '%s (%s)' (id: %d):
							clif_displaymessage(fd, atcmd_output);
						}
						sprintf(atcmd_output, " %s (%s)  %02.02f%% (MVP)", mob->jname, mob->name, ((float)drop_rate) / 100.);
						clif_displaymessage(fd, atcmd_output);
						break;
					}
				}
			}
		}
		// Display number of monster
		if (count == 0) {
			sprintf(atcmd_output, msg_txt(255), item->jname, item->name, item_id); // No monster drops item '%s (%s)' (id: %d).
			clif_displaymessage(fd, atcmd_output);
		} else if (count == 1) {
			sprintf(atcmd_output, msg_txt(256), item->jname, item->name, item_id); // 1 monster drops item '%s (%s)' (id: %d).
			clif_displaymessage(fd, atcmd_output);
		} else {
			sprintf(atcmd_output, msg_txt(257), count, item->jname, item->name, item_id); // %d monsters drop item '%s (%s)' (id: %d).
			clif_displaymessage(fd, atcmd_output);
		}
	} else {
		clif_displaymessage(fd, msg_txt(19)); // Invalid item ID or name.
		return -1;
	}

	return 0;
}

/*==========================================
 * @resetstate - Resets user's stats
 *------------------------------------------
 */
ATCOMMAND_FUNC(resetstate) {
	pc_resetstate(sd);
	clif_displaymessage(fd, "Your status points have been reset.");

	return 0;
}

/*==========================================
 * @resetskill - Resets user's skills
 *------------------------------------------
 */
ATCOMMAND_FUNC(resetskill) {
	pc_resetskill(sd);
	clif_displaymessage(fd, "Your skill points have been reset.");

	return 0;
}

/*==========================================
 * @charstreset - Resets a target's stats
 *------------------------------------------
 */
ATCOMMAND_FUNC(charstreset) {
	struct map_session_data *pl_sd;

	if (!message || !*message || sscanf(message, "%[^\n]", atcmd_name) < 1) {
		send_usage(sd, "Please, enter a player name (usage: %s <char name|account_id>).", original_command);
		return -1;
	}

	if ((pl_sd = map_nick2sd(atcmd_name)) != NULL || ((pl_sd = map_id2sd(atoi(atcmd_name))) != NULL && pl_sd->state.auth)) {
		if (sd->GM_level >= pl_sd->GM_level) { // Only lower or same GM level
			pc_resetstate(pl_sd);
			sprintf(atcmd_output, msg_txt(207), pl_sd->status.name); // '%s' stats points reseted!
			clif_displaymessage(fd, atcmd_output);
		} else {
			clif_displaymessage(fd, msg_txt(81)); // Your GM level don't authorize you to do this action on this player.
			return -1;
		}
	} else {
		clif_displaymessage(fd, msg_txt(3)); // Character not found.
		return -1;
	}

	return 0;
}

/*==========================================
 * @charskreset - Resets a target's skills
 *------------------------------------------
 */
ATCOMMAND_FUNC(charskreset) {
	struct map_session_data *pl_sd;

	if (!message || !*message || sscanf(message, "%[^\n]", atcmd_name) < 1) {
		send_usage(sd, "Please, enter a player name (usage: %s <char name|account_id>).", original_command);
		return -1;
	}

	if ((pl_sd = map_nick2sd(atcmd_name)) != NULL || ((pl_sd = map_id2sd(atoi(atcmd_name))) != NULL && pl_sd->state.auth)) {
		if (sd->GM_level >= pl_sd->GM_level) { // Only lower or same GM level
			pc_resetskill(pl_sd);
			sprintf(atcmd_output, msg_txt(206), pl_sd->status.name); // '%s' skill points reseted!
			clif_displaymessage(fd, atcmd_output);
		} else {
			clif_displaymessage(fd, msg_txt(81)); // Your GM level don't authorize you to do this action on this player.
			return -1;
		}
	} else {
		clif_displaymessage(fd, msg_txt(3)); // Character not found.
		return -1;
	}

	return 0;
}

/*==========================================
 * @charreset - Resets a target's stats and skills
 *------------------------------------------
 */
ATCOMMAND_FUNC(charreset) {
	struct map_session_data *pl_sd;

	if (!message || !*message || sscanf(message, "%[^\n]", atcmd_name) < 1) {
		send_usage(sd, "Please, enter a player name (usage: %s <char name|account_id>).", original_command);
		return -1;
	}

	if ((pl_sd = map_nick2sd(atcmd_name)) != NULL || ((pl_sd = map_id2sd(atoi(atcmd_name))) != NULL && pl_sd->state.auth)) {
		if (sd->GM_level >= pl_sd->GM_level) { // Only lower or same GM level
			pc_resetstate(pl_sd);
			pc_resetskill(pl_sd);
			sprintf(atcmd_output, msg_txt(208), pl_sd->status.name); // '%s' skill and stats points reseted!
			clif_displaymessage(fd, atcmd_output);
		} else {
			clif_displaymessage(fd, msg_txt(81)); // Your GM level don't authorize you to do this action on this player.
			return -1;
		}
	} else {
		clif_displaymessage(fd, msg_txt(3)); // Character not found.
		return -1;
	}

	return 0;
}

/*==========================================
 * @charstyle - Changes palette/hair styles for a target
 *------------------------------------------
 */
ATCOMMAND_FUNC(charstyle) {
	int hair_style, hair_color, cloth_color;
	struct map_session_data *pl_sd;

	if (!message || !*message || sscanf(message, "%d %d %d %[^\n]", &hair_style, &hair_color, &cloth_color, atcmd_name) < 4 || hair_style < 0 || hair_color < 0 || cloth_color < 0) {
		send_usage(sd, "Please, enter valid styles and a player name (usage: %s <hair ID: %d-%d> <hair color: %d-%d> <clothes color: %d-%d> <name>).", original_command,
		           battle_config.min_hair_style, battle_config.max_hair_style, battle_config.min_hair_color, battle_config.max_hair_color, battle_config.min_cloth_color, battle_config.max_cloth_color);
		return -1;
	}

	if ((pl_sd = map_nick2sd(atcmd_name)) != NULL || ((pl_sd = map_id2sd(atoi(atcmd_name))) != NULL && pl_sd->state.auth)) {
		if (hair_style >= battle_config.min_hair_style && hair_style <= battle_config.max_hair_style &&
		    hair_color >= battle_config.min_hair_color && hair_color <= battle_config.max_hair_color &&
		    cloth_color >= battle_config.min_cloth_color && cloth_color <= battle_config.max_cloth_color) {

			if (!battle_config.clothes_color_for_assassin &&
			    cloth_color != 0 &&
			    pl_sd->status.sex == 1 &&
			    (pl_sd->status.class == 12 ||  pl_sd->status.class == 17)) {
				clif_displaymessage(fd, msg_txt(35)); // You can't use this clothes color with this class.
				return -1;
			} else {
				pc_changelook(pl_sd, LOOK_HAIR, hair_style);
				pc_changelook(pl_sd, LOOK_HAIR_COLOR, hair_color);
				pc_changelook(pl_sd, LOOK_CLOTHES_COLOR, cloth_color);
				clif_displaymessage(fd, msg_txt(36)); // Appearence changed.
			}
		} else {
			clif_displaymessage(fd, msg_txt(37)); // An invalid number was specified.
			return -1;
		}
	} else {
		clif_displaymessage(fd, msg_txt(3)); // Character not found.
		return -1;
	}

	return 0;
}

/*==========================================
 * @charskpoint - Character Skill Point
 *------------------------------------------
 */
ATCOMMAND_FUNC(charskpoint) {
	struct map_session_data *pl_sd;
	int new_skill_point;
	int point;

	if (!message || !*message || sscanf(message, "%d %[^\n]", &point, atcmd_name) < 2 || point == 0) {
		send_usage(sd, "Please, enter a number and a player name (usage: %s <amount> <char name|account_id>).", original_command);
		return -1;
	}

	if ((pl_sd = map_nick2sd(atcmd_name)) != NULL || ((pl_sd = map_id2sd(atoi(atcmd_name))) != NULL && pl_sd->state.auth)) {
		new_skill_point = (int)pl_sd->status.skill_point + point;
		if (point > 0 && (point > 0x7FFF || new_skill_point > 0x7FFF)) // Fix positive overflow
			new_skill_point = 0x7FFF;
		else if (point < 0 && (point < -0x7FFF || new_skill_point < 0)) // Fix negative overflow
			new_skill_point = 0;
		if (new_skill_point != (int)pl_sd->status.skill_point) {
			pl_sd->status.skill_point = new_skill_point;
			clif_updatestatus(pl_sd, SP_SKILLPOINT);
			clif_displaymessage(fd, msg_txt(209)); // Character's number of skill points changed!
		} else {
			if (point < 0)
				clif_displaymessage(fd, msg_txt(41)); // Impossible to decrease the number/value.
			else
				clif_displaymessage(fd, msg_txt(149)); // Impossible to increase the number/value.
			return -1;
		}
	} else {
		clif_displaymessage(fd, msg_txt(3)); // Character not found.
		return -1;
	}

	return 0;
}

/*==========================================
 * @charstpoint - Character Status Point
 *------------------------------------------
 */
ATCOMMAND_FUNC(charstpoint) {
	struct map_session_data *pl_sd;
	int new_status_point;
	int point;

	if (!message || !*message || sscanf(message, "%d %[^\n]", &point, atcmd_name) < 2 || point == 0) {
		send_usage(sd, "Please, enter a number and a player name (usage: %s <amount> <char name|account_id>).", original_command);
		return -1;
	}

	if ((pl_sd = map_nick2sd(atcmd_name)) != NULL || ((pl_sd = map_id2sd(atoi(atcmd_name))) != NULL && pl_sd->state.auth)) {
		new_status_point = (int)pl_sd->status.status_point + point;
		if (point > 0 && (point > 0x7FFF || new_status_point > 0x7FFF)) // Fix positive overflow
			new_status_point = 0x7FFF;
		else if (point < 0 && (point < -0x7FFF || new_status_point < 0)) // Fix negative overflow
			new_status_point = 0;
		if (new_status_point != (int)pl_sd->status.status_point) {
			// If player has max in all stats, don't give status_point
			if (pl_sd->status.str  >= battle_config.max_parameter &&
			    pl_sd->status.agi  >= battle_config.max_parameter &&
			    pl_sd->status.vit  >= battle_config.max_parameter &&
			    pl_sd->status.int_ >= battle_config.max_parameter &&
			    pl_sd->status.dex  >= battle_config.max_parameter &&
			    pl_sd->status.luk  >= battle_config.max_parameter &&
			    new_status_point != 0) {
				pl_sd->status.status_point = 0;
				clif_updatestatus(pl_sd, SP_STATUSPOINT);
				clif_displaymessage(fd, "This player have max in each stat -> status points set to 0.");
				return -1;
			} else {
				pl_sd->status.status_point = new_status_point;
				clif_updatestatus(pl_sd, SP_STATUSPOINT);
				clif_displaymessage(fd, msg_txt(210)); // Character's number of status points changed!
			}
		} else {
			if (point < 0)
				clif_displaymessage(fd, msg_txt(41)); // Impossible to decrease the number/value.
			else
				clif_displaymessage(fd, msg_txt(149)); // Impossible to increase the number/value.
			return -1;
		}
	} else {
		clif_displaymessage(fd, msg_txt(3)); // Character not found.
		return -1;
	}

	return 0;
}

/*==========================================
 * @charzeny - Character Zeny Point
 *------------------------------------------
 */
ATCOMMAND_FUNC(charzeny) {
	struct map_session_data *pl_sd;
	int zeny, new_zeny;

	if (!message || !*message || sscanf(message, "%d %[^\n]", &zeny, atcmd_name) < 2 || zeny == 0) {
		send_usage(sd, "Please, enter a number and a player name (usage: %s <zeny> <char name|account_id>).", original_command);
		return -1;
	}

	if ((pl_sd = map_nick2sd(atcmd_name)) != NULL || ((pl_sd = map_id2sd(atoi(atcmd_name))) != NULL && pl_sd->state.auth)) {
		new_zeny = pl_sd->status.zeny + zeny;
		if (zeny > 0 && (zeny > MAX_ZENY || new_zeny > MAX_ZENY)) // Fix positive overflow
			new_zeny = MAX_ZENY;
		else if (zeny < 0 && (zeny < -MAX_ZENY || new_zeny < 0)) // Fix negative overflow
			new_zeny = 0;
		if (new_zeny != pl_sd->status.zeny) {
			pl_sd->status.zeny = new_zeny;
			clif_updatestatus(pl_sd, SP_ZENY);
			clif_displaymessage(fd, msg_txt(211)); // Character's number of zenys changed!
		} else {
			if (zeny < 0)
				clif_displaymessage(fd, msg_txt(41)); // Impossible to decrease the number/value.
			else
				clif_displaymessage(fd, msg_txt(149)); // Impossible to increase the number/value.
			return -1;
		}
	} else {
		clif_displaymessage(fd, msg_txt(3)); // Character not found.
		return -1;
	}

	return 0;
}

/*==========================================
 * @recallall - Recall All Characters Online To Your Location
 *------------------------------------------
 */
ATCOMMAND_FUNC(recallall) {
	struct map_session_data *pl_sd;
	int i;
	int count;

	if (sd->bl.m >= 0 && map[sd->bl.m].flag.nowarpto && battle_config.any_warp_GM_min_level > sd->GM_level) {
		clif_displaymessage(fd, "You are not authorized to warp somenone to your actual map.");
		return -1;
	}

	count = 0;
	for (i = 0; i < fd_max; i++) {
		if (session[i] && (pl_sd = session[i]->session_data) && pl_sd->state.auth && fd != i &&
		    sd->GM_level >= pl_sd->GM_level) { // Only lower or same level
			if (pl_sd->bl.m >= 0 && map[pl_sd->bl.m].flag.nowarp && battle_config.any_warp_GM_min_level > sd->GM_level)
				count++;
			else
				pc_setpos(pl_sd, sd->mapname, sd->bl.x, sd->bl.y, 2, 0);
		}
	}

	clif_displaymessage(fd, msg_txt(92)); // All characters recalled!
	if (count) {
		sprintf(atcmd_output, "Because you are not authorized to warp from some maps, %d player(s) have not been recalled.", count);
		clif_displaymessage(fd, atcmd_output);
	}

	return 0;
}

/*==========================================
 * @guildrecall - Recall online characters of a guild to your location
 *------------------------------------------
 */
ATCOMMAND_FUNC(guildrecall) {
	struct map_session_data *pl_sd;
	int i;
	char guild_name[100];
	struct guild *g;
	int count;

	memset(guild_name, 0, sizeof(guild_name));

	if (!message || !*message || sscanf(message, "%[^\n]", guild_name) < 1) {
		send_usage(sd, "Please, enter a guild name/id (usage: %s <guild_name/id>).", original_command);
		return -1;
	}

	if (sd->bl.m >= 0 && map[sd->bl.m].flag.nowarpto && battle_config.any_warp_GM_min_level > sd->GM_level) {
		clif_displaymessage(fd, "You are not authorized to warp somenone to your actual map.");
		return -1;
	}

	if ((g = guild_searchname(guild_name)) != NULL || // Name first to avoid error when name begin with a number
	    (g = guild_search(atoi(message))) != NULL) {
		count = 0;
		for (i = 0; i < fd_max; i++) {
			if (session[i] && (pl_sd = session[i]->session_data) && pl_sd->state.auth &&
			    fd != i &&
			    pl_sd->status.guild_id == g->guild_id) {
				if (pl_sd->bl.m >= 0 && map[pl_sd->bl.m].flag.nowarp && battle_config.any_warp_GM_min_level > sd->GM_level)
					count++;
				else
					pc_setpos(pl_sd, sd->mapname, sd->bl.x, sd->bl.y, 2, 0);
			}
		}
		sprintf(atcmd_output, msg_txt(93), g->name); // All online characters of the '%s' guild are near you.
		clif_displaymessage(fd, atcmd_output);
		if (count) {
			sprintf(atcmd_output, "Because you are not authorized to warp from some maps, %d player(s) have not been recalled.", count);
			clif_displaymessage(fd, atcmd_output);
		}
	} else {
		clif_displaymessage(fd, msg_txt(94)); // Incorrect name/ID, or no one from the guild is online.
		return -1;
	}

	return 0;
}

/*==========================================
 * @partyrecall - Recall online characters of a party to your location
 *------------------------------------------
 */
ATCOMMAND_FUNC(partyrecall) {
	int i;
	struct map_session_data *pl_sd;
	char party_name[100];
	struct party *p;
	int count;

	memset(party_name, 0, sizeof(party_name));

	if (!message || !*message || sscanf(message, "%[^\n]", party_name) < 1) {
		send_usage(sd, "Please, enter a party name/id (usage: %s <party_name/id>).", original_command);
		return -1;
	}

	if (sd->bl.m >= 0 && map[sd->bl.m].flag.nowarpto && battle_config.any_warp_GM_min_level > sd->GM_level) {
		clif_displaymessage(fd, "You are not authorized to warp somenone to your actual map.");
		return -1;
	}

	if ((p = party_searchname(party_name)) != NULL || // Name first to avoid error when name begin with a number
	    (p = party_search(atoi(message))) != NULL) {
		count = 0;
		for (i = 0; i < fd_max; i++) {
			if (session[i] && (pl_sd = session[i]->session_data) && pl_sd->state.auth &&
			    fd != i &&
			    pl_sd->status.party_id == p->party_id) {
				if (pl_sd->bl.m >= 0 && map[pl_sd->bl.m].flag.nowarp && battle_config.any_warp_GM_min_level > sd->GM_level)
					count++;
				else
					pc_setpos(pl_sd, sd->mapname, sd->bl.x, sd->bl.y, 2, 0);
			}
		}
		sprintf(atcmd_output, msg_txt(95), p->name); // All online characters of the '%s' party are near you.
		clif_displaymessage(fd, atcmd_output);
		if (count) {
			sprintf(atcmd_output, "Because you are not authorized to warp from some maps, %d player(s) have not been recalled.", count);
			clif_displaymessage(fd, atcmd_output);
		}
	} else {
		clif_displaymessage(fd, msg_txt(96)); // Incorrect name or ID, or no one from the party is online.
		return -1;
	}

	return 0;
}

/*==========================================
 * @reloaditemdb - Reloads item database
 *------------------------------------------
 */
ATCOMMAND_FUNC(reloaditemdb) {
	itemdb_reload();
	clif_displaymessage(fd, msg_txt(97)); // Item database reloaded.

	return 0;
}

/*==========================================
 * @reloadmobdb - Reloads mob database
 *------------------------------------------
 */
ATCOMMAND_FUNC(reloadmobdb) {
	mob_reload();
	clif_displaymessage(fd, msg_txt(98)); // Monster database reloaded.

	return 0;
}

/*==========================================
 * @reloadskilldb - Reloads skill database
 *------------------------------------------
 */
ATCOMMAND_FUNC(reloadskilldb) {
	skill_reload();
	clif_displaymessage(fd, msg_txt(99)); // Skill database reloaded.

	return 0;
}

int atkillnpc_sub(struct block_list *bl, va_list ap) {
	nullpo_retr(0, bl);

	npc_delete((struct npc_data *)bl);

	return 0;
}

void rehash(const int fd, struct map_session_data* sd) {
	int map_id;

	for (map_id = 0; map_id < map_num; map_id++) {
		map_foreachinarea(atkillmonster_sub, map_id, 0, 0, map[map_id].xs, map[map_id].ys, BL_MOB, 0);
		map_foreachinarea(atkillnpc_sub, map_id, 0, 0, map[map_id].xs, map[map_id].ys, BL_NPC, 0);
	}

	return;
}

/*==========================================
 * @reloadscript - Reloads NPC scripts
 *------------------------------------------
 */
ATCOMMAND_FUNC(reloadscript) {
	intif_GMmessage("Freya Server is Rehashing...", 0);
	intif_GMmessage("You will feel a bit of lag at this point !", 0);
	flush_fifos(); // Send message to all players

	rehash(fd, sd);

	intif_GMmessage("Reloading NPCs...", 0);
	flush_fifos(); // Send message to all players

	do_init_npc();
	do_init_script();

	npc_event_do_oninit();

	clif_displaymessage(fd, msg_txt(100)); // Scripts reloaded.

	return 0;
}

/*==========================================
 * @mapinfo <map name> [0-3]
 * => Shows information about the map [map name]
 * 0 = No additional information
 * 1 = Show users in that map and their location
 * 2 = Shows NPCs in that map
 * 3 = Shows the shops/chats in that map
 *------------------------------------------
 */
ATCOMMAND_FUNC(mapinfo) {
	struct map_session_data *pl_sd;
	struct npc_data *nd = NULL;
	struct chat_data *cd = NULL;
	char direction[12];
	int map_id, i, chat_num, list = 0;
	struct block_list *bl;
	int b, mob_num, slave_num;

	memset(atcmd_mapname, 0, sizeof(atcmd_mapname));
	memset(direction, 0, sizeof(direction));

	sscanf(message, "%d %[^\n]", &list, atcmd_mapname);

	if (list < 0 || list > 3) {
		send_usage(sd, "Please, enter at least a valid list number (usage: %s <0-3> [map]).", original_command);
		return -1;
	}

	if (atcmd_mapname[0] == '\0')
		strncpy(atcmd_mapname, sd->mapname, sizeof(atcmd_mapname) - 1);
	if (strstr(atcmd_mapname, ".gat") == NULL && strlen(atcmd_mapname) < 13) // 16 - 4 (.gat)
		strcat(atcmd_mapname, ".gat");

	if ((map_id = map_mapname2mapid(atcmd_mapname)) < 0) { // Only from actual map-server
		clif_displaymessage(fd, msg_txt(1)); // Map not found.
		return -1;
	}

	clif_displaymessage(fd, "------ Map Info ------");
	sprintf(atcmd_output, "Map Name: %s", atcmd_mapname);
	clif_displaymessage(fd, atcmd_output);
	sprintf(atcmd_output, "Players In Map: %d", map[map_id].users);
	clif_displaymessage(fd, atcmd_output);
	sprintf(atcmd_output, "NPCs In Map: %d", map[map_id].npc_num);
	clif_displaymessage(fd, atcmd_output);
	slave_num = 0;
	mob_num = 0;
	for (b = 0; b < map[map_id].bxs * map[map_id].bys; b++)
		for (bl = map[map_id].block_mob[b]; bl; bl = bl->next) {
			mob_num++;
			if (((struct mob_data *)bl)->master_id)
				slave_num++;
		}
	if (slave_num == 0)
		sprintf(atcmd_output, "Mobs In Map: %d (of which is no slave).", mob_num);
	else
		sprintf(atcmd_output, "Mobs In Map: %d (of which are %d slaves).", mob_num, slave_num);
	clif_displaymessage(fd, atcmd_output);
	chat_num = 0;
	for (i = 0; i < fd_max; i++)
		if (session[i] && (pl_sd = session[i]->session_data) && pl_sd->state.auth &&
		    map_id == pl_sd->bl.m &&
		    (cd = (struct chat_data*)map_id2bl(pl_sd->chatID)))
			chat_num++;
	sprintf(atcmd_output, "Chats In Map: %d", chat_num);
	clif_displaymessage(fd, atcmd_output);
	clif_displaymessage(fd, "------ Map Flags ------");
	sprintf(atcmd_output, "Player vs Player: PvP: %s | No Guild: %s | No Party: %s",
	        (map[map_id].flag.pvp) ? "True" : "False",
	        (map[map_id].flag.pvp_noguild) ? "True" : "False",
	        (map[map_id].flag.pvp_noparty) ? "True" : "False");
	clif_displaymessage(fd, atcmd_output);
	sprintf(atcmd_output, "PVP options: nightmare drop: %s | No Calcul Rank: %s",
	        (map[map_id].flag.pvp_nightmaredrop) ? "True" : "False",
	        (map[map_id].flag.pvp_nocalcrank) ? "True" : "False");
	clif_displaymessage(fd, atcmd_output);
	sprintf(atcmd_output, "Guild vs Guild: %s | No Party: %s | GvG Dungeon: %s", (map[map_id].flag.gvg) ? "True" : "False", (map[map_id].flag.gvg_noparty) ? "True" : "False", (map[map_id].flag.gvg_dungeon) ? "True" : "False");
	clif_displaymessage(fd, atcmd_output);
	sprintf(atcmd_output, "No Dead/Bloody Branch: %s", (map[map_id].flag.nobranch) ? "True" : "False"); // Forbid usage of Dead Branch (604), Bloody Branch (12103) and Poring Box (12109)
	clif_displaymessage(fd, atcmd_output);
	sprintf(atcmd_output, "No Memo: %s", (map[map_id].flag.nomemo) ? "True" : "False");
	clif_displaymessage(fd, atcmd_output);
	sprintf(atcmd_output, "No Penalty: %s", (map[map_id].flag.nopenalty) ? "True" : "False");
	clif_displaymessage(fd, atcmd_output);
	sprintf(atcmd_output, "No Zeny Penalty: %s", (map[map_id].flag.nozenypenalty) ? "True" : "False");
	clif_displaymessage(fd, atcmd_output);
	sprintf(atcmd_output, "No Return: %s", (map[map_id].flag.noreturn) ? "True" : "False");
	clif_displaymessage(fd, atcmd_output);
	sprintf(atcmd_output, "No Save: %s", (map[map_id].flag.nosave) ? "True" : "False");
	clif_displaymessage(fd, atcmd_output);
	sprintf(atcmd_output, "No Teleport: %s", (map[map_id].flag.noteleport) ? "True" : "False");
	clif_displaymessage(fd, atcmd_output);
	sprintf(atcmd_output, "No Monster Teleport: %s", (map[map_id].flag.monster_noteleport) ? "True" : "False");
	clif_displaymessage(fd, atcmd_output);
	sprintf(atcmd_output, "Warp options: No Warp: %s | No WarpTo: %s | No Go: %s",
	        (map[map_id].flag.nowarp) ? "True" : "False",
	        (map[map_id].flag.nowarpto) ? "True" : "False",
	        (map[map_id].flag.nogo) ? "True" : "False");
	clif_displaymessage(fd, atcmd_output);
	sprintf(atcmd_output, "Spells and Skills: No Spell: %s | No Skill: %s | No Ice Wall: %s",
	        (map[map_id].flag.nospell) ? "True" : "False",
	        (map[map_id].flag.noskill) ? "True" : "False",
	        (map[map_id].flag.noicewall) ? "True" : "False");
	clif_displaymessage(fd, atcmd_output);
	sprintf(atcmd_output, "No Trade: %s", (map[map_id].flag.notrade) ? "True" : "False");
	clif_displaymessage(fd, atcmd_output);
	sprintf(atcmd_output, "Weathers: Snow: %s | Fog: %s | Sakura: %s | Leaves: %s",
	        (map[map_id].flag.snow) ? "True" : "False",
	        (map[map_id].flag.fog) ? "True" : "False",
	        (map[map_id].flag.sakura) ? "True" : "False",
	        (map[map_id].flag.leaves) ? "True" : "False");
	sprintf(atcmd_output, "Indoors: %s", (map[map_id].flag.indoors) ? "True" : "False");
	clif_displaymessage(fd, atcmd_output);
	if (map[map_id].flag.nogmcmd == 100)
		sprintf(atcmd_output, "GM Commands limits: None (All GM commands can be used).");
	else if (map[map_id].flag.nogmcmd == 0)
		sprintf(atcmd_output, "GM Commands limits: No GM command can be used.");
	else if (map[map_id].flag.nogmcmd == 1)
		sprintf(atcmd_output, "GM Commands limits: only GM commands with level 0 can be used.");
	else
		sprintf(atcmd_output, "GM Commands limits: only GM commands with level 0 to %d can be used.", map[map_id].flag.nogmcmd - 1);
	clif_displaymessage(fd, atcmd_output);
	if (map[map_id].flag.mingmlvl == 100)
		sprintf(atcmd_output, "Minimum GM level to use GM commands: All GM commands are forbiden.");
	else if (map[map_id].flag.mingmlvl == 0)
		sprintf(atcmd_output, "Minimum GM level to use GM commands: None (any player can use GM commands, if the player have access to the command).");
	else
		sprintf(atcmd_output, "Minimum GM level to use GM commands: %d.", map[map_id].flag.mingmlvl);
	clif_displaymessage(fd, atcmd_output);
	sprintf(atcmd_output, "Other Flags: No Base EXP: %s | No Job EXP: %s | No Mob Drop: %s | No MVP Drop: %s",
	        (map[map_id].flag.nobaseexp) ? "True" : "False",
	        (map[map_id].flag.nojobexp) ? "True" : "False",
	        (map[map_id].flag.nomobdrop) ? "True" : "False",
	        (map[map_id].flag.nomvpdrop) ? "True" : "False");
	clif_displaymessage(fd, atcmd_output);

	switch (list) {
	case 0:
		// Do nothing, it's list 0, no additional display
		break;
	case 1:
		clif_displaymessage(fd, "----- Players in Map -----");
		for (i = 0; i < fd_max; i++) {
			if (session[i] && (pl_sd = session[i]->session_data) && pl_sd->state.auth && pl_sd->bl.m == map_id) {
				sprintf(atcmd_output, "Player '%s' (session #%d) | Location: %d,%d",
				        pl_sd->status.name, i, pl_sd->bl.x, pl_sd->bl.y);
				clif_displaymessage(fd, atcmd_output);
			}
		}
		break;
	case 2:
		clif_displaymessage(fd, "----- NPCs in Map -----");
		for (i = 0; i < map[map_id].npc_num; i++) {
			nd = map[map_id].npc[i];
			switch(nd->dir) {
			case 0:  strcpy(direction, "North"); break;
			case 1:  strcpy(direction, "North West"); break;
			case 2:  strcpy(direction, "West"); break;
			case 3:  strcpy(direction, "South West"); break;
			case 4:  strcpy(direction, "South"); break;
			case 5:  strcpy(direction, "South East"); break;
			case 6:  strcpy(direction, "East"); break;
			case 7:  strcpy(direction, "North East"); break;
			case 9:  strcpy(direction, "North"); break;
			default: strcpy(direction, "Unknown"); break;
			}
			sprintf(atcmd_output, "NPC %d (%s): %s | Direction: %s | Sprite: %d | Location: %d %d",
			        (i + 1), nd->exname, nd->name, direction, nd->class, nd->bl.x, nd->bl.y);
			clif_displaymessage(fd, atcmd_output);
		}
		break;
	case 3:
		clif_displaymessage(fd, "----- Chats in Map -----");
		for (i = 0; i < fd_max; i++) {
			if (session[i] && (pl_sd = session[i]->session_data) && pl_sd->state.auth &&
			    pl_sd->bl.m == map_id &&
			    (cd = (struct chat_data*)map_id2bl(pl_sd->chatID)) &&
			    cd->usersd[0] == pl_sd) {
				sprintf(atcmd_output, "Chat %d: %s | Player: %s | Location: %d %d",
				        i, cd->title, pl_sd->status.name, cd->bl.x, cd->bl.y);
				clif_displaymessage(fd, atcmd_output);
				sprintf(atcmd_output, "   Users: %d/%d | Password: %s | Public: %s",
				        cd->users, cd->limit, cd->pass, (cd->pub) ? "Yes" : "No");
				clif_displaymessage(fd, atcmd_output);
			}
		}
		break;
	default: // Normally impossible to arrive here
		send_usage(sd, "Please, enter at least a valid list number (usage: %s <0-3> [map]).", original_command);
		return -1;
		break;
	}

	return 0;
}

/*==========================================
* @mobinfo - Show Monster DB Info
*------------------------------------------
*/
ATCOMMAND_FUNC(mobinfo) {
	unsigned char msize[3][7] = {"Small", "Medium", "Large"};
	unsigned char mrace[12][11] = {"Formless", "Undead", "Beast", "Plant", "Insect", "Fish", "Demon", "Demi-Human", "Angel", "Dragon", "Boss", "Non-Boss"};
	unsigned char melement[11][8] = {"None", "Neutral", "Water", "Earth", "Fire", "Wind", "Poison", "Holy", "Dark", "Ghost", "Undead"};
	char output2[200];
	struct item_data *item_data;
	struct mob_db *mob;
	int mob_id;
	int i, j;
	double temp_rate, rate;
	int drop_rate, max_hp;
	int ratemin, ratemax;

	memset(atcmd_output, 0, sizeof(atcmd_output));
	memset(output2, 0, sizeof(output2));

	if (!message || !*message) {
		send_usage(sd, "Please, enter a Monster/NPC name/id (usage: %s <monster_name_or_monster_ID>).", original_command);
		return -1;
	}

	// If monster identifier/name argument is a name
	if ((mob_id = mobdb_searchname(message)) == 0) // Check name first (To avoid possible name begining by a number)
		mob_id = mobdb_checkid(atoi(message));

	if (mob_id == 0) {
		clif_displaymessage(fd, msg_txt(40)); // Invalid monster ID or name.
		return -1;
	}

	mob = &mob_db[mob_id];

	// Stats
	if (mob->mexp)
		sprintf(atcmd_output, "Monster (MVP): '%s'/'%s' (%d)", mob->name, mob->jname, mob_id);
	else
		sprintf(atcmd_output, "Monster: '%s'/'%s' (%d)", mob->name, mob->jname, mob_id);
	clif_displaymessage(fd, atcmd_output);
	max_hp = mob->max_hp;
	if (mob->mexp > 0) {
		if (battle_config.mvp_hp_rate != 100)
			max_hp = max_hp * battle_config.mvp_hp_rate / 100;
	} else {
		if (battle_config.monster_hp_rate != 100)
			max_hp = max_hp * battle_config.monster_hp_rate / 100;
	}
	sprintf(atcmd_output, " Level:%d  HP:%d  SP:%d  Base EXP:%d  Job EXP:%d", mob->lv, max_hp, mob->max_sp, mob->base_exp, mob->job_exp);
	clif_displaymessage(fd, atcmd_output);
	sprintf(atcmd_output, " DEF:%d  MDEF:%d  STR:%d  AGI:%d  VIT:%d  INT:%d  DEX:%d  LUK:%d", mob->def, mob->mdef, mob->str, mob->agi, mob->vit, mob->int_, mob->dex, mob->luk);
	clif_displaymessage(fd, atcmd_output);
	if (mob->element < 20) {
		// Element - None, Level 0
		i = 0;
		j = 0;
	} else {
		i = mob->element % 20 + 1;
		j = mob->element / 20;
	}
	sprintf(atcmd_output, " ATK:%d~%d  Range:%d~%d~%d  Size:%s  Race: %s  Element: %s (Lv:%d)", mob->atk1, mob->atk2, mob->range, mob->range2 , mob->range3, msize[mob->size], mrace[mob->race], melement[i], j);
	clif_displaymessage(fd, atcmd_output);
	// Drops
	clif_displaymessage(fd, " Drops:");
	strcpy(atcmd_output, " ");
	j = 0;
	for (i = 0; i < 10; i++) {
		if (mob->dropitem[i].nameid <= 0 || (item_data = itemdb_search(mob->dropitem[i].nameid)) == NULL)
			continue;
		// Get value for calculation of drop rate
		switch (itemdb_type(mob->dropitem[i].nameid)) {
		case 0:
			rate = (double)battle_config.item_rate_heal;
			ratemin = battle_config.item_drop_heal_min;
			ratemax = battle_config.item_drop_heal_max;
			break;
		case 2:
			rate = (double)battle_config.item_rate_use;
			ratemin = battle_config.item_drop_use_min;
			ratemax = battle_config.item_drop_use_max;
			break;
		case 4:
		case 5:
		case 8: // Changed to include Pet Equip
			rate = (double)battle_config.item_rate_equip;
			ratemin = battle_config.item_drop_equip_min;
			ratemax = battle_config.item_drop_equip_max;
			break;
		case 6:
			rate = (double)battle_config.item_rate_card;
			ratemin = battle_config.item_drop_card_min;
			ratemax = battle_config.item_drop_card_max;
			break;
		default:
			rate = (double)battle_config.item_rate_common;
			ratemin = battle_config.item_drop_common_min;
			ratemax = battle_config.item_drop_common_max;
			break;
		}
		// Calculation of drop rate
		if (mob->dropitem[i].p <= 0)
			drop_rate = 0;
		else {
			temp_rate = ((double)mob->dropitem[i].p) * rate / 100.;
			if (temp_rate > 10000. || temp_rate < 0 || ((int)temp_rate) > 10000)
				drop_rate = 10000;
			else
				drop_rate = (int)temp_rate;
		}
		// Check minimum/maximum with configuration
		if (drop_rate < ratemin)
			drop_rate = ratemin;
		else if (drop_rate > ratemax)
			drop_rate = ratemax;
		if (drop_rate <= 0 && !battle_config.drop_rate0item)
			drop_rate = 1;
		// Display drops
		if (drop_rate > 0) {
			sprintf(output2, " - %s  %02.02f%%", item_data->name, ((float)drop_rate) / 100.);
			strcat(atcmd_output, output2);
			if (++j % 3 == 0) {
				clif_displaymessage(fd, atcmd_output);
				strcpy(atcmd_output, " ");
			}
		}
	}
	if (j == 0)
		clif_displaymessage(fd, "This monster has no drop.");
	else if (j % 3 != 0)
		clif_displaymessage(fd, atcmd_output);
	// MvP
	if (mob->mexp) {
		sprintf(atcmd_output, " MVP Bonus EXP:%d  %02.02f%%", mob->mexp, (float)mob->mexpper / 100.);
		clif_displaymessage(fd, atcmd_output);
		strcpy(atcmd_output, " MVP Items:");
		j = 0;
		for (i = 0; i < 3; i++) {
			if (mob->mvpitem[i].nameid <= 0 || (item_data = itemdb_search(mob->mvpitem[i].nameid)) == NULL)
				continue;
			// Calculation of drop rate
			if (mob->mvpitem[i].p <= 0)
				drop_rate = 0;
			else {
				// Item type specific droprate modifier
				switch(itemdb_type(mob->mvpitem[i].nameid))
				{
					case 0:		// Healing items
						temp_rate = ((double)mob->mvpitem[i].p) * ((double)battle_config.mvp_healing_rate) / (double)100;
						break;
					case 2:		// Usable items
						temp_rate = ((double)mob->mvpitem[i].p) * ((double)battle_config.mvp_usable_rate) / (double)100;
						break;
					case 4:
					case 5:
					case 8: 	// Equipable items
						temp_rate = ((double)mob->mvpitem[i].p) * ((double)battle_config.mvp_equipable_rate) / (double)100;
						break;
					case 6:		// Cards
						temp_rate = ((double)mob->mvpitem[i].p) * ((double)battle_config.mvp_card_rate) / (double)100;
						break;
					default:	// common items
						temp_rate = ((double)mob->mvpitem[i].p) * ((double)battle_config.mvp_common_rate) / (double)100;
						break;
				}
				if(temp_rate > 10000. || temp_rate < 0 || ((int)temp_rate) > 10000)
					drop_rate = 10000;
				else
					drop_rate = (int)temp_rate;
			}
			// Check minimum/maximum with configuration
			if (drop_rate < battle_config.item_drop_mvp_min)
				drop_rate = battle_config.item_drop_mvp_min;
			else if (drop_rate > battle_config.item_drop_mvp_max)
				drop_rate = battle_config.item_drop_mvp_max;
			if (drop_rate <= 0 && !battle_config.drop_rate0item)
				drop_rate = 1;
			// Display drops
			if (drop_rate > 0) {
				j++;
				if (j == 1)
					sprintf(output2, " %s  %02.02f%%", item_data->name, ((float)drop_rate) / 100.);
				else
					sprintf(output2, " - %s  %02.02f%%", item_data->name, ((float)drop_rate) / 100.);
				strcat(atcmd_output, output2);
			}
		}
		if (j == 0)
			clif_displaymessage(fd, "This monster has no MVP drop.");
		else
			clif_displaymessage(fd, atcmd_output);
	}

	return 0;
}

/*==========================================
 * @mountpeco - Mounts user on a Peco
 *------------------------------------------
 */
ATCOMMAND_FUNC(mount_peco) {
	if (sd->disguise > 0) { // Temporary prevention of crash caused by Peco + disguise
		clif_displaymessage(fd, msg_txt(212)); // Cannot mount a Peco while in disguise.
		return -1;
	}

	if (!pc_isriding(sd)) { // If actually no Peco
		if (sd->status.class ==    7 || sd->status.class ==   14 ||
		    sd->status.class == 4008 || sd->status.class == 4015 ||
		    sd->status.class == 4030 || sd->status.class == 4037) {
			// Normal classes
			if (sd->status.class == 7)
				sd->status.class = sd->view_class = 13;
			else if (sd->status.class == 14)
				sd->status.class = sd->view_class = 21;
			// Advanced classes
			else if (sd->status.class == 4008)
				sd->status.class = sd->view_class = 4014;
			else if (sd->status.class == 4015)
				sd->status.class = sd->view_class = 4022;
			// Baby classes
			else if (sd->status.class == 4030)
				sd->status.class = sd->view_class = 4036;
			else if (sd->status.class == 4037)
				sd->status.class = sd->view_class = 4044;
			pc_setoption(sd, sd->status.option | 0x0020);
			clif_changelook(&sd->bl, LOOK_BASE, sd->view_class); // For button in the equip window
			if (sd->status.clothes_color > 0)
				clif_changelook(&sd->bl, LOOK_CLOTHES_COLOR, sd->status.clothes_color);
			clif_displaymessage(fd, msg_txt(102)); // Mounted Peco.
		} else {
			clif_displaymessage(fd, msg_txt(213)); // You can not mount a peco with your job.
			return -1;
		}
	} else {
		// Normal classes
		if (sd->status.class == 13)
			sd->status.class = sd->view_class = 7;
		else if (sd->status.class == 21)
			sd->status.class = sd->view_class = 14;
		// Advanced classes
		else if (sd->status.class == 4014)
			sd->status.class = sd->view_class = 4008;
		else if (sd->status.class == 4022)
			sd->status.class = sd->view_class = 4015;
		// Baby classes
		else if (sd->status.class == 4036)
			sd->status.class = sd->view_class = 4030;
		else if (sd->status.class == 4044)
			sd->status.class = sd->view_class = 4037;
		pc_setoption(sd, sd->status.option & ~0x0020);
		clif_changelook(&sd->bl, LOOK_BASE, sd->view_class); // For button in the equip window
		if (sd->status.clothes_color > 0)
			clif_changelook(&sd->bl, LOOK_CLOTHES_COLOR, sd->status.clothes_color);
		clif_displaymessage(fd, msg_txt(214)); // Unmounted Peco.
	}

	return 0;
}

/*==========================================
 * @charmountpeco - Mounts a target on a Peco
 *------------------------------------------
 */
ATCOMMAND_FUNC(char_mount_peco) {
	struct map_session_data *pl_sd;

	if (!message || !*message || sscanf(message, "%[^\n]", atcmd_name) < 1) {
		send_usage(sd, "Please, enter a player name (usage: %s <char name|account_id>).", original_command);
		return -1;
	}

	if ((pl_sd = map_nick2sd(atcmd_name)) != NULL || ((pl_sd = map_id2sd(atoi(atcmd_name))) != NULL && pl_sd->state.auth)) {
		if (pl_sd->disguise > 0) { // Temporary prevention of crash caused by Peco + disguise
			clif_displaymessage(fd, msg_txt(215)); // This player cannot mount a Peco while in disguise.
			return -1;
		}

		if (!pc_isriding(pl_sd)) { // If actually no Peco
			if (pl_sd->status.class ==    7 || pl_sd->status.class ==   14 ||
			    pl_sd->status.class == 4008 || pl_sd->status.class == 4015 ||
			    pl_sd->status.class == 4030 || pl_sd->status.class == 4037) {
				// Normal classes
				if (pl_sd->status.class == 7)
					pl_sd->status.class = pl_sd->view_class = 13;
				else if (pl_sd->status.class == 14)
					pl_sd->status.class = pl_sd->view_class = 21;
				// Advanced classes
				else if (pl_sd->status.class == 4008)
					pl_sd->status.class = pl_sd->view_class = 4014;
				else if (pl_sd->status.class == 4015)
					pl_sd->status.class = pl_sd->view_class = 4022;
				// Baby classes
				else if (pl_sd->status.class == 4030)
					pl_sd->status.class = pl_sd->view_class = 4036;
				else if (pl_sd->status.class == 4037)
					pl_sd->status.class = pl_sd->view_class = 4044;
				pc_setoption(pl_sd, pl_sd->status.option | 0x0020);
				clif_changelook(&pl_sd->bl, LOOK_BASE, pl_sd->view_class); // For button in the equip window
				if (pl_sd->status.clothes_color > 0)
					clif_changelook(&pl_sd->bl, LOOK_CLOTHES_COLOR, pl_sd->status.clothes_color);
				clif_displaymessage(fd, msg_txt(216)); // Now, this player mounts a peco.
			} else {
				clif_displaymessage(fd, msg_txt(217)); // This player can not mount a peco with his/her job.
				return -1;
			}
		} else {
			// Normal classes
			if (pl_sd->status.class == 13)
				pl_sd->status.class = pl_sd->view_class = 7;
			else if (pl_sd->status.class == 21)
				pl_sd->status.class = pl_sd->view_class = 14;
			// Advanced classes
			else if (pl_sd->status.class == 4014)
				pl_sd->status.class = pl_sd->view_class = 4008;
			else if (pl_sd->status.class == 4022)
				pl_sd->status.class = pl_sd->view_class = 4015;
			// Baby classes
			else if (pl_sd->status.class == 4036)
				pl_sd->status.class = pl_sd->view_class = 4030;
			else if (pl_sd->status.class == 4044)
				pl_sd->status.class = pl_sd->view_class = 4037;
			pc_setoption(pl_sd, pl_sd->status.option & ~0x0020);
			clif_changelook(&pl_sd->bl, LOOK_BASE, pl_sd->view_class); // For button in the equip window
			if (pl_sd->status.clothes_color > 0)
				clif_changelook(&pl_sd->bl, LOOK_CLOTHES_COLOR, pl_sd->status.clothes_color);
			clif_displaymessage(fd, msg_txt(218)); // Now, this player has not more peco.
		}
	} else {
		clif_displaymessage(fd, msg_txt(3)); // Character not found.
		return -1;
	}

	return 0;
}

/*==========================================
 * @falcon - Gives user a falcon
 *------------------------------------------
 */
ATCOMMAND_FUNC(falcon) {
	if (!pc_isfalcon(sd)) { // If actually no falcon
		pc_setoption(sd, sd->status.option | 0x0010);
		clif_changelook(&sd->bl, LOOK_BASE, sd->view_class); // For button in the equip window
		if (sd->status.clothes_color > 0)
			clif_changelook(&sd->bl, LOOK_CLOTHES_COLOR, sd->status.clothes_color);
		clif_displaymessage(fd, "Falcon is here.");
		if (pc_iscarton(sd) && (sd->status.option & 0x0008) != 0x0008)
			clif_displaymessage(fd, "Falcon can display all carts like a basic cart (type 1).");
	} else {
		pc_setoption(sd, sd->status.option & ~0x0010);
		clif_changelook(&sd->bl, LOOK_BASE, sd->view_class); // For button in the equip window
		if (sd->status.clothes_color > 0)
			clif_changelook(&sd->bl, LOOK_CLOTHES_COLOR, sd->status.clothes_color);
		clif_displaymessage(fd, "Falcon is go on.");
	}

	return 0;
}

/*==========================================
 * @charfalcon - Gives target a falcon
 *------------------------------------------
 */
ATCOMMAND_FUNC(char_falcon) {
	struct map_session_data *pl_sd;

	if (!message || !*message || sscanf(message, "%[^\n]", atcmd_name) < 1) {
		send_usage(sd, "Please, enter a player name (usage: %s <char name|account_id>).", original_command);
		return -1;
	}

	if ((pl_sd = map_nick2sd(atcmd_name)) != NULL || ((pl_sd = map_id2sd(atoi(atcmd_name))) != NULL && pl_sd->state.auth)) {
		if (!pc_isfalcon(pl_sd)) { // If actually no falcon
			pc_setoption(pl_sd, pl_sd->status.option | 0x0010);
			clif_changelook(&pl_sd->bl, LOOK_BASE, pl_sd->view_class); // For button in the equip window
			if (pl_sd->status.clothes_color > 0)
				clif_changelook(&pl_sd->bl, LOOK_CLOTHES_COLOR, pl_sd->status.clothes_color);
			clif_displaymessage(fd, "Now, this player has a falcon.");
			if (pc_iscarton(pl_sd) && (pl_sd->status.option & 0x0008) != 0x0008)
				clif_displaymessage(fd, "Falcon can display all carts like a basic cart (type 1).");
		} else {
			pc_setoption(pl_sd, pl_sd->status.option & ~0x0010);
			clif_changelook(&pl_sd->bl, LOOK_BASE, pl_sd->view_class); // For button in the equip window
			if (pl_sd->status.clothes_color > 0)
				clif_changelook(&pl_sd->bl, LOOK_CLOTHES_COLOR, pl_sd->status.clothes_color);
			clif_displaymessage(fd, "Now, this player has not more falcon.");
		}
	} else {
		clif_displaymessage(fd, msg_txt(3)); // Character not found.
		return -1;
	}

	return 0;
}

/*==========================================
 * @cart - Gives user a cart
 *------------------------------------------
 */
ATCOMMAND_FUNC(cart) {
	short cart;

	if (command[5] != 0) { // cart0, cart1, cart2, cart3, cart4, cart5
		cart = (short)command[5] - 48;
		if (cart < 0 || cart > 5) {
			// Iormaly, it's impossible to enter here
			return -1;
		}
	} else if (!message || !*message || sscanf(message, "%hd", &cart) < 1 || cart < 0 || cart > 5) {
		send_usage(sd, "Please, enter a cart type (usage: %s <cart_type:0-5>).", original_command);
		return -1;
	}

	switch (cart) {
	case 0:
		return atcommand_remove_cart(fd, sd, original_command, "@removecart", "");
	case 1:
		cart = 0x0008;
		break;
	case 2:
		cart = 0x0080;
		break;
	case 3:
		cart = 0x0100;
		break;
	case 4:
		cart = 0x0200;
		break;
	case 5:
		cart = 0x0400;
		break;
	}

	if ((sd->status.option & cart) != cart) { // If not right cart
		if (!pc_iscarton(sd)) { // If actually no cart
			pc_setoption(sd, sd->status.option | cart); // First
			clif_cart_itemlist(sd);
			clif_cart_equiplist(sd);
			clif_updatestatus(sd, SP_CARTINFO);
		} else {
			sd->status.option &= ~((short)CART_MASK); // Suppress actual cart
			pc_setoption(sd, sd->status.option | cart);
		}
		clif_changelook(&sd->bl, LOOK_BASE, sd->view_class); // For button in the equip window
		if (sd->status.clothes_color > 0)
			clif_changelook(&sd->bl, LOOK_CLOTHES_COLOR, sd->status.clothes_color);
		clif_displaymessage(fd, "Now, you have the desired cart.");
		if (pc_isfalcon(sd) && (cart & 0x0008) != 0x0008)
			clif_displaymessage(fd, "Falcon can display all carts like a basic cart (type 1).");
	} else {
		clif_displaymessage(fd, "You already have this cart.");
		return -1;
	}

	return 0;
}

/*==========================================
 * @removecart - Removes user's cart
 *------------------------------------------
 */
ATCOMMAND_FUNC(remove_cart) {
	if (pc_iscarton(sd)) { // If actually a cart
		pc_setoption(sd, sd->status.option & ~CART_MASK);
		clif_changelook(&sd->bl, LOOK_BASE, sd->view_class); // For button in the equip window
		if (sd->status.clothes_color > 0)
			clif_changelook(&sd->bl, LOOK_CLOTHES_COLOR, sd->status.clothes_color);
		clif_displaymessage(fd, "Now, you have no more cart.");
	} else {
		clif_displaymessage(fd, "You already have no cart.");
		return -1;
	}

	return 0;
}

/*==========================================
 * @charcart - Gives a target a cart
 *------------------------------------------
 */
ATCOMMAND_FUNC(char_cart) {
	short cart;
	struct map_session_data *pl_sd;

	if (command[9] != 0) { // charcart0, charcart1, charcart2, charcart3, charcart4, charcart5
		cart = (short)command[9] - 48;
		if (!message || !*message || sscanf(message, "%[^\n]", atcmd_name) < 1 || cart < 0 || cart > 5) {
			send_usage(sd, "Please, enter a cart type and a character name (usage: %s <char name|account_id>).", original_command);
			return -1;
		}
	} else if (!message || !*message || sscanf(message, "%hd %[^\n]", &cart, atcmd_name) < 2 || cart < 0 || cart > 5) {
		send_usage(sd, "Please, enter a cart type and a character name (usage: %s <cart_type:0-5> <char name|account_id>).", original_command);
		return -1;
	}

	if ((pl_sd = map_nick2sd(atcmd_name)) != NULL || ((pl_sd = map_id2sd(atoi(atcmd_name))) != NULL && pl_sd->state.auth)) {
		switch (cart) {
		case 0:
			return atcommand_char_remove_cart(fd, sd, original_command, "@charremovecart", pl_sd->status.name); // Not a common variable to all @ commands
		case 1:
			cart = 0x0008;
			break;
		case 2:
			cart = 0x0080;
			break;
		case 3:
			cart = 0x0100;
			break;
		case 4:
			cart = 0x0200;
			break;
		case 5:
			cart = 0x0400;
			break;
		}

		if ((pl_sd->status.option & cart) != cart) { // If not right cart
			if (!pc_iscarton(pl_sd)) { // If actually no cart
				pc_setoption(pl_sd, pl_sd->status.option | cart); /* first */
				clif_cart_itemlist(pl_sd);
				clif_cart_equiplist(pl_sd);
				clif_updatestatus(pl_sd, SP_CARTINFO);
			} else {
				pl_sd->status.option &= ~((short)CART_MASK); // Suppress actual cart
				pc_setoption(pl_sd, pl_sd->status.option | cart);
			}
			clif_changelook(&pl_sd->bl, LOOK_BASE, pl_sd->view_class); // For button in the equip window
			if (pl_sd->status.clothes_color > 0)
				clif_changelook(&pl_sd->bl, LOOK_CLOTHES_COLOR, pl_sd->status.clothes_color);
			clif_displaymessage(fd, "Now, this player has the desired cart.");
			if (pc_isfalcon(pl_sd) && (cart & 0x0008) != 0x0008)
				clif_displaymessage(fd, "Falcon can display all carts like a basic cart (type 1).");
		} else {
			clif_displaymessage(fd, "This player already has this cart.");
			return -1;
		}
	} else {
		clif_displaymessage(fd, msg_txt(3)); // Character not found.
		return -1;
	}

	return 0;
}

/*==========================================
 * @charemovecart - Removes a target's cart
 *------------------------------------------
 */
ATCOMMAND_FUNC(char_remove_cart) {
	struct map_session_data *pl_sd;

	if (!message || !*message || sscanf(message, "%[^\n]", atcmd_name) < 1) {
		send_usage(sd, "Please, enter a character name (usage: %s <char name|account_id>).", original_command);
		return -1;
	}

	if ((pl_sd = map_nick2sd(atcmd_name)) != NULL || ((pl_sd = map_id2sd(atoi(atcmd_name))) != NULL && pl_sd->state.auth)) {
		if (pc_iscarton(pl_sd)) { // If actually a cart
			pc_setoption(pl_sd, pl_sd->status.option & ~CART_MASK);
			clif_changelook(&pl_sd->bl, LOOK_BASE, pl_sd->view_class); // For button in the equip window
			if (pl_sd->status.clothes_color > 0)
				clif_changelook(&pl_sd->bl, LOOK_CLOTHES_COLOR, pl_sd->status.clothes_color);
			clif_displaymessage(fd, "Now, This player has no more cart.");
		} else {
			clif_displaymessage(fd, "This player already has no cart.");
			return -1;
		}
	} else {
		clif_displaymessage(fd, msg_txt(3)); // Character not found.
		return -1;
	}

	return 0;
}

/*==========================================
 * @guildspy - Spy Commands
 *------------------------------------------
 */
ATCOMMAND_FUNC(guildspy) {
	char guild_name[100];
	struct guild *g;

	memset(guild_name, 0, sizeof(guild_name));

	if (!message || !*message || sscanf(message, "%[^\n]", guild_name) < 1) {
		send_usage(sd, "Please, enter a guild name/id (usage: %s <guild_name/id>).", original_command);
		return -1;
	}

	if ((g = guild_searchname(guild_name)) != NULL || // Name first to avoid error when name begin with a number
	    (g = guild_search(atoi(message))) != NULL) {
		if (sd->guildspy == g->guild_id) {
			sd->guildspy = 0;
			sprintf(atcmd_output, msg_txt(103), g->name); // No longer spying on the '%s' guild.
			clif_displaymessage(fd, atcmd_output);
		} else {
			sd->guildspy = g->guild_id;
			sprintf(atcmd_output, msg_txt(104), g->name); // Spying on the '%s' guild.
			clif_displaymessage(fd, atcmd_output);
		}
	} else {
		clif_displaymessage(fd, msg_txt(94)); // Incorrect name/ID, or no one from the guild is online.
		return -1;
	}

	return 0;
}

/*==========================================
 * @partyspy
 *------------------------------------------
 */
ATCOMMAND_FUNC(partyspy) {
	char party_name[100];
	struct party *p;

	memset(party_name, 0, sizeof(party_name));

	if (!message || !*message || sscanf(message, "%[^\n]", party_name) < 1) {
		send_usage(sd, "Please, enter a party name/id (usage: %s <party_name/id>).", original_command);
		return -1;
	}

	if ((p = party_searchname(party_name)) != NULL || // Name first to avoid error when name begin with a number
	    (p = party_search(atoi(message))) != NULL) {
		if (sd->partyspy == p->party_id) {
			sd->partyspy = 0;
			sprintf(atcmd_output, msg_txt(105), p->name); // No longer spying on the '%s' party.
			clif_displaymessage(fd, atcmd_output);
		} else {
			sd->partyspy = p->party_id;
			sprintf(atcmd_output, msg_txt(106), p->name); // Spying on the '%s' party.
			clif_displaymessage(fd, atcmd_output);
		}
	} else {
		clif_displaymessage(fd, msg_txt(96)); // Incorrect name or ID, or no one from the party is online.
		return -1;
	}

	return 0;
}

/*==========================================
 * @repairall
 *------------------------------------------
 */
ATCOMMAND_FUNC(repairall) {
	int count, i;

	count = 0;
	for (i = 0; i < MAX_INVENTORY; i++) {
		if (sd->status.inventory[i].nameid && sd->status.inventory[i].attribute == 1) {
			sd->status.inventory[i].attribute = 0;
			clif_produceeffect(sd, 0, sd->status.inventory[i].nameid);
			count++;
		}
	}

	if (count > 0) {
		clif_misceffect(&sd->bl, 3);
		clif_equiplist(sd);
		clif_displaymessage(fd, msg_txt(107)); // All items have been repaired.
	} else {
		clif_displaymessage(fd, msg_txt(108)); // No item need to be repaired.
		return -1;
	}

	return 0;
}

/*==========================================
 * @nuke - Nukes a player (Kills target with meteor effect)
 *------------------------------------------
 */
ATCOMMAND_FUNC(nuke) {
	struct map_session_data *pl_sd;

	if (!message || !*message || sscanf(message, "%[^\n]", atcmd_name) < 1) {
		send_usage(sd, "Please, enter a player name (usage: %s <char name|account_id>).", original_command);
		return -1;
	}

	if ((pl_sd = map_nick2sd(atcmd_name)) != NULL || ((pl_sd = map_id2sd(atoi(atcmd_name))) != NULL && pl_sd->state.auth)) {
		if (sd->GM_level >= pl_sd->GM_level) { // Only lower or same GM level
			skill_castend_damage_id(&pl_sd->bl, &pl_sd->bl, NPC_SELFDESTRUCTION, 99, gettick_cache, 0);
			clif_displaymessage(fd, msg_txt(109)); // Player has been nuked!
		} else {
			clif_displaymessage(fd, msg_txt(81)); // Your GM level don't authorize you to do this action on this player.
			return -1;
		}
	} else {
		clif_displaymessage(fd, msg_txt(3)); // Character not found.
		return -1;
	}

	return 0;
}

/*==========================================
 * @enablenpc - Enables an NPC script
 *------------------------------------------
 */
ATCOMMAND_FUNC(enablenpc) {

	char NPCname[100];

	memset(NPCname, 0, sizeof(NPCname));

	if (!message || !*message || sscanf(message, "%[^\n]", NPCname) < 1) {
		send_usage(sd, "Please, enter a NPC name (usage: %s <NPC_name>).", original_command);
		return -1;
	}

	if (npc_name2id(NPCname) != NULL) {
		npc_enable(NPCname, 1);
		clif_displaymessage(fd, msg_txt(110)); // Npc Enabled.
	} else {
		clif_displaymessage(fd, msg_txt(111)); // This NPC doesn't exist.
		return -1;
	}

	return 0;
}

/*==========================================
 * @disablenpc - Disables an NPC script
 *------------------------------------------
 */
ATCOMMAND_FUNC(disablenpc) {

	char NPCname[100];

	memset(NPCname, 0, sizeof(NPCname));

	if (!message || !*message || sscanf(message, "%[^\n]", NPCname) < 1) {
		send_usage(sd, "Please, enter a NPC name (usage: %s <NPC_name>).", original_command);
		return -1;
	}

	if (npc_name2id(NPCname) != NULL) {
		npc_enable(NPCname, 0);
		clif_displaymessage(fd, msg_txt(112)); // Npc Disabled.
	} else {
		clif_displaymessage(fd, msg_txt(111)); // This NPC doesn't exist.
		return -1;
	}

	return 0;
}

/*==========================================
 * Time in txt for time command
 *------------------------------------------
 */
char * txt_time(unsigned int duration) {
	int days, hours, minutes, seconds;
	char temp[MAX_MSG_LEN + 100]; // Max size of msg_txt + security (char name, char id, etc...) (100)
	static char temp1[MAX_MSG_LEN + 100]; // Static: Value is the return value

	memset(temp, 0, sizeof(temp));
	memset(temp1, 0, sizeof(temp1));

//	if (duration < 0) // unsigned int, never < 0
//		duration = 0;

	days = duration / (60 * 60 * 24);
	duration = duration - (60 * 60 * 24 * days);
	hours = duration / (60 * 60);
	duration = duration - (60 * 60 * hours);
	minutes = duration / 60;
	seconds = duration - (60 * minutes);

	if (days == 0) {
		// Do nothing (Temp is void)
	} else if (days > -2 && days < 2) // -1 0 1
		sprintf(temp, msg_txt(219), days); // %d day
	else
		sprintf(temp, msg_txt(220), days); // %d days

	if (hours == 0 && temp[0] == '\0') {
		// Don't add hours if no day (temp1 is void)
	} else if (hours > -2 && hours < 2) // -1 0 1
		sprintf(temp1, msg_txt(221), temp, hours); // %s %d hour
	else
		sprintf(temp1, msg_txt(222), temp, hours); // %s %d hours

	if (minutes > -2 && minutes < 2) // -1 0 1
		sprintf(temp, msg_txt(223), temp1, minutes); // %s %d minute
	else
		sprintf(temp, msg_txt(224), temp1, minutes); // %s %d minutes

	// Always add seconds!
	if (seconds > -2 && seconds < 2) // -1 0 1
		sprintf(temp1, msg_txt(225), temp, seconds); // %s and %d second
	else
		sprintf(temp1, msg_txt(226), temp, seconds); // %s and %d seconds

	return temp1;
}

/*==========================================
 * @time/@date/@server_date/@serverdate/@server_time/@servertime: Display the date/time of the server
 * Calculation management of GM modification (@day/@night GM commands) is done
 *------------------------------------------
 */
ATCOMMAND_FUNC(servertime) {
	struct TimerData * timer_data;
	struct TimerData * timer_data2;
	time_t time_server;  // Variable for number of seconds (Used with time() function)
	struct tm *datetime; // Variable for time in structure ->tm_mday, ->tm_sec, ...

	memset(atcmd_output, 0, sizeof(atcmd_output));

	time(&time_server); // Get time in seconds since 1/1/1970
	datetime = localtime(&time_server); // Convert seconds in structure
	// Like sprintf, but only for date/time (Sunday, November 02 2003 15:12:52)
	strftime(atcmd_output, sizeof(atcmd_output) - 1, msg_txt(230), datetime); // Server time (normal time): %A, %B %d %Y %X.
	clif_displaymessage(fd, atcmd_output);

	if (battle_config.night_duration == 0 && battle_config.day_duration == 0) {
		if (night_flag == 0)
			clif_displaymessage(fd, msg_txt(231)); // Game time: The game is in permanent daylight.
		else
			clif_displaymessage(fd, msg_txt(232)); // Game time: The game is in permanent night.
	} else if (battle_config.night_duration == 0)
		if (night_flag == 1) { // We start with night
			timer_data = get_timer(day_timer_tid);
			sprintf(atcmd_output, msg_txt(233), txt_time((timer_data->tick - gettick_cache) / 1000)); // Game time: The game is actualy in night for %s.
			clif_displaymessage(fd, atcmd_output);
			clif_displaymessage(fd, msg_txt(234)); // Game time: After, the game will be in permanent daylight.
		} else
			clif_displaymessage(fd, msg_txt(231)); // Game time: The game is in permanent daylight.
	else if (battle_config.day_duration == 0)
		if (night_flag == 0) { // We start with day
			timer_data = get_timer(night_timer_tid);
			sprintf(atcmd_output, msg_txt(235), txt_time((timer_data->tick - gettick_cache) / 1000)); // Game time: The game is actualy in daylight for %s.
			clif_displaymessage(fd, atcmd_output);
			clif_displaymessage(fd, msg_txt(236)); // Game time: After, the game will be in permanent night.
		} else
			clif_displaymessage(fd, msg_txt(232)); // Game time: The game is in permanent night.
	else {
		if (night_flag == 0) {
			timer_data = get_timer(night_timer_tid);
			timer_data2 = get_timer(day_timer_tid);
			sprintf(atcmd_output, msg_txt(235), txt_time((timer_data->tick - gettick_cache) / 1000)); // Game time: The game is actualy in daylight for %s.
			clif_displaymessage(fd, atcmd_output);
			if (timer_data->tick > timer_data2->tick)
				sprintf(atcmd_output, msg_txt(237), txt_time((timer_data->interval - abs(timer_data->tick - timer_data2->tick)) / 1000)); // Game time: After, the game will be in night for %s.
			else
				sprintf(atcmd_output, msg_txt(237), txt_time(abs(timer_data->tick - timer_data2->tick) / 1000)); // Game time: After, the game will be in night for %s.
			clif_displaymessage(fd, atcmd_output);
			sprintf(atcmd_output, msg_txt(238), txt_time(timer_data->interval / 1000)); // Game time: A day cycle has a normal duration of %s.
			clif_displaymessage(fd, atcmd_output);
		} else {
			timer_data = get_timer(day_timer_tid);
			timer_data2 = get_timer(night_timer_tid);
			sprintf(atcmd_output, msg_txt(233), txt_time((timer_data->tick - gettick_cache) / 1000)); // Game time: The game is actualy in night for %s.
			clif_displaymessage(fd, atcmd_output);
			if (timer_data->tick > timer_data2->tick)
				sprintf(atcmd_output, msg_txt(239), txt_time((timer_data->interval - abs(timer_data->tick - timer_data2->tick)) / 1000)); // Game time: After, the game will be in daylight for %s.
			else
				sprintf(atcmd_output, msg_txt(239), txt_time(abs(timer_data->tick - timer_data2->tick) / 1000)); // Game time: After, the game will be in daylight for %s.
			clif_displaymessage(fd, atcmd_output);
			sprintf(atcmd_output, msg_txt(238), txt_time(timer_data->interval / 1000)); // Game time: A day cycle has a normal duration of %s.
			clif_displaymessage(fd, atcmd_output);
		}
	}

	return 0;
}

/*==========================================
 * @chardelitem <item_name_or_ID> <quantity> <char name|account_id>
 * Removes <quantity> item from a character
 * Item can be equiped or not.
 *------------------------------------------
 */
ATCOMMAND_FUNC(chardelitem) {

	struct map_session_data *pl_sd;
	char item_name[100];
	int i, number, item_id, item_position, count;
	struct item_data *item_data;

	memset(item_name, 0, sizeof(item_name));

	if (!message || !*message || sscanf(message, "%s %d %[^\n]", item_name, &number, atcmd_name) < 3 || number < 1) {
		send_usage(sd, "Please, enter an item name/id, a quantity and a player name (usage: %s <item_name_or_ID> <quantity> <char name|account_id>).", original_command);
		return -1;
	}

	item_id = 0;
	if ((item_data = itemdb_searchname(item_name)) != NULL ||
	    (item_data = itemdb_exists(atoi(item_name))) != NULL)
		item_id = item_data->nameid;

	if (item_id > 500) {
		if ((pl_sd = map_nick2sd(atcmd_name)) != NULL || ((pl_sd = map_id2sd(atoi(atcmd_name))) != NULL && pl_sd->state.auth)) {
			if (sd->GM_level >= pl_sd->GM_level) { // Only lower or same level
				item_position = pc_search_inventory(pl_sd, item_id);
				if (item_position >= 0) {
					count = 0;
					for(i = 0; i < number && item_position >= 0; i++) {
						pc_delitem(pl_sd, item_position, 1, 0);
						count++;
						item_position = pc_search_inventory(pl_sd, item_id); // For next loop
					}
					if (pl_sd != sd) {
						sprintf(atcmd_output, msg_txt(113), count); // %d item(s) removed by a GM.
						clif_displaymessage(pl_sd->fd, atcmd_output);
					}
					if (number == count)
						sprintf(atcmd_output, msg_txt(114), count); // %d item(s) removed from the player.
					else
						sprintf(atcmd_output, msg_txt(115), count, count, number); // %d item(s) removed. Player had only %d on %d items.
					clif_displaymessage(fd, atcmd_output);
				} else {
					clif_displaymessage(fd, msg_txt(116)); // Character does not have the item.
					return -1;
				}
			} else {
				clif_displaymessage(fd, msg_txt(81)); // Your GM level don't authorize you to do this action on this player.
				return -1;
			}
		} else {
			clif_displaymessage(fd, msg_txt(3)); // Character not found.
			return -1;
		}
	} else {
		clif_displaymessage(fd, msg_txt(19)); // Invalid item ID or name.
		return -1;
	}

	return 0;
}

/*==========================================
 * @jail <time> <char name|account_id>
 * Special warp! No check with nowarp and nowarpto flag
 * Time is done as follows:
 *   Adjustment value (-1, 1, +1, etc...)
 *   Modified element:
 *     a or y: year
 *     m:  month
 *     j or d: day
 *     h:  hour
 *     mn: minute
 *     s:  second
 * <example> @jail +1m-2mn1s-6y test_player
 *           this example adds 1 month and 1 second, and substracts 2 minutes and 6 years at the same time.
 *------------------------------------------
 */
ATCOMMAND_FUNC(jail) {
	char modif[100];
	struct map_session_data *pl_sd;
	int x, y;
	char * modif_p;
	int year, month, day, hour, minute, second, value;
	time_t jail_time;

	memset(modif, 0, sizeof(modif));

	if (!message || !*message || sscanf(message, "%s %[^\n]", modif, atcmd_name) < 2) {
		send_usage(sd, "Please, enter a player name (usage: %s <time> <char name|account_id>).", original_command);
		send_usage(sd, "time usage: adjustement (+/- value) and element (y/a, m, d/j, h, mn, s)");
		send_usage(sd, "Example: %s +1m-2mn1s-6y testplayer", original_command);
		return -1;
	}

	modif_p = modif;
	year = month = day = hour = minute = second = 0;
	while (modif_p[0] != '\0') {
		value = atoi(modif_p);
		if (value == 0)
			modif_p++;
		else {
			if (modif_p[0] == '-' || modif_p[0] == '+')
				modif_p++;
			while (modif_p[0] >= '0' && modif_p[0] <= '9')
				modif_p++;
			if (modif_p[0] == 's') {
				second += value;
				modif_p++;
			} else if (modif_p[0] == 'm' && modif_p[1] == 'n') {
				minute += value;
				modif_p = modif_p + 2;
			} else if (modif_p[0] == 'h') {
				hour += value;
				modif_p++;
			} else if (modif_p[0] == 'd' || modif_p[0] == 'j') {
				day += value;
				modif_p++;
			} else if (modif_p[0] == 'm') {
				month += value;
				modif_p++;
			} else if (modif_p[0] == 'y' || modif_p[0] == 'a') {
				year += value;
				modif_p++;
			} else if (modif_p[0] != '\0') {
				modif_p++;
			}
		}
	}
	if (year == 0 && month == 0 && day == 0 && hour == 0 && minute == 0 && second == 0) {
		clif_displaymessage(fd, msg_txt(625)); // Invalid time for jail command.
		return -1;
	}

	if ((pl_sd = map_nick2sd(atcmd_name)) != NULL || ((pl_sd = map_id2sd(atoi(atcmd_name))) != NULL && pl_sd->state.auth)) {
		if (sd->GM_level >= pl_sd->GM_level) { // Only lower or same GM
			if (map_checkmapname("sec_pri.gat") != -1) {
				time_t timestamp;
				struct tm *tmtime;
				jail_time = (time_t)pc_readglobalreg(pl_sd, "JAIL_TIMER");
				if (jail_time == 0 || jail_time < time(NULL))
					timestamp = time(NULL);
				else
					timestamp = jail_time;
				tmtime = localtime(&timestamp);
				tmtime->tm_year = tmtime->tm_year + year;
				tmtime->tm_mon = tmtime->tm_mon + month;
				tmtime->tm_mday = tmtime->tm_mday + day;
				tmtime->tm_hour = tmtime->tm_hour + hour;
				tmtime->tm_min = tmtime->tm_min + minute;
				tmtime->tm_sec = tmtime->tm_sec + second;
				timestamp = mktime(tmtime);
				if (timestamp != -1) {
					if (timestamp <= time(NULL))
						timestamp = 0;
					if (jail_time != timestamp) {
						if (timestamp != 0) {
							pc_setglobalreg(pl_sd, "JAIL_TIMER", timestamp);
							chrif_save(pl_sd); // Do pc_makesavestatus and save storage + account_reg/account_reg2 too
							// If already in jail
							if (jail_time != 0) {
								// Just change timer value
								addtick_timer(pl_sd->jailtimer, (timestamp - jail_time) * 1000);
								// Send message to GM
								sprintf(atcmd_output, msg_txt(629), (timestamp - jail_time), // Jail time of the player mofified by %+d second%s.
								        ((timestamp - jail_time) > 1 || (timestamp - jail_time) < -1) ? "s" : "");
								clif_displaymessage(fd, atcmd_output);
								sprintf(atcmd_output, msg_txt(633), txt_time(timestamp - time(NULL))); // Character is now in jail for %s.
								clif_displaymessage(fd, atcmd_output);
								// Send message to player
								sprintf(atcmd_output, msg_txt(634), (timestamp - jail_time), // Your jail time has been mofified by %+d second%s.
								        ((timestamp - jail_time) > 1 || (timestamp - jail_time) < -1) ? "s" : "");
								clif_displaymessage(pl_sd->fd, atcmd_output);
								sprintf(atcmd_output, msg_txt(635), txt_time(timestamp - time(NULL))); // You are now in jail for %s.
								clif_displaymessage(pl_sd->fd, atcmd_output);
							// Player is not in jail actually
							} else {
								switch(rand() % 2) {
								case 0:
									x = 24;
									y = 75;
									break;
								default:
									x = 49;
									y = 75;
									break;
								}
								pc_setsavepoint(pl_sd, "sec_pri.gat", x, y); // Save Char Respawn Point in the jail room
								if (pl_sd != sd) {
									sprintf(atcmd_output, msg_txt(117), txt_time(timestamp - time(NULL))); // GM has send you in jails for %s.
									clif_displaymessage(pl_sd->fd, atcmd_output);
								}
								if (pl_sd->jailtimer != -1) // Normally impossible
									delete_timer(pl_sd->jailtimer, pc_jail_timer);
								pl_sd->jailtimer = add_timer(gettick_cache + ((timestamp - time(NULL)) * 1000), pc_jail_timer, pl_sd->bl.id, 0);
								if (pc_setpos(pl_sd, "sec_pri.gat", x, y, 3, 0) == 0) {
									sprintf(atcmd_output, msg_txt(118), txt_time(timestamp - time(NULL))); // Player warped in jails for %s.
									clif_displaymessage(fd, atcmd_output);
									// Send message to all players
									if (battle_config.jail_message) { // Do we send message to ALL players when a player is put in jail?
										sprintf(atcmd_output, msg_txt(640), pl_sd->status.name, txt_time(timestamp - time(NULL))); // %s has been put in jail for %s.
										intif_GMmessage(atcmd_output, 0);
									}
								} else {
									clif_displaymessage(fd, msg_txt(1)); // Map not found.
									return -1;
								}
							}
						} else {
							// Call unjail function
							sprintf(modif, "%s", atcmd_name); // Use local buffer to avoid problem (We call another function that can use global variables)
							return atcommand_unjail(fd, sd, original_command, "@unjail", modif);
						}
					} else {
						clif_displaymessage(fd, msg_txt(628)); // No change done for jail time of this player.
						return -1;
					}
				} else {
					clif_displaymessage(fd, msg_txt(627)); // Invalid final time for jail command.
					return -1;
				}
			} else {
				clif_displaymessage(fd, msg_txt(1)); // Map not found.
				return -1;
			}
		} else {
			clif_displaymessage(fd, msg_txt(81)); // Your GM level don't authorize you to do this action on this player.
			return -1;
		}
	} else {
		clif_displaymessage(fd, msg_txt(3)); // Character not found.
		return -1;
	}

	return 0;
}

/*==========================================
 * @unjail/@discharge <char name|account_id>
 * Special warp! No check with nowarp and nowarpto flag
 *------------------------------------------
 */
ATCOMMAND_FUNC(unjail) {
	struct map_session_data *pl_sd;

	if (!message || !*message || sscanf(message, "%[^\n]", atcmd_name) < 1) {
		send_usage(sd, "Please, enter a player name (usage: %s <char name|account_id>).", original_command);
		return -1;
	}

	if ((pl_sd = map_nick2sd(atcmd_name)) != NULL || ((pl_sd = map_id2sd(atoi(atcmd_name))) != NULL && pl_sd->state.auth)) {
		if (sd->GM_level >= pl_sd->GM_level) { // Only lower or same GM
			if (strcmp(pl_sd->status.last_point.map, "sec_pri.gat") != 0) {
				clif_displaymessage(fd, msg_txt(119)); // This player is not in jails.
				return -1;
			} else {
				if (pl_sd->jailtimer != -1) {
					delete_timer(pl_sd->jailtimer, pc_jail_timer);
					pl_sd->jailtimer = -1;
				}
				pc_setglobalreg(pl_sd, "JAIL_TIMER", 0);
				chrif_save(pl_sd); // Do pc_makesavestatus and save storage + account_reg/account_reg2 too
				pc_setsavepoint(pl_sd, "prontera.gat", 150, 33); // Save char respawn point in Prontera
				// Send message to player
				if (pl_sd != sd)
					clif_displaymessage(pl_sd->fd, msg_txt(120)); // GM has discharge you.
				// Send message to all players
				if (battle_config.jail_discharge_message & 2) { // Do we send message to ALL players when a player is discharged?
					char *discharge_message;
					CALLOC(discharge_message, char, 16 + strlen(msg_txt(632)) + 1); // Name (16) + message (A GM has discharged %s from jail.) + NULL (1)
					sprintf(discharge_message, msg_txt(632), pl_sd->status.name); // A GM has discharged %s from jail.
					intif_GMmessage(discharge_message, 0);
					FREE(discharge_message);
				}
				if (pc_setpos(pl_sd, "prontera.gat", 156, 191, 3, 0) == 0) {
					clif_displaymessage(fd, msg_txt(121)); // Player warped to Prontera.
				} else {
					clif_displaymessage(fd, msg_txt(1)); // Map not found.
					return -1;
				}
			}
		} else {
			clif_displaymessage(fd, msg_txt(81)); // Your GM level don't authorize you to do this action on this player.
			return -1;
		}
	} else {
		clif_displaymessage(fd, msg_txt(3)); // Character not found.
		return -1;
	}

	return 0;
}

/*==========================================
 * @jailtime (Give remaining jail time)
 *------------------------------------------
 */
ATCOMMAND_FUNC(jailtime) {
	time_t jail_time;

	jail_time = (time_t)pc_readglobalreg(sd, "JAIL_TIMER");
	if (jail_time == 0)
		clif_displaymessage(fd, msg_txt(637)); // You are not in jail.
	else {
		sprintf(atcmd_output, msg_txt(636), txt_time(jail_time - time(NULL))); // You are actually in jail for %s.
		clif_displaymessage(fd, atcmd_output);
	}

	return 0;
}

/*==========================================
 * @charjailtime (Give remaining jail time of a player)
 *------------------------------------------
 */
ATCOMMAND_FUNC(charjailtime) {
	struct map_session_data* pl_sd;
	time_t jail_time;

	if (!message || !*message || sscanf(message, "%[^\n]", atcmd_name) < 1) {
		send_usage(sd, "Please, enter a player name (usage: %s <char name|account_id>).", original_command);
		return -1;
	}

	if ((pl_sd = map_nick2sd(atcmd_name)) != NULL || ((pl_sd = map_id2sd(atoi(atcmd_name))) != NULL && pl_sd->state.auth)) {
		if (sd->GM_level >= pl_sd->GM_level) { // Only lower or same level
			jail_time = (time_t)pc_readglobalreg(pl_sd, "JAIL_TIMER");
			if (jail_time == 0)
				clif_displaymessage(fd, msg_txt(639)); // This player is not in jail.
			else {
				sprintf(atcmd_output, msg_txt(638), txt_time(jail_time - time(NULL))); // This player is actually in jail for %s.
				clif_displaymessage(fd, atcmd_output);
			}
		} else {
			clif_displaymessage(fd, msg_txt(81)); // Your GM level don't authorize you to do this action on this player.
			return -1;
		}
	} else {
		clif_displaymessage(fd, msg_txt(3)); // Character not found.
		return -1;
	}

	return 0;
}

/*==========================================
 * @disguise <mob_id>
 *------------------------------------------
 */
ATCOMMAND_FUNC(disguise) {
	int mob_id;

	if (!message || !*message) {
		send_usage(sd, "Please, enter a Monster/NPC name/id (usage: %s <monster_name_or_monster_ID>).", original_command);
		return -1;
	}

	if ((mob_id = mobdb_searchname(message)) == 0) { // Check name first (To avoid possible name begining by a number)
		if ((mob_id = mobdb_checkid(atoi(message))) == 0) {
			mob_id = atoi(message);
			if ((mob_id >= 46 && mob_id <= 125) || (mob_id >= 700 && mob_id <= 947)) {
			} else {
				clif_displaymessage(fd, msg_txt(123)); // Monster/NPC name/id hasn't been found.
				return -1;
			}
		}
	}

	if (pc_isriding(sd)) { // Temporary prevention of crash caused by Peco + disguise
		clif_displaymessage(fd, msg_txt(227)); // Cannot wear disguise while riding a Peco.
		return -1;
	}

	pc_stop_walking(sd, 0);
	clif_clearchar(&sd->bl, 9);
	sd->disguise = mob_id;
	sd->disguiseflag = 1; // Set to override items with disguise script
	clif_changeoption(&sd->bl);
	clif_spawnpc(sd);

	clif_displaymessage(fd, msg_txt(122)); // Disguise applied.

	return 0;
}

/*==========================================
 * @undisguise
 *------------------------------------------
 */
ATCOMMAND_FUNC(undisguise) {
	if (sd->disguise) {
		pc_stop_walking(sd, 0);
		clif_clearchar(&sd->bl, 9);
		sd->disguiseflag = 0; // Reset to override items with disguise script
		sd->disguise = 0;
		clif_changeoption(&sd->bl);
		clif_spawnpc(sd);

		clif_displaymessage(fd, msg_txt(124)); // Undisguise applied.
	} else {
		clif_displaymessage(fd, msg_txt(125)); // You're not disguised.
		return -1;
	}

	return 0;
}

/*==========================================
 * @chardisguise <mob_id> <character>
 *------------------------------------------
 */
ATCOMMAND_FUNC(chardisguise) {
	int mob_id;
	char mob_name[100];
	struct map_session_data* pl_sd;

	memset(mob_name, 0, sizeof(mob_name));

	if (!message || !*message || sscanf(message, "%s %[^\n]", mob_name, atcmd_name) < 2) {
		send_usage(sd, "Please, enter a Monster/NPC name/id and a player name (usage: %s <monster_name_or_monster_ID> <char name|account_id>).", original_command);
		return -1;
	}

	if ((mob_id = mobdb_searchname(mob_name)) == 0) { // Check name first (To avoid possible name begining by a number)
		if ((mob_id = mobdb_checkid(atoi(mob_name))) == 0) {
			mob_id = atoi(mob_name);
			if ((mob_id >= 46 && mob_id <= 125) || (mob_id >= 700 && mob_id <= 947)) {
			} else {
				clif_displaymessage(fd, msg_txt(123)); // Monster/NPC name/id hasn't been found.
				return -1;
			}
		}
	}

	if ((pl_sd = map_nick2sd(atcmd_name)) != NULL || ((pl_sd = map_id2sd(atoi(atcmd_name))) != NULL && pl_sd->state.auth)) {
		if (sd->GM_level >= pl_sd->GM_level) { // Only lower or same level
			if (pc_isriding(pl_sd)) { // Temporary prevention of crash caused by Peco + disguise
				clif_displaymessage(fd, msg_txt(228)); // Character cannot wear disguise while riding a Peco.
				return -1;
			}
			pc_stop_walking(pl_sd, 0);
			clif_clearchar(&pl_sd->bl, 9);
			pl_sd->disguise = mob_id;
			pl_sd->disguiseflag = 1; // Set to override items with disguise script
			clif_changeoption(&pl_sd->bl);
			clif_spawnpc(pl_sd);
			clif_displaymessage(pl_sd->fd, "You have been disguised.");

			clif_displaymessage(fd, msg_txt(140)); // Character's disguise applied.
		} else {
			clif_displaymessage(fd, msg_txt(81)); // Your GM level don't authorize you to do this action on this player.
			return -1;
		}
	} else {
		clif_displaymessage(fd, msg_txt(3)); // Character not found.
		return -1;
	}

	return 0;
}

/*==========================================
 * @charundisguise <character>
 *------------------------------------------
 */
ATCOMMAND_FUNC(charundisguise) {
	struct map_session_data* pl_sd;

	if (!message || !*message || sscanf(message, "%[^\n]", atcmd_name) < 1) {
		send_usage(sd, "Please, enter a player name (usage: %s <char name|account_id>).", original_command);
		return -1;
	}

	if ((pl_sd = map_nick2sd(atcmd_name)) != NULL || ((pl_sd = map_id2sd(atoi(atcmd_name))) != NULL && pl_sd->state.auth)) {
		if (sd->GM_level >= pl_sd->GM_level) { // Only lower or same level
			if (pl_sd->disguise) {
				pc_stop_walking(pl_sd, 0);
				clif_clearchar(&pl_sd->bl, 9);
				pl_sd->disguiseflag = 0; // Reset to override items with disguise script
				pl_sd->disguise = 0;
				clif_changeoption(&pl_sd->bl);
				clif_spawnpc(pl_sd);
				clif_displaymessage(pl_sd->fd, "You have been undisguised.");

				clif_displaymessage(fd, msg_txt(141)); // Character's undisguise applied.
			} else {
				clif_displaymessage(fd, msg_txt(142)); // Character is not disguised.
				return -1;
			}
		} else {
			clif_displaymessage(fd, msg_txt(81)); // Your GM level don't authorize you to do this action on this player.
			return -1;
		}
	} else {
		clif_displaymessage(fd, msg_txt(3)); // Character not found.
		return -1;
	}

	return 0;
}

/*==========================================
 * @changelook
 *------------------------------------------
 */
ATCOMMAND_FUNC(changelook) {
	int i, item_id;
	char item_name[100];
	struct item_data *item_data;

	if (!message || !*message || sscanf(message, "%s", item_name) < 1) {
		send_usage(sd, "Usage: %s <item name | ID>", original_command);
		return -1;
	}

	item_id = 0;
	if ((item_data = itemdb_searchname(item_name)) != NULL ||
	    (item_data = itemdb_exists(atoi(item_name))) != NULL)
		item_id = item_data->nameid;

	if (item_id >= 501) {
		if (item_data->type == 4 || item_data->type == 5) { // 4 = Weapons, 5 = Armors
			i = 0;
			if (item_data->equip & 0x0100)
				i |= LOOK_HEAD_TOP;
			if (item_data->equip & 0x0200)
				i |= LOOK_HEAD_MID;
			if (item_data->equip & 0x0001)
				i |= LOOK_HEAD_BOTTOM;
			if (item_data->equip & 0x0002)
				i |= LOOK_WEAPON;
			if (item_data->equip & 0x0020)
				i |= LOOK_SHIELD;
			if (item_data->equip & 0x0040)
				i |= LOOK_SHOES;
			if (i != 0) {
				clif_changelook(&sd->bl, i, item_data->look);
				clif_displaymessage(fd, "Your look has been changed (if item can be visible).");
				if (i & LOOK_WEAPON || i & LOOK_SHIELD || i & LOOK_SHOES)
					clif_displaymessage(fd, "Changing of Weapons, Shields and Shoes is probably not visible."); // It's a problem inside clif_changelook function (Using what the player has equiped)
			} else {
				clif_displaymessage(fd, "This item is not visible -> Changelook canceled.");
				return -1;
			}
		} else {
			clif_displaymessage(fd, "This item is not an equipment.");
			return -1;
		}
	} else {
		clif_displaymessage(fd, msg_txt(19)); // Invalid item ID or name.
		return -1;
	}

	return 0;
}

/*==========================================
 * @charchangelook
 *------------------------------------------
 */
ATCOMMAND_FUNC(charchangelook) {
	int i, item_id;
	char item_name[100];
	struct item_data *item_data;
	struct map_session_data* pl_sd;

	if (!message || !*message || sscanf(message, "%s %[^\n]", item_name, atcmd_name) < 2) {
		send_usage(sd, "Usage: %s <item name | ID> <char name|account_id>", original_command);
		return -1;
	}

	if ((pl_sd = map_nick2sd(atcmd_name)) != NULL || ((pl_sd = map_id2sd(atoi(atcmd_name))) != NULL && pl_sd->state.auth)) {
		if (sd->GM_level >= pl_sd->GM_level) { // Only lower or same level
			item_id = 0;
			if ((item_data = itemdb_searchname(item_name)) != NULL ||
			    (item_data = itemdb_exists(atoi(item_name))) != NULL)
				item_id = item_data->nameid;

			if (item_id >= 501) {
				if (item_data->type == 4 || item_data->type == 5) { // 4 = Weapons, 5 = Armors
					i = 0;
					if (item_data->equip & 0x0100)
						i |= LOOK_HEAD_TOP;
					if (item_data->equip & 0x0200)
						i |= LOOK_HEAD_MID;
					if (item_data->equip & 0x0001)
						i |= LOOK_HEAD_BOTTOM;
					if (item_data->equip & 0x0002)
						i |= LOOK_WEAPON;
					if (item_data->equip & 0x0020)
						i |= LOOK_SHIELD;
					if (item_data->equip & 0x0040)
						i |= LOOK_SHOES;
					if (i != 0) {
						clif_changelook(&pl_sd->bl, i, item_data->look);
						clif_displaymessage(fd, "Look of the player changed (if item can be visible).");
						if (i & LOOK_WEAPON || i & LOOK_SHIELD || i & LOOK_SHOES)
							clif_displaymessage(fd, "Changing of Weapons, Shields and Shoes is probably not visible."); // It's a problem inside clif_changelook function (Using what the player has equiped)
					} else {
						clif_displaymessage(fd, "This item is not visible -> Changelook canceled.");
						return -1;
					}
				} else {
					clif_displaymessage(fd, "This item is not an equipment.");
					return -1;
				}
			} else {
				clif_displaymessage(fd, msg_txt(19)); // Invalid item ID or name.
				return -1;
			}
		} else {
			clif_displaymessage(fd, msg_txt(81)); // Your GM level don't authorize you to do this action on this player.
			return -1;
		}
	} else {
		clif_displaymessage(fd, msg_txt(3)); // Character not found.
		return -1;
	}

	return 0;
}

/*==========================================
 * @broadcast
 *------------------------------------------
 */
ATCOMMAND_FUNC(broadcast) {
	if (!message || !*message) {
		send_usage(sd, "Please, enter a message (usage: %s <message>).", original_command);
		return -1;
	}

	if (check_bad_word(message, strlen(message), sd))
		return -1; // Check_bad_word function display message if necessary

	sprintf(atcmd_output, "%s: %s", sd->status.name, message);
	intif_GMmessage(atcmd_output, 0);

	return 0;
}

/*==========================================
 * @localbroadcast
 *------------------------------------------
 */
ATCOMMAND_FUNC(localbroadcast) {
	if (!message || !*message) {
		send_usage(sd, "Please, enter a message (usage: %s <message>).", original_command);
		return -1;
	}

	if (check_bad_word(message, strlen(message), sd))
		return -1; // Check_bad_word function display message if necessary

	if (battle_config.atcommand_add_local_message_info)
		sprintf(atcmd_output, "%s: %s %s", sd->status.name, msg_txt(253), message); // Map message
	else
		sprintf(atcmd_output, "%s: %s", sd->status.name, message);
	clif_GMmessage(&sd->bl, atcmd_output, strlen(atcmd_output) + 1, 1); // 1: ALL_SAMEMAP

	return 0;
}

/*==========================================
 * @nlb - (Without name of GM)
 *------------------------------------------
 */
ATCOMMAND_FUNC(localbroadcast2) {
	if (!message || !*message) {
		send_usage(sd, "Please, enter a message (usage: %s <message>).", original_command);
		return -1;
	}

	// Because /b give gm name, we check which sentence we propose for bad words
	if (strncmp(message, sd->status.name, strlen(sd->status.name)) == 0 && message[strlen(sd->status.name)] == ':') {
		// printf("/lb: Check with GM name: %s\n", message);
		if (check_bad_word(message + strlen(sd->status.name) + 1, strlen(message), sd))
			return -1; // Check_bad_word function display message if necessary
	} else {
		// printf("/nlb or /mb: Check without GM name: %s\n", message);
		if (check_bad_word(message, strlen(message), sd))
			return -1; // Check_bad_word function display message if necessary
	}

	if (battle_config.atcommand_add_local_message_info)
		sprintf(atcmd_output, "%s %s", msg_txt(253), message); // Map message
	else
		strcpy(atcmd_output, message);
	clif_GMmessage(&sd->bl, atcmd_output, strlen(atcmd_output) + 1, 1); // 1: ALL_SAMEMAP

	return 0;
}

/*==========================================
 * @ignorelist
 *------------------------------------------
 */
ATCOMMAND_FUNC(ignorelist) {
	int i;

	if (sd->ignoreAll == 0)
		if (sd->ignore_num == 0)
			clif_displaymessage(fd, msg_txt(126)); // You accept any wisp (no wisper is refused).
		else {
			sprintf(atcmd_output, msg_txt(127), sd->ignore_num); // You accept any wisp, except thoses from %d player(s):
			clif_displaymessage(fd, atcmd_output);
		}
	else
		if (sd->ignore_num == 0)
			clif_displaymessage(fd, msg_txt(128)); // You refuse all wisps (no specifical wisper is refused).
		else {
			sprintf(atcmd_output, msg_txt(129), sd->ignore_num); // You refuse all wisps, AND refuse wisps from %d player(s):
			clif_displaymessage(fd, atcmd_output);
		}

	for(i = 0; i < sd->ignore_num; i++)
		clif_displaymessage(fd, sd->ignore[i].name);

	return 0;
}

/*==========================================
 * @charignorelist <player_name>
 *------------------------------------------
 */
ATCOMMAND_FUNC(charignorelist) {
	struct map_session_data *pl_sd;
	int i;

	if (!message || !*message || sscanf(message, "%[^\n]", atcmd_name) < 1) {
		send_usage(sd, "Please, enter a player name (usage: %s <char name|account_id>).", original_command);
		return -1;
	}

	if ((pl_sd = map_nick2sd(atcmd_name)) != NULL || ((pl_sd = map_id2sd(atoi(atcmd_name))) != NULL && pl_sd->state.auth)) {

		if (pl_sd->ignoreAll == 0)
			if (pl_sd->ignore_num == 0) {
				sprintf(atcmd_output, msg_txt(130), pl_sd->status.name); // '%s' accept any wisp (no wisper is refused).
				clif_displaymessage(fd, atcmd_output);
			} else {
				sprintf(atcmd_output, msg_txt(131), pl_sd->status.name, pl_sd->ignore_num); // '%s' accept any wisp, except thoses from %d player(s):
				clif_displaymessage(fd, atcmd_output);
			}
		else
			if (pl_sd->ignore_num == 0) {
				sprintf(atcmd_output, msg_txt(132), pl_sd->status.name); // '%s' refuse all wisps (no specifical wisper is refused).
				clif_displaymessage(fd, atcmd_output);
			} else {
				sprintf(atcmd_output, msg_txt(133), pl_sd->status.name, pl_sd->ignore_num); // '%s' refuse all wisps, AND refuse wisps from %d player(s):
				clif_displaymessage(fd, atcmd_output);
			}

		for(i = 0; i < pl_sd->ignore_num; i++)
			clif_displaymessage(fd, pl_sd->ignore[i].name);

	} else {
		clif_displaymessage(fd, msg_txt(3)); // Character not found.
		return -1;
	}

	return 0;
}

/*==========================================
 * @inall <player_name>
 *------------------------------------------
 */
ATCOMMAND_FUNC(inall) {
	struct map_session_data *pl_sd;

	if (!message || !*message || sscanf(message, "%[^\n]", atcmd_name) < 1) {
		send_usage(sd, "Please, enter a player name (usage: %s <char name|account_id>).", original_command);
		return -1;
	}

	if ((pl_sd = map_nick2sd(atcmd_name)) != NULL || ((pl_sd = map_id2sd(atoi(atcmd_name))) != NULL && pl_sd->state.auth)) {
		if (sd->GM_level >= pl_sd->GM_level) { // Only lower or same level
			if (pl_sd->ignoreAll == 0) {
				sprintf(atcmd_output, msg_txt(134), pl_sd->status.name); // '%s' already accepts all wispers.
				clif_displaymessage(fd, atcmd_output);
				return -1;
			} else {
				pl_sd->ignoreAll = 0;
				sprintf(atcmd_output, msg_txt(135), pl_sd->status.name); // '%s' now accepts all wispers.
				clif_displaymessage(fd, atcmd_output);
				// message to player
				clif_displaymessage(pl_sd->fd, msg_txt(136)); // A GM has authorized all wispers for you.
				WPACKETW(0) = 0x0d2; // R 00d2 <type>.B <fail>.B: type: 0: deny, 1: allow, fail: 0: success, 1: fail
				WPACKETB(2) = 1;
				WPACKETB(3) = 0; // Success
				SENDPACKET(pl_sd->fd, 4); // packet_len_table[0x0d2]
			}
		} else {
			clif_displaymessage(fd, msg_txt(81)); // Your GM level don't authorize you to do this action on this player.
			return -1;
		}
	} else {
		clif_displaymessage(fd, msg_txt(3)); // Character not found.
		return -1;
	}

	return 0;
}

/*==========================================
 * @exall <player_name>
 *------------------------------------------
 */
ATCOMMAND_FUNC(exall) {
	struct map_session_data *pl_sd;

	if (!message || !*message || sscanf(message, "%[^\n]", atcmd_name) < 1) {
		send_usage(sd, "Please, enter a player name (usage: %s <char name|account_id>).", original_command);
		return -1;
	}

	if ((pl_sd = map_nick2sd(atcmd_name)) != NULL || ((pl_sd = map_id2sd(atoi(atcmd_name))) != NULL && pl_sd->state.auth)) {
		if (sd->GM_level >= pl_sd->GM_level) { // Only lower or same level
			if (pl_sd->ignoreAll == 1) {
				sprintf(atcmd_output, msg_txt(137), pl_sd->status.name); // '%s' already blocks all wispers.
				clif_displaymessage(fd, atcmd_output);
				return -1;
			} else {
				pl_sd->ignoreAll = 1;
				sprintf(atcmd_output, msg_txt(138), pl_sd->status.name); // '%s' blocks now all wispers.
				clif_displaymessage(fd, atcmd_output);
				// Message to player
				clif_displaymessage(pl_sd->fd, msg_txt(139)); // A GM has blocked all wispers for you.
				WPACKETW(0) = 0x0d2; // R 00d2 <type>.B <fail>.B: type: 0: deny, 1: allow, fail: 0: success, 1: fail
				WPACKETB(2) = 0;
				WPACKETB(3) = 0; // Success
				SENDPACKET(pl_sd->fd, 4); // packet_len_table[0x0d2]
			}
		} else {
			clif_displaymessage(fd, msg_txt(81)); // Your GM level don't authorize you to do this action on this player.
			return -1;
		}
	} else {
		clif_displaymessage(fd, msg_txt(3)); // Character not found.
		return -1;
	}

	return 0;
}

/*==========================================
 * @email <actual@email> <new@email>
 *------------------------------------------
 */
ATCOMMAND_FUNC(email) {
	char actual_email[100];
	char new_email[100];

	memset(actual_email, 0, sizeof(actual_email));
	memset(new_email, 0, sizeof(new_email));

	if (!message || !*message || sscanf(message, "%s %s", actual_email, new_email) < 2) {
		send_usage(sd, "Please enter 2 emails (usage: %s <actual@email> <new@email>).", original_command);
		return -1;
	}

	if (e_mail_check(actual_email) == 0) {
		clif_displaymessage(fd, msg_txt(144)); // Invalid actual email. If you have default e-mail, give a@a.com.
		return -1;
	} else if (e_mail_check(new_email) == 0) {
		clif_displaymessage(fd, msg_txt(145)); // Invalid new email. Please enter a real e-mail.
		return -1;
	} else if (strcasecmp(new_email, "a@a.com") == 0) {
		clif_displaymessage(fd, msg_txt(146)); // New email must be a real e-mail.
		return -1;
	} else if (strcasecmp(actual_email, new_email) == 0) {
		clif_displaymessage(fd, msg_txt(147)); // New email must be different of the actual e-mail.
		return -1;
	} else {
		chrif_changeemail(sd->status.account_id, actual_email, new_email);
		clif_displaymessage(fd, msg_txt(148)); // Information sent to login-server via char-server.
	}

	return 0;
}

/*==========================================
 * @password <old_password> <new_password>
 *------------------------------------------
 */
ATCOMMAND_FUNC(password) {
	char old_password[100];
	char new_password[100];

	memset(old_password, 0, sizeof(old_password));
	memset(new_password, 0, sizeof(new_password));

	if (!message || !*message || sscanf(message, "%s %s", old_password, new_password) < 2) {
		send_usage(sd, "Please enter 2 passwords (usage: %s <old_password> <new_password>).", original_command);
		return -1;
	}

	if (strcasecmp(old_password, new_password) == 0) {
		clif_displaymessage(fd, msg_txt(672)); // New password must be different from the old password.
		return -1;
	} else {
		chrif_changepassword(sd->status.account_id, old_password, new_password);
		clif_displaymessage(fd, msg_txt(148)); // Information sent to login-server via char-server.
	}

	return 0;
}

/*==========================================
 *@effect - Displays an effect
 *------------------------------------------
 */
ATCOMMAND_FUNC(effect) {
	struct map_session_data *pl_sd;
	int type = 0, flag = 0, i;

	if (!message || !*message || sscanf(message, "%d %d", &type, &flag) < 2) {
		send_usage(sd, "Please, enter at least an option (usage: %s <type> <flag>).", original_command);
		return -1;
	}

	if (flag <= 0) {
		clif_specialeffect(&sd->bl, type, 0); // Flag: 0: Player see in the area (Normal), 1: Only player see only by player, 2: All players in a map that see only their (Not see others), 3: All players that see only their (Not see others)
		clif_displaymessage(fd, msg_txt(229)); // Your effect has changed.
	} else {
		for (i = 0; i < fd_max; i++) {
			if (session[i] && (pl_sd = session[i]->session_data) && pl_sd->state.auth) {
				clif_specialeffect(&pl_sd->bl, type, flag); // Flag: 0: Player see in the area (Normal), 1: Only player see only by player, 2: All players in a map that see only their (Not see others), 3: All players that see only their (Not see others)
				clif_displaymessage(pl_sd->fd, msg_txt(229)); // Your effect has changed.
			}
		}
	}

	return 0;
}

/*==========================================
 * @charitemlist <character>: Displays the list of a player's items.
 *------------------------------------------
 */
ATCOMMAND_FUNC(character_item_list) {
	struct map_session_data *pl_sd;
	struct item_data *item_data, *item_temp;
	int i, j, equip, count, counter, counter2;
	char equipstr[100], outputtmp[200];

	memset(equipstr, 0, sizeof(equipstr));
	memset(outputtmp, 0, sizeof(outputtmp));

	if (!message || !*message || sscanf(message, "%[^\n]", atcmd_name) < 1) {
		send_usage(sd, "Please, enter a player name (usage: %s <char name|account_id>).", original_command);
		return -1;
	}

	if ((pl_sd = map_nick2sd(atcmd_name)) != NULL || ((pl_sd = map_id2sd(atoi(atcmd_name))) != NULL && pl_sd->state.auth)) {
		if (sd->GM_level >= pl_sd->GM_level) { // Only lower or same level
			counter = 0;
			count = 0;
			for (i = 0; i < MAX_INVENTORY; i++) {
				if (pl_sd->status.inventory[i].nameid > 0 && (item_data = itemdb_search(pl_sd->status.inventory[i].nameid)) != NULL) {
					counter = counter + pl_sd->status.inventory[i].amount;
					count++;
					if (count == 1) {
						sprintf(atcmd_output, "------ Items list of '%s' ------", pl_sd->status.name);
						clif_displaymessage(fd, atcmd_output);
					}
					if ((equip = pl_sd->status.inventory[i].equip)) {
						strcpy(equipstr, "| equiped: ");
						if (equip & 4)
							strcat(equipstr, "robe/gargment, ");
						if (equip & 8)
							strcat(equipstr, "left accessory, ");
						if (equip & 16)
							strcat(equipstr, "body/armor, ");
						if ((equip & 34) == 2)
							strcat(equipstr, "right hand, ");
						if ((equip & 34) == 32)
							strcat(equipstr, "left hand, ");
						if ((equip & 34) == 34)
							strcat(equipstr, "both hands, ");
						if (equip & 64)
							strcat(equipstr, "feet, ");
						if (equip & 128)
							strcat(equipstr, "right accessory, ");
						if ((equip & 769) == 1)
							strcat(equipstr, "lower head, ");
						if ((equip & 769) == 256)
							strcat(equipstr, "top head, ");
						if ((equip & 769) == 257)
							strcat(equipstr, "lower/top head, ");
						if ((equip & 769) == 512)
							strcat(equipstr, "mid head, ");
						if ((equip & 769) == 512)
							strcat(equipstr, "lower/mid head, ");
						if ((equip & 769) == 769)
							strcat(equipstr, "lower/mid/top head, ");
						// Remove final ', '
						equipstr[strlen(equipstr) - 2] = '\0';
					} else
						memset(equipstr, 0, sizeof(equipstr));
					if (pl_sd->status.inventory[i].refine)
						sprintf(atcmd_output, "%d %s %+d (%s %+d, id: %d) %s", pl_sd->status.inventory[i].amount, item_data->name, pl_sd->status.inventory[i].refine, item_data->jname, pl_sd->status.inventory[i].refine, pl_sd->status.inventory[i].nameid, equipstr);
					else
						sprintf(atcmd_output, "%d %s (%s, id: %d) %s", pl_sd->status.inventory[i].amount, item_data->name, item_data->jname, pl_sd->status.inventory[i].nameid, equipstr);
					clif_displaymessage(fd, atcmd_output);
					memset(atcmd_output, 0, sizeof(atcmd_output));
					counter2 = 0;
					for (j = 0; j < item_data->slot; j++) {
						if (pl_sd->status.inventory[i].card[j]) {
							if ((item_temp = itemdb_search(pl_sd->status.inventory[i].card[j])) != NULL) {
								if (atcmd_output[0] == '\0')
									sprintf(outputtmp, " -> (card(s): #%d %s (%s), ", ++counter2, item_temp->name, item_temp->jname);
								else
									sprintf(outputtmp, "#%d %s (%s), ", ++counter2, item_temp->name, item_temp->jname);
								strcat(atcmd_output, outputtmp);
							}
						}
					}
					if (atcmd_output[0] != '\0') {
						atcmd_output[strlen(atcmd_output) - 2] = ')';
						atcmd_output[strlen(atcmd_output) - 1] = '\0';
						clif_displaymessage(fd, atcmd_output);
					}
				}
			}
			if (count == 0)
				clif_displaymessage(fd, "No item found on this player.");
			else {
				sprintf(atcmd_output, "%d item(s) found in %d kind(s) of items.", counter, count);
				clif_displaymessage(fd, atcmd_output);
			}
		} else {
			clif_displaymessage(fd, msg_txt(81)); // Your GM level don't authorize you to do this action on this player.
			return -1;
		}
	} else {
		clif_displaymessage(fd, msg_txt(3)); // Character not found.
		return -1;
	}

	return 0;
}

/*==========================================
 * @charstoragelist <character>: Displays the items list of a player's storage.
 *------------------------------------------
 */
ATCOMMAND_FUNC(character_storage_list) {
	struct storage *stor;
	struct map_session_data *pl_sd;
	struct item_data *item_data, *item_temp;
	int i, j, count, counter, counter2;
	char outputtmp[200];

	memset(outputtmp, 0, sizeof(outputtmp));

	if (!message || !*message || sscanf(message, "%[^\n]", atcmd_name) < 1) {
		send_usage(sd, "Please, enter a player name (usage: %s <char name|account_id>).", original_command);
		return -1;
	}

	if ((pl_sd = map_nick2sd(atcmd_name)) != NULL || ((pl_sd = map_id2sd(atoi(atcmd_name))) != NULL && pl_sd->state.auth)) {
		if (sd->GM_level >= pl_sd->GM_level) { // Only lower or same level
			if ((stor = account2storage2(pl_sd->status.account_id)) != NULL) {
				counter = 0;
				count = 0;
				for (i = 0; i < MAX_STORAGE; i++) {
					if (stor->storage[i].nameid > 0 && (item_data = itemdb_search(stor->storage[i].nameid)) != NULL) {
						counter = counter + stor->storage[i].amount;
						count++;
						if (count == 1) {
							sprintf(atcmd_output, "------ Storage items list of '%s' ------", pl_sd->status.name);
							clif_displaymessage(fd, atcmd_output);
						}
						if (stor->storage[i].refine)
							sprintf(atcmd_output, "%d %s %+d (%s %+d, id: %d)", stor->storage[i].amount, item_data->name, stor->storage[i].refine, item_data->jname, stor->storage[i].refine, stor->storage[i].nameid);
						else
							sprintf(atcmd_output, "%d %s (%s, id: %d)", stor->storage[i].amount, item_data->name, item_data->jname, stor->storage[i].nameid);
						clif_displaymessage(fd, atcmd_output);
						memset(atcmd_output, 0, sizeof(atcmd_output));
						counter2 = 0;
						for (j = 0; j < item_data->slot; j++) {
							if (stor->storage[i].card[j]) {
								if ((item_temp = itemdb_search(stor->storage[i].card[j])) != NULL) {
									if (atcmd_output[0] == '\0')
										sprintf(outputtmp, " -> (card(s): #%d %s (%s), ", ++counter2, item_temp->name, item_temp->jname);
									else
										sprintf(outputtmp, "#%d %s (%s), ", ++counter2, item_temp->name, item_temp->jname);
									strcat(atcmd_output, outputtmp);
								}
							}
						}
						if (atcmd_output[0] != '\0') {
							atcmd_output[strlen(atcmd_output) - 2] = ')';
							atcmd_output[strlen(atcmd_output) - 1] = '\0';
							clif_displaymessage(fd, atcmd_output);
						}
					}
				}
				if (count == 0)
					clif_displaymessage(fd, "No item found in the storage of this player.");
				else {
					sprintf(atcmd_output, "%d item(s) found in %d kind(s) of items.", counter, count);
					clif_displaymessage(fd, atcmd_output);
				}
			} else {
				clif_displaymessage(fd, "This player has no storage.");
				return -1;
			}
		} else {
			clif_displaymessage(fd, msg_txt(81)); // Your GM level don't authorize you to do this action on this player.
			return -1;
		}
	} else {
		clif_displaymessage(fd, msg_txt(3)); // Character not found.
		return -1;
	}

	return 0;
}

/*==========================================
 * @charcartlist <character>: Displays the items list of a player's cart.
 *------------------------------------------
 */
ATCOMMAND_FUNC(character_cart_list) {
	struct map_session_data *pl_sd;
	struct item_data *item_data, *item_temp;
	int i, j, count, counter, counter2;
	char outputtmp[200];

	memset(outputtmp, 0, sizeof(outputtmp));

	if (!message || !*message || sscanf(message, "%[^\n]", atcmd_name) < 1) {
		send_usage(sd, "Please, enter a player name (usage: %s <char name|account_id>).", original_command);
		return -1;
	}

	if ((pl_sd = map_nick2sd(atcmd_name)) != NULL || ((pl_sd = map_id2sd(atoi(atcmd_name))) != NULL && pl_sd->state.auth)) {
		if (sd->GM_level >= pl_sd->GM_level) { // Only lower or same level
			counter = 0;
			count = 0;
			for (i = 0; i < MAX_CART; i++) {
				if (pl_sd->status.cart[i].nameid > 0 && (item_data = itemdb_search(pl_sd->status.cart[i].nameid)) != NULL) {
					counter = counter + pl_sd->status.cart[i].amount;
					count++;
					if (count == 1) {
						sprintf(atcmd_output, "------ Cart items list of '%s' ------", pl_sd->status.name);
						clif_displaymessage(fd, atcmd_output);
					}
					if (pl_sd->status.cart[i].refine)
						sprintf(atcmd_output, "%d %s %+d (%s %+d, id: %d)", pl_sd->status.cart[i].amount, item_data->name, pl_sd->status.cart[i].refine, item_data->jname, pl_sd->status.cart[i].refine, pl_sd->status.cart[i].nameid);
					else
						sprintf(atcmd_output, "%d %s (%s, id: %d)", pl_sd->status.cart[i].amount, item_data->name, item_data->jname, pl_sd->status.cart[i].nameid);
					clif_displaymessage(fd, atcmd_output);
					memset(atcmd_output, 0, sizeof(atcmd_output));
					counter2 = 0;
					for (j = 0; j < item_data->slot; j++) {
						if (pl_sd->status.cart[i].card[j]) {
							if ((item_temp = itemdb_search(pl_sd->status.cart[i].card[j])) != NULL) {
								if (atcmd_output[0] == '\0')
									sprintf(outputtmp, " -> (card(s): #%d %s (%s), ", ++counter2, item_temp->name, item_temp->jname);
								else
									sprintf(outputtmp, "#%d %s (%s), ", ++counter2, item_temp->name, item_temp->jname);
								strcat(atcmd_output, outputtmp);
							}
						}
					}
					if (atcmd_output[0] != '\0') {
						atcmd_output[strlen(atcmd_output) - 2] = ')';
						atcmd_output[strlen(atcmd_output) - 1] = '\0';
						clif_displaymessage(fd, atcmd_output);
					}
				}
			}
			if (count == 0)
				clif_displaymessage(fd, "No item found in the cart of this player.");
			else {
				sprintf(atcmd_output, "%d item(s) found in %d kind(s) of items.", counter, count);
				clif_displaymessage(fd, atcmd_output);
			}
		} else {
			clif_displaymessage(fd, msg_txt(81)); // Your GM level don't authorize you to do this action on this player.
			return -1;
		}
	} else {
		clif_displaymessage(fd, msg_txt(3)); // Character not found.
		return -1;
	}

	return 0;
}

/*==========================================
 * @killer
 * Enable killing players even when not in pvp
 *------------------------------------------
 */
ATCOMMAND_FUNC(killer) {
	sd->special_state.killer = !sd->special_state.killer;

	if (sd->special_state.killer)
		clif_displaymessage(fd, msg_txt(260)); // You're now a killer.
	else
		clif_displaymessage(fd, msg_txt(261)); // You're no longer a killer.

	return 0;
}

/*==========================================
 * @charkiller
 * Enable killing players even when not in pvp
 *------------------------------------------
 */
ATCOMMAND_FUNC(charkiller) {
	struct map_session_data *pl_sd;

	if (!message || !*message || sscanf(message, "%[^\n]", atcmd_name) < 1) {
		send_usage(sd, "Please, enter a player name (usage: %s <char name|account_id>).", original_command);
		return -1;
	}

	if ((pl_sd = map_nick2sd(atcmd_name)) != NULL || ((pl_sd = map_id2sd(atoi(atcmd_name))) != NULL && pl_sd->state.auth)) {
		pl_sd->special_state.killer = !pl_sd->special_state.killer;
		if (pl_sd->special_state.killer)
			clif_displaymessage(fd, msg_txt(262)); // The player is now a killer.
		else
			clif_displaymessage(fd, msg_txt(263)); // The player is no longer a killer.
	} else {
		clif_displaymessage(fd, msg_txt(3)); // Character not found.
		return -1;
	}

	return 0;
}

/*==========================================
 * @killable
 * Enable other people killing you
 *------------------------------------------
 */
ATCOMMAND_FUNC(killable) {
	sd->special_state.killable = !sd->special_state.killable;

	if (sd->special_state.killable)
		clif_displaymessage(fd, msg_txt(264)); // You're now killable.
	else
		clif_displaymessage(fd, msg_txt(265)); // You're no longer killable.

	return 0;
}

/*==========================================
 * @charkillable
 * Enable another player to be killed
 *------------------------------------------
 */
ATCOMMAND_FUNC(charkillable) {
	struct map_session_data *pl_sd;

	if (!message || !*message || sscanf(message, "%[^\n]", atcmd_name) < 1) {
		send_usage(sd, "Please, enter a player name (usage: %s <char name|account_id>).", original_command);
		return -1;
	}

	if ((pl_sd = map_nick2sd(atcmd_name)) != NULL || ((pl_sd = map_id2sd(atoi(atcmd_name))) != NULL && pl_sd->state.auth)) {
		pl_sd->special_state.killable = !pl_sd->special_state.killable;
		if (pl_sd->special_state.killable)
			clif_displaymessage(fd, msg_txt(266)); // The player is now killable.
		else
			clif_displaymessage(fd, msg_txt(267)); // The player is no longer killable.
	} else {
		clif_displaymessage(fd, msg_txt(3)); // Character not found.
		return -1;
	}

	return 0;
}

/*==========================================
 * @skillon [map]
 * Turn skills on for the map
 *------------------------------------------
 */
ATCOMMAND_FUNC(skillon) {
	int map_id = 0;

	if (!message || !*message || sscanf(message, "%s", atcmd_mapname) < 1)
		map_id = sd->bl.m;
	else {
		if (strstr(atcmd_mapname, ".gat") == NULL && strlen(atcmd_mapname) < 13) // 16 - 4 (.gat)
			strcat(atcmd_mapname, ".gat");
		if ((map_id = map_mapname2mapid(atcmd_mapname)) < 0) // Only from actual map-server
			map_id = sd->bl.m;
	}

	if (map[map_id].flag.noskill == 0) {
		clif_displaymessage(fd, "Map skills are already on.");
		return -1;
	} else {
		map[map_id].flag.noskill = 0;
		clif_displaymessage(fd, msg_txt(244)); // Map skills are on.
	}

	return 0;
}

/*==========================================
 * @skilloff [map]
 * Turn skills off on the map
 *------------------------------------------
 */
ATCOMMAND_FUNC(skilloff) {
	int map_id = 0;

	if (!message || !*message || sscanf(message, "%s", atcmd_mapname) < 1)
		map_id = sd->bl.m;
	else {
		if (strstr(atcmd_mapname, ".gat") == NULL && strlen(atcmd_mapname) < 13) // 16 - 4 (.gat)
			strcat(atcmd_mapname, ".gat");
		if ((map_id = map_mapname2mapid(atcmd_mapname)) < 0) // Only from actual map-server
			map_id = sd->bl.m;
	}

	if (map[map_id].flag.noskill == 1) {
		clif_displaymessage(fd, "Map skills are already off.");
		return -1;
	} else {
		map[map_id].flag.noskill = 1;
		clif_displaymessage(fd, msg_txt(243)); // Map skills are off.
	}

	return 0;
}

/*==========================================
 * @nospell [map]
 * Nospell flag
 *------------------------------------------
 */
ATCOMMAND_FUNC(nospell) {
	int map_id = 0;

	if (!message || !*message || sscanf(message, "%s", atcmd_mapname) < 1)
		map_id = sd->bl.m;
	else {
		if (strstr(atcmd_mapname, ".gat") == NULL && strlen(atcmd_mapname) < 13) // 16 - 4 (.gat)
			strcat(atcmd_mapname, ".gat");
		if ((map_id = map_mapname2mapid(atcmd_mapname)) < 0) // Only from actual map-server
			map_id = sd->bl.m;
	}

	if (!map[map_id].flag.nospell) {
		map[map_id].flag.nospell = 1;
		clif_displaymessage(fd, "Nospell 'On' on the map.");
	} else {
		map[map_id].flag.nospell = 0;
		clif_displaymessage(fd, "Nospell 'Off' on the map.");
	}

	return 0;
}

/*==========================================
 * @npcmove <New_X> <New_Y> <NPC_name>
 * Move an NPC
 *------------------------------------------
 */
ATCOMMAND_FUNC(npcmove) {
	int x, y;
	struct npc_data *nd = 0;

	if (!message || !*message || sscanf(message, "%d %d %[^\n]", &x, &y, atcmd_name) < 3) {
		send_usage(sd, "Please, enter a NPC name and its new coordinates (usage: %s <New_X> <New_Y> <NPC_name>).", original_command);
		return -1;
	}

	nd = npc_name2id(atcmd_name);
	if (nd == NULL) {
		clif_displaymessage(fd, msg_txt(111)); // This NPC doesn't exist.
		return -1;
	}

	if (x < 0 || x >= map[nd->bl.m].xs || y < 0 || y >= map[nd->bl.m].ys) {
		clif_displaymessage(fd, msg_txt(2)); // Coordinates out of range.
		return -1;
	}

	if (map_getcell(nd->bl.m, x, y, CELL_CHKNOPASS)) {
		clif_displaymessage(fd, msg_txt(279)); // Invalid coordinates (can't move on).
		return -1;
	}

	if (map_getcell(nd->bl.m, x, y, CELL_CHKNPC)) {
		clif_displaymessage(fd, msg_txt(280)); // Invalid coordinates (a NPC is already at this position).
		return -1;
	}

	npc_enable(atcmd_name, 0);
	nd->bl.x = x;
	nd->bl.y = y;
	npc_enable(atcmd_name, 1);
	clif_displaymessage(fd, msg_txt(281)); // NPC moved.

	return 0;
}

/*==========================================
 * @addwarp
 * Create a new static warp point.
 *------------------------------------------
 */
ATCOMMAND_FUNC(addwarp) {
	char w1[100], w3[100], w4[100];
	int x, y;

	if (!message || !*message || sscanf(message, "%s %d %d", atcmd_mapname, &x, &y) < 3) {
		send_usage(sd, "Please, enter a map name with a position (usage: %s <map name> <x_coord> <y_coord>).", original_command);
		return -1;
	}

	if (strstr(atcmd_mapname, ".gat") == NULL && strlen(atcmd_mapname) < 13) // 16 - 4 (.gat)
		strcat(atcmd_mapname, ".gat");

	if (map_checkmapname(atcmd_mapname) == -1) { // If map doesn't exist in all map-servers
		clif_displaymessage(fd, msg_txt(1)); // Map not found.
		return -1;
	}

	sprintf(w1, "%s,%d,%d", sd->mapname, sd->bl.x, sd->bl.y);
	sprintf(w3, "%s%d%d%d%d", atcmd_mapname, sd->bl.x, sd->bl.y, x, y);
	// Fix size of npc name (Nor more than 24 char)
	w3[24] = '\0';
	sprintf(w4, "1,1,%s,%d,%d", atcmd_mapname, x, y);

	if (npc_parse_warp(w1, w3, w4, 0)) {
		clif_displaymessage(fd, "Warp can not be created (invalid coordinates, etc.).");
		return -1;
	} else {
		sprintf(atcmd_output, "New warp NPC => %s", w3);
		clif_displaymessage(fd, atcmd_output);
	}

	return 0;
}

/*==========================================
 * @follow
 * Follow a player (staying no more then 5 spaces away)
 *------------------------------------------
 */
ATCOMMAND_FUNC(follow) {
	struct map_session_data *pl_sd;

	if (!message || !*message || sscanf(message, "%[^\n]", atcmd_name) < 1) {
		send_usage(sd, "Please, enter a player name (usage: %s <char name|account_id> or %s off).", original_command, original_command);
		return -1;
	}

	if (strcasecmp(atcmd_name, "off") == 0) {
		if (sd->followtimer != -1) {
			delete_timer(sd->followtimer, pc_follow_timer);
			sd->followtimer = -1;
			clif_displaymessage(fd, "Follow OFF.");
		} else {
			clif_displaymessage(fd, "You don't follow anybody actually.");
			return -1;
		}
	} else {
		if ((pl_sd = map_nick2sd(atcmd_name)) != NULL || ((pl_sd = map_id2sd(atoi(atcmd_name))) != NULL && pl_sd->state.auth)) {
			if (pl_sd == sd) {
				if (sd->followtimer != -1) {
					delete_timer(sd->followtimer, pc_follow_timer);
					sd->followtimer = -1;
					clif_displaymessage(fd, "Follow OFF.");
				} else {
					clif_displaymessage(fd, "You don't follow anybody actually.");
					return -1;
				}
			} else {
				pc_follow(sd, pl_sd->bl.id);
				send_usage(sd, "To cancel follow GM command, type: %s off.", original_command);
			}
		} else {
			clif_displaymessage(fd, msg_txt(3)); // Character not found.
			return -1;
		}
	}

	return 0;
}

/*==========================================
 * @unfollow
 *------------------------------------------
 */
ATCOMMAND_FUNC(unfollow) {
	if (sd->followtimer != -1) {
		delete_timer(sd->followtimer, pc_follow_timer);
		sd->followtimer = -1;
		clif_displaymessage(fd, "Follow OFF.");
	} else {
		clif_displaymessage(fd, "You don't follow anybody actually.");
		return -1;
	}

	return 0;
}

/*==========================================
 * @chareffect
 * Create a effect localized on another character.
 *------------------------------------------
 */
ATCOMMAND_FUNC(chareffect) {
	struct map_session_data *pl_sd;
	int type = 0;

	if (!message || !*message || sscanf(message, "%d %[^\n]", &type, atcmd_name) < 2) {
		send_usage(sd, "usage: %s <type+> <char name|account_id>.", original_command);
		return -1;
	}

	if ((pl_sd = map_nick2sd(atcmd_name)) != NULL || ((pl_sd = map_id2sd(atoi(atcmd_name))) != NULL && pl_sd->state.auth)) {
		clif_specialeffect(&pl_sd->bl, type, 0); // Flag: 0: Player see in the area (Normal), 1: Only player see only by player, 2: All players in a map that see only their (Not see others), 3: All players that see only their (Not see others)
		clif_displaymessage(fd, msg_txt(229)); // Your effect has changed.
	} else {
		clif_displaymessage(fd, msg_txt(3)); // Character not found.
		return -1;
	}

	return 0;
}

/*==========================================
 * @dropall
 * Drop all your possession on the ground.
 *------------------------------------------
 */
ATCOMMAND_FUNC(dropall) {
	int i;

	for (i = 0; i < MAX_INVENTORY; i++) {
		if (sd->status.inventory[i].amount) {
			if (sd->status.inventory[i].equip != 0)
				pc_unequipitem(sd, i, 3);
			pc_dropitem(sd, i, sd->status.inventory[i].amount);
		}
	}

	return 0;
}

/*==========================================
 * @chardropall
 *------------------------------------------
 */
ATCOMMAND_FUNC(chardropall) {
	struct map_session_data *pl_sd;
	int i;

	if (!message || !*message || sscanf(message, "%[^\n]", atcmd_name) < 1) {
		send_usage(sd, "Please, enter a player name (usage: %s <char name|account_id>).", original_command);
		return -1;
	}

	if ((pl_sd = map_nick2sd(atcmd_name)) != NULL || ((pl_sd = map_id2sd(atoi(atcmd_name))) != NULL && pl_sd->state.auth)) {
		for (i = 0; i < MAX_INVENTORY; i++) {
			if (pl_sd->status.inventory[i].amount) {
				if (pl_sd->status.inventory[i].equip != 0)
					pc_unequipitem(pl_sd, i, 3);
				pc_dropitem(pl_sd, i, pl_sd->status.inventory[i].amount);
			}
		}
		clif_displaymessage(pl_sd->fd, "Look your inventory around of you.");
		clif_displaymessage(fd, "It's done");
	} else {
		clif_displaymessage(fd, msg_txt(3)); // Character not found.
		return -1;
	}

	return 0;
}

/*==========================================
 * @storeall
 * Put everything into storage to simplify your inventory to make
 * debugging easier.
 *------------------------------------------
 */
ATCOMMAND_FUNC(storeall) {
	short i;

	if (storage_storageopen(sd) == 1) {
		clif_displaymessage(fd, "No storage found. Run this command again.");
		return -1;
	}

	for (i = 0; i < MAX_INVENTORY; i++) {
		if (sd->status.inventory[i].amount) {
			if (sd->status.inventory[i].equip != 0)
				pc_unequipitem(sd, i, 3);
			storage_storageadd(sd, i, sd->status.inventory[i].amount);
		}
	}
	storage_storageclose(sd);

	clif_displaymessage(fd, "It is done");

	return 0;
}

/*==========================================
 * @charstoreall
 *------------------------------------------
 */
ATCOMMAND_FUNC(charstoreall) {
	struct map_session_data *pl_sd;
	short i;

	if (!message || !*message || sscanf(message, "%[^\n]", atcmd_name) < 1) {
		send_usage(sd, "Please, enter a player name (usage: %s <char name|account_id>).", original_command);
		return -1;
	}

	if ((pl_sd = map_nick2sd(atcmd_name)) != NULL || ((pl_sd = map_id2sd(atoi(atcmd_name))) != NULL && pl_sd->state.auth)) {
		if (storage_storageopen(pl_sd) == 1) {
			clif_displaymessage(fd, "Had to open the characters storage window...");
			clif_displaymessage(fd, "Run this command again.");
			return -1;
		}
		for (i = 0; i < MAX_INVENTORY; i++) {
			if (pl_sd->status.inventory[i].amount) {
				if (pl_sd->status.inventory[i].equip != 0)
					pc_unequipitem(pl_sd, i, 3);
				storage_storageadd(pl_sd, i, pl_sd->status.inventory[i].amount);
			}
		}
		storage_storageclose(pl_sd);
		clif_displaymessage(pl_sd->fd, "Everything you own has been put away for safe keeping.");
		clif_displaymessage(pl_sd->fd, "Go to the nearest kafra to retrieve it.");
		clif_displaymessage(pl_sd->fd, "   -- the management");
		clif_displaymessage(fd, "It's done");
	} else {
		clif_displaymessage(fd, msg_txt(3)); // Character not found.
		return -1;
	}

	return 0;
}

/*==========================================
 * @skillid
 * Look up a skill by its name.
 *------------------------------------------
 */
ATCOMMAND_FUNC(skillid) {
	int mes_len, idx = 0;

	if (!message || !*message)
		return -1;

	mes_len = strlen(message);
	while (skill_names[idx].id != 0) {
		if (strncasecmp(skill_names[idx].name, message, mes_len) == 0 ||
		    strncasecmp(skill_names[idx].desc, message, mes_len) == 0) {
			sprintf(atcmd_output, "skill %d: %s", skill_names[idx].id, skill_names[idx].desc);
			clif_displaymessage(fd, atcmd_output);
			break;
		}
		idx++;
	}

	if (skill_names[idx].id == 0) {
		clif_displaymessage(fd, "Unknown skill");
		return -1;
	}

	return 0;
}

/*==========================================
 * @useskill
 * A way of using skills without having to find them in the skills menu.
 *------------------------------------------
 */
ATCOMMAND_FUNC(useskill) {
	struct map_session_data *pl_sd;
	int skillnum;
	int skilllv;
	int inf;

	if (!message || !*message || sscanf(message, "%d %d %[^\n]", &skillnum, &skilllv, atcmd_name) < 3) {
		send_usage(sd, "Usage: %s <skillnum> <skillv> <char name|account_id>", original_command);
		return -1;
	}

	if ((pl_sd = map_nick2sd(atcmd_name)) != NULL || ((pl_sd = map_id2sd(atoi(atcmd_name))) != NULL && pl_sd->state.auth)) {
		inf = skill_get_inf(skillnum);
		if (inf == 2 || inf == 32) // 2- Place, 32- Trap
			skill_use_pos(sd, pl_sd->bl.x, pl_sd->bl.y, skillnum, skilllv);
		else
			skill_use_id(sd, pl_sd->bl.id, skillnum, skilllv);
	} else {
		clif_displaymessage(fd, msg_txt(3)); // Character not found.
		return -1;
	}

	return 0;
}

/*==========================================
 * @skilltree
 * Prints the skill tree for a player required to get to a skill
 *------------------------------------------
 */
ATCOMMAND_FUNC(skilltree) {
	struct map_session_data *pl_sd;
	int skillnum, skillidx;
	int meets, i, c, s;
	struct pc_base_job s_class;
	struct skill_tree_entry *ent;

	if (!message || !*message || sscanf(message, "%d %[^\r\n]", &skillnum, atcmd_name) < 2) {
		send_usage(sd, "Usage: %s <skillnum:1+> <char name|account_id>", original_command);
		return -1;
	}

	if (skillnum >= 10000 && skillnum < 10015) // Guild skills
		skillnum -= 9100;
	if (skillnum <= 0 || skillnum > MAX_SKILL) {
		clif_displaymessage(fd, "Unknown skill number.");
		return -1;
	}

	if ((pl_sd = map_nick2sd(atcmd_name)) != NULL || ((pl_sd = map_id2sd(atoi(atcmd_name))) != NULL && pl_sd->state.auth)) {
		s_class = pc_calc_base_job(pl_sd->status.class);
		c = s_class.job;
		s = s_class.upper;

		c = pc_calc_skilltree_normalize_job(c, s, pl_sd);

		sprintf(atcmd_output, "Player is using %s %s skill tree (%d basic points)", (s_class.upper == 2) ? "baby" : ((s_class.upper) ? "upper" : "lower"), job_name(c), pc_checkskill(pl_sd, 1));
		clif_displaymessage(fd, atcmd_output);

		i = 0;
		for (skillidx = 0; skillidx < MAX_SKILL_PER_TREE && skill_tree[s][c][skillidx].id > 0; skillidx++)
			if (skill_tree[s][c][skillidx].id == skillnum) {
				i = 1;
				break;
			}

		if (i == 0) {
			clif_displaymessage(fd, "I do not believe the player can use that skill.");
			return 0;
		}

		ent = &skill_tree[s][c][skillidx];

		meets = 1;
		for(i = 0; i < 5; i++)
			if (ent->need[i].id && pc_checkskill(sd, ent->need[i].id) < ent->need[i].lv) {
				int idx = 0;
				char *desc;
				while (skill_names[idx].id != 0 && skill_names[idx].id != ent->need[i].id)
					idx++;
				if (skill_names[idx].id == 0)
					desc = "Unknown skill";
				else
					desc = skill_names[idx].desc;
				sprintf(atcmd_output, "player requires level %d of skill %s", ent->need[i].lv, desc);
				clif_displaymessage(fd, atcmd_output);
				meets = 0;
			}

		if (meets == 1) {
			clif_displaymessage(fd, "I believe the player meets all the requirements for that skill.");
		}
	} else {
		clif_displaymessage(fd, msg_txt(3)); // Character not found.
		return -1;
	}

	return 0;
}

/*==========================================
 * @marry
 * Marry two players.
 *------------------------------------------
 */
ATCOMMAND_FUNC(marry) {
	struct map_session_data *pl_sd1;
	struct map_session_data *pl_sd2;
	char player1[100], player2[100];

	if (!message || !*message ||
	    (sscanf(message, "\"%[^\"]\",\"%[^\"]\"", player1, player2) < 2 &&
	     sscanf(message, "%[^,],\"%[^\"]\"", player1, player2) < 2 &&
	     sscanf(message, "\"%[^\"]\",%[^\r\n]", player1, player2) < 2 &&
	     sscanf(message, "%[^,],%[^\r\n]", player1, player2) < 2)) {
		send_usage(sd, "usage: %s \"<player1>\",\"<player2>\" or %s <player1>,<player2>.", original_command, original_command);
		return -1;
	}

	if((pl_sd1 = map_nick2sd(player1)) == NULL && ((pl_sd1 = map_id2sd(atoi(player1))) == NULL || !pl_sd1->state.auth)) {
		sprintf(atcmd_output, "Cannot find player %s online.", player1);
		clif_displaymessage(fd, atcmd_output);
		return -1;
	}

	if((pl_sd2 = map_nick2sd(player2)) == NULL && ((pl_sd2 = map_id2sd(atoi(player2))) == NULL || !pl_sd2->state.auth)) {
		sprintf(atcmd_output, "Cannot find player %s online.", player2);
		clif_displaymessage(fd, atcmd_output);
		return -1;
	}

	if (pl_sd1->status.partner_id > 0 || pl_sd2->status.partner_id > 0) {
		clif_displaymessage(fd, "This marriage is impossible. One of these characters is already married!");
		return -1;
	}

	if (pc_marriage(pl_sd1, pl_sd2)) {
		clif_displaymessage(fd, "The marriage has failed. Class of the character is incorrect!");
		return -1;
	}

	clif_displaymessage(fd, "Marriage done. Congratulations to the newly wed.");

	return 0;
}

/*==========================================
 * @divorce
 * Divorce two players.
 *------------------------------------------
 */
ATCOMMAND_FUNC(divorce) {
	struct map_session_data *pl_sd1;
	struct map_session_data *pl_sd2;

	if (!message || !*message || sscanf(message, "%[^\n]", atcmd_name) < 1) {
		send_usage(sd, "Please, enter a player name (usage: %s <char name|account_id>).", original_command);
		return -1;
	}

	if ((pl_sd1 = map_nick2sd(atcmd_name)) != NULL || ((pl_sd1 = map_id2sd(atoi(atcmd_name))) != NULL && pl_sd1->state.auth)) {
		if (pl_sd1->status.partner_id == 0) {
			clif_displaymessage(fd, "This player is not married.");
			return -1;
		}
		if ((pl_sd2 = map_nick2sd(map_charid2nick(pl_sd1->status.partner_id))) != NULL) {
			if (pc_divorce(pl_sd1)) {
				clif_displaymessage(fd, "The divorce has failed. Probably incorrect partner (bug?).");
				return -1;
			}
			clif_displaymessage(fd, "Divorce done.");
		} else {
			clif_displaymessage(fd, "The partner of this player is not online.");
			return -1;
		}
	} else {
		clif_displaymessage(fd, msg_txt(3)); // Character not found.
		return -1;
	}

	return 0;
}

/*==========================================
 * @rings
 * Give two players rings.
 *------------------------------------------
 */
ATCOMMAND_FUNC(rings) {
	struct item item_tmp;
	int flag;

	memset(&item_tmp, 0, sizeof(struct item));

	item_tmp.nameid = 2634;
	item_tmp.amount = 1;
	item_tmp.identify = 1;
	if ((flag = pc_additem(sd, &item_tmp, 1))) {
		clif_additem(sd, 0, 0, flag);
		clif_displaymessage(fd, "You cannot receive the Wedding Ring of Groom (id:2634) (overcharged?).");
		return -1;
	}

	memset(&item_tmp, 0, sizeof(struct item));
	item_tmp.nameid = 2635;
	item_tmp.amount = 1;
	item_tmp.identify = 1;
	if ((flag = pc_additem(sd, &item_tmp, 1))) {
		clif_additem(sd, 0, 0, flag);
		clif_displaymessage(fd, "You can not receive the Wedding Ring of Bride (id:2635) (overcharged?).");
		return -1;
	}

	// Message is not necessary, because display is done when ring arrive in the inventory
	clif_displaymessage(fd, "You have rings! Give them to the lovers.");

	return 0;
}

/*==========================================
 * @snow - Causes snow client effect on a map
 *------------------------------------------
 */
ATCOMMAND_FUNC(snow) {
	struct map_session_data *pl_sd;
	int i;

	if (map[sd->bl.m].flag.snow) {
		map[sd->bl.m].flag.snow = 0;
		for (i = 0; i < fd_max; i++)
			if (session[i] && (pl_sd = session[i]->session_data) && pl_sd->state.auth)
				clif_displaymessage(pl_sd->fd, "Snow has stopped. If you want remove weather from your screen, warp you, change map, etc.");
	} else {
		map[sd->bl.m].flag.snow = 1;
		clif_specialeffect(&sd->bl, 162, 2); // Flag: 0: Player see in the area (Normal), 1: Only player see only by player, 2: All players in a map that see only their (Not see others), 3: All players that see only their (Not see others)
	}

	return 0;
}

/*==========================================
 * @sakura - Causes sakura leaves client effect on a map
 *------------------------------------------
 */
ATCOMMAND_FUNC(sakura) {
	struct map_session_data *pl_sd;
	int i;

	if (map[sd->bl.m].flag.sakura) {
		map[sd->bl.m].flag.sakura = 0;
		for (i = 0; i < fd_max; i++)
			if (session[i] && (pl_sd = session[i]->session_data) && pl_sd->state.auth)
				clif_displaymessage(pl_sd->fd, "Cherry tree leaves has stopped. If you want remove weather from your screen, warp you, change map, etc.");
	} else {
		map[sd->bl.m].flag.sakura = 1;
		clif_specialeffect(&sd->bl, 163, 2); // Flag: 0: Player see in the area (Normal), 1: Only player see only by player, 2: All players in a map that see only their (Not see others), 3: All players that see only their (Not see others)
	}

	return 0;
}

/*==========================================
 * @fog - Causes fog client effect on a map
 *------------------------------------------
 */
ATCOMMAND_FUNC(fog) {
	struct map_session_data *pl_sd;
	int i;

	if (map[sd->bl.m].flag.fog) {
		map[sd->bl.m].flag.fog = 0;
		for (i = 0; i < fd_max; i++)
			if (session[i] && (pl_sd = session[i]->session_data) && pl_sd->state.auth)
				clif_displaymessage(pl_sd->fd, "The fog has stopped. If you want remove weather from your screen, warp you, change map, etc.");
	} else {
		map[sd->bl.m].flag.fog = 1;
		clif_specialeffect(&sd->bl, 233, 2); // Flag: 0: Player see in the area (Normal), 1: Only player see only by player, 2: All players in a map that see only their (Not see others), 3: All players that see only their (Not see others)
	}

	return 0;
}

/*==========================================
 * @leaves - Causes falling leaves client effect on a map
 *------------------------------------------
 */
ATCOMMAND_FUNC(leaves) {
	struct map_session_data *pl_sd;
	int i;

	if (map[sd->bl.m].flag.leaves) {
		map[sd->bl.m].flag.leaves = 0;
		for (i = 0; i < fd_max; i++)
			if (session[i] && (pl_sd = session[i]->session_data) && pl_sd->state.auth)
				clif_displaymessage(pl_sd->fd, "Leaves no longer fall. If you want remove weather from your screen, warp you, change map, etc.");
	} else {
		map[sd->bl.m].flag.leaves = 1;
		clif_specialeffect(&sd->bl, 333, 2); // Flag: 0: Player see in the area (Normal), 1: Only player see only by player, 2: All players in a map that see only their (Not see others), 3: All players that see only their (Not see others)
	}

	return 0;
}

/*==========================================
 * @clsweather - Clears all weather effects (@leaves, @snow, etc)
 *------------------------------------------
 */
ATCOMMAND_FUNC(clsweather) {
	struct map_session_data *pl_sd;
	int i;

	if (map[sd->bl.m].flag.snow || map[sd->bl.m].flag.sakura || map[sd->bl.m].flag.leaves || map[sd->bl.m].flag.fog) {
		map[sd->bl.m].flag.snow = 0;
		map[sd->bl.m].flag.sakura = 0;
		map[sd->bl.m].flag.leaves = 0;
		map[sd->bl.m].flag.fog = 0;
		clif_displaymessage(fd, "All special weathers are now removed from your map.");
		for (i = 0; i < fd_max; i++)
			if (session[i] && (pl_sd = session[i]->session_data) && pl_sd->state.auth) {
				clif_displaymessage(pl_sd->fd, "Special weather of the map has been removed.");
				clif_displaymessage(pl_sd->fd, "If you want remove weather from your screen, warp you, change map, etc.");
			}
	} else {
		clif_displaymessage(fd, "There is not special weather on your map.");
		return -1;
	}

	return 0;
}

/*===============================================================
 * @sounds - Sound Command - Plays a sound for everyone!
 *---------------------------------------------------------------
 */
ATCOMMAND_FUNC(sound) {
	char sound_file[100];
	char sound_file2[150];

	memset(sound_file, 0, sizeof(sound_file));
	memset(sound_file2, 0, sizeof(sound_file2));

	if(!message || !*message || sscanf(message, "%s", sound_file) < 1) {
		send_usage(sd, "Please, enter a sound filename. (usage: %s <filename>)", original_command);
		return -1;
	}

	if (strstr(sound_file, ".wav") == NULL)
		strcat(sound_file, ".wav");

	if (strlen(sound_file) > 24) {
		sprintf(atcmd_output, "File name must not have more than 24 characters ('%s' has %d char).", sound_file, strlen(sound_file));
		clif_displaymessage(fd, atcmd_output);
		return -1;
	}

	sprintf(sound_file2, "data\\wav\\%s", sound_file);
	if (filelist_find(sound_file2) == NULL) {
		sprintf(sound_file2, "data\\wav\\effect\\%s", sound_file);
		if (strlen(sound_file) <= (24 - 7) && filelist_find(sound_file2) != NULL) { // 7 = effect-
			strncpy(sound_file, sound_file2 + 9, 24); // 9 = data-wav-
		} else {
			sprintf(atcmd_output, "'%s' is an unknown wav file.", sound_file);
			clif_displaymessage(fd, atcmd_output);
			return -1;
		}
	}

	clif_soundeffectall(&sd->bl, sound_file, 0);

	return 0;
}

/*==========================================
 * @mailbox
 *------------------------------------------
 */
ATCOMMAND_FUNC(mailbox) {

	clif_openmailbox(sd->fd);

	return 0;
}

/*==========================================
 * @mobsearch
 *------------------------------------------
 */
ATCOMMAND_FUNC(mobsearch) {
	char monster[100];
	int mob_id;
	struct mob_data *md;
	struct block_list *bl;
	int b, mob_num, slave_num;

	memset(monster, 0, sizeof(monster));

	mob_id = -1;
	if (sscanf(message, "%s", monster) == 1) {
		// If monster identifier/name argument is a name
		if ((mob_id = mobdb_searchname(monster)) == 0) // Check name first (To avoid possible name begining by a number)
			if ((mob_id = mobdb_checkid(atoi(monster))) == 0) {
				if (atoi(monster) != -1) {
					clif_displaymessage(fd, msg_txt(40)); // Invalid monster ID or name.
					return -1;
				} else
					mob_id = -1;
			}
	}

	if (mob_id == -1)
		sprintf(atcmd_output, "Mob Search: all monsters on %s", sd->mapname);
	else
		sprintf(atcmd_output, "Mob Search: '%s' on %s", mob_db[mob_id].jname, sd->mapname);
	clif_displaymessage(fd, atcmd_output);

	slave_num = 0;
	mob_num = 0;
	for (b = 0; b < map[sd->bl.m].bxs * map[sd->bl.m].bys; b++)
		for (bl = map[sd->bl.m].block_mob[b]; bl; bl = bl->next) {
			md = (struct mob_data *)bl;
			if (md && md->name[0] && (mob_id == -1 || md->class == mob_id)) {
				if (mob_db[md->class].max_hp <= 0)
					sprintf(atcmd_output, "'%s' (class: %d-%s%s) at %d,%d", md->name, md->class, mob_db[md->class].jname, (md->master_id > 0) ? ", slave)" : "", bl->x, bl->y);
				else if (md->hp <= 0)
					sprintf(atcmd_output, "'%s' (class: %d-%s%s) at %d,%d [dead]", md->name, md->class, mob_db[md->class].jname, (md->master_id > 0) ? ", slave)" : "", bl->x, bl->y);
				else
					sprintf(atcmd_output, "'%s' (class: %d-%s%s) at %d,%d - hp: %d/%d", md->name, md->class, mob_db[md->class].jname, (md->master_id > 0) ? ", slave)" : "", bl->x, bl->y, md->hp, mob_db[md->class].max_hp);
				clif_displaymessage(fd, atcmd_output);
				mob_num++;
				if (md->master_id)
					slave_num++;
			}
		}
	if (mob_num == 0) {
		clif_displaymessage(fd, "No such mob found.");
	} else if (mob_num == 1) {
		sprintf(atcmd_output, "%d mob found.", mob_num);
		clif_displaymessage(fd, atcmd_output);
	} else {
		if (slave_num == 0)
			sprintf(atcmd_output, "%d mobs found (of which is no slave).", mob_num);
		else
			sprintf(atcmd_output, "%d mobs found (of which are %d slaves).", mob_num, slave_num);
		clif_displaymessage(fd, atcmd_output);
	}

	return 0;
}

/*==========================================
 * @cleanmap - Clears all drops on a map
 *------------------------------------------
 */
static int atcommand_cleanmap_sub(struct block_list *bl, va_list ap) {
	nullpo_retr(0, bl);

	map_clearflooritem(bl->id);

	return 0;
}

ATCOMMAND_FUNC(cleanmap) {
	struct map_data *m;
	
	nullpo_retr(1, m = &map[sd->bl.m]);
		
	map_foreachinarea(atcommand_cleanmap_sub, sd->bl.m, 0, 0, m->xs, m->ys, BL_ITEM);
	clif_displaymessage(fd, "All dropped items have been cleaned up.");

	return 0;
}

/*==========================================
 * @cleanmap - Clears all drops in a selected area
 *------------------------------------------
 */

ATCOMMAND_FUNC(cleanarea) {
	int area_size;

	area_size = AREA_SIZE * 2;
	sscanf(message, "%d", &area_size);
	if (area_size < 1)
		area_size = 1;

	map_foreachinarea(atcommand_cleanmap_sub, sd->bl.m,
	                  sd->bl.x - area_size, sd->bl.y - area_size,
	                  sd->bl.x + area_size, sd->bl.y + area_size,
	                  BL_ITEM);
	clif_displaymessage(fd, "All dropped items have been cleaned up.");

	return 0;
}

/*==========================================
 * @adjcmdlvl/@setcmdlvl
 *
 * Temp adjust the GM level required to use a GM command
 *
 * Used during beta testing to allow players to use GM commands
 * for short periods of time
 *------------------------------------------
 */
ATCOMMAND_FUNC(adjcmdlvl) {
	int i, newlev;
	char cmd[100];

	if (!message || !*message || sscanf(message, "%d %s", &newlev, cmd) != 2) {
		send_usage(sd, "usage: %s <lvl> <command>.", original_command);
		return -1;
	}

	for (i = 0; atcommand_info[i].type != AtCommand_Unknown; i++)
		if (strcasecmp(cmd, atcommand_info[i].command + 1) == 0) {
			atcommand_info[i].level = newlev;
			clif_displaymessage(fd, "@command level changed.");
			return 0;
		}

	clif_displaymessage(fd, "@command not found.");

	return -1;
}

/*==========================================
 * @adjgmlvl/@setgmlvl
 *
 * Create a temp GM
 *
 * Used during beta testing to allow players to use GM commands
 * for short periods of time
 *------------------------------------------
 */
ATCOMMAND_FUNC(adjgmlvl) {
	int newlev;
	struct map_session_data *pl_sd;

	if (!message || !*message || sscanf(message, "%d %[^\r\n]", &newlev, atcmd_name) != 2 || newlev < 0 || newlev > 99) {
		send_usage(sd, "usage: %s <lvl:0-99> <player>.", original_command);
		return -1;
	}

	if (newlev > sd->GM_level) // Cannot give upper GM level than its level
		newlev = sd->GM_level;

	if ((pl_sd = map_nick2sd(atcmd_name)) != NULL || ((pl_sd = map_id2sd(atoi(atcmd_name))) != NULL && pl_sd->state.auth)) {
		if (sd->GM_level > pl_sd->GM_level || sd == pl_sd) { // Only lower or same GM level (you can change TEMPORARILY you own GM level for some tests)
			if (pl_sd->GM_level != newlev) {
				sprintf(atcmd_output, "GM level of the player temporarily changed from %d to %d.", pl_sd->GM_level, newlev);
				clif_displaymessage(fd, atcmd_output);
				pc_set_gm_level(pl_sd->status.account_id, (unsigned char)newlev);
			} else {
				clif_displaymessage(fd, "This player already has this GM level.");
				return -1;
			}
		} else {
			clif_displaymessage(fd, msg_txt(81)); // Your GM level don't authorize you to do this action on this player.
			return -1;
		}
	} else {
		clif_displaymessage(fd, msg_txt(3)); // Character not found.
		return -1;
	}

	return 0;
}

/*==========================================
 * @adjgmlvl2/@setgmlvl2
 *
 * Permanently modify a GM level of a player!
 *------------------------------------------
 */
ATCOMMAND_FUNC(adjgmlvl2) {
	int newlev;
	struct map_session_data *pl_sd;

	if (command[6] >= '0' && command[6] <= '9') {
		newlev = atoi(command + 6);
		if (!message || !*message || sscanf(message, "%[^\r\n]", atcmd_name) != 1) {
			send_usage(sd, "usage: %s <player>.", original_command);
			return -1;
		}
	} else {
		if (!message || !*message || sscanf(message, "%d %[^\r\n]", &newlev, atcmd_name) != 2 || newlev < 0 || newlev > 99) {
			send_usage(sd, "usage: %s <lvl:0-99> <player>.", original_command);
			return -1;
		}
	}

	if (newlev > sd->GM_level) // Cannot give upper GM level than its level
		newlev = sd->GM_level;

	// Try to foun dplayer online to check immediatly
	if ((pl_sd = map_nick2sd(atcmd_name)) != NULL || ((pl_sd = map_id2sd(atoi(atcmd_name))) != NULL && pl_sd->state.auth)) {
		if (sd != pl_sd) { // You are not authorized to change your own GM level PERMANENTLY
			if (sd->GM_level > pl_sd->GM_level) { // only lower GM level
				if (pl_sd->GM_level == newlev) {
					clif_displaymessage(fd, "This player already has this GM level.");
					return -1;
				}
				// We can try to modify
			} else {
				clif_displaymessage(fd, msg_txt(81)); // Your GM level don't authorize you to do this action on this player.
				return -1;
			}
		} else {
			clif_displaymessage(fd, "You can not change your own GM level permanently.");
			return -1;
		}
	} // Else if not found, we can try to modify

	// If not stopped before, try to modify
	chrif_char_ask_name(sd->status.account_id, atcmd_name, 6, newlev, 0, 0, 0, 0, 0); // Type: 6 - Change GM level
	clif_displaymessage(fd, msg_txt(88)); // Character name sends to char-server to ask it.

	return 0;
}

/*==========================================
 * @trade
 *
 * Open a trade window with a remote player
 *------------------------------------------
 */
ATCOMMAND_FUNC(trade) {
	struct map_session_data *pl_sd;

	if (!message || !*message || sscanf(message, "%[^\n]", atcmd_name) < 1) {
		send_usage(sd, "Please, enter a player name (Usage: %s <char name|account_id>).", original_command);
		return -1;
	}

	if ((pl_sd = map_nick2sd(atcmd_name)) != NULL || ((pl_sd = map_id2sd(atoi(atcmd_name))) != NULL && pl_sd->state.auth)) {
		trade_traderequest(sd, pl_sd->bl.id, 0); // 1: Check if near the other player, 0: Doesn't check
	} else {
		clif_displaymessage(fd, msg_txt(3)); // Character not found.
		return -1;
	}

	return 0;
}

/*==========================================
 * @setbattleflag/@adjbattleflag/@setbattleconf/@adjbattleconf
 *
 * Set a battle_config flag without having to reboot
 *------------------------------------------
 */
ATCOMMAND_FUNC(setbattleflag) {
	char flag[100], value[100];

	if (!message || !*message || sscanf(message, "%s %s", flag, value) < 2) {
		send_usage(sd, "usage: %s <flag> <value>.", original_command);
		return -1;
	}

	if (battle_set_value(flag, value) == 0)
		clif_displaymessage(fd, "unknown battle_config flag.");
	else
		clif_displaymessage(fd, "battle_config set as requested.");

	return 0;
}

/*==========================================
 * @setmapflag/@adjmapflag
 *
 * Set a map flag without having to reboot
 *------------------------------------------
 */
ATCOMMAND_FUNC(setmapflag) {
	char mapflag[100], mapflaglower[100], option[100];
	int i, map_id;

	memset(option, 0, sizeof(option));
	memset(mapflaglower, 0, sizeof(mapflaglower));

	if (!message || !*message || sscanf(message, "%s %s %[^\n]", atcmd_mapname, mapflag, option) < 2) {
		send_usage(sd, "usage: %s <map> <mapflag> [option|value].", original_command);
		return -1;
	}

	if (strstr(atcmd_mapname, ".gat") == NULL && strlen(atcmd_mapname) < 13) // 16 - 4 (.gat)
		strcat(atcmd_mapname, ".gat");

	if ((map_id = map_mapname2mapid(atcmd_mapname)) >= 0) { // Only from actual map-server
		for(i = 0; mapflag[i]; i++)
			mapflaglower[i] = tolower((unsigned char)mapflag[i]); // tolower needs unsigned char
		// Use specific GM command if it exists
		if (strcmp(mapflaglower, "pvp") == 0 && map_id == sd->bl.m)
			atcommand_pvpon(fd, sd, original_command, "@pvpon", "");
		else if (strcmp(mapflaglower, "gvg") == 0 && map_id == sd->bl.m)
			atcommand_gvgon(fd, sd, original_command, "@gvgon", "");
		else if (strcmp(mapflaglower, "noskill") == 0 && map_id == sd->bl.m)
			atcommand_skilloff(fd, sd, original_command, "@skilloff", "");
		else if (strcmp(mapflaglower, "snow") == 0 && map_id == sd->bl.m && !map[map_id].flag.snow)
			atcommand_snow(fd, sd, original_command, "@snow", "");
		else if (strcmp(mapflaglower, "fog") == 0 && map_id == sd->bl.m && !map[map_id].flag.fog)
			atcommand_fog(fd, sd, original_command, "@fog", "");
		else if (strcmp(mapflaglower, "sakura") == 0 && map_id == sd->bl.m && !map[map_id].flag.sakura)
			atcommand_sakura(fd, sd, original_command, "@sakura", "");
		else if (strcmp(mapflaglower, "leaves") == 0 && map_id == sd->bl.m && !map[map_id].flag.leaves)
			atcommand_leaves(fd, sd, original_command, "@leaves", "");
		else if (strcmp(mapflaglower, "nospell") == 0 && map_id == sd->bl.m && !map[map_id].flag.nospell)
			atcommand_nospell(fd, sd, original_command, "@nospell", "");
		// General option
		else if (npc_parse_mapflag(atcmd_mapname, mapflag, option, 0) == 1 && npc_parse_mapflag(atcmd_mapname, mapflaglower, option, 0) == 1) {
			clif_displaymessage(fd, "unknown map flag.");
			return -1;
		} else
			clif_displaymessage(fd, "map flag set as requested.");
	} else {
		clif_displaymessage(fd, msg_txt(1)); // Map not found.
		return -1;
	}

	return 0;
}

/*==========================================
 * @uptime - Shows server uptime
 *------------------------------------------
 */
ATCOMMAND_FUNC(uptime) {
	sprintf(atcmd_output, msg_txt(245), txt_time(time(NULL) - start_time)); // Server Uptime: %s.
	clif_displaymessage(fd, atcmd_output);

	return 0;
}

/*==========================================
 * @clock - Shows server's current time
 *------------------------------------------
 */
ATCOMMAND_FUNC(clock) {
	time_t time_server;  // Variable for number of seconds (Used with time() function)
	struct tm *datetime; // Variable for time in structure ->tm_mday, ->tm_sec, ...

	time(&time_server); // Get time in seconds since 1/1/1970
	datetime = localtime(&time_server); // convert seconds in structure
	// Like sprintf, but only for date/time (Sunday, November 02 2003 15:12:52)
	memset(atcmd_output, 0, sizeof(atcmd_output));
	strftime(atcmd_output, sizeof(atcmd_output) - 1, msg_txt(230), datetime); // Server time (normal time): %A, %B %d %Y %X.
	clif_displaymessage(fd, atcmd_output);
	sprintf(atcmd_output, msg_txt(245), txt_time(time(NULL) - start_time)); // Server Uptime: %s.
	clif_displaymessage(fd, atcmd_output);

	return 0;
}

/*==============================
 * @unmute - Removes mute status
 *------------------------------
*/
ATCOMMAND_FUNC(unmute) {
	struct map_session_data *pl_sd;

	if (!message || !*message || sscanf(message, "%[^\n]", atcmd_name) < 1) {
		send_usage(sd, "Please, enter a player name (usage: %s <char name|account_id>).", original_command);
		return -1;
	}

	if ((pl_sd = map_nick2sd(atcmd_name)) != NULL || ((pl_sd = map_id2sd(atoi(atcmd_name))) != NULL && pl_sd->state.auth)) {
		if (sd->GM_level >= pl_sd->GM_level) { // Only lower or same level
			if (pl_sd->sc_data[SC_NOCHAT].timer != -1) {
				pl_sd->status.manner = 0; // Have to set to 0 first
				status_change_end(&pl_sd->bl, SC_NOCHAT, -1);
				clif_displaymessage(fd,"Player unmuted.");
			} else
				clif_displaymessage(fd,"Player is not muted.");
		} else {
			clif_displaymessage(fd, msg_txt(81)); // Your GM level don't authorize you to do this action on this player.
			return -1;
		}
	} else {
		clif_displaymessage(fd, msg_txt(3)); // Character not found.
		return -1;
	}

	return 0;
}

/*================================================
 * @mute - Mutes a player for a set amount of time
 *------------------------------------------------
 */
ATCOMMAND_FUNC(mute) {
	struct map_session_data *pl_sd;
	int manner;

	if (!message || !*message || sscanf(message, "%d %[^\n]", &manner, atcmd_name) < 2) {
		send_usage(sd, "usage: %s <time> <character name>.", original_command);
		return -1;
	}

	if ((pl_sd = map_nick2sd(atcmd_name)) != NULL || ((pl_sd = map_id2sd(atoi(atcmd_name))) != NULL && pl_sd->state.auth)) {
		if (sd->GM_level >= pl_sd->GM_level) { // Only lower or same level
			pl_sd->status.manner -= manner;
			if (pl_sd->status.manner < 0)
				status_change_start(&pl_sd->bl, SC_NOCHAT, 0, 0, 0, 0, 0, 0);
		} else {
			clif_displaymessage(fd, msg_txt(81)); // Your GM level don't authorize you to do this action on this player.
			return -1;
		}
	} else {
		clif_displaymessage(fd, msg_txt(3)); // Character not found.
		return -1;
	}

	return 0;
}

/*==========================================
 * @refresh (Like @jumpto <<yourself>>)
 *------------------------------------------
 */
ATCOMMAND_FUNC(refresh) {
	pc_setpos(sd, sd->mapname, sd->bl.x, sd->bl.y, 3, 0);

	return 0;
}

/*==========================================
 * @petid <part of pet name>
 * => Displays a list of matching pets.
 *------------------------------------------
 */
ATCOMMAND_FUNC(petid) {
	char searchtext[100];
	char temp0[100];
	char temp1[100];
	int cnt, i, j;

	if (!message || !*message || sscanf(message, "%s", searchtext) < 1)
		return -1;

	for (j = 0; searchtext[j]; j++)
		searchtext[j] = tolower((unsigned char)searchtext[j]); // tolower needs unsigned char
	sprintf(atcmd_output, "Search results for: '%s'.", searchtext);
	clif_displaymessage(fd, atcmd_output);

	cnt = 0;
	i = 0;
	while (i < MAX_PET_DB) {
		for (j = 0; pet_db[i].name[j]; j++)
			temp1[j] = tolower((unsigned char)pet_db[i].name[j]); // tolower needs unsigned char
		temp1[j] = 0;
		for (j = 0; pet_db[i].jname[j]; j++)
			temp0[j] = tolower((unsigned char)pet_db[i].jname[j]); // tolower needs unsigned char
		temp0[j] = 0;
		if (strstr(temp1, searchtext) || strstr(temp0, searchtext)) {
			sprintf(atcmd_output, "ID: %i -- Name: %s", pet_db[i].class, pet_db[i].jname);
			clif_displaymessage(fd, atcmd_output);
			cnt++;
		}
		i++;
	}

	sprintf(atcmd_output, "%i pets have '%s' in their name.", cnt, searchtext);
	clif_displaymessage(fd, atcmd_output);

	return 0;
}

/*==========================================
 * @identify
 * => GM's magnifier.
 *------------------------------------------
 */
ATCOMMAND_FUNC(identify) {
	int i, num;

	num = 0;
	for(i = 0; i < MAX_INVENTORY; i++) {
		if (sd->status.inventory[i].nameid > 0 && sd->status.inventory[i].identify != 1)
			num++;
	}
	if (num > 0) {
		clif_item_identify_list(sd);
	} else {
		clif_displaymessage(fd, "There are no items to appraise.");
	}

	return 0;
}

/*==========================================
 * @misceffect - Shows a misceffect
 *------------------------------------------
 */
ATCOMMAND_FUNC(misceffect) {
	int effect;

	if (!message || !*message || sscanf(message, "%d", &effect) < 1)
		return -1;

	clif_misceffect(&sd->bl, effect);

	return 0;
}

/*==========================================
* @autoloot %
*------------------------------------------
*/
ATCOMMAND_FUNC(autoloot) {
	char message2[100];
	double drop_rate, drop_rate2;
	int i, result_sscanf;

	memset(message2, 0, sizeof(message2));

	result_sscanf = 0;
	if (message && *message) {
		i = 0;
		while(message[i]) {
			if (message[i] == ',')
				message2[i] = '.';
			else
				message2[i] = message[i];
			i++;
		}
		result_sscanf = sscanf(message2, "%lf", &drop_rate2);
	}

	if (!message || !*message || sscanf(message, "%lf", &drop_rate) < 1) {
		if (result_sscanf != 0) {
			drop_rate = drop_rate2;
		} else {
			send_usage(sd, "Please, enter a max drop rate (usage: %s <max_drop_rate>).", original_command);
			if (sd->state.autoloot_rate == 0)
				clif_displaymessage(fd, msg_txt(270)); // Your current autoloot option is disabled.
			else {
				sprintf(atcmd_output, msg_txt(271), ((float)sd->state.autoloot_rate) / 100.); // Your current autoloot option is set to get rate between 0 to %0.02f%%.
				clif_displaymessage(fd, atcmd_output);
			}
			if (sd->state.autolootloot_flag)
				clif_displaymessage(fd, msg_txt(274)); // And you get drops of looted items.
			else
				clif_displaymessage(fd, msg_txt(275)); // And you don't get drops of looted items.
			return -1;
		}
	}

	if (result_sscanf != 0 && ((double)((long long)drop_rate)) == drop_rate && drop_rate != drop_rate2)
		drop_rate = drop_rate2;

	if (drop_rate >= 0. && drop_rate <= 100.) {
		sd->state.autoloot_rate = (int)(drop_rate * 100.);
		if (drop_rate == 0)
			clif_displaymessage(fd, msg_txt(277)); // Autoloot of monster drops disabled.
		else {
			sprintf(atcmd_output, msg_txt(278), drop_rate); // Set autoloot of monster drops when they are between 0 to %0.02f%%.
			clif_displaymessage(fd, atcmd_output);
		}
		if (sd->state.autolootloot_flag)
			clif_displaymessage(fd, msg_txt(274)); // And you get drops of looted items.
		else
			clif_displaymessage(fd, msg_txt(275)); // And you don't get drops of looted items.
	} else {
		clif_displaymessage(fd, msg_txt(276)); // Invalid drop rate. Value must be between 0 (no autoloot) to 100 (autoloot all drops).
		return -1;
	}

	return 0;
}

/*==========================================
* @autolootloot : On/Off
*------------------------------------------
*/
ATCOMMAND_FUNC(autolootloot) {
	if (sd->state.autolootloot_flag) {
		sd->state.autolootloot_flag = 0; // 0: No auto-loot, 1: Autoloot (For looted items)
		clif_displaymessage(fd, msg_txt(272)); // Your auto loot option for looted items is now set to OFF.
	} else {
		sd->state.autolootloot_flag = 1; // 0: No auto-loot, 1: Autoloot (For looted items)
		clif_displaymessage(fd, msg_txt(273)); // Your auto loot option for looted items is now set to ON.
	}

	return 0;
}

/*==========================================
* @displayexp - Turns on/off Displayexp for a specific player
*------------------------------------------
*/
ATCOMMAND_FUNC(displayexp) {
	if (sd->state.displayexp_flag) {
		sd->state.displayexp_flag = 0; // 0: No exp display, 1: Exp display
		clif_displaymessage(fd, msg_txt(249)); // Your display experience option is now set to OFF.
	} else {
		sd->state.displayexp_flag = 1; // 0: No xp display, 1: Exp display
		clif_displaymessage(fd, msg_txt(250)); // Your display experience option is now set to ON.
	}

	return 0;
}

/*==========================================
* @displaydrop - Set display drop for a specific player
*------------------------------------------
*/
ATCOMMAND_FUNC(displaydrop) {
	char message2[100];
	double drop_rate, drop_rate2;
	int i, result_sscanf;

	memset(message2, 0, sizeof(message2));

	result_sscanf = 0;
	if (message && *message) {
		i = 0;
		while(message[i]) {
			if (message[i] == ',')
				message2[i] = '.';
			else
				message2[i] = message[i];
			i++;
		}
		result_sscanf = sscanf(message2, "%lf", &drop_rate2);
	}

	if (!message || !*message || sscanf(message, "%lf", &drop_rate) < 1) {
		if (result_sscanf != 0) {
			drop_rate = drop_rate2;
		} else {
			send_usage(sd, "Please, enter a max drop rate (usage: %s <max_drop_rate>).", original_command);
			if (sd->state.displaydrop_rate == 0)
				clif_displaymessage(fd, msg_txt(661)); // Your current display drop option is disabled.
			else {
				sprintf(atcmd_output, msg_txt(662), ((float)sd->state.displaydrop_rate) / 100.); // Your current display drop option is set to display rate between 0 to %0.02f%%.
				clif_displaymessage(fd, atcmd_output);
			}
			if (sd->state.displaylootdrop)
				clif_displaymessage(fd, msg_txt(665)); // And you display drops of looted items.
			else
				clif_displaymessage(fd, msg_txt(666)); // And you don't display drops of looted items.
			return -1;
		}
	}

	if (result_sscanf != 0 && ((double)((long long)drop_rate)) == drop_rate && drop_rate != drop_rate2)
		drop_rate = drop_rate2;

	if (drop_rate >= 0. && drop_rate <= 100.) {
		sd->state.displaydrop_rate = (int)(drop_rate * 100.);
		if (drop_rate == 0)
			clif_displaymessage(fd, msg_txt(659)); // Displaying of monster drops disabled.
		else {
			sprintf(atcmd_output, msg_txt(658), drop_rate); // Set displaying of monster drops when they are between 0 to %0.02f%%.
			clif_displaymessage(fd, atcmd_output);
		}
		if (sd->state.displaylootdrop)
			clif_displaymessage(fd, msg_txt(665)); // And you display drops of looted items.
		else
			clif_displaymessage(fd, msg_txt(666)); // And you don't display drops of looted items.
	} else {
		clif_displaymessage(fd, msg_txt(657)); // Invalid drop rate. Value must be between 0 (no display) to 100 (display all drops).
		return -1;
	}

	return 0;
}

/*==========================================
* @displaylootdrop - Set display drop of loot for a specific player
*------------------------------------------
*/
ATCOMMAND_FUNC(displaylootdrop) {
	if (sd->state.displaylootdrop) {
		sd->state.displaylootdrop = 0;
		clif_displaymessage(fd, msg_txt(664)); // You don't more display drops of looted items.
	} else {
		sd->state.displaylootdrop = 1;
		clif_displaymessage(fd, msg_txt(663)); // You display now drops of looted items.
	}

	return 0;
}

/*==========================================
 * @invincible - Makes user extremely powerful
 *------------------------------------------
 */
ATCOMMAND_FUNC(invincible) {

	status_change_start(&sd->bl, SC_INVINCIBLE, 0, 0, 0, 0, 0, 0);
	clif_displaymessage(fd, "You are now invincible.");
	return 0;
}

/*==========================================
 * @charinvincible - Makes a char extremely powerful
 *------------------------------------------
 */
ATCOMMAND_FUNC(charinvincible) {

	struct map_session_data *pl_sd;

	if (!message || !*message || sscanf(message, "%[^\n]", atcmd_name) < 1) {
		send_usage(sd, "Please, enter a player name/account ID.", original_command);
		return -1;
	}

	if ((pl_sd = map_nick2sd(atcmd_name)) != NULL || ((pl_sd = map_id2sd(atoi(atcmd_name))) != NULL && pl_sd->state.auth)) {
		if (pc_isdead(pl_sd)) {
			clif_displaymessage(fd, "Player is dead. You must revive them before you can make them invincible.");
			return -1;
		}
		status_change_start(&pl_sd->bl, SC_INVINCIBLE, 0, 0, 0, 0, 0, 0);
		clif_displaymessage(fd, "Player is now invincible.");
	}
	else {
		clif_displaymessage(fd, msg_txt(3)); // Character not found.
		return -1;
	}
	return 0;
}

/*==========================================
 * @sc_start - Starts a status change
 *------------------------------------------
 */
ATCOMMAND_FUNC(sc_start) {

	int sc_id = 0, sc_lvl = 0, sc_time = 0;

	if (!message || !*message ||
	    (sscanf(message, "%d" "%d" "%d", &sc_id, &sc_lvl, &sc_time) < 3)) {
		clif_displaymessage(fd, "Usage: <Status Change ID> <Level> <Duration.");
		return -1;
	}

	status_change_start(&sd->bl, sc_id, sc_lvl, 0, 0, 0, sc_time, 0);

	return 0;
}

/*==========================================
 * @main - Activates main channel (On/Off)
 *------------------------------------------
 */
ATCOMMAND_FUNC(main) {
	char message_to_all[100];

	if (!message || !*message || sscanf(message, "%[^\n]", message_to_all) < 1) {
		send_usage(sd, "Please, enter a message (usage: %s <message>).", original_command);
		return -1;
	}

	if (strcasecmp(message_to_all, "on") == 0) {
		if (sd->state.main_flag == 1) {
			clif_displaymessage(fd, "Your Main channel is already ON.");
		} else {
			sd->state.main_flag = 1;
			clif_displaymessage(fd, "Your Main channel is now ON.");
		}
	} else if (strcasecmp(message_to_all, "off") == 0) {
		if (sd->state.main_flag == 0) {
			clif_displaymessage(fd, "Your Main channel is already OFF.");
		} else {
			sd->state.main_flag = 0;
			clif_displaymessage(fd, "Your Main channel is now OFF.");
		}
	} else if (sd->state.main_flag == 0)
		clif_displaymessage(fd, "Your Main channel is OFF. You can not speak on.");
	else {
		if (check_bad_word(message_to_all, strlen(message_to_all), sd))
			return -1; // Check_bad_word function display message if necessary
		// Check flag for WoE
		if (agit_flag == 1 && // 0: WoE not starting, Woe is running
		    sd->status.guild_id > 0) {
			if (battle_config.atcommand_main_channel_when_woe > sd->GM_level) { // Is not possible to use @main when WoE and in guild
				clif_displaymessage(fd, msg_txt(683)); // War of Emperium is actually running. Because you are member of a guild, you can not use 'Main channel'.
				return -1;
			} else if (map[sd->bl.m].flag.gvg &&
			    battle_config.atcommand_main_channel_on_gvg_map_woe > sd->GM_level) { // Is not possible to use @main when WoE and in guild ON GvG maps
				clif_displaymessage(fd, msg_txt(685)); // War of Emperium is actually running. Because you are member of a guild, you can not use 'Main channel' on GvG maps.
				return -1;
			}
		}

		intif_main_message(sd->status.name, message_to_all);
	}


	return 0;
}

/*==========================================
 * @request
 *------------------------------------------
 */
ATCOMMAND_FUNC(request) {
	char message_to_GM[100];

	if (!message || !*message || sscanf(message, "%[^\n]", message_to_GM) < 1) {
		send_usage(sd, "Please, enter a message (usage: %s <message>).", original_command);
		return -1;
	}

	if (battle_config.atcommand_min_GM_level_for_request > 99)
		clif_displaymessage(fd, msg_txt(251)); // Your request has not been sent. Request system is disabled.
	else if (strcasecmp(message_to_GM, "on") == 0) {
		if (sd->GM_level < battle_config.atcommand_min_GM_level_for_request) {
			clif_displaymessage(fd, "Your option is refused. You are not a GM that can receive requests from players.");
			return -1;
		} else if (sd->state.refuse_request_flag == 0) {
			clif_displaymessage(fd, "You can already receive requests from players.");
		} else {
			sd->state.refuse_request_flag = 0;
			clif_displaymessage(fd, "You can now receive requests from players.");
		}
	} else if (strcasecmp(message_to_GM, "off") == 0) {
		if (sd->GM_level < battle_config.atcommand_min_GM_level_for_request) {
			clif_displaymessage(fd, "Your option is refused. You are not a GM that can receive requests from players.");
			return -1;
		} else if (sd->state.refuse_request_flag == 1) {
			clif_displaymessage(fd, "You already refuse requests from players.");
		} else {
			sd->state.refuse_request_flag = 1;
			clif_displaymessage(fd, "You refuse now requests from players.");
		}
	} else {
		if (check_bad_word(message_to_GM, strlen(message_to_GM), sd))
			return -1; // Check_bad_word function display message if necessary
		// Send message to online GMs
		if (intif_gm_message(sd->status.name, message_to_GM)) // Is there at least 1 online GM to receive the message (On this server)?
			clif_displaymessage(fd, "Your request has been delivered.");
		else if (map_is_alone) // Not in multi-servers
			clif_displaymessage(fd, "Sorry, but there're no GMs online. Try to send your request later.");
		else // No online GM on this server, but perahps on some other map-servers
			clif_displaymessage(fd, msg_txt(252)); // Your request has been sent. If there are no GMs is online, your request is lost.
	}

	return 0;
}

/*==========================================
 * @version - Displays version of Freya. +/- like 0x7530 admin packet
 *------------------------------------------
 */
ATCOMMAND_FUNC(version) {
	sprintf(atcmd_output, "Freya version %d.%d.%d %s", FREYA_MAJORVERSION, FREYA_MINORVERSION, FREYA_REVISION,
#ifdef USE_SQL
	"SQL");
#else
	"TXT");
#endif // USE_SQL
#ifdef SVN_REVISION
	if (SVN_REVISION >= 1) // In case of .svn directories having been deleted
		sprintf(atcmd_output + strlen(atcmd_output), " (SVN rev.: %d).", (int)SVN_REVISION);
#endif // SVN_REVISION

	clif_displaymessage(fd, atcmd_output);

	return 0;
}
