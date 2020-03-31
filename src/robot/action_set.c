
/********************************* Includes ***********************************/

#include    "goahead.h"
#include	"cJSON.h"
#include 	"tools.h"
#include 	"robot_socket.h"
#include	"action_set.h"

/********************************* Defines ************************************/

extern CTRL_STATE ctrl_state;
extern CTRL_STATE vir_ctrl_state;
extern FB_LinkQuene fb_quene;
extern SOCKET_INFO socket_cmd;
extern SOCKET_INFO socket_file;
extern SOCKET_INFO socket_vir_cmd;
extern SOCKET_INFO socket_vir_file;
extern int robot_type;
extern STATE_FEEDBACK state_fb;
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
static int set_state_id(const cJSON *data_json, char *content);
static int set_state(const cJSON *data_json, char *content);
static int mode(const cJSON *data_json, char *content);
static int jointtotcp(const cJSON *data_json, char *content);
//static int setvirtualrobotinitpos(const cJSON *data_json, char *content);
static int enquene_result_dequene(SOCKET_INFO *sock, const int type, pthread_mutex_t *mute, char *send_content, char *recv_content);
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
	char cmd_array[10][10] = {{0}};
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
	cJSON *toolnum = NULL;

	cJSON *j1_2 = NULL;
	cJSON *j2_2 = NULL;
	cJSON *j3_2 = NULL;
	cJSON *j4_2 = NULL;
	cJSON *j5_2 = NULL;
	cJSON *j6_2 = NULL;
	cJSON *x_2 = NULL;
	cJSON *y_2 = NULL;
	cJSON *z_2 = NULL;
	cJSON *rx_2 = NULL;
	cJSON *ry_2 = NULL;
	cJSON *rz_2 = NULL;
	cJSON *speed_2 = NULL;
	cJSON *acc_2 = NULL;
	cJSON *toolnum_2 = NULL;

	/* PTP */
	if(!strncmp(lua_cmd, "PTP:", 4)) {
		strrpc(lua_cmd, "PTP:", "");
		if (separate_string_to_array(lua_cmd, ",", 2, 10, (char *)&cmd_array) != 2) {
			perror("separate recv");

			return FAIL;
		}
		/*
		printf("cmd_array[0] = %s", cmd_array[0]);
		printf("cmd_array[1] = %s", cmd_array[1]);
		*/
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
		cJSON *ptp = cJSON_GetObjectItem(f_json, cmd_array[0]);
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
		toolnum = cJSON_GetObjectItem(ptp, "toolnum");
		speed = cJSON_GetObjectItem(ptp, "speed");
		acc = cJSON_GetObjectItem(ptp, "acc");
		if(j1->valuestring == NULL || j2->valuestring == NULL || j3->valuestring == NULL || j4->valuestring == NULL || j5->valuestring == NULL || j6->valuestring == NULL|| x->valuestring == NULL || y->valuestring == NULL || z->valuestring == NULL || rx->valuestring == NULL || ry->valuestring == NULL || rz->valuestring == NULL || toolnum->valuestring == NULL|| speed->valuestring == NULL || acc->valuestring == NULL || cmd_array[1] == NULL) {
			goto end;
		}
		sprintf(tmp_content, "%sMoveJ(%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s)\n", file_content, j1->valuestring, j2->valuestring, j3->valuestring, j4->valuestring, j5->valuestring, j6->valuestring, x->valuestring, y->valuestring, z->valuestring, rx->valuestring, ry->valuestring, rz->valuestring, toolnum->valuestring, speed->valuestring, acc->valuestring, cmd_array[1]);
		strcpy(file_content, tmp_content);
	/* ARC */
	} else if(!strncmp(lua_cmd, "ARC:", 3)) {
		strrpc(lua_cmd, "ARC:", "");
		if (separate_string_to_array(lua_cmd, ",", 3, 10, (char *)&cmd_array) != 3) {
			perror("separate recv");

			return FAIL;
		}
		/*
		printf("cmd_array[0] = %s", cmd_array[0]);
		printf("cmd_array[1] = %s", cmd_array[1]);
		printf("cmd_array[2] = %s", cmd_array[2]);
		*/
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
		cJSON *point_1 = cJSON_GetObjectItem(f_json, cmd_array[0]);
		if (point_1 == NULL || point_1->type != cJSON_Object) {
			goto end;
		}
		cJSON *joints = cJSON_GetObjectItem(point_1, "joints");
		if (joints == NULL || joints->type != cJSON_Object) {
			goto end;
		}
		j1 = cJSON_GetObjectItem(joints, "j1");
		j2 = cJSON_GetObjectItem(joints, "j2");
		j3 = cJSON_GetObjectItem(joints, "j3");
		j4 = cJSON_GetObjectItem(joints, "j4");
		j5 = cJSON_GetObjectItem(joints, "j5");
		j6 = cJSON_GetObjectItem(joints, "j6");
		cJSON *tcp = cJSON_GetObjectItem(point_1, "tcp");
		if (tcp == NULL || tcp->type != cJSON_Object) {
			goto end;
		}
		x = cJSON_GetObjectItem(tcp, "x");
		y = cJSON_GetObjectItem(tcp, "y");
		z = cJSON_GetObjectItem(tcp, "z");
		rx = cJSON_GetObjectItem(tcp, "rx");
		ry = cJSON_GetObjectItem(tcp, "ry");
		rz = cJSON_GetObjectItem(tcp, "rz");
		toolnum = cJSON_GetObjectItem(point_1, "toolnum");
		speed = cJSON_GetObjectItem(point_1, "speed");
		acc = cJSON_GetObjectItem(point_1, "acc");
		if(j1->valuestring == NULL || j2->valuestring == NULL || j3->valuestring == NULL || j4->valuestring == NULL || j5->valuestring == NULL || j6->valuestring == NULL|| x->valuestring == NULL || y->valuestring == NULL || z->valuestring == NULL || rx->valuestring == NULL || ry->valuestring == NULL || rz->valuestring == NULL || toolnum->valuestring == NULL|| speed->valuestring == NULL || acc->valuestring == NULL) {
			goto end;
		}
		cJSON *point_2 = cJSON_GetObjectItem(f_json, cmd_array[1]);
		if (point_2 == NULL || point_2->type != cJSON_Object) {
			goto end;
		}
		cJSON *joints_2 = cJSON_GetObjectItem(point_2, "joints");
		if (joints_2 == NULL || joints_2->type != cJSON_Object) {
			goto end;
		}
		j1_2 = cJSON_GetObjectItem(joints_2, "j1");
		j2_2 = cJSON_GetObjectItem(joints_2, "j2");
		j3_2 = cJSON_GetObjectItem(joints_2, "j3");
		j4_2 = cJSON_GetObjectItem(joints_2, "j4");
		j5_2 = cJSON_GetObjectItem(joints_2, "j5");
		j6_2 = cJSON_GetObjectItem(joints_2, "j6");
		cJSON *tcp_2 = cJSON_GetObjectItem(point_2, "tcp");
		if (tcp_2 == NULL || tcp_2->type != cJSON_Object) {
			goto end;
		}
		x_2 = cJSON_GetObjectItem(tcp_2, "x");
		y_2 = cJSON_GetObjectItem(tcp_2, "y");
		z_2 = cJSON_GetObjectItem(tcp_2, "z");
		rx_2 = cJSON_GetObjectItem(tcp_2, "rx");
		ry_2 = cJSON_GetObjectItem(tcp_2, "ry");
		rz_2 = cJSON_GetObjectItem(tcp_2, "rz");
		toolnum_2 = cJSON_GetObjectItem(point_2, "toolnum");
		speed_2 = cJSON_GetObjectItem(point_2, "speed");
		acc_2 = cJSON_GetObjectItem(point_2, "acc");
		if(j1_2->valuestring == NULL || j2_2->valuestring == NULL || j3_2->valuestring == NULL || j4_2->valuestring == NULL || j5_2->valuestring == NULL || j6_2->valuestring == NULL|| x_2->valuestring == NULL || y_2->valuestring == NULL || z_2->valuestring == NULL || rx_2->valuestring == NULL || ry_2->valuestring == NULL || rz_2->valuestring == NULL || toolnum_2->valuestring == NULL|| speed_2->valuestring == NULL || acc_2->valuestring == NULL || cmd_array[2] == NULL) {
			goto end;
		}
		sprintf(tmp_content, "%sMoveC(%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s))\n", file_content, j1->valuestring, j2->valuestring, j3->valuestring, j4->valuestring, j5->valuestring, j6->valuestring, x->valuestring, y->valuestring, z->valuestring, rx->valuestring, ry->valuestring, rz->valuestring, toolnum->valuestring, speed->valuestring, acc->valuestring, j1_2->valuestring, j2_2->valuestring, j3_2->valuestring, j4_2->valuestring, j5_2->valuestring, j6_2->valuestring, x_2->valuestring, y_2->valuestring, z_2->valuestring, rx_2->valuestring, ry_2->valuestring, rz_2->valuestring, toolnum_2->valuestring, speed_2->valuestring, acc_2->valuestring, cmd_array[2]);
		strcpy(file_content, tmp_content);
	/* Lin */
	} else if (!strncmp(lua_cmd, "Lin:", 4)) {
		strrpc(lua_cmd, "Lin:", "");
		if (separate_string_to_array(lua_cmd, ",", 3, 10, (char *)&cmd_array) != 3) {
			perror("separate recv");

			return FAIL;
		}
		/*
		printf("cmd_array[0] = %s", cmd_array[0]);
		printf("cmd_array[1] = %s", cmd_array[1]);
		printf("cmd_array[2] = %s", cmd_array[2]);
		*/
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
		toolnum = cJSON_GetObjectItem(lin, "toolnum");
		if(j1->valuestring == NULL || j2->valuestring == NULL || j3->valuestring == NULL || j4->valuestring == NULL || j5->valuestring == NULL || j6->valuestring == NULL || x->valuestring == NULL || y->valuestring == NULL || z->valuestring == NULL || rx->valuestring == NULL || ry->valuestring == NULL || rz->valuestring == NULL || toolnum->valuestring || speed->valuestring == NULL || acc->valuestring == NULL || cmd_array[1] == NULL || cmd_array[2] == NULL) { 
			goto end;
		}
		sprintf(tmp_content, "%sMoveL(%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s)\n", file_content, j1->valuestring, j2->valuestring, j3->valuestring, j4->valuestring, j5->valuestring, j6->valuestring, x->valuestring, y->valuestring, z->valuestring, rx->valuestring, ry->valuestring, rz->valuestring, toolnum->valuestring, speed->valuestring, acc->valuestring, cmd_array[1], cmd_array[2]);
		strcpy(file_content, tmp_content);
	/* set DO */
	} else if (!strncmp(lua_cmd, "SetDO:", 6)) {
		strrpc(lua_cmd, "SetDO:", "");
		if (separate_string_to_array(lua_cmd, ",", 3, 10, (char *)&cmd_array) != 3) {
			perror("separate recv");

			return FAIL;
		}
		/*
		printf("cmd_array[0] = %s", cmd_array[0]);
		printf("cmd_array[1] = %s", cmd_array[1]);
		printf("cmd_array[2] = %s", cmd_array[2]);
		*/
		sprintf(tmp_content, "%sSetDO(%s,%s,%s)\n", file_content, cmd_array[0], cmd_array[1], cmd_array[2]);
		strcpy(file_content, tmp_content);
	/* set AO */
	} else if (!strncmp(lua_cmd, "SetAO:", 6)) {
		strrpc(lua_cmd, "SetAO:", "");
		if (separate_string_to_array(lua_cmd, ",", 2, 10, (char *)&cmd_array) != 2) {
			perror("separate recv");

			return FAIL;
		}
		/*
		printf("cmd_array[0] = %s", cmd_array[0]);
		printf("cmd_array[1] = %s", cmd_array[1]);
		*/
		sprintf(tmp_content, "%sSetAO(%s,%s)\n", file_content, cmd_array[0], cmd_array[1]);
		strcpy(file_content, tmp_content);
	/* wait time */
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
	CTRL_STATE *state = NULL;
	if (robot_type == 1) {
		state = &ctrl_state;
	} else {
		state = &vir_ctrl_state;
	}
	printf("state->toolNum = %d\n", state->toolNum);

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
	cJSON *tcf = cJSON_GetObjectItem(data_json, "tcf");
	if (tcf == NULL || tcf->type != cJSON_Object) {
		perror("json");

		return FAIL;
	}
	cJSON *x = cJSON_GetObjectItem(tcf, "x");
	cJSON *y = cJSON_GetObjectItem(tcf, "y");
	cJSON *z = cJSON_GetObjectItem(tcf, "z");
	cJSON *rx = cJSON_GetObjectItem(tcf, "rx");
	cJSON *ry = cJSON_GetObjectItem(tcf, "ry");
	cJSON *rz = cJSON_GetObjectItem(tcf, "rz");
	if(x->valuestring == NULL || y->valuestring == NULL || z->valuestring == NULL || rx->valuestring == NULL || ry->valuestring == NULL || rz->valuestring == NULL) {
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
	cJSON *ovl = cJSON_GetObjectItem(data_json, "ovl");
	if (ovl->valuestring == NULL) {
		perror("json");

		return FAIL;
	}
	sprintf(content, "MoveJ(%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s)", j1->valuestring, j2->valuestring, j3->valuestring, j4->valuestring, j5->valuestring, j6->valuestring, x->valuestring, y->valuestring, z->valuestring, rx->valuestring, ry->valuestring, rz->valuestring, state->toolNum, speed->valuestring, acc->valuestring, ovl->valuestring);

	return SUCCESS;
}

/* 230 set_state_id */
static int set_state_id(const cJSON *data_json, char *content)
{
	int icount = 0;
	int i;
	cJSON *id_num = NULL;
	cJSON *id = cJSON_GetObjectItem(data_json, "id");
	if (id == NULL) {
		perror("json");

		return FAIL;
	}
	printf("id = %s\n",cJSON_Print(id));
	state_fb.icount = cJSON_GetArraySize(id); /*获取数组长度*/
	printf("state_fb.iCount= %d\n",state_fb.icount);

	/* empty state_fb id */
	for (i = 0; i < 10; i++) {
		state_fb.id[i] = 0;
	}
	for (i = 0; i < state_fb.icount; i++) {
		id_num = cJSON_GetArrayItem(id, i);  /* 目前按1笔处理, 取出一笔放入 state_fb.id */
		printf("string, state_fb.id[%d] = %s\n", i, id_num->valuestring);
		state_fb.id[i] = atoi(id_num->valuestring);
		printf("array , state_fb.id[%d] = %d\n", i, state_fb.id[i]);
	}
	sprintf(content, "SetCTLStateQueryParam(%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d)", state_fb.icount, state_fb.id[0], state_fb.id[1], state_fb.id[2], state_fb.id[3], state_fb.id[4], state_fb.id[5], state_fb.id[6], state_fb.id[7], state_fb.id[8], state_fb.id[9]);

	return SUCCESS;
}

/* 231 set_state */
static int set_state(const cJSON *data_json, char *content)
{
	cJSON *flag = cJSON_GetObjectItem(data_json, "flag");
	if(flag == NULL || flag->valuestring == NULL) {
		perror("json");

		return FAIL;
	}
	/** clear state quene */
	fb_clearquene(&fb_quene);
	state_fb.cur_state = !(atoi(flag->valuestring));
	sprintf(content, "SetCTLStateQuery(%s)", flag->valuestring);

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

/* 320 jointtotcp */
static int jointtotcp(const cJSON *data_json, char *content)
{
	cJSON *j1 = cJSON_GetObjectItem(data_json, "j1");
	cJSON *j2 = cJSON_GetObjectItem(data_json, "j2");
	cJSON *j3 = cJSON_GetObjectItem(data_json, "j3");
	cJSON *j4 = cJSON_GetObjectItem(data_json, "j4");
	cJSON *j5 = cJSON_GetObjectItem(data_json, "j5");
	cJSON *j6 = cJSON_GetObjectItem(data_json, "j6");
	if(j1 == NULL || j2 == NULL || j3 == NULL || j4 == NULL || j5 == NULL || j6 == NULL || j1->valuestring == NULL || j2->valuestring == NULL || j3->valuestring == NULL || j4->valuestring == NULL || j5->valuestring == NULL || j6->valuestring == NULL) {
		perror("json");

		return FAIL;
	}
	sprintf(content, "JointToTCP(%s,%s,%s,%s,%s,%s)", j1->valuestring, j2->valuestring, j3->valuestring, j4->valuestring, j5->valuestring, j6->valuestring);

	return SUCCESS;
}

/* 321 setvirtualrobotinitpos */
/*static int setvirtualrobotinitpos(const cJSON *data_json, char *content)
{
	cJSON *j1 = cJSON_GetObjectItem(data_json, "j1");
	cJSON *j2 = cJSON_GetObjectItem(data_json, "j2");
	cJSON *j3 = cJSON_GetObjectItem(data_json, "j3");
	cJSON *j4 = cJSON_GetObjectItem(data_json, "j4");
	cJSON *j5 = cJSON_GetObjectItem(data_json, "j5");
	cJSON *j6 = cJSON_GetObjectItem(data_json, "j6");
	if(j1 == NULL || j2 == NULL || j3 == NULL || j4 == NULL || j5 == NULL || j6 == NULL || j1->valuedouble == NULL || j2->valuedouble == NULL || j3->valuedouble == NULL || j4->valuedouble == NULL || j5->valuedouble == NULL || j6->valuedouble == NULL) {
		perror("json");

		return FAIL;
	}
	sprintf(content, "SetVirtualRobotInitPos(%lf,%lf,%lf,%lf,%lf,%lf)", j1->valuedouble, j2->valuedouble, j3->valuedouble, j4->valuedouble, j5->valuedouble, j6->valuedouble);
	printf("content = %s\n", content);

	return SUCCESS;
}*/

static int enquene_result_dequene(SOCKET_INFO *sock, const int type, pthread_mutex_t *mute, char *send_content, char *recv_content)
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

	ret = quene_recv_result(node, sock->quene, recv_content);

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
	char recv_content[100] = {0};
	char recv_array[6][10] = {{0}};
	cJSON *recv_json = NULL;

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
		case 230:
			ret = set_state_id(data_json, content);
			break;
		case 231:
			ret = set_state(data_json, content);
			break;
		case 303:
			ret = mode(data_json, content);
			break;
		case 320:
			ret = jointtotcp(data_json, content);
			break;
	/*	case 321:
			ret = setvirtualrobotinitpos(data_json, content);
			break;*/
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

	//printf("port = %d\n", port);
	switch (port) {
		/* send cmd to 8080 port */
		case CMD_PORT:
			ret = enquene_result_dequene(&socket_cmd, cmd, &socket_cmd.mute, content, recv_content);
			break;
		/* send file cmd to 8082 port */
		case FILE_PORT:
			ret = enquene_result_dequene(&socket_file, cmd, &socket_file.mute, content, recv_content);
			break;
		/* send cmd to 8070 port */
		case VIR_CMD_PORT:
			ret = enquene_result_dequene(&socket_vir_cmd, cmd, &socket_vir_cmd.mute, content, recv_content);
			break;
		/* send file cmd to 8072 port */
		case VIR_FILE_PORT:
			ret = enquene_result_dequene(&socket_vir_file, cmd, &socket_vir_file.mute, content, recv_content);
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
	if(strlen(recv_content) == 0) {
		websWrite(wp, "success");
	} else {
		/* 320 jointtotcp */
		if (separate_string_to_array(recv_content, ",", 6, 10, (char *)&recv_array) != 6) {
			perror("separate recv");
			goto end;
		}
		recv_json = cJSON_CreateObject();
		cJSON_AddStringToObject(recv_json, "x", recv_array[0]);
		cJSON_AddStringToObject(recv_json, "y", recv_array[1]);
		cJSON_AddStringToObject(recv_json, "z", recv_array[2]);
		cJSON_AddStringToObject(recv_json, "rx", recv_array[3]);
		cJSON_AddStringToObject(recv_json, "ry", recv_array[4]);
		cJSON_AddStringToObject(recv_json, "rz", recv_array[5]);
		websWrite(wp, cJSON_Print(recv_json));
	}
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
