
/********************************* Includes ***********************************/

#include 	"goahead.h"
#include 	"tools.h"
#include 	"robot_socket.h"

/********************************* Defines ************************************/

SOCKET_INFO socket_cmd;
SOCKET_INFO socket_file;
SOCKET_INFO socket_status;
SOCKET_INFO socket_vir_cmd;
SOCKET_INFO socket_vir_file;
SOCKET_INFO socket_vir_status;
CTRL_STATE ctrl_state;
CTRL_STATE vir_ctrl_state;
//pthread_cond_t cond_cmd;
//pthread_cond_t cond_file;
//pthread_mutex_t mute_cmd;
//pthread_mutex_t mute_file;

/********************************* Function declaration ***********************/

static void socket_init(SOCKET_INFO *sock, const int port);
static int socket_create(SOCKET_INFO *sock);
static int socket_connect(SOCKET_INFO *sock);
static int socket_timeout(SOCKET_INFO *sock);
static int socket_send(SOCKET_INFO *sock);
static int socket_recv(SOCKET_INFO *sock, char *buf_memory);
static void *socket_send_thread(void *arg);
static void *socket_recv_thread(void *arg);
/*
static void *socket_cmd_send_thread(void *arg);
static void *socket_cmd_recv_thread(void *arg);
static void *socket_file_send_thread(void *arg);
static void *socket_file_recv_thread(void *arg);
*/

/*********************************** Code *************************************/

/* socket init */
static void socket_init(SOCKET_INFO *sock, const int port)
{
	bzero(sock, sizeof(SOCKET_INFO));

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
static int socket_send(SOCKET_INFO *sock)
{
	/* 遍历整个队列 */
	Qnode *p = sock->quene.front->next;
	while (p != NULL) {
		/* 处于等待发送的初始状态 */
		if (p->data.state == 0) {

			// TODO: add signal

			QElemType *node = &p->data;
			/* /f/bIII{msghead}III{type}III{msglen}III{msgcontent}III/b/f */
			int send_len = 4 + 3 + get_n_len(node->msghead) + 3 + get_n_len(node->type) + 3 + get_n_len(node->msglen) + 3 + node->msglen + 3 + 4 + 1; // +1 to save '\0
			char sendbuf[send_len];
			memset(sendbuf, 0, send_len);
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
			printf("send_len = %d\n", send_len);

			/* send over 1024 bytes */
			while (send_len > MAX_BUF) {
				char buf[MAX_BUF+1] = {0};
				strncpy(buf, sendbuf, MAX_BUF);
				printf("send data to socket server is: %s\n", buf);
				if (send(sock->fd, buf, MAX_BUF, 0) != MAX_BUF) {
					perror("send");
					/* set socket status: disconnected */
					sock->connect_status = 0;

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
			if (send(sock->fd, sendbuf, strlen(sendbuf), 0) != strlen(sendbuf)) {
				perror("send");
				/* set socket status: disconnected */
				sock->connect_status = 0;

				return FAIL;
			}
			/* set state to 1:send to server */
			node->state = 1;
		}
		p = p->next;
	}

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

static int socket_recv(SOCKET_INFO *sock, char *buf_memory)
{
	char recvbuf[MAX_BUF] = {0};
	char array[6][100] = {{0}};
	int recv_len = 0;

	recv_len = recv(sock->fd, recvbuf, MAX_BUF, 0);
	if (recv_len <= 0) {
		/* 认为连接已经断开 */
		perror("recv");
		/* set socket status: disconnected */
		sock->connect_status = 0;

		return FAIL;
	}
	recvbuf[recv_len] = '\0';

	//if (atoi(array[2]) != 401) {
		printf("recv data from socket server is: %s\n", recvbuf);
	//}

	/* 对于"粘包"，"断包"进行处理 */
	char *pack_head = "/f/b";
	char *pack_tail = "/b/f";
	char *head = NULL;
	char *tail = NULL;
	int frame_len = 0;
	char *frame = NULL;//提取出一帧, 存放buf
	frame =	(char *)calloc(1, sizeof(char)*BUFFSIZE);
	char *buf = NULL; // 内圈循环 buf
	buf = (char *)calloc(1, sizeof(char)*BUFFSIZE);
	buf_memory = strcat(buf_memory, recvbuf);

	/* 如果缓冲区中为空，则可以直接进行下一轮tcp请求 */
	while (strlen(buf_memory) != 0) {
		bzero(buf, BUFFSIZE);
		strcpy(buf, buf_memory);
		//printf("buf = %s\n", buf);
		head = strstr(buf, pack_head);
		tail = strstr(buf, pack_tail);
		/* 断包(有头无尾，没头没尾), 即接收到的包不完整，则跳出内圈循环，进入外圈循环，从输入流中继续读取数据 */
		if ((head != NULL && tail == NULL) || (head == NULL && tail == NULL)) {
			perror("Broken packages");

			break;
		}
		/* 找到了包头包尾，则提取出一帧 */
		if (head != NULL && tail != NULL && head < tail) {
			printf("exist complete frame!\n");
			frame_len = tail - head + strlen(pack_tail);
			/* 取出整包数据然后校验解包 */
			bzero(frame, BUFFSIZE);
			strncpy(frame, head, frame_len);
			printf("frame data = %s\n", frame);

			/* 清空缓冲区, 并把包尾后的内容推入缓冲区 */
			bzero(buf_memory, BUFFSIZE);
			strcpy(buf_memory, (tail + strlen(pack_tail)));

			/* 把接收到的包按照分割符"III"进行分割 */
			if (separate_string_to_array(frame, "III", 6, 100, (char *)&array) != 6) {
				perror("separate recv");

				continue;
				//return FAIL;
			}

			/* 遍历整个队列, 更改相关结点信息 */
			Qnode *p = sock->quene.front->next;
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
		}
		/* 残包，即只找到包尾或包头在包尾后面，则扔掉缓冲区中包尾及其之前的多余字节 */
		if ((head == NULL && tail != NULL) || (head != NULL && tail != NULL && head > tail)) {
			perror("Incomplete packages!");
			/* 清空缓冲区, 并把包尾后的内容推入缓冲区 */
			bzero(buf_memory, BUFFSIZE);
			strcpy(buf_memory, (tail + strlen(pack_tail)));
		}
	}
	free(frame);
	frame = NULL;
	free(buf);
	buf = NULL;

	return SUCCESS;
}

static void *socket_send_thread(void *arg)
{
	//pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);   //设置立即取消
	SOCKET_INFO *sock;
	sock = (SOCKET_INFO *)arg;
	while (1) {
		/*
		pthread_mutex_lock(&mute_file);
		pthread_cond_wait(&cond_file, &mute_file);
		pthread_mutex_unlock(&mute_file);
		*/
		/* socket 连接已经断开 */
		if (sock->connect_status == 0) {

			pthread_exit(NULL);
		}
		if (socket_send(sock) == FAIL) {
			perror("socket send");
		}
		delay(1);
	}
}

static void *socket_recv_thread(void *arg)
{
	SOCKET_INFO *sock;
	sock = (SOCKET_INFO *)arg;
	char *buf_memory = NULL;
	/* calloc buf */
	buf_memory = (char *)calloc(1, sizeof(char)*BUFFSIZE);
	if (buf_memory == NULL) {
		perror("calloc");
		sock->connect_status = 0;

		pthread_exit(NULL);
	}
	while (1) {
		//printf("buf_memory content = %s\n", buf_memory);
		/* socket 连接已经断开 */
		if (sock->connect_status == 0) {
			free(buf_memory);
			buf_memory = NULL;

			pthread_exit(NULL);
		}
		if (socket_recv(sock, buf_memory) == FAIL) {
			perror("socket recv");
		}
	}
}

void *socket_thread(void *arg)
{
	SOCKET_INFO *sock = NULL;
	int port = (int)arg;
	printf("port = %d\n", port);

	switch(port) {
		case CMD_PORT:
			sock = &socket_cmd;
			break;
		case FILE_PORT:
			sock = &socket_file;
			break;
		case VIR_CMD_PORT:
			sock = &socket_vir_cmd;
			break;
		case VIR_FILE_PORT:
			sock = &socket_vir_file;
			break;
		default:
			pthread_exit(NULL);
	}
	/* init socket */
	socket_init(sock, port);
	/* init socket quene */
	initquene(&sock->quene);

	while (1) {
		/* do socket connect */
		/* create socket */
		if (socket_create(sock) == FAIL) {
			/* create fail */
			perror("socket create fail");

			continue;
		}
		/* connect socket */
		if (socket_connect(sock) == FAIL) {
			/* connect fail */
			perror("socket connect fail");
			close(sock->fd);
			delay(1000);

			continue;
		}
		/* socket connected */
		/* set socket status: connected */
		sock->connect_status = 1;
		printf("Socket connect success: sockfd = %d\tserver_ip = %s\t server_port = %d\n", sock->fd, SERVER_IP, port);

		//pthread_t t_socket_cmd_send;
		//pthread_t t_socket_cmd_recv;
		/* 创建锁，相当于new一个对象 */
		//pthread_mutex_init(&mute_cmd, NULL);
		pthread_mutex_init(&sock->mute, NULL);
		//pthread_cond_init(&cond_cmd, NULL);

		/* create socket_cmd_send thread */
		if(pthread_create(&sock->t_socket_send, NULL, (void *)&socket_send_thread, (void *)sock)) {
			perror("pthread_create");
		}
		/* create socket_cmd_recv thread */
		if(pthread_create(&sock->t_socket_recv, NULL, (void *)&socket_recv_thread, (void *)sock)) {
			perror("pthread_create");
		}
		/* 等待线程退出 */
		if (pthread_join(sock->t_socket_send, NULL)) {
			perror("pthread_join");
		}
		if (pthread_join(sock->t_socket_recv, NULL)) {
			perror("pthread_join");
		}
		/* socket disconnected */
		/* 释放互斥锁 */
		pthread_mutex_destroy(&sock->mute);
		/* close socket */
		close(sock->fd);
	}
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
				delay(10000);// Heartbeat package every 10 seconds
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

void *socket_status_thread(void *arg)
{
	SOCKET_INFO *sock = NULL;
	CTRL_STATE *state = NULL;
	int port = (int)arg;
	printf("port = %d\n", port);

	switch(port) {
		case STATUS_PORT:
			sock = &socket_status;
			state = &ctrl_state;
			break;
		case VIR_STATUS_PORT:
			sock = &socket_vir_status;
			state = &vir_ctrl_state;
			break;
		default:
			pthread_exit(NULL);
	}
	socket_init(sock, port);
	bzero(state, sizeof(CTRL_STATE));

	while(1) {
		/* do socket connect */
		/* create socket */
		if (socket_create(sock) == FAIL) {
			/* create fail */
			perror("socket create fail");

			continue;
		}
		/* connect socket */
		if (socket_connect(sock) == FAIL) {
			/* connect fail */
			perror("socket connect fail");
			close(sock->fd);
			delay(1000);

			continue;
		}
		/* socket connected */
		/* set socket status: connected */
		sock->connect_status = 1;
		printf("Socket connect success: sockfd = %d\tserver_ip = %s\t server_port = %d\n", sock->fd, SERVER_IP, port);

		/* recv ctrl status */
		while (1) {
			char status_buf[4000] = {0}; /* Now recv buf is 3336 bytes*/
			int recv_len = 0;

			//printf("sizeof CTRL_STATE = %d\n", sizeof(CTRL_STATE)); /* Now struct is 1112 bytes */
			recv_len = recv(sock->fd, status_buf, 3350, 0);
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
				StringToBytes(status_buf, (BYTE *)state, sizeof(CTRL_STATE));
			/*	int i;
				for (i = 0; i < 6; i++) {
					printf("ctrl_state.jt_cur_pos[%d] = %.3lf\n", i, ctrl_state.jt_cur_pos[i]);
				}*/
			}
			//printf("after StringToBytes\n");
		}
		/* socket disconnected */
		/* close socket */
		close(sock->fd);
		/* set socket status: disconnected */
		sock->connect_status = 0;
	}
}
