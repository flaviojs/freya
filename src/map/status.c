// Copyright (c) Freya Development Team - Licensed under GNU GPL
// For more information, see License.txt in the main folder

#include <config.h>

#include <string.h>
#include <stdlib.h>

#include "../common/socket.h"
#include "../common/timer.h"
#include "../common/utils.h"

#include "map.h"
#include "nullpo.h"
#include "itemdb.h"
#include "pc.h"
#include "skill.h"
#include "battle.h"
#include "clif.h"
#include "script.h"
#include "guild.h"
#include "mob.h"
#include "chrif.h"
#include "atcommand.h"
#include "pet.h"
#include "status.h"
#include "ranking.h"

#define STATE_BLIND 0x10

int SkillStatusChangeTable[MAX_SKILL]; // Stores the status that should be associated to this skill
int StatusIconTable[SC_MAX]; // Stores the client icons IDs - Every real SC has it's own icon

void initStatusIconTable(void) {
	int i;
	for (i = 0; i < MAX_SKILL; i++)
		SkillStatusChangeTable[i] = -1;
	for (i = 0; i < SC_MAX; i++)
		StatusIconTable[i] = ICO_BLANK;

#define init_sc(skillid, sc_enum, iconid) \
	if (skillid < 0 || skillid > MAX_SKILL) printf("init_sc failed at skillid %d: skillid %d > MAX_SKILL\n", skillid, sc_enum); \
	else if(sc_enum < 0 || sc_enum > SC_MAX) printf("init_sc failed at skillid %d: sc_enum %d > SC_MAX\n", skillid, sc_enum); \
	else SkillStatusChangeTable[skillid] = sc_enum;	\
	     StatusIconTable[sc_enum] = iconid;

	// ENUM SKILLID skill.h         ENUM SC_ status.h    ENUM ICO_ status.h
	init_sc(SM_PROVOKE,             SC_PROVOKE,          ICO_PROVOKE);
	init_sc(SM_ENDURE,              SC_ENDURE,           ICO_ENDURE);
	init_sc(SM_MAGNUM,              SC_MAGNUM,           ICO_BLANK);
	init_sc(MG_SIGHT,               SC_SIGHT,            ICO_BLANK);
	init_sc(MG_SAFETYWALL,          SC_SAFETYWALL,       ICO_BLANK);
	init_sc(MG_FROSTDIVER,          SC_FREEZE,           ICO_BLANK);
	init_sc(MG_STONECURSE,          SC_STONE,            ICO_BLANK);
	init_sc(AL_RUWACH,              SC_RUWACH,           ICO_BLANK);
	init_sc(AL_PNEUMA,              SC_PNEUMA,           ICO_BLANK);
	init_sc(AL_INCAGI,              SC_INCREASEAGI,      ICO_INCREASEAGI);
	init_sc(AL_DECAGI,              SC_DECREASEAGI,      ICO_DECREASEAGI);
	init_sc(AL_CRUCIS,              SC_SIGNUMCRUCIS,     ICO_SIGNUMCRUCIS);
	init_sc(AL_ANGELUS,             SC_ANGELUS,          ICO_ANGELUS);
	init_sc(AL_BLESSING,            SC_BLESSING,         ICO_BLESSING);
	init_sc(AC_CONCENTRATION,       SC_CONCENTRATE,      ICO_CONCENTRATE);
	init_sc(TF_HIDING,              SC_HIDING,           ICO_HIDING);
	init_sc(KN_TWOHANDQUICKEN,      SC_TWOHANDQUICKEN,   ICO_TWOHANDQUICKEN);
	init_sc(KN_AUTOCOUNTER,         SC_AUTOCOUNTER,      ICO_BLANK);
	init_sc(PR_IMPOSITIO,           SC_IMPOSITIO,        ICO_IMPOSITIO);
	init_sc(PR_SUFFRAGIUM,          SC_SUFFRAGIUM,       ICO_SUFFRAGIUM);
	init_sc(PR_ASPERSIO,            SC_ASPERSIO,         ICO_ASPERSIO);
	init_sc(PR_BENEDICTIO,          SC_BENEDICTIO,       ICO_BENEDICTIO);
	init_sc(PR_SLOWPOISON,          SC_SLOWPOISON,       ICO_SLOWPOISON);
	init_sc(PR_KYRIE,               SC_KYRIE,            ICO_KYRIE);
	init_sc(PR_MAGNIFICAT,          SC_MAGNIFICAT,       ICO_MAGNIFICAT);
	init_sc(PR_GLORIA,              SC_GLORIA,           ICO_GLORIA);
	init_sc(PR_LEXDIVINA,           SC_SILENCE,          ICO_BLANK);
	init_sc(PR_LEXAETERNA,          SC_AETERNA,          ICO_AETERNA);
	init_sc(WZ_QUAGMIRE,            SC_QUAGMIRE,         ICO_QUAGMIRE);
	init_sc(BS_ADRENALINE,          SC_ADRENALINE,       ICO_ADRENALINE);
	init_sc(BS_WEAPONPERFECT,       SC_WEAPONPERFECTION, ICO_WEAPONPERFECTION);
	init_sc(BS_OVERTHRUST,          SC_OVERTHRUST,       ICO_OVERTHRUST);
	init_sc(BS_MAXIMIZE,            SC_MAXIMIZEPOWER,    ICO_MAXIMIZEPOWER);
	init_sc(AS_CLOAKING,            SC_CLOAKING,         ICO_CLOAKING);
	init_sc(AS_SONICBLOW,           SC_STUN,             ICO_BLANK);
	init_sc(AS_ENCHANTPOISON,       SC_ENCPOISON,        ICO_ENCPOISON);
	init_sc(AS_POISONREACT,         SC_POISONREACT,      ICO_POISONREACT);
	init_sc(AS_VENOMDUST,           SC_POISON,           ICO_BLANK);
	init_sc(AS_SPLASHER,            SC_SPLASHER,         ICO_BLANK);
	init_sc(NV_TRICKDEAD,           SC_TRICKDEAD,        ICO_TRICKDEAD);
	init_sc(SM_AUTOBERSERK,         SC_AUTOBERSERK,      ICO_PROVOKE);
	init_sc(MC_LOUD,                SC_LOUD,             ICO_LOUD);
	init_sc(MG_ENERGYCOAT,          SC_ENERGYCOAT,       ICO_ENERGYCOAT);
	init_sc(NPC_SELFDESTRUCTION,    SC_SELFDESTRUCTION,  ICO_BLANK);
	init_sc(NPC_KEEPING,            SC_KEEPING,          ICO_BLANK);
	init_sc(NPC_DARKBLESSING,       SC_COMA,             ICO_BLANK);
	init_sc(NPC_BARRIER,            SC_BARRIER,          ICO_BLANK);
	init_sc(NPC_HALLUCINATION,      SC_HALLUCINATION,    ICO_BLANK);
	init_sc(RG_STRIPWEAPON,         SC_STRIPWEAPON,      ICO_STRIPWEAPON);
	init_sc(RG_STRIPSHIELD,         SC_STRIPSHIELD,      ICO_STRIPSHIELD);
	init_sc(RG_STRIPARMOR,          SC_STRIPARMOR,       ICO_STRIPARMOR);
	init_sc(RG_STRIPHELM,           SC_STRIPHELM,        ICO_STRIPHELM);
	init_sc(AM_CP_WEAPON,           SC_CP_WEAPON,        ICO_CP_WEAPON);
	init_sc(AM_CP_SHIELD,           SC_CP_SHIELD,        ICO_CP_SHIELD);
	init_sc(AM_CP_ARMOR,            SC_CP_ARMOR,         ICO_CP_ARMOR);
	init_sc(AM_CP_HELM,             SC_CP_HELM,          ICO_CP_HELM);
	init_sc(CR_AUTOGUARD,           SC_AUTOGUARD,        ICO_AUTOGUARD);
	init_sc(CR_REFLECTSHIELD,       SC_REFLECTSHIELD,    ICO_REFLECTSHIELD);
	init_sc(CR_DEVOTION,            SC_DEVOTION,         ICO_DEVOTION);
	init_sc(CR_PROVIDENCE,          SC_PROVIDENCE,       ICO_PROVIDENCE);
	init_sc(CR_DEFENDER,            SC_DEFENDER,         ICO_DEFENDER);
	init_sc(CR_SPEARQUICKEN,        SC_SPEARQUICKEN,     ICO_SPEARQUICKEN);
	init_sc(MO_STEELBODY,           SC_STEELBODY,        ICO_STEELBODY);
	init_sc(MO_BLADESTOP,           SC_BLADESTOP_WAIT,   ICO_BLANK);
	init_sc(MO_EXPLOSIONSPIRITS,    SC_EXPLOSIONSPIRITS, ICO_EXPLOSIONSPIRITS);
	init_sc(MO_EXTREMITYFIST,       SC_EXTREMITYFIST,    ICO_BLANK);
	init_sc(SA_MAGICROD,            SC_MAGICROD,         ICO_BLANK);
	init_sc(SA_AUTOSPELL,           SC_AUTOSPELL,        ICO_AUTOSPELL);
	init_sc(SA_FLAMELAUNCHER,       SC_FIREWEAPON,       ICO_FIREWEAPON);
	init_sc(SA_FROSTWEAPON,         SC_WATERWEAPON,      ICO_WATERWEAPON);
	init_sc(SA_LIGHTNINGLOADER,     SC_WINDWEAPON,       ICO_WINDWEAPON);
	init_sc(SA_SEISMICWEAPON,       SC_EARTHWEAPON,      ICO_EARTHWEAPON);
	init_sc(SA_VOLCANO,             SC_VOLCANO,          ICO_BLANK);
	init_sc(SA_DELUGE,              SC_DELUGE,           ICO_BLANK);
	init_sc(SA_VIOLENTGALE,         SC_VIOLENTGALE,      ICO_BLANK);
	init_sc(SA_LANDPROTECTOR,       SC_LANDPROTECTOR,    ICO_BLANK);
	init_sc(SA_COMA,                SC_COMA,             ICO_BLANK);
	init_sc(BD_LULLABY,             SC_LULLABY,          ICO_BLANK);
	init_sc(BD_RICHMANKIM,          SC_RICHMANKIM,       ICO_BLANK);
	init_sc(BD_ETERNALCHAOS,        SC_ETERNALCHAOS,     ICO_BLANK);
	init_sc(BD_DRUMBATTLEFIELD,     SC_DRUMBATTLE,       ICO_BLANK);
	init_sc(BD_RINGNIBELUNGEN,      SC_NIBELUNGEN,       ICO_BLANK);
	init_sc(BD_ROKISWEIL,           SC_ROKISWEIL,        ICO_BLANK);
	init_sc(BD_INTOABYSS,           SC_INTOABYSS,        ICO_BLANK);
	init_sc(BD_SIEGFRIED,           SC_SIEGFRIED,        ICO_BLANK);
	init_sc(BA_DISSONANCE,          SC_DISSONANCE,       ICO_BLANK);
	init_sc(BA_WHISTLE,             SC_WHISTLE,          ICO_BLANK);
	init_sc(BA_ASSASSINCROSS,       SC_ASSNCROS,         ICO_BLANK);
	init_sc(BA_POEMBRAGI,           SC_POEMBRAGI,        ICO_BLANK);
	init_sc(BA_APPLEIDUN,           SC_APPLEIDUN,        ICO_BLANK);
	init_sc(DC_UGLYDANCE,           SC_UGLYDANCE,        ICO_BLANK);
	init_sc(DC_HUMMING,             SC_HUMMING,          ICO_BLANK);
	init_sc(DC_DONTFORGETME,        SC_DONTFORGETME,     ICO_BLANK);
	init_sc(DC_FORTUNEKISS,         SC_FORTUNE,          ICO_BLANK);
	init_sc(DC_SERVICEFORYOU,       SC_SERVICEFORYOU,    ICO_BLANK);
	init_sc(NPC_SELFDESTRUCTION2,   SC_SELFDESTRUCTION,  ICO_BLANK);
	init_sc(NPC_STOP,               SC_STOP,             ICO_BLANK);
	init_sc(NPC_BREAKWEAPON,        SC_BROKENWEAPON,     ICO_BROKENWEAPON);
	init_sc(NPC_BREAKARMOR,         SC_BROKENARMOR,      ICO_BROKENARMOR);
	init_sc(LK_AURABLADE,           SC_AURABLADE,        ICO_AURABLADE);
	init_sc(LK_PARRYING,            SC_PARRYING,         ICO_PARRYING);
	init_sc(LK_CONCENTRATION,       SC_CONCENTRATION,    ICO_CONCENTRATION);
	init_sc(LK_TENSIONRELAX,        SC_TENSIONRELAX,     ICO_TENSIONRELAX);
	init_sc(LK_BERSERK,             SC_BERSERK,          ICO_BERSERK);
	init_sc(HP_ASSUMPTIO,           SC_ASSUMPTIO,        ICO_ASSUMPTIO);
	init_sc(HP_BASILICA,            SC_BASILICA,         ICO_BLANK);
	init_sc(HW_MAGICPOWER,          SC_MAGICPOWER,       ICO_MAGICPOWER);
	init_sc(PA_SACRIFICE,           SC_SACRIFICE,        ICO_BLANK);
	init_sc(PA_GOSPEL,              SC_GOSPEL,           ICO_BLANK);
	init_sc(ASC_EDP,                SC_EDP,              ICO_EDP);
	init_sc(SN_SIGHT,               SC_TRUESIGHT,        ICO_TRUESIGHT);
	init_sc(SN_WINDWALK,            SC_WINDWALK,         ICO_WINDWALK);
	init_sc(WS_MELTDOWN,            SC_MELTDOWN,         ICO_MELTDOWN);
	init_sc(WS_CARTBOOST,           SC_CARTBOOST,        ICO_CARTBOOST);
	init_sc(ST_CHASEWALK,           SC_CHASEWALK,        ICO_BLANK);
	init_sc(ST_REJECTSWORD,         SC_REJECTSWORD,      ICO_REJECTSWORD);
	init_sc(CG_MOONLIT,             SC_MOONLIT,          ICO_MOONLIT);
	init_sc(CG_MARIONETTE,          SC_MARIONETTE,       ICO_MARIONETTE);
	init_sc(LK_HEADCRUSH,           SC_BLEEDING,         ICO_BLEEDING);
	init_sc(LK_JOINTBEAT,           SC_JOINTBEAT,        ICO_JOINTBEAT);
	init_sc(PF_MINDBREAKER,         SC_MINDBREAKER,      ICO_BLANK);
	init_sc(PF_MEMORIZE,            SC_MEMORIZE,         ICO_BLANK);
	init_sc(PF_FOGWALL,             SC_FOGWALL,          ICO_BLANK);
	init_sc(PF_SPIDERWEB,           SC_SPIDERWEB,        ICO_BLANK);
	init_sc(ST_PRESERVE,            SC_PRESERVE,         ICO_PRESERVE);
	init_sc(PF_DOUBLECASTING,       SC_DOUBLECAST,       ICO_DOUBLECAST);
	init_sc(HW_GRAVITATION,         SC_GRAVITATION,      ICO_BLANK);
	init_sc(WS_MAXOVERTHRUST,       SC_MAXOVERTHRUST,    ICO_MAXOVERTHRUST);
	init_sc(CG_LONGINGFREEDOM,      SC_LONGING,          ICO_BLANK);
	init_sc(CG_HERMODE,             SC_HERMODE,          ICO_BLANK);
	init_sc(CR_SHRINK,              SC_SHRINK,           ICO_SHRINK);
	init_sc(RG_CLOSECONFINE,        SC_CLOSECONFINE2,    ICO_CLOSECONFINE2);
	init_sc(RG_CLOSECONFINE,        SC_CLOSECONFINE,     ICO_CLOSECONFINE);
	init_sc(WZ_SIGHTBLASTER,        SC_SIGHTBLASTER,     ICO_SIGHTBLASTER);
	init_sc(TK_RUN,                 SC_RUN,              ICO_BLANK);
	init_sc(TK_READYSTORM,          SC_READYSTORM,       ICO_READYSTORM);
	init_sc(TK_READYDOWN,           SC_READYDOWN,        ICO_READYDOWN);
	init_sc(TK_READYTURN,           SC_READYTURN,        ICO_READYTURN);
	init_sc(TK_READYCOUNTER,        SC_READYCOUNTER,     ICO_READYCOUNTER);
	init_sc(TK_DODGE,               SC_READYDODGE,       ICO_DODGE);
	init_sc(TK_SPTIME,              SC_TKREST,           ICO_TKREST);
	init_sc(TK_SEVENWIND,           SC_GHOSTWEAPON,      ICO_GHOSTWEAPON);
	init_sc(TK_SEVENWIND,           SC_SHADOWWEAPON,     ICO_SHADOWWEAPON);
	init_sc(SG_SUN_WARM,            SC_WARM,             ICO_WARM);
	init_sc(SG_SUN_COMFORT,         SC_SUN_COMFORT,      ICO_SUN_COMFORT);
	init_sc(SG_MOON_COMFORT,        SC_MOON_COMFORT,     ICO_MOON_COMFORT);
	init_sc(SG_STAR_COMFORT,        SC_STAR_COMFORT,     ICO_STAR_COMFORT);
	init_sc(SG_KNOWLEDGE,           SC_KNOWLEDGE,        ICO_BLANK);
	init_sc(SG_FUSION,              SC_FUSION,           ICO_BLANK);
	init_sc(SL_SKE,                 SC_SKE,              ICO_BLANK);
	init_sc(SL_SKA,                 SC_SKA,              ICO_BLANK);
	init_sc(SL_SWOO,                SC_SWOO,             ICO_BLANK);
	init_sc(SL_SMA,                 SC_SMA,              ICO_BLANK);
	init_sc(SL_KAIZEL,              SC_KAIZEL,           ICO_KAIZEL);
	init_sc(SL_KAAHI,               SC_KAAHI,            ICO_KAAHI);
	init_sc(SL_KAUPE,               SC_KAUPE,            ICO_KAUPE);
	init_sc(SL_KAITE,               SC_KAITE,            ICO_KAITE);
	init_sc(SL_ALCHEMIST,           SC_SPIRIT,           ICO_SPIRIT);
	init_sc(SL_ASSASIN,             SC_SPIRIT,           ICO_SPIRIT);
	init_sc(SL_BARDDANCER,          SC_SPIRIT,           ICO_SPIRIT);
	init_sc(SL_BLACKSMITH,          SC_SPIRIT,           ICO_SPIRIT);
	init_sc(SL_COLLECTOR,           SC_SPIRIT,           ICO_SPIRIT);
	init_sc(SL_CRUSADER,            SC_SPIRIT,           ICO_SPIRIT);
	init_sc(SL_DEATHKNIGHT,         SC_SPIRIT,           ICO_SPIRIT);
	init_sc(SL_GUNNER,              SC_SPIRIT,           ICO_SPIRIT);
	init_sc(SL_HUNTER,              SC_SPIRIT,           ICO_SPIRIT);
	init_sc(SL_KNIGHT,              SC_SPIRIT,           ICO_SPIRIT);
	init_sc(SL_MONK,                SC_SPIRIT,           ICO_SPIRIT);
	init_sc(SL_NINJA,               SC_SPIRIT,           ICO_SPIRIT);
	init_sc(SL_PRIEST,              SC_SPIRIT,           ICO_SPIRIT);
	init_sc(SL_ROGUE,               SC_SPIRIT,           ICO_SPIRIT);
	init_sc(SL_SAGE,                SC_SPIRIT,           ICO_SPIRIT);
	init_sc(SL_SOULLINKER,          SC_SPIRIT,           ICO_SPIRIT);
	init_sc(SL_STAR,                SC_SPIRIT,           ICO_SPIRIT);
	init_sc(SL_SUPERNOVICE,         SC_SPIRIT,           ICO_SPIRIT);
	init_sc(SL_WIZARD,              SC_SPIRIT,           ICO_SPIRIT);
	init_sc(BS_ADRENALINE2,         SC_ADRENALINE2,      ICO_ADRENALINE2);
	init_sc(KN_ONEHAND,             SC_ONEHAND,          ICO_ONEHAND);
	init_sc(GS_FLING,               SC_FLING,            ICO_BLANK);
	init_sc(GS_CRACKER,             SC_STUN,             ICO_BLANK);
	init_sc(GS_DISARM,              SC_STRIPWEAPON,      ICO_BLANK);
	init_sc(GS_MADNESSCANCEL,       SC_MADNESSCANCEL,    ICO_MADNESSCANCEL);
	init_sc(GS_ADJUSTMENT,          SC_ADJUSTMENT,       ICO_ADJUSTMENT);
	init_sc(GS_INCREASING,          SC_INCREASING,       ICO_ACCURACY);
	init_sc(GS_GATLINGFEVER,        SC_GATLINGFEVER,     ICO_GATLINGFEVER);
	init_sc(NJ_NEN,                 SC_NEN,              ICO_NEN);
	init_sc(NJ_UTSUSEMI,            SC_UTSUSEMI,         ICO_UTSUSEMI);
	init_sc(NJ_BUNSINJYUTSU,        SC_BUNSINJYUTSU,     ICO_BUNSINJYUTSU);
	init_sc(NJ_TATAMIGAESHI,        SC_TATAMIGAESHI,     ICO_BLANK);
	init_sc(NJ_SUITON,              SC_SUITON,           ICO_BLANK);
#undef init_sc

	// Misc status icons - Non-Skills
	StatusIconTable[SC_WEIGHT50]          = ICO_WEIGHT50;
	StatusIconTable[SC_WEIGHT90]          = ICO_WEIGHT90;
	StatusIconTable[SC_RIDING]            = ICO_RIDING;
	StatusIconTable[SC_FALCON]            = ICO_FALCON;
	StatusIconTable[SC_ASPDPOTION0]       = ICO_ASPDPOTION;
	StatusIconTable[SC_ASPDPOTION1]       = ICO_ASPDPOTION;
	StatusIconTable[SC_ASPDPOTION2]       = ICO_ASPDPOTION;
	StatusIconTable[SC_ASPDPOTION3]       = ICO_ASPDPOTION;
	StatusIconTable[SC_SPEEDUP0]          = ICO_SPEEDPOTION1;
	StatusIconTable[SC_SPEEDUP1]          = ICO_SPEEDPOTION2;
	StatusIconTable[SC_INCSTR]            = ICO_INCSTR;
	StatusIconTable[SC_MIRACLE]           = ICO_SPIRIT;
	StatusIconTable[SC_INTRAVISION]       = ICO_INTRAVISION;
	StatusIconTable[SC_ELEMENTALCHANGE]   = ICO_UNDEAD;
	StatusIconTable[SC_STRFOOD]           = ICO_STRFOOD;
	StatusIconTable[SC_AGIFOOD]           = ICO_AGIFOOD;
	StatusIconTable[SC_VITFOOD]           = ICO_VITFOOD;
	StatusIconTable[SC_INTFOOD]           = ICO_INTFOOD;
	StatusIconTable[SC_DEXFOOD]           = ICO_DEXFOOD;
	StatusIconTable[SC_LUKFOOD]           = ICO_LUKFOOD;
	StatusIconTable[SC_FLEEFOOD]          = ICO_FLEEFOOD;
	StatusIconTable[SC_HITFOOD]           = ICO_HITFOOD;

	// Guild skills have a base index of 10000
	// -> Therefore they dont fit in SkillStatusChangeTable as the MAX index is MAX_SKILL (1020)
	StatusIconTable[SC_BATTLEORDERS] = ICO_BLANK;
	StatusIconTable[SC_GUILDAURA]    = ICO_BLANK;

	// Disable Hallucination (Will not work on @setbattleflag)
	if (!battle_config.display_hallucination)
		StatusIconTable[SC_HALLUCINATION] = ICO_BLANK;
}

// Database Loading Arrays
static int refinebonus[5][3]; // refine_db.txt
int percentrefinery[5][10]; // refine_db.txt
static int max_weight_base[MAX_PC_JOB_CLASS]; // job_db1.txt
static int hp_coefficient[MAX_PC_JOB_CLASS]; // job_db1.txt
static int hp_coefficient2[MAX_PC_JOB_CLASS]; // job_db1.txt
static int hp_sigma_val[MAX_PC_JOB_CLASS][MAX_LEVEL]; // job_db1.txt
static int sp_coefficient[MAX_PC_JOB_CLASS]; // job_db1.txt
static int aspd_base[MAX_PC_JOB_CLASS][23]; // job_db1.txt
static char job_bonus[3][MAX_PC_JOB_CLASS][MAX_LEVEL]; // job_db1.txt

static int atkmods[3][23];	// size_fix.txt

/*==========================================
 *
 *------------------------------------------
 */
int status_getrefinebonus(int lv, int type) {

	if (lv >= 0 && lv < 5 && type >= 0 && type < 3)
		return refinebonus[lv][type];

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int status_percentrefinery(struct map_session_data *sd, struct item *item) {

	int percent;

	nullpo_retr(0, item);
	percent = percentrefinery[(int)itemdb_wlv(item->nameid)][(int)item->refine];

	percent += pc_checkskill(sd, BS_WEAPONRESEARCH);

	if (percent > 100) {
		percent = 100;
	} else if (percent < 0) {
		percent = 0;
	}

	return percent;
}

/*==========================================
 *
 *------------------------------------------
 */
int status_calc_pc(struct map_session_data* sd, int first) {

	int b_max_hp, b_max_sp, b_hp, b_sp, b_weight, b_max_weight, b_paramb[6], b_parame[6], b_hit, b_flee;
	int b_aspd, b_watk, b_def, b_watk2, b_def2, b_flee2, b_critical, b_attackrange, b_matk1, b_matk2, b_mdef, b_mdef2, b_class;
	int b_base_atk;
	struct skill b_skill[MAX_SKILL];
	int i, bl, idx;
	int skill, aspd_rate, wele, wele_, def_ele, refinedef = 0;
	int pele = 0, pdef_ele = 0;
	int str, dstr, dex;
	struct pc_base_job s_class;

	nullpo_retr(0, sd);

	s_class = pc_calc_base_job(sd->status.class);

	b_max_hp = sd->status.max_hp;
	b_max_sp = sd->status.max_sp;
	b_hp = sd->status.hp;
	b_sp = sd->status.sp;
	b_weight = sd->weight;
	b_max_weight = sd->max_weight;
	memcpy(b_paramb, &sd->paramb, sizeof(b_paramb));
	memcpy(b_parame, &sd->paramc, sizeof(b_parame));
	memcpy(b_skill, &sd->status.skill, sizeof(b_skill));
	b_hit = sd->hit;
	b_flee = sd->flee;
	b_aspd = sd->aspd;
	b_watk = sd->watk;
	b_def = sd->def;
	b_watk2 = sd->watk2;
	b_def2 = sd->def2;
	b_flee2 = sd->flee2;
	b_critical = sd->critical;
	b_attackrange = sd->attackrange;
	b_matk1 = sd->matk1;
	b_matk2 = sd->matk2;
	b_mdef = sd->mdef;
	b_mdef2 = sd->mdef2;
	b_class = sd->view_class;
	sd->view_class = sd->status.class;
	b_base_atk = sd->base_atk;

	pc_calc_skilltree(sd);

	sd->max_weight = max_weight_base[s_class.job] + sd->status.str * 300;

	if (first & 1) {
		sd->weight = 0;
		for(i = 0;i<MAX_INVENTORY; i++){
			if (sd->status.inventory[i].nameid == 0 || sd->inventory_data[i] == NULL)
				continue;
			sd->weight += sd->inventory_data[i]->weight * sd->status.inventory[i].amount;
		}
		sd->cart_max_weight = battle_config.max_cart_weight;
		sd->cart_weight = 0;
		sd->cart_num = 0;
		for(i = 0; i < MAX_CART; i++) {
			if (sd->status.cart[i].nameid == 0)
				continue;
			sd->cart_weight += itemdb_weight(sd->status.cart[i].nameid) * sd->status.cart[i].amount;
			sd->cart_num++;
		}
	}

	memset(sd->paramb,0,sizeof(sd->paramb));
	memset(sd->parame,0,sizeof(sd->parame));
	sd->hit = 0;
	sd->flee = 0;
	sd->flee2 = 0;
	sd->critical = 0;
	sd->aspd = 0;
	sd->watk = 0;
	sd->def = 0;
	sd->mdef = 0;
	sd->watk2 = 0;
	sd->def2 = 0;
	sd->mdef2 = 0;
	sd->status.max_hp = 0;
	sd->status.max_sp = 0;
	sd->attackrange = 0;
	sd->attackrange_ = 0;
	sd->atk_ele = 0;
	sd->def_ele = 0;
	sd->star = 0;
	sd->overrefine = 0;
	sd->matk1 = 0;
	sd->matk2 = 0;
	sd->hprate = battle_config.hp_rate;
	sd->sprate = battle_config.sp_rate;
	sd->castrate = 100;
	sd->delayrate = 100;
	sd->dsprate = 100;
	sd->base_atk = 0;
	sd->arrow_atk = 0;
	sd->arrow_ele = 0;
	sd->arrow_hit = 0;
	sd->arrow_range = 0;
	sd->nhealhp = sd->nhealsp = sd->nshealhp = sd->nshealsp = sd->nsshealhp = sd->nsshealsp = 0;
	memset(sd->addele,0,sizeof(sd->addele));
	memset(sd->addrace,0,sizeof(sd->addrace));
	memset(sd->addsize,0,sizeof(sd->addsize));
	memset(sd->addele_,0,sizeof(sd->addele_));
	memset(sd->addrace_,0,sizeof(sd->addrace_));
	memset(sd->addsize_,0,sizeof(sd->addsize_));
	memset(sd->subele,0,sizeof(sd->subele));
	memset(sd->subrace,0,sizeof(sd->subrace));
	memset(sd->addeff,0,sizeof(sd->addeff));
	memset(sd->addeff2,0,sizeof(sd->addeff2));
	memset(sd->reseff,0,sizeof(sd->reseff));
	memset(sd->sp_gain_race,0,sizeof(sd->sp_gain_race));
	memset(sd->sp_gain_value_race,0,sizeof(sd->sp_gain_value_race));
	memset(sd->expaddrace,0,sizeof(sd->expaddrace));
	memset(&sd->special_state,0,sizeof(sd->special_state));
	memset(sd->weapon_coma_ele,0,sizeof(sd->weapon_coma_ele));
	memset(sd->weapon_coma_race,0,sizeof(sd->weapon_coma_race));
	memset(sd->weapon_atk,0,sizeof(sd->weapon_atk));
	memset(sd->weapon_atk_rate,0,sizeof(sd->weapon_atk_rate));

	sd->watk_ = 0;
	sd->watk_2 = 0;
	sd->atk_ele_ = 0;
	sd->star_ = 0;
	sd->overrefine_ = 0;

	sd->aspd_rate = 100;
	sd->hprecov_rate = 100;
	sd->sprecov_rate = 100;
	sd->critical_def = 0;
	sd->double_rate = 0;
	sd->near_attack_def_rate = sd->long_attack_def_rate = 0;
	sd->atk_rate = sd->matk_rate = 100;
	sd->ignore_def_ele = sd->ignore_def_race = 0;
	sd->ignore_def_ele_ = sd->ignore_def_race_ = 0;
	sd->ignore_mdef_ele = sd->ignore_mdef_race = 0;
	sd->arrow_cri = 0;
	sd->magic_def_rate = sd->misc_def_rate = 0;
	memset(sd->arrow_addele,0,sizeof(sd->arrow_addele));
	memset(sd->arrow_addrace,0,sizeof(sd->arrow_addrace));
	memset(sd->arrow_addsize,0,sizeof(sd->arrow_addsize));
	memset(sd->arrow_addeff,0,sizeof(sd->arrow_addeff));
	memset(sd->arrow_addeff2,0,sizeof(sd->arrow_addeff2));
	memset(sd->magic_addele,0,sizeof(sd->magic_addele));
	memset(sd->magic_addrace,0,sizeof(sd->magic_addrace));
	memset(sd->magic_subrace,0,sizeof(sd->magic_subrace));
	sd->perfect_hit = 0;
	sd->critical_rate = sd->hit_rate = sd->flee_rate = sd->flee2_rate = 100;
	sd->def_rate = sd->def2_rate = sd->mdef_rate = sd->mdef2_rate = 100;
	sd->def_ratio_atk_ele = sd->def_ratio_atk_ele_ = 0;
	sd->def_ratio_atk_race = sd->def_ratio_atk_race_ = 0;
	sd->get_zeny_num = 0;
	sd->get_zeny_rate = 0;
	sd->add_damage_class_count = sd->add_damage_class_count_ = sd->add_magic_damage_class_count = 0;
	sd->add_def_class_count = sd->add_mdef_class_count = 0;
	sd->monster_drop_item_count = 0;
	memset(sd->add_damage_classrate,0,sizeof(sd->add_damage_classrate));
	memset(sd->add_damage_classrate_,0,sizeof(sd->add_damage_classrate_));
	memset(sd->add_magic_damage_classrate,0,sizeof(sd->add_magic_damage_classrate));
	memset(sd->add_def_classrate,0,sizeof(sd->add_def_classrate));
	memset(sd->add_mdef_classrate,0,sizeof(sd->add_mdef_classrate));
	memset(sd->monster_drop,0,sizeof(sd->monster_drop));
	memset(sd->autospell,0,sizeof(sd->autospell));
	memset(sd->autospell2,0,sizeof(sd->autospell2));
	memset(sd->autospell3,0,sizeof(sd->autospell3));
	sd->speed_add_rate = sd->aspd_add_rate = sd->speed_rate = 100;
	sd->double_add_rate = sd->perfect_hit_add = sd->get_zeny_add_num = 0;
	sd->splash_range = sd->splash_add_range = 0;
	sd->hp_drain_rate = sd->hp_drain_per = sd->sp_drain_rate = sd->sp_drain_per = 0;
	sd->hp_drain_rate_ = sd->hp_drain_per_ = sd->sp_drain_rate_ = sd->sp_drain_per_ = 0;
	sd->hp_drain_value = sd->hp_drain_value_ = sd->sp_drain_value = sd->sp_drain_value_ = 0;
	sd->short_weapon_damage_return = sd->long_weapon_damage_return = 0;
	sd->magic_damage_return = 0;
	sd->random_attack_increase_add = sd->random_attack_increase_per = 0;
	sd->sp_vanish_rate = sd->sp_vanish_per = 0;
	sd->unbreakable_equip = 0;
	sd->classchange = 0;

	sd->break_weapon_rate = sd->break_armor_rate = 0;
	sd->add_steal_rate = 0;
	sd->crit_atk_rate = 0;
	sd->no_regen = 0;
	sd->unstripable_equip = 0;
	memset(sd->critaddrace, 0, sizeof(sd->critaddrace));
	memset(sd->addeff3, 0, sizeof(sd->addeff3));
	memset(sd->skillatk, 0, sizeof(sd->skillatk));
	sd->add_damage_class_count = sd->add_damage_class_count_ = sd->add_magic_damage_class_count = 0;
	sd->add_def_class_count = sd->add_mdef_class_count = 0;
	sd->add_damage_class_count2 = 0;
	memset(sd->add_damage_classid, 0, sizeof(sd->add_damage_classid));
	memset(sd->add_damage_classid_, 0, sizeof(sd->add_damage_classid_));
	memset(sd->add_magic_damage_classid, 0, sizeof(sd->add_magic_damage_classid));
	memset(sd->add_damage_classrate, 0, sizeof(sd->add_damage_classrate));
	memset(sd->add_damage_classrate_, 0, sizeof(sd->add_damage_classrate_));
	memset(sd->add_magic_damage_classrate, 0, sizeof(sd->add_magic_damage_classrate));
	memset(sd->add_def_classid, 0, sizeof(sd->add_def_classid));
	memset(sd->add_def_classrate, 0, sizeof(sd->add_def_classrate));
	memset(sd->add_mdef_classid, 0, sizeof(sd->add_mdef_classid));
	memset(sd->add_mdef_classrate, 0, sizeof(sd->add_mdef_classrate));
	memset(sd->add_damage_classid2, 0, sizeof(sd->add_damage_classid2));
	memset(sd->add_damage_classrate2, 0, sizeof(sd->add_damage_classrate2));
	sd->sp_gain_value = 0;
	sd->ignore_def_mob = sd->ignore_def_mob_ = 0;
	sd->hp_loss_rate = sd->hp_loss_value = sd->hp_loss_type = 0;
	memset(sd->addrace2, 0, sizeof(sd->addrace2));
	memset(sd->addrace2_, 0, sizeof(sd->addrace2_));
	sd->hp_gain_value = sd->sp_drain_type = 0;
	memset(sd->subsize, 0, sizeof(sd->subsize));
	sd->unequip_hpdamage = 0;
	sd->unequip_spdamage = 0;
	memset(sd->itemhealrate, 0, sizeof(sd->itemhealrate));

	if (!sd->disguiseflag && sd->disguise) {
		sd->disguise = 0;
		clif_changelook(&sd->bl, LOOK_WEAPON, sd->status.weapon);
		clif_changelook(&sd->bl, LOOK_SHIELD, sd->status.shield);
		clif_changelook(&sd->bl, LOOK_HEAD_BOTTOM, sd->status.head_bottom);
		clif_changelook(&sd->bl, LOOK_HEAD_TOP, sd->status.head_top);
		clif_changelook(&sd->bl, LOOK_HEAD_MID, sd->status.head_mid);
		clif_clearchar(&sd->bl, 9);
		pc_setpos(sd, sd->mapname, sd->bl.x, sd->bl.y, 3, 1);
	}

	for(i = 0; i < 10; i++) {
		idx = sd->equip_index[i];
		if (idx < 0)
			continue;
		if (i == 9 && sd->equip_index[8] == idx)
			continue;
		if (i == 5 && sd->equip_index[4] == idx)
			continue;
		if (i == 6 && (sd->equip_index[5] == idx || sd->equip_index[4] == idx))
			continue;

		if (sd->inventory_data[idx]) {
			if (sd->inventory_data[idx]->type == 4) {
				if (sd->status.inventory[idx].card[0]!=0x00ff && sd->status.inventory[idx].card[0]!=0x00fe && sd->status.inventory[idx].card[0]!=(short)0xff00) {
					int j;
					for(j=0;j<sd->inventory_data[idx]->slot;j++) {
						int c=sd->status.inventory[idx].card[j];
						if (c>0) {
							if (i == 8 && sd->status.inventory[idx].equip == 0x20)
								sd->state.lr_flag = 1;
							run_script(itemdb_equipscript(c),0,sd->bl.id,0);
							sd->state.lr_flag = 0;
						}
					}
				}
			} else if(sd->inventory_data[idx]->type==5) {
				if (sd->status.inventory[idx].card[0]!=0x00ff && sd->status.inventory[idx].card[0]!=0x00fe && sd->status.inventory[idx].card[0]!=(short)0xff00) {
					int j;
					for(j=0;j<sd->inventory_data[idx]->slot;j++){
						int c=sd->status.inventory[idx].card[j];
						if (c>0)
							run_script(itemdb_equipscript(c),0,sd->bl.id,0);
					}
				}
			}
		}
	}
	wele = sd->atk_ele;
	wele_ = sd->atk_ele_;
	def_ele = sd->def_ele;
	if (sd->status.pet_id > 0) {
		struct pet_data *pd = sd->pd;
		if ((pd && battle_config.pet_status_support==1) && (battle_config.pet_equip_required==0 || (battle_config.pet_equip_required && pd->equip > 0))) {
			if (sd->status.pet_id > 0 && sd->petDB && sd->pet.intimate > 0 &&
			    pd->state.skillbonus == 1) {
				pc_bonus(sd, pd->skillbonustype, pd->skillbonusval);
			}
			pele = sd->atk_ele;
			pdef_ele = sd->def_ele;
			sd->atk_ele = sd->def_ele = 0;
		}
	}
	memcpy(sd->paramcard,sd->parame,sizeof(sd->paramcard));

	for(i=0;i<10;i++) {
		idx = sd->equip_index[i];
		if (idx < 0)
			continue;
		if (i == 9 && sd->equip_index[8] == idx)
			continue;
		if (i == 5 && sd->equip_index[4] == idx)
			continue;
		if (i == 6 && (sd->equip_index[5] == idx || sd->equip_index[4] == idx))
			continue;
		if (sd->inventory_data[idx]) {
			sd->def += sd->inventory_data[idx]->def;
			if (sd->inventory_data[idx]->type == 4) {
				int r,wlv = sd->inventory_data[idx]->wlv;
				if (i == 8 && sd->status.inventory[idx].equip == 0x20) {
					sd->watk_ += sd->inventory_data[idx]->atk;
					sd->watk_2 = (r=sd->status.inventory[idx].refine)*
						refinebonus[wlv][0];
					if ((r-=refinebonus[wlv][2])>0)
						sd->overrefine_ = r*refinebonus[wlv][1];

					if (sd->status.inventory[idx].card[0] == 0x00ff) {
						sd->star_ = (sd->status.inventory[idx].card[1]>>8);
						wele_ = (sd->status.inventory[idx].card[1]&0x0f);
						if (ranking_id2rank(*((unsigned long *)(&sd->status.inventory[idx].card[2])), RK_BLACKSMITH))
							sd->star += 10; // Forged weapons from the top 10 ranking Blacksmiths will receive an additional absolute damage of 10 (pierces vit/armor def) [Proximus]
					}
					sd->attackrange_ += sd->inventory_data[idx]->range;
					sd->state.lr_flag = 1;
					run_script(sd->inventory_data[idx]->equip_script,0,sd->bl.id,0);
					sd->state.lr_flag = 0;
				} else {
					sd->watk += sd->inventory_data[idx]->atk;
					sd->watk2 += (r = sd->status.inventory[idx].refine) *
						refinebonus[wlv][0];
					if ((r-=refinebonus[wlv][2])>0)
						sd->overrefine += r*refinebonus[wlv][1];

					if (sd->status.inventory[idx].card[0] == 0x00ff) {
						sd->star += (sd->status.inventory[idx].card[1]>>8);
						wele = (sd->status.inventory[idx].card[1]&0x0f);
						if (ranking_id2rank(*((unsigned long *)(&sd->status.inventory[idx].card[2])), RK_BLACKSMITH))
							sd->star += 10; // Forged weapons from the top 10 ranking Blacksmiths will receive an additional absolute damage of 10 (pierces vit/armor def) [Proximus]
					}
					sd->attackrange += sd->inventory_data[idx]->range;
					run_script(sd->inventory_data[idx]->equip_script,0,sd->bl.id,0);
				}
			} else if (sd->inventory_data[idx]->type == 5) {
				sd->watk += sd->inventory_data[idx]->atk;
				refinedef += sd->status.inventory[idx].refine * refinebonus[0][0];
				run_script(sd->inventory_data[idx]->equip_script, 0, sd->bl.id, 0);
			}
		}
	}

	if (sd->equip_index[10] >= 0) {
		idx = sd->equip_index[10];
		if (sd->inventory_data[idx]) {
			sd->state.lr_flag = 2;
			run_script(sd->inventory_data[idx]->equip_script,0,sd->bl.id,0);
			sd->state.lr_flag = 0;
			sd->arrow_atk += sd->inventory_data[idx]->atk;
		}
	}
	sd->def += (refinedef+50)/100;

	if (sd->attackrange < 1) sd->attackrange = 1;
	if (sd->attackrange_ < 1) sd->attackrange_ = 1;
	if (sd->attackrange < sd->attackrange_)
		sd->attackrange = sd->attackrange_;
	if (sd->status.weapon == 11 || sd->status.weapon == 17 || sd->status.weapon == 18 ||
		 sd->status.weapon == 19 || sd->status.weapon == 20 || sd->status.weapon == 21)
		sd->attackrange += sd->arrow_range;
	if (wele > 0)
		sd->atk_ele = wele;
	if (wele_ > 0)
		sd->atk_ele_ = wele_;
	if (def_ele > 0)
		sd->def_ele = def_ele;
	if (battle_config.pet_status_support) {
		if (pele > 0 && !sd->atk_ele)
			sd->atk_ele = pele;
		if (pdef_ele > 0 && !sd->def_ele)
			sd->def_ele = pdef_ele;
	}
	sd->double_rate += sd->double_add_rate;
	sd->perfect_hit += sd->perfect_hit_add;
	sd->get_zeny_num += sd->get_zeny_add_num;
	sd->splash_range += sd->splash_add_range;
	if (sd->aspd_add_rate != 100)
		sd->aspd_rate += sd->aspd_add_rate - 100;

	sd->atkmods[0] = atkmods[0][sd->weapontype1];
	sd->atkmods[1] = atkmods[1][sd->weapontype1];
	sd->atkmods[2] = atkmods[2][sd->weapontype1];

	sd->atkmods_[0] = atkmods[0][sd->weapontype2];
	sd->atkmods_[1] = atkmods[1][sd->weapontype2];
	sd->atkmods_[2] = atkmods[2][sd->weapontype2];

	for(i = 0; i < sd->status.job_level && i < MAX_LEVEL; i++) {
		if (job_bonus[s_class.upper][s_class.job][i])
			sd->paramb[job_bonus[s_class.upper][s_class.job][i]-1]++;
	}

	if ((skill = pc_checkskill(sd, MC_INCCARRY)) > 0)
		sd->max_weight += skill * 2000; // kRO 14/12/04 Patch: Every level now increases capacity of maximum weight by 200 instead of 100 [Aalye]

	if ((skill = pc_checkskill(sd, AC_OWL)) > 0)
		sd->paramb[4] += skill;

	if ((skill = pc_checkskill(sd, BS_HILTBINDING)) > 0) { // Hilt Binding gives +1 Str +4 Atk
		sd->paramb[0] ++;
		sd->base_atk += 4;
	}

	if ((skill = pc_checkskill(sd, SA_DRAGONOLOGY)) > 0)
		sd->paramb[3] += (skill % 2 == 0) ? skill / 2 : (skill + 1) / 2;

	// TK_Run adds +10 ATK per level if no weapon is equipped
	if ((skill = pc_checkskill(sd, TK_RUN)) > 0) {
		if (sd->status.weapon == 0)
			sd->base_atk += skill * 10;
		}

	// Spirit of the Rebirth Class Stat Buffs
	if (sd->sc_data[SC_SPIRIT].timer != -1 && sd->sc_data[SC_SPIRIT].val2 == SL_HIGH && sd->status.base_level <= 69) {
		if (sd->status.str < 50)
			sd->paramb[0] = 50;
		if (sd->status.agi < 50)
			sd->paramb[1] = 50;
		if (sd->status.vit < 50)
			sd->paramb[2] = 50;
		if (sd->status.int_ < 50)
			sd->paramb[3] = 50;
		if (sd->status.dex < 50)
			sd->paramb[4] = 50;
		if (sd->status.luk < 50)
			sd->paramb[5] = 50;
	}

	if (sd->sc_count) {
		if (sd->sc_data[SC_INCSTR].timer != -1)
			sd->paramb[0] += sd->sc_data[SC_INCSTR].val1;
		if (sd->sc_data[SC_CONCENTRATE].timer != -1 && sd->sc_data[SC_QUAGMIRE].timer == -1) {
			sd->paramb[1] += (sd->status.agi + sd->paramb[1] + sd->parame[1] - sd->paramcard[1]) * (2 + sd->sc_data[SC_CONCENTRATE].val1) / 100;
			sd->paramb[4] += (sd->status.dex + sd->paramb[4] + sd->parame[4] - sd->paramcard[4]) * (2 + sd->sc_data[SC_CONCENTRATE].val1) / 100;
		}
		if (sd->sc_data[SC_NEN].timer != -1) {
			sd->paramb[0] += sd->sc_data[SC_NEN].val1;
			sd->paramb[3] += sd->sc_data[SC_NEN].val1;
		}
		if (sd->sc_data[SC_INCREASEAGI].timer != -1 && sd->sc_data[SC_QUAGMIRE].timer == -1 && sd->sc_data[SC_DONTFORGETME].timer == -1)
			sd->paramb[1] += 2 + sd->sc_data[SC_INCREASEAGI].val1;

		if (sd->sc_data[SC_DECREASEAGI].timer != -1)
			sd->paramb[1] -= 2 + sd->sc_data[SC_DECREASEAGI].val1;

		if (sd->sc_data[SC_SUITON].timer != -1 && sd->sc_data[SC_SUITON].val3)
			sd->paramb[1] -= sd->sc_data[SC_SUITON].val2;

		if (sd->sc_data[SC_CLOAKING].timer != -1) {
			sd->critical_rate += 100;
		}
		if (sd->sc_data[SC_CHASEWALK].timer != -1)
			if (sd->sc_data[SC_CHASEWALK].val4)
				sd->paramb[0] += (1 << (sd->sc_data[SC_CHASEWALK].val1 - 1)); // Increases STR after 10 seconds

		if (sd->sc_data[SC_BLESSING].timer!=-1) {
			sd->paramb[0] += sd->sc_data[SC_BLESSING].val1;
			sd->paramb[3] += sd->sc_data[SC_BLESSING].val1;
			sd->paramb[4] += sd->sc_data[SC_BLESSING].val1;
		}
		if (sd->sc_data[SC_GLORIA].timer!=-1)
			sd->paramb[5]+= 30;
		if (sd->sc_data[SC_LOUD].timer!=-1 && sd->sc_data[SC_QUAGMIRE].timer == -1)
			sd->paramb[0]+= 4;
		if (sd->sc_data[SC_SPURT].timer != -1) {
			sd->paramb[0]+= 10;
		}
		if (sd->sc_data[SC_QUAGMIRE].timer!=-1){
			sd->paramb[1]-= sd->sc_data[SC_QUAGMIRE].val1*5;
			sd->paramb[4]-= sd->sc_data[SC_QUAGMIRE].val1*5;
		}
		if (sd->sc_data[SC_TRUESIGHT].timer!=-1){
			sd->paramb[0]+= 5;
			sd->paramb[1]+= 5;
			sd->paramb[2]+= 5;
			sd->paramb[3]+= 5;
			sd->paramb[4]+= 5;
			sd->paramb[5]+= 5;
		}

		if (sd->sc_data[SC_MARIONETTE].timer != -1) {
			sd->paramb[0]-= sd->status.str >> 1;
			sd->paramb[1]-= sd->status.agi >> 1;
			sd->paramb[2]-= sd->status.vit >> 1;
			sd->paramb[3]-= sd->status.int_>> 1;
			sd->paramb[4]-= sd->status.dex >> 1;
			sd->paramb[5]-= sd->status.luk >> 1;
		} else if (sd->sc_data[SC_MARIONETTE2].timer != -1) {
			struct map_session_data *psd = (struct map_session_data *)map_id2bl(sd->sc_data[SC_MARIONETTE2].val3);
			if (psd)	{
				sd->paramb[0] += sd->status.str + (psd->status.str >> 1) > 99 ? 99 - sd->status.str : (psd->status.str >> 1);
				sd->paramb[1] += sd->status.agi + (psd->status.agi >> 1) > 99 ? 99 - sd->status.agi : (psd->status.agi >> 1);
				sd->paramb[2] += sd->status.vit + (psd->status.vit >> 1) > 99 ? 99 - sd->status.vit : (psd->status.vit >> 1);
				sd->paramb[3] += sd->status.int_+ (psd->status.int_>> 1) > 99 ? 99 - sd->status.int_: (psd->status.int_>> 1);
				sd->paramb[4] += sd->status.dex + (psd->status.dex >> 1) > 99 ? 99 - sd->status.dex : (psd->status.dex >> 1);
				sd->paramb[5] += sd->status.luk + (psd->status.luk >> 1) > 99 ? 99 - sd->status.luk : (psd->status.luk >> 1);
			}
		}

		if (sd->sc_data[SC_BATTLEORDERS].timer != -1) {
			sd->paramb[0] += 5;
			sd->paramb[3] += 5;
			sd->paramb[4] += 5;
		}

		if (sd->state.gmaster_flag != NULL && sd->sc_data[SC_GUILDAURA].timer != -1) {
			if (sd->sc_data[SC_GUILDAURA].val4 & 1 << 0)
				sd->paramb[0] += 2;
			if (sd->sc_data[SC_GUILDAURA].val4 & 1 << 1)
				sd->paramb[2] += 2;
			if (sd->sc_data[SC_GUILDAURA].val4 & 1 << 2)
				sd->paramb[1] += 2;
			if (sd->sc_data[SC_GUILDAURA].val4 & 1 << 3)
				sd->paramb[4] += 2;
		}

		if (sd->sc_data[SC_INCREASING].timer != -1) {
			sd->paramb[1] += 4;
			sd->paramb[4] += 4;
		}

		if (sd->sc_data[SC_STRFOOD].timer != -1)
			sd->paramb[0] += sd->sc_data[SC_STRFOOD].val1;
		if (sd->sc_data[SC_AGIFOOD].timer != -1)
			sd->paramb[1] += sd->sc_data[SC_AGIFOOD].val1;
		if (sd->sc_data[SC_VITFOOD].timer != -1)
			sd->paramb[2] += sd->sc_data[SC_VITFOOD].val1;
		if (sd->sc_data[SC_INTFOOD].timer != -1)
			sd->paramb[3] += sd->sc_data[SC_INTFOOD].val1;
		if (sd->sc_data[SC_DEXFOOD].timer != -1)
			sd->paramb[4] += sd->sc_data[SC_DEXFOOD].val1;
		if (sd->sc_data[SC_LUKFOOD].timer != -1)
			sd->paramb[5] += sd->sc_data[SC_LUKFOOD].val1;
		if (sd->sc_data[SC_HITFOOD].timer != -1)
			sd->hit += sd->sc_data[SC_HITFOOD].val1;
		if (sd->sc_data[SC_FLEEFOOD].timer != -1)
			sd->flee += sd->sc_data[SC_FLEEFOOD].val1;
		if (sd->sc_data[SC_ADJUSTMENT].timer != -1) {
			sd->flee += 30;
			sd->hit -= 30;
		}
		if (sd->sc_data[SC_INCREASING].timer!=-1)
			sd->hit += 20;
		if (sd->sc_data[SC_GATLINGFEVER].timer != -1)
			sd->flee -= 5*sd->sc_data[SC_GATLINGFEVER].val1;
		if (sd->sc_data[SC_BATKFOOD].timer != -1)
			sd->base_atk += sd->sc_data[SC_BATKFOOD].val1;
		/*if(sd->sc_data[SC_WATKFOOD].timer != -1)
			sd->weapon_atk += sd->sc_data[SC_WATKFOOD].val1;*/
		if (sd->sc_data[SC_MATKFOOD].timer != -1)
			sd->matk1 += sd->sc_data[SC_MATKFOOD].val1;

		if (sd->sc_data[SC_INCALLSTATUS].timer != -1) {
			sd->paramb[0] += sd->sc_data[SC_INCALLSTATUS].val1;
			sd->paramb[1] += sd->sc_data[SC_INCALLSTATUS].val1;
			sd->paramb[2] += sd->sc_data[SC_INCALLSTATUS].val1;
			sd->paramb[3] += sd->sc_data[SC_INCALLSTATUS].val1;
			sd->paramb[4] += sd->sc_data[SC_INCALLSTATUS].val1;
			sd->paramb[5] += sd->sc_data[SC_INCALLSTATUS].val1;
		}
	}

	// Super novice special stat bonus
	if (s_class.job == 23) {
		if (sd->status.job_level >= 70 && sd->status.die_counter == 0) {
			sd->paramb[0] += 10;
			sd->paramb[1] += 10;
			sd->paramb[2] += 10;
			sd->paramb[3] += 10;
			sd->paramb[4] += 10;
			sd->paramb[5] += 10;
		}		
	}	

	// Invincible GM Status
	if (sd->sc_data[SC_INVINCIBLE].timer != -1) {
		sd->paramb[0] = 999;
		sd->paramb[1] = 999;
		sd->paramb[2] = 999;
		sd->paramb[3] = 999;
		sd->paramb[4] = 999;
		sd->paramb[5] = 999;
	}

	sd->paramc[0]=sd->status.str+sd->paramb[0]+sd->parame[0];
	sd->paramc[1]=sd->status.agi+sd->paramb[1]+sd->parame[1];
	sd->paramc[2]=sd->status.vit+sd->paramb[2]+sd->parame[2];
	sd->paramc[3]=sd->status.int_+sd->paramb[3]+sd->parame[3];
	sd->paramc[4]=sd->status.dex+sd->paramb[4]+sd->parame[4];
	sd->paramc[5]=sd->status.luk+sd->paramb[5]+sd->parame[5];

	for(i = 0; i < 6; i++) {
		if (sd->paramc[i]
			< 0) sd->paramc[i] = 0;
	}

	if (sd->sc_count) {
		if (sd->sc_data[SC_CURSE].timer != -1)
			sd->paramc[5] = 0;
	}

	// Bows, Instruments, Whips, Revolvers, Rifles, Shotguns, Gatling Guns and Grenade Launchers take base damage from DEX
	if (sd->status.weapon == 11 || sd->status.weapon == 13 || sd->status.weapon == 14 || sd->status.weapon == 17 || sd->status.weapon == 18 || sd->status.weapon == 19 || sd->status.weapon == 20 || sd->status.weapon == 21) {
		str = sd->paramc[4];
		dex = sd->paramc[0];
	// Other weapons take base damage from STR
	} else {
		str = sd->paramc[0];
		dex = sd->paramc[4];
	}

	dstr = str/10;
	sd->base_atk += str + dstr*dstr + dex/5 + sd->paramc[5]/5;
	sd->matk1 += sd->paramc[3]+(sd->paramc[3]/5)*(sd->paramc[3]/5);
	sd->matk2 += sd->paramc[3]+(sd->paramc[3]/7)*(sd->paramc[3]/7);
	if (sd->matk1 < sd->matk2) {
		int temp = sd->matk2;
		sd->matk2 = sd->matk1;
		sd->matk1 = temp;
	}
	sd->hit += sd->paramc[4] + sd->status.base_level;
	sd->flee += sd->paramc[1] + sd->status.base_level;
	sd->def2 += sd->paramc[2];
	sd->mdef2 += sd->paramc[3];
	sd->flee2 += sd->paramc[5]+10;
	sd->critical += (sd->paramc[5]*3)+10;

	if (sd->base_atk < 1)
		sd->base_atk = 1;
	if (sd->critical_rate != 100)
		sd->critical = (sd->critical*sd->critical_rate)/100;
	if (sd->critical < 10) sd->critical = 10;
	if (sd->hit_rate != 100)
		sd->hit = (sd->hit*sd->hit_rate)/100;
	if (sd->hit < 1) sd->hit = 1;
	if (sd->flee_rate != 100)
		sd->flee = (sd->flee*sd->flee_rate)/100;
	if (sd->flee < 1) sd->flee = 1;
	if (sd->flee2_rate != 100)
		sd->flee2 = (sd->flee2*sd->flee2_rate)/100;
	if (sd->flee2 < 10) sd->flee2 = 10;
	if (sd->def_rate != 100)
		sd->def = (sd->def*sd->def_rate)/100;
	if (sd->def < 0) sd->def = 0;
	if (sd->def2_rate != 100)
		sd->def2 = (sd->def2*sd->def2_rate)/100;
	if (sd->def2 < 1) sd->def2 = 1;
	if (sd->mdef_rate != 100)
		sd->mdef = (sd->mdef*sd->mdef_rate)/100;
	if (sd->mdef < 0) sd->mdef = 0;
	if (sd->mdef2_rate != 100)
		sd->mdef2 = (sd->mdef2*sd->mdef2_rate)/100;
	if (sd->mdef2 < 1) sd->mdef2 = 1;

	if (sd->status.weapon <= 22)
		if (sd->status.weapon == 15 && pc_checkskill(sd, SA_ADVANCEDBOOK) > 0)
			sd->aspd += (aspd_base[s_class.job][sd->status.weapon] - (sd->paramc[1] * 4 + sd->paramc[4] + pc_checkskill(sd, SA_ADVANCEDBOOK) * 5) * aspd_base[s_class.job][sd->status.weapon] / 1000);
		else
			sd->aspd +=  aspd_base[s_class.job][sd->status.weapon] - (sd->paramc[1] * 4 + sd->paramc[4]) * aspd_base[s_class.job][sd->status.weapon] / 1000;
	else
		sd->aspd += ((aspd_base[s_class.job][sd->weapontype1] - (sd->paramc[1] * 4 + sd->paramc[4]) * aspd_base[s_class.job][sd->weapontype1] / 1000) +
		             (aspd_base[s_class.job][sd->weapontype2] - (sd->paramc[1] * 4 + sd->paramc[4]) * aspd_base[s_class.job][sd->weapontype2] / 1000))
		            * 140 / 200;

	aspd_rate = sd->aspd_rate;

	if (sd->status.weapon >= 17 && sd->status.weapon <= 21) {
		if ((skill = pc_checkskill(sd, GS_SINGLEACTION)) > 0) {
			sd->hit += 2*skill;
			aspd_rate -= (skill + 1)*10/20;
		}
		if ((skill = pc_checkskill(sd, GS_SNAKEEYE)) > 0) {
			sd->hit += skill;
			sd->attackrange += skill;
		}
	}

	if ((skill = pc_checkskill(sd, AC_VULTURE)) > 0) {
		sd->hit += skill;
		if (sd->status.weapon == 11)
			sd->attackrange += skill;
	}

	if ((skill = pc_checkskill(sd, BS_WEAPONRESEARCH)) > 0)
		sd->hit += skill << 1;
	if (pc_isriding(sd) && pc_checkskill(sd, KN_RIDING) > 0)
		sd->max_weight += 10000;


	if ((skill = pc_checkskill(sd, BS_SKINTEMPER)) > 0) {
		sd->subele[0] += skill;
		sd->subele[3] += skill * 5;
	}
	if ((skill = pc_checkskill(sd, SA_ADVANCEDBOOK)) > 0)
		aspd_rate -= skill >> 1;
		
	if ((skill= pc_checkskill(sd, SG_DEVIL))) {
		clif_status_change(&sd->bl,ICO_DEVIL,1);
		if (sd->status.job_level >= 50)
			aspd_rate -= 30*skill;
	}

	bl = sd->status.base_level;
	idx = (3500 + bl * hp_coefficient2[s_class.job] + hp_sigma_val[s_class.job][(bl > 0)? bl-1:0])/100 * (100 + sd->paramc[2])/100 + (sd->parame[2] - sd->paramcard[2]);
	if (s_class.upper == 1)
		idx = idx * 130 / 100;
	else if (s_class.upper == 2)
		idx = idx * 70 / 100;

	if ((skill = pc_checkskill(sd, CR_TRUST)) > 0) {
		sd->status.max_hp += skill * 200; // Passive skill, max HP modifier
		sd->subele[6] += skill * 5;
	}	

	sd->status.max_hp += idx;

	if (sd->hprate != 100)
		sd->status.max_hp = sd->status.max_hp * sd->hprate / 100;

	if (sd->status.class == JOB_TAEKWON && pc_checkskill(sd, TK_MISSION) && ranking_id2rank(sd->status.char_id, RK_TAEKWON) && sd->status.base_level >= 90)
		sd->status.max_hp *= 3;

	if (sd->sc_count && sd->sc_data[SC_BERSERK].timer != -1) {
		sd->status.max_hp = sd->status.max_hp * 3;
		if (sd->status.max_hp > battle_config.max_hp)
			sd->status.max_hp = battle_config.max_hp;
		if (sd->status.hp > battle_config.max_hp)
			sd->status.hp = battle_config.max_hp;
	}
	if (s_class.job == 23 && sd->status.base_level >= 99) {
		sd->status.max_hp = sd->status.max_hp + 2000;
	}

	if (sd->status.max_hp > battle_config.max_hp)
		sd->status.max_hp = battle_config.max_hp;
	if (sd->status.max_hp <= 0)
		sd->status.max_hp = 1;

	idx = ((sp_coefficient[s_class.job] * bl) + 1000)/100 * (100 + sd->paramc[3])/100 + (sd->parame[3] - sd->paramcard[3]);
	if (s_class.upper == 1)
		idx = idx * 130 / 100;
	else if (s_class.upper == 2)
		idx = idx * 70 / 100;

	sd->status.max_sp += idx;

	if ((skill=pc_checkskill(sd,HP_MEDITATIO)) > 0) // Passive skill, max SP modifier
		sd->status.max_sp += sd->status.max_sp * skill / 100;
	if ((skill=pc_checkskill(sd,HW_SOULDRAIN))>0) // Passive skill, max SP modifier
		sd->status.max_sp += sd->status.max_sp * 2 * skill / 100;

	if (sd->sprate!=100)
		sd->status.max_sp = sd->status.max_sp * sd->sprate / 100;

	if ((skill = pc_checkskill(sd,SL_KAINA)) > 0)
		sd->status.max_sp += 30 * skill;

	// x3 SP for top 10 level 90+ ranking Taekwons
	if (sd->status.class == JOB_TAEKWON && pc_checkskill(sd, TK_MISSION) && ranking_id2rank(sd->status.char_id, RK_TAEKWON) && sd->status.base_level >= 90)
		sd->status.max_sp *= 3;

	if (sd->status.max_sp > battle_config.max_sp)
		sd->status.max_sp = battle_config.max_sp;
	if (sd->status.max_sp <= 0)
		sd->status.max_sp = 1;

	sd->nhealhp = 1 + (sd->paramc[2]/5) + (sd->status.max_hp/200);
	if ((skill=pc_checkskill(sd,SM_RECOVERY)) > 0) {
		sd->nshealhp = skill*5 + (sd->status.max_hp*skill/500);
		if (sd->nshealhp > 0x7fff) sd->nshealhp = 0x7fff;
	}
	sd->nhealsp = 1 + (sd->paramc[3]/6) + (sd->status.max_sp/100);
	if (sd->paramc[3] >= 120)
		sd->nhealsp += ((sd->paramc[3] - 120) >> 1) + 4;
	if ((skill=pc_checkskill(sd,MG_SRECOVERY)) > 0) {
		sd->nshealsp = skill*3 + (sd->status.max_sp*skill/500);
		if (sd->nshealsp > 0x7fff) sd->nshealsp = 0x7fff;
	}
	if ((skill=pc_checkskill(sd,NJ_NINPOU)) > 0) {
		sd->nshealsp = skill*3 + (sd->status.max_sp*skill/500);
		if (sd->nshealsp > 0x7fff) sd->nshealsp = 0x7fff;
	}

	if ((skill = pc_checkskill(sd,MO_SPIRITSRECOVERY)) > 0) {
		sd->nsshealhp += skill*4 + (sd->status.max_hp*skill/500);
		sd->nsshealsp += skill*2 + (sd->status.max_sp*skill/500);
		if (sd->nsshealhp > 0x7fff) sd->nsshealhp = 0x7fff;
		if (sd->nsshealsp > 0x7fff) sd->nsshealsp = 0x7fff;
	}

	if ((skill=pc_checkskill(sd,TK_HPTIME)) > 0 && sd->state.rest) {
		sd->nsshealhp += skill*30 + (sd->status.max_hp*skill/500);
		if (sd->nsshealhp > 0x7fff) sd->nsshealhp = 0x7fff;
	}

	if ((skill=pc_checkskill(sd,TK_SPTIME)) > 0 && sd->state.rest) {
		sd->nsshealsp += skill*3 + (sd->status.max_sp*skill/500);
		if ((skill = pc_checkskill(sd,SL_KAINA)) > 0)
			sd->nsshealsp += sd->nsshealsp * (30 + skill * 10) / 100;
		if (sd->nsshealsp > 0x7fff) sd->nsshealsp = 0x7fff;
	}

	if (sd->hprecov_rate != 100) {
		sd->nhealhp = sd->nhealhp*sd->hprecov_rate/100;
		if (sd->nhealhp < 1) sd->nhealhp = 1;
	}
	if (sd->sprecov_rate != 100) {
		sd->nhealsp = sd->nhealsp*sd->sprecov_rate/100;
		if (sd->nhealsp < 1) sd->nhealsp = 1;
	}

	if ((skill=pc_checkskill(sd,SA_DRAGONOLOGY))>0) {
		skill = skill*4;
		sd->addrace[9]+=skill;
		sd->addrace_[9]+=skill;
		sd->subrace[9]+=skill;
		sd->magic_addrace[9]+=skill;
		sd->magic_subrace[9]-=skill;
	}

	// Flee calculation
	if ((skill = pc_checkskill(sd, TF_MISS)) > 0)
		sd->flee += skill * ((s_class.job == JOB_ASSASSIN || s_class.job == JOB_ROGUE)? 4 : 3);

	if ((skill = pc_checkskill(sd, MO_DODGE)) > 0)
		sd->flee += (skill * 3) >> 1;

	if (map[sd->bl.m].flag.gvg) // GvG map flee penalty
		sd->flee -= sd->flee * battle_config.gvg_flee_penalty / 100;

	// Status change calculations
	if (sd->sc_count) {
		if (sd->sc_data[SC_ANGELUS].timer!=-1)
			sd->def2 = sd->def2*(110+5*sd->sc_data[SC_ANGELUS].val1)/100;

		if (sd->sc_data[SC_IMPOSITIO].timer!=-1)	{
			sd->watk += sd->sc_data[SC_IMPOSITIO].val1*5;
			idx = sd->equip_index[8];
			if (idx >= 0 && sd->inventory_data[idx] && sd->inventory_data[idx]->type == 4)
				sd->watk_ += sd->sc_data[SC_IMPOSITIO].val1*5;
		}

		if (sd->sc_data[SC_PROVOKE].timer!=-1) {
			// Provoke should only reduce the vit def of a player, not vit def and armor def [Bison]
			// Corrected the formula according to kRO site, as provoke does 10% reduction at level 1 and 55% at level 10 [Bison]
			sd->def2 = sd->def2*(100 - (5 * sd->sc_data[SC_PROVOKE].val1 + 5))/100;
			// Corrected the +atk% formula from kRO site, provoke 1 does +5% atk increase, and +3% per level, so provoke 10 = +32% [Bison]
			sd->base_atk = sd->base_atk*(100 + (3 * sd->sc_data[SC_PROVOKE].val1 + 2))/100;
			sd->watk = sd->watk*(100 + (3 * sd->sc_data[SC_PROVOKE].val1 + 2))/100;

			idx = sd->equip_index[8];
			if (idx >= 0 && sd->inventory_data[idx] && sd->inventory_data[idx]->type == 4)
				sd->watk_ = sd->watk_*(100 + (3 * sd->sc_data[SC_PROVOKE].val1 + 2))/100;
		}
		if (sd && sd->sc_data[SC_FUSION].timer!=-1) {
			aspd_rate -= 20;
			sd->perfect_hit += 100;
		}

		if (sd->sc_data[SC_GATLINGFEVER].timer != -1) {
			sd->base_atk += 10*sd->sc_data[SC_GATLINGFEVER].val1 + 20;
			aspd_rate -= 2*sd->sc_data[SC_GATLINGFEVER].val1;
		}

		if (sd->sc_data[SC_MADNESSCANCEL].timer != -1)
			sd->base_atk += 100;

		if (sd->sc_data[SC_ENDURE].timer!=-1)
			sd->mdef += sd->sc_data[SC_ENDURE].val1;

		if (sd->sc_data[SC_MINDBREAKER].timer!=-1) {
			sd->mdef2 = sd->mdef2 * (sd->sc_data[SC_MINDBREAKER].val2) / 100;
			sd->matk1 = sd->matk1 * (sd->sc_data[SC_MINDBREAKER].val3) / 100;
			sd->matk2 = sd->matk2 * (sd->sc_data[SC_MINDBREAKER].val3) / 100;
		}

		if (sd->sc_data[SC_POISON].timer != -1)
			sd->def2 = sd->def2 * 75 / 100;

		if (sd->sc_data[SC_CURSE].timer != -1) {
			sd->base_atk = sd->base_atk * 75 / 100;
			sd->watk = sd->watk * 75 / 100;
			idx = sd->equip_index[8];
			if (idx >= 0 && sd->inventory_data[idx] && sd->inventory_data[idx]->type == 4)
				sd->watk_ = sd->watk_ * 75 / 100;
		}

		if (sd->sc_data[SC_BLEEDING].timer != -1) {
			sd->base_atk -= sd->base_atk >> 2;
			sd->aspd_rate += 25;
		}

		if (sd->sc_data[SC_DRUMBATTLE].timer != -1) {
			sd->watk += sd->sc_data[SC_DRUMBATTLE].val2;
			sd->def  += sd->sc_data[SC_DRUMBATTLE].val3;
			idx = sd->equip_index[8];
			if (idx >= 0 && sd->inventory_data[idx] && sd->inventory_data[idx]->type == 4)
				sd->watk_ += sd->sc_data[SC_DRUMBATTLE].val2;
		}

		if (sd->sc_data[SC_NIBELUNGEN].timer != -1) {
			idx = sd->equip_index[9];
			if (idx >= 0 && sd->inventory_data[idx] && sd->inventory_data[idx]->wlv == 4)
				sd->watk2 += sd->sc_data[SC_NIBELUNGEN].val3;
			idx = sd->equip_index[8];
			if (idx >= 0 && sd->inventory_data[idx] && sd->inventory_data[idx]->wlv == 4)
				sd->watk_2 += sd->sc_data[SC_NIBELUNGEN].val3;
		}

		if (sd->sc_data[SC_VOLCANO].timer !=- 1 && sd->def_ele == 3)
			sd->watk += sd->sc_data[SC_VIOLENTGALE].val3;

		if (sd->sc_data[SC_SIGNUMCRUCIS].timer != -1)
			sd->def = sd->def * (100 - sd->sc_data[SC_SIGNUMCRUCIS].val2)/100;

		if (sd->sc_data[SC_ETERNALCHAOS].timer != -1) {
			sd->def = 0;
			sd->def2 = 0;
		}

		if (sd->sc_data[SC_CONCENTRATION].timer!=-1) {
			sd->base_atk = sd->base_atk * (100 + 5 * sd->sc_data[SC_CONCENTRATION].val1) / 100;
			sd->watk = sd->watk * (100 + 5 * sd->sc_data[SC_CONCENTRATION].val1) / 100;
			sd->watk2 = sd->watk2 * (100 + 5 * sd->sc_data[SC_CONCENTRATION].val1) / 100;
			sd->def = sd->def * (100 - 5 * sd->sc_data[SC_CONCENTRATION].val1) / 100;
			sd->def2 = sd->def2 * (100 - 5 * sd->sc_data[SC_CONCENTRATION].val1) / 100;
		}

		if (sd->sc_data[SC_MAGICPOWER].timer!=-1){
			sd->matk1 = sd->matk1*(100+5*sd->sc_data[SC_MAGICPOWER].val1)/100;
			sd->matk2 = sd->matk2*(100+5*sd->sc_data[SC_MAGICPOWER].val1)/100;
		}

		if (sd->sc_data[SC_ATKPOT].timer!=-1)
			sd->watk += sd->sc_data[SC_ATKPOT].val1;

		if (sd->sc_data[SC_MATKPOT].timer!=-1){
			sd->matk1 += sd->sc_data[SC_MATKPOT].val1;
			sd->matk2 += sd->sc_data[SC_MATKPOT].val1;
		}

		// ASPD Calculation
		if ((sd->sc_data[SC_TWOHANDQUICKEN].timer != -1 || sd->sc_data[SC_ONEHAND].timer != -1) && sd->sc_data[SC_QUAGMIRE].timer == -1 && sd->sc_data[SC_DONTFORGETME].timer == -1)
			aspd_rate -= 30;

		if (sd->sc_data[SC_ADRENALINE].timer != -1 && sd->sc_data[SC_TWOHANDQUICKEN].timer == -1 && sd->sc_data[SC_ONEHAND].timer == -1 &&
			sd->sc_data[SC_QUAGMIRE].timer == -1 && sd->sc_data[SC_DONTFORGETME].timer == -1) {
			if (sd->sc_data[SC_ADRENALINE].val2 || !battle_config.party_skill_penalty)
				aspd_rate -= 30;
			else
				aspd_rate -= 25;
		}

		if (sd->sc_data[SC_SPEARQUICKEN].timer != -1 && sd->sc_data[SC_ADRENALINE].timer == -1 &&
			sd->sc_data[SC_TWOHANDQUICKEN].timer == -1 && sd->sc_data[SC_ONEHAND].timer == -1 && 
			sd->sc_data[SC_QUAGMIRE].timer == -1 && sd->sc_data[SC_DONTFORGETME].timer == -1)
			aspd_rate -= sd->sc_data[SC_SPEARQUICKEN].val2;

		if (sd->sc_data[SC_ASSNCROS].timer!=-1 && 
			sd->sc_data[SC_TWOHANDQUICKEN].timer == -1 && sd->sc_data[SC_ONEHAND].timer == -1 && sd->sc_data[SC_ADRENALINE].timer == -1 && sd->sc_data[SC_SPEARQUICKEN].timer == -1 &&
			sd->sc_data[SC_DONTFORGETME].timer == -1)
				aspd_rate -= 5+sd->sc_data[SC_ASSNCROS].val1+sd->sc_data[SC_ASSNCROS].val2+sd->sc_data[SC_ASSNCROS].val3;

		if (sd->sc_data[SC_GRAVITATION].timer != -1)
			aspd_rate += sd->sc_data[SC_GRAVITATION].val2;

		if (sd->sc_data[SC_DONTFORGETME].timer!=-1)
			aspd_rate += sd->sc_data[SC_DONTFORGETME].val1 * 3 + sd->sc_data[SC_DONTFORGETME].val2 + (sd->sc_data[SC_DONTFORGETME].val3 >> 16);

		if (sd->sc_data[SC_MADNESSCANCEL].timer != -1 && sd->sc_data[SC_BERSERK].timer == -1)
			aspd_rate -= 20;

		if (sd->sc_data[i=SC_ASPDPOTION3].timer != -1 ||
		    sd->sc_data[i=SC_ASPDPOTION2].timer != -1 ||
		    sd->sc_data[i=SC_ASPDPOTION1].timer != -1 ||
		    sd->sc_data[i=SC_ASPDPOTION0].timer != -1)
			aspd_rate -= sd->sc_data[i].val2;

		// Hit/Flee calculation
		if (sd->sc_data[SC_WHISTLE].timer != -1) {
			sd->flee += sd->flee * (sd->sc_data[SC_WHISTLE].val1
			          + sd->sc_data[SC_WHISTLE].val2 + (sd->sc_data[SC_WHISTLE].val3 >> 16)) / 100;
			sd->flee2+= (sd->sc_data[SC_WHISTLE].val1+sd->sc_data[SC_WHISTLE].val2+(sd->sc_data[SC_WHISTLE].val3&0xffff)) * 10;
		}

		if (sd->sc_data[SC_HUMMING].timer != -1)
			sd->hit += (sd->sc_data[SC_HUMMING].val1*2+sd->sc_data[SC_HUMMING].val2
					+sd->sc_data[SC_HUMMING].val3) * sd->hit/100;

		if (sd->sc_data[SC_VIOLENTGALE].timer != -1 && sd->def_ele == 4)
			sd->flee += sd->flee*sd->sc_data[SC_VIOLENTGALE].val3/100;

		if (sd->sc_data[SC_BLIND].timer != -1) {
			sd->hit -= sd->hit >> 2;
			sd->flee -= sd->flee >> 2;
		}

		if (sd->sc_data[SC_WINDWALK].timer != -1)
			sd->flee += sd->flee * (sd->sc_data[SC_WINDWALK].val2) / 100;

		if (sd->sc_data[SC_SPIDERWEB].timer != -1)
			sd->flee -= sd->flee * 50 / 100;

		if (sd->sc_data[SC_TRUESIGHT].timer != -1)
			sd->hit += sd->hit * 3 * (sd->sc_data[SC_TRUESIGHT].val1) / 100;

		if (sd->sc_data[SC_CONCENTRATION].timer != -1)
			sd->hit += 10 * sd->sc_data[SC_CONCENTRATION].val1;

		if (sd->sc_data[SC_SIEGFRIED].timer != -1){
			sd->subele[1] += sd->sc_data[SC_SIEGFRIED].val2;
			sd->subele[2] += sd->sc_data[SC_SIEGFRIED].val2;
			sd->subele[3] += sd->sc_data[SC_SIEGFRIED].val2;
			sd->subele[4] += sd->sc_data[SC_SIEGFRIED].val2;
			sd->subele[5] += sd->sc_data[SC_SIEGFRIED].val2;
			sd->subele[6] += sd->sc_data[SC_SIEGFRIED].val2;
			sd->subele[7] += sd->sc_data[SC_SIEGFRIED].val2;
			sd->subele[8] += sd->sc_data[SC_SIEGFRIED].val2;
			sd->subele[9] += sd->sc_data[SC_SIEGFRIED].val2;
		}

		if (sd->sc_data[SC_PROVIDENCE].timer != -1) {
			sd->subele[6] += sd->sc_data[SC_PROVIDENCE].val2;
			sd->subrace[6] += sd->sc_data[SC_PROVIDENCE].val2;
		}

		// Maximum HP bonus calculations
		if (sd->sc_data[SC_APPLEIDUN].timer != -1) {
			sd->status.max_hp += sd->status.max_hp * (sd->sc_data[SC_APPLEIDUN].val1 / 100);
			if (sd->status.max_hp < 0 || sd->status.max_hp > battle_config.max_hp)
				sd->status.max_hp = battle_config.max_hp;
		}

		if (sd->sc_data[SC_DELUGE].timer!=-1 && sd->def_ele == 1) {
			sd->status.max_hp += sd->status.max_hp*sd->sc_data[SC_DELUGE].val3/100;
			if (sd->status.max_hp < 0 || sd->status.max_hp > battle_config.max_hp)
				sd->status.max_hp = battle_config.max_hp;
		}

		if (sd->sc_data[SC_SERVICEFORYOU].timer!=-1) {
			sd->status.max_sp += sd->status.max_sp*(10+sd->sc_data[SC_SERVICEFORYOU].val1+sd->sc_data[SC_SERVICEFORYOU].val2
						+sd->sc_data[SC_SERVICEFORYOU].val3)/100;
			if (sd->status.max_sp < 0 || sd->status.max_sp > battle_config.max_sp)
				sd->status.max_sp = battle_config.max_sp;
			sd->dsprate-=(10+sd->sc_data[SC_SERVICEFORYOU].val1*3+sd->sc_data[SC_SERVICEFORYOU].val2
					+sd->sc_data[SC_SERVICEFORYOU].val3);
			if (sd->dsprate<0)
				sd->dsprate=0;
		}

		if (sd->sc_data[SC_FORTUNE].timer != -1)
			sd->critical += (10 + sd->sc_data[SC_FORTUNE].val1 + sd->sc_data[SC_FORTUNE].val2
											+ sd->sc_data[SC_FORTUNE].val3) * 10;

		if (sd->sc_data[SC_EXPLOSIONSPIRITS].timer!=-1){
			if (s_class.job == JOB_SUPER_NOVICE)
				sd->critical += sd->sc_data[SC_EXPLOSIONSPIRITS].val1*100;
			else
			sd->critical += sd->sc_data[SC_EXPLOSIONSPIRITS].val2;
		}

		if (sd->sc_data[SC_STEELBODY].timer != -1) {
			sd->def = 90;
			sd->mdef = 90;
			aspd_rate += 25;
		}

		if (sd->sc_data[SC_DEFENDER].timer != -1)
			sd->aspd += (250 - sd->sc_data[SC_DEFENDER].val1 * 50);

		if (sd->sc_data[SC_ENCPOISON].timer != -1)
			sd->addeff[4] += sd->sc_data[SC_ENCPOISON].val2;

		if (sd->sc_data[SC_DANCING].timer != -1) {
			int s_rate = 500 - 40 * pc_checkskill(sd, (sd->status.sex? BA_MUSICALLESSON : DC_DANCINGLESSON));
			if (sd->sc_data[SC_LONGING].timer != -1)
				s_rate -= 20 * sd->sc_data[SC_LONGING].val1;
			sd->nhealsp = 0;
			sd->nshealsp = 0;
			sd->nsshealsp = 0;
		}

		if (sd->sc_data[SC_TRUESIGHT].timer != -1)
			sd->critical += 10 * (sd->sc_data[SC_TRUESIGHT].val1);

		if (sd->sc_data[SC_BERSERK].timer != -1) {	// All Def/Mdef reduced to 0 while in Berserk
			sd->def = sd->def2 = 0;
			sd->mdef = sd->mdef2 = 0;
			sd->flee -= sd->flee * 50/100;
			aspd_rate -= 30;
		}

		if (sd->sc_data[SC_KEEPING].timer != -1)
			sd->def = 100;

		if (sd->sc_data[SC_BARRIER].timer != -1)
			sd->mdef = 100;

		// Random break
		if (sd->sc_data[SC_JOINTBEAT].timer != -1) {
			switch(sd->sc_data[SC_JOINTBEAT].val2) {
			case 1: // Ankle break
				break;
			case 2: // Wrist break
				sd->aspd_rate += 25;
				break;
			case 3: // Knee break
				sd->aspd_rate += 10;
				break;
			case 4: // Shoulder break
				sd->def -= sd->def * 50 / 100;
				sd->def2 -= sd->def2 * 50 / 100;
				break;
			case 5: // Waist break
				sd->def -= sd->def * 25 / 100;
				sd->def2 -= sd->def2 * 25 / 100;
				sd->base_atk -= sd->base_atk * 25 / 100;
				break;
			}
		}

		if (sd->sc_data[SC_INCHIT].timer != -1)
			sd->hit += sd->sc_data[SC_INCHIT].val1;

		if (sd->sc_data[SC_INCFLEE].timer != -1)
			sd->flee += sd->sc_data[SC_INCFLEE].val1;

		if (sd->sc_data[SC_INCMHPRATE].timer != -1) {
			sd->status.max_hp += sd->status.max_hp * sd->sc_data[SC_INCMHPRATE].val1 / 100;
			if (sd->status.max_hp < 0 || sd->status.max_hp > battle_config.max_hp)
				sd->status.max_hp = battle_config.max_hp;
		}

		if (sd->sc_data[SC_INCMSPRATE].timer != -1) {
			sd->status.max_sp += sd->status.max_sp * sd->sc_data[SC_INCMSPRATE].val1 / 100;
			if (sd->status.max_sp < 0 || sd->status.max_sp > battle_config.max_sp)
				sd->status.max_sp = battle_config.max_sp;
		}

		if (sd->sc_data[SC_INCMATKRATE].timer != -1) {
			sd->matk1 = sd->matk1 * (100 + sd->sc_data[SC_INCMATKRATE].val1) /100;
			sd->matk2 = sd->matk2 * (100 + sd->sc_data[SC_INCMATKRATE].val1) /100;
		}

		if (sd->sc_data[SC_INCATKRATE].timer != -1) {
			sd->watk = sd->watk * (100 + sd->sc_data[SC_INCATKRATE].val1) / 100;
			sd->watk2 = sd->watk2 * (100 + sd->sc_data[SC_INCATKRATE].val1) / 100;
		}

		if (sd->sc_data[SC_INCASPDRATE].timer != -1)
			sd->aspd_rate += sd->sc_data[SC_INCASPDRATE].val1;
	}
	// End of status change calculation

	if ((skill = pc_checkskill(sd,HP_MANARECHARGE)) > 0) {
		sd->dsprate -= 4 * skill;
		if (sd->dsprate < 0) sd->dsprate = 0;
	}

	// Matk relative modifiers from equipment
	if (sd->matk_rate != 100) {
		sd->matk1 = sd->matk1 * sd->matk_rate / 100;
		sd->matk2 = sd->matk2 * sd->matk_rate / 100;
	}

	// Speed Calculation
	status_calc_speed(&sd->bl);

	if (aspd_rate != 100)
		sd->aspd = sd->aspd*aspd_rate/100;

	if (pc_isriding(sd))
		sd->aspd = sd->aspd*(100 + 10*(5 - pc_checkskill(sd,KN_CAVALIERMASTERY)))/ 100;

	if (sd->aspd < battle_config.max_aspd) sd->aspd = battle_config.max_aspd;
	sd->amotion = sd->aspd;
	sd->dmotion = 800-sd->paramc[1]*4;

	if (sd->dmotion < 400)
		sd->dmotion = 400;

	if (sd->status.hp > sd->status.max_hp)
		sd->status.hp = sd->status.max_hp;

	if (sd->status.sp > sd->status.max_sp)
		sd->status.sp = sd->status.max_sp;

	// Invincible GM Status
	// Bypasses normal maximum values
	if (sd->sc_data[SC_INVINCIBLE].timer != -1) {
		sd->status.hp = 999999;
		sd->status.sp = 999999;
		sd->status.max_hp = 999999;
		sd->status.max_sp = 999999;
		sd->speed = 50;
		sd->base_atk = 99999;
		sd->watk = 99999;
		sd->watk2 = 99999;
		sd->matk1 = 99999;
		sd->matk2 = 99999;
		sd->def = 99;
		sd->def2 = 99;
		sd->mdef = 99;
		sd->mdef2 = 99;
		sd->hit = 999;
		sd->flee = 999;
		sd->critical = 999;
		clif_updatestatus(sd, SP_SPEED);
		status_change_clear_debuffs(&sd->bl);
	}

	// Refresh client display ->

	if (first & 4)
		return 0;

	if (first & 3) {
		clif_updatestatus(sd, SP_MAXHP);
		clif_updatestatus(sd, SP_MAXSP);

		if (first & 1) {
			clif_updatestatus(sd, SP_HP);
			clif_updatestatus(sd, SP_SP);
		}
		return 0;
	}

	if (b_class != sd->view_class) {
		clif_changelook(&sd->bl, LOOK_BASE, sd->view_class);
		clif_changelook(&sd->bl, LOOK_WEAPON, 0);
	}

	if (memcmp(b_skill, sd->status.skill,sizeof(sd->status.skill)) || b_attackrange != sd->attackrange)
		clif_skillinfoblock(sd);

	if (b_weight != sd->weight)
		clif_updatestatus(sd,SP_WEIGHT);

	if (b_max_weight != sd->max_weight) {
		clif_updatestatus(sd,SP_MAXWEIGHT);
		pc_checkweighticon(sd);
	}

	for(i=0;i<6;i++)
		if (b_paramb[i] + b_parame[i] != sd->paramb[i] + sd->parame[i])
			clif_updatestatus(sd,SP_STR+i);

	if (b_hit != sd->hit)
		clif_updatestatus(sd,SP_HIT);

	if (b_flee != sd->flee)
		clif_updatestatus(sd,SP_FLEE1);

	if (b_aspd != sd->aspd)
		clif_updatestatus(sd,SP_ASPD);

	if (b_watk != sd->watk || b_base_atk != sd->base_atk)
		clif_updatestatus(sd,SP_ATK1);

	if (b_def != sd->def)
		clif_updatestatus(sd,SP_DEF1);

	if (b_watk2 != sd->watk2)
		clif_updatestatus(sd,SP_ATK2);

	if (b_def2 != sd->def2)
		clif_updatestatus(sd,SP_DEF2);

	if (b_flee2 != sd->flee2)
		clif_updatestatus(sd,SP_FLEE2);

	if (b_critical != sd->critical)
		clif_updatestatus(sd,SP_CRITICAL);

	if (b_matk1 != sd->matk1)
		clif_updatestatus(sd,SP_MATK1);

	if (b_matk2 != sd->matk2)
		clif_updatestatus(sd,SP_MATK2);

	if (b_mdef != sd->mdef)
		clif_updatestatus(sd,SP_MDEF1);

	if (b_mdef2 != sd->mdef2)
		clif_updatestatus(sd,SP_MDEF2);

	if (b_attackrange != sd->attackrange)
		clif_updatestatus(sd,SP_ATTACKRANGE);

	if (b_max_hp != sd->status.max_hp)
		clif_updatestatus(sd,SP_MAXHP);

	if (b_max_sp != sd->status.max_sp)
		clif_updatestatus(sd,SP_MAXSP);

	if (b_hp != sd->status.hp)
		clif_updatestatus(sd,SP_HP);

	if (b_sp != sd->status.sp)
		clif_updatestatus(sd,SP_SP);

	// <- End refresh client display

	if (sd->status.hp < sd->status.max_hp>>2 && sd->sc_data[SC_AUTOBERSERK].timer != -1 &&
		(sd->sc_data[SC_PROVOKE].timer == -1 || sd->sc_data[SC_PROVOKE].val2 == 0 ) && !pc_isdead(sd))
		status_change_start(&sd->bl,SC_PROVOKE,10,1,0,0,0,0);

	return 0;
}

/*==========================================
 * Speed Calculation Function
 * Adjusts speed for mobs/pets/players/NPCs
 * As well as recalculates for mobs/players
 *------------------------------------------
 */
void status_calc_speed(struct block_list *bl) {

	int speed, speed_rate, speed_add_rate, skill;
	struct map_session_data *sd = NULL;
	struct mob_data *md = NULL;
	struct pet_data *pd = NULL;
	struct npc_data *nd = NULL;

	nullpo_retv(bl);

	// Speed calculation for players
	if (bl->type == BL_PC) {
		sd = (struct map_session_data*)bl;
		speed = DEFAULT_WALK_SPEED; // Reset speed (Normally 150)
		speed_rate = sd->speed_rate; // Normally 100
		speed_add_rate = sd->speed_add_rate; // Normally 100
	}
	// Speed calculation for monsters
	else if (bl->type == BL_MOB) {
		md = (struct mob_data*)bl;
		speed = mob_db[md->class].speed; // Reset speed
		speed_rate = 100;
		speed_add_rate = 100;
	}
	// Speed return for pets
	else if (bl->type == BL_PET) {
		pd = (struct pet_data*)bl;
		pd->speed = ((struct pet_data *)bl)->msd->petDB->speed;
		return; // No need to do calculation, return now
	}
	// Speed return for NPCs
	else if (bl->type == BL_NPC) {
		nd = (struct npc_data*)bl;
		nd->speed = DEFAULT_WALK_SPEED;
		return; // No need to do calculation, return now
	}
	// If not player/mob/pet/NPC, return
	else {
		return;
	}

	// Get status change data
	struct status_change* sc_data;
	sc_data = status_get_sc_data(bl);
	short *sc_count;
	sc_count = status_get_sc_count(bl);

	if (speed_add_rate != 100)
		speed_rate += speed_add_rate - 100;

	if (sc_count) {
		// Fixed reductions
		if (sc_data[SC_CURSE].timer != -1)	
			speed += 450;
		if (sc_data[SC_SWOO].timer != -1)
			speed += 450; // Official value unknown
		if (sc_data[SC_WEDDING].timer != -1)
			speed += 300;

		// Percent increases, most don't stack
		if (sc_data[SC_GATLINGFEVER].timer == -1) {
			if (sc_data[SC_SPEEDUP1].timer != -1)
				speed -= speed * 50/100;
			else if (sc_data[SC_AVOID].timer != -1)
				speed -= speed * sc_data[SC_AVOID].val2 / 100;

			if (sc_data[SC_RUN].timer != -1)
				speed -= speed * 50/100;
			else if (sc_data[SC_SPEEDUP0].timer != -1)
				speed -= speed * 25/100;
			else if (sc_data[SC_INCREASEAGI].timer != -1)
				speed -= speed * 25/100;
			else if (sc_data[SC_FUSION].timer != -1)
				speed -= speed * 25/100;
			else if (sc_data[SC_CARTBOOST].timer != -1)
				speed -= speed * 20/100;
			else if (sc_data[SC_BERSERK].timer != -1)
				speed -= speed * 20/100;
			else if (sc_data[SC_WINDWALK].timer != -1)
				speed -= speed *(sc_data[SC_WINDWALK].val1*2)/100;
		}
		// Stackable percent reductions
		if (sd && sc_data[SC_DANCING].timer != -1) {
			int s_rate = 500 - 40 * pc_checkskill(sd, (sd->status.sex? BA_MUSICALLESSON: DC_DANCINGLESSON));
			if (sc_data[SC_LONGING].timer != -1)
				s_rate -= 20 * sc_data[SC_LONGING].val1;
			if (sc_data[SC_SPIRIT].timer != -1 && sc_data[SC_SPIRIT].val2 == SL_BARDDANCER)
				speed -= 40; // Custom rate
			else
				speed += speed * s_rate / 100;
		}
		if (sc_data[SC_DECREASEAGI].timer != -1)
			speed = speed * 100/75;
		if (sc_data[SC_STEELBODY].timer != -1)
			speed = speed * 100/75;
		if (sc_data[SC_QUAGMIRE].timer != -1)
			speed = speed * 100/50;
		if (sc_data[SC_SUITON].timer != -1 && sc_data[SC_SUITON].val3)
			speed = speed * 100/sc_data[SC_SUITON].val3;
		if (sc_data[SC_DONTFORGETME].timer != -1)
			speed = speed * (100 + sc_data[SC_DONTFORGETME].val1 * 2 + sc_data[SC_DONTFORGETME].val2 + (sc_data[SC_DONTFORGETME].val3 & 0xffff)) / 100;
		if (sc_data[SC_DEFENDER].timer != -1)
			speed += speed * (55 - 5 * sc_data[SC_DEFENDER].val1) / 100;
		if (sc_data[SC_GOSPEL].timer != -1 && sc_data[SC_GOSPEL].val4 == BCT_ENEMY)
			speed = speed * 100/75;
		if (sc_data[SC_JOINTBEAT].timer != -1) {
			if (sc_data[SC_JOINTBEAT].val2 == 1)
				speed = speed * 150 / 100;
			else if (sc_data[SC_JOINTBEAT].val2 == 3)
				speed = speed * 130 / 100;
		}
		if (sc_data[SC_CLOAKING].timer != -1)
			speed = speed * (sc_data[SC_CLOAKING].val3 - sc_data[SC_CLOAKING].val1 * 3) /100;

		if (sc_data[SC_CHASEWALK].timer != -1) {
			speed = speed * sc_data[SC_CHASEWALK].val3 / 100;
			if (sc_data[SC_SPIRIT].timer != -1 && sc_data[SC_SPIRIT].val2 == SL_ROGUE)
				speed -= speed >> 2;
		}
		if (sc_data[SC_GATLINGFEVER].timer != -1)
			speed = speed * 100/75;
		if (sc_data[SC_SLOWDOWN].timer != -1)
			speed = speed * 100/75;
	}

	if (sd) {
		if (sd->status.option&2 && (skill = pc_checkskill(sd, RG_TUNNELDRIVE)) > 0)
			speed += speed * (100 - 16 * skill) / 100;

		if (pc_iscarton(sd) && (skill = pc_checkskill(sd, MC_PUSHCART)) > 0)
			speed += speed * (100 - 10 * skill) / 100;

		else if (pc_isriding(sd) && pc_checkskill(sd, KN_RIDING) > 0)
			speed -= speed >> 2;

		else if ((sd->status.class == JOB_ASSASSIN || sd->status.class == JOB_BABY_ASSASSIN || sd->status.class == JOB_ASSASSIN_CROSS) && (skill = pc_checkskill(sd, TF_MISS)) > 0)
			speed -= speed * skill / 100;
	}

	if (speed_rate <= 0)
		speed_rate = 1;

	if (speed_rate != 100)
		speed = speed * speed_rate / 100;

	// Correct speed if it goes below 1
	if (speed <= 0)
		speed = 1;

	// Correct speed if it passes max value
	if (speed > 1000)
		speed = 1000;

	if (sd && sd->skilltimer != -1 && (skill = pc_checkskill(sd, SA_FREECAST)) > 0) {
		sd->prev_speed = speed;
		speed = speed * (175 - skill * 5) / 100;
	}

	// Apply player speed changes
	if (sd) {
		sd->speed_add_rate = speed_add_rate;
		sd->speed_rate = speed_rate;
		sd->speed = speed;
		clif_updatestatus(sd, SP_SPEED);
		return;
	}
	// Apply monster speed changes
	else if (md) {
		md->speed = speed;
		return;
	}

	return; // Impossible to reach
}

/*==========================================
 *
 *------------------------------------------
 */
int status_get_class(struct block_list *bl) {

	nullpo_retr(0, bl);

	// Return monster ID for monsters as the class
	if (bl->type == BL_MOB)
		return ((struct mob_data *)bl)->class;

	// Return class for players
	else if (bl->type == BL_PC)
		return ((struct map_session_data *)bl)->status.class;

	// Return monster ID for pets as the class
	else if (bl->type == BL_PET)
		return ((struct pet_data *)bl)->class;

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int status_get_dir(struct block_list *bl) {

	nullpo_retr(0, bl);

	// Monsters
	if (bl->type == BL_MOB)
		return ((struct mob_data *)bl)->dir;

	// Players
	else if (bl->type == BL_PC)
		return ((struct map_session_data *)bl)->dir;

	// Pets
	else if (bl->type == BL_PET)
		return ((struct pet_data *)bl)->dir;

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int status_get_lv(struct block_list *bl) {

	nullpo_retr(0, bl);

	// Monsters
	if (bl->type == BL_MOB)
		return ((struct mob_data *)bl)->level;

	// Players
	else if (bl->type == BL_PC)
		return ((struct map_session_data *)bl)->status.base_level;

	// Players
	else if (bl->type == BL_PET)
		return ((struct pet_data *)bl)->msd->pet.level;

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int status_get_range(struct block_list *bl) {

	nullpo_retr(0, bl);

	// Monsters
	if (bl->type == BL_MOB)
		return mob_db[((struct mob_data *)bl)->class].range;

	// Players
	else if (bl->type == BL_PC)
		return ((struct map_session_data *)bl)->attackrange;

	// Pets
	else if (bl->type == BL_PET)
		return mob_db[((struct pet_data *)bl)->class].range;

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int status_get_hp(struct block_list *bl) {

	nullpo_retr(1, bl);

	// Monsters
	if (bl->type == BL_MOB)
		return ((struct mob_data *)bl)->hp;

	// Players
	else if (bl->type == BL_PC)
		return ((struct map_session_data *)bl)->status.hp;

	return 1;
}

/*==========================================
 *
 *------------------------------------------
 */
int status_get_max_hp(struct block_list *bl) {

	nullpo_retr(1, bl);

	// Players
	if (bl->type == BL_PC)
		return ((struct map_session_data *)bl)->status.max_hp;

	// Non-Players Characters (NPCs)
	else {
		struct status_change *sc_data;
		int max_hp = 1;

		// Monsters
		if (bl->type == BL_MOB) {
			struct mob_data *md;
			nullpo_retr(1, md = (struct mob_data *)bl);
			max_hp = mob_db[md->class].max_hp;
			if (mob_db[md->class].mexp > 0) {
				if (battle_config.mvp_hp_rate != 100)
					max_hp = max_hp * battle_config.mvp_hp_rate / 100;
			} else {
				if (battle_config.monster_hp_rate != 100)
					max_hp = max_hp * battle_config.monster_hp_rate / 100;
			}

		}

		// Pets
		else if (bl->type == BL_PET) {
			struct pet_data *pd;
			nullpo_retr(1, pd = (struct pet_data*)bl);
			max_hp = mob_db[pd->class].max_hp;
			if (mob_db[pd->class].mexp > 0) {
				if (battle_config.mvp_hp_rate != 100)
					max_hp = max_hp * battle_config.mvp_hp_rate / 100;
			} else {
				if (battle_config.monster_hp_rate != 100)
					max_hp = max_hp * battle_config.monster_hp_rate / 100;
			}
		}

		// Extra calculations
		sc_data = status_get_sc_data(bl);
		if (sc_data) {
			if (sc_data[SC_APPLEIDUN].timer != -1)
				max_hp += ((5 + sc_data[SC_APPLEIDUN].val1 * 2 + ((sc_data[SC_APPLEIDUN].val2 + 1) >> 1)
				          + sc_data[SC_APPLEIDUN].val3 / 10) * max_hp) / 100;
		}

		if (max_hp < 1)
			max_hp = 1;

		return max_hp;
	}

	return 1;
}

/*==========================================
 *
 *------------------------------------------
 */
int status_get_str(struct block_list *bl) {

	int str = 0;

	nullpo_retr(0, bl);

	// Players
	if (bl->type == BL_PC)
		return ((struct map_session_data *)bl)->paramc[0];

	// Non-Player Characters (NPCs)
	else {
		struct status_change *sc_data;
		sc_data = status_get_sc_data(bl);

		// Monsters
		if (bl->type == BL_MOB) {
			str = mob_db[((struct mob_data *)bl)->class].str;
		}

		// Pets
		else if (bl->type == BL_PET)
			str = mob_db[((struct pet_data *)bl)->class].str;

		// Status change calculation
		if (sc_data) {
			if (sc_data[SC_LOUD].timer!=-1 && sc_data[SC_QUAGMIRE].timer == -1)
				str += 4;
			if (sc_data[SC_BLESSING].timer != -1) {
				int race=status_get_race(bl);
				if (battle_check_undead(race, status_get_elem_type(bl)) || race == 6)
					str >>= 1;
				else
					str += sc_data[SC_BLESSING].val1;
			}
			if (sc_data[SC_NEN].timer != -1)
				str += sc_data[SC_NEN].val1;
			if (sc_data[SC_TRUESIGHT].timer != -1)
				str += 5;
			if (sc_data[SC_INCSTR].timer != -1)
				str += sc_data[SC_INCSTR].val1;
			if (sc_data[SC_INCALLSTATUS].timer != -1)
				str += sc_data[SC_INCALLSTATUS].val1;
			if (sc_data[SC_STRFOOD].timer != -1)
				str += sc_data[SC_STRFOOD].val1;
		}

		// If Strength value is invalid, set to 0
		if (str < 0)
			str = 0;
	}

	return str;
}

/*==========================================
 *
 *------------------------------------------
 */
int status_get_agi(struct block_list *bl) {

	int agi = 0;

	nullpo_retr(0, bl);

	// Players
	if (bl->type == BL_PC)
		agi = ((struct map_session_data *)bl)->paramc[1];

	// Non-Player Characters (NPCs)
	else {
		struct status_change *sc_data;
		sc_data = status_get_sc_data(bl);

		// Monsters
		if (bl->type == BL_MOB) {
			agi = mob_db[((struct mob_data *)bl)->class].agi;
		}
		
		// Pets
		else if (bl->type == BL_PET)
			agi = mob_db[((struct pet_data *)bl)->class].agi;

		// Status change calculation
		if (sc_data) {
			if (sc_data[SC_QUAGMIRE].timer != -1)
				agi -= sc_data[SC_QUAGMIRE].val1 * 10;
			if (sc_data[SC_INCREASEAGI].timer != -1 && sc_data[SC_DONTFORGETME].timer == -1)
				agi += 2 + sc_data[SC_INCREASEAGI].val1;
			if (sc_data[SC_CONCENTRATE].timer != -1)
				agi += agi * (2 + sc_data[SC_CONCENTRATE].val1) / 100;
			if (sc_data[SC_DECREASEAGI].timer != -1)
				agi -= 2 + sc_data[SC_DECREASEAGI].val1;
			if (sc_data[SC_TRUESIGHT].timer != -1)
				agi += 5;
			if (sc_data[SC_INCAGI].timer != -1)
				agi += sc_data[SC_INCAGI].val1;
			if (sc_data[SC_INCALLSTATUS].timer != -1)
				agi += sc_data[SC_INCALLSTATUS].val1;
			if (sc_data[SC_AGIFOOD].timer != -1)
				agi += sc_data[SC_AGIFOOD].val1;
			if (sc_data[SC_INCREASING].timer != -1)
				agi += 4;
			if (sc_data[SC_SUITON].timer!=-1 && sc_data[SC_SUITON].val3)
				agi -= sc_data[SC_SUITON].val2;
		}

		// If Agility value is invalid, set to 0
		if (agi < 0)
			agi = 0;
	}

	return agi;
}

/*==========================================
 *
 *------------------------------------------
 */
int status_get_vit(struct block_list *bl) {

	int vit = 0;

	nullpo_retr(0, bl);

	// Players
	if (bl->type == BL_PC)
		return ((struct map_session_data *)bl)->paramc[2];

	// Non-Player Characters (NPCs)
	else {
		struct status_change *sc_data;
		sc_data = status_get_sc_data(bl);

		// Monsters
		if (bl->type == BL_MOB) {
			vit = mob_db[((struct mob_data *)bl)->class].vit;
		}
		
		// Pets
		else if (bl->type == BL_PET)
			vit = mob_db[((struct pet_data *)bl)->class].vit;

		// Status change calculation
		if (sc_data) {
			if (sc_data[SC_STRIPARMOR].timer != -1)
				vit = vit * 60 / 100;
			if (sc_data[SC_TRUESIGHT].timer != -1)
				vit += 5;
			if (sc_data[SC_INCALLSTATUS].timer != -1)
				vit += sc_data[SC_INCALLSTATUS].val1;
			if (sc_data[SC_VITFOOD].timer != -1)
				vit += sc_data[SC_VITFOOD].val1;
		}

		// If Vitality value is invalid, set to 0
		if (vit < 0)
			vit = 0;
	}

	return vit;
}

/*==========================================
 *
 *------------------------------------------
 */
int status_get_int(struct block_list *bl) {

	// We use 'int_', since 'int' is a C langauge key syntax term..
	int int_ = 0;

	nullpo_retr(0, bl);

	// Players
	if (bl->type == BL_PC)
		return ((struct map_session_data *)bl)->paramc[3];

	// Non-Player Characters
	else {
		struct status_change *sc_data;
		sc_data = status_get_sc_data(bl);

		// Monsters
		if (bl->type == BL_MOB) {
			int_ = mob_db[((struct mob_data *)bl)->class].int_;
		}

		// Pets
		else if (bl->type == BL_PET)
			int_ = mob_db[((struct pet_data *)bl)->class].int_;

		// Status change calculation
		if (sc_data) {
			if (sc_data[SC_BLESSING].timer != -1) {
				int race = status_get_race(bl);
				if (battle_check_undead(race,status_get_elem_type(bl)) || race == 6)
					int_ >>= 1;
				else
					int_ += sc_data[SC_BLESSING].val1;
			}
			if (sc_data[SC_NEN].timer != -1)
				int_ += sc_data[SC_NEN].val1;
			if (sc_data[SC_STRIPHELM].timer != -1)
				int_ = int_ * 60 / 100;
			if (sc_data[SC_TRUESIGHT].timer != -1)
				int_ += 5;
			if (sc_data[SC_INCALLSTATUS].timer != -1)
				int_ += sc_data[SC_INCALLSTATUS].val1;
			if (sc_data[SC_INTFOOD].timer != -1)
				int_ += sc_data[SC_INTFOOD].val1;
		}

		// If Intelligence value is invalid, set to 0
		if (int_ < 0)
			int_ = 0;
	}

	return int_;
}

/*==========================================
 *
 *------------------------------------------
 */
int status_get_dex(struct block_list *bl) {

	int dex = 0;

	nullpo_retr(0, bl);

	// Players
	if (bl->type == BL_PC)
		return ((struct map_session_data *)bl)->paramc[4];

	// Non-Player Characters (NPCs)
	else {
		struct status_change *sc_data;
		sc_data = status_get_sc_data(bl);

		// Monsters
		if (bl->type == BL_MOB) {
			dex = mob_db[((struct mob_data *)bl)->class].dex;
		}

		// Pets
		else if (bl->type == BL_PET)
			dex = mob_db[((struct pet_data *)bl)->class].dex;

		// Status change calculation
		if (sc_data) {
			if (sc_data[SC_CONCENTRATE].timer != -1 && sc_data[SC_QUAGMIRE].timer == -1)
				dex += dex * (2 + sc_data[SC_CONCENTRATE].val1) / 100;
			if (sc_data[SC_BLESSING].timer != -1) {
				int race = status_get_race(bl);
				if (battle_check_undead(race, status_get_elem_type(bl)) || race == 6)
					dex >>= 1;
				else
					dex += sc_data[SC_BLESSING].val1;
			}
			if (sc_data[SC_QUAGMIRE].timer != -1)
				dex -= sc_data[SC_QUAGMIRE].val1 * 10;
			if (sc_data[SC_TRUESIGHT].timer != -1)
				dex += 5;
			if (sc_data[SC_INCDEX].timer != -1)
				dex += sc_data[SC_INCDEX].val1;
			if (sc_data[SC_INCALLSTATUS].timer != -1)
				dex += sc_data[SC_INCALLSTATUS].val1;
			if (sc_data[SC_DEXFOOD].timer != -1)
				dex += sc_data[SC_DEXFOOD].val1;
			if (sc_data[SC_INCREASING].timer != -1)
				dex += 4;
		}

		// If Dexterity value is invalid, set to 0
		if (dex < 0)
			dex = 0;
	}

	return dex;
}

/*==========================================
 *
 *------------------------------------------
 */
int status_get_luk(struct block_list *bl) {

	int luk = 0;

	nullpo_retr(0, bl);

	// Players
	if (bl->type == BL_PC)
		return ((struct map_session_data *)bl)->paramc[5];

	// Non-Player Characters (NPCs)
	else {
		struct status_change *sc_data;
		sc_data = status_get_sc_data(bl);

		// Monsters
		if (bl->type == BL_MOB) {
			luk = mob_db[((struct mob_data *)bl)->class].luk;
		}

		// Pets
		else if (bl->type == BL_PET)
			luk = mob_db[((struct pet_data *)bl)->class].luk;

		// Status change calculation
		if (sc_data) {
			if (sc_data[SC_GLORIA].timer != -1)
				luk += 30;
			if (sc_data[SC_TRUESIGHT].timer != -1)
				luk += 5;
			if (sc_data[SC_CURSE].timer != -1 )
				luk = 0;
			if (sc_data[SC_INCALLSTATUS].timer != -1)
				luk += sc_data[SC_INCALLSTATUS].val1;
			if (sc_data[SC_LUKFOOD].timer != -1)
				luk += sc_data[SC_LUKFOOD].val1;
		}

		// If Luck value is invalid, set to 0
		if (luk < 0)
			luk = 0;
	}

	return luk;
}

/*==========================================
 *
 *------------------------------------------
 */
int status_get_flee(struct block_list *bl) {

	int flee = 1;

	nullpo_retr(1, bl);

	// Players
	if (bl->type == BL_PC)
		flee = ((struct map_session_data *)bl)->flee;

	// Non-Player Characters (NPCs - Monsters/Pets)
	else {
		struct status_change *sc_data;
		sc_data = status_get_sc_data(bl);
		flee = status_get_agi(bl) + status_get_lv(bl);

		// Status change calculation
		if (sc_data) {
			if (sc_data[SC_WHISTLE].timer != -1)
				flee += flee * (sc_data[SC_WHISTLE].val1 + sc_data[SC_WHISTLE].val2
				       + (sc_data[SC_WHISTLE].val3 >> 16)) / 100;
			if (sc_data[SC_BLIND].timer != -1)
				flee -= flee * 25 / 100;
			if (sc_data[SC_WINDWALK].timer != -1)
				flee += flee * (sc_data[SC_WINDWALK].val2) / 100;
			if (sc_data[SC_SPIDERWEB].timer != -1)
				flee -= flee * 50 / 100;
			if (sc_data[SC_INCFLEE].timer != -1)
				flee += sc_data[SC_INCFLEE].val1;
			if (sc_data[SC_INCFLEERATE].timer != -1)
				flee += flee * sc_data[SC_INCFLEERATE].val1 / 100;
			if (sc_data[SC_CLOSECONFINE].timer != -1)
				flee += 10;
			if (sc_data[SC_FLEEFOOD].timer != -1)
				flee += sc_data[SC_FLEEFOOD].val1;
			if (sc_data[SC_ADJUSTMENT].timer != -1)
				flee += 30;
			if (sc_data[SC_GATLINGFEVER].timer != -1)
				flee -= sc_data[SC_GATLINGFEVER].val4;
		}
	}

		// If Flee value is invalid, set to 1
	if (flee < 1)
		flee = 1;

	return flee;
}

/*==========================================
 *
 *------------------------------------------
 */
int status_get_hit(struct block_list *bl) {

	int hit = 1;

	nullpo_retr(1, bl);

	// Players
	if (bl->type == BL_PC)
		hit = ((struct map_session_data *)bl)->hit;

	// Non-Player Characters (NPCs - Monsters/Pets)
	else {
		struct status_change *sc_data;
		sc_data = status_get_sc_data(bl);
		hit = status_get_dex(bl) + status_get_lv(bl);

		// Status change calculation
		if (sc_data) {
			if (sc_data[SC_HUMMING].timer != -1)
				hit += hit * (sc_data[SC_HUMMING].val1 * 2 + sc_data[SC_HUMMING].val2
				      + sc_data[SC_HUMMING].val3) / 100;
			if (sc_data[SC_BLIND].timer != -1)
				hit -= hit * 25 / 100;
			if (sc_data[SC_TRUESIGHT].timer != -1)
				hit += hit * 3 * (sc_data[SC_TRUESIGHT].val1) / 100;
			if (sc_data[SC_CONCENTRATION].timer != -1)
				hit += 10 * sc_data[SC_CONCENTRATION].val1;
			if (sc_data[SC_INCHITRATE].timer != -1)
				hit += hit * sc_data[SC_INCHITRATE].val1 / 100;
			if (sc_data[SC_HITFOOD].timer != -1)
				hit += sc_data[SC_HITFOOD].val1;
			if (sc_data[SC_ADJUSTMENT].timer != -1)
				hit -= 30;
			if (sc_data[SC_INCREASING].timer!=-1)
				hit += 20;
		}
	}

		// If Hit value is invalid, set to 1
	if (hit < 1)
		hit = 1;

	return hit;
}

/*==========================================
 *
 *------------------------------------------
 */
int status_get_flee2(struct block_list *bl) {

	int flee2 = 1;

	nullpo_retr(1, bl);

	// Players
	if (bl->type == BL_PC)
		return ((struct map_session_data *)bl)->flee2;

	// Non-Player Characters (NPCs - Monsters/Pets)
	else {
		struct status_change *sc_data;
		sc_data = status_get_sc_data(bl);
		flee2 = status_get_luk(bl) + 1;

		// Status change calculation
		if (sc_data) {
			if (sc_data[SC_WHISTLE].timer != -1)
				flee2 += (sc_data[SC_WHISTLE].val1 + sc_data[SC_WHISTLE].val2
				       + (sc_data[SC_WHISTLE].val3 & 0xffff)) * 10;
		}
	}

		// If Flee2 value is invalid, set to 1
	if (flee2 < 1)
		flee2 = 1;

	return flee2;
}

/*==========================================
 *
 *------------------------------------------
 */
int status_get_critical(struct block_list *bl) {

	int critical = 1;

	nullpo_retr(1, bl);

	// Players
	if (bl->type==BL_PC) {
		return ((struct map_session_data *)bl)->critical;
	}

	// Non-Player Characters (NPCs - Monsters/Pets)
	else {
		struct status_change *sc_data;
		sc_data = status_get_sc_data(bl);
		critical = status_get_luk(bl) * 3 + 1;

		// Status change calculation
		if (sc_data) {
			if (sc_data[SC_FORTUNE].timer != -1)
				critical += (10+sc_data[SC_FORTUNE].val1 + sc_data[SC_FORTUNE].val2
				          + sc_data[SC_FORTUNE].val3) * 10;
			if (sc_data[SC_EXPLOSIONSPIRITS].timer != -1)
				critical += sc_data[SC_EXPLOSIONSPIRITS].val2;
			if (sc_data[SC_TRUESIGHT].timer != -1)
				critical += critical * sc_data[SC_TRUESIGHT].val1 / 100;
		}
	}

	// If Critical value is invalid, set to 1
	if (critical < 1)
		critical = 1;

	return critical;
}

/*==========================================
 *
 *------------------------------------------
 */
int status_get_baseatk(struct block_list *bl) {

	int batk = 1;

	nullpo_retr(1, bl);

	// Players
	if (bl->type == BL_PC) {
		batk = ((struct map_session_data *)bl)->base_atk;
		if (((struct map_session_data *)bl)->status.weapon <= 22)
			batk += ((struct map_session_data *)bl)->weapon_atk[((struct map_session_data *)bl)->status.weapon];
	}

	// Non-Player Characters (NPCs - Monsters/Pets)
	else {
		int str, dstr;
		struct status_change *sc_data;
		sc_data = status_get_sc_data(bl);
		str = status_get_str(bl);
		dstr = str / 10;
		batk = dstr * dstr + str;

		// Status Change Calculation
		if (sc_data) {
			if (sc_data[SC_PROVOKE].timer != -1)
				batk = batk * (100 + (3 * sc_data[SC_PROVOKE].val1 + 2)) / 100;
			if (sc_data[SC_CURSE].timer != -1)
				batk -= batk * 25 / 100;
			if (sc_data[SC_CONCENTRATION].timer != -1)
				batk += batk * (5 * sc_data[SC_CONCENTRATION].val1) / 100;
			if (sc_data[SC_BATKFOOD].timer != -1)
				batk += sc_data[SC_BATKFOOD].val1;
			if (sc_data[SC_BLEEDING].timer != -1)
				batk -= batk * 25 / 100;
			if (sc_data[SC_SKE].timer != -1)
				batk *= 4;
			if (sc_data[SC_GATLINGFEVER].timer != -1)
				batk += sc_data[SC_GATLINGFEVER].val3;
			if (sc_data[SC_MADNESSCANCEL].timer != -1)
				batk += 100;
		}
	}

	// If Base Attack value is invalid, set to 1
	if (batk < 1)
		batk = 1;

	return batk;
}

/*==========================================
 *
 *------------------------------------------
 */
int status_get_atk(struct block_list *bl) {

	int atk = 0;

	nullpo_retr(0, bl);

	// Players
	if (bl->type == BL_PC)
		return ((struct map_session_data*)bl)->watk;

	// Non-Player Characters (NPCs - Monsters/Pets)
	else {
		struct status_change *sc_data;
		sc_data = status_get_sc_data(bl);

		// Monsters
		if (bl->type == BL_MOB)
			atk = mob_db[((struct mob_data*)bl)->class].atk1;

		// Pets
		else if (bl->type == BL_PET)
			atk = mob_db[((struct pet_data*)bl)->class].atk1;

		// Status Change Calculation
		if (sc_data) {
			if (sc_data[SC_PROVOKE].timer != -1)
				atk = atk * (100 + (3 * sc_data[SC_PROVOKE].val1 + 2)) / 100;
			if (sc_data[SC_CURSE].timer != -1)
				atk -= atk * 25 / 100;
			if (sc_data[SC_CONCENTRATION].timer != -1)
				atk += atk * (5 * sc_data[SC_CONCENTRATION].val1) / 100;
			if (sc_data[SC_INCATKRATE].timer != -1)
				atk += atk * sc_data[SC_INCATKRATE].val1 / 100;
			if (sc_data[SC_WATKFOOD].timer != -1)
				atk += atk * sc_data[SC_WATKFOOD].val1 / 100;
			if (sc_data[SC_SKE].timer != -1)
				atk *= 4;
		}
	}

	// If Attack value is invalid, set to 0
	if (atk < 0)
		atk = 0;

	return atk;
}

/*==========================================
 *
 *------------------------------------------
 */
int status_get_atk_(struct block_list *bl) {

	nullpo_retr(0, bl);

	// Only works with Players
	if (bl->type == BL_PC)
		return ((struct map_session_data*)bl)->watk_;

	// If not a Player, return 0
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int status_get_atk2(struct block_list *bl) {

	nullpo_retr(0, bl);

	// Players
	if (bl->type == BL_PC)
		return ((struct map_session_data*)bl)->watk2;

	// Non-Player Characters (NPCs - Monsters/Pets)
	else {
		struct status_change *sc_data = status_get_sc_data(bl);
		int atk2 = 0;

		// Monsters
		if (bl->type == BL_MOB)
			atk2 = mob_db[((struct mob_data*)bl)->class].atk2;

		// Pets
		else if (bl->type == BL_PET)
			atk2 = mob_db[((struct pet_data*)bl)->class].atk2;

		// Status change calculation
		if (sc_data) {
			if (sc_data[SC_IMPOSITIO].timer != -1)
				atk2 += sc_data[SC_IMPOSITIO].val1 * 5;
			if (sc_data[SC_PROVOKE].timer != -1)
				atk2 = atk2 * (100 + (3 * sc_data[SC_PROVOKE].val1 + 2)) / 100;
			if (sc_data[SC_CURSE].timer!=-1 )
				atk2 -= atk2 * 25 / 100;
			if (sc_data[SC_DRUMBATTLE].timer != -1)
				atk2 += sc_data[SC_DRUMBATTLE].val2;
			if (sc_data[SC_NIBELUNGEN].timer != -1 && (status_get_element(bl) / 10) >= 8)
				atk2 += sc_data[SC_NIBELUNGEN].val3;
			if (sc_data[SC_STRIPWEAPON].timer != -1)
				atk2 = atk2 * sc_data[SC_STRIPWEAPON].val2 / 100;
			if (sc_data[SC_CONCENTRATION].timer != -1)
				atk2 += atk2 * (5 * sc_data[SC_CONCENTRATION].val1) / 100;
			if (sc_data[SC_INCATKRATE].timer!=-1)
				atk2 += atk2 * sc_data[SC_INCATKRATE].val1 / 100;
			if (sc_data[SC_WATKFOOD].timer != -1)
				atk2 += atk2 * sc_data[SC_WATKFOOD].val1 / 100;
			if (sc_data[SC_SKE].timer != -1)
				atk2 *= 4;
		}

		// If Attack2 value is invalid, set to 0
		if (atk2 < 0)
			atk2 = 0;
		return atk2;
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int status_get_atk_2(struct block_list *bl) {

	nullpo_retr(0, bl);

	// Only works with Players
	if (bl->type == BL_PC)
		return ((struct map_session_data*)bl)->watk_2;

	// If not a Player, return 0
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int status_get_matk1(struct block_list *bl) {

	int matk = 0;

	nullpo_retr(0, bl);

	// Players
	if (bl->type == BL_PC)
		return ((struct map_session_data *)bl)->matk1;

	// Non-Player Characters (NPCs - Monsters/Pets)
	else {
		struct status_change *sc_data;
		int int_ = status_get_int(bl);
		sc_data = status_get_sc_data(bl);
		matk = int_ + (int_ / 5) * (int_ / 5);

		// Status change calculation
		if (sc_data) {
			if (sc_data[SC_MINDBREAKER].timer!=-1)
				matk = matk * (sc_data[SC_MINDBREAKER].val3) / 100;
			if (sc_data[SC_INCMATKRATE].timer!=-1)
				matk = matk * (100 + sc_data[SC_INCMATKRATE].val1) /100;
			if (sc_data[SC_MATKFOOD].timer!=-1)
				matk = matk * (100 + sc_data[SC_MATKFOOD].val1) /100;
		}

	// If Magic Attack value is invalid, set to 0
	if (matk < 0)
		matk = 0;

	}

	return matk;
}

/*==========================================
 *
 *------------------------------------------
 */
int status_get_matk2(struct block_list *bl) {

	int matk = 0;

	nullpo_retr(0, bl);

	// Players
	if (bl->type == BL_PC)
		return ((struct map_session_data *)bl)->matk2;

	// Non-Player Characters (NPCs - Monsters/Pets)
	else {
		struct status_change *sc_data;
		int int_ = status_get_int(bl);
		sc_data = status_get_sc_data(bl);
		matk = int_ + (int_ / 7) * (int_ / 7);

		// Status change calculation
		if (sc_data)
			if (sc_data[SC_MINDBREAKER].timer != -1)
				matk = matk * (sc_data[SC_MINDBREAKER].val3) / 100;
	}

	// If Magic Attack value is invalid, set to 0
	if (matk < 0)
		matk = 0;

	return matk;
}

/*==========================================
 *
 *------------------------------------------
 */
int status_get_def(struct block_list *bl) {

	struct status_change *sc_data;
	int def = 0,skilltimer = -1, skillid = 0;

	nullpo_retr(0, bl);

	// Retrieve status change data
	sc_data = status_get_sc_data(bl);

	// Players
	if (bl->type == BL_PC) {
		def = ((struct map_session_data *)bl)->def;
		skilltimer = ((struct map_session_data *)bl)->skilltimer;
		skillid = ((struct map_session_data *)bl)->skillid;
	}

	// Monsters
	else if (bl->type == BL_MOB) {
		def = mob_db[((struct mob_data *)bl)->class].def;
		skilltimer = ((struct mob_data *)bl)->skilltimer;
		skillid = ((struct mob_data *)bl)->skillid;
	}

	// Pets
	else if (bl->type == BL_PET)
		def = mob_db[((struct pet_data *)bl)->class].def;

	// If Defense is less than 1000000 (Max Defense), activate
	if (def < 1000000) {
		// Status change calculation
		if (sc_data) {
			if (sc_data[SC_FREEZE].timer != -1 || (sc_data[SC_STONE].timer != -1 && sc_data[SC_STONE].val2 == 0))
				def >>= 1;

			// Non-Player Characters (NPCs - Monsters/Pets)
			// Player calculation done in status_calc_pc
			if (bl->type != BL_PC) {
				if (sc_data[SC_PROVOKE].timer != -1)
					def = (def * (100 - (5 * sc_data[SC_PROVOKE].val1 + 5))) / 100;
				if (sc_data[SC_KEEPING].timer != -1)
					def = 100;
				if (sc_data[SC_DRUMBATTLE].timer != -1)
					def += sc_data[SC_DRUMBATTLE].val3;
				if (sc_data[SC_POISON].timer != -1)
					def = def * 75 / 100;
				if (sc_data[SC_STRIPSHIELD].timer != -1)
					def = def * sc_data[SC_STRIPSHIELD].val2 / 100;
				if (sc_data[SC_SIGNUMCRUCIS].timer != -1)
					def = def * (100 - sc_data[SC_SIGNUMCRUCIS].val2) / 100;
				if (sc_data[SC_ETERNALCHAOS].timer != -1)
					def = 0;
				if (sc_data[SC_CONCENTRATION].timer != -1)
					def = (def * (100 - 5 * sc_data[SC_CONCENTRATION].val1)) / 100;
				if (sc_data[SC_JOINTBEAT].timer != -1) {
					if (sc_data[SC_JOINTBEAT].val2 == 4)
						def -= def * 50 / 100;
					else if (sc_data[SC_JOINTBEAT].val2 == 5)
						def -= def * 25 / 100;
				}
				if (sc_data[SC_INCDEFRATE].timer!=-1)
					def += def * sc_data[SC_INCDEFRATE].val1 / 100;
				if (sc_data[SC_SKA].timer != - 1)
					def = sc_data[SC_SKA].val3;
				if (sc_data[SC_SKE].timer != -1)
					def /= 2;
				if (sc_data[SC_FLING].timer != -1)
					def -= def * (sc_data[SC_FLING].val2) / 100;
			}
		}

		// If casting a skill
		if (skilltimer != -1) {
			int def_rate = skill_get_castdef(skillid);
			if (def_rate != 0)
				def = (def * (100 - def_rate)) / 100;
		}
	}

	// If Defense value is invalid, set to 0
	if (def < 0)
		def = 0;

	return def;
}

/*==========================================
 *
 *------------------------------------------
 */
int status_get_mdef(struct block_list *bl) {

	struct status_change *sc_data;
	int mdef = 0;

	nullpo_retr(0, bl);

	// Retreive status change data
	sc_data = status_get_sc_data(bl);

	// Players
	if (bl->type == BL_PC)
		mdef = ((struct map_session_data *)bl)->mdef;

	// Monsters
	else if(bl->type == BL_MOB)
		mdef = mob_db[((struct mob_data *)bl)->class].mdef;

	// Pets
	else if(bl->type == BL_PET)
		mdef = mob_db[((struct pet_data *)bl)->class].mdef;

	// If Magic Defense is less than 1000000 (Max Magic Defense), activate
	if (mdef < 1000000) {
		// Status change calculation
		if (sc_data) {
			if (sc_data[SC_BARRIER].timer != -1)
				mdef = 100;
			if (sc_data[SC_FREEZE].timer != -1 || (sc_data[SC_STONE].timer != -1 && sc_data[SC_STONE].val2 == 0))
				mdef = mdef * 125 / 100;
			if (sc_data[SC_SKA].timer != -1)
				mdef = 90;
		}
	}

	// If Magic Defense value is invalid, set to 0
	if (mdef < 0)
		mdef = 0;

	return mdef;
}

/*==========================================
 *
 *------------------------------------------
 */
int status_get_def2(struct block_list *bl) {

	int def2 = 1;

	nullpo_retr(1, bl);

	// Players
	if (bl->type == BL_PC)
		return ((struct map_session_data *)bl)->def2;

	// Non-Player Characters
	else {
		struct status_change *sc_data;

		// Monsters
		if (bl->type == BL_MOB)
			def2 = mob_db[((struct mob_data *)bl)->class].vit;

		// Pets
		else if (bl->type == BL_PET)
			def2 = mob_db[((struct pet_data *)bl)->class].vit;

		// Retrieve status change data
		sc_data = status_get_sc_data(bl);

		// Status change calculation
		if (sc_data) {
			if (sc_data[SC_ANGELUS].timer != -1)
				def2 = def2 * (110 + 5 * sc_data[SC_ANGELUS].val1) / 100;
			if (sc_data[SC_PROVOKE].timer != -1)
				def2 = (def2 * (100 - (5 * sc_data[SC_PROVOKE].val1) + 5)) / 100;
			if (sc_data[SC_POISON].timer != -1)
				def2 = def2 * 75 / 100;
			if (sc_data[SC_CONCENTRATION].timer != -1)
				def2 = def2 * (100 - 5 * sc_data[SC_CONCENTRATION].val1) / 100;
			if (sc_data[SC_SKE].timer != -1)
				def2 /= 2;
			if (sc_data[SC_FLING].timer!=-1)
				def2 -= def2 * (sc_data[SC_FLING].val3) / 100;
		}
	}

	// If Defense2 value is invalid, set to 1
	if (def2 < 1)
		def2 = 1;

	return def2;
}

/*==========================================
 *
 *------------------------------------------
 */
int status_get_mdef2(struct block_list *bl) {

	int mdef2 = 0;

	nullpo_retr(0, bl);

	// Players
	if (bl->type == BL_PC)
		return ((struct map_session_data *)bl)->mdef2 + (((struct map_session_data *)bl)->paramc[2] >> 1);

	// Non-Player Characters (NPCs - Monsters/Pets)
	else {
		// Retrieve status change data
		struct status_change *sc_data = status_get_sc_data(bl);

		// Monsters
		if (bl->type == BL_MOB)
			mdef2 = mob_db[((struct mob_data *)bl)->class].int_ + (mob_db[((struct mob_data *)bl)->class].vit >> 1);

		// Pets
		else if (bl->type == BL_PET)
			mdef2 = mob_db[((struct pet_data *)bl)->class].int_ + (mob_db[((struct pet_data *)bl)->class].vit >> 1);

		// Status change calculation
		if (sc_data) {
			if (sc_data[SC_MINDBREAKER].timer != -1)
				mdef2 = mdef2 * (sc_data[SC_MINDBREAKER].val2) / 100;
		}
	}

	// If Magic Defense2 value is invalid, set to 0
	if (mdef2 < 0)
		mdef2 = 0;

	return mdef2;
}

/*==========================================
 *
 *-------------------------------------------
*/
int status_get_adelay(struct block_list *bl) {

	nullpo_retr(4000, bl);

	// Players
	if (bl->type==BL_PC)
		return (((struct map_session_data *)bl)->aspd<<1);

	// Non-Player Characters
	else {
		struct status_change *sc_data=status_get_sc_data(bl);
		int adelay = 4000, aspd_rate = 100, i;

		// Monsters
		if (bl->type == BL_MOB)
			adelay = mob_db[((struct mob_data *)bl)->class].adelay;

		// Pets
		else if(bl->type==BL_PET)
			adelay = mob_db[((struct pet_data *)bl)->class].adelay;

		// Status change caculation
		if (sc_data) {
			if ((sc_data[SC_TWOHANDQUICKEN].timer != -1 || sc_data[SC_ONEHAND].timer != -1) && sc_data[SC_QUAGMIRE].timer == -1 && sc_data[SC_DONTFORGETME].timer == -1)
				aspd_rate -= 30;
			if (sc_data[SC_ADRENALINE].timer != -1 && sc_data[SC_TWOHANDQUICKEN].timer == -1 && sc_data[SC_ONEHAND].timer == -1 && sc_data[SC_QUAGMIRE].timer == -1 && sc_data[SC_DONTFORGETME].timer == -1) {
				if (sc_data[SC_ADRENALINE].val2 || !battle_config.party_skill_penalty)
					aspd_rate -= 30;
				else
					aspd_rate -= 25;
			}

			if (sc_data[SC_SPEARQUICKEN].timer != -1 && sc_data[SC_ADRENALINE].timer == -1 &&
				sc_data[SC_TWOHANDQUICKEN].timer == -1 && sc_data[SC_ONEHAND].timer == -1 && sc_data[SC_QUAGMIRE].timer == -1 && sc_data[SC_DONTFORGETME].timer == -1)
				aspd_rate -= sc_data[SC_SPEARQUICKEN].val2;
			if (sc_data[SC_ASSNCROS].timer!=-1 && 
				sc_data[SC_TWOHANDQUICKEN].timer == -1 && sc_data[SC_ONEHAND].timer == -1 && sc_data[SC_ADRENALINE].timer == -1 && sc_data[SC_SPEARQUICKEN].timer == -1 &&
				sc_data[SC_DONTFORGETME].timer == -1)
				aspd_rate -= 5+sc_data[SC_ASSNCROS].val1+sc_data[SC_ASSNCROS].val2+sc_data[SC_ASSNCROS].val3;
			if (sc_data[SC_DONTFORGETME].timer!=-1)
				aspd_rate += sc_data[SC_DONTFORGETME].val1 * 3 + sc_data[SC_DONTFORGETME].val2 + (sc_data[SC_DONTFORGETME].val3 >> 16);
			if (sc_data[SC_STEELBODY].timer != -1)
				aspd_rate += 25;
			if (sc_data[i=SC_ASPDPOTION3].timer != -1 || sc_data[i=SC_ASPDPOTION2].timer != -1 || sc_data[i=SC_ASPDPOTION1].timer != -1 || sc_data[i=SC_ASPDPOTION0].timer != -1)
				aspd_rate -= sc_data[i].val2;
			if (sc_data[SC_DEFENDER].timer != -1)
				adelay += (1100 - sc_data[SC_DEFENDER].val1 * 100);
			if (sc_data[SC_INCASPDRATE].timer != -1)
				aspd_rate += sc_data[SC_INCASPDRATE].val1;
			if (sc_data[SC_JOINTBEAT].timer != -1) {
				if (sc_data[SC_JOINTBEAT].val2 == 2)
					aspd_rate = aspd_rate * 125 / 100;
				else if (sc_data[SC_JOINTBEAT].val2 == 3)
					aspd_rate = aspd_rate * 110 / 100;
			}
		}

		if (aspd_rate != 100)
			adelay = adelay*aspd_rate/100;

		if (adelay < battle_config.monster_max_aspd<<1)
			adelay = battle_config.monster_max_aspd<<1;

		return adelay;
	}

	return 4000;
}

/*==========================================
 *
 *-------------------------------------------
*/
int status_get_amotion(struct block_list *bl) {
	nullpo_retr(2000, bl);

	// Players
	if (bl->type == BL_PC)
		return ((struct map_session_data *)bl)->amotion;

	// Non-Player Characters
	else {
		// Retrieve status change data
		struct status_change *sc_data=status_get_sc_data(bl);

		int amotion = 2000, aspd_rate = 100, i;

		// Monsters
		if (bl->type == BL_MOB)
			amotion = mob_db[((struct mob_data *)bl)->class].amotion;

		// Pets
		else if(bl->type == BL_PET)
			amotion = mob_db[((struct pet_data *)bl)->class].amotion;

		// Status change calculation
		if (sc_data) {
			if ((sc_data[SC_TWOHANDQUICKEN].timer != -1 || sc_data[SC_ONEHAND].timer != -1) && sc_data[SC_QUAGMIRE].timer == -1 && sc_data[SC_DONTFORGETME].timer == -1)
				aspd_rate -= 30;
			if (sc_data[SC_ADRENALINE].timer != -1 && sc_data[SC_TWOHANDQUICKEN].timer == -1 && sc_data[SC_ONEHAND].timer == -1 &&
				sc_data[SC_QUAGMIRE].timer == -1 && sc_data[SC_DONTFORGETME].timer == -1) {
				if (sc_data[SC_ADRENALINE].val2 || !battle_config.party_skill_penalty)
					aspd_rate -= 30;
				else
					aspd_rate -= 25;
			}
			if (sc_data[SC_SPEARQUICKEN].timer != -1 && sc_data[SC_ADRENALINE].timer == -1 &&
				sc_data[SC_TWOHANDQUICKEN].timer == -1 && sc_data[SC_ONEHAND].timer == -1 && sc_data[SC_QUAGMIRE].timer == -1 && sc_data[SC_DONTFORGETME].timer == -1)
				aspd_rate -= sc_data[SC_SPEARQUICKEN].val2;
			if (sc_data[SC_ASSNCROS].timer!=-1 && 
				sc_data[SC_TWOHANDQUICKEN].timer == -1 && sc_data[SC_ONEHAND].timer == -1 && sc_data[SC_ADRENALINE].timer == -1 && sc_data[SC_SPEARQUICKEN].timer == -1 &&
				sc_data[SC_DONTFORGETME].timer == -1)
				aspd_rate -= 5+sc_data[SC_ASSNCROS].val1+sc_data[SC_ASSNCROS].val2+sc_data[SC_ASSNCROS].val3;
			if (sc_data[SC_DONTFORGETME].timer!=-1)
				aspd_rate += sc_data[SC_DONTFORGETME].val1 * 3 + sc_data[SC_DONTFORGETME].val2 + (sc_data[SC_DONTFORGETME].val3 >> 16);
			if (sc_data[SC_STEELBODY].timer!=-1)
				aspd_rate += 25;
			if (sc_data[i=SC_ASPDPOTION3].timer != -1 || sc_data[i=SC_ASPDPOTION2].timer != -1 || sc_data[i=SC_ASPDPOTION1].timer != -1 || sc_data[i=SC_ASPDPOTION0].timer != -1)
				aspd_rate -= sc_data[i].val2;
			if (sc_data[SC_DEFENDER].timer != -1)
				amotion += (550 - sc_data[SC_DEFENDER].val1*50);
		}

		if (aspd_rate != 100)
			amotion = amotion*aspd_rate/100;

		if (amotion < battle_config.monster_max_aspd)
			amotion = battle_config.monster_max_aspd;

		return amotion;
	}

	return 2000;
}

/*==========================================
 *
 *-------------------------------------------
*/
int status_get_dmotion(struct block_list *bl) {

	int ret;
	struct status_change *sc_data;

	nullpo_retr(0, bl);

	// Retrieve status change data
	sc_data = status_get_sc_data(bl);

	// Monsters
	if (bl->type == BL_MOB) {
		ret = mob_db[((struct mob_data *)bl)->class].dmotion;
		if (battle_config.monster_damage_delay_rate != 100)
			ret = ret * battle_config.monster_damage_delay_rate / 100;
	}

	// Players
	else if (bl->type == BL_PC) {
		ret = ((struct map_session_data *)bl)->dmotion;
		if (battle_config.pc_damage_delay_rate != 100)
			ret = ret * battle_config.pc_damage_delay_rate / 100;
	}

	// Pets
	else if (bl->type == BL_PET)
		ret = mob_db[((struct pet_data *)bl)->class].dmotion;

	// Invalid bl->type
	else
		return 2000;

	// Endure setting -> Doesn't work in GvG maps, activates with Concentration, Endure, Berserk, or Infinite Endure state (Eddga Card)
	if (!map[bl->m].flag.gvg && ((bl->type == BL_PC && ((struct map_session_data *)bl)->special_state.infinite_endure) ||
		(sc_data && (sc_data[SC_ENDURE].timer != -1 || sc_data[SC_CONCENTRATION].timer != -1 || sc_data[SC_BERSERK].timer != -1))))
			return 0;

	return ret;
}

/*==========================================
 *
 *-------------------------------------------
*/
int status_get_element(struct block_list *bl) {

	int ret = 20;
	struct status_change *sc_data;

	nullpo_retr(ret, bl);

	// Retreive status change data
	sc_data = status_get_sc_data(bl);

	// Monsters
	if (bl->type == BL_MOB)
		ret=((struct mob_data *)bl)->def_ele;

	// Players
	else if (bl->type == BL_PC)
		ret=20+((struct map_session_data *)bl)->def_ele;

	// Pets
	else if (bl->type == BL_PET)
		ret = mob_db[((struct pet_data *)bl)->class].element;

	// Status change calculation
	if (sc_data) {
		if (sc_data[SC_BENEDICTIO].timer != -1)
			ret=26;
		if (sc_data[SC_FREEZE].timer!=-1)
			ret=21;
		if (sc_data[SC_STONE].timer!=-1 && sc_data[SC_STONE].val2 == 0)
			ret=22;
	}

	return ret;
}

/*==========================================
 *
 *-------------------------------------------
*/
int status_get_attack_element(struct block_list *bl) {

	int ret = 0;
	struct status_change *sc_data;

	nullpo_retr(0, bl);

	// Retreive status change data
	sc_data = status_get_sc_data(bl);

	// Monsters
	if (bl->type == BL_MOB)
		ret=0;

	// Players
	else if (bl->type == BL_PC)
		ret=((struct map_session_data *)bl)->atk_ele;

	// Pets
	else if (bl->type == BL_PET)
		ret=0;

	// Status Change Calculation
	if (sc_data) {
		// Water
		if (sc_data[SC_WATERWEAPON].timer!=-1)
			ret=1;
		// Earth
		if (sc_data[SC_EARTHWEAPON].timer!=-1)
			ret=2;
		// Fire
		if (sc_data[SC_FIREWEAPON].timer!=-1)
			ret=3;
		// Wind
		if (sc_data[SC_WINDWEAPON].timer!=-1)
			ret=4;
		// Poison
		if (sc_data[SC_ENCPOISON].timer!=-1)
			ret=5;
		// Holy
		if (sc_data[SC_ASPERSIO].timer!=-1)
			ret=6;
		// Shadow
		if (sc_data[SC_SHADOWWEAPON].timer!=-1)
			ret=7;
		// Ghost
		if (sc_data[SC_GHOSTWEAPON].timer!=-1)
			ret=8;
	}

	return ret;
}

/*==========================================
 *
 *-------------------------------------------
*/
int status_get_attack_element2(struct block_list *bl) {

	nullpo_retr(0, bl);

	// Players
	if (bl->type==BL_PC) {
		int ret = ((struct map_session_data *)bl)->atk_ele_;

		// Retreive status change data
		struct status_change *sc_data = ((struct map_session_data *)bl)->sc_data;

		// Status change calculation
		if (sc_data) {
			// Water
			if (sc_data[SC_WATERWEAPON].timer!=-1)
				ret=1;
			// Earth
			if (sc_data[SC_EARTHWEAPON].timer!=-1)
				ret=2;
			// Fire
			if (sc_data[SC_FIREWEAPON].timer!=-1)
				ret=3;
			// Wind
			if (sc_data[SC_WINDWEAPON].timer!=-1)
				ret=4;
			// Poison
			if (sc_data[SC_ENCPOISON].timer!=-1)
				ret=5;
			// Aspersio
			if (sc_data[SC_ASPERSIO].timer!=-1)
				ret=6;
		}
		return ret;
	}

	return 0;
}

/*==========================================
 *
 *-------------------------------------------
*/
int status_get_party_id(struct block_list *bl) {
	nullpo_retr(0, bl);

	// Players
	if (bl->type == BL_PC)
		return ((struct map_session_data *)bl)->status.party_id;

	// Monsters
	else if(bl->type == BL_MOB) {
		struct mob_data *md=(struct mob_data *)bl;
		if (md->master_id > 0)
			return -md->master_id;
		return -md->bl.id;
	}

	// Skills
	else if(bl->type == BL_SKILL)
		return ((struct skill_unit *)bl)->group->party_id;

	// To-Do: Add Pet returning?

	return 0;
}

/*==========================================
 *
 *-------------------------------------------
*/
int status_get_guild_id(struct block_list *bl) {

	nullpo_retr(0, bl);

	switch(bl->type) {
	// Players
	case BL_PC:
		return ((struct map_session_data *)bl)->status.guild_id;
	// Monsters
	case BL_MOB:
	{
		struct map_session_data *msd;
		struct mob_data *md = (struct mob_data *)bl;
		// Alchemist's mobs // 0: Nothing, 1: Cannibalize, 2-3: Spheremine
		if (md->state.special_mob_ai && (msd = map_id2sd(md->master_id)) != NULL)
			return msd->status.guild_id;
		else
			return md->guild_id; // Guilds' Guardians and Emperiums, otherwise = 0
	}
	 // Pets
	case BL_PET:
		return ((struct pet_data *)bl)->msd->status.guild_id;
	// Skills
	case BL_SKILL:
		return ((struct skill_unit *)bl)->group->guild_id;
	}

	return 0;
}

/*==========================================
 *
 *-------------------------------------------
*/
int status_get_race(struct block_list *bl) {

	nullpo_retr(0, bl);

	switch(bl->type) {
	// Monsters
	case BL_MOB:
		return mob_db[((struct mob_data *)bl)->class].race;
		break;
	// Players (Return Demi-Human -> 7)
	case BL_PC:
		return 7;
		break;
	// Pets
	case BL_PET:
		return mob_db[((struct pet_data *)bl)->class].race;
		break;
	}

	return 0;
}

/*==========================================
 *
 *-------------------------------------------
*/
int status_get_size(struct block_list *bl) {

	int retval;
	struct map_session_data *sd = (struct map_session_data *)bl;

	nullpo_retr(1, bl);

	switch (bl->type) {
		// Monsters
		case BL_MOB:
			retval = mob_db[((struct mob_data *)bl)->class].size;
			break;

		// Pets
		case BL_PET:
			retval = mob_db[((struct pet_data *)bl)->class].size;
			break;

		// Players
		case BL_PC:
		// Small for Baby Players
		// Medium for Normal Players
		// Large for Peco-riding Players (To-Do)
			retval = (pc_calc_upper(sd->status.class) == 2) ? 0 : 1;
			break;

		// Others (Returns medium)
		default:
			retval = 1;
			break;
	}

	return retval;
}

/*==========================================
 *
 *-------------------------------------------
*/
int status_get_mode(struct block_list *bl) {

	nullpo_retr(0x01, bl);

	// Monsters
	if (bl->type == BL_MOB)
		return mob_db[((struct mob_data *)bl)->class].mode;

	// Pets
	else if (bl->type == BL_PET)
		return mob_db[((struct pet_data *)bl)->class].mode;

	// Default is 0x01
	else
		return 0x01;
}

/*==========================================
 *
 *-------------------------------------------
*/
int status_get_race2(struct block_list *bl) {

	nullpo_retr(0, bl);

	// Monsters
	if (bl->type == BL_MOB)
		return mob_db[((struct mob_data *)bl)->class].race2;

	// Pets
	else if (bl->type == BL_PET)
		return mob_db[((struct pet_data *)bl)->class].race2;

	// Default is 0
	return 0;
}

/*==========================================
 *
 *-------------------------------------------
*/
int status_isdead(struct block_list *bl) {

	nullpo_retr(0, bl);

	// Monsters
	if (bl->type == BL_MOB)
		return ((struct mob_data *)bl)->state.state == MS_DEAD;

	// Players
	if (bl->type == BL_PC)
		return pc_isdead((struct map_session_data *)bl);

	// Pets cannot die
	// Default is 0
	return 0;
}

/*==========================================
 *
 *-------------------------------------------
*/
int status_isimmune(struct block_list *bl) {

	// Retreive Player Map Session data
	struct map_session_data *sd = (struct map_session_data *)bl;
	
	nullpo_retr(0, bl);

	// Players
	if (bl->type == BL_PC) {
		// No Magic Damage State (Golden Thief Bug Card)
		if (sd->special_state.no_magic_damage)
			return 1;
		
		// No Magic damage recieved in Wand of Hermode's Area of Effect
		if (sd->sc_count && sd->sc_data[SC_HERMODE].timer != -1)
			return 1;
	}	

	// Default is 0
	return 0;
}

/*==========================================
 *
 *-------------------------------------------
*/
struct status_change *status_get_sc_data(struct block_list *bl) {

	nullpo_retr(NULL, bl);

	// Monsters
	if (bl->type==BL_MOB)
		return ((struct mob_data*)bl)->sc_data;

	// Players
	else if(bl->type==BL_PC)
		return ((struct map_session_data*)bl)->sc_data;

	// Default
	return NULL;
}

/*==========================================
 *
 *-------------------------------------------
*/
short *status_get_sc_count(struct block_list *bl) {

	nullpo_retr(NULL, bl);

	// Monsters
	if (bl->type==BL_MOB)
		return &((struct mob_data*)bl)->sc_count;

	// Players
	else if(bl->type==BL_PC)
		return &((struct map_session_data*)bl)->sc_count;

	// Default
	return NULL;
}

/*==========================================
 *
 *-------------------------------------------
*/
short *status_get_opt1(struct block_list *bl) {

	nullpo_retr(0, bl);

	// Monsters
	if (bl->type == BL_MOB)
		return &((struct mob_data*)bl)->opt1;

	// Players
	else if (bl->type == BL_PC)
		return &((struct map_session_data*)bl)->opt1;

	// Non-Player Characters (NPCs)
	else if (bl->type == BL_NPC)
		return &((struct npc_data*)bl)->opt1;

	// Default
	return 0;
}

/*==========================================
 *
 *-------------------------------------------
*/
short *status_get_opt2(struct block_list *bl) {

	nullpo_retr(0, bl);

	// Monsters
	if (bl->type==BL_MOB)
		return &((struct mob_data*)bl)->opt2;

	// Players
	else if(bl->type==BL_PC)
		return &((struct map_session_data*)bl)->opt2;
	
	// Non-Player Characters (NPCs)
	else if(bl->type==BL_NPC)
		return &((struct npc_data*)bl)->opt2;

	// Default
	return 0;
}

/*==========================================
 *
 *-------------------------------------------
*/
short *status_get_opt3(struct block_list *bl) {

	nullpo_retr(0, bl);

	// Monsters
	if (bl->type==BL_MOB)
		return &((struct mob_data*)bl)->opt3;

	// Players
	else if(bl->type==BL_PC)
		return &((struct map_session_data*)bl)->opt3;

	// Non-Player Characters (NPCs)
	else if(bl->type==BL_NPC && (struct npc_data *)bl)
		return &((struct npc_data*)bl)->opt3;

	// Default
	return 0;
}

/*==========================================
 *
 *-------------------------------------------
*/
short *status_get_option(struct block_list *bl) {

	nullpo_retr(0, bl);

	// Monsters
	if (bl->type==BL_MOB)
		return &((struct mob_data*)bl)->option;

	// Players
	else if(bl->type==BL_PC)
		return &((struct map_session_data*)bl)->status.option;

	// Non-Player Characters (NPCs)
	else if(bl->type==BL_NPC)
		return &((struct npc_data*)bl)->option;

	// Default
	return 0;
}

/*==========================================
 *
 *-------------------------------------------
*/
int status_get_sc_def(struct block_list *bl, int type) {

	int sc_def;
	nullpo_retr(0, bl);

	// The following statuses are blocked by Golden Thief Bug Card and Wand of Hermode
	if (status_isimmune(bl)) {
		switch (type) {
			case SC_DECREASEAGI:
			case SC_SILENCE:
			case SC_COMA:
			case SC_INCREASEAGI:
			case SC_BLESSING:
			case SC_SLOWPOISON:
			case SC_IMPOSITIO:
			case SC_AETERNA:
			case SC_SUFFRAGIUM:
			case SC_BENEDICTIO:
			case SC_PROVIDENCE:
			case SC_KYRIE:
			case SC_ASSUMPTIO:
			case SC_ANGELUS:
			case SC_MAGNIFICAT:
			case SC_GLORIA:
			case SC_WINDWALK:
			case SC_MAGICROD:
			case SC_HALLUCINATION:
			case SC_STONE:
			case SC_QUAGMIRE:
			case SC_SUITON:
				return 10000;
		}
	}

	// Calculation
	switch (type) {
	case SP_MDEF1:	// Mdef
		sc_def = 100 - (3 + status_get_mdef(bl) + status_get_luk(bl) / 3);
		break;
	case SP_MDEF2:	// Int
		sc_def = 100 - (3 + status_get_int(bl) + status_get_luk(bl) / 3);
		break;
	case SP_DEF1:	// Def
		sc_def = 100 - (3 + status_get_def(bl) + status_get_luk(bl) / 3);
		break;
	case SP_DEF2:	// Vit
		sc_def = 100 - (3 + status_get_vit(bl) + status_get_luk(bl) / 3);
		break;
	case SP_LUK:	// Luk
		sc_def = 100 - (3 + status_get_luk(bl));
		break;

	case SC_STONE:
	case SC_FREEZE:
		sc_def = 100 - (3 + status_get_mdef(bl) + status_get_luk(bl) / 3);
		break;
	case SC_STUN:
	case SC_POISON:
	case SC_SILENCE:
		sc_def = 100 - (3 + status_get_vit(bl) + status_get_luk(bl) / 3);
		break;
	case SC_SLEEP:
	case SC_CONFUSION:
	case SC_BLIND:
		sc_def = 100 - (3 + status_get_int(bl) + status_get_luk(bl) / 3);
		break;
	case SC_CURSE:
		sc_def = 100 - (3 + status_get_luk(bl));
		break;

	default:
		sc_def = 100;
		break;
	}

	// Monsters
	if (bl->type == BL_MOB) {
		struct mob_data *md = (struct mob_data *)bl;
		if (md->class == 1288)
			return 0;
		if (sc_def < 50)
			sc_def = 50;
	}

	// Players
	else if (bl->type == BL_PC) {
		struct status_change* sc_data = status_get_sc_data(bl);
		if (sc_data && sc_data[SC_SCRESIST].timer != -1)
			return 0; // Immunity to all status
	}

	// Default
	return (sc_def < 0) ? 0 : sc_def;
}

/*==========================================
 *
 *-------------------------------------------
*/
int status_change_start(struct block_list *bl, int type, int val1, int val2, int val3, int val4, int tick, int flag) {

	struct map_session_data *sd = NULL;
	struct mob_data *md = NULL;
	struct status_change* sc_data;
	short *sc_count, *option, *opt1, *opt2, *opt3;
	int race, mode, elem;
	int scdef = 0;

	struct {
		unsigned calc : 1; // Re-calculate status_calc_pc
		unsigned send_opt : 1; // Send new option clif_changeoption
		unsigned undead_bl : 1; // Whether the object is undead race/ele or not
		unsigned dye_fix : 1; // Dye reset crash fix for SC_BUNSINJYUTSU
	} scflag;

	nullpo_retr(0, bl);

	switch(bl->type) {
		// Players
		case BL_PC:
			sd = (struct map_session_data *)bl;
			// If dead fail
			if (pc_isdead(sd))
				return 0;
			break;

		// Monsters
		case BL_MOB:
			md = (struct mob_data *)bl;
			// If dead fail
			if (status_isdead(bl))
				return 0;
			break;

		// Default (Fail)
		default:
			return 0;
	}

	memset(&scflag, 0, sizeof(scflag)); // Init scflag structure with 0's

	if (type < 0 || type >= SC_MAX) {
		return 0;
	}

	nullpo_retr(0, sc_data = status_get_sc_data(bl));
	nullpo_retr(0, sc_count = status_get_sc_count(bl));
	nullpo_retr(0, option = status_get_option(bl));
	nullpo_retr(0, opt1 = status_get_opt1(bl));
	nullpo_retr(0, opt2 = status_get_opt2(bl));
	nullpo_retr(0, opt3 = status_get_opt3(bl));

	race = status_get_race(bl);
	mode = status_get_mode(bl);
	elem = status_get_elem_type(bl);
	scflag.undead_bl = battle_check_undead(race, elem);

	// The only status change that works on Emperium (1288) is Safety Wall
	if (md && md->class == 1288 && type != SC_SAFETYWALL)
		return 0;

	switch(type) {
		case SC_AUTOBERSERK:
		case SC_READYSTORM:
		case SC_READYDOWN:
		case SC_READYTURN:
		case SC_READYCOUNTER:
		case SC_DODGE:
		case SC_FUSION:
			if (sc_data[type].timer != -1) {
				status_change_end(bl, type, -1);
				return 0;
			}
			break;
	}

	// Status change defense calculation
	switch(type) {
		case SC_STONE:
		case SC_FREEZE:
			scdef = 3 + status_get_mdef(bl) + status_get_luk(bl) / 3;
			break;
		case SC_STUN:
		case SC_SILENCE:
		case SC_POISON:
		case SC_DPOISON:
		case SC_BLEEDING:
			scdef = 3 + status_get_vit(bl) + status_get_luk(bl) / 3;
			break;
		case SC_SLEEP:
		case SC_BLIND:
			scdef = 3 + status_get_int(bl) + status_get_luk(bl) / 3;
			break;
		case SC_CURSE:
			scdef = 3 + status_get_luk(bl);
			break;

		default:
			scdef = 0;
	}

	if (scdef && sc_data[SC_SIEGFRIED].timer != -1)
		scdef += 50;

	// Immunity check
	if (scdef >= 100 && !(flag&8))
		return 0; // Total inmunity, cannot be inflicted

	// SC_STONE, SC_FREEZE, and SC_BLEED do not work on Undead targets
	if ((race == 1 || elem == 9) && !(flag&1) && (type == SC_STONE || type == SC_FREEZE || type == SC_BLEEDING))
		return 0;

	if (sd && !(flag&8)) {
		if (SC_STONE <= type && type <= SC_BLIND) {
			if (sd->reseff[type - SC_STONE] > 0 && rand() % 10000 < sd->reseff[type - SC_STONE]){
				return 0;
			}
		}
	}

	// Status changes that stop the source from walking
	switch(type) {
		case SC_STUN:
		case SC_SLEEP:
		case SC_STOP:
		case SC_ANKLE:
		case SC_SPIDERWEB:
		case SC_CLOSECONFINE:
		case SC_CLOSECONFINE2:
			battle_stopwalking(bl, 1);
	}

	// Status effects that don't affect boss monsters
	if ((mode&0x20 && !(flag&1))) {
		switch(type) {
			case SC_STONE:
			case SC_FREEZE:
			case SC_STUN:
			case SC_SLEEP:
			case SC_POISON:
			case SC_CURSE:
			case SC_SILENCE:
			case SC_CONFUSION:
			case SC_BLIND:
			case SC_BLEEDING:
			case SC_DPOISON:
			case SC_PROVOKE:
			case SC_QUAGMIRE:
			case SC_DECREASEAGI:
			case SC_STOP:
			case SC_SIGNUMCRUCIS:
			case SC_ROKISWEIL:
			case SC_COMA:
			case SC_GRAVITATION:
			case SC_SUITON:
			case SC_STRIPWEAPON:
			case SC_STRIPSHIELD:
			case SC_STRIPARMOR:
			case SC_STRIPHELM:
				return 0;
				break;
			case SC_BLESSING:
				if (scflag.undead_bl || race == 6)
					return 0;
				break;
		}
	}

	// Another check calculation
	if (sc_data[type].timer != -1) {
		switch(type) {
			case SC_ADRENALINE:
			case SC_WEAPONPERFECTION:
			case SC_OVERTHRUST:
				if (sc_data[type].val2 && !val2)
					return 0;
				break;
			case SC_STUN:
			case SC_SLEEP:
			case SC_POISON:
			case SC_CURSE:
			case SC_SILENCE:
			case SC_CONFUSION:
			case SC_BLIND:
			case SC_BLEEDING:
			case SC_DPOISON:
			case SC_COMBO:
			case SC_CLOSECONFINE2:
				return 0;
			case SC_DANCING:
			case SC_DEVOTION:
			case SC_ASPDPOTION0:
			case SC_ASPDPOTION1:
			case SC_ASPDPOTION2:
			case SC_ASPDPOTION3:
			case SC_ATKPOT:
			case SC_MATKPOT:
			case SC_CONCENTRATION:
			case SC_MAGNUM:
				break;
			case SC_GOSPEL:
				if (sc_data[type].val4 == BCT_SELF)
					return 0;
				break;
			default:
				if (sc_data[type].val1 > val1)
					return 0;
		}

		(*sc_count)--;
		delete_timer(sc_data[type].timer, status_change_timer);
		sc_data[type].timer = -1;
	}

	// Main status change calculation check
	// To-Do: Organize list
	switch(type) {
		case SC_ONEHAND:
			if (sc_data[SC_ASPDPOTION0].timer != -1)
				status_change_end(bl, SC_ASPDPOTION0, -1);
			if (sc_data[SC_ASPDPOTION1].timer != -1)
				status_change_end(bl,SC_ASPDPOTION1,-1);
			if (sc_data[SC_ASPDPOTION2].timer != -1)
				status_change_end(bl, SC_ASPDPOTION2, -1);
			if (sc_data[SC_ASPDPOTION3].timer != -1)
				status_change_end(bl, SC_ASPDPOTION3, -1);
			scflag.calc = 1;
			*opt3 |= 1;
			break;
		case SC_INCREASING:
		case SC_GATLINGFEVER:
			scflag.calc = 1;
			break;
		case SC_FLING:
			if (bl->type == BL_PC)
				val2 = 0; // No armor reduction to players
			else
				val2 = 5*val1; // Def reduction
			val3 = 5*val1; // Def2 reduction
			scflag.calc = 1;
			break;
		case SC_UTSUSEMI:
			val2=(val1+1)/2;
			val3=skill_get_blewcount(NJ_UTSUSEMI, val1);
			break;
		case SC_TATAMIGAESHI:
			tick = 3000;
			break;
		case SC_BUNSINJYUTSU:
			val2=(val1+1)/2;
			scflag.dye_fix = 1;
			break;
		case SC_SPIRIT:
			scflag.calc = 1;
			*opt3 |= 32768;
			// Chance to reset Super Novice die counter with Spirit of the Super Novice
			if (sd && sc_data[SC_SPIRIT].timer != -1 && sc_data[SC_SPIRIT].val2 == SL_SUPERNOVICE && sd->status.base_level >= 90) {
				if (rand()%100 <= 1)
					sd->status.die_counter = 0;
			}
			break;
		case SC_KAITE:
			if (val1 >= 5)
				val2 = 2;
			else
				val2 = 1;
			// Kaite and Assumptio do not stack
			if (sc_data[SC_ASSUMPTIO].timer != -1)
				status_change_end(bl, SC_ASSUMPTIO, -1);
			break;
		case SC_SWOO:
			if (mode&0x20 && !(flag&1))
				tick /= 5;
			scflag.calc = 1;
			break;
		case SC_SKE:
			scflag.calc = 1;
			break;
		case SC_SKA:
			val2 = tick/1000;  
			val3 = rand()%100; // Def changes randomly every second...  
			tick = 1000;  
			scflag.calc = 1;
			break;
		case SC_PROVOKE:
			scflag.calc = 1;
			if (tick <= 0) tick = 1000;
			break;
		case SC_ENDURE:
			if (tick <= 0)
			tick = 1000 * 60;
			scflag.calc = 1;
			val2 = 7;
			if (sd) {    
				struct map_session_data *tsd;
				int i;
				for (i = 0; i < 5; i++) {    
					if (sd->dev.val1[i] && (tsd = map_id2sd(sd->dev.val1[i])))
						status_change_start(&tsd->bl,SC_ENDURE,0,0,0,0,tick,1);
				}
			}
			break;
		case SC_AUTOBERSERK:
			if (!(flag&4))
				tick = 60 * 1000;
			if (sd && sd->status.hp < sd->status.max_hp>>2 &&
				(sc_data[SC_PROVOKE].timer == -1 || sc_data[SC_PROVOKE].val2 == 0))
				status_change_start(bl, SC_PROVOKE, 10, 1, 0, 0, 0, 0);
			break;
		case SC_CONCENTRATE:
		case SC_RUN:
		case SC_SPURT:
			scflag.calc = 1;
			break;
		case SC_NEN:
			*opt3 |= 2;
			scflag.calc = 1;
			break;
	case SC_FUSION:
		if (sc_data[SC_SPIRIT].timer != -1)
			status_change_end(bl, SC_SPIRIT, -1);
			scflag.calc = 1;
		break;
	case SC_ADJUSTMENT:
		if (sc_data[SC_MADNESSCANCEL].timer != -1)
			status_change_end(bl, SC_MADNESSCANCEL, -1);
		scflag.calc = 1;
		break;
	case SC_MADNESSCANCEL:
		if (sc_data[SC_ADJUSTMENT].timer != -1)
			status_change_end(bl, SC_ADJUSTMENT, -1);
		scflag.calc = 1;
		break;
		case SC_READYSTORM:
		case SC_READYDOWN:
		case SC_READYTURN:
		case SC_READYCOUNTER:
		case SC_READYDODGE:
			if (flag&4)
				break;
		case SC_BLESSING:
			if (bl->type == BL_PC || (!scflag.undead_bl && race != 6)) {
				if (sc_data[SC_CURSE].timer != -1 )
					status_change_end(bl, SC_CURSE, -1);
				if (sc_data[SC_STONE].timer != -1 && sc_data[SC_STONE].val2 == 0)
					status_change_end(bl, SC_STONE, -1);
			}
			scflag.calc = 1;
			break;
		case SC_ANGELUS:
			scflag.calc = 1;
			break;
		case SC_INCREASEAGI:
			scflag.calc = 1;
			if (sc_data[SC_DECREASEAGI].timer != -1 )
				status_change_end(bl,SC_DECREASEAGI,-1);
			break;
		case SC_DECREASEAGI:
			if (bl->type == BL_PC)
				tick >>= 1; // Half duration on players
			scflag.calc = 1;
			if (*sc_count > 0) {
				if (sc_data[SC_INCREASEAGI].timer != -1 )
					status_change_end(bl, SC_INCREASEAGI, -1);
				if (sc_data[SC_ADRENALINE].timer != -1 )
					status_change_end(bl, SC_ADRENALINE, -1);
				if (sc_data[SC_ADRENALINE2].timer !=-1 )
					status_change_end(bl, SC_ADRENALINE2,-1);
				if (sc_data[SC_SPEARQUICKEN].timer != -1 )
					status_change_end(bl, SC_SPEARQUICKEN, -1);
				if (sc_data[SC_TWOHANDQUICKEN].timer != -1 )
					status_change_end(bl, SC_TWOHANDQUICKEN, -1);
				if (sc_data[SC_CARTBOOST].timer !=-1 )
					status_change_end(bl, SC_CARTBOOST, -1);
				if (sc_data[SC_ONEHAND].timer !=-1 )
					status_change_end(bl, SC_ONEHAND, -1);
			}
			break;
		case SC_SIGNUMCRUCIS:
			scflag.calc = 1;
			val2 = 10 + val1*2;
			if (!(flag&4))
				tick = 600*1000;
			clif_emotion(bl,4);
			break;
		case SC_SLOWPOISON:
			if (sc_data[SC_POISON].timer == -1 && sc_data[SC_DPOISON].timer == -1)
				return 0;
			break;
		case SC_TWOHANDQUICKEN:
			if (sc_data[SC_DECREASEAGI].timer != -1)
				return 0;
			*opt3 |= 1;
			scflag.calc = 1;
			break;
		case SC_ADRENALINE:
			if (sd && !(skill_get_weapontype(BS_ADRENALINE)&(1<<sd->status.weapon)))
				return 0;
			if (sc_data[SC_DECREASEAGI].timer != -1)
				return 0;
			scflag.calc = 1;
			break;
		case SC_OVERTHRUST:
			if (sc_data[SC_MAXOVERTHRUST].timer != -1)
				return 0; // Overthrust should not take effect if MAXOVERTHRUST is active
			*opt3 |= 2;
			break;
		case SC_MAXOVERTHRUST:
			if (sc_data[SC_OVERTHRUST].timer != -1)	// Cancels normal Overthrust
				status_change_end(bl, SC_OVERTHRUST, -1);
			break;
		case SC_MAXIMIZEPOWER:
			if (!(flag&4)) {
				if (bl->type == BL_PC)
					val2 = tick;
				else
					tick = 5000 * val1;
			}
			break;
		case SC_ENCPOISON:
			scflag.calc = 1;
			val2=(((val1 - 1) / 2) + 3)*100;
			skill_enchant_elemental_end(bl, SC_ENCPOISON);
			break;
		case SC_EDP:
			val2 = val1 + 2; // Chance to poison enemies
			val3 = 50*(val1+1); // Increased damage
			scflag.calc = 1;
			break;
		case SC_POISONREACT:
			if (!(flag&4))
				val2 = (val1 >> 1) + val1%2;
			break;
		case SC_IMPOSITIO:
			scflag.calc = 1;
			break;
		case SC_ASPERSIO:
			skill_enchant_elemental_end(bl, SC_ASPERSIO);
			break;
		case SC_AETERNA:
			if (sc_data[SC_STONE].timer != -1 || sc_data[SC_FREEZE].timer != -1)
				return 0; // Should not take effect if target is Frozen or Stone Cursed
			break;
		case SC_ENERGYCOAT:
			*opt3 |= 4;
			break;
		case SC_MAGICROD:
			val2 = val1 * 20;
			break;
		case SC_KYRIE:
			if (!(flag&4)) {
				val2 = status_get_max_hp(bl) * (val1 * 2 + 10) / 100;
				val3 = (val1 / 2 + 5);
				if (sc_data[SC_ASSUMPTIO].timer!=-1 )
					status_change_end(bl,SC_ASSUMPTIO,-1);
			}
			break;
		case SC_MINDBREAKER:
			scflag.calc = 1;
			if (tick <= 0) tick = 1000;
		case SC_GLORIA:
			scflag.calc = 1;
			break;
		case SC_LOUD:
			scflag.calc = 1;
			break;
		case SC_TRICKDEAD:
			if (sd)
				pc_stopattack(sd);
			break;
		case SC_QUAGMIRE:
			scflag.calc = 1;
			if (*sc_count > 0) {
				if (sc_data[SC_CONCENTRATE].timer != -1 )
					status_change_end(bl, SC_CONCENTRATE, -1);
				if (sc_data[SC_INCREASEAGI].timer != -1 )
					status_change_end(bl, SC_INCREASEAGI, -1);
				if (sc_data[SC_TWOHANDQUICKEN].timer != -1 )
					status_change_end(bl, SC_TWOHANDQUICKEN,-1);
				if (sc_data[SC_SPEARQUICKEN].timer != -1 )
					status_change_end(bl, SC_SPEARQUICKEN, -1);
				if (sc_data[SC_ADRENALINE].timer != -1 )
					status_change_end(bl, SC_ADRENALINE, -1);
				if (sc_data[SC_ADRENALINE2].timer != -1 )
					status_change_end(bl, SC_ADRENALINE2, -1);
				if (sc_data[SC_LOUD].timer != -1 )
					status_change_end(bl, SC_LOUD, -1);
				if (sc_data[SC_TRUESIGHT].timer != -1 )
					status_change_end(bl, SC_TRUESIGHT, -1);
				if (sc_data[SC_WINDWALK].timer != -1 )
					status_change_end(bl, SC_WINDWALK, -1);
				if (sc_data[SC_CARTBOOST].timer != -1 )
					status_change_end(bl, SC_CARTBOOST,-1);
			}
			break;
		case SC_MAGICPOWER:
			scflag.calc = 1;
			val2 = 1;
			break;
		case SC_SACRIFICE:
			if (!(flag&4))
				val2 = 5;
			break;
		case SC_FIREWEAPON:
		case SC_WATERWEAPON:
		case SC_WINDWEAPON:
		case SC_EARTHWEAPON:
		case SC_SHADOWWEAPON:
		case SC_GHOSTWEAPON:
			skill_enchant_elemental_end(bl, type);
			break;
		case SC_DEVOTION:
			if (!(flag&4))
				scflag.calc = 1;
			struct map_session_data *src;
			if ((src = map_id2sd(val1)) && src->sc_count) {    
				int type2 = SC_AUTOGUARD;
				if (src->sc_data[type2].timer != -1)
					status_change_start(bl,type2,src->sc_data[type2].val1,0,0,0,tick,1);
				type2 = SC_ENDURE;
				if (src->sc_data[type2].timer != -1)
					status_change_start(bl,type2,0,0,0,0,tick,1);
				type2 = SC_DEFENDER;
				if (src->sc_data[type2].timer != -1)
					status_change_start(bl,type2,src->sc_data[type2].val1,src->sc_data[type2].val2,0,0,tick,1);
				type2 = SC_REFLECTSHIELD;
				if (src->sc_data[type2].timer != -1)
					status_change_start(bl,type2,src->sc_data[type2].val1,0,0,0,tick,1);  
				type2 = SC_SHRINK;
				if (src->sc_data[type2].timer != -1)
					status_change_start(bl,type2,src->sc_data[type2].val1,0,0,0,tick,1);
			}
			break;
		case SC_PROVIDENCE:
			scflag.calc = 1;
			val2=val1*5;
			break;
		case SC_REFLECTSHIELD:
			val2=10+val1*3;
			if (sd) {    
				struct map_session_data *tsd;
				int i;
				for (i = 0; i < 5; i++)
				{
					if (sd->dev.val1[i] && (tsd = map_id2sd(sd->dev.val1[i])))
						status_change_start(&tsd->bl,SC_REFLECTSHIELD,val1,0,0,0,tick,1);
				}
			}
			break;
		case SC_STRIPWEAPON:
			if (val2==0) val2=90;
			break;
		case SC_STRIPSHIELD:
			if (val2==0) val2=85;
			break;
		case SC_AUTOSPELL:
			val4 = 5 + val1*2;
			break;
		case SC_VOLCANO:
			scflag.calc = 1;
			val3 = val1*10;
			val4 = val1>=5?20: (val1==4?19: (val1==3?17: ( val1==2?14:10 ) ) );
			break;
		case SC_DELUGE:
			scflag.calc = 1;
			val3 = val1>=5?15: (val1==4?14: (val1==3?12: ( val1==2?9:5 ) ) );
			val4 = val1>=5?20: (val1==4?19: (val1==3?17: ( val1==2?14:10 ) ) );
			break;
		case SC_SUITON:
			scflag.calc = 1;
			// No penalties received if job is a Ninja
			if (!val2 || (sd && sd->status.class == JOB_NINJA)) {
				val2 = 0; // Agi
				val3 = 0; // Walk speed
				break;
			}
			val3 = 50;
			val2 = 3 * ((val1 + 1) / 3);
			if (val1 > 4)
				val2--;
			break;
		case SC_VIOLENTGALE:
			scflag.calc = 1;
			val3 = val1*3;
			val4 = val1>=5?20: (val1==4?19: (val1==3?17: ( val1==2?14:10 ) ) );
			break;
		case SC_SPEARQUICKEN:
			scflag.calc = 1;
			val2 = 20+val1;
			*opt3 |= 1;
			break;
		case SC_COMBO:
			switch(val1) { // Skill ID
				case TK_STORMKICK:
					clif_skill_nodamage(bl, bl, TK_READYSTORM, 1, 1);
					if (sd) sd->attackabletime = gettick_cache + tick;
					break;
				case TK_DOWNKICK:
					clif_skill_nodamage(bl, bl, TK_READYDOWN, 1, 1);
					if (sd) sd->attackabletime = gettick_cache + tick;
					break;
				case TK_TURNKICK:
					clif_skill_nodamage(bl, bl, TK_READYTURN, 1, 1);
					if (sd) sd->attackabletime = gettick_cache + tick;
					break;
				case TK_COUNTER:
					clif_skill_nodamage(bl, bl, TK_READYCOUNTER, 1, 1);
					if (sd) sd->attackabletime = gettick_cache + tick;
					break;
			}
			break;
		case SC_BLADESTOP_WAIT:
			break;
		case SC_BLADESTOP:
			if (flag&4)
				break;
			if (val2 == 2)
				clif_bladestop((struct block_list *)val3, (struct block_list *)val4, 1);
			*opt3 |= 32;
			break;
		case SC_LULLABY:
			val2 = 11;
			break;
		case SC_RICHMANKIM:
			break;
		case SC_ETERNALCHAOS:
			scflag.calc = 1;
			break;
		case SC_DRUMBATTLE:
			scflag.calc = 1;
			val2 = (val1+1)*25;
			val3 = (val1+1)*2;
			break;
		case SC_NIBELUNGEN:
			scflag.calc = 1;
			val3 = (val1+2)*25;
			break;
		case SC_SIEGFRIED:
			scflag.calc = 1;
			val2 = 55 + val1*5;
			val3 = val1*10;
			break;
		case SC_DISSONANCE:
			val2 = 10;
			break;
		case SC_WHISTLE:
			scflag.calc = 1;
			break;
		case SC_ASSNCROS:
			scflag.calc = 1;
			break;
		case SC_POEMBRAGI:
			break;
		case SC_APPLEIDUN:
			scflag.calc = 1;
			break;
		case SC_UGLYDANCE:
			val2 = 10;
			break;
		case SC_HUMMING:
			scflag.calc = 1;
			break;
		case SC_DONTFORGETME:
			scflag.calc = 1;
			if (*sc_count > 0) {
				if (sc_data[SC_INCREASEAGI].timer!=-1 )
					status_change_end(bl,SC_INCREASEAGI,-1);
				if (sc_data[SC_TWOHANDQUICKEN].timer!=-1 )
					status_change_end(bl,SC_TWOHANDQUICKEN,-1);
				if (sc_data[SC_ONEHAND].timer!=-1 )
					status_change_end(bl,SC_ONEHAND,-1);
				if (sc_data[SC_SPEARQUICKEN].timer!=-1 )
					status_change_end(bl,SC_SPEARQUICKEN,-1);
				if (sc_data[SC_ADRENALINE].timer!=-1 )
					status_change_end(bl,SC_ADRENALINE,-1);
				if (sc_data[SC_ASSNCROS].timer!=-1 )
					status_change_end(bl,SC_ASSNCROS,-1);
				if (sc_data[SC_TRUESIGHT].timer!=-1 )
					status_change_end(bl,SC_TRUESIGHT,-1);
				if (sc_data[SC_WINDWALK].timer != -1)
					status_change_end(bl, SC_WINDWALK, -1);
				if (sc_data[SC_CARTBOOST].timer != -1)
					status_change_end(bl, SC_CARTBOOST, -1);
			}
			break;
		case SC_FORTUNE:
			scflag.calc = 1;
			break;
		case SC_SERVICEFORYOU:
			scflag.calc = 1;
			break;
		case SC_MOONLIT:
			val2 = bl->id;
			*opt3 |= 512;
			break;
		case SC_DANCING:
			scflag.calc = 1;
			if (!(flag&4)) {
				if (val1 == CG_MOONLIT)
					status_change_start(bl, SkillStatusChangeTable[CG_MOONLIT], 0, 0, 0, 0, tick, 0);
				val3= tick / 1000;
				tick = 1000;
			}
			break;
		case SC_EXPLOSIONSPIRITS:
			scflag.calc = 1;
			val2 = 75 + 25*val1;
			*opt3 |= 8;
			break;
		case SC_STEELBODY:
			scflag.calc = 1;
			*opt3 |= 16;
			break;
		case SC_EXTREMITYFIST:
			break;
		case SC_AUTOCOUNTER:
			val3 = val4 = 0;
			break;
		case SC_ASPDPOTION0:
		case SC_ASPDPOTION1:
		case SC_ASPDPOTION2:
		case SC_ASPDPOTION3:
			scflag.calc = 1;
			if (!(flag&4))
				val2 = 5 * (2 + type - SC_ASPDPOTION0);
			break;
		case SC_ATKPOT:
		case SC_MATKPOT:
			scflag.calc = 1;
			break;
		case SC_WEDDING:
		{
			time_t timer;
			scflag.calc = 1;
			tick = 10000;
			if (!val2)
				val2 = time(&timer);
		}
		break;
		case SC_XMAS:
		{
			time_t timer;
			tick = 10000;
			if (!val2)
				val2 = time(&timer);
		}
		break;
		case SC_NOCHAT:
		{
			time_t timer;
			if (!battle_config.muting_players)
				break;
			if (!(flag&4))
				tick = 60000;
			if (!val2)
				val2 = time(&timer);
			if (sd) {
				clif_updatestatus(sd, SP_MANNER);
				chrif_save(sd); // Do pc_makesavestatus and save storage + account_reg/account_reg2 too
			}
		}
		break;
		case SC_SELFDESTRUCTION:
			clif_skillcasting(bl,bl->id, bl->id,0,0,331,skill_get_time(val2,val1));
			val3 = tick / 1000;
			tick = 1000;
			break;

		case SC_STONE:
			if (!(flag&2)) {
				int sc_def = status_get_mdef(bl)*200;
				tick = tick - sc_def;
			}
			if (!(flag&4))
				val3 = tick / 1000;
			if (val3 < 1) val3 = 1;
			if (!(flag&4))
				tick = 5000;
			val2 = 1;
			break;
		case SC_SLEEP:
			if (!(flag&4)) {
				tick = 30000;
			}
			break;
		case SC_FREEZE:
			if (!(flag&2)) {
				int sc_def = 100 - status_get_mdef(bl);
				tick = tick * sc_def / 100;
			}
			break;
		case SC_STUN:
			if (!(flag&2)) {
				int sc_def = status_get_sc_def_vit(bl);
				tick = tick * sc_def / 100;
			}
			break;
		case SC_DPOISON:
			scflag.calc = 1;
			int mhp = status_get_max_hp(bl);
			int hp = status_get_hp(bl);
			if (hp > mhp>>2) {
				if (sd) {
					int diff = mhp * 10 / 100;
					if (hp - diff < mhp >> 2)
						diff = hp - (mhp >> 2);
					pc_heal(sd, -diff, 0);
				} else if (bl->type == BL_MOB) {
					struct mob_data *md = (struct mob_data *)bl;
					hp -= mhp * 15 / 100;
					if (hp > mhp >> 2)
						md->hp = hp;
					else
						md->hp = mhp >> 2;
				}
			}
			if (!(flag&2)) {
				int sc_def = 100 - (status_get_vit(bl) + status_get_luk(bl)/5);
				tick = tick * sc_def / 100;
			}
			if (!(flag&4))
				val3 = tick / 1000;
			if (val3 < 1) val3 = 1;
			if (!(flag&4))
				tick = 1000;
			break;
		case SC_POISON:
			scflag.calc = 1;
			if (!(flag&2)) {
				int sc_def = 100 - (status_get_vit(bl) + status_get_luk(bl)/5);
				tick = tick * sc_def / 100;
			}
			if (!(flag&4))
				val3 = tick / 1000;
			if (val3 < 1) val3 = 1;
			if (!(flag&4))
				tick = 1000;
			break;
		case SC_SILENCE:
			if (sc_data[SC_GOSPEL].timer != -1 && sc_data[SC_GOSPEL].val4 == BCT_SELF) {
				status_change_end(bl, SC_GOSPEL, -1);
				break;
			}
			if (!(flag&2)) {
				int sc_def = 100 - status_get_vit(bl);
				tick = tick * sc_def / 100;
			}
			break;
		case SC_CONFUSION:
			if (!(flag&4)) {
				val2 = tick;
				tick = 100;
			}
			break;
		case SC_BLIND:
			scflag.calc = 1;
			if (!(flag&4) && tick < 1000)
				tick = 30000;
			if (!(flag&2)) {
				int sc_def = status_get_lv(bl) / 10 + status_get_int(bl) / 15;
				tick = 30000 - sc_def;
			}
			break;
		case SC_CURSE:
			scflag.calc = 1;
			if (!(flag&2)) {
				int sc_def = 100 - status_get_vit(bl);
				tick = tick * sc_def / 100;
			}
			break;
		case SC_HIDING:
			scflag.calc = 1;
			if (bl->type == BL_PC && !(flag&4)) {
				val2 = tick / 1000;
				tick = 1000;
			}
			if (sc_data[SC_CLOSECONFINE].timer != -1)
				status_change_end(bl, SC_CLOSECONFINE, -1);
			if (sc_data[SC_CLOSECONFINE2].timer != -1)
				status_change_end(bl, SC_CLOSECONFINE2, -1);
			break;
		case SC_CHASEWALK:
		case SC_CLOAKING:
			if (flag&4)
				break;
			if (bl->type == BL_PC) {
				scflag.calc = 1;
				val2 = tick;
				val3 = type==SC_CLOAKING ? 130-val1*3 : 135-val1*5;
			}
			else
				tick = 5000 * val1;
			break;
		case SC_SIGHT:
		case SC_RUWACH:
		case SC_SIGHTBLASTER:
			if (flag&4)
				break;
			val2 = tick/250;
			tick = 10;
			break;
		case SC_SAFETYWALL:
		case SC_PNEUMA:
			if (flag&4)
				break;
			tick=((struct skill_unit *)val2)->group->limit;
			break;
		case SC_ANKLE:
		case SC_STOP:
		case SC_SCRESIST:
			break;
        case SC_SHRINK:
			if (sd) {
				struct map_session_data *tsd;
				register int i;
				for(i = 0; i < 5; i++) {
					if ((sd->dev.val1[i] && (tsd = map_id2sd(sd->dev.val1[i]))))
						status_change_start(&tsd->bl, SC_SHRINK, val1, 0, 0, 0, tick, 1);
				}
			}
			break;
		case SC_RIDING:
			scflag.calc = 1;
			tick = 600*1000;
			break;
		case SC_FALCON:
		case SC_WEIGHT50:
		case SC_WEIGHT90:
		case SC_BROKENWEAPON:
		case SC_BROKENARMOR:
			if (flag&4)
				break;
			tick = 600 * 1000;
			break;
		case SC_AUTOGUARD:
		{
			int i, t;
			for(i = val2 = 0; i < val1; i++)
			{
				t = 5 - (i >> 1);
				val2 += (t < 0) ? 1 : t;
			}
			if (sd)
			{
				struct map_session_data *tsd;
				int i;
				for (i = 0; i < 5; i++)
				{
					if (sd->dev.val1[i] && (tsd = map_id2sd(sd->dev.val1[i])))
						status_change_start(&tsd->bl,SC_AUTOGUARD,val1,0,0,0,tick,1);
				}
			}
		}
		break;
		case SC_DEFENDER:
			scflag.calc = 1;
			if (!flag)
				val2 = 5 + val1 * 15;
			if (sd) {
				struct map_session_data *tsd;
				register int i;
				for(i = 0; i < 5; i++)
				{
					if (sd->dev.val1[i] && (tsd = map_id2sd(sd->dev.val1[i])))
						status_change_start(&tsd->bl,SC_DEFENDER,val1,val2,0,0,tick,1);
				}
			}
			break;
		case SC_KEEPING:
		case SC_BARRIER:
			scflag.calc = 1;
		case SC_HALLUCINATION:
			break;
		case SC_CONCENTRATION:
			*opt3 |= 1;
			scflag.calc = 1;
			break;
		case SC_TENSIONRELAX:
			if (flag&4)
				break;
			if (bl->type == BL_PC)
				tick = 10000;
			break;
		case SC_PARRYING:
			val2 = 20 + val1 * 3;
			break;
		case SC_WINDWALK:
			scflag.calc = 1;
			val2 = (val1 / 2);
			break;
		case SC_JOINTBEAT:
			scflag.calc = 1;
			val2 = rand() % 6 + 1;
			if (val2 == 6)
				status_change_start(bl, SC_BLEEDING, val1, 0, 0, 0, skill_get_time2(type, val1), 0);
			break;
		case SC_BERSERK:
			*opt3 |= 128;
			if (!(flag&4))
				tick = 10000;
			scflag.calc = 1;
			break;
		case SC_ASSUMPTIO:
			// Assumptio and Kyrie do not stack
			if (sc_data[SC_KYRIE].timer != -1)
				status_change_end(bl, SC_KYRIE, -1);
			// Assumptio and Kaite do not stack
			if (sc_data[SC_KAITE].timer != -1)
				status_change_end(bl, SC_KAITE, -1);
			*opt3 |= 2048;
			break;
		case SC_GOSPEL:
			if (val4 == BCT_SELF) {
				val2 = tick;
				tick = 10000;
				status_change_clear_buffs(bl);
				status_change_clear_debuffs(bl);
			}
			break;
		case SC_MARIONETTE:
		case SC_MARIONETTE2:
			if (flag&4)
				break;
			val2 = tick;
			if (!val3)
				return 0;
			tick = 1000;
			scflag.calc = 1;
			*opt3 |= 1024;
			break;
		case SC_MELTDOWN:
			scflag.calc = 1;
			break;
		case SC_CARTBOOST:
			if (sc_data[SC_DECREASEAGI].timer != -1) {
				status_change_end(bl, SC_DECREASEAGI, -1);
				return 0;
			}
		case SC_TRUESIGHT:
		case SC_SPIDERWEB:
			scflag.calc = 1;
			break;
		case SC_REJECTSWORD:
			val2 = 3;
			val3 = 0;
			break;
		case SC_MEMORIZE:
			val2 = 5; // Memorize is supposed to reduce the cast time of the next 5 spells by half
			break;
		case SC_GRAVITATION:
			if (val3 != BCT_SELF)
				scflag.calc = 1;
			break;
		case SC_HERMODE:
			status_change_clear_buffs(bl);
			break;
		case SC_COMA:
			battle_damage(NULL, bl, status_get_hp(bl)-1, 0);
			return 0;
		case SC_FOGWALL:
			val2 = 75;
			scflag.calc = 1;
			break;
		case SC_BLEEDING:
			if (!(flag&2)) {
				int sc_def = 100 - (status_get_lv(bl) / 5 + status_get_vit(bl));
				tick = tick * sc_def / 100;
			}
			if (!(flag&4) && tick < 10000)
				tick = 10000;
			val4 = tick;
			tick = 10000;
			break;
		case SC_SLOWDOWN:
		case SC_SPEEDUP0:
		case SC_INCSTR:
		case SC_INCAGI:
		case SC_INCDEX:
		case SC_INCALLSTATUS:
		case SC_INCHIT:
		case SC_INCFLEE:
		case SC_INCMHPRATE:
		case SC_INCMSPRATE:
		case SC_INCMATKRATE:
		case SC_INCATKRATE:
		case SC_INCDEFRATE:
		case SC_INCHITRATE:
		case SC_INCFLEERATE:
		case SC_INCASPDRATE:
			scflag.calc = 1;
			break;
		case SC_STRFOOD:
		case SC_AGIFOOD:
		case SC_VITFOOD:
		case SC_INTFOOD:
		case SC_DEXFOOD:
		case SC_LUKFOOD:
		case SC_HITFOOD:
		case SC_FLEEFOOD:
		case SC_BATKFOOD:
		case SC_WATKFOOD:
		case SC_MATKFOOD:
			scflag.calc = 1;
			break;
		case SC_REGENERATION:
			val1 = 2;
		case SC_BATTLEORDERS:
			if (!(flag&4))
				tick = 60000;
			scflag.calc = 1;
			break;
		case SC_GUILDAURA:
			tick = 1000;
			scflag.calc = 1;
			break;
		case SC_DOUBLE:
			break;
		case SC_INVINCIBLE:
			scflag.calc = 1;
			break;
		default:
			break;
	}
	// End of main status calculation check

	if (bl->type == BL_PC)
	{
#ifdef USE_SQL
		if (flag&4)
			clif_status_load(sd, StatusIconTable[type]); // Sending to owner since they aren't in the map yet
#endif
		clif_status_change(bl, StatusIconTable[type], 1);
	}

	// Option calculation check
	switch(type) {
		case SC_STONE:
		case SC_FREEZE:
		case SC_STUN:
		case SC_SLEEP:

			battle_stopattack(bl);
			skill_stop_dancing(bl,0);
			int i;
			for(i = SC_STONE; i <= SC_SLEEP; i++) {
				if (sc_data[i].timer != -1){
					(*sc_count)--;
					delete_timer(sc_data[i].timer, status_change_timer);
					sc_data[i].timer = -1;
				}
			}
			if (type == SC_STONE)
				*opt1 = 6;
			else
				*opt1 = type - SC_STONE + 1;
			scflag.send_opt = 1;
			break;
		case SC_POISON:
		case SC_CURSE:
		case SC_SILENCE:
		case SC_CONFUSION:
		case SC_BLIND:
			*opt2 |= 1<<(type-SC_POISON);
			scflag.send_opt = 1;
			break;
		case SC_DPOISON:
			*opt2 |= 0x080;
			scflag.send_opt = 1;
			break;
		case SC_SIGNUMCRUCIS:
			*opt2 |= 0x40;
			scflag.send_opt = 1;
			break;
		case SC_HIDING:
		case SC_CLOAKING:
			battle_stopattack(bl);
			*option |= ((type == SC_HIDING) ? 2: 4);
			scflag.send_opt = 1;
			break;
		case SC_CHASEWALK:
			battle_stopattack(bl);
			*option |= 16388;
			scflag.send_opt = 1;
			break;
		case SC_SIGHT:
			*option |= 1;
			scflag.send_opt = 1;
			break;
		case SC_RUWACH:
			*option |= 8192;
			scflag.send_opt = 1;
			break;
		case SC_WEDDING:
			*option |= 4096;
			scflag.send_opt = 1;
			break;
		case SC_SKA:
			*opt3 |= 16;
			scflag.send_opt = 0;
			break;
		case SC_SKE:
			*opt3 |= 4;
			scflag.send_opt = 0;
			break;
		case SC_SWOO:
			*opt3 |= 2;
			scflag.send_opt = 0;
			break;
	}

	// Hiding/Cloaking/Chase Walk dispelled if the following statuses exist
	switch(type) {
		case SC_STONE:
		case SC_FREEZE:
		case SC_STUN:
		case SC_SLEEP:
		case SC_SILENCE:
			if (sc_data && sc_data[SC_HIDING].timer != -1)
				status_change_end(bl, SC_HIDING, -1);
			else if (sc_data && sc_data[SC_CLOAKING].timer != -1)
				status_change_end(bl, SC_CLOAKING, -1);
			else if (sc_data && sc_data[SC_CHASEWALK].timer != -1)
				status_change_end(bl, SC_CHASEWALK, -1);
			break;
	}

	if (scflag.send_opt)
		clif_changeoption(bl);

	if (scflag.dye_fix) {
		if (sd && sd->status.clothes_color) {
			val4 = sd->status.clothes_color;
			clif_changelook(bl, LOOK_CLOTHES_COLOR, 0);
			sd->status.clothes_color = 0;
		}
	}

	(*sc_count)++;

	// Place updated status change data values into sc_data structure
	sc_data[type].val1 = val1;
	sc_data[type].val2 = val2;
	sc_data[type].val3 = val3;
	sc_data[type].val4 = val4;

	// Place updated status change timer (status duration) into sc_data structure
	sc_data[type].timer = add_timer(gettick_cache + tick, status_change_timer, bl->id, type);

	// Check to see if player calculation is needed
	if (sd) {
		if (scflag.calc)
			status_calc_pc(sd, 0);
		if (type == SC_RUN)
			pc_run(sd, val1, val2);
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
void status_change_clear(struct block_list *bl, int type) {

	// Create necessary variables
	struct status_change* sc_data;
	short *sc_count, *option, *opt1, *opt2, *opt3;
	int i;

	// Verify pointers are valid
	nullpo_retv(bl);
	nullpo_retv(sc_data = status_get_sc_data(bl));
	nullpo_retv(sc_count = status_get_sc_count(bl));
	nullpo_retv(option = status_get_option(bl));
	nullpo_retv(opt1 = status_get_opt1(bl));
	nullpo_retv(opt2 = status_get_opt2(bl));
	nullpo_retv(opt3 = status_get_opt3(bl));

	// If no status changes are in effect, return
	if (*sc_count == 0)
		return;

	// Status Change End loop
	for(i = 0; i < SC_MAX; i++) {
		if (sc_data[i].timer == -1)
			continue;
		if (type == 0) {
			switch(i) {
				// These statuses do not dispel on death
				case SC_EDP:
				case SC_MELTDOWN:
				case SC_NOCHAT:
				case SC_TKREST:
				case SC_READYSTORM:
				case SC_READYDOWN:
				case SC_READYCOUNTER:
				case SC_READYTURN:
				case SC_READYDODGE:
				case SC_GDSKILLDELAY:
				case SC_STRFOOD:
				case SC_AGIFOOD:
				case SC_VITFOOD:
				case SC_INTFOOD:
				case SC_DEXFOOD:
				case SC_LUKFOOD:
				case SC_HITFOOD:
				case SC_FLEEFOOD:
				case SC_BATKFOOD:
				case SC_WATKFOOD:
				case SC_MATKFOOD:
				case SC_XMAS:
				case SC_FALCON:
				case SC_RIDING:
					continue;
			}
		}
		// End status
		status_change_end(bl, i, -1);
	}

	// Reset option variables
	*opt1 = 0;
	*opt2 = 0;
	*opt3 = 0;

	// Unknown check
	if (type == BL_PC &&
	    (battle_config.atc_gmonly == 0 || ((struct map_session_data *)bl)->GM_level) &&
	    (((struct map_session_data *)bl)->GM_level >= get_atcommand_level(AtCommand_Hide)))
		*option &= (OPTION_MASK | OPTION_HIDE);
	else
		*option &= OPTION_MASK;

	if (!type || type&2)
		clif_changeoption(bl);

	// Finished
	return;
}

/*==========================================
 *
 *------------------------------------------
 */
int status_change_end(struct block_list* bl, int type, int tid) {

	// Create necessary variables
	struct status_change* sc_data;
	int opt_flag = 0, calc_flag = 0, dye_fix = 0;
	short *sc_count, *option, *opt1, *opt2, *opt3;

	// Verify pointer is valid
	nullpo_retr(0, bl);

	// If not a Monster or Player, return
	if (bl->type != BL_PC && bl->type != BL_MOB)
		return 0;

	// If type (Status Change ID) is invalid, return
	if (type < 0 || type >= SC_MAX)
		return 0;

	// Verify pointers are valid
	nullpo_retr(0, sc_data = status_get_sc_data(bl));
	nullpo_retr(0, sc_count = status_get_sc_count(bl));
	nullpo_retr(0, option = status_get_option(bl));
	nullpo_retr(0, opt1 = status_get_opt1(bl));
	nullpo_retr(0, opt2 = status_get_opt2(bl));
	nullpo_retr(0, opt3 = status_get_opt3(bl));

	if ((*sc_count) > 0 && sc_data[type].timer != -1 && (sc_data[type].timer == tid || tid == -1)) {
		// Remove timer for status change
		if (tid == -1)
			delete_timer(sc_data[type].timer, status_change_timer);

		// Set status change timer comparison value to -1 (Inactive)
		sc_data[type].timer = -1;
		(*sc_count)--;

		switch(type) {
			case SC_CONCENTRATE:
			case SC_BLESSING:
			case SC_ANGELUS:
			case SC_INCREASEAGI:
			case SC_DECREASEAGI:
			case SC_SIGNUMCRUCIS:
			case SC_HIDING:
			case SC_TWOHANDQUICKEN:
			case SC_ADRENALINE:
			case SC_ENCPOISON:
			case SC_IMPOSITIO:
			case SC_GLORIA:
			case SC_LOUD:
			case SC_QUAGMIRE:
			case SC_PROVIDENCE:
			case SC_SPEARQUICKEN:
			case SC_VOLCANO:
			case SC_DELUGE:
			case SC_VIOLENTGALE:
			case SC_ETERNALCHAOS:
			case SC_DRUMBATTLE:
			case SC_NIBELUNGEN:
			case SC_SIEGFRIED:
			case SC_WHISTLE:
			case SC_ASSNCROS:
			case SC_HUMMING:
			case SC_DONTFORGETME:
			case SC_FORTUNE:
			case SC_SERVICEFORYOU:
			case SC_EXPLOSIONSPIRITS:
			case SC_STEELBODY:
			case SC_ASPDPOTION0:
			case SC_ASPDPOTION1:
			case SC_ASPDPOTION2:
			case SC_ASPDPOTION3:
			case SC_APPLEIDUN:
			case SC_RIDING:
			case SC_BLADESTOP_WAIT:
			case SC_AURABLADE:
			case SC_PARRYING:
			case SC_CONCENTRATION:
			case SC_TENSIONRELAX:
			case SC_ASSUMPTIO:
			case SC_WINDWALK:
			case SC_TRUESIGHT:
			case SC_SPIDERWEB:
			case SC_MAGICPOWER:
			case SC_CHASEWALK:
			case SC_ATKPOT:
			case SC_MATKPOT:
			case SC_WEDDING:
			case SC_MELTDOWN:
			case SC_CARTBOOST:
			case SC_EDP:
			case SC_SLOWDOWN:
			case SC_SPEEDUP0:
			case SC_INCSTR:
			case SC_INCAGI:
			case SC_INCDEX:
			case SC_INCALLSTATUS:
			case SC_INCHIT:
			case SC_INCFLEE:
			case SC_INCMHPRATE:
			case SC_INCMSPRATE:			
			case SC_INCMATKRATE:
			case SC_INCHITRATE:
			case SC_INCATKRATE:
			case SC_INCDEFRATE:
			case SC_INCFLEERATE:
			case SC_INCASPDRATE:
			case SC_BATTLEORDERS:
			case SC_REGENERATION:
			case SC_GUILDAURA:
			case SC_SPURT:
			case SC_STRFOOD:
			case SC_AGIFOOD:
			case SC_VITFOOD:
			case SC_INTFOOD:
			case SC_DEXFOOD:
			case SC_LUKFOOD:
			case SC_HITFOOD:
			case SC_FLEEFOOD:
			case SC_BATKFOOD:
			case SC_WATKFOOD:
			case SC_MATKFOOD:
			case SC_NEN:
			case SC_MADNESSCANCEL:
			case SC_ADJUSTMENT:
			case SC_INCREASING:
			case SC_GATLINGFEVER:
			case SC_BERSERK:
			case SC_SUITON:
			case SC_ONEHAND:
			case SC_FUSION:
				// Run status_calc_pc at end of function (Recalculate player's status)
				calc_flag = 1;
				break;
			case SC_INVINCIBLE:
				calc_flag = 1;
				break;
			case SC_PROVOKE:
			case SC_ENDURE:
				if (bl->type == BL_PC) {
					struct map_session_data *sd=NULL;
					sd = (struct map_session_data *)bl;
					if (sd) {
						struct map_session_data *tsd;
						int i;
						for (i = 0; i < 5; i++)
						{    
							if (sd->dev.val1[i] && (tsd = map_id2sd(sd->dev.val1[i])))
							status_change_end(&tsd->bl,SC_ENDURE,-1);
						}
					}
				}
				break;
			case SC_AUTOGUARD:
				if (bl->type == BL_PC) {
					struct map_session_data *sd=NULL;
					sd = (struct map_session_data *)bl;
					if (sd) {
						struct map_session_data *tsd;
						int i;
						for (i = 0; i < 5; i++)
						{    
						if (sd->dev.val1[i] && (tsd = map_id2sd(sd->dev.val1[i])))
							status_change_end(&tsd->bl,SC_AUTOGUARD,-1);
						}
					}
				}
				break;
			case SC_DEFENDER:
				if (bl->type == BL_PC) {
					struct map_session_data *sd = NULL;
					if ((sd = (struct map_session_data *)bl)) {
						struct map_session_data *tsd = NULL;
						register int i;
						for(i = 0; i < 5; i++)
						{
							if (sd->dev.val1[i] && (tsd = map_id2sd(sd->dev.val1[i])))
								status_change_end(&tsd->bl, SC_DEFENDER, -1);
						}
					}
				}
				// Update walking speed
				calc_flag = 1;
				break;
			case SC_REFLECTSHIELD:
				if (bl->type == BL_PC) {
					struct map_session_data *sd=NULL;
					sd = (struct map_session_data *)bl;
					if (sd) {
						struct map_session_data *tsd;
						int i;
						for (i = 0; i < 5; i++)
						{    
							if (sd->dev.val1[i] && (tsd = map_id2sd(sd->dev.val1[i])))
								status_change_end(&tsd->bl,SC_REFLECTSHIELD,-1);
						}
					}
				}
				break;
			case SC_SHRINK:
				if (bl->type == BL_PC) {
					struct map_session_data *sd=NULL;
					sd = (struct map_session_data *)bl;
					if (sd) {
						struct map_session_data *tsd;
						int i;
						for (i = 0; i < 5; i++)
						{    
							if (sd->dev.val1[i] && (tsd = map_id2sd(sd->dev.val1[i])))
							status_change_end(&tsd->bl,SC_SHRINK,-1);
						}
					}
				}
				break;
			case SC_RUN:
			{
				struct map_session_data *sd;
				if (bl->type == BL_PC && (sd = (struct map_session_data *)bl)) {
					if (sd->walktimer != -1)
						pc_stop_walking(sd,1);
					calc_flag = 1;
				}
			}
			break;
			case SC_AUTOBERSERK:
				if (sc_data[SC_PROVOKE].timer != -1)
					status_change_end(bl,SC_PROVOKE,-1);
				break;
			case SC_DEVOTION:
			{
				struct map_session_data *md = map_id2sd(sc_data[type].val1);
				skill_devotion(md,bl->id);
				sc_data[type].val1=sc_data[type].val2=0;
				calc_flag = 1;
				int type2 = SC_AUTOGUARD;
				if (sc_data[type2].timer != -1)
					status_change_end(bl,type2,-1);
				type2 = SC_ENDURE;        
				if (sc_data[type2].timer != -1)
					status_change_end(bl,type2,-1);
				type2 = SC_DEFENDER;
				if (sc_data[type2].timer != -1)
					status_change_end(bl,type2,-1);
				type2 = SC_REFLECTSHIELD;
				if (sc_data[type2].timer != -1)
					status_change_end(bl,type2,-1);
				type2 = SC_SHRINK;
				if (sc_data[type2].timer != -1)
					status_change_end(bl,type2,-1);
			}
			break;
			case SC_BLADESTOP:
			{
				struct status_change *t_sc_data = status_get_sc_data((struct block_list *)sc_data[type].val4);
				if (t_sc_data && t_sc_data[SC_BLADESTOP].timer!=-1)
					status_change_end((struct block_list *)sc_data[type].val4,SC_BLADESTOP,-1);
				if (sc_data[type].val2==2)
					clif_bladestop((struct block_list *)sc_data[type].val3,(struct block_list *)sc_data[type].val4,0);
			}
			break;
			case SC_GRAVITATION:
				if (sc_data[type].val3 != BCT_SELF)
					calc_flag = 1;
				break;
			case SC_DANCING:
			{
				struct map_session_data *dsd;
				struct status_change *d_sc_data;
				if (sc_data[type].val4 && (dsd = map_id2sd(sc_data[type].val4))){
					d_sc_data = dsd->sc_data;
					if (d_sc_data && d_sc_data[type].timer != -1)
						d_sc_data[type].val4 = 0;
				}
				if (sc_data[type].val1 == CG_MOONLIT)
						status_change_end(bl, SC_MOONLIT, -1);
				if (sc_data[SC_LONGING].timer!=-1)
					status_change_end(bl,SC_LONGING,-1);
				calc_flag = 1;
			}
			break;
			case SC_SACRIFICE:
				sc_data[SC_SACRIFICE].val2 = 0;
				break;
			case SC_NOCHAT:
			{
				struct map_session_data *sd = NULL;
				if (bl->type == BL_PC && (sd = (struct map_session_data *)bl)) {
					if (sd->status.manner >= 0)
						sd->status.manner = 0;
					clif_updatestatus(sd, SP_MANNER);
				}
			}
			break;
			case SC_SPLASHER:
			{
				struct block_list *src=map_id2bl(sc_data[type].val3);
				if (src && tid!=-1)
					skill_castend_damage_id(src, bl, sc_data[type].val2, sc_data[type].val1, gettick_cache, 0);
			}
			break;
			case SC_CLOSECONFINE:
			{
				// Unlock target
				if (sc_data[type].val2 > 0) {
					struct block_list *target = map_id2bl(sc_data[type].val2);
					struct status_change *t_sc_data = status_get_sc_data(target);
					// Check if target is still confined
					if (target && t_sc_data && t_sc_data[SC_CLOSECONFINE2].timer != -1)
						status_change_end(target, SC_CLOSECONFINE2, -1);
				}
			}
			break;
			case SC_CLOSECONFINE2:
				if (sc_data[type].val2 > 0) {
					struct block_list *src = map_id2bl(sc_data[type].val2);
					struct status_change *t_sc_data = status_get_sc_data(src);
					// Check if caster is still confined
					if (src && t_sc_data && t_sc_data[SC_CLOSECONFINE].timer != -1)
						status_change_end(src, SC_CLOSECONFINE, -1);
				}
				break;
			case SC_SELFDESTRUCTION:
			{
				struct mob_data *md = NULL;
				if (bl->type == BL_MOB && (md = (struct mob_data*)bl))
					skill_castend_damage_id(bl, bl, sc_data[type].val2, sc_data[type].val1, gettick_cache, 0);
			}
			break;
		// Option1
			case SC_FREEZE:
				sc_data[type].val3 = 0;
				break;
		// Option2
			case SC_POISON:
			case SC_BLIND:
			case SC_CURSE:
				calc_flag = 1;
				break;
			case SC_CONFUSION:
				break;
			case SC_MARIONETTE:
			case SC_MARIONETTE2:
			{
				// Check for partner and end their Marionette status as well
				int type2 = (type == SC_MARIONETTE) ? SC_MARIONETTE2 : SC_MARIONETTE;
				struct block_list *pbl = map_id2bl(sc_data[type].val3);
				if (pbl) {
					struct status_change* p_sc_data;
					if (*status_get_sc_count(pbl) > 0 &&
						(p_sc_data = status_get_sc_data(pbl)) &&
						p_sc_data[type2].timer != -1)
						status_change_end(pbl, type2, -1);
				}
				calc_flag = 1;
			}
			break;
			case SC_GOSPEL:
				if (sc_data[type].val4 == BCT_SELF) {
					struct skill_unit_group *group = (struct skill_unit_group *)sc_data[type].val3;
					sc_data[type].val4 = 0;
					skill_delunitgroup(group);
				}
				break;
			case SC_BASILICA:
				if (sc_data[type].val3 == BCT_SELF)
					skill_clear_unitgroup(bl);
				break;
			case SC_HERMODE:
				if (sc_data[type].val3 == BCT_SELF)
					skill_clear_unitgroup(bl);
				else
					calc_flag = 1;
				break;
			case SC_BUNSINJYUTSU:
				dye_fix = 1;
				break;
		}

		// Remove status changes in the client
		if (bl->type == BL_PC)
			clif_status_change(bl, StatusIconTable[type], 0);

		switch(type) {
			case SC_STONE:
			case SC_FREEZE:
			case SC_STUN:
			case SC_SLEEP:
				*opt1 = 0;
				opt_flag = 1;
				break;
			case SC_POISON:
				if (sc_data[SC_DPOISON].timer != -1)
					break;
				*opt2 &= ~1;
				opt_flag = 1;
				break;
			case SC_CURSE:
			case SC_SILENCE:
			case SC_CONFUSION:
			case SC_BLIND:
				*opt2 &= ~(1<<(type - SC_POISON));
				opt_flag = 1;
				break;
			case SC_DPOISON:
				if (sc_data[SC_POISON].timer != -1)
					break;
				*opt2 &= ~0x080;
				opt_flag = 1;
				break;
			case SC_SIGNUMCRUCIS:
				*opt2 &= ~0x40;
				opt_flag = 1;
				break;
			case SC_HIDING:
			case SC_CLOAKING:
			case SC_CHASEWALK:
			{
				struct map_session_data *sd = NULL;
				if (type == SC_CHASEWALK)
					*option &= ~16388;
				else {
					*option &= ~((type == SC_HIDING) ? 2 : 4);
					calc_flag = 1;
				}
				// To avoid hidden/cloaked chars standing on warp portal and attacking enemies
				if (bl->type == BL_PC && (sd = (struct map_session_data *)bl)) {
					if (map_getcell(sd->bl.m, sd->bl.x, sd->bl.y, CELL_CHKNPC)) {
						clif_changeoption(bl);
						npc_touch_areanpc(sd, sd->bl.m, sd->bl.x, sd->bl.y);
						break;
					}
				}
				opt_flag = 1;
			}
			break;
			case SC_SKA:
				*opt3 &= ~16;
				opt_flag = 0;
				break;
			case SC_SKE:
				*opt3 &= ~4;
				opt_flag = 0;
				break;
			case SC_SWOO:
				*opt3 &= ~2;
				opt_flag = 0;
				break;
			case SC_SIGHT:
				*option &= ~1;
				opt_flag = 1;
				break;
			case SC_WEDDING:
				*option &= ~4096;
				opt_flag = 1;
				break;
			case SC_RUWACH:
				*option &= ~8192;
				opt_flag = 1;
				break;
			// Option3
			case SC_TWOHANDQUICKEN:
			case SC_SPEARQUICKEN:
			case SC_CONCENTRATION:
			case SC_ONEHAND:
				*opt3 &= ~1;
				break;
			case SC_OVERTHRUST:
			case SC_NEN:
				*opt3 &= ~2;
				break;
			case SC_ENERGYCOAT:
				*opt3 &= ~4;
				break;
			case SC_EXPLOSIONSPIRITS:
				*opt3 &= ~8;
				break;
			case SC_STEELBODY:
				*opt3 &= ~16;
				break;
			case SC_BLADESTOP:
				*opt3 &= ~32;
				break;
			case SC_BERSERK:
				*opt3 &= ~128;
				break;
			case SC_MOONLIT:
				*opt3 &= ~512;
				break;
			case SC_MARIONETTE:
			case SC_MARIONETTE2:
				*opt3 &= ~1024;
				break;
			case SC_ASSUMPTIO:
				*opt3 &= ~2048;
				break;
			case SC_SPIRIT:
				*opt3 &= ~32768;
				break;
		}

		// Check and change option flag if necessary
		if (opt_flag)
			clif_changeoption(bl);

		if (bl->type == BL_PC && dye_fix) {
			struct map_session_data *sd = NULL;
			sd = (struct map_session_data *)bl;
			if (sd && !sd->status.clothes_color && sc_data[type].val4)
				clif_changelook(bl, LOOK_CLOTHES_COLOR, sc_data[type].val4);
				sd->status.clothes_color = sc_data[type].val4;
		}

		// Recalculate player status
		if (bl->type == BL_PC && calc_flag)
			status_calc_pc((struct map_session_data *)bl,0);
	}

	// Finished
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int status_change_timer(int tid, unsigned int tick, int id, int data) {

	int type=data;
	struct block_list *bl;
	struct map_session_data *sd = NULL;
	struct status_change *sc_data;

	nullpo_retr(0, bl = map_id2bl(id));
	nullpo_retr(0, sc_data = status_get_sc_data(bl));

	if (bl->type == BL_PC)
		nullpo_retr(0, sd = (struct map_session_data *)bl);

	if (sc_data[type].timer != tid) {
		if (battle_config.error_log)
			printf("status_change_timer %d != %d\n", tid, sc_data[type].timer);
		return 0;
	}

	// Main status change calculation check
	switch(type) {
	case SC_MAXIMIZEPOWER:
	case SC_CLOAKING:
		if (sd) {
			if (sd->status.sp > 0) {
				sd->status.sp--;
				clif_updatestatus(sd, SP_SP);
				sc_data[type].timer = add_timer(sc_data[type].val2 + tick, status_change_timer, bl->id, data);
				return 0;
			}
		}
		break;
	case SC_CHASEWALK:
		if (sd) {
			int sp = 10 + sc_data[SC_CHASEWALK].val1 * 2;
			if (map[sd->bl.m].flag.gvg) sp *= 5;
			if (sd->status.sp > sp){
				sd->status.sp -= sp; // Update SP cost
				clif_updatestatus(sd, SP_SP);
				if ((++sc_data[SC_CHASEWALK].val4) == 1) {
					if (sd->sc_data[SC_SPIRIT].timer != -1 && sd->sc_data[SC_SPIRIT].val2 == SL_ROGUE)
						status_change_start(bl, SC_INCSTR, 1 << (sc_data[SC_CHASEWALK].val1-1), 0, 0, 0, 300000, 0);
					else
						status_change_start(bl, SC_INCSTR, 1 << (sc_data[SC_CHASEWALK].val1-1), 0, 0, 0, 30000, 0);
					status_calc_pc(sd, 0);
				}
				sc_data[type].timer = add_timer(sc_data[type].val2 + tick, status_change_timer, bl->id, data);
				return 0;
			}
		}
		break;
	case SC_HIDING:
		if (sd) {
			if (sd->status.sp > 0 && (--sc_data[type].val2) > 0) {
				if (sc_data[type].val2 % (sc_data[type].val1 + 3) == 0) {
					sd->status.sp--;
					clif_updatestatus(sd, SP_SP);
				}
				sc_data[type].timer = add_timer(1000 + tick, status_change_timer, bl->id, data);
				return 0;
			}
		}
		break;
	case SC_SKA:  
		if ((--sc_data[type].val2) > 0) {
			sc_data[type].val3 = rand()%100; // Random defense
			sc_data[type].timer = add_timer(  
				1000+tick, status_change_timer,  
				bl->id, data);  
			return 0;  
		}  
		break;
	case SC_SIGHT:
	case SC_RUWACH:
	case SC_SIGHTBLASTER:
	{
		int range = battle_config.ruwach_range;
		if (type == SC_SIGHT)
			range = battle_config.sight_range;
		map_foreachinarea(status_change_timer_sub, bl->m, bl->x - range, bl->y - range, bl->x + range, bl->y + range, 0, bl, type, tick);
		if ((--sc_data[type].val2) > 0) {
			sc_data[type].timer = add_timer(250 + tick, status_change_timer, bl->id, data);
			return 0;
		}
	}
	break;
	case SC_SIGNUMCRUCIS:
	{
		int race = status_get_race(bl);
		if (race == 6 || battle_check_undead(race, status_get_elem_type(bl))) {
			sc_data[type].timer = add_timer(1000 * 600 + tick, status_change_timer, bl->id, data);
			return 0;
		}
	}
	break;
	case SC_PROVOKE:
		if (sc_data[type].val2 != 0) {
			if (sd && sd->status.hp > sd->status.max_hp >> 2)
				break;
			sc_data[type].timer = add_timer(1000 + tick, status_change_timer, bl->id, data);
			return 0;
		}
		break;
	case SC_ENDURE:
	case SC_AUTOBERSERK:
		if (sd && sd->special_state.infinite_endure) {
			sc_data[type].timer = add_timer(1000 * 60 + tick, status_change_timer, bl->id, data);
			return 0;
		}
		break;
	case SC_STONE:
		if (sc_data[type].val2 != 0) {
			short *opt1 = status_get_opt1(bl);
			sc_data[type].val2 = 0;
			sc_data[type].val4 = 0;
			battle_stopwalking(bl, 1);
			if (opt1) {
				*opt1 = 1;
				clif_changeoption(bl);
			}
			sc_data[type].timer = add_timer(1000 + tick, status_change_timer, bl->id, data);
			return 0;
		} else if ((--sc_data[type].val3) > 0) {
			int hp = status_get_max_hp(bl);
			if ((++sc_data[type].val4)%5 == 0 && status_get_hp(bl) > hp>>2) {
				hp = hp / 100;
				if (hp < 1) hp = 1;
				if (sd)
					pc_heal(sd,-hp,0);
				else if (bl->type == BL_MOB) {
					struct mob_data *md;
					if ((md = ((struct mob_data *)bl)) == NULL)
						break;
					md->hp -= hp;
				}
			}
			sc_data[type].timer = add_timer(1000 + tick, status_change_timer, bl->id, data);
			return 0;
		}
		break;
	case SC_POISON:
		if (sc_data[SC_SLOWPOISON].timer == -1) {
			if ((--sc_data[type].val3) > 0) {
				int hp = status_get_max_hp(bl);
				if (status_get_hp(bl) > hp>>2) {
					if (sd) {
						hp = 3 + hp * 3 / 200;
						pc_heal(sd, -hp, 0);
					} else if(bl->type == BL_MOB) {
						struct mob_data *md;
						if ((md = ((struct mob_data *)bl)) == NULL)
							break;
						hp = 3 + hp / 200;
						md->hp -= hp;
					}
				}
				sc_data[type].timer = add_timer(1000 + tick, status_change_timer, bl->id, data );
			}
		} else
			sc_data[type].timer = add_timer(1000 + tick, status_change_timer, bl->id, data );
		break;
	case SC_DPOISON:
		if (sc_data[SC_SLOWPOISON].timer == -1 && (--sc_data[type].val3) > 0) {
			int hp = status_get_max_hp(bl);
			if (status_get_hp(bl) > hp>>2) {
				if (sd) {
					hp = 3 + hp/50;
					pc_heal(sd, -hp, 0);
				} else if (bl->type == BL_MOB) {
					struct mob_data *md;
					if ((md = ((struct mob_data *)bl)) == NULL)
						break;
					hp = 3 + hp/100;
					md->hp -= hp;
				}
			}
		}
		if (sc_data[type].val3 > 0)
			sc_data[type].timer = add_timer(1000 + tick, status_change_timer, bl->id, data);
		break;
	case SC_TENSIONRELAX:
		if (sd) {
			if (sd->status.sp > 12 && sd->status.max_hp > sd->status.hp) {
				sc_data[type].timer = add_timer(10000 + tick, status_change_timer, bl->id, data);
				return 0;
			}
			if (sd->status.max_hp <= sd->status.hp)
				status_change_end(&sd->bl,SC_TENSIONRELAX,-1);
		}
		break;
	case SC_BLEEDING:
		if ((sc_data[type].val4 -= 10000) >= 0) {
			int hp = rand()%300 + 400;
			if (sd) {
				pc_heal(sd,-hp,0);
			} else if(bl->type == BL_MOB) {
				struct mob_data *md = (struct mob_data *)bl;
				if (md) md->hp -= hp;
			}
			if (!status_isdead(bl)) {
				// Walking and casting effect is lost
				battle_stopwalking (bl, 1);
				skill_castcancel (bl, 0, 0);
				sc_data[type].timer = add_timer(10000 + tick, status_change_timer, bl->id, data);
			}
			return 0;
		}
		break;
	// No time limit status changes
	case SC_AETERNA:
	case SC_TRICKDEAD:
	case SC_RIDING:
	case SC_FALCON:
	case SC_WEIGHT50:
	case SC_WEIGHT90:
	case SC_MAGICPOWER:
	case SC_REJECTSWORD:
	case SC_MEMORIZE:
	case SC_BROKENWEAPON:
	case SC_BROKENARMOR:
	case SC_SACRIFICE:
	case SC_RUN:
	case SC_READYSTORM:
	case SC_READYDOWN:
	case SC_READYTURN:
	case SC_READYCOUNTER:
	case SC_READYDODGE:
	case SC_INVINCIBLE:
		sc_data[type].timer = add_timer(1000 * 600 + tick, status_change_timer, bl->id, data);
		return 0;
	// End of no time limit status changes
	case SC_DANCING:
	{
		int s = 0, sp = 1;
		if (sd) {
			if (sd->status.sp > 0 && (--sc_data[type].val3)>0) {
				switch(sc_data[type].val1){
				case BD_RICHMANKIM:
				case BD_DRUMBATTLEFIELD:
				case BD_RINGNIBELUNGEN:
				case BD_SIEGFRIED:
				case BA_DISSONANCE:
				case BA_ASSASSINCROSS:
				case DC_UGLYDANCE:
					s=3;
					break;
				case BD_LULLABY:
				case BD_ETERNALCHAOS:
				case BD_ROKISWEIL:
				case DC_FORTUNEKISS:
					s=4;
					break;
				case CG_HERMODE:
					sp = 5;
				case BD_INTOABYSS:
				case BA_WHISTLE:
				case DC_HUMMING:
				case BA_POEMBRAGI:
				case DC_SERVICEFORYOU:
					s = 5;
					break;
				case BA_APPLEIDUN:
					s = 6;
					break;
				case DC_DONTFORGETME:
				case CG_MOONLIT:
					s = 10;
					break;
				}
				if (s && ((sc_data[type].val3 % s) == 0)) {
					if (sc_data[SC_LONGING].timer != -1)
						sd->status.sp -= 3;
					else
						sd->status.sp -= sp;
					if (sd->status.sp <= 0) sd->status.sp = 0;
					clif_updatestatus(sd, SP_SP);
				}
				sc_data[type].timer = add_timer(1000 + tick, status_change_timer, bl->id, data);
				return 0;
			} else if (sd->status.sp <= 0) {
				if (sc_data[SC_DANCING].timer != -1)
					skill_stop_dancing(&sd->bl,0);
			}
		}
	}
	break;
	case SC_BERSERK:
		if (sd) {
			if ((sd->status.hp - sd->status.max_hp * 5 / 100) > 100) {
				sd->status.hp -= sd->status.max_hp * 5 / 100;
				clif_updatestatus(sd, SP_HP);
				sc_data[type].timer = add_timer(10000 + tick, status_change_timer, bl->id, data);
				return 0;
			}
		}
		break;
	case SC_WEDDING:
		if (sd) {
			time_t timer;
			if (time(&timer) < ((sc_data[type].val2) + 3600)) {
				sc_data[type].timer = add_timer(10000 + tick, status_change_timer, bl->id, data);
				return 0;
			}
		}
		break;
	case SC_XMAS:
		if (sd) {
			time_t timer;
			if (time(&timer) < ((sc_data[type].val2) + 3600)) {
				sc_data[type].timer = add_timer(10000 + tick, status_change_timer, bl->id, data);
				return 0;
			}
		}
		break;
	case SC_NOCHAT:
		if (sd && battle_config.muting_players) {
			time_t timer;
			if ((++sd->status.manner) && time(&timer) < ((sc_data[type].val2) + 60 * (0 - sd->status.manner))) {
				clif_updatestatus(sd, SP_MANNER);
				sc_data[type].timer = add_timer(60000 + tick, status_change_timer, bl->id, data);
				return 0;
			}
		}
		break;
	case SC_SELFDESTRUCTION:
		if (--sc_data[type].val3 > 0) {
			struct mob_data *md;
			if (bl->type == BL_MOB && (md = (struct mob_data *)bl) && md->speed > 250) {
				md->speed -= 250;
				md->next_walktime = tick;
			}
			sc_data[type].timer = add_timer(1000 + tick, status_change_timer, bl->id, data);
			return 0;
		}
		break;
	case SC_SPLASHER:
		if (sc_data[type].val4 % 1000 == 0) {
			char timer[2];
			sprintf(timer, "%d", sc_data[type].val4 / 1000);
			clif_message(bl, timer);
		}
		if ((sc_data[type].val4 -= 500) > 0) {
			sc_data[type].timer = add_timer(500 + tick, status_change_timer, bl->id, data);
				return 0;
		}
		break;
	case SC_MARIONETTE:
	case SC_MARIONETTE2:
	{
		struct block_list *pbl = map_id2bl(sc_data[type].val3);
		if (pbl && battle_check_range(bl, pbl, 7) && (sc_data[type].val2 -= 1000) > 0) {
			sc_data[type].timer = add_timer(1000 + tick, status_change_timer, bl->id, data);
			return 0;
		}
	}
	break;
	case SC_CONFUSION:
		if ((sc_data[type].val2 -= 1500) > 0) {
			sc_data[type].timer = add_timer(3000 + tick, status_change_timer, bl->id, data);
			return 0;
		}
		break;
	case SC_GOSPEL:
		if (sc_data[type].val4 == BCT_SELF) {
			int hp, sp;
			hp = (sc_data[type].val1 > 5) ? 45 : 30;
			sp = (sc_data[type].val1 > 5) ? 35 : 20;
			if (status_get_hp(bl) - hp > 0 && (sd == NULL || sd->status.sp - sp > 0)) {
				if (sd)
					pc_heal(sd, -hp, -sp);
				else if (bl->type == BL_MOB)
					mob_heal((struct mob_data *)bl, -hp);
		
				if ((sc_data[type].val2 -= 10000) > 0) {
					sc_data[type].timer = add_timer(10000 + tick, status_change_timer, bl->id, data);
					return 0;
				}
			}
		}
		break;
	case SC_GUILDAURA:
	{
		struct block_list *tbl = map_id2bl(sc_data[type].val2);
		if (tbl && battle_check_range(bl, tbl, 2)) {
			sc_data[type].timer = add_timer(1000 + tick, status_change_timer, bl->id, data);
			return 0;
		}
	}
	break;
	}
	// End of main status calculation check

	return status_change_end( bl,type,tid );
}

/*==========================================
 *
 *------------------------------------------
 */
int status_change_timer_sub(struct block_list *bl, va_list ap) {

	// Create necessary variables
	struct block_list *src;
	int type;
	unsigned int tick;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);
	nullpo_retr(0, src = va_arg(ap, struct block_list*));
	type=va_arg(ap, int);
	tick=va_arg(ap, unsigned int);

	// If source is not a Player, or Monster, fail
	if (bl->type != BL_PC && bl->type != BL_MOB)
		return 0;

	switch(type) {
		case SC_SIGHT:
		case SC_CONCENTRATE:
			if ((*status_get_option(bl)) & 6) {
				status_change_end(bl, SC_HIDING, -1);
				status_change_end(bl, SC_CLOAKING, -1);
			}
			break;
		case SC_RUWACH:
			if ((*status_get_option(bl)) & 6) {
				// Check whether the target is Hiding/Cloaking
				struct status_change *sc_data = status_get_sc_data(bl);
				// If the target is using a special Hiding status, Example: Chase Walk, don't bother
				if (sc_data && (sc_data[SC_HIDING].timer != -1 ||
				    sc_data[SC_CLOAKING].timer != -1)) {
					status_change_end(bl, SC_HIDING, -1);
					status_change_end(bl, SC_CLOAKING, -1);
					if (battle_check_target(src, bl, BCT_ENEMY) > 0)
						skill_attack(BF_MAGIC, src, src, bl, AL_RUWACH, 1, tick, 0);
				}
			}
			break;
		case SC_SIGHTBLASTER:
		{
			struct status_change *sc_data = status_get_sc_data(src);
			short *sc_count = status_get_sc_count(src);
			if (sc_data && sc_count && sc_data[type].val2 > 0 && battle_check_target(src, bl, BCT_ENEMY) > 0) {
			// Status change check prevents a single round of Sight Blaster hitting multiple opponents
				skill_attack(BF_MAGIC, src, src, bl, WZ_SIGHTBLASTER, 1, tick, 0);
				sc_data[type].val2 = 0; // Signals it to end
			}
		}
		break;
		case SC_CLOSECONFINE:
		{
			struct status_change *sc_data = status_get_sc_data(bl);
			short *sc_count = status_get_sc_count(bl);
			// Lock char has released the hold on everyone...
			if (sc_data && sc_count && sc_data[SC_CLOSECONFINE2].timer != -1 && sc_data[SC_CLOSECONFINE2].val2 == src->id) {
				sc_data[SC_CLOSECONFINE2].val2 = 0;
				status_change_end(bl, SC_CLOSECONFINE2, -1);
			}
		}
		break;
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int status_change_clear_buffs (struct block_list *bl) {

	// Get Status Change data
	struct status_change *sc_data = status_get_sc_data(bl);

	// Create necessary variables
	int i;

	// If there is no status change data, cancel
	if (!sc_data)
		return 0;

	// Doesn't affect standard status effects like Poison, Stun, etc
	// SC_COMMON_MAX = 10
	for (i = SC_COMMON_MAX+1; i < SC_MAX; i++) {

		// If status is already deactivated, skip it
		if (sc_data[i].timer == -1)
			continue;

		switch (i) {
			// Statuses that cannot be removed
			case SC_WEIGHT50:
			case SC_WEIGHT90:
			case SC_COMBO:
			case SC_SMA:
			case SC_DANCING:
			case SC_GUILDAURA:
			case SC_SAFETYWALL:
			case SC_NOCHAT:
			case SC_JAILED:
			case SC_ANKLE:
			case SC_BLADESTOP:
			case SC_CP_WEAPON:
			case SC_CP_SHIELD:
			case SC_CP_ARMOR:
			case SC_CP_HELM:
			case SC_FALCON:
			case SC_RIDING:
			case SC_GDSKILLDELAY:
			case SC_XMAS:
			case SC_INVINCIBLE:
				continue;

			// Debuffs that cannot be removed (Use status_change_clear_debuffs)
			case SC_HALLUCINATION:
			case SC_QUAGMIRE:
			case SC_SIGNUMCRUCIS:
			case SC_DECREASEAGI:
			case SC_SLOWDOWN:
			case SC_MINDBREAKER:
			case SC_WINKCHARM:
			case SC_STOP:
			case SC_ORCISH:
			case SC_STRIPWEAPON:
			case SC_STRIPSHIELD:
			case SC_STRIPARMOR:
			case SC_STRIPHELM:
				continue;

			// Berserk cannot be removed if in the effect of Wand of Hermode
			case SC_BERSERK:
				if (sc_data[SC_HERMODE].timer != -1)
					continue;
				break;

			default:
				break;
		}

		// End status change
		status_change_end(bl,i,-1);
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int status_change_clear_debuffs (struct block_list *bl) {

	// Get Status Change data
	struct status_change *sc_data = status_get_sc_data(bl);

	// Create necessary variables
	int i;

	// If there is no status change data, cancel
	if (!sc_data)
		return 0;

	// Quick way to remove common negative status effects (Poison, Stun, etc)
	// SC_COMMON_MIN = 0, SC_COMMON_MAX = 10
	for (i = SC_COMMON_MIN; i <= SC_COMMON_MAX; i++) {
		if (sc_data[i].timer != -1)
			status_change_end(bl, i, -1);
	}

	// Remove the following debuffs
	if (sc_data[SC_HALLUCINATION].timer != -1)
		status_change_end(bl, SC_HALLUCINATION, -1);
	if (sc_data[SC_QUAGMIRE].timer != -1)
		status_change_end(bl, SC_QUAGMIRE, -1);
	if (sc_data[SC_SIGNUMCRUCIS].timer != -1)
		status_change_end(bl, SC_SIGNUMCRUCIS, -1);
	if (sc_data[SC_DECREASEAGI].timer != -1)
		status_change_end(bl, SC_DECREASEAGI, -1);
	if (sc_data[SC_SLOWDOWN].timer != -1)
		status_change_end(bl, SC_SLOWDOWN, -1);
	if (sc_data[SC_MINDBREAKER].timer != -1)
		status_change_end(bl, SC_MINDBREAKER, -1);
	if (sc_data[SC_WINKCHARM].timer != -1)
		status_change_end(bl, SC_WINKCHARM, -1);
	if (sc_data[SC_STOP].timer != -1)
		status_change_end(bl, SC_STOP, -1);
	if (sc_data[SC_ORCISH].timer != -1)
		status_change_end(bl, SC_ORCISH, -1);
	if (sc_data[SC_STRIPWEAPON].timer != -1)
		status_change_end(bl, SC_STRIPWEAPON, -1);
	if (sc_data[SC_STRIPSHIELD].timer != -1)
		status_change_end(bl, SC_STRIPSHIELD, -1);
	if (sc_data[SC_STRIPARMOR].timer != -1)
		status_change_end(bl, SC_STRIPARMOR, -1);
	if (sc_data[SC_STRIPHELM].timer != -1)
		status_change_end(bl, SC_STRIPHELM, -1);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
static int status_calc_sigma(void) {

	// Create necessary variables
	int i, j, k;

	for(i=0;i<MAX_PC_JOB_CLASS;i++) {
		memset(hp_sigma_val[i],0,sizeof(hp_sigma_val[i]));
		for(k=0,j=2;j<=MAX_LEVEL;j++) {
			k += hp_coefficient[i]*j + 50;
			k -= k%100;
			hp_sigma_val[i][j-1] = k;
		}
	}

	return 0;
}

/*==========================================
 * Status Read Database Function
 *------------------------------------------
 * Reads the following files:
 *  -> db/job_db1.txt
 *  -> db/job_db2.txt
 *  -> db/job_db2-2.txt
 *  -> db/size_fix.txt
 *  -> db/refine_db.txt
 * Map server activates function on loading
 *------------------------------------------
 */
int status_readdb(void) {

	int i, j, k;
	FILE *fp;
	char line[1024], *p;

	// Load Job Database 1 ->

	fp = fopen("db/job_db1.txt","r");
	if (fp == NULL) {
		// Show that the database failed ot load in console
		printf(CL_RED "Error:" CL_RESET " Failed to load db/job_db1.txt\n");
		return 1;
	}
	i = 0;
	// fgets reads until maximum one less than size and add '\0' -> so, it's not necessary to add -1
	while(fgets(line, sizeof(line), fp)) {
		char *split[50];
		if ((line[0] == '/' && line[1] == '/') || line[0] == '\0' || line[0] == '\n' || line[0] == '\r')
			continue;
		// It's not necessary to remove 'carriage return ('\n' or '\r')
		for(j = 0, p = line; j < 27 && p; j++) {
			split[j] = p;
			p = strchr(p, ',');
			if (p) *p++ = 0;
		}
		if (j < 27)
			continue;
		max_weight_base[i] = atoi(split[0]);
		hp_coefficient[i] = atoi(split[1]);
		hp_coefficient2[i] = atoi(split[2]);
		sp_coefficient[i] = atoi(split[3]);
		for(j=0;j<23;j++)
			aspd_base[i][j] = atoi(split[j+4]);
		i++;
		if (i == MAX_PC_JOB_CLASS)
			break;
	}
	fclose(fp);
	// Show that the database has been successfully loaded in console
	printf(CL_GREEN "Loaded: " CL_RESET "'" CL_WHITE "db/job_db1.txt" CL_RESET "' read.\n");

	memset(&job_bonus, 0, sizeof(job_bonus));

	// <- End of Job Database 1 loading

	// Load Job Database 2 ->

	fp = fopen("db/job_db2.txt", "r");
	if (fp == NULL) {
		// Show that the database failed ot load in console
		printf(CL_RED "Error:" CL_RESET " Failed to load db/job_db2.txt\n");
		return 1;
	}
	i = 0;
	// fgets reads until maximum one less than size and add '\0' -> so, it's not necessary to add -1
	while(fgets(line, sizeof(line), fp)) {
		if ((line[0] == '/' && line[1] == '/') || line[0] == '\0' || line[0] == '\n' || line[0] == '\r')
			continue;
		// It's not necessary to remove 'carriage return ('\n' or '\r')
		for(j=0,p=line;j<MAX_LEVEL && p;j++){
			if (sscanf(p,"%d",&k)==0)
				break;
			job_bonus[0][i][j]=k;
			job_bonus[2][i][j]=k;
			p=strchr(p,',');
			if (p) p++;
		}
		i++;

		if (i==MAX_PC_JOB_CLASS)
			break;
	}
	fclose(fp);
	// Show that the database has been successfully loaded in console
	printf(CL_GREEN "Loaded: " CL_RESET "'" CL_WHITE "db/job_db2.txt" CL_RESET "' read.\n");
	
	// <- End of Job Database 2 loading

	// Load Job Database 2-2 ->

	fp = fopen("db/job_db2-2.txt","r");
	if (fp == NULL) {
		// Show that the database failed ot load in console
		printf(CL_RED "Error:" CL_RESET " Failed to load db/job_db2-2.txt\n");
		return 1;
	}
	i=0;
	// fgets reads until maximum one less than size and add '\0' -> so, it's not necessary to add -1
	while(fgets(line, sizeof(line), fp)) {
		if ((line[0] == '/' && line[1] == '/') || line[0] == '\0' || line[0] == '\n' || line[0] == '\r')
			continue;
		// It's not necessary to remove 'carriage return ('\n' or '\r')
		for(j=0,p=line;j<MAX_LEVEL && p;j++){
			if (sscanf(p,"%d",&k)==0)
				break;
			job_bonus[1][i][j]=k;
			p=strchr(p,',');
			if (p) p++;
		}
		i++;
		if (i == MAX_PC_JOB_CLASS)
			break;
	}
	fclose(fp);
	// Show that the database has been successfully loaded in console
	printf(CL_GREEN "Loaded: " CL_RESET "'" CL_WHITE "db/job_db2-2.txt" CL_RESET "' read.\n");

	// <- End of Job Database 2-2 loading

	// Load Size Fix Database ->
	for(i=0;i<3;i++)
		for(j = 0; j < 23; j++)
			atkmods[i][j] = 100;
	fp=fopen("db/size_fix.txt","r");
	if (fp==NULL) {
		// Show that the database failed ot load in console
		printf(CL_RED "Error:" CL_RESET " Failed to load db/size_fix.txt\n");
		return 1;
	}
	i=0;
	// fgets reads until maximum one less than size and add '\0' -> so, it's not necessary to add -1
	while(fgets(line, sizeof(line), fp)) {
		char *split[25];
		if ((line[0] == '/' && line[1] == '/') || line[0] == '\0' || line[0] == '\n' || line[0] == '\r')
			continue;
		// It's not necessary to remove 'carriage return ('\n' or '\r')
		if (atoi(line)<=0)
			continue;
		memset(split,0,sizeof(split));
		for(j = 0, p = line; j < 23 && p; j++) {
			split[j] = p;
			p = strchr(p, ',');
			if (p) *p++ = 0;
		}
		for(j = 0; j < 23 && split[j]; j++)
			atkmods[i][j] = atoi(split[j]);
		i++;
	}
	fclose(fp);
	// Show that the database has been successfully loaded in console
	printf(CL_GREEN "Loaded: " CL_RESET "'" CL_WHITE "db/size_fix.txt" CL_RESET "' read ('" CL_WHITE "%d" CL_RESET "' entrie%s).\n", i, (i > 1) ? "s" : "");

	// <- End of Size Fix Database loading

	// Load Refine Database ->
	for(i=0;i<5;i++){
		for(j=0;j<10;j++)
			percentrefinery[i][j]=100;
		refinebonus[i][0]=0;
		refinebonus[i][1]=0;
		refinebonus[i][2]=10;
	}
	fp = fopen("db/refine_db.txt","r");
	if (fp == NULL) {
		// Show that the database failed ot load in console
		printf(CL_RED "Error:" CL_RESET " Failed to load db/refine_db.txt\n");
		return 1;
	}
	i=0;
	// fgets reads until maximum one less than size and add '\0' -> so, it's not necessary to add -1
	while(fgets(line, sizeof(line), fp)) {
		char *split[16];
		if ((line[0] == '/' && line[1] == '/') || line[0] == '\0' || line[0] == '\n' || line[0] == '\r')
			continue;
		// It's not necessary to remove 'carriage return ('\n' or '\r')
		if (atoi(line)<=0)
			continue;
		memset(split, 0, sizeof(split));
		for(j = 0, p = line; j < 16 && p; j++) {
			split[j] = p;
			p = strchr(p,',');
			if (p) *p++ = 0;
		}
		refinebonus[i][0] = atoi(split[0]);
		refinebonus[i][1] = atoi(split[1]);
		refinebonus[i][2] = atoi(split[2]);
		for(j = 0; j < 10 && split[j]; j++)
			percentrefinery[i][j] = atoi(split[j+3]);
		i++;
	}
	fclose(fp);
	// Show that the database has been successfully loaded in console
	printf(CL_GREEN "Loaded: " CL_RESET "'" CL_WHITE "db/refine_db.txt" CL_RESET "' read ('" CL_WHITE "%d" CL_RESET "' entrie%s).\n", i, (i > 1) ? "s" : "");

	// <- End of Refine Database loading

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int do_init_status(void) {

	if (SC_MAX > MAX_STATUSCHANGE) {
		// Check for invalid status changes (Status changes that come after SC_MAX in status.h)
		printf("ERROR: status.h defines %d status changes, but the MAX_STATUSCHANGE in map.h definition is %d! Fix it.\n", SC_MAX, MAX_STATUSCHANGE);
		exit(1);
	}
	initStatusIconTable();
	add_timer_func_list(status_change_timer, "status_change_timer");
	status_readdb();
	status_calc_sigma();

	return 0;
}
