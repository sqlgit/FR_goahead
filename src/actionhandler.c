#include    "goahead.h"
#include	"js.h"
#include	"cJSON.h"
#include	"tools.h"
#include	"actionhandler.h"

//static int no;
static char content[MAX_BUF]={0};

/*static int joints(cJSON *data, char *recvbuf)
{
	int ret;

	cJSON *shoulder_pan_joint = cJSON_GetObjectItem(data, "shoulder_pan_joint");
	printf("shoulder_pan_joint:%s\n", shoulder_pan_joint->valuestring);

	no = 303;
	sprintf(content, "Mode(1)");
	ret = socket_client(no, content, recvbuf, CM_PORT);

	int i;
	for(i = 1; i <= 10; i++){
		no = 208;
		sprintf(content, "MJOINT(1,1,%d,100,100)", i);
		ret = socket_client(no, content, recvbuf, CM_PORT);
	}

	return ret;
}

static int tcp(cJSON *data, char *recvbuf)
{
	strcpy(recvbuf, "success");
	cJSON *x = cJSON_GetObjectItem(data, "x");
	printf("x:%s\n", x->valuestring);

	return SUCCESS;
}*/

/* 101 START */
static void start(cJSON *data)
{
	sprintf(content, "START");
}

/* 102 STOP */
static void stop(cJSON *data)
{
	sprintf(content, "STOP");
}

/* 105 sendFileName */
static void sendfilename(cJSON *data)
{
	cJSON *filename = cJSON_GetObjectItem(data, "data");
	printf("filename:%s\n", filename->valuestring);

	sprintf(content, "/fruser/%s", filename->valuestring);
}

/* 106 sendFile */
static void sendfile(cJSON *data)
{
	cJSON *filecontent = cJSON_GetObjectItem(data, "data");
	printf("filecontent:%s\n", filecontent->valuestring);

	const char s[2] = "\n";
	char *token;
	/* 获取第一个子字符串 */
	token = strtok(filecontent->valuestring, s);
	/* 继续获取其他的子字符串 */
	while( token != NULL ) {
		printf( "token = %s\n", token);
		if(!strncmp(token, "PTP:", 4)){
			strrpc(token,"PTP:", "");
			printf("after strncmp token = %s\n", token);
			char f_content[1024] = {0};
			openfile(token, f_content);
			printf("f_content = %s\n", f_content);
			printf("sizeof(f_content) = %d\n", sizeof(f_content));
			printf("strlen(f_content) = %d\n", strlen(f_content));

		}



		token = strtok(NULL, s);
	}



	sprintf(content, "MoveJ(19.708,-76.173,61.58,1.915,18.544,6.085,100,100)");
}

/* 201 MoveJ */
static void movej(cJSON *data)
{
	cJSON *joints = cJSON_GetObjectItem(data, "data");
	cJSON *joint = NULL;
	printf("joints->type = %d\n", joints->type);
	printf("cJSON_Array = %d\n", cJSON_Array);
	printf("cJSON_Object = %d\n", cJSON_Object);
	//if(joints->type == cJSON_Array) {
	//	cJSON_ArrayForEach(joint, joints) {
			cJSON *j1 = cJSON_GetObjectItem(joints, "J1");
			cJSON *j2 = cJSON_GetObjectItem(joints, "J2");
			cJSON *j3 = cJSON_GetObjectItem(joints, "J3");
			cJSON *j4 = cJSON_GetObjectItem(joints, "J4");
			cJSON *j5 = cJSON_GetObjectItem(joints, "J5");
			cJSON *j6 = cJSON_GetObjectItem(joints, "J6");
			sprintf(content, "MoveJ(%s,%s,%s,%s,%s,%s,100,100)", j1->valuestring, j2->valuestring, j3->valuestring, j4->valuestring, j5->valuestring, j6->valuestring);
	//	}
	//}
	printf("content = %s\n", content);
}

/* 303 Mode */
static void mode(cJSON *data)
{
	cJSON *type = cJSON_GetObjectItem(data, "data");
	printf("type:%d\n", type->valueint);

	sprintf(content, "Mode(%d)", type->valueint);
}

void set(Webs *wp)
{
	int ret = FAIL;
	int cmd = 0;
	char recvbuf[MAX_BUF];
	/*clear content*/
	memset(content, 0, sizeof(content));

	char *jsonString= wp->input.servp;
	cJSON *data = cJSON_Parse(jsonString);
	if (data == NULL)
	{
		const char *error_ptr = cJSON_GetErrorPtr();
		if (error_ptr != NULL)
		{
			fprintf(stderr, "Error before: %s\n", error_ptr);
		}
		goto end;
	}

	char *buf = NULL;
	printf("data:%s\n", buf = cJSON_Print(data));
	free(buf);

	cJSON *command = cJSON_GetObjectItem(data, "cmd");
	if(command != NULL) {
		cmd = command->valueint;
		switch(cmd) {
			case 101:
				start(data);
				break;
			case 105:
				sendfilename(data);
				break;
			case 106:
				sendfile(data);
				break;
			case 102:
				stop(data);
				break;
			case 201:
				movej(data);
				break;
			case 303:
				mode(data);
				break;
			default:
				goto end;
		}
	}

	cJSON *port_n = cJSON_GetObjectItem(data, "port");
	if(port_n != NULL) {
		int port = port_n->valueint;
		printf("port = %d\n", port);
		switch(port) {
			case CM_PORT:
				/*send cmd to 8080 server*/
				printf("content = %s\n", content);
				ret = socket_client(cmd, content, recvbuf, CM_PORT);
				break;
			case STATUS_PORT:
				/*send cmd to 8081 server*/
				ret = socket_client(cmd, content, recvbuf, STATUS_PORT);
				break;
			case FILE_PORT:
				/*send cmd to 8082 server*/
				ret = socket_client(cmd, content, recvbuf, FILE_PORT);
				break;
			default:
				goto end;
		}
	}

	/*char *act = action->valuestring;
	if(!strcmp(act, "tcp"))
	{
		ret = tcp(data, recvbuf);
	}
	else if(!strcmp(act, "joints"))
	{
		ret = joints(data, recvbuf);
	}
	else
		goto end;*/

	if(ret == FAIL){
		goto end;
	}

	printf("recvbuf:%s\n", recvbuf);

	cJSON_Delete(data);
	websSetStatus(wp, 200);
	websWriteHeaders(wp, -1, 0);
	websWriteEndHeaders(wp);
	websWrite(wp, recvbuf);
	websDone(wp);

	return;
	
end:
	cJSON_Delete(data);
	websSetStatus(wp, 403);
	websWriteHeaders(wp, -1, 0);
	websWriteEndHeaders(wp);
	websWrite(wp, recvbuf);
	websDone(wp);

	return;
}

void get(Webs *wp)
{
	//return;
}
