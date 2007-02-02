/*	This file is a part of Freya.
		Freya is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	any later version.
		Freya is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.
		You should have received a copy of the GNU General Public License
	along with Freya; if not, write to the Free Software
	Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA */

#ifndef _STATUS_H_
#define _STATUS_H_

enum {	// struct map_session_dataÇ status_change function

	SC_STONE 								= 0,
	SC_FREEZE,
	SC_STUN,
	SC_SLEEP,
	SC_POISON,
	SC_CURSE, //5
	SC_SILENCE,
	SC_CONFUSION,
	SC_BLIND,
	SC_BLEEDING,
	SC_DPOISON, //10

	SC_ASPDPOTION0          = 16,
	SC_ASPDPOTION1          = 17,
	SC_ASPDPOTION2          = 18,
	SC_ASPDPOTION3          = 19,
	SC_SPEEDUP0             = 20,
	SC_SPEEDUP1             = 21,
	SC_ATKPOT               = 22,
	SC_MATKPOT              = 23,
	SC_ENCPOISON            = 24,
	SC_ASPERSIO             = 25,
	SC_FIREWEAPON           = 26,
	SC_WATERWEAPON          = 27,
	SC_WINDWEAPON           = 28,
	SC_EARTHWEAPON          = 29,
	SC_PROVOKE							= 30,
	SC_ENDURE,
	SC_TWOHANDQUICKEN,
	SC_CONCENTRATE,
	SC_POISONREACT,
	SC_ANGELUS, //35
	SC_BLESSING,
	SC_SIGNUMCRUCIS,
	SC_INCREASEAGI,
	SC_DECREASEAGI,
	SC_SLOWPOISON, //40
	SC_IMPOSITIO,
	SC_SUFFRAGIUM,
	SC_BENEDICTIO,
	SC_KYRIE,
	SC_MAGNIFICAT, //45
	SC_GLORIA,
	SC_AETERNA,
	SC_ADRENALINE,
	SC_WEAPONPERFECTION,
	SC_OVERTHRUST, //50
	SC_MAXIMIZEPOWER,
	SC_TRICKDEAD,
	SC_LOUD,
	SC_ENERGYCOAT,
	SC_SLOWDOWN, //55, For skill slowdown
	SC_AUTOBERSERK,
	SC_AUTOGUARD,
	SC_REFLECTSHIELD,
	SC_DEVOTION,
	SC_PROVIDENCE, //60
	SC_DEFENDER,
	SC_AUTOSPELL,
	SC_SPEARQUICKEN,
	SC_SIGHTTRASHER,
	SC_EXPLOSIONSPIRITS, //65
	SC_STEELBODY,
	SC_AURABLADE,
	SC_PARRYING,
	SC_CONCENTRATION,
	SC_BERSERK, //70
	SC_ASSUMPTIO,
	SC_MAGICPOWER,
	SC_TRUESIGHT,
	SC_WINDWALK,
	SC_MELTDOWN, //75
	SC_REJECTSWORD,
	SC_MARIONETTE,
	SC_MARIONETTE2,
	SC_MAXOVERTHRUST,
	SC_SACRIFICE, //80
	SC_DOUBLECAST,
	SC_MEMORIZE,
	SC_BATTLEORDERS,
	
	SC_EDP, //85
	SC_CP_WEAPON,
	SC_CP_SHIELD,
	SC_CP_ARMOR,
	SC_CP_HELM,
	SC_PRESERVE, //90
	SC_CARTBOOST,
	SC_BUFFMAX, // Normal buffs ends here
	
	SC_QUAGMIRE,
	SC_BASILICA,
	SC_STRIPWEAPON, //95
	SC_STRIPSHIELD,
	SC_STRIPARMOR,
	SC_STRIPHELM,
	SC_COMBO,
	SC_HIDING, //100
	SC_CLOAKING,
	SC_RIDING,
	SC_FALCON,
	SC_TENSIONRELAX,
	SC_WEIGHT50, //105
	SC_WEIGHT90,
	SC_HALLUCINATION,
	SC_BROKNARMOR,
	SC_BROKNWEAPON,
	SC_JOINTBEAT, //110
	SC_SAFETYWALL,
	SC_PNEUMA,
	SC_ANKLE,
	SC_DANCING,
	SC_KEEPING, //115
	SC_BARRIER,
	SC_MAGICROD,
	SC_SIGHT,
	SC_RUWACH,
	SC_AUTOCOUNTER, //120
	SC_VOLCANO,
	SC_DELUGE,
	SC_VIOLENTGALE,
	SC_BLADESTOP_WAIT,
	SC_BLADESTOP, //125
	SC_EXTREMITYFIST,
	SC_GRAFFITI,
	SC_ENSEMBLE,
	SC_LULLABY,
	SC_RICHMANKIM, //130
	SC_ETERNALCHAOS,
	SC_DRUMBATTLE,
	SC_NIBELUNGEN,
	SC_ROKISWEIL,
	SC_INTOABYSS, //135
	SC_SIEGFRIED,
	SC_DISSONANCE,
	SC_WHISTLE,
	SC_ASSNCROS,
	SC_POEMBRAGI, //140
	SC_APPLEIDUN,
	SC_UGLYDANCE,
	SC_HUMMING,
	SC_DONTFORGETME,
	SC_FORTUNE, //145
	SC_SERVICEFORYOU,
	SC_FOGWALL,
	SC_GOSPEL,
	SC_SPIDERWEB,
	SC_MOONLIT, //150
	SC_WEDDING,
	SC_NOCHAT,
	SC_SPLASHER,
	SC_SELFDESTRUCTION,
	SC_MINDBREAKER, //155
	SC_SPELLBREAKER,
	SC_LANDPROTECTOR,
	SC_ADAPTATION,
	SC_CHASEWALK,
	SC_GUILDAURA, //160
	SC_REGENERATION,
	SC_CLOSECONFINE,
	SC_CLOSECONFINE2,
	SC_RUN,
	SC_SPURT, //165
	SC_TKCOMBO,
	SC_READYDODGE,
	SC_READYSTORM,
	SC_READYDOWN,
	SC_READYTURN, //170
	SC_READYCOUNTER,
	SC_GRAVITATION,
	SC_LONGING,
	SC_HERMODE,
	SC_INCSTR, //175
	SC_INCAGI,
	SC_INCVIT,
	SC_INCINT,
	SC_INCDEX,
	SC_INCLUK,
	SC_INCALLSTATUS,
	SC_SCRESIST,
	SC_INCHIT,
	SC_INCFLEE,
	SC_INCMHPRATE,
	SC_INCMSPRATE,
	SC_INCMATKRATE,
	SC_INCATKRATE,
	SC_INCDEFRATE,
	SC_INCHITRATE,		// 190
	SC_INCFLEERATE,
	SC_INCASPDRATE,
	SC_STOP,
	SC_COMA,
	SC_SHRINK,
	SC_SIGHTBLASTER,
	SC_TKREST,
	SC_SHADOWWEAPON,
	SC_GHOSTWEAPON,
	SC_GDSKILLDELAY,		// 200
	SC_STRFOOD,
	SC_AGIFOOD,
	SC_VITFOOD,
	SC_INTFOOD,
	SC_DEXFOOD,
	SC_LUKFOOD,
	SC_HITFOOD,
	SC_FLEEFOOD,
	SC_BATKFOOD,
	SC_WATKFOOD,		// 210
	SC_MATKFOOD,
	SC_MAGNUM,
	SC_XMAS,
	SC_WARM,
	SC_SUN_COMFORT,
	SC_MOON_COMFORT,
	SC_STAR_COMFORT,
	SC_FUSION,
	SC_SKILLRATE_UP,
	SC_SKE,	// 220
	SC_KAITE,
	SC_SWOO,
	SC_SKA,
	SC_MIRACLE,
	//Ninja/GS states
	SC_MADNESSCANCEL,
	SC_ADJUSTMENT,
	SC_INCREASING,
	SC_GATLINGFEVER,
	SC_TATAMIGAESHI,
	SC_UTSUSEMI,	// 230
	SC_BUNSINJYUTSU,
	SC_KAENSIN,
	SC_SUITON,
	SC_NEN,
	SC_KNOWLEDGE,
	SC_SMA,
	SC_FLING,
	SC_AVOID,
	SC_CHANGE,
	SC_BLOODLUST,	// 240
	SC_FLEET,
	SC_SPEED,
	SC_DEFENCE,
	SC_INCAGIRATE,
	SC_INCDEXRATE,
	SC_JAILED,
	SC_SPIRIT,
	SC_ARMOR_ELEMENT, // 248

	SC_KAIZEL,
	SC_KAAHI,
	SC_KAUPE, // 251

	SC_MAX // 252
};
extern int SkillStatusChangeTable[MAX_SKILL];

//Numerates the Number for the status changes (client-dependent), imported from jA
enum {
	ICO_BLANK               = -1,
	ICO_PROVOKE             = 0,
	ICO_ENDURE              = 1,
	ICO_TWOHANDQUICKEN      = 2,
	ICO_CONCENTRATE         = 3,
	ICO_HIDING              = 4,
	ICO_CLOAKING            = 5,
	ICO_ENCPOISON           = 6,
	ICO_POISONREACT         = 7,
	ICO_QUAGMIRE            = 8,
	ICO_ANGELUS             = 9,
	ICO_BLESSING            = 10,
	ICO_SIGNUMCRUCIS        = 11,
	ICO_INCREASEAGI         = 12,
	ICO_DECREASEAGI         = 13,
	ICO_SLOWPOISON          = 14,
	ICO_IMPOSITIO           = 15,
	ICO_SUFFRAGIUM          = 16,
	ICO_ASPERSIO            = 17,
	ICO_BENEDICTIO          = 18,
	ICO_KYRIE               = 19,
	ICO_MAGNIFICAT          = 20,
	ICO_GLORIA              = 21,
	ICO_AETERNA             = 22,
	ICO_ADRENALINE          = 23,
	ICO_WEAPONPERFECTION    = 24,
	ICO_OVERTHRUST          = 25,
	ICO_MAXIMIZEPOWER       = 26,
	ICO_RIDING              = 27,
	ICO_FALCON              = 28,
	ICO_TRICKDEAD           = 29,
	ICO_LOUD                = 30,
	ICO_ENERGYCOAT          = 31,
	ICO_BROKENARMOR         = 32,
	ICO_BROKENWEAPON        = 33,
	ICO_HALLUCINATION       = 34,
	ICO_WEIGHT50            = 35,
	ICO_WEIGHT90            = 36,
	ICO_ASPDPOTION0         = 37,
	ICO_ASPDPOTION1         = 38,
	ICO_ASPDPOTION2         = 39,
	ICO_ASPDPOTION3         = 40,
	ICO_SPEEDPOTION         = 41,
	//42: Again Speed Up
	ICO_STRIPWEAPON         = 50,
	ICO_STRIPSHIELD         = 51,
	ICO_STRIPARMOR          = 52,
	ICO_STRIPHELM           = 53,
	ICO_CP_WEAPON           = 54,
	ICO_CP_SHIELD           = 55,
	ICO_CP_ARMOR            = 56,
	ICO_CP_HELM             = 57,
	ICO_AUTOGUARD           = 58,
	ICO_REFLECTSHIELD       = 59,
	ICO_PROVIDENCE          = 61,
	ICO_DEFENDER            = 62,
	ICO_AUTOSPELL           = 65,
	ICO_SPEARQUICKEN        = 68,
	ICO_EXPLOSIONSPIRITS    = 86,
	ICO_FURY                = 87,
	ICO_FIREWEAPON          = 90,
	ICO_WATERWEAPON         = 91,
	ICO_WINDWEAPON          = 92,
	ICO_EARTHWEAPON         = 93,
// 102 = again gloria - from what I saw on screenshots, I wonder if it isn't gospel... [DracoRPG]
	ICO_AURABLADE           = 103,
	ICO_PARRYING            = 104,
	ICO_CONCENTRATION       = 105,
	ICO_TENSIONRELAX        = 106,
	ICO_BERSERK             = 107,
	ICO_ASSUMPTIO           = 110,
	ICO_GUILDAURA           = 112,
	ICO_MAGICPOWER          = 113,
	ICO_EDP                 = 114,
	ICO_TRUESIGHT           = 115,
	ICO_WINDWALK            = 116,
	ICO_MELTDOWN            = 117,
	ICO_CARTBOOST           = 118,
	ICO_REJECTSWORD         = 120,
	ICO_MARIONETTE          = 121,
	ICO_MARIONETTE2         = 122,
	ICO_MOONLIT             = 123,
	ICO_BLEEDING            = 124,
	ICO_JOINTBEAT           = 125,
	ICO_DEVOTION            = 130,
	ICO_STEELBODY           = 132,
	ICO_CHASEWALK           = 134,
	ICO_READYSTORM          = 135,
	ICO_READYDOWN           = 137,
	ICO_READYTURN           = 139,
	ICO_READYCOUNTER        = 141,
	ICO_DODGE               = 143,
	ICO_SPURT               = 145,
	ICO_SHADOWWEAPON        = 146,
	ICO_ADRENALINE2         = 147,
	ICO_GHOSTWEAPON         = 148,
	ICO_NIGHT               = 149,
	ICO_SPIRIT              = 149,
	ICO_DEVIL               = 152,
	ICO_KAITE               = 153,
	ICO_KAIZEL              = 156,
	ICO_KAAHI               = 157,
	ICO_KAUPE               = 158,
	ICO_SMA			    = 159,
// 160
	ICO_ONEHAND             = 161,
	ICO_WARM                = 165,	
//	166 | The three show the exact same display: ultra red character (165, 166, 167)	
//	167 |	
	ICO_SUN_COMFORT         = 169,
	ICO_MOON_COMFORT        = 170,	
	ICO_STAR_COMFORT        = 171,	
	ICO_PRESERVE            = 181,
	ICO_BATTLEORDERS        = 182,
// 184 = WTF?? creates the black shape of 4_m_02 NPC, with NPC talk cursor
	ICO_DOUBLECAST          = 186,
	ICO_MAXOVERTHRUST       = 188,
	ICO_TAROT               = 191, // the icon allows no doubt... but what is it really used for ?? [DracoRPG]
	ICO_SHRINK              = 197,
	ICO_SIGHTBLASTER        = 198,
	ICO_WINKCHARM           = 199,
	ICO_CLOSECONFINE        = 200,
	ICO_CLOSECONFINE2       = 201,
	ICO_MADNESSCANCEL	= 203,	//[blackhole89]
	ICO_GATLINGFEVER		= 204,
	ICO_TKREST				= 205,
	ICO_UTSUSEMI			= 206,
	ICO_BUNSINJYUTSU		= 207,
	ICO_NEN				= 208,
	ICO_ADJUSTMENT		= 209,
	ICO_ACCURACY			= 210,
	ICO_FUSION			= 211
};
extern int StatusIconTable[SC_MAX];

extern int percentrefinery[5][10];	// ê∏òBê¨å˜ó¶(refine_db.txt)

int status_getrefinebonus(int lv, int type);
int status_percentrefinery(struct map_session_data *sd, struct item *item);

int status_calc_pc(struct map_session_data*, int);
void status_calc_speed(struct map_session_data*); // [Celest]
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
int status_get_speed(struct block_list *bl);
int status_get_adelay(struct block_list *bl);
int status_get_amotion(struct block_list *bl);
int status_get_dmotion(struct block_list *bl);
int status_get_element(struct block_list *bl);
int status_get_attack_element(struct block_list *bl);
int status_get_attack_element2(struct block_list *bl); //ç∂éËïêäÌëÆê´éÊìæ
#define status_get_elem_type(bl) (status_get_element(bl) % 10)
#define status_get_elem_level(bl) (status_get_element(bl) / 10 / 2)
int status_get_party_id(struct block_list *bl);
int status_get_guild_id(struct block_list *bl);
int status_get_race(struct block_list *bl);
int status_get_size(struct block_list *bl);
int status_get_mode(struct block_list *bl);
//int status_get_mexp(struct block_list *bl);
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

// èÛë‘àŸèÌä÷òA skill.c ÇÊÇËà⁄ìÆ
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
