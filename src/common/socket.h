// Copyright (c) Freya Development Team - Licensed under GNU GPL
// For more information, see License.txt in the main folder

#ifndef _SOCKET_H_
#define _SOCKET_H_

#include <stdio.h>

#ifdef __WIN32
#define __USE_W32_SOCKETS
#include <windows.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#endif

#define RFIFOP(fd,pos) (session[fd]->rdata + session[fd]->rdata_pos + (pos))
#define RFIFOB(fd,pos) (*(unsigned char*)(session[fd]->rdata + session[fd]->rdata_pos + (pos)))
#define RFIFOW(fd,pos) (*(unsigned short*)(session[fd]->rdata + session[fd]->rdata_pos + (pos)))
#define RFIFOL(fd,pos) (*(unsigned int*)(session[fd]->rdata + session[fd]->rdata_pos + (pos)))
#define RFIFOSKIP(fd,len) (session[fd]->rdata_pos += (len))
#define RFIFOREST(fd) (session[fd]->rdata_size - session[fd]->rdata_pos)
#define RFIFOFLUSH(fd) (memmove(session[fd]->rdata, RFIFOP(fd,0), RFIFOREST(fd)), session[fd]->rdata_size = RFIFOREST(fd), session[fd]->rdata_pos = 0)
#define RFIFOSPACE(fd) (session[fd]->max_rdata - session[fd]->rdata_size)
#define RBUFP(p,pos) (((unsigned char*)(p)) + (pos))
#define RBUFB(p,pos) (*(unsigned char*)((p) + (pos)))
#define RBUFW(p,pos) (*(unsigned short*)((p) + (pos)))
#define RBUFL(p,pos) (*(unsigned int*)((p) + (pos)))

#define WFIFOSPACE(fd) (session[fd]->max_wdata - session[fd]->wdata_size)
char WPACKETBUF[65536];
#define WPACKETP(pos) (WPACKETBUF + (pos))
#define WPACKETB(pos) (*(unsigned char*)(WPACKETBUF + (pos)))
#define WPACKETW(pos) (*(unsigned short*)(WPACKETBUF + (pos)))
#define WPACKETL(pos) (*(unsigned int*)(WPACKETBUF + (pos)))
#define WBUFP(p,pos) (((unsigned char*)(p)) + (pos))
#define WBUFB(p,pos) (*(unsigned char*)((p) + (pos)))
#define WBUFW(p,pos) (*(unsigned short*)((p) + (pos)))
#define WBUFL(p,pos) (*(unsigned int*)((p) + (pos)))

#ifdef __INTERIX
#define FD_SETSIZE 4096
#endif // __INTERIX

struct socket_data{
	unsigned char eof;
	unsigned char not_used_big_rdata, not_used_big_wdata;
	char *rdata, *wdata;
	int max_rdata, max_wdata;
	int rdata_size, wdata_size;
	int rdata_pos;
	struct sockaddr_in client_addr;
	int (*func_recv)(int);
	int (*func_send)(int);
	int (*func_parse)(int);
	int (*func_console)(char*);
	void* session_data;
};

extern struct socket_data *session[FD_SETSIZE];

int fd_max;

extern char listen_ip[16]; // 15 + NULL

int make_listen_port(int);
int make_connection(long, int);
int delete_session(int);
int realloc_fifo(int fd, int rfifo_size, int wfifo_size);
void SENDPACKET(const int fd, const int len);

void flush_fifos(void);
int do_sendrecv(int next);
int do_parsepacket(void);
void do_socket(void);

int start_console(int (*parse_console_func)(char*));

void set_defaultparse(int (*defaultparse)(int));

int Net_Init(void);

unsigned int addr_[16];
unsigned int naddr_;

#endif // _SOCKET_H_
