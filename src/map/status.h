// Copyright (c) Freya Development Team - Licensed under GNU GPL
// For more information, see License.txt in the main folder

#ifndef _STATUS_H_
#define _STATUS_H_

// struct map_session_data‚ status_change function
enum {
	SC_STONE = 0,
	SC_FREEZE,
	SC_STUN,
	SC_SLEEP,
	SC_POISON,
	SC_CURSE,
	SC_SILENCE,
	SC_CONFUSION,
	SC_BLIND,
	SC_BLEEDING,
	SC_DPOISON, //10

	SC_PROVOKE = 11,
	SC_ENDURE,
	SC_TWOHANDQUICKEN,
	SC_CONCENTRATE,
	SC_HIDING,
	SC_CLOAKING,
	SC_ENCPOISON,
	SC_POISONREACT,
	SC_QUAGMIRE,
	SC_ANGELUS, //20
	SC_BLESSING,
	SC_SIGNUMCRUCIS,
	SC_INCREASEAGI,
	SC_DECREASEAGI,
	SC_SLOWPOISON,
	SC_IMPOSITIO  ,
	SC_SUFFRAGIUM,
	SC_ASPERSIO,
	SC_BENEDICTIO,
	SC_KYRIE, //30
	SC_MAGNIFICAT,
	SC_GLORIA,
	SC_AETERNA,
	SC_ADRENALINE,
	SC_WEAPONPERFECTION,
	SC_OVERTHRUST,
	SC_MAXIMIZEPOWER,
	SC_TRICKDEAD,
	SC_LOUD,
	SC_ENERGYCOAT, //40
	SC_BROKENARMOR, // Unused
	SC_BROKENWEAPON, // Unused
	SC_HALLUCINATION,
	SC_WEIGHT50,
	SC_WEIGHT90,
	SC_ASPDPOTION0,
	SC_ASPDPOTION1,
	SC_ASPDPOTION2,
	SC_ASPDPOTION3,
	SC_SPEEDUP0, //50
	SC_SPEEDUP1,
	SC_ATKPOT,
	SC_MATKPOT,
	SC_WEDDING,
	SC_SLOWDOWN,
	SC_ANKLE,
	SC_KEEPING,
	SC_BARRIER,
	SC_STRIPWEAPON,
	SC_STRIPSHIELD, //60
	SC_STRIPARMOR,
	SC_STRIPHELM,
	SC_CP_WEAPON,
	SC_CP_SHIELD,
	SC_CP_ARMOR,
	SC_CP_HELM,
	SC_AUTOGUARD,
	SC_REFLECTSHIELD,
	SC_SPLASHER,
	SC_PROVIDENCE, //70
	SC_DEFENDER,
	SC_MAGICROD,
	SC_SPELLBREAKER,
	SC_AUTOSPELL,
	SC_SIGHTTRASHER,
	SC_AUTOBERSERK,
	SC_SPEARQUICKEN,
	SC_AUTOCOUNTER,
	SC_SIGHT,
	SC_SAFETYWALL, //80
	SC_RUWACH,
	SC_EXTREMITYFIST,
	SC_EXPLOSIONSPIRITS,
	SC_COMBO,
	SC_BLADESTOP_WAIT,
	SC_BLADESTOP,
	SC_FIREWEAPON,
	SC_WATERWEAPON,
	SC_WINDWEAPON,
	SC_EARTHWEAPON, //90
	SC_VOLCANO,
	SC_DELUGE,
	SC_VIOLENTGALE,
	SC_WATK_ELEMENT,
	SC_ARMOR,
	SC_ARMOR_ELEMENT,
	SC_NOCHAT,
	SC_BABY,
	SC_AURABLADE,
	SC_PARRYING, //100
	SC_CONCENTRATION,
	SC_TENSIONRELAX,
	SC_BERSERK,
	SC_FURY,
	SC_GOSPEL,
	SC_ASSUMPTIO,
	SC_BASILICA,
	SC_GUILDAURA,
	SC_MAGICPOWER,
	SC_EDP, //110
	SC_TRUESIGHT,
	SC_WINDWALK,
	SC_MELTDOWN,
	SC_CARTBOOST,
	SC_CHASEWALK,
	SC_REJECTSWORD,
	SC_MARIONETTE,
	SC_MARIONETTE2,
	SC_UNUSED, // Unused
	SC_JOINTBEAT, //120
	SC_MINDBREAKER,
	SC_MEMORIZE,
	SC_FOGWALL,
	SC_SPIDERWEB,
	SC_DEVOTION,
	SC_SACRIFICE,
	SC_STEELBODY,
	SC_ORCISH,
	SC_READYSTORM,
	SC_READYDOWN, //130
	SC_READYTURN,
	SC_READYCOUNTER,
	SC_DODGE,
	SC_RUN,
	SC_SHADOWWEAPON,
	SC_ADRENALINE2,
	SC_GHOSTWEAPON,
	SC_KAIZEL,
	SC_KAAHI,
	SC_KAUPE, //140
	SC_ONEHAND,
	SC_PRESERVE,
	SC_BATTLEORDERS,
	SC_REGENERATION,
	SC_DOUBLECAST,
	SC_GRAVITATION,
	SC_MAXOVERTHRUST,
	SC_LONGING,
	SC_HERMODE,
	SC_SHRINK, //150
	SC_SIGHTBLASTER,
	SC_WINKCHARM,
	SC_CLOSECONFINE,
	SC_CLOSECONFINE2,
	SC_DANCING,
	SC_ELEMENTALCHANGE,
	SC_RICHMANKIM,
	SC_ETERNALCHAOS,
	SC_DRUMBATTLE,
	SC_NIBELUNGEN, //160
	SC_ROKISWEIL,
	SC_INTOABYSS,
	SC_SIEGFRIED,
	SC_WHISTLE,
	SC_ASSNCROS,
	SC_POEMBRAGI,
	SC_APPLEIDUN,
	SC_MODECHANGE,
	SC_HUMMING,
	SC_DONTFORGETME, //170
	SC_FORTUNE,
	SC_SERVICEFORYOU,
	SC_STOP,
	SC_SPURT,
	SC_SPIRIT,
	SC_COMA,
	SC_INTRAVISION,
	SC_INCALLSTATUS,
	SC_INCSTR,
	SC_INCAGI, //180
	SC_INCVIT,
	SC_INCINT,
	SC_INCDEX,
	SC_INCLUK,
	SC_INCHIT,
	SC_INCHITRATE,
	SC_INCFLEE,
	SC_INCFLEERATE,
	SC_INCMHPRATE,
	SC_INCMSPRATE, //190
	SC_INCATKRATE,
	SC_INCMATKRATE,
	SC_INCDEFRATE,
	SC_STRFOOD,
	SC_AGIFOOD,
	SC_VITFOOD,
	SC_INTFOOD,
	SC_DEXFOOD,
	SC_LUKFOOD,
	SC_HITFOOD, //200
	SC_FLEEFOOD,
	SC_BATKFOOD,
	SC_WATKFOOD,
	SC_MATKFOOD,
	SC_SCRESIST,
	SC_XMAS,
	SC_WARM,
	SC_SUN_COMFORT,
	SC_MOON_COMFORT,
	SC_STAR_COMFORT, //210
	SC_FUSION,
	SC_SKILLRATE_UP,
	SC_SKE,
	SC_KAITE,
	SC_SWOO,
	SC_SKA,
	SC_TKREST,
	SC_MIRACLE,
	SC_MADNESSCANCEL,
	SC_ADJUSTMENT, //220
	SC_INCREASING,
	SC_GATLINGFEVER,
	SC_TATAMIGAESHI,
	SC_UTSUSEMI,
	SC_BUNSINJYUTSU,
	SC_KAENSIN,
	SC_SUITON,
	SC_NEN,
	SC_KNOWLEDGE,
	SC_SMA,	//230
	SC_FLING,
	SC_AVOID,
	SC_CHANGE,
	SC_BLOODLUST,
	SC_FLEET,
	SC_SPEED,
	SC_DEFENCE,
	SC_INCAGIRATE,
	SC_INCDEXRATE,
	SC_JAILED, //240
	SC_FALCON,
	SC_RIDING,
	SC_MAGNUM,
	SC_PNEUMA,
	SC_LULLABY,
	SC_DISSONANCE,
	SC_UGLYDANCE,
	SC_MOONLIT,
	SC_LANDPROTECTOR,
	SC_READYDODGE, //250
	SC_SELFDESTRUCTION,
	SC_GDSKILLDELAY,
	SC_INCASPDRATE,
	SC_DOUBLE,
	SC_INVINCIBLE,
	SC_MAX, //256
};
extern int SkillStatusChangeTable[MAX_SKILL];

// Enumerates the number for the status changes (Client-dependent)
enum {
	ICO_BLANK		= -1,
	ICO_PROVOKE		= 0,
	ICO_ENDURE		= 1,
	ICO_TWOHANDQUICKEN	= 2,
	ICO_CONCENTRATE		= 3,
	ICO_HIDING		= 4,
	ICO_CLOAKING		= 5,
	ICO_ENCPOISON		= 6,
	ICO_POISONREACT		= 7,
	ICO_QUAGMIRE		= 8,
	ICO_ANGELUS		= 9,
	ICO_BLESSING		= 10,
	ICO_SIGNUMCRUCIS		= 11,
	ICO_INCREASEAGI		= 12,
	ICO_DECREASEAGI		= 13,
	ICO_SLOWPOISON		= 14,
	ICO_IMPOSITIO  		= 15,
	ICO_SUFFRAGIUM		= 16,
	ICO_ASPERSIO		= 17,
	ICO_BENEDICTIO		= 18,
	ICO_KYRIE		= 19,
	ICO_MAGNIFICAT		= 20,
	ICO_GLORIA		= 21,
	ICO_AETERNA		= 22,
	ICO_ADRENALINE		= 23,
	ICO_WEAPONPERFECTION	= 24,
	ICO_OVERTHRUST		= 25,
	ICO_MAXIMIZEPOWER	= 26,
	ICO_RIDING		= 27,
	ICO_FALCON		= 28,
	ICO_TRICKDEAD		= 29,
	ICO_LOUD			= 30,
	ICO_ENERGYCOAT		= 31,
	ICO_BROKENARMOR		= 32,
	ICO_BROKENWEAPON		= 33,
	ICO_HALLUCINATION	= 34,
	ICO_WEIGHT50 		= 35,
	ICO_WEIGHT90		= 36,
	ICO_ASPDPOTION		= 37,
	//38: Duplicate Aspd Potion
	//39: Duplicate Aspd Potion
	//40: Duplicate Aspd Potion
	ICO_SPEEDPOTION1	= 41,
	ICO_SPEEDPOTION2	= 42,
	ICO_STRIPWEAPON		= 50,
	ICO_STRIPSHIELD		= 51,
	ICO_STRIPARMOR		= 52,
	ICO_STRIPHELM		= 53,
	ICO_CP_WEAPON		= 54,
	ICO_CP_SHIELD		= 55,
	ICO_CP_ARMOR		= 56,
	ICO_CP_HELM		= 57,
	ICO_AUTOGUARD		= 58,
	ICO_REFLECTSHIELD	= 59,
	ICO_PROVIDENCE		= 61,
	ICO_DEFENDER		= 62,
	ICO_AUTOSPELL		= 65,
	ICO_SPEARQUICKEN		= 68,
	ICO_EXPLOSIONSPIRITS	= 86,
	ICO_FURY			= 87,
	ICO_FIREWEAPON		= 90,
	ICO_WATERWEAPON		= 91,
	ICO_WINDWEAPON		= 92,
	ICO_EARTHWEAPON		= 93,
	ICO_UNDEAD			= 97,
	// 102 = Duplicate Gloria
	ICO_AURABLADE		= 103,
	ICO_PARRYING		= 104,
	ICO_CONCENTRATION	= 105,
	ICO_TENSIONRELAX		= 106,
	ICO_BERSERK		= 107,
	ICO_ASSUMPTIO		= 110,
	ICO_LANDENDOW	= 112,
	ICO_MAGICPOWER		= 113,
	ICO_EDP			= 114,
	ICO_TRUESIGHT		= 115,
	ICO_WINDWALK		= 116,
	ICO_MELTDOWN		= 117,
	ICO_CARTBOOST		= 118,
	//119, Blank
	ICO_REJECTSWORD		= 120,
	ICO_MARIONETTE		= 121,
	ICO_MARIONETTE2		= 122,
	ICO_MOONLIT		= 123,
	ICO_BLEEDING		= 124,
	ICO_JOINTBEAT		= 125,
	ICO_DEVOTION		= 130,
	ICO_STEELBODY		= 132,
	ICO_RUN			= 133,
	ICO_BUMP			= 134,
	ICO_READYSTORM		= 135,
	ICO_READYDOWN		= 137,
	ICO_READYTURN		= 139,
	ICO_READYCOUNTER		= 141,
	ICO_DODGE		= 143,
	//ICO_RUN		= 144, // Not Sprint skill, need info
	ICO_SPURT			= 145,
	ICO_SHADOWWEAPON		= 146,
	ICO_ADRENALINE2		= 147,
	ICO_GHOSTWEAPON		= 148,
	ICO_SPIRIT		= 149,
	ICO_DEVIL		= 152,
	ICO_KAITE		= 153,
	ICO_KAIZEL		= 156,
	ICO_KAAHI		= 157,
	ICO_KAUPE		= 158,
	ICO_SMA		= 159,
	ICO_NIGHT		= 160,
	ICO_ONEHAND		= 161,
	ICO_WARM			= 165,	
	//166, Duplicate Warm
	//167, Duplicate Warm
	ICO_SUN_COMFORT		= 169,
	ICO_MOON_COMFORT		= 170,	
	ICO_STAR_COMFORT		= 171,	
	ICO_PRESERVE		= 181,
	ICO_INCSTR	= 182,
	ICO_INTRAVISION	= 184,
	ICO_DOUBLECAST		= 186,
	ICO_MAXOVERTHRUST	= 188,
	ICO_TAROT		= 191,
	ICO_SHRINK		= 197,
	ICO_SIGHTBLASTER		= 198,
	ICO_WINKCHARM		= 199,
	ICO_CLOSECONFINE		= 200,
	ICO_CLOSECONFINE2	= 201,
	ICO_MADNESSCANCEL	= 203,
	ICO_GATLINGFEVER		= 204,
	ICO_TKREST = 205,
	ICO_UTSUSEMI			= 206,
	ICO_BUNSINJYUTSU		= 207,
	ICO_NEN				= 208,
	ICO_ADJUSTMENT		= 209,
	ICO_ACCURACY			= 210,
	ICO_STRFOOD			= 241,
	ICO_AGIFOOD			= 242,
	ICO_VITFOOD			= 243,
	ICO_DEXFOOD			= 244,
	ICO_INTFOOD			= 245,
	ICO_LUKFOOD			= 246,
	ICO_FLEEFOOD			= 247,
	ICO_HITFOOD			= 248,
	ICO_CRIFOOD			= 249,
};
extern int StatusIconTable[SC_MAX];

extern int percentrefinery[5][10]; // refine_db.txt

int status_getrefinebonus(int lv, int type);
int status_percentrefinery(struct map_session_data *sd, struct item *item);

int status_calc_pc(struct map_session_data*, int);
void status_calc_speed(struct block_list *bl);
int status_get_class(struct block_list *bl);
int status_get_dir(struct block_list *bl);
int status_get_lv(struct block_list *bl);
int status_get_range(struct block_list *bl);
int status_get_hp(struct block_list *bl);
int status_get_max_hp(struct block_list *bl);
int status_get_str(struct block_list *bl);
int status_get_agi(struct block_list *bl);
int status_get_vit(struct block_list *bl);
int status_get_int(struct block_list *bl);
int status_get_dex(struct block_list *bl);
int status_get_luk(struct block_list *bl);
int status_get_hit(struct block_list *bl);
int status_get_flee(struct block_list *bl);
int status_get_def(struct block_list *bl);
int status_get_mdef(struct block_list *bl);
int status_get_flee2(struct block_list *bl);
int status_get_critical(struct block_list *bl);
int status_get_def2(struct block_list *bl);
int status_get_mdef2(struct block_list *bl);
int status_get_baseatk(struct block_list *bl);
int status_get_atk(struct block_list *bl);
int status_get_atk_(struct block_list *bl);
int status_get_atk2(struct block_list *bl);
int status_get_atk_2(struct block_list *bl);
int status_get_matk1(struct block_list *bl);
int status_get_matk2(struct block_list *bl);
int status_get_adelay(struct block_list *bl);
int status_get_amotion(struct block_list *bl);
int status_get_dmotion(struct block_list *bl);
int status_get_element(struct block_list *bl);
int status_get_attack_element(struct block_list *bl);
int status_get_attack_element2(struct block_list *bl);
#define status_get_elem_type(bl) (status_get_element(bl) % 10)
#define status_get_elem_level(bl) (status_get_element(bl) / 10 / 2)
int status_get_party_id(struct block_list *bl);
int status_get_guild_id(struct block_list *bl);
int status_get_race(struct block_list *bl);
int status_get_size(struct block_list *bl);
int status_get_mode(struct block_list *bl);
int status_get_race2(struct block_list *bl);
int status_isdead(struct block_list *bl);
int status_isimmune(struct block_list *bl);

struct status_change *status_get_sc_data(struct block_list *bl);
short *status_get_sc_count(struct block_list *bl);
short *status_get_opt1(struct block_list *bl);
short *status_get_opt2(struct block_list *bl);
short *status_get_opt3(struct block_list *bl);
short *status_get_option(struct block_list *bl);

int status_get_sc_def(struct block_list *bl, int type);
#define status_get_sc_def_mdef(bl) (status_get_sc_def(bl, SP_MDEF1))
#define status_get_sc_def_vit(bl) (status_get_sc_def(bl, SP_DEF2))
#define status_get_sc_def_int(bl) (status_get_sc_def(bl, SP_MDEF2))
#define status_get_sc_def_luk(bl) (status_get_sc_def(bl, SP_LUK))

int status_change_start(struct block_list *bl, int type, int val1, int val2, int val3, int val4, int tick, int flag);
void status_change_clear(struct block_list *bl, int type);
int status_change_end(struct block_list* bl, int type, int tid);
int status_change_timer(int tid, unsigned int tick, int id, int data);
int status_change_timer_sub(struct block_list *bl, va_list ap);
int status_change_clear_buffs(struct block_list *bl);
int status_change_clear_debuffs(struct block_list *bl);

int npc_touch_areanpc(struct map_session_data *, int, int, int);

int do_init_status(void);

#endif
