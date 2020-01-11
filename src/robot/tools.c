
/********************************* Includes ***********************************/

#include 	"goahead.h"
#include 	"tools.h"
#include	"cJSON.h"

/*********************************** Code *************************************/

/*
pszInput:输入待分割字符串
pszDelimiters:分割标识符
uiAry_num:分割的份数
uiAry_size:每份的size
pszAry_out:分割的子串的输出参数
 */
int separate_string_to_array(char *pszInput, char *pszDelimiters , unsigned int Ary_num, unsigned int Ary_size, char *pszAry_out)
{
	//char *pszData = strdup(pszInput);
	char pszData[2048]={0};
	strcpy(pszData, pszInput);
	char *pszToken = NULL, *pszToken_saveptr;
	unsigned int Ary_cnt = 0;

	pszToken = strtok_r(pszData, pszDelimiters, &pszToken_saveptr);
	while( pszToken!=NULL)
	{
		//printf("pszToken=%s\n", pszToken);
		memcpy(pszAry_out + Ary_cnt*Ary_size, pszToken, Ary_size);
		if( ++Ary_cnt >= Ary_num)
			break;
		pszToken = strtok_r( NULL, pszDelimiters, &pszToken_saveptr);
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
	printf("write filename = %s\n", file_name);
	printf("write filecontent = %s\n", file_content);
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

/* oepn file and return complete file content */
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

	if ((dir=opendir(dir_path)) == NULL) {
		perror("Open dir error");

		return NULL;
	}
	root_json = cJSON_CreateArray();
	while ((ptr=readdir(dir)) != NULL) {
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
		cJSON_AddItemToArray(root_json, file_cont);
		cJSON_AddStringToObject(file_cont, "name", ptr->d_name);
		cJSON_AddStringToObject(file_cont, "pgvalue", f_content);
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

/* 实现字符串中指定字符串替换 */
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
