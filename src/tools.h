#if 0 
	#define SERVER_IP "192.168.58.2"
#else
	#define SERVER_IP "127.0.0.1"
#endif

#define CM_PORT 8080
#define STATUS_PORT 8081
#define FILE_PORT 8082
#define MAX_BUF 512
#define SUCCESS 1
#define FAIL 0

#define POINTS_PATH "/tmp/points/"
#define FILE_JSON ".json"

char *openfile(char *filename, char *content);
char *strrpc(char *str, char *oldstr, char *newstr);
int socket_client(int no, char *content, char *recvbuf, int server_port);
