
/********************************* Includes ***********************************/

#include    "goahead.h"
#include	"cJSON.h"
#include 	"tools.h"
#include 	"robot_socket.h"
#include	"action_set.h"

/********************************* Defines ************************************/

extern SOCKET_INFO socket_cmd;
extern SOCKET_INFO socket_file;
extern SOCKET_INFO socket_vir_cmd;
extern SOCKET_INFO socket_vir_file;
extern int robot_type;
//extern pthread_cond_t cond_cmd;
//extern pthread_cond_t cond_file;

/********************************* Function declaration ***********************/

static int program_start(const cJSON *data_json, char *content);
static int program_stop(const cJSON *data_json, char *content);
static int program_pause(const cJSON *data_json, char *content);
static int program_resume(const cJSON *data_json, char *content);
static int sendfilename(const cJSON *data_json, char *content);
static int parse_lua_cmd(char *lua_cmd, int len, char *file_content);
static int sendfile(const cJSON *data_json, int content_len, char *content);
static int step_over(const cJSON *data_json, char *content);
static int movej(const cJSON *data_json, char *content);
static int mode(const cJSON *data_json, char *content);
static int enquene_result_dequene(SOCKET_INFO *sock, const int type, pthread_mutex_t *mute, char *send_content);
static int get_lua_content_size(const cJSON *data_json);

/*********************************** Code *************************************/

/* 101 START */
static int program_start(const cJSON *data_json, char *content)
{
	sprintf(content, "START");

	return SUCCESS;
}

/* 102 STOP */
static int program_stop(const cJSON *data_json, char *content)
{
	sprintf(content, "STOP");

	return SUCCESS;
}

/* 103 PAUSE */
static int program_pause(const cJSON *data_json, char *content)
{
	sprintf(content, "PAUSE");

	return SUCCESS;
}

/* 104 RESUME */
static int program_resume(const cJSON *data_json, char *content)
{
	sprintf(content, "RESUME");

	return SUCCESS;
}

/* 105 sendFileName */
static int sendfilename(const cJSON *data_json, char *content)
{
	cJSON *name = cJSON_GetObjectItem(data_json, "name");
	if (name == NULL || name->valuestring == NULL) {
		perror("json");

		return FAIL;
	}
	sprintf(content, "%s%s", DIR_FRUSER, name->valuestring);
	//printf("content = %s\n", content);

	return SUCCESS;
}

static int parse_lua_cmd(char *lua_cmd, int len, char *file_content)
{
	//printf("lua cmd = %s\n", lua_cmd);
	char *f_content = NULL;
	cJSON *f_json = NULL;
	char tmp_content[len];
	memset(tmp_content, 0, len);

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

	/* PTP */
	if(!strncmp(lua_cmd, "PTP:", 4)) {
		/* open and get points file content */
		f_content = get_file_content(FILE_POINTS);
		/* file is NULL */
		if (f_content == NULL) {
			perror("get file content");

			return FAIL;
		}
		f_json = cJSON_Parse(f_content);
		if (f_json == NULL) {
			goto end;
		}
		strrpc(lua_cmd, "PTP:", "");
		cJSON *ptp = cJSON_GetObjectItem(f_json, lua_cmd);
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
		cJSON *tcp = cJSON_GetObjectItem(ptp, "tcp");
		if (tcp == NULL || tcp->type != cJSON_Object) {
			goto end;
		}
		x = cJSON_GetObjectItem(tcp, "x");
		y = cJSON_GetObjectItem(tcp, "y");
		z = cJSON_GetObjectItem(tcp, "z");
		rx = cJSON_GetObjectItem(tcp, "rx");
		ry = cJSON_GetObjectItem(tcp, "ry");
		rz = cJSON_GetObjectItem(tcp, "rz");
		speed = cJSON_GetObjectItem(ptp, "speed");
		acc = cJSON_GetObjectItem(ptp, "acc");
		if(j1->valuestring == NULL || j2->valuestring == NULL || j3->valuestring == NULL || j4->valuestring == NULL || j5->valuestring == NULL || j6->valuestring == NULL|| x->valuestring == NULL || y->valuestring == NULL || z->valuestring == NULL || rx->valuestring == NULL || ry->valuestring == NULL || rz->valuestring == NULL || speed->valuestring == NULL || acc->valuestring == NULL) {
			goto end;
		}
		sprintf(tmp_content, "%sMoveJ(%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s)\n", file_content, j1->valuestring, j2->valuestring, j3->valuestring, j4->valuestring, j5->valuestring, j6->valuestring, x->valuestring, y->valuestring, z->valuestring, rx->valuestring, ry->valuestring, rz->valuestring, speed->valuestring, acc->valuestring);
		strcpy(file_content, tmp_content);
	/* Lin */
	} else if (!strncmp(lua_cmd, "Lin:", 4)) {
		/* open points file */
		f_content = get_file_content(FILE_POINTS);
		/* file is NULL */
		if (f_content == NULL) {
			perror("get file content");

			return FAIL;
		}
		f_json = cJSON_Parse(f_content);
		if (f_json == NULL) {
			goto end;
		}
		strrpc(lua_cmd, "Lin:", "");
		cJSON *lin = cJSON_GetObjectItem(f_json, lua_cmd);
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
		sprintf(tmp_content, "%sMoveL(%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s)\n", file_content, j1->valuestring, j2->valuestring, j3->valuestring, j4->valuestring, j5->valuestring, j6->valuestring, x->valuestring, y->valuestring, z->valuestring, rx->valuestring, ry->valuestring, rz->valuestring, speed->valuestring, acc->valuestring);
		strcpy(file_content, tmp_content);
	/* wait time*/
	} else if (!strncmp(lua_cmd, "WaitTime:", 9)) {
		strrpc(lua_cmd, "WaitTime:", "");
		sprintf(tmp_content, "%sWaitMs(%s)\n", file_content, lua_cmd);
		strcpy(file_content, tmp_content);
	/* other code send without processing */
	} else {
		sprintf(tmp_content, "%s%s\n", file_content, lua_cmd);
		strcpy(file_content, tmp_content);
	}
	cJSON_Delete(f_json);
	f_json = NULL;
	free(f_content);
	f_content = NULL;

	return SUCCESS;

end:
	cJSON_Delete(f_json);
	f_json = NULL;
	free(f_content);
	f_content = NULL;
	return FAIL;
}

/* 106 sendFile */
static int sendfile(const cJSON *data_json, int content_len, char *content)
{
	const char s[2] = "\n";
	char *token = NULL;

	cJSON *pgvalue = cJSON_GetObjectItem(data_json, "pgvalue");
	if (pgvalue == NULL || pgvalue->valuestring == NULL || !strcmp(pgvalue->valuestring, "")) {
		perror("json");

		return FAIL;
	}
	/* get first line */
	token = strtok(pgvalue->valuestring, s);
	while (token != NULL) {
		if (parse_lua_cmd(token, content_len, content) == FAIL) {
			return FAIL;
		}
		/* get other line */
		token = strtok(NULL, s);
	}
	//printf("content = %s\n", content);

	return SUCCESS;
}

/* 1001 step over */
static int step_over(const cJSON *data_json, char *content)
{
	int cmd = 0;

	cJSON *pgvalue = cJSON_GetObjectItem(data_json, "pgline");
	if (pgvalue == NULL || pgvalue->valuestring == NULL || !strcmp(pgvalue->valuestring, "")) {
		perror("json");

		return FAIL;
	}
	//printf("upload lua cmd:%s\n", pgvalue->valuestring);
	/* PTP */
	if (!strncmp(pgvalue->valuestring, "PTP:", 4)) {
		cmd = 201;
	/* Lin */
	} else if (!strncmp(pgvalue->valuestring, "Lin:", 4)) {
		cmd = 203;
	/* wait time*/
	} else if (!strncmp(pgvalue->valuestring, "WaitTime:", 9)) {
		cmd = 207;
	/* error */
	} else {
		return FAIL;
	}
	if (parse_lua_cmd(pgvalue->valuestring, sizeof(char)*MAX_BUF, content) == FAIL) {
		return FAIL;
	}

	return cmd; 
}

/* 201 MoveJ */
static int movej(const cJSON *data_json, char *content)
{
	cJSON *joints = cJSON_GetObjectItem(data_json, "joints");
	if (joints == NULL || joints->type != cJSON_Object) {
		perror("json");

		return FAIL;
	}
	cJSON *j1 = cJSON_GetObjectItem(joints, "j1");
	cJSON *j2 = cJSON_GetObjectItem(joints, "j2");
	cJSON *j3 = cJSON_GetObjectItem(joints, "j3");
	cJSON *j4 = cJSON_GetObjectItem(joints, "j4");
	cJSON *j5 = cJSON_GetObjectItem(joints, "j5");
	cJSON *j6 = cJSON_GetObjectItem(joints, "j6");
	if(j1->valuestring == NULL || j2->valuestring == NULL || j3->valuestring == NULL || j4->valuestring == NULL || j5->valuestring == NULL || j6->valuestring == NULL) {
		perror("json");

		return FAIL;
	}
	cJSON *speed = cJSON_GetObjectItem(data_json, "speed");
	if (speed->valuestring == NULL) {
		perror("json");

		return FAIL;
	}
	cJSON *acc = cJSON_GetObjectItem(data_json, "acc");
	if (acc->valuestring == NULL) {
		perror("json");

		return FAIL;
	}
	sprintf(content, "MoveJ(%s,%s,%s,%s,%s,%s,%s,%s)", j1->valuestring, j2->valuestring, j3->valuestring, j4->valuestring, j5->valuestring, j6->valuestring, speed->valuestring, acc->valuestring);

	return SUCCESS;
}

/* 303 Mode */
static int mode(const cJSON *data_json, char *content)
{
	cJSON *mode = cJSON_GetObjectItem(data_json, "mode");
	if(mode == NULL || mode->valuestring == NULL) {
		perror("json");

		return FAIL;
	}
	sprintf(content, "Mode(%s)", mode->valuestring);

	return SUCCESS;
}

static int enquene_result_dequene(SOCKET_INFO *sock, const int type, pthread_mutex_t *mute, char *send_content)
{
	int ret = FAIL;
	/* socket 连接已经断开 */
	if (sock->connect_status == 0) {

		return FAIL;
	}

	/* 创建结点 */
	QElemType node;
	createnode(&node, type, send_content);

	/* 创建结点插入队列中 */
	pthread_mutex_lock(mute);
	if (sock->msghead >= MAX_MSGHEAD) {
		sock->msghead = 1;
	} else {
		sock->msghead++;
	}
	node.msghead = sock->msghead;
	enquene(&sock->quene, node);
	//pthread_cond_signal(&cond_cmd);
	pthread_mutex_unlock(mute);

	ret = quene_recv_result(node, sock->quene);

	/* 把结点从队列中删除 */
	pthread_mutex_lock(mute);
	dequene(&sock->quene, node);
	pthread_mutex_unlock(mute);
	// TODO: add signal

	return ret;
}

/* get lua content size */
static int get_lua_content_size(const cJSON *data_json)
{
	char *tmp_file = NULL;
	int line_num = 0;
	int content_size = 0;

	cJSON *pgvalue = cJSON_GetObjectItem(data_json, "pgvalue");
	if (pgvalue == NULL || pgvalue->valuestring == NULL || !strcmp(pgvalue->valuestring, "")) {
		perror("json");

		return FAIL;
	}
	//printf("upload lua file content:%s\n", pgvalue->valuestring);
	tmp_file = pgvalue->valuestring;
	while (*tmp_file) {
		if (*tmp_file == '\n') {
			line_num++;
		}
		tmp_file++;
	}

	content_size = line_num*sizeof(char)*MAX_BUF;

	return content_size;
}

/* set user cmd to task manager */
void set(Webs *wp)
{
	char *content = NULL;
	char *buf = NULL;
	int ret = FAIL;
	int cmd = 0;
	int port = 0;
	int content_len = sizeof(char)*MAX_BUF;

	cJSON *data_json = NULL;
	cJSON *command = NULL;
	cJSON *port_n = NULL;
	cJSON *data = NULL;

	data = cJSON_Parse(wp->input.servp);
	if (data == NULL) {
		perror("json");
		goto end;
	}
	printf("data:%s\n", buf = cJSON_Print(data));
	free(buf);
	buf = NULL;
	/* get data json */
	data_json = cJSON_GetObjectItem(data, "data");
	if (data_json == NULL || data_json->type != cJSON_Object) {
		perror("json");
		goto end;
	}
	/* calloc content */
	content = (char *)calloc(1, sizeof(char)*MAX_BUF);
	if (content == NULL) {
		perror("calloc");
		goto end;
	}
	/* get cmd */
	command = cJSON_GetObjectItem(data, "cmd");
	if(command == NULL) {
		perror("json");
		goto end;
	}
	cmd = command->valueint;
	switch(cmd) {
		case 100:/* 8082 */
			content_len = get_lua_content_size(data_json);
			if (content_len == FAIL) {
				perror("get lua content size");

				goto end;
			}
			// realloc content
			content = (char *)realloc(content, content_len);
			if (content == NULL) {
				perror("realloc");

				goto end;
			}
			memset(content, 0, content_len);
			ret = sendfile(data_json, content_len, content);
			break;
		case 101:
			ret = program_start(data_json, content);
			break;
		case 102:
			ret = program_stop(data_json, content);
			break;
		case 103:
			ret = program_pause(data_json, content);
			break;
		case 104:
			ret = program_resume(data_json, content);
			break;
		case 105:/* 8082 */
			ret = sendfilename(data_json, content);
			break;
		case 106:/* 8082 */
			content_len = get_lua_content_size(data_json);
			if (content_len == FAIL) {
				perror("get lua content size");

				goto end;
			}
			// realloc content
			content = (char *)realloc(content, content_len);
			if (content == NULL) {
				perror("realloc");

				goto end;
			}
			memset(content, 0, content_len);
			ret = sendfile(data_json, content_len, content);
			break;
		case 201:
			ret = movej(data_json, content);
			break;
		case 303:
			ret = mode(data_json, content);
			break;
		case 1001:/* 内部定义指令 */
			ret = step_over(data_json, content);
			if (ret == FAIL) {
				perror("step over");

				goto end;
			}
			cmd = ret;
			break;
		default:
			perror("cmd not found");
			goto end;
	}
	if (ret == FAIL) {
		perror("content fail");
		goto end;
	}
	ret = FAIL;
	/* content is empty */
	if (content == NULL || !strcmp(content, "")) {
		perror("content");
		goto end;
	}
	//printf("content = %s\n", content);
	/* get port */
	port_n = cJSON_GetObjectItem(data, "port");
	if (port_n == NULL) {
		perror("json");
		goto end;
	}
	port = port_n->valueint;

	/** virtual robot */
	if (robot_type == 0) {
		switch (port) {
			case CMD_PORT:
				port = 8070;
				break;
			case FILE_PORT:
				port = 8072;
				break;
			default:
				perror("port");
				goto end;
		}
	}

	switch (port) {
		/* send cmd to 8080 port */
		case CMD_PORT:
			ret = enquene_result_dequene(&socket_cmd, cmd, &socket_cmd.mute, content);
			break;
		/* send file cmd to 8082 port */
		case FILE_PORT:
			ret = enquene_result_dequene(&socket_file, cmd, &socket_file.mute, content);
			break;
		/* send cmd to 8070 port */
		case VIR_CMD_PORT:
			ret = enquene_result_dequene(&socket_vir_cmd, cmd, &socket_vir_cmd.mute, content);
			break;
		/* send file cmd to 8072 port */
		case VIR_FILE_PORT:
			ret = enquene_result_dequene(&socket_vir_file, cmd, &socket_vir_file.mute, content);
			break;
		default:
			perror("port");
			goto end;
	}
	if (ret == FAIL) {
		perror("socket fail");
		goto end;
	}

	/* free content */
	free(content);
	content = NULL;
	/* cjson delete */
	cJSON_Delete(data);
	data = NULL;

	websSetStatus(wp, 200);
	websWriteHeaders(wp, -1, 0);
	websWriteEndHeaders(wp);
	websWrite(wp, "success");
	websDone(wp);

	return;
	
end:
	/* free content */
	free(content);
	content = NULL;
	/* cjson delete */
	cJSON_Delete(data);
	data = NULL;
	websSetStatus(wp, 403);
	websWriteHeaders(wp, -1, 0);
	websWriteEndHeaders(wp);
	websWrite(wp, "fail");
	websDone(wp);
	return;
}
