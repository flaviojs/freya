// $Id: battle.h 659 2006-06-15 20:25:42Z DarkRaven $
#ifndef _BATTLE_H_
#define _BATTLE_H_

// MATK_FIX is used only for magical attack calculation
#define MATK_FIX(a, b) { matk1 = matk1 * (a) / (b); matk2 = matk2 * (a) / (b); }

struct Damage {
	int damage,damage2;
	int type,div_;
	int amotion,dmotion;
	int blewcount;
	int flag;
	int dmg_lv;
};

extern int attr_fix_table[4][10][10];

struct map_session_data;
struct mob_data;
struct block_list;

struct Damage battle_calc_attack(	int attack_type,
	struct block_list *bl,struct block_list *target,int skill_num,int skill_lv,int flag);
struct Damage battle_calc_weapon_attack(
	struct block_list *bl,struct block_list *target,int skill_num,int skill_lv,int flag);
struct Damage battle_calc_magic_attack(
	struct block_list *bl,struct block_list *target,int skill_num,int skill_lv,int flag);
struct Damage  battle_calc_misc_attack(
	struct block_list *bl,struct block_list *target,int skill_num,int skill_lv,int flag);

int battle_attr_fix(int damage,int atk_elem,int def_elem);

int battle_calc_damage(struct block_list *src, struct block_list *bl, int damage, int div_, int skill_num, int skill_lv, int flag); // Calculate Damage
int battle_calc_rdamage(struct block_list *bl, int damage, int damage_type, int attack_type, int skill_id); // Calculate Reflection Damage

enum
{
	BF_WEAPON    = 0x0001,
	BF_MAGIC     = 0x0002,
	BF_MISC      = 0x0004,
	BF_SHORT     = 0x0010,
	BF_LONG      = 0x0040,
	BF_SKILL     = 0x0100,
	BF_NORMAL    = 0x0200,
	BF_WEAPONMASK= 0x000f,
	BF_RANGEMASK = 0x00f0,
	BF_SKILLMASK = 0x0f00
};

// é¿ç€Ç…HPÇëùå∏
int battle_delay_damage (unsigned int tick, struct block_list *src, struct block_list *target, int attack_type, int skill_id, int skill_lv, int damage, int dmg_lv, int flag);
int battle_damage(struct block_list *bl, struct block_list *target, int damage, int flag);
int battle_heal(struct block_list *bl, struct block_list *target, int hp, int sp, int flag);

// çUåÇÇ‚à⁄ìÆÇé~ÇﬂÇÈ
void battle_stopattack(struct block_list *bl);
int battle_stopwalking(struct block_list *bl, int type);

// í èÌçUåÇèàóùÇ‹Ç∆Çﬂ
int battle_weapon_attack(struct block_list *bl, struct block_list *target,
	 unsigned int tick, int flag);

// äeéÌÉpÉâÉÅÅ[É^ÇìæÇÈ
int battle_counttargeted(struct block_list *bl, struct block_list *src, int target_lv);

enum {
	BCT_NOENEMY = 0x00000,
	BCT_PARTY   = 0x10000,
	BCT_ENEMY   = 0x40000,
	BCT_NOPARTY = 0x50000,
	BCT_ALL     = 0x20000,
	BCT_NOONE   = 0x60000,
	BCT_SELF    = 0x60000
};

int battle_check_undead(int race, int element);
int battle_check_target(struct block_list *src, struct block_list *target, int flag);
int battle_check_range(struct block_list *src, struct block_list *bl, int range);


// ê›íË

extern struct Battle_Config {
	int warp_point_debug;
	int enemy_critical;
	int enemy_critical_rate;
	int enemy_str;
	int enemy_perfect_flee;
	int cast_rate,delay_rate,delay_dependon_dex;
	int sdelay_attack_enable;
	int left_cardfix_to_right;
	int pc_skill_add_range;
	int skill_out_range_consume;
	int mob_skill_add_range;
	int pc_damage_delay;
	int pc_damage_delay_rate;
	int defnotenemy;
	int random_monster_checklv;
	int attr_recover;
	int flooritem_lifetime;
	int item_auto_get;
	int item_auto_get_loot;
	int item_auto_get_distance;
	int item_first_get_time;
	int item_second_get_time;
	int item_third_get_time;
	int mvp_item_first_get_time;
	int mvp_item_second_get_time;
	int mvp_item_third_get_time;
	int base_exp_rate, job_exp_rate;
	int drop_rate0item;
	int death_penalty_type_base;
	int death_penalty_base;
	int death_penalty_base2_lvl, death_penalty_base2;
	int death_penalty_base3_lvl, death_penalty_base3;
	int death_penalty_base4_lvl, death_penalty_base4;
	int death_penalty_base5_lvl, death_penalty_base5;
	int death_penalty_type_job;
	int death_penalty_job;
	int death_penalty_job2_lvl, death_penalty_job2;
	int death_penalty_job3_lvl, death_penalty_job3;
	int death_penalty_job4_lvl, death_penalty_job4;
	int death_penalty_job5_lvl, death_penalty_job5;
	int pvp_exp; // [MouseJstr]
	int gtb_pvp_only; // [MouseJstr]
	int zeny_penalty;
	int zeny_penalty_percent;
	int zeny_penalty_by_lvl;
	int restart_hp_rate;
	int restart_sp_rate;
	int mvp_item_rate,mvp_exp_rate;
	int mvp_hp_rate;
	int monster_hp_rate;
	int monster_max_aspd;
	int atc_gmonly;
	int atc_spawn_quantity_limit;
	int atc_map_mob_limit;
	int atc_local_spawn_interval;
	int npc_summon_type;
	int gm_allskill;
	int gm_all_skill_job;
	int gm_all_skill_platinum;
	int gm_allskill_addabra;
	int gm_allequip;
	int gm_skilluncond;
	int skillfree;
	int skillup_limit;
	int check_minimum_skill_points;
	int check_maximum_skill_points;
	int wp_rate;
	int pp_rate;
	int cdp_rate;
	int monster_active_enable;
	int monster_damage_delay_rate;
	int monster_loot_type;
	int mob_skill_use;
	int mob_count_rate;
	int quest_skill_learn;
	int quest_skill_reset;
	int basic_skill_check;
	int no_caption_cloaking; // remove the caption "cloaking !!" when skill is casted
	int display_hallucination; // Enable or disable hallucination Skill.
	int no_guilds_glory;
	int guild_emperium_check;
	int guild_exp_limit;
	int guild_max_castles;
	int pc_invincible_time;
	int pet_birth_effect;
	int pet_catch_rate;
	int pet_rename;
	int pet_friendly_rate;
	int pet_hungry_delay_rate;
	int pet_hungry_friendly_decrease;
	int pet_str;
	int pet_status_support;
	int pet_attack_support;
	int pet_damage_support;
	int pet_support_rate;
	int pet_attack_exp_to_master;
	int pet_attack_exp_rate;
	int skill_min_damage;
	int finger_offensive_type;
	int heal_exp;
	int resurrection_exp;
	int shop_exp;
	int combo_delay_rate;
	int item_check;
	int item_use_interval; // [Skotlex]
	int wedding_modifydisplay;
	int natural_healhp_interval;
	int natural_healsp_interval;
	int natural_heal_skill_interval;
	int natural_heal_weight_rate;
	int item_name_override_grffile;
	int indoors_override_grffile; // [Celest]
	int skill_sp_override_grffile; // [Celest]
	int cardillust_read_grffile;
	int item_equip_override_grffile;
	int item_slots_override_grffile;
	int arrow_decrement;
	int max_aspd;
	int max_hp;
	int max_sp;
	int max_lv;
	int max_parameter;
	int max_cart_weight;
	int pc_skill_log;
	int mob_skill_log;
	int battle_log;
	int save_log;
	int error_log;
	int etc_log;
	int save_clothcolor;
	int undead_detect_type;
	int pc_auto_counter_type;
	int monster_auto_counter_type;
	int agi_penalty_type;
	int agi_penalty_count;
	int agi_penalty_num;
	int vit_penalty_type;
	int vit_penalty_count;
	int vit_penalty_num;
	int player_defense_type;
	int monster_defense_type;
	int pet_defense_type;
	int magic_defense_type;
	int pc_skill_reiteration;
	int monster_skill_reiteration;
	int pc_skill_nofootset;
	int monster_skill_nofootset;
	int pc_cloak_check_type;
	int monster_cloak_check_type;
	int gvg_short_damage_rate;
	int gvg_long_damage_rate;
	int gvg_magic_damage_rate;
	int gvg_misc_damage_rate;
	int gvg_eliminate_time;
	int mob_changetarget_byskill;
	int pc_attack_direction_change;
	int monster_attack_direction_change;
	int pc_land_skill_limit;
	int monster_land_skill_limit;
	int party_skill_penalty;
	int monster_class_change_full_recover;
	int produce_item_name_input;
	int produce_potion_name_input;
	int making_arrow_name_input;
	int holywater_name_input;
	int atcommand_item_creation_name_input;
	int atcommand_max_player_gm_level;
	int display_delay_skill_fail;
	int display_snatcher_skill_fail;
	int chat_warpportal;
	int mob_warpportal;
	int dead_branch_active;
	int vending_max_value;
//	int pet_lootitem; // removed
//	int pet_weight; // removed
	int show_steal_in_same_party;
	int enable_upper_class;
	int pet_attack_attr_none;
	int mob_attack_attr_none;
	int mob_ghostring_fix;
	int pc_attack_attr_none;
	int item_rate_common, item_rate_card, item_rate_equip, item_rate_heal, item_rate_use;
	int item_drop_common_min, item_drop_common_max;
	int item_drop_card_min, item_drop_card_max;
	int item_drop_equip_min, item_drop_equip_max;
	int item_drop_mvp_min, item_drop_mvp_max;
	int item_drop_heal_min, item_drop_heal_max;
	int item_drop_use_min, item_drop_use_max;

	int prevent_logout; // Added by RoVeRT

	int alchemist_summon_reward;	// [Valaris]
	int maximum_level;
	int atcommand_max_job_level_novice;
	int atcommand_max_job_level_job1;
	int atcommand_max_job_level_job2;
	int atcommand_max_job_level_supernovice;
	int atcommand_max_job_level_highnovice;
	int atcommand_max_job_level_highjob1;
	int atcommand_max_job_level_highjob2;
	int atcommand_max_job_level_babynovice;
	int atcommand_max_job_level_babyjob1;
	int atcommand_max_job_level_babyjob2;
	int atcommand_max_job_level_superbaby;
	int drops_by_luk;
	int monsters_ignore_gm;
	int equipment_breaking;
	int equipment_break_rate;
	int pet_equip_required;
	int multi_level_up;
	int pk_mode;
	int show_mob_hp;  // end additions [Valaris]

	int agi_penalty_count_lv;
	int vit_penalty_count_lv;

	int gx_allhit;
	int gx_cardfix;
	int gx_dupele;
	int gx_disptype;
	int devotion_level_difference;
	int player_skill_partner_check;
	int hide_GM_session; // minimum level of hidden GMs
	int unit_movement_type;
	int invite_request_check;
	int skill_removetrap_type;
	int disp_experience;
	int disp_experience_type;
	int castle_defense_rate;
	int riding_weight;
	int backstab_bow_penalty;
	int hp_rate;
	int sp_rate;
	int gm_can_drop_lv;
	int disp_hpmeter;
	int bone_drop;

	int night_at_start; // added by [Yor]
	int day_duration; // added by [Yor]
	int night_duration; // added by [Yor]
	int ban_spoof_namer; // added by [Yor]
	int ban_hack_trade; // added by [Yor]
	int ban_bot; // added by [Yor]
	int check_ban_bot; // added by [Yor]
	int max_message_length; // added by [Yor]
	int max_global_message_length; // added by [Yor]
	int hack_info_GM_level; // added by [Yor]
	int speed_hack_info_GM_level; // added by [Yor]
	int any_warp_GM_min_level; // added by [Yor]
	int packet_ver_flag; // added by [Yor]
	int muting_players; // added by [PoW]

	int min_hair_style; // added by [Yor]
	int max_hair_style; // added by [Yor]
	int min_hair_color; // added by [Yor]
	int max_hair_color; // added by [Yor]
	int min_cloth_color; // added by [Yor]
	int max_cloth_color; // added by [Yor]
	int clothes_color_for_assassin; // added by [Yor]

	int castrate_dex_scale; // added by [Yor]
	int area_size; // added by [Yor]

	int zeny_from_mobs; // [Valaris]
	int mobs_level_up; // [Valaris]
	int pk_min_level; // [celest]
	int skill_steal_type; // [celest]
	int skill_steal_rate; // [celest]
	int night_darkness_level; // [celest]
	int skill_range_leniency; // [celest]
	int motd_type; // [celest]
	int allow_atcommand_when_mute; // [celest]
	int manner_action; // [Yor]
	int finding_ore_rate;
	int min_skill_delay_limit;
	int idle_no_share; // exp share in party
	int idle_delay_no_share; // exp share in party
	int chat_no_share; // exp share in party
	int npc_chat_no_share; // exp share in party
	int shop_no_share; // exp share in party
	int trade_no_share; // exp share in party
	int idle_disconnect; // disconnection without sending information
	int idle_disconnect_chat; // disconnection without sending information
	int idle_disconnect_vender; // disconnection without sending information
	int idle_disconnect_disable_for_restore; // disconnection without sending information
	int idle_disconnect_ignore_GM; // disconnection without sending information
	int jail_message; // Do we send message to ALL players when a player is put in jail?
	int jail_discharge_message; // Do we send message to ALL players when a player is discharged?
	int mingmlvl_message; // Which message do we send when a GM can use a command, but mingmlvl map flag block it?
	int check_invalid_slot; // Do we check invalid slotted cards?
	int ruwach_range; // Set the range (number of squares/tiles around you) of 'ruwach' skill to detect invisible.
	int sight_range; // Set the range (number of squares/tiles around you) of 'sight' skill to detect invisible.
	int max_icewall; // Set maximum number of ice walls active at the same time.

	int atcommand_main_channel_at_start;
	int atcommand_main_channel_when_woe;
	int atcommand_min_GM_level_for_request;
	int atcommand_follow_stop_dead_target;
	int atcommand_add_local_message_info;
	int atcommand_storage_on_pvp_map;
	int atcommand_gstorage_on_pvp_map;

	int pm_gm_not_ignored; // GM minimum level to be not ignored in private message. [BeoWulf] (from freya's bug report)
	int char_disconnect_mode;
	int auto_muting;
	int full_castle_name;
	int mob_skill_delay;
	int mob_skill_success_chance;
	int item_sex_check;

	int extra_system_flag;

#ifdef USE_SQL /* SQL-only options */
	int mail_system; // [Valaris]
#endif /* USE_SQL */
} battle_config;

int battle_config_read(const char *cfgName);
extern int battle_set_value(char *, char *);

#endif // _BATTLE_H_
