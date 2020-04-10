
/********************************* Includes ***********************************/

#include    "goahead.h"
#include	"cJSON.h"
#include 	"tools.h"
#include 	"robot_socket.h"
#include	"action_sta.h"

/********************************* Defines ************************************/

extern CTRL_STATE ctrl_state;
extern CTRL_STATE vir_ctrl_state;
extern FB_LinkQuene fb_quene;
//extern STATE_FB state_fb;
//extern float state_ret[100][10];
extern SOCKET_INFO socket_cmd;
extern SOCKET_INFO socket_file;
extern SOCKET_INFO socket_status;
extern SOCKET_INFO socket_state;
extern SOCKET_INFO socket_vir_cmd;
extern SOCKET_INFO socket_vir_file;
extern SOCKET_INFO socket_vir_status;
extern STATE_FEEDBACK state_fb;
extern int robot_type;

/********************************* Function declaration ***********************/

static int connect_status(char *ret_status);
static int basic(char *ret_status, CTRL_STATE *state);
static int program_teach(char *ret_status, CTRL_STATE *state);
static int vardata_feedback(char *ret_status);

/********************************* Code *************************************/

/* connect status */
static int connect_status(char *ret_status)
{
	char *buf = NULL;
	cJSON *root_json = NULL;
	int ret_connect_status = 0;

	if (robot_type == 1) { // "1" 代表实体机器人
		/* cmd file status state all connect */
		if (socket_status.connect_status && socket_cmd.connect_status && socket_file.connect_status && socket_state.connect_status) {
			ret_connect_status = 1;
		} else {
			ret_connect_status = 0;
		}
	} else { // "0" 代表虚拟机器人
		/* cmd file status state all connect */
		if (socket_status.connect_status && socket_cmd.connect_status && socket_file.connect_status) {
			ret_connect_status = 1;
		} else {
			ret_connect_status = 0;
		}
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

/* basic */
static int basic(char *ret_status, CTRL_STATE *state)
{
	int i = 0;
	char *buf = NULL;
	char joint[10] = {0};
	double joint_value = 0;
	double tcp_value[6] = {0};
	char key[10] = {0};
	cJSON *root_json = NULL;
	cJSON *joints_json = NULL;
	cJSON *tcp_json = NULL;
	cJSON *error_json = NULL;
	int io[16] = {0};
	int n1 = 0;
	int n2 = 0;

	//printf("state->cl_dgt_output_h = %d\n", state->cl_dgt_output_h);
	//printf("state->cl_dgt_output_l = %d\n", state->cl_dgt_output_l);
	n1 = state->cl_dgt_output_l;
	n2 = state->cl_dgt_output_h;
	for(i = 0; i < 16; i++) {
		if (i < 8) {
			io[i]=n1%2;
			n1=n1/2;
		}
		if (i >= 8) {
			io[i] = n2%2;
			n2 = n2/2;
		}
	}

	/*for (i = 0; i < 6; i++) {
		printf("state->jt_cur_pos[%d] = %.3lf\n", i, state->jt_cur_pos[i]);
	}*/
	root_json = cJSON_CreateObject();
	joints_json = cJSON_CreateObject();
	tcp_json = cJSON_CreateObject();
	error_json = cJSON_CreateArray();
	itoa(state->toolNum, key, 10);
	cJSON_AddItemToObject(root_json, "error_info", error_json);
	cJSON_AddItemToObject(root_json, "joints", joints_json);
	cJSON_AddItemToObject(root_json, "tcp", tcp_json);
	cJSON_AddItemToObject(root_json, "cl_do", cJSON_CreateIntArray(io, 16));
	cJSON_AddNumberToObject(root_json, "mode", state->robot_mode);
	cJSON_AddStringToObject(root_json, "toolnum", key);
	cJSON_AddNumberToObject(root_json, "vel_radio", state->vel_ratio);
	cJSON_AddNumberToObject(root_json, "robot_type", robot_type);
	for (i = 0; i < 6; i++) {
		joint_value = double_round(state->jt_cur_pos[i], 3);
		//printf("joint_value = %.3lf\n", joint_value);
		sprintf(joint, "j%d", (i+1));
		cJSON_AddNumberToObject(joints_json, joint, joint_value);
	}
	for (i = 0; i < 6; i++) {
		tcp_value[i] = double_round(state->tl_cur_pos[i], 3);
	}
	cJSON_AddNumberToObject(tcp_json, "x", tcp_value[0]);
	cJSON_AddNumberToObject(tcp_json, "y", tcp_value[1]);
	cJSON_AddNumberToObject(tcp_json, "z", tcp_value[2]);
	cJSON_AddNumberToObject(tcp_json, "rx", tcp_value[3]);
	cJSON_AddNumberToObject(tcp_json, "ry", tcp_value[4]);
	cJSON_AddNumberToObject(tcp_json, "rz", tcp_value[5]);

	if (state->aliveSlaveNumError == 1) {
		cJSON_AddStringToObject(error_json, "key", "活动从站数量错误");
	}
	switch(state->slaveComError) {
		case 1:
			cJSON_AddStringToObject(error_json, "key", "从站掉线");
			break;
		case 2:
			cJSON_AddStringToObject(error_json, "key", "从站状态与设置值不一致");
			break;
		case 3:
			cJSON_AddStringToObject(error_json, "key", "从站未配置");
			break;
		case 4:
			cJSON_AddStringToObject(error_json, "key", "从站配置错误");
			break;
		case 5:
			cJSON_AddStringToObject(error_json, "key", "从站初始化错误");
			break;
		case 6:
			cJSON_AddStringToObject(error_json, "key", "从站邮箱通信初始化错误");
			break;
		default:
			break;
	}
	switch(state->cmdPointError) {
		case 1:
			cJSON_AddStringToObject(error_json, "key", "关节指令点错误");
			break;
		case 2:
			cJSON_AddStringToObject(error_json, "key", "直线目标点错误（包括工具不符）");
			break;
		case 3:
			cJSON_AddStringToObject(error_json, "key", "圆弧中间点错误（包括工具不符）");
			break;
		case 4:
			cJSON_AddStringToObject(error_json, "key", "圆弧目标点错误（包括工具不符）");
			break;
		case 5:
			cJSON_AddStringToObject(error_json, "key", "TPD指令点错误");
			break;
		case 6:
			cJSON_AddStringToObject(error_json, "key", "TPD指令工具与当前工具不符");
			break;
		case 7:
			cJSON_AddStringToObject(error_json, "key", "TPD当前指令与下一指令起始点偏差过大");
			break;
		case 8:
			cJSON_AddStringToObject(error_json, "key", "PTP关节指令超限");
			break;
		case 9:
			cJSON_AddStringToObject(error_json, "key", "TPD关节指令超限");
			break;
		case 10:
			cJSON_AddStringToObject(error_json, "key", "LIN/ARC下发关节指令超限");
			break;
		case 11:
			cJSON_AddStringToObject(error_json, "key", "关节空间内指令速度超限");
			break;
		case 12:
			cJSON_AddStringToObject(error_json, "key", "笛卡尔空间内指令超速");
			break;
		case 13:
			cJSON_AddStringToObject(error_json, "key", "关节空间内扭矩指令超限");
			break;
		case 14:
			cJSON_AddStringToObject(error_json, "key", "下一指令关节配置发生变化");
			break;
		case 15:
			cJSON_AddStringToObject(error_json, "key", "JOG关节指令超限");
			break;
		default:
			break;
	}
	switch(state->ioError) {
		case 1:
			cJSON_AddStringToObject(error_json, "key", "通道错误");
			break;
		case 2:
			cJSON_AddStringToObject(error_json, "key", "数值错误");
			break;
		case 3:
			cJSON_AddStringToObject(error_json, "key", "WaitDI等待超时");
			break;
		case 4:
			cJSON_AddStringToObject(error_json, "key", "WaitAI等待超时");
			break;
		case 5:
			cJSON_AddStringToObject(error_json, "key", "WaitAxleDI等待超时");
			break;
		case 6:
			cJSON_AddStringToObject(error_json, "key", "WaitAxleAI等待超时");
			break;
		default:
			break;
	}
	if (state->gripperError == 1) {
		cJSON_AddStringToObject(error_json, "key", "指令顺序错误");
	}
	switch(state->fileError) {
		case 1:
			cJSON_AddStringToObject(error_json, "key", "zbt配置文件版本错误");
			break;
		case 2:
			cJSON_AddStringToObject(error_json, "key", "zbt配置文件加载失败");
			break;
		case 3:
			cJSON_AddStringToObject(error_json, "key", "user配置文件加载失败");
			break;
		default:
			break;
	}
	if (state->paraError == 1) {
		cJSON_AddStringToObject(error_json, "key", "工具号超限错误");
	}
	switch(state->alarm) {
		case 1:
			cJSON_AddStringToObject(error_json, "key", "肩关节配置变化");
			break;
		case 2:
			cJSON_AddStringToObject(error_json, "key", "肘关节配置变化");
			break;
		case 3:
			cJSON_AddStringToObject(error_json, "key", "腕关节配置变化");
			break;
		case 4:
			cJSON_AddStringToObject(error_json, "key", "RPY初始化失败");
			break;
		default:
			break;
	}
	if (state->dr_com_err == 1) {
		cJSON_AddStringToObject(error_json, "key", "通信故障:控制器与驱动器心跳检测故障");
	}
	switch ((int)state->dr_err) {
		case 1:
			cJSON_AddStringToObject(error_json, "key", "1轴驱动器故障");
			break;
		case 2:
			cJSON_AddStringToObject(error_json, "key", "2轴驱动器故障");
			break;
		case 3:
			cJSON_AddStringToObject(error_json, "key", "3轴驱动器故障");
			break;
		case 4:
			cJSON_AddStringToObject(error_json, "key", "4轴驱动器故障");
			break;
		case 5:
			cJSON_AddStringToObject(error_json, "key", "5轴驱动器故障");
			break;
		case 6:
			cJSON_AddStringToObject(error_json, "key", "6轴驱动器故障");
			break;
		default:
			break;
	}
	switch ((int)state->out_sflimit_err) {
		case 1:
			cJSON_AddStringToObject(error_json, "key", "1轴超出软限位故障");
			break;
		case 2:
			cJSON_AddStringToObject(error_json, "key", "2轴超出软限位故障");
			break;
		case 3:
			cJSON_AddStringToObject(error_json, "key", "3轴超出软限位故障");
			break;
		case 4:
			cJSON_AddStringToObject(error_json, "key", "4轴超出软限位故障");
			break;
		case 5:
			cJSON_AddStringToObject(error_json, "key", "5轴超出软限位故障");
			break;
		case 6:
			cJSON_AddStringToObject(error_json, "key", "6轴超出软限位故障");
			break;
		default:
			break;
	}
	switch((int)state->collision_err) {
		case 1:
			cJSON_AddStringToObject(error_json, "key", "1轴碰撞故障");
			break;
		case 2:
			cJSON_AddStringToObject(error_json, "key", "2轴碰撞故障");
			break;
		case 3:
			cJSON_AddStringToObject(error_json, "key", "3轴碰撞故障");
			break;
		case 4:
			cJSON_AddStringToObject(error_json, "key", "4轴碰撞故障");
			break;
		case 5:
			cJSON_AddStringToObject(error_json, "key", "5轴碰撞故障");
			break;
		case 6:
			cJSON_AddStringToObject(error_json, "key", "6轴碰撞故障");
			break;
		default:
			break;
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
static int program_teach(char *ret_status, CTRL_STATE *state)
{
	cJSON *root_json = NULL;
	char *buf = NULL;

	//printf("state.line_number = %u\n", state->line_number);
//	printf("state->program_state d = %d\n", state->program_state);
	root_json = cJSON_CreateObject();
	cJSON_AddNumberToObject(root_json, "line_number", state->line_number);
	cJSON_AddNumberToObject(root_json, "program_state", state->program_state);
	buf = cJSON_Print(root_json);
	strcpy(ret_status, buf);
	free(buf);
	buf = NULL;
	cJSON_Delete(root_json);
	root_json = NULL;

	return SUCCESS;
}

/* vardata_feedback */
static int vardata_feedback(char *ret_status)
{
	int i = 0;
	int j = 0;
	char key[10] = {0};
	char *buf = NULL;
	cJSON *root_json = NULL;
	cJSON *root = NULL;

	if (fb_queneempty(&fb_quene)) {
		fb_print_node_num(fb_quene);
		root_json = cJSON_CreateObject();

		/*printf("state_fb.iCount= %d\n", state_fb.icount);
		printf("state_ret[0][0] = %f\n", state_ret[0][0]);
		printf("state_ret[1][0] = %f\n", state_ret[1][0]);
		printf("state_ret[0][1] = %f\n", state_ret[0][1]);
		printf("state_ret[1][1] = %f\n", state_ret[1][1]);*/
		for (i = 0; i < state_fb.icount; i++) {
			itoa(state_fb.id[i], key, 10);
			root = cJSON_CreateArray();
			cJSON_AddItemToObject(root_json, key, root);
			for (j = 0; j < 100; j++) {
				//cJSON_AddNumberToObject(root, "key", state_fb.var[j][i]);
				cJSON_AddNumberToObject(root, "key", fb_quene.front->next->data.fb[j][i]);
				//cJSON_AddNumberToObject(root, "key", 100);
			}
		}
		buf = cJSON_Print(root_json);
		//printf("send to GUI = %s\n", buf);
		strcpy(ret_status, buf);

		/* delete front node */
		pthread_mutex_lock(&socket_state.mute);
		fb_dequene(&fb_quene);
		pthread_mutex_unlock(&socket_state.mute);
	/** quene is empty */
	} else {
		root_json = cJSON_CreateObject();
		cJSON_AddNumberToObject(root_json, "empty_data", 0);
		buf = cJSON_Print(root_json);
		strcpy(ret_status, buf);
		//printf("send empty data to GUI\n");
	}
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
	//char *buf = NULL;
	char *cmd = NULL;
	cJSON *command = NULL;
	cJSON *data = NULL;

	/*calloc content*/
	//ret_status = (char *)calloc(1, sizeof(char)*1024);
	ret_status = (char *)calloc(1, 10000);
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
	//free(buf);
	//buf = NULL;
	/* get cmd */
	command = cJSON_GetObjectItem(data, "cmd");
	if (command == NULL) {
		perror("json");
		goto end;
	}
	CTRL_STATE *state = NULL;
	if (robot_type == 1) { // "1" 代表实体机器人
		state = &ctrl_state;
	} else { // "0" 代表虚拟机器人
		state = &vir_ctrl_state;
	}

	cmd = command->valuestring;
	if(!strcmp(cmd, "cons")) {
		ret = connect_status(ret_status);
	} else if(!strcmp(cmd, "basic")) {
		ret = basic(ret_status, state);
	} else if(!strcmp(cmd, "program_teach")) {
		ret = program_teach(ret_status, state);
	} else if(!strcmp(cmd, "vardata_feedback")) {
		ret = vardata_feedback(ret_status);
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

