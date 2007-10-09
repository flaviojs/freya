#--------------------------------------------------------------
#               (c)2004-2007 Aurora Team Presents:             
#                  ___                                         
#                 /   |_   _ _ __ ___  _ __ __ _               
#                / /| | | | | '__/ _ \| '__/ _` |              
#               / ___ | |_| | | | (_) | | | (_| |++            
#              /_/  |_|\____|_|  \___/|_|  \__,_|              
#                 http://aurora.deltaanime.net                 
#--------------------------------------------------------------
#
# Main_MSSQL.sql
# MSSQL 2000/2005 Database Creation Script

create database [ragnarok];
go
use [ragnarok];

/*==========================================
 * Create Table : [cart_inventory]
 *------------------------------------------
 */
create table [cart_inventory] (
	[id] bigint IDENTITY(1,1) NOT NULL,
	[char_id] int NOT NULL DEFAULT 0,
	[nameid] int NOT NULL DEFAULT 0,
	[amount] int NOT NULL DEFAULT 0,
	[equip] int NOT NULL DEFAULT 0,
	[identify] smallint NOT NULL DEFAULT 0,
	[refine] tinyint NOT NULL DEFAULT 0,
	[attribute] tinyint NOT NULL DEFAULT 0,
	[card0] int NOT NULL DEFAULT 0,
	[card1] int NOT NULL DEFAULT 0,
	[card2] int NOT NULL DEFAULT 0,
	[card3] int NOT NULL DEFAULT 0,
	[broken] bigint NOT NULL DEFAULT 0,
	[lockid] int NOT NULL DEFAULT 0,
	[exp] int NOT NULL DEFAULT 0,
	[endurance] int NOT NULL DEFAULT 0,
	[max_endurance] int NOT NULL DEFAULT 0,
	[sp_lv] int NOT NULL DEFAULT 0,
	primary key([id])
);

/*==========================================
 * Create Table : [char]
 *------------------------------------------
 */
create table [char] (
	[char_id] int IDENTITY(15000,1) NOT NULL,
	[account_id] int NOT NULL default 0,
	[char_num] tinyint NOT NULL default 0,
	[name] varchar(255) NOT NULL default '',
	[class] int NOT NULL default 0,
	[base_level] tinyint NOT NULL default 1,
	[job_level] tinyint NOT NULL default 1,
	[base_exp] bigint NOT NULL default 0,
	[job_exp] bigint NOT NULL default 0,
	[zeny] int NOT NULL default 500,
	[str] int NOT NULL default 0,
	[agi] int NOT NULL default 0,
	[vit] int NOT NULL default 0,
	[int] int NOT NULL default 0,
	[dex] int NOT NULL default 0,
	[luk] int NOT NULL default 0,
	[max_hp] int NOT NULL default 0,
	[hp] int NOT NULL default 0,
	[max_sp] int NOT NULL default 0,
	[sp] int NOT NULL default 0,
	[status_point] int NOT NULL default 0,
	[skill_point] int NOT NULL default 0,
	[option] int NOT NULL default 0,
	[karma] int NOT NULL default 0,
	[manner] int NOT NULL default 0,
	[party_id] int NOT NULL default 0,
	[guild_id] int NOT NULL default 0,
	[pet_id] int NOT NULL default 0,
	[hair] tinyint NOT NULL default 0,
	[hair_color] int NOT NULL default 0,
	[clothes_color] tinyint NOT NULL default 0,
	[weapon] int NOT NULL default 1,
	[shield] int NOT NULL default 0,
	[head_top] int NOT NULL default 0,
	[head_mid] int NOT NULL default 0,
	[head_bottom] int NOT NULL default 0,
	[last_map] varchar(20) NOT NULL default 'new_1-1.gat',
	[last_x] int NOT NULL default 53,
	[last_y] int NOT NULL default 111,
	[save_map] varchar(20) NOT NULL default 'new_1-1.gat',
	[save_x] int NOT NULL default 53,
	[save_y] int NOT NULL default 111,
	[partner_id] int NOT NULL default 0,
	[parent_id] int NOT NULL default 0,
	[parent_id2] int NOT NULL default 0,
	[baby_id] int NOT NULL default 0,
	[online] tinyint NOT NULL default 0,
	[homun_id] int NOT NULL default 0,
	primary key([char_id])
);

/*==========================================
 * Create Table : [charlog]
 *------------------------------------------
 */
create table [charlog] (
	[id] bigint IDENTITY(1,1) NOT NULL,
	[time] datetime NOT NULL default GETDATE(),
	[log] TEXT NOT NULL default '',
	primary key([id])
);

/*==========================================
 * Create Table : [friend]
 *------------------------------------------
 */
create table [friend] (
	[char_id] int NOT NULL default 0,
	[id1] int NOT NULL default 0,
	[id2] int NOT NULL default 0,
	[name] varchar(24) NOT NULL default ''
);

/*==========================================
 * Create Table : [global_reg_value]
 *------------------------------------------
 */
create table [global_reg_value] (
	[char_id] int NOT NULL default 0,
	[str] varchar(255) NOT NULL default '',
	[value] int NOT NULL default 0,
	[type] int NOT NULL default 3,
	[account_id] int NOT NULL default 0,
	primary key([char_id],[str],[account_id])
);

/*==========================================
 * Create Table : [guild]
 *------------------------------------------
 */
create table [guild] (
	[guild_id] int NOT NULL default 10000,
	[name] varchar(24) NOT NULL default '',
	[master] varchar(24) NOT NULL default '',
	[guild_lv] smallint NOT NULL default 0,
	[connect_member] smallint NOT NULL default 0,
	[max_member] smallint NOT NULL default 0,
	[average_lv] smallint NOT NULL default 0,
	[exp] int NOT NULL default 0,
	[next_exp] int NOT NULL default 0,
	[skill_point] int NOT NULL default 0,
	[castle_id] int NOT NULL default -1,
	[mes1] varchar(60) NOT NULL default '',
	[mes2] varchar(120) NOT NULL default '',
	[emblem_len] int NOT NULL default 0,
	[emblem_id] int NOT NULL default 0,
	/*[emblem_data] varchar(MAX) NOT NULL default '',*/	/* SQL2000 ERROR */
	[emblem_data] varchar(6000) NOT NULL default '',
	primary key([guild_id])
);

/*==========================================
 * Create Table : [guild_alliance]
 *------------------------------------------
 */
create table [guild_alliance] (
	[guild_id] int NOT NULL default 0,
	[opposition] int NOT NULL default 0,
	[alliance_id] int NOT NULL default 0,
	[name] varchar(24) NOT NULL default ''
);

/*==========================================
 * Create Table : [guild_castle]
 *------------------------------------------
 */
create table [guild_castle] (
	[castle_id] int NOT NULL default 0,
	[guild_id] int NOT NULL default 0,
	[economy] int NOT NULL default 0,
	[defense] int NOT NULL default 0,
	[triggerE] int NOT NULL default 0,
	[triggerD] int NOT NULL default 0,
	[nextTime] int NOT NULL default 0,
	[payTime] int NOT NULL default 0,
	[createTime] int NOT NULL default 0,
	[visibleC] int NOT NULL default 0,
	[visibleG0] int NOT NULL default 0,
	[visibleG1] int NOT NULL default 0,
	[visibleG2] int NOT NULL default 0,
	[visibleG3] int NOT NULL default 0,
	[visibleG4] int NOT NULL default 0,
	[visibleG5] int NOT NULL default 0,
	[visibleG6] int NOT NULL default 0,
	[visibleG7] int NOT NULL default 0,
	[gHP0] int NOT NULL default 0,
	[gHP1] int NOT NULL default 0,
	[gHP2] int NOT NULL default 0,
	[gHP3] int NOT NULL default 0,
	[gHP4] int NOT NULL default 0,
	[gHP5] int NOT NULL default 0,
	[gHP6] int NOT NULL default 0,
	[gHP7] int NOT NULL default 0,
	PRIMARY KEY ([castle_id])
);
INSERT INTO [guild_castle] ([castle_id]) VALUES (0);
INSERT INTO [guild_castle] ([castle_id]) VALUES (1);
INSERT INTO [guild_castle] ([castle_id]) VALUES (2);
INSERT INTO [guild_castle] ([castle_id]) VALUES (3);
INSERT INTO [guild_castle] ([castle_id]) VALUES (4);
INSERT INTO [guild_castle] ([castle_id]) VALUES (5);
INSERT INTO [guild_castle] ([castle_id]) VALUES (6);
INSERT INTO [guild_castle] ([castle_id]) VALUES (7);
INSERT INTO [guild_castle] ([castle_id]) VALUES (8);
INSERT INTO [guild_castle] ([castle_id]) VALUES (9);
INSERT INTO [guild_castle] ([castle_id]) VALUES (10);
INSERT INTO [guild_castle] ([castle_id]) VALUES (11);
INSERT INTO [guild_castle] ([castle_id]) VALUES (12);
INSERT INTO [guild_castle] ([castle_id]) VALUES (13);
INSERT INTO [guild_castle] ([castle_id]) VALUES (14);
INSERT INTO [guild_castle] ([castle_id]) VALUES (15);
INSERT INTO [guild_castle] ([castle_id]) VALUES (16);
INSERT INTO [guild_castle] ([castle_id]) VALUES (17);
INSERT INTO [guild_castle] ([castle_id]) VALUES (18);
INSERT INTO [guild_castle] ([castle_id]) VALUES (19);

/*==========================================
 * Create Table : [guild_expulsion]
 *------------------------------------------
 */
create table [guild_expulsion] (
	[guild_id] int NOT NULL default 0,
	[name] varchar(24) NOT NULL default '',
	[mes] varchar(40) NOT NULL default '',
	[acc] varchar(40) NOT NULL default '',
	[account_id] int NOT NULL default 0,
	[rsv1] int NOT NULL default 0,
	[rsv2] int NOT NULL default 0,
	[rsv3] int NOT NULL default 0
);

/*==========================================
 * Create Table : [guild_member]
 *------------------------------------------
 */
create table [guild_member] (
	[guild_id] int NOT NULL default 0,
	[account_id] int NOT NULL default 0,
	[char_id] int NOT NULL default 0,
	[hair] smallint NOT NULL default 0,
	[hair_color] smallint NOT NULL default 0,
	[gender] smallint NOT NULL default 0,
	[class] smallint NOT NULL default 0,
	[lv] smallint NOT NULL default 0,
	[exp] bigint NOT NULL default 0,
	[exp_payper] int NOT NULL default 0,
	[online] tinyint NOT NULL default 0,
	[position] smallint NOT NULL default 0,
	[rsv1] int NOT NULL default 0,
	[rsv2] int NOT NULL default 0,
	[name] varchar(24) NOT NULL default ''
);

/*==========================================
 * Create Table : [guild_position]
 *------------------------------------------
 */
create table [guild_position] (
	[guild_id] int NOT NULL default 0,
	[position] smallint NOT NULL default 0,
	[name] varchar(24) NOT NULL default '',
	[mode] int NOT NULL default 0,
	[exp_mode] int NOT NULL default 0
);

/*==========================================
 * Create Table : [guild_skill]
 *------------------------------------------
 */
create table [guild_skill] (
	[guild_id] int NOT NULL default 0,
	[id] int NOT NULL default 0,
	[lv] int NOT NULL default 0
);

/*==========================================
 * Create Table : [guild_storage]
 *------------------------------------------
 */
create table [guild_storage] (
	[id] bigint IDENTITY(1,1) NOT NULL,
	[guild_id] int NOT NULL DEFAULT 0,
	[nameid] int NOT NULL DEFAULT 0,
	[amount] int NOT NULL DEFAULT 0,
	[equip] int NOT NULL DEFAULT 0,
	[identify] smallint NOT NULL DEFAULT 0,
	[refine] tinyint NOT NULL DEFAULT 0,
	[attribute] tinyint NOT NULL DEFAULT 0,
	[card0] int NOT NULL DEFAULT 0,
	[card1] int NOT NULL DEFAULT 0,
	[card2] int NOT NULL DEFAULT 0,
	[card3] int NOT NULL DEFAULT 0,
	[broken] bigint NOT NULL DEFAULT 0,
	[lockid] int NOT NULL DEFAULT 0,
	[exp] int NOT NULL DEFAULT 0,
	[endurance] int NOT NULL DEFAULT 0,
	[max_endurance] int NOT NULL DEFAULT 0,
	[sp_lv] int NOT NULL DEFAULT 0,
	primary key([id])
);

/*==========================================
 * Create Table : [homunculus]
 *------------------------------------------
 */
create table [homunculus] (
	[homun_id] bigint IDENTITY(1,1) NOT NULL,
	[class] int NOT NULL default 0,
	[name] varchar(24) NOT NULL default '',
	[account_id] int NOT NULL default 0,
	[char_id] int NOT NULL default 0,
	[base_level] tinyint NOT NULL default 0,
	[base_exp] bigint NOT NULL default 0,
	[max_hp] int NOT NULL default 0,
	[hp] int NOT NULL default 0,
	[max_sp] int NOT NULL default 0,
	[sp] int NOT NULL default 0,
	[str] int NOT NULL default 0,
	[agi] int NOT NULL default 0,
	[vit] int NOT NULL default 0,
	[int] int NOT NULL default 0,
	[dex] int NOT NULL default 0,
	[luk] int NOT NULL default 0,
	[status_point] int NOT NULL default 0,
	[skill_point] int NOT NULL default 0,
	[equip] int NOT NULL default 0,
	[intimate] int NOT NULL default 0,
	[hungry] int NOT NULL default 0,
	[rename_flag] tinyint NOT NULL default 0,
	[incubate] int NOT NULL default 0,
	primary key([homun_id])
);

/*==========================================
 * Create Table : [homunculus_skill]
 *------------------------------------------
 */
create table [homunculus_skill] (
	[homun_id] int NOT NULL default 0,
	[id] int NOT NULL default 0,
	[lv] int NOT NULL default 0
);

/*==========================================
 * Create Table : [interlog]
 *------------------------------------------
 */
create table [interlog] (
	[id] bigint IDENTITY(1,1) NOT NULL,
	[time] datetime NOT NULL default GETDATE(),
	[log] varchar(255) NOT NULL default '',
	primary key([id])
);

/*==========================================
 * Create Table : [inventory]
 *------------------------------------------
 */
create table [inventory] (
	[id] bigint IDENTITY(1,1) NOT NULL,
	[char_id] int NOT NULL DEFAULT 0,
	[nameid] int NOT NULL DEFAULT 0,
	[amount] int NOT NULL DEFAULT 0,
	[equip] int NOT NULL DEFAULT 0,
	[identify] smallint NOT NULL DEFAULT 0,
	[refine] tinyint NOT NULL DEFAULT 0,
	[attribute] tinyint NOT NULL DEFAULT 0,
	[card0] int NOT NULL DEFAULT 0,
	[card1] int NOT NULL DEFAULT 0,
	[card2] int NOT NULL DEFAULT 0,
	[card3] int NOT NULL DEFAULT 0,
	[broken] bigint NOT NULL DEFAULT 0,
	[lockid] int NOT NULL DEFAULT 0,
	[exp] int NOT NULL DEFAULT 0,
	[endurance] int NOT NULL DEFAULT 0,
	[max_endurance] int NOT NULL DEFAULT 0,
	[sp_lv] int NOT NULL DEFAULT 0,
	primary key([id])
);

/*==========================================
 * Create Table : [ipbanlist]
 *------------------------------------------
 */
create table [ipbanlist] (
	[list] varchar(255) NOT NULL default '',
	[btime] datetime NOT NULL default GETDATE(),
	[rtime] datetime NOT NULL default GETDATE(),
	[reason] varchar(255) NOT NULL default ''
);

/*==========================================
 * Create Table : [login]
 *------------------------------------------
 */
create table [login] (
	[account_id] int IDENTITY(2000000,1) NOT NULL,
	[userid] varchar(255) NOT NULL default '',
	[user_pass] varchar(32) NOT NULL default '',
	[lastlogin] datetime NOT NULL default GETDATE(),
	[sex] char(1) NOT NULL default 'M',
	[logincount] int NOT NULL default 0,
	[email] varchar(60) NOT NULL default '',
	[level] smallint NOT NULL default 0,
	[error_message] int NOT NULL default 0,
	[connect_until] int NOT NULL default 0,
	[last_ip] varchar(100) NOT NULL default '',
	[memo] int NOT NULL default 0,
	[ban_until] int NOT NULL default 0,
	[state] int NOT NULL default 0,
	[reg_date] datetime NOT NULL default GETDATE(),
	[ask] varchar(255) default '',
	[answer] varchar(255) default '',
	primary key([account_id])
);
SET IDENTITY_INSERT [login] ON;
INSERT INTO [login] ([account_id], [userid], [user_pass], [sex], [email]) VALUES (1, 's1', 'p1', 'S','athena@athena.com');
SET IDENTITY_INSERT [login] OFF;

/*==========================================
 * Create Table : [login_error]
 *------------------------------------------
 */
create table [login_error] (
	[err_id] int NOT NULL default 0,
	[reason] varchar(100) NOT NULL default 'Unknown',
	primary key([err_id])
);


/*==========================================
 * Create Table : [loginlog]
 *------------------------------------------
 */
create table [loginlog] (
	[id] bigint IDENTITY(1,1) NOT NULL,
	[time] datetime NOT NULL default GETDATE(),
	[log] varchar(255) NOT NULL default '',
	primary key([id])
);

/*==========================================
 * Create Table : [memo]
 *------------------------------------------
 */
create table [memo] (
	[memo_id] bigint IDENTITY(1,1) NOT NULL,
	[char_id] int NOT NULL default 0,
	[type] char(1) NOT NULL default '',
	[map] varchar(255) NOT NULL default '',
	[x] int NOT NULL default -1,
	[y] int NOT NULL default -1,
	primary key([memo_id])
);

/*==========================================
 * Create Table : [party]
 *------------------------------------------
 */
create table [party] (
	[party_id] int NOT NULL default 100,
	[name] varchar(100) NOT NULL default '',
	[exp] int NOT NULL default 0,
	[item] int NOT NULL default 0,
	[leader_id] int NOT NULL default 0,
	primary key([party_id])
);

/*==========================================
 * Create Table : [pet]
 *------------------------------------------
 */
create table [pet] (
	[pet_id] int IDENTITY(1,1) NOT NULL,
	[class] int NOT NULL default 0,
	[name] varchar(24) NOT NULL default '',
	[account_id] int NOT NULL default 0,
	[char_id] int NOT NULL default 0,
	[level] tinyint NOT NULL default 0,
	[egg_id] int NOT NULL default 0,
	[equip] int NOT NULL default 0,
	[intimate] int NOT NULL default 0,
	[hungry] int NOT NULL default 0,
	[rename_flag] tinyint NOT NULL default 0,
	[incubate] int NOT NULL default 0,
	primary key([pet_id])
);

/*==========================================
 * Create Table : [ragsrvinfo]
 *------------------------------------------
 */
create table [ragsrvinfo] (
	[index] int NOT NULL default 0,
	[name] varchar(255) NOT NULL default '',
	[exp] int NOT NULL default 0,
	[jexp] int NOT NULL default 0,
	[drop] int NOT NULL default 0,
	[motd] varchar(255) NOT NULL default ''
);

/*==========================================
 * Create Table : [skill]
 *------------------------------------------
 */
create table [skill] (
	[char_id] int NOT NULL default 0,
	[id] int NOT NULL default 0,
	[lv] tinyint NOT NULL default 0,
	primary key([char_id],[id])
);

/*==========================================
 * Create Table : [sstatus]
 *------------------------------------------
 */
create table [sstatus] (
	[index] tinyint NOT NULL default 0,
	[name] varchar(255) NOT NULL default '',
	[user] int NOT NULL default 0,
	primary key([index])
);

/*==========================================
 * Create Table : [storage]
 *------------------------------------------
 */
create table [storage] (
	[id] bigint IDENTITY(1,1) NOT NULL,
	[account_id] int NOT NULL DEFAULT 0,
	[nameid] int NOT NULL DEFAULT 0,
	[amount] int NOT NULL DEFAULT 0,
	[equip] int NOT NULL DEFAULT 0,
	[identify] smallint NOT NULL DEFAULT 0,
	[refine] tinyint NOT NULL DEFAULT 0,
	[attribute] tinyint NOT NULL DEFAULT 0,
	[card0] int NOT NULL DEFAULT 0,
	[card1] int NOT NULL DEFAULT 0,
	[card2] int NOT NULL DEFAULT 0,
	[card3] int NOT NULL DEFAULT 0,
	[broken] bigint NOT NULL DEFAULT 0,
	[lockid] int NOT NULL DEFAULT 0,
	[exp] int NOT NULL DEFAULT 0,
	[endurance] int NOT NULL DEFAULT 0,
	[max_endurance] int NOT NULL DEFAULT 0,
	[sp_lv] int NOT NULL DEFAULT 0,
	primary key([id])
);

/*==========================================
 * Create Table : [status_change]
 *------------------------------------------
 */
create table [status_change] (
	[char_id] int NOT NULL default 0,
	[account_id] int NOT NULL default 0,
	[type] int NOT NULL default 0,
	[val1] int NOT NULL default 0,
	[val2] int NOT NULL default 0,
	[val3] int NOT NULL default 0,
	[val4] int NOT NULL default 0,
	[tick] int NOT NULL default 0
);

/*==========================================
 * Create Table : [hotkey]
 *------------------------------------------
 */
create table [hotkey] (
	[char_id] int NOT NULL default 0,
	[type] int NOT NULL default 0,
	[hotkey] int NOT NULL default 0,
	[id] int NOT NULL default 0,
	[skill_lv] int NOT NULL default 0
);

/*==========================================
 * Create Table : [mail]
 *------------------------------------------
 */
create table [mail] (
	[char_id] int NOT NULL default 0,
	[account_id] int NOT NULL default 0,
	[rates] int NOT NULL default 0,
	[store] int NOT NULL default 0,
	primary key([char_id],[account_id])
);

/*==========================================
 * Create Table : [mail_data]
 *------------------------------------------
 */
create table [mail_data] (
	[char_id] int NOT NULL default 0,
	[mail_num] int NOT NULL default 0,
	[read] int NOT NULL default 0,
	[char_name] varchar(24) NOT NULL default '',
	[receive_name] varchar(24) NOT NULL default '',
	[title] varchar(40) NOT NULL default '',
	[zeny] int NOT NULL default 0,
	[nameid] int NOT NULL DEFAULT 0,
	[amount] int NOT NULL DEFAULT 0,
	[equip] int NOT NULL DEFAULT 0,
	[identify] smallint NOT NULL DEFAULT 0,
	[refine] tinyint NOT NULL DEFAULT 0,
	[attribute] tinyint NOT NULL DEFAULT 0,
	[card0] int NOT NULL DEFAULT 0,
	[card1] int NOT NULL DEFAULT 0,
	[card2] int NOT NULL DEFAULT 0,
	[card3] int NOT NULL DEFAULT 0,
	[broken] bigint NOT NULL DEFAULT 0,
	[lockid] int NOT NULL DEFAULT 0,
	[exp] int NOT NULL DEFAULT 0,
	[endurance] int NOT NULL DEFAULT 0,
	[max_endurance] int NOT NULL DEFAULT 0,
	[sp_lv] int NOT NULL DEFAULT 0,
	[times] int NOT NULL default 0,
	[body_size] int NOT NULL default 0,
	[body] varchar(1024) NOT NULL default ''
);