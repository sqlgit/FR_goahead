
/********************************* Includes ***********************************/

#include    "goahead.h"
#include	"cJSON.h"
#include 	"tools.h"
#include	"action_get.h"

/********************************* Defines ************************************/

extern int cur_state;

/********************************* Function declaration ***********************/

static int get_points_data(char **ret_f_content);
static int get_user_data(char **ret_f_content);
static int get_template_data(char **ret_f_content);
static int get_state_feedback(char **ret_f_content);

/*********************************** Code *************************************/

/* get points file content */
static int get_points_data(char **ret_f_content)
{
	*ret_f_content = get_file_content(FILE_POINTS);
	/* file is NULL */
	if (*ret_f_content == NULL) {
		perror("get file content");

		return FAIL;
	}

	return SUCCESS;
}

/* get user file content */
static int get_user_data(char **ret_f_content)
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
static int get_template_data(char **ret_f_content)
{
	*ret_f_content = get_dir_content(DIR_TEMPLATE);
	/* file is NULL */
	if (*ret_f_content == NULL) {
		perror("get dir content");

		return FAIL;
	}

	return SUCCESS;
}

/* get state feedback */
static int get_state_feedback(char **ret_f_content)
{
	char *buf = NULL;
	cJSON *root_json = NULL;

	root_json = cJSON_CreateObject();
	printf("cur_state = %d\n", cur_state);
	cJSON_AddNumberToObject(root_json, "state", cur_state);
	buf = cJSON_Print(root_json);
	printf("buf = %s\n", buf);
	*ret_f_content = (char *)calloc(1, strlen(buf)+1);
	if(*ret_f_content != NULL) {
		strcpy((*ret_f_content), buf);
	} else {
		perror("calloc");
	}
	printf("*ret_f_content = %s\n", (*ret_f_content));
	free(buf);
	buf = NULL;
	cJSON_Delete(root_json);
	root_json = NULL;

	return SUCCESS;
}

/* get webserver data and return to page */
void get(Webs *wp)
{
	int ret = FAIL;
	char *buf = NULL;
	char *cmd = NULL;
	cJSON *command = NULL;
	cJSON *data = NULL;
	char *ret_f_content = NULL;

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
		ret = get_points_data(&ret_f_content);
	} else if(!strcmp(cmd, "get_user_data")) {
		ret = get_user_data(&ret_f_content);
	} else if(!strcmp(cmd, "get_template_data")) {
		ret = get_template_data(&ret_f_content);
	} else if(!strcmp(cmd, "get_state_feedback")) {
		ret = get_state_feedback(&ret_f_content);
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

