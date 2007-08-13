# new table 'mec_intimate'
CREATE TABLE `mec_intimate` (
  `char_id` int(11) NOT NULL default '0',
  `intimate` int(11) NOT NULL default '0',
  KEY `char_id` (`char_id`)
) TYPE=MyISAM;
