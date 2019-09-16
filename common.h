#ifndef __H_COMMON__
#define __H_COMMON__

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/select.h>

#include <errno.h>

typedef struct _tuple_t{
	char 		ip[16];
	uint16_t	port;
	uint16_t 	rfu;
	int 		fd;
}tuple_t;

typedef struct _channel_t{
	tuple_t cli;
	tuple_t srv;
}channel_t;

#define C2S_DIR 0
#define S2C_DIR 1

int send_data(channel_t* conn, int dir);
int connect_to_target(char* ip, uint16_t port);

#endif
