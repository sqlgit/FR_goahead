
/********************************* Includes ***********************************/

#include 	"goahead.h"
#include	"cJSON.h"
#include 	"robot_socket.h"
#include 	"tools.h"

/********************************* Defines ************************************/

WEBAPP_SYSCFG web_cfg;
extern ACCOUNT_INFO cur_account;
extern timer_t timerid;
extern int robot_type;
extern int language;
extern SOCKET_INFO socket_cmd;
extern SOCKET_INFO socket_vir_cmd;
extern JIABAO_TORQUE_PRODUCTION_DATA jiabao_torque_pd_data;

/********************************************************************
*@function: string_to_string_list
*
*@param1 [IN]: src_str, 有固定分隔符的字符串
*
*@param2 [IN]: delimiter, 分隔符字符
*
*@param3 [OUT]: delimiter_count, 出参, 分隔符数量+1, 即 array 分割的份数
*
*@param4 [OUT]: str_list, 出参，用于存放结果的二位字符串数组，
*               使用完成后需要使用LB_FREE_ARRAY释放数组内存
*
*@return: 1 for ok, 0 for error
*
*@description: 将传入的有固定分隔符的字符串分解后存入到二维字符串数组
*
*
*效率较高：运行 100 万次大约耗时 2 秒

	*******************************************************************/
int string_to_string_list(char *src_str, char *delimiter, int *delimiter_count, char ***str_list)
{
	char *start = NULL;
	char* end = NULL;
	int count = 0;
	int ret = 0;

	if (src_str == NULL || delimiter_count == NULL || str_list == NULL)
	{
		printf("[%s:%d] param error...\n", __FUNCTION__, __LINE__);
		return ret;
	}

	start = src_str;
	while (*start != '\0')
	{
		if (strncmp(start, delimiter, strlen(delimiter)) == 0) // 统计数组元素的个数
		{
			count++;
		}
		start++;
	}

	count++;
	*str_list = (char **)memset(malloc(count * sizeof(char*)), 0, count*sizeof(char*));
	if (*str_list == NULL)
	{
		printf("[%s:%d] malloc failed...\n", __FUNCTION__, __LINE__);
		return ret;
	}

	*delimiter_count = count;
	count = 0;
	start = src_str;

	//    while (*start == ' ' || *start == '\t')  //过滤掉前面的空格
	//    {
	//        start++;
	//    }

	int size = 0;
	while (*start != '\0' && (end = strstr(start, delimiter)))
	{
		size = end - start + 1;
		(*str_list)[count] = (char *)memset(malloc(size * sizeof(char)), 0, size*sizeof(char));
		if ((*str_list)[count] != NULL)
		{
			strncpy((*str_list)[count], start, end - start);
			ret = 1;
		}
		else
		{
			printf("[%s:%d][Error] malloc failed...\n", __FUNCTION__, __LINE__);
			ret = 0;
			break;
		}

		count++;
		start = end + strlen(delimiter);
		//        while (*start == ' ' || *start == '\t') //过滤掉空格
		//        {
		//            start++;
		//        }
	}

	size = strlen(start) + 1;
	(*str_list)[count] = (char *)memset(malloc(size * sizeof(char)), 0, size*sizeof(char));
	if ((*str_list)[count] != NULL)
	{
		strncpy((*str_list)[count], start, strlen(start));
		ret = 1;
	}
	else
	{
		printf("[%s:%d][Error] malloc failed...\n", __FUNCTION__, __LINE__);
		ret = 0;
	}

	return ret;
}

void string_list_free(char ***str_list, int list_size)
{
	int index = 0;

	if ((*str_list != NULL) && (list_size > 0)) {
		while (index < list_size) {
			if ((*str_list)[index] != NULL) {
				free((*str_list)[index]);
				(*str_list)[index] = NULL;
			}
			index++;
		}
	}

	if (*str_list != NULL) {
		free(*str_list);
		*str_list = NULL;
	}
}

/**
	pData:输入待分割字符串
	pDelimiter:分割标识符
	Ary_num:分割的份数
	Ary_size:每份的size
	pAry_out:分割的子串的输出参数
*/
/*int separate_string_to_array(char *pData, char *pDelimiter , unsigned int Ary_num, unsigned int Ary_size, char ***pAry_out)
{
	assert(pData != NULL);
	//若 pData 为NULL，则抛出异常。

	assert(pDelimiter != NULL);
	//若 pDelimiter 为NULL，则抛出异常。

	assert(pAry_out != NULL);
	//若 pAry_out 为NULL，则抛出异常。

	//char *pSrc = strdup(pData);
	//char *pSrc = (char *)malloc(strlen(pData)+1);
	char *pSrc = NULL;
	char *pToken = NULL;
	char *pSave = NULL;
	unsigned int Ary_cnt = 0;
	pSrc = (char *)calloc(1, strlen(pData) + 1);
	if (pSrc == NULL) {
		perror("calloc");

		return -1;
	}

	strcpy(pSrc, pData);
	pToken = strtok_r(pSrc, pDelimiter, &pSave);
	while (pToken != NULL) {
		printf("pToken=%s\n", pToken);
		//memcpy(pAry_out+Ary_cnt*Ary_size, pToken, Ary_size-1);
		strncpy((*pAry_out)[Ary_cnt], pToken, strlen(pToken));
		if(++Ary_cnt >= Ary_num)
			break;
		pToken = strtok_r(NULL, pDelimiter, &pSave);
	}
	free(pSrc);
	pSrc = NULL;

	return Ary_cnt;
}*/

/* 获取整数的长度 */
int get_n_len(const int n)
{
	char str[100] = {0};
	sprintf(str, "%d", n);

	return strlen(str);
}

/* write file : w */
int write_file(const char *file_name, const char *file_content)
{
	//printf("write filename = %s\n", file_name);
	//printf("write filecontent = %s\n", file_content);
	FILE *fp = NULL;

	if((fp = fopen(file_name, "w+")) == NULL) {
		perror("open file");

		return FAIL;
	}
	if (file_content == NULL) {
		perror("file content null");

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

/* write file : type a+ */
int write_file_append(const char *file_name, const char *file_content)
{
	//printf("write filename = %s\n", file_name);
	//printf("write filecontent = %s\n", file_content);
	FILE *fp = NULL;

	if((fp = fopen(file_name, "a+")) == NULL) {
		perror("open file");

		return FAIL;
	}
	if (file_content == NULL) {
		perror("file content null");

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

/**
  该函数会顺序读取输入字符串中所有字符，当匹配到 '%' 时，在前面添加一个 '%'
  主要防止 cJSON_Print 时会对其进行格式化匹配

  函数执行效率较高.
  参考：函数在操作 520000 字节数的文件时，耗时大约 10ms 左右

  RETURN:
	fail: NULL
	Normal : 转换好后的字符串
*/
char *format_str(const char *source_str)
{
	int str_size = 0;
	int i = 0;
	int j = 0;
	char *dest_str = NULL;

	str_size = strlen(source_str);
	dest_str = (char *)calloc(1, str_size*sizeof(char)+1024);
	if (dest_str == NULL) {
		perror("calloc");

		return NULL;
	}

	while (source_str[i] != '\0') {
		if (source_str[i] == '%') {
			dest_str[j++] = '%';
		}
		dest_str[j++] = source_str[i];
		i++;
	}
	dest_str[j] = '\0';
	//printf("dest_str = %s\n", dest_str);

	return dest_str;
}

/**
  该函数一般用于获取 json 格式的文件内容
  open file and return file content without '\n' '\r' '\t'

  函数执行效率较高.
  参考：函数在操作 520000 字节数的文件时，耗时大约 10ms 左右

  RETURN:
	malloc fail: NULL
	no file: NO_FILE
	empty file: Empty
	Normal file: file content
*/
char *get_file_content(const char *file_path)
{
	FILE *fp = NULL;
	int file_size = 0;
	int i = 0;
	int j = 0;
	int errNum = 0;
	char *file_content = NULL;

	//clock_t start, finish;
	//start = clock();

	//printf("file_path = %s\n", file_path);

	if ((fp = fopen(file_path, "r")) == NULL) {
		//perror("File is empty");
		perror("file");
		errNum = errno;   	//先记录错误码，防止全局变量errno改变
		if(errNum == 2){	//没有此文件
			
			return "NO_FILE";
		}
		
		return NULL;
	}
	fseek(fp, 0, SEEK_END);
	file_size = ftell(fp);
	if (file_size == 0) {
		fclose(fp);
		return "Empty";
	}
	fseek(fp, 0, SEEK_SET);

	file_content = (char *)calloc(1, file_size*sizeof(char)+1);
	//file_content = (char *)calloc(1, 530000000000000);
	if (file_content == NULL) {
		perror("calloc");
		fclose(fp);

		return NULL;
	}
	fread(file_content, sizeof(char), file_size, fp);

	while (file_content[i] != '\0') {
		if(file_content[i] != '\n' && file_content[i] != '\r' && file_content[i] !='\t') {
			file_content[j++] = file_content[i];
		}
		i++;
	}
	file_content[j] = '\0';
	//printf("file_content = %s\n", file_content);
	fclose(fp);
	//finish = clock();
	//printf("%lf m seconds\n", (double)(finish-start)/CLOCKS_PER_SEC);

	return file_content;
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
	char *dest_str = NULL;
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
		file_cont = cJSON_CreateObject();
		cJSON_AddStringToObject(file_cont, "name", ptr->d_name);
		//printf("f_content = %s\n", f_content);
		dest_str = format_str(f_content);
		//printf("dest_str = %s\n", dest_str);
		free(f_content);
		f_content = NULL;
		cJSON_AddStringToObject(file_cont, "pgvalue", dest_str);
		cJSON_AddItemToObject(root_json, ptr->d_name, file_cont);
		free(dest_str);
		dest_str = NULL;
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
	//printf("content = %s\n", content);

	return content;
}

/* open dir and return dir's file name or dir's dir name */
// Ext:["2020-03-15.json","2020-03-14.json","2020-03-13.json","2020-03-12.json","2020-03-11.json","2020-03-10.json"]
// Ext:["2020-03-15.txt", "2020-03-14.txt", "2020-03-13.txt"]
// Ext:["spary","weld"]
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
		//cJSON_AddItemToArray(root_json, cJSON_CreateString(ptr->d_name));
		cJSON_InsertItemInArray(root_json, 0, cJSON_CreateString(ptr->d_name));
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

/* open dir and return dir's file name or dir's dir name, filename head with str */
// Ext:["ptemp_1.lua","ptemp_2.lua"]
char *get_dir_filename_strhead(const char *dir_path, const char *str)
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
		if (strcmp(ptr->d_name, ".") == 0 || strcmp(ptr->d_name, "..") == 0)
			continue;
		//printf("ptr->d_name = %s\n", ptr->d_name);
		//cJSON_AddStringToObject(root_json, "key", ptr->d_name);
		//cJSON_AddItemToArray(root_json, cJSON_CreateString(ptr->d_name));
		if (strncmp(ptr->d_name, str, strlen(str)) == 0) {
			cJSON_InsertItemInArray(root_json, 0, cJSON_CreateString(ptr->d_name));
		}
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

/*
	open dir and judge dir's file name or dir's dir name existed or not
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
			if (dir != NULL) {
				closedir(dir);
				dir = NULL;
			}
			return 1;
		}
	}
	if (dir != NULL) {
		closedir(dir);
		dir = NULL;
	}

	return 0;
}

/* open dir and return dir's file name only .txt file without name suffix*/
// Ext:["test","test2"]
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
	//printf("buf = %s\n", buf);
	content = (char *)calloc(1, strlen(buf)+1);
	if (content != NULL) {
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
	//char bstr[strlen(str)+10];//转换缓冲区
	//memset(bstr, 0, sizeof(strlen(str)+10));
	char *bstr = NULL;
	bstr = (char *)calloc(1, strlen(str)*sizeof(char)+10);

	for (i = 0; i < strlen(str); i++) {
		if (!strncmp(str+i, oldstr, strlen(oldstr))) {//查找目标字符串
			strcat(bstr, newstr);
			i += strlen(oldstr) - 1;
		} else {
			strncat(bstr, str + i, 1);//保存一字节进缓冲区
		}
	}
	strcpy(str, bstr);
	free(bstr);

	return str;
}

/* 判断一个字符串是否包含另一个字符串 */
int is_in(char *s, char *c)
{
	assert(s != NULL);

	assert(c != NULL);

	int i=0,j=0,flag=-1;
	while (i < strlen(s) && j < strlen(c)){
		if(s[i] == c[j]){//如果字符相同则两个字符都增加
			i++;
			j++;
		} else {
			i = i-j+1; //主串字符回到比较最开始比较的后一个字符
			j = 0;     //字串字符重新开始
		}
		if(j == strlen(c)){ //如果匹配成功
			flag = 1;  //字串出现
			break;
		}
	}

	return flag;
}

/*
	判断一个字符串是否包含另一个字符串
	返回值：
	若目标字符串是源字符串的子串，则返回目标字符串在原字符串中的首次出现的位置加上目标字符串长度；
	如果目标字符串不是源字符串的子串，则返回 -1
	Ext：
	SrcStr: XXXPTP(aaa,bb)
	DestStr: PTP
	Return: 6
*/
int is_in_srclen(const char *s, const char *c)
{
	assert(s != NULL);
	assert(c != NULL);

	int i = 0;
	int j = 0;
	int srclen = -1;
	int s_len = 0;
	int c_len = 0;

	s_len = strlen(s);
	c_len = strlen(c);
	while (i < s_len && j < c_len) {
		if (s[i] == c[j]) {//如果字符相同则两个字符都增加
			i++;
			j++;
		} else {
			i = i - j + 1; //主串字符回到比较最开始比较的后一个字符
			j = 0;     //字串字符重新开始
		}
		if (j == c_len) { //如果匹配成功
			srclen = i;  //字串出现, 返回 srclen
			break;
		}
	}

	return srclen;
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
	/* f_content is NULL */
	if (f_content == NULL) {
		return FAIL;
	/* no such file */
	} else if (strcmp(f_content, "NO_FILE") == 0) {
		if (delete_log_file(1) == FAIL) {
			return FAIL;
		}
		root_json = cJSON_CreateArray();
	/* f_content is empty */
	} else if (strcmp(f_content, "Empty") == 0) {
		root_json = cJSON_CreateArray();
	/* f_content exist */
	} else {
		root_json = cJSON_Parse(f_content);
		free(f_content);
		f_content = NULL;
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

	return SUCCESS;
}

/* record en syslog */
int my_en_syslog(const char *class, const char *content, const char *user)
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

	sprintf(dir_filename, "%s%s.txt", DIR_LOG_EN, filename);
	//printf("dir_filename = %s\n", dir_filename);

	f_content = get_file_content(dir_filename);
	/* f_content is NULL */
	if (f_content == NULL) {
		return FAIL;
	/* no such file */
	} else if (strcmp(f_content, "NO_FILE") == 0) {
		if (delete_log_file(1) == FAIL) {
			return FAIL;
		}
		root_json = cJSON_CreateArray();
	/* f_content is empty */
	} else if (strcmp(f_content, "Empty") == 0) {
		root_json = cJSON_CreateArray();
	/* f_content exist */
	} else {
		root_json = cJSON_Parse(f_content);
		free(f_content);
		f_content = NULL;
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

	return SUCCESS;
}

/* record jap syslog */
int my_jap_syslog(const char *class, const char *content, const char *user)
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

	sprintf(dir_filename, "%s%s.txt", DIR_LOG_JAP, filename);
	//printf("dir_filename = %s\n", dir_filename);

	f_content = get_file_content(dir_filename);
	/* f_content is NULL */
	if (f_content == NULL) {
		return FAIL;
	/* no such file */
	} else if (strcmp(f_content, "NO_FILE") == 0) {
		if (delete_log_file(1) == FAIL) {
			return FAIL;
		}
		root_json = cJSON_CreateArray();
	/* f_content is empty */
	} else if (strcmp(f_content, "Empty") == 0) {
		root_json = cJSON_CreateArray();
	/* f_content exist */
	} else {
		root_json = cJSON_Parse(f_content);
		free(f_content);
		f_content = NULL;
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

	return SUCCESS;
}


/* delete log file: 删除旧的 log 文件, 只保留 web 系统配置文件中的 log_count 个 log 文件 */
int delete_log_file(int flag)
{
	char cmd[128] = {0};
	char *f_content = NULL;
	cJSON *root_json = NULL;
	cJSON *count = NULL;
	cJSON *sys_language = NULL;
	int log_count = 0;

	f_content = get_file_content(FILE_CFG);
	/* f_content is not NULL and f_content is not empty */
	if (f_content != NULL && strcmp(f_content, "Empty") != 0 && strcmp(f_content, "NO_FILE")!= 0 ) {
		//printf("f_content = %s\n", f_content);
		root_json = cJSON_Parse(f_content);
		free(f_content);
		f_content = NULL;
		if (root_json != NULL) {
			count = cJSON_GetObjectItem(root_json, "log_count");
			if (count != NULL) {
				//printf("count = %d\n", count->valuestring);
				if (flag) {//此时马上需要新增一个 log文件，所以需要多删除一个最旧的 log 文件
					log_count = count->valueint - 1;
				} else {
					log_count = count->valueint;
				}
				sprintf(cmd, "sh %s %d", SHELL_DELETELOG, log_count);
				system(cmd);
			}
			/* 更新系统语言 */
			sys_language = cJSON_GetObjectItem(root_json, "language");
			if (sys_language != NULL) {
				language = sys_language->valueint;
			}
		}
	}
	//printf("language = %d\n", language);

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

		return 1;
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

	return SUCCESS;
}

/* clear plugin DI DO config, and init config.json file*/
int clear_plugin_config(char *plugin_name)
{
	SOCKET_INFO *sock_cmd = NULL;
	char content[100] = {0};
	char config_path[100] = {0};
	char *config_content = NULL;
	char *buf = NULL;
	int write_ret = FAIL;
	int i = 0;
	cJSON *config_json = NULL;
	cJSON *func_json = NULL;
	cJSON *dio_json = NULL;
	cJSON *dio_value_json = NULL;
	cJSON *dio_level_json = NULL;

	sprintf(config_path, "%s%s/config.json", UPLOAD_WEB_PLUGINS, plugin_name);
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
			cJSON_Delete(config_json);
			config_json = NULL;

			return FAIL;
		}
		if (robot_type == 1) { // "1" 代表实体机器人
			sock_cmd = &socket_cmd;
		} else { // "0" 代表虚拟机器人
			sock_cmd = &socket_vir_cmd;
		}
		bzero(content, sizeof(content));
		/* clear 外设 DO */
		if (dio_json->valueint == 1) {
			sprintf(content, "SetPluginDO(%d,%d,%d)", dio_value_json->valueint, 0, dio_level_json->valueint);
			//printf("content = %s\n", content);
			socket_enquene(sock_cmd, 339, content, 1);
		/* clear 外设 DI */
		} else {
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

	return SUCCESS;
}

char *my_strlwr(char * str)   //定义一个my_strlwr函数,大写字符串转小写
{
	assert(str);         //str的非空性
	char *ret = str;       //定义一个ret保存最初的str
	while(*str != '\0')      //判断字符串是否结束
	{
		if((*str >= 'A')&&(*str <= 'Z'))//判断当前啊字符是否是                        大写字母
		{
			*str = *str +32;     //将其转化为小写字母
			str++;
		}
		else
			str++;
	}
	return ret;       //返回该字符串数组的首地址
}

int local_now_time(char *time_now)
{
	struct tm *my_local;
	time_t t;

	t = time(NULL);
	my_local = localtime(&t);
	bzero(time_now, 100);
	sprintf(time_now, "%d/%d/%d %d:%d:%d", (my_local->tm_year+1900), (my_local->tm_mon+1), my_local->tm_mday, my_local->tm_hour, my_local->tm_min, my_local->tm_sec);
	//printf("time_now = %s\n", time_now);

	return SUCCESS;
}

/**
	update user.config robot_type
*/
int update_userconfig_robottype()
{
	FILE *fp = NULL;
	char **array = NULL;
	int size = 0;
	char strline[LEN_100] = {0};
	char write_line[LEN_100] = {0};
	char write_content[LEN_100*100] = {0};
	char cmd[128] = {0};
	cJSON *type = NULL;
	cJSON *major_ver = NULL;
	cJSON *minor_ver = NULL;
	cJSON *content_json = NULL;
	char *file_content = NULL;
	int robot_type = 0;

	if ((fp = fopen(WEB_USER_CFG, "r")) == NULL) {
		perror("user.config : open file");

		return FAIL;
	}
	while (fgets(strline, LEN_100, fp) != NULL) {
		bzero(write_line, sizeof(char)*LEN_100);
		strcpy(write_line, strline);
		if (is_in(strline, "ROBOT_TYPE = ") == 1) {
			if (string_to_string_list(strline, " = ", &size, &array) == 0 || size != 2) {
				perror("string to string list");
				fclose(fp);
				string_list_free(&array, size);

				return FAIL;
			}
			//printf("strline = %s\n", strline);
			file_content = get_file_content(FILE_ROBOT_TYPE);
			/* NULL */
			if (file_content == NULL || strcmp(file_content, "NO_FILE") == 0 || strcmp(file_content, "Empty") == 0) {
				perror("get file content");

				return FAIL;
			}
			content_json = cJSON_Parse(file_content);
			free(file_content);
			file_content = NULL;

			type = cJSON_GetObjectItem(content_json, "type");
			major_ver = cJSON_GetObjectItem(content_json, "major_ver");
			minor_ver = cJSON_GetObjectItem(content_json, "minor_ver");
			if (type == NULL || major_ver == NULL || minor_ver == NULL) {
				perror("json");

				return FAIL;
			}
			/** 主版本号预留 10 个 (1~10)，次版本号预留 10 个 (0~9) */
			robot_type = (type->valueint - 1) * 100 + (major_ver->valueint - 1) * 10 + (minor_ver->valueint + 1);
			bzero(write_line, sizeof(char)*LEN_100);
			sprintf(write_line, "ROBOT_TYPE = %f\n", (float)robot_type);
			/* cjson delete */
			cJSON_Delete(content_json);
			content_json = NULL;

			string_list_free(&array, size);
		}
		bzero(strline, sizeof(char)*LEN_100);
		strcat(write_content, write_line);
	}
	fclose(fp);

	//printf("write_content len = %d\n", strlen(write_content));
	//printf("write_content = %s\n", write_content);

	return write_file(WEB_USER_CFG, write_content);
}

/* 更新生产数据数据库 */
int update_torquesys_pd_data()
{
	char sql[SQL_LEN] = { 0 };
	char del_sql[SQL_LEN] = { 0 };

    sprintf(del_sql, "delete from torquesys_pd_data;");
    if (change_info_sqlite3(DB_TORQUE_PDDATA, del_sql) == -1) {
		perror("delete all");

		return FAIL;
    }

	sprintf(sql, "insert into torquesys_pd_data values ('%s', %d, %d, %d, '%s', %d, %d, %d);", jiabao_torque_pd_data.left_wk_id, jiabao_torque_pd_data.left_product_count, jiabao_torque_pd_data.left_NG_count, jiabao_torque_pd_data.left_work_time, jiabao_torque_pd_data.right_wk_id, jiabao_torque_pd_data.right_product_count, jiabao_torque_pd_data.right_NG_count, jiabao_torque_pd_data.right_work_time);
	if (change_info_sqlite3(DB_TORQUE_PDDATA, sql) == -1) {
		perror("database");

		return FAIL;
	}

	return SUCCESS;
}

/**
  发送方：
  参数，buf为发送数组，len为数组长度
  返回值，校验值
*/
uint16_t TX_CheckSum(uint8_t *buf, uint8_t len)
{
	uint8_t i = 0;
	uint16_t ret = 0;

	for (i = 0; i < len; i++)
	{
		ret += buf[i];
	}
	ret = ~ret;

	return ret;
}

/**
	接收方：
	参数，buf为接收数组，len为数组长度（已包含发送的校验值）
	返回值, 如果是 0，说明数据正确
*/
uint16_t RX_CheckSum(uint8_t *buf, uint8_t len)
{
	uint8_t i = 0;
	uint16_t ret = 0;

	for (i = 0; i < len; i++)
	{
		// 校验和的高位需要左移 8 位
		if (i == (len - 4)) {
			ret += buf[i] << 8;
		} else {
			ret += buf[i];
		}
	}

	return ret+1;
}

/** 初始化，WebAPP 系统无操作时的超时时间 */
int init_sys_lifespan()
{
	char *cfg_content = NULL;
	cJSON *cfg_json = NULL;
	cJSON *lifespan_json = NULL;

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
	lifespan_json = cJSON_GetObjectItem(cfg_json, "lifespan");
	if (lifespan_json == NULL) {

		return FAIL;
	}
	web_cfg.lifespan = lifespan_json->valueint;
	//printf("web_cfg.lifespan = %d\n", web_cfg.lifespan);

	cJSON_Delete(cfg_json);
	cfg_json = NULL;

	return SUCCESS;
}
