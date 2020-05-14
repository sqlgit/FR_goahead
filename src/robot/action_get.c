
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
	char *buf = NULL;
	const char s[2] = "\n";
	char *token = NULL;
	char *f_content = NULL;

	cJSON *root_json = NULL;

	root_json = cJSON_CreateObject();
	f_content = get_complete_file_content(ROBOT_CFG);
	/* file is NULL */
	if (f_content == NULL) {
		perror("get file content");

		return FAIL;
	}
	/* get first line */
	token = strtok(f_content, s);
	while (token != NULL) {
		//printf("token = %s\n", token);
		if(!strncmp(token, "SPEEDSCALE_AUTO = ", 18)) {
			strrpc(token, "SPEEDSCALE_AUTO = ", "");
			cJSON_AddStringToObject(root_json, "speedscale_auto", token);
		} else if(!strncmp(token, "SPEEDSCALE_MANUAL = ", 20)) {
			strrpc(token, "SPEEDSCALE_MANUAL = ", "");
			cJSON_AddStringToObject(root_json, "speedscale_manual", token);
		} else if(!strncmp(token, "TOOL_NUM = ", 11)) {
			strrpc(token, "TOOL_NUM = ", "");
			cJSON_AddStringToObject(root_json, "tool_num", token);
		} else if(!strncmp(token, "TOOL_X = ", 9)) {
			strrpc(token, "TOOL_X = ", "");
			cJSON_AddStringToObject(root_json, "tool_x", token);
		} else if(!strncmp(token, "TOOL_Y = ", 9)) {
			strrpc(token, "TOOL_Y = ", "");
			cJSON_AddStringToObject(root_json, "tool_y", token);
		} else if(!strncmp(token, "TOOL_Z = ", 9)) {
			strrpc(token, "TOOL_Z = ", "");
			cJSON_AddStringToObject(root_json, "tool_z", token);
		} else if(!strncmp(token, "TOOL_A = ", 9)) {
			strrpc(token, "TOOL_A = ", "");
			cJSON_AddStringToObject(root_json, "tool_a", token);
		} else if(!strncmp(token, "TOOL_B = ", 9)) {
			strrpc(token, "TOOL_B = ", "");
			cJSON_AddStringToObject(root_json, "tool_b", token);
		} else if(!strncmp(token, "TOOL_C = ", 9)) {
			strrpc(token, "TOOL_C = ", "");
			cJSON_AddStringToObject(root_json, "tool_c", token);
		} else if(!strncmp(token, "COLLISION_LEVEL = ", 18)) {
			strrpc(token, "COLLISION_LEVEL = ", "");
			cJSON_AddStringToObject(root_json, "collision_level", token);
		} else if(!strncmp(token, "LOAD_WEIGHT = ", 14)) {
			strrpc(token, "LOAD_WEIGHT = ", "");
			cJSON_AddStringToObject(root_json, "load_weight", token);
		} else if(!strncmp(token, "LOAD_Coord_X = ", 15)) {
			strrpc(token, "LOAD_Coord_X = ", "");
			cJSON_AddStringToObject(root_json, "load_coord_x", token);
		} else if(!strncmp(token, "LOAD_Coord_Y = ", 15)) {
			strrpc(token, "LOAD_Coord_Y = ", "");
			cJSON_AddStringToObject(root_json, "load_coord_y", token);
		} else if(!strncmp(token, "LOAD_Coord_Z = ", 15)) {
			strrpc(token, "LOAD_Coord_Z = ", "");
			cJSON_AddStringToObject(root_json, "load_coord_z", token);
		} else if(!strncmp(token, "J1_MAX_JOINT_LIMIT = ", 21)) {
			strrpc(token, "J1_MAX_JOINT_LIMIT = ", "");
			cJSON_AddStringToObject(root_json, "j1_max_joint_limit", token);
		} else if(!strncmp(token, "J1_MIN_JOINT_LIMIT = ", 21)) {
			strrpc(token, "J1_MIN_JOINT_LIMIT = ", "");
			cJSON_AddStringToObject(root_json, "j1_min_joint_limit", token);
		} else if(!strncmp(token, "J2_MAX_JOINT_LIMIT = ", 21)) {
			strrpc(token, "J2_MAX_JOINT_LIMIT = ", "");
			cJSON_AddStringToObject(root_json, "j2_max_joint_limit", token);
		} else if(!strncmp(token, "J2_MIN_JOINT_LIMIT = ", 21)) {
			strrpc(token, "J2_MIN_JOINT_LIMIT = ", "");
			cJSON_AddStringToObject(root_json, "j2_min_joint_limit", token);
		} else if(!strncmp(token, "J3_MAX_JOINT_LIMIT = ", 21)) {
			strrpc(token, "J3_MAX_JOINT_LIMIT = ", "");
			cJSON_AddStringToObject(root_json, "j3_max_joint_limit", token);
		} else if(!strncmp(token, "J3_MIN_JOINT_LIMIT = ", 21)) {
			strrpc(token, "J3_MIN_JOINT_LIMIT = ", "");
			cJSON_AddStringToObject(root_json, "j3_min_joint_limit", token);
		} else if(!strncmp(token, "J4_MAX_JOINT_LIMIT = ", 21)) {
			strrpc(token, "J4_MAX_JOINT_LIMIT = ", "");
			cJSON_AddStringToObject(root_json, "j4_max_joint_limit", token);
		} else if(!strncmp(token, "J4_MIN_JOINT_LIMIT = ", 21)) {
			strrpc(token, "J4_MIN_JOINT_LIMIT = ", "");
			cJSON_AddStringToObject(root_json, "j4_min_joint_limit", token);
		} else if(!strncmp(token, "J5_MAX_JOINT_LIMIT = ", 21)) {
			strrpc(token, "J5_MAX_JOINT_LIMIT = ", "");
			cJSON_AddStringToObject(root_json, "j5_max_joint_limit", token);
		} else if(!strncmp(token, "J5_MIN_JOINT_LIMIT = ", 21)) {
			strrpc(token, "J5_MIN_JOINT_LIMIT = ", "");
			cJSON_AddStringToObject(root_json, "j5_min_joint_limit", token);
		} else if(!strncmp(token, "J6_MAX_JOINT_LIMIT = ", 21)) {
			strrpc(token, "J6_MAX_JOINT_LIMIT = ", "");
			cJSON_AddStringToObject(root_json, "j6_max_joint_limit", token);
		} else if(!strncmp(token, "J6_MIN_JOINT_LIMIT = ", 21)) {
			strrpc(token, "J6_MIN_JOINT_LIMIT = ", "");
			cJSON_AddStringToObject(root_json, "j6_min_joint_limit", token);
		} else if(!strncmp(token, "COLLISION_ERROR_TIME = ", 23)) {
			strrpc(token, "COLLISION_ERROR_TIME = ", "");
			cJSON_AddStringToObject(root_json, "collision_error_time", token);
		} else if(!strncmp(token, "J1_COLLISIONVALUE = ", 20)) {
			strrpc(token, "J1_COLLISIONVALUE = ", "");
			cJSON_AddStringToObject(root_json, "j1_collisionvalue", token);
		} else if(!strncmp(token, "J2_COLLISIONVALUE = ", 20)) {
			strrpc(token, "J2_COLLISIONVALUE = ", "");
			cJSON_AddStringToObject(root_json, "j2_collisionvalue", token);
		} else if(!strncmp(token, "J3_COLLISIONVALUE = ", 20)) {
			strrpc(token, "J3_COLLISIONVALUE = ", "");
			cJSON_AddStringToObject(root_json, "j3_collisionvalue", token);
		} else if(!strncmp(token, "J4_COLLISIONVALUE = ", 20)) {
			strrpc(token, "J4_COLLISIONVALUE = ", "");
			cJSON_AddStringToObject(root_json, "j4_collisionvalue", token);
		} else if(!strncmp(token, "J5_COLLISIONVALUE = ", 20)) {
			strrpc(token, "J5_COLLISIONVALUE = ", "");
			cJSON_AddStringToObject(root_json, "j5_collisionvalue", token);
		} else if(!strncmp(token, "J6_COLLISIONVALUE = ", 20)) {
			strrpc(token, "J6_COLLISIONVALUE = ", "");
			cJSON_AddStringToObject(root_json, "j6_collisionvalue", token);
		} else if(!strncmp(token, "CTL_DI_FILTERTIME = ", 20)) {
			strrpc(token, "CTL_DI_FILTERTIME = ", "");
			cJSON_AddStringToObject(root_json, "ctl_di_filtertime", token);
		} else if(!strncmp(token, "AXLE_DI_FILTERTIME = ", 21)) {
			strrpc(token, "AXLE_DI_FILTERTIME = ", "");
			cJSON_AddStringToObject(root_json, "axle_di_filtertime", token);
		} else if(!strncmp(token, "CTL_AI0_FILTERTIME = ", 21)) {
			strrpc(token, "CTL_AI0_FILTERTIME = ", "");
			cJSON_AddStringToObject(root_json, "ctl_ai0_filtertime", token);
		} else if(!strncmp(token, "CTL_AI1_FILTERTIME = ", 21)) {
			strrpc(token, "CTL_AI1_FILTERTIME = ", "");
			cJSON_AddStringToObject(root_json, "ctl_ai1_filtertime", token);
		} else if(!strncmp(token, "AXLE_AI0_FILTERTIME = ", 22)) {
			strrpc(token, "AXLE_AI0_FILTERTIME = ", "");
			cJSON_AddStringToObject(root_json, "axle_ai0_filtertime", token);
		} else if(!strncmp(token, "CTL_DO8_CONFIG = ", 17)) {
			strrpc(token, "CTL_DO8_CONFIG = ", "");
			cJSON_AddStringToObject(root_json, "ctl_do8_config", token);
		} else if(!strncmp(token, "CTL_DO9_CONFIG = ", 17)) {
			strrpc(token, "CTL_DO9_CONFIG = ", "");
			cJSON_AddStringToObject(root_json, "ctl_do9_config", token);
		} else if(!strncmp(token, "CTL_DO10_CONFIG = ", 18)) {
			strrpc(token, "CTL_DO10_CONFIG = ", "");
			cJSON_AddStringToObject(root_json, "ctl_do10_config", token);
		} else if(!strncmp(token, "CTL_DO11_CONFIG = ", 18)) {
			strrpc(token, "CTL_DO11_CONFIG = ", "");
			cJSON_AddStringToObject(root_json, "ctl_do11_config", token);
		} else if(!strncmp(token, "CTL_DO12_CONFIG = ", 18)) {
			strrpc(token, "CTL_DO12_CONFIG = ", "");
			cJSON_AddStringToObject(root_json, "ctl_do12_config", token);
		} else if(!strncmp(token, "CTL_DO13_CONFIG = ", 18)) {
			strrpc(token, "CTL_DO13_CONFIG = ", "");
			cJSON_AddStringToObject(root_json, "ctl_do13_config", token);
		} else if(!strncmp(token, "CTL_DO14_CONFIG = ", 18)) {
			strrpc(token, "CTL_DO14_CONFIG = ", "");
			cJSON_AddStringToObject(root_json, "ctl_do14_config", token);
		} else if(!strncmp(token, "CTL_DO15_CONFIG = ", 18)) {
			strrpc(token, "CTL_DO15_CONFIG = ", "");
			cJSON_AddStringToObject(root_json, "ctl_do15_config", token);
		}
		/* get other line */
		token = strtok(NULL, s);
	}

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

/* get webserver data and return to page */
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

