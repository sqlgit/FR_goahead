#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "tools.h"
#include "cJSON.h"

/* file open */
char *openfile(char *filename, char *content)
{
	FILE *fp = NULL;
	char file_path[50] = {0};
	int file_size;
	int i = 0;
	char *tmp;
	cJSON *json;

	sprintf(file_path, "%s%s%s", POINTS_PATH, filename, FILE_JSON);
	printf("file_path = %s\n", file_path);
	if(NULL != (fp = fopen(file_path, "r"))) {
		fseek(fp , 0 , SEEK_END );
		file_size = ftell(fp);
		fseek(fp , 0 , SEEK_SET);

		char file_content[1024] = {0};
		memset(file_content, 0, sizeof(file_content));
		tmp = (char*)malloc(file_size*sizeof(char)+1);
		fread(tmp, sizeof(char), file_size, fp);
		while(*tmp != '\0') {
			if(*tmp != '\n' && *tmp != '\r' && *tmp !='\t'){
				file_content[i] = *tmp;
				i++;
			}
			tmp++;
		}
		file_content[i] = '\0';

		printf("file_content = %s\n", file_content);
		printf("sizeof(file_content) = %d\n", sizeof(file_content));
		printf("strlen(file_content) = %d\n", strlen(file_content));

		json = cJSON_Parse(file_content); //获取整个大的句柄
		//content = cJSON_Print(json);  //这个是可以输出的。为获取的整个json的值
		//printf("content = %s\n", content);
		//memcpy(content, file_content, 1024);
		strcpy(content, file_content);
	}
	fclose(fp);
}

/* 实现字符串中指定字符串替换 */
char *strrpc(char *str, char *oldstr, char *newstr)
{
	char bstr[strlen(str)];//转换缓冲区
	memset(bstr,0,sizeof(bstr));
	int i;

	for(i = 0;i < strlen(str);i++){
		if(!strncmp(str+i,oldstr,strlen(oldstr))){//查找目标字符串
			strcat(bstr,newstr);
			i += strlen(oldstr) - 1;
		}else{
			strncat(bstr,str + i,1);//保存一字节进缓冲区
		}
	}

	strcpy(str,bstr);

	return str;
}

/* socket client connect with server */
int socket_client(int no, char *content, char *recvbuf, int server_port)
{
#if 1
	server_port = 5555;
#endif
	int clientSocket;
	//描述服务器的socket
	struct sockaddr_in serverAddr;
	int iDataNum;
	if((clientSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("socket");
		return FAIL;
	}

	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(server_port);
	//指定服务器端的ip，本地测试：127.0.0.1
	//inet_addr()函数，将点分十进制IP转换成网络字节序IP
	serverAddr.sin_addr.s_addr = inet_addr(SERVER_IP);
	printf("server_ip = %s\n", SERVER_IP);
	printf("server_port = %d\n", server_port);
	if(connect(clientSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
	{
		perror("connect");
		return FAIL;
	}

	printf("connect with destination host...\n");
	
	char sendbuf[MAX_BUF];
	sprintf(sendbuf, "/f/bIII1III%dIII%dIII%sIII/b/f", no, strlen(content), content);
	//char sendbuf[200] = "/f/bIII1III208III22IIIMJOINT(1,1,1,100,100)III/b/f";
	send(clientSocket, sendbuf, strlen(sendbuf), 0);
	if(recv(clientSocket, recvbuf, MAX_BUF, 0) < 0)
	{
		perror("recv");
		return FAIL;
	}
	//iDataNum = recv(clientSocket, recvbuf, MAX_BUF, 0);
	//recvbuf[iDataNum] = '\0';
	printf("recv data of my world is: %s\n", recvbuf);
	close(clientSocket);

	return SUCCESS;
}
