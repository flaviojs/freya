# Upgrade database tables for Freya 1.0 (or later) to next

ALTER TABLE `item_db` MODIFY `name_english` varchar(32);
ALTER TABLE `item_db` MODIFY `name_japanese` varchar(32);

DELETE FROM `item_db` WHERE id=557;
INSERT INTO `item_db` VALUES (557, 'Cut_Rice_Rolls', 'Cut Rice Rolls', 0, NULL, NULL, 10, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
DELETE FROM `item_db` WHERE id=561;
INSERT INTO `item_db` VALUES (561, 'Milk_Chocolate_Bar', 'Milk Chocolate', 0, 0, NULL, 80, NULL, NULL, NULL, NULL, 10477567, 2, NULL, NULL, NULL, NULL, NULL, NULL);
DELETE FROM `item_db` WHERE id=1260;
INSERT INTO `item_db` VALUES (1260, 'Sharpened_Legbone_of_Ghoul', 'Sharpened Legbone of Ghoul', 4, 52500, NULL, 1700, 150, NULL, 1, 0, 4096, 2, 34, 3, 65, 16, NULL, 'bonus bAtkEle,9;');
DELETE FROM `item_db` WHERE id BETWEEN 669 AND 681;
INSERT INTO `item_db` VALUES (669, 'Rice_Cake_Soup', 'Rice Cake Soup', 2, NULL, NULL, 100, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
INSERT INTO `item_db` VALUES (670, 'Gold_Coin_Pouch', 'Gold Coin Pouch', 2, NULL, NULL, 400, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
INSERT INTO `item_db` VALUES (671, 'Gold_Coin', 'Gold Coin', 2, NULL, NULL, 40, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
INSERT INTO `item_db` VALUES (672, 'Copper_Coin_Pouch', 'Copper Coin Pouch', 2, NULL, NULL, 400, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
INSERT INTO `item_db` VALUES (673, 'Copper_Coin', 'Copper_Coin', 2, NULL, NULL, 40, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
INSERT INTO `item_db` VALUES (674, 'Mysterious_Ore_Coin', 'Mysterious Ore Coin', 2, NULL, NULL, 40, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
INSERT INTO `item_db` VALUES (675, 'Silver_Coin', 'Silver Coin', 2, NULL, NULL, 40, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
INSERT INTO `item_db` VALUES (676, 'Silver_Coin_Pouch', 'Silver Coin Pouch', 2, NULL, NULL, 400, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
INSERT INTO `item_db` VALUES (677, 'Platinum Coin', 'Platinum Coin', 2, NULL, NULL, 40, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
INSERT INTO `item_db` VALUES (678, 'Deadly_Poison_Bottle', 'Deadly Poison Bottle', 2, NULL, NULL, 100, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
INSERT INTO `item_db` VALUES (679, 'Recall_Pills', 'Recall Pills', 2, NULL, NULL, 300, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
INSERT INTO `item_db` VALUES (680, 'Carnation', 'Carnation', 2, NULL, NULL, 1000, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
INSERT INTO `item_db` VALUES (681, 'Wedding_Photo_Album', 'Wedding Photo Album', 2, NULL, NULL, 10, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
DELETE FROM `item_db` WHERE id=1260;
INSERT INTO `item_db` VALUES (1260, 'Sharpened_Legbone_of_Ghoul', 'Sharpened Legbone of Ghoul', 4, 52500, NULL, 1700, 150, NULL, 1, 0, 4096, 2, 34, 3, 65, 16, NULL, 'bonus bAtkEle,9;');
DELETE FROM `item_db` WHERE id BETWEEN 7263 AND 7270;
INSERT INTO `item_db` VALUES (7263, 'Cat\'s_Eye', 'Cat\'s-Eye', 3, NULL, 477, 10, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
INSERT INTO `item_db` VALUES (7264, 'Dried_Sand', 'Dried Sand', 3, NULL, 161, 10, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
INSERT INTO `item_db` VALUES (7265, 'Dragon_Horn', 'Dragon Horn', 3, NULL, 272, 10, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
INSERT INTO `item_db` VALUES (7266, 'Dragon_Teeth', 'Dragon Teeth', 3, 218, 10, 10, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
INSERT INTO `item_db` VALUES (7267, 'Tigerskin_Underwear', 'Tigerskin Underwear', 3, NULL, 149, 10, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
INSERT INTO `item_db` VALUES (7268, 'Ghost_Doll', 'Ghost Doll', 3, NULL, 605, 10, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
INSERT INTO `item_db` VALUES (7269, 'Baby_Bib', 'Baby Bib', 3, NULL, NULL, 10, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
INSERT INTO `item_db` VALUES (7270, 'Baby_Bottle', 'Baby Bottle', 3, NULL, 10, 10, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
DELETE FROM `mob_db` WHERE ID=1033;
INSERT INTO `mob_db` VALUES (1033, 'ELDER_WILLOW', 'Elder Willow', 20, 693, 0, 163, 101, 1, 58, 70, 10, 30, 1, 20, 25, 35, 38, 30, 10, 12, 1, 3, 43, 133, 200, 1372, 672, 432, 990, 50, 907, 5500, 1019, 3500, 757, 37, 2329, 30, 516, 1000, 604, 100, 4052, 1, 0, 0, 0, 0, 0, 0, 0, 0);

CREATE TABLE `friends` (
  `char_id` int(11) NOT NULL default '0',
  `friend_id` int(11) NOT NULL default '0',
  PRIMARY KEY (`char_id`,`friend_id`),
  KEY `char_id` (`char_id`)
) TYPE=MyISAM

CREATE TABLE `mail` (
  `message_id` int(11) NOT NULL auto_increment,
  `to_account_id` int(11) NOT NULL default '0',
  `to_char_name` varchar(24) NOT NULL default '',
  `from_account_id` int(11) NOT NULL default '0',
  `from_char_name` varchar(24) NOT NULL default '',
  `message` varchar(80) NOT NULL default '',
  `read_flag` tinyint(1) NOT NULL default '0',
  `priority` tinyint(1) NOT NULL default '0',
  `check_flag` tinyint(1) NOT NULL default '0',
  PRIMARY KEY  (`message_id`)
) TYPE=MyISAM;
