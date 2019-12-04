#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/time.h>
#include <pthread.h>
#include "tools.h"
#include "cJSON.h"

/* file open and return file content */
char *openfile(const char *file_path)
{
	FILE *fp = NULL;
	int file_size = 0;
	int i = 0;
	char *content = NULL;

	//printf("file_path = %s\n", file_path);
	if(NULL != (fp = fopen(file_path, "r"))) {
		fseek(fp , 0 , SEEK_END);
		file_size = ftell(fp);
		//printf("file_size = %d\n", file_size);
		fseek(fp , 0 , SEEK_SET);

		char *tmp = (char *)calloc(1, file_size*sizeof(char)+1);
		if(tmp != NULL) {
			fread(tmp, sizeof(char), file_size, fp);
			/*
			printf("tmp = %s\n", tmp);
			printf("strlen tmp = %d\n", strlen(tmp));
			printf("sizeof tmp = %d\n", sizeof(tmp));
			*/
			char file_content[file_size+1];
			memset(file_content, 0, (file_size+1));
			char *tmp2 = tmp;
			while(*tmp2 != '\0') {
				if(*tmp2 != '\n' && *tmp2 != '\r' && *tmp2 !='\t'){
					file_content[i] = *tmp2;
					i++;
				}
				tmp2++;
			}
			free(tmp);
			file_content[i] = '\0';
			content = (char *)calloc(1, strlen(file_content)+1);
			if(content != NULL) {
				strcpy(content, file_content);
			}
		}
	}
	fclose(fp);

	return content;
}

/* read file list and save in array return */
char *readfilelist(const char *basePath)
{
	DIR *dir = NULL;
	struct dirent *ptr = NULL;
	char *content = NULL;
	char *buf = NULL;
	cJSON *file_list = NULL;

	if ((dir=opendir(basePath)) == NULL) {
		perror("Open dir error...");
	}
	file_list = cJSON_CreateArray();
	while ((ptr=readdir(dir)) != NULL) {
		if(strcmp(ptr->d_name, ".") == 0 || strcmp(ptr->d_name, "..") == 0)    ///current dir OR parrent dir
			continue;
		printf("d_name:%s/%s\n", basePath, ptr->d_name);
		cJSON_AddStringToObject(file_list, "key", ptr->d_name);
	}
	printf("buf = %s\n", buf = cJSON_Print(file_list));
	content = (char *)calloc(1, strlen(buf)+1);
	if(content != NULL) {
		strcpy(content, buf);
	}
	free(buf);
	buf = NULL;
	cJSON_Delete(file_list);
	file_list = NULL;
	closedir(dir);
	dir = NULL;

	return content;
}

/* 实现字符串中指定字符串替换 */
char *strrpc(char *str, char *oldstr, char *newstr)
{
	char bstr[strlen(str)];//转换缓冲区
	memset(bstr, 0, sizeof(bstr));
	int i;

	for(i = 0; i < strlen(str); i++){
		if(!strncmp(str+i, oldstr, strlen(oldstr))){//查找目标字符串
			strcat(bstr, newstr);
			i += strlen(oldstr) - 1;
		} else {
			strncat(bstr, str + i, 1);//保存一字节进缓冲区
		}
	}
	strcpy(str, bstr);

	return str;
}

/* 毫秒定时器 */
void delay_ms(int timeout)
{
	struct timeval timer;
	bzero(&timer, sizeof(struct timeval));
	timer.tv_sec        = 0;    // 0秒
	timer.tv_usec       = 1000*timeout; // 1000us = 1ms
	select(0, NULL, NULL, NULL, &timer);
}

/* create connect */
int create_connect(const char *server_ip, int server_port, int s)
{
	int ret = -1;
	int sockfd = -1;
	unsigned long ul;
	//描述服务器的socket
	struct sockaddr_in serverAddr;

	bzero(&serverAddr, sizeof(struct sockaddr_in));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(server_port);
	//inet_addr()函数，将点分十进制IP转换成网络字节序IP
	serverAddr.sin_addr.s_addr = inet_addr(server_ip);

	sockfd = socket_create();
	if(sockfd < 0){
		return -1;
	}

	// set no block
	//ul = 1;
	//ioctl(sockfd, FIONBIO, &ul);
	int fdopt = fcntl(sockfd, F_GETFL);
	fcntl(sockfd, F_SETFL, fdopt | O_NONBLOCK);

	// client connect
	ret = connect(sockfd, (struct sockaddr_in *)&serverAddr, sizeof(struct sockaddr_in));

	//unblock mode --> local connect, connect return immediately
	if (ret == 0)
	{
		printf("connect with server immediately\n");
		fcntl(sockfd, F_SETFL, fdopt);

		return sockfd;
	}
	else if (errno != EINPROGRESS)
	{
		printf("unblock connect failed!\n");
		close(sockfd);

		return -1;
	}
	// ret = -1 & errno = EINPROGRESS
	else if (errno == EINPROGRESS)
	{
		printf("unblock mode socket is connecting...\n");
	}

	// set socket connect timeout 
	ret = socket_timeout(sockfd, s);
	if(ret < 0){
		close(sockfd);

		return -1;
	}

	//connection successful!
	printf( "connection ready after select with the socket: %d \n", sockfd);
	// set block
	fcntl(sockfd, F_SETFL, fdopt);
	//ul = 0;
	//ioctl(sockfd, FIONBIO, &ul);

	return sockfd;
}

/* socket init */
int socket_create()
{
	int sockfd;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd < 0){
		perror("create socket error");

		return -1;
	}

	return sockfd;
}

/* socket timeout */
int socket_timeout(int sockfd, int s)
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
	if(getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &len) < 0)
	{
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
int socket_send(int clientSocket, int no, const char *content, char *recvbuf)
{
	int send_len = strlen(content)+100;
	printf("send_len = %d\n", send_len);
	char send_buf[send_len];
	memset(send_buf, 0, send_len);
	/* test package */
	if(no == 100) {
		sprintf(send_buf, "/f/bIII1III%dIII%dIII%s", no, strlen(content), content);
	} else {
		sprintf(send_buf, "/f/bIII1III%dIII%dIII%sIII/b/f", no, strlen(content), content);
	}

	/* send over 1024 bytes */
	while(send_len > 1024) {
		char buf[1024] = {0};
		strncpy(buf, send_buf, 1024);
		if(send(clientSocket, buf, 1024, 0) != 1024) {
			perror("send");

			return FAIL;
		}
		send_len = send_len - 1024;
		char tmp_buf[send_len+1];
		memset(tmp_buf, 0, (send_len+1));
		int i;
		for(i = 0; i < send_len; i++){
			tmp_buf[i] = send_buf[i+1024];
		}
		bzero(send_buf, sizeof(send_buf));
		strcpy(send_buf, tmp_buf);
	}
	if(no != 401) {
		printf("send data to socket server is: %s\n", send_buf);
	}
	/* send normal (low)1024 bytes */
	if(send(clientSocket, send_buf, strlen(send_buf), 0) != strlen(send_buf)) {
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
	if(no != 401) {
		/* recv data */
		if(recv(clientSocket, recvbuf, MAX_BUF, 0) <= 0) {
			perror("recv");

			return FAIL;
		}
		//int iDataNum;
		//iDataNum = recv(clientSocket, recvbuf, MAX_BUF, 0);
		//recvbuf[iDataNum] = '\0';
		printf("recv data of socket server is: %s\n", recvbuf);

		return SUCCESS;
	} else {
		/*If the packet is not what the heartbeat packet wants within 10 seconds, continue to wait for 10s to see if you can receive the corresponding content. If you still cannot receive the corresponding data, it will be judged as timeout*/
		do {
			/* recv data */
			if(recv(clientSocket, recvbuf, MAX_BUF, 0) <= 0) {
				perror("recv");

				return FAIL;
			}
			printf("recv data of socket server is: %s\n", recvbuf);
#if local
		} while (strcmp(recvbuf, "/f/bIII1III401III5IIIHEARTIII/b/f") != 0);
#else
		} while (strcmp(recvbuf, "/f/bIII1III401III1III1III/b/f") != 0);
#endif

		return SUCCESS;
	}
}

void *socket_cmd_thread(void *arg)
{
	socket_cmd = -1;
	while(1) {
		/* send Heartbeat package */
		if(socket_cmd > 0){
			char recvbuf[MAX_BUF] = {0};
			int ret = FAIL;
			//上锁
			pthread_mutex_lock(&mute_cmd);
			/* send heartbeat packet */
			ret = socket_send(socket_cmd, 401, "HEART", recvbuf);
			//解锁
			pthread_mutex_unlock(&mute_cmd);
			if(ret == SUCCESS) { 
				/* Heartbeat package every 10 seconds */
				delay_ms(DELAY_MS_TIMEOUT);
				continue;
			}
			/* close socket_cmd */
			close(socket_cmd);
			socket_cmd = -1;
		}
		/* socket reconnected */
		while(socket_cmd < 0) {
			socket_cmd = create_connect(SERVER_IP, CM_PORT, SOCK_TIMEOUT);
			if(socket_cmd < 0){
				printf("Connect failed! \n");
			} else {
				printf("Connect success, sockfd = %d\nserver_ip = %s, server_port = %d\n", socket_cmd, SERVER_IP, CM_PORT);
				break;
			}
			delay_ms(1000);
		}
	}
}

void *socket_file_thread(void *arg)
{
	socket_file = -1;
	while(1) {
		/* send Heartbeat package */
		if(socket_file > 0){
			char recvbuf[MAX_BUF] = {0};
			int ret = FAIL;
			//上锁
			pthread_mutex_lock(&mute_file);
			/* send heartbeat packet */
			ret = socket_send(socket_file, 401, "HEART", recvbuf);
			//解锁
			pthread_mutex_unlock(&mute_file);
			if(ret == SUCCESS) { 
				/* Heartbeat package every 5 seconds */
				delay_ms(DELAY_MS_TIMEOUT);
				continue;
			}
			/* close socket_cmd */
			close(socket_file);
			socket_file = -1;
		}
		/* socket reconnected */
		while(socket_file < 0) {
			socket_file = create_connect(SERVER_IP, FILE_PORT, SOCK_TIMEOUT);
			if(socket_file < 0){
				printf("Connect failed! \n");
			} else {
				printf("Connect success, sockfd = %d\nserver_ip = %s, server_port = %d\n", socket_file, SERVER_IP, FILE_PORT);
				break;
			}
			delay_ms(1000);
		}
	}
}
