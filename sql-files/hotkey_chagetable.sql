#--------------------------------------------------------------
#               (c)2004-2007 Aurora Team Presents:             
#                  ___                                         
#                 /   |_   _ _ __ ___  _ __ __ _               
#                / /| | | | | '__/ _ \| '__/ _` |              
#               / ___ | |_| | | | (_) | | | (_| |              
#              /_/  |_|\____|_|  \___/|_|  \__,_|              
#                 http://aurora.deltaanime.net                 
#--------------------------------------------------------------
#
# hotkey_changetable.sql
# Aurora SQL Database Hot Key Table Update Script

CREATE TABLE `hotkey` (
  `char_id` int(11) NOT NULL default '0',
  `type` int(11) NOT NULL default '0',
  `hotkey` int(11) NOT NULL default '0',
  `id` int(11) NOT NULL default '0',
  `skill_lv` int(11) NOT NULL default '0',
  KEY `char_id` (`char_id`)
) TYPE=MyISAM;
