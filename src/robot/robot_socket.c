
/********************************* Includes ***********************************/

#include 	"goahead.h"
#include 	"tools.h"
#include 	"robot_socket.h"

/********************************* Defines ************************************/

int socket_cmd;
int socket_file;
int socket_status;
CTRL_STATE ctrl_state;
uint8_t socket_connect_status;
extern pthread_mutex_t mute_cmd;
extern pthread_mutex_t mute_file;
extern pthread_mutex_t mute_connect_status;

/********************************* Function declaration ***********************/

static int socket_create();
static int socket_connect(int sockfd, const char *server_ip, int server_port, const int s);
static int socket_timeout(int sockfd, const int s);

/*********************************** Code *************************************/
/* socket init */
int socket_create()
{
	int sockfd = -1;

	while ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("create socket");
	}

	return sockfd;
}

/* create connect */
int socket_connect(int sockfd, const char *server_ip, int server_port, const int s)
{
	int ret = -1;
	int fdopt = -1;
	//描述服务器的socket
	struct sockaddr_in serverAddr;

	bzero(&serverAddr, sizeof(struct sockaddr_in));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(server_port);
	//inet_addr()函数，将点分十进制IP转换成网络字节序IP
	serverAddr.sin_addr.s_addr = inet_addr(server_ip);

	// set no block
	fdopt = fcntl(sockfd, F_GETFL);
	fcntl(sockfd, F_SETFL, fdopt | O_NONBLOCK);

	// client connect
	ret = connect(sockfd, (const struct sockaddr *)&serverAddr, sizeof(serverAddr));

	//unblock mode --> local connect, connect return immediately
	if (ret == 0) {
		printf("connect with server immediately\n");
		fcntl(sockfd, F_SETFL, fdopt);

		return SUCCESS;
	} else if (errno != EINPROGRESS) {
		printf("unblock connect failed!\n");
		fcntl(sockfd, F_SETFL, fdopt);

		return FAIL;
	} else if (errno == EINPROGRESS) {
		printf("unblock mode socket is connecting...\n");
	}

	// set socket connect timeout 
	ret = socket_timeout(sockfd, s);
	if (ret < 0) {
		fcntl(sockfd, F_SETFL, fdopt);

		return FAIL;
	}

	//connection successful!
	//printf("connection ready after select with the socket: %d \n", sockfd);
	// set block
	fcntl(sockfd, F_SETFL, fdopt);

	return SUCCESS;
}

/* socket timeout */
int socket_timeout(int sockfd, const int s)
{
	int ret = 0;
	fd_set writefd;
	struct timeval timeout;
	int error = 0;
	socklen_t len = sizeof(int);

	bzero(&timeout, sizeof(struct timeval));
	timeout.tv_sec = s;
	timeout.tv_usec = 0;
	FD_ZERO(&writefd);
	FD_SET(sockfd, &writefd);

	ret = select((sockfd+1), NULL, &writefd, NULL, &timeout);
	if(ret <= 0) {
		printf("connection timeout or fail! \n");

		return -1;
	}
	if(!FD_ISSET(sockfd, &writefd)) {
		printf( "no events on sockfd found\n" );

		return -1;
	}

	//get socket status
	if(getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &len) < 0) {
		printf( "get socket option failed\n" );

		return -1;
	}
	if(error != 0) {
		printf( "connection failed after select with the error: %d \n", error);

		return -1;
	}

	return 0;
}

/* socket client connect with server */
int socket_send(int clientSocket, const int no, const char *content, char *recvbuf)
{
	int send_len = strlen(content)+100;
	char sendbuf[send_len];
	char recv_success[MAX_BUF] = {0};
	char recv_fail[MAX_BUF] = {0};
	memset(sendbuf, 0, send_len);
	/* test package */
	if(no == 100) {
		sprintf(sendbuf, "/f/bIII1III%dIII%dIII%s", no, strlen(content), content);
	} else {
		sprintf(sendbuf, "/f/bIII1III%dIII%dIII%sIII/b/f", no, strlen(content), content);
#if local
		sprintf(recv_success, "/f/bIII1III%dIII%dIII%sIII/b/f", no, strlen(content), content);
#else
		sprintf(recv_success, "/f/bIII1III%dIII1III1III/b/f", no);
#endif
		sprintf(recv_fail, "/f/bIII1III%dIII1III0III/b/f", no);
	}

	/* send over 1024 bytes */
	while(send_len > 1024) {
		char buf[1024] = {0};
		strncpy(buf, sendbuf, 1024);
		if(send(clientSocket, buf, 1024, 0) != 1024) {
			perror("send");

			return FAIL;
		}
		send_len = send_len - 1024;
		char tmp_buf[send_len+1];
		memset(tmp_buf, 0, (send_len+1));
		int i;
		for(i = 0; i < send_len; i++){
			tmp_buf[i] = sendbuf[i+1024];
		}
		bzero(sendbuf, sizeof(sendbuf));
		strcpy(sendbuf, tmp_buf);
	}

	if (no != 401) {
		printf("send data to socket server is: %s\n", sendbuf);
	}

	/* send normal (low) 1024 bytes */
	if(send(clientSocket, sendbuf, strlen(sendbuf), 0) != strlen(sendbuf)) {
		perror("send");

		return FAIL;
	}

	/* socket set recv timeout */
	struct timeval timeout;
	bzero(&timeout, sizeof(struct timeval));
	timeout.tv_sec = SOCK_SEND_TIMEOUT;
	timeout.tv_usec = 0;
	if(setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout)) < 0 ) {
		perror("setsockopt");

		return FAIL;
	}

	/*If the packet is not what the packet wants within 10 seconds,
	continue to wait for 10s to see if you can receive the corresponding content,
	If you still cannot receive the corresponding data, it will be judged as timeout*/
	do {
		/* recv data */
		if(recv(clientSocket, recvbuf, MAX_BUF, 0) <= 0) {
			/* recv timeout or error */
			perror("recv");

			return FAIL;
		}
		if (no != 401) {
			printf("recv data of socket server is: %s\n", recvbuf);
		}
		/* recv fail */
		if (strcmp(recvbuf, recv_fail) == 0) {
			perror("fail");

			return FAIL;
		}
	} while (strcmp(recvbuf, recv_success) != 0);

	/* recv success */
	return SUCCESS;
}

void *socket_cmd_thread(void *arg)
{
	while (1) {
		/* create socket */
		while (1) {
			/* socket init */
			socket_cmd = socket_create();
			/* socket connect */
			if (socket_connect(socket_cmd, SERVER_IP, CMD_PORT, SOCK_TIMEOUT) == SUCCESS) {
				/* connect success */
				printf("Connect success, sockfd = %d\nserver_ip = %s, server_port = %d\n", socket_cmd, SERVER_IP, CMD_PORT);

				break;
			}
			/* connect fail */
			perror("connect");
			close(socket_cmd);
			delay_ms(1000);
		}
		/* socket connected */
		/* set socket connect flag */
		pthread_mutex_lock(&mute_connect_status);
		setbit(socket_connect_status, 1);
		pthread_mutex_unlock(&mute_connect_status);
		/* send Heartbeat packet */
		while (1) {
			char recvbuf[MAX_BUF] = {0};
			int ret = FAIL;

			pthread_mutex_lock(&mute_cmd);
			ret = socket_send(socket_cmd, 401, "HEART", recvbuf);
			pthread_mutex_unlock(&mute_cmd);
			if (ret == FAIL) {
				perror("send");

				break;
			}
			if (ret == SUCCESS) {
				/* Heartbeat package every 10 seconds */
				delay_ms(HEART_MS_TIMEOUT);
			}
		}
		/* socket disconnected */
		/* close socket */
		close(socket_cmd);
		/* clear socket connect flag */
		pthread_mutex_lock(&mute_connect_status);
		clrbit(socket_connect_status, 1);
		pthread_mutex_unlock(&mute_connect_status);
	}
}

void *socket_file_thread(void *arg)
{
	while (1) {
		/* create socket */
		while (1) {
			/* socket init */
			socket_file = socket_create();
			/* socket connect */
			if (socket_connect(socket_file, SERVER_IP, FILE_PORT, SOCK_TIMEOUT) == SUCCESS) {
				/* connect success */
				printf("Connect success, sockfd = %d\nserver_ip = %s, server_port = %d\n", socket_file, SERVER_IP, FILE_PORT);

				break;
			}
			/* connect fail */
			perror("connect");
			close(socket_file);
			delay_ms(1000);
		}
		/* socket connected */
		/* set socket connect flag */
		pthread_mutex_lock(&mute_connect_status);
		setbit(socket_connect_status, 2);
		pthread_mutex_unlock(&mute_connect_status);
		/* send Heartbeat packet */
		while (1) {
			char recvbuf[MAX_BUF] = {0};
			int ret = FAIL;

			pthread_mutex_lock(&mute_file);
			ret = socket_send(socket_file, 401, "HEART", recvbuf);
			pthread_mutex_unlock(&mute_file);
			if (ret == FAIL) {
				perror("send");

				break;
			}
			if (ret == SUCCESS) {
				/* Heartbeat package every 10 seconds */
				delay_ms(HEART_MS_TIMEOUT);
			}
		}
		/* socket disconnected */
		/* close socket */
		close(socket_file);
		/* clear socket connect flag */
		pthread_mutex_lock(&mute_connect_status);
		clrbit(socket_connect_status, 2);
		pthread_mutex_unlock(&mute_connect_status);
	}
}

void *socket_status_thread(void *arg)
{
	while(1) {
		/* create socket */
		while (1) {
			/* socket init */
			socket_status = socket_create();
			/* socket connect */
			if (socket_connect(socket_status, SERVER_IP, STATUS_PORT, SOCK_TIMEOUT) == SUCCESS) {
				/* connect success */
				printf("Connect success, sockfd = %d\nserver_ip = %s, server_port = %d\n", socket_status, SERVER_IP, STATUS_PORT);

				break;
			}
			/* connect fail */
			perror("connect");
			close(socket_status);
			delay_ms(1000);
		}
		/* socket connected */
		/* set socket connect flag */
		pthread_mutex_lock(&mute_connect_status);
		setbit(socket_connect_status, 3);
		pthread_mutex_unlock(&mute_connect_status);
		/* recv ctrl status */
		while (1) {
			char status_buf[STATUS_BUF] = {0}; /* Now recv buf is 3336 bytes*/
			int recv_len = 0;

			//printf("sizeof CTRL_STATE = %d\n", sizeof(CTRL_STATE)); /* Now struct is 1112 bytes */
			recv_len = recv(socket_status, status_buf, 3350, 0);
			/* recv timeout or error */
			if (recv_len <= 0) {
				perror("recv");

				break;
			}
			//printf("recv len = %d\n", recv_len);
			//printf("recv1 status_buf = %s\n", status_buf);
			strrpc(status_buf, "/f/bIII", "");
			strrpc(status_buf, "III/b/f", "");
			//printf("strlen status_buf = %d\n", strlen(status_buf));
			//printf("recv status_buf = %s\n", status_buf);
			//printf("sizeof CTRL_STATE = %d\n", sizeof(CTRL_STATE));
			if (strlen(status_buf) == 3*sizeof(CTRL_STATE)) {
				bzero(&ctrl_state, sizeof(ctrl_state));
				StringToBytes(status_buf, (BYTE *)&ctrl_state, sizeof(ctrl_state));
			/*	int i;
				for (i = 0; i < 6; i++) {
					printf("ctrl_state.jt_cur_pos[%d] = %.3lf\n", i, ctrl_state.jt_cur_pos[i]);
				}*/
			}
			//printf("after StringToBytes\n");
		}
		/* socket disconnected */
		/* close socket */
		close(socket_status);
		/* clear socket connect flag */
		pthread_mutex_lock(&mute_connect_status);
		clrbit(socket_connect_status, 3);
		pthread_mutex_unlock(&mute_connect_status);
	}
}