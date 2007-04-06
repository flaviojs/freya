/* nezumirc.h
 * $Id: nezumirc.h 228 2006-02-08 21:04:48Z MagicalTux $
 * Server-side for NezumiRC
 */

#define NEZUMIRC_PROTOCOL 1

enum {
	NEZUMIRC_ERROR_UNKNOWN = 0x00,
	NEZUMIRC_ERROR_IPNOTALLOWED = 0x01,
	NEZUMIRC_ERROR_BADPRT = 0x02,
	NEZUMIRC_ERROR_NOADMIN = 0x03,
	NEZUMIRC_ERROR_BADPASSWORD = 0x04
};

void nezumirc_send_error(int, int);
void nezumirc_do_handshake(int);
int parse_nezumirc_1(int);
void nezumirc_broadcast_new_char(int);

