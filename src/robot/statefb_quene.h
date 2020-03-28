#ifndef statefb_quene_h
#define statefb_quene_h

/********************************* Defines ************************************/

/* 
   front,next -----> 
   node1,next -----> 
   node2,next -----> 
   ... -----> 
   noden,next(rear,next) 
*/
#pragma pack(push, 1)
/** 状态反馈结构体 from socket server */
typedef struct _STATE_FB {
	float fb[100][10]
} STATE_FB;
#pragma pack(pop)

typedef struct FB_QNode {
	STATE_FB data;
	struct FB_QNode *next;
} FB_Qnode;

typedef struct FB_LINKQuene {
	FB_Qnode *front;
	FB_Qnode *rear;
} FB_LinkQuene;

/********************************* Function declaration ***********************/

void fb_createnode(STATE_FB *pnode);
void fb_initquene(FB_LinkQuene *q);
void fb_clearquene(FB_LinkQuene *q);
int fb_queneempty(FB_LinkQuene *q);
void fb_enquene(FB_LinkQuene *q, STATE_FB e);
void fb_dequene(FB_LinkQuene *q);
void fb_printquene(FB_LinkQuene q);

#endif
