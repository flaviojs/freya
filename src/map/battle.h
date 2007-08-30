// Copyright (c) Freya Development Team - Licensed under GNU GPL
// For more information, see License.txt in the main folder

#ifndef _BATTLE_H_
#define _BATTLE_H_

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
	struct block_list *bl,struct block_list *target,struct map_session_data *sd,int tick,int skill_num,int skill_lv,int flag);
struct Damage battle_calc_magic_attack(
	struct block_list *bl,struct block_list *target,int tick,int skill_num,int skill_lv,int flag);
struct Damage  battle_calc_misc_attack(
	struct block_list *bl,struct block_list *target,struct map_session_data *sd,int skill_num,int skill_lv,int flag);

int battle_attr_fix(int damage,int atk_elem,int def_elem);


int battle_calc_damage(struct block_list *src,struct block_list *bl,int damage,int div_,int skill_num,int skill_lv,int flag);
int battle_calc_drain(int damage, int rate, int per, int val);
enum {
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

int battle_delay_damage (unsigned int tick, struct block_list *src, struct block_list *target, int attack_type, int skill_id, int skill_lv, int damage, int dmg_lv, int flag);
int battle_damage(struct block_list *bl, struct block_list *target, int damage, int flag);
int battle_heal(struct block_list *bl, struct block_list *target, int hp, int sp, int flag);

void battle_stopattack(struct block_list *bl);
void battle_stopwalking(struct block_list *bl, int type);

int battle_weapon_attack(struct block_list *bl, struct block_list *target,
	 unsigned int tick, int flag);

int battle_counttargeted(struct block_list *bl, struct block_list *src, int target_lv);
int battle_getcurrentskill(struct block_list *bl);

#define BCT_ENEMY 0x020000
#define BCT_NOENEMY 0x1d0000
#define BCT_PARTY	0x040000
#define BCT_NOPARTY 0x1b0000
#define BCT_GUILD	0x080000
#define BCT_NOGUILD 0x170000
#define BCT_ALL 0x1f0000
#define BCT_NOONE 0x000000
#define BCT_SELF 0x010000
#define BCT_NEUTRAL 0x100000

int battle_check_undead(int race, int element);
int battle_check_target(struct block_list *src, struct block_list *target, int flag);
int battle_check_range(struct block_list *src, struct block_list *bl, int range);


extern struct Battle_Config {
	unsigned short warp_point_debug;
	unsigned short enemy_critical;
	unsigned short enemy_critical_rate;
	unsigned short enemy_str;
	unsigned short enemy_perfect_flee;
	unsigned short cast_rate,delay_rate,delay_dependon_dex;
	unsigned short sdelay_attack_enable;
	unsigned short left_cardfix_to_right;
	unsigned short pc_skill_add_range;
	unsigned short skill_out_range_consume;
	unsigned short mob_skill_add_range;
	unsigned short pc_damage_delay;
	unsigned short pc_damage_delay_rate;
	unsigned short defnotenemy;
	unsigned short random_monster_checklv;
	unsigned short attr_recover;
	unsigned short flooritem_lifetime;
	unsigned short item_auto_get;
	unsigned short item_auto_get_loot;
	unsigned short item_auto_get_distance;
	int item_first_get_time;
	int item_second_get_time;
	int item_third_get_time;
	int mvp_item_first_get_time;
	int mvp_item_second_get_time;
	int mvp_item_third_get_time;
	int base_exp_rate, job_exp_rate;
	unsigned short drop_rate0item;
	unsigned short death_penalty_type_base;
	unsigned short death_penalty_base;
	unsigned short death_penalty_base2_lvl, death_penalty_base2;
	unsigned short death_penalty_base3_lvl, death_penalty_base3;
	unsigned short death_penalty_base4_lvl, death_penalty_base4;
	unsigned short death_penalty_base5_lvl, death_penalty_base5;
	unsigned short death_penalty_type_job;
	unsigned short death_penalty_job;
	unsigned short death_penalty_job2_lvl, death_penalty_job2;
	unsigned short death_penalty_job3_lvl, death_penalty_job3;
	unsigned short death_penalty_job4_lvl, death_penalty_job4;
	unsigned short death_penalty_job5_lvl, death_penalty_job5;
	unsigned short pvp_exp;
	unsigned short gtb_pvp_only;
	int zeny_penalty;
	unsigned short zeny_penalty_percent;
	unsigned short zeny_penalty_by_lvl;
	unsigned short restart_hp_rate;
	unsigned short restart_sp_rate;
	int mvp_exp_rate;
	int mvp_common_rate, mvp_healing_rate, mvp_usable_rate, mvp_equipable_rate, mvp_card_rate;
	unsigned short mvp_hp_rate;
	unsigned short monster_hp_rate;
	unsigned short monster_max_aspd;
	unsigned short atc_gmonly;
	unsigned short atc_spawn_quantity_limit;
	unsigned short atc_map_mob_limit;
	unsigned short atc_local_spawn_interval;
	unsigned short gm_allskill;
	unsigned short gm_all_skill_job;
	unsigned short gm_all_skill_platinum;
	unsigned short gm_allskill_addabra;
	unsigned short gm_allequip;
	unsigned short gm_skilluncond;
	unsigned short skillfree;
	unsigned short skillup_limit;
	unsigned short check_minimum_skill_points;
	int check_maximum_skill_points;
	unsigned short wp_rate;
	unsigned short pp_rate;
	unsigned short cdp_rate;
	unsigned short monster_active_enable;
	unsigned short monster_damage_delay_rate;
	unsigned short monster_loot_type;
	unsigned short mob_skill_use;
	unsigned short mob_count_rate;
	unsigned short quest_skill_learn;
	unsigned short quest_skill_reset;
	unsigned short basic_skill_check;
	unsigned short no_caption_cloaking;
	unsigned short display_hallucination;
	unsigned short no_guilds_glory;
	unsigned short guild_emperium_check;
	unsigned short guild_exp_limit;
	unsigned short guild_max_castles;
	unsigned short pc_invincible_time;
	unsigned short pet_birth_effect;
	unsigned short pet_catch_rate;
	unsigned short pet_rename;
	unsigned short pet_friendly_rate;
	unsigned short pet_hungry_delay_rate;
	unsigned short pet_hungry_friendly_decrease;
	unsigned short pet_str;
	unsigned short pet_status_support;
	unsigned short pet_attack_support;
	unsigned short pet_damage_support;
	unsigned short pet_support_rate;
	unsigned short pet_attack_exp_to_master;
	unsigned short pet_attack_exp_rate;
	unsigned short skill_min_damage;
	unsigned short finger_offensive_type;
	unsigned short heal_exp;
	unsigned short resurrection_exp;
	unsigned short shop_exp;
	unsigned short combo_delay_rate;
	unsigned short item_check;
	unsigned short item_use_interval;
	unsigned short wedding_modifydisplay;
	int natural_healhp_interval;
	int natural_healsp_interval;
	int natural_heal_skill_interval;
	unsigned short natural_heal_weight_rate;
	unsigned short item_name_override_grffile;
	unsigned short indoors_override_grffile;
	unsigned short skill_sp_override_grffile;
	unsigned short cardillust_read_grffile;
	unsigned short item_equip_override_grffile;
	unsigned short item_slots_override_grffile;
	unsigned short arrow_decrement;
	unsigned short max_aspd;
	int max_hp;
	int max_sp;
	unsigned short max_lv;
	unsigned short max_parameter;
	int max_cart_weight;
	unsigned short pc_skill_log;
	unsigned short mob_skill_log;
	unsigned short battle_log;
	unsigned short save_log;
	unsigned short error_log;
	unsigned short etc_log;
	unsigned short save_clothcolor;
	unsigned short undead_detect_type;
	unsigned short pc_auto_counter_type;
	unsigned short monster_auto_counter_type;
	unsigned short agi_penalty_type;
	unsigned short agi_penalty_count;
	unsigned short agi_penalty_num;
	unsigned short vit_penalty_type;
	unsigned short vit_penalty_count;
	unsigned short vit_penalty_num;
	unsigned short player_defense_type;
	unsigned short monster_defense_type;
	unsigned short pet_defense_type;
	unsigned short magic_defense_type;
	unsigned short pc_skill_reiteration;
	unsigned short monster_skill_reiteration;
	unsigned short pc_skill_nofootset;
	unsigned short monster_skill_nofootset;
	unsigned short pc_cloak_check_type;
	unsigned short monster_cloak_check_type;
	unsigned short gvg_short_damage_rate;
	unsigned short gvg_long_damage_rate;
	unsigned short gvg_weapon_damage_rate;
	unsigned short gvg_magic_damage_rate;
	unsigned short gvg_misc_damage_rate;
	unsigned short gvg_flee_penalty;
	unsigned short gvg_eliminate_time;
	unsigned short mob_changetarget_byskill;
	unsigned short pc_attack_direction_change;
	unsigned short monster_attack_direction_change;
	unsigned short pc_land_skill_limit;
	unsigned short monster_land_skill_limit;
	unsigned short party_skill_penalty;
	unsigned short monster_class_change_full_recover;
	unsigned short produce_item_name_input;
	unsigned short produce_potion_name_input;
	unsigned short making_arrow_name_input;
	unsigned short holywater_name_input;
	unsigned short atcommand_item_creation_name_input;
	unsigned short atcommand_max_player_gm_level;
	int atcommand_send_usage_type;
	unsigned short display_delay_skill_fail;
	unsigned short display_snatcher_skill_fail;
	unsigned short chat_warpportal;
	unsigned short mob_warpportal;
	unsigned short dead_branch_active;
	int vending_max_value;
	unsigned short show_steal_in_same_party;
	unsigned short enable_upper_class;
	unsigned short pet_attack_attr_none;
	unsigned short mob_attack_attr_none;
	unsigned short mob_ghostring_fix;
	unsigned short pc_attack_attr_none;
	int item_rate_common, item_rate_card, item_rate_equip, item_rate_heal, item_rate_use;
	unsigned short item_drop_common_min, item_drop_common_max;
	unsigned short item_drop_card_min, item_drop_card_max;
	unsigned short item_drop_equip_min, item_drop_equip_max;
	unsigned short item_drop_mvp_min, item_drop_mvp_max;
	unsigned short item_drop_heal_min, item_drop_heal_max;
	unsigned short item_drop_use_min, item_drop_use_max;
	unsigned short prevent_logout;
	unsigned short alchemist_summon_reward;
	unsigned short maximum_level;
	unsigned short atcommand_max_job_level_novice;
	unsigned short atcommand_max_job_level_job1;
	unsigned short atcommand_max_job_level_job2;
	unsigned short atcommand_max_job_level_supernovice;
	unsigned short atcommand_max_job_level_highnovice;
	unsigned short atcommand_max_job_level_highjob1;
	unsigned short atcommand_max_job_level_highjob2;
	unsigned short atcommand_max_job_level_babynovice;
	unsigned short atcommand_max_job_level_babyjob1;
	unsigned short atcommand_max_job_level_babyjob2;
	unsigned short atcommand_max_job_level_superbaby;
	unsigned short atcommand_max_job_level_gunslinger;
	unsigned short atcommand_max_job_level_ninja;
	unsigned short drops_by_luk;
	unsigned short monsters_ignore_gm;
	unsigned short equipment_breaking;
	unsigned short equipment_break_rate;
	unsigned short pet_equip_required;
	unsigned short multi_level_up;
	unsigned short pk_mode;
	unsigned short show_mob_hp;
	unsigned short agi_penalty_count_lv;
	unsigned short vit_penalty_count_lv;
	unsigned short gx_allhit;
	unsigned short gx_cardfix;
	unsigned short gx_dupele;
	unsigned short gx_disptype;
	unsigned short devotion_level_difference;
	unsigned short player_skill_partner_check;
	unsigned short hide_GM_session;
	unsigned short unit_movement_type;
	unsigned short invite_request_check;
	unsigned short skill_removetrap_type;
	unsigned short disp_experience;
	unsigned short disp_experience_type;
	unsigned short castle_defense_rate;
	unsigned short riding_weight;
	unsigned short backstab_bow_penalty;
	unsigned short ka_skills_usage;
	unsigned short es_skills_usage;
	unsigned short hp_rate;
	unsigned short sp_rate;
	unsigned short gm_can_drop_lv;
	unsigned short display_hpmeter;
	unsigned short bone_drop;
	unsigned short night_at_start;
	int day_duration;
	int night_duration;
	unsigned short ban_spoof_namer;
	int ban_hack_trade;
	int ban_bot;
	unsigned short max_message_length;
	unsigned short max_global_message_length;
	unsigned short hack_info_GM_level;
	unsigned short speed_hack_info_GM_level;
	unsigned short any_warp_GM_min_level;
	unsigned short packet_ver_flag;
	unsigned short muting_players;
	unsigned short min_hair_style;
	unsigned short max_hair_style;
	unsigned short min_hair_color;
	unsigned short max_hair_color;
	unsigned short min_cloth_color;
	unsigned short max_cloth_color;
	unsigned short clothes_color_for_assassin;
	unsigned short castrate_dex_scale;
	unsigned short area_size;
	unsigned short zeny_from_mobs;
	unsigned short mobs_level_up;
	unsigned short pk_min_level;
	unsigned short skill_steal_type;
	unsigned short skill_steal_rate;
	unsigned short skill_range_leniency;
	unsigned short motd_type;
	unsigned short allow_atcommand_when_mute;
	unsigned short manner_action;
	unsigned short finding_ore_rate;
	unsigned short min_skill_delay_limit;
	unsigned short idle_no_share;
	int idle_delay_no_share;
	unsigned short chat_no_share;
	unsigned short npc_chat_no_share;
	unsigned short shop_no_share;
	unsigned short trade_no_share;
	unsigned short idle_disconnect;
	unsigned short idle_disconnect_chat;
	unsigned short idle_disconnect_vender;
	unsigned short idle_disconnect_disable_for_restore;
	unsigned short idle_disconnect_ignore_GM;
	unsigned short jail_message;
	unsigned short jail_discharge_message;
	unsigned short mingmlvl_message;
	unsigned short check_invalid_slot;
	unsigned short ruwach_range;
	unsigned short sight_range;
	unsigned short max_icewall;
	unsigned short ignore_items_gender;
	unsigned short party_invite_same_account;
	unsigned short atcommand_main_channel_at_start;
	int atcommand_main_channel_type;
	unsigned short atcommand_main_channel_on_gvg_map_woe;
	unsigned short atcommand_main_channel_when_woe;
	unsigned short atcommand_min_GM_level_for_request;
	unsigned short atcommand_follow_stop_dead_target;
	unsigned short atcommand_add_local_message_info;
	unsigned short atcommand_storage_on_pvp_map;
	unsigned short atcommand_gstorage_on_pvp_map;
	unsigned short pm_gm_not_ignored;
	unsigned short char_disconnect_mode;
	unsigned short extra_system_flag;
} battle_config;

int battle_config_read(const char *cfgName);
extern int battle_set_value(char *, char *);

#endif // _BATTLE_H_
