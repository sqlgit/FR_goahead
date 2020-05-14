
/********************************* Includes ***********************************/

#include    "goahead.h"
#include	"cJSON.h"
#include 	"tools.h"
#include 	"robot_socket.h"
#include	"action_sta.h"

/********************************* Defines ************************************/

timer_t timerid;
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
static int test_index = 0;

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
	char content[100] = {0};
	double joint_value = 0;
	double tcp_value[6] = {0};
	cJSON *root_json = NULL;
	cJSON *joints_json = NULL;
	cJSON *tcp_json = NULL;
	cJSON *error_json = NULL;
	cJSON *feedback_json = NULL;
	cJSON *array_json = NULL;
	int array[16] = {0};
	Qnode *p = NULL;

	//printf("state->cl_dgt_output_h = %d\n", state->cl_dgt_output_h);
	//printf("state->cl_dgt_output_l = %d\n", state->cl_dgt_output_l);

	/*for (i = 0; i < 6; i++) {
		printf("state->jt_cur_pos[%d] = %.3lf\n", i, state->jt_cur_pos[i]);
	}*/
	root_json = cJSON_CreateObject();
	joints_json = cJSON_CreateObject();
	tcp_json = cJSON_CreateObject();
	feedback_json = cJSON_CreateObject();
	error_json = cJSON_CreateArray();
	cJSON_AddNumberToObject(root_json, "state", state->ctrl_query_state);
	cJSON_AddNumberToObject(root_json, "program_state", state->program_state);
	cJSON_AddItemToObject(root_json, "error_info", error_json);
	cJSON_AddItemToObject(root_json, "set_feedback", feedback_json);
	cJSON_AddItemToObject(root_json, "joints", joints_json);
	cJSON_AddItemToObject(root_json, "tcp", tcp_json);

	//printf("state->gripperActStatus = %d\n", state->gripperActStatus);
	memset(array, 0, sizeof(array));
	uint16_to_array(state->gripperActStatus, array);
	array_json = cJSON_CreateArray();
	cJSON_AddItemToObject(root_json, "gripper_state", array_json);
	for (i = 0; i < 8; i++) {
		cJSON_AddNumberToObject(array_json, "key", array[i]);
	}

	memset(array, 0, sizeof(array));
	uint8_to_array(state->cl_dgt_output_l, state->cl_dgt_output_h, array);
	cJSON_AddItemToObject(root_json, "cl_do", cJSON_CreateIntArray(array, 16));
	memset(array, 0, sizeof(array));
	uint8_to_array(state->cl_dgt_input_l, state->cl_dgt_input_h, array);
	cJSON_AddItemToObject(root_json, "cl_di", cJSON_CreateIntArray(array, 16));
	memset(array, 0, sizeof(array));
	uint8_to_array(state->tl_dgt_output_l, state->tl_dgt_output_h, array);
	cJSON_AddItemToObject(root_json, "tl_do", cJSON_CreateIntArray(array, 16));
	memset(array, 0, sizeof(array));
	uint8_to_array(state->tl_dgt_input_l, state->tl_dgt_input_h, array);
	cJSON_AddItemToObject(root_json, "tl_di", cJSON_CreateIntArray(array, 16));

	array_json = cJSON_CreateArray();
	cJSON_AddItemToObject(root_json, "ai", array_json);
	for (i = 0; i < 6; i++) {
		//printf("state->analog_input[%d] = %d\n", i, state->analog_input[i]);
		cJSON_AddNumberToObject(array_json, "key", double_round(1.0*state->analog_input[i]/4095, 3));
	}

	array_json = cJSON_CreateArray();
	cJSON_AddItemToObject(root_json, "ao", array_json);
	for (i = 0; i < 6; i++) {
		//printf("state->analog_output[%d] = %d\n", i, state->analog_output[i]);
		cJSON_AddNumberToObject(array_json, "key", double_round(1.0*state->analog_output[i]/4095, 3));
	}

	cJSON_AddNumberToObject(root_json, "mode", state->robot_mode);
	cJSON_AddNumberToObject(root_json, "toolnum", state->toolNum);
	cJSON_AddNumberToObject(root_json, "vel_radio", double_round(state->vel_ratio, 3));
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

	p = socket_cmd.ret_quene.front->next;
	while (p != NULL) {
		memset(content, 0, sizeof(content));
		sprintf(content, "%d", p->data.type);
		printf("content = %s\n", content);
		printf("p->data.msgcontent = %s\n", p->data.msgcontent);
		cJSON_AddStringToObject(feedback_json, content,  p->data.msgcontent);
		/* 删除结点信息 */
		pthread_mutex_lock(&socket_cmd.ret_mute);
		dequene(&socket_cmd.ret_quene, p->data);
		pthread_mutex_unlock(&socket_cmd.ret_mute);
		p = p->next;
	}

	p = socket_file.ret_quene.front->next;
	while (p != NULL) {
		memset(content, 0, sizeof(content));
		//sprintf(content, "%d%s", p->data.type, p->data.msgcontent);
		//cJSON_AddStringToObject(feedback_json, "key", content);
		sprintf(content, "%d", p->data.type);
		printf("content = %s\n", content);
		printf("p->data.msgcontent = %s\n", p->data.msgcontent);
		cJSON_AddStringToObject(feedback_json, content, p->data.msgcontent);
		/* 删除结点信息 */
		pthread_mutex_lock(&socket_file.ret_mute);
		dequene(&socket_file.ret_quene, p->data);
		pthread_mutex_unlock(&socket_file.ret_mute);
		p = p->next;
	}
	//printf("cJSON_Print = %s\n", cJSON_Print(feedback_json));

	if (state->strangePosFlag == 1) {
		cJSON_AddStringToObject(error_json, "key", "当前处于奇异位姿");
	}
	if (state->aliveSlaveNumError == 1) {
		cJSON_AddStringToObject(error_json, "key", "活动从站数量错误");
	}
	switch(state->gripperFaultNum) {
		case 1:
			cJSON_AddStringToObject(error_json, "key", "夹爪485超时");
			break;
		case 2:
			cJSON_AddStringToObject(error_json, "key", "夹爪指令格式错误");
			break;
		case 5:
			cJSON_AddStringToObject(error_json, "key", "夹爪动作延迟，须先激活");
			break;
		case 7:
			cJSON_AddStringToObject(error_json, "key", "夹爪未激活");
			break;
		case 8:
			cJSON_AddStringToObject(error_json, "key", "夹爪温度过高");
			break;
		case 10:
			cJSON_AddStringToObject(error_json, "key", "夹爪电压过低");
			break;
		case 11:
			cJSON_AddStringToObject(error_json, "key", "夹爪正在自动释放");
			break;
		case 12:
			cJSON_AddStringToObject(error_json, "key", "夹爪内部故障");
			break;
		case 13:
			cJSON_AddStringToObject(error_json, "key", "夹爪激活失败");
			break;
		case 14:
			cJSON_AddStringToObject(error_json, "key", "夹爪电流过大");
			break;
		case 15:
			cJSON_AddStringToObject(error_json, "key", "夹爪自动释放结束");
			break;
	}
	if (state->robot_mode == 0 && state->program_state == 4) {
		cJSON_AddStringToObject(error_json, "key", "切换拖动状态失败");
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
			cJSON_AddStringToObject(error_json, "key", "笛卡尔空间内指令超速");
			break;
		case 12:
			cJSON_AddStringToObject(error_json, "key", "关节空间内扭矩指令超限");
			break;
		case 13:
			cJSON_AddStringToObject(error_json, "key", "下一指令关节配置发生变化");
			break;
		case 14:
			cJSON_AddStringToObject(error_json, "key", "当前指令关节配置发生变化");
			break;
		case 15:
			cJSON_AddStringToObject(error_json, "key", "JOG关节指令超限");
			break;
		case 16:
			cJSON_AddStringToObject(error_json, "key", "轴1关节空间内指令速度超限");
			break;
		case 17:
			cJSON_AddStringToObject(error_json, "key", "轴2关节空间内指令速度超限");
			break;
		case 18:
			cJSON_AddStringToObject(error_json, "key", "轴3关节空间内指令速度超限");
			break;
		case 19:
			cJSON_AddStringToObject(error_json, "key", "轴4关节空间内指令速度超限");
			break;
		case 20:
			cJSON_AddStringToObject(error_json, "key", "轴5关节空间内指令速度超限");
			break;
		case 21:
			cJSON_AddStringToObject(error_json, "key", "轴6关节空间内指令速度超限");
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
		case 7:
			cJSON_AddStringToObject(error_json, "key", "通道已配置功能错误");
			break;
		default:
			break;
	}
	if (state->gripperError == 1) {
		cJSON_AddStringToObject(error_json, "key", "夹爪运动超时错误");
	}
	switch(state->fileError) {
		case 1:
			cJSON_AddStringToObject(error_json, "key", "zbt配置文件版本错误");
			break;
		case 2:
			cJSON_AddStringToObject(error_json, "key", "zbt配置文件加载失败");
			break;
		case 3:
			cJSON_AddStringToObject(error_json, "key", "user配置文件版本错误");
			break;
		case 4:
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
	memset(content, 0, sizeof(content));
	switch ((int)state->dr_err) {
		case 1:
			sprintf(content, "%d轴驱动器故障, 驱动器故障代码:%d", 1, (int)state->dr_err_code);
			cJSON_AddStringToObject(error_json, "key", content);
			break;
		case 2:
			sprintf(content, "%d轴驱动器故障, 驱动器故障代码:%d", 2, (int)state->dr_err_code);
			cJSON_AddStringToObject(error_json, "key", content);
			break;
		case 3:
			sprintf(content, "%d轴驱动器故障, 驱动器故障代码:%d", 3, (int)state->dr_err_code);
			cJSON_AddStringToObject(error_json, "key", content);
			break;
		case 4:
			sprintf(content, "%d轴驱动器故障, 驱动器故障代码:%d", 4, (int)state->dr_err_code);
			cJSON_AddStringToObject(error_json, "key", content);
			break;
		case 5:
			sprintf(content, "%d轴驱动器故障, 驱动器故障代码:%d", 5, (int)state->dr_err_code);
			cJSON_AddStringToObject(error_json, "key", content);
			break;
		case 6:
			sprintf(content, "%d轴驱动器故障, 驱动器故障代码:%d", 6, (int)state->dr_err_code);
			cJSON_AddStringToObject(error_json, "key", content);
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
	//printf("basic buf = %s\n", buf);
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
	cJSON *value_json = NULL;
	cJSON *root = NULL;

	if (fb_queneempty(&fb_quene)) {
		//fb_print_node_num(fb_quene);
		root_json = cJSON_CreateObject();
		value_json = cJSON_CreateObject();

		/*printf("state_fb.iCount= %d\n", state_fb.icount);
		printf("state_ret[0][0] = %f\n", state_ret[0][0]);
		printf("state_ret[1][0] = %f\n", state_ret[1][0]);
		printf("state_ret[0][1] = %f\n", state_ret[0][1]);
		printf("state_ret[1][1] = %f\n", state_ret[1][1]);*/
		for (i = 0; i < state_fb.icount; i++) {
			itoa(state_fb.id[i], key, 10);
			root = cJSON_CreateArray();
			cJSON_AddItemToObject(value_json, key, root);
			for (j = 0; j < 100; j++) {
				//cJSON_AddNumberToObject(root, "key", state_fb.var[j][i]);
				cJSON_AddNumberToObject(root, "key", fb_quene.front->next->data.fb[j][i]);
				//cJSON_AddNumberToObject(root, "key", 100);
			}
		}
		cJSON_AddItemToObject(root_json, "value", value_json);
		test_index++;
		cJSON_AddNumberToObject(root_json, "index", test_index);
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

void sig_handler(int signo)
{
	//printf("enter timer_signal function! %d\n", signo);
	printf("Will free Sessions!\n");
	myfreeSessions();
}

/* get motion controller data and return to page */
void sta(Webs *wp)
{
	// 添加定时器，5秒后触发，清空 session
	if (timer_delete(timerid) == -1) {
		perror("fail to timer_delete");
	}

	struct sigevent evp;
	struct sigaction act;
	memset(&act, 0, sizeof(act));
	act.sa_handler = sig_handler;
	act.sa_flags = 0;
	sigemptyset(&act.sa_mask);
	if (sigaction(SIGUSR1, &act, NULL) == -1) {
		perror("fail to sigaction");
	}

	memset(&evp, 0, sizeof(struct sigevent));
	evp.sigev_signo = SIGUSR1;
	evp.sigev_notify = SIGEV_SIGNAL;
	if (timer_create(CLOCK_REALTIME, &evp, &timerid) == -1) {
		perror("fail to timer_create");
	}

	struct itimerspec it;
	it.it_interval.tv_sec = 0;
	it.it_interval.tv_nsec = 0;
	it.it_value.tv_sec = 5;
	it.it_value.tv_nsec = 0;
	if (timer_settime(timerid, 0, &it, 0) == -1) {
		perror("fail to timer_settime");
	}

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

