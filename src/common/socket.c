// Copyright (c) Freya Development Team - Licensed under GNU GPL
// For more information, see License.txt in the main folder

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#ifdef __WIN32
#define __USE_W32_SOCKETS
#include <windows.h>
#else
#ifdef WITH_C99
#include <sys/utsname.h>
#include <netdb.h>
#endif
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <net/if.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#ifndef SIOCGIFCONF
#include <sys/sockio.h>
#endif
#endif

#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>

#include "mmo.h"
#include "socket.h"
#include "malloc.h"
#include "console.h"

#ifdef MEMWATCH
#include "memwatch.h"
#endif

fd_set readfds; //, sendpacketfd;

#define RFIFO_SIZE (2*1024)

#define WFIFO_SIZE (4*1024)

char listen_ip[16] = "0.0.0.0"; // by default: any adresses are binded // 15 + NULL

struct socket_data *session[FD_SETSIZE];

static int null_parse(int fd);
static int (*default_func_parse)(int) = null_parse;

/*======================================
 *	CORE : Set function
 *--------------------------------------
 */
void set_defaultparse(int (*defaultparse)(int)) {
	default_func_parse = defaultparse;

	return;
}

/*======================================
 *	CORE : Socket Sub Function
 *--------------------------------------
 */

static int recv_to_fifo(int fd) {
	struct socket_data *s = session[fd];
	int len;

	if (s->eof)
		return -1;
	if (s->max_rdata == s->rdata_size)
		return 0;

#ifdef __WIN32
	len = recv(fd, s->rdata + s->rdata_size, s->max_rdata - s->rdata_size, 0);
	if (len == SOCKET_ERROR) {
		if (WSAGetLastError() != WSAEWOULDBLOCK) {
			s->eof = 1;
		}
		return 0;
	} else if (len <= 0) { // If the connection has been gracefully closed, the return value is zero.
#else
	len = read(fd, s->rdata + s->rdata_size, s->max_rdata - s->rdata_size);
	if (len <= 0) {
#endif

		s->eof = 1;
		return 0;
	} else {

		s->rdata_size += len;
		if (s->max_rdata > RFIFO_SIZE) {
			if (s->max_rdata == s->rdata_size) {
				realloc_fifo(fd, s->max_rdata << 1, s->max_wdata);
				s->not_used_big_rdata = 0;
			// if big buffer is not more used
			} else if (s->max_rdata > RFIFOSIZE_SERVER && s->rdata_size < s->max_rdata / 4) {
				if (s->not_used_big_rdata++ >= 16) { // need 16 times to verify
					s->max_rdata = s->max_rdata >> 1;
					REALLOC(s->rdata, char, s->max_rdata);
					/* not need to set at 0 new bytes */
					s->not_used_big_rdata = 0;
				}
			} else
				s->not_used_big_rdata = 0;
		}
	}

	return 0;
}

// ----------------------------------
static int send_from_fifo(int fd) {

	struct socket_data *s = session[fd];
	int len, resized_value, step_size;
#ifndef __WIN32
	int counter;
#endif

	// if client FIFO
	if (s->max_rdata == RFIFO_SIZE)
		step_size = WFIFO_SIZE;
	else
		step_size = WFIFOSIZE_SERVER;
	if (s->max_wdata > step_size) {
		// if big buffer is not more used
		if (s->wdata_size < s->max_wdata / 4) {
			if (s->not_used_big_wdata++ >= 32) { // need 32 times to verify
				resized_value = step_size;
				while (resized_value < s->wdata_size)
					resized_value += step_size;
				REALLOC(s->wdata, char, resized_value);
				s->max_wdata = resized_value;
				/* not need to set at 0 new bytes */
				s->not_used_big_wdata = 0;
			}
		} else
			s->not_used_big_wdata = 0;
	}

#ifdef __WIN32
		len = send(fd, s->wdata, s->wdata_size, 0);
		if (len == SOCKET_ERROR) {
			if (WSAGetLastError() != WSAEWOULDBLOCK) {
#else
	counter = 0;
	do {
		counter++;
		errno = 0;
		len = write(fd, s->wdata, s->wdata_size);
		if (len < 0) {
			if (errno != EAGAIN) {
#endif

				s->eof = 1;
			}
			return 0;
		} else if (len > 0) {
			if (len < s->wdata_size) {
				memmove(s->wdata, s->wdata + len, s->wdata_size - len);
				s->wdata_size -= len;
			} else
				s->wdata_size = 0;
		}
#ifndef __WIN32
	} while (len > 0 && s->wdata_size > 0);
#endif

	return 0;
}

// ----------------------------------
// same function of do_sendrecv(), but only send information to close all correctly (timeout = 0)
// it's used to close server (or to send important message before a possible lag)
void flush_fifos() {
	fd_set wfd;
#ifdef __WIN32
	fd_set err_fd; // exceptfds: * If processing a connect call (nonblocking), connection attempt failed. (http://msdn.microsoft.com/library/default.asp?url=/library/en-us/winsock/winsock/closesocket_2.asp)
#endif
	struct timeval timeout;
	int ret, i;

	FD_ZERO(&wfd);
	for(i = 1; i < fd_max; i++) // without keyboard (fd = 0)
		if (session[i] && session[i]->wdata_size)
			FD_SET(i, &wfd);
	timeout.tv_sec  = 0;
	timeout.tv_usec = 0;
#ifdef __WIN32
	memcpy(&err_fd, &wfd, sizeof(err_fd)); // exceptfds: * If processing a connect call (nonblocking), connection attempt failed. (http://msdn.microsoft.com/library/default.asp?url=/library/en-us/winsock/winsock/closesocket_2.asp)
	ret = select(fd_max, NULL, &wfd, &err_fd, &timeout);
#else
	ret = select(fd_max, NULL, &wfd, NULL, &timeout);
#endif

	// if error, remove invalid connections
	if (ret < 0) {
		// an error give invalid values in fd_set structures -> init them again
		FD_ZERO(&wfd);
#ifdef __WIN32
		FD_ZERO(&err_fd); // exceptfds: * If processing a connect call (nonblocking), connection attempt failed. (http://msdn.microsoft.com/library/default.asp?url=/library/en-us/winsock/winsock/closesocket_2.asp)
#endif
		for(i = 1; i < fd_max; i++) { // without keyboard (fd = 0)
			if (!session[i])
				continue;
			if (session[i]->wdata_size) {
				FD_SET(i, &wfd);
#ifdef __WIN32
				FD_SET(i, &err_fd);
#endif
			}
			timeout.tv_sec = 0;
			timeout.tv_usec = 0;
#ifdef __WIN32
			if (select(i + 1, NULL, &wfd, &err_fd, &timeout) >= 0 && !FD_ISSET(i, &err_fd)) {
#else
			if (select(i + 1, NULL, &wfd, NULL, &timeout) >= 0) {
#endif
				if (FD_ISSET(i, &wfd)) {
					if (session[i]->func_send)
						session[i]->func_send(i);
					FD_CLR(i, &wfd);
				}
#ifdef __WIN32
				FD_CLR(i, &err_fd);
#endif
			} else {
				session[i]->eof = 1; // set eof
				// an error gives invalid values in fd_set structures -> init them again
				FD_ZERO(&wfd);
#ifdef __WIN32
				FD_ZERO(&err_fd);
#endif
			}
		}

	// if no error (and a socket ready)
	} else if (ret > 0) {
		for(i = 1; i < fd_max; i++) { // without keyboard (fd = 0)
			if (!session[i])
				continue;
#ifdef __WIN32
			if (FD_ISSET(i, &err_fd)) { // exceptfds: * If processing a connect call (nonblocking), connection attempt failed. (http://msdn.microsoft.com/library/default.asp?url=/library/en-us/winsock/winsock/closesocket_2.asp)
				session[i]->eof = 1; // set eof
				continue;
			}
#endif
			if (FD_ISSET(i, &wfd)) {
				//printf("write:%d\n", i);
				if (session[i]->func_send)
					session[i]->func_send(i);
			}
		}
	}

	return;
}

// ----------------------------------
static int null_parse(int fd) {
	printf("null_parse: %d\n", fd);
	session[fd]->rdata_pos = session[fd]->rdata_size; //RFIFOSKIP(fd, RFIFOREST(fd)); simplify calculation

	return 0;
}

/*======================================
 *	CORE : Socket Function
 *--------------------------------------
 */

static int connect_client(int listen_fd) {
	int fd;
	struct sockaddr_in client_address;
#ifdef __WIN32
	int len;
#else
	socklen_t len;
#endif
	int yes;

	len = sizeof(client_address);

	fd = accept(listen_fd, (struct sockaddr*)&client_address, &len);
#ifdef __WIN32
	if (fd == SOCKET_ERROR || fd == INVALID_SOCKET) {
#else
	if (fd == -1) {
#endif
		return -1;
	}

	yes = 1;
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *)&yes, sizeof yes) != 0) {
#ifdef __WIN32
		closesocket(fd); // not started, not use shutdown(fd, SD_BOTH);
#else
		close(fd);
#endif
		return -1;
	}
#ifdef SO_REUSEPORT
	yes = 1;
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, (char *)&yes, sizeof yes) != 0) {
#ifdef __WIN32
		closesocket(fd); // not started, not use shutdown(fd, SD_BOTH);
#else
		close(fd);
#endif
		return -1;
	}
#endif
	yes = 1;
	if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (char *)&yes, sizeof yes) != 0) {
#ifdef __WIN32
		closesocket(fd); // not started, not use shutdown(fd, SD_BOTH);
#else
		close(fd);
#endif
		return -1;
	}

#ifdef __WIN32
  {
	//-------------------------
	// Set the socket I/O mode: In this case FIONBIO
	// enables or disables the blocking mode for the
	// socket based on the numerical value of iMode.
	// If iMode = 0, blocking is enabled;
	// If iMode != 0, non-blocking mode is enabled.
	unsigned long iMode = 1;
	if (ioctlsocket(fd, FIONBIO, &iMode) != 0) {
		closesocket(fd); // not started, not use shutdown(fd, SD_BOTH);
		return -1;
	}
  }
#else
	if (fcntl(fd, F_SETFL, O_NONBLOCK) == -1) { // error
		close(fd);
		return -1;
	}
#endif

#ifdef __WIN32
  {
	struct linger opt;
	opt.l_onoff = 1; // If SO_LINGER is enabled with a zero time-out: it always returns immediately —connection is reset/terminated.
	opt.l_linger = 0;
	if (setsockopt(fd, SOL_SOCKET, SO_LINGER, (char*)&opt, sizeof(opt)) != 0) {
		closesocket(fd); // not started, not use shutdown(fd, SD_BOTH);
		return -1;
	}
  }
#endif

	// check array
	if (fd < 1 || fd >= FD_SETSIZE) { // it's possible with windows! (don't check fd = 0, keyboard)
#ifdef __WIN32
		closesocket(fd); // not started, not use shutdown(fd, SD_BOTH);
#else
		close(fd);
#endif
		return -1;
	}

	if (fd_max <= fd)
		fd_max = fd + 1;

	FD_SET(fd, &readfds);

	CALLOC(session[fd], struct socket_data, 1);
	MALLOC(session[fd]->rdata, char, RFIFO_SIZE);
	MALLOC(session[fd]->wdata, char, WFIFO_SIZE);

	session[fd]->max_rdata   = RFIFO_SIZE;
	session[fd]->max_wdata   = WFIFO_SIZE;
	session[fd]->func_recv   = recv_to_fifo;
	session[fd]->func_send   = send_from_fifo;
	session[fd]->func_parse  = default_func_parse;
	session[fd]->client_addr = client_address;

	return fd;
}

// ----------------------------------
int make_listen_port(int port) {
	struct sockaddr_in server_address;
	int fd;
	int yes;

	if (inet_addr(listen_ip) == INADDR_NONE) { // not always -1
		perror("Invalid listen IP (socket.c: make_listen_port).");
		exit(1);
	}

	fd = socket(AF_INET, SOCK_STREAM, 0); // under winsock: SOCKET type is unsigned
#ifdef __WIN32
	if (fd == INVALID_SOCKET) {
#else
	if (fd == -1) {
#endif
		perror("socket error: Unable to open socket (socket.c: make_listen_port).");
		exit(1);
	}

	yes = 1;
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *)&yes, sizeof yes) != 0) {
		perror("setsockopt (SO_REUSEADDR) error (socket.c: make_listen_port).");
		exit(1);
	}
#ifdef SO_REUSEPORT
	yes = 1;
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, (char *)&yes, sizeof yes) != 0) {
		perror("setsockopt (SO_REUSEPORT) error (socket.c: make_listen_port).");
		exit(1);
	}
#endif
	yes = 1;
	if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (char *)&yes, sizeof yes) != 0) {
		perror("setsockopt (TCP_NODELAY) error (socket.c: make_listen_port).");
		exit(1);
	}

#ifdef __WIN32
  {
	//-------------------------
	// Set the socket I/O mode: In this case FIONBIO
	// enables or disables the blocking mode for the
	// socket based on the numerical value of iMode.
	// If iMode = 0, blocking is enabled;
	// If iMode != 0, non-blocking mode is enabled.
	unsigned long iMode = 1;
	if (ioctlsocket(fd, FIONBIO, &iMode) != 0) {
		perror("ioctlsocket error (socket.c: make_listen_port).");
		exit(1);
	}
  }
#else
	if (fcntl(fd, F_SETFL, O_NONBLOCK) == -1) { // error
		perror("fcntl error (socket.c: make_listen_port).");
		exit(1);
	}
#endif

#ifdef __WIN32
  {
	struct linger opt;
	opt.l_onoff = 1; // If SO_LINGER is enabled with a zero time-out: it always returns immediately —connection is reset/terminated.
	opt.l_linger = 0;
	if (setsockopt(fd, SOL_SOCKET, SO_LINGER, (char*)&opt, sizeof(opt)) != 0) {
		perror("setsockopt (SO_LINGER) error (socket.c: make_listen_port).");
		exit(1);
	}
  }
#endif

	server_address.sin_family        = AF_INET;
	server_address.sin_addr.s_addr   = inet_addr(listen_ip);
	server_address.sin_port          = htons(port);

	if (bind(fd, (struct sockaddr*)&server_address, sizeof(server_address)) != 0) { // error when not 0 (can be -1 or any other value)
		perror("bind error (socket.c: make_listen_port).");
		exit(1);
	}

	if (listen(fd, 5) != 0) { // error when not 0 (can be -1 or any other value)
		perror("listen error (socket.c: make_listen_port).");
		exit(1);
	}

	// check array
	if (fd < 1 || fd >= FD_SETSIZE) { // it's possible with windows! (don't check fd = 0, keyboard)
		perror("fd < 0 || fd >= FD_SETSIZE error (socket.c: make_listen_port).");
		exit(1);
	}

	if (fd_max <= fd)
		fd_max = fd + 1;

	FD_SET(fd, &readfds);

	CALLOC(session[fd], struct socket_data, 1);

	session[fd]->func_recv = connect_client;

	return fd;
}

/*======================================
 *	Console Reciever
 *--------------------------------------
 */
int console_recieve(int fd) {
	int n;
	int i, j;
	static char buf[4096];
	char *p;
#ifdef __WIN32
	static size_t used = 0;
	DWORD ret = 0;

	// read console (specifical to Windows)
	n = 0;
	if (term_input_status) {
		INPUT_RECORD ir_p;
		while (PeekConsoleInput(hStdin, &ir_p, 1, &ret) && ret) { // check if an event if present
			ReadConsoleInput(hStdin, &ir_p, 1, &ret);
			if (ir_p.EventType == KEY_EVENT && ir_p.Event.KeyEvent.bKeyDown) {
				DWORD tmp;
				int key = ir_p.Event.KeyEvent.uChar.AsciiChar;
				if (key == '\r') { // end of line
					buf[used] = '\0'; // don't copy \n
					WriteConsole(hStdout, "\r\n", 2, &tmp, NULL);
					n = used;
					used = 0;
					break;
				} else if (key == '\n') { // cariage return
					// write nothing
					buf[used] = '\0';
					n = used;
					used = 0;
					break;
				} else if (key == '\b') {
					if (used > 0) {
						used--;
						buf[used] = '\0';
						WriteConsole(hStdout, "\b \b", 3, &tmp, NULL);
					}
				} else if (key && used < sizeof(buf) - 1) {
					buf[used++] = key;
					buf[used] = '\0';
					WriteConsole(hStdout, buf + used - 1, 1, &tmp, NULL);
				}
			}
		}
	}
#else
	memset(buf, 0, sizeof(buf));
	n = read(0, buf, sizeof(buf) - 1);
#endif

	if (n < 0)
		printf("Console input read error.\n");
	else if (n > 0) {
		buf[sizeof(buf) - 1] = '\0';
		// remove final \n
		if ((p = strrchr(buf, '\n')) != NULL)
			p[0] = '\0';
		// remove all control char
		for (i = 0; buf[i]; i++)
			if (iscntrl((int)buf[i])) { // if (buf[i] > 0 && buf[i] < 32) {
				// remove cursor control.
				if (buf[i] == 27 && buf[i+1] == '[' &&
				    (buf[i+2] == 'H' || // home position (cursor)
				     buf[i+2] == 'J' || // clear screen
				     buf[i+2] == 'A' || // up 1 line
				     buf[i+2] == 'B' || // down 1 line
				     buf[i+2] == 'C' || // right 1 position
				     buf[i+2] == 'D' || // left 1 position
				     buf[i+2] == 'G')) { // center cursor (windows)
					for (j = i; buf[j]; j++)
						buf[j] = buf[j+3];
				} else if (buf[i] == 27 && buf[i+1] == '[' && buf[i+2] == '2' && buf[i+3] == 'J') { // clear screen
					for (j = i; buf[j]; j++)
						buf[j] = buf[j+4];
				} else if (buf[i] == 27 && buf[i+1] == '[' && buf[i+3] == '~' &&
				           (buf[i+2] == '1' || // home (windows)
				            buf[i+2] == '2' || // insert (windows)
				            buf[i+2] == '3' || // del (windows)
				            buf[i+2] == '4' || // end (windows)
				            buf[i+2] == '5' || // pgup (windows)
				            buf[i+2] == '6')) { // pgdown (windows)
					for (j = i; buf[j]; j++)
						buf[j] = buf[j+4];
				} else if (buf[i] == 8 && i > 0) { // backspace (ctrl+H) (if no previous char, next 'else' will remove backspace)
					for (j = i - 1; buf[j]; j++) // i - 1, to remove previous character
						buf[j] = buf[j+2];
						i--; // recheck previous value
				} else {
					/* remove other control char. */
					for (j = i; buf[j]; j++)
						buf[j] = buf[j+1];
				}
				i--;
			}
		session[0]->func_console(buf);
		session[0]->wdata_size = 0; // to be sure to send nothing
		memset(buf, 0, sizeof(buf));
	}

	return 0;
}

/*======================================
 *	Console Initialisation
 *--------------------------------------
 */
int start_console(int (*parse_console_func)(char*)) {
	term_input_enable();

	if (term_input_status) {
#ifndef __WIN32
		if (fcntl(0, F_SETFL, O_NONBLOCK) == -1) { // error
			perror("O_NONBLOCK error (start_console).");
		}

		FD_SET(0, &readfds);
#endif

		CALLOC(session[0], struct socket_data, 1);

		session[0]->func_recv = console_recieve;
		session[0]->func_console = parse_console_func;
	}

	return 0;
}

// ----------------------------------
int make_connection(long ip, int port) {
	struct sockaddr_in server_address;
	int fd;
	int yes;

	fd = socket(AF_INET, SOCK_STREAM, 0); // under winsock: SOCKET type is unsigned
#ifdef __WIN32
	if (fd == INVALID_SOCKET) {
#else
	if (fd == -1) {
#endif
		perror("socket error (socket.c: make_connection).");
		return -1;
	}

	yes = 1;
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *)&yes, sizeof yes) != 0) {
		perror("setsockopt (SO_REUSEADDR) error (socket.c: make_connection).");
#ifdef __WIN32
		closesocket(fd); // not started, not use shutdown(fd, SD_BOTH);
#else
		close(fd);
#endif
		return -1;
	}

#ifdef SO_REUSEPORT
	yes = 1;
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, (char *)&yes, sizeof yes) != 0) {
		perror("setsockopt (SO_REUSEPORT) error (socket.c: make_connection).");
#ifdef __WIN32
		closesocket(fd); // not started, not use shutdown(fd, SD_BOTH);
#else
		close(fd);
#endif
		return -1;
	}
#endif

	yes = 1;
	if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (char *)&yes, sizeof yes) != 0) {
		perror("setsockopt (TCP_NODELAY) error (socket.c: make_connection).");
#ifdef __WIN32
		closesocket(fd); // not started, not use shutdown(fd, SD_BOTH);
#else
		close(fd);
#endif
		return -1;
	}

#ifdef __WIN32
  {
	//-------------------------
	// Set the socket I/O mode: In this case FIONBIO
	// enables or disables the blocking mode for the
	// socket based on the numerical value of iMode.
	// If iMode = 0, blocking is enabled;
	// If iMode != 0, non-blocking mode is enabled.
	unsigned long iMode = 1;
	if (ioctlsocket(fd, FIONBIO, &iMode) != 0) {
		perror("ioctlsocket error (socket.c: make_connection).");
		closesocket(fd); // not started, not use shutdown(fd, SD_BOTH);
		return -1;
	}
  }
#else
	if (fcntl(fd, F_SETFL, O_NONBLOCK) == -1) { // error
		perror("fcntl error (socket.c: make_connection).");
		close(fd);
		return -1;
	}
#endif

#ifdef __WIN32
  {
	struct linger opt;
	opt.l_onoff = 1; // If SO_LINGER is enabled with a zero time-out: it always returns immediately —connection is reset/terminated.
	opt.l_linger = 0;
	if (setsockopt(fd, SOL_SOCKET, SO_LINGER, (char*)&opt, sizeof(opt)) != 0) {
		perror("setsockopt (SO_LINGER) error (socket.c: make_connection).");
		closesocket(fd); // not started, not use shutdown(fd, SD_BOTH);
		return -1;
	}
  }
#endif

	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = ip;
	server_address.sin_port = htons(port);

	if (connect(fd, (struct sockaddr *)(&server_address), sizeof(struct sockaddr_in)) != 0) {
#ifdef __WIN32
		if (WSAGetLastError() != WSAEWOULDBLOCK) {
			perror("connect error (socket.c: make_connection).");
			closesocket(fd); // not started, not use shutdown(fd, SD_BOTH);
			return -1;
		}
#else
		if (errno != EINPROGRESS) {
			perror("connect error (socket.c: make_connection).");
			close(fd);
			return -1;
		}
#endif
	}

	// check array
	if (fd < 1 || fd >= FD_SETSIZE) { // it's possible with windows! (don't check fd = 0, keyboard)
		perror("fd < 0 || fd >= FD_SETSIZE error (socket.c: make_connection).");
#ifdef __WIN32
		closesocket(fd); // not started, not use shutdown(fd, SD_BOTH);
#else
		close(fd);
#endif
		return -1;
	}

	if (fd_max <= fd)
		fd_max = fd + 1;

	FD_SET(fd, &readfds);

	CALLOC(session[fd], struct socket_data, 1);
	MALLOC(session[fd]->rdata, char, RFIFO_SIZE);
	MALLOC(session[fd]->wdata, char, WFIFO_SIZE);

	session[fd]->max_rdata  = RFIFO_SIZE;
	session[fd]->max_wdata  = WFIFO_SIZE;
	session[fd]->func_recv  = recv_to_fifo;
	session[fd]->func_send  = send_from_fifo;
	session[fd]->func_parse = default_func_parse;

	return fd;
}

// ----------------------------------
int delete_session(int fd) {
	if (fd < 0 || fd >= FD_SETSIZE)
		return -1;

#ifdef __WIN32
	if (fd > 0) // if not console
#endif
		FD_CLR(fd, &readfds);
	if (session[fd]) {
		FREE(session[fd]->rdata);
		FREE(session[fd]->wdata);
		FREE(session[fd]->session_data);
		FREE(session[fd]);
	}
	//printf("delete_session: %d.\n", fd);

	return 0;
}

// ----------------------------------
int realloc_fifo(int fd, int rfifo_size, int wfifo_size) {
	struct socket_data *s = session[fd];

	if (s->max_rdata != rfifo_size && s->rdata_size < rfifo_size) {
		REALLOC(s->rdata, char, rfifo_size);
		/* not need to set at 0 new bytes */
		s->max_rdata = rfifo_size;
	}
	if (s->max_wdata != wfifo_size && s->wdata_size < wfifo_size) {
		REALLOC(s->wdata, char, wfifo_size);
		/* not need to set at 0 new bytes */
		s->max_wdata = wfifo_size;
	}

	return 0;
}

// ----------------------------------
void SENDPACKET(const int fd, const int len) {
	struct socket_data *s;
	int resized_value;

	if (len < 2 || fd <= 0 || (s = session[fd]) == NULL) // don't write on console (fd = 0)
		return;

//	printf("Session %d, packet 0x%X (%d bytes) writen.\n", fd, *(unsigned short*)WFIFOBUF, len);

	// if buffer is too short
	if (s->wdata_size + len > s->max_wdata) {
		resized_value = s->max_wdata;
		// if client FIFO
		if (s->max_rdata == RFIFO_SIZE) {
			// if already more than 4 Mb, when disconnect player
			if (s->max_wdata > 4 * 1024 * 1024) { /* more than 4 Mb ??? */
				s->eof = 1;
				return;
			}
			while (resized_value < s->wdata_size + len)
				resized_value += WFIFO_SIZE;
		} else
			while (resized_value < s->wdata_size + len)
				resized_value += WFIFOSIZE_SERVER;
		REALLOC(s->wdata, char, resized_value);
		/* not need to set at 0 new bytes */
		s->max_wdata = resized_value;
		s->not_used_big_wdata = 0;
	}

	// copy packet
	memcpy(s->wdata + s->wdata_size, WPACKETBUF, len);
	s->wdata_size = s->wdata_size + len;

	return;
}

// ----------------------------------
int do_sendrecv(int next) {
	fd_set rfd, wfd;
#ifdef __WIN32
	fd_set err_fd; // exceptfds: * If processing a connect call (nonblocking), connection attempt failed.
#endif
	struct timeval timeout;
	int ret, i;


#ifdef __WIN32
	// read console (specifical to Windows)
	if (term_input_status) {
		session[0]->func_recv(0);
	}
#endif
	FD_ZERO(&wfd);
	for(i = 1; i < fd_max; i++) { // without keyboard (fd = 0)
		if (!session[i]) {
			if (FD_ISSET(i, &readfds)) {
				printf("force clr fds %d\n", i);
				FD_CLR(i, &readfds);
			}
			continue;
		}
		if (session[i]->wdata_size)
			FD_SET(i, &wfd);
	}
	timeout.tv_sec  = next / 1000;
	timeout.tv_usec = next % 1000 * 1000;
	memcpy(&rfd, &readfds, sizeof(rfd)); // copy read fd of console too ;)
//	rfd = readfds;
#ifdef __WIN32
	memcpy(&err_fd, &readfds, sizeof(err_fd)); // exceptfds: * If processing a connect call (nonblocking), connection attempt failed.
	ret = select(fd_max, &rfd, &wfd, &err_fd, &timeout);
#else
	ret = select(fd_max, &rfd, &wfd, NULL, &timeout);
#endif

	// if error, remove invalid connections
	if (ret < 0) {
		// an error give invalid values in fd_set structures -> init them again
		FD_ZERO(&rfd);
		FD_ZERO(&wfd);
#ifdef __WIN32
		FD_ZERO(&err_fd); // exceptfds: * If processing a connect call (nonblocking), connection attempt failed.
#endif
		for(i = 1; i < fd_max; i++) { // without keyboard (fd = 0)
			if (!session[i])
				continue;
			if (FD_ISSET(i, &readfds)) {
				FD_SET(i, &rfd);
#ifdef __WIN32
				FD_SET(i, &err_fd);
#endif
			}
			if (session[i]->wdata_size)
				FD_SET(i, &wfd);
			timeout.tv_sec = 0;
			timeout.tv_usec = 0;
#ifdef __WIN32
			if (select(i + 1, &rfd, &wfd, &err_fd, &timeout) >= 0 && !FD_ISSET(i, &err_fd)) {
#else
			if (select(i + 1, &rfd, &wfd, NULL, &timeout) >= 0) {
#endif
				if (FD_ISSET(i, &wfd)) {
					if (session[i]->func_send)
						session[i]->func_send(i);
					FD_CLR(i, &wfd);
				}
				if (FD_ISSET(i, &rfd)) {
					if (session[i]->func_recv)
						session[i]->func_recv(i);
					FD_CLR(i, &rfd);
				}
#ifdef __WIN32
				FD_CLR(i, &err_fd);
#endif
			} else {
				session[i]->eof = 1; // set eof
				// an error gives invalid values in fd_set structures -> init them again
				FD_ZERO(&rfd);
				FD_ZERO(&wfd);
#ifdef __WIN32
				FD_ZERO(&err_fd);
#endif
			}
		}

	// if no error (and a socket ready)
	} else if (ret > 0) {
		for(i = 0; i < fd_max; i++) {
			if (!session[i])
				continue;
#ifdef __WIN32
			if (FD_ISSET(i, &err_fd)) { // exceptfds: * If processing a connect call (nonblocking), connection attempt failed.
				session[i]->eof = 1; // set eof
				continue;
			}
#endif
			if (FD_ISSET(i, &wfd)) {
				//printf("write:%d\n", i);
				if (session[i]->func_send)
					session[i]->func_send(i);
			}
			if (FD_ISSET(i, &rfd)) {
				//printf("read:%d\n", i);
				if (session[i]->func_recv)
					session[i]->func_recv(i);
			}
		}
	}

	return 0;
}

// ----------------------------------
int do_parsepacket(void) {
	int i;

	for(i = 0; i < fd_max; i++) {
		if (!session[i])
			continue;
		if (session[i]->rdata_size == 0 && session[i]->eof == 0)
			continue;
		if (session[i]->func_parse) {
			session[i]->func_parse(i);
			if (!session[i])
				continue;
			/* after parse, check client's RFIFO size to know if there is an invalid packet (too big and not parsed) */
			if (session[i]->rdata_size == RFIFO_SIZE && session[i]->max_rdata == RFIFO_SIZE) {
				session[i]->eof = 1;
				continue;
			}
		}
		RFIFOFLUSH(i);
	}

	return 0;
}

// ----------------------------------
void do_socket(void) {
	FD_ZERO(&readfds);
//	FD_ZERO(&sendpacketfd);
}

// ----------------------------------
int Net_Init(void) {
#ifdef __WIN32
	char** a;
	unsigned int i;
	char fullhost[255];
	struct hostent* hent;

	/* Start up the windows networking */
	WSADATA wsaData;

	naddr_ = 0; /* # of ip addresses */

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		if (WSAStartup(MAKEWORD(1, 1), &wsaData) != 0) {
			printf("SYSERR: WinSock not available!\n");
			exit(1);
		} else
			printf("Winsock started using v1.1.\n");
	} else
		printf("Winsock started using v2.0.\n");

	if (gethostname(fullhost, sizeof(fullhost)) == SOCKET_ERROR) {
		printf("No hostname defined!\n");
		return 0;
	}

	// XXX This should look up the local IP addresses in the registry
	// instead of calling gethostbyname. However, the way IP addresses
	// are stored in the registry is annoyingly complex, so I'll leave
	// this as T.B.D.
	hent = gethostbyname(fullhost);
	if (hent == NULL) {
		printf("Cannot resolve our own hostname to an IP address.\n");
		return 0;
	}

	a = hent->h_addr_list;
	for(i = 0; a[i] != 0 && i < 16; i++) {
		unsigned long addr1 = ntohl(*(unsigned long*) a[i]);
		addr_[i] = addr1;
	}
	naddr_ = i;

#else // else __WIN32

#ifdef WITH_C99
	char** a;
	unsigned int i;
	struct utsname utsname;
	struct hostent* hent;

	naddr_ = 0; // # of ip addresses

	if (uname(&utsname) == -1) {
		printf("No hostname defined!\n");
		return 0;
	}

	hent = gethostbyname(utsname.nodename);
	if (hent == NULL) {
		printf("Cannot resolve our own hostname to an IP address.\n");
		return 0;
	}

	if (hent->h_addrtype == AF_INET) {
		a = hent->h_addr_list;
		for(i = 0; a[i] != 0 && i < 16; i++) {
			unsigned long addr1 = ntohl(*(unsigned long*) a[i]);
			addr_[i] = addr1;
		}
		naddr_ = i;
	}

#else // WITH_C99

	int pos;
	int fdes = socket(AF_INET, SOCK_STREAM, 0); // under winsock: SOCKET type is unsigned
	char buf[16 * sizeof(struct ifreq)];
	struct ifconf ic;

	naddr_ = 0; // # of ip addresses

	// The ioctl call will fail with Invalid Argument if there are more
	// interfaces than will fit in the buffer
	ic.ifc_len = sizeof(buf);
	ic.ifc_buf = buf;
	if (ioctl(fdes, SIOCGIFCONF, &ic) == -1) {
		printf("SIOCGIFCONF failed!\n");
		return 0;
	}

	pos = 0;
	while(pos < ic.ifc_len) {
		struct ifreq * ir = (struct ifreq *) (ic.ifc_buf + pos);
		struct sockaddr_in * a = (struct sockaddr_in *) &(ir->ifr_addr);

		if (a->sin_family == AF_INET) {
			u_long ad = ntohl(a->sin_addr.s_addr);
			if (ad != INADDR_LOOPBACK) {
				addr_[naddr_ ++] = ad;
			if (naddr_ == 16)
				break;
			}
		}

#if defined(_AIX) || defined(__APPLE__)
		pos += ir->ifr_addr.sa_len; /* For when we port Freya to run on Mac's :) */
		pos += sizeof(ir->ifr_name);
#else
		pos += sizeof(struct ifreq);
#endif
	}
#endif

#endif // WITH_C99

	// if no address, set 127.0.0.1 by default
	if (naddr_ == 0) {
		addr_[0] = (127 * 256 * 65536) + 0 + 0 + 1;
		naddr_ = 1;
	}

	return 0;
}

