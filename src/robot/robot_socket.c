
/********************************* Includes ***********************************/

#include 	"goahead.h"
#include	"cJSON.h"
#include 	"tools.h"
#include 	"check_lua_file.h"
#include 	"robot_socket.h"

/********************************* Defines ************************************/

#if local
//	char SERVER_IP[20] = "127.0.0.1";
	char SERVER_IP[20] = "192.168.152.129"; //sql
//	char SERVER_IP[20] = "192.168.172.128";	//zjq,gjc
//	char SERVER_IP[20] = "192.168.121.129";	//wsk
#else
	char SERVER_IP[20] = "192.168.58.2";
#endif
char SERVER_PI_IP[20] = "192.168.58.77";
int WEBAPP_PORT = 8055;
//#define SERVER_PI_IP "192.168.58.88"

PI_PTHREAD pi_pt_status;   /** PI 状态反馈线程结构体 */
PI_PTHREAD pi_pt_cmd;	/** PI 指令下发线程结构体 */

SOCKET_INFO socket_cmd;
SOCKET_INFO socket_file;
SOCKET_INFO socket_status;
SOCKET_INFO socket_state;
SOCKET_SERVER_INFO socket_upper_computer;
int socket_connect_client_num; // server 已连接 client 个数;
SOCKET_INFO socket_torquesys;
SOCKET_PI_INFO socket_pi_status;
SOCKET_PI_INFO socket_pi_cmd;
SOCKET_INFO socket_vir_cmd;
SOCKET_INFO socket_vir_file;
SOCKET_INFO socket_vir_status;

STATE_FEEDBACK state_fb;
CTRL_STATE ctrl_state;
PI_STATUS pi_status;
CTRL_STATE vir_ctrl_state;
CTRL_STATE pre_ctrl_state;
CTRL_STATE pre_vir_ctrl_state;

FB_LinkQuene fb_quene;
extern ACCOUNT_INFO cur_account;
extern int robot_type;
extern int language;
extern char error_info[ERROR_SIZE];
TORQUE_SYS torquesys;
TORQUE_SYS_STATE torque_sys_state;
JIABAO_TORQUE_PRODUCTION_DATA jiabao_torque_pd_data;
char lua_filename[FILENAME_SIZE] = "";
POINT_HOME_INFO point_home_info = {
	.error_flag = 0,
	.pre_error_flag = 0,
};
ZHENGKU zhengku_info;
//pthread_cond_t cond_cmd;
//pthread_cond_t cond_file;
//pthread_mutex_t mute_cmd;
//pthread_mutex_t mute_file;

/********************************* Function declaration ***********************/

static void argc_error_info(int lua_argc, char *func_name);
static void database_error_info();
static void state_feedback_init(STATE_FEEDBACK *fb);
static void socket_init(SOCKET_INFO *sock, const int port);
static void socket_pi_init(SOCKET_PI_INFO *sock, const int port);
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
static int update_homefile(int line_num);
static int socket_bind_listen(SOCKET_SERVER_INFO *sock);
static int socket_upper_computer_send(SOCKET_CONNECT_CLIENT_INFO *sock, const int cmd_id, const int data_len, const char *data_content);
static int socket_gengku_send(SOCKET_CONNECT_CLIENT_INFO *sock, const char *data_content);
static int movej_point(char *pname, char *ovl);
static int check_lua_point_pos();
static int gengku_servar(cJSON *data_json);
static int gengku_getvar(cJSON *data_json);
static int gengku_backtohome(cJSON *data_json);
static int gengku_get_alarm_error(cJSON *data_json);
static void *socket_upper_computer_recv_send(void *arg);
static int socket_pi_pkg_handle(char* buf_memory, uint8* frame, char** change_memory);
static void init_torquesys();
static int init_torque_production_data();
/*
static void *socket_cmd_send_thread(void *arg);
static void *socket_cmd_recv_thread(void *arg);
static void *socket_file_send_thread(void *arg);
static void *socket_file_recv_thread(void *arg);
*/

/*********************************** Code *************************************/
/* argc error info */
static void argc_error_info(int lua_argc, char *func_name)
{
	sprintf(error_info, "bad argument #%d to '%s' (Error number of parameters)", lua_argc, func_name);
}

/* database error info */
static void database_error_info()
{
	sprintf(error_info, "failed to query the database (the data does not exist)");
}

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

/* socket pi init */
static void socket_pi_init(SOCKET_PI_INFO *sock, const int port)
{
	bzero(sock, sizeof(SOCKET_PI_INFO));

	sock->fd = 0;
	strcpy(sock->server_ip, SERVER_PI_IP);
	sock->server_port = port;
	sock->select_timeout = SOCK_SELECT_TIMEOUT;
	sock->pre_connect_status = 0;
	sock->connect_status = 0;
	sock->send_flag = 0;
}

/* socket server init */
static void socket_server_init(SOCKET_SERVER_INFO *sock, const int port)
{
	int i = 0;
	bzero(sock, sizeof(SOCKET_SERVER_INFO));

	sock->serv_fd = 0;
	strcpy(sock->server_ip, SERVER_IP);
	sock->server_port = port;
	socket_connect_client_num = 0;
	sock->server_sendcmd_TM_flag = 0;
	for (i = 0; i < SOCKET_CONNECT_CLIENT_NUM_MAX; i++) {
		sock->socket_connect_client_info[i].clnt_fd = 0;
		sock->socket_connect_client_info[i].connect_status = 0;
		sock->socket_connect_client_info[i].msghead = 0;
	}
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
#if print_mode
		printf("connect with server immediately\n");
#endif
		fcntl(sock->fd, F_SETFL, fdopt);

		return SUCCESS;
	} else if (errno != EINPROGRESS) {
#if print_mode
		printf("unblock connect failed!\n");
#endif
		fcntl(sock->fd, F_SETFL, fdopt);

		return FAIL;
	} else if (errno == EINPROGRESS) {
#if print_mode
		printf("unblock mode socket is connecting...\n");
#endif
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
#if print_mode
		printf("connection timeout or fail!\n");
#endif

		return -1;
	}
	if(!FD_ISSET(sock->fd, &writefd)) {
#if print_mode
		printf("no events on sock fd found\n");
#endif

		return -1;
	}

	//get socket status
	if(getsockopt(sock->fd, SOL_SOCKET, SO_ERROR, &error, &len) < 0) {
#if print_mode
		printf("get socket option failed\n");
#endif

		return -1;
	}
	if(error != 0) {
#if print_mode
		printf("connection failed after select with the error: %d \n", error);
#endif

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
	int sendbuf_len = 0;
	int send_index = 0;

	sendbuf_len = 4 + 3 + get_n_len(node->msghead) + 3 + get_n_len(node->type) + 3 + get_n_len(node->msglen) + 3 + node->msglen + 3 + 4 + 1; // +1 to save '\0
	sendbuf = (char *)calloc(1, sizeof(char)*(sendbuf_len));
	if (sendbuf == NULL) {
		perror("calloc");

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
	//printf("sendbuf_len = %d\n", sendbuf_len);
	printf("strlen(sendbuf) = %d\n", strlen(sendbuf));
	printf("send data to socket server is: %s\n", sendbuf);

	/* send over 1024 bytes */
	while ((sendbuf_len-send_index) > MAX_BUF) {
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
	send_len = sendbuf_len - send_index - 1; // -1 代表 '\0', '\0' 不要发送，否则会产生一个空格，导致发送异常
	if (send(sock->fd, (sendbuf + send_index), send_len, 0) != send_len) {
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
	char cmd[128] = {0};
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
	char frame[MAX_BUF] = {0};//提取出一帧, 存放buf
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
	while (socket_pkg_handle(buf_memory, sec_buf_memory, frame, BUFFSIZE, MAX_BUF) != 0) {
		//printf("frame is : %s\n", frame);
		/* 把接收到的包按照分割符"III"进行分割 */
		if (string_to_string_list(frame, "III", &size_package, &array) == 0 || size_package != 6) {
			perror("string to string list");
			string_list_free(&array, size_package);

			continue;
		}
		/* 此处为 webserver 作为 socket server 直接发送 cmd 指令到任务管理器, 任务管理器反馈运行结果, 直接丢弃接收到的任务管理器指令反馈数据 */
		if (socket_upper_computer.server_sendcmd_TM_flag > 0) {
			// 更酷程序标志: 下发设置变量指令， 先发 105 文件名， 收到 105 的指令反馈后，再发 101 start
			if ((zhengku_info.setvar == 1 || zhengku_info.backhome == 1) && atoi(array[2]) == 105) {
				SOCKET_INFO *sock_cmd = NULL;
				if (robot_type == 1) { // "1" 代表实体机器人
					sock_cmd = &socket_cmd;
				} else { // "0" 代表虚拟机器人
					sock_cmd = &socket_vir_cmd;
				}
				//printf("更酷 send start\n");
				socket_enquene(sock_cmd, 101, "START", 0);
				socket_upper_computer.server_sendcmd_TM_flag++;
			}
			socket_upper_computer.server_sendcmd_TM_flag--;
			string_list_free(&array, size_package);

			continue;
		}
		// 没有账户登录时或者示教器（浏览器） 不存在时, 直接丢弃接收到的任务管理器指令反馈数据
		//printf("websGetSessionCount() = %d\n", websGetSessionCount());
		if (websGetSessionCount() <= 0) {
			if (atoi(array[2]) == 502) {//获取按钮盒 IO 状态
				if (atoi(array[4]) == 3) {
					socket_enquene(&socket_cmd, 101, "START", 0);
				}
			}
			string_list_free(&array, size_package);

			continue;
		}
		/* 遍历整个队列, 更改相关结点信息 */
		/* 创建结点 */
		QElemType node;
		//printf("array[2] = %s\n", array[2]);
		//printf("array[4] = %s\n", array[4]);
		if (atoi(array[2]) == 229 || atoi(array[2]) == 527 || atoi(array[2]) == 546) {//反馈夹爪配置信息, 获取力/扭矩传感器配置信息,获取传感器配置
			bzero(&grippers_config_info, sizeof(GRIPPERS_CONFIG_INFO));
			//printf("array[4] = %s\n", array[4]);
			StringToBytes(array[4], (BYTE *)&grippers_config_info, sizeof(GRIPPERS_CONFIG_INFO));
			root_json = cJSON_CreateArray();
			for (i = 0; i < MAXGRIPPER; i++) {
				/*
				printf("id = %d\n", (i+1));
				printf("name = %d\n", grippers_config_info.id_company[i]);
				printf("type = %d\n", grippers_config_info.id_device[i]);
				printf("version = %d\n", grippers_config_info.id_softversion[i]);
				printf("position = %d\n", grippers_config_info.id_bus[i]);
				*/
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
				string_list_free(&array, size_package);
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
				string_list_free(&msg_array, size_content);
				string_list_free(&array, size_package);

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
				string_list_free(&msg_array, size_content);
				string_list_free(&array, size_package);
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
			string_list_free(&msg_array, size_content);
		} else if (atoi(array[2]) == 278) {//反馈激光传感器记录点内
			if (string_to_string_list(array[4], ",", &size_content, &msg_array) == 0 || size_content != 16) {
				perror("string to string list");
				printf("size_content = %d\n", size_content);
				string_list_free(&msg_array, size_content);
				string_list_free(&array, size_package);

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
				string_list_free(&msg_array, size_content);
				string_list_free(&array, size_package);
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
			string_list_free(&msg_array, size_content);
		} else if (atoi(array[2]) == 325) {//计算 TCF to Joint
			if (string_to_string_list(array[4], ",", &size_content, &msg_array) == 0 || size_content != 6) {
				perror("string to string list");
				printf("size_content = %d\n", size_content);
				string_list_free(&msg_array, size_content);
				string_list_free(&array, size_package);

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
				string_list_free(&msg_array, size_content);
				string_list_free(&array, size_package);
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
			string_list_free(&msg_array, size_content);
		} else if (atoi(array[2]) == 283) {//获取激光跟踪传感器配置信息
			root_json = cJSON_CreateObject();
			if (string_to_string_list(array[4], ",", &size_content, &msg_array) == 0 || size_content != 4) {
				perror("string to string list");
				//printf("size_content = %d\n", size_content);
				string_list_free(&msg_array, size_content);
				string_list_free(&array, size_package);

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
				string_list_free(&msg_array, size_content);
				string_list_free(&array, size_package);
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
			string_list_free(&msg_array, size_content);
		} else if (atoi(array[2]) == 358) {//获取负载信息
			root_json = cJSON_CreateObject();
			if (string_to_string_list(array[4], ",", &size_content, &msg_array) == 0 || size_content != 4) {
				perror("string to string list");
				//printf("size_content = %d\n", size_content);
				string_list_free(&msg_array, size_content);
				string_list_free(&array, size_package);

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
				string_list_free(&msg_array, size_content);
				string_list_free(&array, size_package);
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
			string_list_free(&msg_array, size_content);
		} else if (atoi(array[2]) == 345) {//检测导入的机器人配置文件
			//printf("345: array[4] = %s\n", array[4]);
			/*
			   1. 用户恢复出厂值成功
			   2. 导入控制器端用户配置文件成功
			   3. 导入用户数据文件成功
			*/
			/** 拷贝 user.config 到 /root/robot 目录下 */
			if (strcmp(array[4], "1") == 0) {
				memset(cmd, 0, 128);
				sprintf(cmd, "cp %s %s", WEB_USER_CFG, USER_CFG);
				system(cmd);
			/** 拷贝 exaxis.config 到 /root/robot 目录下 */
			} else if (strcmp(array[4], "2") == 0) {
				memset(cmd, 0, 128);
				sprintf(cmd, "cp %s %s", WEB_EXAXIS_CFG, EXAXIS_CFG);
				system(cmd);
			/** 拷贝 ex_device.config 到 /root/robot 目录下 */
			} else if (strcmp(array[4], "3") == 0) {
				memset(cmd, 0, 128);
				sprintf(cmd, "cp %s %s", WEB_EX_DEVICE_CFG, EX_DEVICE_CFG);
				system(cmd);
			}
			if (createnode(&node, atoi(array[2]), array[4]) == FAIL) {
				string_list_free(&array, size_package);

				continue;
			}
			// 延迟 300 ms, 防止 100ms 状态反馈内一次性插入多条 345 指令反馈，由于 json 格式特性会发生覆盖
			usleep(300000);
		} else if (atoi(array[2]) == 380) {//获取控制器计算后,修改示教点数据
			root_json = cJSON_CreateObject();
			if (string_to_string_list(array[4], ",", &size_content, &msg_array) == 0 || size_content != 6) {
				perror("string to string list");
				//printf("size_content = %d\n", size_content);
				string_list_free(&msg_array, size_content);
				string_list_free(&array, size_package);

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
				string_list_free(&msg_array, size_content);
				string_list_free(&array, size_package);
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
			string_list_free(&msg_array, size_content);
		} else if (atoi(array[2]) == 386) {
			root_json = cJSON_CreateObject();
			if (string_to_string_list(array[4], ",", &size_content, &msg_array) == 0 || size_content != 3) {
				perror("string to string list");
				//printf("size_content = %d\n", size_content);
				string_list_free(&msg_array, size_content);
				string_list_free(&array, size_package);

				continue;
			}

			cJSON_AddStringToObject(root_json, "x_offset", msg_array[0]);
			cJSON_AddStringToObject(root_json, "y_offset", msg_array[1]);
			cJSON_AddStringToObject(root_json, "z_offset", msg_array[2]);
			msg_content = cJSON_Print(root_json);
			cJSON_Delete(root_json);
			root_json = NULL;
			if (createnode(&node, atoi(array[2]), msg_content) == FAIL) {
				string_list_free(&msg_array, size_content);
				string_list_free(&array, size_package);
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
			string_list_free(&msg_array, size_content);
		} else if (atoi(array[2]) == 393) {//获取外部轴驱动器配置信息
			root_json = cJSON_CreateObject();
			if (string_to_string_list(array[4], ",", &size_content, &msg_array) == 0 || size_content != 3) {
				perror("string to string list");
				//printf("size_content = %d\n", size_content);
				string_list_free(&msg_array, size_content);
				string_list_free(&array, size_package);

				continue;
			}

			cJSON_AddStringToObject(root_json, "company", msg_array[0]);
			cJSON_AddStringToObject(root_json, "model", msg_array[1]);
			cJSON_AddStringToObject(root_json, "encType", msg_array[2]);
			msg_content = cJSON_Print(root_json);
			cJSON_Delete(root_json);
			root_json = NULL;
			if (createnode(&node, atoi(array[2]), msg_content) == FAIL) {
				string_list_free(&msg_array, size_content);
				string_list_free(&array, size_package);
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
			string_list_free(&msg_array, size_content);
		} else if (atoi(array[2]) == 412) {
			root_json = cJSON_CreateObject();
			cJSON_AddNumberToObject(root_json, "enable", (atoi(array[4])-10));
			msg_content = cJSON_Print(root_json);
			cJSON_Delete(root_json);
			root_json = NULL;
			if (createnode(&node, atoi(array[2]), msg_content) == FAIL) {
				string_list_free(&array, size_package);
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
					string_list_free(&array, size_package);

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
					string_list_free(&array, size_package);

					continue;
				}
				//printf("after pthread_join\n");
			}
		} else if (atoi(array[2]) == 423) {//获取从站硬件版本
			if (string_to_string_list(array[4], ",", &size_content, &msg_array) == 0 || size_content != 8) {
				perror("string to string list");
				//printf("size_content = %d\n", size_content);
				string_list_free(&msg_array, size_content);
				string_list_free(&array, size_package);

				continue;
			}

			root_json = cJSON_CreateStringArray(msg_array, 8);
			msg_content = cJSON_Print(root_json);
			//printf("msg_content = %s\n", msg_content);
			cJSON_Delete(root_json);
			root_json = NULL;
			if (createnode(&node, atoi(array[2]), msg_content) == FAIL) {
				string_list_free(&msg_array, size_content);
				string_list_free(&array, size_package);
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
			string_list_free(&msg_array, size_content);
		} else if (atoi(array[2]) == 424) {//获取从站固件版本
			if (string_to_string_list(array[4], ",", &size_content, &msg_array) == 0 || size_content != 8) {
				perror("string to string list");
				//printf("size_content = %d\n", size_content);
				string_list_free(&msg_array, size_content);
				string_list_free(&array, size_package);

				continue;
			}

			root_json = cJSON_CreateStringArray(msg_array, 8);
			msg_content = cJSON_Print(root_json);
			//printf("msg_content = %s\n", msg_content);
			cJSON_Delete(root_json);
			root_json = NULL;
			if (createnode(&node, atoi(array[2]), msg_content) == FAIL) {
				string_list_free(&msg_array, size_content);
				string_list_free(&array, size_package);
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
			string_list_free(&msg_array, size_content);
		} else if (atoi(array[2]) == 429) {//获取机器人作业原点
			root_json = cJSON_CreateObject();
			if (string_to_string_list(array[4], ",", &size_content, &msg_array) == 0 || size_content != 7) {
				perror("string to string list");
				//printf("size_content = %d\n", size_content);
				string_list_free(&msg_array, size_content);
				string_list_free(&array, size_package);

				continue;
			}
			cJSON *joints_json = NULL;

			cJSON_AddStringToObject(root_json, "flag", msg_array[0]);
			joints_json = cJSON_CreateObject();
			cJSON_AddItemToObject(root_json, "joints", joints_json);
			cJSON_AddStringToObject(joints_json, "j1", msg_array[1]);
			cJSON_AddStringToObject(joints_json, "j2", msg_array[2]);
			cJSON_AddStringToObject(joints_json, "j3", msg_array[3]);
			cJSON_AddStringToObject(joints_json, "j4", msg_array[4]);
			cJSON_AddStringToObject(joints_json, "j5", msg_array[5]);
			cJSON_AddStringToObject(joints_json, "j6", msg_array[6]);
			msg_content = cJSON_Print(root_json);
			cJSON_Delete(root_json);
			root_json = NULL;
			//printf("msg_content = %s\n", msg_content);
			if (createnode(&node, atoi(array[2]), msg_content) == FAIL) {
				string_list_free(&msg_array, size_content);
				string_list_free(&array, size_package);
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
			string_list_free(&msg_array, size_content);
		} else if (atoi(array[2]) == 530) {//获取重量辨识数据
			root_json = cJSON_CreateObject();
			if (string_to_string_list(array[4], ",", &size_content, &msg_array) == 0 || size_content != 1) {
				perror("string to string list");
				string_list_free(&msg_array, size_content);
				string_list_free(&array, size_package);

				continue;
			}
			cJSON_AddStringToObject(root_json, "weight", msg_array[0]);
			msg_content = cJSON_Print(root_json);
			cJSON_Delete(root_json);
			root_json = NULL;
			if (createnode(&node, atoi(array[2]), msg_content) == FAIL) {
				string_list_free(&msg_array, size_content);
				string_list_free(&array, size_package);
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
			string_list_free(&msg_array, size_content);
		} else if (atoi(array[2]) == 532) {//获取质心辨识数据
			root_json = cJSON_CreateObject();
			if (string_to_string_list(array[4], ",", &size_content, &msg_array) == 0 || size_content != 3) {
				perror("string to string list");
				string_list_free(&msg_array, size_content);
				string_list_free(&array, size_package);

				continue;
			}
			cJSON_AddStringToObject(root_json, "x", msg_array[0]);
			cJSON_AddStringToObject(root_json, "y", msg_array[1]);
			cJSON_AddStringToObject(root_json, "z", msg_array[2]);
			msg_content = cJSON_Print(root_json);
			cJSON_Delete(root_json);
			root_json = NULL;
			if (createnode(&node, atoi(array[2]), msg_content) == FAIL) {
				string_list_free(&msg_array, size_content);
				string_list_free(&array, size_package);
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
			string_list_free(&msg_array, size_content);
		} else if (atoi(array[2]) == 557) {//计算 TCP 四点法
			root_json = cJSON_CreateObject();
			if (string_to_string_list(array[4], ",", &size_content, &msg_array) == 0 || size_content != 6) {
				perror("string to string list");
				string_list_free(&msg_array, size_content);
				string_list_free(&array, size_package);

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
			if (createnode(&node, atoi(array[2]), msg_content) == FAIL) {
				string_list_free(&msg_array, size_content);
				string_list_free(&array, size_package);
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
			string_list_free(&msg_array, size_content);
		} else {
			if (createnode(&node, atoi(array[2]), array[4]) == FAIL) {
				string_list_free(&array, size_package);

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
		string_list_free(&array, size_package);
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
		// 延迟 20 ms
		//usleep(20000);
		// 延迟 1 ms
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
			/* 嘉宝项目：从生产数据数据库中读取生产数据，应用到系统变量中，并更新全局的结构体 */
			init_torque_production_data();
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

static int update_homefile(int line_num)
{
	const char s[2] = "\n";
	int line = 0;
	char *f_content = NULL;
	char *token = NULL;
	char write_content[LINE_LEN] = { 0 };
	char lua_content[GENGKU_LUASIZE] = { 0 };

	strcpy(lua_content, zhengku_info.lua_content);

	token = strtok(lua_content, s);
	while (token != NULL) {
		line++;
		if (line == line_num) {
			if (strstr(token, "MoveJ") || strstr(token, "MoveL")) {
				f_content = get_complete_file_content(FILE_GENGKU_HOMELUA);
				sprintf(write_content, "%s\n", token);
				if (write_file(FILE_GENGKU_HOMELUA, write_content) == FAIL) {

					return FAIL;
				}

				if (f_content == NULL) {

					return FAIL;
				} else if (strcmp(f_content, "NO_FILE") == 0 || strcmp(f_content, "Empty") == 0) {

				} else {
					//printf("f_content = %s\n", f_content);
					write_file_append(FILE_GENGKU_HOMELUA, f_content);
					free(f_content);
					f_content = NULL;
				}
			}
			break;
		}
		/* get other line */
		token = strtok(NULL, s);
	}

	return SUCCESS;
}

void *socket_status_thread(void *arg)
{
	SOCKET_INFO *sock = NULL;
	CTRL_STATE *state = NULL;
	CTRL_STATE *pre_state = NULL;
	int port = (int)arg;
	int recv_len = 0;
	char buf_memory[STATE_BUFFSIZE] = {0};
	int buf_memory_len = 0; /** buf 中已存储的数据，所占字节长度 */
	char frame[STATE_SIZE] = {0};//提取出一帧, 存放buf
	char recvbuf[STATE_SIZE] = {0};
	//int i = 0;

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

		memset(buf_memory, 0, STATE_BUFFSIZE);
		buf_memory_len = 0;
		/* recv ctrl status */
		while (1) {
			//printf("sizeof CTRL_STATE = %d\n", sizeof(CTRL_STATE)); /* Now struct is 3289 bytes */
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
			// 如果收到的数据包长度加上已有 buf_memory_len, 已经超过 buf_memory 定义空间大小(STATE_BUFFSIZE), 清空 buf_memory
			if ((buf_memory_len + recv_len) > STATE_BUFFSIZE) {
				memset(buf_memory, 0, STATE_BUFFSIZE);
				buf_memory_len = 0;
			}
			memcpy((buf_memory + buf_memory_len), recvbuf, recv_len);
			buf_memory_len = buf_memory_len + recv_len;
			// 获取到的一帧长度等于期望长度（结构体长度，包头包尾长度，分隔符等）
			while (buf_memory_handle(buf_memory, STATE_BUFFSIZE, &buf_memory_len, frame, STATE_SIZE) == (sizeof(CTRL_STATE) + 14)) {
				//printf("state = %p\n", state);

				bzero(state, sizeof(CTRL_STATE));
				memcpy(state, (frame + 7), sizeof(CTRL_STATE));

				//for (i = 11; i <= 13; i++) {
				//	printf("state->sys_var[%d] = %d\n", i, (int)state->sys_var[i]);
				//}
				//for (i = 16; i <= 18; i++) {
				//	printf("state->sys_var[%d] = %d\n", i, (int)state->sys_var[i]);
				//}
				//for (i = 0; i <= 5; i++) {
				//	printf("state->FT_data[%d] = %lf\n", i, state->FT_data[i]);
				//}
				/* 系统变量发生改变时 */
				if (jiabao_torque_pd_data.left_product_count != (int)state->sys_var[11] || jiabao_torque_pd_data.left_NG_count != (int)state->sys_var[12] || jiabao_torque_pd_data.left_work_time != (int)state->sys_var[13] || jiabao_torque_pd_data.right_product_count != (int)state->sys_var[16] || jiabao_torque_pd_data.right_NG_count != (int)state->sys_var[17] || jiabao_torque_pd_data.right_work_time != (int)state->sys_var[18]) {
					/* 更新全局的结构体 */
					jiabao_torque_pd_data.left_product_count = (int)state->sys_var[11];
					jiabao_torque_pd_data.left_NG_count = (int)state->sys_var[12];
					jiabao_torque_pd_data.left_work_time = (int)state->sys_var[13];
					jiabao_torque_pd_data.right_product_count = (int)state->sys_var[16];
					jiabao_torque_pd_data.right_NG_count = (int)state->sys_var[17];
					jiabao_torque_pd_data.right_work_time = (int)state->sys_var[18];
					/* 保存最新数据到生产数据数据库 */
					update_torquesys_pd_data();
				}
				// lua 程序正在运行
				if (state->program_state == 2) {
					//printf("zhengku_info.line_num = %d\n", zhengku_info.line_num);
					//printf("state->line_number = %d\n", state->line_number);
					//printf("state->program_state = %d\n", state->program_state);
					// 更酷：下发设置变量指令,代表正在运行路点程序
					// 更酷程序标志： 已经下发设置变量指令
					if (zhengku_info.setvar == 1) {
						zhengku_info.setvar = 2;
					}
					// 更酷程序标志：正在运行程序
					if (zhengku_info.setvar == 2) {
						if (zhengku_info.line_num != state->line_number) {
							zhengku_info.line_num = state->line_number;
							// 更新原路返回的路径点 lua 文件
							update_homefile(zhengku_info.line_num);
						}
					}
					// 更酷回原点程序标志： 已经下发回原点指令
					if (zhengku_info.backhome == 1) {
						zhengku_info.backhome = 2;
					}
				}
				//printf("state->abnormal_stop = %d\n", state->abnormal_stop);
				// lua 程序 停止
				if (state->program_state == 1) {
					// 正常停止
					if (state->abnormal_stop == 0) {
						// 更酷程序标志: 正在运行程序
						if (zhengku_info.setvar == 2) {
							//更酷程序标志: 设置变量指令运行程序已经结束
							zhengku_info.setvar = 0;
							zhengku_info.line_num = 0;
							strcpy(zhengku_info.result, zhengku_info.luaname);
							// 更新原路返回的路径点 lua 文件, 保存当前运行程序的最后一个点
							//update_homefile(state->line_number);
						}
						// 更酷回原点程序标志：正在运行程序
						if (zhengku_info.backhome == 2) {
							zhengku_info.backhome = 0;
							char cmd[128] = {0};
							sprintf(cmd, "rm %s", FILE_GENGKU_HOMELUA);
							//printf("cmd = %s\n", cmd);
							system(cmd);
						}
					}
				}
			}
		}
		/* socket disconnected */
		/* close socket */
		close(sock->fd);
		/* set socket status: disconnected */
		sock->connect_status = 0;
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
				memcpy(&torque_sys_state, (frame + 7), sizeof(TORQUE_SYS_STATE));
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
	//int linenum = 0;
	char strline[LINE_LEN] = {0};
	char *buf_memory = NULL;
	char *sec_buf_memory = NULL;
//	char *frame = NULL;//提取出一帧, 存放buf
	char frame[STATEFB_SIZE] = {0};
	int recv_len = 0;
	//char **array = NULL;
	int varnum = 0;
	int varnum_len = 0;
	int flag_print = 0;
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
			//printf("recv len = %d\n", recv_len);
			//printf("recv buf = %s\n", recv_buf);

			//clock_t time_1, time_2, time_3, time_4, time_5;

			//time_1 = clock();
			//printf("time_1, %d\n", time_1);
			//recv_buf[recv_len] = '\0';
			// 如果收到的数据包长度加上已有 buf_memory 长度已经超过 buf_memory 定义空间大小(STAFB_BUFFSIZE), 清空 buf_memory
			if ((strlen(buf_memory)+recv_len) > 2*STATEFB_SIZE) {
				bzero(buf_memory, 2*STATEFB_SIZE);
			}
			memcpy((buf_memory+strlen(buf_memory)), recv_buf, recv_len);

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
					string_list_free(&array, size);

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
					/*
					int i = 0;
					int j = 0;
					if (flag_print == 0) {
						for (i = 0; i < STATEFB_PERPKG_NUM; i++) {
							printf("fb_quene.front fb[%d] = ", i);
							for (j = 0; j < state_fb.icount; j++) {
								printf("%f ", sta_fb.fb[i][j]);
							}
							printf("\n");
						}
						flag_print = 1; //只打印一次
					}
					*/
					//time_4 = clock();
					//printf("time_4, %d\n", time_4);
					//printf("enter/if\n");
					if (state_fb.type == 0) { //"0":图表查询
						/** 查询队列中存储 node 超过最大数量，清空队列 */
						if (fb_get_node_num(fb_quene) >= STATEFB_MAX) {
							printf("state_fb overflow\n");
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
						//printf("state_fb.buf strlen = %d\n", strlen(state_fb.buf));
						//printf("state_fb.index = %d\n", state_fb.index);
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
				//string_list_free(&array, size);
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

	/*
	   每当有一个客户端 connect 了，
	   listen 的队列中就加入一个连接，
	   每当服务器端 accept 了，
	   就从listen的队列中取出一个连接，
	   转成一个专门用来传输数据的socket（accept函数的返回值）
	*/
	if (listen(sock->serv_fd, 10) == -1) {
		printf("listen socket error: %s(errno: %d)\n", strerror(errno), errno);

		return FAIL;
	}

	return SUCCESS;
}

/* socket_upper_computer_send */
static int socket_upper_computer_send(SOCKET_CONNECT_CLIENT_INFO *sock, const int cmd_id, const int data_len, const char *data_content)
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
	printf("send data to socket client is: %s\n", sendbuf);

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

/* socket_gengku_send */
static int socket_gengku_send(SOCKET_CONNECT_CLIENT_INFO *sock, const char *data_content)
{
	int send_len = 0;

	send_len = strlen(data_content);

	printf("send data len to gengku socket client is: %d\n", send_len);
	printf("send data to gengku socket client is: %s\n", data_content);
	if (send(sock->clnt_fd, data_content, send_len, 0) != send_len) {
		perror("send");
		/* set socket status: disconnected */
		sock->connect_status = 0;

		return FAIL;
	}

	return SUCCESS;
}

static int movej_point(char *pname, char *ovl)
{
	SOCKET_INFO *sock_cmd = NULL;
	char sql[MAX_BUF] = "";
	cJSON *f_json = NULL;
	cJSON *ptp = NULL;
	cJSON *j1 = NULL;
	cJSON *j2 = NULL;
	cJSON *j3 = NULL;
	cJSON *j4 = NULL;
	cJSON *j5 = NULL;
	cJSON *j6 = NULL;
	cJSON *x = NULL;
	cJSON *y = NULL;
	cJSON *z = NULL;
	cJSON *rx = NULL;
	cJSON *ry = NULL;
	cJSON *rz = NULL;
	cJSON *speed = NULL;
	cJSON *acc = NULL;
	cJSON *toolnum = NULL;
	cJSON *workpiecenum = NULL;
	cJSON *E1 = NULL;
	cJSON *E2 = NULL;
	cJSON *E3 = NULL;
	cJSON *E4 = NULL;
	int i = 0;
	char content[MAX_BUF] = { 0 };
	char joint_value[6][10] = { 0 };
	char *joint_value_ptr[6];
	for (i = 0; i < 6; i++) {
		joint_value_ptr[i] = NULL;
	}

	sprintf(sql, "select * from points;");
	if (select_info_json_sqlite3(DB_POINTS, sql, &f_json) == -1) {
		perror("select ptp points");

		return FAIL;
	}
	ptp = cJSON_GetObjectItemCaseSensitive(f_json, pname);
	if (ptp == NULL || ptp->type != cJSON_Object) {

		return FAIL;
	}
	j1 = cJSON_GetObjectItem(ptp, "j1");
	j2 = cJSON_GetObjectItem(ptp, "j2");
	j3 = cJSON_GetObjectItem(ptp, "j3");
	j4 = cJSON_GetObjectItem(ptp, "j4");
	j5 = cJSON_GetObjectItem(ptp, "j5");
	j6 = cJSON_GetObjectItem(ptp, "j6");
	x = cJSON_GetObjectItem(ptp, "x");
	y = cJSON_GetObjectItem(ptp, "y");
	z = cJSON_GetObjectItem(ptp, "z");
	rx = cJSON_GetObjectItem(ptp, "rx");
	ry = cJSON_GetObjectItem(ptp, "ry");
	rz = cJSON_GetObjectItem(ptp, "rz");
	toolnum = cJSON_GetObjectItem(ptp, "toolnum");
	workpiecenum = cJSON_GetObjectItem(ptp, "workpiecenum");
	speed = cJSON_GetObjectItem(ptp, "speed");
	acc = cJSON_GetObjectItem(ptp, "acc");
	E1 = cJSON_GetObjectItem(ptp, "E1");
	E2 = cJSON_GetObjectItem(ptp, "E2");
	E3 = cJSON_GetObjectItem(ptp, "E3");
	E4 = cJSON_GetObjectItem(ptp, "E4");
	if (j1 == NULL || j2 == NULL || j3 == NULL || j4 == NULL || j5 == NULL || j6 == NULL || x == NULL || y == NULL || z == NULL || rx == NULL || ry == NULL || rz == NULL || toolnum == NULL || workpiecenum == NULL || speed == NULL || acc == NULL || E1 == NULL || E2 == NULL || E3 == NULL || E4 == NULL || j1->valuestring == NULL || j2->valuestring == NULL || j3->valuestring == NULL || j4->valuestring == NULL || j5->valuestring == NULL || j6->valuestring == NULL || x->valuestring == NULL || y->valuestring == NULL || z->valuestring == NULL || rx->valuestring == NULL || ry->valuestring == NULL || rz->valuestring == NULL || toolnum->valuestring == NULL || workpiecenum->valuestring == NULL || speed->valuestring == NULL || acc->valuestring == NULL || E1->valuestring == NULL || E2->valuestring == NULL || E3->valuestring == NULL || E4->valuestring == NULL) {

		return FAIL;
	}

	/* 当点为 pHOME 原点时，进行检查 */
	if (strcmp(pname, POINT_HOME) == 0) {
		sprintf(joint_value[0], "%.1lf", atof(j1->valuestring));
		sprintf(joint_value[1], "%.1lf", atof(j2->valuestring));
		sprintf(joint_value[2], "%.1lf", atof(j3->valuestring));
		sprintf(joint_value[3], "%.1lf", atof(j4->valuestring));
		sprintf(joint_value[4], "%.1lf", atof(j5->valuestring));
		sprintf(joint_value[5], "%.1lf", atof(j6->valuestring));
		for (i = 0; i < 6; i++) {
			joint_value_ptr[i] = joint_value[i];
		}
		/* 置异常报错的标志位， 添加异常错误到 sta 状态反馈 error_info 中 */
		if (check_pointhome_data(joint_value_ptr) == FAIL) {
			point_home_info.error_flag = 1;

			return FAIL;
		}
		point_home_info.error_flag = 0;
	}
	sprintf(content, "MoveJ(%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,0,0,0,0,0,0,0,0)\n", j1->valuestring, j2->valuestring, j3->valuestring, j4->valuestring, j5->valuestring, j6->valuestring, x->valuestring, y->valuestring, z->valuestring, rx->valuestring, ry->valuestring, rz->valuestring, toolnum->valuestring, workpiecenum->valuestring, speed->valuestring, acc->valuestring, ovl, E1->valuestring, E2->valuestring, E3->valuestring, E4->valuestring);
	if (robot_type == 1) { // "1" 代表实体机器人
		sock_cmd = &socket_cmd;
	} else { // "0" 代表虚拟机器人
		sock_cmd = &socket_vir_cmd;
	}
	socket_enquene(sock_cmd, 201, content, 1);
	cJSON_Delete(f_json);
	f_json = NULL;

	return SUCCESS;
}

static int check_lua_point_pos()
{
	char lua_content[GENGKU_LUASIZE] = { 0 };
	const char s[2] = "\n";
	char *token = NULL;
	char **cmd_array = NULL;
	int size = 0;
	CTRL_STATE *state = NULL;
	int ret = FAIL;
	int i = 0;

	if (robot_type == 1) { // "1" 代表实体机器人
		state = &ctrl_state;
	} else { // "0" 代表虚拟机器人
		state = &vir_ctrl_state;
	}
	strcpy(lua_content, zhengku_info.lua_content);

	token = strtok(lua_content, s);
	while (token != NULL) {
		if (strstr(token, "MoveJ") || strstr(token, "MoveL")) {
			if (string_to_string_list(token, ",", &size, &cmd_array) == 0) {
				perror("string to string listi");
				string_list_free(&cmd_array, size);

				return FAIL;
			}
			/**
			for (i = 0; i < 6; i++) {
				printf("state->tl_cur_pos[%d] = %lf\n", i, state->tl_cur_pos[i]);
			}
			for (i = 6; i < 12; i++) {
				printf("cmd_array[%d] = %lf\n", i, atof(cmd_array[i]));
			}
			*/
			if (((atof(cmd_array[6]) - 0.2) < state->tl_cur_pos[0] && state->tl_cur_pos[0] < (atof(cmd_array[6]) + 0.2)) && ((atof(cmd_array[7]) - 0.2) < state->tl_cur_pos[1] && state->tl_cur_pos[1] < (atof(cmd_array[7]) + 0.2)) && ((atof(cmd_array[8]) - 0.2) < state->tl_cur_pos[2] && state->tl_cur_pos[2] < (atof(cmd_array[8]) + 0.2)) && ((atof(cmd_array[9]) - 0.2) < state->tl_cur_pos[3] && state->tl_cur_pos[3] < (atof(cmd_array[9]) + 0.2)) && ((atof(cmd_array[10]) - 0.2) < state->tl_cur_pos[4] && state->tl_cur_pos[4] < (atof(cmd_array[10]) + 0.2)) && ((atof(cmd_array[11]) - 0.2) < state->tl_cur_pos[5] && state->tl_cur_pos[5] < (atof(cmd_array[11]) + 0.2))) {

				ret = SUCCESS;
			} else {

				ret = FAIL;
			}
			//printf("ret = %d\n", ret);
			string_list_free(&cmd_array, size);

			break;
		}
		/* get other line */
		token = strtok(NULL, s);
	}

	return ret;
}

static int gengku_servar(cJSON *data_json)
{
	SOCKET_INFO *sock_file = NULL;
	SOCKET_INFO *sock_cmd = NULL;
	cJSON *luaname_json = NULL;
	char luaname[100] = {0};
	char *lua_content = NULL;
	char user_luaname[100] = {0};
	char cmd[128] = {0};

	if (robot_type == 1) { // "1" 代表实体机器人
		sock_file = &socket_file;
		sock_cmd = &socket_cmd;
	} else { // "0" 代表虚拟机器人
		sock_file = &socket_vir_file;
		sock_cmd = &socket_vir_cmd;
	}

	luaname_json = cJSON_GetObjectItem(data_json, "luaName");
	if (luaname_json == NULL || luaname_json->valuestring == NULL) {
		perror("json");

		return FAIL;
	}
	sprintf(luaname, "%s.lua", luaname_json->valuestring);

	bzero(lua_filename, FILENAME_SIZE);
	sprintf(lua_filename, "%s%s", DIR_FRUSER, luaname);
    lua_content = get_complete_file_content(lua_filename);
	memset(zhengku_info.lua_content, 0, GENGKU_LUASIZE);
	strcpy(zhengku_info.lua_content, lua_content);
	free(lua_content);
	lua_content = NULL;

	// 记录 phome 点
	if (strcmp(luaname, "pHome.lua") == 0) {
		memset(cmd, 0, sizeof(cmd));
		sprintf(cmd, "rm %s", FILE_GENGKU_HOMELUA);
		//printf("cmd = %s\n", cmd);
		system(cmd);
		update_homefile(1);
		strcpy(zhengku_info.result, luaname_json->valuestring);
	} else {
		if (check_lua_point_pos() == SUCCESS) {
			socket_enquene(sock_file, 105, lua_filename, 1);
			socket_upper_computer.server_sendcmd_TM_flag++;

			// 下发设置变量指令,正在运行程序
			zhengku_info.setvar = 1;
			zhengku_info.line_num = 0;
			strcpy(zhengku_info.result, "0");
			strcpy(zhengku_info.luaname, luaname_json->valuestring);
		} else {

			return FAIL;
		}
	}

	return SUCCESS;
}

static int gengku_getvar(cJSON *data_json)
{

	return SUCCESS;
}

static int gengku_backtohome(cJSON *data_json)
{
	SOCKET_INFO *sock_cmd = NULL;
	SOCKET_INFO *sock_file = NULL;
	char *lua_content = NULL;

	if (robot_type == 1) { // "1" 代表实体机器人
		sock_cmd = &socket_cmd;
		sock_file = &socket_file;
	} else { // "0" 代表虚拟机器人
		sock_cmd = &socket_vir_cmd;
		sock_file = &socket_vir_file;
	}

	socket_enquene(sock_cmd, 107, "ResetAllError()", 1);
	socket_upper_computer.server_sendcmd_TM_flag++;

	socket_enquene(sock_file, 105, FILE_GENGKU_HOMELUA, 1);
	socket_upper_computer.server_sendcmd_TM_flag++;

    lua_content = get_complete_file_content(FILE_GENGKU_HOMELUA);
	if (lua_content == NULL) {

		return FAIL;
	}

	free(lua_content);
	lua_content = NULL;
	// 更酷回原点程序标志： 下发回原点指令
	zhengku_info.backhome = 1;
	/**
		设置变量指令运行程序已经结束
		* 防止重复发 start 指令
		* 防止重复记录路点到 lua 文件中
	*/
	zhengku_info.setvar = 0;
	zhengku_info.line_num = 0;

	return SUCCESS;
}

static int gengku_get_alarm_error(cJSON *data_json)
{
	cJSON *error_json = NULL;
	cJSON *alarm_json = NULL;
	char *alarm_info_buf = NULL;
	char *error_info_buf = NULL;

	alarm_json = cJSON_CreateArray();
	error_json = cJSON_CreateArray();

	get_robot_alarm_error_info(alarm_json, error_json);

	alarm_info_buf = cJSON_Print(alarm_json);
	strcpy(zhengku_info.alarm_info, alarm_info_buf);
	free(alarm_info_buf);
	alarm_info_buf = NULL;

	error_info_buf = cJSON_Print(error_json);
	strcpy(zhengku_info.error_info, error_info_buf);
	free(error_info_buf);
	error_info_buf = NULL;

	return SUCCESS;
}

static void *socket_upper_computer_recv_send(void *arg)
{
	SOCKET_CONNECT_CLIENT_INFO *sock = (SOCKET_CONNECT_CLIENT_INFO *)arg;
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
	cJSON *data = NULL;
	char *cmd = NULL;
	cJSON *command = NULL;
	cJSON *param = NULL;
	char log_content[1024] = {0};
	char en_log_content[1024] = {0};
	char jap_log_content[1024] = {0};
	int errorcode = 0;
	char errormsg[100] = { 0 };

	/* set socket status: connected */
	sock->connect_status = 1;
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
			printf("sock do client connent, recv_len : %d\n", recv_len);
			/* 认为连接已经断开 */
			printf("sock do client connent errno : %d\n", errno);
			printf("sock do client connent strerror : %s\n", strerror(errno));
			perror("sock do client connent perror recv :");
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
				string_list_free(&array, size_package);

				continue;
			}
			/** 接收嘉宝的外部设备数据交互指令 */
			if (atoi(array[2]) == 100) {
				if (strcmp(array[4], "GetTorqueData()") == 0) {
					sprintf(data_content, "%s,%d,%d,%d,%s,%d,%d,%d", jiabao_torque_pd_data.left_wk_id, jiabao_torque_pd_data.left_product_count, jiabao_torque_pd_data.left_NG_count, jiabao_torque_pd_data.left_work_time, jiabao_torque_pd_data.right_wk_id, jiabao_torque_pd_data.right_product_count, jiabao_torque_pd_data.right_NG_count, jiabao_torque_pd_data.right_work_time);
				}
			/** 接收文件名 */
			} else if (atoi(array[2]) == 105) {
				socket_enquene(&socket_file, 105, array[4], 1);
				socket_upper_computer.server_sendcmd_TM_flag = 1;
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
				socket_upper_computer.server_sendcmd_TM_flag = 1;
				strcpy(data_content, "1");
			/** STOP */
			} else if (atoi(array[2]) == 102) {
				socket_enquene(&socket_cmd, 102, "STOP", 0);
				socket_upper_computer.server_sendcmd_TM_flag = 1;
				strcpy(data_content, "1");
			/** PAUSE */
			} else if (atoi(array[2]) == 103) {
				socket_enquene(&socket_cmd, 103, "PAUSE", 0);
				socket_upper_computer.server_sendcmd_TM_flag = 1;
				strcpy(data_content, "1");
			/** PESUME */
			} else if (atoi(array[2]) == 104) {
				socket_enquene(&socket_cmd, 104, "RESUME", 0);
				socket_upper_computer.server_sendcmd_TM_flag = 1;
				strcpy(data_content, "1");
			/** 更酷 */
			} else if (atoi(array[2]) == 1002) {
				//printf("array[4] = %s\n", array[4]);
				data = cJSON_Parse(array[4]);
				if (data == NULL) {
					perror("json");
					errorcode = 1;
				} else {
					command = cJSON_GetObjectItem(data, "cmdName");
					if (command == NULL) {
						perror("json");
						errorcode = 1;
					} else {
						cmd = command->valuestring;
						if (!strcmp(cmd, "setVar")) {
							if (gengku_servar(data) == SUCCESS) {
								errorcode = 0;
								strcpy(log_content, "更酷，路点运行-设置变量");
								strcpy(en_log_content, "Even cooler, waypoint run - set variables");
								strcpy(jap_log_content, "よりクールなロードポイント実行変数の設定");
							} else {
								errorcode = 1;
							}
						} else if (!strcmp(cmd, "getVar")) {
							if (gengku_getvar(data) == SUCCESS) {
								errorcode = 0;
								strcpy(log_content, "更酷，路点运行-获取变量");
								strcpy(en_log_content, "Even cooler, waypoints run - get variables");
								strcpy(jap_log_content, "よりクールに、ポイントを実行し、変数を取得する");
							} else {
								errorcode = 1;
							}
						} else if (!strcmp(cmd, "backToHome")) {
							if (gengku_backtohome(data) == SUCCESS) {
								errorcode = 0;
								strcpy(log_content, "更酷，机器人回原点");
								strcpy(en_log_content, "Even cooler, the robot goes back to the origin");
								strcpy(jap_log_content, "ロボットは原点に戻ります");
							} else {
								errorcode = 1;
							}
						} else if (!strcmp(cmd, "getAlarmError")) {
							if (gengku_get_alarm_error(data) == SUCCESS) {
								errorcode = 0;
								strcpy(log_content, "更酷，报警信息查询");
								strcpy(en_log_content, "Even cooler, alarm information query");
								strcpy(jap_log_content, "もっとクールな、通報情報検索");
							} else {
								errorcode = 1;
							}
						} else {
							perror("cmd not found");
							errorcode = 2;
						}
					}
				}
				memset(errormsg, 0, sizeof(errormsg));
				if (errorcode == 0) {
					strcpy(errormsg, "unknown");
				} else if (errorcode == 1) {
					strcpy(errormsg, "Exception:function call failed");
					strcpy(log_content, "更酷，程序发生异常");
					strcpy(en_log_content, "Even cooler, exception:function call failed");
					strcpy(jap_log_content, "プログラムに異常が発生します");
				} else if (errorcode == 2) {
					strcpy(errormsg, "Cmd does not exist!");
					strcpy(log_content, "更酷，指令不存在");
					strcpy(en_log_content, "Even cooler, cmd does not exist");
					strcpy(jap_log_content, "もっと酷, 指令は存在しない");
				}
				if (!strcmp(cmd, "getVar")) {
					sprintf(data_content, "{\"cmdName\":\"%s\",\"errorCode\":\"%d\",\"errorMsg\":\"%s\",\"result\":\"%s\"}", cmd, errorcode, errormsg, zhengku_info.result);
				} else if (!strcmp(cmd, "getAlarmError")) {
					sprintf(data_content, "{\"cmdName\":\"%s\",\"errorCode\":\"%d\",\"errorMsg\":\"%s\",\"alarmInfo\":%s,\"errorInfo\":%s}", cmd, errorcode, errormsg, zhengku_info.alarm_info, zhengku_info.error_info);
				} else {
					sprintf(data_content, "{\"cmdName\":\"%s\",\"errorCode\":\"%d\",\"errorMsg\":\"%s\"}", cmd, errorcode, errormsg);
				}
				my_syslog("机器人操作", log_content, cur_account.username);
				my_en_syslog("robot operation", en_log_content, cur_account.username);
				my_jap_syslog("ロボット操作", jap_log_content, cur_account.username);
				/* cjson delete */
				cJSON_Delete(data);
				data = NULL;
				/* send recv info to upper computer */
				socket_gengku_send(sock, data_content);
				string_list_free(&array, size_package);

				continue;
			} else {
				perror("cmd error");
				string_list_free(&array, size_package);

				continue;
			}
			/* send recv info to upper computer */
			socket_upper_computer_send(sock, atoi(array[2]), strlen(data_content), data_content);
			string_list_free(&array, size_package);
		}
	}
	/* close clnt socket fd */
	printf("close clnt fd\n");
	close(sock->clnt_fd);
	/* clear client info */
	sock->clnt_fd = 0;
	sock->connect_status = 0;
	sock->msghead = 0;
	socket_connect_client_num--;
}

void *socket_upper_computer_thread(void* arg)
{
	pthread_t t_socket_client;
	SOCKET_SERVER_INFO* sock = NULL;
	struct sockaddr_in clnt_socket;
	socklen_t clnt_socket_len = sizeof(clnt_socket);
	char buf_ip[20] = { 0 };
	int port = (int)arg;
	int i = 0;
	int client_socket_fd = 0;
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
			/* socket accepted */
			if ((client_socket_fd = accept(sock->serv_fd, (struct sockaddr *)(&clnt_socket), &clnt_socket_len)) == -1) {
				printf("accept socket error: %s(errno: %d)\n", strerror(errno), errno);

				continue;
			}
			// 如果已经达到最大 client 连接数
			if (socket_connect_client_num == SOCKET_CONNECT_CLIENT_NUM_MAX) {
				printf("connect client num is MAX : %d\n", SOCKET_CONNECT_CLIENT_NUM_MAX);
				printf("close fd: %d\n", client_socket_fd);
				close(client_socket_fd);

				continue;
			}

			// 没有超过最大 client 连接数，允许连接，查询哪个 SOCKET_CONNECT_CLIENT_INFO 结构体可用
			for (i = 0; i < SOCKET_CONNECT_CLIENT_NUM_MAX; i++) {
				if (sock->socket_connect_client_info[i].clnt_fd == 0)  {
					sock->socket_connect_client_info[i].clnt_fd = client_socket_fd;

					break;
				}
			}
			memset(buf_ip, 0,sizeof(buf_ip));
			inet_ntop(AF_INET, &clnt_socket.sin_addr, buf_ip, sizeof(buf_ip));
			printf("Client online, ip is %s, port is %d\n", buf_ip, ntohs(clnt_socket.sin_port));
			printf("clnt_fd = %d\n", sock->socket_connect_client_info[i].clnt_fd);
			// client 连接数加 1
			socket_connect_client_num++;
			printf("connect_client_num = %d\n", socket_connect_client_num);

			/* init zhengku_info */
			memset(&zhengku_info, 0, sizeof(ZHENGKU));
			strcpy(zhengku_info.result, "0");

			pthread_create(&t_socket_client, NULL, (void *)socket_upper_computer_recv_send, (void *)(&sock->socket_connect_client_info[i]));
			pthread_detach(t_socket_client);
		}
		/* close serv socket fd */
		close(sock->serv_fd);
	}
}

/**
	function : 数据包收包后对于“粘包”，“断包”，“残包”，“错包”的处理

	buf_memory[IN] : 待处理数据缓冲区

	frame[out]：获取到的完整一帧数据包

	change_memory：记录每次接收完数据后的位置

	return: 如果有完整一帧数据包，返回其数据长度，否则返回 0 for error
*/
static int socket_pi_pkg_handle(char* buf_memory, uint8* frame, char** change_memory)
{
	assert(buf_memory != NULL);

	int i;
	uint8 head = 0;
	uint8 tail = 0;
	uint8 frame_len = 0;
	uint8 memory_len = 0;
	int data_len = PI_STATUS_SIZE;
	char buf[PI_STATUS_BUFFSIZE] = { 0 }; //内圈循环 buf
	char *temp = NULL;

	//如果缓冲区中为空，则可以直接进行下一轮tcp请求
	while (((*change_memory) - buf_memory) != 0) {
		temp = buf_memory;
		memory_len = (*change_memory) - buf_memory;
		//printf("memory_len = %d\n", memory_len);
		//循环找包头
		for (i = 1; i < memory_len; i++) {
			if(*(temp+i-1) == 0x5a && *(temp+i) == 0x5a) {
				head = i;
				//printf("this is head\n");
				break;
			}
		}
		//循环找包尾
		for (i = 1; i < memory_len; i++) {
			if (*(temp+i-1) == 0x5b && *(temp+i) == 0x5b) {
				tail = i;
				//printf("this is tail\n");
				break;
			}
		}
		// 断包(有头无尾), 即接收到的包不完整，则跳出内圈循环，进入外圈循环，从输入流中继续读取数据
		if (head != 0 && tail == 0) {

			break;
		}
		// 找到了包头包尾，则提取出完整一帧
		if (head != 0 && tail != 0 && head < tail) {
			//判断整包长度是否为 data_len
			if ((tail - head + 2) == data_len) {
				frame_len = data_len;
				memset(frame, 0, data_len);
				memcpy(frame, temp + (head - 1), frame_len);
			}

			//若包的长度不为 32 或者 整包的数据被提出，将包尾后的内容推入最前方
			temp = temp + tail + 1;
			memset(buf, 0, PI_STATUS_BUFFSIZE);
			memcpy(buf, temp, (*change_memory) - temp); //保存末尾数据, (*change_memory) - temp 是包尾后的数据长度
			memset(buf_memory, 0, PI_STATUS_BUFFSIZE); //清空缓冲区, 并把包尾后的内容推入缓冲区
			memcpy(buf_memory, buf, (*change_memory) - temp); //将保存的数据重新复制给buf_memory
			*change_memory = buf_memory + ((*change_memory) - temp);

			break;
		}

		// 残包(只找到包尾,没头)或者错包(没头没尾)的，清空缓冲区，进入外圈循环，从输入流中重新读取数据
		if ((head == 0 && tail != 0) || (head == 0 && tail == 0)) {
			perror("Incomplete package!");
			// 清空缓冲区, 直接跳出内圈循环，到外圈循环里
			memset(buf_memory, 0, PI_STATUS_BUFFSIZE);
			*change_memory = buf_memory;

			break;
		}

		//包头在包尾后面，则扔掉缓冲区中包头之前的多余字节, 继续内圈循环
		if (head != 0 && tail != 0 && head > tail) {
			perror("Error package");
			printf("tail ... head ...\n");
			temp = temp + head - 1;
			memset(buf, 0, PI_STATUS_BUFFSIZE);
			memcpy(buf, temp, (*change_memory) - temp);
			memset(buf_memory, 0, PI_STATUS_BUFFSIZE); //清空缓冲区, 并把包头后的内容推入缓冲区
			memcpy(buf_memory, buf, (*change_memory) - temp);
			*change_memory = buf_memory + ((*change_memory) - temp);
		}
	}

	return frame_len;
}

/* socket_pi_status thread */
void *socket_pi_status_thread(void *arg)
{
	SOCKET_PI_INFO *sock = NULL;
	PI_STATUS *status = NULL;
	int port = (int)arg;
	int recv_len = 0;
	char buf_memory[PI_STATUS_BUFFSIZE] = { 0 }; 	//保存所有的已接收的数据
	char *change_memory = NULL; 					//记录每次拷贝后的位置
	char recvbuf[PI_STATUS_SIZE + 1] = { 0 };
	uint8 frame[PI_STATUS_SIZE] = { 0 };
	int i = 0;

	change_memory = buf_memory;
	printf("port = %d\n", port);
	sock = &socket_pi_status;
	status = &pi_status;

	socket_pi_init(sock, port);

	while (pi_pt_status.enable == 1) {
		bzero(status, sizeof(PI_STATUS));
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
#if print_mode
			perror("socket connect fail");
#endif
			close(sock->fd);
			//delay(1000);
			sleep(1);

			continue;
		}
		/* socket connected */
		/* set socket status: connected */
		sock->connect_status = 1;
		printf("Socket connect success: sockfd = %d\tserver_ip = %s\t server_port = %d\n", sock->fd, SERVER_PI_IP, port);

		/* recv PI status */
		while (pi_pt_status.enable == 1) {
			//bzero(recvbuf, (PI_STATUS_SIZE + 1));
			memset(recvbuf, 0, (PI_STATUS_SIZE + 1));
			recv_len = recv(sock->fd, recvbuf, PI_STATUS_SIZE, 0);
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
			//printf("recvbuf = ");
			//for (i = 0; i < recv_len; i++) {
			//	printf("%x ", recvbuf[i]);
			//}
			//printf("\n");
#if print_mode
			printf("recv len = %d\n", recv_len);
#endif
			memcpy(change_memory, recvbuf, recv_len);
			change_memory = change_memory + recv_len;  //每次接收完消息后，将指针的位置指向消息最后

			//获取到的一帧长度等于期望长度（结构体长度，包头包尾长度，分隔符等）
			while (socket_pi_pkg_handle(buf_memory, frame, &change_memory) == PI_STATUS_SIZE) {
#if print_mode
				printf("frame = ");
				for (i = 0; i < PI_STATUS_SIZE; i++) {
					printf("%x ", frame[i]);
				}
				printf("\n");
#endif
				if (RX_CheckSum(frame, PI_STATUS_SIZE) == 0) {
					//printf("update PI_STATUS\n");
					memset(status, 0, sizeof(PI_STATUS));
					memcpy(status, (frame + 3), (recv_len - 7));
				}
			}
		}
		/* socket disconnected */
		/* close socket */
		close(sock->fd);
		/* set socket status: disconnected */
		sock->connect_status = 0;
	}
}

/* socket_pi_cmd thread */
void *socket_pi_cmd_thread(void *arg)
{
	SOCKET_PI_INFO *sock = NULL;
	int port = (int)arg;
	int recv_len = 0;
	char recvbuf[MAX_BUF] = { 0 };
	char sendbuf[MAX_BUF] = { 0 };
	int msghead = 0;//帧计数

	//pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);   //设置立即取消

	printf("port = %d\n", port);
	sock = &socket_pi_cmd;
	socket_pi_init(sock, port);
	while (pi_pt_cmd.enable == 1) {
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
#if print_mode
			perror("socket connect fail");
#endif
			close(sock->fd);
			//delay(1000);
			sleep(1);

			continue;
		}
		/* socket connected */
		/* set socket status: connected */
		sock->connect_status = 1;
		printf("Socket connect success: sockfd = %d\tserver_ip = %s\t server_port = %d\n", sock->fd, SERVER_PI_IP, port);

		/* send/recv PI cmd */
		while (pi_pt_cmd.enable == 1) {
			if (sock->send_flag == 1) {
				msghead++;
				/* send 树莓派 ip */
				memset(sendbuf, 0, MAX_BUF);
				sprintf(sendbuf, "/f/bIII%dIII%dIII%dIII%sIII/b/f", msghead, 100, strlen(SERVER_PI_IP), SERVER_PI_IP);
				if (send(sock->fd, sendbuf, strlen(sendbuf), 0) != strlen(sendbuf)) {
					perror("send");

					break;
				}
				//bzero(recvbuf, (PI_STATUS_SIZE + 1));
				memset(recvbuf, 0, MAX_BUF);
				recv_len = recv(sock->fd, recvbuf, MAX_BUF, 0);
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
				printf("recv len = %d\n", recv_len);
				printf("recvbuf = %s\n", recvbuf);

				msghead++;
				/* send 控制器 ip */
				memset(sendbuf, 0, MAX_BUF);
				sprintf(sendbuf, "/f/bIII%dIII%dIII%dIII%sIII/b/f", msghead, 101, strlen(SERVER_IP), SERVER_IP);
				if (send(sock->fd, sendbuf, strlen(sendbuf), 0) != strlen(sendbuf)) {
					perror("send");

					break;
				}
				//bzero(recvbuf, (PI_STATUS_SIZE + 1));
				memset(recvbuf, 0, MAX_BUF);
				recv_len = recv(sock->fd, recvbuf, MAX_BUF, 0);
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
				printf("recv len = %d\n", recv_len);
				printf("recvbuf = %s\n", recvbuf);

				sock->send_flag = 0;
			}
			// 延迟 1 ms
			usleep(1000);
		}
		/* socket disconnected */
		/* close socket */
		close(sock->fd);
		/* set socket status: disconnected */
		sock->connect_status = 0;
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

/**
	下发获取原点指令，读取配置文件中的 pHome 数据，和数据库中数据进行比较, 比较小数点后一位小数
	比较这两组数据内容，是否相同，如果不同，说明原点发生改变，需要添加异常错误到 sta 状态反馈 error_info 中
	最多等待 1 秒钟获取指令反馈，否则超时报错
*/
int check_pointhome_data(char *arr[])
{
	Qnode *p = NULL;
	int i = 0;
	int ret = FAIL;
	SOCKET_INFO *sock_cmd = NULL;
	cJSON *msgcontent_json = NULL;
	cJSON *joints_json = NULL;
	cJSON *j1_json = NULL;
	cJSON *j2_json = NULL;
	cJSON *j3_json = NULL;
	cJSON *j4_json = NULL;
	cJSON *j5_json = NULL;
	cJSON *j6_json = NULL;
	char joint_value[6][10] = {0};
	char content[MAX_BUF] = "";

	if (robot_type == 1) { // "1" 代表实体机器人
		sock_cmd = &socket_cmd;
	} else { // "0" 代表虚拟机器人
		sock_cmd = &socket_vir_cmd;
	}
	sprintf(content, "GetRobotWorkHomePoint()");
	socket_enquene(sock_cmd, 429, content, 1);
	for (i = 0; i < 1000; i++) {
		p = sock_cmd->ret_quene.front->next;
		while (p != NULL) {
			if (p->data.type == 429) {
				msgcontent_json = cJSON_Parse(p->data.msgcontent);
				if (msgcontent_json != NULL) {
					joints_json = cJSON_GetObjectItem(msgcontent_json, "joints");
					if (joints_json != NULL && joints_json->type == cJSON_Object) {
						j1_json = cJSON_GetObjectItem(joints_json, "j1");
						j2_json = cJSON_GetObjectItem(joints_json, "j2");
						j3_json = cJSON_GetObjectItem(joints_json, "j3");
						j4_json = cJSON_GetObjectItem(joints_json, "j4");
						j5_json = cJSON_GetObjectItem(joints_json, "j5");
						j6_json = cJSON_GetObjectItem(joints_json, "j6");
						if (j1_json != NULL && j2_json != NULL && j3_json != NULL && j4_json != NULL && j5_json != NULL && j6_json != NULL) {
							sprintf(joint_value[0], "%.1lf", atof(j1_json->valuestring));
							sprintf(joint_value[1], "%.1lf", atof(j2_json->valuestring));
							sprintf(joint_value[2], "%.1lf", atof(j3_json->valuestring));
							sprintf(joint_value[3], "%.1lf", atof(j4_json->valuestring));
							sprintf(joint_value[4], "%.1lf", atof(j5_json->valuestring));
							sprintf(joint_value[5], "%.1lf", atof(j6_json->valuestring));

							if (strcmp(joint_value[0], arr[0]) == 0 && strcmp(joint_value[1], arr[1]) == 0 && strcmp(joint_value[2], arr[2]) == 0 && strcmp(joint_value[3], arr[3]) == 0 && strcmp(joint_value[4], arr[4]) == 0 && strcmp(joint_value[5], arr[5]) == 0) {

								ret = SUCCESS;
							}
						}
					}
				}
				/* 删除结点信息 */
				pthread_mutex_lock(&sock_cmd->ret_mute);
				dequene(&sock_cmd->ret_quene, p->data);
				pthread_mutex_unlock(&sock_cmd->ret_mute);

				return ret;
			}
			p = p->next;
		}
		usleep(1000);
	}

	return ret;
}

/**
  init WEBAPP_PORT, SERVER_IP, SERVER_PI_IP
*/
int init_network()
{
	//printf("before SERVER_IP = %s\n", SERVER_IP);
	FILE *fp = NULL;
	int i = 0;
	int j = 0;
	char strline[LINE_LEN] = { 0 };
	char *ptr = NULL;

	if ((fp = fopen(ROBOT_CFG, "r")) == NULL) {
		perror("file");
	
		return FAIL;
	}

	while (fgets(strline, LINE_LEN, fp) != NULL) {
		/* without '\n' '\r' '\t' */
		i = 0;
		j = 0;
		while (strline[i] != '\0') {
			if (strline[i] != '\n' && strline[i] != '\r' && strline[i] !='\t') {
				strline[j++] = strline[i];
			}
			i++;
		}
		strline[j] = '\0';

		if (ptr = strstr(strline, "CTRL_IP = ")) {
			memset(SERVER_IP, 0, 20);
			strcpy(SERVER_IP, (ptr + 10));
		} else if (ptr = strstr(strline, "PI_IP = ")) {
			memset(SERVER_PI_IP, 0, 20);
			strcpy(SERVER_PI_IP, (ptr + 8));
		} else if (ptr = strstr(strline, "WebAPP_Port = ")) {
			WEBAPP_PORT = atoi(ptr + 14);
		}
		bzero(strline, sizeof(char)*LINE_LEN);
	}
	fclose(fp);

	//printf("after SERVER_IP = %s\n", SERVER_IP);
	return SUCCESS;
}

int init_PI_cfg()
{
	char *config_content = NULL;
	cJSON *config_json = NULL;
	cJSON *enable = NULL;

	config_content = get_file_content(FILE_PI_CFG);
	if (config_content == NULL || strcmp(config_content, "NO_FILE") == 0 || strcmp(config_content, "Empty") == 0) {
		perror("get file content");

		return FAIL;
	}
	printf("config_content = %s\n", config_content);
	config_json = cJSON_Parse(config_content);
	free(config_content);
	config_content = NULL;
	if (config_json == NULL) {
		perror("cJSON_Parse");

		return FAIL;
	}
	enable = cJSON_GetObjectItem(config_json, "enable");
	if (enable == NULL) {
		perror("JSON");

		return FAIL;
	}

	pi_pt_status.enable = enable->valueint;
	pi_pt_cmd.enable = enable->valueint;

	/* 开启 PI socket thread */
	if (enable->valueint == 1) {
		printf("create PI socket thread\n");
		/* create socket_pi_status thread */
		if (pthread_create(&pi_pt_status.t_pi, NULL, (void *)&socket_pi_status_thread, (void *)PI_STATUS_PORT)) {
			perror("pthread_create");
		}
		/* create socket_pi_cmd thread */
		if (pthread_create(&pi_pt_cmd.t_pi, NULL, (void *)&socket_pi_cmd_thread, (void *)PI_CMD_PORT)) {
			perror("pthread_create");
		}
	}
	/* cjson delete */
	cJSON_Delete(config_json);
	config_json = NULL;

	return SUCCESS;
}

/**
	嘉宝:
	从生产数据数据库中读取生产数据，应用到系统变量中
	更新全局的结构体
*/
static int init_torque_production_data()
{
	char sql[SQL_LEN] = {0};
	char **resultp = NULL;
	int nrow = 0;
	int ncloumn = 0;
	char cmd_content[100] = "";
	SOCKET_INFO *sock_cmd = NULL;

	sprintf(sql, "select * from torquesys_pd_data;");
	if (select_info_sqlite3(DB_TORQUE_PDDATA, sql, &resultp, &nrow, &ncloumn) == -1) {

		return FAIL;
	}
	memset(jiabao_torque_pd_data.left_wk_id, 0, 100);
	strcpy(jiabao_torque_pd_data.left_wk_id, resultp[ncloumn]);
	jiabao_torque_pd_data.left_product_count = atoi(resultp[ncloumn + 1]);
	jiabao_torque_pd_data.left_NG_count = atoi(resultp[ncloumn + 2]);
	jiabao_torque_pd_data.left_work_time = atoi(resultp[ncloumn + 3]);
	memset(jiabao_torque_pd_data.right_wk_id, 0, 100);
	strcpy(jiabao_torque_pd_data.right_wk_id, resultp[ncloumn + 4]);
	jiabao_torque_pd_data.right_product_count = atoi(resultp[ncloumn + 5]);
	jiabao_torque_pd_data.right_NG_count = atoi(resultp[ncloumn + 6]);
	jiabao_torque_pd_data.right_work_time = atoi(resultp[ncloumn + 7]);

	if (robot_type == 1) { // "1" 代表实体机器人
		sock_cmd = &socket_cmd;
	} else { // "0" 代表虚拟机器人
		sock_cmd = &socket_vir_cmd;
	}
	/* 左工位系统变量更新 */
	memset(cmd_content, 0, 100);
	sprintf(cmd_content, "SetSysVarValue(12, %d)", atoi(resultp[ncloumn + 1]));
	socket_enquene(sock_cmd, 511, cmd_content, 1);
	memset(cmd_content, 0, 100);
	sprintf(cmd_content, "SetSysVarValue(13, %d)", atoi(resultp[ncloumn + 2]));
	socket_enquene(sock_cmd, 511, cmd_content, 1);
	memset(cmd_content, 0, 100);
	sprintf(cmd_content, "SetSysVarValue(14, %d)", atoi(resultp[ncloumn + 3]));
	socket_enquene(sock_cmd, 511, cmd_content, 1);

	/* 右工位系统变量更新 */
	memset(cmd_content, 0, 100);
	sprintf(cmd_content, "SetSysVarValue(17, %d)", atoi(resultp[ncloumn + 5]));
	socket_enquene(sock_cmd, 511, cmd_content, 1);
	memset(cmd_content, 0, 100);
	sprintf(cmd_content, "SetSysVarValue(18, %d)", atoi(resultp[ncloumn + 6]));
	socket_enquene(sock_cmd, 511, cmd_content, 1);
	memset(cmd_content, 0, 100);
	sprintf(cmd_content, "SetSysVarValue(19, %d)", atoi(resultp[ncloumn + 7]));
	socket_enquene(sock_cmd, 511, cmd_content, 1);

	sqlite3_free_table(resultp); //释放结果集

	return SUCCESS;
}

int init_db_json(DB_JSON *p_db_json)
{
	char sql[MAX_BUF] = {0};

	/* open and get point.db content */
	memset(sql, 0, MAX_BUF);
	sprintf(sql, "select * from points;");
	if (select_info_json_sqlite3(DB_POINTS, sql, &p_db_json->point) == -1) {
		perror("points sqlite3 database");

		return FAIL;
	}

	memset(sql, 0, MAX_BUF);
	sprintf(sql, "select * from coordinate_system;");
	if (select_info_json_sqlite3(DB_CDSYSTEM, sql, &p_db_json->cdsystem) == -1) {
		perror("cdsystem sqlite3 database");

		return FAIL;
	}

	memset(sql, 0, MAX_BUF);
	sprintf(sql, "select * from wobj_coordinate_system;");
	if (select_info_json_sqlite3(DB_WOBJ_CDSYSTEM, sql, &p_db_json->wobj_cdsystem) == -1) {
		perror("wobj_cdsystem sqlite3 database");

		return FAIL;
	}

	memset(sql, 0, MAX_BUF);
	sprintf(sql, "select * from et_coordinate_system;");
	if (select_info_json_sqlite3(DB_ET_CDSYSTEM, sql, &p_db_json->et_cdsystem) == -1) {
		perror("et_cdsystem sqlite3 database");

		return FAIL;
	}

	memset(sql, 0, MAX_BUF);
	sprintf(sql, "select * from sysvar;");
	if (select_info_json_sqlite3(DB_SYSVAR, sql, &p_db_json->sysvar) == -1) {
		perror("sysvar sqlite3 database");

		return FAIL;
	}

	return SUCCESS;
}

void db_json_delete(DB_JSON *p_db_json)
{
	cJSON_Delete(p_db_json->point);
	p_db_json->point = NULL;

	cJSON_Delete(p_db_json->cdsystem);
	p_db_json->cdsystem = NULL;

	cJSON_Delete(p_db_json->wobj_cdsystem);
	p_db_json->wobj_cdsystem = NULL;

	cJSON_Delete(p_db_json->et_cdsystem);
	p_db_json->et_cdsystem = NULL;

	cJSON_Delete(p_db_json->sysvar);
	p_db_json->sysvar = NULL;
}

/* parse cmd of lua file */
int parse_lua_cmd(char *lua_cmd, char *file_content, DB_JSON *p_db_json)
{
	//printf("lua cmd = %s\n", lua_cmd);
	char content[MAX_BUF] = { 0 }; // 存储 lua_cmd 解析转换后的内容
	char head[MAX_BUF] = { 0 }; // 存储 lua_cmd 中指令, 之前的字符串内容
	char *ptr = NULL;  // 指向 lua_cmd 中包含的指令开头的指针
	char *end_ptr = NULL; // 指向 lua_cmd 结尾的指针
	char cmd_arg[MAX_BUF] = { 0 }; // 存储 lua_cmd （） 中参数内容
	char joint_value[6][10] = { 0 };
	char **cmd_array = NULL;
	int size = 0;
	char *joint_value_ptr[6];
	int i = 0;

	for (i = 0; i < 6; i++) {
		joint_value_ptr[i] = NULL;
	}
	cJSON *id = NULL;

	cJSON *j1 = NULL;
	cJSON *j2 = NULL;
	cJSON *j3 = NULL;
	cJSON *j4 = NULL;
	cJSON *j5 = NULL;
	cJSON *j6 = NULL;
	cJSON *x = NULL;
	cJSON *y = NULL;
	cJSON *z = NULL;
	cJSON *rx = NULL;
	cJSON *ry = NULL;
	cJSON *rz = NULL;
	cJSON *ex = NULL;
	cJSON *ey = NULL;
	cJSON *ez = NULL;
	cJSON *erx = NULL;
	cJSON *ery = NULL;
	cJSON *erz = NULL;
	cJSON *tx = NULL;
	cJSON *ty = NULL;
	cJSON *tz = NULL;
	cJSON *trx = NULL;
	cJSON *try = NULL;
	cJSON *trz = NULL;
	cJSON *speed = NULL;
	cJSON *acc = NULL;
	cJSON *toolnum = NULL;
	cJSON *workpiecenum = NULL;
	cJSON *E1 = NULL;
	cJSON *E2 = NULL;
	cJSON *E3 = NULL;
	cJSON *E4 = NULL;

	cJSON *j1_2 = NULL;
	cJSON *j2_2 = NULL;
	cJSON *j3_2 = NULL;
	cJSON *j4_2 = NULL;
	cJSON *j5_2 = NULL;
	cJSON *j6_2 = NULL;
	cJSON *x_2 = NULL;
	cJSON *y_2 = NULL;
	cJSON *z_2 = NULL;
	cJSON *rx_2 = NULL;
	cJSON *ry_2 = NULL;
	cJSON *rz_2 = NULL;
	cJSON *speed_2 = NULL;
	cJSON *acc_2 = NULL;
	cJSON *toolnum_2 = NULL;
	cJSON *workpiecenum_2 = NULL;
	cJSON *E1_2 = NULL;
	cJSON *E2_2 = NULL;
	cJSON *E3_2 = NULL;
	cJSON *E4_2 = NULL;

	cJSON *j1_3 = NULL;
	cJSON *j2_3 = NULL;
	cJSON *j3_3 = NULL;
	cJSON *j4_3 = NULL;
	cJSON *j5_3 = NULL;
	cJSON *j6_3 = NULL;
	cJSON *x_3 = NULL;
	cJSON *y_3 = NULL;
	cJSON *z_3 = NULL;
	cJSON *rx_3 = NULL;
	cJSON *ry_3 = NULL;
	cJSON *rz_3 = NULL;
	cJSON *speed_3 = NULL;
	cJSON *acc_3 = NULL;
	cJSON *toolnum_3 = NULL;
	cJSON *workpiecenum_3 = NULL;
	cJSON *E1_3 = NULL;
	cJSON *E2_3 = NULL;
	cJSON *E3_3 = NULL;
	cJSON *E4_3 = NULL;

	cJSON *cd = NULL;
	cJSON *et_cd = NULL;
	cJSON *ptp = NULL;
	cJSON *lin = NULL;
	cJSON *point_1 = NULL;
	cJSON *point_2 = NULL;
	cJSON *point_3 = NULL;
	cJSON *ext_axis_ptp = NULL;
	cJSON *var = NULL;
	cJSON *installation_site = NULL;
	cJSON *type = NULL;

	/* 如果 lua 文件是旧版本格式，需要进行转换 */
	char old_head[MAX_BUF] = { 0 }; // 存储 lua_cmd 中":", 之前的字符串内容
	char *old_ptr = NULL;  // 指向 lua_cmd 中 ":" 之后的字符串指针
	char old_ptr_comment[MAX_BUF] = { 0 }; // 存储 lua_cmd 中 "：" 到 "--"之间的字符串内容
	char *old_comment = NULL; // 指向 lua_cmd 中 "--" 之后的字符串指针
	char new_lua_cmd[MAX_BUF] = { 0 }; // 存储转换后的 new_lua_cmd 字符串内容
	char new_lua_cmd_2[MAX_BUF] = { 0 }; // 存储转换后的 new_lua_cmd 字符串内容
	char new_lua_cmd_3[MAX_BUF] = { 0 }; // 存储转换后的 new_lua_cmd 字符串内容

	/** 删除 -- 之后的内容 */
	if (old_ptr = strstr(lua_cmd, "--")) {
		strncpy(new_lua_cmd_3, lua_cmd, (old_ptr - lua_cmd));
		lua_cmd = new_lua_cmd_3;
	}

	//兼容 V3.2.0 之前的旧版本，兼容性代码,未来可删除
	//printf("lua_cmd = %s\n", lua_cmd);
	if (old_ptr = strstr(lua_cmd, "WaitTime")) {
		strncpy(old_head, lua_cmd, (old_ptr - lua_cmd));
		sprintf(new_lua_cmd, "%sWaitMs%s", old_head, (old_ptr + 8));
		lua_cmd = new_lua_cmd;
		//printf("lua_cmd = %s\n", lua_cmd);
	} else if (old_ptr = strstr(lua_cmd, "EXT_AXIS_SETHOMING")) {
		strncpy(old_head, lua_cmd, (old_ptr - lua_cmd));
		sprintf(new_lua_cmd, "%sExtAxisSetHoming%s", old_head, (old_ptr + 18));
		lua_cmd = new_lua_cmd;
		//printf("lua_cmd = %s\n", lua_cmd);
	/* PTP 添加了平滑过度半径参数 */
	} else if (strstr(lua_cmd, "PTP") && (strstr(lua_cmd, "EXT_AXIS_PTP") == NULL) && (strstr(lua_cmd, "laserPTP") == NULL) && (strstr(lua_cmd, "SPTP") == NULL)) {
		if (string_to_string_list(lua_cmd, ",", &size, &cmd_array) == 0) {
			perror("string to string list");

			goto end;
		}
		//printf("size = %d\n", size);
		if (size == 3) {
			sprintf(new_lua_cmd, "%s,%s,0,%s", cmd_array[0], cmd_array[1], cmd_array[2]);
			lua_cmd = new_lua_cmd;
		}
		if (size == 9) {
			sprintf(new_lua_cmd, "%s,%s,0,%s,%s,%s,%s,%s,%s,%s", cmd_array[0], cmd_array[1], cmd_array[2], cmd_array[3], cmd_array[4], cmd_array[5], cmd_array[6], cmd_array[7], cmd_array[8]);
			lua_cmd = new_lua_cmd;
		}
		string_list_free(&cmd_array, size);
	//兼容 V3.3.4 之前的旧版本, 兼容性代码, 未来可删除
	/* Lin 添加了是否寻位参数 */
	} else if (strstr(lua_cmd, "Lin") && (strstr(lua_cmd, "laserLin") == NULL)) {
		if (string_to_string_list(lua_cmd, ",", &size, &cmd_array) == 0) {
			perror("string to string list");

			goto end;
		}
		if (size == 4 && (strstr(cmd_array[0], "cvrCatchPoint") == NULL) && (strstr(cmd_array[0], "cvrRaisePoint") == NULL)) {
			sprintf(new_lua_cmd, "%s,%s,%s,0,%s", cmd_array[0], cmd_array[1], cmd_array[2], cmd_array[3]);
			lua_cmd = new_lua_cmd;
		}
		if (size == 10) {
			sprintf(new_lua_cmd, "%s,%s,%s,0,%s,%s,%s,%s,%s,%s,%s", cmd_array[0], cmd_array[1], cmd_array[2], cmd_array[3], cmd_array[4], cmd_array[5], cmd_array[6], cmd_array[7], cmd_array[8], cmd_array[9]);
			lua_cmd = new_lua_cmd;
		}
		string_list_free(&cmd_array, size);
	}

	if ((old_ptr = strstr(lua_cmd, ":")) && (strstr(lua_cmd, "::") == NULL)) {
		//printf("old_ptr = %s\n", old_ptr);
		strncpy(old_head, lua_cmd, (old_ptr - lua_cmd));
		//printf("old_head = %s\n", old_head);
		//strcpy(old_end, (old_ptr + 1));
		/*
		if (old_comment = strstr(old_ptr, "--")) {
			//printf("old_comment = %s\n", old_comment);
			strncpy(old_ptr_comment, (old_ptr + 1), (old_comment - old_ptr - 1));
			//printf("old_ptr_comment = %s\n", old_ptr_comment);
			// 去掉字符串结尾多余空格
			//old_ptr_comment[strlen(old_ptr_comment)] = '\0';
			//printf("old_ptr_comment = %s\n", old_ptr_comment);
			sprintf(new_lua_cmd_2, "%s(%s)%s", old_head, old_ptr_comment, old_comment);
		} else {*/
			sprintf(new_lua_cmd_2, "%s(%s)", old_head, (old_ptr + 1));
		//}
		lua_cmd = new_lua_cmd_2;
		//printf("lua_cmd = %s\n", lua_cmd);
	}

	/* lua 文件中，不允许 pcall 的字样，需要检测，并报警提示 */
	if (strstr(lua_cmd, "pcall")) {
		sprintf(error_info, "pcall is not allowed in lua file");

		goto end;
	}

	/* laserPTP */
	if ((ptr = strstr(lua_cmd, "laserPTP(")) && strrchr(lua_cmd, ')')) {
		end_ptr = strrchr(lua_cmd, ')') + 1;
		strncpy(head, lua_cmd, (ptr - lua_cmd));
		strncpy(cmd_arg, (ptr + 9), (end_ptr - ptr - 10));
		if (string_to_string_list(cmd_arg, ",", &size, &cmd_array) == 0 || size != 2) {
			perror("string to string list");
			argc_error_info(2, "laserPTP");

			goto end;
		}
		sprintf(content, "%sMoveJ(%s,%s)%s\n", head, cmd_array[0], cmd_array[1], end_ptr);
		strcat(file_content, content);
	/* EXT_AXIS_PTP */
	} else if ((ptr = strstr(lua_cmd, "EXT_AXIS_PTP(")) && strrchr(lua_cmd, ')')) {
		end_ptr = strrchr(lua_cmd, ')') + 1;
		strncpy(head, lua_cmd, (ptr - lua_cmd));
		strncpy(cmd_arg, (ptr + 13), (end_ptr - ptr - 14));
		if (string_to_string_list(cmd_arg, ",", &size, &cmd_array) == 0 || size != 3) {
			perror("string to string list");
			argc_error_info(3, "EXT_AXIS_PTP");

			goto end;
		}
		if (strcmp(cmd_array[1], "seamPos") == 0) {
			sprintf(content,"%sExtAxisMoveJ(%s,\"%s\",%s)%s\n", head, cmd_array[0], cmd_array[1], cmd_array[2], end_ptr);
		} else {
			ext_axis_ptp = cJSON_GetObjectItemCaseSensitive(p_db_json->point, cmd_array[1]);
			if (ext_axis_ptp == NULL || ext_axis_ptp->type != cJSON_Object) {
				database_error_info();

				goto end;
			}
			E1 = cJSON_GetObjectItem(ext_axis_ptp, "E1");
			E2 = cJSON_GetObjectItem(ext_axis_ptp, "E2");
			E3 = cJSON_GetObjectItem(ext_axis_ptp, "E3");
			E4 = cJSON_GetObjectItem(ext_axis_ptp, "E4");
			if (E1 == NULL || E2 == NULL || E3 == NULL || E4 == NULL || cmd_array[0] == NULL || cmd_array[2] == NULL || E1->valuestring == NULL || E2->valuestring == NULL || E3->valuestring == NULL || E4->valuestring == NULL) {

				goto end;
			}
			sprintf(content,"%sExtAxisMoveJ(%s,%s,%s,%s,%s,%s)%s\n", head, cmd_array[0], E1->valuestring, E2->valuestring, E3->valuestring, E4->valuestring, cmd_array[2], end_ptr);
		}
		strcat(file_content, content);
	/* SPTP */
	} else if ((ptr = strstr(lua_cmd, "SPTP(")) && strrchr(lua_cmd, ')')) {
		end_ptr = strrchr(lua_cmd, ')') + 1;
		strncpy(head, lua_cmd, (ptr - lua_cmd));
		strncpy(cmd_arg, (ptr + 5), (end_ptr - ptr - 6));
		if (string_to_string_list(cmd_arg, ",", &size, &cmd_array) == 0 || size != 2) {
			perror("string to string list");
			argc_error_info(2, "SPTP");

			goto end;
		}
		point_1 = cJSON_GetObjectItemCaseSensitive(p_db_json->point, cmd_array[0]);
		if (point_1 == NULL || point_1->type != cJSON_Object) {
			database_error_info();

			goto end;
		}
		j1 = cJSON_GetObjectItem(point_1, "j1");
		j2 = cJSON_GetObjectItem(point_1, "j2");
		j3 = cJSON_GetObjectItem(point_1, "j3");
		j4 = cJSON_GetObjectItem(point_1, "j4");
		j5 = cJSON_GetObjectItem(point_1, "j5");
		j6 = cJSON_GetObjectItem(point_1, "j6");
		x = cJSON_GetObjectItem(point_1, "x");
		y = cJSON_GetObjectItem(point_1, "y");
		z = cJSON_GetObjectItem(point_1, "z");
		rx = cJSON_GetObjectItem(point_1, "rx");
		ry = cJSON_GetObjectItem(point_1, "ry");
		rz = cJSON_GetObjectItem(point_1, "rz");
		toolnum = cJSON_GetObjectItem(point_1, "toolnum");
		workpiecenum = cJSON_GetObjectItem(point_1, "workpiecenum");
		speed = cJSON_GetObjectItem(point_1, "speed");
		acc = cJSON_GetObjectItem(point_1, "acc");
		if (j1 == NULL || j2 == NULL || j3 == NULL || j4 == NULL || j5 == NULL || j6 == NULL || x == NULL || y == NULL || z == NULL || rx == NULL || ry == NULL || rz == NULL || toolnum == NULL || workpiecenum == NULL || speed == NULL || acc == NULL || j1->valuestring == NULL || j2->valuestring == NULL || j3->valuestring == NULL || j4->valuestring == NULL || j5->valuestring == NULL || j6->valuestring == NULL || x->valuestring == NULL || y->valuestring == NULL || z->valuestring == NULL || rx->valuestring == NULL || ry->valuestring == NULL || rz->valuestring == NULL || toolnum->valuestring == NULL || workpiecenum->valuestring == NULL || speed->valuestring == NULL || acc->valuestring == NULL || cmd_array[1] == NULL) {

			goto end;
		}
		/* 当点为 pHOME 原点时，进行检查 */
		if (strcmp(cmd_array[0], POINT_HOME) == 0) {
			sprintf(joint_value[0], "%.1lf", atof(j1->valuestring));
			sprintf(joint_value[1], "%.1lf", atof(j2->valuestring));
			sprintf(joint_value[2], "%.1lf", atof(j3->valuestring));
			sprintf(joint_value[3], "%.1lf", atof(j4->valuestring));
			sprintf(joint_value[4], "%.1lf", atof(j5->valuestring));
			sprintf(joint_value[5], "%.1lf", atof(j6->valuestring));
			for (i = 0; i < 6; i++) {
				joint_value_ptr[i] = joint_value[i];
			}
			/* 置异常报错的标志位， 添加异常错误到 sta 状态反馈 error_info 中 */
			if (check_pointhome_data(joint_value_ptr) == FAIL) {
				point_home_info.error_flag = 1;

				goto end;
			}
			point_home_info.error_flag = 0;
		}
		sprintf(content,"%sSplinePTP(%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s)%s\n", head, j1->valuestring, j2->valuestring, j3->valuestring, j4->valuestring, j5->valuestring,j6->valuestring, x->valuestring, y->valuestring, z->valuestring, rx->valuestring, ry->valuestring, rz->valuestring, toolnum->valuestring, workpiecenum->valuestring, speed->valuestring, acc->valuestring, cmd_array[1], end_ptr);
		strcat(file_content, content);
	/* NewSplinePoint */
	} else if ((ptr = strstr(lua_cmd, "NewSplinePoint(")) && strrchr(lua_cmd, ')')) {
		end_ptr = strrchr(lua_cmd, ')') + 1;
		strncpy(head, lua_cmd, (ptr - lua_cmd));
		strncpy(cmd_arg, (ptr + 15), (end_ptr - ptr - 16));
		if (string_to_string_list(cmd_arg, ",", &size, &cmd_array) == 0 || size != 1) {
			perror("string to string list");
			argc_error_info(1, "NewSplinePoint");

			goto end;
		}
		point_1 = cJSON_GetObjectItemCaseSensitive(p_db_json->point, cmd_array[0]);
		if (point_1 == NULL || point_1->type != cJSON_Object) {
			database_error_info();

			goto end;
		}
		j1 = cJSON_GetObjectItem(point_1, "j1");
		j2 = cJSON_GetObjectItem(point_1, "j2");
		j3 = cJSON_GetObjectItem(point_1, "j3");
		j4 = cJSON_GetObjectItem(point_1, "j4");
		j5 = cJSON_GetObjectItem(point_1, "j5");
		j6 = cJSON_GetObjectItem(point_1, "j6");
		x = cJSON_GetObjectItem(point_1, "x");
		y = cJSON_GetObjectItem(point_1, "y");
		z = cJSON_GetObjectItem(point_1, "z");
		rx = cJSON_GetObjectItem(point_1, "rx");
		ry = cJSON_GetObjectItem(point_1, "ry");
		rz = cJSON_GetObjectItem(point_1, "rz");
		toolnum = cJSON_GetObjectItem(point_1, "toolnum");
		workpiecenum = cJSON_GetObjectItem(point_1, "workpiecenum");
		speed = cJSON_GetObjectItem(point_1, "speed");
		acc = cJSON_GetObjectItem(point_1, "acc");
		if (j1 == NULL || j2 == NULL || j3 == NULL || j4 == NULL || j5 == NULL || j6 == NULL || x == NULL || y == NULL || z == NULL || rx == NULL || ry == NULL || rz == NULL || toolnum == NULL || workpiecenum == NULL || speed == NULL || acc == NULL || j1->valuestring == NULL || j2->valuestring == NULL || j3->valuestring == NULL || j4->valuestring == NULL || j5->valuestring == NULL || j6->valuestring == NULL || x->valuestring == NULL || y->valuestring == NULL || z->valuestring == NULL || rx->valuestring == NULL || ry->valuestring == NULL || rz->valuestring == NULL || toolnum->valuestring == NULL || workpiecenum->valuestring == NULL || speed->valuestring == NULL || acc->valuestring == NULL) {

			goto end;
		}
		/* 当点为 pHOME 原点时，进行检查 */
		if (strcmp(cmd_array[0], POINT_HOME) == 0) {
			sprintf(joint_value[0], "%.1lf", atof(j1->valuestring));
			sprintf(joint_value[1], "%.1lf", atof(j2->valuestring));
			sprintf(joint_value[2], "%.1lf", atof(j3->valuestring));
			sprintf(joint_value[3], "%.1lf", atof(j4->valuestring));
			sprintf(joint_value[4], "%.1lf", atof(j5->valuestring));
			sprintf(joint_value[5], "%.1lf", atof(j6->valuestring));
			for (i = 0; i < 6; i++) {
				joint_value_ptr[i] = joint_value[i];
			}
			/* 置异常报错的标志位， 添加异常错误到 sta 状态反馈 error_info 中 */
			if (check_pointhome_data(joint_value_ptr) == FAIL) {
				point_home_info.error_flag = 1;

				goto end;
			}
			point_home_info.error_flag = 0;
		}
		sprintf(content,"%sNewSplinePoint(%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s)%s\n", head, j1->valuestring, j2->valuestring, j3->valuestring, j4->valuestring, j5->valuestring,j6->valuestring, x->valuestring, y->valuestring, z->valuestring, rx->valuestring, ry->valuestring, rz->valuestring, toolnum->valuestring, workpiecenum->valuestring, speed->valuestring, acc->valuestring, end_ptr);
		strcat(file_content, content);
	/* PTP */
	} else if ((ptr = strstr(lua_cmd, "PTP(")) && strrchr(lua_cmd, ')') && strstr(lua_cmd, "SplinePTP") == NULL) {
		end_ptr = strrchr(lua_cmd, ')') + 1;
		strncpy(head, lua_cmd, (ptr - lua_cmd));
		strncpy(cmd_arg, (ptr + 4), (end_ptr - ptr - 5));
		if (string_to_string_list(cmd_arg, ",", &size, &cmd_array) == 0 || (size != 10 && size != 4)) {
			perror("string to string list");
			sprintf(error_info, "bad argument #%d #%d to '%s' (Error number of parameters)", 4, 10, "PTP");

			goto end;
		}
		ptp = cJSON_GetObjectItemCaseSensitive(p_db_json->point, cmd_array[0]);
		if (ptp == NULL || ptp->type != cJSON_Object) {
			database_error_info();

			goto end;
		}
		j1 = cJSON_GetObjectItem(ptp, "j1");
		j2 = cJSON_GetObjectItem(ptp, "j2");
		j3 = cJSON_GetObjectItem(ptp, "j3");
		j4 = cJSON_GetObjectItem(ptp, "j4");
		j5 = cJSON_GetObjectItem(ptp, "j5");
		j6 = cJSON_GetObjectItem(ptp, "j6");
		x = cJSON_GetObjectItem(ptp, "x");
		y = cJSON_GetObjectItem(ptp, "y");
		z = cJSON_GetObjectItem(ptp, "z");
		rx = cJSON_GetObjectItem(ptp, "rx");
		ry = cJSON_GetObjectItem(ptp, "ry");
		rz = cJSON_GetObjectItem(ptp, "rz");
		toolnum = cJSON_GetObjectItem(ptp, "toolnum");
		workpiecenum = cJSON_GetObjectItem(ptp, "workpiecenum");
		speed = cJSON_GetObjectItem(ptp, "speed");
		acc = cJSON_GetObjectItem(ptp, "acc");
		E1 = cJSON_GetObjectItem(ptp, "E1");
		E2 = cJSON_GetObjectItem(ptp, "E2");
		E3 = cJSON_GetObjectItem(ptp, "E3");
		E4 = cJSON_GetObjectItem(ptp, "E4");
		if (j1 == NULL || j2 == NULL || j3 == NULL || j4 == NULL || j5 == NULL || j6 == NULL || x == NULL || y == NULL || z == NULL || rx == NULL || ry == NULL || rz == NULL || toolnum == NULL || workpiecenum == NULL || speed == NULL || acc == NULL || E1 == NULL || E2 == NULL || E3 == NULL || E4 == NULL || j1->valuestring == NULL || j2->valuestring == NULL || j3->valuestring == NULL || j4->valuestring == NULL || j5->valuestring == NULL || j6->valuestring == NULL || x->valuestring == NULL || y->valuestring == NULL || z->valuestring == NULL || rx->valuestring == NULL || ry->valuestring == NULL || rz->valuestring == NULL || toolnum->valuestring == NULL || workpiecenum->valuestring == NULL || speed->valuestring == NULL || acc->valuestring == NULL || E1->valuestring == NULL || E2->valuestring == NULL || E3->valuestring == NULL || E4->valuestring == NULL) {

			goto end;
		}
		/* 当点为 pHOME 原点时，进行检查 */
		if (strcmp(cmd_array[0], POINT_HOME) == 0) {
			sprintf(joint_value[0], "%.1lf", atof(j1->valuestring));
			sprintf(joint_value[1], "%.1lf", atof(j2->valuestring));
			sprintf(joint_value[2], "%.1lf", atof(j3->valuestring));
			sprintf(joint_value[3], "%.1lf", atof(j4->valuestring));
			sprintf(joint_value[4], "%.1lf", atof(j5->valuestring));
			sprintf(joint_value[5], "%.1lf", atof(j6->valuestring));
			for (i = 0; i < 6; i++) {
				joint_value_ptr[i] = joint_value[i];
			}
			// 置异常报错的标志位， 添加异常错误到 sta 状态反馈 error_info 中
			if (check_pointhome_data(joint_value_ptr) == FAIL) {
				point_home_info.error_flag = 1;

				goto end;
			}
			point_home_info.error_flag = 0;
		}
		if (cmd_array[1] == NULL || cmd_array[2] == NULL || cmd_array[3] == NULL) {

			goto end;
		}
		/* 参数个数为 10 时，即存在偏移 */
		if (size == 10) {
			sprintf(content,"%sMoveJ(%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s)%s\n", head, j1->valuestring, j2->valuestring, j3->valuestring, j4->valuestring, j5->valuestring, j6->valuestring, x->valuestring, y->valuestring, z->valuestring, rx->valuestring, ry->valuestring, rz->valuestring, toolnum->valuestring, workpiecenum->valuestring, speed->valuestring, acc->valuestring, cmd_array[1], E1->valuestring, E2->valuestring, E3->valuestring, E4->valuestring, cmd_array[2], cmd_array[3], cmd_array[4], cmd_array[5], cmd_array[6], cmd_array[7], cmd_array[8], cmd_array[9], end_ptr);
		} else {
			sprintf(content,"%sMoveJ(%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,0,0,0,0,0,0)%s\n", head, j1->valuestring, j2->valuestring, j3->valuestring, j4->valuestring, j5->valuestring, j6->valuestring, x->valuestring, y->valuestring, z->valuestring, rx->valuestring, ry->valuestring, rz->valuestring, toolnum->valuestring, workpiecenum->valuestring, speed->valuestring, acc->valuestring, cmd_array[1], E1->valuestring, E2->valuestring, E3->valuestring, E4->valuestring, cmd_array[2], cmd_array[3], end_ptr);
		}
		strcat(file_content, content);
	/* laserLin */
	} else if ((ptr = strstr(lua_cmd, "laserLin(")) && strrchr(lua_cmd, ')')) {
		end_ptr = strrchr(lua_cmd, ')') + 1;
		strncpy(head, lua_cmd, (ptr - lua_cmd));
		strncpy(cmd_arg, (ptr + 9), (end_ptr - ptr - 10));
		if (string_to_string_list(cmd_arg, ",", &size, &cmd_array) == 0 || size != 2) {
			perror("string to string list");
			argc_error_info(2, "laserLin");

			goto end;
		}
		sprintf(content, "%sMoveL(%s,%s)%s\n", head, cmd_array[0], cmd_array[1], end_ptr);
		strcat(file_content, content);
	/* SLIN */
	} else if ((ptr = strstr(lua_cmd, "SLIN(")) && strrchr(lua_cmd, ')')) {
		end_ptr = strrchr(lua_cmd, ')') + 1;
		strncpy(head, lua_cmd, (ptr - lua_cmd));
		strncpy(cmd_arg, (ptr + 5), (end_ptr - ptr - 6));
		if (string_to_string_list(cmd_arg, ",", &size, &cmd_array) == 0 || size != 2) {
			perror("string to string list");
			argc_error_info(2, "SLIN");

			goto end;
		}
		point_1 = cJSON_GetObjectItemCaseSensitive(p_db_json->point, cmd_array[0]);
		if (point_1 == NULL || point_1->type != cJSON_Object) {
			database_error_info();

			goto end;
		}
		j1 = cJSON_GetObjectItem(point_1, "j1");
		j2 = cJSON_GetObjectItem(point_1, "j2");
		j3 = cJSON_GetObjectItem(point_1, "j3");
		j4 = cJSON_GetObjectItem(point_1, "j4");
		j5 = cJSON_GetObjectItem(point_1, "j5");
		j6 = cJSON_GetObjectItem(point_1, "j6");
		x = cJSON_GetObjectItem(point_1, "x");
		y = cJSON_GetObjectItem(point_1, "y");
		z = cJSON_GetObjectItem(point_1, "z");
		rx = cJSON_GetObjectItem(point_1, "rx");
		ry = cJSON_GetObjectItem(point_1, "ry");
		rz = cJSON_GetObjectItem(point_1, "rz");
		toolnum = cJSON_GetObjectItem(point_1, "toolnum");
		workpiecenum = cJSON_GetObjectItem(point_1, "workpiecenum");
		speed = cJSON_GetObjectItem(point_1, "speed");
		acc = cJSON_GetObjectItem(point_1, "acc");
		if (j1 == NULL || j2 == NULL || j3 == NULL || j4 == NULL || j5 == NULL || j6 == NULL || x == NULL || y == NULL || z == NULL || rx == NULL || ry == NULL || rz == NULL || toolnum == NULL || workpiecenum == NULL || speed == NULL || acc == NULL || j1->valuestring == NULL || j2->valuestring == NULL || j3->valuestring == NULL || j4->valuestring == NULL || j5->valuestring == NULL || j6->valuestring == NULL || x->valuestring == NULL || y->valuestring == NULL || z->valuestring == NULL || rx->valuestring == NULL || ry->valuestring == NULL || rz->valuestring == NULL || toolnum->valuestring == NULL || workpiecenum->valuestring == NULL || speed->valuestring == NULL || acc->valuestring == NULL || cmd_array[1] == NULL) {

			goto end;
		}
		/* 当点为 pHOME 原点时，进行检查 */
		if (strcmp(cmd_array[0], POINT_HOME) == 0) {
			sprintf(joint_value[0], "%.1lf", atof(j1->valuestring));
			sprintf(joint_value[1], "%.1lf", atof(j2->valuestring));
			sprintf(joint_value[2], "%.1lf", atof(j3->valuestring));
			sprintf(joint_value[3], "%.1lf", atof(j4->valuestring));
			sprintf(joint_value[4], "%.1lf", atof(j5->valuestring));
			sprintf(joint_value[5], "%.1lf", atof(j6->valuestring));
			for (i = 0; i < 6; i++) {
				joint_value_ptr[i] = joint_value[i];
			}
			/* 置异常报错的标志位， 添加异常错误到 sta 状态反馈 error_info 中 */
			if (check_pointhome_data(joint_value_ptr) == FAIL) {
				point_home_info.error_flag = 1;

				goto end;
			}
			point_home_info.error_flag = 0;
		}
		sprintf(content, "%sSplineLINE(%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s)%s\n", head, j1->valuestring, j2->valuestring, j3->valuestring, j4->valuestring, j5->valuestring, j6->valuestring, x->valuestring, y->valuestring, z->valuestring, rx->valuestring, ry->valuestring, rz->valuestring, toolnum->valuestring, workpiecenum->valuestring, speed->valuestring, acc->valuestring, cmd_array[1], end_ptr);
		strcat(file_content, content);
	/* Lin */
	} else if ((ptr = strstr(lua_cmd, "Lin(")) && strrchr(lua_cmd, ')')) {
		end_ptr = strrchr(lua_cmd, ')') + 1;
		/*
		printf("end_ptr = %s\n", end_ptr);
		printf("ptr = %s\n", ptr);
		printf("ptr - lua_cmd = %d\n", (ptr - lua_cmd));
		*/
		strncpy(head, lua_cmd, (ptr - lua_cmd));
		/*
		printf("head = %s\n", head);
		printf("end_ptr - ptr - 5 = %d\n", (end_ptr - ptr - 5));
		printf("ptr + 4 = %s\n", (ptr + 4));
		*/
		strncpy(cmd_arg, (ptr + 4), (end_ptr - ptr - 5));
		if (string_to_string_list(cmd_arg, ",", &size, &cmd_array) == 0 || (size != 4 && size != 5 && size != 6 && size != 11)) {
			perror("string to string list");
			sprintf(error_info, "bad argument #%d #%d #%d #%d to '%s' (Error number of parameters)", 4, 5, 6, 11, "Lin");

			goto end;
		}
		lin = cJSON_GetObjectItemCaseSensitive(p_db_json->point, cmd_array[0]);
		if (lin == NULL || lin->type != cJSON_Object) {
			database_error_info();

			goto end;
		}
		/* seamPos 下发参数为 6 个, 第 6 个参数为偏置，暂时不处理 */
		if (strcmp(cmd_array[0], "seamPos") == 0 && size == 6) {
			speed = cJSON_GetObjectItem(lin, "speed");
			acc = cJSON_GetObjectItem(lin, "acc");
			toolnum = cJSON_GetObjectItem(lin, "toolnum");
			workpiecenum = cJSON_GetObjectItem(lin, "workpiecenum");
			if (toolnum == NULL || workpiecenum == NULL || speed == NULL || acc == NULL || toolnum->valuestring == NULL || workpiecenum->valuestring == NULL || speed->valuestring == NULL || acc->valuestring == NULL || cmd_array[1] == NULL) {

				goto end;
			}
			sprintf(content, "%sMoveL(\"%s\",%s,%s,%s,%s,%s,%s,%s,%s)%s\n", head, cmd_array[0], toolnum->valuestring, workpiecenum->valuestring, speed->valuestring, acc->valuestring, cmd_array[1], cmd_array[2], cmd_array[3], cmd_array[4], end_ptr);
		/* cvrCatchPoint 和 cvrRaisePoint 下发参数为 4 个, 第 4 个参数为偏置，暂时不处理 */
		} else if ((strcmp(cmd_array[0], "cvrCatchPoint") == 0 || strcmp(cmd_array[0], "cvrRaisePoint") == 0) && size == 4) {
			speed = cJSON_GetObjectItem(lin, "speed");
			acc = cJSON_GetObjectItem(lin, "acc");
			toolnum = cJSON_GetObjectItem(lin, "toolnum");
			workpiecenum = cJSON_GetObjectItem(lin, "workpiecenum");
			if (toolnum == NULL || workpiecenum == NULL || speed == NULL || acc == NULL || toolnum->valuestring == NULL || workpiecenum->valuestring == NULL || speed->valuestring == NULL || acc->valuestring == NULL || cmd_array[1] == NULL) {

				goto end;
			}
			sprintf(content, "%sMoveL(\"%s\",%s,%s,%s,%s,%s,%s,0,0)%s\n", head, cmd_array[0], toolnum->valuestring, workpiecenum->valuestring, speed->valuestring, acc->valuestring, cmd_array[1], cmd_array[2], end_ptr);
		/* 正常 Lin 指令下发参数为 5 或者 11 个, 第 5 个参数为偏置位 */
		} else if (size == 5 || size == 11) {
			j1 = cJSON_GetObjectItem(lin, "j1");
			j2 = cJSON_GetObjectItem(lin, "j2");
			j3 = cJSON_GetObjectItem(lin, "j3");
			j4 = cJSON_GetObjectItem(lin, "j4");
			j5 = cJSON_GetObjectItem(lin, "j5");
			j6 = cJSON_GetObjectItem(lin, "j6");
			x = cJSON_GetObjectItem(lin, "x");
			y = cJSON_GetObjectItem(lin, "y");
			z = cJSON_GetObjectItem(lin, "z");
			rx = cJSON_GetObjectItem(lin, "rx");
			ry = cJSON_GetObjectItem(lin, "ry");
			rz = cJSON_GetObjectItem(lin, "rz");
			speed = cJSON_GetObjectItem(lin, "speed");
			acc = cJSON_GetObjectItem(lin, "acc");
			toolnum = cJSON_GetObjectItem(lin, "toolnum");
			workpiecenum = cJSON_GetObjectItem(lin, "workpiecenum");
			E1 = cJSON_GetObjectItem(lin, "E1");
			E2 = cJSON_GetObjectItem(lin, "E2");
			E3 = cJSON_GetObjectItem(lin, "E3");
			E4 = cJSON_GetObjectItem(lin, "E4");
			if (j1 == NULL || j2 == NULL || j3 == NULL || j4 == NULL || j5 == NULL || j6 == NULL || x == NULL || y == NULL || z == NULL || rx == NULL || ry == NULL || rz == NULL || toolnum == NULL || workpiecenum == NULL || speed == NULL || acc == NULL || E1 == NULL || E2 == NULL || E3 == NULL || E4 == NULL || j1->valuestring == NULL || j2->valuestring == NULL || j3->valuestring == NULL || j4->valuestring == NULL || j5->valuestring == NULL || j6->valuestring == NULL || x->valuestring == NULL || y->valuestring == NULL || z->valuestring == NULL || rx->valuestring == NULL || ry->valuestring == NULL || rz->valuestring == NULL || toolnum->valuestring == NULL || workpiecenum->valuestring == NULL || speed->valuestring == NULL || acc->valuestring == NULL || cmd_array[1] == NULL || cmd_array[2] == NULL || E1->valuestring == NULL || E2->valuestring == NULL || E3->valuestring == NULL || E4->valuestring == NULL) {

				goto end;
			}
			/* 当点为 pHOME 原点时，进行检查 */
			if (strcmp(cmd_array[0], POINT_HOME) == 0) {
				sprintf(joint_value[0], "%.1lf", atof(j1->valuestring));
				sprintf(joint_value[1], "%.1lf", atof(j2->valuestring));
				sprintf(joint_value[2], "%.1lf", atof(j3->valuestring));
				sprintf(joint_value[3], "%.1lf", atof(j4->valuestring));
				sprintf(joint_value[4], "%.1lf", atof(j5->valuestring));
				sprintf(joint_value[5], "%.1lf", atof(j6->valuestring));
				for (i = 0; i < 6; i++) {
					joint_value_ptr[i] = joint_value[i];
				}
				/* 置异常报错的标志位， 添加异常错误到 sta 状态反馈 error_info 中 */
				if (check_pointhome_data(joint_value_ptr) == FAIL) {
					point_home_info.error_flag = 1;

					goto end;
				}
				point_home_info.error_flag = 0;
			}
			/* 参数个数为 11 时，即存在偏移 */
			if (size == 11) {
				sprintf(content, "%sMoveL(%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s)%s\n", head, j1->valuestring, j2->valuestring, j3->valuestring, j4->valuestring, j5->valuestring, j6->valuestring, x->valuestring, y->valuestring, z->valuestring, rx->valuestring, ry->valuestring, rz->valuestring, toolnum->valuestring, workpiecenum->valuestring, speed->valuestring, acc->valuestring, cmd_array[1], cmd_array[2], E1->valuestring, E2->valuestring, E3->valuestring, E4->valuestring, cmd_array[3], cmd_array[4], cmd_array[5], cmd_array[6], cmd_array[7], cmd_array[8], cmd_array[9], cmd_array[10], end_ptr);
			} else {
				sprintf(content, "%sMoveL(%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,0,0,0,0,0,0)%s\n", head, j1->valuestring, j2->valuestring, j3->valuestring, j4->valuestring, j5->valuestring, j6->valuestring, x->valuestring, y->valuestring, z->valuestring, rx->valuestring, ry->valuestring, rz->valuestring, toolnum->valuestring, workpiecenum->valuestring, speed->valuestring, acc->valuestring, cmd_array[1], cmd_array[2], E1->valuestring, E2->valuestring, E3->valuestring, E4->valuestring, cmd_array[3], cmd_array[4], end_ptr);
			}
		}
		strcat(file_content, content);
	/* laserARC */
	} else if ((ptr = strstr(lua_cmd, "laserARC(")) && strrchr(lua_cmd, ')')) {
		end_ptr = strrchr(lua_cmd, ')') + 1;
		strncpy(head, lua_cmd, (ptr - lua_cmd));
		strncpy(cmd_arg, (ptr + 9), (end_ptr - ptr - 10));
		if (string_to_string_list(cmd_arg, ",", &size, &cmd_array) == 0 || size != 4) {
			perror("string to string list");
			argc_error_info(4, "laserARC");

			goto end;
		}
		sprintf(content, "%sMoveC(%s,%s,%s,%s)%s\n", head, cmd_array[0], cmd_array[1], cmd_array[2], cmd_array[3], end_ptr);
		strcat(file_content, content);
	/* ARC */
	} else if ((ptr = strstr(lua_cmd, "ARC(")) && strrchr(lua_cmd, ')')) {
		end_ptr = strrchr(lua_cmd, ')') + 1;
		strncpy(head, lua_cmd, (ptr - lua_cmd));
		strncpy(cmd_arg, (ptr + 4), (end_ptr - ptr - 5));
		if (string_to_string_list(cmd_arg, ",", &size, &cmd_array) == 0 || (size != 5 && size != 11)) {
			perror("string to string list");
			sprintf(error_info, "bad argument #%d #%d to '%s' (Error number of parameters)", 5, 11, "ARC");

			goto end;
		}
		if (cmd_array[0] == NULL) {

			goto end;
		}
		point_1 = cJSON_GetObjectItemCaseSensitive(p_db_json->point, cmd_array[0]);
		if (point_1 == NULL || point_1->type != cJSON_Object) {
			database_error_info();

			goto end;
		}
		j1 = cJSON_GetObjectItem(point_1, "j1");
		j2 = cJSON_GetObjectItem(point_1, "j2");
		j3 = cJSON_GetObjectItem(point_1, "j3");
		j4 = cJSON_GetObjectItem(point_1, "j4");
		j5 = cJSON_GetObjectItem(point_1, "j5");
		j6 = cJSON_GetObjectItem(point_1, "j6");
		x = cJSON_GetObjectItem(point_1, "x");
		y = cJSON_GetObjectItem(point_1, "y");
		z = cJSON_GetObjectItem(point_1, "z");
		rx = cJSON_GetObjectItem(point_1, "rx");
		ry = cJSON_GetObjectItem(point_1, "ry");
		rz = cJSON_GetObjectItem(point_1, "rz");
		toolnum = cJSON_GetObjectItem(point_1, "toolnum");
		workpiecenum = cJSON_GetObjectItem(point_1, "workpiecenum");
		speed = cJSON_GetObjectItem(point_1, "speed");
		acc = cJSON_GetObjectItem(point_1, "acc");
		E1 = cJSON_GetObjectItem(point_1, "E1");
		E2 = cJSON_GetObjectItem(point_1, "E2");
		E3 = cJSON_GetObjectItem(point_1, "E3");
		E4 = cJSON_GetObjectItem(point_1, "E4");
		if (j1 == NULL || j2 == NULL || j3 == NULL || j4 == NULL || j5 == NULL || j6 == NULL || x == NULL || y == NULL || z == NULL || rx == NULL || ry == NULL || rz == NULL || toolnum == NULL || workpiecenum == NULL || speed == NULL || acc == NULL || j1->valuestring == NULL || j2->valuestring == NULL || j3->valuestring == NULL || j4->valuestring == NULL || j5->valuestring == NULL || j6->valuestring == NULL || x->valuestring == NULL || y->valuestring == NULL || z->valuestring == NULL || rx->valuestring == NULL || ry->valuestring == NULL || rz->valuestring == NULL || toolnum->valuestring == NULL || workpiecenum->valuestring == NULL || speed->valuestring == NULL || acc->valuestring == NULL || E1->valuestring == NULL || E2->valuestring == NULL || E3->valuestring == NULL || E4->valuestring == NULL) {

			goto end;
		}
		/* 当点为 pHOME 原点时，进行检查 */
		if (strcmp(cmd_array[0], POINT_HOME) == 0) {
			sprintf(joint_value[0], "%.1lf", atof(j1->valuestring));
			sprintf(joint_value[1], "%.1lf", atof(j2->valuestring));
			sprintf(joint_value[2], "%.1lf", atof(j3->valuestring));
			sprintf(joint_value[3], "%.1lf", atof(j4->valuestring));
			sprintf(joint_value[4], "%.1lf", atof(j5->valuestring));
			sprintf(joint_value[5], "%.1lf", atof(j6->valuestring));
			for (i = 0; i < 6; i++) {
				joint_value_ptr[i] = joint_value[i];
			}
			/* 置异常报错的标志位，添加异常错误到 sta 状态反馈 error_info 中 */
			if (check_pointhome_data(joint_value_ptr) == FAIL) {
				point_home_info.error_flag = 1;

				goto end;
			}
			point_home_info.error_flag = 0;
		}
		if (cmd_array[1] == NULL) {

			goto end;
		}
		point_2 = cJSON_GetObjectItemCaseSensitive(p_db_json->point, cmd_array[1]);
		if (point_2 == NULL || point_2->type != cJSON_Object) {
			database_error_info();

			goto end;
		}
		j1_2 = cJSON_GetObjectItem(point_2, "j1");
		j2_2 = cJSON_GetObjectItem(point_2, "j2");
		j3_2 = cJSON_GetObjectItem(point_2, "j3");
		j4_2 = cJSON_GetObjectItem(point_2, "j4");
		j5_2 = cJSON_GetObjectItem(point_2, "j5");
		j6_2 = cJSON_GetObjectItem(point_2, "j6");
		x_2 = cJSON_GetObjectItem(point_2, "x");
		y_2 = cJSON_GetObjectItem(point_2, "y");
		z_2 = cJSON_GetObjectItem(point_2, "z");
		rx_2 = cJSON_GetObjectItem(point_2, "rx");
		ry_2 = cJSON_GetObjectItem(point_2, "ry");
		rz_2 = cJSON_GetObjectItem(point_2, "rz");
		toolnum_2 = cJSON_GetObjectItem(point_2, "toolnum");
		workpiecenum_2 = cJSON_GetObjectItem(point_2, "workpiecenum");
		speed_2 = cJSON_GetObjectItem(point_2, "speed");
		acc_2 = cJSON_GetObjectItem(point_2, "acc");
		E1_2 = cJSON_GetObjectItem(point_2, "E1");
		E2_2 = cJSON_GetObjectItem(point_2, "E2");
		E3_2 = cJSON_GetObjectItem(point_2, "E3");
		E4_2 = cJSON_GetObjectItem(point_2, "E4");
		if (j1_2 == NULL || j2_2 == NULL || j3_2 == NULL || j4_2 == NULL || j5_2 == NULL || j6_2 == NULL || x_2 == NULL || y_2 == NULL || z_2 == NULL || rx_2 == NULL || ry_2 == NULL || rz_2 == NULL || toolnum_2 == NULL || workpiecenum_2 == NULL || speed_2 == NULL || acc_2 == NULL || E1_2 == NULL || E2_2 == NULL || E3_2 == NULL || E4_2 == NULL || j1_2->valuestring == NULL || j2_2->valuestring == NULL || j3_2->valuestring == NULL || j4_2->valuestring == NULL || j5_2->valuestring == NULL || j6_2->valuestring == NULL || x_2->valuestring == NULL || y_2->valuestring == NULL || z_2->valuestring == NULL || rx_2->valuestring == NULL || ry_2->valuestring == NULL || rz_2->valuestring == NULL || toolnum_2->valuestring == NULL || workpiecenum_2->valuestring == NULL || speed_2->valuestring == NULL || acc_2->valuestring == NULL || E1_2->valuestring == NULL || E2_2->valuestring == NULL || E3_2->valuestring == NULL || E4_2->valuestring == NULL) {

			goto end;
		}
		/* 当点为 pHOME 原点时，进行检查 */
		if (strcmp(cmd_array[1], POINT_HOME) == 0) {
			for (i = 0; i < 6; i++) {
				memset(joint_value[i], 0, 10);
			}
			sprintf(joint_value[0], "%.1lf", atof(j1_2->valuestring));
			sprintf(joint_value[1], "%.1lf", atof(j2_2->valuestring));
			sprintf(joint_value[2], "%.1lf", atof(j3_2->valuestring));
			sprintf(joint_value[3], "%.1lf", atof(j4_2->valuestring));
			sprintf(joint_value[4], "%.1lf", atof(j5_2->valuestring));
			sprintf(joint_value[5], "%.1lf", atof(j6_2->valuestring));
			for (i = 0; i < 6; i++) {
				joint_value_ptr[i] = joint_value[i];
			}
			/* 置异常报错的标志位， 添加异常错误到 sta 状态反馈 error_info 中 */
			if (check_pointhome_data(joint_value_ptr) == FAIL) {
				point_home_info.error_flag = 1;

				goto end;
			}
			point_home_info.error_flag = 0;
		}
		if (cmd_array[2] == NULL || cmd_array[3] == NULL || cmd_array[4] == NULL) {

			goto end;
		}
		/* 参数个数为 11 时，即存在偏移 */
		if (size == 11) {
			sprintf(content, "%sMoveC(%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s)%s\n", head, j1->valuestring, j2->valuestring, j3->valuestring, j4->valuestring, j5->valuestring, j6->valuestring, x->valuestring, y->valuestring, z->valuestring, rx->valuestring, ry->valuestring, rz->valuestring, toolnum->valuestring, workpiecenum->valuestring, speed->valuestring, acc->valuestring, E1->valuestring, E2->valuestring, E3->valuestring, E4->valuestring, j1_2->valuestring, j2_2->valuestring, j3_2->valuestring, j4_2->valuestring, j5_2->valuestring, j6_2->valuestring, x_2->valuestring, y_2->valuestring, z_2->valuestring, rx_2->valuestring, ry_2->valuestring, rz_2->valuestring, toolnum_2->valuestring, workpiecenum_2->valuestring, speed_2->valuestring, acc_2->valuestring, E1_2->valuestring, E2_2->valuestring, E3_2->valuestring, E4_2->valuestring, cmd_array[2], cmd_array[3], cmd_array[4], cmd_array[5], cmd_array[6], cmd_array[7], cmd_array[8], cmd_array[9], cmd_array[10], end_ptr);
		/* 参数个数为 5 时，即不存在偏移 */
		} else {
			sprintf(content, "%sMoveC(%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,0,0,0,0,0,0)%s\n", head, j1->valuestring, j2->valuestring, j3->valuestring, j4->valuestring, j5->valuestring, j6->valuestring, x->valuestring, y->valuestring, z->valuestring, rx->valuestring, ry->valuestring, rz->valuestring, toolnum->valuestring, workpiecenum->valuestring, speed->valuestring, acc->valuestring, E1->valuestring, E2->valuestring, E3->valuestring, E4->valuestring, j1_2->valuestring, j2_2->valuestring, j3_2->valuestring, j4_2->valuestring, j5_2->valuestring, j6_2->valuestring, x_2->valuestring, y_2->valuestring, z_2->valuestring, rx_2->valuestring, ry_2->valuestring, rz_2->valuestring, toolnum_2->valuestring, workpiecenum_2->valuestring, speed_2->valuestring, acc_2->valuestring, E1_2->valuestring, E2_2->valuestring, E3_2->valuestring, E4_2->valuestring, cmd_array[2], cmd_array[3], cmd_array[4], end_ptr);
		}
		strcat(file_content, content);
	/* laserCircle */
	} else if ((ptr = strstr(lua_cmd, "laserCircle(")) && strrchr(lua_cmd, ')')) {
		end_ptr = strrchr(lua_cmd, ')') + 1;
		strncpy(head, lua_cmd, (ptr - lua_cmd));
		strncpy(cmd_arg, (ptr + 12), (end_ptr - ptr - 13));
		if (string_to_string_list(cmd_arg, ",", &size, &cmd_array) == 0 || size != 4) {
			perror("string to string list");
			argc_error_info(4, "laserCircle");

			goto end;
		}
		sprintf(content, "%sCircle(%s,%s,%s,%s)%s\n", head, cmd_array[0], cmd_array[1], cmd_array[2], cmd_array[3], end_ptr);
		strcat(file_content, content);
	/* Circle */
	} else if ((ptr = strstr(lua_cmd, "Circle(")) && strrchr(lua_cmd, ')')) {
		end_ptr = strrchr(lua_cmd, ')') + 1;
		strncpy(head, lua_cmd, (ptr - lua_cmd));
		strncpy(cmd_arg, (ptr + 7), (end_ptr - ptr - 8));
		if (string_to_string_list(cmd_arg, ",", &size, &cmd_array) == 0 || (size != 4 && size != 10)) {
			perror("string to string list");
			sprintf(error_info, "bad argument #%d #%d to '%s' (Error number of parameters)", 4, 10, "Circle");

			goto end;
		}
		if (cmd_array[0] == NULL) {

			goto end;
		}
		point_1 = cJSON_GetObjectItemCaseSensitive(p_db_json->point, cmd_array[0]);
		if (point_1 == NULL || point_1->type != cJSON_Object) {
			database_error_info();

			goto end;
		}
		j1 = cJSON_GetObjectItem(point_1, "j1");
		j2 = cJSON_GetObjectItem(point_1, "j2");
		j3 = cJSON_GetObjectItem(point_1, "j3");
		j4 = cJSON_GetObjectItem(point_1, "j4");
		j5 = cJSON_GetObjectItem(point_1, "j5");
		j6 = cJSON_GetObjectItem(point_1, "j6");
		x = cJSON_GetObjectItem(point_1, "x");
		y = cJSON_GetObjectItem(point_1, "y");
		z = cJSON_GetObjectItem(point_1, "z");
		rx = cJSON_GetObjectItem(point_1, "rx");
		ry = cJSON_GetObjectItem(point_1, "ry");
		rz = cJSON_GetObjectItem(point_1, "rz");
		toolnum = cJSON_GetObjectItem(point_1, "toolnum");
		workpiecenum = cJSON_GetObjectItem(point_1, "workpiecenum");
		speed = cJSON_GetObjectItem(point_1, "speed");
		acc = cJSON_GetObjectItem(point_1, "acc");
		E1 = cJSON_GetObjectItem(point_1, "E1");
		E2 = cJSON_GetObjectItem(point_1, "E2");
		E3 = cJSON_GetObjectItem(point_1, "E3");
		E4 = cJSON_GetObjectItem(point_1, "E4");
		if (j1 == NULL || j2 == NULL || j3 == NULL || j4 == NULL || j5 == NULL || j6 == NULL || x == NULL || y == NULL || z == NULL || rx == NULL || ry == NULL || rz == NULL || toolnum == NULL || workpiecenum == NULL || speed == NULL || acc == NULL || j1->valuestring == NULL || j2->valuestring == NULL || j3->valuestring == NULL || j4->valuestring == NULL || j5->valuestring == NULL || j6->valuestring == NULL || x->valuestring == NULL || y->valuestring == NULL || z->valuestring == NULL || rx->valuestring == NULL || ry->valuestring == NULL || rz->valuestring == NULL || toolnum->valuestring == NULL || workpiecenum->valuestring == NULL || speed->valuestring == NULL || acc->valuestring == NULL || E1->valuestring == NULL || E2->valuestring == NULL || E3->valuestring == NULL || E4->valuestring == NULL) {

			goto end;
		}
		/* 当点为 pHOME 原点时，进行检查 */
		if (strcmp(cmd_array[0], POINT_HOME) == 0) {
			sprintf(joint_value[0], "%.1lf", atof(j1->valuestring));
			sprintf(joint_value[1], "%.1lf", atof(j2->valuestring));
			sprintf(joint_value[2], "%.1lf", atof(j3->valuestring));
			sprintf(joint_value[3], "%.1lf", atof(j4->valuestring));
			sprintf(joint_value[4], "%.1lf", atof(j5->valuestring));
			sprintf(joint_value[5], "%.1lf", atof(j6->valuestring));
			for (i = 0; i < 6; i++) {
				joint_value_ptr[i] = joint_value[i];
			}
			/* 置异常报错的标志位，添加异常错误到 sta 状态反馈 error_info 中 */
			if (check_pointhome_data(joint_value_ptr) == FAIL) {
				point_home_info.error_flag = 1;

				goto end;
			}
			point_home_info.error_flag = 0;
		}
		if (cmd_array[1] == NULL) {

			goto end;
		}
		point_2 = cJSON_GetObjectItemCaseSensitive(p_db_json->point, cmd_array[1]);
		if (point_2 == NULL || point_2->type != cJSON_Object) {
			database_error_info();

			goto end;
		}
		j1_2 = cJSON_GetObjectItem(point_2, "j1");
		j2_2 = cJSON_GetObjectItem(point_2, "j2");
		j3_2 = cJSON_GetObjectItem(point_2, "j3");
		j4_2 = cJSON_GetObjectItem(point_2, "j4");
		j5_2 = cJSON_GetObjectItem(point_2, "j5");
		j6_2 = cJSON_GetObjectItem(point_2, "j6");
		x_2 = cJSON_GetObjectItem(point_2, "x");
		y_2 = cJSON_GetObjectItem(point_2, "y");
		z_2 = cJSON_GetObjectItem(point_2, "z");
		rx_2 = cJSON_GetObjectItem(point_2, "rx");
		ry_2 = cJSON_GetObjectItem(point_2, "ry");
		rz_2 = cJSON_GetObjectItem(point_2, "rz");
		toolnum_2 = cJSON_GetObjectItem(point_2, "toolnum");
		workpiecenum_2 = cJSON_GetObjectItem(point_2, "workpiecenum");
		speed_2 = cJSON_GetObjectItem(point_2, "speed");
		acc_2 = cJSON_GetObjectItem(point_2, "acc");
		E1_2 = cJSON_GetObjectItem(point_2, "E1");
		E2_2 = cJSON_GetObjectItem(point_2, "E2");
		E3_2 = cJSON_GetObjectItem(point_2, "E3");
		E4_2 = cJSON_GetObjectItem(point_2, "E4");
		if (j1_2 == NULL || j2_2 == NULL || j3_2 == NULL || j4_2 == NULL || j5_2 == NULL || j6_2 == NULL || x_2 == NULL || y_2 == NULL || z_2 == NULL || rx_2 == NULL || ry_2 == NULL || rz_2 == NULL || toolnum_2 == NULL || workpiecenum_2 == NULL || speed_2 == NULL || acc_2 == NULL || E1_2 == NULL || E2_2 == NULL || E3_2 == NULL || E4_2 == NULL || j1_2->valuestring == NULL || j2_2->valuestring == NULL || j3_2->valuestring == NULL || j4_2->valuestring == NULL || j5_2->valuestring == NULL || j6_2->valuestring == NULL || x_2->valuestring == NULL || y_2->valuestring == NULL || z_2->valuestring == NULL || rx_2->valuestring == NULL || ry_2->valuestring == NULL || rz_2->valuestring == NULL || toolnum_2->valuestring == NULL || workpiecenum_2->valuestring == NULL || speed_2->valuestring == NULL || acc_2->valuestring == NULL || E1_2->valuestring == NULL || E2_2->valuestring == NULL || E3_2->valuestring == NULL || E4_2->valuestring == NULL) {

			goto end;
		}
		/* 当点为 pHOME 原点时，进行检查 */
		if (strcmp(cmd_array[1], POINT_HOME) == 0) {
			for (i = 0; i < 6; i++) {
				memset(joint_value[i], 0, 10);
			}
			sprintf(joint_value[0], "%.1lf", atof(j1_2->valuestring));
			sprintf(joint_value[1], "%.1lf", atof(j2_2->valuestring));
			sprintf(joint_value[2], "%.1lf", atof(j3_2->valuestring));
			sprintf(joint_value[3], "%.1lf", atof(j4_2->valuestring));
			sprintf(joint_value[4], "%.1lf", atof(j5_2->valuestring));
			sprintf(joint_value[5], "%.1lf", atof(j6_2->valuestring));
			for (i = 0; i < 6; i++) {
				joint_value_ptr[i] = joint_value[i];
			}
			/* 置异常报错的标志位， 添加异常错误到 sta 状态反馈 error_info 中 */
			if (check_pointhome_data(joint_value_ptr) == FAIL) {
				point_home_info.error_flag = 1;

				goto end;
			}
			point_home_info.error_flag = 0;
		}
		if (cmd_array[2] == NULL) {

			goto end;
		}
		/* 参数个数为 10 时，即存在偏移 */
		if (size == 10) {
			sprintf(content, "%sCircle(%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s)%s\n", head, j1->valuestring, j2->valuestring, j3->valuestring, j4->valuestring, j5->valuestring, j6->valuestring, x->valuestring, y->valuestring, z->valuestring, rx->valuestring, ry->valuestring, rz->valuestring, toolnum->valuestring, workpiecenum->valuestring, speed->valuestring, acc->valuestring, E1->valuestring, E2->valuestring, E3->valuestring, E4->valuestring, j1_2->valuestring, j2_2->valuestring, j3_2->valuestring, j4_2->valuestring, j5_2->valuestring, j6_2->valuestring, x_2->valuestring, y_2->valuestring, z_2->valuestring, rx_2->valuestring, ry_2->valuestring, rz_2->valuestring, toolnum_2->valuestring, workpiecenum_2->valuestring, speed_2->valuestring, acc_2->valuestring, E1_2->valuestring, E2_2->valuestring, E3_2->valuestring, E4_2->valuestring, cmd_array[2], cmd_array[3], cmd_array[4], cmd_array[5], cmd_array[6], cmd_array[7], cmd_array[8], cmd_array[9], end_ptr);
		} else {
			sprintf(content, "%sCircle(%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,0,0,0,0,0,0)%s\n", head, j1->valuestring, j2->valuestring, j3->valuestring, j4->valuestring, j5->valuestring, j6->valuestring, x->valuestring, y->valuestring, z->valuestring, rx->valuestring, ry->valuestring, rz->valuestring, toolnum->valuestring, workpiecenum->valuestring, speed->valuestring, acc->valuestring, E1->valuestring, E2->valuestring, E3->valuestring, E4->valuestring, j1_2->valuestring, j2_2->valuestring, j3_2->valuestring, j4_2->valuestring, j5_2->valuestring, j6_2->valuestring, x_2->valuestring, y_2->valuestring, z_2->valuestring, rx_2->valuestring, ry_2->valuestring, rz_2->valuestring, toolnum_2->valuestring, workpiecenum_2->valuestring, speed_2->valuestring, acc_2->valuestring, E1_2->valuestring, E2_2->valuestring, E3_2->valuestring, E4_2->valuestring, cmd_array[2], cmd_array[3], end_ptr);
		}
		strcat(file_content, content);
	/* Spiral */
	} else if ((ptr = strstr(lua_cmd, "Spiral(")) && strrchr(lua_cmd, ')')) {
		end_ptr = strrchr(lua_cmd, ')') + 1;
		strncpy(head, lua_cmd, (ptr - lua_cmd));
		strncpy(cmd_arg, (ptr + 7), (end_ptr - ptr - 8));
		if (string_to_string_list(cmd_arg, ",", &size, &cmd_array) == 0 || size != 10) {
			perror("string to string list");
			argc_error_info(10, "Spiral");

			goto end;
		}
		if (cmd_array[0] == NULL) {

			goto end;
		}
		point_1 = cJSON_GetObjectItemCaseSensitive(p_db_json->point, cmd_array[0]);
		if (point_1 == NULL || point_1->type != cJSON_Object) {
			database_error_info();

			goto end;
		}
		j1 = cJSON_GetObjectItem(point_1, "j1");
		j2 = cJSON_GetObjectItem(point_1, "j2");
		j3 = cJSON_GetObjectItem(point_1, "j3");
		j4 = cJSON_GetObjectItem(point_1, "j4");
		j5 = cJSON_GetObjectItem(point_1, "j5");
		j6 = cJSON_GetObjectItem(point_1, "j6");
		x = cJSON_GetObjectItem(point_1, "x");
		y = cJSON_GetObjectItem(point_1, "y");
		z = cJSON_GetObjectItem(point_1, "z");
		rx = cJSON_GetObjectItem(point_1, "rx");
		ry = cJSON_GetObjectItem(point_1, "ry");
		rz = cJSON_GetObjectItem(point_1, "rz");
		toolnum = cJSON_GetObjectItem(point_1, "toolnum");
		workpiecenum = cJSON_GetObjectItem(point_1, "workpiecenum");
		speed = cJSON_GetObjectItem(point_1, "speed");
		acc = cJSON_GetObjectItem(point_1, "acc");
		E1 = cJSON_GetObjectItem(point_1, "E1");
		E2 = cJSON_GetObjectItem(point_1, "E2");
		E3 = cJSON_GetObjectItem(point_1, "E3");
		E4 = cJSON_GetObjectItem(point_1, "E4");
		if (j1 == NULL || j2 == NULL || j3 == NULL || j4 == NULL || j5 == NULL || j6 == NULL || x == NULL || y == NULL || z == NULL || rx == NULL || ry == NULL || rz == NULL || toolnum == NULL || workpiecenum == NULL || speed == NULL || acc == NULL || j1->valuestring == NULL || j2->valuestring == NULL || j3->valuestring == NULL || j4->valuestring == NULL || j5->valuestring == NULL || j6->valuestring == NULL || x->valuestring == NULL || y->valuestring == NULL || z->valuestring == NULL || rx->valuestring == NULL || ry->valuestring == NULL || rz->valuestring == NULL || toolnum->valuestring == NULL || workpiecenum->valuestring == NULL || speed->valuestring == NULL || acc->valuestring == NULL || E1->valuestring == NULL || E2->valuestring == NULL || E3->valuestring == NULL || E4->valuestring == NULL) {

			goto end;
		}
		/* 当点为 pHOME 原点时，进行检查 */
		if (strcmp(cmd_array[0], POINT_HOME) == 0) {
			sprintf(joint_value[0], "%.1lf", atof(j1->valuestring));
			sprintf(joint_value[1], "%.1lf", atof(j2->valuestring));
			sprintf(joint_value[2], "%.1lf", atof(j3->valuestring));
			sprintf(joint_value[3], "%.1lf", atof(j4->valuestring));
			sprintf(joint_value[4], "%.1lf", atof(j5->valuestring));
			sprintf(joint_value[5], "%.1lf", atof(j6->valuestring));
			for (i = 0; i < 6; i++) {
				joint_value_ptr[i] = joint_value[i];
			}
			/* 置异常报错的标志位，添加异常错误到 sta 状态反馈 error_info 中 */
			if (check_pointhome_data(joint_value_ptr) == FAIL) {
				point_home_info.error_flag = 1;

				goto end;
			}
			point_home_info.error_flag = 0;
		}
		if (cmd_array[1] == NULL) {

			goto end;
		}
		point_2 = cJSON_GetObjectItemCaseSensitive(p_db_json->point, cmd_array[1]);
		if (point_2 == NULL || point_2->type != cJSON_Object) {
			database_error_info();

			goto end;
		}
		j1_2 = cJSON_GetObjectItem(point_2, "j1");
		j2_2 = cJSON_GetObjectItem(point_2, "j2");
		j3_2 = cJSON_GetObjectItem(point_2, "j3");
		j4_2 = cJSON_GetObjectItem(point_2, "j4");
		j5_2 = cJSON_GetObjectItem(point_2, "j5");
		j6_2 = cJSON_GetObjectItem(point_2, "j6");
		x_2 = cJSON_GetObjectItem(point_2, "x");
		y_2 = cJSON_GetObjectItem(point_2, "y");
		z_2 = cJSON_GetObjectItem(point_2, "z");
		rx_2 = cJSON_GetObjectItem(point_2, "rx");
		ry_2 = cJSON_GetObjectItem(point_2, "ry");
		rz_2 = cJSON_GetObjectItem(point_2, "rz");
		toolnum_2 = cJSON_GetObjectItem(point_2, "toolnum");
		workpiecenum_2 = cJSON_GetObjectItem(point_2, "workpiecenum");
		speed_2 = cJSON_GetObjectItem(point_2, "speed");
		acc_2 = cJSON_GetObjectItem(point_2, "acc");
		E1_2 = cJSON_GetObjectItem(point_2, "E1");
		E2_2 = cJSON_GetObjectItem(point_2, "E2");
		E3_2 = cJSON_GetObjectItem(point_2, "E3");
		E4_2 = cJSON_GetObjectItem(point_2, "E4");
		if (j1_2 == NULL || j2_2 == NULL || j3_2 == NULL || j4_2 == NULL || j5_2 == NULL || j6_2 == NULL || x_2 == NULL || y_2 == NULL || z_2 == NULL || rx_2 == NULL || ry_2 == NULL || rz_2 == NULL || toolnum_2 == NULL || workpiecenum_2 == NULL || speed_2 == NULL || acc_2 == NULL || E1_2 == NULL || E2_2 == NULL || E3_2 == NULL || E4_2 == NULL || j1_2->valuestring == NULL || j2_2->valuestring == NULL || j3_2->valuestring == NULL || j4_2->valuestring == NULL || j5_2->valuestring == NULL || j6_2->valuestring == NULL || x_2->valuestring == NULL || y_2->valuestring == NULL || z_2->valuestring == NULL || rx_2->valuestring == NULL || ry_2->valuestring == NULL || rz_2->valuestring == NULL || toolnum_2->valuestring == NULL || workpiecenum_2->valuestring == NULL || speed_2->valuestring == NULL || acc_2->valuestring == NULL || E1_2->valuestring == NULL || E2_2->valuestring == NULL || E3_2->valuestring == NULL || E4_2->valuestring == NULL) {

			goto end;
		}
		/* 当点为 pHOME 原点时，进行检查 */
		if (strcmp(cmd_array[1], POINT_HOME) == 0) {
			for (i = 0; i < 6; i++) {
				memset(joint_value[i], 0, 10);
			}
			sprintf(joint_value[0], "%.1lf", atof(j1_2->valuestring));
			sprintf(joint_value[1], "%.1lf", atof(j2_2->valuestring));
			sprintf(joint_value[2], "%.1lf", atof(j3_2->valuestring));
			sprintf(joint_value[3], "%.1lf", atof(j4_2->valuestring));
			sprintf(joint_value[4], "%.1lf", atof(j5_2->valuestring));
			sprintf(joint_value[5], "%.1lf", atof(j6_2->valuestring));
			for (i = 0; i < 6; i++) {
				joint_value_ptr[i] = joint_value[i];
			}
			/* 置异常报错的标志位， 添加异常错误到 sta 状态反馈 error_info 中 */
			if (check_pointhome_data(joint_value_ptr) == FAIL) {
				point_home_info.error_flag = 1;

				goto end;
			}
			point_home_info.error_flag = 0;
		}
		if (cmd_array[2] == NULL) {

			goto end;
		}
		point_3 = cJSON_GetObjectItemCaseSensitive(p_db_json->point, cmd_array[2]);
		if (point_3 == NULL || point_3->type != cJSON_Object) {
			database_error_info();

			goto end;
		}
		j1_3 = cJSON_GetObjectItem(point_3, "j1");
		j2_3 = cJSON_GetObjectItem(point_3, "j2");
		j3_3 = cJSON_GetObjectItem(point_3, "j3");
		j4_3 = cJSON_GetObjectItem(point_3, "j4");
		j5_3 = cJSON_GetObjectItem(point_3, "j5");
		j6_3 = cJSON_GetObjectItem(point_3, "j6");
		x_3 = cJSON_GetObjectItem(point_3, "x");
		y_3 = cJSON_GetObjectItem(point_3, "y");
		z_3 = cJSON_GetObjectItem(point_3, "z");
		rx_3 = cJSON_GetObjectItem(point_3, "rx");
		ry_3 = cJSON_GetObjectItem(point_3, "ry");
		rz_3 = cJSON_GetObjectItem(point_3, "rz");
		toolnum_3 = cJSON_GetObjectItem(point_3, "toolnum");
		workpiecenum_3 = cJSON_GetObjectItem(point_3, "workpiecenum");
		speed_3 = cJSON_GetObjectItem(point_3, "speed");
		acc_3 = cJSON_GetObjectItem(point_3, "acc");
		E1_3 = cJSON_GetObjectItem(point_3, "E1");
		E2_3 = cJSON_GetObjectItem(point_3, "E2");
		E3_3 = cJSON_GetObjectItem(point_3, "E3");
		E4_3 = cJSON_GetObjectItem(point_3, "E4");
		if (j1_3 == NULL || j2_3 == NULL || j3_3 == NULL || j4_3 == NULL || j5_3 == NULL || j6_3 == NULL || x_3 == NULL || y_3 == NULL || z_3 == NULL || rx_3 == NULL || ry_3 == NULL || rz_3 == NULL || toolnum_3 == NULL || workpiecenum_3 == NULL || speed_3 == NULL || acc_3 == NULL || E1_3 == NULL || E2_3 == NULL || E3_3 == NULL || E4_3 == NULL || j1_3->valuestring == NULL || j2_3->valuestring == NULL || j3_3->valuestring == NULL || j4_3->valuestring == NULL || j5_3->valuestring == NULL || j6_3->valuestring == NULL || x_3->valuestring == NULL || y_3->valuestring == NULL || z_3->valuestring == NULL || rx_3->valuestring == NULL || ry_3->valuestring == NULL || rz_3->valuestring == NULL || toolnum_3->valuestring == NULL || workpiecenum_3->valuestring == NULL || speed_3->valuestring == NULL || acc_3->valuestring == NULL || E1_3->valuestring == NULL || E2_3->valuestring == NULL || E3_3->valuestring == NULL || E4_3->valuestring == NULL) {

			goto end;
		}
		/* 当点为 pHOME 原点时，进行检查 */
		if (strcmp(cmd_array[2], POINT_HOME) == 0) {
			for (i = 0; i < 6; i++) {
				memset(joint_value[i], 0, 10);
			}
			sprintf(joint_value[0], "%.1lf", atof(j1_3->valuestring));
			sprintf(joint_value[1], "%.1lf", atof(j2_3->valuestring));
			sprintf(joint_value[2], "%.1lf", atof(j3_3->valuestring));
			sprintf(joint_value[3], "%.1lf", atof(j4_3->valuestring));
			sprintf(joint_value[4], "%.1lf", atof(j5_3->valuestring));
			sprintf(joint_value[5], "%.1lf", atof(j6_3->valuestring));
			for (i = 0; i < 6; i++) {
				joint_value_ptr[i] = joint_value[i];
			}
			/* 置异常报错的标志位， 添加异常错误到 sta 状态反馈 error_info 中 */
			if (check_pointhome_data(joint_value_ptr) == FAIL) {
				point_home_info.error_flag = 1;

				goto end;
			}
			point_home_info.error_flag = 0;
		}
		if (cmd_array[3] == NULL || cmd_array[4] == NULL || cmd_array[5] == NULL || cmd_array[6] == NULL || cmd_array[7] == NULL || cmd_array[8] == NULL || cmd_array[9] == NULL) {

			goto end;
		}
		sprintf(content, "%sSpiral(%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s)%s\n", head, j1->valuestring, j2->valuestring, j3->valuestring, j4->valuestring, j5->valuestring, j6->valuestring, x->valuestring, y->valuestring, z->valuestring, rx->valuestring, ry->valuestring, rz->valuestring, toolnum->valuestring, workpiecenum->valuestring, speed->valuestring, acc->valuestring, E1->valuestring, E2->valuestring, E3->valuestring, E4->valuestring, j1_2->valuestring, j2_2->valuestring, j3_2->valuestring, j4_2->valuestring, j5_2->valuestring, j6_2->valuestring, x_2->valuestring, y_2->valuestring, z_2->valuestring, rx_2->valuestring, ry_2->valuestring, rz_2->valuestring, toolnum_2->valuestring, workpiecenum_2->valuestring, speed_2->valuestring, acc_2->valuestring, E1_2->valuestring, E2_2->valuestring, E3_2->valuestring, E4_2->valuestring, j1_3->valuestring, j2_3->valuestring, j3_3->valuestring, j4_3->valuestring, j5_3->valuestring, j6_3->valuestring, x_3->valuestring, y_3->valuestring, z_3->valuestring, rx_3->valuestring, ry_3->valuestring, rz_3->valuestring, toolnum_3->valuestring, workpiecenum_3->valuestring, speed_3->valuestring, acc_3->valuestring, E1_3->valuestring, E2_3->valuestring, E3_3->valuestring, E4_3->valuestring, cmd_array[3], cmd_array[4], cmd_array[5], cmd_array[6], cmd_array[7], cmd_array[8], cmd_array[9], end_ptr);
		strcat(file_content, content);
	/* SCIRC */
	} else if ((ptr = strstr(lua_cmd, "SCIRC(")) && strrchr(lua_cmd, ')')) {
		end_ptr = strrchr(lua_cmd, ')') + 1;
		strncpy(head, lua_cmd, (ptr - lua_cmd));
		strncpy(cmd_arg, (ptr + 6), (end_ptr - ptr - 7));
		if (string_to_string_list(cmd_arg, ",", &size, &cmd_array) == 0 || size != 3) {
			perror("string to string list");
			argc_error_info(3, "SCIRC");

			goto end;
		}
		point_1 = cJSON_GetObjectItemCaseSensitive(p_db_json->point, cmd_array[0]);
		if (point_1 == NULL || point_1->type != cJSON_Object) {
			database_error_info();

			goto end;
		}
		j1 = cJSON_GetObjectItem(point_1, "j1");
		j2 = cJSON_GetObjectItem(point_1, "j2");
		j3 = cJSON_GetObjectItem(point_1, "j3");
		j4 = cJSON_GetObjectItem(point_1, "j4");
		j5 = cJSON_GetObjectItem(point_1, "j5");
		j6 = cJSON_GetObjectItem(point_1, "j6");
		x = cJSON_GetObjectItem(point_1, "x");
		y = cJSON_GetObjectItem(point_1, "y");
		z = cJSON_GetObjectItem(point_1, "z");
		rx = cJSON_GetObjectItem(point_1, "rx");
		ry = cJSON_GetObjectItem(point_1, "ry");
		rz = cJSON_GetObjectItem(point_1, "rz");
		toolnum = cJSON_GetObjectItem(point_1, "toolnum");
		workpiecenum = cJSON_GetObjectItem(point_1, "workpiecenum");
		speed = cJSON_GetObjectItem(point_1, "speed");
		acc = cJSON_GetObjectItem(point_1, "acc");
		if (j1 == NULL || j2 == NULL || j3 == NULL || j4 == NULL || j5 == NULL || j6 == NULL || x == NULL || y == NULL || z == NULL || rx == NULL || ry == NULL || rz == NULL || toolnum == NULL || workpiecenum == NULL || speed == NULL || acc == NULL || j1->valuestring == NULL || j2->valuestring == NULL || j3->valuestring == NULL || j4->valuestring == NULL || j5->valuestring == NULL || j6->valuestring == NULL || x->valuestring == NULL || y->valuestring == NULL || z->valuestring == NULL || rx->valuestring == NULL || ry->valuestring == NULL || rz->valuestring == NULL || toolnum->valuestring == NULL || workpiecenum->valuestring == NULL || speed->valuestring == NULL || acc->valuestring == NULL || cmd_array[1] == NULL) {

			goto end;
		}
		/* 当点为 pHOME 原点时，进行检查 */
		if (strcmp(cmd_array[0], POINT_HOME) == 0) {
			sprintf(joint_value[0], "%.1lf", atof(j1->valuestring));
			sprintf(joint_value[1], "%.1lf", atof(j2->valuestring));
			sprintf(joint_value[2], "%.1lf", atof(j3->valuestring));
			sprintf(joint_value[3], "%.1lf", atof(j4->valuestring));
			sprintf(joint_value[4], "%.1lf", atof(j5->valuestring));
			sprintf(joint_value[5], "%.1lf", atof(j6->valuestring));
			for (i = 0; i < 6; i++) {
				joint_value_ptr[i] = joint_value[i];
			}
			/* 置异常报错的标志位， 添加异常错误到 sta 状态反馈 error_info 中 */
			if (check_pointhome_data(joint_value_ptr) == FAIL) {
				point_home_info.error_flag = 1;

				goto end;
			}
			point_home_info.error_flag = 0;
		}
		point_2 = cJSON_GetObjectItemCaseSensitive(p_db_json->point, cmd_array[1]);
		if (point_2 == NULL || point_2->type != cJSON_Object) {
			database_error_info();

			goto end;
		}
		j1_2 = cJSON_GetObjectItem(point_2, "j1");
		j2_2 = cJSON_GetObjectItem(point_2, "j2");
		j3_2 = cJSON_GetObjectItem(point_2, "j3");
		j4_2 = cJSON_GetObjectItem(point_2, "j4");
		j5_2 = cJSON_GetObjectItem(point_2, "j5");
		j6_2 = cJSON_GetObjectItem(point_2, "j6");
		x_2 = cJSON_GetObjectItem(point_2, "x");
		y_2 = cJSON_GetObjectItem(point_2, "y");
		z_2 = cJSON_GetObjectItem(point_2, "z");
		rx_2 = cJSON_GetObjectItem(point_2, "rx");
		ry_2 = cJSON_GetObjectItem(point_2, "ry");
		rz_2 = cJSON_GetObjectItem(point_2, "rz");
		toolnum_2 = cJSON_GetObjectItem(point_2, "toolnum");
		workpiecenum_2 = cJSON_GetObjectItem(point_2, "workpiecenum");
		speed_2 = cJSON_GetObjectItem(point_2, "speed");
		acc_2 = cJSON_GetObjectItem(point_2, "acc");
		if (j1_2 == NULL || j2_2 == NULL || j3_2 == NULL || j4_2 == NULL || j5_2 == NULL || j6_2 == NULL || x_2 == NULL || y_2 == NULL || z_2 == NULL || rx_2 == NULL || ry_2 == NULL || rz_2 == NULL || toolnum_2 == NULL || workpiecenum_2 == NULL || speed_2 == NULL || acc_2 == NULL || j1_2->valuestring == NULL || j2_2->valuestring == NULL || j3_2->valuestring == NULL || j4_2->valuestring == NULL || j5_2->valuestring == NULL || j6_2->valuestring == NULL || x_2->valuestring == NULL || y_2->valuestring == NULL || z_2->valuestring == NULL || rx_2->valuestring == NULL || ry_2->valuestring == NULL || rz_2->valuestring == NULL || toolnum_2->valuestring == NULL || workpiecenum_2->valuestring == NULL || speed_2->valuestring == NULL || acc_2->valuestring == NULL || cmd_array[2] == NULL) {

			goto end;
		}
		/* 当点为 pHOME 原点时，进行检查 */
		if (strcmp(cmd_array[1], POINT_HOME) == 0) {
			for (i = 0; i < 6; i++) {
				memset(joint_value[i], 0, 10);
			}
			sprintf(joint_value[0], "%.1lf", atof(j1_2->valuestring));
			sprintf(joint_value[1], "%.1lf", atof(j2_2->valuestring));
			sprintf(joint_value[2], "%.1lf", atof(j3_2->valuestring));
			sprintf(joint_value[3], "%.1lf", atof(j4_2->valuestring));
			sprintf(joint_value[4], "%.1lf", atof(j5_2->valuestring));
			sprintf(joint_value[5], "%.1lf", atof(j6_2->valuestring));
			for (i = 0; i < 6; i++) {
				joint_value_ptr[i] = joint_value[i];
			}
			/* 置异常报错的标志位， 添加异常错误到 sta 状态反馈 error_info 中 */
			if (check_pointhome_data(joint_value_ptr) == FAIL) {
				point_home_info.error_flag = 1;

				goto end;
			}
			point_home_info.error_flag = 0;
		}
		sprintf(content, "%sSplineCIRC(%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s)%s\n", head, j1->valuestring, j2->valuestring, j3->valuestring, j4->valuestring, j5->valuestring, j6->valuestring, x->valuestring, y->valuestring, z->valuestring, rx->valuestring, ry->valuestring, rz->valuestring, toolnum->valuestring, workpiecenum->valuestring, speed->valuestring, acc->valuestring, j1_2->valuestring, j2_2->valuestring, j3_2->valuestring, j4_2->valuestring, j5_2->valuestring, j6_2->valuestring, x_2->valuestring, y_2->valuestring, z_2->valuestring, rx_2->valuestring, ry_2->valuestring, rz_2->valuestring, toolnum_2->valuestring, workpiecenum_2->valuestring, speed_2->valuestring, acc_2->valuestring, cmd_array[2], end_ptr);
		strcat(file_content, content);
	/* set AO */
	} else if ((ptr = strstr(lua_cmd, "SetAO(")) && strrchr(lua_cmd, ')')) {
		end_ptr = strrchr(lua_cmd, ')') + 1;
		strncpy(head, lua_cmd, (ptr - lua_cmd));
		strncpy(cmd_arg, (ptr + 6), (end_ptr - ptr - 7));
		if (string_to_string_list(cmd_arg, ",", &size, &cmd_array) == 0 || size != 2) {
			perror("string to string list");
			argc_error_info(2, "SetAO");

			goto end;
		}
		sprintf(content, "%sSetAO(%s,%.2f)%s\n", head, cmd_array[0], (float)(atoi(cmd_array[1])*40.95), end_ptr);
		strcat(file_content, content);
	/* set ToolAO */
	} else if ((ptr = strstr(lua_cmd, "SetToolAO(")) && strrchr(lua_cmd, ')')) {
		end_ptr = strrchr(lua_cmd, ')') + 1;
		strncpy(head, lua_cmd, (ptr - lua_cmd));
		strncpy(cmd_arg, (ptr + 10), (end_ptr - ptr - 11));
		if (string_to_string_list(cmd_arg, ",", &size, &cmd_array) == 0 || size != 2) {
			perror("string to string list");
			argc_error_info(2, "SetToolAO");

			goto end;
		}
		sprintf(content, "%sSetToolAO(%s,%.2f)%s\n", head, cmd_array[0], (float)(atoi(cmd_array[1])*40.95), end_ptr);
		strcat(file_content, content);
	/* soft-PLC setAO */
	} else if ((ptr = strstr(lua_cmd, "SPLCSetAO(")) && strrchr(lua_cmd, ')')) {
		end_ptr = strrchr(lua_cmd, ')') + 1;
		strncpy(head, lua_cmd, (ptr - lua_cmd));
		strncpy(cmd_arg, (ptr + 10), (end_ptr - ptr - 11));
		if (string_to_string_list(cmd_arg, ",", &size, &cmd_array) == 0 || size != 2) {
			perror("string to string list");
			argc_error_info(2, "SPLCSetAO");

			goto end;
		}
		sprintf(content, "%sSPLCSetAO(%s,%.2f)%s\n", head, cmd_array[0], (float)(atoi(cmd_array[1])*40.95), end_ptr);
		strcat(file_content, content);
	/* soft-PLC setToolAO */
	} else if ((ptr = strstr(lua_cmd, "SPLCSetToolAO(")) && strrchr(lua_cmd, ')')) {
		end_ptr = strrchr(lua_cmd, ')') + 1;
		strncpy(head, lua_cmd, (ptr - lua_cmd));
		strncpy(cmd_arg, (ptr + 14), (end_ptr - ptr - 15));
		if (string_to_string_list(cmd_arg, ",", &size, &cmd_array) == 0 || size != 2) {
			perror("string to string list");
			argc_error_info(2, "SPLCSetToolAO");

			goto end;
		}
		sprintf(content, "%sSPLCSetToolAO(%s,%.2f)%s\n", head, cmd_array[0], (float)(atoi(cmd_array[1])*40.95), end_ptr);
		strcat(file_content, content);
	/* set ToolList */
	} else if ((ptr = strstr(lua_cmd, "SetToolList(")) && strrchr(lua_cmd, ')')) {
		end_ptr = strrchr(lua_cmd, ')') + 1;
		strncpy(head, lua_cmd, (ptr - lua_cmd));
		strncpy(cmd_arg, (ptr + 12), (end_ptr - ptr - 13));
		if (string_to_string_list(cmd_arg, ",", &size, &cmd_array) == 0 || size != 1) {
			perror("string to string list");
			argc_error_info(1, "SetToolList");

			goto end;
		}
		cd = cJSON_GetObjectItemCaseSensitive(p_db_json->cdsystem, cmd_array[0]);
		if (cd == NULL || cd->type != cJSON_Object) {
			database_error_info();

			goto end;
		}
		id = cJSON_GetObjectItem(cd, "id");
		type = cJSON_GetObjectItem(cd, "type");
		installation_site = cJSON_GetObjectItem(cd, "installation_site");
		x = cJSON_GetObjectItem(cd, "x");
		y = cJSON_GetObjectItem(cd, "y");
		z = cJSON_GetObjectItem(cd, "z");
		rx = cJSON_GetObjectItem(cd, "rx");
		ry = cJSON_GetObjectItem(cd, "ry");
		rz = cJSON_GetObjectItem(cd, "rz");
		if (id == NULL || type == NULL || installation_site == NULL || x == NULL || y == NULL || z == NULL || rx == NULL || ry == NULL || rz == NULL || id->valuestring == NULL || type->valuestring == NULL || installation_site->valuestring == NULL  || x->valuestring == NULL || y->valuestring == NULL || z->valuestring == NULL || rx->valuestring == NULL || ry->valuestring == NULL || rz->valuestring == NULL) {

			goto end;
		}
		sprintf(content, "%sSetToolList(%s,%s,%s,%s,%s,%s,%s,%s,%s)%s\n", head, id->valuestring, x->valuestring, y->valuestring, z->valuestring, rx->valuestring, ry->valuestring, rz->valuestring, type->valuestring, installation_site->valuestring, end_ptr);
		strcat(file_content, content);
	/* SetWobjList */
	} else if ((ptr = strstr(lua_cmd, "SetWobjList(")) && strrchr(lua_cmd, ')')) {
		end_ptr = strrchr(lua_cmd, ')') + 1;
		strncpy(head, lua_cmd, (ptr - lua_cmd));
		strncpy(cmd_arg, (ptr + 12), (end_ptr - ptr - 13));
		if (string_to_string_list(cmd_arg, ",", &size, &cmd_array) == 0 || size != 1) {
			perror("string to string list");
			argc_error_info(1, "SetWobjList");

			goto end;
		}
		cd = cJSON_GetObjectItemCaseSensitive(p_db_json->wobj_cdsystem, cmd_array[0]);
		if (cd == NULL || cd->type != cJSON_Object) {
			database_error_info();

			goto end;
		}
		id = cJSON_GetObjectItem(cd, "id");
		x = cJSON_GetObjectItem(cd, "x");
		y = cJSON_GetObjectItem(cd, "y");
		z = cJSON_GetObjectItem(cd, "z");
		rx = cJSON_GetObjectItem(cd, "rx");
		ry = cJSON_GetObjectItem(cd, "ry");
		rz = cJSON_GetObjectItem(cd, "rz");
		if (id == NULL || x == NULL || y == NULL || z == NULL || rx == NULL || ry == NULL || rz == NULL || id->valuestring == NULL || x->valuestring == NULL || y->valuestring == NULL || z->valuestring == NULL || rx->valuestring == NULL || ry->valuestring == NULL || rz->valuestring == NULL) {

			goto end;
		}
		sprintf(content, "%sSetWobjList(%s,%s,%s,%s,%s,%s,%s)%s\n", head, id->valuestring, x->valuestring, y->valuestring, z->valuestring, rx->valuestring, ry->valuestring, rz->valuestring, end_ptr);
		strcat(file_content, content);
	/* SetExToolList */
	} else if ((ptr = strstr(lua_cmd, "SetExToolList(")) && strrchr(lua_cmd, ')')) {
		end_ptr = strrchr(lua_cmd, ')') + 1;
		strncpy(head, lua_cmd, (ptr - lua_cmd));
		strncpy(cmd_arg, (ptr + 14), (end_ptr - ptr - 15));
		if (string_to_string_list(cmd_arg, ",", &size, &cmd_array) == 0 || size != 1) {
			perror("string to string list");
			argc_error_info(1, "SetExToolList");

			goto end;
		}
		et_cd = cJSON_GetObjectItemCaseSensitive(p_db_json->et_cdsystem, cmd_array[0]);
		if (et_cd == NULL || et_cd->type != cJSON_Object) {
			database_error_info();

			goto end;
		}
		id = cJSON_GetObjectItem(et_cd, "id");
		ex = cJSON_GetObjectItem(et_cd, "ex");
		ey = cJSON_GetObjectItem(et_cd, "ey");
		ez = cJSON_GetObjectItem(et_cd, "ez");
		erx = cJSON_GetObjectItem(et_cd, "erx");
		ery = cJSON_GetObjectItem(et_cd, "ery");
		erz = cJSON_GetObjectItem(et_cd, "erz");
		tx = cJSON_GetObjectItem(et_cd, "tx");
		ty = cJSON_GetObjectItem(et_cd, "ty");
		tz = cJSON_GetObjectItem(et_cd, "tz");
		trx = cJSON_GetObjectItem(et_cd, "trx");
		try = cJSON_GetObjectItem(et_cd, "try");
		trz = cJSON_GetObjectItem(et_cd, "trz");
		if (id == NULL || ex == NULL || ey == NULL || ez == NULL || erx == NULL || ery == NULL || erz == NULL || tx == NULL || ty == NULL || tz == NULL || trx == NULL || try == NULL || trz == NULL || id->valuestring == NULL || ex->valuestring == NULL || ey->valuestring == NULL || ez->valuestring == NULL || erx->valuestring == NULL || ery->valuestring == NULL || erz->valuestring == NULL || tx->valuestring == NULL || ty->valuestring == NULL || tz->valuestring == NULL || trx->valuestring == NULL || try->valuestring == NULL || trz->valuestring == NULL) {

			goto end;
		}
		sprintf(content, "%sSetExToolList(%d,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s)%s\n", head, (atoi(id->valuestring) + 14), ex->valuestring, ey->valuestring, ez->valuestring, erx->valuestring, ery->valuestring, erz->valuestring, tx->valuestring, ty->valuestring, tz->valuestring, trx->valuestring, try->valuestring, trz->valuestring, end_ptr);
		strcat(file_content, content);
	/* PostureAdjustOn */
	} else if ((ptr = strstr(lua_cmd, "PostureAdjustOn(")) && strrchr(lua_cmd, ')')) {
		end_ptr = strrchr(lua_cmd, ')') + 1;
		strncpy(head, lua_cmd, (ptr - lua_cmd));
		strncpy(cmd_arg, (ptr + 16), (end_ptr - ptr - 17));
		if (string_to_string_list(cmd_arg, ",", &size, &cmd_array) == 0 || size != 11) {
			perror("string to string list");
			argc_error_info(11, "PostureAdjustOn");

			goto end;
		}
		point_1 = cJSON_GetObjectItemCaseSensitive(p_db_json->point, cmd_array[1]);
		if (point_1 == NULL || point_1->type != cJSON_Object) {
			database_error_info();

			goto end;
		}
		rx = cJSON_GetObjectItem(point_1, "rx");
		ry = cJSON_GetObjectItem(point_1, "ry");
		rz = cJSON_GetObjectItem(point_1, "rz");
		if (rx == NULL || ry == NULL || rz == NULL || rx->valuestring == NULL || ry->valuestring == NULL || rz->valuestring == NULL) {

			goto end;
		}
		point_2 = cJSON_GetObjectItemCaseSensitive(p_db_json->point, cmd_array[2]);
		if (point_2 == NULL || point_2->type != cJSON_Object) {
			database_error_info();

			goto end;
		}
		rx_2 = cJSON_GetObjectItem(point_2, "rx");
		ry_2 = cJSON_GetObjectItem(point_2, "ry");
		rz_2 = cJSON_GetObjectItem(point_2, "rz");
		if (rx_2 == NULL || ry_2 == NULL || rz_2 == NULL || rx_2->valuestring == NULL || ry_2->valuestring == NULL || rz_2->valuestring == NULL) {

			goto end;
		}
		point_3 = cJSON_GetObjectItemCaseSensitive(p_db_json->point, cmd_array[3]);
		if (point_3 == NULL || point_3->type != cJSON_Object) {
			database_error_info();

			goto end;
		}
		rx_3 = cJSON_GetObjectItem(point_3, "rx");
		ry_3 = cJSON_GetObjectItem(point_3, "ry");
		rz_3 = cJSON_GetObjectItem(point_3, "rz");
		if (rx_3 == NULL || ry_3 == NULL || rz_3 == NULL || rx_3->valuestring == NULL || ry_3->valuestring == NULL || rz_3->valuestring == NULL) {

			goto end;
		}
		sprintf(content, "%sPostureAdjustOn(%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s)%s\n", head, cmd_array[0], rx->valuestring, ry->valuestring, rz->valuestring, rx_2->valuestring, ry_2->valuestring, rz_2->valuestring, rx_3->valuestring, ry_3->valuestring, rz_3->valuestring, cmd_array[4], cmd_array[5], cmd_array[6], cmd_array[7], cmd_array[8], cmd_array[9], cmd_array[10], end_ptr);
		strcat(file_content, content);
	/* RegisterVar */
	} else if ((ptr = strstr(lua_cmd, "RegisterVar(")) && strrchr(lua_cmd, ')')) {
		end_ptr = strrchr(lua_cmd, ')') + 1;
		strncpy(head, lua_cmd, (ptr - lua_cmd));
		strncpy(cmd_arg, (ptr + 12), (end_ptr - ptr - 13));
		if (string_to_string_list(cmd_arg, ",", &size, &cmd_array) == 0 || size != 2) {
			perror("string to string list");
			argc_error_info(2, "RegisterVar");

			goto end;
		}
		if ((strstr(cmd_array[0], "\"number\"") || strstr(cmd_array[0], "\"string\"")) && strstr(cmd_array[1], "\"")) {
			strcat(file_content, lua_cmd);
			strcat(file_content, "\n");
		} else {
			perror("argv error");
			argc_error_info(2, "RegisterVar");

			goto end;
		}
	/* SetSysVarValue */
	} else if ((ptr = strstr(lua_cmd, "SetSysVarValue(")) && strrchr(lua_cmd, ')')) {
		end_ptr = strrchr(lua_cmd, ')') + 1;
		strncpy(head, lua_cmd, (ptr - lua_cmd));
		strncpy(cmd_arg, (ptr + 15), (end_ptr - ptr - 16));
		if (string_to_string_list(cmd_arg, ",", &size, &cmd_array) == 0 || size != 2) {
			perror("string to string list");
			argc_error_info(2, "SetSysVarValue");

			goto end;
		}
		var = cJSON_GetObjectItemCaseSensitive(p_db_json->sysvar, cmd_array[0]);
		if (var == NULL || var->type != cJSON_Object) {
			database_error_info();

			goto end;
		}
		id = cJSON_GetObjectItem(var, "id");
		if (id == NULL) {

			goto end;
		}
		sprintf(content, "%sSetSysVarValue(%s,%s)%s\n", head, id->valuestring, cmd_array[1], end_ptr);
		strcat(file_content, content);
	/* GetSysVarValue */
	} else if ((ptr = strstr(lua_cmd, "GetSysVarValue(")) && strrchr(lua_cmd, ')')) {
		end_ptr = strrchr(lua_cmd, ')') + 1;
		strncpy(head, lua_cmd, (ptr - lua_cmd));
		strncpy(cmd_arg, (ptr + 15), (end_ptr - ptr - 16));
		if (string_to_string_list(cmd_arg, ",", &size, &cmd_array) == 0 || size != 1) {
			perror("string to string list");
			argc_error_info(1, "GetSysVarValue");

			goto end;
		}
		var = cJSON_GetObjectItemCaseSensitive(p_db_json->sysvar, cmd_array[0]);
		if (var == NULL || var->type != cJSON_Object) {
			database_error_info();

			goto end;
		}
		id = cJSON_GetObjectItem(var, "id");
		if (id == NULL) {

			goto end;
		}
		sprintf(content, "%sGetSysVarValue(%s)%s\n", head, id->valuestring, end_ptr);
		strcat(file_content, content);
	/* MultilayerOffsetTrsfToBase */
	} else if ((ptr = strstr(lua_cmd, "MultilayerOffsetTrsfToBase(")) && strrchr(lua_cmd, ')')) {
		end_ptr = strrchr(lua_cmd, ')') + 1;
		strncpy(head, lua_cmd, (ptr - lua_cmd));
		strncpy(cmd_arg, (ptr + 27), (end_ptr - ptr - 28));
		if (string_to_string_list(cmd_arg, ",", &size, &cmd_array) == 0 || size != 6) {
			perror("string to string list");
			argc_error_info(6, "MultilayerOffsetTrsfToBase");

			goto end;
		}
		point_1 = cJSON_GetObjectItemCaseSensitive(p_db_json->point, cmd_array[0]);
		if (point_1 == NULL || point_1->type != cJSON_Object) {
			database_error_info();

			goto end;
		}
		x = cJSON_GetObjectItem(point_1, "x");
		y = cJSON_GetObjectItem(point_1, "y");
		z = cJSON_GetObjectItem(point_1, "z");
		if (x == NULL || y == NULL || z == NULL) {

			goto end;
		}
		point_2 = cJSON_GetObjectItemCaseSensitive(p_db_json->point, cmd_array[1]);
		if (point_2 == NULL || point_2->type != cJSON_Object) {
			database_error_info();

			goto end;
		}
		x_2 = cJSON_GetObjectItem(point_2, "x");
		y_2 = cJSON_GetObjectItem(point_2, "y");
		z_2 = cJSON_GetObjectItem(point_2, "z");
		if (x_2 == NULL || y_2 == NULL || z_2 == NULL) {

			goto end;
		}
		point_3 = cJSON_GetObjectItemCaseSensitive(p_db_json->point, cmd_array[2]);
		if (point_3 == NULL || point_3->type != cJSON_Object) {
			database_error_info();

			goto end;
		}
		x_3 = cJSON_GetObjectItem(point_3, "x");
		y_3 = cJSON_GetObjectItem(point_3, "y");
		z_3 = cJSON_GetObjectItem(point_3, "z");
		if (x_3 == NULL || y_3 == NULL || z_3 == NULL) {

			goto end;
		}
		sprintf(content, "%sMultilayerOffsetTrsfToBase(%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s)%s\n", head, x->valuestring, y->valuestring, z->valuestring, x_2->valuestring, y_2->valuestring, z_2->valuestring, x_3->valuestring, y_3->valuestring, z_3->valuestring, cmd_array[3], cmd_array[4], cmd_array[5], end_ptr);
		strcat(file_content, content);
	/* GetSegWeldDisDir */
	} else if ((ptr = strstr(lua_cmd, "GetSegWeldDisDir(")) && strrchr(lua_cmd, ')')) {
		end_ptr = strrchr(lua_cmd, ')') + 1;
		strncpy(head, lua_cmd, (ptr - lua_cmd));
		strncpy(cmd_arg, (ptr + 17), (end_ptr - ptr - 18));
		if (string_to_string_list(cmd_arg, ",", &size, &cmd_array) == 0 || size != 2) {
			perror("string to string list");
			argc_error_info(2, "GetSegWeldDisDir");

			goto end;
		}
		if (cmd_array[0] == NULL) {

			goto end;
		}
		point_1 = cJSON_GetObjectItemCaseSensitive(p_db_json->point, cmd_array[0]);
		if (point_1 == NULL || point_1->type != cJSON_Object) {
			database_error_info();

			goto end;
		}
		j1 = cJSON_GetObjectItem(point_1, "j1");
		j2 = cJSON_GetObjectItem(point_1, "j2");
		j3 = cJSON_GetObjectItem(point_1, "j3");
		j4 = cJSON_GetObjectItem(point_1, "j4");
		j5 = cJSON_GetObjectItem(point_1, "j5");
		j6 = cJSON_GetObjectItem(point_1, "j6");
		x = cJSON_GetObjectItem(point_1, "x");
		y = cJSON_GetObjectItem(point_1, "y");
		z = cJSON_GetObjectItem(point_1, "z");
		rx = cJSON_GetObjectItem(point_1, "rx");
		ry = cJSON_GetObjectItem(point_1, "ry");
		rz = cJSON_GetObjectItem(point_1, "rz");
		toolnum = cJSON_GetObjectItem(point_1, "toolnum");
		workpiecenum = cJSON_GetObjectItem(point_1, "workpiecenum");
		speed = cJSON_GetObjectItem(point_1, "speed");
		acc = cJSON_GetObjectItem(point_1, "acc");
		E1 = cJSON_GetObjectItem(point_1, "E1");
		E2 = cJSON_GetObjectItem(point_1, "E2");
		E3 = cJSON_GetObjectItem(point_1, "E3");
		E4 = cJSON_GetObjectItem(point_1, "E4");
		if (j1 == NULL || j2 == NULL || j3 == NULL || j4 == NULL || j5 == NULL || j6 == NULL || x == NULL || y == NULL || z == NULL || rx == NULL || ry == NULL || rz == NULL || toolnum == NULL || workpiecenum == NULL || speed == NULL || acc == NULL || j1->valuestring == NULL || j2->valuestring == NULL || j3->valuestring == NULL || j4->valuestring == NULL || j5->valuestring == NULL || j6->valuestring == NULL || x->valuestring == NULL || y->valuestring == NULL || z->valuestring == NULL || rx->valuestring == NULL || ry->valuestring == NULL || rz->valuestring == NULL || toolnum->valuestring == NULL || workpiecenum->valuestring == NULL || speed->valuestring == NULL || acc->valuestring == NULL || E1->valuestring == NULL || E2->valuestring == NULL || E3->valuestring == NULL || E4->valuestring == NULL) {

			goto end;
		}
		/* 当点为 pHOME 原点时，进行检查 */
		if (strcmp(cmd_array[0], POINT_HOME) == 0) {
			sprintf(joint_value[0], "%.1lf", atof(j1->valuestring));
			sprintf(joint_value[1], "%.1lf", atof(j2->valuestring));
			sprintf(joint_value[2], "%.1lf", atof(j3->valuestring));
			sprintf(joint_value[3], "%.1lf", atof(j4->valuestring));
			sprintf(joint_value[4], "%.1lf", atof(j5->valuestring));
			sprintf(joint_value[5], "%.1lf", atof(j6->valuestring));
			for (i = 0; i < 6; i++) {
				joint_value_ptr[i] = joint_value[i];
			}
			/* 置异常报错的标志位，添加异常错误到 sta 状态反馈 error_info 中 */
			if (check_pointhome_data(joint_value_ptr) == FAIL) {
				point_home_info.error_flag = 1;

				goto end;
			}
			point_home_info.error_flag = 0;
		}
		if (cmd_array[1] == NULL) {

			goto end;
		}
		point_2 = cJSON_GetObjectItemCaseSensitive(p_db_json->point, cmd_array[1]);
		if (point_2 == NULL || point_2->type != cJSON_Object) {
			database_error_info();

			goto end;
		}
		j1_2 = cJSON_GetObjectItem(point_2, "j1");
		j2_2 = cJSON_GetObjectItem(point_2, "j2");
		j3_2 = cJSON_GetObjectItem(point_2, "j3");
		j4_2 = cJSON_GetObjectItem(point_2, "j4");
		j5_2 = cJSON_GetObjectItem(point_2, "j5");
		j6_2 = cJSON_GetObjectItem(point_2, "j6");
		x_2 = cJSON_GetObjectItem(point_2, "x");
		y_2 = cJSON_GetObjectItem(point_2, "y");
		z_2 = cJSON_GetObjectItem(point_2, "z");
		rx_2 = cJSON_GetObjectItem(point_2, "rx");
		ry_2 = cJSON_GetObjectItem(point_2, "ry");
		rz_2 = cJSON_GetObjectItem(point_2, "rz");
		toolnum_2 = cJSON_GetObjectItem(point_2, "toolnum");
		workpiecenum_2 = cJSON_GetObjectItem(point_2, "workpiecenum");
		speed_2 = cJSON_GetObjectItem(point_2, "speed");
		acc_2 = cJSON_GetObjectItem(point_2, "acc");
		E1_2 = cJSON_GetObjectItem(point_2, "E1");
		E2_2 = cJSON_GetObjectItem(point_2, "E2");
		E3_2 = cJSON_GetObjectItem(point_2, "E3");
		E4_2 = cJSON_GetObjectItem(point_2, "E4");
		if (j1_2 == NULL || j2_2 == NULL || j3_2 == NULL || j4_2 == NULL || j5_2 == NULL || j6_2 == NULL || x_2 == NULL || y_2 == NULL || z_2 == NULL || rx_2 == NULL || ry_2 == NULL || rz_2 == NULL || toolnum_2 == NULL || workpiecenum_2 == NULL || speed_2 == NULL || acc_2 == NULL || E1_2 == NULL || E2_2 == NULL || E3_2 == NULL || E4_2 == NULL || j1_2->valuestring == NULL || j2_2->valuestring == NULL || j3_2->valuestring == NULL || j4_2->valuestring == NULL || j5_2->valuestring == NULL || j6_2->valuestring == NULL || x_2->valuestring == NULL || y_2->valuestring == NULL || z_2->valuestring == NULL || rx_2->valuestring == NULL || ry_2->valuestring == NULL || rz_2->valuestring == NULL || toolnum_2->valuestring == NULL || workpiecenum_2->valuestring == NULL || speed_2->valuestring == NULL || acc_2->valuestring == NULL || E1_2->valuestring == NULL || E2_2->valuestring == NULL || E3_2->valuestring == NULL || E4_2->valuestring == NULL) {

			goto end;
		}
		/* 当点为 pHOME 原点时，进行检查 */
		if (strcmp(cmd_array[1], POINT_HOME) == 0) {
			for (i = 0; i < 6; i++) {
				memset(joint_value[i], 0, 10);
			}
			sprintf(joint_value[0], "%.1lf", atof(j1_2->valuestring));
			sprintf(joint_value[1], "%.1lf", atof(j2_2->valuestring));
			sprintf(joint_value[2], "%.1lf", atof(j3_2->valuestring));
			sprintf(joint_value[3], "%.1lf", atof(j4_2->valuestring));
			sprintf(joint_value[4], "%.1lf", atof(j5_2->valuestring));
			sprintf(joint_value[5], "%.1lf", atof(j6_2->valuestring));
			for (i = 0; i < 6; i++) {
				joint_value_ptr[i] = joint_value[i];
			}
			/* 置异常报错的标志位， 添加异常错误到 sta 状态反馈 error_info 中 */
			if (check_pointhome_data(joint_value_ptr) == FAIL) {
				point_home_info.error_flag = 1;

				goto end;
			}
			point_home_info.error_flag = 0;
		}
		sprintf(content, "%sGetSegWeldDisDir(%s,%s,%s,%s,%s,%s)%s\n", head, x->valuestring, y->valuestring, z->valuestring, x_2->valuestring, y_2->valuestring, z_2->valuestring, end_ptr);
		strcat(file_content, content);
	/* other code send without processing */
	} else {
		strcat(file_content, lua_cmd);
		strcat(file_content, "\n");
	}
	//printf("file_content = %s\n", file_content);
	string_list_free(&cmd_array, size);

	return SUCCESS;

end:
	if (strlen(error_info) == 0) {
		sprintf(error_info, "parse lua fail");
	}
	string_list_free(&cmd_array, size);
	return FAIL;
}

int get_robot_alarm_error_info(cJSON *alarm_json, cJSON *error_json)
{
	CTRL_STATE *state = NULL;
	CTRL_STATE *pre_state = NULL;
	char content[MAX_BUF] = {0};
	char en_content[MAX_BUF] = {0};
	char jap_content[MAX_BUF] = {0};
	int i = 0;

	if (robot_type == 1) { // "1" 代表实体机器人
		state = &ctrl_state;
		pre_state = &pre_ctrl_state;
	} else { // "0" 代表虚拟机器人
		state = &vir_ctrl_state;
		pre_state = &pre_vir_ctrl_state;
	}

	/** alarm info */
	if (state->tpd_num_limit == 1) {
		if (language == 0) {
			cJSON_AddStringToObject(alarm_json, "key", "TPD轨迹加载数量超限");
		}
		if (language == 1) {
			cJSON_AddStringToObject(alarm_json, "key", "The number of TPD tracks loaded exceeded the upper limit");
		}
		if (language == 2) {
			cJSON_AddStringToObject(alarm_json, "key", "TPDトラックロード数オーバーラン");
		}
		if (pre_state->tpd_num_limit != 1) {
			my_syslog("警告", "TPD轨迹加载数量超限", cur_account.username);
			my_en_syslog("alarm", "The number of TPD tracks loaded exceeded the upper limit", cur_account.username);
			my_jap_syslog("戒告する", "TPDトラックロード数オーバーラン", cur_account.username);
			pre_state->tpd_num_limit = 1;
		}
	} else {
		pre_state->tpd_num_limit = 0;
	}
	if (state->reg_var.num_full == 1) {
		if (language == 0) {
			cJSON_AddStringToObject(alarm_json, "key", "数值型变量监控个数已满");
		}
		if (language == 1) {
			cJSON_AddStringToObject(alarm_json, "key", "The number of numeric variables monitored was full. Procedure");
		}
		if (language == 2) {
			cJSON_AddStringToObject(alarm_json, "key", "数値型変数のモニタリング数は満杯");
		}
		if (pre_state->reg_var.num_full != 1) {
			my_syslog("警告", "数值型变量监控个数已满", cur_account.username);
			my_en_syslog("alarm", "The number of numeric variables monitored was full. Procedure", cur_account.username);
			my_jap_syslog("戒告する", "数値型変数のモニタリング数は満杯", cur_account.username);
			pre_state->reg_var.num_full = 1;
		}
	} else {
		pre_state->reg_var.num_full = 0;
	}
	if (state->reg_var.str_full == 1) {
		if (language == 0) {
			cJSON_AddStringToObject(alarm_json, "key", "字符型变量监控个数已满");
		}
		if (language == 1) {
			cJSON_AddStringToObject(alarm_json, "key", "The number of character variables monitored was full. Procedure");
		}
		if (language == 2) {
			cJSON_AddStringToObject(alarm_json, "key", "文字型変数の監視個数が満杯");
		}
		if (pre_state->reg_var.str_full != 1) {
			my_syslog("警告", "字符型变量监控个数已满", cur_account.username);
			my_en_syslog("alarm", "The number of character variables monitored was full. Procedure", cur_account.username);
			my_jap_syslog("戒告する", "文字型変数の監視個数が満杯", cur_account.username);
			pre_state->reg_var.str_full = 1;
		}
	} else {
		pre_state->reg_var.str_full = 0;
	}
	if (state->socket_conn_timeout >= 1) {
		memset(content, 0, sizeof(content));
		memset(en_content, 0, sizeof(en_content));
		memset(jap_content, 0, sizeof(jap_content));
		sprintf(content, "socket_%d: 连接超时", (state->socket_conn_timeout - 1));
		sprintf(en_content, "socket_%d: connection timeout", (state->socket_conn_timeout - 1));
		sprintf(jap_content, "ソケット%d: 接続タイムアウト", (state->socket_conn_timeout - 1));
		if (language == 0) {
			cJSON_AddStringToObject(alarm_json, "key", content);
		}
		if (language == 1) {
			cJSON_AddStringToObject(alarm_json, "key", en_content);
		}
		if (language == 2) {
			cJSON_AddStringToObject(alarm_json, "key", jap_content);
		}
		if (pre_state->socket_conn_timeout != 1) {
			my_syslog("警告", content, cur_account.username);
			my_en_syslog("alarm", en_content, cur_account.username);
			my_jap_syslog("戒告する", jap_content, cur_account.username);
			pre_state->socket_conn_timeout = 1;
		}
	} else {
		pre_state->socket_conn_timeout = 0;
	}
	if (state->socket_read_timeout >= 1) {
		memset(content, 0, sizeof(content));
		memset(en_content, 0, sizeof(en_content));
		memset(jap_content, 0, sizeof(jap_content));
		sprintf(content, "socket_%d: 读取超时", (state->socket_conn_timeout - 1));
		sprintf(en_content, "socket_%d: read timeout", (state->socket_conn_timeout - 1));
		sprintf(jap_content, "ソケット%d: 読み取りタイムアウト", (state->socket_conn_timeout - 1));
		if (language == 0) {
			cJSON_AddStringToObject(alarm_json, "key", content);
		}
		if (language == 1) {
			cJSON_AddStringToObject(alarm_json, "key", en_content);
		}
		if (language == 2) {
			cJSON_AddStringToObject(alarm_json, "key", jap_content);
		}
		if (pre_state->socket_read_timeout != 1) {
			my_syslog("警告", content, cur_account.username);
			my_en_syslog("alarm", en_content, cur_account.username);
			my_jap_syslog("戒告する", jap_content, cur_account.username);
			pre_state->socket_read_timeout = 1;
		}
	} else {
		pre_state->socket_read_timeout = 0;
	}
	if (state->btn_box_stop_signal == 1) {
		if (language == 0) {
			cJSON_AddStringToObject(alarm_json, "key", "按钮盒急停已按下");
		}
		if (language == 1) {
			cJSON_AddStringToObject(alarm_json, "key", "The button box emergency stop has been pressed");
		}
		if (language == 2) {
			cJSON_AddStringToObject(alarm_json, "key", "ボタンボックスが押されました");
		}
		if (pre_state->btn_box_stop_signal != 1) {
			my_syslog("警告", "按钮盒急停已按下", cur_account.username);
			my_en_syslog("alarm", "The button box emergency stop has been pressed", cur_account.username);
			my_jap_syslog("戒告する", "ボタンボックスが押されました", cur_account.username);
			pre_state->btn_box_stop_signal = 1;
		}
	} else {
		pre_state->btn_box_stop_signal = 0;
	}
	if (state->strangePosFlag == 1) {
		if (language == 0) {
			cJSON_AddStringToObject(alarm_json, "key", "当前处于奇异位姿");
		}
		if (language == 1) {
			cJSON_AddStringToObject(alarm_json, "key", "It is currently in a singular position");
		}
		if (language == 2) {
			cJSON_AddStringToObject(alarm_json, "key", "今は不思議な姿をしている");
		}
		if (pre_state->strangePosFlag != 1) {
			my_syslog("警告", "当前处于奇异位姿", cur_account.username);
			my_en_syslog("alarm", "It is currently in a singular position", cur_account.username);
			my_jap_syslog("戒告する", "今は不思議な姿をしている", cur_account.username);
			pre_state->strangePosFlag = 1;
		}
	} else {
		pre_state->strangePosFlag = 0;
	}
	if (state->drag_alarm == 1) {
		if (language == 0) {
			cJSON_AddStringToObject(alarm_json, "key", "拖动警告, 当前处于自动模式");
		}
		if (language == 1) {
			cJSON_AddStringToObject(alarm_json, "key", "Drag warning, currently in automatic mode");
		}
		if (language == 2) {
			cJSON_AddStringToObject(alarm_json, "key", "警告をドラッグすると、自動モードになります");
		}
		if (pre_state->drag_alarm != 1) {
			my_syslog("警告", "拖动警告, 当前处于自动模式", cur_account.username);
			my_en_syslog("alarm", "Drag warning, currently in automatic mode", cur_account.username);
			my_jap_syslog("戒告する", "警告をドラッグすると、自動モードになります", cur_account.username);
			pre_state->drag_alarm = 1;
		}
	} else {
		pre_state->drag_alarm = 0;
	}
	if (state->robot_mode == 0 && state->program_state == 4) {
		if (language == 0) {
			cJSON_AddStringToObject(alarm_json, "key", "切换拖动状态失败");
		}
		if (language == 1) {
			cJSON_AddStringToObject(alarm_json, "key", "Failed to toggle drag state");
		}
		if (language == 2) {
			cJSON_AddStringToObject(alarm_json, "key", "ドラッグ状態の切り替えに失敗しました");
		}
		if (pre_state->robot_mode != 1) {
			my_syslog("警告", "切换拖动状态失败", cur_account.username);
			my_en_syslog("alarm", "Failed to toggle drag state", cur_account.username);
			my_jap_syslog("戒告する", "ドラッグ状態の切り替えに失敗しました", cur_account.username);
			pre_state->robot_mode = 1;
		}
	} else {
		pre_state->robot_mode = 0;
	}
	switch(state->alarm) {
		case 1:
			if (language == 0) {
				cJSON_AddStringToObject(alarm_json, "key", "肩关节配置变化");
			}
			if (language == 1) {
				cJSON_AddStringToObject(alarm_json, "key", "Shoulder joint configuration changes");
			}
			if (language == 2) {
				cJSON_AddStringToObject(alarm_json, "key", "肩関節の配置変化");
			}
			if (pre_state->alarm != 1) {
				my_syslog("警告", "肩关节配置变化", cur_account.username);
				my_en_syslog("alarm", "Shoulder joint configuration changes", cur_account.username);
				my_jap_syslog("戒告する", "肩関節の配置変化", cur_account.username);
				pre_state->alarm = 1;
			}
			break;
		case 2:
			if (language == 0) {
				cJSON_AddStringToObject(alarm_json, "key", "肘关节配置变化");
			}
			if (language == 1) {
				cJSON_AddStringToObject(alarm_json, "key", "Elbow  joint configuration changes");
			}
			if (language == 2) {
				cJSON_AddStringToObject(alarm_json, "key", "肘関節の配置変化");
			}
			if (pre_state->alarm != 2) {
				my_syslog("警告", "肘关节配置变化", cur_account.username);
				my_en_syslog("alarm", "Elbow  joint configuration changes", cur_account.username);
				my_jap_syslog("戒告する", "肘関節の配置変化", cur_account.username);
				pre_state->alarm = 2;
			}
			break;
		case 3:
			if (language == 0) {
				cJSON_AddStringToObject(alarm_json, "key", "腕关节配置变化");
			}
			if (language == 1) {
				cJSON_AddStringToObject(alarm_json, "key", "Wrist  joint configuration changes");
			}
			if (language == 2) {
				cJSON_AddStringToObject(alarm_json, "key", "腕関節の配置変化");
			}
			if (pre_state->alarm != 3) {
				my_syslog("警告", "腕关节配置变化", cur_account.username);
				my_en_syslog("alarm", "Wrist  joint configuration changes", cur_account.username);
				my_jap_syslog("戒告する", "腕関節の配置変化", cur_account.username);
				pre_state->alarm = 3;
			}
			break;
		case 4:
			if (language == 0) {
				cJSON_AddStringToObject(alarm_json, "key", "RPY初始化失败");
			}
			if (language == 1) {
				cJSON_AddStringToObject(alarm_json, "key", "RPY initialization failure");
			}
			if (language == 2) {
				cJSON_AddStringToObject(alarm_json, "key", "rpyの初期化に失敗する");
			}
			if (pre_state->alarm != 4) {
				my_syslog("警告", "RPY初始化失败", cur_account.username);
				my_en_syslog("alarm", "RPY initialization failure", cur_account.username);
				my_jap_syslog("戒告する", "rpyの初期化に失敗する", cur_account.username);
				pre_state->alarm = 4;
			}
			break;
		case 5:
			if (language == 0) {
				cJSON_AddStringToObject(alarm_json, "key", "警告: WaitDI 等待超时");
			}
			if (language == 1) {
				cJSON_AddStringToObject(alarm_json, "key", "Warning: WaitDI wait for a timeout");
			}
			if (language == 2) {
				cJSON_AddStringToObject(alarm_json, "key", "戒告する: waitdiタイムアウトを待つ");
			}
			if (pre_state->alarm != 4) {
				my_syslog("警告", "警告: WaitDI 等待超时", cur_account.username);
				my_en_syslog("alarm", "Warning: WaitDI wait for a timeout", cur_account.username);
				my_jap_syslog("戒告する", "戒告する: waitdiタイムアウトを待つ", cur_account.username);
				pre_state->alarm = 4;
			}
			break;
		case 6:
			if (language == 0) {
				cJSON_AddStringToObject(alarm_json, "key", "警告: WaitAI 等待超时");
			}
			if (language == 1) {
				cJSON_AddStringToObject(alarm_json, "key", "Warning: WaitAI wait for a timeout");
			}
			if (language == 2) {
				cJSON_AddStringToObject(alarm_json, "key", "戒告する: waitaiタイムアウト待ち");
			}
			if (pre_state->alarm != 4) {
				my_syslog("警告", "警告: WaitAI 等待超时", cur_account.username);
				my_en_syslog("alarm", "Warning: WaitAI wait for a timeout", cur_account.username);
				my_jap_syslog("戒告する", "戒告する: waitaiタイムアウト待ち", cur_account.username);
				pre_state->alarm = 4;
			}
			break;
		case 7:
			if (language == 0) {
				cJSON_AddStringToObject(alarm_json, "key", "警告: WaitToolDI 等待超时");
			}
			if (language == 1) {
				cJSON_AddStringToObject(alarm_json, "key", "Warning: WaitToolDI wait for a timeout");
			}
			if (language == 2) {
				cJSON_AddStringToObject(alarm_json, "key", "戒告する: waittooldiタイムアウトを待つ");
			}
			if (pre_state->alarm != 4) {
				my_syslog("警告", "警告: WaitToolDI 等待超时", cur_account.username);
				my_en_syslog("alarm", "Warning: WaitToolDI wait for a timeout", cur_account.username);
				my_jap_syslog("戒告する", "戒告する: waittooldiタイムアウトを待つ", cur_account.username);
				pre_state->alarm = 4;
			}
			break;
		case 8:
			if (language == 0) {
				cJSON_AddStringToObject(alarm_json, "key", "警告: WaitToolAI 等待超时");
			}
			if (language == 1) {
				cJSON_AddStringToObject(alarm_json, "key", "Warning: WaitToolAI wait for a timeout");
			}
			if (language == 2) {
				cJSON_AddStringToObject(alarm_json, "key", "戒告する: waittoolaiタイムアウトを待つ");
			}
			if (pre_state->alarm != 4) {
				my_syslog("警告", "警告: WaitToolAI 等待超时", cur_account.username);
				my_en_syslog("alarm", "Warning: WaitToolAI wait for a timeout", cur_account.username);
				my_jap_syslog("戒告する", "戒告する: waittoolaiタイムアウトを待つ", cur_account.username);
				pre_state->alarm = 4;
			}
			break;
		case 9:
			if (language == 0) {
				cJSON_AddStringToObject(alarm_json, "key", "警告: 起弧成功 DI 未配置");
			}
			if (language == 1) {
				cJSON_AddStringToObject(alarm_json, "key", "Warning: Arcing success DI is not configured");
			}
			if (language == 2) {
				cJSON_AddStringToObject(alarm_json, "key", "戒告する: 起弧成功di未配置");
			}
			if (pre_state->alarm != 4) {
				my_syslog("警告", "警告: 起弧成功 DI 未配置", cur_account.username);
				my_en_syslog("alarm", "Warning: Arcing success DI is not configured", cur_account.username);
				my_jap_syslog("戒告する", "戒告する: 起弧成功di未配置", cur_account.username);
				pre_state->alarm = 4;
			}
			break;
		default:
			pre_state->alarm = 0;
			break;
	}
	if (state->safetydoor_alarm == 1) {
		if (language == 0) {
			cJSON_AddStringToObject(alarm_json, "key", "安全停止（安全门）触发");
		}
		if (language == 1) {
			cJSON_AddStringToObject(alarm_json, "key", "Safety stop (door) trigger");
		}
		if (language == 2) {
			cJSON_AddStringToObject(alarm_json, "key", "安全停止(セーフゲート)トリガー");
		}
		if (pre_state->safetydoor_alarm != 1) {
			my_syslog("警告", "安全停止（安全门）触发", cur_account.username);
			my_en_syslog("alarm", "Safety stop (door) trigger", cur_account.username);
			my_jap_syslog("戒告する", "安全停止(セーフゲート)トリガー", cur_account.username);
			pre_state->safetydoor_alarm = 1;
		}
	} else {
		pre_state->safetydoor_alarm = 0;
	}
	if (state->motionAlarm == 1) {
		if (language == 0) {
			cJSON_AddStringToObject(alarm_json, "key", "警告： LIN 指令姿态变化过大");
		}
		if (language == 1) {
			cJSON_AddStringToObject(alarm_json, "key", "Warning: The LIN command posture has changed too much");
		}
		if (language == 2) {
			cJSON_AddStringToObject(alarm_json, "key", "戒告する: linコマンドの姿勢変化が大きすぎる");
		}
		if (pre_state->motionAlarm != 1) {
			my_syslog("警告", "警告： LIN指令姿态变化过大", cur_account.username);
			my_en_syslog("alarm", "Warning: The LIN command posture has changed too much", cur_account.username);
			my_jap_syslog("戒告する", "戒告する: linコマンドの姿勢変化が大きすぎる", cur_account.username);
			pre_state->motionAlarm = 1;
		}
	} else {
		pre_state->motionAlarm = 0;
	}
	if (state->motionAlarm == 2) {
		if (language == 0) {
			cJSON_AddStringToObject(alarm_json, "key", "警告： TCP 超速");
		}
		if (language == 1) {
			cJSON_AddStringToObject(alarm_json, "key", "Warning: TCP over speeding");
		}
		if (language == 2) {
			cJSON_AddStringToObject(alarm_json, "key", "戒告する: TCPスピードオーバー");
		}
		if (pre_state->motionAlarm != 2) {
			my_syslog("警告", "警告： TCP 超速", cur_account.username);
			my_en_syslog("alarm", "Warning: TCP over speeding", cur_account.username);
			my_jap_syslog("戒告する", "戒告する: TCPスピードオーバー", cur_account.username);
			pre_state->motionAlarm = 2;
		}
	} else {
		pre_state->motionAlarm = 0;
	}
	if (state->motionAlarm == 3) {
		if (language == 0) {
			cJSON_AddStringToObject(alarm_json, "key", "警告： 发生碰撞");
		}
		if (language == 1) {
			cJSON_AddStringToObject(alarm_json, "key", "Warning: A collision");
		}
		if (language == 2) {
			cJSON_AddStringToObject(alarm_json, "key", "戒告する: 衝突する");
		}
		if (pre_state->motionAlarm != 3) {
			my_syslog("警告", "警告： 发生碰撞", cur_account.username);
			my_en_syslog("alarm", "Warning: A collision", cur_account.username);
			my_jap_syslog("戒告する", "戒告する: 衝突する", cur_account.username);
			pre_state->motionAlarm = 3;
		}
	} else {
		pre_state->motionAlarm = 0;
	}
	if (state->interfereAlarm == 1) {
		if (language == 0) {
			cJSON_AddStringToObject(alarm_json, "key", "警告：进入干涉区");
		}
		if (language == 1) {
			cJSON_AddStringToObject(alarm_json, "key", "Warning: Entering interference zone");
		}
		if (language == 2) {
			cJSON_AddStringToObject(alarm_json, "key", "警告:干渉領域に入る");
		}
		if (pre_state->interfereAlarm != 1) {
			my_syslog("警告", "警告：进入干涉区", cur_account.username);
			my_en_syslog("alarm", "Warning: Entering interference zone", cur_account.username);
			my_jap_syslog("戒告する", "警告:干渉領域に入る", cur_account.username);
			pre_state->interfereAlarm = 1;
		}
	} else {
		pre_state->interfereAlarm = 0;
	}

	/** error_info */
	if (point_home_info.error_flag == 1) {
		if (language == 0) {
			cJSON_AddStringToObject(error_json, "key", "原点已发生改变，需要重新设置原点");
		}
		if (language == 1) {
			cJSON_AddStringToObject(error_json, "key", "The origin point has changed and needs to reset origin point");
		}
		if (language == 2) {
			cJSON_AddStringToObject(error_json, "key", "原点が変わったので、原点をリセットする必要があります");
		}
		if (point_home_info.pre_error_flag != 1) {
			my_syslog("错误", "原点已发生改变，需要重新设置原点", cur_account.username);
			my_en_syslog("error", "The origin point has changed and needs to reset origin point", cur_account.username);
			my_jap_syslog("さくご", "原点が変わったので、原点をリセットする必要があります", cur_account.username);
			point_home_info.pre_error_flag = 1;
		}
	} else {
		point_home_info.pre_error_flag = 0;
	}
	if (state->aliveSlaveNumError == 1) {
		memset(content, 0, sizeof(content));
		memset(en_content, 0, sizeof(en_content));
		memset(jap_content, 0, sizeof(jap_content));
		sprintf(content, "活动从站数量错误，活动从站数量为:%d", state->aliveSlaveNumFeedback);
		sprintf(en_content, "Number of active slave stations is wrong. Number of active slave stations is:%d", state->aliveSlaveNumFeedback);
		sprintf(jap_content, "キャンペーンスレーブは誤作動し、キャンペーンスレーブは次のようになる。", state->aliveSlaveNumFeedback);
		if (language == 0) {
			cJSON_AddStringToObject(error_json, "key", content);
		}
		if (language == 1) {
			cJSON_AddStringToObject(error_json, "key", en_content);
		}
		if (language == 2) {
			cJSON_AddStringToObject(error_json, "key", jap_content);
		}
		if (pre_state->aliveSlaveNumError != 1) {
			my_syslog("错误", content, cur_account.username);
			my_en_syslog("error", en_content, cur_account.username);
			my_jap_syslog("さくご", jap_content, cur_account.username);
			pre_state->aliveSlaveNumError = 1;
		}
	} else {
		pre_state->aliveSlaveNumError = 0;
	}
	switch(state->gripperFaultNum) {
		case 1:
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", "夹爪485超时");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "Claw 485 timed out");
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", "はさみ爪485タイムアウト");
			}
			if (pre_state->gripperFaultNum != 1) {
				my_syslog("错误", "夹爪485超时", cur_account.username);
				my_en_syslog("error", "Claw 485 timed out", cur_account.username);
				my_jap_syslog("さくご", "はさみ爪485タイムアウト", cur_account.username);
				pre_state->gripperFaultNum = 1;
			}
			break;
		case 2:
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", "夹爪指令格式错误");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "Incorrect format of claw instruction");
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", "クリップコマンドのフォーマットが間違っています");
			}
			if (pre_state->gripperFaultNum != 2) {
				my_syslog("错误", "夹爪指令格式错误", cur_account.username);
				my_en_syslog("error", "Incorrect format of claw instruction", cur_account.username);
				my_jap_syslog("さくご", "クリップコマンドのフォーマットが間違っています", cur_account.username);
				pre_state->gripperFaultNum = 2;
			}
			break;
		case 5:
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", "夹爪动作延迟，须先激活");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "Claw action delay, must be activated first");
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", "爪の動作が遅れて、まず起動しなければならない");
			}
			if (pre_state->gripperFaultNum != 5) {
				my_syslog("错误", "夹爪动作延迟，须先激活", cur_account.username);
				my_en_syslog("error", "Claw action delay, must be activated first", cur_account.username);
				my_jap_syslog("さくご", "爪の動作が遅れて、まず起動しなければならない", cur_account.username);
				pre_state->gripperFaultNum = 5;
			}
			break;
		case 7:
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", "夹爪未激活");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "Claw not active");
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", "クリップが作動していない");
			}
			if (pre_state->gripperFaultNum != 7) {
				my_syslog("错误", "夹爪未激活", cur_account.username);
				my_en_syslog("error", "Claw not active", cur_account.username);
				my_jap_syslog("さくご", "クリップが作動していない", cur_account.username);
				pre_state->gripperFaultNum = 7;
			}
			break;
		case 8:
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", "夹爪温度过高");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "Claw temperature is too high");
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", "爪の温度が高すぎる");
			}
			if (pre_state->gripperFaultNum != 8) {
				my_syslog("错误", "夹爪温度过高", cur_account.username);
				my_en_syslog("error", "Claw temperature is too high", cur_account.username);
				my_jap_syslog("さくご", "爪の温度が高すぎる", cur_account.username);
				pre_state->gripperFaultNum = 8;
			}
			break;
		case 10:
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", "夹爪电压过低");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "Claw voltage is too low");
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", "クリップ電圧が低すぎる");
			}
			if (pre_state->gripperFaultNum != 10) {
				my_syslog("错误", "夹爪电压过低", cur_account.username);
				my_en_syslog("error", "Claw voltage is too low", cur_account.username);
				my_jap_syslog("さくご", "クリップ電圧が低すぎる", cur_account.username);
				pre_state->gripperFaultNum = 10;
			}
			break;
		case 11:
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", "夹爪正在自动释放");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "Claw is releasing automatically");
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", "クリップが自動的に解放されている");
			}
			if (pre_state->gripperFaultNum != 11) {
				my_syslog("错误", "夹爪正在自动释放", cur_account.username);
				my_en_syslog("error", "Claw is releasing automatically", cur_account.username);
				my_jap_syslog("さくご", "クリップが自動的に解放されている", cur_account.username);
				pre_state->gripperFaultNum = 11;
			}
			break;
		case 12:
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", "夹爪内部故障");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "Internal failure of clamping claw");
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", "クリップ爪の内部が故障している");
			}
			if (pre_state->gripperFaultNum != 12) {
				my_syslog("错误", "夹爪内部故障", cur_account.username);
				my_en_syslog("error", "Internal failure of clamping claw", cur_account.username);
				my_jap_syslog("さくご", "クリップ爪の内部が故障している", cur_account.username);
				pre_state->gripperFaultNum = 12;
			}
			break;
		case 13:
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", "夹爪激活失败");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "Claw activation failed");
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", "クリップの起動に失敗する");
			}
			if (pre_state->gripperFaultNum != 13) {
				my_syslog("错误", "夹爪激活失败", cur_account.username);
				my_en_syslog("error", "Claw activation failed", cur_account.username);
				my_jap_syslog("さくご", "クリップの起動に失敗する", cur_account.username);
				pre_state->gripperFaultNum = 13;
			}
			break;
		case 14:
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", "夹爪电流过大");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "The gripper current is too large");
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", "クリップ電流が大きすぎる");
			}
			if (pre_state->gripperFaultNum != 14) {
				my_syslog("错误", "夹爪电流过大", cur_account.username);
				my_en_syslog("error", "The gripper current is too large", cur_account.username);
				my_jap_syslog("さくご", "クリップ電流が大きすぎる", cur_account.username);
				pre_state->gripperFaultNum = 14;
			}
			break;
		case 15:
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", "夹爪自动释放结束");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "Claw automatic release end");
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", "クリップの自動解放が終了します");
			}
			if (pre_state->gripperFaultNum != 15) {
				my_syslog("错误", "夹爪自动释放结束", cur_account.username);
				my_en_syslog("error", "Claw automatic release end", cur_account.username);
				my_jap_syslog("さくご", "クリップの自動解放が終了します", cur_account.username);
				pre_state->gripperFaultNum = 15;
			}
			break;
		default:
			pre_state->gripperFaultNum = 0;
			break;
	}
	switch(state->slaveComError[0]) {
		case 1:
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", "控制箱从站掉线");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "Control box salve offline");
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", "制御ボックスが駅から外れた");
			}
			if (pre_state->slaveComError[0] != 1) {
				my_syslog("错误", "控制箱从站掉线", cur_account.username);
				my_en_syslog("error", "Control box salve offline", cur_account.username);
				my_jap_syslog("さくご", "制御ボックスが駅から外れた", cur_account.username);
				pre_state->slaveComError[0] = 1;
			}
			break;
		case 2:
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", "控制箱从站状态与设置值不一致");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "Control box slave status is not consistent with the set value");
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", "制御ボックスはステーション状態と設定値が一致しません");
			}
			if (pre_state->slaveComError[0] != 2) {
				my_syslog("错误", "控制箱从站状态与设置值不一致", cur_account.username);
				my_en_syslog("error", "Control box slave status is not consistent with the set value", cur_account.username);
				my_jap_syslog("さくご", "制御ボックスはステーション状態と設定値が一致しません", cur_account.username);
				pre_state->slaveComError[0] = 2;
			}
			break;
		case 3:
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", "控制箱从站未配置");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "Control box slave is not configured");
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", "制御ボックスは従局未配置");
			}
			if (pre_state->slaveComError[0] != 3) {
				my_syslog("错误", "控制箱从站未配置", cur_account.username);
				my_en_syslog("error", "Control box slave is not configured", cur_account.username);
				my_jap_syslog("さくご", "制御ボックスは従局未配置", cur_account.username);
				pre_state->slaveComError[0] = 3;
			}
			break;
		case 4:
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", "控制箱从站配置错误");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "Control box slave configure error");
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", "制御ボックスがステーションから誤って配置されています");
			}
			if (pre_state->slaveComError[0] != 4) {
				my_syslog("错误", "控制箱从站配置错误", cur_account.username);
				my_en_syslog("error", "Control box slave configure error", cur_account.username);
				my_jap_syslog("さくご", "制御ボックスがステーションから誤って配置されています", cur_account.username);
				pre_state->slaveComError[0] = 4;
			}
			break;
		case 5:
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", "控制箱从站初始化错误");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "Control box slave initialize error");
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", "コントロールボックスはステーションからエラーを初期化します");
			}
			if (pre_state->slaveComError[0] != 5) {
				my_syslog("错误", "控制箱从站初始化错误", cur_account.username);
				my_en_syslog("error", "Control box slave initialize error", cur_account.username);
				my_jap_syslog("さくご", "コントロールボックスはステーションからエラーを初期化します", cur_account.username);
				pre_state->slaveComError[0] = 5;
			}
			break;
		case 6:
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", "控制箱从站邮箱通信初始化错误");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "Control box slave mailbox communication initialize error");
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", "制御ボックスはステーションメールボックスから通信初期化エラー");
			}
			if (pre_state->slaveComError[0] != 6) {
				my_syslog("错误", "控制箱从站邮箱通信初始化错误", cur_account.username);
				my_en_syslog("error", "Control box slave mailbox communication initialize error", cur_account.username);
				my_jap_syslog("さくご", "制御ボックスはステーションメールボックスから通信初期化エラー", cur_account.username);
				pre_state->slaveComError[0] = 6;
			}
			break;
		default:
			pre_state->slaveComError[0] = 0;
			break;
	}
	for (i = 1; i <= 6; i++) {
		memset(content, 0, sizeof(content));
		memset(en_content, 0, sizeof(en_content));
		memset(jap_content, 0, sizeof(jap_content));
		switch(state->slaveComError[i]) {
			case 1:
				sprintf(content, "%d 轴从站掉线", i);
				sprintf(en_content, "%d axis salve offline", i);
				sprintf(jap_content, "%d 軸が駅から抜ける", i);
				if (language == 0) {
					cJSON_AddStringToObject(error_json, "key", content);
				}
				if (language == 1) {
					cJSON_AddStringToObject(error_json, "key", en_content);
				}
				if (language == 2) {
					cJSON_AddStringToObject(error_json, "key", jap_content);
				}
				if (pre_state->slaveComError[i] != 1) {
					my_syslog("错误", content, cur_account.username);
					my_en_syslog("error", en_content, cur_account.username);
					my_jap_syslog("さくご", jap_content, cur_account.username);
					pre_state->slaveComError[i] = 1;
				}
				break;
			case 2:
				sprintf(content, "%d 轴从站状态与设置值不一致", i);
				sprintf(en_content, "%d axis slave status is not consistent with the set value", i);
				sprintf(jap_content, "%d 軸スレーブステーションの状態と設定値が一致しない", i);
				if (language == 0) {
					cJSON_AddStringToObject(error_json, "key", content);
				}
				if (language == 1) {
					cJSON_AddStringToObject(error_json, "key", en_content);
				}
				if (language == 2) {
					cJSON_AddStringToObject(error_json, "key", jap_content);
				}
				if (pre_state->slaveComError[i] != 2) {
					my_syslog("错误", content, cur_account.username);
					my_en_syslog("error", en_content, cur_account.username);
					my_jap_syslog("さくご", jap_content, cur_account.username);
					pre_state->slaveComError[i] = 2;
				}
				break;
			case 3:
				sprintf(content, "%d 轴从站未配置", i);
				sprintf(en_content, "%d axis slave is not configured", i);
				sprintf(jap_content, "%d 軸スレーブステーションは未配置", i);
				if (language == 0) {
					cJSON_AddStringToObject(error_json, "key", content);
				}
				if (language == 1) {
					cJSON_AddStringToObject(error_json, "key", en_content);
				}
				if (language == 2) {
					cJSON_AddStringToObject(error_json, "key", jap_content);
				}
				if (pre_state->slaveComError[i] != 3) {
					my_syslog("错误", content, cur_account.username);
					my_en_syslog("error", en_content, cur_account.username);
					my_jap_syslog("さくご", jap_content, cur_account.username);
					pre_state->slaveComError[i] = 3;
				}
				break;
			case 4:
				sprintf(content, "%d 轴从站配置错误", i);
				sprintf(en_content, "%d axis slave configure error", i);
				sprintf(jap_content, "%d 軸スレーブステーション配置ミス", i);
				if (language == 0) {
					cJSON_AddStringToObject(error_json, "key", content);
				}
				if (language == 1) {
					cJSON_AddStringToObject(error_json, "key", en_content);
				}
				if (language == 2) {
					cJSON_AddStringToObject(error_json, "key", jap_content);
				}
				if (pre_state->slaveComError[i] != 4) {
					my_syslog("错误", content, cur_account.username);
					my_en_syslog("error", en_content, cur_account.username);
					my_jap_syslog("さくご", jap_content, cur_account.username);
					pre_state->slaveComError[i] = 4;
				}
				break;
			case 5:
				sprintf(content, "%d 轴从站初始化错误", i);
				sprintf(en_content, "%d axis slave initialize error", i);
				sprintf(jap_content, "%d 軸スレーブステーション初期化エラー", i);
				if (language == 0) {
					cJSON_AddStringToObject(error_json, "key", content);
				}
				if (language == 1) {
					cJSON_AddStringToObject(error_json, "key", en_content);
				}
				if (language == 2) {
					cJSON_AddStringToObject(error_json, "key", jap_content);
				}
				if (pre_state->slaveComError[i] != 5) {
					my_syslog("错误", content, cur_account.username);
					my_en_syslog("error", en_content, cur_account.username);
					my_jap_syslog("さくご", jap_content, cur_account.username);
					pre_state->slaveComError[i] = 5;
				}
				break;
			case 6:
				sprintf(content, "%d 轴从站邮箱通信初始化错误", i);
				sprintf(en_content, "%d axis slave mailbox communication initialize error", i);
				sprintf(jap_content, "%d 軸スレーブメールボックス通信初期化エラー", i);
				if (language == 0) {
					cJSON_AddStringToObject(error_json, "key", content);
				}
				if (language == 1) {
					cJSON_AddStringToObject(error_json, "key", en_content);
				}
				if (language == 2) {
					cJSON_AddStringToObject(error_json, "key", jap_content);
				}
				if (pre_state->slaveComError[i] != 6) {
					my_syslog("错误", content, cur_account.username);
					my_en_syslog("error", en_content, cur_account.username);
					my_jap_syslog("さくご", jap_content, cur_account.username);
					pre_state->slaveComError[i] = 6;
				}
				break;
			default:
				pre_state->slaveComError[i] = 0;
				break;
		}
	}
	switch(state->slaveComError[7]) {
		case 1:
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", "末端从站掉线");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "The terminal salve offline");
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", "末端が駅から途切れた");
			}
			if (pre_state->slaveComError[7] != 1) {
				my_syslog("错误", "末端从站掉线", cur_account.username);
				my_en_syslog("error", "The terminal salve offline", cur_account.username);
				my_jap_syslog("さくご", "末端が駅から途切れた", cur_account.username);
				pre_state->slaveComError[7] = 1;
			}
			break;
		case 2:
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", "末端从站状态与设置值不一致");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "The terminal slave status is not consistent with the set value");
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", "終端スレーブ局の状態と設定値が一致しない");
			}
			if (pre_state->slaveComError[7] != 2) {
				my_syslog("错误", "末端从站状态与设置值不一致", cur_account.username);
				my_en_syslog("error", "The terminal slave status is not consistent with the set value", cur_account.username);
				my_jap_syslog("さくご", "終端スレーブ局の状態と設定値が一致しない", cur_account.username);
				pre_state->slaveComError[7] = 2;
			}
			break;
		case 3:
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", "末端从站未配置");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "The terminal slave is not configured");
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", "末端スレーブ局は未配置");
			}
			if (pre_state->slaveComError[7] != 3) {
				my_syslog("错误", "末端从站未配置", cur_account.username);
				my_en_syslog("error", "The terminal slave is not configured", cur_account.username);
				my_jap_syslog("さくご", "末端スレーブ局は未配置", cur_account.username);
				pre_state->slaveComError[7] = 3;
			}
			break;
		case 4:
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", "末端从站配置错误");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "The terminal  slave configure error");
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", "エンドスレーブ局の設定が間違っています");
			}
			if (pre_state->slaveComError[7] != 4) {
				my_syslog("错误", "末端从站配置错误", cur_account.username);
				my_en_syslog("error", "The terminal slave configure error", cur_account.username);
				my_jap_syslog("さくご", "エンドスレーブ局の設定が間違っています", cur_account.username);
				pre_state->slaveComError[7] = 4;
			}
			break;
		case 5:
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", "末端从站初始化错误");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "The terminal  slave initialize error");
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", "エンドスレーブ初期化エラー");
			}
			if (pre_state->slaveComError[7] != 5) {
				my_syslog("错误", "末端从站初始化错误", cur_account.username);
				my_en_syslog("error", "The terminal  slave initialize error", cur_account.username);
				my_jap_syslog("さくご", "エンドスレーブ初期化エラー", cur_account.username);
				pre_state->slaveComError[7] = 5;
			}
			break;
		case 6:
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", "末端从站邮箱通信初始化错误");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "The terminal  slave  mailbox communication initialize error");
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", "終端メールボックスからの通信初期化エラー");
			}
			if (pre_state->slaveComError[7] != 6) {
				my_syslog("错误", "末端从站邮箱通信初始化错误", cur_account.username);
				my_en_syslog("error", "The terminal  slave  mailbox communication initialize error", cur_account.username);
				my_jap_syslog("さくご", "終端メールボックスからの通信初期化エラー", cur_account.username);
				pre_state->slaveComError[7] = 6;
			}
			break;
		default:
			pre_state->slaveComError[7] = 0;
			break;
	}
	//printf("state->cmdPointError = %d", state->cmdPointError);
	switch(state->cmdPointError) {
		case 1:
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", "关节指令点错误, 可复位");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "Joint command point error, can be reset");
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", "関節の指令点が間違っている, リセット可能");
			}
			if (pre_state->cmdPointError != 1) {
				my_syslog("错误", "关节指令点错误, 可复位", cur_account.username);
				my_en_syslog("error", "Joint command point error, can be reset", cur_account.username);
				my_jap_syslog("さくご", "関節の指令点が間違っている, リセット可能", cur_account.username);
				pre_state->cmdPointError = 1;
			}
			break;
		case 2:
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", "直线目标点错误（包括工具不符）, 可复位");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "Straight line target point error(including tool discrepancy), can be reset");
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", "直線の目標点が間違っている(道具が違っていることも含む), リセット可能");
			}
			if (pre_state->cmdPointError != 2) {
				my_syslog("错误", "直线目标点错误（包括工具不符）, 可复位", cur_account.username);
				my_en_syslog("error", "Straight line target point error(including tool discrepancy), can be reset", cur_account.username);
				my_jap_syslog("さくご", "直線の目標点が間違っている(道具が違っていることも含む), リセット可能", cur_account.username);
				pre_state->cmdPointError = 2;
			}
			break;
		case 3:
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", "圆弧中间点错误（包括工具不符）, 可复位");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "Midpoint of arc error (including tool discrepancy), can be reset");
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", "円弧の中間点のエラー(ツールの不一致を含む), リセット可能");
			}
			if (pre_state->cmdPointError != 3) {
				my_syslog("错误", "圆弧中间点错误（包括工具不符）, 可复位", cur_account.username);
				my_en_syslog("error", "Midpoint of arc error (including tool discrepancy), can be reset", cur_account.username);
				my_jap_syslog("さくご", "円弧の中間点のエラー(ツールの不一致を含む), リセット可能", cur_account.username);
				pre_state->cmdPointError = 3;
			}
			break;
		case 4:
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", "圆弧目标点错误（包括工具不符）, 可复位");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "Arc target point error (including tool discrepancy), can be reset");
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", "円弧の目標点エラー(ツールが一致しないことを含む), リセット可能");
			}
			if (pre_state->cmdPointError != 4) {
				my_syslog("错误", "圆弧目标点错误（包括工具不符）, 可复位", cur_account.username);
				my_en_syslog("error", "Arc target point error (including tool discrepancy), can be reset", cur_account.username);
				my_jap_syslog("さくご", "円弧の目標点エラー(ツールが一致しないことを含む), リセット可能", cur_account.username);
				pre_state->cmdPointError = 4;
			}
			break;
		case 5:
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", "圆弧指令点间距过小, 可复位");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "The distance between arc instruction points is too small, can be reset");
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", "円弧コマンド点の間隔が小さすぎる, リセット可能");
			}
			if (pre_state->cmdPointError != 5) {
				my_syslog("错误", "圆弧指令点间距过小, 可复位", cur_account.username);
				my_en_syslog("error", "The distance between arc instruction points is too small, can be reset", cur_account.username);
				my_jap_syslog("さくご", "円弧コマンド点の間隔が小さすぎる, リセット可能", cur_account.username);
				pre_state->cmdPointError = 5;
			}
			break;
		case 6:
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", "整圆/螺旋线指令中间点1错误（包括工具不符），可复位");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "Full circle/helix instruction midpoint 1 error (including tool mismatch), resettable");
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", "全円/螺旋線指令中間点1エラー(工具が合わないことを含む),リセット可能");
			}
			if (pre_state->cmdPointError != 6) {
				my_syslog("错误", "整圆/螺旋线指令中间点1错误（包括工具不符），可复位", cur_account.username);
				my_en_syslog("error", "Full circle/helix instruction midpoint 1 error (including tool mismatch), resettable", cur_account.username);
				my_jap_syslog("さくご", "全円/螺旋線指令中間点1エラー(工具が合わないことを含む),リセット可能", cur_account.username);
				pre_state->cmdPointError = 6;
			}
			break;
		case 7:
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", "整圆/螺旋线指令中间点2错误（包括工具不符），可复位");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "Full circle/helix instruction midpoint 2 error (including tool mismatch), resettable");
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", "全円/螺旋線指令中間点2エラー(工具が合わないことを含む),リセット可能");
			}
			if (pre_state->cmdPointError != 7) {
				my_syslog("错误", "整圆/螺旋线指令中间点2错误（包括工具不符），可复位", cur_account.username);
				my_en_syslog("error", "Full circle/helix instruction midpoint 2 error (including tool mismatch), resettable", cur_account.username);
				my_jap_syslog("さくご", "全円/螺旋線指令中間点2エラー(工具が合わないことを含む),リセット可能", cur_account.username);
				pre_state->cmdPointError = 7;
			}
			break;
		case 8:
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", "整圆/螺旋线指令中间点3错误（包括工具不符），可复位");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "Full circle/helix instruction midpoint 3 error (including tool mismatch), resettable");
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", "全円/螺旋線指令中間点3エラー(工具が合わないことを含む),リセット可能");
			}
			if (pre_state->cmdPointError != 8) {
				my_syslog("错误", "整圆/螺旋线指令中间点3错误（包括工具不符），可复位", cur_account.username);
				my_en_syslog("error", "Full circle/helix instruction midpoint 3 error (including tool mismatch), resettable", cur_account.username);
				my_jap_syslog("さくご", "全円/螺旋線指令中間点3エラー(工具が合わないことを含む),リセット可能", cur_account.username);
				pre_state->cmdPointError = 8;
			}
			break;
		case 9:
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", "整圆/螺旋线指令点间距过小，可复位");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "Full circle/helix command point spacing is too small, resettable");
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", "全円/螺旋線コマンドピッチが小さすぎて、リセット可能");
			}
			if (pre_state->cmdPointError != 9) {
				my_syslog("错误", "整圆/螺旋线指令点间距过小，可复位", cur_account.username);
				my_en_syslog("error", "Full circle/helix command point spacing is too small, resettable", cur_account.username);
				my_jap_syslog("さくご", "全円/螺旋線コマンドピッチが小さすぎて、リセット可能", cur_account.username);
				pre_state->cmdPointError = 9;
			}
			break;
		case 10:
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", "TPD指令点错误，可复位");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "TPD point error, can be reset");
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", "TPD指令点エラー, リセット可能");
			}
			if (pre_state->cmdPointError != 10) {
				my_syslog("错误", "TPD指令点错误，可复位", cur_account.username);
				my_en_syslog("error", "TPD point error, can be reset", cur_account.username);
				my_jap_syslog("さくご", "TPD指令点エラー, リセット可能", cur_account.username);
				pre_state->cmdPointError = 10;
			}
			break;
		case 11:
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", "TPD指令工具与当前工具不符, 可复位");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "TPD instruction tool does not match the current tool, can be reset");
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", "TPDコマンドツールは、現在のツールと一致しません, リセット可能");
			}
			if (pre_state->cmdPointError != 11) {
				my_syslog("错误", "TPD指令工具与当前工具不符, 可复位", cur_account.username);
				my_en_syslog("error", "TPD instruction tool does not match the current tool, can be reset", cur_account.username);
				my_jap_syslog("さくご", "TPDコマンドツールは、現在のツールと一致しません, リセット可能", cur_account.username);
				pre_state->cmdPointError = 11;
			}
			break;
		case 12:
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", "TPD当前指令与下一指令起始点偏差过大, 可复位");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "TPD the current instruction deviates too much from the starting point of the next instruction, can be reset");
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", "TPD現在の命令は、次の命令の開始点から過大にずれている, リセット可能");
			}
			if (pre_state->cmdPointError != 12) {
				my_syslog("错误", "TPD当前指令与下一指令起始点偏差过大, 可复位", cur_account.username);
				my_en_syslog("error", "TPD the current instruction deviates too much from the starting point of the next instruction, can be reset", cur_account.username);
				my_jap_syslog("さくご", "TPD現在の命令は、次の命令の開始点から過大にずれている, リセット可能", cur_account.username);
				pre_state->cmdPointError = 12;
			}
			break;
		case 13:
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", "内外部工具切换错误, 可复位");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "Error switching internal and external tools, can be reset");
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", "内部ツールと外部ツールの切り替えエラー, リセット可能");
			}
			if (pre_state->cmdPointError != 13) {
				my_syslog("错误", "内外部工具切换错误, 可复位", cur_account.username);
				my_en_syslog("error", "Error switching internal and external tools, can be reset", cur_account.username);
				my_jap_syslog("さくご", "内部ツールと外部ツールの切り替えエラー, リセット可能", cur_account.username);
				pre_state->cmdPointError = 13;
			}
			break;
		case 17:
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", "PTP关节指令超限, 可复位");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "PTP joint instruction out of limit, can be reset");
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", "ptp関節コマンドオーバー, リセット可能");
			}
			if (pre_state->cmdPointError != 17) {
				my_syslog("错误", "PTP关节指令超限, 可复位", cur_account.username);
				my_en_syslog("error", "PTP joint instruction out of limit, can be reset", cur_account.username);
				my_jap_syslog("さくご", "ptp関節コマンドオーバー, リセット可能", cur_account.username);
				pre_state->cmdPointError = 17;
			}
			break;
		case 18:
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", "TPD关节指令超限, 可复位");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "TPD joint instruction out of limit, can be reset");
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", "tpd関節コマンドオーバー, リセット可能");
			}
			if (pre_state->cmdPointError != 18) {
				my_syslog("错误", "TPD关节指令超限, 可复位", cur_account.username);
				my_en_syslog("error", "TPD joint instruction out of limit, can be reset", cur_account.username);
				my_jap_syslog("さくご", "tpd関節コマンドオーバー, リセット可能", cur_account.username);
				pre_state->cmdPointError = 18;
			}
			break;
		case 19:
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", "LIN/ARC下发关节指令超限, 可复位");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "LIN/ARC offering joint command out of limit, can be reset");
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", "LIN/ARC関節指令を出してオーバーランさせる, リセット可能");
			}
			if (pre_state->cmdPointError != 19) {
				my_syslog("错误", "LIN/ARC下发关节指令超限, 可复位", cur_account.username);
				my_en_syslog("error", "LIN/ARC offering joint command out of limit, can be reset", cur_account.username);
				my_jap_syslog("さくご", "LIN/ARC関節指令を出してオーバーランさせる, リセット可能", cur_account.username);
				pre_state->cmdPointError = 19;
			}
			break;
		case 20:
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", "笛卡尔空间内指令超速, 不可复位");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "Overspeed command in Cartesian space, do not reset");
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", "デカルト空間内ではスピードの出しすぎを指示する, リセット不可");
			}
			if (pre_state->cmdPointError != 20) {
				my_syslog("错误", "笛卡尔空间内指令超速, 不可复位", cur_account.username);
				my_en_syslog("error", "Overspeed command in Cartesian space, do not reset", cur_account.username);
				my_jap_syslog("さくご", "デカルト空間内ではスピードの出しすぎを指示する, リセット不可", cur_account.username);
				pre_state->cmdPointError = 20;
			}
			break;
		case 21:
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", "关节空间内扭矩指令超限, 可复位");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "Excessive torque command in joint space, can be reset");
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", "関節空間内トルク指令オーバー, リセット可能");
			}
			if (pre_state->cmdPointError != 21) {
				my_syslog("错误", "关节空间内扭矩指令超限, 可复位", cur_account.username);
				my_en_syslog("error", "Excessive torque command in joint space, can be reset", cur_account.username);
				my_jap_syslog("さくご", "関節空間内トルク指令オーバー, リセット可能", cur_account.username);
				pre_state->cmdPointError = 21;
			}
			break;
		case 22:
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", "JOG关节指令超限, 可复位");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "JOG joint instruction out of limit, can be reset");
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", "JOG関節コマンドオーバー, リセット可能");
			}
			if (pre_state->cmdPointError != 22) {
				my_syslog("错误", "JOG关节指令超限, 可复位", cur_account.username);
				my_en_syslog("error", "JOG joint instruction out of limit, can be reset", cur_account.username);
				my_jap_syslog("さくご", "JOG関節コマンドオーバー, リセット可能", cur_account.username);
				pre_state->cmdPointError = 22;
			}
			break;
		case 23:
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", "轴1关节空间内指令速度超限, 可复位");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "Axis 1 overrun command velocity in joint space, can be reset");
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", "軸1関節空間内指令速度オーバー, リセット可能");
			}
			if (pre_state->cmdPointError != 23) {
				my_syslog("错误", "轴1关节空间内指令速度超限, 可复位", cur_account.username);
				my_en_syslog("error", "Axis 1 joint overrun command velocity in joint space, can be reset", cur_account.username);
				my_jap_syslog("さくご", "軸1関節空間内指令速度オーバー, リセット可能", cur_account.username);
				pre_state->cmdPointError = 23;
			}
			break;
		case 24:
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", "轴2关节空间内指令速度超限, 可复位");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "Axis 2 overrun command velocity in joint space, can be reset");
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", "軸2関節空間内指令速度オーバー, リセット可能");
			}
			if (pre_state->cmdPointError != 24) {
				my_syslog("错误", "轴2关节空间内指令速度超限, 可复位", cur_account.username);
				my_en_syslog("error", "Axis 2 overrun command velocity in joint space, can be reset", cur_account.username);
				my_jap_syslog("さくご", "軸2関節空間内指令速度オーバー, リセット可能", cur_account.username);
				pre_state->cmdPointError = 24;
			}
			break;
		case 25:
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", "轴3关节空间内指令速度超限, 可复位");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "Axis 3 overrun command velocity in joint space, can be reset");
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", "軸3関節空間内指令速度オーバー, リセット可能");
			}
			if (pre_state->cmdPointError != 25) {
				my_syslog("错误", "轴3关节空间内指令速度超限, 可复位", cur_account.username);
				my_en_syslog("error", "Axis 3 overrun command velocity in joint space, can be reset", cur_account.username);
				my_jap_syslog("さくご", "軸3関節空間内指令速度オーバー, リセット可能", cur_account.username);
				pre_state->cmdPointError = 25;
			}
			break;
		case 26:
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", "轴4关节空间内指令速度超限, 可复位");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "Axis 4 overrun command velocity in joint space, can be reset");
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", "軸4関節空間内指令速度オーバー, リセット可能");
			}
			if (pre_state->cmdPointError != 26) {
				my_syslog("错误", "轴4关节空间内指令速度超限, 可复位", cur_account.username);
				my_en_syslog("error", "Axis 4 overrun command velocity in joint space, can be reset", cur_account.username);
				my_jap_syslog("さくご", "軸4関節空間内指令速度オーバー, リセット可能", cur_account.username);
				pre_state->cmdPointError = 26;
			}
			break;
		case 27:
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", "轴5关节空间内指令速度超限, 可复位");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "Axis 5 overrun command velocity in joint space, can be reset");
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", "軸5関節空間内指令速度オーバー, リセット可能");
			}
			if (pre_state->cmdPointError != 27) {
				my_syslog("错误", "轴5关节空间内指令速度超限, 可复位", cur_account.username);
				my_en_syslog("error", "Axis 5 overrun command velocity in joint space, can be reset", cur_account.username);
				my_jap_syslog("さくご", "軸5関節空間内指令速度オーバー, リセット可能", cur_account.username);
				pre_state->cmdPointError = 27;
			}
			break;
		case 28:
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", "轴6关节空间内指令速度超限, 可复位");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "Axis 6 overrun command velocity in joint space, can be reset");
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", "軸6関節空間内指令速度オーバー, リセット可能");
			}
			if (pre_state->cmdPointError != 28) {
				my_syslog("错误", "轴6关节空间内指令速度超限, 可复位", cur_account.username);
				my_en_syslog("error", "Axis 6 overrun command velocity in joint space, can be reset", cur_account.username);
				my_jap_syslog("さくご", "軸6関節空間内指令速度オーバー, リセット可能", cur_account.username);
				pre_state->cmdPointError = 28;
			}
			break;
		case 33:
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", "下一指令关节配置发生变化 (下一指令中存在奇异位姿，请使用 PTP 指令或更改下一指令点)，可复位");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "The next instruction changes the joint configuration (Singularity pose exists in the next instruction, please use PTP instruction or change the next instruction point), which can be reset");
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", "次の指令関節の配置が変わる (次のコマンドの中で奇妙な姿勢が存在して、PTPコマンドを使用してくださいまたは次のコマンドポイントを変更して)、リセットすることができます");
			}
			if (pre_state->cmdPointError != 33) {
				my_syslog("错误", "下一指令关节配置发生变化 (下一指令中存在奇异位姿，请使用 PTP 指令或更改下一指令点)，可复位", cur_account.username);
				my_en_syslog("error", "The next instruction changes the joint configuration (Singularity pose exists in the next instruction, please use PTP instruction or change the next instruction point), which can be reset", cur_account.username);
				my_jap_syslog("さくご", "次の指令関節の配置が変わる (次のコマンドの中で奇妙な姿勢が存在して、PTPコマンドを使用してくださいまたは次のコマンドポイントを変更して)、リセットすることができます", cur_account.username);
				pre_state->cmdPointError = 33;
			}
			break;
		case 34:
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", "当前指令关节配置发生变化 （当前指令中存在奇异位姿，请使用 PTP 指令或更改当前指令点），可复位");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "The current command joint configuration has changed (singular pose exists in the current command, please use PTP command or change the current command point), which can be reset");
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", "現在のコマンドに特異な姿勢が存在する場合は、PTPコマンドを使用するか、現在のコマンドポイントを変更してリセット可能にしてください。");
			}
			if (pre_state->cmdPointError != 34) {
				my_syslog("错误", "当前指令关节配置发生变化 （当前指令中存在奇异位姿，请使用 PTP 指令或更改当前指令点），可复位", cur_account.username);
				my_en_syslog("error", "The current command joint configuration has changed (singular pose exists in the current command, please use PTP command or change the current command point), which can be reset", cur_account.username);
				my_jap_syslog("さくご", "現在のコマンドに特異な姿勢が存在する場合は、PTPコマンドを使用するか、現在のコマンドポイントを変更してリセット可能にしてください。", cur_account.username);
				pre_state->cmdPointError = 34;
			}
			break;
		case 49:
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", "焊接指令错误，ARCSTART 和 ARCEND 之间只允许 LIN 和 ARC 指令, 可复位");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "Incorrect welding instruction. Only LIN and ARC instructions are allowed between ARCSTART and ARCEND, can be reset");
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", "ハンダ付け命令が間違っていて、ARCSTARTとARCENDの間でLINとARC命令しか許されない, リセット可能");
			}
			if (pre_state->cmdPointError != 49) {
				my_syslog("错误", "焊接指令错误，ARCSTART 和 ARCEND 之间只允许 LIN 和 ARC 指令, 可复位", cur_account.username);
				my_en_syslog("error", "Incorrect welding instruction. Only LIN and ARC instructions are allowed between ARCSTART and ARCEND, can be reset", cur_account.username);
				my_jap_syslog("さくご", "ハンダ付け命令が間違っていて、ARCSTARTとARCENDの間でLINとARC命令しか許されない, リセット可能", cur_account.username);
				pre_state->cmdPointError = 49;
			}
			break;
		case 50:
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", "焊接指令错误， WEAVESTART 和 WEAVEEND 之间只允许 LIN 指令, 可复位");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "Incorrect welding instruction. Only LIN instructions are allowed between WEAVESTART and WEAVEEND, can be reset");
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", "ハンダ付け命令が間違っていて、WEAVESTARTとWEAVEENDの間でLIN命令しか許されない, リセット可能");
			}
			if (pre_state->cmdPointError != 50) {
				my_syslog("错误", "焊接指令错误，WEAVESTART 和 WEAVEEND 之间只允许 LIN 指令, 可复位", cur_account.username);
				my_en_syslog("error", "Incorrect welding instruction. Only LIN instructions are allowed between WEAVESTART and WEAVEEND, can be reset", cur_account.username);
				my_jap_syslog("さくご", "ハンダ付け命令が間違っていて、WEAVESTARTとWEAVEENDの間でLIN命令しか許されない, リセット可能", cur_account.username);
				pre_state->cmdPointError = 50;
			}
			break;
		case 51:
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", "摆焊参数错误, 可复位");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "Swing welding parameter error, can be reset");
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", "振り子溶接パラメータが間違っている, リセット可能");
			}
			if (pre_state->cmdPointError != 51) {
				my_syslog("错误", "摆焊参数错误, 可复位", cur_account.username);
				my_en_syslog("error", "Swing welding parameter error, can be reset", cur_account.username);
				my_jap_syslog("さくご", "振り子溶接パラメータが間違っている, リセット可能", cur_account.username);
				pre_state->cmdPointError = 51;
			}
			break;
		case 65:
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", "激光传感器指令偏差过大, 可复位");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "Laser sensor instruction deviation is too large, can be reset");
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", "レーザーセンサーの指令偏差が大きすぎる, リセット可能");
			}
			if (pre_state->cmdPointError != 65) {
				my_syslog("错误", "激光传感器指令偏差过大, 可复位", cur_account.username);
				my_en_syslog("error", "Laser sensor instruction deviation is too large, can be reset", cur_account.username);
				my_jap_syslog("さくご", "レーザーセンサーの指令偏差が大きすぎる, リセット可能", cur_account.username);
				pre_state->cmdPointError = 65;
			}
			break;
		case 66:
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", "激光传感器指令中断, 焊缝跟踪提前结束, 可复位");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "Laser sensor instruction is interrupted, weld tracking ends prematurely, can be reset");
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", "レーザーセンサは中断され、溶接部の追跡は早期に終了します, リセット可能");
			}
			if (pre_state->cmdPointError != 66) {
				my_syslog("错误", "激光传感器指令中断, 焊缝跟踪提前结束, 可复位", cur_account.username);
				my_en_syslog("error", "Laser sensor instruction is interrupted, weld tracking ends prematurely, can be reset", cur_account.username);
				my_jap_syslog("さくご", "レーザーセンサは中断され、溶接部の追跡は早期に終了します, リセット可能", cur_account.username);
				pre_state->cmdPointError = 66;
			}
			break;
		case 81:
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", "外部轴指令速度超限, 可复位");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "External shaft instruction speed over limit, can be reset");
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", "外部軸指令速度オーバー, リセット可能");
			}
			if (pre_state->cmdPointError != 81) {
				my_syslog("错误", "外部轴指令速度超限, 可复位", cur_account.username);
				my_en_syslog("error", "External shaft instruction speed over limit, can be reset", cur_account.username);
				my_jap_syslog("さくご", "外部軸指令速度オーバー, リセット可能", cur_account.username);
				pre_state->cmdPointError = 81;
			}
			break;
		case 82:
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", "外部轴指令与反馈偏差过大，不可复位，需要回零或重启");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "The deviation between the external axis instruction and the feedback is too large, which cannot be reset. It needs to be reset to zero or restart");
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", "外部軸指令とフィードバックのずれが大きすぎてリセットできないので、ゼロに戻すかリセットする必要があります");
			}
			if (pre_state->cmdPointError != 82) {
				my_syslog("错误", "外部轴指令与反馈偏差过大，不可复位，需要回零或重启", cur_account.username);
				my_en_syslog("error", "The deviation between the external axis instruction and the feedback is too large, which cannot be reset. It needs to be reset to zero or restart", cur_account.username);
				my_jap_syslog("さくご", "外部軸指令とフィードバックのずれが大きすぎてリセットできないので、ゼロに戻すかリセットする必要があります", cur_account.username);
				pre_state->cmdPointError = 82;
			}
			break;
		case 97:
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", "传送带跟踪-起始点与参考点姿态变化过大, 可复位");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "Belt tracking - starting point and reference point attitude change too much, can be reset");
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", "ベルトコンベヤー追跡-開始点と基準点の姿勢が変化しすぎている, リセット可能");
			}
			if (pre_state->cmdPointError != 97) {
				my_syslog("错误", "传送带跟踪-起始点与参考点姿态变化过大, 可复位", cur_account.username);
				my_en_syslog("error", "Belt tracking - starting point and reference point attitude change too much, can be reset", cur_account.username);
				my_jap_syslog("さくご", "ベルトコンベヤー追跡-開始点と基準点の姿勢が変化しすぎている, リセット可能", cur_account.username);
				pre_state->cmdPointError = 97;
			}
			break;
		case 113:
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", "恒力控制-X方向超过最大调整距离，可复位");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "Constant force control -X direction exceeds the maximum adjustment distance, can be reset");
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", "定力制御-X方向に最大調整距離を超え、リセット可能");
			}
			if (pre_state->cmdPointError != 113) {
				my_syslog("错误", "恒力控制-X方向超过最大调整距离，可复位", cur_account.username);
				my_en_syslog("error", "Constant force control -X direction exceeds the maximum adjustment distance, can be reset", cur_account.username);
				my_jap_syslog("さくご", "定力制御-X方向に最大調整距離を超え、リセット可能", cur_account.username);
				pre_state->cmdPointError = 113;
			}
			break;
		case 114:
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", "恒力控制-Y方向超过最大调整距离，可复位");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "Constant force control -Y direction exceeds the maximum adjustment distance, can be reset");
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", "定力制御-Y方向が最大調整距離を超えてリセット可能");
			}
			if (pre_state->cmdPointError != 114) {
				my_syslog("错误", "恒力控制-Y方向超过最大调整距离，可复位", cur_account.username);
				my_en_syslog("error", "Constant force control -Y direction exceeds the maximum adjustment distance, can be reset", cur_account.username);
				my_jap_syslog("さくご", "定力制御-Y方向が最大調整距離を超えてリセット可能", cur_account.username);
				pre_state->cmdPointError = 114;
			}
			break;
		case 115:
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", "恒力控制-Z方向超过最大调整距离，可复位");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "Constant force control -Z direction exceeds the maximum adjustment distance, can be reset");
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", "定力制御-Z方向最大調整距離を超えてリセット可能");
			}
			if (pre_state->cmdPointError != 115) {
				my_syslog("错误", "恒力控制-Z方向超过最大调整距离，可复位", cur_account.username);
				my_en_syslog("error", "Constant force control -Z direction exceeds the maximum adjustment distance, can be reset", cur_account.username);
				my_jap_syslog("さくご", "定力制御-Z方向最大調整距離を超えてリセット可能", cur_account.username);
				pre_state->cmdPointError = 115;
			}
			break;
		case 116:
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", "恒力控制-RX方向超过最大调整角度，可复位");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "Constant force control -RX direction exceeds the maximum adjustment Angle, can be reset");
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", "定力制御-RX方向を最大調整角度以上、リセット可能");
			}
			if (pre_state->cmdPointError != 116) {
				my_syslog("错误", "恒力控制-RX方向超过最大调整角度，可复位", cur_account.username);
				my_en_syslog("error", "Constant force control -RX direction exceeds the maximum adjustment Angle, can be reset", cur_account.username);
				my_jap_syslog("さくご", "定力制御-RX方向を最大調整角度以上、リセット可能", cur_account.username);
				pre_state->cmdPointError = 116;
			}
			break;
		case 117:
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", "恒力控制-RY方向超过最大调整角度，可复位");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "Constant force control -RY direction exceeds the maximum adjustment Angle, can be reset");
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", "力制御-RY方向が最大調整角度を超えてリセット可能");
			}
			if (pre_state->cmdPointError != 117) {
				my_syslog("错误", "恒力控制-RY方向超过最大调整角度，可复位", cur_account.username);
				my_en_syslog("error", "Constant force control -RY direction exceeds the maximum adjustment Angle, can be reset", cur_account.username);
				my_jap_syslog("さくご", "力制御-RY方向が最大調整角度を超えてリセット可能", cur_account.username);
				pre_state->cmdPointError = 117;
			}
			break;
		case 118:
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", "恒力控制-RZ方向超过最大调整角度，可复位");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "Constant force control -RZ direction exceeds the maximum adjustment Angle, can be reset");
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", "定力制御-RZ方向最大調整角度を超えてリセット可能");
			}
			if (pre_state->cmdPointError != 118) {
				my_syslog("错误", "恒力控制-RZ方向超过最大调整角度，可复位", cur_account.username);
				my_en_syslog("error", "Constant force control -RZ direction exceeds the maximum adjustment Angle, can be reset", cur_account.username);
				my_jap_syslog("さくご", "定力制御-RZ方向最大調整角度を超えてリセット可能", cur_account.username);
				pre_state->cmdPointError = 118;
			}
			break;
		default:
			pre_state->cmdPointError = 0;
			break;
	}
	switch(state->ioError) {
		case 1:
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", "通道错误");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "The channel error");
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", "チャネルエラー");
			}
			if (pre_state->ioError != 1) {
				my_syslog("错误", "通道错误", cur_account.username);
				my_en_syslog("error", "The channel error", cur_account.username);
				my_jap_syslog("さくご", "チャネルエラー", cur_account.username);
				pre_state->ioError = 1;
			}
			break;
		case 2:
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", "数值错误");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "numerical fault");
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", "数値エラー");
			}
			if (pre_state->ioError != 2) {
				my_syslog("错误", "数值错误", cur_account.username);
				my_en_syslog("error", "numerical fault", cur_account.username);
				my_jap_syslog("さくご", "数値エラー", cur_account.username);
				pre_state->ioError = 2;
			}
			break;
		case 3:
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", "WaitDI等待超时");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "WaitDI wait for a timeout");
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", "WaitDI タイムアウト待ち");
			}
			if (pre_state->ioError != 3) {
				my_syslog("错误", "WaitDI等待超时", cur_account.username);
				my_en_syslog("error", "WaitDI wait for a timeout", cur_account.username);
				my_jap_syslog("さくご", "WaitDI タイムアウト待ち", cur_account.username);
				pre_state->ioError = 3;
			}
			break;
		case 4:
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", "WaitAI等待超时");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "WaitAI wait for a timeout");
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", "WaitAI タイムアウト待ち");
			}
			if (pre_state->ioError != 4) {
				my_syslog("错误", "WaitAI等待超时", cur_account.username);
				my_en_syslog("error", "WaitAI wait for a timeout", cur_account.username);
				my_jap_syslog("さくご", "WaitAI タイムアウト待ち", cur_account.username);
				pre_state->ioError = 4;
			}
			break;
		case 5:
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", "WaitAxleDI等待超时");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "WaitAxleDI wait for a timeout");
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", "WaitAxleDI タイムアウト待ち");
			}
			if (pre_state->ioError != 5) {
				my_syslog("错误", "WaitAxleDI等待超时", cur_account.username);
				my_en_syslog("error", "WaitAxleDI wait for a timeout", cur_account.username);
				my_jap_syslog("さくご", "WaitAxleDI タイムアウト待ち", cur_account.username);
				pre_state->ioError = 5;
			}
			break;
		case 6:
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", "WaitAxleAI等待超时");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "WaitAxleAI wait for a timeout");
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", "WaitAxleAI タイムアウト待ち");
			}
			if (pre_state->ioError != 6) {
				my_syslog("错误", "WaitAxleAI等待超时", cur_account.username);
				my_en_syslog("error", "WaitAxleAI wait for a timeout", cur_account.username);
				my_jap_syslog("さくご", "WaitAxleAI タイムアウト待ち", cur_account.username);
				pre_state->ioError = 6;
			}
			break;
		case 7:
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", "通道已配置功能错误");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "The channel has been configured functionally wrong");
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", "チャンネルが設定されています");
			}
			if (pre_state->ioError != 7) {
				my_syslog("错误", "通道已配置功能错误", cur_account.username);
				my_en_syslog("error", "The channel has been configured functionally wrong", cur_account.username);
				my_jap_syslog("さくご", "チャンネルが設定されています", cur_account.username);
				pre_state->ioError = 7;
			}
			break;
		case 8:
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", "起弧超时");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "Striking a timeout");
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", "起弧タイムアウト");
			}
			if (pre_state->ioError != 8) {
				my_syslog("错误", "起弧超时", cur_account.username);
				my_en_syslog("error", "Striking a timeout", cur_account.username);
				my_jap_syslog("さくご", "起弧タイムアウト", cur_account.username);
				pre_state->ioError = 8;
			}
			break;
		case 9:
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", "收弧超时");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "Are-receive timeout");
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", "円弧タイムアウト");
			}
			if (pre_state->ioError != 9) {
				my_syslog("错误", "收弧超时", cur_account.username);
				my_en_syslog("error", "Are-receive timeout", cur_account.username);
				my_jap_syslog("さくご", "円弧タイムアウト", cur_account.username);
				pre_state->ioError = 9;
			}
			break;
		case 10:
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", "寻位超时");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "Find a timeout");
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", "アドレッシング・タイムアウト");
			}
			if (pre_state->ioError != 10) {
				my_syslog("错误", "寻位超时", cur_account.username);
				my_en_syslog("error", "Find a timeout", cur_account.username);
				my_jap_syslog("さくご", "アドレッシング・タイムアウト", cur_account.username);
				pre_state->ioError = 10;
			}
			break;
		case 11:
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", "传送带IO检测超时");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "IO detection of conveyor belt has expired");
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", "ベルトコンベヤーioがタイムアウトを検出する");
			}
			if (pre_state->ioError != 11) {
				my_syslog("错误", "传送带IO检测超时", cur_account.username);
				my_en_syslog("error", "IO detection of conveyor belt has expired", cur_account.username);
				my_jap_syslog("さくご", "ベルトコンベヤーioがタイムアウトを検出する", cur_account.username);
				pre_state->ioError = 11;
			}
			break;
		default:
			pre_state->ioError = 0;
			break;
	}
	if (state->gripperError == 1) {
		if (language == 0) {
			cJSON_AddStringToObject(error_json, "key", "夹爪运动超时错误");
		}
		if (language == 1) {
			cJSON_AddStringToObject(error_json, "key", "Claw movement timeout error");
		}
		if (language == 2) {
			cJSON_AddStringToObject(error_json, "key", "爪運動タイムアウトエラー");
		}
		if (pre_state->gripperError != 1) {
			my_syslog("错误", "夹爪运动超时错误", cur_account.username);
			my_en_syslog("error", "Claw movement timeout error", cur_account.username);
			my_jap_syslog("さくご", "爪運動タイムアウトエラー", cur_account.username);
			pre_state->gripperError = 1;
		}
	} else {
		pre_state->gripperError = 0;
	}
	switch(state->fileError) {
		case 1:
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", "zbt配置文件版本错误");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "zbt incorrect configuration file version");
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", "zbtプロファイルのバージョンが間違っています");
			}
			if (pre_state->fileError != 1) {
				my_syslog("错误", "zbt配置文件版本错误", cur_account.username);
				my_en_syslog("error", "zbt incorrect configuration file version", cur_account.username);
				my_jap_syslog("さくご", "zbtプロファイルのバージョンが間違っています", cur_account.username);
				pre_state->fileError = 1;
			}
			break;
		case 2:
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", "zbt配置文件加载失败");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "zbt the configuration file failed to load");
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", "zbtプロファイルのロードに失敗しました");
			}
			if (pre_state->fileError != 2) {
				my_syslog("错误", "zbt配置文件加载失败", cur_account.username);
				my_en_syslog("error", "zbt the configuration file failed to load", cur_account.username);
				my_jap_syslog("さくご", "zbtプロファイルのロードに失敗しました", cur_account.username);
				pre_state->fileError = 2;
			}
			break;
		case 3:
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", "user配置文件版本错误");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "user incorrect configuration file version");
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", "userプロファイルのバージョンが間違っています");
			}
			if (pre_state->fileError != 3) {
				my_syslog("错误", "user配置文件版本错误", cur_account.username);
				my_en_syslog("error", "user incorrect configuration file version", cur_account.username);
				my_jap_syslog("さくご", "userプロファイルのバージョンが間違っています", cur_account.username);
				pre_state->fileError = 3;
			}
			break;
		case 4:
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", "user配置文件加载失败");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "user the configuration file failed to load");
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", "userプロファイルの読み込みに失敗しました");
			}
			if (pre_state->fileError != 4) {
				my_syslog("错误", "user配置文件加载失败", cur_account.username);
				my_en_syslog("error", "user the configuration file failed to load", cur_account.username);
				my_jap_syslog("さくご", "userプロファイルの読み込みに失敗しました", cur_account.username);
				pre_state->fileError = 4;
			}
			break;
		case 5:
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", "exaxis配置文件版本错误");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "exaxis incorrect configuration file version");
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", "exaxisプロファイルのバージョンが間違っています");
			}
			if (pre_state->fileError != 5) {
				my_syslog("错误", "exaxis配置文件版本错误", cur_account.username);
				my_en_syslog("error", "exaxis incorrect configuration file version", cur_account.username);
				my_jap_syslog("さくご", "exaxisプロファイルのバージョンが間違っています", cur_account.username);
				pre_state->fileError = 5;
			}
			break;
		case 6:
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", "exaxis配置文件加载失败");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "exaxis the configuration file failed to load");
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", "exaxisプロファイルのロードに失敗しました");
			}
			if (pre_state->fileError != 6) {
				my_syslog("错误", "exaxis配置文件加载失败", cur_account.username);
				my_en_syslog("error", "exaxis the configuration file failed to load", cur_account.username);
				my_jap_syslog("さくご", "exaxisプロファイルのロードに失敗しました", cur_account.username);
				pre_state->fileError = 6;
			}
			break;
		case 7:
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", "机器人型号不一致，需要重新设置-不可复位");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "Robot models are inconsistent and need to be reset - not reset");
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", "ロボットの型番が一致していないので再設定が必要-リセット不可");
			}
			if (pre_state->fileError != 7) {
				my_syslog("错误", "机器人型号不一致，需要重新设置-不可复位", cur_account.username);
				my_en_syslog("error", "Robot models are inconsistent and need to be reset - not reset", cur_account.username);
				my_jap_syslog("さくご", "ロボットの型番が一致していないので再設定が必要-リセット不可", cur_account.username);
				pre_state->fileError = 7;
			}
			break;
		default:
			pre_state->fileError = 0;
			break;
	}
	//printf("state->paraError = %d\n", state->paraError);
	switch(state->paraError) {
		case 1:
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", "工具号超限错误");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "Error with tool number overrun");
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", "工具番号オーバーエラー");
			}
			if (pre_state->paraError != 1) {
				my_syslog("错误", "工具号超限错误", cur_account.username);
				my_en_syslog("error", "Error with tool number overrun", cur_account.username);
				my_jap_syslog("さくご", "工具番号オーバーエラー", cur_account.username);
				pre_state->paraError = 1;
			}
			break;
		case 2:
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", "定位完成阈值错误");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "Error in positioning completion threshold");
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", "定定完了しきい値エラー");
			}
			if (pre_state->paraError != 2) {
				my_syslog("错误", "定位完成阈值错误", cur_account.username);
				my_en_syslog("error", "Error in positioning completion threshold", cur_account.username);
				my_jap_syslog("さくご", "定定完了しきい値エラー", cur_account.username);
				pre_state->paraError = 2;
			}
			break;
		case 3:
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", "碰撞等级错误");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "Collision level error");
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", "衝突レベルの誤り");
			}
			if (pre_state->paraError != 3) {
				my_syslog("错误", "碰撞等级错误", cur_account.username);
				my_en_syslog("error", "Collision level error", cur_account.username);
				my_jap_syslog("さくご", "衝突レベルの誤り", cur_account.username);
				pre_state->paraError = 3;
			}
			break;
		case 4:
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", "负载重量错误");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "Load weight error");
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", "荷重重量の誤り");
			}
			if (pre_state->paraError != 4) {
				my_syslog("错误", "负载重量错误", cur_account.username);
				my_en_syslog("error", "Load weight error", cur_account.username);
				my_jap_syslog("さくご", "荷重重量の誤り", cur_account.username);
				pre_state->paraError = 4;
			}
			break;
		case 5:
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", "负载质心X错误");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "Load center of mass X error");
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", "荷重质心x間違って");
			}
			if (pre_state->paraError != 5) {
				my_syslog("错误", "负载质心X错误", cur_account.username);
				my_en_syslog("error", "Load center of mass X error", cur_account.username);
				my_jap_syslog("さくご", "荷重质心x間違って", cur_account.username);
				pre_state->paraError = 5;
			}
			break;
		case 6:
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", "负载质心Y错误");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "Load center of mass Y error");
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", "負荷重心Yエラー");
			}
			if (pre_state->paraError != 6) {
				my_syslog("错误", "负载质心Y错误", cur_account.username);
				my_en_syslog("error", "Load center of mass Y error", cur_account.username);
				my_jap_syslog("さくご", "負荷重心Yエラー", cur_account.username);
				pre_state->paraError = 6;
			}
			break;
		case 7:
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", "负载质心Z错误");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "Load center of mass Z error");
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", "負荷重心zエラー");
			}
			if (pre_state->paraError != 7) {
				my_syslog("错误", "负载质心Z错误", cur_account.username);
				my_en_syslog("error", "Load center of mass Z error", cur_account.username);
				my_jap_syslog("さくご", "負荷重心zエラー", cur_account.username);
				pre_state->paraError = 7;
			}
			break;
		case 8:
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", "DI滤波时间错误");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "DI filtering time error");
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", "diフィルタリング時間エラー");
			}
			if (pre_state->paraError != 8) {
				my_syslog("错误", "DI滤波时间错误", cur_account.username);
				my_en_syslog("error", "DI filtering time error", cur_account.username);
				my_jap_syslog("さくご", "diフィルタリング時間エラー", cur_account.username);
				pre_state->paraError = 8;
			}
			break;
		case 9:
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", "AxleDI滤波时间错误");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "AxleDI filtering time error");
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", "axlediフィルタリング時間エラー");
			}
			if (pre_state->paraError != 9) {
				my_syslog("错误", "AxleDI滤波时间错误", cur_account.username);
				my_en_syslog("error", "AxleDI filtering time error", cur_account.username);
				my_jap_syslog("さくご", "axlediフィルタリング時間エラー", cur_account.username);
				pre_state->paraError = 9;
			}
			break;
		case 10:
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", "AI滤波时间错误");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "AI filtering time error");
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", "aiは時間エラーをフィルタリングする");
			}
			if (pre_state->paraError != 10) {
				my_syslog("错误", "AI滤波时间错误", cur_account.username);
				my_en_syslog("error", "AI filtering time error", cur_account.username);
				my_jap_syslog("さくご", "aiは時間エラーをフィルタリングする", cur_account.username);
				pre_state->paraError = 10;
			}
			break;
		case 11:
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", "AxleAI滤波时间错误");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "AxleAI filtering time error");
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", "axleaiは時間エラーをフィルタリングする");
			}
			if (pre_state->paraError != 11) {
				my_syslog("错误", "AxleAI滤波时间错误", cur_account.username);
				my_en_syslog("error", "AxleAI filtering time error", cur_account.username);
				my_jap_syslog("さくご", "axleaiは時間エラーをフィルタリングする", cur_account.username);
				pre_state->paraError = 11;
			}
			break;
		case 12:
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", "DI高低电平范围错误");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "DI wrong range of high and low levels");
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", "diの高低の範囲が間違っている");
			}
			if (pre_state->paraError != 12) {
				my_syslog("错误", "DI高低电平范围错误", cur_account.username);
				my_en_syslog("error", "DI wrong range of high and low levels", cur_account.username);
				my_jap_syslog("さくご", "diの高低の範囲が間違っている", cur_account.username);
				pre_state->paraError = 12;
			}
			break;
		case 13:
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", "DO高低电平范围错误");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "DO wrong range of high and low levels");
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", "do高低範囲が間違っている");
			}
			if (pre_state->paraError != 13) {
				my_syslog("错误", "DO高低电平范围错误", cur_account.username);
				my_en_syslog("error", "DO wrong range of high and low levels", cur_account.username);
				my_jap_syslog("さくご", "do高低範囲が間違っている", cur_account.username);
				pre_state->paraError = 13;
			}
			break;
		case 14:
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", "工件号超限错误");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "Workpiece number out of limit error");
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", "工作物番号が間違っている");
			}
			if (pre_state->paraError != 14) {
				my_syslog("错误", "工件号超限错误", cur_account.username);
				my_en_syslog("error", "Workpiece number out of limit error", cur_account.username);
				my_jap_syslog("さくご", "工作物番号が間違っている", cur_account.username);
				pre_state->paraError = 14;
			}
			break;
		case 15:
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", "外部轴号超限错误");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "External shaft number overrun error");
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", "外軸号がエラーオーバー");
			}
			if (pre_state->paraError != 15) {
				my_syslog("错误", "外部轴号超限错误", cur_account.username);
				my_en_syslog("error", "External shaft number overrun error", cur_account.username);
				my_jap_syslog("さくご", "外軸号がエラーオーバー", cur_account.username);
				pre_state->paraError = 15;
			}
			break;
		case 16:
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", "传送带跟踪-编码器通道错误-可复位");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "Conveyor - encoder channel error - Reset");
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", "ベルトトラッカー-エンコーダチャンネルエラー-リセット可能");
			}
			if (pre_state->paraError != 16) {
				my_syslog("错误", "传送带跟踪-编码器通道错误-可复位", cur_account.username);
				my_en_syslog("error", "Conveyor - encoder channel error - Reset", cur_account.username);
				my_jap_syslog("さくご", "ベルトトラッカー-エンコーダチャンネルエラー-リセット可能", cur_account.username);
				pre_state->paraError = 16;
			}
			break;
		case 17:
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", "传送带跟踪-工件轴号错误-可复位");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "Conveyor - Workpiece Axis Number error - Reset");
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", "ベルトコンベヤートラッキング-ワーク軸番号エラー-リセット可能");
			}
			if (pre_state->paraError != 17) {
				my_syslog("错误", "传送带跟踪-工件轴号错误-可复位", cur_account.username);
				my_en_syslog("error", "Conveyor - Workpiece Axis Number error - Reset", cur_account.username);
				my_jap_syslog("さくご", "ベルトコンベヤートラッキング-ワーク軸番号エラー-リセット可能", cur_account.username);
				pre_state->paraError = 17;
			}
			break;
		default:
			pre_state->paraError = 0;
			break;
	}
	//printf("state->exaxis_out_slimit_error = %d\n", state->exaxis_out_slimit_error);
	switch(state->exaxis_out_slimit_error)
	{
		case 1:
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", "外部轴1轴超出软限位故障");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "External axis 1 axis out of soft limit fault");
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", "外軸1軸がソフトリミットを超えるトラブル");
			}
			if (pre_state->exaxis_out_slimit_error != 1) {
				my_syslog("错误", "外部轴1轴超出软限位故障", cur_account.username);
				my_en_syslog("error", "External axis 1 axis out of soft limit fault", cur_account.username);
				my_jap_syslog("さくご", "外軸1軸がソフトリミットを超えるトラブル", cur_account.username);
				pre_state->exaxis_out_slimit_error = 1;
			}
			break;
		case 2:
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", "外部轴2轴超出软限位故障");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "External axis 2 axis out of soft limit fault");
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", "外軸2軸がソフトリミットを超えるトラブル");
			}
			if (pre_state->exaxis_out_slimit_error != 2) {
				my_syslog("错误", "外部轴2轴超出软限位故障", cur_account.username);
				my_en_syslog("error", "External axis 2 axis out of soft limit fault", cur_account.username);
				my_jap_syslog("さくご", "外軸2軸がソフトリミットを超えるトラブル", cur_account.username);
				pre_state->exaxis_out_slimit_error = 2;
			}
			break;
		case 3:
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", "外部轴3轴超出软限位故障");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "External axis 3 axis out of soft limit fault");
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", "外軸3軸がソフトリミットを超えるトラブル");
			}
			if (pre_state->exaxis_out_slimit_error != 3) {
				my_syslog("错误", "外部轴3轴超出软限位故障", cur_account.username);
				my_en_syslog("error", "External axis 3 axis out of soft limit fault", cur_account.username);
				my_jap_syslog("さくご", "外軸3軸がソフトリミットを超えるトラブル", cur_account.username);
				pre_state->exaxis_out_slimit_error = 3;
			}
			break;
		case 4:
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", "外部轴4轴超出软限位故障");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "External axis 4 axis out of soft limit fault");
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", "外軸4軸がソフトリミットを超えるトラブル");
			}
			if (pre_state->exaxis_out_slimit_error != 4) {
				my_syslog("错误", "外部轴4轴超出软限位故障", cur_account.username);
				my_en_syslog("error", "External axis 4 axis out of soft limit fault", cur_account.username);
				my_jap_syslog("さくご", "外軸4軸がソフトリミットを超えるトラブル", cur_account.username);
				pre_state->exaxis_out_slimit_error = 4;
			}
			break;
		default:
			pre_state->exaxis_out_slimit_error = 0;
			break;
	}
	if (state->dr_com_err == 1) {
		if (language == 0) {
			cJSON_AddStringToObject(error_json, "key", "通信故障:控制器与驱动器心跳检测故障");
		}
		if (language == 1) {
			cJSON_AddStringToObject(error_json, "key", "Communication failure: controller and drive heartbeat detection failure");
		}
		if (language == 2) {
			cJSON_AddStringToObject(error_json, "key", "通信障害:コントローラとドライバの心拍検出に障害がある");
		}
		if (pre_state->dr_com_err != 1) {
			my_syslog("错误", "通信故障:控制器与驱动器心跳检测故障", cur_account.username);
			my_en_syslog("error", "Communication failure: controller and drive heartbeat detection failure", cur_account.username);
			my_jap_syslog("さくご", "通信障害:コントローラとドライバの心拍検出に障害がある", cur_account.username);
			pre_state->dr_com_err = 1;
		}
	} else {
		pre_state->dr_com_err = 0;
	}
	if ((int)state->dr_err != 0) {
		memset(content, 0, sizeof(content));
		memset(en_content, 0, sizeof(en_content));
		memset(jap_content, 0, sizeof(jap_content));
		sprintf(content, "%d 轴驱动器故障, 驱动器故障代码: %d", (int)state->dr_err, (int)state->dr_err_code);
		sprintf(en_content, "%d axis drive failure, drive failure code:%d", (int)state->dr_err, (int)state->dr_err_code);
		sprintf(jap_content, "%d シャフトドライブ障害ドライブ障害コード: %d", (int)state->dr_err, (int)state->dr_err_code);
		if (language == 0) {
			cJSON_AddStringToObject(error_json, "key", content);
		}
		if (language == 1) {
			cJSON_AddStringToObject(error_json, "key", en_content);
		}
		if (language == 2) {
			cJSON_AddStringToObject(error_json, "key", jap_content);
		}
		if (pre_state->dr_err != 1) {
			my_syslog("错误", content, cur_account.username);
			my_en_syslog("error", en_content, cur_account.username);
			my_jap_syslog("さくご", jap_content, cur_account.username);
			pre_state->dr_err = 1;
		}
	} else {
		pre_state->dr_err = 0;
	}
	if ((int)state->out_sflimit_err != 0) {
		memset(content, 0, sizeof(content));
		memset(en_content, 0, sizeof(en_content));
		memset(jap_content, 0, sizeof(jap_content));
		sprintf(content, "%d 轴超出软限位故障, 可复位", (int)state->out_sflimit_err);
		sprintf(en_content, "%d axis out of soft limit fault, can be reset", (int)state->out_sflimit_err);
		sprintf(jap_content, "%d シャフトがソフトリミットを超えて故障する, リセット可能", (int)state->out_sflimit_err);
		if (language == 0) {
			cJSON_AddStringToObject(error_json, "key", content);
		}
		if (language == 1) {
			cJSON_AddStringToObject(error_json, "key", en_content);
		}
		if (language == 2) {
			cJSON_AddStringToObject(error_json, "key", jap_content);
		}
		if (pre_state->out_sflimit_err != 1) {
			my_syslog("错误", content, cur_account.username);
			my_en_syslog("error", en_content, cur_account.username);
			my_jap_syslog("さくご", jap_content, cur_account.username);
			pre_state->out_sflimit_err = 1;
		}
	} else {
		pre_state->out_sflimit_err = 0;
	}
	if ((int)state->collision_err != 0) {
		memset(content, 0, sizeof(content));
		memset(en_content, 0, sizeof(en_content));
		memset(jap_content, 0, sizeof(jap_content));
		if ((int)state->collision_err == 7) {
			sprintf(content, "末端碰撞故障, 可复位", (int)state->collision_err);
			sprintf(en_content, "Terminal collision fault, can be reset", (int)state->collision_err);
			sprintf(jap_content, "末端衝突障害, リセット可能", (int)state->collision_err);
		} else {
			sprintf(content, "%d 轴碰撞故障, 可复位", (int)state->collision_err);
			sprintf(en_content, "%d axis impact fault, can be reset", (int)state->collision_err);
			sprintf(jap_content, "%d 軸衝突故障, リセット可能", (int)state->collision_err);
		}
		if (language == 0) { 
			cJSON_AddStringToObject(error_json, "key", content);
		}
		if (language == 1) {
			cJSON_AddStringToObject(error_json, "key", en_content);
		}
		if (language == 2) {
			cJSON_AddStringToObject(error_json, "key", jap_content);
		}
		if (pre_state->collision_err != 1) {
			my_syslog("错误", content, cur_account.username);
			my_en_syslog("error", en_content, cur_account.username);
			my_jap_syslog("さくご", jap_content, cur_account.username);
			pre_state->collision_err = 1;
		}
	} else {
		pre_state->collision_err = 0;
	}
	if (state->weld_readystate == 0) {
		if (language == 0) {
			cJSON_AddStringToObject(error_json, "key", "焊机未准备好");
		}
		if (language == 1) {
			cJSON_AddStringToObject(error_json, "key", "The welder is not ready");
		}
		if (language == 2) {
			cJSON_AddStringToObject(error_json, "key", "溶接機が準備できていない");
		}
		if (pre_state->weld_readystate != 1) {
			my_syslog("错误", "焊机未准备好", cur_account.username);
			my_en_syslog("error", "The welder is not ready", cur_account.username);
			my_jap_syslog("さくご", "溶接機が準備できていない", cur_account.username);
			pre_state->weld_readystate = 1;
		}
	} else {
		pre_state->weld_readystate = 0;
	}
	for (i = 0; i < 4; i++) {
		if ((int)state->exaxis_status[i].exAxisALM == 1) {
			memset(content, 0, sizeof(content));
			memset(en_content, 0, sizeof(en_content));
			memset(jap_content, 0, sizeof(jap_content));
			sprintf(content, "外部轴 %d 伺服报警", (i+1));
			sprintf(en_content, "exaxis %d servo alarm", (i+1));
			sprintf(jap_content, "外部軸 %d サーボアラーム", (i+1));
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", content);
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", en_content);
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", jap_content);
			}
			if (pre_state->exaxis_status[i].exAxisALM != 1) {
				my_syslog("错误", content, cur_account.username);
				my_en_syslog("error", en_content, cur_account.username);
				my_jap_syslog("さくご", jap_content, cur_account.username);
				pre_state->exaxis_status[i].exAxisALM = 1;
			}
		} else {
			pre_state->exaxis_status[i].exAxisALM = 0;
		}
	}
	for (i = 0; i < 4; i++) {
		if ((int)state->exaxis_status[i].exAxisFLERR == 1) {
			memset(content, 0, sizeof(content));
			memset(en_content, 0, sizeof(en_content));
			memset(jap_content, 0, sizeof(jap_content));
			sprintf(content, "外部轴 %d 跟随误差过大", (i+1));
			sprintf(en_content, "exaxis %d too much following error", (i+1));
			sprintf(jap_content, "外部軸 %d 追従誤差が大きすぎる", (i+1));
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", content);
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", en_content);
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", jap_content);
			}
			if (pre_state->exaxis_status[i].exAxisFLERR != 1) {
				my_syslog("错误", content, cur_account.username);
				my_en_syslog("error", en_content, cur_account.username);
				my_jap_syslog("さくご", jap_content, cur_account.username);
				pre_state->exaxis_status[i].exAxisFLERR = 1;
			}
		} else {
			pre_state->exaxis_status[i].exAxisFLERR = 0;
		}
	}
	for (i = 0; i < 4; i++) {
		if ((int)state->exaxis_status[i].exAxisNLMT == 1) {
			memset(content, 0, sizeof(content));
			memset(en_content, 0, sizeof(en_content));
			memset(jap_content, 0, sizeof(jap_content));
			sprintf(content, "外部轴 %d 到负限位", (i+1));
			sprintf(en_content, "exaxis %d to the negative limit", (i+1));
			sprintf(jap_content, "外部軸 %d 負のリミットまで", (i+1));
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", content);
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", en_content);
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", jap_content);
			}
			if (pre_state->exaxis_status[i].exAxisNLMT != 1) {
				my_syslog("错误", content, cur_account.username);
				my_en_syslog("error", en_content, cur_account.username);
				my_jap_syslog("さくご", jap_content, cur_account.username);
				pre_state->exaxis_status[i].exAxisNLMT = 1;
			}
		} else {
			pre_state->exaxis_status[i].exAxisNLMT = 0;
		}
	}
	for (i = 0; i < 4; i++) {
		if ((int)state->exaxis_status[i].exAxisPLMT == 1) {
			memset(content, 0, sizeof(content));
			memset(en_content, 0, sizeof(en_content));
			memset(jap_content, 0, sizeof(jap_content));
			sprintf(content, "外部轴 %d 到正限位", (i+1));
			sprintf(en_content, "exaxis %d to the forward limit", (i+1));
			sprintf(jap_content, "外部軸 %d 正のリミットまで", (i+1));
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", content);
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", en_content);
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", jap_content);
			}
			if (pre_state->exaxis_status[i].exAxisPLMT != 1) {
				my_syslog("错误", content, cur_account.username);
				my_en_syslog("error", en_content, cur_account.username);
				my_jap_syslog("さくご", jap_content, cur_account.username);
				pre_state->exaxis_status[i].exAxisPLMT = 1;
			}
		} else {
			pre_state->exaxis_status[i].exAxisPLMT = 0;
		}
	}
	for (i = 0; i < 4; i++) {
		if ((int)state->exaxis_status[i].exAxisAbsOFLN == 1) {
			memset(content, 0, sizeof(content));
			memset(en_content, 0, sizeof(en_content));
			memset(jap_content, 0, sizeof(jap_content));
			sprintf(content, "外部轴 %d 驱动器485总线掉线", (i+1));
			sprintf(en_content, "exaxis %d the driver 485 bus is disconnected", (i+1));
			sprintf(jap_content, "外部軸 %d ドライバ485バスが中断される", (i+1));
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", content);
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", en_content);
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", jap_content);
			}
			if (pre_state->exaxis_status[i].exAxisAbsOFLN != 1) {
				my_syslog("错误", content, cur_account.username);
				my_en_syslog("error", en_content, cur_account.username);
				my_jap_syslog("さくご", jap_content, cur_account.username);
				pre_state->exaxis_status[i].exAxisAbsOFLN = 1;
			}
		} else {
			pre_state->exaxis_status[i].exAxisAbsOFLN = 0;
		}
	}
	for (i = 0; i < 4; i++) {
		if ((int)state->exaxis_status[i].exAxisOFLIN == 1) {
			memset(content, 0, sizeof(content));
			memset(en_content, 0, sizeof(en_content));
			memset(jap_content, 0, sizeof(jap_content));
			sprintf(content, "外部轴 %d 通信超时，控制卡与控制箱板485通信超时", (i+1));
			sprintf(en_content, "exaxis %d communication timeout, control card and control box board 485 communication timeout", (i+1));
			sprintf(jap_content, "外部軸 %d 通信タイムアウト制御カードと制御ボックス基板485との通信タイムアウト", (i+1));
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", content);
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", en_content);
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", jap_content);
			}
			if (pre_state->exaxis_status[i].exAxisOFLIN != 1) {
				my_syslog("错误", content, cur_account.username);
				my_en_syslog("error", en_content, cur_account.username);
				my_jap_syslog("さくご", jap_content, cur_account.username);
				pre_state->exaxis_status[i].exAxisOFLIN = 1;
			}
		} else {
			pre_state->exaxis_status[i].exAxisOFLIN = 0;
		}
	}
	if (state->alarm_check_emerg_stop_btn == 1) {
		if (language == 0) {
			cJSON_AddStringToObject(error_json, "key", "通信异常,检查急停按钮是否松开");
		}
		if (language == 1) {
			cJSON_AddStringToObject(error_json, "key", "Warning: abnormal communication, check whether the emergency stop button is loosened");
		}
		if (language == 2) {
			cJSON_AddStringToObject(error_json, "key", "通信異常、急停止ボタンが外れているかチェック");
		}
		if (pre_state->alarm_check_emerg_stop_btn != 1) {
			my_syslog("错误", "通信异常,检查急停按钮是否松开", cur_account.username);
			my_en_syslog("error", "Warning: abnormal communication, check whether the emergency stop button is released", cur_account.username);
			my_jap_syslog("さくご", "通信異常、急停止ボタンが外れているかチェック", cur_account.username);
			pre_state->alarm_check_emerg_stop_btn = 1;
		}
	} else {
		pre_state->alarm_check_emerg_stop_btn = 0;
	}
	if (state->alarm_reboot_robot == 1) {
		if (language == 0) {
			cJSON_AddStringToObject(error_json, "key", "断电重启机器人");
		}
		if (language == 1) {
			cJSON_AddStringToObject(error_json, "key", "Power off and restart the robot");
		}
		if (language == 2) {
			cJSON_AddStringToObject(error_json, "key", "電源を切ってロボットを再起動する");
		}
		if (pre_state->alarm_reboot_robot != 1) {
			my_syslog("错误", "断电重启机器人", cur_account.username);
			my_en_syslog("error", "Power off and restart the robot", cur_account.username);
			my_jap_syslog("さくご", "電源を切ってロボットを再起動す", cur_account.username);
			pre_state->alarm_reboot_robot = 1;
		}
	} else {
		pre_state->alarm_reboot_robot = 0;
	}
	if (state->ts_web_state_com_error == 1) {
		if (language == 0) {
			cJSON_AddStringToObject(error_json, "key", "扭矩：WEB-TM 状态反馈，通信失败");
		}
		if (language == 1) {
			cJSON_AddStringToObject(error_json, "key", "Torque: WEB-TM state feedback, communication failure");
		}
		if (language == 2) {
			cJSON_AddStringToObject(error_json, "key", "トルク:web-tm状態フィードバック、通信失敗");
		}
		if (pre_state->ts_web_state_com_error != 1) {
			my_syslog("错误", "扭矩：WEB-TM 状态反馈，通信失败", cur_account.username);
			my_en_syslog("error", "Torque: WEB-TM state feedback, communication failure", cur_account.username);
			my_jap_syslog("さくご", "トルク:web-tm状態フィードバック、通信失敗", cur_account.username);
			pre_state->ts_web_state_com_error = 1;
		}
	} else {
		pre_state->ts_web_state_com_error = 0;
	}
	if (state->ts_tm_cmd_com_error == 1) {
		if (language == 0) {
			cJSON_AddStringToObject(error_json, "key", "扭矩：TM-扭矩 指令下发，通信失败");
		}
		if (language == 1) {
			cJSON_AddStringToObject(error_json, "key", "Torque: TM- Torque command issued, communication failed");
		}
		if (language == 2) {
			cJSON_AddStringToObject(error_json, "key", "トルク:tm -トルク指令が出され、通信が失敗する");
		}
		if (pre_state->ts_tm_cmd_com_error != 1) {
			my_syslog("错误", "扭矩：TM-扭矩 指令下发，通信失败", cur_account.username);
			my_en_syslog("error", "Torque: TM- Torque command issued, communication failed", cur_account.username);
			my_jap_syslog("さくご", "トルク:tm -トルク指令が出され、通信が失敗する", cur_account.username);
			pre_state->ts_tm_cmd_com_error = 1;
		}
	} else {
		pre_state->ts_tm_cmd_com_error = 0;
	}
	if (state->ts_tm_state_com_error == 1) {
		if (language == 0) {
			cJSON_AddStringToObject(error_json, "key", "扭矩：TM-扭矩 状态反馈，通信失败");
		}
		if (language == 1) {
			cJSON_AddStringToObject(error_json, "key", "Torque: TM- Torque status feedback, communication failure");
		}
		if (language == 2) {
			cJSON_AddStringToObject(error_json, "key", "トルク:tm -トルク状態フィードバック、通信失敗");
		}
		if (pre_state->ts_tm_state_com_error != 1) {
			my_syslog("错误", "扭矩：TM-扭矩 状态反馈，通信失败", cur_account.username);
			my_en_syslog("error", "Torque: TM- Torque status feedback, communication failure", cur_account.username);
			my_jap_syslog("さくご", "トルク:tm -トルク状態フィードバック、通信失敗", cur_account.username);
			pre_state->ts_tm_state_com_error = 1;
		}
	} else {
		pre_state->ts_tm_state_com_error = 0;
	}
	switch(state->ctrlBoxError)
	{
		case 1:
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", "超时");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "timeout");
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", "ターンオーバー");
			}
			if (pre_state->ctrlBoxError != 1) {
				my_syslog("错误", "超时", cur_account.username);
				my_en_syslog("error", "timeout", cur_account.username);
				my_jap_syslog("さくご", "ターンオーバー", cur_account.username);
				pre_state->ctrlBoxError = 1;
			}
			break;
		case 2:
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", "从机错误");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "slave machine error");
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", "スレーブエラー");
			}
			if (pre_state->ctrlBoxError != 1) {
				my_syslog("错误", "从机错误", cur_account.username);
				my_en_syslog("error", "slave machine error", cur_account.username);
				my_jap_syslog("さくご", "スレーブエラー", cur_account.username);
				pre_state->ctrlBoxError = 1;
			}
			break;
		case 3:
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", "主机错误");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "master machine error");
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", "ホストエラー");
			}
			if (pre_state->ctrlBoxError != 1) {
				my_syslog("错误", "主机错误", cur_account.username);
				my_en_syslog("error", "master machine error", cur_account.username);
				my_jap_syslog("さくご", "ホストエラー", cur_account.username);
				pre_state->ctrlBoxError = 1;
			}
			break;
		case 4:
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", "STO从机错误");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "STO slave machine error");
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", "STOスレーブエラー");
			}
			if (pre_state->ctrlBoxError != 1) {
				my_syslog("错误", "STO从机错误", cur_account.username);
				my_en_syslog("error", "STO slave machine error", cur_account.username);
				my_jap_syslog("さくご", "STOスレーブエラー", cur_account.username);
				pre_state->ctrlBoxError = 1;
			}
			break;
		case 5:
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", "STO主机错误");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "STO master machine error");
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", "STOホストエラー");
			}
			if (pre_state->ctrlBoxError != 1) {
				my_syslog("错误", "STO主机错误", cur_account.username);
				my_en_syslog("error", "STO master machine error", cur_account.username);
				my_jap_syslog("さくご", "STOホストエラー", cur_account.username);
				pre_state->ctrlBoxError = 1;
			}
			break;
		case 6:
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", "急停模式下主从机错误");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "The master/slave machine fails in emergency stop mode");
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", "急停止モードで主従機エラー");
			}
			if (pre_state->ctrlBoxError != 1) {
				my_syslog("错误", "急停模式下主从机错误", cur_account.username);
				my_en_syslog("error", "The master/slave machine fails in emergency stop mode", cur_account.username);
				my_jap_syslog("さくご", "急停止モードで主従機エラー", cur_account.username);
				pre_state->ctrlBoxError = 1;
			}
			break;
		case 7:
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", "STO模式下主从机错误");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "The master/slave server fails in STO mode");
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", "STOモードでのマスタースレーブエラー");
			}
			if (pre_state->ctrlBoxError != 1) {
				my_syslog("错误", "STO模式下主从机错误", cur_account.username);
				my_en_syslog("error", "The master/slave server fails in STO mode", cur_account.username);
				my_jap_syslog("さくご", "STOモードでのマスタースレーブエラー", cur_account.username);
				pre_state->ctrlBoxError = 1;
			}
			break;
		case 8:
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", "三位开关使能下STO从机错误");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "STO slave enabled error");
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", "3位スイッチが作動しSTO誤作動");
			}
			if (pre_state->ctrlBoxError != 1) {
				my_syslog("错误", "三位开关使能下STO从机错误", cur_account.username);
				my_en_syslog("error", "STO slave enabled error", cur_account.username);
				my_jap_syslog("さくご", "3位スイッチが作動しSTO誤作動", cur_account.username);
				pre_state->ctrlBoxError = 1;
			}
			break;
		case 9:
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", "三位开关使能下STO主机错误");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "STO host enabled by the three-digit switch fails");
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", "3桁スイッチでSTOホストエラーが発生");
			}
			if (pre_state->ctrlBoxError != 1) {
				my_syslog("错误", "三位开关使能下STO主机错误", cur_account.username);
				my_en_syslog("error", "STO host enabled by the three-digit switch fails", cur_account.username);
				my_jap_syslog("さくご", "3桁スイッチでSTOホストエラーが発生", cur_account.username);
				pre_state->ctrlBoxError = 1;
			}
			break;
		case 10:
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", "三位开关使能下STO主从机错误");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "STO master/slave enabled error");
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", "3位スイッチで作動STO主従機エラー");
			}
			if (pre_state->ctrlBoxError != 1) {
				my_syslog("错误", "三位开关使能下STO主从机错误", cur_account.username);
				my_en_syslog("error", "STO master/slave enabled error", cur_account.username);
				my_jap_syslog("さくご", "3位スイッチで作動STO主従機エラー", cur_account.username);
				pre_state->ctrlBoxError = 1;
			}
			break;
		case 11:
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", "STO输入错误");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "STO input error");
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", "STO入力エラー");
			}
			if (pre_state->ctrlBoxError != 1) {
				my_syslog("错误", "STO输入错误", cur_account.username);
				my_en_syslog("error", "STO input error", cur_account.username);
				my_jap_syslog("さくご", "STO入力エラー", cur_account.username);
				pre_state->ctrlBoxError = 1;
			}
			break;
		default:
			pre_state->exaxis_out_slimit_error = 0;
			break;
	}
}
