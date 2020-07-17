
/********************************* Includes ***********************************/

#include 	"goahead.h"
#include	"cJSON.h"
#include 	"tools.h"

/********************************* Defines ************************************/

extern ACCOUNT_INFO cur_account;
extern timer_t timerid;
extern print_num;

/*********************************** Code *************************************/

/*
pszInput:输入待分割字符串
pszDelimiters:分割标识符
Ary_num:分割的份数
Ary_size:每份的size
pszAry_out:分割的子串的输出参数
 */
int separate_string_to_array(char *pszInput, char *pszDelimiters , unsigned int Ary_num, unsigned int Ary_size, char *pszAry_out)
{
	//char *pszData = strdup(pszInput);
	char pszData[20480]={0};
	strcpy(pszData, pszInput);
	char *pszToken = NULL, *pszToken_saveptr;
	unsigned int Ary_cnt = 0;

	pszToken = strtok_r(pszData, pszDelimiters, &pszToken_saveptr);
	while (pszToken != NULL) {
		//printf("pszToken=%s\n", pszToken);
		memcpy(pszAry_out + Ary_cnt*Ary_size, pszToken, Ary_size);
		if(++Ary_cnt >= Ary_num)
			break;
		pszToken = strtok_r(NULL, pszDelimiters, &pszToken_saveptr);
	}
	//free(pszData);

	return Ary_cnt;
}

/* 获取整数的长度 */
int get_n_len(const int n)
{
	char str[100] = {0};
	sprintf(str, "%d", n);

	return strlen(str);
}

/* write file */
int write_file(const char *file_name, const char *file_content)
{
	//printf("write filename = %s\n", file_name);
	//printf("write filecontent = %s\n", file_content);
	FILE *fp = NULL;

	if((fp = fopen(file_name, "w")) == NULL) {
		perror("open file");

		return FAIL;
	}
	if(fputs(file_content, fp) < 0){
		perror("write file");
		fclose(fp);

		return FAIL;
	}
	fclose(fp);

	return SUCCESS;
}

/* oepn file and return file content without '\n' '\r' '\t' */
char *get_file_content(const char *file_path)
{
	FILE *fp = NULL;
	int file_size = 0;
	int i = 0;
	char *content = NULL;
	char *tmp = NULL;

//	printf("file_path = %s\n", file_path);
	if ((fp = fopen(file_path, "r")) == NULL) {
		perror("open file");

		return NULL;
	}
	fseek(fp, 0, SEEK_END);
	file_size = ftell(fp);
//	printf("file_size = %d\n", file_size);
	fseek(fp, 0, SEEK_SET);

	tmp = (char *)calloc(1, file_size*sizeof(char)+1);
	if(tmp == NULL) {
		perror("calloc");
		fclose(fp);

		return NULL;
	}
	fread(tmp, sizeof(char), file_size, fp);
/*
	printf("tmp = %s\n", tmp);
	printf("strlen tmp = %d\n", strlen(tmp));
	printf("sizeof tmp = %d\n", sizeof(tmp));
*/
	char file_content[file_size+1];
	memset(file_content, 0, (file_size+1));
	char *tmp2 = tmp;
	while(*tmp2 != '\0') {
		if(*tmp2 != '\n' && *tmp2 != '\r' && *tmp2 !='\t'){
			file_content[i] = *tmp2;
			i++;
		}
		tmp2++;
	}
	file_content[i] = '\0';
	content = (char *)calloc(1, strlen(file_content)+1);
	if(content != NULL) {
		strcpy(content, file_content);
	} else {
		perror("calloc");
	}
	free(tmp);
	tmp = NULL;
	fclose(fp);

	return content;
}

/* open file and return complete file content */
char *get_complete_file_content(const char *file_path)
{
	FILE *fp = NULL;
	int file_size = 0;
	char *content = NULL;

	if ((fp = fopen(file_path, "r")) == NULL) {
		perror("open file");

		return NULL;
	}
	fseek(fp, 0, SEEK_END);
	file_size = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	content = (char *)calloc(1, file_size*sizeof(char)+1);
	if(content == NULL) {
		perror("calloc");
		fclose(fp);

		return NULL;
	}
	fread(content, sizeof(char), file_size, fp);
	fclose(fp);

	return content;
}

/* open dir and return dir's file name and file content */
char *get_dir_content(const char *dir_path)
{
	DIR *dir = NULL;
	struct dirent *ptr = NULL;
	char *content = NULL;
	char *buf = NULL;
	char *f_content = NULL;
	char dir_filename[100] = {0};
	cJSON *root_json = NULL;
	cJSON *file_cont = NULL;

	root_json = cJSON_CreateObject();
	dir = opendir(dir_path);
	if (dir == NULL) {
		perror("opendir");

		return NULL;
	}
	while ((ptr = readdir(dir)) != NULL) {
		/* current dir OR parrent dir */
		if(strcmp(ptr->d_name, ".") == 0 || strcmp(ptr->d_name, "..") == 0)
			continue;
		bzero(dir_filename, sizeof(dir_filename));
		sprintf(dir_filename, "%s%s", dir_path, ptr->d_name);
		/* open lua file */
		f_content = get_complete_file_content(dir_filename);
		if (f_content == NULL) {
			perror("Open file error");
			continue;
		}
		//printf("f_content = %s\n", f_content);
		file_cont = cJSON_CreateObject();
		cJSON_AddStringToObject(file_cont, "name", ptr->d_name);
		cJSON_AddStringToObject(file_cont, "pgvalue", f_content);
		cJSON_AddItemToObject(root_json, ptr->d_name, file_cont);
		free(f_content);
		f_content = NULL;
	}
	buf = cJSON_Print(root_json);
	//printf("buf = %s\n", buf);
	content = (char *)calloc(1, strlen(buf)+1);
	if(content != NULL) {
		strcpy(content, buf);
	} else {
		perror("calloc");
	}
	free(buf);
	buf = NULL;
	cJSON_Delete(root_json);
	root_json = NULL;
	if (dir != NULL) {
		closedir(dir);
		dir = NULL;
	}

	return content;
}

/* open dir and return dir's file name */
// Ext:["2020-03-15.json","2020-03-14.json","2020-03-13.json","2020-03-12.json","2020-03-11.json","2020-03-10.json"]
// Ext:["2020-03-15.txt", "2020-03-14.txt", "2020-03-13.txt"]
char *get_dir_filename(const char *dir_path)
{
	DIR *dir = NULL;
	struct dirent *ptr = NULL;
	char *content = NULL;
	char *buf = NULL;
	cJSON *root_json = NULL;

	root_json = cJSON_CreateArray();
	dir = opendir(dir_path);
	if (dir == NULL) {
		perror("opendir");

		return NULL;
	}
	while ((ptr = readdir(dir)) != NULL) {
		/* current dir OR parrent dir */
		if(strcmp(ptr->d_name, ".") == 0 || strcmp(ptr->d_name, "..") == 0)
			continue;
		//cJSON_AddStringToObject(root_json, "key", ptr->d_name);
		cJSON_InsertItemInArray(root_json, 0, cJSON_CreateString(ptr->d_name));
	}
	buf = cJSON_Print(root_json);
	printf("buf = %s\n", buf);
	content = (char *)calloc(1, strlen(buf)+1);
	if(content != NULL) {
		strcpy(content, buf);
	} else {
		perror("calloc");
	}
	free(buf);
	buf = NULL;
	cJSON_Delete(root_json);
	root_json = NULL;
	if (dir != NULL) {
		closedir(dir);
		dir = NULL;
	}

	return content;
}

/*
	open dir and judge dir's file name existed or not
	return "1" exist, "0" not exist
 */
int check_dir_filename(const char *dir_path, const char *filename)
{
	DIR *dir = NULL;
	struct dirent *ptr = NULL;

	dir = opendir(dir_path);
	if (dir == NULL) {
		perror("opendir");

		return 0;
	}
	while ((ptr = readdir(dir)) != NULL) {
		/* current dir OR parrent dir */
		if (strcmp(ptr->d_name, ".") == 0 || strcmp(ptr->d_name, "..") == 0)
			continue;
		if (strcmp(ptr->d_name, filename) == 0) {
			return 1;
		}
	}

	return 0;
}

/* open dir and return dir's file name only .txt file without name suffix*/
// Ext:["test.txt","test2.txt"]
char *get_dir_filename_txt(const char *dir_path)
{
	DIR *dir = NULL;
	struct dirent *ptr = NULL;
	char *content = NULL;
	char *buf = NULL;
	cJSON *root_json = NULL;

	root_json = cJSON_CreateArray();
	dir = opendir(dir_path);
	if (dir == NULL) {
		perror("opendir");

		return NULL;
	}
	while ((ptr=readdir(dir)) != NULL) {
		/* current dir OR parrent dir */
		if(is_in(ptr->d_name, ".txt") == 1) {
			strrpc(ptr->d_name, ".txt", "");
			cJSON_AddStringToObject(root_json, "key", ptr->d_name);
		}
	}
	buf = cJSON_Print(root_json);
	printf("buf = %s\n", buf);
	content = (char *)calloc(1, strlen(buf)+1);
	if(content != NULL) {
		strcpy(content, buf);
	} else {
		perror("calloc");
	}
	free(buf);
	buf = NULL;
	cJSON_Delete(root_json);
	root_json = NULL;
	if (dir != NULL) {
		closedir(dir);
		dir = NULL;
	}

	return content;
}

/**
   实现字符串中所有旧字符串替换为新的字符串,
   在 str 长度较长时，例如 1024 字节，
   谨慎使用，效率较低，会占用大量 cpu 时间
*/

char *strrpc(char *str, const char *oldstr, const char *newstr)
{
	int i;
	char bstr[strlen(str)+10];//转换缓冲区
	memset(bstr, 0, sizeof(strlen(str)+10));

	for (i = 0; i < strlen(str); i++) {
		if (!strncmp(str+i, oldstr, strlen(oldstr))) {//查找目标字符串
			strcat(bstr, newstr);
			i += strlen(oldstr) - 1;
		} else {
			strncat(bstr, str + i, 1);//保存一字节进缓冲区
		}
	}
	strcpy(str, bstr);

	return str;
}

/* 判断一个字符串是否包含另一个字符串 */
int is_in(char *s, char *c)
{
	int i=0,j=0,flag=-1;
	while(i<strlen(s) && j<strlen(c)){
		if(s[i]==c[j]){//如果字符相同则两个字符都增加
			i++;
			j++;
		}else{
			i=i-j+1; //主串字符回到比较最开始比较的后一个字符
			j=0;     //字串字符重新开始
		}
		if(j==strlen(c)){ //如果匹配成功
			flag=1;  //字串出现
			break;
		}
	}

	return flag;
}

/* 毫秒定时器 */
void delay_ms(const int timeout)
{
	struct timeval timer;
	bzero(&timer, sizeof(struct timeval));
	timer.tv_sec        = 0;    // 秒
	timer.tv_usec       = 1000*timeout; // 1000us = 1ms
	select(0, NULL, NULL, NULL, &timer);
}

/* The double type retains a few bits */
double double_round(double dVal, int iPlaces) //iPlaces>=0
{
	unsigned char s[20];
	double dRetval;

	sprintf(s, "%.*lf", iPlaces, dVal);
	sscanf(s, "%lf", &dRetval);

	return dRetval;
}

/* two uint8_t type value to save in array[16] */
void uint8_to_array(int n1, int n2, int *array)
{
	int i = 0;

	for(i = 0; i < 16; i++) {
		if (i < 8) {
			array[i] = n1%2;
			n1 = n1/2;
		}
		if (i >= 8) {
			array[i] = n2%2;
			n2 = n2/2;
		}
	}
}

/* uint16_t type value to save in array[16] */
void uint16_to_array(int n, int *array)
{
	int i = 0;

	for(i = 0; i < 16; i++) {
		array[i] = n%2;
		n = n/2;
	}
}

int BytesToString(BYTE *pSrc, char *pDst, int nSrcLength)
{
	const char tab[] = "0123456789ABCDEF";
	int i = 0;

	for(i = 0; i < nSrcLength; i++) {
		*pDst++ = tab[*pSrc >> 4];
		*pDst++ = tab[*pSrc & 0x0f];
		*pDst++ = ' ';
		pSrc++;
	}

	*pDst = '\0';

	return 3*nSrcLength;
}

int StringToBytes(char *pSrc, BYTE *pDst, int nSrcLength)
{
	int i = 0;

	for (i = 0; i < nSrcLength; i++) {
		// 输出高4位
		if ((*pSrc >= '0') && (*pSrc <= '9'))
			*pDst = (*pSrc - '0') << 4;
		else if ((*pSrc >= 'A') && (*pSrc <= 'Z'))
			*pDst = (*pSrc - 'A' + 10) << 4;
		else
			*pDst = (*pSrc - 'a' + 10) << 4;
		pSrc++;

		// 输出低4位
		if ((*pSrc >= '0') && (*pSrc <= '9'))
			*pDst |= *pSrc - '0';
		else if ((*pSrc >= 'A') && (*pSrc <= 'Z'))
			*pDst |= *pSrc - 'A' + 10;
		else
			*pDst |= *pSrc - 'a' + 10;

		pSrc++;
		pDst++;
		pSrc++;
	}

	// 返回目标数据长度
	return nSrcLength;
}

/* record syslog */
int my_syslog(const char *class, const char *content, const char *user)
{
	struct tm *my_local;
	time_t t;
	int ret = FAIL;
	char filename[100] = {0};
	char now_time[100] = {0};
	char dir_filename[100] = {0};
	char *buf = NULL;
	char *f_content = NULL;
	cJSON *root_json = NULL;
	cJSON *newitem = NULL;
	t = time(NULL);

	//printf("ctime(&t) = %s\n", ctime(&t));
	my_local = localtime(&t);
	//printf("Local sec is: %d\n", my_local->tm_sec); /* 秒 – 取值区间为[0,59] */
	//printf("Local min is: %d\n", my_local->tm_min); /* 分 - 取值区间为[0,59] */
	//printf("Local hour is: %d\n", my_local->tm_hour); /* 时 - 取值区间为[0,23] */
	//printf("Local mday is: %d\n", my_local->tm_mday); /* 一个月中的日期 - 取值区间为[1,31] */
	//printf("Local mon is: %d\n", (my_local->tm_mon+1)); /* 月份（从一月开始，0代表一月） - 取值区间为[0,11] */
	//printf("Local year is: %d\n", (my_local->tm_year+1900)); /* 年份，其值等于实际年份减去1900 */
	sprintf(filename, "%d-%d-%d", (my_local->tm_year+1900), (my_local->tm_mon+1), my_local->tm_mday);
	//printf("filename = %s\n", filename);
	sprintf(now_time, "%d:%d:%d", (my_local->tm_hour), (my_local->tm_min), my_local->tm_sec);

	sprintf(dir_filename, "%s%s.txt", DIR_LOG, filename);
	//printf("dir_filename = %s\n", dir_filename);

	f_content = get_file_content(dir_filename);
	/* file is NULL */
	if (f_content == NULL) {
		delete_log_file(1);
		root_json = cJSON_CreateArray();
	} else {
		root_json = cJSON_Parse(f_content);
	}

	newitem = cJSON_CreateObject();
	cJSON_AddStringToObject(newitem, "time", now_time);
	cJSON_AddStringToObject(newitem, "class", class);
	cJSON_AddStringToObject(newitem, "content", content);
	cJSON_AddStringToObject(newitem, "user", user);
	//cJSON_AddItemToArray(root_json, newitem);
	/** 将一个新元素插入数组中的0索引的位置，旧的元素的索引依次加1 */
	cJSON_InsertItemInArray(root_json, 0, newitem);
	buf = cJSON_Print(root_json);

	ret = write_file(dir_filename, buf);//write file

	free(buf);
	buf = NULL;
	cJSON_Delete(root_json);
	root_json = NULL;
	free(f_content);
	f_content = NULL;

	return SUCCESS;
}

/* delete log file: 删除旧的 log 文件, 只保留 web 系统配置文件中的 log_count 个 log 文件 */
int delete_log_file(int flag)
{
	char cmd[128] = {0};
	char *f_content = NULL;
	cJSON *root_json = NULL;
	cJSON *count = NULL;
	int log_count = 0;

	f_content = get_file_content(FILE_CFG);
	/* file is not NULL */
	if (f_content != NULL) {
		root_json = cJSON_Parse(f_content);
		if (root_json != NULL) {
			count = cJSON_GetObjectItem(root_json, "log_count");
			if (count != NULL) {
				printf("count = %d\n", count->valuestring);
				if (flag) {//此时马上需要新增一个 log文件，所以需要多删除一个最旧的 log 文件
					log_count = atoi(count->valuestring) - 1;
				} else {
					log_count = atoi(count->valuestring);
				}
				sprintf(cmd, "sh %s %d", SHELL_DELETELOG, log_count);
				system(cmd);
			}
		}
	}

	return SUCCESS;
}

void *create_dir(const char *dir_path)
{
	//printf("dir_path = %s\n", dir_path);
	/* create file dir */
	if (opendir(dir_path) == NULL) {
		perror("Not found DIR");
		if (mkdir(dir_path, 0777) != 0) {
			perror("mkdir DIR");
		} else {
			printf("mkdir DIR SUCCESS!\n");
		}
	}
}

/**
  当Web前端发起http请求时，根据cmd类型获取权限类型，
  如果cmd权限类型不在当前用户权限范围内，返回权限出错给Web前端，
  如果权限匹配通过，继续正常的业务逻辑处理流程。
*/
int authority_management(const char *cmd_type)
{
	if (!strcmp(cur_account.auth, "0")) { //管理员
		if (!strcmp(cmd_type, "0")) {
			return 1;
		} else {
			return 0;
		}
	} else if (!strcmp(cur_account.auth, "1")) { //程序员
		if (!strcmp(cmd_type, "1") || !strcmp(cmd_type, "2")) {
			return 1;
		} else {
			return 0;
		}
	} else if (!strcmp(cur_account.auth, "2")) { //操作员
		if (!strcmp(cmd_type, "2")) {
			return 1;
		} else {
			return 0;
		}
	} else {
		perror("authority_management");

		return 0;
	}

	return 0;
}

/* delete timer */
void delete_timer()
{
	// delete 定时器
	if (timer_delete(timerid) == -1) {
		perror("fail to timer_delete");

		return FAIL;
	}

	//printf("%d : delete timer success \n", print_num);
	return SUCCESS;
}
