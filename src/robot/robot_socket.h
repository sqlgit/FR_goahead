#ifndef robot_socket_h
#define robot_socket_h

#include 	"robot_quene.h"
#include    "statefb_quene.h"
/********************************* Defines ************************************/

#if local
	//#define SERVER_IP "127.0.0.1"
	#define SERVER_IP "192.168.152.129" //sql
	//#define SERVER_IP "192.168.172.128" //zjq
#else
	#define SERVER_IP "192.168.58.2"
#endif
#define CMD_PORT 8060
#define STATUS_PORT 8061
#define FILE_PORT 8062
#define STATE_FEEDBACK_PORT 8063
#define VIR_CMD_PORT 8070
#define VIR_STATUS_PORT 8071
#define VIR_FILE_PORT 8072
#define SOCK_SELECT_TIMEOUT 1 /* 秒 */
#define MAX_BUF 1024
#define MAX_MSGHEAD 10000
#define BUFFSIZE 1300000*2
#define STATEFB_SIZE 12000
#define STATE_FB_ID 3
#define MAXGRIPPER 8

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
	double	   dr_err;                  /**< 驱动器哪个轴故障,驱动器故障              */
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
	uint8_t    aliveSlaveNumError;    /**< 活动从站数量错误，1：数量错误；0：正常  */
	uint8_t    slaveComError;      /**< 从站错误，0：正常；1：从站掉线；2：从站状态与设置值不一致；3：从站未配置；4：从站配置错误；5：从站初始化错误；6：从站邮箱通信初始化错误 */
	uint8_t    cmdPointError;      /**< 指令点关节位置与末端位姿不符错误，0：正常；1：直线指令错误；2：圆弧指令点错误；3：TPD指令工具与当前工具不符；4：TPD当前指令与下一条指令起始点偏差过大*/
	uint8_t    ioError;                 /** IO错误 */
	uint8_t    gripperError;            /** 夹爪错误 */
	uint8_t    fileError;               /** 文件错误 */
	uint8_t    paraError;               /** 参数错误 */
	uint8_t    alarm;                   /** 警告 */
	int        toolNum;                 /** 工具号 */
	uint8_t    gripperFaultId;          /** 错误夹爪号 */
	uint8_t    gripperFaultNum;         /** 夹爪错误编号 */
	uint16_t   gripperConfigStatus;     /** 夹爪配置状态 */
	uint16_t   gripperActStatus;		/** 夹爪激活状态 */
	int 	   aliveSlaveNumFeedback;   /** 活动从站数量反馈 */
	uint8_t	   ctrl_query_state;		/** 控制器查询状态 0-未查询，1-查询中 */
} CTRL_STATE;
#pragma pack(pop)

#pragma pack(push, 1)
/** GRIPPERS_CONFIG_INFO 结构体 */
typedef struct _GRIPPERS_CONFIG_INFO
{
	uint8_t id_company[MAXGRIPPER];
	uint8_t id_device[MAXGRIPPER];
	uint8_t id_softversion [MAXGRIPPER];
	uint8_t id_bus[MAXGRIPPER];
} GRIPPERS_CONFIG_INFO;
#pragma pack(pop)

/** 状态查询结构体 */
typedef struct _STATE_FEEDBACK
{
	int id[10];			// state feedback id
	int icount;			// state feedback icount
} STATE_FEEDBACK;

/* socket 相关信息结构体 */
typedef struct _SOCKET_INFO
{
	int fd;
	char server_ip[20];
	int server_port;
	int select_timeout; // socket select timeout
	uint8_t connect_status; // socket 连接状态
	int msghead; // 当前有记录的消息头
	LinkQuene quene; //非即时指令队列
	LinkQuene im_quene; //即时指令队列
	LinkQuene ret_quene; //指令执行结果反馈队列
	pthread_t t_socket_send;
	pthread_t t_socket_recv;
	pthread_mutex_t mut; // socket 锁
	pthread_mutex_t mute;//非即时指令队列锁
	pthread_mutex_t im_mute;//即时指令队列锁
	pthread_mutex_t ret_mute;//指令执行结果反馈队列锁
} SOCKET_INFO;

/********************************* Function declaration ***********************/

void *socket_thread(void *arg);
void *socket_status_thread(void *arg);
void *socket_state_feedback_thread(void *arg);

#endif
