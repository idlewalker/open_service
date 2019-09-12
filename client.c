#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <pthread.h>

#include "common.h"

void* process_data(void* linkinfo)
{
	int ret = 0;
	fd_set fds;
	int maxfd = 0;
	int fdcli = ((channel_t*)linkinfo)->cli.fd;
	int fdsvr = ((channel_t*)linkinfo)->srv.fd;
	struct timeval timeout={3,0};

	channel_t conn;
	memset(&conn, 0, sizeof(conn));
	conn.cli.fd = fdcli;
	conn.srv.fd = fdsvr;
	maxfd = fdcli > fdsvr ? fdcli+1 : fdsvr+1;
	while(1)
	{
		FD_ZERO(&fds);
		FD_SET(fdcli, &fds);
		FD_SET(fdsvr, &fds);

		ret = select(maxfd, &fds, NULL, NULL, &timeout);
		if(ret < 0)
		{
			fprintf(stderr, "select() : %s\n", strerror(errno));
			exit(-1);
		}
		if(ret == 0) 
		{
			sleep(1);
			continue;
		}
		if(FD_ISSET(fdcli, &fds))
		{
			if(send_data(&conn, C2S_DIR)<0)
			{
				return NULL;
			}
		}
		else if(FD_ISSET(fdsvr, &fds))
		{
			if(send_data(&conn, S2C_DIR)<0)
			{
				return NULL;
			}
		}
	}
	return NULL;
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

	if(connect(sockfd, (struct sockaddr*)&server, sizeof(server)) != 0)
	{
		fprintf(stderr, "connect() : %s\n", strerror(errno));
		return -1;
	}

	/* if(send(sockfd, "client", 6, 0)<0) */
	/* { */
		/* fprintf(stderr, "crc send() %s\n", strerror(errno)); */
		/* return -1; */
	/* } */

	return sockfd;
}

int main(int argc, char** argv)
{
	int server_sock = 0;
	int service_sock = 0;
	int id = 0;
	channel_t linkinfo;

	//connnect to server
	if(argc != 4)
	{
		fprintf(stderr, "Usage: %s serverip serverport port2offer\n", argv[0]);
		exit(-1);
	}

	while(1)
	{
		server_sock = connect_to_target(argv[1], atoi(argv[2]));
		if(server_sock <= 0)
		{
			fprintf(stderr, "connect to server failed(ip:%s, port:%d)!\n", argv[1], atoi(argv[2]));
			sleep(5);
			continue;
		}

		service_sock = connect_to_target((char*)"127.0.0.1", atoi(argv[3]));
		if(service_sock <= 0)
		{
			fprintf(stderr, "client(ID:%d) connect to service failed(port:%d)!\n",id, atoi(argv[3]));
			close(server_sock);
			sleep(5);
			continue;
		}
		fprintf(stdout, "wait for client(ID:%d) come to use service on port:%d\n", id, atoi(argv[3]));
		pthread_t thr_id;
		linkinfo.cli.fd = service_sock;
		linkinfo.srv.fd = server_sock;
		
		fd_set fds;
		int ret = 0;
		struct timeval timeout={2,0};
		while(1)
		{
			FD_ZERO(&fds);
			FD_SET(server_sock, &fds);
			ret = select(server_sock+1, &fds, NULL, NULL, &timeout);
			if(ret == 0)
			{
				sleep(1);
				continue;
			}
			printf("server (sock:%d) start a conversation!\n", server_sock);
			break;
		}

		pthread_create(&thr_id, NULL, process_data, (void*)&linkinfo);
		pthread_detach(thr_id);
		id++;
	}

	return 0;
}
