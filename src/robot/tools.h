#ifndef tools_h
#define tools_h

/********************************* Defines ************************************/

#define local 0
#define virtual_robot 0
#define recover_mode 1 // 0 为进入 recover mode, 1 为 normal mode
#define print_mode 0 // 1 为进入 print mode
#define test_package 0
#define FILE_VERSION "V1.0.0"

#define SUCCESS 1
#define FAIL 0
#define PWD_FAIL -1
#define USER_CFG "/root/robot/user.config"
#define EX_DEVICE_CFG "/root/robot/ex_device.config"
#define EXAXIS_CFG "/root/robot/exaxis.config"
#define ROBOT_CFG "/root/robot/robot.config"
#define WEB_USER_CFG "/root/web/file/robotcfg/user.config"
#define WEB_EX_DEVICE_CFG "/root/web/file/robotcfg/ex_device.config"
#define WEB_EXAXIS_CFG "/root/web/file/robotcfg/exaxis.config"
#define UPGRADE_USER_CFG "/tmp/web/file/robotcfg/user.config"
#define UPGRADE_EXAXIS_CFG "/tmp/web/file/robotcfg/exaxis.config"
#define UPGRADE_EX_DEVICE_CFG "/tmp/web/file/robotcfg/ex_device.config"
#define UPGRADE_ROBOT_CFG "/tmp/web/file/robotcfg/robot.config"
#define UPGRADE_WEBTARCP "/tmp/web/webserver/shell/web_tar_cp.sh"
#define UPGRADE_CRLUPGRADE "/tmp/web/webserver/shell/fr_control_upgrade.sh"
#define README_WEB_NOW "/root/README/README_WEB.txt"
#define README_WEB_UP "/tmp/software/README_WEB.txt"
#define README_CTL_NOW "/root/README/README_CTL.txt"
#define README_CTL_UP "/tmp/software/README_CTL.txt"
#define README_FILE "/root/web/file/README_FILE.txt"
#define UPGRADE_README_FILE "/tmp/web/file/README_FILE.txt"
#define UPGRADE_SOFTWARE "/tmp/software.tar.gz"
#define UPGRADE_WEB "/tmp/software/web.tar.gz"
#define UPGRADE_FR_CONTROL "/tmp/software/fr_control.tar.gz"
#define UPGRADE_FILE_USERDATA "/tmp/fr_user_data.tar.gz"

//#define SHELL_WEBUPGRADE "/root/web/webserver/shell/web_upgrade.sh"
#define SHELL_DELETELOG "/root/web/webserver/shell/delete_file.sh"
#define SHELL_WEBTARCP "/root/web/webserver/shell/web_tar_cp.sh"
#define SHELL_CRLUPGRADE "/root/web/webserver/shell/fr_control_upgrade.sh"
#define SHELL_RECOVER_WEBTARCP "/root/web_recover/webserver/shell/web_tar_cp.sh"
#define SHELL_RECOVER_CRLUPGRADE "/root/web_recover/webserver/shell/fr_control_upgrade.sh"

#define FILE_GENGKU_HOMELUA "/fruser/gengku_home.lua"
#define FILE_STATEFB "/root/web/file/statefb/statefb.txt"
#define FILE_STATEFB10 "/root/web/file/statefb/statefb10.txt"
#define FILE_CFG "/root/web/file/cfg/system.txt"
#define FILE_POINTS_CFG "/root/web/file/points/point_cfg.txt"
#define FILE_PI_CFG "/root/web/file/cfg/PI.txt"
#define FILE_AUTH "/root/web/webserver/auth.txt"
#define FILE_VISION "/root/robot/vision_pkg_des.txt"
#define FILE_RBLOG "/root/robot/rblog.tar.gz"
#define FILE_USERDATA "/root/fr_user_data.tar.gz"
#define FILE_SN "/root/web/file/SN/SN.txt"
#define FILE_DH_POINT "/root/web/file/points/DH_point.txt"
#define FILE_ODM_CFG "/root/web/frontend/file/odm/cfg.txt"
#define FILE_ROBOT_TYPE "/root/RobotType/RobotType.txt"
//#define FILE_TORQUE_POINTS "/root/web/file/torquesys/torquesys_points.txt"
#define FILE_TORQUE_PAGEFLAG "/root/web/file/torquesys/torquesys_pageflag.txt"
#define FILE_TORQUE_DIO "/root/web/file/torquesys/torquesys_DIO.txt"
#define FILE_STATUS_SHOWFLAG "/root/web/file/customer/web_status_showflag.txt"
#define FILE_RTDE_CFG "/root/robot/RTDEConfig.lua"
#define FILENAME_UP_SUC "upgrade_success.txt"

#define DIR_SHELL "/root/web/webserver/shell/"
#define DIR_FACTORY "/root/web/webserver/file_factory/"
#define DIR_LOG "/root/web/log/"
#define DIR_LOG_EN "/root/web/log_en/"
#define DIR_LOG_JAP "/root/web/log_jap/"
#define DIR_FILE "/root/web/file/"
#define DIR_BLOCK "/root/web/file/block/"
#define DIR_CUSTOMER "/root/web/file/customer/"
#define DIR_USER "/root/web/file/user/"
#define DIR_TEMPLATE "/root/web/file/template/"
#define DIR_CDSYSTEM "/root/web/file/cdsystem/"
#define DIR_POINTS "/root/web/file/points/"
#define DIR_CFG "/root/web/file/cfg/"
//#define DIR_WELD "/root/web/file/weld/"
#define DIR_ROBOT_CFG "/root/web/file/robotcfg/"
#define DIR_STATEFB "/root/web/file/statefb/"
#define DIR_FRUSER "/fruser/"
#define DIR_ROBOT_TYPE "/root/RobotType/"
#define DIR_TORQUESYS "/root/web/file/torquesys/"
#define DIR_FACTORY_RESET_TORQUESYS "/root/web/webserver/file_factory/torquesys/"
#define DIR_SYSVAR "/root/web/file/sysvar/"
#define DIR_FACTORY_RESET_SYSVAR "/root/web/webserver/file_factory/sysvar/"
#define DIR_FACTORY_RESET "/root/web/webserver/file_factory/"

#define DB_POINTS "/root/web/file/points/web_point.db"
#define DB_CDSYSTEM "/root/web/file/cdsystem/coordinate_system.db"
#define DB_ET_CDSYSTEM "/root/web/file/cdsystem/et_coordinate_system.db"
#define DB_WOBJ_CDSYSTEM "/root/web/file/cdsystem/wobj_coordinate_system.db"
#define DB_EXAXIS_CDSYSTEM "/root/web/file/cdsystem/exaxis_coordinate_system.db"
#define DB_ACCOUNT "/usr/local/etc/web/cfg/account/account.db"
#define DB_SYSVAR "/root/web/file/sysvar/sysvar.db"
#define DB_TORQUE_CFG "/root/web/file/torquesys/torquesys_cfg.db"
#define DB_TORQUE_POINTS "/root/web/file/torquesys/torquesys_points.db"
#define DB_TORQUE_CUSTOM "/root/web/file/torquesys/torquesys_custom.db"
#define DB_TORQUE_PDDATA "/root/web/file/torquesys/torquesys_pd_data.db"

#define UPLOAD_DB_TORQUE_CFG "/tmp/torquesys_cfg.db"
#define UPLOAD_DB_TORQUE_POINTS_CFG "/tmp/torquesys_points_cfg.db"
#define UPLOAD_DB_TORQUE_POINTS "/tmp/torquesys_points.db"
#define UPLOAD_TOOL_MODEL "/root/web/frontend/data/toolmodel/"
#define UPLOAD_WEB_PLUGINS "/root/web/frontend/plugins/web-plugins/"
#define UPLOAD_WEB_ODM "/root/web/frontend/file/odm.tar.gz"
#define LOAD_TOOL_MODEL "./data/toolmodel/"
#define setbit(x,y) x|=(1<<(y-1)) //将X的第Y位置1
#define clrbit(x,y) x&=~(1<<(y-1)) //将X的第Y位清0
#define MD5_READ_DATA_SIZE	1024
#define MD5_SIZE		16
#define MD5_STR_LEN		(MD5_SIZE * 2)
#define LINE_LEN	1024
#define SQL_LEN		1024
#define LOG_LEN		1024
#define LEN_100	100
#define FILENAME_SIZE	1024
#define ODM_PASSWORD	"ODM"
#define RTS_PASSWORD	"frrts2021"
#define POINT_HOME	"pHome"
#define GENGKU_LUASIZE	10000

typedef unsigned char BYTE;

/* ACCOUNT 相关信息结构体 */
typedef struct _ACCOUNT_INFO
{
	char auth[10];
	char username[100];
} ACCOUNT_INFO;

/* WebAPP 系统配置结构体 */
typedef struct _WEBAPP_SYSCFG
{
	int lifespan;				/** Webapp 无操作时的登出时间，可配置 5min ~ 24H */
} WEBAPP_SYSCFG;

/********************************* Function declaration ***********************/

int string_to_string_list(char *src_str, char *delimiter, int *delimiter_count, char ***str_list);
void string_list_free(char ***str_list, int list_size);
//int separate_string_to_array(char *pData, char *pDelimiter , unsigned int Ary_num, unsigned int Ary_size, char ***pAry_out);
int get_n_len(const int n);
int write_file(const char *file_name, const char *file_content);
int write_file_append(const char *file_name, const char *file_content);
char *format_str(const char *source_str);
char *get_file_content(const char *file_path);
char *get_complete_file_content(const char *file_path);
char *get_dir_content(const char *dir_path);
char *get_dir_filename(const char *dir_path);
char *get_dir_filename_strhead(const char *dir_path, const char *str);
int check_dir_filename(const char *dir_path, const char *filename);
char *get_dir_filename_txt(const char *dir_path);
char *strrpc(char *str, const char *oldstr, const char *newstr);
int is_in(char *s, char *c);
int is_in_srclen(const char *s, const char *c);
void delay_ms(const int timeout);
double double_round(double dVal, int iPlaces);
void uint8_to_array(int n1, int n2, int *array);
void uint16_to_array(int n, int *array);
int BytesToString(BYTE *pSrc, char *pDst, int nSrcLength);
int StringToBytes(char *pSrc, BYTE *pDst, int nSrcLength);
int my_syslog(const char *class, const char *content, const char *user);
int my_en_syslog(const char *class, const char *content, const char *user);
int my_jap_syslog(const char *class, const char *content, const char *user);
int delete_log_file(int flag);
void *create_dir(const char *dir_path);
int authority_management(const char *cmd_type);
void delete_timer();
int clear_plugin_config(char *plugin_name);
char *my_strlwr(char * str);
int local_now_time(char *time_now);
int update_userconfig_robottype();
int update_torquesys_pd_data();
uint16_t TX_CheckSum(uint8_t *buf, uint8_t len);
uint16_t RX_CheckSum(uint8_t *buf, uint8_t len);
int init_sys_lifespan();
int get_file_linenum(char *filename);

#endif
