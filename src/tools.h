#ifndef tools_h
#define tools_h

#define local 1
#if local
	//#define SERVER_IP "127.0.0.1"
	#define SERVER_IP "192.168.152.129"
	//#define SERVER_IP "192.168.172.128"
#else
	#define SERVER_IP "192.168.58.2"
#endif

#define SOCK_TIMEOUT 1
#define SOCK_SEND_TIMEOUT 10
#define DELAY_MS_TIMEOUT 10000
#define CM_PORT 8080
#define STATUS_PORT 8081
#define FILE_PORT 8082
#define MAX_BUF 1024
#define SUCCESS 1
#define FAIL 0
#define FILE_POINTS "/tmp/points/points.json"
#define DIR_LUA "/tmp/lua"

int socket_cmd;
int socket_file;
pthread_mutex_t mute_cmd;
pthread_mutex_t mute_file;
pthread_t t_socket_cmd;
pthread_t t_socket_file;

char *get_file_content(const char *file_path);
char *get_dir_content(const char *dir_path);
char *strrpc(char *str, const char *oldstr, const char *newstr);
void delay_ms(const int timeout);
int create_connect(const char *server_ip, int server_port, const int s);
int socket_create();
int socket_timeout(int sockfd, const int s);
int socket_send(int clientSocket, const int no, const char *content, char *recvbuf);
void *socket_cmd_thread(void * arg);
void *socket_file_thread(void * arg);

#endif
