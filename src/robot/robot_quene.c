
/********************************* Includes ***********************************/

#include 	"goahead.h"
#include 	"tools.h"
#include 	"robot_quene.h"

/********************************* Defines ************************************/

/********************************* Function declaration ***********************/

/*********************************** Code *************************************/

int createnode(QElemType *pnode, int type, char *msgcontent)
{
	bzero(pnode, sizeof(QElemType));

	pnode->msghead = 0;
	pnode->type = type;
	pnode->msgcontent = (char *)calloc(1, strlen(msgcontent)+1);
	if (pnode->msgcontent == NULL) {
		perror("calloc");

		return FAIL;
	}
	strcpy(pnode->msgcontent, msgcontent);
	pnode->msglen = strlen(pnode->msgcontent);

	return SUCCESS;
}

/* 初始化带有头结点队列 */
void initquene(LinkQuene *q)
{
	/* front,next(rear,next) */
	q->front = (Qnode *)malloc(sizeof(Qnode));
	if (q->front == NULL) {
		printf("malloc failed\n");

		return;
	}
	q->rear = q->front;
	q->front->next = NULL;

	return;
}

/* 清除链队中除头结点外的所有元素 */
void clearquene(LinkQuene *q)
{
	Qnode *p = q->front->next;        /* 队首指针赋给p */
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
int queneempty(LinkQuene *q)
{
	if (q->rear == q->front) {
		printf("判空：是\n");

		return 0;
	} else {
		printf("判空：否\n");

		return 1;
	}
}

/* 向链式队列队尾插入一个元素 e */
void enquene(LinkQuene *q, QElemType e)
{
	/* 得到一个由 node 指针所指向的新结点 */
	Qnode *node = (Qnode *)malloc(sizeof(Qnode));
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

/* 从链式队列中删除一个元素 e，e 可以是任意一个元素 */
void dequene(LinkQuene *q, QElemType e)
{
	Qnode *p = q->front->next;
	Qnode *pre = q->front;

	while (p != NULL) {
		if (p->data.msghead == e.msghead)
			break;
		pre = pre->next;
		p = p->next;
	}
	/* e is not found */
	if (p == NULL) {
		printf("error: not found QElemType e\n");

		return;
	}
	/* e is found */
	//printf("will delete node: e\n");
	pre->next = p->next;
	/* e is rear */
	if (p->next == NULL) {
		//printf("will delete rear node: e\n");
		q->rear = pre;
	}
	if (p->data.msgcontent != NULL) {
		free(p->data.msgcontent);
		p->data.msgcontent = NULL;
	}
	free(p);

	return;
}

/* print quene */
void printquene(LinkQuene q)
{
	Qnode *p = q.front->next;

	printf("print:\n");
	while (p != NULL) {
		printf("p->data.type = %d\t", p->data.type);
		printf("p->data.msghead = %d\t", p->data.msghead);
		printf("p->data.msgcontent = %s\t", p->data.msgcontent);
		printf("p->data.msglen = %d\t", p->data.msglen);
		p = p->next;
		printf("\n");
	}
	printf("end\n");

	return;
}

/* 判断队列中的某个结点的内容，状态是否已经改变 */
int quene_recv_result(const QElemType node, const LinkQuene q, char *recv_content)
{
	// TODO: 优化,不使用轮询的方式
	/* 遍历整个队列, 更改相关结点信息 */
	Qnode *p = q.front->next;
	while (p != NULL) {
		if (p->data.msghead == node.msghead) {
			int i;
			/* 最多等待 10 s */
			for (i = 0; i < 1000; i++) {
				/* 标志已经拿到服务器端的返回值 */
			//	if (p->data.state == 2) {
#if local
					return SUCCESS;
#else
					if (!strcmp(p->data.msgcontent, "1")) {

						return SUCCESS;
					} else if (!strcmp(p->data.msgcontent, "0")) {

						return FAIL;
					} else {
						/* 获取到了除了“0”和“1”的其他返回值, 需要进一步处理 */
						printf("recv server return data: %s\n", p->data.msgcontent);
						strcpy(recv_content, p->data.msgcontent);

						return SUCCESS;
					}
#endif
			//	}
				delay(10);
			}
		}
		p = p->next;
	}

	return FAIL;
}

