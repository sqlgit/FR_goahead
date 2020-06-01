
/********************************* Includes ***********************************/

#include    "goahead.h"
#include	"cJSON.h"
#include 	"tools.h"
#include 	"robot_socket.h"
#include	"action_get.h"

/********************************* Defines ************************************/

extern STATE_FEEDBACK state_fb;
extern ACCOUNT_INFO cur_account;

/********************************* Function declaration ***********************/

static int get_points(char **ret_f_content);
static int get_tool_cdsystem(char **ret_f_content);
static int get_ex_tool_cdsystem(char **ret_f_content);
static int get_user(char **ret_f_content);
static int get_template(char **ret_f_content);
static int get_tpd_name(char **ret_f_content);
static int get_log_name(char **ret_f_content);
static int get_log_data(char **ret_f_content, const cJSON *data_json);
static int get_syscfg(char **ret_f_content);
static int get_accounts(char **ret_f_content);
static int get_robot_cfg(char **ret_f_content);

/*********************************** Code *************************************/

/* get points file content */
static int get_points(char **ret_f_content)
{
	char sql[1024] = { 0 };
	cJSON *JSON_Data = NULL;
	char *sqlite_select_data = NULL;
	sprintf(sql, "select * from points");
	if (select_info_json_sqlite3(DB_POINTS, sql, &JSON_Data) == -1) {
		memset(sql, 0, sizeof(sql));
		perror("select points\n");
	}

	sqlite_select_data = cJSON_Print(JSON_Data);
	cJSON_Delete(JSON_Data);
	printf("get_points: sqlite_select_data_json is \n %s\n", sqlite_select_data);

	free(sqlite_select_data);
	sqlite_select_data = NULL;
	JSON_Data = NULL;
	memset(sql, 0, sizeof(sql));

	*ret_f_content = get_file_content(FILE_POINTS);
	/* file is NULL */
	if (*ret_f_content == NULL) {
		perror("get file content");

		return FAIL;
	}

	return SUCCESS;
}

/* get tool coordinate system data */
static int get_tool_cdsystem(char **ret_f_content)
{
	char sql[1024] = { 0 };
	cJSON *JSON_Data = NULL;
	char *sqlite_select_data = NULL;
	sprintf(sql, "select * from coordinate_system");
	if (select_info_json_sqlite3(DB_CDSYSTEM, sql, &JSON_Data) == -1) {
		memset(sql, 0, sizeof(sql));
		perror("select coordinate_system\n");
	}

	sqlite_select_data = cJSON_Print(JSON_Data);
	cJSON_Delete(JSON_Data);
	JSON_Data = NULL;
	printf("get_tool_cdsystem: sqlite_select_data_json is \n %s\n",
			sqlite_select_data);

	free(sqlite_select_data);
	sqlite_select_data = NULL;
	memset(sql, 0, sizeof(sql));

	*ret_f_content = get_file_content(FILE_CDSYSTEM);
	/* file is NULL */
	if (*ret_f_content == NULL) {
		perror("get file content");

		return FAIL;
	}

	return SUCCESS;
}

/* get exter && tool coordinate system data */
static int get_ex_tool_cdsystem(char **ret_f_content)
{
	char sql[1024] = { 0 };
	cJSON *JSON_Data = NULL;
	char *sqlite_select_data = NULL;

	sprintf(sql, "select * from et_coordinate_system");
	if (select_info_json_sqlite3(DB_ET_CDSYSTEM, sql, &JSON_Data) == -1) {
		memset(sql, 0, sizeof(sql));
		perror("select ex_tool_cdsystem\n");
	}

	sqlite_select_data = cJSON_Print(JSON_Data);
	cJSON_Delete(JSON_Data);
	JSON_Data = NULL;
	printf("get_ex_tool_cdsystem: sqlite_select_data_json is \n %s\n",
			sqlite_select_data);

	free(sqlite_select_data);
	sqlite_select_data = NULL;
	memset(sql, 0, sizeof(sql));

	*ret_f_content = get_file_content(FILE_ET_CDSYSTEM);
	/* file is NULL */
	if (*ret_f_content == NULL) {
		perror("get file content");

		return FAIL;
	}

	return SUCCESS;
}

/* get user file content */
static int get_user(char **ret_f_content)
{
	*ret_f_content = get_dir_content(DIR_USER);
	/* file is NULL */
	if (*ret_f_content == NULL) {
		perror("get dir content");

		return FAIL;
	}

	return SUCCESS;
}

/* get template file content */
static int get_template(char **ret_f_content)
{
	*ret_f_content = get_dir_content(DIR_TEMPLATE);
	/* file is NULL */
	if (*ret_f_content == NULL) {
		perror("get dir content");

		return FAIL;
	}

	return SUCCESS;
}

/* get DIR FRUSER .txt file, TPD file */
static int get_tpd_name(char **ret_f_content)
{
	*ret_f_content = get_dir_filename_txt(DIR_FRUSER);
	/* file is NULL */
	if (*ret_f_content == NULL) {
		perror("get dir content");

		return FAIL;
	}

	return SUCCESS;
}

/* get log name */
static int get_log_name(char **ret_f_content)
{
	*ret_f_content = get_dir_filename(DIR_LOG);
	/* file is NULL */
	if (*ret_f_content == NULL) {
		perror("get dir content");

		return FAIL;
	}

	return SUCCESS;
}

/* get log data */
static int get_log_data(char **ret_f_content, const cJSON *data_json)
{
	char dir_filename[100] = {0};

	cJSON *file_name = cJSON_GetObjectItem(data_json, "name");
	if (file_name == NULL || file_name->valuestring == NULL) {
		perror("json");

		return FAIL;
	}
	sprintf(dir_filename, "%s%s", DIR_LOG, file_name->valuestring);
	*ret_f_content = get_file_content(dir_filename);
	/* file is NULL */
	if (*ret_f_content == NULL) {
		perror("get file content");

		return FAIL;
	}

	return SUCCESS;
}

/* get system cfg and return to page */
static int get_syscfg(char **ret_f_content)
{
	*ret_f_content = get_file_content(FILE_CFG);
	/* file is NULL */
	if (*ret_f_content == NULL) {
		perror("get file content");

		return FAIL;
	}

	return SUCCESS;
}

/* get account file and return to page */
static int get_accounts(char **ret_f_content)
{
	*ret_f_content = get_file_content(FILE_ACCOUNT);
	/* file is NULL */
	if (*ret_f_content == NULL) {
		perror("get file content");

		return FAIL;
	}

	return SUCCESS;
}

/* get account info */
static int get_account_info(char **ret_f_content)
{
	cJSON *root_json = NULL;
	char *buf = NULL;

	root_json = cJSON_CreateObject();
	cJSON_AddStringToObject(root_json, "username", cur_account.username);
	cJSON_AddStringToObject(root_json, "auth", cur_account.auth);
	buf = cJSON_Print(root_json);
	//printf("buf = %s\n", buf);
	*ret_f_content = (char *)calloc(1, strlen(buf)+1);
	if(*ret_f_content != NULL) {
		strcpy((*ret_f_content), buf);
	} else {
		perror("calloc");

		return FAIL;
	}
	//printf("*ret_f_content = %s\n", (*ret_f_content));
	free(buf);
	buf = NULL;
	cJSON_Delete(root_json);
	root_json = NULL;

	return SUCCESS;
}

/* get robot cfg and return to page */
static int get_robot_cfg(char **ret_f_content)
{
	char strline[LINE_LEN] = {};
	char *buf = NULL;
	FILE *fp;

	cJSON *root_json = NULL;

	root_json = cJSON_CreateObject();
	if ((fp = fopen(ROBOT_CFG, "r")) == NULL) {
		perror("open file");

		return FAIL;
	}
	while (fgets(strline, LINE_LEN, fp) != NULL) {
		strrpc(strline, "\n", "");
		if(!strncmp(strline, "SPEEDSCALE_AUTO = ", 18)) {
			strrpc(strline, "SPEEDSCALE_AUTO = ", "");
			cJSON_AddStringToObject(root_json, "speedscale_auto", strline);
		} else if(!strncmp(strline, "SPEEDSCALE_MANUAL = ", 20)) {
			strrpc(strline, "SPEEDSCALE_MANUAL = ", "");
			cJSON_AddStringToObject(root_json, "speedscale_manual", strline);
		} else if(!strncmp(strline, "TOOL_NUM = ", 11)) {
			strrpc(strline, "TOOL_NUM = ", "");
			cJSON_AddStringToObject(root_json, "tool_num", strline);
		} else if(!strncmp(strline, "TOOL_X = ", 9)) {
			strrpc(strline, "TOOL_X = ", "");
			cJSON_AddStringToObject(root_json, "tool_x", strline);
		} else if(!strncmp(strline, "TOOL_Y = ", 9)) {
			strrpc(strline, "TOOL_Y = ", "");
			cJSON_AddStringToObject(root_json, "tool_y", strline);
		} else if(!strncmp(strline, "TOOL_Z = ", 9)) {
			strrpc(strline, "TOOL_Z = ", "");
			cJSON_AddStringToObject(root_json, "tool_z", strline);
		} else if(!strncmp(strline, "TOOL_A = ", 9)) {
			strrpc(strline, "TOOL_A = ", "");
			cJSON_AddStringToObject(root_json, "tool_a", strline);
		} else if(!strncmp(strline, "TOOL_B = ", 9)) {
			strrpc(strline, "TOOL_B = ", "");
			cJSON_AddStringToObject(root_json, "tool_b", strline);
		} else if(!strncmp(strline, "TOOL_C = ", 9)) {
			strrpc(strline, "TOOL_C = ", "");
			cJSON_AddStringToObject(root_json, "tool_c", strline);
		} else if(!strncmp(strline, "COLLISION_LEVEL = ", 18)) {
			strrpc(strline, "COLLISION_LEVEL = ", "");
			cJSON_AddStringToObject(root_json, "collision_level", strline);
		} else if(!strncmp(strline, "LOAD_WEIGHT = ", 14)) {
			strrpc(strline, "LOAD_WEIGHT = ", "");
			cJSON_AddStringToObject(root_json, "load_weight", strline);
		} else if(!strncmp(strline, "LOAD_Coord_X = ", 15)) {
			strrpc(strline, "LOAD_Coord_X = ", "");
			cJSON_AddStringToObject(root_json, "load_coord_x", strline);
		} else if(!strncmp(strline, "LOAD_Coord_Y = ", 15)) {
			strrpc(strline, "LOAD_Coord_Y = ", "");
			cJSON_AddStringToObject(root_json, "load_coord_y", strline);
		} else if(!strncmp(strline, "LOAD_Coord_Z = ", 15)) {
			strrpc(strline, "LOAD_Coord_Z = ", "");
			cJSON_AddStringToObject(root_json, "load_coord_z", strline);
		} else if(!strncmp(strline, "J1_MAX_JOINT_LIMIT = ", 21)) {
			strrpc(strline, "J1_MAX_JOINT_LIMIT = ", "");
			cJSON_AddStringToObject(root_json, "j1_max_joint_limit", strline);
		} else if(!strncmp(strline, "J1_MIN_JOINT_LIMIT = ", 21)) {
			strrpc(strline, "J1_MIN_JOINT_LIMIT = ", "");
			cJSON_AddStringToObject(root_json, "j1_min_joint_limit", strline);
		} else if(!strncmp(strline, "J2_MAX_JOINT_LIMIT = ", 21)) {
			strrpc(strline, "J2_MAX_JOINT_LIMIT = ", "");
			cJSON_AddStringToObject(root_json, "j2_max_joint_limit", strline);
		} else if(!strncmp(strline, "J2_MIN_JOINT_LIMIT = ", 21)) {
			strrpc(strline, "J2_MIN_JOINT_LIMIT = ", "");
			cJSON_AddStringToObject(root_json, "j2_min_joint_limit", strline);
		} else if(!strncmp(strline, "J3_MAX_JOINT_LIMIT = ", 21)) {
			strrpc(strline, "J3_MAX_JOINT_LIMIT = ", "");
			cJSON_AddStringToObject(root_json, "j3_max_joint_limit", strline);
		} else if(!strncmp(strline, "J3_MIN_JOINT_LIMIT = ", 21)) {
			strrpc(strline, "J3_MIN_JOINT_LIMIT = ", "");
			cJSON_AddStringToObject(root_json, "j3_min_joint_limit", strline);
		} else if(!strncmp(strline, "J4_MAX_JOINT_LIMIT = ", 21)) {
			strrpc(strline, "J4_MAX_JOINT_LIMIT = ", "");
			cJSON_AddStringToObject(root_json, "j4_max_joint_limit", strline);
		} else if(!strncmp(strline, "J4_MIN_JOINT_LIMIT = ", 21)) {
			strrpc(strline, "J4_MIN_JOINT_LIMIT = ", "");
			cJSON_AddStringToObject(root_json, "j4_min_joint_limit", strline);
		} else if(!strncmp(strline, "J5_MAX_JOINT_LIMIT = ", 21)) {
			strrpc(strline, "J5_MAX_JOINT_LIMIT = ", "");
			cJSON_AddStringToObject(root_json, "j5_max_joint_limit", strline);
		} else if(!strncmp(strline, "J5_MIN_JOINT_LIMIT = ", 21)) {
			strrpc(strline, "J5_MIN_JOINT_LIMIT = ", "");
			cJSON_AddStringToObject(root_json, "j5_min_joint_limit", strline);
		} else if(!strncmp(strline, "J6_MAX_JOINT_LIMIT = ", 21)) {
			strrpc(strline, "J6_MAX_JOINT_LIMIT = ", "");
			cJSON_AddStringToObject(root_json, "j6_max_joint_limit", strline);
		} else if(!strncmp(strline, "J6_MIN_JOINT_LIMIT = ", 21)) {
			strrpc(strline, "J6_MIN_JOINT_LIMIT = ", "");
			cJSON_AddStringToObject(root_json, "j6_min_joint_limit", strline);
		} else if(!strncmp(strline, "COLLISION_ERROR_TIME = ", 23)) {
			strrpc(strline, "COLLISION_ERROR_TIME = ", "");
			cJSON_AddStringToObject(root_json, "collision_error_time", strline);
		} else if(!strncmp(strline, "J1_COLLISIONVALUE = ", 20)) {
			strrpc(strline, "J1_COLLISIONVALUE = ", "");
			cJSON_AddStringToObject(root_json, "j1_collisionvalue", strline);
		} else if(!strncmp(strline, "J2_COLLISIONVALUE = ", 20)) {
			strrpc(strline, "J2_COLLISIONVALUE = ", "");
			cJSON_AddStringToObject(root_json, "j2_collisionvalue", strline);
		} else if(!strncmp(strline, "J3_COLLISIONVALUE = ", 20)) {
			strrpc(strline, "J3_COLLISIONVALUE = ", "");
			cJSON_AddStringToObject(root_json, "j3_collisionvalue", strline);
		} else if(!strncmp(strline, "J4_COLLISIONVALUE = ", 20)) {
			strrpc(strline, "J4_COLLISIONVALUE = ", "");
			cJSON_AddStringToObject(root_json, "j4_collisionvalue", strline);
		} else if(!strncmp(strline, "J5_COLLISIONVALUE = ", 20)) {
			strrpc(strline, "J5_COLLISIONVALUE = ", "");
			cJSON_AddStringToObject(root_json, "j5_collisionvalue", strline);
		} else if(!strncmp(strline, "J6_COLLISIONVALUE = ", 20)) {
			strrpc(strline, "J6_COLLISIONVALUE = ", "");
			cJSON_AddStringToObject(root_json, "j6_collisionvalue", strline);
		} else if(!strncmp(strline, "CTL_DI_FILTERTIME = ", 20)) {
			strrpc(strline, "CTL_DI_FILTERTIME = ", "");
			cJSON_AddStringToObject(root_json, "ctl_di_filtertime", strline);
		} else if(!strncmp(strline, "AXLE_DI_FILTERTIME = ", 21)) {
			strrpc(strline, "AXLE_DI_FILTERTIME = ", "");
			cJSON_AddStringToObject(root_json, "axle_di_filtertime", strline);
		} else if(!strncmp(strline, "CTL_AI0_FILTERTIME = ", 21)) {
			strrpc(strline, "CTL_AI0_FILTERTIME = ", "");
			cJSON_AddStringToObject(root_json, "ctl_ai0_filtertime", strline);
		} else if(!strncmp(strline, "CTL_AI1_FILTERTIME = ", 21)) {
			strrpc(strline, "CTL_AI1_FILTERTIME = ", "");
			cJSON_AddStringToObject(root_json, "ctl_ai1_filtertime", strline);
		} else if(!strncmp(strline, "AXLE_AI0_FILTERTIME = ", 22)) {
			strrpc(strline, "AXLE_AI0_FILTERTIME = ", "");
			cJSON_AddStringToObject(root_json, "axle_ai0_filtertime", strline);
		} else if(!strncmp(strline, "CTL_DO8_CONFIG = ", 17)) {
			strrpc(strline, "CTL_DO8_CONFIG = ", "");
			cJSON_AddStringToObject(root_json, "ctl_do8_config", strline);
		} else if(!strncmp(strline, "CTL_DO9_CONFIG = ", 17)) {
			strrpc(strline, "CTL_DO9_CONFIG = ", "");
			cJSON_AddStringToObject(root_json, "ctl_do9_config", strline);
		} else if(!strncmp(strline, "CTL_DO10_CONFIG = ", 18)) {
			strrpc(strline, "CTL_DO10_CONFIG = ", "");
			cJSON_AddStringToObject(root_json, "ctl_do10_config", strline);
		} else if(!strncmp(strline, "CTL_DO11_CONFIG = ", 18)) {
			strrpc(strline, "CTL_DO11_CONFIG = ", "");
			cJSON_AddStringToObject(root_json, "ctl_do11_config", strline);
		} else if(!strncmp(strline, "CTL_DO12_CONFIG = ", 18)) {
			strrpc(strline, "CTL_DO12_CONFIG = ", "");
			cJSON_AddStringToObject(root_json, "ctl_do12_config", strline);
		} else if(!strncmp(strline, "CTL_DO13_CONFIG = ", 18)) {
			strrpc(strline, "CTL_DO13_CONFIG = ", "");
			cJSON_AddStringToObject(root_json, "ctl_do13_config", strline);
		} else if(!strncmp(strline, "CTL_DO14_CONFIG = ", 18)) {
			strrpc(strline, "CTL_DO14_CONFIG = ", "");
			cJSON_AddStringToObject(root_json, "ctl_do14_config", strline);
		} else if(!strncmp(strline, "CTL_DO15_CONFIG = ", 18)) {
			strrpc(strline, "CTL_DO15_CONFIG = ", "");
			cJSON_AddStringToObject(root_json, "ctl_do15_config", strline);
		}
	}

	buf = cJSON_Print(root_json);
	//printf("buf = %s\n", buf);
	*ret_f_content = (char *)calloc(1, strlen(buf)+1);
	if(*ret_f_content != NULL) {
		strcpy((*ret_f_content), buf);
	} else {
		perror("calloc");
		free(buf);
		buf = NULL;
		cJSON_Delete(root_json);
		root_json = NULL;

		return FAIL;
	}
	//printf("*ret_f_content = %s\n", (*ret_f_content));
	free(buf);
	buf = NULL;
	cJSON_Delete(root_json);
	root_json = NULL;

	return SUCCESS;
}

/* get web data and return to page */
void get(Webs *wp)
{
	/*
	if (wp->user) {
		printf("__LINE__ = %d\n", __LINE__);
		printf("wp->user->name = %s\n", wp->user->name);
	} else {
		printf("__LINE__ = %d\n", __LINE__);
	}
	if (wp->username) {
		printf("wp->username = %s\n", wp->username);
	} else {
		printf("__LINE__ = %d\n", __LINE__);
	}
	*/
	int ret = FAIL;
	char *buf = NULL;
	char *cmd = NULL;
	char *ret_f_content = NULL;
	cJSON *data_json = NULL;
	cJSON *command = NULL;
	cJSON *data = NULL;

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

	// cmd_auth "0"
	if (!strcmp(cmd, "get_accounts")) {
		if (!authority_management("0")) {
			perror("authority_management");
			goto auth_end;
		}
	}

	if(!strcmp(cmd, "get_points")) {
		ret = get_points(&ret_f_content);
	} else if(!strcmp(cmd, "get_tool_cdsystem")) {
		ret = get_tool_cdsystem(&ret_f_content);
	} else if(!strcmp(cmd, "get_ex_tool_cdsystem")) {
		ret = get_ex_tool_cdsystem(&ret_f_content);
	} else if(!strcmp(cmd, "get_user_data")) {
		ret = get_user(&ret_f_content);
	} else if(!strcmp(cmd, "get_template_data")) {
		ret = get_template(&ret_f_content);
	} else if(!strcmp(cmd, "get_tpd_name")) {
		ret = get_tpd_name(&ret_f_content);
	} else if(!strcmp(cmd, "get_log_name")) {
		ret = get_log_name(&ret_f_content);
	} else if(!strcmp(cmd, "get_log_data")) {
		/* get data json */
		data_json = cJSON_GetObjectItem(data, "data");
		if (data_json == NULL || data_json->type != cJSON_Object) {
			perror("json");
			goto end;
		}
		ret = get_log_data(&ret_f_content, data_json);
	} else if(!strcmp(cmd, "get_syscfg")) {
		ret = get_syscfg(&ret_f_content);
	} else if(!strcmp(cmd, "get_accounts")) {
		ret = get_accounts(&ret_f_content);
	} else if(!strcmp(cmd, "get_account_info")) {
		ret = get_account_info(&ret_f_content);
	} else if(!strcmp(cmd, "get_robot_cfg")) {
		ret = get_robot_cfg(&ret_f_content);
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

auth_end:
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

