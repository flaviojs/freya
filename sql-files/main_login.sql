# Database: Ragnarok
# Table: 'login'
CREATE TABLE `login`
(
  `account_id` int(11) NOT NULL AUTO_INCREMENT,
  `userid` varchar(24) NOT NULL default '',
  `user_pass` varchar(32) NOT NULL default '',
  `lastlogin` datetime NOT NULL default '0000-00-00 00:00:00',
  `sex` char(1) NOT NULL default 'M',
  `logincount` mediumint(9) NOT NULL default '0',
  `email` varchar(40) NOT NULL default 'a@a.com',
  `level` smallint(3) NOT NULL default '0',
  `error_message` varchar(40) NOT NULL default '',
  `connect_until` int(11) NOT NULL default '0',
  `last_ip` varchar(16) NOT NULL default '',
  `memo` TEXT NOT NULL default '',
  `ban_until` int(11) NOT NULL default '0',
  `state` smallint(3) NOT NULL default '0',
  INDEX (`account_id`),
  PRIMARY KEY  (`account_id`),
  KEY `name` (`userid`)
) TYPE=MyISAM AUTO_INCREMENT=2000001;

# Database: Ragnarok
# Table: 'account_reg2_db'
CREATE TABLE `account_reg2_db`
(
  `account_id` int(11) NOT NULL default '0',
  `str` varchar(32) NOT NULL default '',
  `value` int(11) NOT NULL default '0',
  PRIMARY KEY (`account_id`, `str`)
) TYPE = MyISAM;

# Database: Ragnarok
# Table: 'sstatus'
CREATE TABLE `sstatus`
(
  `index` tinyint(4) NOT NULL default '0',
  `name` varchar(20) NOT NULL default '',
  `user` int(11) NOT NULL default '0'
) TYPE=MyISAM;

INSERT INTO `login` (`account_id`, `userid`, `user_pass`, `sex`, `email`) VALUES ('1', 's1', 'p1', 'S','a@a.com');
INSERT INTO `login` (`account_id`, `userid`, `user_pass`, `sex`, `email`) VALUES ('2', 's2', 'p2', 'S','a@a.com');
INSERT INTO `login` (`account_id`, `userid`, `user_pass`, `sex`, `email`) VALUES ('3', 's3', 'p3', 'S','a@a.com');
INSERT INTO `login` (`account_id`, `userid`, `user_pass`, `sex`, `email`) VALUES ('4', 's4', 'p4', 'S','a@a.com');
INSERT INTO `login` (`account_id`, `userid`, `user_pass`, `sex`, `email`) VALUES ('5', 's5', 'p5', 'S','a@a.com');
INSERT INTO `login` (`account_id`, `userid`, `user_pass`, `sex`, `email`) VALUES ('6', 's6', 'p6', 'S','a@a.com');
INSERT INTO `login` (`account_id`, `userid`, `user_pass`, `sex`, `email`) VALUES ('7', 's7', 'p7', 'S','a@a.com');
INSERT INTO `login` (`account_id`, `userid`, `user_pass`, `sex`, `email`) VALUES ('8', 's8', 'p8', 'S','a@a.com');
INSERT INTO `login` (`account_id`, `userid`, `user_pass`, `sex`, `email`) VALUES ('9', 's9', 'p9', 'S','a@a.com');
INSERT INTO `login` (`account_id`, `userid`, `user_pass`, `sex`, `email`) VALUES ('10', 's10', 'p10', 'S','a@a.com');
INSERT INTO `login` (`account_id`, `userid`, `user_pass`, `sex`, `email`) VALUES ('11', 's11', 'p11', 'S','a@a.com');
INSERT INTO `login` (`account_id`, `userid`, `user_pass`, `sex`, `email`) VALUES ('12', 's12', 'p12', 'S','a@a.com');
INSERT INTO `login` (`account_id`, `userid`, `user_pass`, `sex`, `email`) VALUES ('13', 's13', 'p13', 'S','a@a.com');
INSERT INTO `login` (`account_id`, `userid`, `user_pass`, `sex`, `email`) VALUES ('14', 's14', 'p14', 'S','a@a.com');
INSERT INTO `login` (`account_id`, `userid`, `user_pass`, `sex`, `email`) VALUES ('15', 's15', 'p15', 'S','a@a.com');
INSERT INTO `login` (`account_id`, `userid`, `user_pass`, `sex`, `email`) VALUES ('2000001', 'Test', 'Test', 'M', 'a@a.com', '99');
