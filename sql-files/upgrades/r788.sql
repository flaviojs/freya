# @DB						: Ragnarok
# @Table				: char
# @Description	: Needed table modifications for the adoption system.
# @Author				: Proximus


# BACKUP YOUR DATABASE BEFORE MAKING ANY CHANGES!

ALTER TABLE `char` 
ADD `father` INT(11) UNSIGNED NOT NULL DEFAULT '0' AFTER `partner_id`,
ADD `mother` INT(11) UNSIGNED NOT NULL DEFAULT '0' AFTER `partner_id`,
ADD `child` INT(11) UNSIGNED NOT NULL DEFAULT '0' AFTER `partner_id`;
