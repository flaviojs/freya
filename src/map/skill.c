// $Id: skill.c 577 2005-12-03 18:06:48Z Yor $
/* ƒXƒLƒ‹ŠÖŒW */

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../common/timer.h"
#include "../common/malloc.h"
#include "../common/utils.h"

#include "nullpo.h"
#include "skill.h"
#include "map.h"
#include "clif.h"
#include "pc.h"
#include "pet.h"
#include "mob.h"
#include "battle.h"
#include "party.h"
#include "itemdb.h"
#include "script.h"
#include "intif.h"
#include "chrif.h"
#include "guild.h"
#include "atcommand.h"
#include "grfio.h"
#include "status.h"

#ifdef MEMWATCH
#include "memwatch.h"
#endif

#define SKILLUNITTIMER_INVERVAL 100
#define STATE_BLIND 0x10
#define swap(x,y) { int t; t = x; x = y; y = t; }

const struct skill_name_db skill_names[] = {
 { AC_CHARGEARROW, "CHARGEARROW", "Charge_Arrow" } ,
 { AC_CONCENTRATION, "CONCENTRATION", "Improve_Concentration" } ,
 { AC_DOUBLE, "DOUBLE", "Double_Strafe" } ,
 { AC_MAKINGARROW, "MAKINGARROW", "Arrow_Creation" } ,
 { AC_OWL, "OWL", "Owl's_Eye" } ,
 { AC_SHOWER, "SHOWER", "Arrow_Shower" } ,
 { AC_VULTURE, "VULTURE", "Vulture's_Eye" } ,
 { ALL_RESURRECTION, "RESURRECTION", "Resurrection" } ,
 { AL_ANGELUS, "ANGELUS", "Angelus" } ,
 { AL_BLESSING, "BLESSING", "Blessing" } ,
 { AL_CRUCIS, "CRUCIS", "Signum_Crusis" } ,
 { AL_CURE, "CURE", "Cure" } ,
 { AL_DECAGI, "DECAGI", "Decrease_AGI" } ,
 { AL_DEMONBANE, "DEMONBANE", "Demon_Bane" } ,
 { AL_DP, "DP", "Divine_Protection" } ,
 { AL_HEAL, "HEAL", "Heal" } ,
 { AL_HOLYLIGHT, "HOLYLIGHT", "Holy_Light" } ,
 { AL_HOLYWATER, "HOLYWATER", "Aqua_Benedicta" } ,
 { AL_INCAGI, "INCAGI", "Increase_AGI" } ,
 { AL_PNEUMA, "PNEUMA", "Pneuma" } ,
 { AL_RUWACH, "RUWACH", "Ruwach" } ,
 { AL_TELEPORT, "TELEPORT", "Teleport" } ,
 { AL_WARP, "WARP", "Warp_Portal" } ,
 { AM_ACIDTERROR, "ACIDTERROR", "Acid_Terror" } ,
 { AM_AXEMASTERY, "AXEMASTERY", "Axe_Mastery" } ,
 { AM_BERSERKPITCHER, "BERSERKPITCHER", "Berserk Pitcher" } ,
 { AM_BIOETHICS, "BIOETHICS", "Bioethics" } ,
 { AM_BIOTECHNOLOGY, "BIOTECHNOLOGY", "Biotechnology" } ,
 { AM_CALLHOMUN, "CALLHOMUN", "Call_Homunculus" } ,
 { AM_CANNIBALIZE, "CANNIBALIZE", "Bio_Cannibalize" } ,
 { AM_CP_ARMOR, "ARMOR", "Chemical_Protection_Armor" } ,
 { AM_CP_HELM, "HELM", "Chemical_Protection_Helm" } ,
 { AM_CP_SHIELD, "SHIELD", "Chemical_Protection_Shield" } ,
 { AM_CP_WEAPON, "WEAPON", "Chemical_Protection_Weapon" } ,
 { AM_CREATECREATURE, "CREATECREATURE", "Life_Creation" } ,
 { AM_CULTIVATION, "CULTIVATION", "Cultivation" } ,
 { AM_DEMONSTRATION, "DEMONSTRATION", "Demonstration" } ,
 { AM_DRILLMASTER, "DRILLMASTER", "Drillmaster" } ,
 { AM_FLAMECONTROL, "FLAMECONTROL", "Flame_Control" } ,
 { AM_HEALHOMUN, "HEALHOMUN", "Heal_Homunculus" } ,
 { AM_LEARNINGPOTION, "LEARNINGPOTION", "AM_LEARNINGPOTION" } ,
 { AM_PHARMACY, "PHARMACY", "Pharmacy" } ,
 { AM_POTIONPITCHER, "POTIONPITCHER", "Potion_Pitcher" } ,
 { AM_REST, "REST", "Sabbath" } ,
 { AM_RESURRECTHOMUN, "RESURRECTHOMUN", "Ressurect_Homunculus" } ,
 { AM_SPHEREMINE, "SPHEREMINE", "Sphere_Mine" } ,
 { ASC_BREAKER, "BREAKER", "Breaker" } ,
 { ASC_CDP, "CDP", "Create_Deadly_Poison" } ,
 { ASC_EDP, "EDP", "Deadly_Poison_Enchantment" } ,
 { ASC_HALLUCINATION, "HALLUCINATION", "Hallucination_Walk" } ,
 { ASC_KATAR, "KATAR", "Advanced_Katar_Mastery" } ,
 { ASC_METEORASSAULT, "METEORASSAULT", "Meteor_Assault" } ,
 { AS_CLOAKING, "CLOAKING", "Cloaking" } ,
 { AS_ENCHANTPOISON, "ENCHANTPOISON", "Enchant_Poison" } ,
 { AS_GRIMTOOTH, "GRIMTOOTH", "Grimtooth" } ,
 { AS_KATAR, "KATAR", "Katar_Mastery" } ,
 { AS_LEFT, "LEFT", "Lefthand_Mastery" } ,
 { AS_POISONREACT, "POISONREACT", "Poison_React" } ,
 { AS_RIGHT, "RIGHT", "Righthand_Mastery" } ,
 { AS_SONICBLOW, "SONICBLOW", "Sonic_Blow" } ,
 { AS_SPLASHER, "SPLASHER", "Venom_Splasher" } ,
 { AS_VENOMDUST, "VENOMDUST", "Venom_Dust" } ,
 { BA_APPLEIDUN, "APPLEIDUN", "Apple_of_Idun" } ,
 { BA_ASSASSINCROSS, "ASSASSINCROSS", "Assassin_Cross" } ,
 { BA_DISSONANCE, "DISSONANCE", "Dissonance" } ,
 { BA_FROSTJOKE, "FROSTJOKE", "Dumb_Joke" } ,
 { BA_MUSICALLESSON, "MUSICALLESSON", "Musical_Lesson" } ,
 { BA_MUSICALSTRIKE, "MUSICALSTRIKE", "Musical_Strike" } ,
 { BA_POEMBRAGI, "POEMBRAGI", "Poem_of_Bragi" } ,
 { BA_WHISTLE, "WHISTLE", "Whistle" } ,
 { BD_ADAPTATION, "ADAPTATION", "Adaption" } ,
 { BD_DRUMBATTLEFIELD, "DRUMBATTLEFIELD", "Drumb_BattleField" } ,
 { BD_ENCORE, "ENCORE", "Encore" } ,
 { BD_ETERNALCHAOS, "ETERNALCHAOS", "Eternal_Chaos" } ,
 { BD_INTOABYSS, "INTOABYSS", "Into_the_Abyss" } ,
 { BD_LULLABY, "LULLABY", "Lullaby" } ,
 { BD_RAGNAROK, "RAGNAROK", "Ragnarok" } ,
 { BD_RICHMANKIM, "RICHMANKIM", "Rich_Mankim" } ,
 { BD_RINGNIBELUNGEN, "RINGNIBELUNGEN", "Ring_of_Nibelugen" } ,
 { BD_ROKISWEIL, "ROKISWEIL", "Loki's_Wail" } ,
 { BD_SIEGFRIED, "SIEGFRIED", "Invulnerable_Siegfried" } ,
 { BS_ADRENALINE, "ADRENALINE", "Adrenaline_Rush" } ,
 { BS_ADRENALINE2, "ADRENALINE2", "Adrenaline Rush 2" } ,
 { BS_AXE, "AXE", "Smith_Axe" } ,
 { BS_DAGGER, "DAGGER", "Smith_Dagger" } ,
 { BS_ENCHANTEDSTONE, "ENCHANTEDSTONE", "Enchantedstone_Craft" } ,
 { BS_FINDINGORE, "FINDINGORE", "Ore_Discovery" } ,
 { BS_HAMMERFALL, "HAMMERFALL", "Hammer_Fall" } ,
 { BS_HILTBINDING, "HILTBINDING", "Hilt_Binding" } ,
 { BS_IRON, "IRON", "Iron_Tempering" } ,
 { BS_KNUCKLE, "KNUCKLE", "Smith_Knucklebrace" } ,
 { BS_MACE, "MACE", "Smith_Mace" } ,
 { BS_MAXIMIZE, "MAXIMIZE", "Power_Maximize" } ,
 { BS_ORIDEOCON, "ORIDEOCON", "Orideocon_Research" } ,
 { BS_OVERTHRUST, "OVERTHRUST", "Power-Thrust" } ,
 { BS_REPAIRWEAPON, "REPAIRWEAPON", "Weapon_Repair" } ,
 { BS_SKINTEMPER, "SKINTEMPER", "Skin_Tempering" } ,
 { BS_SPEAR, "SPEAR", "Smith_Spear" } ,
 { BS_STEEL, "STEEL", "Steel_Tempering" } ,
 { BS_SWORD, "SWORD", "Smith_Sword" } ,
 { BS_TWOHANDSWORD, "TWOHANDSWORD", "Smith_Two-handed_Sword" } ,
 { BS_WEAPONPERFECT, "WEAPONPERFECT", "Weapon_Perfection" } ,
 { BS_WEAPONRESEARCH, "WEAPONRESEARCH", "Weaponry_Research" } ,
 { CG_ARROWVULCAN, "ARROWVULCAN", "Vulcan_Arrow" } ,
 { CG_MARIONETTE, "MARIONETTE", "Marionette_Control" } ,
 { CG_MOONLIT, "MOONLIT", "Moonlight_Petals" } ,
 { CH_CHAINCRUSH, "CHAINCRUSH", "Chain_Crush_Combo" } ,
 { CH_PALMSTRIKE, "PALMSTRIKE", "Palm_Push_Strike" } ,
 { CH_SOULCOLLECT, "SOULCOLLECT", "Collect_Soul" } ,
 { CH_TIGERFIST, "TIGERFIST", "Tiger_Knuckle_Fist" } ,
 { CR_ACIDDEMONSTRATION, "CR_ACIDDEMONSTRATION", "Acid_Demonstration" } ,
 { CR_ALCHEMY, "ALCHEMY", "Alchemy" } ,
 { CR_SLIMPITCHER, "SLIMPITCHER", "Slim_Pitcher" } ,
 { CR_FULLPROTECTION, "FULLPROTECTION", "Full_Chemical_Protection" } ,
 { CR_AUTOGUARD, "AUTOGUARD", "Guard" } ,
 { CR_DEFENDER, "DEFENDER", "Defender" } ,
 { CR_DEVOTION, "DEVOTION", "Sacrifice" } ,
 { CR_GRANDCROSS, "GRANDCROSS", "Grand_Cross" } ,
 { CR_HOLYCROSS, "HOLYCROSS", "Holy_Cross" } ,
 { CR_PROVIDENCE, "PROVIDENCE", "Providence" } ,
 { CR_REFLECTSHIELD, "REFLECTSHIELD", "Shield_Reflect" } ,
 { CR_SHIELDBOOMERANG, "SHIELDBOOMERANG", "Shield_Boomerang" } ,
 { CR_SHIELDCHARGE, "SHIELDCHARGE", "Shield_Charge" } ,
 { CR_SPEARQUICKEN, "SPEARQUICKEN", "Spear_Quicken" } ,
 { CR_SYNTHESISPOTION, "SYNTHESISPOTION", "Potion_Synthesis" } ,
 { CR_TRUST, "TRUST", "Faith" } ,
 { DC_DANCINGLESSON, "DANCINGLESSON", "Dancing_Lesson" } ,
 { DC_DONTFORGETME, "DONTFORGETME", "Don't_Forget_Me" } ,
 { DC_FORTUNEKISS, "FORTUNEKISS", "Fortune_Kiss" } ,
 { DC_HUMMING, "HUMMING", "Humming" } ,
 { DC_SCREAM, "SCREAM", "Scream" } ,
 { DC_SERVICEFORYOU, "SERVICEFORYOU", "Prostitute" } ,
 { DC_THROWARROW, "THROWARROW", "Throw_Arrow" } ,
 { DC_UGLYDANCE, "UGLYDANCE", "Ugly_Dance" } ,
 { GD_BATTLEORDER, "BATTLEORDER", "Battle_Orders" } ,
 { GD_REGENERATION, "REGENERATION", "Regeneration" } ,
 { GD_RESTORE, "RESTORE", "Restore" } ,
 { GD_EMERGENCYCALL, "EMERGENCYCALL", "Emergency_Call" } ,
 { HP_ASSUMPTIO, "ASSUMPTIO", "Assumptio" } ,
 { HP_BASILICA, "BASILICA", "Basilica" } ,
 { HP_MEDITATIO, "MEDITATIO", "Meditation" } ,
 { HT_ANKLESNARE, "ANKLESNARE", "Ankle_Snare" } ,
 { HT_BEASTBANE, "BEASTBANE", "Beast_Bane" } ,
 { HT_BLASTMINE, "BLASTMINE", "Blast_Mine" } ,
 { HT_BLITZBEAT, "BLITZBEAT", "Blitz_Beat" } ,
 { HT_CLAYMORETRAP, "CLAYMORETRAP", "Claymore_Trap" } ,
 { HT_DETECTING, "DETECTING", "Detect" } ,
 { HT_FALCON, "FALCON", "Falconry_Mastery" } ,
 { HT_FLASHER, "FLASHER", "Flasher" } ,
 { HT_FREEZINGTRAP, "FREEZINGTRAP", "Freezing_Trap" } ,
 { HT_LANDMINE, "LANDMINE", "Land_Mine" } ,
 { HT_REMOVETRAP, "REMOVETRAP", "Remove_Trap" } ,
 { HT_SANDMAN, "SANDMAN", "Sandman" } ,
 { HT_SHOCKWAVE, "SHOCKWAVE", "Shockwave_Trap" } ,
 { HT_SKIDTRAP, "SKIDTRAP", "Skid_Trap" } ,
 { HT_SPRINGTRAP, "SPRINGTRAP", "Spring_Trap" } ,
 { HT_STEELCROW, "STEELCROW", "Steel_Crow" } ,
 { HT_TALKIEBOX, "TALKIEBOX", "Talkie_Box" } ,
 { HW_MAGICCRASHER, "MAGICCRASHER", "Magic_Crasher" } ,
 { HW_MAGICPOWER, "MAGICPOWER", "Magic_Power" } ,
 { HW_NAPALMVULCAN, "NAPALMVULCAN", "Napalm_Vulcan" } ,
 { HW_SOULDRAIN, "SOULDRAIN", "Soul_Drain" } ,
 { ITM_TOMAHAWK, "TOMAHAWK", "Throw_Tomahawk" } ,
 { KN_AUTOCOUNTER, "AUTOCOUNTER", "Counter_Attack" } ,
 { KN_BOWLINGBASH, "BOWLINGBASH", "Bowling_Bash" } ,
 { KN_BRANDISHSPEAR, "BRANDISHSPEAR", "Brandish_Spear" } ,
 { KN_CAVALIERMASTERY, "CAVALIERMASTERY", "Cavalier_Mastery" } ,
 { KN_PIERCE, "PIERCE", "Pierce" } ,
 { KN_RIDING, "RIDING", "Peco_Peco_Ride" } ,
 { KN_SPEARBOOMERANG, "SPEARBOOMERANG", "Spear_Boomerang" } ,
 { KN_SPEARMASTERY, "SPEARMASTERY", "Spear_Mastery" } ,
 { KN_SPEARSTAB, "SPEARSTAB", "Spear_Stab" } ,
 { KN_TWOHANDQUICKEN, "TWOHANDQUICKEN", "Twohand_Quicken" } ,
 { LK_AURABLADE, "AURABLADE", "Aura_Blade" } ,
 { LK_BERSERK, "BERSERK", "Berserk" } ,
 { LK_CONCENTRATION, "CONCENTRATION", "Concentration" } ,
 { LK_FURY, "FURY", "LK_FURY" } ,
 { LK_HEADCRUSH, "HEADCRUSH", "Head_Crusher" } ,
 { LK_JOINTBEAT, "JOINTBEAT", "Joint_Beat" } ,
 { LK_PARRYING, "PARRYING", "Parrying" } ,
 { LK_SPIRALPIERCE, "SPIRALPIERCE", "Spiral_Pierce" } ,
 { LK_TENSIONRELAX, "TENSIONRELAX", "Tension_Relax" } ,
 { MC_CARTREVOLUTION, "CARTREVOLUTION", "Cart_Revolution" } ,
 { MC_CHANGECART, "CHANGECART", "Change_Cart" } ,
 { MC_DISCOUNT, "DISCOUNT", "Discount" } ,
 { MC_IDENTIFY, "IDENTIFY", "Item_Appraisal" } ,
 { MC_INCCARRY, "INCCARRY", "Enlarge_Weight_Limit" } ,
 { MC_LOUD, "LOUD", "Lord_Exclamation" } ,
 { MC_MAMMONITE, "MAMMONITE", "Mammonite" } ,
 { MC_OVERCHARGE, "OVERCHARGE", "Overcharge" } ,
 { MC_PUSHCART, "PUSHCART", "Pushcart" } ,
 { MC_VENDING, "VENDING", "Vending" } ,
 { MG_COLDBOLT, "COLDBOLT", "Cold_Bolt" } ,
 { MG_ENERGYCOAT, "ENERGYCOAT", "Energy_Coat" } ,
 { MG_FIREBALL, "FIREBALL", "Fire_Ball" } ,
 { MG_FIREBOLT, "FIREBOLT", "Fire_Bolt" } ,
 { MG_FIREWALL, "FIREWALL", "Fire_Wall" } ,
 { MG_FROSTDIVER, "FROSTDIVER", "Frost_Diver" } ,
 { MG_LIGHTNINGBOLT, "LIGHTNINGBOLT", "Lightening_Bolt" } ,
 { MG_NAPALMBEAT, "NAPALMBEAT", "Napalm_Beat" } ,
 { MG_SAFETYWALL, "SAFETYWALL", "Safety_Wall" } ,
 { MG_SIGHT, "SIGHT", "Sight" } ,
 { MG_SOULSTRIKE, "SOULSTRIKE", "Soul_Strike" } ,
 { MG_SRECOVERY, "SRECOVERY", "Increase_SP_Recovery" } ,
 { MG_STONECURSE, "STONECURSE", "Stone_Curse" } ,
 { MG_THUNDERSTORM, "THUNDERSTORM", "Thunderstorm" } ,
 { MO_ABSORBSPIRITS, "ABSORBSPIRITS", "Absorb_Spirits" } ,
 { MO_BLADESTOP, "BLADESTOP", "Blade_Stop" } ,
 { MO_BODYRELOCATION, "BODYRELOCATION", "Body_Relocation" } ,
 { MO_CALLSPIRITS, "CALLSPIRITS", "Call_Spirits" } ,
 { MO_CHAINCOMBO, "CHAINCOMBO", "Chain_Combo" } ,
 { MO_COMBOFINISH, "COMBOFINISH", "Combo_Finish" } ,
 { MO_DODGE, "DODGE", "Dodge" } ,
 { MO_EXPLOSIONSPIRITS, "EXPLOSIONSPIRITS", "Explosion_Spirits" } ,
 { MO_EXTREMITYFIST, "EXTREMITYFIST", "Extremity_Fist" } ,
 { MO_FINGEROFFENSIVE, "FINGEROFFENSIVE", "Finger_Offensive" } ,
 { MO_INVESTIGATE, "INVESTIGATE", "Investigate" } ,
 { MO_IRONHAND, "IRONHAND", "Iron_Hand" } ,
 { MO_SPIRITSRECOVERY, "SPIRITSRECOVERY", "Spirit_Recovery" } ,
 { MO_STEELBODY, "STEELBODY", "Steel_Body" } ,
 { MO_TRIPLEATTACK, "TRIPLEATTACK", "Triple_Blows" } ,
 { NPC_ATTRICHANGE, "ATTRICHANGE", "NPC_ATTRICHANGE" } ,
 { NPC_BARRIER, "BARRIER", "NPC_BARRIER" } ,
 { NPC_BLINDATTACK, "BLINDATTACK", "NPC_BLINDATTACK" } ,
 { NPC_BLOODDRAIN, "BLOODDRAIN", "NPC_BLOODDRAIN" } ,
 { NPC_CHANGEDARKNESS, "CHANGEDARKNESS", "NPC_CHANGEDARKNESS" } ,
 { NPC_CHANGEFIRE, "CHANGEFIRE", "NPC_CHANGEFIRE" } ,
 { NPC_CHANGEGROUND, "CHANGEGROUND", "NPC_CHANGEGROUND" } ,
 { NPC_CHANGEHOLY, "CHANGEHOLY", "NPC_CHANGEHOLY" } ,
 { NPC_CHANGEPOISON, "CHANGEPOISON", "NPC_CHANGEPOISON" } ,
 { NPC_CHANGETELEKINESIS, "CHANGETELEKINESIS", "NPC_CHANGETELEKINESIS" } ,
 { NPC_CHANGEWATER, "CHANGEWATER", "NPC_CHANGEWATER" } ,
 { NPC_CHANGEWIND, "CHANGEWIND", "NPC_CHANGEWIND" } ,
 { NPC_COMBOATTACK, "COMBOATTACK", "NPC_COMBOATTACK" } ,
 { NPC_CRITICALSLASH, "CRITICALSLASH", "NPC_CRITICALSLASH" } ,
 { NPC_CURSEATTACK, "CURSEATTACK", "NPC_CURSEATTACK" } ,
 { NPC_DARKBLESSING, "DARKBLESSING", "NPC_DARKBLESSING" } ,
 { NPC_DARKBREATH, "DARKBREATH", "NPC_DARKBREATH" } ,
 { NPC_DARKCROSS, "DARKCROSS", "NPC_DARKCROSS" } ,
 { NPC_DARKNESSATTACK, "DARKNESSATTACK", "NPC_DARKNESSATTACK" } ,
 { NPC_DEFENDER, "DEFENDER", "NPC_DEFENDER" } ,
 { NPC_EMOTION, "EMOTION", "NPC_EMOTION" } ,
 { NPC_ENERGYDRAIN, "ENERGYDRAIN", "NPC_ENERGYDRAIN" } ,
 { NPC_FIREATTACK, "FIREATTACK", "NPC_FIREATTACK" } ,
 { NPC_GROUNDATTACK, "GROUNDATTACK", "NPC_GROUNDATTACK" } ,
 { NPC_GUIDEDATTACK, "GUIDEDATTACK", "NPC_GUIDEDATTACK" } ,
 { NPC_HALLUCINATION, "HALLUCINATION", "NPC_HALLUCINATION" } ,
 { NPC_HOLYATTACK, "HOLYATTACK", "NPC_HOLYATTACK" } ,
 { NPC_KEEPING, "KEEPING", "NPC_KEEPING" } ,
 { NPC_LICK, "LICK", "NPC_LICK" } ,
 { NPC_MAGICALATTACK, "MAGICALATTACK", "NPC_MAGICALATTACK" } ,
 { NPC_MENTALBREAKER, "MENTALBREAKER", "NPC_MENTALBREAKER" } ,
 { NPC_METAMORPHOSIS, "METAMORPHOSIS", "NPC_METAMORPHOSIS" } ,
 { NPC_PETRIFYATTACK, "PETRIFYATTACK", "NPC_PETRIFYATTACK" } ,
 { NPC_PIERCINGATT, "PIERCINGATT", "NPC_PIERCINGATT" } ,
 { NPC_POISON, "POISON", "NPC_POISON" } ,
 { NPC_POISONATTACK, "POISONATTACK", "NPC_POISONATTACK" } ,
 { NPC_PROVOCATION, "PROVOCATION", "NPC_PROVOCATION" } ,
 { NPC_RANDOMATTACK, "RANDOMATTACK", "NPC_RANDOMATTACK" } ,
 { NPC_RANGEATTACK, "RANGEATTACK", "NPC_RANGEATTACK" } ,
 { NPC_REBIRTH, "REBIRTH", "NPC_REBIRTH" } ,
 { NPC_SELFDESTRUCTION, "SELFDESTRUCTION", "Kabooooom!" } ,
 { NPC_SELFDESTRUCTION2, "SELFDESTRUCTION2", "NPC_SELFDESTRUCTION2" } ,
 { NPC_SILENCEATTACK, "SILENCEATTACK", "NPC_SILENCEATTACK" } ,
 { NPC_SLEEPATTACK, "SLEEPATTACK", "NPC_SLEEPATTACK" } ,
 { NPC_SMOKING, "SMOKING", "NPC_SMOKING" } ,
 { NPC_SPLASHATTACK, "SPLASHATTACK", "NPC_SPLASHATTACK" } ,
 { NPC_STUNATTACK, "STUNATTACK", "NPC_STUNATTACK" } ,
 { NPC_SUICIDE, "SUICIDE", "NPC_SUICIDE" } ,
 { NPC_SUMMONMONSTER, "SUMMONMONSTER", "NPC_SUMMONMONSTER" } ,
 { NPC_SUMMONSLAVE, "SUMMONSLAVE", "NPC_SUMMONSLAVE" } ,
 { NPC_TELEKINESISATTACK, "TELEKINESISATTACK", "NPC_TELEKINESISATTACK" } ,
 { NPC_TRANSFORMATION, "TRANSFORMATION", "NPC_TRANSFORMATION" } ,
 { NPC_WATERATTACK, "WATERATTACK", "NPC_WATERATTACK" } ,
 { NPC_WINDATTACK, "WINDATTACK", "NPC_WINDATTACK" } ,
 { NV_BASIC, "BASIC", "Basic_Skill" } ,
 { NV_FIRSTAID, "FIRSTAID", "First Aid" } ,
 { NV_TRICKDEAD, "TRICKDEAD", "Play_Dead" } ,
 { PA_GOSPEL, "GOSPEL", "Gospel" } ,
 { PA_PRESSURE, "PRESSURE", "Pressure" } ,
 { PA_SACRIFICE, "SACRIFICE", "Sacrificial_Ritual" } ,
 { PA_SHIELDCHAIN, "PA_SHIELDCHAIN", "Shield_Chain" } ,
 { PF_FOGWALL, "FOGWALL", "Wall_of_Fog" } ,
 { PF_HPCONVERSION, "HPCONVERSION", "Health_Conversion" } ,
 { PF_MEMORIZE, "MEMORIZE", "Memorize" } ,
 { PF_MINDBREAKER, "MINDBREAKER", "Mind_Breaker" } ,
 { PF_SOULBURN, "SOULBURN", "Soul_Burn" } ,
 { PF_SOULCHANGE, "SOULCHANGE", "Soul_Change" } ,
 { PF_SPIDERWEB, "SPIDERWEB", "Spider_Web" } ,
 { PR_ASPERSIO, "ASPERSIO", "Aspersio" } ,
 { PR_BENEDICTIO, "BENEDICTIO", "B.S_Sacramenti" } ,
 { PR_GLORIA, "GLORIA", "Gloria" } ,
 { PR_IMPOSITIO, "IMPOSITIO", "Impositio_Manus" } ,
 { PR_KYRIE, "KYRIE", "Kyrie_Eleison" } ,
 { PR_LEXAETERNA, "LEXAETERNA", "Lex_Aeterna" } ,
 { PR_LEXDIVINA, "LEXDIVINA", "Lex_Divina" } ,
 { PR_MACEMASTERY, "MACEMASTERY", "Mace_Mastery" } ,
 { PR_MAGNIFICAT, "MAGNIFICAT", "Magnificat" } ,
 { PR_MAGNUS, "MAGNUS", "Magnus_Exorcismus" } ,
 { PR_SANCTUARY, "SANCTUARY", "Santuary" } ,
 { PR_SLOWPOISON, "SLOWPOISON", "Slow_Poison" } ,
 { PR_STRECOVERY, "STRECOVERY", "Status_Recovery" } ,
 { PR_SUFFRAGIUM, "SUFFRAGIUM", "Suffragium" } ,
 { PR_TURNUNDEAD, "TURNUNDEAD", "Turn_Undead" } ,
 { RG_BACKSTAP, "BACKSTAP", "Back_Stab" } ,
 { RG_CLEANER, "CLEANER", "Remover" } ,
 { RG_COMPULSION, "COMPULSION", "Compulsion_Discount" } ,
 { RG_FLAGGRAFFITI, "FLAGGRAFFITI", "Flag_Graffity" } ,
 { RG_GANGSTER, "GANGSTER", "Gangster's_Paradise" } ,
 { RG_GRAFFITI, "GRAFFITI", "Graffiti" } ,
 { RG_INTIMIDATE, "INTIMIDATE", "Intimidate" } ,
 { RG_PLAGIARISM, "PLAGIARISM", "Plagiarism" } ,
 { RG_RAID, "RAID", "Raid" } ,
 { RG_SNATCHER, "SNATCHER", "Snatcher" } ,
 { RG_STEALCOIN, "STEALCOIN", "Steal_Coin" } ,
 { RG_STRIPARMOR, "STRIPARMOR", "Strip_Armor" } ,
 { RG_STRIPHELM, "STRIPHELM", "Strip_Helm" } ,
 { RG_STRIPSHIELD, "STRIPSHIELD", "Strip_Shield" } ,
 { RG_STRIPWEAPON, "STRIPWEAPON", "Strip_Weapon" } ,
 { RG_TUNNELDRIVE, "TUNNELDRIVE", "Tunnel_Drive" } ,
 { SA_ABRACADABRA, "ABRACADABRA", "Hocus-pocus" } ,
 { SA_ADVANCEDBOOK, "ADVANCEDBOOK", "Advanced_Book" } ,
 { SA_AUTOSPELL, "AUTOSPELL", "Auto_Cast" } ,
 { SA_CASTCANCEL, "CASTCANCEL", "Cast_Cancel" } ,
 { SA_CLASSCHANGE, "CLASSCHANGE", "Class_Change" } ,
 { SA_COMA, "COMA", "Coma" } ,
 { SA_DEATH, "DEATH", "Death" } ,
 { SA_DELUGE, "DELUGE", "Deluge" } ,
 { SA_DISPELL, "DISPELL", "Dispel" } ,
 { SA_DRAGONOLOGY, "DRAGONOLOGY", "Dragonology" } ,
 { SA_FLAMELAUNCHER, "FLAMELAUNCHER", "Flame_Launcher" } ,
 { SA_FORTUNE, "FORTUNE", "Fortune" } ,
 { SA_FREECAST, "FREECAST", "Cast_Freedom" } ,
 { SA_FROSTWEAPON, "FROSTWEAPON", "Frost_Weapon" } ,
 { SA_FULLRECOVERY, "FULLRECOVERY", "Full_Recovery" } ,
 { SA_GRAVITY, "GRAVITY", "Gravity" } ,
 { SA_INSTANTDEATH, "INSTANTDEATH", "Instant_Death" } ,
 { SA_LANDPROTECTOR, "LANDPROTECTOR", "Land_Protector" } ,
 { SA_LEVELUP, "LEVELUP", "Level_Up" } ,
 { SA_LIGHTNINGLOADER, "LIGHTNINGLOADER", "Lightning_Loader" } ,
 { SA_MAGICROD, "MAGICROD", "Magic_Rod" } ,
 { SA_MONOCELL, "MONOCELL", "Monocell" } ,
 { SA_QUESTION, "QUESTION", "Question?" } ,
 { SA_REVERSEORCISH, "REVERSEORCISH", "Reverse_Orcish" } ,
 { SA_SEISMICWEAPON, "SEISMICWEAPON", "Seismic_Weapon" } ,
 { SA_SPELLBREAKER, "SPELLBREAKER", "Break_Spell" } ,
 { SA_SUMMONMONSTER, "SUMMONMONSTER", "Summon_Monster" } ,
 { SA_TAMINGMONSTER, "TAMINGMONSTER", "Taming_Monster" } ,
 { SA_VIOLENTGALE, "VIOLENTGALE", "Violent_Gale" } ,
 { SA_VOLCANO, "VOLCANO", "Volcano" } ,
 { SG_DEVIL, "DEVIL", "Devil" } ,
 { SG_FEEL, "FEEL", "Feel" } ,
 { SG_FRIEND, "FRIEND", "Friend" } ,
 { SG_FUSION, "FUSION", "Fusion" } ,
 { SG_HATE, "HATE", "Hate" } ,
 { SG_KNOWLEDGE, "KNOWLEDGE", "Knowledge" } ,
 { SG_MOON_ANGER, "ANGER", "Moon Anger" } ,
 { SG_MOON_BLESS, "BLESS", "Moon Bless" } ,
 { SG_MOON_COMFORT, "COMFORT", "Moon Comfort" } ,
 { SG_MOON_WARM, "WARM", "Moon Warm" } ,
 { SG_STAR_ANGER, "ANGER", "Star Anger" } ,
 { SG_STAR_BLESS, "BLESS", "Star Bless" } ,
 { SG_STAR_COMFORT, "COMFORT", "Star Comfort" } ,
 { SG_STAR_WARM, "WARM", "Star Warm" } ,
 { SG_SUN_ANGER, "ANGER", "Sun Anger" } ,
 { SG_SUN_BLESS, "BLESS", "Sun Bless" } ,
 { SG_SUN_COMFORT, "COMFORT", "Sun Comfort" } ,
 { SG_SUN_WARM, "WARM", "Sun Warm" } ,
 { SL_ALCHEMIST, "ALCHEMIST", "Alchemist" } ,
 { SL_ASSASIN, "ASSASIN", "Assasin" } ,
 { SL_BARDDANCER, "BARDDANCER", "Bard Dancer" } ,
 { SL_BLACKSMITH, "BLACKSMITH", "Black Smith" } ,
 { SL_CRUSADER, "CRUSADER", "Crusader" } ,
 { SL_HUNTER, "HUNTER", "Hunter" } ,
 { SL_KAAHI, "KAAHI", "Kaahi" } ,
 { SL_KAINA, "KAINA", "Kaina" } ,
 { SL_KAITE, "KAITE", "Kaite" } ,
 { SL_KAIZEL, "KAIZEL", "Kaizel" } ,
 { SL_KAUPE, "KAUPE", "Kaupe" } ,
 { SL_KNIGHT, "KNIGHT", "Knight" } ,
 { SL_MONK, "MONK", "Monk" } ,
 { SL_PRIEST, "PRIEST", "Priest" } ,
 { SL_ROGUE, "ROGUE", "Rogue" } ,
 { SL_SAGE, "SAGE", "Sage" } ,
 { SL_SKA, "SKA", "SKA" } ,
 { SL_SKE, "SKE", "SKE" } ,
 { SL_SMA, "SMA", "SMA" } ,
 { SL_SOULLINKER, "SOULLINKER", "Soul Linker" } ,
 { SL_STAR, "STAR", "Star" } ,
 { SL_STIN, "STIN", "Stin" } ,
 { SL_STUN, "STUN", "Stun" } ,
 { SL_SUPERNOVICE, "SUPERNOVICE", "Super Novice" } ,
 { SL_SWOO, "SWOO", "Swoo" } ,
 { SL_WIZARD, "WIZARD", "Wizard" } ,
 { SM_AUTOBERSERK, "AUTOBERSERK", "Auto_Berserk" } ,
 { SM_BASH, "BASH", "Bash" } ,
 { SM_ENDURE, "ENDURE", "Endure" } ,
 { SM_FATALBLOW, "FATALBLOW", "Attack_Weak_Point" } ,
 { SM_MAGNUM, "MAGNUM", "Magnum_Break" } ,
 { SM_MOVINGRECOVERY, "MOVINGRECOVERY", "Moving_HP_Recovery" } ,
 { SM_PROVOKE, "PROVOKE", "Provoke" } ,
 { SM_RECOVERY, "RECOVERY", "Increase_HP_Recovery" } ,
 { SM_SWORD, "SWORD", "Sword_Mastery" } ,
 { SM_TWOHAND, "TWOHAND", "Two-Handed_Sword_Mastery" } ,
 { SN_FALCONASSAULT, "FALCONASSAULT", "Falcon_Assault" } ,
 { SN_SHARPSHOOTING, "SHARPSHOOTING", "Sharpshooting" } ,
 { SN_SIGHT, "SIGHT", "True_Sight" } ,
 { SN_WINDWALK, "WINDWALK", "Wind_Walk" } ,
 { ST_CHASEWALK, "CHASEWALK", "Chase_Walk" } ,
 { ST_REJECTSWORD, "REJECTSWORD", "Reject_Sword" } ,
 { ST_STEALBACKPACK, "STEALBACKPACK", "Steal_Backpack" } ,
 { ST_PRESERVE, "PRESERVE", "Preserve" } ,
 { ST_FULLSTRIP, "FULLSTRIP", "Full_Strip" } ,
 { TF_BACKSLIDING, "BACKSLIDING", "Back_Sliding" } ,
 { TF_DETOXIFY, "DETOXIFY", "Detoxify" } ,
 { TF_DOUBLE, "DOUBLE", "Double_Attack" } ,
 { TF_HIDING, "HIDING", "Hiding" } ,
 { TF_MISS, "MISS", "Improve_Dodge" } ,
 { TF_PICKSTONE, "PICKSTONE", "Take_Stone" } ,
 { TF_POISON, "POISON", "Envenom" } ,
 { TF_SPRINKLESAND, "SPRINKLESAND", "Throw_Sand" } ,
 { TF_STEAL, "STEAL", "Steal" } ,
 { TF_THROWSTONE, "THROWSTONE", "Throw_Stone" } ,
 { TK_COUNTER, "COUNTER", "Counter" } ,
 { TK_DODGE, "DODGE", "Dodge" } ,
 { TK_DOWNKICK, "DOWNKICK", "Down Kick" } ,
 { TK_HIGHJUMP, "HIGHJUMP", "High Jump" } ,
 { TK_HPTIME, "HPTIME", "HP Time" } ,
 { TK_JUMPKICK, "JUMPKICK", "Jump Kick" } ,
 { TK_POWER, "POWER", "Power" } ,
 { TK_READYCOUNTER, "READYCOUNTER", "Ready Counter" } ,
 { TK_READYDOWN, "READYDOWN", "Ready Down" } ,
 { TK_READYSTORM, "READYSTORM", "Ready Storm" } ,
 { TK_READYTURN, "READYTURN", "Ready Turn" } ,
 { TK_RUN, "RUN", "TK_RUN" } ,
 { TK_SEVENWIND, "SEVENWIND", "Seven Wind" } ,
 { TK_SPTIME, "SPTIME", "SP Time" } ,
 { TK_STORMKICK, "STORMKICK", "Storm Kick" } ,
 { TK_TURNKICK, "TURNKICK", "Turn Kick" } ,
 { WE_BABY, "BABY", "Adopt_Baby" } ,
 { WE_CALLBABY, "CALLBABY", "Call_Baby" } ,
 { WE_CALLPARENT, "CALLPARENT", "Call_Parent" } ,
 { WE_CALLPARTNER, "CALLPARTNER", "I Want to See You" } ,
 { WE_FEMALE, "FEMALE", "I Only Look Up to You" } ,
 { WE_MALE, "MALE", "I Will Protect You" } ,
 { WS_CARTBOOST, "CARTBOOST", "Cart_Boost" } ,
 { WS_CARTTERMINATION, "WS_CARTTERMINATION", "Cart_Termination" } ,
 { WS_CREATECOIN, "CREATECOIN", "Create_Coins" } ,
 { WS_CREATENUGGET, "CREATENUGGET", "Create_Nuggets" } ,
 { WS_MELTDOWN, "MELTDOWN", "Meltdown" } ,
 { WS_SYSTEMCREATE, "SYSTEMCREATE", "Create_System_tower" } ,
 { WS_WEAPONREFINE, "WEAPONREFINE", "Weapon_Refine" } ,
 { WZ_EARTHSPIKE, "EARTHSPIKE", "Earth_Spike" } ,
 { WZ_ESTIMATION, "ESTIMATION", "Sense" } ,
 { WZ_FIREIVY, "FIREIVY", "Fire_Ivy" } ,
 { WZ_FIREPILLAR, "FIREPILLAR", "Fire_Pillar" } ,
 { WZ_FROSTNOVA, "FROSTNOVA", "Frost_Nova" } ,
 { WZ_HEAVENDRIVE, "HEAVENDRIVE", "Heaven's_Drive" } ,
 { WZ_ICEWALL, "ICEWALL", "Ice_Wall" } ,
 { WZ_JUPITEL, "JUPITEL", "Jupitel_Thunder" } ,
 { WZ_METEOR, "METEOR", "Meteor_Storm" } ,
 { WZ_QUAGMIRE, "QUAGMIRE", "Quagmire" } ,
 { WZ_SIGHTRASHER, "SIGHTRASHER", "Sightrasher" } ,
 { WZ_STORMGUST, "STORMGUST", "Storm_Gust" } ,
 { WZ_VERMILION, "VERMILION", "Lord_of_Vermilion" } ,
 { WZ_WATERBALL, "WATERBALL", "Water_Ball" } ,
 { 0, 0, 0 }
};

static const int dirx[8]={0,-1,-1,-1,0,1,1,1};
static const int diry[8]={1,1,0,-1,-1,-1,0,1};

static int rdamage;

/* ƒXƒLƒ‹ƒf[ƒ^ƒx[ƒX */
struct skill_db skill_db[MAX_SKILL_DB];

/* ƒAƒCƒeƒ€ì¬ƒf[ƒ^ƒx[ƒX */
struct skill_produce_db skill_produce_db[MAX_SKILL_PRODUCE_DB];

/* –îì¬ƒXƒLƒ‹ƒf[ƒ^ƒx[ƒX */
//struct skill_arrow_db skill_arrow_db[MAX_SKILL_ARROW_DB]; -> dynamic now
short num_skill_arrow_db = 0;
struct skill_arrow_db *skill_arrow_db = NULL;

/* ƒAƒuƒ‰ƒJƒ_ƒuƒ‰”­“®ƒXƒLƒ‹ƒf[ƒ^ƒx[ƒX */
struct skill_abra_db skill_abra_db[MAX_SKILL_ABRA_DB];

// macros to check for out of bounds errors
// i: Skill ID, l: Skill Level, var: Value to return after checking
// for values that don't require level use #define xxxx2
// macros with level
#define skill_chk(i, l) \
	if (i >= 10000 && i < 10015) i -= 9500; \
	if (i < 1 || i > MAX_SKILL_DB) return 0; \
	if (l <= 0 || l > MAX_SKILL_LEVEL) return 0
#define skill_get(var, i, l) \
	skill_chk(i, l); return var
// macros without level
#define skill_chk2(i) \
	if (i >= 10000 && i < 10015) i -= 9500; \
	if (i < 1 || i > MAX_SKILL_DB) return 0
#define skill_get2(var, i) \
	skill_chk2(i); return var

// Skill DB
int skill_get_hit(int id) { skill_get2(skill_db[id].hit, id); }
int skill_get_inf(int id) { skill_chk2(id); return (id < 500) ? skill_db[id].inf : guild_skill_get_inf(id); }
int skill_get_pl(int id) { skill_get2(skill_db[id].pl, id); }
int skill_get_nk(int id) { skill_get2(skill_db[id].nk, id); }
int skill_get_max(int id) { skill_chk2(id); return (id < 500) ? skill_db[id].max : guild_skill_get_max(id); }
int skill_get_range(int id, int lv) { skill_chk(id, lv); return (id < 500) ? skill_db[id].range[lv-1] : guild_skill_get_range(id); }
int skill_get_hp(int id, int lv) { skill_get(skill_db[id].hp[lv-1], id, lv); }
int skill_get_sp(int id, int lv) { skill_get(skill_db[id].sp[lv-1], id, lv); }
int skill_get_zeny(int id, int lv) { skill_get(skill_db[id].zeny[lv-1], id, lv); }
int skill_get_num(int id, int lv) { skill_get(skill_db[id].num[lv-1], id, lv); }
int skill_get_cast(int id, int lv) { skill_get(skill_db[id].cast[lv-1], id, lv); }
int skill_get_delay(int id, int lv) { skill_get(skill_db[id].delay[lv-1], id, lv); }
int skill_get_time(int id, int lv) { skill_get(skill_db[id].upkeep_time[lv-1], id, lv); }
int skill_get_time2(int id, int lv) { skill_get(skill_db[id].upkeep_time2[lv-1], id, lv); }
int skill_get_castdef(int id) { skill_get2(skill_db[id].cast_def_rate, id); }
int skill_get_weapontype(int id) { skill_get2(skill_db[id].weapon, id); }
int skill_get_inf2(int id) { skill_get2(skill_db[id].inf2, id); }
int skill_get_castcancel(int id) { skill_get2(skill_db[id].castcancel, id); }
int skill_get_maxcount(int id) { skill_get2(skill_db[id].maxcount, id); }
int skill_get_blewcount(int id, int lv) { skill_get(skill_db[id].blewcount[lv-1], id, lv); }
int skill_get_mhp(int id, int lv) { skill_get(skill_db[id].mhp[lv-1], id, lv); }
int skill_get_castnodex(int id, int lv) { skill_get(skill_db[id].castnodex[lv-1], id, lv); }
int skill_get_delaynodex(int id, int lv) { skill_get(skill_db[id].delaynodex[lv-1], id, lv); }
int skill_get_nocast(int id) { skill_get2(skill_db[id].nocast, id); }
int skill_get_type(int id) { skill_get2(skill_db[id].skill_type, id); }
int skill_get_unit_id(int id, int flag) { skill_get(skill_db[id].unit_id[flag], id, 1); }
int skill_get_unit_layout_type(int id, int lv) { skill_get(skill_db[id].unit_layout_type[lv-1], id, lv); }
int skill_get_unit_interval(int id) { skill_get2(skill_db[id].unit_interval, id); }
int skill_get_unit_range(int id) { skill_get2(skill_db[id].unit_range, id); }
int skill_get_unit_target(int id) { skill_get2(skill_db[id].unit_target, id); }
int skill_get_unit_flag(int id) { skill_get2(skill_db[id].unit_flag, id); }

int skill_tree_get_max(int id, int b_class) {
	struct pc_base_job s_class = pc_calc_base_job(b_class);
	int i, skillid;
	for(i = 0; i < MAX_SKILL_TREE && (skillid = skill_tree[s_class.upper][s_class.job][i].id) > 0; i++)
		if (id == skillid)
			return skill_tree[s_class.upper][s_class.job][i].max;

	return skill_get_max(id);
}

/* ƒvƒƒgƒ^ƒCƒv */
//struct skill_unit_group *skill_unitsetting( struct block_list *src, int skillid,int skilllv,int x,int y,int flag);
int skill_check_condition(struct map_session_data *sd, int type);
int skill_castend_damage_id(struct block_list* src, struct block_list *bl, int skillid, int skilllv, unsigned int tick, int flag);
int skill_frostjoke_scream(struct block_list *bl, va_list ap);
int skill_attack_area(struct block_list *bl, va_list ap);
int skill_abra_dataset(int skilllv);
int skill_clear_element_field(struct block_list *bl);
int skill_landprotector(struct block_list *bl, va_list ap );
int skill_trap_splash(struct block_list *bl, va_list ap );
int skill_count_target(struct block_list *bl, va_list ap );
struct skill_unit_group_tickset *skill_unitgrouptickset_search(struct block_list *bl, struct skill_unit_group *sg, int tick);
int skill_unit_onplace(struct skill_unit *src, struct block_list *bl, unsigned int tick);
int skill_unit_effect(struct block_list *bl, va_list ap);

// [MouseJstr] - skill ok to cast? and when?
int skillnotok(int skillid, struct map_session_data *sd) {
	if (sd == NULL)
		return 1;

	if (!(skillid >= 10000 && skillid < 10015) &&
	    (skillid < 0 || skillid > MAX_SKILL))
		return 1;

  {
	int i = skillid;
	if (i >= 10000 && i < 10015)
		i -= 9500;
	if (sd->blockskill[i] > 0)
		return 1;
  }

	//if (sd->GM_level >= 20)
	if (battle_config.gm_skilluncond > 0 && sd->GM_level >= battle_config.gm_skilluncond)
		return 0; // gm's can do anything damn thing they want

	// Check skill restrictions [Celest]
	if(!map[sd->bl.m].flag.pvp && !map[sd->bl.m].flag.gvg && skill_get_nocast(skillid) & 1)
		return 1;
	if(map[sd->bl.m].flag.pvp && skill_get_nocast(skillid) & 2)
		return 1;
	if(map[sd->bl.m].flag.gvg && skill_get_nocast(skillid) & 4)
		return 1;
	if (agit_flag && skill_get_nocast(skillid) & 8) // 0: WoE not starting, Woe is running
		return 1;
	if (battle_config.pk_mode && map[sd->bl.m].flag.pvp && skill_get_nocast(skillid) & 16)
		return 1;
	if (map[sd->bl.m].flag.nospell && (skill_get_nocast(skillid) & 32) == 0) // the nospell condition
		return 1;

	switch (skillid) {
	case AL_WARP:
	case AL_TELEPORT:
	case MC_VENDING:
	case MC_IDENTIFY:
		return 0; // always allowed
	default:
		return(map[sd->bl.m].flag.noskill);
	}
}


static int distance(int x0, int y_0, int x1, int y_1)
{
	int dx, dy;

	dx = abs(x0  -  x1);
	dy = abs(y_0 - y_1);

	return dx > dy ? dx : dy;
}

/* ƒXƒLƒ‹ƒ†ƒjƒbƒg‚Ì”z’uî•ñ‚ğ•Ô‚· */
struct skill_unit_layout skill_unit_layout[MAX_SKILL_UNIT_LAYOUT];
int firewall_unit_pos;
int icewall_unit_pos;

struct skill_unit_layout *skill_get_unit_layout(int skillid, int skilllv, struct block_list *src, int x, int y) {

	int pos = skill_get_unit_layout_type(skillid, skilllv);
	int dir;

	if (pos != -1)
		return &skill_unit_layout[pos];

	if (src->x == x && src->y == y)
		dir = 2;
	else
		dir = map_calc_dir(src,x,y);

	if (skillid == MG_FIREWALL)
		return &skill_unit_layout[firewall_unit_pos+dir];
	else if (skillid == WZ_ICEWALL)
		return &skill_unit_layout[icewall_unit_pos+dir];

	printf("unknown unit layout for skill %d, %d\n", skillid, skilllv);

	return &skill_unit_layout[0];
}

//	0x89,0x8a,0x8b •\¦–³‚µ
//	0x9a ‰Š?«‚Ì‰r¥‚İ‚½‚¢‚ÈƒGƒtƒFƒNƒg
//	0x9b …?«‚Ì‰r¥‚İ‚½‚¢‚ÈƒGƒtƒFƒNƒg
//	0x9c •—?«‚Ì‰r¥‚İ‚½‚¢‚ÈƒGƒtƒFƒNƒg
//	0x9d ”’‚¢¬‚³‚ÈƒGƒtƒFƒNƒg
//	0xb1 Alchemist Demonstration
//	0xb2 = Pink Warp Portal
//	0xb3 = Gospel For Paladin
//	0xb4 = Basilica
//	0xb5 = Empty
//	0xb6 = Fog Wall for Professor
//	0xb7 = Spider Web for Professor
//	0xb8 = Empty
//	0xb9 =

/*==========================================
 * ƒXƒLƒ‹’Ç‰ÁŒø‰Ê
 *------------------------------------------
 */
int skill_additional_effect(struct block_list* src, struct block_list *bl, int skillid, int skilllv, int attack_type, unsigned int tick)
{
	/* MOB’Ç‰ÁŒø‰ÊƒXƒLƒ‹—p */
	const int sc[] = {
		SC_POISON, SC_BLIND, SC_SILENCE, SC_STAN,
		SC_STONE, SC_CURSE, SC_SLEEP
	};
	const int sc2[]={
		MG_STONECURSE,MG_FROSTDIVER,NPC_STUNATTACK,
		NPC_SLEEPATTACK,TF_POISON,NPC_CURSEATTACK,
		NPC_SILENCEATTACK,0,NPC_BLINDATTACK
	};

	struct map_session_data *sd = NULL;
	struct map_session_data *dstsd = NULL;
	struct mob_data *md = NULL;
	struct mob_data *dstmd = NULL;
	struct pet_data *pd = NULL;

	int skill, skill2;
	int rate;

	int sc_def_mdef,sc_def_vit,sc_def_int,sc_def_luk;
	int sc_def_mdef2,sc_def_vit2,sc_def_int2,sc_def_luk2;

	nullpo_retr(0, src);
	nullpo_retr(0, bl);

	//if(skilllv <= 0) return 0;
	if(skillid > 0 && skilllv <= 0) return 0;	// don't forget auto attacks! - celest

	if(src->type==BL_PC){
		nullpo_retr(0, sd=(struct map_session_data *)src);
	}else if(src->type==BL_MOB){
		nullpo_retr(0, md=(struct mob_data *)src); //–¢g—pH
	}else if(src->type==BL_PET){
		nullpo_retr(0, pd=(struct pet_data *)src); // [Valaris]
	}

	if (bl->type == BL_PC) {
		nullpo_retr(0, dstsd = (struct map_session_data *)bl);
	} else if(bl->type == BL_MOB) {
		nullpo_retr(0, dstmd = (struct mob_data *)bl); //–¢g—pH
	}

	//‘ÎÛ‚Ì‘Ï«
	sc_def_mdef = status_get_sc_def_mdef(bl);
	sc_def_vit = status_get_sc_def_vit(bl);
	sc_def_int = status_get_sc_def_int(bl);
	sc_def_luk = status_get_sc_def_luk(bl);

	//©•ª‚Ì‘Ï«
	sc_def_mdef2 = status_get_sc_def_mdef(src);
	sc_def_vit2 = status_get_sc_def_vit(src);
	sc_def_int2 = status_get_sc_def_int(src);
	sc_def_luk2 = status_get_sc_def_luk(src);

	switch(skillid){
	case 0:					/* ’ÊíUŒ‚ */
		/* ©“®‘é */
		if(sd && pc_isfalcon(sd) && sd->status.weapon == 11 && (skill=pc_checkskill(sd,HT_BLITZBEAT))>0 &&
			rand()%1000 <= sd->paramc[5]*10/3+1 ) {
			int lv=(sd->status.job_level+9)/10;
			skill_castend_damage_id(src,bl,HT_BLITZBEAT,(skill<lv)?skill:lv,tick,0xf00000);
		}
		// ƒXƒiƒbƒ`ƒƒ[
		if(sd && sd->status.weapon != 11 && (skill=pc_checkskill(sd,RG_SNATCHER)) > 0)
			if((skill*15 + 55) + (skill2 = pc_checkskill(sd,TF_STEAL))*10 > rand()%1000) {
				if(pc_steal_item(sd,bl))
					clif_skill_nodamage(src,bl,TF_STEAL,skill2,1);
				else if (battle_config.display_snatcher_skill_fail)
					clif_skill_fail(sd, skillid, 0, 0); // it's annoying! =p [Celest]
			}
		// ƒGƒ“ƒ`ƒƒƒ“ƒgƒfƒbƒgƒŠ[ƒ|ƒCƒYƒ“(–Ò“ÅŒø‰Ê)
		if (sd && sd->sc_data[SC_EDP].timer != -1 && rand() % 10000 < sd->sc_data[SC_EDP].val2 * sc_def_vit) {
			int mhp = status_get_max_hp(bl);
			int hp = status_get_hp(bl);
			int lvl = sd->sc_data[SC_EDP].val1;
			int diff;
			// MHP‚Ì1/4ˆÈ‰º‚É‚Í‚È‚ç‚È‚¢
			if (hp > mhp >> 2) {
				if (bl->type == BL_PC) {
					diff = mhp * 10 / 100;
					if (hp - diff < mhp >> 2)
						diff = hp - (mhp >> 2);
					pc_heal(dstsd, -hp, 0);
				} else if(bl->type == BL_MOB) {
					struct mob_data *md = (struct mob_data *)bl;
					hp -= mhp * 15 / 100;
					if (hp > mhp >> 2)
						md->hp = hp;
					else
						md->hp = mhp >> 2;
				}
			}
			status_change_start(bl,SC_DPOISON,lvl,0,0,0,skill_get_time2(ASC_EDP,lvl),0);
		}
		break;

	case SM_BASH:			/* ƒoƒbƒVƒ…i‹}ŠUŒ‚j */
		if(sd && (skill=pc_checkskill(sd,SM_FATALBLOW))>0 ){
			if( rand()%100 < 6*(skilllv-5)*sc_def_vit/100 )
				status_change_start(bl,SC_STAN,skilllv,0,0,0,skill_get_time2(SM_FATALBLOW,skilllv),0);
		}
		break;

	case TF_POISON: /* ƒCƒ“ƒxƒiƒ€ */
	case AS_SPLASHER: /* ƒxƒiƒ€ƒXƒvƒ‰ƒbƒVƒƒ[ */
		if (rand() % 100< (2 * skilllv + 10) * sc_def_vit / 100)
			status_change_start(bl, SC_POISON, skilllv, 0, 0, 0, skill_get_time2(skillid, skilllv), 0);
		else {
			if (sd && skillid == TF_POISON)
				clif_skill_fail(sd, skillid, 0, 0);
		}
		break;

	case AS_SONICBLOW:		/* ƒ\ƒjƒbƒNƒuƒ[ */
		if( rand()%100 < (2*skilllv+10)*sc_def_vit/100 )
			status_change_start(bl,SC_STAN,skilllv,0,0,0,skill_get_time2(skillid,skilllv),0);
		break;

	case HT_FREEZINGTRAP:	/* ƒtƒŠ[ƒWƒ“ƒOƒgƒ‰ƒbƒv */
		rate=skilllv*3+35;
		if(rand()%100 < rate*sc_def_mdef/100)
			status_change_start(bl,SC_FREEZE,skilllv,0,0,0,skill_get_time2(skillid,skilllv),0);
		break;

	case MG_FROSTDIVER:		/* ƒtƒƒXƒgƒ_ƒCƒo[ */
	case WZ_FROSTNOVA:		/* ƒtƒƒXƒgƒmƒ”ƒ@ */
		rate=(skilllv*3+35)*sc_def_mdef/100-(status_get_int(bl)+status_get_luk(bl))/15;
		rate=rate<=5?5:rate;
		if(rand()%100 < rate)
			status_change_start(bl,SC_FREEZE,skilllv,0,0,0,skill_get_time2(skillid,skilllv),0);
		else if (sd && skillid == MG_FROSTDIVER)
			clif_skill_fail(sd, skillid, 0, 0);
		break;

	case WZ_STORMGUST:		/* ƒXƒg[ƒ€ƒKƒXƒg */
	  {
		struct status_change *sc_data = status_get_sc_data(bl);
		if (sc_data) {
			sc_data[SC_FREEZE].val3++;
//			if (sc_data[SC_FREEZE].val3 >= 3 && rand() % 1000 < skilllv * sc_def_mdef / 100) // previous
			if (sc_data[SC_FREEZE].val3 >= 3) // better formula to calculate the freezing time - [Aalye] from freya' forum
				status_change_start(bl,SC_FREEZE,skilllv,0,0,0,skill_get_time2(skillid,skilllv),0);
		}
	  }
		break;

	case HT_LANDMINE:		/* ƒ‰ƒ“ƒhƒ}ƒCƒ“ */
		if( rand()%100 < (5*skilllv+30)*sc_def_vit/100 )
			status_change_start(bl,SC_STAN,skilllv,0,0,0,skill_get_time2(skillid,skilllv),0);
		break;

/*	case HT_ANKLESNARE: // ƒAƒ“ƒNƒ‹ƒXƒlƒA
		{
			int sec=skill_get_time2(skillid,skilllv);
			if(status_get_mode(bl) & 0x20)
				sec = sec/5;
			battle_stopwalking(bl,1);
			status_change_start(bl,SC_ANKLE,skilllv,0,0,0,sec,0);
		}
		break;*/

	case HT_SHOCKWAVE:				/* ƒVƒ‡ƒbƒNƒEƒF[ƒuƒgƒ‰ƒbƒv */
		if(map[bl->m].flag.pvp && dstsd){
			dstsd->status.sp -= dstsd->status.sp*(5+15*skilllv)/100;
			status_calc_pc(dstsd,0);
		}
		break;
	case HT_SANDMAN:		/* ƒTƒ“ƒhƒ}ƒ“ */
		if( rand()%100 < (5*skilllv+30)*sc_def_int/100 )
			status_change_start(bl,SC_SLEEP,skilllv,0,0,0,skill_get_time2(skillid,skilllv),0);
		break;
	case TF_SPRINKLESAND:	/* »‚Ü‚« */
		if( rand()%100 < 20*sc_def_int/100 )
			status_change_start(bl,SC_BLIND,skilllv,0,0,0,skill_get_time2(skillid,skilllv),0);
		break;

	case TF_THROWSTONE:		/* Î“Š‚° */
		if( rand()%100 < 7*sc_def_vit/100 )
			status_change_start(bl,SC_STAN,skilllv,0,0,0,skill_get_time2(skillid,skilllv),0);
		break;

	case CR_HOLYCROSS:		/* ƒz[ƒŠ[ƒNƒƒX */
		if( rand()%100 < 3*skilllv*sc_def_int/100 )
			status_change_start(bl,SC_BLIND,skilllv,0,0,0,skill_get_time2(skillid,skilllv),0);
		break;

	case CR_GRANDCROSS:		/* ƒOƒ‰ƒ“ƒhƒNƒƒX */
	case NPC_DARKGRANDCROSS:	/*ˆÅƒOƒ‰ƒ“ƒhƒNƒƒX*/
	  {
		int race = status_get_race(bl);
		if ((battle_check_undead(race, status_get_elem_type(bl)) || race == 6) && rand() % 100 < 100000 * sc_def_int / 100) //‹­§•t—^‚¾‚ªŠ®‘S‘Ï«‚É‚Í–³Œø
			status_change_start(bl, SC_BLIND, skilllv, 0, 0, 0, skill_get_time2(skillid, skilllv), 0);
	  }
		break;

	case AM_ACIDTERROR:
		if( rand()%100 < (skilllv*3)*sc_def_vit/100 )
			status_change_start(bl,SC_BLEEDING,skilllv,0,0,0,skill_get_time2(skillid,skilllv),0);
		break;

	case CR_SHIELDCHARGE:		/* ƒV[ƒ‹ƒhƒ`ƒƒ[ƒW */
		if( rand()%100 < (15 + skilllv*5)*sc_def_vit/100 )
			status_change_start(bl,SC_STAN,skilllv,0,0,0,skill_get_time2(skillid,skilllv),0);
		break;

	case RG_RAID:		/* ƒTƒvƒ‰ƒCƒYƒAƒ^ƒbƒN */
		if( rand()%100 < (10+3*skilllv)*sc_def_vit/100 )
			status_change_start(bl,SC_STAN,skilllv,0,0,0,skill_get_time2(skillid,skilllv),0);
		if( rand()%100 < (10+3*skilllv)*sc_def_int/100 )
			status_change_start(bl,SC_BLIND,skilllv,0,0,0,skill_get_time2(skillid,skilllv),0);
		break;
	case BA_FROSTJOKE:
		if(rand()%100 < (15+5*skilllv)*sc_def_mdef/100)
			status_change_start(bl,SC_FREEZE,skilllv,0,0,0,skill_get_time2(skillid,skilllv),0);
		break;

	case DC_SCREAM:
		if( rand()%100 < (25+5*skilllv)*sc_def_vit/100 )
			status_change_start(bl,SC_STAN,skilllv,0,0,0,skill_get_time2(skillid,skilllv),0);
		break;

	case BD_LULLABY:	/* qç‰S */
		if( rand()%100 < 15*sc_def_int/100 )
			status_change_start(bl,SC_SLEEP,skilllv,0,0,0,skill_get_time2(skillid,skilllv),0);
		break;

	case WS_CARTTERMINATION:
		if (rand() % 100 < 30 * skilllv * sc_def_vit)
			status_change_start(bl,SC_STAN,skilllv,0,0,0,skill_get_time2(skillid,skilllv),0);
		break;

	case CR_ACIDDEMONSTRATION:
		if (dstsd) {
			if (rand()%100 < skilllv)
				pc_breakweapon(dstsd);
			if (rand()%100 < skilllv)
				pc_breakarmor(dstsd);
		}
		break;

	/* MOB‚Ì’Ç‰ÁŒø‰Ê•t‚«ƒXƒLƒ‹ */

	case NPC_PETRIFYATTACK:
		if(rand()%100 < sc_def_mdef)
			status_change_start(bl,sc[skillid-NPC_POISON],skilllv,0,0,0,skill_get_time2(skillid,skilllv),0);
		break;
	case NPC_POISON:
	case NPC_SILENCEATTACK:
	case NPC_STUNATTACK:
		if(rand()%100 < sc_def_vit && src->type!=BL_PET)
			status_change_start(bl,sc[skillid-NPC_POISON],skilllv,0,0,0,skill_get_time2(skillid,skilllv),0);
		if(src->type==BL_PET)
			status_change_start(bl,sc[skillid-NPC_POISON],skilllv,0,0,0,skilllv*1000,0);
		break;
	case NPC_CURSEATTACK:
		if(rand()%100 < sc_def_luk)
			status_change_start(bl,sc[skillid-NPC_POISON],skilllv,0,0,0,skill_get_time2(skillid,skilllv),0);
		break;
	case NPC_SLEEPATTACK:
	case NPC_BLINDATTACK:
		if(rand()%100 < sc_def_int)
			status_change_start(bl,sc[skillid-NPC_POISON],skilllv,0,0,0,skill_get_time2(skillid,skilllv),0);
		break;
	case NPC_MENTALBREAKER:
		if(dstsd) {
			int sp = dstsd->status.max_sp*(10+skilllv)/100;
			if(sp < 1) sp = 1;
			pc_heal(dstsd,0,-sp);
		}
		break;

// -- moonsoul (adding status effect chance given to wizard aoe skills meteor and vermillion)
//
	case WZ_METEOR:
		if(rand()%100 < sc_def_vit)
			status_change_start(bl,SC_STAN,skilllv,0,0,0,skill_get_time2(skillid,skilllv),0);
		break;
	case WZ_VERMILION:
		if(rand()%100 < sc_def_int)
			status_change_start(bl,SC_BLIND,skilllv,0,0,0,skill_get_time2(skillid,skilllv),0);
		break;

// -- moonsoul (stun ability of new champion skill tigerfist)
//
	case CH_TIGERFIST:
		if( rand()%100 < (10 + skilllv*10)*sc_def_vit/100 ) {
			int sec = skill_get_time2(skillid,skilllv) - status_get_agi(bl) / 10;
			status_change_start(bl,SC_STAN,skilllv,0,0,0,sec,0);
		}
		break;

	case LK_SPIRALPIERCE:
		if( rand()%100 < (15 + skilllv*5)*sc_def_vit/100 )
			status_change_start(bl,SC_STAN,skilllv,0,0,0,skill_get_time2(skillid,skilllv),0);
		break;
	case ST_REJECTSWORD:	/* ƒtƒŠ[ƒWƒ“ƒOƒgƒ‰ƒbƒv */
		if (rand() % 100 < (skilllv * 15))
			status_change_start(bl, SC_AUTOCOUNTER, skilllv, 0, 0, 0, skill_get_time2(skillid, skilllv), 0);
		break;
	case PF_FOGWALL:		/* ƒz[ƒŠ[ƒNƒƒX */
		//if (src != bl && rand() % 100 < 3 * skilllv * sc_def_int / 100) - previous formula
		if (src != bl && rand() % 100 < sc_def_int) /* better formula from [Aalye] - freya' forum */
			status_change_start(bl, SC_BLIND, skilllv, 0, 0, 0, skill_get_time2(skillid, skilllv), 0);
		break;
	case LK_HEADCRUSH:				/* ƒwƒbƒhƒNƒ‰ƒbƒVƒ… */
	  {//ğŒ‚ª—Ç‚­•ª‚©‚ç‚È‚¢‚Ì‚Å“K“–‚É
		int race = status_get_race(bl);
		int bleed_time = skill_get_time2(skillid, skilllv) - status_get_vit(bl) * 1000;
		if (bleed_time < 90000)
			bleed_time = 90000;	// minimum 90 seconds
		if (!(battle_check_undead(race, status_get_elem_type(bl)) || race == 6) && rand() % 100 < 50 * sc_def_vit / 100)
			status_change_start(bl, SkillStatusChangeTable[skillid], skilllv, 0, 0, 0, bleed_time, 0);
	  }
		break;
	case LK_JOINTBEAT:				/* ƒWƒ‡ƒCƒ“ƒgƒr[ƒg */
		//ğŒ‚ª—Ç‚­•ª‚©‚ç‚È‚¢‚Ì‚Å“K“–‚É
		if (rand() % 100 < (5 * skilllv + 5) * sc_def_vit / 100)
			status_change_start(bl, SC_JOINTBEAT, skilllv, 0, 0, 0, skill_get_time2(skillid, skilllv), 0);
		break;
	case PF_SPIDERWEB:		/* ƒXƒpƒCƒ_[ƒEƒFƒbƒu */
		{
			if(bl->type == BL_MOB)
			{
				int sec=skill_get_time2(skillid,skilllv);
				if(map[src->m].flag.pvp) //PvP‚Å‚ÍS‘©ŠÔ”¼Œ¸H
					sec = sec/2;
				battle_stopwalking(bl,1);
				status_change_start(bl,SC_SPIDERWEB,skilllv,0,0,0,sec,0);
			}
		}
		break;
	case ASC_METEORASSAULT:			/* ƒƒeƒIƒAƒTƒ‹ƒg */
		/*Effect: An attack that causes mass damage to all enemies within a 5x5 cells Area around the caster. Any enemies hit by this skill
		will receive Stun, Darkness, or Bleeding status ailment randomly with a 5%+5*SkillLV% chance. Attack power is 40%+40*LV%.*/
		if (rand() % 100 < (5 + (5 * skilllv) * sc_def_int / 100)) //ó‘ÔˆÙí‚ÍÚ×‚ª•ª‚©‚ç‚È‚¢‚Ì‚Å“K“–‚É
			switch(rand() % 3) {
			case 0:
				status_change_start(bl, SC_STAN,     skilllv, 0, 0, 0, skill_get_time2(skillid, skilllv), 0);
				break;
			case 1:
				status_change_start(bl, SC_BLIND,    skilllv, 0, 0, 0, skill_get_time2(skillid, skilllv), 0);
				break;
			default:
				status_change_start(bl, SC_BLEEDING, skilllv, 0, 0, 0, skill_get_time2(skillid, skilllv), 0);
				break;
			}
		break;
	case MO_EXTREMITYFIST:			/* ˆ¢C—…”e™€Œ */
		//ˆ¢C—…‚ğg‚¤‚Æ5•ªŠÔ©‘R‰ñ•œ‚µ‚È‚¢‚æ‚¤‚É‚È‚é
		status_change_start(src,SkillStatusChangeTable[skillid],skilllv,0,0,0,skill_get_time2(skillid,skilllv),0 );
		break;
	case HW_NAPALMVULCAN:			/* ƒiƒp[ƒ€ƒoƒ‹ƒJƒ“ */
		// skilllv*5%‚ÌŠm—¦‚Åô‚¢
		if (rand()%10000 < 5*skilllv*sc_def_luk)
			status_change_start(bl,SC_CURSE,7,0,0,0,skill_get_time2(NPC_CURSEATTACK,7),0);
		break;
	}

	if (sd && skillid != MC_CARTREVOLUTION && attack_type&BF_WEAPON){	/* ƒJ[ƒh‚É‚æ‚é’Ç‰ÁŒø‰Ê */
		int i;
		int sc_def_card = 100;

		for(i = SC_STONE; i <= SC_BLIND; i++) {
			//‘ÎÛ‚Éó‘ÔˆÙí
			switch (i) {
			case SC_STONE:
			case SC_FREEZE:
				sc_def_card = sc_def_mdef;
				break;
			case SC_STAN:
			case SC_POISON:
			case SC_SILENCE:
				sc_def_card = sc_def_vit;
				break;
			case SC_SLEEP:
			case SC_CONFUSION:
			case SC_BLIND:
				sc_def_card = sc_def_int;
				break;
			case SC_CURSE:
				sc_def_card = sc_def_luk;
				break;
			}
			if (sd) {
				if (!sd->state.arrow_atk) {
					if (rand() % 10000 < (sd->addeff[i - SC_STONE]) * sc_def_card / 100) {
						if (battle_config.battle_log)
							printf("PC %d skill_addeff: card‚É‚æ‚éˆÙí”­“® %d %d\n", sd->bl.id, i, sd->addeff[i - SC_STONE]);
						status_change_start(bl, i, 7, 0, 0, 0, (i == SC_CONFUSION) ? 10000 + 7000 : skill_get_time2(sc2[i - SC_STONE], 7), 0);
					}
				} else {
					if (rand() % 10000 < (sd->addeff[i - SC_STONE] + sd->arrow_addeff[i - SC_STONE]) * sc_def_card / 100) {
						if (battle_config.battle_log)
							printf("PC %d skill_addeff: card‚É‚æ‚éˆÙí”­“® %d %d\n", sd->bl.id, i, sd->addeff[i - SC_STONE]);
						status_change_start(bl, i, 7, 0, 0, 0, (i == SC_CONFUSION) ? 10000 + 7000 : skill_get_time2(sc2[i - SC_STONE], 7), 0);
					}
				}
			}

			//©•ª‚Éó‘ÔˆÙí
			switch (i) {
			case SC_STONE:
			case SC_FREEZE:
				sc_def_card = sc_def_mdef2;
				break;
			case SC_STAN:
			case SC_POISON:
			case SC_SILENCE:
				sc_def_card = sc_def_vit2;
				break;
			case SC_SLEEP:
			case SC_CONFUSION:
			case SC_BLIND:
				sc_def_card = sc_def_int2;
				break;
			case SC_CURSE:
				sc_def_card = sc_def_luk2;
				break;
			}
			if (sd) {
				if (!sd->state.arrow_atk) {
					if (rand() % 10000 < (sd->addeff2[i - SC_STONE]) * sc_def_card / 100) {
						if (battle_config.battle_log)
							printf("PC %d skill_addeff: card‚É‚æ‚éˆÙí”­“® %d %d\n", src->id, i, sd->addeff2[i - SC_STONE]);
						status_change_start(src, i, 7, 0, 0, 0, (i == SC_CONFUSION) ? 10000 + 7000 : skill_get_time2(sc2[i - SC_STONE], 7), 0);
					}
				} else {
					if (rand() % 10000 < (sd->addeff2[i - SC_STONE] + sd->arrow_addeff2[i - SC_STONE]) * sc_def_card / 100) {
						if (battle_config.battle_log)
							printf("PC %d skill_addeff: card‚É‚æ‚éˆÙí”­“® %d %d\n", src->id, i, sd->addeff2[i - SC_STONE]);
						status_change_start(src, i, 7, 0, 0, 0, (i == SC_CONFUSION) ? 10000 + 7000 : skill_get_time2(sc2[i - SC_STONE], 7), 0);
					}
				}
			}

			if (dstsd &&
			    rand() % 10000 < dstsd->addeff3[i - SC_STONE] * sc_def_card / 100) {
				if (battle_config.battle_log)
					printf("PC %d skill_addeff: card?É?æ?é?Ùí??® %d %d\n", src->id, i, dstsd->addeff3[i - SC_STONE]);
				status_change_start(src, i, 7, 0, 0, 0, (i == SC_CONFUSION) ? 10000 + 7000 : skill_get_time2(sc2[i - SC_STONE], 7), 0);
			}
		}
	}

	return 0;
}

/*=========================================================================
 ƒXƒLƒ‹UŒ‚‚«”ò‚Î‚µˆ—
-------------------------------------------------------------------------*/
int skill_blown(struct block_list *src, struct block_list *target, int count)
{
	int dx = 0, dy = 0, nx, ny;
	int x, y;
	int ret, prev_state = MS_IDLE;
	int moveblock;
	struct map_session_data *sd = NULL;
	struct mob_data *md = NULL;
	struct pet_data *pd = NULL;
	struct skill_unit *su = NULL;

	nullpo_retr(0, src);
	nullpo_retr(0, target);

	// no knockback in WoE
	if (map[src->m].flag.gvg)
		return 0;

	if (target->type == BL_PC) {
		sd = (struct map_session_data *)target;
	} else if (target->type == BL_MOB) {
		md = (struct mob_data *)target;
	} else if (target->type == BL_PET) {
		pd = (struct pet_data *)target;
	} else if (target->type == BL_SKILL) {
		su = (struct skill_unit *)target;
	} else
		return 0;

	x = target->x;
	y = target->y;

	if (!(count & 0x10000)) { /* w’è‚È‚µ‚È‚çˆÊ’uŠÖŒW‚©‚ç•ûŒü‚ğ‹‚ß‚é */
		dx = target->x - src->x;
		dx = (dx > 0) ? 1: ((dx < 0) ? -1 : 0);
		dy = target->y - src->y;
		dy = (dy > 0) ? 1: ((dy < 0) ? -1 : 0);
	}
	if (dx == 0 && dy == 0) {
		int dir = status_get_dir(target);
		if (dir >= 0 && dir < 8) {
			dx = -dirx[dir];
			dy = -diry[dir];
		}
	}

	ret = path_blownpos(target->m, x, y, dx, dy, count & 0xffff);
	nx = ret >> 16;
	ny = ret & 0xffff;
	// Boss monsters will no longer be pushed away by knockback skills. [As per kRO Patch 5/11/05]
	if (md && (mob_db[md->class].mexp || md->class == 1288)) { // no mvp (based on [Mikey] from freya's bug report) (emperium can not be blowned [Yor])
		nx = target->x;
		ny = target->y;
	}
	moveblock = (x / BLOCK_SIZE != nx / BLOCK_SIZE || y / BLOCK_SIZE != ny / BLOCK_SIZE);

	if (count & 0x20000) {
		battle_stopwalking(target, 1);
		if (sd) {
			sd->to_x = nx;
			sd->to_y = ny;
//			sd->walktimer = 1;
			clif_walkok(sd);
			clif_movechar(sd);
		} else if (md) {
			md->to_x = nx;
			md->to_y = ny;
			prev_state = md->state.state;
			md->state.state = MS_WALK;
			clif_fixmobpos(md);
		} else if (pd) {
			pd->to_x = nx;
			pd->to_y = ny;
			prev_state = pd->state.state;
			pd->state.state = MS_WALK;
			clif_fixpetpos(pd);
		}
	} else
		battle_stopwalking(target, 2);

	dx = nx - x;
	dy = ny - y;

	if (sd) /* ‰æ–ÊŠO‚Éo‚½‚Ì‚ÅÁ‹ */
		map_foreachinmovearea(clif_pcoutsight, target->m, x - AREA_SIZE, y - AREA_SIZE, x + AREA_SIZE, y + AREA_SIZE, dx, dy, 0, sd);
	else if(md)
		map_foreachinmovearea(clif_moboutsight, target->m, x - AREA_SIZE, y - AREA_SIZE, x + AREA_SIZE, y + AREA_SIZE, dx, dy, BL_PC, md);
	else if(pd)
		map_foreachinmovearea(clif_petoutsight, target->m, x - AREA_SIZE, y - AREA_SIZE, x + AREA_SIZE, y + AREA_SIZE, dx, dy, BL_PC, pd);

	if (su) {
		skill_unit_move_unit_group(su->group, target->m, dx, dy);
	} else {
		skill_unit_move(target, gettick_cache, 0);
		if (moveblock) map_delblock(target);
		target->x = nx;
		target->y = ny;
		if (moveblock) map_addblock(target);
		skill_unit_move(target, gettick_cache, 1);
	}

	if (sd) {	/* ‰æ–Ê“à‚É“ü‚Á‚Ä‚«‚½‚Ì‚Å•\¦ */
		map_foreachinmovearea(clif_pcinsight, target->m, nx - AREA_SIZE, ny - AREA_SIZE, nx + AREA_SIZE, ny + AREA_SIZE, -dx, -dy, 0, sd);
		if (count & 0x20000)
			pc_stop_walking(sd, 1);
	} else if (md) {
		map_foreachinmovearea(clif_mobinsight, target->m, nx - AREA_SIZE, ny - AREA_SIZE, nx + AREA_SIZE, ny + AREA_SIZE, -dx, -dy, BL_PC, md);
		if (count & 0x20000)
			md->state.state = prev_state;
	} else if (pd) {
		map_foreachinmovearea(clif_petinsight, target->m, nx - AREA_SIZE, ny - AREA_SIZE, nx + AREA_SIZE, ny + AREA_SIZE, -dx, -dy, BL_PC, pd);
		if (count & 0x20000)
			pd->state.state = prev_state;
	}

	return 0;
}

/*
 * =========================================================================
 * ƒXƒLƒ‹UŒ‚Œø‰Êˆ—‚Ü‚Æ‚ß
 * flag‚Ìà–¾B16i}
 *	00XRTTff
 *  ff	= magic‚ÅŒvZ‚É“n‚³‚ê‚éj
 *	TT	= ƒpƒPƒbƒg‚Ìtype•”•ª(0‚ÅƒfƒtƒHƒ‹ƒgj
 *  X	= ƒpƒPƒbƒg‚ÌƒXƒLƒ‹Lv
 *  R	= —\–ñiskill_area_sub‚Åg—p‚·‚éj
 *-------------------------------------------------------------------------
 */
int skill_attack(int attack_type, struct block_list* src, struct block_list *dsrc,
                 struct block_list *bl, int skillid, int skilllv, unsigned int tick, int flag) {
	struct Damage dmg;
	struct status_change *sc_data;
	int type, lv, damage;
	static int tmpdmg = 0; // A lot of Corrections to BREAKER SKILL (pneuma included) (Posted on freya's bug report by Gawaine)

	if (skillid > 0 && skilllv <= 0) return 0;

	nullpo_retr(0, src);
	nullpo_retr(0, dsrc);
	nullpo_retr(0, bl);

	rdamage = 0;

	sc_data = status_get_sc_data(bl);

//‰½‚à‚µ‚È‚¢”»’è‚±‚±‚©‚ç
	if(dsrc->m != bl->m) //‘ÎÛ‚ª“¯‚¶ƒ}ƒbƒv‚É‚¢‚È‚¯‚ê‚Î‰½‚à‚µ‚È‚¢
		return 0;
	if(src->prev == NULL || dsrc->prev == NULL || bl->prev == NULL) //prev‚æ‚­‚í‚©‚ç‚È‚¢¦
		return 0;
	if(src->type == BL_PC && pc_isdead((struct map_session_data *)src)) //pÒH‚ªPC‚Å‚·‚Å‚É€‚ñ‚Å‚¢‚½‚ç‰½‚à‚µ‚È‚¢
		return 0;
	if(src != dsrc && dsrc->type == BL_PC && pc_isdead((struct map_session_data *)dsrc)) //pÒH‚ªPC‚Å‚·‚Å‚É€‚ñ‚Å‚¢‚½‚ç‰½‚à‚µ‚È‚¢
		return 0;
	if(bl->type == BL_PC && pc_isdead((struct map_session_data *)bl)) //‘ÎÛ‚ªPC‚Å‚·‚Å‚É€‚ñ‚Å‚¢‚½‚ç‰½‚à‚µ‚È‚¢
		return 0;
	if(src->type == BL_PC && skillnotok(skillid, (struct map_session_data *)src))
		return 0;
	if(sc_data && sc_data[SC_HIDING].timer != -1) { //ƒnƒCƒfƒBƒ“ƒOó‘Ô‚Å
		if(skill_get_pl(skillid) != 2) //ƒXƒLƒ‹‚Ì‘®«‚ª’n‘®«‚Å‚È‚¯‚ê‚Î‰½‚à‚µ‚È‚¢
			return 0;
	}
	if(sc_data && sc_data[SC_TRICKDEAD].timer != -1) //€‚ñ‚¾‚Ó‚è’†‚Í‰½‚à‚µ‚È‚¢
		return 0;
	if(skillid == WZ_STORMGUST) { //g—pƒXƒLƒ‹‚ªƒXƒg[ƒ€ƒKƒXƒg‚Å
		if(sc_data && sc_data[SC_FREEZE].timer != -1) //“€Œ‹ó‘Ô‚È‚ç‰½‚à‚µ‚È‚¢
			return 0;
	}
	if(skillid == WZ_FROSTNOVA && dsrc->x == bl->x && dsrc->y == bl->y) //g—pƒXƒLƒ‹‚ªƒtƒƒXƒgƒmƒ”ƒ@‚ÅAdsrc‚Æbl‚ª“¯‚¶êŠ‚È‚ç‰½‚à‚µ‚È‚¢
		return 0;
	if(src->type == BL_PC && ((struct map_session_data *)src)->chatID) //pÒ‚ªPC‚Åƒ`ƒƒƒbƒg’†‚È‚ç‰½‚à‚µ‚È‚¢
		return 0;
	if(dsrc->type == BL_PC && ((struct map_session_data *)dsrc)->chatID) //pÒ‚ªPC‚Åƒ`ƒƒƒbƒg’†‚È‚ç‰½‚à‚µ‚È‚¢
		return 0;
	if(src->type == BL_PC && bl && mob_gvmobcheck(((struct map_session_data *)src),bl)==0)
		return 0;

//‰½‚à‚µ‚È‚¢”»’è‚±‚±‚Ü‚Å

	type=-1;
	lv=(flag>>20)&0xf;
	dmg=battle_calc_attack(attack_type,src,bl,skillid,skilllv,flag&0xff ); //ƒ_ƒ[ƒWŒvZ

//ƒ}ƒWƒbƒNƒƒbƒhˆ—‚±‚±‚©‚ç
	if(attack_type&BF_MAGIC && sc_data && sc_data[SC_MAGICROD].timer != -1 && src == dsrc) { //–‚–@UŒ‚‚Åƒ}ƒWƒbƒNƒƒbƒhó‘Ô‚Åsrc=dsrc‚È‚ç
		dmg.damage = dmg.damage2 = 0; //ƒ_ƒ[ƒW0
		if(bl->type == BL_PC) { //‘ÎÛ‚ªPC‚Ìê‡
			struct map_session_data *sd = (struct map_session_data *)bl;
			if (sd) {
				int sp = skill_get_sp(skillid, skilllv); //g—p‚³‚ê‚½ƒXƒLƒ‹‚ÌSP‚ğ‹zû
				sp = sp * sc_data[SC_MAGICROD].val2 / 100; //‹zû—¦ŒvZ
				if(skillid == WZ_WATERBALL && skilllv > 1) //ƒEƒH[ƒ^[ƒ{[ƒ‹Lv1ˆÈã
					sp = sp / ((skilllv | 1) * (skilllv | 1)); //‚³‚ç‚ÉŒvZH
				if (sp > 0x7fff) sp = 0x7fff; //SP‘½‚·‚¬‚Ìê‡‚Í—˜_Å‘å’l
				else if (sp < 1) sp = 1; //1ˆÈ‰º‚Ìê‡‚Í1
				if (sd->status.sp + sp > sd->status.max_sp) { //‰ñ•œSP+Œ»İ‚ÌSP‚ªMSP‚æ‚è‘å‚«‚¢ê‡
					sp = sd->status.max_sp - sd->status.sp; //SP‚ğMSP-Œ»İSP‚É‚·‚é
					sd->status.sp = sd->status.max_sp; //Œ»İ‚ÌSP‚ÉMSP‚ğ‘ã“ü
				}
				else //‰ñ•œSP+Œ»İ‚ÌSP‚ªMSP‚æ‚è¬‚³‚¢ê‡‚Í‰ñ•œSP‚ğ‰ÁZ
					sd->status.sp += sp;
				clif_heal(sd->fd, SP_SP, sp); //SP‰ñ•œƒGƒtƒFƒNƒg‚Ì•\¦
				sd->canact_tick = tick + skill_delayfix(bl, skill_get_delay(SA_MAGICROD, sc_data[SC_MAGICROD].val1)); //
			}
		}
		clif_skill_nodamage(bl,bl,SA_MAGICROD,sc_data[SC_MAGICROD].val1,1); //ƒ}ƒWƒbƒNƒƒbƒhƒGƒtƒFƒNƒg‚ğ•\¦
	}
//ƒ}ƒWƒbƒNƒƒbƒhˆ—‚±‚±‚Ü‚Å

	if (src->type == BL_PET) { // [Valaris]
		dmg.damage = battle_attr_fix(skilllv, skill_get_pl(skillid), status_get_element(bl));
		dmg.damage2 = 0;
	}

	damage = dmg.damage + dmg.damage2;

	if(lv==15)
		lv=-1;

	if( flag&0xff00 )
		type=(flag&0xff00)>>8;

	if(damage <= 0 || damage < dmg.div_) //‚«”ò‚Î‚µ”»’èH¦
		dmg.blewcount = 0;

	if (skillid == CR_GRANDCROSS || skillid == NPC_DARKGRANDCROSS) { //ƒOƒ‰ƒ“ƒhƒNƒƒX
		if (battle_config.gx_disptype) dsrc = src; // “Gƒ_ƒ[ƒW”’•¶š•\¦
		if (src == bl) type = 4; // ”½“®‚Íƒ_ƒ[ƒWƒ‚[ƒVƒ‡ƒ“‚È‚µ
	}

//g—pÒ‚ªPC‚Ìê‡‚Ìˆ—‚±‚±‚©‚ç
	if(src->type == BL_PC) {
		struct map_session_data *sd = (struct map_session_data *)src;
		nullpo_retr(0, sd);
//˜A‘Å¶(MO_CHAINCOMBO)‚±‚±‚©‚ç
		if(skillid == MO_CHAINCOMBO) {
			int delay = 1000 - 4 * status_get_agi(src) - 2 * status_get_dex(src); //Šî–{ƒfƒBƒŒƒC‚ÌŒvZ
			if(damage < status_get_hp(bl)) { //ƒ_ƒ[ƒW‚ª‘ÎÛ‚ÌHP‚æ‚è¬‚³‚¢ê‡
				if(pc_checkskill(sd, MO_COMBOFINISH) > 0 && sd->spiritball > 0) //–Ò—´Œ(MO_COMBOFINISH)æ“¾•‹C‹…•Û‚Í+300ms
					delay += 300 * battle_config.combo_delay_rate /100; //’Ç‰ÁƒfƒBƒŒƒC‚ğconf‚É‚æ‚è’²®

					status_change_start(src,SC_COMBO,MO_CHAINCOMBO,skilllv,0,0,delay,0); //ƒRƒ“ƒ{ó‘Ô‚É
			}
			sd->attackabletime = sd->canmove_tick = tick + delay;
			clif_combo_delay(src,delay); //ƒRƒ“ƒ{ƒfƒBƒŒƒCƒpƒPƒbƒg‚Ì‘—M
		}
//˜A‘Å¶(MO_CHAINCOMBO)‚±‚±‚Ü‚Å
//–Ò—´Œ(MO_COMBOFINISH)‚±‚±‚©‚ç
		else if(skillid == MO_COMBOFINISH) {
			int delay = 700 - 4 * status_get_agi(src) - 2 * status_get_dex(src);
			if(damage < status_get_hp(bl)) {
				//ˆ¢C—…”e™€Œ(MO_EXTREMITYFIST)æ“¾•‹C‹…4ŒÂ•Û•”š—ô”g“®(MO_EXPLOSIONSPIRITS)ó‘Ô‚Í+300ms
				//•šŒÕŒ(CH_TIGERFIST)æ“¾‚à+300ms
				if ((pc_checkskill(sd, MO_EXTREMITYFIST) > 0 && sd->spiritball >= 4 && sd->sc_data[SC_EXPLOSIONSPIRITS].timer != -1) ||
				    (pc_checkskill(sd, CH_TIGERFIST) > 0 && sd->spiritball > 0) ||
				    (pc_checkskill(sd, CH_CHAINCRUSH) > 0 && sd->spiritball > 1))
					delay += 300 * battle_config.combo_delay_rate /100; //’Ç‰ÁƒfƒBƒŒƒC‚ğconf‚É‚æ‚è’²®

				status_change_start(src,SC_COMBO,MO_COMBOFINISH,skilllv,0,0,delay,0); //ƒRƒ“ƒ{ó‘Ô‚É
			}
			sd->attackabletime = sd->canmove_tick = tick + delay;
			clif_combo_delay(src,delay); //ƒRƒ“ƒ{ƒfƒBƒŒƒCƒpƒPƒbƒg‚Ì‘—M
		}
//–Ò—´Œ(MO_COMBOFINISH)‚±‚±‚Ü‚Å
//•šŒÕŒ(CH_TIGERFIST)‚±‚±‚©‚ç
		else if(skillid == CH_TIGERFIST) {
			int delay = 1000 - 4 * status_get_agi(src) - 2 * status_get_dex(src);
			if(damage < status_get_hp(bl)) {
				if(pc_checkskill(sd, CH_CHAINCRUSH) > 0) //˜A’Œ•öŒ‚(CH_CHAINCRUSH)æ“¾‚Í+300ms
					delay += 300 * battle_config.combo_delay_rate /100; //’Ç‰ÁƒfƒBƒŒƒC‚ğconf‚É‚æ‚è’²®

				status_change_start(src,SC_COMBO,CH_TIGERFIST,skilllv,0,0,delay,0); //ƒRƒ“ƒ{ó‘Ô‚É
			}
			sd->attackabletime = sd->canmove_tick = tick + delay;
			clif_combo_delay(src,delay); //ƒRƒ“ƒ{ƒfƒBƒŒƒCƒpƒPƒbƒg‚Ì‘—M
		}
//•šŒÕŒ(CH_TIGERFIST)‚±‚±‚Ü‚Å
//˜A’Œ•öŒ‚(CH_CHAINCRUSH)‚±‚±‚©‚ç
		else if(skillid == CH_CHAINCRUSH) {
			int delay = 1000 - 4 * status_get_agi(src) - 2 * status_get_dex(src);
			if(damage < status_get_hp(bl)) {
				//ˆ¢C—…”e™€Œ(MO_EXTREMITYFIST)æ“¾•‹C‹…4ŒÂ•Û•”š—ô”g“®(MO_EXPLOSIONSPIRITS)ó‘Ô‚Í+300ms
				if(pc_checkskill(sd, MO_EXTREMITYFIST) > 0 && sd->spiritball >= 4 && sd->sc_data[SC_EXPLOSIONSPIRITS].timer != -1)
					delay += 300 * battle_config.combo_delay_rate /100; //’Ç‰ÁƒfƒBƒŒƒC‚ğconf‚É‚æ‚è’²®

				status_change_start(src,SC_COMBO,CH_CHAINCRUSH,skilllv,0,0,delay,0); //ƒRƒ“ƒ{ó‘Ô‚É
			}
			sd->attackabletime = sd->canmove_tick = tick + delay;
			clif_combo_delay(src,delay); //ƒRƒ“ƒ{ƒfƒBƒŒƒCƒpƒPƒbƒg‚Ì‘—M
		}
//˜A’Œ•öŒ‚(CH_CHAINCRUSH)‚±‚±‚Ü‚Å
	}
//g—pÒ‚ªPC‚Ìê‡‚Ìˆ—‚±‚±‚Ü‚Å
//•ŠíƒXƒLƒ‹H‚±‚±‚©‚ç
	if(attack_type&BF_MAGIC && damage > 0 && src != bl && src == dsrc) {
		if(bl->type == BL_PC) {
			struct map_session_data *tsd = (struct map_session_data *)bl;
			if(tsd->magic_damage_return > 0) {
				rdamage += damage * tsd->magic_damage_return / 100;
				if(rdamage < 1) rdamage = 1;
			}
		}
	}

	// Stop Here
	if(attack_type&BF_WEAPON && damage > 0 && src != bl && src == dsrc) { //•ŠíƒXƒLƒ‹•ƒ_ƒ[ƒW‚ ‚è•g—pÒ‚Æ‘ÎÛÒ‚ªˆá‚¤•src=dsrc
		if (dmg.flag & BF_SHORT) { //‹ß‹——£UŒ‚H¦
			// Reject Sword working on Active Skills ? Check if the damage reflect "flag" is turned on. - [Aalye]
			if (sc_data && sc_data[SC_REJECTSWORD].val3 != 0) { /* corrected from freya forum by Celest */
				rdamage += damage;
				sc_data[SC_REJECTSWORD].val3 = 0;
				if (rdamage < 1) rdamage = 1;
			}
			if (bl->type == BL_PC) { //‘ÎÛ‚ªPC‚Ì
				struct map_session_data *tsd = (struct map_session_data *)bl;
				nullpo_retr(0, tsd);
				if (tsd->short_weapon_damage_return > 0) { //‹ß‹——£UŒ‚’µ‚Ë•Ô‚µH¦
					rdamage += damage * tsd->short_weapon_damage_return / 100;
					if (rdamage < 1) rdamage = 1;
				}
			}
			if(sc_data && sc_data[SC_REFLECTSHIELD].timer != -1) { //ƒŠƒtƒŒƒNƒgƒV[ƒ‹ƒh
				rdamage += damage * sc_data[SC_REFLECTSHIELD].val2 / 100; //’µ‚Ë•Ô‚µŒvZ
				if(rdamage < 1) rdamage = 1;
			}
		}
		else if(dmg.flag&BF_LONG) { //‰“‹——£UŒ‚H¦
			if(bl->type == BL_PC) { //‘ÎÛ‚ªPC‚Ì
				struct map_session_data *tsd = (struct map_session_data *)bl;
				nullpo_retr(0, tsd);
				if(tsd->long_weapon_damage_return > 0) { //‰“‹——£UŒ‚’µ‚Ë•Ô‚µH¦
					rdamage += damage * tsd->long_weapon_damage_return / 100;
					if(rdamage < 1) rdamage = 1;
				}
			}
		}
		if(rdamage > 0)
			clif_damage(src,src,tick, dmg.amotion,0,rdamage,1,4,0);
	}
//•ŠíƒXƒLƒ‹H‚±‚±‚Ü‚Å

	switch(skillid){
	case AS_SPLASHER:
		clif_skill_damage(dsrc, bl, tick, dmg.amotion, dmg.dmotion, damage, dmg.div_, skillid, -1, 5);
		break;
// A lot of Corrections to BREAKER SKILL (pneuma included) (Posted on freya's bug report by Gawaine)
	case ASC_BREAKER: // [celest]
		if (attack_type&BF_WEAPON) { // the 1st attack won't really deal any damage
			tmpdmg = damage; // store the temporary weapon damage
		} else { // only display damage for the 2nd attack
			if (tmpdmg == 0 || damage == 0) // if one or both attack(s) missed, display a 'miss'
				clif_skill_damage(dsrc, bl, tick, dmg.amotion, dmg.dmotion, 0, dmg.div_, skillid, skilllv, type);
			damage += tmpdmg; // add weapon and magic damage
			tmpdmg = 0; // clear the temporary weapon damage
			if (damage > 0) // if both attacks missed, do not display a 2nd 'miss'
				clif_skill_damage(dsrc, bl, tick, dmg.amotion, dmg.dmotion, damage, dmg.div_, skillid, skilllv, type);
		}
		break;
// End ----------- A lot of Corrections to BREAKER SKILL (pneuma included) (Posted on freya's bug report by Gawaine)
	case NPC_SELFDESTRUCTION:
	case NPC_SELFDESTRUCTION2:
		break;
	case SN_SHARPSHOOTING:
		clif_damage(src, bl, tick, dmg.amotion, dmg.dmotion, damage, 0, 0, 0);
		break;
	default:
		clif_skill_damage(dsrc, bl, tick, dmg.amotion, dmg.dmotion, damage, dmg.div_, skillid, (lv != 0) ? lv : skilllv, (skillid == 0) ? 5 : type);
	}
	/* ‚«”ò‚Î‚µˆ—‚Æ‚»‚ÌƒpƒPƒbƒg */
	if (dmg.blewcount > 0 && bl->type != BL_SKILL && !status_isdead(bl) && !map[src->m].flag.gvg) {
		skill_blown(dsrc, bl, dmg.blewcount);
		if (bl->type == BL_MOB)
			clif_fixmobpos((struct mob_data *)bl);
		else if (bl->type == BL_PET)
			clif_fixpetpos((struct pet_data *)bl);
		else
			clif_fixpos(bl);
	}

	map_freeblock_lock();
	/* ÀÛ‚Éƒ_ƒ[ƒWˆ—‚ğs‚¤ */
	if ((skillid || flag) && !(skillid == ASC_BREAKER && attack_type & BF_WEAPON)) { // do not really deal damage for ASC_BREAKER's 1st attack  // A lot of Corrections to BREAKER SKILL (pneuma included) (Posted on freya's bug report by Gawaine)
	//if (skillid || flag) {
		if (attack_type & BF_WEAPON)
			battle_delay_damage(tick + dmg.amotion, src, bl, damage, 0);
		else
			battle_damage(src, bl, damage, 0);
	}
	if (skillid == RG_INTIMIDATE && damage > 0 && !(status_get_mode(bl) & 0x20) && !map[src->m].flag.gvg) {
		int s_lv = status_get_lv(src);
		int t_lv = status_get_lv(bl);
		int rate;
		rate = 50 + skilllv * 5 + (s_lv - t_lv);
		if (rand() % 100 < rate)
			skill_addtimerskill(src, tick + 800, bl->id, 0, 0, skillid, skilllv, 0, flag);
	}
	if (damage > 0 && dmg.flag & BF_SKILL && bl->type == BL_PC &&
	    pc_checkskill((struct map_session_data *)bl, RG_PLAGIARISM) &&
	    !pc_isdead((struct map_session_data *)bl) && // Updated to not be able to copy skills while dead. [Skotlex]
	    sc_data[SC_PRESERVE].timer == -1){
		struct map_session_data *tsd = (struct map_session_data *)bl;
		if (tsd == NULL) {
			map_freeblock_unlock();
			return 0;
		}
		if (!tsd->status.skill[skillid].id && !tsd->status.skill[skillid].lv &&
		    !(skillid >= NPC_PIERCINGATT && skillid <= NPC_SUMMONMONSTER) &&
		    !(skillid >= NPC_SELFDESTRUCTION2 && skillid <= NPC_INCAGI) &&
		    !(skillid >= LK_AURABLADE)) { // Rogues/Stalker should not copy Adv class skills.
			//Šù‚É“‚ñ‚Å‚¢‚éƒXƒLƒ‹‚ª‚ ‚ê‚ÎŠY“–ƒXƒLƒ‹‚ğÁ‚·
			skill_copy_skill(tsd, skillid, skilllv);
		}
	}
	/* ƒ_ƒ[ƒW‚ª‚ ‚é‚È‚ç’Ç‰ÁŒø‰Ê”»’è */
	if (bl->prev != NULL) {
		struct map_session_data *sd = (struct map_session_data *)bl;
		if (sd == NULL) {
			map_freeblock_unlock();
			return 0;
		}
		if (bl->type != BL_PC || (sd && !pc_isdead(sd))) {
			if (damage > 0)
				skill_additional_effect(src, bl, skillid, skilllv, attack_type, tick);
			if (bl->type == BL_MOB && src != bl) /* ƒXƒLƒ‹g—pğŒ‚ÌMOBƒXƒLƒ‹ */
			{
				struct mob_data *md = (struct mob_data *)bl;
				if (md == NULL) {
					map_freeblock_unlock();
					return 0;
				}
				if (battle_config.mob_changetarget_byskill == 1)
				{
					int target;
					target = md->target_id;
					if (src->type == BL_PC)
						md->target_id = src->id;
					mobskill_use(md, tick, MSC_SKILLUSED | (skillid<<16));
					md->target_id = target;
				} else
					mobskill_use(md, tick, MSC_SKILLUSED | (skillid<<16));
			}
		}
	}

	if (src->type == BL_PC && dmg.flag&BF_WEAPON && src != bl && src == dsrc && damage > 0) {
		struct map_session_data *sd = (struct map_session_data *)src;
		int hp = 0,sp = 0;
		if (sd == NULL) {
			map_freeblock_unlock();
			return 0;
		}
		if (sd->hp_drain_rate && sd->hp_drain_per > 0 && dmg.damage > 0 && rand()%100 < sd->hp_drain_rate) {
			hp += (dmg.damage * sd->hp_drain_per)/100;
			if (sd->hp_drain_rate > 0 && hp < 1) hp = 1;
			else if (sd->hp_drain_rate < 0 && hp > -1) hp = -1;
		}
		if (sd->hp_drain_rate_ && sd->hp_drain_per_ > 0 && dmg.damage2 > 0 && rand()%100 < sd->hp_drain_rate_) {
			hp += (dmg.damage2 * sd->hp_drain_per_)/100;
			if (sd->hp_drain_rate_ > 0 && hp < 1) hp = 1;
			else if (sd->hp_drain_rate_ < 0 && hp > -1) hp = -1;
		}
		if (sd->sp_drain_rate > 0 && sd->sp_drain_per > 0 && dmg.damage > 0 && rand()%100 < sd->sp_drain_rate) {
			sp += (dmg.damage * sd->sp_drain_per)/100;
			if (sd->sp_drain_rate > 0 && sp < 1) sp = 1;
			else if (sd->sp_drain_rate < 0 && sp > -1) sp = -1;
		}
		if (sd->sp_drain_rate_ > 0 && sd->sp_drain_per_ > 0 && dmg.damage2 > 0 && rand()%100 < sd->sp_drain_rate_) {
			sp += (dmg.damage2 * sd->sp_drain_per_)/100;
			if (sd->sp_drain_rate_ > 0 && sp < 1) sp = 1;
			else if (sd->sp_drain_rate_ < 0 && sp > -1) sp = -1;
		}
		if (hp || sp)
			pc_heal(sd, hp, sp);
		if (sd->sp_drain_type && bl->type == BL_PC)
			battle_heal(NULL, bl, 0, -sp, 0);
	}

	if ((skillid || flag) && rdamage > 0) {
		if (attack_type & BF_WEAPON)
			battle_delay_damage(tick + dmg.amotion, bl, src, rdamage, 0);
		else
			battle_damage(bl, src, rdamage, 0);
	}

	if (attack_type&BF_WEAPON && sc_data && sc_data[SC_AUTOCOUNTER].timer != -1 && sc_data[SC_AUTOCOUNTER].val4 > 0) {
		if(sc_data[SC_AUTOCOUNTER].val3 == dsrc->id)
			battle_weapon_attack(bl,dsrc,tick,0x8000|sc_data[SC_AUTOCOUNTER].val1);
		status_change_end(bl,SC_AUTOCOUNTER,-1);
	}

	map_freeblock_unlock();

	return (dmg.damage+dmg.damage2); /* —^ƒ_ƒ‚ğ•Ô‚· */
}

/*==========================================
 * ƒXƒLƒ‹”ÍˆÍUŒ‚—p(map_foreachinarea‚©‚çŒÄ‚Î‚ê‚é)
 * flag‚É‚Â‚¢‚ÄF16i}‚ğŠm”F
 * MSB <- 00fTffff ->LSB
 *	T	=ƒ^[ƒQƒbƒg‘I‘ğ—p(BCT_*)
 *  ffff=©—R‚Ég—p‰Â”\
 *  0	=—\–ñB0‚ÉŒÅ’è
 *------------------------------------------
 */
static int skill_area_temp[8];	/* ˆê•Ï”B•K—v‚È‚çg‚¤B */
typedef int (*SkillFunc)(struct block_list *,struct block_list *,int,int,unsigned int,int);
int skill_area_sub( struct block_list *bl,va_list ap )
{
	struct block_list *src;
	int skill_id, skill_lv, flag;
	unsigned int tick;
	SkillFunc func;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);

	if(bl->type!=BL_PC && bl->type!=BL_MOB && bl->type!=BL_SKILL)
		return 0;

	src=va_arg(ap,struct block_list *); //‚±‚±‚Å‚Ísrc‚Ì’l‚ğQÆ‚µ‚Ä‚¢‚È‚¢‚Ì‚ÅNULLƒ`ƒFƒbƒN‚Í‚µ‚È‚¢
	skill_id=va_arg(ap,int);
	skill_lv=va_arg(ap,int);
	tick=va_arg(ap,unsigned int);
	flag=va_arg(ap,int);
	func=va_arg(ap,SkillFunc);

	if(battle_check_target(src,bl,flag) > 0)
		func(src,bl,skill_id,skill_lv,tick,flag);

	return 0;
}

static int skill_check_unit_range_sub(struct block_list *bl, va_list ap) {
	struct skill_unit *unit;
	int *c;
	int skillid, unit_id;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);
	nullpo_retr(0, unit = (struct skill_unit *)bl);
	nullpo_retr(0, c = va_arg(ap,int *));

	if(bl->prev == NULL || bl->type != BL_SKILL)
		return 0;

	if(!unit->alive)
		return 0;

	skillid = va_arg(ap,int);
	unit_id = unit->group->unit_id;

	if (skillid == MG_SAFETYWALL || skillid == AL_PNEUMA) {
		if (unit_id != 0x7e && unit_id != 0x85)
			return 0;
	} else if (skillid == AL_WARP) {
		if ((unit_id < 0x8f || unit_id > 0x99) && unit_id != 0x92)
			return 0;
	} else if ((skillid >= HT_SKIDTRAP && skillid <= HT_CLAYMORETRAP) || skillid == HT_TALKIEBOX) {
		if ((unit_id < 0x8f || unit_id > 0x99) && unit_id != 0x92)
			return 0;
	} else if (skillid == WZ_FIREPILLAR) {
		if (unit_id != 0x87)
			return 0;
	} else if (skillid == HP_BASILICA) {
		if ((unit_id < 0x8f || unit_id > 0x99) && unit_id != 0x92 && unit_id != 0x83)
			return 0;
	} else
		return 0;

	(*c)++;

	return 0;
}

int skill_check_unit_range(int m, int x, int y, int skillid, int skilllv) {
	int c = 0;
	int range = skill_get_unit_range(skillid);
	int layout_type = skill_get_unit_layout_type(skillid, skilllv);

	if (layout_type == -1 || layout_type > MAX_SQUARE_LAYOUT) {
		printf("skill_check_unit_range: unsupported layout type %d for skill %d\n", layout_type, skillid);
		return 0;
	}

	// ‚Æ‚è‚ ‚¦‚¸³•ûŒ`‚Ìƒ†ƒjƒbƒgƒŒƒCƒAƒEƒg‚Ì‚İ‘Î‰
	range += layout_type;
	map_foreachinarea(skill_check_unit_range_sub, m, x - range, y - range, x + range, y + range, BL_SKILL, &c, skillid);

	return c;
}

static int skill_check_unit_range2_sub(struct block_list *bl, va_list ap) {
	int *c;
	int skillid;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);
	nullpo_retr(0, c = va_arg(ap,int *));

	if (bl->prev == NULL || (bl->type != BL_PC && bl->type != BL_MOB))
		return 0;

	if (bl->type == BL_PC && pc_isdead((struct map_session_data *)bl))
		return 0;

	skillid = va_arg(ap, int);
	if (skillid == HP_BASILICA && bl->type == BL_PC)
		return 0;

	(*c)++;

	return 0;
}

int skill_check_unit_range2(int m, int x, int y, int skillid, int skilllv) {
	int c = 0, range;

	switch (skillid) { // fix by akrus (from freya's bug report)
	case WZ_ICEWALL:
		range = 2;
		break;
	default:
	  {
		int layout_type = skill_get_unit_layout_type(skillid, skilllv);
		if (layout_type == -1 || layout_type > MAX_SQUARE_LAYOUT) {
			printf("skill_check_unit_range2: unsupported layout type %d for skill %d.\n", layout_type, skillid);
			return 0;
		}
		range = skill_get_unit_range(skillid) + layout_type;
	  }
		break;
	}

	// ‚Æ‚è‚ ‚¦‚¸³•ûŒ`‚Ìƒ†ƒjƒbƒgƒŒƒCƒAƒEƒg‚Ì‚İ‘Î‰
	map_foreachinarea(skill_check_unit_range2_sub, m, x - range, y - range, x + range, y + range, 0, &c, skillid);

	return c;
}

int skill_guildaura_sub(struct block_list *bl, va_list ap) {
	struct map_session_data *sd;
//	struct guild *g; // doesn't calculate flag in skill_guildaura_sub (repetitiv)
	int gid, id;
	int flag;

	nullpo_retr(0, sd = (struct map_session_data *)bl); // player
	nullpo_retr(0, ap);

	id = va_arg(ap, int); // id of the guild master
	gid = va_arg(ap, int); // guild_id
	if (sd->status.guild_id != gid) // check if in same guild
		return 0;

/*	// doesn't calculate flag in skill_guildaura_sub (repetitiv)
	g = va_arg(ap, struct guild *);
	flag = 0;
	if (guild_checkskill(g, GD_LEADERSHIP) > 0) flag |= 1 << 0;
	if (guild_checkskill(g, GD_GLORYWOUNDS) > 0) flag |= 1 << 1;
	if (guild_checkskill(g, GD_SOULCOLD) > 0) flag |= 1 << 2;
	if (guild_checkskill(g, GD_HAWKEYES) > 0) flag |= 1 << 3;
	if (guild_checkskill(g, GD_CHARISMA) > 0) flag |= 1 << 4;*/
	flag = va_arg(ap, int); // flag (checked before: > 0)

//	if (flag > 0) { // doesn't calculate flag in skill_guildaura_sub (repetitiv)
		if (sd->sc_count && sd->sc_data[SC_GUILDAURA].timer != -1) {
			if (sd->sc_data[SC_GUILDAURA].val4 != flag) {
				sd->sc_data[SC_GUILDAURA].val4 = flag;
				status_calc_pc(sd, 0);
			}
			return 0;
		}
		status_change_start(&sd->bl, SC_GUILDAURA, 1, id, 0, flag, 0, 0);
//	}

	return 0;
}

/*=========================================================================
 * ”ÍˆÍƒXƒLƒ‹g—pˆ—¬•ª‚¯‚±‚±‚©‚ç
 */
/* ‘ÎÛ‚Ì”‚ğƒJƒEƒ“ƒg‚·‚éBiskill_area_temp[0]‚ğ‰Šú‰»‚µ‚Ä‚¨‚­‚±‚Æj */
int skill_area_sub_count(struct block_list *src,struct block_list *target,int skillid,int skilllv,unsigned int tick,int flag)
{
	if (skillid > 0 && skilllv <= 0) return 0;
	if (skill_area_temp[0] < 0xffff)
		skill_area_temp[0]++;

	return 0;
}

int skill_count_water(struct block_list *src, int range) {
	int i, x, y, cnt = 0,size = range * 2 + 1;
	struct skill_unit *unit;

	for (i = 0; i < size * size; i++) {
		x = src->x + (i % size - range);
		y = src->y + (i / size - range);
		if (map_getcell(src->m, x, y, CELL_CHKWATER)) {
			cnt++;
			continue;
		}
		unit = map_find_skill_unit_oncell(src, x, y, SA_DELUGE, NULL);
		if (unit) {
			cnt++;
			skill_delunit(unit);
		}
	}

	return cnt;
}

/*==========================================
 *
 *------------------------------------------
 */
static int skill_timerskill(int tid, unsigned int tick, int id, int data) {
	struct map_session_data *sd = NULL;
	struct mob_data *md = NULL;
	struct pet_data *pd = NULL;
	struct block_list *src = map_id2bl(id),*target;
	struct skill_timerskill *skl = NULL;
	int range;

	nullpo_retr(0, src);

	if(src->prev == NULL)
		return 0;

	if (src->type == BL_PC) {
		nullpo_retr(0, sd = (struct map_session_data *)src);
		skl = &sd->skilltimerskill[data];
	}
	else if (src->type == BL_MOB) {
		nullpo_retr(0, md = (struct mob_data *)src);
		skl = &md->skilltimerskill[data];
	}
	else if (src->type == BL_PET) { // [Valaris]
		nullpo_retr(0, pd = (struct pet_data *)src);
		skl = &pd->skilltimerskill[data];
	}
	else
		return 0;

	nullpo_retr(0, skl);

	skl->timer = -1;
	if(skl->target_id) {
		struct block_list tbl;
		target = map_id2bl(skl->target_id);
		if(skl->skill_id == RG_INTIMIDATE) {
			if(target == NULL) {
				target = &tbl; //‰Šú‰»‚µ‚Ä‚È‚¢‚Ì‚ÉƒAƒhƒŒƒX“Ë‚Á‚ñ‚Å‚¢‚¢‚Ì‚©‚ÈH
				target->type = BL_NUL;
				target->m = src->m;
				target->prev = target->next = NULL;
			}
		}
		if(target == NULL)
			return 0;
		if(target->prev == NULL && skl->skill_id != RG_INTIMIDATE)
			return 0;
		if(src->m != target->m)
			return 0;
		if(sd && pc_isdead(sd))
			return 0;
		if(target->type == BL_PC && pc_isdead((struct map_session_data *)target) && skl->skill_id != RG_INTIMIDATE)
			return 0;

		switch(skl->skill_id) {
			case TF_BACKSLIDING:
				clif_skill_nodamage(src,src,skl->skill_id,skl->skill_lv,1);
				break;
			case RG_INTIMIDATE:
				if (sd && !map[src->m].flag.noteleport) {
					int x, y, i, j;
					pc_randomwarp(sd);
					for(i=0;i<16;i++) {
						j = rand()%8;
						x = sd->bl.x + dirx[j];
						y = sd->bl.y + diry[j];
						if (map_getcell(sd->bl.m, x, y, CELL_CHKPASS))
							break;
					}
					if(i >= 16) {
						x = sd->bl.x;
						y = sd->bl.y;
					}
					if(target->prev != NULL) {
						if(target->type == BL_PC && !pc_isdead((struct map_session_data *)target))
							pc_setpos((struct map_session_data *)target,map[sd->bl.m].name,x,y,3);
						else if(target->type == BL_MOB)
							mob_warp((struct mob_data *)target,-1,x,y,3);
					}
				}
				else if(md && !map[src->m].flag.monster_noteleport) {
					int x, y, i, j;
					mob_warp(md,-1,-1,-1,3);
					for(i=0;i<16;i++) {
						j = rand()%8;
						x = md->bl.x + dirx[j];
						y = md->bl.y + diry[j];
						if (map_getcell(md->bl.m, x, y, CELL_CHKPASS))
							break;
					}
					if(i >= 16) {
						x = md->bl.x;
						y = md->bl.y;
					}
					if(target->prev != NULL) {
						if(target->type == BL_PC && !pc_isdead((struct map_session_data *)target))
							pc_setpos((struct map_session_data *)target,map[md->bl.m].name,x,y,3);
						else if(target->type == BL_MOB)
							mob_warp((struct mob_data *)target,-1,x,y,3);
					}
				}
				break;

			case BA_FROSTJOKE:			/* Š¦‚¢ƒWƒ‡[ƒN */
			case DC_SCREAM:				/* ƒXƒNƒŠ[ƒ€ */
				range = 15;		//‹ŠE‘S‘Ì
				map_foreachinarea(skill_frostjoke_scream, src->m, src->x - range, src->y - range,
				                  src->x + range, src->y + range, 0, src, skl->skill_id, skl->skill_lv, tick);
				break;

			case WZ_WATERBALL:
				if (skl->type > 1) {
					skl->timer = 0; // skill_addtimerskill‚Åg—p‚³‚ê‚È‚¢‚æ‚¤‚É
					skill_addtimerskill(src, tick + 150, target->id, 0, 0, skl->skill_id, skl->skill_lv, skl->type - 1, skl->flag);
					skl->timer = -1;
				}
				skill_attack(BF_MAGIC, src, src, target, skl->skill_id, skl->skill_lv, tick, skl->flag);
				break;

			default:
				skill_attack(skl->type,src,src,target,skl->skill_id,skl->skill_lv,tick,skl->flag);
				break;
		}
	} else {
		if(src->m != skl->map)
			return 0;
		switch(skl->skill_id) {
			case WZ_METEOR:
				if (skl->type >= 0) {
					skill_unitsetting(src, skl->skill_id, skl->skill_lv, skl->type >> 16, skl->type & 0xFFFF, 0);
					clif_skill_poseffect(src, skl->skill_id, skl->skill_lv, skl->x, skl->y, tick);
				}
				else
					skill_unitsetting(src,skl->skill_id,skl->skill_lv,skl->x,skl->y,0);
				break;
		}
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int skill_addtimerskill(struct block_list *src, unsigned int tick, int target, int x, int y, int skill_id, int skill_lv, int type, int flag) {
	int i;

	nullpo_retr(1, src);

	if (src->type == BL_PC) {
		struct map_session_data *sd = (struct map_session_data *)src;
		nullpo_retr(1, sd);
		for(i = 0; i < MAX_SKILLTIMERSKILL; i++) {
			if (sd->skilltimerskill[i].timer == -1) {
				sd->skilltimerskill[i].timer = add_timer(tick, skill_timerskill, src->id, i);
				sd->skilltimerskill[i].src_id = src->id;
				sd->skilltimerskill[i].target_id = target;
				sd->skilltimerskill[i].skill_id = skill_id;
				sd->skilltimerskill[i].skill_lv = skill_lv;
				sd->skilltimerskill[i].map = src->m;
				sd->skilltimerskill[i].x = x;
				sd->skilltimerskill[i].y = y;
				sd->skilltimerskill[i].type = type;
				sd->skilltimerskill[i].flag = flag;
				return 0;
			}
		}
		return 1;
	} else if (src->type == BL_MOB) {
		struct mob_data *md = (struct mob_data *)src;
		nullpo_retr(1, md);
		for(i = 0; i < MAX_MOBSKILLTIMERSKILL; i++) {
			if (md->skilltimerskill[i].timer == -1) {
				md->skilltimerskill[i].timer = add_timer(tick, skill_timerskill, src->id, i);
				md->skilltimerskill[i].src_id = src->id;
				md->skilltimerskill[i].target_id = target;
				md->skilltimerskill[i].skill_id = skill_id;
				md->skilltimerskill[i].skill_lv = skill_lv;
				md->skilltimerskill[i].map = src->m;
				md->skilltimerskill[i].x = x;
				md->skilltimerskill[i].y = y;
				md->skilltimerskill[i].type = type;
				md->skilltimerskill[i].flag = flag;
				return 0;
			}
		}
		return 1;
	} else if (src->type == BL_PET) { // [Valaris]
		struct pet_data *pd = (struct pet_data *)src;
		nullpo_retr(1, pd);
		for(i = 0; i < MAX_MOBSKILLTIMERSKILL; i++) {
			if (pd->skilltimerskill[i].timer == -1) {
				pd->skilltimerskill[i].timer = add_timer(tick, skill_timerskill, src->id, i);
				pd->skilltimerskill[i].src_id = src->id;
				pd->skilltimerskill[i].target_id = target;
				pd->skilltimerskill[i].skill_id = skill_id;
				pd->skilltimerskill[i].skill_lv = skill_lv;
				pd->skilltimerskill[i].map = src->m;
				pd->skilltimerskill[i].x = x;
				pd->skilltimerskill[i].y = y;
				pd->skilltimerskill[i].type = type;
				pd->skilltimerskill[i].flag = flag;
				return 0;
			}
		}
		return 1;
	}

	return 1;
}

/*==========================================
 *
 *------------------------------------------
 */
int skill_cleartimerskill(struct block_list *src)
{
	int i;

	nullpo_retr(0, src);

	if (src->type == BL_PC) {
		struct map_session_data *sd = (struct map_session_data *)src;
		nullpo_retr(0, sd);
		for(i = 0; i < MAX_SKILLTIMERSKILL; i++) {
			if (sd->skilltimerskill[i].timer != -1) {
				delete_timer(sd->skilltimerskill[i].timer, skill_timerskill);
				sd->skilltimerskill[i].timer = -1;
			}
		}
	} else if (src->type == BL_MOB) {
		struct mob_data *md = (struct mob_data *)src;
		nullpo_retr(0, md);
		for(i = 0; i < MAX_MOBSKILLTIMERSKILL; i++) {
			if (md->skilltimerskill[i].timer != -1) {
				delete_timer(md->skilltimerskill[i].timer, skill_timerskill);
				md->skilltimerskill[i].timer = -1;
			}
		}
	} else if (src->type == BL_PET) {
		struct pet_data *pd = (struct pet_data *)src;
		nullpo_retr(0, pd);
		for(i = 0; i < MAX_MOBSKILLTIMERSKILL; i++) {
			if (pd->skilltimerskill[i].timer != -1) {
				delete_timer(pd->skilltimerskill[i].timer, skill_timerskill);
				pd->skilltimerskill[i].timer = -1;
			}
		}
	}

	return 0;
}

/* ”ÍˆÍƒXƒLƒ‹g—pˆ—¬•ª‚¯‚±‚±‚Ü‚Å
 * -------------------------------------------------------------------------
 */


/*==========================================
 * ƒXƒLƒ‹g—pi‰r¥Š®—¹AIDw’èUŒ‚Œnj
 * iƒXƒpƒQƒbƒeƒB‚ÉŒü‚¯‚Ä‚P•à‘OiI(ƒ_ƒƒ|)j
 *------------------------------------------
 */
int skill_castend_damage_id(struct block_list* src, struct block_list *bl, int skillid, int skilllv, unsigned int tick, int flag)
{
	struct map_session_data *sd = NULL;
	struct status_change *sc_data;
	int i;

	if (skillid < 0) {
		//printf("skill_castend_damage_id: skillid=%i(lvl:%d)\ncall: %p %p %i %i %i %i", skillid, skilllv, src, bl, skillid, skilllv, tick, flag);
		return 0;
	}
	if (skillid > 0 && skilllv <= 0) return 0;

	nullpo_retr(1, src);
	nullpo_retr(1, bl);

	sc_data = status_get_sc_data(src);

	if(src->type==BL_PC)
		sd=(struct map_session_data *)src;
	if(sd && pc_isdead(sd))
		return 1;

	if ((skillid == CR_GRANDCROSS || skillid == NPC_DARKGRANDCROSS) && src != bl)
		bl = src;
	if (bl->prev == NULL)
		return 1;
	if (bl->type == BL_PC && pc_isdead((struct map_session_data *)bl))
		return 1;

	map_freeblock_lock();
	switch(skillid)
	{
	/* •ŠíUŒ‚ŒnƒXƒLƒ‹ */
	case SM_BASH:			/* ƒoƒbƒVƒ… */
	case MC_MAMMONITE:		/* ƒƒ}[ƒiƒCƒg */
	case AC_DOUBLE:			/* ƒ_ƒuƒ‹ƒXƒgƒŒƒCƒtƒBƒ“ƒO */
	case AS_SONICBLOW:		/* ƒ\ƒjƒbƒNƒuƒ[ */
	case KN_PIERCE:			/* ƒsƒA[ƒX */
	case KN_SPEARBOOMERANG:	/* ƒXƒsƒAƒu[ƒƒ‰ƒ“ */
	case TF_POISON:			/* ƒCƒ“ƒxƒiƒ€ */
	case TF_SPRINKLESAND:	/* »‚Ü‚« */
	case AC_CHARGEARROW:	/* ƒ`ƒƒ[ƒWƒAƒ[ */
	case KN_SPEARSTAB:		/* ƒXƒsƒAƒXƒ^ƒu */
	case RG_RAID:		/* ƒTƒvƒ‰ƒCƒYƒAƒ^ƒbƒN */
	case RG_INTIMIDATE:		/* ƒCƒ“ƒeƒBƒ~ƒfƒCƒg */
	case BA_MUSICALSTRIKE:	/* ƒ~ƒ…[ƒWƒJƒ‹ƒXƒgƒ‰ƒCƒN */
	case DC_THROWARROW:		/* –îŒ‚‚¿ */
	case BA_DISSONANCE:		/* •s‹¦˜a‰¹ */
	case CR_HOLYCROSS:		/* ƒz[ƒŠ[ƒNƒƒX */
	case CR_SHIELDCHARGE:
	case CR_SHIELDBOOMERANG:

	/* ˆÈ‰ºMOBê—p */
	/* ’P‘ÌUŒ‚ASPŒ¸­UŒ‚A‰“‹——£UŒ‚A–hŒä–³‹UŒ‚A‘½’iUŒ‚ */
	case NPC_PIERCINGATT:
	case NPC_MENTALBREAKER:
	case NPC_RANGEATTACK:
	case NPC_CRITICALSLASH:
	case NPC_COMBOATTACK:
	/* •K’†UŒ‚A“ÅUŒ‚AˆÃ•UŒ‚A’¾–ÙUŒ‚AƒXƒ^ƒ“UŒ‚ */
	case NPC_GUIDEDATTACK:
	case NPC_POISON:
	case NPC_BLINDATTACK:
	case NPC_SILENCEATTACK:
	case NPC_STUNATTACK:
	/* Î‰»UŒ‚Aô‚¢UŒ‚A‡–°UŒ‚Aƒ‰ƒ“ƒ_ƒ€ATKUŒ‚ */
	case NPC_PETRIFYATTACK:
	case NPC_CURSEATTACK:
	case NPC_SLEEPATTACK:
	case NPC_RANDOMATTACK:
	/* …‘®«UŒ‚A’n‘®«UŒ‚A‰Î‘®«UŒ‚A•—‘®«UŒ‚ */
	case NPC_WATERATTACK:
	case NPC_GROUNDATTACK:
	case NPC_FIREATTACK:
	case NPC_WINDATTACK:
	/* “Å‘®«UŒ‚A¹‘®«UŒ‚AˆÅ‘®«UŒ‚A”O‘®«UŒ‚ASPŒ¸­UŒ‚ */
	case NPC_POISONATTACK:
	case NPC_HOLYATTACK:
	case NPC_DARKNESSATTACK:
	case NPC_TELEKINESISATTACK:
	case NPC_UNDEADATTACK:
	case LK_AURABLADE:		/* ƒI[ƒ‰ƒuƒŒ[ƒh */
	case LK_SPIRALPIERCE:	/* ƒXƒpƒCƒ‰ƒ‹ƒsƒA[ƒX */
	case LK_HEADCRUSH:	/* ƒwƒbƒhƒNƒ‰ƒbƒVƒ… */
	case LK_JOINTBEAT:	/* ƒWƒ‡ƒCƒ“ƒgƒr[ƒg */
//	case PA_SACRIFICE:	/* ƒTƒNƒŠƒtƒ@ƒCƒX */
//	case SN_SHARPSHOOTING:			/* ƒVƒƒ[ƒvƒVƒ…[ƒeƒBƒ“ƒO */
	case CG_ARROWVULCAN:			/* ƒAƒ[ƒoƒ‹ƒJƒ“ */
	case HW_MAGICCRASHER:		/* ƒ}ƒWƒbƒNƒNƒ‰ƒbƒVƒƒ[ */
	case ITM_TOMAHAWK:
	case PA_SHIELDCHAIN:
	case WS_CARTTERMINATION:
	case ASC_METEORASSAULT:	/* ƒƒeƒIƒAƒTƒ‹ƒg */ // Meteor Assault skill fix (thanks to [Mikey] from freya's bug report)
		skill_attack(BF_WEAPON, src, src, bl, skillid, skilllv, tick, flag);
		break;

// A lot of Corrections to BREAKER SKILL (pneuma included) (Posted on freya's bug report by Gawaine)
	case ASC_BREAKER:				/* ƒ\ƒEƒ‹ƒuƒŒ[ƒJ[ */
		// Separate weapon and magic attacks
		skill_attack(BF_WEAPON, src, src, bl, skillid, skilllv, tick, flag);
		skill_attack(BF_MAGIC, src, src, bl, skillid, skilllv, tick, flag);
		break;
// End ----------- A lot of Corrections to BREAKER SKILL (pneuma included) (Posted on freya's bug report by Gawaine)

	case SN_SHARPSHOOTING:			/* ƒVƒƒ[ƒvƒVƒ…[ƒeƒBƒ“ƒO */
		// Does it stop if touch an obstacle? it shouldn't shoot trough walls
		map_foreachinpath(skill_attack_area, src->m, src->x, src->y, bl->x, bl->y, 2, 0, // function, map, source xy, target xy, range, type
		                  BF_WEAPON, src, src, skillid, skilllv, tick, flag, BCT_ENEMY); // varargs
		break;

	case CR_ACIDDEMONSTRATION:  // Acid Demonstration
		skill_attack(BF_MISC, src, src, bl, skillid, skilllv, tick, flag);
		break;

	case PA_PRESSURE:	/* ƒvƒŒƒbƒVƒƒ[ */
		skill_attack(BF_WEAPON, src, src, bl, skillid, skilllv, tick, flag);
		if (rand() % 100 < 50)
			status_change_start(bl, SC_STAN, skilllv, 0, 0, 0, skill_get_time2(PA_PRESSURE, skilllv), 0);
		else
			if (rand() % 100 < (50 - (status_get_vit(bl) / 10))) // <-- % fixed by [SePhII2oTh] from freya's bug report
				status_change_start(bl, SC_BLEEDING, skilllv, 0, 0, 0, skill_get_time2(PA_PRESSURE, skilllv), 0);
		if (bl->type == BL_PC) {
			struct map_session_data *tsd = (struct map_session_data *)bl;
			tsd->status.sp -= (tsd->status.sp * (15 + 5 * skilllv) / 100);
			clif_updatestatus(tsd, SP_SP);
		}
		break;

	case NPC_DARKBREATH:
		clif_emotion(src,7);
		skill_attack(BF_MISC, src, src, bl, skillid, skilllv, tick, flag);
		break;

	case MO_INVESTIGATE:	/* ”­™¤ */
		{
			struct status_change *sc_data = status_get_sc_data(src);
			skill_attack(BF_WEAPON,src,src,bl,skillid,skilllv,tick,flag);
			if(sc_data && sc_data[SC_BLADESTOP].timer != -1)
				status_change_end(src,SC_BLADESTOP,-1);
		}
		break;
	case SN_FALCONASSAULT:			/* ƒtƒ@ƒ‹ƒRƒ“ƒAƒTƒ‹ƒg */
		skill_attack(BF_MISC,src,src,bl,skillid,skilllv,tick,flag);
		break;
	case KN_BRANDISHSPEAR:		/* ƒuƒ‰ƒ“ƒfƒBƒbƒVƒ…ƒXƒsƒA */
		{
			struct mob_data *md = (struct mob_data *)bl;
			if (md == NULL) {
				map_freeblock_unlock();
				return 1;
			}
			skill_attack(BF_WEAPON,src,src,bl,skillid,skilllv,tick,flag);
			if (md->hp > 0){
				skill_blown(src,bl,skill_get_blewcount(skillid,skilllv));
				if (bl->type == BL_MOB)
					clif_fixmobpos((struct mob_data *)bl);
				else if (bl->type == BL_PET)
					clif_fixpetpos((struct pet_data *)bl);
				else
					clif_fixpos(bl);
			}
		}
		break;
	case RG_BACKSTAP:		/* ƒoƒbƒNƒXƒ^ƒu */
		{
			int dir = map_calc_dir(src,bl->x,bl->y);
			int t_dir = status_get_dir(bl);
			int dist = distance(src->x,src->y,bl->x,bl->y);
			if ((dist > 0 && !map_check_dir(dir,t_dir)) || bl->type == BL_SKILL) {
				struct status_change *sc_data = status_get_sc_data(src);
				if(sc_data && sc_data[SC_HIDING].timer != -1)
					status_change_end(src, SC_HIDING, -1);	// ƒnƒCƒfƒBƒ“ƒO‰ğœ
				skill_attack(BF_WEAPON,src,src,bl,skillid,skilllv,tick,flag);
				dir = dir < 4 ? dir+4 : dir-4; // change direction [Celest]
				if (bl->type == BL_PC)
					((struct map_session_data *)bl)->dir = dir;
				else if (bl->type == BL_MOB)
					((struct mob_data *)bl)->dir = dir;
				clif_changed_dir(bl);
			}
			else if (src->type == BL_PC)
				clif_skill_fail(sd, sd->skillid, 0, 0);
		}
		break;

	case AM_ACIDTERROR:		/* ƒAƒVƒbƒhƒeƒ‰[ */
		skill_attack(BF_WEAPON,src,src,bl,skillid,skilllv,tick,flag);
		if (bl->type == BL_PC && rand()%100 < skill_get_time(skillid,skilllv) && battle_config.equipment_breaking) {
			pc_breakarmor((struct map_session_data *)bl);
			clif_emotion(bl, 23);
		}
		break;
	case MO_FINGEROFFENSIVE:	/* w’e */
	  {
		struct status_change *sc_data = status_get_sc_data(src);
		if (!battle_config.finger_offensive_type)
			skill_attack(BF_WEAPON, src, src, bl, skillid, skilllv, tick, flag);
		else {
			skill_attack(BF_WEAPON, src, src, bl, skillid, skilllv, tick, flag);
			if (sd) {
				for(i = 1; i < sd->spiritball_old; i++)
					skill_addtimerskill(src, tick + i * 200, bl->id, 0, 0, skillid, skilllv, BF_WEAPON, flag);
				sd->canmove_tick = tick + (sd->spiritball_old - 1) * 200;
			}
		}
		if (sc_data && sc_data[SC_BLADESTOP].timer != -1)
			status_change_end(src, SC_BLADESTOP, -1);
	  }
		break;
	case MO_CHAINCOMBO:		/* ˜A‘Å¶ */
	  {
		struct status_change *sc_data = status_get_sc_data(src);
		skill_attack(BF_WEAPON,src,src,bl,skillid,skilllv,tick,flag);
		if(sc_data && sc_data[SC_BLADESTOP].timer != -1)
			status_change_end(src,SC_BLADESTOP,-1);
	  }
		break;
	case MO_COMBOFINISH:	/* –Ò—´Œ */
	case CH_TIGERFIST:		/* •šŒÕŒ */
	case CH_CHAINCRUSH:		/* ˜A’Œ•öŒ‚ */
	case CH_PALMSTRIKE:		/* –ÒŒÕd”hR */
		skill_attack(BF_WEAPON,src,src,bl,skillid,skilllv,tick,flag);
		break;
	case MO_EXTREMITYFIST:	/* ˆ¢C—…”e–PŒ */
	  {
		struct status_change *sc_data = status_get_sc_data(src);

		if(sd) {
			struct walkpath_data wpd;
			int dx,dy;

			dx = bl->x - sd->bl.x;
			dy = bl->y - sd->bl.y;
			if(dx > 0) dx++;
			else if(dx < 0) dx--;
			if(dy > 0) dy++;
			else if(dy < 0) dy--;
			if(dx == 0 && dy == 0) dx++;
			if(path_search(&wpd,src->m,sd->bl.x,sd->bl.y,sd->bl.x+dx,sd->bl.y+dy,1) == -1) {
				dx = bl->x - sd->bl.x;
				dy = bl->y - sd->bl.y;
				if(path_search(&wpd,src->m,sd->bl.x,sd->bl.y,sd->bl.x+dx,sd->bl.y+dy,1) == -1) {
					clif_skill_fail(sd, sd->skillid, 0, 0);
					break;
				}
			}
			sd->to_x = sd->bl.x + dx;
			sd->to_y = sd->bl.y + dy;
			skill_attack(BF_WEAPON,src,src,bl,skillid,skilllv,tick,flag);
			clif_walkok(sd);
			clif_movechar(sd);
			if(dx < 0) dx = -dx;
			if(dy < 0) dy = -dy;
			sd->attackabletime = sd->canmove_tick = tick + 100 + sd->speed * ((dx > dy)? dx:dy);
			if(sd->canact_tick < sd->canmove_tick)
				sd->canact_tick = sd->canmove_tick;
			pc_movepos(sd,sd->to_x,sd->to_y);
			status_change_end(&sd->bl,SC_COMBO,-1);
		}
		else
			skill_attack(BF_WEAPON,src,src,bl,skillid,skilllv,tick,flag);
		status_change_end(src, SC_EXPLOSIONSPIRITS, -1);
		if(sc_data && sc_data[SC_BLADESTOP].timer != -1)
			status_change_end(src,SC_BLADESTOP,-1);
	  }
		break;
	/* •ŠíŒn”ÍˆÍUŒ‚ƒXƒLƒ‹ */
	case AC_SHOWER:			/* ƒAƒ[ƒVƒƒƒ[ */
//	case SM_MAGNUM:			/* ƒ}ƒOƒiƒ€ƒuƒŒƒCƒN */
	case AS_GRIMTOOTH:		/* ƒOƒŠƒ€ƒgƒD[ƒX */
	case MC_CARTREVOLUTION:	/* ƒJ[ƒgƒŒƒ”ƒHƒŠƒ…[ƒVƒ‡ƒ“ */
	case NPC_SPLASHATTACK:	/* ƒXƒvƒ‰ƒbƒVƒ…ƒAƒ^ƒbƒN */
//	case ASC_METEORASSAULT:	/* ƒƒeƒIƒAƒTƒ‹ƒg */ // Meteor Assault skill fix (thanks to [Mikey] from freya's bug report)
	case AS_SPLASHER: /* ƒxƒiƒ€ƒXƒvƒ‰ƒbƒVƒƒ[ */
		if (flag & 1) {
			/* ŒÂ•Ê‚Éƒ_ƒ[ƒW‚ğ—^‚¦‚é */
			if (bl->id != skill_area_temp[1]) {
				skill_attack(BF_WEAPON, src, src, bl, skillid, skilllv, tick, 0x0500);
				if (bl->type == BL_MOB && skillid == AS_GRIMTOOTH) {
					struct status_change *sc_data = status_get_sc_data(bl);
					if (sc_data && sc_data[SC_SLOWDOWN].timer == -1)
						status_change_start(bl, SC_SLOWDOWN, 0, 0, 0, 0, 1000, 0);
				}
			}
		}else{
			int ar = 1;
			int x = bl->x, y = bl->y;
			//if (skillid == AC_SHOWER || skillid == ASC_METEORASSAULT) /* ƒAƒ[ƒVƒƒƒ[AƒƒeƒIƒAƒTƒ‹ƒg”ÍˆÍ5*5 */ // Meteor Assault skill fix (thanks to [Mikey] from freya's bug report)
			if (skillid == AC_SHOWER) /* ƒAƒ[ƒVƒƒƒ[AƒƒeƒIƒAƒTƒ‹ƒg”ÍˆÍ5*5 */
				ar=2;
			else if (skillid == AS_SPLASHER) /* ƒxƒiƒ€ƒXƒvƒ‰ƒbƒVƒƒ[”ÍˆÍ3*3 */
				ar=1;
			else if (skillid == NPC_SPLASHATTACK) /* ƒXƒvƒ‰ƒbƒVƒ…ƒAƒ^ƒbƒN‚Í”ÍˆÍ7*7 */
				ar=3;

//			if (skillid == ASC_METEORASSAULT) // Meteor Assault skill fix (thanks to [Mikey] from freya's bug report)
//				clif_skill_nodamage(src,bl,skillid,skilllv,1);

			skill_area_temp[1] = bl->id;
			skill_area_temp[2] = x;
			skill_area_temp[3] = y;
			/* ‚Ü‚¸ƒ^[ƒQƒbƒg‚ÉUŒ‚‚ğ‰Á‚¦‚é */
			skill_attack(BF_WEAPON,src,src,bl,skillid,skilllv,tick,0);
			/* ‚»‚ÌŒãƒ^[ƒQƒbƒgˆÈŠO‚Ì”ÍˆÍ“à‚Ì“G‘S‘Ì‚Éˆ—‚ğs‚¤ */
			map_foreachinarea(skill_area_sub,
				bl->m,x-ar,y-ar,x+ar,y+ar,0,
				src,skillid,skilllv,tick, flag|BCT_ENEMY|1,
				skill_castend_damage_id);
		}
		break;

	case SM_MAGNUM:			/* ƒ}ƒOƒiƒ€ƒuƒŒƒCƒN */
		if (flag & 1) {
			/* ŒÂ•Ê‚Éƒ_ƒ[ƒW‚ğ—^‚¦‚é */
			if (bl->id != skill_area_temp[1]) {
				int dist = distance(bl->x, bl->y, skill_area_temp[2], skill_area_temp[3]); /* ƒ}ƒOƒiƒ€ƒuƒŒƒCƒN‚È‚ç’†S‚©‚ç‚Ì‹——£‚ğŒvZ */
				skill_attack(BF_WEAPON, src, src, bl, skillid, skilllv, tick, 0x0500 | dist);
			}
		} else {
			skill_area_temp[1] = src->id;
			skill_area_temp[2] = src->x;
			skill_area_temp[3] = src->y;
			map_foreachinarea(skill_area_sub, src->m, src->x - 2, src->y - 2, src->x + 2, src->y + 2, 0,
			                  src, skillid, skilllv, tick, flag | BCT_ENEMY | 1,
			                  skill_castend_damage_id);
			status_change_start(src, SC_FLAMELAUNCHER, 0, 0, 0, 0, 10000, 0); // fire element for 10 seconds
			clif_skill_nodamage(src, src, skillid, skilllv, 1);
		}
		break;

	case KN_BOWLINGBASH:	/* ƒ{ƒEƒŠƒ“ƒOƒoƒbƒVƒ… */
		if(flag&1){
			/* ŒÂ•Ê‚Éƒ_ƒ[ƒW‚ğ—^‚¦‚é */
			if(bl->id!=skill_area_temp[1])
				skill_attack(BF_WEAPON,src,src,bl,skillid,skilllv,tick,0x0500);
		} else {
/*			int damage;
			damage = skill_attack(BF_WEAPON,src,src,bl,skillid,skilllv,tick,0);
			if(damage > 0) {*/
				int i,c;	/* ‘¼l‚©‚ç•·‚¢‚½“®‚«‚È‚Ì‚ÅŠÔˆá‚Á‚Ä‚é‰Â”\«‘å•Œø—¦‚ªˆ«‚¢‚Á‚·„ƒ */
				c = skill_get_blewcount(skillid,skilllv);
				if (map[bl->m].flag.gvg)
					c = 0;
				for(i=0;i<c;i++){
					skill_blown(src,bl,1);
					if(bl->type == BL_MOB)
						clif_fixmobpos((struct mob_data *)bl);
					else if(bl->type == BL_PET)
						clif_fixpetpos((struct pet_data *)bl);
					else
						clif_fixpos(bl);
					skill_area_temp[0]=0;
					map_foreachinarea(skill_area_sub,
						bl->m,bl->x-1,bl->y-1,bl->x+1,bl->y+1,0,
						src,skillid,skilllv,tick, flag|BCT_ENEMY ,
						skill_area_sub_count);
					if (skill_area_temp[0] > 1) break;
				}
				skill_area_temp[1]=bl->id;
				skill_area_temp[2]=bl->x;
				skill_area_temp[3]=bl->y;
				skill_attack(BF_WEAPON,src,src,bl,skillid,skilllv,tick,0);
				/* ‚»‚ÌŒãƒ^[ƒQƒbƒgˆÈŠO‚Ì”ÍˆÍ“à‚Ì“G‘S‘Ì‚Éˆ—‚ğs‚¤ */
				map_foreachinarea(skill_area_sub,
					bl->m,bl->x-1,bl->y-1,bl->x+1,bl->y+1,0,
					src,skillid,skilllv,tick, flag|BCT_ENEMY|1,
					skill_castend_damage_id);
/*				battle_damage(src,bl,damage,1);
				if(rdamage > 0)
					battle_damage(bl,src,rdamage,0);
			}*/
		}
		break;

	case ALL_RESURRECTION:		/* ƒŠƒUƒŒƒNƒVƒ‡ƒ“ */
	case PR_TURNUNDEAD:			/* ƒ^[ƒ“ƒAƒ“ƒfƒbƒh */
		if(bl->type != BL_PC && battle_check_undead(status_get_race(bl),status_get_elem_type(bl)))
			skill_attack(BF_MAGIC,src,src,bl,skillid,skilllv,tick,flag);
		else {
			map_freeblock_unlock();
			return 1;
		}
		break;

	/* –‚–@ŒnƒXƒLƒ‹ */
	case MG_SOULSTRIKE:			/* ƒ\ƒEƒ‹ƒXƒgƒ‰ƒCƒN */
	case NPC_DARKSOULSTRIKE:		/*ˆÅƒ\ƒEƒ‹ƒXƒgƒ‰ƒCƒN*/
	case MG_COLDBOLT:			/* ƒR[ƒ‹ƒhƒ{ƒ‹ƒg */
	case MG_FIREBOLT:			/* ƒtƒ@ƒCƒA[ƒ{ƒ‹ƒg */
	case MG_LIGHTNINGBOLT:		/* ƒ‰ƒCƒgƒjƒ“ƒOƒ{ƒ‹ƒg */
	case WZ_EARTHSPIKE:			/* ƒA[ƒXƒXƒpƒCƒN */
	case AL_HEAL:				/* ƒq[ƒ‹ */
	case AL_HOLYLIGHT:			/* ƒz[ƒŠ[ƒ‰ƒCƒg */
//	case MG_FROSTDIVER:			/* ƒtƒƒXƒgƒ_ƒCƒo[ */
	case WZ_JUPITEL:			/* ƒ†ƒsƒeƒ‹ƒTƒ“ƒ_[ */
	case NPC_DARKJUPITEL:			/*ˆÅƒ†ƒsƒeƒ‹*/
	case NPC_MAGICALATTACK:		/* MOB:–‚–@‘Å?U? */
	case PR_ASPERSIO:			/* ƒAƒXƒyƒ‹ƒVƒI */
//	case HW_NAPALMVULCAN:		/* ƒiƒp[ƒ€ƒoƒ‹ƒJƒ“ */
		skill_attack(BF_MAGIC,src,src,bl,skillid,skilllv,tick,flag);
		break;

	case MG_FROSTDIVER:			/* ƒtƒƒXƒgƒ_ƒCƒo[ */
	{
		struct status_change *sc_data = status_get_sc_data(bl);
		int sc_def_mdef, rate, damage;
		sc_def_mdef = status_get_sc_def_mdef(bl);
		rate = (skilllv * 3 + 35) * sc_def_mdef / 100 - (status_get_int(bl) + status_get_luk(bl)) / 15;
		rate = rate <= 5 ? 5 : rate;
		if (sc_data && sc_data[SC_FREEZE].timer != -1) {
			skill_attack(BF_MAGIC, src, src, bl, skillid, skilllv, tick, flag);
			if (sd)
				clif_skill_fail(sd, skillid, 0, 0);
			break;
		}
		damage = skill_attack(BF_MAGIC,src,src,bl,skillid,skilllv,tick,flag);
		if (status_get_hp(bl) > 0 && damage > 0 && rand() % 100 < rate) {
			status_change_start(bl, SC_FREEZE, skilllv, 0, 0, 0, skill_get_time2(skillid, skilllv), 0);
		} else if (sd) {
			clif_skill_fail(sd, skillid, 0, 0);
		}
		break;
	}

	case WZ_WATERBALL:			/* ƒEƒH[ƒ^[ƒ{[ƒ‹ */
		skill_attack(BF_MAGIC, src, src, bl, skillid, skilllv, tick, flag);
		if (skilllv > 1) {
			int cnt,range;
			range = skilllv > 5 ? 2 : skilllv / 2;
			if (sd)
				cnt = skill_count_water(src, range) - 1;
			else
				cnt = skill_get_num(skillid, skilllv) - 1;
			if (cnt > 0)
				skill_addtimerskill(src, tick + 150, bl->id, 0, 0, skillid, skilllv, cnt, flag);
		}
		break;

	case PR_BENEDICTIO:			/* ¹‘Ì~•Ÿ */
		if(status_get_race(bl)==1 || status_get_race(bl)==6)
			skill_attack(BF_MAGIC,src,src,bl,skillid,skilllv,tick,flag);
		break;

	/* –‚–@Œn”ÍˆÍUŒ‚ƒXƒLƒ‹ */
	case MG_NAPALMBEAT:			/* ƒiƒp[ƒ€ƒr[ƒg */
	case MG_FIREBALL:			/* ƒtƒ@ƒCƒ„[ƒ{[ƒ‹ */
	case WZ_SIGHTRASHER:		/* ƒTƒCƒgƒ‰ƒbƒVƒƒ[ */
		if(flag&1){
			/* ŒÂ•Ê‚Éƒ_ƒ[ƒW‚ğ—^‚¦‚é */
			if(bl->id!=skill_area_temp[1]){
				if(skillid==MG_FIREBALL){	/* ƒtƒ@ƒCƒ„[ƒ{[ƒ‹‚È‚ç’†S‚©‚ç‚Ì‹——£‚ğŒvZ */
					int dx=abs( bl->x - skill_area_temp[2] );
					int dy=abs( bl->y - skill_area_temp[3] );
					skill_area_temp[0]=((dx>dy)?dx:dy);
				}
				skill_attack(BF_MAGIC,src,src,bl,skillid,skilllv,tick,
					skill_area_temp[0]| 0x0500);
			}
		} else {
			int ar;
			skill_area_temp[0] = 0;
			skill_area_temp[1] = bl->id;
			switch (skillid) {
			case MG_NAPALMBEAT:
				ar = 1;
				/* ƒiƒp[ƒ€ƒr[ƒg‚Í•ªUƒ_ƒ[ƒW‚È‚Ì‚Å“G‚Ì”‚ğ”‚¦‚é */
				map_foreachinarea(skill_area_sub,
						bl->m, bl->x - ar, bl->y - ar, bl->x + ar, bl->y + ar, 0,
						src, skillid, skilllv, tick, flag | BCT_ENEMY,
						skill_area_sub_count);
				break;
			case MG_FIREBALL:
				ar = 2;
				skill_area_temp[2] = bl->x;
				skill_area_temp[3] = bl->y;
				/* ƒ^[ƒQƒbƒg‚ÉUŒ‚‚ğ‰Á‚¦‚é(ƒXƒLƒ‹ƒGƒtƒFƒNƒg•\¦) */
				skill_attack(BF_MAGIC, src, src, bl, skillid, skilllv, tick,
						skill_area_temp[0]);
				break;
			case WZ_SIGHTRASHER:
			default:
				ar = 3;
				bl = src;
				status_change_end(src, SC_SIGHT, -1);
				break;
			}
			if (skillid == WZ_SIGHTRASHER) {
				/* ƒXƒLƒ‹ƒGƒtƒFƒNƒg•\¦ */
				clif_skill_nodamage(src, bl, skillid, skilllv, 1);
			} else {
				/* ƒ^[ƒQƒbƒg‚ÉUŒ‚‚ğ‰Á‚¦‚é(ƒXƒLƒ‹ƒGƒtƒFƒNƒg•\¦) */
				skill_attack(BF_MAGIC, src, src, bl, skillid, skilllv, tick,
						skill_area_temp[0]);
			}
			/* ƒ^[ƒQƒbƒgˆÈŠO‚Ì”ÍˆÍ“à‚Ì“G‘S‘Ì‚Éˆ—‚ğs‚¤ */
			map_foreachinarea(skill_area_sub,
					bl->m, bl->x - ar, bl->y - ar, bl->x + ar, bl->y + ar, 0,
					src, skillid, skilllv, tick, flag | BCT_ENEMY | 1,
					skill_castend_damage_id);
		}
		break;

		case HW_NAPALMVULCAN: // Fixed By SteelViruZ
			if(flag&1){
				if(bl->id!=skill_area_temp[1]){
					skill_attack(BF_MAGIC,src,src,bl,skillid,skilllv,tick,
						skill_area_temp[0]);
				}
			}else{
				int ar=(skillid==HW_NAPALMVULCAN)?1:2;
				skill_area_temp[1]=bl->id;
				if(skillid==HW_NAPALMVULCAN){
					skill_area_temp[0]=0;
					map_foreachinarea(skill_area_sub,
						bl->m,bl->x-1,bl->y-1,bl->x+1,bl->y+1,0,
						src,skillid,skilllv,tick, flag|BCT_ENEMY ,
						skill_area_sub_count);
				}else{
					skill_area_temp[0]=0;
					skill_area_temp[2]=bl->x;
					skill_area_temp[3]=bl->y;
				}
				skill_attack(BF_MAGIC,src,src,bl,skillid,skilllv,tick,
					skill_area_temp[0] );
				map_foreachinarea(skill_area_sub,
					bl->m,bl->x-ar,bl->y-ar,bl->x+ar,bl->y+ar,0,
					src,skillid,skilllv,tick, flag|BCT_ENEMY|1,
					skill_castend_damage_id);
			}
			break;

	case WZ_FROSTNOVA:			/* ƒtƒƒXƒgƒmƒ”ƒ@ */
		//skill_castend_pos2(src, bl->x, bl->y, skillid, skilllv, tick, 0);
		//skill_attack(BF_MAGIC, src, src, bl, skillid, skilllv, tick, flag);
		map_foreachinarea(skill_attack_area, src->m, src->x-5, bl->y-5, bl->x+5, bl->y+5, 0, BF_MAGIC, src, src, skillid, skilllv, tick, flag, BCT_ENEMY);
		break;

	/* ‚»‚Ì‘¼ */
	case HT_BLITZBEAT:			/* ƒuƒŠƒbƒcƒr[ƒg */
		if(flag&1){
			/* ŒÂ•Ê‚Éƒ_ƒ[ƒW‚ğ—^‚¦‚é */
			if(bl->id!=skill_area_temp[1])
				skill_attack(BF_MISC,src,src,bl,skillid,skilllv,tick,skill_area_temp[0]|(flag&0xf00000));
		}else{
			skill_area_temp[0]=0;
			skill_area_temp[1]=bl->id;
			if(flag&0xf00000)
				map_foreachinarea(skill_area_sub,bl->m,bl->x-1,bl->y-1,bl->x+1,bl->y+1,0,
					src,skillid,skilllv,tick, flag|BCT_ENEMY ,skill_area_sub_count);
			/* ‚Ü‚¸ƒ^[ƒQƒbƒg‚ÉUŒ‚‚ğ‰Á‚¦‚é */
			skill_attack(BF_MISC,src,src,bl,skillid,skilllv,tick,skill_area_temp[0]|(flag&0xf00000));
			/* ‚»‚ÌŒãƒ^[ƒQƒbƒgˆÈŠO‚Ì”ÍˆÍ“à‚Ì“G‘S‘Ì‚Éˆ—‚ğs‚¤ */
			map_foreachinarea(skill_area_sub,
				bl->m,bl->x-1,bl->y-1,bl->x+1,bl->y+1,0,
				src,skillid,skilllv,tick, flag|BCT_ENEMY|1,
				skill_castend_damage_id);
		}
		break;

	case CR_GRANDCROSS:			/* ƒOƒ‰ƒ“ƒhƒNƒƒX */
	case NPC_DARKGRANDCROSS:		/*ˆÅƒOƒ‰ƒ“ƒhƒNƒƒX*/
		/* ƒXƒLƒ‹ƒ†ƒjƒbƒg”z’u */
		skill_castend_pos2(src,bl->x,bl->y,skillid,skilllv,tick,0);
		if(sd)
			sd->canmove_tick = tick + 1000;
		else if (src->type == BL_MOB)
			mob_changestate((struct mob_data *)src, MS_DELAY, 1000);
		break;

	case TF_THROWSTONE:			/* Î“Š‚° */
	case NPC_SMOKING:			/* ƒXƒ‚[ƒLƒ“ƒO */
		skill_attack(BF_MISC,src,src,bl,skillid,skilllv,tick,0 );
		break;

	// Celest
	case PF_SOULBURN:
	  {
		int per = skilllv < 5 ? 30 + skilllv * 10 : 70; // http://guide.ragnarok.co.kr/Jobprofessorskill.asp [updated by BLB]
		if (rand() % 100 < per) {
			clif_skill_nodamage(src, bl, skillid, skilllv, 1);
			if (skilllv == 5)
				skill_attack(BF_MAGIC, src, src, bl, skillid, skilllv, tick, 0);
			if (bl->type == BL_PC) {
				struct map_session_data *tsd = (struct map_session_data *)bl;
				if (tsd) {
					tsd->status.sp = 0;
					clif_updatestatus(tsd, SP_SP);
				}
			}
		} else {
			clif_skill_nodamage(src, src, skillid, skilllv, 1);
			if (skilllv == 5)
				skill_attack(BF_MAGIC, src, src, src, skillid, skilllv, tick, 0);
			if (sd) {
				sd->status.sp = 0;
				clif_updatestatus(sd, SP_SP);
			}
		}
		if (sd)
			pc_blockskill_start(sd, skillid, (skilllv < 5 ? 10000: 15000));
	  }
		break;

	case NPC_SELFDESTRUCTION:	/* ©”š */
	case NPC_SELFDESTRUCTION2:	/* ©”š2 */
		if (flag & 1) {
			/* ŒÂ•Ê‚Éƒ_ƒ[ƒW‚ğ—^‚¦‚é */
			if (bl->id != skill_area_temp[1])
				skill_attack(BF_MISC, src, src, bl, NPC_SELFDESTRUCTION, skilllv, tick, flag);
			/* ŒÂ•Ê‚Éƒ_ƒ[ƒW‚ğ—^‚¦‚é */
/*			if(src->type==BL_MOB){
				struct mob_data* mb = (struct mob_data*)src;
				if (mb == NULL) {
					map_freeblock_unlock();
					return 1;
				}
				mb->hp=skill_area_temp[2];
				if(bl->id!=skill_area_temp[1])
					skill_attack(BF_MISC,src,src,bl,NPC_SELFDESTRUCTION,skilllv,tick,flag );
				mb->hp=1;
			}*/
		} else {
			skill_area_temp[1] = bl->id;
			skill_area_temp[2] = status_get_hp(src);
			clif_skill_nodamage(src, src, NPC_SELFDESTRUCTION, -1, 1);
			map_foreachinarea(skill_area_sub, bl->m, bl->x-5, bl->y-5, bl->x+5, bl->y+5, 0,
					src, skillid, skilllv, tick, flag | BCT_ENEMY | 1, skill_castend_damage_id);
			battle_damage(src, src, skill_area_temp[2], 0);
/*			struct mob_data *md;
			if((md=(struct mob_data *)src)){
				skill_area_temp[1]=bl->id;
				skill_area_temp[2]=status_get_hp(src);
				clif_skill_nodamage(src,src,NPC_SELFDESTRUCTION,-1,1);
				map_foreachinarea(skill_area_sub, bl->m, bl->x-5, bl->y-5, bl->x+5, bl->y+5, 0,
				       src, skillid, skilllv, tick, flag|BCT_ENEMY|1, skill_castend_damage_id);
				battle_damage(src,src,md->hp,0);
			}*/

/*			if(src && src->type==BL_MOB){	//Add BL_PC for use as player - Valaris
				struct mob_data* md=(struct mob_data*)src;
				if (md == NULL) {
					map_freeblock_unlock();
					return 1;
				}
				md->hp=skill_area_temp[2];
				if(bl->id!=skill_area_temp[1])
					skill_attack(BF_MISC,src,src,bl,skillid,skilllv,tick,flag );
				if(md) md->hp=1;
			}
			if(src && src->type==BL_PC){
				struct map_session_data* sd=(struct map_session_data*)src; //Add player data for damage calculation - Valaris
				if(sd) sd->status.hp=skill_area_temp[2]; //Player damage calculation - Valaris
				if(bl->id!=skill_area_temp[1])
					skill_attack(BF_MISC,src,src,bl,skillid,skilllv,tick,flag );
				if(sd) sd->status.hp=1;	//Player calculation damage
			}

		}else if(src && bl){
			skill_area_temp[1]=bl->id;
			if(skilllv==99) skill_area_temp[2]=999999;
			else skill_area_temp[2]=status_get_hp(src);
			// ‚Ü‚¸ƒ^[ƒQƒbƒg‚ÉUŒ‚‚ğ‰Á‚¦‚é
			skill_attack(BF_MISC,src,src,bl,skillid,skilllv,tick,flag );
			// ‚»‚ÌŒãƒ^[ƒQƒbƒgˆÈŠO‚Ì”ÍˆÍ“à‚Ì“G‘S‘Ì‚Éˆ—‚ğs‚¤
			map_foreachinarea(skill_area_sub,
				bl->m,bl->x-5,bl->y-5,bl->x+5,bl->y+5,0,
				src,skillid,skilllv,tick, flag|BCT_ALL|1,
				skill_castend_damage_id);
				battle_damage(src,src,1,0);

				}

		if(src && src->type==BL_PC){				// Remove sight fireball after res - Valaris
		if(sd) sd->status.option = 0x0000;
		if((sd) && (sd->status.class ==   13 || sd->status.class ==   21 ||
		            sd->status.class == 4014 || sd->status.class == 4022 ||
		            sd->status.class == 4036 || sd->status.class == 4044))
			pc_setoption(sd,sd->status.option | 0x0020);*/
		}
		break;

	/* HP‹zû/HP‹zû–‚–@ */
	case NPC_BLOODDRAIN:
	case NPC_ENERGYDRAIN:
		{
			int heal;
			heal = skill_attack((skillid==NPC_BLOODDRAIN)?BF_WEAPON:BF_MAGIC,src,src,bl,skillid,skilllv,tick,flag);
			if( heal > 0 ){
				struct block_list tbl;
				tbl.id = 0;
				tbl.m = src->m;
				tbl.x = src->x;
				tbl.y = src->y;
				clif_skill_nodamage(&tbl,src,AL_HEAL,heal,1);
				battle_heal(NULL,src,heal,0,0);
			}
		}
		break;

	// unknown skills [Celest]
	case NPC_BIND:
	case NPC_EXPLOSIONSPIRITS:
	case NPC_INCAGI:
		clif_skill_nodamage(src, bl, skillid, skilllv, 1);
		break;

	case 0:
		if(sd) {
			if(flag&3){
				if(bl->id!=skill_area_temp[1])
					skill_attack(BF_WEAPON,src,src,bl,skillid,skilllv,tick,0x0500);
			}
			else{
				int ar=sd->splash_range;
				skill_area_temp[1]=bl->id;
				map_foreachinarea(skill_area_sub,
					bl->m, bl->x - ar, bl->y - ar, bl->x + ar, bl->y + ar, 0,
					src, skillid, skilllv, tick, flag | BCT_ENEMY | 1,
					skill_castend_damage_id);
			}
		}
		break;
	default:
		printf("Unknown skill used:%d\n",skillid);
		map_freeblock_unlock();
		return 1;
	}
	if(sc_data) {
		if (sc_data[SC_MAGICPOWER].timer != -1 && skillid != HW_MAGICPOWER)	//ƒ}ƒWƒbƒNƒpƒ?‚Ì?‰ÊI—¹
			status_change_end(src,SC_MAGICPOWER,-1);
	}
	map_freeblock_unlock();

	return 0;
}

/*==========================================
 * ƒXƒLƒ‹g—pi‰r¥Š®—¹AIDw’èx‰‡Œnj
 *------------------------------------------
 */
int skill_castend_nodamage_id(struct block_list *src, struct block_list *bl, int skillid, int skilllv, unsigned int tick, int flag)
{
	struct map_session_data *sd = NULL;
	struct map_session_data *dstsd = NULL;
	struct mob_data *md = NULL;
	struct mob_data *dstmd = NULL;
	int i, abra_skillid = 0, abra_skilllv;
	int sc_def_vit, sc_def_mdef;
	int sc_dex, sc_luk;
	//ƒNƒ‰ƒXƒ`ƒFƒ“ƒW—pƒ{ƒXƒ‚ƒ“ƒXƒ^[ID
	int changeclass[]= {1038,1039,1046,1059,1086,1087,1112,1115,
	                    1157,1159,1190,1272,1312,1373,1492};
	int poringclass[] = {1002};

	if (skillid > 0 && skilllv <= 0)
		return 0; // celest

	nullpo_retr(1, src);
	nullpo_retr(1, bl);

	if (src->type == BL_PC)
		sd = (struct map_session_data *)src;
	else if (src->type == BL_MOB)
		md = (struct mob_data *)src;

	sc_dex = status_get_mdef(bl);
	sc_luk = status_get_luk(bl);
	sc_def_vit = status_get_sc_def_vit(bl);
	sc_def_mdef = status_get_sc_def_mdef(bl);

	if (bl->type == BL_PC) {
		nullpo_retr(1, dstsd = (struct map_session_data *)bl);
	} else if (bl->type == BL_MOB) {
		nullpo_retr(1, dstmd = (struct mob_data *)bl);
	}

	if (bl == NULL || bl->prev == NULL)
		return 1;
	if (sd && pc_isdead(sd))
		return 1;
	if (dstsd && pc_isdead(dstsd) && skillid != ALL_RESURRECTION)
		return 1;
	if (status_get_class(bl) == 1288)
		return 1;
	if (sd && skillnotok(skillid, sd)) // [MouseJstr]
		return 0;

	map_freeblock_lock();
	switch(skillid)
	{
	case AL_HEAL:				/* ƒq[ƒ‹ */
	  {
		int heal=skill_calc_heal( src, skilllv );
		int heal_get_jobexp;
		int skill;
		struct pc_base_job s_class;

		if( dstsd && dstsd->special_state.no_magic_damage )
			heal=0;	/* ‰©‹àå³ƒJ[ƒhiƒq[ƒ‹—Ê‚Oj */
		if (sd){
			s_class = pc_calc_base_job(sd->status.class);
		if((skill=pc_checkskill(sd,HP_MEDITATIO))>0) // ƒƒfƒBƒeƒCƒeƒBƒI
			heal += heal*skill*2/100; // calculation fixed by Yor
			if(sd && dstsd && sd->status.partner_id == dstsd->status.char_id && s_class.job == 23 && sd->status.sex == 0) //©•ª‚à‘ÎÛ‚àPCA‘ÎÛ‚ª©•ª‚Ìƒp[ƒgƒi[A©•ª‚ªƒXƒpƒmƒrA©•ª‚ªŠ‚È‚ç
				heal = heal*2;	//ƒXƒpƒmƒr‚Ì‰Å‚ª’U“ß‚Éƒq[ƒ‹‚·‚é‚Æ2”{‚É‚È‚é
		}

		clif_skill_nodamage(src,bl,skillid,heal,1);
		heal_get_jobexp = battle_heal(NULL,bl,heal,0,0);

		// JOBŒoŒ±’lŠl“¾
		if(src->type == BL_PC && bl->type==BL_PC && heal > 0 && src != bl && battle_config.heal_exp > 0){
			heal_get_jobexp = heal_get_jobexp * battle_config.heal_exp / 100;
			if(heal_get_jobexp <= 0)
				heal_get_jobexp = 1;
			pc_gainexp((struct map_session_data *)src,0,heal_get_jobexp);
		}
	  }
		break;

	case ALL_RESURRECTION:		/* ƒŠƒUƒŒƒNƒVƒ‡ƒ“ */
		if (bl->type == BL_PC) {
			int per = 0;
			struct map_session_data *tsd = (struct map_session_data*)bl;
			if (tsd == NULL) {
				map_freeblock_unlock();
				return 1;
			}
			if ((map[bl->m].flag.pvp) && tsd->pvp_point < 0)
				break;			/* PVP‚Å•œŠˆ•s‰Â”\ó‘Ô */

			if(pc_isdead(tsd)){	/* €–S”»’è */
				clif_skill_nodamage(src,bl,skillid,skilllv,1);
				switch(skilllv){
				case 1: per=10; break;
				case 2: per=30; break;
				case 3: per=50; break;
				case 4: per=80; break;
				}
				tsd->status.hp=tsd->status.max_hp*per/100;
				if(tsd->status.hp<=0) tsd->status.hp=1;
				if(tsd->special_state.restart_full_recover ){	/* ƒIƒVƒŠƒXƒJ[ƒh */
					tsd->status.hp=tsd->status.max_hp;
					tsd->status.sp=tsd->status.max_sp;
				}
				pc_setstand(tsd);
				if(battle_config.pc_invincible_time > 0)
					pc_setinvincibletimer(tsd,battle_config.pc_invincible_time);
				clif_updatestatus(tsd,SP_HP);
				clif_resurrection(&tsd->bl,1);
				if(src != bl && sd && battle_config.resurrection_exp > 0) {
					int exp = 0,jexp = 0;
					int lv = tsd->status.base_level - sd->status.base_level, jlv = tsd->status.job_level - sd->status.job_level;
					if(lv > 0) {
						exp = (int)((double)tsd->status.base_exp * (double)lv * (double)battle_config.resurrection_exp / 1000000.);
						if(exp < 1) exp = 1;
					}
					if(jlv > 0) {
						jexp = (int)((double)tsd->status.job_exp * (double)lv * (double)battle_config.resurrection_exp / 1000000.);
						if(jexp < 1) jexp = 1;
					}
					if(exp > 0 || jexp > 0)
						pc_gainexp(sd,exp,jexp);
				}
			}
		}
		break;

	case AL_DECAGI:			/* ‘¬“xŒ¸­ */
		if (bl->type==BL_PC && ((struct map_session_data *)bl)->special_state.no_magic_damage )
			break;
		if (rand() % 100 < (50 + skilllv * 3+(status_get_lv(src) + status_get_int(src) / 5) - sc_def_mdef)) {
			clif_skill_nodamage(src,bl,skillid,skilllv,1);
			if (bl->type == BL_PC)
				i = skill_get_time(skillid,skilllv) / 2;
			else
				i = skill_get_time(skillid,skilllv);
			status_change_start(bl,SkillStatusChangeTable[skillid],skilllv,0,0,0,i,0);
		}
		break;

	case AL_CRUCIS:
		if(flag&1) {
			int race = status_get_race(bl);
			int ele = status_get_elem_type(bl);
			if (battle_check_target(src,bl,BCT_ENEMY) && (race == 6 || battle_check_undead(race,ele))) {
				if (rand() % 100 < 25 + skilllv * 2 + status_get_lv(src) - status_get_lv(bl))
					status_change_start(bl, SkillStatusChangeTable[skillid], skilllv, 0, 0, 0, 0, 0);
			}
		}
		else {
			int range = 15;
			clif_skill_nodamage(src,bl,skillid,skilllv,1);
			map_foreachinarea(skill_area_sub,
				src->m,src->x-range,src->y-range,src->x+range,src->y+range,0,
				src,skillid,skilllv,tick, flag|BCT_ENEMY|1,
				skill_castend_nodamage_id);
		}
		break;

	case PR_LEXDIVINA:		/* ƒŒƒbƒNƒXƒfƒBƒr[ƒi */
		{
			struct status_change *sc_data = status_get_sc_data(bl);
			clif_skill_nodamage(src,bl,skillid,skilllv,1);
			if( bl->type==BL_PC && ((struct map_session_data *)bl)->special_state.no_magic_damage )
				break;
			if(sc_data && sc_data[SC_DIVINA].timer != -1)
				status_change_end(bl,SC_DIVINA,-1);
			else if( rand()%100 < sc_def_vit ) {
				status_change_start(bl,SkillStatusChangeTable[skillid],skilllv,0,0,0,skill_get_time(skillid,skilllv),0);
			}
		}
		break;
	case SA_ABRACADABRA:
		//require 1 yellow gemstone even with mistress card or Into the Abyss
		if ((i = pc_search_inventory(sd, 715)) < 0) { //bug fixed by Lupus (item pos can be 0, too!)
			clif_skill_fail(sd, sd->skillid, 0, 0);
			break;
		}
		//pc_delitem(sd, pc_search_inventory(sd, 715), 1, 0);
		pc_delitem(sd, i, 1, 0);
		//
		do{
			abra_skillid=skill_abra_dataset(skilllv);
		}while(abra_skillid == 0);
		abra_skilllv=skill_get_max(abra_skillid)>pc_checkskill(sd,SA_ABRACADABRA)?pc_checkskill(sd,SA_ABRACADABRA):skill_get_max(abra_skillid);
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		sd->skillitem=abra_skillid;
		sd->skillitemlv=abra_skilllv;
		clif_item_skill(sd,abra_skillid,abra_skilllv,"ƒAƒuƒ‰ƒJƒ_ƒuƒ‰");
		break;
	case SA_COMA:
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		if( bl->type==BL_PC && ((struct map_session_data *)bl)->special_state.no_magic_damage )
			break;
		if(dstsd){
			dstsd->status.hp=1;
			dstsd->status.sp=1;
			clif_updatestatus(dstsd,SP_HP);
			clif_updatestatus(dstsd,SP_SP);
		}
		if(dstmd) dstmd->hp=1;
		break;
	case SA_FULLRECOVERY:
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		if( bl->type==BL_PC && ((struct map_session_data *)bl)->special_state.no_magic_damage )
			break;
		if(dstsd) pc_heal(dstsd,dstsd->status.max_hp,dstsd->status.max_sp);
		if(dstmd) dstmd->hp=status_get_max_hp(&dstmd->bl);
		break;
	case SA_SUMMONMONSTER:
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		if (sd) mob_once_spawn(sd,map[sd->bl.m].name,sd->bl.x,sd->bl.y,"--ja--",-1,1,"");
		break;
	case SA_LEVELUP:
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		if (sd && pc_nextbaseexp(sd)) pc_gainexp(sd,pc_nextbaseexp(sd)*10/100,0);
		break;

	case SA_INSTANTDEATH:
		clif_skill_nodamage(src, bl, skillid, skilllv, 1);
		if (sd)
			pc_damage(NULL, sd, sd->status.max_hp);
		break;

	case SA_QUESTION:
	case SA_GRAVITY:
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		break;
	case SA_CLASSCHANGE:
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		if(dstmd) mob_class_change(dstmd,changeclass);
		break;
	case SA_MONOCELL:
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		if(dstmd) mob_class_change(dstmd,poringclass);
		break;
	case SA_DEATH:
		clif_skill_nodamage(src, bl, skillid, skilllv, 1);
		if (dstsd)
			pc_damage(NULL, dstsd, dstsd->status.max_hp);
		if (dstmd)
			mob_damage(NULL, dstmd, dstmd->hp, 1);
		break;
	case SA_REVERSEORCISH:
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		if (dstsd) pc_setoption(dstsd,dstsd->status.option|0x0800);
		break;
	case SA_FORTUNE:
		clif_skill_nodamage(src, bl, skillid, skilllv, 1);
		if (sd)
			pc_getzeny(sd, status_get_lv(bl) * 100);
		break;
	case SA_TAMINGMONSTER:
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		if (dstmd){
			for(i=0;i<MAX_PET_DB;i++){
				if(dstmd->class == pet_db[i].class){
					pet_catch_process1(sd,dstmd->class);
					break;
				}
			}
		}
		break;
	case AL_INCAGI:			/* ‘¬“x‘‰Á */
	case AL_BLESSING:		/* ƒuƒŒƒbƒVƒ“ƒO */
	case PR_SLOWPOISON:
	case PR_IMPOSITIO:		/* ƒCƒ€ƒ|ƒVƒeƒBƒIƒ}ƒkƒX */
	case PR_LEXAETERNA:		/* ƒŒƒbƒNƒXƒG[ƒeƒ‹ƒi */
	case PR_SUFFRAGIUM:		/* ƒTƒtƒ‰ƒMƒEƒ€ */
	case PR_BENEDICTIO:		/* ¹‘Ì~•Ÿ */
	case CR_PROVIDENCE:		/* ƒvƒƒ”ƒBƒfƒ“ƒX */
		if( bl->type==BL_PC && ((struct map_session_data *)bl)->special_state.no_magic_damage ){
			clif_skill_nodamage(src,bl,skillid,skilllv,1);
		}else{
			status_change_start(bl,SkillStatusChangeTable[skillid],skilllv,0,0,0,skill_get_time(skillid,skilllv),0 );
			clif_skill_nodamage(src,bl,skillid,skilllv,1);
		}
		break;

	case CG_MARIONETTE: /* ƒ}ƒŠƒIƒlƒbƒgƒRƒ“ƒgƒ?ƒ‹ */
		if (sd && dstsd) {
			struct status_change *sc_data = status_get_sc_data(src);
			struct status_change *tsc_data = status_get_sc_data(bl);
			int sc = SkillStatusChangeTable[skillid];
			int sc2 = SC_MARIONETTE2;

			if((dstsd->bl.type != BL_PC) ||
			   (sd->bl.id == dstsd->bl.id) ||
			   (!sd->status.party_id) ||
			   (sd->status.party_id != dstsd->status.party_id)) {
				clif_skill_fail(sd, skillid, 0, 0);
				map_freeblock_unlock();
				return 1;
			}
			if (sc_data && tsc_data){
				if(sc_data[sc].timer == -1 && tsc_data[sc2].timer == -1) {
					status_change_start (src,sc,skilllv,0,bl->id,0,skill_get_time(skillid,skilllv),0);
					status_change_start (bl,sc2,skilllv,0,src->id,0,skill_get_time(skillid,skilllv),0);
				}
				else if (sc_data[sc].timer != -1 && tsc_data[sc2].timer != -1 &&
				sc_data[sc].val3 == bl->id && tsc_data[sc2].val3 == src->id) {
					status_change_end(src, sc, -1);
					status_change_end(bl, sc2, -1);
				}
				else {
					clif_skill_fail(sd, skillid, 0, 0);
					map_freeblock_unlock();
					return 1;
				}
				clif_skill_nodamage(src,bl,skillid,skilllv,1);
			}
		}
		break;

	case SA_FLAMELAUNCHER:	// added failure chance and chance to break weapon if turned on [Valaris]
	case SA_FROSTWEAPON:
	case SA_LIGHTNINGLOADER:
	case SA_SEISMICWEAPON:
		if(bl->type==BL_PC && ((struct map_session_data *)bl)->special_state.no_magic_damage ){
			clif_skill_nodamage(src,bl,skillid,skilllv,0);
			break;
		}
		if(bl->type==BL_PC) {
			struct map_session_data *sd2=(struct map_session_data *)bl;
			if(sd2->status.weapon==0 || sd2->sc_data[SC_FLAMELAUNCHER].timer!=-1 || sd2->sc_data[SC_FROSTWEAPON].timer!=-1 ||
				sd2->sc_data[SC_LIGHTNINGLOADER].timer!=-1 || sd2->sc_data[SC_SEISMICWEAPON].timer!=-1 ||
					sd2->sc_data[SC_ENCPOISON].timer!=-1) {
				clif_skill_fail(sd, skillid, 0, 0);
				clif_skill_nodamage(src, bl, skillid, skilllv, 0);
				break;
			}
		}
		if (skilllv < 5 && rand() % 100 > (60 + skilllv * 10)) {
			clif_skill_fail(sd, skillid, 0, 0);
			clif_skill_nodamage(src,bl,skillid,skilllv,0);
			if(bl->type==BL_PC && battle_config.equipment_breaking) {
				struct map_session_data *sd2=(struct map_session_data *)bl;
				if(sd!=sd2) clif_displaymessage(sd->fd, msg_txt(537)); // You broke target's weapon.
				pc_breakweapon(sd2);
			}
			break;
		}
		else {
			status_change_start(bl,SkillStatusChangeTable[skillid],skilllv,0,0,0,skill_get_time(skillid,skilllv),0 );
			clif_skill_nodamage(src,bl,skillid,skilllv,1);
		}
		break;

	case PR_ASPERSIO:		/* ƒAƒXƒyƒ‹ƒVƒI */
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		if( bl->type==BL_PC && ((struct map_session_data *)bl)->special_state.no_magic_damage )
			break;
		if(bl->type==BL_MOB)
			break;
		status_change_start(bl,SkillStatusChangeTable[skillid],skilllv,0,0,0,skill_get_time(skillid,skilllv),0 );
		break;
	case PR_KYRIE:			/* ƒLƒŠƒGƒGƒŒƒCƒ\ƒ“ */
		clif_skill_nodamage(bl,bl,skillid,skilllv,1);
		if( bl->type==BL_PC && ((struct map_session_data *)bl)->special_state.no_magic_damage )
			break;
		status_change_start(bl,SkillStatusChangeTable[skillid],skilllv,0,0,0,skill_get_time(skillid,skilllv),0 );
		break;
	case KN_AUTOCOUNTER:		/* ƒI[ƒgƒJƒEƒ“ƒ^[ */
	case KN_TWOHANDQUICKEN:	/* ƒc[ƒnƒ“ƒhƒNƒCƒbƒPƒ“ */
	case CR_SPEARQUICKEN:	/* ƒXƒsƒAƒNƒCƒbƒPƒ“ */
	case CR_REFLECTSHIELD:
	case AS_POISONREACT:	/* ƒ|ƒCƒYƒ“ƒŠƒAƒNƒg */
	case MC_LOUD:			/* ƒ‰ƒEƒhƒ{ƒCƒX */
	case MG_ENERGYCOAT:		/* ƒGƒiƒW[ƒR[ƒg */
	//case SM_ENDURE:			/* ƒCƒ“ƒfƒ…ƒA */
	case MG_SIGHT:			/* ƒTƒCƒg */
	case AL_RUWACH:			/* ƒ‹ƒAƒt */
	case MO_EXPLOSIONSPIRITS:	// ”š—ô”g“®
	case MO_STEELBODY:		// ‹à„
	case LK_AURABLADE:		/* ƒI[ƒ‰ƒuƒŒ[ƒh */
	case LK_PARRYING:		/* ƒpƒŠƒCƒ“ƒO */
	case LK_CONCENTRATION:	/* ƒRƒ“ƒZƒ“ƒgƒŒ[ƒVƒ‡ƒ“ */
	//case LK_BERSERK:		/* ƒo[ƒT[ƒN */
	case WS_CARTBOOST:		/* ƒJ[ƒgƒu[ƒXƒg */
	case SN_SIGHT:			/* ƒgƒDƒ‹[ƒTƒCƒg */
	case WS_MELTDOWN:		/* ƒƒ‹ƒgƒ_ƒEƒ“ */
	case ST_REJECTSWORD:	/* ƒŠƒWƒFƒNƒgƒ\[ƒh */
	case HW_MAGICPOWER:		/* –‚–@—Í‘• */
	case PF_MEMORIZE:		/* ƒƒ‚ƒ‰ƒCƒY */
	case PA_SACRIFICE:
	case ASC_EDP:			// [Celest]
	case CG_MOONLIT:		/* Œ–¾‚è‚Ìò‚É—‚¿‚é‰Ô‚Ñ‚ç */
		clif_skill_nodamage(src, bl, skillid, skilllv, 1);
		status_change_start(bl, SkillStatusChangeTable[skillid], skilllv, 0, 0, 0, skill_get_time(skillid, skilllv), 0);
		break;
	case HP_ASSUMPTIO:
		if (flag&1)
			status_change_start(bl,SkillStatusChangeTable[skillid],skilllv,0,0,0,skill_get_time(skillid,skilllv),0 );
		else // Due to the patch change, this skill now affects people around you.
		{
			clif_skill_nodamage(src,bl,skillid,skilllv,1);
			map_foreachinarea(skill_area_sub,
				bl->m, bl->x-1, bl->y-1, bl->x+1, bl->y+1, BL_PC,
				src, skillid, skilllv, tick, flag|BCT_ALL|1,
				skill_castend_nodamage_id);
		}
		break;
	case SM_ENDURE:			/* ƒCƒ“ƒfƒ…ƒA */
		clif_skill_nodamage(src, bl, skillid, skilllv, 1);
		status_change_start(bl, SkillStatusChangeTable[skillid], skilllv, 0, 0, 0, skill_get_time(skillid, skilllv), 0);
		if (sd)
			pc_blockskill_start(sd, skillid, 10000);
		break;

	case SM_AUTOBERSERK:	// Celest
	  {
		struct status_change *tsc_data = status_get_sc_data(bl);
		int sc=SkillStatusChangeTable[skillid];
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		if( tsc_data ){
			if( tsc_data[sc].timer == -1 )
				status_change_start(bl,sc,skilllv,0,0,0,0,0);
			else
				status_change_end(bl, sc, -1);
		}
	  }
		break;

	case AS_ENCHANTPOISON: // Prevent spamming [Valaris]
		if (bl->type == BL_PC) {
			struct map_session_data *sd2 = (struct map_session_data *)bl;
			if (sd2->sc_data[SC_FLAMELAUNCHER].timer != -1 || sd2->sc_data[SC_FROSTWEAPON].timer != -1 ||
			    sd2->sc_data[SC_LIGHTNINGLOADER].timer != -1 || sd2->sc_data[SC_SEISMICWEAPON].timer != -1 ||
			    sd2->sc_data[SC_ENCPOISON].timer != -1) {
				clif_skill_nodamage(src, bl, skillid, skilllv, 0);
				clif_skill_fail(sd, skillid, 0, 0);
				break;
			}
		}
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		status_change_start(bl,SkillStatusChangeTable[skillid],skilllv,0,0,0,skill_get_time(skillid,skilllv),0 );
		break;
	case LK_TENSIONRELAX:	/* ƒeƒ“ƒVƒ‡ƒ“ƒŠƒ‰ƒbƒNƒX */
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		pc_setsit(sd);
		clif_sitting(sd);
		status_change_start(bl,SkillStatusChangeTable[skillid],skilllv,0,0,0,skill_get_time(skillid,skilllv),0 );
		break;
	case LK_BERSERK: /* ƒo[ƒT[ƒN */ // Bug fixing by [Yor]
		clif_skill_nodamage(src, bl, skillid, skilllv, 1);
		status_change_start(bl, SkillStatusChangeTable[skillid], skilllv, 0, 0, 0, skill_get_time(skillid, skilllv), 0);
		//sd->status.hp += (sd->status.max_hp * 3 - sd->status.hp);
		break;
	case MC_CHANGECART:
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		break;
	case AC_CONCENTRATION:	/* W’†—ÍŒüã */
	  {
		int range = 1;
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		status_change_start(bl,SkillStatusChangeTable[skillid],skilllv,0,0,0,skill_get_time(skillid,skilllv),0 );
		map_foreachinarea( status_change_timer_sub,
			src->m, src->x-range, src->y-range, src->x+range,src->y+range,0,
			src,SkillStatusChangeTable[skillid],tick);
	  }
		break;
	case SM_PROVOKE:		/* ƒvƒƒ{ƒbƒN */
		{
			struct status_change *sc_data = status_get_sc_data(bl);

			/* MVPmob‚Æ•s€‚É‚ÍŒø‚©‚È‚¢ */
			if((bl->type==BL_MOB && status_get_mode(bl) & 0x20) || battle_check_undead(status_get_race(bl),status_get_elem_type(bl))) //•s€‚É‚ÍŒø‚©‚È‚¢
			{
				map_freeblock_unlock();
				return 1;
			}

			clif_skill_nodamage(src,bl,skillid,skilllv,1);
			status_change_start(bl,SkillStatusChangeTable[skillid],skilllv,0,0,0,skill_get_time(skillid,skilllv),0 );

			if(dstmd && dstmd->skilltimer!=-1 && dstmd->state.skillcastcancel)	// ‰r¥–WŠQ
				skill_castcancel(bl,0);
			if(dstsd && dstsd->skilltimer!=-1 && (!dstsd->special_state.no_castcancel || map[bl->m].flag.gvg)
				&& dstsd->state.skillcastcancel && !dstsd->special_state.no_castcancel2)
				skill_castcancel(bl,0);

			if(sc_data){
				if(sc_data[SC_FREEZE].timer!=-1)
					status_change_end(bl,SC_FREEZE,-1);
				if(sc_data[SC_STONE].timer!=-1 && sc_data[SC_STONE].val2==0)
					status_change_end(bl,SC_STONE,-1);
				if(sc_data[SC_SLEEP].timer!=-1)
					status_change_end(bl,SC_SLEEP,-1);
			}

			if (dstmd) {
				int range = skill_get_range(skillid, skilllv);
				if (range < 0)
					range = status_get_range(src) - (range + 1);
				dstmd->state.provoke_flag = src->id;
				mob_target(dstmd, src, range);
			}
		}
		break;

	case CR_DEVOTION:		/* ƒfƒBƒ{[ƒVƒ‡ƒ“ */
		if (sd && dstsd) {
			//“]¶‚â—{q‚Ìê‡‚ÌŒ³‚ÌE‹Æ‚ğZo‚·‚é
			struct pc_base_job dst_s_class = pc_calc_base_job(dstsd->status.class);

			int lv = sd->status.base_level-dstsd->status.base_level;
			lv = (lv < 0) ? -lv : lv;
			if ((dstsd->bl.type != BL_PC)	// ‘Šè‚ÍPC‚¶‚á‚È‚¢‚Æ‚¾‚ß
			    || (sd->bl.id == dstsd->bl.id)	// ‘Šè‚ª©•ª‚Í‚¾‚ß
			    || (lv > battle_config.devotion_level_difference)	// ƒŒƒxƒ‹·}10‚Ü‚Å
			    || (!sd->status.party_id && !sd->status.guild_id)	// PT‚É‚àƒMƒ‹ƒh‚É‚àŠ‘®–³‚µ‚Í‚¾‚ß
			    || ((sd->status.party_id != dstsd->status.party_id)	// “¯‚¶ƒp[ƒeƒB[‚©A
			     && (sd->status.guild_id != dstsd->status.guild_id))	// “¯‚¶ƒMƒ‹ƒh‚¶‚á‚È‚¢‚Æ‚¾‚ß
			    || (dst_s_class.job == 14 || dst_s_class.job == 21)) {	// ƒNƒ‹ƒZ‚¾‚ß
				clif_skill_fail(sd, skillid, 0, 0);
				map_freeblock_unlock();
				return 1;
			}
			for(i=0;i<skilllv;i++){
				if (!sd->dev.val1[i]) {		// ‹ó‚«‚ª‚ ‚Á‚½‚ç“ü‚ê‚é
					sd->dev.val1[i] = bl->id;
					sd->dev.val2[i] = bl->id;
					break;
				} else if (i == skilllv-1) {		// ‹ó‚«‚ª‚È‚©‚Á‚½
					clif_skill_fail(sd, skillid, 0, 0);
					map_freeblock_unlock();
					return 1;
				}
			}
			clif_skill_nodamage(src,bl,skillid,skilllv,1);
			clif_devotion(sd,bl->id);
			status_change_start(bl,SkillStatusChangeTable[skillid],src->id,1,0,0,1000*(15+15*skilllv),0 );
		} else
			clif_skill_fail(sd, skillid, 0, 0);
		break;
	case MO_CALLSPIRITS:	// ‹CŒ÷
		if(sd) {
			clif_skill_nodamage(src,bl,skillid,skilllv,1);
			pc_addspiritball(sd,skill_get_time(skillid,skilllv),skilllv);
		}
		break;
	case CH_SOULCOLLECT:	// ‹¶‹CŒ÷
		if(sd) {
			clif_skill_nodamage(src,bl,skillid,skilllv,1);
			for(i=0;i<5;i++)
				pc_addspiritball(sd,skill_get_time(skillid,skilllv),5);
		}
		break;
	case MO_BLADESTOP:	// ”’næ‚è
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		status_change_start(src,SkillStatusChangeTable[skillid],skilllv,0,0,0,skill_get_time(skillid,skilllv),0 );
		break;
	case MO_ABSORBSPIRITS:	// ‹C’D
		i = 0;
		if (dstsd) {
			if ((sd && sd == dstsd) || map[src->m].flag.pvp || map[src->m].flag.gvg) {
				if (dstsd->spiritball > 0) {
					clif_skill_nodamage(src, bl, skillid, skilllv, 1);
					i = dstsd->spiritball * 7;
					pc_delspiritball(dstsd, dstsd->spiritball, 0);
					if (i > 0x7FFF)
						i = 0x7FFF;
					if (sd && sd->status.sp + i > sd->status.max_sp)
						i = sd->status.max_sp - sd->status.sp;
					}
			}
		} else if (dstmd) { //‘ÎÛ‚ªƒ‚ƒ“ƒXƒ^[‚Ìê‡
			//20%‚ÌŠm—¦‚Å‘ÎÛ‚ÌLv*2‚ÌSP‚ğ‰ñ•œ‚·‚éB¬Œ÷‚µ‚½‚Æ‚«‚Íƒ^[ƒQƒbƒg(ƒĞß„Dß)ƒĞ¹Ş¯Â!!
			if (rand() % 100 < 20){
				i = 2 * mob_db[dstmd->class].lv;
				mob_target(dstmd, src, 0);
			}
		}
		if (i && sd) {
			sd->status.sp += i;
			clif_heal(sd->fd, SP_SP, i);
		} else
			clif_skill_nodamage(src, bl, skillid, skilllv, 0);
		break;

	case AC_MAKINGARROW:			/* –îì¬ */
		if (sd) {
			clif_arrow_create_list(sd);
			clif_skill_nodamage(src,bl,skillid,skilllv,1);
		}
		break;

	case AM_PHARMACY:			/* ƒ|[ƒVƒ‡ƒ“ì¬ */
	case CR_ALCHEMY: // We made that Alchemy dont require any bottle nor medecine bowl. [Aalye] from freya' forum
		if (sd) {
			clif_skill_produce_mix_list(sd, 32);
			clif_skill_nodamage(src, bl, skillid, skilllv, 1);
		}
		break;
	case WS_CREATECOIN:				/* ƒNƒŠƒGƒCƒgƒRƒCƒ“ */
		if (sd) {
			clif_skill_produce_mix_list(sd,64);
			clif_skill_nodamage(src,bl,skillid,skilllv,1);
		}
		break;
	case WS_CREATENUGGET:			/* ‰ò»‘¢ */
		if (sd) {
			clif_skill_produce_mix_list(sd,128);
			clif_skill_nodamage(src,bl,skillid,skilllv,1);
		}
		break;
	case ASC_CDP: // [DracoRPG]
		// notes: success rate (from emperium.org) = 20 + [(20*Dex)/50] + [(20*Luk)/100]
		if (sd) {
			clif_skill_produce_mix_list(sd,256);
			clif_skill_nodamage(src,bl,skillid,skilllv,1);
		}
		break;
	case BS_HAMMERFALL:		/* ƒnƒ“ƒ}[ƒtƒH[ƒ‹ */
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		if( bl->type==BL_PC && ((struct map_session_data *)bl)->special_state.no_weapon_damage )
			break;
		if( rand()%100 < (20+ 10*skilllv)*sc_def_vit/100 ) {
			status_change_start(bl,SC_STAN,skilllv,0,0,0,skill_get_time2(skillid,skilllv),0);
		}
		break;

	case RG_RAID:			/* ƒTƒvƒ‰ƒCƒYƒAƒ^ƒbƒN */
	case ASC_METEORASSAULT:	/* ƒƒeƒIƒAƒTƒ‹ƒg */ // Meteor Assault skill fix (thanks to [Mikey] from freya's bug report)
		clif_skill_nodamage(src, bl, skillid, skilllv, 1);
	  {
		int x = bl->x, y = bl->y;
		int ar = 1;
		if (skillid == ASC_METEORASSAULT) // Meteor Assault skill fix (thanks to [Mikey] from freya's bug report)
			ar = 2;
		skill_area_temp[1] = bl->id;
		skill_area_temp[2] = x;
		skill_area_temp[3] = y;
		map_foreachinarea(skill_area_sub,
			bl->m, x - ar, y - ar, x + ar, y + ar, 0,
			src, skillid, skilllv, tick, flag | BCT_ENEMY | 1,
			skill_castend_damage_id);
	  }
		if (skillid == RG_RAID)
			status_change_end(src, SC_HIDING, -1);	// ƒnƒCƒfƒBƒ“ƒO‰ğœ
		break;

	case KN_BRANDISHSPEAR:	/*ƒuƒ‰ƒ“ƒfƒBƒbƒVƒ…ƒXƒsƒA*/
		{
			int c,n=4,ar;
			int dir = map_calc_dir(src,bl->x,bl->y);
			struct square tc;
			int x=bl->x,y=bl->y;
			ar=skilllv/3;
			skill_brandishspear_first(&tc,dir,x,y);
			skill_brandishspear_dir(&tc,dir,4);
			/* ”ÍˆÍ‡C */
			if(skilllv == 10){
				for(c=1;c<4;c++){
					map_foreachinarea(skill_area_sub,
						bl->m,tc.val1[c],tc.val2[c],tc.val1[c],tc.val2[c],0,
						src,skillid,skilllv,tick, flag|BCT_ENEMY|n,
						skill_castend_damage_id);
				}
			}
			/* ”ÍˆÍ‡B‡A */
			if(skilllv > 6){
				skill_brandishspear_dir(&tc,dir,-1);
				n--;
			}else{
				skill_brandishspear_dir(&tc,dir,-2);
				n-=2;
			}

			if(skilllv > 3){
				for(c=0;c<5;c++){
					map_foreachinarea(skill_area_sub,
						bl->m,tc.val1[c],tc.val2[c],tc.val1[c],tc.val2[c],0,
						src,skillid,skilllv,tick, flag|BCT_ENEMY|n,
						skill_castend_damage_id);
					if(skilllv > 6 && n==3 && c==4){
						skill_brandishspear_dir(&tc,dir,-1);
						n--;c=-1;
					}
				}
			}
			/* ”ÍˆÍ‡@ */
			for(c=0;c<10;c++){
				if(c==0||c==5) skill_brandishspear_dir(&tc,dir,-1);
				map_foreachinarea(skill_area_sub,
					bl->m,tc.val1[c%5],tc.val2[c%5],tc.val1[c%5],tc.val2[c%5],0,
					src,skillid,skilllv,tick, flag|BCT_ENEMY|1,
					skill_castend_damage_id);
			}
		}
		break;

	/* ƒp[ƒeƒBƒXƒLƒ‹ */
	case AL_ANGELUS:		/* ƒGƒ“ƒWƒFƒ‰ƒX */
	case PR_MAGNIFICAT:		/* ƒ}ƒOƒjƒtƒBƒJ[ƒg */
	case PR_GLORIA:			/* ƒOƒƒŠƒA */
	case SN_WINDWALK:		/* ƒEƒCƒ“ƒhƒEƒH[ƒN */
		if(sd == NULL || sd->status.party_id==0 || (flag&1) ){
			/* ŒÂ•Ê‚Ìˆ— */
			clif_skill_nodamage(bl,bl,skillid,skilllv,1);
			if( bl->type==BL_PC && ((struct map_session_data *)bl)->special_state.no_magic_damage )
				break;
			status_change_start(bl,SkillStatusChangeTable[skillid],skilllv,0,0,0,skill_get_time(skillid,skilllv),0);
		}
		else{
			/* ƒp[ƒeƒB‘S‘Ì‚Ö‚Ìˆ— */
			party_foreachsamemap(skill_area_sub,
				sd,1,
				src,skillid,skilllv,tick, flag|BCT_PARTY|1,
				skill_castend_nodamage_id);
		}
		break;
	case BS_ADRENALINE:		/* ƒAƒhƒŒƒiƒŠƒ“ƒ‰ƒbƒVƒ… */
	case BS_WEAPONPERFECT:	/* ƒEƒFƒ|ƒ“ƒp[ƒtƒFƒNƒVƒ‡ƒ“ */
	case BS_OVERTHRUST:		/* ƒI[ƒo[ƒgƒ‰ƒXƒg */
		if(sd == NULL || sd->status.party_id==0 || (flag&1) ){
			/* ŒÂ•Ê‚Ìˆ— */
			clif_skill_nodamage(bl,bl,skillid,skilllv,1);
			status_change_start(bl,SkillStatusChangeTable[skillid],skilllv,(src == bl)? 1:0,0,0,skill_get_time(skillid,skilllv),0);
		}
		else{
			/* ƒp[ƒeƒB‘S‘Ì‚Ö‚Ìˆ— */
			party_foreachsamemap(skill_area_sub,
				sd,1,
				src,skillid,skilllv,tick, flag|BCT_PARTY|1,
				skill_castend_nodamage_id);
		}
		break;

	/*i•t‰Á‚Æ‰ğœ‚ª•K—vj */
	case BS_MAXIMIZE:		/* ƒ}ƒLƒVƒ}ƒCƒYƒpƒ[ */
	case NV_TRICKDEAD:		/* €‚ñ‚¾‚Ó‚è */
	case CR_DEFENDER:		/* ƒfƒBƒtƒFƒ“ƒ_[ */
	case CR_AUTOGUARD:		/* ƒI[ƒgƒK[ƒh */
		{
			struct status_change *tsc_data = status_get_sc_data(bl);
			int sc=SkillStatusChangeTable[skillid];
			clif_skill_nodamage(src,bl,skillid,skilllv,1);
			if( tsc_data ){
				if( tsc_data[sc].timer == -1 )
					/* •t‰Á‚·‚é */
					status_change_start(bl,sc,skilllv,0,0,0,skill_get_time(skillid,skilllv),0);
				else
					/* ‰ğœ‚·‚é */
					status_change_end(bl, sc, -1);
			}
		}
		break;

	case TF_HIDING:			/* ƒnƒCƒfƒBƒ“ƒO */
	  {
		struct status_change *tsc_data = status_get_sc_data(bl);
		int sc = SkillStatusChangeTable[skillid];
		clif_skill_nodamage(src,bl,skillid,-1,1);
		if (tsc_data && tsc_data[sc].timer != -1) {
			/* ‰ğœ‚·‚é */
			status_change_end(bl, sc, -1);
		} else {
			/* •t‰Á‚·‚é */
			status_change_start(bl, sc, skilllv, 0, 0, 0, skill_get_time(skillid, skilllv), 0);
		}
	  }
		break;

	case AS_CLOAKING:		/* ƒNƒ[ƒLƒ“ƒO */
		if (skilllv >= 3 || !skill_check_cloaking(bl)) // <--- correction of cloacking and walls found on freya's bug report (thanks to [BeoWulf])
		{
			struct status_change *tsc_data = status_get_sc_data(bl);
			int sc = SkillStatusChangeTable[skillid];
			if (battle_config.no_caption_cloaking)
				clif_skill_nodamage(src, bl, skillid, -1, 1);
			else
				clif_skill_nodamage(src, bl, skillid, skilllv, 1);
			if (tsc_data && tsc_data[sc].timer != -1) {
				/* ‰ğœ‚·‚é */
				status_change_end(bl, sc, -1);
			} else {
				/* •t‰Á‚·‚é */
				status_change_start(bl, sc, skilllv, 0, 0, 0, skill_get_time(skillid, skilllv), 0);
			}
		}
		break;

	case ST_CHASEWALK:			/* ƒnƒCƒfƒBƒ“ƒO */
	  {
		struct status_change *tsc_data = status_get_sc_data(bl);
		int sc=SkillStatusChangeTable[skillid];
		clif_skill_nodamage(src,bl,skillid,-1,1);
		if (tsc_data && tsc_data[sc].timer != -1) {
			/* ‰ğœ‚·‚é */
			status_change_end(bl, sc, -1);
		} else {
			/* •t‰Á‚·‚é */
			status_change_start(bl, sc, skilllv, 0, 0, 0, skill_get_time(skillid,skilllv), 0);
		}
	  }
		break;

	/* ‘Î’nƒXƒLƒ‹ */
	case BD_LULLABY:			/* qç‰S */
	case BD_RICHMANKIM:			/* ƒjƒˆƒ‹ƒh‚Ì‰ƒ */
	case BD_ETERNALCHAOS:		/* ‰i‰“‚Ì¬“× */
	case BD_DRUMBATTLEFIELD:	/* í‘¾ŒÛ‚Ì‹¿‚« */
	case BD_RINGNIBELUNGEN:		/* ƒj[ƒxƒ‹ƒ“ƒO‚Ìw—Ö */
	case BD_ROKISWEIL:			/* ƒƒL‚Ì‹©‚Ñ */
	case BD_INTOABYSS:			/* [•£‚Ì’†‚É */
	case BD_SIEGFRIED:			/* •s€g‚ÌƒW[ƒNƒtƒŠ[ƒh */
	case BA_DISSONANCE:			/* •s‹¦˜a‰¹ */
	case BA_POEMBRAGI:			/* ƒuƒ‰ƒM‚Ì */
	case BA_WHISTLE:			/* Œû“J */
	case BA_ASSASSINCROSS:		/* —[—z‚ÌƒAƒTƒVƒ“ƒNƒƒX */
	case BA_APPLEIDUN:			/* ƒCƒhƒDƒ“‚Ì—ÑŒç */
	case DC_UGLYDANCE:			/* ©•ªŸè‚Èƒ_ƒ“ƒX */
	case DC_HUMMING:			/* ƒnƒ~ƒ“ƒO */
	case DC_DONTFORGETME:		/* „‚ğ–Y‚ê‚È‚¢‚Åc */
	case DC_FORTUNEKISS:		/* K‰^‚ÌƒLƒX */
	case DC_SERVICEFORYOU:		/* ƒT[ƒrƒXƒtƒH[ƒ†[ */
//	case CG_MOONLIT:			/* Œ–¾‚è‚Ìò‚É—‚¿‚é‰Ô‚Ñ‚ç */
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		skill_unitsetting(src,skillid,skilllv,src->x,src->y,0);
		break;

	case HP_BASILICA:			/* ƒoƒWƒŠƒJ */
	  {
		struct skill_unit_group *sg;
		battle_stopwalking(src, 1);
		skill_clear_unitgroup(src);
		clif_skill_nodamage(src, bl, skillid, skilllv, 1);
		sg = skill_unitsetting(src, skillid, skilllv, src->x, src->y, 0);
		status_change_start(src, SkillStatusChangeTable[skillid], skilllv, 0, 0, (int)sg, skill_get_time(skillid, skilllv), 0);
	  }
		break;

	case PA_GOSPEL:				/* ƒSƒXƒyƒ‹ */
		skill_clear_unitgroup(src);
		clif_skill_nodamage(src, bl, skillid, skilllv, 1);
		skill_unitsetting(src, skillid, skilllv, src->x, src->y, 0);
		status_change_start(src, SkillStatusChangeTable[skillid], skilllv, 0, 0, BCT_SELF, skill_get_time(skillid, skilllv), 0);
		break;

	case BD_ADAPTATION:			/* ƒAƒhƒŠƒu */
	  {
		struct status_change *sc_data = status_get_sc_data(src);
		if(sc_data && sc_data[SC_DANCING].timer!=-1){
			clif_skill_nodamage(src,bl,skillid,skilllv,1);
			skill_stop_dancing(src,0);
		}
	  }
		break;

	case BA_FROSTJOKE:			/* Š¦‚¢ƒWƒ‡[ƒN */
	case DC_SCREAM:				/* ƒXƒNƒŠ[ƒ€ */
		clif_skill_nodamage(src, bl, skillid, skilllv, 1);
		skill_addtimerskill(src, tick + 3000, bl->id, 0, 0, skillid, skilllv, 0, flag);
		break;

	case TF_STEAL:			// ƒXƒeƒB[ƒ‹
		if (sd) {
			if (pc_steal_item(sd, bl))
				clif_skill_nodamage(src, bl, skillid, skilllv, 1);
			else
				clif_skill_fail(sd, skillid, 0x0a, 0);
		}
		break;

	case RG_STEALCOIN:		// ƒXƒeƒB[ƒ‹ƒRƒCƒ“
		if (sd) {
			if (pc_steal_coin(sd,bl)) {
				int range = skill_get_range(skillid,skilllv);
				if (range < 0)
					range = status_get_range(src) - (range + 1);
				clif_skill_nodamage(src,bl,skillid,skilllv,1);
				mob_target((struct mob_data *)bl,src,range);
			} else
				clif_skill_fail(sd, skillid, 0, 0);
		}
		break;

	case MG_STONECURSE: /* ƒXƒg[ƒ“ƒJ[ƒX */
	  {
		// Level 6-10 doesn't consume a red gem if it fails [celest]
		int i, gem_flag = 1;
		if (bl->type == BL_MOB && status_get_mode(bl) & 0x20) {
			if(sd) clif_skill_fail(sd, sd->skillid, 0, 0);
			break;
		}

		if (bl->type == BL_PC && ((struct map_session_data *)bl)->special_state.no_magic_damage)
			break;
	  {
		// To avoid Stone Curse stacking [Aalye]
		struct status_change *sc_data = status_get_sc_data(bl);
		if (sc_data && sc_data[SC_STONE].timer != -1) {
			status_change_end(bl,SC_STONE,-1);
			if (sd)
				clif_skill_fail(sd, skillid, 0, 0);
		}
		else if (rand() % 100 < skilllv * 4 + 20 && !battle_check_undead(status_get_race(bl),status_get_elem_type(bl))){
			clif_skill_nodamage(src, bl, skillid, skilllv, 1);
			status_change_start(bl, SC_STONE, skilllv, 0, 0, 0, skill_get_time2(skillid, skilllv), 0);
		} else if (sd) {
			if (skilllv > 5)
				gem_flag = 0;
			clif_skill_fail(sd, skillid, 0, 0);
		}
	  }
		if (dstmd)
			mob_target(dstmd, src, skill_get_range(skillid, skilllv));
		if (sd && gem_flag) {
			if ((i = pc_search_inventory(sd, skill_db[skillid].itemid[0])) < 0) {
				clif_skill_fail(sd, sd->skillid, 0, 0);
				break;
			}
			pc_delitem(sd, i, skill_db[skillid].amount[0], 0);
		}
	  }
		break;

	case NV_FIRSTAID:			/* ‰‹}è“– */
		clif_skill_nodamage(src,bl,skillid,5,1);
		battle_heal(NULL,bl,5,0,0);
		break;

	case AL_CURE:				/* ƒLƒ…ƒA[ */
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		if( bl->type==BL_PC && ((struct map_session_data *)bl)->special_state.no_magic_damage )
			break;
		status_change_end(bl, SC_SILENCE	, -1 );
		status_change_end(bl, SC_BLIND	, -1 );
		status_change_end(bl, SC_CONFUSION, -1 );
		if( battle_check_undead(status_get_race(bl),status_get_elem_type(bl)) ){//ƒAƒ“ƒfƒbƒh‚È‚çˆÃˆÅŒø‰Ê
			status_change_start(bl, SC_CONFUSION,1,0,0,0,6000,0);
		}
		break;

	case TF_DETOXIFY:			/* ‰ğ“Å */
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		status_change_end(bl, SC_POISON	, -1 );
		status_change_end(bl, SC_DPOISON	, -1 );
		break;

	case PR_STRECOVERY:			/* ƒŠƒJƒoƒŠ[ */
	  {
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		if( bl->type==BL_PC && ((struct map_session_data *)bl)->special_state.no_magic_damage )
			break;
		status_change_end(bl, SC_FREEZE	, -1 );
		status_change_end(bl, SC_STONE	, -1 );
		status_change_end(bl, SC_SLEEP	, -1 );
		status_change_end(bl, SC_STAN		, -1 );
		if( battle_check_undead(status_get_race(bl),status_get_elem_type(bl)) ){//ƒAƒ“ƒfƒbƒh‚È‚çˆÃˆÅŒø‰Ê
			int blind_time;
			//blind_time=30-status_get_vit(bl)/10-status_get_int/15;
			blind_time=30*(100-(status_get_int(bl)+status_get_vit(bl))/2)/100;
			if(rand()%100 < (100-(status_get_int(bl)/2+status_get_vit(bl)/3+status_get_luk(bl)/10)))
				status_change_start(bl, SC_BLIND,1,0,0,0,blind_time,0);
		}
		if(dstmd){
			dstmd->attacked_id=0;
			dstmd->target_id=0;
			dstmd->state.targettype = NONE_ATTACKABLE;
			dstmd->state.skillstate=MSS_IDLE;
			dstmd->next_walktime=tick+rand()%3000+3000;
		}
	  }
		break;

	case WZ_ESTIMATION:			/* ƒ‚ƒ“ƒXƒ^[î•ñ */
		if(src->type==BL_PC){
			clif_skill_nodamage(src,bl,skillid,skilllv,1);
			clif_skill_estimation((struct map_session_data *)src,bl);
		}
		break;

	case MC_IDENTIFY:			/* ƒAƒCƒeƒ€ŠÓ’è */
		if(sd)
			clif_item_identify_list(sd);
		break;

	case BS_REPAIRWEAPON:			/* •ŠíC— */
		if(sd) {
//“®ì‚µ‚È‚¢‚Ì‚Å‚Æ‚è‚ ‚¦‚¸ƒRƒƒ“ƒgƒAƒEƒg
/*			if (pc_search_inventory(sd, 999) < 0) { //fixed by Lupus (item pos can be = 0!)
				clif_skill_fail(sd, sd->skillid, 0, 0);
				map_freeblock_unlock();
				return 1;
			}*/
			clif_item_repair_list(sd);
		}
		break;

	case MC_VENDING:			/* ˜I“XŠJİ */
		if(sd)
			clif_openvendingreq(sd,2+sd->skilllv);
		break;

	case AL_TELEPORT:			/* ƒeƒŒƒ|[ƒg */
		if (sd) {
			if (map[sd->bl.m].flag.noteleport){	/* ƒeƒŒƒ|‹Ö~ */
				clif_skill_teleportmessage(sd,0);
				break;
			}
			clif_skill_nodamage(src, bl, skillid, skilllv, 1);
			if (sd->skilllv == 1)
				clif_skill_warppoint(sd, sd->skillid, "Random", "", "", "");
			else {
				clif_skill_warppoint(sd, sd->skillid, "Random", sd->status.save_point.map, "", "");
			}
		}else if (bl->type == BL_MOB)
			mob_warp((struct mob_data *)bl, -1, -1, -1, 3);
		break;

	case AL_HOLYWATER:			/* ƒAƒNƒAƒxƒlƒfƒBƒNƒ^ */
		if(sd) {
			int eflag;
			struct item item_tmp;
			clif_skill_nodamage(src,bl,skillid,skilllv,1);
			memset(&item_tmp,0,sizeof(item_tmp));
			item_tmp.nameid = 523;
			item_tmp.identify = 1;
			if(battle_config.holywater_name_input) {
				item_tmp.card[0] = 0x00fe;
				item_tmp.card[1] = 0;
				*((unsigned long *)(&item_tmp.card[2]))=sd->char_id;	/* ƒLƒƒƒ‰ID */
			}
			eflag = pc_additem(sd, &item_tmp, 1);
			if (eflag) {
				clif_additem(sd, 0, 0, eflag);
				map_addflooritem(&item_tmp, 1, sd->bl.m, sd->bl.x, sd->bl.y, NULL, NULL, NULL, sd->bl.id, 0);
			}
		}
		break;
	case TF_PICKSTONE:
		if (sd) {
			int eflag;
			struct item item_tmp;
			struct block_list tbl;
			clif_skill_nodamage(src,bl,skillid,skilllv,1);
			memset(&item_tmp,0,sizeof(item_tmp));
			memset(&tbl,0,sizeof(tbl)); // [MouseJstr]
			item_tmp.nameid = 7049;
			item_tmp.identify = 1;
			tbl.id = 0;
			clif_takeitem(&sd->bl, &tbl);
			eflag = pc_additem(sd, &item_tmp, 1);
			if (eflag) {
				clif_additem(sd, 0, 0, eflag);
				map_addflooritem(&item_tmp, 1, sd->bl.m, sd->bl.x, sd->bl.y, NULL, NULL, NULL, sd->bl.id, 0);
			}
		}
		break;

	case RG_STRIPWEAPON:		/* ƒXƒgƒŠƒbƒvƒEƒFƒ|ƒ“ */
	case RG_STRIPSHIELD:		/* ƒXƒgƒŠƒbƒvƒV[ƒ‹ƒh */
	case RG_STRIPARMOR:			/* ƒXƒgƒŠƒbƒvƒA[ƒ}[ */
	case RG_STRIPHELM:			/* ƒXƒgƒŠƒbƒvƒwƒ‹ƒ€ */
	case ST_FULLSTRIP:
	  {
		struct status_change *tsc_data;
		int equip, strip_fix;
		int sclist[4] = {0, 0, 0, 0};

		switch (skillid) {
		case RG_STRIPWEAPON:
			equip = EQP_WEAPON;
			break;
		case RG_STRIPSHIELD:
			equip = EQP_SHIELD;
			break;
		case RG_STRIPARMOR:
			equip = EQP_ARMOR;
			break;
		case RG_STRIPHELM:
			equip = EQP_HELM;
			break;
		case ST_FULLSTRIP:
			equip = EQP_WEAPON | EQP_SHIELD | EQP_ARMOR | EQP_HELM;
			break;
		default:
			map_freeblock_unlock();
			return 1;
		}

		strip_fix = status_get_dex(src) - status_get_dex(bl);
		if (strip_fix < 0)
			strip_fix = 0;
		if (rand() % 100 >= 5 + 2 * skilllv + strip_fix / 5)
			break;
		clif_skill_nodamage(src, bl, skillid, skilllv, 1);

		if (dstsd) {
			tsc_data = status_get_sc_data(bl);
			for (i = 0; i < 11; i++) {
				if (dstsd->equip_index[i] >= 0 && dstsd->inventory_data[dstsd->equip_index[i]]) {
					if (equip & EQP_WEAPON && (i == 9 || (i == 8 && dstsd->inventory_data[dstsd->equip_index[8]]->type == 4)) && !(dstsd->unstripable_equip & EQP_WEAPON) && !(tsc_data && tsc_data[SC_CP_WEAPON].timer != -1)) {
						sclist[0] = SC_STRIPWEAPON;
						pc_unequipitem(dstsd, dstsd->equip_index[i], 3);
					} else if (equip & EQP_SHIELD && i == 8 && dstsd->inventory_data[dstsd->equip_index[8]]->type == 5 && !(dstsd->unstripable_equip & EQP_SHIELD) && !(tsc_data && tsc_data[SC_CP_SHIELD].timer != -1)) {
						sclist[1] = SC_STRIPSHIELD;
						pc_unequipitem(dstsd, dstsd->equip_index[i], 3);
					} else if (equip & EQP_ARMOR && i == 7 && !(dstsd->unstripable_equip & EQP_ARMOR) && !(tsc_data && tsc_data[SC_CP_ARMOR].timer != -1)) {
						sclist[2] = SC_STRIPARMOR;
						pc_unequipitem(dstsd, dstsd->equip_index[i], 3);
					} else if (equip & EQP_HELM && i == 6 && !(dstsd->unstripable_equip & EQP_HELM) && !(tsc_data && tsc_data[SC_CP_HELM].timer != -1)) {
						sclist[3] = SC_STRIPHELM;
						pc_unequipitem(dstsd, dstsd->equip_index[i], 3);
					}
				}
			}
		} else if (dstmd) {
			if (equip & EQP_WEAPON)
				sclist[0] = SC_STRIPWEAPON;
			if (equip & EQP_SHIELD)
				sclist[1] = SC_STRIPSHIELD;
			if (equip & EQP_ARMOR)
				sclist[2] = SC_STRIPARMOR;
			if (equip & EQP_HELM)
				sclist[3] = SC_STRIPHELM;
		}

		for (i = 0; i < 4; i++) {
			if (sclist[i] > 0) // Start the SC only if an equipment was stripped from this location
				status_change_start(bl, sclist[i], skilllv, 0, 0, 0, skill_get_time(skillid, skilllv) + strip_fix / 2, 0);
		}
	  }
		break;

	/* PotionPitcher */
	case AM_POTIONPITCHER: /* ƒ|[ƒVƒ‡ƒ“ƒsƒbƒ`ƒƒ[ */
	  {
		struct block_list tbl;
		int i, x, hp = 0,sp = 0;
		if (sd) {
			/* On kRO/iRO, you can use on oneself. [Aalye]
			if (sd == dstsd) { // cancel use on oneself
				map_freeblock_unlock();
				return 1;
			}*/
			x = skilllv % 11 - 1;
			i = pc_search_inventory(sd,skill_db[skillid].itemid[x]);
			if (i < 0 || skill_db[skillid].itemid[x] <= 0) {
				clif_skill_fail(sd, skillid, 0, 0);
				map_freeblock_unlock();
				return 1;
			}
			if (sd->inventory_data[i] == NULL || sd->status.inventory[i].amount < skill_db[skillid].amount[x]) {
				clif_skill_fail(sd, skillid, 0, 0);
				map_freeblock_unlock();
				return 1;
			}
			sd->state.potionpitcher_flag = 1;
			sd->potion_hp = sd->potion_sp = sd->potion_per_hp = sd->potion_per_sp = 0;
			sd->skilltarget = bl->id;
			run_script(sd->inventory_data[i]->use_script, 0, sd->bl.id, 0);
			pc_delitem(sd, i, skill_db[skillid].amount[x], 0);
			sd->state.potionpitcher_flag = 0;
			if (sd->potion_per_hp > 0 || sd->potion_per_sp > 0) {
				hp = status_get_max_hp(bl) * sd->potion_per_hp / 100;
				hp = hp * (100 + pc_checkskill(sd, AM_POTIONPITCHER) * 10 + pc_checkskill(sd, AM_LEARNINGPOTION) * 5) / 100;
				if(dstsd) {
					sp = dstsd->status.max_sp * sd->potion_per_sp / 100;
					sp = sp * (100 + pc_checkskill(sd, AM_POTIONPITCHER) * 10 + pc_checkskill(sd, AM_LEARNINGPOTION) * 5) / 100;
				}
			}
			else {
				if (sd->potion_hp > 0) {
					hp = sd->potion_hp * (100 + pc_checkskill(sd, AM_POTIONPITCHER) * 10 + pc_checkskill(sd, AM_LEARNINGPOTION) * 5) / 100;
					hp = hp * (100 + (status_get_vit(bl)<<1)) / 100;
					if (dstsd)
						hp = hp * (100 + pc_checkskill(dstsd, SM_RECOVERY) * 10) / 100;
				}
				if (sd->potion_sp > 0) {
					sp = sd->potion_sp * (100 + pc_checkskill(sd, AM_POTIONPITCHER) * 10 + pc_checkskill(sd, AM_LEARNINGPOTION) * 5) / 100;
					sp = sp * (100 + (status_get_int(bl)<<1)) / 100;
					if (dstsd)
						sp = sp * (100 + pc_checkskill(dstsd, MG_SRECOVERY) * 10) / 100;
				}
			}
		}
		else {
			hp = (1 + rand() % 400) * (100 + skilllv * 10) / 100;
			hp = hp * (100 + (status_get_vit(bl)<<1)) / 100;
			if (dstsd)
				hp = hp * (100 + pc_checkskill(dstsd, SM_RECOVERY) * 10) / 100;
		}
		tbl.id = 0;
		tbl.m = src->m;
		tbl.x = src->x;
		tbl.y = src->y;
		clif_skill_nodamage(src, bl, skillid, skilllv, 1);
		if (hp > 0 || (hp <= 0 && sp <= 0))
			clif_skill_nodamage(&tbl, bl, AL_HEAL, hp, 1);
		if (sp > 0)
			clif_skill_nodamage(&tbl, bl, MG_SRECOVERY, sp, 1);
		battle_heal(src, bl, hp, sp, 0);
	  }
		break;
	case AM_CP_WEAPON:
	case AM_CP_SHIELD:
	case AM_CP_ARMOR:
	case AM_CP_HELM:
	  {
		int scid = SC_STRIPWEAPON + (skillid - AM_CP_WEAPON);
		struct status_change *tsc_data = status_get_sc_data(bl);
		clif_skill_nodamage(src, bl, skillid, skilllv, 1);
		if (tsc_data && tsc_data[scid].timer != -1)
			status_change_end(bl, scid, -1 );
		status_change_start(bl, SkillStatusChangeTable[skillid], skilllv, 0, 0, 0, skill_get_time(skillid, skilllv), 0);
	  }
		break;
	case SA_DISPELL:			/* ƒfƒBƒXƒyƒ‹ */
	  {
		int i;
		int ii = (10 * (rand() / (RAND_MAX + 1.0)));
		if(ii < (5 - skilllv))
		      break;
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		if(bl->type == BL_PC && ((struct map_session_data *)bl)->special_state.no_magic_damage)
			break;
		for(i=0;i<136;i++){
			if(i==SC_RIDING || i== SC_FALCON || i==SC_HALLUCINATION || i==SC_WEIGHT50
				|| i==SC_WEIGHT90 || i==SC_STRIPWEAPON || i==SC_STRIPSHIELD || i==SC_STRIPARMOR
				|| i==SC_STRIPHELM || i==SC_CP_WEAPON || i==SC_CP_SHIELD || i==SC_CP_ARMOR
				|| i==SC_CP_HELM || i==SC_COMBO)
					continue;
			status_change_end(bl,i,-1);
		}
	  }
		break;

	case TF_BACKSLIDING:		/* ƒoƒbƒNƒXƒeƒbƒv */
		battle_stopwalking(src,1);
		skill_blown(src,bl,skill_get_blewcount(skillid,skilllv)|0x10000);
		if(src->type == BL_MOB)
			clif_fixmobpos((struct mob_data *)src);
		else if(src->type == BL_PET)
			clif_fixpetpos((struct pet_data *)src);
		else if(src->type == BL_PC)
			clif_fixpos(src);
		skill_addtimerskill(src, tick + 200, src->id, 0, 0, skillid, skilllv, 0, flag);
		break;

	case SA_CASTCANCEL:
		clif_skill_nodamage(src, bl, skillid, skilllv, 1);
		skill_castcancel(src, 1);
		if (sd) {
			int sp = skill_get_sp(sd->skillid_old, sd->skilllv_old);
			sp = sp * (90 - (skilllv-1) * 20) / 100;
			if (sp < 0) sp = 0;
			pc_heal(sd, 0, -sp);
		}
		break;
	case SA_SPELLBREAKER:	// ƒXƒyƒ‹ƒuƒŒƒCƒJ[
	  {
		struct status_change *sc_data = status_get_sc_data(bl);
		int sp;
		if(sc_data && sc_data[SC_MAGICROD].timer != -1) {
			if(dstsd) {
				sp = skill_get_sp(skillid,skilllv);
				sp = sp * sc_data[SC_MAGICROD].val2 / 100;
				if(sp > 0x7fff) sp = 0x7fff;
				else if(sp < 1) sp = 1;
				if(dstsd->status.sp + sp > dstsd->status.max_sp) {
					sp = dstsd->status.max_sp - dstsd->status.sp;
					dstsd->status.sp = dstsd->status.max_sp;
				}
				else
					dstsd->status.sp += sp;
				clif_heal(dstsd->fd,SP_SP,sp);
			}
			clif_skill_nodamage(bl,bl,SA_MAGICROD,sc_data[SC_MAGICROD].val1,1);
			if(sd) {
				sp = sd->status.max_sp/5;
				if(sp < 1) sp = 1;
				pc_heal(sd,0,-sp);
			}
		}
		else {
			int bl_skillid = 0, bl_skilllv = 0;
			if (bl->type == BL_PC) {
				if (dstsd && dstsd->skilltimer != -1) {
					bl_skillid = dstsd->skillid;
					bl_skilllv = dstsd->skilllv;
				}
			}
			else if (bl->type == BL_MOB) {
				if (dstmd && dstmd->skilltimer != -1) {
					bl_skillid = dstmd->skillid;
					bl_skilllv = dstmd->skilllv;
				}
			}
			if (bl_skillid > 0 && skill_db[bl_skillid].skill_type == BF_MAGIC) {
				clif_skill_nodamage(src,bl,skillid,skilllv,1);
				skill_castcancel(bl,0);
				sp = skill_get_sp(bl_skillid,bl_skilllv);
				if(dstsd)
					pc_heal(dstsd,0,-sp);
				if(sd) {
					sp = sp*(25*(skilllv-1))/100;
					if (skilllv > 1 && sp < 1) sp = 1;
					if (sp > 0x7fff) sp = 0x7fff;
					else if (sp < 1) sp = 1;
					if (sd->status.sp + sp > sd->status.max_sp) {
						sp = sd->status.max_sp - sd->status.sp;
						sd->status.sp = sd->status.max_sp;
					}
					else
						sd->status.sp += sp;
					clif_heal(sd->fd, SP_SP, sp);
				}
			} else if (sd)
				clif_skill_fail(sd, skillid, 0, 0);
		}
	  }
		break;
	case SA_MAGICROD:
		if( bl->type==BL_PC && ((struct map_session_data *)bl)->special_state.no_magic_damage )
			break;
		status_change_start(bl,SkillStatusChangeTable[skillid],skilllv,0,0,0,skill_get_time(skillid,skilllv),0 );
		break;
	case SA_AUTOSPELL:			/* ƒI[ƒgƒXƒyƒ‹ */
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		if(sd)
			clif_autospell(sd,skilllv);
		else {
			int maxlv=1,spellid=0;
			static const int spellarray[3] = { MG_COLDBOLT,MG_FIREBOLT,MG_LIGHTNINGBOLT };
			if(skilllv >= 10) {
				spellid = MG_FROSTDIVER;
				maxlv = skilllv - 9;
			}
			else if(skilllv >=8) {
				spellid = MG_FIREBALL;
				maxlv = skilllv - 7;
			}
			else if(skilllv >=5) {
				spellid = MG_SOULSTRIKE;
				maxlv = skilllv - 4;
			}
			else if(skilllv >=2) {
				int i = rand()%3;
				spellid = spellarray[i];
				maxlv = skilllv - 1;
			}
			else if(skilllv > 0) {
				spellid = MG_NAPALMBEAT;
				maxlv = 3;
			}
			if(spellid > 0)
				status_change_start(src,SC_AUTOSPELL,skilllv,spellid,maxlv,0,
					skill_get_time(SA_AUTOSPELL,skilllv),0);
		}
		break;

	/* ƒ‰ƒ“ƒ_ƒ€‘®«•Ï‰»A…‘®«•Ï‰»A’nA‰ÎA•— */
	case NPC_ATTRICHANGE:
	case NPC_CHANGEWATER:
	case NPC_CHANGEGROUND:
	case NPC_CHANGEFIRE:
	case NPC_CHANGEWIND:
	/* “ÅA¹A”OAˆÅ */
	case NPC_CHANGEPOISON:
	case NPC_CHANGEHOLY:
	case NPC_CHANGEDARKNESS:
	case NPC_CHANGETELEKINESIS:
		if(md){
			clif_skill_nodamage(src,bl,skillid,skilllv,1);
			md->def_ele=skill_get_pl(skillid);
			if(md->def_ele==0)			/* ƒ‰ƒ“ƒ_ƒ€•Ï‰»A‚½‚¾‚µA*/
				md->def_ele=rand()%10;	/* •s€‘®«‚Íœ‚­ */
			md->def_ele+=(1+rand()%4)*20;	/* ‘®«ƒŒƒxƒ‹‚Íƒ‰ƒ“ƒ_ƒ€ */
		}
		break;

	case NPC_PROVOCATION:
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		if(md)
			clif_pet_performance(src,mob_db[md->class].skill[md->skillidx].val[0]);
		break;

	case NPC_HALLUCINATION:
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		if( bl->type==BL_PC && ((struct map_session_data *)bl)->special_state.no_magic_damage )
			break;
		status_change_start(bl,SkillStatusChangeTable[skillid],skilllv,0,0,0,skill_get_time(skillid,skilllv),0 );
		break;

	case NPC_KEEPING:
	case NPC_BARRIER:
	  {
		int skill_time = skill_get_time(skillid,skilllv);
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		status_change_start(bl,SkillStatusChangeTable[skillid],skilllv,0,0,0,skill_time,0 );
		if (src->type == BL_MOB)
			mob_changestate((struct mob_data *)src, MS_DELAY, skill_time);
		else if (src->type == BL_PC)
			sd->attackabletime = sd->canmove_tick = tick + skill_time;
	  }
		break;

	case NPC_DARKBLESSING:
	  {
		int sc_def = 100 - status_get_mdef(bl);
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		if( bl->type==BL_PC && ((struct map_session_data *)bl)->special_state.no_magic_damage )
			break;
		if(status_get_elem_type(bl) == 7 || status_get_race(bl) == 6)
			break;
		if(rand()%100 < sc_def*(50+skilllv*5)/100) {
			if(dstsd) {
				int hp = status_get_hp(bl)-1;
				pc_heal(dstsd,-hp,0);
			}
			else if(dstmd)
				dstmd->hp = 1;
		}
	  }
		break;

	case NPC_SELFDESTRUCTION:	/* ©”š */
	case NPC_SELFDESTRUCTION2:	/* ©”š2 */
		status_change_start(bl,SkillStatusChangeTable[skillid],skilllv,skillid,0,0,skill_get_time(skillid,skilllv),0);
		break;
	case NPC_LICK:
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		if( bl->type==BL_PC && ((struct map_session_data *)bl)->special_state.no_weapon_damage )
			break;
		if(dstsd)
			pc_heal(dstsd,0,-100);
		if(rand()%100 < (skilllv*5)*sc_def_vit/100)
			status_change_start(bl,SC_STAN,skilllv,0,0,0,skill_get_time2(skillid,skilllv),0);
		break;

	case NPC_SUICIDE:			/* ©Œˆ */
		if(src && bl){
			clif_skill_nodamage(src,bl,skillid,skilllv,1);
			if (md)
				mob_damage(NULL,md,md->hp,0);
			else if (sd)
				pc_damage(NULL,sd,sd->status.hp);
		}
		break;

	case NPC_SUMMONSLAVE:		/* è‰º¢Š« */
	case NPC_SUMMONMONSTER:		/* MOB¢Š« */
		if (md) {
			mob_summonslave(md, mob_db[md->class].skill[md->skillidx].val, skilllv, (skillid == NPC_SUMMONSLAVE) ? 1 : 0);
		}
		break;

	case NPC_TRANSFORMATION:
	case NPC_METAMORPHOSIS:
		if(md)
			mob_class_change(md,mob_db[md->class].skill[md->skillidx].val);
		break;

	case NPC_EMOTION:			/* ƒGƒ‚[ƒVƒ‡ƒ“ */
		if(md)
			clif_emotion(&md->bl,mob_db[md->class].skill[md->skillidx].val[0]);
		break;

	case NPC_DEFENDER:
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		break;

	// Equipment breaking monster skills [Celest]
	case NPC_BREAKWEAPON:
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		if(bl->type == BL_PC && rand()%100 < skilllv && battle_config.equipment_breaking)
			pc_breakweapon((struct map_session_data *)bl);
		break;

	case NPC_BREAKARMOR:
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		if(bl->type == BL_PC && rand()%100 < skilllv && battle_config.equipment_breaking)
			pc_breakarmor((struct map_session_data *)bl);
		break;

	case NPC_BREAKHELM:
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		if(bl->type == BL_PC && rand()%100 < skilllv && battle_config.equipment_breaking)
			// since we don't have any code for helm breaking yet...
			pc_breakweapon((struct map_session_data *)bl);
		break;

	case NPC_BREAKSHIELD:
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		if(bl->type == BL_PC && rand()%100 < skilllv && battle_config.equipment_breaking)
			// since we don't have any code for helm breaking yet...
			pc_breakweapon((struct map_session_data *)bl);
		break;

	case WE_MALE:				/* ŒN‚¾‚¯‚ÍŒì‚é‚æ */
		if(sd && dstsd){
			int hp_rate=(skilllv <= 0)? 0:skill_db[skillid].hp_rate[skilllv-1];
			int gain_hp=sd->status.max_hp*abs(hp_rate)/100;// 15%
			clif_skill_nodamage(src,bl,skillid,gain_hp,1);
			battle_heal(NULL,bl,gain_hp,0,0);
		}
		break;
	case WE_FEMALE:				/* ‚ ‚È‚½‚Ìˆ×‚É‹]µ‚É‚È‚è‚Ü‚· */
		if(sd && dstsd){
			int sp_rate=(skilllv <= 0)? 0:skill_db[skillid].sp_rate[skilllv-1];
			int gain_sp=sd->status.max_sp*abs(sp_rate)/100;// 15%
			clif_skill_nodamage(src,bl,skillid,gain_sp,1);
			battle_heal(NULL,bl,0,gain_sp,0);
		}
		break;

	case WE_CALLPARTNER:			/* ‚ ‚È‚½‚É‰ï‚¢‚½‚¢ */
		if (sd && dstsd) {
			if ((dstsd = pc_get_partner(sd)) == NULL) {
				clif_skill_fail(sd, skillid, 0, 0);
				map_freeblock_unlock();
				return 0;
			}
			if(map[sd->bl.m].flag.nomemo || map[sd->bl.m].flag.nowarpto || map[dstsd->bl.m].flag.nowarp){
				clif_skill_teleportmessage(sd,1);
				map_freeblock_unlock();
				return 0;
			}
			skill_unitsetting(src,skillid,skilllv,sd->bl.x,sd->bl.y,0);
		}
		break;

	case PF_HPCONVERSION:			/* ƒ‰ƒCƒt’u‚«Š·‚¦ */
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		if(sd){
			int conv_hp=0,conv_sp=0;
			conv_hp=sd->status.hp/10; //Šî–{‚ÍHP‚Ì10%
			sd->status.hp -= conv_hp; //HP‚ğŒ¸‚ç‚·
			conv_sp=conv_hp*10*skilllv/100;
			conv_sp=(sd->status.sp+conv_sp>sd->status.max_sp)?sd->status.max_sp-sd->status.sp:conv_sp;
			sd->status.sp += conv_sp; //SP‚ğ‘‚â‚·
			pc_heal(sd,-conv_hp,conv_sp);
			clif_heal(sd->fd,SP_SP,conv_sp);
		}
		break;
	case HT_REMOVETRAP:				/* ƒŠƒ€[ƒuƒgƒ‰ƒbƒv */
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
	  {
		struct skill_unit *su=NULL;
		struct item item_tmp;
		int flag;
		if((bl->type==BL_SKILL) &&
		   (su=(struct skill_unit *)bl) &&
		   (su->group->src_id == src->id || map[bl->m].flag.pvp || map[bl->m].flag.gvg) &&
		   (su->group->unit_id >= 0x8f && su->group->unit_id <= 0x99) &&
		   (su->group->unit_id != 0x92)) { //ã©‚ğæ‚è•Ô‚·
			if (sd) {
				if(battle_config.skill_removetrap_type == 1) {
					for(i=0;i<10;i++) {
						if(skill_db[su->group->skill_id].itemid[i] > 0) {
							memset(&item_tmp,0,sizeof(item_tmp));
							item_tmp.nameid = skill_db[su->group->skill_id].itemid[i];
							item_tmp.identify = 1;
							if (item_tmp.nameid && (flag = pc_additem(sd, &item_tmp, skill_db[su->group->skill_id].amount[i]))) {
								clif_additem(sd, 0, 0, flag);
								map_addflooritem(&item_tmp, skill_db[su->group->skill_id].amount[i], sd->bl.m, sd->bl.x, sd->bl.y, NULL, NULL, NULL, sd->bl.id, 0);
							}
						}
					}
				} else {
					memset(&item_tmp,0,sizeof(item_tmp));
					item_tmp.nameid = 1065;
					item_tmp.identify = 1;
					if (item_tmp.nameid && (flag = pc_additem(sd, &item_tmp, 1))){
						clif_additem(sd, 0, 0, flag);
						map_addflooritem(&item_tmp, 1, sd->bl.m, sd->bl.x, sd->bl.y, NULL, NULL, NULL, sd->bl.id, 0);
					}
				}
			}
			if(su->group->unit_id == 0x91 && su->group->val2){
				struct block_list *target=map_id2bl(su->group->val2);
				if(target && (target->type == BL_PC || target->type == BL_MOB))
					status_change_end(target,SC_ANKLE,-1);
			}
			skill_delunit(su);
		}
	  }
		break;
	case HT_SPRINGTRAP:				/* ƒXƒvƒŠƒ“ƒOƒgƒ‰ƒbƒv */
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
	  {
		struct skill_unit *su=NULL;
		if((bl->type==BL_SKILL) && (su=(struct skill_unit *)bl) && (su->group) ){
			switch(su->group->unit_id){
				case 0x91: // ankle snare
					if (su->group->val2 != 0)
						// if it is already trapping something don't spring it,
						// remove trap should be used instead
						break;
					// otherwise fallthrough to below
				case 0x8f:	/* ƒuƒ‰ƒXƒgƒ}ƒCƒ“ */
				case 0x90:	/* ƒXƒLƒbƒhƒgƒ‰ƒbƒv */
				case 0x93:	/* ƒ‰ƒ“ƒhƒ}ƒCƒ“ */
				case 0x94:	/* ƒVƒ‡ƒbƒNƒEƒF[ƒuƒgƒ‰ƒbƒv */
				case 0x95:	/* ƒTƒ“ƒhƒ}ƒ“ */
				case 0x96:	/* ƒtƒ‰ƒbƒVƒƒ[ */
				case 0x97:	/* ƒtƒŠ[ƒWƒ“ƒOƒgƒ‰ƒbƒv */
				case 0x98:	/* ƒNƒŒƒCƒ‚ƒA[ƒgƒ‰ƒbƒv */
				case 0x99:	/* ƒg[ƒL[ƒ{ƒbƒNƒX */
					su->group->unit_id = 0x8c;
					clif_changelook(bl,LOOK_BASE,su->group->unit_id);
					su->group->limit=DIFF_TICK(tick+1500,su->group->tick);
					su->limit=DIFF_TICK(tick+1500,su->group->tick);
			}
		}
	  }
		break;
	case BD_ENCORE: /* ƒAƒ“ƒR[ƒ‹ */
		clif_skill_nodamage(src, bl, skillid, skilllv, 1);
		if (sd)
			skill_use_id(sd, src->id, sd->skillid_dance, sd->skilllv_dance);
		break;
	case AS_SPLASHER: /* ƒxƒiƒ€ƒXƒvƒ‰ƒbƒVƒƒ[ */
		if ((double)status_get_max_hp(bl) * 2 / 3 < status_get_hp(bl)) { //HP‚ª2/3ˆÈãc‚Á‚Ä‚¢‚½‚ç¸”s
			map_freeblock_unlock();
			return 1;
		}
		clif_skill_nodamage(src, bl, skillid, skilllv, 1);
		status_change_start(bl, SkillStatusChangeTable[skillid], skilllv, skillid, src->id, skill_get_time(skillid, skilllv), 1000, 0);
		break;
	case PF_MINDBREAKER: /* ƒvƒƒ{ƒbƒN */
	  {
		struct status_change *sc_data = status_get_sc_data(bl);

		/* MVPmob‚Æ•s€‚É‚ÍŒø‚©‚È‚¢ */
		if((bl->type==BL_MOB && status_get_mode(bl) & 0x20) || battle_check_undead(status_get_race(bl), status_get_elem_type(bl))) //•s€‚É‚ÍŒø‚©‚È‚¢
		{
			map_freeblock_unlock();
			return 1;
		}

		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		status_change_start(bl,SkillStatusChangeTable[skillid],skilllv,0,0,0,skill_get_time(skillid,skilllv),0 );

		if (dstmd && dstmd->skilltimer != -1 && dstmd->state.skillcastcancel) // ‰r¥–WŠQ
			skill_castcancel(bl,0);
		if (dstsd && dstsd->skilltimer != -1 && (!dstsd->special_state.no_castcancel || map[bl->m].flag.gvg)
			&& dstsd->state.skillcastcancel && !dstsd->special_state.no_castcancel2)
			skill_castcancel(bl,0);

		if(sc_data){
			if(sc_data[SC_FREEZE].timer!=-1)
				status_change_end(bl,SC_FREEZE,-1);
			if(sc_data[SC_STONE].timer!=-1 && sc_data[SC_STONE].val2==0)
				status_change_end(bl,SC_STONE,-1);
			if(sc_data[SC_SLEEP].timer!=-1)
				status_change_end(bl,SC_SLEEP,-1);
		}

		if(bl->type==BL_MOB) {
			int range = skill_get_range(skillid,skilllv);
			if(range < 0)
				range = status_get_range(src) - (range + 1);
			mob_target((struct mob_data *)bl,src,range);
		}
	  }
		break;

	case PF_SOULCHANGE:
	  {
		int sp1 = 0, sp2 = 0;
		if (sd) {
			if (dstsd) {
				sp1 = sd->status.sp > dstsd->status.max_sp ? dstsd->status.max_sp : sd->status.sp;
				sp2 = dstsd->status.sp > sd->status.max_sp ? sd->status.max_sp : dstsd->status.sp;
				sd->status.sp = sp2;
				dstsd->status.sp = sp1;
				clif_heal(sd->fd, SP_SP, sp2);
				clif_updatestatus(sd, SP_SP);
				clif_heal(dstsd->fd, SP_SP, sp1);
				clif_updatestatus(dstsd, SP_SP);
			} else if (dstmd) {
				if (dstmd->state.soul_change_flag) {
					clif_skill_fail(sd, skillid, 0, 0);
					map_freeblock_unlock();
					return 0;
				}
				sp2 = sd->status.max_sp * 3 /100;
				if (sd->status.sp + sp2 > sd->status.max_sp)
					sp2 = sd->status.max_sp - sd->status.sp;
				sd->status.sp += sp2;
				clif_heal(sd->fd, SP_SP, sp2);
				clif_updatestatus(sd, SP_SP);
				dstmd->state.soul_change_flag = 1;
			}
		}
		clif_skill_nodamage(src, bl, skillid, skilllv, 1);
	  }
		break;

	// Weapon Refining [Celest]
	case WS_WEAPONREFINE:
		if(sd)
			clif_item_refine_list(sd);
		break;

	// Slim Pitcher
	case CR_SLIMPITCHER:
	  {
		if (sd && flag & 1) {
			struct block_list tbl;
			int hp = sd->potion_hp * (100 + pc_checkskill(sd, CR_SLIMPITCHER) * 10 + pc_checkskill(sd, AM_POTIONPITCHER) * 10 + pc_checkskill(sd, AM_LEARNINGPOTION) * 5) / 100;
			hp = hp * (100 + (status_get_vit(bl) << 1)) / 100;
			if (dstsd) {
				hp = hp * (100 + pc_checkskill(dstsd, SM_RECOVERY) * 10) / 100;
			}
			tbl.id = 0;
			tbl.m = src->m;
			tbl.x = src->x;
			tbl.y = src->y;
			clif_skill_nodamage(&tbl, bl, AL_HEAL, hp, 1);
			battle_heal(NULL, bl, hp, 0, 0);
		}
	  }
		break;

	// Full Chemical Protection
	case CR_FULLPROTECTION:
	  {
		int i, skilltime;
		struct status_change *tsc_data = status_get_sc_data(bl);
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		skilltime = skill_get_time(skillid,skilllv);
		for (i=0; i<4; i++) {
			if(tsc_data && tsc_data[SC_STRIPWEAPON + i].timer != -1)
				status_change_end(bl, SC_STRIPWEAPON + i, -1 );
			status_change_start(bl,SC_CP_WEAPON + i,skilllv,0,0,0,skilltime,0 );
		}
	  }
		break;

	case RG_CLEANER:
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
	  {
		struct skill_unit *su;
		if((bl->type==BL_SKILL) &&
		   (su=(struct skill_unit *)bl) &&
		   (su->group->src_id == src->id || map[bl->m].flag.pvp || map[bl->m].flag.gvg) &&
		   (su->group->unit_id == 0xb0)){ //ã©‚ğæ‚è•Ô‚·
			if(sd)
			skill_delunit(su);
		}
	  }
		break;

	case ST_PRESERVE:
		if (sd) {
			if (sd->sc_count && sd->sc_data[SC_PRESERVE].timer != -1)
				status_change_end(src, SC_PRESERVE, -1);
			else
				status_change_start(src, SC_PRESERVE, skilllv, 0, 0, 0, skill_get_time(skillid, skilllv), 0);
			clif_skill_nodamage(src, src, skillid, skilllv, 1);
		}
		break;

	// New guild skills [Celest]
	case GD_BATTLEORDER:
		// Only usable during WoE
		if (!agit_flag) { // 0: WoE not starting, Woe is running
			clif_skill_fail(sd, skillid, 0, 0);
			map_freeblock_unlock();
			return 0;
		}
	  {
		struct guild *g = NULL;
		if (flag & 1) {
			if (dstsd && dstsd->status.guild_id == sd->status.guild_id) {
				status_change_start(&dstsd->bl, SC_BATTLEORDERS, skilllv, 0, 0, 0, 0, 0);
			}
		} else if (sd && sd->status.guild_id > 0 && (g = guild_search(sd->status.guild_id)) &&
		           strcmp(sd->status.name, g->master) == 0) {
			clif_skill_nodamage(src, bl, skillid, skilllv, 1);
			map_foreachinarea(skill_area_sub,
			                  src->m, src->x-15, src->y-15, src->x+15, src->y+15, 0,
			                  src, skillid, skilllv, tick, flag|BCT_ALL|1,
			                  skill_castend_nodamage_id);
			pc_blockskill_start(sd, skillid, 300000);
		}
	  }
		break;
	case GD_REGENERATION:
		// Only usable during WoE
		if (!agit_flag) { // 0: WoE not starting, Woe is running
			clif_skill_fail(sd, skillid, 0, 0);
			map_freeblock_unlock();
			return 0;
		}
	  {
		struct guild *g = NULL;
		if (flag & 1) {
			if (dstsd && dstsd->status.guild_id == sd->status.guild_id) {
				status_change_start(&dstsd->bl, SC_REGENERATION, skilllv, 0, 0, 0, 0, 0);
			}
		} else if (sd && sd->status.guild_id > 0 && (g = guild_search(sd->status.guild_id)) &&
		           strcmp(sd->status.name, g->master) == 0) {
			clif_skill_nodamage(src, bl, skillid, skilllv, 1);
			map_foreachinarea(skill_area_sub,
			                  src->m, src->x-15, src->y-15, src->x+15, src->y+15, 0,
			                  src, skillid, skilllv, tick, flag|BCT_ALL|1,
			                  skill_castend_nodamage_id);
			pc_blockskill_start(sd, skillid, 300000);
		}
	  }
		break;
	case GD_RESTORE:
		// Only usable during WoE
		if (!agit_flag) { // 0: WoE not starting, Woe is running
			clif_skill_fail(sd, skillid, 0, 0);
			map_freeblock_unlock();
			return 0;
		}
	  {
		struct guild *g = NULL;
		if (flag & 1) {
			if (dstsd && dstsd->status.guild_id == sd->status.guild_id) {
				int hp, sp;
				hp = dstsd->status.max_hp * 9 / 10;
				sp = dstsd->status.max_sp * 9 / 10;
				sp = (dstsd->status.sp + sp <= dstsd->status.max_sp) ? sp : dstsd->status.max_sp - dstsd->status.sp;
				clif_skill_nodamage(src, bl, AL_HEAL, hp, 1);
				battle_heal(NULL, bl, hp, sp, 0);
			}
		} else if (sd && sd->status.guild_id > 0 && (g = guild_search(sd->status.guild_id)) &&
		           strcmp(sd->status.name, g->master) == 0) {
			clif_skill_nodamage(src, bl, skillid, skilllv, 1);
			map_foreachinarea(skill_area_sub,
			                  src->m, src->x-15, src->y-15, src->x+15, src->y+15, 0,
			                  src, skillid, skilllv, tick, flag|BCT_ALL|1,
			                  skill_castend_nodamage_id);
			pc_blockskill_start (sd, skillid, 300000);
		}
	  }
		break;
	case GD_EMERGENCYCALL:
		// Only usable during WoE
		if (!agit_flag || // 0: WoE not starting, Woe is running
		    (sd && map[sd->bl.m].flag.nowarpto && // if not allowed to warp to the map
		     guild_mapname2gc(sd->mapname) == NULL)) { // and it's not a castle...
			clif_skill_fail(sd, skillid, 0, 0);
			map_freeblock_unlock();
			return 0;
		}
	  {
		int dx[9] = {-1, 1, 0, 0,-1, 1,-1, 1, 0};
		int dy[9] = { 0, 0, 1,-1, 1,-1,-1, 1, 0};
		int j = 0;
		struct guild *g = NULL;
		// i don't know if it actually summons in a circle, but oh well. ;P
		if (sd && sd->status.guild_id > 0 && (g = guild_search(sd->status.guild_id)) &&
		    strcmp(sd->status.name, g->master) == 0) {
			for(i = 0; i < g->max_member; i++, j++) {
				if (j > 8) j = 0;
				if ((dstsd = g->member[i].sd) != NULL && sd != dstsd) {
					if (map[dstsd->bl.m].flag.nowarp &&
					    guild_mapname2gc(sd->mapname) == NULL)
						continue;
					clif_skill_nodamage(src, bl, skillid, skilllv, 1);
					if (map_getcell(sd->bl.m, sd->bl.x + dx[j], sd->bl.y + dy[j], CELL_CHKNOPASS))
						dx[j] = dy[j] = 0;
					pc_setpos(dstsd, sd->mapname, sd->bl.x + dx[j], sd->bl.y + dy[j], 2);
				}
			}
			pc_blockskill_start(sd, skillid, 300000);
		}
	  }
		break;

	default:
		printf("Unknown skill used:%d\n",skillid);
		map_freeblock_unlock();
		return 1;
	}

	map_freeblock_unlock();

	return 0;
}

/*==========================================
 * ƒXƒLƒ‹g—pi‰r¥Š®—¹AIDw’èj
 *------------------------------------------
 */
int skill_castend_id(int tid, unsigned int tick, int id, int data)
{
	struct map_session_data* sd = map_id2sd(id)/*,*target_sd=NULL*/;
	struct block_list *bl;
	int range,inf2;

	nullpo_retr(0, sd);

	if (sd->bl.prev == NULL) //prev‚ª–³‚¢‚Ì‚Í‚ ‚è‚È‚ÌH
		return 0;

	if (sd->skillid != SA_CASTCANCEL) {
		if (sd->skilltimer != tid) /* ƒ^ƒCƒ}ID‚ÌŠm”F */
			return 0;
		if (sd->skilltimer != -1 && pc_checkskill(sd, SA_FREECAST) > 0) {
			sd->speed = sd->prev_speed;
			clif_updatestatus(sd, SP_SPEED);
		}
		sd->skilltimer = -1;
	}

	if ((bl = map_id2bl(sd->skilltarget)) == NULL || bl->prev == NULL) {
		sd->canact_tick = tick;
		sd->canmove_tick = tick;
		sd->skillitem = sd->skillitemlv = -1;
		return 0;
	}
	if (sd->bl.m != bl->m || pc_isdead(sd)) { //ƒ}ƒbƒv‚ªˆá‚¤‚©©•ª‚ª€‚ñ‚Å‚¢‚é
		sd->canact_tick = tick;
		sd->canmove_tick = tick;
		sd->skillitem = sd->skillitemlv = -1;
		return 0;
	}

	if (sd->skillid == PR_LEXAETERNA) {
		struct status_change *sc_data = status_get_sc_data(bl);
		if (sc_data && (sc_data[SC_FREEZE].timer != -1 || (sc_data[SC_STONE].timer != -1 && sc_data[SC_STONE].val2 == 0))) {
			clif_skill_fail(sd, sd->skillid, 0, 0);
			sd->canact_tick = tick;
			sd->canmove_tick = tick;
			return 0;
		}
	}
	else if (sd->skillid == RG_BACKSTAP) {
		int dir = map_calc_dir(&sd->bl,bl->x,bl->y);
		int t_dir = status_get_dir(bl);
		int dist = distance(sd->bl.x,sd->bl.y,bl->x,bl->y);
		if (bl->type != BL_SKILL && (dist == 0 || map_check_dir(dir,t_dir))) {
			clif_skill_fail(sd, sd->skillid, 0, 0);
			sd->canact_tick = tick;
			sd->canmove_tick = tick;
			return 0;
		}
	}

	inf2 = skill_get_inf2(sd->skillid);
	if (((skill_get_inf(sd->skillid) & 1) || inf2 & 4) && // ”Ş‰ä“G‘ÎŠÖŒWƒ`ƒFƒbƒN
	    battle_check_target(&sd->bl, bl, BCT_ENEMY) <= 0) {
		sd->canact_tick = tick;
		sd->canmove_tick = tick;
		sd->skillitem = sd->skillitemlv = -1;
		return 0;
	}
	if (inf2 & 0xC00 && sd->bl.id != bl->id) {
		int fail_flag = 1;
		if (inf2 & 0x400 && battle_check_target(&sd->bl, bl, BCT_PARTY) > 0)
			fail_flag = 0;
		if (inf2 & 0x800 && sd->status.guild_id > 0 && sd->status.guild_id == status_get_guild_id(bl))
			fail_flag = 0;
		if (fail_flag) {
			clif_skill_fail(sd, sd->skillid, 0, 0);
			sd->canact_tick = tick;
			sd->canmove_tick = tick;
			return 0;
		}
	}

	range = skill_get_range(sd->skillid,sd->skilllv);
	if (range < 0)
		range = status_get_range(&sd->bl) - (range + 1);
	range += battle_config.pc_skill_add_range;
	if ((sd->skillid == MO_EXTREMITYFIST && sd->sc_data[SC_COMBO].timer != -1 && sd->sc_data[SC_COMBO].val1 == MO_COMBOFINISH) ||
	    (sd->skillid == CH_TIGERFIST && sd->sc_data[SC_COMBO].timer != -1 && sd->sc_data[SC_COMBO].val1 == MO_COMBOFINISH) ||
	    (sd->skillid == CH_CHAINCRUSH && sd->sc_data[SC_COMBO].timer != -1 && sd->sc_data[SC_COMBO].val1 == MO_COMBOFINISH) ||
	    (sd->skillid == CH_CHAINCRUSH && sd->sc_data[SC_COMBO].timer != -1 && sd->sc_data[SC_COMBO].val1 == CH_TIGERFIST))
		range += skill_get_blewcount(MO_COMBOFINISH,sd->sc_data[SC_COMBO].val2);
	if (battle_config.skill_out_range_consume) { // changed to allow casting when target walks out of range [Valaris]
		if (range < distance(sd->bl.x, sd->bl.y, bl->x, bl->y)) {
			clif_skill_fail(sd, sd->skillid, 0, 0);
			sd->canact_tick = tick;
			sd->canmove_tick = tick;
			return 0;
		}
	}
	if (!skill_check_condition(sd,1)) { /* g—pğŒƒ`ƒFƒbƒN */
		sd->canact_tick = tick;
		sd->canmove_tick = tick;
		sd->skillitem = sd->skillitemlv = -1;
		return 0;
	}
	sd->skillitem = sd->skillitemlv = -1;
	if (battle_config.skill_out_range_consume) {
		if (range < distance(sd->bl.x, sd->bl.y, bl->x, bl->y)) {
			clif_skill_fail(sd, sd->skillid, 0, 0);
			sd->canact_tick = tick;
			sd->canmove_tick = tick;
			return 0;
		}
	}

	if (battle_config.pc_skill_log)
		printf("PC %d skill castend skill=%d\n", sd->bl.id, sd->skillid);
	pc_stop_walking(sd, 0);

	switch(skill_get_nk(sd->skillid))
	{
	/* UŒ‚Œn/‚«”ò‚Î‚µŒn */
	case 0:
	case 2:
		skill_castend_damage_id(&sd->bl, bl, sd->skillid, sd->skilllv, tick, 0);
		break;
	case 1:/* x‰‡Œn */
		if ((sd->skillid == AL_HEAL || (sd->skillid == ALL_RESURRECTION && bl->type != BL_PC) || sd->skillid == PR_ASPERSIO) && battle_check_undead(status_get_race(bl), status_get_elem_type(bl)))
			skill_castend_damage_id(&sd->bl, bl, sd->skillid, sd->skilllv, tick, 0);
		else
			skill_castend_nodamage_id(&sd->bl, bl, sd->skillid, sd->skilllv, tick, 0);
		break;
	}

	return 0;
}

/*==========================================
 * ƒXƒLƒ‹g—pi‰r¥Š®—¹AêŠw’è‚ÌÀÛ‚Ìˆ—j
 *------------------------------------------
 */
int skill_castend_pos2(struct block_list *src, int x, int y, int skillid, int skilllv, unsigned int tick, int flag)
{
	struct map_session_data *sd = NULL;
	int i, tmpx = 0, tmpy = 0, x1 = 0, y_1 = 0;

	if (skillid > 0 && skilllv <= 0) return 0; // celest

	nullpo_retr(0, src);

	if (src->type == BL_PC) {
		nullpo_retr(0, sd = (struct map_session_data *)src);
	}
	if (skillid != WZ_METEOR &&
	    skillid != AM_CANNIBALIZE &&
	    skillid != AM_SPHEREMINE)
		clif_skill_poseffect(src, skillid, skilllv, x, y, tick);

	if (sd && skillnotok(skillid, sd))
		return 0;

	switch(skillid)
	{
	case PR_BENEDICTIO:			/* ¹‘Ì~•Ÿ */
		skill_area_temp[1]=src->id;
		map_foreachinarea(skill_area_sub,
			src->m,x-1,y-1,x+1,y+1,0,
			src,skillid,skilllv,tick, flag|BCT_NOENEMY|1,
			skill_castend_nodamage_id);
		map_foreachinarea(skill_area_sub,
			src->m,x-1,y-1,x+1,y+1,0,
			src,skillid,skilllv,tick, flag|BCT_ENEMY|1,
			skill_castend_damage_id);
		break;

	case BS_HAMMERFALL:			/* ƒnƒ“ƒ}[ƒtƒH[ƒ‹ */
		skill_area_temp[1]=src->id;
		skill_area_temp[2]=x;
		skill_area_temp[3]=y;
		map_foreachinarea(skill_area_sub,
			src->m,x-2,y-2,x+2,y+2,0,
			src,skillid,skilllv,tick, flag|BCT_ENEMY|2,
			skill_castend_nodamage_id);
		break;

	case HT_DETECTING:				/* ƒfƒBƒeƒNƒeƒBƒ“ƒO */
		{
			const int range = 7;
			if (src->x != x)
				x += (src->x - x > 0) ? -range : range;
			if (src->y != y)
				y += (src->y - y > 0) ? -range: range;
			map_foreachinarea(status_change_timer_sub,
				src->m, x - range, y - range, x + range, y + range, 0,
				src, SC_SIGHT, tick);
		}
		break;

	case MG_SAFETYWALL:			/* ƒZƒCƒtƒeƒBƒEƒH[ƒ‹ */
	case MG_FIREWALL:			/* ƒtƒ@ƒCƒ„[ƒEƒH[ƒ‹ */
	case MG_THUNDERSTORM:		/* ƒTƒ“ƒ_[ƒXƒg[ƒ€ */
	case AL_PNEUMA:				/* ƒjƒ…[ƒ} */
	case WZ_ICEWALL:			/* ƒAƒCƒXƒEƒH[ƒ‹ */
	case WZ_FIREPILLAR:			/* ƒtƒ@ƒCƒAƒsƒ‰[ */
	case WZ_QUAGMIRE:			/* ƒNƒ@ƒOƒ}ƒCƒA */
	case WZ_VERMILION:			/* ƒ[ƒhƒIƒuƒ”ƒ@[ƒ~ƒŠƒIƒ“ */
//	case WZ_FROSTNOVA:			/* ƒtƒƒXƒgƒmƒ”ƒ@ */
	case WZ_STORMGUST:			/* ƒXƒg[ƒ€ƒKƒXƒg */
	case WZ_HEAVENDRIVE:		/* ƒwƒ”ƒ“ƒYƒhƒ‰ƒCƒu */
	case PR_SANCTUARY:			/* ƒTƒ“ƒNƒ`ƒ…ƒAƒŠ */
	case PR_MAGNUS:				/* ƒ}ƒOƒkƒXƒGƒNƒ\ƒVƒYƒ€ */
	case CR_GRANDCROSS:			/* ƒOƒ‰ƒ“ƒhƒNƒƒX */
	case NPC_DARKGRANDCROSS:	/*ˆÅƒOƒ‰ƒ“ƒhƒNƒƒX*/
	case HT_SKIDTRAP:			/* ƒXƒLƒbƒhƒgƒ‰ƒbƒv */
	case HT_LANDMINE:			/* ƒ‰ƒ“ƒhƒ}ƒCƒ“ */
	case HT_ANKLESNARE:			/* ƒAƒ“ƒNƒ‹ƒXƒlƒA */
	case HT_SHOCKWAVE:			/* ƒVƒ‡ƒbƒNƒEƒF[ƒuƒgƒ‰ƒbƒv */
	case HT_SANDMAN:			/* ƒTƒ“ƒhƒ}ƒ“ */
	case HT_FLASHER:			/* ƒtƒ‰ƒbƒVƒƒ[ */
	case HT_FREEZINGTRAP:		/* ƒtƒŠ[ƒWƒ“ƒOƒgƒ‰ƒbƒv */
	case HT_BLASTMINE:			/* ƒuƒ‰ƒXƒgƒ}ƒCƒ“ */
	case HT_CLAYMORETRAP:		/* ƒNƒŒƒCƒ‚ƒA[ƒgƒ‰ƒbƒv */
	case AS_VENOMDUST:			/* ƒxƒmƒ€ƒ_ƒXƒg */
	case AM_DEMONSTRATION:			/* ƒfƒ‚ƒ“ƒXƒgƒŒ[ƒVƒ‡ƒ“ */
	case PF_SPIDERWEB:			/* ƒXƒpƒCƒ_[ƒEƒFƒbƒu */
	case PF_FOGWALL:			/* ƒtƒHƒOƒEƒH[ƒ‹ */
	case HT_TALKIEBOX:			/* ƒg[ƒL[ƒ{ƒbƒNƒX */
		skill_unitsetting(src,skillid,skilllv,x,y,0);
			break;

	case RG_GRAFFITI:			/* Graffiti [Valaris] */
		skill_clear_unitgroup(src);
		skill_unitsetting(src,skillid,skilllv,x,y,0);
			break;

	case SA_VOLCANO:		/* ƒ{ƒ‹ƒP[ƒm */
	case SA_DELUGE:			/* ƒfƒŠƒ…[ƒW */
	case SA_VIOLENTGALE:	/* ƒoƒCƒIƒŒƒ“ƒgƒQƒCƒ‹ */
	case SA_LANDPROTECTOR:	/* ƒ‰ƒ“ƒhƒvƒƒeƒNƒ^[ */
		skill_clear_element_field(src);//Šù‚É©•ª‚ª”­“®‚µ‚Ä‚¢‚é‘®«ê‚ğƒNƒŠƒA
		skill_unitsetting(src,skillid,skilllv,x,y,0);
		break;

	case WZ_METEOR:				//ƒƒeƒIƒXƒg[ƒ€
		{
			int flag = 0;
			for(i = 0; i < 2 + (skilllv >> 1); i++) {
				int j = 0;
				do {
					tmpx = x + (rand()%7 - 3);
					tmpy = y + (rand()%7 - 3);
					if(tmpx < 0)
						tmpx = 0;
					else if(tmpx >= map[src->m].xs)
						tmpx = map[src->m].xs - 1;
					if(tmpy < 0)
						tmpy = 0;
					else if(tmpy >= map[src->m].ys)
						tmpy = map[src->m].ys - 1;
					j++;
				} while(map_getcell(src->m, tmpx, tmpy, CELL_CHKNOPASS) && j < 100);
				if(j >= 100)
					continue;
				if (flag == 0) {
					clif_skill_poseffect(src,skillid,skilllv,tmpx,tmpy,tick);
					flag=1;
				}
				if (i > 0)
					skill_addtimerskill(src, tick + i * 1000, 0, tmpx, tmpy, skillid, skilllv, (x1<<16)|y_1, flag);
				x1  = tmpx;
				y_1 = tmpy;
			}
			skill_addtimerskill(src, tick + i * 1000, 0, tmpx, tmpy, skillid, skilllv, -1, flag);
		}
		break;

	case AL_WARP:				/* ƒ[ƒvƒ|[ƒ^ƒ‹ */
		if (sd) {
			if (map[sd->bl.m].flag.noteleport) /* ƒeƒŒƒ|‹Ö~ */
				break;
			clif_skill_warppoint(sd, sd->skillid, sd->status.save_point.map,
			                     (sd->skilllv > 1) ? sd->status.memo_point[0].map : "",
			                     (sd->skilllv > 2) ? sd->status.memo_point[1].map : "",
			                     (sd->skilllv > 3) ? sd->status.memo_point[2].map : ""); // MAX_PORTAL_MEMO
		}
		break;
	case MO_BODYRELOCATION:
		if (sd) {
			pc_movepos(sd, x, y);
			pc_blockskill_start(sd, MO_EXTREMITYFIST, 2000);
		} else if (src->type == BL_MOB)
			mob_warp((struct mob_data *)src, -1, x, y, 0);
		break;
	case AM_CANNIBALIZE: // ƒoƒCƒIƒvƒ‰ƒ“ƒg
		if (sd) {
			int mx, my, id = 0;
			struct mob_data *md;
			int summons[5] = { 1020, 1068, 1118, 1500, 1368 }; // kRO 14/12/04 Patch - Bio Cannibalize: Monsters that are spawned are different based on the skill level [Aalye] from freya' forum

			mx = x; // + (rand() % 10 - 5);
			my = y; // + (rand() % 10 - 5);
			id = mob_once_spawn(sd, "this", mx, my, "--ja--", ((skilllv < 6) ? summons[skilllv-1] : 1368), 1, "");
			if ((md = (struct mob_data *)map_id2bl(id)) !=NULL) {
				md->master_id = sd->bl.id;
				md->hp = 2210 + skilllv * 200;
				md->state.special_mob_ai = 1; // 0: nothing, 1: cannibalize, 2-3: spheremine
				md->deletetimer = add_timer(gettick_cache + skill_get_time(skillid, skilllv), mob_timer_delete, id, 0);
			}
			clif_skill_poseffect(src, skillid, skilllv, x, y, tick);
		}
		break;
	case AM_SPHEREMINE:	// ƒXƒtƒBƒA[ƒ}ƒCƒ“
		if (sd) {
			int mx, my, id = 0;
			struct mob_data *md;

			mx = x; // + (rand()%10 - 5);
			my = y; // + (rand()%10 - 5);
			id = mob_once_spawn(sd, "this", mx, my, "--ja--", 1142, 1, "");
			if ((md = (struct mob_data *)map_id2bl(id)) != NULL) {
				md->master_id = sd->bl.id;
				md->hp = 2000 + skilllv * 400;
				md->state.special_mob_ai = 2; // 0: nothing, 1: cannibalize, 2-3: spheremine
				md->deletetimer = add_timer(gettick_cache + skill_get_time(skillid, skilllv), mob_timer_delete, id, 0);
			}
			clif_skill_poseffect(src, skillid, skilllv, x, y, tick);
		}
		break;

	// Slim Pitcher [Celest]
	case CR_SLIMPITCHER:
	  {
		if (sd) {
			int i = skilllv % 11 - 1;
			int j = pc_search_inventory(sd,skill_db[skillid].itemid[i]);
			if (j < 0 || skill_db[skillid].itemid[i] <= 0 || sd->inventory_data[j] == NULL ||
				sd->status.inventory[j].amount < skill_db[skillid].amount[i]) {
				clif_skill_fail(sd, skillid, 0, 0);
				return 1;
			}
			sd->state.potionpitcher_flag = 1;
			sd->potion_hp = 0;
			run_script(sd->inventory_data[j]->use_script, 0, sd->bl.id, 0);
			pc_delitem(sd, j, skill_db[skillid].amount[i], 0);
			sd->state.potionpitcher_flag = 0;
			clif_skill_poseffect(src, skillid, skilllv, x, y, tick);
			if (sd->potion_hp > 0) {
				map_foreachinarea(skill_area_sub,
				                  src->m, x-3, y-3, x+3, y+3, 0,
				                  src, skillid, skilllv, tick, flag|BCT_PARTY|1,
				                  skill_castend_nodamage_id);
			}
		}
	  }
		break;
	}

	return 0;
}

/*==========================================
 * ƒXƒLƒ‹g—pi‰r¥Š®—¹Amapw’èj
 *------------------------------------------
 */
void skill_castend_map(struct map_session_data *sd, int skill_num, const char *mapname) {
	int x = 0, y = 0;

//	nullpo_retv(sd); // checked before to call function

//	if (sd->bl.prev == NULL || pc_isdead(sd))
	if (pc_isdead(sd))
		return;

	if (skillnotok(skill_num, sd))
		return;

	if (sd->opt1 > 0 || sd->status.option & 2)
		return;
	//ƒXƒLƒ‹‚ªg‚¦‚È‚¢ó‘ÔˆÙí’†
	if (sd->sc_count) {
		if (sd->sc_data[SC_DIVINA].timer!=-1 ||
		    sd->sc_data[SC_ROKISWEIL].timer!=-1 ||
		    sd->sc_data[SC_AUTOCOUNTER].timer != -1 ||
		    sd->sc_data[SC_STEELBODY].timer != -1 ||
		    sd->sc_data[SC_DANCING].timer!=-1 ||
		//    sd->sc_data[SC_BERSERK].timer != -1 || // checked before to call function
		    sd->sc_data[SC_MARIONETTE].timer != -1)
			return;
	}

	if (skill_num != sd->skillid) /* •s³ƒpƒPƒbƒg‚ç‚µ‚¢ */
		return;

	pc_stopattack(sd);

	if (battle_config.pc_skill_log)
		printf("PC %d skill castend skill =%d map=%s\n", sd->bl.id, skill_num, mapname);
	pc_stop_walking(sd, 0);

	if (strcmp(mapname, "cancel") == 0)
		return;

	switch(skill_num) {
	case AL_TELEPORT:		/* ƒeƒŒƒ|[ƒg */
		if (strcmp(mapname, "Random") == 0)
			pc_randomwarp(sd);
		else
			pc_setpos(sd, sd->status.save_point.map, sd->status.save_point.x, sd->status.save_point.y, 3);
		break;

	case AL_WARP:			/* ƒ[ƒvƒ|[ƒ^ƒ‹ */
	  {
		const struct point *p[4];
		struct skill_unit_group *group;
		int i;
		int maxcount = 0;
		p[0] = &sd->status.save_point;
		p[1] = &sd->status.memo_point[0];
		p[2] = &sd->status.memo_point[1];
		p[3] = &sd->status.memo_point[2]; // MAX_PORTAL_MEMO

		if ((maxcount = skill_get_maxcount(sd->skillid)) > 0) {
			int c;
			c = 0;
			for(i = 0; i < MAX_SKILLUNITGROUP; i++) {
				if (sd->skillunit[i].alive_count > 0 && sd->skillunit[i].skill_id == sd->skillid)
					c++;
			}
			if (c >= maxcount) {
				clif_skill_fail(sd, sd->skillid, 0, 0);
				sd->canact_tick = gettick_cache;
				sd->canmove_tick = gettick_cache;
				return;
			}
		}

		if (sd->skilllv <= 0)
			return;
		for(i = 0; i < sd->skilllv; i++) {
			if (strcmp(mapname, p[i]->map) == 0) {
				x = p[i]->x;
				y = p[i]->y;
				break;
			}
		}
		if (x == 0 || y == 0) /* •s³ƒpƒPƒbƒgH */
			return;

		if (!skill_check_condition(sd, 3))
			return;
		if ((group = skill_unitsetting(&sd->bl, sd->skillid, sd->skilllv, sd->skillx, sd->skilly, 0)) == NULL)
			return;
		CALLOC(group->valstr, char, 17); // 16 + NULL
		strncpy(group->valstr, mapname, 16);
		group->val2 = (x << 16) | y;
	  }
		break;
	}

	return;
}

/*==========================================
 * ƒXƒLƒ‹ƒ†ƒjƒbƒgİ’èˆ—
 *------------------------------------------
 */
struct skill_unit_group *skill_unitsetting( struct block_list *src, int skillid,int skilllv,int x,int y,int flag)
{
	struct skill_unit_group *group;
	int i,limit, val1 = 0, val2 = 0, val3 = 0;
	int count = 0;
	int target, interval, range, unit_flag;
	struct skill_unit_layout *layout;
	struct status_change *sc_data;
	int active_flag = 1;

	nullpo_retr(0, src);

	limit = skill_get_time(skillid, skilllv);
	range = skill_get_unit_range(skillid);
	interval = skill_get_unit_interval(skillid);
	target = skill_get_unit_target(skillid);
	unit_flag = skill_get_unit_flag(skillid);
	layout = skill_get_unit_layout(skillid, skilllv, src, x, y);

	if (unit_flag & UF_DEFNOTENEMY && battle_config.defnotenemy)
		target = BCT_NOENEMY;

	sc_data = status_get_sc_data(src); // for firewall and fogwall - celest

	switch(skillid) { /* İ’è */

	case MG_SAFETYWALL:			/* ƒZƒCƒtƒeƒBƒEƒH[ƒ‹ */
		val2 = skilllv + 1;
		break;

	case MG_FIREWALL:			/* ƒtƒ@ƒCƒ„[ƒEƒH[ƒ‹ */
		if (sc_data && sc_data[SC_VIOLENTGALE].timer != -1)
			limit = limit * 3 / 2;
		val2 = 4 + skilllv;
		break;

	case AL_WARP:				/* ƒ[ƒvƒ|[ƒ^ƒ‹ */
		val1 = skilllv + 6;
		if (flag == 0)
			limit = 2000;
		active_flag = 0;
		break;

	case PR_SANCTUARY:			/* ƒTƒ“ƒNƒ`ƒ…ƒAƒŠ */
		val1 = (skilllv + 3) * 2;
		val2 = (skilllv > 6) ? 777 : skilllv * 100;
		interval += 500;
		break;

	case WZ_FIREPILLAR:			/* ƒtƒ@ƒCƒA[ƒsƒ‰[ */
		if (flag != 0)
			limit = 1000;
		val1 = skilllv + 2;
		if (skilllv >= 6)
			range=2;
		break;

	case HT_SANDMAN:			/* ƒTƒ“ƒhƒ}ƒ“ */
	case HT_CLAYMORETRAP:		/* ƒNƒŒƒCƒ‚ƒA[ƒgƒ‰ƒbƒv */
	case HT_SKIDTRAP:			/* ƒXƒLƒbƒhƒgƒ‰ƒbƒv */
	case HT_LANDMINE:			/* ƒ‰ƒ“ƒhƒ}ƒCƒ“ */
	case HT_ANKLESNARE:			/* ƒAƒ“ƒNƒ‹ƒXƒlƒA */
	case HT_FLASHER:			/* ƒtƒ‰ƒbƒVƒƒ[ */
	case HT_FREEZINGTRAP:		/* ƒtƒŠ[ƒWƒ“ƒOƒgƒ‰ƒbƒv */
	case HT_BLASTMINE:			/* ƒuƒ‰ƒXƒgƒ}ƒCƒ“ */
		// longer trap times in WOE [celest]
		if (map[src->m].flag.gvg) limit *= 4;
		break;

	case HT_SHOCKWAVE:			/* ƒVƒ‡ƒbƒNƒEƒF[ƒuƒgƒ‰ƒbƒv */
		val1 = skilllv * 15 + 10;
		break;

	case SA_LANDPROTECTOR:	/* ƒOƒ‰ƒ“ƒhƒNƒƒX */
	  {
		int aoe_diameter;	// -- aoe_diameter (moonsoul) added for sage Area Of Effect skills
		val1 = skilllv * 15 + 10;
		aoe_diameter = skilllv + skilllv % 2 + 5;
		count = aoe_diameter * aoe_diameter; // -- this will not function if changed to ^2 (moonsoul)
	  }
		break;

	case BA_WHISTLE:			/* Œû“J */
		if (src->type == BL_PC)
			val1 = (pc_checkskill((struct map_session_data *)src, BA_MUSICALLESSON) + 1) >> 1;
		val2 = ((status_get_agi(src) / 10) & 0xffff) << 16;
		val2 |= (status_get_luk(src) / 10) & 0xffff;
		break;

	case DC_HUMMING:			/* ƒnƒ~ƒ“ƒO */
		if (src->type == BL_PC)
			val1 = (pc_checkskill((struct map_session_data *)src, DC_DANCINGLESSON) + 1) >> 1;
		val2 = status_get_dex(src) / 10;
		break;

	case DC_DONTFORGETME:		/* „‚ğ–Y‚ê‚È‚¢‚Åc */
		if (src->type == BL_PC)
			val1 = (pc_checkskill((struct map_session_data *)src, DC_DANCINGLESSON) + 1) >> 1;
		val2 = ((status_get_str(src) / 20) & 0xffff) << 16;
		val2 |= (status_get_agi(src) / 10) & 0xffff;
		break;

	case BA_POEMBRAGI:			/* ƒuƒ‰ƒM‚Ì */
		if (src->type == BL_PC)
			val1 = pc_checkskill((struct map_session_data *)src, BA_MUSICALLESSON);
		val2 = ((status_get_dex(src) / 10) & 0xffff) << 16;
		val2 |= (status_get_int(src) / 5) & 0xffff;
		break;

	case BA_APPLEIDUN:			/* ƒCƒhƒDƒ“‚Ì—ÑŒç */
		if (src->type == BL_PC)
			val1 = pc_checkskill((struct map_session_data *)src, BA_MUSICALLESSON) & 0xffff;
		val2 |= (status_get_vit(src)) & 0xffff;
		val3 = 0;//‰ñ•œ—pƒ^ƒCƒ€ƒJƒEƒ“ƒ^(6•b?‚É1?‰Á)
		break;

	case DC_SERVICEFORYOU:		/* ƒT[ƒrƒXƒtƒH[ƒ†[ */
		if (src->type == BL_PC)
			val1 = (pc_checkskill((struct map_session_data *)src, DC_DANCINGLESSON) + 1) >> 1;
		val2 = status_get_int(src) / 10;
		break;

	case BA_ASSASSINCROSS:		/* —[—z‚ÌƒAƒTƒVƒ“ƒNƒƒX */
		if (src->type == BL_PC)
			val1 = (pc_checkskill((struct map_session_data *)src, BA_MUSICALLESSON) + 1) >> 1;
		val2 = status_get_agi(src) / 20;
		break;

	case DC_FORTUNEKISS:		/* K‰^‚ÌƒLƒX */
		if (src->type == BL_PC)
			val1 = (pc_checkskill((struct map_session_data *)src, DC_DANCINGLESSON) + 1) >> 1;
		val2 = status_get_luk(src) / 10;
		break;

	case PF_FOGWALL:	/* ƒtƒHƒOƒEƒH[ƒ‹ */
		if (sc_data && sc_data[SC_DELUGE].timer != -1) limit *= 2;
		break;

	case RG_GRAFFITI: /* Graffiti */
		count = 1; // Leave this at 1 [Valaris]
		break;
	}

	nullpo_retr(NULL, group = skill_initunitgroup(src, (count > 0 ? count : layout->count),
	            skillid, skilllv, skill_get_unit_id(skillid, flag & 1)));
	group->limit = limit;
	group->val1 = val1;
	group->val2 = val2;
	group->val3 = val3;
	group->target_flag = target;
	group->interval = interval;
	if (skillid == HT_TALKIEBOX || skillid == RG_GRAFFITI) {
		CALLOC(group->valstr, char, 81); // 80 + NULL
		strncpy(group->valstr, talkie_mes, 80); // 80 + NULL
	}
	for(i = 0; i < layout->count; i++) {
		struct skill_unit *unit;
		int ux, uy, val1 = skilllv, val2 = 0, limit = group->limit, alive = 1;
		ux = x + layout->dx[i];
		uy = y + layout->dy[i];

		switch (skillid) {
		case MG_FIREWALL:		/* ƒtƒ@ƒCƒ„[ƒEƒH[ƒ‹ */
			val2 = group->val2;
			break;

		case WZ_ICEWALL:		/* ƒAƒCƒXƒEƒH[ƒ‹ */
			if (skilllv <= 1)
				val1 = 500;
			else
				val1 = 200 + 200 * skilllv;
			break;

		case RG_GRAFFITI:	/* Graffiti [Valaris] */
			ux += (i % 5 - 2);
			uy += (i / 5 - 2);
			break;
		}
		//’¼ãƒXƒLƒ‹‚Ìê‡İ’uÀ•Wã‚Éƒ‰ƒ“ƒhƒvƒƒeƒNƒ^[‚ª‚È‚¢‚©ƒ`ƒFƒbƒN
		if(range<=0)
			map_foreachinarea(skill_landprotector,src->m,ux,uy,ux,uy,BL_SKILL,skillid,&alive);

		if (skillid == WZ_ICEWALL && alive) {
			val2 = map_getcell(src->m, ux, uy, CELL_GETTYPE);
			if (val2 == 5 || val2 == 1)
				alive = 0;
			else {
				map_setcell(src->m, ux, uy, 5);
				clif_changemapcell(src->m, ux, uy, 5, 0);
			}
		}

		if (alive) {
			nullpo_retr(NULL, unit=skill_initunit(group, i, ux, uy));
			unit->val1 = val1;
			unit->val2 = val2;
			unit->limit = limit;
			unit->range = range;

			if (range == 0 && active_flag)
				map_foreachinarea(skill_unit_effect, unit->bl.m
				                 , unit->bl.x, unit->bl.y, unit->bl.x, unit->bl.y
				                 , 0, &unit->bl, gettick_cache, 1);
		}
	}

	return group;
}

/*==========================================
 * ƒXƒLƒ‹ƒ†ƒjƒbƒg‚Ì”­“®ƒCƒxƒ“ƒg
 *------------------------------------------
 */
int skill_unit_onplace(struct skill_unit *src, struct block_list *bl, unsigned int tick)
{
	struct skill_unit_group *sg;
	struct block_list *ss;
	struct skill_unit *unit2;
	struct status_change *sc_data;
	int type;

	nullpo_retr(0, src);
	nullpo_retr(0, bl);

	if (bl->prev == NULL || !src->alive || (bl->type == BL_PC && pc_isdead((struct map_session_data *)bl)))
		return 0;

	nullpo_retr(0, sg = src->group);
	nullpo_retr(0, ss = map_id2bl(sg->src_id));

	sc_data = status_get_sc_data(bl);
	type = SkillStatusChangeTable[sg->skill_id];

	if (battle_check_target(&src->bl, bl, sg->target_flag) <= 0)
		return 0;

	// ‘ÎÛ‚ªLPã‚É‹‚éê‡‚Í–³Œø
	if (map_find_skill_unit_oncell(bl, bl->x, bl->y, SA_LANDPROTECTOR, NULL))
		return 0;

	switch (sg->unit_id) {
	case 0x85:	/* ƒjƒ…[ƒ} */
	case 0x7e:	/* ƒZƒCƒtƒeƒBƒEƒH[ƒ‹ */
		if (sc_data && sc_data[type].timer == -1)
			status_change_start(bl, type, sg->skill_lv, (int)src, 0, 0, 0, 0);
		break;

	case 0x80:	/* ƒ[ƒvƒ|[ƒ^ƒ‹(”­“®Œã) */
		if (bl->type == BL_PC) {
			struct map_session_data *sd = (struct map_session_data *)bl;
			if (sd && src->bl.m == bl->m && src->bl.x == bl->x && src->bl.y == bl->y &&
			    src->bl.x == sd->to_x && src->bl.y == sd->to_y) {
				if (battle_config.chat_warpportal || !sd->chatID) {
					pc_setpos(sd, sg->valstr, sg->val2 >> 16, sg->val2 & 0xffff, 3);
					if (sg->src_id == bl->id || (strcmp(map[src->bl.m].name, sg->valstr) == 0 &&
						src->bl.x == (sg->val2 >> 16) && src->bl.y == (sg->val2 & 0xffff)))
						skill_delunitgroup(sg);
					if (--sg->val1 <= 0)
						skill_delunitgroup(sg);
				}
			}
		} else if (bl->type == BL_MOB && battle_config.mob_warpportal) {
			int m = map_mapname2mapid(sg->valstr); // map id on this server (m == -1 if not in actual map-server)
			// what's about map on other map-servers?
			mob_warp((struct mob_data *)bl, m, sg->val2 >> 16, sg->val2 & 0xffff, 3);
		}
		break;

	case 0x8e:	/* ƒNƒ@ƒOƒ}ƒCƒA */
		if (bl->type == BL_PC && ((struct map_session_data *)bl)->special_state.no_magic_damage)
			break;
		if (sc_data && sc_data[type].timer == -1)
			status_change_start(bl, type, sg->skill_lv, (int)src, 0, 0,
			                    skill_get_time2(sg->skill_id, sg->skill_lv), 0);
		break;

	case 0x9a:	/* ƒ{ƒ‹ƒP?ƒm */
	case 0x9b:	/* ƒfƒŠƒ…?ƒW */
	case 0x9c:	/* ƒoƒCƒIƒŒƒ“ƒgƒQƒCƒ‹ */
		if (sc_data && sc_data[type].timer!=-1) {
			unit2 = (struct skill_unit *)sc_data[type].val2; // correction by akrus (val4 -> val2)
			if (unit2 && unit2->group &&
			    (unit2 == src || DIFF_TICK(sg->tick, unit2->group->tick) <= 0))
				break;
		}
		status_change_start(bl, type, sg->skill_lv, (int)src, 0, 0,
		                    skill_get_time2(sg->skill_id, sg->skill_lv), 0);
		break;

	case 0x9e:	/* qç‰S */
	case 0x9f:	/* ƒjƒˆƒ‹ƒh‚Ì‰ƒ */
	case 0xa0:	/* ‰i‰“‚Ì¬“× */
	case 0xa1:	/* ?‘¾ŒÛ‚Ì‹¿‚« */
	case 0xa2:	/* ƒj?ƒxƒ‹ƒ“ƒO‚Ìw—Ö */
	case 0xa3:	/* ƒƒL‚Ì‹©‚Ñ */
	case 0xa4:	/* [•£‚Ì’†‚É */
	case 0xa5:	/* •s€g‚ÌƒW?ƒNƒtƒŠ?ƒh */
	case 0xa6:	/* •s‹¦˜a‰¹ */
	case 0xa7:	/* Œû“J */
	case 0xa8:	/* —[—z‚ÌƒAƒTƒVƒ“ƒNƒƒX */
	case 0xa9:	/* ƒuƒ‰ƒM‚Ì */
	case 0xaa:	/* ƒCƒhƒDƒ“‚Ì—ÑŒç */
	case 0xab:	/* ©•ªŸè‚Èƒ_ƒ“ƒX */
	case 0xac:	/* ƒnƒ~ƒ“ƒO */
	case 0xad:	/* „‚ğ–Y‚ê‚È‚¢‚Åc */
	case 0xae:	/* K‰^‚ÌƒLƒX */
	case 0xaf:	/* ƒT?ƒrƒXƒtƒH?ƒ†? */
		if (sg->src_id == bl->id)
			break;
		if (sc_data && sc_data[type].timer != -1) {
			unit2 = (struct skill_unit *)sc_data[type].val4;
			if (unit2 && unit2->group &&
				(unit2 == src || DIFF_TICK(sg->tick, unit2->group->tick) <= 0))
				break;
		}
		status_change_start(bl, type, sg->skill_lv, sg->val1, sg->val2,
		                    (int)src, skill_get_time2(sg->skill_id, sg->skill_lv), 0);
		break;

	case 0xb4:	/* ƒoƒWƒŠƒJ */	// Basilica
		if (battle_check_target(&src->bl, bl, BCT_NOENEMY) > 0) {
			if (sc_data && sc_data[type].timer != -1) {
				struct skill_unit_group *sg2 = (struct skill_unit_group *)sc_data[type].val4;
				if (sg2 && (sg2 == src->group || DIFF_TICK(sg->tick, sg2->tick) <= 0))
					break;
			} else
				status_change_start(bl, type, sg->skill_lv, (int)src, 0, 0, skill_get_time2(sg->skill_id, sg->skill_lv), 0);
		} else if (!status_get_mode(bl) & 0x20)
			skill_blown(&src->bl, bl, 1);
		break;

	case 0xb6:				/* ƒtƒHƒOƒEƒH?ƒ‹ */
		if (sc_data && sc_data[type].timer != -1) {
			unit2 = (struct skill_unit *)sc_data[type].val4;
			if (unit2 && unit2->group &&
			    (unit2 == src || DIFF_TICK(sg->tick, unit2->group->tick) <= 0))
				break;
		}
		status_change_start(bl, type, sg->skill_lv, sg->val1, sg->val2,
		                    (int)src, skill_get_time2(sg->skill_id, sg->skill_lv), 0);
		skill_additional_effect(ss, bl, sg->skill_id, sg->skill_lv, BF_MISC, tick);
		break;

	case 0xb2:				/* ‚ ‚È‚½‚ğ_?‚¢‚½‚¢‚Å‚· */
	case 0xb3:				/* ƒSƒXƒyƒ‹ */
	//case 0xb6:				/* ƒtƒHƒOƒEƒH?ƒ‹ */ - moved [celest]
	//‚Æ‚è‚ ‚¦‚¸‰½‚à‚µ‚È‚¢
		break;
	/*	default:
		if (battle_config.error_log)
			printf("skill_unit_onplace: Unknown skill unit id=%d block=%d\n", sg->unit_id, bl->id);
		break;*/
	}

	return 0;
}

/*==========================================
 * ƒXƒLƒ‹ƒ†ƒjƒbƒg‚Ì”­“®ƒCƒxƒ“ƒg(ƒ^ƒCƒ}[”­“®)
 *------------------------------------------
 */
int skill_unit_onplace_timer(struct skill_unit *src, struct block_list *bl, unsigned int tick)
{
	struct skill_unit_group *sg;
	struct block_list *ss;
	int splash_count = 0;
	struct status_change *sc_data;
	struct skill_unit_group_tickset *ts;
	int type;
	int diff = 0;

	nullpo_retr(0, src);
	nullpo_retr(0, bl);

	if (bl->type != BL_PC && bl->type != BL_MOB)
		return 0;

	if (bl->prev == NULL || !src->alive ||
	    (bl->type == BL_PC && pc_isdead((struct map_session_data *)bl)))
		return 0;

	nullpo_retr(0, sg = src->group);
	nullpo_retr(0, ss = map_id2bl(sg->src_id));
	sc_data = status_get_sc_data(bl);
	type = SkillStatusChangeTable[sg->skill_id];

	// ‘ÎÛ‚ªLPã‚É‹‚éê‡‚Í–³Œø
	if (map_find_skill_unit_oncell(bl, bl->x, bl->y, SA_LANDPROTECTOR, NULL))
		return 0;

	// ‘O‚É‰e‹¿‚ğó‚¯‚Ä‚©‚çinterval‚ÌŠÔ‚Í‰e‹¿‚ğó‚¯‚È‚¢
	nullpo_retr(0, ts = skill_unitgrouptickset_search(bl, sg, tick));
	diff = DIFF_TICK(tick, ts->tick);
	if (sg->skill_id == PR_SANCTUARY)
		diff += 500; // V‹K‚É‰ñ•œ‚µ‚½ƒ†ƒjƒbƒg‚¾‚¯ƒJƒEƒ“ƒg‚·‚é‚½‚ß‚ÌdŠ|‚¯
	if (diff < 0)
		return 0;
	ts->tick = tick+sg->interval;
	// GX‚Íd‚È‚Á‚Ä‚¢‚½‚ç3HIT‚µ‚È‚¢
	if (sg->skill_id == CR_GRANDCROSS && !battle_config.gx_allhit)
		ts->tick += sg->interval * (map_count_oncell(bl->m, bl->x, bl->y) - 1);

	switch (sg->unit_id) {
	case 0x83:	/* ƒTƒ“ƒNƒ`ƒ…ƒAƒŠ */
		{
			int race = status_get_race(bl);

			if (battle_check_undead(race, status_get_elem_type(bl)) || race == 6) {
				if (skill_attack(BF_MAGIC, ss, &src->bl, bl, sg->skill_id, sg->skill_lv, tick, 0)) {
					// reduce healing count if this was meant for damaging [celest]
					// sg->val1 /= 2;
					sg->val1--;	// ƒ`ƒƒƒbƒgƒLƒƒƒ“ƒZƒ‹‚É‘Î‰
				}
			} else {
				int heal = sg->val2;
				if (status_get_hp(bl) >= status_get_max_hp(bl))
					break;
				if (bl->type == BL_PC && ((struct map_session_data *)bl)->special_state.no_magic_damage)
					heal = 0; /* ‰©‹àå³ƒJ[ƒhiƒq[ƒ‹—Ê‚Oj */
				clif_skill_nodamage(&src->bl, bl, AL_HEAL, heal, 1);
				battle_heal(NULL, bl, heal, 0, 0);
				if (diff >= 500)
					sg->val1--; // V‹K‚É“ü‚Á‚½ƒ†ƒjƒbƒg‚¾‚¯ƒJƒEƒ“ƒg
			}
			if (sg->val1 <= 0)
				skill_delunitgroup(sg);
			break;
		}

	case 0x84:	/* ƒ}ƒOƒkƒXƒGƒNƒ\ƒVƒYƒ€ */
		{
			int race = status_get_race(bl);
			if (!battle_check_undead(race, status_get_elem_type(bl)) && race != 6)
				return 0;
			skill_attack(BF_MAGIC, ss, &src->bl, bl, sg->skill_id, sg->skill_lv, tick, 0);
			src->val2++;
			break;
		}

	case 0x7f:	/* ƒtƒ@ƒCƒ„[ƒEƒH[ƒ‹ */
		skill_attack(BF_MAGIC, ss, &src->bl, bl, sg->skill_id, sg->skill_lv, tick, 0);
		if (--src->val2 <= 0)
			skill_delunit(src);
		break;
	case 0x86:	/* ƒ[ƒhƒIƒuƒ”ƒ@[ƒ~ƒŠƒIƒ“(TS,MS,FN,SG,HD,GX,ˆÅGX) */
		skill_attack(BF_MAGIC, ss, &src->bl, bl, sg->skill_id, sg->skill_lv, tick, 0);
		break;
	case 0x87:	/* ƒtƒ@ƒCƒA[ƒsƒ‰[(”­“®‘O) */
		skill_delunit(src);
		skill_unitsetting(ss, sg->skill_id, sg->skill_lv, src->bl.x, src->bl.y, 1);
		break;

	case 0x88:	/* ƒtƒ@ƒCƒA[ƒsƒ‰[(”­“®Œã) */
		map_foreachinarea(skill_attack_area, bl->m, bl->x - 1, bl->y - 1, bl->x + 1, bl->y + 1, 0,
		                  BF_MAGIC, ss, &src->bl, sg->skill_id, sg->skill_lv, tick, 0, BCT_ENEMY);  // area damage [Celest]
		//skill_attack(BF_MAGIC,ss,&src->bl,bl,sg->skill_id,sg->skill_lv,tick,0);
		break;

	case 0x90:	/* ƒXƒLƒbƒhƒgƒ‰ƒbƒv */
		{
			int i, c = skill_get_blewcount(sg->skill_id, sg->skill_lv);
			if (map[bl->m].flag.gvg) c = 0;
			for(i = 0; i < c; i++)
				skill_blown(&src->bl, bl, 1 | 0x30000);
			sg->unit_id = 0x8c;
			clif_changelook(&src->bl, LOOK_BASE, sg->unit_id);
			sg->limit = DIFF_TICK(tick, sg->tick) + 1500;
		}
		break;

	case 0x93:	/* ƒ‰ƒ“ƒhƒ}ƒCƒ“ */
		skill_attack(BF_MISC,ss,&src->bl,bl,sg->skill_id,sg->skill_lv,tick,0);
		sg->unit_id = 0x8c;
		clif_changelook(&src->bl,LOOK_BASE,0x88);
		sg->limit=DIFF_TICK(tick,sg->tick)+1500;
		break;

	case 0x8f:	/* ƒuƒ‰ƒXƒgƒ}ƒCƒ“ */
	case 0x94:	/* ƒVƒ‡ƒbƒNƒEƒF[ƒuƒgƒ‰ƒbƒv */
	case 0x95:	/* ƒTƒ“ƒhƒ}ƒ“ */
	case 0x96:	/* ƒtƒ‰ƒbƒVƒƒ[ */
	case 0x97:	/* ƒtƒŠ[ƒWƒ“ƒOƒgƒ‰ƒbƒv */
	case 0x98:	/* ƒNƒŒƒCƒ‚ƒA[ƒgƒ‰ƒbƒv */
		map_foreachinarea(skill_count_target,src->bl.m
					,src->bl.x-src->range,src->bl.y-src->range
					,src->bl.x+src->range,src->bl.y+src->range
					,0,&src->bl,&splash_count);
		map_foreachinarea(skill_trap_splash,src->bl.m
					,src->bl.x-src->range,src->bl.y-src->range
					,src->bl.x+src->range,src->bl.y+src->range
					,0,&src->bl,tick,splash_count);
		sg->unit_id = 0x8c;
		clif_changelook(&src->bl,LOOK_BASE,sg->unit_id);
		sg->limit=DIFF_TICK(tick,sg->tick)+1500;
		break;

	case 0x91:	/* ƒAƒ“ƒNƒ‹ƒXƒlƒA */
		if (sg->val2 == 0 && sc_data && sc_data[SC_ANKLE].timer == -1) {
			int moveblock = (bl->x / BLOCK_SIZE != src->bl.x / BLOCK_SIZE || bl->y / BLOCK_SIZE != src->bl.y / BLOCK_SIZE);
			int sec = skill_get_time2(sg->skill_id, sg->skill_lv) - status_get_agi(bl) * 100;
			if (status_get_mode(bl) & 0x20) // Lasts 5 times less on bosses
				sec = sec / 5;
			if (sec < 3000) // Minimum trap time of 3 seconds [celest]
				sec = 3000;
			battle_stopwalking(bl, 1);
			status_change_start(bl, SC_ANKLE, sg->skill_lv, 0, 0, 0, sec, 0);
			skill_unit_move(bl, tick, 0);
			if (moveblock) map_delblock(bl);
			bl->x = src->bl.x;
			bl->y = src->bl.y;
			if (moveblock) map_addblock(bl);
			skill_unit_move(bl, tick, 1);
			if (bl->type == BL_MOB)
				clif_fixmobpos((struct mob_data *)bl);
			else if (bl->type == BL_PET)
				clif_fixpetpos((struct pet_data *)bl);
			else
				clif_fixpos(bl);
			clif_01ac(&src->bl);
			sg->limit = DIFF_TICK(tick, sg->tick) + sec;
			sg->val2 = bl->id;
			sg->interval = -1;
			src->range = 0;
		}
		break;

	case 0x92:	/* ƒxƒmƒ€ƒ_ƒXƒg */
		if (sc_data && sc_data[type].timer == -1)
			status_change_start(bl, type, sg->skill_lv, (int)src, 0, 0, skill_get_time2(sg->skill_id, sg->skill_lv), 0);
		break;

	case 0xb1:	/* ƒfƒ‚ƒ“ƒXƒgƒŒ[ƒVƒ‡ƒ“ */
		skill_attack(BF_WEAPON,ss,&src->bl,bl,sg->skill_id,sg->skill_lv,tick,0);
		if(bl->type == BL_PC && rand()%100 < sg->skill_lv && battle_config.equipment_breaking)
			pc_breakweapon((struct map_session_data *)bl);
		break;

	case 0x99:				/* ƒg[ƒL[ƒ{ƒbƒNƒX */
		if(sg->src_id == bl->id) //©•ª‚ª“¥‚ñ‚Å‚à”­“®‚µ‚È‚¢
			break;
		if(sg->val2==0){
			clif_talkiebox(&src->bl,sg->valstr);
			sg->unit_id = 0x8c;
			clif_changelook(&src->bl,LOOK_BASE,sg->unit_id);
			sg->limit=DIFF_TICK(tick,sg->tick)+5000;
			sg->val2=-1; //“¥‚ñ‚¾
		}
		break;

	// Basilica
	case 0xb4:				/* ƒoƒWƒŠƒJ */
		if (battle_check_target(&src->bl, bl, BCT_ENEMY) > 0 && !(status_get_mode(bl) & 0x20))
			skill_blown(&src->bl, bl, 1);
		if (sg->src_id == bl->id)
			break;
		if (battle_check_target(&src->bl, bl, BCT_NOENEMY) > 0 && sc_data && sc_data[type].timer == -1)
			status_change_start(bl, type, sg->skill_lv, (int)src, 0, 0, skill_get_time2(sg->skill_id, sg->skill_lv), 0);
		break;

	case 0xb7:	/* ƒXƒpƒCƒ_[ƒEƒFƒbƒu */
		if(sg->val2==0){
			int moveblock = ( bl->x/BLOCK_SIZE != src->bl.x/BLOCK_SIZE || bl->y/BLOCK_SIZE != src->bl.y/BLOCK_SIZE);
			skill_additional_effect(ss,bl,sg->skill_id,sg->skill_lv,BF_MISC,tick);
			skill_unit_move(bl, tick, 0);
			if (moveblock) map_delblock(bl);
			bl->x = src->bl.x;
			bl->y = src->bl.y;
			if (moveblock) map_addblock(bl);
			skill_unit_move(bl, tick, 1);
			if (bl->type == BL_MOB)
				clif_fixmobpos((struct mob_data *)bl);
			else if (bl->type == BL_PET)
				clif_fixpetpos((struct pet_data *)bl);
			else
				clif_fixpos(bl);
			sg->limit = DIFF_TICK(tick, sg->tick) + skill_get_time2(sg->skill_id, sg->skill_lv);
			sg->val2 = bl->id;
			sg->interval = -1;
			src->range = 0;
		}
		break;

/*	default:
		if(battle_config.error_log)
			printf("skill_unit_onplace: Unknown skill unit id=%d block=%d\n",sg->unit_id,bl->id);
		break;*/
	}

	if(bl->type==BL_MOB && ss!=bl)	/* ƒXƒLƒ‹g—pğŒ‚ÌMOBƒXƒLƒ‹ */
	{
		if(battle_config.mob_changetarget_byskill == 1)
		{
			int target=((struct mob_data *)bl)->target_id;
			if(ss->type == BL_PC)
				((struct mob_data *)bl)->target_id=ss->id;
			mobskill_use((struct mob_data *)bl,tick,MSC_SKILLUSED|(sg->skill_id<<16));
			((struct mob_data *)bl)->target_id=target;
		}
		else
			mobskill_use((struct mob_data *)bl,tick,MSC_SKILLUSED|(sg->skill_id<<16));
	}

	return 0;
}

/*==========================================
 * ƒXƒLƒ‹ƒ†ƒjƒbƒg‚©‚ç—£’E‚·‚é(‚à‚µ‚­‚Í‚µ‚Ä‚¢‚é)ê‡
 *------------------------------------------
 */
int skill_unit_onout(struct skill_unit *src, struct block_list *bl, unsigned int tick) {
	struct skill_unit_group *sg;
	struct status_change *sc_data;
	int type;

	nullpo_retr(0, src);
	nullpo_retr(0, bl);
	nullpo_retr(0, sg = src->group);

	sc_data = status_get_sc_data(bl);
	type = SkillStatusChangeTable[sg->skill_id];

	if (bl->prev == NULL || !src->alive ||
	    (bl->type == BL_PC && pc_isdead((struct map_session_data *)bl)))
		return 0;

	switch(sg->unit_id){
	case 0x7e:	/* ƒZƒCƒtƒeƒBƒEƒH[ƒ‹ */
	case 0x85:	/* ƒjƒ…[ƒ} */
	case 0x8e:	/* ƒNƒ@ƒOƒ}ƒCƒA */
	case 0x9a:	/* ƒ{ƒ‹ƒP[ƒm */
	case 0x9b:	/* ƒfƒŠƒ…[ƒW */
	case 0x9c:	/* ƒoƒCƒIƒŒƒ“ƒgƒQƒCƒ‹ */
		if (type == SC_QUAGMIRE && bl->type == BL_MOB)
			break;
		if (sc_data && sc_data[type].timer != -1 && sc_data[type].val2 == (int)src) {
			status_change_end(bl, type, -1);
		}
		break;

	case 0x91:	/* ƒAƒ“ƒNƒ‹ƒXƒlƒA */
		{
			struct block_list *target = map_id2bl(sg->val2);
			if (target && target == bl) {
				status_change_end(bl, SC_ANKLE, -1);
				sg->limit = DIFF_TICK(tick, sg->tick) + 1000;
			}
		}
		break;

	case 0x9e:	/* qç‰S */
	case 0x9f:	/* ƒjƒˆƒ‹ƒh‚Ì‰ƒ */
	case 0xa0:	/* ‰i‰“‚Ì¬“× */
	case 0xa1:	/* í‘¾ŒÛ‚Ì‹¿‚« */
	case 0xa2:	/* ƒj[ƒxƒ‹ƒ“ƒO‚Ìw—Ö */
	case 0xa3:	/* ƒƒL‚Ì‹©‚Ñ */
	case 0xa4:	/* [•£‚Ì’†‚É */
	case 0xa5:	/* •s€g‚ÌƒW[ƒNƒtƒŠ[ƒh */
	case 0xad:	/* „‚ğ–Y‚ê‚È‚¢‚Åc */
		if (sc_data[type].timer != -1 && sc_data[type].val4 == (int)src) {
			status_change_end(bl, type, -1);
		}
		break;

	case 0xa6:	/* •s‹¦˜a‰¹ */
	case 0xa7:	/* Œû“J */
	case 0xa8:	/* —[—z‚ÌƒAƒTƒVƒ“ƒNƒƒX */
	case 0xa9:	/* ƒuƒ‰ƒM‚Ì */
	case 0xaa:	/* ƒCƒhƒDƒ“‚Ì—ÑŒç */
	case 0xab:	/* ©•ªŸè‚Èƒ_ƒ“ƒX */
	case 0xac:	/* ƒnƒ~ƒ“ƒO */
	case 0xae:	/* K‰^‚ÌƒLƒX */
	case 0xaf:	/* ƒT[ƒrƒXƒtƒH[ƒ†[ */
		status_change_start(bl, SkillStatusChangeTable[sg->skill_id], sg->skill_lv, 0, 0, 0, 20000, 0);
		break;

	case 0xb4:				/* ƒoƒWƒŠƒJ */ // basilica
		if (sc_data[type].timer != -1 && sc_data[type].val4 == (int)sg) {
			status_change_end(bl, type, -1);
		}
		break;

	case 0xb6:
	  {
		struct block_list *target = map_id2bl(sg->val2);
		if (target && target == bl) {
			status_change_end(bl, SC_FOGWALL, -1);
			if (sc_data && sc_data[SC_BLIND].timer != -1)
				sc_data[SC_BLIND].timer = add_timer(gettick_cache + 30000, status_change_timer, bl->id, 0);
		}
	  }
		break;

	case 0xb7:	/* ƒXƒpƒCƒ_[ƒEƒFƒbƒu */
	  {
		struct block_list *target = map_id2bl(sg->val2);
		if (target && target == bl)
			status_change_end(bl, SC_SPIDERWEB, -1);
		sg->limit = DIFF_TICK(tick, sg->tick) + 1000;
	  }
		break;

/*	default:
		if (battle_config.error_log)
			printf("skill_unit_onout: Unknown skill unit id=%d block=%d\n", sg->unit_id, bl->id);
		break;*/
	}

	return 0;
}

/*==========================================
 * ƒXƒLƒ‹ƒ†ƒjƒbƒgŒø‰Ê”­“®/—£’Eˆ—(foreachinarea)
 *    bl: ƒ†ƒjƒbƒg(BL_PC/BL_MOB)
 *------------------------------------------
 */
int skill_unit_effect(struct block_list *bl, va_list ap)
{
	struct skill_unit *unit;
	struct skill_unit_group *group;
	int flag;
	unsigned int tick;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);
	nullpo_retr(0, unit = va_arg(ap, struct skill_unit*));
	tick = va_arg(ap, unsigned int);
	flag = va_arg(ap, unsigned int);

	if (bl->type != BL_PC && bl->type != BL_MOB)
		return 0;

	if (!unit->alive || bl->prev == NULL)
		return 0;

	nullpo_retr(0, group = unit->group);

	if (flag)
		skill_unit_onplace(unit, bl, tick);
	else {
		skill_unit_onout(unit, bl, tick);
		unit = map_find_skill_unit_oncell(bl, bl->x, bl->y, group->skill_id, unit);
		if (unit)
			skill_unit_onplace(unit, bl, tick);
	}

	return 0;
}

/*==========================================
 * ƒXƒLƒ‹ƒ†ƒjƒbƒg‚ÌŒÀŠEƒCƒxƒ“ƒg
 *------------------------------------------
 */
int skill_unit_onlimit(struct skill_unit *src,unsigned int tick)
{
	struct skill_unit_group *sg;

	nullpo_retr(0, src);
	nullpo_retr(0, sg=src->group);

	switch(sg->unit_id){
	case 0x81:	/* ƒ[ƒvƒ|[ƒ^ƒ‹(”­“®‘O) */
		{
			struct skill_unit_group *group=
				skill_unitsetting(map_id2bl(sg->src_id),sg->skill_id,sg->skill_lv,
					src->bl.x,src->bl.y,1);
			if (group == NULL)
				return 0;
			CALLOC(group->valstr, char, 25); // 24 + NULL
			strncpy(group->valstr, sg->valstr, 24);
			group->val2 = sg->val2;
		}
		break;

	case 0x8d:	/* ƒAƒCƒXƒEƒH[ƒ‹ */
		map_setcell(src->bl.m, src->bl.x, src->bl.y, src->val2);
		clif_changemapcell(src->bl.m, src->bl.x, src->bl.y, src->val2, 1);
		break;

	case 0xb2:	/* ‚ ‚È‚½‚É‰ï‚¢‚½‚¢ */
	  {
		struct map_session_data *sd = NULL;
		struct map_session_data *p_sd = NULL;
		if ((sd = map_id2sd(sg->src_id)) == NULL)
			return 0;
		if ((p_sd = pc_get_partner(sd)) == NULL)
			return 0;

		pc_setpos(p_sd, map[src->bl.m].name, src->bl.x, src->bl.y, 3);
	  }
		break;
	}

	return 0;
}

/*==========================================
 * ƒXƒLƒ‹ƒ†ƒjƒbƒg‚Ìƒ_ƒ[ƒWƒCƒxƒ“ƒg
 *------------------------------------------
 */
int skill_unit_ondamaged(struct skill_unit *src,struct block_list *bl,
	int damage,unsigned int tick)
{
	struct skill_unit_group *sg;

	nullpo_retr(0, src);
	nullpo_retr(0, sg=src->group);

	switch(sg->unit_id){
	case 0x8d:	/* ƒAƒCƒXƒEƒH[ƒ‹ */
		src->val1-=damage;
		break;
	case 0x8f:	/* ƒuƒ‰ƒXƒgƒ}ƒCƒ“ */
	case 0x98:	/* ƒNƒŒƒCƒ‚ƒA[ƒgƒ‰ƒbƒv */
		skill_blown(bl,&src->bl,2); //‚«”ò‚Î‚µ‚Ä‚İ‚é
		break;
	default:
		damage = 0;
		break;
	}

	return damage;
}


/*---------------------------------------------------------------------------- */

/*==========================================
 * ƒXƒLƒ‹g—pi‰r¥Š®—¹AêŠw’èj
 *------------------------------------------
 */
int skill_castend_pos(int tid, unsigned int tick, int id, int data)
{
	struct map_session_data* sd = map_id2sd(id)/*,*target_sd=NULL*/;
	int range,maxcount;

	nullpo_retr(0, sd);

	if (sd->bl.prev == NULL)
		return 0;
	if (sd->skilltimer != tid) /* ƒ^ƒCƒ}ID‚ÌŠm”F */
		return 0;
	if (sd->skilltimer != -1 && pc_checkskill(sd, SA_FREECAST) > 0) {
		sd->speed = sd->prev_speed;
		clif_updatestatus(sd, SP_SPEED);
	}

	sd->skilltimer = -1;

	if (pc_isdead(sd)) {
		sd->canact_tick = tick;
		sd->canmove_tick = tick;
		sd->skillitem = sd->skillitemlv = -1;
		return 0;
	}

	/*		case MG_SAFETYWALL:
			case WZ_FIREPILLAR:
			case HT_SKIDTRAP:
			case HT_LANDMINE:
			case HT_ANKLESNARE:
			case HT_SHOCKWAVE:
			case HT_SANDMAN:
			case HT_FLASHER:
			case HT_FREEZINGTRAP:
			case HT_BLASTMINE:
			case HT_CLAYMORETRAP:
			case HT_TALKIEBOX:
			case AL_WARP:*/
//			case PF_SPIDERWEB:		/* ƒXƒpƒCƒ_[ƒEƒFƒbƒu */
//			case RG_GRAFFITI:		/* ƒOƒ‰ƒtƒBƒeƒB */
/*				range = 0;
				break;
			case AL_PNEUMA:
				range = 1;
				break;*/
	if ((!battle_config.pc_skill_reiteration &&
	     skill_get_unit_flag(sd->skillid) & UF_NOREITERATION &&
	     skill_check_unit_range(sd->bl.m, sd->skillx, sd->skilly, sd->skillid, sd->skilllv)) ||
	    (map_getcell(sd->bl.m, sd->skillx, sd->skilly, CELL_CHKNOPASS))) { // not "Wall trapping" fixed by [Mikey] from freya's bug report
		clif_skill_fail(sd, sd->skillid, 0, 0);
		sd->canact_tick = tick;
		sd->canmove_tick = tick;
		sd->skillitem = sd->skillitemlv = -1;
		return 0;
	}

	/*		case WZ_FIREPILLAR:
			case HT_SKIDTRAP:
			case HT_LANDMINE:
			case HT_ANKLESNARE:
			case HT_SHOCKWAVE:
			case HT_SANDMAN:
			case HT_FLASHER:
			case HT_FREEZINGTRAP:
			case HT_BLASTMINE:
			case HT_CLAYMORETRAP:
			case HT_TALKIEBOX:*/
//			case PF_SPIDERWEB:		/* ƒXƒpƒCƒ_[ƒEƒFƒbƒu */
/*			case WZ_ICEWALL:
				range = 2;
				break;
			case AL_WARP:
				range = 0;
				break;*/
	if (battle_config.pc_skill_nofootset &&
	    skill_get_unit_flag(sd->skillid) & UF_NOFOOTSET &&
	    skill_check_unit_range2(sd->bl.m, sd->skillx, sd->skilly, sd->skillid, sd->skilllv)) {
		clif_skill_fail(sd, sd->skillid, 0, 0);
		sd->canact_tick = tick;
		sd->canmove_tick = tick;
		sd->skillitem = sd->skillitemlv = -1;
		return 0;
	}

	if (battle_config.pc_land_skill_limit) {
		maxcount = skill_get_maxcount(sd->skillid);
		if(maxcount > 0) {
			int i,c;
			for(i=c=0;i<MAX_SKILLUNITGROUP;i++) {
				if(sd->skillunit[i].alive_count > 0 && sd->skillunit[i].skill_id == sd->skillid)
					c++;
			}
			if (c >= maxcount) {
				clif_skill_fail(sd, sd->skillid, 0, 0);
				sd->canact_tick = tick;
				sd->canmove_tick = tick;
				return 0;
			}
		}
	}

	if (sd->skilllv <= 0) return 0;
	range = skill_get_range(sd->skillid,sd->skilllv);
	if (range < 0)
		range = status_get_range(&sd->bl) - (range + 1);
	range += battle_config.pc_skill_add_range;
	if (battle_config.skill_out_range_consume) { // changed to allow casting when target walks out of range [Valaris]
		if (range < distance(sd->bl.x,sd->bl.y,sd->skillx,sd->skilly)) {
			clif_skill_fail(sd, sd->skillid, 0, 0);
			sd->canact_tick = tick;
			sd->canmove_tick = tick;
			return 0;
		}
	}
	if (!skill_check_condition(sd, 1)) { /* g—pğŒƒ`ƒFƒbƒN */
		sd->canact_tick = tick;
		sd->canmove_tick = tick;
		sd->skillitem = sd->skillitemlv = -1;
		return 0;
	}
	sd->skillitem = sd->skillitemlv = -1;
	if (battle_config.skill_out_range_consume) {
		if (range < distance(sd->bl.x, sd->bl.y, sd->skillx, sd->skilly)) {
			clif_skill_fail(sd, sd->skillid, 0, 0);
			sd->canact_tick = tick;
			sd->canmove_tick = tick;
			return 0;
		}
	}

	if (battle_config.pc_skill_log)
		printf("PC %d skill castend skill=%d\n",sd->bl.id,sd->skillid);
	pc_stop_walking(sd,0);

	skill_castend_pos2(&sd->bl, sd->skillx, sd->skilly, sd->skillid, sd->skilllv, tick, 0);

	return 0;
}

/*==========================================
 * ”ÍˆÍ“àƒLƒƒƒ‰‘¶İŠm”F”»’èˆ—(foreachinarea)
 *------------------------------------------
 */

static int skill_check_condition_char_sub(struct block_list *bl,va_list ap)
{
	int *c;
	struct block_list *src;
	struct map_session_data *sd;
	struct map_session_data *ssd;
	struct pc_base_job s_class;
	struct pc_base_job ss_class;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);
	nullpo_retr(0, sd=(struct map_session_data*)bl);
	nullpo_retr(0, src=va_arg(ap,struct block_list *));
	nullpo_retr(0, c=va_arg(ap,int *));
	nullpo_retr(0, ssd=(struct map_session_data*)src);

	s_class = pc_calc_base_job(sd->status.class);
	//ƒ`ƒFƒbƒN‚µ‚È‚¢İ’è‚È‚çc‚É‚ ‚è‚¦‚È‚¢‘å‚«‚È”š‚ğ•Ô‚µ‚ÄI—¹
	if(!battle_config.player_skill_partner_check){	//–{“–‚Íforeach‚Ì‘O‚É‚â‚è‚½‚¢‚¯‚Çİ’è“K—p‰ÓŠ‚ğ‚Ü‚Æ‚ß‚é‚½‚ß‚É‚±‚±‚Ö
		(*c)=99;
		return 0;
	}

	ss_class = pc_calc_base_job(ssd->status.class);

	switch(ssd->skillid){
	case PR_BENEDICTIO:				/* ¹‘Ì~•Ÿ */
		if(sd != ssd && (s_class.job == 4 || s_class.job == 8 || s_class.job == 15) &&
			(sd->bl.x == ssd->bl.x - 1 || sd->bl.x == ssd->bl.x + 1) && sd->status.sp >= 10)
			(*c)++;
		break;
	case BD_LULLABY:				/* qç‰Ì */
	case BD_RICHMANKIM:				/* ƒjƒˆƒ‹ƒh‚Ì‰ƒ */
	case BD_ETERNALCHAOS:			/* ‰i‰“‚Ì¬“× */
	case BD_DRUMBATTLEFIELD:		/* í‘¾ŒÛ‚Ì‹¿‚« */
	case BD_RINGNIBELUNGEN:			/* ƒj[ƒxƒ‹ƒ“ƒO‚Ìw—Ö */
	case BD_ROKISWEIL:				/* ƒƒL‚Ì‹©‚Ñ */
	case BD_INTOABYSS:				/* [•£‚Ì’†‚É */
	case BD_SIEGFRIED:				/* •s€g‚ÌƒW[ƒNƒtƒŠ[ƒh */
	case BD_RAGNAROK:				/* _X‚Ì‰©¨ */
	case CG_MOONLIT:				/* Œ–¾‚è‚Ìò‚É—‚¿‚é‰Ô‚Ñ‚ç */
		if(sd != ssd &&
		 ((ss_class.job==19 && s_class.job==20) ||
		 (ss_class.job==20 && s_class.job==19)) &&
		 pc_checkskill(sd,ssd->skillid) > 0 &&
		 (*c)==0 &&
		 sd->status.party_id == ssd->status.party_id &&
		 !pc_issit(sd) &&
		 sd->sc_data[SC_DANCING].timer == -1
		 )
			(*c)=pc_checkskill(sd,ssd->skillid);
		break;
	}

	return 0;
}

/*==========================================
 * ”ÍˆÍ“àƒLƒƒƒ‰‘¶İŠm”F”»’èŒãƒXƒLƒ‹g—pˆ—(foreachinarea)
 *------------------------------------------
 */

static int skill_check_condition_use_sub(struct block_list *bl,va_list ap)
{
	int *c;
	struct block_list *src;
	struct map_session_data *sd;
	struct map_session_data *ssd;
	struct pc_base_job s_class;
	struct pc_base_job ss_class;
	int skillid, skilllv;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);
	nullpo_retr(0, sd=(struct map_session_data*)bl);
	nullpo_retr(0, src=va_arg(ap,struct block_list *));
	nullpo_retr(0, c=va_arg(ap,int *));
	nullpo_retr(0, ssd=(struct map_session_data*)src);

	s_class = pc_calc_base_job(sd->status.class);

	//ƒ`ƒFƒbƒN‚µ‚È‚¢İ’è‚È‚çc‚É‚ ‚è‚¦‚È‚¢‘å‚«‚È”š‚ğ•Ô‚µ‚ÄI—¹
	if(!battle_config.player_skill_partner_check){	//–{“–‚Íforeach‚Ì‘O‚É‚â‚è‚½‚¢‚¯‚Çİ’è“K—p‰ÓŠ‚ğ‚Ü‚Æ‚ß‚é‚½‚ß‚É‚±‚±‚Ö
		(*c)=99;
		return 0;
	}

	ss_class = pc_calc_base_job(ssd->status.class);
	skillid=ssd->skillid;
	skilllv=ssd->skilllv;
	if (skillid > 0 && skilllv <= 0) return 0; // celest
	switch(skillid){
	case PR_BENEDICTIO:				/* ¹‘Ì~•Ÿ */
		if(sd != ssd && (s_class.job == 4 || s_class.job == 8 || s_class.job == 15) &&
			(sd->bl.x == ssd->bl.x - 1 || sd->bl.x == ssd->bl.x + 1) && sd->status.sp >= 10){
			sd->status.sp -= 10;
			status_calc_pc(sd,0);
			(*c)++;
		}
		break;
	case BD_LULLABY:				/* qç‰Ì */
	case BD_RICHMANKIM:				/* ƒjƒˆƒ‹ƒh‚Ì‰ƒ */
	case BD_ETERNALCHAOS:			/* ‰i‰“‚Ì¬“× */
	case BD_DRUMBATTLEFIELD:		/* í‘¾ŒÛ‚Ì‹¿‚« */
	case BD_RINGNIBELUNGEN:			/* ƒj[ƒxƒ‹ƒ“ƒO‚Ìw—Ö */
	case BD_ROKISWEIL:				/* ƒƒL‚Ì‹©‚Ñ */
	case BD_INTOABYSS:				/* [•£‚Ì’†‚É */
	case BD_SIEGFRIED:				/* •s€g‚ÌƒW[ƒNƒtƒŠ[ƒh */
	case BD_RAGNAROK:				/* _X‚Ì‰©¨ */
	case CG_MOONLIT:				/* Œ–¾‚è‚Ìò‚É—‚¿‚é‰Ô‚Ñ‚ç */
		if(sd != ssd && //–{lˆÈŠO‚Å
		  ((ss_class.job==19 && s_class.job==20) || //©•ª‚ªƒo[ƒh‚È‚çƒ_ƒ“ƒT[‚Å
		   (ss_class.job==20 && s_class.job==19)) && //©•ª‚ªƒ_ƒ“ƒT[‚È‚çƒo[ƒh‚Å
		   pc_checkskill(sd,skillid) > 0 && //ƒXƒLƒ‹‚ğ‚Á‚Ä‚¢‚Ä
		   (*c)==0 && //Å‰‚Ìˆêl‚Å
		   sd->status.party_id == ssd->status.party_id && //ƒp[ƒeƒB[‚ª“¯‚¶‚Å
		   !pc_issit(sd) && //À‚Á‚Ä‚È‚¢
		   sd->sc_data[SC_DANCING].timer == -1 //ƒ_ƒ“ƒX’†‚¶‚á‚È‚¢
		  ){
			ssd->sc_data[SC_DANCING].val4=bl->id;
			clif_skill_nodamage(bl,src,skillid,skilllv,1);
			status_change_start(bl,SC_DANCING,skillid,ssd->sc_data[SC_DANCING].val2,0,src->id,skill_get_time(skillid,skilllv)+1000,0);
			sd->skillid_dance = sd->skillid = skillid;
			sd->skilllv_dance = sd->skilllv = skilllv;
			(*c)++;
		}
		break;
	}

	return 0;
}

/*==========================================
 * ”ÍˆÍ“àƒoƒCƒIƒvƒ‰ƒ“ƒgAƒXƒtƒBƒAƒ}ƒCƒ“—pMob‘¶İŠm”F”»’èˆ—(foreachinarea)
 *------------------------------------------
 */

static int skill_check_condition_mob_master_sub(struct block_list *bl,va_list ap)
{
	int *c,src_id=0,mob_class=0;
	struct mob_data *md;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);
	nullpo_retr(0, md=(struct mob_data*)bl);
	nullpo_retr(0, src_id=va_arg(ap,int));
	nullpo_retr(0, mob_class=va_arg(ap,int));
	nullpo_retr(0, c=va_arg(ap,int *));

	if(md->class==mob_class && md->master_id==src_id)
		(*c)++;

	return 0;
}

/*==========================================
 * ƒXƒLƒ‹g—pğŒi‹U‚Åg—p¸”sj
 *------------------------------------------
 */
int skill_check_condition(struct map_session_data *sd, int type) {
	int i, hp, sp, hp_rate, sp_rate, zeny, weapon, state, spiritball, skill, lv, mhp;
	int idx[10], itemid[10], amount[10];
	int arrow_flag = 0;

	nullpo_retr(0, sd);

	// If the target is under chase walk, cant use any skill, except chasewalk itself - [Aalye] - freya' forum
	if (sd->sc_data[SC_CHASEWALK].timer != -1 && sd->skillid != ST_CHASEWALK)
		return 0;

	if (battle_config.gm_skilluncond > 0 && sd->GM_level >= battle_config.gm_skilluncond) {
		sd->skillitem = sd->skillitemlv = -1;
		return 1;
	}

	if (sd->opt1 > 0) {
		clif_skill_fail(sd, sd->skillid, 0, 0);
		return 0;
	}
	if (pc_is90overweight(sd)) {
		clif_skill_fail(sd, sd->skillid, 9, 0);
		return 0;
	}

	if (sd->skillid == AC_MAKINGARROW && sd->state.make_arrow_flag == 1) {
		sd->skillitem = sd->skillitemlv = -1;
		return 0;
	}
	if ((sd->skillid == AM_PHARMACY || sd->skillid == ASC_CDP || sd->skillid == CR_ALCHEMY) // We made that Alchemy dont require any bottle nor medecine bowl. [Aalye] from freya' forum
	    && sd->state.produce_flag == 1) {
		sd->skillitem = sd->skillitemlv = -1;
		return 0;
	}

	if (sd->skillitem == sd->skillid) { /* ƒAƒCƒeƒ€‚Ìê‡–³ğŒ¬Œ÷ */
		if (type & 1)
			sd->skillitem = sd->skillitemlv = -1;
		return 1;
	}
	if (sd->opt1 > 0) {
		clif_skill_fail(sd, sd->skillid, 0, 0);
		return 0;
	}
	if (sd->sc_count){
		if (sd->sc_data[SC_DIVINA].timer!=-1 ||
		    sd->sc_data[SC_ROKISWEIL].timer!=-1 ||
		    (sd->sc_data[SC_AUTOCOUNTER].timer != -1 && sd->skillid != KN_AUTOCOUNTER) ||
		     sd->sc_data[SC_STEELBODY].timer != -1 ||
		     sd->sc_data[SC_BERSERK].timer != -1 ||
		     (sd->sc_data[SC_MARIONETTE].timer != -1 && sd->skillid != CG_MARIONETTE)) {
			clif_skill_fail(sd, sd->skillid, 0, 0);
			return 0; /* ó‘ÔˆÙí‚â’¾–Ù‚È‚Ç */
		}
	}
	skill = sd->skillid;
	lv = sd->skilllv;
	if (lv <= 0)
		return 0;
	if (skill >= 10000 && skill < 10015)
		skill -= 9500;
	hp=skill_get_hp(skill, lv);	/* Á”ïHP */
	sp=skill_get_sp(skill, lv);	/* Á”ïSP */
	if ((sd->skillid_old == BD_ENCORE) && skill==sd->skillid_dance)
		sp=sp/2; //ƒAƒ“ƒR[ƒ‹‚ÍSPÁ”ï‚ª”¼•ª
	hp_rate = (lv <= 0) ? 0:skill_db[skill].hp_rate[lv-1];
	sp_rate = (lv <= 0) ? 0:skill_db[skill].sp_rate[lv-1];
	zeny = skill_get_zeny(skill,lv);
	weapon = skill_db[skill].weapon;
	state = skill_db[skill].state;
	spiritball = (lv <= 0) ? 0 : skill_db[skill].spiritball[lv-1];
	mhp=skill_get_mhp(skill, lv);	/* Á”ïHP */
	for(i=0;i<10;i++) {
		itemid[i] = skill_db[skill].itemid[i];
		amount[i] = skill_db[skill].amount[i];
	}
	if(mhp > 0)
		hp += (sd->status.max_hp * mhp)/100;
	if(hp_rate > 0)
		hp += (sd->status.hp * hp_rate)/100;
	else
		hp += (sd->status.max_hp * abs(hp_rate))/100;
	if(sp_rate > 0)
		sp += (sd->status.sp * sp_rate)/100;
	else
		sp += (sd->status.max_sp * abs(sp_rate))/100;
	if(sd->dsprate!=100)
		sp=sp*sd->dsprate/100;	/* Á”ïSPC³ */

	switch(skill) {
	case SA_CASTCANCEL:
		if (sd->skilltimer == -1) {
			clif_skill_fail(sd, skill, 0, 0);
			return 0;
		}
		break;
	case BS_MAXIMIZE:		/* ƒ}ƒLƒVƒ}ƒCƒYƒpƒ[ */
	case NV_TRICKDEAD:		/* €‚ñ‚¾‚Ó‚è */
	case TF_HIDING:			/* ƒnƒCƒfƒBƒ“ƒO */
	case AS_CLOAKING:		/* ƒNƒ[ƒLƒ“ƒO */
	case CR_AUTOGUARD:				/* ƒI[ƒgƒK[ƒh */
	case CR_DEFENDER:				/* ƒfƒBƒtƒFƒ“ƒ_[ */
	case ST_CHASEWALK:
		if(sd->sc_data[SkillStatusChangeTable[skill]].timer!=-1)
			return 1;			/* ‰ğœ‚·‚éê‡‚ÍSPÁ”ï‚µ‚È‚¢ */
		break;
	case AL_TELEPORT:
	case AL_WARP:
		if(map[sd->bl.m].flag.noteleport) {
			clif_skill_teleportmessage(sd,0);
			return 0;
		}
		break;
	case MO_CALLSPIRITS:	/* ‹CŒ÷ */
		if (sd->spiritball >= lv) {
			clif_skill_fail(sd, skill, 0, 0);
			return 0;
		}
		break;
	case CH_SOULCOLLECT: /* ‹¶‹CŒ÷ */
		if (sd->spiritball >= 5) {
			clif_skill_fail(sd, skill, 0, 0);
			return 0;
		}
		break;
	case MO_FINGEROFFENSIVE: //w’e
		if (sd->spiritball > 0 && sd->spiritball < spiritball) {
			spiritball = sd->spiritball;
			sd->spiritball_old = sd->spiritball;
		} else
			sd->spiritball_old = lv;
		break;
	// kRO Patch 14/12/04 - Snap dont require a spiritball when under fury [Aalye] from freya' forum
	case MO_BODYRELOCATION:
		if (sd->sc_count && sd->sc_data[SC_EXPLOSIONSPIRITS].timer != -1)
			spiritball = 0;
		break;
	case MO_CHAINCOMBO: //˜A‘Å¶
		if (sd->sc_data[SC_BLADESTOP].timer == -1) {
			if (sd->sc_data[SC_COMBO].timer == -1 || sd->sc_data[SC_COMBO].val1 != MO_TRIPLEATTACK)
				return 0;
		}
		break;
	case MO_COMBOFINISH: //–Ò—´Œ
		if (sd->sc_data[SC_COMBO].timer == -1 || sd->sc_data[SC_COMBO].val1 != MO_CHAINCOMBO)
			return 0;
		break;
	case CH_TIGERFIST: //•šŒÕŒ
		if ((sd->sc_data[SC_COMBO].timer == -1 || sd->sc_data[SC_COMBO].val1 != MO_COMBOFINISH) && !sd->state.skill_flag)
			return 0;
		break;
	case CH_CHAINCRUSH: //˜A’Œ•öŒ‚
		if (sd->sc_data[SC_COMBO].timer == -1)
			return 0;
		if (sd->sc_data[SC_COMBO].val1 != MO_COMBOFINISH && sd->sc_data[SC_COMBO].val1 != CH_TIGERFIST)
			return 0;
		break;
	case MO_EXTREMITYFIST: // ˆ¢C—…”e–PŒ
		// kRO patch 14/12/10 - Adding a timer to prevent Asura Strike to be casted immediately after snaping [Aalye]
		if (sd->sc_data[SC_COMBO].timer != -1 && sd->sc_data[SC_COMBO].val1 == MO_BODYRELOCATION)
			return 0;
		if ((sd->sc_data[SC_COMBO].timer != -1 && (sd->sc_data[SC_COMBO].val1 == MO_COMBOFINISH || sd->sc_data[SC_COMBO].val1 == CH_CHAINCRUSH)) || sd->sc_data[SC_BLADESTOP].timer!=-1)
			spiritball--;
		break;
	case BD_ADAPTATION: /* ƒAƒhƒŠƒu */
	  {
		struct skill_unit_group *group = NULL;
		if (sd->sc_data[SC_DANCING].timer == -1 || ((group = (struct skill_unit_group*)sd->sc_data[SC_DANCING].val2) && (skill_get_time(sd->sc_data[SC_DANCING].val1, group->skill_lv) - sd->sc_data[SC_DANCING].val3 * 1000) <= skill_get_time2(skill, lv))) { //ƒ_ƒ“ƒX’†‚Åg—pŒã5•bˆÈã‚Ì‚İH
			clif_skill_fail(sd, skill, 0, 0);
			return 0;
		}
	  }
		break;
	case PR_BENEDICTIO: /* ¹‘Ì~•Ÿ */
		{
			int range=1;
			int c=0;
			if(!(type&1)){
			map_foreachinarea(skill_check_condition_char_sub,sd->bl.m,
				sd->bl.x-range,sd->bl.y-range,
				sd->bl.x+range,sd->bl.y+range,BL_PC,&sd->bl,&c);
			if (c < 2) {
				clif_skill_fail(sd, skill, 0, 0);
				return 0;
			}
			}else{
				map_foreachinarea(skill_check_condition_use_sub,sd->bl.m,
					sd->bl.x-range,sd->bl.y-range,
					sd->bl.x+range,sd->bl.y+range,BL_PC,&sd->bl,&c);
			}
		}
		break;
	case WE_CALLPARTNER:		/* ‚ ‚È‚½‚Éˆ§‚¢‚½‚¢ */
		if (!sd->status.partner_id) {
			clif_skill_fail(sd, skill, 0, 0);
			return 0;
		}
		break;
	case AM_CANNIBALIZE: /* ƒoƒCƒIƒvƒ‰ƒ“ƒg */
	case AM_SPHEREMINE: /* ƒXƒtƒBƒA[ƒ}ƒCƒ“ */
		if (type & 1) {
			int c = 0;
			int summons[5] = { 1020, 1068, 1118, 1500, 1368 }; // kRO 14/12/04 Patch - Bio Cannibalize: Monsters that are spawned are different based on the skill level [Aalye] from freya' forum
			int maxcount = (skill == AM_CANNIBALIZE) ? ((lv < 6) ? 6 - lv : 1) : skill_get_maxcount(skill);
			int mob_class = (skill == AM_CANNIBALIZE) ? ((lv < 6) ? summons[lv-1] : 1368) : 1142;
			if (battle_config.pc_land_skill_limit && maxcount > 0) {
				map_foreachinarea(skill_check_condition_mob_master_sub, sd->bl.m, 0, 0, map[sd->bl.m].xs, map[sd->bl.m].ys, BL_MOB, sd->bl.id, mob_class, &c);
				if (c >= maxcount) {
					clif_skill_fail(sd, skill, 0, 0);
					return 0;
				}
			}
		}
		break;
	case MG_FIREWALL:		/* ƒtƒ@ƒCƒA[ƒEƒH[ƒ‹ */
	case WZ_QUAGMIRE:
	case PF_FOGWALL:
		/* ”§ŒÀ */
		if (battle_config.pc_land_skill_limit) {
			int maxcount = skill_get_maxcount(skill);
			if (maxcount > 0) {
				int i, c;
				c = 0;
				for(i = 0; i < MAX_SKILLUNITGROUP; i++) {
					if (sd->skillunit[i].alive_count > 0 && sd->skillunit[i].skill_id == skill)
						c++;
				}
				if (c >= maxcount || c == MAX_SKILLUNITGROUP) {
					clif_skill_fail(sd, skill, 0, 0);
					return 0;
				}
			}
		}
		break;

	// Special option to fix maximum number of icewall of a player.
	case WZ_ICEWALL:
		if (battle_config.pc_land_skill_limit) {
			if (battle_config.max_icewall > 0) {
				int i, c;
				c = 0;
				for(i = 0; i < MAX_SKILLUNITGROUP; i++) {
					if (sd->skillunit[i].alive_count > 0 && sd->skillunit[i].skill_id == skill)
						c++;
				}
				if (c >= battle_config.max_icewall || c == MAX_SKILLUNITGROUP) {
					clif_skill_fail(sd, skill, 0, 0);
					return 0;
				}
			}
		}
		break;

	case WZ_FIREPILLAR:
		/* ”§ŒÀ */
		if (lv <= 5) // no gems required at level 1-5
			itemid[0] = 0;
		if (battle_config.pc_land_skill_limit) {
			int maxcount = skill_get_maxcount(skill);
			if (maxcount > 0) {
				int i, c;
				c = 0;
				for(i= 0; i < MAX_SKILLUNITGROUP; i++) {
					if (sd->skillunit[i].alive_count > 0 && sd->skillunit[i].skill_id == skill)
						c++;
				}
				if (c >= maxcount || c == MAX_SKILLUNITGROUP) {
					clif_skill_fail(sd, skill, 0, 0);
					return 0;
				}
			}
		}
		break;

	// skills require arrows as of 12/07 [celest]
	case AC_DOUBLE:
	case AC_SHOWER:
	case AC_CHARGEARROW:
	case BA_MUSICALSTRIKE:
	case DC_THROWARROW:
	case SN_SHARPSHOOTING:
	case CG_ARROWVULCAN:
		if (sd->equip_index[10] < 0) {
			clif_arrow_fail(sd, 0);
			return 0;
		}
		arrow_flag = 1;
		break;

	case RG_BACKSTAP:
		if (sd->status.weapon == 11) {
			if (sd->equip_index[10] < 0) {
				clif_arrow_fail(sd, 0);
				return 0;
			}
			arrow_flag = 1;
		}
		break;
	}

	if (!(type & 2)) {
		if (hp > 0 && sd->status.hp < hp) {				/* HPƒ`ƒFƒbƒN */
			clif_skill_fail(sd, skill, 2, 0);		/* HP•s‘«F¸”s’Ê’m */
			return 0;
		}
		if (sp > 0 && sd->status.sp < sp) {				/* SPƒ`ƒFƒbƒN */
			clif_skill_fail(sd, skill, 1, 0);		/* SP•s‘«F¸”s’Ê’m */
			return 0;
		}
		if (zeny > 0 && sd->status.zeny < zeny) {
			clif_skill_fail(sd, skill, 5, 0);
			return 0;
		}
		if (!(weapon & (1 << sd->status.weapon))) {
			clif_skill_fail(sd, skill, 6, 0);
			return 0;
		}
		if (spiritball > 0 && sd->spiritball < spiritball) {
			clif_skill_fail(sd, skill, 0, 0);		// Ÿ†‹…•s‘«
			return 0;
		}
	}

	switch(state) {
	case ST_HIDING:
		if (!(sd->status.option & 2)) {
			clif_skill_fail(sd, skill, 0, 0);
			return 0;
		}
		break;
	case ST_CLOAKING:
		if (!pc_iscloaking(sd)) {
			clif_skill_fail(sd, skill, 0, 0);
			return 0;
		}
		break;
	case ST_HIDDEN:
		if (!pc_ishiding(sd)) {
			clif_skill_fail(sd, skill, 0, 0);
			return 0;
		}
		break;
	case ST_RIDING:
		if (!pc_isriding(sd)) {
			clif_skill_fail(sd, skill, 0, 0);
			return 0;
		}
		break;
	case ST_FALCON:
		if (!pc_isfalcon(sd)) {
			clif_skill_fail(sd, skill, 0, 0);
			return 0;
		}
		break;
	case ST_CART:
		if (!pc_iscarton(sd)) {
			clif_skill_fail(sd, skill, 0, 0);
			return 0;
		}
		break;
	case ST_SHIELD:
		if (sd->status.shield <= 0) {
			clif_skill_fail(sd, skill, 0, 0);
			return 0;
		}
		break;
	case ST_SIGHT:
		if (sd->sc_data[SC_SIGHT].timer == -1 && type & 1) {
			clif_skill_fail(sd, skill, 0, 0);
			return 0;
		}
		break;
	case ST_EXPLOSIONSPIRITS:
		if (skill == MO_EXTREMITYFIST && ((sd->sc_data[SC_COMBO].timer != -1 && (sd->sc_data[SC_COMBO].val1 == MO_COMBOFINISH || sd->sc_data[SC_COMBO].val1 == CH_CHAINCRUSH)) || sd->sc_data[SC_BLADESTOP].timer != -1)) {
			break;
		}
		if (sd->sc_data[SC_EXPLOSIONSPIRITS].timer == -1) {
			clif_skill_fail(sd, skill, 0, 0);
			return 0;
		}
		break;
	case ST_CARTBOOST:
		if(!pc_iscarton(sd) || sd->sc_data[SC_CARTBOOST].timer == -1) {
			clif_skill_fail(sd,skill,0,0);
			return 0;
		}
		break;
	case ST_RECOV_WEIGHT_RATE:
		if (battle_config.natural_heal_weight_rate <= 100 && sd->weight * 100 / sd->max_weight >= battle_config.natural_heal_weight_rate) {
			clif_skill_fail(sd, skill, 0, 0);
			return 0;
		}
		break;
	case ST_MOVE_ENABLE:
		{
			struct walkpath_data wpd;
			if (path_search(&wpd,sd->bl.m, sd->bl.x, sd->bl.y, sd->skillx, sd->skilly, 1) == -1) {
				clif_skill_fail(sd, skill, 0, 0);
				return 0;
			}
		}
		break;
	case ST_WATER:
		if((!map_getcell(sd->bl.m, sd->bl.x, sd->bl.y, CELL_CHKWATER)) && (sd->sc_data[SC_DELUGE].timer == -1)) { //…ê”»’è
			clif_skill_fail(sd, skill, 0, 0);
			return 0;
		}
		break;
	}

	for(i = 0; i < 10; i++) {
		int x = lv % 11 - 1;
		idx[i] = -1;
		if (itemid[i] <= 0)
			continue;
		if (itemid[i] >= 715 && itemid[i] <= 717 && sd->special_state.no_gemstone)
			continue;
		if (((itemid[i] >= 715 && itemid[i] <= 717) || itemid[i] == 1065) && sd->sc_data[SC_INTOABYSS].timer != -1)
			continue;
		if ((skill == AM_POTIONPITCHER ||
		     skill == CR_SLIMPITCHER) && i != x)
			continue;

		idx[i] = pc_search_inventory(sd,itemid[i]);
		if(idx[i] < 0 || sd->status.inventory[idx[i]].amount < amount[i]) {
			if (itemid[i] == 716 || itemid[i] == 717)
				clif_skill_fail(sd, skill, (7+(itemid[i]-716)), 0);
			else
				clif_skill_fail(sd, skill, 0, 0);
			return 0;
		}
	}

	if(!(type&1))
		return 1;

	if (skill != AM_POTIONPITCHER &&
	    skill != CR_SLIMPITCHER &&
	    skill != MG_STONECURSE) {
		if(skill == AL_WARP && !(type&2))
			return 1;
		for(i=0;i<10;i++) {
			if(idx[i] >= 0)
				pc_delitem(sd,idx[i],amount[i],0); // ƒAƒCƒeƒ€Á”ï
		}
		if (arrow_flag && battle_config.arrow_decrement)
			pc_delitem(sd, sd->equip_index[10], 1, 0);
	}

	if(type&2)
		return 1;

	if(sp > 0) {					// SPÁ”ï
		sd->status.sp-=sp;
		clif_updatestatus(sd,SP_SP);
	}
	if(hp > 0) {					// HPÁ”ï
		sd->status.hp-=hp;
		clif_updatestatus(sd,SP_HP);
	}
	if (zeny > 0)					// ZenyÁ”ï
		pc_payzeny(sd,zeny);
	if (spiritball > 0)				// Ÿ†‹…Á”ï
		pc_delspiritball(sd, spiritball, 0);


	return 1;
}

/*==========================================
 * ‰r¥ŠÔŒvZ
 *------------------------------------------
 */
int skill_castfix(struct block_list *bl, int time_duration) {
	struct map_session_data *sd = NULL;
	struct mob_data *md;
	struct status_change *sc_data;
	int castrate;
	int skill = 0, lv = 0;

	nullpo_retr(0, bl);

	if (bl->type == BL_MOB) { // Crash fix [Valaris]
		md = (struct mob_data*)bl; // nullpo_retr?
		skill = md->skillid;
		lv = md->skilllv;
//	} else if (bl->type == BL_PET) {
//		return 0;
	} else if (bl->type == BL_PC) {
		sd = (struct map_session_data*)bl; // nullpo_retr?
		skill = sd->skillid;
		lv = sd->skilllv;
	}

	if (lv <= 0) return 0;

	if (skill > MAX_SKILL_DB || skill < 0)
		return 0;

	if (time_duration == 0)
		return 0;

	if (sd) {
		if (skill_get_castnodex(sd->skillid, sd->skilllv) <= 0) {
			int scale = battle_config.castrate_dex_scale - status_get_dex(bl);
			if (scale > 0) { // not instant cast
				castrate = ((struct map_session_data *)bl)->castrate;
				time_duration = time_duration * castrate * scale / (battle_config.castrate_dex_scale * 100);
			} else
				return 0; // instant cast
		}
		// config cast time multiplier
		if (battle_config.cast_rate != 100)
			time_duration = time_duration * battle_config.cast_rate / 100;

		// calculate cast time reduced by card bonuses
		if (sd->castrate != 100)
			time_duration -= time_duration * (100 - sd->castrate) / 100;
	}

	// calculate cast time reduced by skill bonuses
	sc_data = status_get_sc_data(bl);
	if (sc_data) {
		/* ƒTƒtƒ‰ƒMƒEƒ€ */
		if (sc_data[SC_SUFFRAGIUM].timer != -1) {
			time_duration = time_duration * (100 - sc_data[SC_SUFFRAGIUM].val1 * 15) / 100;
			status_change_end(bl, SC_SUFFRAGIUM, -1);
		}
		/* ƒuƒ‰ƒM‚Ì */
		if (sc_data[SC_POEMBRAGI].timer != -1)
			time_duration = time_duration * (100 - (sc_data[SC_POEMBRAGI].val1 * 3 + sc_data[SC_POEMBRAGI].val2 + (sc_data[SC_POEMBRAGI].val3 >> 16))) / 100;
	}

	return (time_duration > 0) ? time_duration : 0;
}

/*==========================================
 * ƒfƒBƒŒƒCŒvZ
 *------------------------------------------
 */
int skill_delayfix(struct block_list *bl, int time_duration) {
	struct status_change *sc_data;

	nullpo_retr(0, bl);

	if (bl->type == BL_PC) {
		struct map_session_data *sd = (struct map_session_data*)bl;
		nullpo_retr(0, sd);

		// instant cast attack skills depend on aspd as delay [celest]
		if (time_duration == 0) {
			if (skill_db[sd->skillid].skill_type == BF_WEAPON)
				time_duration = status_get_adelay(bl) / 2;
			else
				time_duration = 300; // default delay, according to official servers
		} else if (time_duration < 0)
			time_duration = abs(time_duration) + status_get_adelay(bl) / 2; // if set to <0, the aspd delay will be added

		if (battle_config.delay_dependon_dex && /* dex‚Ì‰e‹¿‚ğŒvZ‚·‚é */
		    !skill_get_delaynodex(sd->skillid, sd->skilllv)) { // if skill casttime is allowed to be reduced by dex
			int scale = battle_config.castrate_dex_scale - status_get_dex(bl);
			if (scale < 0)
				scale = 0;
			time_duration = time_duration * scale / battle_config.castrate_dex_scale;
		}

		if (battle_config.delay_rate != 100)
			time_duration = time_duration * battle_config.delay_rate / 100;

		if (sd->delayrate != 100)
			time_duration = time_duration * sd->delayrate / 100;

		if (time_duration < battle_config.min_skill_delay_limit) // check minimum skill delay
			time_duration = battle_config.min_skill_delay_limit;
	}

	sc_data = status_get_sc_data(bl);

	/* ƒuƒ‰ƒM‚Ì */
	if (sc_data && sc_data[SC_POEMBRAGI].timer != -1)
		time_duration = time_duration * (100 - (sc_data[SC_POEMBRAGI].val1 * 3 + sc_data[SC_POEMBRAGI].val2 + (sc_data[SC_POEMBRAGI].val3&0xffff))) / 100;

	return (time_duration > 0) ? time_duration : 0;
}

/*==========================================
 * ƒXƒLƒ‹g—piIDw’èj
 *------------------------------------------
 */
int skill_use_id(struct map_session_data *sd, int target_id, int skill_num, int skill_lv) {
	int casttime = 0, delay = 0, skill, range;
	struct map_session_data* target_sd = NULL;
	int forcecast = 0;
	struct block_list *bl;
	struct status_change *sc_data;

	nullpo_retr(0, sd);

	if ((bl = map_id2bl(target_id)) == NULL) {
/*		if (battle_config.error_log)
			printf("skill target not found %d\n", target_id); */
		// if player asks for the fake mob/player (only bot and modified client can see a hiden mob/player)
		check_fake_id(sd, target_id);
		return 0;
	}
	if (sd->bl.m != bl->m || pc_isdead(sd))
		return 0;

	if (skillnotok(skill_num, sd))
		return 0;

	sc_data = sd->sc_data;

	/* ’¾–Ù‚âˆÙíi‚½‚¾‚µAƒOƒŠƒ€‚È‚Ç‚Ì”»’è‚ğ‚·‚éj */
	if (sd->opt1 > 0)
		return 0;
	if (sc_data) {
//		if (sc_data[SC_CHASEWALK].timer != -1) return 0; // If the target is under chase walk, cant use any skill, except chasewalk itself - [Aalye] - freya's forum
		if (sc_data[SC_VOLCANO].timer != -1) {
			if (skill_num == WZ_ICEWALL) return 0;
		}
		if (sc_data[SC_ROKISWEIL].timer != -1) {
			if(skill_num == BD_ADAPTATION) return 0;
		}
		if(sc_data[SC_DIVINA].timer!=-1 ||
			sc_data[SC_ROKISWEIL].timer!=-1 ||
			(sc_data[SC_AUTOCOUNTER].timer != -1 && sd->skillid != KN_AUTOCOUNTER) ||
			sc_data[SC_STEELBODY].timer != -1 ||
			sc_data[SC_BERSERK].timer != -1 ||
			(sc_data[SC_MARIONETTE].timer != -1 && sd->skillid != CG_MARIONETTE)){
			return 0;	/* ó‘ÔˆÙí‚â’¾–Ù‚È‚Ç */
		}

		if(sc_data[SC_BLADESTOP].timer != -1){
			int lv = sc_data[SC_BLADESTOP].val1;
			if(sc_data[SC_BLADESTOP].val2==1) return 0;//”’‰H‚³‚ê‚½‘¤‚È‚Ì‚Åƒ_ƒ
			if(lv==1) return 0;
			if(lv==2 && skill_num!=MO_FINGEROFFENSIVE) return 0;
			if(lv==3 && skill_num!=MO_FINGEROFFENSIVE && skill_num!=MO_INVESTIGATE) return 0;
			if(lv==4 && skill_num!=MO_FINGEROFFENSIVE && skill_num!=MO_INVESTIGATE && skill_num!=MO_CHAINCOMBO) return 0;
			if(lv==5 && skill_num!=MO_FINGEROFFENSIVE && skill_num!=MO_INVESTIGATE && skill_num!=MO_CHAINCOMBO && skill_num!=MO_EXTREMITYFIST) return 0;
		}

		if (sc_data[SC_BASILICA].timer != -1) { // Disallow all other skills in Basilica [celest]
			struct skill_unit_group *sg = (struct skill_unit_group *)sc_data[SC_BASILICA].val4;
			// if caster is the owner of basilica
			if (sg && sg->src_id == sd->bl.id &&
			    skill_num == HP_BASILICA)
				; // do nothing
			// otherwise...
			else
				return 0;
		}
		/* ‰‰‘t/ƒ_ƒ“ƒX’† */
		if(sc_data[SC_DANCING].timer!=-1 ){
//			if(battle_config.pc_skill_log)
//				printf("dancing! %d\n",skill_num);
			if( sc_data[SC_DANCING].val4 && skill_num!=BD_ADAPTATION ) //‡‘t’†‚ÍƒAƒhƒŠƒuˆÈŠO•s‰Â
				return 0;
			if(skill_num!=BD_ADAPTATION && skill_num!=BA_MUSICALSTRIKE && skill_num!=DC_THROWARROW){
				return 0;
			}
		}
	}

	if (pc_iscloaking(sd) && skill_num == TF_HIDING)
		return 0;
	if (sd->status.option & 2 && skill_num != TF_HIDING && skill_num != AS_GRIMTOOTH && skill_num != RG_BACKSTAP && skill_num != RG_RAID)
		return 0;
	if (pc_ischasewalk(sd) && skill_num != ST_CHASEWALK)
		return 0;

	if (skill_get_inf2(skill_num) & 0x200 && sd->bl.id == target_id)
		return 0;
	//’¼‘O‚ÌƒXƒLƒ‹‚ª‰½‚©Šo‚¦‚é•K—v‚Ì‚ ‚éƒXƒLƒ‹
	switch(skill_num) {
	case SA_CASTCANCEL:
		if (sd->skillid != skill_num) { //ƒLƒƒƒXƒgƒLƒƒƒ“ƒZƒ‹©‘Ì‚ÍŠo‚¦‚È‚¢
			sd->skillid_old = sd->skillid;
			sd->skilllv_old = sd->skilllv;
			break;
		}
	case BD_ENCORE: /* ƒAƒ“ƒR[ƒ‹ */
		if (!sd->skillid_dance) { //‘O‰ñg—p‚µ‚½—x‚è‚ª‚È‚¢‚Æ‚¾‚ß
			clif_skill_fail(sd, skill_num, 0, 0);
			return 0;
		}else{
			sd->skillid_old = skill_num;
		}
		break;
	case GD_BATTLEORDER:
	case GD_REGENERATION:
	case GD_RESTORE:
	case GD_EMERGENCYCALL:
	  {
		struct guild *g;
		if (!sd->status.guild_id)
			return 0;
		if ((g = guild_search(sd->status.guild_id)) == NULL)
			return 0;
		if (strcmp(sd->status.name, g->master))
			return 0;
		skill_lv = guild_checkskill(g, skill_num);
		if (skill_lv <= 0)
			return 0;
	  }
		break;
	}

	sd->skillid = skill_num;
	sd->skilllv = skill_lv;

	switch(skill_num){ //–‘O‚ÉƒŒƒxƒ‹‚ª•Ï‚í‚Á‚½‚è‚·‚éƒXƒLƒ‹
	case BD_LULLABY:				/* qç‰Ì */
	case BD_RICHMANKIM:				/* ƒjƒˆƒ‹ƒh‚Ì‰ƒ */
	case BD_ETERNALCHAOS:			/* ‰i‰“‚Ì¬“× */
	case BD_DRUMBATTLEFIELD:		/* í‘¾ŒÛ‚Ì‹¿‚« */
	case BD_RINGNIBELUNGEN:			/* ƒj[ƒxƒ‹ƒ“ƒO‚Ìw—Ö */
	case BD_ROKISWEIL:				/* ƒƒL‚Ì‹©‚Ñ */
	case BD_INTOABYSS:				/* [•£‚Ì’†‚É */
	case BD_SIEGFRIED:				/* •s€g‚ÌƒW[ƒNƒtƒŠ[ƒh */
	case BD_RAGNAROK:				/* _X‚Ì‰©¨ */
	case CG_MOONLIT:				/* Œ–¾‚è‚Ìò‚É—‚¿‚é‰Ô‚Ñ‚ç */
	  {
//		int range = 1; // always 1?
		int c = 0;
//		map_foreachinarea(skill_check_condition_char_sub, sd->bl.m,
//		                  sd->bl.x - range, sd->bl.y - range,
//		                  sd->bl.x + range, sd->bl.y + range, BL_PC, &sd->bl, &c);
		map_foreachinarea(skill_check_condition_char_sub, sd->bl.m,
		                  sd->bl.x - 1, sd->bl.y - 1,
		                  sd->bl.x + 1, sd->bl.y + 1, BL_PC, &sd->bl, &c);
		if (c < 1) {
			clif_skill_fail(sd, skill_num, 0, 0);
			return 0;
		} else if (c == 99) { //‘Š•û•s—vİ’è‚¾‚Á‚½
			;
		} else {
			sd->skilllv = (c + skill_lv) / 2;
		}
	  }
		break;
	}

	if (!skill_check_condition(sd, 0)) return 0;

	{
		int check_range_flag = 0;

		/* Ë’ö‚ÆáŠQ•¨ƒ`ƒFƒbƒN */
		range = skill_get_range(skill_num, skill_lv);
		if (range < 0)
			range = status_get_range(&sd->bl) - (range + 1);
		// be lenient if the skill was cast before we have moved to the correct position [Celest]
		if (sd->walktimer != -1)
			range += battle_config.skill_range_leniency;
		else check_range_flag = 1;
		if(!battle_check_range(&sd->bl,bl,range)) {
			if (check_range_flag && battle_check_range(&sd->bl,bl,range + battle_config.skill_range_leniency)) {
				int dir, mask[8][2] = {{0,1},{-1,1},{-1,0},{-1,-1},{0,-1},{1,-1},{1,0},{1,1}};
				dir = map_calc_dir(&sd->bl,bl->x,bl->y);
				pc_walktoxy (sd, sd->bl.x + mask[dir][0] * battle_config.skill_range_leniency,
					sd->bl.y + mask[dir][1] * battle_config.skill_range_leniency);
			} else
				return 0;
		}
	}

	if (bl->type == BL_PC) {
		target_sd = (struct map_session_data*)bl;
		if (target_sd && skill_num == ALL_RESURRECTION && !pc_isdead(target_sd))
			return 0;
	}
	if ((skill_num != MO_CHAINCOMBO &&
	     skill_num != MO_COMBOFINISH &&
	     skill_num != MO_EXTREMITYFIST &&
	     skill_num != CH_TIGERFIST &&
	     skill_num != CH_CHAINCRUSH) ||
	    (skill_num == CH_CHAINCRUSH && sd->state.skill_flag) ||
	    (skill_num == MO_EXTREMITYFIST && sd->state.skill_flag))
		pc_stopattack(sd);

	casttime=skill_castfix(&sd->bl, skill_get_cast( skill_num,skill_lv) );
	if(skill_num != SA_MAGICROD)
		delay=skill_delayfix(&sd->bl, skill_get_delay( skill_num,skill_lv) );
	//sd->state.skillcastcancel = skill_db[skill_num].castcancel;
	sd->state.skillcastcancel = skill_get_castcancel(skill_num);

	switch(skill_num){	/* ‰½‚©“Áê‚Èˆ—‚ª•K—v */
//	case AL_HEAL:	/* ƒq[ƒ‹ */
//		if(battle_check_undead(status_get_race(bl),status_get_elem_type(bl)))
//			forcecast=1;	/* ƒq[ƒ‹ƒAƒ^ƒbƒN‚È‚ç‰r¥ƒGƒtƒFƒNƒg—L‚è */
//		break;
	case ALL_RESURRECTION:	/* ƒŠƒUƒŒƒNƒVƒ‡ƒ“ */
		if(bl->type != BL_PC && battle_check_undead(status_get_race(bl),status_get_elem_type(bl))){	/* “G‚ªƒAƒ“ƒfƒbƒh‚È‚ç */
			forcecast=1;	/* ƒ^[ƒ“ƒAƒ“ƒfƒbƒg‚Æ“¯‚¶‰r¥ŠÔ */
			casttime=skill_castfix(&sd->bl, skill_get_cast(PR_TURNUNDEAD,skill_lv) );
		}
		break;
	case MO_FINGEROFFENSIVE:	/* w’e */
		casttime += casttime * ((skill_lv > sd->spiritball)? sd->spiritball:skill_lv);
		break;
	case MO_CHAINCOMBO:		/*˜A‘Å¶*/
		target_id = sd->attacktarget;
		if( sc_data && sc_data[SC_BLADESTOP].timer!=-1 ){
			struct block_list *tbl;
			if((tbl=(struct block_list *)sc_data[SC_BLADESTOP].val4) == NULL) //ƒ^[ƒQƒbƒg‚ª‚¢‚È‚¢H
				return 0;
			target_id = tbl->id;
		}
		break;
	case MO_COMBOFINISH:		/*–Ò—´Œ*/
//	case CH_TIGERFIST:		/* •šŒÕŒ */
	case CH_CHAINCRUSH:		/* ˜A’Œ•öŒ‚ */
		target_id = sd->attacktarget;
		break;

	case CH_TIGERFIST:		/* •šŒÕŒ */
/* Tiger Knuckle Fist can be used everywhere!!! this skill can be used w/out combo on players everywhere! Fix from [akrus] (freya's bug report)
		if (sc_data && sc_data[SC_COMBO].timer != -1 && sc_data[SC_COMBO].val1 == MO_COMBOFINISH) */
			target_id = sd->attacktarget;
		break;

// -- moonsoul	(altered to allow proper usage of extremity from new champion combos)
//
	case MO_EXTREMITYFIST:	/*ˆ¢C—…”e–PŒ*/
		if (sc_data && sc_data[SC_COMBO].timer != -1 && (sc_data[SC_COMBO].val1 == MO_COMBOFINISH || sc_data[SC_COMBO].val1 == CH_CHAINCRUSH)) {
			casttime = 0;
			target_id = sd->attacktarget;
		}
		forcecast=1;
		break;
	case SA_MAGICROD:
	case SA_SPELLBREAKER:
		forcecast=1;
		break;
	case WE_MALE:
	case WE_FEMALE:
		{
			struct map_session_data *p_sd;
			if ((p_sd = pc_get_partner(sd)) == NULL) // it's possible to get null if we're not married --> no use NULLPO
				return 0;
			if (skill_num == WE_MALE && sd->status.hp <= ((15 * sd->status.max_hp) / 100)) // Requires more than 15% of Max HP for WE_MALE
				return 0;
			else if (skill_num == WE_FEMALE && sd->status.sp <= ((15 * sd->status.max_sp) / 100)) // Requires more than 15% of Max SP for WE_FEMALE
				return 0;
			target_id = p_sd->bl.id;
			//range‚ğ‚à‚¤1‰ñŒŸ¸
			range = skill_get_range(skill_num,skill_lv);
			if (range < 0)
				range = status_get_range(&sd->bl) - (range + 1);
			if (!battle_check_range(&sd->bl, &p_sd->bl, range))
				return 0;
		}
		break;

	case PF_MEMORIZE: /* ƒƒ‚ƒ‰ƒCƒY */
//		casttime = 12000; // removed (found on freya's bug report, posted by [BLB])
		break;
	case HW_MAGICPOWER:
//		casttime = 700; // removed [Yor]
		break;
	case HP_BASILICA:		/* ƒoƒWƒŠƒJ */
		if (skill_check_unit_range(sd->bl.m, sd->bl.x, sd->bl.y, sd->skillid, sd->skilllv)) {
			clif_skill_fail(sd, sd->skillid, 0, 0);
			return 0;
		}
		if (skill_check_unit_range2(sd->bl.m, sd->bl.x, sd->bl.y, sd->skillid, sd->skilllv)) {
			clif_skill_fail(sd, sd->skillid, 0, 0);
			return 0;
		}
	  {
		// cancel Basilica if already in effect
		struct status_change *sc_data = status_get_sc_data(&sd->bl);
		if (sc_data && sc_data[SC_BASILICA].timer != -1) {
			struct skill_unit_group *sg = (struct skill_unit_group *)sc_data[SC_BASILICA].val4;
			if (sg && sg->src_id == sd->bl.id) {
				status_change_end(&sd->bl, SC_BASILICA, -1);
				skill_delunitgroup(sg);
				return 0;
			}
		}
	  }
		break;
	case GD_BATTLEORDER:
	case GD_REGENERATION:
	case GD_RESTORE:
	case GD_EMERGENCYCALL:
		casttime = 1000; // temporary [Celest]
		break;
	}

	//ƒƒ‚ƒ‰ƒCƒYó‘Ô‚È‚çƒLƒƒƒXƒgƒ^ƒCƒ€‚ª1/3
	if(sc_data && sc_data[SC_MEMORIZE].timer != -1 && casttime > 0){
		casttime = casttime/2;
		if((--sc_data[SC_MEMORIZE].val2)<=0)
			status_change_end(&sd->bl, SC_MEMORIZE, -1);
	}

	if(battle_config.pc_skill_log)
		printf("PC %d skill use target_id=%d skill=%d lv=%d cast=%d\n",sd->bl.id,target_id,skill_num,skill_lv,casttime);

//	if(sd->skillitem == skill_num)
//		casttime = delay = 0;

	if( casttime>0 || forcecast ){ /* ‰r¥‚ª•K—v */
		struct mob_data *md;
		clif_skillcasting( &sd->bl, sd->bl.id, target_id, 0,0, skill_num,casttime);

		/* ‰r¥”½‰ƒ‚ƒ“ƒXƒ^[ */
		if (bl->type==BL_MOB && (md=(struct mob_data *)bl) && mob_db[md->class].mode&0x10 &&
		    md->state.state != MS_ATTACK && sd->invincible_timer == -1){
				md->target_id=sd->bl.id;
				md->state.targettype = ATTACKABLE;
				md->min_chase=13;
		}
	}

	if( casttime<=0 )	/* ‰r¥‚Ì–³‚¢‚à‚Ì‚ÍƒLƒƒƒ“ƒZƒ‹‚³‚ê‚È‚¢ */
		sd->state.skillcastcancel=0;

	sd->skilltarget	= target_id;
/*	sd->cast_target_bl	= bl; */
	sd->skillx       = 0;
	sd->skilly       = 0;
	sd->canact_tick  = gettick_cache + casttime + delay;
	sd->canmove_tick = gettick_cache;
	if (!(battle_config.pc_cloak_check_type & 2) && sc_data && sc_data[SC_CLOAKING].timer != -1 && sd->skillid != AS_CLOAKING)
		status_change_end(&sd->bl,SC_CLOAKING,-1);

	if (sd->skilltimer != -1 && skill_num != SA_CASTCANCEL) { // SA_CASTCANCEL is cast immediatly // normally, we never entering in this test
		int inf;
		if ((inf = skill_get_inf(sd->skillid)) == 2 || inf == 32)
			delete_timer(sd->skilltimer, skill_castend_pos);
		else
			delete_timer(sd->skilltimer, skill_castend_id);
		sd->skilltimer = -1;
	}
	if (casttime > 0) { // cast with castime
		sd->skilltimer = add_timer(gettick_cache + casttime, skill_castend_id, sd->bl.id, 0);
		if ((skill = pc_checkskill(sd, SA_FREECAST)) > 0) {
			sd->prev_speed = sd->speed;
			sd->speed = sd->speed * (175 - skill * 5) / 100;
			clif_updatestatus(sd, SP_SPEED);
		} else
			pc_stop_walking(sd, 0);
	} else // cast immediatly
		skill_castend_id(sd->skilltimer, gettick_cache, sd->bl.id, 0);

	return 0;
}

/*==========================================
 * ƒXƒLƒ‹g—piêŠw’èj
 *------------------------------------------
 */
int skill_use_pos(struct map_session_data *sd,
	int skill_x, int skill_y, int skill_num, int skill_lv)
{
	struct block_list bl;
	struct status_change *sc_data;
	int casttime = 0, delay = 0, skill, range;

	nullpo_retr(0, sd);

	if (pc_isdead(sd))
		return 0;

	if (skillnotok(skill_num, sd))
		return 0;

	if (skill_num == WZ_ICEWALL && map[sd->bl.m].flag.noicewall && !map[sd->bl.m].flag.pvp) { // noicewall flag [Valaris]
		clif_skill_fail(sd, sd->skillid, 0, 0);
		return 0;
	}

	sc_data = sd->sc_data;

	if (sd->opt1 > 0)
		return 0;
	if (sc_data) {
		if (sc_data[SC_DIVINA].timer!=-1 ||
		    sc_data[SC_ROKISWEIL].timer!=-1 ||
		    sc_data[SC_AUTOCOUNTER].timer != -1 ||
		    sc_data[SC_STEELBODY].timer != -1 ||
		    sc_data[SC_DANCING].timer!=-1 ||
		    sc_data[SC_BERSERK].timer != -1 ||
		    sc_data[SC_MARIONETTE].timer != -1)
			return 0; /* ó‘ÔˆÙí‚â’¾–Ù‚È‚Ç */

		if (sc_data[SC_BASILICA].timer != -1) {
			struct skill_unit_group *sg = (struct skill_unit_group *)sc_data[SC_BASILICA].val4;
			// if caster is the owner of basilica
			if (sg && sg->src_id == sd->bl.id &&
				skill_num == HP_BASILICA)
				;	// do nothing
			// otherwise...
			else
				return 0;
		}
	}

	if (sd->status.option & 2)
		return 0;

	sd->skillid = skill_num;
	sd->skilllv = skill_lv;
	if (skill_lv <= 0) return 0;
	sd->skillx = skill_x;
	sd->skilly = skill_y;
	if (!skill_check_condition(sd,0)) return 0;

	/* Ë’ö‚ÆáŠQ•¨ƒ`ƒFƒbƒN */
	bl.type = BL_NUL;
	bl.m = sd->bl.m;
	bl.x = skill_x;
	bl.y = skill_y;

  {
	int check_range_flag = 0;
	/* Ë’ö‚ÆáŠQ•¨ƒ`ƒFƒbƒN */
	range = skill_get_range(skill_num, skill_lv);
	if (range < 0)
		range = status_get_range(&sd->bl) - (range + 1);
	// be lenient if the skill was cast before we have moved to the correct position [Celest]
	if (sd->walktimer != -1)
		range++;
	else
		check_range_flag = 1;
	if (!battle_check_range(&sd->bl, &bl,range)) {
		if (check_range_flag && battle_check_range(&sd->bl, &bl, range + 1)) {
			int mask[8][2] = {{0,1}, {-1,1}, {-1,0}, {-1,-1}, {0,-1}, {1,-1}, {1,0}, {1,1}};
			int dir = map_calc_dir(&sd->bl, bl.x, bl.y);
			pc_walktoxy(sd, sd->bl.x + mask[dir][0], sd->bl.y + mask[dir][1]);
		} else
			return 0;
	}
  }

	pc_stopattack(sd);

	casttime = skill_castfix(&sd->bl, skill_get_cast(skill_num,skill_lv));
	delay = skill_delayfix(&sd->bl, skill_get_delay(skill_num,skill_lv));
	sd->state.skillcastcancel = skill_db[skill_num].castcancel;

	if (battle_config.pc_skill_log)
		printf("PC %d skill use target_pos=(%d,%d) skill=%d lv=%d cast=%d\n", sd->bl.id, skill_x, skill_y, skill_num, skill_lv, casttime);

//	if(sd->skillitem == skill_num)
//		casttime = delay = 0;
	//ƒƒ‚ƒ‰ƒCƒYó‘Ô‚È‚çƒLƒƒƒXƒgƒ^ƒCƒ€‚ª1/3
	if (sc_data && sc_data[SC_MEMORIZE].timer != -1 && casttime > 0){
		casttime = casttime / 2; // Memorize is supposed to reduce the cast time of the next 5 spells by half (thanks to [Mikey] from freya's bug report)
		if ((--sc_data[SC_MEMORIZE].val2) <= 0)
			status_change_end(&sd->bl, SC_MEMORIZE, -1);
	}

	if (casttime > 0) /* ‰r¥‚ª•K—v */
		clif_skillcasting(&sd->bl, sd->bl.id, 0, skill_x, skill_y, skill_num, casttime);

	if (casttime <= 0) /* ‰r¥‚Ì–³‚¢‚à‚Ì‚ÍƒLƒƒƒ“ƒZƒ‹‚³‚ê‚È‚¢ */
		sd->state.skillcastcancel = 0;

	sd->skilltarget = 0;
/*	sd->cast_target_bl = NULL; */
	sd->canact_tick = gettick_cache + casttime + delay;
	sd->canmove_tick = gettick_cache;
	if (!(battle_config.pc_cloak_check_type&2) && sc_data && sc_data[SC_CLOAKING].timer != -1)
		status_change_end(&sd->bl, SC_CLOAKING, -1);

	if (sd->skilltimer != -1) { // normally, we never entering in this test
		int inf;
		if ((inf = skill_get_inf(sd->skillid)) == 2 || inf == 32)
			delete_timer(sd->skilltimer, skill_castend_pos);
		else
			delete_timer(sd->skilltimer, skill_castend_id);
		sd->skilltimer = -1;
	}
	if (casttime > 0) { // cast with castime
		sd->skilltimer = add_timer(gettick_cache + casttime, skill_castend_pos, sd->bl.id, 0);
		if ((skill = pc_checkskill(sd, SA_FREECAST)) > 0) {
			sd->prev_speed = sd->speed;
			sd->speed = sd->speed*(175 - skill * 5) / 100;
			clif_updatestatus(sd, SP_SPEED);
		} else
			pc_stop_walking(sd, 0);
	} else // cast immediatly
		skill_castend_pos(sd->skilltimer, gettick_cache, sd->bl.id, 0);

	//ƒ}ƒWƒbƒNƒpƒ[‚ÌŒø‰ÊI—¹
	if (sc_data && sc_data[SC_MAGICPOWER].timer != -1 && skill_num != HW_MAGICPOWER)
		status_change_end(&sd->bl,SC_MAGICPOWER,-1);

	return 0;
}

/*==========================================
 * ƒXƒLƒ‹‰r¥ƒLƒƒƒ“ƒZƒ‹
 *------------------------------------------
 */
int skill_castcancel(struct block_list *bl, int type)
{
	int inf;
	int ret = 0;

	nullpo_retr(0, bl);

	if (bl->type == BL_PC) {
		struct map_session_data *sd = (struct map_session_data *)bl;
		nullpo_retr(0, sd);
		sd->canact_tick = gettick_cache;
		sd->canmove_tick = gettick_cache;
		if (sd->skilltimer != -1) {
			if (pc_checkskill(sd, SA_FREECAST) > 0) {
				sd->speed = sd->prev_speed;
				clif_updatestatus(sd, SP_SPEED);
			}
			if (!type) {
				if ((inf = skill_get_inf(sd->skillid)) == 2 || inf == 32) {
					ret = delete_timer(sd->skilltimer, skill_castend_pos);
					if (ret < 0)
						printf("delete timer error (skill_castend_pos): skillid : %d\n", sd->skillid);
				} else {
					ret = delete_timer(sd->skilltimer, skill_castend_id);
					if (ret < 0)
						printf("delete timer error (skill_castend_id): skillid : %d\n", sd->skillid);
				}
			} else {
				if ((inf = skill_get_inf(sd->skillid_old)) == 2 || inf == 32) {
					ret = delete_timer(sd->skilltimer, skill_castend_pos);
					if (ret < 0)
						printf("delete timer error (skill_castend_pos): skillid : %d\n", sd->skillid_old);
				} else {
					ret = delete_timer(sd->skilltimer, skill_castend_id);
					if (ret < 0)
						printf("delete timer error (skill_castend_id): skillid : %d\n", sd->skillid_old);
				}
			}
			sd->skilltimer = -1;
			clif_skillcastcancel(bl);
		}
		return 0;
	} else if (bl->type == BL_MOB) {
		struct mob_data *md = (struct mob_data *)bl;
		nullpo_retr(0, md);
		if (md->skilltimer != -1) {
			if ((inf = skill_get_inf(md->skillid)) == 2 || inf == 32) {
				ret = delete_timer(md->skilltimer, mobskill_castend_pos);
				if (ret < 0)
					printf("delete timer error (mobskill_castend_pos): skillid : %d\n", md->skillid);
			} else {
				ret = delete_timer(md->skilltimer, mobskill_castend_id);
				if (ret < 0)
					printf("delete timer error (mobskill_castend_id): skillid : %d\n", md->skillid);
			}
			md->skilltimer = -1;
			clif_skillcastcancel(bl);
		}
		return 0;
	}

	return 1;
}

/*=========================================
 * ƒuƒ‰ƒ“ƒfƒBƒbƒVƒ…ƒXƒsƒA ‰Šú”ÍˆÍŒˆ’è
 *----------------------------------------
 */
void skill_brandishspear_first(struct square *tc,int dir,int x,int y){

	nullpo_retv(tc);

	if(dir == 0){
		tc->val1[0]=x-2;
		tc->val1[1]=x-1;
		tc->val1[2]=x;
		tc->val1[3]=x+1;
		tc->val1[4]=x+2;
		tc->val2[0]=
		tc->val2[1]=
		tc->val2[2]=
		tc->val2[3]=
		tc->val2[4]=y-1;
	}
	else if(dir==2){
		tc->val1[0]=
		tc->val1[1]=
		tc->val1[2]=
		tc->val1[3]=
		tc->val1[4]=x+1;
		tc->val2[0]=y+2;
		tc->val2[1]=y+1;
		tc->val2[2]=y;
		tc->val2[3]=y-1;
		tc->val2[4]=y-2;
	}
	else if(dir==4){
		tc->val1[0]=x-2;
		tc->val1[1]=x-1;
		tc->val1[2]=x;
		tc->val1[3]=x+1;
		tc->val1[4]=x+2;
		tc->val2[0]=
		tc->val2[1]=
		tc->val2[2]=
		tc->val2[3]=
		tc->val2[4]=y+1;
	}
	else if(dir==6){
		tc->val1[0]=
		tc->val1[1]=
		tc->val1[2]=
		tc->val1[3]=
		tc->val1[4]=x-1;
		tc->val2[0]=y+2;
		tc->val2[1]=y+1;
		tc->val2[2]=y;
		tc->val2[3]=y-1;
		tc->val2[4]=y-2;
	}
	else if(dir==1){
		tc->val1[0]=x-1;
		tc->val1[1]=x;
		tc->val1[2]=x+1;
		tc->val1[3]=x+2;
		tc->val1[4]=x+3;
		tc->val2[0]=y-4;
		tc->val2[1]=y-3;
		tc->val2[2]=y-1;
		tc->val2[3]=y;
		tc->val2[4]=y+1;
	}
	else if(dir==3){
		tc->val1[0]=x+3;
		tc->val1[1]=x+2;
		tc->val1[2]=x+1;
		tc->val1[3]=x;
		tc->val1[4]=x-1;
		tc->val2[0]=y-1;
		tc->val2[1]=y;
		tc->val2[2]=y+1;
		tc->val2[3]=y+2;
		tc->val2[4]=y+3;
	}
	else if(dir==5){
		tc->val1[0]=x+1;
		tc->val1[1]=x;
		tc->val1[2]=x-1;
		tc->val1[3]=x-2;
		tc->val1[4]=x-3;
		tc->val2[0]=y+3;
		tc->val2[1]=y+2;
		tc->val2[2]=y+1;
		tc->val2[3]=y;
		tc->val2[4]=y-1;
	}
	else if(dir==7){
		tc->val1[0]=x-3;
		tc->val1[1]=x-2;
		tc->val1[2]=x-1;
		tc->val1[3]=x;
		tc->val1[4]=x+1;
		tc->val2[1]=y;
		tc->val2[0]=y+1;
		tc->val2[2]=y-1;
		tc->val2[3]=y-2;
		tc->val2[4]=y-3;
	}

	return;
}

/*=========================================
 * ƒuƒ‰ƒ“ƒfƒBƒbƒVƒ…ƒXƒsƒA •ûŒü”»’è ”ÍˆÍŠg’£
 *-----------------------------------------
 */
void skill_brandishspear_dir(struct square *tc,int dir,int are){

	int c;

	nullpo_retv(tc);

	for(c=0;c<5;c++){
		if(dir==0){
			tc->val2[c]+=are;
		}else if(dir==1){
			tc->val1[c]-=are; tc->val2[c]+=are;
		}else if(dir==2){
			tc->val1[c]-=are;
		}else if(dir==3){
			tc->val1[c]-=are; tc->val2[c]-=are;
		}else if(dir==4){
			tc->val2[c]-=are;
		}else if(dir==5){
			tc->val1[c]+=are; tc->val2[c]-=are;
		}else if(dir==6){
			tc->val1[c]+=are;
		}else if(dir==7){
			tc->val1[c]+=are; tc->val2[c]+=are;
		}
	}
}

/*==========================================
 * ƒfƒBƒ{[ƒVƒ‡ƒ“ —LŒøŠm”F
 *------------------------------------------
 */
void skill_devotion(struct map_session_data *md,int target)
{
	// ‘Šm”F
	int n;

	nullpo_retv(md);

	for(n=0;n<5;n++){
		if(md->dev.val1[n]){
			struct map_session_data *sd = map_id2sd(md->dev.val1[n]);
			// ‘Šè‚ªŒ©‚Â‚©‚ç‚È‚¢ // ‘Šè‚ğƒfƒBƒ{‚µ‚Ä‚é‚Ì‚ª©•ª‚¶‚á‚È‚¢ // ‹——£‚ª—£‚ê‚Ä‚é
			if( sd == NULL || (sd->sc_data && (md->bl.id != sd->sc_data[SC_DEVOTION].val1)) || skill_devotion3(&md->bl,md->dev.val1[n])){
				skill_devotion_end(md,sd,n);
			}
		}
	}
}

void skill_devotion2(struct block_list *bl, int crusader)
{
	// ”íƒfƒBƒ{[ƒVƒ‡ƒ“‚ª•à‚¢‚½‚Ì‹——£ƒ`ƒFƒbƒN
	struct map_session_data *sd;

	nullpo_retv(bl);

	sd = map_id2sd(crusader);
	if (sd) skill_devotion3(&sd->bl, bl->id);
}

int skill_devotion3(struct block_list *bl, int target)
{
	// ƒNƒ‹ƒZ‚ª•à‚¢‚½‚Ì‹——£ƒ`ƒFƒbƒN
	struct map_session_data *md;
	struct map_session_data *sd;
	int n,r=0;

	nullpo_retr(1, bl);

	md = (struct map_session_data *)bl;

	if ((sd = map_id2sd(target)) == NULL)
		return 1;
	else
		r = distance(bl->x, bl->y, sd->bl.x, sd->bl.y);

	if (pc_checkskill(md, CR_DEVOTION) + 6 < r) { // ‹–—e”ÍˆÍ‚ğ’´‚¦‚Ä‚½
		for(n = 0; n < 5; n++)
			if (md->dev.val1[n] == target)
				md->dev.val2[n] = 0; // —£‚ê‚½‚ÍA…‚ğØ‚é‚¾‚¯
		clif_devotion(md, sd->bl.id);
		return 1;
	}

	return 0;
}

void skill_devotion_end(struct map_session_data *md,struct map_session_data *sd,int target)
{
	// ƒNƒ‹ƒZ‚Æ”íƒfƒBƒ{ƒLƒƒƒ‰‚ÌƒŠƒZƒbƒg
	nullpo_retv(md);
	nullpo_retv(sd);

	md->dev.val1[target]=md->dev.val2[target]=0;
	if(sd && sd->sc_data){
	//	status_change_end(sd->bl,SC_DEVOTION,-1);
		sd->sc_data[SC_DEVOTION].val1=0;
		sd->sc_data[SC_DEVOTION].val2=0;
		clif_status_change(&sd->bl,SC_DEVOTION,0);
		clif_devotion(md,sd->bl.id);
	}
}

/*==========================================
 * ƒI[ƒgƒXƒyƒ‹
 *------------------------------------------
 */
void skill_autospell(struct map_session_data *sd, int skillid) {
	int skilllv;
	int maxlv = 1, lv;

//	nullpo_retv(sd); // checked before to call function

	skilllv = pc_checkskill(sd, SA_AUTOSPELL);
	if (skilllv <= 0) return;

	if (skillid == MG_NAPALMBEAT)
		maxlv = 3;
	else if (skillid == MG_COLDBOLT || skillid == MG_FIREBOLT || skillid == MG_LIGHTNINGBOLT) {
		if (skilllv == 2) maxlv = 1;
		else if (skilllv == 3) maxlv = 2;
		else if (skilllv >= 4) maxlv = 3;
	} else if (skillid == MG_SOULSTRIKE){
		if (skilllv == 5) maxlv = 1;
		else if (skilllv == 6) maxlv = 2;
		else if (skilllv >= 7) maxlv = 3;
	} else if (skillid == MG_FIREBALL) {
		if (skilllv == 8) maxlv = 1;
		else if (skilllv >= 9) maxlv = 2;
	} else if (skillid == MG_FROSTDIVER)
		maxlv = 1;
	else
		return;

	if (maxlv > (lv = pc_checkskill(sd, skillid)))
		maxlv = lv;

	status_change_start(&sd->bl, SC_AUTOSPELL, skilllv, skillid, maxlv, 0, // val1:ƒXƒLƒ‹ID val2:g—pÅ‘åLv
		skill_get_time(SA_AUTOSPELL, skilllv), 0); // ‚É‚µ‚Ä‚İ‚½‚¯‚Çbscript‚ª‘‚«ˆÕ‚¢EEEH

	return;
}

/*==========================================
 * ƒMƒƒƒ“ƒOƒXƒ^[ƒpƒ‰ƒ_ƒCƒX”»’èˆ—(foreachinarea)
 *------------------------------------------
 */

static int skill_gangster_count(struct block_list *bl, va_list ap)
{
	int *c;
	struct map_session_data *sd;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);

	sd = (struct map_session_data*)bl;
	c = va_arg(ap, int *);

	if (sd && c && pc_issit(sd) && pc_checkskill(sd, RG_GANGSTER) > 0)
		(*c)++;

	return 0;
}

static int skill_gangster_in(struct block_list *bl, va_list ap)
{
	struct map_session_data *sd;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);

	sd = (struct map_session_data*)bl;
	if (sd && pc_issit(sd) && pc_checkskill(sd, RG_GANGSTER) > 0)
		sd->state.gangsterparadise = 1;

	return 0;
}

static int skill_gangster_out(struct block_list *bl,va_list ap)
{
	struct map_session_data *sd;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);

	sd=(struct map_session_data*)bl;
	if(sd && sd->state.gangsterparadise)
		sd->state.gangsterparadise=0;

	return 0;
}

int skill_gangsterparadise(struct map_session_data *sd, int type)
{
	int range = 1;
	int c = 0;

	nullpo_retr(0, sd);

	if (pc_checkskill(sd, RG_GANGSTER) <= 0)
		return 0;

	if (type == 1) { /* À‚Á‚½‚Ìˆ— */
		map_foreachinarea(skill_gangster_count, sd->bl.m,
		                  sd->bl.x - range, sd->bl.y - range,
		                  sd->bl.x + range, sd->bl.y + range, BL_PC, &c);
		if (c > 1) { /*ƒMƒƒƒ“ƒOƒXƒ^[¬Œ÷‚µ‚½‚ç©•ª‚É‚àƒMƒƒƒ“ƒOƒXƒ^[‘®«•t—^*/
			map_foreachinarea(skill_gangster_in, sd->bl.m,
			                  sd->bl.x - range, sd->bl.y - range,
			                  sd->bl.x + range, sd->bl.y + range, BL_PC);
			sd->state.gangsterparadise = 1;
		}
		return 0;
	}
	else if (type == 0) {/* —§‚¿ã‚ª‚Á‚½‚Æ‚«‚Ìˆ— */
		map_foreachinarea(skill_gangster_count, sd->bl.m,
		                  sd->bl.x - range, sd->bl.y - range,
		                  sd->bl.x + range, sd->bl.y + range, BL_PC, &c);
		if (c < 2)
			map_foreachinarea(skill_gangster_out, sd->bl.m,
			                  sd->bl.x - range, sd->bl.y - range,
			                  sd->bl.x + range, sd->bl.y + range, BL_PC);
		sd->state.gangsterparadise = 0;
		return 0;
	}

	return 0;
}

/*==========================================
 * Š¦‚¢ƒWƒ‡[ƒNEƒXƒNƒŠ[ƒ€”»’èˆ—(foreachinarea)
 *------------------------------------------
 */
int skill_frostjoke_scream(struct block_list *bl, va_list ap)
{
	struct block_list *src;
	int skillnum, skilllv;
	unsigned int tick;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);
	nullpo_retr(0, src = va_arg(ap,struct block_list*));

	skillnum = va_arg(ap,int);
	skilllv = va_arg(ap,int);
	if (skilllv <= 0) return 0;
	tick = va_arg(ap, unsigned int);

	if (src == bl || bl->prev == NULL || status_isdead(bl))
		return 0;

	if (battle_check_target(src, bl, BCT_ENEMY) > 0)
		skill_additional_effect(src, bl, skillnum, skilllv, BF_MISC, tick);
	else if (battle_check_target(src, bl, BCT_PARTY) > 0) {
		if (rand() % 100 < 10)//PTƒƒ“ƒo‚É‚à’áŠm—¦‚Å‚©‚©‚é(‚Æ‚è‚ ‚¦‚¸10%)
			skill_additional_effect(src, bl, skillnum, skilllv, BF_MISC, tick);
	}

	return 0;
}

/*==========================================
 * Moonlit creates a 'safe zone' [celest]
 *------------------------------------------
 */
static int skill_moonlit_count(struct block_list *bl, va_list ap)
{
	int *c, id;
	struct map_session_data *sd;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);
	nullpo_retr(0, (sd = (struct map_session_data *)bl));

	id=va_arg(ap, int);
	c=va_arg(ap, int *);

	if (sd->bl.id != id && sd->sc_count && sd->sc_data[SC_MOONLIT].timer != -1 && c)
		(*c)++;

	return 0;
}

int skill_check_moonlit (struct block_list *bl, int dx, int dy)
{
	int c = 0;

	nullpo_retr(0, bl);

	map_foreachinarea(skill_moonlit_count, bl->m, dx-1, dy-1, dx+1, dy+1, BL_PC, bl->id, &c);

	return (c > 0);
}

/*==========================================
 *ƒAƒuƒ‰ƒJƒ_ƒuƒ‰‚Ìg—pƒXƒLƒ‹Œˆ’è(Œˆ’èƒXƒLƒ‹‚ªƒ_ƒ‚È‚ç0‚ğ•Ô‚·)
 *------------------------------------------
 */
int skill_abra_dataset(int skilllv)
{
	int skill = rand() % 331;
	if (skilllv <= 0) return 0;
	//db‚ÉŠî‚Ã‚­ƒŒƒxƒ‹EŠm—¦”»’è
	if (skill_abra_db[skill].req_lv > skilllv || rand() % 10000 >= skill_abra_db[skill].per) return 0;
	//NPCƒXƒLƒ‹‚Íƒ_ƒ
	if (skill >= NPC_PIERCINGATT && skill <= NPC_SUMMONMONSTER) return 0;
	//‰‰‘tƒXƒLƒ‹‚Íƒ_ƒ
	if (skill_get_unit_flag(skill) & UF_DANCE) return 0;

	return skill;
}

/*==========================================
 * ƒoƒWƒŠƒJ‚ÌƒZƒ‹‚ğİ’è‚·‚é
 *------------------------------------------
 */
void skill_basilica_cell(struct skill_unit *unit, int flag) {
	int i, x, y, range = skill_get_unit_range(HP_BASILICA);
	int size = range * 2 + 1;

	for (i = 0; i < size * size; i++) {
		x = unit->bl.x + (i % size - range);
		y = unit->bl.y + (i / size - range);
		map_setcell(unit->bl.m, x, y, flag);
	}
}

/*==========================================
 *
 *------------------------------------------
 */
int skill_attack_area(struct block_list *bl,va_list ap)
{
	struct block_list *src,*dsrc;
	int atk_type,skillid,skilllv,flag,type;
	unsigned int tick;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);

	atk_type = va_arg(ap,int);
	if((src=va_arg(ap,struct block_list*)) == NULL)
		return 0;
	if((dsrc=va_arg(ap,struct block_list*)) == NULL)
		return 0;
	skillid=va_arg(ap,int);
	skilllv=va_arg(ap,int);
	if (skillid > 0 && skilllv <= 0) return 0; // celest
	tick=va_arg(ap,unsigned int);
	flag=va_arg(ap,int);
	type=va_arg(ap,int);

	if(battle_check_target(dsrc,bl,type) > 0)
		skill_attack(atk_type,src,dsrc,bl,skillid,skilllv,tick,flag);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int skill_clear_element_field(struct block_list *bl)
{
	struct mob_data *md = NULL;
	struct map_session_data *sd = NULL;
	int i, max, skillid;

	nullpo_retr(0, bl);

	if (bl->type == BL_MOB) {
		max = MAX_MOBSKILLUNITGROUP;
		md = (struct mob_data *)bl;
	} else if (bl->type == BL_PC) {
		max = MAX_SKILLUNITGROUP;
		sd = (struct map_session_data *)bl;
	} else
		return 0;

	for (i = 0; i < max; i++) {
		if(sd){
			skillid=sd->skillunit[i].skill_id;
			if(skillid==SA_DELUGE||skillid==SA_VOLCANO||skillid==SA_VIOLENTGALE||skillid==SA_LANDPROTECTOR)
				skill_delunitgroup(&sd->skillunit[i]);
		}else if(md){
			skillid=md->skillunit[i].skill_id;
			if(skillid==SA_DELUGE||skillid==SA_VOLCANO||skillid==SA_VIOLENTGALE||skillid==SA_LANDPROTECTOR)
				skill_delunitgroup(&md->skillunit[i]);
		}
	}

	return 0;
}

/*==========================================
 * ƒ‰ƒ“ƒhƒvƒƒeƒNƒ^[ƒ`ƒFƒbƒN(foreachinarea)
 *------------------------------------------
 */
int skill_landprotector(struct block_list *bl, va_list ap )
{
	int skillid;
	int *alive;
	struct skill_unit *unit;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);

	skillid=va_arg(ap,int);
	alive=va_arg(ap,int *);
	if((unit=(struct skill_unit *)bl) == NULL)
		return 0;

	if(skillid==SA_LANDPROTECTOR){
		skill_delunit(unit);
	}else{
		if(alive && unit->group->skill_id==SA_LANDPROTECTOR)
			(*alive)=0;
	}

	return 0;
}

/*==========================================
 * ƒCƒhƒDƒ“‚Ì—ÑŒç‚Ì‰ñ•œˆ—(foreachinarea)
 *------------------------------------------
 */
int skill_idun_heal(struct block_list *bl, va_list ap )
{
	struct skill_unit *unit;
	struct skill_unit_group *sg;
	int heal;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);
	nullpo_retr(0, unit = va_arg(ap,struct skill_unit *));
	nullpo_retr(0, sg = unit->group);

	heal = 30 + sg->skill_lv * 5 + ((sg->val1) >> 16) * 5 + ((sg->val1) & 0xfff) / 2;

	if(bl->type == BL_SKILL || bl->id == sg->src_id)
		return 0;

	if(bl->type == BL_PC || bl->type == BL_MOB){
	clif_skill_nodamage(&unit->bl,bl,AL_HEAL,heal,1);
	battle_heal(NULL,bl,heal,0,0);
	}

	return 0;
}

/*==========================================
 * w’è”ÍˆÍ“à‚Åsrc‚É‘Î‚µ‚Ä—LŒø‚Èƒ^[ƒQƒbƒg‚Ìbl‚Ì”‚ğ”‚¦‚é(foreachinarea)
 *------------------------------------------
 */
int skill_count_target(struct block_list *bl, va_list ap) {
	struct block_list *src;
	int *c;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);

	if((src = va_arg(ap,struct block_list *)) == NULL)
		return 0;
	if((c = va_arg(ap,int *)) == NULL)
		return 0;
	if(battle_check_target(src,bl,BCT_ENEMY) > 0)
		(*c)++;

	return 0;
}

/*==========================================
 * ƒgƒ‰ƒbƒv”ÍˆÍˆ—(foreachinarea)
 *------------------------------------------
 */
int skill_trap_splash(struct block_list *bl, va_list ap )
{
	struct block_list *src;
	int tick;
	int splash_count;
	struct skill_unit *unit;
	struct skill_unit_group *sg;
	struct block_list *ss;
	int i;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);
	nullpo_retr(0, src = va_arg(ap,struct block_list *));
	nullpo_retr(0, unit = (struct skill_unit *)src);
	nullpo_retr(0, sg = unit->group);
	nullpo_retr(0, ss = map_id2bl(sg->src_id));

	tick = va_arg(ap,int);
	splash_count = va_arg(ap,int);

	if(battle_check_target(src,bl,BCT_ENEMY) > 0){
		switch(sg->unit_id){
			case 0x95:	/* ƒTƒ“ƒhƒ}ƒ“ */
			case 0x96:	/* ƒtƒ‰ƒbƒVƒƒ[ */
			case 0x94:	/* ƒVƒ‡ƒbƒNƒEƒF[ƒuƒgƒ‰ƒbƒv */
				skill_additional_effect(ss,bl,sg->skill_id,sg->skill_lv,BF_MISC,tick);
				break;
			case 0x8f:	/* ƒuƒ‰ƒXƒgƒ}ƒCƒ“ */
			case 0x98:	/* ƒNƒŒƒCƒ‚ƒA[ƒgƒ‰ƒbƒv */
				for(i=0;i<splash_count;i++){
					skill_attack(BF_MISC,ss,src,bl,sg->skill_id,sg->skill_lv,tick,(sg->val2)?0x0500:0);
				}
			case 0x97:	/* ƒtƒŠ[ƒWƒ“ƒOƒgƒ‰ƒbƒv */
					skill_attack(BF_WEAPON,	ss,src,bl,sg->skill_id,sg->skill_lv,tick,(sg->val2)?0x0500:0);
				break;
			default:
				break;
		}
	}

	return 0;
}

/*----------------------------------------------------------------------------
 * ƒXƒe[ƒ^ƒXˆÙí
 *----------------------------------------------------------------------------
 */

/*==========================================
 * ƒXƒe[ƒ^ƒXˆÙíI—¹
 *------------------------------------------
 */
int skill_encchant_eremental_end(struct block_list *bl,int type)
{
	struct status_change *sc_data;

	nullpo_retr(0, bl);
	nullpo_retr(0, sc_data=status_get_sc_data(bl));

	if( type!=SC_ENCPOISON && sc_data[SC_ENCPOISON].timer!=-1 )	/* ƒGƒ“ƒ`ƒƒƒ“ƒgƒ|ƒCƒYƒ“‰ğœ */
		status_change_end(bl,SC_ENCPOISON,-1);
	if( type!=SC_ASPERSIO && sc_data[SC_ASPERSIO].timer!=-1 )	/* ƒAƒXƒyƒ‹ƒVƒI‰ğœ */
		status_change_end(bl,SC_ASPERSIO,-1);
	if( type!=SC_FLAMELAUNCHER && sc_data[SC_FLAMELAUNCHER].timer!=-1 )	/* ƒtƒŒƒCƒ€ƒ‰ƒ“ƒ`ƒƒ‰ğœ */
		status_change_end(bl,SC_FLAMELAUNCHER,-1);
	if( type!=SC_FROSTWEAPON && sc_data[SC_FROSTWEAPON].timer!=-1 )	/* ƒtƒƒXƒgƒEƒFƒ|ƒ“‰ğœ */
		status_change_end(bl,SC_FROSTWEAPON,-1);
	if( type!=SC_LIGHTNINGLOADER && sc_data[SC_LIGHTNINGLOADER].timer!=-1 )	/* ƒ‰ƒCƒgƒjƒ“ƒOƒ[ƒ_[‰ğœ */
		status_change_end(bl,SC_LIGHTNINGLOADER,-1);
	if( type!=SC_SEISMICWEAPON && sc_data[SC_SEISMICWEAPON].timer!=-1 )	/* ƒTƒCƒXƒ~ƒbƒNƒEƒFƒ|ƒ“‰ğœ */
		status_change_end(bl,SC_SEISMICWEAPON,-1);

	return 0;
}

/* ƒNƒ[ƒLƒ“ƒOŒŸ¸iü‚è‚ÉˆÚ“®•s‰Â”\’n‘Ñ‚ª‚ ‚é‚©j */
int skill_check_cloaking(struct block_list *bl)
{
	struct map_session_data *sd = NULL; // init to avoid a warning at compilation
	static int dx[] = {-1, 0, 1,-1, 1,-1, 0, 1};
	static int dy[] = {-1,-1,-1, 0, 0, 1, 1, 1};
	int end = 1, i;

	nullpo_retr(1, bl);

	if (bl->type == BL_PC) {
		nullpo_retr(1, sd = (struct map_session_data *)bl);
		//if (!(battle_config.pc_cloak_check_type & 1)) // If it's No it shouldn't be checked // if we doesn't check walls.
		if (!(battle_config.pc_cloak_check_type & 1) || *status_get_option(bl) == 16388) // If it's No it shouldn't be checked; added stalker check [BeoWulf]
			return 0;
	} else if (bl->type == BL_MOB && !battle_config.monster_cloak_check_type)
		return 0;

	for(i = 0;i < sizeof(dx) / sizeof(dx[0]); i++) {
		if (map_getcell(bl->m, bl->x + dx[i], bl->y + dy[i], CELL_CHKNOPASS)) { // must not be CELL_CHKWALL ?
			end = 0;
			break;
		}
	}

	if (end) {
		if ((sd && pc_checkskill(sd, AS_CLOAKING) < 3) || bl->type == BL_MOB) {
			status_change_end(bl, SC_CLOAKING, -1);
			*status_get_option(bl) &= ~4; /* ”O‚Ì‚½‚ß‚Ìˆ— */
		}
		else if (sd && sd->sc_data[SC_CLOAKING].val3 != 130) {
			sd->sc_data[SC_CLOAKING].val3 = 130;
			status_calc_speed(sd);
		}
	}
	else {
		if (sd && sd->sc_data[SC_CLOAKING].val3 != 103) {
			sd->sc_data[SC_CLOAKING].val3 = 103;
			status_calc_speed(sd);
		}
	}

	return end;
}

/*
 *----------------------------------------------------------------------------
 * ƒXƒLƒ‹ƒ†ƒjƒbƒg
 *----------------------------------------------------------------------------
 */

/*==========================================
 * ‰‰‘t/ƒ_ƒ“ƒX‚ğ‚â‚ß‚é
 * flag 1‚Å‡‘t’†‚È‚ç‘Š•û‚Éƒ†ƒjƒbƒg‚ğ”C‚¹‚é
 *
 *------------------------------------------
 */
void skill_stop_dancing(struct block_list *src, int flag)
{
	struct status_change* sc_data;
	struct skill_unit_group* group;
	short* sc_count;

	nullpo_retv(src);
	nullpo_retv(sc_data = status_get_sc_data(src));
	nullpo_retv(sc_count = status_get_sc_count(src));

	if ((*sc_count) > 0 && sc_data[SC_DANCING].timer != -1) {
		group=(struct skill_unit_group *)sc_data[SC_DANCING].val2; //ƒ_ƒ“ƒX‚ÌƒXƒLƒ‹ƒ†ƒjƒbƒgID‚Íval2‚É“ü‚Á‚Ä‚é
		if(group && src->type==BL_PC && sc_data && sc_data[SC_DANCING].val4){ //‡‘t’†’f
			struct map_session_data* dsd=map_id2sd(sc_data[SC_DANCING].val4); //‘Š•û‚Ìsdæ“¾
			if(flag){ //ƒƒOƒAƒEƒg‚È‚Ç•Ğ•û‚ª—‚¿‚Ä‚à‰‰‘t‚ªŒp‘±‚³‚ê‚é
				if(dsd && src->id == group->src_id){ //ƒOƒ‹[ƒv‚ğ‚Á‚Ä‚éPC‚ª—‚¿‚é
					group->src_id=sc_data[SC_DANCING].val4; //‘Š•û‚ÉƒOƒ‹[ƒv‚ğ”C‚¹‚é
					if(flag&1) //ƒƒOƒAƒEƒg
					dsd->sc_data[SC_DANCING].val4=0; //‘Š•û‚Ì‘Š•û‚ğ0‚É‚µ‚Ä‡‘tI—¹¨’Êí‚Ìƒ_ƒ“ƒXó‘Ô
					if(flag&2) //ƒnƒG”ò‚Ñ‚È‚Ç
						return; //‡‘t‚àƒ_ƒ“ƒXó‘Ô‚àI—¹‚³‚¹‚È‚¢•ƒXƒLƒ‹ƒ†ƒjƒbƒg‚Í’u‚¢‚Ä‚¯‚Ú‚è
				}else if(dsd && dsd->bl.id == group->src_id){ //‘Š•û‚ªƒOƒ‹[ƒv‚ğ‚Á‚Ä‚¢‚éPC‚ª—‚¿‚é(©•ª‚ÍƒOƒ‹[ƒv‚ğ‚Á‚Ä‚¢‚È‚¢)
					if(flag&1) //ƒƒOƒAƒEƒg
					dsd->sc_data[SC_DANCING].val4=0; //‘Š•û‚Ì‘Š•û‚ğ0‚É‚µ‚Ä‡‘tI—¹¨’Êí‚Ìƒ_ƒ“ƒXó‘Ô
					if(flag&2) //ƒnƒG”ò‚Ñ‚È‚Ç
						return; //‡‘t‚àƒ_ƒ“ƒXó‘Ô‚àI—¹‚³‚¹‚È‚¢•ƒXƒLƒ‹ƒ†ƒjƒbƒg‚Í’u‚¢‚Ä‚¯‚Ú‚è
				}
				status_change_end(src,SC_DANCING,-1);//©•ª‚ÌƒXƒe[ƒ^ƒX‚ğI—¹‚³‚¹‚é
				//‚»‚µ‚ÄƒOƒ‹[ƒv‚ÍÁ‚³‚È‚¢•Á‚³‚È‚¢‚Ì‚ÅƒXƒe[ƒ^ƒXŒvZ‚à‚¢‚ç‚È‚¢H
				return;
			}else{
				if(dsd && src->id == group->src_id){ //ƒOƒ‹[ƒv‚ğ‚Á‚Ä‚éPC‚ª~‚ß‚é
					status_change_end((struct block_list *)dsd,SC_DANCING,-1);//‘Šè‚ÌƒXƒe[ƒ^ƒX‚ğI—¹‚³‚¹‚é
				}
				if(dsd && dsd->bl.id == group->src_id){ //‘Š•û‚ªƒOƒ‹[ƒv‚ğ‚Á‚Ä‚¢‚éPC‚ª~‚ß‚é(©•ª‚ÍƒOƒ‹[ƒv‚ğ‚Á‚Ä‚¢‚È‚¢)
					status_change_end(src,SC_DANCING,-1);//©•ª‚ÌƒXƒe[ƒ^ƒX‚ğI—¹‚³‚¹‚é
				}
			}
		}
		if(flag&2 && group && src->type==BL_PC){ //ƒnƒG‚Å”ò‚ñ‚¾‚Æ‚«‚Æ‚©‚Íƒ†ƒjƒbƒg‚à”ò‚Ô
			struct map_session_data *sd = (struct map_session_data *)src;
			skill_unit_move_unit_group(group, sd->bl.m,(sd->to_x - sd->bl.x),(sd->to_y - sd->bl.y));
			return;
		}
		skill_delunitgroup(group);
		if(src->type==BL_PC)
			status_calc_pc((struct map_session_data *)src,0);
	}
}

/*==========================================
 * ƒXƒLƒ‹ƒ†ƒjƒbƒg‰Šú‰»
 *------------------------------------------
 */
struct skill_unit *skill_initunit(struct skill_unit_group *group,int idx,int x,int y)
{
	struct skill_unit *unit;

	nullpo_retr(NULL, group);
	nullpo_retr(NULL, unit=&group->unit[idx]);

	if(!unit->alive)
		group->alive_count++;

	unit->bl.id=map_addobject(&unit->bl);
	unit->bl.type=BL_SKILL;
	unit->bl.m=group->map;
	unit->bl.x=x;
	unit->bl.y=y;
	unit->group=group;
	unit->val1=unit->val2=0;
	unit->alive=1;

	map_addblock(&unit->bl);
	clif_skill_setunit(unit);

	if (group->skill_id == HP_BASILICA)
		skill_basilica_cell(unit, CELL_SETBASILICA);

	return unit;
}

/*==========================================
 * ƒXƒLƒ‹ƒ†ƒjƒbƒgíœ
 *------------------------------------------
 */
int skill_delunit(struct skill_unit *unit)
{
	struct skill_unit_group *group;

	nullpo_retr(0, unit);
	if (!unit->alive)
		return 0;
	nullpo_retr(0, group = unit->group);

	/* onlimitƒCƒxƒ“ƒgŒÄ‚Ño‚µ */
	skill_unit_onlimit(unit, gettick_cache);

	/* onoutƒCƒxƒ“ƒgŒÄ‚Ño‚µ */
	if (!unit->range) {
		map_foreachinarea(skill_unit_effect, unit->bl.m,
			unit->bl.x, unit->bl.y, unit->bl.x, unit->bl.y, 0,
			&unit->bl, gettick_cache, 0);
	}

	if (group->skill_id == HP_BASILICA)
		skill_basilica_cell(unit, CELL_CLRBASILICA);

	clif_skill_delunit(unit);

	unit->group = NULL;
	unit->alive = 0;
	map_delobjectnofree(unit->bl.id);
	if (group->alive_count > 0 && (--group->alive_count) <= 0)
		skill_delunitgroup(group);

	return 0;
}

/*==========================================
 * ƒXƒLƒ‹ƒ†ƒjƒbƒgƒOƒ‹[ƒv‰Šú‰»
 *------------------------------------------
 */
static int skill_unit_group_newid = MAX_SKILL_DB;
struct skill_unit_group *skill_initunitgroup(struct block_list *src,
    int count, int skillid, int skilllv, int unit_id)
{
	int i;
	struct skill_unit_group *group = NULL, *list = NULL;
	int maxsug = 0;

	nullpo_retr(NULL, src);

	if(skilllv <= 0) return 0;

	if(src->type==BL_PC){
		list=((struct map_session_data *)src)->skillunit;
		maxsug=MAX_SKILLUNITGROUP;
	}else if(src->type==BL_MOB){
		list=((struct mob_data *)src)->skillunit;
		maxsug=MAX_MOBSKILLUNITGROUP;
	}else if(src->type==BL_PET){
		list=((struct pet_data *)src)->skillunit;
		maxsug=MAX_MOBSKILLUNITGROUP;
	}
	if(list){
		for(i=0;i<maxsug;i++)	/* ‹ó‚¢‚Ä‚¢‚é‚à‚ÌŒŸõ */
			if(list[i].group_id==0){
				group=&list[i];
				break;
			}

		if(group==NULL){	/* ‹ó‚¢‚Ä‚È‚¢‚Ì‚ÅŒÃ‚¢‚à‚ÌŒŸõ */
			int j = 0;
			unsigned maxdiff = 0, x;
			for(i = 0; i < maxsug; i++)
				if((x = DIFF_TICK(gettick_cache, list[i].tick)) > maxdiff) {
					maxdiff = x;
					j = i;
				}
			skill_delunitgroup(&list[j]);
			group=&list[j];
		}
	}

	if(group==NULL){
		printf("skill_initunitgroup: error unit group !\n");
		exit(1);
	}

	group->src_id = src->id;
	group->party_id = status_get_party_id(src);
	group->guild_id = status_get_guild_id(src);
	group->group_id = skill_unit_group_newid++;
	if (skill_unit_group_newid <= 0)
		skill_unit_group_newid = MAX_SKILL_DB;
	CALLOC(group->unit, struct skill_unit, count);
	group->unit_count=count;
	group->val1=group->val2=0;
	group->skill_id=skillid;
	group->skill_lv=skilllv;
	group->unit_id=unit_id;
	group->map=src->m;
	group->limit=10000;
	group->interval=1000;
	group->tick = gettick_cache;
	group->valstr = NULL;

	if (skill_get_unit_flag(skillid) & UF_DANCE) {
		struct map_session_data *sd = NULL;
		if(src->type==BL_PC && (sd=(struct map_session_data *)src) ){
			sd->skillid_dance=skillid;
			sd->skilllv_dance=skilllv;
		}
		status_change_start(src,SC_DANCING,skillid,(int)group,0,0,skill_get_time(skillid,skilllv)+1000,0);
		//‡‘tƒXƒLƒ‹‚Í‘Š•û‚ğƒ_ƒ“ƒXó‘Ô‚É‚·‚é
		if (sd && skill_get_unit_flag(skillid) & UF_ENSEMBLE) {
			int c=0;
			map_foreachinarea(skill_check_condition_use_sub,sd->bl.m,
			                  sd->bl.x - 1, sd->bl.y - 1, sd->bl.x + 1, sd->bl.y + 1, BL_PC, &sd->bl, &c);
		}
	}

	return group;
}

/*==========================================
 * ƒXƒLƒ‹ƒ†ƒjƒbƒgƒOƒ‹[ƒvíœ
 *------------------------------------------
 */
int skill_delunitgroup(struct skill_unit_group *group)
{
	struct block_list *src;
	int i;

	nullpo_retr(0, group);
	if(group->unit_count<=0)
		return 0;

	src=map_id2bl(group->src_id);
	//ƒ_ƒ“ƒXƒXƒLƒ‹‚Íƒ_ƒ“ƒXó‘Ô‚ğ‰ğœ‚·‚é
	if (skill_get_unit_flag(group->skill_id) & UF_DANCE) {
		if(src)
			status_change_end(src,SC_DANCING,-1);
		}

	group->alive_count=0;
	if(group->unit!=NULL){
		for(i=0;i<group->unit_count;i++)
			if(group->unit[i].alive)
				skill_delunit(&group->unit[i]);
	}
	if(group->valstr!=NULL){
		map_freeblock(group->valstr);
		group->valstr=NULL;
	}

	map_freeblock(group->unit);	/* free()‚Ì‘Ö‚í‚è */
	group->unit=NULL;
	group->src_id=0;
	group->group_id=0;
	group->unit_count=0;

	return 0;
}

/*==========================================
 * ƒXƒLƒ‹ƒ†ƒjƒbƒgƒOƒ‹[ƒv‘Síœ
 *------------------------------------------
 */
int skill_clear_unitgroup(struct block_list *src)
{
	struct skill_unit_group *group=NULL;
	int maxsug=0;

	nullpo_retr(0, src);

	if(src->type==BL_PC){
		group=((struct map_session_data *)src)->skillunit;
		maxsug=MAX_SKILLUNITGROUP;
	}else if(src->type==BL_MOB){
		group=((struct mob_data *)src)->skillunit;
		maxsug=MAX_MOBSKILLUNITGROUP;
	}else if(src->type==BL_PET){ // [Valaris]
		group=((struct pet_data *)src)->skillunit;
		maxsug=MAX_MOBSKILLUNITGROUP;
	} else
		return 0;

	if(group){
		int i;
		for(i=0;i<maxsug;i++)
			if(group[i].group_id>0 && group[i].src_id == src->id)
				skill_delunitgroup(&group[i]);
	}

	return 0;
}

/*==========================================
 * ƒXƒLƒ‹ƒ†ƒjƒbƒgƒOƒ‹[ƒv‚Ì”í‰e‹¿tickŒŸõ
 *------------------------------------------
 */
struct skill_unit_group_tickset *skill_unitgrouptickset_search(struct block_list *bl, struct skill_unit_group *group, int tick)
{
	int i, j = -1, k, s, id;
	struct skill_unit_group_tickset *set;

	nullpo_retr(0, bl);

	if (group->interval == -1)
		return NULL;

	if (bl->type == BL_PC)
		set = ((struct map_session_data *)bl)->skillunittick;
	else if (bl->type == BL_MOB)
		set = ((struct mob_data *)bl)->skillunittick;
	else
		return 0;

	if (skill_get_unit_flag(group->skill_id) & UF_NOOVERLAP)
		id = s = group->skill_id;
	else
		id = s = group->group_id;

	for (i = 0; i < MAX_SKILLUNITGROUPTICKSET; i++) {
		k = (i + s) % MAX_SKILLUNITGROUPTICKSET;
		if (set[k].id == id)
			return &set[k];
		else if (j == -1 && (DIFF_TICK(tick, set[k].tick) > 0 || set[k].id == 0))
			j = k;
	}

	if (j == -1) {
		if (battle_config.error_log) {
			printf("skill_unitgrouptickset_search: tickset is full.\n");
		}
		j = id % MAX_SKILLUNITGROUPTICKSET;
	}

	set[j].id = id;
	set[j].tick = tick;

	return &set[j];
}

/*==========================================
 * ƒXƒLƒ‹ƒ†ƒjƒbƒgƒ^ƒCƒ}[”­“®ˆ——p(foreachinarea)
 *------------------------------------------
 */
int skill_unit_timer_sub_onplace(struct block_list *bl, va_list ap)
{
	struct skill_unit *unit;
	struct skill_unit_group *group;
	unsigned int tick;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);
	unit = va_arg(ap, struct skill_unit *);
	tick = va_arg(ap, unsigned int);

	if (bl->type != BL_PC && bl->type != BL_MOB)
		return 0;
	if (!unit->alive || bl->prev == NULL)
		return 0;

	nullpo_retr(0, group = unit->group);

	if (battle_check_target(&unit->bl, bl, group->target_flag) <= 0)
		return 0;

	skill_unit_onplace_timer(unit, bl, tick);

	return 0;
}

/*==========================================
 * ƒXƒLƒ‹ƒ†ƒjƒbƒgƒ^ƒCƒ}[ˆ——p(foreachobject)
 *------------------------------------------
 */
int skill_unit_timer_sub(struct block_list *bl, va_list ap)
{
	struct skill_unit *unit;
	struct skill_unit_group *group;
	int range;
	unsigned int tick;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);
	nullpo_retr(0, unit = (struct skill_unit *)bl);
	tick = va_arg(ap, unsigned int);

	if (!unit->alive)
		return 0;

	nullpo_retr(0, group = unit->group);
	range = unit->range;

	/* onplace_timerƒCƒxƒ“ƒgŒÄ‚Ño‚µ */
	if (range >= 0 && group->interval != -1) {
		map_foreachinarea(skill_unit_timer_sub_onplace, bl->m,
		                  bl->x - range, bl->y - range, bl->x + range, bl->y + range, 0, bl, tick);
		if (!unit->alive)
			return 0;
		// ƒ}ƒOƒkƒX‚Í”­“®‚µ‚½ƒ†ƒjƒbƒg‚Ííœ‚·‚é
		if (group->skill_id == PR_MAGNUS && unit->val2) {
			skill_delunit(unit);
			return 0;
		}
	}
	// ƒCƒhƒDƒ“‚Ì—ÑŒç‚É‚æ‚é‰ñ•œ
	if (group->unit_id == 0xaa && DIFF_TICK(tick, group->tick) >= 6000 * group->val3) {
		struct block_list *src = map_id2bl(group->src_id);
		int range2;
		nullpo_retr(0, src);
		range2 = skill_get_unit_layout_type(group->skill_id, group->skill_lv);
		map_foreachinarea(skill_idun_heal, src->m,
		                  src->x - range2, src->y - range2, src->x + range2, src->y + range2, 0, unit);
		group->val3++;
	}
	/* ŠÔØ‚êíœ */
	if ((DIFF_TICK(tick,group->tick) >= group->limit || DIFF_TICK(tick, group->tick) >= unit->limit)) {
		switch(group->unit_id) {
			case 0x8f:	/* ƒuƒ‰ƒXƒgƒ}ƒCƒ“ */
				group->unit_id = 0x8c;
				clif_changelook(bl,LOOK_BASE,group->unit_id);
				group->limit=DIFF_TICK(tick+1500,group->tick);
				unit->limit=DIFF_TICK(tick+1500,group->tick);
				break;
			case 0x90:	/* ƒXƒLƒbƒhƒgƒ‰ƒbƒv */
			case 0x91:	/* ƒAƒ“ƒNƒ‹ƒXƒlƒA */
			case 0x93:	/* ƒ‰ƒ“ƒhƒ}ƒCƒ“ */
			case 0x94:	/* ƒVƒ‡ƒbƒNƒEƒF[ƒuƒgƒ‰ƒbƒv */
			case 0x95:	/* ƒTƒ“ƒhƒ}ƒ“ */
			case 0x96:	/* ƒtƒ‰ƒbƒVƒƒ[ */
			case 0x97:	/* ƒtƒŠ[ƒWƒ“ƒOƒgƒ‰ƒbƒv */
			case 0x98:	/* ƒNƒŒƒCƒ‚ƒA[ƒgƒ‰ƒbƒv */
			case 0x99:	/* ƒg[ƒL[ƒ{ƒbƒNƒX */
				{
					struct block_list *src = map_id2bl(group->src_id);
					if (group->unit_id == 0x91 && group->val2)
						;
					else {
						if (src && src->type == BL_PC){
							struct item item_tmp;
							memset(&item_tmp, 0, sizeof(item_tmp));
							item_tmp.nameid = 1065;
							item_tmp.identify = 1;
							map_addflooritem(&item_tmp, 1, bl->m, bl->x, bl->y, NULL, NULL, NULL, bl->id, 0);	// ?•ÔŠÒ
						}
					}
					skill_delunit(unit);
				}
				break;

			case 0xc1:
			case 0xc2:
			case 0xc3:
			case 0xc4:
				{
					struct block_list *src=map_id2bl(group->src_id);
					if (src)
						group->tick = tick;
				}
				break;

			default:
				skill_delunit(unit);
		}
	}

	if(group->unit_id == 0x8d) {
		unit->val1 -= 5;
		if(unit->val1 <= 0 && unit->limit + group->tick > tick + 700)
			unit->limit = DIFF_TICK(tick+700,group->tick);
	}

	return 0;
}

/*==========================================
 * ƒXƒLƒ‹ƒ†ƒjƒbƒgƒ^ƒCƒ}[ˆ—
 *------------------------------------------
 */
int skill_unit_timer(int tid, unsigned int tick, int id, int data)
{
	map_foreachobject(skill_unit_timer_sub, BL_SKILL, tick);

	return 0;
}

/*==========================================
 * ƒXƒLƒ‹ƒ†ƒjƒbƒgˆÚ“®ˆ——p(foreachinarea)
 *------------------------------------------
 */
int skill_unit_move_sub(struct block_list *bl, va_list ap)
{
	struct skill_unit *unit = (struct skill_unit *)bl;
	struct skill_unit_group *group;
	struct block_list *target;
	unsigned int tick, flag;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);
	nullpo_retr(0, target = va_arg(ap, struct block_list*));
	tick = va_arg(ap, unsigned int);
	flag = va_arg(ap, int);

	if (target->type != BL_PC && target->type != BL_MOB)
		return 0;

	nullpo_retr(0, group = unit->group);
	if (group->interval != -1)
		return 0;

	if (!unit->alive || target->prev == NULL)
		return 0;

	if (flag)
		skill_unit_onplace(unit, target, tick);
	else
		skill_unit_onout(unit, target, tick);

	return 0;
}

/*==========================================
 * ƒXƒLƒ‹ƒ†ƒjƒbƒgˆÚ“®ˆ—
 *------------------------------------------
 */
int skill_unit_move(struct block_list *bl, unsigned int tick, int flag)
{
	nullpo_retr(0, bl);

	if (bl->prev == NULL)
		return 0;

	map_foreachinarea(skill_unit_move_sub,
	                  bl->m, bl->x, bl->y, bl->x, bl->y, BL_SKILL, bl, tick, flag);

	return 0;
}

/*==========================================
 * ƒXƒLƒ‹ƒ†ƒjƒbƒg©‘Ì‚ÌˆÚ“®ˆ—
 * ˆø”‚ÍƒOƒ‹[ƒv‚ÆˆÚ“®—Ê
 *------------------------------------------
 */
int skill_unit_move_unit_group( struct skill_unit_group *group, int m,int dx,int dy)
{
	int i, j;
	int *m_flag;
	struct skill_unit *unit1;
	struct skill_unit *unit2;

	nullpo_retr(0, group);
	if (group->unit_count <= 0)
		return 0;
	if (group->unit == NULL)
		return 0;

	// ˆÚ“®‰Â”\‚ÈƒXƒLƒ‹‚Íƒ_ƒ“ƒXŒn‚ÆAƒuƒ‰ƒXƒgƒ}ƒCƒ“AƒNƒŒƒCƒ‚ƒA[ƒgƒ‰ƒbƒv‚Ì‚İ
	if (!(skill_get_unit_flag(group->skill_id) & UF_DANCE) &&
	    group->skill_id != HT_CLAYMORETRAP && group->skill_id != HT_BLASTMINE)
		return 0;

	CALLOC(m_flag, int, group->unit_count); // ˆÚ“®ƒtƒ‰ƒO
	//æ‚Éƒtƒ‰ƒO‚ğ‘S•”Œˆ‚ß‚é
	//    m_flag
	//		0: ’PƒˆÚ“®
	//      1: ƒ†ƒjƒbƒg‚ğˆÚ“®‚·‚é(Œ»ˆÊ’u‚©‚çƒ†ƒjƒbƒg‚ª‚È‚­‚È‚é)
	//      2: c—¯•VˆÊ’u‚ªˆÚ“®æ‚Æ‚È‚é(ˆÚ“®æ‚Éƒ†ƒjƒbƒg‚ª‘¶İ‚µ‚È‚¢)
	//      3: c—¯
	for(i = 0; i < group->unit_count; i++) {
		unit1 = &group->unit[i];
		if (!unit1->alive || unit1->bl.m != m)
			continue;
		for(j = 0; j < group->unit_count; j++) {
			unit2 = &group->unit[j];
			if (!unit2->alive)
				continue;
			if (unit1->bl.x + dx == unit2->bl.x && unit1->bl.y + dy == unit2->bl.y) {
				// ˆÚ“®æ‚Éƒ†ƒjƒbƒg‚ª‚©‚Ô‚Á‚Ä‚¢‚é
				m_flag[i] |= 0x1;
			}
			if (unit1->bl.x - dx == unit2->bl.x && unit1->bl.y - dy == unit2->bl.y) {
				// ƒ†ƒjƒbƒg‚ª‚±‚ÌêŠ‚É‚â‚Á‚Ä‚­‚é
				m_flag[i] |= 0x2;
			}
		}
	}
	// ƒtƒ‰ƒO‚ÉŠî‚Ã‚¢‚Äƒ†ƒjƒbƒgˆÚ“®
	// ƒtƒ‰ƒO‚ª1‚Ìunit‚ğ’T‚µAƒtƒ‰ƒO‚ª2‚Ìunit‚ÌˆÚ“®æ‚ÉˆÚ‚·
	j = 0;
	for (i = 0; i < group->unit_count; i++) {
		unit1 = &group->unit[i];
		if (!unit1->alive)
			continue;
		if (!(m_flag[i] & 0x2)) {
			// ƒ†ƒjƒbƒg‚ª‚È‚­‚È‚éêŠ‚ÅƒXƒLƒ‹ƒ†ƒjƒbƒg‰e‹¿‚ğÁ‚·
			map_foreachinarea(skill_unit_effect, unit1->bl.m,
			                  unit1->bl.x, unit1->bl.y, unit1->bl.x, unit1->bl.y, 0,
			                  &unit1->bl, gettick_cache, 0);
		}
		if (m_flag[i] == 0) {
			// ’PƒˆÚ“®
			map_delblock(&unit1->bl);
			unit1->bl.m = m;
			unit1->bl.x += dx;
			unit1->bl.y += dy;
			map_addblock(&unit1->bl);
			clif_skill_setunit(unit1);
		} else if (m_flag[i] == 1) {
			// ƒtƒ‰ƒO‚ª2‚Ì‚à‚Ì‚ğ’T‚µ‚Ä‚»‚Ìƒ†ƒjƒbƒg‚ÌˆÚ“®æ‚ÉˆÚ“®
			for( ; j < group->unit_count; j++) {
				if (m_flag[j] == 2) {
					// Œp³ˆÚ“®
					unit2 = &group->unit[j];
					if (!unit2->alive)
						continue;
					map_delblock(&unit1->bl);
					unit1->bl.m = m;
					unit1->bl.x = unit2->bl.x + dx;
					unit1->bl.y = unit2->bl.y + dy;
					map_addblock(&unit1->bl);
					clif_skill_setunit(unit1);
					j++;
					break;
				}
			}
		}
		if (!(m_flag[i] & 0x2)) {
			// ˆÚ“®Œã‚ÌêŠ‚ÅƒXƒLƒ‹ƒ†ƒjƒbƒg‚ğ”­“®
			map_foreachinarea(skill_unit_effect, unit1->bl.m,
			                  unit1->bl.x, unit1->bl.y, unit1->bl.x, unit1->bl.y, 0,
			                  &unit1->bl, gettick_cache, 1);
		}
	}
	FREE(m_flag);

	return 0;
}

/*----------------------------------------------------------------------------
 * ƒAƒCƒeƒ€‡¬
 *----------------------------------------------------------------------------
 */

/*==========================================
 * ƒAƒCƒeƒ€‡¬‰Â”\”»’è
 *------------------------------------------
 */
int skill_can_produce_mix( struct map_session_data *sd, int nameid, int trigger) {
	int i, j;

	nullpo_retr(0, sd);

	if (nameid <= 0)
		return 0;

	for(i = 0; i < MAX_SKILL_PRODUCE_DB; i++) {
		if (skill_produce_db[i].nameid == nameid)
			break;
	}
	if (i >= MAX_SKILL_PRODUCE_DB) /* ƒf[ƒ^ƒx[ƒX‚É‚È‚¢ */
		return 0;

	if (trigger >= 0) {
		if (trigger == 32 || trigger == 16 || trigger == 64 || trigger == 256) {
			if (skill_produce_db[i].itemlv != trigger) /* ƒtƒ@[ƒ}ƒV[–ƒ|[ƒVƒ‡ƒ“—Ş‚Æ—nz˜F–zÎˆÈŠO‚Í‚¾‚ß */
				return 0;
		} else {
			if (skill_produce_db[i].itemlv >= 16) /* •ŠíˆÈŠO‚Í‚¾‚ß */
				return 0;
			if (itemdb_wlv(nameid) > trigger) /* •ŠíLv”»’è */
				return 0;
		}
	}
	if ((j = skill_produce_db[i].req_skill) > 0 && pc_checkskill(sd, j) <= 0)
		return 0; /* ƒXƒLƒ‹‚ª‘«‚è‚È‚¢ */

	for(j = 0; j < MAX_PRODUCE_RESOURCE; j++) {
		int id, x, y;
		id = skill_produce_db[i].mat_id[j];
		// Check if its a bottle, and if he has alchemy (Level 1 for empty bottle, 3 for Empty potion bottle, 5 for Empty test tube - [Aalye]
		if (!(id == 713 && pc_checkskill(sd,CR_ALCHEMY)>=1) && !(id == 1092 && pc_checkskill(sd,CR_ALCHEMY)>=3) && !(id == 1093 && pc_checkskill(sd,CR_ALCHEMY)>=5)) {
			if (id <= 0) /* ‚±‚êˆÈã‚ÍŞ—¿—v‚ç‚È‚¢ */
				continue;
			if (skill_produce_db[i].mat_amount[j] <= 0) {
				if (pc_search_inventory(sd,id) < 0)
					return 0;
			} else {
				for(y = 0, x = 0; y < MAX_INVENTORY; y++)
					if (sd->status.inventory[y].nameid == id)
						x += sd->status.inventory[y].amount;
				if (x < skill_produce_db[i].mat_amount[j]) /* ƒAƒCƒeƒ€‚ª‘«‚è‚È‚¢ */
					return 0;
			}
		}
	}

	return i+1;
}

/*==========================================
 * ƒAƒCƒeƒ€‡¬‰Â”\”»’è
 *------------------------------------------
 */
void skill_produce_mix( struct map_session_data *sd, int nameid, int slot1, int slot2, int slot3) {
	int slot[3];
	int i, sc, ele, idx, equip, wlv, make_per, flag;

//	nullpo_retv(sd); // checked before to call function

	if (!(idx = skill_can_produce_mix(sd, nameid, -1))) /* ğŒ•s‘« */
		return;

	idx--;
	slot[0] = slot1;
	slot[1] = slot2;
	slot[2] = slot3;

	/* –„‚ß‚İˆ— */
	for(i = 0, sc = 0, ele = 0; i < 3; i++) {
		int j;
		if (slot[i] <= 0)
			continue;
		j = pc_search_inventory(sd, slot[i]);
		if (j < 0) /* •s³ƒpƒPƒbƒg(ƒAƒCƒeƒ€‘¶İ)ƒ`ƒFƒbƒN */
			continue;
		if (slot[i] == 1000) { /* ¯‚Ì‚©‚¯‚ç */
			pc_delitem(sd, j, 1, 1);
			sc++;
		}
		if (slot[i] >= 994 && slot[i] <= 997 && ele == 0) { /* ‘®«Î */
			static const int ele_table[4] = {3, 1, 4, 2};
			pc_delitem(sd, j, 1, 1);
			ele = ele_table[slot[i] - 994];
		}
	}

	/* Ş—¿Á”ï */
	for(i = 0; i < MAX_PRODUCE_RESOURCE; i++) {
		int j, id, x;
		id = skill_produce_db[idx].mat_id[i];
		// Check if its a bottle, and if he has alchemy (Level 1 for empty bottle, 3 for Empty potion bottle, 5 for Empty test tube - [Aalye]
		if (!(id == 713 && pc_checkskill(sd, CR_ALCHEMY) >= 1) && !(id == 1092 && pc_checkskill(sd, CR_ALCHEMY) >= 3) && !(id == 1093 && pc_checkskill(sd, CR_ALCHEMY) >= 5)) {
			if (id <= 0)
				continue;
			x = skill_produce_db[idx].mat_amount[i]; /* •K—v‚ÈŒÂ” */
			do{ /* ‚Q‚ÂˆÈã‚ÌƒCƒ“ƒfƒbƒNƒX‚É‚Ü‚½‚ª‚Á‚Ä‚¢‚é‚©‚à‚µ‚ê‚È‚¢ */
				int y = 0;
				j = pc_search_inventory(sd,id);

				if (j >= 0) {
					y = sd->status.inventory[j].amount;
					if (y > x) y = x; /* ‘«‚è‚Ä‚¢‚é */
					pc_delitem(sd, j, y, 0);
				}else {
					if (battle_config.error_log)
						printf("skill_produce_mix: material item error\n");
				}

				x -= y; /* ‚Ü‚¾‘«‚è‚È‚¢ŒÂ”‚ğŒvZ */
			}while(j >= 0 && x > 0); /* Ş—¿‚ğÁ”ï‚·‚é‚©AƒGƒ‰[‚É‚È‚é‚Ü‚ÅŒJ‚è•Ô‚· */
		}
	}

	/* Šm—¦”»’è */
	equip = itemdb_isequip(nameid);
	if (!equip) {
		if (skill_produce_db[idx].req_skill == AM_PHARMACY) {
			make_per = pc_checkskill(sd, AM_LEARNINGPOTION) * 100
			        + pc_checkskill(sd, AM_PHARMACY) * 300 + sd->status.job_level * 20
			        + sd->status.dex * 10 + sd->status.int_ * 5;
			if (nameid >= 501 && nameid <= 505) // Normal potions
				make_per += 2000 + pc_checkskill(sd, AM_POTIONPITCHER) * 100;
			else if (nameid >= 605 && nameid <= 606) // Anodyne & Aloevera (not sure of the formula, I put the same base value as normal pots but without the Aid Potion bonus since they are not throwable pots ^^)
				make_per += 2000;
			else if (nameid >= 545 && nameid <= 547) // Concentrated potions
				;
			else if (nameid == 970) // Alcohol
				make_per += 1000;
			else if (nameid == 7135) // Bottle Grenade
				make_per += 500 + pc_checkskill(sd, AM_DEMONSTRATION) * 100;
			else if (nameid == 7136) // Acid Bottle
				make_per += 500 + pc_checkskill(sd, AM_ACIDTERROR) * 100;
			else if (nameid == 7137) // Plant Bottle
				make_per += 500 + pc_checkskill(sd, AM_CANNIBALIZE) * 100;
			else if (nameid == 7138) // Marine Sphere Bottle
				make_per += 500 + pc_checkskill(sd, AM_SPHEREMINE) * 100;
			else if (nameid == 7139) // Glistening Coat
				make_per += 500 + pc_checkskill(sd, AM_CP_WEAPON) * 100 + pc_checkskill(sd,AM_CP_SHIELD) * 100 +
				            pc_checkskill(sd, AM_CP_ARMOR) * 100 + pc_checkskill(sd, AM_CP_HELM) * 100;
			else
				make_per = 1000 + sd->status.base_level * 30 + sd->paramc[3] * 20 + sd->paramc[4] * 15 + pc_checkskill(sd, AM_LEARNINGPOTION) * 100 + pc_checkskill(sd, AM_PHARMACY) * 300;
			//	make_per = 1000 + sd->status.job_level * 20 + sd->paramc[4] * 10 + sd->paramc[5] * 10 + pc_checkskill(sd, AM_LEARNINGPOTION) * 100 + pc_checkskill(sd, AM_PHARMACY) * 300;
		} else if (skill_produce_db[idx].req_skill == ASC_CDP) {
			make_per = 2000 + 40 * sd->paramc[4] + 20 * sd->paramc[5];
			//make_per = 20 + (20*sd->paramc[4])/50 + (20*sd->paramc[5])/100;
		} else {
			if (nameid == 998)
				make_per = 1500 + sd->status.job_level * 35 + sd->paramc[4] * 10 + sd->paramc[5] * 10 + pc_checkskill(sd, skill_produce_db[idx].req_skill) * 600;
			else
				make_per = 1000 + sd->status.job_level * 35 + sd->paramc[4] * 10 + sd->paramc[5] * 10 + pc_checkskill(sd, skill_produce_db[idx].req_skill) * 500;
		}
	}
	else { // Corrected rates [DracoRPG]
		int add_per = 0;
		if (pc_search_inventory(sd,989) >= 0) add_per = 400;
		else if (pc_search_inventory(sd,988) >= 0) add_per = 300;
		else if (pc_search_inventory(sd,987) >= 0) add_per = 200;
		else if (pc_search_inventory(sd,986) >= 0) add_per = 100;
		wlv = itemdb_wlv(nameid);
		make_per = 1500 + sd->status.job_level * 35 + sd->paramc[4] * 10 + sd->paramc[5] * 10 + pc_checkskill(sd, skill_produce_db[idx].req_skill) * 1000 + pc_checkskill(sd, BS_WEAPONRESEARCH) * 100 +
		           ((wlv >= 3) ? pc_checkskill(sd, BS_ORIDEOCON) * 100 : 0) + add_per - (ele ? 2500 : 0) - sc * ((4-wlv) * 500) - wlv * 1000;
	}

	if (make_per < 1)
		make_per = 1;

	if (skill_produce_db[idx].req_skill == AM_PHARMACY ||
	    skill_produce_db[idx].req_skill == ASC_CDP ||
	    skill_produce_db[idx].req_skill == CR_ALCHEMY) {
		if (battle_config.pp_rate != 100)
			make_per = make_per * battle_config.pp_rate / 100;
	} else {
		if (battle_config.wp_rate != 100) /* Šm—¦•â³ */
			make_per = make_per * battle_config.wp_rate / 100;
	}

//	if(battle_config.etc_log)
//		printf("make rate = %d\n",make_per);

	if (rand() % 10000 < make_per) {
		/* ¬Œ÷ */
		struct item tmp_item;
		memset(&tmp_item, 0, sizeof(tmp_item));
		tmp_item.nameid = nameid;
		tmp_item.amount = 1;
		tmp_item.identify = 1;
		if (equip) { /* •Ší‚Ìê‡ */
			tmp_item.card[0] = 0x00ff; /* »‘¢•Šíƒtƒ‰ƒO */
			tmp_item.card[1] = ((sc*5)<<8) + ele; /* ‘®«‚Æ‚Â‚æ‚³ */
			*((unsigned long *)(&tmp_item.card[2])) = sd->char_id; /* ƒLƒƒƒ‰ID */
		} else if((battle_config.produce_item_name_input && skill_produce_db[idx].req_skill != AM_PHARMACY) ||
		          (battle_config.produce_potion_name_input && skill_produce_db[idx].req_skill == AM_PHARMACY)) {
			tmp_item.card[0] = 0x00fe;
			tmp_item.card[1] = 0;
			*((unsigned long *)(&tmp_item.card[2])) = sd->char_id; /* ƒLƒƒƒ‰ID */
		} else if((battle_config.produce_item_name_input && skill_produce_db[idx].req_skill != CR_ALCHEMY) ||
		          (battle_config.produce_potion_name_input && skill_produce_db[idx].req_skill == CR_ALCHEMY)) {
			tmp_item.card[0] = 0x00fe;
			tmp_item.card[1] = 0;
			*((unsigned long *)(&tmp_item.card[2])) = sd->char_id; /* ƒLƒƒƒ‰ID */
		}

		switch (skill_produce_db[idx].req_skill) {
			case AM_PHARMACY:
			case CR_ALCHEMY: // added a small chance to the potion success rate. [Aalye] from freya' forum
			clif_produceeffect(sd,2,nameid);/* »–òƒGƒtƒFƒNƒgƒpƒPƒbƒg */
			clif_misceffect(&sd->bl,5); /* ‘¼l‚É‚à¬Œ÷‚ğ’Ê’m*/
			break;
		case ASC_CDP:
			clif_produceeffect(sd,2,nameid);/* b’è‚Å»–òƒGƒtƒFƒNƒg */
			clif_misceffect(&sd->bl,5); /* ‘¼l‚É‚à¬Œ÷‚ğ’Ê’m*/
			break;
		default: /* •Ší»‘¢AƒRƒCƒ“»‘¢ */
			clif_produceeffect(sd,0,nameid);/* •s–¾‚È‚Ì‚Å‚Æ‚è‚ ‚¦‚¸»‘¢ƒGƒtƒFƒNƒgƒpƒPƒbƒg */
			clif_misceffect(&sd->bl,3); /* ‘¼l‚É‚à¬Œ÷‚ğ’Ê’m*/
			break;
		}

		if ((flag = pc_additem(sd, &tmp_item, 1))) {
			clif_additem(sd, 0, 0, flag);
			map_addflooritem(&tmp_item, 1, sd->bl.m, sd->bl.x, sd->bl.y, NULL, NULL, NULL, sd->bl.id, 0);
		}
	} else {
		switch (skill_produce_db[idx].req_skill) {
		case AM_PHARMACY:
		case CR_ALCHEMY: // added a small chance to the potion success rate. [Aalye] from freya' forum
		clif_produceeffect(sd,3,nameid);/* »–ò¸”sƒGƒtƒFƒNƒgƒpƒPƒbƒg */
		clif_misceffect(&sd->bl,6); /* ‘¼l‚É‚à¸”s‚ğ’Ê’m*/
			break;
		case ASC_CDP:
			{
			clif_produceeffect(sd,3,nameid);/* »–ò¸”sƒGƒtƒFƒNƒgƒpƒPƒbƒg */
			clif_misceffect(&sd->bl,6); /* ‘¼l‚É‚à¸”s‚ğ’Ê’m*/
				pc_heal(sd, -(sd->status.max_hp>>2), 0);
			}
			break;
		default:
			clif_produceeffect(sd,1,nameid);/* •s–¾‚È‚Ì‚Å‚Æ‚è‚ ‚¦‚¸»‘¢¸”sƒGƒtƒFƒNƒgƒpƒPƒbƒg */
			clif_misceffect(&sd->bl,2); /* ‘¼l‚É‚à¸”s‚ğ’Ê’m*/
			break;
		}
	}

	return;
}

void skill_arrow_create(struct map_session_data *sd, unsigned short nameid) {
	int i, j, flag, idx;
	struct item tmp_item;

//	nullpo_retv(sd); // checked before to call function

	for(idx = 0; idx < num_skill_arrow_db; idx++) // MAX_SKILL_ARROW_DB -> dynamic now
		if (nameid == skill_arrow_db[idx].nameid)
			break;

	if (idx == num_skill_arrow_db || (j = pc_search_inventory(sd, nameid)) < 0)
		return;

	pc_delitem(sd, j, 1, 0);
	for(i = 0; i < 5; i++) {
		memset(&tmp_item, 0, sizeof(tmp_item));
		tmp_item.nameid = skill_arrow_db[idx].cre_id[i];
		if (tmp_item.nameid <= 0)
			continue;
		tmp_item.amount = skill_arrow_db[idx].cre_amount[i];
		if (tmp_item.amount <= 0)
			continue;
		tmp_item.identify = 1;
		if (battle_config.making_arrow_name_input) {
			tmp_item.card[0] = 0x00fe;
			tmp_item.card[1] = 0;
			*((unsigned long *)(&tmp_item.card[2])) = sd->char_id; /* ƒLƒƒƒ‰ID */
		}
		if ((flag = pc_additem(sd, &tmp_item, tmp_item.amount))) {
			clif_additem(sd, 0, 0, flag);
			map_addflooritem(&tmp_item, tmp_item.amount, sd->bl.m, sd->bl.x, sd->bl.y, NULL, NULL, NULL, sd->bl.id, 0);
		}
	}

	return;
}

/*--------------------------------------------------------------------------------------
 Plagiarism to work on passive damaging skill, and implement Preserve later - [Aalye]
 ------------------------------------------------------------------------------------ */
void skill_copy_skill(struct map_session_data *tsd, short skillid, short skilllv) {

	// don't plagia a known skill
	if (tsd->status.skill[skillid].lv > 0)
		return;

	if (tsd->cloneskill_id && tsd->status.skill[tsd->cloneskill_id].flag == 13) {
		tsd->status.skill[tsd->cloneskill_id].id = 0;
		tsd->status.skill[tsd->cloneskill_id].lv = 0;
		tsd->status.skill[tsd->cloneskill_id].flag = 0; // flag: 0 (normal), 1 (only card), 2-12 (card and skill (skill level +2)), 13 (cloneskill)
	}
	tsd->cloneskill_id = skillid;
	tsd->status.skill[skillid].id = skillid;
	tsd->status.skill[skillid].lv = (pc_checkskill(tsd, RG_PLAGIARISM) > skill_get_max(skillid)) ?
	                                skill_get_max(skillid) : pc_checkskill(tsd, RG_PLAGIARISM);
	tsd->status.skill[skillid].flag = 13; // flag: 0 (normal), 1 (only card), 2-12 (card and skill (skill level +2)), 13 (cloneskill)
	pc_setglobalreg(tsd, "CLONE_SKILL", tsd->cloneskill_id);
	pc_setglobalreg(tsd, "CLONE_SKILL_LV", tsd->status.skill[skillid].lv);
	clif_skillinfoblock(tsd);

	return;
}

/*----------------------------------------------------------------------------
 * ‰Šú‰»Œn
 */

/*
 * •¶š—ñˆ—
 *        ',' ‚Å‹æØ‚Á‚Ä val ‚É–ß‚·
 */
int skill_split_str(char *str, char **val, int num)
{
	int i;

	for (i = 0; i < num && str; i++){
		val[i] = str;
		str = strchr(str, ',');
		if (str)
			*str++ = 0;
	}

	return i;
}

/*
 * •¶š—ñˆ—
 *      ':' ‚Å‹æØ‚Á‚Äatoi‚µ‚Äval‚É–ß‚·
 */
int skill_split_atoi(char *str, int *val)
{
	int i, max = 0;

	for (i = 0; i < MAX_SKILL_LEVEL; i++) {
		if (str) {
			val[i] = max = atoi(str);
			str = strchr(str, ':');
			if (str)
				*str++ = 0;
		} else {
			val[i] = max;
		}
	}

	return i;
}

/*
 * ƒXƒLƒ‹ƒ†ƒjƒbƒg‚Ì”z’uî•ñì¬
 */
void skill_init_unit_layout()
{
	int i, j, size, pos = 0;

	memset(skill_unit_layout, 0, sizeof(skill_unit_layout));
	// ‹éŒ`‚Ìƒ†ƒjƒbƒg”z’u‚ğì¬‚·‚é
	for (i = 0; i <= MAX_SQUARE_LAYOUT; i++) {
		size = i * 2 + 1;
		skill_unit_layout[i].count = size * size;
		for (j = 0; j < size * size; j++) {
			skill_unit_layout[i].dx[j] = (j % size - i);
			skill_unit_layout[i].dy[j] = (j / size - i);
		}
	}
	pos = i;
	// ‹éŒ`ˆÈŠO‚Ìƒ†ƒjƒbƒg”z’u‚ğì¬‚·‚é
	for (i = 0; i < MAX_SKILL_DB; i++) {
		if (!skill_db[i].unit_id[0] || skill_db[i].unit_layout_type[0] != -1)
			continue;
		switch (i) {
			case MG_FIREWALL:
			case WZ_ICEWALL:
				// ƒtƒ@ƒCƒA[ƒEƒH[ƒ‹AƒAƒCƒXƒEƒH[ƒ‹‚Í•ûŒü‚Å•Ï‚í‚é‚Ì‚Å•Êˆ—
				break;
			case PR_SANCTUARY:
			{
				static const int dx[] = {
					-1, 0, 1,-2,-1, 0, 1, 2,-2,-1,
					 0, 1, 2,-2,-1, 0, 1, 2,-1, 0, 1};
				static const int dy[]={
					-2,-2,-2,-1,-1,-1,-1,-1, 0, 0,
					 0, 0, 0, 1, 1, 1, 1, 1, 2, 2, 2};
				skill_unit_layout[pos].count = 21;
				memcpy(skill_unit_layout[pos].dx, dx, sizeof(dx));
				memcpy(skill_unit_layout[pos].dy, dy, sizeof(dy));
				break;
			}
			case PR_MAGNUS:
			{
				static const int dx[] = {
					-1, 0, 1,-1, 0, 1,-3,-2,-1, 0,
					 1, 2, 3,-3,-2,-1, 0, 1, 2, 3,
					-3,-2,-1, 0, 1, 2, 3,-1, 0, 1,-1, 0, 1};
				static const int dy[] = {
					-3,-3,-3,-2,-2,-2,-1,-1,-1,-1,
					-1,-1,-1, 0, 0, 0, 0, 0, 0, 0,
					 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 3, 3, 3};
				skill_unit_layout[pos].count = 33;
				memcpy(skill_unit_layout[pos].dx, dx, sizeof(dx));
				memcpy(skill_unit_layout[pos].dy, dy, sizeof(dy));
				break;
			}
			case AS_VENOMDUST:
			{
				static const int dx[] = {-1, 0, 0, 0, 1};
				static const int dy[] = { 0,-1, 0, 1, 0};
				skill_unit_layout[pos].count = 5;
				memcpy(skill_unit_layout[pos].dx, dx, sizeof(dx));
				memcpy(skill_unit_layout[pos].dy, dy, sizeof(dy));
				break;
			}
			case CR_GRANDCROSS:
			case NPC_DARKGRANDCROSS:
			{
				static const int dx[] = {
					 0, 0,-1, 0, 1,-2,-1, 0, 1, 2,
					-4,-3,-2,-1, 0, 1, 2, 3, 4,-2,
					-1, 0, 1, 2,-1, 0, 1, 0, 0};
				static const int dy[] = {
					-4,-3,-2,-2,-2,-1,-1,-1,-1,-1,
					 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
					 1, 1, 1, 1, 2, 2, 2, 3, 4};
				skill_unit_layout[pos].count = 29;
				memcpy(skill_unit_layout[pos].dx, dx, sizeof(dx));
				memcpy(skill_unit_layout[pos].dy, dy, sizeof(dy));
				break;
			}
			case PF_FOGWALL:
			{
				static const int dx[] = {
					-2,-1, 0, 1, 2,-2,-1, 0, 1, 2,-2,-1, 0, 1, 2};
				static const int dy[] = {
					-1,-1,-1,-1,-1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1};
				skill_unit_layout[pos].count = 15;
				memcpy(skill_unit_layout[pos].dx, dx, sizeof(dx));
				memcpy(skill_unit_layout[pos].dy, dy, sizeof(dy));
				break;
			}
			default:
				printf("unknown unit layout at skill %d\n", i);
				break;
		}
		if (!skill_unit_layout[pos].count)
			continue;
		for (j = 0; j < MAX_SKILL_LEVEL; j++)
			skill_db[i].unit_layout_type[j] = pos;
		pos++;
	}
	// ƒtƒ@ƒCƒ„[ƒEƒH[ƒ‹
	firewall_unit_pos = pos;
	for (i = 0; i < 8; i++) {
		if (i & 1) { /* Î‚ß”z’u */
			skill_unit_layout[pos].count = 5;
			if (i & 0x2) {
				int dx[] = {-1,-1, 0, 0, 1};
				int dy[] = { 1, 0, 0,-1,-1};
				memcpy(skill_unit_layout[pos].dx, dx, sizeof(dx));
				memcpy(skill_unit_layout[pos].dy, dy, sizeof(dy));
			} else {
				int dx[] = { 1, 1 ,0, 0,-1};
				int dy[] = { 1, 0, 0,-1,-1};
				memcpy(skill_unit_layout[pos].dx, dx, sizeof(dx));
				memcpy(skill_unit_layout[pos].dy, dy, sizeof(dy));
			}
		} else {	/* c‰¡”z’u */
			skill_unit_layout[pos].count = 3;
			if (i % 4 == 0) { /* ã‰º */
				int dx[] = {-1, 0, 1};
				int dy[] = { 0, 0, 0};
				memcpy(skill_unit_layout[pos].dx, dx, sizeof(dx));
				memcpy(skill_unit_layout[pos].dy, dy, sizeof(dy));
			} else { /* ¶‰E */
				int dx[] = { 0, 0, 0};
				int dy[] = {-1, 0, 1};
				memcpy(skill_unit_layout[pos].dx, dx, sizeof(dx));
				memcpy(skill_unit_layout[pos].dy, dy, sizeof(dy));
			}
		}
		pos++;
	}
	// ƒAƒCƒXƒEƒH[ƒ‹
	icewall_unit_pos = pos;
	for (i = 0; i < 8; i++) {
		skill_unit_layout[pos].count = 5;
		if (i & 1) { /* Î‚ß”z’u */
			if (i & 0x2) {
				int dx[] = {-2,-1, 0, 1, 2};
				int dy[] = { 2,-1, 0,-1,-2};
				memcpy(skill_unit_layout[pos].dx, dx, sizeof(dx));
				memcpy(skill_unit_layout[pos].dy, dy, sizeof(dy));
			} else {
				int dx[] = { 2, 1 ,0,-1,-2};
				int dy[] = { 2, 1, 0,-1,-2};
				memcpy(skill_unit_layout[pos].dx, dx, sizeof(dx));
				memcpy(skill_unit_layout[pos].dy, dy, sizeof(dy));
			}
		} else { /* c‰¡”z’u */
			if (i % 4 == 0) { /* ã‰º */
				int dx[] = {-2,-1, 0, 1, 2};
				int dy[] = { 0, 0, 0, 0, 0};
				memcpy(skill_unit_layout[pos].dx, dx, sizeof(dx));
				memcpy(skill_unit_layout[pos].dy, dy, sizeof(dy));
			} else {			/* ¶‰E */
				int dx[] = { 0, 0, 0, 0, 0};
				int dy[] = {-2,-1, 0, 1, 2};
				memcpy(skill_unit_layout[pos].dx, dx, sizeof(dx));
				memcpy(skill_unit_layout[pos].dy, dy, sizeof(dy));
			}
		}
		pos++;
	}
}

/*==========================================
 * ƒXƒLƒ‹ŠÖŒWƒtƒ@ƒCƒ‹“Ç‚İ‚İ
 * skill_db.txt ƒXƒLƒ‹ƒf[ƒ^
 * skill_cast_db.txt ƒXƒLƒ‹‚Ì‰r¥ŠÔ‚ÆƒfƒBƒŒƒCƒf[ƒ^
 * produce_db.txt ƒAƒCƒeƒ€ì¬ƒXƒLƒ‹—pƒf[ƒ^
 * create_arrow_db.txt –îì¬ƒXƒLƒ‹—pƒf[ƒ^
 * abra_db.txt ƒAƒuƒ‰ƒJƒ_ƒuƒ‰”­“®ƒXƒLƒ‹ƒf[ƒ^
 *------------------------------------------
 */
int skill_readdb(void) {
	int i,j,k,l,m;
	FILE *fp;
	char line[1024],*p;
	char *filename[] = {"db/produce_db.txt","db/produce_db2.txt"};

	/* ƒXƒLƒ‹ƒf[ƒ^ƒx[ƒX */
	memset(skill_db, 0, sizeof(skill_db));
	fp=fopen("db/skill_db.txt","r");
	if(fp==NULL){
		printf("can't read db/skill_db.txt\n");
		return 1;
	}
	while(fgets(line, sizeof(line), fp)) { // fgets reads until maximum one less than size and add '\0' -> so, it's not necessary to add -1
		char *split[50];
		if ((line[0] == '/' && line[1] == '/') || line[0] == '\0' || line[0] == '\n' || line[0] == '\r')
			continue;
		// it's not necessary to remove 'carriage return ('\n' or '\r')
		memset(split,0,sizeof(split));
		j = skill_split_str(line, split, 14);
		if (split[13] == NULL || j < 14)
			continue;

		i = atoi(split[0]);
		if (i >= 10000 && i < 10015) // for guild skills [Celest]
			i -= 9500;
		else if (i <= 0 || i > MAX_SKILL_DB)
			continue;

/*		printf("skill id=%d\n",i); */
		skill_split_atoi(split[1], skill_db[i].range);
		skill_db[i].hit=atoi(split[2]);
		skill_db[i].inf=atoi(split[3]);
		skill_db[i].pl=atoi(split[4]);
		skill_db[i].nk=atoi(split[5]);
		skill_db[i].max=atoi(split[6]);
		skill_split_atoi(split[7], skill_db[i].num);

		if (strcasecmp(split[8],"yes") == 0)
			skill_db[i].castcancel=1;
		else
			skill_db[i].castcancel=0;
		skill_db[i].cast_def_rate=atoi(split[9]);
		skill_db[i].inf2=atoi(split[10]);
		skill_db[i].maxcount=atoi(split[11]);
		if (strcasecmp(split[12],"weapon") == 0)
			skill_db[i].skill_type=BF_WEAPON;
		else if (strcasecmp(split[12],"magic") == 0)
			skill_db[i].skill_type=BF_MAGIC;
		else if (strcasecmp(split[12],"misc") == 0)
			skill_db[i].skill_type=BF_MISC;
		else
			skill_db[i].skill_type=0;
		skill_split_atoi(split[13], skill_db[i].blewcount);
	}
	fclose(fp);
	printf("DB '" CL_WHITE "db/skill_db.txt" CL_RESET "' readed.\n");

	fp=fopen("db/skill_require_db.txt","r");
	if(fp==NULL){
		printf("can't read db/skill_require_db.txt\n");
		return 1;
	}
	while(fgets(line, sizeof(line), fp)) { // fgets reads until maximum one less than size and add '\0' -> so, it's not necessary to add -1
		char *split[50];
		if ((line[0] == '/' && line[1] == '/') || line[0] == '\0' || line[0] == '\n' || line[0] == '\r')
			continue;
		// it's not necessary to remove 'carriage return ('\n' or '\r')
		memset(split, 0, sizeof(split));
		j = skill_split_str(line, split, 30);
		if(split[29]==NULL || j<30)
			continue;

		i=atoi(split[0]);
		if (i>=10000 && i<10015) // for guild skills [Celest]
			i -= 9500;
		else if(i<=0 || i>MAX_SKILL_DB)
			continue;

		skill_split_atoi(split[1], skill_db[i].hp);
		skill_split_atoi(split[2], skill_db[i].mhp);
		skill_split_atoi(split[3], skill_db[i].sp);
		skill_split_atoi(split[4], skill_db[i].hp_rate);
		skill_split_atoi(split[5], skill_db[i].sp_rate);
		skill_split_atoi(split[6], skill_db[i].zeny);

		p = split[7];
		for(j = 0; j < 32; j++) {
			l = atoi(p);
			if (l == 99) {
				skill_db[i].weapon = 0xffffffff;
				break;
			}
			else
				skill_db[i].weapon |= 1 << l;
			p = strchr(p, ':');
			if (!p)
				break;
			p++;
		}

		if ( strcasecmp(split[8], "hiding")==0 ) skill_db[i].state=ST_HIDING;
		else if ( strcasecmp(split[8], "cloaking")==0 ) skill_db[i].state=ST_CLOAKING;
		else if ( strcasecmp(split[8], "hidden")==0 ) skill_db[i].state=ST_HIDDEN;
		else if ( strcasecmp(split[8], "riding")==0 ) skill_db[i].state=ST_RIDING;
		else if ( strcasecmp(split[8], "falcon")==0 ) skill_db[i].state=ST_FALCON;
		else if ( strcasecmp(split[8], "cart")==0 ) skill_db[i].state=ST_CART;
		else if ( strcasecmp(split[8], "shield")==0 ) skill_db[i].state=ST_SHIELD;
		else if ( strcasecmp(split[8], "sight")==0 ) skill_db[i].state=ST_SIGHT;
		else if ( strcasecmp(split[8], "explosionspirits")==0 ) skill_db[i].state=ST_EXPLOSIONSPIRITS;
		else if ( strcasecmp(split[8], "cartboost")==0 ) skill_db[i].state=ST_CARTBOOST;
		else if ( strcasecmp(split[8], "recover_weight_rate")==0 ) skill_db[i].state=ST_RECOV_WEIGHT_RATE;
		else if ( strcasecmp(split[8], "move_enable")==0 ) skill_db[i].state=ST_MOVE_ENABLE;
		else if ( strcasecmp(split[8], "water")==0 ) skill_db[i].state=ST_WATER;
		else skill_db[i].state = ST_NONE;

		skill_split_atoi(split[9], skill_db[i].spiritball);
		skill_db[i].itemid[0]=atoi(split[10]);
		skill_db[i].amount[0]=atoi(split[11]);
		skill_db[i].itemid[1]=atoi(split[12]);
		skill_db[i].amount[1]=atoi(split[13]);
		skill_db[i].itemid[2]=atoi(split[14]);
		skill_db[i].amount[2]=atoi(split[15]);
		skill_db[i].itemid[3]=atoi(split[16]);
		skill_db[i].amount[3]=atoi(split[17]);
		skill_db[i].itemid[4]=atoi(split[18]);
		skill_db[i].amount[4]=atoi(split[19]);
		skill_db[i].itemid[5]=atoi(split[20]);
		skill_db[i].amount[5]=atoi(split[21]);
		skill_db[i].itemid[6]=atoi(split[22]);
		skill_db[i].amount[6]=atoi(split[23]);
		skill_db[i].itemid[7]=atoi(split[24]);
		skill_db[i].amount[7]=atoi(split[25]);
		skill_db[i].itemid[8]=atoi(split[26]);
		skill_db[i].amount[8]=atoi(split[27]);
		skill_db[i].itemid[9]=atoi(split[28]);
		skill_db[i].amount[9]=atoi(split[29]);
	}
	fclose(fp);
	printf("DB '" CL_WHITE "db/skill_require_db.txt" CL_RESET "' readed.\n");

	/* ƒLƒƒƒXƒeƒBƒ“ƒOƒf[ƒ^ƒx[ƒX */
	fp=fopen("db/skill_cast_db.txt","r");
	if(fp==NULL){
		printf("can't read db/skill_cast_db.txt\n");
		return 1;
	}
	while(fgets(line, sizeof(line), fp)) { // fgets reads until maximum one less than size and add '\0' -> so, it's not necessary to add -1
		char *split[50];
		if ((line[0] == '/' && line[1] == '/') || line[0] == '\0' || line[0] == '\n' || line[0] == '\r')
			continue;
		// it's not necessary to remove 'carriage return ('\n' or '\r')
		memset(split, 0, sizeof(split));
		j = skill_split_str(line, split, 5);
		if(split[4]==NULL || j<5)
			continue;

		i=atoi(split[0]);
		if (i>=10000 && i<10015) // for guild skills [Celest]
			i -= 9500;
		else if(i<=0 || i>MAX_SKILL_DB)
			continue;

		skill_split_atoi(split[1], skill_db[i].cast);
		skill_split_atoi(split[2], skill_db[i].delay);
		skill_split_atoi(split[3], skill_db[i].upkeep_time);
		skill_split_atoi(split[4], skill_db[i].upkeep_time2);
	}
	fclose(fp);
	printf("DB '" CL_WHITE "db/skill_cast_db.txt" CL_RESET "' readed.\n");

	/* ƒXƒLƒ‹ƒ†ƒjƒbƒgƒf[ƒ^ƒx[ƒX */
	fp = fopen("db/skill_unit_db.txt", "r");
	if (fp == NULL) {
		printf("can't read db/skill_unit_db.txt\n");
		return 1;
	}
	while (fgets(line, sizeof(line), fp)) { // fgets reads until maximum one less than size and add '\0' -> so, it's not necessary to add -1
		char *split[50];
		if ((line[0] == '/' && line[1] == '/') || line[0] == '\0' || line[0] == '\n' || line[0] == '\r')
			continue;
		// it's not necessary to remove 'carriage return ('\n' or '\r')
		memset(split, 0, sizeof(split));
		j = skill_split_str(line, split, 8);
		if (split[7] == NULL || j < 8)
			continue;

		i = atoi(split[0]);
		if (i >= 10000 && i < 10015) // for guild skills [Celest]
			i -= 9500;
		else if (i <= 0 || i > MAX_SKILL_DB)
			continue;
		skill_db[i].unit_id[0] = strtol(split[1], NULL, 16);
		skill_db[i].unit_id[1] = strtol(split[2], NULL, 16);
		skill_split_atoi(split[3], skill_db[i].unit_layout_type);
		skill_db[i].unit_range = atoi(split[4]);
		skill_db[i].unit_interval = atoi(split[5]);

		if (strcasecmp(split[6], "noenemy") == 0) skill_db[i].unit_target = BCT_NOENEMY;
		else if (strcasecmp(split[6], "friend") == 0) skill_db[i].unit_target = BCT_NOENEMY;
		else if (strcasecmp(split[6], "party") == 0) skill_db[i].unit_target = BCT_PARTY;
		else if (strcasecmp(split[6], "all") == 0) skill_db[i].unit_target = BCT_ALL;
		else if (strcasecmp(split[6], "enemy") == 0) skill_db[i].unit_target = BCT_ENEMY;
		else if (strcasecmp(split[6], "self") == 0) skill_db[i].unit_target = BCT_SELF;
		else skill_db[i].unit_target = strtol(split[6], NULL, 16);

		skill_db[i].unit_flag = strtol(split[7], NULL, 16);
		k++;
	}
	fclose(fp);
	printf("DB '" CL_WHITE "db/skill_unit_db.txt" CL_RESET "' readed.\n");

	skill_init_unit_layout();

	/* »‘¢ŒnƒXƒLƒ‹ƒf[ƒ^ƒx[ƒX */
	memset(skill_produce_db,0,sizeof(skill_produce_db));
	for(m = 0; m < 2; m++) {
		fp=fopen(filename[m],"r");
		if(fp==NULL){
			if(m>0)
				continue;
			printf("can't read %s\n",filename[m]);
			return 1;
		}
		k=0;
		while(fgets(line, sizeof(line), fp)) { // fgets reads until maximum one less than size and add '\0' -> so, it's not necessary to add -1
			char *split[6 + MAX_PRODUCE_RESOURCE * 2];
			int x, y;
			if ((line[0] == '/' && line[1] == '/') || line[0] == '\0' || line[0] == '\n' || line[0] == '\r')
				continue;
			// it's not necessary to remove 'carriage return ('\n' or '\r')
			memset(split,0,sizeof(split));
			j = skill_split_str(line, split, (3 + MAX_PRODUCE_RESOURCE * 2));
			if (split[0] == 0)
				continue;
			i=atoi(split[0]);
			if(i<=0)
				continue;

			skill_produce_db[k].nameid=i;
			skill_produce_db[k].itemlv=atoi(split[1]);
			skill_produce_db[k].req_skill=atoi(split[2]);

			for(x=3,y=0; split[x] && split[x+1] && y<MAX_PRODUCE_RESOURCE; x+=2,y++){
				skill_produce_db[k].mat_id[y]=atoi(split[x]);
				skill_produce_db[k].mat_amount[y]=atoi(split[x+1]);
			}
			k++;
			if(k >= MAX_SKILL_PRODUCE_DB)
				break;
		}
		fclose(fp);
		printf("DB '" CL_WHITE "%s" CL_RESET "' readed ('" CL_WHITE "%d" CL_RESET "' entrie%s).\n", filename[m], k, (k > 1) ? "s" : "");
	}

	num_skill_arrow_db = 0;
	FREE(skill_arrow_db);
	fp = fopen("db/create_arrow_db.txt", "r");
	if (fp == NULL) {
		printf("can't read db/create_arrow_db.txt\n");
		return 1;
	}
	while(fgets(line, sizeof(line), fp)) { // fgets reads until maximum one less than size and add '\0' -> so, it's not necessary to add -1
		char *split[16];
		int x, y;
		if ((line[0] == '/' && line[1] == '/') || line[0] == '\0' || line[0] == '\n' || line[0] == '\r')
			continue;
		// it's not necessary to remove 'carriage return ('\n' or '\r')
		memset(split, 0, sizeof(split));
		j = skill_split_str(line, split, 13);
		if (split[0] == 0 || j < 3) // at least 'SourceID,MakeID1,MakeNum1'
			continue;
		i = atoi(split[0]);
		if (i <= 0)
			continue;

		if (num_skill_arrow_db == 0) {
			CALLOC(skill_arrow_db, struct skill_arrow_db, 1);
		} else {
			REALLOC(skill_arrow_db, struct skill_arrow_db, num_skill_arrow_db + 1);
			memset(skill_arrow_db + num_skill_arrow_db, 0, sizeof(struct skill_arrow_db)); // need for the list of created items
		}

		skill_arrow_db[num_skill_arrow_db].nameid = i;

		y = 0;
		for(x = 1; split[x] && split[x + 1] && y < 5; x += 2) {
			if (atoi(split[x]) > 0 && atoi(split[x + 1]) > 0) {
				skill_arrow_db[num_skill_arrow_db].cre_id[y] = atoi(split[x]);
				skill_arrow_db[num_skill_arrow_db].cre_amount[y] = atoi(split[x + 1]);
				y++;
			}
		}
		num_skill_arrow_db++;
	}
	fclose(fp);
	printf("DB '" CL_WHITE "db/create_arrow_db.txt" CL_RESET "' readed ('" CL_WHITE "%d" CL_RESET "' entrie%s).\n", num_skill_arrow_db, (num_skill_arrow_db > 1) ? "s" : "");

	memset(skill_abra_db, 0, sizeof(skill_abra_db));
	fp=fopen("db/abra_db.txt","r");
	if(fp==NULL){
		printf("can't read db/abra_db.txt\n");
		return 1;
	}
	k=0;
	while(fgets(line, sizeof(line), fp)) { // fgets reads until maximum one less than size and add '\0' -> so, it's not necessary to add -1
		char *split[16];
		if ((line[0] == '/' && line[1] == '/') || line[0] == '\0' || line[0] == '\n' || line[0] == '\r')
			continue;
		// it's not necessary to remove 'carriage return ('\n' or '\r')
		memset(split,0,sizeof(split));
		j = skill_split_str(line, split, 13);
		if (split[0] == 0)
			continue;
		i=atoi(split[0]);
		if(i<=0)
			continue;

		skill_abra_db[i].req_lv=atoi(split[2]);
		skill_abra_db[i].per=atoi(split[3]);

		k++;
		if(k >= MAX_SKILL_ABRA_DB)
			break;
	}
	fclose(fp);
	printf("DB '" CL_WHITE "db/abra_db.txt" CL_RESET "' readed ('" CL_WHITE "%d" CL_RESET "' entrie%s).\n", k, (k > 1) ? "s" : "");

	fp=fopen("db/skill_castnodex_db.txt","r");
	if(fp==NULL){
		printf("can't read db/skill_castnodex_db.txt\n");
		return 1;
	}
	while(fgets(line, sizeof(line), fp)) { // fgets reads until maximum one less than size and add '\0' -> so, it's not necessary to add -1
		char *split[50];
		if ((line[0] == '/' && line[1] == '/') || line[0] == '\0' || line[0] == '\n' || line[0] == '\r')
			continue;
		// it's not necessary to remove 'carriage return ('\n' or '\r')
		memset(split, 0, sizeof(split));
		j = skill_split_str(line, split, 3);
		if (split[0] == 0)
			continue;

		i = atoi(split[0]);
		if (i >= 10000 && i < 10015) // for guild skills [Celest]
			i -= 9500;
		else if (i <= 0 || i > MAX_SKILL_DB)
			continue;

		skill_split_atoi(split[1], skill_db[i].castnodex);
		if (split[2] == 0)
			continue;
		skill_split_atoi(split[2], skill_db[i].delaynodex);
	}
	fclose(fp);
	printf("DB '" CL_WHITE "db/skill_castnodex_db.txt" CL_RESET "' readed.\n");

	fp = fopen("db/skill_nocast_db.txt","r");
	if (fp == NULL) {
		printf("can't read db/skill_nocast_db.txt\n");
		return 1;
	}
	k = 0;
	while(fgets(line, sizeof(line), fp)) { // fgets reads until maximum one less than size and add '\0' -> so, it's not necessary to add -1
		char *split[16];
		if ((line[0] == '/' && line[1] == '/') || line[0] == '\0' || line[0] == '\n' || line[0] == '\r')
			continue;
		// it's not necessary to remove 'carriage return ('\n' or '\r')
		memset(split, 0, sizeof(split));
		j = skill_split_str(line, split, 2);
		if (split[0] == 0)
			continue;
		i=atoi(split[0]);
		if (i>=10000 && i<10015) // for guild skills [Celest]
			i -= 9500;
		else if(i<=0 || i>MAX_SKILL_DB)
			continue;
		skill_db[i].nocast=atoi(split[1]);
		k++;
	}
	fclose(fp);
	printf("DB '" CL_WHITE "db/skill_nocast_db.txt" CL_RESET "' readed ('" CL_WHITE "%d" CL_RESET "' entrie%s).\n", k, (k > 1) ? "s" : "");

	return 0;
}

/*===============================================
 * For reading leveluseskillspamount.txt [Celest]
 *-----------------------------------------------
 */
static int skill_read_skillspamount(void) {
	char *buf, *p;
	struct skill_db *skill = NULL;
	int s, idx, new_flag = 1, level = 1, sp = 0;

	buf = grfio_reads("data\\leveluseskillspamount.txt", &s);

	if (buf == NULL)
		return -1;

	buf[s] = 0;
	p = buf;
	while(p - buf < s) {
		char buf2[64];

		if (sscanf(p, "%[@]", buf2) == 1) {
			level = 1;
			new_flag = 1;
		} else if (new_flag && sscanf(p, "%[^#]#", buf2) == 1) {
			for (idx = 0; skill_names[idx].id != 0; idx++) {
				if (strstr(buf2, skill_names[idx].name) != NULL) {
					skill = &skill_db[skill_names[idx].id];
					new_flag = 0;
					break;
				}
			}
		} else if (!new_flag && sscanf(p, "%d#", &sp) == 1) {
			skill->sp[level-1] = sp;
			level++;
		}

		p = strchr(p, 10);
		if (!p)
			break;
		p++;
	}
	FREE(buf);

	printf("File '" CL_WHITE "data\\leveluseskillspamount.txt" CL_RESET "' readed.\n");

	return 0;
}

void skill_reload(void) {
	/*
	<empty skill database>
	<?>
	*/

	do_init_skill();
}

/*==========================================
 * ƒXƒLƒ‹ŠÖŒW‰Šú‰»ˆ—
 *------------------------------------------
 */
int do_init_skill(void) {
	skill_readdb();
	if (battle_config.skill_sp_override_grffile)
		skill_read_skillspamount();

	add_timer_func_list(skill_unit_timer, "skill_unit_timer");
	add_timer_func_list(skill_castend_id, "skill_castend_id");
	add_timer_func_list(skill_castend_pos, "skill_castend_pos");
	add_timer_func_list(skill_timerskill, "skill_timerskill");

	add_timer_interval(gettick_cache + SKILLUNITTIMER_INVERVAL, skill_unit_timer, 0, 0, SKILLUNITTIMER_INVERVAL);

	return 0;
}
