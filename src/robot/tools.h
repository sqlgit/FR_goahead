#ifndef tools_h
#define tools_h

/********************************* Defines ************************************/

#define local 0
#define virtual_robot 1
#define test_package 0
#define SUCCESS 1
#define FAIL 0
#define ROBOT_CFG "/root/robot/user.config"
#define README_WEB_NOW "/root/README/README_WEB.txt"
#define README_WEB_UP "/tmp/webapp/README_WEB.txt"
#define README_CTL_NOW "/root/README/README_CTL.txt"
#define README_CTL_UP "/tmp/control/README_CTL.txt"
#define UPGRADE_WEBAPP "/tmp/webapp.tar.gz"
#define UPGRADE_WEB "/tmp/webapp/web.tar.gz"
#define UPGRADE_CONTROL "/tmp/control.tar.gz"
#define UPGRADE_FR_CONTROL "/tmp/control/fr_control.tar.gz"
#define SHELL_DELETELOG "/root/web/shell/delete_file.sh"
#define FILE_POINTS "/root/web/points/points.json"
#define FILE_CFG "/root/web/cfg/system.json"
#define FILE_CDSYSTEM "/root/web/cdsystem/coordinate_system.json"
#define FILE_ET_CDSYSTEM "/root/web/cdsystem/et_coordinate_system.json"
#define FILE_GRIPPER "/root/web/gripper/gripper.json"
#define FILE_ACCOUNT "/root/web/account/account.json"
#define FILE_AUTH "/root/web/auth.txt"
#define DIR_USER "/root/web/user/"
#define DIR_LOG "/root/web/log/"
#define DIR_TEMPLATE "/root/web/template/"
#define DIR_CDSYSTEM "/root/web/cdsystem/"
#define DIR_POINTS "/root/web/points/"
#define DIR_SHELL "/root/web/shell/"
#define DIR_CFG "/root/web/cfg/"
#define DIR_FRUSER "/fruser/"
#define setbit(x,y) x|=(1<<(y-1)) //将X的第Y位置1
#define clrbit(x,y) x&=~(1<<(y-1)) //将X的第Y位清0
#define MD5_READ_DATA_SIZE	1024
#define MD5_SIZE		16
#define MD5_STR_LEN		(MD5_SIZE * 2)
#define LINE_LEN	1024

typedef unsigned char BYTE;

/* ACCOUNT 相关信息结构体 */
typedef struct _ACCOUNT_INFO
{
	char auth[10];
	char username[100];
} ACCOUNT_INFO;
/********************************* Function declaration ***********************/

int separate_string_to_array(char *pszInput, char *pszDelimiters , unsigned int Ary_num, unsigned int Ary_size, char *pszAry_out);
int get_n_len(const int n);
int write_file(const char *file_name, const char *file_content);
char *get_file_content(const char *file_path);
char *get_complete_file_content(const char *file_path);
char *get_dir_content(const char *dir_path);
char *get_dir_filename(const char *dir_path);
char *get_dir_filename_txt(const char *dir_path);
char *strrpc(char *str, const char *oldstr, const char *newstr);
int is_in(char *s, char *c);
void delay_ms(const int timeout);
double double_round(double dVal, int iPlaces);
void uint8_to_array(int n1, int n2, int *array);
void uint16_to_array(int n, int *array);
int BytesToString(BYTE *pSrc, char *pDst, int nSrcLength);
int StringToBytes(char *pSrc, BYTE *pDst, int nSrcLength);
int my_syslog(const char *class, const char *content, const char *user);
int delete_log_file(int flag);
void *create_dir(const char *dir_path);
int authority_management(const char *cmd_type);
void delete_timer();

#endif
