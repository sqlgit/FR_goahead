
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
static int get_exaxis_cdsystem(char **ret_f_content);
static int get_user(char **ret_f_content);
static int get_template(char **ret_f_content);
static int get_tpd_name(char **ret_f_content);
static int get_log_name(char **ret_f_content);
static int get_log_data(char **ret_f_content, const cJSON *data_json);
static int get_syscfg(char **ret_f_content);
static int get_accounts(char **ret_f_content);
static int get_webversion(char **ret_f_content);
static int get_checkpoint(char **ret_f_content, const cJSON *data_json);
static int get_robot_cfg(char **ret_f_content);
static int get_weave(char **ret_f_content);
static int get_exaxis_cfg(char **ret_f_content);
static int get_plugin_info(char **ret_f_content);
static int get_plugin_nav(char **ret_f_content);
static int get_plugin_config(char **ret_f_content, const cJSON *data_json);

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

	sprintf(sql, "select * from coordinate_system order by id ASC");
	if (select_info_json_sqlite3(DB_CDSYSTEM, sql, &json_data) == -1) {
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
	cJSON *root_json = NULL;

	root_json = cJSON_CreateObject();
	if ((fp = fopen(ROBOT_CFG, "r")) == NULL) {
		perror("user.config : open file");

		return FAIL;
	}
	while (fgets(strline, LINE_LEN, fp) != NULL) {
		if (is_in(strline, "=") == -1) {
			continue;
		}
		strrpc(strline, "\n", "");
		//printf("user.config strline = %s\n", strline);
		if (string_to_string_list(strline, " = ", &size, &array) == 0) {
			perror("string to string list");
			fclose(fp);
			string_list_free(array, size);

			return FAIL;
		}
		if (array[0] != NULL && (array[1] != NULL || array[1] == 0)) {
			cJSON_AddStringToObject(root_json, my_strlwr(array[0]), array[1]);
		}
		string_list_free(array, size);
		bzero(strline, sizeof(char)*LINE_LEN);
	}
	fclose(fp);

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

/** get exaxis cfg */
static int get_exaxis_cfg(char **ret_f_content)
{
	char strline[LINE_LEN] = {0};
	FILE *fp;
	cJSON *root_json = NULL;
	cJSON *item1 = NULL;
	cJSON *item2 = NULL;
	cJSON *item3 = NULL;
	cJSON *item4 = NULL;

	if ((fp = fopen(EXAXIS_CFG, "r")) == NULL) {
		perror("open file");

		return FAIL;
	}
	root_json = cJSON_CreateArray();
	item1 = cJSON_CreateObject();
	item2 = cJSON_CreateObject();
	item3 = cJSON_CreateObject();
	item4 = cJSON_CreateObject();
	cJSON_AddItemToArray(root_json, item1);
	cJSON_AddItemToArray(root_json, item2);
	cJSON_AddItemToArray(root_json, item3);
	cJSON_AddItemToArray(root_json, item4);
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

	if ((fp = fopen(ROBOT_CFG, "r")) == NULL) {
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
	if ((fp = fopen(ROBOT_CFG, "r")) == NULL) {
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
	if (ret_f_content != NULL) {
		free(ret_f_content);
		ret_f_content = NULL;
	}
	return;
}

