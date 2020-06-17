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
extern ACCOUNT_INFO cur_account;
//extern pthread_cond_t cond_cmd;
//extern pthread_cond_t cond_file;

/********************************* Function declaration ***********************/

static int copy_content(const cJSON *data_json, char *content);
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
static int jointtotcf(const cJSON *data_json, char *content);
static int enquene_result_dequene(SOCKET_INFO *sock, const int type, char *send_content, const int cmd_type);
static int get_lua_content_size(const cJSON *data_json);

/*********************************** Code *************************************/

/* copy json data to content */
static int copy_content(const cJSON *data_json, char *content)
{
	cJSON *data = cJSON_GetObjectItem(data_json, "content");
	if(data == NULL || data->valuestring == NULL) {
		perror("json");

		return FAIL;
	}
	sprintf(content, "%s", data->valuestring);

	return SUCCESS;
}

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
	if(name == NULL || name->valuestring == NULL) {
		perror("json");

		return FAIL;
	}
	sprintf(content, "%s%s", DIR_FRUSER, name->valuestring);
	//printf("content = %s\n", content);

	return SUCCESS;
}

/* parse cmd of lua file */
static int parse_lua_cmd(char *lua_cmd, int len, char *file_content)
{
	//printf("lua cmd = %s\n", lua_cmd);
	char tmp_content[len];
	char sql[1024] = {0};
	char cmd_array[10][20] = {{0}};
	memset(tmp_content, 0, len);

	cJSON *f_json = NULL;
	cJSON *id = NULL;
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
	cJSON *ex = NULL;
	cJSON *ey = NULL;
	cJSON *ez = NULL;
	cJSON *erx = NULL;
	cJSON *ery = NULL;
	cJSON *erz = NULL;
	cJSON *tx = NULL;
	cJSON *ty = NULL;
	cJSON *tz = NULL;
	cJSON *trx = NULL;
	cJSON *try = NULL;
	cJSON *trz = NULL;
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
	cJSON *cd = NULL;
	cJSON *et_cd = NULL;
	cJSON *ptp = NULL;
	cJSON *lin = NULL;
	cJSON *point_1 = NULL;
	cJSON *point_2 = NULL;

	/* PTP */
	if(!strncmp(lua_cmd, "PTP:", 4)) {
		strrpc(lua_cmd, "PTP:", "");
		if(separate_string_to_array(lua_cmd, ",", 2, 20, (char *)&cmd_array) != 2) {
			perror("separate recv");

			return FAIL;
		}
		/*
		 printf("cmd_array[0] = %s", cmd_array[0]);
		 printf("cmd_array[1] = %s", cmd_array[1]);
		 */
		/* open and get point.db content */
		memset(sql, 0, sizeof(sql));
		sprintf(sql, "select * from points;");
		if(select_info_json_sqlite3(DB_POINTS, sql, &f_json) == -1) {
			perror("select ptp points");

			return FAIL;
		}

		ptp = cJSON_GetObjectItem(f_json, cmd_array[0]);
		if(ptp == NULL || ptp->type != cJSON_Object) {

			goto end;
		}

		j1 = cJSON_GetObjectItem(ptp, "j1");
		j2 = cJSON_GetObjectItem(ptp, "j2");
		j3 = cJSON_GetObjectItem(ptp, "j3");
		j4 = cJSON_GetObjectItem(ptp, "j4");
		j5 = cJSON_GetObjectItem(ptp, "j5");
		j6 = cJSON_GetObjectItem(ptp, "j6");
		x = cJSON_GetObjectItem(ptp, "x");
		y = cJSON_GetObjectItem(ptp, "y");
		z = cJSON_GetObjectItem(ptp, "z");
		rx = cJSON_GetObjectItem(ptp, "rx");
		ry = cJSON_GetObjectItem(ptp, "ry");
		rz = cJSON_GetObjectItem(ptp, "rz");
		toolnum = cJSON_GetObjectItem(ptp, "toolnum");
		speed = cJSON_GetObjectItem(ptp, "speed");
		acc = cJSON_GetObjectItem(ptp, "acc");

		if(j1->valuestring == NULL || j2->valuestring == NULL || j3->valuestring == NULL || j4->valuestring == NULL || j5->valuestring == NULL || j6->valuestring == NULL || x->valuestring == NULL || y->valuestring == NULL || z->valuestring == NULL || rx->valuestring == NULL || ry->valuestring == NULL || rz->valuestring == NULL	|| toolnum->valuestring == NULL || speed->valuestring == NULL || acc->valuestring == NULL || cmd_array[1] == NULL) {

			goto end;
		}

		sprintf(tmp_content,"%sMoveJ(%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s)\n", file_content, j1->valuestring, j2->valuestring, j3->valuestring, j4->valuestring, j5->valuestring,j6->valuestring, x->valuestring, y->valuestring, z->valuestring, rx->valuestring, ry->valuestring, rz->valuestring, toolnum->valuestring, speed->valuestring, acc->valuestring, cmd_array[1]);
		strcpy(file_content, tmp_content);
	/* ARC */
	} else if(!strncmp(lua_cmd, "ARC:", 4)) {
		strrpc(lua_cmd, "ARC:", "");
		if(separate_string_to_array(lua_cmd, ",", 3, 20, (char *)&cmd_array) != 3) {
			perror("separate recv");

			return FAIL;
		}

		/* open and get point.db content */
		memset(sql, 0, sizeof(sql));
		sprintf(sql, "select * from points;");
		if (select_info_json_sqlite3(DB_POINTS, sql, &f_json) == -1) {
			perror("select arc1 points");

			return FAIL;
		}
		point_1 = cJSON_GetObjectItem(f_json, cmd_array[0]);
		if(point_1 == NULL || point_1->type != cJSON_Object) {

			goto end;
		}

		j1 = cJSON_GetObjectItem(point_1, "j1");
		j2 = cJSON_GetObjectItem(point_1, "j2");
		j3 = cJSON_GetObjectItem(point_1, "j3");
		j4 = cJSON_GetObjectItem(point_1, "j4");
		j5 = cJSON_GetObjectItem(point_1, "j5");
		j6 = cJSON_GetObjectItem(point_1, "j6");
		x = cJSON_GetObjectItem(point_1, "x");
		y = cJSON_GetObjectItem(point_1, "y");
		z = cJSON_GetObjectItem(point_1, "z");
		rx = cJSON_GetObjectItem(point_1, "rx");
		ry = cJSON_GetObjectItem(point_1, "ry");
		rz = cJSON_GetObjectItem(point_1, "rz");
		toolnum = cJSON_GetObjectItem(point_1, "toolnum");
		speed = cJSON_GetObjectItem(point_1, "speed");
		acc = cJSON_GetObjectItem(point_1, "acc");
		if(j1->valuestring == NULL || j2->valuestring == NULL || j3->valuestring == NULL || j4->valuestring == NULL || j5->valuestring == NULL || j6->valuestring == NULL|| x->valuestring == NULL || y->valuestring == NULL || z->valuestring == NULL || rx->valuestring == NULL || ry->valuestring == NULL || rz->valuestring == NULL || toolnum->valuestring == NULL|| speed->valuestring == NULL || acc->valuestring == NULL) {

			goto end;
		}

		point_2 = cJSON_GetObjectItem(f_json, cmd_array[1]);
		if (point_2 == NULL || point_2->type != cJSON_Object) {

			goto end;
		}

		j1_2 = cJSON_GetObjectItem(point_2, "j1");
		j2_2 = cJSON_GetObjectItem(point_2, "j2");
		j3_2 = cJSON_GetObjectItem(point_2, "j3");
		j4_2 = cJSON_GetObjectItem(point_2, "j4");
		j5_2 = cJSON_GetObjectItem(point_2, "j5");
		j6_2 = cJSON_GetObjectItem(point_2, "j6");
		x_2 = cJSON_GetObjectItem(point_2, "x");
		y_2 = cJSON_GetObjectItem(point_2, "y");
		z_2 = cJSON_GetObjectItem(point_2, "z");
		rx_2 = cJSON_GetObjectItem(point_2, "rx");
		ry_2 = cJSON_GetObjectItem(point_2, "ry");
		rz_2 = cJSON_GetObjectItem(point_2, "rz");
		toolnum_2 = cJSON_GetObjectItem(point_2, "toolnum");
		speed_2 = cJSON_GetObjectItem(point_2, "speed");
		acc_2 = cJSON_GetObjectItem(point_2, "acc");

		if(j1_2->valuestring == NULL || j2_2->valuestring == NULL || j3_2->valuestring == NULL || j4_2->valuestring == NULL || j5_2->valuestring == NULL || j6_2->valuestring == NULL || x_2->valuestring == NULL || y_2->valuestring == NULL || z_2->valuestring == NULL || rx_2->valuestring == NULL || ry_2->valuestring == NULL || rz_2->valuestring == NULL || toolnum_2->valuestring == NULL || speed_2->valuestring == NULL || acc_2->valuestring == NULL || cmd_array[2] == NULL) {

			goto end;
		}

		sprintf(tmp_content, "%sMoveC(%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s)\n", file_content, j1->valuestring, j2->valuestring, j3->valuestring, j4->valuestring, j5->valuestring, j6->valuestring, x->valuestring, y->valuestring, z->valuestring, rx->valuestring, ry->valuestring, rz->valuestring, toolnum->valuestring, speed->valuestring, acc->valuestring, j1_2->valuestring, j2_2->valuestring, j3_2->valuestring, j4_2->valuestring, j5_2->valuestring, j6_2->valuestring, x_2->valuestring, y_2->valuestring, z_2->valuestring, rx_2->valuestring, ry_2->valuestring, rz_2->valuestring, toolnum_2->valuestring, speed_2->valuestring, acc_2->valuestring, cmd_array[2]);
		strcpy(file_content, tmp_content);
	/* Lin */
	} else if(!strncmp(lua_cmd, "Lin:", 4)) {
		strrpc(lua_cmd, "Lin:", "");
		if(separate_string_to_array(lua_cmd, ",", 3, 20, (char *)&cmd_array) != 3) {
			perror("separate recv");

			return FAIL;
		}

		/* open and get point.db content */
		memset(sql, 0, sizeof(sql));
		sprintf(sql, "select * from points where name = \'%s\';", cmd_array[0]);
		if (select_info_json_sqlite3(DB_POINTS, sql, &f_json) == -1) {
			perror("select lin points");

			return FAIL;
		}

		lin = cJSON_GetObjectItem(f_json, cmd_array[0]);
		if(lin == NULL || lin->type != cJSON_Object) {

			goto end;
		}
		if (strcmp(cmd_array[0], "seamPos") == 0) {
			speed = cJSON_GetObjectItem(lin, "speed");
			acc = cJSON_GetObjectItem(lin, "acc");
			toolnum = cJSON_GetObjectItem(lin, "toolnum");
			if(toolnum->valuestring == NULL || speed->valuestring == NULL || acc->valuestring == NULL || cmd_array[1] == NULL || cmd_array[2] == NULL) {

				goto end;
			}
			sprintf(tmp_content, "%sMoveL(\"%s\",%s,%s,%s,%s,%s)\n", file_content, cmd_array[0], toolnum->valuestring, speed->valuestring, acc->valuestring, cmd_array[1], cmd_array[2]);
		} else {
			j1 = cJSON_GetObjectItem(lin, "j1");
			j2 = cJSON_GetObjectItem(lin, "j2");
			j3 = cJSON_GetObjectItem(lin, "j3");
			j4 = cJSON_GetObjectItem(lin, "j4");
			j5 = cJSON_GetObjectItem(lin, "j5");
			j6 = cJSON_GetObjectItem(lin, "j6");
			x = cJSON_GetObjectItem(lin, "x");
			y = cJSON_GetObjectItem(lin, "y");
			z = cJSON_GetObjectItem(lin, "z");
			rx = cJSON_GetObjectItem(lin, "rx");
			ry = cJSON_GetObjectItem(lin, "ry");
			rz = cJSON_GetObjectItem(lin, "rz");
			speed = cJSON_GetObjectItem(lin, "speed");
			acc = cJSON_GetObjectItem(lin, "acc");
			toolnum = cJSON_GetObjectItem(lin, "toolnum");
			if(j1->valuestring == NULL || j2->valuestring == NULL || j3->valuestring == NULL || j4->valuestring == NULL || j5->valuestring == NULL || j6->valuestring == NULL || x->valuestring == NULL || y->valuestring == NULL || z->valuestring == NULL || rx->valuestring == NULL || ry->valuestring == NULL || rz->valuestring == NULL || toolnum->valuestring == NULL || speed->valuestring == NULL || acc->valuestring == NULL || cmd_array[1] == NULL || cmd_array[2] == NULL) {

				goto end;
			}
			sprintf(tmp_content, "%sMoveL(%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s)\n", file_content, j1->valuestring, j2->valuestring, j3->valuestring, j4->valuestring, j5->valuestring, j6->valuestring, x->valuestring, y->valuestring, z->valuestring, rx->valuestring, ry->valuestring, rz->valuestring, toolnum->valuestring, speed->valuestring, acc->valuestring, cmd_array[1], cmd_array[2]);
		}
		strcpy(file_content, tmp_content);
	/* set DO */
	} else if(!strncmp(lua_cmd, "SetDO:", 6)) {
		strrpc(lua_cmd, "SetDO:", "");
		if(separate_string_to_array(lua_cmd, ",", 3, 20, (char *)&cmd_array) != 3) {
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
	/* set ToolDO */
	} else if(!strncmp(lua_cmd, "SetToolDO:", 10)) {
		strrpc(lua_cmd, "SetToolDO:", "");
		if(separate_string_to_array(lua_cmd, ",", 3, 20, (char *)&cmd_array) != 3) {
			perror("separate recv");

			return FAIL;
		}
		/*
		 printf("cmd_array[0] = %s", cmd_array[0]);
		 printf("cmd_array[1] = %s", cmd_array[1]);
		 printf("cmd_array[2] = %s", cmd_array[2]);
		 */
		sprintf(tmp_content, "%sSetToolDO(%s,%s,%s)\n", file_content, cmd_array[0], cmd_array[1], cmd_array[2]);
		strcpy(file_content, tmp_content);
	/* get DI */
	} else if(!strncmp(lua_cmd, "GetDI:", 6)) {
		strrpc(lua_cmd, "GetDI:", "");
		if(separate_string_to_array(lua_cmd, ",", 1, 20, (char *)&cmd_array) != 1) {
			perror("separate recv");

			return FAIL;
		}
		/*
		 printf("cmd_array[0] = %s", cmd_array[0]);
		 */
		sprintf(tmp_content, "%sGetDI(%s)\n", file_content, cmd_array[0]);
		strcpy(file_content, tmp_content);
	/* get ToolDI */
	} else if (!strncmp(lua_cmd, "GetToolDI:", 10)) {
		strrpc(lua_cmd, "GetToolDI:", "");
		if(separate_string_to_array(lua_cmd, ",", 1, 20, (char *)&cmd_array) != 1) {
			perror("separate recv");

			return FAIL;
		}
		/*
		 printf("cmd_array[0] = %s", cmd_array[0]);
		 */
		sprintf(tmp_content, "%sGetToolDI(%s)\n", file_content, cmd_array[0]);
		strcpy(file_content, tmp_content);
	/* set AO */
	} else if(!strncmp(lua_cmd, "SetAO:", 6)) {
		strrpc(lua_cmd, "SetAO:", "");
		if(separate_string_to_array(lua_cmd, ",", 2, 20, (char *)&cmd_array) != 2) {
			perror("separate recv");

			return FAIL;
		}
		/*
		 printf("cmd_array[0] = %s", cmd_array[0]);
		 printf("cmd_array[1] = %s", cmd_array[1]);
		 */
		sprintf(tmp_content, "%sSetAO(%s,%.2f)\n", file_content, cmd_array[0], (float)(atoi(cmd_array[1])*40.95));
		strcpy(file_content, tmp_content);
	/* set ToolAO */
	} else if(!strncmp(lua_cmd, "SetToolAO:", 10)) {
		strrpc(lua_cmd, "SetToolAO:", "");
		if (separate_string_to_array(lua_cmd, ",", 2, 20, (char *)&cmd_array) != 2) {
			perror("separate recv");

			return FAIL;
		}
		/*
		 printf("cmd_array[0] = %s", cmd_array[0]);
		 printf("cmd_array[1] = %s", cmd_array[1]);
		 */
		sprintf(tmp_content, "%sSetToolAO(%s,%.2f)\n", file_content, cmd_array[0], (float)(atoi(cmd_array[1])*40.95));
		strcpy(file_content, tmp_content);
	/* get AI */
	} else if(!strncmp(lua_cmd, "GetAI:", 6)) {
		strrpc(lua_cmd, "GetAI:", "");
		if(separate_string_to_array(lua_cmd, ",", 1, 20, (char *)&cmd_array) != 1) {
			perror("separate recv");

			return FAIL;
		}
		/*
		 printf("cmd_array[0] = %s", cmd_array[0]);
		 */
		sprintf(tmp_content, "%sGetAI(%s)\n", file_content, cmd_array[0]);
		strcpy(file_content, tmp_content);
	/* get ToolAI */
	} else if(!strncmp(lua_cmd, "GetToolAI:", 10)) {
		strrpc(lua_cmd, "GetToolAI:", "");
		if(separate_string_to_array(lua_cmd, ",", 1, 20, (char *)&cmd_array) != 1) {
			perror("separate recv");

			return FAIL;
		}
		/*
		 printf("cmd_array[0] = %s", cmd_array[0]);
		 */
		sprintf(tmp_content, "%sGetToolAI(%s)\n", file_content, cmd_array[0]);
		strcpy(file_content, tmp_content);
	/* Wait Time */
	} else if(!strncmp(lua_cmd, "WaitTime:", 9)) {
		strrpc(lua_cmd, "WaitTime:", "");
		sprintf(tmp_content, "%sWaitMs(%s)\n", file_content, lua_cmd);
		strcpy(file_content, tmp_content);
	/* WaitDI */
	} else if(!strncmp(lua_cmd, "WaitDI:", 7)) {
		strrpc(lua_cmd, "WaitDI:", "");
		if (separate_string_to_array(lua_cmd, ",", 3, 20, (char *)&cmd_array) != 3) {
			perror("separate recv");

			return FAIL;
		}
		/*
		 printf("cmd_array[0] = %s", cmd_array[0]);
		 */
		sprintf(tmp_content, "%sWaitDI(%s,%s,%s)\n", file_content, cmd_array[0], cmd_array[1], cmd_array[2]);
		strcpy(file_content, tmp_content);
	/* WaitToolDI */
	} else if(!strncmp(lua_cmd, "WaitToolDI:", 11)) {
		strrpc(lua_cmd, "WaitToolDI:", "");
		if(separate_string_to_array(lua_cmd, ",", 3, 20, (char *)&cmd_array) != 3) {
			perror("separate recv");

			return FAIL;
		}
		/*
		 printf("cmd_array[0] = %s", cmd_array[0]);
		 */
		sprintf(tmp_content, "%sWaitToolDI(%s,%s,%s)\n", file_content, cmd_array[0], cmd_array[1], cmd_array[2]);
		strcpy(file_content, tmp_content);
	/* WaitAI */
	} else if(!strncmp(lua_cmd, "WaitAI:", 7)) {
		strrpc(lua_cmd, "WaitAI:", "");
		if(separate_string_to_array(lua_cmd, ",", 4, 20, (char *)&cmd_array) != 4) {
			perror("separate recv");

			return FAIL;
		}
		/*
		 printf("cmd_array[0] = %s", cmd_array[0]);
		 */
		sprintf(tmp_content, "%sWaitAI(%s,%s,%s,%s)\n", file_content, cmd_array[0], cmd_array[1], cmd_array[2], cmd_array[3]);
		strcpy(file_content, tmp_content);
	/* WaitToolAI */
	} else if(!strncmp(lua_cmd, "WaitToolAI:", 11)) {
		strrpc(lua_cmd, "WaitToolAI:", "");
		if(separate_string_to_array(lua_cmd, ",", 4, 20, (char *)&cmd_array) != 4) {
			perror("separate recv");

			return FAIL;
		}
		/*
		 printf("cmd_array[0] = %s", cmd_array[0]);
		 */
		sprintf(tmp_content, "%sWaitToolAI(%s,%s,%s,%s)\n", file_content, cmd_array[0], cmd_array[1], cmd_array[2], cmd_array[3]);
		strcpy(file_content, tmp_content);
	/* set MoveTPD */
	} else if(!strncmp(lua_cmd, "MoveTPD:", 8)) {
		//	printf("enter moveTPD\n");
		strrpc(lua_cmd, "MoveTPD:", "");
		if(separate_string_to_array(lua_cmd, ",", 3, 20, (char *)&cmd_array) != 3) {
			perror("separate recv");

			return FAIL;
		}
		/*
		 printf("cmd_array[0] = %s", cmd_array[0]);
		 printf("cmd_array[1] = %s", cmd_array[1]);
		 printf("cmd_array[2] = %s", cmd_array[2]);
		 */
		sprintf(tmp_content, "%sMoveTPD(%s,%s,%s)\n", file_content, cmd_array[0], cmd_array[1], cmd_array[2]);
		strcpy(file_content, tmp_content);
		//	printf("finish moveTPD\n");
	/* set MoveGripper */
	} else if(!strncmp(lua_cmd, "MoveGripper:", 12)) {
		strrpc(lua_cmd, "MoveGripper:", "");
		if(separate_string_to_array(lua_cmd, ",", 5, 20, (char *)&cmd_array) != 5) {
			perror("separate recv");

			return FAIL;
		}
		/*
		 printf("cmd_array[0] = %s", cmd_array[0]);
		 printf("cmd_array[1] = %s", cmd_array[1]);
		 printf("cmd_array[2] = %s", cmd_array[2]);
		 printf("cmd_array[2] = %s", cmd_array[3]);
		 */
		sprintf(tmp_content, "%sMoveGripper(%s,%s,%s,%s,%s)\n", file_content, cmd_array[0], cmd_array[1], cmd_array[2], cmd_array[3], cmd_array[4]);
		strcpy(file_content, tmp_content);
	/* SetToolCoord */
	} else if(!strncmp(lua_cmd, "SetToolList:", 12)) {
		strrpc(lua_cmd, "SetToolList:", "");
		if(separate_string_to_array(lua_cmd, ",", 1, 20, (char *)&cmd_array) != 1) {
			perror("separate recv");

			return FAIL;
		}

		memset(sql, 0, sizeof(sql));
		sprintf(sql, "select * from coordinate_system where name = \'%s\';", cmd_array[0]);
		if(select_info_json_sqlite3(DB_CDSYSTEM, sql, &f_json) == -1) {
			perror("select cdsystem");

			return FAIL;
		}

		cd = cJSON_GetObjectItem(f_json, cmd_array[0]);
		if(cd == NULL || cd->type != cJSON_Object) {

			goto end;
		}

		id = cJSON_GetObjectItem(cd, "id");
		x = cJSON_GetObjectItem(cd, "x");
		y = cJSON_GetObjectItem(cd, "y");
		z = cJSON_GetObjectItem(cd, "z");
		rx = cJSON_GetObjectItem(cd, "rx");
		ry = cJSON_GetObjectItem(cd, "ry");
		rz = cJSON_GetObjectItem(cd, "rz");
		if(id->valuestring == NULL || x->valuestring == NULL || y->valuestring == NULL || z->valuestring == NULL || rx->valuestring == NULL || ry->valuestring == NULL || rz->valuestring == NULL) {

			goto end;
		}
		sprintf(tmp_content, "%sSetToolList(%s,%s,%s,%s,%s,%s,%s)\n", file_content, id->valuestring, x->valuestring, y->valuestring, z->valuestring, rx->valuestring, ry->valuestring, rz->valuestring);
		strcpy(file_content, tmp_content);
	/* SetExToolCoord */
	} else if(!strncmp(lua_cmd, "SetExToolList:", 14)) {
		strrpc(lua_cmd, "SetExToolList:", "");
		if(separate_string_to_array(lua_cmd, ",", 1, 20, (char *)&cmd_array) != 1) {
			perror("separate recv");

			return FAIL;
		}
		memset(sql, 0, sizeof(sql));
		sprintf(sql, "select * from et_coordinate_system where name = \'%s\';", cmd_array[0]);
		if(select_info_json_sqlite3(DB_ET_CDSYSTEM, sql, &f_json) == -1) {
			perror("select cdsystem");

			return FAIL;
		}
		et_cd = cJSON_GetObjectItem(f_json, cmd_array[0]);
		if(et_cd == NULL || et_cd->type != cJSON_Object) {

			goto end;
		}
		id = cJSON_GetObjectItem(et_cd, "id");
		ex = cJSON_GetObjectItem(et_cd, "ex");
		ey = cJSON_GetObjectItem(et_cd, "ey");
		ez = cJSON_GetObjectItem(et_cd, "ez");
		erx = cJSON_GetObjectItem(et_cd, "erx");
		ery = cJSON_GetObjectItem(et_cd, "ery");
		erz = cJSON_GetObjectItem(et_cd, "erz");
		tx = cJSON_GetObjectItem(et_cd, "tx");
		ty = cJSON_GetObjectItem(et_cd, "ty");
		tz = cJSON_GetObjectItem(et_cd, "tz");
		trx = cJSON_GetObjectItem(et_cd, "trx");
		try = cJSON_GetObjectItem(et_cd, "try");
		trz = cJSON_GetObjectItem(et_cd, "trz");
		if(id->valuestring == NULL || ex->valuestring == NULL || ey->valuestring == NULL || ez->valuestring == NULL || erx->valuestring == NULL || ery->valuestring == NULL || erz->valuestring == NULL || tx->valuestring == NULL || ty->valuestring == NULL || tz->valuestring == NULL || trx->valuestring == NULL || try->valuestring == NULL || trz->valuestring == NULL) {

			goto end;
		}
		sprintf(tmp_content, "%sSetExToolList(%d,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s)\n", file_content, (atoi(id->valuestring) + 14), ex->valuestring, ey->valuestring, ez->valuestring, erx->valuestring, ery->valuestring, erz->valuestring, tx->valuestring, ty->valuestring, tz->valuestring, trx->valuestring, try->valuestring, trz->valuestring);
		strcpy(file_content, tmp_content);
	/* WeaveStart */
	} else if(!strncmp(lua_cmd, "WeaveStart:", 11)) {
		strrpc(lua_cmd, "WeaveStart:", "");
		if(separate_string_to_array(lua_cmd, ",", 1, 20, (char *)&cmd_array) != 1) {
			perror("separate recv");

			return FAIL;
		}
		sprintf(tmp_content, "%sWeaveStart(%s)\n", file_content, cmd_array[0]);
		strcpy(file_content, tmp_content);
	/* WeaveEnd */
	} else if(!strncmp(lua_cmd, "WeaveEnd:", 9)) {
		strrpc(lua_cmd, "WeaveEnd:", "");
		if(separate_string_to_array(lua_cmd, ",", 1, 20, (char *)&cmd_array) != 1) {
			perror("separate recv");

			return FAIL;
		}
		sprintf(tmp_content, "%sWeaveEnd(%s)\n", file_content, cmd_array[0]);
		strcpy(file_content, tmp_content);
	/* ARCStart */
	} else if(!strncmp(lua_cmd, "ARCStart:", 9)) {
		strrpc(lua_cmd, "ARCStart:", "");
		if (separate_string_to_array(lua_cmd, ",", 2, 20, (char *)&cmd_array) != 2) {
			perror("separate recv");

			return FAIL;
		}
		sprintf(tmp_content, "%sARCStart(%s,%s)\n", file_content, cmd_array[0], cmd_array[1]);
		strcpy(file_content, tmp_content);
	/* ARCEnd */
	} else if(!strncmp(lua_cmd, "ARCEnd:", 7)) {
		strrpc(lua_cmd, "ARCEnd:", "");
		if (separate_string_to_array(lua_cmd, ",", 2, 20, (char *)&cmd_array) != 2) {
			perror("separate recv");

			return FAIL;
		}
		sprintf(tmp_content, "%sARCEnd(%s,%s)\n", file_content, cmd_array[0], cmd_array[1]);
		strcpy(file_content, tmp_content);
	/* LTLaserOn */
	} else if(!strncmp(lua_cmd, "LTLaserOn:", 10)) {
		strrpc(lua_cmd, "LTLaserOn:", "");
		if(separate_string_to_array(lua_cmd, ",", 1, 20, (char *)&cmd_array) != 1) {
			perror("separate recv");

			return FAIL;
		}
		sprintf(tmp_content, "%sLTLaserOn(%s)\n", file_content, cmd_array[0]);
		strcpy(file_content, tmp_content);
	/* LTSearchStart */
	} else if(!strncmp(lua_cmd, "LTSearchStart:", 14)) {
		strrpc(lua_cmd, "LTSearchStart:", "");
		if (separate_string_to_array(lua_cmd, ",", 4, 20, (char *)&cmd_array) != 4) {
			perror("separate recv");

			return FAIL;
		}
		sprintf(tmp_content, "%sLTSearchStart(%s,%s,%s,%s)\n", file_content, cmd_array[0], cmd_array[1], cmd_array[2], cmd_array[3]);
		strcpy(file_content, tmp_content);
	/* other code send without processing */
	} else {
		sprintf(tmp_content, "%s%s\n", file_content, lua_cmd);
		strcpy(file_content, tmp_content);
	}
	//printf("file_content = %s\n", file_content);
	cJSON_Delete(f_json);
	f_json = NULL;

	return SUCCESS;

end:
	cJSON_Delete(f_json);
	f_json = NULL;
	return FAIL;
}

/* 106 sendFile */
static int sendfile(const cJSON *data_json, int content_len, char *content)
{
	const char s[2] = "\n";
	char *token = NULL;

	cJSON *pgvalue = cJSON_GetObjectItem(data_json, "pgvalue");
	if(pgvalue == NULL || pgvalue->valuestring == NULL || !strcmp(pgvalue->valuestring, "")) {
		perror("json");

		return FAIL;
	}
	/* get first line */
	token = strtok(pgvalue->valuestring, s);
	while (token != NULL) {
		//printf("token = %s\n", token);
		if (parse_lua_cmd(token, content_len, content) == FAIL) {

			return FAIL;
		}
		/* get other line */
		token = strtok(NULL, s);
	}
	printf("content = %s\n", content);

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
	/* ARC */
	} else if (!strncmp(pgvalue->valuestring, "ARC:", 4)) {
		cmd = 202;
	/* Lin */
	} else if (!strncmp(pgvalue->valuestring, "Lin:", 4)) {
		cmd = 203;
	/* set DO */
	} else if (!strncmp(pgvalue->valuestring, "SetDO:", 6)) {
		cmd = 204;
	/* wait time*/
	} else if (!strncmp(pgvalue->valuestring, "WaitTime:", 9)) {
		cmd = 207;
	/* set AO */
	} else if (!strncmp(pgvalue->valuestring, "SetAO:", 6)) {
		cmd = 209;
	/* set ToolDO */
	} else if (!strncmp(pgvalue->valuestring, "SetToolDO:", 10)) {
		cmd = 210;
	/* set ToolAO */
	} else if (!strncmp(pgvalue->valuestring, "SetToolAO:", 10)) {
		cmd = 211;
	/* get DI */
	} else if (!strncmp(pgvalue->valuestring, "GetDI:", 6)) {
		cmd = 212;
	/* get ToolDI */
	} else if (!strncmp(pgvalue->valuestring, "GetToolDI:", 10)) {
		cmd = 213;
	/* get AI */
	} else if (!strncmp(pgvalue->valuestring, "GetAI:", 6)) {
		cmd = 214;
	/* get ToolAI */
	} else if (!strncmp(pgvalue->valuestring, "GetToolAI:", 10)) {
		cmd = 215;
	/* moveTPD */
	} else if (!strncmp(pgvalue->valuestring, "MoveTPD:", 8)) {
		cmd = 217;
	/* waitDI */
	} else if (!strncmp(pgvalue->valuestring, "WaitDI:", 7)) {
		cmd = 218;
	/* waitToolDI */
	} else if (!strncmp(pgvalue->valuestring, "WaitToolDI:", 11)) {
		cmd = 219;
	/* waitAI */
	} else if (!strncmp(pgvalue->valuestring, "WaitAI:", 7)) {
		cmd = 220;
		/* waitToolDI */
	} else if (!strncmp(pgvalue->valuestring, "WaitToolAI:", 11)) {
		cmd = 221;
	/* MoveGripper */
	} else if (!strncmp(pgvalue->valuestring, "MoveGripper:", 12)) {
		cmd = 228;
	/* SprayStart */
	} else if (!strncmp(pgvalue->valuestring, "SprayStart", 10)) {
		cmd = 236;
	/* SprayStop */
	} else if (!strncmp(pgvalue->valuestring, "SprayStop", 9)) {
		cmd = 237;
	/* PowerCleanStart */
	} else if (!strncmp(pgvalue->valuestring, "PowerCleanStart", 15)) {
		cmd = 238;
	/* PowerCleanStop */
	} else if (!strncmp(pgvalue->valuestring, "PowerCleanStop", 14)) {
		cmd = 239;
	/* ARCStart */
	} else if (!strncmp(pgvalue->valuestring, "ARCStart", 8)) {
		cmd = 247;
	/* ARCEnd */
	} else if (!strncmp(pgvalue->valuestring, "ARCEnd", 6)) {
		cmd = 248;
	/* WeaveStart */
	} else if (!strncmp(pgvalue->valuestring, "WeaveStart", 10)) {
		cmd = 253;
	/* WeaveEnd */
	} else if (!strncmp(pgvalue->valuestring, "WeaveEnd", 8)) {
		cmd = 254;
	/* LTLaserOn */
	} else if (!strncmp(pgvalue->valuestring, "LTLaserOn", 9)) {
		cmd = 255;
	/* LTLaserOff */
	} else if (!strncmp(pgvalue->valuestring, "LTLaserOff", 10)) {
		cmd = 256;
	/* LTTrackOn */
	} else if (!strncmp(pgvalue->valuestring, "LTTrackOn", 9)) {
		cmd = 257;
	/* LTTrackOff */
	} else if (!strncmp(pgvalue->valuestring, "LTTrackOff", 10)) {
		cmd = 258;
	/* LTSearchStart */
	} else if (!strncmp(pgvalue->valuestring, "LTSearchStart", 13)) {
		cmd = 259;
	/* LTSearchStop */
	} else if (!strncmp(pgvalue->valuestring, "LTSearchStop", 12)) {
		cmd = 260;
	/* SetToolList */
	} else if (!strncmp(pgvalue->valuestring, "SetToolList:", 12)) {
		cmd = 319;
	/* SetExToolCoord */
	} else if (!strncmp(pgvalue->valuestring, "SetExToolList:", 14)) {
		cmd = 331;
	/* error */
	} else {
		return FAIL;
	}
	if (parse_lua_cmd(pgvalue->valuestring, sizeof(char) * MAX_BUF, content) == FAIL) {
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
	if (j1->valuestring == NULL || j2->valuestring == NULL || j3->valuestring == NULL || j4->valuestring == NULL || j5->valuestring == NULL || j6->valuestring == NULL) {
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
	if (x->valuestring == NULL || y->valuestring == NULL || z->valuestring == NULL || rx->valuestring == NULL || ry->valuestring == NULL || rz->valuestring == NULL) {
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
	sprintf(content, "MoveJ(%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%d,%s,%s,%s)", j1->valuestring, j2->valuestring, j3->valuestring, j4->valuestring, j5->valuestring, j6->valuestring, x->valuestring, y->valuestring, z->valuestring, rx->valuestring, ry->valuestring, rz->valuestring, state->toolNum, speed->valuestring, acc->valuestring, ovl->valuestring);

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
	printf("id = %s\n", cJSON_Print(id));
	state_fb.icount = cJSON_GetArraySize(id); /*获取数组长度*/
	printf("state_fb.iCount= %d\n", state_fb.icount);

	/* empty state_fb id */
	for (i = 0; i < 10; i++) {
		state_fb.id[i] = 0;
	}
	for (i = 0; i < state_fb.icount; i++) {
		id_num = cJSON_GetArrayItem(id, i); /* 目前按1笔处理, 取出一笔放入 state_fb.id */
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

/* 320 jointtotcf */
static int jointtotcf(const cJSON *data_json, char *content)
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
	sprintf(content, "JointToTCF(%s,%s,%s,%s,%s,%s)", j1->valuestring, j2->valuestring, j3->valuestring, j4->valuestring, j5->valuestring, j6->valuestring);

	return SUCCESS;
}

static int enquene_result_dequene(SOCKET_INFO *sock, const int type, char *send_content, const int cmd_type)
{
	//int ret = FAIL;
	/* socket 连接已经断开 */
	if (sock->connect_status == 0) {

		return FAIL;
	}

	/* 创建结点 */
	QElemType node;
	createnode(&node, type, send_content);
	pthread_mutex_lock(&sock->mut);
	if (sock->msghead >= MAX_MSGHEAD) {
		sock->msghead = 1;
	} else {
		sock->msghead++;
	}
	node.msghead = sock->msghead;
	pthread_mutex_unlock(&sock->mut);
	/* 创建结点插入队列中 */
	if (cmd_type == 1) { //非即时指令
		pthread_mutex_lock(&sock->mute);
		enquene(&sock->quene, node);
		pthread_mutex_unlock(&sock->mute);
	} else { //即时指令
		pthread_mutex_lock(&sock->im_mute);
		enquene(&sock->im_quene, node);
		pthread_mutex_unlock(&sock->im_mute);
	}
	//pthread_cond_signal(&cond_cmd);

	//ret = quene_recv_result(node, sock->quene, recv_content);

	/* 把结点从队列中删除 */
	/*
	 pthread_mutex_lock(mute);
	 dequene(&sock->quene, node);
	 pthread_mutex_unlock(mute);
	 */
	// TODO: add signal

	//return ret;
	return SUCCESS;
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

	content_size = line_num * sizeof(char) * MAX_BUF;

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
	int cmdport = 0;
	int fileport = 0;
	int cmd_type = 1;// 1:非即时指令 0:即时指令
	int content_len = sizeof(char) * MAX_BUF;
	//char recv_content[100] = {0};
	char recv_array[6][10] = { { 0 } };
	cJSON *recv_json = NULL;
	cJSON *data_json = NULL;
	cJSON *post_type = NULL;
	cJSON *command = NULL;
	cJSON *port_n = NULL;
	cJSON *data = NULL;

	/** virtual robot */
	if (robot_type == 0) {
		cmdport = VIR_CMD_PORT;
		fileport = VIR_FILE_PORT;
		/** Physical robot */
	} else {
		cmdport = CMD_PORT;
		fileport = FILE_PORT;
	}

	data = cJSON_Parse(wp->input.servp);
	if (data == NULL) {
		perror("json");
		goto end;
	}

	buf = cJSON_Print(data);
	printf("data:%s\n", buf);
	free(buf);
	buf = NULL;

	/* get data json */
	data_json = cJSON_GetObjectItem(data, "data");
	if (data_json == NULL || data_json->type != cJSON_Object) {
		perror("json");
		goto end;
	}

	/* calloc content */
	content = (char *) calloc(1, sizeof(char) * MAX_BUF);
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
	// cmd_auth "1"
	if (cmd == 313 || cmd == 314 || cmd == 230 || cmd == 231 || cmd == 302 || cmd == 312 || cmd == 326 || cmd == 327 || cmd == 328 || cmd == 329 || cmd == 332) {
		if (!authority_management("1")) {
			perror("authority_management");
			goto auth_end;
		}
	// cmd_auth "2"
	} else if (cmd == 320 || cmd == 201 || cmd == 303 || cmd == 101 || cmd == 102 || cmd == 103 || cmd == 104 || cmd == 1001 || cmd == 232 || cmd == 233 || cmd == 208 || cmd == 216 || cmd == 203 || cmd == 204 || cmd == 209 || cmd == 210 || cmd == 211 || cmd == 234 || cmd == 316 || cmd == 308 || cmd == 309 || cmd == 306 || cmd == 307 || cmd == 206 || cmd == 305 || cmd == 321 || cmd == 323 ||cmd == 324 || cmd == 222 || cmd == 223 || cmd == 224 || cmd == 225 || cmd == 105 || cmd == 106 || cmd == 315 || cmd == 317 || cmd == 318 || cmd == 226 || cmd == 229 || cmd == 227 || cmd == 330 || cmd == 235 || cmd == 236 || cmd == 237 || cmd == 238 || cmd == 239 || cmd == 247 || cmd == 248 || cmd == 249 || cmd == 250 || cmd == 251 || cmd == 252 || cmd == 253 || cmd == 254 || cmd == 255 || cmd == 256 || cmd == 257 || cmd == 258 || cmd == 259 || cmd == 260 || cmd == 261 || cmd == 262 || cmd == 263 || cmd == 264 || cmd == 265 || cmd == 266 || cmd==333 || cmd == 334) {
		if (!authority_management("2")) {
			perror("authority_management");
			goto auth_end;
		}
	}
	switch (cmd) {
	case 100:// test
		port = fileport;
		content_len = get_lua_content_size(data_json);
		if (content_len == FAIL) {
			perror("get lua content size");

			goto end;
		}
		// realloc content
		content = (char *) realloc(content, content_len);
		if (content == NULL) {
			perror("realloc");

			goto end;
		}
		memset(content, 0, content_len);
		ret = sendfile(data_json, content_len, content);
		break;
	case 101:
		port = cmdport;
		cmd_type = 0;
		my_syslog("机器人操作", "开始程序示教", cur_account.username);
		ret = program_start(data_json, content);
		break;
	case 102:
		port = cmdport;
		cmd_type = 0;
		my_syslog("机器人操作", "停止程序示教", cur_account.username);
		ret = program_stop(data_json, content);
		break;
	case 103:
		port = cmdport;
		cmd_type = 0;
		my_syslog("机器人操作", "暂停程序示教", cur_account.username);
		ret = program_pause(data_json, content);
		break;
	case 104:
		port = cmdport;
		cmd_type = 0;
		my_syslog("机器人操作", "恢复程序示教", cur_account.username);
		ret = program_resume(data_json, content);
		break;
	case 105:/* 8082 */
		port = fileport;
		my_syslog("机器人操作", "下发程序示教名称", cur_account.username);
		ret = sendfilename(data_json, content);
		break;
	case 106:/* 8082 */
		port = fileport;
		my_syslog("机器人操作", "下发程序示教文件内容", cur_account.username);
		content_len = get_lua_content_size(data_json);
		//printf("content_len = %d\n", content_len);
		if (content_len == FAIL) {
			perror("get lua content size");

			goto end;
		}
		// realloc content
		content = (char *) realloc(content, content_len);
		if (content == NULL) {
			perror("realloc");

			goto end;
		}
		memset(content, 0, content_len);
		ret = sendfile(data_json, content_len, content);
		break;
	case 201:
		port = cmdport;
		my_syslog("机器人操作", "下发关节数据", cur_account.username);
		ret = movej(data_json, content);
		break;
	case 203:
		port = cmdport;
		my_syslog("机器人操作", "基坐标单轴点动-点按开始", cur_account.username);
		ret = copy_content(data_json, content);
		break;
	case 204:
		port = cmdport;
		my_syslog("机器人操作", "设置控制箱DO", cur_account.username);
		ret = copy_content(data_json, content);
		break;
	case 206:
		port = cmdport;
		my_syslog("机器人操作", "设置速度百分比", cur_account.username);
		ret = copy_content(data_json, content);
		break;
	case 208:
		port = cmdport;
		my_syslog("机器人操作", "关节坐标单轴点动-点按开始", cur_account.username);
		ret = copy_content(data_json, content);
		break;
	case 209:
		port = cmdport;
		my_syslog("机器人操作", "设置控制箱AO", cur_account.username);
		ret = copy_content(data_json, content);
		break;
	case 210:
		port = cmdport;
		my_syslog("机器人操作", "设置末端工具DO", cur_account.username);
		ret = copy_content(data_json, content);
		break;
	case 211:
		port = cmdport;
		my_syslog("机器人操作", "设置末端工具AO", cur_account.username);
		ret = copy_content(data_json, content);
		break;
	case 216:
		port = cmdport;
		cmd_type = 0;
		my_syslog("机器人操作", "关节坐标单轴点动-点按结束", cur_account.username);
		ret = copy_content(data_json, content);
		break;
	case 222:
		port = cmdport;
		my_syslog("机器人操作", "控制箱DI滤波", cur_account.username);
		ret = copy_content(data_json, content);
		break;
	case 223:
		port = cmdport;
		my_syslog("机器人操作", "工具DI滤波", cur_account.username);
		ret = copy_content(data_json, content);
		break;
	case 224:
		port = cmdport;
		my_syslog("机器人操作", "控制箱AI滤波", cur_account.username);
		ret = copy_content(data_json, content);
		break;
	case 225:
		port = cmdport;
		my_syslog("机器人操作", "工具AI0滤波", cur_account.username);
		ret = copy_content(data_json, content);
		break;
	case 226:
		port = cmdport;
		my_syslog("机器人操作", "配置夹爪", cur_account.username);
		ret = copy_content(data_json, content);
		break;
	case 227:
		port = cmdport;
		my_syslog("机器人操作", "激活和复位夹爪", cur_account.username);
		ret = copy_content(data_json, content);
		break;
	case 229:
		port = cmdport;
		my_syslog("机器人操作", "读取夹爪配置信息", cur_account.username);
		ret = copy_content(data_json, content);
		break;
	case 230:
		port = cmdport;
		my_syslog("机器人操作", "设置查询图表id号", cur_account.username);
		ret = set_state_id(data_json, content);
		break;
	case 231:
		port = cmdport;
		my_syslog("机器人操作", "状态查询开始/结束", cur_account.username);
		ret = set_state(data_json, content);
		break;
	case 232:
		port = cmdport;
		my_syslog("机器人操作", "单轴点动-长按开始", cur_account.username);
		ret = copy_content(data_json, content);
		break;
	case 233:
		port = cmdport;
		cmd_type = 0;
		my_syslog("机器人操作", "单轴点动-长按结束", cur_account.username);
		ret = copy_content(data_json, content);
		break;
	case 234:
		port = cmdport;
		cmd_type = 0;
		my_syslog("机器人操作", "基坐标单轴点动-点按结束", cur_account.username);
		ret = copy_content(data_json, content);
		break;
	case 235:
		port = cmdport;
		cmd_type = 0;
		my_syslog("机器人操作", "外部工具坐标单轴点动-长按结束", cur_account.username);
		ret = copy_content(data_json, content);
		break;
	case 236:
		port = cmdport;
		my_syslog("机器人操作", "开始喷涂", cur_account.username);
		ret = copy_content(data_json, content);
		break;
	case 237:
		port = cmdport;
		my_syslog("机器人操作", "停止喷涂", cur_account.username);
		ret = copy_content(data_json, content);
		break;
	case 238:
		port = cmdport;
		my_syslog("机器人操作", "开始清枪", cur_account.username);
		ret = copy_content(data_json, content);
		break;
	case 239:
		port = cmdport;
		my_syslog("机器人操作", "停止清枪", cur_account.username);
		ret = copy_content(data_json, content);
		break;
	case 247:
		port = cmdport;
		my_syslog("机器人操作", "起弧", cur_account.username);
		ret = copy_content(data_json, content);
		break;
	case 248:
		port = cmdport;
		my_syslog("机器人操作", "收弧", cur_account.username);
		ret = copy_content(data_json, content);
		break;
	case 249:
		port = cmdport;
		my_syslog("机器人操作", "设定摆焊坐标系参考点", cur_account.username);
		ret = copy_content(data_json, content);
		break;
	case 250:
		port = cmdport;
		my_syslog("机器人操作", "计算摆焊坐标系", cur_account.username);
		ret = copy_content(data_json, content);
		break;
	case 251:
		port = cmdport;
		my_syslog("机器人操作", "应用摆焊坐标系", cur_account.username);
		ret = copy_content(data_json, content);
		break;
	case 252:
		port = cmdport;
		my_syslog("机器人操作", "摆焊参数设置", cur_account.username);
		ret = copy_content(data_json, content);
		break;
	case 253:
		port = cmdport;
		my_syslog("机器人操作", "开始摆焊", cur_account.username);
		ret = copy_content(data_json, content);
		break;
	case 254:
		port = cmdport;
		my_syslog("机器人操作", "停止摆焊", cur_account.username);
		ret = copy_content(data_json, content);
		break;
	case 255:
		port = cmdport;
		my_syslog("机器人操作", "激光打开", cur_account.username);
		ret = copy_content(data_json, content);
		break;
	case 256:
		port = cmdport;
		my_syslog("机器人操作", "激光关闭", cur_account.username);
		ret = copy_content(data_json, content);
		break;
	case 257:
		port = cmdport;
		my_syslog("机器人操作", "开始跟踪", cur_account.username);
		ret = copy_content(data_json, content);
		break;
	case 258:
		port = cmdport;
		my_syslog("机器人操作", "停止跟踪", cur_account.username);
		ret = copy_content(data_json, content);
		break;
	case 259:
		port = cmdport;
		my_syslog("机器人操作", "寻位开始,设置寻位参数", cur_account.username);
		ret = copy_content(data_json, content);
		break;
	case 260:
		port = cmdport;
		my_syslog("机器人操作", "寻位结束", cur_account.username);
		ret = copy_content(data_json, content);
		break;
	case 261:
		port = cmdport;
		my_syslog("机器人操作", "设定传感器参考点", cur_account.username);
		ret = copy_content(data_json, content);
		break;
	case 262:
		port = cmdport;
		my_syslog("机器人操作", "计算传感器位姿", cur_account.username);
		ret = copy_content(data_json, content);
		break;
	case 263:
		port = cmdport;
		my_syslog("机器人操作", "配置机器人IP", cur_account.username);
		ret = copy_content(data_json, content);
		break;
	case 264:
		port = cmdport;
		my_syslog("机器人操作", "配置激光跟踪传感器IP和端口", cur_account.username);
		ret = copy_content(data_json, content);
		break;
	case 265:
		port = cmdport;
		my_syslog("机器人操作", "加载传感器通信协议", cur_account.username);
		ret = copy_content(data_json, content);
		break;
	case 266:
		port = cmdport;
		my_syslog("机器人操作", "卸载传感器通信协议", cur_account.username);
		ret = copy_content(data_json, content);
		break;
	case 302:
		port = cmdport;
		my_syslog("机器人操作", "机器手急停后电机使能", cur_account.username);
		ret = copy_content(data_json, content);
		break;
	case 303:
		port = cmdport;
		my_syslog("机器人操作", "更改机器人模式", cur_account.username);
		ret = mode(data_json, content);
		break;
	case 305:
		port = cmdport;
		my_syslog("机器人操作", "设置碰撞等级", cur_account.username);
		ret = copy_content(data_json, content);
		break;
	case 306:
		port = cmdport;
		my_syslog("机器人操作", "设置负载重量", cur_account.username);
		ret = copy_content(data_json, content);
		break;
	case 307:
		port = cmdport;
		my_syslog("机器人操作", "设置负载质心", cur_account.username);
		ret = copy_content(data_json, content);
		break;
	case 308:
		port = cmdport;
		my_syslog("机器人操作", "设置机器人正限位角度", cur_account.username);
		ret = copy_content(data_json, content);
		break;
	case 309:
		port = cmdport;
		my_syslog("机器人操作", "设置机器人负限位角度", cur_account.username);
		ret = copy_content(data_json, content);
		break;
	case 312:
		port = cmdport;
		cmd_type = 0;
		my_syslog("机器人操作", "零点设定", cur_account.username);
		ret = copy_content(data_json, content);
		break;
	case 313:
		port = cmdport;
		my_syslog("机器人操作", "新建工具坐标系下发点", cur_account.username);
		ret = copy_content(data_json, content);
		break;
	case 314:
		port = cmdport;
		my_syslog("机器人操作", "计算工具坐标系", cur_account.username);
		ret = copy_content(data_json, content);
		break;
	case 315:
		port = cmdport;
		my_syslog("机器人操作", "开始记录TPD轨迹", cur_account.username);
		ret = copy_content(data_json, content);
		break;
	case 316:
		port = cmdport;
		my_syslog("机器人操作", "应用当前显示的工具坐标系", cur_account.username);
		ret = copy_content(data_json, content);
		break;
	case 317:
		port = cmdport;
		my_syslog("机器人操作", "停止记录TPD轨迹", cur_account.username);
		ret = copy_content(data_json, content);
		break;
	case 318:
		port = cmdport;
		my_syslog("机器人操作", "删除TPD轨迹", cur_account.username);
		ret = copy_content(data_json, content);
		break;
	case 320:
		port = cmdport;
		my_syslog("机器人操作", "计算TCF", cur_account.username);
		ret = jointtotcf(data_json, content);
		break;
	case 321:
		port = cmdport;
		my_syslog("机器人操作", "机器人配置文件生效", cur_account.username);
		ret = copy_content(data_json, content);
		break;
	case 323:
		port = cmdport;
		my_syslog("机器人操作", "设置 DI 配置", cur_account.username);
		ret = copy_content(data_json, content);
		break;
	case 324:
		port = cmdport;
		my_syslog("机器人操作", "设置 DO 配置", cur_account.username);
		ret = copy_content(data_json, content);
		break;
	case 326:
		port = cmdport;
		my_syslog("机器人操作", "设定外部TCP参考点", cur_account.username);
		ret = copy_content(data_json, content);
		break;
	case 327:
		port = cmdport;
		my_syslog("机器人操作", "计算外部TCF", cur_account.username);
		ret = copy_content(data_json, content);
		break;
	case 328:
		port = cmdport;
		my_syslog("机器人操作", "设定外部TCP工具参考点", cur_account.username);
		ret = copy_content(data_json, content);
		break;
	case 329:
		port = cmdport;
		my_syslog("机器人操作", "计算工具TCF", cur_account.username);
		ret = copy_content(data_json, content);
		break;
	case 330:
		port = cmdport;
		my_syslog("机器人操作", "应用当前显示的外部和工具坐标系", cur_account.username);
		ret = copy_content(data_json, content);
		break;
	case 332:
		port = cmdport;
		my_syslog("机器人操作", "进入 boot 模式", cur_account.username);
		ret = copy_content(data_json, content);
		break;
	case 333:
		port = cmdport;
		my_syslog("机器人操作", "切换拖动示教模式", cur_account.username);
		ret = copy_content(data_json, content);
		break;
	case 334:
		port = cmdport;
		my_syslog("机器人操作", "定位完成预值", cur_account.username);
		ret = copy_content(data_json, content);
		break;
	case 400:
		port = cmdport;
		my_syslog("机器人操作", "获取控制器软件版本", cur_account.username);
		ret = copy_content(data_json, content);
		break;
	case 1001:/* 内部定义指令 */
		port = cmdport;
		ret = step_over(data_json, content);
		if (ret == FAIL) {
			perror("step over");

			goto end;
		}
		cmd = ret;
		//printf("cmd = %d\n", cmd);
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

	//printf("port = %d\n", port);
	switch (port) {
	/* send cmd to 8080 port */
	case CMD_PORT:
		ret = enquene_result_dequene(&socket_cmd, cmd, content, cmd_type);
		break;
		/* send file cmd to 8082 port */
	case FILE_PORT:
		ret = enquene_result_dequene(&socket_file, cmd, content, cmd_type);
		break;
		/* send cmd to 8070 port */
	case VIR_CMD_PORT:
		ret = enquene_result_dequene(&socket_vir_cmd, cmd, content, cmd_type);
		break;
		/* send file cmd to 8072 port */
	case VIR_FILE_PORT:
		ret = enquene_result_dequene(&socket_vir_file, cmd, content, cmd_type);
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

auth_end:
	/* free content */
	free(content);
	content = NULL;
	/* cjson delete */
	cJSON_Delete(data);
	data = NULL;
	websSetStatus(wp, 400);
	websWriteHeaders(wp, -1, 0);
	websWriteEndHeaders(wp);
	websWrite(wp, "fail");
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
