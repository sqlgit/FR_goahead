#ifndef tools_h
#define tools_h

#define local 0
#if local
	//#define SERVER_IP "127.0.0.1"
	#define SERVER_IP "192.168.152.129" //sql
	//#define SERVER_IP "192.168.172.128" //zjq
#else
	#define SERVER_IP "192.168.58.2"
#endif

#define setbit(x,y) x|=(1<<(y-1)) //将X的第Y位置1
#define clrbit(x,y) x&=~(1<<(y-1)) //将X的第Y位清0
#define SOCK_TIMEOUT 1 /* s */
#define SOCK_SEND_TIMEOUT 10 /* s */
#define HEART_MS_TIMEOUT 10000 /* ms */
#define CMD_PORT 8080
#define STATUS_PORT 8081
#define FILE_PORT 8082
#define MAX_BUF 1024
#define STATUS_BUF 4000
#define SUCCESS 1
#define FAIL 0
#define FILE_POINTS "/tmp/points/points.json"
#define DIR_LUA "/tmp/lua"
#define PATH_SEND_LUA "/fruser/"

/********************************* Defines ************************************/

typedef unsigned char BYTE;
#pragma pack(push, 1)
/** 运动控制器状态结构体 */
typedef struct _CTRL_STATE
{
	int        frame_len;               /**< 消息帧长度                     */
	double     runtime;                 /**< 控制器启动时间,断电清零        */
	double 	   jt_tgt_pos[6];           /**< 关节1-6目标位置                */
	double     jt_tgt_vel[6];           /**< 关节1-6目标速度                */
	double     jt_tgt_acc[6];			/**< 关节1-6目标加速度              */
	double     jt_tgt_cur[6];			/**< 关节1-6目标电流		        */
	double 	   jt_tgt_tor[6];           /**< 关节1-6目标扭矩                */
	double     jt_cur_pos[6];           /**< 关节1-6当前位置                */
	double     jt_cur_vel[6]; 			/**< 关节1-6当前速度                */
	double     jt_cur_cur[6];			/**< 关节1-6当前电流 		        */
	double     jt_ctl_cur[6];			/**< 关节1-6当前扭矩                */
	double     tl_cur_pos[6];			/**< 工具当前位置DKR		        */
	double	   tl_cur_vel[6];			/**< 工具当前速度DKR                */
	double     tl_jtforce[6];			/**< 工具合力DKR			        */
	double     tl_tgt_pos[6];  			/**< 工具目标位置DKR	            */
	double     tl_tgt_vel[6];			/**< 工具目标速度DKR                */
	uint8_t	   cl_dgt_input_h;          /**< 控制箱数字输入15-8             */
	uint8_t     cl_dgt_input_l;	        /**< 控制箱数字输入7-0              */
	uint8_t     tl_dgt_input_h;          /**< 工具数字输入15-8               */
	uint8_t     tl_dgt_input_l;          /**< 工具数字输入7-0                */
	uint8_t     tmp_dgt_input1;			/**< 空闲                           */
	uint8_t     tmp_dgt_input2;			/**< 空闲                           */
	uint8_t     tmp_dgt_input3;			/**< 空闲                           */
	uint8_t     tmp_dgt_input4;			/**< 空闲                           */
	uint16_t     analog_input[6];         /**< 模拟输入,四个控制箱,2个工具    */
	double     ctrl_time;               /**< 控制器实时线程执行时间         */
	double     test_data;               /**< 测试数据ur机器人软件使用       */
	uint8_t        robot_mode;              /**< 机器人模式                     */
	double     joint_mode[6];  			/**< 关节模式					    */
	double     safe_mode;               /**< 安全模式                       */
	int        collision_level;         /**< 碰撞等级,一级敏感,三级不敏感   */
	double     drag_enable;   			/**< 拖动使能                       */
	double     tl_acc[7];				/**< 工具加速度     			    */
	double     dr_com_err; 				/**< 与驱动器通信故障               */
	double	   dr_err;                  /**< 驱动器哪个轴故障               */
	double     out_sflimit_err;         /**< 超出软限位故障                 */
	double     collision_err;           /**< 碰撞故障                       */
	double     dr_err_code;  			/**< 驱动器故障代码 			    */
	double     err_reser;               /**< 故障预留                       */
	double     vel_ratio;               /**< 机器人运行速度比例             */
	double     linear_m_bmak;           /**< 线性动量基准 				    */
	uint8_t     flag_zero_set;           /**< 机器人零位设定完成标志         */
	double     test_data1;  			/**< 测试数据ur机器人软件使用       */
	double     cb_vol;					/**< 控制板电压   				    */
	double     cb_robot_vol;            /**< 控制板机器人电压               */
	double     cb_robot_cur;            /**< 控制板机器人电流               */
	double     jt_cur_vol[6];           /**< 关节当前电压                   */
	uint8_t	   cl_dgt_output_h;          /**< 控制箱数字输出15-8             */
	uint8_t     cl_dgt_output_l;	        /**< 控制箱数字输出7-0              */
	uint8_t     tl_dgt_output_h;			/**< 末端工具数字输出15-8     */
	uint8_t     tl_dgt_output_l;			/**< 末端工具数字输出7-0     */
	uint8_t     tmp_dgt_output3;			/**< 空闲                           */
	uint8_t     tmp_dgt_output4;			/**< 空闲                           */
	uint8_t     tmp_dgt_output5;			/**< 空闲                           */
	uint8_t     tmp_dgt_output6;			/**< 空闲                           */
	uint16_t   analog_output[6];         /**< 模拟输出,四个控制箱,2个工具    */
	uint8_t     program_state;           /**< 程序状态1:停止,2:运行,3：暂停,4：拖动                       */
	int        line_number;             /**< 自动运行程序执行到某一指令行号 */
	double     elbow_vel[5];            /**< 肘速度                         */
	double     tool_pos_att[6];         /**< 工具坐标中心相对于末端的位置及姿态 */
	uint8_t    strangePosFlag;          /**< 当前处于奇异位姿标志 */
	int        configStatus;            /**< 机器人关节配置状态  */
	uint8_t    tpd_state;               /**< TPD状态,低位->高位,bit0-允许轨迹记录，bit1-点数超限,bit2-文件保存完成,1为真,0为非 */
	uint8_t    aliveSlaveNumError;    /**< 活动从站数量错误，1：数量错误；0：正常  */
	uint8_t    slaveComError;      /**< 从站错误，0：正常；1：从站掉线；2：从站状态与设置值不一致；3：从站未配置；4：从站配置错误；5：从站初始化错误；6：从站邮箱通信初始化错误 */
	uint8_t    cmdPointError;      /**< 指令点关节位置与末端位姿不符错误，0：正常；1：直线指令错误；2：圆弧指令点错误；3：TPD指令工具与当前工具不符；4：TPD当前指令与下一条指令起始点偏差过大*/
}CTRL_STATE;
#pragma pack(pop)

/********************************* Function declaration ***********************/

double double_round(double dVal, int iPlaces);
int write_file(const char *file_name, const char *file_content);
char *get_file_content(const char *file_path);
char *get_complete_file_content(const char *file_path);
char *get_dir_content(const char *dir_path);
char *strrpc(char *str, const char *oldstr, const char *newstr);
void delay_ms(const int timeout);
int socket_create();
int socket_connect(int sockfd, const char *server_ip, int server_port, const int s);
int socket_timeout(int sockfd, const int s);
int socket_send(int clientSocket, const int no, const char *content, char *recvbuf);
void *socket_cmd_thread(void * arg);
void *socket_file_thread(void * arg);
void *socket_status_thread(void * arg);

#endif
