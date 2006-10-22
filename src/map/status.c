// $Id: status.c 571 2005-12-01 23:19:59Z Yor $

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

int SkillStatusChangeTable[MAX_SKILL]; //Stores the status that should be associated to this skill.
int StatusIconTable[SC_MAX]; //Stores the client icons IDs. Every real SC has it's own icon.

void initStatusIconTable(void) {
	int i;
	for (i = 0; i < MAX_SKILL; i++)
		SkillStatusChangeTable[i] = -1;
	for (i = 0; i < SC_MAX; i++)
		StatusIconTable[i] = ICO_BLANK;

#define init_sc(skillid, sc_enum, iconid) \
	if(skillid < 0 || skillid > MAX_SKILL) printf("init_sc failed at skillid %d: skillid %d > MAX_SKILL\n", skillid, sc_enum); \
	else if(sc_enum < 0 || sc_enum > SC_MAX) printf("init_sc failed at skillid %d: sc_enum %d > SC_MAX\n", skillid, sc_enum); \
	else SkillStatusChangeTable[skillid] = sc_enum;	\
	     StatusIconTable[sc_enum] = iconid;

	//ENUM SKILLID skill.h       ENUM SC_ status.h     ENUM ICO_ status.h
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
	init_sc(ST_CHASEWALK,           SC_CHASEWALK,        ICO_CHASEWALK);
	init_sc(ST_REJECTSWORD,         SC_REJECTSWORD,      ICO_REJECTSWORD);
	init_sc(CG_MOONLIT,             SC_MOONLIT,          ICO_MOONLIT);
	init_sc(CG_MARIONETTE,          SC_MARIONETTE,       ICO_MARIONETTE);
	init_sc(LK_HEADCRUSH,           SC_BLEEDING,         ICO_BLEEDING);
	init_sc(LK_JOINTBEAT,           SC_JOINTBEAT,        ICO_JOINTBEAT);
	init_sc(PF_MINDBREAKER,         SC_MINDBREAKER,      ICO_BLANK);
	init_sc(PF_MEMORIZE,            SC_MEMORIZE,         ICO_BLANK);
	init_sc(PF_FOGWALL,             SC_FOGWALL,          ICO_BLANK);
	init_sc(PF_SPIDERWEB,           SC_SPIDERWEB,        ICO_BLANK);
	init_sc(TK_RUN,                 SC_RUN,              ICO_BLANK);
	init_sc(TK_READYSTORM,          SC_READYSTORM,       ICO_READYSTORM);
	init_sc(TK_READYDOWN,           SC_READYDOWN,        ICO_READYDOWN);
	init_sc(TK_READYTURN,           SC_READYTURN,        ICO_READYTURN);
	init_sc(TK_READYCOUNTER,        SC_READYCOUNTER,     ICO_READYCOUNTER);
	init_sc(TK_DODGE,               SC_READYDODGE,       ICO_DODGE);
	init_sc(TK_SPTIME,              SC_TKREST,           ICO_TKREST);
	init_sc(TK_SEVENWIND,           SC_GHOSTWEAPON,      ICO_GHOSTWEAPON);
	init_sc(TK_SEVENWIND,           SC_SHADOWWEAPON,     ICO_SHADOWWEAPON);
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

#undef init_sc

	//Misc status icons. Non-Skills
	StatusIconTable[SC_WEIGHT50]     = ICO_WEIGHT50;
	StatusIconTable[SC_WEIGHT90]     = ICO_WEIGHT90;
	StatusIconTable[SC_RIDING]       = ICO_RIDING;
	StatusIconTable[SC_FALCON]       = ICO_FALCON;
	StatusIconTable[SC_ASPDPOTION0]  = ICO_ASPDPOTION0;
	StatusIconTable[SC_ASPDPOTION1]  = ICO_ASPDPOTION1;
	StatusIconTable[SC_ASPDPOTION2]  = ICO_ASPDPOTION2;
	StatusIconTable[SC_ASPDPOTION3]  = ICO_ASPDPOTION3;
	StatusIconTable[SC_SPEEDUP0]     = ICO_SPEEDPOTION;
	StatusIconTable[SC_SPEEDUP1]     = ICO_SPEEDPOTION;

	//Guild skills have a base index of 10000. Therefore they dont fit in SkillStatusChangeTable as the MAX index is MAX_SKILL (1020)
	StatusIconTable[SC_BATTLEORDERS] = ICO_BLANK;
	StatusIconTable[SC_GUILDAURA]    = ICO_BLANK;

	if(!battle_config.display_hallucination) //Disable Hallucination (will not work on @setbattleflag)
		StatusIconTable[SC_HALLUCINATION] = ICO_BLANK;
}

static int refinebonus[5][3]; // ¸˜Bƒ{[ƒiƒXƒe[ƒuƒ‹(refine_db.txt)
int percentrefinery[5][10]; // ¸˜B¬Œ÷—¦(refine_db.txt)
static int max_weight_base[MAX_PC_JOB_CLASS];
static int hp_coefficient[MAX_PC_JOB_CLASS];
static int hp_coefficient2[MAX_PC_JOB_CLASS];
static int hp_sigma_val[MAX_PC_JOB_CLASS][MAX_LEVEL];
static int sp_coefficient[MAX_PC_JOB_CLASS];
static int aspd_base[MAX_PC_JOB_CLASS][20];
static char job_bonus[3][MAX_PC_JOB_CLASS][MAX_LEVEL];

static int atkmods[3][20];	// •ŠíATKƒTƒCƒYC³(size_fix.txt)

/*==========================================
 * ¸˜Bƒ{[ƒiƒX
 *------------------------------------------
 */
int status_getrefinebonus(int lv, int type) {
	if (lv >= 0 && lv < 5 && type >= 0 && type < 3)
		return refinebonus[lv][type];

	return 0;
}

/*==========================================
 * ¸˜B¬Œ÷—¦
 *------------------------------------------
 */
int status_percentrefinery(struct map_session_data *sd, struct item *item) {
	int percent;

	nullpo_retr(0, item);
	percent = percentrefinery[(int)itemdb_wlv(item->nameid)][(int)item->refine];

	percent += pc_checkskill(sd, BS_WEAPONRESEARCH); // •ŠíŒ¤‹†ƒXƒLƒ‹Š

	// Šm—¦‚Ì—LŒø”ÍˆÍƒ`ƒFƒbƒN
	if (percent > 100) {
		percent = 100;
	} else if (percent < 0) {
		percent = 0;
	}

	return percent;
}

/*==========================================
 * ƒpƒ‰ƒ[ƒ^ŒvZ
 * first==0‚ÌAŒvZ‘ÎÛ‚Ìƒpƒ‰ƒ[ƒ^‚ªŒÄ‚Ño‚µ‘O‚©‚ç
 * •Ï ‰»‚µ‚½ê‡©“®‚Åsend‚·‚é‚ªA
 * ”\“®“I‚É•Ï‰»‚³‚¹‚½ƒpƒ‰ƒ[ƒ^‚Í©‘O‚Åsend‚·‚é‚æ‚¤‚É
 *------------------------------------------
 */
int status_calc_pc(struct map_session_data* sd, int first) {
	int b_speed, b_max_hp, b_max_sp, b_hp, b_sp, b_weight, b_max_weight, b_paramb[6], b_parame[6], b_hit, b_flee;
	int b_aspd, b_watk, b_def, b_watk2, b_def2, b_flee2, b_critical, b_attackrange, b_matk1, b_matk2, b_mdef, b_mdef2, b_class;
	int b_base_atk;
	struct skill b_skill[MAX_SKILL];
	int i, bl, idx;
	int skill, aspd_rate, wele, wele_, def_ele, refinedef = 0;
	int pele = 0, pdef_ele = 0;
	int str, dstr, dex;
	struct pc_base_job s_class;

	nullpo_retr(0, sd);

	//“]¶‚â—{q‚Ìê‡‚ÌŒ³‚ÌE‹Æ‚ğZo‚·‚é
	s_class = pc_calc_base_job(sd->status.class);

	b_speed = sd->speed;
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

	pc_calc_skilltree(sd); // ƒXƒLƒ‹ƒcƒŠ[‚ÌŒvZ

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
		//sd->cart_max_num = MAX_CART; // it's always MAX_CART... removed
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
	sd->speed = DEFAULT_WALK_SPEED;
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
	memset(sd->expaddrace,0,sizeof(sd->expaddrace));
	memset(&sd->special_state,0,sizeof(sd->special_state));
	memset(sd->weapon_coma_ele,0,sizeof(sd->weapon_coma_ele));
	memset(sd->weapon_coma_race,0,sizeof(sd->weapon_coma_race));
	memset(sd->weapon_atk,0,sizeof(sd->weapon_atk));
	memset(sd->weapon_atk_rate,0,sizeof(sd->weapon_atk_rate));

	sd->watk_ = 0;			//“ñ“—¬—p(‰¼)
	sd->watk_2 = 0;
	sd->atk_ele_ = 0;
	sd->star_ = 0;
	sd->overrefine_ = 0;

	sd->aspd_rate = 100;
	sd->speed_rate = 100;
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
	sd->speed_add_rate = sd->aspd_add_rate = 100;
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

		if(sd->inventory_data[idx]) {
			if(sd->inventory_data[idx]->type == 4) {
				if(sd->status.inventory[idx].card[0]!=0x00ff && sd->status.inventory[idx].card[0]!=0x00fe && sd->status.inventory[idx].card[0]!=(short)0xff00) {
					int j;
					for(j=0;j<sd->inventory_data[idx]->slot;j++){	// ƒJ[ƒh
						int c=sd->status.inventory[idx].card[j];
						if(c>0){
							if(i == 8 && sd->status.inventory[idx].equip == 0x20)
								sd->state.lr_flag = 1;
							run_script(itemdb_equipscript(c),0,sd->bl.id,0);
							sd->state.lr_flag = 0;
						}
					}
				}
			}
			else if(sd->inventory_data[idx]->type==5){ // –h‹ï
				if(sd->status.inventory[idx].card[0]!=0x00ff && sd->status.inventory[idx].card[0]!=0x00fe && sd->status.inventory[idx].card[0]!=(short)0xff00) {
					int j;
					for(j=0;j<sd->inventory_data[idx]->slot;j++){	// ƒJ[ƒh
						int c=sd->status.inventory[idx].card[j];
						if(c>0)
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
//				run_script(sd->petDB->script, 0, sd->bl.id, 0);
			}
			pele = sd->atk_ele;
			pdef_ele = sd->def_ele;
			sd->atk_ele = sd->def_ele = 0;
		}
	}
	memcpy(sd->paramcard,sd->parame,sizeof(sd->paramcard));

	// ‘•”õ•i‚É‚æ‚éƒXƒe[ƒ^ƒX•Ï‰»‚Í‚±‚±‚ÅÀs
	for(i=0;i<10;i++) {
		idx = sd->equip_index[i];
		if(idx < 0)
			continue;
		if(i == 9 && sd->equip_index[8] == idx)
			continue;
		if(i == 5 && sd->equip_index[4] == idx)
			continue;
		if(i == 6 && (sd->equip_index[5] == idx || sd->equip_index[4] == idx))
			continue;
		if(sd->inventory_data[idx]) {
			sd->def += sd->inventory_data[idx]->def;
			if(sd->inventory_data[idx]->type == 4) {
				int r,wlv = sd->inventory_data[idx]->wlv;
				if(i == 8 && sd->status.inventory[idx].equip == 0x20) {
					//“ñ“—¬—pƒf[ƒ^“ü—Í
					sd->watk_ += sd->inventory_data[idx]->atk;
					sd->watk_2 = (r=sd->status.inventory[idx].refine)*	// ¸˜BUŒ‚—Í
						refinebonus[wlv][0];
					if( (r-=refinebonus[wlv][2])>0 )	// ‰ßè¸˜Bƒ{[ƒiƒX
						sd->overrefine_ = r*refinebonus[wlv][1];

					if(sd->status.inventory[idx].card[0] == 0x00ff) {	// »‘¢•Ší
						sd->star_ = (sd->status.inventory[idx].card[1]>>8);	// ¯‚Ì‚©‚¯‚ç
						wele_ = (sd->status.inventory[idx].card[1]&0x0f);	// ‘® «
						if(ranking_id2rank(*((unsigned long *)(&sd->status.inventory[idx].card[2])), RK_BLACKSMITH))
							sd->star += 10; //Forged weapons from the top10 blacksmith will receive an additional absolute damage of 10 (pierces vit/armor def) [Proximus]
					}
					sd->attackrange_ += sd->inventory_data[idx]->range;
					sd->state.lr_flag = 1;
					run_script(sd->inventory_data[idx]->equip_script,0,sd->bl.id,0);
					sd->state.lr_flag = 0;
				}
				else {	//“ñ“—¬•ŠíˆÈŠO
					sd->watk += sd->inventory_data[idx]->atk;
					sd->watk2 += (r = sd->status.inventory[idx].refine) *	// ¸˜BUŒ‚—Í
						refinebonus[wlv][0];
					if( (r-=refinebonus[wlv][2])>0 )	// ‰ßè¸˜Bƒ{[ƒiƒX
						sd->overrefine += r*refinebonus[wlv][1];

					if(sd->status.inventory[idx].card[0] == 0x00ff) {	// »‘¢•Ší
						sd->star += (sd->status.inventory[idx].card[1]>>8);	// ¯‚Ì‚©‚¯‚ç
						wele = (sd->status.inventory[idx].card[1]&0x0f);	// ‘® «
						if(ranking_id2rank(*((unsigned long *)(&sd->status.inventory[idx].card[2])), RK_BLACKSMITH))
							sd->star += 10; //Forged weapons from the top10 blacksmith will receive an additional absolute damage of 10 (pierces vit/armor def) [Proximus]
					}
					sd->attackrange += sd->inventory_data[idx]->range;
					run_script(sd->inventory_data[idx]->equip_script,0,sd->bl.id,0);
				}
			}
			else if (sd->inventory_data[idx]->type == 5) {
				sd->watk += sd->inventory_data[idx]->atk;
				refinedef += sd->status.inventory[idx].refine * refinebonus[0][0];
				run_script(sd->inventory_data[idx]->equip_script, 0, sd->bl.id, 0);
			}
		}
	}

	if(sd->equip_index[10] >= 0){ // –î
		idx = sd->equip_index[10];
		if(sd->inventory_data[idx]){		//‚Ü‚¾‘®«‚ª“ü‚Á‚Ä‚¢‚È‚¢
			sd->state.lr_flag = 2;
			run_script(sd->inventory_data[idx]->equip_script,0,sd->bl.id,0);
			sd->state.lr_flag = 0;
			sd->arrow_atk += sd->inventory_data[idx]->atk;
		}
	}
	sd->def += (refinedef+50)/100;

	if(sd->attackrange < 1) sd->attackrange = 1;
	if(sd->attackrange_ < 1) sd->attackrange_ = 1;
	if(sd->attackrange < sd->attackrange_)
		sd->attackrange = sd->attackrange_;
	if(sd->status.weapon == 11)
		sd->attackrange += sd->arrow_range;
	if(wele > 0)
		sd->atk_ele = wele;
	if(wele_ > 0)
		sd->atk_ele_ = wele_;
	if(def_ele > 0)
		sd->def_ele = def_ele;
	if(battle_config.pet_status_support) {
		if(pele > 0 && !sd->atk_ele)
			sd->atk_ele = pele;
		if(pdef_ele > 0 && !sd->def_ele)
			sd->def_ele = pdef_ele;
	}
	sd->double_rate += sd->double_add_rate;
	sd->perfect_hit += sd->perfect_hit_add;
	sd->get_zeny_num += sd->get_zeny_add_num;
	sd->splash_range += sd->splash_add_range;
	if(sd->speed_add_rate != 100)
		sd->speed_rate += sd->speed_add_rate - 100;
	if(sd->aspd_add_rate != 100)
		sd->aspd_rate += sd->aspd_add_rate - 100;

	// •ŠíATKƒTƒCƒY•â³ (‰Eè)
	sd->atkmods[0] = atkmods[0][sd->weapontype1];
	sd->atkmods[1] = atkmods[1][sd->weapontype1];
	sd->atkmods[2] = atkmods[2][sd->weapontype1];
	//•ŠíATKƒTƒCƒY•â³ (¶è)
	sd->atkmods_[0] = atkmods[0][sd->weapontype2];
	sd->atkmods_[1] = atkmods[1][sd->weapontype2];
	sd->atkmods_[2] = atkmods[2][sd->weapontype2];

	// jobƒ{[ƒiƒX•ª
	for(i = 0; i < sd->status.job_level && i < MAX_LEVEL; i++) {
		if (job_bonus[s_class.upper][s_class.job][i])
			sd->paramb[job_bonus[s_class.upper][s_class.job][i]-1]++;
	}

	if ((skill = pc_checkskill(sd, MC_INCCARRY)) > 0) // skill can be used with an item now, thanks to orn [Valaris]
		sd->max_weight += skill * 2000; // kRO 14/12/04 Patch: Every level now increases capacity of maximum weight by 200 instead of 100. [Aalye]

	if ((skill = pc_checkskill(sd, AC_OWL)) > 0) // ‚Ó‚­‚ë‚¤‚Ì–Ú
		sd->paramb[4] += skill;

	if ((skill = pc_checkskill(sd, BS_HILTBINDING)) > 0) { // Hilt binding gives +1 str +4 atk
		sd->paramb[0] ++;
		sd->base_atk += 4;
	}

	// kRO patch 14/12/04 - Dragonology give Int bonus every 2 level. [Aalye]
	if ((skill = pc_checkskill(sd, SA_DRAGONOLOGY)) > 0)
		sd->paramb[3] += (skill % 2 == 0) ? skill / 2 : (skill + 1) / 2;

	// TK_Run adds +10 ATK per level if no weapon is equipped [Tsuyuki]
	if ((skill = pc_checkskill(sd, TK_RUN)) > 0) {
		if (sd->status.weapon == 0)
			sd->base_atk += skill * 10;
		}

	if(sd->sc_count)
	{
		if (sd->sc_data[SC_INCSTR].timer != -1)
			sd->paramb[0] += sd->sc_data[SC_INCSTR].val1;
		if (sd->sc_data[SC_CONCENTRATE].timer != -1 && sd->sc_data[SC_QUAGMIRE].timer == -1) { // W’†—ÍŒüã
			sd->paramb[1] += (sd->status.agi + sd->paramb[1] + sd->parame[1] - sd->paramcard[1]) * (2 + sd->sc_data[SC_CONCENTRATE].val1) / 100;
			sd->paramb[4] += (sd->status.dex + sd->paramb[4] + sd->parame[4] - sd->paramcard[4]) * (2 + sd->sc_data[SC_CONCENTRATE].val1) / 100;
		}
		if (sd->sc_data[SC_INCREASEAGI].timer != -1 && sd->sc_data[SC_QUAGMIRE].timer == -1 && sd->sc_data[SC_DONTFORGETME].timer == -1) { // ‘¬“x‘‰Á
			sd->paramb[1] += 2 + sd->sc_data[SC_INCREASEAGI].val1;
			sd->speed -= sd->speed * 25 / 100;
		}
		if (sd->sc_data[SC_DECREASEAGI].timer != -1) {	// ‘¬“xŒ¸­(agi‚Íbattle.c‚Å)
			sd->speed = sd->speed * 125 / 100;
			sd->paramb[1] -= 2 + sd->sc_data[SC_DECREASEAGI].val1; // reduce agility [celest]
		}
		if (sd->sc_data[SC_CLOAKING].timer != -1) {
			sd->critical_rate += 100; // critical increases
			sd->speed = sd->speed * (sd->sc_data[SC_CLOAKING].val3 - sd->sc_data[SC_CLOAKING].val1 * 3) /100;
		}
		if (sd->sc_data[SC_CHASEWALK].timer != -1) {
			sd->speed = sd->speed * sd->sc_data[SC_CHASEWALK].val3 / 100; // slow down by chasewalk
			if (sd->sc_data[SC_CHASEWALK].val4)
				sd->paramb[0] += (1 << (sd->sc_data[SC_CHASEWALK].val1 - 1)); // increases strength after 10 seconds
		}
		if(sd->sc_data[SC_RUN].timer != -1) {
			sd->speed -= (sd->speed * 50) / 100;
		}
		if (sd->sc_data[SC_SLOWDOWN].timer!=-1)
			sd->speed = sd->speed * 150 / 100;
		if (sd->sc_data[SC_SPEEDUP0].timer!=-1)
			sd->speed -= sd->speed * 25 / 100;
		if (sd->sc_data[SC_BLESSING].timer!=-1) { // ƒuƒŒƒbƒVƒ“ƒO
			sd->paramb[0] += sd->sc_data[SC_BLESSING].val1;
			sd->paramb[3] += sd->sc_data[SC_BLESSING].val1;
			sd->paramb[4] += sd->sc_data[SC_BLESSING].val1;
		}
		if(sd->sc_data[SC_GLORIA].timer!=-1)	// ƒOƒƒŠƒA
			sd->paramb[5]+= 30;
		if(sd->sc_data[SC_LOUD].timer!=-1 && sd->sc_data[SC_QUAGMIRE].timer == -1)	// ƒ‰ƒEƒhƒ{ƒCƒX
			sd->paramb[0]+= 4;
		if(sd->sc_data[SC_SPURT].timer != -1) {
			sd->paramb[0]+= 10;
		}
		if(sd->sc_data[SC_QUAGMIRE].timer!=-1){	// ƒNƒ@ƒOƒ}ƒCƒA
			//int agib = (sd->status.agi+sd->paramb[1]+sd->parame[1])*(sd->sc_data[SC_QUAGMIRE].val1*10)/100;
			//int dexb = (sd->status.dex+sd->paramb[4]+sd->parame[4])*(sd->sc_data[SC_QUAGMIRE].val1*10)/100;
			//sd->paramb[1]-= agib > 50 ? 50 : agib;
			//sd->paramb[4]-= dexb > 50 ? 50 : dexb;
			sd->paramb[1]-= sd->sc_data[SC_QUAGMIRE].val1*5;
			sd->paramb[4]-= sd->sc_data[SC_QUAGMIRE].val1*5;
			sd->speed = sd->speed*3/2;
		}
		if(sd->sc_data[SC_TRUESIGHT].timer!=-1){	// ƒgƒDƒ‹[ƒTƒCƒg
			sd->paramb[0]+= 5;
			sd->paramb[1]+= 5;
			sd->paramb[2]+= 5;
			sd->paramb[3]+= 5;
			sd->paramb[4]+= 5;
			sd->paramb[5]+= 5;
		}

		if(sd->sc_data[SC_MARIONETTE].timer != -1) {
			sd->paramb[0]-= sd->status.str >> 1;
			sd->paramb[1]-= sd->status.agi >> 1;
			sd->paramb[2]-= sd->status.vit >> 1;
			sd->paramb[3]-= sd->status.int_>> 1;
			sd->paramb[4]-= sd->status.dex >> 1;
			sd->paramb[5]-= sd->status.luk >> 1;
		} else if (sd->sc_data[SC_MARIONETTE2].timer != -1) {
			struct map_session_data *psd = (struct map_session_data *)map_id2bl(sd->sc_data[SC_MARIONETTE2].val3);
			if(psd)	{
				sd->paramb[0] += sd->status.str + (psd->status.str >> 1) > 99 ? 99 - sd->status.str : (psd->status.str >> 1);
				sd->paramb[1] += sd->status.agi + (psd->status.agi >> 1) > 99 ? 99 - sd->status.agi : (psd->status.agi >> 1);
				sd->paramb[2] += sd->status.vit + (psd->status.vit >> 1) > 99 ? 99 - sd->status.vit : (psd->status.vit >> 1);
				sd->paramb[3] += sd->status.int_+ (psd->status.int_>> 1) > 99 ? 99 - sd->status.int_: (psd->status.int_>> 1);
				sd->paramb[4] += sd->status.dex + (psd->status.dex >> 1) > 99 ? 99 - sd->status.dex : (psd->status.dex >> 1);
				sd->paramb[5] += sd->status.luk + (psd->status.luk >> 1) > 99 ? 99 - sd->status.luk : (psd->status.luk >> 1);
			}
		}

		if(sd->sc_data[SC_BATTLEORDERS].timer != -1)
		{
			sd->paramb[0] += 5;
			sd->paramb[3] += 5;
			sd->paramb[4] += 5;
		}

		if(sd->sc_data[SC_GUILDAURA].timer != -1)
		{
			if(sd->sc_data[SC_GUILDAURA].val4 & 1 << 0)
				sd->paramb[0] += 2;
			if(sd->sc_data[SC_GUILDAURA].val4 & 1 << 1)
				sd->paramb[2] += 2;
			if(sd->sc_data[SC_GUILDAURA].val4 & 1 << 2)
				sd->paramb[1] += 2;
			if(sd->sc_data[SC_GUILDAURA].val4 & 1 << 3)
				sd->paramb[4] += 2;
		}

		if(sd->sc_data[SC_STRFOOD].timer != -1)
			sd->paramb[0] += sd->sc_data[SC_STRFOOD].val1;
		if(sd->sc_data[SC_AGIFOOD].timer != -1)
			sd->paramb[1] += sd->sc_data[SC_AGIFOOD].val1;
		if(sd->sc_data[SC_VITFOOD].timer != -1)
			sd->paramb[2] += sd->sc_data[SC_VITFOOD].val1;
		if(sd->sc_data[SC_INTFOOD].timer != -1)
			sd->paramb[3] += sd->sc_data[SC_INTFOOD].val1;
		if(sd->sc_data[SC_DEXFOOD].timer != -1)
			sd->paramb[4] += sd->sc_data[SC_DEXFOOD].val1;
		if(sd->sc_data[SC_LUKFOOD].timer != -1)
			sd->paramb[5] += sd->sc_data[SC_LUKFOOD].val1;
		if(sd->sc_data[SC_HITFOOD].timer != -1)
			sd->hit += sd->sc_data[SC_HITFOOD].val1;
		if(sd->sc_data[SC_FLEEFOOD].timer != -1)
			sd->flee += sd->sc_data[SC_FLEEFOOD].val1;
		if(sd->sc_data[SC_BATKFOOD].timer != -1)
			sd->base_atk += sd->sc_data[SC_BATKFOOD].val1;
//		if(sd->sc_data[SC_WATKFOOD].timer != -1)
//			sd->weapon_atk += sd->sc_data[SC_WATKFOOD].val1;
		if(sd->sc_data[SC_MATKFOOD].timer != -1)
			sd->matk1 += sd->sc_data[SC_MATKFOOD].val1;

		if(sd->sc_data[SC_INCALLSTATUS].timer != -1)
		{
			sd->paramb[0] += sd->sc_data[SC_INCALLSTATUS].val1;
			sd->paramb[1] += sd->sc_data[SC_INCALLSTATUS].val1;
			sd->paramb[2] += sd->sc_data[SC_INCALLSTATUS].val1;
			sd->paramb[3] += sd->sc_data[SC_INCALLSTATUS].val1;
			sd->paramb[4] += sd->sc_data[SC_INCALLSTATUS].val1;
			sd->paramb[5] += sd->sc_data[SC_INCALLSTATUS].val1;
		}
	}

	// super novice's special stat bonus
	if(s_class.job == 23)
	{
		if(pc_readglobalreg(sd, "SN_EXSTAT"))
		{
			if(sd->status.base_level >= 99)
			{
				sd->paramb[0] += 10;
				sd->paramb[1] += 10;
				sd->paramb[2] += 10;
				sd->paramb[3] += 10;
				sd->paramb[4] += 10;
				sd->paramb[5] += 10;
			} else {
				pc_setglobalreg(sd, "SN_EXSTAT", 0);
			}
		} else {
			if(sd->status.base_level >= 99 && sd->status.die_counter == 0)
			{
				sd->paramb[0] += 10;
				sd->paramb[1] += 10;
				sd->paramb[2] += 10;
				sd->paramb[3] += 10;
				sd->paramb[4] += 10;
				sd->paramb[5] += 10;
				pc_setglobalreg(sd, "SN_EXSTAT", 1);
			}
		}
	}

	sd->paramc[0]=sd->status.str+sd->paramb[0]+sd->parame[0];
	sd->paramc[1]=sd->status.agi+sd->paramb[1]+sd->parame[1];
	sd->paramc[2]=sd->status.vit+sd->paramb[2]+sd->parame[2];
	sd->paramc[3]=sd->status.int_+sd->paramb[3]+sd->parame[3];
	sd->paramc[4]=sd->status.dex+sd->paramb[4]+sd->parame[4];
	sd->paramc[5]=sd->status.luk+sd->paramb[5]+sd->parame[5];

	for(i = 0; i < 6; i++) {
		if(sd->paramc[i]
			< 0) sd->paramc[i] = 0;
	}

	if (sd->sc_count) {
		if (sd->sc_data[SC_CURSE].timer != -1)
			sd->paramc[5] = 0;
	}

	if(sd->status.weapon == 11 || sd->status.weapon == 13 || sd->status.weapon == 14) {
		str = sd->paramc[4];
		dex = sd->paramc[0];
	}
	else {
		str = sd->paramc[0];
		dex = sd->paramc[4];
	}
	dstr = str/10;
	sd->base_atk += str + dstr*dstr + dex/5 + sd->paramc[5]/5;
	sd->matk1 += sd->paramc[3]+(sd->paramc[3]/5)*(sd->paramc[3]/5);
	sd->matk2 += sd->paramc[3]+(sd->paramc[3]/7)*(sd->paramc[3]/7);
	if(sd->matk1 < sd->matk2) {
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

	if(sd->base_atk < 1)
		sd->base_atk = 1;
	if(sd->critical_rate != 100)
		sd->critical = (sd->critical*sd->critical_rate)/100;
	if(sd->critical < 10) sd->critical = 10;
	if(sd->hit_rate != 100)
		sd->hit = (sd->hit*sd->hit_rate)/100;
	if(sd->hit < 1) sd->hit = 1;
	if(sd->flee_rate != 100)
		sd->flee = (sd->flee*sd->flee_rate)/100;
	if(sd->flee < 1) sd->flee = 1;
	if(sd->flee2_rate != 100)
		sd->flee2 = (sd->flee2*sd->flee2_rate)/100;
	if(sd->flee2 < 10) sd->flee2 = 10;
	if(sd->def_rate != 100)
		sd->def = (sd->def*sd->def_rate)/100;
	if(sd->def < 0) sd->def = 0;
	if(sd->def2_rate != 100)
		sd->def2 = (sd->def2*sd->def2_rate)/100;
	if(sd->def2 < 1) sd->def2 = 1;
	if(sd->mdef_rate != 100)
		sd->mdef = (sd->mdef*sd->mdef_rate)/100;
	if(sd->mdef < 0) sd->mdef = 0;
	if(sd->mdef2_rate != 100)
		sd->mdef2 = (sd->mdef2*sd->mdef2_rate)/100;
	if(sd->mdef2 < 1) sd->mdef2 = 1;

	// “ñ“—¬ ASPD C³
	if (sd->status.weapon <= 16)
		// kRO 14/12/04 Patch - Each level of Book mastery give 0.5% attack speed with book [Aalye]
		if (sd->status.weapon == 15 && pc_checkskill(sd, SA_ADVANCEDBOOK) > 0)
			sd->aspd += (aspd_base[s_class.job][sd->status.weapon] - (sd->paramc[1] * 4 + sd->paramc[4] + pc_checkskill(sd, SA_ADVANCEDBOOK) * 5) * aspd_base[s_class.job][sd->status.weapon] / 1000);
		else
			sd->aspd +=  aspd_base[s_class.job][sd->status.weapon] - (sd->paramc[1] * 4 + sd->paramc[4]) * aspd_base[s_class.job][sd->status.weapon] / 1000;
	else
		sd->aspd += ((aspd_base[s_class.job][sd->weapontype1] - (sd->paramc[1] * 4 + sd->paramc[4]) * aspd_base[s_class.job][sd->weapontype1] / 1000) +
		             (aspd_base[s_class.job][sd->weapontype2] - (sd->paramc[1] * 4 + sd->paramc[4]) * aspd_base[s_class.job][sd->weapontype2] / 1000))
		            * 140 / 200;

	aspd_rate = sd->aspd_rate;

	//UŒ‚‘¬“x‘‰Á

	if ((skill = pc_checkskill(sd, AC_VULTURE)) > 0) { // ƒƒV‚Ì–Ú
		sd->hit += skill;
		if (sd->status.weapon == 11)
			sd->attackrange += skill;
	}
			
	if((skill = pc_checkskill(sd, BS_WEAPONRESEARCH)) > 0)	// •ŠíŒ¤‹†‚Ì–½’†—¦‘‰Á
		sd->hit += skill << 1;
	if(sd->status.option&2 && (skill = pc_checkskill(sd, RG_TUNNELDRIVE)) > 0 )
		sd->speed += sd->speed * (100 - 16 * skill) / 100;
	if (pc_iscarton(sd) && (skill = pc_checkskill(sd, MC_PUSHCART)) > 0)
		sd->speed += sd->speed * (100 - 10 * skill) / 100;
	else if (pc_isriding(sd) && pc_checkskill(sd, KN_RIDING) > 0) {
		sd->speed -= sd->speed >> 2;
		sd->max_weight += 10000;
	}

	if ((skill = pc_checkskill(sd, BS_SKINTEMPER)) > 0) {
		sd->subele[0] += skill;
		sd->subele[3] += skill * 5;
	}
	if ((skill = pc_checkskill(sd, SA_ADVANCEDBOOK)) > 0)
		aspd_rate -= skill >> 1;

	bl = sd->status.base_level;
	idx = (3500 + bl * hp_coefficient2[s_class.job] + hp_sigma_val[s_class.job][(bl > 0)? bl-1:0])/100 * (100 + sd->paramc[2])/100 + (sd->parame[2] - sd->paramcard[2]);
	if (s_class.upper == 1) // [MouseJstr]
		idx = idx * 130 / 100;
	else if (s_class.upper == 2)
		idx = idx * 70 / 100;

	if((skill = pc_checkskill(sd, CR_TRUST)) > 0) { // ƒtƒFƒCƒX
		sd->status.max_hp += skill * 200; // Passive skill, maxHP modifier
		sd->subele[6] += skill * 5;
	}	

	// here we update sd->status.max_hp with the final calculation of hp according job class hp coefficient, etc
	// because when there was a maxhp item modifier (eg pupa +700 hp) the total hp after equip pupa was 700*130% = 910 HP
	// which is incorrect, HP item modifiers should be applied after doing all the calculation [Proximus]
	sd->status.max_hp += idx;
	
	if(sd->hprate != 100)
		sd->status.max_hp = sd->status.max_hp * sd->hprate / 100;
	
	if(sd->sc_count && sd->sc_data[SC_BERSERK].timer != -1) {	// ƒo[ƒT[ƒN
		sd->status.max_hp = sd->status.max_hp * 3;
		//sd->status.hp = sd->status.hp * 3; // removed by [Yor] - bug fixe
		if(sd->status.max_hp > battle_config.max_hp) // removed negative max hp bug by Valaris
		sd->status.max_hp = battle_config.max_hp;
		if(sd->status.hp > battle_config.max_hp) // removed negative max hp bug by Valaris
		sd->status.hp = battle_config.max_hp;
	}
	if(s_class.job == 23 && sd->status.base_level >= 99){
		sd->status.max_hp = sd->status.max_hp + 2000;
	}

	if (sd->status.max_hp > battle_config.max_hp) // removed negative max hp bug by Valaris
		sd->status.max_hp = battle_config.max_hp;
	if (sd->status.max_hp <= 0) sd->status.max_hp = 1; // end

	// Å‘åSPŒvZ
	idx = ((sp_coefficient[s_class.job] * bl) + 1000)/100 * (100 + sd->paramc[3])/100 + (sd->parame[3] - sd->paramcard[3]);
	if (s_class.upper == 1) // [MouseJstr]
		idx = idx * 130 / 100;
	else if (s_class.upper == 2)
		idx = idx * 70 / 100;
	
	// here we update sd->status.max_sp with the final calculation of hp according job class
	// because when there was a maxsp item modifier (eg willow +80 maxsp) the total sp after equip willow was 80*130% = 104 SP
	// which is incorrect, sp item modifiers should be applied after doing all the calculation [Proximus]
	sd->status.max_sp += idx;

	if((skill=pc_checkskill(sd,HP_MEDITATIO)) > 0) // Passive skill, maxSP modifier
		sd->status.max_sp += sd->status.max_sp * skill / 100;
	if((skill=pc_checkskill(sd,HW_SOULDRAIN))>0) // Passive skill, maxSP modifier
		sd->status.max_sp += sd->status.max_sp * 2 * skill / 100;

	if(sd->sprate!=100)
		sd->status.max_sp = sd->status.max_sp * sd->sprate / 100;

	if(sd->status.max_sp < 0 || sd->status.max_sp > battle_config.max_sp)
		sd->status.max_sp = battle_config.max_sp;

	//©‘R‰ñ•œHP
	sd->nhealhp = 1 + (sd->paramc[2]/5) + (sd->status.max_hp/200);
	if((skill=pc_checkskill(sd,SM_RECOVERY)) > 0) {	/* HP‰ñ•œ—ÍŒüã */
		sd->nshealhp = skill*5 + (sd->status.max_hp*skill/500);
		if(sd->nshealhp > 0x7fff) sd->nshealhp = 0x7fff;
	}
	//©‘R‰ñ•œSP
	sd->nhealsp = 1 + (sd->paramc[3]/6) + (sd->status.max_sp/100);
	if(sd->paramc[3] >= 120)
		sd->nhealsp += ((sd->paramc[3] - 120) >> 1) + 4;
	if((skill=pc_checkskill(sd,MG_SRECOVERY)) > 0) { /* SP‰ñ•œ—ÍŒüã */
		sd->nshealsp = skill*3 + (sd->status.max_sp*skill/500);
		if(sd->nshealsp > 0x7fff) sd->nshealsp = 0x7fff;
	}

	if((skill = pc_checkskill(sd,MO_SPIRITSRECOVERY)) > 0) {
		sd->nsshealhp = skill*4 + (sd->status.max_hp*skill/500);
		sd->nsshealsp = skill*2 + (sd->status.max_sp*skill/500);
		if(sd->nsshealhp > 0x7fff) sd->nsshealhp = 0x7fff;
		if(sd->nsshealsp > 0x7fff) sd->nsshealsp = 0x7fff;
	}

	if((skill=pc_checkskill(sd,TK_HPTIME)) > 0 && sd->state.rest) {
		sd->nsshealhp = skill*30 + (sd->status.max_hp*skill/500);
		if(sd->nsshealhp > 0x7fff) sd->nsshealhp = 0x7fff;
	}

	if((skill=pc_checkskill(sd,TK_SPTIME)) > 0 && sd->state.rest) {
		sd->nsshealsp = skill*3 + (sd->status.max_sp*skill/500);
		if (sd->nsshealsp > 0x7fff) sd->nsshealsp = 0x7fff;
	}

	if(sd->hprecov_rate != 100) {
		sd->nhealhp = sd->nhealhp*sd->hprecov_rate/100;
		if(sd->nhealhp < 1) sd->nhealhp = 1;
	}
	if(sd->sprecov_rate != 100) {
		sd->nhealsp = sd->nhealsp*sd->sprecov_rate/100;
		if(sd->nhealsp < 1) sd->nhealsp = 1;
	}
	/*if((skill=pc_checkskill(sd,HP_MEDITATIO)) > 0) { // ƒƒfƒBƒeƒCƒeƒBƒI‚ÍSPR‚Å‚Í‚È‚­©‘R‰ñ•œ‚É‚©‚©‚é
		sd->nhealsp += 3 * skill * (sd->status.max_sp) / 1000; // fixed by [Yor]
		if (sd->nhealsp > 0x7fff) sd->nhealsp = 0x7fff;
	}*/

	// í‘°‘Ï«i‚±‚ê‚Å‚¢‚¢‚ÌH ƒfƒBƒoƒCƒ“ƒvƒƒeƒNƒVƒ‡ƒ“‚Æ“¯‚¶ˆ—‚ª‚¢‚é‚©‚àj
	if( (skill=pc_checkskill(sd,SA_DRAGONOLOGY))>0 ){ // ƒhƒ‰ƒSƒmƒƒW[
		skill = skill*4;
		sd->addrace[9]+=skill;
		sd->addrace_[9]+=skill;
		sd->subrace[9]+=skill;
		sd->magic_addrace[9]+=skill;
		sd->magic_subrace[9]-=skill;
	}

	//Fleeã¸
	if ((skill = pc_checkskill(sd, TF_MISS)) > 0) {	// ‰ñ”ğ—¦‘‰Á
		sd->flee += skill * ((s_class.job == JOB_ASSASSIN || s_class.job == JOB_ROGUE)? 4 : 3);
		if (s_class.job == JOB_ASSASSIN && sd->sc_data[SC_CLOAKING].timer == -1)
			sd->speed -= sd->speed * skill / 100;
	}
	if ((skill = pc_checkskill(sd, MO_DODGE)) > 0)	// Œ©Ø‚è
		sd->flee += (skill * 3) >> 1;

	if (map[sd->bl.m].flag.gvg) //GVG grounds flee penalty,
		sd->flee -= sd->flee * battle_config.gvg_flee_penalty / 100;
		
	// ƒXƒLƒ‹‚âƒXƒe[ƒ^ƒXˆÙí‚É‚æ‚éc‚è‚Ìƒpƒ‰ƒ[ƒ^•â³
	if(sd->sc_count) {
		// ATK/DEF•Ï‰»Œ`
		if(sd->sc_data[SC_ANGELUS].timer!=-1)	// ƒGƒ“ƒWƒFƒ‰ƒX
			sd->def2 = sd->def2*(110+5*sd->sc_data[SC_ANGELUS].val1)/100;
		if(sd->sc_data[SC_IMPOSITIO].timer!=-1)	{// ƒCƒ“ƒ|ƒVƒeƒBƒIƒ}ƒkƒX
			sd->watk += sd->sc_data[SC_IMPOSITIO].val1*5;
			idx = sd->equip_index[8];
			if(idx >= 0 && sd->inventory_data[idx] && sd->inventory_data[idx]->type == 4)
				sd->watk_ += sd->sc_data[SC_IMPOSITIO].val1*5;
		}
		if(sd->sc_data[SC_PROVOKE].timer!=-1){	// ƒvƒƒ{ƒbƒN
			// Provoke should only reduce the vit def of a player, not vit def and armor def. [Bison]
			// Corrected the formula according to kRO site, as provoke does 10% reduction at level 1 and 55% at level 10. [Bison]
			sd->def2 = sd->def2*(100 - (5 * sd->sc_data[SC_PROVOKE].val1 + 5))/100;
			// Corrected the +atk% formula from kRO site, provoke 1 does +5% atk increase, and +3% per level, so provoke 10 = +32%. [Bison]
			sd->base_atk = sd->base_atk*(100 + (3 * sd->sc_data[SC_PROVOKE].val1 + 2))/100;
			sd->watk = sd->watk*(100 + (3 * sd->sc_data[SC_PROVOKE].val1 + 2))/100;
			idx = sd->equip_index[8];
			if(idx >= 0 && sd->inventory_data[idx] && sd->inventory_data[idx]->type == 4)
				sd->watk_ = sd->watk_*(100 + (3 * sd->sc_data[SC_PROVOKE].val1 + 2))/100;
		}
		if(sd->sc_data[SC_ENDURE].timer!=-1)
			sd->mdef += sd->sc_data[SC_ENDURE].val1;
		if(sd->sc_data[SC_MINDBREAKER].timer!=-1){	// ƒvƒƒ{ƒbƒN
			sd->mdef2 = sd->mdef2*(100-6*sd->sc_data[SC_MINDBREAKER].val1)/100;
			sd->matk1 = sd->matk1*(100+2*sd->sc_data[SC_MINDBREAKER].val1)/100;
			sd->matk2 = sd->matk2*(100+2*sd->sc_data[SC_MINDBREAKER].val1)/100;
		}
		if (sd->sc_data[SC_POISON].timer != -1)	// “Åó‘Ô
			sd->def2 = sd->def2 * 75 / 100;
		if (sd->sc_data[SC_CURSE].timer != -1) {
			sd->base_atk = sd->base_atk * 75 / 100;
			sd->watk = sd->watk * 75 / 100;
			idx = sd->equip_index[8];
			if (idx >= 0 && sd->inventory_data[idx] && sd->inventory_data[idx]->type == 4)
				sd->watk_ = sd->watk_ * 75 / 100;
		}
		if(sd->sc_data[SC_BLEEDING].timer != -1) {
			sd->base_atk -= sd->base_atk >> 2;
			sd->aspd_rate += 25;
		}
		if (sd->sc_data[SC_DRUMBATTLE].timer != -1){	// í‘¾ŒÛ‚Ì‹¿‚«
			sd->watk += sd->sc_data[SC_DRUMBATTLE].val2;
			sd->def  += sd->sc_data[SC_DRUMBATTLE].val3;
			idx = sd->equip_index[8];
			if (idx >= 0 && sd->inventory_data[idx] && sd->inventory_data[idx]->type == 4)
				sd->watk_ += sd->sc_data[SC_DRUMBATTLE].val2;
		}
		if(sd->sc_data[SC_NIBELUNGEN].timer!=-1) {	// ƒj[ƒxƒ‹ƒ“ƒO‚Ìw—Ö
			idx = sd->equip_index[9];
			/*if(idx >= 0 && sd->inventory_data[idx] && sd->inventory_data[idx]->wlv == 3)
				sd->watk += sd->sc_data[SC_NIBELUNGEN].val3;
			idx = sd->equip_index[8];
			if(idx >= 0 && sd->inventory_data[idx] && sd->inventory_data[idx]->wlv == 3)
				sd->watk_ += sd->sc_data[SC_NIBELUNGEN].val3;
			idx = sd->equip_index[9];*/
			if(idx >= 0 && sd->inventory_data[idx] && sd->inventory_data[idx]->wlv == 4)
				sd->watk2 += sd->sc_data[SC_NIBELUNGEN].val3;
			idx = sd->equip_index[8];
			if(idx >= 0 && sd->inventory_data[idx] && sd->inventory_data[idx]->wlv == 4)
				sd->watk_2 += sd->sc_data[SC_NIBELUNGEN].val3;
		}

		if(sd->sc_data[SC_VOLCANO].timer!=-1 && sd->def_ele==3){	// ƒ{ƒ‹ƒP[ƒm
			sd->watk += sd->sc_data[SC_VIOLENTGALE].val3;
		}

		if(sd->sc_data[SC_SIGNUMCRUCIS].timer!=-1)
			sd->def = sd->def * (100 - sd->sc_data[SC_SIGNUMCRUCIS].val2)/100;
		if(sd->sc_data[SC_ETERNALCHAOS].timer!=-1) { // ƒGƒ^[ƒiƒ‹ƒJƒIƒX
			sd->def = 0;
			sd->def2 = 0;
		}

		if(sd->sc_data[SC_CONCENTRATION].timer!=-1){ //ƒRƒ“ƒZƒ“ƒgƒŒ[ƒVƒ‡ƒ“
			sd->base_atk = sd->base_atk * (100 + 5 * sd->sc_data[SC_CONCENTRATION].val1) / 100;
			sd->watk = sd->watk * (100 + 5 * sd->sc_data[SC_CONCENTRATION].val1) / 100;
			sd->watk2 = sd->watk2 * (100 + 5 * sd->sc_data[SC_CONCENTRATION].val1) / 100;
			sd->def = sd->def * (100 - 5 * sd->sc_data[SC_CONCENTRATION].val1) / 100;
			sd->def2 = sd->def2 * (100 - 5 * sd->sc_data[SC_CONCENTRATION].val1) / 100;
		}

		if(sd->sc_data[SC_MAGICPOWER].timer!=-1){ //–‚–@—Í‘•
			sd->matk1 = sd->matk1*(100+5*sd->sc_data[SC_MAGICPOWER].val1)/100;
			sd->matk2 = sd->matk2*(100+5*sd->sc_data[SC_MAGICPOWER].val1)/100;
		}
		if(sd->sc_data[SC_ATKPOT].timer!=-1)
			sd->watk += sd->sc_data[SC_ATKPOT].val1;
		if(sd->sc_data[SC_MATKPOT].timer!=-1){
			sd->matk1 += sd->sc_data[SC_MATKPOT].val1;
			sd->matk2 += sd->sc_data[SC_MATKPOT].val1;
		}

		// ASPD/ˆÚ“®‘¬“x•Ï‰»Œn
		if(sd->sc_data[SC_TWOHANDQUICKEN].timer != -1 && sd->sc_data[SC_QUAGMIRE].timer == -1 && sd->sc_data[SC_DONTFORGETME].timer == -1)	// 2HQ
			aspd_rate -= 30;
		if(sd->sc_data[SC_ADRENALINE].timer != -1 && sd->sc_data[SC_TWOHANDQUICKEN].timer == -1 &&
			sd->sc_data[SC_QUAGMIRE].timer == -1 && sd->sc_data[SC_DONTFORGETME].timer == -1) {	// ƒAƒhƒŒƒiƒŠƒ“ƒ‰ƒbƒVƒ…
			if(sd->sc_data[SC_ADRENALINE].val2 || !battle_config.party_skill_penalty)
				aspd_rate -= 30;
			else
				aspd_rate -= 25;
		}
		if(sd->sc_data[SC_SPEARQUICKEN].timer != -1 && sd->sc_data[SC_ADRENALINE].timer == -1 &&
			sd->sc_data[SC_TWOHANDQUICKEN].timer == -1 && sd->sc_data[SC_QUAGMIRE].timer == -1 && sd->sc_data[SC_DONTFORGETME].timer == -1)	// ƒXƒsƒAƒNƒBƒbƒPƒ“
			aspd_rate -= sd->sc_data[SC_SPEARQUICKEN].val2;
		if(sd->sc_data[SC_ASSNCROS].timer!=-1 && // —[—z‚ÌƒAƒTƒVƒ“ƒNƒƒX
			sd->sc_data[SC_TWOHANDQUICKEN].timer == -1 && sd->sc_data[SC_ADRENALINE].timer == -1 && sd->sc_data[SC_SPEARQUICKEN].timer == -1 &&
			sd->sc_data[SC_DONTFORGETME].timer == -1)
				aspd_rate -= 5+sd->sc_data[SC_ASSNCROS].val1+sd->sc_data[SC_ASSNCROS].val2+sd->sc_data[SC_ASSNCROS].val3;
		if(sd->sc_data[SC_GRAVITATION].timer != -1)
			aspd_rate += sd->sc_data[SC_GRAVITATION].val2;
		if(sd->sc_data[SC_DONTFORGETME].timer!=-1){		// „‚ğ–Y‚ê‚È‚¢‚Å
			aspd_rate += sd->sc_data[SC_DONTFORGETME].val1 * 3 + sd->sc_data[SC_DONTFORGETME].val2 + (sd->sc_data[SC_DONTFORGETME].val3 >> 16);
			sd->speed = sd->speed * (100 + sd->sc_data[SC_DONTFORGETME].val1 * 2 + sd->sc_data[SC_DONTFORGETME].val2 + (sd->sc_data[SC_DONTFORGETME].val3 & 0xffff)) / 100;
		}
		if (sd->sc_data[i=SC_ASPDPOTION3].timer != -1 ||
		    sd->sc_data[i=SC_ASPDPOTION2].timer != -1 ||
		    sd->sc_data[i=SC_ASPDPOTION1].timer != -1 ||
		    sd->sc_data[i=SC_ASPDPOTION0].timer != -1) // ‘ ‘¬ƒ|[ƒVƒ‡ƒ“
			aspd_rate -= sd->sc_data[i].val2;
		if (sd->sc_data[SC_WINDWALK].timer != -1 && sd->sc_data[SC_INCREASEAGI].timer == -1) //ƒEƒBƒ“ƒhƒEƒH?ƒN‚ÍLv*2%Œ¸Z
			sd->speed -= sd->speed * (sd->sc_data[SC_WINDWALK].val1 * 2) / 100;
		if (sd->sc_data[SC_CARTBOOST].timer != -1) // ƒJ?ƒgƒu?ƒXƒg
		sd->speed -= (DEFAULT_WALK_SPEED * 20)/100;
		if (sd->sc_data[SC_BERSERK].timer != -1) //ƒo?ƒT?ƒN’†‚ÍIA‚Æ“¯‚¶‚®‚ç‚¢‘¬‚¢H
			sd->speed -= sd->speed * 25 / 100;
		if (sd->sc_data[SC_WEDDING].timer != -1) //Œ‹¥’†‚Í?‚­‚Ì‚ª?‚¢
			sd->speed = 2*DEFAULT_WALK_SPEED;

		// HIT/FLEE•Ï‰»Œn
		if(sd->sc_data[SC_WHISTLE].timer!=-1){  // Œû“J
			sd->flee += sd->flee * (sd->sc_data[SC_WHISTLE].val1
			          + sd->sc_data[SC_WHISTLE].val2 + (sd->sc_data[SC_WHISTLE].val3 >> 16)) / 100;
			sd->flee2+= (sd->sc_data[SC_WHISTLE].val1+sd->sc_data[SC_WHISTLE].val2+(sd->sc_data[SC_WHISTLE].val3&0xffff)) * 10;
		}
		if(sd->sc_data[SC_HUMMING].timer!=-1)  // ƒnƒ~ƒ“ƒO
			sd->hit += (sd->sc_data[SC_HUMMING].val1*2+sd->sc_data[SC_HUMMING].val2
					+sd->sc_data[SC_HUMMING].val3) * sd->hit/100;
		if(sd->sc_data[SC_VIOLENTGALE].timer!=-1 && sd->def_ele==4){	// ƒoƒCƒIƒŒƒ“ƒgƒQƒCƒ‹
			sd->flee += sd->flee*sd->sc_data[SC_VIOLENTGALE].val3/100;
		}
		if(sd->sc_data[SC_BLIND].timer!=-1){	// ˆÃ•
			sd->hit -= sd->hit >> 2;
			sd->flee -= sd->flee >> 2;
		}
		if (sd->sc_data[SC_WINDWALK].timer != -1) // ƒEƒBƒ“ƒhƒEƒH[ƒN
			sd->flee += sd->flee * (sd->sc_data[SC_WINDWALK].val2) / 100;
		if (sd->sc_data[SC_SPIDERWEB].timer != -1) //ƒXƒpƒCƒ_[ƒEƒFƒu
			sd->flee -= sd->flee * 50 / 100;
		if (sd->sc_data[SC_TRUESIGHT].timer != -1) //âgâDâïü[âTâCâg
			sd->hit += sd->hit * 3 * (sd->sc_data[SC_TRUESIGHT].val1) / 100;
		if (sd->sc_data[SC_CONCENTRATION].timer != -1) //ƒRƒ“ƒZƒ“ƒgƒŒ[ƒVƒ‡ƒ“
			sd->hit += 10 * sd->sc_data[SC_CONCENTRATION].val1;

		// ‘Ï«
		if(sd->sc_data[SC_SIEGFRIED].timer != -1){  // •s€g‚ÌƒW[ƒNƒtƒŠ[ƒh
			sd->subele[1] += sd->sc_data[SC_SIEGFRIED].val2;	// …
			sd->subele[2] += sd->sc_data[SC_SIEGFRIED].val2;	// …
			sd->subele[3] += sd->sc_data[SC_SIEGFRIED].val2;	// ‰Î
			sd->subele[4] += sd->sc_data[SC_SIEGFRIED].val2;	// …
			sd->subele[5] += sd->sc_data[SC_SIEGFRIED].val2;	// …
			sd->subele[6] += sd->sc_data[SC_SIEGFRIED].val2;	// …
			sd->subele[7] += sd->sc_data[SC_SIEGFRIED].val2;	// …
			sd->subele[8] += sd->sc_data[SC_SIEGFRIED].val2;	// …
			sd->subele[9] += sd->sc_data[SC_SIEGFRIED].val2;	// …
		}
		if(sd->sc_data[SC_PROVIDENCE].timer != -1){	// ƒvƒƒ”ƒBƒfƒ“ƒX
			sd->subele[6] += sd->sc_data[SC_PROVIDENCE].val2;	// ‘Î ¹‘®«
			sd->subrace[6] += sd->sc_data[SC_PROVIDENCE].val2;	// ‘Î ˆ«–‚
		}

		// Maximum HP bonus calculations
		if(sd->sc_data[SC_APPLEIDUN].timer != -1)
		{
			sd->status.max_hp += sd->status.max_hp * (sd->sc_data[SC_APPLEIDUN].val1 / 100);
			if(sd->status.max_hp < 0 || sd->status.max_hp > battle_config.max_hp)
				sd->status.max_hp = battle_config.max_hp;
		}
		if(sd->sc_data[SC_DELUGE].timer!=-1 && sd->def_ele==1){	// ƒfƒŠƒ…[ƒW
			sd->status.max_hp += sd->status.max_hp*sd->sc_data[SC_DELUGE].val3/100;
			if(sd->status.max_hp < 0 || sd->status.max_hp > battle_config.max_hp)
				sd->status.max_hp = battle_config.max_hp;
		}
		if(sd->sc_data[SC_SERVICEFORYOU].timer!=-1) {	// ƒT[ƒrƒXƒtƒH[ƒ†[
			sd->status.max_sp += sd->status.max_sp*(10+sd->sc_data[SC_SERVICEFORYOU].val1+sd->sc_data[SC_SERVICEFORYOU].val2
						+sd->sc_data[SC_SERVICEFORYOU].val3)/100;
			if(sd->status.max_sp < 0 || sd->status.max_sp > battle_config.max_sp)
				sd->status.max_sp = battle_config.max_sp;
			sd->dsprate-=(10+sd->sc_data[SC_SERVICEFORYOU].val1*3+sd->sc_data[SC_SERVICEFORYOU].val2
					+sd->sc_data[SC_SERVICEFORYOU].val3);
			if(sd->dsprate<0)sd->dsprate=0;
		}

		if(sd->sc_data[SC_FORTUNE].timer != -1)	// K‰^‚ÌƒLƒX
			sd->critical += (10 + sd->sc_data[SC_FORTUNE].val1 + sd->sc_data[SC_FORTUNE].val2
											+ sd->sc_data[SC_FORTUNE].val3) * 10;

		if(sd->sc_data[SC_EXPLOSIONSPIRITS].timer!=-1){	// ”š—ô”g“®
			if(s_class.job == JOB_SUPER_NOVICE)
				sd->critical += sd->sc_data[SC_EXPLOSIONSPIRITS].val1*100;
			else
			sd->critical += sd->sc_data[SC_EXPLOSIONSPIRITS].val2;
		}

		if (sd->sc_data[SC_STEELBODY].timer != -1) { // ‹à„
			sd->def = 90;
			sd->mdef = 90;
			aspd_rate += 25;
			sd->speed = (sd->speed * 125) / 100;
		}
		if (sd->sc_data[SC_DEFENDER].timer != -1) {
			sd->aspd += (250 - sd->sc_data[SC_DEFENDER].val1 * 50); // Fixed formula by Raveux (from mantis bug reports)
			sd->speed += sd->speed * (55 - 5 * sd->sc_data[SC_DEFENDER].val1) / 100;
		}
		if (sd->sc_data[SC_ENCPOISON].timer != -1)
			sd->addeff[4] += sd->sc_data[SC_ENCPOISON].val2;

		if(sd->sc_data[SC_DANCING].timer != -1){
			int s_rate = 500 - 40 * pc_checkskill(sd, (sd->status.sex? BA_MUSICALLESSON : DC_DANCINGLESSON));
			if (sd->sc_data[SC_LONGING].timer != -1)
				s_rate -= 20 * sd->sc_data[SC_LONGING].val1;
			sd->speed += sd->speed * s_rate / 100;
			sd->nhealsp = 0;
			sd->nshealsp = 0;
			sd->nsshealsp = 0;
		}

		if(sd->sc_data[SC_CURSE].timer != -1)
			sd->speed += 450;

		if(sd->sc_data[SC_TRUESIGHT].timer != -1) //âgâDâïü[âTâCâg
			sd->critical += 10 * (sd->sc_data[SC_TRUESIGHT].val1);

		if(sd->sc_data[SC_BERSERK].timer != -1) {	//All Def/MDef reduced to 0 while in Berserk [DracoRPG]
			sd->def = sd->def2 = 0;
			sd->mdef = sd->mdef2 = 0;
			sd->flee -= sd->flee * 50/100;
			aspd_rate -= 30;
			//sd->base_atk *= 3;
		}
		if (sd->sc_data[SC_KEEPING].timer != -1)
			sd->def = 100;
		if (sd->sc_data[SC_BARRIER].timer != -1)
			sd->mdef = 100;

		if (sd->sc_data[SC_JOINTBEAT].timer != -1) { // Random break [DracoRPG]
			switch(sd->sc_data[SC_JOINTBEAT].val2) {
			case 1: //Ankle break
				sd->speed_rate += 50;
				break;
			case 2: //Wrist break
				sd->aspd_rate += 25;
				break;
			case 3: //Knee break
				sd->speed_rate += 30;
				sd->aspd_rate += 10;
				break;
			case 4: //Shoulder break
				sd->def -= sd->def * 50 / 100;
				sd->def2 -= sd->def2 * 50 / 100;
				break;
			case 5: //Waist break
				sd->def -= sd->def * 25 / 100;
				sd->def2 -= sd->def2 * 25 / 100;
				sd->base_atk -= sd->base_atk * 25 / 100;
				break;
			}
		}

		// custom stats, since there's no info on how much it actually gives ^^; [Celest]
		if (sd->sc_data[SC_GUILDAURA].timer != -1) {
			if (sd->sc_data[SC_GUILDAURA].val4 & 1 << 4) {
				sd->hit += 10;
				sd->flee += 10;
			}
		}
		if(sd->sc_data[SC_INCHIT].timer != -1)
			sd->hit += sd->sc_data[SC_INCHIT].val1;
		if(sd->sc_data[SC_INCFLEE].timer != -1)
			sd->flee += sd->sc_data[SC_INCFLEE].val1;
		if(sd->sc_data[SC_INCMHPRATE].timer != -1) {
			sd->status.max_hp += sd->status.max_hp * sd->sc_data[SC_INCMHPRATE].val1 / 100;
			if(sd->status.max_hp < 0 || sd->status.max_hp > battle_config.max_hp)
				sd->status.max_hp = battle_config.max_hp;
		}
		if(sd->sc_data[SC_INCMSPRATE].timer != -1) {
			sd->status.max_sp += sd->status.max_sp * sd->sc_data[SC_INCMSPRATE].val1 / 100;
			if(sd->status.max_sp < 0 || sd->status.max_sp > battle_config.max_sp)
				sd->status.max_sp = battle_config.max_sp;
		}
		if(sd->sc_data[SC_INCMATKRATE].timer != -1) {
			sd->matk1 = sd->matk1 * (100 + sd->sc_data[SC_INCMATKRATE].val1) /100;
			sd->matk2 = sd->matk2 * (100 + sd->sc_data[SC_INCMATKRATE].val1) /100;
		}
		if(sd->sc_data[SC_INCATKRATE].timer != -1) {
			sd->watk = sd->watk * (100 + sd->sc_data[SC_INCATKRATE].val1) / 100;
			sd->watk2 = sd->watk2 * (100 + sd->sc_data[SC_INCATKRATE].val1) / 100;
		}
		if(sd->sc_data[SC_INCASPDRATE].timer != -1)
			sd->aspd_rate += sd->sc_data[SC_INCASPDRATE].val1;
	}

	if((skill = pc_checkskill(sd,HP_MANARECHARGE)) > 0 ) {
		sd->dsprate -= 4 * skill;
		if(sd->dsprate < 0) sd->dsprate = 0;
	}

	// Matk relative modifiers from equipment
	if(sd->matk_rate != 100) {
		sd->matk1 = sd->matk1 * sd->matk_rate / 100;
		sd->matk2 = sd->matk2 * sd->matk_rate / 100;
	}

	if (sd->speed_rate <= 0)
		sd->speed_rate = 1;
	if (sd->speed_rate != 100)
		sd->speed = sd->speed * sd->speed_rate / 100;
	if (sd->speed < 1) sd->speed = 1;
	if (aspd_rate != 100)
		sd->aspd = sd->aspd*aspd_rate/100;
	if (pc_isriding(sd))							// ‹R•ºC—û
		sd->aspd = sd->aspd*(100 + 10*(5 - pc_checkskill(sd,KN_CAVALIERMASTERY)))/ 100;
	if(sd->aspd < battle_config.max_aspd) sd->aspd = battle_config.max_aspd;
	sd->amotion = sd->aspd;
	sd->dmotion = 800-sd->paramc[1]*4;
	if (sd->dmotion < 400)
		sd->dmotion = 400;
	if (sd->skilltimer != -1 && (skill = pc_checkskill(sd, SA_FREECAST)) > 0) {
		sd->prev_speed = sd->speed;
		sd->speed = sd->speed * (175 - skill * 5) / 100;
	}

	if(sd->status.hp>sd->status.max_hp)
		sd->status.hp=sd->status.max_hp;
	if(sd->status.sp>sd->status.max_sp)
		sd->status.sp=sd->status.max_sp;

	if (first & 4)
		return 0;
	if (first & 3) {
		clif_updatestatus(sd, SP_SPEED);
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
		clif_skillinfoblock(sd);	// ƒXƒLƒ‹‘—M

	if(b_speed != sd->speed)
		clif_updatestatus(sd,SP_SPEED);
	if(b_weight != sd->weight)
		clif_updatestatus(sd,SP_WEIGHT);
	if(b_max_weight != sd->max_weight) {
		clif_updatestatus(sd,SP_MAXWEIGHT);
		pc_checkweighticon(sd);
	}
	for(i=0;i<6;i++)
		if(b_paramb[i] + b_parame[i] != sd->paramb[i] + sd->parame[i])
			clif_updatestatus(sd,SP_STR+i);
	if(b_hit != sd->hit)
		clif_updatestatus(sd,SP_HIT);
	if(b_flee != sd->flee)
		clif_updatestatus(sd,SP_FLEE1);
	if(b_aspd != sd->aspd)
		clif_updatestatus(sd,SP_ASPD);
	if(b_watk != sd->watk || b_base_atk != sd->base_atk)
		clif_updatestatus(sd,SP_ATK1);
	if(b_def != sd->def)
		clif_updatestatus(sd,SP_DEF1);
	if(b_watk2 != sd->watk2)
		clif_updatestatus(sd,SP_ATK2);
	if(b_def2 != sd->def2)
		clif_updatestatus(sd,SP_DEF2);
	if(b_flee2 != sd->flee2)
		clif_updatestatus(sd,SP_FLEE2);
	if(b_critical != sd->critical)
		clif_updatestatus(sd,SP_CRITICAL);
	if(b_matk1 != sd->matk1)
		clif_updatestatus(sd,SP_MATK1);
	if(b_matk2 != sd->matk2)
		clif_updatestatus(sd,SP_MATK2);
	if(b_mdef != sd->mdef)
		clif_updatestatus(sd,SP_MDEF1);
	if(b_mdef2 != sd->mdef2)
		clif_updatestatus(sd,SP_MDEF2);
	if(b_attackrange != sd->attackrange)
		clif_updatestatus(sd,SP_ATTACKRANGE);
	if(b_max_hp != sd->status.max_hp)
		clif_updatestatus(sd,SP_MAXHP);
	if(b_max_sp != sd->status.max_sp)
		clif_updatestatus(sd,SP_MAXSP);
	if(b_hp != sd->status.hp)
		clif_updatestatus(sd,SP_HP);
	if(b_sp != sd->status.sp)
		clif_updatestatus(sd,SP_SP);

	//if(sd->status.hp<sd->status.max_hp>>2 && pc_checkskill(sd,SM_AUTOBERSERK)>0 &&
	if(sd->status.hp<sd->status.max_hp>>2 && sd->sc_data[SC_AUTOBERSERK].timer != -1 &&
		(sd->sc_data[SC_PROVOKE].timer == -1 || sd->sc_data[SC_PROVOKE].val2 == 0 ) && !pc_isdead(sd))
		// ƒI[ƒgƒo[ƒT[ƒN”­“®
		status_change_start(&sd->bl,SC_PROVOKE,10,1,0,0,0,0);

	return 0;
}

/*==========================================
 * For quick calculating [Celest]
 *------------------------------------------
 */
void status_calc_speed(struct map_session_data *sd) {
	int b_speed, skill;
	unsigned int s_class;

	nullpo_retv(sd);

	s_class = pc_calc_base_job2(sd->status.class);

	b_speed = sd->speed;
	sd->speed = DEFAULT_WALK_SPEED;

	if (sd->sc_count) {
		if(sd->sc_data[SC_INCREASEAGI].timer != -1 && sd->sc_data[SC_QUAGMIRE].timer == -1 && sd->sc_data[SC_DONTFORGETME].timer == -1)	// ‘¬“x?‰Á
			sd->speed -= sd->speed >> 2;
		if(sd->sc_data[SC_DECREASEAGI].timer != -1)
			sd->speed = sd->speed * 125 / 100;
		if(sd->sc_data[SC_CLOAKING].timer != -1)
			sd->speed = sd->speed * (sd->sc_data[SC_CLOAKING].val3-sd->sc_data[SC_CLOAKING].val1 * 3) / 100;
		if(sd->sc_data[SC_CHASEWALK].timer != -1)
			sd->speed = sd->speed * sd->sc_data[SC_CHASEWALK].val3 / 100;
		if(sd->sc_data[SC_QUAGMIRE].timer != -1)
			sd->speed = sd->speed * 3 / 2;
		if (sd->sc_data[SC_WINDWALK].timer != -1 && sd->sc_data[SC_INCREASEAGI].timer == -1)
			sd->speed -= sd->speed *(sd->sc_data[SC_WINDWALK].val1*2)/100;
		if(sd->sc_data[SC_CARTBOOST].timer != -1)
			sd->speed -= (DEFAULT_WALK_SPEED * 20) / 100;
		if(sd->sc_data[SC_BERSERK].timer != -1)
			sd->speed -= sd->speed >> 2;
		if(sd->sc_data[SC_WEDDING].timer != -1)
			sd->speed = 2 * DEFAULT_WALK_SPEED;
		if(sd->sc_data[SC_DONTFORGETME].timer != -1)
			sd->speed = sd->speed * (100 + sd->sc_data[SC_DONTFORGETME].val1 * 2 +
									sd->sc_data[SC_DONTFORGETME].val2 + (sd->sc_data[SC_DONTFORGETME].val3&0xffff)) / 100;
		if(sd->sc_data[SC_STEELBODY].timer != -1)
			sd->speed = (sd->speed * 125) / 100;
		if(sd->sc_data[SC_DEFENDER].timer != -1)
			sd->speed += sd->speed * (55 - 5 * sd->sc_data[SC_DEFENDER].val1) / 100;
		if(sd->sc_data[SC_DANCING].timer != -1) {
			int s_rate = 500 - 40 * pc_checkskill(sd, (sd->status.sex? BA_MUSICALLESSON: DC_DANCINGLESSON));
			if (sd->sc_data[SC_LONGING].timer != -1)
				s_rate -= 20 * sd->sc_data[SC_LONGING].val1;
			sd->speed += sd->speed * s_rate / 100;
		}
		if (sd->sc_data[SC_CURSE].timer != -1)
			sd->speed += 450;
		if (sd->sc_data[SC_SLOWDOWN].timer != -1)
			sd->speed = sd->speed * 150 / 100;
		if (sd->sc_data[SC_SPEEDUP0].timer != -1)
			sd->speed -= sd->speed >> 2;
	}

	if(sd->status.option&2 && (skill = pc_checkskill(sd, RG_TUNNELDRIVE)) > 0 )
		sd->speed += sd->speed * (100 - 16 * skill) / 100;
	if (pc_iscarton(sd) && (skill = pc_checkskill(sd, MC_PUSHCART)) > 0)
		sd->speed += sd->speed * (100 - 10 * skill) / 100;
	else if (pc_isriding(sd) && pc_checkskill(sd, KN_RIDING) > 0)
		sd->speed -= sd->speed >> 2;
	else
	if(s_class == JOB_ASSASSIN && (skill = pc_checkskill(sd, TF_MISS)) > 0)
			sd->speed -= sd->speed * skill / 100;

	if(sd->speed_rate != 100)
		sd->speed = sd->speed * sd->speed_rate / 100;
	if(sd->speed < 1) sd->speed = 1;

	if (sd->skilltimer != -1 && (skill = pc_checkskill(sd, SA_FREECAST)) > 0) {
		sd->prev_speed = sd->speed;
		sd->speed = sd->speed * (175 - skill * 5) / 100;
	}

	if (b_speed != sd->speed)
		clif_updatestatus(sd, SP_SPEED);

	return;
}

/*==========================================
 * ‘ÎÛ‚ÌClass‚ğ•Ô‚·(”Ä—p)
 * –ß‚è‚Í®”‚Å0ˆÈã
 *------------------------------------------
 */
int status_get_class(struct block_list *bl) {
	nullpo_retr(0, bl);

	if (bl->type == BL_MOB)
		return ((struct mob_data *)bl)->class;
	else if (bl->type == BL_PC)
		return ((struct map_session_data *)bl)->status.class;
	else if (bl->type == BL_PET)
		return ((struct pet_data *)bl)->class;
	
	return 0;
}

/*==========================================
 * ‘ÎÛ‚Ì•ûŒü‚ğ•Ô‚·(”Ä—p)
 * –ß‚è‚Í®”‚Å0ˆÈã
 *------------------------------------------
 */
int status_get_dir(struct block_list *bl) {
	nullpo_retr(0, bl);

	if (bl->type == BL_MOB)
		return ((struct mob_data *)bl)->dir;
	else if (bl->type == BL_PC)
		return ((struct map_session_data *)bl)->dir;
	else if (bl->type == BL_PET)
		return ((struct pet_data *)bl)->dir;

	return 0;
}

/*==========================================
 * ‘ÎÛ‚ÌƒŒƒxƒ‹‚ğ•Ô‚·(”Ä—p)
 * –ß‚è‚Í®”‚Å0ˆÈã
 *------------------------------------------
 */
int status_get_lv(struct block_list *bl) {
	nullpo_retr(0, bl);

	if (bl->type == BL_MOB)
		return ((struct mob_data *)bl)->level;
	else if (bl->type == BL_PC)
		return ((struct map_session_data *)bl)->status.base_level;
	else if (bl->type == BL_PET)
		return ((struct pet_data *)bl)->msd->pet.level;

	return 0;
}

/*==========================================
 * ‘ÎÛ‚ÌË’ö‚ğ•Ô‚·(”Ä—p)
 * –ß‚è‚Í®”‚Å0ˆÈã
 *------------------------------------------
 */
int status_get_range(struct block_list *bl) {
	nullpo_retr(0, bl);

	if (bl->type == BL_MOB)
		return mob_db[((struct mob_data *)bl)->class].range;
	else if (bl->type == BL_PC)
		return ((struct map_session_data *)bl)->attackrange;
	else if (bl->type == BL_PET)
		return mob_db[((struct pet_data *)bl)->class].range;

	return 0;
}

/*==========================================
 * ‘ÎÛ‚ÌHP‚ğ•Ô‚·(”Ä—p)
 * –ß‚è‚Í®”‚Å0ˆÈã
 *------------------------------------------
 */
int status_get_hp(struct block_list *bl) {
	nullpo_retr(1, bl);

	if (bl->type == BL_MOB)
		return ((struct mob_data *)bl)->hp;
	else if (bl->type == BL_PC)
		return ((struct map_session_data *)bl)->status.hp;

	return 1;
}

/*==========================================
 * ‘ÎÛ‚ÌMHP‚ğ•Ô‚·(”Ä—p)
 * –ß‚è‚Í®”‚Å0ˆÈã
 *------------------------------------------
 */
int status_get_max_hp(struct block_list *bl) {
	nullpo_retr(1, bl);

	if (bl->type == BL_PC)
		return ((struct map_session_data *)bl)->status.max_hp;
	else {
		struct status_change *sc_data;
		int max_hp = 1;

		if (bl->type == BL_MOB) {
			struct mob_data *md;
			nullpo_retr(1, md = (struct mob_data *)bl);
			max_hp = mob_db[md->class].max_hp;
			if (battle_config.mobs_level_up) // mobs leveling up increase [Valaris]
				max_hp += (md->level - mob_db[md->class].lv) * status_get_vit(bl);
			if (mob_db[md->class].mexp > 0) {
				if (battle_config.mvp_hp_rate != 100)
					max_hp = max_hp * battle_config.mvp_hp_rate / 100;
			} else {
				if (battle_config.monster_hp_rate != 100)
					max_hp = max_hp * battle_config.monster_hp_rate / 100;
			}

		} else if (bl->type == BL_PET) {
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
 * ‘ÎÛ‚ÌStr‚ğ•Ô‚·(”Ä—p)
 * –ß‚è‚Í®”‚Å0ˆÈã
 *------------------------------------------
 */
int status_get_str(struct block_list *bl) {
	int str = 0;

	nullpo_retr(0, bl);

	if (bl->type == BL_PC)
		return ((struct map_session_data *)bl)->paramc[0];
	else {
		struct status_change *sc_data;
		sc_data = status_get_sc_data(bl);

		if (bl->type == BL_MOB) {
			str = mob_db[((struct mob_data *)bl)->class].str;
			if (battle_config.mobs_level_up) // mobs leveling up increase [Valaris]
				str += ((struct mob_data *)bl)->level - mob_db[((struct mob_data *)bl)->class].lv;
		}
		else if (bl->type == BL_PET)
			str = mob_db[((struct pet_data *)bl)->class].str;

		if (sc_data) {
			if (sc_data[SC_LOUD].timer!=-1 && sc_data[SC_QUAGMIRE].timer == -1)
				str += 4;
			if (sc_data[SC_BLESSING].timer != -1) { // ƒuƒŒƒbƒVƒ“ƒO
				int race=status_get_race(bl);
				if (battle_check_undead(race, status_get_elem_type(bl)) || race == 6)
					str >>= 1; // ˆ« –‚/•s€
				else
					str += sc_data[SC_BLESSING].val1; // ‚»‚Ì‘¼
			}
			if(sc_data[SC_TRUESIGHT].timer != -1) // ƒgƒDƒ‹[ƒTƒCƒg
				str += 5;
			if (sc_data[SC_INCSTR].timer != -1)
				str += sc_data[SC_INCSTR].val1;
			/*if (sc_data[SC_INCALLSTATUS].timer != -1)
				str += sc_data[SC_INCALLSTATUS].val1;*/
			if (sc_data[SC_STRFOOD].timer != -1)
				str += sc_data[SC_STRFOOD].val1;
		}
		if (str < 0)
			str = 0;
	}

	return str;
}

/*==========================================
 * ‘ÎÛ‚ÌAgi‚ğ•Ô‚·(”Ä—p)
 * –ß‚è‚Í®”‚Å0ˆÈã
 *------------------------------------------
 */

int status_get_agi(struct block_list *bl) {
	int agi = 0;

	nullpo_retr(0, bl);

	if (bl->type == BL_PC)
		agi = ((struct map_session_data *)bl)->paramc[1];
	else {
		struct status_change *sc_data;
		sc_data = status_get_sc_data(bl);

		if (bl->type == BL_MOB) {
			agi = mob_db[((struct mob_data *)bl)->class].agi;
			if (battle_config.mobs_level_up) // increase of mobs leveling up [Valaris]
				agi += ((struct mob_data *)bl)->level - mob_db[((struct mob_data *)bl)->class].lv;
		}
		else if (bl->type == BL_PET)
			agi = mob_db[((struct pet_data *)bl)->class].agi;

		if (sc_data) {
			if (sc_data[SC_QUAGMIRE].timer != -1) // ƒNƒ@ƒOƒ}ƒCƒA
				agi -= sc_data[SC_QUAGMIRE].val1 * 10;
			else {
				if (sc_data[SC_INCREASEAGI].timer != -1 && sc_data[SC_DONTFORGETME].timer == -1) // ‘¬“x‘‰Á(PC‚Ípc.c‚Å)
					agi += 2 + sc_data[SC_INCREASEAGI].val1;
				if (sc_data[SC_CONCENTRATE].timer != -1)
					agi += agi * (2 + sc_data[SC_CONCENTRATE].val1) / 100;
			}
			if (sc_data[SC_DECREASEAGI].timer != -1) // ‘¬“xŒ¸­
				agi -= 2 + sc_data[SC_DECREASEAGI].val1;
			if (sc_data[SC_TRUESIGHT].timer != -1) // ƒgƒDƒ‹[ƒTƒCƒg
				agi += 5;
			if (sc_data[SC_INCAGI].timer != -1)
				agi += sc_data[SC_INCAGI].val1;
			if (sc_data[SC_INCALLSTATUS].timer != -1)
				agi += sc_data[SC_INCALLSTATUS].val1;
			if (sc_data[SC_AGIFOOD].timer != -1)
				agi += sc_data[SC_AGIFOOD].val1;
		}
		
		if (agi < 0)
			agi = 0;
	}

	return agi;
}

/*==========================================
 * ‘ÎÛ‚ÌVit‚ğ•Ô‚·(”Ä—p)
 * –ß‚è‚Í®”‚Å0ˆÈã
 *------------------------------------------
 */
int status_get_vit(struct block_list *bl) {
	int vit = 0;

	nullpo_retr(0, bl);

	if (bl->type == BL_PC)
		return ((struct map_session_data *)bl)->paramc[2];
	else {
		struct status_change *sc_data;
		sc_data = status_get_sc_data(bl);
		if (bl->type == BL_MOB) {
			vit = mob_db[((struct mob_data *)bl)->class].vit;
			if (battle_config.mobs_level_up) // increase from mobs leveling up [Valaris]
				vit += ((struct mob_data *)bl)->level - mob_db[((struct mob_data *)bl)->class].lv;
		}
		else if (bl->type == BL_PET)
			vit = mob_db[((struct pet_data *)bl)->class].vit;
		if (sc_data) {
			if (sc_data[SC_STRIPARMOR].timer != -1)
				vit = vit * 60 / 100;
			if (sc_data[SC_TRUESIGHT].timer != -1) // ƒgƒDƒ‹[ƒTƒCƒg
				vit += 5;
			/*if(sc_data[SC_INCALLSTATUS].timer != -1)
				vit += sc_data[SC_INCALLSTATUS].val1;*/
			if(sc_data[SC_VITFOOD].timer != -1)
				vit += sc_data[SC_VITFOOD].val1;
		}

		if (vit < 0)
			vit = 0;
	}

	return vit;
}

/*==========================================
 * ‘ÎÛ‚ÌInt‚ğ•Ô‚·(”Ä—p)
 * –ß‚è‚Í®”‚Å0ˆÈã
 *------------------------------------------
 */
int status_get_int(struct block_list *bl) {
	int int_ = 0;

	nullpo_retr(0, bl);

	if (bl->type == BL_PC)
		return ((struct map_session_data *)bl)->paramc[3];
	else {
		struct status_change *sc_data;
		sc_data = status_get_sc_data(bl);
		if (bl->type == BL_MOB) {
			int_ = mob_db[((struct mob_data *)bl)->class].int_;
			if (battle_config.mobs_level_up) // increase from mobs leveling up [Valaris]
				int_ += ((struct mob_data *)bl)->level - mob_db[((struct mob_data *)bl)->class].lv;
		}
		else if (bl->type == BL_PET)
			int_ = mob_db[((struct pet_data *)bl)->class].int_;

		if (sc_data) {
			if (sc_data[SC_BLESSING].timer != -1) { // ƒuƒŒƒbƒVƒ“ƒO
				int race = status_get_race(bl);
				if (battle_check_undead(race,status_get_elem_type(bl)) || race == 6)
					int_ >>= 1; // ˆ« –‚/•s€
				else
					int_ += sc_data[SC_BLESSING].val1; // ‚»‚Ì‘¼
			}
			if (sc_data[SC_STRIPHELM].timer != -1)
				int_ = int_ * 60 / 100;
			if (sc_data[SC_TRUESIGHT].timer != -1) // ƒgƒDƒ‹[ƒTƒCƒg
				int_ += 5;
			/*if(sc_data[SC_INCALLSTATUS].timer != -1)
				int_ += sc_data[SC_INCALLSTATUS].val1;*/
			if (sc_data[SC_INTFOOD].timer != -1)
				int_ += sc_data[SC_INTFOOD].val1;
		}
		if (int_ < 0)
			int_ = 0;
	}

	return int_;
}

/*==========================================
 * ‘ÎÛ‚ÌDex‚ğ•Ô‚·(”Ä—p)
 * –ß‚è‚Í®”‚Å0ˆÈã
 *------------------------------------------
 */
int status_get_dex(struct block_list *bl) {
	int dex = 0;

	nullpo_retr(0, bl);

	if (bl->type == BL_PC)
		return ((struct map_session_data *)bl)->paramc[4];
	else {
		struct status_change *sc_data;
		sc_data = status_get_sc_data(bl);
		if (bl->type == BL_MOB) {
			dex = mob_db[((struct mob_data *)bl)->class].dex;
			if (battle_config.mobs_level_up) // increase from mobs leveling up [Valaris]
				dex += ((struct mob_data *)bl)->level - mob_db[((struct mob_data *)bl)->class].lv;
		}
		else if (bl->type == BL_PET)
			dex = mob_db[((struct pet_data *)bl)->class].dex;

		if (sc_data) {
			if (sc_data[SC_CONCENTRATE].timer != -1 && sc_data[SC_QUAGMIRE].timer == -1)
				dex += dex * (2 + sc_data[SC_CONCENTRATE].val1) / 100;
			if (sc_data[SC_BLESSING].timer != -1) { // ƒuƒŒƒbƒVƒ“ƒO
				int race = status_get_race(bl);
				if (battle_check_undead(race, status_get_elem_type(bl)) || race == 6)
					dex >>= 1;	// ˆ« –‚/•s€
				else
					dex += sc_data[SC_BLESSING].val1; // ‚»‚Ì‘¼
			}
			if (sc_data[SC_QUAGMIRE].timer != -1) // ƒNƒ@ƒOƒ}ƒCƒA
				dex -= sc_data[SC_QUAGMIRE].val1 * 10;
			if (sc_data[SC_TRUESIGHT].timer != -1) // ƒgƒDƒ‹[ƒTƒCƒg
				dex += 5;
			if (sc_data[SC_INCDEX].timer != -1) // ƒgƒDƒ‹[ƒTƒCƒg
				dex += sc_data[SC_INCDEX].val1;
			/*if (sc_data[SC_INCALLSTATUS].timer != -1)
				dex += sc_data[SC_INCALLSTATUS].val1;*/
			if (sc_data[SC_DEXFOOD].timer != -1)
				dex += sc_data[SC_DEXFOOD].val1;
		}
		if (dex < 0)
			dex = 0;
	}

	return dex;
}

/*==========================================
 * ‘ÎÛ‚ÌLuk‚ğ•Ô‚·(”Ä—p)
 * –ß‚è‚Í®”‚Å0ˆÈã
 *------------------------------------------
 */
int status_get_luk(struct block_list *bl) {
	int luk = 0;

	nullpo_retr(0, bl);

	if (bl->type == BL_PC)
		return ((struct map_session_data *)bl)->paramc[5];
	else {
		struct status_change *sc_data;
		sc_data = status_get_sc_data(bl);
		if (bl->type == BL_MOB) {
			luk = mob_db[((struct mob_data *)bl)->class].luk;
			if (battle_config.mobs_level_up) // increase from mobs leveling up [Valaris]
				luk += ((struct mob_data *)bl)->level - mob_db[((struct mob_data *)bl)->class].lv;
		}
		else if (bl->type == BL_PET)
			luk = mob_db[((struct pet_data *)bl)->class].luk;

		if (sc_data) {
			if (sc_data[SC_GLORIA].timer != -1) // ƒOƒƒŠƒA(PC‚Ípc.c‚Å)
				luk += 30;
			if (sc_data[SC_TRUESIGHT].timer != -1) // ƒgƒDƒ‹[ƒTƒCƒg
				luk += 5;
			if (sc_data[SC_CURSE].timer != -1 ) // ô‚¢
				luk = 0;
			/*if (sc_data[SC_INCALLSTATUS].timer != -1)
				luk += sc_data[SC_INCALLSTATUS].val1;*/
			if (sc_data[SC_LUKFOOD].timer != -1)
				luk += sc_data[SC_LUKFOOD].val1;
		}
		if (luk < 0)
			luk = 0;
	}

	return luk;
}

/*==========================================
 * ‘ÎÛ‚ÌFlee‚ğ•Ô‚·(”Ä—p)
 * –ß‚è‚Í®”‚Å1ˆÈã
 *------------------------------------------
 */
int status_get_flee(struct block_list *bl) {
	int flee = 1;

	nullpo_retr(1, bl);

	if (bl->type == BL_PC)
		flee = ((struct map_session_data *)bl)->flee;
	else {
		struct status_change *sc_data;
		sc_data = status_get_sc_data(bl);
		flee = status_get_agi(bl) + status_get_lv(bl);

		if (sc_data) {
			if (sc_data[SC_WHISTLE].timer != -1)
				flee += flee * (sc_data[SC_WHISTLE].val1 + sc_data[SC_WHISTLE].val2
				       + (sc_data[SC_WHISTLE].val3 >> 16)) / 100;
			if (sc_data[SC_BLIND].timer != -1)
				flee -= flee * 25 / 100;
			if (sc_data[SC_WINDWALK].timer != -1) // ƒEƒBƒ“ƒhƒEƒH[ƒN
				flee += flee * (sc_data[SC_WINDWALK].val2) / 100;
			if (sc_data[SC_SPIDERWEB].timer != -1) //ƒXƒpƒCƒ_[ƒEƒFƒu
				flee -= flee * 50 / 100;
			if(sc_data[SC_INCFLEE].timer != -1)
				flee += sc_data[SC_INCFLEE].val1;
			if(sc_data[SC_INCFLEERATE].timer != -1)
				flee += flee * sc_data[SC_INCFLEERATE].val1 / 100;
			if(sc_data[SC_CLOSECONFINE].timer != -1)
				flee += 10;
			if(sc_data[SC_FLEEFOOD].timer != -1)
				flee += sc_data[SC_FLEEFOOD].val1;
		}
	}

	if (flee < 1)
		flee = 1;

	return flee;
}

/*==========================================
 * ‘ÎÛ‚ÌHit‚ğ•Ô‚·(”Ä—p)
 * –ß‚è‚Í®”‚Å1ˆÈã
 *------------------------------------------
 */
int status_get_hit(struct block_list *bl) {
	int hit = 1;

	nullpo_retr(1, bl);

	if (bl->type == BL_PC)
		hit = ((struct map_session_data *)bl)->hit;
	else {
		struct status_change *sc_data;
		sc_data = status_get_sc_data(bl);
		hit = status_get_dex(bl) + status_get_lv(bl);

		if (sc_data) {
			if (sc_data[SC_HUMMING].timer != -1) //
				hit += hit * (sc_data[SC_HUMMING].val1 * 2 + sc_data[SC_HUMMING].val2
				      + sc_data[SC_HUMMING].val3) / 100;
			if (sc_data[SC_BLIND].timer != -1) // ô‚¢
				hit -= hit * 25 / 100;
			if (sc_data[SC_TRUESIGHT].timer != -1) // âgâDâïü[âTâCâg
				hit += hit * 3 * (sc_data[SC_TRUESIGHT].val1) / 100;
			if (sc_data[SC_CONCENTRATION].timer != -1) //ƒRƒ“ƒZƒ“ƒgƒŒ[ƒVƒ‡ƒ“
				hit += 10 * sc_data[SC_CONCENTRATION].val1;
			if(sc_data[SC_INCHITRATE].timer != -1)
				hit += hit * sc_data[SC_INCHITRATE].val1 / 100;
			if(sc_data[SC_HITFOOD].timer != -1)
				hit += sc_data[SC_HITFOOD].val1;
		}
	}
	if (hit < 1)
		hit = 1;

	return hit;
}

/*==========================================
 * ‘ÎÛ‚ÌŠ®‘S‰ñ”ğ‚ğ•Ô‚·(”Ä—p)
 * –ß‚è‚Í®”‚Å1ˆÈã
 *------------------------------------------
 */
int status_get_flee2(struct block_list *bl) {
	int flee2 = 1;

	nullpo_retr(1, bl);

	if (bl->type == BL_PC) {
//		flee2 = status_get_luk(bl) + 10;
//		flee2 += ((struct map_session_data *)bl)->flee2 - (((struct map_session_data *)bl)->paramc[5] + 10);
		return ((struct map_session_data *)bl)->flee2;
	} else {
		struct status_change *sc_data;
		sc_data = status_get_sc_data(bl);
		flee2 = status_get_luk(bl) + 1;

		if (sc_data) {
			if(sc_data[SC_WHISTLE].timer != -1)
				flee2 += (sc_data[SC_WHISTLE].val1 + sc_data[SC_WHISTLE].val2
				       + (sc_data[SC_WHISTLE].val3 & 0xffff)) * 10;
		}
	}
	if (flee2 < 1)
		flee2 = 1;

	return flee2;
}

/*==========================================
 * ‘ÎÛ‚ÌƒNƒŠƒeƒBƒJƒ‹‚ğ•Ô‚·(”Ä—p)
 * –ß‚è‚Í®”‚Å1ˆÈã
 *------------------------------------------
 */
int status_get_critical(struct block_list *bl) {
	int critical = 1;

	nullpo_retr(1, bl);

	if(bl->type==BL_PC){
//		critical = status_get_luk(bl)*3 + 10;
//		critical += ((struct map_session_data *)bl)->critical - ((((struct map_session_data *)bl)->paramc[5]*3) + 10);
		return ((struct map_session_data *)bl)->critical;
	} else {
		struct status_change *sc_data;
		sc_data = status_get_sc_data(bl);
		critical = status_get_luk(bl) * 3 + 1;

		if (sc_data) {
			if (sc_data[SC_FORTUNE].timer != -1)
				critical += (10+sc_data[SC_FORTUNE].val1 + sc_data[SC_FORTUNE].val2
				          + sc_data[SC_FORTUNE].val3) * 10;
			if (sc_data[SC_EXPLOSIONSPIRITS].timer != -1)
				critical += sc_data[SC_EXPLOSIONSPIRITS].val2;
			if (sc_data[SC_TRUESIGHT].timer != -1) //ƒgƒDƒ‹[ƒTƒCƒg
				critical += critical * sc_data[SC_TRUESIGHT].val1 / 100;
		}
	}
	if (critical < 1)
		critical = 1;

	return critical;
}

/*==========================================
 * base_atk‚Ìæ“¾
 * –ß‚è‚Í®”‚Å1ˆÈã
 *------------------------------------------
 */
int status_get_baseatk(struct block_list *bl) {
	int batk = 1;

	nullpo_retr(1, bl);

	if (bl->type == BL_PC) {
		batk = ((struct map_session_data *)bl)->base_atk; //İ’è‚³‚ê‚Ä‚¢‚ébase_atk
		if (((struct map_session_data *)bl)->status.weapon < 16)
			batk += ((struct map_session_data *)bl)->weapon_atk[((struct map_session_data *)bl)->status.weapon];
	} else { //‚»‚êˆÈŠO‚È‚ç
		int str, dstr;
		struct status_change *sc_data;
		sc_data = status_get_sc_data(bl);
		str = status_get_str(bl); //STR
		dstr = str / 10;
		batk = dstr * dstr + str; //base_atk‚ğŒvZ‚·‚é
		if (sc_data) { //ó‘ÔˆÙí‚ ‚è
			if (sc_data[SC_PROVOKE].timer != -1) //PC‚Åƒvƒƒ{ƒbƒN(SM_PROVOKE)ó‘Ô
				batk = batk * (100 + (3 * sc_data[SC_PROVOKE].val1 + 2)) / 100; //base_atk‘‰Á  Formula correction. [Bison]
			if (sc_data[SC_CURSE].timer != -1) //ô‚í‚ê‚Ä‚¢‚½‚ç
				batk -= batk * 25 / 100; //base_atk‚ª25%Œ¸­
			if (sc_data[SC_CONCENTRATION].timer != -1) //ƒRƒ“ƒZƒ“ƒgƒŒ[ƒVƒ‡ƒ“
				batk += batk * (5 * sc_data[SC_CONCENTRATION].val1) / 100;
			if(sc_data[SC_BATKFOOD].timer != -1)
				batk += sc_data[SC_BATKFOOD].val1;
		}
	}
	if (batk < 1) //base_atk‚ÍÅ’á‚Å‚à1
		batk = 1;

	return batk;
}

/*==========================================
 * ‘ÎÛ‚ÌAtk‚ğ•Ô‚·(”Ä—p)
 * –ß‚è‚Í®”‚Å0ˆÈã
 *------------------------------------------
 */
int status_get_atk(struct block_list *bl) {
	int atk = 0;

	nullpo_retr(0, bl);

	if (bl->type == BL_PC)
		return ((struct map_session_data*)bl)->watk;
	else {
		struct status_change *sc_data;
		sc_data = status_get_sc_data(bl);

		if (bl->type == BL_MOB)
			atk = mob_db[((struct mob_data*)bl)->class].atk1;
		else if (bl->type == BL_PET)
			atk = mob_db[((struct pet_data*)bl)->class].atk1;

		if (sc_data) {
			if (sc_data[SC_PROVOKE].timer != -1)
				atk = atk * (100 + (3 * sc_data[SC_PROVOKE].val1 + 2)) / 100;	// Formula correction. [Bison]
			if (sc_data[SC_CURSE].timer != -1)
				atk -= atk * 25 / 100;
			if (sc_data[SC_CONCENTRATION].timer != -1) //ƒRƒ“ƒZƒ“ƒgƒŒ[ƒVƒ‡ƒ“
				atk += atk * (5 * sc_data[SC_CONCENTRATION].val1) / 100;
			if(sc_data[SC_INCATKRATE].timer != -1)
				atk += atk * sc_data[SC_INCATKRATE].val1 / 100;
			/*if(sc_data[SC_WATKFOOD].timer != -1)
				atk += atk * sc_data[SC_WATKFOOD].val1 / 100;*/
		}
	}
	if (atk < 0)
		atk = 0;

	return atk;
}

/*==========================================
 * ‘ÎÛ‚Ì¶èAtk‚ğ•Ô‚·(”Ä—p)
 * –ß‚è‚Í®”‚Å0ˆÈã
 *------------------------------------------
 */
int status_get_atk_(struct block_list *bl) {
	nullpo_retr(0, bl);

	if (bl->type == BL_PC)
		return ((struct map_session_data*)bl)->watk_;

	return 0;
}

/*==========================================
 * ‘ÎÛ‚ÌAtk2‚ğ•Ô‚·(”Ä—p)
 * –ß‚è‚Í®”‚Å0ˆÈã
 *------------------------------------------
 */
int status_get_atk2(struct block_list *bl) {
	nullpo_retr(0, bl);

	if (bl->type == BL_PC)
		return ((struct map_session_data*)bl)->watk2;
	else {
		struct status_change *sc_data = status_get_sc_data(bl);
		int atk2 = 0;
		if (bl->type == BL_MOB)
			atk2 = mob_db[((struct mob_data*)bl)->class].atk2;
		else if (bl->type == BL_PET)
			atk2 = mob_db[((struct pet_data*)bl)->class].atk2;
		if (sc_data) {
			if (sc_data[SC_IMPOSITIO].timer != -1)
				atk2 += sc_data[SC_IMPOSITIO].val1 * 5;
			if (sc_data[SC_PROVOKE].timer != -1)
				atk2 = atk2 * (100 + (3 * sc_data[SC_PROVOKE].val1 + 2)) / 100;	// Formula correction. [Bison]
			if (sc_data[SC_CURSE].timer!=-1 )
				atk2 -= atk2 * 25 / 100;
			if (sc_data[SC_DRUMBATTLE].timer != -1)
				atk2 += sc_data[SC_DRUMBATTLE].val2;
			if (sc_data[SC_NIBELUNGEN].timer != -1 && (status_get_element(bl) / 10) >= 8)
				atk2 += sc_data[SC_NIBELUNGEN].val3;
			if (sc_data[SC_STRIPWEAPON].timer != -1)
				atk2 = atk2 * sc_data[SC_STRIPWEAPON].val2 / 100;
			if (sc_data[SC_CONCENTRATION].timer != -1) //ƒRƒ“ƒZƒ“ƒgƒŒ[ƒVƒ‡ƒ“
				atk2 += atk2 * (5 * sc_data[SC_CONCENTRATION].val1) / 100;
			if(sc_data[SC_INCATKRATE].timer!=-1)
				atk2 += atk2 * sc_data[SC_INCATKRATE].val1 / 100;
			/*if(sc_data[SC_WATKFOOD].timer != -1)
				atk2 += atk2 * sc_data[SC_WATKFOOD].val1 / 100;*/
		}
		if (atk2 < 0)
			atk2 = 0;
		return atk2;
	}

	return 0;
}

/*==========================================
 * ‘ÎÛ‚Ì¶èAtk2‚ğ•Ô‚·(”Ä—p)
 * –ß‚è‚Í®”‚Å0ˆÈã
 *------------------------------------------
 */
int status_get_atk_2(struct block_list *bl) {
	nullpo_retr(0, bl);

	if (bl->type == BL_PC)
		return ((struct map_session_data*)bl)->watk_2;

	return 0;
}

/*==========================================
 * ‘ÎÛ‚ÌMAtk1‚ğ•Ô‚·(”Ä—p)
 * –ß‚è‚Í®”‚Å0ˆÈã
 *------------------------------------------
 */
int status_get_matk1(struct block_list *bl) {
	int matk = 0;

	nullpo_retr(0, bl);

	if (bl->type == BL_PC)
		return ((struct map_session_data *)bl)->matk1;
	else {
		struct status_change *sc_data;
		int int_ = status_get_int(bl);
		sc_data = status_get_sc_data(bl);
		matk = int_ + (int_ / 5) * (int_ / 5);

		if (sc_data) {
			if (sc_data[SC_MINDBREAKER].timer!=-1)
				matk = matk * (100 + 2 * sc_data[SC_MINDBREAKER].val1) / 100;
			if (sc_data[SC_INCMATKRATE].timer!=-1)
				matk = matk * (100 + sc_data[SC_INCMATKRATE].val1) /100;
			if (sc_data[SC_MATKFOOD].timer!=-1)
				matk = matk * (100 + sc_data[SC_MATKFOOD].val1) /100;
		}
	}

	return matk;
}

/*==========================================
 * ‘ÎÛ‚ÌMAtk2‚ğ•Ô‚·(”Ä—p)
 * –ß‚è‚Í®”‚Å0ˆÈã
 *------------------------------------------
 */
int status_get_matk2(struct block_list *bl) {
	int matk = 0;

	nullpo_retr(0, bl);

	if (bl->type == BL_PC)
		return ((struct map_session_data *)bl)->matk2;
	else {
		struct status_change *sc_data;
		int int_ = status_get_int(bl);
		sc_data = status_get_sc_data(bl);
		matk = int_ + (int_ / 7) * (int_ / 7);

		if (sc_data) {
			if (sc_data[SC_MINDBREAKER].timer != -1)
				matk = matk * (100 + 2 * sc_data[SC_MINDBREAKER].val1) / 100;
		}
	}

	return matk;
}

/*==========================================
 * ‘ÎÛ‚ÌDef‚ğ•Ô‚·(”Ä—p)
 * –ß‚è‚Í®”‚Å0ˆÈã
 *------------------------------------------
 */
int status_get_def(struct block_list *bl) {
	struct status_change *sc_data;
	int def = 0,skilltimer = -1, skillid = 0;

	nullpo_retr(0, bl);

	sc_data = status_get_sc_data(bl);
	if (bl->type == BL_PC){
		def = ((struct map_session_data *)bl)->def;
		skilltimer = ((struct map_session_data *)bl)->skilltimer;
		skillid = ((struct map_session_data *)bl)->skillid;
	}
	else if (bl->type == BL_MOB) {
		def = mob_db[((struct mob_data *)bl)->class].def;
		skilltimer = ((struct mob_data *)bl)->skilltimer;
		skillid = ((struct mob_data *)bl)->skillid;
	}
	else if (bl->type == BL_PET)
		def = mob_db[((struct pet_data *)bl)->class].def;

	if(def < 1000000) {
		if(sc_data) {
			//“€Œ‹AÎ‰»‚Í‰EƒVƒtƒg
			if(sc_data[SC_FREEZE].timer != -1 || (sc_data[SC_STONE].timer != -1 && sc_data[SC_STONE].val2 == 0))
				def >>= 1;

			if (bl->type != BL_PC) {
				// Provoke def reduction for monsters.
				if (sc_data[SC_PROVOKE].timer != -1)
					def = (def * (100 - (5 * sc_data[SC_PROVOKE].val1 + 5))) / 100;	// Formula correction. [Bison]
				//ƒL[ƒsƒ“ƒO‚ÍDEF100
				if (sc_data[SC_KEEPING].timer != -1)
					def = 100;
				//í‘¾ŒÛ‚Ì‹¿‚«‚Í‰ÁZ
				if (sc_data[SC_DRUMBATTLE].timer != -1)
					def += sc_data[SC_DRUMBATTLE].val3;
				//“Å‚É‚©‚©‚Á‚Ä‚¢‚é‚ÍŒ¸Z
				if (sc_data[SC_POISON].timer != -1)
					def = def * 75 / 100;
				//ƒXƒgƒŠƒbƒvƒV[ƒ‹ƒh‚ÍŒ¸Z
				if (sc_data[SC_STRIPSHIELD].timer != -1)
					def = def * sc_data[SC_STRIPSHIELD].val2 / 100;
				//ƒVƒOƒiƒ€ƒNƒ‹ƒVƒX‚ÍŒ¸Z
				if (sc_data[SC_SIGNUMCRUCIS].timer != -1)
					def = def * (100 - sc_data[SC_SIGNUMCRUCIS].val2) / 100;
				//‰i‰“‚Ì¬“×‚ÍDEF0‚É‚È‚é
				if (sc_data[SC_ETERNALCHAOS].timer != -1)
					def = 0;
				//ƒRƒ“ƒZƒ“ƒgƒŒ[ƒVƒ‡ƒ“‚ÍŒ¸Z
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
			}
		}
		//‰r¥’†‚Í‰r¥Œ¸Z—¦‚ÉŠî‚Ã‚¢‚ÄŒ¸Z
		if (skilltimer != -1) {
			int def_rate = skill_get_castdef(skillid);
			if (def_rate != 0)
				def = (def * (100 - def_rate)) / 100;
		}
	}
	if (def < 0) def = 0;

	return def;
}

/*==========================================
 * ‘ÎÛ‚ÌMDef‚ğ•Ô‚·(”Ä—p)
 * –ß‚è‚Í®”‚Å0ˆÈã
 *------------------------------------------
 */
int status_get_mdef(struct block_list *bl) {
	struct status_change *sc_data;
	int mdef = 0;

	nullpo_retr(0, bl);

	sc_data=status_get_sc_data(bl);
	if(bl->type == BL_PC)
		mdef = ((struct map_session_data *)bl)->mdef;
	else if(bl->type == BL_MOB)
		mdef = mob_db[((struct mob_data *)bl)->class].mdef;
	else if(bl->type == BL_PET)
		mdef = mob_db[((struct pet_data *)bl)->class].mdef;

	if(mdef < 1000000) {
		if(sc_data) {
			//ƒoƒŠƒA[ó‘Ô‚ÍMDEF100
			if(sc_data[SC_BARRIER].timer != -1)
				mdef = 100;
			//“€Œ‹AÎ‰»‚Í1.25”{
			if(sc_data[SC_FREEZE].timer != -1 || (sc_data[SC_STONE].timer != -1 && sc_data[SC_STONE].val2 == 0))
				mdef = mdef * 125 / 100;
			if(sc_data[SC_MINDBREAKER].timer != -1 && bl->type != BL_PC)
				mdef -= (mdef * 6 * sc_data[SC_MINDBREAKER].val1) / 100;
		}
	}
	if(mdef < 0) mdef = 0;

	return mdef;
}

/*==========================================
 * ‘ÎÛ‚ÌDef2‚ğ•Ô‚·(”Ä—p)
 * –ß‚è‚Í®”‚Å1ˆÈã
 *------------------------------------------
 */
int status_get_def2(struct block_list *bl) {
	int def2 = 1;

	nullpo_retr(1, bl);

	if (bl->type == BL_PC)
		return ((struct map_session_data *)bl)->def2;
	else {
		struct status_change *sc_data;

		if (bl->type == BL_MOB)
			def2 = mob_db[((struct mob_data *)bl)->class].vit;
		else if (bl->type == BL_PET)
			def2 = mob_db[((struct pet_data *)bl)->class].vit;

		sc_data = status_get_sc_data(bl);
		if (sc_data) {
			if (sc_data[SC_ANGELUS].timer != -1)
				def2 = def2 * (110 + 5 * sc_data[SC_ANGELUS].val1) / 100;
			if (sc_data[SC_PROVOKE].timer != -1)
				def2 = (def2 * (100 - (5 * sc_data[SC_PROVOKE].val1) + 5)) / 100;	// Formula correction. [Bison]
			if (sc_data[SC_POISON].timer != -1)
				def2 = def2 * 75 / 100;
			//ƒRƒ“ƒZƒ“ƒgƒŒ[ƒVƒ‡ƒ“‚ÍŒ¸Z
			if (sc_data[SC_CONCENTRATION].timer != -1)
				def2 = def2 * (100 - 5 * sc_data[SC_CONCENTRATION].val1) / 100;
		}
	}

	if (def2 < 1)
		def2 = 1;

	return def2;
}

/*==========================================
 * ‘ÎÛ‚ÌMDef2‚ğ•Ô‚·(”Ä—p)
 * –ß‚è‚Í®”‚Å0ˆÈã
 *------------------------------------------
 */
int status_get_mdef2(struct block_list *bl) {
	int mdef2 = 0;

	nullpo_retr(0, bl);

	if (bl->type == BL_PC)
		return ((struct map_session_data *)bl)->mdef2 + (((struct map_session_data *)bl)->paramc[2] >> 1);
	else {
		struct status_change *sc_data = status_get_sc_data(bl);
		if (bl->type == BL_MOB)
			mdef2 = mob_db[((struct mob_data *)bl)->class].int_ + (mob_db[((struct mob_data *)bl)->class].vit >> 1);
		else if (bl->type == BL_PET)
			mdef2 = mob_db[((struct pet_data *)bl)->class].int_ + (mob_db[((struct pet_data *)bl)->class].vit >> 1);

		if (sc_data) {
			if (sc_data[SC_MINDBREAKER].timer != -1)
				mdef2 -= (mdef2 * 6 * sc_data[SC_MINDBREAKER].val1) / 100;
		}
	}

	if (mdef2 < 0)
		mdef2 = 0;

	return mdef2;
}

/*==========================================
 * ‘ÎÛ‚ÌSpeed(ˆÚ“®‘¬“x)‚ğ•Ô‚·(”Ä—p)
 * –ß‚è‚Í®”‚Å1ˆÈã
 * Speed‚Í¬‚³‚¢‚Ù‚¤‚ªˆÚ“®‘¬“x‚ª‘¬‚¢
 *------------------------------------------
 */
int status_get_speed(struct block_list *bl) {
	nullpo_retr(1000, bl);

	if (bl->type == BL_PC)
		return ((struct map_session_data *)bl)->speed;
	else {
		struct status_change *sc_data = status_get_sc_data(bl);
		int speed = 1000;
		if (bl->type == BL_MOB) {
			speed = ((struct mob_data *)bl)->speed;
			if (battle_config.mobs_level_up) // increase from mobs leveling up [Valaris]
				speed -= ((struct mob_data *)bl)->level - mob_db[((struct mob_data *)bl)->class].lv;
		} else if (bl->type == BL_PET)
			speed = ((struct pet_data *)bl)->msd->petDB->speed;

		if (sc_data) {
			//‘¬“x‘‰Á‚Í25%Œ¸Z
			if (sc_data[SC_INCREASEAGI].timer != -1 && sc_data[SC_DONTFORGETME].timer == -1)
				speed -= speed >> 2;
			//‘¬“xŒ¸­‚Í25%‰ÁZ
			if (sc_data[SC_DECREASEAGI].timer != -1)
				speed = speed * 125 / 100;
			//ƒNƒ@ƒOƒ}ƒCƒA‚Í50%‰ÁZ
			if (sc_data[SC_QUAGMIRE].timer != -1)
				speed = speed * 3 / 2;
			//„‚ğ–Y‚ê‚È‚¢‚Åc‚Í‰ÁZ
			if (sc_data[SC_DONTFORGETME].timer != -1)
				speed = speed * (100 + sc_data[SC_DONTFORGETME].val1 * 2 + sc_data[SC_DONTFORGETME].val2 + (sc_data[SC_DONTFORGETME].val3 & 0xffff)) / 100;
			//‹à„‚Í25%‰ÁZ
			if (sc_data[SC_STEELBODY].timer != -1)
				speed = speed * 125 / 100;
			if (sc_data[SC_DANCING].timer != -1)
				speed *= 6;
			//ô‚¢‚Í450‰ÁZ
			if (sc_data[SC_CURSE].timer != -1)
				speed = speed + 450;
			//ƒEƒBƒ“ƒhƒEƒH[ƒN‚ÍLv*2%Œ¸Z
			if (sc_data[SC_WINDWALK].timer != -1 && sc_data[SC_INCREASEAGI].timer == -1)
				speed -= (speed * (sc_data[SC_WINDWALK].val1 * 2)) / 100;
			if (sc_data[SC_SLOWDOWN].timer != -1)
				speed = speed * 150 / 100;
			if (sc_data[SC_SPEEDUP0].timer != -1)
				speed -= speed >> 2; // speed -= speed * 25 / 100
			if (sc_data[SC_JOINTBEAT].timer != -1) {
				if (sc_data[SC_JOINTBEAT].val2 == 1)
					speed = speed * 150 / 100;
				else if (sc_data[SC_JOINTBEAT].val2 == 3)
					speed = speed * 130 / 100;
			}
		}
		if (speed < 1)
			speed = 1;
		return speed;
	}

	return 1000;
}

/*==========================================
 * ‘ÎÛ‚ÌaDelay(UŒ‚ƒfƒBƒŒƒC)‚ğ•Ô‚·(”Ä—p)
 * aDelay‚Í¬‚³‚¢‚Ù‚¤‚ªUŒ‚‘¬“x‚ª‘¬‚¢
 *------------------------------------------
 */
int status_get_adelay(struct block_list *bl) {
	nullpo_retr(4000, bl);

	if(bl->type==BL_PC)
		return (((struct map_session_data *)bl)->aspd<<1);
	else {
		struct status_change *sc_data=status_get_sc_data(bl);
		int adelay = 4000, aspd_rate = 100, i;
		if(bl->type == BL_MOB)
			adelay = mob_db[((struct mob_data *)bl)->class].adelay;
		else if(bl->type==BL_PET)
			adelay = mob_db[((struct pet_data *)bl)->class].adelay;

		if(sc_data) {
			//ƒc[ƒnƒ“ƒhƒNƒCƒbƒPƒ“g—p‚ÅƒNƒ@ƒOƒ}ƒCƒA‚Å‚à„‚ğ–Y‚ê‚È‚¢‚Åc‚Å‚à‚È‚¢‚Í3Š„Œ¸Z
			if(sc_data[SC_TWOHANDQUICKEN].timer != -1 && sc_data[SC_QUAGMIRE].timer == -1 && sc_data[SC_DONTFORGETME].timer == -1)	// 2HQ
				aspd_rate -= 30;
			//ƒAƒhƒŒƒiƒŠƒ“ƒ‰ƒbƒVƒ…g—p‚Åƒc[ƒnƒ“ƒhƒNƒCƒbƒPƒ“‚Å‚àƒNƒ@ƒOƒ}ƒCƒA‚Å‚à„‚ğ–Y‚ê‚È‚¢‚Åc‚Å‚à‚È‚¢‚Í
			if(sc_data[SC_ADRENALINE].timer != -1 && sc_data[SC_TWOHANDQUICKEN].timer == -1 &&
				sc_data[SC_QUAGMIRE].timer == -1 && sc_data[SC_DONTFORGETME].timer == -1) {	// ƒAƒhƒŒƒiƒŠƒ“ƒ‰ƒbƒVƒ…
				//g—pÒ‚Æƒp[ƒeƒBƒƒ“ƒo[‚ÅŠi·‚ªo‚éİ’è‚Å‚È‚¯‚ê‚Î3Š„Œ¸Z
				if(sc_data[SC_ADRENALINE].val2 || !battle_config.party_skill_penalty)
					aspd_rate -= 30;
				//‚»‚¤‚Å‚È‚¯‚ê‚Î2.5Š„Œ¸Z
				else
					aspd_rate -= 25;
			}
			//ƒXƒsƒAƒNƒBƒbƒPƒ“‚ÍŒ¸Z
			if(sc_data[SC_SPEARQUICKEN].timer != -1 && sc_data[SC_ADRENALINE].timer == -1 &&
				sc_data[SC_TWOHANDQUICKEN].timer == -1 && sc_data[SC_QUAGMIRE].timer == -1 && sc_data[SC_DONTFORGETME].timer == -1)	// ƒXƒsƒAƒNƒBƒbƒPƒ“
				aspd_rate -= sc_data[SC_SPEARQUICKEN].val2;
			//—[“ú‚ÌƒAƒTƒVƒ“ƒNƒƒX‚ÍŒ¸Z
			if(sc_data[SC_ASSNCROS].timer!=-1 && // —[—z‚ÌƒAƒTƒVƒ“ƒNƒƒX
				sc_data[SC_TWOHANDQUICKEN].timer == -1 && sc_data[SC_ADRENALINE].timer == -1 && sc_data[SC_SPEARQUICKEN].timer == -1 &&
				sc_data[SC_DONTFORGETME].timer == -1)
				aspd_rate -= 5+sc_data[SC_ASSNCROS].val1+sc_data[SC_ASSNCROS].val2+sc_data[SC_ASSNCROS].val3;
			//„‚ğ–Y‚ê‚È‚¢‚Åc‚Í‰ÁZ
			if(sc_data[SC_DONTFORGETME].timer!=-1)		// „‚ğ–Y‚ê‚È‚¢‚Å
				aspd_rate += sc_data[SC_DONTFORGETME].val1 * 3 + sc_data[SC_DONTFORGETME].val2 + (sc_data[SC_DONTFORGETME].val3 >> 16);
			//‹à„25%‰ÁZ
			if (sc_data[SC_STEELBODY].timer != -1)	// ‹à„
				aspd_rate += 25;
			//‘‘¬ƒ|[ƒVƒ‡ƒ“g—p‚ÍŒ¸Z
			if (sc_data[i=SC_ASPDPOTION3].timer != -1 || sc_data[i=SC_ASPDPOTION2].timer != -1 || sc_data[i=SC_ASPDPOTION1].timer != -1 || sc_data[i=SC_ASPDPOTION0].timer != -1)
				aspd_rate -= sc_data[i].val2;
			//ƒfƒBƒtƒFƒ“ƒ_[‚Í‰ÁZ
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
		if(aspd_rate != 100)
			adelay = adelay*aspd_rate/100;
		if(adelay < battle_config.monster_max_aspd<<1) adelay = battle_config.monster_max_aspd<<1;
		return adelay;
	}

	return 4000;
}

int status_get_amotion(struct block_list *bl) {
	nullpo_retr(2000, bl);

	if(bl->type==BL_PC)
		return ((struct map_session_data *)bl)->amotion;
	else {
		struct status_change *sc_data=status_get_sc_data(bl);
		int amotion=2000,aspd_rate = 100,i;
		if(bl->type==BL_MOB)
			amotion = mob_db[((struct mob_data *)bl)->class].amotion;
		else if(bl->type==BL_PET)
			amotion = mob_db[((struct pet_data *)bl)->class].amotion;

		if(sc_data) {
			if(sc_data[SC_TWOHANDQUICKEN].timer != -1 && sc_data[SC_QUAGMIRE].timer == -1 && sc_data[SC_DONTFORGETME].timer == -1)	// 2HQ
				aspd_rate -= 30;
			if(sc_data[SC_ADRENALINE].timer != -1 && sc_data[SC_TWOHANDQUICKEN].timer == -1 &&
				sc_data[SC_QUAGMIRE].timer == -1 && sc_data[SC_DONTFORGETME].timer == -1) {	// ƒAƒhƒŒƒiƒŠƒ“ƒ‰ƒbƒVƒ…
				if(sc_data[SC_ADRENALINE].val2 || !battle_config.party_skill_penalty)
					aspd_rate -= 30;
				else
					aspd_rate -= 25;
			}
			if(sc_data[SC_SPEARQUICKEN].timer != -1 && sc_data[SC_ADRENALINE].timer == -1 &&
				sc_data[SC_TWOHANDQUICKEN].timer == -1 && sc_data[SC_QUAGMIRE].timer == -1 && sc_data[SC_DONTFORGETME].timer == -1)	// ƒXƒsƒAƒNƒBƒbƒPƒ“
				aspd_rate -= sc_data[SC_SPEARQUICKEN].val2;
			if(sc_data[SC_ASSNCROS].timer!=-1 && // —[—z‚ÌƒAƒTƒVƒ“ƒNƒƒX
				sc_data[SC_TWOHANDQUICKEN].timer == -1 && sc_data[SC_ADRENALINE].timer == -1 && sc_data[SC_SPEARQUICKEN].timer == -1 &&
				sc_data[SC_DONTFORGETME].timer == -1)
				aspd_rate -= 5+sc_data[SC_ASSNCROS].val1+sc_data[SC_ASSNCROS].val2+sc_data[SC_ASSNCROS].val3;
			if(sc_data[SC_DONTFORGETME].timer!=-1)		// „‚ğ–Y‚ê‚È‚¢‚Å
				aspd_rate += sc_data[SC_DONTFORGETME].val1 * 3 + sc_data[SC_DONTFORGETME].val2 + (sc_data[SC_DONTFORGETME].val3 >> 16);
			if(sc_data[SC_STEELBODY].timer!=-1)	// ‹à„
				aspd_rate += 25;
			if (sc_data[i=SC_ASPDPOTION3].timer != -1 || sc_data[i=SC_ASPDPOTION2].timer != -1 || sc_data[i=SC_ASPDPOTION1].timer != -1 || sc_data[i=SC_ASPDPOTION0].timer != -1)
				aspd_rate -= sc_data[i].val2;
			if(sc_data[SC_DEFENDER].timer != -1)
				amotion += (550 - sc_data[SC_DEFENDER].val1*50);
		}
		if(aspd_rate != 100)
			amotion = amotion*aspd_rate/100;
		if(amotion < battle_config.monster_max_aspd) amotion = battle_config.monster_max_aspd;
		return amotion;
	}

	return 2000;
}

int status_get_dmotion(struct block_list *bl) {
	int ret;
	struct status_change *sc_data;

	nullpo_retr(0, bl);

	sc_data = status_get_sc_data(bl);
	if (bl->type == BL_MOB){
		ret = mob_db[((struct mob_data *)bl)->class].dmotion;
		if (battle_config.monster_damage_delay_rate != 100)
			ret = ret * battle_config.monster_damage_delay_rate / 100;
	}
	else if (bl->type == BL_PC){
		ret = ((struct map_session_data *)bl)->dmotion;
		if (battle_config.pc_damage_delay_rate != 100)
			ret = ret * battle_config.pc_damage_delay_rate / 100;
	}
	else if (bl->type == BL_PET)
		ret = mob_db[((struct pet_data *)bl)->class].dmotion;
	else
		return 2000;

	if (!map[bl->m].flag.gvg && ((bl->type == BL_PC && ((struct map_session_data *)bl)->special_state.infinite_endure) ||
		(sc_data && (sc_data[SC_ENDURE].timer != -1 || sc_data[SC_CONCENTRATION].timer != -1 || sc_data[SC_BERSERK].timer != -1))))
			return 0;

	return ret;
}

int status_get_element(struct block_list *bl) {
	int ret = 20;
	struct status_change *sc_data;

	nullpo_retr(ret, bl);

	sc_data = status_get_sc_data(bl);
	if(bl->type==BL_MOB)	// 10‚ÌˆÊLv*2A‚P‚ÌˆÊ‘®«
		ret=((struct mob_data *)bl)->def_ele;
	else if(bl->type==BL_PC)
		ret=20+((struct map_session_data *)bl)->def_ele;	// –hŒä‘®«Lv1
	else if(bl->type==BL_PET)
		ret = mob_db[((struct pet_data *)bl)->class].element;

	if(sc_data) {
		if( sc_data[SC_BENEDICTIO].timer!=-1 )	// ¹‘Ì~•Ÿ
			ret=26;
		if( sc_data[SC_FREEZE].timer!=-1 )	// “€Œ‹
			ret=21;
		if( sc_data[SC_STONE].timer!=-1 && sc_data[SC_STONE].val2==0)
			ret=22;
	}

	return ret;
}

int status_get_attack_element(struct block_list *bl) {
	int ret = 0;
	struct status_change *sc_data;

	nullpo_retr(0, bl);

	sc_data = status_get_sc_data(bl);
	if(bl->type==BL_MOB)
		ret=0;
	else if(bl->type==BL_PC)
		ret=((struct map_session_data *)bl)->atk_ele;
	else if(bl->type==BL_PET)
		ret=0;

	if(sc_data) {
		if( sc_data[SC_WATERWEAPON].timer!=-1)	// ƒtƒƒXƒgƒEƒFƒ|ƒ“
			ret=1;
		if( sc_data[SC_EARTHWEAPON].timer!=-1)	// ƒTƒCƒYƒ~ƒbƒNƒEƒFƒ|ƒ“
			ret=2;
		if( sc_data[SC_FIREWEAPON].timer!=-1)	// ƒtƒŒ[ƒ€ƒ‰ƒ“ƒ`ƒƒ[
			ret=3;
		if( sc_data[SC_WINDWEAPON].timer!=-1)	// ƒ‰ƒCƒgƒjƒ“ƒOƒ[ƒ_[
			ret=4;
		if( sc_data[SC_ENCPOISON].timer!=-1)	// ƒGƒ“ƒ`ƒƒƒ“ƒgƒ|ƒCƒYƒ“
			ret=5;
		if( sc_data[SC_ASPERSIO].timer!=-1)		// ƒAƒXƒyƒ‹ƒVƒI
			ret=6;
		if( sc_data[SC_SHADOWWEAPON].timer!=-1)
			ret=7;
		if( sc_data[SC_GHOSTWEAPON].timer!=-1)
			ret=8;
	}

	return ret;
}

int status_get_attack_element2(struct block_list *bl) {
	nullpo_retr(0, bl);

	if(bl->type==BL_PC) {
		int ret = ((struct map_session_data *)bl)->atk_ele_;
		struct status_change *sc_data = ((struct map_session_data *)bl)->sc_data;

		if( sc_data[SC_WATERWEAPON].timer!=-1)	// ƒtƒƒXƒgƒEƒFƒ|ƒ“
			ret=1;
		if( sc_data[SC_EARTHWEAPON].timer!=-1)	// ƒTƒCƒYƒ~ƒbƒNƒEƒFƒ|ƒ“
			ret=2;
		if( sc_data[SC_FIREWEAPON].timer!=-1)	// ƒtƒŒ[ƒ€ƒ‰ƒ“ƒ`ƒƒ[
			ret=3;
		if( sc_data[SC_WINDWEAPON].timer!=-1)	// ƒ‰ƒCƒgƒjƒ“ƒOƒ[ƒ_[
			ret=4;
		if( sc_data[SC_ENCPOISON].timer!=-1)	// ƒGƒ“ƒ`ƒƒƒ“ƒgƒ|ƒCƒYƒ“
			ret=5;
		if( sc_data[SC_ASPERSIO].timer!=-1)		// ƒAƒXƒyƒ‹ƒVƒI
			ret=6;
		return ret;
	}

	return 0;
}

int status_get_party_id(struct block_list *bl) {
	nullpo_retr(0, bl);

	if(bl->type == BL_PC)
		return ((struct map_session_data *)bl)->status.party_id;
	else if(bl->type == BL_MOB){
		struct mob_data *md=(struct mob_data *)bl;
		if (md->master_id > 0)
			return -md->master_id;
		return -md->bl.id;
	}
	else if(bl->type == BL_SKILL)
		return ((struct skill_unit *)bl)->group->party_id;

	return 0;
}

int status_get_guild_id(struct block_list *bl) {
	nullpo_retr(0, bl);

	switch(bl->type) {
	case BL_PC:
		return ((struct map_session_data *)bl)->status.guild_id;
	case BL_MOB:
	  {
		struct map_session_data *msd;
		struct mob_data *md = (struct mob_data *)bl;
		if (md->state.special_mob_ai && (msd = map_id2sd(md->master_id)) != NULL) // Alchemist's mobs // 0: nothing, 1: cannibalize, 2-3: spheremine
			return msd->status.guild_id;
		else
			return md->guild_id; // Guilds' gardians & emperiums, otherwize = 0
	  }
	case BL_PET:
		return ((struct pet_data *)bl)->msd->status.guild_id;
	case BL_SKILL:
		return ((struct skill_unit *)bl)->group->guild_id;
	}

	return 0;
}

int status_get_race(struct block_list *bl) {
	nullpo_retr(0, bl);

	switch(bl->type) {
	case BL_MOB:
		return mob_db[((struct mob_data *)bl)->class].race;
	case BL_PC:
		return 7;
	case BL_PET:
		return mob_db[((struct pet_data *)bl)->class].race;
	}

	return 0;
}

int status_get_size(struct block_list *bl)
{
	int retval;
	struct map_session_data *sd = (struct map_session_data *)bl;

	nullpo_retr(1, bl);

	switch (bl->type)
	{
		case BL_MOB:
			retval = mob_db[((struct mob_data *)bl)->class].size;
			break;
		case BL_PET:
			retval = mob_db[((struct pet_data *)bl)->class].size;
			break;
		case BL_PC:	// Medium size for normal players, Small size for baby classes
			retval = (pc_calc_upper(sd->status.class) == 2) ? 0 : 1;
			break;
		default:
			retval = 1;
			break;
	}

	return retval;
}

int status_get_mode(struct block_list *bl) {
	nullpo_retr(0x01, bl);

	if (bl->type == BL_MOB)
		return mob_db[((struct mob_data *)bl)->class].mode;
	else if (bl->type == BL_PET)
		return mob_db[((struct pet_data *)bl)->class].mode;
	else
		return 0x01;	// ‚Æ‚è‚ ‚¦‚¸“®‚­‚Æ‚¢‚¤‚±‚Æ‚Å1
}

/*int status_get_mexp(struct block_list *bl) {
	nullpo_retr(0, bl);

	if (bl->type == BL_MOB)
		return mob_db[((struct mob_data *)bl)->class].mexp;
	else if (bl->type == BL_PET)
		return mob_db[((struct pet_data *)bl)->class].mexp;
	else
		return 0;
}*/

int status_get_race2(struct block_list *bl) {
	nullpo_retr(0, bl);

	if (bl->type == BL_MOB)
		return mob_db[((struct mob_data *)bl)->class].race2;
	else if (bl->type == BL_PET)
		return mob_db[((struct pet_data *)bl)->class].race2;

	return 0;
}

int status_isdead(struct block_list *bl) {
	nullpo_retr(0, bl);

	if (bl->type == BL_MOB)
		return ((struct mob_data *)bl)->state.state == MS_DEAD;
	if (bl->type == BL_PC)
		return pc_isdead((struct map_session_data *)bl);

	return 0;
}

int status_isimmune(struct block_list *bl)
{
	struct map_session_data *sd = (struct map_session_data *)bl;
	
	nullpo_retr(0, bl);
	if (bl->type == BL_PC) {
		if (sd->special_state.no_magic_damage)
			return 1;
		if (sd->sc_count && sd->sc_data[SC_HERMODE].timer != -1)
			return 1;
	}	
	return 0;
}

// StatusChangeŒn‚ÌŠ“¾
struct status_change *status_get_sc_data(struct block_list *bl)
{
	nullpo_retr(NULL, bl);

	if(bl->type==BL_MOB)
		return ((struct mob_data*)bl)->sc_data;
	else if(bl->type==BL_PC)
		return ((struct map_session_data*)bl)->sc_data;

	return NULL;
}

short *status_get_sc_count(struct block_list *bl) {
	nullpo_retr(NULL, bl);

	if(bl->type==BL_MOB)
		return &((struct mob_data*)bl)->sc_count;
	else if(bl->type==BL_PC)
		return &((struct map_session_data*)bl)->sc_count;

	return NULL;
}

short *status_get_opt1(struct block_list *bl) {
	nullpo_retr(0, bl);

	if(bl->type==BL_MOB)
		return &((struct mob_data*)bl)->opt1;
	else if(bl->type==BL_PC)
		return &((struct map_session_data*)bl)->opt1;
	else if(bl->type==BL_NPC)
		return &((struct npc_data*)bl)->opt1;

	return 0;
}

short *status_get_opt2(struct block_list *bl) {
	nullpo_retr(0, bl);

	if(bl->type==BL_MOB)
		return &((struct mob_data*)bl)->opt2;
	else if(bl->type==BL_PC)
		return &((struct map_session_data*)bl)->opt2;
	else if(bl->type==BL_NPC)
		return &((struct npc_data*)bl)->opt2;

	return 0;
}

short *status_get_opt3(struct block_list *bl) {
	nullpo_retr(0, bl);

	if(bl->type==BL_MOB)
		return &((struct mob_data*)bl)->opt3;
	else if(bl->type==BL_PC)
		return &((struct map_session_data*)bl)->opt3;
	else if(bl->type==BL_NPC && (struct npc_data *)bl)
		return &((struct npc_data*)bl)->opt3;

	return 0;
}

short *status_get_option(struct block_list *bl) {
	nullpo_retr(0, bl);

	if(bl->type==BL_MOB)
		return &((struct mob_data*)bl)->option;
	else if(bl->type==BL_PC)
		return &((struct map_session_data*)bl)->status.option;
	else if(bl->type==BL_NPC)
		return &((struct npc_data*)bl)->option;

	return 0;
}

int status_get_sc_def(struct block_list *bl, int type) {
	int sc_def;
	nullpo_retr(0, bl);

	switch (type) {
	case SP_MDEF1:	// mdef
		sc_def = 100 - (3 + status_get_mdef(bl) + status_get_luk(bl) / 3);
		break;
	case SP_MDEF2:	// int
		sc_def = 100 - (3 + status_get_int(bl) + status_get_luk(bl) / 3);
		break;
	case SP_DEF1:	// def
		sc_def = 100 - (3 + status_get_def(bl) + status_get_luk(bl) / 3);
		break;
	case SP_DEF2:	// vit
		sc_def = 100 - (3 + status_get_vit(bl) + status_get_luk(bl) / 3);
		break;
	case SP_LUK:	// luck
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

	if (bl->type == BL_MOB) {
		struct mob_data *md = (struct mob_data *)bl;
		if (md->class == 1288)
			return 0;
		if (sc_def < 50)
			sc_def = 50;
	} else if (bl->type == BL_PC) {
		struct status_change* sc_data = status_get_sc_data(bl);
		if(sc_data && sc_data[SC_SCRESIST].timer != -1)
			return 0; //Immunity to all status
	}

	return (sc_def < 0) ? 0 : sc_def;
}

int status_change_start(struct block_list *bl, int type, int val1, int val2, int val3, int val4, int tick, int flag)
{
	struct map_session_data *sd = NULL;
	struct mob_data *md = NULL;
	struct status_change* sc_data;
	short *sc_count, *option, *opt1, *opt2, *opt3;
	int race, mode, elem;
	int scdef = 0;

	struct {
		unsigned calc : 1; //Re-calculate status_calc_pc
		unsigned send_opt : 1; //Send new option clif_changeoption
		unsigned undead_bl : 1; //Wether the object is undead race/ele or not
	} scflag;

	nullpo_retr(0, bl);

	switch(bl->type)
	{
		case BL_PC:
			sd = (struct map_session_data *)bl;
			if(pc_isdead(sd))
				return 0;
			break;
		case BL_MOB:
			md = (struct mob_data *)bl;
			if(status_isdead(bl))
				return 0;
			break;
		default:
			return 0;
	}

	memset(&scflag, 0, sizeof(scflag)); //Init scflag structure with 0's

	if(type < 0 || type >= SC_MAX) {
		if(battle_config.error_log)
			printf("status_change_start: invalid status change, sc type: (%d)!\n", type);
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

	switch(type) {
		case SC_STONE:
		case SC_FREEZE:
			if(scflag.undead_bl && !(flag&1))
				return 0; //Undead char/mobs can not be stone cursed/frozed
			scdef = 3 + status_get_mdef(bl) + status_get_luk(bl) / 3;
			break;
		case SC_STUN:
		case SC_SILENCE:
		case SC_POISON:
		case SC_DPOISON:
			scdef = 3 + status_get_vit(bl) + status_get_luk(bl) / 3;
			break;
		case SC_SLEEP:
		case SC_BLIND:
			scdef = 3 + status_get_int(bl) + status_get_luk(bl) / 3;
			break;
		case SC_CURSE:
			scdef = 3 + status_get_luk(bl);
			break;

//		case SC_CONFUSION:
		default:
			scdef = 0;
	}
	
	if(scdef >= 100)
		return 0; //Total inmunity, can not be inflicted
	
	if(sd) {
		if(SC_STONE <= type && type <= SC_BLIND){	/* ƒJ[ƒh‚É‚æ‚é‘Ï« */
			if(sd->reseff[type - SC_STONE] > 0 && rand() % 10000 < sd->reseff[type - SC_STONE]){
				if(battle_config.battle_log)
					printf("PC %d skill_sc_start: card‚É‚æ‚éˆÙí‘Ï«”­“®\n",sd->bl.id);
				return 0;
			}
		}
	}

	switch(type) {
		case SC_SLEEP:
		case SC_STOP:
		case SC_ANKLE:
		case SC_SPIDERWEB:
		case SC_CLOSECONFINE:
		case SC_CLOSECONFINE2:
			battle_stopwalking(bl, 1);
	}
	
	// status effects that won't work on bosses and emperium
	if((mode&0x20 && !(flag&1)) || (md && md->class == 1288))
	{
		switch(type)
		{
			case SC_BLESSING:
				if(scflag.undead_bl || race == 6)
					return 0;
				break;
			case SC_FREEZE:
			case SC_STUN:
			case SC_SLEEP:
			case SC_CONFUSION:
			case SC_STOP:
			case SC_STONE:
			case SC_CURSE:
			case SC_SIGNUMCRUCIS:
			case SC_SILENCE:
			case SC_BLIND:
			case SC_QUAGMIRE:
			case SC_DECREASEAGI:
			case SC_PROVOKE:
			case SC_ROKISWEIL:
			case SC_COMA:
					return 0;
		}
	}

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
			case SC_CLOSECONFINE2: //Can't be re-closed in.
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
				break;
			case SC_GOSPEL:
				if(sc_data[type].val4 == BCT_SELF)
					return 0;
				break;
			default:
				if(sc_data[type].val1 > val1)
					return 0;
		}
		
		(*sc_count)--;
		delete_timer(sc_data[type].timer, status_change_timer);
		sc_data[type].timer = -1;
	}
	
	switch(type) { /* ˆÙí‚Ìí—Ş‚²‚Æ‚Ìˆ— */
		case SC_PROVOKE:			/* ƒvƒƒ{ƒbƒN */
			scflag.calc = 1;
			if (tick <= 0) tick = 1000;	/* (ƒI[ƒgƒo[ƒT[ƒN) */
			break;
		case SC_ENDURE:				/* ƒCƒ“ƒfƒ…ƒA */
			if (tick <= 0)
				tick = 1000 * 60;
			scflag.calc = 1; // for updating mdef
			val2 = 7;
			break;
		case SC_AUTOBERSERK:
			if(!(flag&4))
				tick = 60 * 1000;
			if(sd && sd->status.hp < sd->status.max_hp>>2 &&
				(sc_data[SC_PROVOKE].timer == -1 || sc_data[SC_PROVOKE].val2 == 0))
				status_change_start(bl, SC_PROVOKE, 10, 1, 0, 0, 0, 0);
			break;
		case SC_CONCENTRATE:		/* W’†—ÍŒüã */
		case SC_RUN:
		case SC_SPURT:
			scflag.calc = 1;
			break;
		case SC_READYSTORM:
		case SC_READYDOWN:
		case SC_READYTURN:
		case SC_READYCOUNTER:
		case SC_READYDODGE:
			if (flag&4)
				break;
		case SC_BLESSING:			/* ƒuƒŒƒbƒVƒ“ƒO */
			if(bl->type == BL_PC || (!scflag.undead_bl && race != 6)) {
				if(sc_data[SC_CURSE].timer != -1 )
					status_change_end(bl, SC_CURSE, -1);
				if(sc_data[SC_STONE].timer != -1 && sc_data[SC_STONE].val2 == 0)
					status_change_end(bl, SC_STONE, -1);
			}
			scflag.calc = 1;
			break;
		case SC_ANGELUS:			/* ƒAƒ“ƒ[ƒ‹ƒX */
			scflag.calc = 1;
			break;
		case SC_INCREASEAGI:		/* ‘¬“xã¸ */
			scflag.calc = 1;
			if(sc_data[SC_DECREASEAGI].timer != -1 )
				status_change_end(bl,SC_DECREASEAGI,-1);
			break;
		case SC_DECREASEAGI:		/* ‘¬“xŒ¸­ */
			if(bl->type == BL_PC)
				tick >>= 1; //Half duration on players
			scflag.calc = 1;
			if(*sc_count > 0) {
				if(sc_data[SC_INCREASEAGI].timer !=-1 )
					status_change_end(bl, SC_INCREASEAGI, -1);
				if(sc_data[SC_ADRENALINE].timer !=-1 )
					status_change_end(bl, SC_ADRENALINE,-1);
				if(sc_data[SC_SPEARQUICKEN].timer !=-1 )
					status_change_end(bl, SC_SPEARQUICKEN, -1);
				if(sc_data[SC_TWOHANDQUICKEN].timer !=-1 )
					status_change_end(bl, SC_TWOHANDQUICKEN, -1);
				if(sc_data[SC_CARTBOOST].timer !=-1 )
					status_change_end(bl, SC_CARTBOOST, -1);
			}
			break;
		case SC_SIGNUMCRUCIS:		/* ƒVƒOƒiƒ€ƒNƒ‹ƒVƒX */
			scflag.calc = 1;
//			val2 = 14 + val1;
			val2 = 10 + val1*2;
			if(!(flag&4))
				tick = 600*1000;
			clif_emotion(bl,4);
			break;
		case SC_SLOWPOISON:
			if (sc_data[SC_POISON].timer == -1 && sc_data[SC_DPOISON].timer == -1)
				return 0;
			break;
		case SC_TWOHANDQUICKEN:		/* 2HQ */
			if (sc_data[SC_DECREASEAGI].timer != -1)
				return 0;
			*opt3 |= 1;
			scflag.calc = 1;
			break;
		case SC_ADRENALINE:			/* ƒAƒhƒŒƒiƒŠƒ“ƒ‰ƒbƒVƒ… */
			if(sd && !(skill_get_weapontype(BS_ADRENALINE)&(1<<sd->status.weapon)))
				return 0;
			if (sc_data[SC_DECREASEAGI].timer != -1)
				return 0;
			scflag.calc = 1;
			break;
		case SC_WEAPONPERFECTION:	/* ƒEƒFƒ|ƒ“ƒp[ƒtƒFƒNƒVƒ‡ƒ“ */
			/* kRO 14/12/04 Patch - No more time penalty [Aalye]
			if (battle_config.party_skill_penalty && !val2) tick /= 5; */
			break;
		case SC_OVERTHRUST:			/* ƒI[ƒo[ƒXƒ‰ƒXƒg */
			if(sc_data[SC_MAXOVERTHRUST].timer != -1)
				return 0; //Overthrust should not take effect if MAXOVERTHRUST is active
			*opt3 |= 2;
			/* kRO 14/12/04 Patch - No more time penalty [Aalye]
			if (battle_config.party_skill_penalty && !val2) tick /= 10; */
			break;
		case SC_MAXOVERTHRUST:
			if (sc_data[SC_OVERTHRUST].timer != -1)	// cancels normal overthrust
				status_change_end(bl, SC_OVERTHRUST, -1);
			break;
		case SC_MAXIMIZEPOWER:		/* ƒ}ƒLƒVƒ}ƒCƒYƒpƒ[(SP‚ª1Œ¸‚éŠÔ,val2‚É‚à) */
			if(!(flag&4)) {
				if(bl->type == BL_PC)
					val2 = tick;
				else
					tick = 5000 * val1;
			}
			break;
		case SC_ENCPOISON:			/* ƒGƒ“ƒ`ƒƒƒ“ƒgƒ|ƒCƒYƒ“ */
			scflag.calc = 1;
			val2=(((val1 - 1) / 2) + 3)*100;	/* “Å•t—^Šm—¦ */
			skill_enchant_elemental_end(bl, SC_ENCPOISON);
			break;
		case SC_EDP:
			val2 = val1 + 2;			/* –Ò“Å•t—^Šm—¦(%) */
			scflag.calc = 1;
			break;
		case SC_POISONREACT:	/* ƒ|ƒCƒYƒ“ƒŠƒAƒNƒg */
			if(!(flag&4))
				val2 = (val1 >> 1) + val1%2; // [Celest]
			break;
		case SC_IMPOSITIO:			/* ƒCƒ“ƒ|ƒVƒeƒBƒIƒ}ƒkƒX */
			scflag.calc = 1;
			break;
		case SC_ASPERSIO:			/* ƒAƒXƒyƒ‹ƒVƒI */
			skill_enchant_elemental_end(bl, SC_ASPERSIO);
			break;
		case SC_SUFFRAGIUM:			/* ƒTƒtƒ‰ƒMƒ€ */
		case SC_BENEDICTIO:			/* ¹‘Ì */
		case SC_MAGNIFICAT:			/* ƒ}ƒOƒjƒtƒBƒJ[ƒg */
			break;
		case SC_AETERNA:			/* ƒG[ƒeƒ‹ƒi */
			if(sc_data[SC_STONE].timer != -1 || sc_data[SC_FREEZE].timer != -1)
				return 0; //Aexterna should not take effect if target is Frozen or Stonned
			break;
		case SC_ENERGYCOAT:			/* ƒGƒiƒW[ƒR[ƒg */
			*opt3 |= 4;
			break;
		case SC_MAGICROD:
			val2 = val1 * 20;
			break;
		case SC_KYRIE:				/* ƒLƒŠƒGƒGƒŒƒCƒ\ƒ“ */
			if(!(flag&4)) {
				val2 = status_get_max_hp(bl) * (val1 * 2 + 10) / 100;/* ‘Ï‹v“x */
				val3 = (val1 / 2 + 5);	/* ‰ñ” */
				if(sc_data[SC_ASSUMPTIO].timer!=-1 )
					status_change_end(bl,SC_ASSUMPTIO,-1);
			}
			break;
		case SC_MINDBREAKER:
			scflag.calc = 1;
			if(tick <= 0) tick = 1000;	/* (ƒI[ƒgƒo[ƒT[ƒN) */
		case SC_GLORIA:				/* ƒOƒƒŠƒA */
			scflag.calc = 1;
			break;
		case SC_LOUD:				/* ƒ‰ƒEƒhƒ{ƒCƒX */
			scflag.calc = 1;
			break;
		case SC_TRICKDEAD:			/* €‚ñ‚¾‚Ó‚è */
			if(sd)
				pc_stopattack(sd);
			break;
		case SC_QUAGMIRE:			/* ƒNƒ@ƒOƒ}ƒCƒA */
			scflag.calc = 1;
			if(*sc_count > 0) {
				if(sc_data[SC_CONCENTRATE].timer != -1 )	/* W’†—ÍŒüã‰ğœ */
					status_change_end(bl, SC_CONCENTRATE, -1);
				if(sc_data[SC_INCREASEAGI].timer != -1 )	/* ‘¬“xã¸‰ğœ */
					status_change_end(bl, SC_INCREASEAGI, -1);
				if(sc_data[SC_TWOHANDQUICKEN].timer != -1 )
					status_change_end(bl, SC_TWOHANDQUICKEN,-1);
				if(sc_data[SC_SPEARQUICKEN].timer != -1 )
					status_change_end(bl, SC_SPEARQUICKEN, -1);
				if(sc_data[SC_ADRENALINE].timer != -1 )
					status_change_end(bl, SC_ADRENALINE, -1);
				if(sc_data[SC_LOUD].timer != -1 )
					status_change_end(bl, SC_LOUD, -1);
				if(sc_data[SC_TRUESIGHT].timer != -1 )	/* ƒgƒDƒ‹[ƒTƒCƒg */
					status_change_end(bl, SC_TRUESIGHT, -1);
				if(sc_data[SC_WINDWALK].timer != -1 )	/* ƒEƒCƒ“ƒhƒEƒH[ƒN */
					status_change_end(bl, SC_WINDWALK, -1);
				if(sc_data[SC_CARTBOOST].timer != -1 )	/* ƒJ[ƒgƒu[ƒXƒg */
					status_change_end(bl, SC_CARTBOOST,-1);
			}
			break;
		case SC_MAGICPOWER:		/* –‚–@—Í‘• */
			scflag.calc = 1;
			val2 = 1;
			break;
		case SC_SACRIFICE:
			if(!(flag&4))
				val2 = 5;
			break;
		case SC_FIREWEAPON:		/* ƒtƒŒ[ƒ€ƒ‰ƒ“ƒ`ƒƒ[ */
		case SC_WATERWEAPON:		/* ƒtƒƒXƒgƒEƒFƒ|ƒ“ */
		case SC_WINDWEAPON:	/* ƒ‰ƒCƒgƒjƒ“ƒOƒ[ƒ_[ */
		case SC_EARTHWEAPON:		/* ƒTƒCƒYƒ~ƒbƒNƒEƒFƒ|ƒ“ */
		case SC_SHADOWWEAPON:
		case SC_GHOSTWEAPON:
			skill_enchant_elemental_end(bl, type);
			break;
		case SC_DEVOTION:			/* ƒfƒBƒ{[ƒVƒ‡ƒ“ */
			if(!(flag&4))
				scflag.calc = 1;
			break;
		case SC_PROVIDENCE:			/* ƒvƒƒ”ƒBƒfƒ“ƒX */
			scflag.calc = 1;
			val2=val1*5;
			break;
		case SC_REFLECTSHIELD:
			val2=10+val1*3;
			break;
		case SC_STRIPWEAPON:
			if (val2==0) val2=90;
			break;
		case SC_STRIPSHIELD:
			if (val2==0) val2=85;
			break;
		case SC_STRIPARMOR:
		case SC_STRIPHELM:
		case SC_CP_WEAPON:
		case SC_CP_SHIELD:
		case SC_CP_ARMOR:
		case SC_CP_HELM:
			break;

		case SC_AUTOSPELL:			/* ƒI[ƒgƒXƒyƒ‹ */
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
		case SC_VIOLENTGALE:
			scflag.calc = 1;
			val3 = val1*3;
			val4 = val1>=5?20: (val1==4?19: (val1==3?17: ( val1==2?14:10 ) ) );
			break;

		case SC_SPEARQUICKEN:		/* ƒXƒsƒAƒNƒCƒbƒPƒ“ */
			scflag.calc = 1;
			val2 = 20+val1;
			*opt3 |= 1;
			break;
		case SC_CLOSECONFINE:	// confine caster
		case SC_CLOSECONFINE2:	// confine target
			break;
		case SC_COMBO:
			switch (val1) { // skill id
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
		case SC_BLADESTOP_WAIT:		/* ”’næ‚è(‘Ò‚¿) */
			break;
		case SC_BLADESTOP:		/* ”’næ‚è */
			if(flag&4)
				break;
			if(val2 == 2)
				clif_bladestop((struct block_list *)val3, (struct block_list *)val4, 1);
			*opt3 |= 32;
			break;

		case SC_LULLABY:			/* qç‰S */
			val2 = 11;
			break;
		case SC_RICHMANKIM:
			break;
		case SC_ETERNALCHAOS:		/* ƒGƒ^[ƒiƒ‹ƒJƒIƒX */
			scflag.calc = 1;
			break;
		case SC_DRUMBATTLE:			/* í‘¾ŒÛ‚Ì‹¿‚« */
			scflag.calc = 1;
			val2 = (val1+1)*25;
			val3 = (val1+1)*2;
			break;
		case SC_NIBELUNGEN:			/* ƒj[ƒxƒ‹ƒ“ƒO‚Ìw—Ö */
			scflag.calc = 1;
			//val2 = (val1+2)*50;
			val3 = (val1+2)*25;
			break;
		case SC_ROKISWEIL:			/* ƒƒL‚Ì‹©‚Ñ */
			break;
		case SC_INTOABYSS:			/* [•£‚Ì’†‚É */
			break;
		case SC_SIEGFRIED:			/* •s€g‚ÌƒW[ƒNƒtƒŠ[ƒh */
			scflag.calc = 1;
			val2 = 55 + val1*5;
			val3 = val1*10;
			break;
		case SC_DISSONANCE:			/* •s‹¦˜a‰¹ */
			val2 = 10;
			break;
		case SC_WHISTLE:			/* Œû“J */
			scflag.calc = 1;
			break;
		case SC_ASSNCROS:			/* —[—z‚ÌƒAƒTƒVƒ“ƒNƒƒX */
			scflag.calc = 1;
			break;
		case SC_POEMBRAGI:			/* ƒuƒ‰ƒM‚Ì */
			break;
		case SC_APPLEIDUN:			/* ƒCƒhƒDƒ“‚Ì—ÑŒç */
			scflag.calc = 1;
			break;
		case SC_UGLYDANCE:			/* ©•ªŸè‚Èƒ_ƒ“ƒX */
			val2 = 10;
			break;
		case SC_HUMMING:			/* ƒnƒ~ƒ“ƒO */
			scflag.calc = 1;
			break;
		case SC_DONTFORGETME:		/* „‚ğ–Y‚ê‚È‚¢‚Å */
			scflag.calc = 1;
			if(*sc_count > 0) {
				if(sc_data[SC_INCREASEAGI].timer!=-1 )	/* ‘¬“xã¸‰ğœ */
					status_change_end(bl,SC_INCREASEAGI,-1);
				if(sc_data[SC_TWOHANDQUICKEN].timer!=-1 )
					status_change_end(bl,SC_TWOHANDQUICKEN,-1);
				if(sc_data[SC_SPEARQUICKEN].timer!=-1 )
					status_change_end(bl,SC_SPEARQUICKEN,-1);
				if(sc_data[SC_ADRENALINE].timer!=-1 )
					status_change_end(bl,SC_ADRENALINE,-1);
				if(sc_data[SC_ASSNCROS].timer!=-1 )
					status_change_end(bl,SC_ASSNCROS,-1);
				if(sc_data[SC_TRUESIGHT].timer!=-1 )	/* ƒgƒDƒ‹[ƒTƒCƒg */
					status_change_end(bl,SC_TRUESIGHT,-1);
				if (sc_data[SC_WINDWALK].timer != -1)	/* ƒEƒCƒ“ƒhƒEƒH[ƒN */
					status_change_end(bl, SC_WINDWALK, -1);
				if (sc_data[SC_CARTBOOST].timer != -1)	/* ƒJ[ƒgƒu[ƒXƒg */
					status_change_end(bl, SC_CARTBOOST, -1);
			}
			break;
		case SC_FORTUNE:			/* K‰^‚ÌƒLƒX */
			scflag.calc = 1;
			break;
		case SC_SERVICEFORYOU:			/* ƒT[ƒrƒXƒtƒH[ƒ†[ */
			scflag.calc = 1;
			break;
		case SC_MOONLIT:
			val2 = bl->id;
			*opt3 |= 512;
			break;
		case SC_DANCING:			/* ƒ_ƒ“ƒX/‰‰‘t’† */
			scflag.calc = 1;
			if(!(flag&4)) {
				if (val1 == CG_MOONLIT) //To set moonlit sprite effect on both chars [Proximus]
					status_change_start(bl, SkillStatusChangeTable[CG_MOONLIT], 0, 0, 0, 0, tick, 0);
				val3= tick / 1000;
				tick = 1000;
			}
			break;

		case SC_EXPLOSIONSPIRITS:	// ”š—ô”g“®
			scflag.calc = 1;
			val2 = 75 + 25*val1;
			*opt3 |= 8;
			break;
		case SC_STEELBODY:			// ‹à„
			scflag.calc = 1;
			*opt3 |= 16;
			break;
		case SC_EXTREMITYFIST:		/* ˆ¢C—…”e™€Œ */
			break;
		case SC_AUTOCOUNTER:
			val3 = val4 = 0;
			break;

		case SC_ASPDPOTION0:		/* ‘‘¬ƒ|[ƒVƒ‡ƒ“ */
		case SC_ASPDPOTION1:
		case SC_ASPDPOTION2:
		case SC_ASPDPOTION3:
			scflag.calc = 1;
			if(!(flag&4))
				val2 = 5 * (2 + type - SC_ASPDPOTION0);
			break;

		/* atk & matk potions [Valaris] */
		case SC_ATKPOT:
		case SC_MATKPOT:
			scflag.calc = 1;
			break;
		case SC_WEDDING:	//Œ‹¥—p(Œ‹¥ˆßÖ‚É‚È‚Á‚Ä•à‚­‚Ì‚ª’x‚¢‚Æ‚©)
			{
				time_t timer;
				scflag.calc = 1;
				tick = 10000;
				if(!val2)
					val2 = time(&timer);
			}
			break;
		case SC_NOCHAT:	//ƒ`ƒƒƒbƒg‹Ö~ó‘Ô
			{
				time_t timer;

				if(!battle_config.muting_players)
					break;
				if(!(flag&4))
					tick = 60000;
				if(!val2)
					val2 = time(&timer);
				if(sd) {
					clif_updatestatus(sd, SP_MANNER);
					chrif_save(sd); // do pc_makesavestatus and save storage + account_reg/account_reg2 too
				}
			}
			break;
		case SC_SELFDESTRUCTION: //©”š
			clif_skillcasting(bl,bl->id, bl->id,0,0,331,skill_get_time(val2,val1));
			val3 = tick / 1000;
			tick = 1000;
			break;

		/* option1 */
		case SC_STONE:				/* Î‰» */
			if(!(flag&2)) {
				int sc_def = status_get_mdef(bl)*200;
				tick = tick - sc_def;
			}
			if(!(flag&4))
				val3 = tick / 1000;
			if(val3 < 1) val3 = 1;
			if(!(flag&4))
				tick = 5000;
			val2 = 1;
			break;
		case SC_SLEEP:				/* ‡–° */
			if(!(flag&4)) {
//				int sc_def = 100 - (battle_get_int(bl) + status_get_luk(bl)/3);
//				tick = tick * sc_def / 100;
//				if(tick < 1000) tick = 1000;
				tick = 30000;//‡–°‚ÍƒXƒe[ƒ^ƒX‘Ï«‚ÉŠÖ‚í‚ç‚¸30•b
			}
			break;
		case SC_FREEZE:				/* “€Œ‹ */
			if(!(flag&2)) {
				int sc_def = 100 - status_get_mdef(bl);
				tick = tick * sc_def / 100;
			}
			break;
		case SC_STUN:				/* ƒXƒ^ƒ“ival2‚Éƒ~ƒŠ•bƒZƒbƒgj */
			if(!(flag&2)) {
				int sc_def = status_get_sc_def_vit(bl);
				tick = tick * sc_def / 100;
			}
			break;

		/* option2 */
		case SC_DPOISON: /* –Ò“Å */
		{
			int mhp = status_get_max_hp(bl);
			int hp = status_get_hp(bl);
			// MHP?1/4????????
			if (hp > mhp>>2) {
				if(sd) {
					int diff = mhp * 10 / 100;
					if (hp - diff < mhp >> 2)
						hp = hp - (mhp >> 2);
					pc_heal((struct map_session_data *)bl, -hp, 0);
				} else if (bl->type == BL_MOB) {
					struct mob_data *md = (struct mob_data *)bl;
					hp -= mhp * 15 / 100;
					if (hp > mhp >> 2)
						md->hp = hp;
					else
						md->hp = mhp >> 2;
				}
			}
		}	// fall through
		case SC_POISON: /* “Å */
			scflag.calc = 1;
			if (!(flag&2)) {
				int sc_def = 100 - (status_get_vit(bl) + status_get_luk(bl)/5);
				tick = tick * sc_def / 100;
			}
			if(!(flag&4))
				val3 = tick / 1000;
			if (val3 < 1) val3 = 1;
			if(!(flag&4))
				tick = 1000;
			break;
		case SC_SILENCE:			/* ’¾–ÙiƒŒƒbƒNƒXƒfƒr[ƒij */
			if(sc_data[SC_GOSPEL].timer != -1 && sc_data[SC_GOSPEL].val4 == BCT_SELF) {
				status_change_end(bl, SC_GOSPEL, -1);
				break;
			}
			if(!(flag&2)) {
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
		case SC_BLIND:				/* ˆÃ• */
			scflag.calc = 1;
			if(!(flag&4) && tick < 1000)
				tick = 30000;
			if (!(flag&2)) {
				int sc_def = status_get_lv(bl) / 10 + status_get_int(bl) / 15;
				tick = 30000 - sc_def;
			}
			break;
		case SC_CURSE:
			scflag.calc = 1;
			if(!(flag&2)) {
				int sc_def = 100 - status_get_vit(bl);
				tick = tick * sc_def / 100;
			}
			break;

		/* option */
		case SC_HIDING:		/* ƒnƒCƒfƒBƒ“ƒO */
			scflag.calc = 1;
			if(bl->type == BL_PC && !(flag&4)) {
				val2 = tick / 1000;		/* ‘±ŠÔ */
				tick = 1000;
			}
			if(sc_data[SC_CLOSECONFINE].timer != -1)
				status_change_end(bl, SC_CLOSECONFINE, -1);
			if(sc_data[SC_CLOSECONFINE2].timer != -1)
				status_change_end(bl, SC_CLOSECONFINE2, -1);
			break;
		case SC_CHASEWALK:
		case SC_CLOAKING:		/* ƒNƒ[ƒLƒ“ƒO */
			if(flag&4)
				break;
			if(bl->type == BL_PC) {
				scflag.calc = 1; // [Celest]
				val2 = tick;
				val3 = type==SC_CLOAKING ? 130-val1*3 : 135-val1*5;
			}
			else
				tick = 5000 * val1;
			break;
		case SC_SIGHT:			/* ƒTƒCƒg/ƒ‹ƒAƒt */
		case SC_RUWACH:
		case SC_SIGHTBLASTER:
			if(flag&4)
				break;
			val2 = tick/250;
			tick = 10;
			break;

		/* ƒZ[ƒtƒeƒBƒEƒH[ƒ‹Aƒjƒ…[ƒ} */
		case SC_SAFETYWALL:
		case SC_PNEUMA:
			if(flag&4)
				break;
			tick=((struct skill_unit *)val2)->group->limit;
			break;

		/* ƒAƒ“ƒNƒ‹ */
		case SC_ANKLE:
		case SC_STOP:
		case SC_SCRESIST:
		case SC_SHRINK:
			break;

		/* ƒXƒLƒ‹‚¶‚á‚È‚¢/ŠÔ‚ÉŠÖŒW‚µ‚È‚¢ */
		case SC_RIDING:
			scflag.calc = 1;
			tick = 600*1000;
			break;
		case SC_FALCON:
		case SC_WEIGHT50:
		case SC_WEIGHT90:
		case SC_BROKNWEAPON:
		case SC_BROKNARMOR:
			if(flag&4)
				break;
			tick = 600 * 1000;
			break;

		case SC_AUTOGUARD:
			{
				int i, t;
				for(i = val2 = 0; i < val1; i++) {
					t = 5 - (i >> 1);
					val2 += (t < 0) ? 1 : t;
				}
			}
			break;

		case SC_DEFENDER:
			scflag.calc = 1;
			if(!flag)
				val2 = 5 + val1 * 15;
			break;

		case SC_KEEPING:
		case SC_BARRIER:
			scflag.calc = 1;
		case SC_HALLUCINATION:
			break;
		case SC_CONCENTRATION:	/* ƒRƒ“ƒZƒ“ƒgƒŒ[ƒVƒ‡ƒ“ */
			*opt3 |= 1;
			scflag.calc = 1;
			break;
		case SC_TENSIONRELAX:	/* ƒeƒ“ƒVƒ‡ƒ“ƒŠƒ‰ƒbƒNƒX */
			if(flag&4)
				break;
			if(bl->type == BL_PC)
				tick = 10000;
			break;
		case SC_AURABLADE:		/* ƒI[ƒ‰ƒuƒŒ[ƒh */
//		case SC_ASSUMPTIO:		/*  */
//		case SC_HEADCRUSH:		/* ƒwƒbƒhƒNƒ‰ƒbƒVƒ… */
//		case SC_JOINTBEAT:		/* ƒWƒ‡ƒCƒ“ƒgƒr[ƒg */
//		case SC_MARIONETTE:		/* ƒ}ƒŠƒIƒlƒbƒgƒRƒ“ƒgƒ[ƒ‹ */

			//‚Æ‚è‚ ‚¦‚¸è”²‚«
			break;

		case SC_PARRYING:		/* ƒpƒŠƒCƒ“ƒO */
			val2 = 20 + val1 * 3;
			break;

		case SC_WINDWALK:		/* ƒEƒCƒ“ƒhƒEƒH[ƒN */
			scflag.calc = 1;
			val2 = (val1 / 2); //Fleeã¸—¦
			break;

		case SC_JOINTBEAT: // Random break [DracoRPG]
			scflag.calc = 1;
			val2 = rand() % 6 + 1;
			if (val2 == 6)
				status_change_start(bl, SC_BLEEDING, val1, 0, 0, 0, skill_get_time2(type, val1), 0);
			break;

		case SC_BERSERK:		/* ƒo[ƒT[ƒN */
			*opt3 |= 128;
			if(!(flag&4))
				tick = 10000;
			scflag.calc = 1;
			break;

		case SC_ASSUMPTIO:		/* ƒAƒXƒ€ƒvƒeƒBƒI */
			if(sc_data[SC_KYRIE].timer != -1)
				status_change_end(bl, SC_KYRIE, -1);
			*opt3 |= 2048;
			break;

		case SC_BASILICA:
			break;

		case SC_LONGING:
			break;

		case SC_GOSPEL:
			if(val4 == BCT_SELF) {
				val2 = tick;
				tick = 10000;
				status_change_clear_buffs(bl);
				status_change_clear_debuffs(bl);
			}
			break;

		case SC_MARIONETTE:		/* ƒ}ƒŠƒIƒlƒbƒgƒRƒ“ƒgƒ[ƒ‹ */
		case SC_MARIONETTE2:
			if(flag&4)
				break;
			val2 = tick;
			if (!val3)
				return 0;
			tick = 1000;
			scflag.calc = 1;
			*opt3 |= 1024;
			break;

		case SC_MELTDOWN:		/* ƒƒ‹ƒgƒ_ƒEƒ“ */
			scflag.calc = 1;
			break;
		case SC_CARTBOOST:		/* ƒJ[ƒgƒu[ƒXƒg */
			if(sc_data[SC_DECREASEAGI].timer != -1) {
				status_change_end(bl, SC_DECREASEAGI, -1);
				return 0;
			}
		case SC_TRUESIGHT:		/* ƒgƒDƒ‹[ƒTƒCƒg */
		case SC_SPIDERWEB:		/* ƒXƒpƒCƒ_[ƒEƒFƒbƒu */
			scflag.calc = 1;
			break;

		case SC_REJECTSWORD:	/* ƒŠƒWƒFƒNƒgƒ\[ƒh */
			val2 = 3; //3‰ñUŒ‚‚ğ’µ‚Ë•Ô‚·
			val3 = 0; // Damage reflect state - [Aalye]
			break;

		case SC_MEMORIZE:		/* ƒƒ‚ƒ‰ƒCƒY */
			val2 = 5; //3‰ñ‰r¥‚ğ1/5‚É‚·‚é // Memorize is supposed to reduce the cast time of the next 5 spells by half (thanks to [BLB] from freya's bug report)
			break;

		case SC_GRAVITATION:
			if (val3 != BCT_SELF)
				scflag.calc = 1;
			break;

		case SC_HERMODE:
			status_change_clear_buffs(bl);
			break;

		case SC_COMA: //Coma. Sends a char to 1HP/SP
			battle_damage(NULL, bl, status_get_hp(bl)-1, 0);
			return 0;

		case SC_SPLASHER:		/* ƒxƒiƒ€ƒXƒvƒ‰ƒbƒVƒƒ[ */
			break;

		case SC_FOGWALL:
			val2 = 75;
			// scflag.calc = 1;	// not sure of effects yet [celest]
			break;

		case SC_PRESERVE:
			break;

		case SC_DOUBLECAST:
			break;

		case SC_BLEEDING:
			if(!(flag&2)) {
				int sc_def = 100 - (status_get_lv(bl) / 5 + status_get_vit(bl));
				tick = tick * sc_def / 100;
			}
			if(!(flag&4) && tick < 10000)
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
		case SC_INCHITRATE:		/* HIT%ã¸ */
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
			if(!(flag&4))
				tick = 60000; // 1 minute
			scflag.calc = 1;
			break;

		case SC_GUILDAURA:
			tick = 1000;
			scflag.calc = 1;
			break;

		case SC_GDSKILLDELAY:
			break;

		default:
			if (battle_config.error_log)
				printf("Unknown StatusChange [%d].\n", type);
			return 0;
	}
	if (bl->type == BL_PC) {
#ifdef USE_SQL
		if (flag&4)
			clif_status_load(sd, StatusIconTable[type]); //Sending to owner since they aren't in the map yet. [Skotlex]
#endif
		clif_status_change(bl, StatusIconTable[type], 1); /* ƒAƒCƒRƒ“•\¦ */
	}

	/* option‚Ì•ÏX */
	switch(type){
		case SC_STONE:
		case SC_FREEZE:
		case SC_STUN:
		case SC_SLEEP:
			battle_stopattack(bl);	/* UŒ‚’â~ */
			skill_stop_dancing(bl,0);	/* ‰‰‘t/ƒ_ƒ“ƒX‚Ì’†’f */
			{	/* “¯‚ÉŠ|‚©‚ç‚È‚¢ƒXƒe[ƒ^ƒXˆÙí‚ğ‰ğœ */
				int i;
				for(i = SC_STONE; i <= SC_SLEEP; i++){
					if(sc_data[i].timer != -1){
						(*sc_count)--;
						delete_timer(sc_data[i].timer, status_change_timer);
						sc_data[i].timer = -1;
					}
				}
			}
			if(type == SC_STONE)
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
		case SC_DPOISON:	// b’è‚Å“Å‚ÌƒGƒtƒFƒNƒg‚ğg—p
			*opt2 |= 1;
			scflag.send_opt = 1;
			break;
		case SC_SIGNUMCRUCIS:
			*opt2 |= 0x40;
			scflag.send_opt = 1;
			break;
		case SC_HIDING:
		case SC_CLOAKING:
			battle_stopattack(bl);	/* UŒ‚’â~ */
			*option |= ((type == SC_HIDING) ? 2: 4);
			scflag.send_opt = 1;
			break;
		case SC_CHASEWALK:
			battle_stopattack(bl);	/* UŒ‚’â~ */
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
	}

	if (scflag.send_opt) /* option‚Ì•ÏX */
		clif_changeoption(bl);

	(*sc_count)++; /* ƒXƒe[ƒ^ƒXˆÙí‚Ì” */

	sc_data[type].val1 = val1;
	sc_data[type].val2 = val2;
	sc_data[type].val3 = val3;
	sc_data[type].val4 = val4;
	/* ƒ^ƒCƒ}[İ’è */
	sc_data[type].timer = add_timer(gettick_cache + tick, status_change_timer, bl->id, type);

	if(sd) {
		if(scflag.calc)
			status_calc_pc(sd, 0); /* ƒXƒe[ƒ^ƒXÄŒvZ */
		if(type == SC_RUN)
			pc_run(sd, val1, val2);
	}

	return 0;
}

void status_change_clear(struct block_list *bl, int type)
{
	struct status_change* sc_data;
	short *sc_count, *option, *opt1, *opt2, *opt3;
	int i;

	nullpo_retv(bl);
	nullpo_retv(sc_data = status_get_sc_data(bl));
	nullpo_retv(sc_count = status_get_sc_count(bl));
	nullpo_retv(option = status_get_option(bl));
	nullpo_retv(opt1 = status_get_opt1(bl));
	nullpo_retv(opt2 = status_get_opt2(bl));
	nullpo_retv(opt3 = status_get_opt3(bl));

	if(*sc_count == 0)
		return;

	for(i = 0; i < SC_MAX; i++)
	{
		if(sc_data[i].timer == -1)
			continue;
		if(type == 0)
		{
			switch(i)
			{
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
					continue;
			}
		}
		status_change_end(bl, i, -1);
	}

	*opt1 = 0;
	*opt2 = 0;
	*opt3 = 0;
	if (type == BL_PC &&
	    (battle_config.atc_gmonly == 0 || ((struct map_session_data *)bl)->GM_level) &&
	    (((struct map_session_data *)bl)->GM_level >= get_atcommand_level(AtCommand_Hide)))
		*option &= (OPTION_MASK | OPTION_HIDE);
	else
		*option &= OPTION_MASK;

	if(!type || type&2)
		clif_changeoption(bl);

	return;
}

/*==========================================
 * ƒXƒe[ƒ^ƒXˆÙíI—¹
 *------------------------------------------
 */
int status_change_end(struct block_list* bl, int type, int tid)
{
	struct status_change* sc_data;
	int opt_flag = 0, calc_flag = 0;
	short *sc_count, *option, *opt1, *opt2, *opt3;

	nullpo_retr(0, bl);

	if(bl->type != BL_PC && bl->type != BL_MOB)
		return 0;
	
	if(type < 0 || type >= SC_MAX)
		return 0;
		
	nullpo_retr(0, sc_data = status_get_sc_data(bl));
	nullpo_retr(0, sc_count = status_get_sc_count(bl));
	nullpo_retr(0, option = status_get_option(bl));
	nullpo_retr(0, opt1 = status_get_opt1(bl));
	nullpo_retr(0, opt2 = status_get_opt2(bl));
	nullpo_retr(0, opt3 = status_get_opt3(bl));

	if ((*sc_count) > 0 && sc_data[type].timer != -1 && (sc_data[type].timer == tid || tid == -1)) {

		if (tid == -1) // ƒ^ƒCƒ}‚©‚çŒÄ‚Î‚ê‚Ä‚¢‚È‚¢‚È‚çƒ^ƒCƒ}íœ‚ğ‚·‚é
			delete_timer(sc_data[type].timer, status_change_timer);

		/* ŠY“–‚ÌˆÙí‚ğ³í‚É–ß‚· */
		sc_data[type].timer = -1;
		(*sc_count)--;

		switch(type) {	/* ˆÙí‚Ìí—Ş‚²‚Æ‚Ìˆ— */
			case SC_PROVOKE:			/* ƒvƒƒ{ƒbƒN */
			case SC_ENDURE:
			case SC_CONCENTRATE:		/* W’†—ÍŒüã */
			case SC_BLESSING:			/* ƒuƒŒƒbƒVƒ“ƒO */
			case SC_ANGELUS:			/* ƒAƒ“ƒ[ƒ‹ƒX */
			case SC_INCREASEAGI:		/* ‘¬“xã¸ */
			case SC_DECREASEAGI:		/* ‘¬“xŒ¸­ */
			case SC_SIGNUMCRUCIS:		/* ƒVƒOƒiƒ€ƒNƒ‹ƒVƒX */
			case SC_HIDING:
			case SC_TWOHANDQUICKEN:		/* 2HQ */
			case SC_ADRENALINE:			/* ƒAƒhƒŒƒiƒŠƒ“ƒ‰ƒbƒVƒ… */
			case SC_ENCPOISON:			/* ƒGƒ“ƒ`ƒƒƒ“ƒgƒ|ƒCƒYƒ“ */
			case SC_IMPOSITIO:			/* ƒCƒ“ƒ|ƒVƒeƒBƒIƒ}ƒkƒX */
			case SC_GLORIA:				/* ƒOƒƒŠƒA */
			case SC_LOUD:				/* ƒ‰ƒEƒhƒ{ƒCƒX */
			case SC_QUAGMIRE:			/* ƒNƒ@ƒOƒ}ƒCƒA */
			case SC_PROVIDENCE:			/* ƒvƒƒ”ƒBƒfƒ“ƒX */
			case SC_SPEARQUICKEN:		/* ƒXƒsƒAƒNƒCƒbƒPƒ“ */
			case SC_VOLCANO:
			case SC_DELUGE:
			case SC_VIOLENTGALE:
			case SC_ETERNALCHAOS:		/* ƒGƒ^[ƒiƒ‹ƒJƒIƒX */
			case SC_DRUMBATTLE:			/* í‘¾ŒÛ‚Ì‹¿‚« */
			case SC_NIBELUNGEN:			/* ƒj[ƒxƒ‹ƒ“ƒO‚Ìw—Ö */
			case SC_SIEGFRIED:			/* •s€g‚ÌƒW[ƒNƒtƒŠ[ƒh */
			case SC_WHISTLE:			/* Œû“J */
			case SC_ASSNCROS:			/* —[—z‚ÌƒAƒTƒVƒ“ƒNƒƒX */
			case SC_HUMMING:			/* ƒnƒ~ƒ“ƒO */
			case SC_DONTFORGETME:		/* „‚ğ–Y‚ê‚È‚¢‚Å */
			case SC_FORTUNE:			/* K‰^‚ÌƒLƒX */
			case SC_SERVICEFORYOU:			/* ƒT[ƒrƒXƒtƒH[ƒ†[ */
			case SC_EXPLOSIONSPIRITS:	// ”š—ô”g“®
			case SC_STEELBODY:			// ‹à„
			case SC_DEFENDER:
			case SC_ASPDPOTION0:		/* ‘‘¬ƒ|[ƒVƒ‡ƒ“ */
			case SC_ASPDPOTION1:
			case SC_ASPDPOTION2:
			case SC_ASPDPOTION3:
			case SC_APPLEIDUN:			/* ƒCƒhƒDƒ“‚Ì—ÑŒç */
			case SC_RIDING:
			case SC_BLADESTOP_WAIT:
			case SC_AURABLADE:			/* ƒI[ƒ‰ƒuƒŒ[ƒh */
			case SC_PARRYING:			/* ƒpƒŠƒCƒ“ƒO */
			case SC_CONCENTRATION:		/* ƒRƒ“ƒZƒ“ƒgƒŒ[ƒVƒ‡ƒ“ */
			case SC_TENSIONRELAX:		/* ƒeƒ“ƒVƒ‡ƒ“ƒŠƒ‰ƒbƒNƒX */
			case SC_ASSUMPTIO:			/* ƒAƒVƒƒƒ“ƒvƒeƒBƒI */
			case SC_WINDWALK:		/* ƒEƒCƒ“ƒhƒEƒH[ƒN */
			case SC_TRUESIGHT:		/* ƒgƒDƒ‹[ƒTƒCƒg */
			case SC_SPIDERWEB:		/* ƒXƒpƒCƒ_[ƒEƒFƒbƒu */
			case SC_MAGICPOWER:		/* –‚–@—Í‘• */
			case SC_CHASEWALK:
			case SC_ATKPOT:		/* attack potion [Valaris] */
			case SC_MATKPOT:		/* magic attack potion [Valaris] */
			case SC_WEDDING:	//Œ‹¥—p(Œ‹¥ˆßÖ‚É‚È‚Á‚Ä•à‚­‚Ì‚ª’x‚¢‚Æ‚©)
			case SC_MELTDOWN:		/* ƒƒ‹ƒgƒ_ƒEƒ“ */
			case SC_CARTBOOST:
			// Celest
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
			case SC_INCFLEERATE:		/* FLEE%ã¸ */
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
				calc_flag = 1;
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
			case SC_BERSERK:			/* ƒo[ƒT[ƒN */
				calc_flag = 1;
				break;
			case SC_DEVOTION:		/* ƒfƒBƒ{[ƒVƒ‡ƒ“ */
				{
					struct map_session_data *md = map_id2sd(sc_data[type].val1);
					sc_data[type].val1=sc_data[type].val2=0;
					skill_devotion(md,bl->id);
					calc_flag = 1;
				}
				break;
			case SC_BLADESTOP:
				{
				struct status_change *t_sc_data = status_get_sc_data((struct block_list *)sc_data[type].val4);
				//•Ğ•û‚ªØ‚ê‚½‚Ì‚Å‘Šè‚Ì”’nó‘Ô‚ªØ‚ê‚Ä‚È‚¢‚Ì‚È‚ç‰ğœ
				if(t_sc_data && t_sc_data[SC_BLADESTOP].timer!=-1)
					status_change_end((struct block_list *)sc_data[type].val4,SC_BLADESTOP,-1);

				if(sc_data[type].val2==2)
					clif_bladestop((struct block_list *)sc_data[type].val3,(struct block_list *)sc_data[type].val4,0);
				
				}
				break;
			case SC_GRAVITATION:
				if(sc_data[type].val3 != BCT_SELF)
					calc_flag = 1;
				break;

			case SC_DANCING:
				{
					struct map_session_data *dsd;
					struct status_change *d_sc_data;
					if (sc_data[type].val4 && (dsd = map_id2sd(sc_data[type].val4))){
						d_sc_data = dsd->sc_data;
						//‡‘t‚Å‘Šè‚ª‚¢‚éê‡‘Šè‚Ìval4‚ğ0‚É‚·‚é
						if (d_sc_data && d_sc_data[type].timer != -1)
							d_sc_data[type].val4 = 0;
					}
				}
				if (sc_data[type].val1 == CG_MOONLIT)
						status_change_end(bl, SC_MOONLIT, -1); //Remove the sprite effect from both players [Proximus]
				if (sc_data[SC_LONGING].timer!=-1)
					status_change_end(bl,SC_LONGING,-1);
				calc_flag = 1;
				break;

			case SC_SACRIFICE:
				sc_data[SC_SACRIFICE].val2 = 0;
				break;

			case SC_NOCHAT:	//ƒ`ƒƒƒbƒg‹Ö~ó‘Ô
				{
					struct map_session_data *sd = NULL;
					if (bl->type == BL_PC && (sd = (struct map_session_data *)bl)) {
						if (sd->status.manner >= 0)
							sd->status.manner = 0;
						clif_updatestatus(sd, SP_MANNER);
					}
				}
				break;

			case SC_SPLASHER:		/* ƒxƒiƒ€ƒXƒvƒ‰ƒbƒVƒƒ[ */
				{
					struct block_list *src=map_id2bl(sc_data[type].val3);
					if(src && tid!=-1){
						//©•ª‚Éƒ_ƒ[ƒW•üˆÍ3*3‚Éƒ_ƒ[ƒW
						skill_castend_damage_id(src, bl, sc_data[type].val2, sc_data[type].val1, gettick_cache, 0);
					}
				}
				break;
			case SC_CLOSECONFINE:	// caster confine ended
				if (sc_data[type].val2 > 0) { // unlock target
					struct block_list *target = map_id2bl(sc_data[type].val2);
					struct status_change *t_sc_data = status_get_sc_data(target);
					if (target && t_sc_data && t_sc_data[SC_CLOSECONFINE2].timer != -1) // check if target is still confined
						status_change_end(target, SC_CLOSECONFINE2, -1);
				}
				break;
			case SC_CLOSECONFINE2:	// target confine ended
				if (sc_data[type].val2 > 0) {
					struct block_list *src = map_id2bl(sc_data[type].val2);
					struct status_change *t_sc_data = status_get_sc_data(src);
					if (src && t_sc_data && t_sc_data[SC_CLOSECONFINE].timer != -1) // check if caster is still confined
						status_change_end(src, SC_CLOSECONFINE, -1);
				}
				break;
			case SC_SELFDESTRUCTION:		/* ©”š */
				{
					//©•ª‚Ìƒ_ƒ[ƒW‚Í0‚É‚µ‚Ä
					struct mob_data *md = NULL;
					if (bl->type == BL_MOB && (md = (struct mob_data*)bl))
						skill_castend_damage_id(bl, bl, sc_data[type].val2, sc_data[type].val1, gettick_cache, 0);
				}
				break;

		/* option1 */
			case SC_FREEZE:
				sc_data[type].val3 = 0;
				break;

		/* option2 */
			case SC_POISON:				/* “Å */
			case SC_BLIND:				/* ˆÃ• */
			case SC_CURSE:
				calc_flag = 1;
				break;

			// celest
			case SC_CONFUSION:
				break;

			case SC_MARIONETTE:		/* ƒ}ƒŠƒIƒlƒbƒgƒRƒ“ƒgƒ?ƒ‹ */
			case SC_MARIONETTE2:	/// Marionette target
				{
					// check for partner and end their marionette status as well
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
				if(sc_data[type].val4 == BCT_SELF) {
					struct skill_unit_group *group = (struct skill_unit_group *)sc_data[type].val3;
					sc_data[type].val4 = 0;
					skill_delunitgroup(group);
				}
				break;

			case SC_BASILICA:
				if(sc_data[type].val3 == BCT_SELF)
					skill_clear_unitgroup(bl);
				break;
			case SC_HERMODE:
				if(sc_data[type].val3 == BCT_SELF)
					skill_clear_unitgroup(bl);
				else
					calc_flag = 1;
				break;
		}

		if (bl->type == BL_PC)
			clif_status_change(bl, StatusIconTable[type], 0);	/* ƒAƒCƒRƒ“Á‹ */

		switch(type){	/* ³í‚É–ß‚é‚Æ‚«‚È‚É‚©ˆ—‚ª•K—v */
		case SC_STONE:
		case SC_FREEZE:
		case SC_STUN:
		case SC_SLEEP:
			*opt1 = 0;
			opt_flag = 1;
			break;

		case SC_POISON:
			if (sc_data[SC_DPOISON].timer != -1)	//
				break;						// DPOISON—p‚ÌƒIƒvƒVƒ‡ƒ“
			*opt2 &= ~1;					// ‚ªê—p‚É—pˆÓ‚³‚ê‚½ê‡‚É‚Í
			opt_flag = 1;					// ‚±‚±‚Ííœ‚·‚é
			break;							//
		case SC_CURSE:
		case SC_SILENCE:
		case SC_CONFUSION:
		case SC_BLIND:
			*opt2 &= ~(1<<(type - SC_POISON));
			opt_flag = 1;
			break;
		case SC_DPOISON:
			if (sc_data[SC_POISON].timer != -1)	// DPOISON—p‚ÌƒIƒvƒVƒ‡ƒ“‚ª
				break;							// —pˆÓ‚³‚ê‚½‚çíœ
			*opt2 &= ~1;	// “Åó‘Ô‰ğœ
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
			if(type == SC_CHASEWALK)
				*option &= ~16388;
			else {
				*option &= ~((type == SC_HIDING) ? 2 : 4);
				calc_flag = 1;	// orn
			}
			//To avoid hidden/cloaked chars standing on warp portal and attacking enemies
			if (bl->type == BL_PC && (sd = (struct map_session_data *)bl)) {
				if (map_getcell(sd->bl.m, sd->bl.x, sd->bl.y, CELL_CHKNPC)) {
					clif_changeoption(bl);
					npc_touch_areanpc(sd, sd->bl.m, sd->bl.x, sd->bl.y);
					break;
				}
			}
			
			opt_flag = 1;
			break;
		}
		
		case SC_SIGHT:
			*option &= ~1;
			opt_flag = 1;
			break;
		case SC_WEDDING:	//Œ‹¥—p(Œ‹¥ˆßÖ‚É‚È‚Á‚Ä•à‚­‚Ì‚ª’x‚¢‚Æ‚©)
			*option &= ~4096;
			opt_flag = 1;
			break;
		case SC_RUWACH:
			*option &= ~8192;
			opt_flag = 1;
			break;

		//opt3
		case SC_TWOHANDQUICKEN:		/* 2HQ */
		case SC_SPEARQUICKEN:		/* ƒXƒsƒAƒNƒCƒbƒPƒ“ */
		case SC_CONCENTRATION:		/* ƒRƒ“ƒZƒ“ƒgƒŒ[ƒVƒ‡ƒ“ */
			*opt3 &= ~1;
			break;
		case SC_OVERTHRUST:			/* ƒI[ƒo[ƒXƒ‰ƒXƒg */
			*opt3 &= ~2;
			break;
		case SC_ENERGYCOAT:			/* ƒGƒiƒW[ƒR[ƒg */
			*opt3 &= ~4;
			break;
		case SC_EXPLOSIONSPIRITS:	// ”š—ô”g“®
			*opt3 &= ~8;
			break;
		case SC_STEELBODY:			// ‹à„
			*opt3 &= ~16;
			break;
		case SC_BLADESTOP:		/* ”’næ‚è */
			*opt3 &= ~32;
			break;
		case SC_BERSERK:		/* ƒo[ƒT[ƒN */
			*opt3 &= ~128;
			break;
		//256 missing? need to find out what sprite effect is it [Proximus]
		case SC_MOONLIT:
			*opt3 &= ~512;
			break;
		case SC_MARIONETTE:		/* ƒ}ƒŠƒIƒlƒbƒgƒRƒ“ƒgƒ[ƒ‹ */
		case SC_MARIONETTE2:
			*opt3 &= ~1024;
			break;
		case SC_ASSUMPTIO:		/* ƒAƒXƒ€ƒvƒeƒBƒI */
			*opt3 &= ~2048;
			break;
		}

		if(opt_flag)	/* option‚Ì•ÏX‚ğ“`‚¦‚é */
			clif_changeoption(bl);

		if (bl->type == BL_PC && calc_flag)
			status_calc_pc((struct map_session_data *)bl,0);	/* ƒXƒe[ƒ^ƒXÄŒvZ */
	}

	return 0;
}

/*==========================================
 * ƒXƒe[ƒ^ƒXˆÙíI—¹ƒ^ƒCƒ}[
 *------------------------------------------
 */
int status_change_timer(int tid, unsigned int tick, int id, int data)
{
	int type=data;
	struct block_list *bl;
	struct map_session_data *sd = NULL;
	struct status_change *sc_data;
	//short *sc_count; //g‚Á‚Ä‚È‚¢H

	nullpo_retr(0, bl = map_id2bl(id)); //ŠY“–ID‚ª‚·‚Å‚ÉÁ–Å‚µ‚Ä‚¢‚é‚Æ‚¢‚¤‚Ì‚Í‚¢‚©‚É‚à‚ ‚è‚»‚¤‚È‚Ì‚ÅƒXƒ‹[‚µ‚Ä‚İ‚é
	nullpo_retr(0, sc_data = status_get_sc_data(bl));

	if (bl->type == BL_PC)
		nullpo_retr(0, sd = (struct map_session_data *)bl);

	//sc_count = status_get_sc_count(bl); //g‚Á‚Ä‚È‚¢H

	if (sc_data[type].timer != tid) {
		if (battle_config.error_log)
			printf("status_change_timer %d != %d\n", tid, sc_data[type].timer);
		return 0;
	}

	switch(type) { /* “Áê‚Èˆ—‚É‚È‚éê‡ */
	case SC_MAXIMIZEPOWER: /* ƒ}ƒLƒVƒ}ƒCƒYƒpƒ[ */
	case SC_CLOAKING: /* ƒNƒ[ƒLƒ“ƒO */
		if (sd) {
			if (sd->status.sp > 0) {	/* SPØ‚ê‚é‚Ü‚Å‘± */
				sd->status.sp--;
				clif_updatestatus(sd, SP_SP);
				sc_data[type].timer = add_timer(sc_data[type].val2 + tick, status_change_timer, bl->id, data);/* ƒ^ƒCƒ}[Äİ’è */
				return 0;
			}
		}
		break;

	case SC_CHASEWALK:
		if (sd) {
			int sp = 10 + sc_data[SC_CHASEWALK].val1 * 2;
			if (map[sd->bl.m].flag.gvg) sp *= 5;
			if (sd->status.sp > sp){
				sd->status.sp -= sp; // update sp cost [Celest]
				clif_updatestatus(sd, SP_SP);
				if ((++sc_data[SC_CHASEWALK].val4) == 1) {
					status_change_start(bl, SC_INCSTR, 1 << (sc_data[SC_CHASEWALK].val1-1), 0, 0, 0, 30000, 0);
					status_calc_pc(sd, 0);
				}
				sc_data[type].timer = add_timer(sc_data[type].val2 + tick, status_change_timer, bl->id, data); /* ƒ^ƒCƒ}[Äİ’è */
				return 0;
			}
		}
		break;

	case SC_HIDING: /* ƒnƒCƒfƒBƒ“ƒO */
		if (sd) { /* SP‚ª‚ ‚Á‚ÄAŠÔ§ŒÀ‚ÌŠÔ‚Í‘± */
			if (sd->status.sp > 0 && (--sc_data[type].val2) > 0) {
				if(sc_data[type].val2 % (sc_data[type].val1 + 3) == 0) {
					sd->status.sp--;
					clif_updatestatus(sd, SP_SP);
				}
				sc_data[type].timer = add_timer(1000 + tick, status_change_timer, bl->id, data); /* ƒ^ƒCƒ}[Äİ’è */
				return 0;
			}
		}
		break;

	case SC_SIGHT: /* ƒTƒCƒg */
	case SC_RUWACH: /* ƒ‹ƒAƒt */
	case SC_SIGHTBLASTER:		
	  {
		int range = battle_config.ruwach_range;

		if (type == SC_SIGHT)
			range = battle_config.sight_range;
		map_foreachinarea(status_change_timer_sub, bl->m, bl->x - range, bl->y - range, bl->x + range, bl->y + range, 0, bl, type, tick);

		if ((--sc_data[type].val2) > 0) {
			sc_data[type].timer = add_timer(250 + tick, status_change_timer, bl->id, data); /* ƒ^ƒCƒ}[Äİ’è */
			return 0;
		}
	  }
		break;

	case SC_SIGNUMCRUCIS:		/* ƒVƒOƒiƒ€ƒNƒ‹ƒVƒX */
	  {
		int race = status_get_race(bl);
		if (race == 6 || battle_check_undead(race, status_get_elem_type(bl))) {
			sc_data[type].timer = add_timer(1000 * 600 + tick, status_change_timer, bl->id, data);
			return 0;
		}
	  }
		break;

	case SC_PROVOKE:	/* ƒvƒƒ{ƒbƒN/ƒI[ƒgƒo[ƒT[ƒN */
		if (sc_data[type].val2 != 0) { /* ƒI[ƒgƒo[ƒT[ƒNi‚P•b‚²‚Æ‚ÉHPƒ`ƒFƒbƒNj */
			if (sd && sd->status.hp > sd->status.max_hp >> 2) /* ’â~ */
				break;
			sc_data[type].timer = add_timer(1000 + tick, status_change_timer, bl->id, data);
			return 0;
		}
		break;

	case SC_ENDURE:	/* ƒCƒ“ƒfƒ…ƒA */
	case SC_AUTOBERSERK:
		if (sd && sd->special_state.infinite_endure) {
			sc_data[type].timer = add_timer(1000 * 60 + tick, status_change_timer, bl->id, data);
			//sc_data[type].val2=1;
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
		}
		else if ((--sc_data[type].val3) > 0) {
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
		if(sc_data[SC_SLOWPOISON].timer == -1) {
			if ((--sc_data[type].val3) > 0) {
				int hp = status_get_max_hp(bl);
				if (status_get_hp(bl) > hp>>2) {
					if (sd) {
						hp = 3 + hp * 3 / 200;
						pc_heal(sd, -hp, 0);
					}
					else if(bl->type == BL_MOB) {
						struct mob_data *md;
						if ((md = ((struct mob_data *)bl)) == NULL) // not: nullpo_retr(0, md = (struct mob_data *)bl); --> if mob is not more here, it's not an error (not display a message)
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
					if ((md = ((struct mob_data *)bl)) == NULL) // not: nullpo_retr(0, md = (struct mob_data *)bl); --> if mob is not more here, it's not an error (not display a message)
						break;
					hp = 3 + hp/100;
					md->hp -= hp;
				}
			}
		}
		if (sc_data[type].val3 > 0)
			sc_data[type].timer = add_timer(1000 + tick, status_change_timer, bl->id, data);
		break;

	case SC_TENSIONRELAX: /* ƒeƒ“ƒVƒ‡ƒ“ƒŠƒ‰ƒbƒNƒX */
		if (sd) { /* SP‚ª‚ ‚Á‚ÄAHP‚ª–ƒ^ƒ“‚Å‚È‚¯‚ê‚ÎŒp‘± */
			if (sd->status.sp > 12 && sd->status.max_hp > sd->status.hp) {
/*				if (sc_data[type].val2 % (sc_data[type].val1+3) == 0) {
					sd->status.sp -= 12;
					clif_updatestatus(sd, SP_SP);
				}*/
				sc_data[type].timer = add_timer(10000 + tick, status_change_timer, bl->id, data); /* ƒ^ƒCƒ}[Äİ’è */
				return 0;
			}
			if(sd->status.max_hp <= sd->status.hp)
				status_change_end(&sd->bl,SC_TENSIONRELAX,-1);
		}
		break;

	case SC_BLEEDING:	// [celest]
		// i hope i haven't interpreted it wrong.. which i might ^^;
		// Source:
		// - 10õ©ª´ªÈªËHPª¬Êõá´
		// - õóúìªÎªŞªŞ«µ?«Ğì¹ÔÑªä«ê«í«°ª·ªÆªâ?ÍıªÏá¼ª¨ªÊª¤
		// To-do: bleeding effect increases damage taken?
		if ((sc_data[type].val4 -= 10000) >= 0) {
			int hp = rand()%300 + 400;
			if(sd) {
				pc_heal(sd,-hp,0);
			} else if(bl->type == BL_MOB) {
				struct mob_data *md = (struct mob_data *)bl;
				if (md) md->hp -= hp;
			}
			if (!status_isdead(bl)) {
				// walking and casting effect is lost
				battle_stopwalking (bl, 1);
				skill_castcancel (bl, 0);
				sc_data[type].timer = add_timer(10000 + tick, status_change_timer, bl->id, data );
			}
			return 0;
		}
		break;

	/* No time-limit statuses */
	case SC_AETERNA:
	case SC_TRICKDEAD:
	case SC_RIDING:
	case SC_FALCON:
	case SC_WEIGHT50:
	case SC_WEIGHT90:
	case SC_MAGICPOWER:		/* –‚–@—Í‘• */
	case SC_REJECTSWORD:	/* ƒŠƒWƒFƒNƒgƒ\[ƒh */
	case SC_MEMORIZE:	/* ƒƒ‚ƒ‰ƒCƒY */
	case SC_BROKNWEAPON:
	case SC_BROKNARMOR:
	case SC_SACRIFICE:
	case SC_RUN:
	case SC_READYSTORM:
	case SC_READYDOWN:
	case SC_READYTURN:
	case SC_READYCOUNTER:
	case SC_READYDODGE:
		sc_data[type].timer = add_timer(1000 * 600 + tick, status_change_timer, bl->id, data);
		return 0;
	/* End no time-limit statuses */
	
	case SC_DANCING: //ƒ_ƒ“ƒXƒXƒLƒ‹‚ÌŠÔSPÁ”ï
		{
			int s = 0, sp = 1;
			if(sd){
				if(sd->status.sp > 0 && (--sc_data[type].val3)>0){
					switch(sc_data[type].val1){
					case BD_RICHMANKIM:				/* ƒjƒˆƒ‹ƒh‚Ì‰ƒ 3•b‚ÉSP1 */
					case BD_DRUMBATTLEFIELD:		/* í‘¾ŒÛ‚Ì‹¿‚« 3•b‚ÉSP1 */
					case BD_RINGNIBELUNGEN:			/* ƒj[ƒxƒ‹ƒ“ƒO‚Ìw—Ö 3•b‚ÉSP1 */
					case BD_SIEGFRIED:				/* •s€g‚ÌƒW[ƒNƒtƒŠ[ƒh 3•b‚ÉSP1 */
					case BA_DISSONANCE:				/* •s‹¦˜a‰¹ 3•b‚ÅSP1 */
					case BA_ASSASSINCROSS:			/* —[—z‚ÌƒAƒTƒVƒ“ƒNƒƒX 3•b‚ÅSP1 */
					case DC_UGLYDANCE:				/* ©•ªŸè‚Èƒ_ƒ“ƒX 3•b‚ÅSP1 */
						s=3;
						break;
					case BD_LULLABY:				/* qç‰Ì 4•b‚ÉSP1 */
					case BD_ETERNALCHAOS:			/* ‰i‰“‚Ì¬“× 4•b‚ÉSP1 */
					case BD_ROKISWEIL:				/* ƒƒL‚Ì‹©‚Ñ 4•b‚ÉSP1 */
					case DC_FORTUNEKISS:			/* K‰^‚ÌƒLƒX 4•b‚ÅSP1 */
						s=4;
						break;
					case CG_HERMODE:
						sp = 5;
					case BD_INTOABYSS:				/* [•£‚Ì’†‚É 5•b‚ÉSP1 */
					case BA_WHISTLE:				/* Œû“J 5•b‚ÅSP1 */
					case DC_HUMMING:				/* ƒnƒ~ƒ“ƒO 5•b‚ÅSP1 */
					case BA_POEMBRAGI:				/* ƒuƒ‰ƒM‚Ì 5•b‚ÅSP1 */
					case DC_SERVICEFORYOU:			/* ƒT[ƒrƒXƒtƒH[ƒ†[ 5•b‚ÅSP1 */
						s = 5;
						break;
					case BA_APPLEIDUN: /* ƒCƒhƒDƒ“‚Ì—ÑŒç 6•b‚ÅSP1 */
						s = 6;
						break;
					case DC_DONTFORGETME: /* „‚ğ–Y‚ê‚È‚¢‚Åc 10•b‚ÅSP1 */
					case CG_MOONLIT: /* Œ–¾‚è‚Ìò‚É—‚¿‚é‰Ô‚Ñ‚ç 10•b‚ÅSP1H */
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
					sc_data[type].timer = add_timer(1000 + tick, status_change_timer, bl->id, data); /* ƒ^ƒCƒ}[Äİ’è */
					return 0;
				}
				else if (sd->status.sp <= 0) {
					if(sc_data[SC_DANCING].timer != -1)
						skill_stop_dancing(&sd->bl,0);
				}
			}
		}
		break;
	case SC_BERSERK: /* ƒo[ƒT[ƒN */
		if (sd) { /* HP‚ª100ˆÈã‚È‚çŒp‘± */
			if ((sd->status.hp - sd->status.max_hp * 5 / 100) > 100 ) { // 5% every 10 seconds [DracoRPG]
				sd->status.hp -= sd->status.max_hp * 5 / 100; // changed to max hp [celest]
				clif_updatestatus(sd, SP_HP);
				sc_data[type].timer = add_timer(10000 + tick, status_change_timer, bl->id, data); /* ƒ^ƒCƒ}[Äİ’è */
				return 0;
			}
		}
		break;
	case SC_WEDDING: //Œ‹¥—p(Œ‹¥ˆßÖ‚É‚È‚Á‚Ä•à‚­‚Ì‚ª’x‚¢‚Æ‚©)
		if (sd) {
			time_t timer;
			if (time(&timer) < ((sc_data[type].val2) + 3600)) { //1ŠÔ‚½‚Á‚Ä‚¢‚È‚¢‚Ì‚ÅŒp‘±
				sc_data[type].timer = add_timer(10000 + tick, status_change_timer, bl->id, data); /* ƒ^ƒCƒ}[Äİ’è */
				return 0;
			}
		}
		break;
	case SC_NOCHAT: //ƒ`ƒƒƒbƒg‹Ö~ó‘Ô
		if (sd && battle_config.muting_players) {
			time_t timer;
			if ((++sd->status.manner) && time(&timer) < ((sc_data[type].val2) + 60 * (0 - sd->status.manner))) { //ŠJn‚©‚çstatus.manner•ªŒo‚Á‚Ä‚È‚¢‚Ì‚ÅŒp‘±
				clif_updatestatus(sd, SP_MANNER);
				sc_data[type].timer = add_timer(60000 + tick, status_change_timer, bl->id, data); /* ƒ^ƒCƒ}[Äİ’è(60•b) */
				return 0;
			}
		}
		break;
	case SC_SELFDESTRUCTION: /* ©”š */
		if (--sc_data[type].val3 > 0) {
			struct mob_data *md;
			if (bl->type == BL_MOB && (md = (struct mob_data *)bl) && md->speed > 250) {
				md->speed -= 250;
				md->next_walktime = tick;
			}
			sc_data[type].timer = add_timer(1000 + tick, status_change_timer, bl->id, data); /* ƒ^ƒCƒ}[Äİ’è */
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

	case SC_MARIONETTE: /* ƒ}ƒŠƒIƒlƒbƒgƒRƒ“ƒgƒ?ƒ‹ */
	case SC_MARIONETTE2:
	  {
		struct block_list *pbl = map_id2bl(sc_data[type].val3);
		if (pbl && battle_check_range(bl, pbl, 7) && (sc_data[type].val2 -= 1000) > 0) {
			sc_data[type].timer = add_timer(1000 + tick, status_change_timer, bl->id, data);
			return 0;
		}
	  }
		break;

	// Celest
	case SC_CONFUSION:
		if ((sc_data[type].val2 -= 1500) > 0) {
			sc_data[type].timer = add_timer(3000 + tick, status_change_timer, bl->id, data);
			return 0;
		}
		break;

	case SC_GOSPEL:
		if(sc_data[type].val4 == BCT_SELF) { //Crusader SP/HP consumition
			int hp, sp;
			hp = (sc_data[type].val1 > 5) ? 45 : 30;
			sp = (sc_data[type].val1 > 5) ? 35 : 20;
			if(status_get_hp(bl) - hp > 0 && (sd == NULL || sd->status.sp - sp > 0))
			{
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

	return status_change_end( bl,type,tid );
}

/*==========================================
 * ƒXƒe[ƒ^ƒXˆÙíƒ^ƒCƒ}[”ÍˆÍˆ—
 *------------------------------------------
 */
int status_change_timer_sub(struct block_list *bl, va_list ap) {
	struct block_list *src;
	int type;
	unsigned int tick;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);
	nullpo_retr(0, src = va_arg(ap, struct block_list*));
	type=va_arg(ap, int);
	tick=va_arg(ap, unsigned int);

	if (bl->type != BL_PC && bl->type != BL_MOB)
		return 0;

	switch(type) {
	case SC_SIGHT: /* ƒTƒCƒg */
	case SC_CONCENTRATE:
		if ((*status_get_option(bl)) & 6) {
			status_change_end(bl, SC_HIDING, -1);
			status_change_end(bl, SC_CLOAKING, -1);
		}
		break;
	case SC_RUWACH: /* ƒ‹ƒAƒt */
		if ((*status_get_option(bl)) & 6) {
			struct status_change *sc_data = status_get_sc_data(bl); // check whether the target is hiding/cloaking [celest]
			if (sc_data && (sc_data[SC_HIDING].timer != -1 || // if the target is using a special hiding, i.e not using normal hiding/cloaking, don't bother
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
		//sc_ check prevents a single round of Sight Blaster hitting multiple opponents. [Skotlex]
			skill_attack(BF_MAGIC, src, src, bl, WZ_SIGHTBLASTER, 1, tick, 0);
			sc_data[type].val2 = 0; //This signals it to end.
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

int status_change_clear_buffs (struct block_list *bl) {
	int i;
	struct status_change *sc_data = status_get_sc_data(bl);
	if (!sc_data)
		return 0;

	for (i = 0; i < SC_BUFFMAX; i++) {
		if(i == SC_DECREASEAGI || i == SC_SLOWDOWN)
			continue;
		if(sc_data[i].timer != -1)
			status_change_end(bl,i,-1);
	}
	return 0;
}

int status_change_clear_debuffs (struct block_list *bl)
{
	int i;
	struct status_change *sc_data = status_get_sc_data(bl);
	if (!sc_data)
		return 0;
	for (i = SC_STONE; i <= SC_DPOISON; i++) {
		if(sc_data[i].timer != -1)
			status_change_end(bl, i, -1);
	}
	if(sc_data[SC_HALLUCINATION].timer != -1)
		status_change_end(bl, SC_HALLUCINATION, -1);
	if(sc_data[SC_QUAGMIRE].timer != -1)
		status_change_end(bl, SC_QUAGMIRE, -1);
	if(sc_data[SC_SIGNUMCRUCIS].timer != -1)
		status_change_end(bl, SC_SIGNUMCRUCIS, -1);
	if(sc_data[SC_DECREASEAGI].timer != -1)
		status_change_end(bl, SC_DECREASEAGI, -1);
	if(sc_data[SC_SLOWDOWN].timer != -1)
		status_change_end(bl, SC_SLOWDOWN, -1);
	if(sc_data[SC_MINDBREAKER].timer != -1)
		status_change_end(bl, SC_MINDBREAKER, -1);
	if(sc_data[SC_STRIPWEAPON].timer != -1)
		status_change_end(bl, SC_STRIPWEAPON, -1);
	if(sc_data[SC_STRIPSHIELD].timer != -1)
		status_change_end(bl, SC_STRIPSHIELD, -1);
	if(sc_data[SC_STRIPARMOR].timer != -1)
		status_change_end(bl, SC_STRIPARMOR, -1);
	if(sc_data[SC_STRIPHELM].timer != -1)
		status_change_end(bl, SC_STRIPHELM, -1);
	return 0;
}

static int status_calc_sigma(void) {
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

int status_readdb(void) {
	int i, j, k;
	FILE *fp;
	char line[1024], *p;

	// JOB•â³”’l‚P
	fp = fopen("db/job_db1.txt","r");
	if (fp == NULL) {
		printf("can't read db/job_db1.txt\n");
		return 1;
	}
	i = 0;
	while(fgets(line, sizeof(line), fp)){ // fgets reads until maximum one less than size and add '\0' -> so, it's not necessary to add -1
		char *split[50];
		if ((line[0] == '/' && line[1] == '/') || line[0] == '\0' || line[0] == '\n' || line[0] == '\r')
			continue;
		// it's not necessary to remove 'carriage return ('\n' or '\r')
		for(j = 0, p = line; j < 21 && p; j++) {
			split[j] = p;
			p = strchr(p, ',');
			if (p) *p++ = 0;
		}
		if (j < 21)
			continue;
		max_weight_base[i] = atoi(split[0]);
		hp_coefficient[i] = atoi(split[1]);
		hp_coefficient2[i] = atoi(split[2]);
		sp_coefficient[i] = atoi(split[3]);
		for(j=0;j<17;j++)
			aspd_base[i][j] = atoi(split[j+4]);
		i++;
		if (i == MAX_PC_JOB_CLASS)
			break;
	}
	fclose(fp);
	printf("DB '" CL_WHITE "db/job_db1.txt" CL_RESET "' readed.\n");

	// JOBƒ{[ƒiƒX
	memset(&job_bonus, 0, sizeof(job_bonus));
	fp = fopen("db/job_db2.txt", "r");
	if (fp == NULL){
		printf("can't read db/job_db2.txt\n");
		return 1;
	}
	i = 0;
	while(fgets(line, sizeof(line), fp)) { // fgets reads until maximum one less than size and add '\0' -> so, it's not necessary to add -1
		if ((line[0] == '/' && line[1] == '/') || line[0] == '\0' || line[0] == '\n' || line[0] == '\r')
			continue;
		// it's not necessary to remove 'carriage return ('\n' or '\r')
		for(j=0,p=line;j<MAX_LEVEL && p;j++){
			if(sscanf(p,"%d",&k)==0)
				break;
			job_bonus[0][i][j]=k;
			job_bonus[2][i][j]=k; //—{qE‚Ìƒ{[ƒiƒX‚Í•ª‚©‚ç‚È‚¢‚Ì‚Å‰¼
			p=strchr(p,',');
			if(p) p++;
		}
		i++;

		if(i==MAX_PC_JOB_CLASS)
			break;
	}
	fclose(fp);
	printf("DB '" CL_WHITE "db/job_db2.txt" CL_RESET "' readed.\n");

	// JOBƒ{[ƒiƒX2 “]¶E—p
	fp = fopen("db/job_db2-2.txt","r");
	if (fp == NULL) {
		printf("can't read db/job_db2-2.txt\n");
		return 1;
	}
	i=0;
	while(fgets(line, sizeof(line), fp)) { // fgets reads until maximum one less than size and add '\0' -> so, it's not necessary to add -1
		if ((line[0] == '/' && line[1] == '/') || line[0] == '\0' || line[0] == '\n' || line[0] == '\r')
			continue;
		// it's not necessary to remove 'carriage return ('\n' or '\r')
		for(j=0,p=line;j<MAX_LEVEL && p;j++){
			if(sscanf(p,"%d",&k)==0)
				break;
			job_bonus[1][i][j]=k;
			p=strchr(p,',');
			if(p) p++;
		}
		i++;
		if (i == MAX_PC_JOB_CLASS)
			break;
	}
	fclose(fp);
	printf("DB '" CL_WHITE "db/job_db2-2.txt" CL_RESET "' readed.\n");

	// ƒTƒCƒY•â³ƒe[ƒuƒ‹
	for(i=0;i<3;i++)
		for(j = 0; j < 20; j++)
			atkmods[i][j] = 100;
	fp=fopen("db/size_fix.txt","r");
	if(fp==NULL){
		printf("can't read db/size_fix.txt\n");
		return 1;
	}
	i=0;
	while(fgets(line, sizeof(line), fp)) { // fgets reads until maximum one less than size and add '\0' -> so, it's not necessary to add -1
		char *split[20];
		if ((line[0] == '/' && line[1] == '/') || line[0] == '\0' || line[0] == '\n' || line[0] == '\r')
			continue;
		// it's not necessary to remove 'carriage return ('\n' or '\r')
		if(atoi(line)<=0)
			continue;
		memset(split,0,sizeof(split));
		for(j = 0, p = line; j < 20 && p; j++) {
			split[j] = p;
			p = strchr(p, ',');
			if (p) *p++ = 0;
		}
		for(j = 0; j < 20 && split[j]; j++)
			atkmods[i][j] = atoi(split[j]);
		i++;
	}
	fclose(fp);
	printf("DB '" CL_WHITE "db/size_fix.txt" CL_RESET "' readed ('" CL_WHITE "%d" CL_RESET "' entrie%s).\n", i, (i > 1) ? "s" : "");

	// ¸˜Bƒf[ƒ^ƒe[ƒuƒ‹
	for(i=0;i<5;i++){
		for(j=0;j<10;j++)
			percentrefinery[i][j]=100;
		refinebonus[i][0]=0;
		refinebonus[i][1]=0;
		refinebonus[i][2]=10;
	}
	fp = fopen("db/refine_db.txt","r");
	if (fp == NULL) {
		printf("can't read db/refine_db.txt\n");
		return 1;
	}
	i=0;
	while(fgets(line, sizeof(line), fp)) { // fgets reads until maximum one less than size and add '\0' -> so, it's not necessary to add -1
		char *split[16];
		if ((line[0] == '/' && line[1] == '/') || line[0] == '\0' || line[0] == '\n' || line[0] == '\r')
			continue;
		// it's not necessary to remove 'carriage return ('\n' or '\r')
		if(atoi(line)<=0)
			continue;
		memset(split, 0, sizeof(split));
		for(j = 0, p = line; j < 16 && p; j++) {
			split[j] = p;
			p = strchr(p,',');
			if (p) *p++ = 0;
		}
		refinebonus[i][0] = atoi(split[0]);	// ¸˜Bƒ{[ƒiƒX
		refinebonus[i][1] = atoi(split[1]);	// ‰ßè¸˜Bƒ{[ƒiƒX
		refinebonus[i][2] = atoi(split[2]);	// ˆÀ‘S¸˜BŒÀŠE
		for(j = 0; j < 10 && split[j]; j++)
			percentrefinery[i][j] = atoi(split[j+3]);
		i++;
	}
	fclose(fp);
	printf("DB '" CL_WHITE "db/refine_db.txt" CL_RESET "' readed ('" CL_WHITE "%d" CL_RESET "' entrie%s).\n", i, (i > 1) ? "s" : "");

	return 0;
}

/*==========================================
 * ƒXƒLƒ‹ŠÖŒW‰Šú‰»ˆ—
 *------------------------------------------
 */
int do_init_status(void) {
	if (SC_MAX > MAX_STATUSCHANGE) {
		printf("ERROR: status.h defines %d status changes, but the MAX_STATUSCHANGE in map.h definition is %d! Fix it.\n", SC_MAX, MAX_STATUSCHANGE);
		exit(1);
	}
	initStatusIconTable();
	add_timer_func_list(status_change_timer, "status_change_timer");
	status_readdb();
	status_calc_sigma();

	return 0;
}
