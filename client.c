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

		ret = select(maxfd, &fds, NULL, NULL, NULL);
		if(ret < 0)
		{
			fprintf(stderr, "select() : %s\n", strerror(errno));
			goto l_exit;
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
				goto l_exit;
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
				goto l_exit;
			}
		}
	}
l_exit:
	//in case of quiting without setting another thread start flag
	if(set_flag == 0)
	{
		g_start_thr_flag = 1;
		printf("thread down, restart a new thread now!\n");
	}
	printf("thread exit now!\n");
	return NULL;
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
		
		g_start_thr_flag = 0;
		pthread_create(&thr_id, NULL, process_data, (void*)&linkinfo);
		pthread_detach(thr_id);
		id++;

		while(!g_start_thr_flag) sleep(1);
	}

	return 0;
}
