
/********************************* Includes ***********************************/

#include    "goahead.h"
#include	"cJSON.h"
#include 	"tools.h"
#include 	"robot_socket.h"
#include    "action_act.h"
#include 	"mysqlite3.h"

/********************************* Defines ************************************/

extern pthread_t t_socket_pi_status;   /** PI 状态反馈线程*/
extern pthread_t t_socket_pi_cmd;		/** PI 指令下发线程*/
extern PI_PTHREAD pi_pt_status;   /** PI 状态反馈线程结构体 */
extern PI_PTHREAD pi_pt_cmd;		/** PI 指令下发线程结构体 */

extern SOCKET_PI_INFO socket_pi_status;
extern SOCKET_PI_INFO socket_pi_cmd;
extern CTRL_STATE ctrl_state;
extern CTRL_STATE vir_ctrl_state;
extern int robot_type;
extern int language;
extern ACCOUNT_INFO cur_account;
extern SOCKET_INFO socket_cmd;
extern SOCKET_INFO socket_vir_cmd;
extern int robot_type;
extern POINT_HOME_INFO point_home_info;
extern JIABAO_TORQUE_PRODUCTION_DATA jiabao_torque_pd_data;
extern SERVER_IP[20];
extern SERVER_PI_IP[20];
extern WEBAPP_SYSCFG web_cfg;

/********************************* Function declaration ***********************/

static int save_lua_file(const cJSON *data_json);
static int remove_lua_file(const cJSON *data_json);
static int save_template_file(const cJSON *data_json);
static int remove_template_file(const cJSON *data_json);
static int rename_lua_file(const cJSON *data_json);
static int modify_tool_cdsystem(const cJSON *data_json);
static int modify_wobj_tool_cdsystem(const cJSON *data_json);
static int modify_ex_tool_cdsystem(const cJSON *data_json);
static int modify_exaxis_cdsystem(const cJSON *data_json);
static int save_point(const cJSON *data_json);
static int save_laser_point(const cJSON *data_json);
static int modify_point(const cJSON *data_json);
static int remove_points(const cJSON *data_json);
static int change_type(const cJSON *data_json);
static int set_sys_logcount(const cJSON *data_json);
static int set_sys_lifespan(const cJSON *data_json);
static int set_sys_language(const cJSON *data_json);
static int set_ODM_cfg(const cJSON *data_json);
static int set_ptn_cfg(const cJSON *data_json);
static int save_accounts(const cJSON *data_json);
static int shutdown_system(const cJSON *data_json);
static int plugin_enable(const cJSON *data_json);
static int plugin_remove(const cJSON *data_json);
static int clear_DH_file(const cJSON *data_json);
static int save_DH_point(const cJSON *data_json);
static int factory_reset(const cJSON *data_json);
static int odm_password(const cJSON *data_json);
static int save_robot_type(const cJSON *data_json);
static int torque_save_cfg(const cJSON *data_json);
static int generate_luafile(const cJSON *data_json);
static int torque_ensure_points(const cJSON *data_json);
static int set_DIO_cfg(const cJSON *data_json);
static int set_status_flag(const cJSON *data_json);
static int rename_var(const cJSON *data_json);
static int clear_product_info(const cJSON *data_json);
static int move_to_home_point(const cJSON *data_json);
static int torque_generate_program(const cJSON *data_json);
static int torque_save_custom_pause(const cJSON *data_json);
static int modify_network(const cJSON *data_json);
static int modify_customcfg(const cJSON *data_json);
static int save_blockly_workspace(const cJSON *data_json);
static int modify_PI_cfg(const cJSON *data_json);
static int robottype_password(const cJSON *data_json);

/*********************************** Code *************************************/

/* save lua file */
static int save_lua_file(const cJSON *data_json)
{
	char dir_filename[100] = {0};

	cJSON *file_name = cJSON_GetObjectItem(data_json, "name");
	cJSON *pgvalue = cJSON_GetObjectItem(data_json, "pgvalue");
	if (file_name == NULL || pgvalue == NULL || file_name->valuestring == NULL || pgvalue->valuestring == NULL) {
		perror("json");

		return FAIL;
	}
	sprintf(dir_filename, "%s%s", DIR_USER, file_name->valuestring);

	return write_file(dir_filename, pgvalue->valuestring);
}

/* remove lua file */
static int remove_lua_file(const cJSON *data_json)
{
	char dir_filename[100] = {0};

	cJSON *name = cJSON_GetObjectItem(data_json, "name");
	if (name == NULL || name->valuestring == NULL) {
		perror("json");

		return FAIL;
	}
	sprintf(dir_filename, "%s%s", DIR_USER, name->valuestring);
	if (remove(dir_filename) == -1) {
		perror("remove");

		return FAIL;
	}

	return SUCCESS;
}

/* save template file */
static int save_template_file(const cJSON *data_json)
{
	char dir_filename[100] = {0};

	cJSON *file_name = cJSON_GetObjectItem(data_json, "name");
	cJSON *pgvalue = cJSON_GetObjectItem(data_json, "pgvalue");
	if (file_name == NULL || pgvalue == NULL || file_name->valuestring == NULL || pgvalue->valuestring == NULL) {
		perror("json");

		return FAIL;
	}
	sprintf(dir_filename, "%s%s", DIR_TEMPLATE, file_name->valuestring);

	return write_file(dir_filename, pgvalue->valuestring);
}

/* remove template file */
static int remove_template_file(const cJSON *data_json)
{
	char dir_filename[100] = {0};

	cJSON *name = cJSON_GetObjectItem(data_json, "name");
	if (name == NULL || name->valuestring == NULL) {
		perror("json");

		return FAIL;
	}
	sprintf(dir_filename, "%s%s", DIR_TEMPLATE, name->valuestring);
	if (remove(dir_filename) == -1) {
		perror("remove");

		return FAIL;
	}

	return SUCCESS;
}

/* rename lua file */
static int rename_lua_file(const cJSON *data_json)
{
	char old_filename[100] = {0};
	char new_filename[100] = {0};

	cJSON *oldname = cJSON_GetObjectItem(data_json, "oldname");
	cJSON *newname = cJSON_GetObjectItem(data_json, "newname");
	if (oldname == NULL || newname == NULL || oldname->valuestring == NULL || newname->valuestring == NULL) {
		perror("json");

		return FAIL;
	}
	sprintf(old_filename, "%s%s", DIR_USER, oldname->valuestring);
	sprintf(new_filename, "%s%s", DIR_USER, newname->valuestring);
	if (rename(old_filename, new_filename) == -1) {
		perror("rename");

		return FAIL;
	}

	return SUCCESS;
}

/* modify tool cdsystem */
static int modify_tool_cdsystem(const cJSON *data_json)
{
	char sql[1204] = {0};
	cJSON *name = NULL;
	cJSON *id = NULL;
	cJSON *x = NULL;
	cJSON *y = NULL;
	cJSON *z = NULL;
	cJSON *rx = NULL;
	cJSON *ry = NULL;
	cJSON *rz = NULL;
	cJSON *type = NULL;
	cJSON *installation_site = NULL;

	name = cJSON_GetObjectItem(data_json, "name");
	id = cJSON_GetObjectItem(data_json, "id");
	x = cJSON_GetObjectItem(data_json, "x");
	y = cJSON_GetObjectItem(data_json, "y");
	z = cJSON_GetObjectItem(data_json, "z");
	rx = cJSON_GetObjectItem(data_json, "rx");
	ry = cJSON_GetObjectItem(data_json, "ry");
	rz = cJSON_GetObjectItem(data_json, "rz");
	type = cJSON_GetObjectItem(data_json, "type");
	installation_site = cJSON_GetObjectItem(data_json, "installation_site");
	if (name == NULL || id == NULL || x == NULL || y == NULL || z == NULL || rx == NULL || ry == NULL || rz == NULL || type == NULL || installation_site == NULL || name->valuestring == NULL || id->valuestring == NULL || x->valuestring == NULL || y->valuestring == NULL || z->valuestring == NULL || rx->valuestring == NULL || ry->valuestring == NULL || rz->valuestring == NULL) {
		perror("json");

		return FAIL;
	}

	//sprintf(sql, "insert into coordinate_system(name,id,x,y,z,rx,ry,rz,type,installation_site) values('%s','%s','%s','%s','%s','%s','%s','%s','%s','%s');", name->valuestring, id->valuestring, x->valuestring, y->valuestring, z->valuestring, rx->valuestring, ry->valuestring, rz->valuestring, type->valuestring, installation_site->valuestring);
	sprintf(sql, "update coordinate_system set name='%s', id='%s', x='%s', y='%s', z='%s', rx='%s', ry='%s', rz='%s', type='%s', installation_site='%s' where id='%s';", name->valuestring, id->valuestring, x->valuestring, y->valuestring, z->valuestring, rx->valuestring, ry->valuestring, rz->valuestring, type->valuestring, installation_site->valuestring, id->valuestring);

	if (change_info_sqlite3(DB_CDSYSTEM, sql) == -1) {
		perror("database");

		return FAIL;
	}

	return SUCCESS;
}

/* modify wobj tool cdsystem */
static int modify_wobj_tool_cdsystem(const cJSON *data_json)
{
	char sql[1204] = {0};
	cJSON *name = NULL;
	cJSON *id = NULL;
	cJSON *x = NULL;
	cJSON *y = NULL;
	cJSON *z = NULL;
	cJSON *rx = NULL;
	cJSON *ry = NULL;
	cJSON *rz = NULL;

	name = cJSON_GetObjectItem(data_json, "name");
	id = cJSON_GetObjectItem(data_json, "id");
	x = cJSON_GetObjectItem(data_json, "x");
	y = cJSON_GetObjectItem(data_json, "y");
	z = cJSON_GetObjectItem(data_json, "z");
	rx = cJSON_GetObjectItem(data_json, "rx");
	ry = cJSON_GetObjectItem(data_json, "ry");
	rz = cJSON_GetObjectItem(data_json, "rz");
	if(name == NULL || id == NULL || x == NULL || y == NULL || z == NULL || rx == NULL || ry == NULL|| rz == NULL || name->valuestring == NULL || id->valuestring == NULL || x->valuestring == NULL || y->valuestring == NULL || z->valuestring == NULL || rx->valuestring == NULL || ry->valuestring == NULL || rz->valuestring == NULL) {
		perror("json");

		return FAIL;
	}

	sprintf(sql, "update wobj_coordinate_system set name='%s', id='%s', x='%s', y='%s', z='%s', rx='%s', ry='%s', rz='%s' where id='%s';", name->valuestring, id->valuestring, x->valuestring, y->valuestring, z->valuestring, rx->valuestring, ry->valuestring, rz->valuestring, id->valuestring);
	//sprintf(sql, "insert into wobj_coordinate_system(name,id,x,y,z,rx,ry,rz) values('%s','%s','%s','%s','%s','%s','%s','%s');", name->valuestring, id->valuestring, x->valuestring, y->valuestring, z->valuestring, rx->valuestring, ry->valuestring, rz->valuestring);

	if (change_info_sqlite3(DB_WOBJ_CDSYSTEM, sql) == -1) {
		perror("database");

		return FAIL;
	}

	return SUCCESS;
}

/* modify ex && tool cdsystem */
static int modify_ex_tool_cdsystem(const cJSON *data_json)
{
	char sql[1024] = {0};
	cJSON *name = NULL;
	cJSON *user_name = NULL;
	cJSON *id = NULL;
	cJSON *ex = NULL;
	cJSON *ey = NULL;
	cJSON *ez = NULL;
	cJSON *erx = NULL;
	cJSON *ery = NULL;
	cJSON *erz = NULL;
	cJSON *tx = NULL;
	cJSON *ty = NULL;
	cJSON *tz = NULL;
	cJSON *trx = NULL;
	cJSON *try = NULL;
	cJSON *trz = NULL;

	name = cJSON_GetObjectItem(data_json, "name");
	user_name = cJSON_GetObjectItem(data_json, "user_name");
	id = cJSON_GetObjectItem(data_json, "id");
	ex = cJSON_GetObjectItem(data_json, "ex");
	ey = cJSON_GetObjectItem(data_json, "ey");
	ez = cJSON_GetObjectItem(data_json, "ez");
	erx = cJSON_GetObjectItem(data_json, "erx");
	ery = cJSON_GetObjectItem(data_json, "ery");
	erz = cJSON_GetObjectItem(data_json, "erz");
	tx = cJSON_GetObjectItem(data_json, "tx");
	ty = cJSON_GetObjectItem(data_json, "ty");
	tz = cJSON_GetObjectItem(data_json, "tz");
	trx = cJSON_GetObjectItem(data_json, "trx");
	try = cJSON_GetObjectItem(data_json, "try");
	trz = cJSON_GetObjectItem(data_json, "trz");
	if(name == NULL || user_name == NULL || id == NULL || ex == NULL || ey == NULL || ez == NULL || erx == NULL || ery == NULL|| erz == NULL || tx == NULL || ty == NULL || tz == NULL || trx == NULL || try == NULL|| trz == NULL || name->valuestring == NULL || user_name->valuestring == NULL || id->valuestring == NULL || ex->valuestring == NULL || ey->valuestring == NULL || ez->valuestring == NULL || erx->valuestring == NULL || ery->valuestring == NULL || erz->valuestring == NULL || tx->valuestring == NULL || ty->valuestring == NULL || tz->valuestring == NULL || trx->valuestring == NULL || try->valuestring == NULL || trz->valuestring == NULL) {
		perror("json");

		return FAIL;
	}

	sprintf(sql, "update et_coordinate_system set name='%s', user_name='%s', id='%s', ex='%s', ey='%s', ez='%s', erx='%s', ery='%s', erz='%s', tx='%s', ty='%s', tz='%s', trx='%s', try='%s', trz='%s' where id='%s';", name->valuestring, user_name->valuestring, id->valuestring, ex->valuestring, ey->valuestring, ez->valuestring, erx->valuestring, ery->valuestring, erz->valuestring, tx->valuestring, ty->valuestring, tz->valuestring, trx->valuestring, try->valuestring, trz->valuestring, id->valuestring);
	//sprintf(sql, "insert into et_coordinate_system(name,user_name,id,ex,ey,ez,erx,ery,erz,tx,ty,tz,trx,try,trz) values('%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s');", name->valuestring, user_name->valuestring, id->valuestring, ex->valuestring, ey->valuestring, ez->valuestring, erx->valuestring, ery->valuestring, erz->valuestring, tx->valuestring, ty->valuestring, tz->valuestring, trx->valuestring, try->valuestring, trz->valuestring);

	if (change_info_sqlite3(DB_ET_CDSYSTEM, sql) == -1) {
		perror("database");

		return FAIL;
	}

	return SUCCESS;
}

/* modify exaxis cdsystem */
static int modify_exaxis_cdsystem(const cJSON *data_json)
{
	char sql[1204] = {0};
	cJSON *name = NULL;
	cJSON *exaxisid = NULL;
	cJSON *id = NULL;
	cJSON *x = NULL;
	cJSON *y = NULL;
	cJSON *z = NULL;
	cJSON *rx = NULL;
	cJSON *ry = NULL;
	cJSON *rz = NULL;
	cJSON *flag = NULL;

	name = cJSON_GetObjectItem(data_json, "name");
	exaxisid = cJSON_GetObjectItem(data_json, "exaxisid");
	id = cJSON_GetObjectItem(data_json, "id");
	x = cJSON_GetObjectItem(data_json, "x");
	y = cJSON_GetObjectItem(data_json, "y");
	z = cJSON_GetObjectItem(data_json, "z");
	rx = cJSON_GetObjectItem(data_json, "rx");
	ry = cJSON_GetObjectItem(data_json, "ry");
	rz = cJSON_GetObjectItem(data_json, "rz");
	flag = cJSON_GetObjectItem(data_json, "flag");
	if(name == NULL || exaxisid == NULL || id == NULL || x == NULL || y == NULL || z == NULL || rx == NULL || ry == NULL || rz == NULL || flag == NULL || name->valuestring == NULL || exaxisid->valuestring == NULL || id->valuestring == NULL || x->valuestring == NULL || y->valuestring == NULL || z->valuestring == NULL || rx->valuestring == NULL || ry->valuestring == NULL || rz->valuestring == NULL || flag->valuestring == NULL) {
		perror("json");
 
		return FAIL;
	}

	sprintf(sql, "update exaxis_coordinate_system set name='%s', exaxisid='%s', id='%s', x='%s', y='%s', z='%s', rx='%s', ry='%s', rz='%s', flag='%s' where id='%s';", name->valuestring, exaxisid->valuestring, id->valuestring, x->valuestring, y->valuestring, z->valuestring, rx->valuestring, ry->valuestring, rz->valuestring, flag->valuestring, id->valuestring);
	//sprintf(sql, "insert into exaxis_coordinate_system(name,exaxisid,id,x,y,z,rx,ry,rz,flag) values ('%s','%s','%s','%s','%s','%s','%s','%s','%s','%s');", name->valuestring, exaxisid->valuestring, id->valuestring, x->valuestring, y->valuestring, z->valuestring, rx->valuestring, ry->valuestring, rz->valuestring, flag->valuestring);

	if (change_info_sqlite3(DB_EXAXIS_CDSYSTEM, sql) == -1) {
		perror("database");

		return FAIL;
	}

	return SUCCESS;
}

/* save point */
static int save_point(const cJSON *data_json)
{
	CTRL_STATE *state = NULL;
	char sql[1024] = {0};
	int i = 0;
	char E1[20] = {0};
	char E2[20] = {0};
	char E3[20] = {0};
	char E4[20] = {0};
	char tcp_value_string[6][10] = {0};
	char joint_value_string[6][10] = {0};
	cJSON *name = NULL;
	cJSON *speed = NULL;
	cJSON *elbow_speed = NULL;
	cJSON *acc = NULL;
	cJSON *elbow_acc = NULL;
	cJSON *toolnum = NULL;
	cJSON *workpiecenum = NULL;
	cJSON *j1 = NULL;
	cJSON *j2 = NULL;
	cJSON *j3 = NULL;
	cJSON *j4 = NULL;
	cJSON *j5 = NULL;
	cJSON *j6 = NULL;
	cJSON *x = NULL;
	cJSON *y = NULL;
	cJSON *z = NULL;
	cJSON *rx = NULL;
	cJSON *ry = NULL;
	cJSON *rz = NULL;
	char content[MAX_BUF] = "";
	SOCKET_INFO *sock_cmd = NULL;

	if (robot_type == 1) { // "1" 代表实体机器人
		sock_cmd = &socket_cmd;
		state = &ctrl_state;
	} else { // "0" 代表虚拟机器人
		sock_cmd = &socket_vir_cmd;
		state = &vir_ctrl_state;
	}
	name = cJSON_GetObjectItem(data_json, "name");
	speed = cJSON_GetObjectItem(data_json, "speed");
	acc = cJSON_GetObjectItem(data_json, "acc");
	elbow_speed = cJSON_GetObjectItem(data_json, "elbow_speed");
	elbow_acc = cJSON_GetObjectItem(data_json, "elbow_acc");
	toolnum = cJSON_GetObjectItem(data_json, "toolnum");
	workpiecenum = cJSON_GetObjectItem(data_json, "workpiecenum");
	if (name == NULL || speed == NULL || acc == NULL || elbow_speed == NULL || elbow_acc == NULL || toolnum == NULL || workpiecenum == NULL || name->valuestring == NULL || speed->valuestring == NULL || acc->valuestring == NULL || elbow_speed->valuestring == NULL || elbow_acc->valuestring == NULL || toolnum->valuestring == NULL || workpiecenum->valuestring == NULL) {
		perror("json");

		return FAIL;
	}
	for (i = 0; i < 6; i++) {
		sprintf(joint_value_string[i], "%.3lf", state->jt_cur_pos[i]);
	}
	for (i = 0; i < 6; i++) {
		sprintf(tcp_value_string[i], "%.3lf", state->tl_cur_pos[i]);
	}
	sprintf(E1, "%.3lf", state->exaxis_status[0].exAxisPos);
	sprintf(E2, "%.3lf", state->exaxis_status[1].exAxisPos);
	sprintf(E3, "%.3lf", state->exaxis_status[2].exAxisPos);
	sprintf(E4, "%.3lf", state->exaxis_status[3].exAxisPos);
	sprintf(sql, "insert into points(name,speed,elbow_speed,acc,elbow_acc,toolnum,workpiecenum,j1,j2,j3,j4,j5,j6,E1,E2,E3,E4,x,y,z,rx,ry,rz) values ('%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s');"\
				, name->valuestring, speed->valuestring, elbow_speed->valuestring,\
				acc->valuestring, elbow_acc->valuestring, toolnum->valuestring, workpiecenum->valuestring,\
				joint_value_string[0], joint_value_string[1], joint_value_string[2], joint_value_string[3], joint_value_string[4], joint_value_string[5], E1, E2, E3, E4,\
				tcp_value_string[0], tcp_value_string[1], tcp_value_string[2], tcp_value_string[3], tcp_value_string[4], tcp_value_string[5]);
	if (change_info_sqlite3(DB_POINTS, sql) == -1) {
		perror("database");

		return FAIL;
	}

	if (strcmp(name->valuestring, POINT_HOME) == 0) {
		point_home_info.error_flag = 0;
		sprintf(content, "SetRobotWorkHomePoint(%s,%s,%s,%s,%s,%s)", joint_value_string[0], joint_value_string[1], joint_value_string[2], joint_value_string[3], joint_value_string[4], joint_value_string[5]);
		socket_enquene(sock_cmd, 428, content, 1);
	}

	return SUCCESS;
}

/* save laserpoint */
static int save_laser_point(const cJSON *data_json)
{
	char sql[1024] = {0};
	cJSON *name = NULL;
	cJSON *speed = NULL;
	cJSON *elbow_speed = NULL;
	cJSON *acc = NULL;
	cJSON *elbow_acc = NULL;
	cJSON *toolnum = NULL;
	cJSON *workpiecenum = NULL;
	cJSON *j1 = NULL;
	cJSON *j2 = NULL;
	cJSON *j3 = NULL;
	cJSON *j4 = NULL;
	cJSON *j5 = NULL;
	cJSON *j6 = NULL;
	cJSON *x = NULL;
	cJSON *y = NULL;
	cJSON *z = NULL;
	cJSON *rx = NULL;
	cJSON *ry = NULL;
	cJSON *rz = NULL;
	cJSON *E1_json = NULL;
	cJSON *E2_json = NULL;
	cJSON *E3_json = NULL;
	cJSON *E4_json = NULL;

	name = cJSON_GetObjectItem(data_json, "name");
	speed = cJSON_GetObjectItem(data_json, "speed");
	acc = cJSON_GetObjectItem(data_json, "acc");
	elbow_speed = cJSON_GetObjectItem(data_json, "elbow_speed");
	elbow_acc = cJSON_GetObjectItem(data_json, "elbow_acc");
	toolnum = cJSON_GetObjectItem(data_json, "toolnum");
	workpiecenum = cJSON_GetObjectItem(data_json, "workpiecenum");
	j1 = cJSON_GetObjectItem(data_json, "j1");
	j2 = cJSON_GetObjectItem(data_json, "j2");
	j3 = cJSON_GetObjectItem(data_json, "j3");
	j4 = cJSON_GetObjectItem(data_json, "j4");
	j5 = cJSON_GetObjectItem(data_json, "j5");
	j6 = cJSON_GetObjectItem(data_json, "j6");
	x = cJSON_GetObjectItem(data_json, "x");
	y = cJSON_GetObjectItem(data_json, "y");
	z = cJSON_GetObjectItem(data_json, "z");
	rx = cJSON_GetObjectItem(data_json, "rx");
	ry = cJSON_GetObjectItem(data_json, "ry");
	rz = cJSON_GetObjectItem(data_json, "rz");
	E1_json = cJSON_GetObjectItem(data_json, "E1");
	E2_json = cJSON_GetObjectItem(data_json, "E2");
	E3_json = cJSON_GetObjectItem(data_json, "E3");
	E4_json = cJSON_GetObjectItem(data_json, "E4");

	if (name == NULL || speed == NULL || acc == NULL || elbow_speed == NULL || elbow_acc == NULL || toolnum == NULL || workpiecenum == NULL || j1 == NULL || j2 == NULL || j3 == NULL || j4 == NULL || j5 == NULL || j6 == NULL || x == NULL || y == NULL || z == NULL|| rx == NULL || ry == NULL || rz == NULL || E1_json == NULL || E2_json == NULL || E3_json == NULL || E4_json == NULL || name->valuestring == NULL || speed->valuestring == NULL || acc->valuestring == NULL || elbow_speed->valuestring == NULL || elbow_acc->valuestring == NULL || toolnum->valuestring == NULL || workpiecenum->valuestring == NULL || j1->valuestring == NULL || j2->valuestring == NULL || j3->valuestring == NULL || j4->valuestring == NULL || j5->valuestring == NULL || j6->valuestring == NULL || x->valuestring == NULL || y->valuestring == NULL || z->valuestring == NULL|| rx->valuestring == NULL || ry->valuestring == NULL || rz->valuestring == NULL || E1_json->valuestring == NULL || E2_json->valuestring == NULL || E3_json->valuestring == NULL || E4_json->valuestring == NULL ) {
		perror("json");

		return FAIL;
	}
	sprintf(sql, "insert into points(name,speed,elbow_speed,acc,elbow_acc,toolnum,workpiecenum,j1,j2,j3,j4,j5,j6,E1,E2,E3,E4,x,y,z,rx,ry,rz) values ('%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s');"\
				, name->valuestring, speed->valuestring, elbow_speed->valuestring,\
				acc->valuestring, elbow_acc->valuestring, toolnum->valuestring, workpiecenum->valuestring,\
				j1->valuestring, j2->valuestring, j3->valuestring, j4->valuestring, j5->valuestring, j6->valuestring, E1_json->valuestring, E2_json->valuestring, E3_json->valuestring, E4_json->valuestring,\
				x->valuestring, y->valuestring, z->valuestring, rx->valuestring, ry->valuestring, rz->valuestring);
	if (change_info_sqlite3(DB_POINTS, sql) == -1) {
		perror("database");

		return FAIL;
	}

	return SUCCESS;
}

/* modify point */
static int modify_point(const cJSON *data_json)
{
	char sql[1024] = {0};
	cJSON *name = NULL;
	cJSON *speed = NULL;
	cJSON *elbow_speed = NULL;
	cJSON *acc = NULL;
	cJSON *elbow_acc = NULL;
	cJSON *toolnum = NULL;
	cJSON *workpiecenum = NULL;
	cJSON *j1 = NULL;
	cJSON *j2 = NULL;
	cJSON *j3 = NULL;
	cJSON *j4 = NULL;
	cJSON *j5 = NULL;
	cJSON *j6 = NULL;
	cJSON *x = NULL;
	cJSON *y = NULL;
	cJSON *z = NULL;
	cJSON *rx = NULL;
	cJSON *ry = NULL;
	cJSON *rz = NULL;
	cJSON *E1 = NULL;
	cJSON *E2 = NULL;
	cJSON *E3 = NULL;
	cJSON *E4 = NULL;

	name = cJSON_GetObjectItem(data_json, "name");
	speed = cJSON_GetObjectItem(data_json, "speed");
	acc = cJSON_GetObjectItem(data_json, "acc");
	elbow_speed = cJSON_GetObjectItem(data_json, "elbow_speed");
	elbow_acc = cJSON_GetObjectItem(data_json, "elbow_acc");
	toolnum = cJSON_GetObjectItem(data_json, "toolnum");
	workpiecenum = cJSON_GetObjectItem(data_json, "workpiecenum");
	j1 = cJSON_GetObjectItem(data_json, "j1");
	j2 = cJSON_GetObjectItem(data_json, "j2");
	j3 = cJSON_GetObjectItem(data_json, "j3");
	j4 = cJSON_GetObjectItem(data_json, "j4");
	j5 = cJSON_GetObjectItem(data_json, "j5");
	j6 = cJSON_GetObjectItem(data_json, "j6");
	x = cJSON_GetObjectItem(data_json, "x");
	y = cJSON_GetObjectItem(data_json, "y");
	z = cJSON_GetObjectItem(data_json, "z");
	rx = cJSON_GetObjectItem(data_json, "rx");
	ry = cJSON_GetObjectItem(data_json, "ry");
	rz = cJSON_GetObjectItem(data_json, "rz");
	E1 = cJSON_GetObjectItem(data_json, "E1");
	E2 = cJSON_GetObjectItem(data_json, "E2");
	E3 = cJSON_GetObjectItem(data_json, "E3");
	E4 = cJSON_GetObjectItem(data_json, "E4");
	if (name == NULL || speed == NULL || acc == NULL || elbow_speed == NULL || elbow_acc == NULL || toolnum == NULL || workpiecenum == NULL || j1 == NULL || j2 == NULL || j3 == NULL || j4 == NULL || j5 == NULL || j6 == NULL || x == NULL || y == NULL || z == NULL || rx == NULL || ry == NULL || rz == NULL || E1 == NULL || E2 == NULL || E3 == NULL || E4 == NULL || name->valuestring == NULL || speed->valuestring == NULL || acc->valuestring == NULL || elbow_speed->valuestring == NULL || elbow_acc->valuestring == NULL || toolnum->valuestring == NULL || workpiecenum->valuestring == NULL || j1->valuestring == NULL || j2->valuestring == NULL || j3->valuestring == NULL || j4->valuestring == NULL || j5->valuestring == NULL || j6->valuestring == NULL || x->valuestring == NULL || y->valuestring == NULL || z->valuestring == NULL || rx->valuestring == NULL || ry->valuestring == NULL || rz->valuestring == NULL || E1->valuestring == NULL || E2->valuestring == NULL || E3->valuestring == NULL || E4->valuestring == NULL) {
		perror("json");

		return FAIL;
	}
	sprintf(sql, "update points set name='%s', speed='%s', elbow_speed='%s', acc='%s', elbow_acc='%s', toolnum='%s', workpiecenum='%s', j1='%s', j2='%s', j3='%s', j4='%s', j5='%s', j6='%s', E1='%s', E2='%s', E3='%s', E4='%s', x='%s', y='%s', z='%s', rx='%s', ry='%s', rz='%s' where name='%s';"\
				, name->valuestring, speed->valuestring, elbow_speed->valuestring,\
				acc->valuestring, elbow_acc->valuestring, toolnum->valuestring, workpiecenum->valuestring,\
				j1->valuestring, j2->valuestring, j3->valuestring, j4->valuestring, j5->valuestring, j6->valuestring, E1->valuestring, E2->valuestring, E3->valuestring, E4->valuestring, x->valuestring, y->valuestring, z->valuestring, rx->valuestring, ry->valuestring, rz->valuestring, name->valuestring);
	if (change_info_sqlite3(DB_POINTS, sql) == -1) {
		perror("database");

		return FAIL;
	}

	return SUCCESS;
}

/* remove points info */
static int remove_points(const cJSON *data_json)
{
	int ret = FAIL;
	int num = 0;
	int i = 0;
	char *buf = NULL;
	char sql[1024] = {0};
	cJSON *name_index = NULL;
	cJSON *name = cJSON_GetObjectItem(data_json, "name");
	if (name == NULL) {
		perror("json");

		return FAIL;
	}
	//printf("name = %s\n", cJSON_Print(name));
	num = cJSON_GetArraySize(name);

	//printf("num = %d\n", num);
	for (i = 0; i < num; i++) {
		name_index = cJSON_GetArrayItem(name, i);
		printf("name_index->valuestring = %s\n", name_index->valuestring);
		if (strcmp(name_index->valuestring, "seamPos") == 0) {
			continue;
		}
		memset(sql, 0, sizeof(sql));
		sprintf(sql, "delete from points where name = \'%s\';", name_index->valuestring);
		if (change_info_sqlite3(DB_POINTS, sql) == -1) {
			perror("database");

			return FAIL;
		}
	}

	return SUCCESS;
}

/* change type */
static int change_type(const cJSON *data_json)
{
	cJSON *type = cJSON_GetObjectItem(data_json, "type");
	if (type == NULL || type->valuestring == NULL) {
		perror("json");

		return FAIL;
	}
	robot_type = atoi(type->valuestring);
	printf("robot_type = %d\n", robot_type);

	return SUCCESS;
}

/* set_sys_logcount */
static int set_sys_logcount(const cJSON *data_json)
{
	int ret = FAIL;
	char *buf = NULL;
	char *cfg_content = NULL;
	cJSON *cfg_json = NULL;
	cJSON *count = NULL;

	count = cJSON_GetObjectItem(data_json, "log_count");
	if (count == NULL) {
		perror("json");

		return FAIL;
	}

	cfg_content = get_file_content(FILE_CFG);
	if (cfg_content == NULL || strcmp(cfg_content, "NO_FILE") == 0 || strcmp(cfg_content, "Empty") == 0) {
		perror("get file content");

		return FAIL;
	}
	cfg_json = cJSON_Parse(cfg_content);
	free(cfg_content);
	cfg_content = NULL;
	if (cfg_json == NULL) {
		perror("cJSON_Parse");

		return FAIL;
	}

	cJSON_ReplaceItemInObject(cfg_json, "log_count", cJSON_CreateString(count->valuestring));
	buf = cJSON_Print(cfg_json);
	ret = write_file(FILE_CFG, buf);
	free(buf);
	buf = NULL;
	cJSON_Delete(cfg_json);
	cfg_json = NULL;

	delete_log_file(0);

	return ret;
}

/* set_sys_lifespan */
static int set_sys_lifespan(const cJSON *data_json)
{
	int ret = FAIL;
	char *buf = NULL;
	char *cfg_content = NULL;
	cJSON *cfg_json = NULL;
	cJSON *lifespan = NULL;

	lifespan = cJSON_GetObjectItem(data_json, "lifespan");
	if (lifespan == NULL) {
		perror("json");

		return FAIL;
	}

	cfg_content = get_file_content(FILE_CFG);
	if (cfg_content == NULL || strcmp(cfg_content, "NO_FILE") == 0 || strcmp(cfg_content, "Empty") == 0) {
		perror("get file content");

		return FAIL;
	}
	cfg_json = cJSON_Parse(cfg_content);
	free(cfg_content);
	cfg_content = NULL;
	if (cfg_json == NULL) {
		perror("cJSON_Parse");

		return FAIL;
	}

	cJSON_ReplaceItemInObject(cfg_json, "lifespan", cJSON_CreateString(lifespan->valuestring));

	web_cfg.lifespan = atoi(lifespan->valuestring);
	//printf("web_cfg.lifespan = %d\n", web_cfg.lifespan);

	buf = cJSON_Print(cfg_json);
	ret = write_file(FILE_CFG, buf);
	free(buf);
	buf = NULL;
	cJSON_Delete(cfg_json);
	cfg_json = NULL;

	return ret;
}

/* set_sys_language */
static int set_sys_language(const cJSON *data_json)
{
	int ret = FAIL;
	char *buf = NULL;
	char *cfg_content = NULL;
	cJSON *cfg_json = NULL;
	cJSON *language_json = NULL;

	language_json = cJSON_GetObjectItem(data_json, "language");
	if (language_json == NULL) {
		perror("json");

		return FAIL;
	}

	cfg_content = get_file_content(FILE_CFG);
	if (cfg_content == NULL || strcmp(cfg_content, "NO_FILE") == 0 || strcmp(cfg_content, "Empty") == 0) {
		perror("get file content");

		return FAIL;
	}
	cfg_json = cJSON_Parse(cfg_content);
	free(cfg_content);
	cfg_content = NULL;
	if (cfg_json == NULL) {
		perror("cJSON_Parse");

		return FAIL;
	}

	cJSON_ReplaceItemInObject(cfg_json, "language", cJSON_CreateString(language_json->valuestring));
	buf = cJSON_Print(cfg_json);
	ret = write_file(FILE_CFG, buf);
	free(buf);
	buf = NULL;
	cJSON_Delete(cfg_json);
	cfg_json = NULL;

	language = atoi(language_json->valuestring);

	return ret;
}

/* set_ODM_cfg */
static int set_ODM_cfg(const cJSON *data_json)
{
	int ret = FAIL;
	char *buf = NULL;
	cJSON *cfg_json = NULL;
	cJSON *robot_model = NULL;

	cfg_json = cJSON_CreateObject();
	robot_model = cJSON_GetObjectItem(data_json, "robot_model");
	if (robot_model == NULL || robot_model->valuestring == NULL) {
		perror("json");

		return FAIL;
	}

	cJSON_AddStringToObject(cfg_json, "robot_model", robot_model->valuestring);
	buf = cJSON_Print(cfg_json);
	ret = write_file(FILE_ODM_CFG, buf);
	free(buf);
	buf = NULL;
	cJSON_Delete(cfg_json);
	cfg_json = NULL;

	return ret;
}

/* set_ptn_cfg */
static int set_ptn_cfg(const cJSON *data_json)
{
	char *buf = NULL;
	int write_ret = FAIL;

	buf = cJSON_Print(data_json);
	write_ret = write_file(FILE_POINTS_CFG, buf);
	free(buf);
	buf = NULL;
	if (write_ret == FAIL) {
		perror("write file");

		return FAIL;
	}

	return SUCCESS;
}

/* save accounts */
static int save_accounts(const cJSON *data_json)
{
    char passbuf[ME_GOAHEAD_LIMIT_PASSWORD * 3 + 3];
	char content[MAX_BUF] = "";
	char sql[2048] = {0};
	char del_sql[1024] = {0};
	char temp[2048] = {0};
	int i = 0;
	int array_size = 0;
	cJSON *item = NULL;
	cJSON *username = NULL;
	cJSON *password = NULL;
	cJSON *auth = NULL;
	cJSON *account_array = cJSON_GetObjectItem(data_json, "accounts");
	if (account_array == NULL || account_array->type != cJSON_Array) {
		perror("json");

		return FAIL;
	}

	array_size = cJSON_GetArraySize(account_array); //获取数组长度
	for (i = 0; i < array_size; i++) {
		item = cJSON_GetArrayItem(account_array, i);
		username = cJSON_GetObjectItem(item, "username");
		password = cJSON_GetObjectItem(item, "password");
		auth = cJSON_GetObjectItem(item, "auth");
		if (username == NULL || username->valuestring == NULL || password == NULL || password->valuestring == NULL || auth == NULL || auth->valuestring == NULL ) {
			perror("json");

			return FAIL;
		}
        fmt(passbuf, sizeof(passbuf), "%s:%s:%s", username->valuestring, ME_GOAHEAD_REALM, password->valuestring);
		sprintf(content, "%suser name=%s password=%s\n", content, username->valuestring, websMD5(passbuf));
		sprintf(temp, "%sinsert into account (username, password, auth) values (\'%s\', \'%s\', \'%s\'); ", sql, username->valuestring, password->valuestring, auth->valuestring);
		strcpy(sql, temp);
		memset(temp, 0, sizeof(temp));
	}
	printf("sql = %s\n",sql);

    sprintf(del_sql, "delete from account;");       //清空account表
    if (change_info_sqlite3(DB_ACCOUNT, del_sql) == -1) {
		perror("delete all");

		return FAIL;
    }
    if (change_info_sqlite3(DB_ACCOUNT, sql) == -1) {
		perror("insert");

		return FAIL;
    }

	printf("content = %s\n", content);

	if (write_file(FILE_AUTH, content) == FAIL) {//save to file auth.txt, 更新账号密码到auth.txt文件中
		return FAIL;
	}
#if ME_GOAHEAD_AUTH
	websCloseAuth();
	websOpenAuth(0);
    if (websLoad("auth.txt") < 0) {
        perror("Cannot load auth.txt");

        return FAIL;
    }
#endif

	return SUCCESS;
}

/** shutdown system */
static int shutdown_system(const cJSON *data_json)
{
	system("shutdown -b");

	return SUCCESS;
}

/** plugin_enable */
static int plugin_enable(const cJSON *data_json)
{
	char package_path[100] = {0};
	char *package_content = NULL;
	char *buf = NULL;
	int write_ret = FAIL;
	cJSON *name = NULL;
	cJSON *value = NULL;
	cJSON *package_json = NULL;

	name = cJSON_GetObjectItem(data_json, "name");
	value = cJSON_GetObjectItem(data_json, "value");
	if (name == NULL || value == NULL || name->valuestring == NULL) {
		perror("json");

		return FAIL;
	}
	/** 修改对应外设插件package.json中enable */
	sprintf(package_path, "%s%s/package.json", UPLOAD_WEB_PLUGINS, name->valuestring);
	package_content = get_file_content(package_path);
	if (package_content == NULL || strcmp(package_content, "NO_FILE") == 0 || strcmp(package_content, "Empty") == 0) {
		perror("get file content");

		return FAIL;
	}
	package_json = cJSON_Parse(package_content);
	free(package_content);
	package_content = NULL;
	if (package_json == NULL) {
		perror("cJSON_Parse");

		return FAIL;
	}
	cJSON_ReplaceItemInObject(package_json, "enable", cJSON_CreateNumber(value->valueint));
	buf = cJSON_Print(package_json);
	write_ret = write_file(package_path, buf);//write file
	free(buf);
	buf = NULL;
	cJSON_Delete(package_json);
	package_json = NULL;
	if (write_ret == FAIL) {
		perror("write file");

		return FAIL;
	}

	/** enable plugin */
	if (value->valueint == 1) {

		return SUCCESS;
	/** disable plugin */
	} else {

		return clear_plugin_config(name->valuestring);
	}
}

/** plugin_remove */
static int plugin_remove(const cJSON *data_json)
{
	char delete_cmd[128] = {0};
	cJSON *name = NULL;

	name = cJSON_GetObjectItem(data_json, "name");
	if (name == NULL || name->valuestring == NULL) {
		perror("json");

		return FAIL;
	}

	if (clear_plugin_config(name->valuestring) == FAIL) {
		perror("clear_plugin_config");

		return FAIL;
	}
	sprintf(delete_cmd, "rm -rf %s%s", UPLOAD_WEB_PLUGINS, name->valuestring);
	system(delete_cmd);

	return SUCCESS;
}

/** clear_DH_file */
static int clear_DH_file(const cJSON *data_json)
{
	int ret = open(FILE_DH_POINT, O_WRONLY | O_TRUNC);
	if (ret == -1) {
		printf("DH POINT file is not exist!\n");
	} else {
		close(ret);
	}

	return SUCCESS;
}

/** save_DH_point */
static int save_DH_point(const cJSON *data_json)
{
	char point_info[LINE_LEN] = {0};
	char strline[LINE_LEN] = {0};
	char write_content[26*LINE_LEN] = {0};
	char tmp_content[26*LINE_LEN] = {0};
	FILE *fp = NULL;
	char **array = NULL;
	int size = 0;
	int write_ret = FAIL;
	int exist_point = 0;
	CTRL_STATE *state = NULL;
	cJSON *no = NULL;

	no = cJSON_GetObjectItem(data_json, "no");
	if (no == NULL || no->valuestring == NULL) {
		perror("json");

		return FAIL;
	}
	if (robot_type == 1) { // "1" 代表实体机器人
		state = &ctrl_state;
	} else { // "0" 代表虚拟机器人
		state = &vir_ctrl_state;
	}

	sprintf(point_info, "%s %lf %lf %lf %lf %lf %lf\n", no->valuestring, state->jt_cur_pos[0], state->jt_cur_pos[1], state->jt_cur_pos[2], state->jt_cur_pos[3], state->jt_cur_pos[4], state->jt_cur_pos[5]);
	//printf("point_info = %s\n", point_info);

	/** DH point 文件存在 */
	if ((fp = fopen(FILE_DH_POINT, "r+")) != NULL) {
		while (fgets(strline, LINE_LEN, fp) != NULL) {
			if (string_to_string_list(strline, " ", &size, &array) == 0 || size != 7) {
				perror("string to string list");
				fclose(fp);
				string_list_free(&array, size);

				return FAIL;
			}
			bzero(tmp_content, 26*LINE_LEN);
			/** 已经存在记录过的 DH 校准点, 替换原本点信息 */
			if (strcmp(array[0], no->valuestring) == 0) {
				sprintf(tmp_content, "%s%s", write_content, point_info);
				exist_point = 1;
			} else {
				sprintf(tmp_content, "%s%s", write_content, strline);
			}
			strcpy(write_content, tmp_content);
			string_list_free(&array, size);
			bzero(strline, LINE_LEN);
		}
		fclose(fp);
		//printf("write_content = %s\n", write_content);
		write_ret = write_file(FILE_DH_POINT, write_content);
		if (write_ret == FAIL) {
			perror("write file");

			return FAIL;
		}
		/** 文件不存在记录过的 DH 校准点，追加点的信息到文末即可 */
		if (exist_point == 0) {
			return write_file_append(FILE_DH_POINT, point_info);
		}
	/** DH point 文件不存在 */
	} else {
		return write_file_append(FILE_DH_POINT, point_info);
	}
}

/** factory reset */
static int factory_reset(const cJSON *data_json)
{
	char cmd[128] = {0};

	memset(cmd, 0, 128);
	sprintf(cmd, "rm -rf %s*", DIR_FILE);
	system(cmd);
/*
	memset(cmd, 0, 128);
	sprintf(cmd, "rm -rf %s*", DIR_LOG);
	system(cmd);

	memset(cmd, 0, 128);
	sprintf(cmd, "rm -rf %s*", DIR_LOG_EN);
	system(cmd);

	memset(cmd, 0, 128);
	sprintf(cmd, "rm -rf %s*", DIR_LOG_JAP);
	system(cmd);
*/
	memset(cmd, 0, 128);
	sprintf(cmd, "cp -r %s* %s", DIR_FACTORY, DIR_FILE);
	system(cmd);

	system("rm -f /root/robot/ex_device.config");
	memset(cmd, 0, 128);
	sprintf(cmd, "cp %s %s", WEB_EX_DEVICE_CFG, EX_DEVICE_CFG);
	system(cmd);

	system("rm -f /root/robot/exaxis.config");
	memset(cmd, 0, 128);
	sprintf(cmd, "cp %s %s", WEB_EXAXIS_CFG, EXAXIS_CFG);
	system(cmd);

	/**
		update user.config robot_type
	*/
	return update_userconfig_robottype();
}

/** odm_password */
static int odm_password(const cJSON *data_json)
{
	cJSON *password = NULL;

	password = cJSON_GetObjectItem(data_json, "password");
	if (password == NULL || password->valuestring == NULL) {
		perror("json");

		return FAIL;
	}

	if (strcmp(password->valuestring, ODM_PASSWORD) == 0) {

		return SUCCESS;
	} else {

		return FAIL;
	}
}

/** save robot type */
static int save_robot_type(const cJSON *data_json)
{
	cJSON *type_json = NULL;
	char *buf = NULL;
	int write_ret = FAIL;
	cJSON *root_json = NULL;

	type_json = cJSON_GetObjectItem(data_json, "type");
	if (type_json == NULL) {
		perror("json");

		return FAIL;
	}

	root_json = cJSON_CreateObject();
	if (type_json->valueint == 1) {
		cJSON_AddStringToObject(root_json, "robot_model", "FR3");
	} else if (type_json->valueint == 2) {
		cJSON_AddStringToObject(root_json, "robot_model", "FR5");
	} else if (type_json->valueint == 3) {
		cJSON_AddStringToObject(root_json, "robot_model", "FR10");
	}
	/** 写入 odm/cfg.txt 文件 */
	buf = cJSON_Print(root_json);
	write_ret = write_file(FILE_ODM_CFG, buf);//write file
	free(buf);
	buf = NULL;
	cJSON_Delete(root_json);
	root_json = NULL;
	if (write_ret == FAIL) {
		perror("write file");

		return FAIL;
	}
	/** 写入RobotType.txt 文件 */
	buf = cJSON_Print(data_json);
	write_ret = write_file(FILE_ROBOT_TYPE, buf);//write file
	free(buf);
	buf = NULL;
	if (write_ret == FAIL) {
		perror("write file");

		return FAIL;
	}
	/**
		代码中调用 write/read 等文件系统函数时,
	    看似程序已经返回文件写入或读取成功，
		实际硬盘上的文件尚未更改，
		需要一定的等待时间
	*/
	sleep(10);

	return SUCCESS;
}

/** torque save cfg */
static int torque_save_cfg(const cJSON *data_json)
{
	char sql[1204] = {0};
	cJSON *old_workpiece_name = NULL;
	cJSON *new_workpiece_name = NULL;
	cJSON *screw_num = NULL;
	cJSON *screw_time = NULL;
	cJSON *screw_period = NULL;
	cJSON *float_time = NULL;
	cJSON *slip_time = NULL;
	cJSON *dispensing_time = NULL;

	old_workpiece_name = cJSON_GetObjectItem(data_json, "old_workpiece_id");
	new_workpiece_name = cJSON_GetObjectItem(data_json, "new_workpiece_id");
	screw_num = cJSON_GetObjectItem(data_json, "screw_num");
	screw_time = cJSON_GetObjectItem(data_json, "screw_time");
	screw_period = cJSON_GetObjectItem(data_json, "screw_period");
	float_time = cJSON_GetObjectItem(data_json, "float_time");
	slip_time = cJSON_GetObjectItem(data_json, "slip_time");
	dispensing_time = cJSON_GetObjectItem(data_json, "dispensing_time");
	if (old_workpiece_name == NULL || new_workpiece_name == NULL || screw_num == NULL || screw_time == NULL || screw_period == NULL || float_time == NULL || slip_time == NULL || dispensing_time == NULL) {
		perror("json");

		return FAIL;
	}

	if (strcmp(old_workpiece_name->valuestring, "new") == 0) {
		sprintf(sql, "insert into torquesys_cfg values ('%s', %d, %d, %d, %d, %d, %d);", new_workpiece_name->valuestring, screw_num->valueint, screw_time->valueint, screw_period->valueint, float_time->valueint, slip_time->valueint, dispensing_time->valueint);
	} else {
		sprintf(sql, "update torquesys_cfg set workpiece_id='%s', screw_num=%d, screw_time=%d, screw_period=%d, float_time=%d, slip_time=%d, dispensing_time=%d where workpiece_id='%s';", new_workpiece_name->valuestring, screw_num->valueint, screw_time->valueint, screw_period->valueint, float_time->valueint, slip_time->valueint, dispensing_time->valueint, old_workpiece_name->valuestring);
	}

	if (change_info_sqlite3(DB_TORQUE_CFG, sql) == -1) {
		perror("database");

		return FAIL;
	}

	return SUCCESS;
}

/** 生成示教点文件 */
static int generate_luafile(const cJSON *data_json)
{
	cJSON *workpiece_name = NULL;
	cJSON *ptemp = NULL;
	cJSON *perscrew_pnum = NULL;
	cJSON *left_workstation = NULL;
	cJSON *right_workstation = NULL;
	cJSON *item = NULL;
	cJSON *name = NULL;
	char cmd[1024] = "";
	char ptemp_name[100] = "";
	FILE *fp_ptemp = NULL;
	char strline[100] = "";
	char write_line[100] = "";
	char *write_content = NULL;
	char pointfile_name[100] = "";
	char *strstr_ptr = NULL;
	char strstr_head[100] = "";
	char point[10] = "";
	int screw_num = 0;
	int i = 0;
	int j = 0;

	write_content = (char *)calloc(1, sizeof(char)*20000);
	if (write_content == NULL) {
		perror("calloc");

		return FAIL;
	}
	workpiece_name = cJSON_GetObjectItem(data_json, "workpiece_id");
	ptemp = cJSON_GetObjectItem(data_json, "ptemp");
	perscrew_pnum = cJSON_GetObjectItem(data_json, "perscrew_pnum");
	left_workstation = cJSON_GetObjectItem(data_json, "left_workstation");
	right_workstation = cJSON_GetObjectItem(data_json, "right_workstation");
	if (workpiece_name == NULL || ptemp == NULL || perscrew_pnum == NULL || left_workstation == NULL || left_workstation->type != cJSON_Array || right_workstation == NULL || right_workstation->type != cJSON_Array) {
		perror("json");

		return FAIL;
	}

	/* 清空旧的工件示教点文件 */
	memset(cmd, 0, 1024);
	sprintf(cmd, "rm -f /root/web/file/user/point_%s*", workpiece_name->valuestring);
	system(cmd);

	sprintf(ptemp_name, "%s%s", DIR_TEMPLATE, ptemp->valuestring);

	/*
	   锁附单个螺丝所需示教点个数
	   吹气式: 2
	   点胶加磁吸：6
	*/
	/* 左工位 */
	screw_num = cJSON_GetArraySize(left_workstation) / perscrew_pnum->valueint;
	for (i = 1; i <= screw_num; i++) {
		memset(write_content, 0, 20000);
		/* 根据选择的工件示教点模板文件，生成新的工件示教点 lua 文件 */
		if ((fp_ptemp = fopen(ptemp_name, "r")) == NULL) {
			perror("open file");

			return FAIL;
		}
		/* 每个螺丝生成一个示教点文件 */
		while (fgets(strline, 100, fp_ptemp) != NULL) {
			memset(write_line, 0, sizeof(char)*100);
			strcpy(write_line, strline);

			if (strstr_ptr = strstr(strline, "<id>")) {
				memset(strstr_head, 0, 100);
				strncpy(strstr_head, strline, (strstr_ptr - strline));
				memset(write_line, 0, sizeof(char)*100);
				sprintf(write_line, "%s%d%s", strstr_head, i, (strstr_ptr + strlen("<id>")));
				memset(strline, 0, sizeof(char)*100);
				strcpy(strline, write_line);
			}
			if (strstr_ptr = strstr(strline, "<station>")) {
				memset(strstr_head, 0, 100);
				strncpy(strstr_head, strline, (strstr_ptr - strline));
				memset(write_line, 0, sizeof(char)*100);
				sprintf(write_line, "%s%s%s", strstr_head, "left", (strstr_ptr + strlen("<station>")));
				memset(strline, 0, sizeof(char)*100);
				strcpy(strline, write_line);
			}
			for (j = 1; j <= perscrew_pnum->valueint; j++) {
				memset(point, 0, 10);
				sprintf(point, "<point%d>", j);
				if (strstr_ptr = strstr(strline, point)) {
					item = cJSON_GetArrayItem(left_workstation, ((i - 1) * perscrew_pnum->valueint + (j - 1)));
					if (item != NULL) {
						name = cJSON_GetObjectItem(item, "name");
						memset(strstr_head, 0, 100);
						strncpy(strstr_head, strline, (strstr_ptr - strline));
						memset(write_line, 0, sizeof(char)*100);
						sprintf(write_line, "%s%s%s", strstr_head, name->valuestring, (strstr_ptr + strlen(point)));
						memset(strline, 0, sizeof(char)*100);
						strcpy(strline, write_line);
					}
				}
			}
			//printf("write_line = %s\n", write_line);
			strcat(write_content, write_line);
			memset(strline, 0, sizeof(char)*100);
		}
		memset(pointfile_name, 0, 100);
		sprintf(pointfile_name, "/root/web/file/user/point_%s_left_%d.lua", workpiece_name->valuestring, i);
		//printf("pointfile_name = %s\n", pointfile_name);
		//printf("write_content = %s\n", write_content);
		//printf("strlen write_content = %d\n", strlen(write_content));
		if (write_file(pointfile_name, write_content) == FAIL) {
			perror("write file");
			free(write_content);
			write_content = NULL;

			return FAIL;
		}
		fclose(fp_ptemp);
	}

	/* 右工位 */
	screw_num = cJSON_GetArraySize(right_workstation) / perscrew_pnum->valueint;
	for (i = 1; i <= screw_num; i++) {
		memset(write_content, 0, 20000);
		/* 根据选择的工件示教点模板文件，生成新的工件示教点 lua 文件 */
		if ((fp_ptemp = fopen(ptemp_name, "r")) == NULL) {
			perror("open file");

			return FAIL;
		}
		/* 每个螺丝生成一个示教点文件 */
		while (fgets(strline, 100, fp_ptemp) != NULL) {
			memset(write_line, 0, sizeof(char)*100);
			strcpy(write_line, strline);
			if (strstr_ptr = strstr(strline, "<id>")) {
				memset(strstr_head, 0, 100);
				strncpy(strstr_head, strline, (strstr_ptr - strline));
				memset(write_line, 0, sizeof(char)*100);
				sprintf(write_line, "%s%d%s", strstr_head, i, (strstr_ptr + strlen("<id>")));
				memset(strline, 0, sizeof(char)*100);
				strcpy(strline, write_line);
			}
			if (strstr_ptr = strstr(strline, "<station>")) {
				memset(strstr_head, 0, 100);
				strncpy(strstr_head, strline, (strstr_ptr - strline));
				memset(write_line, 0, sizeof(char)*100);
				sprintf(write_line, "%s%s%s", strstr_head, "right", (strstr_ptr + strlen("<station>")));
				memset(strline, 0, sizeof(char)*100);
				strcpy(strline, write_line);
			}
			for (j = 1; j <= perscrew_pnum->valueint; j++) {
				memset(point, 0, 10);
				sprintf(point, "<point%d>", j);
				if (strstr_ptr = strstr(strline, point)) {
					item = cJSON_GetArrayItem(right_workstation, ((i - 1) * perscrew_pnum->valueint + (j - 1)));
					if (item != NULL) {
						name = cJSON_GetObjectItem(item, "name");
						memset(strstr_head, 0, 100);
						strncpy(strstr_head, strline, (strstr_ptr - strline));
						memset(write_line, 0, sizeof(char)*100);
						sprintf(write_line, "%s%s%s", strstr_head, name->valuestring, (strstr_ptr + strlen(point)));
						memset(strline, 0, sizeof(char)*100);
						strcpy(strline, write_line);
					}
				}
			}
			//printf("write_line = %s\n", write_line);
			strcat(write_content, write_line);
			memset(strline, 0, sizeof(char)*100);
		}
		memset(pointfile_name, 0, 100);
		sprintf(pointfile_name, "/root/web/file/user/point_%s_right_%d.lua", workpiece_name->valuestring, i);
		if (write_file(pointfile_name, write_content) == FAIL) {
			perror("write file");
			free(write_content);
			write_content = NULL;

			return FAIL;
		}
		fclose(fp_ptemp);
	}

	free(write_content);
	write_content = NULL;

	return SUCCESS;
}

/** torque ensure points */
static int torque_ensure_points(const cJSON *data_json)
{
	cJSON *workpiece_name = NULL;
	cJSON *ptemp = NULL;
	cJSON *perscrew_pnum = NULL;
	cJSON *left_workstation = NULL;
	cJSON *right_workstation = NULL;
	cJSON *item = NULL;
	cJSON *name = NULL;
	cJSON *id = NULL;
	char sql[2048] = {0};
	char *sql_left_insert = NULL;
	char *sql_right_insert = NULL;
	char temp[1024] = {0};
	char wk_left[100] = "";
	char wk_right[100] = "";
	char wk_cfg[100] = "";
	int array_size_left = 0;
	int array_size_right = 0;
	int i = 0;
	char** resultp = NULL;
	int nrow = 0;
	int ncloumn = 0;

	workpiece_name = cJSON_GetObjectItem(data_json, "workpiece_id");
	ptemp = cJSON_GetObjectItem(data_json, "ptemp");
	perscrew_pnum = cJSON_GetObjectItem(data_json, "perscrew_pnum");
	left_workstation = cJSON_GetObjectItem(data_json, "left_workstation");
	right_workstation = cJSON_GetObjectItem(data_json, "right_workstation");
	if (workpiece_name == NULL || ptemp == NULL || perscrew_pnum == NULL || left_workstation == NULL || left_workstation->type != cJSON_Array || right_workstation == NULL || right_workstation->type != cJSON_Array) {
		perror("json");

		return FAIL;
	}
	sprintf(wk_left, "%s_left", workpiece_name->valuestring);
	sprintf(wk_right, "%s_right", workpiece_name->valuestring);
	sprintf(wk_cfg, "%s_cfg", workpiece_name->valuestring);

	/** 保存 points 信息到数据库 */
	/** 删除已有的工件数据表 */
	memset(sql, 0, sizeof(sql));
	sprintf(sql, "select name from sqlite_master where type = 'table' and name like '%s%';", workpiece_name->valuestring);
	select_info_sqlite3(DB_TORQUE_POINTS, sql, &resultp, &nrow, &ncloumn);
	for (i = 0; i < nrow; i++) {
		memset(sql, 0, sizeof(sql));
		sprintf(sql, "drop table [%s];", resultp[(i + 1) * ncloumn]);
		if (change_info_sqlite3(DB_TORQUE_POINTS, sql) == -1) {
			perror("database");

			return FAIL;
		}
	}
	sqlite3_free_table(resultp);

	/** 创建 table */
	memset(sql, 0, sizeof(sql));
	sprintf(sql, "create table [%s] (name TEXT, id INTEGER primary key);", wk_left);
	if (change_info_sqlite3(DB_TORQUE_POINTS, sql) == -1) {
		perror("database");

		return FAIL;
	}
	memset(sql, 0, sizeof(sql));
	sprintf(sql, "create table [%s] (name TEXT, id INTEGER primary key);", wk_right);
	if (change_info_sqlite3(DB_TORQUE_POINTS, sql) == -1) {
		perror("database");

		return FAIL;
	}
	memset(sql, 0, sizeof(sql));
	sprintf(sql, "create table [%s] (ptemp TEXT, perscrew_pnum INTEGER);", wk_cfg);
	if (change_info_sqlite3(DB_TORQUE_POINTS, sql) == -1) {
		perror("database");

		return FAIL;
	}
	/** 插入示教点信息 */
	array_size_left = cJSON_GetArraySize(left_workstation);
	sql_left_insert = (char *)calloc(1, sizeof(char)*array_size_left*100);
	if (sql_left_insert == NULL) {
		perror("calloc");

		return FAIL;
	}
	for (i = 0; i < array_size_left; i++) {
		item = cJSON_GetArrayItem(left_workstation, i);
		name = cJSON_GetObjectItem(item, "name");
		id = cJSON_GetObjectItem(item, "id");
		memset(temp, 0, sizeof(temp));
		sprintf(temp, "insert into [%s] values ('%s', %d);", wk_left, name->valuestring, id->valueint);
		strcat(sql_left_insert, temp);
	}
	if (change_info_sqlite3(DB_TORQUE_POINTS, sql_left_insert) == -1) {
		perror("insert");

		return FAIL;
	}
	free(sql_left_insert);
	sql_left_insert = NULL;

	array_size_right = cJSON_GetArraySize(right_workstation);
	sql_right_insert = (char *)calloc(1, sizeof(char)*array_size_right*100);
	if (sql_right_insert == NULL) {
		perror("calloc");

		return FAIL;
	}
	for (i = 0; i < array_size_right; i++) {
		item = cJSON_GetArrayItem(right_workstation, i);
		name = cJSON_GetObjectItem(item, "name");
		id = cJSON_GetObjectItem(item, "id");
		memset(temp, 0, sizeof(temp));
		sprintf(temp, "insert into [%s] values ('%s', %d); ", wk_right, name->valuestring, id->valueint);
		strcat(sql_right_insert, temp);
	}
	if (change_info_sqlite3(DB_TORQUE_POINTS, sql_right_insert) == -1) {
		perror("insert");

		return FAIL;
	}
	free(sql_right_insert);
	sql_right_insert = NULL;

	memset(sql, 0, sizeof(sql));
	sprintf(sql, "insert into [%s] values ('%s', %d);", wk_cfg, ptemp->valuestring, perscrew_pnum->valueint);
	if (change_info_sqlite3(DB_TORQUE_POINTS, sql) == -1) {
		perror("insert");

		return FAIL;
	}

	/** 生成示教点 lua 文件 */
	return generate_luafile(data_json);
}

/** torque set DIO cfg */
static int set_DIO_cfg(const cJSON *data_json)
{
	char *buf = NULL;
	int write_ret = FAIL;

	if (data_json == NULL) {
		perror("json");

		return FAIL;
	}

	buf = cJSON_Print(data_json);
	write_ret = write_file(FILE_TORQUE_DIO, buf);
	free(buf);
	buf = NULL;
	if (write_ret == FAIL) {
		perror("write file");

		return FAIL;
	}

	return SUCCESS;
}

/** set status showflag */
static int set_status_flag(const cJSON *data_json)
{
	char *buf = NULL;
	int write_ret = FAIL;

	buf = cJSON_Print(data_json);
	write_ret = write_file(FILE_STATUS_SHOWFLAG, buf);
	free(buf);
	buf = NULL;
	if (write_ret == FAIL) {
		perror("write file");

		return FAIL;
	}

	return SUCCESS;
}

/* rename var */
static int rename_var(const cJSON *data_json)
{
	char sql[1024] = {0};
	cJSON *name = NULL;
	cJSON *id = NULL;

	name = cJSON_GetObjectItem(data_json, "name");
	id = cJSON_GetObjectItem(data_json, "id");
	if (name == NULL || id == NULL) {
		perror("json");

		return FAIL;
	}
	sprintf(sql, "update sysvar set name='%s', id=%d where id=%d;", name->valuestring, id->valueint, id->valueint);
	if (change_info_sqlite3(DB_SYSVAR, sql) == -1) {
		perror("database");

		return FAIL;
	}

	return SUCCESS;
}

/* clear product info */
static int clear_product_info(const cJSON *data_json)
{
	SOCKET_INFO *sock_cmd = NULL;
	cJSON *station = NULL;

	station = cJSON_GetObjectItem(data_json, "station");
	// set sys value
	if (robot_type == 1) { // "1" 代表实体机器人
		sock_cmd = &socket_cmd;
	} else { // "0" 代表虚拟机器人
		sock_cmd = &socket_vir_cmd;
	}

	/* 左工位清除 */
	if (station->valueint == 0) {
		socket_enquene(sock_cmd, 511, "SetSysVarValue(12, 0)", 1);
		socket_enquene(sock_cmd, 511, "SetSysVarValue(13, 0)", 1);
		socket_enquene(sock_cmd, 511, "SetSysVarValue(14, 0)", 1);
		memset(jiabao_torque_pd_data.left_wk_id, 0, 100);
	}
	/* 右工位清除 */
	if (station->valueint == 1) {
		socket_enquene(sock_cmd, 511, "SetSysVarValue(17, 0)", 1);
		socket_enquene(sock_cmd, 511, "SetSysVarValue(18, 0)", 1);
		socket_enquene(sock_cmd, 511, "SetSysVarValue(19, 0)", 1);
		memset(jiabao_torque_pd_data.right_wk_id, 0, 100);
	}

	return SUCCESS;
}

/* move to home point */
static int move_to_home_point(const cJSON *data_json)
{
	cJSON *f_json = NULL;
	cJSON *ovl = NULL;
	cJSON *ptp = NULL;
	cJSON *j1 = NULL;
	cJSON *j2 = NULL;
	cJSON *j3 = NULL;
	cJSON *j4 = NULL;
	cJSON *j5 = NULL;
	cJSON *j6 = NULL;
	cJSON *x = NULL;
	cJSON *y = NULL;
	cJSON *z = NULL;
	cJSON *rx = NULL;
	cJSON *ry = NULL;
	cJSON *rz = NULL;
	cJSON *speed = NULL;
	cJSON *acc = NULL;
	cJSON *toolnum = NULL;
	cJSON *workpiecenum = NULL;
	cJSON *E1 = NULL;
	cJSON *E2 = NULL;
	cJSON *E3 = NULL;
	cJSON *E4 = NULL;
	char sql[MAX_BUF] = "";
	char content[MAX_BUF] = "";
	SOCKET_INFO *sock_cmd = NULL;
	char joint_value[6][10] = { 0 };
	char *joint_value_ptr[6];
	int i = 0;

	for (i = 0; i < 6; i++) {
		joint_value_ptr[i] = NULL;
	}
	ovl = cJSON_GetObjectItem(data_json, "ovl");
	if (ovl == NULL) {
		perror("json");

		return FAIL;
	}
	sprintf(sql, "select * from points;");
	if (select_info_json_sqlite3(DB_POINTS, sql, &f_json) == -1) {
		perror("select ptp points");

		return FAIL;
	}
	ptp = cJSON_GetObjectItemCaseSensitive(f_json, POINT_HOME);
	if (ptp == NULL || ptp->type != cJSON_Object) {

		return FAIL;
	}
	j1 = cJSON_GetObjectItem(ptp, "j1");
	j2 = cJSON_GetObjectItem(ptp, "j2");
	j3 = cJSON_GetObjectItem(ptp, "j3");
	j4 = cJSON_GetObjectItem(ptp, "j4");
	j5 = cJSON_GetObjectItem(ptp, "j5");
	j6 = cJSON_GetObjectItem(ptp, "j6");
	x = cJSON_GetObjectItem(ptp, "x");
	y = cJSON_GetObjectItem(ptp, "y");
	z = cJSON_GetObjectItem(ptp, "z");
	rx = cJSON_GetObjectItem(ptp, "rx");
	ry = cJSON_GetObjectItem(ptp, "ry");
	rz = cJSON_GetObjectItem(ptp, "rz");
	toolnum = cJSON_GetObjectItem(ptp, "toolnum");
	workpiecenum = cJSON_GetObjectItem(ptp, "workpiecenum");
	speed = cJSON_GetObjectItem(ptp, "speed");
	acc = cJSON_GetObjectItem(ptp, "acc");
	E1 = cJSON_GetObjectItem(ptp, "E1");
	E2 = cJSON_GetObjectItem(ptp, "E2");
	E3 = cJSON_GetObjectItem(ptp, "E3");
	E4 = cJSON_GetObjectItem(ptp, "E4");
	if (j1 == NULL || j2 == NULL || j3 == NULL || j4 == NULL || j5 == NULL || j6 == NULL || x == NULL || y == NULL || z == NULL || rx == NULL || ry == NULL || rz == NULL || toolnum == NULL || workpiecenum == NULL || speed == NULL || acc == NULL || E1 == NULL || E2 == NULL || E3 == NULL || E4 == NULL || j1->valuestring == NULL || j2->valuestring == NULL || j3->valuestring == NULL || j4->valuestring == NULL || j5->valuestring == NULL || j6->valuestring == NULL || x->valuestring == NULL || y->valuestring == NULL || z->valuestring == NULL || rx->valuestring == NULL || ry->valuestring == NULL || rz->valuestring == NULL || toolnum->valuestring == NULL || workpiecenum->valuestring == NULL || speed->valuestring == NULL || acc->valuestring == NULL || E1->valuestring == NULL || E2->valuestring == NULL || E3->valuestring == NULL || E4->valuestring == NULL) {

		return FAIL;
	}

	sprintf(joint_value[0], "%.1lf", atof(j1->valuestring));
	sprintf(joint_value[1], "%.1lf", atof(j2->valuestring));
	sprintf(joint_value[2], "%.1lf", atof(j3->valuestring));
	sprintf(joint_value[3], "%.1lf", atof(j4->valuestring));
	sprintf(joint_value[4], "%.1lf", atof(j5->valuestring));
	sprintf(joint_value[5], "%.1lf", atof(j6->valuestring));
	for (i = 0; i < 6; i++) {
		joint_value_ptr[i] = joint_value[i];
	}

	/* 置异常报错的标志位， 添加异常错误到 sta 状态反馈 error_info 中 */
	if (check_pointhome_data(joint_value_ptr) == FAIL) {
		point_home_info.error_flag = 1;

		return FAIL;
	}
	point_home_info.error_flag = 0;

	sprintf(content, "MoveJ(%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,0,0,0,0,0,0,0,0)\n", j1->valuestring, j2->valuestring, j3->valuestring, j4->valuestring, j5->valuestring, j6->valuestring, x->valuestring, y->valuestring, z->valuestring, rx->valuestring, ry->valuestring, rz->valuestring, toolnum->valuestring, workpiecenum->valuestring, speed->valuestring, acc->valuestring, ovl->valuestring, E1->valuestring, E2->valuestring, E3->valuestring, E4->valuestring);
	if (robot_type == 1) { // "1" 代表实体机器人
		sock_cmd = &socket_cmd;
	} else { // "0" 代表虚拟机器人
		sock_cmd = &socket_vir_cmd;
	}
	socket_enquene(sock_cmd, 201, content, 1);

	return SUCCESS;
}

/* torque generate program */
static int torque_generate_program(const cJSON *data_json)
{
	int i = 0;
	char sql[1024] = {0};
	char **resultp = NULL;
	int nrow = 0;
	int ncloumn = 0;
	FILE *fp = NULL;
	char strline[100] = "";
	char write_line[100] = "";
	char *write_content = NULL;
	char mainlua_name[100] = "";
	char *strstr_ptr = NULL;
	char strstr_head[100] = "";
	char maintemp_name[100] = "";
	int left_screw_time = 0;
	int left_float_time = 0;
	int left_slip_time = 0;
	int left_dispensing_time = 0;
	int right_screw_time = 0;
	int right_float_time = 0;
	int right_slip_time = 0;
	int right_dispensing_time = 0;
	cJSON *main_temp = NULL;
	cJSON *left_wp = NULL;
	cJSON *right_wp = NULL;
	cJSON *gp_name = NULL;
	cJSON *left_screw_array = NULL;
	cJSON *right_screw_array = NULL;

	write_content = (char *)calloc(1, sizeof(char)*(20000));
	if (write_content == NULL) {
		perror("calloc");

		return FAIL;
	}
	main_temp = cJSON_GetObjectItem(data_json, "main_temp");
	left_wp = cJSON_GetObjectItem(data_json, "left_wp");
	left_screw_array = cJSON_GetObjectItem(data_json, "left_screw_array");
	right_wp = cJSON_GetObjectItem(data_json, "right_wp");
	right_screw_array = cJSON_GetObjectItem(data_json, "right_screw_array");
	gp_name = cJSON_GetObjectItem(data_json, "gp_name");
	if (main_temp == NULL || left_wp == NULL || left_screw_array == NULL || right_wp == NULL || right_screw_array == NULL || gp_name == NULL || left_screw_array->type != cJSON_Array || right_screw_array->type != cJSON_Array) {
		perror("json");

		return FAIL;
	}

	/* 获取左工位上工件信息 */
	memset(sql, 0, sizeof(sql));
	sprintf(sql, "select * from torquesys_cfg where workpiece_id = '%s'", left_wp->valuestring);
	if (select_info_sqlite3(DB_TORQUE_CFG, sql, &resultp, &nrow, &ncloumn) == -1) {

		return FAIL;
	}
	if (resultp[ncloumn + 2] != NULL) {
		left_screw_time = atoi(resultp[ncloumn + 2]);
	}
	if (resultp[ncloumn + 4] != NULL) {
		left_float_time = atoi(resultp[ncloumn + 4]);
	}
	if (resultp[ncloumn + 5] != NULL) {
		left_slip_time = atoi(resultp[ncloumn + 5]);
	}
	if (resultp[ncloumn + 6] != NULL) {
		left_dispensing_time = atoi(resultp[ncloumn + 6]);
	}
	sqlite3_free_table(resultp); //释放结果集

	/* 获取右工位上工件信息 */
	memset(sql, 0, sizeof(sql));
	sprintf(sql, "select * from torquesys_cfg where workpiece_id = '%s'", right_wp->valuestring);
	if (select_info_sqlite3(DB_TORQUE_CFG, sql, &resultp, &nrow, &ncloumn) == -1) {

		return FAIL;
	}
	if (resultp[ncloumn + 2] != NULL) {
		right_screw_time = atoi(resultp[ncloumn + 2]);
	}
	if (resultp[ncloumn + 4] != NULL) {
		right_float_time = atoi(resultp[ncloumn + 4]);
	}
	if (resultp[ncloumn + 5] != NULL) {
		right_slip_time = atoi(resultp[ncloumn + 5]);
	}
	if (resultp[ncloumn + 6] != NULL) {
		right_dispensing_time = atoi(resultp[ncloumn + 6]);
	}
	sqlite3_free_table(resultp); //释放结果集

	sprintf(maintemp_name, "%s%s", DIR_TEMPLATE, main_temp->valuestring);

	/* 根据选择的 main 模板文件，生成程序 lua 文件 */
	if ((fp = fopen(maintemp_name, "r")) == NULL) {
		perror("open file");

		return FAIL;
	}
	while (fgets(strline, 100, fp) != NULL) {
		memset(write_line, 0, sizeof(char)*100);
		strcpy(write_line, strline);

		/* 生成左工位 call 示教点.lua */
		if (strstr_ptr = strstr(strline, "<left_point>")) {
			for (i = 0; i < cJSON_GetArraySize(left_screw_array); i++) {
				memset(write_line, 0, sizeof(char)*100);
				sprintf(write_line, "Call:point_%s_left_%d.lua\n", left_wp->valuestring, cJSON_GetArrayItem(left_screw_array, i)->valueint);
				strcat(write_content, write_line);
				memset(strline, 0, sizeof(char)*100);
			}
			continue;
		}

		/* 生成右工位 call 示教点.lua */
		if (strstr_ptr = strstr(strline, "<right_point>")) {
			for (i = 0; i < cJSON_GetArraySize(right_screw_array); i++) {
				memset(write_line, 0, sizeof(char)*100);
				sprintf(write_line, "Call:point_%s_right_%d.lua\n", right_wp->valuestring, cJSON_GetArrayItem(right_screw_array, i)->valueint);
				strcat(write_content, write_line);
				memset(strline, 0, sizeof(char)*100);
			}
			continue;
		}

		/* 左工位工艺参数信息模板替换 */
		if (strstr_ptr = strstr(strline, "<left_floattime>")) {
			memset(strstr_head, 0, 100);
			strncpy(strstr_head, strline, (strstr_ptr - strline));
			memset(write_line, 0, sizeof(char)*100);
			sprintf(write_line, "%s%d%s", strstr_head, left_float_time, (strstr_ptr + strlen("<left_floattime>")));
			memset(strline, 0, sizeof(char)*100);
			strcpy(strline, write_line);
		}
		if (strstr_ptr = strstr(strline, "<left_sliptime>")) {
			memset(strstr_head, 0, 100);
			strncpy(strstr_head, strline, (strstr_ptr - strline));
			memset(write_line, 0, sizeof(char)*100);
			sprintf(write_line, "%s%d%s", strstr_head, left_slip_time, (strstr_ptr + strlen("<left_sliptime>")));
			memset(strline, 0, sizeof(char)*100);
			strcpy(strline, write_line);
		}
		if (strstr_ptr = strstr(strline, "<left_reclaimtime>")) {
			memset(strstr_head, 0, 100);
			strncpy(strstr_head, strline, (strstr_ptr - strline));
			memset(write_line, 0, sizeof(char)*100);
			sprintf(write_line, "%s%d%s", strstr_head, left_screw_time, (strstr_ptr + strlen("<left_reclaimtime>")));
			memset(strline, 0, sizeof(char)*100);
			strcpy(strline, write_line);
		}
		if (strstr_ptr = strstr(strline, "<left_dispeningtime>")) {
			memset(strstr_head, 0, 100);
			strncpy(strstr_head, strline, (strstr_ptr - strline));
			memset(write_line, 0, sizeof(char)*100);
			sprintf(write_line, "%s%d%s", strstr_head, left_dispensing_time, (strstr_ptr + strlen("<left_dispeningtime>")));
			memset(strline, 0, sizeof(char)*100);
			strcpy(strline, write_line);
		}

		/* 右工位工艺参数信息模板替换 */
		if (strstr_ptr = strstr(strline, "<right_floattime>")) {
			memset(strstr_head, 0, 100);
			strncpy(strstr_head, strline, (strstr_ptr - strline));
			memset(write_line, 0, sizeof(char)*100);
			sprintf(write_line, "%s%d%s", strstr_head, right_float_time, (strstr_ptr + strlen("<right_floattime>")));
			memset(strline, 0, sizeof(char)*100);
			strcpy(strline, write_line);
		}
		if (strstr_ptr = strstr(strline, "<right_sliptime>")) {
			memset(strstr_head, 0, 100);
			strncpy(strstr_head, strline, (strstr_ptr - strline));
			memset(write_line, 0, sizeof(char)*100);
			sprintf(write_line, "%s%d%s", strstr_head, right_slip_time, (strstr_ptr + strlen("<right_sliptime>")));
			memset(strline, 0, sizeof(char)*100);
			strcpy(strline, write_line);
		}
		if (strstr_ptr = strstr(strline, "<right_reclaimtime>")) {
			memset(strstr_head, 0, 100);
			strncpy(strstr_head, strline, (strstr_ptr - strline));
			memset(write_line, 0, sizeof(char)*100);
			sprintf(write_line, "%s%d%s", strstr_head, right_screw_time, (strstr_ptr + strlen("<right_reclaimtime>")));
			memset(strline, 0, sizeof(char)*100);
			strcpy(strline, write_line);
		}
		if (strstr_ptr = strstr(strline, "<right_dispeningtime>")) {
			memset(strstr_head, 0, 100);
			strncpy(strstr_head, strline, (strstr_ptr - strline));
			memset(write_line, 0, sizeof(char)*100);
			sprintf(write_line, "%s%d%s", strstr_head, right_dispensing_time, (strstr_ptr + strlen("<right_dispeningtime>")));
			memset(strline, 0, sizeof(char)*100);
			strcpy(strline, write_line);
		}

		strcat(write_content, write_line);
		memset(strline, 0, sizeof(char)*100);
	}

	sprintf(mainlua_name, "/root/web/file/user/%s", gp_name->valuestring);
	if (write_file(mainlua_name, write_content) == FAIL) {
		perror("write file");
		free(write_content);
		write_content = NULL;

		return FAIL;
	}
	free(write_content);
	write_content = NULL;
	fclose(fp);

	return SUCCESS;
}

/* save custom pause */
static int torque_save_custom_pause(const cJSON *data_json)
{
	char sql[1204] = {0};
	cJSON *id = NULL;
	cJSON *title = NULL;
	cJSON *content = NULL;

	id = cJSON_GetObjectItem(data_json, "modal_func_id");
	title = cJSON_GetObjectItem(data_json, "modal_title");
	content = cJSON_GetObjectItem(data_json, "modal_content");
	if (id == NULL || title == NULL || content == NULL || id->valuestring == NULL || title->valuestring == NULL || content->valuestring == NULL) {
		perror("json");

		return FAIL;
	}

	sprintf(sql, "update torquesys_custom set modal_func_id='%s', modal_title='%s', modal_content='%s' where modal_func_id='%s';", id->valuestring, title->valuestring, content->valuestring, id->valuestring);

	if (change_info_sqlite3(DB_TORQUE_CUSTOM, sql) == -1) {
		perror("database");

		return FAIL;
	}

	return SUCCESS;
}

/* modify network */
static int modify_network(const cJSON *data_json)
{
	FILE *fp = NULL;
	char strline[LEN_100] = {0};
	char write_line[LEN_100] = {0};
	char write_content[LEN_100*100] = {0};
	cJSON *ip_json = NULL;
	cJSON *ctrl_ip = NULL;
	cJSON *user_ip = NULL;
	cJSON *PI_ip = NULL;
	cJSON *port_json = NULL;
	cJSON *webapp_port = NULL;

	ip_json = cJSON_GetObjectItem(data_json, "ip");
	if (ip_json == NULL || ip_json->type != cJSON_Object) {
		perror("json");

		return FAIL;
	}
	ctrl_ip = cJSON_GetObjectItem(ip_json, "ctrl_ip");
	user_ip = cJSON_GetObjectItem(ip_json, "user_ip");
	PI_ip = cJSON_GetObjectItem(ip_json, "PI_ip");
	if (ctrl_ip == NULL || ctrl_ip->valuestring == NULL || user_ip == NULL || user_ip->valuestring == NULL || PI_ip == NULL || PI_ip->valuestring == NULL) {
		perror("json");

		return FAIL;
	}
	if (strstr(ctrl_ip->valuestring, "192.168.57")) {
		perror("ip set fail: same 192.168.57.XXX");

		return FAIL;
	}

	port_json = cJSON_GetObjectItem(data_json, "port");
	if (port_json == NULL || port_json->type != cJSON_Object) {
		perror("json");

		return FAIL;
	}
	webapp_port = cJSON_GetObjectItem(port_json, "webapp_port");
	if (webapp_port == NULL || webapp_port->valuestring == NULL) {
		perror("json");

		return FAIL;
	}

	if ((fp = fopen(ROBOT_CFG, "r")) == NULL) {
		perror("open file");

		return FAIL;
	}
	while (fgets(strline, LEN_100, fp) != NULL) {
		bzero(write_line, sizeof(char)*LEN_100);
		if (strstr(strline, "CTRL_IP = ")) {
			sprintf(write_line, "CTRL_IP = %s\n", ctrl_ip->valuestring);
		} else if (strstr(strline, "PI_IP = ")) {
			sprintf(write_line, "PI_IP = %s\n", PI_ip->valuestring);
		} else if (strstr(strline, "WebAPP_Port = ")) {
			sprintf(write_line, "WebAPP_Port = %s\n", webapp_port->valuestring);
		} else {
			strcpy(write_line, strline);
		}

		strcat(write_content, write_line);
		bzero(strline, sizeof(char)*LEN_100);
	}
	fclose(fp);

	if (write_file(ROBOT_CFG, write_content) == FAIL) {
		perror("write file");

		return FAIL;
	}

	/** 发送控制器和树莓派 IP 到树莓派 */
	memset(SERVER_IP, 0, 20);
	strcpy(SERVER_IP, ctrl_ip->valuestring);
	memset(SERVER_PI_IP, 0, 20);
	strcpy(SERVER_PI_IP, PI_ip->valuestring);
	socket_pi_cmd.send_flag = 1;

	/**
		代码中调用 write/read 等文件系统函数时,
	    看似程序已经返回文件写入或读取成功，
		实际硬盘上的文件尚未更改，
		需要一定的等待时间
	*/
	sleep(10);

	return SUCCESS;
}

/* modify customcfg */
static int modify_customcfg(const cJSON *data_json)
{
	FILE *fp = NULL;
	char strline[LEN_100] = {0};
	char write_line[LEN_100] = {0};
	char write_content[LEN_100*100] = {0};
	cJSON *port = NULL;
	cJSON *type = NULL;
	cJSON *ctrl_ip = NULL;
	cJSON *PI_ip = NULL;

	ctrl_ip = cJSON_GetObjectItem(data_json, "ctrl_ip");
	PI_ip = cJSON_GetObjectItem(data_json, "PI_ip");
	port = cJSON_GetObjectItem(data_json, "port");
	type = cJSON_GetObjectItem(data_json, "type");
	if (ctrl_ip == NULL || ctrl_ip->valuestring == NULL || PI_ip == NULL || PI_ip->valuestring == NULL || port == NULL  || port->valuestring == NULL|| type == NULL || type->valuestring == NULL) {
		perror("json");

		return FAIL;
	}

	if (strstr(ctrl_ip->valuestring, "192.168.57")) {
		perror("ip set fail: same 192.168.57.XXX");

		return FAIL;
	}

	if ((fp = fopen(ROBOT_CFG, "r")) == NULL) {
		perror("open file");

		return FAIL;
	}
	while (fgets(strline, LEN_100, fp) != NULL) {
		bzero(write_line, sizeof(char)*LEN_100);
		if (strstr(strline, "CTRL_IP = ")) {
			sprintf(write_line, "CTRL_IP = %s\n", ctrl_ip->valuestring);
		} else if (strstr(strline, "PI_IP = ")) {
			sprintf(write_line, "PI_IP = %s\n", PI_ip->valuestring);
		} else if (strstr(strline, "CTRL_Port = ")) {
			sprintf(write_line, "CTRL_Port = %s\n", port->valuestring);
		} else if (strstr(strline, "Protocol_Type = ")) {
			sprintf(write_line, "Protocol_Type = %s\n", type->valuestring);
		} else {
			strcpy(write_line, strline);
		}

		strcat(write_content, write_line);
		bzero(strline, sizeof(char)*LEN_100);
	}
	fclose(fp);

	if (write_file(ROBOT_CFG, write_content) == FAIL) {
		perror("write file");

		return FAIL;
	}

	/** 发送控制器和树莓派 IP 到树莓派 */
	memset(SERVER_IP, 0, 20);
	strcpy(SERVER_IP, ctrl_ip->valuestring);
	memset(SERVER_PI_IP, 0, 20);
	strcpy(SERVER_PI_IP, PI_ip->valuestring);
	socket_pi_cmd.send_flag = 1;

	/**
		代码中调用 write/read 等文件系统函数时,
	    看似程序已经返回文件写入或读取成功，
		实际硬盘上的文件尚未更改，
		需要一定的等待时间
	*/
	sleep(10);

	return SUCCESS;
}

/* save blockly workspace */
static int save_blockly_workspace(const cJSON *data_json)
{
	char dir_filename[100] = {0};
	cJSON *ws_name = NULL;
	cJSON *ws_xml_text = NULL;
	cJSON *ws_code = NULL;
	cJSON *root_json = NULL;
	char *buf = NULL;
	int ret = FAIL;

	ws_name = cJSON_GetObjectItem(data_json, "ws_name");
	ws_xml_text = cJSON_GetObjectItem(data_json, "ws_xml_text");
	ws_code = cJSON_GetObjectItem(data_json, "ws_code");
	if (ws_name == NULL || ws_xml_text == NULL || ws_code == NULL || ws_name->valuestring == NULL || ws_xml_text->valuestring == NULL || ws_code->valuestring == NULL) {
		perror("json");

		return FAIL;
	}

	root_json = cJSON_CreateObject();
	cJSON_AddStringToObject(root_json, "ws_xml_text", ws_xml_text->valuestring);
	cJSON_AddStringToObject(root_json, "ws_code", ws_code->valuestring);
	buf = cJSON_Print(root_json);

	sprintf(dir_filename, "%s%s", DIR_BLOCK, ws_name->valuestring);
	ret = write_file(dir_filename, buf);
	free(buf);
	buf = NULL;
	cJSON_Delete(root_json);
	root_json = NULL;

	return ret;
}

/** modify PI cfg */
static int modify_PI_cfg(const cJSON *data_json)
{
	char *buf = NULL;
	int write_ret = FAIL;
	char *config_content = NULL;
	cJSON *config_json = NULL;
	cJSON *old_enable = NULL;
	cJSON *enable = NULL;

	config_content = get_file_content(FILE_PI_CFG);
	if (config_content == NULL || strcmp(config_content, "NO_FILE") == 0 || strcmp(config_content, "Empty") == 0) {
		perror("get file content");

		return FAIL;
	}
	config_json = cJSON_Parse(config_content);
	free(config_content);
	config_content = NULL;
	if (config_json == NULL) {
		perror("cJSON_Parse");

		return FAIL;
	}
	old_enable = cJSON_GetObjectItem(config_json, "enable");
	if (old_enable == NULL) {
		perror("json");

		return FAIL;
	}

	enable = cJSON_GetObjectItem(data_json, "enable");
	if (enable == NULL) {
		perror("json");

		return FAIL;
	}
	/* update pi pthread enable */
	pi_pt_status.enable = enable->valueint;
	pi_pt_cmd.enable = enable->valueint;

	printf("pthread_kill(pi_pt_status.t_pi, 0) = %d\n", pthread_kill(pi_pt_status.t_pi, 0));
	printf("pthread_kill(pi_pt_cmd.t_pi, 0) = %d\n", pthread_kill(pi_pt_cmd.t_pi, 0));
	printf("ESRCH = %d\n", ESRCH);
	/** 关闭使用示教器树莓派 */
	if (enable->valueint == 0) {
		if (old_enable->valueint == 1) {
			/** pi status 线程存在 */
			//if (pthread_kill(pi_pt_status.t_pi, 0) == 0) {
				printf("before pthread_cancel pi status\n");
				if (pthread_cancel(pi_pt_status.t_pi) != 0) {
					perror("pthread_cancel");

					return FAIL;
				}
				printf("before pthread_join pi status\n");
				/* 当前线程挂起, 等待创建线程返回，获取该线程的返回值后，当前线程退出 */
				if (pthread_join(pi_pt_status.t_pi, NULL)) {
					perror("pthread_join");

					return FAIL;
				}
				printf("after pthread_join pi status\n");
				/* set socket status: disconnected */
				socket_pi_status.connect_status = 0;
				/* close socket fd */
				close(socket_pi_status.fd);
			//}
			/** pi cmd 线程存在 */
			//if (pthread_kill(pi_pt_cmd.t_pi, 0) == 0) {
				printf("before pthread_join pi cmd\n");
				/* 当前线程挂起, 等待创建线程返回，获取该线程的返回值后，当前线程退出 */
				if (pthread_join(pi_pt_cmd.t_pi, NULL)) {
					perror("pthread_join");

					return FAIL;
				}
				printf("after pthread_join pi cmd\n");
			//}
		}
	/** 开启使用示教器树莓派 */
	} else {
		if (old_enable->valueint == 0) {
			/* create socket_pi_status thread */
			if (pthread_create(&pi_pt_status.t_pi, NULL, (void *)&socket_pi_status_thread, (void *)PI_STATUS_PORT)) {
				perror("pthread_create");

				return FAIL;
			}
			/* create socket_pi_cmd thread */
			if (pthread_create(&pi_pt_cmd.t_pi, NULL, (void *)&socket_pi_cmd_thread, (void *)PI_CMD_PORT)) {
				perror("pthread_create");

				return FAIL;
			}
		}
	}
	/* cjson delete */
	cJSON_Delete(config_json);
	config_json = NULL;

	buf = cJSON_Print(data_json);
	write_ret = write_file(FILE_PI_CFG, buf);
	free(buf);
	buf = NULL;
	if (write_ret == FAIL) {
		perror("write file");

		return FAIL;
	}

	return SUCCESS;
}

/** robottype_password */
static int robottype_password(const cJSON *data_json)
{
	cJSON *password = NULL;

	password = cJSON_GetObjectItem(data_json, "password");
	if (password == NULL || password->valuestring == NULL) {
		perror("json");

		return FAIL;
	}

	if (strcmp(password->valuestring, RTS_PASSWORD) == 0) {

		return SUCCESS;
	} else {

		return FAIL;
	}
}

/* do some user actions basic on web */
void act(Webs *wp)
{
	int ret = FAIL;
	char *buf = NULL;
	char *cmd = NULL;
	cJSON *command = NULL;
	cJSON *data = NULL;
	cJSON *data_json = NULL;
	cJSON *post_type = NULL;
	char log_content[1024] = {0};
	char en_log_content[1024] = {0};
	char jap_log_content[1024] = {0};

	data = cJSON_Parse(wp->input.servp);
	if (data == NULL) {
		perror("json");
		goto end;
	}
	printf("data:%s\n", buf = cJSON_Print(data));
	free(buf);
	buf = NULL;
	/* get data json */
	data_json = cJSON_GetObjectItem(data, "data");
	if (data_json == NULL || data_json->type != cJSON_Object) {
		perror("json");
		goto end;
	}
	/* get cmd */
	command = cJSON_GetObjectItem(data, "cmd");
	if (command == NULL) {
		perror("json");
		goto end;
	}
	cmd = command->valuestring;
	//printf("cmd = %s\n", cmd);

	// cmd_auth "0"
	if (!strcmp(cmd, "set_sys_logcount") || !strcmp(cmd, "set_sys_lifespan") || !strcmp(cmd, "set_sys_language") || !strcmp(cmd, "set_ODM_cfg") || !strcmp(cmd, "save_accounts") || !strcmp(cmd, "shutdown") || !strcmp(cmd, "factory_reset") || !strcmp(cmd, "odm_password") || !strcmp(cmd, "save_robot_type") || !strcmp(cmd, "modify_network") || !strcmp(cmd, "modify_customcfg") || !strcmp(cmd, "modify_PI_cfg") || !strcmp(cmd, "robottype_password")) {
		if (!authority_management("0")) {
			perror("authority_management");

			goto auth_end;
		}
	// cmd_auth "1"
	} else if (!strcmp(cmd, "save_lua_file") || !strcmp(cmd, "remove_lua_file") || !strcmp(cmd, "save_template_file") || !strcmp(cmd, "remove_template_file") || !strcmp(cmd, "rename_lua_file") || !strcmp(cmd, "modify_tool_cdsystem") || !strcmp(cmd, "modify_wobj_tool_cdsystem") || !strcmp(cmd, "modify_ex_tool_cdsystem") || !strcmp(cmd, "modify_exaxis_cdsystem") || !strcmp(cmd, "save_point") || !strcmp(cmd, "save_laser_point") || !strcmp(cmd, "modify_point") || !strcmp(cmd, "remove_points") || !strcmp(cmd, "set_ptn_cfg") || !strcmp(cmd, "plugin_enable") || !strcmp(cmd, "plugin_remove") || !strcmp(cmd, "clear_DH_file") || !strcmp(cmd, "save_DH_point") || !strcmp(cmd, "rename_var") || !strcmp(cmd, "move_to_home_point") || !strcmp(cmd, "torque_save_cfg") || !strcmp(cmd, "torque_ensure_points") || !strcmp(cmd, "torque_generate_program") || !strcmp(cmd, "set_DIO_cfg") || !strcmp(cmd, "set_status_flg") || !strcmp(cmd, "clear_product_info") || !strcmp(cmd, "torque_save_custom_pause") || !strcmp(cmd, "save_blockly_workspace")) {
		if (!authority_management("1")) {
			perror("authority_management");

			goto auth_end;
		}
	}
	/*
	// cmd_auth "2"
	} else if () {
		if (!authority_management("2")) {
			perror("authority_management");

			goto auth_end;
		}
	}
	*/

	if (!strcmp(cmd, "save_lua_file")) {
		ret = save_lua_file(data_json);
		strcpy(log_content, "保存当前程序示教文件");
		strcpy(en_log_content, "Save the current program teaching file");
		strcpy(jap_log_content, "現在のプログラム表示ファイルを保存します");
	} else if (!strcmp(cmd, "remove_lua_file")) {
		ret = remove_lua_file(data_json);
		strcpy(log_content, "删除当前程序示教文件");
		strcpy(en_log_content, "Deletes the current program teaching file");
		strcpy(jap_log_content, "現在のプログラム表示ファイルを削除します");
	} else if (!strcmp(cmd, "save_template_file")) {
		ret = save_template_file(data_json);
		strcpy(log_content, "保存程序示教模板文件");
		strcpy(en_log_content, "Save the program teaching template file");
		strcpy(jap_log_content, "プログラム教示テンプレートファイルを保存します");
	} else if (!strcmp(cmd, "remove_template_file")) {
		ret = remove_template_file(data_json);
		strcpy(log_content, "删除程序示教模板文件");
		strcpy(en_log_content, "Delete program teaching template file");
		strcpy(jap_log_content, "プログラム教示テンプレートファイルを削除します");
	} else if (!strcmp(cmd, "rename_lua_file")) {
		ret = rename_lua_file(data_json);
		strcpy(log_content, "重命名当前程序示教文件");
		strcpy(en_log_content, "Rename the current program teaching file");
		strcpy(jap_log_content, "現在のプログラム表示ファイルの名前を変更します");
	} else if (!strcmp(cmd, "modify_tool_cdsystem")) {
		ret = modify_tool_cdsystem(data_json);
		strcpy(log_content, "修改工具坐标系");
		strcpy(en_log_content, "Modify the tool coordinate system");
		strcpy(jap_log_content, "ツール座標系の修正");
	} else if (!strcmp(cmd, "modify_wobj_tool_cdsystem")) {
		ret = modify_wobj_tool_cdsystem(data_json);
		strcpy(log_content, "修改工件工具坐标系");
		strcpy(en_log_content, "Modify the workpiece tool coordinate system");
		strcpy(jap_log_content, "ワークツール座標系を修正します");
	} else if (!strcmp(cmd, "modify_ex_tool_cdsystem")) {
		ret = modify_ex_tool_cdsystem(data_json);
		strcpy(log_content, "修改外部工具坐标系");
		strcpy(en_log_content, "Modify the external tool coordinate system");
		strcpy(jap_log_content, "外部ツール座標系を修正します");
	} else if (!strcmp(cmd, "modify_exaxis_cdsystem")) {
		ret = modify_exaxis_cdsystem(data_json);
		strcpy(log_content, "修改外部轴工具坐标系");
		strcpy(en_log_content, "Modify the external axis tool coordinate system");
		strcpy(jap_log_content, "外部軸ツール座標系を修正します");
	} else if (!strcmp(cmd, "save_point")) {
		ret = save_point(data_json);
		strcpy(log_content, "保存点信息");
		strcpy(en_log_content, "save point");
		strcpy(jap_log_content, "ポイント情報を保存する");
	} else if (!strcmp(cmd, "save_laser_point")) {
		ret = save_laser_point(data_json);
		strcpy(log_content, "保存激光点信息");
		strcpy(en_log_content, "save laser point");
		strcpy(jap_log_content, "レーザーポイント情報を保存します");
	} else if (!strcmp(cmd, "modify_point")) {
		ret = modify_point(data_json);
		strcpy(log_content, "修改记录点信息");
		strcpy(en_log_content, "modify point");
		strcpy(jap_log_content, "ログポイント情報を修正します");
	} else if (!strcmp(cmd, "remove_points")) {
		ret = remove_points(data_json);
		strcpy(log_content, "移除点信息");
		strcpy(en_log_content, "remove points");
		strcpy(jap_log_content, "ポイント情報を除去する");
	} else if (!strcmp(cmd, "change_type")) {
		ret = change_type(data_json);
		strcpy(log_content, "切换实体和虚拟机器人");
		strcpy(en_log_content, "Switch between physical and virtual robots");
		strcpy(jap_log_content, "実物と仮想ロボットを切り替えます");
	} else if (!strcmp(cmd, "set_sys_logcount")) {
		ret = set_sys_logcount(data_json);
		strcpy(log_content, "设置系统日志");
		strcpy(en_log_content, "Setting system Logs");
		strcpy(jap_log_content, "システムログを設定する");
	} else if (!strcmp(cmd, "set_sys_lifespan")) {
		ret = set_sys_lifespan(data_json);
		strcpy(log_content, "设置系统超时时间");
		strcpy(en_log_content, "Example Set the system timeout period");
		strcpy(jap_log_content, "システムタイムアウト時間を設定する");
	} else if (!strcmp(cmd, "set_sys_language")) {
		ret = set_sys_language(data_json);
		strcpy(log_content, "设置系统语言");
		strcpy(en_log_content, "Set up system language");
		strcpy(jap_log_content, "システム言語を設定する");
	} else if (!strcmp(cmd, "set_ODM_cfg")) {
		ret = set_ODM_cfg(data_json);
		strcpy(log_content, "设置 ODM 配置");
		strcpy(en_log_content, "Set up the ODM configuration");
		strcpy(jap_log_content, "odm構成を設定します");
	} else if (!strcmp(cmd, "set_ptn_cfg")) {
		ret = set_ptn_cfg(data_json);
		strcpy(log_content, "设置示教点配置");
		strcpy(en_log_content, "Set the teaching point configuration");
		strcpy(jap_log_content, "示教点配置を設ける");
	} else if (!strcmp(cmd, "save_accounts")) {
		ret = save_accounts(data_json);
		strcpy(log_content, "保存账户信息");
		strcpy(en_log_content, "save accounts");
		strcpy(jap_log_content, "口座情報を保存する");
	} else if (!strcmp(cmd, "plugin_enable")) {
		ret = plugin_enable(data_json);
		strcpy(log_content, "启用/停用外设插件");
		strcpy(en_log_content, "Enable/disable peripheral plug-ins");
		strcpy(jap_log_content, "周辺機器プラグインを有効/無効にします");
	} else if (!strcmp(cmd, "plugin_remove")) {
		ret = plugin_remove(data_json);
		strcpy(log_content, "删除外设插件");
		strcpy(en_log_content, "Remove the peripheral plug-in");
		strcpy(jap_log_content, "周辺機器プラグインを削除する");
	} else if (!strcmp(cmd, "clear_DH_file")) {
		ret = clear_DH_file(data_json);
		strcpy(log_content, "清空DH参数校准数据采集文件");
		strcpy(en_log_content, "Clear the DH parameter calibration data collection file");
		strcpy(jap_log_content, "dhパラメータを空にしてデータ収集ファイルを較正します");
	} else if (!strcmp(cmd, "save_DH_point")) {
		ret = save_DH_point(data_json);
		strcpy(log_content, "DH 参数校准数据采集下发");
		strcpy(en_log_content, "DH parameter calibration data collection and issuance");
		strcpy(jap_log_content, "dhパラメータキャリブレーションデータが取得されます");
	} else if (!strcmp(cmd, "shutdown")) {
		ret = shutdown_system(data_json);
		strcpy(log_content, "系统关机");
		strcpy(en_log_content, "Shutdown");
		strcpy(jap_log_content, "システムのシャットダウン");
	} else if (!strcmp(cmd, "factory_reset")) {
		ret = factory_reset(data_json);
		strcpy(log_content, "恢复出厂值");
		strcpy(en_log_content, "Factory reset");
		strcpy(jap_log_content, "出荷値を回復する");
	} else if (!strcmp(cmd, "odm_password")) {
		ret = odm_password(data_json);
		strcpy(log_content, "认证ODM密码");
		strcpy(en_log_content, "Authenticate the ODM password");
		strcpy(jap_log_content, "odmパスワードを認証する");
	} else if (!strcmp(cmd, "save_robot_type")) {
		ret = save_robot_type(data_json);
		strcpy(log_content, "保存机器人型号");
		strcpy(en_log_content, "Save the robot type");
		strcpy(jap_log_content, "ロボット型保存");
	} else if (!strcmp(cmd, "rename_var")) {
		ret = rename_var(data_json);
		strcpy(log_content, "变量重命名");
		strcpy(en_log_content, "variable renaming");
		strcpy(jap_log_content, "変数の名前変更");
	} else if (!strcmp(cmd, "move_to_home_point")) {
		ret = move_to_home_point(data_json);
		strcpy(log_content, "移至原点");
		strcpy(en_log_content, "Move to the origin point");
		strcpy(jap_log_content, "原点に移す");
	} else if (!strcmp(cmd, "torque_save_cfg")) {
		ret = torque_save_cfg(data_json);
		strcpy(log_content, "设置扭矩参数");
		strcpy(en_log_content, "Set torque parameters");
		strcpy(jap_log_content, "トルクパラメータを設定する");
	} else if (!strcmp(cmd, "torque_ensure_points")) {
		ret = torque_ensure_points(data_json);
		strcpy(log_content, "确认扭矩示教点");
		strcpy(en_log_content, "Ensure torque teaching points");
		strcpy(jap_log_content, "トルク表示点を確認する");
	} else if (!strcmp(cmd, "torque_generate_program")) {
		ret = torque_generate_program(data_json);
		strcpy(log_content, "扭矩: 生成示教程序");
		strcpy(en_log_content, "torque: Generate the instruction program");
		strcpy(jap_log_content, "トルク: 表示手順を生成する");
	} else if (!strcmp(cmd, "set_DIO_cfg")) {
		ret = set_DIO_cfg(data_json);
		strcpy(log_content, "设置扭矩 DIO 配置");
		strcpy(en_log_content, "torque: setting the DIO configuration");
		strcpy(jap_log_content, "トルク: dio構成の設定");
	} else if (!strcmp(cmd, "set_status_flag")) {
		ret = set_status_flag(data_json);
		strcpy(log_content, "设置状态展示页面标志位");
		strcpy(en_log_content, "Set the flag of the status display page");
		strcpy(jap_log_content, "設定状態表示ページフラグビット");
	} else if (!strcmp(cmd, "clear_product_info")) {
		ret = clear_product_info(data_json);
		strcpy(log_content, "扭矩: 生产数据清除");
		strcpy(en_log_content, "torque: Production data cleanup");
		strcpy(jap_log_content, "トルク: 生産データ消去");
	} else if (!strcmp(cmd, "torque_save_custom_pause")) {
		ret = torque_save_custom_pause(data_json);
		strcpy(log_content, "扭矩: 保存自定义提示内容");
		strcpy(en_log_content, "torque: Save the custom prompt content");
		strcpy(jap_log_content, "トルク: カスタマイズのヒントを保存する");
	} else if (!strcmp(cmd, "modify_network")) {
		ret = modify_network(data_json);
		strcpy(log_content, "修改网络配置");
		strcpy(en_log_content, "Modifying Network Configurations");
		strcpy(jap_log_content, "ネットワーク構成を変更する");
	} else if (!strcmp(cmd, "modify_customcfg")) {
		ret = modify_customcfg(data_json);
		strcpy(log_content, "修改客户配置");
		strcpy(en_log_content, "Modifying Customer Configurations");
		strcpy(jap_log_content, "顧客プロファイルの変更");
	} else if (!strcmp(cmd, "save_blockly_workspace")) {
		ret = save_blockly_workspace(data_json);
		strcpy(log_content, "保存工作区内容");
		strcpy(en_log_content, "Save workspace contents");
		strcpy(jap_log_content, "作業スペースを保存する");
	} else if (!strcmp(cmd, "modify_PI_cfg")) {
		ret = modify_PI_cfg(data_json);
		strcpy(log_content, "修改示教器（树莓派）配置");
		strcpy(en_log_content, "Modify the display (Raspberry PI) configuration");
		strcpy(jap_log_content, "ラズベリーパイの構成を修正する");
	} else if (!strcmp(cmd, "robottype_password")) {
		ret = robottype_password(data_json);
		strcpy(log_content, "认证机器人型号配置密码");
		strcpy(en_log_content, "Authentication robot model configures a password");
		strcpy(jap_log_content, "認証ロボットモデル設定パスワード");
	} else {
		perror("cmd not found");
		goto end;
	}
	if (ret == FAIL) {
		perror("ret fail");
		goto end;
	}
	my_syslog("应用操作", log_content, cur_account.username);
	my_en_syslog("apply operation", en_log_content, cur_account.username);
	my_jap_syslog("応用オペレーション", jap_log_content, cur_account.username);
	/* cjson delete */
	cJSON_Delete(data);
	data = NULL;
	websSetStatus(wp, 200);
	websWriteHeaders(wp, -1, 0);
	websWriteEndHeaders(wp);
	websWrite(wp, "success");
	websDone(wp);

	return;

auth_end:
	my_syslog("应用操作", "当前用户无相应指令操作权限", cur_account.username);
	my_en_syslog("apply operation", "The current user has no authority to operate the corresponding instruction", cur_account.username);
	my_jap_syslog("応用オペレーション", "現在のユーザには、対応するコマンド操作権限がありません", cur_account.username);
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
	my_syslog("应用操作", "应用操作失败", cur_account.username);
	my_en_syslog("apply operation", "apply operation fail", cur_account.username);
	my_jap_syslog("応用オペレーション", "アプリケーション動作に失敗する", cur_account.username);
	/* cjson delete */
	cJSON_Delete(data);
	data = NULL;
	websSetStatus(wp, 403);
	websWriteHeaders(wp, -1, 0);
	websWriteEndHeaders(wp);
	websWrite(wp, "fail");
	websDone(wp);
	return;
}
