/********************************* Includes ***********************************/

#include    "goahead.h"
#include	"cJSON.h"
#include 	"tools.h"
#include 	"robot_socket.h"
#include	"action_get.h"

/********************************* Defines ************************************/

extern STATE_FEEDBACK state_fb;
extern ACCOUNT_INFO cur_account;
extern int language;

/********************************* Function declaration ***********************/

static int get_points(char **ret_f_content);
static int get_tool_cdsystem(char **ret_f_content);
static int get_wobj_tool_cdsystem(char **ret_f_content);
static int get_ex_tool_cdsystem(char **ret_f_content);
static int get_exaxis_cdsystem(char **ret_f_content);
static int get_user(char **ret_f_content);
static int get_template(char **ret_f_content);
static int get_tpd_name(char **ret_f_content);
static int get_log_name(char **ret_f_content);
static int get_log_data(char **ret_f_content, const cJSON *data_json);
static int get_syscfg(char **ret_f_content);
static int get_ODM_cfg(char **ret_f_content);
static int get_accounts(char **ret_f_content);
static int get_account_info(char **ret_f_content);
static int get_webversion(char **ret_f_content);
static int get_checkpoint(char **ret_f_content, const cJSON *data_json);
static int get_robot_cfg(char **ret_f_content);
static int get_ex_device_cfg(char **ret_f_content);
static int get_weave(char **ret_f_content);
static int get_exaxis_cfg(char **ret_f_content);
static int get_plugin_info(char **ret_f_content);
static int get_plugin_nav(char **ret_f_content);
static int get_plugin_config(char **ret_f_content, const cJSON *data_json);
static int get_DH_file(char **ret_f_content);
static int get_robot_serialnumber(char **ret_f_content);
static int get_robot_type(char **ret_f_content);
static int get_TSP_flg(char **ret_f_content);
static int torque_get_wpname_list(char **ret_f_content);
static int torque_get_cfg(char **ret_f_content, const cJSON *data_json);
static int torque_get_points(char **ret_f_content, const cJSON *data_json);
static int get_DIO_cfg(char **ret_f_content);
static int get_varlist(char **ret_f_content);
static int get_checkvar(char **ret_f_content, const cJSON *data_json);
static int torque_get_wkpoints(char **ret_f_content, const cJSON *data_json);
static int torque_get_ptemp_list(char **ret_f_content);
static int torque_get_main_list(char **ret_f_content);
static int torque_get_custom_pause(char **ret_f_content, const cJSON *data_json);
static int torque_get_all_custom_pause(char **ret_f_content);
static int get_ip(char **ret_f_content);
static int get_blockly_workspace(char **ret_f_content, const cJSON *data_json);
static int get_blockly_workspace_names(char **ret_f_content);
//static int index_get_config = 0;

/*********************************** Code *************************************/

/* get points file content */
static int get_points(char **ret_f_content)
{
	char sql[1024] = {0};
	cJSON *json_data = NULL;

	sprintf(sql, "select * from points");
	if (select_info_json_sqlite3_reversed_order(DB_POINTS, sql, &json_data) == -1) {
		perror("select points");

		return FAIL;
	}

	*ret_f_content = cJSON_Print(json_data);
	cJSON_Delete(json_data);
	json_data = NULL;
	/* content is NULL */
	if (*ret_f_content == NULL) {
		perror("cJSON_Print");

		return FAIL;
	}

	return SUCCESS;
}

/* get tool coordinate system data */
static int get_tool_cdsystem(char **ret_f_content)
{
	char sql[1024] = {0};
	cJSON *json_data = NULL;

	// 此处的 ASC 排序不需要了， 数据库创建时已经以 id 为主键并设置了 ASC 排序
	sprintf(sql, "select * from coordinate_system order by id ASC");
	//sprintf(sql, "select * from coordinate_system");
	if (select_info_nokey_json_sqlite3(DB_CDSYSTEM, sql, &json_data) == -1) {
	//if (select_info_json_sqlite3(DB_CDSYSTEM, sql, &json_data) == -1) {
		perror("select coordinate_system");

		return FAIL;
	}

	*ret_f_content = cJSON_Print(json_data);
	cJSON_Delete(json_data);
	json_data = NULL;
	/* content is NULL */
	if (*ret_f_content == NULL) {
		perror("cJSON_Print");

		return FAIL;
	}

	return SUCCESS;
}

/* get wobj tool coordinate system data */
static int get_wobj_tool_cdsystem(char **ret_f_content)
{
	char sql[1024] = {0};
	cJSON *json_data = NULL;

	sprintf(sql, "select * from wobj_coordinate_system order by id ASC");
	if (select_info_json_sqlite3(DB_WOBJ_CDSYSTEM, sql, &json_data) == -1) {
		perror("select wobj coordinate_system");

		return FAIL;
	}

	*ret_f_content = cJSON_Print(json_data);
	cJSON_Delete(json_data);
	json_data = NULL;
	/* content is NULL */
	if (*ret_f_content == NULL) {
		perror("cJSON_Print");

		return FAIL;
	}

	return SUCCESS;
}

/* get exter && tool coordinate system data */
static int get_ex_tool_cdsystem(char **ret_f_content)
{
	char sql[1024] = {0};
	cJSON *json_data = NULL;

	sprintf(sql, "select * from et_coordinate_system order by id ASC");
	if (select_info_json_sqlite3(DB_ET_CDSYSTEM, sql, &json_data) == -1) {
		perror("select ex_tool_cdsystem");

		return FAIL;
	}

	*ret_f_content = cJSON_Print(json_data);
	cJSON_Delete(json_data);
	json_data = NULL;
	/* content is NULL */
	if (*ret_f_content == NULL) {
		perror("cJSON_Print");

		return FAIL;
	}

	return SUCCESS;
}

/* get exaxis coordinate system data */
static int get_exaxis_cdsystem(char **ret_f_content)
{
	char sql[1024] = {0};
	cJSON *json_data = NULL;

	sprintf(sql, "select * from exaxis_coordinate_system order by id ASC");
	if (select_info_json_sqlite3(DB_EXAXIS_CDSYSTEM, sql, &json_data) == -1) {
		perror("select exaxis_cdsystem");

		return FAIL;
	}

	*ret_f_content = cJSON_Print(json_data);
	cJSON_Delete(json_data);
	json_data = NULL;
	/* content is NULL */
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
	if (language == 0){
		*ret_f_content = get_dir_filename(DIR_LOG);
	}
	if (language == 1){
		*ret_f_content = get_dir_filename(DIR_LOG_EN);
	}
	if (language == 2){
		*ret_f_content = get_dir_filename(DIR_LOG_JAP);
	}
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
	if (language == 0){
		sprintf(dir_filename, "%s%s", DIR_LOG, file_name->valuestring);
	}
	if (language == 1){
		sprintf(dir_filename, "%s%s", DIR_LOG_EN, file_name->valuestring);
	}
	if (language == 2){
		sprintf(dir_filename, "%s%s", DIR_LOG_JAP, file_name->valuestring);
	}
	*ret_f_content = get_file_content(dir_filename);
	/* ret_f_content is NULL or no such file or empty */
	if (*ret_f_content == NULL || strcmp(*ret_f_content, "NO_FILE") == 0 || strcmp(*ret_f_content, "Empty") == 0) {
		*ret_f_content = NULL;
		perror("get file content");

		return FAIL;
	}

	return SUCCESS;
}

/* get system cfg and return to page */
static int get_syscfg(char **ret_f_content)
{
	*ret_f_content = get_file_content(FILE_CFG);
	/* ret_f_content is NULL or no such file or empty */
	if (*ret_f_content == NULL || strcmp(*ret_f_content, "NO_FILE") == 0 || strcmp(*ret_f_content, "Empty") == 0) {
		*ret_f_content = NULL;
		perror("get file content");

		return FAIL;
	}

	return SUCCESS;
}

/* get ODM cfg and return to page */
static int get_ODM_cfg(char **ret_f_content)
{
	*ret_f_content = get_file_content(FILE_ODM_CFG);
	/* ret_f_content is NULL or no such file or empty */
	if (*ret_f_content == NULL || strcmp(*ret_f_content, "NO_FILE") == 0 || strcmp(*ret_f_content, "Empty") == 0) {
		*ret_f_content = NULL;
		perror("get file content");

		return FAIL;
	}

	return SUCCESS;
}

/* get account file and return to page */
static int get_accounts(char **ret_f_content)
{
	char sql[1024] = {0};
	cJSON *json_data = NULL;

	sprintf(sql, "select * from account");
	if (select_info_nokey_json_sqlite3(DB_ACCOUNT, sql, &json_data) == -1) {
		perror("select account");

		return FAIL;
	}

	*ret_f_content = cJSON_Print(json_data);
	cJSON_Delete(json_data);
	json_data = NULL;
	/* content is NULL */
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

	root_json = cJSON_CreateObject();
	cJSON_AddStringToObject(root_json, "username", cur_account.username);
	cJSON_AddStringToObject(root_json, "auth", cur_account.auth);
	*ret_f_content = cJSON_Print(root_json);
	cJSON_Delete(root_json);
	root_json = NULL;
	if (*ret_f_content == NULL) {
		perror("cJSON_Print");

		return FAIL;
	}
	//printf("*ret_f_content = %s\n", (*ret_f_content));

	return SUCCESS;
}

/* get web version */
static int get_webversion(char **ret_f_content)
{
	char *strline = NULL;
	strline = (char *)calloc(1, sizeof(char)*LINE_LEN);
	char version[LINE_LEN] = {0};
	FILE *fp;
	cJSON *root_json = NULL;

	root_json = cJSON_CreateObject();
	if ((fp = fopen(README_WEB_NOW, "r")) == NULL) {
		perror("open file");

		return FAIL;
	}
	while (fgets(strline, LINE_LEN, fp) != NULL) {
		if(!strncmp(strline, "VERSION=", 8)) {
			strncpy(version, (strline + 8), (strlen(strline) - 8 - 1));
			cJSON_AddStringToObject(root_json, "version", version);

			break;
		}
		bzero(strline, sizeof(char)*LINE_LEN);
	}
	free(strline);
	strline = NULL;
	fclose(fp);

	*ret_f_content = cJSON_Print(root_json);
	cJSON_Delete(root_json);
	root_json = NULL;
	if (*ret_f_content == NULL) {
		perror("cJSON_Print");

		return FAIL;
	}
	//printf("*ret_f_content = %s\n", (*ret_f_content));

	return SUCCESS;
}

/* check point result */
static int get_checkpoint(char **ret_f_content, const cJSON *data_json)
{
	char sql[1024] = {0};
	cJSON *name = NULL;
	cJSON *root_json = NULL;
	cJSON *json_data = NULL;

	name = cJSON_GetObjectItem(data_json, "name");
	if (name == NULL || name->valuestring == NULL) {
		perror("json");

		return FAIL;
	}
	root_json = cJSON_CreateObject();
	sprintf(sql, "select * from points where name = \'%s\';", name->valuestring);
	if (select_info_json_sqlite3(DB_POINTS, sql, &json_data) == -1) {
		printf("check point result : not exist!");
		cJSON_AddStringToObject(root_json, "result", "0");
	} else {
		printf("check point result : exist!");
		cJSON_AddStringToObject(root_json, "result", "1");
	}
	cJSON_Delete(json_data);
	json_data = NULL;

	*ret_f_content = cJSON_Print(root_json);
	cJSON_Delete(root_json);
	root_json = NULL;
	if (*ret_f_content == NULL) {
		perror("cJSON_Print");

		return FAIL;
	}
	//printf("*ret_f_content = %s\n", (*ret_f_content));

	return SUCCESS;
}

/* get robot cfg and return to page */
static int get_robot_cfg(char **ret_f_content)
{
	char **array = NULL;
	int size = 0;
	char strline[LINE_LEN] = {0};
	FILE *fp = NULL;
	int line_num = 0;
	cJSON *root_json = NULL;
	//struct timespec start, stop, end;

	//index_get_config++;
	//printf("get config index : %d\n", index_get_config);
	//clock_gettime(CLOCK_REALTIME, &start);
	//printf("before fopen, %d, %ld\n", start.tv_sec, start.tv_nsec);
	if ((fp = fopen(USER_CFG, "r")) == NULL) {
		perror("user.config : open file");

		return FAIL;
	}
	//clock_gettime(CLOCK_REALTIME, &stop);
	//printf("after fopen, %d, %ld\n", stop.tv_sec, stop.tv_nsec);
	root_json = cJSON_CreateObject();
	while (fgets(strline, LINE_LEN, fp) != NULL) {
		if (is_in(strline, "=") == -1) {
			continue;
		}
		strrpc(strline, "\n", "");
		//printf("user.config strline = %s\n", strline);
		if (string_to_string_list(strline, " = ", &size, &array) == 0 || size != 2) {
			perror("string to string list");
			fclose(fp);
			string_list_free(&array, size);
			cJSON_Delete(root_json);
			root_json = NULL;

			return FAIL;
		}
		if (array[0] != NULL) {
			cJSON_AddStringToObject(root_json, my_strlwr(array[0]), array[1]);
		}
		string_list_free(&array, size);
		bzero(strline, sizeof(char)*LINE_LEN);
		line_num++;
	}
	fclose(fp);
	//clock_gettime(CLOCK_REALTIME, &end);
	//printf("after fclose, %d, %ld\n", end.tv_sec, end.tv_nsec);
	//printf("line_num = %d\n", line_num);
	*ret_f_content = cJSON_Print(root_json);
	cJSON_Delete(root_json);
	root_json = NULL;
	if (*ret_f_content == NULL) {
		perror("user.config : cJSON_Print");

		return FAIL;
	}
	//printf("user.config : content = %s\n", (*ret_f_content));

	return SUCCESS;
}

/* get ex device cfg and return to page */
static int get_ex_device_cfg(char **ret_f_content)
{
	char **array = NULL;
	int size = 0;
	char strline[LINE_LEN] = {0};
	FILE *fp = NULL;
	int line_num = 0;
	cJSON *root_json = NULL;

	root_json = cJSON_CreateObject();
	if ((fp = fopen(EX_DEVICE_CFG, "r")) == NULL) {
		perror("ex_device.config : open file");

		return FAIL;
	}
	while (fgets(strline, LINE_LEN, fp) != NULL) {
		if (is_in(strline, "=") == -1) {
			continue;
		}
		strrpc(strline, "\n", "");
		//printf("user.config strline = %s\n", strline);
		if (string_to_string_list(strline, " = ", &size, &array) == 0 || size != 2) {
			perror("string to string list");
			fclose(fp);
			string_list_free(&array, size);
			cJSON_Delete(root_json);
			root_json = NULL;

			return FAIL;
		}
		if (array[0] != NULL) {
			cJSON_AddStringToObject(root_json, my_strlwr(array[0]), array[1]);
		}
		string_list_free(&array, size);
		bzero(strline, sizeof(char)*LINE_LEN);
		line_num++;
	}
	fclose(fp);
	//printf("line_num = %d\n", line_num);
	*ret_f_content = cJSON_Print(root_json);
	cJSON_Delete(root_json);
	root_json = NULL;
	if (*ret_f_content == NULL) {
		perror("ex_device.config : cJSON_Print");

		return FAIL;
	}

	return SUCCESS;
}

/** get exaxis cfg */
static int get_exaxis_cfg(char **ret_f_content)
{
	char strline[LINE_LEN] = {0};
	FILE *fp;
	cJSON *root_json = NULL;
	cJSON *axis_json = NULL;
	cJSON *item1 = NULL;
	cJSON *item2 = NULL;
	cJSON *item3 = NULL;
	cJSON *item4 = NULL;

	if ((fp = fopen(EXAXIS_CFG, "r")) == NULL) {
		perror("open file");

		return FAIL;
	}
	root_json = cJSON_CreateObject();
	axis_json = cJSON_CreateArray();
	cJSON_AddItemToObject(root_json, "cfg", axis_json);
	item1 = cJSON_CreateObject();
	item2 = cJSON_CreateObject();
	item3 = cJSON_CreateObject();
	item4 = cJSON_CreateObject();
	cJSON_AddItemToArray(axis_json, item1);
	cJSON_AddItemToArray(axis_json, item2);
	cJSON_AddItemToArray(axis_json, item3);
	cJSON_AddItemToArray(axis_json, item4);
	cJSON_AddStringToObject(item1, "axis_id", "1");
	cJSON_AddStringToObject(item2, "axis_id", "2");
	cJSON_AddStringToObject(item3, "axis_id", "3");
	cJSON_AddStringToObject(item4, "axis_id", "4");

	while (fgets(strline, LINE_LEN, fp) != NULL) {
		strrpc(strline, "\n", "");
		if(!strncmp(strline, "EXTERNALAXIS1_TYPE = ", 21)) {
			strrpc(strline, "EXTERNALAXIS1_TYPE = ", "");
			cJSON_AddStringToObject(item1, "axis_type", strline);
		} else if(!strncmp(strline, "EXTERNALAXIS1_DIRECT = ", 23)) {
			strrpc(strline, "EXTERNALAXIS1_DIRECT = ", "");
			cJSON_AddStringToObject(item1, "axis_direction", strline);
		} else if(!strncmp(strline, "EXTERNALAXIS1_VEL = ", 20)) {
			strrpc(strline, "EXTERNALAXIS1_VEL = ", "");
			cJSON_AddStringToObject(item1, "axis_speed", strline);
		} else if(!strncmp(strline, "EXTERNALAXIS1_ACC = ", 20)) {
			strrpc(strline, "EXTERNALAXIS1_ACC = ", "");
			cJSON_AddStringToObject(item1, "axis_acc", strline);
		} else if(!strncmp(strline, "EXTERNALAXIS1_MINPOS = ", 23)) {
			strrpc(strline, "EXTERNALAXIS1_MINPOS = ", "");
			cJSON_AddStringToObject(item1, "axis_negsoftlimit", strline);
		} else if(!strncmp(strline, "EXTERNALAXIS1_MAXPOS = ", 23)) {
			strrpc(strline, "EXTERNALAXIS1_MAXPOS = ", "");
			cJSON_AddStringToObject(item1, "axis_possoftlimit", strline);
		} else if(!strncmp(strline, "EXTERNALAXIS1_RESOLUTION = ", 27)) {
			strrpc(strline, "EXTERNALAXIS1_RESOLUTION = ", "");
			cJSON_AddStringToObject(item1, "axis_enres", strline);
		} else if(!strncmp(strline, "EXTERNALAXIS1_LEAD = ", 21)) {
			strrpc(strline, "EXTERNALAXIS1_LEAD = ", "");
			cJSON_AddStringToObject(item1, "axis_lead", strline);
		} else if(!strncmp(strline, "EXTERNALAXIS1_OFFSET = ", 23)) {
			strrpc(strline, "EXTERNALAXIS1_OFFSET = ", "");
			cJSON_AddStringToObject(item1, "axis_offset", strline);
		} else if(!strncmp(strline, "EXTERNALAXIS1_COMPANY = ", 24)) {
			strrpc(strline, "EXTERNALAXIS1_COMPANY = ", "");
			cJSON_AddStringToObject(item1, "axis_company", strline);
		} else if(!strncmp(strline, "EXTERNALAXIS1_MODEL = ", 22)) {
			strrpc(strline, "EXTERNALAXIS1_MODEL = ", "");
			cJSON_AddStringToObject(item1, "axis_model", strline);
		} else if(!strncmp(strline, "EXTERNALAXIS1_ENCTYPE = ", 24)) {
			strrpc(strline, "EXTERNALAXIS1_ENCTYPE = ", "");
			cJSON_AddStringToObject(item1, "axis_enctype", strline);
		}else if(!strncmp(strline, "EXTERNALAXIS2_TYPE = ", 21)) {
			strrpc(strline, "EXTERNALAXIS2_TYPE = ", "");
			cJSON_AddStringToObject(item2, "axis_type", strline);
		} else if(!strncmp(strline, "EXTERNALAXIS2_DIRECT = ", 23)) {
			strrpc(strline, "EXTERNALAXIS2_DIRECT = ", "");
			cJSON_AddStringToObject(item2, "axis_direction", strline);
		} else if(!strncmp(strline, "EXTERNALAXIS2_VEL = ", 20)) {
			strrpc(strline, "EXTERNALAXIS2_VEL = ", "");
			cJSON_AddStringToObject(item2, "axis_speed", strline);
		} else if(!strncmp(strline, "EXTERNALAXIS2_ACC = ", 20)) {
			strrpc(strline, "EXTERNALAXIS2_ACC = ", "");
			cJSON_AddStringToObject(item2, "axis_acc", strline);
		} else if(!strncmp(strline, "EXTERNALAXIS2_MINPOS = ", 23)) {
			strrpc(strline, "EXTERNALAXIS2_MINPOS = ", "");
			cJSON_AddStringToObject(item2, "axis_negsoftlimit", strline);
		} else if(!strncmp(strline, "EXTERNALAXIS2_MAXPOS = ", 23)) {
			strrpc(strline, "EXTERNALAXIS2_MAXPOS = ", "");
			cJSON_AddStringToObject(item2, "axis_possoftlimit", strline);
		} else if(!strncmp(strline, "EXTERNALAXIS2_RESOLUTION = ", 27)) {
			strrpc(strline, "EXTERNALAXIS2_RESOLUTION = ", "");
			cJSON_AddStringToObject(item2, "axis_enres", strline);
		} else if(!strncmp(strline, "EXTERNALAXIS2_LEAD = ", 21)) {
			strrpc(strline, "EXTERNALAXIS2_LEAD = ", "");
			cJSON_AddStringToObject(item2, "axis_lead", strline);
		} else if(!strncmp(strline, "EXTERNALAXIS2_OFFSET = ", 23)) {
			strrpc(strline, "EXTERNALAXIS2_OFFSET = ", "");
			cJSON_AddStringToObject(item2, "axis_offset", strline);
		} else if(!strncmp(strline, "EXTERNALAXIS2_COMPANY = ", 24)) {
			strrpc(strline, "EXTERNALAXIS2_COMPANY = ", "");
			cJSON_AddStringToObject(item2, "axis_company", strline);
		} else if(!strncmp(strline, "EXTERNALAXIS2_MODEL = ", 22)) {
			strrpc(strline, "EXTERNALAXIS2_MODEL = ", "");
			cJSON_AddStringToObject(item2, "axis_model", strline);
		} else if(!strncmp(strline, "EXTERNALAXIS2_ENCTYPE = ", 24)) {
			strrpc(strline, "EXTERNALAXIS2_ENCTYPE = ", "");
			cJSON_AddStringToObject(item2, "axis_enctype", strline);
		}else if(!strncmp(strline, "EXTERNALAXIS3_TYPE = ", 21)) {
			strrpc(strline, "EXTERNALAXIS3_TYPE = ", "");
			cJSON_AddStringToObject(item3, "axis_type", strline);
		} else if(!strncmp(strline, "EXTERNALAXIS3_DIRECT = ", 23)) {
			strrpc(strline, "EXTERNALAXIS3_DIRECT = ", "");
			cJSON_AddStringToObject(item3, "axis_direction", strline);
		} else if(!strncmp(strline, "EXTERNALAXIS3_VEL = ", 20)) {
			strrpc(strline, "EXTERNALAXIS3_VEL = ", "");
			cJSON_AddStringToObject(item3, "axis_speed", strline);
		} else if(!strncmp(strline, "EXTERNALAXIS3_ACC = ", 20)) {
			strrpc(strline, "EXTERNALAXIS3_ACC = ", "");
			cJSON_AddStringToObject(item3, "axis_acc", strline);
		} else if(!strncmp(strline, "EXTERNALAXIS3_MINPOS = ", 23)) {
			strrpc(strline, "EXTERNALAXIS3_MINPOS = ", "");
			cJSON_AddStringToObject(item3, "axis_negsoftlimit", strline);
		} else if(!strncmp(strline, "EXTERNALAXIS3_MAXPOS = ", 23)) {
			strrpc(strline, "EXTERNALAXIS3_MAXPOS = ", "");
			cJSON_AddStringToObject(item3, "axis_possoftlimit", strline);
		} else if(!strncmp(strline, "EXTERNALAXIS3_RESOLUTION = ", 27)) {
			strrpc(strline, "EXTERNALAXIS3_RESOLUTION = ", "");
			cJSON_AddStringToObject(item3, "axis_enres", strline);
		} else if(!strncmp(strline, "EXTERNALAXIS3_LEAD = ", 21)) {
			strrpc(strline, "EXTERNALAXIS3_LEAD = ", "");
			cJSON_AddStringToObject(item3, "axis_lead", strline);
		} else if(!strncmp(strline, "EXTERNALAXIS3_OFFSET = ", 23)) {
			strrpc(strline, "EXTERNALAXIS3_OFFSET = ", "");
			cJSON_AddStringToObject(item3, "axis_offset", strline);
		} else if(!strncmp(strline, "EXTERNALAXIS3_COMPANY = ", 24)) {
			strrpc(strline, "EXTERNALAXIS3_COMPANY = ", "");
			cJSON_AddStringToObject(item3, "axis_company", strline);
		} else if(!strncmp(strline, "EXTERNALAXIS3_MODEL = ", 22)) {
			strrpc(strline, "EXTERNALAXIS3_MODEL = ", "");
			cJSON_AddStringToObject(item3, "axis_model", strline);
		} else if(!strncmp(strline, "EXTERNALAXIS3_ENCTYPE = ", 24)) {
			strrpc(strline, "EXTERNALAXIS3_ENCTYPE = ", "");
			cJSON_AddStringToObject(item3, "axis_enctype", strline);
		}else if(!strncmp(strline, "EXTERNALAXIS4_TYPE = ", 21)) {
			strrpc(strline, "EXTERNALAXIS4_TYPE = ", "");
			cJSON_AddStringToObject(item4, "axis_type", strline);
		} else if(!strncmp(strline, "EXTERNALAXIS4_DIRECT = ", 23)) {
			strrpc(strline, "EXTERNALAXIS4_DIRECT = ", "");
			cJSON_AddStringToObject(item4, "axis_direction", strline);
		} else if(!strncmp(strline, "EXTERNALAXIS4_VEL = ", 20)) {
			strrpc(strline, "EXTERNALAXIS4_VEL = ", "");
			cJSON_AddStringToObject(item4, "axis_speed", strline);
		} else if(!strncmp(strline, "EXTERNALAXIS4_ACC = ", 20)) {
			strrpc(strline, "EXTERNALAXIS4_ACC = ", "");
			cJSON_AddStringToObject(item4, "axis_acc", strline);
		} else if(!strncmp(strline, "EXTERNALAXIS4_MINPOS = ", 23)) {
			strrpc(strline, "EXTERNALAXIS4_MINPOS = ", "");
			cJSON_AddStringToObject(item4, "axis_negsoftlimit", strline);
		} else if(!strncmp(strline, "EXTERNALAXIS4_MAXPOS = ", 23)) {
			strrpc(strline, "EXTERNALAXIS4_MAXPOS = ", "");
			cJSON_AddStringToObject(item4, "axis_possoftlimit", strline);
		} else if(!strncmp(strline, "EXTERNALAXIS4_RESOLUTION = ", 27)) {
			strrpc(strline, "EXTERNALAXIS4_RESOLUTION = ", "");
			cJSON_AddStringToObject(item4, "axis_enres", strline);
		} else if(!strncmp(strline, "EXTERNALAXIS4_LEAD = ", 21)) {
			strrpc(strline, "EXTERNALAXIS4_LEAD = ", "");
			cJSON_AddStringToObject(item4, "axis_lead", strline);
		} else if(!strncmp(strline, "EXTERNALAXIS4_OFFSET = ", 23)) {
			strrpc(strline, "EXTERNALAXIS4_OFFSET = ", "");
			cJSON_AddStringToObject(item4, "axis_offset", strline);
		} else if(!strncmp(strline, "EXTERNALAXIS4_COMPANY = ", 24)) {
			strrpc(strline, "EXTERNALAXIS4_COMPANY = ", "");
			cJSON_AddStringToObject(item4, "axis_company", strline);
		} else if(!strncmp(strline, "EXTERNALAXIS4_MODEL = ", 22)) {
			strrpc(strline, "EXTERNALAXIS4_MODEL = ", "");
			cJSON_AddStringToObject(item4, "axis_model", strline);
		} else if(!strncmp(strline, "EXTERNALAXIS4_ENCTYPE = ", 24)) {
			strrpc(strline, "EXTERNALAXIS4_ENCTYPE = ", "");
			cJSON_AddStringToObject(item4, "axis_enctype", strline);
		} else if(!strncmp(strline, "EXTERNALAXIS_CMDDONETIME = ", 27)) {
			strrpc(strline, "EXTERNALAXIS_CMDDONETIME = ", "");
			cJSON_AddStringToObject(root_json, "cmddonetime", strline);
		}
		bzero(strline, sizeof(char)*LINE_LEN);
	}
	fclose(fp);

	*ret_f_content = cJSON_Print(root_json);
	cJSON_Delete(root_json);
	root_json = NULL;
	if (*ret_f_content == NULL) {
		perror("cJSON_Print");

		return FAIL;
	}
	printf("get_exaxis_cfg = %s\n", (*ret_f_content));

	return SUCCESS;
}

static int get_weave(char **ret_f_content)
{
	char strline[LINE_LEN] = {0};
	FILE *fp;
	cJSON *root_json = NULL;
	cJSON *item0 = NULL;
	cJSON *item1 = NULL;
	cJSON *item2 = NULL;
	cJSON *item3 = NULL;
	cJSON *item4 = NULL;
	cJSON *item5 = NULL;
	cJSON *item6 = NULL;
	cJSON *item7 = NULL;

	if ((fp = fopen(USER_CFG, "r")) == NULL) {
		perror("open file");

		return FAIL;
	}
	root_json = cJSON_CreateArray();
	item0 = cJSON_CreateObject();
	item1 = cJSON_CreateObject();
	item2 = cJSON_CreateObject();
	item3 = cJSON_CreateObject();
	item4 = cJSON_CreateObject();
	item5 = cJSON_CreateObject();
	item6 = cJSON_CreateObject();
	item7 = cJSON_CreateObject();
	cJSON_AddItemToArray(root_json, item0);
	cJSON_AddItemToArray(root_json, item1);
	cJSON_AddItemToArray(root_json, item2);
	cJSON_AddItemToArray(root_json, item3);
	cJSON_AddItemToArray(root_json, item4);
	cJSON_AddItemToArray(root_json, item5);
	cJSON_AddItemToArray(root_json, item6);
	cJSON_AddItemToArray(root_json, item7);
	cJSON_AddStringToObject(item0, "id", "0");
	cJSON_AddStringToObject(item1, "id", "1");
	cJSON_AddStringToObject(item2, "id", "2");
	cJSON_AddStringToObject(item3, "id", "3");
	cJSON_AddStringToObject(item4, "id", "4");
	cJSON_AddStringToObject(item5, "id", "5");
	cJSON_AddStringToObject(item6, "id", "6");
	cJSON_AddStringToObject(item7, "id", "7");

	while (fgets(strline, LINE_LEN, fp) != NULL) {
		strrpc(strline, "\n", "");
		if(!strncmp(strline, "WEAVE0_TYPE = ", 14)) {
			strrpc(strline, "WEAVE0_TYPE = ", "");
			cJSON_AddStringToObject(item0, "type", strline);
		} else if(!strncmp(strline, "WEAVE0_FREQ = ", 14)) {
			strrpc(strline, "WEAVE0_FREQ = ", "");
			cJSON_AddStringToObject(item0, "freq", strline);
		} else if(!strncmp(strline, "WEAVE0_RANGE = ", 15)) {
			strrpc(strline, "WEAVE0_RANGE = ", "");
			cJSON_AddStringToObject(item0, "range", strline);
		} else if(!strncmp(strline, "WEAVE0_LTIME = ", 15)) {
			strrpc(strline, "WEAVE0_LTIME = ", "");
			cJSON_AddStringToObject(item0, "ltime", strline);
		} else if(!strncmp(strline, "WEAVE0_RTIME = ", 15)) {
			strrpc(strline, "WEAVE0_RTIME = ", "");
			cJSON_AddStringToObject(item0, "rtime", strline);
		} else if(!strncmp(strline, "WEAVE0_CIRCLERATIO = ", 21)) {
			strrpc(strline, "WEAVE0_CIRCLERATIO = ", "");
			cJSON_AddStringToObject(item0, "circleratio", strline);
		} else if(!strncmp(strline, "WEAVE1_TYPE = ", 14)) {
			strrpc(strline, "WEAVE1_TYPE = ", "");
			cJSON_AddStringToObject(item1, "type", strline);
		} else if(!strncmp(strline, "WEAVE1_FREQ = ", 14)) {
			strrpc(strline, "WEAVE1_FREQ = ", "");
			cJSON_AddStringToObject(item1, "freq", strline);
		} else if(!strncmp(strline, "WEAVE1_RANGE = ", 15)) {
			strrpc(strline, "WEAVE1_RANGE = ", "");
			cJSON_AddStringToObject(item1, "range", strline);
		} else if(!strncmp(strline, "WEAVE1_LTIME = ", 15)) {
			strrpc(strline, "WEAVE1_LTIME = ", "");
			cJSON_AddStringToObject(item1, "ltime", strline);
		} else if(!strncmp(strline, "WEAVE1_RTIME = ", 15)) {
			strrpc(strline, "WEAVE1_RTIME = ", "");
			cJSON_AddStringToObject(item1, "rtime", strline);
		} else if(!strncmp(strline, "WEAVE1_CIRCLERATIO = ", 21)) {
			strrpc(strline, "WEAVE1_CIRCLERATIO = ", "");
			cJSON_AddStringToObject(item1, "circleratio", strline);
		} else if(!strncmp(strline, "WEAVE2_TYPE = ", 14)) {
			strrpc(strline, "WEAVE2_TYPE = ", "");
			cJSON_AddStringToObject(item2, "type", strline);
		} else if(!strncmp(strline, "WEAVE2_FREQ = ", 14)) {
			strrpc(strline, "WEAVE2_FREQ = ", "");
			cJSON_AddStringToObject(item2, "freq", strline);
		} else if(!strncmp(strline, "WEAVE2_RANGE = ", 15)) {
			strrpc(strline, "WEAVE2_RANGE = ", "");
			cJSON_AddStringToObject(item2, "range", strline);
		} else if(!strncmp(strline, "WEAVE2_LTIME = ", 15)) {
			strrpc(strline, "WEAVE2_LTIME = ", "");
			cJSON_AddStringToObject(item2, "ltime", strline);
		} else if(!strncmp(strline, "WEAVE2_RTIME = ", 15)) {
			strrpc(strline, "WEAVE2_RTIME = ", "");
			cJSON_AddStringToObject(item2, "rtime", strline);
		} else if(!strncmp(strline, "WEAVE2_CIRCLERATIO = ", 21)) {
			strrpc(strline, "WEAVE2_CIRCLERATIO = ", "");
			cJSON_AddStringToObject(item2, "circleratio", strline);
		} else if(!strncmp(strline, "WEAVE3_TYPE = ", 14)) {
			strrpc(strline, "WEAVE3_TYPE = ", "");
			cJSON_AddStringToObject(item3, "type", strline);
		} else if(!strncmp(strline, "WEAVE3_FREQ = ", 14)) {
			strrpc(strline, "WEAVE3_FREQ = ", "");
			cJSON_AddStringToObject(item3, "freq", strline);
		} else if(!strncmp(strline, "WEAVE3_RANGE = ", 15)) {
			strrpc(strline, "WEAVE3_RANGE = ", "");
			cJSON_AddStringToObject(item3, "range", strline);
		} else if(!strncmp(strline, "WEAVE3_LTIME = ", 15)) {
			strrpc(strline, "WEAVE3_LTIME = ", "");
			cJSON_AddStringToObject(item3, "ltime", strline);
		} else if(!strncmp(strline, "WEAVE3_RTIME = ", 15)) {
			strrpc(strline, "WEAVE3_RTIME = ", "");
			cJSON_AddStringToObject(item3, "rtime", strline);
		} else if(!strncmp(strline, "WEAVE3_CIRCLERATIO = ", 21)) {
			strrpc(strline, "WEAVE3_CIRCLERATIO = ", "");
			cJSON_AddStringToObject(item3, "circleratio", strline);
		} else if(!strncmp(strline, "WEAVE4_TYPE = ", 14)) {
			strrpc(strline, "WEAVE4_TYPE = ", "");
			cJSON_AddStringToObject(item4, "type", strline);
		} else if(!strncmp(strline, "WEAVE4_FREQ = ", 14)) {
			strrpc(strline, "WEAVE4_FREQ = ", "");
			cJSON_AddStringToObject(item4, "freq", strline);
		} else if(!strncmp(strline, "WEAVE4_RANGE = ", 15)) {
			strrpc(strline, "WEAVE4_RANGE = ", "");
			cJSON_AddStringToObject(item4, "range", strline);
		} else if(!strncmp(strline, "WEAVE4_LTIME = ", 15)) {
			strrpc(strline, "WEAVE4_LTIME = ", "");
			cJSON_AddStringToObject(item4, "ltime", strline);
		} else if(!strncmp(strline, "WEAVE4_RTIME = ", 15)) {
			strrpc(strline, "WEAVE4_RTIME = ", "");
			cJSON_AddStringToObject(item4, "rtime", strline);
		} else if(!strncmp(strline, "WEAVE4_CIRCLERATIO = ", 21)) {
			strrpc(strline, "WEAVE4_CIRCLERATIO = ", "");
			cJSON_AddStringToObject(item4, "circleratio", strline);
		} else if(!strncmp(strline, "WEAVE5_TYPE = ", 14)) {
			strrpc(strline, "WEAVE5_TYPE = ", "");
			cJSON_AddStringToObject(item5, "type", strline);
		} else if(!strncmp(strline, "WEAVE5_FREQ = ", 14)) {
			strrpc(strline, "WEAVE5_FREQ = ", "");
			cJSON_AddStringToObject(item5, "freq", strline);
		} else if(!strncmp(strline, "WEAVE5_RANGE = ", 15)) {
			strrpc(strline, "WEAVE5_RANGE = ", "");
			cJSON_AddStringToObject(item5, "range", strline);
		} else if(!strncmp(strline, "WEAVE5_LTIME = ", 15)) {
			strrpc(strline, "WEAVE5_LTIME = ", "");
			cJSON_AddStringToObject(item5, "ltime", strline);
		} else if(!strncmp(strline, "WEAVE5_RTIME = ", 15)) {
			strrpc(strline, "WEAVE5_RTIME = ", "");
			cJSON_AddStringToObject(item5, "rtime", strline);
		} else if(!strncmp(strline, "WEAVE5_CIRCLERATIO = ", 21)) {
			strrpc(strline, "WEAVE5_CIRCLERATIO = ", "");
			cJSON_AddStringToObject(item5, "circleratio", strline);
		} else if(!strncmp(strline, "WEAVE6_TYPE = ", 14)) {
			strrpc(strline, "WEAVE6_TYPE = ", "");
			cJSON_AddStringToObject(item6, "type", strline);
		} else if(!strncmp(strline, "WEAVE6_FREQ = ", 14)) {
			strrpc(strline, "WEAVE6_FREQ = ", "");
			cJSON_AddStringToObject(item6, "freq", strline);
		} else if(!strncmp(strline, "WEAVE6_RANGE = ", 15)) {
			strrpc(strline, "WEAVE6_RANGE = ", "");
			cJSON_AddStringToObject(item6, "range", strline);
		} else if(!strncmp(strline, "WEAVE6_LTIME = ", 15)) {
			strrpc(strline, "WEAVE6_LTIME = ", "");
			cJSON_AddStringToObject(item6, "ltime", strline);
		} else if(!strncmp(strline, "WEAVE6_RTIME = ", 15)) {
			strrpc(strline, "WEAVE6_RTIME = ", "");
			cJSON_AddStringToObject(item6, "rtime", strline);
		} else if(!strncmp(strline, "WEAVE6_CIRCLERATIO = ", 21)) {
			strrpc(strline, "WEAVE6_CIRCLERATIO = ", "");
			cJSON_AddStringToObject(item6, "circleratio", strline);
		} else if(!strncmp(strline, "WEAVE7_TYPE = ", 14)) {
			strrpc(strline, "WEAVE7_TYPE = ", "");
			cJSON_AddStringToObject(item7, "type", strline);
		} else if(!strncmp(strline, "WEAVE7_FREQ = ", 14)) {
			strrpc(strline, "WEAVE7_FREQ = ", "");
			cJSON_AddStringToObject(item7, "freq", strline);
		} else if(!strncmp(strline, "WEAVE7_RANGE = ", 15)) {
			strrpc(strline, "WEAVE7_RANGE = ", "");
			cJSON_AddStringToObject(item7, "range", strline);
		} else if(!strncmp(strline, "WEAVE7_LTIME = ", 15)) {
			strrpc(strline, "WEAVE7_LTIME = ", "");
			cJSON_AddStringToObject(item7, "ltime", strline);
		} else if(!strncmp(strline, "WEAVE7_RTIME = ", 15)) {
			strrpc(strline, "WEAVE7_RTIME = ", "");
			cJSON_AddStringToObject(item7, "rtime", strline);
		} else if(!strncmp(strline, "WEAVE7_CIRCLERATIO = ", 21)) {
			strrpc(strline, "WEAVE7_CIRCLERATIO = ", "");
			cJSON_AddStringToObject(item7, "circleratio", strline);
		}
		bzero(strline, sizeof(char)*LINE_LEN);
	}
	fclose(fp);

	*ret_f_content = cJSON_Print(root_json);
	cJSON_Delete(root_json);
	root_json = NULL;
	if (*ret_f_content == NULL) {
		perror("cJSON_Print");

		return FAIL;
	}
	//printf("*ret_f_content = %s\n", (*ret_f_content));

	return SUCCESS;
}

/* get plugin info */
static int get_plugin_info(char **ret_f_content)
{
	int i = 0;
	char *dir_list = NULL;
	char package_path[100] = {0};
	char *package_content = NULL;
	cJSON *dir_list_json = NULL;
	cJSON *dir_plugin_name_json = NULL;
	cJSON *root_json = NULL;
	cJSON *package_json = NULL;

	dir_list = get_dir_filename(UPLOAD_WEB_PLUGINS);
	if (dir_list == NULL) {
		perror("get dir filename");

		return FAIL;
	}
	//printf("dir_list = %s\n", dir_list);

	dir_list_json = cJSON_Parse(dir_list);
	free(dir_list);
	dir_list = NULL;
	if (dir_list_json == NULL) {
		perror("cJSON_Parse");

		return FAIL;
	}

	root_json = cJSON_CreateArray();
	for (i = 0; i < cJSON_GetArraySize(dir_list_json); i++) {
		bzero(package_path, sizeof(package_path));
		dir_plugin_name_json = cJSON_GetArrayItem(dir_list_json, i);
		sprintf(package_path, "%s%s/package.json", UPLOAD_WEB_PLUGINS, dir_plugin_name_json->valuestring);
		//printf("package_path = %s\n", package_path);

		package_content = get_file_content(package_path);
		if (package_content == NULL || strcmp(package_content, "NO_FILE") == 0 || strcmp(package_content, "Empty") == 0) {
			perror("get file content");
			cJSON_Delete(dir_list_json);
			dir_list_json = NULL;
			cJSON_Delete(root_json);
			root_json = NULL;

			return FAIL;
		}
		//printf("package_content = %s\n", package_content);
		package_json = cJSON_Parse(package_content);
		free(package_content);
		package_content = NULL;
		cJSON_AddItemToArray(root_json, package_json);
	}
	cJSON_Delete(dir_list_json);
	dir_list_json = NULL;

	*ret_f_content = cJSON_Print(root_json);
	cJSON_Delete(root_json);
	root_json = NULL;
	/* content is NULL */
	if (*ret_f_content == NULL) {
		perror("cJSON_Print");

		return FAIL;
	}
	//printf("*ret_f_content = %s\n", *ret_f_content);

	return SUCCESS;
}

/* get plugin nav */
static int get_plugin_nav(char **ret_f_content)
{
	DIR *dir = NULL;
	struct dirent *ptr = NULL;
	int i = 0;
	char dir_plugin[100] = {0};
	char html_path[100] = {0};
	char package_path[100] = {0};
	char *dir_list = NULL;
	char *package_content = NULL;
	cJSON *dir_list_json = NULL;
	cJSON *dir_plugin_name_json = NULL;
	cJSON *root_json = NULL;
	cJSON *package_json = NULL;
	cJSON *newitem = NULL;
	cJSON *nav_name = NULL;
	cJSON *enable = NULL;

	dir_list = get_dir_filename(UPLOAD_WEB_PLUGINS);
	if (dir_list == NULL) {
		perror("get dir filename");

		return FAIL;
	}
	//printf("dir_list = %s\n", dir_list);

	dir_list_json = cJSON_Parse(dir_list);
	free(dir_list);
	dir_list = NULL;
	if (dir_list_json == NULL) {
		perror("cJSON_Parse");

		return FAIL;
	}

	root_json = cJSON_CreateArray();
	for (i = 0; i < cJSON_GetArraySize(dir_list_json); i++) {
		bzero(dir_plugin, sizeof(dir_plugin));
		bzero(html_path, sizeof(html_path));
		bzero(package_path, sizeof(package_path));
		dir_plugin_name_json = cJSON_GetArrayItem(dir_list_json, i);

		sprintf(dir_plugin, "%s%s/", UPLOAD_WEB_PLUGINS, dir_plugin_name_json->valuestring);
		//printf("dir_plugin = %s\n", dir_plugin);
		dir = opendir(dir_plugin);
		if (dir == NULL) {
			perror("opendir");

			goto end;
		}
		while((ptr = readdir(dir)) != NULL) {
			if(is_in(ptr->d_name, ".html") == 1) {
				sprintf(html_path, "./plugins/web-plugins/%s/%s", dir_plugin_name_json->valuestring, ptr->d_name);
				break;
			}
		}
		closedir(dir);
		dir = NULL;

		sprintf(package_path, "%s%s/package.json", UPLOAD_WEB_PLUGINS, dir_plugin_name_json->valuestring);
		package_content = get_file_content(package_path);
		if (package_content == NULL || strcmp(package_content, "NO_FILE") == 0 || strcmp(package_content, "Empty") == 0) {
			perror("get file content");

			goto end;
		}
		//printf("package_content = %s\n", package_content);
		package_json = cJSON_Parse(package_content);
		free(package_content);
		package_content = NULL;
		if (package_json == NULL) {
			perror("cJSON_Parse");

			goto end;
		}
		nav_name = cJSON_GetObjectItem(package_json, "nav_name");
		if (nav_name == NULL || nav_name->valuestring == NULL) {
			perror("JSON");

			goto end;
		}
		enable = cJSON_GetObjectItem(package_json, "enable");
		if (enable == NULL) {
			perror("JSON");

			goto end;
		}
		if (enable->valueint == 1) {
			newitem = cJSON_CreateObject();
			cJSON_AddStringToObject(newitem, "url", html_path);
			cJSON_AddStringToObject(newitem, "nav_name", nav_name->valuestring);
			cJSON_AddItemToArray(root_json, newitem);
		}
	}
	cJSON_Delete(dir_list_json);
	dir_list_json = NULL;

	*ret_f_content = cJSON_Print(root_json);
	cJSON_Delete(root_json);
	root_json = NULL;
	/* content is NULL */
	if (*ret_f_content == NULL) {
		perror("cJSON_Print");

		return FAIL;
	}
	//printf("*ret_f_content = %s\n", *ret_f_content);

	return SUCCESS;

end:
	cJSON_Delete(dir_list_json);
	dir_list_json = NULL;
	cJSON_Delete(root_json);
	root_json = NULL;
	return FAIL;
}

/* get plugin config */
static int get_plugin_config(char **ret_f_content, const cJSON *data_json)
{
	cJSON *name = NULL;
	cJSON *config_json = NULL;
	cJSON *di_json = NULL;
	cJSON *do_json = NULL;
	cJSON *root_json = NULL;
	char config_path[100] = {0};
	char *config_content = NULL;
	char strline[LINE_LEN] = {0};
	FILE *fp;

	name = cJSON_GetObjectItem(data_json, "name");
	if (name == NULL || name->valuestring == NULL) {
		perror("json");

		return FAIL;
	}
	sprintf(config_path, "%s%s/config.json", UPLOAD_WEB_PLUGINS, name->valuestring);
	config_content = get_file_content(config_path);
	if (config_content == NULL || strcmp(config_content, "NO_FILE") == 0 || strcmp(config_content, "Empty") == 0) {
		perror("get file content");

		return FAIL;
	}
	//printf("config_content = %s\n", config_content);
	config_json = cJSON_Parse(config_content);
	free(config_content);
	config_content = NULL;
	if (config_json == NULL) {
		perror("cJSON_Parse");

		return FAIL;
	}
	if ((fp = fopen(USER_CFG, "r")) == NULL) {
		perror("open file");

		return FAIL;
	}
	root_json = cJSON_CreateObject();
	di_json = cJSON_CreateArray();
	do_json = cJSON_CreateArray();
	cJSON_AddItemToObject(root_json, "config", config_json);
	cJSON_AddItemToObject(root_json, "di", di_json);
	cJSON_AddItemToObject(root_json, "do", do_json);
	while (fgets(strline, LINE_LEN, fp) != NULL) {
		strrpc(strline, "\n", "");
		if (!strncmp(strline, "CTL_DI8_CONFIG = ", 17)) {
			strrpc(strline, "CTL_DI8_CONFIG = ", "");
			if (atoi(strline) == 0) {
				cJSON_AddItemToArray(di_json, cJSON_CreateNumber(8));
			}
			bzero(strline, sizeof(char)*LINE_LEN);
			continue;
		} else if (!strncmp(strline, "CTL_DI9_CONFIG = ", 17)) {
			strrpc(strline, "CTL_DI9_CONFIG = ", "");
			if (atoi(strline) == 0) {
				cJSON_AddItemToArray(di_json, cJSON_CreateNumber(9));
			}
			bzero(strline, sizeof(char)*LINE_LEN);
			continue;
		} else if (!strncmp(strline, "CTL_DI10_CONFIG = ", 18)) {
			strrpc(strline, "CTL_DI10_CONFIG = ", "");
			if (atoi(strline) == 0) {
				cJSON_AddItemToArray(di_json, cJSON_CreateNumber(10));
			}
			bzero(strline, sizeof(char)*LINE_LEN);
			continue;
		} else if (!strncmp(strline, "CTL_DI11_CONFIG = ", 18)) {
			strrpc(strline, "CTL_DI11_CONFIG = ", "");
			if (atoi(strline) == 0) {
				cJSON_AddItemToArray(di_json, cJSON_CreateNumber(11));
			}
			bzero(strline, sizeof(char)*LINE_LEN);
			continue;
		} else if (!strncmp(strline, "CTL_DI12_CONFIG = ", 18)) {
			strrpc(strline, "CTL_DI12_CONFIG = ", "");
			if (atoi(strline) == 0) {
				cJSON_AddItemToArray(di_json, cJSON_CreateNumber(12));
			}
			bzero(strline, sizeof(char)*LINE_LEN);
			continue;
		} else if (!strncmp(strline, "CTL_DI13_CONFIG = ", 18)) {
			strrpc(strline, "CTL_DI13_CONFIG = ", "");
			if (atoi(strline) == 0) {
				cJSON_AddItemToArray(di_json, cJSON_CreateNumber(13));
			}
			bzero(strline, sizeof(char)*LINE_LEN);
			continue;
		} else if (!strncmp(strline, "CTL_DI14_CONFIG = ", 18)) {
			strrpc(strline, "CTL_DI14_CONFIG = ", "");
			if (atoi(strline) == 0) {
				cJSON_AddItemToArray(di_json, cJSON_CreateNumber(14));
			}
			bzero(strline, sizeof(char)*LINE_LEN);
			continue;
		} else if (!strncmp(strline, "CTL_DI15_CONFIG = ", 18)) {
			strrpc(strline, "CTL_DI15_CONFIG = ", "");
			if (atoi(strline) == 0) {
				cJSON_AddItemToArray(di_json, cJSON_CreateNumber(15));
			}
			bzero(strline, sizeof(char)*LINE_LEN);
			continue;
		} else if (!strncmp(strline, "CTL_DO8_CONFIG = ", 17)) {
			strrpc(strline, "CTL_DO8_CONFIG = ", "");
			if (atoi(strline) == 0) {
				cJSON_AddItemToArray(do_json, cJSON_CreateNumber(8));
			}
			bzero(strline, sizeof(char)*LINE_LEN);
			continue;
		} else if (!strncmp(strline, "CTL_DO9_CONFIG = ", 17)) {
			strrpc(strline, "CTL_DO9_CONFIG = ", "");
			if (atoi(strline) == 0) {
				cJSON_AddItemToArray(do_json, cJSON_CreateNumber(9));
			}
			bzero(strline, sizeof(char)*LINE_LEN);
			continue;
		} else if (!strncmp(strline, "CTL_DO10_CONFIG = ", 18)) {
			strrpc(strline, "CTL_DO10_CONFIG = ", "");
			if (atoi(strline) == 0) {
				cJSON_AddItemToArray(do_json, cJSON_CreateNumber(10));
			}
			bzero(strline, sizeof(char)*LINE_LEN);
			continue;
		} else if (!strncmp(strline, "CTL_DO11_CONFIG = ", 18)) {
			strrpc(strline, "CTL_DO11_CONFIG = ", "");
			if (atoi(strline) == 0) {
				cJSON_AddItemToArray(do_json, cJSON_CreateNumber(11));
			}
			bzero(strline, sizeof(char)*LINE_LEN);
			continue;
		} else if (!strncmp(strline, "CTL_DO12_CONFIG = ", 18)) {
			strrpc(strline, "CTL_DO12_CONFIG = ", "");
			if (atoi(strline) == 0) {
				cJSON_AddItemToArray(do_json, cJSON_CreateNumber(12));
			}
			bzero(strline, sizeof(char)*LINE_LEN);
			continue;
		} else if (!strncmp(strline, "CTL_DO13_CONFIG = ", 18)) {
			strrpc(strline, "CTL_DO13_CONFIG = ", "");
			if (atoi(strline) == 0) {
				cJSON_AddItemToArray(do_json, cJSON_CreateNumber(13));
			}
			bzero(strline, sizeof(char)*LINE_LEN);
			continue;
		} else if (!strncmp(strline, "CTL_DO14_CONFIG = ", 18)) {
			strrpc(strline, "CTL_DO14_CONFIG = ", "");
			if (atoi(strline) == 0) {
				cJSON_AddItemToArray(do_json, cJSON_CreateNumber(14));
			}
			bzero(strline, sizeof(char)*LINE_LEN);
			continue;
		} else if (!strncmp(strline, "CTL_DO15_CONFIG = ", 18)) {
			strrpc(strline, "CTL_DO15_CONFIG = ", "");
			if (atoi(strline) == 0) {
				cJSON_AddItemToArray(do_json, cJSON_CreateNumber(15));
			}
			bzero(strline, sizeof(char)*LINE_LEN);
			continue;
		}
	}
	fclose(fp);

	*ret_f_content = cJSON_Print(root_json);
	cJSON_Delete(root_json);
	root_json = NULL;
	if (*ret_f_content == NULL) {
		perror("cJSON_Print");

		return FAIL;
	}
	//printf("*ret_f_content = %s\n", (*ret_f_content));

	return SUCCESS;
}

/* get DH file */
static int get_DH_file(char **ret_f_content)
{
	cJSON *root_json = NULL;
	char strline[LINE_LEN] = {0};
	FILE *fp = NULL;
	char **array = NULL;
	int size = 0;

	root_json = cJSON_CreateArray();
	if ((fp = fopen(FILE_DH_POINT, "r+")) != NULL) {
		while (fgets(strline, LINE_LEN, fp) != NULL) {
			if (string_to_string_list(strline, " ", &size, &array) == 0 || size != 7) {
				perror("string to string list");
				fclose(fp);
				string_list_free(&array, size);
				cJSON_Delete(root_json);
				root_json = NULL;

				return FAIL;
			}
			if (array[0] != NULL) {
				cJSON_AddItemToArray(root_json, cJSON_CreateNumber(atoi(array[0])));
			}
			string_list_free(&array, size);
			bzero(strline, LINE_LEN);
		}
		fclose(fp);
	}
	*ret_f_content = cJSON_Print(root_json);
	cJSON_Delete(root_json);
	root_json = NULL;
	if (*ret_f_content == NULL) {
		perror("cjson_Print");

		return FAIL;
	}

	return SUCCESS;
}

/* get SN */
static int get_robot_serialnumber(char **ret_f_content)
{
	char *strline = NULL;
	strline = (char *)calloc(1, sizeof(char)*LINE_LEN);
	char serialnumber[LINE_LEN] = {0};
	FILE *fp;
	cJSON *root_json = NULL;

	root_json = cJSON_CreateObject();
	if ((fp = fopen(FILE_SN, "r")) == NULL) {
		perror("open file");

		return FAIL;
	}
	while (fgets(strline, LINE_LEN, fp) != NULL) {
		if (!strncmp(strline, "SN=", 3)) {
			//printf("strlen(strline) = %d\n", strlen(strline));
			strncpy(serialnumber, (strline + 3), (strlen(strline) - 3));
			cJSON_AddStringToObject(root_json, "serialnumber", serialnumber);

			break;
		}
		bzero(strline, sizeof(char)*LINE_LEN);
	}
	free(strline);
	strline = NULL;
	fclose(fp);

	*ret_f_content = cJSON_Print(root_json);
	cJSON_Delete(root_json);
	root_json = NULL;
	if (*ret_f_content == NULL) {
		perror("cJSON_Print");

		return FAIL;
	}
	//printf("*ret_f_content = %s\n", (*ret_f_content));

	return SUCCESS;
}

/* get robot type and return to page */
static int get_robot_type(char **ret_f_content)
{
	cJSON *root_json = NULL;

	*ret_f_content = get_file_content(FILE_ROBOT_TYPE);
	/* ret_f_content is NULL */
	if (*ret_f_content == NULL) {
		perror("get file content");

		return FAIL;
	}

	/* file is not exist or empty */
	if (strcmp(*ret_f_content, "NO_FILE") == 0 || strcmp(*ret_f_content, "Empty") == 0) {
		perror("file is not exist or empty");
		root_json = cJSON_CreateObject();
		cJSON_AddNumberToObject(root_json, "type", 0);
		cJSON_AddNumberToObject(root_json, "major_ver", 0);
		cJSON_AddNumberToObject(root_json, "minor_ver", 0);
		cJSON_AddNumberToObject(root_json, "load_range_max", 0);
		*ret_f_content = cJSON_Print(root_json);
		cJSON_Delete(root_json);
		root_json = NULL;
		if (*ret_f_content == NULL) {
			perror("cJSON_Print");

			return FAIL;
		}

		return SUCCESS;
	}

	return SUCCESS;
}

/* get TSP flg and return to page */
static int get_TSP_flg(char **ret_f_content)
{
	*ret_f_content = get_file_content(FILE_TORQUE_PAGEFLAG);
	/* ret_f_content is NULL or no such file or empty */
	if (*ret_f_content == NULL || strcmp(*ret_f_content, "NO_FILE") == 0 || strcmp(*ret_f_content, "Empty") == 0) {
		*ret_f_content = NULL;
		perror("get file content");

		return FAIL;
	}

	return SUCCESS;
}

/* get torque wpname list and return to page */
static int torque_get_wpname_list(char **ret_f_content)
{
	cJSON *root_json = NULL;
	char sql[1024] = {0};
	char **resultp = NULL;
	int nrow = 0;
	int ncloumn = 0;
	int i = 0;

	root_json = cJSON_CreateArray();
	sprintf(sql, "select * from torquesys_cfg");
	select_info_sqlite3(DB_TORQUE_CFG, sql, &resultp, &nrow, &ncloumn);

	for (i = 0; i < nrow; i++) {
		if (resultp[(i + 1) * ncloumn] != NULL) {
			cJSON_AddItemToArray(root_json, cJSON_CreateString(resultp[(i + 1) * ncloumn]));
		}
	}
	*ret_f_content = cJSON_Print(root_json);
	cJSON_Delete(root_json);
	root_json = NULL;
	sqlite3_free_table(resultp); //释放结果集

	/* content is NULL */
	if (*ret_f_content == NULL) {
		perror("cJSON_Print");

		return FAIL;
	}

	return SUCCESS;
}

/* get torque cfg and return to page */
static int torque_get_cfg(char **ret_f_content, const cJSON *data_json)
{
	cJSON *root_json = NULL;
	cJSON *name = NULL;
	char sql[1024] = {0};
	char **resultp = NULL;
	int nrow = 0;
	int ncloumn = 0;
	int j = 0;

	name = cJSON_GetObjectItem(data_json, "workpiece_id");
	if (name == NULL || name->valuestring == NULL) {
		perror("json");

		return FAIL;
	}

	sprintf(sql, "select * from torquesys_cfg where workpiece_id = '%s'", name->valuestring);
	if (select_info_sqlite3(DB_TORQUE_CFG, sql, &resultp, &nrow, &ncloumn) == -1) {

		return FAIL;
	}

	root_json = cJSON_CreateObject();
	for (j = 0; j < ncloumn; j++) {
		if (resultp[ncloumn + j] != NULL) {
			if (j == 0) {
				cJSON_AddStringToObject(root_json, resultp[j], resultp[ncloumn + j]);
			} else {
				cJSON_AddNumberToObject(root_json, resultp[j], atoi(resultp[ncloumn + j]));
			}
		}
	}
	*ret_f_content = cJSON_Print(root_json);
	cJSON_Delete(root_json);
	root_json = NULL;
	sqlite3_free_table(resultp); //释放结果集

	/* content is NULL */
	if (*ret_f_content == NULL) {
		perror("cJSON_Print");

		return FAIL;
	}

	return SUCCESS;
}

/* get torque points and return to page */
static int torque_get_points(char **ret_f_content, const cJSON *data_json)
{
	cJSON *root_json = NULL;
	cJSON *left_workstation_json = NULL;
	cJSON *right_workstation_json = NULL;
	cJSON *item = NULL;
	cJSON *name = NULL;
	char sql[1024] = {0};
	char **resultp = NULL;
	int nrow = 0;
	int ncloumn = 0;
	int i = 0;

	root_json = cJSON_CreateObject();
	name = cJSON_GetObjectItem(data_json, "workpiece_id");
	if (name == NULL || name->valuestring == NULL) {
		perror("json");

		return FAIL;
	}

	memset(sql, 0, sizeof(sql));
	sprintf(sql, "select * from sqlite_master where type = 'table' and name = '%s_cfg';", name->valuestring);
	/** 如果工件已配置 */
	if (select_info_sqlite3(DB_TORQUE_POINTS, sql, &resultp, &nrow, &ncloumn) != -1) {
		sqlite3_free_table(resultp);

		/** 获取工件在左工位上的示教点 */
		memset(sql, 0, 1024);
		sprintf(sql, "select * from [%s_left];", name->valuestring);
		left_workstation_json = cJSON_CreateArray();
		if (select_info_sqlite3(DB_TORQUE_POINTS, sql, &resultp, &nrow, &ncloumn) != -1) {
			for (i = 0; i < nrow; i++) {
				if (resultp[(i + 1) * ncloumn] != NULL && resultp[(i + 1) * ncloumn + 1] != NULL) {
					item = cJSON_CreateObject();
					cJSON_AddStringToObject(item, resultp[0], resultp[(i + 1) * ncloumn]);
					cJSON_AddNumberToObject(item, resultp[1], atoi(resultp[(i + 1) * ncloumn + 1]));
					cJSON_AddItemToArray(left_workstation_json, item);
				}
			}
		}
		cJSON_AddItemToObject(root_json, "left_workstation", left_workstation_json);
		if (resultp != NULL) {
			sqlite3_free_table(resultp);
			resultp = NULL;
		}

		/** 获取工件在右工位上的示教点 */
		memset(sql, 0, 1024);
		sprintf(sql, "select * from [%s_right];", name->valuestring);
		right_workstation_json = cJSON_CreateArray();
		if (select_info_sqlite3(DB_TORQUE_POINTS, sql, &resultp, &nrow, &ncloumn) != -1) {
			for (i = 0; i < nrow; i++) {
				if (resultp[(i + 1) * ncloumn] != NULL && resultp[(i + 1) * ncloumn + 1] != NULL) {
					item = cJSON_CreateObject();
					cJSON_AddStringToObject(item, resultp[0], resultp[(i + 1) * ncloumn]);
					cJSON_AddNumberToObject(item, resultp[1], atoi(resultp[(i + 1) * ncloumn + 1]));
					cJSON_AddItemToArray(right_workstation_json, item);
				}
			}
		}
		cJSON_AddItemToObject(root_json, "right_workstation", right_workstation_json);
		if (resultp != NULL) {
			sqlite3_free_table(resultp);
			resultp = NULL;
		}

		/** 获取工件的示教点配置信息 */
		memset(sql, 0, 1024);
		sprintf(sql, "select * from [%s_cfg];", name->valuestring);
		if (select_info_sqlite3(DB_TORQUE_POINTS, sql, &resultp, &nrow, &ncloumn) != -1) {
			cJSON_AddStringToObject(root_json, resultp[0], resultp[ncloumn]);
			cJSON_AddNumberToObject(root_json, resultp[1], atoi(resultp[ncloumn + 1]));
		}
		if (resultp != NULL) {
			sqlite3_free_table(resultp);
			resultp = NULL;
		}
	} else {
		if (resultp != NULL) {
			sqlite3_free_table(resultp);
			resultp = NULL;
		}
	}

	*ret_f_content = cJSON_Print(root_json);
	cJSON_Delete(root_json);
	root_json = NULL;
	/* content is NULL */
	if (*ret_f_content == NULL) {
		perror("cJSON_Print");

		return FAIL;
	}

	return SUCCESS;
}

/* get torque DIO and return to page */
static int get_DIO_cfg(char **ret_f_content)
{
	cJSON *root_json = NULL;

	*ret_f_content = get_file_content(FILE_TORQUE_DIO);
	/* ret_f_content is NULL */
	if (*ret_f_content == NULL) {
		perror("get file content");

		return FAIL;
	}

	/* file is not exist or empty */
	if (strcmp(*ret_f_content, "NO_FILE") == 0 || strcmp(*ret_f_content, "Empty") == 0) {
		*ret_f_content = NULL;
		perror("file is not exist or empty");

		return FAIL;
	}

	return SUCCESS;
}

/* get varlist */
static int get_varlist(char **ret_f_content)
{
	char sql[1024] = {0};
	cJSON *json_data = NULL;

	sprintf(sql, "select * from sysvar");
	if (select_info_nokey_json_sqlite3(DB_SYSVAR, sql, &json_data) == -1) {
		perror("select points");

		return FAIL;
	}

	*ret_f_content = cJSON_Print(json_data);
	cJSON_Delete(json_data);
	json_data = NULL;
	/* content is NULL */
	if (*ret_f_content == NULL) {
		perror("cJSON_Print");

		return FAIL;
	}

	return SUCCESS;
}

/* check var result */
static int get_checkvar(char **ret_f_content, const cJSON *data_json)
{
	char sql[1024] = {0};
	cJSON *name = NULL;
	cJSON *root_json = NULL;
	cJSON *json_data = NULL;

	name = cJSON_GetObjectItem(data_json, "name");
	if (name == NULL || name->valuestring == NULL) {
		perror("json");

		return FAIL;
	}
	root_json = cJSON_CreateObject();
	sprintf(sql, "select * from sysvar where name = \'%s\';", name->valuestring);
	if (select_info_json_sqlite3(DB_SYSVAR, sql, &json_data) == -1) {
		printf("check sysvar result : not exist!");
		cJSON_AddStringToObject(root_json, "result", "0");
	} else {
		printf("check sysvar result : exist!");
		cJSON_AddStringToObject(root_json, "result", "1");
	}
	cJSON_Delete(json_data);
	json_data = NULL;

	*ret_f_content = cJSON_Print(root_json);
	cJSON_Delete(root_json);
	root_json = NULL;
	if (*ret_f_content == NULL) {
		perror("cJSON_Print");

		return FAIL;
	}
	//printf("*ret_f_content = %s\n", (*ret_f_content));

	return SUCCESS;
}

/* torque get wkpoints file content */
static int torque_get_wkpoints(char **ret_f_content, const cJSON *data_json)
{
	char sql[1024] = {0};
	int nrow = 0;
	int ncloumn = 0;
	int i = 0;
	char **resultp = NULL;
	cJSON *json_data = NULL;
	cJSON *left_data = NULL;
	cJSON *right_data = NULL;
	cJSON *workpiece_name = NULL;

	workpiece_name = cJSON_GetObjectItem(data_json, "workpiece_id");
	if (workpiece_name == NULL || workpiece_name->valuestring == NULL) {
		perror("json");

		return FAIL;
	}
	json_data = cJSON_CreateObject();

	left_data = cJSON_CreateArray();
	sprintf(sql, "select name from points where name like '%sleft%';", workpiece_name->valuestring);
	select_info_sqlite3(DB_POINTS, sql, &resultp, &nrow, &ncloumn);
	for (i = 0; i < nrow; i++) {
		cJSON_AddItemToArray(left_data, cJSON_CreateString(resultp[ncloumn * (i + 1)]));
	}
	cJSON_AddItemToObject(json_data, "left_workstation", left_data);
	sqlite3_free_table(resultp);

	right_data = cJSON_CreateArray();
	sprintf(sql, "select name from points where name like '%sright%';", workpiece_name->valuestring);
	select_info_sqlite3(DB_POINTS, sql, &resultp, &nrow, &ncloumn);
	for (i = 0; i < nrow; i++) {
		cJSON_AddItemToArray(right_data, cJSON_CreateString(resultp[ncloumn * (i + 1)]));
	}
	cJSON_AddItemToObject(json_data, "right_workstation", right_data);
	sqlite3_free_table(resultp);

	*ret_f_content = cJSON_Print(json_data);
	cJSON_Delete(json_data);
	json_data = NULL;
	/* content is NULL */
	if (*ret_f_content == NULL) {
		perror("cJSON_Print");

		return FAIL;
	}

	return SUCCESS;
}

/* get torque ptemp list */
static int torque_get_ptemp_list(char **ret_f_content)
{
	*ret_f_content = get_dir_filename_strhead(DIR_TEMPLATE, "ptemp");
	/* file is NULL */
	if (*ret_f_content == NULL) {
		perror("get dir content");

		return FAIL;
	}

	return SUCCESS;
}

/* get torque main list */
static int torque_get_main_list(char **ret_f_content)
{
	*ret_f_content = get_dir_filename_strhead(DIR_TEMPLATE, "main");
	/* file is NULL */
	if (*ret_f_content == NULL) {
		perror("get dir content");

		return FAIL;
	}

	return SUCCESS;
}

/* torque get custom pause */
static int torque_get_custom_pause(char **ret_f_content, const cJSON *data_json)
{
	char sql[1024] = {0};
	cJSON *json_data = NULL;
	cJSON *modal_func_id = NULL;

	modal_func_id = cJSON_GetObjectItem(data_json, "modal_func_id");
	if (modal_func_id == NULL || modal_func_id->valuestring == NULL) {
		perror("json");

		return FAIL;
	}

	sprintf(sql, "select modal_title, modal_content from torquesys_custom where modal_func_id = '%s';", modal_func_id->valuestring);
	if (select_info_json_sqlite3_single(DB_TORQUE_CUSTOM, sql, &json_data) == -1) {
		perror("select");

		return FAIL;
	}

	*ret_f_content = cJSON_Print(json_data);
	cJSON_Delete(json_data);
	json_data = NULL;
	/* content is NULL */
	if (*ret_f_content == NULL) {
		perror("cJSON_Print");

		return FAIL;
	}

	return SUCCESS;
}

/* torque_get_all_custom_pause */
static int torque_get_all_custom_pause(char **ret_f_content)
{
	char sql[1024] = {0};
	cJSON *json_data = NULL;

	sprintf(sql, "select * from torquesys_custom;");
	if (select_info_json_sqlite3(DB_TORQUE_CUSTOM, sql, &json_data) == -1) {
		perror("select");

		return FAIL;
	}

	*ret_f_content = cJSON_Print(json_data);
	cJSON_Delete(json_data);
	json_data = NULL;
	/* content is NULL */
	if (*ret_f_content == NULL) {
		perror("cJSON_Print");

		return FAIL;
	}

	return SUCCESS;
}

/* get ip */
static int get_ip(char **ret_f_content)
{
	cJSON *root_json = NULL;
	char strline[LINE_LEN] = { 0 };
	FILE *fp;
	char *ptr = NULL;
	int i = 0;
	int j = 0;

	root_json = cJSON_CreateObject();
	if ((fp = fopen(ROBOT_CFG, "r")) == NULL) {
		perror("open file");

		return FAIL;
	}
	while (fgets(strline, LINE_LEN, fp) != NULL) {
		/* without '\n' '\r' '\t' */
		i = 0;
		j = 0;
		while (strline[i] != '\0') {
			if (strline[i] != '\n' && strline[i] != '\r' && strline[i] !='\t') {
				strline[j++] = strline[i];
			}
			i++;
		}
		strline[j] = '\0';

		if (ptr = strstr(strline, "CTRL_IP = ")) {
			cJSON_AddStringToObject(root_json, "ctrl_ip", (ptr + 10));
		}
		if (ptr = strstr(strline, "USER_IP = ")) {
			cJSON_AddStringToObject(root_json, "user_ip", (ptr + 10));
		}
		bzero(strline, sizeof(char)*LINE_LEN);
	}
	fclose(fp);

	*ret_f_content = cJSON_Print(root_json);
	cJSON_Delete(root_json);
	root_json = NULL;
	if (*ret_f_content == NULL) {
		perror("cJSON_Print");

		return FAIL;
	}
	//printf("*ret_f_content = %s\n", (*ret_f_content));

	return SUCCESS;
}

/* get blockly workspace */
static int get_blockly_workspace(char **ret_f_content, const cJSON *data_json)
{
	char dir_filename[100] = {0};

	cJSON *file_name = cJSON_GetObjectItem(data_json, "ws_name");
	if (file_name == NULL || file_name->valuestring == NULL) {
		perror("json");

		return FAIL;
	}
	sprintf(dir_filename, "%s%s", DIR_BLOCK, file_name->valuestring);

	*ret_f_content = get_file_content(dir_filename);
	/* ret_f_content is NULL or no such file or empty */
	if (*ret_f_content == NULL || strcmp(*ret_f_content, "NO_FILE") == 0 || strcmp(*ret_f_content, "Empty") == 0) {
		*ret_f_content = NULL;
		perror("get file content");

		return FAIL;
	}

	return SUCCESS;
}

/* get blockly workspace names */
static int get_blockly_workspace_names(char **ret_f_content)
{
	*ret_f_content = get_dir_filename(DIR_BLOCK);

	/* file is NULL */
	if (*ret_f_content == NULL) {
		perror("get dir content");

		return FAIL;
	}

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
	} else if(!strcmp(cmd, "get_wobj_tool_cdsystem")) {
		ret = get_wobj_tool_cdsystem(&ret_f_content);
	} else if(!strcmp(cmd, "get_ex_tool_cdsystem")) {
		ret = get_ex_tool_cdsystem(&ret_f_content);
	} else if(!strcmp(cmd, "get_exaxis_cdsystem")) {
		ret = get_exaxis_cdsystem(&ret_f_content);
	} else if(!strcmp(cmd, "get_user_data")) {
		ret = get_user(&ret_f_content);
	} else if(!strcmp(cmd, "get_template_data")) {
		ret = get_template(&ret_f_content);
	} else if(!strcmp(cmd, "get_tpd_name")) {
		ret = get_tpd_name(&ret_f_content);
	} else if(!strcmp(cmd, "get_log_name")) {
		ret = get_log_name(&ret_f_content);
	} else if(!strcmp(cmd, "get_log_data")) {
		data_json = cJSON_GetObjectItem(data, "data");
		if (data_json == NULL || data_json->type != cJSON_Object) {
			perror("json");
			goto end;
		}
		ret = get_log_data(&ret_f_content, data_json);
	} else if(!strcmp(cmd, "get_syscfg")) {
		ret = get_syscfg(&ret_f_content);
	} else if(!strcmp(cmd, "get_ODM_cfg")) {
		ret = get_ODM_cfg(&ret_f_content);
	} else if(!strcmp(cmd, "get_accounts")) {
		ret = get_accounts(&ret_f_content);
	} else if(!strcmp(cmd, "get_account_info")) {
		ret = get_account_info(&ret_f_content);
	} else if(!strcmp(cmd, "get_webversion")) {
		ret = get_webversion(&ret_f_content);
	} else if(!strcmp(cmd, "get_checkpoint")) {
		data_json = cJSON_GetObjectItem(data, "data");
		if (data_json == NULL || data_json->type != cJSON_Object) {
			perror("json");
			goto end;
		}
		ret = get_checkpoint(&ret_f_content, data_json);
	} else if(!strcmp(cmd, "get_robot_cfg")) {
		ret = get_robot_cfg(&ret_f_content);
	} else if(!strcmp(cmd, "get_ex_device_cfg")) {
		ret = get_ex_device_cfg(&ret_f_content);
	} else if(!strcmp(cmd, "get_weave")) {
		ret = get_weave(&ret_f_content);
	} else if(!strcmp(cmd, "get_exaxis_cfg")) {
		ret = get_exaxis_cfg(&ret_f_content);
	} else if(!strcmp(cmd, "get_plugin_info")) {
		ret = get_plugin_info(&ret_f_content);
	} else if(!strcmp(cmd, "get_plugin_nav")) {
		ret = get_plugin_nav(&ret_f_content);
	} else if(!strcmp(cmd, "get_plugin_config")) {
		data_json = cJSON_GetObjectItem(data, "data");
		if (data_json == NULL || data_json->type != cJSON_Object) {
			perror("json");
			goto end;
		}
		ret = get_plugin_config(&ret_f_content, data_json);
	} else if(!strcmp(cmd, "get_DH_file")) {
		ret = get_DH_file(&ret_f_content);
	} else if(!strcmp(cmd, "get_robot_serialnumber")) {
		ret = get_robot_serialnumber(&ret_f_content);
	} else if(!strcmp(cmd, "get_robot_type")) {
		ret = get_robot_type(&ret_f_content);
	} else if(!strcmp(cmd, "get_varlist")) {
		ret = get_varlist(&ret_f_content);
	} else if(!strcmp(cmd, "get_checkvar")) {
		data_json = cJSON_GetObjectItem(data, "data");
		if (data_json == NULL || data_json->type != cJSON_Object) {
			perror("json");
			goto end;
		}
		ret = get_checkvar(&ret_f_content, data_json);
	} else if(!strcmp(cmd, "torque_get_wpname_list")) {
		ret = torque_get_wpname_list(&ret_f_content);
	} else if(!strcmp(cmd, "torque_get_cfg")) {
		data_json = cJSON_GetObjectItem(data, "data");
		if (data_json == NULL || data_json->type != cJSON_Object) {
			perror("json");
			goto end;
		}
		ret = torque_get_cfg(&ret_f_content, data_json);
	} else if(!strcmp(cmd, "torque_get_wkpoints")) {
		data_json = cJSON_GetObjectItem(data, "data");
		if (data_json == NULL || data_json->type != cJSON_Object) {
			perror("json");
			goto end;
		}
		ret = torque_get_wkpoints(&ret_f_content, data_json);
	} else if(!strcmp(cmd, "torque_get_points")) {
		data_json = cJSON_GetObjectItem(data, "data");
		if (data_json == NULL || data_json->type != cJSON_Object) {
			perror("json");
			goto end;
		}
		ret = torque_get_points(&ret_f_content, data_json);
	} else if(!strcmp(cmd, "torque_get_ptemp_list")) {
		ret = torque_get_ptemp_list(&ret_f_content);
	} else if(!strcmp(cmd, "torque_get_main_list")) {
		ret = torque_get_main_list(&ret_f_content);
	} else if(!strcmp(cmd, "get_DIO_cfg")) {
		ret = get_DIO_cfg(&ret_f_content);
	} else if(!strcmp(cmd, "get_TSP_flg")) {
		ret = get_TSP_flg(&ret_f_content);
	} else if(!strcmp(cmd, "torque_get_custom_pause")) {
		data_json = cJSON_GetObjectItem(data, "data");
		if (data_json == NULL || data_json->type != cJSON_Object) {
			perror("json");
			goto end;
		}
		ret = torque_get_custom_pause(&ret_f_content, data_json);
	} else if(!strcmp(cmd, "torque_get_all_custom_pause")) {
		ret = torque_get_all_custom_pause(&ret_f_content);
	} else if(!strcmp(cmd, "get_ip")) {
		ret = get_ip(&ret_f_content);
	} else if(!strcmp(cmd, "get_blockly_workspace")) {
		data_json = cJSON_GetObjectItem(data, "data");
		if (data_json == NULL || data_json->type != cJSON_Object) {
			perror("json");
			goto end;
		}
		ret = get_blockly_workspace(&ret_f_content, data_json);
	} else if(!strcmp(cmd, "get_blockly_workspace_names")) {
		ret = get_blockly_workspace_names(&ret_f_content);
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
	//printf("ret_f_content = %s\n", ret_f_content);
	websSetStatus(wp, 200);
	websWriteHeaders(wp, -1, 0);
	websWriteEndHeaders(wp);
	websWrite(wp, ret_f_content);
	websDone(wp);
	/* free ret_f_content */
	if (ret_f_content != NULL) {
		free(ret_f_content);
		ret_f_content = NULL;
	}

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
	if (ret_f_content != NULL) {
		free(ret_f_content);
		ret_f_content = NULL;
	}
	return;
}

