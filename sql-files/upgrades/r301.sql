# Database: Ragnarok
# Table: 'ranking'
# Action: Update
# Reason: Solution to the problem with loading the ranking points of a player. Sometimes failing, causing a reset of points because of
# using ENUM type, which its somehow not well supported by the mysql header used in the emulator.
# Note: Backup your database before executing this script!
ALTER TABLE `ranking` CHANGE `class` `class` TINYINT(1) UNSIGNED NOT NULL; #Change type from 'ENUM' to 'TINYINT'
UPDATE `ranking` set `class` = 0 WHERE `class` = 1; #Blacksmith
UPDATE `ranking` set `class` = 1 WHERE `class` = 2; #Alchemist
UPDATE `ranking` set `class` = 2 WHERE `class` = 3; #Taekwon
UPDATE `ranking` set `class` = 3 WHERE `class` = 4; #PK
