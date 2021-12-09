#ifndef robot_socket_h
#define robot_socket_h

#include 	"robot_quene.h"
#include    "statefb_quene.h"
#include    "tools.h"
/********************************* Defines ************************************/

#define CMD_PORT 8060
#define STATUS_PORT 8061
#define FILE_PORT 8062
#define STATE_FEEDBACK_PORT 8063
#define TORQUE_PORT 8064
#define PI_STATUS_PORT 8899
#define PI_CMD_PORT 8898
#define VIR_CMD_PORT 8070
#define VIR_STATUS_PORT 8071
#define VIR_FILE_PORT 8072
#define SOCK_SELECT_TIMEOUT 1 /* 秒 */
#define MAX_BUF 1024
#define MAX_MSGHEAD 10000
//#define BUFFSIZE 1300000*2
#define BUFFSIZE 8192
#define STATE_BUFFSIZE 16384
#define STATE_SIZE 8192
#define PI_STATUS_BUFFSIZE 1024
#define PI_STATUS_SIZE 32
#define STATEFB_SIZE 25000 /** 4(sizeof(float))*100(row)*20(column)*3 + 1000(包头包尾分隔符等) */
#define STATEFB_BUFSIZE 2400000 /** 4(sizeof(float))*100(row)*20(column)*3*100(num) */
//#define STATEFB_WRITESIZE 10*8000*2 /** 10(num)*4(sizeof(float))*100(row)*20(column)*2 */
//#define STATEFB_FILESIZE 100*8000*2 /** 100(num)*4(sizeof(float))*100(row)*20(column)*2 */
#define STATEFB_PERPKG_NUM 100
#define STATEFB_ID_MAXNUM 20
#define TORQUE_SYS_SIZE 1024
#define STATEFB_MAX 100 /** state feedback quene, node max number */
#define MAXGRIPPER 8
#define MAXSLAVES 8
#define TM_SYS_VAR_NUM 20
#define SOCKET_CONNECT_CLIENT_NUM_MAX 8
#define REG_VAR_NB_MAX_NUM 20	/** 数值型变量个数 */
#define REG_VAR_STR_MAX_NUM 10	/** 字符型变量个数 */
#define MAXPIOIN	1
#define MAXADCIN 	2
#define MAXAUXADCIN	1

#pragma pack(push, 1)
/** 外部轴状态结构体 */
typedef struct _REG_VAR
{
	uint8_t	num_full;			//数值型变量监控个数已满, 1:代表已满 0:代表未满
	uint8_t	str_full;			//字符型变量监控个数已满, 1:代表已满 0:代表未满
	uint8_t	num_cnt;			//数值型变量使用个数
	uint8_t	str_cnt;			//字符型变量使用个数
	double	num[REG_VAR_NB_MAX_NUM];			//数值型变量值
	char	str[REG_VAR_STR_MAX_NUM][100];		//字符型变量值
	char	num_name[REG_VAR_NB_MAX_NUM][20];	//数值型变量名
	char	str_name[REG_VAR_STR_MAX_NUM][20];	//字符型变量名
}REG_VAR;
#pragma pack(pop)

#pragma pack(push, 1)
/** 外部轴状态结构体 */
typedef struct _EXTERNALAXIS_STATU
{
	double exAxisPos;			//外部轴位置
	double exAxisSpeedBack;		//外部轴速度
	int exAxisErrorCode;		//外部轴故障码
	unsigned char exAxisRDY;	//伺服准备好
	unsigned char exAxisINPOS;	//伺服到位
	unsigned char exAxisALM;	//伺服报警
	unsigned char exAxisFLERR;	//跟随误差
	unsigned char exAxisNLMT;	//到负限位
	unsigned char exAxisPLMT;	//到正限位
	unsigned char exAxisAbsOFLN;//驱动器485总线掉线
	unsigned char exAxisOFLIN;	//通信超时，控制卡与控制箱板485通信超时
	uint8_t	exAxisHomeStatus;	//外部轴回零状态
}EXTERNALAXIS_STATUS;
#pragma pack(pop)

#pragma pack(push, 1)
/** 运动控制器状态结构体 */
typedef struct _CTRL_STATE
{
	int			frame_len;               /**< 消息帧长度                     */
	double     	runtime;                 /**< 控制器启动时间,断电清零        */
	double 	   	jt_tgt_pos[6];           /**< 关节1-6目标位置                */
	double     	jt_tgt_vel[6];           /**< 关节1-6目标速度                */
	double     	jt_tgt_acc[6];			/**< 关节1-6目标加速度              */
	double     	jt_tgt_cur[6];			/**< 关节1-6目标电流		        */
	double 	   	jt_tgt_tor[6];           /**< 关节1-6目标扭矩                */
	double     	jt_cur_pos[6];           /**< 关节1-6当前位置                */
	double     	jt_cur_vel[6]; 			/**< 关节1-6当前速度                */
	double     	jt_cur_cur[6];			/**< 关节1-6当前电流 		        */
	double     	jt_cur_tor[6];			/**< 关节1-6当前扭矩                */
	double     	tl_cur_pos[6];			/**< 工具当前位置DKR		        */
	double	   	tl_cur_vel[6];			/**< 工具当前速度DKR                */
	double     	tl_jtforce[6];			/**< 工具合力DKR			        */
	double     	tl_tgt_pos[6];  			/**< 工具目标位置DKR	            */
	double     	tl_tgt_vel[6];			/**< 工具目标速度DKR                */
	double     	tl_cur_pos_base[6];  	/**< 工具当前位置Base	            */
	double     	tl_tgt_pos_base[6];		/**< 工具目标位置Base               */
	uint8_t	   	cl_dgt_input_h;          /**< 控制箱数字输入15-8             */
	uint8_t     cl_dgt_input_l;	        /**< 控制箱数字输入7-0              */
	uint8_t     tl_dgt_input_h;          /**< 工具数字输入15-8               */
	uint8_t     tl_dgt_input_l;          /**< 工具数字输入7-0                */
	uint8_t     tmp_dgt_input1;			/**< 空闲                           */
	uint8_t     tmp_dgt_input2;			/**< 空闲                           */
	uint8_t     tmp_dgt_input3;			/**< 空闲                           */
	uint8_t     tmp_dgt_input4;			/**< 空闲                           */
	uint16_t    analog_input[6];         /**< 模拟输入,四个控制箱,2个工具    */
	double     	ctrl_time;               /**< 控制器实时线程执行时间         */
	double     	test_data;               /**< 测试数据ur机器人软件使用       */
	uint8_t    	robot_mode;              /**< 机器人模式 0:自动 1:手动       */
	double     	joint_mode[6];  			/**< 关节模式					    */
	double     	safe_mode;               /**< 安全模式                       */
	int        	collision_level;         /**< 碰撞等级,一级敏感,三级不敏感   */
	double     	drag_enable;   			/**< 拖动使能                       */
	double     	tl_acc[7];				/**< 工具加速度     			    */
	double     	dr_com_err; 				/**< 与驱动器通信故障               */
	double	   	dr_err;                  /**< 驱动器哪个轴故障,驱动器故障              */
	double     	out_sflimit_err;         /**< 超出软限位故障                 */
	double     	collision_err;           /**< 碰撞故障                       */
	double     	dr_err_code;  			/**< 驱动器故障代码 			    */
	double     	err_reser;               /**< 故障预留                       */
	double     	vel_ratio;               /**< 机器人运行速度比例             */
	double     	linear_m_bmak;           /**< 线性动量基准 				    */
	uint8_t     flag_zero_set;           /**< 机器人零位设定完成标志         */
	double     	test_data1;  			/**< 测试数据ur机器人软件使用       */
	double     	cb_vol;					/**< 控制板电压   				    */
	double     	cb_robot_vol;            /**< 控制板机器人电压               */
	double     	cb_robot_cur;            /**< 控制板机器人电流               */
	double		jt_cur_vol[6];           /**< 关节当前电压                   */
	uint8_t	   	cl_dgt_output_h;          /**< 控制箱数字输出15-8             */
	uint8_t     cl_dgt_output_l;	        /**< 控制箱数字输出7-0              */
	uint8_t     tl_dgt_output_h;			/**< 末端工具数字输出15-8 (暂未使用)     */
	uint8_t     tl_dgt_output_l;			/**< 末端工具数字输出7-0 (bit0-bit1有效)     */
	uint8_t     tmp_dgt_output3;			/**< 空闲                           */
	uint8_t     tmp_dgt_output4;			/**< 空闲                           */
	uint8_t     tmp_dgt_output5;			/**< 空闲                           */
	uint8_t     tmp_dgt_output6;			/**< 空闲                           */
	uint16_t   	analog_output[6];         /**< 模拟输出,四个控制箱,1个工具(当前只引出一路)    */
	uint8_t     program_state;           /**< 程序状态1:停止,2:运行,3：暂停,4：拖动                       */
	int        	line_number;             /**< 自动运行程序执行到某一指令行号 */
	double     	elbow_vel[5];            /**< 肘速度                         */
	uint8_t    	strangePosFlag;          /**< 当前处于奇异位姿标志 */
	int        	configStatus;            /**< 机器人关节配置状态  */
	uint8_t    	aliveSlaveNumError;    /**< 活动从站数量错误，1：数量错误；0：正常  */
	uint8_t    	slaveComError[MAXSLAVES];      /**< 从站错误，0：正常；1：从站掉线；2：从站状态与设置值不一致；3：从站未配置；4：从站配置错误；5：从站初始化错误；6：从站邮箱通信初始化错误 */
	uint8_t    	cmdPointError;      /**< 指令点关节位置与末端位姿不符错误，0：正常；*/
	uint8_t    	ioError;                 /** IO错误 */
	uint8_t    	gripperError;            /** 夹爪错误 */
	uint8_t    	fileError;               /** 文件错误 */
	uint8_t    	paraError;               /** 参数错误 */
	uint8_t	   	exaxis_out_slimit_error;  /** 外部轴超出软限位故障 */
	EXTERNALAXIS_STATUS  exaxis_status[4];	/** 外部轴状态反馈结构体*/
	uint8_t    	exAxisActiveFlag;		/** 外部轴激活标志*/
	uint8_t    	alarm;                   /** 警告 */
	uint8_t    	safetydoor_alarm;        /** 安全门警告 */
	int        	toolNum;                 /** 工具号 */
	int        	workPieceNum;            /** 工件号 */
	int        	exAxisNum;            	/** 外部轴号 */
	uint8_t    	gripperFaultId;          /** 错误夹爪号 */
	uint8_t    	gripperFaultNum;         /** 夹爪错误编号 */
	uint16_t   	gripperActStatus;		/** 夹爪激活状态 */
	uint8_t    	gripperMotionDone;		/** 夹爪运动完成 */
	int 	   	aliveSlaveNumFeedback;   /** 活动从站数量反馈 */
	uint8_t	   	ctrl_query_state;		/** 控制器查询状态 0-未查询，1-查询中 */
	uint8_t    	weld_readystate;			/** 焊机准备状态 1-准备好；0-未准备好 */
	double     	weldTrackSpeed;			/** 焊缝跟踪速度 mm/s */
	uint8_t    	drag_alarm;				/** 拖动警告, 当前处于自动模式, 0-不报警 1-报警 */
	double	   	LoadIdentifyData[4];		/** 负载辨识结果 (weight, x, y, z) */
	long	   	conveyor_encoder_pos;	/** 传送带编码器位置 */
	double	   	conveyor_speed;			/** 传送带速度 mm/s */
	double	   	conveyorWorkPiecePos;	/** 传送带工件当前位置，单位mm */
	uint8_t	   	btn_box_stop_signal;		/** 按钮盒急停信号, 1-按下急停 */
	uint8_t	   	motionAlarm;				/** 运动警告 */
	uint8_t	   	interfereAlarm;			/** 进入干涉区警告 */
	REG_VAR	   	reg_var;					/** 注册变量 */
	uint8_t	   	encoder_type_flag;		/** 编码器类型切换完成标志,0:未完成,1:完成,2:超时 */
	uint8_t	   	curEncoderType[6]; 		/** 当前各轴编码器类型,0:光编,1:磁编 */
	uint8_t    	alarm_check_emerg_stop_btn; /** 1-通信异常,检查急停按钮是否松开 */
	uint8_t    	alarm_reboot_robot; 		/** 1-断电重启机器人 */
	uint8_t    	ts_web_state_com_error; 	/** 扭矩：WEB-TM 状态反馈，通信失败 */
	uint8_t    	ts_tm_cmd_com_error; 	/** 扭矩：TM-扭矩 指令下发，通信失败 */
	uint8_t    	ts_tm_state_com_error; 	/** 扭矩：TM-扭矩 状态反馈，通信失败 */
	uint8_t	   	pause_parameter; 		/** pause 参数 */
	float	   	sys_var[TM_SYS_VAR_NUM]; /** 系统变量 */
	uint8_t	   	tpd_record_state;		/** TPD记录状态， 1-记录中， 0-不记录 */
	uint8_t	   	tpd_record_scale;		/** TPD记录进度百分比，0-100 */
	uint8_t	   	tpd_num_limit;			/** TPD轨迹加载数量超限，0-未超限，1-超限 */
	double	   	FT_data[6];				/** 力/扭矩传感器数据，Fx,Fy,Fz,Tx,Ty,Tz */
	uint8_t	   	FT_ActStatus;			/** 力/扭矩传感器激活状态， 0-复位，1-激活 */
	int	   	   	motion_done;				/** 运动完成信号， 0-未完成，1-完成 */
	uint8_t	   	abnormal_stop;			/** 非正常停止  0-正常，1-非正常 */
	uint8_t	   	socket_conn_timeout;		/** socket连接超时，1-4 */
	uint8_t	   	socket_read_timeout;		/** socket读取超时，1-4 */
	uint16_t   	virtual_cl_dgt_input[MAXPIOIN]; 		/** 控制箱模拟 DI 输入 */
	uint16_t   	virtual_tl_dgt_input[MAXPIOIN]; 		/** 末端模拟 DI 输入 */
	float	   	virtual_cl_analog_input[MAXADCIN];	/** 控制箱模拟 AI 输入 */
	float	   	virtual_tl_analog_input[MAXAUXADCIN];/** 末端模拟 AI 输入 */
	uint8_t		pushBtnBoxState;		/** 按钮盒状态, 0-使用，1-禁用 */
	int			rbtEnableState;			/** 机器人使能状态, 0-不使能，1-使能 */
} CTRL_STATE;
#pragma pack(pop)

/* DB JSON 结构体 */
typedef struct _DB_JSON
{
	cJSON *point;
	cJSON *cdsystem;
	cJSON *wobj_cdsystem;
	cJSON *et_cdsystem;
	cJSON *sysvar;
} DB_JSON;

#pragma pack(push, 1)
/** PI_STATUS 结构体 */
typedef struct _PI_STATUS
{
	uint8_t power_supply_mode;			/** 供电方式检测，H：外部供电，L：电池 */
	uint8_t electric_quantity[4];		/** 电池电量，3%~25%:LHHH，25%~50%:LLHH, 50%~75%:LLLH, >75%:LLLL */
	uint8_t key[2];						/** 钥匙，自动模式：钥匙1H+钥匙2H，手动模式：钥匙1H+钥匙2L */
	uint8_t start;						/** 开始按钮，H:按下 */
	uint8_t stop;						/** 停止按钮，L:按下 */
	uint8_t axis_plus[6];				/** 轴增，L:按下 */
	uint8_t axis_minus[6];				/** 轴减，L:按下 */
	uint8_t custom[4];					/** 自定义，L:按下 */
} PI_STATUS;
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

/** 扭矩管理系统状态 */
#pragma pack(push, 1)
typedef struct _TORQUE_SYS_STATE
{
	uint16_t motion_state;		/** 运动状态 0-停止，1-运动 */
	uint16_t input_io_state;	/** 输入IO状态, 4pin In:[0,0,0,0]{拧紧、反松、自由、备用} */
	uint16_t output_io_state;	/** 输出IO状态, 4pin Out:[0,0,0,0]{BUSY、OK、ERR（浮高、滑牙和驱动器故障等）、备用} */
	uint16_t lock_result;		/** 锁附结果 */
	uint16_t error_code;		/** 故障码 */
	uint16_t task_runtime;		/** 任务执行时间 */
	double feed_turns;			/** 反馈圈数 单位：0.01r */
	double feed_rev;			/** 反馈转速 单位：rpm/min */
	double feed_torque;		/** 反馈扭矩 单位：mN.m */
	uint16_t work_state;		/** 工作状态 0-停止，1-拧紧，2-反松，3-自由 */
	//uint16_t btn_state;			/** 按钮状态 仅适用于手动款，[0,0]{0-拧紧/1-反松、0-停止/1-启动} */
	uint8_t control_mode;		/** 当前控制方式 0-软件控制，1-IO控制 */
	uint8_t current_unit;		/** 当前单位 0-mN·m，1-kgf·cm */
} TORQUE_SYS_STATE;
#pragma pack(pop)

/** 状态查询结构体 */
typedef struct _STATE_FEEDBACK
{
	int id[STATEFB_ID_MAXNUM];			// state feedback id    查询变量数组
	int icount;							// state feedback icount  查询变量数组中，查询变量的个数
	int type;							// state feedback type, "0": 图表查询，"1": 轨迹数据查询, "2": 查询 10 秒内固定机器人数据，"3":查询 10 秒内部分选择的机器人数据
	int overflow;						// state feedback overflow
	int index;						    // state feedback index "最大为 100"
	char *buf;						    // state feedback buf 缓存数据
} STATE_FEEDBACK;

/** 扭矩管理系统结构体 */
typedef struct _TORQUE_SYS
{
	int enable;
	pthread_t t_socket_TORQUE_SYS;
} TORQUE_SYS;

/** 嘉宝--扭矩管理系统生产数据结构体 */
typedef struct _JIABAO_TORQUE_PRODUCTION_DATA
{
	char left_wk_id[100]; 			/** 左工位工件编号 */
	int left_product_count;			/** 左工位总生产量 */
	int left_NG_count;				/** 左工位 NG 数量 */
	int left_work_time;				/** 左工位工时 */
	char right_wk_id[100];			/** 右工位工件编号 */
	int right_product_count;		/** 右工位总生产量 */
	int right_NG_count;				/** 右工位 NG 数量 */
	int right_work_time;			/** 右工位工时 */
} JIABAO_TORQUE_PRODUCTION_DATA;

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

/* PI status socket 相关信息结构体 */
typedef struct _SOCKET_PI_INFO
{
	int fd;							/** socket fd */
	char server_ip[20];				/** server ip */
	int server_port;				/** server port */
	int select_timeout; 			/** socket select timeout */
	int pre_connect_status; 		/** pre socket 连接状态 0:未连接 1:连接 */
	int connect_status; 			/** socket 连接状态 0:未连接 1:连接 */
	int send_flag; 					/** send data 标志位，1:代表需要发送数据，0:代表没有数据需要发送 */
} SOCKET_PI_INFO;

/* PI pthread 相关信息结构体 */
typedef struct _PI_PTHREAD
{
	int enable;						/** 启用线程 1:启用，0:未启用 */
	pthread_t t_pi;					/** 线程 ID */
} PI_PTHREAD;

/* socket connect client info */
typedef	struct _SOCKET_CONNECT_CLIENT_INFO
{
	int clnt_fd;					// client fd
	int connect_status; 			// server 与 client socket 连接状态
	int msghead; 					// 记录消息头, 依此增加
} SOCKET_CONNECT_CLIENT_INFO;

/* socket server 相关信息结构体 */
typedef struct _SOCKET_SERVER_INFO
{
	int serv_fd;					// server fd
	char server_ip[20];
	int server_port;
	int server_sendcmd_TM_flag;				// 大于0: 代表 webserver 直接发送 cmd 指令到任务管理器， 0: 默认值，代表这是示教器发送指令或者 webserver 直接发送的 cmd 指令任务管理器已经回复了
	SOCKET_CONNECT_CLIENT_INFO socket_connect_client_info[SOCKET_CONNECT_CLIENT_NUM_MAX];
} SOCKET_SERVER_INFO;

/* point home 相关信息 */
typedef struct _POINT_HOME_INFO
{
	int error_flag;			// 0: 初始值，代表正常， 1：检查 homepoint 发现出错
	int pre_error_flag;
} POINT_HOME_INFO;

/* 更酷客户相关结构体信息 */
typedef struct _ZHENGKU
{
	int setvar;							// 更酷程序标志 0: 初始值，代表未下发设置变量指令,或者设置变量指令运行程序已经结束， 1：代表已经下发设置变量指令   2: 正在运行程序
	int backhome;						// 更酷回原点程序标志 0: 初始值，代表未下发回原点指令,或者回原点指令运行程序已经结束， 1：代表已经下发回原点指令   2: 正在运行程序
	char result[100];					// "0": 初始值，代表未查询或者程序运行未结束， 程序运行结束时：存储运行的 lua 文件名
	int line_num;						// 程序运行行号
	char luaname[100];					// 运行程序名称
	char lua_content[GENGKU_LUASIZE];	// 程序运行内容
} ZHENGKU;

/********************************* Function declaration ***********************/

void *socket_thread(void *arg);
void *socket_status_thread(void *arg);
void *socket_TORQUE_SYS_thread(void *arg);
void *socket_state_feedback_thread(void *arg);
void *socket_upper_computer_thread(void* arg);
void *socket_pi_status_thread(void *arg);
void *socket_pi_cmd_thread(void *arg);
int socket_enquene(SOCKET_INFO *sock, const int type, char *send_content, const int cmd_type);
int check_pointhome_data(char *arr[]);
int init_network();
int init_db_json(DB_JSON *p_db_json);
void db_json_delete(DB_JSON *p_db_json);
int parse_lua_cmd(char *lua_cmd, char *file_content, DB_JSON *p_db_json);
//int send_cmd_set_robot_type();

#endif
