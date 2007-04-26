// Copyright (c) Freya Development Team - Licensed under GNU GPL
// For more information, see License.txt in the main folder

#include <config.h>

#include <string.h>
#include <stdlib.h>
#include <fcntl.h> /* for open, and O_WRONLY | O_CREAT | O_APPEND */
#include <unistd.h> /* for write and close */
#include <sys/time.h> /* gettimeofday */
#include <stdarg.h> // va_list
#include <sys/stat.h> //for mkdir

#include "char.h"
#include "inter.h"
#include "int_party.h"
#include "int_guild.h"
#include "int_storage.h"
#include "int_pet.h"
#include "../common/lock.h"
#include "../common/malloc.h"


#define WISDATA_TTL (45*1000) // Existence time of Wisp/page data (45 seconds) // Wisデータの生存時間(60秒)
                              // that is the waiting time of answers of all map-servers

unsigned char map_is_alone = 1; // define communication usage of map-server if it is alone

struct logs {
	char path[128];
	char prefix[32];
	char extension[24];
	unsigned short level;
	unsigned short length; //to store total length of path+prefix+extension
} logging[LOG_MAX] =
{
	{ "log/atcommand/", "", "log", 40, 256 },
	{ "log/trade/", "", "log", 0, 256 },
	{ "log/script/", "", "log", 0, 256 },
	{ "log/vending/", "", "log", 0, 256 }
};

char tmp_sql[MAX_SQL_BUFFER];
int tmp_sql_len;

short party_share_level = 10;
short guild_extension_bonus = GUILD_EXTENTION_BONUS;
unsigned char log_inter = 1; // log inter or not


struct WisData {
	int id;
	unsigned char count; // number of map-servers
	unsigned int tick;
	char src[25];
} *wis_db = NULL;
static int wis_db_num = 0;
static int wisid = 0; // identify each wisp message


// recv. packet list // 受信パケット長リスト
int inter_recv_packet_length[] = {
	-1,-1, 7,-1, -1, 6, -1, -1, -1,-1, 0, 0,  0, 0,  0, 0, // 0x3000-0x300f
	 6,-1, 0, 0,  0, 0,  0,  0, 10,-1, 0, 0,  0, 0,  0, 0, // 0x3010-0x301f
	74, 6,52,14, 10,29,  6, -1, 34, 0, 0, 0,  0, 0,  0, 0, // 0x3020-0x302f
	-1, 6,-1, 0, 55,19,  6, -1, 14,-1,-1,-1, 18,19,186,-1, // 0x3030-0x303f
	 5, 9, 0, 0,  0, 0,  0,  0,  0, 0, 0, 0,  0, 0,  0, 0, // 0x3040-0x304f
	 0, 0, 0, 0,  0, 0,  0,  0,  0, 0, 0, 0,  0, 0,  0, 0, // 0x3050-0x305f
	 0, 0, 0, 0,  0, 0,  0,  0,  0, 0, 0, 0,  0, 0,  0, 0, // 0x3060-0x306f
	 0, 0, 0, 0,  0, 0,  0,  0,  0, 0, 0, 0,  0, 0,  0, 0, // 0x3070-0x307f
	48,14,-1, 6,  0, 0,  0,  0,  0, 0, 0, 0,  0, 0,  0, 0, // 0x3080-0x308f
};

/*===============================================
 * Parses map-server logging requests
 * Logging types:
 * 0 -> Atcommands | LOG_ATCOMMAND
 * 1 -> Trades     | LOG_TRADE
 * 2 -> Scripts    | LOG_SCRIPT
 * 3 -> Vending    | LOG_VENDING
 *-----------------------------------------------
 */
static inline void mapif_parse_LogSaveReq(unsigned char type, const char *log_mes) { // 0x3008 <packet_len>.w <log_type>.B <message>.?B
	int log_fp, tmpstr_len;
	struct timeval tv;
	time_t now;
	char tmpstr[30];
	char *filename;

	if(type >= LOG_MAX)
		return;

	now = time(NULL);

	CALLOC(filename, char, logging[type].length + 20);

	tmpstr_len = sprintf(filename, "%s", logging[type].path);
	tmpstr_len += strftime(filename + tmpstr_len, 9, "%Y%m%d", localtime(&now));
	tmpstr_len += sprintf(filename + tmpstr_len, "%s", logging[type].prefix);
	sprintf(filename + tmpstr_len, ".%s", logging[type].extension);
	log_fp = open(filename, O_WRONLY | O_CREAT | O_APPEND, 0644);

	if (log_fp != -1) {
		// write date/time
		gettimeofday(&tv, NULL);
		memset(tmpstr, 0, sizeof(tmpstr));
		tmpstr_len = strftime(tmpstr, 24, "%m/%d/%Y %H:%M:%S", localtime(&now));
		tmpstr_len += sprintf(tmpstr + tmpstr_len, ".%03d: ", (int)tv.tv_usec / 1000);
		write(log_fp, tmpstr, tmpstr_len);
		// log message
		write(log_fp, log_mes, strlen(log_mes));
		close(log_fp);
	}

	FREE(filename);

	return;
}

//-----------------------------------------
// Sending map_is_alone flag to map-servers
//-----------------------------------------
void send_alone_map_server_flag() {
	int i;
	unsigned char c;

	// check value of the flag (if necessary)
	if (map_is_alone) {
		// count the number of online map-servers
		c = 0;
		for(i = 0; i < MAX_MAP_SERVERS; i++)
			if (server_fd[i] >= 0) // if map-server is online
				c++;
		// modify value of the flag if multi-map-servers
		if (c > 1)
			map_is_alone = 0;
	}

	WPACKETW(0) = 0x2b17;
	WPACKETB(2) = map_is_alone;
	WPACKETB(3) = logging[LOG_ATCOMMAND].level;
	WPACKETB(4) = logging[LOG_TRADE].level;
	WPACKETB(5) = logging[LOG_SCRIPT].level;
	WPACKETB(6) = logging[LOG_VENDING].level;
	mapif_sendall(7);

	return;
}

void inter_log_init(void) {
	short int i;

	printf("Checking and creating logging folder(s)...\n");
	for(i = LOG_BASE; i < LOG_MAX; i++) {
		logging[i].length = strlen(logging[i].path) + strlen(logging[i].prefix) + strlen(logging[i].extension);
#ifdef __WIN32
		mkdir(logging[i].path);
#else
		mkdir(logging[i].path, 0700);
#endif
	}
}

/*==========================================
 * read inter config file
 *------------------------------------------
 */
static void inter_config_read(const char *cfgName) { // not inline, called too often
	char line[65536];
	char w1[65536], w2[65536];
	FILE *fp;

	printf("Start reading interserver configuration: %s\n", cfgName);

	if ((fp = fopen(cfgName, "r")) == NULL) {
//		if ((fp = fopen("conf/inter_freya.conf", "r")) == NULL) { // not try default, possible infinite loop with import
			printf("Interserver configuration file not found: %s.\n", cfgName);
			return;
//		}
	}

	while(fgets(line, sizeof(line), fp)) { // fgets reads until maximum one less than size and add '\0' -> so, it's not necessary to add -1
		if ((line[0] == '/' && line[1] == '/') || line[0] == '\0' || line[0] == '\n' || line[0] == '\r')
			continue;
		// it's not necessary to remove 'carriage return ('\n' or '\r')

		if (sscanf(line, "%[^:]: %[^\r\n]", w1, w2) != 2)
			continue;

		if (strcasecmp(w1, "party_share_level") == 0) {
			party_share_level = atoi(w2);
			if (party_share_level < 0)
				party_share_level = 0;

		} else if (strcasecmp(w1, "guild_extension_bonus") == 0) {
			if (atoi(w2) >= 0 && atoi(w2) <= GUILD_EXTENTION_BONUS)
				guild_extension_bonus = atoi(w2);

// map-server options
		} else if (strcasecmp(w1, "map_is_alone") == 0) {
			map_is_alone = config_switch(w2);
			if (map_is_alone != 0)
				map_is_alone = 1;

		} else if (strcasecmp(w1, "log_atcommand_path") == 0) { //Path of the log file for GM commads
			memset(logging[LOG_ATCOMMAND].path, 0, sizeof(logging[LOG_ATCOMMAND].path));
			strncpy(logging[LOG_ATCOMMAND].path, w2, sizeof(logging[LOG_ATCOMMAND].path) - 1);
		} else if (strcasecmp(w1, "log_atcommand_extension") == 0) { //Extension name of the log file for GM commands
			memset(logging[LOG_ATCOMMAND].extension, 0, sizeof(logging[LOG_ATCOMMAND].extension));
			strncpy(logging[LOG_ATCOMMAND].extension, w2, sizeof(logging[LOG_ATCOMMAND].extension) - 1);
		} else if (strcasecmp(w1, "log_atcommand_prefix") == 0) { //Prefix name of the log file for GM commands
			memset(logging[LOG_ATCOMMAND].prefix, 0, sizeof(logging[LOG_ATCOMMAND].prefix));
			strncpy(logging[LOG_ATCOMMAND].prefix, w2, sizeof(logging[LOG_ATCOMMAND].prefix) - 1);
		} else if (strcasecmp(w1, "log_atcommand_level") == 0) { //Minimum level to log a GM command
			logging[LOG_ATCOMMAND].level = atoi(w2);

		} else if (strcasecmp(w1, "log_trade_path") == 0) { //Path of the log file for TRADE actions
			memset(logging[LOG_TRADE].path, 0, sizeof(logging[LOG_TRADE].path));
			strncpy(logging[LOG_TRADE].path, w2, sizeof(logging[LOG_TRADE].path) - 1);
		} else if (strcasecmp(w1, "log_trade_prefix") == 0) { //Prefix name of the log file for TRADE actions
			memset(logging[LOG_TRADE].prefix, 0, sizeof(logging[LOG_TRADE].prefix));
			strncpy(logging[LOG_TRADE].prefix, w2, sizeof(logging[LOG_TRADE].prefix) - 1);
		} else if (strcasecmp(w1, "log_trade_extension") == 0) { //Extension name of the log file for TRADE actions
			memset(logging[LOG_TRADE].extension, 0, sizeof(logging[LOG_TRADE].extension));
			strncpy(logging[LOG_TRADE].extension, w2, sizeof(logging[LOG_TRADE].extension) - 1);
		} else if (strcasecmp(w1, "log_trade_level") == 0) { //Minimum level to log a TRADE actions
			logging[LOG_TRADE].level = atoi(w2);

		} else if (strcasecmp(w1, "log_script_path") == 0) { //Path of the log file for SCRIPT actions
			memset(logging[LOG_SCRIPT].path, 0, sizeof(logging[LOG_SCRIPT].path));
			strncpy(logging[LOG_SCRIPT].path, w2, sizeof(logging[LOG_SCRIPT].path) - 1);
		} else if (strcasecmp(w1, "log_script_prefix") == 0) { //Prefix name of the log file for SCRIPT actions
			memset(logging[LOG_SCRIPT].prefix, 0, sizeof(logging[LOG_SCRIPT].prefix));
			strncpy(logging[LOG_SCRIPT].prefix, w2, sizeof(logging[LOG_SCRIPT].prefix) - 1);
		} else if (strcasecmp(w1, "log_script_extension") == 0) { //Extension name of the log file for SCRIPT actions
			memset(logging[LOG_SCRIPT].extension, 0, sizeof(logging[LOG_SCRIPT].extension));
			strncpy(logging[LOG_SCRIPT].extension, w2, sizeof(logging[LOG_SCRIPT].extension) - 1);
		} else if (strcasecmp(w1, "log_script_level") == 0) { //Minimum level to log a SCRIPT actions
			logging[LOG_SCRIPT].level = atoi(w2);		

		} else if (strcasecmp(w1, "log_vending_path") == 0) { //Path of the log file for VENDING actions
			memset(logging[LOG_VENDING].path, 0, sizeof(logging[LOG_VENDING].path));
			strncpy(logging[LOG_VENDING].path, w2, sizeof(logging[LOG_VENDING].path) - 1);
		} else if (strcasecmp(w1, "log_vending_prefix") == 0) { //Prefix name of the log file for VENDING actions
			memset(logging[LOG_VENDING].prefix, 0, sizeof(logging[LOG_VENDING].prefix));
			strncpy(logging[LOG_VENDING].prefix, w2, sizeof(logging[LOG_VENDING].prefix) - 1);
		} else if (strcasecmp(w1, "log_vending_extension") == 0) { //Extension name of the log file for VENDING actions
			memset(logging[LOG_VENDING].extension, 0, sizeof(logging[LOG_VENDING].extension));
			strncpy(logging[LOG_VENDING].extension, w2, sizeof(logging[LOG_VENDING].extension) - 1);
		} else if (strcasecmp(w1, "log_vending_level") == 0) { //Minimum level to log a VENDING actions
			logging[LOG_VENDING].level = atoi(w2);

		} else if (strcasecmp(w1, "mysql_server_ip") == 0) {
			memset(db_mysql_server_ip, 0, sizeof(db_mysql_server_ip));
			strncpy(db_mysql_server_ip, w2, sizeof(db_mysql_server_ip) - 1);
		} else if (strcasecmp(w1, "mysql_server_port") == 0) {
			db_mysql_server_port = atoi(w2);
		} else if (strcasecmp(w1, "mysql_server_id") == 0) {
			memset(db_mysql_server_id, 0, sizeof(db_mysql_server_id));
			strncpy(db_mysql_server_id, w2, sizeof(db_mysql_server_id) - 1);
		} else if (strcasecmp(w1, "mysql_server_pw") == 0) {
			memset(db_mysql_server_pw, 0, sizeof(db_mysql_server_pw));
			strncpy(db_mysql_server_pw, w2, sizeof(db_mysql_server_pw) - 1);
		} else if (strcasecmp(w1, "mysql_char_db") == 0) {
			memset(db_mysql_server_db, 0, sizeof(db_mysql_server_db));
			strncpy(db_mysql_server_db, w2, sizeof(db_mysql_server_db) - 1);
		} else if (strcasecmp(w1, "log_inter") == 0) {
			log_inter = config_switch(w2);
// import
		} else if (strcasecmp(w1, "import") == 0) {
			printf("inter_config_read: Import file: %s.\n", w2);
			inter_config_read(w2);
		}
	}
	fclose(fp);

	printf("Success reading interserver configuration!\n");

	return;
}

// Save log // ログ書き出し
void inter_log(char *fmt, ...) {
	char str[255], temp_str[511];
	va_list ap;

	if (!log_inter)
		return;

	va_start(ap, fmt);

	vsprintf(str, fmt, ap);
	mysql_escape_string(temp_str, str, strlen(str));
	sql_request("INSERT INTO `%s` (`time`, `log`) VALUES (NOW(), '%s')", interlog_db, temp_str);

	va_end(ap);

	return;
}

// セーブ
void inter_save() {
	return;
}

// initialize // 初期化
void inter_init(const char *file) {
	struct {
		unsigned int acc_reg_db 	: 1;
		unsigned int scdata_db 		: 1;
		unsigned int rank_db 			: 1;
		unsigned int adopt_mother	: 1;
		unsigned int adopt_father	: 1;
		unsigned int adopt_child	: 1;
	} flag;

	memset(&flag, 0, sizeof(flag));
	printf("Initializing Inter-server...\n");
	inter_config_read(file);

	// DB connection initialized
	sql_init();

	// Update DB if necessary
	sql_request("SHOW TABLES");
	while (sql_get_row()) {
		if (strcmp(sql_get_string(0), "account_reg_db") == 0)
			flag.acc_reg_db = 1;
		else if (strcmp(sql_get_string(0), statuschange_db) == 0)
			flag.scdata_db = 1;
		else if (strcmp(sql_get_string(0), rank_db) == 0)
			flag.rank_db = 1;
	}

	// create `account_reg_db` table if it doesn't exist
	if (!flag.acc_reg_db) {
		if (sql_request("CREATE TABLE IF NOT EXISTS `account_reg_db` ("
						"  `account_id` int(11) NOT NULL default '0',"
						"  `str` varchar(32) NOT NULL default '',"
						"  `value` int(11) NOT NULL default '0',"
						"  PRIMARY KEY (`account_id`, `str`)"
						") TYPE = MyISAM") == 0) {
			printf(CL_RED "ERROR: Char-server could not create `account_reg_db` SQL table." CL_RESET "\n");
			printf("       Char-server must be stopped.\n");
			exit(1);
		}
		printf("The char-server has created the `account_reg_db` SQL table.\n");
		// insert values
		sql_request("INSERT INTO `account_reg_db` (`account_id`, `str`, `value`) SELECT `account_id`, `str`, `value` FROM `%s` WHERE `type`='2'", global_reg_value);
		// delete values
		sql_request("DELETE FROM `%s` WHERE `type`='2'", global_reg_value);
		// delete PRIMARY KEY and create a new PRIMARY KEY
		sql_request("ALTER TABLE `%s` DROP PRIMARY KEY", global_reg_value);
		sql_request("ALTER TABLE `%s` ADD PRIMARY KEY (`char_id`, `str`)", global_reg_value);
		// delete unused column
		sql_request("ALTER TABLE `%s` DROP COLUMN `account_id`", global_reg_value);
		sql_request("ALTER TABLE `%s` DROP COLUMN `type`", global_reg_value);
		// optimize global_reg_value TABLE
		sql_request("OPTIMIZE TABLE `global_reg_value`");
	}

	// create `statuschange` table if it doesn't exist
	if (!flag.scdata_db) {
		if (sql_request("CREATE TABLE IF NOT EXISTS %s ("
						" `char_id` int(11) unsigned NOT NULL,"
						" `type` smallint(11) unsigned NOT NULL,"
						" `tick` int(11) NOT NULL,"
						" `val1` int(11) NOT NULL default '0',"
						" `val2` int(11) NOT NULL default '0',"
						" `val3` int(11) NOT NULL default '0',"
						" `val4` int(11) NOT NULL default '0',"
						"  KEY (`char_id`)"
						") TYPE = MyISAM", statuschange_db) == 0) {
			printf(CL_RED "ERROR: Char-server could not create %s SQL table." CL_RESET "\n", statuschange_db);
			printf("       Char-server must be stopped.\n");
			exit(1);
		}
		printf("The char-server has created the %s SQL table.\n", statuschange_db);
	}

	// create `ranking` table if doesn't exist
	if (!flag.rank_db) {
		if (sql_request("CREATE TABLE IF NOT EXISTS %s ("
						" `char_id` int(10) unsigned NOT NULL,"
						" `class` tinyint(1) unsigned NOT NULL,"
						" `points` int(10) NOT NULL"
						") TYPE = MyISAM", rank_db) == 0) {
			printf(CL_RED "ERROR: Char-server could not create %s SQL table." CL_RESET "\n", rank_db);
			printf("       Char-server must be stopped.\n");
			exit(1);
		}
		printf("The char-server has created the %s SQL table.\n", rank_db);
	}

	sql_request("SHOW COLUMNS FROM `%s` like 'die_counter'", char_db);
	if (!sql_num_rows()) {
		if (sql_request("ALTER TABLE `%s` ADD `die_counter` int(11) NOT NULL default '0' AFTER `save_y`", char_db)== 0) {
			printf(CL_RED "ERROR: Char-server could not alter SQL table %s." CL_RESET "\n", char_db);
			printf("       Char-server must be stopped.\n");
			exit(1);
		}
		printf("The char-server has added column `die_counter` to the %s SQL table.\n", char_db);
		printf("Transferring `die_counter` data...\n");

		// retrieve PC_DIE_COUNTER values from SQL Table `global_reg_value`
		sql_request("SELECT `char_id`, `value` FROM `%s` WHERE `str` = 'PC_DIE_COUNTER'", global_reg_value);

		// insert PC_DIE_COUNTER values into SQL Table `char`
		while (sql_get_row())
			sql_request("UPDATE `%s` set `die_counter` = '%d' WHERE `char_id` = '%d'", char_db, sql_get_integer(1), sql_get_integer(0));

		// delete values
		sql_request("DELETE FROM `%s` WHERE `str` = 'PC_DIE_COUNTER'", global_reg_value);

		printf("Transfer of `die_counter` data done.\n");
	}

	//Adoption system [GoodKat]
	sql_request("SHOW COLUMNS FROM `%s`", char_db);
	while (sql_get_row()) {
   	if (strcmp(sql_get_string(0), "father") == 0)
			flag.adopt_father = 1;
		else if (strcmp(sql_get_string(0), "mother") == 0)
			flag.adopt_mother = 1;
		else if (strcmp(sql_get_string(0), "child") == 0)
			flag.adopt_child = 1;
	}
	if (flag.adopt_father != 1 || flag.adopt_mother != 1 || flag.adopt_child != 1) {
		printf(CL_RED "ERROR: Missing rows in %s table: father, mother, child. \nConsider upgrading it manually!" CL_RESET "\n", char_db);
		exit(1);
	}

	wis_db = NULL;
	wis_db_num = 0;
	wisid = 0; // identify each wisp message

	inter_party_init();
	inter_guild_init();
	inter_storage_init();

	inter_pet_init();

	inter_log_init();

	// Whether the failure of previous wisp/page transmission (timeout)
	add_timer_func_list(check_ttl_wisdata, "check_ttl_wisdata");
	add_timer_interval(gettick_cache + 30000, check_ttl_wisdata, 0, 0, 30000);

	return;
}

// マップサーバー接続
void inter_mapif_init(int fd) {
	inter_guild_mapif_init(fd);

	return;
}

//--------------------------------------------------------
// sended packets to map-server

// Wisp/page transmission result to map-server
static void mapif_wis_end(char * sender_name, unsigned char flag) { // not inline, called too often
	WPACKETW( 0) = 0x3802; // 0x3802 <sender_name>.24B <flag>.B (flag: 0: success to send wisper, 1: target character is not loged in?, 2: ignored by target)
	strncpy(WPACKETP(2), sender_name, 24);
	WPACKETB(26) = flag; // flag: 0: success to send wisper, 1: target character is not loged in?, 2: ignored by target
	mapif_sendall(27); // not: mapif_send(wd->fd, 27);, because player can have change to map-server
//	printf("inter server wis_end: flag: %d.\n", flag);

	return;
}

//--------------------------------------------------------

// timer to delete elapsed wisps (with no answer)
int check_ttl_wisdata(int tid, unsigned int tick, int id, int data) {
	int i;
	
	for(i = 0; i < wis_db_num; i++) {
		if (DIFF_TICK(tick, wis_db[i].tick) > WISDATA_TTL) {
			//printf("inter: wis data id=%d time out : from %s to %s\n", wis_db[i].id, wis_db[i].src, wis_db[i].dst);
			mapif_wis_end(wis_db[i].src, 1); // flag: 0: success to send wisper, 1: target character is not loged in?, 2: ignored by target
			if (wis_db_num == 1) {
				FREE(wis_db);
			} else {
				if (i != wis_db_num - 1)
					memcpy(&wis_db[i], &wis_db[wis_db_num - 1], sizeof(struct WisData));
				REALLOC(wis_db, struct WisData, wis_db_num - 1);
			}
			wis_db_num--;
			// reduce 'i' too, to check again the same position
			i--;
		}
	}

	return 0;
}

//--------------------------------------------------------
// received packets from map-server

// GM message sending - GMメッセージ送信
static inline void mapif_parse_GMmessage(int fd) { // 0x3000/0x3800 <packet_len>.w <message>.?B
	WPACKETW(0) = 0x3800;
	memcpy(WPACKETP(2), RFIFOP(fd,2), RFIFOW(fd,2) - 2);
	mapif_sendallwos(fd, WPACKETW(2)); // sender server have send message to these local players
	//printf(CL_BLUE " inter server: GM[len:%d] - '%s'" CL_RESET "\n", RFIFOW(fd, 2), RFIFOP(fd, 4));

	return;
}

// Colored GM message sending
static inline void mapif_parse_announce(int fd) { // 0x3009/0x3809 <packet_len>.w <color>.L <flag>.L <message>.?B
	WPACKETW(0) = 0x3809;
	memcpy(WPACKETP(2), RFIFOP(fd,2), RFIFOW(fd,2) - 2);
	mapif_sendallwos(fd, WPACKETW(2)); // sender server have send message to these local players
	//printf(CL_BLUE " inter server: colored GM[len:%d] - '%s'" CL_RESET "\n", RFIFOW(fd, 2), RFIFOP(fd, 12));

	return;
}

// Wisp/page request to send
static inline void mapif_parse_WisRequest(int fd) { // 0x3001/0x3801 <packet_len>.w (<w_id_0x3801>.L) <sender_GM_level>.B <sender_name>.24B <nick_name>.24B <message>.?B
	char player[25]; // 24 + NULL
	int idx;
	int i;
	char t_player[49]; // (24 * 2) + NULL

	if (RFIFOW(fd,2) <= 53 + 1) { // normaly, impossible, but who knows... (53 + 1 (NULL))
		printf("inter: Wis message doesn't exist (void).\n");
		return;
	}

	// search if character exists before to ask all map-servers
	strncpy(player, RFIFOP(fd,29), 24);
	player[24] = '\0';

	// search player name in memory (-> -1, player is offline or doesn't exist; -2 player was found, but with incorrect sensitive case)
	if ((idx = char_nick2idx(player)) == -2) { // -1: no char with this name, -2: exact sensitive name not found
		// if not found in memory, try to found if name is unique or offline
		mysql_escape_string(t_player, player, strlen(player));
		sql_request("SELECT `name` FROM `%s` WHERE `name`='%s'", char_db, t_player);
		i = 0;
		while(sql_get_row()) {
			if (strcmp(sql_get_string(0), player) == 0) {
				// player is offline (not in memory)
				WPACKETW( 0) = 0x3802; // 0x3802 <sender_name>.24B <flag>.B (flag: 0: success to send wisper, 1: target character is not loged in?, 2: ignored by target)
				memcpy(WPACKETP(2), RFIFOP(fd,5), 24);
				WPACKETB(26) = 1; // flag: 0: success to send wisper, 1: target character is not loged in?, 2: ignored by target
				mapif_send(fd, 27);
				return;
			}
			// to be sure of the correct name, rewrite it
			strncpy(RFIFOP(fd,29), sql_get_string(0), 24);
			i++;
		}
		// if only 1 player found (name was in an incorrect sensitiv case and was in memory (online?))
		if (i == 1) {
			idx = -1;
			// we try to know if player is online
			for(i = 0; i < char_num; i++)
				if (strcmp(char_dat[i].name, RFIFOP(fd,29)) == 0) {
					idx = i;
					break;
				}
		}
	}
	if (idx < 0 || online_chars[idx] == 0) { // to check online here, check visible and hidden characters
		WPACKETW( 0) = 0x3802; // 0x3802 <sender_name>.24B <flag>.B (flag: 0: success to send wisper, 1: target character is not loged in?, 2: ignored by target)
		memcpy(WPACKETP(2), RFIFOP(fd,5), 24);
		WPACKETB(26) = 1; // flag: 0: success to send wisper, 1: target character is not loged in?, 2: ignored by target
		mapif_send(fd, 27);
		return;
	// Character exists. So, ask all map-servers
	} else {
		// if source is destination, don't ask other servers.
		if (strcmp(RFIFOP(fd,5), RFIFOP(fd,29)) == 0) {
			WPACKETW( 0) = 0x3802; // 0x3802 <sender_name>.24B <flag>.B (flag: 0: success to send wisper, 1: target character is not loged in?, 2: ignored by target)
			memcpy(WPACKETP(2), RFIFOP(fd,5), 24);
			WPACKETB(26) = 1; // flag: 0: success to send wisper, 1: target character is not loged in?, 2: ignored by target
			mapif_send(fd, 27);
		} else {
			// save message
			if (wis_db_num == 0) {
				MALLOC(wis_db, struct WisData, 1);
			} else {
				REALLOC(wis_db, struct WisData, wis_db_num + 1);
			}
			wis_db[wis_db_num].id = ++wisid;
			strncpy(wis_db[wis_db_num].src, RFIFOP(fd,5), 24);
			wis_db[wis_db_num].tick = gettick_cache;
			// ask all map-servers
			WPACKETW(0) = 0x3801; // 0x3001/0x3801 <packet_len>.w (<w_id_0x3801>.L) <sender_GM_level>.B <sender_name>.24B <nick_name>.24B <message>.?B
			WPACKETW(2) = 57 + RFIFOW(fd,2) - 53; // including NULL
			WPACKETL(4) = wis_db[wis_db_num].id;
			WPACKETB(8) = RFIFOB(fd,4); // need for pm_gm_not_ignored option
			strncpy(WPACKETP( 9), RFIFOP(fd, 5), 24);
			strncpy(WPACKETP(33), RFIFOP(fd,29), 24);
			strncpy(WPACKETP(57), RFIFOP(fd,53), RFIFOW(fd,2) - 53); // including NULL
			wis_db[wis_db_num].count = mapif_sendall(WPACKETW(2));
			// increase wisp message number
			wis_db_num++;
		}
	}

	return;
}

// Wisp/page transmission result
static inline void mapif_parse_WisReply(int fd) { // 0x3002 <Wis_id>.L <flag>.B (flag: 0: success to send wisper, 1: target character is not loged in?, 2: ignored by target)
	int id, i;
	unsigned char flag;

	id = RFIFOL(fd,2);

	for(i = 0; i < wis_db_num; i++)
		if (wis_db[i].id == id) {
			// if found, decrease answer
			wis_db[i].count--;
			// if valid answer or not more answer
			flag = RFIFOB(fd,6);
			if (wis_db[i].count <= 0 || flag != 1) {
				// send answer
				mapif_wis_end(wis_db[i].src, flag); // flag: 0: success to send wisper, 1: target character is not loged in?, 2: ignored by target
				// delete wisp from memory
				if (wis_db_num == 1) {
					FREE(wis_db);
				} else {
					if (i != wis_db_num - 1)
						memcpy(&wis_db[i], &wis_db[wis_db_num - 1], sizeof(struct WisData));
					REALLOC(wis_db, struct WisData, wis_db_num - 1);
				}
				wis_db_num--;
			}
			return;
		}

	// if not found, this wisp was probably suppress before, because it was timeout or because of target was found on another map-server

	return;
}

// Received wisp message from map-server for ALL gm (just copy the message and resends it to ALL map-servers)
static inline void mapif_parse_WisToGM(int fd) {
	WPACKETW(0) = 0x3803; // 0x3003/0x3803 <packet_len>.w <wispname>.24B <min_gm_level>.w <message>.?B
	memcpy(WPACKETP(2), RFIFOP(fd,2), RFIFOW(fd,2)-2);
	mapif_sendallwos(fd, WPACKETW(2));

	return;
}

// Save account_reg - アカウント変数保存要求
void mapif_parse_AccReg(int fd) { // 0x3004 <packet_len>.w <account_id>.L account_reg.structure.*B
	int p, account_id;
	struct global_reg *reg;
	char temp_str[65]; // 32 + NULL, 32 * 2 + NULL

	account_id = RFIFOL(fd,4);
	//printf("mapif_parse_AccReg: account %d, num of account_reg: %d.\n", account_id, (RFIFOW(fd,2) - 8) / sizeof(struct global_reg));
	if (account_id <= 0)
		return;

	//`account_reg_db` (`account_id`, `str`, `value`)
	sql_request("DELETE FROM `account_reg_db` WHERE `account_id`='%d'", account_id);
	// insert here.
	tmp_sql_len = 0;
	temp_str[64] = '\0';
	for(p = 8; p < RFIFOW(fd,2); p += sizeof(struct global_reg)) {
		reg = (struct global_reg*)RFIFOP(fd,p);
		mysql_escape_string(temp_str, reg->str, strlen(reg->str));
		if (tmp_sql_len == 0)
			tmp_sql_len = sprintf(tmp_sql, "INSERT INTO `account_reg_db` (`account_id`, `str`, `value`) VALUES ('%d', '%s', '%d')",
			                      account_id, temp_str, reg->value);
		else
			tmp_sql_len += sprintf(tmp_sql + tmp_sql_len, ", ('%d', '%s', '%d')",
			                       account_id, temp_str, reg->value);
		// don't check length here, 65536 is enough for all information.
	}
	if (tmp_sql_len != 0) {
		//printf("%s\n", tmp_sql);
		sql_request(tmp_sql);
	}

	// send answer (accound_id saved) to remove flag
	WPACKETW(0) = 0x3805; // 0x3805 <account_id>.L
	WPACKETL(2) = account_id;
	SENDPACKET(fd, 6);

	return;
}

// Request the value of account_reg
void mapif_parse_AccRegRequest(int fd, int account_id) { // 0x3005 <account_id>.L
	int j, p;

//	printf("mapif: accreg request\n");

	p = 8;
	j = 0;
	//`account_reg_db` (`account_id`, `str`, `value`)
	sql_request("SELECT `str`, `value` FROM `account_reg_db` WHERE `account_id`='%d'", account_id);
	while (sql_get_row())
		if (strlen(sql_get_string(0)) > 0 && sql_get_integer(1) != 0 && j < ACCOUNT_REG_NUM) {
			strncpy(WPACKETP(p), sql_get_string(0), 32);
			WPACKETL(p + 32) = sql_get_integer(1);
			j++;
			p += 36;
		}

	if (j > 0) {
		WPACKETW(0) = 0x3804; // 0x3804 <packet_len>.w <account_id>.L {<account_reg_str>.32B <account_reg_value>.L}.x
		WPACKETW(2) = p;
		WPACKETL(4) = account_id;
		SENDPACKET(fd, p);
	}

	return;
}

// Received Main message from map-server for ALL players (just copy the message and resends it to ALL map-servers)
static inline void mapif_parse_MainMessage(int fd) {
	memcpy(WPACKETP(0), RFIFOP(fd,0), RFIFOW(fd,2));
	WPACKETW(0) = 0x3806; // 0x3006/0x3806 <packet_len>.w <wispname>.24B <message>.?B
	mapif_sendallwos(fd, WPACKETW(2));

	return;
}

// Received message from map-server for ALL GMs (just copy the message and resends it to ALL map-servers)
static inline void mapif_parse_MessageToGM(int fd) { // 0x3007/0x3807 <packet_len>.w <wispname>.24B <message>.?B
	memcpy(WPACKETP(0), RFIFOP(fd,0), RFIFOW(fd,2));
	WPACKETW(0) = 0x3807; // 0x3007/0x3807 <packet_len>.w <wispname>.24B <message>.?B
	mapif_sendallwos(fd, WPACKETW(2));

	return;
}

//--------------------------------------------------------
// map server からの通信（１パケットのみ解析すること）
// エラーなら0(false)、処理できたなら1、
// パケット長が足りなければ2をかえさなければならない
//--------------------------------------------------------
int inter_parse_frommap(int fd) {
	unsigned short cmd;
	int len;

	// inter鯖管轄かを調べる
	cmd = RFIFOW(fd,0);
	if (cmd < 0x3000 || cmd >= 0x3000 + (sizeof(inter_recv_packet_length) / sizeof(inter_recv_packet_length[0])))
		return 0; // unknow packet

	// パケット長を調べる
	len = inter_recv_packet_length[cmd - 0x3000];
	if (len == 0)
		return 0; // unknow packet
	else if (len == -1) { // 可変パケット長
		if (RFIFOREST(fd) < 4) // パケット長が未着
			return 2; // need more packet
		len = RFIFOW(fd,2);
	}
	if (RFIFOREST(fd) < len) // パケットが未着
		return 2; // need more packet

	switch(cmd) {
	case 0x3000: mapif_parse_GMmessage(fd); break; // 0x3000/0x3800 <packet_len>.w <message>.?B
	case 0x3001: mapif_parse_WisRequest(fd); break; // 0x3001/0x3801 <packet_len>.w (<w_id_0x3801>.L) <sender_GM_level>.B <sender_name>.24B <nick_name>.24B <message>.?B
	case 0x3002: mapif_parse_WisReply(fd); break; // 0x3002 <Wis_id>.L <flag>.B (flag: 0: success to send wisper, 1: target character is not loged in?, 2: ignored by target)
	case 0x3003: mapif_parse_WisToGM(fd); break; // 0x3003/0x3803 <packet_len>.w <wispname>.24B <min_gm_level>.w <message>.?B
	case 0x3004: mapif_parse_AccReg(fd); break; // 0x3004 <packet_len>.w <account_id>.L account_reg.structure.*B
//	case 0x3005: mapif_parse_AccRegRequest(fd, RFIFOL(fd,2)); break; // 0x3005 <account_id>.L // now send at same moment of character (synchronized)
	case 0x3006: mapif_parse_MainMessage(fd); break; // 0x3006/0x3806 <packet_len>.w <wispname>.24B <message>.?B
	case 0x3007: mapif_parse_MessageToGM(fd); break; // 0x3007/0x3807 <packet_len>.w <wispname>.24B <message>.?B
	case 0x3008: mapif_parse_LogSaveReq(RFIFOB(fd,4), RFIFOP(fd,5)); break; // 0x3008 <packet_len>.w <log_type>.B <message>.?B (types: 0 GM commands, 1: Trades, 2: Scripts, 3: Vending)
	case 0x3009: mapif_parse_announce(fd); break; // 0x3009/0x3809 <packet_len>.w <color>.L <flag>.L <message>.?B
	default:
		if (inter_party_parse_frommap(fd))
			break;
		if (inter_guild_parse_frommap(fd))
			break;
		if (inter_storage_parse_frommap(fd))
			break;
		if (inter_pet_parse_frommap(fd))
			break;
		return 0; // unknow packet
	}
	RFIFOSKIP(fd, len);

	return 1; // processed
}
