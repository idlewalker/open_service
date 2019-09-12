#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <pthread.h>

#include "common.h"

int g_start_thr_flag = 0;

void* process_data(void* linkinfo)
{
	int ret = 0;
	fd_set fds;
	int maxfd = 0;
	int fdcli = ((channel_t*)linkinfo)->cli.fd;
	int fdsvr = ((channel_t*)linkinfo)->srv.fd;
	int set_flag = 0;

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

		printf("THR:%d -> I am working!\n", (int)pthread_self());
		ret = select(maxfd, &fds, NULL, NULL, NULL);
		if(ret < 0)
		{
			fprintf(stderr, "select() : %s\n", strerror(errno));
			return NULL;
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
				printf("thread exit now!\n");
				return NULL;
			}
		}
		else if(FD_ISSET(fdsvr, &fds))
		{
			if(set_flag == 0)
			{
				set_flag = 1;
				g_start_thr_flag = 1;
				printf("could start a new thread now!\n");
			}

			if(send_data(&conn, S2C_DIR)<0)
			{
				printf("thread exit now!\n");
				return NULL;
			}
		}
	}
	printf("thread exit now!\n");
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

	struct sockaddr_in addr;
	socklen_t len = sizeof(addr);
	getsockname(sockfd, (struct sockaddr*)&addr, &len);
	printf("IP:%s Port:%d\n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));

	return sockfd;
}

int main(int argc, char** argv)
{
	int server_sock = 0;
	int service_sock = 0;
	int id = 0;

	//connnect to server
	if(argc != 4)
	{
		fprintf(stderr, "Usage: %s serverip serverport port2offer\n", argv[0]);
		exit(-1);
	}

	while(1)
	{
		channel_t linkinfo;
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
		
		/* fd_set fds; */
		/* int ret = 0; */
		/* while(1) */
		/* { */
			/* FD_ZERO(&fds); */
			/* FD_SET(server_sock, &fds); */
			/* ret = select(server_sock+1, &fds, NULL, NULL, NULL); */
			/* if(ret == 0) */
			/* { */
				/* sleep(1); */
				/* continue; */
			/* } */
			/* printf("server (sock:%d) start a conversation!\n", server_sock); */
			/* break; */
		/* } */
		g_start_thr_flag = 0;
		pthread_create(&thr_id, NULL, process_data, (void*)&linkinfo);
		pthread_detach(thr_id);
		id++;

		while(!g_start_thr_flag) sleep(1);
	}

	return 0;
}
