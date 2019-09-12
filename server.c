#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#include "common.h"

int socket_init(uint16_t my_port)
{
	int sockfd = 0;
	int ret = 0;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd < 0)
	{
		fprintf(stderr, "get inside conn : socket() -> %s\n", strerror(errno));
		return -1;
	}

	int val = 1;
	ret = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));
	if(ret != 0)
	{
		fprintf(stderr, "setsocketopt() : %s\n", strerror(errno));
		return -1;
	}

	/* val = 1; */
	/* ret = ioctl(sockfd, FIONBIO, &val); */
	/* if(ret != 0) */
	/* { */
		/* fprintf(stderr, "ioctl() %s\n", strerror(errno)); */
		/* return -1; */
	/* } */
	struct sockaddr_in myaddr;
	bzero(&myaddr, sizeof(myaddr));
	myaddr.sin_family = AF_INET;
	myaddr.sin_port = htons(my_port);
	myaddr.sin_addr.s_addr = INADDR_ANY;

	ret = bind(sockfd, (struct sockaddr*)&myaddr, sizeof(myaddr));
	if(ret != 0)
	{
		fprintf(stderr, "get inside conn : bind() -> %s\n", strerror(errno));
		return -1;
	}

	listen(sockfd, 5);

	return sockfd;
}

int get_income_connection(tuple_t* conn, int sockfd)
{
	struct sockaddr_in inner_conn;
	socklen_t	len;

	len = sizeof(inner_conn);
	conn->fd = accept(sockfd, (struct sockaddr*)&inner_conn, &len);
	if(conn->fd < 0)
	{
		fprintf(stderr, "get inside conn : accept(%d) -> %s\n", sockfd, strerror(errno));
		return -1;
	}

	struct sockaddr_in sa;
	socklen_t sa_len = sizeof(sa);
	if(getpeername(conn->fd, (struct sockaddr*)&sa, &sa_len) == 0)
	{
		strcpy(conn->ip, inet_ntoa(sa.sin_addr));
		conn->port = ntohs(sa.sin_port);
	}

	return 0;
}

void* process_channal_data(void* arg)
{
	channel_t* 	conn = (channel_t*)arg;
	int ret = 0;
	fd_set fdsets;
	int maxfd = 0;

	fprintf(stdout, "Communication start between socket %d(%s:%d) and %d(%s,%d)!\n", 
			conn->cli.fd, conn->cli.ip, conn->cli.port, conn->srv.fd, conn->srv.ip, conn->srv.port);

	maxfd = conn->cli.fd > conn->srv.fd ? conn->cli.fd + 1 : conn->srv.fd + 1;
	while(1)
	{
		FD_ZERO(&fdsets);
		FD_SET(conn->cli.fd, &fdsets);
		FD_SET(conn->srv.fd, &fdsets);

		ret = select(maxfd, &fdsets, NULL, NULL, NULL);
		if(ret < 0)
		{
			fprintf(stderr, "process over by select() : %s\n", strerror(errno));
			close(conn->cli.fd);
			close(conn->srv.fd);
			printf("thread exit now!\n");
			return NULL;
		}
		if(ret == 0)
		{
			sleep(1);
			continue;
		}
		if(FD_ISSET(conn->cli.fd, &fdsets))
		{
			if(send_data(conn, C2S_DIR)<0)
			{
				close(conn->cli.fd);
				close(conn->srv.fd);
				printf("thread exit now!\n");
				return NULL;
			}
		}
		else if(FD_ISSET(conn->srv.fd, &fdsets))
		{
			if(send_data(conn, S2C_DIR)<0)
			{
				close(conn->cli.fd);
				close(conn->srv.fd);
				printf("thread exit now!\n");
				return NULL;
			}
		}
	}

	printf("thread exit now!\n");
	return NULL;
}

int main(int argc, char** argv)
{
	int ret = 0;
	
	int in_sock = 0;
	int out_sock = 0;

	if(argc != 3)
	{
		fprintf(stdout, "Usage: %s insideport outsideport\n", argv[0]);
		exit(-1);
	}
	
	//listen for inside port connection
	in_sock = socket_init(atoi(argv[1]));
	if(in_sock < 0)
	{
		fprintf(stderr, "socket_init() err!\n");
		exit(-1);
	}

	out_sock = socket_init(atoi(argv[2]));
	if(out_sock < 0)
	{
		fprintf(stderr, "socket_init() err!\n");
		exit(-1);
	}

	while(1)
	{

		pthread_t thr_id;
		channel_t conn;
		ret = get_income_connection(&conn.srv, in_sock);
		if(ret != 0)
		{
			fprintf(stderr, "failed to get connect from inside!\n");
			sleep(5);
			continue;
		}

		/* fprintf(stdout, "Inner side machine(ip:%s, port:%d) is online!\n", conn.srv.ip, conn.srv.port); */

		ret = get_income_connection(&conn.cli, out_sock);
		if(ret != 0)
		{
			fprintf(stderr, "failed to get connect from out!\n");
			close(conn.srv.fd);
			sleep(5);
			continue;
		}

		/* fprintf(stdout, "Outer side machine(ip:%s, port:%d) is online!\n", conn.cli.ip, conn.cli.port); */

		pthread_create(&thr_id, NULL, process_channal_data, (void*)&conn);
		pthread_detach(thr_id);
	}

	return 0;
}
