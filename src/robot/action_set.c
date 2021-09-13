/********************************* Includes ***********************************/

#include    "goahead.h"
#include	"cJSON.h"
#include 	"sqlite3.h"
#include 	"tools.h"
#include 	"robot_socket.h"
#include 	"check_lua_file.h"
#include	"action_set.h"

/********************************* Defines ************************************/

extern char error_info[ERROR_SIZE];
extern char lua_filename[FILENAME_SIZE];
extern CTRL_STATE ctrl_state;
extern CTRL_STATE vir_ctrl_state;
extern FB_LinkQuene fb_quene;
extern SOCKET_INFO socket_cmd;
extern SOCKET_INFO socket_file;
extern SOCKET_INFO socket_state;
extern SOCKET_INFO socket_vir_cmd;
extern SOCKET_INFO socket_vir_file;
extern int robot_type;
extern STATE_FEEDBACK state_fb;
extern ACCOUNT_INFO cur_account;
extern TORQUE_SYS torquesys;
extern POINT_HOME_INFO point_home_info;
extern JIABAO_TORQUE_PRODUCTION_DATA jiabao_torque_pd_data;
//extern pthread_cond_t cond_cmd;
//extern pthread_cond_t cond_file;

/* DB JSON 结构体 */
typedef struct _DB_JSON
{
	cJSON *point;
	cJSON *cdsystem;
	cJSON *wobj_cdsystem;
	cJSON *et_cdsystem;
	cJSON *sysvar;
} DB_JSON;

/********************************* Function declaration ***********************/

static int copy_content(const cJSON *data_json, char *content);
static int program_start(const cJSON *data_json, char *content);
static int program_stop(const cJSON *data_json, char *content);
static int program_pause(const cJSON *data_json, char *content);
static int program_resume(const cJSON *data_json, char *content);
static int sendfilename(const cJSON *data_json, char *content);
static int init_db_json(DB_JSON *p_db_json);
static void db_json_delete(DB_JSON *p_db_json);
static int parse_lua_cmd(char *lua_cmd, char *file_content, DB_JSON *p_db_json);
static int sendfile(const cJSON *data_json, char *content);
static int step_over(const cJSON *data_json, char *content);
static int movej(const cJSON *data_json, char *content);
static int set_state_id(const cJSON *data_json, char *content);
static int set_state(const cJSON *data_json, char *content);
static int mode(const cJSON *data_json, char *content);
static int jointtotcf(const cJSON *data_json, char *content);
static int set_plugin_dio(const cJSON *data_json, char *content, int dio);
static int get_program_array(const cJSON *data_json, char *content);
static int get_program_content(const cJSON *data_json, char *content);
static int save_program(const cJSON *data_json, char *content);
static int upload_program(const cJSON *data_json, char *content);
static int get_control_mode(const cJSON *data_json, char *content);
static int set_control_mode(const cJSON *data_json, char *content);
static int test_tighten(const cJSON *data_json, char *content);
static int test_unscrew(const cJSON *data_json, char *content);
static int test_free(const cJSON *data_json, char *content);
static int test_stop(const cJSON *data_json, char *content);
static int set_on_off(const cJSON *data_json, char *content);
static int get_on_off(const cJSON *data_json, char *content);
static int get_current_range(const cJSON *data_json, char *content);
static int get_current_product(const cJSON *data_json, char *content);
static int set_torque_unit(const cJSON *data_json, char *content);
static int set_robot_type(const cJSON *data_json, char * content);
static int get_lua_content_size(const cJSON *data_json);
static int wait_cmd_feedback();

/*********************************** Code *************************************/

/* copy json data to content */
static int copy_content(const cJSON *data_json, char *content)
{
	cJSON *data = cJSON_GetObjectItem(data_json, "content");
	if (data == NULL || data->valuestring == NULL) {
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
	char **cmd_array = NULL;
	int size = 0;
	cJSON *name = NULL;

	name = cJSON_GetObjectItem(data_json, "name");
	if (name == NULL || name->valuestring == NULL) {
		perror("json");

		return FAIL;
	}
	sprintf(content, "%s%s", DIR_FRUSER, name->valuestring);
	bzero(lua_filename, FILENAME_SIZE);
	strcpy(lua_filename, content);

	if (strstr(lua_filename, "main")) {
		if (string_to_string_list(lua_filename, "_", &size, &cmd_array) == 0 || size != 3) {
			perror("string to string list");
			string_list_free(cmd_array, size);

			return FAIL;
		}
		memset(jiabao_torque_pd_data.left_wk_id, 0, 100);
		strcpy(jiabao_torque_pd_data.left_wk_id, cmd_array[1]);
		memset(jiabao_torque_pd_data.right_wk_id, 0, 100);
		strncpy(jiabao_torque_pd_data.right_wk_id, cmd_array[2], (strlen(cmd_array[2]) - 4));
		/* 保存最新数据到生产数据数据库 */
		update_torquesys_pd_data();
		string_list_free(cmd_array, size);
	}
	//printf("content = %s\n", content);

	return SUCCESS;
}

static int init_db_json(DB_JSON *p_db_json)
{
	char sql[MAX_BUF] = {0};

	/* open and get point.db content */
	memset(sql, 0, MAX_BUF);
	sprintf(sql, "select * from points;");
	if (select_info_json_sqlite3(DB_POINTS, sql, &p_db_json->point) == -1) {
		perror("points sqlite3 database");

		return FAIL;
	}

	memset(sql, 0, MAX_BUF);
	sprintf(sql, "select * from coordinate_system;");
	if (select_info_json_sqlite3(DB_CDSYSTEM, sql, &p_db_json->cdsystem) == -1) {
		perror("cdsystem sqlite3 database");

		return FAIL;
	}

	memset(sql, 0, MAX_BUF);
	sprintf(sql, "select * from wobj_coordinate_system;");
	if (select_info_json_sqlite3(DB_WOBJ_CDSYSTEM, sql, &p_db_json->wobj_cdsystem) == -1) {
		perror("wobj_cdsystem sqlite3 database");

		return FAIL;
	}

	memset(sql, 0, MAX_BUF);
	sprintf(sql, "select * from et_coordinate_system;");
	if (select_info_json_sqlite3(DB_ET_CDSYSTEM, sql, &p_db_json->et_cdsystem) == -1) {
		perror("et_cdsystem sqlite3 database");

		return FAIL;
	}

	memset(sql, 0, MAX_BUF);
	sprintf(sql, "select * from sysvar;");
	if (select_info_json_sqlite3(DB_SYSVAR, sql, &p_db_json->sysvar) == -1) {
		perror("sysvar sqlite3 database");

		return FAIL;
	}

	return SUCCESS;
}

static void db_json_delete(DB_JSON *p_db_json)
{
	cJSON_Delete(p_db_json->point);
	p_db_json->point = NULL;

	cJSON_Delete(p_db_json->cdsystem);
	p_db_json->cdsystem = NULL;

	cJSON_Delete(p_db_json->wobj_cdsystem);
	p_db_json->wobj_cdsystem = NULL;

	cJSON_Delete(p_db_json->et_cdsystem);
	p_db_json->et_cdsystem = NULL;

	cJSON_Delete(p_db_json->sysvar);
	p_db_json->sysvar = NULL;
}

/* parse cmd of lua file */
static int parse_lua_cmd(char *lua_cmd, char *file_content, DB_JSON *p_db_json)
{
	//printf("lua cmd = %s\n", lua_cmd);
	char content[MAX_BUF] = { 0 }; // 存储 lua_cmd 解析转换后的内容
	char head[MAX_BUF] = { 0 }; // 存储 lua_cmd 中指令, 之前的字符串内容
	char *ptr = NULL;  // 指向 lua_cmd 中包含的指令开头的指针
	char *end_ptr = NULL; // 指向 lua_cmd 结尾的指针
	char cmd_arg[MAX_BUF] = { 0 }; // 存储 lua_cmd （） 中参数内容
	char joint_value[6][10] = { 0 };
	char **cmd_array = NULL;
	int size = 0;
	char *joint_value_ptr[6];
	int i = 0;

	for (i = 0; i < 6; i++) {
		joint_value_ptr[i] = NULL;
	}
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
	cJSON *workpiecenum = NULL;
	cJSON *E1 = NULL;
	cJSON *E2 = NULL;
	cJSON *E3 = NULL;
	cJSON *E4 = NULL;

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
	cJSON *workpiecenum_2 = NULL;
	cJSON *E1_2 = NULL;
	cJSON *E2_2 = NULL;
	cJSON *E3_2 = NULL;
	cJSON *E4_2 = NULL;
	cJSON *cd = NULL;
	cJSON *et_cd = NULL;
	cJSON *ptp = NULL;
	cJSON *lin = NULL;
	cJSON *point_1 = NULL;
	cJSON *point_2 = NULL;
	cJSON *point_3 = NULL;
	cJSON *x_3 = NULL;
	cJSON *y_3 = NULL;
	cJSON *z_3 = NULL;
	cJSON *rx_3 = NULL;
	cJSON *ry_3 = NULL;
	cJSON *rz_3 = NULL;
	cJSON *ext_axis_ptp = NULL;
	cJSON *var = NULL;
	cJSON *installation_site = NULL;
	cJSON *type = NULL;

	/* 如果 lua 文件是旧版本格式，需要进行转换 */
	char old_head[MAX_BUF] = { 0 }; // 存储 lua_cmd 中":", 之前的字符串内容
	char *old_ptr = NULL;  // 指向 lua_cmd 中 ":" 之后的字符串指针
	char old_ptr_comment[MAX_BUF] = { 0 }; // 存储 lua_cmd 中 "：" 到 "--"之间的字符串内容
	char *old_comment = NULL; // 指向 lua_cmd 中 "--" 之后的字符串指针
	char new_lua_cmd[MAX_BUF] = { 0 }; // 存储转换后的 new_lua_cmd 字符串内容
	char new_lua_cmd_2[MAX_BUF] = { 0 }; // 存储转换后的 new_lua_cmd 字符串内容

	//printf("lua_cmd = %s\n", lua_cmd);
	if (old_ptr = strstr(lua_cmd, "WaitTime")) {
		strncpy(old_head, lua_cmd, (old_ptr - lua_cmd));
		sprintf(new_lua_cmd, "%sWaitMs%s", old_head, (old_ptr + 8));
		lua_cmd = new_lua_cmd;
		//printf("lua_cmd = %s\n", lua_cmd);
	} else if (old_ptr = strstr(lua_cmd, "EXT_AXIS_SETHOMING")) {
		strncpy(old_head, lua_cmd, (old_ptr - lua_cmd));
		sprintf(new_lua_cmd, "%sExtAxisSetHoming%s", old_head, (old_ptr + 18));
		lua_cmd = new_lua_cmd;
		//printf("lua_cmd = %s\n", lua_cmd);
	/* PTP 添加了平滑过度半径参数 */
	} else if (strstr(lua_cmd, "PTP") && (strstr(lua_cmd, "EXT_AXIS_PTP") == NULL) && (strstr(lua_cmd, "laserPTP") == NULL)) {
		if (string_to_string_list(lua_cmd, ",", &size, &cmd_array) == 0) {
			perror("string to string list");

			goto end;
		}
		if (size == 3) {
			sprintf(new_lua_cmd, "%s,%s,0,%s", cmd_array[0], cmd_array[1], cmd_array[2]);
			lua_cmd = new_lua_cmd;
		}
		if (size == 9) {
			sprintf(new_lua_cmd, "%s,%s,0,%s,%s,%s,%s,%s,%s,%s", cmd_array[0], cmd_array[1], cmd_array[2], cmd_array[3], cmd_array[4], cmd_array[5], cmd_array[6], cmd_array[7], cmd_array[8]);
			lua_cmd = new_lua_cmd;
		}
		string_list_free(cmd_array, size);
		//printf("lua_cmd = %s\n", lua_cmd);
	}

	if ((old_ptr = strstr(lua_cmd, ":")) && (strstr(lua_cmd, "::") == NULL)) {
		//printf("old_ptr = %s\n", old_ptr);
		strncpy(old_head, lua_cmd, (old_ptr - lua_cmd));
		//printf("old_head = %s\n", old_head);
		//strcpy(old_end, (old_ptr + 1));
		if (old_comment = strstr(old_ptr, "--")) {
			//printf("old_comment = %s\n", old_comment);
			strncpy(old_ptr_comment, (old_ptr + 1), (old_comment - old_ptr - 1));
			//printf("old_ptr_comment = %s\n", old_ptr_comment);
			/* 去掉字符串结尾多余空格 */
			//old_ptr_comment[strlen(old_ptr_comment)] = '\0';
			//printf("old_ptr_comment = %s\n", old_ptr_comment);
			sprintf(new_lua_cmd_2, "%s(%s)%s", old_head, old_ptr_comment, old_comment);
		} else {
			sprintf(new_lua_cmd_2, "%s(%s)", old_head, (old_ptr + 1));
		}
		lua_cmd = new_lua_cmd_2;
		//printf("lua_cmd = %s\n", lua_cmd);
	}

	/* laserPTP */
	if ((ptr = strstr(lua_cmd, "laserPTP(")) && strrchr(lua_cmd, ')')) {
		end_ptr = strrchr(lua_cmd, ')') + 1;
		strncpy(head, lua_cmd, (ptr - lua_cmd));
		strncpy(cmd_arg, (ptr + 9), (end_ptr - ptr - 10));
		if (string_to_string_list(cmd_arg, ",", &size, &cmd_array) == 0 || size != 2) {
			perror("string to string list");

			goto end;
		}
		sprintf(content, "%sMoveJ(%s,%s)%s\n", head, cmd_array[0], cmd_array[1], end_ptr);
		strcat(file_content, content);
	/* EXT_AXIS_PTP */
	} else if ((ptr = strstr(lua_cmd, "EXT_AXIS_PTP(")) && strrchr(lua_cmd, ')')) {
		end_ptr = strrchr(lua_cmd, ')') + 1;
		strncpy(head, lua_cmd, (ptr - lua_cmd));
		strncpy(cmd_arg, (ptr + 13), (end_ptr - ptr - 14));
		if (string_to_string_list(cmd_arg, ",", &size, &cmd_array) == 0 || size != 3) {
			perror("string to string list");

			goto end;
		}
		if (strcmp(cmd_array[1], "seamPos") == 0) {
			sprintf(content,"%sExtAxisMoveJ(%s,\"%s\",%s)%s\n", head, cmd_array[0], cmd_array[1], cmd_array[2], end_ptr);
		} else {
			ext_axis_ptp = cJSON_GetObjectItemCaseSensitive(p_db_json->point, cmd_array[1]);
			if (ext_axis_ptp == NULL || ext_axis_ptp->type != cJSON_Object) {

				goto end;
			}
			E1 = cJSON_GetObjectItem(ext_axis_ptp, "E1");
			E2 = cJSON_GetObjectItem(ext_axis_ptp, "E2");
			E3 = cJSON_GetObjectItem(ext_axis_ptp, "E3");
			E4 = cJSON_GetObjectItem(ext_axis_ptp, "E4");
			if (E1 == NULL || E2 == NULL || E3 == NULL || E4 == NULL || cmd_array[0] == NULL || cmd_array[2] == NULL || E1->valuestring == NULL || E2->valuestring == NULL || E3->valuestring == NULL || E4->valuestring == NULL) {

				goto end;
			}
			sprintf(content,"%sExtAxisMoveJ(%s,%s,%s,%s,%s,%s)%s\n", head, cmd_array[0], E1->valuestring, E2->valuestring, E3->valuestring, E4->valuestring, cmd_array[2], end_ptr);
		}
		strcat(file_content, content);
	/* PTP */
	} else if ((ptr = strstr(lua_cmd, "PTP(")) && strrchr(lua_cmd, ')')) {
		end_ptr = strrchr(lua_cmd, ')') + 1;
		strncpy(head, lua_cmd, (ptr - lua_cmd));
		strncpy(cmd_arg, (ptr + 4), (end_ptr - ptr - 5));
		if (string_to_string_list(cmd_arg, ",", &size, &cmd_array) == 0 || (size != 10 && size != 4)) {
			perror("string to string list");

			goto end;
		}
		ptp = cJSON_GetObjectItemCaseSensitive(p_db_json->point, cmd_array[0]);
		if (ptp == NULL || ptp->type != cJSON_Object) {

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
		workpiecenum = cJSON_GetObjectItem(ptp, "workpiecenum");
		speed = cJSON_GetObjectItem(ptp, "speed");
		acc = cJSON_GetObjectItem(ptp, "acc");
		E1 = cJSON_GetObjectItem(ptp, "E1");
		E2 = cJSON_GetObjectItem(ptp, "E2");
		E3 = cJSON_GetObjectItem(ptp, "E3");
		E4 = cJSON_GetObjectItem(ptp, "E4");
		if (j1 == NULL || j2 == NULL || j3 == NULL || j4 == NULL || j5 == NULL || j6 == NULL || x == NULL || y == NULL || z == NULL || rx == NULL || ry == NULL || rz == NULL || toolnum == NULL || workpiecenum == NULL || speed == NULL || acc == NULL || E1 == NULL || E2 == NULL || E3 == NULL || E4 == NULL || j1->valuestring == NULL || j2->valuestring == NULL || j3->valuestring == NULL || j4->valuestring == NULL || j5->valuestring == NULL || j6->valuestring == NULL || x->valuestring == NULL || y->valuestring == NULL || z->valuestring == NULL || rx->valuestring == NULL || ry->valuestring == NULL || rz->valuestring == NULL || toolnum->valuestring == NULL || workpiecenum->valuestring == NULL || speed->valuestring == NULL || acc->valuestring == NULL || cmd_array[1] == NULL || E1->valuestring == NULL || E2->valuestring == NULL || E3->valuestring == NULL || E4->valuestring == NULL) {

			goto end;
		}
		/* 当点为 pHOME 原点时，进行检查 */
		if (strcmp(cmd_array[0], POINT_HOME) == 0) {
			sprintf(joint_value[0], "%.1lf", atof(j1->valuestring));
			sprintf(joint_value[1], "%.1lf", atof(j2->valuestring));
			sprintf(joint_value[2], "%.1lf", atof(j3->valuestring));
			sprintf(joint_value[3], "%.1lf", atof(j4->valuestring));
			sprintf(joint_value[4], "%.1lf", atof(j5->valuestring));
			sprintf(joint_value[5], "%.1lf", atof(j6->valuestring));
			for (i = 0; i < 6; i++) {
				joint_value_ptr[i] = joint_value[i];
			}
			// 置异常报错的标志位， 添加异常错误到 sta 状态反馈 error_info 中
			if (check_pointhome_data(joint_value_ptr) == FAIL) {
				point_home_info.error_flag = 1;

				goto end;
			}
			point_home_info.error_flag = 0;
		}
		/* 参数个数为 10 时，即存在偏移 */
		if (size == 10) {
			sprintf(content,"%sMoveJ(%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s)%s\n", head, j1->valuestring, j2->valuestring, j3->valuestring, j4->valuestring, j5->valuestring, j6->valuestring, x->valuestring, y->valuestring, z->valuestring, rx->valuestring, ry->valuestring, rz->valuestring, toolnum->valuestring, workpiecenum->valuestring, speed->valuestring, acc->valuestring, cmd_array[1], E1->valuestring, E2->valuestring, E3->valuestring, E4->valuestring, cmd_array[2], cmd_array[3], cmd_array[4], cmd_array[5], cmd_array[6], cmd_array[7], cmd_array[8], cmd_array[9], end_ptr);
		} else {
			sprintf(content,"%sMoveJ(%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,0,0,0,0,0,0)%s\n", head, j1->valuestring, j2->valuestring, j3->valuestring, j4->valuestring, j5->valuestring, j6->valuestring, x->valuestring, y->valuestring, z->valuestring, rx->valuestring, ry->valuestring, rz->valuestring, toolnum->valuestring, workpiecenum->valuestring, speed->valuestring, acc->valuestring, cmd_array[1], E1->valuestring, E2->valuestring, E3->valuestring, E4->valuestring, cmd_array[2], cmd_array[3], end_ptr);
		}
		strcat(file_content, content);
	/* SPTP */
	} else if ((ptr = strstr(lua_cmd, "SPTP(")) && strrchr(lua_cmd, ')')) {
		end_ptr = strrchr(lua_cmd, ')') + 1;
		strncpy(head, lua_cmd, (ptr - lua_cmd));
		strncpy(cmd_arg, (ptr + 4), (end_ptr - ptr - 5));
		if (string_to_string_list(cmd_arg, ",", &size, &cmd_array) == 0 || size != 2) {
			perror("string to string list");

			goto end;
		}
		point_1 = cJSON_GetObjectItemCaseSensitive(p_db_json->point, cmd_array[0]);
		if (point_1 == NULL || point_1->type != cJSON_Object) {

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
		workpiecenum = cJSON_GetObjectItem(point_1, "workpiecenum");
		speed = cJSON_GetObjectItem(point_1, "speed");
		acc = cJSON_GetObjectItem(point_1, "acc");
		if (j1 == NULL || j2 == NULL || j3 == NULL || j4 == NULL || j5 == NULL || j6 == NULL || x == NULL || y == NULL || z == NULL || rx == NULL || ry == NULL || rz == NULL || toolnum == NULL || workpiecenum == NULL || speed == NULL || acc == NULL || j1->valuestring == NULL || j2->valuestring == NULL || j3->valuestring == NULL || j4->valuestring == NULL || j5->valuestring == NULL || j6->valuestring == NULL || x->valuestring == NULL || y->valuestring == NULL || z->valuestring == NULL || rx->valuestring == NULL || ry->valuestring == NULL || rz->valuestring == NULL || toolnum->valuestring == NULL || workpiecenum->valuestring == NULL || speed->valuestring == NULL || acc->valuestring == NULL || cmd_array[1] == NULL) {

			goto end;
		}
		/* 当点为 pHOME 原点时，进行检查 */
		if (strcmp(cmd_array[0], POINT_HOME) == 0) {
			sprintf(joint_value[0], "%.1lf", atof(j1->valuestring));
			sprintf(joint_value[1], "%.1lf", atof(j2->valuestring));
			sprintf(joint_value[2], "%.1lf", atof(j3->valuestring));
			sprintf(joint_value[3], "%.1lf", atof(j4->valuestring));
			sprintf(joint_value[4], "%.1lf", atof(j5->valuestring));
			sprintf(joint_value[5], "%.1lf", atof(j6->valuestring));
			for (i = 0; i < 6; i++) {
				joint_value_ptr[i] = joint_value[i];
			}
			/* 置异常报错的标志位， 添加异常错误到 sta 状态反馈 error_info 中 */
			if (check_pointhome_data(joint_value_ptr) == FAIL) {
				point_home_info.error_flag = 1;

				goto end;
			}
			point_home_info.error_flag = 0;
		}
		sprintf(content,"%sSplinePTP(%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s)%s\n", head, j1->valuestring, j2->valuestring, j3->valuestring, j4->valuestring, j5->valuestring,j6->valuestring, x->valuestring, y->valuestring, z->valuestring, rx->valuestring, ry->valuestring, rz->valuestring, toolnum->valuestring, workpiecenum->valuestring, speed->valuestring, acc->valuestring, cmd_array[1], end_ptr);
		strcat(file_content, content);
	/* laserLin */
	} else if ((ptr = strstr(lua_cmd, "laserLin(")) && strrchr(lua_cmd, ')')) {
		end_ptr = strrchr(lua_cmd, ')') + 1;
		strncpy(head, lua_cmd, (ptr - lua_cmd));
		strncpy(cmd_arg, (ptr + 9), (end_ptr - ptr - 10));
		if (string_to_string_list(cmd_arg, ",", &size, &cmd_array) == 0 || size != 2) {
			perror("string to string list");

			goto end;
		}
		sprintf(content, "%sMoveL(%s,%s)%s\n", head, cmd_array[0], cmd_array[1], end_ptr);
		strcat(file_content, content);
	/* SLIN */
	} else if ((ptr = strstr(lua_cmd, "SLIN(")) && strrchr(lua_cmd, ')')) {
		end_ptr = strrchr(lua_cmd, ')') + 1;
		strncpy(head, lua_cmd, (ptr - lua_cmd));
		strncpy(cmd_arg, (ptr + 5), (end_ptr - ptr - 6));
		if (string_to_string_list(cmd_arg, ",", &size, &cmd_array) == 0 || size != 2) {
			perror("string to string list");

			goto end;
		}
		point_1 = cJSON_GetObjectItemCaseSensitive(p_db_json->point, cmd_array[0]);
		if (point_1 == NULL || point_1->type != cJSON_Object) {

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
		workpiecenum = cJSON_GetObjectItem(point_1, "workpiecenum");
		speed = cJSON_GetObjectItem(point_1, "speed");
		acc = cJSON_GetObjectItem(point_1, "acc");
		if (j1 == NULL || j2 == NULL || j3 == NULL || j4 == NULL || j5 == NULL || j6 == NULL || x == NULL || y == NULL || z == NULL || rx == NULL || ry == NULL || rz == NULL || toolnum == NULL || workpiecenum == NULL || speed == NULL || acc == NULL || j1->valuestring == NULL || j2->valuestring == NULL || j3->valuestring == NULL || j4->valuestring == NULL || j5->valuestring == NULL || j6->valuestring == NULL || x->valuestring == NULL || y->valuestring == NULL || z->valuestring == NULL || rx->valuestring == NULL || ry->valuestring == NULL || rz->valuestring == NULL || toolnum->valuestring == NULL || workpiecenum->valuestring == NULL || speed->valuestring == NULL || acc->valuestring == NULL || cmd_array[1] == NULL) {

			goto end;
		}
		/* 当点为 pHOME 原点时，进行检查 */
		if (strcmp(cmd_array[0], POINT_HOME) == 0) {
			sprintf(joint_value[0], "%.1lf", atof(j1->valuestring));
			sprintf(joint_value[1], "%.1lf", atof(j2->valuestring));
			sprintf(joint_value[2], "%.1lf", atof(j3->valuestring));
			sprintf(joint_value[3], "%.1lf", atof(j4->valuestring));
			sprintf(joint_value[4], "%.1lf", atof(j5->valuestring));
			sprintf(joint_value[5], "%.1lf", atof(j6->valuestring));
			for (i = 0; i < 6; i++) {
				joint_value_ptr[i] = joint_value[i];
			}
			/* 置异常报错的标志位， 添加异常错误到 sta 状态反馈 error_info 中 */
			if (check_pointhome_data(joint_value_ptr) == FAIL) {
				point_home_info.error_flag = 1;

				goto end;
			}
			point_home_info.error_flag = 0;
		}
		sprintf(content, "%sSplineLINE(%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s)%s\n", head, j1->valuestring, j2->valuestring, j3->valuestring, j4->valuestring, j5->valuestring, j6->valuestring, x->valuestring, y->valuestring, z->valuestring, rx->valuestring, ry->valuestring, rz->valuestring, toolnum->valuestring, workpiecenum->valuestring, speed->valuestring, acc->valuestring, cmd_array[1], end_ptr);
		strcat(file_content, content);
	/* Lin */
	} else if ((ptr = strstr(lua_cmd, "Lin(")) && strrchr(lua_cmd, ')')) {
		end_ptr = strrchr(lua_cmd, ')') + 1;
		/*
		printf("end_ptr = %s\n", end_ptr);
		printf("ptr = %s\n", ptr);
		printf("ptr - lua_cmd = %d\n", (ptr - lua_cmd));
		*/
		strncpy(head, lua_cmd, (ptr - lua_cmd));
		/*
		printf("head = %s\n", head);
		printf("end_ptr - ptr - 5 = %d\n", (end_ptr - ptr - 5));
		printf("ptr + 4 = %s\n", (ptr + 4));
		*/
		strncpy(cmd_arg, (ptr + 4), (end_ptr - ptr - 5));
		if (string_to_string_list(cmd_arg, ",", &size, &cmd_array) == 0 || (size != 6 && size != 4 && size != 10)) {
			perror("string to string list");

			goto end;
		}
		lin = cJSON_GetObjectItemCaseSensitive(p_db_json->point, cmd_array[0]);
		if (lin == NULL || lin->type != cJSON_Object) {

			goto end;
		}
		/* seamPos 下发参数为6个, 第6个参数为偏置，暂时不处理 */
		if (strcmp(cmd_array[0], "seamPos") == 0 && size == 6) {
			speed = cJSON_GetObjectItem(lin, "speed");
			acc = cJSON_GetObjectItem(lin, "acc");
			toolnum = cJSON_GetObjectItem(lin, "toolnum");
			workpiecenum = cJSON_GetObjectItem(lin, "workpiecenum");
			if (toolnum == NULL || workpiecenum == NULL || speed == NULL || acc == NULL || toolnum->valuestring == NULL || workpiecenum->valuestring == NULL || speed->valuestring == NULL || acc->valuestring == NULL || cmd_array[1] == NULL) {

				goto end;
			}
			sprintf(content, "%sMoveL(\"%s\",%s,%s,%s,%s,%s,%s,%s,%s)%s\n", head, cmd_array[0], toolnum->valuestring, workpiecenum->valuestring, speed->valuestring, acc->valuestring, cmd_array[1], cmd_array[2], cmd_array[3], cmd_array[4], end_ptr);
		/* cvrCatchPoint 和 cvrRaisePoint 下发参数为 4 个, 第 4 个参数为偏置，暂时不处理 */
		} else if ((strcmp(cmd_array[0], "cvrCatchPoint") == 0 || strcmp(cmd_array[0], "cvrRaisePoint") == 0) && size == 4) {
			speed = cJSON_GetObjectItem(lin, "speed");
			acc = cJSON_GetObjectItem(lin, "acc");
			toolnum = cJSON_GetObjectItem(lin, "toolnum");
			workpiecenum = cJSON_GetObjectItem(lin, "workpiecenum");
			if (toolnum == NULL || workpiecenum == NULL || speed == NULL || acc == NULL || toolnum->valuestring == NULL || workpiecenum->valuestring == NULL || speed->valuestring == NULL || acc->valuestring == NULL || cmd_array[1] == NULL) {

				goto end;
			}
			sprintf(content, "%sMoveL(\"%s\",%s,%s,%s,%s,%s,%s,0,0)%s\n", head, cmd_array[0], toolnum->valuestring, workpiecenum->valuestring, speed->valuestring, acc->valuestring, cmd_array[1], cmd_array[2], end_ptr);
		/* 正常 Lin 指令下发参数为 4 或者 10 个, 第 4 个参数为偏置位 */
		} else if (size == 4 || size == 10) {
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
			workpiecenum = cJSON_GetObjectItem(lin, "workpiecenum");
			E1 = cJSON_GetObjectItem(lin, "E1");
			E2 = cJSON_GetObjectItem(lin, "E2");
			E3 = cJSON_GetObjectItem(lin, "E3");
			E4 = cJSON_GetObjectItem(lin, "E4");
			if (j1 == NULL || j2 == NULL || j3 == NULL || j4 == NULL || j5 == NULL || j6 == NULL || x == NULL || y == NULL || z == NULL || rx == NULL || ry == NULL || rz == NULL || toolnum == NULL || workpiecenum == NULL || speed == NULL || acc == NULL || E1 == NULL || E2 == NULL || E3 == NULL || E4 == NULL || j1->valuestring == NULL || j2->valuestring == NULL || j3->valuestring == NULL || j4->valuestring == NULL || j5->valuestring == NULL || j6->valuestring == NULL || x->valuestring == NULL || y->valuestring == NULL || z->valuestring == NULL || rx->valuestring == NULL || ry->valuestring == NULL || rz->valuestring == NULL || toolnum->valuestring == NULL || workpiecenum->valuestring == NULL || speed->valuestring == NULL || acc->valuestring == NULL || cmd_array[1] == NULL || cmd_array[2] == NULL || E1->valuestring == NULL || E2->valuestring == NULL || E3->valuestring == NULL || E4->valuestring == NULL) {

				goto end;
			}
			/* 当点为 pHOME 原点时，进行检查 */
			if (strcmp(cmd_array[0], POINT_HOME) == 0) {
				sprintf(joint_value[0], "%.1lf", atof(j1->valuestring));
				sprintf(joint_value[1], "%.1lf", atof(j2->valuestring));
				sprintf(joint_value[2], "%.1lf", atof(j3->valuestring));
				sprintf(joint_value[3], "%.1lf", atof(j4->valuestring));
				sprintf(joint_value[4], "%.1lf", atof(j5->valuestring));
				sprintf(joint_value[5], "%.1lf", atof(j6->valuestring));
				for (i = 0; i < 6; i++) {
					joint_value_ptr[i] = joint_value[i];
				}
				/* 置异常报错的标志位， 添加异常错误到 sta 状态反馈 error_info 中 */
				if (check_pointhome_data(joint_value_ptr) == FAIL) {
					point_home_info.error_flag = 1;

					goto end;
				}
				point_home_info.error_flag = 0;
			}
			/* 参数个数为 10 时，即存在偏移 */
			if (size == 10) {
				sprintf(content, "%sMoveL(%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s)%s\n", head, j1->valuestring, j2->valuestring, j3->valuestring, j4->valuestring, j5->valuestring, j6->valuestring, x->valuestring, y->valuestring, z->valuestring, rx->valuestring, ry->valuestring, rz->valuestring, toolnum->valuestring, workpiecenum->valuestring, speed->valuestring, acc->valuestring, cmd_array[1], cmd_array[2], E1->valuestring, E2->valuestring, E3->valuestring, E4->valuestring, cmd_array[3], cmd_array[4], cmd_array[5], cmd_array[6], cmd_array[7], cmd_array[8], cmd_array[9], end_ptr);
			} else {
				sprintf(content, "%sMoveL(%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,0,0,0,0,0,0,0)%s\n", head, j1->valuestring, j2->valuestring, j3->valuestring, j4->valuestring, j5->valuestring, j6->valuestring, x->valuestring, y->valuestring, z->valuestring, rx->valuestring, ry->valuestring, rz->valuestring, toolnum->valuestring, workpiecenum->valuestring, speed->valuestring, acc->valuestring, cmd_array[1], cmd_array[2], E1->valuestring, E2->valuestring, E3->valuestring, E4->valuestring, end_ptr);
			}
		}
		strcat(file_content, content);
	/* ARC */
	} else if ((ptr = strstr(lua_cmd, "ARC(")) && strrchr(lua_cmd, ')')) {
		end_ptr = strrchr(lua_cmd, ')') + 1;
		strncpy(head, lua_cmd, (ptr - lua_cmd));
		strncpy(cmd_arg, (ptr + 4), (end_ptr - ptr - 5));
		if (string_to_string_list(cmd_arg, ",", &size, &cmd_array) == 0 || size != 3) {
			perror("string to string list");

			goto end;
		}
		point_1 = cJSON_GetObjectItemCaseSensitive(p_db_json->point, cmd_array[0]);
		if (point_1 == NULL || point_1->type != cJSON_Object) {

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
		workpiecenum = cJSON_GetObjectItem(point_1, "workpiecenum");
		speed = cJSON_GetObjectItem(point_1, "speed");
		acc = cJSON_GetObjectItem(point_1, "acc");
		if (j1 == NULL || j2 == NULL || j3 == NULL || j4 == NULL || j5 == NULL || j6 == NULL || x == NULL || y == NULL || z == NULL || rx == NULL || ry == NULL || rz == NULL || toolnum == NULL || workpiecenum == NULL || speed == NULL || acc == NULL || j1->valuestring == NULL || j2->valuestring == NULL || j3->valuestring == NULL || j4->valuestring == NULL || j5->valuestring == NULL || j6->valuestring == NULL || x->valuestring == NULL || y->valuestring == NULL || z->valuestring == NULL || rx->valuestring == NULL || ry->valuestring == NULL || rz->valuestring == NULL || toolnum->valuestring == NULL || workpiecenum->valuestring == NULL || speed->valuestring == NULL || acc->valuestring == NULL || cmd_array[1] == NULL) {

			goto end;
		}
		/* 当点为 pHOME 原点时，进行检查 */
		if (strcmp(cmd_array[0], POINT_HOME) == 0) {
			sprintf(joint_value[0], "%.1lf", atof(j1->valuestring));
			sprintf(joint_value[1], "%.1lf", atof(j2->valuestring));
			sprintf(joint_value[2], "%.1lf", atof(j3->valuestring));
			sprintf(joint_value[3], "%.1lf", atof(j4->valuestring));
			sprintf(joint_value[4], "%.1lf", atof(j5->valuestring));
			sprintf(joint_value[5], "%.1lf", atof(j6->valuestring));
			for (i = 0; i < 6; i++) {
				joint_value_ptr[i] = joint_value[i];
			}
			/* 置异常报错的标志位，添加异常错误到 sta 状态反馈 error_info 中 */
			if (check_pointhome_data(joint_value_ptr) == FAIL) {
				point_home_info.error_flag = 1;

				goto end;
			}
			point_home_info.error_flag = 0;
		}
		point_2 = cJSON_GetObjectItemCaseSensitive(p_db_json->point, cmd_array[1]);
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
		workpiecenum_2 = cJSON_GetObjectItem(point_2, "workpiecenum");
		speed_2 = cJSON_GetObjectItem(point_2, "speed");
		acc_2 = cJSON_GetObjectItem(point_2, "acc");
		E1_2 = cJSON_GetObjectItem(point_2, "E1");
		E2_2 = cJSON_GetObjectItem(point_2, "E2");
		E3_2 = cJSON_GetObjectItem(point_2, "E3");
		E4_2 = cJSON_GetObjectItem(point_2, "E4");
		if (j1_2 == NULL || j2_2 == NULL || j3_2 == NULL || j4_2 == NULL || j5_2 == NULL || j6_2 == NULL || x_2 == NULL || y_2 == NULL || z_2 == NULL || rx_2 == NULL || ry_2 == NULL || rz_2 == NULL || toolnum_2 == NULL || workpiecenum_2 == NULL || speed_2 == NULL || acc_2 == NULL || E1_2 == NULL || E2_2 == NULL || E3_2 == NULL || E4_2 == NULL || j1_2->valuestring == NULL || j2_2->valuestring == NULL || j3_2->valuestring == NULL || j4_2->valuestring == NULL || j5_2->valuestring == NULL || j6_2->valuestring == NULL || x_2->valuestring == NULL || y_2->valuestring == NULL || z_2->valuestring == NULL || rx_2->valuestring == NULL || ry_2->valuestring == NULL || rz_2->valuestring == NULL || toolnum_2->valuestring == NULL || workpiecenum_2->valuestring == NULL || speed_2->valuestring == NULL || acc_2->valuestring == NULL || E1_2->valuestring == NULL || E2_2->valuestring == NULL || E3_2->valuestring == NULL || E4_2->valuestring == NULL || cmd_array[2] == NULL) {

			goto end;
		}
		/* 当点为 pHOME 原点时，进行检查 */
		if (strcmp(cmd_array[1], POINT_HOME) == 0) {
			for (i = 0; i < 6; i++) {
				memset(joint_value[i], 0, 10);
			}
			sprintf(joint_value[0], "%.1lf", atof(j1_2->valuestring));
			sprintf(joint_value[1], "%.1lf", atof(j2_2->valuestring));
			sprintf(joint_value[2], "%.1lf", atof(j3_2->valuestring));
			sprintf(joint_value[3], "%.1lf", atof(j4_2->valuestring));
			sprintf(joint_value[4], "%.1lf", atof(j5_2->valuestring));
			sprintf(joint_value[5], "%.1lf", atof(j6_2->valuestring));
			for (i = 0; i < 6; i++) {
				joint_value_ptr[i] = joint_value[i];
			}
			/* 置异常报错的标志位， 添加异常错误到 sta 状态反馈 error_info 中 */
			if (check_pointhome_data(joint_value_ptr) == FAIL) {
				point_home_info.error_flag = 1;

				goto end;
			}
			point_home_info.error_flag = 0;
		}
		sprintf(content, "%sMoveC(%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s)%s\n", head, j1->valuestring, j2->valuestring, j3->valuestring, j4->valuestring, j5->valuestring, j6->valuestring, x->valuestring, y->valuestring, z->valuestring, rx->valuestring, ry->valuestring, rz->valuestring, toolnum->valuestring, workpiecenum->valuestring, speed->valuestring, acc->valuestring, j1_2->valuestring, j2_2->valuestring, j3_2->valuestring, j4_2->valuestring, j5_2->valuestring, j6_2->valuestring, x_2->valuestring, y_2->valuestring, z_2->valuestring, rx_2->valuestring, ry_2->valuestring, rz_2->valuestring, toolnum_2->valuestring, workpiecenum_2->valuestring, speed_2->valuestring, acc_2->valuestring, cmd_array[2], E1_2->valuestring, E2_2->valuestring, E3_2->valuestring, E4_2->valuestring, end_ptr);
		strcat(file_content, content);
	/* Circle */
	} else if ((ptr = strstr(lua_cmd, "Circle(")) && strrchr(lua_cmd, ')')) {
		end_ptr = strrchr(lua_cmd, ')') + 1;
		strncpy(head, lua_cmd, (ptr - lua_cmd));
		strncpy(cmd_arg, (ptr + 7), (end_ptr - ptr - 8));
		if (string_to_string_list(cmd_arg, ",", &size, &cmd_array) == 0 || size != 3) {
			perror("string to string list");

			goto end;
		}
		if (cmd_array[0] == NULL) {

			goto end;
		}
		point_1 = cJSON_GetObjectItemCaseSensitive(p_db_json->point, cmd_array[0]);
		if (point_1 == NULL || point_1->type != cJSON_Object) {

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
		workpiecenum = cJSON_GetObjectItem(point_1, "workpiecenum");
		speed = cJSON_GetObjectItem(point_1, "speed");
		acc = cJSON_GetObjectItem(point_1, "acc");
		E1 = cJSON_GetObjectItem(point_1, "E1");
		E2 = cJSON_GetObjectItem(point_1, "E2");
		E3 = cJSON_GetObjectItem(point_1, "E3");
		E4 = cJSON_GetObjectItem(point_1, "E4");
		if (j1 == NULL || j2 == NULL || j3 == NULL || j4 == NULL || j5 == NULL || j6 == NULL || x == NULL || y == NULL || z == NULL || rx == NULL || ry == NULL || rz == NULL || toolnum == NULL || workpiecenum == NULL || speed == NULL || acc == NULL || j1->valuestring == NULL || j2->valuestring == NULL || j3->valuestring == NULL || j4->valuestring == NULL || j5->valuestring == NULL || j6->valuestring == NULL || x->valuestring == NULL || y->valuestring == NULL || z->valuestring == NULL || rx->valuestring == NULL || ry->valuestring == NULL || rz->valuestring == NULL || toolnum->valuestring == NULL || workpiecenum->valuestring == NULL || speed->valuestring == NULL || acc->valuestring == NULL || E1->valuestring == NULL || E2->valuestring == NULL || E3->valuestring == NULL || E4->valuestring == NULL) {

			goto end;
		}
		/* 当点为 pHOME 原点时，进行检查 */
		if (strcmp(cmd_array[0], POINT_HOME) == 0) {
			sprintf(joint_value[0], "%.1lf", atof(j1->valuestring));
			sprintf(joint_value[1], "%.1lf", atof(j2->valuestring));
			sprintf(joint_value[2], "%.1lf", atof(j3->valuestring));
			sprintf(joint_value[3], "%.1lf", atof(j4->valuestring));
			sprintf(joint_value[4], "%.1lf", atof(j5->valuestring));
			sprintf(joint_value[5], "%.1lf", atof(j6->valuestring));
			for (i = 0; i < 6; i++) {
				joint_value_ptr[i] = joint_value[i];
			}
			/* 置异常报错的标志位，添加异常错误到 sta 状态反馈 error_info 中 */
			if (check_pointhome_data(joint_value_ptr) == FAIL) {
				point_home_info.error_flag = 1;

				goto end;
			}
			point_home_info.error_flag = 0;
		}
		if (cmd_array[1] == NULL) {

			goto end;
		}
		point_2 = cJSON_GetObjectItemCaseSensitive(p_db_json->point, cmd_array[1]);
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
		workpiecenum_2 = cJSON_GetObjectItem(point_2, "workpiecenum");
		speed_2 = cJSON_GetObjectItem(point_2, "speed");
		acc_2 = cJSON_GetObjectItem(point_2, "acc");
		E1_2 = cJSON_GetObjectItem(point_2, "E1");
		E2_2 = cJSON_GetObjectItem(point_2, "E2");
		E3_2 = cJSON_GetObjectItem(point_2, "E3");
		E4_2 = cJSON_GetObjectItem(point_2, "E4");
		if (j1_2 == NULL || j2_2 == NULL || j3_2 == NULL || j4_2 == NULL || j5_2 == NULL || j6_2 == NULL || x_2 == NULL || y_2 == NULL || z_2 == NULL || rx_2 == NULL || ry_2 == NULL || rz_2 == NULL || toolnum_2 == NULL || workpiecenum_2 == NULL || speed_2 == NULL || acc_2 == NULL || E1_2 == NULL || E2_2 == NULL || E3_2 == NULL || E4_2 == NULL || j1_2->valuestring == NULL || j2_2->valuestring == NULL || j3_2->valuestring == NULL || j4_2->valuestring == NULL || j5_2->valuestring == NULL || j6_2->valuestring == NULL || x_2->valuestring == NULL || y_2->valuestring == NULL || z_2->valuestring == NULL || rx_2->valuestring == NULL || ry_2->valuestring == NULL || rz_2->valuestring == NULL || toolnum_2->valuestring == NULL || workpiecenum_2->valuestring == NULL || speed_2->valuestring == NULL || acc_2->valuestring == NULL || E1_2->valuestring == NULL || E2_2->valuestring == NULL || E3_2->valuestring == NULL || E4_2->valuestring == NULL) {

			goto end;
		}
		/* 当点为 pHOME 原点时，进行检查 */
		if (strcmp(cmd_array[1], POINT_HOME) == 0) {
			for (i = 0; i < 6; i++) {
				memset(joint_value[i], 0, 10);
			}
			sprintf(joint_value[0], "%.1lf", atof(j1_2->valuestring));
			sprintf(joint_value[1], "%.1lf", atof(j2_2->valuestring));
			sprintf(joint_value[2], "%.1lf", atof(j3_2->valuestring));
			sprintf(joint_value[3], "%.1lf", atof(j4_2->valuestring));
			sprintf(joint_value[4], "%.1lf", atof(j5_2->valuestring));
			sprintf(joint_value[5], "%.1lf", atof(j6_2->valuestring));
			for (i = 0; i < 6; i++) {
				joint_value_ptr[i] = joint_value[i];
			}
			/* 置异常报错的标志位， 添加异常错误到 sta 状态反馈 error_info 中 */
			if (check_pointhome_data(joint_value_ptr) == FAIL) {
				point_home_info.error_flag = 1;

				goto end;
			}
			point_home_info.error_flag = 0;
		}
		if (cmd_array[2] == NULL) {

			goto end;
		}
		sprintf(content, "%sCircle(%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s)%s\n", head, j1->valuestring, j2->valuestring, j3->valuestring, j4->valuestring, j5->valuestring, j6->valuestring, x->valuestring, y->valuestring, z->valuestring, rx->valuestring, ry->valuestring, rz->valuestring, toolnum->valuestring, workpiecenum->valuestring, speed->valuestring, acc->valuestring, E1->valuestring, E2->valuestring, E3->valuestring, E4->valuestring, j1_2->valuestring, j2_2->valuestring, j3_2->valuestring, j4_2->valuestring, j5_2->valuestring, j6_2->valuestring, x_2->valuestring, y_2->valuestring, z_2->valuestring, rx_2->valuestring, ry_2->valuestring, rz_2->valuestring, toolnum_2->valuestring, workpiecenum_2->valuestring, speed_2->valuestring, acc_2->valuestring, E1_2->valuestring, E2_2->valuestring, E3_2->valuestring, E4_2->valuestring, cmd_array[2], end_ptr);
		strcat(file_content, content);
	/* SCIRC */
	} else if ((ptr = strstr(lua_cmd, "SCIRC(")) && strrchr(lua_cmd, ')')) {
		end_ptr = strrchr(lua_cmd, ')') + 1;
		strncpy(head, lua_cmd, (ptr - lua_cmd));
		strncpy(cmd_arg, (ptr + 6), (end_ptr - ptr - 7));
		if (string_to_string_list(cmd_arg, ",", &size, &cmd_array) == 0 || size != 3) {
			perror("string to string list");

			goto end;
		}
		point_1 = cJSON_GetObjectItemCaseSensitive(p_db_json->point, cmd_array[0]);
		if (point_1 == NULL || point_1->type != cJSON_Object) {

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
		workpiecenum = cJSON_GetObjectItem(point_1, "workpiecenum");
		speed = cJSON_GetObjectItem(point_1, "speed");
		acc = cJSON_GetObjectItem(point_1, "acc");
		if (j1 == NULL || j2 == NULL || j3 == NULL || j4 == NULL || j5 == NULL || j6 == NULL || x == NULL || y == NULL || z == NULL || rx == NULL || ry == NULL || rz == NULL || toolnum == NULL || workpiecenum == NULL || speed == NULL || acc == NULL || j1->valuestring == NULL || j2->valuestring == NULL || j3->valuestring == NULL || j4->valuestring == NULL || j5->valuestring == NULL || j6->valuestring == NULL || x->valuestring == NULL || y->valuestring == NULL || z->valuestring == NULL || rx->valuestring == NULL || ry->valuestring == NULL || rz->valuestring == NULL || toolnum->valuestring == NULL || workpiecenum->valuestring == NULL || speed->valuestring == NULL || acc->valuestring == NULL || cmd_array[1] == NULL) {

			goto end;
		}
		/* 当点为 pHOME 原点时，进行检查 */
		if (strcmp(cmd_array[0], POINT_HOME) == 0) {
			sprintf(joint_value[0], "%.1lf", atof(j1->valuestring));
			sprintf(joint_value[1], "%.1lf", atof(j2->valuestring));
			sprintf(joint_value[2], "%.1lf", atof(j3->valuestring));
			sprintf(joint_value[3], "%.1lf", atof(j4->valuestring));
			sprintf(joint_value[4], "%.1lf", atof(j5->valuestring));
			sprintf(joint_value[5], "%.1lf", atof(j6->valuestring));
			for (i = 0; i < 6; i++) {
				joint_value_ptr[i] = joint_value[i];
			}
			/* 置异常报错的标志位， 添加异常错误到 sta 状态反馈 error_info 中 */
			if (check_pointhome_data(joint_value_ptr) == FAIL) {
				point_home_info.error_flag = 1;

				goto end;
			}
			point_home_info.error_flag = 0;
		}
		point_2 = cJSON_GetObjectItemCaseSensitive(p_db_json->point, cmd_array[1]);
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
		workpiecenum_2 = cJSON_GetObjectItem(point_2, "workpiecenum");
		speed_2 = cJSON_GetObjectItem(point_2, "speed");
		acc_2 = cJSON_GetObjectItem(point_2, "acc");
		if (j1_2 == NULL || j2_2 == NULL || j3_2 == NULL || j4_2 == NULL || j5_2 == NULL || j6_2 == NULL || x_2 == NULL || y_2 == NULL || z_2 == NULL || rx_2 == NULL || ry_2 == NULL || rz_2 == NULL || toolnum_2 == NULL || workpiecenum_2 == NULL || speed_2 == NULL || acc_2 == NULL || j1_2->valuestring == NULL || j2_2->valuestring == NULL || j3_2->valuestring == NULL || j4_2->valuestring == NULL || j5_2->valuestring == NULL || j6_2->valuestring == NULL || x_2->valuestring == NULL || y_2->valuestring == NULL || z_2->valuestring == NULL || rx_2->valuestring == NULL || ry_2->valuestring == NULL || rz_2->valuestring == NULL || toolnum_2->valuestring == NULL || workpiecenum_2->valuestring == NULL || speed_2->valuestring == NULL || acc_2->valuestring == NULL || cmd_array[2] == NULL) {

			goto end;
		}
		/* 当点为 pHOME 原点时，进行检查 */
		if (strcmp(cmd_array[1], POINT_HOME) == 0) {
			for (i = 0; i < 6; i++) {
				memset(joint_value[i], 0, 10);
			}
			sprintf(joint_value[0], "%.1lf", atof(j1_2->valuestring));
			sprintf(joint_value[1], "%.1lf", atof(j2_2->valuestring));
			sprintf(joint_value[2], "%.1lf", atof(j3_2->valuestring));
			sprintf(joint_value[3], "%.1lf", atof(j4_2->valuestring));
			sprintf(joint_value[4], "%.1lf", atof(j5_2->valuestring));
			sprintf(joint_value[5], "%.1lf", atof(j6_2->valuestring));
			for (i = 0; i < 6; i++) {
				joint_value_ptr[i] = joint_value[i];
			}
			/* 置异常报错的标志位， 添加异常错误到 sta 状态反馈 error_info 中 */
			if (check_pointhome_data(joint_value_ptr) == FAIL) {
				point_home_info.error_flag = 1;

				goto end;
			}
			point_home_info.error_flag = 0;
		}
		sprintf(content, "%sSplineCIRC(%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s)%s\n", head, j1->valuestring, j2->valuestring, j3->valuestring, j4->valuestring, j5->valuestring, j6->valuestring, x->valuestring, y->valuestring, z->valuestring, rx->valuestring, ry->valuestring, rz->valuestring, toolnum->valuestring, workpiecenum->valuestring, speed->valuestring, acc->valuestring, j1_2->valuestring, j2_2->valuestring, j3_2->valuestring, j4_2->valuestring, j5_2->valuestring, j6_2->valuestring, x_2->valuestring, y_2->valuestring, z_2->valuestring, rx_2->valuestring, ry_2->valuestring, rz_2->valuestring, toolnum_2->valuestring, workpiecenum_2->valuestring, speed_2->valuestring, acc_2->valuestring, cmd_array[2], end_ptr);
		strcat(file_content, content);
	/* set AO */
	} else if ((ptr = strstr(lua_cmd, "SetAO(")) && strrchr(lua_cmd, ')')) {
		end_ptr = strrchr(lua_cmd, ')') + 1;
		strncpy(head, lua_cmd, (ptr - lua_cmd));
		strncpy(cmd_arg, (ptr + 6), (end_ptr - ptr - 7));
		if (string_to_string_list(cmd_arg, ",", &size, &cmd_array) == 0 || size != 2) {
			perror("string to string list");

			goto end;
		}
		sprintf(content, "%sSetAO(%s,%.2f)%s\n", head, cmd_array[0], (float)(atoi(cmd_array[1])*40.95), end_ptr);
		strcat(file_content, content);
	/* set ToolAO */
	} else if ((ptr = strstr(lua_cmd, "SetToolAO(")) && strrchr(lua_cmd, ')')) {
		end_ptr = strrchr(lua_cmd, ')') + 1;
		strncpy(head, lua_cmd, (ptr - lua_cmd));
		strncpy(cmd_arg, (ptr + 10), (end_ptr - ptr - 11));
		if (string_to_string_list(cmd_arg, ",", &size, &cmd_array) == 0 || size != 2) {
			perror("string to string list");

			goto end;
		}
		sprintf(content, "%sSetToolAO(%s,%.2f)%s\n", head, cmd_array[0], (float)(atoi(cmd_array[1])*40.95), end_ptr);
		strcat(file_content, content);
	/* soft-PLC setAO */
	} else if ((ptr = strstr(lua_cmd, "SPLCSetAO(")) && strrchr(lua_cmd, ')')) {
		end_ptr = strrchr(lua_cmd, ')') + 1;
		strncpy(head, lua_cmd, (ptr - lua_cmd));
		strncpy(cmd_arg, (ptr + 10), (end_ptr - ptr - 11));
		if (string_to_string_list(cmd_arg, ",", &size, &cmd_array) == 0 || size != 2) {
			perror("string to string list");

			goto end;
		}
		sprintf(content, "%sSPLCSetAO(%s,%.2f)%s\n", head, cmd_array[0], (float)(atoi(cmd_array[1])*40.95), end_ptr);
		strcat(file_content, content);
	/* soft-PLC setToolAO */
	} else if ((ptr = strstr(lua_cmd, "SPLCSetToolAO(")) && strrchr(lua_cmd, ')')) {
		end_ptr = strrchr(lua_cmd, ')') + 1;
		strncpy(head, lua_cmd, (ptr - lua_cmd));
		strncpy(cmd_arg, (ptr + 14), (end_ptr - ptr - 15));
		if (string_to_string_list(cmd_arg, ",", &size, &cmd_array) == 0 || size != 2) {
			perror("string to string list");

			goto end;
		}
		sprintf(content, "%sSPLCSetToolAO(%s,%.2f)%s\n", head, cmd_array[0], (float)(atoi(cmd_array[1])*40.95), end_ptr);
		strcat(file_content, content);
	/* set ToolList */
	} else if ((ptr = strstr(lua_cmd, "SetToolList(")) && strrchr(lua_cmd, ')')) {
		end_ptr = strrchr(lua_cmd, ')') + 1;
		strncpy(head, lua_cmd, (ptr - lua_cmd));
		strncpy(cmd_arg, (ptr + 12), (end_ptr - ptr - 13));
		if (string_to_string_list(cmd_arg, ",", &size, &cmd_array) == 0) {
			perror("string to string list");

			goto end;
		}
		cd = cJSON_GetObjectItemCaseSensitive(p_db_json->cdsystem, cmd_array[0]);
		if (cd == NULL || cd->type != cJSON_Object) {

			goto end;
		}
		id = cJSON_GetObjectItem(cd, "id");
		type = cJSON_GetObjectItem(cd, "type");
		installation_site = cJSON_GetObjectItem(cd, "installation_site");
		x = cJSON_GetObjectItem(cd, "x");
		y = cJSON_GetObjectItem(cd, "y");
		z = cJSON_GetObjectItem(cd, "z");
		rx = cJSON_GetObjectItem(cd, "rx");
		ry = cJSON_GetObjectItem(cd, "ry");
		rz = cJSON_GetObjectItem(cd, "rz");
		if (id == NULL || type == NULL || installation_site == NULL || x == NULL || y == NULL || z == NULL || rx == NULL || ry == NULL || rz == NULL || id->valuestring == NULL || type->valuestring == NULL || installation_site->valuestring == NULL  || x->valuestring == NULL || y->valuestring == NULL || z->valuestring == NULL || rx->valuestring == NULL || ry->valuestring == NULL || rz->valuestring == NULL) {

			goto end;
		}
		sprintf(content, "%sSetToolList(%s,%s,%s,%s,%s,%s,%s,%s,%s)%s\n", head, id->valuestring, x->valuestring, y->valuestring, z->valuestring, rx->valuestring, ry->valuestring, rz->valuestring, type->valuestring, installation_site->valuestring, end_ptr);
		strcat(file_content, content);
	/* SetWobjList */
	} else if ((ptr = strstr(lua_cmd, "SetWobjList(")) && strrchr(lua_cmd, ')')) {
		end_ptr = strrchr(lua_cmd, ')') + 1;
		strncpy(head, lua_cmd, (ptr - lua_cmd));
		strncpy(cmd_arg, (ptr + 12), (end_ptr - ptr - 13));
		if (string_to_string_list(cmd_arg, ",", &size, &cmd_array) == 0) {
			perror("string to string list");

			goto end;
		}
		cd = cJSON_GetObjectItemCaseSensitive(p_db_json->wobj_cdsystem, cmd_array[0]);
		if (cd == NULL || cd->type != cJSON_Object) {

			goto end;
		}
		id = cJSON_GetObjectItem(cd, "id");
		x = cJSON_GetObjectItem(cd, "x");
		y = cJSON_GetObjectItem(cd, "y");
		z = cJSON_GetObjectItem(cd, "z");
		rx = cJSON_GetObjectItem(cd, "rx");
		ry = cJSON_GetObjectItem(cd, "ry");
		rz = cJSON_GetObjectItem(cd, "rz");
		if (id == NULL || x == NULL || y == NULL || z == NULL || rx == NULL || ry == NULL || rz == NULL || id->valuestring == NULL || x->valuestring == NULL || y->valuestring == NULL || z->valuestring == NULL || rx->valuestring == NULL || ry->valuestring == NULL || rz->valuestring == NULL) {

			goto end;
		}
		sprintf(content, "%sSetWobjList(%s,%s,%s,%s,%s,%s,%s)%s\n", head, id->valuestring, x->valuestring, y->valuestring, z->valuestring, rx->valuestring, ry->valuestring, rz->valuestring, end_ptr);
		strcat(file_content, content);
	/* SetExToolList */
	} else if ((ptr = strstr(lua_cmd, "SetExToolList(")) && strrchr(lua_cmd, ')')) {
		end_ptr = strrchr(lua_cmd, ')') + 1;
		strncpy(head, lua_cmd, (ptr - lua_cmd));
		strncpy(cmd_arg, (ptr + 14), (end_ptr - ptr - 15));
		if (string_to_string_list(cmd_arg, ",", &size, &cmd_array) == 0) {
			perror("string to string list");

			goto end;
		}
		et_cd = cJSON_GetObjectItemCaseSensitive(p_db_json->et_cdsystem, cmd_array[0]);
		if (et_cd == NULL || et_cd->type != cJSON_Object) {

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
		if (id == NULL || ex == NULL || ey == NULL || ez == NULL || erx == NULL || ery == NULL || erz == NULL || tx == NULL || ty == NULL || tz == NULL || trx == NULL || try == NULL || trz == NULL || id->valuestring == NULL || ex->valuestring == NULL || ey->valuestring == NULL || ez->valuestring == NULL || erx->valuestring == NULL || ery->valuestring == NULL || erz->valuestring == NULL || tx->valuestring == NULL || ty->valuestring == NULL || tz->valuestring == NULL || trx->valuestring == NULL || try->valuestring == NULL || trz->valuestring == NULL) {

			goto end;
		}
		sprintf(content, "%sSetExToolList(%d,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s)%s\n", head, (atoi(id->valuestring) + 14), ex->valuestring, ey->valuestring, ez->valuestring, erx->valuestring, ery->valuestring, erz->valuestring, tx->valuestring, ty->valuestring, tz->valuestring, trx->valuestring, try->valuestring, trz->valuestring, end_ptr);
		strcat(file_content, content);
	/* PostureAdjustOn */
	} else if ((ptr = strstr(lua_cmd, "PostureAdjustOn(")) && strrchr(lua_cmd, ')')) {
		end_ptr = strrchr(lua_cmd, ')') + 1;
		strncpy(head, lua_cmd, (ptr - lua_cmd));
		strncpy(cmd_arg, (ptr + 16), (end_ptr - ptr - 17));
		if (string_to_string_list(cmd_arg, ",", &size, &cmd_array) == 0 || size != 11) {
			perror("string to string list");

			goto end;
		}
		point_1 = cJSON_GetObjectItemCaseSensitive(p_db_json->point, cmd_array[1]);
		if (point_1 == NULL || point_1->type != cJSON_Object) {

			goto end;
		}
		rx = cJSON_GetObjectItem(point_1, "rx");
		ry = cJSON_GetObjectItem(point_1, "ry");
		rz = cJSON_GetObjectItem(point_1, "rz");
		if (rx == NULL || ry == NULL || rz == NULL || rx->valuestring == NULL || ry->valuestring == NULL || rz->valuestring == NULL) {

			goto end;
		}
		point_2 = cJSON_GetObjectItemCaseSensitive(p_db_json->point, cmd_array[2]);
		if (point_2 == NULL || point_2->type != cJSON_Object) {

			goto end;
		}
		rx_2 = cJSON_GetObjectItem(point_2, "rx");
		ry_2 = cJSON_GetObjectItem(point_2, "ry");
		rz_2 = cJSON_GetObjectItem(point_2, "rz");
		if (rx_2 == NULL || ry_2 == NULL || rz_2 == NULL || rx_2->valuestring == NULL || ry_2->valuestring == NULL || rz_2->valuestring == NULL) {

			goto end;
		}
		point_3 = cJSON_GetObjectItemCaseSensitive(p_db_json->point, cmd_array[3]);
		if (point_3 == NULL || point_3->type != cJSON_Object) {

			goto end;
		}
		rx_3 = cJSON_GetObjectItem(point_3, "rx");
		ry_3 = cJSON_GetObjectItem(point_3, "ry");
		rz_3 = cJSON_GetObjectItem(point_3, "rz");
		if (rx_3 == NULL || ry_3 == NULL || rz_3 == NULL || rx_3->valuestring == NULL || ry_3->valuestring == NULL || rz_3->valuestring == NULL) {

			goto end;
		}
		sprintf(content, "%sPostureAdjustOn(%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s)%s\n", head, cmd_array[0], rx->valuestring, ry->valuestring, rz->valuestring, rx_2->valuestring, ry_2->valuestring, rz_2->valuestring, rx_3->valuestring, ry_3->valuestring, rz_3->valuestring, cmd_array[4], cmd_array[5], cmd_array[6], cmd_array[7], cmd_array[8], cmd_array[9], cmd_array[10], end_ptr);
		strcat(file_content, content);
	/* RegisterVar */
	} else if ((ptr = strstr(lua_cmd, "RegisterVar(")) && strrchr(lua_cmd, ')')) {
		end_ptr = strrchr(lua_cmd, ')') + 1;
		strncpy(head, lua_cmd, (ptr - lua_cmd));
		strncpy(cmd_arg, (ptr + 12), (end_ptr - ptr - 13));
		if (string_to_string_list(cmd_arg, ",", &size, &cmd_array) == 0 || (size < 1 || size > 6)) {
			perror("string to string list");

			goto end;
		}
		/* 一个参数时 */
		if (size == 1) {
			sprintf(content, "%sRegisterVar(\"%s\")%s\n", head, cmd_array[0], end_ptr);
		/* 多个参数时 */
		} else {
			memset(content, 0, MAX_BUF);
			sprintf(content, "%sRegisterVar(\"%s\",", head, cmd_array[0]);
			strcat(file_content, content);
			for (i = 1; i < (size - 1); i++) {
				memset(content, 0, MAX_BUF);
				sprintf(content, "\"%s\",", cmd_array[i]);
				strcat(file_content, content);
			}
			memset(content, 0, MAX_BUF);
			sprintf(content, "\"%s\")%s\n", cmd_array[size - 1], end_ptr);
		}
		strcat(file_content, content);
	/* SetSysVarValue */
	} else if ((ptr = strstr(lua_cmd, "SetSysVarValue(")) && strrchr(lua_cmd, ')')) {
		end_ptr = strrchr(lua_cmd, ')') + 1;
		strncpy(head, lua_cmd, (ptr - lua_cmd));
		strncpy(cmd_arg, (ptr + 15), (end_ptr - ptr - 16));
		if (string_to_string_list(cmd_arg, ",", &size, &cmd_array) == 0 || size != 2) {
			perror("string to string list");

			goto end;
		}
		var = cJSON_GetObjectItemCaseSensitive(p_db_json->sysvar, cmd_array[0]);
		if (var == NULL || var->type != cJSON_Object) {

			goto end;
		}
		id = cJSON_GetObjectItem(var, "id");
		if (id == NULL) {

			goto end;
		}
		sprintf(content, "%sSetSysVarValue(%s,%s)%s\n", head, id->valuestring, cmd_array[1], end_ptr);
		strcat(file_content, content);
	/* GetSysVarValue */
	} else if ((ptr = strstr(lua_cmd, "GetSysVarValue(")) && strrchr(lua_cmd, ')')) {
		end_ptr = strrchr(lua_cmd, ')') + 1;
		strncpy(head, lua_cmd, (ptr - lua_cmd));
		strncpy(cmd_arg, (ptr + 15), (end_ptr - ptr - 16));
		if (string_to_string_list(cmd_arg, ",", &size, &cmd_array) == 0) {
			perror("string to string list");

			goto end;
		}
		var = cJSON_GetObjectItemCaseSensitive(p_db_json->sysvar, cmd_array[0]);
		if (var == NULL || var->type != cJSON_Object) {

			goto end;
		}
		id = cJSON_GetObjectItem(var, "id");
		if (id == NULL) {

			goto end;
		}
		sprintf(content, "%sGetSysVarValue(%s)%s\n", head, id->valuestring, end_ptr);
		strcat(file_content, content);
	/* MultilayerOffsetTrsfToBase */
	} else if ((ptr = strstr(lua_cmd, "MultilayerOffsetTrsfToBase(")) && strrchr(lua_cmd, ')')) {
		end_ptr = strrchr(lua_cmd, ')') + 1;
		strncpy(head, lua_cmd, (ptr - lua_cmd));
		strncpy(cmd_arg, (ptr + 27), (end_ptr - ptr - 28));
		if (string_to_string_list(cmd_arg, ",", &size, &cmd_array) == 0 || size != 6) {
			perror("string to string list");

			goto end;
		}
		point_1 = cJSON_GetObjectItemCaseSensitive(p_db_json->point, cmd_array[0]);
		if (point_1 == NULL || point_1->type != cJSON_Object) {

			goto end;
		}
		x = cJSON_GetObjectItem(point_1, "x");
		y = cJSON_GetObjectItem(point_1, "y");
		z = cJSON_GetObjectItem(point_1, "z");
		if (x == NULL || y == NULL || z == NULL) {

			goto end;
		}
		point_2 = cJSON_GetObjectItemCaseSensitive(p_db_json->point, cmd_array[1]);
		if (point_2 == NULL || point_2->type != cJSON_Object) {

			goto end;
		}
		x_2 = cJSON_GetObjectItem(point_2, "x");
		y_2 = cJSON_GetObjectItem(point_2, "y");
		z_2 = cJSON_GetObjectItem(point_2, "z");
		if (x_2 == NULL || y_2 == NULL || z_2 == NULL) {

			goto end;
		}
		point_3 = cJSON_GetObjectItemCaseSensitive(p_db_json->point, cmd_array[2]);
		if (point_3 == NULL || point_3->type != cJSON_Object) {

			goto end;
		}
		x_3 = cJSON_GetObjectItem(point_3, "x");
		y_3 = cJSON_GetObjectItem(point_3, "y");
		z_3 = cJSON_GetObjectItem(point_3, "z");
		if (x_3 == NULL || y_3 == NULL || z_3 == NULL) {

			goto end;
		}
		sprintf(content, "%sMultilayerOffsetTrsfToBase(%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s)%s\n", head, x->valuestring, y->valuestring, z->valuestring, x_2->valuestring, y_2->valuestring, z_2->valuestring, x_3->valuestring, y_3->valuestring, z_3->valuestring, cmd_array[3], cmd_array[4], cmd_array[5], end_ptr);
		strcat(file_content, content);
	/* other code send without processing */
	} else {
		strcat(file_content, lua_cmd);
		strcat(file_content, "\n");
	}
	//printf("file_content = %s\n", file_content);
	string_list_free(cmd_array, size);

	return SUCCESS;

end:
	string_list_free(cmd_array, size);
	return FAIL;
}

/* 106 sendFile */
static int sendfile(const cJSON *data_json, char *content)
{
	const char s[2] = "\n";
	char *token = NULL;
	DB_JSON db_json = {
		.point = NULL,
		.cdsystem = NULL,
		.wobj_cdsystem = NULL,
		.et_cdsystem = NULL,
		.sysvar = NULL,
	};
	//int i = 0;

	cJSON *pgvalue = cJSON_GetObjectItem(data_json, "pgvalue");
	if (pgvalue == NULL || pgvalue->valuestring == NULL || !strcmp(pgvalue->valuestring, "")) {
		perror("json");

		return FAIL;
	}

	//printf("pgvalue->valuestring = %s\n", pgvalue->valuestring);
	/** 如果是内嵌脚本 */
	if (is_in(lua_filename, "Embedded_") == 1) {
		strcpy(content, pgvalue->valuestring);
	/** 如果是程序编辑脚本 */
	} else {
		if (init_db_json(&db_json) == FAIL) {
			perror("get database json");
			db_json_delete(&db_json);

			return FAIL;
		}

		/* get first line */
		token = strtok(pgvalue->valuestring, s);
		while (token != NULL) {
			//clock_t time_1, time_2, time_3, time_4, time_6;

			//time_1 = clock();
			//printf("time_1, %d\n", time_1);
			//printf("i = %d\n", i);
			//i++;
			//printf("token = %s\n", token);
			if (parse_lua_cmd(token, content, &db_json) == FAIL) {
				perror("parse lua cmd");
				db_json_delete(&db_json);

				return FAIL;
			}
			/* get other line */
			token = strtok(NULL, s);
			//time_6 = clock();
			//printf("time_6, %d\n", time_6);
		}
		db_json_delete(&db_json);
	}

	//printf("content = %s\n", content);
	//printf("strlen content = %d\n", strlen(content));
	if (write_file(lua_filename, content) == FAIL) {
		perror("write file");

		return FAIL;
	}

	/** 如果是内嵌脚本, 检查 lua 文件内容合法性 */
//	if (is_in(lua_filename, "Embedded_") == 1) {

		return check_lua_file();
//	} else {
//
//		return SUCCESS;
//	}
}

/* 1001 step over */
static int step_over(const cJSON *data_json, char *content)
{
	int cmd = 0;
	DB_JSON db_json = {
		.point = NULL,
		.cdsystem = NULL,
		.wobj_cdsystem = NULL,
		.et_cdsystem = NULL,
		.sysvar = NULL,
	};

	cJSON *pgvalue = cJSON_GetObjectItem(data_json, "pgline");
	if (pgvalue == NULL || pgvalue->valuestring == NULL || !strcmp(pgvalue->valuestring, "")) {
		perror("json");

		return FAIL;
	}
	//printf("upload lua cmd:%s\n", pgvalue->valuestring);
	/* PTP */
	if (strstr(pgvalue->valuestring, "PTP")) {
		cmd = 201;
	/* ARC */
	} else if (strstr(pgvalue->valuestring, "ARC")) {
		cmd = 202;
	/* Lin */
	} else if (strstr(pgvalue->valuestring, "Lin")) {
		cmd = 203;
	/* set DO */
	} else if (strstr(pgvalue->valuestring, "SetDO")) {
		cmd = 204;
	/* wait ms */
	} else if (strstr(pgvalue->valuestring, "WaitMs")) {
		cmd = 207;
	/* set AO */
	} else if (strstr(pgvalue->valuestring, "SetAO")) {
		cmd = 209;
	/* set ToolDO */
	} else if (strstr(pgvalue->valuestring, "SetToolDO")) {
		cmd = 210;
	/* set ToolAO */
	} else if (strstr(pgvalue->valuestring, "SetToolAO")) {
		cmd = 211;
	/* waitDI */
	} else if (strstr(pgvalue->valuestring, "WaitDI")) {
		cmd = 218;
	/* waitToolDI */
	} else if (strstr(pgvalue->valuestring, "WaitToolDI")) {
		cmd = 219;
	/* waitAI */
	} else if (strstr(pgvalue->valuestring, "WaitAI")) {
		cmd = 220;
	/* waitToolAI */
	} else if (strstr(pgvalue->valuestring, "WaitToolAI")) {
		cmd = 221;
	/* ActGripper */
	} else if (strstr(pgvalue->valuestring, "ActGripper")) {
		cmd = 227;
	/* MoveGripper */
	} else if (strstr(pgvalue->valuestring, "MoveGripper")) {
		cmd = 228;
	/* SprayStart */
	} else if (strstr(pgvalue->valuestring, "SprayStart")) {
		cmd = 236;
	/* SprayStop */
	} else if (strstr(pgvalue->valuestring, "SprayStop")) {
		cmd = 237;
	/* PowerCleanStart */
	} else if (strstr(pgvalue->valuestring, "PowerCleanStart")) {
		cmd = 238;
	/* PowerCleanStop */
	} else if (strstr(pgvalue->valuestring, "PowerCleanStop")) {
		cmd = 239;
	/* ARCStart */
	} else if (strstr(pgvalue->valuestring, "ARCStart")) {
		cmd = 247;
	/* ARCEnd */
	} else if (strstr(pgvalue->valuestring, "ARCEnd")) {
		cmd = 248;
	/* LTLaserOn */
	} else if (strstr(pgvalue->valuestring, "LTLaserOn")) {
		cmd = 255;
	/* LTLaserOff */
	} else if (strstr(pgvalue->valuestring, "LTLaserOff")) {
		cmd = 256;
	/* LoadPosSensorDriver */
	} else if (strstr(pgvalue->valuestring, "LoadPosSensorDriver")) {
		cmd = 265;
	/* UnloadPosSensorDriver */
	} else if (strstr(pgvalue->valuestring, "UnloadPosSensorDriver")) {
		cmd = 266;
	/* ExtAxisSetHoming */
	} else if (strstr(pgvalue->valuestring, "ExtAxisSetHoming")) {
		cmd = 290;
	/* ExtAxisServoOn */
	} else if (strstr(pgvalue->valuestring, "ExtAxisServoOn")) {
		cmd = 296;
	/* EXT_AXIS_PTP */
	} else if (strstr(pgvalue->valuestring, "EXT_AXIS_PTP")) {
		cmd = 297;
	/* Mode */
	} else if (strstr(pgvalue->valuestring, "Mode")) {
		cmd = 303;
	/* SetToolList */
	} else if (strstr(pgvalue->valuestring, "SetToolList")) {
		cmd = 319;
	/* SetWobjList */
	} else if (strstr(pgvalue->valuestring, "SetWobjList")) {
		cmd = 383 ;
	/* SetExToolCoord */
	} else if (strstr(pgvalue->valuestring, "SetExToolList")) {
		cmd = 331;
	/* soft-PLC setDO */
	} else if (strstr(pgvalue->valuestring, "SPLCSetDO")) {
		cmd = 394;
	/* soft-PLC setToolDO */
	} else if (strstr(pgvalue->valuestring, "SPLCSetToolDO")) {
		cmd = 395;
	/* soft-PLC setAO */
	} else if (strstr(pgvalue->valuestring, "SPLCSetAO")) {
		cmd = 396;
	/* soft-PLC setToolAO */
	} else if (strstr(pgvalue->valuestring, "SPLCSetToolAO")) {
		cmd = 397;
	/* SetSysVarValue */
	} else if (strstr(pgvalue->valuestring, "SetSysVarValue")) {
		cmd = 511;
	/* GetSysVarValue */
	} else if (strstr(pgvalue->valuestring, "GetSysVarValue")) {
		cmd = 512;
	/* Circle */
	} else if (strstr(pgvalue->valuestring, "Circle")) {
		cmd = 540;
	/* error */
	} else {
		return FAIL;
	}

	if (init_db_json(&db_json) == FAIL) {
		perror("get database json");
		db_json_delete(&db_json);

		return FAIL;
	}
	if (parse_lua_cmd(pgvalue->valuestring, content, &db_json) == FAIL) {
		perror("parse lua cmd");
		db_json_delete(&db_json);

		return FAIL;
	}
	db_json_delete(&db_json);

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
	printf("state->workPieceNum = %d\n", state->workPieceNum);

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
	sprintf(content, "MoveJ(%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%d,%d,%s,%s,%s,%lf,%lf,%lf,%lf,0,0,0,0,0,0,0,0)", j1->valuestring, j2->valuestring, j3->valuestring, j4->valuestring, j5->valuestring, j6->valuestring, x->valuestring, y->valuestring, z->valuestring, rx->valuestring, ry->valuestring, rz->valuestring, state->toolNum, state->workPieceNum, speed->valuestring, acc->valuestring, ovl->valuestring, double_round(state->exaxis_status[0].exAxisPos, 3), double_round(state->exaxis_status[1].exAxisPos, 3), double_round(state->exaxis_status[2].exAxisPos, 3), double_round(state->exaxis_status[3].exAxisPos, 3));

	return SUCCESS;
}

/* 230 set_state_id */
static int set_state_id(const cJSON *data_json, char *content)
{
	int icount = 0;
	int i;
	cJSON *id_num = NULL;
	cJSON *id = cJSON_GetObjectItem(data_json, "id");
	cJSON *type = cJSON_GetObjectItem(data_json, "type");
	if (id == NULL || type == NULL) {
		perror("json");

		return FAIL;
	}
	//printf("id = %s\n", cJSON_Print(id));
	//printf("type = %d\n", type->valueint);
	state_fb.icount = cJSON_GetArraySize(id); /*获取数组长度*/
	state_fb.type = type->valueint; /*获取type*/
	//printf("state_fb.iCount= %d\n", state_fb.icount);

	/* empty state_fb id */
	for (i = 0; i < STATEFB_ID_MAXNUM; i++) {
		state_fb.id[i] = 0;
	}
	for (i = 0; i < state_fb.icount; i++) {
		id_num = cJSON_GetArrayItem(id, i); /* 目前按1笔处理, 取出一笔放入 state_fb.id */
		//printf("string, state_fb.id[%d] = %s\n", i, id_num->valuestring);
		state_fb.id[i] = atoi(id_num->valuestring);
		//printf("array , state_fb.id[%d] = %d\n", i, state_fb.id[i]);
	}
	sprintf(content, "SetCTLStateQueryParam(%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d)", state_fb.icount, state_fb.id[0], state_fb.id[1], state_fb.id[2], state_fb.id[3], state_fb.id[4], state_fb.id[5], state_fb.id[6], state_fb.id[7], state_fb.id[8], state_fb.id[9], state_fb.id[10], state_fb.id[11], state_fb.id[12], state_fb.id[13], state_fb.id[14], state_fb.id[15], state_fb.id[16], state_fb.id[17], state_fb.id[18], state_fb.id[19]);

	return SUCCESS;
}

/* 231 set_state */
static int set_state(const cJSON *data_json, char *content)
{
	cJSON *flag = cJSON_GetObjectItem(data_json, "flag");
	char *src_buf = NULL;
	int ret = 0;

	if(flag == NULL || flag->valuestring == NULL) {
		perror("json");

		return FAIL;
	}
	/** clear state quene */
	pthread_mutex_lock(&socket_state.mute);
	fb_clearquene(&fb_quene);
	pthread_mutex_unlock(&socket_state.mute);

	/** 开始查询，清空文件 */
	if (strcmp(flag->valuestring, "0") == 1) {
		/** clear statefb.txt */
		if (state_fb.type == 1) {
			ret = open(FILE_STATEFB, O_WRONLY | O_TRUNC);
			if (ret != -1) {
				close(ret);
			}
		}
		/** clear statefb10.txt && clear index */
		if (state_fb.type == 2 || state_fb.type == 3) {
			ret = open(FILE_STATEFB10, O_WRONLY | O_TRUNC);
			if (ret != -1) {
				close(ret);
			}
			state_fb.index = 0;
		}
	}

	/** 停止查询，写入文件 */
	if(strcmp(flag->valuestring, "0") == 0 && (state_fb.type == 2 || state_fb.type == 3)) {
		//clock_t time_6, time_7, time_8, time_9;
		//time_6 = clock();
		//printf("time_6, %d\n", time_6);
		src_buf = (char *)calloc(1, sizeof(char)*(STATEFB_BUFSIZE+1));
		if (src_buf == NULL) {
			perror("calloc\n");

			return FAIL;
		}
		//time_7 = clock();
		//printf("time_7, %d\n", time_7);
		//printf("strlen state_fb.buf = %d\n", strlen(state_fb.buf));
		strcpy(src_buf, state_fb.buf);
		//time_8 = clock();
		//printf("time_8, %d\n", time_8);
		//double duration;
		//clock_t start, finish;
		//start = clock();
		//printf("before write, %d\n", start);
		if (write_file(FILE_STATEFB10, src_buf) == FAIL) {
			perror("write file content");
		}
		//time_9 = clock();
		//printf("time_9, %d\n", time_9);
		//inish = clock();
		//printf("after write, %d\n", finish);
		//duration = (double)(finish - start) / CLOCKS_PER_SEC;
		//printf("run time is %f seconds\n", duration);
		free(src_buf);
		src_buf = NULL;
	}

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

/* 339 340 SetPluginDO DI or clear function DI DO config */
static int set_plugin_dio(const cJSON *data_json, char *content, int dio)
{
	cJSON *config_json = NULL;
	cJSON *plugin_name = NULL;
	cJSON *func_name = NULL;
	cJSON *value = NULL;
	cJSON *enable = NULL;
	cJSON *level = NULL;
	cJSON *func_json = NULL;
	cJSON *func_name_json = NULL;
	cJSON *dio_json = NULL;
	cJSON *dio_value_json = NULL;
	cJSON *dio_level_json = NULL;
	cJSON *newitem = NULL;
	SOCKET_INFO *sock_cmd = NULL;
	char config_path[100] = {0};
	char *config_content = NULL;
	char *buf = NULL;
	int write_ret = FAIL;
	int i = 0;
	char socket_send_content[100] = {0};

	plugin_name = cJSON_GetObjectItem(data_json, "plugin_name");
	func_name = cJSON_GetObjectItem(data_json, "func_name");
	value = cJSON_GetObjectItem(data_json, "value");
	enable = cJSON_GetObjectItem(data_json, "enable");
	level = cJSON_GetObjectItem(data_json, "level");

	if (plugin_name == NULL || func_name == NULL || value == NULL || enable == NULL || level == NULL || plugin_name->type != cJSON_String || func_name->type != cJSON_String || value->type != cJSON_Number || enable->type != cJSON_Number || level->type != cJSON_Number) {
		perror("json");

		return FAIL;
	}
	/* send set plugin DI DO cmd or clear plugin DI DO cmd */
	if (dio == 1) {
		sprintf(content, "SetPluginDO(%d,%d,%d)", value->valueint, enable->valueint, level->valueint);
	} else {
		sprintf(content, "SetPluginDI(%d,%d,%d)", value->valueint, enable->valueint, level->valueint);
	}

	sprintf(config_path, "%s%s/config.json", UPLOAD_WEB_PLUGINS, plugin_name->valuestring);
	config_content = get_file_content(config_path);
	if (config_content == NULL || strcmp(config_content, "NO_FILE") == 0 || strcmp(config_content, "Empty") == 0) {
		perror("get file content");

		return FAIL;
	}
	//printf("config_content = %s\n", config_content);
	config_json = cJSON_Parse(config_content);
	free(config_content);
	config_content = NULL;
	if (config_json == NULL || config_json->type != cJSON_Array) {
		perror("cJSON_Parse");

		return FAIL;
	}
	for (i = 0; i < cJSON_GetArraySize(config_json); i++) {
		func_json = cJSON_GetArrayItem(config_json, i);
		func_name_json = cJSON_GetObjectItem(func_json, "func_name");
		dio_json = cJSON_GetObjectItem(func_json, "dio");
		dio_value_json = cJSON_GetObjectItem(func_json, "dio_value");
		dio_level_json = cJSON_GetObjectItem(func_json, "dio_level");
		if (dio_json == NULL || dio_value_json == NULL || func_name_json == NULL || func_name_json->type != cJSON_String || dio_value_json == NULL || dio_value_json->type != cJSON_Number || dio_level_json == NULL || dio_level_json->type != cJSON_Number ) {
			perror("json");
			cJSON_Delete(config_json);
			config_json = NULL;

			return FAIL;
		}
		if (strcmp(func_name_json->valuestring, func_name->valuestring) == 0) {
			/** 之前已经配置过 DI、DO 的功能, remove 外设插件配置文件 config.json 中的 object */
			if (enable->valueint == 0) {
				cJSON_DeleteItemFromArray(config_json, i);
			} else {
				if (robot_type == 1) { // "1" 代表实体机器人
					sock_cmd = &socket_cmd;
				} else { // "0" 代表虚拟机器人
					sock_cmd = &socket_vir_cmd;
				}
				/* send clear configed plugin DO cmd */
				if (dio_json->valueint == 1) {
					sprintf(socket_send_content, "SetPluginDO(%d,%d,%d)", dio_value_json->valueint, 0, dio_level_json->valueint);
					socket_enquene(sock_cmd, 339, socket_send_content, 1);
				/* send clear configed plugin DI cmd */
				} else {
					sprintf(socket_send_content, "SetPluginDI(%d,%d,%d)", dio_value_json->valueint, 0, dio_level_json->valueint);
					socket_enquene(sock_cmd, 340, socket_send_content, 1);
				}
				/** 之前已经配置过 DI、DO 的功能, 修改外设插件配置文件 config.json 中的 object */
				cJSON_ReplaceItemInObject(func_json, "dio", cJSON_CreateNumber(dio));
				cJSON_ReplaceItemInObject(func_json, "dio_value", cJSON_CreateNumber(value->valueint));
				cJSON_ReplaceItemInObject(func_json, "dio_level", cJSON_CreateNumber(level->valueint));
			}

			buf = cJSON_Print(config_json);
			//printf("buf = %s\n", buf);
			write_ret = write_file(config_path, buf);//write file
			free(buf);
			buf = NULL;
			cJSON_Delete(config_json);
			config_json = NULL;
			if (write_ret == FAIL) {
				perror("write file");

				return FAIL;
			}

			return SUCCESS;
		}
	}
	/** 之前没有配置过 DI、DO 的功能, 插入 object 到外设插件配置文件 config.json 中 */
	newitem = cJSON_CreateObject();
	cJSON_AddStringToObject(newitem, "func_name", func_name->valuestring);
	cJSON_AddNumberToObject(newitem, "dio", dio);
	cJSON_AddNumberToObject(newitem, "dio_value", value->valueint);
	cJSON_AddNumberToObject(newitem, "dio_level", level->valueint);
	cJSON_AddItemToArray(config_json, newitem);

	buf = cJSON_Print(config_json);
	write_ret = write_file(config_path, buf);//write file
	free(buf);
	buf = NULL;
	cJSON_Delete(config_json);
	config_json = NULL;
	if (write_ret == FAIL) {
		perror("write file");

		return FAIL;
	}

	return SUCCESS;
}

/* 401 get_program_array */
static int get_program_array(const cJSON *data_json, char *content)
{
	strcpy(content, "TorqueSysGetProgramArray()");

	return SUCCESS;
}

/* 402 get_program_content */
static int get_program_content(const cJSON *data_json, char *content)
{
	cJSON *program_name = cJSON_GetObjectItem(data_json, "program_name");

	if (program_name == NULL || program_name->valuestring == NULL) {
		perror("json");

		return FAIL;
	}
	sprintf(content, "TorqueSysGetProgramContent(\"%s\")", program_name->valuestring);

	return SUCCESS;
}

/* 403 save_program */
static int save_program(const cJSON *data_json, char *content)
{
	char *buf = NULL;

	buf = cJSON_Print(data_json);
	if (buf == NULL) {
		perror("json");

		return FAIL;
	}
	sprintf(content, "TorqueSysSaveProgram('%s')", buf);
	free(buf);
	buf = NULL;

	return SUCCESS;
}

/* 404 upload_program */
static int upload_program(const cJSON *data_json, char *content)
{
	cJSON *program_name = cJSON_GetObjectItem(data_json, "program_name");

	if (program_name == NULL || program_name->valuestring == NULL) {
		perror("json");

		return FAIL;
	}
	sprintf(content, "TorqueSysUploadProgram(\"%s\")", program_name->valuestring);

	return SUCCESS;
}

/* 405 get_control_mode */
static int get_control_mode(const cJSON *data_json, char *content)
{
	strcpy(content, "TorqueSysGetControlMode()");

	return SUCCESS;
}

/* 406 set_control_mode */
static int set_control_mode(const cJSON *data_json, char *content)
{
	cJSON *control_mode = cJSON_GetObjectItem(data_json, "control_mode");

	if (control_mode == NULL) {
		perror("json");

		return FAIL;
	}
	sprintf(content, "TorqueSysSetControlMode(%d)", control_mode->valueint);

	return SUCCESS;
}

/* 407 test_tighten */
static int test_tighten(const cJSON *data_json, char *content)
{
	strcpy(content, "TorqueSysTestTighten()");

	return SUCCESS;
}

/* 408 test_unscrew */
static int test_unscrew(const cJSON *data_json, char *content)
{
	strcpy(content, "TorqueSysTestUnscrew()");

	return SUCCESS;
}

/* 409 test_free */
static int test_free(const cJSON *data_json, char *content)
{
	strcpy(content, "TorqueSysTestFree()");

	return SUCCESS;
}

/* 410 test_stop */
static int test_stop(const cJSON *data_json, char *content)
{
	strcpy(content, "TorqueSysTestStop()");

	return SUCCESS;
}

/* 411 set_on_off */
static int set_on_off(const cJSON *data_json, char *content)
{
	cJSON *enable = cJSON_GetObjectItem(data_json, "enable");

	if (enable == NULL) {
		perror("json");

		return FAIL;
	}
	if (enable->valueint == 0) {
		/* 关闭 socket thread */
		if (torquesys.enable == 1) {
			torquesys.enable = 0;
			/** TODO cancel pthread */
			//pthread_cancel(torquesys.t_socket_TORQUE_SYS);
			//printf("start pthread_join(torquesys.t_socket_TORQUE_SYS, NULL)\n");
			/* 当前线程挂起, 等待创建线程返回，获取该线程的返回值后，当前线程退出 */
			if (pthread_join(torquesys.t_socket_TORQUE_SYS, NULL)) {
				perror("pthread_join");

				return FAIL;
			}
			//printf("after pthread_join\n");
		}
	}
	if (enable->valueint == 1) {
		/* 开启 socket thread */
		if (torquesys.enable == 0) {
			torquesys.enable = 1;
			//printf("before pthread_create\n");
			/* create socket PCI thread */
			if (pthread_create(&torquesys.t_socket_TORQUE_SYS, NULL, (void*)&socket_TORQUE_SYS_thread, (void *)TORQUE_PORT)) {
				perror("pthread_create");

				return FAIL;
			}
			//printf("after pthread_create\n");
		}

	}
	sprintf(content, "TorqueSysSetOnOff(%d)", enable->valueint);

	return SUCCESS;
}

/* 412 get_on_off */
static int get_on_off(const cJSON *data_json, char *content)
{
	strcpy(content, "TorqueSysGetOnOff()");

	return SUCCESS;
}

/* 413 get_current_range */
static int get_current_range(const cJSON *data_json, char *content)
{
	cJSON *current_product = cJSON_GetObjectItem(data_json, "current_product");

	if (current_product == NULL || current_product->valuestring == NULL) {
		perror("json");

		return FAIL;
	}
	sprintf(content, "TorqueSysGetCurrentRange(\"%s\")", current_product->valuestring);

	return SUCCESS;
}

/* 414 get_current_product */
static int get_current_product(const cJSON *data_json, char *content)
{
	strcpy(content, "TorqueSysGetCurrentProduct()");

	return SUCCESS;
}

/* 415 set_torque_unit */
static int set_torque_unit(const cJSON *data_json, char *content)
{
	cJSON *torque_unit = cJSON_GetObjectItem(data_json, "torque_unit");

	if (torque_unit == NULL) {
		perror("json");

		return FAIL;
	}
	sprintf(content, "TorqueSysSetTorqueUnit(%d)", torque_unit->valueint);

	return SUCCESS;
}

/* 425 set robot type */
static int set_robot_type(const cJSON *data_json, char * content)
{
	int robot_type = 0;
	cJSON *password = NULL;
	cJSON *content_json = NULL;
	cJSON *type = NULL;
	cJSON *major_ver = NULL;
	cJSON *minor_ver = NULL;

	password = cJSON_GetObjectItem(data_json, "pwd");
	if (password == NULL || password->valuestring == NULL) {
		perror("json");

		return FAIL;
	}

	if (strcmp(password->valuestring, RTS_PASSWORD) == 0) {
		content_json = cJSON_GetObjectItem(data_json, "content");
		type = cJSON_GetObjectItem(content_json, "type");
		major_ver = cJSON_GetObjectItem(content_json, "major_ver");
		minor_ver = cJSON_GetObjectItem(content_json, "minor_ver");
		if (type == NULL || major_ver == NULL || minor_ver == NULL) {
			perror("json");

			return FAIL;
		}
		/** 主版本号预留 10 个 (1~10)，次版本号预留 10 个 (0~9) */
		robot_type = (type->valueint - 1) * 100 + (major_ver->valueint - 1) * 10 + (minor_ver->valueint + 1);

		sprintf(content, "SetRobotType(%d)", robot_type);

		return SUCCESS;
	} else {

		return PWD_FAIL;
	}
}

static int wait_cmd_feedback()
{
	Qnode *p = NULL;
	int i = 0;
	int ret = FAIL;
	SOCKET_INFO *sock_cmd = NULL;

	if (robot_type == 1) { // "1" 代表实体机器人
		sock_cmd = &socket_cmd;
	} else { // "0" 代表虚拟机器人
		sock_cmd = &socket_vir_cmd;
	}
	//printf("cmd = %d\n", cmd);
	for (i = 0; i < 1000; i++) {
		//printf("i = %d\n", i);
		p = sock_cmd->ret_quene.front->next;
		while (p != NULL) {
			//printf("p->data.type = %d\n", p->data.type);
			if (p->data.type == 425) {
				//strcpy(p->data.msgcontent, "1");
				//printf("p->data.msgcontent = %s\n", p->data.msgcontent);
				if (strcmp(p->data.msgcontent, "1") == 0) {

					ret = SUCCESS;
				}
				/* 删除结点信息 */
				pthread_mutex_lock(&sock_cmd->ret_mute);
				dequene(&sock_cmd->ret_quene, p->data);
				pthread_mutex_unlock(&sock_cmd->ret_mute);

				return ret;
			}
			p = p->next;
		}
		usleep(1000);
	}

	return ret;
}

/* get lua content size */
static int get_lua_content_size(const cJSON *data_json)
{
	int line_num = 1;
	int content_size = 0;
	char *tmp_file = NULL;
	const char s = '\n';

	cJSON *pgvalue = cJSON_GetObjectItem(data_json, "pgvalue");
	if (pgvalue == NULL || pgvalue->valuestring == NULL || !strcmp(pgvalue->valuestring, "")) {
		perror("json");

		return FAIL;
	}
	//printf("upload lua file content:%s\n", pgvalue->valuestring);
	tmp_file = pgvalue->valuestring;
	while (*tmp_file) {
		if (*tmp_file == s) {
			line_num++;
		}
		tmp_file++;
	}

	//printf("line_num = %d\n", line_num);

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
	char log_content[1024] = {0};
	char en_log_content[1024] = {0};
	char jap_log_content[1024] = {0};

	memset(error_info, 0, ERROR_SIZE);
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
	content = (char *)calloc(1, sizeof(char) * MAX_BUF);
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
	// cmd_auth "0"
	if (cmd == 302 || cmd == 305 || cmd == 308 || cmd == 309 || cmd == 312 || cmd == 332 || cmd == 337 || cmd == 343 || cmd == 391 || cmd == 425 || cmd == 541 || cmd == 542 || cmd == 543 || cmd == 544) {
		if (!authority_management("0")) {
			perror("authority_management");

			goto auth_end;
		}
	// cmd_auth "1"
	} else if (cmd == 201 || cmd == 203 || cmd == 204 || cmd == 208 || cmd == 209 || cmd == 210 || cmd == 211 || cmd == 216 || cmd == 222 || cmd == 223 || cmd == 224 || cmd == 225 || cmd == 226 || cmd == 230 || cmd == 231 || cmd == 232 || cmd == 233 || cmd == 234 || cmd == 235 || cmd == 236 || cmd == 237 || cmd == 238 || cmd == 239 || cmd == 240 || cmd == 247 || cmd == 248 || cmd == 249 || cmd == 250 || cmd == 251 || cmd == 252 || cmd == 253 || cmd == 254 || cmd == 255 || cmd == 256 || cmd == 257 || cmd == 258 || cmd == 259 || cmd == 260 || cmd == 261 || cmd == 262 || cmd == 263 || cmd == 264 || cmd == 265 || cmd == 266 || cmd == 267 || cmd == 268 || cmd == 269 ||  cmd == 270 || cmd == 271 || cmd == 272 || cmd == 273 || cmd == 274 || cmd == 276 || cmd == 277 || cmd == 278 || cmd == 279 || cmd == 280 || cmd == 283 || cmd == 287 || cmd == 288 || cmd == 289 || cmd == 290 || cmd == 291 || cmd == 292 || cmd == 293 || cmd == 294 || cmd == 295 || cmd == 296 || cmd == 297 || cmd == 298 || cmd == 306 || cmd == 307 || cmd == 313 || cmd == 314 || cmd == 315 || cmd == 317 || cmd == 318 || cmd == 320 || cmd == 323 || cmd == 324 || cmd == 325 || cmd == 326 || cmd == 327 || cmd == 328 || cmd == 329 || cmd == 334 || cmd == 335 || cmd == 336 || cmd == 339 || cmd == 340 || cmd == 341 || cmd == 353 || cmd == 354 || cmd == 355 || cmd == 356 || cmd == 357 || cmd == 358 || cmd == 359 || cmd == 360 || cmd == 361 || cmd == 362 || cmd == 367 || cmd == 368 || cmd == 369 || cmd == 370 || cmd == 371 || cmd == 372 || cmd == 376 || cmd == 380 || cmd == 381 || cmd == 382 || cmd == 384 || cmd == 386 || cmd == 387 || cmd == 388 || cmd == 389 || cmd == 390 || cmd == 393 || cmd == 403 || cmd == 404 || cmd == 406 || cmd == 407 || cmd == 408 || cmd == 409 || cmd == 410 || cmd == 411 || cmd == 415 || cmd == 422 || cmd == 426 || cmd == 430 || cmd == 431 || cmd == 432 || cmd == 433 || cmd == 434 || cmd == 435 || cmd == 436 || cmd == 511 || cmd == 523 || cmd == 524 || cmd == 525 || cmd == 526 || cmd == 528 || cmd == 529 || cmd == 530 || cmd == 531 || cmd == 532) {
		if (!authority_management("1")) {
			perror("authority_management");

			goto auth_end;
		}
	// cmd_auth "2"
	} else if (cmd == 101 || cmd == 102 || cmd == 103 || cmd == 104 || cmd == 105 || cmd == 106 || cmd == 107 || cmd == 206 || cmd == 227 || cmd == 229 || cmd == 303 || cmd == 316 || cmd == 321 || cmd == 330 || cmd == 333 || cmd == 338 || cmd == 345 || cmd == 375 || cmd == 400 || cmd == 401 || cmd == 402 || cmd == 405 || cmd == 412 || cmd == 413 || cmd == 414 || cmd == 423 || cmd == 424 || cmd == 429 || cmd == 527 || cmd == 1001) {
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
		ret = sendfile(data_json, content);
		break;
	case 101:
		port = cmdport;
		cmd_type = 0;
		strcpy(log_content, "开始程序示教");
		strcpy(en_log_content, "Begin program teaching");
		strcpy(jap_log_content, "プログラムを教えてください");
		ret = program_start(data_json, content);
		break;
	case 102:
		port = cmdport;
		cmd_type = 0;
		strcpy(log_content, "停止程序示教");
		strcpy(en_log_content, "Stop program teaching");
		strcpy(jap_log_content, "プログラムの教示を停止する");
		ret = program_stop(data_json, content);
		break;
	case 103:
		port = cmdport;
		cmd_type = 0;
		strcpy(log_content, "暂停程序示教");
		strcpy(en_log_content, "Pause program teaching");
		strcpy(jap_log_content, "プログラムを一時停止して教える");
		ret = program_pause(data_json, content);
		break;
	case 104:
		port = cmdport;
		cmd_type = 0;
		strcpy(log_content, "恢复程序示教");
		strcpy(en_log_content, "Restore program teaching");
		strcpy(jap_log_content, "回復プログラムが教えてくれる");
		ret = program_resume(data_json, content);
		break;
	case 105:/* 8082 */
		port = fileport;
		strcpy(log_content, "下发程序示教名称");
		strcpy(en_log_content, "Send the program teaching name");
		strcpy(jap_log_content, "プログラムを発行して名称を教える");
		ret = sendfilename(data_json, content);
		break;
	case 106:/* 8082 */
		port = fileport;
		strcpy(log_content, "下发程序示教文件内容");
		strcpy(en_log_content, "Send program teaching document content");
		strcpy(jap_log_content, "プログラムを交付して文書の内容を教える");
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
		ret = sendfile(data_json, content);
		break;
	case 107:
		port = cmdport;
		cmd_type = 0;
		strcpy(log_content, "清除控制器错误");
		strcpy(en_log_content, "Clear controller error");
		strcpy(jap_log_content, "コントローラエラーをクリアする");
		ret = copy_content(data_json, content);
		break;
	case 201:
		port = cmdport;
		strcpy(log_content, "下发关节数据");
		strcpy(en_log_content, "Send joint data");
		strcpy(jap_log_content, "下毛関節データ");
		ret = movej(data_json, content);
		break;
	case 203:
		port = cmdport;
		strcpy(log_content, "基坐标单轴点动-点按开始");
		strcpy(en_log_content, "Base coordinate uniaxial pint - start by pint");
		strcpy(jap_log_content, "基本座標単軸点動-点押し開始");
		ret = copy_content(data_json, content);
		break;
	case 204:
		port = cmdport;
		strcpy(log_content, "设置控制箱DO");
		strcpy(en_log_content, "Set the control box DO");
		strcpy(jap_log_content, "コントロールボックスdoを設置する");
		ret = copy_content(data_json, content);
		break;
	case 206:
		port = cmdport;
		strcpy(log_content, "设置速度百分比");
		strcpy(en_log_content, "Set speed percentage");
		strcpy(jap_log_content, "速度のパーセンテージを設定する");
		ret = copy_content(data_json, content);
		break;
	case 208:
		port = cmdport;
		strcpy(log_content, "关节坐标单轴点动-点按开始");
		strcpy(en_log_content, "Joint coordinate uniaxial pinging - start by pinging");
		strcpy(jap_log_content, "関節座標一軸点働-点押し開始");
		ret = copy_content(data_json, content);
		break;
	case 209:
		port = cmdport;
		strcpy(log_content, "设置控制箱AO");
		strcpy(en_log_content, "Set the control box AO");
		strcpy(jap_log_content, "");
		ret = copy_content(data_json, content);
		break;
	case 210:
		port = cmdport;
		strcpy(log_content, "设置末端工具DO");
		strcpy(en_log_content, "Set the end tool DO");
		strcpy(jap_log_content, "エンドツールdoを設定します");
		ret = copy_content(data_json, content);
		break;
	case 211:
		port = cmdport;
		strcpy(log_content, "设置末端工具AO");
		strcpy(en_log_content, "Set the end tool AO");
		strcpy(jap_log_content, "エンドツールaoを設定します");
		ret = copy_content(data_json, content);
		break;
	case 216:
		port = cmdport;
		cmd_type = 0;
		strcpy(log_content, "关节坐标单轴点动-点按结束");
		strcpy(en_log_content, "Joint coordinate uniaxial point - point to end");
		strcpy(jap_log_content, "関節座標一軸点働-点押し終了");
		ret = copy_content(data_json, content);
		break;
	case 222:
		port = cmdport;
		strcpy(log_content, "控制箱DI滤波");
		strcpy(en_log_content, "Control box DI filtering");
		strcpy(jap_log_content, "コントロールボックスdiフィルタリング");
		ret = copy_content(data_json, content);
		break;
	case 223:
		port = cmdport;
		strcpy(log_content, "工具DI滤波");
		strcpy(en_log_content, "Tool DI Filtering");
		strcpy(jap_log_content, "ツールdiフィルタリング");
		ret = copy_content(data_json, content);
		break;
	case 224:
		port = cmdport;
		strcpy(log_content, "控制箱AI滤波");
		strcpy(en_log_content, "Control box AI filtering");
		strcpy(jap_log_content, "コントロールボックスaiフィルタリング");
		ret = copy_content(data_json, content);
		break;
	case 225:
		port = cmdport;
		strcpy(log_content, "工具AI0滤波");
		strcpy(en_log_content, "Tool AI0 Filtering");
		strcpy(jap_log_content, "ツールai0フィルタリング");
		ret = copy_content(data_json, content);
		break;
	case 226:
		port = cmdport;
		strcpy(log_content, "配置夹爪");
		strcpy(en_log_content, "Configuration grip");
		strcpy(jap_log_content, "クリップ爪を配置する");
		ret = copy_content(data_json, content);
		break;
	case 227:
		port = cmdport;
		strcpy(log_content, "激活和复位夹爪");
		strcpy(en_log_content, "Activate and reset the gripper");
		strcpy(jap_log_content, "クリップを起動してリセットします");
		ret = copy_content(data_json, content);
		break;
	case 229:
		port = cmdport;
		strcpy(log_content, "读取夹爪配置信息");
		strcpy(en_log_content, "Read the gripper configuration information");
		strcpy(jap_log_content, "クリップの配置情報を読み取る");
		ret = copy_content(data_json, content);
		break;
	case 230:
		port = cmdport;
		strcpy(log_content, "设置查询图表id号");
		strcpy(en_log_content, "Sets the query chart ID number");
		strcpy(jap_log_content, "クエリーチャートid番号を設定します");
		ret = set_state_id(data_json, content);
		break;
	case 231:
		port = cmdport;
		strcpy(log_content, "状态查询开始/结束");
		strcpy(en_log_content, "Status query starts/ends");
		strcpy(jap_log_content, "ステータス照会の開始/終了");
		ret = set_state(data_json, content);
		break;
	case 232:
		port = cmdport;
		strcpy(log_content, "单轴点动-长按开始");
		strcpy(en_log_content, "Single axis point - long press to start");
		strcpy(jap_log_content, "単軸点働-長押し開始");
		ret = copy_content(data_json, content);
		break;
	case 233:
		port = cmdport;
		cmd_type = 0;
		strcpy(log_content, "单轴点动-长按结束");
		strcpy(en_log_content, "Single axis point - long press to end");
		strcpy(jap_log_content, "単軸点働-長押し終了");
		ret = copy_content(data_json, content);
		break;
	case 234:
		port = cmdport;
		cmd_type = 0;
		strcpy(log_content, "基坐标单轴点动-点按结束");
		strcpy(en_log_content, "Base coordinate uniaxial point - end of point pres");
		strcpy(jap_log_content, "基本座標単軸点動-点押し終了");
		ret = copy_content(data_json, content);
		break;
	case 235:
		port = cmdport;
		cmd_type = 0;
		strcpy(log_content, "外部工具坐标单轴点动-长按结束");
		strcpy(en_log_content, "External tool coordinates single axis point - long press to end");
		strcpy(jap_log_content, "外部工具座標一軸点動-長押しで終了");
		ret = copy_content(data_json, content);
		break;
	case 236:
		port = cmdport;
		strcpy(log_content, "开始喷涂");
		strcpy(en_log_content, "Began spray");
		strcpy(jap_log_content, "スプレーをかける");
		ret = copy_content(data_json, content);
		break;
	case 237:
		port = cmdport;
		strcpy(log_content, "停止喷涂");
		strcpy(en_log_content, "Stop spray");
		strcpy(jap_log_content, "スプレーを止める");
		ret = copy_content(data_json, content);
		break;
	case 238:
		port = cmdport;
		strcpy(log_content, "开始清枪");
		strcpy(en_log_content, "Began to clear gun");
		strcpy(jap_log_content, "銃の洗浄を始める");
		ret = copy_content(data_json, content);
		break;
	case 239:
		port = cmdport;
		strcpy(log_content, "停止清枪");
		strcpy(en_log_content, "Stop the qing gun");
		strcpy(jap_log_content, "清銃を止める");
		ret = copy_content(data_json, content);
		break;
	case 240:
		port = cmdport;
		strcpy(log_content, "停止外部轴运动");
		strcpy(en_log_content, "Stop the external axis movement");
		strcpy(jap_log_content, "外軸の動きを止める");
		ret = copy_content(data_json, content);
		break;
	case 247:
		port = cmdport;
		strcpy(log_content, "起弧");
		strcpy(en_log_content, "start arc");
		strcpy(jap_log_content, "から弧");
		ret = copy_content(data_json, content);
		break;
	case 248:
		port = cmdport;
		strcpy(log_content, "收弧");
		strcpy(en_log_content, "stop arc");
		strcpy(jap_log_content, "弧を");
		ret = copy_content(data_json, content);
		break;
	case 249:
		port = cmdport;
		strcpy(log_content, "设定工件参考点");
		strcpy(en_log_content, "Set workpiece reference points");
		strcpy(jap_log_content, "ワークの基準点を設定する");
		ret = copy_content(data_json, content);
		break;
	case 250:
		port = cmdport;
		strcpy(log_content, "计算工件位姿");
		strcpy(en_log_content, "Calculate the workpiece pose");
		strcpy(jap_log_content, "ワークポジションを計算する");
		ret = copy_content(data_json, content);
		break;
	case 251:
		port = cmdport;
		strcpy(log_content, "设置工件坐标系");
		strcpy(en_log_content, "Set the workpiece coordinate system");
		strcpy(jap_log_content, "ワーク座標系を設定します");
		ret = copy_content(data_json, content);
		break;
	case 252:
		port = cmdport;
		strcpy(log_content, "摆焊参数设置");
		strcpy(en_log_content, "Swing welding parameter setting");
		strcpy(jap_log_content, "振り子溶接パラメータを設定します");
		ret = copy_content(data_json, content);
		break;
	case 253:
		port = cmdport;
		strcpy(log_content, "开始摆焊");
		strcpy(en_log_content, "Began to swing welding");
		strcpy(jap_log_content, "振り子溶接を開始する");
		ret = copy_content(data_json, content);
		break;
	case 254:
		port = cmdport;
		strcpy(log_content, "停止摆焊");
		strcpy(en_log_content, "Stop swing welding");
		strcpy(jap_log_content, "振り子溶接を止める");
		ret = copy_content(data_json, content);
		break;
	case 255:
		port = cmdport;
		strcpy(log_content, "激光打开");
		strcpy(en_log_content, "Laser on");
		strcpy(jap_log_content, "レーザーオープン");
		ret = copy_content(data_json, content);
		break;
	case 256:
		port = cmdport;
		strcpy(log_content, "激光关闭");
		strcpy(en_log_content, "Laser off");
		strcpy(jap_log_content, "レーザー閉鎖");
		ret = copy_content(data_json, content);
		break;
	case 257:
		port = cmdport;
		strcpy(log_content, "开始跟踪");
		strcpy(en_log_content, "Began track");
		strcpy(jap_log_content, "追跡を開始する");
		ret = copy_content(data_json, content);
		break;
	case 258:
		port = cmdport;
		strcpy(log_content, "停止跟踪");
		strcpy(en_log_content, "Stop trace");
		strcpy(jap_log_content, "尾行を止める");
		ret = copy_content(data_json, content);
		break;
	case 259:
		port = cmdport;
		strcpy(log_content, "寻位开始,设置寻位参数");
		strcpy(en_log_content, "To start the search, set the search parameters");
		strcpy(jap_log_content, "ビットアドレシングを開始し、ビットアドレシングパラメータを設定します");
		ret = copy_content(data_json, content);
		break;
	case 260:
		port = cmdport;
		strcpy(log_content, "寻位结束");
		strcpy(en_log_content, "Find an end");
		strcpy(jap_log_content, "アドレッシング・エンド");
		ret = copy_content(data_json, content);
		break;
	case 261:
		port = cmdport;
		strcpy(log_content, "设定传感器参考点");
		strcpy(en_log_content, "Set the sensor reference point");
		strcpy(jap_log_content, "センサ基準点を設定する");
		ret = copy_content(data_json, content);
		break;
	case 262:
		port = cmdport;
		strcpy(log_content, "计算传感器位姿");
		strcpy(en_log_content, "Calculate the sensor pose");
		strcpy(jap_log_content, "センサポージングを計算します");
		ret = copy_content(data_json, content);
		break;
	case 263:
		port = cmdport;
		strcpy(log_content, "配置机器人IP (192.168.57.2 网口)");
		strcpy(en_log_content, "Configure Robot IP");
		strcpy(jap_log_content, "ロボットipを構成する");
		ret = copy_content(data_json, content);
		break;
	case 264:
		port = cmdport;
		strcpy(log_content, "配置激光跟踪传感器IP和端口");
		strcpy(en_log_content, "Equipped with laser tracking sensor IP and port");
		strcpy(jap_log_content, "レーザートラッキングセンサipとポートを構成します");
		ret = copy_content(data_json, content);
		break;
	case 265:
		port = cmdport;
		strcpy(log_content, "加载传感器通信协议");
		strcpy(en_log_content, "Load the sensor communication protocol");
		strcpy(jap_log_content, "センサの通信プロトコルをロードします");
		ret = copy_content(data_json, content);
		break;
	case 266:
		port = cmdport;
		strcpy(log_content, "卸载传感器通信协议");
		strcpy(en_log_content, "Unload the sensor communication protocol");
		strcpy(jap_log_content, "センサ通信プロトコルをアンインストールします");
		ret = copy_content(data_json, content);
		break;
	case 267:
		port = cmdport;
		strcpy(log_content, "配置传感器采样周期");
		strcpy(en_log_content, "Configure the sensor sampling cycle");
		strcpy(jap_log_content, "センサのサンプリング期間を設定します");
		ret = copy_content(data_json, content);
		break;
	case 268:
		port = cmdport;
		strcpy(log_content, "开始/停止正向送丝");
		strcpy(en_log_content, "Start/stop forward wire feed");
		strcpy(jap_log_content, "糸の送りを開始/停止します");
		ret = copy_content(data_json, content);
		break;
	case 269:
		port = cmdport;
		strcpy(log_content, "开始/停止反向送丝");
		strcpy(en_log_content, "Start/stop reverse wire feed");
		strcpy(jap_log_content, "逆送りを開始/停止します");
		ret = copy_content(data_json, content);
		break;
	case 270:
		port = cmdport;
		strcpy(log_content, "开始/停止送气");
		strcpy(en_log_content, "Start/stop air supply");
		strcpy(jap_log_content, "送気を開始/停止する");
		ret = copy_content(data_json, content);
		break;
	case 271:
		port = cmdport;
		strcpy(log_content, "十点法设定传感器参考点");
		strcpy(en_log_content, "Ten point method to set the sensor reference point");
		strcpy(jap_log_content, "10時法でセンサの基準点を設定する");
		ret = copy_content(data_json, content);
		break;
	case 272:
		port = cmdport;
		strcpy(log_content, "十点法计算传感器位姿");
		strcpy(en_log_content, "Ten point method to calculate the sensor posture");
		strcpy(jap_log_content, "十時法はセンサの姿勢を計算する");
		ret = copy_content(data_json, content);
		break;
	case 273:
		port = cmdport;
		strcpy(log_content, "八点法设置激光跟踪传感器参考点");
		strcpy(en_log_content, "Eight point method is used to set the reference point of the laser tracking sensor");
		strcpy(jap_log_content, "レーザートラッキングセンサの基準点を8点で設定します");
		ret = copy_content(data_json, content);
		break;
	case 274:
		port = cmdport;
		strcpy(log_content, "八点法计算激光跟踪传感器位姿");
		strcpy(en_log_content, "Eight point method is used to calculate the position and pose of laser tracking sensor");
		strcpy(jap_log_content, "8点法でレーザートラッキングセンサの姿勢を計算します");
		ret = copy_content(data_json, content);
		break;
	case 276:
		port = cmdport;
		strcpy(log_content, "三点法设置激光跟踪传感器参考点");
		strcpy(en_log_content, "The reference point of laser tracking sensor is set by three-point method");
		strcpy(jap_log_content, "3点法でレーザートラッキングセンサの基準点を設定します");
		ret = copy_content(data_json, content);
		break;
	case 277:
		port = cmdport;
		strcpy(log_content, "三点法计算激光跟踪传感器位姿");
		strcpy(en_log_content, "The position and pose of laser tracking sensor are calculated by three-point method");
		strcpy(jap_log_content, "3点法でレーザートラッキングセンサの姿勢を計算します");
		ret = copy_content(data_json, content);
		break;
	case 278:
		port = cmdport;
		strcpy(log_content, "激光跟踪数据记录");
		strcpy(en_log_content, "Laser tracking data recording");
		strcpy(jap_log_content, "レーザートレーシングデータ記録");
		ret = copy_content(data_json, content);
		break;
	case 279:
		port = cmdport;
		strcpy(log_content, "激光跟踪最大差值");
		strcpy(en_log_content, "Laser tracking maximum difference");
		strcpy(jap_log_content, "レーザートラッカー最大差");
		ret = copy_content(data_json, content);
		break;
	case 280:
		port = cmdport;
		strcpy(log_content, "设置激光跟踪传感器位置");
		strcpy(en_log_content, "Set the position of the laser tracking sensor");
		strcpy(jap_log_content, "レーザートラッキングセンサの位置を設定します");
		ret = copy_content(data_json, content);
		break;
	case 283:
		port = cmdport;
		strcpy(log_content, "获取激光跟踪传感器配置信息");
		strcpy(en_log_content, "Gets the laser tracking sensor configuration information");
		strcpy(jap_log_content, "レーザートラッキングセンサの構成情報を取得する");
		ret = copy_content(data_json, content);
		break;
	case 287:
		port = cmdport;
		strcpy(log_content, "激活/去激活外部轴坐标系");
		strcpy(en_log_content, "Activate/deactivate the external axis coordinate system");
		strcpy(jap_log_content, "軸軸座標系をアクティブ化/非アクティブ化します");
		ret = copy_content(data_json, content);
		break;
	case 288:
		port = cmdport;
		strcpy(log_content, "四点法设定外部轴坐标系参考点");
		strcpy(en_log_content, "The four-point method sets the reference point of the external axis coordinate system");
		strcpy(jap_log_content, "4点法は外部軸座標系の基準点を設定する");
		ret = copy_content(data_json, content);
		break;
	case 289:
		port = cmdport;
		strcpy(log_content, "四点法计算外部轴坐标系");
		strcpy(en_log_content, "The four-point method is used to calculate the external axis coordinate system");
		strcpy(jap_log_content, "4点法は外部軸座標系を計算する");
		ret = copy_content(data_json, content);
		break;
	case 290:
		port = cmdport;
		strcpy(log_content, "设定外部轴零位");
		strcpy(en_log_content, "Set external axis zero");
		strcpy(jap_log_content, "外軸ゼロビットを設定する");
		ret = copy_content(data_json, content);
		break;
	case 291:
		port = cmdport;
		strcpy(log_content, "外部轴参数配置");
		strcpy(en_log_content, "External axis parameter configuration");
		strcpy(jap_log_content, "外部軸パラメータの設定");
		ret = copy_content(data_json, content);
		break;
	case 292:
		port = cmdport;
		strcpy(log_content, "外部轴点动开始");
		strcpy(en_log_content, "The external axis starts to move");
		strcpy(jap_log_content, "外軸の点動が始まる");
		ret = copy_content(data_json, content);
		break;
	case 293:
		port = cmdport;
		strcpy(log_content, "扩展轴系统 DH 参数配置");
		strcpy(en_log_content, "Expansion axis system DH parameter configuration");
		strcpy(jap_log_content, "軸軸系dhパラメータ構成を拡張します");
		ret = copy_content(data_json, content);
		break;
	case 294:
		port = cmdport;
		strcpy(log_content, "机器人相对扩展轴位置, 0:扩展轴上,1:扩展轴外");
		strcpy(en_log_content, "Position of the robot relative to the extension axis, 0: on the extension axis,1: off the extension axis");
		strcpy(jap_log_content, "ロボットの相対拡幅軸位置は、0:拡幅軸上、1:拡幅軸外である");
		ret = copy_content(data_json, content);
		break;
	case 295:
		port = cmdport;
		strcpy(log_content, "外部轴伺服警告清除");
		strcpy(en_log_content, "External shaft servo warning cleared");
		strcpy(jap_log_content, "外部軸サーボ警告をクリアします");
		ret = copy_content(data_json, content);
		break;
	case 296:
		port = cmdport;
		strcpy(log_content, "外部轴伺服使能");
		strcpy(en_log_content, "External shaft servo enabling");
		strcpy(jap_log_content, "外部軸サーボが可能です");
		ret = copy_content(data_json, content);
		break;
	case 297:
		port = cmdport;
		strcpy(log_content, "外部轴运动");
		strcpy(en_log_content, "External axis motion");
		strcpy(jap_log_content, "外軸運動");
		ret = copy_content(data_json, content);
		break;
	case 298:
		port = cmdport;
		strcpy(log_content, "外部轴定位完成时间");
		strcpy(en_log_content, "External axis positioning done time");
		strcpy(jap_log_content, "外軸の位置決め完了時間");
		ret = copy_content(data_json, content);
		break;
	case 302:
		port = cmdport;
		strcpy(log_content, "机器手急停后电机使能");
		strcpy(en_log_content, "Enable motor after emergency stop of machine hand");
		strcpy(jap_log_content, "机械の手が急停止して電机が作動する");
		ret = copy_content(data_json, content);
		break;
	case 303:
		port = cmdport;
		strcpy(log_content, "更改机器人模式");
		strcpy(en_log_content, "Change robot mode");
		strcpy(jap_log_content, "ロボットモードを変更する");
		ret = mode(data_json, content);
		break;
	case 305:
		port = cmdport;
		strcpy(log_content, "设置碰撞等级");
		strcpy(en_log_content, "Set collision level");
		strcpy(jap_log_content, "衝突レベルを設定する");
		ret = copy_content(data_json, content);
		break;
	case 306:
		port = cmdport;
		strcpy(log_content, "设置负载重量");
		strcpy(en_log_content, "Set load weight");
		strcpy(jap_log_content, "荷重重量を設ける");
		ret = copy_content(data_json, content);
		break;
	case 307:
		port = cmdport;
		strcpy(log_content, "设置负载质心");
		strcpy(en_log_content, "Sets the load center of mass");
		strcpy(jap_log_content, "負荷重心を設定する");
		ret = copy_content(data_json, content);
		break;
	case 308:
		port = cmdport;
		strcpy(log_content, "设置机器人正限位角度");
		strcpy(en_log_content, "Set the positive limit Angle of the robot");
		strcpy(jap_log_content, "ロボットのポジティブストッパ角度を設定します");
		ret = copy_content(data_json, content);
		break;
	case 309:
		port = cmdport;
		strcpy(log_content, "设置机器人负限位角度");
		strcpy(en_log_content, "Set the negative limit Angle of the robot");
		strcpy(jap_log_content, "ロボットの負のストッパ角度を設定します");
		ret = copy_content(data_json, content);
		break;
	case 312:
		port = cmdport;
		cmd_type = 0;
		strcpy(log_content, "零点设定");
		strcpy(en_log_content, "Zero setting");
		strcpy(jap_log_content, "零点設定");
		ret = copy_content(data_json, content);
		break;
	case 313:
		port = cmdport;
		strcpy(log_content, "新建工具坐标系下发点");
		strcpy(en_log_content, "Create a new point in the tool coordinate system");
		strcpy(jap_log_content, "ツール座標系の配点点を新設する");
		ret = copy_content(data_json, content);
		break;
	case 314:
		port = cmdport;
		strcpy(log_content, "计算工具坐标系");
		strcpy(en_log_content, "Calculate tool coordinates");
		strcpy(jap_log_content, "計算ツール座標系");
		ret = copy_content(data_json, content);
		break;
	case 315:
		port = cmdport;
		strcpy(log_content, "开始记录TPD轨迹");
		strcpy(en_log_content, "Start recording TPD tracks");
		strcpy(jap_log_content, "tpdの軌跡を記録し始めました");
		ret = copy_content(data_json, content);
		break;
	case 316:
		port = cmdport;
		strcpy(log_content, "应用当前显示的工具坐标系");
		strcpy(en_log_content, "Apply the currently displayed tool coordinate system");
		strcpy(jap_log_content, "現在表示されているツール座標系を適用する");
		ret = copy_content(data_json, content);
		break;
	case 317:
		port = cmdport;
		strcpy(log_content, "停止记录TPD轨迹");
		strcpy(en_log_content, "Stop recording the TPD trajectory");
		strcpy(jap_log_content, "tpdトラックの記録を停止する");
		ret = copy_content(data_json, content);
		break;
	case 318:
		port = cmdport;
		strcpy(log_content, "删除TPD轨迹");
		strcpy(en_log_content, "Delete the TPD trace");
		strcpy(jap_log_content, "tpdトラックを削除します");
		ret = copy_content(data_json, content);
		break;
	case 320:
		port = cmdport;
		strcpy(log_content, "计算 Joint to TCF");
		strcpy(en_log_content, "Calculate Joint to TCF");
		strcpy(jap_log_content, "joint to tcfを計算する");
		ret = jointtotcf(data_json, content);
		break;
	case 321:
		port = cmdport;
		strcpy(log_content, "生效机器人配置文件");
		strcpy(en_log_content, "Valid robot configuration file");
		strcpy(jap_log_content, "ロボットのプロファイルを有効にします");
		ret = copy_content(data_json, content);
		break;
	case 323:
		port = cmdport;
		strcpy(log_content, "设置 DI 配置");
		strcpy(en_log_content, "Setting the DI configuration");
		strcpy(jap_log_content, "di構成を設定する");
		ret = copy_content(data_json, content);
		break;
	case 324:
		port = cmdport;
		strcpy(log_content, "设置 DO 配置");
		strcpy(en_log_content, "Setting the DO configuration");
		strcpy(jap_log_content, "do構成を設定する");
		ret = copy_content(data_json, content);
		break;
	case 325:
		port = cmdport;
		strcpy(log_content, "计算 TCF to Joint");
		strcpy(en_log_content, "Calculate the TCF to Joint");
		strcpy(jap_log_content, "tcf to jointを計算する");
		ret = copy_content(data_json, content);
		break;
	case 326:
		port = cmdport;
		strcpy(log_content, "设定外部TCP参考点");
		strcpy(en_log_content, "Set the external TCP reference point");
		strcpy(jap_log_content, "外部tcp基準点を設定する");
		ret = copy_content(data_json, content);
		break;
	case 327:
		port = cmdport;
		strcpy(log_content, "计算外部TCF");
		strcpy(en_log_content, "Calculate the external TCF");
		strcpy(jap_log_content, "外部tcfを計算する");
		ret = copy_content(data_json, content);
		break;
	case 328:
		port = cmdport;
		strcpy(log_content, "设定外部TCP工具参考点");
		strcpy(en_log_content, "Set the external TCP tool reference point");
		strcpy(jap_log_content, "外部tcpツールの参照点を設定する");
		ret = copy_content(data_json, content);
		break;
	case 329:
		port = cmdport;
		strcpy(log_content, "计算工具TCF");
		strcpy(en_log_content, "The calculation tool TCF");
		strcpy(jap_log_content, "計算ツールtcf");
		ret = copy_content(data_json, content);
		break;
	case 330:
		port = cmdport;
		strcpy(log_content, "应用当前显示的外部工具坐标系");
		strcpy(en_log_content, "Apply the external tool coordinate system that is currently displayed");
		strcpy(jap_log_content, "");
		ret = copy_content(data_json, content);
		break;
	case 332:
		port = cmdport;
		strcpy(log_content, "进入 boot 模式");
		strcpy(en_log_content, "Enter Boot mode");
		strcpy(jap_log_content, "bootモードに入ります");
		ret = copy_content(data_json, content);
		break;
	case 333:
		port = cmdport;
		strcpy(log_content, "切换拖动示教模式");
		strcpy(en_log_content, "Toggle the drag teaching mode");
		strcpy(jap_log_content, "ドラッグモードを切り替えます");
		ret = copy_content(data_json, content);
		break;
	case 334:
		port = cmdport;
		strcpy(log_content, "定位完成阈值");
		strcpy(en_log_content, "Locate the completion threshold");
		strcpy(jap_log_content, "完了閾値を特定する");
		ret = copy_content(data_json, content);
		break;
	case 335:
		port = cmdport;
		strcpy(log_content, "设置 DI 有效电平");
		strcpy(en_log_content, "Set the DI valid level");
		strcpy(jap_log_content, "di有効レベルを設定する");
		ret = copy_content(data_json, content);
		break;
	case 336:
		port = cmdport;
		strcpy(log_content, "设置 DO 有效电平");
		strcpy(en_log_content, "Set the DO valid level");
		strcpy(jap_log_content, "do有効レベルを設定します");
		ret = copy_content(data_json, content);
		break;
	case 337:
		port = cmdport;
		strcpy(log_content, "机器人安装方式");
		strcpy(en_log_content, "Installation of robot");
		strcpy(jap_log_content, "ロボットの取り付け方");
		ret = copy_content(data_json, content);
		break;
	case 338:
		port = cmdport;
		strcpy(log_content, "拖动示教摩擦力补偿开关");
		strcpy(en_log_content, "Drag the show friction compensation switch");
		strcpy(jap_log_content, "示教摩擦力補償スイッチをドラッグします");
		ret = copy_content(data_json, content);
		break;
	case 339:
		port = cmdport;
		strcpy(log_content, "配置外设DO");
		strcpy(en_log_content, "Configure the peripheral DO");
		strcpy(jap_log_content, "周辺機器doを配置する");
		ret = set_plugin_dio(data_json, content, 1);
		break;
	case 340:
		port = cmdport;
		strcpy(log_content, "配置外设DI");
		strcpy(en_log_content, "Configure the peripheral DI");
		strcpy(jap_log_content, "周辺機器diを配置する");
		ret = set_plugin_dio(data_json, content, 0);
		break;
	case 341:
		port = cmdport;
		strcpy(log_content, "设置摩擦力补偿系数");
		strcpy(en_log_content, "Set the friction compensation coefficient");
		strcpy(jap_log_content, "摩擦力補償係数を設定する");
		ret = copy_content(data_json, content);
		break;
	case 343:
		port = cmdport;
		strcpy(log_content, "同步系统时间");
		strcpy(en_log_content, "Synchronous system time");
		strcpy(jap_log_content, "同期システム時間");
		ret = copy_content(data_json, content);
		break;
	case 345:
		port = cmdport;
		strcpy(log_content, "检测机器人配置文件");
		strcpy(en_log_content, "Check the robot configuration file");
		strcpy(jap_log_content, "ロボットのプロファイルを検出する");
		ret = copy_content(data_json, content);
		break;
	case 353:
		port = cmdport;
		strcpy(log_content, "配置视觉控制器 IP 和 Port");
		strcpy(en_log_content, "Configure the visual controller IP and Port");
		strcpy(jap_log_content, "ビジョンコントローラipとportを設定します");
		ret = copy_content(data_json, content);
		break;
	case 354:
		port = cmdport;
		strcpy(log_content, "加载视觉传感器通信驱动");
		strcpy(en_log_content, "Load the visual sensor communication driver");
		strcpy(jap_log_content, "通信用視覚センサーを装填します");
		ret = copy_content(data_json, content);
		break;
	case 355:
		port = cmdport;
		strcpy(log_content, "卸载视觉传感器通信驱动");
		strcpy(en_log_content, "Unload the vision sensor communication driver");
		strcpy(jap_log_content, "視覚センサ通信ドライブを取り外します");
		ret = copy_content(data_json, content);
		break;
	case 356:
		port = cmdport;
		strcpy(log_content, "触发相机拍照");
		strcpy(en_log_content, "Trigger camera to take pictures");
		strcpy(jap_log_content, "カメラを作動させて");
		ret = copy_content(data_json, content);
		break;
	case 357:
		port = cmdport;
		strcpy(log_content, "负载辨识启停");
		strcpy(en_log_content, "Load identification starts and stops");
		strcpy(jap_log_content, "負荷認識が開始し停止する");
		ret = copy_content(data_json, content);
		break;
	case 358:
		port = cmdport;
		strcpy(log_content, "传送带启动");
		strcpy(en_log_content, "The conveyor belt starts");
		strcpy(jap_log_content, "ベルトコンベヤーが作動する");
		ret = copy_content(data_json, content);
		break;
	case 359:
		port = cmdport;
		strcpy(log_content, "传送带 IO 切入点标定");
		strcpy(en_log_content, "Calibration of IO cut point for conveyor belt");
		strcpy(jap_log_content, "ベルトコンベヤーio切り口を決める");
		ret = copy_content(data_json, content);
		break;
	case 360:
		port = cmdport;
		strcpy(log_content, "传送带 A 点标定");
		strcpy(en_log_content, "Calibration of point A of conveyor belt");
		strcpy(jap_log_content, "ベルトコンベヤーのa点校正");
		ret = copy_content(data_json, content);
		break;
	case 361:
		port = cmdport;
		strcpy(log_content, "传送带参考点标定");
		strcpy(en_log_content, "Calibration of conveyor belt reference point");
		strcpy(jap_log_content, "ベルトコンベヤーの基準点を校正する");
		ret = copy_content(data_json, content);
		break;
	case 362:
		port = cmdport;
		strcpy(log_content, "传送带 B 点标定");
		strcpy(en_log_content, "Calibration of point B of conveyor belt");
		strcpy(jap_log_content, "ベルトコンベヤーのb地点を校正する");
		ret = copy_content(data_json, content);
		break;
	case 367:
		port = cmdport;
		strcpy(log_content, "传送带参数配置");
		strcpy(en_log_content, "Conveyor parameters configuration");
		strcpy(jap_log_content, "ベルトコンベヤーのパラメータ設定");
		ret = copy_content(data_json, content);
		break;
	case 368:
		port = cmdport;
		strcpy(log_content, "传送带抓取点补偿");
		strcpy(en_log_content, "Carousel grabbing point compensation");
		strcpy(jap_log_content, "ベルトコンベヤーのつかみ点補償");
		ret = copy_content(data_json, content);
		break;
	case 369:
		port = cmdport;
		strcpy(log_content, "配置末端DI");
		strcpy(en_log_content, "Configure terminal DI");
		strcpy(jap_log_content, "エンドdiを配置する");
		ret = copy_content(data_json, content);
		break;
	case 370:
		port = cmdport;
		strcpy(log_content, "配置末端DO");
		strcpy(en_log_content, "Configure terminal DO");
		strcpy(jap_log_content, "エンドdoを配置する");
		ret = copy_content(data_json, content);
		break;
	case 371:
		port = cmdport;
		strcpy(log_content, "设置末端DI有效电平");
		strcpy(en_log_content, "Set the terminal DI effective level");
		strcpy(jap_log_content, "エンドdi有効レベルを設定する");
		ret = copy_content(data_json, content);
		break;
	case 372:
		port = cmdport;
		strcpy(log_content, "设置末端DO有效电平");
		strcpy(en_log_content, "Set the terminal DO effective level");
		strcpy(jap_log_content, "");
		ret = copy_content(data_json, content);
		break;
	case 375:
		port = cmdport;
		strcpy(log_content, "获取关节位置");
		strcpy(en_log_content, "Obtain joint position");
		strcpy(jap_log_content, "関節の位置を把握する");
		ret = copy_content(data_json, content);
		break;
	case 376:
		port = cmdport;
		strcpy(log_content, "ServoJ");
		strcpy(en_log_content, "ServoJ");
		strcpy(jap_log_content, "ServoJ");
		ret = copy_content(data_json, content);
		break;
	case 380:
		port = cmdport;
		strcpy(log_content, "获取控制器计算后,修改示教点数据");
		strcpy(en_log_content, "After obtaining the controller calculation, modify the data of the teaching point");
		strcpy(jap_log_content, "コントローラ計算を取得した後、示教点データを修正する");
		ret = copy_content(data_json, content);
		break;
	case 381:
		port = cmdport;
		strcpy(log_content, "设置激光点");
		strcpy(en_log_content, "Set laser point");
		strcpy(jap_log_content, "レーザーポイントを設置する");
		ret = copy_content(data_json, content);
		break;
	case 382:
		port = cmdport;
		strcpy(log_content, "计算激光点");
		strcpy(en_log_content, "Computed laser point");
		strcpy(jap_log_content, "レーザー点を計算する");
		ret = copy_content(data_json, content);
		break;
	case 384:
		port = cmdport;
		strcpy(log_content, "设置默认启动程序");
		strcpy(en_log_content, "Set the default launcher");
		strcpy(jap_log_content, "デフォルトの起動プログラムを設定します");
		ret = copy_content(data_json, content);
		break;
	case 386:
		port = cmdport;
		strcpy(log_content, "计算激光传感器点偏移量");
		strcpy(en_log_content, "Calculate the laser sensor point offset");
		strcpy(jap_log_content, "レーザーセンサーのポイントオフセットを計算します");
		ret = copy_content(data_json, content);
		break;
	case 387:
		port = cmdport;
		strcpy(log_content, "设置激光传感器标定点");
		strcpy(en_log_content, "Set laser sensor marking point");
		strcpy(jap_log_content, "レーザーセンサ標点を設置する");
		ret = copy_content(data_json, content);
		break;
	case 388:
		port = cmdport;
		strcpy(log_content, "变位机坐标系参考点设置");
		strcpy(en_log_content, "Locator coordinate system reference point setting");
		strcpy(jap_log_content, "変位機座標系の基準点を設定します");
		ret = copy_content(data_json, content);
		break;
	case 389:
		port = cmdport;
		strcpy(log_content, "变位机四点法标定参考点设置");
		strcpy(en_log_content, "Positioning machine four-point calibration reference point setting");
		strcpy(jap_log_content, "変位机4点法校正基準点の設定");
		ret = copy_content(data_json, content);
		break;
	case 390:
		port = cmdport;
		strcpy(log_content, "变位机坐标系计算");
		strcpy(en_log_content, "Alterator coordinate system calculation");
		strcpy(jap_log_content, "変位機械座標系の計算");
		ret = copy_content(data_json, content);
		break;
	case 391:
		port = cmdport;
		strcpy(log_content, "编码器类型切换");
		strcpy(en_log_content, "Encoder type switching");
		strcpy(jap_log_content, "エンコーダタイプ切替");
		ret = copy_content(data_json, content);
		break;
	case 393:
		port = cmdport;
		strcpy(log_content, "获取外部轴驱动器配置信息");
		strcpy(en_log_content, "Gets external axle drive configuration information");
		strcpy(jap_log_content, "外付け軸ドライブ構成情報を取得する");
		ret = copy_content(data_json, content);
		break;
	case 400:
		port = cmdport;
		strcpy(log_content, "获取控制器软件版本");
		strcpy(en_log_content, "Get the controller software version");
		strcpy(jap_log_content, "コントローラソフトウェアのバージョンを取得します");
		ret = copy_content(data_json, content);
		break;
	case 401:
		port = cmdport;
		strcpy(log_content, "获取电批程序名数组");
		strcpy(en_log_content, "Gets an array of batch program names");
		strcpy(jap_log_content, "電子バッチのプログラム名の配列を取得します");
		ret = get_program_array(data_json, content);
		break;
	case 402:
		port = cmdport;
		strcpy(log_content, "获取电批程序内容");
		strcpy(en_log_content, "Gets the contents of the telegram batch program");
		strcpy(jap_log_content, "電気バッチのプログラム内容を入手する");
		ret = get_program_content(data_json, content);
		break;
	case 403:
		port = cmdport;
		strcpy(log_content, "扭矩程序编辑确认修改");
		strcpy(en_log_content, "Torque: Program editors confirm the changes");
		strcpy(jap_log_content, "プログラム編集者が修正を確認する");
		ret = save_program(data_json, content);
		break;
	case 404:
		port = cmdport;
		strcpy(log_content, "扭矩程序下发");
		strcpy(en_log_content, "Torque: upload program");
		strcpy(jap_log_content, "プログラム交付");
		ret = upload_program(data_json, content);
		break;
	case 405:
		port = cmdport;
		strcpy(log_content, "获取当前扭矩控制模式");
		strcpy(en_log_content, "Gets the current torque control mode");
		strcpy(jap_log_content, "現在のトルク制御モードを取得します");
		ret = get_control_mode(data_json, content);
		break;
	case 406:
		port = cmdport;
		strcpy(log_content, "设置扭矩控制模式");
		strcpy(en_log_content, "Set torque control mode");
		strcpy(jap_log_content, "トルク制御モードを設定します");
		ret = set_control_mode(data_json, content);
		break;
	case 407:
		port = cmdport;
		strcpy(log_content, "测试拧紧");
		strcpy(en_log_content, "Test on tight");
		strcpy(jap_log_content, "締め付けをテストする");
		ret = test_tighten(data_json, content);
		break;
	case 408:
		port = cmdport;
		strcpy(log_content, "测试反松");
		strcpy(en_log_content, "Test the loose");
		strcpy(jap_log_content, "反マツをテストする");
		ret = test_unscrew(data_json, content);
		break;
	case 409:
		port = cmdport;
		strcpy(log_content, "测试自由");
		strcpy(en_log_content, "Test free");
		strcpy(jap_log_content, "自由を試す");
		ret = test_free(data_json, content);
		break;
	case 410:
		port = cmdport;
		strcpy(log_content, "测试停止");
		strcpy(en_log_content, "Test stop");
		strcpy(jap_log_content, "テスト停止");
		ret = test_stop(data_json, content);
		break;
	case 411:
		port = cmdport;
		strcpy(log_content, "设置扭矩系统开关");
		strcpy(en_log_content, "Set torque system On-Off");
		strcpy(jap_log_content, "トルク系スイッチをセットします");
		ret = set_on_off(data_json, content);
		break;
	case 412:
		port = cmdport;
		strcpy(log_content, "获取扭矩系统开关");
		strcpy(en_log_content, "Get torque system On-Off");
		strcpy(jap_log_content, "トルクシステムのスイッチを取得します");
		ret = get_on_off(data_json, content);
		break;
	case 413:
		port = cmdport;
		strcpy(log_content, "获取当前型号范围");
		strcpy(en_log_content, "Gets the current model range");
		strcpy(jap_log_content, "現在のモデル範囲を取得します");
		ret = get_current_range(data_json, content);
		break;
	case 414:
		port = cmdport;
		strcpy(log_content, "获取当前扭矩系统型号");
		strcpy(en_log_content, "Gets the current torque system model");
		strcpy(jap_log_content, "現在のトルクシステムのモデルを取得します");
		ret = get_current_product(data_json, content);
		break;
	case 415:
		port = cmdport;
		strcpy(log_content, "设置扭矩系统单位");
		strcpy(en_log_content, "Set torque system units");
		strcpy(jap_log_content, "トルク系単位を設定します");
		ret = set_torque_unit(data_json, content);
		break;
	case 422:
		port = cmdport;
		strcpy(log_content, "设置激光数据使用方式");
		strcpy(en_log_content, "Set the laser data usage mode");
		strcpy(jap_log_content, "レーザーデータの使用方法を設定する");
		ret = copy_content(data_json, content);
		break;
	case 423:
		port = cmdport;
		strcpy(log_content, "获取从站硬件版本");
		strcpy(en_log_content, "Gets the slave hardware version");
		strcpy(jap_log_content, "スレーブステーションのハードウェアバージョンを取得する");
		ret = copy_content(data_json, content);
		break;
	case 424:
		port = cmdport;
		strcpy(log_content, "获取从站固件版本");
		strcpy(en_log_content, "Gets the slave firmware version");
		strcpy(jap_log_content, "ステーションファームウェアのバージョンを取得します");
		ret = copy_content(data_json, content);
		break;
	case 425:
		port = cmdport;
		strcpy(log_content, "配置机器人型号");
		strcpy(en_log_content, "Configuration robot model");
		strcpy(jap_log_content, "構成ロボットのモデル");
		ret = set_robot_type(data_json, content);
		break;
	case 426:
		port = cmdport;
		strcpy(log_content, "设置停止/暂停后输出是否复位");
		strcpy(en_log_content, "Sets whether the output is reset after stopping/pausing");
		strcpy(jap_log_content, "停止/一時停止後に出力をリセットするかどうかを設定します");
		ret = copy_content(data_json, content);
		break;
	case 429:
		port = cmdport;
		strcpy(log_content, "获取机器人作业原点");
		strcpy(en_log_content, "Obtain the origin of robot operation");
		strcpy(jap_log_content, "ロボット作業の原点を取得する");
		ret = copy_content(data_json, content);
		break;
	case 430:
		port = cmdport;
		strcpy(log_content, "轴干涉区开关设置");
		strcpy(en_log_content, "Axis interference zone switch setting");
		strcpy(jap_log_content, "軸干渉型スイッチ設定");
		ret = copy_content(data_json, content);
		break;
	case 431:
		port = cmdport;
		strcpy(log_content, "轴干涉区参数设置");
		strcpy(en_log_content, "Axis interference zone parameter setting");
		strcpy(jap_log_content, "軸干渉領域のパラメータ設定");
		ret = copy_content(data_json, content);
		break;
	case 432:
		port = cmdport;
		strcpy(log_content, "立方体干涉区开关设置");
		strcpy(en_log_content, "Cube interference zone switch setting");
		strcpy(jap_log_content, "立方体干渉スイッチ設定");
		ret = copy_content(data_json, content);
		break;
	case 433:
		port = cmdport;
		strcpy(log_content, "立方体干涉区参数设置");
		strcpy(en_log_content, "Cube interference zone parameter setting");
		strcpy(jap_log_content, "立方体干渉領域のパラメータ設定");
		ret = copy_content(data_json, content);
		break;
	case 434:
		port = cmdport;
		strcpy(log_content, "立方体干涉区范围设置 - 两点法");
		strcpy(en_log_content, "Cube interference zone range setting - two point method");
		strcpy(jap_log_content, "立方体干渉領域の範囲を設定する - 2点法");
		ret = copy_content(data_json, content);
		break;
	case 435:
		port = cmdport;
		strcpy(log_content, "立方体干涉区范围设置 - 一点+边长");
		strcpy(en_log_content, "Cube interference zone range setting - one point + side length");
		strcpy(jap_log_content, "立方体の干渉領域の範囲を設定 - 一点+辺の長さ");
		ret = copy_content(data_json, content);
		break;
	case 436:
		port = cmdport;
		strcpy(log_content, "进入干涉区是否停止设置");
		strcpy(en_log_content, "Whether to stop setting when entering the interference zone");
		strcpy(jap_log_content, "干渉領域に入るかどうかの設定を停止します");
		ret = copy_content(data_json, content);
		break;
	case 511:
		port = cmdport;
		strcpy(log_content, "设置系统变量");
		strcpy(en_log_content, "Setting System Variables");
		strcpy(jap_log_content, "システム変数を設定する");
		ret = copy_content(data_json, content);
		break;
	case 523:
		port = cmdport;
		strcpy(log_content, "力/扭矩传感器：牵引示教");
		strcpy(en_log_content, "The force/torque sensor: traction teaching");
		strcpy(jap_log_content, "力/トルクセンサー:けん引示教");
		ret = copy_content(data_json, content);
		break;
	case 524:
		port = cmdport;
		strcpy(log_content, "力/扭矩传感器：复位激活");
		strcpy(en_log_content, "The force/torque sensor: reset the activation");
		strcpy(jap_log_content, "力/トルクセンサー:リセット起動");
		ret = copy_content(data_json, content);
		break;
	case 525:
		port = cmdport;
		strcpy(log_content, "设置力/扭矩传感器参考坐标系");
		strcpy(en_log_content, "Set the force/torque sensor the reference frame");
		strcpy(jap_log_content, "設置力/トルクセンサーは座標系を参照");
		ret = copy_content(data_json, content);
		break;
	case 526:
		port = cmdport;
		strcpy(log_content, "设置力/扭矩传感器配置信息");
		strcpy(en_log_content, "Set the force/torque sensor configuration");
		strcpy(jap_log_content, "設置力・トルクセンサ配置情報");
		ret = copy_content(data_json, content);
		break;
	case 527:
		port = cmdport;
		strcpy(log_content, "获取力/扭矩传感器配置信息");
		strcpy(en_log_content, "Obtain force/torque sensor configuration information");
		strcpy(jap_log_content, "パワー/トルクセンサ構成情報取得");
		ret = copy_content(data_json, content);
		break;
	case 528:
		port = cmdport;
		strcpy(log_content, "设置力/扭矩传感器零点");
		strcpy(en_log_content, "Set the force/torque sensor the zero point");
		strcpy(jap_log_content, "力/トルクセンサーを0点設置");
		ret = copy_content(data_json, content);
		break;
	case 529:
		port = cmdport;
		strcpy(log_content, "重量辨识数据记录");
		strcpy(en_log_content, "Weight identification data recording");
		strcpy(jap_log_content, "重量認識データ記録");
		ret = copy_content(data_json, content);
		break;
	case 530:
		port = cmdport;
		strcpy(log_content, "重量辨识数据计算");
		strcpy(en_log_content, "Weight identification data calculation");
		strcpy(jap_log_content, "重量認識データ計算");
		ret = copy_content(data_json, content);
		break;
	case 531:
		port = cmdport;
		strcpy(log_content, "质心辨识数据记录");
		strcpy(en_log_content, "Centroid identification data recording");
		strcpy(jap_log_content, "認識データ記録");
		ret = copy_content(data_json, content);
		break;
	case 532:
		port = cmdport;
		strcpy(log_content, "质心辨识数据计算");
		strcpy(en_log_content, "Calculation of centroid identification data");
		strcpy(jap_log_content, "質感認識データ計算");
		ret = copy_content(data_json, content);
		break;
	case 541:
		port = cmdport;
		strcpy(log_content, "设置摩擦力补偿系数-平装");
		strcpy(en_log_content, "Set friction compensation coefficient - flat mount");
		strcpy(jap_log_content, "摩擦力補償係数の設定-ページング");
		ret = copy_content(data_json, content);
		break;
	case 542:
		port = cmdport;
		strcpy(log_content, "设置摩擦力补偿系数-侧装");
		strcpy(en_log_content, "Set friction compensation coefficient - side mount");
		strcpy(jap_log_content, "摩擦力補償系数-サイドセット");
		ret = copy_content(data_json, content);
		break;
	case 543:
		port = cmdport;
		strcpy(log_content, "设置摩擦力补偿系数-挂装");
		strcpy(en_log_content, "Set friction compensation coefficient - upside down mounting");
		strcpy(jap_log_content, "摩擦力補償係数をセットする-マウント");
		ret = copy_content(data_json, content);
		break;
	case 544:
		port = cmdport;
		strcpy(log_content, "设置末端灯色对应机器人模式");
		strcpy(en_log_content, "Set the end light color corresponding to the robot mode");
		strcpy(jap_log_content, "エンドライト対応ロボットモードを設定");
		ret = copy_content(data_json, content);
		break;
	case 1001:/* 内部定义指令 */
		port = cmdport;
		strcpy(log_content, "单步执行指令");
		strcpy(en_log_content, "Step the instruction");
		strcpy(jap_log_content, "1ステップでコマンドを実行する");
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

	if (ret == PWD_FAIL) {
		perror("password fail");
		goto pwderror_end;
	}
	//printf("strlen(error_info) = %d\n", strlen(error_info));
	if (ret == FAIL && strlen(error_info) != 0) {
		//printf("error info = %s\n", error_info);
		goto end_error_info;
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
	/* send cmd to 8060 port */
	case CMD_PORT:
		ret = socket_enquene(&socket_cmd, cmd, content, cmd_type);
		break;
		/* send file cmd to 8062 port */
	case FILE_PORT:
		if (cmd != 106) {
			ret = socket_enquene(&socket_file, cmd, content, cmd_type);
		} else {
			ret = SUCCESS;
		}
		break;
		/* send cmd to 8070 port */
	case VIR_CMD_PORT:
		ret = socket_enquene(&socket_vir_cmd, cmd, content, cmd_type);
		break;
		/* send file cmd to 8072 port */
	case VIR_FILE_PORT:
		if (cmd != 106) {
			ret = socket_enquene(&socket_vir_file, cmd, content, cmd_type);
		} else {
			ret = SUCCESS;
		}
		break;
	default:
		perror("port");
		goto end;
	}

	if (ret == FAIL) {
		perror("socket fail");
		goto end;
	}

	/* wait 425 cmd feedback, 最多等待 1 秒 */
	if (cmd == 425) {
		ret = wait_cmd_feedback();
		if (ret == FAIL) {
			perror("cmd feedback fail");
			goto end;
		}
	}

	my_syslog("机器人操作", log_content, cur_account.username);
	my_en_syslog("robot operation", en_log_content, cur_account.username);
	my_jap_syslog("ロボット操作", jap_log_content, cur_account.username);
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
	my_syslog("机器人操作", "当前用户无相应机器人操作权限", cur_account.username);
	my_en_syslog("robot operation", "The current user has no corresponding robot operation permissions", cur_account.username);
	my_jap_syslog("ロボット操作", "現在のユーザには対応するロボットの操作権限がありません", cur_account.username);
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
	my_syslog("机器人操作", "机器人操作失败", cur_account.username);
	my_en_syslog("robot operation", "robot operation fail", cur_account.username);
	my_jap_syslog("ロボット操作", "ロボットの操作に失敗する", cur_account.username);
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

end_error_info:
	my_syslog("机器人操作", "lua 文件解析，发现错误", cur_account.username);
	my_en_syslog("robot operation", "The Lua file was parsed and an error was found", cur_account.username);
	my_jap_syslog("ロボット操作", "luaファイル解析、エラー発見", cur_account.username);
	/* free content */
	free(content);
	content = NULL;
	/* cjson delete */
	cJSON_Delete(data);
	data = NULL;
	websSetStatus(wp, 200);
	websWriteHeaders(wp, -1, 0);
	websWriteEndHeaders(wp);
	websWrite(wp, error_info);
	websDone(wp);
	return;

pwderror_end:
	my_syslog("机器人操作", "密码检验错误", cur_account.username);
	my_en_syslog("robot operation", "Password check error", cur_account.username);
	my_jap_syslog("ロボット操作", "暗号検証エラー", cur_account.username);
	/* free content */
	free(content);
	content = NULL;
	/* cjson delete */
	cJSON_Delete(data);
	data = NULL;
	websSetStatus(wp, 200);
	websWriteHeaders(wp, -1, 0);
	websWriteEndHeaders(wp);
	websWrite(wp, "pwd_error");
	websDone(wp);
	return;
}
