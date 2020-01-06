
/********************************* Includes ***********************************/

#include 	"goahead.h"
#include 	"tools.h"
#include 	"robot_quene.h"
#include 	"robot_socket.h"

/********************************* Defines ************************************/

SOCKET_INFO socket_cmd;
SOCKET_INFO socket_file;
SOCKET_INFO socket_status;
CTRL_STATE ctrl_state;
extern LinkQuene cmd_quene;
extern LinkQuene file_quene;
//extern pthread_cond_t cond_cmd;
//extern pthread_cond_t cond_file;
//extern pthread_mutex_t mute_cmd;
//extern pthread_mutex_t mute_file;
//extern pthread_mutex_t mute_connect_status;

/********************************* Function declaration ***********************/

static void socket_init(SOCKET_INFO *sock, const int port);
static int socket_create(SOCKET_INFO *sock);
static int socket_connect(SOCKET_INFO *sock);
static int socket_timeout(SOCKET_INFO *sock);
static int socket_send(const SOCKET_INFO sock, QElemType *node);
static int socket_recv(SOCKET_INFO *sock, const LinkQuene q);
static void *socket_cmd_send_thread(void *arg);
static void *socket_cmd_recv_thread(void *arg);
static void *socket_file_send_thread(void *arg);
static void *socket_file_recv_thread(void *arg);

/*********************************** Code *************************************/

/* socket init */
static void socket_init(SOCKET_INFO *sock, const int port)
{
	sock->fd = 0;
	strcpy(sock->server_ip, SERVER_IP);
	sock->server_port = port;
	sock->select_timeout = SOCK_SELECT_TIMEOUT;
	sock->connect_status = 0;
	sock->msghead = 0;
}

/* socket create */
static int socket_create(SOCKET_INFO *sock)
{
	if ((sock->fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {

		return FAIL;
	}

	return SUCCESS;
}

/* create connect */
static int socket_connect(SOCKET_INFO *sock)
{
	int ret = -1;
	int fdopt = -1;
	//描述服务器的socket
	struct sockaddr_in serverAddr;

	bzero(&serverAddr, sizeof(struct sockaddr_in));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(sock->server_port);
	//inet_addr()函数，将点分十进制IP转换成网络字节序IP
	serverAddr.sin_addr.s_addr = inet_addr(sock->server_ip);

	// set no block
	fdopt = fcntl(sock->fd, F_GETFL);
	fcntl(sock->fd, F_SETFL, fdopt | O_NONBLOCK);

	// client connect
	ret = connect(sock->fd, (const struct sockaddr *)&serverAddr, sizeof(serverAddr));

	//unblock mode --> local connect, connect return immediately
	if (ret == 0) {
		printf("connect with server immediately\n");
		fcntl(sock->fd, F_SETFL, fdopt);

		return SUCCESS;
	} else if (errno != EINPROGRESS) {
		printf("unblock connect failed!\n");
		fcntl(sock->fd, F_SETFL, fdopt);

		return FAIL;
	} else if (errno == EINPROGRESS) {
		printf("unblock mode socket is connecting...\n");
	}

	// set socket connect timeout 
	ret = socket_timeout(sock);
	if (ret < 0) {
		//printf("__LINE__ = %d, set block\n", __LINE__);
		fcntl(sock->fd, F_SETFL, fdopt);

		return FAIL;
	}

	//connection successful!
	//printf("connection ready after select with the socket: %d \n", sock->fd);
	// set block
	//printf("__LINE__ = %d, set block\n", __LINE__);
	fcntl(sock->fd, F_SETFL, fdopt);

	return SUCCESS;
}

/* socket timeout */
static int socket_timeout(SOCKET_INFO *sock)
{
	int ret = 0;
	fd_set writefd;
	struct timeval timeout;
	int error = 0;
	socklen_t len = sizeof(int);

	bzero(&timeout, sizeof(struct timeval));
	timeout.tv_sec = sock->select_timeout;
	timeout.tv_usec = 0;
	FD_ZERO(&writefd);
	FD_SET(sock->fd, &writefd);

	ret = select((sock->fd+1), NULL, &writefd, NULL, &timeout);
	if(ret <= 0) {
		printf("connection timeout or fail!\n");

		return -1;
	}
	if(!FD_ISSET(sock->fd, &writefd)) {
		printf("no events on sock fd found\n");

		return -1;
	}

	//get socket status
	if(getsockopt(sock->fd, SOL_SOCKET, SO_ERROR, &error, &len) < 0) {
		printf("get socket option failed\n");

		return -1;
	}
	if(error != 0) {
		printf("connection failed after select with the error: %d \n", error);

		return -1;
	}

	return 0;
}

/* socket send */
static int socket_send(const SOCKET_INFO sock, QElemType *node)
{
	/* /f/bIII{msghead}III{type}III{msglen}III{msgcontent}III/b/f */
	int send_len = 4 + 3 + get_n_len(node->msghead) + 3 + get_n_len(node->type) + 3 + get_n_len(node->msglen) + 3 + node->msglen + 3 + 4 + 1; // +1 to save '\0
	char sendbuf[send_len];
	memset(sendbuf, 0, (send_len));
	/* sprintf 会在 sendbuf 最后自动添加一个 '\0' 作为字符串结束的标识符 */
	sprintf(sendbuf, "/f/bIII%dIII%dIII%dIII%sIII/b/f", node->msghead, node->type, node->msglen, node->msgcontent);
#if test_package
	if (node->type == 106) {
		sprintf(sendbuf, "/f/bIII%dIII%dIII%dIII%sIII", node->msghead, node->type, node->msglen, node->msgcontent);
	}
	if (node->type == 100) {
		sprintf(sendbuf, "/b/f");
	}
#endif
	//printf("send_len = %d\n", send_len);

	/* send over 1024 bytes */
	while (send_len > MAX_BUF) {
		char buf[MAX_BUF] = {0};
		strncpy(buf, sendbuf, MAX_BUF);
		printf("send data to socket server is: %s\n", buf);
		if(send(sock.fd, buf, MAX_BUF, 0) != MAX_BUF) {
			perror("send");

			return FAIL;
		}
		send_len = send_len - MAX_BUF;
		char tmp_buf[send_len];
		memset(tmp_buf, 0, send_len);
		int i;
		for(i = 0; i < send_len; i++){
			tmp_buf[i] = sendbuf[i+MAX_BUF];
		}
		bzero(sendbuf, sizeof(sendbuf));
		strcpy(sendbuf, tmp_buf);
	}

	//if (node->type != 401) {
		printf("send data to socket server is: %s\n", sendbuf);
	//}

	/* send normal (low) 1024 bytes */
	if (send(sock.fd, sendbuf, strlen(sendbuf), 0) != strlen(sendbuf)) {
		perror("send");

		return FAIL;
	}
	/* set state to 1:send to server */
	node->state = 1;

	return SUCCESS;

	/* socket set recv timeout */
	/*struct timeval timeout;
	bzero(&timeout, sizeof(struct timeval));
	timeout.tv_sec = 10;
	timeout.tv_usec = 0;
	if (setsockopt(sock->fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout)) < 0 ) {
		perror("setsockopt");

		return FAIL;
	}*/

	/*
	HeartbBeat Package:
	If cannot receive the corresponding data within 10 seconds, perror recv, return FAIL, socket disconnect
	If the packet is not what the packet wants within 10 seconds, continue to wait for 10s to see if you can receive the corresponding content, if you still cannot receive the corresponding data, perror recv, return FAIL, socket disconnect.
	*/

	/*
	Normal Package:
	If cannot receive the corresponding data within 10 seconds, perror recv, return FAIL.
	If the packet is recv_fail, perror fail, return FAIL.
	If the packet is not what the packet wants within 10 seconds, continue to wait for 10s to see if you can receive the corresponding content, if you still cannot receive the corresponding data, perror recv, return FAIL.
	*/
//	do {
		/* recv data */
//		if (recv(sock->fd, recvbuf, MAX_BUF, 0) <= 0) {
			/* recv timeout or error */
//			perror("recv");
//
//			return FAIL;
//		}
		/* normal package */
//		if (sock->send_no != 401) {
//			printf("recv data of socket server is: %s\n", recvbuf);
			/* recv fail */
//			if (strcmp(recvbuf, recv_fail) == 0) {
//				perror("fail");
//
//				return FAIL;
//			}
//		}
//	} while (strcmp(recvbuf, recv_success) != 0);
}

static int socket_recv(SOCKET_INFO *sock, const LinkQuene q)
{
	char recvbuf[MAX_BUF] = {0};
	char array[6][100] = {{0}};

	// TODO: 解决粘包的问题
	if (recv(sock->fd, recvbuf, MAX_BUF, 0) <= 0) {
		/* 认为连接已经断开 */
		sock->connect_status = 0;
		perror("recv");

		return FAIL;
	}

	//if (atoi(array[2]) != 401) {
		printf("recv data from socket server is: %s\n", recvbuf);
	//}

	/* 把接收到的包按照分割符"III"进行分割 */
	if(separate_string_to_array(recvbuf, "III", 6, 100, (char *)&array) != 6) {
		perror("separate recv");

		return FAIL;
	}

	/* 遍历整个队列, 更改相关结点信息 */
	Qnode *p = q.front->next;
	while (p != NULL) {
		/* 处于已经发送的状态并且接收和发送的消息头一致, 认为已经收到服务器端的回复 */
		if (p->data.state == 1 && p->data.msghead == atoi(array[1])) {
			/* set state to 2: have recv data */
			strcpy(p->data.msgcontent, array[4]);
			p->data.msglen = strlen(p->data.msgcontent);
			p->data.state = 2;
		}
		p = p->next;
	}

	return SUCCESS;
}

static void *socket_cmd_send_thread(void *arg)
{
	while (1) {
		/* socket 连接已经断开 */
		if (socket_cmd.connect_status == 0) {

			pthread_exit(NULL);
		}
		/* 遍历整个队列 */
		Qnode *p = cmd_quene.front->next;
		while (p != NULL) {
			/* 处于等待发送的初始状态 */
			if (p->data.state == 0) {
				socket_send(socket_cmd, &p->data);
				// TODO: add signal
			}
			p = p->next;
		}
		usleep(1);
	}
}

static void *socket_cmd_recv_thread(void *arg)
{
	while (1) {
		/* socket 连接已经断开 */
		if (socket_cmd.connect_status == 0) {

			pthread_exit(NULL);
		}
		socket_recv(&socket_cmd, cmd_quene);
	}
}

void *socket_cmd_thread(void *arg)
{
	/* init cmd quene */
	initquene(&cmd_quene);
	socket_init(&socket_cmd, CMD_PORT);

	while (1) {
		/* do socket connect */
		/* create socket */
		if (socket_create(&socket_cmd) == FAIL) {
			/* create fail */
			perror("socket create fail");
			usleep(1);

			continue;
		}
		/* connect socket */
		if (socket_connect(&socket_cmd) == FAIL) {
			/* connect fail */
			perror("socket connect fail");
			close(socket_cmd.fd);
			sleep(1);

			continue;
		}
		/* socket connected */
		socket_cmd.connect_status = 1;
		printf("Socket connect success: sockfd = %d\tserver_ip = %s\t server_port = %d\n", socket_cmd.fd, SERVER_IP, CMD_PORT);

		pthread_t t_socket_cmd_send;
		pthread_t t_socket_cmd_recv;
		/* create socket_cmd_send thread */
		if(pthread_create(&t_socket_cmd_send, NULL, (void *)&socket_cmd_send_thread, NULL)) {
			perror("pthread_create");
		}
		/* create socket_cmd_recv thread */
		if(pthread_create(&t_socket_cmd_recv, NULL, (void *)&socket_cmd_recv_thread, NULL)) {
			perror("pthread_create");
		}
		/* 等待线程退出 */
		if (pthread_join(t_socket_cmd_send, NULL)) {
			perror("pthread_join");
		}
		if (pthread_join(t_socket_cmd_recv, NULL)) {
			perror("pthread_join");
		}
		/* close socket */
		close(socket_cmd.fd);
		/* socket disconnected */
		socket_cmd.connect_status = 0;
	}
}

static void *socket_file_send_thread(void *arg)
{
	//pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);   //设置立即取消
	while (1) {
		/*
		pthread_mutex_lock(&mute_file);
		pthread_cond_wait(&cond_file, &mute_file);
		pthread_mutex_unlock(&mute_file);
		*/
		/* socket 连接已经断开 */
		if (socket_file.connect_status == 0) {

			pthread_exit(NULL);
		}
		/* 遍历整个队列 */
		Qnode *p = file_quene.front->next;
		while (p != NULL) {
			/* 处于等待发送的初始状态 */
			if (p->data.state == 0) {
				socket_send(socket_file, &p->data);
			}
			p = p->next;
		}
		usleep(1);
	}
}

static void *socket_file_recv_thread(void *arg)
{
	//pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);   //设置立即取消
	while (1) {
		/* socket 连接已经断开 */
		if (socket_file.connect_status == 0) {

			pthread_exit(NULL);
		}
		socket_recv(&socket_file, file_quene);
	}
}

void *socket_file_thread(void *arg)
{
	/* init file quene */
	initquene(&file_quene);
	socket_init(&socket_file, FILE_PORT);

	while (1) {
		/* do socket connect */
		/* create socket */
		if (socket_create(&socket_file) == FAIL) {
			/* create fail */
			perror("socket create fail");
			usleep(1);

			continue;
		}
		/* connect socket */
		if (socket_connect(&socket_file) == FAIL) {
			/* connect fail */
			perror("socket connect fail");
			close(socket_file.fd);
			sleep(1);

			continue;
		}
		/* socket connected */
		socket_file.connect_status = 1;
		printf("Socket connect success: sockfd = %d\tserver_ip = %s\t server_port = %d\n", socket_file.fd, SERVER_IP, FILE_PORT);

		pthread_t t_socket_file_send;
		pthread_t t_socket_file_recv;
		/* create socket_file_send thread */
		if(pthread_create(&t_socket_file_send, NULL, (void *)&socket_file_send_thread, NULL)) {
			perror("pthread_create");
		}
		/* create socket_file_recv thread */
		if(pthread_create(&t_socket_file_recv, NULL, (void *)&socket_file_recv_thread, NULL)) {
			perror("pthread_create");
		}
	/*
		// send Heartbeat packet
		while (1) {
			pthread_mutex_lock(&mute_file);
			QElemType node;
			createnode(&node, 401, "HEART", 0);
			if (socket_file.msghead >= MAX_MSGHEAD) {
				socket_file.msghead = 1;
			} else {
				socket_file.msghead++;
			}
			node.msghead = socket_file.msghead;
			enquene(&file_quene, node);
			//pthread_cond_signal(&cond_cmd);
			pthread_mutex_unlock(&mute_file);
		//	time_t currentTm = time(NULL);
		//	printf("before time : %s\n", (asctime(localtime(&currentTm))));
			int ret = FAIL;
			ret = socket_recv_result(node, file_quene);
		//	printf("after time : %s\n", (asctime(localtime(&currentTm))));

			pthread_mutex_lock(&mute_file);
			dequene(&file_quene, node);
			pthread_mutex_unlock(&mute_file);

			if (ret == FAIL) {
				perror("socket disconnect");

				break;
			}
			if (ret == SUCCESS) {
				sleep(10);// Heartbeat package every 10 seconds
			}
		}
		*/
		// cancel pthread
	/*	if (pthread_cancel(t_socket_file_send)) {
			perror("pthread cancel");
		}
		if (pthread_cancel(t_socket_file_recv)) {
			perror("pthread cancel");
		}*/
		/* 等待线程退出 */
		if (pthread_join(t_socket_file_send, NULL)) {
			perror("pthread_join");
		}
		if (pthread_join(t_socket_file_recv, NULL)) {
			perror("pthread_join");
		}
		/* close socket */
		close(socket_file.fd);
		/* socket disconnected */
		socket_file.connect_status = 0;
	}
}

void *socket_status_thread(void *arg)
{
	bzero(&ctrl_state, sizeof(ctrl_state));
	socket_init(&socket_status, STATUS_PORT);

	while(1) {
		/* do socket connect */
		/* create socket */
		if (socket_create(&socket_status) == FAIL) {
			/* create fail */
			perror("socket create fail");
			usleep(1);

			continue;
		}
		/* connect socket */
		if (socket_connect(&socket_status) == FAIL) {
			/* connect fail */
			perror("socket connect fail");
			close(socket_status.fd);
			sleep(1);

			continue;
		}
		/* socket connected */
		socket_status.connect_status = 1;
		printf("Socket connect success: sockfd = %d\tserver_ip = %s\t server_port = %d\n", socket_status.fd, SERVER_IP, STATUS_PORT);

		/* recv ctrl status */
		while (1) {
			char status_buf[4000] = {0}; /* Now recv buf is 3336 bytes*/
			int recv_len = 0;

			//printf("sizeof CTRL_STATE = %d\n", sizeof(CTRL_STATE)); /* Now struct is 1112 bytes */
			recv_len = recv(socket_status.fd, status_buf, 3350, 0);
			/* recv timeout or error */
			if (recv_len <= 0) {
				perror("recv");

				break;
			}
			//printf("recv len = %d\n", recv_len);
			//printf("recv status_buf = %s\n", status_buf);
			strrpc(status_buf, "/f/bIII", "");
			strrpc(status_buf, "III/b/f", "");
			//printf("strlen status_buf = %d\n", strlen(status_buf));
			//printf("recv status_buf = %s\n", status_buf);
			if (strlen(status_buf) == 3*sizeof(CTRL_STATE)) {
				StringToBytes(status_buf, (BYTE *)&ctrl_state, sizeof(ctrl_state));
			/*	int i;
				for (i = 0; i < 6; i++) {
					printf("ctrl_state.jt_cur_pos[%d] = %.3lf\n", i, ctrl_state.jt_cur_pos[i]);
				}*/
			}
			//printf("after StringToBytes\n");
		}
		/* close socket */
		close(socket_status.fd);
		/* socket disconnected */
		socket_status.connect_status = 0;
	}
}
