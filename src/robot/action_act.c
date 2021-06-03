
/********************************* Includes ***********************************/

#include    "goahead.h"
#include	"cJSON.h"
#include 	"tools.h"
#include 	"robot_socket.h"
#include    "action_act.h"
#include 	"mysqlite3.h"

/********************************* Defines ************************************/

extern CTRL_STATE ctrl_state;
extern CTRL_STATE vir_ctrl_state;
extern int robot_type;
extern int language;
extern ACCOUNT_INFO cur_account;

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
static int set_syscfg(const cJSON *data_json);
static int set_language(const cJSON *data_json);
static int set_ODM_cfg(const cJSON *data_json);
static int ptnbox(const cJSON *data_json);
static int save_accounts(const cJSON *data_json);
static int shutdown_system(const cJSON *data_json);
static int plugin_enable(const cJSON *data_json);
static int plugin_remove(const cJSON *data_json);
static int clear_DH_file(const cJSON *data_json);
static int save_DH_point(const cJSON *data_json);
static int factory_reset(const cJSON *data_json);
static int odm_password(const cJSON *data_json);
static int save_robot_type(const cJSON *data_json);

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

	sprintf(sql, "insert into coordinate_system(name,id,x,y,z,rx,ry,rz) values('%s','%s','%s','%s','%s','%s','%s','%s');"\
					, name->valuestring, id->valuestring, x->valuestring, y->valuestring, z->valuestring, rx->valuestring, ry->valuestring, rz->valuestring);

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

	sprintf(sql, "insert into wobj_coordinate_system(name,id,x,y,z,rx,ry,rz) values('%s','%s','%s','%s','%s','%s','%s','%s');"\
					, name->valuestring, id->valuestring, x->valuestring, y->valuestring, z->valuestring, rx->valuestring, ry->valuestring, rz->valuestring);

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

	sprintf(sql,"insert into et_coordinate_system(name,user_name,id,ex,ey,ez,erx,ery,erz,tx,ty,tz,trx,try,trz) "\
						"values('%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s');"\
						,name->valuestring, user_name->valuestring, id->valuestring, ex->valuestring, ey->valuestring, ez->valuestring, erx->valuestring, ery->valuestring, erz->valuestring,\
						tx->valuestring, ty->valuestring, tz->valuestring, trx->valuestring, try->valuestring, trz->valuestring);

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

	sprintf(sql, "insert into exaxis_coordinate_system(name,exaxisid,id,x,y,z,rx,ry,rz,flag) values('%s','%s','%s','%s','%s','%s','%s','%s','%s','%s');"\
					, name->valuestring, exaxisid->valuestring, id->valuestring, x->valuestring, y->valuestring, z->valuestring, rx->valuestring, ry->valuestring, rz->valuestring, flag->valuestring);

	if (change_info_sqlite3( DB_EXAXIS_CDSYSTEM, sql) == -1) {
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

	if (robot_type == 1) { // "1" 代表实体机器人
		state = &ctrl_state;
	} else { // "0" 代表虚拟机器人
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
		sprintf(joint_value_string[i], "%.3lf", state->jt_cur_pos[i]); //数据库存储
	}
	for (i = 0; i < 6; i++) {
		sprintf(tcp_value_string[i], "%.3lf", state->tl_cur_pos[i]);
	}
	sprintf(E1, "%.3lf", state->exaxis_status[0].exAxisPos);
	sprintf(E2, "%.3lf", state->exaxis_status[1].exAxisPos);
	sprintf(E3, "%.3lf", state->exaxis_status[2].exAxisPos);
	sprintf(E4, "%.3lf", state->exaxis_status[3].exAxisPos);
	sprintf(sql, "insert into points(name,speed,elbow_speed,acc,elbow_acc,toolnum,workpiecenum,j1,j2,j3,j4,j5,j6,E1,E2,E3,E4,x,y,z,rx,ry,rz) values('%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s');"\
				, name->valuestring, speed->valuestring, elbow_speed->valuestring,\
				acc->valuestring, elbow_acc->valuestring, toolnum->valuestring, workpiecenum->valuestring,\
				joint_value_string[0], joint_value_string[1], joint_value_string[2], joint_value_string[3], joint_value_string[4], joint_value_string[5], E1, E2, E3, E4,\
				tcp_value_string[0], tcp_value_string[1], tcp_value_string[2], tcp_value_string[3], tcp_value_string[4], tcp_value_string[5]);
	if (change_info_sqlite3(DB_POINTS, sql) == -1) {
		perror("database");

		return FAIL;
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
	sprintf(sql, "insert into points(name,speed,elbow_speed,acc,elbow_acc,toolnum,workpiecenum,j1,j2,j3,j4,j5,j6,E1,E2,E3,E4,x,y,z,rx,ry,rz) values('%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s');"\
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
		memset(sql,0,sizeof(sql));
		sprintf(sql, "delete from points where name = \'%s\'", name_index->valuestring);
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

/* set_syscfg */
static int set_syscfg(const cJSON *data_json)
{
	int ret = FAIL;
	char *buf = NULL;
	char *cfg_content = NULL;
	cJSON *cfg_json = NULL;
	cJSON *count = NULL;

	count = cJSON_GetObjectItem(data_json, "log_count");
	if (count == NULL || count->valuestring == NULL) {
		perror("json");

		return FAIL;
	}

	cfg_content = get_file_content(FILE_CFG);
	if (cfg_content == NULL || strcmp(cfg_content, "NO_FILE") == 0 || strcmp(cfg_content, "Empty") == 0) {
		perror("get file content");

		return FAIL;
	}
	cfg_json = cJSON_Parse(cfg_content);
	if (cfg_json == NULL) {
		perror("cJSON_Parse");

		return FAIL;
	}
	free(cfg_content);
	cfg_content = NULL;

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

/* set_language */
static int set_language(const cJSON *data_json)
{
	int ret = FAIL;
	char *buf = NULL;
	char *cfg_content = NULL;
	cJSON *cfg_json = NULL;
	cJSON *language_json = NULL;

	language_json = cJSON_GetObjectItem(data_json, "language");
	if (language_json == NULL || language_json->valuestring == NULL) {
		perror("json");

		return FAIL;
	}

	cfg_content = get_file_content(FILE_CFG);
	if (cfg_content == NULL || strcmp(cfg_content, "NO_FILE") == 0 || strcmp(cfg_content, "Empty") == 0) {
		perror("get file content");

		return FAIL;
	}
	cfg_json = cJSON_Parse(cfg_content);
	if (cfg_json == NULL) {
		perror("cJSON_Parse");

		return FAIL;
	}
	free(cfg_content);
	cfg_content = NULL;

	if (cJSON_GetObjectItem(cfg_json, "language") == NULL) {
		cJSON_AddStringToObject(cfg_json, "language", language_json->valuestring);
	} else {
		cJSON_ReplaceItemInObject(cfg_json, "language", cJSON_CreateString(language_json->valuestring));
	}
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
	char *cfg_content = NULL;
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

	delete_log_file(0);

	return ret;
}

/* ptnbox */
static int ptnbox(const cJSON *data_json)
{
	int ret = FAIL;
	char *buf = NULL;
	char *cfg_content = NULL;
	cJSON *cfg_json = NULL;
	cJSON *name = NULL;
	cJSON *number = NULL;
	cJSON *laser = NULL;

	name = cJSON_GetObjectItem(data_json, "name");
	if (name == NULL || name->valuestring == NULL) {
		perror("json");

		return FAIL;
	}
	number = cJSON_GetObjectItem(data_json, "number");
	if (number == NULL || number->valuestring == NULL) {
		perror("json");

		return FAIL;
	}
	laser = cJSON_GetObjectItem(data_json, "laser");
	if (laser == NULL || laser->valuestring == NULL) {
		perror("json");

		return FAIL;
	}

	cfg_content = get_file_content(FILE_CFG);
	if (cfg_content == NULL || strcmp(cfg_content, "NO_FILE") == 0 || strcmp(cfg_content, "Empty") == 0) {
		perror("get file content");

		return FAIL;
	}
	cfg_json = cJSON_Parse(cfg_content);
	if (cfg_json == NULL) {
		perror("cJSON_Parse");

		return FAIL;
	}
	free(cfg_content);
	cfg_content = NULL;
	if (cJSON_GetObjectItem(cfg_json, "name") == NULL) {
		cJSON_AddStringToObject(cfg_json, "name", name->valuestring);
	} else {
		cJSON_ReplaceItemInObject(cfg_json, "name", cJSON_CreateString(name->valuestring));
	}
	if (cJSON_GetObjectItem(cfg_json, "number") == NULL) {
		cJSON_AddStringToObject(cfg_json, "number", number->valuestring);
	} else {
		cJSON_ReplaceItemInObject(cfg_json, "number", cJSON_CreateString(number->valuestring));
	}
	if (cJSON_GetObjectItem(cfg_json, "laser") == NULL) {
		cJSON_AddStringToObject(cfg_json, "laser", laser->valuestring);
	} else {
		cJSON_ReplaceItemInObject(cfg_json, "laser", cJSON_CreateString(laser->valuestring));
	}
	buf = cJSON_Print(cfg_json);
	ret = write_file(FILE_CFG, buf);
	free(buf);
	buf = NULL;
	cJSON_Delete(cfg_json);
	cfg_json = NULL;

	return ret;
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
				string_list_free(array, size);

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
			string_list_free(array, size);
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

	memset(cmd, 0, 128);
	sprintf(cmd, "rm -rf %s*", DIR_LOG);
	system(cmd);

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

	return SUCCESS;
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
	char *buf = NULL;
	int write_ret = FAIL;

	buf = cJSON_Print(data_json);
	write_ret = write_file(FILE_ROBOT_TYPE, buf);//write file
	free(buf);
	buf = NULL;
	if (write_ret == FAIL) {
		perror("write file");

		return FAIL;
	}

	return SUCCESS;
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
	// cmd_auth "1"
	if (!strcmp(cmd, "save_lua_file") || !strcmp(cmd, "remove_lua_file") || !strcmp(cmd, "save_template_file") || !strcmp(cmd, "remove_template_file") || !strcmp(cmd, "rename_lua_file") || !strcmp(cmd, "remove_points") || !strcmp(cmd, "set_syscfg") || !strcmp(cmd, "set_language") || !strcmp(cmd, "set_ODM_cfg") || !strcmp(cmd, "ptnbox") || !strcmp(cmd, "modify_tool_cdsystem") || !strcmp(cmd, "modify_wobj_tool_cdsystem") || !strcmp(cmd, "modify_ex_tool_cdsystem") || !strcmp(cmd, "modify_exaxis_cdsystem") || !strcmp(cmd, "factory_reset") || !strcmp(cmd, "shutdown") || !strcmp(cmd, "save_DH_point") || !strcmp(cmd, "clear_DH_file") || !strcmp(cmd, "odm_password")) {
		if (!authority_management("1")) {
			perror("authority_management");
			goto auth_end;
		}
	// cmd_auth "2"
	} else if (!strcmp(cmd, "change_type") || !strcmp(cmd, "save_point") || !strcmp(cmd, "save_laser_point") || !strcmp(cmd, "modify_point") || !strcmp(cmd, "plugin_enable") || !strcmp(cmd, "plugin_remove")) {
		if (!authority_management("2")) {
			perror("authority_management");
			goto auth_end;
		}
	// cmd_auth "0"
	} else if (!strcmp(cmd, "save_accounts") || !strcmp(cmd, "save_robot_type")) {
		if (!authority_management("0")) {
			perror("authority_management");
			goto auth_end;
		}
	}
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
	} else if (!strcmp(cmd, "set_syscfg")) {
		ret = set_syscfg(data_json);
		strcpy(log_content, "设置系统配置");
		strcpy(en_log_content, "Set up system configuration");
		strcpy(jap_log_content, "システム構成を設定する");
	} else if (!strcmp(cmd, "set_language")) {
		ret = set_language(data_json);
		strcpy(log_content, "设置系统语言");
		strcpy(en_log_content, "Set up system language");
		strcpy(jap_log_content, "システム言語を設定する");
	} else if (!strcmp(cmd, "set_ODM_cfg")) {
		ret = set_ODM_cfg(data_json);
		strcpy(log_content, "设置 ODM 配置");
		strcpy(en_log_content, "Set up the ODM configuration");
		strcpy(jap_log_content, "odm構成を設定します");
	} else if (!strcmp(cmd, "ptnbox")) {
		ret = ptnbox(data_json);
		strcpy(log_content, "按钮盒记录点个数限制");
		strcpy(en_log_content, "Limit the number of record points in the button box");
		strcpy(jap_log_content, "ボタンボックス記録ポイント数制限");
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
