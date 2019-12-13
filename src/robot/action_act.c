
/********************************* Includes ***********************************/

#include    "goahead.h"
#include    "action_act.h"
#include 	"tools.h"
#include	"cJSON.h"

/********************************* Forwards ***********************************/

static int save_lua_file(const cJSON *data_json);
static int remove_lua_file(const cJSON *data_json);
static int rename_lua_file(const cJSON *data_json);

/*********************************** Code *************************************/

/* save lua file */
static int save_lua_file(const cJSON *data_json)
{
	int ret = FAIL;
	char dir_filename[100] = {0};

	cJSON *file_name = cJSON_GetObjectItem(data_json, "name");
	cJSON *pgvalue = cJSON_GetObjectItem(data_json, "pgvalue");
	if (file_name == NULL || pgvalue == NULL || file_name->valuestring == NULL || pgvalue->valuestring == NULL) {
		perror("json");

		return FAIL;
	}
	sprintf(dir_filename, "%s/%s", DIR_LUA, file_name->valuestring);
	ret = write_file(dir_filename, pgvalue->valuestring);

	return ret;
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
	sprintf(dir_filename, "%s/%s", DIR_LUA, name->valuestring);
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
	sprintf(old_filename, "%s/%s", DIR_LUA, oldname->valuestring);
	sprintf(new_filename, "%s/%s", DIR_LUA, newname->valuestring);
	if (rename(old_filename, new_filename) == -1) {
		perror("rename");

		return FAIL;
	}

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
