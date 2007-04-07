// $Id: status.c 718 2006-08-06 18:31:36Z akrus $

#include <config.h>

#include <string.h>
#include <stdlib.h>
#include <stdint.h>

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

#define STATE_BLIND 0x10

int SkillStatusChangeTable[]={
/* 0- */
	-1,-1,-1,-1,-1,-1,
	SC_PROVOKE,			/* プロボック */
	-1,
	SC_ENDURE,
	-1,
/* 10- */
	SC_SIGHT,			/* サイト */
	-1,
	SC_SAFETYWALL,		/* セーフティーウォール */
	-1,-1,-1,
	SC_FREEZE,			/* フロストダイバ? */
	SC_STONE,			/* スト?ンカ?ス */
	-1,-1,
/* 20- */
	-1,-1,-1,-1,
	SC_RUWACH,			/* ルアフ */
	SC_PNEUMA,			/* ニューマ */
	-1,-1,-1,
	SC_INCREASEAGI,		/* 速度増加 */
/* 30- */
	SC_DECREASEAGI,		/* 速度減少 */
	-1,
	SC_SIGNUMCRUCIS,	/* シグナムクルシス */
	SC_ANGELUS,			/* エンジェラス */
	SC_BLESSING,		/* ブレッシング */
	-1,-1,-1,-1,-1,
/* 40- */
	-1,-1,-1,-1,-1,
	SC_CONCENTRATE,		/* 集中力向上 */
	-1,-1,-1,-1,
/* 50- */
	-1,
	SC_HIDING,			/* ハイディング */
	-1,-1,-1,-1,-1,-1,-1,-1,
/* 60- */
	SC_TWOHANDQUICKEN,	/* 2HQ */
	SC_AUTOCOUNTER,
	-1,-1,-1,-1,
	SC_IMPOSITIO,		/* インポシティオマヌス */
	SC_SUFFRAGIUM,		/* サフラギウム */
	SC_ASPERSIO,		/* アスペルシオ */
	SC_BENEDICTIO,		/* 聖体降福 */
/* 70- */
	-1,
	SC_SLOWPOISON,
	-1,
	SC_KYRIE,			/* キリエエレイソン */
	SC_MAGNIFICAT,		/* マグニフィカート */
	SC_GLORIA,			/* グロリア */
	SC_SILENCE,			/* レックスディビーナ */
	-1,
	SC_AETERNA,			/* レックスエーテルナ */
	-1,
/* 80- */
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
/* 90- */
	-1,-1,
	SC_QUAGMIRE,		/* クァグマイア */
	-1,-1,-1,-1,-1,-1,-1,
/* 100- */
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
/* 110- */
	-1,
	SC_ADRENALINE,		/* アドレナリンラッシュ */
	SC_WEAPONPERFECTION,/* ウェポンパーフェクション */
	SC_OVERTHRUST,		/* オーバートラスト */
	SC_MAXIMIZEPOWER,	/* マキシマイズパワー */
	-1,-1,-1,-1,-1,
/* 120- */
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
/* 130- */
	-1,-1,-1,-1,-1,
	SC_CLOAKING,		/* クローキング */
	SC_STAN,			/* ソニックブロー */
	-1,
	SC_ENCPOISON,		/* エンチャントポイズン */
	SC_POISONREACT,		/* ポイズンリアクト */
/* 140- */
	SC_POISON,			/* ベノムダスト */
	SC_SPLASHER,		/* ベナムスプラッシャー */
	-1,
	SC_TRICKDEAD,		/* 死んだふり */
	-1,-1,SC_AUTOBERSERK,-1,-1,-1,
/* 150- */
	-1,-1,-1,-1,-1,
	SC_LOUD,			/* ラウドボイス */
	-1,
	SC_ENERGYCOAT,		/* エナジーコート */
	-1,-1,
/* 160- */
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,
	SC_KEEPING,
	-1,
	SC_COMA,
	SC_BARRIER,
	-1,-1,
	SC_HALLUCINATION,
	-1,-1,
/* 210- */
	-1,-1,-1,-1,-1,
	SC_STRIPWEAPON,
	SC_STRIPSHIELD,
	SC_STRIPARMOR,
	SC_STRIPHELM,
	-1,
/* 220- */
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
/* 230- */
	-1,-1,-1,-1,
	SC_CP_WEAPON,
	SC_CP_SHIELD,
	SC_CP_ARMOR,
	SC_CP_HELM,
	-1,-1,
/* 240- */
	-1,-1,-1,-1,-1,-1,-1,-1,-1,
	SC_AUTOGUARD,
/* 250- */
	-1,-1,
	SC_REFLECTSHIELD,
	-1,-1,
	SC_DEVOTION,
	SC_PROVIDENCE,
	SC_DEFENDER,
	SC_SPEARSQUICKEN,
	-1,
/* 260- */
	-1,-1,-1,-1,-1,-1,-1,-1,
	SC_STEELBODY,
	SC_BLADESTOP_WAIT,
/* 270- */
	SC_EXPLOSIONSPIRITS,
	SC_EXTREMITYFIST,
	-1,-1,-1,-1,
	SC_MAGICROD,
	-1,-1,-1,
/* 280- */
	SC_FLAMELAUNCHER,
	SC_FROSTWEAPON,
	SC_LIGHTNINGLOADER,
	SC_SEISMICWEAPON,
	-1,
	SC_VOLCANO,
	SC_DELUGE,
	SC_VIOLENTGALE,
	SC_LANDPROTECTOR,
	-1,
/* 290- */
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
/* 300- */
	-1,-1,-1,
	SC_COMA,
	-1,-1,
	SC_LULLABY,
	SC_RICHMANKIM,
	SC_ETERNALCHAOS,
	SC_DRUMBATTLE,
/* 310- */
	SC_NIBELUNGEN,
	SC_ROKISWEIL,
	SC_INTOABYSS,
	SC_SIEGFRIED,
	-1,-1,-1,
	-1,
	-1,
	SC_WHISTLE,
/* 320- */
	SC_ASSNCROS,
	SC_POEMBRAGI,
	SC_APPLEIDUN,
	-1,-1,
	SC_UGLYDANCE,
	-1,
	SC_HUMMING,
	SC_DONTFORGETME,
	SC_FORTUNE,
/* 330- */
	SC_SERVICE4U,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,
/* 340- */
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
/* 350- */
	-1,-1,-1,-1,-1,
	SC_AURABLADE,
	SC_PARRYING,
	SC_CONCENTRATION,
	SC_TENSIONRELAX,
	SC_BERSERK,
/* 360- */
	SC_BERSERK,
	SC_ASSUMPTIO,
	SC_BASILICA,
	-1,-1,-1,
	SC_MAGICPOWER,
	-1,
	SC_SACRIFICE,
	SC_GOSPEL,
/* 370- */
	-1,-1,-1,-1,-1,-1,-1,-1,
	SC_EDP,
	-1,
/* 380- */
	SC_TRUESIGHT,
	-1,-1,
	SC_WINDWALK,
	SC_MELTDOWN,
	-1,-1,
	SC_CARTBOOST,
	-1,
	SC_CHASEWALK,
/* 390- */
	SC_REJECTSWORD,
	-1,-1,-1,-1,
	SC_MOONLIT,
	SC_MARIONETTE,
	-1,
	SC_BLEEDING,
	SC_JOINTBEAT,
/* 400- */
	-1,-1,
	SC_MINDBREAKER,
	SC_MEMORIZE,
	SC_FOGWALL,
	SC_SPIDERWEB,
	-1,-1,-1,-1,-1,
/* 410- */
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
/* 480- */
	-1,
	SC_DOUBLECASTING,
	-1,
	SC_GRAVITATION,
	-1,
	SC_MAXOVERTHRUST,
	-1,
	-1,-1,-1,
};

int StatusIconChangeTable[SC_MAX];

static int refinebonus[5][3];
int percentrefinery[5][10];
static int max_weight_base[MAX_PC_CLASS];
static int hp_coefficient[MAX_PC_CLASS];
static int hp_coefficient2[MAX_PC_CLASS];
static int hp_sigma_val[MAX_PC_CLASS][MAX_LEVEL];
static int sp_coefficient[MAX_PC_CLASS];
static int aspd_base[MAX_PC_CLASS][20];
static char job_bonus[3][MAX_PC_CLASS][MAX_LEVEL];

static int atkmods[3][20];

int inv_index;

void initStatusIconChangeTable() {
	int i;
	for (i = 0; i < SC_MAX; i++)
		StatusIconChangeTable[i] = SI_BLANK;

	StatusIconChangeTable[SC_PROVOKE] = SI_PROVOKE;
	StatusIconChangeTable[SC_ENDURE] = SI_ENDURE;
	StatusIconChangeTable[SC_TWOHANDQUICKEN] = SI_TWOHANDQUICKEN;
	StatusIconChangeTable[SC_CONCENTRATE] = SI_CONCENTRATE;
	StatusIconChangeTable[SC_HIDING] = SI_HIDING;
	StatusIconChangeTable[SC_CLOAKING] = SI_CLOAKING;
	StatusIconChangeTable[SC_ENCPOISON] = SI_ENCPOISON;
	StatusIconChangeTable[SC_POISONREACT] = SI_POISONREACT;
	StatusIconChangeTable[SC_QUAGMIRE] = SI_QUAGMIRE;
	StatusIconChangeTable[SC_ANGELUS] = SI_ANGELUS;
	StatusIconChangeTable[SC_BLESSING] = SI_BLESSING;
	StatusIconChangeTable[SC_SIGNUMCRUCIS] = SI_SIGNUMCRUCIS;
	StatusIconChangeTable[SC_INCREASEAGI] = SI_INCREASEAGI;
	StatusIconChangeTable[SC_DECREASEAGI] = SI_DECREASEAGI;
	StatusIconChangeTable[SC_SLOWPOISON] = SI_SLOWPOISON;
	StatusIconChangeTable[SC_IMPOSITIO] = SI_IMPOSITIO;
	StatusIconChangeTable[SC_SUFFRAGIUM] = SI_SUFFRAGIUM;
	StatusIconChangeTable[SC_ASPERSIO] = SI_ASPERSIO;
	StatusIconChangeTable[SC_BENEDICTIO] = SI_BENEDICTIO;
	StatusIconChangeTable[SC_KYRIE] = SI_KYRIE;
	StatusIconChangeTable[SC_MAGNIFICAT] = SI_MAGNIFICAT;
	StatusIconChangeTable[SC_GLORIA] = SI_GLORIA;
	StatusIconChangeTable[SC_AETERNA] = SI_AETERNA;
	StatusIconChangeTable[SC_ADRENALINE] = SI_ADRENALINE;
	StatusIconChangeTable[SC_WEAPONPERFECTION]	= SI_WEAPONPERFECTION;
	StatusIconChangeTable[SC_OVERTHRUST] = SI_OVERTHRUST;
	StatusIconChangeTable[SC_MAXIMIZEPOWER] = SI_MAXIMIZEPOWER;
	StatusIconChangeTable[SC_RIDING] = SI_RIDING;
	StatusIconChangeTable[SC_FALCON] = SI_FALCON;
	StatusIconChangeTable[SC_TRICKDEAD] = SI_TRICKDEAD;
	StatusIconChangeTable[SC_LOUD] = SI_LOUD;
	StatusIconChangeTable[SC_ENERGYCOAT] = SI_ENERGYCOAT;
	StatusIconChangeTable[SC_BROKENARMOR] = SI_BROKENARMOR;
	StatusIconChangeTable[SC_BROKENWEAPON] = SI_BROKENWEAPON;
	StatusIconChangeTable[SC_HALLUCINATION] = SI_HALLUCINATION;
	StatusIconChangeTable[SC_WEIGHT50] = SI_WEIGHT50;
	StatusIconChangeTable[SC_WEIGHT90] = SI_WEIGHT90;
	StatusIconChangeTable[SC_ANKLE] = SI_ANKLE;
	StatusIconChangeTable[SC_STRIPWEAPON] = SI_STRIPWEAPON;
	StatusIconChangeTable[SC_STRIPSHIELD] = SI_STRIPSHIELD;
	StatusIconChangeTable[SC_STRIPARMOR] = SI_STRIPARMOR;
	StatusIconChangeTable[SC_STRIPHELM] = SI_STRIPHELM;
	StatusIconChangeTable[SC_CP_WEAPON] = SI_CP_WEAPON;
	StatusIconChangeTable[SC_CP_SHIELD] = SI_CP_SHIELD;
	StatusIconChangeTable[SC_CP_ARMOR] = SI_CP_ARMOR;
	StatusIconChangeTable[SC_CP_HELM] = SI_CP_HELM;
	StatusIconChangeTable[SC_AUTOGUARD] = SI_AUTOGUARD;
	StatusIconChangeTable[SC_REFLECTSHIELD] = SI_REFLECTSHIELD;
	StatusIconChangeTable[SC_PROVIDENCE] = SI_PROVIDENCE;
	StatusIconChangeTable[SC_DEFENDER] = SI_DEFENDER;
	StatusIconChangeTable[SC_AUTOSPELL] = SI_AUTOSPELL;
	StatusIconChangeTable[SC_SIGHTTRASHER] = SI_SIGHTTRASHER;
	StatusIconChangeTable[SC_AUTOBERSERK] = SI_AUTOBERSERK;
	StatusIconChangeTable[SC_SPEARSQUICKEN] = SI_SPEARQUICKEN;
	StatusIconChangeTable[SC_AUTOCOUNTER] = SI_AUTOCOUNTER;
	StatusIconChangeTable[SC_SIGHT] = SI_SIGHT;
	StatusIconChangeTable[SC_SAFETYWALL] = SI_SAFETYWALL;
	StatusIconChangeTable[SC_RUWACH] = SI_RUWACH;
	StatusIconChangeTable[SC_PNEUMA] = SI_PNEUMA;
	StatusIconChangeTable[SC_STONE] = SI_STONE;
	StatusIconChangeTable[SC_FREEZE] = SI_FREEZE;
	StatusIconChangeTable[SC_STAN] = SI_STAN;
	StatusIconChangeTable[SC_SLEEP] = SI_SLEEP;
	StatusIconChangeTable[SC_POISON] = SI_POISON;
	StatusIconChangeTable[SC_CURSE] = SI_CURSE;
	StatusIconChangeTable[SC_SILENCE] = SI_SILENCE;
	StatusIconChangeTable[SC_CONFUSION] = SI_CONFUSION;
	StatusIconChangeTable[SC_BLIND] = SI_BLIND;
	StatusIconChangeTable[SC_BLEEDING] = SI_BLEEDING;
	StatusIconChangeTable[SC_DPOISON] = SI_DPOISON;
	StatusIconChangeTable[SC_EXPLOSIONSPIRITS] = SI_EXPLOSIONSPIRITS;
	StatusIconChangeTable[SC_BLADESTOP_WAIT] = SI_BLADESTOP_WAIT;
	StatusIconChangeTable[SC_BLADESTOP] = SI_BLADESTOP;
	StatusIconChangeTable[SC_FLAMELAUNCHER] = SI_FLAMELAUNCHER;
	StatusIconChangeTable[SC_FROSTWEAPON] = SI_FROSTWEAPON;
	StatusIconChangeTable[SC_LIGHTNINGLOADER] = SI_LIGHTNINGLOADER;
	StatusIconChangeTable[SC_SEISMICWEAPON] = SI_SEISMICWEAPON;
	StatusIconChangeTable[SC_VOLCANO] = SI_VOLCANO;
	StatusIconChangeTable[SC_DELUGE] = SI_DELUGE;
	StatusIconChangeTable[SC_VIOLENTGALE] = SI_VIOLENTGALE;
	StatusIconChangeTable[SC_NOCHAT] = SI_NOCHAT;
	StatusIconChangeTable[SC_AURABLADE] = SI_AURABLADE;
	StatusIconChangeTable[SC_PARRYING] = SI_PARRYING;
	StatusIconChangeTable[SC_CONCENTRATION] = SI_CONCENTRATION;
	StatusIconChangeTable[SC_TENSIONRELAX] = SI_TENSIONRELAX;
	StatusIconChangeTable[SC_BERSERK] = SI_BERSERK;
	StatusIconChangeTable[SC_FURY] = SI_FURY;
	StatusIconChangeTable[SC_GOSPEL] = SI_GOSPEL;
	StatusIconChangeTable[SC_ASSUMPTIO] = SI_ASSUMPTIO;
	StatusIconChangeTable[SC_GUILDAURA] = SI_GUILDAURA;
	StatusIconChangeTable[SC_MAGICPOWER] = SI_MAGICPOWER;
	StatusIconChangeTable[SC_EDP] = SI_EDP;
	StatusIconChangeTable[SC_TRUESIGHT] = SI_TRUESIGHT;
	StatusIconChangeTable[SC_WINDWALK] = SI_WINDWALK;
	StatusIconChangeTable[SC_MELTDOWN] = SI_MELTDOWN;
	StatusIconChangeTable[SC_CARTBOOST] = SI_CARTBOOST;
	StatusIconChangeTable[SC_CHASEWALK] = SI_CHASEWALK;
	StatusIconChangeTable[SC_REJECTSWORD] = SI_REJECTSWORD;
	StatusIconChangeTable[SC_MARIONETTE] = SI_MARIONETTE;
	StatusIconChangeTable[SC_MARIONETTE2] = SI_MARIONETTE2;
	StatusIconChangeTable[SC_MOONLIT] = SI_MOONLIT;
	StatusIconChangeTable[SC_JOINTBEAT] = SI_JOINTBEAT;
	StatusIconChangeTable[SC_MINDBREAKER] = SI_MINDBREAKER;
	StatusIconChangeTable[SC_MEMORIZE] = SI_MEMORIZE;
	StatusIconChangeTable[SC_FOGWALL] = SI_FOGWALL;
	StatusIconChangeTable[SC_SPIDERWEB] = SI_SPIDERWEB;
	StatusIconChangeTable[SC_DEVOTION] = SI_DEVOTION;
	StatusIconChangeTable[SC_SACRIFICE] = SI_SACRIFICE;
	StatusIconChangeTable[SC_STEELBODY] = SI_STEELBODY;
	StatusIconChangeTable[SC_ORCISH] = SI_WIGGLE;
	StatusIconChangeTable[SC_READYSTORM] = SI_READYSTORM;
	StatusIconChangeTable[SC_READYDOWN] = SI_READYDOWN;
	StatusIconChangeTable[SC_READYTURN] = SI_READYTURN;
	StatusIconChangeTable[SC_READYCOUNTER] = SI_READYCOUNTER;
	StatusIconChangeTable[SC_DODGE] = SI_DODGE;
	StatusIconChangeTable[SC_JUMPKICK] = SI_JUMPKICK;
	StatusIconChangeTable[SC_RUN] = SI_RUN;
	StatusIconChangeTable[SC_ADRENALINE2] = SI_ADRENALINE2;
	StatusIconChangeTable[SC_NIGHT] = SI_NIGHT;
	StatusIconChangeTable[SC_KAIZEL] = SI_KAIZEL;
	StatusIconChangeTable[SC_KAAHI] = SI_KAAHI;
	StatusIconChangeTable[SC_KAUPE] = SI_KAUPE;
	StatusIconChangeTable[SC_ONEHAND] = SI_ONEHAND;
	StatusIconChangeTable[SC_PRESERVE] = SI_PRESERVE;
	StatusIconChangeTable[SC_BATTLEORDERS] = SI_BATTLEORDERS;
	StatusIconChangeTable[SC_REGENERATION] = SI_REGENERATION;
	StatusIconChangeTable[SC_DOUBLECASTING] = SI_DOUBLECASTING;
	StatusIconChangeTable[SC_GRAVITATION] = SI_GRAVITATION;
	StatusIconChangeTable[SC_MAXOVERTHRUST] = SI_MAXOVERTHRUST;
	StatusIconChangeTable[SC_LONGING] = SI_LONGING;
	StatusIconChangeTable[SC_HERMODE] = SI_HERMODE;
	StatusIconChangeTable[SC_SHRINK] = SI_SHRINK;
	StatusIconChangeTable[SC_SIGHTBLASTER] = SI_SIGHTBLASTER;
	StatusIconChangeTable[SC_WINKCHARM] = SI_WINKCHARM;
	StatusIconChangeTable[SC_CLOSECONFINE] = SI_CLOSECONFINE;
	StatusIconChangeTable[SC_CLOSECONFINE2] = SI_CLOSECONFINE2;
	StatusIconChangeTable[SC_SPEEDUP0] = SI_SPEEDUP0;
	StatusIconChangeTable[SC_SPEEDUP1] = SI_SPEEDUP1;
	StatusIconChangeTable[SC_SPEEDPOTION0] = SI_SPEEDPOTION0;
	StatusIconChangeTable[SC_SPEEDPOTION1] = SI_SPEEDPOTION1;
	StatusIconChangeTable[SC_SPEEDPOTION2] = SI_SPEEDPOTION2;
	StatusIconChangeTable[SC_SPEEDPOTION3] = SI_SPEEDPOTION3;
}

/*==========================================
 * Get Refine Bonus
 *------------------------------------------
 */
int status_getrefinebonus(int lv, int type) {
	if (lv >= 0 && lv < 5 && type >= 0 && type < 3)
		return refinebonus[lv][type];

	return 0;
}

/*==========================================
 * Status Percent Refinery
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
 * Status Calc PC
 *------------------------------------------
 */
int status_calc_pc(struct map_session_data* sd, int first)
{
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
	memset(sd->monster_drop_race,0,sizeof(sd->monster_drop_race));
	memset(sd->monster_drop_itemrate,0,sizeof(sd->monster_drop_itemrate));
	sd->speed_add_rate = sd->aspd_add_rate = 100;
	sd->double_add_rate = sd->perfect_hit_add = sd->get_zeny_add_num = 0;
	sd->splash_range = sd->splash_add_range = 0;
	sd->hp_drain_rate = sd->hp_drain_per = sd->sp_drain_rate = sd->sp_drain_per = 0;
	sd->hp_drain_rate_ = sd->hp_drain_per_ = sd->sp_drain_rate_ = sd->sp_drain_per_ = 0;
	sd->short_weapon_damage_return = sd->long_weapon_damage_return = 0;
	sd->magic_damage_return = 0;
	sd->random_attack_increase_add = sd->random_attack_increase_per = 0;
	sd->hp_drain_value = sd->hp_drain_value_ = sd->sp_drain_value = sd->sp_drain_value_ = 0;
	sd->unbreakable_equip = 0;

	sd->break_weapon_rate = sd->break_armor_rate = 0;
	sd->add_steal_rate = 0;
	sd->crit_atk_rate = 0;
	sd->no_regen = 0;
	sd->unstripable_equip = 0;
	memset(sd->critaddrace, 0, sizeof(sd->critaddrace));
	memset(sd->addeff3, 0, sizeof(sd->addeff3));
	memset(sd->skillatk, 0, sizeof(sd->skillatk));
	memset(sd->autospell, 0, sizeof(sd->autospell));
	memset(sd->autospell2, 0, sizeof(sd->autospell2));
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
	memset(sd->exp_addrace, 0, sizeof(sd->exp_addrace));
	memset(sd->sp_gain_race, 0, sizeof(sd->sp_gain_race));
	memset(sd->itemhealrate,0,sizeof(sd->itemhealrate));	// Reset itemhealrate to prevent stacking
	sd->classchange = 0;

	if (!sd->disguiseflag && sd->disguise) {
		sd->disguise = 0;
		clif_changelook(&sd->bl, LOOK_WEAPON, sd->status.weapon);
		clif_changelook(&sd->bl, LOOK_SHIELD, sd->status.shield);
		clif_changelook(&sd->bl, LOOK_HEAD_BOTTOM, sd->status.head_bottom);
		clif_changelook(&sd->bl, LOOK_HEAD_TOP, sd->status.head_top);
		clif_changelook(&sd->bl, LOOK_HEAD_MID, sd->status.head_mid);
		clif_clearchar(&sd->bl, 9);
		pc_setpos(sd, sd->mapname, sd->bl.x, sd->bl.y, 3);
	}

	if (sd->status.guild_id > 0) {
		struct guild *g = guild_search(sd->status.guild_id);
		if (g && strcmp(sd->status.name, g->master) == 0)
			sd->state.gmaster_flag = g;
		else
			sd->state.gmaster_flag = NULL;
	} else
		sd->state.gmaster_flag = NULL;

	for(i = 0; i < 10; i++) {
		inv_index = idx = sd->equip_index[i];
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
					for(j=0;j<sd->inventory_data[idx]->slot;j++){
						int c=sd->status.inventory[idx].card[j];
						if(c>0){
							if(i == 8 && sd->status.inventory[idx].equip == 0x20)
								sd->state.lr_flag = 1;
							run_script(itemdb_usescript(c),0,sd->bl.id,0);
							sd->state.lr_flag = 0;
						}
					}
				}
			}
			else if(sd->inventory_data[idx]->type==5){ // 防具
				if(sd->status.inventory[idx].card[0]!=0x00ff && sd->status.inventory[idx].card[0]!=0x00fe && sd->status.inventory[idx].card[0]!=(short)0xff00) {
					int j;
					for(j=0;j<sd->inventory_data[idx]->slot;j++){	// カード
						int c=sd->status.inventory[idx].card[j];
						if(c>0)
							run_script(itemdb_usescript(c),0,sd->bl.id,0);
					}
				}
			}
		}
	}
	wele = sd->atk_ele;
	wele_ = sd->atk_ele_;
	def_ele = sd->def_ele;

	if(sd->status.pet_id > 0)
	{
		struct pet_data *pd = sd->pd;

		if(pd && battle_config.pet_status_support == 1 && (battle_config.pet_equip_required == 0 || (battle_config.pet_equip_required == 1 && pd->equip > 0)))
		{
			if(sd->petDB && sd->pet.intimate > 0 && pd->state.skillbonus == 1)
				pc_bonus(sd, pd->skillbonustype, pd->skillbonusval);

			pele = sd->atk_ele;
			pdef_ele = sd->def_ele;
			sd->atk_ele = 0;
			sd->def_ele = 0;
		}
	}

	memcpy(sd->paramcard,sd->parame,sizeof(sd->paramcard));

	for(i=0;i<10;i++) {
		inv_index = idx = sd->equip_index[i];
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
					//二刀流用データ入力
					sd->watk_ += sd->inventory_data[idx]->atk;
					sd->watk_2 = (r=sd->status.inventory[idx].refine)*	// 精錬攻撃力
						refinebonus[wlv][0];
					if( (r-=refinebonus[wlv][2])>0 )	// 過剰精錬ボーナス
						sd->overrefine_ = r*refinebonus[wlv][1];

					if(sd->status.inventory[idx].card[0]==0x00ff){	// 製造武器
						sd->star_ = (sd->status.inventory[idx].card[1]>>8);	// 星のかけら
						wele_= (sd->status.inventory[idx].card[1]&0x0f);	// 属 性
					}
					sd->attackrange_ += sd->inventory_data[idx]->range;
					sd->state.lr_flag = 1;
					run_script(sd->inventory_data[idx]->script,0,sd->bl.id,0);
					sd->state.lr_flag = 0;
				}
				else {	//二刀流武器以外
					sd->watk += sd->inventory_data[idx]->atk;
					sd->watk2 += (r = sd->status.inventory[idx].refine) *	// 精錬攻撃力
						refinebonus[wlv][0];
					if( (r-=refinebonus[wlv][2])>0 )	// 過剰精錬ボーナス
						sd->overrefine += r*refinebonus[wlv][1];

					if(sd->status.inventory[idx].card[0]==0x00ff){	// 製造武器
						sd->star += (sd->status.inventory[idx].card[1]>>8);	// 星のかけら
						wele = (sd->status.inventory[idx].card[1]&0x0f);	// 属 性
					}
					sd->attackrange += sd->inventory_data[idx]->range;
					run_script(sd->inventory_data[idx]->script,0,sd->bl.id,0);
				}
			}
			else if (sd->inventory_data[idx]->type == 5) {
				sd->watk += sd->inventory_data[idx]->atk;
				refinedef += sd->status.inventory[idx].refine * refinebonus[0][0];
				run_script(sd->inventory_data[idx]->script, 0, sd->bl.id, 0);
			}
		}
	}

	if(sd->equip_index[10] >= 0){ // 矢
		idx = sd->equip_index[10];
		if(sd->inventory_data[idx]){		//まだ属性が入っていない
			sd->state.lr_flag = 2;
			run_script(sd->inventory_data[idx]->script,0,sd->bl.id,0);
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
	sd->splash_range += sd->splash_add_range;
	if(sd->speed_add_rate != 100)
		sd->speed_rate += sd->speed_add_rate - 100;
	if(sd->aspd_add_rate != 100)
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

	if ((skill = pc_checkskill(sd, MC_INCCARRY)) > 0) // skill can be used with an item now, thanks to orn [Valaris]
		sd->max_weight += skill * 2000; // kRO 14/12/04 Patch: Every level now increases capacity of maximum weight by 200 instead of 100. [Aalye]

	if ((skill = pc_checkskill(sd, AC_OWL)) > 0) // ふくろうの目
		sd->paramb[4] += skill;

	if ((skill = pc_checkskill(sd, BS_HILTBINDING)) > 0) { // Hilt binding gives +1 str +4 atk
		sd->paramb[0] ++;
		sd->base_atk += 4;
	}

	if ((skill = pc_checkskill(sd, SA_DRAGONOLOGY)) > 0)
		sd->paramb[3] += (skill % 2 == 0) ? skill / 2 : (skill + 1) / 2;

	if (sd->sc_count) {
		if (sd->sc_data[SC_INCSTR].timer != -1)
			sd->paramb[0] += sd->sc_data[SC_INCSTR].val1;
		if (sd->sc_data[SC_CONCENTRATE].timer != -1 && sd->sc_data[SC_QUAGMIRE].timer == -1) { // 集中力向上
			sd->paramb[1] += (sd->status.agi + sd->paramb[1] + sd->parame[1] - sd->paramcard[1]) * (2 + sd->sc_data[SC_CONCENTRATE].val1) / 100;
			sd->paramb[4] += (sd->status.dex + sd->paramb[4] + sd->parame[4] - sd->paramcard[4]) * (2 + sd->sc_data[SC_CONCENTRATE].val1) / 100;
		}
		if (sd->sc_data[SC_INCREASEAGI].timer != -1 && sd->sc_data[SC_QUAGMIRE].timer == -1 && sd->sc_data[SC_DONTFORGETME].timer == -1) { // 速度増加
			sd->paramb[1] += 2 + sd->sc_data[SC_INCREASEAGI].val1;
			sd->speed -= sd->speed * 25 / 100;
		}
		if (sd->sc_data[SC_DECREASEAGI].timer != -1) {	// 速度減少(agiはbattle.cで)
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
		if (sd->sc_data[SC_SLOWDOWN].timer!=-1)
			sd->speed = sd->speed * 150 / 100;
		if (sd->sc_data[SC_SPEEDUP0].timer!=-1)
			sd->speed -= sd->speed * 25 / 100;
		if (sd->sc_data[SC_BLESSING].timer!=-1)
		{
			sd->paramb[0] += sd->sc_data[SC_BLESSING].val1;
			sd->paramb[3] += sd->sc_data[SC_BLESSING].val1;
			sd->paramb[4] += sd->sc_data[SC_BLESSING].val1;
		}
		if(sd->sc_data[SC_GLORIA].timer!=-1)
			sd->paramb[5]+= 30;
		if(sd->sc_data[SC_LOUD].timer!=-1 && sd->sc_data[SC_QUAGMIRE].timer == -1)
			sd->paramb[0]+= 4;
		if(sd->sc_data[SC_QUAGMIRE].timer!=-1)
		{
			sd->paramb[1]-= sd->sc_data[SC_QUAGMIRE].val1*5;
			sd->paramb[4]-= sd->sc_data[SC_QUAGMIRE].val1*5;
			sd->speed = sd->speed * 3 / 2;
		}
		if(sd->sc_data[SC_TRUESIGHT].timer!=-1)
		{
			sd->paramb[0]+= 5;
			sd->paramb[1]+= 5;
			sd->paramb[2]+= 5;
			sd->paramb[3]+= 5;
			sd->paramb[4]+= 5;
			sd->paramb[5]+= 5;
		}
		if (sd->sc_data[SC_MARIONETTE].timer != -1)
		{
//			struct map_session_data *psd = map_id2sd(sd->sc_data[SC_MARIONETTE2].val3);
//			if(psd)
//			{
			sd->paramb[0]-= sd->status.str / 2;
			sd->paramb[1]-= sd->status.agi / 2;
			sd->paramb[2]-= sd->status.vit / 2;
			sd->paramb[3]-= sd->status.int_ / 2;
			sd->paramb[4]-= sd->status.dex / 2;
			sd->paramb[5]-= sd->status.luk / 2;
//			}
		} else if (sd->sc_data[SC_MARIONETTE2].timer != -1)
		{
			struct map_session_data *psd = (struct map_session_data *)map_id2bl(sd->sc_data[SC_MARIONETTE2].val3);
			if(psd)
			{
				sd->paramb[0] += sd->status.str + psd->status.str / 2 > 99 ? 99-sd->status.str : psd->status.str / 2;
				sd->paramb[1] += sd->status.agi + psd->status.agi / 2 > 99 ? 99-sd->status.agi : psd->status.agi / 2;
				sd->paramb[2] += sd->status.vit + psd->status.vit / 2 > 99 ? 99-sd->status.vit : psd->status.vit / 2;
				sd->paramb[3] += sd->status.int_ + psd->status.int_ / 2 > 99 ? 99-sd->status.int_ : psd->status.int_ / 2;
				sd->paramb[4] += sd->status.dex + psd->status.dex / 2 > 99 ? 99-sd->status.dex : psd->status.dex / 2;
				sd->paramb[5] += sd->status.luk + psd->status.luk / 2 > 99 ? 99-sd->status.luk : psd->status.luk / 2;
			}
		}
		if (sd->sc_data[SC_GOSPEL].timer != -1 && sd->sc_data[SC_GOSPEL].val4 == BCT_PARTY) {
			if (sd->sc_data[SC_GOSPEL].val3 == 6) {
				sd->paramb[0] += 2;
				sd->paramb[1] += 2;
				sd->paramb[2] += 2;
				sd->paramb[3] += 2;
				sd->paramb[4] += 2;
				sd->paramb[5] += 2;
			}
		}

		if (sd->sc_data[SC_BATTLEORDERS].timer != -1) {
			sd->paramb[0] += 5;
			sd->paramb[3] += 5;
			sd->paramb[4] += 5;
		}
		if (sd->sc_data[SC_GUILDAURA].timer != -1) {
			if (sd->sc_data[SC_GUILDAURA].val4 & 1 << 0)
				sd->paramb[0] += 2;
			if (sd->sc_data[SC_GUILDAURA].val4 & 1 << 1)
				sd->paramb[2] += 2;
			if (sd->sc_data[SC_GUILDAURA].val4 & 1 << 2)
				sd->paramb[1] += 2;
			if (sd->sc_data[SC_GUILDAURA].val4 & 1 << 3)
				sd->paramb[4] += 2;
		}
	}

	if (s_class.job == 23 && sd->die_counter == 0 && sd->status.base_level >= 99 && !pc_readglobalreg(sd,"SN_EXSTAT"))
		pc_setglobalreg(sd,"SN_FINALBONUS",1);

	if (s_class.job == 23 && pc_readglobalreg(sd,"SN_EXSTAT") && sd->status.job_level >= 99)
	{
		sd->paramb[0] += 15;
		sd->paramb[1] += 15;
		sd->paramb[2] += 15;
		sd->paramb[3] += 15;
		sd->paramb[4] += 15;
		sd->paramb[5] += 15;
	}
	sd->paramc[0]=sd->status.str+sd->paramb[0]+sd->parame[0];
	sd->paramc[1]=sd->status.agi+sd->paramb[1]+sd->parame[1];
	sd->paramc[2]=sd->status.vit+sd->paramb[2]+sd->parame[2];
	sd->paramc[3]=sd->status.int_+sd->paramb[3]+sd->parame[3];
	sd->paramc[4]=sd->status.dex+sd->paramb[4]+sd->parame[4];
	sd->paramc[5]=sd->status.luk+sd->paramb[5]+sd->parame[5];
	for(i=0;i<6;i++)
		if(sd->paramc[i] < 0) sd->paramc[i] = 0;

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

	// 二刀流 ASPD 修正
	if (sd->status.weapon <= 22)
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

	//攻撃速度増加

	if ((skill = pc_checkskill(sd, AC_VULTURE)) > 0) { // ワシの目
		sd->hit += skill;
		if (sd->status.weapon == 11)
			sd->attackrange += skill;
	}

	if( (skill=pc_checkskill(sd,BS_WEAPONRESEARCH))>0)	// 武器研究の命中率増加
		sd->hit += skill*2;
	if(sd->status.option & 2 && (skill = pc_checkskill(sd,RG_TUNNELDRIVE))>0 )	// トンネルドライブ	// トンネルドライブ
		sd->speed += (1.2*DEFAULT_WALK_SPEED - skill*9);
	if (pc_iscarton(sd) && (skill=pc_checkskill(sd,MC_PUSHCART))>0)	// カートによる速度低下
		sd->speed += (10-skill) * (DEFAULT_WALK_SPEED * 0.1);
	else if (pc_isriding(sd)) {	// ペコペコ乗りによる速度増加
		sd->speed -= (0.25 * DEFAULT_WALK_SPEED);
		sd->max_weight += 10000;
	}

	if((skill=pc_checkskill(sd,CR_TRUST))>0) { // フェイス
		sd->status.max_hp += skill*200;
		sd->subele[6] += skill*5;
	}
	if ((skill=pc_checkskill(sd,BS_SKINTEMPER)) > 0) {
		sd->subele[0] += skill;
		sd->subele[3] += skill*5;
	}
	if ((skill = pc_checkskill(sd,SA_ADVANCEDBOOK)) > 0)
		aspd_rate -= skill * 0.5;

	bl=sd->status.base_level;

	sd->status.max_hp += (3500 + bl*hp_coefficient2[s_class.job] + hp_sigma_val[s_class.job][(bl > 0)? bl-1:0])/100 * (100 + sd->paramc[2])/100 + (sd->parame[2] - sd->paramcard[2]);
	if (s_class.upper==1) // [MouseJstr]
		sd->status.max_hp = sd->status.max_hp * 130/100;
	else if (s_class.upper==2)
		sd->status.max_hp = sd->status.max_hp * 70/100;
	if(sd->hprate!=100)
		sd->status.max_hp = sd->status.max_hp*sd->hprate/100;

	if(sd->sc_count && sd->sc_data[SC_BERSERK].timer!=-1)
	{
		sd->status.max_hp = sd->status.max_hp * 3;
		if(sd->status.max_hp > battle_config.max_hp)
			sd->status.max_hp = battle_config.max_hp;
		if(sd->status.hp > battle_config.max_hp)
			sd->status.hp = battle_config.max_hp;
	}
	if(s_class.job == 23 && sd->status.base_level >= 99)
		sd->status.max_hp = sd->status.max_hp + 2000;

	if (sd->status.max_hp > battle_config.max_hp)
		sd->status.max_hp = battle_config.max_hp;
	if (sd->status.max_hp <= 0) sd->status.max_hp = 1;

	sd->status.max_sp += ((sp_coefficient[s_class.job] * bl) + 1000)/100 * (100 + sd->paramc[3])/100 + (sd->parame[3] - sd->paramcard[3]);
	if (s_class.upper==1) // [MouseJstr]
		sd->status.max_sp = sd->status.max_sp * 130/100;
	else if (s_class.upper==2)
		sd->status.max_sp = sd->status.max_sp * 70/100;
	if(sd->sprate!=100)
		sd->status.max_sp = sd->status.max_sp*sd->sprate/100;

	if((skill=pc_checkskill(sd,HP_MEDITATIO))>0) // メディテイティオ
		sd->status.max_sp += sd->status.max_sp*skill/100;
	if((skill=pc_checkskill(sd,HW_SOULDRAIN))>0) /* ソウルドレイン */
		sd->status.max_sp += sd->status.max_sp*2*skill/100;

	if(sd->status.max_sp < 0 || sd->status.max_sp > battle_config.max_sp)
		sd->status.max_sp = battle_config.max_sp;

	sd->nhealhp = 1 + (sd->paramc[2]/5) + (sd->status.max_hp/200);
	if((skill=pc_checkskill(sd,SM_RECOVERY)) > 0) {	/* HP回復力向上 */
		sd->nshealhp = skill*5 + (sd->status.max_hp*skill/500);
		if(sd->nshealhp > 0x7fff) sd->nshealhp = 0x7fff;
	}

	sd->nhealsp = 1 + (sd->paramc[3]/6) + (sd->status.max_sp/100);
	if(sd->paramc[3] >= 120)
		sd->nhealsp += ((sd->paramc[3] - 120) >> 1) + 4;
	if((skill=pc_checkskill(sd,MG_SRECOVERY)) > 0)
	{
		sd->nshealsp = skill*3 + (sd->status.max_sp*skill/500);
		if(sd->nshealsp > 0x7fff) sd->nshealsp = 0x7fff;
	}

	if((skill = pc_checkskill(sd,MO_SPIRITSRECOVERY)) > 0)
	{
		sd->nsshealhp = skill * 4 + (sd->status.max_hp * skill / 500);
		if(sd->nsshealhp > 0x7fff)
			sd->nsshealhp = 0x7fff;
		if(sd->sc_data[SC_EXTREMITYFIST].timer == -1)
		{
			sd->nsshealsp = skill * 2 + (sd->status.max_sp * skill / 500);
			if(sd->nsshealsp > 0x7fff)
				sd->nsshealsp = 0x7fff;
		}
	}
	if(sd->hprecov_rate != 100) {
		sd->nhealhp = sd->nhealhp*sd->hprecov_rate/100;
		if(sd->nhealhp < 1) sd->nhealhp = 1;
	}
	if(sd->sprecov_rate != 100) {
		sd->nhealsp = sd->nhealsp*sd->sprecov_rate/100;
		if(sd->nhealsp < 1) sd->nhealsp = 1;
	}

	if((skill = pc_checkskill(sd,HP_MANARECHARGE)) > 0)
		sd->dsprate -= 4 * skill;

	if((skill = pc_checkskill(sd,SA_DRAGONOLOGY)) > 0){
		skill = skill*4;
		sd->addrace[9]+=skill;
		sd->addrace_[9]+=skill;
		sd->subrace[9]+=skill;
		sd->magic_addrace[9]+=skill;
		sd->magic_subrace[9]-=skill;
	}

	if( (skill=pc_checkskill(sd,TF_MISS))>0 ){
		if(sd->status.class==6||sd->status.class==4007 || sd->status.class==23){
			sd->flee += skill*3;
		}
		if(sd->status.class==12||sd->status.class==17||sd->status.class==4013||sd->status.class==4018)
			sd->flee += skill*4;
		if(sd->status.class==12||sd->status.class==4013)
			sd->speed -= sd->speed *(skill*1.5)/100;
	}
	if ((skill = pc_checkskill(sd, MO_DODGE)) > 0)	// 見切り
		sd->flee += (skill * 3) >> 1;

	if(sd->sc_count){
		if(sd->sc_data[SC_ANGELUS].timer != -1)
			sd->def2 = sd->def2*(110+5*sd->sc_data[SC_ANGELUS].val1)/100;
		if(sd->sc_data[SC_IMPOSITIO].timer != -1) {
			sd->watk += sd->sc_data[SC_IMPOSITIO].val1 * 5;
			idx = sd->equip_index[8];
			if(idx >= 0 && sd->inventory_data[idx] && sd->inventory_data[idx]->type == 4)
				sd->watk_ += sd->sc_data[SC_IMPOSITIO].val1 * 5;
		}
		if(sd->sc_data[SC_PROVOKE].timer != -1) {
// Old Formula
//			sd->def = sd->def *(100 - 6 * sd->sc_data[SC_PROVOKE].val1) / 100;
			sd->def -= sd->def * sd->sc_data[SC_PROVOKE].val4 / 100;
// Old Formula
//			sd->base_atk = sd->base_atk * (100 + 2 * sd->sc_data[SC_PROVOKE].val1) / 100;
			sd->base_atk += sd->base_atk * sd->sc_data[SC_PROVOKE].val3 / 100;
// Old Formula
//			sd->watk = sd->watk * (100 + 2 * sd->sc_data[SC_PROVOKE].val1) / 100;
			sd->watk += sd->watk * sd->sc_data[SC_PROVOKE].val3 / 100;
			idx = sd->equip_index[8];
			if(idx >= 0 && sd->inventory_data[idx] && sd->inventory_data[idx]->type == 4)
// Old Formula
//				sd->watk_ = sd->watk_*(100+2*sd->sc_data[SC_PROVOKE].val1)/100;
				sd->watk_ += sd->watk_ * sd->sc_data[SC_PROVOKE].val3 / 100;
		}
		if(sd->sc_data[SC_ENDURE].timer!=-1)
			sd->mdef += sd->sc_data[SC_ENDURE].val1;
		if(sd->sc_data[SC_MINDBREAKER].timer!=-1){	// プロボック
			sd->mdef2 = sd->mdef2*(100-6*sd->sc_data[SC_MINDBREAKER].val1)/100;
			sd->matk1 = sd->matk1*(100+2*sd->sc_data[SC_MINDBREAKER].val1)/100;
			sd->matk2 = sd->matk2*(100+2*sd->sc_data[SC_MINDBREAKER].val1)/100;
		}
		if (sd->sc_data[SC_POISON].timer != -1)
			sd->def2 = sd->def2 * 75 / 100;
		if (sd->sc_data[SC_CURSE].timer != -1)
		{
			sd->base_atk = sd->base_atk * 75 / 100;
			sd->watk = sd->watk * 75 / 100;
			idx = sd->equip_index[8];
			if (idx >= 0 && sd->inventory_data[idx] && sd->inventory_data[idx]->type == 4)
				sd->watk_ = sd->watk_ * 75 / 100;
		}
		if(sd->sc_data[SC_BLEEDING].timer != -1)
			sd->watk = sd->watk - (sd->watk * 25 / 100);
		if (sd->sc_data[SC_DRUMBATTLE].timer != -1){	// 戦太鼓の響き
			sd->watk += sd->sc_data[SC_DRUMBATTLE].val2;
			sd->def  += sd->sc_data[SC_DRUMBATTLE].val3;
			idx = sd->equip_index[8];
			if (idx >= 0 && sd->inventory_data[idx] && sd->inventory_data[idx]->type == 4)
				sd->watk_ += sd->sc_data[SC_DRUMBATTLE].val2;
		}
		if(sd->sc_data[SC_NIBELUNGEN].timer!=-1) {	// ニーベルングの指輪
			idx = sd->equip_index[9];
			if(idx >= 0 && sd->inventory_data[idx] && sd->inventory_data[idx]->wlv == 4)
				sd->watk2 += sd->sc_data[SC_NIBELUNGEN].val3;
			idx = sd->equip_index[8];
			if(idx >= 0 && sd->inventory_data[idx] && sd->inventory_data[idx]->wlv == 4)
				sd->watk_2 += sd->sc_data[SC_NIBELUNGEN].val3;
		}

		if(sd->sc_data[SC_VOLCANO].timer != -1 && sd->def_ele == 3)
		{
			sd->watk += sd->sc_data[SC_VIOLENTGALE].val3;
		}

		if(sd->sc_data[SC_SIGNUMCRUCIS].timer != -1)
			sd->def = sd->def * (100 - sd->sc_data[SC_SIGNUMCRUCIS].val2)/100;

		if(sd->sc_data[SC_ETERNALCHAOS].timer != -1)
			sd->def2 = 0;

		if(sd->sc_data[SC_CONCENTRATION].timer != -1)
		{
			sd->watk = sd->watk * (100 + 5 * sd->sc_data[SC_CONCENTRATION].val1) / 100;
			idx = sd->equip_index[8];
			if(idx >= 0 && sd->inventory_data[idx] && sd->inventory_data[idx]->type == 4)
				sd->watk_ = sd->watk * (100 + 5 * sd->sc_data[SC_CONCENTRATION].val1) / 100;
			sd->def = sd->def * (100 - 5 * sd->sc_data[SC_CONCENTRATION].val1) / 100;
		}

		if(sd->sc_data[SC_MAGICPOWER].timer!=-1)
		{
			sd->matk1 = sd->matk1 * (100 + 5 * sd->sc_data[SC_MAGICPOWER].val1) / 100;
			sd->matk2 = sd->matk2 * (100 + 5 * sd->sc_data[SC_MAGICPOWER].val1) / 100;
		}
		if(sd->sc_data[SC_ATKPOT].timer != -1)
			sd->watk += sd->sc_data[SC_ATKPOT].val1;
		if(sd->sc_data[SC_MATKPOT].timer != -1)
		{
			sd->matk1 += sd->sc_data[SC_MATKPOT].val1;
			sd->matk2 += sd->sc_data[SC_MATKPOT].val1;
		}

		// ATTACK SPEED MODIFIERS
		if(sd->sc_data[SC_TWOHANDQUICKEN].timer != -1 && sd->sc_data[SC_QUAGMIRE].timer == -1 && sd->sc_data[SC_DONTFORGETME].timer == -1)
			aspd_rate -= 30;
		if(sd->sc_data[SC_ADRENALINE].timer != -1 && sd->sc_data[SC_TWOHANDQUICKEN].timer == -1 && sd->sc_data[SC_QUAGMIRE].timer == -1 && sd->sc_data[SC_DONTFORGETME].timer == -1)
		{
			if(sd->sc_data[SC_ADRENALINE].val2 || !battle_config.party_skill_penalty)
				aspd_rate = aspd_rate - 30;
			else
				aspd_rate = aspd_rate - 25;
		}
		if(sd->sc_data[SC_SPEARSQUICKEN].timer != -1 && sd->sc_data[SC_ADRENALINE].timer == -1 && sd->sc_data[SC_TWOHANDQUICKEN].timer == -1 && sd->sc_data[SC_QUAGMIRE].timer == -1 && sd->sc_data[SC_DONTFORGETME].timer == -1)
			aspd_rate -= sd->sc_data[SC_SPEARSQUICKEN].val2;

		if(sd->sc_data[SC_ASSNCROS].timer!=-1 && sd->sc_data[SC_TWOHANDQUICKEN].timer == -1 && sd->sc_data[SC_ADRENALINE].timer == -1 && sd->sc_data[SC_SPEARSQUICKEN].timer == -1 && sd->sc_data[SC_DONTFORGETME].timer == -1)
				aspd_rate -= 5+sd->sc_data[SC_ASSNCROS].val1+sd->sc_data[SC_ASSNCROS].val2+sd->sc_data[SC_ASSNCROS].val3;

		if(sd->sc_data[SC_GRAVITATION].timer != -1)
			aspd_rate += sd->sc_data[SC_GRAVITATION].val2;

		if(sd->sc_data[SC_DONTFORGETME].timer!=-1)
		{
			aspd_rate += sd->sc_data[SC_DONTFORGETME].val1 * 3 + sd->sc_data[SC_DONTFORGETME].val2 + (sd->sc_data[SC_DONTFORGETME].val3 >> 16);
			sd->speed = sd->speed * (100 + sd->sc_data[SC_DONTFORGETME].val1 * 2 + sd->sc_data[SC_DONTFORGETME].val2 + (sd->sc_data[SC_DONTFORGETME].val3 & 0xffff)) / 100;
		}

		if (sd->sc_data[i=SC_SPEEDPOTION3].timer != -1 || sd->sc_data[i=SC_SPEEDPOTION2].timer != -1 || sd->sc_data[i=SC_SPEEDPOTION1].timer != -1 || sd->sc_data[i=SC_SPEEDPOTION0].timer != -1)
			aspd_rate -= sd->sc_data[i].val2;

		if(sd->sc_data[SC_BLEEDING].timer != -1)
			aspd_rate += 25;

		if (sd->sc_data[SC_WINDWALK].timer != -1 && sd->sc_data[SC_INCREASEAGI].timer == -1)
			sd->speed -= sd->speed * (sd->sc_data[SC_WINDWALK].val1 * 2) / 100;

		if (sd->sc_data[SC_CARTBOOST].timer != -1)
			sd->speed -= (DEFAULT_WALK_SPEED * 20) / 100;

		if (sd->sc_data[SC_BERSERK].timer != -1)
			sd->speed -= sd->speed * 25 / 100;

		if (sd->sc_data[SC_WEDDING].timer != -1)
			sd->speed = 2 * DEFAULT_WALK_SPEED;

		// HIT & FLEE MODIFIERS
		if(sd->sc_data[SC_WHISTLE].timer!=-1)
		{
			sd->flee += sd->flee * (sd->sc_data[SC_WHISTLE].val1
			          + sd->sc_data[SC_WHISTLE].val2 + (sd->sc_data[SC_WHISTLE].val3 >> 16)) / 100;
			sd->flee2+= (sd->sc_data[SC_WHISTLE].val1+sd->sc_data[SC_WHISTLE].val2+(sd->sc_data[SC_WHISTLE].val3&0xffff)) * 10;
		}
		if(sd->sc_data[SC_HUMMING].timer!=-1)
			sd->hit += (sd->sc_data[SC_HUMMING].val1*2+sd->sc_data[SC_HUMMING].val2
					+sd->sc_data[SC_HUMMING].val3) * sd->hit/100;
		if(sd->sc_data[SC_VIOLENTGALE].timer!=-1 && sd->def_ele==4)
			sd->flee += sd->flee*sd->sc_data[SC_VIOLENTGALE].val3/100;
		if(sd->sc_data[SC_BLIND].timer!=-1)
		{
			sd->hit -= sd->hit*25/100;
			sd->flee -= sd->flee*25/100;
		}
		if (sd->sc_data[SC_WINDWALK].timer != -1)
			sd->flee += sd->flee * (sd->sc_data[SC_WINDWALK].val2) / 100;
		if (sd->sc_data[SC_SPIDERWEB].timer != -1)
			sd->flee -= sd->flee * 50 / 100;
		if (sd->sc_data[SC_TRUESIGHT].timer != -1)
			sd->hit += sd->hit * (sd->sc_data[SC_TRUESIGHT].val1) * 3 / 100;	// New method to calculate damage bonus
		if (sd->sc_data[SC_CONCENTRATION].timer != -1)
			sd->hit += sd->hit * 10 * sd->sc_data[SC_CONCENTRATION].val1 / 100;

		// 耐性
		if(sd->sc_data[SC_SIEGFRIED].timer!=-1){  // 不死身のジークフリード
			sd->subele[1] += sd->sc_data[SC_SIEGFRIED].val2;	// 水
			sd->subele[2] += sd->sc_data[SC_SIEGFRIED].val2;	// 水
			sd->subele[3] += sd->sc_data[SC_SIEGFRIED].val2;	// 火
			sd->subele[4] += sd->sc_data[SC_SIEGFRIED].val2;	// 水
			sd->subele[5] += sd->sc_data[SC_SIEGFRIED].val2;	// 水
			sd->subele[6] += sd->sc_data[SC_SIEGFRIED].val2;	// 水
			sd->subele[7] += sd->sc_data[SC_SIEGFRIED].val2;	// 水
			sd->subele[8] += sd->sc_data[SC_SIEGFRIED].val2;	// 水
			sd->subele[9] += sd->sc_data[SC_SIEGFRIED].val2;	// 水
		}
		if(sd->sc_data[SC_PROVIDENCE].timer!=-1){	// プロヴィデンス
			sd->subele[6] += sd->sc_data[SC_PROVIDENCE].val2;	// 対 聖属性
			sd->subrace[6] += sd->sc_data[SC_PROVIDENCE].val2;	// 対 悪魔
		}

		// その他
		if(sd->sc_data[SC_APPLEIDUN].timer!=-1){	// イドゥンの林檎
			sd->status.max_hp += ((5 + sd->sc_data[SC_APPLEIDUN].val1 * 2 + ((sd->sc_data[SC_APPLEIDUN].val2 + 1) >> 1)
						+sd->sc_data[SC_APPLEIDUN].val3/10) * sd->status.max_hp)/100;
			if(sd->status.max_hp < 0 || sd->status.max_hp > battle_config.max_hp)
				sd->status.max_hp = battle_config.max_hp;
		}
		if(sd->sc_data[SC_DELUGE].timer!=-1 && sd->def_ele==1){	// デリュージ
			sd->status.max_hp += sd->status.max_hp*sd->sc_data[SC_DELUGE].val3/100;
			if(sd->status.max_hp < 0 || sd->status.max_hp > battle_config.max_hp)
				sd->status.max_hp = battle_config.max_hp;
		}
		if(sd->sc_data[SC_SERVICE4U].timer!=-1) {	// サービスフォーユー
			sd->status.max_sp += sd->status.max_sp*(10+sd->sc_data[SC_SERVICE4U].val1+sd->sc_data[SC_SERVICE4U].val2
						+sd->sc_data[SC_SERVICE4U].val3)/100;
			if(sd->status.max_sp < 0 || sd->status.max_sp > battle_config.max_sp)
				sd->status.max_sp = battle_config.max_sp;
			sd->dsprate-=(10+sd->sc_data[SC_SERVICE4U].val1*3+sd->sc_data[SC_SERVICE4U].val2
					+sd->sc_data[SC_SERVICE4U].val3);
			if(sd->dsprate<0)sd->dsprate=0;
		}

		if(sd->sc_data[SC_FORTUNE].timer!=-1)
			sd->critical += (10+sd->sc_data[SC_FORTUNE].val1+sd->sc_data[SC_FORTUNE].val2
						+sd->sc_data[SC_FORTUNE].val3)*10;

		if(sd->sc_data[SC_EXPLOSIONSPIRITS].timer!=-1) {
			if(s_class.job == 23)
				sd->critical += sd->sc_data[SC_EXPLOSIONSPIRITS].val1 * 100;
			else
			sd->critical += sd->sc_data[SC_EXPLOSIONSPIRITS].val2;
		}

		if (sd->sc_data[SC_STEELBODY].timer != -1)
		{
			sd->def = 90;
			sd->mdef = 90;
			aspd_rate += 25;
			sd->speed = (sd->speed * 125) / 100;
		}
		if (sd->sc_data[SC_DEFENDER].timer != -1 && sd->sc_data[SC_DEFENDER].val1 <= 4)
		{
			sd->aspd = sd->aspd + (25 - (sd->sc_data[SC_DEFENDER].val1 * 5));	// Updated way to calculate attack speed reduction
			sd->speed = sd->speed + (sd->speed / 3);							// Updated way to set movement speed reduction
		}
		if(sd->sc_data[SC_ENCPOISON].timer != -1)
			sd->addeff[4] += sd->sc_data[SC_ENCPOISON].val2;
		if(sd->sc_data[SC_DANCING].timer != -1)
		{
			sd->speed = (double)sd->speed * (6.- 0.4 * pc_checkskill(sd, ((s_class.job == 19) ? BA_MUSICALLESSON : DC_DANCINGLESSON)));
			sd->nhealsp = 0;
			sd->nshealsp = 0;
			sd->nsshealsp = 0;
		}
		if(sd->sc_data[SC_CURSE].timer!=-1)
			sd->speed += 450;

		if(sd->sc_data[SC_TRUESIGHT].timer!=-1)
			sd->critical += sd->sc_data[SC_TRUESIGHT].val1 * 10;

		if(sd->sc_data[SC_BERSERK].timer!=-1)
		{
			sd->def = sd->def2 = 0;
			sd->mdef = sd->mdef2 = 0;
			sd->flee -= sd->flee * 50 / 100;
			aspd_rate -= 30;
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
				sd->def2 -= sd->def2 * 50 / 100;
				break;
			case 5: //Waist break
				sd->def2 -= sd->def2 * 50 / 100;
				sd->base_atk -= sd->base_atk * 25 / 100;
				break;
			}
		}

		if (sd->sc_data[SC_GOSPEL].timer != -1) {
			if (sd->sc_data[SC_GOSPEL].val4 == BCT_PARTY) {
				switch (sd->sc_data[SC_GOSPEL].val3) {
				case 4:
					sd->status.max_hp += sd->status.max_hp * 25 / 100;
					if (sd->status.max_hp > battle_config.max_hp)
						sd->status.max_hp = battle_config.max_hp;
					break;
				case 5:
					sd->status.max_sp += sd->status.max_sp * 25 / 100;
					if (sd->status.max_sp > battle_config.max_sp)
						sd->status.max_sp = battle_config.max_sp;
					break;
				case 11:
					sd->def += sd->def * 25 / 100;
					sd->def2 += sd->def2 * 25 / 100;
					break;
				case 12:
					sd->base_atk += sd->base_atk * 8 / 100;
					break;
				case 13:
					sd->flee += sd->flee * 5 / 100;
					break;
				case 14:
					sd->hit += sd->hit * 5 / 100;
					break;
				}
			} else if (sd->sc_data[SC_GOSPEL].val4 == BCT_ENEMY) {
				switch (sd->sc_data[SC_GOSPEL].val3) {
				case 5:
					sd->def = 0;
					sd->def2 = 0;
					break;
				case 6:
					sd->base_atk = 0;
					sd->watk = 0;
					sd->watk2 = 0;
					break;
				case 7:
					sd->flee = 0;
					break;
				case 8:
					sd->speed_rate += 75;
					aspd_rate += 75;
					break;
				}
			}
		}
		// custom stats, since there's no info on how much it actually gives ^^; [Celest]
		if (sd->sc_data[SC_GUILDAURA].timer != -1) {
			if (sd->sc_data[SC_GUILDAURA].val4 & 1 << 4) {
				sd->hit += 10;
				sd->flee += 10;
			}
		}
	}

	if (sd->matk_rate != 100) {
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
	if (pc_isriding(sd))
		sd->aspd += 50-10*pc_checkskill(sd,KN_CAVALIERMASTERY);								// New way to calculate attack speed with Peco
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
		clif_skillinfoblock(sd);	// スキル送信

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

	if(sd->status.hp<sd->status.max_hp>>2 && sd->sc_data[SC_AUTOBERSERK].timer != -1 && (sd->sc_data[SC_PROVOKE].timer == -1 || sd->sc_data[SC_PROVOKE].val2 == 0 ) && !pc_isdead(sd))
		status_change_start(&sd->bl,SC_PROVOKE,10,1,0,0,0,0);

	return 0;
}

/*==========================================
 * For quick calculating [Celest]
 *------------------------------------------
 */
int status_calc_speed(struct map_session_data *sd) {
	int b_speed, skill;
	struct pc_base_job s_class;

	nullpo_retr(0, sd);

	s_class = pc_calc_base_job(sd->status.class);

	b_speed = sd->speed;
	sd->speed = DEFAULT_WALK_SPEED;

	if (sd->sc_count) {
		if(sd->sc_data[SC_INCREASEAGI].timer!=-1 && sd->sc_data[SC_QUAGMIRE].timer == -1 && sd->sc_data[SC_DONTFORGETME].timer == -1){	// 速度?加
			sd->speed -= sd->speed *25/100;
		}
		if(sd->sc_data[SC_DECREASEAGI].timer!=-1) {
			sd->speed = sd->speed *125/100;
		}
		if(sd->sc_data[SC_CLOAKING].timer!=-1) {
			sd->speed = sd->speed * (sd->sc_data[SC_CLOAKING].val3-sd->sc_data[SC_CLOAKING].val1*3) /100;
		}
		if(sd->sc_data[SC_CHASEWALK].timer!=-1) {
			sd->speed = sd->speed * sd->sc_data[SC_CHASEWALK].val3 /100;
		}
		if(sd->sc_data[SC_QUAGMIRE].timer!=-1){
			sd->speed = sd->speed*3/2;
		}
		if (sd->sc_data[SC_WINDWALK].timer != -1 && sd->sc_data[SC_INCREASEAGI].timer == -1) {
			sd->speed -= sd->speed *(sd->sc_data[SC_WINDWALK].val1*2)/100;
		}
		if(sd->sc_data[SC_CARTBOOST].timer!=-1) {
			sd->speed -= (DEFAULT_WALK_SPEED * 20)/100;
		}
		if(sd->sc_data[SC_BERSERK].timer!=-1) {
			sd->speed -= sd->speed * 20/100;
		}
		if(sd->sc_data[SC_WEDDING].timer!=-1) {
			sd->speed = 2*DEFAULT_WALK_SPEED;
		}
		if(sd->sc_data[SC_DONTFORGETME].timer!=-1){
			sd->speed= sd->speed*(100+sd->sc_data[SC_DONTFORGETME].val1*2 + sd->sc_data[SC_DONTFORGETME].val2 + (sd->sc_data[SC_DONTFORGETME].val3&0xffff))/100;
		}
		if(sd->sc_data[SC_STEELBODY].timer!=-1){
			sd->speed = (sd->speed * 125) / 100;
		}
		if(sd->sc_data[SC_DEFENDER].timer != -1)
		{
			if(sd->sc_data[SC_DEFENDER].val1 <= 4)						// Only level 4 and under defender gains movement reduction
				sd->speed -= (sd->speed / 3);							// Updated way to set movement speed reduction
		}
		if(sd->sc_data[SC_DANCING].timer != -1)
		{
			short speed_mod = 500 - 40 * pc_checkskill(sd, ((s_class.job == 19) ? BA_MUSICALLESSON : DC_DANCINGLESSON));
			if(sd->sc_data[SC_LONGING].timer != -1)
				speed_mod -= 20 * sd->sc_data[SC_LONGING].val1;
			sd->speed += sd->speed * (speed_mod / 100);
		}
		if (sd->sc_data[SC_CURSE].timer != -1)
			sd->speed += 450;
		if (sd->sc_data[SC_SLOWDOWN].timer != -1)
			sd->speed = sd->speed * 150 / 100;
		if (sd->sc_data[SC_SPEEDUP0].timer != -1)
			sd->speed -= sd->speed * 25 / 100;
	}

	if(sd->status.option & 2 && (skill = pc_checkskill(sd,RG_TUNNELDRIVE))>0 )
		sd->speed += (1.2*DEFAULT_WALK_SPEED - skill*9);
	if (pc_iscarton(sd) && (skill=pc_checkskill(sd,MC_PUSHCART))>0)
		sd->speed += (10-skill) * (DEFAULT_WALK_SPEED * 0.1);
	else if (pc_isriding(sd)) {
		sd->speed -= (0.25 * DEFAULT_WALK_SPEED);
	}
	if((skill=pc_checkskill(sd,TF_MISS))>0)
		if(s_class.job==12)
			sd->speed -= sd->speed *(skill*1.5)/100;

	if(sd->speed_rate != 100)
		sd->speed = sd->speed*sd->speed_rate/100;
	if(sd->speed < 1) sd->speed = 1;

	if (sd->skilltimer != -1 && (skill = pc_checkskill(sd, SA_FREECAST)) > 0) {
		sd->prev_speed = sd->speed;
		sd->speed = sd->speed * (175 - skill * 5) / 100;
	}

	if (b_speed != sd->speed)
		clif_updatestatus(sd, SP_SPEED);

	return 0;
}

/*==========================================
 * 対象のClassを返す(汎用)
 * 戻りは整数で0以上
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
 * 対象の方向を返す(汎用)
 * 戻りは整数で0以上
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
 * 対象のレベルを返す(汎用)
 * 戻りは整数で0以上
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
 * 対象の射程を返す(汎用)
 * 戻りは整数で0以上
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
 * 対象のHPを返す(汎用)
 * 戻りは整数で0以上
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
 * 対象のMHPを返す(汎用)
 * 戻りは整数で0以上
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
			if (sc_data[SC_GOSPEL].timer != -1 &&
			    sc_data[SC_GOSPEL].val4 == BCT_PARTY &&
			    sc_data[SC_GOSPEL].val3 == 4)
				max_hp += max_hp * 25 / 100;
		}

		if (max_hp < 1)
			max_hp = 1;
		return max_hp;
	}

	return 1;
}

/*==========================================
 * 対象のStrを返す(汎用)
 * 戻りは整数で0以上
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
			if (sc_data[SC_BLESSING].timer != -1) { // ブレッシング
				int race=status_get_race(bl);
				if (battle_check_undead(race, status_get_elem_type(bl)) || race == 6)
					str >>= 1; // 悪 魔/不死
				else
					str += sc_data[SC_BLESSING].val1; // その他
			}
			if(sc_data[SC_TRUESIGHT].timer != -1) // トゥルーサイト
				str += 5;
			if (sc_data[SC_INCSTR].timer != -1)
				str += sc_data[SC_INCSTR].val1;
		}
		if (str < 0)
			str = 0;
	}

	return str;
}

/*==========================================
 * 対象のAgiを返す(汎用)
 * 戻りは整数で0以上
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
			if (sc_data[SC_INCREASEAGI].timer != -1 && sc_data[SC_QUAGMIRE].timer == -1 && sc_data[SC_DONTFORGETME].timer == -1) // 速度増加(PCはpc.cで)
				agi += 2 + sc_data[SC_INCREASEAGI].val1;

			if (sc_data[SC_CONCENTRATE].timer != -1 && sc_data[SC_QUAGMIRE].timer == -1)
				agi += agi * (2 + sc_data[SC_CONCENTRATE].val1) / 100;

			if (sc_data[SC_DECREASEAGI].timer != -1) // 速度減少
				agi -= 2 + sc_data[SC_DECREASEAGI].val1;

			if (sc_data[SC_QUAGMIRE].timer != -1) { // クァグマイア
				//agi >>= 1;
				//int agib = agi * (sc_data[SC_QUAGMIRE].val1 * 10) / 100;
				//agi -= agib > 50 ? 50 : agib;
				agi -= sc_data[SC_QUAGMIRE].val1 * 10;
			}
			if (sc_data[SC_TRUESIGHT].timer != -1) // トゥルーサイト
				agi += 5;
		}
		if (agi < 0)
			agi = 0;
	}

	return agi;
}

/*==========================================
 * 対象のVitを返す(汎用)
 * 戻りは整数で0以上
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
			if (sc_data[SC_TRUESIGHT].timer != -1) // トゥルーサイト
				vit += 5;
		}

		if (vit < 0)
			vit = 0;
	}

	return vit;
}

/*==========================================
 * 対象のIntを返す(汎用)
 * 戻りは整数で0以上
 *------------------------------------------
 */
int status_get_int(struct block_list *bl) {
	int int_=0;

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
			if (sc_data[SC_BLESSING].timer != -1) { // ブレッシング
				int race = status_get_race(bl);
				if (battle_check_undead(race,status_get_elem_type(bl)) || race == 6)
					int_ >>= 1; // 悪 魔/不死
				else
					int_ += sc_data[SC_BLESSING].val1; // その他
			}
			if (sc_data[SC_STRIPHELM].timer != -1)
				int_ = int_ * 60 / 100;
			if (sc_data[SC_TRUESIGHT].timer != -1) // トゥルーサイト
				int_ += 5;
		}
		if (int_ < 0)
			int_ = 0;
	}

	return int_;
}

/*==========================================
 * 対象のDexを返す(汎用)
 * 戻りは整数で0以上
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

			if (sc_data[SC_BLESSING].timer != -1) {
				int race=status_get_race(bl);
				if (battle_check_undead(race, status_get_elem_type(bl)) || race == 6)
					dex >>= 1;	// 悪 魔/不死
				else
					dex += sc_data[SC_BLESSING].val1; // その他
			}

			if (sc_data[SC_QUAGMIRE].timer != -1) { // クァグマイア
				// dex >>= 1;
				//int dexb = dex * (sc_data[SC_QUAGMIRE].val1 * 10) / 100;
				//dex -= dexb > 50 ? 50 : dexb;
				dex -= sc_data[SC_QUAGMIRE].val1 * 10;
			}
			if (sc_data[SC_TRUESIGHT].timer != -1) // トゥルーサイト
				dex += 5;
		}
		if (dex < 0)
			dex = 0;
	}

	return dex;
}

/*==========================================
 * 対象のLukを返す(汎用)
 * 戻りは整数で0以上
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
			if (sc_data[SC_GLORIA].timer != -1) // グロリア(PCはpc.cで)
				luk += 30;
			if (sc_data[SC_TRUESIGHT].timer != -1) // トゥルーサイト
				luk += 5;
			if (sc_data[SC_CURSE].timer != -1 ) // 呪い
				luk = 0;
		}
		if (luk < 0)
			luk = 0;
	}

	return luk;
}

/*==========================================
 * 対象のFleeを返す(汎用)
 * 戻りは整数で1以上
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
			if (sc_data[SC_WINDWALK].timer != -1) // ウィンドウォーク
				flee += flee * (sc_data[SC_WINDWALK].val2) / 100;
			if (sc_data[SC_SPIDERWEB].timer != -1) //スパイダーウェブ
				flee -= flee * 50 / 100;
			if (sc_data[SC_GOSPEL].timer!=-1) {
				if (sc_data[SC_GOSPEL].val4 == BCT_PARTY &&
				    sc_data[SC_GOSPEL].val3 == 13)
					flee += flee * 5 / 100;
				else if (sc_data[SC_GOSPEL].val4 == BCT_ENEMY &&
				         sc_data[SC_GOSPEL].val3 == 7)
					flee = 0;
			}
			if(sc_data[SC_INCFLEERATE].timer!=-1)
				flee += flee * sc_data[SC_INCFLEERATE].val1 / 100;
		}
	}

	if (flee < 1)
		flee = 1;

	return flee;
}

/*==========================================
 * 対象のHitを返す(汎用)
 * 戻りは整数で1以上
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
			if (sc_data[SC_HUMMING].timer != -1)
				hit += hit * (sc_data[SC_HUMMING].val1 * 2 + sc_data[SC_HUMMING].val2
				      + sc_data[SC_HUMMING].val3) / 100;
			if (sc_data[SC_BLIND].timer != -1)
				hit -= hit * 25 / 100;
			if (sc_data[SC_TRUESIGHT].timer != -1)
				hit += hit * (sc_data[SC_TRUESIGHT].val1) * 3 / 100;	// New way to calculate accuracy bonus
			if (sc_data[SC_CONCENTRATION].timer != -1)
				hit += hit * 10 * sc_data[SC_CONCENTRATION].val1 / 100;
			if (sc_data[SC_GOSPEL].timer != -1 && sc_data[SC_GOSPEL].val4 == BCT_PARTY && sc_data[SC_GOSPEL].val3 == 14)
				hit += hit * 5 / 100;
			if(sc_data[SC_INCHITRATE].timer != -1)
				hit += hit * sc_data[SC_INCHITRATE].val1 / 100;
		}
	}
	if (hit < 1)
		hit = 1;

	return hit;
}

/*==========================================
 * 対象の完全回避を返す(汎用)
 * 戻りは整数で1以上
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
 * 対象のクリティカルを返す(汎用)
 * 戻りは整数で1以上
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
			if (sc_data[SC_TRUESIGHT].timer != -1)
				critical += critical * sc_data[SC_TRUESIGHT].val1 / 100;
		}
	}
	if (critical < 1)
		critical = 1;

	return critical;
}

/*==========================================
 * Base Attack Calculation
 *------------------------------------------
 */
int status_get_baseatk(struct block_list *bl) {
	int batk = 1;

	nullpo_retr(1, bl);

	if (bl->type == BL_PC) {
		batk = ((struct map_session_data *)bl)->base_atk;
		if (((struct map_session_data *)bl)->status.weapon <= 22)
			batk += ((struct map_session_data *)bl)->weapon_atk[((struct map_session_data *)bl)->status.weapon];
	} else {
		int str, dstr;
		struct status_change *sc_data;
		sc_data = status_get_sc_data(bl);
		str = status_get_str(bl);
		dstr = str / 10;
		batk = dstr * dstr + str;
		if (sc_data) {
			if (sc_data[SC_PROVOKE].timer != -1)
// Old Formula
//				batk = batk*(100+2*sc_data[SC_PROVOKE].val1) / 100;
				batk += batk * sc_data[SC_PROVOKE].val3 / 100;
			if (sc_data[SC_CURSE].timer != -1)
				batk -= batk * 25 / 100;
			if (sc_data[SC_CONCENTRATION].timer != -1)
				batk += batk * (5 * sc_data[SC_CONCENTRATION].val1) / 100;
			if (sc_data[SC_BLEEDING].timer != -1)
				batk -= batk * 25 / 100;
		}
	}
	if (batk < 1)
		batk = 1;

	return batk;
}

/*==========================================
 * Attack Rate Calculation
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
// Old Formula
//				atk = atk * (100 + 2 * sc_data[SC_PROVOKE].val1) / 100;
				atk += atk * sc_data[SC_PROVOKE].val3 / 100;
			if (sc_data[SC_CURSE].timer != -1)
				atk -= atk * 25 / 100;
			if (sc_data[SC_CONCENTRATION].timer != -1)
				atk += atk * (5 * sc_data[SC_CONCENTRATION].val1) / 100;
			if (sc_data[SC_GOSPEL].timer != -1) {
				if (sc_data[SC_GOSPEL].val4 == BCT_PARTY &&
				    sc_data[SC_GOSPEL].val3 == 12)
					atk += atk * 8 / 100;
				else if (sc_data[SC_GOSPEL].val4 == BCT_ENEMY &&
				         sc_data[SC_GOSPEL].val3 == 6)
					atk = 0;
			}
			if(sc_data[SC_INCATKRATE].timer != -1)
				atk += atk * sc_data[SC_INCATKRATE].val1 / 100;
		}
	}
	if (atk < 0)
		atk = 0;

	return atk;
}

/*==========================================
 * 対象の左手Atkを返す(汎用)
 * 戻りは整数で0以上
 *------------------------------------------
 */
int status_get_atk_(struct block_list *bl) {
	nullpo_retr(0, bl);

	if (bl->type == BL_PC)
		return ((struct map_session_data*)bl)->watk_;

	return 0;
}

/*==========================================
 * 対象のAtk2を返す(汎用)
 * 戻りは整数で0以上
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
// Old Formula
//				atk2 = atk2 * (100 + 2 * sc_data[SC_PROVOKE].val1) / 100;
				atk2 += atk2 * sc_data[SC_PROVOKE].val3 / 100;
			if (sc_data[SC_CURSE].timer!=-1 )
				atk2 -= atk2 * 25 / 100;
			if (sc_data[SC_DRUMBATTLE].timer != -1)
				atk2 += sc_data[SC_DRUMBATTLE].val2;
			if (sc_data[SC_NIBELUNGEN].timer != -1 && (status_get_element(bl) / 10) >= 8)
				atk2 += sc_data[SC_NIBELUNGEN].val3;
			if (sc_data[SC_STRIPWEAPON].timer != -1)
				atk2 = atk2 * sc_data[SC_STRIPWEAPON].val2 / 100;
			if (sc_data[SC_CONCENTRATION].timer != -1) //コンセントレーション
				atk2 += atk2 * (5 * sc_data[SC_CONCENTRATION].val1) / 100;
		}
		if (atk2 < 0)
			atk2 = 0;
		return atk2;
	}

	return 0;
}

/*==========================================
 * 対象の左手Atk2を返す(汎用)
 * 戻りは整数で0以上
 *------------------------------------------
 */
int status_get_atk_2(struct block_list *bl) {
	nullpo_retr(0, bl);

	if (bl->type == BL_PC)
		return ((struct map_session_data*)bl)->watk_2;

	return 0;
}

/*==========================================
 * 対象のMAtk1を返す(汎用)
 * 戻りは整数で0以上
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
			if (sc_data[SC_MINDBREAKER].timer != -1)
				matk = matk * (100 + 2 * sc_data[SC_MINDBREAKER].val1) / 100;
			if (sc_data[SC_INCMATKRATE].timer != -1)
				matk = matk * (100 + sc_data[SC_INCMATKRATE].val1) /100;
		}
	}

	return matk;
}

/*==========================================
 * 対象のMAtk2を返す(汎用)
 * 戻りは整数で0以上
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
 * Get DEF Status
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
			if(sc_data[SC_FREEZE].timer != -1 || (sc_data[SC_STONE].timer != -1 && sc_data[SC_STONE].val2 == 0))
				def >>= 1;

			if (bl->type != BL_PC)
			{
				if (sc_data[SC_KEEPING].timer != -1)
					def = 100;

				if (sc_data[SC_PROVOKE].timer != -1)
// Old Formula
//					def = (def * (100 - 6 * sc_data[SC_PROVOKE].val1) + 50) / 100;
					def -= def * sc_data[SC_PROVOKE].val4 / 100;

				if (sc_data[SC_DRUMBATTLE].timer != -1)
					def += sc_data[SC_DRUMBATTLE].val3;

				if (sc_data[SC_INCDEFRATE].timer != -1)
					def += def * sc_data[SC_INCDEFRATE].val1 / 100;

				if (sc_data[SC_POISON].timer != -1)
					def = def * 75 / 100;

				if (sc_data[SC_STRIPSHIELD].timer != -1)
					def = def * sc_data[SC_STRIPSHIELD].val2 / 100;

				if (sc_data[SC_SIGNUMCRUCIS].timer != -1)
					def = def * (100 - sc_data[SC_SIGNUMCRUCIS].val2) / 100;

				if (sc_data[SC_CONCENTRATION].timer != -1)
					def = (def * (100 - 5 * sc_data[SC_CONCENTRATION].val1)) / 100;

				if (sc_data[SC_GOSPEL].timer != -1) {
					if (sc_data[SC_GOSPEL].val4 == BCT_PARTY &&
					    sc_data[SC_GOSPEL].val3 == 11)
						def += def * 25 / 100;
					else if (sc_data[SC_GOSPEL].val4 == BCT_ENEMY &&
					         sc_data[SC_GOSPEL].val3 == 5)
						def = 0;
				}

				if (sc_data[SC_JOINTBEAT].timer != -1) {
					if (sc_data[SC_JOINTBEAT].val2 == 4)
						def -= def * 50 / 100;
					else if (sc_data[SC_JOINTBEAT].val2 == 5)
						def -= def * 25 / 100;
				}
			}
		}
		//詠唱中は詠唱時減算率に基づいて減算
		if (skilltimer != -1) {
			int def_rate = skill_get_castdef(skillid);
			if (def_rate != 0)
				def = (def * (100 - def_rate)) / 100;
		}
	}
	if (def < 0) def = 0;

	return def;
}
// === GET MAGIC DEFENCE ===
// =========================
int status_get_mdef(struct block_list *bl) {
	struct status_change *sc_data;
	int mdef = 0;

	nullpo_retr(0, bl);

	sc_data=status_get_sc_data(bl);
	if(bl->type==BL_PC)
		mdef = ((struct map_session_data *)bl)->mdef;
	else if(bl->type==BL_MOB)
		mdef = mob_db[((struct mob_data *)bl)->class].mdef;
	else if(bl->type==BL_PET)
		mdef = mob_db[((struct pet_data *)bl)->class].mdef;

	if(mdef < 1000000)
	{
		if(sc_data)
		{
			if(sc_data[SC_BARRIER].timer != -1)
				mdef = 100;
			if(sc_data[SC_FREEZE].timer != -1 || (sc_data[SC_STONE].timer != -1 && sc_data[SC_STONE].val2 == 0))
				mdef = mdef*125/100;
			if( sc_data[SC_MINDBREAKER].timer!=-1 && bl->type != BL_PC)
				mdef -= (mdef*6*sc_data[SC_MINDBREAKER].val1)/100;
		}
	}

	if(mdef <= 0)
		return 0;

	return mdef;
}

// === GET VIT DEFENCE ===
// =======================
int status_get_def2(struct block_list *bl)
{
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
		if (sc_data)
		{
			if(sc_data[SC_ETERNALCHAOS].timer != -1)
				return 1;
			if(sc_data[SC_ANGELUS].timer != -1)
				def2 = def2 * (110 + 5 * sc_data[SC_ANGELUS].val1) / 100;
			if(sc_data[SC_POISON].timer != -1)
				def2 = def2 * 75 / 100;
			if(sc_data[SC_CONCENTRATION].timer != -1)
				def2 = def2 * (100 - 5 * sc_data[SC_CONCENTRATION].val1) / 100;
			if(sc_data[SC_GOSPEL].timer != -1)
			{
				if (sc_data[SC_GOSPEL].val4 == BCT_PARTY && sc_data[SC_GOSPEL].val3 == 11)
					def2 += def2 * 25 / 100;
				else if (sc_data[SC_GOSPEL].val4 == BCT_ENEMY && sc_data[SC_GOSPEL].val3 == 5)
					return 1;
			}
		}
	}

	if(def2 <= 1)
		return 1;

	return def2;
}

/*==========================================
 * 対象のMDef2を返す(汎用)
 * 戻りは整数で0以上
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
 * 対象のSpeed(移動速度)を返す(汎用)
 * 戻りは整数で1以上
 * Speedは小さいほうが移動速度が速い
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
			//速度増加時は25%減算
			if (sc_data[SC_INCREASEAGI].timer != -1 && sc_data[SC_DONTFORGETME].timer == -1)
				speed -= speed / 4; // speed -= speed * 25 / 100;
			//速度減少時は25%加算
			if (sc_data[SC_DECREASEAGI].timer != -1)
				speed = speed * 125 / 100;
			//クァグマイア時は50%加算
			if (sc_data[SC_QUAGMIRE].timer != -1)
				speed = speed * 3 / 2;
			//私を忘れないで…時は加算
			if (sc_data[SC_DONTFORGETME].timer != -1)
				speed = speed * (100 + sc_data[SC_DONTFORGETME].val1 * 2 + sc_data[SC_DONTFORGETME].val2 + (sc_data[SC_DONTFORGETME].val3 & 0xffff)) / 100;
			//金剛時は25%加算
			if (sc_data[SC_STEELBODY].timer != -1)
				speed = speed * 125 / 100;
			//ディフェンダー時は加算
			// removed as of 12/14's patch [celest]
			/*if (sc_data[SC_DEFENDER].timer != -1)
				speed = (speed * (155 - sc_data[SC_DEFENDER].val1 * 5)) / 100;*/
			//踊り状態は4倍遅い
			if (sc_data[SC_DANCING].timer != -1)
				speed *= 6;
			//呪い時は450加算
			if (sc_data[SC_CURSE].timer != -1)
				speed = speed + 450;
			//ウィンドウォーク時はLv*2%減算
			if (sc_data[SC_WINDWALK].timer != -1 && sc_data[SC_INCREASEAGI].timer == -1)
				speed -= (speed * (sc_data[SC_WINDWALK].val1 * 2)) / 100;
			if (sc_data[SC_SLOWDOWN].timer != -1)
				speed = speed * 150 / 100;
			if (sc_data[SC_SPEEDUP0].timer != -1)
				speed -= speed / 4; // speed -= speed * 25 / 100
			if (sc_data[SC_GOSPEL].timer != -1 &&
			    sc_data[SC_GOSPEL].val4 == BCT_ENEMY &&
			    sc_data[SC_GOSPEL].val3 == 8)
				speed = speed * 125 / 100;
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
 * 対象のaDelay(攻撃時ディレイ)を返す(汎用)
 * aDelayは小さいほうが攻撃速度が速い
 *------------------------------------------
 */
int status_get_adelay(struct block_list *bl) {
	nullpo_retr(4000, bl);

	if(bl->type==BL_PC)
		return (((struct map_session_data *)bl)->aspd<<1);
	else {
		struct status_change *sc_data=status_get_sc_data(bl);
		int adelay=4000,aspd_rate = 100,i;
		if(bl->type==BL_MOB)
			adelay = mob_db[((struct mob_data *)bl)->class].adelay;
		else if(bl->type==BL_PET)
			adelay = mob_db[((struct pet_data *)bl)->class].adelay;

		if(sc_data) {
			//ツーハンドクイッケン使用時でクァグマイアでも私を忘れないで…でもない時は3割減算
			if(sc_data[SC_TWOHANDQUICKEN].timer != -1 && sc_data[SC_QUAGMIRE].timer == -1 && sc_data[SC_DONTFORGETME].timer == -1)	// 2HQ
				aspd_rate -= 30;
			//アドレナリンラッシュ使用時でツーハンドクイッケンでもクァグマイアでも私を忘れないで…でもない時は
			if(sc_data[SC_ADRENALINE].timer != -1 && sc_data[SC_TWOHANDQUICKEN].timer == -1 &&
				sc_data[SC_QUAGMIRE].timer == -1 && sc_data[SC_DONTFORGETME].timer == -1) {	// アドレナリンラッシュ
				//使用者とパーティメンバーで格差が出る設定でなければ3割減算
				if(sc_data[SC_ADRENALINE].val2 || !battle_config.party_skill_penalty)
					aspd_rate -= 30;
				//そうでなければ2.5割減算
				else
					aspd_rate -= 25;
			}
			//スピアクィッケン時は減算
			if(sc_data[SC_SPEARSQUICKEN].timer != -1 && sc_data[SC_ADRENALINE].timer == -1 &&
				sc_data[SC_TWOHANDQUICKEN].timer == -1 && sc_data[SC_QUAGMIRE].timer == -1 && sc_data[SC_DONTFORGETME].timer == -1)	// スピアクィッケン
				aspd_rate -= sc_data[SC_SPEARSQUICKEN].val2;
			//夕日のアサシンクロス時は減算
			if(sc_data[SC_ASSNCROS].timer!=-1 && // 夕陽のアサシンクロス
				sc_data[SC_TWOHANDQUICKEN].timer == -1 && sc_data[SC_ADRENALINE].timer == -1 && sc_data[SC_SPEARSQUICKEN].timer == -1 &&
				sc_data[SC_DONTFORGETME].timer == -1)
				aspd_rate -= 5+sc_data[SC_ASSNCROS].val1+sc_data[SC_ASSNCROS].val2+sc_data[SC_ASSNCROS].val3;
			//私を忘れないで…時は加算
			if(sc_data[SC_DONTFORGETME].timer!=-1)		// 私を忘れないで
				aspd_rate += sc_data[SC_DONTFORGETME].val1 * 3 + sc_data[SC_DONTFORGETME].val2 + (sc_data[SC_DONTFORGETME].val3 >> 16);
			//金剛時25%加算
			if (sc_data[SC_STEELBODY].timer != -1)	// 金剛
				aspd_rate += 25;
			//増速ポーション使用時は減算
			if (sc_data[i=SC_SPEEDPOTION3].timer != -1 || sc_data[i=SC_SPEEDPOTION2].timer != -1 || sc_data[i=SC_SPEEDPOTION1].timer != -1 || sc_data[i=SC_SPEEDPOTION0].timer != -1)
				aspd_rate -= sc_data[i].val2;
			//ディフェンダー時は加算
			if (sc_data[SC_DEFENDER].timer != -1)
				adelay += (1100 - sc_data[SC_DEFENDER].val1 * 100);
			if (sc_data[SC_GOSPEL].timer!=-1 &&
			    sc_data[SC_GOSPEL].val4 == BCT_ENEMY &&
			    sc_data[SC_GOSPEL].val3 == 8)
				aspd_rate += 25;
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
				sc_data[SC_QUAGMIRE].timer == -1 && sc_data[SC_DONTFORGETME].timer == -1) {	// アドレナリンラッシュ
				if(sc_data[SC_ADRENALINE].val2 || !battle_config.party_skill_penalty)
					aspd_rate -= 30;
				else
					aspd_rate -= 25;
			}
			if(sc_data[SC_SPEARSQUICKEN].timer != -1 && sc_data[SC_ADRENALINE].timer == -1 &&
				sc_data[SC_TWOHANDQUICKEN].timer == -1 && sc_data[SC_QUAGMIRE].timer == -1 && sc_data[SC_DONTFORGETME].timer == -1)	// スピアクィッケン
				aspd_rate -= sc_data[SC_SPEARSQUICKEN].val2;
			if(sc_data[SC_ASSNCROS].timer!=-1 && // 夕陽のアサシンクロス
				sc_data[SC_TWOHANDQUICKEN].timer == -1 && sc_data[SC_ADRENALINE].timer == -1 && sc_data[SC_SPEARSQUICKEN].timer == -1 &&
				sc_data[SC_DONTFORGETME].timer == -1)
				aspd_rate -= 5+sc_data[SC_ASSNCROS].val1+sc_data[SC_ASSNCROS].val2+sc_data[SC_ASSNCROS].val3;
			if(sc_data[SC_DONTFORGETME].timer!=-1)		// 私を忘れないで
				aspd_rate += sc_data[SC_DONTFORGETME].val1 * 3 + sc_data[SC_DONTFORGETME].val2 + (sc_data[SC_DONTFORGETME].val3 >> 16);
			if(sc_data[SC_STEELBODY].timer!=-1)	// 金剛
				aspd_rate += 25;
			if (sc_data[i=SC_SPEEDPOTION3].timer != -1 || sc_data[i=SC_SPEEDPOTION2].timer != -1 || sc_data[i=SC_SPEEDPOTION1].timer != -1 || sc_data[i=SC_SPEEDPOTION0].timer != -1)
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

int status_get_dmotion(struct block_list *bl)
{
	int ret;
	struct status_change *sc_data;

	nullpo_retr(0, bl);

	sc_data = status_get_sc_data(bl);
	if (bl->type == BL_MOB){
		ret = mob_db[((struct mob_data *)bl)->class].dmotion;
		if (battle_config.monster_damage_delay_rate != 100)
			ret = ret * battle_config.monster_damage_delay_rate / 400;
	}
	else if (bl->type == BL_PC){
		ret = ((struct map_session_data *)bl)->dmotion;
		if (battle_config.pc_damage_delay_rate != 100)
			ret = ret * battle_config.pc_damage_delay_rate / 400;
	}
	else if (bl->type == BL_PET)
		ret = mob_db[((struct pet_data *)bl)->class].dmotion;
	else
		return 2000;

	if(sc_data)
	{
		if(!map[bl->m].flag.gvg)	// Endure and Berserk won't give protection against flinch in gvg maps
			if(sc_data[SC_ENDURE].timer != -1 || sc_data[SC_BERSERK].timer != -1)
				return 0;
		if(sc_data[SC_CONCENTRATION].timer != -1 || (bl->type == BL_PC && ((struct map_session_data *)bl)->special_state.infinite_endure))
			return 0;
	}
	return ret;
}

int status_get_element(struct block_list *bl) {
	int ret = 20;
	struct status_change *sc_data;

	nullpo_retr(ret, bl);

	sc_data = status_get_sc_data(bl);
	if(bl->type==BL_MOB)	// 10の位＝Lv*2、１の位＝属性
		ret=((struct mob_data *)bl)->def_ele;
	else if(bl->type==BL_PC)
		ret=20+((struct map_session_data *)bl)->def_ele;	// 防御属性Lv1
	else if(bl->type==BL_PET)
		ret = mob_db[((struct pet_data *)bl)->class].element;

	if(sc_data) {
		if( sc_data[SC_BENEDICTIO].timer!=-1 )	// 聖体降福
			ret=26;
		if( sc_data[SC_FREEZE].timer!=-1 )	// 凍結
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
		if(sc_data[SC_FROSTWEAPON].timer != -1)
			ret = 1;
		if(sc_data[SC_SEISMICWEAPON].timer != -1)
			ret = 2;
		if(sc_data[SC_FLAMELAUNCHER].timer != -1)
			ret = 3;
		if(sc_data[SC_LIGHTNINGLOADER].timer != -1)
			ret = 4;
		if(sc_data[SC_ENCPOISON].timer != -1)
			ret = 5;
		if(sc_data[SC_ASPERSIO].timer != -1)
			ret = 6;
	}

	return ret;
}

int status_get_attack_element2(struct block_list *bl) {
	nullpo_retr(0, bl);

	if(bl->type==BL_PC) {
		int ret = ((struct map_session_data *)bl)->atk_ele_;
		struct status_change *sc_data = ((struct map_session_data *)bl)->sc_data;

		if(sc_data[SC_FROSTWEAPON].timer != -1)
			ret = 1;
		if(sc_data[SC_SEISMICWEAPON].timer != -1)
			ret = 2;
		if(sc_data[SC_FLAMELAUNCHER].timer != -1)
			ret = 3;
		if(sc_data[SC_LIGHTNINGLOADER].timer != -1)
			ret = 4;
		if(sc_data[SC_ENCPOISON].timer != -1)
			ret = 5;
		if(sc_data[SC_ASPERSIO].timer != -1)
			ret = 6;
		return ret;
	}

	return 0;
}

int status_get_party_id(struct block_list *bl) {
	nullpo_retr(0, bl);

	if(bl->type==BL_PC)
		return ((struct map_session_data *)bl)->status.party_id;
	else if(bl->type==BL_MOB){
		struct mob_data *md=(struct mob_data *)bl;
		if (md->master_id > 0)
			return -md->master_id;
		return -md->bl.id;
	}
	else if(bl->type==BL_SKILL)
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

// === SET SIZE OF MONSTER , PET OR PLAYER ===
// ===========================================
int status_get_size(struct block_list *bl)
{
	int value;
	struct map_session_data *sd = (struct map_session_data *)bl;

	nullpo_retr(1, bl);

	switch(bl->type) {
	case BL_MOB:
		value = mob_db[((struct mob_data *)bl)->class].size;
		break;
	case BL_PET:
		value = mob_db[((struct pet_data *)bl)->class].size;
		break;
	case BL_PC:
		value = (sd->class_&JOBL_BABY) ? 0 : 1;
		break;
	default:
		value = 1;
		break;
	}
	return value;
}

int status_get_mode(struct block_list *bl) {
	nullpo_retr(0x01, bl);

	if (bl->type == BL_MOB)
		return mob_db[((struct mob_data *)bl)->class].mode;
	else if (bl->type == BL_PET)
		return mob_db[((struct pet_data *)bl)->class].mode;
	else
		return 0x01;
}

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
	else if (bl->type == BL_PC)
		return pc_isdead((struct map_session_data *)bl);

	return 0;
}

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
	case SC_STAN:
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
		if (sc_data && sc_data[SC_GOSPEL].timer != -1 &&
		    sc_data[SC_GOSPEL].val4 == BCT_PARTY &&
		    sc_data[SC_GOSPEL].val3 == 3)
			sc_def -= 25;
	}

	return (sc_def < 0) ? 0 : sc_def;
}

// === INFLICT SPECIAL EFFECT TO TARGET ===
// ========================================
int status_change_start(struct block_list *bl, int type, intptr_t val1, intptr_t val2, intptr_t val3, intptr_t val4, int tick, int flag)
{
	struct map_session_data *sd = NULL;
	struct status_change* sc_data;
	short *sc_count, *option, *opt1, *opt2, *opt3;
	int opt_flag = 0, calc_flag = 0, updateflag = 0, save_flag = 0, race, mode, elem, undead_flag;
	int scdef = 0;

	nullpo_retr(0, bl);

	if(bl->type == BL_PET)
		return 0;
	if(bl->type == BL_SKILL)
		return 0;
	if(bl->type == BL_MOB)
		if(status_isdead(bl))
			return 0;

	nullpo_retr(0, sc_data = status_get_sc_data(bl));
	nullpo_retr(0, sc_count = status_get_sc_count(bl));
	nullpo_retr(0, option = status_get_option(bl));
	nullpo_retr(0, opt1 = status_get_opt1(bl));
	nullpo_retr(0, opt2 = status_get_opt2(bl));
	nullpo_retr(0, opt3 = status_get_opt3(bl));

	race = status_get_race(bl);
	mode = status_get_mode(bl);
	elem = status_get_elem_type(bl);
	undead_flag = battle_check_undead(race,elem);

	if (type == SC_AETERNA && (sc_data[SC_STONE].timer != -1 || sc_data[SC_FREEZE].timer != -1))
		return 0;

	switch(type)
	{
		case SC_STONE:
		case SC_FREEZE:
			scdef = 3 + status_get_mdef(bl) + status_get_luk(bl) / 3;
			break;
		case SC_STAN:
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

		default:
			scdef=0;
	}
	if(scdef>=100)
		return 0;
	if(bl->type==BL_PC){
		sd = (struct map_session_data *)bl;
		if(sd && type == SC_ADRENALINE && !(skill_get_weapontype(BS_ADRENALINE)&(1<<sd->status.weapon)))
			return 0;

		if(SC_STONE<=type && type<=SC_BLIND){
			if(sd && sd->reseff[type-SC_STONE] > 0 && rand()%10000<sd->reseff[type-SC_STONE]){
				if(battle_config.battle_log)
					printf("PC %d skill_sc_start\n",sd->bl.id);
				return 0;
			}
		}
	}
	else if(bl->type == BL_MOB) {
	}
	else {
		if(battle_config.error_log)
			printf("status_change_start: neither MOB nor PC !\n");
		return 0;
	}

	if((type == SC_FREEZE || type==SC_STONE ) && undead_flag && !(flag&1))
		return 0;

	if(type == SC_OVERTHRUST && sc_data[type].timer != -1 && sc_data[type].val2 && !val2) {
			if (sc_data[SC_MAXOVERTHRUST].timer != -1)
				return 0;
	}

	if((type == SC_ADRENALINE || type == SC_WEAPONPERFECTION) &&
		sc_data[type].timer != -1 && sc_data[type].val2 && !val2)
		return 0;

	if(mode & 0x20 && (type==SC_STONE || type==SC_FREEZE || type==SC_COMA ||
		type==SC_STAN || type==SC_SLEEP || type==SC_SILENCE || type==SC_QUAGMIRE || type == SC_DECREASEAGI || type == SC_SIGNUMCRUCIS || type == SC_PROVOKE ||
		(type == SC_BLESSING && (undead_flag || race == 6))) && !(flag&1)){
		return 0;
	}
	if (type == SC_FREEZE || type == SC_STAN || type == SC_SLEEP)
		battle_stopwalking(bl, 1);

	if (sc_data[type].timer != -1) {
		if (sc_data[type].val1 > val1 && type != SC_COMBO && type != SC_DANCING && type != SC_DEVOTION &&
		    type != SC_SPEEDPOTION0 && type != SC_SPEEDPOTION1 && type != SC_SPEEDPOTION2 && type != SC_SPEEDPOTION3
		    && type != SC_ATKPOT && type != SC_MATKPOT) // added atk and matk potions [Valaris]
			return 0;
		if ((type >=SC_STAN && type <= SC_BLIND) || type == SC_DPOISON)
			return 0;
		(*sc_count)--;
		delete_timer(sc_data[type].timer, status_change_timer);
		sc_data[type].timer = -1;
	}

	switch(type)
	{
		case SC_PROVOKE:
			if (tick <= 0) tick = 1000;
			calc_flag = 1;
			val3 = 2 + 3 * val1;
			val4 = 5 + 5 * val1;
			break;
		case SC_ENDURE:
			if (tick <= 0) tick = 1000 * 60;
			calc_flag = 1;
			val2 = 7;
			break;
		case SC_AUTOBERSERK:
			{
				if (!(flag&2))
					tick = 60 * 1000;
				if (bl->type == BL_PC && sd->status.hp<sd->status.max_hp>>2 &&
					(sc_data[SC_PROVOKE].timer == -1 || sc_data[SC_PROVOKE].val2 == 0))
					status_change_start(bl,SC_PROVOKE,10,1,0,0,0,0);
			}
			break;
		case SC_CONCENTRATE:
			calc_flag = 1;
			break;
		case SC_BLESSING:
			{
				if(bl->type == BL_PC || (!undead_flag && race != 6)) {
					if(sc_data[SC_CURSE].timer!=-1 )
						status_change_end(bl,SC_CURSE,-1);
					if(sc_data[SC_STONE].timer!=-1 && sc_data[SC_STONE].val2 == 0)
						status_change_end(bl,SC_STONE,-1);
				}
				calc_flag = 1;
			}
			break;
		case SC_ANGELUS:
			calc_flag = 1;
			break;
		case SC_INCREASEAGI:
			calc_flag = 1;
			if(sc_data[SC_DECREASEAGI].timer!=-1 )
				status_change_end(bl,SC_DECREASEAGI,-1);
			break;
		case SC_DECREASEAGI:
			if (bl->type == BL_PC)
				tick>>=1;
			calc_flag = 1;
			if(sc_data[SC_CARTBOOST].timer!=-1 )
				status_change_end(bl,SC_CARTBOOST,-1);
			if(sc_data[SC_INCREASEAGI].timer!=-1 )
				status_change_end(bl,SC_INCREASEAGI,-1);
			if(sc_data[SC_ADRENALINE].timer!=-1 )
				status_change_end(bl,SC_ADRENALINE,-1);
			if(sc_data[SC_SPEARSQUICKEN].timer!=-1 )
				status_change_end(bl,SC_SPEARSQUICKEN,-1);
			if(sc_data[SC_TWOHANDQUICKEN].timer!=-1 )
				status_change_end(bl,SC_TWOHANDQUICKEN,-1);
			break;
		case SC_SIGNUMCRUCIS:
			calc_flag = 1;
			val2 = 10 + val1*2;
			if(!(flag&2))
				tick = 600 * 1000;
			clif_emotion(bl,4);
			break;
		case SC_MAXOVERTHRUST:
			if (sc_data[SC_OVERTHRUST].timer != -1)
				status_change_end(bl, SC_OVERTHRUST, -1);
			break;
		case SC_SLOWPOISON:
			if (sc_data[SC_POISON].timer == -1 && sc_data[SC_DPOISON].timer == -1)
				return 0;
			break;
		case SC_TWOHANDQUICKEN:
			if (sc_data[SC_DECREASEAGI].timer != -1)
				return 0;
			*opt3 |= 1;
			calc_flag = 1;
			break;
		case SC_ADRENALINE:
			if (sc_data[SC_DECREASEAGI].timer != -1)
				return 0;
			calc_flag = 1;
			break;
		case SC_WEAPONPERFECTION:
			break;
		case SC_OVERTHRUST:
			*opt3 |= 2;
			break;
		case SC_MAXIMIZEPOWER:
			if(!(flag&2)) {
				if (bl->type == BL_PC)
					val2 = tick;
				else
					tick = 5000 * val1;
			}
			break;
		case SC_ENCPOISON:
			calc_flag = 1;
			val2=(((val1 - 1) / 2) + 3)*100;
			skill_encchant_eremental_end(bl,SC_ENCPOISON);
			break;
		case SC_EDP:
			val2 = val1 + 2;
			calc_flag = 1;
			break;
		case SC_POISONREACT:
			if(!(flag&4))
				val2 = val1/2 + val1%2;
			break;
		case SC_IMPOSITIO:
			calc_flag = 1;
			break;
		case SC_ASPERSIO:
			skill_encchant_eremental_end(bl,SC_ASPERSIO);
			break;
		case SC_SUFFRAGIUM:
		case SC_BENEDICTIO:
		case SC_MAGNIFICAT:
		case SC_AETERNA:
			break;
		case SC_ENERGYCOAT:
			*opt3 |= 4;
			break;
		case SC_MAGICROD:
			val2 = val1*20;
			break;
		case SC_KYRIE:
			if(!(flag&4)) {
				val2 = status_get_max_hp(bl) * (val1 * 2 + 10) / 100;	// set hp counter
				val3 = (val1 / 2 + 5);	// set hit counter
			}
			if(sc_data[SC_ASSUMPTIO].timer != -1)
				status_change_end(bl,SC_ASSUMPTIO,-1);
			break;
		case SC_MINDBREAKER:
			calc_flag = 1;
			if(tick <= 0) tick = 1000;
		case SC_GLORIA:
			calc_flag = 1;
			break;
		case SC_LOUD:
			calc_flag = 1;
			break;
		case SC_TRICKDEAD:
			if (bl->type == BL_PC) {
				pc_stopattack(sd);
			}
			break;
		case SC_QUAGMIRE:
			calc_flag = 1;
			if(sc_data[SC_CONCENTRATE].timer!=-1 )
				status_change_end(bl,SC_CONCENTRATE,-1);
			if(sc_data[SC_INCREASEAGI].timer!=-1 )
				status_change_end(bl,SC_INCREASEAGI,-1);
			if(sc_data[SC_TWOHANDQUICKEN].timer!=-1 )
				status_change_end(bl,SC_TWOHANDQUICKEN,-1);
			if(sc_data[SC_SPEARSQUICKEN].timer!=-1 )
				status_change_end(bl,SC_SPEARSQUICKEN,-1);
			if(sc_data[SC_ADRENALINE].timer!=-1 )
				status_change_end(bl,SC_ADRENALINE,-1);
			if(sc_data[SC_LOUD].timer!=-1 )
				status_change_end(bl,SC_LOUD,-1);
			if(sc_data[SC_TRUESIGHT].timer!=-1 )
				status_change_end(bl,SC_TRUESIGHT,-1);
			if(sc_data[SC_WINDWALK].timer!=-1 )
				status_change_end(bl,SC_WINDWALK,-1);
			if(sc_data[SC_CARTBOOST].timer!=-1 )
				status_change_end(bl,SC_CARTBOOST,-1);
			break;
		case SC_MAGICPOWER:		/* 魔法力増幅 */
			calc_flag = 1;
			val2 = 1;
			break;
		case SC_SACRIFICE:
			if(!(flag&4))
				val2 = 5;
			break;
		case SC_FLAMELAUNCHER:
			skill_encchant_eremental_end(bl, SC_FLAMELAUNCHER);
			break;
		case SC_FROSTWEAPON:
			skill_encchant_eremental_end(bl, SC_FROSTWEAPON);
			break;
		case SC_LIGHTNINGLOADER:
			skill_encchant_eremental_end(bl, SC_LIGHTNINGLOADER);
			break;
		case SC_SEISMICWEAPON:
			skill_encchant_eremental_end(bl, SC_SEISMICWEAPON);
			break;
		case SC_DEVOTION:
			calc_flag = 1;
			break;
		case SC_PROVIDENCE:
			calc_flag = 1;
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

		case SC_AUTOSPELL:			/* オートスペル */
			val4 = 5 + val1*2;
			break;

		case SC_VOLCANO:
			calc_flag = 1;
			val3 = val1*10;
			val4 = val1>=5?20: (val1==4?19: (val1==3?17: ( val1==2?14:10 ) ) );
			break;
		case SC_DELUGE:
			calc_flag = 1;
			val3 = val1>=5?15: (val1==4?14: (val1==3?12: ( val1==2?9:5 ) ) );
			val4 = val1>=5?20: (val1==4?19: (val1==3?17: ( val1==2?14:10 ) ) );
			break;
		case SC_VIOLENTGALE:
			calc_flag = 1;
			val3 = val1*3;
			val4 = val1>=5?20: (val1==4?19: (val1==3?17: ( val1==2?14:10 ) ) );
			break;

		case SC_SPEARSQUICKEN:		/* スピアクイッケン */
			calc_flag = 1;
			val2 = 20+val1;
			*opt3 |= 1;
			break;
		case SC_COMBO:
			break;
		case SC_BLADESTOP_WAIT:		/* 白刃取り(待ち) */
			break;
		case SC_BLADESTOP:		/* 白刃取り */
			if(val2==2) clif_bladestop((struct block_list *)val3,(struct block_list *)val4,1);
			*opt3 |= 32;
			break;

		case SC_LULLABY:			/* 子守唄 */
			val2 = 11;
			break;
		case SC_RICHMANKIM:
			break;
		case SC_ETERNALCHAOS:		/* エターナルカオス */
			calc_flag = 1;
			break;
		case SC_DRUMBATTLE:			/* 戦太鼓の響き */
			calc_flag = 1;
			val2 = (val1+1)*25;
			val3 = (val1+1)*2;
			break;
		case SC_NIBELUNGEN:
			calc_flag = 1;
			val3 = (val1+2)*25;
			break;
		case SC_ROKISWEIL:
		case SC_INTOABYSS:
		case SC_LONGING:
			break;
		case SC_SIEGFRIED:			/* 不死身のジークフリード */
			calc_flag = 1;
			val2 = 55 + val1*5;
			val3 = val1*10;
			break;
		case SC_DISSONANCE:			/* 不協和音 */
			val2 = 10;
			break;
		case SC_WHISTLE:			/* 口笛 */
			calc_flag = 1;
			break;
		case SC_ASSNCROS:			/* 夕陽のアサシンクロス */
			calc_flag = 1;
			break;
		case SC_POEMBRAGI:			/* ブラギの詩 */
			break;
		case SC_APPLEIDUN:			/* イドゥンの林檎 */
			calc_flag = 1;
			break;
		case SC_UGLYDANCE:			/* 自分勝手なダンス */
			val2 = 10;
			break;
		case SC_HUMMING:			/* ハミング */
			calc_flag = 1;
			break;
		case SC_DONTFORGETME:		/* 私を忘れないで */
			calc_flag = 1;
			if(sc_data[SC_INCREASEAGI].timer!=-1 )	/* 速度上昇解除 */
				status_change_end(bl,SC_INCREASEAGI,-1);
			if(sc_data[SC_TWOHANDQUICKEN].timer!=-1 )
				status_change_end(bl,SC_TWOHANDQUICKEN,-1);
			if(sc_data[SC_SPEARSQUICKEN].timer!=-1 )
				status_change_end(bl,SC_SPEARSQUICKEN,-1);
			if(sc_data[SC_ADRENALINE].timer!=-1 )
				status_change_end(bl,SC_ADRENALINE,-1);
			if(sc_data[SC_ASSNCROS].timer!=-1 )
				status_change_end(bl,SC_ASSNCROS,-1);
			if(sc_data[SC_TRUESIGHT].timer!=-1 )	/* トゥルーサイト */
				status_change_end(bl,SC_TRUESIGHT,-1);
			if (sc_data[SC_WINDWALK].timer != -1)	/* ウインドウォーク */
				status_change_end(bl, SC_WINDWALK, -1);
			if (sc_data[SC_CARTBOOST].timer != -1)	/* カートブースト */
				status_change_end(bl, SC_CARTBOOST, -1);
			break;
		case SC_FORTUNE:			/* 幸運のキス */
			calc_flag = 1;
			break;
		case SC_SERVICE4U:			/* サービスフォーユー */
			calc_flag = 1;
			break;
		case SC_MOONLIT:
			val2 = bl->id;
			break;
		case SC_DANCING:			/* ダンス/演奏中 */
			calc_flag = 1;
			if(!(flag&4)) {
				val3 = tick / 1000;
				tick = 1000;
			}
			break;
		case SC_EXPLOSIONSPIRITS:
			calc_flag = 1;
			val2 = 75 + 25 * val1;
			*opt3 |= 8;
			break;
		case SC_STEELBODY:			// 金剛
			calc_flag = 1;
			*opt3 |= 16;
			break;
		case SC_EXTREMITYFIST:		/* 阿修羅覇凰拳 */
			break;
		case SC_AUTOCOUNTER:
			val3 = val4 = 0;
			break;

		case SC_SPEEDPOTION0:		/* 増速ポーション */
		case SC_SPEEDPOTION1:
		case SC_SPEEDPOTION2:
		case SC_SPEEDPOTION3:
			calc_flag = 1;
			if(!(flag&2))
				tick = 1000 * tick;
			val2 = 5 * (2 + type - SC_SPEEDPOTION0);
			break;

		/* atk & matk potions [Valaris] */
		case SC_ATKPOT:
		case SC_MATKPOT:
			calc_flag = 1;
			tick = 1000 * tick;
			break;
		case SC_WEDDING:
			{
				time_t timer;

				calc_flag = 1;
				tick = 10000;
				if(!val2)
					val2 = time(&timer);
			}
			break;
		case SC_NOCHAT:
			{
				time_t timer;

				if(!battle_config.muting_players)
					break;

				if(!(flag&2))
					tick = 60000;
				if(!val2)
					val2 = time(&timer);
				updateflag = SP_MANNER;
				save_flag = 1; // celest
			}
			break;

		/* option1 */
		case SC_STONE:				/* 石化 */
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
		case SC_SLEEP:
			if(!(flag&2)) {
				tick = 30000;
			}
			break;
		case SC_FREEZE:				/* 凍結 */
			if(!(flag&2)) {
				int sc_def = 100 - status_get_mdef(bl);
				tick = tick * sc_def / 100;
			}
			break;
		case SC_STAN:				/* スタン（val2にミリ秒セット） */
			if(!(flag&2)) {
				int sc_def = status_get_sc_def_vit(bl);
				tick = tick * sc_def / 100;
			}
			break;

		/* option2 */
		case SC_DPOISON: /* 猛毒 */
		{
			int mhp = status_get_max_hp(bl);
			int hp = status_get_hp(bl);
			if (hp > mhp>>2) {
				if(bl->type == BL_PC) {
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
		}
		case SC_POISON:
			calc_flag = 1;
			if (!(flag & 2)) {
				int sc_def = 100 - (status_get_vit(bl) + status_get_luk(bl)/5);
				tick = tick * sc_def / 100;
			}
			if(!(flag&4))
				val3 = tick / 1000;
			if (val3 < 1) val3 = 1;
			if(!(flag&4))
				tick = 1000;
			break;
		case SC_SILENCE:			/* 沈黙（レックスデビーナ） */
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
			clif_emotion(bl, 1);
			if(sd)
				pc_stop_walking(sd, 0);
			break;
		case SC_BLIND:
			calc_flag = 1;
			if (!(flag & 2)) {
				int sc_def = status_get_lv(bl) / 10 + status_get_int(bl) / 15;
				tick = 30000 - sc_def;
			}
			break;
		case SC_CURSE:
			calc_flag = 1;
			if(!(flag&2)) {
				int sc_def = 100 - status_get_vit(bl);
				tick = tick * sc_def / 100;
			}
			break;

		/* option */
		case SC_HIDING:
			calc_flag = 1;
			if(bl->type == BL_PC && !(flag&4)) {
				val2 = tick / 1000;
				tick = 1000;
			}
			break;
		case SC_CHASEWALK:
		case SC_CLOAKING:
			if(flag&4)
				break;
			if(bl->type == BL_PC) {
				calc_flag = 1;
				val2 = tick;
				val3 = type==SC_CLOAKING ? 130-val1*3 : 135-val1*5;
			}
			else
				tick = 5000*val1;
			break;
		case SC_SIGHT:
		case SC_RUWACH:
			if(flag&4)
				break;
			val2 = tick/250;
			tick = 10;
			break;

		case SC_SAFETYWALL:
		case SC_PNEUMA:
			if(flag&4)
				break;
			tick = ((struct skill_unit *)val2)->group->limit;
			break;

		case SC_ANKLE:
			break;

		case SC_RIDING:
			calc_flag = 1;
			tick = 600*1000;
			break;
		case SC_FALCON:
		case SC_WEIGHT50:
		case SC_WEIGHT90:
		case SC_BROKNWEAPON:
		case SC_BROKNARMOR:
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
			calc_flag = 1;
			val2 = 5 + val1*15;
			break;

		case SC_KEEPING:
		case SC_BARRIER:
			calc_flag = 1;
		case SC_HALLUCINATION:
			break;
		case SC_CONCENTRATION:
			*opt3 |= 1;
			calc_flag = 1;
			break;
		case SC_TENSIONRELAX:
			calc_flag = 1;
			if(bl->type == BL_PC) {
				tick = 10000;
			}
			break;
		case SC_AURABLADE:
			break;

		case SC_PARRYING:
			val2 = 20 + val1 * 3;
			break;

		case SC_WINDWALK:
			calc_flag = 1;
			val2 = (val1 / 2);
			break;

		case SC_JOINTBEAT: // Random break [DracoRPG]
			calc_flag = 1;
			val2 = rand() % 6 + 1;
			if (val2 == 6)
				status_change_start(bl, SC_BLEEDING, val1, 0, 0, 0, skill_get_time2(type, val1), 0);
			break;

		case SC_BERSERK:
			if(sd && !(flag&4)){
				sd->status.hp = sd->status.max_hp * 3;
				sd->status.sp = 0;
				clif_updatestatus(sd, SP_HP);
				clif_updatestatus(sd, SP_SP);
				clif_status_change(bl, SC_INCREASEAGI,1);
				sd->canregen_tick = gettick_cache + 300000;
			}
			*opt3 |= 128;
			if(!(flag&4))
				tick = 10000;
			calc_flag = 1;
			break;

		case SC_ASSUMPTIO:
			if (sc_data[SC_KYRIE].timer != -1)
				status_change_end(bl, SC_KYRIE, -1);
				break;
			*opt3 |= 2048;
			break;

		case SC_BASILICA:
			break;

		case SC_GOSPEL:
			if (val4 == BCT_SELF) {	// self effect
				int i;
				if (sd) {
					sd->canact_tick += tick;
					sd->canmove_tick += tick;
				}
				val2 = tick;
				tick = 1000;
				for (i=0; i<=26; i++) {
					if(sc_data[i].timer!=-1)
						status_change_end(bl,i,-1);
				}
				for (i=58; i<=62; i++) {
					if(sc_data[i].timer!=-1)
						status_change_end(bl,i,-1);
				}
				for (i=132; i<=136; i++) {
					if(sc_data[i].timer!=-1)
						status_change_end(bl,i,-1);
				}
			}
			break;

		case SC_MARIONETTE:
		case SC_MARIONETTE2:
			val2 = tick;
			if (!val3)
				return 0;
			tick = 1000;
			calc_flag = 1;
			*opt3 |= 1024;
			break;

		case SC_MELTDOWN:
		case SC_CARTBOOST:
		case SC_TRUESIGHT:
		case SC_SPIDERWEB:
			calc_flag = 1;
			break;

		case SC_DOUBLECASTING:
			break;

		case SC_REJECTSWORD:
			val2 = 3;
			val3 = 0; // Damage reflect state - [Aalye]
			break;

		case SC_MEMORIZE:
			val2 = 5;
			break;

		case SC_GRAVITATION:
			if (val3 != BCT_SELF)
				calc_flag = 1;
			break;

		case SC_HERMODE:
			status_change_clear_buffs(bl);
			break;

		case SC_COMA:
			battle_damage(NULL, bl, status_get_hp(bl)-1, 0);
			break;

		case SC_SPLASHER:	
			break;

		case SC_FOGWALL:
			val2 = 75;
			// calc_flag = 1;	// not sure of effects yet [celest]
			break;

		case SC_PRESERVE:
			break;

		case SC_BLEEDING:
			val4 = tick;
			tick = 10000;
			break;

		case SC_SLOWDOWN:
		case SC_SPEEDUP0:
		case SC_INCSTR:
		case SC_INCMATKRATE:
		case SC_INCATKRATE:
		case SC_INCDEFRATE:
		case SC_INCHITRATE:
		case SC_INCFLEERATE:
			calc_flag = 1;
			break;

		case SC_REGENERATION:
			val1 = 2;
		case SC_BATTLEORDERS:
			if(!(flag&4))
				tick = 60000; // 1 minute
			calc_flag = 1;
			break;

		case SC_GUILDAURA:
			if(!(flag&4))
				tick = 1000;
			calc_flag = 1;
			break;

		default:
			if (battle_config.error_log)
				printf("UnknownStatusChange [%d].\n", type);
			return 0;
	}

	if (bl->type == BL_PC && ((type < SC_MAX && (type != SC_HALLUCINATION || battle_config.display_hallucination)) || type == SC_PRESERVE || type == SC_BATTLEORDERS)) {
#ifdef USE_SQL
		if (flag&4)
			clif_status_load(bl,StatusIconChangeTable[type],1); //Sending to owner since they aren't in the map yet. [Skotlex]
#endif
		
		clif_status_change(bl, StatusIconChangeTable[type], 1);
	}

	switch(type){
		case SC_STONE:
		case SC_FREEZE:
		case SC_STAN:
		case SC_SLEEP:
			battle_stopattack(bl);
			skill_stop_dancing(bl,0);
			{
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
			opt_flag = 1;
			break;
		case SC_POISON:
		case SC_CURSE:
		case SC_SILENCE:
		case SC_BLIND:
			*opt2 |= 1<<(type-SC_POISON);
			opt_flag = 1;
			break;
		case SC_DPOISON:
			*opt2 |= 1;
			opt_flag = 1;
			break;
		case SC_SIGNUMCRUCIS:
			*opt2 |= 0x40;
			opt_flag = 1;
			break;
		case SC_MAXOVERTHRUST:
			*opt3 |= 2;
			opt_flag = 0;
			break;
		case SC_HIDING:
		case SC_CLOAKING:
			battle_stopattack(bl);
			*option |= ((type == SC_HIDING) ? 2: 4);
			opt_flag = 1;
			break;
		case SC_CHASEWALK:
			battle_stopattack(bl);
			*option |= 16388;
			opt_flag = 1;
			break;
		case SC_SIGHT:
			*option |= 1;
			opt_flag = 1;
			break;
		case SC_RUWACH:
			*option |= 8192;
			opt_flag = 1;
			break;
		case SC_WEDDING:
			*option |= 4096;
			opt_flag = 1;
	}

	if (opt_flag)
		clif_changeoption(bl);

	(*sc_count)++;

	sc_data[type].val1 = val1;
	sc_data[type].val2 = val2;
	sc_data[type].val3 = val3;
	sc_data[type].val4 = val4;
	sc_data[type].timer = add_timer(gettick_cache + tick, status_change_timer, bl->id, type);

	if (bl->type == BL_PC && calc_flag)
		status_calc_pc(sd,0);

	if (bl->type == BL_PC && save_flag)
		chrif_save(sd); // do pc_makesavestatus and save storage + account_reg/account_reg2 too

	if (bl->type == BL_PC && updateflag)
		clif_updatestatus(sd,updateflag);

	return 0;
}

/*==========================================
 * Status Change Clear
 *------------------------------------------
 */
int status_change_clear(struct block_list *bl, int type)
{
	struct status_change* sc_data;
	short *sc_count, *option, *opt1, *opt2, *opt3;
	int i;

	nullpo_retr(0, bl);
	nullpo_retr(0, sc_data = status_get_sc_data(bl));
	nullpo_retr(0, sc_count = status_get_sc_count(bl));
	nullpo_retr(0, option = status_get_option(bl));
	nullpo_retr(0, opt1 = status_get_opt1(bl));
	nullpo_retr(0, opt2 = status_get_opt2(bl));
	nullpo_retr(0, opt3 = status_get_opt3(bl));

	if (*sc_count == 0)
		return 0;
	for(i = 0; i < MAX_STATUSCHANGE; i++){
		if(sc_data[i].timer != -1){
			status_change_end(bl, i, -1);
		}
	}
	*sc_count = 0;
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

	return 0;
}

/*==========================================
 * Status Change End
 *------------------------------------------
 */
int status_change_end(struct block_list* bl, int type, int tid)
{
	struct status_change* sc_data;
	int opt_flag=0, calc_flag = 0;
	short *sc_count, *option, *opt1, *opt2, *opt3;

	nullpo_retr(0, bl);

	if(bl->type!=BL_PC && bl->type!=BL_MOB) {
		if(battle_config.error_log)
			printf("status_change_end: neither MOB nor PC !\n");
		return 0;
	}
	nullpo_retr(0, sc_data = status_get_sc_data(bl));
	nullpo_retr(0, sc_count = status_get_sc_count(bl));
	nullpo_retr(0, option = status_get_option(bl));
	nullpo_retr(0, opt1 = status_get_opt1(bl));
	nullpo_retr(0, opt2 = status_get_opt2(bl));
	nullpo_retr(0, opt3 = status_get_opt3(bl));

	if ((*sc_count) > 0 && sc_data[type].timer != -1 && (sc_data[type].timer == tid || tid == -1)) {

		if (tid == -1)
			delete_timer(sc_data[type].timer, status_change_timer);

		sc_data[type].timer = -1;
		(*sc_count)--;

		switch(type){
			case SC_PROVOKE:			/* プロボック */
			case SC_ENDURE:
			case SC_CONCENTRATE:		/* 集中力向上 */
			case SC_BLESSING:			/* ブレッシング */
			case SC_ANGELUS:			/* アンゼルス */
			case SC_INCREASEAGI:		/* 速度上昇 */
			case SC_DECREASEAGI:		/* 速度減少 */
			case SC_SIGNUMCRUCIS:		/* シグナムクルシス */
			case SC_HIDING:
			case SC_TWOHANDQUICKEN:		/* 2HQ */
			case SC_ADRENALINE:			/* アドレナリンラッシュ */
			case SC_ENCPOISON:			/* エンチャントポイズン */
			case SC_IMPOSITIO:			/* インポシティオマヌス */
			case SC_GLORIA:				/* グロリア */
			case SC_LOUD:				/* ラウドボイス */
			case SC_QUAGMIRE:			/* クァグマイア */
			case SC_PROVIDENCE:			/* プロヴィデンス */
			case SC_SPEARSQUICKEN:		/* スピアクイッケン */
			case SC_VOLCANO:
			case SC_DELUGE:
			case SC_VIOLENTGALE:
			case SC_ETERNALCHAOS:		/* エターナルカオス */
			case SC_DRUMBATTLE:			/* 戦太鼓の響き */
			case SC_NIBELUNGEN:			/* ニーベルングの指輪 */
			case SC_SIEGFRIED:			/* 不死身のジークフリード */
			case SC_WHISTLE:			/* 口笛 */
			case SC_ASSNCROS:			/* 夕陽のアサシンクロス */
			case SC_HUMMING:			/* ハミング */
			case SC_DONTFORGETME:		/* 私を忘れないで */
			case SC_FORTUNE:			/* 幸運のキス */
			case SC_SERVICE4U:
			case SC_EXPLOSIONSPIRITS:
			case SC_STEELBODY:
			case SC_DEFENDER:
			case SC_SPEEDPOTION0:
			case SC_SPEEDPOTION1:
			case SC_SPEEDPOTION2:
			case SC_SPEEDPOTION3:
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
			case SC_EDP:
			case SC_SLOWDOWN:
			case SC_SPEEDUP0:
			case SC_INCSTR:
			case SC_INCMATKRATE:
			case SC_INCHITRATE:
			case SC_INCATKRATE:
			case SC_INCDEFRATE:
			case SC_INCFLEERATE:
			case SC_BATTLEORDERS:
			case SC_REGENERATION:
			case SC_GUILDAURA:
				calc_flag = 1;
				break;
			case SC_EXTREMITYFIST:
				calc_flag = 1;
			case SC_AUTOBERSERK:
				if (sc_data[SC_PROVOKE].timer != -1)
					status_change_end(bl,SC_PROVOKE,-1);
				break;

			case SC_BERSERK:
				calc_flag = 1;
				clif_status_change(bl,SC_INCREASEAGI,0);
				break;

			case SC_DEVOTION:
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
					if(sc_data[type].val4 && (dsd = map_id2sd(sc_data[type].val4)))
					{
						d_sc_data = dsd->sc_data;
						if(d_sc_data && d_sc_data[type].timer != -1)
							d_sc_data[type].val4 = 0;
					}
				}
				if(sc_data[SC_LONGING].timer != -1)
					status_change_end(bl, SC_LONGING, -1);
				calc_flag = 1;
				break;

			case SC_NOCHAT:	//チャット禁止状態
				{
					struct map_session_data *sd;
					if (bl->type == BL_PC && (sd = (struct map_session_data *)bl)) {
						if (sd->status.manner >= 0)
							sd->status.manner = 0;
						clif_updatestatus(sd, SP_MANNER);
					}
				}
				break;

			case SC_SPLASHER:
				{
					struct block_list *src = map_id2bl(sc_data[type].val3);
					if(src && tid != -1)
						skill_castend_damage_id(src, bl, sc_data[type].val2, sc_data[type].val1, gettick_cache, 0);
				}
				break;

		/* option1 */
			case SC_FREEZE:
				sc_data[type].val3 = 0;
				break;

		/* option2 */
			case SC_POISON:				/* 毒 */
			case SC_BLIND:				/* 暗黒 */
			case SC_CURSE:
				calc_flag = 1;
				break;

			case SC_CONFUSION:
			  {
				struct map_session_data *sd;
				if (bl->type == BL_PC && (sd = (struct map_session_data *)bl)) {
					sd->next_walktime = -1;
				}
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

			case SC_MARIONETTE:
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
			}

		if (bl->type == BL_PC && (type < SC_MAX || type == SC_PRESERVE || type == SC_BATTLEORDERS))
			clif_status_change(bl, StatusIconChangeTable[type], 0);

		switch(type){
		case SC_STONE:
		case SC_FREEZE:
		case SC_STAN:
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
		case SC_BLIND:
			*opt2 &= ~(1<<(type-SC_POISON));
			opt_flag = 1;
			break;
		case SC_DPOISON:
			if (sc_data[SC_POISON].timer != -1)
				break;
			*opt2 &= ~1;
			opt_flag = 1;
			break;
		case SC_SIGNUMCRUCIS:
			*opt2 &= ~0x40;
			opt_flag = 1;
			break;

		case SC_HIDING:
		case SC_CLOAKING:
			*option &= ~((type == SC_HIDING) ? 2 : 4);
			calc_flag = 1;
			opt_flag = 1;
			break;

		case SC_CHASEWALK:
			*option &= ~16388;
			opt_flag = 1;
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

		//opt3
		case SC_TWOHANDQUICKEN:
		case SC_SPEARSQUICKEN:
		case SC_CONCENTRATION:
			*opt3 &= ~1;
			break;
		case SC_OVERTHRUST:
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
		case SC_MARIONETTE:
		case SC_MARIONETTE2:
			*opt3 &= ~1024;
			break;
		case SC_ASSUMPTIO:
			*opt3 &= ~2048;
			break;
		}

		if (opt_flag)
			clif_changeoption(bl);

		if (bl->type == BL_PC && calc_flag)
			status_calc_pc((struct map_session_data *)bl, 0);
	}

	return 0;
}

/*==========================================
 * Status Change Timer
 *------------------------------------------
 */
TIMER_FUNC(status_change_timer) {
	int type=data;
	struct block_list *bl;
	struct map_session_data *sd=NULL;
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
				sd->status.sp -= sp; // update sp cost [Celest]
				clif_updatestatus(sd, SP_SP);
				if ((++sc_data[SC_CHASEWALK].val4) == 1) {
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
				if(sc_data[type].val2 % (sc_data[type].val1 + 3) == 0) {
					sd->status.sp--;
					clif_updatestatus(sd, SP_SP);
				}
				sc_data[type].timer = add_timer(1000 + tick, status_change_timer, bl->id, data);
				return 0;
			}
		}
		break;

	case SC_SIGHT:
	case SC_RUWACH:
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

	case SC_TENSIONRELAX:
		if (sd) {
			if (sd->status.sp > 12 && sd->status.max_hp > sd->status.hp) {
				sc_data[type].timer = add_timer(10000 + tick, status_change_timer, bl->id, data);
				return 0;
			}
			if(sd->status.max_hp <= sd->status.hp)
				status_change_end(&sd->bl,SC_TENSIONRELAX,-1);
		}
		break;

	case SC_BLEEDING:
		if ((sc_data[type].val3 -= 1000) > 0) {
			if ((sc_data[type].val4 -= 1000) > 0) {
				int hp = rand() % 300 + 400;
				if (sd) {
					pc_heal(sd, -hp, 0);
					sd->canmove_tick = tick + 1000;
				} else if (bl->type == BL_MOB) {
					struct mob_data *md;
					if ((md = ((struct mob_data *)bl)) == NULL) // not: nullpo_retr(0, md = (struct mob_data *)bl); --> if mob is not more here, it's not an error (not display a message)
						break;
					md->hp -= hp;
				}
			}
			if (sd) {
				sd->canact_tick = tick + 1000;
			}
			sc_data[type].timer = add_timer(1000 + tick, status_change_timer, bl->id, data);
		}
		break;

	case SC_AETERNA:
	case SC_TRICKDEAD:
	case SC_RIDING:
	case SC_FALCON:
	case SC_WEIGHT50:
	case SC_WEIGHT90:
	case SC_MAGICPOWER:
	case SC_REJECTSWORD:
	case SC_MEMORIZE:
	case SC_BROKNWEAPON:
	case SC_BROKNARMOR:
	case SC_SACRIFICE:
		sc_data[type].timer = add_timer(1000 * 600 + tick, status_change_timer, bl->id, data);
		return 0;

	case SC_DANCING:
		{
			int s = 0, sp = 1;
			if(sd){
				if(sd->status.sp > 0 && (--sc_data[type].val3)>0){
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
					if(s && ((sc_data[type].val3 % s) == 0))
					{
						if(sc_data[SC_LONGING].timer != -1)
							sd->status.sp -= 3;
						else
							sd->status.sp -= sp;
						if (sd->status.sp < 0) sd->status.sp = 0;
						clif_updatestatus(sd, SP_SP);
					}
					sc_data[type].timer = add_timer(1000 + tick, status_change_timer, bl->id, data); 
					return 0;
				}
			}
		}
		break;
	case SC_BERSERK:
		if (sd) {
			if ((sd->status.hp - sd->status.max_hp * 5 / 100) > 100 ) { // 5% every 10 seconds [DracoRPG]
				sd->status.hp -= sd->status.max_hp * 5 / 100; // changed to max hp [celest]
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
	case SC_NOCHAT: // NoChat, fixed by MagicalTux
		if (sd && battle_config.muting_players) {
			time_t timer;
			if (sd->status.manner) {
				if (time(&timer) < ((sc_data[type].val2) + 60 * (0 - sd->status.manner))) { // is the player still muted ?
					sc_data[type].val2 += 60; // should match with current time
					sd->status.manner++; // increment value...
					clif_updatestatus(sd, SP_MANNER); // send remailing minutes to the client
					if (sd->status.manner < 0) sc_data[type].timer = add_timer(60000 + tick, status_change_timer, bl->id, data); // if the player is still muted, schedule another call to this function
					return 0;
				} else {
					sd->status.manner = 0; // player should not be muted anymore
					clif_updatestatus(sd, SP_MANNER);
				}
			}
		}
		break;

	case SC_SPLASHER:
		if(sc_data[type].val4 % 1000 == 0)
		{
			char timer[2];
			sprintf(timer, "%d", (int)(sc_data[type].val4 / 1000));
			clif_message(bl, timer);
		}
		if((sc_data[type].val4 -= 500) > 0)
		{
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
	  {
		int i = 3000;
		//struct mob_data *md;
		if (sd) {
			pc_randomwalk(sd);
			sd->next_walktime = tick + (i = 1000 + rand() % 1000);
		}
		if ((sc_data[type].val2 -= 1000) > 0) {
			sc_data[type].timer = add_timer(i + tick, status_change_timer, bl->id, data);
			return 0;
		}
	  }
		break;

	case SC_GOSPEL:
		{
			int calc_flag = 0;
			if (sc_data[type].val3 > 0) {
				sc_data[type].val3 = 0;
				calc_flag = 1;
			}
			if(sd && sc_data[type].val4 == BCT_SELF){
				int hp, sp;
				hp = (sc_data[type].val1 > 5) ? 45 : 30;
				sp = (sc_data[type].val1 > 5) ? 35 : 20;
				if(sd->status.hp - hp > 0 &&
					sd->status.sp - sp > 0){
					sd->status.hp -= hp;
					sd->status.sp -= sp;
					clif_updatestatus(sd,SP_HP);
					clif_updatestatus(sd,SP_SP);
					if ((sc_data[type].val2 -= 10000) > 0) {
						sc_data[type].timer = add_timer(
							10000+tick, status_change_timer,
							bl->id, data);
						return 0;
					}
				}
			} else if (sd && sc_data[type].val4 == BCT_PARTY) {
				int i;
				switch ((i = rand() % 12)) {
				case 1: // heal between 100-1000
					{
						struct block_list tbl;
						int heal = rand() % 900 + 100;
						tbl.id = 0;
						tbl.m = bl->m;
						tbl.x = bl->x;
						tbl.y = bl->y;
						clif_skill_nodamage(&tbl,bl,AL_HEAL,heal,1);
						battle_heal(NULL,bl,heal,0,0);
					}
					break;
				case 2: // end negative status
					{
						int j;
						for (j=0; j<4; j++)
							if(sc_data[i + SC_POISON].timer!=-1) {
								status_change_end(bl,j,-1);
								break;
							}
					}
					break;
				case 3:	// +25% resistance to negative status
				case 4: // +25% max hp
				case 5: // +25% max sp
				case 6: // +2 to all stats
				case 11: // +25% armor and vit def
				case 12: // +8% atk
				case 13: // +5% flee
				case 14: // +5% hit
					sc_data[type].val3 = i;
					if (i == 6 ||
						(i >= 11 && i <= 14))
						calc_flag = 1;
					break;
				case 7: // level 5 bless
					{
						struct block_list tbl;
						tbl.id = 0;
						tbl.m = bl->m;
						tbl.x = bl->x;
						tbl.y = bl->y;
						clif_skill_nodamage(&tbl,bl,AL_BLESSING,5,1);
						status_change_start(bl,SkillStatusChangeTable[AL_BLESSING],5,0,0,0,10000,0 );
					}
					break;
				case 8: // level 5 increase agility
					{
						struct block_list tbl;
						tbl.id = 0;
						tbl.m = bl->m;
						tbl.x = bl->x;
						tbl.y = bl->y;
						clif_skill_nodamage(&tbl,bl,AL_INCAGI,5,1);
						status_change_start(bl,SkillStatusChangeTable[AL_INCAGI],5,0,0,0,10000,0 );
					}
					break;
				case 9: // holy element to weapon
					{
						struct block_list tbl;
						tbl.id = 0;
						tbl.m = bl->m;
						tbl.x = bl->x;
						tbl.y = bl->y;
						clif_skill_nodamage(&tbl,bl,PR_ASPERSIO,1,1);
						status_change_start(bl,SkillStatusChangeTable[PR_ASPERSIO],1,0,0,0,10000,0 );
					}
					break;
				case 10: // holy element to armour
					{
						struct block_list tbl;
						tbl.id = 0;
						tbl.m = bl->m;
						tbl.x = bl->x;
						tbl.y = bl->y;
						clif_skill_nodamage(&tbl,bl,PR_BENEDICTIO,1,1);
						status_change_start(bl,SkillStatusChangeTable[PR_BENEDICTIO],1,0,0,0,10000,0 );
					}
					break;
				default:
					break;
				}
			} else if (sc_data[type].val4 == BCT_ENEMY) {
				int i;
				switch ((i = rand() % 8)) {
				case 1: // damage between 300-800
				case 2: // damage between 150-550 (ignore def)
					battle_damage(NULL, bl, rand() % 500,0); // temporary damage
					break;
				case 3: // random status effect
					{
						int effect[3] = {
							SC_CURSE,
							SC_BLIND,
							SC_POISON };
						status_change_start(bl,effect[rand()%3],1,0,0,0,10000,0 );
					}
					break;
				case 4: // level 10 provoke
					{
						struct block_list tbl;
						tbl.id = 0;
						tbl.m = bl->m;
						tbl.x = bl->x;
						tbl.y = bl->y;
						clif_skill_nodamage(&tbl,bl,SM_PROVOKE,1,1);
						status_change_start(bl,SkillStatusChangeTable[SM_PROVOKE],10,0,0,0,10000,0 );
					}
					break;
				case 5: // 0 def
				case 6: // 0 atk
				case 7: // 0 flee
				case 8: // -75% move speed and aspd
					sc_data[type].val3 = i;
					calc_flag = 1;
					break;
				default:
					break;
				}
			}
			if (sd && calc_flag)
				status_calc_pc(sd, 0);
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
 * Status Change Timer (Sub)
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
	case SC_SIGHT: /* サイト */
	case SC_CONCENTRATE:
		if ((*status_get_option(bl)) & 6) {
			status_change_end(bl, SC_HIDING, -1);
			status_change_end(bl, SC_CLOAKING, -1);
		}
		break;
	case SC_RUWACH: /* ルアフ */
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
	}

	return 0;
}

int status_change_clear_buffs (struct block_list *bl){
	int i;
	struct status_change *sc_data = status_get_sc_data(bl);
	if (!sc_data)
		return 0;

	for (i = 0; i < SC_MAX; i++) {
		if(i == SC_HALLUCINATION || i == SC_WEIGHT50 || i == SC_WEIGHT90
			|| i == SC_QUAGMIRE || i == SC_SIGNUMCRUCIS || i == SC_DECREASEAGI 
			|| i == SC_SLOWDOWN || i == SC_ANKLE|| i == SC_BLADESTOP
			|| i == SC_MINDBREAKER || i == SC_STOP || i == SC_NOCHAT
			|| i == SC_STRIPWEAPON || i == SC_STRIPSHIELD || i == SC_STRIPARMOR || i == SC_STRIPHELM
			|| i == SC_COMBO || i == SC_DANCING || i == SC_GUILDAURA)
			continue;
		if(sc_data[i].timer != -1)
			status_change_end(bl,i,-1);
	}
	return 0;
}

int status_change_clear_debuffs (struct block_list *bl){
	int i;
	struct status_change *sc_data = status_get_sc_data(bl);
	if (!sc_data)
		return 0;
	for (i = SC_STONE; i <= SC_MAX; i++) {
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

	for(i=0;i<MAX_PC_CLASS;i++) {
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
		for(j=0;j<=22;j++)
			aspd_base[i][j] = atoi(split[j+4]);
		i++;
		if (i == 26)
			i = 4046;
		if (i == MAX_PC_CLASS)
			break;
	}
	fclose(fp);
	printf(CL_WHITE "Status: " CL_RESET " '" CL_WHITE "db/job_db1.txt" CL_RESET "' read.\n");

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
			job_bonus[2][i][j]=k;
			p=strchr(p,',');
			if(p) p++;
		}
		i++;

		if (i == 26)
			i = 4046;
		if (i == MAX_PC_CLASS)
			break;
	}
	fclose(fp);
	printf(CL_WHITE "Status: " CL_RESET " '" CL_WHITE "db/job_db2.txt" CL_RESET "' read.\n");

	fp = fopen("db/job_db2-2.txt","r");
	if (fp == NULL) {
		printf("can't read db/job_db2-2.txt\n");
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
			job_bonus[1][i][j]=k;
			p=strchr(p,',');
			if(p) p++;
		}
		i++;
		if (i == 26)
			i = 4046;
		if (i == MAX_PC_CLASS)
			break;
	}
	fclose(fp);
	printf(CL_WHITE "Status: " CL_RESET " '" CL_WHITE "db/job_db2-2.txt" CL_RESET "' read.\n");

	for(i=0;i<3;i++)
		for(j = 0; j <= 22; j++)
			atkmods[i][j] = 100;
	fp=fopen("db/size_fix.txt","r");
	if(fp==NULL){
		printf("can't read db/size_fix.txt\n");
		return 1;
	}
	i=0;
	while(fgets(line, sizeof(line), fp)) { // fgets reads until maximum one less than size and add '\0' -> so, it's not necessary to add -1
		char *split[25];
		if ((line[0] == '/' && line[1] == '/') || line[0] == '\0' || line[0] == '\n' || line[0] == '\r')
			continue;
		// it's not necessary to remove 'carriage return ('\n' or '\r')
		if(atoi(line)<=0)
			continue;
		memset(split,0,sizeof(split));
		for(j = 0, p = line; j <= 22 && p; j++) {
			split[j] = p;
			p = strchr(p, ',');
			if (p) *p++ = 0;
		}
		for(j = 0; j <= 22 && split[j]; j++)
			atkmods[i][j] = atoi(split[j]);
		i++;
	}
	fclose(fp);
	printf(CL_WHITE "Status: " CL_RESET " '" CL_WHITE "db/size_fix.txt" CL_RESET "' read ('" CL_WHITE "%d" CL_RESET "' entrie%s).\n", i, (i > 1) ? "s" : "");

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
	i = 0;
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
		refinebonus[i][0] = atoi(split[0]);
		refinebonus[i][1] = atoi(split[1]);
		refinebonus[i][2] = atoi(split[2]);
		for(j = 0; j < 10 && split[j]; j++)
			percentrefinery[i][j] = atoi(split[j+3]);
		i++;
	}
	fclose(fp);
	printf(CL_WHITE "Status: " CL_RESET " '" CL_WHITE "db/refine_db.txt" CL_RESET "' read ('" CL_WHITE "%d" CL_RESET "' entrie%s).\n", i, (i > 1) ? "s" : "");

	return 0;
}

/*==========================================
 * Load Tables
 *------------------------------------------
 */
int do_init_status(void) {
	add_timer_func_list(status_change_timer, "status_change_timer");
	initStatusIconChangeTable();
	status_readdb();
	status_calc_sigma();

	return 0;
}
