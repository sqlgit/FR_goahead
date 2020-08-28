#include    "goahead.h"
#include	"cJSON.h"
#include    "md5.h"
#include 	"tools.h"
#include 	"robot_socket.h"
#include	"filehandler.h"

extern ACCOUNT_INFO cur_account;
static void fileWriteEvent(Webs *wp);
static int avolfileHandler(Webs *wp);
static int compute_file_md5(const char *file_path, char *md5_str);
static int check_upfile(const char *upgrade_path, const char *readme_now_path, const char *readme_up_path);

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
	char version_now[20] = "";
	char version_up[20] = "";

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
		} else if (!strncmp(strline, "VERSION=", 8)) {
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

	printf("md5sum_com = %s\n", md5sum_com);
	printf("md5sum_up = %s\n", md5sum_up);
	printf("version_now = %s\n", version_now);
	printf("version_up = %s\n", version_up);
	printf("strcmp(md5sum_com, md5sum_up) = %d\n", strcmp(md5sum_com, md5sum_up));
	printf("strcmp(version_up, version_now) = %d\n", strcmp(version_up, version_now));

	if (strcmp(md5sum_com, md5sum_up) != 0 || strcmp(version_up, version_now) <= 0) {
		perror("upgrade fail");

		return FAIL;
	}

	return SUCCESS;
}


void upload(Webs *wp)
{
	WebsKey         *s;
	WebsUpload      *up;
	char            *upfile;
	char filename[128] = {0};
	char cmd[128] = {0};

	if (scaselessmatch(wp->method, "POST")) {
		for (s = hashFirst(wp->files); s; s = hashNext(wp->files, s)) {
			up = s->content.value.symbol;
			/* close printf */
			/*websWrite(wp, "FILE: %s\r\n", s->name.value.string);
			websWrite(wp, "FILENAME=%s\r\n", up->filename);
			websWrite(wp, "CLIENT=%s\r\n", up->clientFilename);
			websWrite(wp, "TYPE=%s\r\n", up->contentType);
			websWrite(wp, "SIZE=%d\r\n", up->size);*/
			/*printf("FILE: %s\r\n", s->name.value.string);
		    printf("FILENAME=%s\r\n", up->filename);
			printf("CLIENT=%s\r\n", up->clientFilename);
			printf("TYPE=%s\r\n", up->contentType);
			printf("SIZE=%d\r\n", up->size);*/

			/* web_point.db file */
			if (strcmp(up->clientFilename, "web_point.db") == 0) {
				upfile = sfmt("%s", DB_POINTS);
				my_syslog("普通操作", "导入示教点文件成功", cur_account.username);
			/* user lua file */
			} else if (is_in(up->clientFilename, ".lua") == 1) {
				upfile = sfmt("%s%s", DIR_USER, up->clientFilename);
				my_syslog("普通操作", "导入用户程序文件成功", cur_account.username);
			} else if (is_in(up->clientFilename, ".dae") == 1 || is_in(up->clientFilename, ".stl") == 1) {
				upfile = sfmt("%s%s", UPLOAD_TOOL_MODEL, up->clientFilename);
				my_syslog("普通操作", "导入工具模型成功", cur_account.username);
				sprintf(filename, "%s%s", LOAD_TOOL_MODEL, up->clientFilename);
			/* web system config file */
			} else if (strcmp(up->clientFilename, "system.txt") == 0) {
				upfile = sfmt("%s", FILE_CFG);
				my_syslog("普通操作", "导入 web 端系统配置文件成功", cur_account.username);
				strcpy(filename, upfile);
			/* web user data file */
			} else if (strcmp(up->clientFilename, "fr_user_data.tar.gz") == 0) {
				upfile = sfmt("%s", FILE_USERDATA);
				my_syslog("普通操作", "导入用户数据文件成功", cur_account.username);
				strcpy(filename, upfile);
			/* control user file */
			} else if (strcmp(up->clientFilename, "user.config") == 0) {
				upfile = sfmt("%s", ROBOT_CFG);
				my_syslog("普通操作", "导入控制器端用户配置文件成功", cur_account.username);
			/* webapp upgrade file */
			} else if (strcmp(up->clientFilename, "webapp.tar.gz") == 0) {
				upfile = sfmt("%s", UPGRADE_WEBAPP);
				my_syslog("普通操作", "导入 webapp 升级文件成功", cur_account.username);
				strcpy(filename, upfile);
			/* control upgrade file */
			} else if (strcmp(up->clientFilename, "control.tar.gz") == 0) {
				upfile = sfmt("%s", UPGRADE_CONTROL);
				my_syslog("普通操作", "导入控制器升级文件成功", cur_account.username);
				strcpy(filename, upfile);
			} else {
				my_syslog("普通操作", "导入文件未匹配", cur_account.username);
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
		system("rm -f /root/robot/user.config");
		system("cd /root/ && tar -zxvf fr_user_data.tar.gz");
		system("rm -f /root/fr_user_data.tar.gz");
	} else if (strcmp(filename, UPGRADE_WEBAPP) == 0) {
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
		}

	/**	bzero(cmd, sizeof(cmd));
		sprintf(cmd, "chmod 777 %s", SHELL_WEBUPGRADE);
		system(cmd);
	 */
		bzero(cmd, sizeof(cmd));
		//sprintf(cmd, "nohup %s %s &", SHELL_WEBUPGRADE, CLIENT_IP);
		//sprintf(cmd, "sh %s %s &", SHELL_WEBUPGRADE, CLIENT_IP);
		sprintf(cmd, "sh %s %s", SHELL_WEBUPGRADE, CLIENT_IP);
		system(cmd);
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
		my_syslog("普通操作", "升级控制器软件成功", cur_account.username);
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
		system("rm -f /root/fr_user_data.tar.gz");
		system("cd /root/ && tar -zcvf fr_user_data.tar.gz ./web/file ./robot/user.config");
		my_syslog("普通操作", "导出用户数据文件成功", cur_account.username);
	} else if (strcmp(pathfilename, DB_POINTS) == 0) {
		my_syslog("普通操作", "导出示教点文件成功", cur_account.username);
	} else if (strcmp(pathfilename, FILE_CFG) == 0) {
		my_syslog("普通操作", "导出 web 端系统配置文件成功", cur_account.username);
	} else if (strcmp(pathfilename, ROBOT_CFG) == 0) {
		my_syslog("普通操作", "导出控制器端用户配置文件成功", cur_account.username);
	} else if (is_in(pathfilename, DIR_USER) == 1) {
		my_syslog("普通操作", "导出用户程序文件成功", cur_account.username);
	} else if (strcmp(pathfilename, FILE_STATEFB) == 0) {
		my_syslog("普通操作", "导出状态查询文件成功", cur_account.username);
	} else if (is_in(pathfilename, DIR_LOG) == 1) {
		my_syslog("普通操作", "导出 log 文件成功", cur_account.username);
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
