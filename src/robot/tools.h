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

/** frontend */
#define FILE_ODM_CFG 				"/var/www/frontend/file/odm/cfg.txt"
#define UPLOAD_TOOL_MODEL 			"/var/www/frontend/data/toolmodel/"
#define UPLOAD_WEB_PLUGINS 			"/var/www/frontend/plugins/web-plugins/"
#define UPLOAD_WEB_ODM 				"/var/www/frontend/file/odm.tar.gz"
#define LOAD_TOOL_MODEL 			"./data/toolmodel/"

/** web */
#define README_WEB_NOW 				"/usr/local/etc/web/README_WEB.txt"
#define FILE_AUTH 					"/usr/local/etc/web/auth.txt"

#define DIR_FILE 					"/usr/local/etc/web/file/"
#define README_FILE 				"/usr/local/etc/web/file/README_FILE.txt"
#define DIR_BLOCK 					"/usr/local/etc/web/file/block/"
#define DIR_USER 					"/usr/local/etc/web/file/user/"
#define DIR_TEMPLATE 				"/usr/local/etc/web/file/template/"
#define DIR_CDSYSTEM 				"/usr/local/etc/web/file/cdsystem/"
#define DB_CDSYSTEM 				"/usr/local/etc/web/file/cdsystem/coordinate_system.db"
#define DB_ET_CDSYSTEM 				"/usr/local/etc/web/file/cdsystem/et_coordinate_system.db"
#define DB_WOBJ_CDSYSTEM 			"/usr/local/etc/web/file/cdsystem/wobj_coordinate_system.db"
#define DB_EXAXIS_CDSYSTEM 			"/usr/local/etc/web/file/cdsystem/exaxis_coordinate_system.db"
#define DIR_STATEFB 				"/usr/local/etc/web/file/statefb/"
#define FILE_STATEFB 				"/usr/local/etc/web/file/statefb/statefb.txt"
#define FILE_STATEFB10 				"/usr/local/etc/web/file/statefb/statefb10.txt"
#define DIR_POINTS 					"/usr/local/etc/web/file/points/"
#define FILE_POINTS_CFG 			"/usr/local/etc/web/file/points/point_cfg.txt"
#define FILE_DH_POINT 				"/usr/local/etc/web/file/points/DH_point.txt"
#define DB_POINTS 					"/usr/local/etc/web/file/points/web_point.db"
#define DIR_TORQUESYS 				"/usr/local/etc/web/file/torquesys/"
#define FILE_TORQUE_PAGEFLAG 		"/usr/local/etc/web/file/torquesys/torquesys_pageflag.txt"
#define FILE_TORQUE_DIO 			"/usr/local/etc/web/file/torquesys/torquesys_DIO.txt"
#define DB_TORQUE_CFG 				"/usr/local/etc/web/file/torquesys/torquesys_cfg.db"
#define DB_TORQUE_POINTS 			"/usr/local/etc/web/file/torquesys/torquesys_points.db"
#define DB_TORQUE_CUSTOM 			"/usr/local/etc/web/file/torquesys/torquesys_custom.db"
#define DB_TORQUE_PDDATA 			"/usr/local/etc/web/file/torquesys/torquesys_pd_data.db"
#define DIR_CUSTOMER 				"/usr/local/etc/web/file/customer/"
#define FILE_STATUS_SHOWFLAG 		"/usr/local/etc/web/file/customer/web_status_showflag.txt"
#define DIR_SYSVAR 					"/usr/local/etc/web/file/sysvar/"
#define DB_SYSVAR 					"/usr/local/etc/web/file/sysvar/sysvar.db"

#define DIR_CFG 					"/usr/local/etc/web/cfg/"
#define FILE_CFG 					"/usr/local/etc/web/cfg/system.txt"
#define FILE_PI_CFG 				"/usr/local/etc/web/cfg/PI.txt"
#define DB_ACCOUNT 					"/usr/local/etc/web/cfg/account.db"

#define DIR_SHELL 					"/usr/local/etc/web/shell/"
#define SHELL_DELETELOG 			"/usr/local/etc/web/shell/delete_file.sh"

#define DIR_ROBOT_CFG 				"/usr/local/etc/web/controller/"
#define WEB_USER_CFG 				"/usr/local/etc/web/controller/user.config"
#define WEB_EX_DEVICE_CFG 			"/usr/local/etc/web/controller/ex_device.config"
#define WEB_EXAXIS_CFG 				"/usr/local/etc/web/controller/exaxis.config"

#define DIR_FACTORY 				"/usr/local/etc/web/file_factory/"
#define DIR_FACTORY_RESET_TORQUESYS "/usr/local/etc/web/file_factory/torquesys/"
#define DIR_FACTORY_RESET_SYSVAR 	"/usr/local/etc/web/file_factory/sysvar/"

/** TODO */
#define DIR_LOG 					"/usr/local/etc/web/log/zh/"
#define DIR_LOG_EN 					"/usr/local/etc/web/log/en/"
#define DIR_LOG_JAP 				"/usr/local/etc/web/log/ja/"

/** controller */
#define USER_CFG 					"/usr/local/etc/controller/user.config"
#define EX_DEVICE_CFG 				"/usr/local/etc/controller/ex_device.config"
#define EXAXIS_CFG 					"/usr/local/etc/controller/exaxis.config"
#define README_CTL_NOW 				"/usr/local/etc/controller/README_CTL.txt"
#define FILE_VISION 				"/usr/local/etc/controller/vision_pkg_des.txt"
#define FILE_RBLOG 					"/usr/local/etc/controller/rblog.tar.gz"
#define FILE_RTDE_CFG 				"/usr/local/etc/controller/RTDEConfig.lua"

#define DIR_FRUSER 					"/usr/local/etc/controller/lua/"
#define FILE_GENGKU_HOMELUA 		"/usr/local/etc/controller/lua/gengku_home.lua"

/** robot */
#define ROBOT_CFG 					"/usr/local/etc/robot/robot.config"
#define FILE_SN 					"/usr/local/etc/robot/SN.txt"
#define FILE_ROBOT_TYPE 			"/usr/local/etc/robot/RobotType.txt"

/** TODO user data */
//#define FILE_USERDATA 				"/usr/local/etc/web/user_data.tar.gz"
#define FILE_USERDATA 				"/usr/local/etc/fr_user_data.tar.gz"

/** tmp */
#define UPGRADE_USER_CFG 			"/tmp/web/controller/user.config"
#define UPGRADE_EXAXIS_CFG 			"/tmp/web/controller/exaxis.config"
#define UPGRADE_EX_DEVICE_CFG 		"/tmp/web/controller/ex_device.config"
#define UPGRADE_ROBOT_CFG 			"/tmp/web/robot/robot.config"
#define UPGRADE_README_FILE 		"/tmp/web/file/README_FILE.txt"
#define UPGRADE_WEBTARCP 			"/tmp/web/shell/web_tar_cp.sh"
#define UPGRADE_CRLUPGRADE 			"/tmp/web/shell/fr_control_upgrade.sh"
#define README_WEB_UP 				"/tmp/software/README_WEB.txt"
#define README_CTL_UP 				"/tmp/software/README_CTL.txt"
#define UPGRADE_SOFTWARE 			"/tmp/software.tar.gz"
#define UPGRADE_WEB 				"/tmp/software/web.tar.gz"
#define UPGRADE_FR_CONTROL 			"/tmp/software/fr_control.tar.gz"
#define UPGRADE_FILE_USERDATA 		"/tmp/fr_user_data.tar.gz"
#define UPLOAD_DB_TORQUE_CFG 		"/tmp/torquesys_cfg.db"
#define UPLOAD_DB_TORQUE_POINTS_CFG "/tmp/torquesys_points_cfg.db"
#define UPLOAD_DB_TORQUE_POINTS 	"/tmp/torquesys_points.db"

#define setbit(x,y) 				x|=(1<<(y-1)) //将X的第Y位置1
#define clrbit(x,y) 				x&=~(1<<(y-1)) //将X的第Y位清0
#define MD5_SIZE					16
#define MD5_STR_LEN					(MD5_SIZE * 2)
#define MD5_READ_DATA_SIZE			1024
#define LEN_100						100
#define LINE_LEN					1024
#define SQL_LEN						1024
#define LOG_LEN						1024
#define FILENAME_SIZE				1024
#define GENGKU_LUASIZE				10000

#define FILENAME_UP_SUC 			"upgrade_success.txt"
#define ODM_PASSWORD				"ODM"
#define RTS_PASSWORD				"frrts2021"
#define POINT_HOME					"pHome"

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
