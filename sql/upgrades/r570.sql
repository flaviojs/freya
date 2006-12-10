# Database: Ragnarok
# Action: Update
# Reason: adds indexing on heavily used tables and reduces size of some data fields to improve performance

ALTER TABLE `login` ADD INDEX(account_id);
ALTER TABLE `char` ADD INDEX(char_id);
ALTER TABLE `global_reg_value` ADD INDEX(char_id);
ALTER TABLE `guild` ADD INDEX(guild_id);
ALTER TABLE `party` ADD INDEX(party_id);
ALTER TABLE `statuschange` ADD INDEX(char_id);

ALTER TABLE `cart_inventory` CHANGE `broken` `broken` TINYINT(1) NOT NULL DEFAULT '0';
ALTER TABLE `char` CHANGE `char_num` `char_num` TINYINT(1) NOT NULL DEFAULT '0';
ALTER TABLE `char` CHANGE `name` `name` VARCHAR(32) NOT NULL DEFAULT '';
ALTER TABLE `char` CHANGE `class` `class` SMALLINT(5) NOT NULL DEFAULT '0';
ALTER TABLE `char` CHANGE `base_level` `base_level` TINYINT(3) UNSIGNED NOT NULL DEFAULT '1';
ALTER TABLE `char` CHANGE `job_level` `job_level` TINYINT(3) UNSIGNED NOT NULL DEFAULT '1';
ALTER TABLE `char` CHANGE `str` `str` TINYINT(3) UNSIGNED NOT NULL DEFAULT '0';
ALTER TABLE `char` CHANGE `agi` `agi` TINYINT(3) UNSIGNED NOT NULL DEFAULT '0';
ALTER TABLE `char` CHANGE `vit` `vit` TINYINT(3) UNSIGNED NOT NULL DEFAULT '0';
ALTER TABLE `char` CHANGE `int` `int` TINYINT(3) UNSIGNED NOT NULL DEFAULT '0';
ALTER TABLE `char` CHANGE `dex` `dex` TINYINT(3) UNSIGNED NOT NULL DEFAULT '0';
ALTER TABLE `char` CHANGE `luk` `luk` TINYINT(3) UNSIGNED NOT NULL DEFAULT '0';
ALTER TABLE `char` CHANGE `last_x` `last_x` TINYINT(3) NOT NULL DEFAULT '53';
ALTER TABLE `char` CHANGE `last_y` `last_y` TINYINT(3) NOT NULL DEFAULT '111';
ALTER TABLE `char` CHANGE `save_x` `save_x` TINYINT(3) NOT NULL DEFAULT '53';
ALTER TABLE `char` CHANGE `save_y` `save_y` TINYINT(3) NOT NULL DEFAULT '111';
ALTER TABLE `char` CHANGE `online` `online` TINYINT(1) NOT NULL DEFAULT '0';
ALTER TABLE `charlog` CHANGE `str` `str` TINYINT(3) UNSIGNED NOT NULL DEFAULT '0';
ALTER TABLE `charlog` CHANGE `agi` `agi` TINYINT(3) UNSIGNED NOT NULL DEFAULT '0';
ALTER TABLE `charlog` CHANGE `vit` `vit` TINYINT(3) UNSIGNED NOT NULL DEFAULT '0';
ALTER TABLE `charlog` CHANGE `int` `int` TINYINT(3) UNSIGNED NOT NULL DEFAULT '0';
ALTER TABLE `charlog` CHANGE `dex` `dex` TINYINT(3) UNSIGNED NOT NULL DEFAULT '0';
ALTER TABLE `charlog` CHANGE `luk` `luk` TINYINT(3) UNSIGNED NOT NULL DEFAULT '0';
ALTER TABLE `guild` CHANGE `connect_member` `connect_member` TINYINT(2) NOT NULL DEFAULT '0';
ALTER TABLE `guild` CHANGE `max_member` `max_member` TINYINT(2) NOT NULL DEFAULT '0';
ALTER TABLE `guild` CHANGE `average_lv` `average_lv` SMALLINT(3) NOT NULL DEFAULT '0';
ALTER TABLE `guild` CHANGE `mes2` `mes2` VARCHAR(60) NOT NULL DEFAULT '';
ALTER TABLE `guild_storage` CHANGE `broken` `broken` TINYINT(1) NOT NULL DEFAULT '0';
ALTER TABLE `inventory` CHANGE `broken` `broken` TINYINT(1) NOT NULL DEFAULT '0';
ALTER TABLE `memo` CHANGE `map` `map` VARCHAR(20) NOT NULL DEFAULT '';
ALTER TABLE `memo` CHANGE `x` `x` TINYINT(3) NOT NULL DEFAULT '0';
ALTER TABLE `memo` CHANGE `y` `y` TINYINT(3) NOT NULL DEFAULT '0';
ALTER TABLE `skill` CHANGE `id` `id` SMALLINT(5) NOT NULL DEFAULT '0';
ALTER TABLE `skill` CHANGE `lv` `lv` TINYINT(3) NOT NULL DEFAULT '0';
ALTER TABLE `storage` CHANGE `broken` `broken` TINYINT(1) NOT NULL DEFAULT '0';