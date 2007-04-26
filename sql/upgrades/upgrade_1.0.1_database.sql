# Upgrade database tables for Freya 1.0 to 1.0.1

ALTER TABLE `item_db` MODIFY `name_english` varchar(32);
ALTER TABLE `item_db` MODIFY `name_japanese` varchar(32);
DELETE FROM `item_db` where id=1260;
INSERT INTO `item_db` VALUES (1260, 'Sharpened_Legbone_of_Ghoul', 'Sharpened Legbone of Ghoul', 4, 52500, NULL, 1700, 150, NULL, 1, 0, 4096, 2, 34, 3, 65, 16, NULL, 'bonus bAtkEle,9;');