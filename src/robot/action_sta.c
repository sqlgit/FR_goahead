
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
extern CTRL_STATE pre_ctrl_state;
extern CTRL_STATE pre_vir_ctrl_state;
extern FB_LinkQuene fb_quene;
extern SOCKET_INFO socket_cmd;
extern SOCKET_INFO socket_file;
extern SOCKET_INFO socket_status;
extern SOCKET_INFO socket_state;
extern SOCKET_INFO socket_vir_cmd;
extern SOCKET_INFO socket_vir_file;
extern SOCKET_INFO socket_vir_status;
extern STATE_FEEDBACK state_fb;
extern int robot_type;
extern int language;
extern ACCOUNT_INFO cur_account;
static int test_index = 0;
//int print_num = 0;

/********************************* Function declaration ***********************/

static int basic(char *ret_status, CTRL_STATE *state, CTRL_STATE *pre_state);
static int vardata_feedback(char *ret_status);

/********************************* Code *************************************/

/* basic */
static int basic(char *ret_status, CTRL_STATE *state, CTRL_STATE *pre_state)
{
	int i = 0;
	char *buf = NULL;
	char joint[10] = {0};
	char curencodertype[100] = "";
	char content[MAX_BUF] = {0};
	char en_content[MAX_BUF] = {0};
	double joint_value = 0;
	double tcp_value[6] = {0};
	cJSON *root_json = NULL;
	cJSON *joints_json = NULL;
	cJSON *curencodertype_json = NULL;
	cJSON *tcp_json = NULL;
	cJSON *error_json = NULL;
	cJSON *feedback_json = NULL;
	cJSON *array_exAxisPos = NULL;
	cJSON *array_exAxisRDY = NULL;
	cJSON *array_exAxisINPOS = NULL;
	cJSON *array_exAxisSpeedBack = NULL;
	cJSON *array_exAxisHomeStatus = NULL;
	cJSON *array_indentifydata = NULL;
	cJSON *array_register_var = NULL;
	cJSON *array_ai = NULL;
	cJSON *array_ao = NULL;
	int array[16] = {0};
	int ret_connect_status = 0;
	Qnode *p = NULL;
	SOCKET_INFO *sock_cmd = NULL;
	SOCKET_INFO *sock_file = NULL;

	//printf("state->cl_dgt_output_h = %d\n", state->cl_dgt_output_h);
	//printf("state->cl_dgt_output_l = %d\n", state->cl_dgt_output_l);

	/*for (i = 0; i < 6; i++) {
		printf("state->jt_cur_pos[%d] = %.3lf\n", i, state->jt_cur_pos[i]);
	}*/
	root_json = cJSON_CreateObject();
	joints_json = cJSON_CreateObject();
	curencodertype_json = cJSON_CreateObject();
	tcp_json = cJSON_CreateObject();
	feedback_json = cJSON_CreateObject();
	error_json = cJSON_CreateArray();
	cJSON_AddItemToObject(root_json, "error_info", error_json);
	cJSON_AddItemToObject(root_json, "set_feedback", feedback_json);
	cJSON_AddItemToObject(root_json, "joints", joints_json);
	cJSON_AddNumberToObject(root_json, "encoder_type_flag", state->encoder_type_flag);
	cJSON_AddItemToObject(root_json, "curencodertype", curencodertype_json);
	cJSON_AddItemToObject(root_json, "tcp", tcp_json);
	cJSON_AddNumberToObject(root_json, "state", state->ctrl_query_state);
	cJSON_AddNumberToObject(root_json, "program_state", state->program_state);
	cJSON_AddNumberToObject(root_json, "flag_zero_set", state->flag_zero_set);
	cJSON_AddNumberToObject(root_json, "weldTrackSpeed", state->weldTrackSpeed);
	cJSON_AddNumberToObject(root_json, "conveyor_encoder_pos", state->conveyor_encoder_pos);
	cJSON_AddNumberToObject(root_json, "conveyor_speed", state->conveyor_speed);
	cJSON_AddNumberToObject(root_json, "btn_box_stop_signal", state->btn_box_stop_signal);
	cJSON_AddNumberToObject(root_json, "line_number", state->line_number);

	//printf("state->gripperActStatus = %d\n", state->gripperActStatus);
	memset(array, 0, sizeof(array));
	uint16_to_array(state->gripperActStatus, array);
	cJSON_AddItemToObject(root_json, "gripper_state", cJSON_CreateIntArray(array, 8));
	/*array_json = cJSON_CreateArray();
	cJSON_AddItemToObject(root_json, "gripper_state", array_json);
	for (i = 0; i < 8; i++) {
		cJSON_AddNumberToObject(array_json, "key", array[i]);
	}*/

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

	array_exAxisPos = cJSON_CreateArray();
	cJSON_AddItemToObject(root_json, "exAxisPos", array_exAxisPos);
	for (i = 0; i < 4; i++) {
		//printf("state->exaxis_status[%d].exAxisPos = %d\n", i, state->exaxis_status[i].exAxisPos);
		cJSON_AddNumberToObject(array_exAxisPos, "key", double_round(state->exaxis_status[i].exAxisPos, 3));
	}
					
	array_exAxisRDY = cJSON_CreateArray();
	cJSON_AddItemToObject(root_json, "exAxisRDY", array_exAxisRDY);
	for (i = 0; i < 4; i++) {
		cJSON_AddNumberToObject(array_exAxisRDY, "key", (double)state->exaxis_status[i].exAxisRDY);
	}

	array_exAxisINPOS = cJSON_CreateArray();
	cJSON_AddItemToObject(root_json, "exAxisINPOS", array_exAxisINPOS);
	for (i = 0; i < 4; i++) {
		cJSON_AddNumberToObject(array_exAxisINPOS, "key", (double)state->exaxis_status[i].exAxisINPOS);
	}
	cJSON_AddNumberToObject(root_json, "exAxisActiveFlag", state->exAxisActiveFlag);

	array_exAxisSpeedBack = cJSON_CreateArray();
	cJSON_AddItemToObject(root_json, "exAxisSpeedBack", array_exAxisSpeedBack);
	for (i = 0; i < 4; i++) {
		//printf("state->exaxis_status[%d].exAxisSpeedBack = %d\n", i, state->exaxis_status[i].exAxisSpeedBack);
		cJSON_AddNumberToObject(array_exAxisSpeedBack, "key", double_round(state->exaxis_status[i].exAxisSpeedBack, 3));
	}
	array_exAxisHomeStatus = cJSON_CreateArray();
	cJSON_AddItemToObject(root_json, "exAxisHomeStatus", array_exAxisHomeStatus);
	for (i = 0; i < 4; i++) {
		cJSON_AddNumberToObject(array_exAxisHomeStatus, "key", state->exaxis_status[i].exAxisHomeStatus);
	}

	array_ai = cJSON_CreateArray();
	cJSON_AddItemToObject(root_json, "ai", array_ai);
	for (i = 0; i < 6; i++) {
		//printf("state->analog_input[%d] = %d\n", i, state->analog_input[i]);
		cJSON_AddNumberToObject(array_ai, "key", double_round(1.0*state->analog_input[i]/40.95, 3));
	}

	array_ao = cJSON_CreateArray();
	cJSON_AddItemToObject(root_json, "ao", array_ao);
	for (i = 0; i < 6; i++) {
		//printf("state->analog_output[%d] = %d\n", i, state->analog_output[i]);
		cJSON_AddNumberToObject(array_ao, "key", double_round(1.0*state->analog_output[i]/40.95, 3));
	}

	cJSON_AddNumberToObject(root_json, "mode", state->robot_mode);
	cJSON_AddNumberToObject(root_json, "toolnum", state->toolNum);
	cJSON_AddNumberToObject(root_json, "workpiecenum", state->workPieceNum);
	cJSON_AddNumberToObject(root_json, "exaxisnum", state->exAxisNum);
	cJSON_AddNumberToObject(root_json, "vel_radio", double_round(state->vel_ratio, 3));
	cJSON_AddNumberToObject(root_json, "robot_type", robot_type);
	for (i = 0; i < 6; i++) {
		joint_value = double_round(state->jt_cur_pos[i], 3);
		//printf("joint_value = %.3lf\n", joint_value);
		memset(joint, 0, sizeof(joint));
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

	if (robot_type == 1) { // "1" 代表实体机器人
		sock_cmd = &socket_cmd;
		sock_file = &socket_file;
		/* cmd file status state all connect */
		if (socket_status.connect_status && socket_cmd.connect_status && socket_file.connect_status && socket_state.connect_status) {
			ret_connect_status = 1;
		} else {
			ret_connect_status = 0;
		}
	} else { // "0" 代表虚拟机器人
		sock_cmd = &socket_vir_cmd;
		sock_file = &socket_vir_file;
		/* cmd file status state all connect */
		if (socket_vir_status.connect_status && socket_vir_cmd.connect_status && socket_vir_file.connect_status) {
			ret_connect_status = 1;
		} else {
			ret_connect_status = 0;
		}
	}
	cJSON_AddNumberToObject(root_json, "cons", ret_connect_status);


	p = sock_cmd->ret_quene.front->next;
	while (p != NULL) {
		memset(content, 0, sizeof(content));
		sprintf(content, "%d", p->data.type);
		//printf("content = %s\n", content);
		//printf("p->data.msgcontent = %s\n", p->data.msgcontent);
		cJSON_AddStringToObject(feedback_json, content, p->data.msgcontent);
		/* 删除结点信息 */
		pthread_mutex_lock(&sock_cmd->ret_mute);
		dequene(&sock_cmd->ret_quene, p->data);
		pthread_mutex_unlock(&sock_cmd->ret_mute);
		p = sock_cmd->ret_quene.front->next;
	}

	p = sock_file->ret_quene.front->next;
	while (p != NULL) {
		memset(content, 0, sizeof(content));
		sprintf(content, "%d", p->data.type);
	//	printf("content = %s\n", content);
	//	printf("p->data.msgcontent = %s\n", p->data.msgcontent);
		cJSON_AddStringToObject(feedback_json, content, p->data.msgcontent);
		/* 删除结点信息 */
		pthread_mutex_lock(&sock_file->ret_mute);
		dequene(&sock_file->ret_quene, p->data);
		pthread_mutex_unlock(&sock_file->ret_mute);
		p = sock_file->ret_quene.front->next;
	}
	//printf("cJSON_Print = %s\n", cJSON_Print(feedback_json));

	//printf("sta language = %d\n", language);

	if (state->btn_box_stop_signal == 1) {
		if (language == 0) {
			cJSON_AddStringToObject(error_json, "key", "按钮盒急停已按下");
		}
		if (language == 1) {
			cJSON_AddStringToObject(error_json, "key", "The button box emergency stop has been pressed");
		}
		if (pre_state->btn_box_stop_signal != 1) {
			my_syslog("错误", "按钮盒急停已按下", cur_account.username);
			my_en_syslog("error", "The button box emergency stop has been pressed", cur_account.username);
			pre_state->btn_box_stop_signal = 1;
		}
	} else {
		pre_state->btn_box_stop_signal = 0;
	}

	if (state->strangePosFlag == 1) {
		if (language == 0) {
			cJSON_AddStringToObject(error_json, "key", "当前处于奇异位姿");
		}
		if (language == 1) {
			cJSON_AddStringToObject(error_json, "key", "It is currently in a singular position");
		}
		if (pre_state->strangePosFlag != 1) {
			my_syslog("错误", "当前处于奇异位姿", cur_account.username);
			my_en_syslog("error", "", cur_account.username);
			pre_state->strangePosFlag = 1;
		}
	} else {
		pre_state->strangePosFlag = 0;
	}

	if (state->drag_alarm == 1) {
		if (language == 0) { 
			cJSON_AddStringToObject(error_json, "key", "拖动警告, 当前处于自动模式");
		}
		if (language == 1) {
			cJSON_AddStringToObject(error_json, "key", "Drag warning, currently in automatic mode");
		}
		if (pre_state->drag_alarm != 1) {
			my_syslog("错误", "拖动警告, 当前处于自动模式", cur_account.username);
			my_en_syslog("error", "", cur_account.username);
			pre_state->drag_alarm = 1;
		}
	} else {
		pre_state->drag_alarm = 0;
	}

	if (state->aliveSlaveNumError == 1) {
		memset(content, 0, sizeof(content));
		sprintf(content, "活动从站数量错误，活动从站数量为:%d", state->aliveSlaveNumFeedback);
		memset(en_content, 0, sizeof(en_content));
		sprintf(en_content, "Number of active slave stations is wrong. Number of active slave stations is:%d", state->aliveSlaveNumFeedback);
		if (language == 0) { 
			cJSON_AddStringToObject(error_json, "key", content);
		}
		if (language == 1) {
			cJSON_AddStringToObject(error_json, "key", en_content);
		}
		if (pre_state->aliveSlaveNumError != 1) {
			my_syslog("错误", content, cur_account.username);
			my_en_syslog("error", en_content, cur_account.username);
			pre_state->aliveSlaveNumError = 1;
		}
	} else {
		pre_state->aliveSlaveNumError = 0;
	}
	switch(state->gripperFaultNum) {
		case 1:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "夹爪485超时");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "Claw 485 timed out");
			}
			if (pre_state->gripperFaultNum != 1) {
				my_syslog("错误", "夹爪485超时", cur_account.username);
				my_en_syslog("error", "Claw 485 timed out", cur_account.username);
				pre_state->gripperFaultNum = 1;
			}
			break;
		case 2:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "夹爪指令格式错误");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "Incorrect format of claw instruction");
			}
			if (pre_state->gripperFaultNum != 2) {
				my_syslog("错误", "夹爪指令格式错误", cur_account.username);
				my_en_syslog("error", "Incorrect format of claw instruction", cur_account.username);
				pre_state->gripperFaultNum = 2;
			}
			break;
		case 5:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "夹爪动作延迟，须先激活");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "Claw action delay, must be activated first");
			}
			if (pre_state->gripperFaultNum != 5) {
				my_syslog("错误", "夹爪动作延迟，须先激活", cur_account.username);
				my_en_syslog("error", "Claw action delay, must be activated first", cur_account.username);
				pre_state->gripperFaultNum = 5;
			}
			break;
		case 7:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "夹爪未激活");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "Claw not active");
			}
			if (pre_state->gripperFaultNum != 7) {
				my_syslog("错误", "夹爪未激活", cur_account.username);
				my_en_syslog("error", "Claw not active", cur_account.username);
				pre_state->gripperFaultNum = 7;
			}
			break;
		case 8:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "夹爪温度过高");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "Claw temperature is too high");
			}
			if (pre_state->gripperFaultNum != 8) {
				my_syslog("错误", "夹爪温度过高", cur_account.username);
				my_en_syslog("error", "Claw temperature is too high", cur_account.username);
				pre_state->gripperFaultNum = 8;
			}
			break;
		case 10:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "夹爪电压过低");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "Claw voltage is too low");
			}
			if (pre_state->gripperFaultNum != 10) {
				my_syslog("错误", "夹爪电压过低", cur_account.username);
				my_en_syslog("error", "Claw voltage is too low", cur_account.username);
				pre_state->gripperFaultNum = 10;
			}
			break;
		case 11:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "夹爪正在自动释放");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "Claw is releasing automatically");
			}
			if (pre_state->gripperFaultNum != 11) {
				my_syslog("错误", "夹爪正在自动释放", cur_account.username);
				my_en_syslog("error", "Claw is releasing automatically", cur_account.username);
				pre_state->gripperFaultNum = 11;
			}
			break;
		case 12:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "夹爪内部故障");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "Internal failure of clamping claw");
			}
			if (pre_state->gripperFaultNum != 12) {
				my_syslog("错误", "夹爪内部故障", cur_account.username);
				my_en_syslog("error", "Internal failure of clamping claw", cur_account.username);
				pre_state->gripperFaultNum = 12;
			}
			break;
		case 13:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "夹爪激活失败");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "Claw activation failed");
			}
			if (pre_state->gripperFaultNum != 13) {
				my_syslog("错误", "夹爪激活失败", cur_account.username);
				my_en_syslog("error", "Claw activation failed", cur_account.username);
				pre_state->gripperFaultNum = 13;
			}
			break;
		case 14:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "夹爪电流过大");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "The gripper current is too large");
			}
			if (pre_state->gripperFaultNum != 14) {
				my_syslog("错误", "夹爪电流过大", cur_account.username);
				my_en_syslog("error", "The gripper current is too large", cur_account.username);
				pre_state->gripperFaultNum = 14;
			}
			break;
		case 15:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "夹爪自动释放结束");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "Claw automatic release end");
			}
			if (pre_state->gripperFaultNum != 15) {
				my_syslog("错误", "夹爪自动释放结束", cur_account.username);
				my_en_syslog("error", "Claw automatic release end", cur_account.username);
				pre_state->gripperFaultNum = 15;
			}
			break;
		default:
			pre_state->gripperFaultNum = 0;
			break;
	}
	if (state->robot_mode == 0 && state->program_state == 4) {
		if (language == 0) { 
			cJSON_AddStringToObject(error_json, "key", "切换拖动状态失败");
		}
		if (language == 1) {
			cJSON_AddStringToObject(error_json, "key", "Failed to toggle drag state");
		}
		if (pre_state->robot_mode != 1) {
			my_syslog("错误", "切换拖动状态失败", cur_account.username);
			my_en_syslog("error", "Failed to toggle drag state", cur_account.username);
			pre_state->robot_mode = 1;
		}
	} else {
		pre_state->robot_mode = 0;
	}
	switch(state->slaveComError[0]) {
		case 1:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "控制箱从站掉线");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "Control box salve offline");
			}
			if (pre_state->slaveComError[0] != 1) {
				my_syslog("错误", "控制箱从站掉线", cur_account.username);
				my_en_syslog("error", "Control box salve offline", cur_account.username);
				pre_state->slaveComError[0] = 1;
			}
			break;
		case 2:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "控制箱从站状态与设置值不一致");
			} else {
				cJSON_AddStringToObject(error_json, "key", "Control box slave status is not consistent with the set value");
			}
			if (pre_state->slaveComError[0] != 2) {
				my_syslog("错误", "控制箱从站状态与设置值不一致", cur_account.username);
				my_en_syslog("error", "Control box slave status is not consistent with the set value", cur_account.username);
				pre_state->slaveComError[0] = 2;
			}
			break;
		case 3:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "控制箱从站未配置");
			} else {
				cJSON_AddStringToObject(error_json, "key", "Control box slave is not configured");
			}
			if (pre_state->slaveComError[0] != 3) {
				my_syslog("错误", "控制箱从站未配置", cur_account.username);
				my_en_syslog("error", "Control box slave is not configured", cur_account.username);
				pre_state->slaveComError[0] = 3;
			}
			break;
		case 4:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "控制箱从站配置错误");
			} else {
				cJSON_AddStringToObject(error_json, "key", "Control box slave configure error");
			}
			if (pre_state->slaveComError[0] != 4) {
				my_syslog("错误", "控制箱从站配置错误", cur_account.username);
				my_en_syslog("error", "Control box slave configure error", cur_account.username);
				pre_state->slaveComError[0] = 4;
			}
			break;
		case 5:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "控制箱从站初始化错误");
			} else {
				cJSON_AddStringToObject(error_json, "key", "Control box slave initialize error");
			}
			if (pre_state->slaveComError[0] != 5) {
				my_syslog("错误", "控制箱从站初始化错误", cur_account.username);
				my_en_syslog("error", "Control box slave initialize error", cur_account.username);
				pre_state->slaveComError[0] = 5;
			}
			break;
		case 6:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "控制箱从站邮箱通信初始化错误");
			} else {
				cJSON_AddStringToObject(error_json, "key", "Control box slave mailbox communication initialize error");
			}
			if (pre_state->slaveComError[0] != 6) {
				my_syslog("错误", "控制箱从站邮箱通信初始化错误", cur_account.username);
				my_en_syslog("error", "Control box slave mailbox communication initialize error", cur_account.username);
				pre_state->slaveComError[0] = 6;
			}
			break;
		default:
			pre_state->slaveComError[0] = 0;
			break;
	}
	switch(state->slaveComError[1]) {
		case 1:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "一轴从站掉线");
			} else {
				cJSON_AddStringToObject(error_json, "key", "One axis salve offline");
			}
			if (pre_state->slaveComError[1] != 1) {
				my_syslog("错误", "一轴从站掉线", cur_account.username);
				my_en_syslog("error", "One axis salve offline", cur_account.username);
				pre_state->slaveComError[1] = 1;
			}
			break;
		case 2:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "一轴从站状态与设置值不一致");
			} else {
				cJSON_AddStringToObject(error_json, "key", "One axis slave status is not consistent with the set value");
			}
			if (pre_state->slaveComError[1] != 2) {
				my_syslog("错误", "一轴从站状态与设置值不一致", cur_account.username);
				my_en_syslog("error", "One axis slave status is not consistent with the set value", cur_account.username);
				pre_state->slaveComError[1] = 2;
			}
			break;
		case 3:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "一轴从站未配置");
			} else {
				cJSON_AddStringToObject(error_json, "key", "One axis slave is not configured");
			}
			if (pre_state->slaveComError[1] != 3) {
				my_syslog("错误", "一轴从站未配置", cur_account.username);
				my_en_syslog("error", "One axis slave is not configured", cur_account.username);
				pre_state->slaveComError[1] = 3;
			}
			break;
		case 4:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "一轴从站配置错误");
			} else {
				cJSON_AddStringToObject(error_json, "key", "One axis slave configure error");
			}
			if (pre_state->slaveComError[1] != 4) {
				my_syslog("错误", "一轴从站配置错误", cur_account.username);
				my_en_syslog("error", "One axis slave configure error", cur_account.username);
				pre_state->slaveComError[1] = 4;
			}
			break;
		case 5:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "一轴从站初始化错误");
			} else {
				cJSON_AddStringToObject(error_json, "key", "One axis slave initialize error");
			}
			if (pre_state->slaveComError[1] != 5) {
				my_syslog("错误", "一轴从站初始化错误", cur_account.username);
				my_en_syslog("error", "One axis slave initialize error", cur_account.username);
				pre_state->slaveComError[1] = 5;
			}
			break;
		case 6:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "一轴从站邮箱通信初始化错误");
			} else {
				cJSON_AddStringToObject(error_json, "key", "One axis slave mailbox communication initialize error");
			}
			if (pre_state->slaveComError[1] != 6) {
				my_syslog("错误", "一轴从站邮箱通信初始化错误", cur_account.username);
				my_en_syslog("error", "One axis slave mailbox communication initialization error", cur_account.username);
				pre_state->slaveComError[1] = 6;
			}
			break;
		default:
			pre_state->slaveComError[1] = 0;
			break;
	}
	switch(state->slaveComError[2]) {
		case 1:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "二轴从站掉线");
			} else {
				cJSON_AddStringToObject(error_json, "key", "Two axis salve offline");
			}
			if (pre_state->slaveComError[2] != 1) {
				my_syslog("错误", "二轴从站掉线", cur_account.username);
				my_en_syslog("error", "Two axis salve offline", cur_account.username);
				pre_state->slaveComError[2] = 1;
			}
			break;
		case 2:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "二轴从站状态与设置值不一致");
			} else {
				cJSON_AddStringToObject(error_json, "key", "Two axis slave status is not consistent with the set value");
			}
			if (pre_state->slaveComError[2] != 2) {
				my_syslog("错误", "二轴从站状态与设置值不一致", cur_account.username);
				my_en_syslog("error", "Two axis slave status is not consistent with the set value", cur_account.username);
				pre_state->slaveComError[2] = 2;
			}
			break;
		case 3:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "二轴从站未配置");
			} else {
				cJSON_AddStringToObject(error_json, "key", "Two axis slave is not configured");
			}
			if (pre_state->slaveComError[2] != 3) {
				my_syslog("错误", "二轴从站未配置", cur_account.username);
				my_en_syslog("error", "Two axis slave is not configured", cur_account.username);
				pre_state->slaveComError[2] = 3;
			}
			break;
		case 4:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "二轴从站配置错误");
			} else {
				cJSON_AddStringToObject(error_json, "key", "Two axis slave configure error");
			}
			if (pre_state->slaveComError[2] != 4) {
				my_syslog("错误", "二轴从站配置错误", cur_account.username);
				my_en_syslog("error", "Two axis slave configure error", cur_account.username);
				pre_state->slaveComError[2] = 4;
			}
			break;
		case 5:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "二轴从站初始化错误");
			} else {
				cJSON_AddStringToObject(error_json, "key", "Two axis slave initialize error");
			}
			if (pre_state->slaveComError[2] != 5) {
				my_syslog("错误", "二轴从站初始化错误", cur_account.username);
				my_en_syslog("error", "Two axis slave initialize error", cur_account.username);
				pre_state->slaveComError[2] = 5;
			}
			break;
		case 6:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "二轴从站邮箱通信初始化错误");
			} else {
				cJSON_AddStringToObject(error_json, "key", "Two axis slave mailbox communication initialize error");
			}
			if (pre_state->slaveComError[2] != 6) {
				my_syslog("错误", "二轴从站邮箱通信初始化错误", cur_account.username);
				my_en_syslog("error", "Two axis slave mailbox communication initialize error", cur_account.username);
				pre_state->slaveComError[2] = 6;
			}
			break;
		default:
			pre_state->slaveComError[2] = 0;
			break;
	}
	switch(state->slaveComError[3]) {
		case 1:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "三轴从站掉线");
			} else {
				cJSON_AddStringToObject(error_json, "key", "Three axis salve offline");
			}
			if (pre_state->slaveComError[3] != 1) {
				my_syslog("错误", "三轴从站掉线", cur_account.username);
				my_en_syslog("error", "Three axis salve offline", cur_account.username);
				pre_state->slaveComError[3] = 1;
			}
			break;
		case 2:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "三轴从站状态与设置值不一致");
			} else {
				cJSON_AddStringToObject(error_json, "key", "Three axis slave status is not consistent with the set value");
			}
			if (pre_state->slaveComError[3] != 2) {
				my_syslog("错误", "三轴从站状态与设置值不一致", cur_account.username);
				my_en_syslog("error", "Three axis slave status is not consistent with the set value", cur_account.username);
				pre_state->slaveComError[3] = 2;
			}
			break;
		case 3:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "三轴从站未配置");
			} else {
				cJSON_AddStringToObject(error_json, "key", "Three axis slave is not configured");
			}
			if (pre_state->slaveComError[3] != 3) {
				my_syslog("错误", "三轴从站未配置", cur_account.username);
				my_en_syslog("error", "Three axis slave is not configured", cur_account.username);
				pre_state->slaveComError[3] = 3;
			}
			break;
		case 4:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "三轴从站配置错误");
			} else {
				cJSON_AddStringToObject(error_json, "key", "Three axis slave configure error");
			}
			if (pre_state->slaveComError[3] != 4) {
				my_syslog("错误", "三轴从站配置错误", cur_account.username);
				my_en_syslog("error", "Three axis slave configure error", cur_account.username);
				pre_state->slaveComError[3] = 4;
			}
			break;
		case 5:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "三轴从站初始化错误");
			} else {
				cJSON_AddStringToObject(error_json, "key", "Three axis slave initialize error");
			}
			if (pre_state->slaveComError[3] != 5) {
				my_syslog("错误", "三轴从站初始化错误", cur_account.username);
				my_en_syslog("error", "Three axis slave initialize error", cur_account.username);
				pre_state->slaveComError[3] = 5;
			}
			break;
		case 6:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "三轴从站邮箱通信初始化错误");
			} else {
				cJSON_AddStringToObject(error_json, "key", "Three axis slave mailbox communication initialize error");
			}
			if (pre_state->slaveComError[3] != 6) {
				my_syslog("错误", "三轴从站邮箱通信初始化错误", cur_account.username);
				my_en_syslog("error", "Three axis slave mailbox communication initialize error", cur_account.username);
				pre_state->slaveComError[3] = 6;
			}
			break;
		default:
			pre_state->slaveComError[3] = 0;
			break;
	}
	switch(state->slaveComError[4]) {
		case 1:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "四轴从站掉线");
			} else {
				cJSON_AddStringToObject(error_json, "key", "Four axis salve offline");
			}
			if (pre_state->slaveComError[4] != 1) {
				my_syslog("错误", "四轴从站掉线", cur_account.username);
				my_en_syslog("error", "Four axis salve offline", cur_account.username);
				pre_state->slaveComError[4] = 1;
			}
			break;
		case 2:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "四轴从站状态与设置值不一致");
			} else {
				cJSON_AddStringToObject(error_json, "key", "Four axis slave status is not consistent with the set value");
			}
			if (pre_state->slaveComError[4] != 2) {
				my_syslog("错误", "四轴从站状态与设置值不一致", cur_account.username);
				my_en_syslog("error", "Four axis slave status is not consistent with the set value", cur_account.username);
				pre_state->slaveComError[4] = 2;
			}
			break;
		case 3:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "四轴从站未配置");
			} else {
				cJSON_AddStringToObject(error_json, "key", "Four axis slave station is not configured");
			}
			if (pre_state->slaveComError[4] != 3) {
				my_syslog("错误", "四轴从站未配置", cur_account.username);
				my_en_syslog("error", "Four axis slave station is not configured", cur_account.username);
				pre_state->slaveComError[4] = 3;
			}
			break;
		case 4:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "四轴从站配置错误");
			} else {
				cJSON_AddStringToObject(error_json, "key", "Four axis slave configure error");
			}
			if (pre_state->slaveComError[4] != 4) {
				my_syslog("错误", "四轴从站配置错误", cur_account.username);
				my_en_syslog("error", "Four axis slave configure error", cur_account.username);
				pre_state->slaveComError[4] = 4;
			}
			break;
		case 5:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "四轴从站初始化错误");
			} else {
				cJSON_AddStringToObject(error_json, "key", "Four axis slave initialize error");
			}
			if (pre_state->slaveComError[4] != 5) {
				my_syslog("错误", "四轴从站初始化错误", cur_account.username);
				my_en_syslog("error", "Four axis slave initialize error", cur_account.username);
				pre_state->slaveComError[4] = 5;
			}
			break;
		case 6:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "四轴从站邮箱通信初始化错误");
			} else {
				cJSON_AddStringToObject(error_json, "key", "Four axis slave mailbox communication initialize error");
			}
			if (pre_state->slaveComError[4] != 6) {
				my_syslog("错误", "四轴从站邮箱通信初始化错误", cur_account.username);
				my_en_syslog("error", "Four axis slave mailbox communication initialize error", cur_account.username);
				pre_state->slaveComError[4] = 6;
			}
			break;
		default:
			pre_state->slaveComError[4] = 0;
			break;
	}
	switch(state->slaveComError[5]) {
		case 1:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "五轴从站掉线");
			} else {
				cJSON_AddStringToObject(error_json, "key", "Five axis salve offline");
			}
			if (pre_state->slaveComError[5] != 1) {
				my_syslog("错误", "五轴从站掉线", cur_account.username);
				my_en_syslog("error", "Five axis salve offline", cur_account.username);
				pre_state->slaveComError[5] = 1;
			}
			break;
		case 2:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "五轴从站状态与设置值不一致");
			} else {
				cJSON_AddStringToObject(error_json, "key", "Five axis slave status is not consistent with the set value");
			}
			if (pre_state->slaveComError[5] != 2) {
				my_syslog("错误", "五轴从站状态与设置值不一致", cur_account.username);
				my_en_syslog("error", "Five axis slave status is not consistent with the set value", cur_account.username);
				pre_state->slaveComError[5] = 2;
			}
			break;
		case 3:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "五轴从站未配置");
			} else {
				cJSON_AddStringToObject(error_json, "key", "Five axis slave is not configured");
			}
			if (pre_state->slaveComError[5] != 3) {
				my_syslog("错误", "五轴从站未配置", cur_account.username);
				my_en_syslog("error", "Five axis slave is not configured", cur_account.username);
				pre_state->slaveComError[5] = 3;
			}
			break;
		case 4:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "五轴从站配置错误");
			} else {
				cJSON_AddStringToObject(error_json, "key", "Five axis slave configure error");
			}
			if (pre_state->slaveComError[5] != 4) {
				my_syslog("错误", "五轴从站配置错误", cur_account.username);
				my_en_syslog("error", "Five axis slave configure error", cur_account.username);
				pre_state->slaveComError[5] = 4;
			}
			break;
		case 5:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "五轴从站初始化错误");
			} else {
				cJSON_AddStringToObject(error_json, "key", "Five axis slave initialize error");
			}
			if (pre_state->slaveComError[5] != 5) {
				my_syslog("错误", "五轴从站初始化错误", cur_account.username);
				my_en_syslog("error", "Five axis slave initialize error", cur_account.username);
				pre_state->slaveComError[5] = 5;
			}
			break;
		case 6:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "五轴从站邮箱通信初始化错误");
			} else {
				cJSON_AddStringToObject(error_json, "key", "Five axis slave mailbox communication initialize error");
			}
			if (pre_state->slaveComError[5] != 6) {
				my_syslog("错误", "五轴从站邮箱通信初始化错误", cur_account.username);
				my_en_syslog("error", "Five axis slave mailbox communication initialize error", cur_account.username);
				pre_state->slaveComError[5] = 6;
			}
			break;
		default:
			pre_state->slaveComError[5] = 0;
			break;
	}
	switch(state->slaveComError[6]) {
		case 1:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "六轴从站掉线");
			} else {
				cJSON_AddStringToObject(error_json, "key", "Six axis salve offline");
			}
			if (pre_state->slaveComError[6] != 1) {
				my_syslog("错误", "六轴从站掉线", cur_account.username);
				my_en_syslog("error", "Six axis salve offline", cur_account.username);
				pre_state->slaveComError[6] = 1;
			}
			break;
		case 2:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "六轴从站状态与设置值不一致");
			} else {
				cJSON_AddStringToObject(error_json, "key", "Six axis slave status is not consistent with the set value");
			}
			if (pre_state->slaveComError[6] != 2) {
				my_syslog("错误", "六轴从站状态与设置值不一致", cur_account.username);
				my_en_syslog("error", "Six axis slave status is not consistent with the set value", cur_account.username);
				pre_state->slaveComError[6] = 2;
			}
			break;
		case 3:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "六轴从站未配置");
			} else {
				cJSON_AddStringToObject(error_json, "key", "Six axis slave is not configured");
			}
			if (pre_state->slaveComError[6] != 3) {
				my_syslog("错误", "六轴从站未配置", cur_account.username);
				my_en_syslog("error", "Six axis slave is not configured", cur_account.username);
				pre_state->slaveComError[6] = 3;
			}
			break;
		case 4:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "六轴从站配置错误");
			} else {
				cJSON_AddStringToObject(error_json, "key", "Six axis slave configure error");
			}
			if (pre_state->slaveComError[6] != 4) {
				my_syslog("错误", "六轴从站配置错误", cur_account.username);
				my_en_syslog("error", "Six axis slave configure error", cur_account.username);
				pre_state->slaveComError[6] = 4;
			}
			break;
		case 5:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "六轴从站初始化错误");
			} else {
				cJSON_AddStringToObject(error_json, "key", "Six axis slave initialize error");
			}
			if (pre_state->slaveComError[6] != 5) {
				my_syslog("错误", "六轴从站初始化错误", cur_account.username);
				my_en_syslog("error", "Six axis slave initialize error", cur_account.username);
				pre_state->slaveComError[6] = 5;
			}
			break;
		case 6:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "六轴从站邮箱通信初始化错误");
			} else {
				cJSON_AddStringToObject(error_json, "key", "Six axis slave mailbox communication initialize error");
			}
			if (pre_state->slaveComError[6] != 6) {
				my_syslog("错误", "六轴从站邮箱通信初始化错误", cur_account.username);
				my_en_syslog("error", "Six axis slave mailbox communication initialize error", cur_account.username);
				pre_state->slaveComError[6] = 6;
			}
			break;
		default:
			pre_state->slaveComError[6] = 0;
			break;
	}
	switch(state->slaveComError[7]) {
		case 1:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "末端从站掉线");
			} else {
				cJSON_AddStringToObject(error_json, "key", "The terminal salve offline");
			}
			if (pre_state->slaveComError[7] != 1) {
				my_syslog("错误", "末端从站掉线", cur_account.username);
				my_en_syslog("error", "The terminal salve offline", cur_account.username);
				pre_state->slaveComError[7] = 1;
			}
			break;
		case 2:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "末端从站状态与设置值不一致");
			} else {
				cJSON_AddStringToObject(error_json, "key", "The terminal slave status is not consistent with the set value");
			}
			if (pre_state->slaveComError[7] != 2) {
				my_syslog("错误", "末端从站状态与设置值不一致", cur_account.username);
				my_en_syslog("error", "The terminal slave status is not consistent with the set value", cur_account.username);
				pre_state->slaveComError[7] = 2;
			}
			break;
		case 3:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "末端从站未配置");
			} else {
				cJSON_AddStringToObject(error_json, "key", "The terminal slave is not configured");
			}
			if (pre_state->slaveComError[7] != 3) {
				my_syslog("错误", "末端从站未配置", cur_account.username);
				my_en_syslog("error", "The terminal slave is not configured", cur_account.username);
				pre_state->slaveComError[7] = 3;
			}
			break;
		case 4:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "末端从站配置错误");
			} else {
				cJSON_AddStringToObject(error_json, "key", "The terminal  slave configure error");
			}
			if (pre_state->slaveComError[7] != 4) {
				my_syslog("错误", "末端从站配置错误", cur_account.username);
				my_en_syslog("error", "The terminal slave configure error", cur_account.username);
				pre_state->slaveComError[7] = 4;
			}
			break;
		case 5:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "末端从站初始化错误");
			} else {
				cJSON_AddStringToObject(error_json, "key", "The terminal  slave initialize error");
			}
			if (pre_state->slaveComError[7] != 5) {
				my_syslog("错误", "末端从站初始化错误", cur_account.username);
				my_en_syslog("error", "The terminal  slave initialize error", cur_account.username);
				pre_state->slaveComError[7] = 5;
			}
			break;
		case 6:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "末端从站邮箱通信初始化错误");
			} else {
				cJSON_AddStringToObject(error_json, "key", "The terminal  slave  mailbox communication initialize error");
			}
			if (pre_state->slaveComError[7] != 6) {
				my_syslog("错误", "末端从站邮箱通信初始化错误", cur_account.username);
				my_en_syslog("error", "The terminal  slave  mailbox communication initialize error", cur_account.username);
				pre_state->slaveComError[7] = 6;
			}
			break;
		default:
			pre_state->slaveComError[7] = 0;
			break;
	}
	//printf("state->cmdPointError = %d", state->cmdPointError);
	switch(state->cmdPointError) {
		case 1:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "关节指令点错误");
			} else {
				cJSON_AddStringToObject(error_json, "key", "Joint command point error");
			}
			if (pre_state->cmdPointError != 1) {
				my_syslog("错误", "关节指令点错误", cur_account.username);
				my_en_syslog("error", "Joint command point error", cur_account.username);
				pre_state->cmdPointError = 1;
			}
			break;
		case 2:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "直线目标点错误（包括工具不符）");
			} else {
				cJSON_AddStringToObject(error_json, "key", "Straight line target point error(including tool discrepancy)");
			}
			if (pre_state->cmdPointError != 2) {
				my_syslog("错误", "直线目标点错误（包括工具不符）", cur_account.username);
				my_en_syslog("error", "Straight line target point error(including tool discrepancy)", cur_account.username);
				pre_state->cmdPointError = 2;
			}
			break;
		case 3:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "圆弧中间点错误（包括工具不符）");
			} else {
				cJSON_AddStringToObject(error_json, "key", "Midpoint of arc error (including tool discrepancy)");
			}
			if (pre_state->cmdPointError != 3) {
				my_syslog("错误", "圆弧中间点错误（包括工具不符）", cur_account.username);
				my_en_syslog("error", "Midpoint of arc error (including tool discrepancy)", cur_account.username);
				pre_state->cmdPointError = 3;
			}
			break;
		case 4:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "圆弧目标点错误（包括工具不符）");
			} else {
				cJSON_AddStringToObject(error_json, "key", "Arc target point error (including tool discrepancy)");
			}
			if (pre_state->cmdPointError != 4) {
				my_syslog("错误", "圆弧目标点错误（包括工具不符）", cur_account.username);
				my_en_syslog("error", "Arc target point error (including tool discrepancy)", cur_account.username);
				pre_state->cmdPointError = 4;
			}
			break;
		case 5:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "TPD指令点错误");
			} else {
				cJSON_AddStringToObject(error_json, "key", "TPD point error");
			}
			if (pre_state->cmdPointError != 5) {
				my_syslog("错误", "TPD指令点错误", cur_account.username);
				my_en_syslog("error", "TPD point error", cur_account.username);
				pre_state->cmdPointError = 5;
			}
			break;
		case 6:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "TPD指令工具与当前工具不符");
			} else {
				cJSON_AddStringToObject(error_json, "key", "TPD instruction tool does not match the current tool");
			}
			if (pre_state->cmdPointError != 6) {
				my_syslog("错误", "TPD指令工具与当前工具不符", cur_account.username);
				my_en_syslog("error", "TPD instruction tool does not match the current tool", cur_account.username);
				pre_state->cmdPointError = 6;
			}
			break;
		case 7:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "TPD当前指令与下一指令起始点偏差过大");
			} else {
				cJSON_AddStringToObject(error_json, "key", "TPD the current instruction deviates too much from the starting point of the next instruction");
			}
			if (pre_state->cmdPointError != 7) {
				my_syslog("错误", "TPD当前指令与下一指令起始点偏差过大", cur_account.username);
				my_en_syslog("error", "TPD the current instruction deviates too much from the starting point of the next instruction", cur_account.username);
				pre_state->cmdPointError = 7;
			}
			break;
		case 8:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "PTP关节指令超限");
			} else {
				cJSON_AddStringToObject(error_json, "key", "PTP joint instruction out of limit");
			}
			if (pre_state->cmdPointError != 8) {
				my_syslog("错误", "PTP关节指令超限", cur_account.username);
				my_en_syslog("error", "PTP joint instruction out of limit", cur_account.username);
				pre_state->cmdPointError = 8;
			}
			break;
		case 9:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "TPD关节指令超限");
			} else {
				cJSON_AddStringToObject(error_json, "key", "TPD joint instruction out of limit");
			}
			if (pre_state->cmdPointError != 9) {
				my_syslog("错误", "TPD关节指令超限", cur_account.username);
				my_en_syslog("error", "TPD joint instruction out of limit", cur_account.username);
				pre_state->cmdPointError = 9;
			}
			break;
		case 10:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "LIN/ARC下发关节指令超限");
			} else {
				cJSON_AddStringToObject(error_json, "key", "LIN/ARC offering joint command out of limit");
			}
			if (pre_state->cmdPointError != 10) {
				my_syslog("错误", "LIN/ARC下发关节指令超限", cur_account.username);
				my_en_syslog("error", "LIN/ARC offering joint command out of limit", cur_account.username);
				pre_state->cmdPointError = 10;
			}
			break;
		case 11:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "笛卡尔空间内指令超速");
			} else {
				cJSON_AddStringToObject(error_json, "key", "Overspeed command in Cartesian space");
			}
			if (pre_state->cmdPointError != 11) {
				my_syslog("错误", "笛卡尔空间内指令超速", cur_account.username);
				my_en_syslog("error", "Overspeed command in Cartesian space", cur_account.username);
				pre_state->cmdPointError = 11;
			}
			break;
		case 12:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "关节空间内扭矩指令超限");
			} else {
				cJSON_AddStringToObject(error_json, "key", "Excessive torque command in joint space");
			}
			if (pre_state->cmdPointError != 12) {
				my_syslog("错误", "关节空间内扭矩指令超限", cur_account.username);
				my_en_syslog("error", "Excessive torque command in joint space", cur_account.username);
				pre_state->cmdPointError = 12;
			}
			break;
		case 13:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "下一指令关节配置发生变化");
			} else {
				cJSON_AddStringToObject(error_json, "key", "The next command changes the joint configuration");
			}
			if (pre_state->cmdPointError != 13) {
				my_syslog("错误", "下一指令关节配置发生变化", cur_account.username);
				my_en_syslog("error", "The next command changes the joint configuration", cur_account.username);
				pre_state->cmdPointError = 13;
			}
			break;
		case 14:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "当前指令关节配置发生变化");
			} else {
				cJSON_AddStringToObject(error_json, "key", "The current instruction joint configuration has changed");
			}
			if (pre_state->cmdPointError != 14) {
				my_syslog("错误", "当前指令关节配置发生变化", cur_account.username);
				my_en_syslog("error", "The current instruction joint configuration has changed", cur_account.username);
				pre_state->cmdPointError = 14;
			}
			break;
		case 15:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "JOG关节指令超限");
			} else {
				cJSON_AddStringToObject(error_json, "key", "JOG joint instruction out of limit");
			}
			if (pre_state->cmdPointError != 15) {
				my_syslog("错误", "JOG关节指令超限", cur_account.username);
				my_en_syslog("error", "JOG joint instruction out of limit", cur_account.username);
				pre_state->cmdPointError = 15;
			}
			break;
		case 16:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "轴1关节空间内指令速度超限");
			} else {
				cJSON_AddStringToObject(error_json, "key", "Axis 1 overrun command velocity in joint space");
			}
			if (pre_state->cmdPointError != 16) {
				my_syslog("错误", "轴1关节空间内指令速度超限", cur_account.username);
				my_en_syslog("error", "Axis 1 joint overrun command velocity in joint space", cur_account.username);
				pre_state->cmdPointError = 16;
			}
			break;
		case 17:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "轴2关节空间内指令速度超限");
			} else {
				cJSON_AddStringToObject(error_json, "key", "Axis 2 overrun command velocity in joint space");
			}
			if (pre_state->cmdPointError != 17) {
				my_syslog("错误", "轴2关节空间内指令速度超限", cur_account.username);
				my_en_syslog("error", "Axis 2 overrun command velocity in joint space", cur_account.username);
				pre_state->cmdPointError = 17;
			}
			break;
		case 18:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "轴3关节空间内指令速度超限");
			} else {
				cJSON_AddStringToObject(error_json, "key", "Axis 3 overrun command velocity in joint space");
			}
			if (pre_state->cmdPointError != 18) {
				my_syslog("错误", "轴3关节空间内指令速度超限", cur_account.username);
				my_en_syslog("error", "Axis 3 overrun command velocity in joint space", cur_account.username);
				pre_state->cmdPointError = 18;
			}
			break;
		case 19:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "轴4关节空间内指令速度超限");
			} else {
				cJSON_AddStringToObject(error_json, "key", "Axis 4 overrun command velocity in joint space");
			}
			if (pre_state->cmdPointError != 19) {
				my_syslog("错误", "轴4关节空间内指令速度超限", cur_account.username);
				my_en_syslog("error", "Axis 4 overrun command velocity in joint space", cur_account.username);
				pre_state->cmdPointError = 19;
			}
			break;
		case 20:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "轴5关节空间内指令速度超限");
			} else {
				cJSON_AddStringToObject(error_json, "key", "Axis 5 overrun command velocity in joint space");
			}
			if (pre_state->cmdPointError != 20) {
				my_syslog("错误", "轴5关节空间内指令速度超限", cur_account.username);
				my_en_syslog("error", "Axis 5 overrun command velocity in joint space", cur_account.username);
				pre_state->cmdPointError = 20;
			}
			break;
		case 21:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "轴6关节空间内指令速度超限");
			} else {
				cJSON_AddStringToObject(error_json, "key", "Axis 6 overrun command velocity in joint space");
			}
			if (pre_state->cmdPointError != 21) {
				my_syslog("错误", "轴6关节空间内指令速度超限", cur_account.username);
				my_en_syslog("error", "Axis 6 overrun command velocity in joint space", cur_account.username);
				pre_state->cmdPointError = 21;
			}
			break;
		case 22:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "内外部工具切换错误");
			} else {
				cJSON_AddStringToObject(error_json, "key", "Error switching internal and external tools");
			}
			if (pre_state->cmdPointError != 22) {
				my_syslog("错误", "内外部工具切换错误", cur_account.username);
				my_en_syslog("error", "Error switching internal and external tools", cur_account.username);
				pre_state->cmdPointError = 22;
			}
			break;
		case 23:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "焊接指令错误，起收弧间只允许LIN和ARC指令");
			} else {
				cJSON_AddStringToObject(error_json, "key", "Incorrect welding instruction. Only LIN and ARC instructions are allowed between starting and closing arcs");
			}
			if (pre_state->cmdPointError != 23) {
				my_syslog("错误", "焊接指令错误，起收弧间只允许LIN和ARC指令", cur_account.username);
				my_en_syslog("error", "Incorrect welding instruction. Only LIN and ARC instructions are allowed between starting and closing arcs", cur_account.username);
				pre_state->cmdPointError = 23;
			}
			break;
		case 24:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "摆焊参数错误");
			} else {
				cJSON_AddStringToObject(error_json, "key", "Swing welding parameter error");
			}
			if (pre_state->cmdPointError != 24) {
				my_syslog("错误", "摆焊参数错误", cur_account.username);
				my_en_syslog("error", "Swing welding parameter error", cur_account.username);
				pre_state->cmdPointError = 24;
			}
			break;
		case 25:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "圆弧指令点间距过小");
			} else {
				cJSON_AddStringToObject(error_json, "key", "The distance between arc instruction points is too small");
			}
			if (pre_state->cmdPointError != 25) {
				my_syslog("错误", "圆弧指令点间距过小", cur_account.username);
				my_en_syslog("error", "The distance between arc instruction points is too small", cur_account.username);
				pre_state->cmdPointError = 25;
			}
			break;
		case 26:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "激光传感器指令偏差过大");
			} else {
				cJSON_AddStringToObject(error_json, "key", "Laser sensor instruction deviation is too large");
			}
			if (pre_state->cmdPointError != 26) {
				my_syslog("错误", "激光传感器指令偏差过大", cur_account.username);
				my_en_syslog("error", "Laser sensor instruction deviation is too large", cur_account.username);
				pre_state->cmdPointError = 26;
			}
			break;
		case 27:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "激光传感器指令中断, 焊缝跟踪提前结束");
			} else {
				cJSON_AddStringToObject(error_json, "key", "Laser sensor instruction is interrupted, weld tracking ends prematurely");
			}
			if (pre_state->cmdPointError != 27) {
				my_syslog("错误", "激光传感器指令中断, 焊缝跟踪提前结束", cur_account.username);
				my_en_syslog("error", "Laser sensor instruction is interrupted, weld tracking ends prematurely", cur_account.username);
				pre_state->cmdPointError = 27;
			}
			break;
		case 28:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "外部轴指令速度超限");
			} else {
				cJSON_AddStringToObject(error_json, "key", "External shaft instruction speed over limit");
			}
			if (pre_state->cmdPointError != 28) {
				my_syslog("错误", "外部轴指令速度超限", cur_account.username);
				my_en_syslog("error", "External shaft instruction speed over limit", cur_account.username);
				pre_state->cmdPointError = 28;
			}
			break;
		case 29:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "传送带跟踪-起始点与参考点姿态变化过大");
			} else {
				cJSON_AddStringToObject(error_json, "key", "Belt tracking - starting point and reference point attitude change too much");
			}
			if (pre_state->cmdPointError != 29) {
				my_syslog("错误", "传送带跟踪-起始点与参考点姿态变化过大", cur_account.username);
				my_en_syslog("error", "Belt tracking - starting point and reference point attitude change too much", cur_account.username);
				pre_state->cmdPointError = 29;
			}
			break;
		default:
			pre_state->cmdPointError = 0;
			break;
	}
	switch(state->ioError) {
		case 1:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "通道错误");
			} else {
				cJSON_AddStringToObject(error_json, "key", "The channel error");
			}
			if (pre_state->ioError != 1) {
				my_syslog("错误", "通道错误", cur_account.username);
				my_en_syslog("error", "The channel error", cur_account.username);
				pre_state->ioError = 1;
			}
			break;
		case 2:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "数值错误");
			} else {
				cJSON_AddStringToObject(error_json, "key", "numerical fault");
			}
			if (pre_state->ioError != 2) {
				my_syslog("错误", "数值错误", cur_account.username);
				my_en_syslog("error", "numerical fault", cur_account.username);
				pre_state->ioError = 2;
			}
			break;
		case 3:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "WaitDI等待超时");
			} else {
				cJSON_AddStringToObject(error_json, "key", "WaitDI wait for a timeout");
			}
			if (pre_state->ioError != 3) {
				my_syslog("错误", "WaitDI等待超时", cur_account.username);
				my_en_syslog("error", "WaitDI wait for a timeout", cur_account.username);
				pre_state->ioError = 3;
			}
			break;
		case 4:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "WaitAI等待超时");
			} else {
				cJSON_AddStringToObject(error_json, "key", "WaitAI wait for a timeout");
			}
			if (pre_state->ioError != 4) {
				my_syslog("错误", "WaitAI等待超时", cur_account.username);
				my_en_syslog("error", "WaitAI wait for a timeout", cur_account.username);
				pre_state->ioError = 4;
			}
			break;
		case 5:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "WaitAxleDI等待超时");
			} else {
				cJSON_AddStringToObject(error_json, "key", "WaitAxleDI wait for a timeout");
			}
			if (pre_state->ioError != 5) {
				my_syslog("错误", "WaitAxleDI等待超时", cur_account.username);
				my_en_syslog("error", "WaitAxleDI wait for a timeout", cur_account.username);
				pre_state->ioError = 5;
			}
			break;
		case 6:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "WaitAxleAI等待超时");
			} else {
				cJSON_AddStringToObject(error_json, "key", "WaitAxleAI wait for a timeout");
			}
			if (pre_state->ioError != 6) {
				my_syslog("错误", "WaitAxleAI等待超时", cur_account.username);
				my_en_syslog("error", "WaitAxleAI wait for a timeout", cur_account.username);
				pre_state->ioError = 6;
			}
			break;
		case 7:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "通道已配置功能错误");
			} else {
				cJSON_AddStringToObject(error_json, "key", "The channel has been configured functionally wrong");
			}
			if (pre_state->ioError != 7) {
				my_syslog("错误", "通道已配置功能错误", cur_account.username);
				my_en_syslog("error", "The channel has been configured functionally wrong", cur_account.username);
				pre_state->ioError = 7;
			}
			break;
		case 8:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "起弧超时");
			} else {
				cJSON_AddStringToObject(error_json, "key", "Striking a timeout");
			}
			if (pre_state->ioError != 8) {
				my_syslog("错误", "起弧超时", cur_account.username);
				my_en_syslog("error", "Striking a timeout", cur_account.username);
				pre_state->ioError = 8;
			}
			break;
		case 9:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "收弧超时");
			} else {
				cJSON_AddStringToObject(error_json, "key", "Are-receive timeout");
			}
			if (pre_state->ioError != 9) {
				my_syslog("错误", "收弧超时", cur_account.username);
				my_en_syslog("error", "Are-receive timeout", cur_account.username);
				pre_state->ioError = 9;
			}
			break;
		case 10:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "寻位超时");
			} else {
				cJSON_AddStringToObject(error_json, "key", "Find a timeout");
			}
			if (pre_state->ioError != 10) {
				my_syslog("错误", "寻位超时", cur_account.username);
				my_en_syslog("error", "Find a timeout", cur_account.username);
				pre_state->ioError = 10;
			}
			break;
		default:
			pre_state->ioError = 0;
			break;
	}
	if (state->gripperError == 1) {
		if (language == 0) { 
			cJSON_AddStringToObject(error_json, "key", "夹爪运动超时错误");
		} else {
			cJSON_AddStringToObject(error_json, "key", "Claw movement timeout error");
		}
		if (pre_state->gripperError != 1) {
			my_syslog("错误", "夹爪运动超时错误", cur_account.username);
			my_en_syslog("error", "Claw movement timeout error", cur_account.username);
			pre_state->gripperError = 1;
		}
	} else {
		pre_state->gripperError = 0;
	}
	switch(state->fileError) {
		case 1:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "zbt配置文件版本错误");
			} else {
				cJSON_AddStringToObject(error_json, "key", "zbt incorrect configuration file version");
			}
			if (pre_state->fileError != 1) {
				my_syslog("错误", "zbt配置文件版本错误", cur_account.username);
				my_en_syslog("error", "zbt incorrect configuration file version", cur_account.username);
				pre_state->fileError = 1;
			}
			break;
		case 2:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "zbt配置文件加载失败");
			} else {
				cJSON_AddStringToObject(error_json, "key", "zbt the configuration file failed to load");
			}
			if (pre_state->fileError != 2) {
				my_syslog("错误", "zbt配置文件加载失败", cur_account.username);
				my_en_syslog("error", "zbt the configuration file failed to load", cur_account.username);
				pre_state->fileError = 2;
			}
			break;
		case 3:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "user配置文件版本错误");
			} else {
				cJSON_AddStringToObject(error_json, "key", "user incorrect configuration file version");
			}
			if (pre_state->fileError != 3) {
				my_syslog("错误", "user配置文件版本错误", cur_account.username);
				my_en_syslog("error", "user incorrect configuration file version", cur_account.username);
				pre_state->fileError = 3;
			}
			break;
		case 4:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "user配置文件加载失败");
			} else {
				cJSON_AddStringToObject(error_json, "key", "user the configuration file failed to load");
			}
			if (pre_state->fileError != 4) {
				my_syslog("错误", "user配置文件加载失败", cur_account.username);
				my_en_syslog("error", "user the configuration file failed to load", cur_account.username);
				pre_state->fileError = 4;
			}
			break;
		case 5:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "exaxis配置文件版本错误");
			} else {
				cJSON_AddStringToObject(error_json, "key", "exaxis incorrect configuration file version");
			}
			if (pre_state->fileError != 5) {
				my_syslog("错误", "exaxis配置文件版本错误", cur_account.username);
				my_en_syslog("error", "exaxis incorrect configuration file version", cur_account.username);
				pre_state->fileError = 5;
			}
			break;
		case 6:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "exaxis配置文件加载失败");
			} else {
				cJSON_AddStringToObject(error_json, "key", "exaxis the configuration file failed to load");
			}
			if (pre_state->fileError != 6) {
				my_syslog("错误", "exaxis配置文件加载失败", cur_account.username);
				my_en_syslog("error", "exaxis the configuration file failed to load", cur_account.username);
				pre_state->fileError = 6;
			}
			break;
		default:
			pre_state->fileError = 0;
			break;
	}
	//printf("state->paraError = %d\n", state->paraError);
	switch(state->paraError) {
		case 1:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "工具号超限错误");
			} else {
				cJSON_AddStringToObject(error_json, "key", "Error with tool number overrun");
			}
			if (pre_state->paraError != 1) {
				my_syslog("错误", "工具号超限错误", cur_account.username);
				my_en_syslog("error", "Error with tool number overrun", cur_account.username);
				pre_state->paraError = 1;
			}
			break;
		case 2:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "定位完成阈值错误");
			} else {
				cJSON_AddStringToObject(error_json, "key", "Error in positioning completion threshold");
			}
			if (pre_state->paraError != 2) {
				my_syslog("错误", "定位完成阈值错误", cur_account.username);
				my_en_syslog("error", "Error in positioning completion threshold", cur_account.username);
				pre_state->paraError = 2;
			}
			break;
		case 3:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "碰撞等级错误");
			} else {
				cJSON_AddStringToObject(error_json, "key", "Collision level error");
			}
			if (pre_state->paraError != 3) {
				my_syslog("错误", "碰撞等级错误", cur_account.username);
				my_en_syslog("error", "Collision level error", cur_account.username);
				pre_state->paraError = 3;
			}
			break;
		case 4:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "负载重量错误");
			} else {
				cJSON_AddStringToObject(error_json, "key", "Load weight error");
			}
			if (pre_state->paraError != 4) {
				my_syslog("错误", "负载重量错误", cur_account.username);
				my_en_syslog("error", "Load weight error", cur_account.username);
				pre_state->paraError = 4;
			}
			break;
		case 5:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "负载质心X错误");
			} else {
				cJSON_AddStringToObject(error_json, "key", "Load center of mass X error");
			}
			if (pre_state->paraError != 5) {
				my_syslog("错误", "负载质心X错误", cur_account.username);
				my_en_syslog("error", "Load center of mass X error", cur_account.username);
				pre_state->paraError = 5;
			}
			break;
		case 6:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "负载质心Y错误");
			} else {
				cJSON_AddStringToObject(error_json, "key", "Load center of mass Y error");
			}
			if (pre_state->paraError != 6) {
				my_syslog("错误", "负载质心Y错误", cur_account.username);
				my_en_syslog("error", "Load center of mass Y error", cur_account.username);
				pre_state->paraError = 6;
			}
			break;
		case 7:
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", "负载质心Z错误");
			} else {
				cJSON_AddStringToObject(error_json, "key", "Load center of mass Z error");
			}
			if (pre_state->paraError != 7) {
				my_syslog("错误", "负载质心Z错误", cur_account.username);
				my_en_syslog("error", "Load center of mass Z error", cur_account.username);
				pre_state->paraError = 7;
			}
			break;
		case 8:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "DI滤波时间错误");
			} else {
				cJSON_AddStringToObject(error_json, "key", "DI filtering time error");
			}
			if (pre_state->paraError != 8) {
				my_syslog("错误", "DI滤波时间错误", cur_account.username);
				my_en_syslog("error", "DI filtering time error", cur_account.username);
				pre_state->paraError = 8;
			}
			break;
		case 9:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "AxleDI滤波时间错误");
			} else {
				cJSON_AddStringToObject(error_json, "key", "AxleDI filtering time error");
			}
			if (pre_state->paraError != 9) {
				my_syslog("错误", "AxleDI滤波时间错误", cur_account.username);
				my_en_syslog("error", "AxleDI filtering time error", cur_account.username);
				pre_state->paraError = 9;
			}
			break;
		case 10:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "AI滤波时间错误");
			} else {
				cJSON_AddStringToObject(error_json, "key", "AI filtering time error");
			}
			if (pre_state->paraError != 10) {
				my_syslog("错误", "AI滤波时间错误", cur_account.username);
				my_en_syslog("error", "AI filtering time error", cur_account.username);
				pre_state->paraError = 10;
			}
			break;
		case 11:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "AxleAI滤波时间错误");
			} else {
				cJSON_AddStringToObject(error_json, "key", "AxleAI filtering time error");
			}
			if (pre_state->paraError != 11) {
				my_syslog("错误", "AxleAI滤波时间错误", cur_account.username);
				my_en_syslog("error", "AxleAI filtering time error", cur_account.username);
				pre_state->paraError = 11;
			}
			break;
		case 12:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "DI高低电平范围错误");
			} else {
				cJSON_AddStringToObject(error_json, "key", "DI wrong range of high and low levels");
			}
			if (pre_state->paraError != 12) {
				my_syslog("错误", "DI高低电平范围错误", cur_account.username);
				my_en_syslog("error", "DI wrong range of high and low levels", cur_account.username);
				pre_state->paraError = 12;
			}
			break;
		case 13:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "DO高低电平范围错误");
			} else {
				cJSON_AddStringToObject(error_json, "key", "DO wrong range of high and low levels");
			}
			if (pre_state->paraError != 13) {
				my_syslog("错误", "DO高低电平范围错误", cur_account.username);
				my_en_syslog("error", "DO wrong range of high and low levels", cur_account.username);
				pre_state->paraError = 13;
			}
			break;
		default:
			pre_state->paraError = 0;
			break;
	}
//printf("state->exAxisExSoftLimitError = %d\n", state->exAxisExSoftLimitError);
	switch(state->exAxisExSoftLimitError)
	{
		case 1:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "外部轴1轴超出软限位故障");
			} else {
				cJSON_AddStringToObject(error_json, "key", "External axis 1 axis out of soft limit fault");
			}
			if (pre_state->exAxisExSoftLimitError != 1) {
				my_syslog("错误", "外部轴1轴超出软限位故障", cur_account.username);
				my_en_syslog("error", "External axis 1 axis out of soft limit fault", cur_account.username);
				pre_state->exAxisExSoftLimitError = 1;
			}
			break;
		case 2:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "外部轴2轴超出软限位故障");
			} else {
				cJSON_AddStringToObject(error_json, "key", "External axis 2 axis out of soft limit fault");
			}
			if (pre_state->exAxisExSoftLimitError != 2) {
				my_syslog("错误", "外部轴2轴超出软限位故障", cur_account.username);
				my_en_syslog("error", "External axis 2 axis out of soft limit fault", cur_account.username);
				pre_state->exAxisExSoftLimitError = 2;
			}
			break;
		case 3:
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", "外部轴3轴超出软限位故障");
			} else {
				cJSON_AddStringToObject(error_json, "key", "External axis 3 axis out of soft limit fault");
			}
			if (pre_state->exAxisExSoftLimitError != 3) {
				my_syslog("错误", "外部轴3轴超出软限位故障", cur_account.username);
				my_en_syslog("error", "External axis 3 axis out of soft limit fault", cur_account.username);
				pre_state->exAxisExSoftLimitError = 3;
			}
			break;
		case 4:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "外部轴4轴超出软限位故障");
			} else {
				cJSON_AddStringToObject(error_json, "key", "External axis 4 axis out of soft limit fault");
			}
			if (pre_state->exAxisExSoftLimitError != 4) {
				my_syslog("错误", "外部轴4轴超出软限位故障", cur_account.username);
				my_en_syslog("error", "External axis 4 axis out of soft limit fault", cur_account.username);
				pre_state->exAxisExSoftLimitError = 4;
			}
			break;
		default:
			pre_state->exAxisExSoftLimitError = 0;
			break;
	}
	switch(state->alarm) {
		case 1:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "肩关节配置变化");
			} else {
				cJSON_AddStringToObject(error_json, "key", "Shoulder joint configuration changes");
			}
			if (pre_state->alarm != 1) {
				my_syslog("错误", "肩关节配置变化", cur_account.username);
				my_en_syslog("error", "Shoulder joint configuration changes", cur_account.username);
				pre_state->alarm = 1;
			}
			break;
		case 2:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "肘关节配置变化");
			} else {
				cJSON_AddStringToObject(error_json, "key", "Elbow  joint configuration changes");
			}
			if (pre_state->alarm != 2) {
				my_syslog("错误", "肘关节配置变化", cur_account.username);
				my_en_syslog("error", "Elbow  joint configuration changes", cur_account.username);
				pre_state->alarm = 2;
			}
			break;
		case 3:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "腕关节配置变化");
			} else {
				cJSON_AddStringToObject(error_json, "key", "Wrist  joint configuration changes");
			}
			if (pre_state->alarm != 3) {
				my_syslog("错误", "腕关节配置变化", cur_account.username);
				my_en_syslog("error", "Wrist  joint configuration changes", cur_account.username);
				pre_state->alarm = 3;
			}
			break;
		case 4:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "RPY初始化失败");
			} else {
				cJSON_AddStringToObject(error_json, "key", "RPY initialization failure");
			}
			if (pre_state->alarm != 4) {
				my_syslog("错误", "RPY初始化失败", cur_account.username);
				my_en_syslog("error", "RPY initialization failure", cur_account.username);
				pre_state->alarm = 4;
			}
			break;
		case 5:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "WaitDI 等待超时");
			} else {
				cJSON_AddStringToObject(error_json, "key", "WaitDI wait for a timeout");
			}
			if (pre_state->alarm != 4) {
				my_syslog("错误", " WaitDI 等待超时", cur_account.username);
				my_en_syslog("error", "WaitDI wait for a timeout", cur_account.username);
				pre_state->alarm = 4;
			}
			break;
		case 6:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "WaitAI 等待超时");
			} else {
				cJSON_AddStringToObject(error_json, "key", "WaitAI wait for a timeout");
			}
			if (pre_state->alarm != 4) {
				my_syslog("错误", "WaitAI 等待超时", cur_account.username);
				my_en_syslog("error", "WaitAI wait for a timeout", cur_account.username);
				pre_state->alarm = 4;
			}
			break;
		case 7:
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", "WaitToolDI 等待超时");
			} else {
				cJSON_AddStringToObject(error_json, "key", "WaitToolDI wait for a timeout");
			}
			if (pre_state->alarm != 4) {
				my_syslog("错误", "WaitToolDI 等待超时", cur_account.username);
				my_en_syslog("error", "WaitToolDI wait for a timeout", cur_account.username);
				pre_state->alarm = 4;
			}
			break;
		case 8:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "WaitToolAI 等待超时");
			} else {
				cJSON_AddStringToObject(error_json, "key", "WaitToolAI wait for a timeout");
			}
			if (pre_state->alarm != 4) {
				my_syslog("错误", "WaitToolAI 等待超时", cur_account.username);
				my_en_syslog("error", "WaitToolAI wait for a timeout", cur_account.username);
				pre_state->alarm = 4;
			}
			break;
		case 9:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "起弧成功 DI 未配置");
			} else {
				cJSON_AddStringToObject(error_json, "key", "Arcing success DI is not configured");
			}
			if (pre_state->alarm != 4) {
				my_syslog("错误", "起弧成功 DI 未配置", cur_account.username);
				my_en_syslog("error", "Arcing success DI is not configured", cur_account.username);
				pre_state->alarm = 4;
			}
			break;
		default:
			pre_state->alarm = 0;
			break;
	}
	if (state->dr_com_err == 1) {
		if (language == 0) { 
			cJSON_AddStringToObject(error_json, "key", "通信故障:控制器与驱动器心跳检测故障");
		} else {
			cJSON_AddStringToObject(error_json, "key", "Communication failure: controller and drive heartbeat detection failure");
		}
		if (pre_state->dr_com_err != 1) {
			my_syslog("错误", "通信故障:控制器与驱动器心跳检测故障", cur_account.username);
			my_en_syslog("error", "Communication failure: controller and drive heartbeat detection failure", cur_account.username);
			pre_state->dr_com_err = 1;
		}
	} else {
		pre_state->dr_com_err = 0;
	}
	memset(content, 0, sizeof(content));
	memset(en_content, 0, sizeof(en_content));
	switch ((int)state->dr_err) {
		case 1:
			sprintf(content, "%d轴驱动器故障, 驱动器故障代码:%d", 1, (int)state->dr_err_code);
			sprintf(en_content, "%d axis drive failure, drive failure code:%d", 1, (int)state->dr_err_code);
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", content);
			} else {
				cJSON_AddStringToObject(error_json, "key", en_content);
			}
			if (pre_state->dr_err != 1) {
				my_syslog("错误", content, cur_account.username);
				my_en_syslog("error", en_content, cur_account.username);
				pre_state->dr_err = 1;
			}
			break;
		case 2:
			sprintf(content, "%d轴驱动器故障, 驱动器故障代码:%d", 2, (int)state->dr_err_code);
			sprintf(en_content, "%d axis drive failure, drive failure code:%d", 2, (int)state->dr_err_code);
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", content);
			} else {
				cJSON_AddStringToObject(error_json, "key", en_content);
			}
			if (pre_state->dr_err != 2) {
				my_syslog("错误", content, cur_account.username);
				my_en_syslog("error", en_content, cur_account.username);
				pre_state->dr_err = 2;
			}
			break;
		case 3:
			sprintf(content, "%d轴驱动器故障, 驱动器故障代码:%d", 3, (int)state->dr_err_code);
			sprintf(en_content, "%d axis drive failure, drive failure code:%d", 3, (int)state->dr_err_code);
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", content);
			} else {
				cJSON_AddStringToObject(error_json, "key", en_content);
			}
			if (pre_state->dr_err != 3) {
				my_syslog("错误", content, cur_account.username);
				my_en_syslog("error", en_content, cur_account.username);
				pre_state->dr_err = 3;
			}
			break;
		case 4:
			sprintf(content, "%d轴驱动器故障, 驱动器故障代码:%d", 4, (int)state->dr_err_code);
			sprintf(en_content, "%d axis drive failure, drive failure code:%d", 4, (int)state->dr_err_code);
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", content);
			} else {
				cJSON_AddStringToObject(error_json, "key", en_content);
			}
			if (pre_state->dr_err != 4) {
				my_syslog("错误", content, cur_account.username);
				my_en_syslog("error", en_content, cur_account.username);
				pre_state->dr_err = 4;
			}
			break;
		case 5:
			sprintf(content, "%d轴驱动器故障, 驱动器故障代码:%d", 5, (int)state->dr_err_code);
			sprintf(en_content, "%d axis drive failure, drive failure code:%d", 5, (int)state->dr_err_code);
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", content);
			} else {
				cJSON_AddStringToObject(error_json, "key", en_content);
			}
			if (pre_state->dr_err != 5) {
				my_syslog("错误", content, cur_account.username);
				my_en_syslog("error", en_content, cur_account.username);
				pre_state->dr_err = 5;
			}
			break;
		case 6:
			sprintf(content, "%d轴驱动器故障, 驱动器故障代码:%d", 6, (int)state->dr_err_code);
			sprintf(en_content, "%d axis drive failure, drive failure code:%d", 6, (int)state->dr_err_code);
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", content);
			} else {
				cJSON_AddStringToObject(error_json, "key", en_content);
			}
			if (pre_state->dr_err != 6) {
				my_syslog("错误", content, cur_account.username);
				my_en_syslog("error", en_content, cur_account.username);
				pre_state->dr_err = 6;
			}
			break;
		default:
			pre_state->dr_err = 0;
			break;
	}
	switch ((int)state->out_sflimit_err) {
		case 1:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "1轴超出软限位故障");
			} else {
				cJSON_AddStringToObject(error_json, "key", "1 axis out of soft limit fault");
			}
			if (pre_state->out_sflimit_err != 1) {
				my_syslog("错误", "1轴超出软限位故障", cur_account.username);
				my_en_syslog("error", "1 axis out of soft limit fault", cur_account.username);
				pre_state->out_sflimit_err = 1;
			}
			break;
		case 2:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "2轴超出软限位故障");
			} else {
				cJSON_AddStringToObject(error_json, "key", "2 axis out of soft limit fault");
			}
			if (pre_state->out_sflimit_err != 2) {
				my_syslog("错误", "2轴超出软限位故障", cur_account.username);
				my_en_syslog("error", "2 axis out of soft limit fault", cur_account.username);
				pre_state->out_sflimit_err = 2;
			}
			break;
		case 3:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "3轴超出软限位故障");
			} else {
				cJSON_AddStringToObject(error_json, "key", "3 axis out of soft limit fault");
			}
			if (pre_state->out_sflimit_err != 3) {
				my_syslog("错误", "3轴超出软限位故障", cur_account.username);
				my_en_syslog("error", "3 axis out of soft limit fault", cur_account.username);
				pre_state->out_sflimit_err = 3;
			}
			break;
		case 4:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "4轴超出软限位故障");
			} else {
				cJSON_AddStringToObject(error_json, "key", "4 axis out of soft limit fault");
			}
			if (pre_state->out_sflimit_err != 4) {
				my_syslog("错误", "4轴超出软限位故障", cur_account.username);
				my_en_syslog("error", "4 axis out of soft limit fault", cur_account.username);
				pre_state->out_sflimit_err = 4;
			}
			break;
		case 5:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "5轴超出软限位故障");
			} else {
				cJSON_AddStringToObject(error_json, "key", "5 axis out of soft limit fault");
			}
			if (pre_state->out_sflimit_err != 5) {
				my_syslog("错误", "5轴超出软限位故障", cur_account.username);
				my_en_syslog("error", "5 axis out of soft limit fault", cur_account.username);
				pre_state->out_sflimit_err = 5;
			}
			break;
		case 6:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "6轴超出软限位故障");
			} else {
				cJSON_AddStringToObject(error_json, "key", "6 axis out of soft limit fault");
			}
			if (pre_state->out_sflimit_err != 6) {
				my_syslog("错误", "6轴超出软限位故障", cur_account.username);
				my_en_syslog("error", "6 axis out of soft limit fault", cur_account.username);
				pre_state->out_sflimit_err = 6;
			}
			break;
		default:
			pre_state->out_sflimit_err = 0;
			break;
	}
	switch((int)state->collision_err) {
		case 1:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "1轴碰撞故障");
			} else {
				cJSON_AddStringToObject(error_json, "key", "1 axis impact failure");
			}
			if (pre_state->collision_err != 1) {
				my_syslog("错误", "1轴碰撞故障", cur_account.username);
				my_en_syslog("error", "1 axis impact failure", cur_account.username);
				pre_state->collision_err = 1;
			}
			break;
		case 2:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "2轴碰撞故障");
			} else {
				cJSON_AddStringToObject(error_json, "key", "2 axis impact failure");
			}
			if (pre_state->collision_err != 2) {
				my_syslog("错误", "2轴碰撞故障", cur_account.username);
				my_en_syslog("error", "2 axis impact failure", cur_account.username);
				pre_state->collision_err = 2;
			}
			break;
		case 3:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "3轴碰撞故障");
			} else {
				cJSON_AddStringToObject(error_json, "key", "3 axis impact failure");
			}
			if (pre_state->collision_err != 3) {
				my_syslog("错误", "3轴碰撞故障", cur_account.username);
				my_en_syslog("error", "3 axis impact failure", cur_account.username);
				pre_state->collision_err = 3;
			}
			break;
		case 4:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "4轴碰撞故障");
			} else {
				cJSON_AddStringToObject(error_json, "key", "4 axis impact failure");
			}
			if (pre_state->collision_err != 4) {
				my_syslog("错误", "4轴碰撞故障", cur_account.username);
				my_en_syslog("error", "4 axis impact failure", cur_account.username);
				pre_state->collision_err = 4;
			}
			break;
		case 5:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "5轴碰撞故障");
			} else {
				cJSON_AddStringToObject(error_json, "key", "5 axis impact failure");
			}
			if (pre_state->collision_err != 5) {
				my_syslog("错误", "5轴碰撞故障", cur_account.username);
				my_en_syslog("error", "5 axis impact failure", cur_account.username);
				pre_state->collision_err = 5;
			}
			break;
		case 6:
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "6轴碰撞故障");
			} else {
				cJSON_AddStringToObject(error_json, "key", "6 axis impact failure");
			}
			if (pre_state->collision_err != 6) {
				my_syslog("错误", "6轴碰撞故障", cur_account.username);
				my_en_syslog("error", "6 axis impact failure", cur_account.username);
				pre_state->collision_err = 6;
			}
			break;
		default:
			pre_state->collision_err = 0;
			break;
	}
	if (state->safetydoor_alarm == 1) {
		if (language == 0) { 
			cJSON_AddStringToObject(error_json, "key", "安全门触发");
		} else {
			cJSON_AddStringToObject(error_json, "key", "Safety door trigger");
		}
		if (pre_state->safetydoor_alarm != 1) {
			my_syslog("错误", "安全门触发", cur_account.username);
			my_en_syslog("error", "Safety door trigger", cur_account.username);
			pre_state->safetydoor_alarm = 1;
		}
	} else {
		pre_state->safetydoor_alarm = 0;
	}
	if (state->weld_readystate == 0) {
		if (language == 0) { 
			cJSON_AddStringToObject(error_json, "key", "焊机未准备好");
		} else {
			cJSON_AddStringToObject(error_json, "key", "The welder is not ready");
		}
		if (pre_state->weld_readystate != 1) {
			my_syslog("错误", "焊机未准备好", cur_account.username);
			my_en_syslog("error", "The welder is not ready", cur_account.username);
			pre_state->weld_readystate = 1;
		}
	} else {
		pre_state->weld_readystate = 0;
	}
	//for (i = 0; i < 4; i++) {
	for (i = 0; i < 4; i++) {
		if ((int)state->exaxis_status[i].exAxisALM == 1) {
			memset(content, 0, sizeof(content));
			sprintf(content, "外部轴 %d 伺服报警", (i+1));
			memset(en_content, 0, sizeof(en_content));
			sprintf(en_content, "exaxis %d servo alarm", (i+1));
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", content);
			} else {
				cJSON_AddStringToObject(error_json, "key", en_content);
			}
			if (pre_state->exaxis_status[i].exAxisALM != 1) {
				my_syslog("错误", content, cur_account.username);
				my_en_syslog("error", en_content, cur_account.username);
				pre_state->exaxis_status[i].exAxisALM = 1;
			}
		} else {
			pre_state->exaxis_status[i].exAxisALM = 0;
		}
	}
	for (i = 0; i < 4; i++) {
		if ((int)state->exaxis_status[i].exAxisFLERR == 1) {
			memset(content, 0, sizeof(content));
			sprintf(content, "外部轴 %d 跟随误差过大", (i+1));
			memset(en_content, 0, sizeof(en_content));
			sprintf(en_content, "exaxis %d too much following error", (i+1));
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", content);
			} else {
				cJSON_AddStringToObject(error_json, "key", en_content);
			}
			if (pre_state->exaxis_status[i].exAxisFLERR != 1) {
				my_syslog("错误", content, cur_account.username);
				my_en_syslog("error", en_content, cur_account.username);
				pre_state->exaxis_status[i].exAxisFLERR = 1;
			}
		} else {
			pre_state->exaxis_status[i].exAxisFLERR = 0;
		}
	}
	//for (i = 0; i < 4; i++) {
	for (i = 0; i < 4; i++) {
		if ((int)state->exaxis_status[i].exAxisNLMT == 1) {
			memset(content, 0, sizeof(content));
			sprintf(content, "外部轴 %d 到负限位", (i+1));
			memset(en_content, 0, sizeof(en_content));
			sprintf(en_content, "exaxis %d to the negative limit", (i+1));
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", content);
			} else {
				cJSON_AddStringToObject(error_json, "key", en_content);
			}
			if (pre_state->exaxis_status[i].exAxisNLMT != 1) {
				my_syslog("错误", content, cur_account.username);
				my_en_syslog("error", en_content, cur_account.username);
				pre_state->exaxis_status[i].exAxisNLMT = 1;
			}
		} else {
			pre_state->exaxis_status[i].exAxisNLMT = 0;
		}
	}
	//for (i = 0; i < 4; i++) {
	for (i = 0; i < 4; i++) {
		if ((int)state->exaxis_status[i].exAxisPLMT == 1) {
			memset(content, 0, sizeof(content));
			sprintf(content, "外部轴 %d 到正限位", (i+1));
			memset(en_content, 0, sizeof(en_content));
			sprintf(en_content, "exaxis %d to the forward limit", (i+1));
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", content);
			} else {
				cJSON_AddStringToObject(error_json, "key", en_content);
			}
			if (pre_state->exaxis_status[i].exAxisPLMT != 1) {
				my_syslog("错误", content, cur_account.username);
				my_en_syslog("error", en_content, cur_account.username);
				pre_state->exaxis_status[i].exAxisPLMT = 1;
			}
		} else {
			pre_state->exaxis_status[i].exAxisPLMT = 0;
		}
	}
	//for (i = 0; i < 4; i++) {
	for (i = 0; i < 4; i++) {
		if ((int)state->exaxis_status[i].exAxisOFLIN == 1) {
			memset(content, 0, sizeof(content));
			sprintf(content, "外部轴 %d 通信超时，控制卡与控制箱板485通信超时", (i+1));
			memset(en_content, 0, sizeof(en_content));
			sprintf(en_content, "exaxis %d communication timeout, control card and control box board 485 communication timeout", (i+1));
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", content);
			} else {
				cJSON_AddStringToObject(error_json, "key", en_content);
			}
			if (pre_state->exaxis_status[i].exAxisOFLIN != 1) {
				my_syslog("错误", content, cur_account.username);
				my_en_syslog("error", en_content, cur_account.username);
				pre_state->exaxis_status[i].exAxisOFLIN = 1;
			}
		} else {
			pre_state->exaxis_status[i].exAxisOFLIN = 0;
		}
	}
	//printf("LoadIdentifyData[0]: weight = %lf\n", state->LoadIdentifyData[0]);
	//printf("LoadIdentifyData[1]: x = %lf\n", state->LoadIdentifyData[1]);
	//printf("LoadIdentifyData[2]: y = %lf\n", state->LoadIdentifyData[2]);
	//printf("LoadIdentifyData[3]: z = %lf\n", state->LoadIdentifyData[3]);
	array_indentifydata = cJSON_CreateArray();
	cJSON_AddItemToObject(root_json, "loadidentifydata", array_indentifydata);
	cJSON_AddNumberToObject(array_indentifydata, "key", double_round(state->LoadIdentifyData[0], 3));
	cJSON_AddNumberToObject(array_indentifydata, "key", double_round(state->LoadIdentifyData[1], 3));
	cJSON_AddNumberToObject(array_indentifydata, "key", double_round(state->LoadIdentifyData[2], 3));
	cJSON_AddNumberToObject(array_indentifydata, "key", double_round(state->LoadIdentifyData[3], 3));

	array_register_var = cJSON_CreateArray();
	cJSON_AddItemToObject(root_json, "register_var", array_register_var);
	cJSON_AddNumberToObject(array_register_var, "key", double_round(state->register_var[0], 3));
	cJSON_AddNumberToObject(array_register_var, "key", double_round(state->register_var[1], 3));
	cJSON_AddNumberToObject(array_register_var, "key", double_round(state->register_var[2], 3));
	cJSON_AddNumberToObject(array_register_var, "key", double_round(state->register_var[3], 3));
	cJSON_AddNumberToObject(array_register_var, "key", double_round(state->register_var[4], 3));
	cJSON_AddNumberToObject(array_register_var, "key", double_round(state->register_var[5], 3));

	if (state->motionAlarm == 1) {
		if (language == 0) { 
			cJSON_AddStringToObject(error_json, "key", "LIN 指令姿态变化过大");
		} else {
			cJSON_AddStringToObject(error_json, "key", "The LIN command posture has changed too much");
		}
		if (pre_state->motionAlarm != 1) {
			my_syslog("错误", "LIN指令姿态变化过大", cur_account.username);
			my_en_syslog("error", "The LIN command posture has changed too much", cur_account.username);
			pre_state->motionAlarm = 1;
		}
	} else {
		pre_state->motionAlarm = 0;
	}

	for (i = 0; i < 6; i++) {
		//printf("joint_value = %.3lf\n", joint_value);
		memset(curencodertype, 0, sizeof(curencodertype));
		sprintf(curencodertype, "curencodertype%d", (i+1));
		cJSON_AddNumberToObject(curencodertype_json, curencodertype, state->curEncoderType[i]);
	}

	if (state->alarm_check_emerg_stop_btn == 1) {
		if (language == 0) { 
			cJSON_AddStringToObject(error_json, "key", "通信异常,检查急停按钮是否松开");
		} else {
			cJSON_AddStringToObject(error_json, "key", "Abnormal communication, check whether the emergency stop button is loosened");
		}
		if (pre_state->alarm_check_emerg_stop_btn != 1) {
			my_syslog("错误", "通信异常,检查急停按钮是否松开", cur_account.username);
			my_en_syslog("error", "Warning: abnormal communication, check whether the emergency stop button is released", cur_account.username);
			pre_state->alarm_check_emerg_stop_btn = 1;
		}
	} else {
		pre_state->alarm_check_emerg_stop_btn = 0;
	}

	if (state->alarm_reboot_rebot == 1) {
		if (language == 0) { 
			cJSON_AddStringToObject(error_json, "key", "断电重启机器人");
		} else {
			cJSON_AddStringToObject(error_json, "key", "Power off and restart the robot");
		}
		if (pre_state->alarm_reboot_rebot != 1) {
			my_syslog("错误", "断电重启机器人", cur_account.username);
			my_en_syslog("error", "Power off and restart the robot", cur_account.username);
			pre_state->alarm_reboot_rebot = 1;
		}
	} else {
		pre_state->alarm_reboot_rebot = 0;
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

/* vardata_feedback */
static int vardata_feedback(char *ret_status)
{
	int i = 0;
	int j = 0;
	int k = 0;
	char key[10] = {0};
	char *buf = NULL;
	cJSON *root_json = NULL;
	cJSON *value_json = NULL;
	cJSON *root[4] = {0};
	//clock_t time_2, time_3, time_4, time_5, time_6;

/*	if (fb_queneempty(&fb_quene)) {
		fb_print_node_num(fb_quene);
		root_json = cJSON_CreateObject();
		value_json = cJSON_CreateObject();

		//printf("state_fb.iCount= %d\n", state_fb.icount);
		for (i = 0; i < state_fb.icount; i++) {
			//printf("state_fb.id[%d] = %d\n", i, state_fb.id[i]);
			itoa(state_fb.id[i], key, 10);
			root = cJSON_CreateArray();
			cJSON_AddItemToObject(value_json, key, root);
			for (j = 0; j < STATEFB_PERPKG_NUM; j++) {
				//cJSON_AddNumberToObject(root, "key", state_fb.var[j][i]);
				//printf("fb_quene.front fb[%d][%d] = %d\n", j, i, fb_quene.front->next->data.fb[j][i]);
				cJSON_AddNumberToObject(root, "key", fb_quene.front->next->data.fb[j][i]);
				//cJSON_AddNumberToObject(root, "key", 100);
			}
		}
		cJSON_AddItemToObject(root_json, "value", value_json);
		test_index++;
		cJSON_AddNumberToObject(root_json, "index", test_index);
		cJSON_AddNumberToObject(root_json, "overflow", state_fb.overflow);
		buf = cJSON_Print(root_json);
		//printf("send to GUI = %s\n", buf);
		strcpy(ret_status, buf);

		// delete front node
		pthread_mutex_lock(&socket_state.mute);
		fb_dequene(&fb_quene);
		pthread_mutex_unlock(&socket_state.mute);*/
	fb_print_node_num(fb_quene);
	root_json = cJSON_CreateObject();
	if (state_fb.overflow == 0) {
		if (fb_get_node_num(fb_quene) >= 10) {
			value_json = cJSON_CreateObject();
			//time_2 = clock();
			//printf("feedback time_2, %d\n", time_2);

			for (i = 0; i < state_fb.icount; i++) {
				//printf("state_fb.id[%d] = %d\n", i, state_fb.id[i]);
				itoa(state_fb.id[i], key, 10);
				root[i] = cJSON_CreateArray();
				cJSON_AddItemToObject(value_json, key, root[i]);
			}
			for(k = 0; k < 10; k++) {
				for (i = 0; i < state_fb.icount; i++) {
					//printf("state_fb.id[%d] = %d\n", i, state_fb.id[i]);
					for (j = 0; j < STATEFB_PERPKG_NUM; j++) {
						//cJSON_AddNumberToObject(root, "key", state_fb.var[j][i]);
						//printf("fb_quene.front fb[%d][%d] = %d\n", j, i, fb_quene.front->next->data.fb[j][i]);
						cJSON_AddNumberToObject(root[i], "key", fb_quene.front->next->data.fb[j][i]);
						//cJSON_AddNumberToObject(root, "key", 100);
					}
				}
				/* delete front node */
				pthread_mutex_lock(&socket_state.mute);
				fb_dequene(&fb_quene);
				pthread_mutex_unlock(&socket_state.mute);
			}
			cJSON_AddItemToObject(root_json, "value", value_json);
			test_index++;
			cJSON_AddNumberToObject(root_json, "index", test_index);
			//time_3 = clock();
			//printf("feedback time_3, %d\n", time_3);
			//cJSON_AddNumberToObject(root_json, "overflow", state_fb.overflow);
		/** quene is empty or node less then 10 */
		} else {
			cJSON_AddNumberToObject(root_json, "empty_data", 0);
		}
	} else {
		/** Node in quene is over then STATEFB_MAX */
		cJSON_AddNumberToObject(root_json, "overflow", 0);
		state_fb.overflow = 0;
	}
	//time_4 = clock();
	//printf("feedback time_4, %d\n", time_4);
	buf = cJSON_Print(root_json);
	//time_5 = clock();
	//printf("feedback time_5, %d\n", time_5);
	//printf("send to GUI = %s\n", buf);
	//printf("strlen buf = %d\n", strlen(buf));
	strcpy(ret_status, buf);
	//time_6 = clock();
	//printf("feedback time_6, %d\n", time_6);
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
	//清空 session
	myfreeSessions();

	printf("Will clear state quene!\n");
	/** clear state quene */
	pthread_mutex_lock(&socket_state.mute);
	fb_clearquene(&fb_quene);
	pthread_mutex_unlock(&socket_state.mute);
	/** send stop vardata_feedback to TaskManagement */
	socket_enquene(&socket_cmd, 231, "SetCTLStateQuery(0)", 1);
}

/* set timer */
void set_timer()
{
	struct sigevent evp;
	struct sigaction act;
	struct itimerspec it;

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

	// 添加定时器，5秒后触发
	it.it_interval.tv_sec = 0;
	it.it_interval.tv_nsec = 0;
	it.it_value.tv_sec = 5;
	it.it_value.tv_nsec = 0;
	if (timer_settime(timerid, 0, &it, 0) == -1) {
		perror("fail to timer_settime");
	}
	//printf("%d : set timer success \n", print_num);
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
	CTRL_STATE *state = NULL;
	CTRL_STATE *pre_state = NULL;

	/*calloc content*/
	//ret_status = (char *)calloc(1, sizeof(char)*1024);
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
	if (robot_type == 1) { // "1" 代表实体机器人
		state = &ctrl_state;
		pre_state = &pre_ctrl_state;
	} else { // "0" 代表虚拟机器人
		state = &vir_ctrl_state;
		pre_state = &pre_vir_ctrl_state;
	}

	cmd = command->valuestring;
	if(!strcmp(cmd, "basic")) {
		ret_status = (char *)calloc(1, 10000);
		if (ret_status == NULL) {
			perror("calloc");
			goto end;
		}
		ret = basic(ret_status, state, pre_state);
	} else if(!strcmp(cmd, "vardata_feedback")) {
		//clock_t time_0, time_1, time_10;
		//time_0 = clock();
		//printf("feedback time_0, %d\n", time_0);
		ret_status = (char *)calloc(1, 100000);
		if (ret_status == NULL) {
			perror("calloc");
			goto end;
		}
		//time_1 = clock();
		//printf("feedback time_1, %d\n", time_1);
		ret = vardata_feedback(ret_status);
		//time_10 = clock();
		//printf("feedback time_10, %d\n", time_10);
	} else if(!strcmp(cmd, "refresh")) {
		ret_status = (char *)calloc(1, 10);
		if (ret_status == NULL) {
			perror("calloc");
			goto end;
		}
		printf("refresh !\n");
		printf("Will clear state quene!\n");
		/** clear state quene */
		pthread_mutex_lock(&socket_state.mute);
		fb_clearquene(&fb_quene);
		pthread_mutex_unlock(&socket_state.mute);
		/** send stop vardata_feedback to TaskManagement */
		ret = socket_enquene(&socket_cmd, 231, "SetCTLStateQuery(0)", 1);
		//print_num++;
		//delete_timer();
		strcpy(ret_status, "refresh!");
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
	//printf("strlen(ret_status) = %d\n", strlen(ret_status));
	websSetStatus(wp, 200);
	websWriteHeaders(wp, -1, 0);
	websWriteEndHeaders(wp);
	websWrite(wp, ret_status);
	websDone(wp);
	/* free ret_status */
	if (ret_status != NULL) {
		free(ret_status);
		ret_status = NULL;
	}

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
	if (ret_status != NULL) {
		free(ret_status);
		ret_status = NULL;
	}
	return;
}

