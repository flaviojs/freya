# new table 'hotkey'
CREATE TABLE `hotkey` (
  `char_id` int(11) NOT NULL default '0',
  `type` int(11) NOT NULL default '0',
  `hotkey` int(11) NOT NULL default '0',
  `id` int(11) NOT NULL default '0',
  `skill_lv` int(11) NOT NULL default '0',
  KEY `char_id` (`char_id`)
) TYPE=MyISAM;
