
/********************************* Includes ***********************************/

#include    "goahead.h"
#include	"cJSON.h"
#include 	"tools.h"
#include 	"robot_socket.h"
#include	"action_sta.h"

/********************************* Defines ************************************/

timer_t timerid;
char time_now[100] = "";
int basic_index = 0;
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
extern TORQUE_SYS_STATE torque_sys_state;
extern TORQUE_SYS torquesys;
extern POINT_HOME_INFO point_home_info;
extern JIABAO_TORQUE_PRODUCTION_DATA jiabao_torque_pd_data;

extern PI_STATUS pi_status;
extern PI_PTHREAD pi_pt_status;   /** PI 状态反馈线程结构体 */
extern SOCKET_PI_INFO socket_pi_status;
extern SOCKET_PI_INFO socket_pi_cmd;
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
	double joint_value = 0;
	double tcp_value[6] = {0};
	cJSON *root_json = NULL;
	cJSON *joints_json = NULL;
	cJSON *curencodertype_json = NULL;
	cJSON *tcp_json = NULL;
	cJSON *error_json = NULL;
	cJSON *alarm_json = NULL;
	cJSON *feedback_json = NULL;
	cJSON *torquesys_json = NULL;
	cJSON *jiabao_torquesys_json = NULL;
	cJSON *leftstation_json = NULL;
	cJSON *rightstation_json = NULL;
	cJSON *array_exAxisPos = NULL;
	cJSON *array_exAxisRDY = NULL;
	cJSON *array_exAxisINPOS = NULL;
	cJSON *array_exAxisSpeedBack = NULL;
	cJSON *array_exAxisHomeStatus = NULL;
	cJSON *array_indentifydata = NULL;
	cJSON *FT_data_json = NULL;
	cJSON *array_ai = NULL;
	cJSON *array_ao = NULL;
	cJSON *vir_array_ai = NULL;
	cJSON *PI_IO_json = NULL;
	cJSON *electric_quantity = NULL;
	cJSON *switch_json = NULL;
	cJSON *axis_plus = NULL;
	cJSON *axis_minus = NULL;
	cJSON *custom = NULL;
	cJSON *var_json = NULL;
	cJSON *array_num_name = NULL;
	cJSON *array_num_value = NULL;
	cJSON *array_str_name = NULL;
	cJSON *array_str_value = NULL;
	cJSON *tl_cur_pos_base_json = NULL;
	int array[16] = {0};
	int WEB_TM_connect_status = 0;
	Qnode *p = NULL;
	SOCKET_INFO *sock_cmd = NULL;
	SOCKET_INFO *sock_file = NULL;

	//printf("state->cl_dgt_output_h = %d\n", state->cl_dgt_output_h);
	//printf("state->cl_dgt_output_l = %d\n", state->cl_dgt_output_l);

	/*for (i = 0; i < 6; i++) {
		printf("state->jt_cur_pos[%d] = %.3lf\n", i, state->jt_cur_pos[i]);
	}*/
	root_json = cJSON_CreateObject();

	/** alarm json */
	alarm_json = cJSON_CreateArray();
	cJSON_AddItemToObject(root_json, "alarm_info", alarm_json);
	/* error_json */
	error_json = cJSON_CreateArray();
	cJSON_AddItemToObject(root_json, "error_info", error_json);
	cJSON_AddNumberToObject(root_json, "robot_type", robot_type);
	/** "1" 代表实体机器人 */
	if (robot_type == 1) {
		sock_cmd = &socket_cmd;
		sock_file = &socket_file;
		/* cmd file status state all connect */
		if (socket_status.connect_status && socket_cmd.connect_status && socket_file.connect_status && socket_state.connect_status) {
			WEB_TM_connect_status = 1;
		} else {
			WEB_TM_connect_status = 0;
		}
	/** "0" 代表虚拟机器人 */
	} else {
		sock_cmd = &socket_vir_cmd;
		sock_file = &socket_vir_file;
		/* cmd file status state all connect */
		if (socket_vir_status.connect_status && socket_vir_cmd.connect_status && socket_vir_file.connect_status) {
			WEB_TM_connect_status = 1;
		} else {
			WEB_TM_connect_status = 0;
		}
	}
	cJSON_AddNumberToObject(root_json, "cons", WEB_TM_connect_status);

	cJSON_AddNumberToObject(root_json, "encoder_type_flag", state->encoder_type_flag);
	cJSON_AddNumberToObject(root_json, "state", state->ctrl_query_state);
	cJSON_AddNumberToObject(root_json, "program_state", state->program_state);
	cJSON_AddNumberToObject(root_json, "flag_zero_set", state->flag_zero_set);
	cJSON_AddNumberToObject(root_json, "weldTrackSpeed", state->weldTrackSpeed);
	cJSON_AddNumberToObject(root_json, "conveyor_encoder_pos", state->conveyor_encoder_pos);
	cJSON_AddNumberToObject(root_json, "conveyor_speed", state->conveyor_speed);
	cJSON_AddNumberToObject(root_json, "conveyorWorkPiecePos", state->conveyorWorkPiecePos);
	cJSON_AddNumberToObject(root_json, "btn_box_stop_signal", state->btn_box_stop_signal);
	cJSON_AddNumberToObject(root_json, "line_number", state->line_number);
	cJSON_AddNumberToObject(root_json, "pause_parameter", state->pause_parameter);
	cJSON_AddNumberToObject(root_json, "tpd_record_state", state->tpd_record_state);
	cJSON_AddNumberToObject(root_json, "tpd_record_scale", state->tpd_record_scale);
	cJSON_AddNumberToObject(root_json, "FT_ActStatus", state->FT_ActStatus);
	cJSON_AddNumberToObject(root_json, "pushBtnBoxState", state->pushBtnBoxState);
	cJSON_AddNumberToObject(root_json, "rbtEnableState", state->rbtEnableState);
#if local
	cJSON_AddNumberToObject(root_json, "mode", 1);
#else
	cJSON_AddNumberToObject(root_json, "mode", state->robot_mode);
#endif
	cJSON_AddNumberToObject(root_json, "toolnum", state->toolNum);
	cJSON_AddNumberToObject(root_json, "workpiecenum", state->workPieceNum);
	cJSON_AddNumberToObject(root_json, "exaxisnum", state->exAxisNum);
	cJSON_AddNumberToObject(root_json, "vel_radio", double_round(state->vel_ratio, 3));
	if (basic_index%10 == 0) {
		local_now_time(time_now);
	}
	//printf("basic_index = %d\n", basic_index);
	basic_index++;
	cJSON_AddStringToObject(root_json, "time_now", time_now);

	/* joints */
	joints_json = cJSON_CreateObject();
	cJSON_AddItemToObject(root_json, "joints", joints_json);
	for (i = 0; i < 6; i++) {
		joint_value = double_round(state->jt_cur_pos[i], 3);
		memset(joint, 0, sizeof(joint));
		sprintf(joint, "j%d", (i+1));
		cJSON_AddNumberToObject(joints_json, joint, joint_value);
	}

	/* DI, DO */
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
	/* virtual_cl_dgt_input */
	memset(array, 0, sizeof(array));
	uint16_to_array(state->virtual_cl_dgt_input[0], array);
	cJSON_AddItemToObject(root_json, "vir_cl_di", cJSON_CreateIntArray(array, 16));
	/* virtual_tl_dgt_input */
	memset(array, 0, sizeof(array));
	uint16_to_array(state->virtual_tl_dgt_input[0], array);
	cJSON_AddItemToObject(root_json, "vir_tl_di", cJSON_CreateIntArray(array, 16));
	vir_array_ai = cJSON_CreateArray();
	cJSON_AddItemToObject(root_json, "vir_ai", vir_array_ai);
	cJSON_AddNumberToObject(vir_array_ai, "key", double_round(state->virtual_cl_analog_input[0], 3));
	cJSON_AddNumberToObject(vir_array_ai, "key", double_round(state->virtual_cl_analog_input[1], 3));
	cJSON_AddNumberToObject(vir_array_ai, "key", double_round(state->virtual_tl_analog_input[0], 3));

	/* gripper_state*/
	memset(array, 0, sizeof(array));
	uint16_to_array(state->gripperActStatus, array);
	cJSON_AddItemToObject(root_json, "gripper_state", cJSON_CreateIntArray(array, 8));

	/* exaxis */
	array_exAxisPos = cJSON_CreateArray();
	cJSON_AddItemToObject(root_json, "exAxisPos", array_exAxisPos);
	for (i = 0; i < 4; i++) {
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
	array_exAxisSpeedBack = cJSON_CreateArray();
	cJSON_AddItemToObject(root_json, "exAxisSpeedBack", array_exAxisSpeedBack);
	for (i = 0; i < 4; i++) {
		cJSON_AddNumberToObject(array_exAxisSpeedBack, "key", double_round(state->exaxis_status[i].exAxisSpeedBack, 3));
	}
	array_exAxisHomeStatus = cJSON_CreateArray();
	cJSON_AddItemToObject(root_json, "exAxisHomeStatus", array_exAxisHomeStatus);
	for (i = 0; i < 4; i++) {
		cJSON_AddNumberToObject(array_exAxisHomeStatus, "key", state->exaxis_status[i].exAxisHomeStatus);
	}
	cJSON_AddNumberToObject(root_json, "exAxisActiveFlag", state->exAxisActiveFlag);

	/* tcp */
	tcp_json = cJSON_CreateObject();
	cJSON_AddItemToObject(root_json, "tcp", tcp_json);
	for (i = 0; i < 6; i++) {
		//printf("state->tl_cur_pos[%d] = %lf\n", i, state->tl_cur_pos[i]);
		tcp_value[i] = double_round(state->tl_cur_pos[i], 3);
	}
	cJSON_AddNumberToObject(tcp_json, "x", tcp_value[0]);
	cJSON_AddNumberToObject(tcp_json, "y", tcp_value[1]);
	cJSON_AddNumberToObject(tcp_json, "z", tcp_value[2]);
	cJSON_AddNumberToObject(tcp_json, "rx", tcp_value[3]);
	cJSON_AddNumberToObject(tcp_json, "ry", tcp_value[4]);
	cJSON_AddNumberToObject(tcp_json, "rz", tcp_value[5]);
	tl_cur_pos_base_json = cJSON_CreateArray();
	cJSON_AddItemToObject(root_json, "tl_cur_pos_base", tl_cur_pos_base_json);
	for (i = 0; i < 6; i++) {
		cJSON_AddNumberToObject(tl_cur_pos_base_json, "key", double_round(state->tl_cur_pos_base[i], 3));
	}

	/** Register var */
	var_json = cJSON_CreateObject();
	cJSON_AddItemToObject(root_json, "var", var_json);
	cJSON_AddNumberToObject(var_json, "num_total", state->reg_var.num_cnt);
	cJSON_AddNumberToObject(var_json, "str_total", state->reg_var.str_cnt);
	array_num_name = cJSON_CreateArray();
	array_num_value = cJSON_CreateArray();
	array_str_name = cJSON_CreateArray();
	array_str_value = cJSON_CreateArray();
	cJSON_AddItemToObject(var_json, "num_name", array_num_name);
	cJSON_AddItemToObject(var_json, "num_value", array_num_value);
	cJSON_AddItemToObject(var_json, "str_name", array_str_name);
	cJSON_AddItemToObject(var_json, "str_value", array_str_value);
	for (i = 0; i < REG_VAR_NB_MAX_NUM; i++) {
		cJSON_AddStringToObject(array_num_name, "key", state->reg_var.num_name[i]);
		cJSON_AddNumberToObject(array_num_value, "key", state->reg_var.num[i]);
	}
	for (i = 0; i < REG_VAR_STR_MAX_NUM; i++) {
		cJSON_AddStringToObject(array_str_name, "key", state->reg_var.str_name[i]);
		cJSON_AddStringToObject(array_str_value, "key", state->reg_var.str[i]);
	}

	/** curencodertype */
	curencodertype_json = cJSON_CreateObject();
	cJSON_AddItemToObject(root_json, "curencodertype", curencodertype_json);
	for (i = 0; i < 6; i++) {
		memset(curencodertype, 0, sizeof(curencodertype));
		sprintf(curencodertype, "curencodertype%d", (i+1));
		cJSON_AddNumberToObject(curencodertype_json, curencodertype, state->curEncoderType[i]);
	}

	/** loadidentifydata */
	array_indentifydata = cJSON_CreateArray();
	cJSON_AddItemToObject(root_json, "loadidentifydata", array_indentifydata);
	for (i = 0; i < 4; i++) {
		cJSON_AddNumberToObject(array_indentifydata, "key", double_round(state->LoadIdentifyData[i], 3));
	}

	/* FT_data */
	//printf("state->FT_data[0] = %lf\n", state->FT_data[0]);
	//printf("state->FT_data[5] = %lf\n", state->FT_data[5]);
	FT_data_json = cJSON_CreateArray();
	cJSON_AddItemToObject(root_json, "FT_data", FT_data_json);
	for (i = 0; i < 6; i++) {
		cJSON_AddNumberToObject(FT_data_json, "key", state->FT_data[i]);
	}

	//printf("torquesys.enable = %d\n", torquesys.enable);
	/** 扭矩管理系统状态反馈 */
	torquesys_json = cJSON_CreateObject();
	cJSON_AddItemToObject(root_json, "torque_sys_state", torquesys_json);
	if (torquesys.enable == 1) {
		cJSON_AddNumberToObject(torquesys_json, "motion_state", torque_sys_state.motion_state);
		memset(array, 0, sizeof(array));
		uint16_to_array(torque_sys_state.input_io_state, array);
		cJSON_AddItemToObject(torquesys_json, "input_io_state", cJSON_CreateIntArray(array, 4));
		memset(array, 0, sizeof(array));
		uint16_to_array(torque_sys_state.output_io_state, array);
		cJSON_AddItemToObject(torquesys_json, "output_io_state", cJSON_CreateIntArray(array, 4));
		cJSON_AddNumberToObject(torquesys_json, "lock_result", torque_sys_state.lock_result);
		cJSON_AddNumberToObject(torquesys_json, "error_code", torque_sys_state.error_code);
		cJSON_AddNumberToObject(torquesys_json, "task_runtime", torque_sys_state.task_runtime);
		cJSON_AddNumberToObject(torquesys_json, "feed_turns", double_round(torque_sys_state.feed_turns, 2));
		cJSON_AddNumberToObject(torquesys_json, "feed_rev", double_round(torque_sys_state.feed_rev, 2));
		cJSON_AddNumberToObject(torquesys_json, "feed_torque", double_round(torque_sys_state.feed_torque, 2));
		cJSON_AddNumberToObject(torquesys_json, "work_state", torque_sys_state.work_state);
		cJSON_AddNumberToObject(torquesys_json, "control_mode", torque_sys_state.control_mode);
		cJSON_AddNumberToObject(torquesys_json, "current_unit", torque_sys_state.current_unit);
	}

	//printf("jiabao_torque_pd_data.left_wk_id = %s\n", jiabao_torque_pd_data.left_wk_id);
	/** 嘉宝扭矩系统状态反馈 */
	jiabao_torquesys_json = cJSON_CreateObject();
	cJSON_AddItemToObject(root_json, "jiabao_torque_sys_state", jiabao_torquesys_json);
	//if () {
	leftstation_json = cJSON_CreateObject();
	rightstation_json = cJSON_CreateObject();
	cJSON_AddItemToObject(jiabao_torquesys_json, "left_station", leftstation_json);
	cJSON_AddItemToObject(jiabao_torquesys_json, "right_station", rightstation_json);
	cJSON_AddStringToObject(leftstation_json, "workpiece_id", jiabao_torque_pd_data.left_wk_id);
	cJSON_AddNumberToObject(leftstation_json, "product_count", jiabao_torque_pd_data.left_product_count);
	cJSON_AddNumberToObject(leftstation_json, "NG_count", jiabao_torque_pd_data.left_NG_count);
	cJSON_AddNumberToObject(leftstation_json, "work_time", jiabao_torque_pd_data.left_work_time);
	cJSON_AddStringToObject(rightstation_json, "workpiece_id", jiabao_torque_pd_data.right_wk_id);
	cJSON_AddNumberToObject(rightstation_json, "product_count", jiabao_torque_pd_data.right_product_count);
	cJSON_AddNumberToObject(rightstation_json, "NG_count", jiabao_torque_pd_data.right_NG_count);
	cJSON_AddNumberToObject(rightstation_json, "work_time", jiabao_torque_pd_data.right_work_time);
	//}

	/* pi function enable */
	PI_IO_json = cJSON_CreateObject();
	cJSON_AddItemToObject(root_json, "PI_IO", PI_IO_json);
	if (pi_pt_status.enable == 1) {
		/** PI IO 状态反馈 */
		if (socket_pi_status.connect_status == 1) {
			cJSON_AddNumberToObject(PI_IO_json, "power_supply_mode", pi_status.power_supply_mode);
			electric_quantity = cJSON_CreateArray();
			cJSON_AddItemToObject(PI_IO_json, "electric_quantity", electric_quantity);
			for (i = 0; i < 4; i++) {
				cJSON_AddNumberToObject(electric_quantity, "key", pi_status.electric_quantity[i]);
			}
			switch_json = cJSON_CreateArray();
			cJSON_AddItemToObject(PI_IO_json, "switch", switch_json);
			for (i = 0; i < 2; i++) {
				cJSON_AddNumberToObject(switch_json, "key", pi_status.key[i]);
			}
			cJSON_AddNumberToObject(PI_IO_json, "start", pi_status.start);
			cJSON_AddNumberToObject(PI_IO_json, "stop", pi_status.stop);
			axis_plus = cJSON_CreateArray();
			cJSON_AddItemToObject(PI_IO_json, "axis_plus", axis_plus);
			for (i = 0; i < 6; i++) {
				cJSON_AddNumberToObject(axis_plus, "key", pi_status.axis_plus[i]);
			}
			axis_minus = cJSON_CreateArray();
			cJSON_AddItemToObject(PI_IO_json, "axis_minus", axis_minus);
			for (i = 0; i < 6; i++) {
				cJSON_AddNumberToObject(axis_minus, "key", pi_status.axis_minus[i]);
			}
			custom = cJSON_CreateArray();
			cJSON_AddItemToObject(PI_IO_json, "custom", custom);
			for (i = 0; i < 4; i++) {
				cJSON_AddNumberToObject(custom, "key", pi_status.custom[i]);
			}
		}
#if print_mode
		printf("socket_pi_status.connect_status = %d\n", socket_pi_status.connect_status);
		printf("socket_pi_cmd.connect_status = %d\n", socket_pi_cmd.connect_status);
#endif
		/* PI socket connect error */
		if (socket_pi_status.connect_status == 0 || socket_pi_cmd.connect_status == 0) {
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", "WebAPP 与示教器（树莓派）通信失败");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "WebAPP failed to communicate with the teaching device (Raspberry PI)");
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", "WebAPPがシエナ(ラズベリーパイ)と通信に失敗");
			}
			if (socket_pi_status.pre_connect_status != 0 || socket_pi_cmd.pre_connect_status != 0) {
				my_syslog("错误", "WebAPP 与示教器（树莓派）通信失败", cur_account.username);
				my_en_syslog("error", "WebAPP failed to communicate with the teaching device (Raspberry PI)", cur_account.username);
				my_jap_syslog("さくご", "WebAPPがシエナ(ラズベリーパイ)と通信に失敗", cur_account.username);
				socket_pi_status.pre_connect_status = 0;
				socket_pi_cmd.pre_connect_status = 0;
			}
		} else {
			socket_pi_status.pre_connect_status = 1;
			socket_pi_cmd.pre_connect_status = 1;
		}
	}

	/** feedback_json */
	feedback_json = cJSON_CreateObject();
	cJSON_AddItemToObject(root_json, "set_feedback", feedback_json);
	/* 如果 web 与控制器通信正常 */
	if (WEB_TM_connect_status == 1) {
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
	}

	/* 如果 web 与控制器通信正常 */
	if (WEB_TM_connect_status == 1) {
		get_robot_alarm_error_info(alarm_json, error_json);
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
			sprintf(key, "%d", state_fb.id[i]);
			//itoa(state_fb.id[i], key, 10);
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
	//fb_print_node_num(fb_quene);
	root_json = cJSON_CreateObject();
	if (state_fb.overflow == 0) {
		if (fb_get_node_num(fb_quene) >= 10) {
			value_json = cJSON_CreateObject();
			//time_2 = clock();
			//printf("feedback time_2, %d\n", time_2);

			for (i = 0; i < state_fb.icount; i++) {
				//printf("state_fb.id[%d] = %d\n", i, state_fb.id[i]);
				//itoa(state_fb.id[i], key, 10);
				sprintf(key, "%d", state_fb.id[i]);
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
	CTRL_STATE *state = NULL;

	if (robot_type == 1) { // "1" 代表实体机器人
		state = &ctrl_state;
	} else { // "0" 代表虚拟机器人
		state = &vir_ctrl_state;
	}
	//printf("enter timer_signal function! %d\n", signo);
	printf("Will free Sessions!\n");
	//清空 session
	myfreeSessions();

	if (state->ctrl_query_state == 1) {
		printf("Will clear state quene!\n");
		/** clear state quene */
		pthread_mutex_lock(&socket_state.mute);
		fb_clearquene(&fb_quene);
		pthread_mutex_unlock(&socket_state.mute);
		/** send stop vardata_feedback to TaskManagement */
		socket_enquene(&socket_cmd, 231, "SetCTLStateQuery(0)", 1);
	}
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
		strcpy(ret_status, "refresh!");

		if (state->ctrl_query_state == 1) {
			printf("Will clear state quene!\n");
			/** clear state quene */
			pthread_mutex_lock(&socket_state.mute);
			fb_clearquene(&fb_quene);
			pthread_mutex_unlock(&socket_state.mute);
			/** send stop vardata_feedback to TaskManagement */
			ret = socket_enquene(&socket_cmd, 231, "SetCTLStateQuery(0)", 1);
			//print_num++;
			//delete_timer();
		}
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
	return;
}

