#ifndef socket_quene_h
#define socket_quene_h

/********************************* Defines ************************************/

/* 
   front,next -----> 
   node1,next -----> 
   node2,next -----> 
   ... -----> 
   noden,next(rear,next) 
*/
typedef struct COMMAND_INTERACTION {
	//uint8_t msghead;                  	// 指令信息头，唯一标识,8位
	//uint8_t type;                       	// 指令标签,具体参照指令字典,8位
	int 	msghead;                  		// 指令信息头，唯一标识
	int		type;                       	// 指令标签,具体参照指令字典位
	char	*msgcontent;     				// 消息内容
	int		msglen;                     	// 消息内容长度
	int		state;                      	// 处理状态, 0:init, 1:send to server, 2:recv data 
} COMMAND_INTERACTION;

typedef COMMAND_INTERACTION QElemType;

typedef struct QNode {
	QElemType data;
	struct QNode *next;
} Qnode;

typedef struct LINKQuene {
	Qnode *front;
	Qnode *rear;
} LinkQuene;

/********************************* Function declaration ***********************/

void createnode(QElemType *pnode, int type, char *msgcontent, int state);
void initquene(LinkQuene *q);
void clearquene(LinkQuene *q);
void enquene(LinkQuene *q, QElemType e);
void dequene(LinkQuene *q, QElemType e);
void printquene(LinkQuene q);
int queneempty(LinkQuene *q);
int quene_recv_result(const QElemType node, const LinkQuene q);

#endif
