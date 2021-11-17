
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
	char en_content[MAX_BUF] = {0};
	char jap_content[MAX_BUF] = {0};
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

	/** alarm json */
	alarm_json = cJSON_CreateArray();
	cJSON_AddItemToObject(root_json, "alarm_info", alarm_json);
	/* 如果 web 与控制器通信正常 */
	if (WEB_TM_connect_status == 1) {
		if (state->tpd_num_limit == 1) {
			if (language == 0) {
				cJSON_AddStringToObject(alarm_json, "key", "TPD轨迹加载数量超限");
			}
			if (language == 1) {
				cJSON_AddStringToObject(alarm_json, "key", "The number of TPD tracks loaded exceeded the upper limit");
			}
			if (language == 2) {
				cJSON_AddStringToObject(alarm_json, "key", "TPDトラックロード数オーバーラン");
			}
			if (pre_state->tpd_num_limit != 1) {
				my_syslog("警告", "TPD轨迹加载数量超限", cur_account.username);
				my_en_syslog("alarm", "The number of TPD tracks loaded exceeded the upper limit", cur_account.username);
				my_jap_syslog("戒告する", "TPDトラックロード数オーバーラン", cur_account.username);
				pre_state->tpd_num_limit = 1;
			}
		} else {
			pre_state->tpd_num_limit = 0;
		}
		if (state->reg_var.num_full == 1) {
			if (language == 0) {
				cJSON_AddStringToObject(alarm_json, "key", "数值型变量监控个数已满");
			}
			if (language == 1) {
				cJSON_AddStringToObject(alarm_json, "key", "The number of numeric variables monitored was full. Procedure");
			}
			if (language == 2) {
				cJSON_AddStringToObject(alarm_json, "key", "数値型変数のモニタリング数は満杯");
			}
			if (pre_state->reg_var.num_full != 1) {
				my_syslog("警告", "数值型变量监控个数已满", cur_account.username);
				my_en_syslog("alarm", "The number of numeric variables monitored was full. Procedure", cur_account.username);
				my_jap_syslog("戒告する", "数値型変数のモニタリング数は満杯", cur_account.username);
				pre_state->reg_var.num_full = 1;
			}
		} else {
			pre_state->reg_var.num_full = 0;
		}
		if (state->reg_var.str_full == 1) {
			if (language == 0) {
				cJSON_AddStringToObject(alarm_json, "key", "字符型变量监控个数已满");
			}
			if (language == 1) {
				cJSON_AddStringToObject(alarm_json, "key", "The number of character variables monitored was full. Procedure");
			}
			if (language == 2) {
				cJSON_AddStringToObject(alarm_json, "key", "文字型変数の監視個数が満杯");
			}
			if (pre_state->reg_var.str_full != 1) {
				my_syslog("警告", "字符型变量监控个数已满", cur_account.username);
				my_en_syslog("alarm", "The number of character variables monitored was full. Procedure", cur_account.username);
				my_jap_syslog("戒告する", "文字型変数の監視個数が満杯", cur_account.username);
				pre_state->reg_var.str_full = 1;
			}
		} else {
			pre_state->reg_var.str_full = 0;
		}
		if (state->socket_conn_timeout >= 1) {
			memset(content, 0, sizeof(content));
			memset(en_content, 0, sizeof(en_content));
			memset(jap_content, 0, sizeof(jap_content));
			sprintf(content, "socket_%d: 连接超时", (state->socket_conn_timeout - 1));
			sprintf(en_content, "socket_%d: connection timeout", (state->socket_conn_timeout - 1));
			sprintf(jap_content, "ソケット%d: 接続タイムアウト", (state->socket_conn_timeout - 1));
			if (language == 0) {
				cJSON_AddStringToObject(alarm_json, "key", content);
			}
			if (language == 1) {
				cJSON_AddStringToObject(alarm_json, "key", en_content);
			}
			if (language == 2) {
				cJSON_AddStringToObject(alarm_json, "key", jap_content);
			}
			if (pre_state->socket_conn_timeout != 1) {
				my_syslog("警告", content, cur_account.username);
				my_en_syslog("alarm", en_content, cur_account.username);
				my_jap_syslog("戒告する", jap_content, cur_account.username);
				pre_state->socket_conn_timeout = 1;
			}
		} else {
			pre_state->socket_conn_timeout = 0;
		}
		if (state->socket_read_timeout >= 1) {
			memset(content, 0, sizeof(content));
			memset(en_content, 0, sizeof(en_content));
			memset(jap_content, 0, sizeof(jap_content));
			sprintf(content, "socket_%d: 读取超时", (state->socket_conn_timeout - 1));
			sprintf(en_content, "socket_%d: read timeout", (state->socket_conn_timeout - 1));
			sprintf(jap_content, "ソケット%d: 読み取りタイムアウト", (state->socket_conn_timeout - 1));
			if (language == 0) {
				cJSON_AddStringToObject(alarm_json, "key", content);
			}
			if (language == 1) {
				cJSON_AddStringToObject(alarm_json, "key", en_content);
			}
			if (language == 2) {
				cJSON_AddStringToObject(alarm_json, "key", jap_content);
			}
			if (pre_state->socket_read_timeout != 1) {
				my_syslog("警告", content, cur_account.username);
				my_en_syslog("alarm", en_content, cur_account.username);
				my_jap_syslog("戒告する", jap_content, cur_account.username);
				pre_state->socket_read_timeout = 1;
			}
		} else {
			pre_state->socket_read_timeout = 0;
		}
		if (state->btn_box_stop_signal == 1) {
			if (language == 0) {
				cJSON_AddStringToObject(alarm_json, "key", "按钮盒急停已按下");
			}
			if (language == 1) {
				cJSON_AddStringToObject(alarm_json, "key", "The button box emergency stop has been pressed");
			}
			if (language == 2) {
				cJSON_AddStringToObject(alarm_json, "key", "ボタンボックスが押されました");
			}
			if (pre_state->btn_box_stop_signal != 1) {
				my_syslog("警告", "按钮盒急停已按下", cur_account.username);
				my_en_syslog("alarm", "The button box emergency stop has been pressed", cur_account.username);
				my_jap_syslog("戒告する", "ボタンボックスが押されました", cur_account.username);
				pre_state->btn_box_stop_signal = 1;
			}
		} else {
			pre_state->btn_box_stop_signal = 0;
		}
		if (state->strangePosFlag == 1) {
			if (language == 0) {
				cJSON_AddStringToObject(alarm_json, "key", "当前处于奇异位姿");
			}
			if (language == 1) {
				cJSON_AddStringToObject(alarm_json, "key", "It is currently in a singular position");
			}
			if (language == 2) {
				cJSON_AddStringToObject(alarm_json, "key", "今は不思議な姿をしている");
			}
			if (pre_state->strangePosFlag != 1) {
				my_syslog("警告", "当前处于奇异位姿", cur_account.username);
				my_en_syslog("alarm", "It is currently in a singular position", cur_account.username);
				my_jap_syslog("戒告する", "今は不思議な姿をしている", cur_account.username);
				pre_state->strangePosFlag = 1;
			}
		} else {
			pre_state->strangePosFlag = 0;
		}
		if (state->drag_alarm == 1) {
			if (language == 0) { 
				cJSON_AddStringToObject(alarm_json, "key", "拖动警告, 当前处于自动模式");
			}
			if (language == 1) {
				cJSON_AddStringToObject(alarm_json, "key", "Drag warning, currently in automatic mode");
			}
			if (language == 2) {
				cJSON_AddStringToObject(alarm_json, "key", "警告をドラッグすると、自動モードになります");
			}
			if (pre_state->drag_alarm != 1) {
				my_syslog("警告", "拖动警告, 当前处于自动模式", cur_account.username);
				my_en_syslog("alarm", "Drag warning, currently in automatic mode", cur_account.username);
				my_jap_syslog("戒告する", "警告をドラッグすると、自動モードになります", cur_account.username);
				pre_state->drag_alarm = 1;
			}
		} else {
			pre_state->drag_alarm = 0;
		}
		if (state->robot_mode == 0 && state->program_state == 4) {
			if (language == 0) { 
				cJSON_AddStringToObject(alarm_json, "key", "切换拖动状态失败");
			}
			if (language == 1) {
				cJSON_AddStringToObject(alarm_json, "key", "Failed to toggle drag state");
			}
			if (language == 2) {
				cJSON_AddStringToObject(alarm_json, "key", "ドラッグ状態の切り替えに失敗しました");
			}
			if (pre_state->robot_mode != 1) {
				my_syslog("警告", "切换拖动状态失败", cur_account.username);
				my_en_syslog("alarm", "Failed to toggle drag state", cur_account.username);
				my_jap_syslog("戒告する", "ドラッグ状態の切り替えに失敗しました", cur_account.username);
				pre_state->robot_mode = 1;
			}
		} else {
			pre_state->robot_mode = 0;
		}
		switch(state->alarm) {
			case 1:
				if (language == 0) { 
					cJSON_AddStringToObject(alarm_json, "key", "肩关节配置变化");
				}
				if (language == 1) {
					cJSON_AddStringToObject(alarm_json, "key", "Shoulder joint configuration changes");
				}
				if (language == 2) {
					cJSON_AddStringToObject(alarm_json, "key", "肩関節の配置変化");
				}
				if (pre_state->alarm != 1) {
					my_syslog("警告", "肩关节配置变化", cur_account.username);
					my_en_syslog("alarm", "Shoulder joint configuration changes", cur_account.username);
					my_jap_syslog("戒告する", "肩関節の配置変化", cur_account.username);
					pre_state->alarm = 1;
				}
				break;
			case 2:
				if (language == 0) { 
					cJSON_AddStringToObject(alarm_json, "key", "肘关节配置变化");
				}
				if (language == 1) {
					cJSON_AddStringToObject(alarm_json, "key", "Elbow  joint configuration changes");
				}
				if (language == 2) {
					cJSON_AddStringToObject(alarm_json, "key", "肘関節の配置変化");
				}
				if (pre_state->alarm != 2) {
					my_syslog("警告", "肘关节配置变化", cur_account.username);
					my_en_syslog("alarm", "Elbow  joint configuration changes", cur_account.username);
					my_jap_syslog("戒告する", "肘関節の配置変化", cur_account.username);
					pre_state->alarm = 2;
				}
				break;
			case 3:
				if (language == 0) { 
					cJSON_AddStringToObject(alarm_json, "key", "腕关节配置变化");
				}
				if (language == 1) {
					cJSON_AddStringToObject(alarm_json, "key", "Wrist  joint configuration changes");
				}
				if (language == 2) {
					cJSON_AddStringToObject(alarm_json, "key", "腕関節の配置変化");
				}
				if (pre_state->alarm != 3) {
					my_syslog("警告", "腕关节配置变化", cur_account.username);
					my_en_syslog("alarm", "Wrist  joint configuration changes", cur_account.username);
					my_jap_syslog("戒告する", "腕関節の配置変化", cur_account.username);
					pre_state->alarm = 3;
				}
				break;
			case 4:
				if (language == 0) { 
					cJSON_AddStringToObject(alarm_json, "key", "RPY初始化失败");
				}
				if (language == 1) {
					cJSON_AddStringToObject(alarm_json, "key", "RPY initialization failure");
				}
				if (language == 2) {
					cJSON_AddStringToObject(alarm_json, "key", "rpyの初期化に失敗する");
				}
				if (pre_state->alarm != 4) {
					my_syslog("警告", "RPY初始化失败", cur_account.username);
					my_en_syslog("alarm", "RPY initialization failure", cur_account.username);
					my_jap_syslog("戒告する", "rpyの初期化に失敗する", cur_account.username);
					pre_state->alarm = 4;
				}
				break;
			case 5:
				if (language == 0) { 
					cJSON_AddStringToObject(alarm_json, "key", "警告: WaitDI 等待超时");
				}
				if (language == 1) {
					cJSON_AddStringToObject(alarm_json, "key", "Warning: WaitDI wait for a timeout");
				}
				if (language == 2) {
					cJSON_AddStringToObject(alarm_json, "key", "戒告する: waitdiタイムアウトを待つ");
				}
				if (pre_state->alarm != 4) {
					my_syslog("警告", "警告: WaitDI 等待超时", cur_account.username);
					my_en_syslog("alarm", "Warning: WaitDI wait for a timeout", cur_account.username);
					my_jap_syslog("戒告する", "戒告する: waitdiタイムアウトを待つ", cur_account.username);
					pre_state->alarm = 4;
				}
				break;
			case 6:
				if (language == 0) { 
					cJSON_AddStringToObject(alarm_json, "key", "警告: WaitAI 等待超时");
				}
				if (language == 1) {
					cJSON_AddStringToObject(alarm_json, "key", "Warning: WaitAI wait for a timeout");
				}
				if (language == 2) {
					cJSON_AddStringToObject(alarm_json, "key", "戒告する: waitaiタイムアウト待ち");
				}
				if (pre_state->alarm != 4) {
					my_syslog("警告", "警告: WaitAI 等待超时", cur_account.username);
					my_en_syslog("alarm", "Warning: WaitAI wait for a timeout", cur_account.username);
					my_jap_syslog("戒告する", "戒告する: waitaiタイムアウト待ち", cur_account.username);
					pre_state->alarm = 4;
				}
				break;
			case 7:
				if (language == 0) {
					cJSON_AddStringToObject(alarm_json, "key", "警告: WaitToolDI 等待超时");
				}
				if (language == 1) {
					cJSON_AddStringToObject(alarm_json, "key", "Warning: WaitToolDI wait for a timeout");
				}
				if (language == 2) {
					cJSON_AddStringToObject(alarm_json, "key", "戒告する: waittooldiタイムアウトを待つ");
				}
				if (pre_state->alarm != 4) {
					my_syslog("警告", "警告: WaitToolDI 等待超时", cur_account.username);
					my_en_syslog("alarm", "Warning: WaitToolDI wait for a timeout", cur_account.username);
					my_jap_syslog("戒告する", "戒告する: waittooldiタイムアウトを待つ", cur_account.username);
					pre_state->alarm = 4;
				}
				break;
			case 8:
				if (language == 0) {
					cJSON_AddStringToObject(alarm_json, "key", "警告: WaitToolAI 等待超时");
				}
				if (language == 1) {
					cJSON_AddStringToObject(alarm_json, "key", "Warning: WaitToolAI wait for a timeout");
				}
				if (language == 2) {
					cJSON_AddStringToObject(alarm_json, "key", "戒告する: waittoolaiタイムアウトを待つ");
				}
				if (pre_state->alarm != 4) {
					my_syslog("警告", "警告: WaitToolAI 等待超时", cur_account.username);
					my_en_syslog("alarm", "Warning: WaitToolAI wait for a timeout", cur_account.username);
					my_jap_syslog("戒告する", "戒告する: waittoolaiタイムアウトを待つ", cur_account.username);
					pre_state->alarm = 4;
				}
				break;
			case 9:
				if (language == 0) {
					cJSON_AddStringToObject(alarm_json, "key", "警告: 起弧成功 DI 未配置");
				}
				if (language == 1) {
					cJSON_AddStringToObject(alarm_json, "key", "Warning: Arcing success DI is not configured");
				}
				if (language == 2) {
					cJSON_AddStringToObject(alarm_json, "key", "戒告する: 起弧成功di未配置");
				}
				if (pre_state->alarm != 4) {
					my_syslog("警告", "警告: 起弧成功 DI 未配置", cur_account.username);
					my_en_syslog("alarm", "Warning: Arcing success DI is not configured", cur_account.username);
					my_jap_syslog("戒告する", "戒告する: 起弧成功di未配置", cur_account.username);
					pre_state->alarm = 4;
				}
				break;
			default:
				pre_state->alarm = 0;
				break;
		}
		if (state->safetydoor_alarm == 1) {
			if (language == 0) { 
				cJSON_AddStringToObject(alarm_json, "key", "安全门触发");
			}
			if (language == 1) {
				cJSON_AddStringToObject(alarm_json, "key", "Safety door trigger");
			}
			if (language == 2) {
				cJSON_AddStringToObject(alarm_json, "key", "安全扉トリガ");
			}
			if (pre_state->safetydoor_alarm != 1) {
				my_syslog("警告", "安全门触发", cur_account.username);
				my_en_syslog("alarm", "Safety door trigger", cur_account.username);
				my_jap_syslog("戒告する", "安全扉トリガ", cur_account.username);
				pre_state->safetydoor_alarm = 1;
			}
		} else {
			pre_state->safetydoor_alarm = 0;
		}
		if (state->motionAlarm == 1) {
			if (language == 0) { 
				cJSON_AddStringToObject(alarm_json, "key", "警告： LIN 指令姿态变化过大");
			}
			if (language == 1) {
				cJSON_AddStringToObject(alarm_json, "key", "Warning: The LIN command posture has changed too much");
			}
			if (language == 2) {
				cJSON_AddStringToObject(alarm_json, "key", "戒告する: linコマンドの姿勢変化が大きすぎる");
			}
			if (pre_state->motionAlarm != 1) {
				my_syslog("警告", "警告： LIN指令姿态变化过大", cur_account.username);
				my_en_syslog("alarm", "Warning: The LIN command posture has changed too much", cur_account.username);
				my_jap_syslog("戒告する", "戒告する: linコマンドの姿勢変化が大きすぎる", cur_account.username);
				pre_state->motionAlarm = 1;
			}
		} else {
			pre_state->motionAlarm = 0;
		}
		if (state->interfereAlarm == 1) {
			if (language == 0) {
				cJSON_AddStringToObject(alarm_json, "key", "警告：进入干涉区");
			}
			if (language == 1) {
				cJSON_AddStringToObject(alarm_json, "key", "Warning: Entering interference zone");
			}
			if (language == 2) {
				cJSON_AddStringToObject(alarm_json, "key", "警告:干渉領域に入る");
			}
			if (pre_state->interfereAlarm != 1) {
				my_syslog("警告", "警告：进入干涉区", cur_account.username);
				my_en_syslog("alarm", "Warning: Entering interference zone", cur_account.username);
				my_jap_syslog("戒告する", "警告:干渉領域に入る", cur_account.username);
				pre_state->interfereAlarm = 1;
			}
		} else {
			pre_state->interfereAlarm = 0;
		}
	}

	/* error_json */
	error_json = cJSON_CreateArray();
	cJSON_AddItemToObject(root_json, "error_info", error_json);
	/* 如果 web 与控制器通信正常 */
	if (WEB_TM_connect_status == 1) {
		if (point_home_info.error_flag == 1) {
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", "原点已发生改变，需要重新设置原点");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "The origin point has changed and needs to reset origin point");
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", "原点が変わったので、原点をリセットする必要があります");
			}
			if (point_home_info.pre_error_flag != 1) {
				my_syslog("错误", "原点已发生改变，需要重新设置原点", cur_account.username);
				my_en_syslog("error", "The origin point has changed and needs to reset origin point", cur_account.username);
				my_jap_syslog("さくご", "原点が変わったので、原点をリセットする必要があります", cur_account.username);
				point_home_info.pre_error_flag = 1;
			}
		} else {
			point_home_info.pre_error_flag = 0;
		}
		if (state->aliveSlaveNumError == 1) {
			memset(content, 0, sizeof(content));
			memset(en_content, 0, sizeof(en_content));
			memset(jap_content, 0, sizeof(jap_content));
			sprintf(content, "活动从站数量错误，活动从站数量为:%d", state->aliveSlaveNumFeedback);
			sprintf(en_content, "Number of active slave stations is wrong. Number of active slave stations is:%d", state->aliveSlaveNumFeedback);
			sprintf(jap_content, "キャンペーンスレーブは誤作動し、キャンペーンスレーブは次のようになる。", state->aliveSlaveNumFeedback);
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", content);
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", en_content);
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", jap_content);
			}
			if (pre_state->aliveSlaveNumError != 1) {
				my_syslog("错误", content, cur_account.username);
				my_en_syslog("error", en_content, cur_account.username);
				my_jap_syslog("さくご", jap_content, cur_account.username);
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
				if (language == 2) {
					cJSON_AddStringToObject(error_json, "key", "はさみ爪485タイムアウト");
				}
				if (pre_state->gripperFaultNum != 1) {
					my_syslog("错误", "夹爪485超时", cur_account.username);
					my_en_syslog("error", "Claw 485 timed out", cur_account.username);
					my_jap_syslog("さくご", "はさみ爪485タイムアウト", cur_account.username);
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
				if (language == 2) {
					cJSON_AddStringToObject(error_json, "key", "クリップコマンドのフォーマットが間違っています");
				}
				if (pre_state->gripperFaultNum != 2) {
					my_syslog("错误", "夹爪指令格式错误", cur_account.username);
					my_en_syslog("error", "Incorrect format of claw instruction", cur_account.username);
					my_jap_syslog("さくご", "クリップコマンドのフォーマットが間違っています", cur_account.username);
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
				if (language == 2) {
					cJSON_AddStringToObject(error_json, "key", "爪の動作が遅れて、まず起動しなければならない");
				}
				if (pre_state->gripperFaultNum != 5) {
					my_syslog("错误", "夹爪动作延迟，须先激活", cur_account.username);
					my_en_syslog("error", "Claw action delay, must be activated first", cur_account.username);
					my_jap_syslog("さくご", "爪の動作が遅れて、まず起動しなければならない", cur_account.username);
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
				if (language == 2) {
					cJSON_AddStringToObject(error_json, "key", "クリップが作動していない");
				}
				if (pre_state->gripperFaultNum != 7) {
					my_syslog("错误", "夹爪未激活", cur_account.username);
					my_en_syslog("error", "Claw not active", cur_account.username);
					my_jap_syslog("さくご", "クリップが作動していない", cur_account.username);
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
				if (language == 2) {
					cJSON_AddStringToObject(error_json, "key", "爪の温度が高すぎる");
				}
				if (pre_state->gripperFaultNum != 8) {
					my_syslog("错误", "夹爪温度过高", cur_account.username);
					my_en_syslog("error", "Claw temperature is too high", cur_account.username);
					my_jap_syslog("さくご", "爪の温度が高すぎる", cur_account.username);
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
				if (language == 2) {
					cJSON_AddStringToObject(error_json, "key", "クリップ電圧が低すぎる");
				}
				if (pre_state->gripperFaultNum != 10) {
					my_syslog("错误", "夹爪电压过低", cur_account.username);
					my_en_syslog("error", "Claw voltage is too low", cur_account.username);
					my_jap_syslog("さくご", "クリップ電圧が低すぎる", cur_account.username);
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
				if (language == 2) {
					cJSON_AddStringToObject(error_json, "key", "クリップが自動的に解放されている");
				}
				if (pre_state->gripperFaultNum != 11) {
					my_syslog("错误", "夹爪正在自动释放", cur_account.username);
					my_en_syslog("error", "Claw is releasing automatically", cur_account.username);
					my_jap_syslog("さくご", "クリップが自動的に解放されている", cur_account.username);
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
				if (language == 2) {
					cJSON_AddStringToObject(error_json, "key", "クリップ爪の内部が故障している");
				}
				if (pre_state->gripperFaultNum != 12) {
					my_syslog("错误", "夹爪内部故障", cur_account.username);
					my_en_syslog("error", "Internal failure of clamping claw", cur_account.username);
					my_jap_syslog("さくご", "クリップ爪の内部が故障している", cur_account.username);
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
				if (language == 2) {
					cJSON_AddStringToObject(error_json, "key", "クリップの起動に失敗する");
				}
				if (pre_state->gripperFaultNum != 13) {
					my_syslog("错误", "夹爪激活失败", cur_account.username);
					my_en_syslog("error", "Claw activation failed", cur_account.username);
					my_jap_syslog("さくご", "クリップの起動に失敗する", cur_account.username);
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
				if (language == 2) {
					cJSON_AddStringToObject(error_json, "key", "クリップ電流が大きすぎる");
				}
				if (pre_state->gripperFaultNum != 14) {
					my_syslog("错误", "夹爪电流过大", cur_account.username);
					my_en_syslog("error", "The gripper current is too large", cur_account.username);
					my_jap_syslog("さくご", "クリップ電流が大きすぎる", cur_account.username);
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
				if (language == 2) {
					cJSON_AddStringToObject(error_json, "key", "クリップの自動解放が終了します");
				}
				if (pre_state->gripperFaultNum != 15) {
					my_syslog("错误", "夹爪自动释放结束", cur_account.username);
					my_en_syslog("error", "Claw automatic release end", cur_account.username);
					my_jap_syslog("さくご", "クリップの自動解放が終了します", cur_account.username);
					pre_state->gripperFaultNum = 15;
				}
				break;
			default:
				pre_state->gripperFaultNum = 0;
				break;
		}
		switch(state->slaveComError[0]) {
			case 1:
				if (language == 0) {
					cJSON_AddStringToObject(error_json, "key", "控制箱从站掉线");
				}
				if (language == 1) {
					cJSON_AddStringToObject(error_json, "key", "Control box salve offline");
				}
				if (language == 2) {
					cJSON_AddStringToObject(error_json, "key", "制御ボックスが駅から外れた");
				}
				if (pre_state->slaveComError[0] != 1) {
					my_syslog("错误", "控制箱从站掉线", cur_account.username);
					my_en_syslog("error", "Control box salve offline", cur_account.username);
					my_jap_syslog("さくご", "制御ボックスが駅から外れた", cur_account.username);
					pre_state->slaveComError[0] = 1;
				}
				break;
			case 2:
				if (language == 0) {
					cJSON_AddStringToObject(error_json, "key", "控制箱从站状态与设置值不一致");
				}
				if (language == 1) {
					cJSON_AddStringToObject(error_json, "key", "Control box slave status is not consistent with the set value");
				}
				if (language == 2) {
					cJSON_AddStringToObject(error_json, "key", "制御ボックスはステーション状態と設定値が一致しません");
				}
				if (pre_state->slaveComError[0] != 2) {
					my_syslog("错误", "控制箱从站状态与设置值不一致", cur_account.username);
					my_en_syslog("error", "Control box slave status is not consistent with the set value", cur_account.username);
					my_jap_syslog("さくご", "制御ボックスはステーション状態と設定値が一致しません", cur_account.username);
					pre_state->slaveComError[0] = 2;
				}
				break;
			case 3:
				if (language == 0) {
					cJSON_AddStringToObject(error_json, "key", "控制箱从站未配置");
				}
				if (language == 1) {
					cJSON_AddStringToObject(error_json, "key", "Control box slave is not configured");
				}
				if (language == 2) {
					cJSON_AddStringToObject(error_json, "key", "制御ボックスは従局未配置");
				}
				if (pre_state->slaveComError[0] != 3) {
					my_syslog("错误", "控制箱从站未配置", cur_account.username);
					my_en_syslog("error", "Control box slave is not configured", cur_account.username);
					my_jap_syslog("さくご", "制御ボックスは従局未配置", cur_account.username);
					pre_state->slaveComError[0] = 3;
				}
				break;
			case 4:
				if (language == 0) {
					cJSON_AddStringToObject(error_json, "key", "控制箱从站配置错误");
				}
				if (language == 1) {
					cJSON_AddStringToObject(error_json, "key", "Control box slave configure error");
				}
				if (language == 2) {
					cJSON_AddStringToObject(error_json, "key", "制御ボックスがステーションから誤って配置されています");
				}
				if (pre_state->slaveComError[0] != 4) {
					my_syslog("错误", "控制箱从站配置错误", cur_account.username);
					my_en_syslog("error", "Control box slave configure error", cur_account.username);
					my_jap_syslog("さくご", "制御ボックスがステーションから誤って配置されています", cur_account.username);
					pre_state->slaveComError[0] = 4;
				}
				break;
			case 5:
				if (language == 0) {
					cJSON_AddStringToObject(error_json, "key", "控制箱从站初始化错误");
				}
				if (language == 1) {
					cJSON_AddStringToObject(error_json, "key", "Control box slave initialize error");
				}
				if (language == 2) {
					cJSON_AddStringToObject(error_json, "key", "コントロールボックスはステーションからエラーを初期化します");
				}
				if (pre_state->slaveComError[0] != 5) {
					my_syslog("错误", "控制箱从站初始化错误", cur_account.username);
					my_en_syslog("error", "Control box slave initialize error", cur_account.username);
					my_jap_syslog("さくご", "コントロールボックスはステーションからエラーを初期化します", cur_account.username);
					pre_state->slaveComError[0] = 5;
				}
				break;
			case 6:
				if (language == 0) {
					cJSON_AddStringToObject(error_json, "key", "控制箱从站邮箱通信初始化错误");
				}
				if (language == 1) {
					cJSON_AddStringToObject(error_json, "key", "Control box slave mailbox communication initialize error");
				}
				if (language == 2) {
					cJSON_AddStringToObject(error_json, "key", "制御ボックスはステーションメールボックスから通信初期化エラー");
				}
				if (pre_state->slaveComError[0] != 6) {
					my_syslog("错误", "控制箱从站邮箱通信初始化错误", cur_account.username);
					my_en_syslog("error", "Control box slave mailbox communication initialize error", cur_account.username);
					my_jap_syslog("さくご", "制御ボックスはステーションメールボックスから通信初期化エラー", cur_account.username);
					pre_state->slaveComError[0] = 6;
				}
				break;
			default:
				pre_state->slaveComError[0] = 0;
				break;
		}
		for (i = 1; i <= 6; i++) {
			memset(content, 0, sizeof(content));
			memset(en_content, 0, sizeof(en_content));
			memset(jap_content, 0, sizeof(jap_content));
			switch(state->slaveComError[i]) {
				case 1:
					sprintf(content, "%d 轴从站掉线", i);
					sprintf(en_content, "%d axis salve offline", i);
					sprintf(jap_content, "%d 軸が駅から抜ける", i);
					if (language == 0) {
						cJSON_AddStringToObject(error_json, "key", content);
					}
					if (language == 1) {
						cJSON_AddStringToObject(error_json, "key", en_content);
					}
					if (language == 2) {
						cJSON_AddStringToObject(error_json, "key", jap_content);
					}
					if (pre_state->slaveComError[i] != 1) {
						my_syslog("错误", content, cur_account.username);
						my_en_syslog("error", en_content, cur_account.username);
						my_jap_syslog("さくご", jap_content, cur_account.username);
						pre_state->slaveComError[i] = 1;
					}
					break;
				case 2:
					sprintf(content, "%d 轴从站状态与设置值不一致", i);
					sprintf(en_content, "%d axis slave status is not consistent with the set value", i);
					sprintf(jap_content, "%d 軸スレーブステーションの状態と設定値が一致しない", i);
					if (language == 0) {
						cJSON_AddStringToObject(error_json, "key", content);
					}
					if (language == 1) {
						cJSON_AddStringToObject(error_json, "key", en_content);
					}
					if (language == 2) {
						cJSON_AddStringToObject(error_json, "key", jap_content);
					}
					if (pre_state->slaveComError[i] != 2) {
						my_syslog("错误", content, cur_account.username);
						my_en_syslog("error", en_content, cur_account.username);
						my_jap_syslog("さくご", jap_content, cur_account.username);
						pre_state->slaveComError[i] = 2;
					}
					break;
				case 3:
					sprintf(content, "%d 轴从站未配置", i);
					sprintf(en_content, "%d axis slave is not configured", i);
					sprintf(jap_content, "%d 軸スレーブステーションは未配置", i);
					if (language == 0) {
						cJSON_AddStringToObject(error_json, "key", content);
					}
					if (language == 1) {
						cJSON_AddStringToObject(error_json, "key", en_content);
					}
					if (language == 2) {
						cJSON_AddStringToObject(error_json, "key", jap_content);
					}
					if (pre_state->slaveComError[i] != 3) {
						my_syslog("错误", content, cur_account.username);
						my_en_syslog("error", en_content, cur_account.username);
						my_jap_syslog("さくご", jap_content, cur_account.username);
						pre_state->slaveComError[i] = 3;
					}
					break;
				case 4:
					sprintf(content, "%d 轴从站配置错误", i);
					sprintf(en_content, "%d axis slave configure error", i);
					sprintf(jap_content, "%d 軸スレーブステーション配置ミス", i);
					if (language == 0) {
						cJSON_AddStringToObject(error_json, "key", content);
					}
					if (language == 1) {
						cJSON_AddStringToObject(error_json, "key", en_content);
					}
					if (language == 2) {
						cJSON_AddStringToObject(error_json, "key", jap_content);
					}
					if (pre_state->slaveComError[i] != 4) {
						my_syslog("错误", content, cur_account.username);
						my_en_syslog("error", en_content, cur_account.username);
						my_jap_syslog("さくご", jap_content, cur_account.username);
						pre_state->slaveComError[i] = 4;
					}
					break;
				case 5:
					sprintf(content, "%d 轴从站初始化错误", i);
					sprintf(en_content, "%d axis slave initialize error", i);
					sprintf(jap_content, "%d 軸スレーブステーション初期化エラー", i);
					if (language == 0) {
						cJSON_AddStringToObject(error_json, "key", content);
					}
					if (language == 1) {
						cJSON_AddStringToObject(error_json, "key", en_content);
					}
					if (language == 2) {
						cJSON_AddStringToObject(error_json, "key", jap_content);
					}
					if (pre_state->slaveComError[i] != 5) {
						my_syslog("错误", content, cur_account.username);
						my_en_syslog("error", en_content, cur_account.username);
						my_jap_syslog("さくご", jap_content, cur_account.username);
						pre_state->slaveComError[i] = 5;
					}
					break;
				case 6:
					sprintf(content, "%d 轴从站邮箱通信初始化错误", i);
					sprintf(en_content, "%d axis slave mailbox communication initialize error", i);
					sprintf(jap_content, "%d 軸スレーブメールボックス通信初期化エラー", i);
					if (language == 0) {
						cJSON_AddStringToObject(error_json, "key", content);
					}
					if (language == 1) {
						cJSON_AddStringToObject(error_json, "key", en_content);
					}
					if (language == 2) {
						cJSON_AddStringToObject(error_json, "key", jap_content);
					}
					if (pre_state->slaveComError[i] != 6) {
						my_syslog("错误", content, cur_account.username);
						my_en_syslog("error", en_content, cur_account.username);
						my_jap_syslog("さくご", jap_content, cur_account.username);
						pre_state->slaveComError[i] = 6;
					}
					break;
				default:
					pre_state->slaveComError[i] = 0;
					break;
			}
		}
		switch(state->slaveComError[7]) {
			case 1:
				if (language == 0) {
					cJSON_AddStringToObject(error_json, "key", "末端从站掉线");
				}
				if (language == 1) {
					cJSON_AddStringToObject(error_json, "key", "The terminal salve offline");
				}
				if (language == 2) {
						cJSON_AddStringToObject(error_json, "key", "末端が駅から途切れた");
				}
				if (pre_state->slaveComError[7] != 1) {
					my_syslog("错误", "末端从站掉线", cur_account.username);
					my_en_syslog("error", "The terminal salve offline", cur_account.username);
					my_jap_syslog("さくご", "末端が駅から途切れた", cur_account.username);
					pre_state->slaveComError[7] = 1;
				}
				break;
			case 2:
				if (language == 0) {
					cJSON_AddStringToObject(error_json, "key", "末端从站状态与设置值不一致");
				}
				if (language == 1) {
					cJSON_AddStringToObject(error_json, "key", "The terminal slave status is not consistent with the set value");
				}
				if (language == 2) {
						cJSON_AddStringToObject(error_json, "key", "終端スレーブ局の状態と設定値が一致しない");
				}
				if (pre_state->slaveComError[7] != 2) {
					my_syslog("错误", "末端从站状态与设置值不一致", cur_account.username);
					my_en_syslog("error", "The terminal slave status is not consistent with the set value", cur_account.username);
					my_jap_syslog("さくご", "終端スレーブ局の状態と設定値が一致しない", cur_account.username);
					pre_state->slaveComError[7] = 2;
				}
				break;
			case 3:
				if (language == 0) {
					cJSON_AddStringToObject(error_json, "key", "末端从站未配置");
				}
				if (language == 1) {
					cJSON_AddStringToObject(error_json, "key", "The terminal slave is not configured");
				}
				if (language == 2) {
						cJSON_AddStringToObject(error_json, "key", "末端スレーブ局は未配置");
				}
				if (pre_state->slaveComError[7] != 3) {
					my_syslog("错误", "末端从站未配置", cur_account.username);
					my_en_syslog("error", "The terminal slave is not configured", cur_account.username);
					my_jap_syslog("さくご", "末端スレーブ局は未配置", cur_account.username);
					pre_state->slaveComError[7] = 3;
				}
				break;
			case 4:
				if (language == 0) {
					cJSON_AddStringToObject(error_json, "key", "末端从站配置错误");
				}
				if (language == 1) {
					cJSON_AddStringToObject(error_json, "key", "The terminal  slave configure error");
				}
				if (language == 2) {
						cJSON_AddStringToObject(error_json, "key", "エンドスレーブ局の設定が間違っています");
				}
				if (pre_state->slaveComError[7] != 4) {
					my_syslog("错误", "末端从站配置错误", cur_account.username);
					my_en_syslog("error", "The terminal slave configure error", cur_account.username);
					my_jap_syslog("さくご", "エンドスレーブ局の設定が間違っています", cur_account.username);
					pre_state->slaveComError[7] = 4;
				}
				break;
			case 5:
				if (language == 0) {
					cJSON_AddStringToObject(error_json, "key", "末端从站初始化错误");
				}
				if (language == 1) {
					cJSON_AddStringToObject(error_json, "key", "The terminal  slave initialize error");
				}
				if (language == 2) {
						cJSON_AddStringToObject(error_json, "key", "エンドスレーブ初期化エラー");
				}
				if (pre_state->slaveComError[7] != 5) {
					my_syslog("错误", "末端从站初始化错误", cur_account.username);
					my_en_syslog("error", "The terminal  slave initialize error", cur_account.username);
					my_jap_syslog("さくご", "エンドスレーブ初期化エラー", cur_account.username);
					pre_state->slaveComError[7] = 5;
				}
				break;
			case 6:
				if (language == 0) {
					cJSON_AddStringToObject(error_json, "key", "末端从站邮箱通信初始化错误");
				}
				if (language == 1) {
					cJSON_AddStringToObject(error_json, "key", "The terminal  slave  mailbox communication initialize error");
				}
				if (language == 2) {
						cJSON_AddStringToObject(error_json, "key", "終端メールボックスからの通信初期化エラー");
				}
				if (pre_state->slaveComError[7] != 6) {
					my_syslog("错误", "末端从站邮箱通信初始化错误", cur_account.username);
					my_en_syslog("error", "The terminal  slave  mailbox communication initialize error", cur_account.username);
					my_jap_syslog("さくご", "終端メールボックスからの通信初期化エラー", cur_account.username);
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
					cJSON_AddStringToObject(error_json, "key", "关节指令点错误, 可复位");
				}
				if (language == 1) {
					cJSON_AddStringToObject(error_json, "key", "Joint command point error, can be reset");
				}
				if (language == 2) {
					cJSON_AddStringToObject(error_json, "key", "関節の指令点が間違っている, リセット可能");
				}
				if (pre_state->cmdPointError != 1) {
					my_syslog("错误", "关节指令点错误, 可复位", cur_account.username);
					my_en_syslog("error", "Joint command point error, can be reset", cur_account.username);
					my_jap_syslog("さくご", "関節の指令点が間違っている, リセット可能", cur_account.username);
					pre_state->cmdPointError = 1;
				}
				break;
			case 2:
				if (language == 0) {
					cJSON_AddStringToObject(error_json, "key", "直线目标点错误（包括工具不符）, 可复位");
				}
				if (language == 1) {
					cJSON_AddStringToObject(error_json, "key", "Straight line target point error(including tool discrepancy), can be reset");
				}
				if (language == 2) {
					cJSON_AddStringToObject(error_json, "key", "直線の目標点が間違っている(道具が違っていることも含む), リセット可能");
				}
				if (pre_state->cmdPointError != 2) {
					my_syslog("错误", "直线目标点错误（包括工具不符）, 可复位", cur_account.username);
					my_en_syslog("error", "Straight line target point error(including tool discrepancy), can be reset", cur_account.username);
					my_jap_syslog("さくご", "直線の目標点が間違っている(道具が違っていることも含む), リセット可能", cur_account.username);
					pre_state->cmdPointError = 2;
				}
				break;
			case 3:
				if (language == 0) {
					cJSON_AddStringToObject(error_json, "key", "圆弧中间点错误（包括工具不符）, 可复位");
				}
				if (language == 1) {
					cJSON_AddStringToObject(error_json, "key", "Midpoint of arc error (including tool discrepancy), can be reset");
				}
				if (language == 2) {
					cJSON_AddStringToObject(error_json, "key", "円弧の中間点のエラー(ツールの不一致を含む), リセット可能");
				}
				if (pre_state->cmdPointError != 3) {
					my_syslog("错误", "圆弧中间点错误（包括工具不符）, 可复位", cur_account.username);
					my_en_syslog("error", "Midpoint of arc error (including tool discrepancy), can be reset", cur_account.username);
					my_jap_syslog("さくご", "円弧の中間点のエラー(ツールの不一致を含む), リセット可能", cur_account.username);
					pre_state->cmdPointError = 3;
				}
				break;
			case 4:
				if (language == 0) {
					cJSON_AddStringToObject(error_json, "key", "圆弧目标点错误（包括工具不符）, 可复位");
				}
				if (language == 1) {
					cJSON_AddStringToObject(error_json, "key", "Arc target point error (including tool discrepancy), can be reset");
				}
				if (language == 2) {
					cJSON_AddStringToObject(error_json, "key", "円弧の目標点エラー(ツールが一致しないことを含む), リセット可能");
				}
				if (pre_state->cmdPointError != 4) {
					my_syslog("错误", "圆弧目标点错误（包括工具不符）, 可复位", cur_account.username);
					my_en_syslog("error", "Arc target point error (including tool discrepancy), can be reset", cur_account.username);
					my_jap_syslog("さくご", "円弧の目標点エラー(ツールが一致しないことを含む), リセット可能", cur_account.username);
					pre_state->cmdPointError = 4;
				}
				break;
			case 5:
				if (language == 0) {
					cJSON_AddStringToObject(error_json, "key", "圆弧指令点间距过小, 可复位");
				}
				if (language == 1) {
					cJSON_AddStringToObject(error_json, "key", "The distance between arc instruction points is too small, can be reset");
				}
				if (language == 2) {
					cJSON_AddStringToObject(error_json, "key", "円弧コマンド点の間隔が小さすぎる, リセット可能");
				}
				if (pre_state->cmdPointError != 5) {
					my_syslog("错误", "圆弧指令点间距过小, 可复位", cur_account.username);
					my_en_syslog("error", "The distance between arc instruction points is too small, can be reset", cur_account.username);
					my_jap_syslog("さくご", "円弧コマンド点の間隔が小さすぎる, リセット可能", cur_account.username);
					pre_state->cmdPointError = 5;
				}
				break;
			case 6:
				if (language == 0) {
					cJSON_AddStringToObject(error_json, "key", "整圆/螺旋线指令中间点1错误（包括工具不符），可复位");
				}
				if (language == 1) {
					cJSON_AddStringToObject(error_json, "key", "Full circle/helix instruction midpoint 1 error (including tool mismatch), resettable");
				}
				if (language == 2) {
					cJSON_AddStringToObject(error_json, "key", "全円/螺旋線指令中間点1エラー(工具が合わないことを含む),リセット可能");
				}
				if (pre_state->cmdPointError != 6) {
					my_syslog("错误", "整圆/螺旋线指令中间点1错误（包括工具不符），可复位", cur_account.username);
					my_en_syslog("error", "Full circle/helix instruction midpoint 1 error (including tool mismatch), resettable", cur_account.username);
					my_jap_syslog("さくご", "全円/螺旋線指令中間点1エラー(工具が合わないことを含む),リセット可能", cur_account.username);
					pre_state->cmdPointError = 6;
				}
				break;
			case 7:
				if (language == 0) {
					cJSON_AddStringToObject(error_json, "key", "整圆/螺旋线指令中间点2错误（包括工具不符），可复位");
				}
				if (language == 1) {
					cJSON_AddStringToObject(error_json, "key", "Full circle/helix instruction midpoint 2 error (including tool mismatch), resettable");
				}
				if (language == 2) {
					cJSON_AddStringToObject(error_json, "key", "全円/螺旋線指令中間点2エラー(工具が合わないことを含む),リセット可能");
				}
				if (pre_state->cmdPointError != 7) {
					my_syslog("错误", "整圆/螺旋线指令中间点2错误（包括工具不符），可复位", cur_account.username);
					my_en_syslog("error", "Full circle/helix instruction midpoint 2 error (including tool mismatch), resettable", cur_account.username);
					my_jap_syslog("さくご", "全円/螺旋線指令中間点2エラー(工具が合わないことを含む),リセット可能", cur_account.username);
					pre_state->cmdPointError = 7;
				}
				break;
			case 8:
				if (language == 0) {
					cJSON_AddStringToObject(error_json, "key", "整圆/螺旋线指令中间点3错误（包括工具不符），可复位");
				}
				if (language == 1) {
					cJSON_AddStringToObject(error_json, "key", "Full circle/helix instruction midpoint 3 error (including tool mismatch), resettable");
				}
				if (language == 2) {
					cJSON_AddStringToObject(error_json, "key", "全円/螺旋線指令中間点3エラー(工具が合わないことを含む),リセット可能");
				}
				if (pre_state->cmdPointError != 8) {
					my_syslog("错误", "整圆/螺旋线指令中间点3错误（包括工具不符），可复位", cur_account.username);
					my_en_syslog("error", "Full circle/helix instruction midpoint 3 error (including tool mismatch), resettable", cur_account.username);
					my_jap_syslog("さくご", "全円/螺旋線指令中間点3エラー(工具が合わないことを含む),リセット可能", cur_account.username);
					pre_state->cmdPointError = 8;
				}
				break;
			case 9:
				if (language == 0) {
					cJSON_AddStringToObject(error_json, "key", "整圆/螺旋线指令点间距过小，可复位");
				}
				if (language == 1) {
					cJSON_AddStringToObject(error_json, "key", "Full circle/helix command point spacing is too small, resettable");
				}
				if (language == 2) {
					cJSON_AddStringToObject(error_json, "key", "全円/螺旋線コマンドピッチが小さすぎて、リセット可能");
				}
				if (pre_state->cmdPointError != 9) {
					my_syslog("错误", "整圆/螺旋线指令点间距过小，可复位", cur_account.username);
					my_en_syslog("error", "Full circle/helix command point spacing is too small, resettable", cur_account.username);
					my_jap_syslog("さくご", "全円/螺旋線コマンドピッチが小さすぎて、リセット可能", cur_account.username);
					pre_state->cmdPointError = 9;
				}
				break;
			case 10:
				if (language == 0) {
					cJSON_AddStringToObject(error_json, "key", "TPD指令点错误，可复位");
				}
				if (language == 1) {
					cJSON_AddStringToObject(error_json, "key", "TPD point error, can be reset");
				}
				if (language == 2) {
					cJSON_AddStringToObject(error_json, "key", "TPD指令点エラー, リセット可能");
				}
				if (pre_state->cmdPointError != 10) {
					my_syslog("错误", "TPD指令点错误，可复位", cur_account.username);
					my_en_syslog("error", "TPD point error, can be reset", cur_account.username);
					my_jap_syslog("さくご", "TPD指令点エラー, リセット可能", cur_account.username);
					pre_state->cmdPointError = 10;
				}
				break;
			case 11:
				if (language == 0) {
					cJSON_AddStringToObject(error_json, "key", "TPD指令工具与当前工具不符, 可复位");
				}
				if (language == 1) {
					cJSON_AddStringToObject(error_json, "key", "TPD instruction tool does not match the current tool, can be reset");
				}
				if (language == 2) {
					cJSON_AddStringToObject(error_json, "key", "TPDコマンドツールは、現在のツールと一致しません, リセット可能");
				}
				if (pre_state->cmdPointError != 11) {
					my_syslog("错误", "TPD指令工具与当前工具不符, 可复位", cur_account.username);
					my_en_syslog("error", "TPD instruction tool does not match the current tool, can be reset", cur_account.username);
					my_jap_syslog("さくご", "TPDコマンドツールは、現在のツールと一致しません, リセット可能", cur_account.username);
					pre_state->cmdPointError = 11;
				}
				break;
			case 12:
				if (language == 0) {
					cJSON_AddStringToObject(error_json, "key", "TPD当前指令与下一指令起始点偏差过大, 可复位");
				}
				if (language == 1) {
					cJSON_AddStringToObject(error_json, "key", "TPD the current instruction deviates too much from the starting point of the next instruction, can be reset");
				}
				if (language == 2) {
					cJSON_AddStringToObject(error_json, "key", "TPD現在の命令は、次の命令の開始点から過大にずれている, リセット可能");
				}
				if (pre_state->cmdPointError != 12) {
					my_syslog("错误", "TPD当前指令与下一指令起始点偏差过大, 可复位", cur_account.username);
					my_en_syslog("error", "TPD the current instruction deviates too much from the starting point of the next instruction, can be reset", cur_account.username);
					my_jap_syslog("さくご", "TPD現在の命令は、次の命令の開始点から過大にずれている, リセット可能", cur_account.username);
					pre_state->cmdPointError = 12;
				}
				break;
			case 13:
				if (language == 0) {
					cJSON_AddStringToObject(error_json, "key", "内外部工具切换错误, 可复位");
				}
				if (language == 1) {
					cJSON_AddStringToObject(error_json, "key", "Error switching internal and external tools, can be reset");
				}
				if (language == 2) {
					cJSON_AddStringToObject(error_json, "key", "内部ツールと外部ツールの切り替えエラー, リセット可能");
				}
				if (pre_state->cmdPointError != 13) {
					my_syslog("错误", "内外部工具切换错误, 可复位", cur_account.username);
					my_en_syslog("error", "Error switching internal and external tools, can be reset", cur_account.username);
					my_jap_syslog("さくご", "内部ツールと外部ツールの切り替えエラー, リセット可能", cur_account.username);
					pre_state->cmdPointError = 13;
				}
				break;
			case 17:
				if (language == 0) {
					cJSON_AddStringToObject(error_json, "key", "PTP关节指令超限, 可复位");
				}
				if (language == 1) {
					cJSON_AddStringToObject(error_json, "key", "PTP joint instruction out of limit, can be reset");
				}
				if (language == 2) {
						cJSON_AddStringToObject(error_json, "key", "ptp関節コマンドオーバー, リセット可能");
				}
				if (pre_state->cmdPointError != 17) {
					my_syslog("错误", "PTP关节指令超限, 可复位", cur_account.username);
					my_en_syslog("error", "PTP joint instruction out of limit, can be reset", cur_account.username);
					my_jap_syslog("さくご", "ptp関節コマンドオーバー, リセット可能", cur_account.username);
					pre_state->cmdPointError = 17;
				}
				break;
			case 18:
				if (language == 0) {
					cJSON_AddStringToObject(error_json, "key", "TPD关节指令超限, 可复位");
				}
				if (language == 1) {
					cJSON_AddStringToObject(error_json, "key", "TPD joint instruction out of limit, can be reset");
				}
				if (language == 2) {
					cJSON_AddStringToObject(error_json, "key", "tpd関節コマンドオーバー, リセット可能");
				}
				if (pre_state->cmdPointError != 18) {
					my_syslog("错误", "TPD关节指令超限, 可复位", cur_account.username);
					my_en_syslog("error", "TPD joint instruction out of limit, can be reset", cur_account.username);
					my_jap_syslog("さくご", "tpd関節コマンドオーバー, リセット可能", cur_account.username);
					pre_state->cmdPointError = 18;
				}
				break;
			case 19:
				if (language == 0) {
					cJSON_AddStringToObject(error_json, "key", "LIN/ARC下发关节指令超限, 可复位");
				}
				if (language == 1) {
					cJSON_AddStringToObject(error_json, "key", "LIN/ARC offering joint command out of limit, can be reset");
				}
				if (language == 2) {
					cJSON_AddStringToObject(error_json, "key", "LIN/ARC関節指令を出してオーバーランさせる, リセット可能");
				}
				if (pre_state->cmdPointError != 19) {
					my_syslog("错误", "LIN/ARC下发关节指令超限, 可复位", cur_account.username);
					my_en_syslog("error", "LIN/ARC offering joint command out of limit, can be reset", cur_account.username);
					my_jap_syslog("さくご", "LIN/ARC関節指令を出してオーバーランさせる, リセット可能", cur_account.username);
					pre_state->cmdPointError = 19;
				}
				break;
			case 20:
				if (language == 0) {
					cJSON_AddStringToObject(error_json, "key", "笛卡尔空间内指令超速, 不可复位");
				}
				if (language == 1) {
					cJSON_AddStringToObject(error_json, "key", "Overspeed command in Cartesian space, do not reset");
				}
				if (language == 2) {
					cJSON_AddStringToObject(error_json, "key", "デカルト空間内ではスピードの出しすぎを指示する, リセット不可");
				}
				if (pre_state->cmdPointError != 20) {
					my_syslog("错误", "笛卡尔空间内指令超速, 不可复位", cur_account.username);
					my_en_syslog("error", "Overspeed command in Cartesian space, do not reset", cur_account.username);
					my_jap_syslog("さくご", "デカルト空間内ではスピードの出しすぎを指示する, リセット不可", cur_account.username);
					pre_state->cmdPointError = 20;
				}
				break;
			case 21:
				if (language == 0) {
					cJSON_AddStringToObject(error_json, "key", "关节空间内扭矩指令超限, 可复位");
				}
				if (language == 1) {
					cJSON_AddStringToObject(error_json, "key", "Excessive torque command in joint space, can be reset");
				}
				if (language == 2) {
						cJSON_AddStringToObject(error_json, "key", "関節空間内トルク指令オーバー, リセット可能");
				}
				if (pre_state->cmdPointError != 21) {
					my_syslog("错误", "关节空间内扭矩指令超限, 可复位", cur_account.username);
					my_en_syslog("error", "Excessive torque command in joint space, can be reset", cur_account.username);
					my_jap_syslog("さくご", "関節空間内トルク指令オーバー, リセット可能", cur_account.username);
					pre_state->cmdPointError = 21;
				}
				break;
			case 22:
				if (language == 0) {
					cJSON_AddStringToObject(error_json, "key", "JOG关节指令超限, 可复位");
				}
				if (language == 1) {
					cJSON_AddStringToObject(error_json, "key", "JOG joint instruction out of limit, can be reset");
				}
				if (language == 2) {
					cJSON_AddStringToObject(error_json, "key", "JOG関節コマンドオーバー, リセット可能");
				}
				if (pre_state->cmdPointError != 22) {
					my_syslog("错误", "JOG关节指令超限, 可复位", cur_account.username);
					my_en_syslog("error", "JOG joint instruction out of limit, can be reset", cur_account.username);
					my_jap_syslog("さくご", "JOG関節コマンドオーバー, リセット可能", cur_account.username);
					pre_state->cmdPointError = 22;
				}
				break;
			case 23:
				if (language == 0) {
					cJSON_AddStringToObject(error_json, "key", "轴1关节空间内指令速度超限, 可复位");
				}
				if (language == 1) {
					cJSON_AddStringToObject(error_json, "key", "Axis 1 overrun command velocity in joint space, can be reset");
				}
				if (language == 2) {
					cJSON_AddStringToObject(error_json, "key", "軸1関節空間内指令速度オーバー, リセット可能");
				}
				if (pre_state->cmdPointError != 23) {
					my_syslog("错误", "轴1关节空间内指令速度超限, 可复位", cur_account.username);
					my_en_syslog("error", "Axis 1 joint overrun command velocity in joint space, can be reset", cur_account.username);
					my_jap_syslog("さくご", "軸1関節空間内指令速度オーバー, リセット可能", cur_account.username);
					pre_state->cmdPointError = 23;
				}
				break;
			case 24:
				if (language == 0) {
					cJSON_AddStringToObject(error_json, "key", "轴2关节空间内指令速度超限, 可复位");
				}
				if (language == 1) {
					cJSON_AddStringToObject(error_json, "key", "Axis 2 overrun command velocity in joint space, can be reset");
				}
				if (language == 2) {
					cJSON_AddStringToObject(error_json, "key", "軸2関節空間内指令速度オーバー, リセット可能");
				}
				if (pre_state->cmdPointError != 24) {
					my_syslog("错误", "轴2关节空间内指令速度超限, 可复位", cur_account.username);
					my_en_syslog("error", "Axis 2 overrun command velocity in joint space, can be reset", cur_account.username);
					my_jap_syslog("さくご", "軸2関節空間内指令速度オーバー, リセット可能", cur_account.username);
					pre_state->cmdPointError = 24;
				}
				break;
			case 25:
				if (language == 0) {
					cJSON_AddStringToObject(error_json, "key", "轴3关节空间内指令速度超限, 可复位");
				}
				if (language == 1) {
					cJSON_AddStringToObject(error_json, "key", "Axis 3 overrun command velocity in joint space, can be reset");
				}
				if (language == 2) {
					cJSON_AddStringToObject(error_json, "key", "軸3関節空間内指令速度オーバー, リセット可能");
				}
				if (pre_state->cmdPointError != 25) {
					my_syslog("错误", "轴3关节空间内指令速度超限, 可复位", cur_account.username);
					my_en_syslog("error", "Axis 3 overrun command velocity in joint space, can be reset", cur_account.username);
					my_jap_syslog("さくご", "軸3関節空間内指令速度オーバー, リセット可能", cur_account.username);
					pre_state->cmdPointError = 25;
				}
				break;
			case 26:
				if (language == 0) {
					cJSON_AddStringToObject(error_json, "key", "轴4关节空间内指令速度超限, 可复位");
				}
				if (language == 1) {
					cJSON_AddStringToObject(error_json, "key", "Axis 4 overrun command velocity in joint space, can be reset");
				}
				if (language == 2) {
					cJSON_AddStringToObject(error_json, "key", "軸4関節空間内指令速度オーバー, リセット可能");
				}
				if (pre_state->cmdPointError != 26) {
					my_syslog("错误", "轴4关节空间内指令速度超限, 可复位", cur_account.username);
					my_en_syslog("error", "Axis 4 overrun command velocity in joint space, can be reset", cur_account.username);
					my_jap_syslog("さくご", "軸4関節空間内指令速度オーバー, リセット可能", cur_account.username);
					pre_state->cmdPointError = 26;
				}
				break;
			case 27:
				if (language == 0) {
					cJSON_AddStringToObject(error_json, "key", "轴5关节空间内指令速度超限, 可复位");
				}
				if (language == 1) {
					cJSON_AddStringToObject(error_json, "key", "Axis 5 overrun command velocity in joint space, can be reset");
				}
				if (language == 2) {
					cJSON_AddStringToObject(error_json, "key", "軸5関節空間内指令速度オーバー, リセット可能");
				}
				if (pre_state->cmdPointError != 27) {
					my_syslog("错误", "轴5关节空间内指令速度超限, 可复位", cur_account.username);
					my_en_syslog("error", "Axis 5 overrun command velocity in joint space, can be reset", cur_account.username);
					my_jap_syslog("さくご", "軸5関節空間内指令速度オーバー, リセット可能", cur_account.username);
					pre_state->cmdPointError = 27;
				}
				break;
			case 28:
				if (language == 0) {
					cJSON_AddStringToObject(error_json, "key", "轴6关节空间内指令速度超限, 可复位");
				}
				if (language == 1) {
					cJSON_AddStringToObject(error_json, "key", "Axis 6 overrun command velocity in joint space, can be reset");
				}
				if (language == 2) {
					cJSON_AddStringToObject(error_json, "key", "軸6関節空間内指令速度オーバー, リセット可能");
				}
				if (pre_state->cmdPointError != 28) {
					my_syslog("错误", "轴6关节空间内指令速度超限, 可复位", cur_account.username);
					my_en_syslog("error", "Axis 6 overrun command velocity in joint space, can be reset", cur_account.username);
					my_jap_syslog("さくご", "軸6関節空間内指令速度オーバー, リセット可能", cur_account.username);
					pre_state->cmdPointError = 28;
				}
				break;
			case 33:
				if (language == 0) {
					cJSON_AddStringToObject(error_json, "key", "下一指令关节配置发生变化 (下一指令中存在奇异位姿，请使用 PTP 指令或更改下一指令点)，可复位");
				}
				if (language == 1) {
					cJSON_AddStringToObject(error_json, "key", "The next instruction changes the joint configuration (Singularity pose exists in the next instruction, please use PTP instruction or change the next instruction point), which can be reset");
				}
				if (language == 2) {
					cJSON_AddStringToObject(error_json, "key", "次の指令関節の配置が変わる (次のコマンドの中で奇妙な姿勢が存在して、PTPコマンドを使用してくださいまたは次のコマンドポイントを変更して)、リセットすることができます");
				}
				if (pre_state->cmdPointError != 33) {
					my_syslog("错误", "下一指令关节配置发生变化 (下一指令中存在奇异位姿，请使用 PTP 指令或更改下一指令点)，可复位", cur_account.username);
					my_en_syslog("error", "The next instruction changes the joint configuration (Singularity pose exists in the next instruction, please use PTP instruction or change the next instruction point), which can be reset", cur_account.username);
					my_jap_syslog("さくご", "次の指令関節の配置が変わる (次のコマンドの中で奇妙な姿勢が存在して、PTPコマンドを使用してくださいまたは次のコマンドポイントを変更して)、リセットすることができます", cur_account.username);
					pre_state->cmdPointError = 33;
				}
				break;
			case 34:
				if (language == 0) {
					cJSON_AddStringToObject(error_json, "key", "当前指令关节配置发生变化 （当前指令中存在奇异位姿，请使用 PTP 指令或更改当前指令点），可复位");
				}
				if (language == 1) {
					cJSON_AddStringToObject(error_json, "key", "The current command joint configuration has changed (singular pose exists in the current command, please use PTP command or change the current command point), which can be reset");
				}
				if (language == 2) {
					cJSON_AddStringToObject(error_json, "key", "現在のコマンドに特異な姿勢が存在する場合は、PTPコマンドを使用するか、現在のコマンドポイントを変更してリセット可能にしてください。");
				}
				if (pre_state->cmdPointError != 34) {
					my_syslog("错误", "当前指令关节配置发生变化 （当前指令中存在奇异位姿，请使用 PTP 指令或更改当前指令点），可复位", cur_account.username);
					my_en_syslog("error", "The current command joint configuration has changed (singular pose exists in the current command, please use PTP command or change the current command point), which can be reset", cur_account.username);
					my_jap_syslog("さくご", "現在のコマンドに特異な姿勢が存在する場合は、PTPコマンドを使用するか、現在のコマンドポイントを変更してリセット可能にしてください。", cur_account.username);
					pre_state->cmdPointError = 34;
				}
				break;
			case 49:
				if (language == 0) {
					cJSON_AddStringToObject(error_json, "key", "焊接指令错误，ARCSTART 和 ARCEND 之间只允许 LIN 和 ARC 指令, 可复位");
				}
				if (language == 1) {
					cJSON_AddStringToObject(error_json, "key", "Incorrect welding instruction. Only LIN and ARC instructions are allowed between ARCSTART and ARCEND, can be reset");
				}
				if (language == 2) {
					cJSON_AddStringToObject(error_json, "key", "ハンダ付け命令が間違っていて、ARCSTARTとARCENDの間でLINとARC命令しか許されない, リセット可能");
				}
				if (pre_state->cmdPointError != 49) {
					my_syslog("错误", "焊接指令错误，ARCSTART 和 ARCEND 之间只允许 LIN 和 ARC 指令, 可复位", cur_account.username);
					my_en_syslog("error", "Incorrect welding instruction. Only LIN and ARC instructions are allowed between ARCSTART and ARCEND, can be reset", cur_account.username);
					my_jap_syslog("さくご", "ハンダ付け命令が間違っていて、ARCSTARTとARCENDの間でLINとARC命令しか許されない, リセット可能", cur_account.username);
					pre_state->cmdPointError = 49;
				}
				break;
			case 50:
				if (language == 0) {
					cJSON_AddStringToObject(error_json, "key", "焊接指令错误， WEAVESTART 和 WEAVEEND 之间只允许 LIN 指令, 可复位");
				}
				if (language == 1) {
					cJSON_AddStringToObject(error_json, "key", "Incorrect welding instruction. Only LIN instructions are allowed between WEAVESTART and WEAVEEND, can be reset");
				}
				if (language == 2) {
					cJSON_AddStringToObject(error_json, "key", "ハンダ付け命令が間違っていて、WEAVESTARTとWEAVEENDの間でLIN命令しか許されない, リセット可能");
				}
				if (pre_state->cmdPointError != 50) {
					my_syslog("错误", "焊接指令错误，WEAVESTART 和 WEAVEEND 之间只允许 LIN 指令, 可复位", cur_account.username);
					my_en_syslog("error", "Incorrect welding instruction. Only LIN instructions are allowed between WEAVESTART and WEAVEEND, can be reset", cur_account.username);
					my_jap_syslog("さくご", "ハンダ付け命令が間違っていて、WEAVESTARTとWEAVEENDの間でLIN命令しか許されない, リセット可能", cur_account.username);
					pre_state->cmdPointError = 50;
				}
				break;
			case 51:
				if (language == 0) {
					cJSON_AddStringToObject(error_json, "key", "摆焊参数错误, 可复位");
				}
				if (language == 1) {
					cJSON_AddStringToObject(error_json, "key", "Swing welding parameter error, can be reset");
				}
				if (language == 2) {
					cJSON_AddStringToObject(error_json, "key", "振り子溶接パラメータが間違っている, リセット可能");
				}
				if (pre_state->cmdPointError != 51) {
					my_syslog("错误", "摆焊参数错误, 可复位", cur_account.username);
					my_en_syslog("error", "Swing welding parameter error, can be reset", cur_account.username);
					my_jap_syslog("さくご", "振り子溶接パラメータが間違っている, リセット可能", cur_account.username);
					pre_state->cmdPointError = 51;
				}
				break;
			case 65:
				if (language == 0) {
					cJSON_AddStringToObject(error_json, "key", "激光传感器指令偏差过大, 可复位");
				}
				if (language == 1) {
					cJSON_AddStringToObject(error_json, "key", "Laser sensor instruction deviation is too large, can be reset");
				}
				if (language == 2) {
					cJSON_AddStringToObject(error_json, "key", "レーザーセンサーの指令偏差が大きすぎる, リセット可能");
				}
				if (pre_state->cmdPointError != 65) {
					my_syslog("错误", "激光传感器指令偏差过大, 可复位", cur_account.username);
					my_en_syslog("error", "Laser sensor instruction deviation is too large, can be reset", cur_account.username);
					my_jap_syslog("さくご", "レーザーセンサーの指令偏差が大きすぎる, リセット可能", cur_account.username);
					pre_state->cmdPointError = 65;
				}
				break;
			case 66:
				if (language == 0) {
					cJSON_AddStringToObject(error_json, "key", "激光传感器指令中断, 焊缝跟踪提前结束, 可复位");
				}
				if (language == 1) {
					cJSON_AddStringToObject(error_json, "key", "Laser sensor instruction is interrupted, weld tracking ends prematurely, can be reset");
				}
				if (language == 2) {
					cJSON_AddStringToObject(error_json, "key", "レーザーセンサは中断され、溶接部の追跡は早期に終了します, リセット可能");
				}
				if (pre_state->cmdPointError != 66) {
					my_syslog("错误", "激光传感器指令中断, 焊缝跟踪提前结束, 可复位", cur_account.username);
					my_en_syslog("error", "Laser sensor instruction is interrupted, weld tracking ends prematurely, can be reset", cur_account.username);
					my_jap_syslog("さくご", "レーザーセンサは中断され、溶接部の追跡は早期に終了します, リセット可能", cur_account.username);
					pre_state->cmdPointError = 66;
				}
				break;
			case 81:
				if (language == 0) {
					cJSON_AddStringToObject(error_json, "key", "外部轴指令速度超限, 可复位");
				}
				if (language == 1) {
					cJSON_AddStringToObject(error_json, "key", "External shaft instruction speed over limit, can be reset");
				}
				if (language == 2) {
					cJSON_AddStringToObject(error_json, "key", "外部軸指令速度オーバー, リセット可能");
				}
				if (pre_state->cmdPointError != 81) {
					my_syslog("错误", "外部轴指令速度超限, 可复位", cur_account.username);
					my_en_syslog("error", "External shaft instruction speed over limit, can be reset", cur_account.username);
					my_jap_syslog("さくご", "外部軸指令速度オーバー, リセット可能", cur_account.username);
					pre_state->cmdPointError = 81;
				}
				break;
			case 82:
				if (language == 0) {
					cJSON_AddStringToObject(error_json, "key", "外部轴指令与反馈偏差过大，不可复位，需要回零或重启");
				}
				if (language == 1) {
					cJSON_AddStringToObject(error_json, "key", "The deviation between the external axis instruction and the feedback is too large, which cannot be reset. It needs to be reset to zero or restart");
				}
				if (language == 2) {
					cJSON_AddStringToObject(error_json, "key", "外部軸指令とフィードバックのずれが大きすぎてリセットできないので、ゼロに戻すかリセットする必要があります");
				}
				if (pre_state->cmdPointError != 82) {
					my_syslog("错误", "外部轴指令与反馈偏差过大，不可复位，需要回零或重启", cur_account.username);
					my_en_syslog("error", "The deviation between the external axis instruction and the feedback is too large, which cannot be reset. It needs to be reset to zero or restart", cur_account.username);
					my_jap_syslog("さくご", "外部軸指令とフィードバックのずれが大きすぎてリセットできないので、ゼロに戻すかリセットする必要があります", cur_account.username);
					pre_state->cmdPointError = 82;
				}
				break;
			case 97:
				if (language == 0) {
					cJSON_AddStringToObject(error_json, "key", "传送带跟踪-起始点与参考点姿态变化过大, 可复位");
				}
				if (language == 1) {
					cJSON_AddStringToObject(error_json, "key", "Belt tracking - starting point and reference point attitude change too much, can be reset");
				}
				if (language == 2) {
					cJSON_AddStringToObject(error_json, "key", "ベルトコンベヤー追跡-開始点と基準点の姿勢が変化しすぎている, リセット可能");
				}
				if (pre_state->cmdPointError != 97) {
					my_syslog("错误", "传送带跟踪-起始点与参考点姿态变化过大, 可复位", cur_account.username);
					my_en_syslog("error", "Belt tracking - starting point and reference point attitude change too much, can be reset", cur_account.username);
					my_jap_syslog("さくご", "ベルトコンベヤー追跡-開始点と基準点の姿勢が変化しすぎている, リセット可能", cur_account.username);
					pre_state->cmdPointError = 97;
				}
				break;
			case 113:
				if (language == 0) {
					cJSON_AddStringToObject(error_json, "key", "恒力控制-X方向超过最大调整距离，可复位");
				}
				if (language == 1) {
					cJSON_AddStringToObject(error_json, "key", "Constant force control -X direction exceeds the maximum adjustment distance, can be reset");
				}
				if (language == 2) {
					cJSON_AddStringToObject(error_json, "key", "定力制御-X方向に最大調整距離を超え、リセット可能");
				}
				if (pre_state->cmdPointError != 113) {
					my_syslog("错误", "恒力控制-X方向超过最大调整距离，可复位", cur_account.username);
					my_en_syslog("error", "Constant force control -X direction exceeds the maximum adjustment distance, can be reset", cur_account.username);
					my_jap_syslog("さくご", "定力制御-X方向に最大調整距離を超え、リセット可能", cur_account.username);
					pre_state->cmdPointError = 113;
				}
				break;
			case 114:
				if (language == 0) {
					cJSON_AddStringToObject(error_json, "key", "恒力控制-Y方向超过最大调整距离，可复位");
				}
				if (language == 1) {
					cJSON_AddStringToObject(error_json, "key", "Constant force control -Y direction exceeds the maximum adjustment distance, can be reset");
				}
				if (language == 2) {
					cJSON_AddStringToObject(error_json, "key", "定力制御-Y方向が最大調整距離を超えてリセット可能");
				}
				if (pre_state->cmdPointError != 114) {
					my_syslog("错误", "恒力控制-Y方向超过最大调整距离，可复位", cur_account.username);
					my_en_syslog("error", "Constant force control -Y direction exceeds the maximum adjustment distance, can be reset", cur_account.username);
					my_jap_syslog("さくご", "定力制御-Y方向が最大調整距離を超えてリセット可能", cur_account.username);
					pre_state->cmdPointError = 114;
				}
				break;
			case 115:
				if (language == 0) {
					cJSON_AddStringToObject(error_json, "key", "恒力控制-Z方向超过最大调整距离，可复位");
				}
				if (language == 1) {
					cJSON_AddStringToObject(error_json, "key", "Constant force control -Z direction exceeds the maximum adjustment distance, can be reset");
				}
				if (language == 2) {
					cJSON_AddStringToObject(error_json, "key", "定力制御-Z方向最大調整距離を超えてリセット可能");
				}
				if (pre_state->cmdPointError != 115) {
					my_syslog("错误", "恒力控制-Z方向超过最大调整距离，可复位", cur_account.username);
					my_en_syslog("error", "Constant force control -Z direction exceeds the maximum adjustment distance, can be reset", cur_account.username);
					my_jap_syslog("さくご", "定力制御-Z方向最大調整距離を超えてリセット可能", cur_account.username);
					pre_state->cmdPointError = 115;
				}
				break;
			case 116:
				if (language == 0) {
					cJSON_AddStringToObject(error_json, "key", "恒力控制-RX方向超过最大调整角度，可复位");
				}
				if (language == 1) {
					cJSON_AddStringToObject(error_json, "key", "Constant force control -RX direction exceeds the maximum adjustment Angle, can be reset");
				}
				if (language == 2) {
					cJSON_AddStringToObject(error_json, "key", "定力制御-RX方向を最大調整角度以上、リセット可能");
				}
				if (pre_state->cmdPointError != 116) {
					my_syslog("错误", "恒力控制-RX方向超过最大调整角度，可复位", cur_account.username);
					my_en_syslog("error", "Constant force control -RX direction exceeds the maximum adjustment Angle, can be reset", cur_account.username);
					my_jap_syslog("さくご", "定力制御-RX方向を最大調整角度以上、リセット可能", cur_account.username);
					pre_state->cmdPointError = 116;
				}
				break;
			case 117:
				if (language == 0) {
					cJSON_AddStringToObject(error_json, "key", "恒力控制-RY方向超过最大调整角度，可复位");
				}
				if (language == 1) {
					cJSON_AddStringToObject(error_json, "key", "Constant force control -RY direction exceeds the maximum adjustment Angle, can be reset");
				}
				if (language == 2) {
					cJSON_AddStringToObject(error_json, "key", "力制御-RY方向が最大調整角度を超えてリセット可能");
				}
				if (pre_state->cmdPointError != 117) {
					my_syslog("错误", "恒力控制-RY方向超过最大调整角度，可复位", cur_account.username);
					my_en_syslog("error", "Constant force control -RY direction exceeds the maximum adjustment Angle, can be reset", cur_account.username);
					my_jap_syslog("さくご", "力制御-RY方向が最大調整角度を超えてリセット可能", cur_account.username);
					pre_state->cmdPointError = 117;
				}
				break;
			case 118:
				if (language == 0) {
					cJSON_AddStringToObject(error_json, "key", "恒力控制-RZ方向超过最大调整角度，可复位");
				}
				if (language == 1) {
					cJSON_AddStringToObject(error_json, "key", "Constant force control -RZ direction exceeds the maximum adjustment Angle, can be reset");
				}
				if (language == 2) {
					cJSON_AddStringToObject(error_json, "key", "定力制御-RZ方向最大調整角度を超えてリセット可能");
				}
				if (pre_state->cmdPointError != 118) {
					my_syslog("错误", "恒力控制-RZ方向超过最大调整角度，可复位", cur_account.username);
					my_en_syslog("error", "Constant force control -RZ direction exceeds the maximum adjustment Angle, can be reset", cur_account.username);
					my_jap_syslog("さくご", "定力制御-RZ方向最大調整角度を超えてリセット可能", cur_account.username);
					pre_state->cmdPointError = 118;
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
				}
				if (language == 1) {
					cJSON_AddStringToObject(error_json, "key", "The channel error");
				}
				if (language == 2) {
					cJSON_AddStringToObject(error_json, "key", "チャネルエラー");
				}
				if (pre_state->ioError != 1) {
					my_syslog("错误", "通道错误", cur_account.username);
					my_en_syslog("error", "The channel error", cur_account.username);
					my_jap_syslog("さくご", "チャネルエラー", cur_account.username);
					pre_state->ioError = 1;
				}
				break;
			case 2:
				if (language == 0) {
					cJSON_AddStringToObject(error_json, "key", "数值错误");
				}
				if (language == 1) {
					cJSON_AddStringToObject(error_json, "key", "numerical fault");
				}
				if (language == 2) {
					cJSON_AddStringToObject(error_json, "key", "数値エラー");
				}
				if (pre_state->ioError != 2) {
					my_syslog("错误", "数值错误", cur_account.username);
					my_en_syslog("error", "numerical fault", cur_account.username);
					my_jap_syslog("さくご", "数値エラー", cur_account.username);
					pre_state->ioError = 2;
				}
				break;
			case 3:
				if (language == 0) {
					cJSON_AddStringToObject(error_json, "key", "WaitDI等待超时");
				}
				if (language == 1) {
					cJSON_AddStringToObject(error_json, "key", "WaitDI wait for a timeout");
				}
				if (language == 2) {
					cJSON_AddStringToObject(error_json, "key", "WaitDI タイムアウト待ち");
				}
				if (pre_state->ioError != 3) {
					my_syslog("错误", "WaitDI等待超时", cur_account.username);
					my_en_syslog("error", "WaitDI wait for a timeout", cur_account.username);
					my_jap_syslog("さくご", "WaitDI タイムアウト待ち", cur_account.username);
					pre_state->ioError = 3;
				}
				break;
			case 4:
				if (language == 0) {
					cJSON_AddStringToObject(error_json, "key", "WaitAI等待超时");
				}
				if (language == 1) {
					cJSON_AddStringToObject(error_json, "key", "WaitAI wait for a timeout");
				}
				if (language == 2) {
					cJSON_AddStringToObject(error_json, "key", "WaitAI タイムアウト待ち");
				}
				if (pre_state->ioError != 4) {
					my_syslog("错误", "WaitAI等待超时", cur_account.username);
					my_en_syslog("error", "WaitAI wait for a timeout", cur_account.username);
					my_jap_syslog("さくご", "WaitAI タイムアウト待ち", cur_account.username);
					pre_state->ioError = 4;
				}
				break;
			case 5:
				if (language == 0) {
					cJSON_AddStringToObject(error_json, "key", "WaitAxleDI等待超时");
				}
				if (language == 1) {
					cJSON_AddStringToObject(error_json, "key", "WaitAxleDI wait for a timeout");
				}
				if (language == 2) {
					cJSON_AddStringToObject(error_json, "key", "WaitAxleDI タイムアウト待ち");
				}
				if (pre_state->ioError != 5) {
					my_syslog("错误", "WaitAxleDI等待超时", cur_account.username);
					my_en_syslog("error", "WaitAxleDI wait for a timeout", cur_account.username);
					my_jap_syslog("さくご", "WaitAxleDI タイムアウト待ち", cur_account.username);
					pre_state->ioError = 5;
				}
				break;
			case 6:
				if (language == 0) {
					cJSON_AddStringToObject(error_json, "key", "WaitAxleAI等待超时");
				}
				if (language == 1) {
					cJSON_AddStringToObject(error_json, "key", "WaitAxleAI wait for a timeout");
				}
				if (language == 2) {
					cJSON_AddStringToObject(error_json, "key", "WaitAxleAI タイムアウト待ち");
				}
				if (pre_state->ioError != 6) {
					my_syslog("错误", "WaitAxleAI等待超时", cur_account.username);
					my_en_syslog("error", "WaitAxleAI wait for a timeout", cur_account.username);
					my_jap_syslog("さくご", "WaitAxleAI タイムアウト待ち", cur_account.username);
					pre_state->ioError = 6;
				}
				break;
			case 7:
				if (language == 0) {
					cJSON_AddStringToObject(error_json, "key", "通道已配置功能错误");
				}
				if (language == 1) {
					cJSON_AddStringToObject(error_json, "key", "The channel has been configured functionally wrong");
				}
				if (language == 2) {
					cJSON_AddStringToObject(error_json, "key", "チャンネルが設定されています");
				}
				if (pre_state->ioError != 7) {
					my_syslog("错误", "通道已配置功能错误", cur_account.username);
					my_en_syslog("error", "The channel has been configured functionally wrong", cur_account.username);
					my_jap_syslog("さくご", "チャンネルが設定されています", cur_account.username);
					pre_state->ioError = 7;
				}
				break;
			case 8:
				if (language == 0) {
					cJSON_AddStringToObject(error_json, "key", "起弧超时");
				}
				if (language == 1) {
					cJSON_AddStringToObject(error_json, "key", "Striking a timeout");
				}
				if (language == 2) {
					cJSON_AddStringToObject(error_json, "key", "起弧タイムアウト");
				}
				if (pre_state->ioError != 8) {
					my_syslog("错误", "起弧超时", cur_account.username);
					my_en_syslog("error", "Striking a timeout", cur_account.username);
					my_jap_syslog("さくご", "起弧タイムアウト", cur_account.username);
					pre_state->ioError = 8;
				}
				break;
			case 9:
				if (language == 0) {
					cJSON_AddStringToObject(error_json, "key", "收弧超时");
				}
				if (language == 1) {
					cJSON_AddStringToObject(error_json, "key", "Are-receive timeout");
				}
				if (language == 2) {
					cJSON_AddStringToObject(error_json, "key", "円弧タイムアウト");
				}
				if (pre_state->ioError != 9) {
					my_syslog("错误", "收弧超时", cur_account.username);
					my_en_syslog("error", "Are-receive timeout", cur_account.username);
					my_jap_syslog("さくご", "円弧タイムアウト", cur_account.username);
					pre_state->ioError = 9;
				}
				break;
			case 10:
				if (language == 0) {
					cJSON_AddStringToObject(error_json, "key", "寻位超时");
				}
				if (language == 1) {
					cJSON_AddStringToObject(error_json, "key", "Find a timeout");
				}
				if (language == 2) {
					cJSON_AddStringToObject(error_json, "key", "アドレッシング・タイムアウト");
				}
				if (pre_state->ioError != 10) {
					my_syslog("错误", "寻位超时", cur_account.username);
					my_en_syslog("error", "Find a timeout", cur_account.username);
					my_jap_syslog("さくご", "アドレッシング・タイムアウト", cur_account.username);
					pre_state->ioError = 10;
				}
				break;
			case 11:
				if (language == 0) {
					cJSON_AddStringToObject(error_json, "key", "传送带IO检测超时");
				}
				if (language == 1) {
					cJSON_AddStringToObject(error_json, "key", "IO detection of conveyor belt has expired");
				}
				if (language == 2) {
					cJSON_AddStringToObject(error_json, "key", "ベルトコンベヤーioがタイムアウトを検出する");
				}
				if (pre_state->ioError != 11) {
					my_syslog("错误", "传送带IO检测超时", cur_account.username);
					my_en_syslog("error", "IO detection of conveyor belt has expired", cur_account.username);
					my_jap_syslog("さくご", "ベルトコンベヤーioがタイムアウトを検出する", cur_account.username);
					pre_state->ioError = 11;
				}
				break;
			default:
				pre_state->ioError = 0;
				break;
		}
		if (state->gripperError == 1) {
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "夹爪运动超时错误");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "Claw movement timeout error");
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", "爪運動タイムアウトエラー");
			}
			if (pre_state->gripperError != 1) {
				my_syslog("错误", "夹爪运动超时错误", cur_account.username);
				my_en_syslog("error", "Claw movement timeout error", cur_account.username);
				my_jap_syslog("さくご", "爪運動タイムアウトエラー", cur_account.username);
				pre_state->gripperError = 1;
			}
		} else {
			pre_state->gripperError = 0;
		}
		switch(state->fileError) {
			case 1:
				if (language == 0) { 
					cJSON_AddStringToObject(error_json, "key", "zbt配置文件版本错误");
				}
				if (language == 1) {
					cJSON_AddStringToObject(error_json, "key", "zbt incorrect configuration file version");
				}
				if (language == 2) {
					cJSON_AddStringToObject(error_json, "key", "zbtプロファイルのバージョンが間違っています");
				}
				if (pre_state->fileError != 1) {
					my_syslog("错误", "zbt配置文件版本错误", cur_account.username);
					my_en_syslog("error", "zbt incorrect configuration file version", cur_account.username);
					my_jap_syslog("さくご", "zbtプロファイルのバージョンが間違っています", cur_account.username);
					pre_state->fileError = 1;
				}
				break;
			case 2:
				if (language == 0) {
					cJSON_AddStringToObject(error_json, "key", "zbt配置文件加载失败");
				}
				if (language == 1) {
					cJSON_AddStringToObject(error_json, "key", "zbt the configuration file failed to load");
				}
				if (language == 2) {
					cJSON_AddStringToObject(error_json, "key", "zbtプロファイルのロードに失敗しました");
				}
				if (pre_state->fileError != 2) {
					my_syslog("错误", "zbt配置文件加载失败", cur_account.username);
					my_en_syslog("error", "zbt the configuration file failed to load", cur_account.username);
					my_jap_syslog("さくご", "zbtプロファイルのロードに失敗しました", cur_account.username);
					pre_state->fileError = 2;
				}
				break;
			case 3:
				if (language == 0) {
					cJSON_AddStringToObject(error_json, "key", "user配置文件版本错误");
				}
				if (language == 1) {
					cJSON_AddStringToObject(error_json, "key", "user incorrect configuration file version");
				}
				if (language == 2) {
					cJSON_AddStringToObject(error_json, "key", "userプロファイルのバージョンが間違っています");
				}
				if (pre_state->fileError != 3) {
					my_syslog("错误", "user配置文件版本错误", cur_account.username);
					my_en_syslog("error", "user incorrect configuration file version", cur_account.username);
					my_jap_syslog("さくご", "userプロファイルのバージョンが間違っています", cur_account.username);
					pre_state->fileError = 3;
				}
				break;
			case 4:
				if (language == 0) {
					cJSON_AddStringToObject(error_json, "key", "user配置文件加载失败");
				}
				if (language == 1) {
					cJSON_AddStringToObject(error_json, "key", "user the configuration file failed to load");
				}
				if (language == 2) {
					cJSON_AddStringToObject(error_json, "key", "userプロファイルの読み込みに失敗しました");
				}
				if (pre_state->fileError != 4) {
					my_syslog("错误", "user配置文件加载失败", cur_account.username);
					my_en_syslog("error", "user the configuration file failed to load", cur_account.username);
					my_jap_syslog("さくご", "userプロファイルの読み込みに失敗しました", cur_account.username);
					pre_state->fileError = 4;
				}
				break;
			case 5:
				if (language == 0) {
					cJSON_AddStringToObject(error_json, "key", "exaxis配置文件版本错误");
				}
				if (language == 1) {
					cJSON_AddStringToObject(error_json, "key", "exaxis incorrect configuration file version");
				}
				if (language == 2) {
					cJSON_AddStringToObject(error_json, "key", "exaxisプロファイルのバージョンが間違っています");
				}
				if (pre_state->fileError != 5) {
					my_syslog("错误", "exaxis配置文件版本错误", cur_account.username);
					my_en_syslog("error", "exaxis incorrect configuration file version", cur_account.username);
					my_jap_syslog("さくご", "exaxisプロファイルのバージョンが間違っています", cur_account.username);
					pre_state->fileError = 5;
				}
				break;
			case 6:
				if (language == 0) {
					cJSON_AddStringToObject(error_json, "key", "exaxis配置文件加载失败");
				}
				if (language == 1) {
					cJSON_AddStringToObject(error_json, "key", "exaxis the configuration file failed to load");
				}
				if (language == 2) {
					cJSON_AddStringToObject(error_json, "key", "exaxisプロファイルのロードに失敗しました");
				}
				if (pre_state->fileError != 6) {
					my_syslog("错误", "exaxis配置文件加载失败", cur_account.username);
					my_en_syslog("error", "exaxis the configuration file failed to load", cur_account.username);
					my_jap_syslog("さくご", "exaxisプロファイルのロードに失敗しました", cur_account.username);
					pre_state->fileError = 6;
				}
				break;
			case 7:
				if (language == 0) {
					cJSON_AddStringToObject(error_json, "key", "机器人型号不一致，需要重新设置-不可复位");
				}
				if (language == 1) {
					cJSON_AddStringToObject(error_json, "key", "Robot models are inconsistent and need to be reset - not reset");
				}
				if (language == 2) {
					cJSON_AddStringToObject(error_json, "key", "ロボットの型番が一致していないので再設定が必要-リセット不可");
				}
				if (pre_state->fileError != 7) {
					my_syslog("错误", "机器人型号不一致，需要重新设置-不可复位", cur_account.username);
					my_en_syslog("error", "Robot models are inconsistent and need to be reset - not reset", cur_account.username);
					my_jap_syslog("さくご", "ロボットの型番が一致していないので再設定が必要-リセット不可", cur_account.username);
					pre_state->fileError = 7;
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
				}
				if (language == 1) {
					cJSON_AddStringToObject(error_json, "key", "Error with tool number overrun");
				}
				if (language == 2) {
					cJSON_AddStringToObject(error_json, "key", "工具番号オーバーエラー");
				}
				if (pre_state->paraError != 1) {
					my_syslog("错误", "工具号超限错误", cur_account.username);
					my_en_syslog("error", "Error with tool number overrun", cur_account.username);
					my_jap_syslog("さくご", "工具番号オーバーエラー", cur_account.username);
					pre_state->paraError = 1;
				}
				break;
			case 2:
				if (language == 0) {
					cJSON_AddStringToObject(error_json, "key", "定位完成阈值错误");
				}
				if (language == 1) {
					cJSON_AddStringToObject(error_json, "key", "Error in positioning completion threshold");
				}
				if (language == 2) {
					cJSON_AddStringToObject(error_json, "key", "定定完了しきい値エラー");
				}
				if (pre_state->paraError != 2) {
					my_syslog("错误", "定位完成阈值错误", cur_account.username);
					my_en_syslog("error", "Error in positioning completion threshold", cur_account.username);
					my_jap_syslog("さくご", "定定完了しきい値エラー", cur_account.username);
					pre_state->paraError = 2;
				}
				break;
			case 3:
				if (language == 0) {
					cJSON_AddStringToObject(error_json, "key", "碰撞等级错误");
				}
				if (language == 1) {
					cJSON_AddStringToObject(error_json, "key", "Collision level error");
				}
				if (language == 2) {
					cJSON_AddStringToObject(error_json, "key", "衝突レベルの誤り");
				}
				if (pre_state->paraError != 3) {
					my_syslog("错误", "碰撞等级错误", cur_account.username);
					my_en_syslog("error", "Collision level error", cur_account.username);
					my_jap_syslog("さくご", "衝突レベルの誤り", cur_account.username);
					pre_state->paraError = 3;
				}
				break;
			case 4:
				if (language == 0) {
					cJSON_AddStringToObject(error_json, "key", "负载重量错误");
				}
				if (language == 1) {
					cJSON_AddStringToObject(error_json, "key", "Load weight error");
				}
				if (language == 2) {
					cJSON_AddStringToObject(error_json, "key", "荷重重量の誤り");
				}
				if (pre_state->paraError != 4) {
					my_syslog("错误", "负载重量错误", cur_account.username);
					my_en_syslog("error", "Load weight error", cur_account.username);
					my_jap_syslog("さくご", "荷重重量の誤り", cur_account.username);
					pre_state->paraError = 4;
				}
				break;
			case 5:
				if (language == 0) {
					cJSON_AddStringToObject(error_json, "key", "负载质心X错误");
				}
				if (language == 1) {
					cJSON_AddStringToObject(error_json, "key", "Load center of mass X error");
				}
				if (language == 2) {
					cJSON_AddStringToObject(error_json, "key", "荷重质心x間違って");
				}
				if (pre_state->paraError != 5) {
					my_syslog("错误", "负载质心X错误", cur_account.username);
					my_en_syslog("error", "Load center of mass X error", cur_account.username);
					my_jap_syslog("さくご", "荷重质心x間違って", cur_account.username);
					pre_state->paraError = 5;
				}
				break;
			case 6:
				if (language == 0) {
					cJSON_AddStringToObject(error_json, "key", "负载质心Y错误");
				}
				if (language == 1) {
					cJSON_AddStringToObject(error_json, "key", "Load center of mass Y error");
				}
				if (language == 2) {
					cJSON_AddStringToObject(error_json, "key", "負荷重心Yエラー");
				}
				if (pre_state->paraError != 6) {
					my_syslog("错误", "负载质心Y错误", cur_account.username);
					my_en_syslog("error", "Load center of mass Y error", cur_account.username);
					my_jap_syslog("さくご", "負荷重心Yエラー", cur_account.username);
					pre_state->paraError = 6;
				}
				break;
			case 7:
				if (language == 0) {
					cJSON_AddStringToObject(error_json, "key", "负载质心Z错误");
				}
				if (language == 1) {
					cJSON_AddStringToObject(error_json, "key", "Load center of mass Z error");
				}
				if (language == 2) {
					cJSON_AddStringToObject(error_json, "key", "負荷重心zエラー");
				}
				if (pre_state->paraError != 7) {
					my_syslog("错误", "负载质心Z错误", cur_account.username);
					my_en_syslog("error", "Load center of mass Z error", cur_account.username);
					my_jap_syslog("さくご", "負荷重心zエラー", cur_account.username);
					pre_state->paraError = 7;
				}
				break;
			case 8:
				if (language == 0) {
					cJSON_AddStringToObject(error_json, "key", "DI滤波时间错误");
				}
				if (language == 1) {
					cJSON_AddStringToObject(error_json, "key", "DI filtering time error");
				}
				if (language == 2) {
					cJSON_AddStringToObject(error_json, "key", "diフィルタリング時間エラー");
				}
				if (pre_state->paraError != 8) {
					my_syslog("错误", "DI滤波时间错误", cur_account.username);
					my_en_syslog("error", "DI filtering time error", cur_account.username);
					my_jap_syslog("さくご", "diフィルタリング時間エラー", cur_account.username);
					pre_state->paraError = 8;
				}
				break;
			case 9:
				if (language == 0) {
					cJSON_AddStringToObject(error_json, "key", "AxleDI滤波时间错误");
				}
				if (language == 1) {
					cJSON_AddStringToObject(error_json, "key", "AxleDI filtering time error");
				}
				if (language == 2) {
					cJSON_AddStringToObject(error_json, "key", "axlediフィルタリング時間エラー");
				}
				if (pre_state->paraError != 9) {
					my_syslog("错误", "AxleDI滤波时间错误", cur_account.username);
					my_en_syslog("error", "AxleDI filtering time error", cur_account.username);
					my_jap_syslog("さくご", "axlediフィルタリング時間エラー", cur_account.username);
					pre_state->paraError = 9;
				}
				break;
			case 10:
				if (language == 0) {
					cJSON_AddStringToObject(error_json, "key", "AI滤波时间错误");
				}
				if (language == 1) {
					cJSON_AddStringToObject(error_json, "key", "AI filtering time error");
				}
				if (language == 2) {
					cJSON_AddStringToObject(error_json, "key", "aiは時間エラーをフィルタリングする");
				}
				if (pre_state->paraError != 10) {
					my_syslog("错误", "AI滤波时间错误", cur_account.username);
					my_en_syslog("error", "AI filtering time error", cur_account.username);
					my_jap_syslog("さくご", "aiは時間エラーをフィルタリングする", cur_account.username);
					pre_state->paraError = 10;
				}
				break;
			case 11:
				if (language == 0) {
					cJSON_AddStringToObject(error_json, "key", "AxleAI滤波时间错误");
				}
				if (language == 1) {
					cJSON_AddStringToObject(error_json, "key", "AxleAI filtering time error");
				}
				if (language == 2) {
					cJSON_AddStringToObject(error_json, "key", "axleaiは時間エラーをフィルタリングする");
				}
				if (pre_state->paraError != 11) {
					my_syslog("错误", "AxleAI滤波时间错误", cur_account.username);
					my_en_syslog("error", "AxleAI filtering time error", cur_account.username);
					my_jap_syslog("さくご", "axleaiは時間エラーをフィルタリングする", cur_account.username);
					pre_state->paraError = 11;
				}
				break;
			case 12:
				if (language == 0) {
					cJSON_AddStringToObject(error_json, "key", "DI高低电平范围错误");
				}
				if (language == 1) {
					cJSON_AddStringToObject(error_json, "key", "DI wrong range of high and low levels");
				}
				if (language == 2) {
					cJSON_AddStringToObject(error_json, "key", "diの高低の範囲が間違っている");
				}
				if (pre_state->paraError != 12) {
					my_syslog("错误", "DI高低电平范围错误", cur_account.username);
					my_en_syslog("error", "DI wrong range of high and low levels", cur_account.username);
					my_jap_syslog("さくご", "diの高低の範囲が間違っている", cur_account.username);
					pre_state->paraError = 12;
				}
				break;
			case 13:
				if (language == 0) {
					cJSON_AddStringToObject(error_json, "key", "DO高低电平范围错误");
				}
				if (language == 1) {
					cJSON_AddStringToObject(error_json, "key", "DO wrong range of high and low levels");
				}
				if (language == 2) {
					cJSON_AddStringToObject(error_json, "key", "do高低範囲が間違っている");
				}
				if (pre_state->paraError != 13) {
					my_syslog("错误", "DO高低电平范围错误", cur_account.username);
					my_en_syslog("error", "DO wrong range of high and low levels", cur_account.username);
					my_jap_syslog("さくご", "do高低範囲が間違っている", cur_account.username);
					pre_state->paraError = 13;
				}
				break;
			case 14:
				if (language == 0) {
					cJSON_AddStringToObject(error_json, "key", "工件号超限错误");
				}
				if (language == 1) {
					cJSON_AddStringToObject(error_json, "key", "Workpiece number out of limit error");
				}
				if (language == 2) {
					cJSON_AddStringToObject(error_json, "key", "工作物番号が間違っている");
				}
				if (pre_state->paraError != 14) {
					my_syslog("错误", "工件号超限错误", cur_account.username);
					my_en_syslog("error", "Workpiece number out of limit error", cur_account.username);
					my_jap_syslog("さくご", "工作物番号が間違っている", cur_account.username);
					pre_state->paraError = 14;
				}
				break;
			case 15:
				if (language == 0) {
					cJSON_AddStringToObject(error_json, "key", "外部轴号超限错误");
				}
				if (language == 1) {
					cJSON_AddStringToObject(error_json, "key", "External shaft number overrun error");
				}
				if (language == 2) {
					cJSON_AddStringToObject(error_json, "key", "外軸号がエラーオーバー");
				}
				if (pre_state->paraError != 15) {
					my_syslog("错误", "外部轴号超限错误", cur_account.username);
					my_en_syslog("error", "External shaft number overrun error", cur_account.username);
					my_jap_syslog("さくご", "外軸号がエラーオーバー", cur_account.username);
					pre_state->paraError = 15;
				}
				break;
			case 16:
				if (language == 0) {
					cJSON_AddStringToObject(error_json, "key", "传送带跟踪-编码器通道错误-可复位");
				}
				if (language == 1) {
					cJSON_AddStringToObject(error_json, "key", "Conveyor - encoder channel error - Reset");
				}
				if (language == 2) {
					cJSON_AddStringToObject(error_json, "key", "ベルトトラッカー-エンコーダチャンネルエラー-リセット可能");
				}
				if (pre_state->paraError != 16) {
					my_syslog("错误", "传送带跟踪-编码器通道错误-可复位", cur_account.username);
					my_en_syslog("error", "Conveyor - encoder channel error - Reset", cur_account.username);
					my_jap_syslog("さくご", "ベルトトラッカー-エンコーダチャンネルエラー-リセット可能", cur_account.username);
					pre_state->paraError = 16;
				}
				break;
			case 17:
				if (language == 0) {
					cJSON_AddStringToObject(error_json, "key", "传送带跟踪-工件轴号错误-可复位");
				}
				if (language == 1) {
					cJSON_AddStringToObject(error_json, "key", "Conveyor - Workpiece Axis Number error - Reset");
				}
				if (language == 2) {
					cJSON_AddStringToObject(error_json, "key", "ベルトコンベヤートラッキング-ワーク軸番号エラー-リセット可能");
				}
				if (pre_state->paraError != 17) {
					my_syslog("错误", "传送带跟踪-工件轴号错误-可复位", cur_account.username);
					my_en_syslog("error", "Conveyor - Workpiece Axis Number error - Reset", cur_account.username);
					my_jap_syslog("さくご", "ベルトコンベヤートラッキング-ワーク軸番号エラー-リセット可能", cur_account.username);
					pre_state->paraError = 17;
				}
				break;
			default:
				pre_state->paraError = 0;
				break;
		}
		//printf("state->exaxis_out_slimit_error = %d\n", state->exaxis_out_slimit_error);
		switch(state->exaxis_out_slimit_error)
		{
			case 1:
				if (language == 0) {
					cJSON_AddStringToObject(error_json, "key", "外部轴1轴超出软限位故障");
				}
				if (language == 1) {
					cJSON_AddStringToObject(error_json, "key", "External axis 1 axis out of soft limit fault");
				}
				if (language == 2) {
					cJSON_AddStringToObject(error_json, "key", "外軸1軸がソフトリミットを超えるトラブル");
				}
				if (pre_state->exaxis_out_slimit_error != 1) {
					my_syslog("错误", "外部轴1轴超出软限位故障", cur_account.username);
					my_en_syslog("error", "External axis 1 axis out of soft limit fault", cur_account.username);
					my_jap_syslog("さくご", "外軸1軸がソフトリミットを超えるトラブル", cur_account.username);
					pre_state->exaxis_out_slimit_error = 1;
				}
				break;
			case 2:
				if (language == 0) {
					cJSON_AddStringToObject(error_json, "key", "外部轴2轴超出软限位故障");
				}
				if (language == 1) {
					cJSON_AddStringToObject(error_json, "key", "External axis 2 axis out of soft limit fault");
				}
				if (language == 2) {
					cJSON_AddStringToObject(error_json, "key", "外軸2軸がソフトリミットを超えるトラブル");
				}
				if (pre_state->exaxis_out_slimit_error != 2) {
					my_syslog("错误", "外部轴2轴超出软限位故障", cur_account.username);
					my_en_syslog("error", "External axis 2 axis out of soft limit fault", cur_account.username);
					my_jap_syslog("さくご", "外軸2軸がソフトリミットを超えるトラブル", cur_account.username);
					pre_state->exaxis_out_slimit_error = 2;
				}
				break;
			case 3:
				if (language == 0) {
					cJSON_AddStringToObject(error_json, "key", "外部轴3轴超出软限位故障");
				}
				if (language == 1) {
					cJSON_AddStringToObject(error_json, "key", "External axis 3 axis out of soft limit fault");
				}
				if (language == 2) {
					cJSON_AddStringToObject(error_json, "key", "外軸3軸がソフトリミットを超えるトラブル");
				}
				if (pre_state->exaxis_out_slimit_error != 3) {
					my_syslog("错误", "外部轴3轴超出软限位故障", cur_account.username);
					my_en_syslog("error", "External axis 3 axis out of soft limit fault", cur_account.username);
					my_jap_syslog("さくご", "外軸3軸がソフトリミットを超えるトラブル", cur_account.username);
					pre_state->exaxis_out_slimit_error = 3;
				}
				break;
			case 4:
				if (language == 0) {
					cJSON_AddStringToObject(error_json, "key", "外部轴4轴超出软限位故障");
				}
				if (language == 1) {
					cJSON_AddStringToObject(error_json, "key", "External axis 4 axis out of soft limit fault");
				}
				if (language == 2) {
					cJSON_AddStringToObject(error_json, "key", "外軸4軸がソフトリミットを超えるトラブル");
				}
				if (pre_state->exaxis_out_slimit_error != 4) {
					my_syslog("错误", "外部轴4轴超出软限位故障", cur_account.username);
					my_en_syslog("error", "External axis 4 axis out of soft limit fault", cur_account.username);
					my_jap_syslog("さくご", "外軸4軸がソフトリミットを超えるトラブル", cur_account.username);
					pre_state->exaxis_out_slimit_error = 4;
				}
				break;
			default:
				pre_state->exaxis_out_slimit_error = 0;
				break;
		}
		if (state->dr_com_err == 1) {
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", "通信故障:控制器与驱动器心跳检测故障");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "Communication failure: controller and drive heartbeat detection failure");
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", "通信障害:コントローラとドライバの心拍検出に障害がある");
			}
			if (pre_state->dr_com_err != 1) {
				my_syslog("错误", "通信故障:控制器与驱动器心跳检测故障", cur_account.username);
				my_en_syslog("error", "Communication failure: controller and drive heartbeat detection failure", cur_account.username);
				my_jap_syslog("さくご", "通信障害:コントローラとドライバの心拍検出に障害がある", cur_account.username);
				pre_state->dr_com_err = 1;
			}
		} else {
			pre_state->dr_com_err = 0;
		}
		if ((int)state->dr_err != 0) {
			memset(content, 0, sizeof(content));
			memset(en_content, 0, sizeof(en_content));
			memset(jap_content, 0, sizeof(jap_content));
			sprintf(content, "%d 轴驱动器故障, 驱动器故障代码: %d", (int)state->dr_err, (int)state->dr_err_code);
			sprintf(en_content, "%d axis drive failure, drive failure code:%d", (int)state->dr_err, (int)state->dr_err_code);
			sprintf(jap_content, "%d シャフトドライブ障害ドライブ障害コード: %d", (int)state->dr_err, (int)state->dr_err_code);
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", content);
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", en_content);
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", jap_content);
			}
			if (pre_state->dr_err != 1) {
				my_syslog("错误", content, cur_account.username);
				my_en_syslog("error", en_content, cur_account.username);
				my_jap_syslog("さくご", jap_content, cur_account.username);
				pre_state->dr_err = 1;
			}
		} else {
			pre_state->dr_err = 0;
		}
		if ((int)state->out_sflimit_err != 0) {
			memset(content, 0, sizeof(content));
			memset(en_content, 0, sizeof(en_content));
			memset(jap_content, 0, sizeof(jap_content));
			sprintf(content, "%d 轴超出软限位故障, 可复位", (int)state->out_sflimit_err);
			sprintf(en_content, "%d axis out of soft limit fault, can be reset", (int)state->out_sflimit_err);
			sprintf(jap_content, "%d シャフトがソフトリミットを超えて故障する, リセット可能", (int)state->out_sflimit_err);
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", content);
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", en_content);
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", jap_content);
			}
			if (pre_state->out_sflimit_err != 1) {
				my_syslog("错误", content, cur_account.username);
				my_en_syslog("error", en_content, cur_account.username);
				my_jap_syslog("さくご", jap_content, cur_account.username);
				pre_state->out_sflimit_err = 1;
			}
		} else {
			pre_state->out_sflimit_err = 0;
		}
		if ((int)state->collision_err != 0) {
			memset(content, 0, sizeof(content));
			memset(en_content, 0, sizeof(en_content));
			memset(jap_content, 0, sizeof(jap_content));
			if ((int)state->collision_err == 7) {
				sprintf(content, "末端碰撞故障, 可复位", (int)state->collision_err);
				sprintf(en_content, "Terminal collision fault, can be reset", (int)state->collision_err);
				sprintf(jap_content, "末端衝突障害, リセット可能", (int)state->collision_err);
			} else {
				sprintf(content, "%d 轴碰撞故障, 可复位", (int)state->collision_err);
				sprintf(en_content, "%d axis impact fault, can be reset", (int)state->collision_err);
				sprintf(jap_content, "%d 軸衝突故障, リセット可能", (int)state->collision_err);
			}
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", content);
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", en_content);
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", jap_content);
			}
			if (pre_state->collision_err != 1) {
				my_syslog("错误", content, cur_account.username);
				my_en_syslog("error", en_content, cur_account.username);
				my_jap_syslog("さくご", jap_content, cur_account.username);
				pre_state->collision_err = 1;
			}
		} else {
			pre_state->collision_err = 0;
		}
		if (state->weld_readystate == 0) {
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", "焊机未准备好");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "The welder is not ready");
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", "溶接機が準備できていない");
			}
			if (pre_state->weld_readystate != 1) {
				my_syslog("错误", "焊机未准备好", cur_account.username);
				my_en_syslog("error", "The welder is not ready", cur_account.username);
				my_jap_syslog("さくご", "溶接機が準備できていない", cur_account.username);
				pre_state->weld_readystate = 1;
			}
		} else {
			pre_state->weld_readystate = 0;
		}
		for (i = 0; i < 4; i++) {
			if ((int)state->exaxis_status[i].exAxisALM == 1) {
				memset(content, 0, sizeof(content));
				memset(en_content, 0, sizeof(en_content));
				memset(jap_content, 0, sizeof(jap_content));
				sprintf(content, "外部轴 %d 伺服报警", (i+1));
				sprintf(en_content, "exaxis %d servo alarm", (i+1));
				sprintf(jap_content, "外部軸 %d サーボアラーム", (i+1));
				if (language == 0) {
					cJSON_AddStringToObject(error_json, "key", content);
				}
				if (language == 1) {
					cJSON_AddStringToObject(error_json, "key", en_content);
				}
				if (language == 2) {
					cJSON_AddStringToObject(error_json, "key", jap_content);
				}
				if (pre_state->exaxis_status[i].exAxisALM != 1) {
					my_syslog("错误", content, cur_account.username);
					my_en_syslog("error", en_content, cur_account.username);
					my_jap_syslog("さくご", jap_content, cur_account.username);
					pre_state->exaxis_status[i].exAxisALM = 1;
				}
			} else {
				pre_state->exaxis_status[i].exAxisALM = 0;
			}
		}
		for (i = 0; i < 4; i++) {
			if ((int)state->exaxis_status[i].exAxisFLERR == 1) {
				memset(content, 0, sizeof(content));
				memset(en_content, 0, sizeof(en_content));
				memset(jap_content, 0, sizeof(jap_content));
				sprintf(content, "外部轴 %d 跟随误差过大", (i+1));
				sprintf(en_content, "exaxis %d too much following error", (i+1));
				sprintf(jap_content, "外部軸 %d 追従誤差が大きすぎる", (i+1));
				if (language == 0) {
					cJSON_AddStringToObject(error_json, "key", content);
				}
				if (language == 1) {
					cJSON_AddStringToObject(error_json, "key", en_content);
				}
				if (language == 2) {
					cJSON_AddStringToObject(error_json, "key", jap_content);
				}
				if (pre_state->exaxis_status[i].exAxisFLERR != 1) {
					my_syslog("错误", content, cur_account.username);
					my_en_syslog("error", en_content, cur_account.username);
					my_jap_syslog("さくご", jap_content, cur_account.username);
					pre_state->exaxis_status[i].exAxisFLERR = 1;
				}
			} else {
				pre_state->exaxis_status[i].exAxisFLERR = 0;
			}
		}
		for (i = 0; i < 4; i++) {
			if ((int)state->exaxis_status[i].exAxisNLMT == 1) {
				memset(content, 0, sizeof(content));
				memset(en_content, 0, sizeof(en_content));
				memset(jap_content, 0, sizeof(jap_content));
				sprintf(content, "外部轴 %d 到负限位", (i+1));
				sprintf(en_content, "exaxis %d to the negative limit", (i+1));
				sprintf(jap_content, "外部軸 %d 負のリミットまで", (i+1));
				if (language == 0) { 
					cJSON_AddStringToObject(error_json, "key", content);
				}
				if (language == 1) {
					cJSON_AddStringToObject(error_json, "key", en_content);
				}
				if (language == 2) {
					cJSON_AddStringToObject(error_json, "key", jap_content);
				}
				if (pre_state->exaxis_status[i].exAxisNLMT != 1) {
					my_syslog("错误", content, cur_account.username);
					my_en_syslog("error", en_content, cur_account.username);
					my_jap_syslog("さくご", jap_content, cur_account.username);
					pre_state->exaxis_status[i].exAxisNLMT = 1;
				}
			} else {
				pre_state->exaxis_status[i].exAxisNLMT = 0;
			}
		}
		for (i = 0; i < 4; i++) {
			if ((int)state->exaxis_status[i].exAxisPLMT == 1) {
				memset(content, 0, sizeof(content));
				memset(en_content, 0, sizeof(en_content));
				memset(jap_content, 0, sizeof(jap_content));
				sprintf(content, "外部轴 %d 到正限位", (i+1));
				sprintf(en_content, "exaxis %d to the forward limit", (i+1));
				sprintf(jap_content, "外部軸 %d 正のリミットまで", (i+1));
				if (language == 0) {
					cJSON_AddStringToObject(error_json, "key", content);
				}
				if (language == 1) {
					cJSON_AddStringToObject(error_json, "key", en_content);
				}
				if (language == 2) {
					cJSON_AddStringToObject(error_json, "key", jap_content);
				}
				if (pre_state->exaxis_status[i].exAxisPLMT != 1) {
					my_syslog("错误", content, cur_account.username);
					my_en_syslog("error", en_content, cur_account.username);
					my_jap_syslog("さくご", jap_content, cur_account.username);
					pre_state->exaxis_status[i].exAxisPLMT = 1;
				}
			} else {
				pre_state->exaxis_status[i].exAxisPLMT = 0;
			}
		}
		for (i = 0; i < 4; i++) {
			if ((int)state->exaxis_status[i].exAxisAbsOFLN == 1) {
				memset(content, 0, sizeof(content));
				memset(en_content, 0, sizeof(en_content));
				memset(jap_content, 0, sizeof(jap_content));
				sprintf(content, "外部轴 %d 驱动器485总线掉线", (i+1));
				sprintf(en_content, "exaxis %d the driver 485 bus is disconnected", (i+1));
				sprintf(jap_content, "外部軸 %d ドライバ485バスが中断される", (i+1));
				if (language == 0) {
					cJSON_AddStringToObject(error_json, "key", content);
				}
				if (language == 1) {
					cJSON_AddStringToObject(error_json, "key", en_content);
				}
				if (language == 2) {
					cJSON_AddStringToObject(error_json, "key", jap_content);
				}
				if (pre_state->exaxis_status[i].exAxisAbsOFLN != 1) {
					my_syslog("错误", content, cur_account.username);
					my_en_syslog("error", en_content, cur_account.username);
					my_jap_syslog("さくご", jap_content, cur_account.username);
					pre_state->exaxis_status[i].exAxisAbsOFLN = 1;
				}
			} else {
				pre_state->exaxis_status[i].exAxisAbsOFLN = 0;
			}
		}
		for (i = 0; i < 4; i++) {
			if ((int)state->exaxis_status[i].exAxisOFLIN == 1) {
				memset(content, 0, sizeof(content));
				memset(en_content, 0, sizeof(en_content));
				memset(jap_content, 0, sizeof(jap_content));
				sprintf(content, "外部轴 %d 通信超时，控制卡与控制箱板485通信超时", (i+1));
				sprintf(en_content, "exaxis %d communication timeout, control card and control box board 485 communication timeout", (i+1));
				sprintf(jap_content, "外部軸 %d 通信タイムアウト制御カードと制御ボックス基板485との通信タイムアウト", (i+1));
				if (language == 0) {
					cJSON_AddStringToObject(error_json, "key", content);
				}
				if (language == 1) {
					cJSON_AddStringToObject(error_json, "key", en_content);
				}
				if (language == 2) {
					cJSON_AddStringToObject(error_json, "key", jap_content);
				}
				if (pre_state->exaxis_status[i].exAxisOFLIN != 1) {
					my_syslog("错误", content, cur_account.username);
					my_en_syslog("error", en_content, cur_account.username);
					my_jap_syslog("さくご", jap_content, cur_account.username);
					pre_state->exaxis_status[i].exAxisOFLIN = 1;
				}
			} else {
				pre_state->exaxis_status[i].exAxisOFLIN = 0;
			}
		}
		if (state->alarm_check_emerg_stop_btn == 1) {
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", "通信异常,检查急停按钮是否松开");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "Warning: abnormal communication, check whether the emergency stop button is loosened");
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", "通信異常、急停止ボタンが外れているかチェック");
			}
			if (pre_state->alarm_check_emerg_stop_btn != 1) {
				my_syslog("错误", "通信异常,检查急停按钮是否松开", cur_account.username);
				my_en_syslog("error", "Warning: abnormal communication, check whether the emergency stop button is released", cur_account.username);
				my_jap_syslog("さくご", "通信異常、急停止ボタンが外れているかチェック", cur_account.username);
				pre_state->alarm_check_emerg_stop_btn = 1;
			}
		} else {
			pre_state->alarm_check_emerg_stop_btn = 0;
		}
		if (state->alarm_reboot_robot == 1) {
			if (language == 0) { 
				cJSON_AddStringToObject(error_json, "key", "断电重启机器人");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "Power off and restart the robot");
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", "電源を切ってロボットを再起動する");
			}
			if (pre_state->alarm_reboot_robot != 1) {
				my_syslog("错误", "断电重启机器人", cur_account.username);
				my_en_syslog("error", "Power off and restart the robot", cur_account.username);
				my_jap_syslog("さくご", "電源を切ってロボットを再起動す", cur_account.username);
				pre_state->alarm_reboot_robot = 1;
			}
		} else {
			pre_state->alarm_reboot_robot = 0;
		}
		if (state->ts_web_state_com_error == 1) {
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", "扭矩：WEB-TM 状态反馈，通信失败");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "Torque: WEB-TM state feedback, communication failure");
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", "トルク:web-tm状態フィードバック、通信失敗");
			}
			if (pre_state->ts_web_state_com_error != 1) {
				my_syslog("错误", "扭矩：WEB-TM 状态反馈，通信失败", cur_account.username);
				my_en_syslog("error", "Torque: WEB-TM state feedback, communication failure", cur_account.username);
				my_jap_syslog("さくご", "トルク:web-tm状態フィードバック、通信失敗", cur_account.username);
				pre_state->ts_web_state_com_error = 1;
			}
		} else {
			pre_state->ts_web_state_com_error = 0;
		}
		if (state->ts_tm_cmd_com_error == 1) {
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", "扭矩：TM-扭矩 指令下发，通信失败");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "Torque: TM- Torque command issued, communication failed");
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", "トルク:tm -トルク指令が出され、通信が失敗する");
			}
			if (pre_state->ts_tm_cmd_com_error != 1) {
				my_syslog("错误", "扭矩：TM-扭矩 指令下发，通信失败", cur_account.username);
				my_en_syslog("error", "Torque: TM- Torque command issued, communication failed", cur_account.username);
				my_jap_syslog("さくご", "トルク:tm -トルク指令が出され、通信が失敗する", cur_account.username);
				pre_state->ts_tm_cmd_com_error = 1;
			}
		} else {
			pre_state->ts_tm_cmd_com_error = 0;
		}
		if (state->ts_tm_state_com_error == 1) {
			if (language == 0) {
				cJSON_AddStringToObject(error_json, "key", "扭矩：TM-扭矩 状态反馈，通信失败");
			}
			if (language == 1) {
				cJSON_AddStringToObject(error_json, "key", "Torque: TM- Torque status feedback, communication failure");
			}
			if (language == 2) {
				cJSON_AddStringToObject(error_json, "key", "トルク:tm -トルク状態フィードバック、通信失敗");
			}
			if (pre_state->ts_tm_state_com_error != 1) {
				my_syslog("错误", "扭矩：TM-扭矩 状态反馈，通信失败", cur_account.username);
				my_en_syslog("error", "Torque: TM- Torque status feedback, communication failure", cur_account.username);
				my_jap_syslog("さくご", "トルク:tm -トルク状態フィードバック、通信失敗", cur_account.username);
				pre_state->ts_tm_state_com_error = 1;
			}
		} else {
			pre_state->ts_tm_state_com_error = 0;
		}
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

