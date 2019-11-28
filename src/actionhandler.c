#include    "goahead.h"
#include	"js.h"
#include	"cJSON.h"
#include	"tools.h"
#include	"actionhandler.h"

//static int no;
static int program_start(cJSON *data);
static int program_stop(cJSON *data);
static int program_pause(cJSON *data);
static int program_resume(cJSON *data);
static int sendfilename(cJSON *data);
static int sendfile(cJSON *data);
static int movej(cJSON *data);
static int mode(cJSON *data);

static char *content = NULL;
static char *ret_f_content = NULL;

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
static int program_start(cJSON *data)
{
	sprintf(content, "START");

	return SUCCESS;
}

/* 102 STOP */
static int program_stop(cJSON *data)
{
	sprintf(content, "STOP");

	return SUCCESS;
}

/* 103 PAUSE */
static int program_pause(cJSON *data)
{
	sprintf(content, "PAUSE");

	return SUCCESS;
}

/* 104 RESUME */
static int program_resume(cJSON *data)
{
	sprintf(content, "RESUME");

	return SUCCESS;
}

/* 105 sendFileName */
static int sendfilename(cJSON *data)
{
	cJSON *filename = cJSON_GetObjectItem(data, "data");
	if(filename->valuestring == NULL) {
		fprintf(stderr, "Parse json file Error!\n");

		return FAIL;
	}
	printf("filename:%s\n", filename->valuestring);
	sprintf(content, "/fruser/%s", filename->valuestring);

	return SUCCESS;
}

/* 106 sendFile */
static int sendfile(cJSON *data)
{
	const char s[2] = "\n";
	char *token = NULL;
	char *f_content = NULL;
	cJSON *f_json = NULL;

	cJSON *filecontent = cJSON_GetObjectItem(data, "data");
	if(filecontent->valuestring == NULL) {
		goto end;
	}
	printf("upload file content:\n%s\n", filecontent->valuestring);
	/* get first line */
	token = strtok(filecontent->valuestring, s);

	while( token != NULL ) {
		printf( "line = %s\n", token);
		cJSON *j1 = NULL;
		cJSON *j2 = NULL;
		cJSON *j3 = NULL;
		cJSON *j4 = NULL;
		cJSON *j5 = NULL;
		cJSON *j6 = NULL;
		cJSON *x = NULL;
		cJSON *y = NULL;
		cJSON *z = NULL;
		cJSON *rx = NULL;
		cJSON *ry = NULL;
		cJSON *rz = NULL;
		cJSON *speed = NULL;
		cJSON *acc = NULL;
		
		if(!strncmp(token, "PTP:", 4)) {
			/* PTP */
			/* open points file */
			f_content = openfile(FILE_POINTS);
			/* file is NULL */
			if (f_content == NULL) {
				fprintf(stderr, "open file error!\n");

				return FAIL;
			}
			f_json = cJSON_Parse(f_content);
			if (f_json == NULL) {
				goto end;
			}
			strrpc(token, "PTP:", "");
			cJSON *ptp = cJSON_GetObjectItem(f_json, token);
			if (ptp == NULL || ptp->type != cJSON_Object) {
				goto end;
			}
			cJSON *joints = cJSON_GetObjectItem(ptp, "joints");
			if (joints == NULL || joints->type != cJSON_Object) {
				goto end;
			}
			j1 = cJSON_GetObjectItem(joints, "j1");
			j2 = cJSON_GetObjectItem(joints, "j2");
			j3 = cJSON_GetObjectItem(joints, "j3");
			j4 = cJSON_GetObjectItem(joints, "j4");
			j5 = cJSON_GetObjectItem(joints, "j5");
			j6 = cJSON_GetObjectItem(joints, "j6");
			speed = cJSON_GetObjectItem(ptp, "speed");
			acc = cJSON_GetObjectItem(ptp, "acc");
			if(j1->valuestring == NULL || j2->valuestring == NULL || j3->valuestring == NULL || j4->valuestring == NULL || j5->valuestring == NULL || j6->valuestring == NULL || speed->valuestring == NULL || acc->valuestring == NULL) { 
				goto end;
			}
			sprintf(content, "%sMoveJ(%s,%s,%s,%s,%s,%s,%s,%s)\n", content, j1->valuestring, j2->valuestring, j3->valuestring, j4->valuestring, j5->valuestring, j6->valuestring, speed->valuestring, acc->valuestring);
			if (f_json != NULL) {
				cJSON_Delete(f_json);
			}
			if (f_content != NULL) {
				printf("free f_content\n");
				free(f_content);
				f_content = NULL;
			}
		} else if (!strncmp(token, "Lin:", 4)) {
			/* Lin */
			/* open points file */
			f_content = openfile(FILE_POINTS);
			/* file is NULL */
			if (f_content == NULL) {
				fprintf(stderr, "open file error!\n");

				return FAIL;
			}
			f_json = cJSON_Parse(f_content);
			if (f_json == NULL) {
				goto end;
			}
			strrpc(token, "Lin:", "");
			cJSON *lin = cJSON_GetObjectItem(f_json, token);
			if (lin == NULL || lin->type != cJSON_Object) {
				goto end;
			}
			cJSON *joints = cJSON_GetObjectItem(lin, "joints");
			if (joints == NULL || joints->type != cJSON_Object) {
				goto end;
			}
			j1 = cJSON_GetObjectItem(joints, "j1");
			j2 = cJSON_GetObjectItem(joints, "j2");
			j3 = cJSON_GetObjectItem(joints, "j3");
			j4 = cJSON_GetObjectItem(joints, "j4");
			j5 = cJSON_GetObjectItem(joints, "j5");
			j6 = cJSON_GetObjectItem(joints, "j6");
			cJSON *tcp = cJSON_GetObjectItem(lin, "tcp");
			if (tcp == NULL || tcp->type != cJSON_Object) {
				goto end;
			}
			x = cJSON_GetObjectItem(tcp, "x");
			y = cJSON_GetObjectItem(tcp, "y");
			z = cJSON_GetObjectItem(tcp, "z");
			rx = cJSON_GetObjectItem(tcp, "rx");
			ry = cJSON_GetObjectItem(tcp, "ry");
			rz = cJSON_GetObjectItem(tcp, "rz");
			speed = cJSON_GetObjectItem(lin, "speed");
			acc = cJSON_GetObjectItem(lin, "acc");
			if(j1->valuestring == NULL || j2->valuestring == NULL || j3->valuestring == NULL || j4->valuestring == NULL || j5->valuestring == NULL || j6->valuestring == NULL || x->valuestring == NULL || y->valuestring == NULL || z->valuestring == NULL || rx->valuestring == NULL || ry->valuestring == NULL || rz->valuestring == NULL || speed->valuestring == NULL || acc->valuestring == NULL) { 
				goto end;
			}
			sprintf(content, "%sMoveL(%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s)\n", content, j1->valuestring, j2->valuestring, j3->valuestring, j4->valuestring, j5->valuestring, j6->valuestring, x->valuestring, y->valuestring, z->valuestring, rx->valuestring, ry->valuestring, rz->valuestring, speed->valuestring, acc->valuestring);
			if (f_json != NULL) {
				cJSON_Delete(f_json);
			}
			if (f_content != NULL) {
				printf("free f_content\n");
				free(f_content);
				f_content = NULL;
			}
		} else if (!strncmp(token, "WaitTime:", 9)) {
			/* wait time*/
			strrpc(token, "WaitTime:", "");
			sprintf(content, "%sWaitMs(%s)\n", content, token);
		} else {
			/* other code send without processing */
			sprintf(content, "%s%s\n", content, token);
		}
		/* get other line */
		token = strtok(NULL, s);
	}

	return SUCCESS;

end:
	fprintf(stderr, "Parse json file Error!\n");
	if (f_json != NULL) {
		cJSON_Delete(f_json);
	}
	if (f_content != NULL) {
		printf("free f_content\n");
		free(f_content);
		f_content = NULL;
	}
	return FAIL;
}

/* 201 MoveJ */
static int movej(cJSON *data)
{
	cJSON *joints = cJSON_GetObjectItem(data, "data");
	if (joints == NULL || joints->type != cJSON_Object) {
		fprintf(stderr, "Parse json file Error!\n");

		return FAIL;
	}
	cJSON *j1 = cJSON_GetObjectItem(joints, "j1");
	cJSON *j2 = cJSON_GetObjectItem(joints, "j2");
	cJSON *j3 = cJSON_GetObjectItem(joints, "j3");
	cJSON *j4 = cJSON_GetObjectItem(joints, "j4");
	cJSON *j5 = cJSON_GetObjectItem(joints, "j5");
	cJSON *j6 = cJSON_GetObjectItem(joints, "j6");
	if(j1->valuestring == NULL || j2->valuestring == NULL || j3->valuestring == NULL || j4->valuestring == NULL || j5->valuestring == NULL || j6->valuestring == NULL) {
		fprintf(stderr, "Parse json file Error!\n");

		return FAIL;
	}
	sprintf(content, "MoveJ(%s,%s,%s,%s,%s,%s,30,100)", j1->valuestring, j2->valuestring, j3->valuestring, j4->valuestring, j5->valuestring, j6->valuestring);

	return SUCCESS;
}

/* 303 Mode */
static int mode(cJSON *data)
{
	cJSON *type = cJSON_GetObjectItem(data, "data");
	/*if(type->valueint == NULL) {
		fprintf(stderr, "Parse json file Error!\n");

		return FAIL;
	}*/
	sprintf(content, "Mode(%d)", type->valueint);

	return SUCCESS;
}

void set(Webs *wp)
{
	int ret = FAIL;
	int cmd = 0;
	char recvbuf[MAX_BUF] = {0};

	char *jsonString= wp->input.servp;
	cJSON *data = cJSON_Parse(jsonString);
	if (data == NULL)
	{
		fprintf(stderr, "Parse json file Error!\n");
		goto end;
	}

	char *buf = NULL;
	printf("data:%s\n", buf = cJSON_Print(data));
	free(buf);

	/*calloc content*/
	content = (char *)calloc(1, sizeof(char)*1024);
	if(content == NULL){
		goto end;
	}

	cJSON *command = cJSON_GetObjectItem(data, "cmd");
	if(command != NULL) {
		cmd = command->valueint;
		switch(cmd) {
			case 101:
				ret = program_start(data);
				break;
			case 102:
				ret = program_stop(data);
				break;
			case 103:
				ret = program_pause(data);
				break;
			case 104:
				ret = program_resume(data);
				break;
			case 105:
				ret = sendfilename(data);
				break;
			case 106:
				ret = sendfile(data);
				break;
			case 201:
				ret = movej(data);
				break;
			case 303:
				ret = mode(data);
				break;
			default:
				goto end;
		}
	}

	if(ret == FAIL){
		goto end;
	}

	ret = FAIL;
	cJSON *port_n = cJSON_GetObjectItem(data, "port");
	if(port_n != NULL) {
		int port = port_n->valueint;
		printf("port = %d\n", port);
		/* content is empty */
		if(!strcmp(content, "")) {
			goto end;
		}
		printf("content = %s\n", content);
		switch(port) {
			case CM_PORT:
				/* send cmd to 8080 port */
				//上锁
				pthread_mutex_lock(&mute_cmd);
				ret = socket_send(socket_cmd, cmd, content, recvbuf);
				//解锁
				pthread_mutex_unlock(&mute_cmd);
				break;
			case FILE_PORT:
				/* send file cmd to 8082 port*/
				//上锁
				pthread_mutex_lock(&mute_file);
				ret = socket_send(socket_file, cmd, content, recvbuf);
				//解锁
				pthread_mutex_unlock(&mute_file);
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

	/* free content */
	if(content != NULL) {
		free(content);
		content = NULL;
	}
	if(data != NULL) {
		cJSON_Delete(data);
	}
	websSetStatus(wp, 200);
	websWriteHeaders(wp, -1, 0);
	websWriteEndHeaders(wp);
	websWrite(wp, recvbuf);
	websDone(wp);

	return;
	
end:
	/* free content */
	if(content != NULL) {
		free(content);
		content = NULL;
	}
	if(data != NULL) {
		cJSON_Delete(data);
	}
	websSetStatus(wp, 403);
	websWriteHeaders(wp, -1, 0);
	websWriteEndHeaders(wp);
	//websWrite(wp, recvbuf);
	websWrite(wp, "fail");
	websDone(wp);
	return;
}


static int get_points_file()
{
	ret_f_content = openfile(FILE_POINTS);
	/* file is NULL */
	if (ret_f_content == NULL) {
		fprintf(stderr, "open file error!\n");

		return FAIL;
	}

	return SUCCESS;
}


void get(Webs *wp)
{
	int ret = FAIL;
	char *jsonString= wp->input.servp;
	cJSON *data = cJSON_Parse(jsonString);
	if (data == NULL)
	{
		fprintf(stderr, "Parse json file Error!\n");
		goto end;
	}

	char *buf = NULL;
	printf("data:%s\n", buf = cJSON_Print(data));
	free(buf);

	cJSON *command = cJSON_GetObjectItem(data, "data");
	if (command != NULL) {
		char *cmd = command->valuestring;
		if(!strcmp(cmd, "get_points")) {
			ret = get_points_file();
		} else {
			goto end;
		}
	}

	if(ret == FAIL){
		goto end;
	}

	printf("ret_f_content = %s\n", ret_f_content);
	cJSON_Delete(data);
	websSetStatus(wp, 200);
	websWriteHeaders(wp, -1, 0);
	websWriteEndHeaders(wp);
	websWrite(wp, ret_f_content);
	websDone(wp);
	
	if (ret_f_content != NULL) {
		printf("free ret_f_content\n");
		free(ret_f_content);
		ret_f_content = NULL;
	}

	return;

end:
	if (ret_f_content != NULL) {
		printf("free ret_f_content\n");
		free(ret_f_content);
		ret_f_content = NULL;
	}
	cJSON_Delete(data);
	websSetStatus(wp, 403);
	websWriteHeaders(wp, -1, 0);
	websWriteEndHeaders(wp);
	//websWrite(wp, ret_f_content);
	websWrite(wp, "fail");
	websDone(wp);
	return;
}
