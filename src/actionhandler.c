#include    "goahead.h"
#include	"actionhandler.h"

/* set */
static int program_start(const cJSON *data_json);
static int program_stop(const cJSON *data_json);
static int program_pause(const cJSON *data_json);
static int program_resume(const cJSON *data_json);
static int sendfilename(const cJSON *data_json);
static int sendfile(const cJSON *data_json);
static int movej(const cJSON *data_json);
static int mode(const cJSON *data_json);
/* get */
static int get_points_file();
static int get_lua_data();
/* act */
static int save_lua_file(const cJSON *data_json);
static int remove_lua_file(const cJSON *data_json);
static int rename_lua_file(const cJSON *data_json);

static char *content = NULL;
static char *file_content = NULL;
static char *ret_f_content = NULL;

//static int no;
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
	/*calloc file content*/
	file_content = (char *)calloc(1, sizeof(char)*1024);
	if (file_content == NULL) {
		perror("calloc");

		return FAIL;
	}
	sprintf(file_content, "/fruser/%s", name->valuestring);

	return SUCCESS;
}

/* 106 sendFile */
static int sendfile(const cJSON *data_json)
{
	const char s[2] = "\n";
	char *token = NULL;
	char *f_content = NULL;
	cJSON *f_json = NULL;
	int line_num = 0;
	char *tmp_file = NULL;

	cJSON *pgvalue = cJSON_GetObjectItem(data_json, "pgvalue");
	if(pgvalue == NULL || pgvalue->valuestring == NULL || !strcmp(pgvalue->valuestring, "")) {
		goto end;
	}
	printf("upload lua file content:%s\n", pgvalue->valuestring);
	tmp_file = pgvalue->valuestring;
	while(*tmp_file) {
		if(*tmp_file == '\n')
			line_num++;
		tmp_file++;
	}
	/*calloc file content*/
	file_content = (char *)calloc(1, line_num*sizeof(char)*1024);
	if(file_content == NULL){
		perror("file_content error");

		return FAIL;
	}
	/* get first line */
	token = strtok(pgvalue->valuestring, s);
	while(token != NULL) {
		printf("line content = %s\n", token);
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
		if(!strncmp(token, "PTP:", 4)) {
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
			sprintf(file_content, "%sMoveJ(%s,%s,%s,%s,%s,%s,%s,%s)\n", file_content, j1->valuestring, j2->valuestring, j3->valuestring, j4->valuestring, j5->valuestring, j6->valuestring, speed->valuestring, acc->valuestring);
		/* Lin */
		} else if (!strncmp(token, "Lin:", 4)) {
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
			sprintf(file_content, "%sMoveL(%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s)\n", file_content, j1->valuestring, j2->valuestring, j3->valuestring, j4->valuestring, j5->valuestring, j6->valuestring, x->valuestring, y->valuestring, z->valuestring, rx->valuestring, ry->valuestring, rz->valuestring, speed->valuestring, acc->valuestring);
		/* wait time*/
		} else if (!strncmp(token, "WaitTime:", 9)) {
			strrpc(token, "WaitTime:", "");
			sprintf(file_content, "%sWaitMs(%s)\n", file_content, token);
		/* other code send without processing */
		} else {
			sprintf(file_content, "%s%s\n", file_content, token);
		}
		cJSON_Delete(f_json);
		f_json = NULL;
		free(f_content);
		f_content = NULL;
		/* get other line */
		token = strtok(NULL, s);
	}

	return SUCCESS;

end:
	perror("json");
	cJSON_Delete(f_json);
	f_json = NULL;
	free(f_content);
	f_content = NULL;
	return FAIL;
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
	sprintf(content, "MoveJ(%s,%s,%s,%s,%s,%s,%s,100)", j1->valuestring, j2->valuestring, j3->valuestring, j4->valuestring, j5->valuestring, j6->valuestring, speed->valuestring);

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

void set(Webs *wp)
{
	int ret = FAIL;
	int cmd = 0;
	int port = 0;
	char recvbuf[MAX_BUF] = {0};
	char *buf = NULL;
	cJSON *data_json = NULL;
	cJSON *f_content_json = NULL;
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
		case 104:
			ret = program_resume(data_json);
			break;
		case 105:
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
		default:
			perror("cmd not found");
			goto end;
	}
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
		case CM_PORT:
			/* content is empty */
			if(content == NULL || !strcmp(content, "")) {
				perror("content");
				goto end;
			}
			printf("content = %s\n", content);
			/* send cmd to 8080 port */
			pthread_mutex_lock(&mute_cmd);
			ret = socket_send(socket_cmd, cmd, content, recvbuf);
			pthread_mutex_unlock(&mute_cmd);
			break;
		case FILE_PORT:
			/* file_content is empty */
			if(file_content == NULL || !strcmp(file_content, "")) {
				perror("file content");
				goto end;
			}
			printf("file_content = %s\n", file_content);
			/* send file cmd to 8082 port*/
			pthread_mutex_lock(&mute_file);
			ret = socket_send(socket_file, cmd, file_content, recvbuf);
			pthread_mutex_unlock(&mute_file);
			break;
		default:
			perror("port");
			goto end;
	}
	if(ret == FAIL){
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
	/* free file_content */
	free(file_content);
	file_content = NULL;
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
	/* free file_content */
	free(file_content);
	file_content = NULL;
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

/* get points file content */
static int get_points_file()
{
	ret_f_content = get_file_content(FILE_POINTS);
	/* file is NULL */
	if (ret_f_content == NULL) {
		perror("get file content");

		return FAIL;
	}

	return SUCCESS;
}

/* get lua name */
static int get_lua_data()
{
	ret_f_content = get_dir_content(DIR_LUA);
	/* file is NULL */
	if (ret_f_content == NULL) {
		perror("get dir content");

		return FAIL;
	}

	return SUCCESS;
}

void get(Webs *wp)
{
	int ret = FAIL;
	char *buf = NULL;
	char *cmd = NULL;
	cJSON *command = NULL;
	cJSON *data = NULL;

	ret_f_content = NULL;
	data = cJSON_Parse(wp->input.servp);
	if (data == NULL) {
		perror("json");
		goto end;
	}
	printf("data:%s\n", buf = cJSON_Print(data));
	free(buf);
	buf = NULL;
	/* get cmd */
	command = cJSON_GetObjectItem(data, "cmd");
	if (command == NULL) {
		perror("json");
		goto end;
	}
	cmd = command->valuestring;
	if(!strcmp(cmd, "get_points")) {
		ret = get_points_file();
	} else if(!strcmp(cmd, "get_lua_data")) {
		ret = get_lua_data();
	} else {
		perror("cmd not found");
		goto end;
	}
	if(ret == FAIL) {
		perror("ret fail");
		goto end;
	}
	/* cjson delete */
	cJSON_Delete(data);
	data = NULL;
	printf("ret_f_content = %s\n", ret_f_content);
	websSetStatus(wp, 200);
	websWriteHeaders(wp, -1, 0);
	websWriteEndHeaders(wp);
	websWrite(wp, ret_f_content);
	websDone(wp);
	/* free ret_f_content */
	free(ret_f_content);
	ret_f_content = NULL;

	return;

end:
	/* cjson delete */
	cJSON_Delete(data);
	data = NULL;
	websSetStatus(wp, 403);
	websWriteHeaders(wp, -1, 0);
	websWriteEndHeaders(wp);
	//websWrite(wp, ret_f_content);
	websWrite(wp, "fail");
	websDone(wp);
	/* free ret_f_content */
	free(ret_f_content);
	ret_f_content = NULL;
	return;
}

/* save lua file */
static int save_lua_file(const cJSON *data_json)
{
	int ret = FAIL;
	char dir_filename[100] = {0};

	cJSON *file_name = cJSON_GetObjectItem(data_json, "name");
	cJSON *pgvalue = cJSON_GetObjectItem(data_json, "pgvalue");
	if (file_name == NULL || pgvalue == NULL || file_name->valuestring == NULL || pgvalue->valuestring == NULL) {
		perror("json");

		return FAIL;
	}
	sprintf(dir_filename, "%s/%s", DIR_LUA, file_name->valuestring);
	ret = write_file(dir_filename, pgvalue->valuestring);

	return ret;
}

/* remove lua file */
static int remove_lua_file(const cJSON *data_json)
{
	char dir_filename[100] = {0};

	cJSON *name = cJSON_GetObjectItem(data_json, "name");
	if (name == NULL || name->valuestring == NULL) {
		perror("json");

		return FAIL;
	}
	sprintf(dir_filename, "%s/%s", DIR_LUA, name->valuestring);
	if (remove(dir_filename) == -1) {
		perror("remove");

		return FAIL;
	}

	return SUCCESS;
}

/* rename lua file */
static int rename_lua_file(const cJSON *data_json)
{
	char old_filename[100] = {0};
	char new_filename[100] = {0};

	cJSON *oldname = cJSON_GetObjectItem(data_json, "oldname");
	cJSON *newname = cJSON_GetObjectItem(data_json, "newname");
	if (oldname == NULL || newname == NULL || oldname->valuestring == NULL || newname->valuestring == NULL) {
		perror("json");

		return FAIL;
	}
	sprintf(old_filename, "%s/%s", DIR_LUA, oldname->valuestring);
	sprintf(new_filename, "%s/%s", DIR_LUA, newname->valuestring);
	if (rename(old_filename, new_filename) == -1) {
		perror("rename");

		return FAIL;
	}

	return SUCCESS;
}

void act(Webs *wp)
{
	int ret = FAIL;
	char *buf = NULL;
	char *cmd = NULL;
	cJSON *command = NULL;
	cJSON *data = NULL;
	cJSON *data_json = NULL;

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
	/* get cmd */
	command = cJSON_GetObjectItem(data, "cmd");
	if (command == NULL) {
		perror("json");
		goto end;
	}
	cmd = command->valuestring;
	if(!strcmp(cmd, "save_lua_file")) {
		ret = save_lua_file(data_json);
	} else if(!strcmp(cmd, "remove_lua_file")) {
		ret = remove_lua_file(data_json);
	} else if(!strcmp(cmd, "rename_lua_file")) {
		ret = rename_lua_file(data_json);
	} else {
		perror("cmd not found");
		goto end;
	}
	if(ret == FAIL){
		perror("ret fail");
		goto end;
	}
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
