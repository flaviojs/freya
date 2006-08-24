# Upgrade database tables of char-server for Freya 1.2.0 (or later) to next freya

# char_db Table
# ----------------
ALTER TABLE `char` ADD INDEX (`account_id`);

# to improve searching in the table `memo`
# ----------------------------------------
ALTER IGNORE TABLE `memo` DROP INDEX ( `char_id` );
ALTER TABLE `memo` ADD INDEX ( `char_id` );

# to improve request in the table `char`
# --------------------------------------
ALTER IGNORE TABLE `char` DROP INDEX ( `online` );
ALTER TABLE `char` ADD INDEX ( `online` );

# 'friends' Table
# --------------------------------------
ALTER TABLE `friends` DROP INDEX `char_id`; # index is in primary key
# update `friends` table to do that a friend must be friend of its friend too
INSERT IGNORE INTO `friends` SELECT `friend_id`, `char_id` FROM `friends`;

# 'skill' Table
# --------------------------------------
ALTER TABLE `skill` DROP INDEX `char_id`; # index is in primary key

# split global_reg_value to create global_reg_value (for only global reg) and account_reg_db
# --------------------------------------
ALTER TABLE `global_reg_value` MODIFY `str` varchar(32) NOT NULL default '';
ALTER TABLE `global_reg_value` MODIFY `value` int(11) NOT NULL default '0';
ALTER IGNORE TABLE `global_reg_value` DROP INDEX `account_id`;
ALTER IGNORE TABLE `global_reg_value` DROP INDEX `char_id`;

CREATE TABLE IF NOT EXISTS `account_reg_db` (
  `account_id` int(11) NOT NULL default '0',
  `str` varchar(32) NOT NULL default '',
  `value` int(11) NOT NULL default '0',
  PRIMARY KEY (`account_id`, `str`)
) TYPE = MyISAM;
# insert values
INSERT INTO `account_reg_db` (`account_id`, `str`, `value`) SELECT `account_id`, `str`, `value` FROM `global_reg_value` WHERE `type`='2';
# delete values
DELETE FROM `global_reg_value` WHERE `type`='2';
# delete PRIMARY KEY and create a new PRIMARY KEY
ALTER TABLE `global_reg_value` DROP PRIMARY KEY;
ALTER TABLE `global_reg_value` ADD PRIMARY KEY (`char_id`, `str`);
# delete unused column
ALTER TABLE `global_reg_value` DROP COLUMN `account_id`;
ALTER TABLE `global_reg_value` DROP COLUMN `type`;
