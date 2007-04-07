// $Id: inter.c 495 2006-04-07 09:02:46Z Harbin $

#include <config.h>

#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>
#include <stdarg.h>
#include <stdint.h>

#ifdef TXT_ONLY
#include <mmo.h>
#include "../common/socket.h"
#include "../common/timer.h"
#endif /* TXT_ONLY */

#include "char.h"
#include "inter.h"
#include "int_party.h"
#include "int_guild.h"
#include "int_storage.h"
#include "int_pet.h"
#include "../common/lock.h"
#include "../common/malloc.h"
#include "../common/utils.h"


#define WISDATA_TTL (45*1000)

unsigned char map_is_alone = 1;

char log_gm_file[1024] = "log/atcommand.log";
short log_gm_level = 40;
char log_trade_file[1024] = "log/trade.log";
short log_trade_level = 0;
char log_script_file[1024] = "log/script.log";
short log_script_level = 0;
char log_vending_file[1024] = "log/vending.log";
short log_vending_level = 0;
char log_refine_file[1024] = "log/refine.log";
short log_refine_level = 0;
char log_npcsell_file[1024] = "log/npcsell.log";
short log_npcsell_level = 0;
char log_pcdrop_file[1024] = "log/pcdrop.log";
short log_pcdrop_level = 0;
char log_pcpick_file[1024] = "log/pcpick.log";
short log_pcpick_level = 0;


#ifdef TXT_ONLY
char inter_log_filename[1024] = "log/inter.log";

char accreg_txt[1024] = "save/accreg.txt";
static struct accreg_db {
	int account_id;
	char *reg;
} *accreg_db = NULL;
static int accreg_db_num = 0;
static char unsigned accreg_txt_dirty = 0; // to know if we must save account registers file

static char line[65536];
#else // TXT_ONLY -> USE_SQL
char tmp_sql[MAX_SQL_BUFFER];
int tmp_sql_len;
#endif // USE_SQL


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
	-1,-1, 7,-1, -1, 6, -1, -1, -1, 0, 0, 0,  0, 0,  0, 0, // 0x3000-0x300f
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
 * This function logs informations in different file
 *-----------------------------------------------
 */
static inline void log_atcommand(unsigned char log_type, const char *log_mes) { // 0x3008 <packet_len>.w <log_type>.B <message>.?B (types: 1 GM commands, 2: Trades, 3: Scripts, 4: Vending)
	int log_fp;
	struct timeval tv;
	time_t now;
	char tmpstr[30];
	int tmpstr_len;

	switch (log_type) {
	case 1:
		log_fp = open(log_gm_file, O_WRONLY | O_CREAT | O_APPEND, 0644);
		break;
	case 2:
		log_fp = open(log_trade_file, O_WRONLY | O_CREAT | O_APPEND, 0644);
		break;
	case 3:
		log_fp = open(log_script_file, O_WRONLY | O_CREAT | O_APPEND, 0644);
		break;
	case 4:
		log_fp = open(log_vending_file, O_WRONLY | O_CREAT | O_APPEND, 0644);
		break;
	case 5:
		log_fp = open(log_refine_file, O_WRONLY | O_CREAT | O_APPEND, 0644);
		break;
	case 6:
		log_fp = open(log_npcsell_file, O_WRONLY | O_CREAT | O_APPEND, 0644);
		break;
	case 7:
		log_fp = open(log_pcdrop_file, O_WRONLY | O_CREAT | O_APPEND, 0644);
		break;
	case 8:
		log_fp = open(log_pcpick_file, O_WRONLY | O_CREAT | O_APPEND, 0644);
		break;
	default:
		return;
	}

	if (log_fp != -1) {
		// write date/time
		gettimeofday(&tv, NULL);
		memset(tmpstr, 0, sizeof(tmpstr));
		now = time(NULL);
		tmpstr_len = strftime(tmpstr, 24, "%m/%d/%Y %H:%M:%S", localtime(&now));
		tmpstr_len += sprintf(tmpstr + tmpstr_len, ".%03d: ", (int)tv.tv_usec / 1000);
		write(log_fp, tmpstr, tmpstr_len);
		// log message
		write(log_fp, log_mes, strlen(log_mes));
		close(log_fp);
	}

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
	WPACKETB(3) = log_gm_level;
	WPACKETB(4) = log_trade_level;
	WPACKETB(5) = log_script_level;
	WPACKETB(6) = log_vending_level;
	WPACKETB(7) = log_refine_level;
	WPACKETB(8) = log_npcsell_level;
	WPACKETB(9) = log_pcdrop_level;
	WPACKETB(10)= log_pcpick_level;
	mapif_sendall(11);

	return;
}

//--------------------------------------------------------
// Initialize acc registers
//--------------------------------------------------------
#ifdef TXT_ONLY
static inline void inter_accreg_init()
{
	char buf[65536]; // .c file static local line[65536]
	FILE *fp;
	int account_id, c, i, v;

	accreg_db = NULL;
	accreg_db_num = 0;
	accreg_txt_dirty = 0; // to know if we must save account registers file

	if ((fp = fopen(accreg_txt, "r")) == NULL)
		if ((fp = fopen("save/accreg.txt", "r")) == NULL) // try default file
			return;

	c = 0;
	while(fgets(line, sizeof(line), fp)) { // fgets reads until maximum one less than size and add '\0' -> so, it's not necessary to add -1
		c++;
		if ((line[0] == '/' && line[1] == '/') || line[0] == '\0' || line[0] == '\n' || line[0] == '\r')
			continue;
		/* remove carriage return if exist */
		while(line[0] != '\0' && (line[(i = strlen(line) - 1)] == '\n' || line[i] == '\r'))
			line[i] = '\0';

		if (sscanf(line, "%d\t%[^,],%d ", &account_id, buf, &v) == 3) { // check account and at least 1 value
			// check if account reg alrady exist
			for(i = 0; i < accreg_db_num; i++)
				if (accreg_db[i].account_id == account_id)
					break;
			if (i != accreg_db_num)
			{
				printf(CL_YELLOW "Warning: " CL_RESET "duplicate account id '%d' on account register (line %d) \n", account_id, c);
				accreg_txt_dirty = 1;
				continue;
			}

			if(accreg_db_num == 0)
			{
				MALLOC(accreg_db, struct accreg_db, 1);
			} else {
				REALLOC(accreg_db, struct accreg_db, accreg_db_num + 1);
			}
			accreg_db[accreg_db_num].account_id = account_id;
			MALLOC(accreg_db[accreg_db_num].reg, char, strlen(line) + 1); // + NULL
			strcpy(accreg_db[accreg_db_num].reg, line);
			accreg_db_num++;
		} else {
			printf(CL_YELLOW "Warning: " CL_RESET "corrupted line on account register (line %d) \n", c);
			accreg_txt_dirty = 1;
		}
	}
	fclose(fp);

//	printf("inter: %s read done (%d)\n", accreg_txt, c);

	return;
}

//--------------------------------------------------------
// save account registers file
//--------------------------------------------------------
static inline void inter_accreg_save() {
	FILE *fp;
	int lock, i;

	if (accreg_txt_dirty == 0)
		return;

	if ((fp = lock_fopen(accreg_txt, &lock)) == NULL)
	{
		printf(CL_YELLOW "Warning: " CL_RESET "failed to save account register. data lost \n");
		return;
	}

	for(i = 0; i < accreg_db_num; i++)
		fprintf(fp, "%s" RETCODE, accreg_db[i].reg);

	lock_fclose(fp, accreg_txt, &lock);
//	printf("inter: %s saved.\n", accreg_txt);

	accreg_txt_dirty = 0;

	return;
}
#endif /* TXT_ONLY */

/*==========================================
 * read inter config file
 *------------------------------------------
 */
static void inter_config_read(const char *cfgName) { // not inline, called too often
#ifdef TXT_ONLY
	// .c file static local line[65536]
#else /* TXT_ONLY -> USE_SQL*/
	char line[65536];
#endif /* USE_SQL */
	char w1[65536], w2[65536];
	FILE *fp;

	if((fp = fopen(cfgName, "r")) == NULL)
	{
			printf(CL_WHITE "Error: " CL_RESET "failed to read inter-server configuration file \n");
			return;
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
		} else if (strcasecmp(w1, "log_gm_file") == 0) { // name of the log file for GM commands
			memset(log_gm_file, 0, sizeof(log_gm_file));
			strncpy(log_gm_file, w2, sizeof(log_gm_file) - 1);
		} else if (strcasecmp(w1, "log_gm_level") == 0) { // minimum level to log a GM command
			log_gm_level = atoi(w2);
		} else if (strcasecmp(w1, "log_trade_file") == 0) {
			memset(log_trade_file, 0, sizeof(log_trade_file));
			strncpy(log_trade_file, w2, sizeof(log_trade_file) - 1);
		} else if (strcasecmp(w1, "log_trade_level") == 0) {
			log_trade_level = atoi(w2);
		} else if (strcasecmp(w1, "log_script_file") == 0) {
			memset(log_script_file, 0, sizeof(log_script_file));
			strncpy(log_script_file, w2, sizeof(log_script_file) - 1);
		} else if (strcasecmp(w1, "log_script_level") == 0) {
			log_script_level = atoi(w2);
		} else if (strcasecmp(w1, "log_vending_file") == 0) {
			memset(log_vending_file, 0, sizeof(log_vending_file));
			strncpy(log_vending_file, w2, sizeof(log_vending_file) - 1);
		} else if (strcasecmp(w1, "log_vending_level") == 0) {
			log_vending_level = atoi(w2);
		} else if (strcasecmp(w1, "log_refine_file") == 0) {
			memset(log_refine_file, 0, sizeof(log_refine_file));
			strncpy(log_refine_file, w2, sizeof(log_refine_file) - 1);
		} else if (strcasecmp(w1, "log_refine_level") == 0) {
			log_refine_level = atoi(w2);
		} else if (strcasecmp(w1, "log_refine_file") == 0) {
			memset(log_refine_file, 0, sizeof(log_refine_file));
			strncpy(log_refine_file, w2, sizeof(log_refine_file) - 1);
		} else if (strcasecmp(w1, "log_refine_level") == 0) {
			log_refine_level = atoi(w2);
		} else if (strcasecmp(w1, "log_npcsell_file") == 0) {
			memset(log_npcsell_file, 0, sizeof(log_npcsell_file));
			strncpy(log_npcsell_file, w2, sizeof(log_npcsell_file) - 1);
		} else if (strcasecmp(w1, "log_npcsell_level") == 0) {
			log_npcsell_level = atoi(w2);
		} else if (strcasecmp(w1, "log_pcdrop_file") == 0) {
			memset(log_pcdrop_file, 0, sizeof(log_pcdrop_file));
			strncpy(log_pcdrop_file, w2, sizeof(log_pcdrop_file) - 1);
		} else if (strcasecmp(w1, "log_pcdrop_level") == 0) {
			log_pcdrop_level = atoi(w2);
		} else if (strcasecmp(w1, "log_pcpick_file") == 0) {
			memset(log_pcpick_file, 0, sizeof(log_pcpick_file));
			strncpy(log_pcpick_file, w2, sizeof(log_pcpick_file) - 1);
		} else if (strcasecmp(w1, "log_pcpick_level") == 0) {
			log_pcpick_level = atoi(w2);

#ifdef TXT_ONLY
		} else if (strcasecmp(w1, "storage_txt") == 0) {
			memset(storage_txt, 0, sizeof(storage_txt));
			strncpy(storage_txt, w2, sizeof(storage_txt) - 1);
		} else if (strcasecmp(w1, "party_txt") == 0) {
			memset(party_txt, 0, sizeof(party_txt));
			strncpy(party_txt, w2, sizeof(party_txt) - 1);
		} else if (strcasecmp(w1, "guild_txt") == 0) {
			memset(guild_txt, 0, sizeof(guild_txt));
			strncpy(guild_txt, w2, sizeof(guild_txt) - 1);
		} else if (strcasecmp(w1, "pet_txt") == 0) {
			memset(pet_txt, 0, sizeof(pet_txt));
			strncpy(pet_txt, w2, sizeof(pet_txt) - 1);
		} else if (strcasecmp(w1, "castle_txt") == 0) {
			memset(castle_txt, 0, sizeof(castle_txt));
			strncpy(castle_txt, w2, sizeof(castle_txt) - 1);
		} else if (strcasecmp(w1, "accreg_txt") == 0) {
			memset(accreg_txt, 0, sizeof(accreg_txt));
			strncpy(accreg_txt, w2, sizeof(accreg_txt) - 1);
		} else if (strcasecmp(w1, "guild_storage_txt") == 0) {
			memset(guild_storage_txt, 0, sizeof(guild_storage_txt));
			strncpy(guild_storage_txt, w2, sizeof(guild_storage_txt) - 1);
		} else if (strcasecmp(w1, "inter_log_filename") == 0) {
			memset(inter_log_filename, 0, sizeof(inter_log_filename));
			strncpy(inter_log_filename, w2, sizeof(inter_log_filename) - 1);
#endif /* TXT_ONLY */

#ifdef USE_SQL
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
#endif /* USE_SQL */

		} else if (strcasecmp(w1, "log_inter") == 0) {
			log_inter = config_switch(w2);
// import
		} else if (strcasecmp(w1, "import") == 0)
		{
			printf(CL_WHITE "Status: " CL_RESET "importing inter-server configuration file \n");
			inter_config_read(w2);
		}
	}
	fclose(fp);

	printf(CL_WHITE "Status: " CL_RESET "inter-server configuration file read \n");

	return;
}

void inter_log(char *fmt, ...)
{
#ifdef TXT_ONLY
	FILE *logfp;
#else
	char str[255], temp_str[511];
#endif
	va_list ap;

	if (!log_inter)
		return;

	va_start(ap, fmt);

#ifdef TXT_ONLY
	logfp = fopen(inter_log_filename, "a");
	if (logfp) {
		vfprintf(logfp, fmt, ap);
		fclose(logfp);
	}
#else /* TXT_ONLY -> USE_SQL*/
	vsprintf(str, fmt, ap);
	mysql_escape_string(temp_str, str, strlen(str));
	sql_request("INSERT INTO `%s` (`time`, `log`) VALUES (NOW(), '%s')", interlog_db, temp_str);
#endif /* USE_SQL */

	va_end(ap);

	return;
}

// セーブ
void inter_save() {
#ifdef TXT_ONLY
	inter_party_save();
	inter_guild_save();
	inter_storage_save();
	inter_guild_storage_save();
	inter_pet_save();
	inter_accreg_save();
#endif /* TXT_ONLY */

	return;
}

// initialize // 初期化
void inter_init(const char *file) {
#ifdef USE_SQL
	int i;
#endif // USE_SQL

	inter_config_read(file);

#ifdef USE_SQL
	// DB connection initialized
	sql_init();

	// Update DB if necessary
	// create `account_reg2_db` table if it not exists
	sql_request("SHOW TABLES");
	i = 0; // flag
	while (sql_get_row()) {
		if (strcmp(sql_get_string(0), "account_reg_db") == 0) {
			i = 1;
			break;
		}
	}
	// create `account_reg_db` table is not exist
	if (i == 0) {
		if (sql_request("CREATE TABLE IF NOT EXISTS `account_reg_db` ("
		                "  `account_id` int(11) NOT NULL default '0',"
		                "  `str` varchar(32) NOT NULL default '',"
		                "  `value` int(11) NOT NULL default '0',"
		                "  PRIMARY KEY (`account_id`, `str`)"
		                ") TYPE = MyISAM") == 0) {
			printf(CL_WHITE "Error: " CL_RESET "failed to create account register file. closing character-server \n");
			exit(1);
		}
		printf(CL_WHITE "Status: " CL_RESET "new account register file created \n");
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
#endif // USE_SQL

	wis_db = NULL;
	wis_db_num = 0;
	wisid = 0; // identify each wisp message

	inter_party_init();
	inter_guild_init();
	inter_storage_init();

	inter_pet_init();
#ifdef TXT_ONLY
	inter_accreg_init();
#endif // TXT_ONLY

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
TIMER_FUNC(check_ttl_wisdata) {
	int i;

	for(i = 0; i < wis_db_num; i++) {
		if (DIFF_TICK(gettick_cache, wis_db[i].tick) > WISDATA_TTL) {
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

// Wisp/page request to send
static inline void mapif_parse_WisRequest(int fd) { // 0x3001/0x3801 <packet_len>.w (<w_id_0x3801>.L) <sender_GM_level>.B <sender_name>.24B <nick_name>.24B <message>.?B
	char player[25]; // 24 + NULL
	int idx;
#ifdef USE_SQL
	int i;
	char t_player[49]; // (24 * 2) + NULL
#endif /* USE_SQL */

	if(RFIFOW(fd,2) <= 53 + 1)
	{
		printf(CL_YELLOW "Warning: " CL_RESET "failed to send wisp message \n");
		return;
	}

	strncpy(player, RFIFOP(fd,29), 24);
	player[24] = '\0';
#ifdef TXT_ONLY
	// if player is not found or not inline
	if ((idx = search_character_index(player)) < 0 || online_chars[idx] == 0) { // -1: no char with this name, -2: exact sensitive name not found // to check online here, check visible and hidden characters
		WPACKETW( 0) = 0x3802; // 0x3802 <sender_name>.24B <flag>.B (flag: 0: success to send wisper, 1: target character is not loged in?, 2: ignored by target)
		memcpy(WPACKETP(2), RFIFOP(fd,5), 24);
		WPACKETB(26) = 1; // flag: 0: success to send wisper, 1: target character is not loged in?, 2: ignored by target
		mapif_send(fd, 27);
		return;
	// Character exists. So, ask all map-servers
	} else {
		// to be sure of the correct name, rewrite it
		strncpy(RFIFOP(fd,29), char_dat[idx].name, 24);
#else /* TXT_ONLY -> USE_SQL*/
	// search player name in memory (-> -1, player is offline or doesn't exist; -2 player was found, but with incorrect sensitive case)
	if ((idx = search_character_index(player)) == -2) { // -1: no char with this name, -2: exact sensitive name not found
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
#endif /* USE_SQL */
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
#ifdef TXT_ONLY
	// .c file static local line[65536]
	int i;
	char *p_line;
#else /* TXT_ONLY -> USE_SQL*/
	char temp_str[65]; // 32 + NULL, 32 * 2 + NULL
#endif /* USE_SQL */

	account_id = RFIFOL(fd,4);
	//printf("mapif_parse_AccReg: account %d, num of account_reg: %d.\n", account_id, (RFIFOW(fd,2) - 8) / sizeof(struct global_reg));
	if (account_id <= 0)
		return;

#ifdef TXT_ONLY

	// if no reg to save
	if (RFIFOW(fd,2) == 8) {
		// search actual reg
		for(i = 0; i < accreg_db_num; i++) {
			// account found
			if (accreg_db[i].account_id == account_id) {
				FREE(accreg_db[i].reg);
				// only 1 account in database
				if (accreg_db_num < 2) {
					FREE(accreg_db);
					accreg_db_num = 0;
				// more than 1 account in database
				} else {
					if (i != accreg_db_num - 1)
						memcpy(&accreg_db[i], &accreg_db[accreg_db_num - 1], sizeof(struct accreg_db));
					REALLOC(accreg_db, struct accreg_db, accreg_db_num - 1);
					accreg_db_num--;
				}
				accreg_txt_dirty = 1; // to know if we must save account registers file
				break;
			}
		}
	} else {
		// create line
		p_line = line + int2string(line, account_id);
		*(p_line++) = '\t';
		for(p = 8; p < RFIFOW(fd,2); p += sizeof(struct global_reg)) {
			reg = (struct global_reg*)RFIFOP(fd,p);
			strcpy(p_line, reg->str);
			p_line += strlen(p_line);
			*(p_line++) = ',';
			p_line += int2string(p_line, reg->value);
			*(p_line++) = ' ';
		}
		*(p_line) = '\0';
		// search actual reg
		for(i = 0; i < accreg_db_num; i++)
			if (accreg_db[i].account_id == account_id) {
				// save new line
				REALLOC(accreg_db[i].reg, char, strlen(line) + 1); // + NULL
				strcpy(accreg_db[i].reg, line);
				break;
			}
		// if account not found
		if (i == accreg_db_num) {
			// save new line
			if (accreg_db_num == 0) {
				MALLOC(accreg_db, struct accreg_db, 1);
			} else {
				REALLOC(accreg_db, struct accreg_db, accreg_db_num + 1);
			}
			accreg_db[accreg_db_num].account_id = account_id;
			MALLOC(accreg_db[accreg_db_num].reg, char, strlen(line) + 1); // + NULL
			strcpy(accreg_db[accreg_db_num].reg, line);
			accreg_db_num++;
		}
		accreg_txt_dirty = 1; // to know if we must save account registers file
	}

#endif /* TXT_ONLY */

#ifdef USE_SQL
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
#endif /* USE_SQL */

	// send answer (accound_id saved) to remove flag
	WPACKETW(0) = 0x3805; // 0x3805 <account_id>.L
	WPACKETL(2) = account_id;
	SENDPACKET(fd, 6);

	return;
}

// Request the value of account_reg
void mapif_parse_AccRegRequest(int fd, int account_id) { // 0x3005 <account_id>.L
	int j, p;
#ifdef TXT_ONLY
	char buf[33];
	char *p_reg;
	int i, v, n;
#endif /* TXT_ONLY */

//	printf("mapif: accreg request\n");

#ifdef TXT_ONLY
	p = 8;
	j = 0;
	// search actual reg
	for(i = 0; i < accreg_db_num; i++) {
		// account found
		if (accreg_db[i].account_id == account_id) {
			p_reg = accreg_db[i].reg;
			if (sscanf(p_reg, "%d\t%n", &v, &n) == 1 && n > 0) {
				p_reg += n;
				while (sscanf(p_reg, "%[^,],%d %n", buf, &v, &n) == 2) {
					if (buf[0] != '\0' && v != 0 && j < ACCOUNT_REG_NUM) {
						strncpy(WPACKETP(p), buf, 32);
						WPACKETL(p + 32) = v;
						j++;
						p += 36;
					}
					p_reg += n;
				}
			}
		}
	}
#endif /* TXT_ONLY */
#ifdef USE_SQL
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
#endif /* USE_SQL */

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
	case 0x3008: log_atcommand(RFIFOB(fd,4), RFIFOP(fd,5)); break; // 0x3008 <packet_len>.w <log_type>.B <message>.?B (types: 1 GM commands, 2: Trades, 3: Scripts, 4: Vending)
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
