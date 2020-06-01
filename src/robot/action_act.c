
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

/********************************* Function declaration ***********************/

static int save_lua_file(const cJSON *data_json);
static int remove_lua_file(const cJSON *data_json);
static int rename_lua_file(const cJSON *data_json);
static int modify_tool_cdsystem(const cJSON *data_json);
static int modify_ex_tool_cdsystem(const cJSON *data_json);
static int save_point(const cJSON *data_json);
static int remove_points(const cJSON *data_json);
static int change_type(const cJSON *data_json);
static int log_management(const cJSON *data_json);
static int save_accounts(const cJSON *data_json);

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
	/*int ret = FAIL;
	char *buf = NULL;
	cJSON *f_json = NULL;
	char *f_content = NULL;
	cJSON *newitem = NULL;
	cJSON *cd_name = NULL;
	cJSON *value = NULL;*/
	cJSON *name = NULL;
	cJSON *id = NULL;
	cJSON *x = NULL;
	cJSON *y = NULL;
	cJSON *z = NULL;
	cJSON *rx = NULL;
	cJSON *ry = NULL;
	cJSON *rz = NULL;

	/*newitem = cJSON_CreateObject();
	cd_name = cJSON_GetObjectItem(data_json, "name");
	value = cJSON_GetObjectItem(data_json, "value");
	if (cd_name == NULL || cd_name->valuestring == NULL || value == NULL || value->type != cJSON_Object) {
		perror("json");

		return FAIL;
	}*/
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
/*
	cJSON_AddStringToObject(newitem, "name", name->valuestring);
	cJSON_AddStringToObject(newitem, "id", id->valuestring);
	cJSON_AddStringToObject(newitem, "x", x->valuestring);
	cJSON_AddStringToObject(newitem, "y", y->valuestring);
	cJSON_AddStringToObject(newitem, "z", z->valuestring);
	cJSON_AddStringToObject(newitem, "rx", rx->valuestring);
	cJSON_AddStringToObject(newitem, "ry", ry->valuestring);
	cJSON_AddStringToObject(newitem, "rz", rz->valuestring);
	f_content = get_file_content(FILE_CDSYSTEM);
	if (f_content == NULL) {
		perror("get file content");

		return FAIL;
	}
	//printf("f_content = %s\n", f_content);
	f_json = cJSON_Parse(f_content);
	// replace exist object
	cJSON_ReplaceItemInObject(f_json, cd_name->valuestring, newitem);
	buf = cJSON_Print(f_json);
	//printf("buf = %s\n", buf);

	ret = write_file(FILE_CDSYSTEM, buf);
*/
	sprintf(sql, "insert into coordinate_system(name,id,x,y,z,rx,ry,rz) values('%s','%s','%s','%s','%s','%s','%s','%s');"\
					, name->valuestring, id->valuestring, x->valuestring, y->valuestring, z->valuestring, rx->valuestring, ry->valuestring, rz->valuestring);

	if (change_info_sqlite3(DB_CDSYSTEM, "coordinate_system", name->valuestring, sql) == -1) {
		perror("database");

		return FAIL;
	}
	//printf("ret = %d\n", ret);
/*	memset(sql, 0, sizeof(sql));
	free(buf);
	buf = NULL;
	cJSON_Delete(f_json);
	f_json = NULL;
	free(f_content);
	f_content = NULL;

	return ret;*/
	return SUCCESS;
}

/* modify ex && tool cdsystem */
static int modify_ex_tool_cdsystem(const cJSON *data_json)
{
	/*int ret = FAIL;
	char *buf = NULL;
	cJSON *f_json = NULL;
	char *f_content = NULL;*/
	char sql[1024] = {0};
	//cJSON *newitem = NULL;
	//cJSON *cd_name = NULL;
	//cJSON *value = NULL;
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

	//newitem = cJSON_CreateObject();
	//cd_name = cJSON_GetObjectItem(data_json, "name");
	//value = cJSON_GetObjectItem(data_json, "value");
	//if (cd_name == NULL || cd_name->valuestring == NULL || value == NULL || value->type != cJSON_Object) {
	//if (value == NULL || value->type != cJSON_Object) {
	//	perror("json");

	//	return FAIL;
	//}
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

/*	cJSON_AddStringToObject(newitem, "name", name->valuestring);
	cJSON_AddStringToObject(newitem, "user_name", user_name->valuestring);
	cJSON_AddStringToObject(newitem, "id", id->valuestring);
	cJSON_AddStringToObject(newitem, "ex", ex->valuestring);
	cJSON_AddStringToObject(newitem, "ey", ey->valuestring);
	cJSON_AddStringToObject(newitem, "ez", ez->valuestring);
	cJSON_AddStringToObject(newitem, "erx", erx->valuestring);
	cJSON_AddStringToObject(newitem, "ery", ery->valuestring);
	cJSON_AddStringToObject(newitem, "erz", erz->valuestring);
	cJSON_AddStringToObject(newitem, "tx", tx->valuestring);
	cJSON_AddStringToObject(newitem, "ty", ty->valuestring);
	cJSON_AddStringToObject(newitem, "tz", tz->valuestring);
	cJSON_AddStringToObject(newitem, "trx", trx->valuestring);
	cJSON_AddStringToObject(newitem, "try", try->valuestring);
	cJSON_AddStringToObject(newitem, "trz", trz->valuestring);
	f_content = get_file_content(FILE_ET_CDSYSTEM);
	if (f_content == NULL) {
		perror("get file content");

		return FAIL;
	}
	//printf("f_content = %s\n", f_content);
	f_json = cJSON_Parse(f_content);
	// replace exist object
	cJSON_ReplaceItemInObject(f_json, cd_name->valuestring, newitem);
	buf = cJSON_Print(f_json);
	//printf("buf = %s\n", buf);

	ret = write_file(FILE_ET_CDSYSTEM, buf);

	//printf("ret = %d\n", ret);
*/
	sprintf(sql,"insert into et_coordinate_system(name,user_name,id,ex,ey,ez,erx,ery,erz,tx,ty,tz,trx,try,trz) "\
						"values('%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s');"\
						,name->valuestring, user_name->valuestring, id->valuestring, ex->valuestring, ey->valuestring, ez->valuestring, erx->valuestring, ery->valuestring, erz->valuestring,\
						tx->valuestring, ty->valuestring, tz->valuestring, trx->valuestring, try->valuestring, trz->valuestring);

	if (change_info_sqlite3(DB_ET_CDSYSTEM, "et_coordinate_system", name->valuestring, sql) == -1) {
		perror("database");

		return FAIL;
	}
/*	memset(sql, 0, sizeof(sql));
	free(buf);
	buf = NULL;
	cJSON_Delete(f_json);
	f_json = NULL;
	free(f_content);
	f_content = NULL;

	return ret;
	*/
	return SUCCESS;
}

/* save point */
static int save_point(const cJSON *data_json)
{
	CTRL_STATE *state = NULL;
	/*cJSON *f_json = NULL;
	cJSON *newitem = NULL;
	cJSON *joints_json = NULL;
	cJSON *tcp_json = NULL;*/
	char sql[1024] = {0};
	int i = 0;
	/*int ret = FAIL;
	char *buf = NULL;
	char *f_content = NULL;
	char joint[10] = {0};
	char joint_value_string[10] = {0};*/
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
	//newitem = cJSON_CreateObject();
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
	//joints_json = cJSON_CreateObject();
	for (i = 0; i < 6; i++) {
	//	sprintf(joint, "j%d", (i+1));
	//	sprintf(joint_value_string, "%.3lf", state->jt_cur_pos[i]);
		sprintf(joint_value_string[i], "%.3lf", state->jt_cur_pos[i]); //数据库存储
	//	cJSON_AddStringToObject(joints_json, joint, joint_value_string);
	}
	//tcp_json = cJSON_CreateObject();
	for (i = 0; i < 6; i++) {
		sprintf(tcp_value_string[i], "%.3lf", state->tl_cur_pos[i]);
	}
	/*cJSON_AddStringToObject(tcp_json, "x", tcp_value_string[0]);
	cJSON_AddStringToObject(tcp_json, "y", tcp_value_string[1]);
	cJSON_AddStringToObject(tcp_json, "z", tcp_value_string[2]);
	cJSON_AddStringToObject(tcp_json, "rx", tcp_value_string[3]);
	cJSON_AddStringToObject(tcp_json, "ry", tcp_value_string[4]);
	cJSON_AddStringToObject(tcp_json, "rz", tcp_value_string[5]);

	cJSON_AddStringToObject(newitem, "name", name->valuestring);
	cJSON_AddStringToObject(newitem, "speed", speed->valuestring);
	cJSON_AddStringToObject(newitem, "elbow_speed", elbow_speed->valuestring);
	cJSON_AddStringToObject(newitem, "acc", acc->valuestring);
	cJSON_AddStringToObject(newitem, "elbow_acc", elbow_acc->valuestring);
	cJSON_AddStringToObject(newitem, "toolnum", toolnum->valuestring);
	cJSON_AddItemToObject(newitem, "joints", joints_json);
	cJSON_AddItemToObject(newitem, "tcp", tcp_json);*/

	sprintf(sql, "insert into points(name,speed,elbow_speed,acc,elbow_acc,toolnum,j1,j2,j3,j4,j5,j6,x,y,z,rx,ry,rz) "\
				"values('%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s');"\
				, name->valuestring, speed->valuestring, elbow_speed->valuestring,\
				acc->valuestring, elbow_acc->valuestring, toolnum->valuestring,\
				joint_value_string[0], joint_value_string[1], joint_value_string[2], joint_value_string[3], joint_value_string[4], joint_value_string[5],\
				tcp_value_string[0], tcp_value_string[1], tcp_value_string[2], tcp_value_string[3], tcp_value_string[4], tcp_value_string[5]);
	if (change_info_sqlite3(DB_POINTS, "points", name->valuestring, sql) == -1) {
		perror("database");

		return FAIL;
	}

	/*f_content = get_file_content(FILE_POINTS);
	if (f_content == NULL) {
		perror("get file content");

		return FAIL;
	}
	f_json = cJSON_Parse(f_content);
	if(cJSON_GetObjectItem(f_json, name->valuestring) == NULL) {
		// add new object to file
		cJSON_AddItemToObject(f_json, name->valuestring, newitem);
	} else {
		// replace exist object with new object
		cJSON_ReplaceItemInObject(f_json, name->valuestring, newitem);
	}
	buf = cJSON_Print(f_json);
	ret = write_file(FILE_POINTS, buf);
	free(buf);
	buf = NULL;
	cJSON_Delete(f_json);
	f_json = NULL;
	free(f_content);
	f_content = NULL;

	return ret;
	*/
	return SUCCESS;
}

/* remove points info */
static int remove_points(const cJSON *data_json)
{
	int ret = FAIL;
	int num = 0;
	int i = 0;
	//char *f_content = NULL;
	//cJSON *f_json = NULL;
	char *buf = NULL;
	char sql[1024] = {0};
	//char points_name[100][10] = {{0}};
	cJSON *name_index = NULL;
	cJSON *name = cJSON_GetObjectItem(data_json, "name");
	if (name == NULL) {
		perror("json");

		return FAIL;
	}
	printf("name = %s\n", cJSON_Print(name));
	num = cJSON_GetArraySize(name);

	/*f_content = get_file_content(FILE_POINTS);
	if (f_content == NULL) {
		perror("get file content");

		return FAIL;
	}
	f_json = cJSON_Parse(f_content);*/
	printf("num = %d\n", num);
	for (i = 0; i < num; i++) {
		name_index = cJSON_GetArrayItem(name, i);
		printf("name_index->valuestring = %s\n", name_index->valuestring);
		//cJSON_DeleteItemFromObject(f_json, name_index->valuestring);
		memset(sql,0,sizeof(sql));
		sprintf(sql, "delete from points where name = \'%s\'", name_index->valuestring);
		if (change_info_sqlite3(DB_POINTS, "points", name_index->valuestring, sql) == -1) {
			perror("database");

			return FAIL;
		}
	}
	/*buf = cJSON_Print(f_json);
	ret = write_file(FILE_POINTS, buf);

	free(buf);
	buf = NULL;
	cJSON_Delete(f_json);
	f_json = NULL;
	free(f_content);
	f_content = NULL;
	

	return ret;*/
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
	int i = 0;
	int len = 0;
	//int ret = FAIL;
	//char *buf = NULL;
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
//	if (array_size > 10) { // 超过10个账号不予保存
//		return FAIL;
//	}
	sprintf(sql, "delete from account;");
	if (delete_all_info_sqlite3(DB_ACCOUNT, sql) == -1) {
		perror("delete all");
	}
	memset(sql,0,sizeof(sql));

	sprintf(sql,"insert into account(name,password,auth) Values");        //拼接 insert语句
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
		sprintf(sql,"%s('%s','%s','%s'),",sql,username->valuestring,password->valuestring,auth->valuestring);
	}
	len = strlen(sql);
	sql[len -1] = '\0';   //去掉sql最后一个字符 ‘，’
	sprintf(sql,"%s;",sql);
	printf("sql = %s\n",sql);
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
    if (change_info_sqlite3(DB_ACCOUNT, "account", NULL, sql) == -1) {
    	perror("insert");

		return FAIL;
    }
/*  memset(sql,0,sizeof(sql));
	buf = cJSON_Print(account_array);
	//save to file account.json, 保存到账户文件 account.json 中
	ret = write_file(FILE_ACCOUNT, buf);
	free(buf);
	buf = NULL;
	if (ret == FAIL) {
		return FAIL;
	}*/

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
	if (!strcmp(cmd, "save_lua_file") || !strcmp(cmd, "remove_lua_file") || !strcmp(cmd, "rename_lua_file") || !strcmp(cmd, "remove_points") || !strcmp(cmd, "log_management") || !strcmp(cmd, "modify_tool_cdsystem") || !strcmp(cmd, "modify_ex_tool_cdsystem")) {
		if (!authority_management("1")) {
			perror("authority_management");
			goto auth_end;
		}
	// cmd_auth "2"
	} else if (!strcmp(cmd, "change_type") || !strcmp(cmd, "save_point")) {
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
	} else if (!strcmp(cmd, "rename_lua_file")) {
		ret = rename_lua_file(data_json);
	} else if (!strcmp(cmd, "modify_tool_cdsystem")) {
		ret = modify_tool_cdsystem(data_json);
	} else if (!strcmp(cmd, "modify_ex_tool_cdsystem")) {
		ret = modify_ex_tool_cdsystem(data_json);
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
	} else {
		perror("cmd not found");
		goto end;
	}
	if(ret == FAIL){
		perror("ret fail");
		goto end;
	}
	my_syslog("普通操作", cmd, "admin");
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
