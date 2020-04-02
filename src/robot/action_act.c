
/********************************* Includes ***********************************/

#include    "goahead.h"
#include	"cJSON.h"
#include 	"tools.h"
#include    "action_act.h"

/********************************* Defines ************************************/

extern int robot_type;

/********************************* Function declaration ***********************/

static int save_lua_file(const cJSON *data_json);
static int remove_lua_file(const cJSON *data_json);
static int rename_lua_file(const cJSON *data_json);
static int save_tool_cdsystem(const cJSON *data_json);
static int change_type(const cJSON *data_json);

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

/* save tool cdsystem */
static int save_tool_cdsystem(const cJSON *data_json)
{
	int ret = FAIL;
	char *buf = NULL;
	cJSON *f_json = NULL;
	char *f_content = NULL;
	cJSON *newitem = NULL;
	cJSON *cd_name = NULL;
	cJSON *value = NULL;
	cJSON *name = NULL;
	cJSON *id = NULL;
	cJSON *x = NULL;
	cJSON *y = NULL;
	cJSON *z = NULL;
	cJSON *rx = NULL;
	cJSON *ry = NULL;
	cJSON *rz = NULL;

	newitem = cJSON_CreateObject();
	cd_name = cJSON_GetObjectItem(data_json, "name");
	value = cJSON_GetObjectItem(data_json, "value");
	if (cd_name == NULL || cd_name->valuestring == NULL || value == NULL || value->type != cJSON_Object) {
		perror("json");

		return FAIL;
	}
	name = cJSON_GetObjectItem(value, "name");
	id = cJSON_GetObjectItem(value, "id");
	x = cJSON_GetObjectItem(value, "x");
	y = cJSON_GetObjectItem(value, "y");
	z = cJSON_GetObjectItem(value, "z");
	rx = cJSON_GetObjectItem(value, "rx");
	ry = cJSON_GetObjectItem(value, "ry");
	rz = cJSON_GetObjectItem(value, "rz");
	if(name == NULL || id == NULL || x == NULL || y == NULL || z == NULL || rx == NULL || ry == NULL|| rz == NULL || name->valuestring == NULL || id->valuestring == NULL || x->valuestring == NULL || y->valuestring == NULL || z->valuestring == NULL || rx->valuestring == NULL || ry->valuestring == NULL || rz->valuestring == NULL) {
		perror("json");

		return FAIL;
	}

	cJSON_AddStringToObject(newitem, "name", name->valuestring);
	cJSON_AddStringToObject(newitem, "id", id->valuestring);
	cJSON_AddStringToObject(newitem, "x", x->valuestring);
	cJSON_AddStringToObject(newitem, "y", y->valuestring);
	cJSON_AddStringToObject(newitem, "z", z->valuestring);
	cJSON_AddStringToObject(newitem, "rx", rx->valuestring);
	cJSON_AddStringToObject(newitem, "ry", ry->valuestring);
	cJSON_AddStringToObject(newitem, "rz", rz->valuestring);
	f_content = get_file_content(FILE_CDSYSTEM);
	/* file is NULL */
	if (f_content == NULL) {
		perror("get file content");

		return FAIL;
	}
	//printf("f_content = %s\n", f_content);
	f_json = cJSON_Parse(f_content);
	cJSON_ReplaceItemInObject(f_json, cd_name->valuestring, newitem);
	buf = cJSON_Print(f_json);
	//printf("buf = %s\n", buf);

	ret = write_file(FILE_CDSYSTEM, buf);

	//printf("ret = %d\n", ret);

	free(buf);
	buf = NULL;
	cJSON_Delete(f_json);
	f_json = NULL;
	free(f_content);
	f_content = NULL;

	return ret;
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

/* do some user actions on webserver */
void act(Webs *wp)
{
	int ret = FAIL;
	char *buf = NULL;
	char *cmd = NULL;
	cJSON *command = NULL;
	cJSON *data = NULL;
	cJSON *data_json = NULL;

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
	if(!strcmp(cmd, "save_lua_file")) {
		ret = save_lua_file(data_json);
	} else if(!strcmp(cmd, "remove_lua_file")) {
		ret = remove_lua_file(data_json);
	} else if(!strcmp(cmd, "rename_lua_file")) {
		ret = rename_lua_file(data_json);
	} else if(!strcmp(cmd, "save_tool_cdsystem")) {
		ret = save_tool_cdsystem(data_json);
	} else if(!strcmp(cmd, "change_type")) {
		ret = change_type(data_json);
	} else {
		perror("cmd not found");
		goto end;
	}
	if(ret == FAIL){
		perror("ret fail");
		goto end;
	}
	/* cjson delete */
	cJSON_Delete(data);
	data = NULL;
	websSetStatus(wp, 200);
	websWriteHeaders(wp, -1, 0);
	websWriteEndHeaders(wp);
	websWrite(wp, "success");
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
