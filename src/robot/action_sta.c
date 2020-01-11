
/********************************* Includes ***********************************/

#include    "goahead.h"
#include	"cJSON.h"
#include 	"tools.h"
#include 	"robot_socket.h"
#include	"action_sta.h"

/********************************* Defines ************************************/

extern CTRL_STATE ctrl_state;
extern SOCKET_INFO socket_cmd;
extern SOCKET_INFO socket_file;
extern SOCKET_INFO socket_status;

/********************************* Function declaration ***********************/

static int connect_status(char *ret_status);
static int menu(char *ret_status);
static int basic(char *ret_status);
static int program_teach(char *ret_status);

/********************************* Code *************************************/

/* connect status */
static int connect_status(char *ret_status)
{
	char *buf = NULL;
	cJSON *root_json = NULL;
	int ret_connect_status = 0;

	/* cmd file status all connect */
	if ((socket_status.connect_status + socket_cmd.connect_status + socket_file.connect_status) == 3) {
		ret_connect_status = 1;
	} else {
		ret_connect_status = 0;
	}
	root_json = cJSON_CreateObject();
	cJSON_AddNumberToObject(root_json, "cons", ret_connect_status);
	buf = cJSON_Print(root_json);
	strcpy(ret_status, buf);
	free(buf);
	buf = NULL;
	cJSON_Delete(root_json);
	root_json = NULL;

	return SUCCESS;
}

/* menu */
static int menu(char *ret_status)
{
	char *buf = NULL;
	cJSON *root_json = NULL;

	//printf("ctrl_state.robot_mode = %u\n", ctrl_state.robot_mode);
	root_json = cJSON_CreateObject();
	cJSON_AddNumberToObject(root_json, "mode", ctrl_state.robot_mode);
	buf = cJSON_Print(root_json);
	strcpy(ret_status, buf);
	free(buf);
	buf = NULL;
	cJSON_Delete(root_json);
	root_json = NULL;

	return SUCCESS;
}

/* basic */
static int basic(char *ret_status)
{
	int i = 0;
	char *buf = NULL;
	char joint[10] = {0};
	double joint_value = 0;
	cJSON *root_json = NULL;
	cJSON *joints_json = NULL;

	/*for (i = 0; i < 6; i++) {
		printf("ctrl_state.jt_cur_pos[%d] = %.3lf\n", i, ctrl_state.jt_cur_pos[i]);
	}*/
	root_json = cJSON_CreateObject();
	joints_json = cJSON_CreateObject();
	cJSON_AddItemToObject(root_json, "joints", joints_json);
	for (i = 0; i < 6; i++) {
		joint_value = double_round(ctrl_state.jt_cur_pos[i], 3);
		//printf("joint_value = %.3lf\n", joint_value);
		sprintf(joint, "j%d", (i+1));
		cJSON_AddNumberToObject(joints_json, joint, joint_value);
	}
	buf = cJSON_Print(root_json);
	strcpy(ret_status, buf);
	free(buf);
	buf = NULL;
	cJSON_Delete(root_json);
	root_json = NULL;

	return SUCCESS;
}

/* program teach */
static int program_teach(char *ret_status)
{
	char *buf = NULL;
	cJSON *root_json = NULL;

	//printf("ctrl_state.line_number = %u\n", ctrl_state.line_number);
	root_json = cJSON_CreateObject();
	cJSON_AddNumberToObject(root_json, "line_number", ctrl_state.line_number);
	cJSON_AddNumberToObject(root_json, "program_state", ctrl_state.program_state);
	buf = cJSON_Print(root_json);
	strcpy(ret_status, buf);
	free(buf);
	buf = NULL;
	cJSON_Delete(root_json);
	root_json = NULL;

	return SUCCESS;
}

/* get motion controller data and return to page */
void sta(Webs *wp)
{
	char *ret_status = NULL;
	int ret = FAIL;
	char *buf = NULL;
	char *cmd = NULL;
	cJSON *command = NULL;
	cJSON *data = NULL;

	/*calloc content*/
	ret_status = (char *)calloc(1, sizeof(char)*1024);
	if (ret_status == NULL) {
		perror("calloc");
		goto end;
	}
	data = cJSON_Parse(wp->input.servp);
	if (data == NULL) {
		perror("json");
		goto end;
	}
	//printf("data:%s\n", buf = cJSON_Print(data));
	free(buf);
	buf = NULL;
	/* get cmd */
	command = cJSON_GetObjectItem(data, "cmd");
	if (command == NULL) {
		perror("json");
		goto end;
	}
	cmd = command->valuestring;
	if(!strcmp(cmd, "cons")) {
		ret = connect_status(ret_status);
	} else if(!strcmp(cmd, "menu")) {
		ret = menu(ret_status);
	} else if(!strcmp(cmd, "basic")) {
		ret = basic(ret_status);
	} else if(!strcmp(cmd, "program_teach")) {
		ret = program_teach(ret_status);
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
	//printf("ret_status = %s\n", ret_status);
	websSetStatus(wp, 200);
	websWriteHeaders(wp, -1, 0);
	websWriteEndHeaders(wp);
	websWrite(wp, ret_status);
	websDone(wp);
	/* free ret_status */
	free(ret_status);
	ret_status = NULL;

	return;

end:
	/* cjson delete */
	cJSON_Delete(data);
	data = NULL;
	websSetStatus(wp, 403);
	websWriteHeaders(wp, -1, 0);
	websWriteEndHeaders(wp);
	//websWrite(wp, ret_status);
	websWrite(wp, "fail");
	websDone(wp);
	/* free ret_status */
	free(ret_status);
	ret_status = NULL;
	return;
}

