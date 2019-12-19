
/********************************* Includes ***********************************/

#include    "goahead.h"
#include	"cJSON.h"
#include 	"tools.h"
#include 	"robot_socket.h"
#include	"action_set.h"

/********************************* Defines ************************************/

static char *content = NULL;
extern int socket_cmd;
extern int socket_file;
extern pthread_mutex_t mute_cmd;
extern pthread_mutex_t mute_file;

/********************************* Function declaration ***********************/

static int program_start(const cJSON *data_json);
static int program_stop(const cJSON *data_json);
static int program_pause(const cJSON *data_json);
static int program_resume(const cJSON *data_json);
static int sendfilename(const cJSON *data_json);
static int parse_lua_cmd(char *lua_cmd);
static int sendfile(const cJSON *data_json);
static int step_over(const cJSON *data_json);
static int movej(const cJSON *data_json);
static int mode(const cJSON *data_json);

/*********************************** Code *************************************/


/*action*/
//static int joints(cJSON *data, char *recvbuf);
//static int tcp(cJSON *data, char *recvbuf);
//static int no;
/*static int joints(cJSON *data, char *recvbuf)
{
	int ret;

	cJSON *shoulder_pan_joint = cJSON_GetObjectItem(data, "shoulder_pan_joint");
	printf("shoulder_pan_joint:%s\n", shoulder_pan_joint->valuestring);

	no = 303;
	sprintf(content, "Mode(1)");
	ret = socket_client(no, content, recvbuf, CMD_PORT);

	int i;
	for(i = 1; i <= 10; i++){
		no = 208;
		sprintf(content, "MJOINT(1,1,%d,100,100)", i);
		ret = socket_client(no, content, recvbuf, CMD_PORT);
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
static int program_start(const cJSON *data_json)
{
	sprintf(content, "START");

	return SUCCESS;
}

/* 102 STOP */
static int program_stop(const cJSON *data_json)
{
	sprintf(content, "STOP");

	return SUCCESS;
}

/* 103 PAUSE */
static int program_pause(const cJSON *data_json)
{
	sprintf(content, "PAUSE");

	return SUCCESS;
}

/* 104 RESUME */
static int program_resume(const cJSON *data_json)
{
	sprintf(content, "RESUME");

	return SUCCESS;
}

/* 105 sendFileName */
static int sendfilename(const cJSON *data_json)
{
	cJSON *name = cJSON_GetObjectItem(data_json, "name");
	if (name == NULL || name->valuestring == NULL) {
		perror("json");

		return FAIL;
	}
	sprintf(content, "%s%s", PATH_SEND_LUA, name->valuestring);

	return SUCCESS;
}

static int parse_lua_cmd(char *lua_cmd)
{
	printf("lua cmd = %s\n", lua_cmd);
	char *f_content = NULL;
	cJSON *f_json = NULL;

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
		speed = cJSON_GetObjectItem(ptp, "speed");
		acc = cJSON_GetObjectItem(ptp, "acc");
		if(j1->valuestring == NULL || j2->valuestring == NULL || j3->valuestring == NULL || j4->valuestring == NULL || j5->valuestring == NULL || j6->valuestring == NULL || speed->valuestring == NULL || acc->valuestring == NULL) { 
			goto end;
		}
		sprintf(content, "%sMoveJ(%s,%s,%s,%s,%s,%s,%s,%s)\n", content, j1->valuestring, j2->valuestring, j3->valuestring, j4->valuestring, j5->valuestring, j6->valuestring, speed->valuestring, acc->valuestring);
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
		sprintf(content, "%sMoveL(%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s)\n", content, j1->valuestring, j2->valuestring, j3->valuestring, j4->valuestring, j5->valuestring, j6->valuestring, x->valuestring, y->valuestring, z->valuestring, rx->valuestring, ry->valuestring, rz->valuestring, speed->valuestring, acc->valuestring);
	/* wait time*/
	} else if (!strncmp(lua_cmd, "WaitTime:", 9)) {
		strrpc(lua_cmd, "WaitTime:", "");
		sprintf(content, "%sWaitMs(%s)\n", content, lua_cmd);
	/* other code send without processing */
	} else {
		sprintf(content, "%s%s\n", content, lua_cmd);
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
static int sendfile(const cJSON *data_json)
{
	const char s[2] = "\n";
	char *token = NULL;
	int line_num = 0;
	char *tmp_file = NULL;

	cJSON *pgvalue = cJSON_GetObjectItem(data_json, "pgvalue");
	if (pgvalue == NULL || pgvalue->valuestring == NULL || !strcmp(pgvalue->valuestring, "")) {
		perror("json");

		return FAIL;
	}
	printf("upload lua file content:%s\n", pgvalue->valuestring);
	tmp_file = pgvalue->valuestring;
	while(*tmp_file) {
		if (*tmp_file == '\n') {
			line_num++;
		}
		tmp_file++;
	}
	/* realloc content */
	content = (char *)realloc(content, line_num*sizeof(char)*1024);
	if (content == NULL) {
		perror("realloc");

		return FAIL;
	}
	memset(content, 0, line_num*sizeof(char)*1024);
	/* get first line */
	token = strtok(pgvalue->valuestring, s);
	while(token != NULL) {
		if (parse_lua_cmd(token) == FAIL) {
			return FAIL;
		}
		/* get other line */
		token = strtok(NULL, s);
	}

	return SUCCESS;
}

/* 1001 step over */
static int step_over(const cJSON *data_json)
{
	int ret = FAIL;
	int cmd = 0;

	cJSON *pgvalue = cJSON_GetObjectItem(data_json, "pgline");
	if (pgvalue == NULL || pgvalue->valuestring == NULL || !strcmp(pgvalue->valuestring, "")) {
		perror("json");

		return FAIL;
	}
	printf("upload lua cmd:%s\n", pgvalue->valuestring);
	/* PTP */
	if(!strncmp(pgvalue->valuestring, "PTP:", 4)) {
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
	if (parse_lua_cmd(pgvalue->valuestring) == FAIL) {
		return FAIL;
	}

	return cmd; 
}

/* 201 MoveJ */
static int movej(const cJSON *data_json)
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
static int mode(const cJSON *data_json)
{
	cJSON *mode = cJSON_GetObjectItem(data_json, "mode");
	if(mode == NULL || mode->valuestring == NULL) {
		perror("json");

		return FAIL;
	}
	sprintf(content, "Mode(%s)", mode->valuestring);

	return SUCCESS;
}

/* set user cmd to task manager */
void set(Webs *wp)
{
	int ret = FAIL;
	int cmd = 0;
	int port = 0;
	char recvbuf[MAX_BUF] = {0};
	char *buf = NULL;
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
	/*calloc content*/
	content = (char *)calloc(1, sizeof(char)*1024);
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
		case 100:
			ret = sendfile(data_json);
			break;
		case 101:
			ret = program_start(data_json);
			break;
		case 102:
			ret = program_stop(data_json);
			break;
		case 103:
			ret = program_pause(data_json);
			break;
		case 104:/*8081*/
			ret = program_resume(data_json);
			break;
		case 105:/*8081*/
			ret = sendfilename(data_json);
			break;
		case 106:
			ret = sendfile(data_json);
			break;
		case 201:
			ret = movej(data_json);
			break;
		case 303:
			ret = mode(data_json);
			break;
		case 1001:
			ret = step_over(data_json);
			cmd = ret;
			break;
		default:
			perror("cmd not found");
			goto end;
	}
	/* content is empty */
	if(content == NULL || !strcmp(content, "")) {
		perror("content");
		goto end;
	}
	printf("content = %s\n", content);
	if(ret == FAIL){
		perror("ret fail");
		goto end;
	}
	ret = FAIL;
	/* get port */
	port_n = cJSON_GetObjectItem(data, "port");
	if(port_n == NULL) {
		perror("json");
		goto end;
	}
	port = port_n->valueint;
	switch(port) {
		case CMD_PORT:
			/* send cmd to 8080 port */
			pthread_mutex_lock(&mute_cmd);
			ret = socket_send(socket_cmd, cmd, content, recvbuf);
			pthread_mutex_unlock(&mute_cmd);
			break;
		case FILE_PORT:
			/* send file cmd to 8082 port*/
			pthread_mutex_lock(&mute_file);
			ret = socket_send(socket_file, cmd, content, recvbuf);
			pthread_mutex_unlock(&mute_file);
			break;
		default:
			perror("port");
			goto end;
	}
	if (ret == FAIL) {
		perror("ret fail");
		goto end;
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
	free(content);
	content = NULL;
	/* cjson delete */
	cJSON_Delete(data);
	data = NULL;

	websSetStatus(wp, 200);
	websWriteHeaders(wp, -1, 0);
	websWriteEndHeaders(wp);
	websWrite(wp, recvbuf);
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
	//websWrite(wp, recvbuf);
	websWrite(wp, "fail");
	websDone(wp);
	return;
}
