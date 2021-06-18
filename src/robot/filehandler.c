#include    "goahead.h"
#include	"cJSON.h"
#include    "md5.h"
#include 	"tools.h"
#include 	"robot_socket.h"
#include	"filehandler.h"

extern ACCOUNT_INFO cur_account;
//extern SOCKET_INFO socket_cmd;
//extern SOCKET_INFO socket_vir_cmd;
//extern int robot_type;
static void fileWriteEvent(Webs *wp);
static int avolfileHandler(Webs *wp);
static int compute_file_md5(const char *file_path, char *md5_str);
static int check_upfile(const char *upgrade_path, const char *readme_now_path, const char *readme_up_path);
static int check_version(const char *readme_now_path, const char *readme_up_path);
static int update_config(char *filename);

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
  update user.config/ex_device.config/exaxis.config
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

	if (!strcmp(filename, ROBOT_CFG)) {
		strcpy(upgrade_filename, UPGRADE_ROBOT_CFG);
		strcpy(now_filename, ROBOT_CFG);
	} else if (!strcmp(filename, EX_DEVICE_CFG)) {
		strcpy(upgrade_filename, UPGRADE_EX_DEVICE_CFG);
		strcpy(now_filename, EX_DEVICE_CFG);
	} else if (!strcmp(filename, EXAXIS_CFG)) {
		strcpy(upgrade_filename, UPGRADE_EXAXIS_CFG);
		strcpy(now_filename, EXAXIS_CFG);
	}
	printf("upgrade_filename = %s\n", upgrade_filename);
	printf("now_filename = %s\n", now_filename);

	if ((fp_up = fopen(upgrade_filename, "r")) == NULL) {
		perror("user.config : open file");

		return FAIL;
	}
	while (fgets(strline_up, LINE_LEN, fp_up) != NULL) {
		line_index++;
		bzero(write_line, sizeof(char)*LINE_LEN);
		strcpy(write_line, strline_up);
		if (is_in(strline_up, "=") == 1 && line_index > 2) {
			if (string_to_string_list(strline_up, " = ", &size_up, &array_up) == 0 || size_up != 2) {
				perror("string to string list");
				fclose(fp_up);
				string_list_free(array_up, size_up);

				return FAIL;
			}
			if (array_up[0] != NULL) {// 出现左边的 key 值
				//  now config file is not exist
				if ((fp_now = fopen(now_filename, "r")) == NULL) {
					fclose(fp_up);
					perror("now user.config : open file");
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
						string_list_free(array_now, size_now);

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
							string_list_free(array_now, size_now);
							bzero(strline_now, sizeof(char)*LINE_LEN);
							break;
						}
					}
					string_list_free(array_now, size_now);
					bzero(strline_now, sizeof(char)*LINE_LEN);
				}
				fclose(fp_now);
			}
			string_list_free(array_up, size_up);
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
				upfile = sfmt("%s", FILE_USERDATA);
				my_syslog("普通操作", "导入用户数据文件成功", cur_account.username);
				my_en_syslog("normal operation", "Import of user data file successfully", cur_account.username);
				my_jap_syslog("普通の操作", "ユーザーデータファイルのインポートに成功しました", cur_account.username);
				strcpy(filename, upfile);
			/* control user file */
			} else if (strcmp(up->clientFilename, "user.config") == 0) {
				upfile = sfmt("%s", WEB_ROBOT_CFG);
				my_syslog("普通操作", "导入控制器端用户配置文件成功", cur_account.username);
				my_en_syslog("normal operation", "Import of controller user profile successfully", cur_account.username);
				my_jap_syslog("普通の操作", "コントローラ側のユーザプロファイルのインポートが成功しました", cur_account.username);
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
	} else if (strcmp(filename, FILE_USERDATA) == 0) {
		system("rm -rf /root/web/file");
		system("rm -f /root/robot/exaxis.config");
		system("rm -f /root/robot/ex_device.config");
		system("cd /root/ && tar -zxvf fr_user_data.tar.gz");
		//system("rm -f /root/fr_user_data.tar.gz");
	} else if (strcmp(filename, UPGRADE_SOFTWARE) == 0) {
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
		/** 更新 user.config 文件 */
		update_config(ROBOT_CFG);
		/** 更新 ex_device.config 文件 */
		update_config(EX_DEVICE_CFG);
		/** 更新 exaxis.config 文件 */
		update_config(EXAXIS_CFG);

		/** 文件写入硬盘需要一定时间，等待 1 秒 */
		sleep(1);

		bzero(cmd, sizeof(cmd));
#if recover_mode
		//sprintf(cmd, "sh %s", SHELL_WEBTARCP);
		sprintf(cmd, "sh %s", UPGRADE_WEBTARCP);
#else
		sprintf(cmd, "sh %s", SHELL_RECOVER_WEBTARCP);
#endif
		do {
			system(cmd);
			/** 文件写入硬盘需要一定时间，等待 5 秒 */
			sleep(5);
		} while (check_version(README_WEB_NOW, README_WEB_UP) == FAIL);
		my_syslog("普通操作", "升级 webapp 成功", cur_account.username);
		my_en_syslog("normal operation", "Successed to upgrade webapp", cur_account.username);
		my_jap_syslog("普通の操作", "webappのアップグレードに成功", cur_account.username);

		bzero(cmd, sizeof(cmd));
#if recover_mode
		//sprintf(cmd, "sh %s", SHELL_CRLUPGRADE);
		sprintf(cmd, "sh %s", UPGRADE_CRLUPGRADE);
#else
		sprintf(cmd, "sh %s", SHELL_RECOVER_CRLUPGRADE);
#endif
		do {
			system(cmd);
			/** 文件写入硬盘需要一定时间，等待 1 秒 */
			sleep(1);
		} while (check_version(README_CTL_NOW, README_CTL_UP) == FAIL);
		my_syslog("普通操作", "升级控制器软件成功", cur_account.username);
		my_en_syslog("normal operation", "Controller software upgrade successful", cur_account.username);
		my_jap_syslog("普通の操作", "コントローラソフトウェアのアップグレードに成功", cur_account.username);

		system("rm -f /tmp/software.tar.gz && rm -rf /tmp/fr_control && rm -rf /tmp/web && rm -rf /tmp/software");

	//	system("sleep 1 && shutdown -b &");

	/*} else if (strcmp(filename, UPGRADE_WEBAPP) == 0) {
		system("cd /tmp && tar -zxvf webapp.tar.gz");
		if (check_upfile(UPGRADE_WEB, README_WEB_NOW, README_WEB_UP) == FAIL) {
			perror("md5 & version check fail!");
			my_syslog("普通操作", "升级 webapp 失败", cur_account.username);
			goto end;
		}
		bzero(cmd, sizeof(cmd));
		sprintf(cmd, "openssl des3 -d -k frweb -salt -in %s | tar xzvf - -C /tmp/", UPGRADE_WEB);
		if (system(cmd) != 0) {
			my_syslog("普通操作", "升级 webapp 失败", cur_account.username);
			perror("uncompress fail!");
			goto end;
		}*/

	/**	bzero(cmd, sizeof(cmd));
		sprintf(cmd, "chmod 777 %s", SHELL_WEBUPGRADE);
		system(cmd);
	*/
	/*	bzero(cmd, sizeof(cmd));
		sprintf(cmd, "sh %s", SHELL_WEBTARCP);
		system(cmd);*/
	//	bzero(cmd, sizeof(cmd));
	//	sprintf(cmd, "nohup %s %s &", SHELL_WEBUPGRADE, CLIENT_IP);
	//	sprintf(cmd, "nohup sh %s %s &", SHELL_WEBUPGRADE, CLIENT_IP);
	//	sprintf(cmd, "sh %s %s", SHELL_WEBUPGRADE, CLIENT_IP);
	//	sprintf(cmd, "sh %s %s &", SHELL_WEBUPGRADE, CLIENT_IP);
	//	char *exec_argv[] = {"web_upgrade.sh", "192.168.58.2", NULL};
	//	execv("/root/web/webserver/shell/web_upgrade.sh", exec_argv);
	//	sleep(5);

		/* send webappupgrade cmd to TM to do web_upgrade.sh shell */
	/*	if (robot_type == 1) { // "1" 代表实体机器人
			sock_cmd = &socket_cmd;
		} else { // "0" 代表虚拟机器人
			sock_cmd = &socket_vir_cmd;
		}
		socket_enquene(sock_cmd, 344, "WebAppUpgrade()", 1);
		printf("webapp upgrade success!\n");
		my_syslog("普通操作", "升级 webapp 成功", cur_account.username);
	} else if (strcmp(filename, UPGRADE_CONTROL) == 0) {
		system("cd /tmp && tar -zxvf control.tar.gz");
		if (check_upfile(UPGRADE_FR_CONTROL, README_CTL_NOW, README_CTL_UP) == FAIL) {
			my_syslog("普通操作", "升级控制器软件失败", cur_account.username);
			goto end;
		}
		bzero(cmd, sizeof(cmd));
		sprintf(cmd, "sh %s", SHELL_CRLUPGRADE);
		system(cmd);
		my_syslog("普通操作", "升级控制器软件成功", cur_account.username);*/
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

	assert(websValid(wp));
	assert(wp->method);
	assert(wp->filename && wp->filename[0]);    

	pathfilename = websGetVar(wp, "pathfilename", NULL);
	printf("pathfilename = %s\n", pathfilename);
	if (pathfilename == NULL)
		return 1;
	if (strcmp(pathfilename, FILE_USERDATA) == 0) {
		char cmd[128] = {0};
		sprintf(cmd, "cp %s %s", ROBOT_CFG, WEB_ROBOT_CFG);
		system(cmd);
		memset(cmd, 0, 128);
		sprintf(cmd, "cp %s %s", EX_DEVICE_CFG, WEB_EX_DEVICE_CFG);
		system(cmd);
		memset(cmd, 0, 128);
		sprintf(cmd, "cp %s %s", EXAXIS_CFG, WEB_EXAXIS_CFG);
		system(cmd);
		system("rm -f /root/fr_user_data.tar.gz");
		system("cd /root/ && tar -zcvf fr_user_data.tar.gz ./web/file ./robot/exaxis.config ./robot/ex_device.config");
		my_syslog("普通操作", "导出用户数据文件成功", cur_account.username);
		my_en_syslog("normal operation", "Export of user data file successful", cur_account.username);
		my_jap_syslog("普通の操作", "ユーザーデータファイルのエクスポートに成功", cur_account.username);
	} else if (strcmp(pathfilename, DB_POINTS) == 0) {
		my_syslog("普通操作", "导出示教点文件成功", cur_account.username);
		my_en_syslog("normal operation", "Export of points file successful", cur_account.username);
		my_jap_syslog("普通の操作", "ポイントファイルを提示して成功させる", cur_account.username);
	} else if (strcmp(pathfilename, FILE_CFG) == 0) {
		my_syslog("普通操作", "导出 web 端系统配置文件成功", cur_account.username);
		my_en_syslog("normal operation", "Export of web system configure file successful", cur_account.username);
		my_jap_syslog("普通の操作", "web側システムプロファイルのエクスポートに成功しました", cur_account.username);
	} else if (strcmp(pathfilename, ROBOT_CFG) == 0) {
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
