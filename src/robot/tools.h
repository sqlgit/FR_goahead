#ifndef tools_h
#define tools_h

/********************************* Defines ************************************/

#define local 1
#define virtual_robot 0
#define test_package 0
#define SUCCESS 1
#define FAIL 0
#define ROBOT_CFG "/root/robot/user.config"
#define FILE_POINTS "/root/webserver/points/points.json"
#define SYSTEM_CFG "/root/webserver/cfg/system.json"
#define FILE_CDSYSTEM "/root/webserver/cdsystem/coordinate_system.json"
#define DIR_USER "/root/webserver/user/"
#define DIR_LOG "/root/webserver/log/"
#define DIR_TEMPLATE "/root/webserver/template/"
#define DIR_CDSYSTEM "/root/webserver/cdsystem/"
#define DIR_POINTS "/root/webserver/points/"
#define DIR_SHELL "/root/webserver/shell/"
#define DIR_CFG "/root/webserver/cfg/"
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
char *get_dir_filename(const char *dir_path);
char *strrpc(char *str, const char *oldstr, const char *newstr);
int is_in(char *s, char *c);
void delay_ms(const int timeout);
double double_round(double dVal, int iPlaces);
int BytesToString(BYTE *pSrc, char *pDst, int nSrcLength);
int StringToBytes(char *pSrc, BYTE *pDst, int nSrcLength);
int my_syslog(const char *class, const char *content, const char *user);
int delete_log_file();
void *create_dir(const char *dir_path);

#endif
