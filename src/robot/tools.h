#ifndef tools_h
#define tools_h

/********************************* Defines ************************************/

#define setbit(x,y) x|=(1<<(y-1)) //将X的第Y位置1
#define clrbit(x,y) x&=~(1<<(y-1)) //将X的第Y位清0
#define MAX_BUF 1024
#define SUCCESS 1
#define FAIL 0
#define FILE_POINTS "/tmp/points/points.json"
#define DIR_LUA "/tmp/lua"
#define PATH_SEND_LUA "/fruser/"

typedef unsigned char BYTE;

/********************************* Function declaration ***********************/

int write_file(const char *file_name, const char *file_content);
char *get_file_content(const char *file_path);
char *get_complete_file_content(const char *file_path);
char *get_dir_content(const char *dir_path);
char *strrpc(char *str, const char *oldstr, const char *newstr);
void delay_ms(const int timeout);
double double_round(double dVal, int iPlaces);
int BytesToString(BYTE *pSrc, char *pDst, int nSrcLength);
int StringToBytes(char *pSrc, BYTE *pDst, int nSrcLength);

#endif
