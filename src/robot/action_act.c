
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
extern SOCKET_INFO socket_cmd;
extern SOCKET_INFO socket_vir_cmd;
extern ACCOUNT_INFO cur_account;

/********************************* Function declaration ***********************/

static int save_lua_file(const cJSON *data_json);
static int remove_lua_file(const cJSON *data_json);
static int save_template_file(const cJSON *data_json);
static int remove_template_file(const cJSON *data_json);
static int rename_lua_file(const cJSON *data_json);
static int modify_tool_cdsystem(const cJSON *data_json);
static int modify_ex_tool_cdsystem(const cJSON *data_json);
static int modify_exaxis_cdsystem(const cJSON *data_json);
static int save_point(const cJSON *data_json);
static int remove_points(const cJSON *data_json);
static int change_type(const cJSON *data_json);
static int log_management(const cJSON *data_json);
static int save_accounts(const cJSON *data_json);
static int shutdown_system(const cJSON *data_json);
static int plugin_enable(const cJSON *data_json);

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

	name = cJSON_GetObjectItem(data_json, "name");
	exaxisid = cJSON_GetObjectItem(data_json, "exaxisid");
	id = cJSON_GetObjectItem(data_json, "id");
	x = cJSON_GetObjectItem(data_json, "x");
	y = cJSON_GetObjectItem(data_json, "y");
	z = cJSON_GetObjectItem(data_json, "z");
	rx = cJSON_GetObjectItem(data_json, "rx");
	ry = cJSON_GetObjectItem(data_json, "ry");
	rz = cJSON_GetObjectItem(data_json, "rz");
	if(name == NULL || exaxisid == NULL || id == NULL || x == NULL || y == NULL || z == NULL || rx == NULL || ry == NULL|| rz == NULL || name->valuestring == NULL || exaxisid->valuestring == NULL || id->valuestring == NULL || x->valuestring == NULL || y->valuestring == NULL || z->valuestring == NULL || rx->valuestring == NULL || ry->valuestring == NULL || rz->valuestring == NULL) {
		perror("json");
 
		return FAIL;
	}

	sprintf(sql, "insert into exaxis_coordinate_system(name,exaxisid,id,x,y,z,rx,ry,rz) values('%s','%s','%s','%s','%s','%s','%s','%s','%s');"\
					, name->valuestring, exaxisid->valuestring, id->valuestring, x->valuestring, y->valuestring, z->valuestring, rx->valuestring, ry->valuestring, rz->valuestring);

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
	char tcp_value_string[6][10] = {0};
	char joint_value_string[6][10] = {0};
	cJSON *name = NULL;
	cJSON *speed = NULL;
	cJSON *elbow_speed = NULL;
	cJSON *acc = NULL;
	cJSON *elbow_acc = NULL;
	cJSON *toolnum = NULL;
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
	if (name == NULL || speed == NULL || acc == NULL || elbow_speed == NULL || elbow_acc == NULL || toolnum == NULL || name->valuestring == NULL || speed->valuestring == NULL || acc->valuestring == NULL || elbow_speed->valuestring == NULL || elbow_acc->valuestring == NULL || toolnum->valuestring == NULL) {
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
	sprintf(sql, "insert into points(name,speed,elbow_speed,acc,elbow_acc,toolnum,j1,j2,j3,j4,j5,j6,E1,x,y,z,rx,ry,rz) values('%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s');"\
				, name->valuestring, speed->valuestring, elbow_speed->valuestring,\
				acc->valuestring, elbow_acc->valuestring, toolnum->valuestring,\
				joint_value_string[0], joint_value_string[1], joint_value_string[2], joint_value_string[3], joint_value_string[4], joint_value_string[5], E1,\
				tcp_value_string[0], tcp_value_string[1], tcp_value_string[2], tcp_value_string[3], tcp_value_string[4], tcp_value_string[5]);
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

/* log management */
static int log_management(const cJSON *data_json)
{
	int ret = FAIL;
	char *buf = NULL;
	cJSON *root_json = NULL;
	cJSON *count = cJSON_GetObjectItem(data_json, "count");
	if (count == NULL || count->valuestring == NULL) {
		perror("json");

		return FAIL;
	}

	root_json = cJSON_CreateObject();
	cJSON_AddStringToObject(root_json, "log_count", count->valuestring);
	buf = cJSON_Print(root_json);
	ret = write_file(FILE_CFG, buf);
	free(buf);
	buf = NULL;
	cJSON_Delete(root_json);
	root_json = NULL;

	delete_log_file(0);

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
	char config_path[100] = {0};
	char *config_content = NULL;
	int i = 0;
	char content[100] = {0};
	SOCKET_INFO *sock_cmd = NULL;
	cJSON *name = NULL;
	cJSON *value = NULL;
	cJSON *package_json = NULL;
	cJSON *config_json = NULL;
	cJSON *func_json = NULL;
	cJSON *dio_json = NULL;
	cJSON *dio_value_json = NULL;
	cJSON *dio_level_json = NULL;

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

	sprintf(config_path, "%s%s/config.json", UPLOAD_WEB_PLUGINS, name->valuestring);
	/** enable plugin */
	if (value->valueint == 1) {

		return SUCCESS;
	/** disable plugin */
	} else {
		config_content = get_file_content(config_path);
		if (config_content == NULL || strcmp(config_content, "NO_FILE") == 0 || strcmp(config_content, "Empty") == 0) {
			perror("get file content");

			return FAIL;
		}
		//printf("config_content = %s\n", config_content);
		config_json = cJSON_Parse(config_content);
		free(config_content);
		config_content = NULL;
		if (config_json == NULL || config_json->type != cJSON_Array) {
			perror("cJSON_Parse");

			return FAIL;
		}
		for (i = 0; i < cJSON_GetArraySize(config_json); i++) {
			func_json = cJSON_GetArrayItem(config_json, i);
			dio_json = cJSON_GetObjectItem(func_json, "dio");
			dio_value_json = cJSON_GetObjectItem(func_json, "dio_value");
			dio_level_json = cJSON_GetObjectItem(func_json, "dio_level");
			if (dio_json == NULL || dio_value_json == NULL || dio_level_json == NULL || dio_json->type != cJSON_Number || dio_value_json->type != cJSON_Number || dio_level_json->type != cJSON_Number) {
				perror("json");

				return FAIL;
			}

			if (robot_type == 1) { // "1" 代表实体机器人
				sock_cmd = &socket_cmd;
			} else { // "0" 代表虚拟机器人
				sock_cmd = &socket_vir_cmd;
			}
			/* clear 外设DO */
			if (dio_json->valueint == 1) {
				bzero(content, sizeof(content));
				sprintf(content, "SetPluginDO(%d,%d,%d)", dio_value_json->valueint, 0, dio_level_json->valueint);
				//printf("content = %s\n", content);
				socket_enquene(sock_cmd, 339, content, 1);
			/* clear 外设DI */
			} else {
				bzero(content, sizeof(content));
				sprintf(content, "SetPluginDI(%d,%d,%d)", dio_value_json->valueint, 0, dio_level_json->valueint);
				//printf("content = %s\n", content);
				socket_enquene(sock_cmd, 340, content, 1);
			}
		}
		cJSON_Delete(config_json);
		config_json = NULL;

		/* 清空config.json文件 */
		buf = cJSON_Print(cJSON_CreateArray());
		write_ret = write_file(config_path, buf);//write file
		free(buf);
		buf = NULL;
		if (write_ret == FAIL) {
			perror("write file");

			return FAIL;
		}
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
	if (!strcmp(cmd, "save_lua_file") || !strcmp(cmd, "remove_lua_file") || !strcmp(cmd, "save_template_file") || !strcmp(cmd, "remove_template_file") || !strcmp(cmd, "rename_lua_file") || !strcmp(cmd, "remove_points") || !strcmp(cmd, "log_management") || !strcmp(cmd, "modify_tool_cdsystem") || !strcmp(cmd, "modify_ex_tool_cdsystem") || !strcmp(cmd, "modify_exaxis_cdsystem") || !strcmp(cmd, "shutdown")) {
		if (!authority_management("1")) {
			perror("authority_management");
			goto auth_end;
		}
	// cmd_auth "2"
	} else if (!strcmp(cmd, "change_type") || !strcmp(cmd, "save_point") || !strcmp(cmd, "plugin_enable")) {
		if (!authority_management("2")) {
			perror("authority_management");
			goto auth_end;
		}
	// cmd_auth "0"
	} else if (!strcmp(cmd, "save_accounts")) {
		if (!authority_management("0")) {
			perror("authority_management");
			goto auth_end;
		}
	}
	if (!strcmp(cmd, "save_lua_file")) {
		ret = save_lua_file(data_json);
	} else if (!strcmp(cmd, "remove_lua_file")) {
		ret = remove_lua_file(data_json);
	} else if (!strcmp(cmd, "save_template_file")) {
		ret = save_template_file(data_json);
	} else if (!strcmp(cmd, "remove_template_file")) {
		ret = remove_template_file(data_json);
	} else if (!strcmp(cmd, "rename_lua_file")) {
		ret = rename_lua_file(data_json);
	} else if (!strcmp(cmd, "modify_tool_cdsystem")) {
		ret = modify_tool_cdsystem(data_json);
	} else if (!strcmp(cmd, "modify_ex_tool_cdsystem")) {
		ret = modify_ex_tool_cdsystem(data_json);
	} else if (!strcmp(cmd, "modify_exaxis_cdsystem")) {
		ret = modify_exaxis_cdsystem(data_json);
	} else if (!strcmp(cmd, "save_point")) {
		ret = save_point(data_json);
	} else if (!strcmp(cmd, "remove_points")) {
		ret = remove_points(data_json);
	} else if (!strcmp(cmd, "change_type")) {
		ret = change_type(data_json);
	} else if (!strcmp(cmd, "log_management")) {
		ret = log_management(data_json);
	} else if (!strcmp(cmd, "save_accounts")) {
		ret = save_accounts(data_json);
	} else if (!strcmp(cmd, "plugin_enable")) {
		ret = plugin_enable(data_json);
	} else if (!strcmp(cmd, "shutdown")) {
		ret = shutdown_system(data_json);
	} else {
		perror("cmd not found");
		goto end;
	}
	if(ret == FAIL){
		perror("ret fail");
		goto end;
	}
	my_syslog("应用操作", cmd, cur_account.username);
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
	my_syslog("机器人操作", "当前用户无相应指令操作权限", cur_account.username);
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
	my_syslog("机器人操作", "普通操作失败", cur_account.username);
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
