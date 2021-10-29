#include    "goahead.h"
#include	"cJSON.h"
#include    "md5.h"
#include 	"tools.h"
#include 	"robot_socket.h"
#include 	"sqlite3.h"
#include	"filehandler.h"

extern ACCOUNT_INFO cur_account;
//extern SOCKET_INFO socket_cmd;
//extern SOCKET_INFO socket_vir_cmd;
//extern int robot_type;

static int compute_file_md5(const char *file_path, char *md5_str);
static int check_upfile(const char *upgrade_path, const char *readme_now_path, const char *readme_up_path);
static int check_version(const char *readme_now_path, const char *readme_up_path);
static int check_robot_type();
static int update_config(char *filename);
static int import_torquesys(char *pathfilename);

static void fileWriteEvent(Webs *wp);
static int export_torquesys(char *pathfilename);
static int avolfileHandler(Webs *wp);

/**
	计算 MD5 值：
	file_path (文件路径)
	md5_str (md5值)
*/
static int compute_file_md5(const char *file_path, char *md5_str)
{
	int i;
	int fd;
	int ret;
	unsigned char data[MD5_READ_DATA_SIZE];
	unsigned char md5_value[MD5_SIZE];
	MD5_CTX md5;

	fd = open(file_path, O_RDONLY);
	if (-1 == fd)
	{
		perror("open");
		return -1;
	}

	// init md5
	MD5Init(&md5);

	while (1)
	{
		ret = read(fd, data, MD5_READ_DATA_SIZE);
		if (-1 == ret)
		{
			perror("read");
			return -1;
		}

		MD5Update(&md5, data, ret);

		if (0 == ret || ret < MD5_READ_DATA_SIZE)
		{
			break;
		}
	}

	close(fd);

	MD5Final(&md5, md5_value);

	for(i = 0; i < MD5_SIZE; i++)
	{
		snprintf(md5_str + i*2, 2+1, "%02x", md5_value[i]);
	}
	md5_str[MD5_STR_LEN] = '\0'; // add end

	return 0;
}

/**
	do md5 check and version check
*/
static int check_upfile(const char *upgrade_path, const char *readme_now_path, const char *readme_up_path)
{
	FILE *fp;
	char strline[LINE_LEN] = {0};
	char md5sum_up[MD5_STR_LEN + 1] = "";
	char md5sum_com[MD5_STR_LEN + 1] = "";
#if recover_mode
	char version_now[20] = "";
	char version_up[20] = "";
#endif

	if (compute_file_md5(upgrade_path, md5sum_com) == -1) {
		perror("md5 compute");

		return FAIL;
	}

	if ((fp = fopen(readme_up_path, "r")) == NULL) {
		perror("open file");

		return FAIL;
	}
	while (fgets(strline, LINE_LEN, fp) != NULL) {
		strrpc(strline, "\n", "");
		if (!strncmp(strline, "MD5SUM=", 7)) {
			strrpc(strline, "MD5SUM=", "");
			strcpy(md5sum_up, strline);
#if recover_mode
		} else if (!strncmp(strline, "VERSION=", 8)) {
			strrpc(strline, "VERSION=", "");
			strcpy(version_up, strline);
#endif
		}
		bzero(strline, sizeof(char)*LINE_LEN);
	}
	fclose(fp);

	printf("md5sum_com = %s\n", md5sum_com);
	printf("md5sum_up = %s\n", md5sum_up);
	printf("strcmp(md5sum_com, md5sum_up) = %d\n", strcmp(md5sum_com, md5sum_up));

#if recover_mode
	if ((fp = fopen(readme_now_path, "r")) == NULL) {
		perror("open file");

		return FAIL;
	}
	while (fgets(strline, LINE_LEN, fp) != NULL) {
		strrpc(strline, "\n", "");
		if (!strncmp(strline, "VERSION=", 8)) {
			strrpc(strline, "VERSION=", "");
			strcpy(version_now, strline);
		}
		bzero(strline, sizeof(char)*LINE_LEN);
	}
	fclose(fp);

	printf("version_now = %s\n", version_now);
	printf("version_up = %s\n", version_up);
	printf("strcmp(version_up, version_now) = %d\n", strcmp(version_up, version_now));
#endif

#if recover_mode
	if (strcmp(md5sum_com, md5sum_up) != 0 || strcmp(version_up, version_now) < 0) {
#else
	if (strcmp(md5sum_com, md5sum_up) != 0) {
#endif
		perror("upgrade fail");

		return FAIL;
	}

	return SUCCESS;
}

/**
	do version check again
*/
static int check_version(const char *readme_now_path, const char *readme_up_path)
{
	FILE *fp;
	char strline[LINE_LEN] = {0};
	char version_now[20] = "";
	char version_up[20] = "";

	if ((fp = fopen(readme_up_path, "r")) == NULL) {
		perror("open file");

		return FAIL;
	}
	while (fgets(strline, LINE_LEN, fp) != NULL) {
		strrpc(strline, "\n", "");
		if (!strncmp(strline, "VERSION=", 8)) {
			strrpc(strline, "VERSION=", "");
			strcpy(version_up, strline);
		}
		bzero(strline, sizeof(char)*LINE_LEN);
	}
	fclose(fp);

	if ((fp = fopen(readme_now_path, "r")) == NULL) {
		perror("open file");

		return FAIL;
	}
	while (fgets(strline, LINE_LEN, fp) != NULL) {
		strrpc(strline, "\n", "");
		if (!strncmp(strline, "VERSION=", 8)) {
			strrpc(strline, "VERSION=", "");
			strcpy(version_now, strline);
		}
		bzero(strline, sizeof(char)*LINE_LEN);
	}
	fclose(fp);

	printf("version_now = %s\n", version_now);
	printf("version_up = %s\n", version_up);
	printf("strcmp(version_up, version_now) = %d\n", strcmp(version_up, version_now));

	if (strcmp(version_up, version_now) != 0) {
		perror("upgrade fail");

		return FAIL;
	}

	return SUCCESS;
}

/**
  check robot type
*/
static int check_robot_type()
{
	FILE *fp_up = NULL;
	char strline_up[LINE_LEN] = {0};
	char **array_up = NULL;
	int size_up = 0;
	FILE *fp_now = NULL;
	char strline_now[LINE_LEN] = {0};
	char **array_now = NULL;
	int size_now = 0;
	char robot_type_now[100] = "";
	char robot_type_up[100] = "";

	if ((fp_up = fopen(WEB_USER_CFG, "r")) == NULL) {
		perror("user.config : open file");

		return FAIL;
	}
	while (fgets(strline_up, LINE_LEN, fp_up) != NULL) {
		if (is_in(strline_up, "ROBOT_TYPE = ") == 1) {
			if (string_to_string_list(strline_up, " = ", &size_up, &array_up) == 0 || size_up != 2) {
				perror("string to string list");
				fclose(fp_up);
				string_list_free(&array_up, size_up);

				return FAIL;
			}
			strcpy(robot_type_up, array_up[1]);
			string_list_free(&array_up, size_up);

			break;
		}
		bzero(strline_up, sizeof(char)*LINE_LEN);
	}
	fclose(fp_up);

	if ((fp_now = fopen(USER_CFG, "r")) == NULL) {
		perror("now user.config : open file");

		return FAIL;
	}
	while (fgets(strline_now, LINE_LEN, fp_now) != NULL) {
		if (is_in(strline_now, "ROBOT_TYPE = ") == 1) {
			if (string_to_string_list(strline_now, " = ", &size_now, &array_now) == 0 || size_now != 2) {
				perror("string to string list");
				fclose(fp_now);
				string_list_free(&array_now, size_now);

				return FAIL;
			}
			strcpy(robot_type_now, array_now[1]);
			string_list_free(&array_now, size_now);

			break;
		}
		bzero(strline_now, sizeof(char)*LINE_LEN);
	}
	fclose(fp_now);

	if (strcmp(robot_type_now, robot_type_up) == 0) {

		return SUCCESS;
	} else {
		perror("robot type is not same!");

		return FAIL;
	}
}

/**
  update user.config/ex_device.config/exaxis.config/robot.config
*/
static int update_config(char *filename)
{
	FILE *fp_up = NULL;
	char strline_up[LINE_LEN] = {0};
	char **array_up = NULL;
	int size_up = 0;
	FILE *fp_now = NULL;
	char strline_now[LINE_LEN] = {0};
	char **array_now = NULL;
	int size_now = 0;
	char write_line[LINE_LEN] = {0};
	char write_content[LINE_LEN*10] = {0};
	int line_index = 0;
	char cmd[128] = {0};
	char upgrade_filename[100] = "";
	char now_filename[100] = "";

	strcpy(now_filename, filename);
	if (!strcmp(filename, USER_CFG)) {
		strcpy(upgrade_filename, UPGRADE_USER_CFG);
	} else if (!strcmp(filename, EX_DEVICE_CFG)) {
		strcpy(upgrade_filename, UPGRADE_EX_DEVICE_CFG);
	} else if (!strcmp(filename, EXAXIS_CFG)) {
		strcpy(upgrade_filename, UPGRADE_EXAXIS_CFG);
	} else if (!strcmp(filename, ROBOT_CFG)) {
		strcpy(upgrade_filename, UPGRADE_ROBOT_CFG);
	}
	printf("upgrade_filename = %s\n", upgrade_filename);
	printf("now_filename = %s\n", now_filename);

	if ((fp_up = fopen(upgrade_filename, "r")) == NULL) {
		perror("upgrade config : open file");
		//printf("rm now config\n");
		//memset(cmd, 0, 128);
		//sprintf(cmd, "rm %s", now_filename);
		//system(cmd);

		return SUCCESS;
	}
	while (fgets(strline_up, LINE_LEN, fp_up) != NULL) {
		line_index++;
		bzero(write_line, sizeof(char)*LINE_LEN);
		strcpy(write_line, strline_up);
		if (is_in(strline_up, "=") == 1 && line_index > 2) {
			if (string_to_string_list(strline_up, " = ", &size_up, &array_up) == 0 || size_up != 2) {
				perror("string to string list");
				fclose(fp_up);
				string_list_free(&array_up, size_up);

				return FAIL;
			}
			if (array_up[0] != NULL) {// 出现左边的 key 值
				//  now config file is not exist
				if ((fp_now = fopen(now_filename, "r")) == NULL) {
					perror("now config : open file");
					fclose(fp_up);
					string_list_free(&array_up, size_up);

					printf("cp upgrade config file to now config\n");
					memset(cmd, 0, 128);
					sprintf(cmd, "cp %s %s", upgrade_filename, now_filename);
					system(cmd);

					return SUCCESS;
				}
				while (fgets(strline_now, LINE_LEN, fp_now) != NULL) {
					//printf("strline_now = %s\n", strline_now);
					if (is_in(strline_now, "=") == -1) {
						continue;
					}
					if (string_to_string_list(strline_now, " = ", &size_now, &array_now) == 0 || size_now != 2) {
						perror("string to string list");
						fclose(fp_up);
						fclose(fp_now);
						string_list_free(&array_up, size_up);
						string_list_free(&array_now, size_now);

						return FAIL;
					}
					if (array_now[0] != NULL) {
						//printf("array_now[0] = %s\n", array_now[0]);
						//printf("array_up[0] = %s\n", array_up[0]);
						if (strcmp(array_now[0], array_up[0]) == 0) {
							//printf("array_up[0] = array_up[0]\n");
							//printf("array_now[1] = %s\n", array_now[1]);
							bzero(write_line, sizeof(char)*LINE_LEN);
							sprintf(write_line, "%s = %s", array_up[0], array_now[1]);
							string_list_free(&array_now, size_now);
							bzero(strline_now, sizeof(char)*LINE_LEN);
							break;
						}
					}
					string_list_free(&array_now, size_now);
					bzero(strline_now, sizeof(char)*LINE_LEN);
				}
				fclose(fp_now);
			}
			string_list_free(&array_up, size_up);
		}
		bzero(strline_up, sizeof(char)*LINE_LEN);
		//printf("write_line = %s\n", write_line);
		strcat(write_content, write_line);
	}
	fclose(fp_up);

	//printf("write_content len = %d\n", strlen(write_content));
	//printf("write_content = %s\n", write_content);

	return write_file(now_filename, write_content);
}

int update_file_dir()
{
	char sql[MAX_BUF] = {0};
	char **resultp = NULL;
	int nrow = 0;
	int ncloumn = 0;
	char cmd[128] = {0};

	/**
		更新 coordinate_system.db
		V3.1.7 版本中增加 type，installation_site 两列
	*/
	memset(sql, MAX_BUF, 0);
	sprintf(sql, "select * from coordinate_system");
	if (select_info_sqlite3(DB_CDSYSTEM, sql, &resultp, &nrow, &ncloumn) == -1) {
		perror("select");
		sqlite3_free_table(resultp); //释放结果集

		return FAIL;
	}
	sqlite3_free_table(resultp); //释放结果集
	if (ncloumn != 10) {
		memset(sql, MAX_BUF, 0);
		sprintf(sql, "\
		PRAGMA foreign_keys = 0; \
		CREATE TABLE sqlitestudio_temp_table AS SELECT *FROM coordinate_system; \
		DROP TABLE coordinate_system; \
		CREATE TABLE coordinate_system ( \
				name              TEXT, \
				id                INTEGER PRIMARY KEY ASC,\
				x                 TEXT, \
				y                 TEXT, \
				z                 TEXT, \
				rx                TEXT, \
				ry                TEXT, \
				rz                TEXT, \
				type              TEXT    DEFAULT (0), \
				installation_site TEXT    DEFAULT (0) \
				); \
		INSERT INTO coordinate_system ( \
				name, \
				id, \
				x, \
				y, \
				z, \
				rx, \
				ry, \
				rz \
				) \
			SELECT name, \
				   id, \
				   x, \
				   y, \
				   z, \
				   rx, \
				   ry, \
				   rz \
					   FROM sqlitestudio_temp_table; \
		DROP TABLE sqlitestudio_temp_table; \
		PRAGMA foreign_keys = 1; \
		");
		//printf("strlen sql = %d\n", strlen(sql));
		if (change_info_sqlite3(DB_CDSYSTEM, sql) == -1) {
			perror("database");

			return FAIL;
		}
	}

	/**
	    V3.1.8 版本
		增加了 sysvar 文件夹

		如果 sysvar.db 文件不存在， 更新 sysvar 系统变量文件夹
	*/
	if (check_dir_filename(DIR_SYSVAR, "sysvar.db") == 0) {
		bzero(cmd, sizeof(cmd));
		sprintf(cmd, "cp -r %s/* %s", DIR_FACTORY_RESET_SYSVAR, DIR_SYSVAR);
		system(cmd);
	}

	/**
	    V3.1.8 版本:
		增加了 torquesys_cfg.db torquesys_DIO.txt torquesys_pageflag.txt 文件
	    V3.1.9 版本:
		增加了 torquesys_points.db, torquesys_custom.db 文件
		V3.2.0 版本：
		增加了 torquesys_pd_data.db

		如果 torquesys 文件夹下相关文件不存在，更新 torquesys 扭矩文件夹
	*/
	if (check_dir_filename(DIR_TORQUESYS, "torquesys_pd_data.db") == 0) {
		bzero(cmd, sizeof(cmd));
		sprintf(cmd, "cp -rf %s/* %s", DIR_FACTORY_RESET_TORQUESYS, DIR_TORQUESYS);
		system(cmd);
	}

	/**
	    V3.3.1 版本
		* cfg 文件夹下，增加了 pi.txt 文件,并修改了 system.txt 文件格式（修改为数字格式）
		  拷贝恢复出厂值 cfg 文件夹下所有文件到 cfg 文件夹下
		* points 文件夹下，新增 point_cfg.txt
	*/
	/**
	    V3.3.2 临时升级版本
		恢复 system.txt 文件格式（字符串格式），为了支持 V3.3.0 之前的旧版本
		如果当前版本为 V3.3.1 和 V3.3.2 必须升级到 V3.3.2 临时版本，目的是恢复 system.txt 文件格式（字符串格式），再往上升级
	*/
	/** TODO: 在 2021/10/19 V3.3.2 版本增加这两行 if 判断的注释， 待系统稳定后需要删除，目前注释了只是确保 system.txt 文件在升级时能够更新，是最新字符串格式 */
	//if (check_dir_filename(DIR_CFG, "PI.txt") == 0) {
		bzero(cmd, sizeof(cmd));
		sprintf(cmd, "cp -rf %scfg/* %s", DIR_FACTORY_RESET, DIR_CFG);
		printf("cmd = %s\n", cmd);
		system(cmd);

	/** V3.3.3 版本
		在 2021/10/26 版本修改了 point_cfg.txt 文件，增加 flag 标志，在升级时能够更新, 待系统稳定后需要删除
	*/
		bzero(cmd, sizeof(cmd));
		sprintf(cmd, "cp %spoints/point_cfg.txt %s", DIR_FACTORY_RESET, DIR_POINTS);
		printf("cmd = %s\n", cmd);
		system(cmd);
	//}

	return SUCCESS;
}

/* 嘉宝扭矩系统：导入机种工艺包 */
static int import_torquesys(char *pathfilename)
{
	char cmd[100] = {0};
	char wk_id[100] = "";
	char sql[SQL_LEN] = {0};
	char sql_exec[SQL_LEN] = {0};
	char** resultp = NULL;
	int nrow = 0;
	int ncloumn = 0;
	int i = 0;
	char *errmsg = NULL;
	sqlite3 *db = NULL;

	strncpy(wk_id, (pathfilename + strlen("/tmp/torquesys_")), (strlen(pathfilename) - strlen("/tmp/torquesys_") - strlen(".tar.gz")));
	//printf("wk_id = %s\n", wk_id);

	memset(cmd, 0, 100);
	sprintf(cmd, "cd /tmp/ && tar -zxvf torquesys_%s.tar.gz", wk_id);
	system(cmd);

	/* 更新 torquesys_cfg.db 工件生产参数配置信息 */
	memset(sql, 0, sizeof(sql));
	sprintf(sql, "select * from torquesys_cfg;");
	if (select_info_sqlite3(UPLOAD_DB_TORQUE_CFG, sql, &resultp, &nrow, &ncloumn) == -1) {

		return FAIL;
	}
	memset(sql_exec, 0, sizeof(sql_exec));
	sprintf(sql_exec, "insert into torquesys_cfg values ('%s', %d, %d, %d, %d, %d, %d);", resultp[ncloumn], atoi(resultp[ncloumn + 1]), atoi(resultp[ncloumn + 2]),atoi(resultp[ncloumn + 3]),atoi(resultp[ncloumn + 4]),atoi(resultp[ncloumn + 5]),atoi(resultp[ncloumn + 6]));
	if (change_info_sqlite3(DB_TORQUE_CFG, sql_exec) == -1) {
		perror("database");

		return FAIL;
	}
	sqlite3_free_table(resultp);

	/* 更新 torquesys_points.db	工件示教点信息 */
	/* 创建/打开 box 数据库 */
	if (sqlite3_open(DB_POINTS, &db) != SQLITE_OK) {
		fprintf(stderr, "Cannot create database:%s\n", sqlite3_errmsg(db));
		sqlite3_close(db);

		return FAIL;
	}
	//printf("%s database open success!\n", DB_POINTS);
	memset(sql, 0, sizeof(sql));
	sprintf(sql, "select * from points;");
	if (select_info_sqlite3(UPLOAD_DB_TORQUE_POINTS, sql, &resultp, &nrow, &ncloumn) == -1) {

		return FAIL;
	}
	/* 在插入数据前显式开启事务 */
	sqlite3_exec(db, "begin;", 0, 0, 0);
	for (i = 0; i < nrow; i++) {
		memset(sql_exec, 0, sizeof(sql_exec));
		sprintf(sql_exec, "insert into points(name,speed,elbow_speed,acc,elbow_acc,toolnum,workpiecenum,j1,j2,j3,j4,j5,j6,E1,E2,E3,E4,x,y,z,rx,ry,rz) values ('%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s');", resultp[(i+1)*ncloumn], resultp[(i+1)*ncloumn+1], resultp[(i+1)*ncloumn+2], resultp[(i+1)*ncloumn+3], resultp[(i+1)*ncloumn+4], resultp[(i+1)*ncloumn+5], resultp[(i+1)*ncloumn+6], resultp[(i+1)*ncloumn+7], resultp[(i+1)*ncloumn+8], resultp[(i+1)*ncloumn+9], resultp[(i+1)*ncloumn+10], resultp[(i+1)*ncloumn+11], resultp[(i+1)*ncloumn+12], resultp[(i+1)*ncloumn+13], resultp[(i+1)*ncloumn+14], resultp[(i+1)*ncloumn+15], resultp[(i+1)*ncloumn+16], resultp[(i+1)*ncloumn+17], resultp[(i+1)*ncloumn+18], resultp[(i+1)*ncloumn+19], resultp[(i+1)*ncloumn+20], resultp[(i+1)*ncloumn+21], resultp[(i+1)*ncloumn+22]); 
		if (sqlite3_exec(db, sql_exec, NULL, NULL, &errmsg) != SQLITE_OK) {
			fprintf(stderr, "SQL error: %s\n", errmsg);
			sqlite3_free(errmsg);
			sqlite3_close(db);
			sqlite3_free_table(resultp);

			return FAIL;
		}
	}
	sqlite3_free_table(resultp);
	/* 插入后再一起提交 */
	sqlite3_exec(db, "commit;", 0, 0, 0);
	sqlite3_close(db);

	/* 更新 torquesys_points_cfg.db 工件示教点配置信息 */
	/** 删除已有的工件数据表 */
	memset(sql, 0, sizeof(sql));
	sprintf(sql, "select name from sqlite_master where type = 'table' and name like '%s%';", wk_id);
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
	/** 创建新的 table, 插入示教点信息 */
	/* 创建/打开 box 数据库 */
	if (sqlite3_open(DB_TORQUE_POINTS, &db) != SQLITE_OK) {
		fprintf(stderr, "Cannot create database:%s\n", sqlite3_errmsg(db));
		sqlite3_close(db);

		return FAIL;
	}
	//printf("%s database open success!\n", DB_TORQUE_POINTS);
	/* 在插入数据前显式开启事务 */
	sqlite3_exec(db, "begin;", 0, 0, 0);
	/* 创建 left 数据表 */
	memset(sql, 0, sizeof(sql));
	sprintf(sql, "create table [%s_left] (name TEXT, id INTEGER primary key);", wk_id);
	if (sqlite3_exec(db, sql, NULL, NULL, &errmsg) != SQLITE_OK) {
		fprintf(stderr, "SQL error: %s\n", errmsg);
		sqlite3_free(errmsg);
		sqlite3_close(db);

		return FAIL;
	}
	memset(sql, 0, 1024);
	sprintf(sql, "select * from [%s_left];", wk_id);
	if (select_info_sqlite3(UPLOAD_DB_TORQUE_POINTS_CFG, sql, &resultp, &nrow, &ncloumn) == -1) {

		return FAIL;
	}
	for (i = 0; i < nrow; i++) {
		if (resultp[(i + 1) * ncloumn] != NULL && resultp[(i + 1) * ncloumn + 1] != NULL) {
			memset(sql, 0, sizeof(sql));
			sprintf(sql, "insert into [%s_left] values ('%s', %d);", wk_id, resultp[(i + 1) * ncloumn], atoi(resultp[(i + 1) * ncloumn + 1]));
			if (sqlite3_exec(db, sql, NULL, NULL, &errmsg) != SQLITE_OK) {
				fprintf(stderr, "SQL error: %s\n", errmsg);
				sqlite3_free(errmsg);
				sqlite3_close(db);
				sqlite3_free_table(resultp);

				return FAIL;
			}
		}
	}
	sqlite3_free_table(resultp);
	/* 插入后再一起提交 */
	sqlite3_exec(db, "commit;", 0, 0, 0);
	sqlite3_close(db);

	/* 创建/打开 box 数据库 */
	if (sqlite3_open(DB_TORQUE_POINTS, &db) != SQLITE_OK) {
		fprintf(stderr, "Cannot create database:%s\n", sqlite3_errmsg(db));
		sqlite3_close(db);

		return FAIL;
	}
	//printf("%s database open success!\n", DB_TORQUE_POINTS);
	/* 在插入数据前显式开启事务 */
	sqlite3_exec(db, "begin;", 0, 0, 0);
	/* 创建 right 数据表 */
	memset(sql, 0, sizeof(sql));
	sprintf(sql, "create table [%s_right] (name TEXT, id INTEGER primary key);", wk_id);
	if (sqlite3_exec(db, sql, NULL, NULL, &errmsg) != SQLITE_OK) {
		fprintf(stderr, "SQL error: %s\n", errmsg);
		sqlite3_free(errmsg);
		sqlite3_close(db);

		return FAIL;
	}
	memset(sql, 0, 1024);
	sprintf(sql, "select * from [%s_right];", wk_id);
	if (select_info_sqlite3(UPLOAD_DB_TORQUE_POINTS_CFG, sql, &resultp, &nrow, &ncloumn) == -1) {

		return FAIL;
	}
	for (i = 0; i < nrow; i++) {
		if (resultp[(i + 1) * ncloumn] != NULL && resultp[(i + 1) * ncloumn + 1] != NULL) {
			memset(sql, 0, sizeof(sql));
			sprintf(sql, "insert into [%s_right] values ('%s', %d);", wk_id, resultp[(i + 1) * ncloumn], atoi(resultp[(i + 1) * ncloumn + 1]));
			if (sqlite3_exec(db, sql, NULL, NULL, &errmsg) != SQLITE_OK) {
				fprintf(stderr, "SQL error: %s\n", errmsg);
				sqlite3_free(errmsg);
				sqlite3_close(db);
				sqlite3_free_table(resultp);

				return FAIL;
			}
		}
	}
	sqlite3_free_table(resultp);
	/* 插入后再一起提交 */
	sqlite3_exec(db, "commit;", 0, 0, 0);
	sqlite3_close(db);

	/* 创建/打开 box 数据库 */
	if (sqlite3_open(DB_TORQUE_POINTS, &db) != SQLITE_OK) {
		fprintf(stderr, "Cannot create database:%s\n", sqlite3_errmsg(db));
		sqlite3_close(db);

		return FAIL;
	}
	//printf("%s database open success!\n", DB_TORQUE_POINTS);
	/* 在插入数据前显式开启事务 */
	sqlite3_exec(db, "begin;", 0, 0, 0);
	/* 创建 right 数据表 */
	memset(sql, 0, sizeof(sql));
	sprintf(sql, "create table [%s_cfg] (ptemp TEXT, perscrew_pnum INTEGER);", wk_id);
	if (sqlite3_exec(db, sql, NULL, NULL, &errmsg) != SQLITE_OK) {
		fprintf(stderr, "SQL error: %s\n", errmsg);
		sqlite3_free(errmsg);
		sqlite3_close(db);

		return FAIL;
	}
	memset(sql, 0, 1024);
	sprintf(sql, "select * from [%s_cfg];", wk_id);
	if (select_info_sqlite3(UPLOAD_DB_TORQUE_POINTS_CFG, sql, &resultp, &nrow, &ncloumn) == -1) {

		return FAIL;
	}
	if (resultp[ncloumn] != NULL && resultp[ncloumn + 1] != NULL) {
		memset(sql, 0, sizeof(sql));
		sprintf(sql, "insert into [%s_cfg] values ('%s', %d);", wk_id, resultp[ncloumn], atoi(resultp[ncloumn + 1]));
		if (sqlite3_exec(db, sql, NULL, NULL, &errmsg) != SQLITE_OK) {
			fprintf(stderr, "SQL error: %s\n", errmsg);
			sqlite3_free(errmsg);
			sqlite3_close(db);
			sqlite3_free_table(resultp);

			return FAIL;
		}
	}
	sqlite3_free_table(resultp);
	/* 插入后再一起提交 */
	sqlite3_exec(db, "commit;", 0, 0, 0);
	sqlite3_close(db);

	return SUCCESS;
}

void upload(Webs *wp)
{
	//printf("__FUNC__ = %s, __LINE__ = %d\n", __FUNCTION__, __LINE__);
	WebsKey         *s;
	WebsUpload      *up;
	SOCKET_INFO *sock_cmd = NULL;
	char            *upfile;
	int i = 0;
	char *dir_list = NULL;
	char filename[128] = {0};
	char cmd[128] = {0};
	char delete_cmd[128] = {0};
	char config_path[100] = {0};
	char package_path[100] = {0};
	char *package_content = NULL;
	char *buf = NULL;
	char plugin_name[100] = {0};
	int write_ret = FAIL;
	cJSON *root_json = NULL;
	cJSON *name = NULL;
	cJSON *nav_name = NULL;
	cJSON *version = NULL;
	cJSON *description = NULL;
	cJSON *author = NULL;
	cJSON *dir_list_json = NULL;
	cJSON *dir_plugin_name_json = NULL;

	if (scaselessmatch(wp->method, "POST")) {
		for (s = hashFirst(wp->files); s; s = hashNext(wp->files, s)) {
			up = s->content.value.symbol;
			/* close printf */
		/*	websWrite(wp, "FILE: %s\r\n", s->name.value.string);
			websWrite(wp, "FILENAME=%s\r\n", up->filename);
			websWrite(wp, "CLIENT=%s\r\n", up->clientFilename);
			websWrite(wp, "TYPE=%s\r\n", up->contentType);
			websWrite(wp, "SIZE=%d\r\n", up->size);*/
		/*	printf("wp->uploadTmp = %s\n", wp->uploadTmp);
			printf("wp->uploadVar = %s\n", wp->uploadVar);

			printf("FILE: %s\r\n", s->name.value.string);
		    printf("FILENAME=%s\r\n", up->filename);
			printf("CLIENT=%s\r\n", up->clientFilename);
			printf("TYPE=%s\r\n", up->contentType);
			printf("SIZE=%d\r\n", up->size);*/

			/* web_point.db file */
			if (strcmp(up->clientFilename, "web_point.db") == 0) {
				upfile = sfmt("%s", DB_POINTS);
				my_syslog("普通操作", "导入示教点文件成功", cur_account.username);
				my_en_syslog("normal operation", "Import the teaching point file successfully", cur_account.username);
				my_jap_syslog("普通の操作", "インポート web_point 文書が成功する", cur_account.username);
			/* user lua file */
			} else if (is_in(up->clientFilename, ".lua") == 1) {
				upfile = sfmt("%s%s", DIR_USER, up->clientFilename);
				my_syslog("普通操作", "导入 lua 文件成功", cur_account.username);
				my_en_syslog("normal operation", "Imported lua file successfully", cur_account.username);
				my_jap_syslog("普通の操作", "luaファイルのインポートに成功", cur_account.username);
			/* web Tool model file */
			} else if (is_in(up->clientFilename, ".dae") == 1 || is_in(up->clientFilename, ".stl") == 1) {
				upfile = sfmt("%s%s", UPLOAD_TOOL_MODEL, up->clientFilename);
				my_syslog("普通操作", "导入工具模型成功", cur_account.username);
				my_en_syslog("normal operation", "Import tool model successfully", cur_account.username);
				my_jap_syslog("普通の操作", "ツールモデルの導入に成功", cur_account.username);
				sprintf(filename, "%s%s", LOAD_TOOL_MODEL, up->clientFilename);
			/* web system config file */
			} else if (strcmp(up->clientFilename, "system.txt") == 0) {
				upfile = sfmt("%s", FILE_CFG);
				my_syslog("普通操作", "导入 web 端系统配置文件成功", cur_account.username);
				my_en_syslog("normal operation", "Import the web side system configuration file successfully", cur_account.username);
				my_jap_syslog("普通の操作", "インポートweb側のシステム構成 文書が成功する", cur_account.username);
				strcpy(filename, upfile);
			/* vision_pkg_des.txt file */
			} else if (strcmp(up->clientFilename, "vision_pkg_des.txt") == 0) {
				upfile = sfmt("%s", FILE_VISION);
				my_syslog("普通操作", "导入 vision_pkg_des.txt 文件成功", cur_account.username);
				my_en_syslog("normal operation", "Import vision_pkg_des.txt file successfully", cur_account.username);
				my_jap_syslog("普通の操作", "インポートvision_pkg_des.txt 文書が成功する", cur_account.username);
				strcpy(filename, upfile);
			/* web user data file */
			} else if (strcmp(up->clientFilename, "fr_user_data.tar.gz") == 0) {
				upfile = sfmt("%s", UPGRADE_FILE_USERDATA);
				my_syslog("普通操作", "导入用户数据文件成功", cur_account.username);
				my_en_syslog("normal operation", "Import of user data file successfully", cur_account.username);
				my_jap_syslog("普通の操作", "ユーザーデータファイルのインポートに成功しました", cur_account.username);
				strcpy(filename, upfile);
			/* control user file */
			} else if (strcmp(up->clientFilename, "user.config") == 0) {
				upfile = sfmt("%s", WEB_USER_CFG);
				my_syslog("普通操作", "导入控制器端用户配置文件成功", cur_account.username);
				my_en_syslog("normal operation", "Import of controller user profile successfully", cur_account.username);
				my_jap_syslog("普通の操作", "コントローラ側のユーザプロファイルのインポートが成功しました", cur_account.username);
				strcpy(filename, upfile);
			/* webapp upgrade file */
		/*	} else if (strcmp(up->clientFilename, "webapp.tar.gz") == 0) {
				upfile = sfmt("%s", UPGRADE_WEBAPP);
				my_syslog("普通操作", "导入 webapp 升级文件成功", cur_account.username);
				strcpy(filename, upfile);*/
			/* control upgrade file */
		/*	} else if (strcmp(up->clientFilename, "control.tar.gz") == 0) {
				upfile = sfmt("%s", UPGRADE_CONTROL);
				my_syslog("普通操作", "导入控制器升级文件成功", cur_account.username);
				strcpy(filename, upfile);*/
			/** Software Upgrade package file */
			} else if (strcmp(up->clientFilename, "software.tar.gz") == 0) {
				upfile = sfmt("%s", UPGRADE_SOFTWARE);
				my_syslog("普通操作", "导入软件升级文件成功", cur_account.username);
				my_en_syslog("normal operation", "Import software update file successfully", cur_account.username);
				my_jap_syslog("普通の操作", "ソフトウェアアップグレードファイルのインポートに成功", cur_account.username);
				strcpy(filename, upfile);
			/* peripheral plugin file */
			} else if (is_in(up->clientFilename, "plugin") == 1 && is_in(up->clientFilename, ".tar.gz") == 1) {
				upfile = sfmt("%s%s", UPLOAD_WEB_PLUGINS, up->clientFilename);
				my_syslog("普通操作", "导入外设插件文件成功", cur_account.username);
				my_en_syslog("normal operation", "Import peripheral plug-in file successfully", cur_account.username);
				my_jap_syslog("普通の操作", "周辺機器プラグインファイルのインポートに成功しました", cur_account.username);
				strcpy(filename, upfile);
			/* ODM file */
			} else if (strcmp(up->clientFilename, "odm.tar.gz") == 0) {
				upfile = sfmt("%s", UPLOAD_WEB_ODM);
				my_syslog("普通操作", "导入 ODM 文件成功", cur_account.username);
				my_en_syslog("normal operation", "Import ODM file successfully", cur_account.username);
				my_jap_syslog("普通の操作", "odmファイルのインポートに成功", cur_account.username);
				strcpy(filename, upfile);
			/* torquesys pkg file */
			} else if (is_in(up->clientFilename, "torquesys") == 1 && is_in(up->clientFilename, ".tar.gz") == 1) {
				upfile = sfmt("/tmp/%s", up->clientFilename);
				my_syslog("普通操作", "导入嘉宝工件配方文件成功", cur_account.username);
				my_en_syslog("normal operation", "The garbo artifact recipe file was imported successfully", cur_account.username);
				my_jap_syslog("普通の操作", "ガルボワークのレシピファイルの導入に成功", cur_account.username);
				strcpy(filename, upfile);
			} else {
				my_syslog("普通操作", "导入文件未匹配", cur_account.username);
				my_en_syslog("normal operation", "Import file fail", cur_account.username);
				my_jap_syslog("普通の操作", "インポートファイルが一致しません", cur_account.username);
				goto end;
			}
			//printf("upfile = %s\n", upfile);
			if (rename(up->filename, upfile) < 0) {
				error("Cannot rename uploaded file: %s to %s, errno %d", up->filename, upfile, errno);
			}
			wfree(upfile);
		}
		/*websWrite(wp, "\r\nVARS:\r\n");
		for (s = hashFirst(wp->vars); s; s = hashNext(wp->vars, s)) {
			websWrite(wp, "%s=%s\r\n", s->name.value.string, s->content.value.string);
		}*/
	}
	//printf("filename = %s\n", filename);
	if (strcmp(filename, FILE_CFG) == 0) {
		delete_log_file(0);
		init_sys_lifespan();
	} else if (strcmp(filename, WEB_USER_CFG) == 0) {
		//printf("before check robot type\n");
		if (check_robot_type() == FAIL) {
			perror("check_robot_type");

			goto end;
		}
	} else if (strcmp(filename, UPGRADE_FILE_USERDATA) == 0) {
		//system("rm -rf /root/web/file");
		//system("rm -f /root/robot/exaxis.config");
		//system("rm -f /root/robot/ex_device.config");
		system("cd /tmp/ && tar -zxvf fr_user_data.tar.gz");
		// 用户数据包版本不匹配，导入失败
		/*
		if (check_file_version() == FAIL) {
			perror("check_file_version");

			goto end;
		}
		*/
		system("cd /root/web/file/ && rm -rf ./block ./cdsystem ./points ./robotcfg ./sysvar ./template ./user");
		bzero(cmd, sizeof(cmd));
		sprintf(cmd, "cp %s %s", UPGRADE_FILE_USERDATA, FILE_USERDATA);
		system(cmd);
		system("cd /root/ && tar -zxvf fr_user_data.tar.gz");
		// 用户数据包中控制器日志--机器人型号不一致，导入失败
		if (check_robot_type() == FAIL) {
			perror("check_robot_type");
			system("rm -f /root/fr_user_data.tar.gz");
			system("rm -f /tmp/fr_user_data.tar.gz");

			goto end;
		}
		system("rm -f /root/fr_user_data.tar.gz");
		system("rm -f /tmp/fr_user_data.tar.gz");
	} else if (strcmp(filename, UPGRADE_SOFTWARE) == 0) {
		/* 准备升级， 进入升级流程， 首先删除标志 “升级成功” 的文件（可能升级之前该文件就已经存在） */
		bzero(cmd, sizeof(cmd));
		sprintf(cmd, "rm %s%s", DIR_FILE, FILENAME_UP_SUC);
		system(cmd);

		system("cd /tmp && tar -zxvf software.tar.gz");
		// md5 check && verison check
		if (check_upfile(UPGRADE_WEB, README_WEB_NOW, README_WEB_UP) == FAIL || check_upfile(UPGRADE_FR_CONTROL, README_CTL_NOW, README_CTL_UP) == FAIL) {
			perror("md5 & version check fail!");
			my_syslog("普通操作", "升级软件失败", cur_account.username);
			my_en_syslog("normal operation", "Failed to upgrade software", cur_account.username);
			my_jap_syslog("普通の操作", "ソフトウェアのアップグレードに失敗する", cur_account.username);
			goto end;
		}

		bzero(cmd, sizeof(cmd));
		sprintf(cmd, "openssl des3 -d -k frweb -salt -in %s | tar xzvf - -C /tmp/", UPGRADE_WEB);
		if (system(cmd) != 0) {
			my_syslog("普通操作", "升级 webapp 失败", cur_account.username);
			my_en_syslog("normal operation", "Failed to upgrade webapp", cur_account.username);
			my_jap_syslog("普通の操作", "webappのアップグレードに失敗", cur_account.username);
			perror("uncompress fail!");
			goto end;
		}

		/** 更新文件 */
		/** 更新 user.config 文件 */
		if (update_config(USER_CFG) == FAIL) {
			my_syslog("普通操作", "升级 user.config 失败", cur_account.username);
			my_en_syslog("normal operation", "Failed to upgrade user.config", cur_account.username);
			my_jap_syslog("普通の操作", "user.config のアップグレードに失敗", cur_account.username);
			perror("user.config");
			goto end;
		}
		/* 下发 set rebot type 指令，确保 robot type 正确 */
		/*if (send_cmd_set_robot_type() == FAIL) {
			perror("send cmd set robot type!");

			goto end;
		}*/
		/** 更新 ex_device.config 文件 */
		if (update_config(EX_DEVICE_CFG) == FAIL) {
			my_syslog("普通操作", "升级 ex_device.config 失败", cur_account.username);
			my_en_syslog("normal operation", "Failed to upgrade ex_device.config", cur_account.username);
			my_jap_syslog("普通の操作", "ex_device.config のアップグレードに失敗", cur_account.username);
			perror("ex_device.config");
			goto end;
		}
		/** 更新 exaxis.config 文件 */
		if (update_config(EXAXIS_CFG) == FAIL) {
			my_syslog("普通操作", "升级 exaxis.config 失败", cur_account.username);
			my_en_syslog("normal operation", "Failed to upgrade exaxis.config", cur_account.username);
			my_jap_syslog("普通の操作", "exaxis.config のアップグレードに失敗", cur_account.username);
			perror("exaxis.config");
			goto end;
		}

		/** 更新 robot.config 文件 */
		if (update_config(ROBOT_CFG) == FAIL) {
			my_syslog("普通操作", "升级 robot.config 失败", cur_account.username);
			my_en_syslog("normal operation", "Failed to upgrade robot.config", cur_account.username);
			my_jap_syslog("普通の操作", "robot.config のアップグレードに失敗", cur_account.username);
			perror("robot.config");
			goto end;
		}

		/** 更新 file 文件夹 */
		/*if (update_file_dir() == FAIL){
			my_syslog("普通操作", "升级 FAILDIR 失败", cur_account.username);
			my_en_syslog("normal operation", "Failed to upgrade FAILDIR", cur_account.username);
			my_jap_syslog("普通の操作", "FAILDIR のアップグレードに失敗", cur_account.username);
			perror("FAILDIR");
			goto end;
		}*/

		/** 文件写入硬盘需要一定时间，等待 1 秒 */
		//sleep(1);

		/** 更新 webserver 文件夹 */
		bzero(cmd, sizeof(cmd));
//#if recover_mode
		//sprintf(cmd, "sh %s", SHELL_WEBTARCP);
		sprintf(cmd, "sh %s", UPGRADE_WEBTARCP);
//#else
//		sprintf(cmd, "sh %s", SHELL_RECOVER_WEBTARCP);
//#endif
		do {
			system(cmd);
			/** 文件写入硬盘需要一定时间，等待 5 秒 */
			//sleep(5);
		} while (check_version(README_WEB_NOW, README_WEB_UP) == FAIL);
		my_syslog("普通操作", "升级 webapp 成功", cur_account.username);
		my_en_syslog("normal operation", "Successed to upgrade webapp", cur_account.username);
		my_jap_syslog("普通の操作", "webappのアップグレードに成功", cur_account.username);


		/** 更新 control 相关文件 */
		bzero(cmd, sizeof(cmd));
//#if recover_mode
		//sprintf(cmd, "sh %s", SHELL_CRLUPGRADE);
		sprintf(cmd, "sh %s", UPGRADE_CRLUPGRADE);
//#else
//		sprintf(cmd, "sh %s", SHELL_RECOVER_CRLUPGRADE);
//#endif
		do {
			system(cmd);
			/** 文件写入硬盘需要一定时间，等待 1 秒 */
			//sleep(1);
		} while (check_version(README_CTL_NOW, README_CTL_UP) == FAIL);
		my_syslog("普通操作", "升级控制器软件成功", cur_account.username);
		my_en_syslog("normal operation", "Controller software upgrade successful", cur_account.username);
		my_jap_syslog("普通の操作", "コントローラソフトウェアのアップグレードに成功", cur_account.username);

		system("rm -f /tmp/software.tar.gz && rm -rf /tmp/fr_control && rm -rf /tmp/web && rm -rf /tmp/software");

		/* 创建标志 “升级成功” 的文件 */
	//	bzero(cmd, sizeof(cmd));
	//	sprintf(cmd, "touch %s%s", DIR_FILE, FILENAME_UP_SUC);
	//	system(cmd);

		/** 文件写入硬盘需要一定时间，等待 10 秒 */
	//	sleep(10);

	//	system("sleep 1 && shutdown -b &");

	} else if (is_in(up->clientFilename, "plugin") == 1 && is_in(up->clientFilename, ".tar.gz") == 1) {
		strncpy(plugin_name, up->clientFilename, (strlen(up->clientFilename)-7));
		//printf("plugin_name = %s\n", plugin_name);
		bzero(delete_cmd, sizeof(delete_cmd));
		sprintf(delete_cmd, "rm -rf %s%s", UPLOAD_WEB_PLUGINS, plugin_name);
		//printf("delete_cmd = %s\n", delete_cmd);
		dir_list = get_dir_filename(UPLOAD_WEB_PLUGINS);
		if (dir_list == NULL) {
			perror("get dir filename");

			goto end;
		}
		dir_list_json = cJSON_Parse(dir_list);
		free(dir_list);
		dir_list = NULL;
		if (dir_list_json == NULL) {
			perror("cJSON_Parse");

			goto end;
		}
		for (i = 0; i < cJSON_GetArraySize(dir_list_json); i++) {
			dir_plugin_name_json = cJSON_GetArrayItem(dir_list_json, i);
			/* 上传外设插件与已有的外设插件同名（外设插件更新）*/
			if (strcmp(dir_plugin_name_json->valuestring, plugin_name) == 0) {
				if (clear_plugin_config(plugin_name) == FAIL) {
					perror("clear_plugin_config");
					cJSON_Delete(dir_list_json);
					dir_list_json = NULL;

					goto end;
				}
				system(delete_cmd);
				break;
			}
		}

		bzero(cmd, sizeof(cmd));
		sprintf(cmd, "cd %s && tar -zxvf %s", UPLOAD_WEB_PLUGINS, up->clientFilename);
		system(cmd);

		bzero(cmd, sizeof(cmd));
		sprintf(cmd, "rm -f %s%s", UPLOAD_WEB_PLUGINS, up->clientFilename);
		//printf("cmd = %s\n", cmd);
		system(cmd);

		/* ADD file validity check */
		bzero(package_path, sizeof(package_path));
		sprintf(package_path, "%s%s/package.json", UPLOAD_WEB_PLUGINS, plugin_name);
		//printf("package_path = %s\n", package_path);
		package_content = get_file_content(package_path);
		if (package_content == NULL || strcmp(package_content, "NO_FILE") == 0 || strcmp(package_content, "Empty") == 0) {
			perror("get file content");
			package_content = NULL;
			system(delete_cmd);

			goto end;
		}
		//printf("package_content = %s\n", package_content);
		root_json = cJSON_Parse(package_content);
		free(package_content);
		package_content = NULL;
		if (root_json == NULL || root_json->type != cJSON_Object) {
			perror("cJSON_Parse");
			system(delete_cmd);

			goto end;
		}
		name = cJSON_GetObjectItem(root_json, "name");
		nav_name = cJSON_GetObjectItem(root_json, "nav_name");
		version = cJSON_GetObjectItem(root_json, "version");
		description = cJSON_GetObjectItem(root_json, "description");
		author = cJSON_GetObjectItem(root_json, "author");
		if (name == NULL || nav_name == NULL || version == NULL || description == NULL || author == NULL) {
			perror("json");
			cJSON_Delete(root_json);
			root_json = NULL;
			system(delete_cmd);

			goto end;
		}

		/* add enable option to package.json file */
		cJSON_AddNumberToObject(root_json, "enable", 1);
		buf = cJSON_Print(root_json);
		//printf("buf = %s\n", buf);
		write_ret = write_file(package_path, buf);//write file
		free(buf);
		buf = NULL;
		cJSON_Delete(root_json);
		root_json = NULL;
		if (write_ret == FAIL) {
			system(delete_cmd);
			goto end;
		}

		/* init config.json file */
		sprintf(config_path, "%s%s/config.json", UPLOAD_WEB_PLUGINS, plugin_name);
		//printf("config_path = %s\n", config_path);
		buf = cJSON_Print(cJSON_CreateArray());
		write_ret = write_file(config_path, buf);//write file
		free(buf);
		buf = NULL;
		if (write_ret == FAIL) {
			system(delete_cmd);
			goto end;
		}
	} else if (is_in(filename, "torquesys") == 1 && is_in(filename, ".tar.gz") == 1) {
		import_torquesys(filename);
	} else if (strcmp(filename, UPLOAD_WEB_ODM) == 0) {
		system("rm -rf /root/web/frontend/file/odm");
		system("cd /root/web/frontend/file/ && tar -zxvf odm.tar.gz");
		system("rm -f /root/web/frontend/file/odm.tar.gz");
	}

	websSetStatus(wp, 200);
	websWriteHeaders(wp, -1, 0);
	//websWriteHeader(wp, "Content-Type", "text/plain");
	websWriteEndHeaders(wp);
	//websRedirect(wp,"/index.html#/programteach");
	if (is_in(up->clientFilename, ".dae") == 1 || is_in(up->clientFilename, ".stl") == 1) {
		websWrite(wp, filename);
	/**
	  HTTP 状态码为 200 OK 时， jquery ajax报错
	  原因:
	  1. 后端返回的json数据格式不规范
	  2. HTTP状态码为200，但是返回空的响应
	  解决方案：如果后端接口想返回200，那么请返回一个null或者{}去代替空响应
	*/
	} else if (is_in(filename, "torquesys") == 1 && is_in(filename, ".tar.gz") == 1) {
		websWrite(wp, "{}");
	} else {
		websWrite(wp, "success");
	}
	websDone(wp);

	return;

end:
	websSetStatus(wp, 403);
	websWriteHeaders(wp, -1, 0);
	websWriteEndHeaders(wp);
	websWrite(wp, "fail");
	websDone(wp);
	return;
}

static void fileWriteEvent(Webs *wp)
{

	char    *buf;
	ssize   len, wrote;

	assert(wp);
	assert(websValid(wp));


	if ((buf = walloc(ME_GOAHEAD_LIMIT_BUFFER)) == NULL) {
		websError(wp, HTTP_CODE_INTERNAL_SERVER_ERROR, "Can't get memory");
		return;
	}
	while ((len = websPageReadData(wp, buf, ME_GOAHEAD_LIMIT_BUFFER)) > 0) {
		if ((wrote = websWriteSocket(wp, buf, len)) < 0) {
			break;
		}
		if (wrote != len) {
			websPageSeek(wp, - (len - wrote), SEEK_CUR);
			break;
		}
	}
	wfree(buf);
	if (len <= 0) {
		websDone(wp);
	}
}

/* 嘉宝扭矩系统：导出机种工艺包 */
static int export_torquesys(char *pathfilename)
{
	char cmd[100] = {0};
	char wk_id[100] = "";
	char sql[SQL_LEN] = {0};
	char** resultp = NULL;
	int nrow = 0;
	int ncloumn = 0;
	int i = 0;

	strncpy(wk_id, (pathfilename + strlen("/tmp/torquesys_")), (strlen(pathfilename) - strlen("/tmp/torquesys_") - strlen(".tar.gz")));
	//printf("wk_id = %s\n", wk_id);

	memset(cmd, 0, 100);
	sprintf(cmd, "rm -f %s", pathfilename);
	system(cmd);
	memset(cmd, 0, 100);
	sprintf(cmd, "rm -f %s %s %s", UPLOAD_DB_TORQUE_CFG, UPLOAD_DB_TORQUE_POINTS_CFG, UPLOAD_DB_TORQUE_POINTS);
	system(cmd);
	memset(cmd, 0, 100);
	sprintf(cmd, "cp %s %s", DB_TORQUE_CFG, UPLOAD_DB_TORQUE_CFG);
	system(cmd);
	memset(cmd, 0, 100);
	sprintf(cmd, "cp %s %s", DB_TORQUE_POINTS, UPLOAD_DB_TORQUE_POINTS_CFG);
	system(cmd);
	memset(cmd, 0, 100);
	sprintf(cmd, "cp %s %s", DB_POINTS, UPLOAD_DB_TORQUE_POINTS);
	system(cmd);

	/* 只保留当前工件生产参数配置信息 */
	memset(sql, SQL_LEN, 0);
	sprintf(sql, "delete from torquesys_cfg where workpiece_id != '%s';", wk_id);
	if (change_info_sqlite3(UPLOAD_DB_TORQUE_CFG, sql) == -1) {
		perror("database");

		return FAIL;
	}
	/* 只保留当前工件示教点配置信息 */
	memset(sql, 0, sizeof(sql));
	sprintf(sql, "select name from sqlite_master where type = 'table' and name not like '%s%';", wk_id);
	select_info_sqlite3(UPLOAD_DB_TORQUE_POINTS_CFG, sql, &resultp, &nrow, &ncloumn);
	for (i = 0; i < nrow; i++) {
		memset(sql, 0, sizeof(sql));
		sprintf(sql, "drop table [%s];", resultp[(i + 1) * ncloumn]);
		if (change_info_sqlite3(UPLOAD_DB_TORQUE_POINTS_CFG, sql) == -1) {
			perror("database");

			return FAIL;
		}
	}
	sqlite3_free_table(resultp);
	/* 只保留当前工件示教点信息 */
	memset(sql, SQL_LEN, 0);
	sprintf(sql, "delete from points where name not like '%s%';", wk_id);
	if (change_info_sqlite3(UPLOAD_DB_TORQUE_POINTS, sql) == -1) {
		perror("database");

		return FAIL;
	}

	memset(cmd, 0, 100);
	sprintf(cmd, "cd /tmp/ && tar -zcvf torquesys_%s.tar.gz ./torquesys_cfg.db ./torquesys_points_cfg.db ./torquesys_points.db", wk_id);
	system(cmd);

	return SUCCESS;
}

static int avolfileHandler(Webs *wp)
{
	WebsFileInfo    info;
	char            *tmp, *date;
	ssize           nchars;
	int             code;

	char    *pathfilename; //带路径的文件名 用于找到对应的文件  
	char    *filenameExt; //文件扩展名 用于 设置 MIME类型  
	char    *filename;      //文件名 用于下载后保存的文件名称  
	char    *disposition; //临时保存 附件 标识  
	char	*ext = '.';
	char	*slash= '/';
	char 	cmd[128] = {0};

	assert(websValid(wp));
	assert(wp->method);
	assert(wp->filename && wp->filename[0]);    

	pathfilename = websGetVar(wp, "pathfilename", NULL);
	printf("pathfilename = %s\n", pathfilename);
	if (pathfilename == NULL) {

		return 1;
	}
	if (strcmp(pathfilename, FILE_USERDATA) == 0) {
		memset(cmd, 0, 128);
		sprintf(cmd, "cp %s %s", USER_CFG, WEB_USER_CFG);
		system(cmd);
		memset(cmd, 0, 128);
		sprintf(cmd, "cp %s %s", EXAXIS_CFG, WEB_EXAXIS_CFG);
		system(cmd);
		memset(cmd, 0, 128);
		sprintf(cmd, "cp %s %s", EX_DEVICE_CFG, WEB_EX_DEVICE_CFG);
		system(cmd);
		// 写入 FILE VERSION
		write_file(README_FILE, FILE_VERSION);
		system("rm -f /root/fr_user_data.tar.gz");
		system("cd /root/ && tar -zcvf fr_user_data.tar.gz ./web/file/README_FILE.txt ./web/file/block ./web/file/cdsystem ./web/file/points ./web/file/robotcfg ./web/file/sysvar ./web/file/template ./web/file/user");
		my_syslog("普通操作", "导出用户数据文件成功", cur_account.username);
		my_en_syslog("normal operation", "Export of user data file successful", cur_account.username);
		my_jap_syslog("普通の操作", "ユーザーデータファイルのエクスポートに成功", cur_account.username);
	} else if (strcmp(pathfilename, FILE_RBLOG) == 0) {
		memset(cmd, 0, 128);
		sprintf(cmd, "rm -f %s", FILE_RBLOG);
		system(cmd);
		system("cd /root/robot/ && tar -zcvf rblog.tar.gz ./rblog");
		my_syslog("普通操作", "导出控制器日志成功", cur_account.username);
		my_en_syslog("normal operation", "Description Exporting controller logs succeeded", cur_account.username);
		my_jap_syslog("普通の操作", "コントローラログの導出に成功", cur_account.username);
	} else if (strstr(pathfilename, "torquesys")) {
		export_torquesys(pathfilename);
		my_syslog("普通操作", "导出工件配方成功", cur_account.username);
		my_en_syslog("normal operation", "Export of workpiece package successful", cur_account.username);
		my_jap_syslog("普通の操作", "ワークレシピを導き出すことに成功", cur_account.username);
	} else if (strcmp(pathfilename, DB_POINTS) == 0) {
		my_syslog("普通操作", "导出示教点文件成功", cur_account.username);
		my_en_syslog("normal operation", "Export of points file successful", cur_account.username);
		my_jap_syslog("普通の操作", "ポイントファイルを提示して成功させる", cur_account.username);
	} else if (strcmp(pathfilename, FILE_CFG) == 0) {
		my_syslog("普通操作", "导出 web 端系统配置文件成功", cur_account.username);
		my_en_syslog("normal operation", "Export of web system configure file successful", cur_account.username);
		my_jap_syslog("普通の操作", "web側システムプロファイルのエクスポートに成功しました", cur_account.username);
	} else if (strcmp(pathfilename, USER_CFG) == 0) {
		my_syslog("普通操作", "导出控制器端用户配置文件成功", cur_account.username);
		my_en_syslog("normal operation", "Export of controller side user configure file successful", cur_account.username);
		my_jap_syslog("普通の操作", "コントローラ側のユーザプロファイルを成功裏にエクスポートする", cur_account.username);
	} else if (is_in(pathfilename, DIR_USER) == 1) {
		my_syslog("普通操作", "导出用户程序文件成功", cur_account.username);
		my_en_syslog("normal operation", "Export of user program file successful", cur_account.username);
		my_jap_syslog("普通の操作", "ユーザープログラムファイルをエクスポートします", cur_account.username);
	} else if (strcmp(pathfilename, FILE_STATEFB) == 0) {
		my_syslog("普通操作", "导出状态查询文件成功", cur_account.username);
		my_en_syslog("normal operation", "Export of Status query file successful", cur_account.username);
		my_jap_syslog("普通の操作", "ステータス照会ファイルのエクスポートに成功しました", cur_account.username);
	} else if (strcmp(pathfilename, FILE_STATEFB10) == 0) {
		my_syslog("普通操作", "导出出现异常 10s 前机器人数据文件成功", cur_account.username);
		my_en_syslog("normal operation", "Export of  the robot data file 10s before the exception occurs successful", cur_account.username);
		my_jap_syslog("普通の操作", "異常10s前ロボットデータファイルのエクスポートに成功しました", cur_account.username);
	} else if (is_in(pathfilename, DIR_LOG) == 1) {
		my_syslog("普通操作", "导出 log 文件成功", cur_account.username);
		my_en_syslog("normal operation", "Export of log file successful", cur_account.username);
		my_jap_syslog("普通の操作", "ログファイルのエクスポートに成功", cur_account.username);
	}

	//取文件名和扩展名
	//filename =sclone(getUrlLastSplit(sclone(pathfilename),"\\"));
	//filenameExt =sclone(getUrlLastSplit(sclone(filename),"."));
	//filename = "ip.txt";
	filename = strrchr(pathfilename, slash) + 1;
	if (filename)
		printf("The filename: %s\n", filename);
	else {
		websError(wp, HTTP_CODE_NOT_FOUND, "The filename was not found");
		return 1;
	}
	//filenameExt = "txt";
	filenameExt = strrchr(filename, ext) + 1;
	if (filename)
		printf("The filenameExt: %s\n", filenameExt);
	else {
		websError(wp, HTTP_CODE_NOT_FOUND, "The filenameExt was not found");
		return 1;
	}

	if (wp->ext) wfree(wp->ext);

	wp->ext=walloc(1+strlen(filenameExt)+1);
	sprintf(wp->ext,".%s",sclone(filenameExt));
	printf("wp->ext = %s\n", wp->ext);
	//free(filenameExt);
	//filenameExt=NULL;

	if (wp->filename) wfree(wp->filename);
	wp->filename=sclone(pathfilename);
	printf("wp->filename = %s\n", wp->filename);

	if (wp->path) wfree(wp->path);
	wp->path=sclone(pathfilename);
	printf("wp->path = %s\n", wp->path);

	if (websPageIsDirectory(wp)) {
		nchars = strlen(wp->path);
		if (wp->path[nchars - 1] == '/' || wp->path[nchars - 1] == '\\') {
			wp->path[--nchars] = '\0';
		}
		char *websIndex = "testdownload";
		tmp = sfmt("%s/%s", wp->path, websIndex);
		websRedirect(wp, tmp);
		wfree(tmp);
		return 1;
	}

	if (websPageOpen(wp, O_RDONLY | O_BINARY, 0666) < 0) {
		websError(wp, HTTP_CODE_NOT_FOUND, "Cannot open   document for: %s", wp->path);
		return 1;
	}   
	if (websPageStat(wp, &info) < 0) {
		websError(wp, HTTP_CODE_NOT_FOUND, "Cannot stat page for URL");
		return 1;
	}

	code = 200;
	if (wp->since && info.mtime <= wp->since) {
		code = 304;
	}

	websSetStatus(wp, code);
	websWriteHeaders(wp, info.size, 0);
	disposition = walloc(20+strlen(filename)+1);
	//设置下载文件的名称
	sprintf(disposition,"attachment;filename=%s",sclone(filename));
	websWriteHeader(wp, "Content-Disposition", sclone(disposition));

	//free(filename);
	free(disposition);
	//filename=NULL;
	disposition=NULL;  
	if ((date = websGetDateString(&info)) != NULL) {
		websWriteHeader(wp, "Last-modified", "%s", date);
		wfree(date);
	}

	websWriteEndHeaders(wp);

	/*
	   All done if the browser did a HEAD request
	 */
	if (smatch(wp->method, "HEAD")) {
		websDone(wp);
		return 1;
	}
	websSetBackgroundWriter(wp, fileWriteEvent);

	return 1;
}

void download(Webs *wp, char *path, char *query){
	WebsHandlerProc service = (*wp).route->handler->service; 
//	(*wp).route->handler->close = (*avolfileClose);
	(*wp).route->handler->service =(*avolfileHandler); 
	(*wp).route->handler->service(wp); 
	(*wp).route->handler->service= service; 
}
