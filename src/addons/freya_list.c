// $Id: freya_list.c 572 2005-12-02 19:09:43Z Yor $
// Freya List Addon
//
// This addon will allow your server to be listed on the "Freya Servers List"
// which is on the website.

#include <config.h>

#ifdef __WIN32
#warning This module is not available for WIN32
#else /* WIN32 */

// Basic config is here for the moment - will be moved some where else one day :)
// SERVER NAME : limited to 64 chars
// SERVER OWNER : limited to 64 chars
// SERVER WEBSITE : limited to 128 chars
// SERVER LANGUAGE : limited to 2 chars
#define SERVER_NAME "Freya Noname"
#define SERVER_DESCRIPTION "Some unconfigured Freya server"
#define SERVER_OWNER "Anonymous"
#define SERVER_WEBSITE "http://www.ro-freya.net/"
#define SERVER_LANGUAGE "EN"

// refresh interval. Below 10 will get your IP banned.
// Higher than 300 will make you disappear from times to times from the list.
#define REFRESH_INTERVAL 180

// Server IP (you should not have to change that)
#define FREYA_YP_IP "69.64.32.27"
#define FREYA_YP_PORT 9958

/* ***** DO NOT EDIT BELOW ***** */

#define __ADDON

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>

#ifdef __WIN32
#define __USE_W32_SOCKETS
#include <windows.h>
#endif

#include <unistd.h>
#include <sys/types.h>
#ifndef __WIN32
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#include "../common/addons.h"
#include "../common/addons_exports.h"
#include "../login/addons.h"
// get version defines
#include "../common/core.h"
// we also use timers...
#include "../common/timer.h"
// needed for server list
#include "../login/login.h"

int freya_list_timer = -1;
int freya_list_socket = -1;

struct sockaddr_in localAddr;
struct sockaddr_in serverAddr;

void *info_packet;
int info_packet_size;

int freya_list_register(int tid, unsigned int tick, int id, int data);

// add 'inet_aton' function, not a C99 function
#ifdef WITH_C99
#include <ctype.h> // isdigit...
#ifndef isascii
#define isascii(c) (!(((int)(c)) & ~0177))
#endif // isascii

#ifndef HAVE_INET_ATON
/* Check whether "cp" is a valid ascii representation
   of an Internet address and convert to a binary address.
   Returns 1 if the address is valid, 0 if not.
   This replaces inet_addr, the return value from which
   cannot distinguish between failure and a local broadcast address. */
int inet_aton(const char *cp, struct in_addr *addr) {
	register u_int32_t val;
	register int base;
	register char c;
	unsigned int parts[4];
	register unsigned int *pp = parts;

	c = *cp;
	for (;;) {
		/* Collect number up to ``.''.
		   Values are specified as for C:
		   0x=hex, 0=octal, isdigit=decimal. */
		if (!isdigit(c))
			return (0);
		val = 0;
		base = 10;
		if (c == '0') {
			c = *++cp;
			if (c == 'x' || c == 'X') {
				base = 16;
				c = *++cp;
			} else
				base = 8;
		}
		for (;;) {
			if (isascii(c) && isdigit(c)) {
				val = (val * base) + (c - '0');
				c = *++cp;
			} else if (base == 16 && isascii(c) && isxdigit(c)) {
				val = (val << 4) | (c + 10 - (islower(c) ? 'a' : 'A'));
				c = *++cp;
			} else
				break;
		}
		if (c == '.') {
			/* Internet format:
			   a.b.c.d
			   a.b.c   (with c treated as 16 bits)
			   a.b     (with b treated as 24 bits) */
			if (pp >= parts + 3)
				return (0);
			*pp++ = val;
			c = *++cp;
		} else
			break;
	}

	// Check for trailing characters.
	if (c != '\0' && (!isascii(c) || !isspace(c)))
		return (0);
	// Concoct the address according to the number of parts specified.
	switch (pp - parts + 1) {
	case 0:
		return (0); // initial nondigit
	case 1:        // a -- 32 bits
		break;
	case 2:        // a.b -- 8.24 bits
		if ((val > 0xffffff) || (parts[0] > 0xff))
			return (0);
		val |= parts[0] << 24;
		break;
	case 3:        // a.b.c -- 8.8.16 bits
		if ((val > 0xffff) || (parts[0] > 0xff) || (parts[1] > 0xff))
			return (0);
		val |= (parts[0] << 24) | (parts[1] << 16);
		break;
	case 4:        // a.b.c.d -- 8.8.8.8 bits
		if ((val > 0xff) || (parts[0] > 0xff) || (parts[1] > 0xff) || (parts[2] > 0xff))
			return (0);
		val |= (parts[0] << 24) | (parts[1] << 16) | (parts[2] << 8);
		break;
	}

	if (addr)
		addr->s_addr = htonl(val);

	return (1);
}
#endif // HAVE_INET_ATON
#endif // WITH_C99

DLLFUNC ModuleHeader MOD_HEADER(freya_list) = {
	"FreyaList",
	"$Id: freya_list.c 572 2005-12-02 19:09:43Z Yor $",
	"This addon will allow your server to be listed on the \"Freya Servers Yellow Pages\" which is on the website.",
	MOD_VERSION,
	ADDONS_LOGIN
};

DLLFUNC int MOD_TEST(freya_list)() {
	// Nothing to do here...
	return MOD_SUCCESS;
}

DLLFUNC int MOD_INIT(freya_list)(void *ct) {
	call_table = ct;
	/* load local symbols table */
	MFNC_LOCAL_TABLE(local_table);
	freya_list_timer = -1;

	return MOD_SUCCESS;
}

DLLFUNC int MOD_LOAD(freya_list)() {
	// add a timer to register the server on the website :3
	int (*add_timer_interval)(unsigned int, int (*)(int,unsigned int,int,int), int, int, int);
	int (*delete_timer)(int, int (*)(int,unsigned int, int, int));
	void *gettick_cache_p;
	unsigned int gettick_cache_;
	MFNC_ADD_TIMER_INTERVAL(add_timer_interval);
	MFNC_DELETE_TIMER(delete_timer);
	LFNC_GETTICK_CACHE(gettick_cache_p);
	if (freya_list_timer >= 0) {
		(*delete_timer)(freya_list_timer, freya_list_register);
		freya_list_timer = -1;
	}

	/* Build the "info" packet that will be sent regulary
	 * when optimizing, the compiler should replace the strlens...
	 *
	 * Packet :
	 * SERVER NAME<0>SERVER DESC<0>SERVER OWNER<0>SERVER WEBSITE<0>SERVER LANG<0><port(long)><charcount(byte)><totalusers(long)> */

	info_packet_size = strlen(SERVER_NAME)+strlen(SERVER_DESCRIPTION)+strlen(SERVER_OWNER)+strlen(SERVER_LANGUAGE)+strlen(SERVER_WEBSITE)+5+4+1+4;
	info_packet = malloc(info_packet_size);
	memset(info_packet, 0, info_packet_size);
	memcpy(info_packet, SERVER_NAME, strlen(SERVER_NAME));
	memcpy(info_packet+strlen(SERVER_NAME)+1,SERVER_DESCRIPTION,strlen(SERVER_DESCRIPTION));
	memcpy(info_packet+strlen(SERVER_NAME)+strlen(SERVER_DESCRIPTION)+2,SERVER_OWNER,strlen(SERVER_OWNER));
	memcpy(info_packet+strlen(SERVER_NAME)+strlen(SERVER_DESCRIPTION)+strlen(SERVER_OWNER)+3,SERVER_WEBSITE,strlen(SERVER_WEBSITE));
	memcpy(info_packet+strlen(SERVER_NAME)+strlen(SERVER_DESCRIPTION)+strlen(SERVER_OWNER)+strlen(SERVER_WEBSITE)+4,SERVER_LANGUAGE,strlen(SERVER_LANGUAGE));

	/* try to open an UDP socket */
	if (freya_list_socket < 0) {
		freya_list_socket = socket(AF_INET, SOCK_DGRAM, 0);
		if (freya_list_socket < 0) {
			freya_list_socket = -1;
			puts("Could not open UDP socket !");
			return MOD_FAILED;
		}
		memset(&localAddr, 0, sizeof(localAddr));
		localAddr.sin_family       = AF_INET;
		localAddr.sin_port         = htons( 0 ); /* 0 = any port, we don't care... */
		localAddr.sin_addr.s_addr  = htonl( INADDR_ANY );
		if (bind(freya_list_socket, ( struct sockaddr * ) &localAddr, sizeof( struct sockaddr_in )) < 0) {
			close(freya_list_socket);
			freya_list_socket = -1;
			puts("Could not bind UDP socket !");
			return MOD_FAILED;
		}
		memset(&serverAddr, 0, sizeof(serverAddr));
		serverAddr.sin_family      = AF_INET;
		serverAddr.sin_port        = htons( FREYA_YP_PORT );
		inet_aton(FREYA_YP_IP, &serverAddr.sin_addr);
	}

	memcpy(&gettick_cache_, gettick_cache_p, sizeof(unsigned int));
	freya_list_timer = (*add_timer_interval) (gettick_cache_ + 1000, freya_list_register, 0, 0, REFRESH_INTERVAL * 1000);

	return MOD_SUCCESS;
}

DLLFUNC int MOD_UNLOAD(freya_list)() {
	int (*delete_timer)(int, int (*)(int,unsigned int, int, int));
	MFNC_DELETE_TIMER(delete_timer);

	// remove timer
	(*delete_timer)(freya_list_timer, freya_list_register);

	// close socket
	if (freya_list_socket >= 0) {
		close(freya_list_socket);
		freya_list_socket = -1;
	}

	return MOD_SUCCESS;
}

int freya_list_register(int tid, unsigned int tick, int id, int data) {
	// register
	int i;
	int j;
	struct mmo_char_server tmp_server;
	void *server_fd;
	void *server;
	int login_server_port;
	void *login_port_p;
	unsigned char char_count = 0;
	int player_count = 0;
#ifdef __DEBUG
	puts("Sending listing informations to Freya YellowPages...");
#endif
	if (freya_list_socket < 0) return 0;
	// get servers
	LFNC_SERVER(server);
	LFNC_SERVER_FD(server_fd);
	LFNC_LOGIN_PORT(login_port_p);
	memcpy(&login_server_port, login_port_p, sizeof(int));
	for(i = 0; i < MAX_SERVERS; i++) { // max number of char-servers (and account_id values: 0 to max-1)
		memcpy(&j, server_fd + (i * sizeof(int)), sizeof(int));
		if (j >= 0) {
			memcpy(&tmp_server, server + (i * sizeof(struct mmo_char_server)), sizeof(struct mmo_char_server));
//			printf("Test : %s\n", tmp_server.name);
			char_count++;
			player_count = player_count + tmp_server.users;
/* properties :
 * server[i].ip
 * server[i].port
 * server[i].name
 * server[i].users
 * server[i].maintenance
 * server[i].new */
		}
	}

	memcpy(info_packet + (info_packet_size - 9), &login_server_port, sizeof(int));
	memcpy(info_packet + (info_packet_size - 5), &char_count, sizeof(char));
	memcpy(info_packet + (info_packet_size - 4), &player_count, sizeof(int));
	i = sendto(freya_list_socket, info_packet, info_packet_size, 0, (struct sockaddr*) &serverAddr, sizeof(serverAddr));

	return 0;
}
#endif /* WIN32 */

