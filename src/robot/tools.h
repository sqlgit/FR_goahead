#ifndef tools_h
#define tools_h

/********************************* Defines ************************************/

#define local 1
#define virtual_robot 0
#define test_package 0
#define SUCCESS 1
#define FAIL 0
#define FILE_POINTS "/root/webserver/points/points.json"
#define DIR_USER "/root/webserver/user/"
#define DIR_TEMPLATE "/root/webserver/template/"
#define DIR_FRUSER "/fruser/"
#define setbit(x,y) x|=(1<<(y-1)) //将X的第Y位置1
#define clrbit(x,y) x&=~(1<<(y-1)) //将X的第Y位清0

typedef unsigned char BYTE;

/********************************* Function declaration ***********************/

int separate_string_to_array(char *pszInput, char *pszDelimiters , unsigned int Ary_num, unsigned int Ary_size, char *pszAry_out);
int get_n_len(const int n);
int write_file(const char *file_name, const char *file_content);
char *get_file_content(const char *file_path);
char *get_complete_file_content(const char *file_path);
char *get_dir_content(const char *dir_path);
char *strrpc(char *str, const char *oldstr, const char *newstr);
int is_in(char *s, char *c);
void delay_ms(const int timeout);
double double_round(double dVal, int iPlaces);
int BytesToString(BYTE *pSrc, char *pDst, int nSrcLength);
int StringToBytes(char *pSrc, BYTE *pDst, int nSrcLength);

#endif
