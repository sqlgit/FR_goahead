
/********************************* Includes ***********************************/

#include 	"goahead.h"
#include	"cJSON.h"
#include 	"tools.h"
#include 	"check_lua_file.h"
#include 	"robot_socket.h"

/********************************* Defines ************************************/

SOCKET_INFO socket_cmd;
SOCKET_INFO socket_file;
SOCKET_INFO socket_status;
SOCKET_INFO socket_state;
SOCKET_SERVER_INFO socket_upper_computer;
SOCKET_INFO socket_torquesys;
SOCKET_INFO socket_vir_cmd;
SOCKET_INFO socket_vir_file;
SOCKET_INFO socket_vir_status;
STATE_FEEDBACK state_fb;
CTRL_STATE ctrl_state;
CTRL_STATE vir_ctrl_state;
CTRL_STATE pre_ctrl_state;
CTRL_STATE pre_vir_ctrl_state;
FB_LinkQuene fb_quene;
extern int robot_type;
extern char error_info[ERROR_SIZE];
TORQUE_SYS torquesys;
TORQUE_SYS_STATE torque_sys_state;
char lua_filename[FILENAME_SIZE] = "";
//pthread_cond_t cond_cmd;
//pthread_cond_t cond_file;
//pthread_mutex_t mute_cmd;
//pthread_mutex_t mute_file;

/********************************* Function declaration ***********************/

static void state_feedback_init(STATE_FEEDBACK *fb);
static void socket_init(SOCKET_INFO *sock, const int port);
static void socket_server_init(SOCKET_SERVER_INFO *sock, const int port);
static int socket_create(SOCKET_INFO *sock);
static int socket_server_create(SOCKET_SERVER_INFO *sock);
static int socket_connect(SOCKET_INFO *sock);
static int socket_timeout(SOCKET_INFO *sock);
static int socket_pkg_handle(char *buf_memory, char *sec_buf_memory, char *frame, int buf_size, int frame_size);
static int buf_memory_handle(char *buf_memory, int buf_size, int *buf_memory_len, char *frame, int frame_size);
static int socket_send(SOCKET_INFO *sock, QElemType *node);
static int socket_recv(SOCKET_INFO *sock, char *buf_memory);
static void *socket_send_thread(void *arg);
static void *socket_recv_thread(void *arg);
static int socket_bind_listen(SOCKET_SERVER_INFO *sock);
static int socket_upper_computer_send(SOCKET_SERVER_INFO *sock, const int cmd_id, const int data_len, const char *data_content);
static void *socket_upper_computer_recv_send(SOCKET_SERVER_INFO *sock);
static void init_torquesys();
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
	for (i = 0; i < STATEFB_ID_MAXNUM; i++) {
		fb->id[i] = 0;
	}
	fb->icount = 0;
	fb->overflow = 0;
	fb->index = 0;
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

/* socket server init */
static void socket_server_init(SOCKET_SERVER_INFO *sock, const int port)
{
	bzero(sock, sizeof(SOCKET_SERVER_INFO));

	sock->serv_fd = 0;
	sock->clnt_fd = 0;
	strcpy(sock->server_ip, SERVER_IP);
	sock->server_port = port;
	sock->connect_status = 0;
	sock->msghead = 0;
}

/* socket create */
static int socket_create(SOCKET_INFO *sock)
{
	if ((sock->fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {

		return FAIL;
	}
	//printf("sock->fd = %d\n", sock->fd);

	return SUCCESS;
}

/* socket server create */
static int socket_server_create(SOCKET_SERVER_INFO *sock)
{
	if ((sock->serv_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {

		return FAIL;
	}
	//printf("sock->fd = %d\n", sock->fd);

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

/**
	function : 数据包收包后对于“粘包”，“断包”，“残包”，“错包”的处理

	buf_memory[IN] : 待处理数据缓冲区

	sec_buf_memory[IN] : 二级数据缓冲区

	frame[out]：获取到的完整一帧数据包

	buf_size : 分配的缓存区大小

	frame_size : 分配的一帧数据存储的大小

	return: 如果有完整一帧数据包，返回其数据长度，否则返回 0 for error
*/
static int socket_pkg_handle(char *buf_memory, char *sec_buf_memory, char *frame, int buf_size, int frame_size)
{
	//printf("buf_memory is: %s\n", buf_memory);
	assert(buf_memory != NULL);

	char *pack_head = "/f/b";
	char *pack_tail = "/b/f";
	char *head = NULL;
	char *tail = NULL;
	int frame_len = 0;
	//clock_t time_0, time_1, time_2, time_3, time_4, time_5;

	//time_0 = clock();
	//printf("0, %d\n", time_0);

	//time_1 = clock();
	//printf("1, %d\n", time_1);
	/* 如果缓冲区中为空，则可以直接进行下一轮tcp请求 */
	while (strlen(buf_memory) != 0) {
		//time_2 = clock();
		//printf("2, %d\n", time_2);
		bzero(sec_buf_memory, buf_size);
		memcpy(sec_buf_memory, buf_memory, strlen(buf_memory));
		head = strstr(sec_buf_memory, pack_head);
		tail = strstr(sec_buf_memory, pack_tail);
		//time_3 = clock();
		//printf("3, %d\n", time_3);
		/* 断包(有头无尾), 即接收到的包不完整，则跳出内圈循环，进入外圈循环，从输入流中继续读取数据 */
		if (head != NULL && tail == NULL) {
		//	perror("Broken package");

			break;
		}

		/* 找到了包头包尾，则提取出一帧 */
		if (head != NULL && tail != NULL && head < tail) {
			frame_len = tail - head + strlen(pack_tail);
			//printf("frame_len = %d\n", frame_len);
			//*frame = (char *)calloc(1, sizeof(char)*BUFFSIZE);
			/* 取出整包数据然后校验解包 */
			bzero(frame, frame_size);
			strncpy(frame, head, frame_len);
			//time_4 = clock();
			//printf("4, %d\n", time_4);
			//printf("exist complete frame!\nframe data = %s\n", *frame);

			/* 清空缓冲区, 并把包尾后的内容推入缓冲区 */
			bzero(buf_memory, buf_size);
			memcpy(buf_memory, (tail+strlen(pack_tail)), (strlen(tail)-strlen(pack_tail)));
			//time_5 = clock();
			//printf("5, %d\n", time_5);

			break;
		}
		/* 残包(只找到包尾,没头)或者错包(没头没尾)的，清空缓冲区，进入外圈循环，从输入流中重新读取数据 */
		if ((head == NULL && tail != NULL) || (head == NULL && tail == NULL)) {
			perror("Incomplete package!");
			/* 清空缓冲区, 直接跳出内圈循环，到外圈循环里 */
			bzero(buf_memory, buf_size);

			break;
		}
		/* 包头在包尾后面，则扔掉缓冲区中包头之前的多余字节, 继续内圈循环 */
		if (head != NULL && tail != NULL && head > tail) {
			perror("Error package");
			/* 清空缓冲区, 并把包头后的内容推入缓冲区 */
			bzero(buf_memory, buf_size);
			memcpy(buf_memory, head, strlen(head));
		}
	}

	return frame_len;
}

/* socket send */
static int socket_send(SOCKET_INFO *sock, QElemType *node)
{
	char *sendbuf = NULL;
	//char buf[MAX_BUF+1] = {0};
	int send_len = 0;
	int send_index = 0;

	send_len = 4 + 3 + get_n_len(node->msghead) + 3 + get_n_len(node->type) + 3 + get_n_len(node->msglen) + 3 + node->msglen + 3 + 4 + 1; // +1 to save '\0
	sendbuf = (char *)calloc(1, sizeof(char)*(send_len));
	if (sendbuf == NULL) {
		perror("calloc\n");

		return FAIL;
	}
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
	printf("send data to socket server is: %s\n", sendbuf);

	/* send over 1024 bytes */
	while ((send_len-send_index) > MAX_BUF) {
		if (send(sock->fd, (sendbuf+send_index), MAX_BUF, 0) != MAX_BUF) {
			perror("send");
			free(sendbuf);
			sendbuf = NULL;
			/* set socket status: disconnected */
			sock->connect_status = 0;

			return FAIL;
		}
		send_index += MAX_BUF;
	}

	/* send normal (low) 1024 bytes */
	if (send(sock->fd, (sendbuf+send_index), (send_len-send_index), 0) != (send_len-send_index)) {
		perror("send");
		/* set socket status: disconnected */
		sock->connect_status = 0;
		free(sendbuf);
		sendbuf = NULL;

		return FAIL;
	}
	free(sendbuf);
	sendbuf = NULL;

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
	cJSON *newitem = NULL;
	cJSON *root_json = NULL;
	char recvbuf[MAX_BUF] = {0};
	char **array = NULL;
	int recv_len = 0;
	char *msg_content = NULL;
	int i = 0;
	int size_package = 0;
	int size_content = 0;
	char **msg_array = NULL;
	char frame[STATE_SIZE] = {0};//提取出一帧, 存放buf
	char sec_buf_memory[BUFFSIZE] = {0};
	GRIPPERS_CONFIG_INFO grippers_config_info;

	recv_len = recv(sock->fd, recvbuf, MAX_BUF, 0);
	//printf("recv_len = %d\n", recv_len);
	if (recv_len <= 0) {
		printf("sock->fd = %d\n", sock->fd);
		printf("sock cmd/file recv_len : %d\n", recv_len);
		/* 认为连接已经断开 */
		printf("sock cmd/file errno : %d\n", errno);
		printf("sock cmd/file strerror : %s\n", strerror(errno));
		perror("sock cmd/file perror recv :");
		/* set socket status: disconnected */
		sock->connect_status = 0;

		return FAIL;
	}

	//if (atoi(array[2]) != 401) {
		printf("recv data from socket server is: %s\n", recvbuf);
	//}
	// 如果收到的数据包长度加上已有 buf_memory 长度已经超过 buf_memory 定义空间大小(BUFFSIZE), 清空 buf_memory
	if ((strlen(buf_memory)+recv_len) > BUFFSIZE) {
		bzero(buf_memory, BUFFSIZE);
	}
	memcpy((buf_memory+strlen(buf_memory)), recvbuf, recv_len);

	/* 对于"粘包"，"断包"进行处理 */
	while (socket_pkg_handle(buf_memory, sec_buf_memory, frame, BUFFSIZE, STATE_SIZE) != 0) {
		//printf("frame is : %s\n", frame);
		/* 把接收到的包按照分割符"III"进行分割 */
		if (string_to_string_list(frame, "III", &size_package, &array) == 0 || size_package != 6) {
			perror("string to string list");
			string_list_free(array, size_package);

			continue;
		}
		/* 遍历整个队列, 更改相关结点信息 */
		/* 创建结点 */
		QElemType node;
		//printf("array[2] = %s\n", array[2]);
		//printf("array[4] = %s\n", array[4]);
		if (atoi(array[2]) == 229) {//反馈夹爪配置信息
			bzero(&grippers_config_info, sizeof(GRIPPERS_CONFIG_INFO));
			StringToBytes(array[4], (BYTE *)&grippers_config_info, sizeof(GRIPPERS_CONFIG_INFO));
			root_json = cJSON_CreateArray();
			for(i = 0; i < MAXGRIPPER; i++) {
				newitem = cJSON_CreateObject();
				cJSON_AddNumberToObject(newitem, "id", (i+1));
				cJSON_AddNumberToObject(newitem, "name", grippers_config_info.id_company[i]);
				cJSON_AddNumberToObject(newitem, "type", grippers_config_info.id_device[i]);
				cJSON_AddNumberToObject(newitem, "version", grippers_config_info.id_softversion[i]);
				cJSON_AddNumberToObject(newitem, "position", grippers_config_info.id_bus[i]);
				cJSON_AddItemToArray(root_json, newitem);
			}
			msg_content = cJSON_Print(root_json);
			cJSON_Delete(root_json);
			root_json = NULL;
			if (createnode(&node, atoi(array[2]), msg_content) == FAIL) {
				string_list_free(array, size_package);
				if (msg_content != NULL) {
					free(msg_content);
					msg_content = NULL;
				}

				continue;
			}
			//printf("msg_content = %s\n", msg_content);
			if (msg_content != NULL) {
				free(msg_content);
				msg_content = NULL;
			}
		} else if (atoi(array[2]) == 320 || atoi(array[2]) == 314 || atoi(array[2]) == 327 || atoi(array[2]) == 329 || atoi(array[2]) == 262 || atoi(array[2]) == 250 || atoi(array[2]) == 272 || atoi(array[2]) == 274 || atoi(array[2]) == 277 || atoi(array[2]) == 289 || atoi(array[2]) == 390) {// 计算TCF, 计算工具坐标系, 计算外部TCF, 计算工具TCF, 计算传感器位姿, 计算摆焊坐标系, 十点法计算传感器位姿, 八点法计算激光跟踪传感器位姿， 三点法计算跟踪传感器位姿，四点法外部轴坐标TCP计算,变位机坐标系计算
			cJSON *root_json = cJSON_CreateObject();
			if (string_to_string_list(array[4], ",", &size_content, &msg_array) == 0 || size_content != 6) {
				perror("string to string list");
				//printf("size_content = %d\n", size_content);
				string_list_free(msg_array, size_content);
				string_list_free(array, size_package);

				continue;
			}

			cJSON_AddStringToObject(root_json, "x", msg_array[0]);
			cJSON_AddStringToObject(root_json, "y", msg_array[1]);
			cJSON_AddStringToObject(root_json, "z", msg_array[2]);
			cJSON_AddStringToObject(root_json, "rx", msg_array[3]);
			cJSON_AddStringToObject(root_json, "ry", msg_array[4]);
			cJSON_AddStringToObject(root_json, "rz", msg_array[5]);
			msg_content = cJSON_Print(root_json);
			cJSON_Delete(root_json);
			root_json = NULL;
			//printf("msg_content = %s\n", msg_content);
			if (createnode(&node, atoi(array[2]), msg_content) == FAIL) {
				string_list_free(msg_array, size_content);
				string_list_free(array, size_package);
				if (msg_content != NULL) {
					free(msg_content);
					msg_content = NULL;
				}

				continue;
			}
			if (msg_content != NULL) {
				free(msg_content);
				msg_content = NULL;
			}
			string_list_free(msg_array, size_content);
		} else if (atoi(array[2]) == 278) {//反馈激光传感器记录点内
			if (string_to_string_list(array[4], ",", &size_content, &msg_array) == 0 || size_content != 16) {
				perror("string to string list");
				printf("size_content = %d\n", size_content);
				string_list_free(msg_array, size_content);
				string_list_free(array, size_package);

				continue;
			}
			//printf("size_content = %d\n", size_content);
			root_json = cJSON_CreateObject();
			cJSON_AddStringToObject(root_json, "j1", msg_array[0]);
			cJSON_AddStringToObject(root_json, "j2", msg_array[1]);
			cJSON_AddStringToObject(root_json, "j3", msg_array[2]);
			cJSON_AddStringToObject(root_json, "j4", msg_array[3]);
			cJSON_AddStringToObject(root_json, "j5", msg_array[4]);
			cJSON_AddStringToObject(root_json, "j6", msg_array[5]);
			cJSON_AddStringToObject(root_json, "x", msg_array[6]);
			cJSON_AddStringToObject(root_json, "y", msg_array[7]);
			cJSON_AddStringToObject(root_json, "z", msg_array[8]);
			cJSON_AddStringToObject(root_json, "rx", msg_array[9]);
			cJSON_AddStringToObject(root_json, "ry", msg_array[10]);
			cJSON_AddStringToObject(root_json, "rz", msg_array[11]);
			cJSON_AddStringToObject(root_json, "E1", msg_array[12]);
			cJSON_AddStringToObject(root_json, "E2", msg_array[13]);
			cJSON_AddStringToObject(root_json, "E3", msg_array[14]);
			cJSON_AddStringToObject(root_json, "E4", msg_array[15]);
			msg_content = cJSON_Print(root_json);
			cJSON_Delete(root_json);
			root_json = NULL;
			if (createnode(&node, atoi(array[2]), msg_content) == FAIL) {
				string_list_free(msg_array, size_content);
				string_list_free(array, size_package);
				if (msg_content != NULL) {
					free(msg_content);
					msg_content = NULL;
				}

				continue;
			}
			if (msg_content != NULL) {
				free(msg_content);
				msg_content = NULL;
			}
			string_list_free(msg_array, size_content);
		} else if (atoi(array[2]) == 325) {//计算 TCF to Joint
			if (string_to_string_list(array[4], ",", &size_content, &msg_array) == 0 || size_content != 6) {
				perror("string to string list");
				printf("size_content = %d\n", size_content);
				string_list_free(msg_array, size_content);
				string_list_free(array, size_package);

				continue;
			}
			//printf("size_content = %d\n", size_content);
			root_json = cJSON_CreateObject();
			cJSON_AddStringToObject(root_json, "j1", msg_array[0]);
			cJSON_AddStringToObject(root_json, "j2", msg_array[1]);
			cJSON_AddStringToObject(root_json, "j3", msg_array[2]);
			cJSON_AddStringToObject(root_json, "j4", msg_array[3]);
			cJSON_AddStringToObject(root_json, "j5", msg_array[4]);
			cJSON_AddStringToObject(root_json, "j6", msg_array[5]);
			msg_content = cJSON_Print(root_json);
			cJSON_Delete(root_json);
			root_json = NULL;
			if (createnode(&node, atoi(array[2]), msg_content) == FAIL) {
				string_list_free(msg_array, size_content);
				string_list_free(array, size_package);
				if (msg_content != NULL) {
					free(msg_content);
					msg_content = NULL;
				}

				continue;
			}
			if (msg_content != NULL) {
				free(msg_content);
				msg_content = NULL;
			}
			string_list_free(msg_array, size_content);
		} else if (atoi(array[2]) == 283) {//获取激光跟踪传感器配置信息
			root_json = cJSON_CreateObject();
			if (string_to_string_list(array[4], ",", &size_content, &msg_array) == 0 || size_content != 4) {
				perror("string to string list");
				//printf("size_content = %d\n", size_content);
				string_list_free(msg_array, size_content);
				string_list_free(array, size_package);

				continue;
			}

			cJSON_AddStringToObject(root_json, "ip", msg_array[0]);
			cJSON_AddStringToObject(root_json, "port", msg_array[1]);
			cJSON_AddStringToObject(root_json, "period", msg_array[2]);
			cJSON_AddStringToObject(root_json, "protocol_id", msg_array[3]);
			msg_content = cJSON_Print(root_json);
			cJSON_Delete(root_json);
			root_json = NULL;
			if (createnode(&node, atoi(array[2]), msg_content) == FAIL) {
				string_list_free(msg_array, size_content);
				string_list_free(array, size_package);
				if (msg_content != NULL) {
					free(msg_content);
					msg_content = NULL;
				}

				continue;
			}
			if (msg_content != NULL) {
				free(msg_content);
				msg_content = NULL;
			}
			string_list_free(msg_array, size_content);
		} else if (atoi(array[2]) == 358) {//获取负载信息
			root_json = cJSON_CreateObject();
			if (string_to_string_list(array[4], ",", &size_content, &msg_array) == 0 || size_content != 4) {
				perror("string to string list");
				//printf("size_content = %d\n", size_content);
				string_list_free(msg_array, size_content);
				string_list_free(array, size_package);

				continue;
			}

			cJSON_AddStringToObject(root_json, "weight", msg_array[0]);
			cJSON_AddStringToObject(root_json, "x", msg_array[1]);
			cJSON_AddStringToObject(root_json, "y", msg_array[2]);
			cJSON_AddStringToObject(root_json, "z", msg_array[3]);
			msg_content = cJSON_Print(root_json);
			cJSON_Delete(root_json);
			root_json = NULL;
			if (createnode(&node, atoi(array[2]), msg_content) == FAIL) {
				string_list_free(msg_array, size_content);
				string_list_free(array, size_package);
				if (msg_content != NULL) {
					free(msg_content);
					msg_content = NULL;
				}

				continue;
			}
			if (msg_content != NULL) {
				free(msg_content);
				msg_content = NULL;
			}
			string_list_free(msg_array, size_content);
		} else if (atoi(array[2]) == 345) {//检测导入的机器人配置文件并生效
			//printf("robot cfg : array[4] = %s\n", array[4]);
			if (strcmp(array[4], "0") == 0) {
				//printf("fail！\n");
			}
			if (strcmp(array[4], "1") == 0) {
				//printf("success！\n");
				char cmd[128] = {0};
				sprintf(cmd, "cp %s %s", WEB_ROBOT_CFG, ROBOT_CFG);
				system(cmd);

				/**
					发送 set rebot type 指令,
					确保 robot type 正确，
					特别在恢复出厂值时，需要发送此指令
				*/
				if (send_cmd_set_robot_type() == FAIL) {
					perror("send cmd set robot type!");

					return FAIL;
				}
			}
			if (createnode(&node, atoi(array[2]), array[4]) == FAIL) {
				string_list_free(array, size_package);

				continue;
			}
		} else if (atoi(array[2]) == 380) {//修改示教点数据
			root_json = cJSON_CreateObject();
			if (string_to_string_list(array[4], ",", &size_content, &msg_array) == 0 || size_content != 6) {
				perror("string to string list");
				//printf("size_content = %d\n", size_content);
				string_list_free(msg_array, size_content);
				string_list_free(array, size_package);

				continue;
			}

			cJSON_AddStringToObject(root_json, "j1", msg_array[0]);
			cJSON_AddStringToObject(root_json, "j2", msg_array[1]);
			cJSON_AddStringToObject(root_json, "j3", msg_array[2]);
			cJSON_AddStringToObject(root_json, "j4", msg_array[3]);
			cJSON_AddStringToObject(root_json, "j5", msg_array[4]);
			cJSON_AddStringToObject(root_json, "j6", msg_array[5]);
			msg_content = cJSON_Print(root_json);
			cJSON_Delete(root_json);
			root_json = NULL;
			if (createnode(&node, atoi(array[2]), msg_content) == FAIL) {
				string_list_free(msg_array, size_content);
				string_list_free(array, size_package);
				if (msg_content != NULL) {
					free(msg_content);
					msg_content = NULL;
				}

				continue;
			}
			if (msg_content != NULL) {
				free(msg_content);
				msg_content = NULL;
			}
			string_list_free(msg_array, size_content);
		} else if (atoi(array[2]) == 386) {//修改示教点数据
			root_json = cJSON_CreateObject();
			if (string_to_string_list(array[4], ",", &size_content, &msg_array) == 0 || size_content != 3) {
				perror("string to string list");
				//printf("size_content = %d\n", size_content);
				string_list_free(msg_array, size_content);
				string_list_free(array, size_package);

				continue;
			}

			cJSON_AddStringToObject(root_json, "x_offset", msg_array[0]);
			cJSON_AddStringToObject(root_json, "y_offset", msg_array[1]);
			cJSON_AddStringToObject(root_json, "z_offset", msg_array[2]);
			msg_content = cJSON_Print(root_json);
			cJSON_Delete(root_json);
			root_json = NULL;
			if (createnode(&node, atoi(array[2]), msg_content) == FAIL) {
				string_list_free(msg_array, size_content);
				string_list_free(array, size_package);
				if (msg_content != NULL) {
					free(msg_content);
					msg_content = NULL;
				}

				continue;
			}
			if (msg_content != NULL) {
				free(msg_content);
				msg_content = NULL;
			}
			string_list_free(msg_array, size_content);
		} else if (atoi(array[2]) == 393) {//获取外部轴驱动器配置信息
			root_json = cJSON_CreateObject();
			if (string_to_string_list(array[4], ",", &size_content, &msg_array) == 0 || size_content != 3) {
				perror("string to string list");
				//printf("size_content = %d\n", size_content);
				string_list_free(msg_array, size_content);
				string_list_free(array, size_package);

				continue;
			}

			cJSON_AddStringToObject(root_json, "company", msg_array[0]);
			cJSON_AddStringToObject(root_json, "model", msg_array[1]);
			cJSON_AddStringToObject(root_json, "encType", msg_array[2]);
			msg_content = cJSON_Print(root_json);
			cJSON_Delete(root_json);
			root_json = NULL;
			if (createnode(&node, atoi(array[2]), msg_content) == FAIL) {
				string_list_free(msg_array, size_content);
				string_list_free(array, size_package);
				if (msg_content != NULL) {
					free(msg_content);
					msg_content = NULL;
				}

				continue;
			}
			if (msg_content != NULL) {
				free(msg_content);
				msg_content = NULL;
			}
			string_list_free(msg_array, size_content);
		} else if (atoi(array[2]) == 412) {
			root_json = cJSON_CreateObject();
			cJSON_AddNumberToObject(root_json, "enable", (atoi(array[4])-10));
			msg_content = cJSON_Print(root_json);
			cJSON_Delete(root_json);
			root_json = NULL;
			if (createnode(&node, atoi(array[2]), msg_content) == FAIL) {
				string_list_free(array, size_package);
				if (msg_content != NULL) {
					free(msg_content);
					msg_content = NULL;
				}

				continue;
			}
			if (msg_content != NULL) {
				free(msg_content);
				msg_content = NULL;
			}

			/* 开启 socket thread */
			if (atoi(array[4]) == 11 && torquesys.enable == 0) {
				torquesys.enable = 1;

				//printf("before pthread_create\n");
				/* create socket PCI thread */
				if (pthread_create(&torquesys.t_socket_TORQUE_SYS, NULL, (void*)&socket_TORQUE_SYS_thread, (void *)TORQUE_PORT)) {
					perror("pthread_create");
					string_list_free(array, size_package);

					continue;
				}
				//printf("after pthread_create\n");
			}
			/* 关闭 socket thread */
			if (atoi(array[4]) == 10 && torquesys.enable == 1) {
				torquesys.enable = 0;
				/** TODO cancel pthread */
				//pthread_cancel(torquesys.t_socket_TORQUE_SYS);

				//printf("start pthread_join(torquesys.t_socket_TORQUE_SYS, NULL)\n");
				/* 当前线程挂起, 等待创建线程返回，获取该线程的返回值后，当前线程退出 */
				if (pthread_join(torquesys.t_socket_TORQUE_SYS, NULL)) {
					perror("pthread_join");
					string_list_free(array, size_package);

					continue;
				}
				//printf("after pthread_join\n");
			}
		} else if (atoi(array[2]) == 423) {//获取从站硬件版本
			if (string_to_string_list(array[4], ",", &size_content, &msg_array) == 0 || size_content != 8) {
				perror("string to string list");
				//printf("size_content = %d\n", size_content);
				string_list_free(msg_array, size_content);
				string_list_free(array, size_package);

				continue;
			}

			root_json = cJSON_CreateStringArray(msg_array, 8);
			msg_content = cJSON_Print(root_json);
			//printf("msg_content = %s\n", msg_content);
			cJSON_Delete(root_json);
			root_json = NULL;
			if (createnode(&node, atoi(array[2]), msg_content) == FAIL) {
				string_list_free(msg_array, size_content);
				string_list_free(array, size_package);
				if (msg_content != NULL) {
					free(msg_content);
					msg_content = NULL;
				}

				continue;
			}
			if (msg_content != NULL) {
				free(msg_content);
				msg_content = NULL;
			}
			string_list_free(msg_array, size_content);
		} else if (atoi(array[2]) == 424) {//获取从站固件版本
			if (string_to_string_list(array[4], ",", &size_content, &msg_array) == 0 || size_content != 8) {
				perror("string to string list");
				//printf("size_content = %d\n", size_content);
				string_list_free(msg_array, size_content);
				string_list_free(array, size_package);

				continue;
			}

			root_json = cJSON_CreateStringArray(msg_array, 8);
			msg_content = cJSON_Print(root_json);
			//printf("msg_content = %s\n", msg_content);
			cJSON_Delete(root_json);
			root_json = NULL;
			if (createnode(&node, atoi(array[2]), msg_content) == FAIL) {
				string_list_free(msg_array, size_content);
				string_list_free(array, size_package);
				if (msg_content != NULL) {
					free(msg_content);
					msg_content = NULL;
				}

				continue;
			}
			if (msg_content != NULL) {
				free(msg_content);
				msg_content = NULL;
			}
			string_list_free(msg_array, size_content);
		} else {
			if (createnode(&node, atoi(array[2]), array[4]) == FAIL) {
				string_list_free(array, size_package);

				continue;
			}
		}

		/*
		pthread_mutex_lock(&sock->mut);
		if (sock->msghead >= MAX_MSGHEAD) {
			sock->msghead = 1;
		} else {
			sock->msghead++;
		}
		node.msghead = sock->msghead;
		pthread_mutex_unlock(&sock->mut);
		*/
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
		//	p = p->next;
		string_list_free(array, size_package);
	}

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
				//printf("before send:");
				//printquene(sock->quene);
				if (socket_send(sock, &p->data) == FAIL) {
					perror("socket send");
				} else {
				/* 发送成功后删除结点信息 */
					pthread_mutex_lock(&sock->mute);
					dequene(&sock->quene, p->data);
					pthread_mutex_unlock(&sock->mute);
					//printf("after dequene:");
					//printquene(sock->quene);
				}
		//		break; //发送完一个非即时指令后，立刻重新进入 while 循环,看是否有即时指令需要下发
			//}
		}

		//delay(1);
		usleep(1000);
	}
}

static void *socket_recv_thread(void *arg)
{
	SOCKET_INFO *sock;
	sock = (SOCKET_INFO *)arg;
	char buf_memory[BUFFSIZE] = {0};

	while (1) {
		//printf("buf_memory content = %s\n", buf_memory);
		/* socket 连接已经断开 */
		if (sock->connect_status == 0) {

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
			//delay(1000);
			sleep(1);

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

		if (port == CMD_PORT) {
			/* 初始化扭矩系统 */
			init_torquesys();
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
	char buf_memory[BUFFSIZE] = {0};
	char sec_buf_memory[BUFFSIZE] = {0};
	char frame[STATE_SIZE] = {0};//提取出一帧, 存放buf
	char recvbuf[STATE_SIZE] = {0};
	char status_buf[STATE_SIZE] = {0};

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
			//delay(1000);
			sleep(1);

			continue;
		}
		/* socket connected */
		/* set socket status: connected */
		sock->connect_status = 1;
		printf("Socket connect success: sockfd = %d\tserver_ip = %s\t server_port = %d\n", sock->fd, SERVER_IP, port);

		/* recv ctrl status */
		while (1) {
			//printf("sizeof CTRL_STATE = %d\n", sizeof(CTRL_STATE)); /* Now struct is 1221 bytes */
			bzero(recvbuf, STATE_SIZE);
			recv_len = recv(sock->fd, recvbuf, STATE_SIZE, 0);
			//printf("sock status recv_len = %d\n", recv_len);
			/* recv timeout or error */
			if (recv_len <= 0) {
				printf("sock status recv_len : %d\n", recv_len);
				printf("sock status errno : %d\n", errno);
				printf("sock status strerror : %s\n", strerror(errno));
				perror("sock status recv perror :");
				/* 认为连接已经断开 */

				break;
			}
			//recvbuf[recv_len] = '\0';
			//printf("recv len = %d\n", recv_len);
			//printf("strlen(buf_memory) = %d\n", strlen(buf_memory));
			//printf("sizeof(CTRL_STATE)*3+14 = %d\n", sizeof(CTRL_STATE)*3+14);
			//clock_t time_1, time_2, time_3, time_4, time_5;

			//time_1 = clock();
			//printf("time_1, %d\n", time_1);
			//printf("strlen(buf_memory) + recv len = %d\n", (strlen(buf_memory)+recv_len));
			//printf("BUFFSIZE = %d\n", BUFFSIZE);
			// 如果收到的数据包长度加上已有 buf_memory 长度已经超过 buf_memory 定义空间大小(BUFFSIZE), 清空 buf_memory
			if ((strlen(buf_memory)+recv_len) > BUFFSIZE) {
				bzero(buf_memory, BUFFSIZE);
			}

			memcpy((buf_memory+strlen(buf_memory)), recvbuf, recv_len);

			//time_2 = clock();
			//printf("time_2, %d\n", time_2);

			//获取到的一帧长度等于期望长度（结构体长度，包头包尾长度，分隔符等）
			while (socket_pkg_handle(buf_memory, sec_buf_memory, frame, BUFFSIZE, STATE_SIZE) == sizeof(CTRL_STATE)*3+14) {
				//time_3 = clock();
				//printf("time_3, %d\n", time_3);
				//printf("strlen frame = %d\n", strlen(frame));
				bzero(status_buf, STATE_SIZE);
				//strncpy(status_buf, (frame+7), (strlen(frame)-14));
				memcpy(status_buf, (frame+7), (strlen(frame)-14));
				//strrpc(status_buf, "/f/bIII", "");
				//strrpc(status_buf, "III/b/f", "");
				//printf("strlen status_buf = %d\n", strlen(status_buf));
				//printf("recv status_buf = %s\n", status_buf);
				//printf("state = %p\n", state);
				if (strlen(status_buf) == 3*sizeof(CTRL_STATE)) {
					//time_4 = clock();
					//printf("time_4, %d\n", time_4);

					StringToBytes(status_buf, (BYTE *)state, sizeof(CTRL_STATE));

					//time_5 = clock();
					//printf("time_5, %d\n", time_5);
				/*	printf("state->program_state = %d\n", state->program_state);
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
		}
		/* socket disconnected */
		/* close socket */
		close(sock->fd);
		/* set socket status: disconnected */
		sock->connect_status = 0;
	//	free(buf_memory);
	//	buf_memory = NULL;
	}
}

/**
	function : 数据包收包后对于“粘包”，“断包”，“残包”，“错包”的处理

	buf_memory[IN] : 待处理数据缓冲区

	buf_size : 分配的数据缓存区大小

	*buf_memory_len[IN]：buf 中已存储的数据，所占字节长度

	frame[out]：获取到的完整一帧数据包

	frame_size : 分配的一帧数据存储的大小

	return: 如果有完整一帧数据包，返回数据帧实际长度，否则返回 0 for error
 */
static int buf_memory_handle(char *buf_memory, int buf_size, int *buf_memory_len, char *frame, int frame_size)
{
	assert(buf_memory != NULL);

	char *head_str = "/f/b";
	char *tail_str = "/b/f";
	int head = -1;
	int tail = -1;
	int frame_len = 0;   	// 一帧数据的实际长度
	char *ptr_buf = NULL; 	// 内圈循环 buf
	int i;

	/** calloc buf */
	ptr_buf = (char *)calloc(1, sizeof(char)*buf_size);
	if (ptr_buf == NULL) {
		perror("calloc");

		return FAIL;
	}

	/** 如果缓冲区中为空，则可以直接进行下一轮tcp请求 */
	while (*buf_memory_len != 0) {
		memcpy(ptr_buf, buf_memory, *buf_memory_len);
		/** 循环找包头 */
		for (i = 0; i < *buf_memory_len; i++) {
			if (memcmp((ptr_buf + i), head_str, strlen(head_str)) == 0) {
				head = i;
				break;
			}
		}
		/** 循环找包尾 */
		for (i = 1; i < *buf_memory_len; i++) {
			if (memcmp((ptr_buf + i), tail_str, strlen(tail_str)) == 0) {
				tail = i + strlen(tail_str);
				break;
			}
		}
		/** 断包(有头无尾), 即接收到的包不完整，则跳出内圈循环，进入外圈循环，从输入流中继续读取数据 */
		if (head != -1 && tail == -1) {
		//	perror("Broken package");

			break;
		}
		/** 找到了包头包尾，则提取出一帧 */
		if (head != -1 && tail != -1 && head < tail) {
			/* 取出整包数据, save to frame */
			frame_len = tail - head;
			memset(frame, 0, frame_size);
			memcpy(frame, (ptr_buf + head), frame_len);

			/* 清空缓冲区, 并把包尾后的内容推入缓冲区 */
			memset(buf_memory, 0, buf_size);
			*buf_memory_len = *buf_memory_len - tail;
			memcpy(buf_memory, (ptr_buf + tail), *buf_memory_len);

			break;
		}

		/** 残包(只找到包尾,没头)或者错包(没头没尾)的，清空缓冲区，进入外圈循环，从输入流中重新读取数据 */
		if ((head == -1 && tail != -1) || (head == -1 && tail == -1)) {
			perror("Incomplete package!");
			/** 清空缓冲区, 直接跳出内圈循环，到外圈循环里 */
			memset(buf_memory, 0, buf_size);
			*buf_memory_len = 0;

			break;
		}

		/** 包头在包尾后面，则扔掉缓冲区中包头之前的多余字节, 继续内圈循环 */
		if (head != -1 && tail != -1 && head > tail) {
			perror("Error package");
			/* 清空缓冲区, 并把包头后的内容推入缓冲区 */
			memset(buf_memory, 0, buf_size);
			*buf_memory_len = *buf_memory_len - head;
			memcpy(buf_memory, (ptr_buf + head), *buf_memory_len);
		}
	}
	free(ptr_buf);
	ptr_buf = NULL;

	return frame_len;
}

void *socket_TORQUE_SYS_thread(void *arg)
{
	SOCKET_INFO *sock = NULL;
	int port = (int)arg;
	int recv_len = 0;
	char buf_memory[BUFFSIZE] = { 0 }; /** 保存所有的已接收的数据 */
	int buf_memory_len = 0; /** buf 中已存储的数据，所占字节长度 */
	char frame[TORQUE_SYS_SIZE] = { 0 };
	char recvbuf[TORQUE_SYS_SIZE] = { 0 };
	int i = 0;

	sock = &socket_torquesys;
	/* init socket */
	socket_init(sock, port);

	while (torquesys.enable == 1) {
		bzero(&torque_sys_state, sizeof(TORQUE_SYS_STATE));
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
			//delay(1000);
			sleep(1);

			continue;
		}
		/* socket connected */
		/* set socket status: connected */
		sock->connect_status = 1;
		printf("Socket connect success: sockfd = %d\tserver_ip = %s\t server_port = %d\n", sock->fd, SERVER_IP, port);

		memset(buf_memory, 0, BUFFSIZE);
		buf_memory_len = 0;
		while (torquesys.enable == 1) {
			bzero(recvbuf, TORQUE_SYS_SIZE);
			//printf("before recvbuf");
			recv_len = recv(sock->fd, recvbuf, TORQUE_SYS_SIZE, 0);
			/*
			printf("after recvbuf\n");
			printf("sock status recv_len = %d\n", recv_len);
			printf("recvbuf = %s\n", recvbuf);
			*/
			/* recv timeout or error */
			if (recv_len <= 0) {
				printf("sock status recv_len : %d\n", recv_len);
				printf("sock status errno : %d\n", errno);
				printf("sock status strerror : %s\n", strerror(errno));
				perror("sock status recv perror :");
				/* 认为连接已经断开 */

				break;
			}
			/*
			printf("recvbuf = ");
			for (i = 0; i < recv_len; i++) {
				printf("%x ", recvbuf[i]);
			}
			printf("\n");
			*/
			// 如果收到的数据包长度加上已有 buf_memory_len, 已经超过 buf_memory 定义空间大小(BUFFSIZE), 清空 buf_memory
			if ((buf_memory_len + recv_len) > BUFFSIZE) {
				memset(buf_memory, 0, BUFFSIZE);
				buf_memory_len = 0;
			}
			memcpy((buf_memory + buf_memory_len), recvbuf, recv_len);
			buf_memory_len = buf_memory_len + recv_len;
			/** 获取到的一帧长度等于期望长度（结构体长度，包头包尾长度，分隔符等） */
			//printf("sizeof(TORQUE_SYS_STATE)+14 = %d\n", (sizeof(TORQUE_SYS_STATE)+14)); /* Now struct is 30 bytes, +14 = 54 */
			while (buf_memory_handle(buf_memory, BUFFSIZE, &buf_memory_len, frame, TORQUE_SYS_SIZE) == (sizeof(TORQUE_SYS_STATE) + 14)) {
				/*printf("frame = ");
				for (i = 0; i < recv_len; i++) {
					printf("%x ", recvbuf[i]);
				}
				printf("\n");*/
				bzero(&torque_sys_state, sizeof(TORQUE_SYS_STATE));
				memcpy(&torque_sys_state, (recvbuf + 7), (recv_len - 14));
				//printf("torque_sys_state.task_runtime = %d\n", torque_sys_state.task_runtime);
				//printf("torque_sys_state.feed_turns = %.2lf\n", torque_sys_state.feed_turns);
			}
		}
		/* socket disconnected */
		/* close socket */
		close(sock->fd);
		/* set socket status: disconnected */
		sock->connect_status = 0;
	}
}

/** 状态查询线程 */
void *socket_state_feedback_thread(void *arg)
{
	SOCKET_INFO *sock = NULL;
	char state_buf[STATEFB_SIZE] = {0};
	char recv_buf[STATEFB_SIZE] = {0};
	//char *pkg_content = NULL;
	char pkg_content[STATEFB_SIZE] = {0};
/*	char *file_content = NULL;
	char *write_content = NULL;*/
	char *tmp_buf = NULL;
	int pkg_len[100] = {0};
	int i;
	int j;
	//int num = 0;
	int port = (int)arg;
	int size = 0;
	FILE *fp = NULL;
	int linenum = 0;
	char strline[LINE_LEN] = {0};
	char *buf_memory = NULL;
	char *sec_buf_memory = NULL;
//	char *frame = NULL;//提取出一帧, 存放buf
	char frame[STATEFB_SIZE] = {0};
	int recv_len = 0;
	char **array = NULL;
	int varnum = 0;
	int varnum_len = 0;
	printf("port = %d\n", port);

/*	file_content = (char *)calloc(1, sizeof(char)*(STATEFB_FILESIZE+100));
	write_content = (char *)calloc(1, sizeof(char)*(STATEFB_WRITESIZE+100));*/
	sock = &socket_state;
	/* init socket */
	socket_init(sock, port);
	/* init FB quene */
	fb_initquene(&fb_quene);
	/* init FB struct */
	state_feedback_init(&state_fb);
	state_fb.buf = (char *)calloc(1, sizeof(char)*(STATEFB_BUFSIZE+1));

	while(1) {
		/* calloc buf */
		buf_memory = (char *)calloc(1, sizeof(char)*2*STATEFB_SIZE);
		if (buf_memory == NULL) {
			perror("calloc");

			continue;
		}
		/* calloc buf */
		sec_buf_memory = (char *)calloc(1, sizeof(char)*2*STATEFB_SIZE);
		if (sec_buf_memory == NULL) {
			perror("calloc");

			continue;
		}
	/*	frame = (char *)calloc(1, sizeof(char)*STATEFB_SIZE);
		if (frame == NULL) {
			perror("calloc");

			continue;
		}*/
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
			sleep(1);

			continue;
		}
		pthread_mutex_init(&sock->mute, NULL);
		/* socket connected */
		/* set socket status: connected */
		sock->connect_status = 1;
		printf("Socket connect success: sockfd = %d\tserver_ip = %s\t server_port = %d\n", sock->fd, SERVER_IP, port);

		/* recv ctrl status */
		while (1) {
			bzero(recv_buf, sizeof(char)*(STATEFB_SIZE));

			//printf("sizeof CTRL_STATE = %d\n", sizeof(CTRL_STATE)); /* Now struct is bytes */
			recv_len = recv(sock->fd, recv_buf, STATEFB_SIZE, 0); /* Now recv buf is 24026 bytes*/
			//printf("state feedback recv_len = %d\n", recv_len);
			/* recv timeout or error */
			if (recv_len <= 0) {
				printf("state feedback recv_len = %d\n", recv_len);
				printf("state feedback errno : %d\n", errno);
				printf("state feedback strerror : %s\n", strerror(errno));
				perror("state feedback perror recv :");

				break;
			}

			//clock_t time_1, time_2, time_3, time_4, time_5;

			//time_1 = clock();
			//printf("time_1, %d\n", time_1);
			//recv_buf[recv_len] = '\0';
			// 如果收到的数据包长度加上已有 buf_memory 长度已经超过 buf_memory 定义空间大小(STAFB_BUFFSIZE), 清空 buf_memory
			if ((strlen(buf_memory)+recv_len) > 2*STATEFB_SIZE) {
				bzero(buf_memory, 2*STATEFB_SIZE);
			}
			memcpy((buf_memory+strlen(buf_memory)), recv_buf, recv_len);

			//printf("recv len = %d\n", recv_len);
			//printf("recv buf = %s\n", recv_buf);
			/**
				获取到的一帧长度等于期望长度（/f/bIII变量个数III数据长度III数据区III/b/f）
				变量个数长度：varnum_len
				数据长度：5
				数据区：sizeof(STATE_FB)
			*/
			varnum = state_fb.icount;
			varnum_len = 0;
			while (varnum) {
				varnum/=10;
				varnum_len++;
			}
			//printf("varnum_len = %d\n", varnum_len);
			//printf("7+varnum_len+3+5+3+sizeof(STATE_FB)*3+7 = %d\n", 7+varnum_len+3+5+3+sizeof(STATE_FB)*3+7);
			while (socket_pkg_handle(buf_memory, sec_buf_memory, frame, 2*STATEFB_SIZE, STATEFB_SIZE) == (7+varnum_len+3+5+3+sizeof(STATE_FB)*3+7)) {
				/*if (string_to_string_list(frame, "III", &size, &array) == 0 || size != 5) {
					perror("string to string list");
					string_list_free(array, size);

					continue;
				}*/
				bzero(state_buf, STATEFB_SIZE);
				//strncpy(state_buf, (frame+7+varnum_len+3+5+3), sizeof(STATE_FB)*3);
				memcpy(state_buf, (frame+7+varnum_len+3+5+3), sizeof(STATE_FB)*3);
				/*printf("array[0]=%s\n", array[0]);
				printf("array[1]=%s\n", array[1]);
				printf("array[2]=%s\n", array[2]);
				printf("array[3]=%s\n", array[3]);
				printf("array[4]=%s\n", array[4]);*/
				//printf("strlen(array[3]) = %d\n", strlen(array[3]));
				//printf("3*sizeof(STATE_FB) = %d\n", 3*sizeof(STATE_FB));
				//if (strlen(array[3]) == 3*sizeof(STATE_FB)) {
				//time_2 = clock();
				//printf("time_2, %d\n", time_2);
				if (strlen(state_buf) == 3*sizeof(STATE_FB)) {
					STATE_FB sta_fb;
					fb_createnode(&sta_fb);
					//time_3 = clock();
					//printf("time_3, %d\n", time_3);
					//bzero(state, sizeof(STATE_FB));
					//StringToBytes(array[3], (BYTE *)&sta_fb, sizeof(STATE_FB));
					StringToBytes(state_buf, (BYTE *)&sta_fb, sizeof(STATE_FB));
					//time_4 = clock();
					//printf("time_4, %d\n", time_4);
					//printf("enter/if\n");
					if (state_fb.type == 0) { //"0":图表查询
						if (fb_get_node_num(fb_quene) >= STATEFB_MAX) {
							state_fb.overflow = 1;
							/** clear state quene */
							pthread_mutex_lock(&sock->mute);
							fb_clearquene(&fb_quene);
							pthread_mutex_unlock(&sock->mute);
							/** send stop vardata_feedback to TaskManagement */
							socket_enquene(&socket_cmd, 231, "SetCTLStateQuery(0)", 1);
						} else {
							state_fb.overflow = 0;
						}
						/** enquene node */
						pthread_mutex_lock(&sock->mute);
						fb_enquene(&fb_quene, sta_fb);
						pthread_mutex_unlock(&sock->mute);
						//time_5 = clock();
						//printf("time_5, %d\n", time_5);
					} else if (state_fb.type == 1) {// "1":轨迹数据查询
						memset(pkg_content, 0, STATEFB_SIZE);
						for (i = 0; i < STATEFB_PERPKG_NUM; i++) {
							for (j = 0; j < 7; j ++) {
								if (j < 6) {
									sprintf(pkg_content, "%s%f,", pkg_content, sta_fb.fb[i][j]);
								} else {
									sprintf(pkg_content, "%s%f\n", pkg_content, sta_fb.fb[i][j]);
								}
							}
						}
						write_file_append(FILE_STATEFB, pkg_content);
					} else if (state_fb.type == 2 || state_fb.type == 3) { //"2":查询 10 秒内固定格式机器人数据 "3":查询 10 秒内部分选择的机器人数据
						/** 重新开始计包个数时，清空缓冲区 */
						if (state_fb.index == 0) {
							bzero(state_fb.buf, sizeof(char)*(STATEFB_BUFSIZE+1));
						}
						//time_2 = clock();
						//printf("time_2, %d\n", time_2);
						memset(pkg_content, 0, STATEFB_SIZE);
						for (i = 0; i < STATEFB_PERPKG_NUM; i++) {
							for (j = 0; j < 15; j++) {
								if (j < 14) {
									sprintf(pkg_content, "%s%f,", pkg_content, sta_fb.fb[i][j]);
								} else {
									sprintf(pkg_content, "%s%f\n", pkg_content, sta_fb.fb[i][j]);
								}
							}
						}
						//time_3 = clock();
						//printf("time_3, %d\n", time_3);
						//printf("end print state fb\n");
						if (state_fb.index >= 100) { /** 收到包超过 100 个，即超过 10 秒 */
							tmp_buf = (char *)calloc(1, sizeof(char)*(STATEFB_BUFSIZE+1));
							//strcpy(tmp_buf, state_fb.buf+STATEFB_SIZE);
							strcpy(tmp_buf, state_fb.buf+pkg_len[state_fb.index%100]);
							bzero(state_fb.buf, sizeof(char)*(STATEFB_BUFSIZE+1));
							strcpy(state_fb.buf, tmp_buf);
							free(tmp_buf);
							tmp_buf = NULL;
						}
						pkg_len[state_fb.index%100] = strlen(pkg_content);
						//printf("strlen(pkg_content) = %d\n", strlen(pkg_content));

						//time_4 = clock();
						//printf("time_4, %d\n", time_4);
						strcat(state_fb.buf, pkg_content);
						//free(pkg_content);
						//pkg_content = NULL;
						printf("state_fb.buf strlen = %d\n", strlen(state_fb.buf));
						printf("state_fb.index = %d\n", state_fb.index);
						state_fb.index++;
						//time_5 = clock();
						//printf("time_5, %d\n", time_5);

						// 重新开始计包个数时，清空二级缓冲区
						/*if (state_fb.index == 0) {
							bzero(write_content, sizeof(char)*(STATEFB_WRITESIZE+100));
						}

						strcat(write_content, pkg_content);
						state_fb.index++;
						printf("index = %d\n", state_fb.index);
						if (state_fb.index%10 == 0) {
							printf("strlen(write_content) = %d\n", strlen(write_content));
							// check line num is over 10000 or not
							bzero(file_content, sizeof(char)*(STATEFB_FILESIZE+100));
							linenum = 0;
							if ((fp = fopen(FILE_STATEFB, "r")) == NULL) {
								perror("file statefb : open file");

								continue;
							}
							//rewind(fp);
							//fseek(fp, strlen(write_content), SEEK_SET);
							while (fgets(strline, LINE_LEN, fp) != NULL) {
								linenum++;
								if (linenum > STATEFB_PERPKG_NUM*10) {
									file_content = strcat(file_content, strline);
								}
							}
							fclose(fp);
							//printf("read file: file_content = %s\n", file_content);
							printf("linenum = %d\n", linenum);
							if (linenum >= 10000) {
								if (write_file(FILE_STATEFB, file_content) == FAIL) {
									perror("write file content");
								}
							}

							//printf("write_content = %s\n", write_content);
							write_file_append(FILE_STATEFB, write_content);
							bzero(write_content, sizeof(char)*(STATEFB_WRITESIZE+100));
						}*/


						/** 重新开始计包个数时，清空缓冲区 */
						/*if (state_fb.index == 0) {
							bzero(state_fb.buf, sizeof(char)*(STATEFB_BUFSIZE+1));
						}
						//printf("end print state fb\n");
						if (state_fb.index >= 100) { // 收到包超过 100 个，即超过 10 秒
							tmp_buf = (char *)calloc(1, sizeof(char)*(STATEFB_BUFSIZE+1));
							strcpy(tmp_buf, state_fb.buf+STATEFB_SIZE);
							bzero(state_fb.buf, sizeof(char)*(STATEFB_BUFSIZE+1));
							strcpy(state_fb.buf, tmp_buf);
							free(tmp_buf);
							tmp_buf = NULL;
						}
						strcat(state_fb.buf, array[3]);
					//	printf("state_fb.buf strlen = %d\n", strlen(state_fb.buf));
						printf("state_fb.index = %d\n", state_fb.index);
						state_fb.index++;*/
					}
				}
				string_list_free(array, size);
				//printf("after StringToBytes\n");
			}
		}
		/* socket disconnected */
		/* 释放互斥锁 */
		pthread_mutex_destroy(&sock->mute);
		/* close socket */
		close(sock->fd);
		/* set socket status: disconnected */
		sock->connect_status = 0;
		free(sec_buf_memory);
		sec_buf_memory = NULL;
		free(buf_memory);
		buf_memory = NULL;
	}
/*	free(write_content);
	write_content = NULL;*/
	free(state_fb.buf);
	state_fb.buf = NULL;
}

/* socket bind/listen */
static int socket_bind_listen(SOCKET_SERVER_INFO *sock)
{
	//描述服务器的socket
	struct sockaddr_in serverAddr;

	printf("sock->server_port = %d\n", sock->server_port);
	printf("sock->server_ip = %s\n",sock->server_ip);

	memset(&serverAddr, 0, sizeof(struct sockaddr_in));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(sock->server_port);
	//inet_addr()函数，将点分十进制IP转换成网络字节序IP
	serverAddr.sin_addr.s_addr = inet_addr(sock->server_ip);

	if (bind(sock->serv_fd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
		printf("bind socket error: %s(errno: %d)\n", strerror(errno), errno);

		return FAIL;
	}

	if (listen(sock->serv_fd, 10) == -1) {
		printf("listen socket error: %s(errno: %d)\n", strerror(errno), errno);

		return FAIL;
	}

	return SUCCESS;
}

/* socket_upper_computer_send */
static int socket_upper_computer_send(SOCKET_SERVER_INFO *sock, const int cmd_id, const int data_len, const char *data_content)
{
	char *sendbuf = NULL;
	int send_len = 0;

	if (sock->msghead >= MAX_MSGHEAD) {
		sock->msghead = 1;
	} else {
		sock->msghead++;
	}
	send_len = 4 + 3 + get_n_len(sock->msghead) + 3 + get_n_len(cmd_id) + 3 + get_n_len(data_len) + 3 + data_len + 3 + 4 + 1; // +1 to save '\0
	sendbuf = (char *)calloc(1, sizeof(char)*(send_len));
	if (sendbuf == NULL) {
		perror("calloc\n");

		return FAIL;
	}
	/* sprintf 会在 sendbuf 最后自动添加一个 '\0' 作为字符串结束的标识符 */
	sprintf(sendbuf, "/f/bIII%dIII%dIII%dIII%sIII/b/f", sock->msghead, cmd_id, data_len, data_content);

	printf("send_len = %d\n", send_len);
	printf("send data to socket server is: %s\n", sendbuf);

	/* send normal (low) 1024 bytes */
	if (send(sock->clnt_fd, sendbuf, send_len, 0) != send_len) {
		perror("send");
		/* set socket status: disconnected */
		sock->connect_status = 0;
		free(sendbuf);
		sendbuf = NULL;

		return FAIL;
	}
	free(sendbuf);
	sendbuf = NULL;

	return SUCCESS;
}

static void *socket_upper_computer_recv_send(SOCKET_SERVER_INFO *sock)
{
	char buf_memory[BUFFSIZE] = {0};
	char sec_buf_memory[BUFFSIZE] = {0};
	char recvbuf[MAX_BUF] = {0};
	char **array = NULL;
	int recv_len = 0;
	char *msg_content = NULL;
	int size_package = 0;
	char frame[BUFFSIZE] = {0};//提取出一帧, 存放buf
	int check_result = 0;
	char data_content[1024] = "";

	while (1) {
		//printf("buf_memory content = %s\n", buf_memory);
		/* socket 连接已经断开 */
		if (sock->connect_status == 0) {

			break;
		}
		recv_len = recv(sock->clnt_fd, recvbuf, MAX_BUF, 0);
		//printf("recv_len = %d\n", recv_len);
		if (recv_len <= 0) {
			printf("sock->clnt_fd = %d\n", sock->clnt_fd);
			printf("sock cmd/file recv_len : %d\n", recv_len);
			/* 认为连接已经断开 */
			printf("sock cmd/file errno : %d\n", errno);
			printf("sock cmd/file strerror : %s\n", strerror(errno));
			perror("sock cmd/file perror recv :");
			/* set socket status: disconnected */
			sock->connect_status = 0;

			continue;
		}

		printf("recv data from socket client is: %s\n", recvbuf);
		// 如果收到的数据包长度加上已有 buf_memory 长度已经超过 buf_memory 定义空间大小(BUFFSIZE 8192), 清空 buf_memory
		if ((strlen(buf_memory)+recv_len) > BUFFSIZE) {
			bzero(buf_memory, BUFFSIZE);
		}
		memcpy((buf_memory+strlen(buf_memory)), recvbuf, recv_len);

		/* 对于"粘包"，"断包"进行处理 */
		while (socket_pkg_handle(buf_memory, sec_buf_memory, frame, BUFFSIZE, BUFFSIZE) != 0) {
			//printf("frame is : %s\n", frame);
			/* 把接收到的包按照分割符"III"进行分割 */
			if (string_to_string_list(frame, "III", &size_package, &array) == 0 || size_package != 6) {
				perror("string to string list");
				string_list_free(array, size_package);

				continue;
			}
			/** 接收文件名 */
			if (atoi(array[2]) == 105) {
				socket_enquene(&socket_file, 105, array[4], 1);
				bzero(lua_filename, FILENAME_SIZE);
				strcpy(lua_filename, array[4]);
				strcpy(data_content, "1");
			/** 接收文件内容 */
			} else if (atoi(array[2]) == 106) {
				if (write_file(lua_filename, array[4]) == FAIL) {
					perror("write file");

					continue;
				}

				/** 检查 lua 文件内容合法性 */
				check_result = check_lua_file();
				//printf("check_result = %d\n", check_result);
				if (check_result == SUCCESS) {
					strcpy(data_content, "SUCCESS");
				} else {
					strcpy(data_content, error_info);
				}
			/** START */
			} else if (atoi(array[2]) == 101) {
				socket_enquene(&socket_cmd, 101, "START", 0);
				strcpy(data_content, "1");
			/** STOP */
			} else if (atoi(array[2]) == 102) {
				socket_enquene(&socket_cmd, 102, "STOP", 0);
				strcpy(data_content, "1");
			/** PAUSE */
			} else if (atoi(array[2]) == 103) {
				socket_enquene(&socket_cmd, 103, "PAUSE", 0);
				strcpy(data_content, "1");
			/** PESUME */
			} else if (atoi(array[2]) == 104) {
				socket_enquene(&socket_cmd, 104, "RESUME", 0);
				strcpy(data_content, "1");
			}
			/* send recv info to upper computer */
			socket_upper_computer_send(sock, atoi(array[2]), strlen(data_content), data_content);
			string_list_free(array, size_package);
		}
	}
}

void *socket_upper_computer_thread(void* arg)
{
	SOCKET_SERVER_INFO* sock = NULL;
	int port = (int)arg;
	printf("port = %d\n", port);

	sock = &socket_upper_computer;

	while (1) {
		/* init socket */
		socket_server_init(sock, port);
		/* do socket connect */
		/* create socket */
		if (socket_server_create(sock) == FAIL) {
			/* create fail */
			perror("socket create fail");

			continue;
		}
		/* socket bind listen */
		if (socket_bind_listen(sock) == FAIL) {
			perror("socket bind listen fail");

			sleep(1);
			continue;
		}
		printf("====== waiting for client's request ======\n");
		while (1) {
			//接收客户端请求
			struct sockaddr_in clnt_addr;
			socklen_t clnt_addr_size = sizeof(clnt_addr);
			/* socket accepted */
			//if ((sock->clnt_fd = accept(sock->serv_fd, (struct sockaddr*)NULL, NULL)) == -1) {
			if ((sock->clnt_fd = accept(sock->serv_fd, (struct sockaddr*)&clnt_addr, &clnt_addr_size)) == -1) {
				printf("accept socket error: %s(errno: %d)", strerror(errno), errno);

				continue;
			}
			/* set socket status: connected */
			sock->connect_status = 1;
			printf("Client online\n");

			socket_upper_computer_recv_send(sock);

			/* close clnt socket fd */
			close(sock->clnt_fd);
			/* set socket status: disconnected */
			sock->connect_status = 0;
		}
		/* close serv socket fd */
		close(sock->serv_fd);
	}
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
	if (createnode(&node, type, send_content) == FAIL) {

		return FAIL;
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

static void init_torquesys()
{
	/** init torquesys on off status */
	torquesys.enable = 0;
	/** send get_on_off to TaskManagement */
	socket_enquene(&socket_cmd, 412, "TorqueSysGetOnOff()", 1);
}

int send_cmd_set_robot_type()
{
	cJSON *type = NULL;
	cJSON *major_ver = NULL;
	cJSON *minor_ver = NULL;
	cJSON *content_json = NULL;
	char *file_content = NULL;
	char cmd_content[100] = "";
	int robot_type = 0;

	file_content = get_file_content(FILE_ROBOT_TYPE);
	/* content_json is NULL */
	if (file_content == NULL || strcmp(file_content, "NO_FILE") == 0 || strcmp(file_content, "Empty") == 0) {
		perror("get file content");

		return FAIL;
	}
	content_json = cJSON_Parse(file_content);
	free(file_content);
	file_content = NULL;

	type = cJSON_GetObjectItem(content_json, "type");
	major_ver = cJSON_GetObjectItem(content_json, "major_ver");
	minor_ver = cJSON_GetObjectItem(content_json, "minor_ver");
	if (type == NULL || major_ver == NULL || minor_ver == NULL) {
		perror("json");

		return FAIL;
	}
	/** 主版本号预留 10 个 (1~10)，次版本号预留 10 个 (0~9) */
	robot_type = (type->valueint - 1) * 100 + (major_ver->valueint - 1) * 10 + (minor_ver->valueint + 1);
	sprintf(cmd_content, "SetRobotType(%d)", robot_type);
	socket_enquene(&socket_cmd, 425, cmd_content, 1);

	return SUCCESS;
}

