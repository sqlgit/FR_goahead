#define local 0
#if local
	//#define SERVER_IP "127.0.0.1"
	#define SERVER_IP "192.168.152.129"
#else
	#define SERVER_IP "192.168.58.2"
#endif

#define SOCK_TIMEOUT 1
#define SOCK_SEND_TIMEOUT 3
#define DELAY_MS_TIMEOUT 10000
#define CM_PORT 8080
#define STATUS_PORT 8081
#define FILE_PORT 8082
#define MAX_BUF 1024
#define SUCCESS 1
#define FAIL 0
#define FILE_POINTS "/tmp/points/points.json"

int socket_cmd;
int socket_file;
pthread_mutex_t mute_cmd;
pthread_mutex_t mute_file;
pthread_t t_socket_cmd;
pthread_t t_socket_file;

char *openfile(char *file_path);
char *strrpc(char *str, char *oldstr, char *newstr);
void delay_ms(int timeout);
int create_connect(const char *server_ip, int server_port, int s);
int socket_create();
int socket_timeout(int sockfd, int s);
int socket_send(int clientSocket, int no, char *content, char *recvbuf);
void *socket_cmd_thread(void * arg);
void *socket_file_thread(void * arg);
