#include "common.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>

int send_data(channel_t* conn, int dir)
{
	char buf[65536];
	int  recv_fd = -1, send_fd = -1, len=0;
	ssize_t recv_len = 0, send_len = 0;

	if(dir == C2S_DIR)
	{
		send_fd = conn->srv.fd;
		recv_fd = conn->cli.fd;
	}
	else
	{
		send_fd = conn->cli.fd;
		recv_fd = conn->srv.fd;
	}

	recv_len = recv(recv_fd, buf, sizeof(buf), 0);
	if(recv_len < 0)
	{
		fprintf(stderr, "%s:%d recv() %s\n", __FUNCTION__, __LINE__, strerror(errno));
		return -1;
	}

	if(recv_len == 0)
	{
		fprintf(stderr, "communication between %d:%d is finished!\n", recv_fd, send_fd);
		close(recv_fd);
		close(send_fd);
		return -1;
	}

	send_len = 0;
	do {
		len = send(send_fd, buf, recv_len, 0);
		if(len < 0)
		{
			fprintf(stderr, "%s:%d send() %s\n", __FUNCTION__, __LINE__, strerror(errno));
			return -1;
		}
		send_len += len;
	} while(send_len < recv_len);

	/* printf("recv len %d, send len %d\n", (int)recv_len, (int)send_len); */
	return 0;
}

int client_send_data(channel_t* conn, int dir, char *magic)
{
	char buf[4096];
	int  recvoffset = 0;
	int  sendoffset = 0;
	int  recv_fd = -1, send_fd = -1 ;
	ssize_t recv_len;

	if(dir == C2S_DIR)
	{
		send_fd = conn->srv.fd;
		recv_fd = conn->cli.fd;
		sendoffset = 0;
		recvoffset = 20;
	}
	else
	{
		send_fd = conn->cli.fd;
		recv_fd = conn->srv.fd;
		sendoffset = 20;
		recvoffset = 0;
	}

	recv_len = recv(recv_fd, buf+recvoffset, sizeof(buf)-recvoffset, 0);
	if(recv_len < 0)
	{
		fprintf(stderr, "%s:%d recv() %s\n", __FUNCTION__, __LINE__, strerror(errno));
		return -1;
	}

	if(recv_len == 0)
	{
		return 0;
	}

	if(recvoffset) 
	{
		memcpy(buf, magic, recvoffset);
	}

	if(send(send_fd, buf+sendoffset, recv_len-sendoffset, 0) < 0)
	{
		fprintf(stderr, "%s:%d send() %s\n", __FUNCTION__, __LINE__, strerror(errno));
		return -1;
	}

	return 0;
}
