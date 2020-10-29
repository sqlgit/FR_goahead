
/********************************* Includes ***********************************/

#include 	"goahead.h"
#include	"cJSON.h"
#include 	"tools.h"
#include 	"robot_socket.h"

/********************************* Defines ************************************/

SOCKET_INFO socket_cmd;
SOCKET_INFO socket_file;
SOCKET_INFO socket_status;
SOCKET_INFO socket_state;
SOCKET_INFO socket_vir_cmd;
SOCKET_INFO socket_vir_file;
SOCKET_INFO socket_vir_status;
STATE_FEEDBACK state_fb;
GRIPPERS_CONFIG_INFO grippers_config_info;
CTRL_STATE ctrl_state;
CTRL_STATE vir_ctrl_state;
CTRL_STATE pre_ctrl_state;
CTRL_STATE pre_vir_ctrl_state;
FB_LinkQuene fb_quene;
extern int robot_type;
//pthread_cond_t cond_cmd;
//pthread_cond_t cond_file;
//pthread_mutex_t mute_cmd;
//pthread_mutex_t mute_file;

/********************************* Function declaration ***********************/

static void state_feedback_init(STATE_FEEDBACK *fb);
static void socket_init(SOCKET_INFO *sock, const int port);
static int socket_create(SOCKET_INFO *sock);
static int socket_connect(SOCKET_INFO *sock);
static int socket_timeout(SOCKET_INFO *sock);
static int socket_send(SOCKET_INFO *sock, QElemType *node);
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

/* state feedback init */
static void state_feedback_init(STATE_FEEDBACK *fb)
{
	bzero(fb, sizeof(STATE_FEEDBACK));

	int i = 0;
	for (i = 0; i < 10; i++) {
		fb->id[i] = 0;
	}
	fb->icount = 0;
	fb->overflow = 0;
}

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
static int socket_send(SOCKET_INFO *sock, QElemType *node)
{
//	Qnode *p = sock->quene.front->next;
//	while (p != NULL) {
		/* 处于等待发送的初始状态 */
//		if (p->data.state == 0) {

			// TODO: add signal

//			QElemType *node = &p->data;
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
			//node->state = 1;
//		}
//		p = p->next;
//	}

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
	char **array = NULL;
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
	int size = 0;
	char **msg_array = NULL;
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
		/* 断包(有头无尾), 即接收到的包不完整，则跳出内圈循环，进入外圈循环，从输入流中继续读取数据 */
		if (head != NULL && tail == NULL) {
			perror("Broken package");

			break;
		}
		/* 找到了包头包尾，则提取出一帧 */
		if (head != NULL && tail != NULL && head < tail) {
			frame_len = tail - head + strlen(pack_tail);
			/* 取出整包数据然后校验解包 */
			bzero(frame, BUFFSIZE);
			strncpy(frame, head, frame_len);
			//printf("exist complete frame!\nframe data = %s\n", frame);

			/* 清空缓冲区, 并把包尾后的内容推入缓冲区 */
			bzero(buf_memory, BUFFSIZE);
			strcpy(buf_memory, (tail + strlen(pack_tail)));

			/* 把接收到的包按照分割符"III"进行分割 */
			if (string_to_string_list(frame, "III", &size, &array) == 0) {
				perror("string to string list");
				string_list_free(array, size);

				continue;
			}

			/* 遍历整个队列, 更改相关结点信息 */
		//	Qnode *p = sock->ret_quene.front->next;
		//	while (p != NULL) {
					/* 创建结点 */
					QElemType node;
					GRIPPERS_CONFIG_INFO *gri_info = &grippers_config_info;
					//printf("array[2] = %s\n", array[2]);
					//printf("array[4] = %s\n", array[4]);
					if (atoi(array[2]) == 229) {//反馈夹爪配置信息
						char *msg_content = NULL;
						int i = 0;
						cJSON *root_json = cJSON_CreateArray();
						cJSON *newitem = NULL;

						bzero(gri_info, sizeof(GRIPPERS_CONFIG_INFO));
						StringToBytes(array[4], (BYTE *)gri_info, sizeof(GRIPPERS_CONFIG_INFO));
						for(i = 0; i < MAXGRIPPER; i++) {
							newitem = cJSON_CreateObject();
							cJSON_AddNumberToObject(newitem, "id", (i+1));
							//printf("gri_info->id_company[%d] = %d", i, gri_info->id_company[i]);
							cJSON_AddNumberToObject(newitem, "name", gri_info->id_company[i]);
							cJSON_AddNumberToObject(newitem, "type", gri_info->id_device[i]);
							cJSON_AddNumberToObject(newitem, "version", gri_info->id_softversion[i]);
							cJSON_AddNumberToObject(newitem, "position", gri_info->id_bus[i]);
							cJSON_AddItemToArray(root_json, newitem);
						}
						msg_content = cJSON_Print(root_json);
						createnode(&node, atoi(array[2]), msg_content);
						free(msg_content);
						msg_content = NULL;
					} else if (atoi(array[2]) == 320 || atoi(array[2]) == 314 || atoi(array[2]) == 327 || atoi(array[2]) == 329 || atoi(array[2]) == 262 || atoi(array[2]) == 250 || atoi(array[2]) == 272 || atoi(array[2]) == 274 || atoi(array[2]) == 277 || atoi(array[2]) == 289) {// 计算TCF, 计算工具坐标系, 计算外部TCF, 计算工具TCF, 计算传感器位姿, 计算摆焊坐标系, 十点法计算传感器位姿, 八点法计算激光跟踪传感器位姿， 三点法计算跟踪传感器位姿，四点法外部轴坐标TCP计算
						char *msg_content = NULL;
						cJSON *root_json = cJSON_CreateObject();
						size = 0;
						if (string_to_string_list(array[4], ",", &size, &msg_array) == 0) {
							perror("string to string list");
							string_list_free(msg_array, size);
						}

						cJSON_AddStringToObject(root_json, "x", msg_array[0]);
						cJSON_AddStringToObject(root_json, "y", msg_array[1]);
						cJSON_AddStringToObject(root_json, "z", msg_array[2]);
						cJSON_AddStringToObject(root_json, "rx", msg_array[3]);
						cJSON_AddStringToObject(root_json, "ry", msg_array[4]);
						cJSON_AddStringToObject(root_json, "rz", msg_array[5]);
						msg_content = cJSON_Print(root_json);
						//printf("msg_content = %s\n", msg_content);
						createnode(&node, atoi(array[2]), msg_content);
						string_list_free(msg_array, size);
						free(msg_content);
						msg_content = NULL;
					} else if (atoi(array[2]) == 283) {//获取激光跟踪传感器配置信息
						char *msg_content = NULL;
						cJSON *root_json = cJSON_CreateObject();

						size = 0;
						if (string_to_string_list(array[4], ",", &size, &msg_array) == 0) {
							perror("string to string list");
							string_list_free(msg_array, size);
						}

						cJSON_AddStringToObject(root_json, "ip", msg_array[0]);
						cJSON_AddStringToObject(root_json, "port", msg_array[1]);
						cJSON_AddStringToObject(root_json, "period", msg_array[2]);
						cJSON_AddStringToObject(root_json, "protocol_id", msg_array[3]);
						msg_content = cJSON_Print(root_json);
						createnode(&node, atoi(array[2]), msg_content);
						string_list_free(msg_array, size);
						free(msg_content);
						msg_content = NULL;
					} else {
						createnode(&node, atoi(array[2]), array[4]);
					}
					if (atoi(array[2]) == 345) {//检测导入的机器人配置文件并生效
						//printf("robot cfg : array[4] = %s\n", array[4]);
						if (strcmp(array[4], "0") == 0) {
							//printf("fail！\n");
						}
						if (strcmp(array[4], "1") == 0) {
							//printf("success！\n");
							char cmd[128] = {0};
							sprintf(cmd, "cp %s %s", WEB_ROBOT_CFG, ROBOT_CFG);
							system(cmd);
						}
					}

					pthread_mutex_lock(&sock->mut);
					if (sock->msghead >= MAX_MSGHEAD) {
						sock->msghead = 1;
					} else {
						sock->msghead++;
					}
					node.msghead = sock->msghead;
					pthread_mutex_unlock(&sock->mut);
					/* 创建结点插入队列中 */
					pthread_mutex_lock(&sock->ret_mute);
					enquene(&sock->ret_quene, node);
					pthread_mutex_unlock(&sock->ret_mute);
				/* 处于已经发送的状态并且接收和发送的消息头一致, 认为已经收到服务器端的回复 */
				//if (p->data.state == 1 && p->data.msghead == atoi(array[1])) {
					/* set state to 2: have recv data */
				//	strcpy(p->data.msgcontent, array[4]);
					//p->data.msglen = strlen(p->data.msgcontent);
				//	p->data.type = atoi(array[2]);
					//p->data.state = 2;
				//}
		//		p = p->next;
		//	}
			string_list_free(array, size);
		}
		/* 残包(只找到包尾,没头)或者错包(没头没尾)的，清空缓冲区，进入外圈循环，从输入流中重新读取数据 */
		if ((head == NULL && tail != NULL) || (head == NULL && tail == NULL)) {
			perror("Incomplete package!");
			/* 清空缓冲区, 直接跳出内圈循环，到外圈循环里 */
			bzero(buf_memory, BUFFSIZE);

			break;
		}
		/* 包头在包尾后面，则扔掉缓冲区中包头之前的多余字节, 继续内圈循环 */
		if (head != NULL && tail != NULL && head > tail) {
			perror("Error package");
			/* 清空缓冲区, 并把包头后的内容推入缓冲区 */
			bzero(buf_memory, BUFFSIZE);
			strcpy(buf_memory, head);
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
	SOCKET_INFO *sock = NULL;
	Qnode *p = NULL;
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
		/* 先遍历即时指令队列,是否有数据需要发送 */
		p = sock->im_quene.front->next;
		while (p != NULL) {
			/* 处于等待发送的初始状态 */
			//if (p->data.state == 0) {
				if (socket_send(sock, &p->data) == FAIL) {
					perror("socket send");
					break;
				} else {
					/* 发送成功后删除结点信息 */
					pthread_mutex_lock(&sock->im_mute);
					dequene(&sock->im_quene, p->data);
					pthread_mutex_unlock(&sock->im_mute);
				}
			//}
			p = sock->im_quene.front->next;
		}
		/* 再查询非即时指令队列,是否有数据需要发送 */
		p = sock->quene.front->next;
		if (p != NULL) {
			/* 处于等待发送的初始状态 */
			//if (p->data.state == 0) {
				if (socket_send(sock, &p->data) == FAIL) {
					perror("socket send");
				} else {
				/* 发送成功后删除结点信息 */
					pthread_mutex_lock(&sock->mute);
					dequene(&sock->quene, p->data);
					pthread_mutex_unlock(&sock->mute);
				}
		//		break; //发送完一个非即时指令后，立刻重新进入 while 循环,看是否有即时指令需要下发
			//}
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
	initquene(&sock->im_quene);
	initquene(&sock->ret_quene);

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
		pthread_mutex_init(&sock->mut, NULL);
		pthread_mutex_init(&sock->mute, NULL);
		pthread_mutex_init(&sock->im_mute, NULL);
		pthread_mutex_init(&sock->ret_mute, NULL);
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
		pthread_mutex_destroy(&sock->mut);
		pthread_mutex_destroy(&sock->mute);
		pthread_mutex_destroy(&sock->im_mute);
		pthread_mutex_destroy(&sock->ret_mute);
		/* close socket */
		close(sock->fd);
		/* set socket status: disconnected */
		sock->connect_status = 0;
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
	CTRL_STATE *pre_state = NULL;
	int port = (int)arg;
	int recv_len = 0;
	char *recv_buf = NULL;//提取出一帧, 存放buf
	recv_buf = (char *)calloc(1, sizeof(char)*5000);

	printf("port = %d\n", port);
	switch(port) {
		case STATUS_PORT:
			sock = &socket_status;
			state = &ctrl_state;
			pre_state = &pre_ctrl_state;
			break;
		case VIR_STATUS_PORT:
			sock = &socket_vir_status;
			state = &vir_ctrl_state;
			pre_state = &pre_vir_ctrl_state;
			break;
		default:
			pthread_exit(NULL);
	}
	socket_init(sock, port);

	while(1) {
		bzero(state, sizeof(CTRL_STATE));
		bzero(pre_state, sizeof(CTRL_STATE));
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
			//printf("sizeof CTRL_STATE = %d\n", sizeof(CTRL_STATE)); /* Now struct is 1081 bytes */
			bzero(recv_buf, sizeof(char)*5000);
			recv_len = recv(sock->fd, recv_buf, (sizeof(CTRL_STATE)*3+14), 0);
			/* recv timeout or error */
			if (recv_len <= 0) {
				perror("recv");

				break;
			}
			char status_buf[5000] = {0};
			//printf("recv len = %d\n", recv_len);
			strncpy(status_buf, (recv_buf + 7), (strlen(recv_buf) - 14));
			//strrpc(status_buf, "/f/bIII", "");
			//strrpc(status_buf, "III/b/f", "");
			//printf("strlen status_buf = %d\n", strlen(status_buf));
			//printf("recv status_buf = %s\n", status_buf);
			//printf("state = %p\n", state);
			if (strlen(status_buf) == 3*sizeof(CTRL_STATE)) {
				StringToBytes(status_buf, (BYTE *)state, sizeof(CTRL_STATE));
			/*	printf("state->program_state d = %d\n", state->program_state);
				printf("state->cl_dgt_output_h = %d\n", state->cl_dgt_output_h);
				printf("state->cl_dgt_output_l = %d\n", state->cl_dgt_output_l);
				*/
			/*	int i;
				for (i = 0; i < 6; i++) {
					printf("state->jt_cur_pos[%d] = %.3lf\n", i, state->jt_cur_pos[i]);
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
	free(recv_buf);
	recv_buf = NULL;
}

/** 状态查询线程 */
void *socket_state_feedback_thread(void *arg)
{
	SOCKET_INFO *sock = NULL;
	char *state_buf = NULL;
	char *write_content = NULL;
	char *tmp_content = NULL;
	int i;
	int j;
	int num = 0;
	int port = (int)arg;
	int size = 0;
	printf("port = %d\n", port);

	state_buf = (char *)calloc(1, sizeof(char)*(STATEFB_SIZE+100));
	write_content = (char *)calloc(1, sizeof(char)*(STATEFB_SIZE+100));
	tmp_content = (char *)calloc(1, sizeof(char)*(STATEFB_SIZE+100));
	sock = &socket_state;
	/* init socket */
	socket_init(sock, port);
	/* init FB quene */
	fb_initquene(&fb_quene);
	/* init FB struct */
	state_feedback_init(&state_fb);

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
		pthread_mutex_init(&sock->mute, NULL);
		/* socket connected */
		/* set socket status: connected */
		sock->connect_status = 1;
		printf("Socket connect success: sockfd = %d\tserver_ip = %s\t server_port = %d\n", sock->fd, SERVER_IP, port);

		/* recv ctrl status */
		while (1) {
			bzero(state_buf, sizeof(char)*(STATEFB_SIZE+100));
			char **array = NULL;
			int recv_len = 0;

			//printf("sizeof CTRL_STATE = %d\n", sizeof(CTRL_STATE)); /* Now struct is bytes */
			recv_len = recv(sock->fd, state_buf, (7+1+3+5+3+sizeof(STATE_FB)*3+7), 0); /* Now recv buf is 1225 bytes*/
			//printf("recv_len = %d\n", recv_len);
			/* recv timeout or error */
			if (recv_len <= 0) {
				perror("recv");

				break;
			}
				//printf("__LINE__ = %d\n", __LINE__);
			//printf("recv len = %d\n", recv_len);
			//printf("recv state_buf = %s\n", state_buf);
			/* 把接收到的包按照分割符"III"进行分割 */
			/*if (separate_string_to_array(state_buf, "III", 5, 13000, &array) != 5) {
				perror("separate recv");

				continue;
			}*/
			if (string_to_string_list(state_buf, "III", &size, &array) == 0) {
				perror("string to string list");
				string_list_free(array, size);

				continue;
			}
			/*printf("array[0]=%s\n", array[0]);
			printf("array[1]=%s\n", array[1]);
			printf("array[2]=%s\n", array[2]);
			printf("array[3]=%s\n", array[3]);
			printf("array[4]=%s\n", array[4]);*/
			//strrpc(state_buf, "/f/bIII", "");
			//strrpc(state_buf, "III/b/f", "");
			//printf("strlen state_buf = %d\n", strlen(state_buf));
			//printf("recv state_buf = %s\n", state_buf);
			//printf("strlen(array[3]) = %d\n", strlen(array[3]));
			//printf("3*sizeof(STATE_FB) = %d\n", 3*sizeof(STATE_FB));
			if (strlen(array[3]) == 3*sizeof(STATE_FB)) {
				//printf("enter if\n");
				STATE_FB sta_fb;
				fb_createnode(&sta_fb);
				//bzero(state, sizeof(STATE_FB));
				StringToBytes(array[3], (BYTE *)&sta_fb, sizeof(STATE_FB));
				if (state_fb.type == 0) { //"0":图表查询
					if (fb_get_node_num(fb_quene) >= STATEFB_MAX) {
						state_fb.overflow = 1;
						/** clear state quene */
						pthread_mutex_lock(&sock->mute);
						fb_clearquene(&fb_quene);
						pthread_mutex_unlock(&sock->mute);
					} else {
						state_fb.overflow = 0;
					}
					/** enquene node */
					pthread_mutex_lock(&sock->mute);
					fb_enquene(&fb_quene, sta_fb);
					pthread_mutex_unlock(&sock->mute);
				} else { //"1":轨迹数据查询
					//printf("state fb = ");
					bzero(write_content, sizeof(char)*(STATEFB_SIZE+100));
					/*bzero(tmp_content, sizeof(char)*(STATEFB_SIZE+100));
					num++;
					sprintf(tmp_content, "%sNo:%d\n", write_content, num);
					strcpy(write_content, tmp_content);*/
					for(i = 0; i < 100; i++) {
						for(j = 0; j < 7; j++) {
							bzero(tmp_content, sizeof(char)*(STATEFB_SIZE+100));
							if (j < 6) {
								sprintf(tmp_content, "%s%f,", write_content, sta_fb.fb[i][j]);
							} else {
								sprintf(tmp_content, "%s%f\n", write_content, sta_fb.fb[i][j]);
							}
							strcpy(write_content, tmp_content);
						}
					}
					//printf("write_content = %s\n", write_content);
					write_file_append(FILE_STATEFB, write_content);
					//printf("end print state fb\n");
				}
			}
			string_list_free(array, size);
			//printf("after StringToBytes\n");
		}
		/* socket disconnected */
		/* 释放互斥锁 */
		pthread_mutex_destroy(&sock->mute);
		/* close socket */
		close(sock->fd);
		/* set socket status: disconnected */
		sock->connect_status = 0;
	}
	free(state_buf);
	state_buf = NULL;
	free(tmp_content);
	tmp_content = NULL;
	free(write_content);
	write_content = NULL;
}

int socket_enquene(SOCKET_INFO *sock, const int type, char *send_content, const int cmd_type)
{
	//int ret = FAIL;
	/* socket 连接已经断开 */
	if (sock->connect_status == 0) {

		return FAIL;
	}

	/* 创建结点 */
	QElemType node;
	createnode(&node, type, send_content);
	pthread_mutex_lock(&sock->mut);
	if (sock->msghead >= MAX_MSGHEAD) {
		sock->msghead = 1;
	} else {
		sock->msghead++;
	}
	node.msghead = sock->msghead;
	pthread_mutex_unlock(&sock->mut);
	/* 创建结点插入队列中 */
	if (cmd_type == 1) { //非即时指令
		pthread_mutex_lock(&sock->mute);
		enquene(&sock->quene, node);
		pthread_mutex_unlock(&sock->mute);
	} else { //即时指令
		pthread_mutex_lock(&sock->im_mute);
		enquene(&sock->im_quene, node);
		pthread_mutex_unlock(&sock->im_mute);
	}
	//pthread_cond_signal(&cond_cmd);

	//ret = quene_recv_result(node, sock->quene, recv_content);

	/* 把结点从队列中删除 */
	/*
	 pthread_mutex_lock(mute);
	 dequene(&sock->quene, node);
	 pthread_mutex_unlock(mute);
	 */
	// TODO: add signal

	//return ret;
	return SUCCESS;
}

