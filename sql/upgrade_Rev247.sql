# Database: Ragnarok
# Table: 'ranking'
# Action: Create
# Reason: To store the fame points of every character, as per the newly implemented 'Fame System' (Rev. 247)
CREATE TABLE `ranking` (
  `char_id` int(10) unsigned NOT NULL,
  `class` enum('RK_BLACKSMITH','RK_ALCHEMIST','RK_TAEKWON','RK_PK') NOT NULL,
  `points` int(10) NOT NULL
) TYPE=MyISAM;