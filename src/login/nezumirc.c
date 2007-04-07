/* nezumirc.c
 * $Id: nezumirc.c 610 2006-05-15 15:39:52Z MagicalTux $
 * Server-side for NezumiRC
 */

#include <config.h>

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>

#include <mmo.h> /* RETCODE */
#include "../common/socket.h"
#include "../common/version.h"
#include "login.h"
#include "lutils.h"
#include "nezumirc.h"

char *nezumirc_error_messages[] = {
/* 0x00 */	"An unknown error has occured.",
/* x001 */	"Your IP is not allowed to connect to this server",
/* 0x02 */	"Bad protocol version - please update server",
/* 0x03 */	"Remote admin is disabled on this server",
/* 0x04 */	"The provided password is incorrect"
};

char admin_pass[25]; /* from login.c */
static int parse_nezumirc(int fd); /* from this file, bottom */
static void nezumirc_send_serverinfo(int fd, int id); /* from here */

void nezumirc_send_error(int fd, int errorno) {
	WPACKETW(0) = 0x80ff;
	WPACKETL(2) = errorno;
	WPACKETW(6) = strlen(nezumirc_error_messages[errorno]);
	memcpy(WPACKETP(8), nezumirc_error_messages[errorno], strlen(nezumirc_error_messages[errorno]));
	SENDPACKET(fd, strlen(nezumirc_error_messages[errorno]) + 8);
}

void nezumirc_do_handshake(int fd) {
	WPACKETW(0) = 0x8001;
	WPACKETL(2) = NEZUMIRC_PROTOCOL;
	SENDPACKET(fd, 6);
}

void nezumirc_broadcast_new_char(int id) {
	return;
	// We got a new char, broadcast the info to all NezumiRC connected clients
	for(int i = 0; i < fd_max; i++) { // max number of char-servers (and account_id values: 0 to max-1)
		if (session[i] == NULL) continue;
		if (session[i]->func_parse != parse_nezumirc) continue;
		nezumirc_send_serverinfo(i, id);
	}
}

/*----------------------------------------
  nezumirc function:
  0x7535 - Request of the server version
----------------------------------------*/
static void nezumirc_packet_version(const int fd, const char* ip) {
	WPACKETW(0) = 0x7536;
	WPACKETB(2) = ATHENA_MAJOR_VERSION;
	WPACKETB(3) = ATHENA_MINOR_VERSION;
	WPACKETB(4) = ATHENA_REVISION;
	WPACKETB(5) = ATHENA_RELEASE_FLAG;
	WPACKETB(6) = ATHENA_OFFICIAL_FLAG;
	WPACKETB(7) = ATHENA_SERVER_LOGIN;
	WPACKETW(8) = ATHENA_MOD_VERSION;
#ifdef SVN_REVISION
	if (SVN_REVISION >= 1) // in case of .svn directories have been deleted
		WPACKETW(10) = SVN_REVISION;
	else
		WPACKETW(10) = 0;
#else
	WPACKETW(10) = 0;
#endif /* SVN_REVISION */
#ifdef TXT_ONLY
	WPACKETB(12) = 0;
#else
	WPACKETB(12) = 1;
#endif /* TXT_ONLY */
	SENDPACKET(fd, 13);

	return;
}

static void nezumirc_send_8009(const int fd, const int pkid, const char *str) {
	WPACKETW(0) = 0x8009;
	WPACKETB(2) = pkid;
	WPACKETW(3) = strlen(str);
	memcpy(WPACKETP(5), str, strlen(str));
	SENDPACKET(fd, strlen(str)+5);
}

static void nezumirc_packet_compileinfo(const int fd, const char* ip) {
	nezumirc_send_8009(fd, 0, CONF_PLATFORM);
	nezumirc_send_8009(fd, 1, CONF_PREFIX);
	nezumirc_send_8009(fd, 2, CONF_DATE);
	nezumirc_send_8009(fd, 3, CONF_CONFIGURE);
	nezumirc_send_8009(fd, 4, CONF_INCLUDES);
	nezumirc_send_8009(fd, 5, CONF_LIBS);
}

static void nezumirc_send_serverinfo(int fd, int id) {
	WPACKETW(0) = 0x8007;
	WPACKETL(2) = id;
	if (server_fd[id] < 0) {
		WPACKETL(12) = 0; // first 4 bytes of name set to 0. We don't care for the rest
		SENDPACKET(fd, 38);
		return;
	}
	WPACKETL(6) = server[id].ip;
	WPACKETW(10) = server[id].port;
	strncpy(WPACKETP(12), server[id].name, 20);
	WPACKETW(32) = server[id].users;
	WPACKETW(34) = server[id].maintenance;
	WPACKETW(36) = server[id].new;
	SENDPACKET(fd, 38);
}

void nezumirc_packet_srvinfo(int fd, const char *ip) {
	if (RFIFOW(fd, 0)==0x8006) {
		int id=RFIFOL(fd, 2);
		nezumirc_send_serverinfo(fd, id);
		return;
	}
	for(int i = 0; i < MAX_SERVERS; i++) { // max number of char-servers (and account_id values: 0 to max-1)
		if (server_fd[i] >= 0) nezumirc_send_serverinfo(fd, i);
	}
}

int parse_nezumirc_1(int fd) {
	unsigned char *p = (unsigned char *) &session[fd]->client_addr.sin_addr;
	char ip[16];

	sprintf(ip, "%d.%d.%d.%d", p[0], p[1], p[2], p[3]);

	if (session[fd]->eof) {
#ifdef __WIN32
		shutdown(fd, SD_BOTH);
		closesocket(fd);
#else
		close(fd);
#endif
		delete_session(fd);
		printf(CL_WHITE "Status: " CL_RESET "remote control client has disconnected '%d' \n", fd);
		write_log("'NezumiRC': Disconnection (session #%d, ip: %s)." RETCODE, fd, ip);
		return 0;
	}

	while(RFIFOREST(fd) >= 2 && !session[fd]->eof) {
		unsigned char passlen;
		char *password;
		switch(RFIFOW(fd, 0)) {
			case 0x8002:
				/* pwd ident */
				if (RFIFOREST(fd) < 3) return 0;
				passlen = RFIFOB(fd, 2);
				if (RFIFOREST(fd) < 3+passlen) return 0;
				password = RFIFOP(fd, 3);
				if (passlen > 24) passlen = 24;
				if (strncmp(admin_pass, password, passlen) != 0) {
					nezumirc_send_error(fd, NEZUMIRC_ERROR_BADPASSWORD);
					session[fd]->eof=1;
					return 0;
				}
				WPACKETW(0) = 0x8003;
				SENDPACKET(fd, 2);
				session[fd]->func_parse = parse_nezumirc;
				RFIFOSKIP(fd, 3+passlen);
				break;
			default:
				session[fd]->eof=1;
				return 0;
		}
	}

	return 0;
}

/*--------------
  Packet table



---------------*/
static struct nezumirc_func_table {
	int packet;
	signed short length; /* signed, because -1 */
	void (*proc)(const int fd, const char * ip);
} nezumirc_func_table[] = {
	{ 0x7535,   2, nezumirc_packet_version },
	{ 0x8004,   2, nezumirc_packet_srvinfo },
	{ 0x8006,   6, nezumirc_packet_srvinfo },
	{ 0x8008,   2, nezumirc_packet_compileinfo },
//	{ 0x7942,  -1, ladmin_change_memo },
	/* add functions before this line */
	{      0,   0, NULL },
};

/*----------------------------------------
  Packet parsing for administation login
----------------------------------------*/
static int parse_nezumirc(int fd) {
	int i, packet_len;
	unsigned char *p = (unsigned char *) &session[fd]->client_addr.sin_addr;
	char ip[16];

	sprintf(ip, "%d.%d.%d.%d", p[0], p[1], p[2], p[3]);

	if (session[fd]->eof) {
#ifdef __WIN32
		shutdown(fd, SD_BOTH);
		closesocket(fd);
#else
		close(fd);
#endif
		delete_session(fd);
		printf(CL_WHITE "Status: " CL_RESET "remote control client has disconnected '%d' \n", fd);
		write_log("'ladmin': Disconnection (session #%d, ip: %s)." RETCODE, fd, ip);
		return 0;
	}

	while(RFIFOREST(fd) >= 2 && !session[fd]->eof) {

		i = 0;
		while(nezumirc_func_table[i].packet != 0) {
			if (nezumirc_func_table[i].packet == RFIFOW(fd,0)) {
				packet_len = nezumirc_func_table[i].length;
				if (packet_len == -1) {
					if (RFIFOREST(fd) < 4)
						return 0;
					packet_len = RFIFOW(fd,2);
					if (packet_len < 4) {
						session[fd]->eof = 1;
						return 0;
					}
				}
				if (RFIFOREST(fd) < packet_len)
					return 0;
				nezumirc_func_table[i].proc(fd, ip);
				RFIFOSKIP(fd,packet_len);
				if (session[fd]->eof)
					return 0;
				break;
			}
			i++;
		}

		/* if packet was not found */
		if (nezumirc_func_table[i].packet == 0) {
			session[fd]->eof = 1;
			return 0;
		}

	}

	return 0;
}
