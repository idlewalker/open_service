#include "common.h"

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

int connect_to_target(char* ip, uint16_t port)
{
	int sockfd = 0;
	struct sockaddr_in server;

	memset(&server, 0, sizeof(server));

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd < 0)
	{
		fprintf(stderr, "get inside conn : socket() -> %s\n", strerror(errno));
		return -1;
	}

	server.sin_addr.s_addr = inet_addr(ip);
	server.sin_family = AF_INET;
	server.sin_port = htons(port);

	while(!connect(sockfd, (struct sockaddr*)&server, sizeof(server)))
	{
		sleep(5);
	}

	struct sockaddr_in addr;
	socklen_t len = sizeof(addr);
	getsockname(sockfd, (struct sockaddr*)&addr, &len);
	printf("IP:%s Port:%d\n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));

	return sockfd;
}
