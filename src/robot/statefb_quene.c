
/********************************* Includes ***********************************/

#include 	"goahead.h"
#include 	"tools.h"
#include 	"statefb_quene.h"

/********************************* Defines ************************************/

/********************************* Function declaration ***********************/

/*********************************** Code *************************************/

void fb_createnode(STATE_FB *pnode)
{
	bzero(pnode, sizeof(STATE_FB));
}

/* 初始化带有头结点队列 */
void fb_initquene(FB_LinkQuene *q)
{
	/* front,next(rear,next) */
	q->front = (FB_Qnode *)malloc(sizeof(FB_Qnode));
	if (q->front == NULL) {
		printf("malloc failed\n");

		return;
	}
	q->rear = q->front;
	q->front->next = NULL;

	return;
}

/* 清除链队中除头结点外的所有元素 */
void fb_clearquene(FB_LinkQuene *q)
{
	FB_Qnode *p = q->front->next;        /* 队首指针赋给p */
	/* 依次删除队列中的每一个结点 */
	while (p != NULL) {
		q->front->next = p->next;
		free(p);
		p = q->front->next;
	}
	q->rear = q->front;        /* 置队头队尾指针重合 */

	return;
}

/* 检查链队是否为空，若为空则返回0, 否则返回1 */
int fb_queneempty(FB_LinkQuene *q)
{
	if (q->rear == q->front) {
	//	printf("Is NULL: Yes\n");

		return 0;
	} else {
	//	printf("Is NULL：No\n");

		return 1;
	}
}

/* 向链式队列队尾插入一个元素 e */
void fb_enquene(FB_LinkQuene *q, STATE_FB e)
{
	/* 得到一个由 node 指针所指向的新结点 */
	FB_Qnode *node = (FB_Qnode *)malloc(sizeof(FB_Qnode));
	if (node == NULL) {
		printf("malloc failed\n");

		return;
	}
	/* 把 e 的值赋给新结点的值域，把新结点的指针域置空 */
	node->data = e;
	node->next = NULL;
	q->rear->next = node; /* 最后一个节点的指针指向新建节点*/
	q->rear = node;  /* 队尾指针指向新建节点*/

	return;
}

/* 从链式队列中删除头元素 */
void fb_dequene(FB_LinkQuene *q)
{
	FB_Qnode *p = q->front->next;
	FB_Qnode *pre = q->front;

	/* front node is not found */
	if (p == NULL) {
		printf("error: not found quene front node\n");

		return;
	}
	/* front node is found */
	pre->next = p->next;
	/* front node is rear */
	if (p->next == NULL) {
		//printf("will delete rear node\n");
		q->rear = pre;
	}
	free(p);

	return;
}

/* print quene */
void fb_printquene(FB_LinkQuene q)
{
	FB_Qnode *p = q.front->next;

	int len = 1;
	int i = 0;
	int j = 0;
	printf("print quene:\n");
	while (p != NULL) {
		printf("node %d:", len);
		for(i = 0; i < 100; i++) {
			for(j = 0; i < 10; j++) {
				printf("%f ", p->data.fb[i][j]);
			}
		}
		p = p->next;
		printf("\n");
	}
	printf("end print\n");

	return;
}

/* print quene node number */
void fb_print_node_num(FB_LinkQuene q)
{
	FB_Qnode *p = q.front->next;

	int len = 0;
	printf("print quene node num:");
	while (p != NULL) {
		p = p->next;
		len++;
	}
	printf("%d\n", len);

	return;
}

/* get quene node number */
int fb_get_node_num(FB_LinkQuene q)
{
	FB_Qnode *p = q.front->next;

	int len = 0;
	while (p != NULL) {
		p = p->next;
		len++;
	}

	return len;
}
