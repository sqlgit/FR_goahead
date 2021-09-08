/*
    goahead.c -- Main program for GoAhead

    Usage: goahead [options] [documents] [IP][:port] ...
        Options:
        --auth authFile        # User and role configuration
        --background           # Run as a Linux daemon
        --home directory       # Change to directory to run
        --log logFile:level    # Log to file file at verbosity level
        --route routeFile      # Route configuration file
        --verbose              # Same as --log stdout:2
        --version              # Output version information

    Copyright (c) All Rights Reserved. See details at the end of the file.
 */

/********************************* Includes ***********************************/

#include    "goahead.h"
#include	"js.h"
#include	"cJSON.h"
#include	"action_set.h"
#include	"action_get.h"
#include	"action_act.h"
#include	"action_sta.h"
#include	"filehandler.h"
#include	"tools.h"
#include 	"robot_socket.h"

/********************************* Defines ************************************/

static int finished = 0;
int robot_type = 1;// 默认实体机器人
int language = 0;// 默认中文
ACCOUNT_INFO cur_account;
/********************************* Function declaration ***********************/

static void initPlatform(void);
static void logHeader(void);
static void usage(void);

static void personInfoAction(Webs *wp);
static void actionTest(Webs *wp);
static int actionTest2(Webs *wp);
static void ShowString(Webs *wp);
static void on_led_set(Webs *wp);
static void on_buzzer_set(Webs *wp);
static void getDate(int jid, Webs *wp, int argc, char **argv);
static int AspMyTest(int eid, Webs *wp, int argc, char **argv);
static void FormMyTest(Webs *wp, char *path, char *query);


#if WINDOWS
static void windowsClose();
static int windowsInit();
static LRESULT CALLBACK websWindProc(HWND hwnd, UINT msg, UINT wp, LPARAM lp);
#endif

#if ME_UNIX_LIKE
static void sigHandler(int signo);
#endif

/*********************************** Code *************************************/

MAIN(goahead, int argc, char **argv, char **envp)
{
    char    *argp, *home, *documents, *endpoints, *endpoint, *route, *auth, *tok, *lspec;
    int     argind;

#if WINDOWS
    if (windowsInit() < 0) {
        return 0;
    }
#endif
    route = "route.txt";
    auth = "auth.txt";

    for (argind = 1; argind < argc; argind++) {
        argp = argv[argind];
        if (*argp != '-') {
            break;

        } else if (smatch(argp, "--auth") || smatch(argp, "-a")) {
            if (argind >= argc) usage();
            auth = argv[++argind];

#if ME_UNIX_LIKE && !MACOSX
        } else if (smatch(argp, "--background") || smatch(argp, "-b")) {
            websSetBackground(1);
#endif

        } else if (smatch(argp, "--debugger") || smatch(argp, "-d") || smatch(argp, "-D")) {
            websSetDebug(1);

        } else if (smatch(argp, "--home")) {
            if (argind >= argc) usage();
            home = argv[++argind];
            if (chdir(home) < 0) {
                error("Cannot change directory to %s", home);
                exit(-1);
            }
        } else if (smatch(argp, "--log") || smatch(argp, "-l")) {
            if (argind >= argc) usage();
            logSetPath(argv[++argind]);

        } else if (smatch(argp, "--verbose") || smatch(argp, "-v")) {
            logSetPath("stdout:2");

        } else if (smatch(argp, "--route") || smatch(argp, "-r")) {
            route = argv[++argind];

        } else if (smatch(argp, "--version") || smatch(argp, "-V")) {
            printf("%s\n", ME_VERSION);
            exit(0);

        } else if (*argp == '-' && isdigit((uchar) argp[1])) {
            lspec = sfmt("stdout:%s", &argp[1]);
            logSetPath(lspec);
            wfree(lspec);

        } else {
            usage();
        }
    }
    documents = ME_GOAHEAD_DOCUMENTS;
    if (argc > argind) {
        documents = argv[argind++];
    }
    initPlatform();
    if (websOpen(documents, route) < 0) {
        error("Cannot initialize server. Exiting.");
        return -1;
    }
#if ME_GOAHEAD_AUTH
    if (websLoad(auth) < 0) {
        error("Cannot load %s", auth);
        return -1;
    }
#endif
    logHeader();
    if (argind < argc) {
        while (argind < argc) {
            endpoint = argv[argind++];
			printf("endpoint = %s\n", endpoint); // endpoint = ip, Ex: 192.168.152.129
            if (websListen(endpoint) < 0) {
                return -1;
            }
        }
    } else {
        endpoints = sclone(ME_GOAHEAD_LISTEN);
        for (endpoint = stok(endpoints, ", \t", &tok); endpoint; endpoint = stok(NULL, ", \t,", &tok)) {
#if !ME_COM_SSL
            if (strstr(endpoint, "https")) continue;
#endif
            if (websListen(endpoint) < 0) {
                wfree(endpoints);
                return -1;
            }
        }
        wfree(endpoints);
    }
#if ME_ROM && KEEP
    /*
        If not using a route/auth config files, then manually create the routes like this:
        If custom matching is required, use websSetRouteMatch. If authentication is required, use websSetRouteAuth.
     */
    websAddRoute("/", "file", 0);
#endif
#ifdef GOAHEAD_INIT
    /*
        Define your init function in main.me goahead.init, or
        configure with DFLAGS=GOAHEAD_INIT=myInitFunction
     */
    {
        extern int GOAHEAD_INIT();

        if (GOAHEAD_INIT() < 0) {
            exit(1);
        }
    }
#endif

	websDefineAction("get", get);
	websDefineAction("set", set);
	websDefineAction("sta", sta);
	websDefineAction("act", act);
	websDefineAction("upload", upload);
	websDefineAction("download", download);

//	websDefineHandler("test", testHandler, 0, 0, 0);    // 定义一个请求处理程序
	//websAddRoute("/test", "test", 0);
#if ME_GOAHEAD_LEGACY
	// 表示对 /goform 的请求都交给 websFormHandler 函数处理。函数的参数列表如下
	websUrlHandlerDefine("/legacy/", 0, 0, legacyTest, 0);    // 设置form方式调用时候的文件位置
	//    websFormDefine(T("odbc_form_web_login"), odbc_form_web_login);    // 定义form方式调用接口函数字符对应的函数名称
#endif
	websDefineAction("test", actionTest);    // goAction定义，在asp文件中调用C函数
	websDefineAction("test2", actionTest2);    // goAction定义，在asp文件中调用C函数
	//websUrlHandlerDefine("/ajax/json_data", 0, 0, ajax_json_data, 0);    // 设置form方式调用时候的文件位置
	websDefineAction("showString", ShowString);
	websDefineAction("personInfoAction", personInfoAction);
	websDefineAction("led", on_led_set);
	websDefineAction("buzzer", on_buzzer_set);
	websDefineAction("FormMyTest", FormMyTest);
	websDefineJst("AspMyTest", AspMyTest);
	websDefineJst("date", getDate);


#if ME_UNIX_LIKE && !MACOSX
    /*
        Service events till terminated
     */
    if (websGetBackground()) {
        if (daemon(0, 0) < 0) {
            error("Cannot run as daemon");
            return -1;
        }
    }
#endif

#if recover_mode

	create_dir(DIR_CDSYSTEM);
	create_dir(DIR_CFG);
	create_dir(DIR_POINTS);
	create_dir(DIR_ROBOT_CFG);
	create_dir(DIR_STATEFB);
	create_dir(DIR_TEMPLATE);
	create_dir(DIR_USER);
	create_dir(DIR_LOG);
	create_dir(DIR_LOG_EN);
	create_dir(DIR_LOG_JAP);
	create_dir(DIR_WELD);
	create_dir(DIR_ROBOT_TYPE);
	create_dir(DIR_TORQUESYS);
	create_dir(DIR_SYSVAR);
	delete_log_file(0);

	/* 如果标志 “升级成功” 的文件存在，需要检查并更新 file 文件夹下数据文件到最新 */
	if (check_dir_filename(DIR_FILE, FILENAME_UP_SUC) == 1) {
		//printf("update file dir\n");
		/** 更新 file 文件夹 */
		if (update_file_dir() == SUCCESS) {
			/* 更新完成，删除 “升级成功” 的标志文件夹 */
			char cmd[128] = {0};
			sprintf(cmd, "rm %s%s", DIR_FILE, FILENAME_UP_SUC);
			system(cmd);
		}
	}

	/* 更新控制器 IP */
	update_server_ip();

#endif

	pthread_t t_socket_cmd;
	pthread_t t_socket_file;
	pthread_t t_socket_status;
	pthread_t t_socket_state_feedback;
	pthread_t t_socket_upper_computer;
	pthread_t t_socket_pi_status;
	pthread_t t_socket_vir_cmd;
	pthread_t t_socket_vir_file;
	pthread_t t_socket_vir_status;

#if recover_mode
	/* create socket_cmd thread */
	if (pthread_create(&t_socket_cmd, NULL, (void *)&socket_thread, (void *)CMD_PORT)) {
		perror("pthread_create");
	}
	/* create socket_file thread */
	if (pthread_create(&t_socket_file, NULL, (void *)&socket_thread, (void *)FILE_PORT)) {
		perror("pthread_create");
	}
	/* create socket_status thread */
	if (pthread_create(&t_socket_status, NULL, (void *)&socket_status_thread, (void *)STATUS_PORT)) {
		perror("pthread_create");
	}
	/* create socket_state_feedback thread */
	if (pthread_create(&t_socket_state_feedback, NULL, (void *)&socket_state_feedback_thread, (void *)STATE_FEEDBACK_PORT)) {
		perror("pthread_create");
	}
	/* create socket_upper_computer thread */
	if (pthread_create(&t_socket_upper_computer, NULL, (void *)&socket_upper_computer_thread, (void *)UPPER_COMPUTER_PORT)) {
		perror("pthread_create");
	}
	/* create socket_pi_status thread */
	if (pthread_create(&t_socket_pi_status, NULL, (void *)&socket_pi_status_thread, (void *)PI_STATUS_PORT)) {
		perror("pthread_create");
	}
#endif
#if virtual_robot
	/* create socket_vir_cmd thread */
	if (pthread_create(&t_socket_vir_cmd, NULL, (void *)&socket_thread, (void *)VIR_CMD_PORT)) {
		perror("pthread_create");
	}
	/* create socket_vir_file thread */
	if (pthread_create(&t_socket_vir_file, NULL, (void *)&socket_thread, (void *)VIR_FILE_PORT)) {
		perror("pthread_create");
	}
	/* create socket_vir_status thread */
	if (pthread_create(&t_socket_vir_status, NULL, (void *)&socket_status_thread, (void *)VIR_STATUS_PORT)) {
		perror("pthread_create");
	}
#endif

    websServiceEvents(&finished);

#if recover_mode
	/* 线程挂起, 主线程要等到创建的线程返回了，获取该线程的返回值后主线程才退出 */
	if (pthread_join(t_socket_cmd, NULL)) {
		perror("pthread_join");
	}
	if (pthread_join(t_socket_file, NULL)) {
		perror("pthread_join");
	}
	if (pthread_join(t_socket_status, NULL)) {
		perror("pthread_join");
	}
	if (pthread_join(t_socket_state_feedback, NULL)) {
		perror("pthread_join");
	}
	if (pthread_join(t_socket_upper_computer, NULL)) {
		perror("pthread_join");
	}
	if (pthread_join(t_socket_pi_status, NULL)) {
		perror("pthread_join");
	}
#endif
#if virtual_robot
	if (pthread_join(t_socket_vir_cmd, NULL)) {
		perror("pthread_join");
	}
	if (pthread_join(t_socket_vir_file, NULL)) {
		perror("pthread_join");
	}
	if (pthread_join(t_socket_vir_status, NULL)) {
		perror("pthread_join");
	}
#endif

    logmsg(1, "Instructed to exit");
    websClose();
#if WINDOWS
    windowsClose();
#endif

    return 0;
}

static void getDate(int jid, Webs *wp, int argc, char **argv) {
		char *date = "2019/10/22 11:56";
		printf("date = %s\n", date);
		websWrite(wp, "%s", date);
		//gfree(date);
}

char g_test_name[32] = {0};
unsigned int g_test_age = 0;

static int AspMyTest(int eid, Webs *wp, int argc, char **argv)
{
		//char *name;
		char buffer[128];
		/* 判断参数是否过少 */
		/*if (jsArgs(argc, argv, "%s", &name) < 1) {
				websError(wp, 400, "Insufficient args\n");
				return -1;
		}*/
		/* 根据页面上input标签内的name属性判断将什么变量显示到页面上对应的文本框内 */
		printf("argc = %d\n", argc);
		printf("argv = %s\n", *argv);

		if (!strcmp(*argv, "name"))
		{   
				sprintf(buffer, "%s", g_test_name);
				return websWrite(wp, "%s", buffer);
				//websWrite()是goahead的API，可以将内容写回html页面
		}
		else if (!strcmp(*argv, "age"))
		{
				sprintf(buffer, "%d", g_test_age);  
				return websWrite(wp, "%s",buffer);
		}
		else
		{
				return -1;
		}
}

static void FormMyTest(Webs *wp, char *path, char *query)
{
		char *strval = NULL;
		strval = websGetVar(wp, "name", "");
		/* websGetVar()可以从页面取出文本框内的数据 */
		if (NULL != strval)
		{
				/* 将从页面上取出的数据存入全局变量中 */
				strcpy(g_test_name, strval);
		}

		strval = websGetVar(wp, "age", "");
		if (NULL != strval)
		{
				g_test_age = atoi(strval);
		}

		websSetStatus(wp, 200);
		websWriteHeaders(wp, -1, 0);
		websWriteEndHeaders(wp);
		//websRedirect(wp,"/mytest.html");
		websDone(wp);
}

typedef struct PERSON
{
		char *name;
		int age;
		char *gender;
}Person;

static void personInfoAction(Webs *wp)
{
		/*int i; 
		Person person[2];

		person[0].name = "kangkang";
		person[0].age = 12;
		person[0].gender = "male";
		person[1].name = "Jane";
		person[1].age = 14;
		person[1].gender = "female";

		websSetStatus(wp, 200);
		websWriteHeaders(wp, -1, 0);
		websWriteEndHeaders(wp);

		websWrite(wp,"[");
		for(i = 0;i < 2; i++)
		{
				websWrite(wp, "{\"name\":\"%s\",\"age\":\"%d\",\"gender\":\"%s\"}", person[i].name,person[i].age,person[i].gender);
				if(i != 1)
				{
						websWrite(wp, ",");
				}
		}
		websWrite(wp, "]");
		websDone(wp);*/

		char *char_json ="{\"aplist\":[{\"isKick\":false,\"fwver\":\"1.06b08\",\"his_traffic\":[0,0,0,0,0,0,0,0,0,0],\"method\":\"discovery\",\"clients\":0,\"channel\":\"1(ng),161(ac)\",\"g5channel\":161,\"name\":\"sqlTEW-821DAP\",\"acurl\":\"192.168.99.60\",\"acmac\":\"00:17:71:20:56:08\",\"up\":\"0B\",\"groupname\":\"1(2.4G)\/2(5G)\",\"model\":\"TEW-821DAP\",\"ip\":\"192.168.99.100\",\"down\":\"0B\",\"regstatus\":\"run\",\"isBlock\":false,\"type\":\"AP\",\"mac\":\"00:17:30:10:60:78\",\"domain\":\"0x14\"}]}"; 
		cJSON *json = cJSON_Parse(char_json);
		char *buf = NULL;
		printf("data:%s\n", buf = cJSON_Print(json));

		websSetStatus(wp, 200);
		websWriteHeaders(wp, -1, 0);
		websWriteEndHeaders(wp);
		websWrite(wp, char_json);
		websDone(wp);

		free(buf);
		cJSON_Delete(json);
}

static void actionTest(Webs *wp)
{
		char    *name, *address;

		name = websGetVar(wp, "name", NULL);
		address = websGetVar(wp, "address", NULL);
		websSetStatus(wp, 200);
		websWriteHeaders(wp, -1, 0);
		websWriteEndHeaders(wp);
		websWrite(wp, "<html><body><h2>name: %s, address: %s</h2></body></html>\n", name, address);
		websFlush(wp, 0);
		websDone(wp);
}

static int actionTest2(Webs *wp)
{
	websSetStatus(wp,200);

	websWriteHeaders(wp,-1,0);

	//websWriterHeader(wp,"Content-Type","application/text");

	char *jsonString= wp->input.servp;
	printf("jsonString = %s\n", jsonString);


		//char *test = NULL;
		//test = websGetVar(wp, "test", "");
		//printf("test = %s\n", test);

		
		/*cJSON *json = cJSON_Parse(jsonString);
		printf("aaa\n");
		char *buf = NULL;
		printf("data:%s\n", buf = cJSON_Print(json));*/
		
	/*	cJSON *item = cJSON_GetObjectItem(test,"db_user");
		printf("db_user:%s\n", item->valuestring);*/
		cJSON *resolution = NULL;
		cJSON *resolutions = NULL;
		const cJSON *name = NULL;
		int status = 0;
		cJSON *monitor_json = cJSON_Parse(jsonString);
		char *buf = NULL;
		printf("monitor:%s\n", buf = cJSON_Print(monitor_json));
		if (monitor_json == NULL)
		{
			const char *error_ptr = cJSON_GetErrorPtr();
			if (error_ptr != NULL)
			{
				fprintf(stderr, "Error before: %s\n", error_ptr);
			}
			status = 0;
			goto end;
		}

		name = cJSON_GetObjectItemCaseSensitive(monitor_json, "name");
		if (cJSON_IsString(name) && (name->valuestring != NULL))
		{
			printf("Checking name \"%s\"\n", name->valuestring);
		}

		resolutions = cJSON_GetObjectItem(monitor_json, "resolutions");
		printf("resolutions:%s\n", cJSON_Print(resolutions));
		printf("resolutions->type:%d\n", resolutions->type);
		printf("cJSON_Array:%d\n", cJSON_Array);
		//printf("resolutions size:%d\n", cJSON_GetArraySize(resolutions));
		if((resolutions->type) == (cJSON_Array))
		{
			printf("enter =\n");
			printf("array size is %d\n",cJSON_GetArraySize(resolutions));
		}
		/*if(resolutions->type == cJSON_Object)
		{
			printf("cjson object");
		}*/
		cJSON_ArrayForEach(resolution, resolutions)
		{
			printf("enter foreach\n");
			cJSON *width = cJSON_GetObjectItemCaseSensitive(resolution, "width");
			cJSON *height = cJSON_GetObjectItemCaseSensitive(resolution, "height");
			printf("Checking width \"%lf\"\n", width->valuedouble);
			printf("Checking height \"%lf\"\n", height->valuedouble);

			if (!cJSON_IsNumber(width) || !cJSON_IsNumber(height))
			{
				status = 0;
				goto end;
			}

			if ((width->valuedouble == 1920) && (height->valuedouble == 1080))
			{
			printf("width \"%lf\"\n", width->valuedouble);
			printf("height \"%lf\"\n", height->valuedouble);
				status = 1;
				goto end;
			}
		}

end:
		cJSON_Delete(monitor_json);
		return status;
}

static void ShowString(Webs *wp)
{
		//char result[64];
		/*WebsRoute *route;
		assert(wp);
		route = wp->route;
		assert(route);*/
		/*websSetStatus(wp, 200);
		websWriteHeaders(wp, -1, 0);
		websWriteEndHeaders(wp);
		sprintf(result, "I am ShowString : %s", websGetVar(wp, "str", ""));
		websWrite(wp,result);
		websFlush(wp, 0);
		websDone(wp);*/
		websRedirect(wp,"/test.html");
}

// led控制
static void on_led_set(Webs *wp)
{
		// get the input value in query stream
		char *io_val;

		io_val = websGetVar(wp, "val_led", NULL);
		if(io_val)
				system("echo 1 > /sys/class/leds/led-err/brightness");
		else
				system("echo 0 > /sys/class/leds/led-err/brightness");
}

// 蜂鸣器控制
static void on_buzzer_set(Webs *wp)
{
		char *io_val;

		io_val = websGetVar(wp, "val_buz", NULL);
		if(io_val)
				system("echo 1 > /sys/class/leds/beep/brightness");
		else
				system("echo 0 > /sys/class/leds/beep/brightness");
}

static void logHeader(void)
{
    char    home[ME_GOAHEAD_LIMIT_STRING];

    getcwd(home, sizeof(home));
    logmsg(2, "Configuration for %s", ME_TITLE);
    logmsg(2, "---------------------------------------------");
    logmsg(2, "Version:            %s", ME_VERSION);
    logmsg(2, "BuildType:          %s", ME_DEBUG ? "Debug" : "Release");
    logmsg(2, "CPU:                %s", ME_CPU);
    logmsg(2, "OS:                 %s", ME_OS);
    logmsg(2, "Host:               %s", websGetServer());
    logmsg(2, "Directory:          %s", home);
    logmsg(2, "Documents:          %s", websGetDocuments());
    logmsg(2, "Configure:          %s", ME_CONFIG_CMD);
    logmsg(2, "---------------------------------------------");
}


static void usage(void) {
    fprintf(stderr, "\n%s Usage:\n\n"
        "  %s [options] [documents] [[IPaddress][:port] ...]\n\n"
        "  Options:\n"
#if ME_GOAHEAD_AUTH
        "    --auth authFile        # User and role configuration\n"
#endif
#if ME_UNIX_LIKE && !MACOSX
        "    --background           # Run as a Unix daemon\n"
#endif
        "    --debugger             # Run in debug mode\n"
        "    --home directory       # Change to directory to run\n"
        "    --log logFile:level    # Log to file file at verbosity level\n"
        "    --route routeFile      # Route configuration file\n"
        "    --verbose              # Same as --log stdout:2\n"
        "    --version              # Output version information\n\n",
        ME_TITLE, ME_NAME);
    exit(-1);
}


static void initPlatform(void)
{
#if ME_UNIX_LIKE
    signal(SIGTERM, sigHandler);
    #ifdef SIGPIPE
        signal(SIGPIPE, SIG_IGN);
    #endif
#elif ME_WIN_LIKE
    _fmode=_O_BINARY;
#endif
}


#if ME_UNIX_LIKE
static void sigHandler(int signo)
{
    finished = 1;
}
#endif


#if WINDOWS
/*
    Create a taskbar entry. Register the window class and create a window
 */
static int windowsInit()
{
    HINSTANCE   inst;
    WNDCLASS    wc;                     /* Window class */
    HMENU       hSysMenu;
    HWND        hwnd;

    inst = websGetInst();
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = inst;
    wc.hIcon         = NULL;
    wc.lpfnWndProc   = (WNDPROC) websWindProc;
    wc.lpszMenuName  = wc.lpszClassName = ME_NAME;
    if (! RegisterClass(&wc)) {
        return -1;
    }
    /*
        Create a window just so we can have a taskbar to close this web server
     */
    hwnd = CreateWindow(ME_NAME, ME_TITLE, WS_MINIMIZE | WS_POPUPWINDOW, CW_USEDEFAULT,
        0, 0, 0, NULL, NULL, inst, NULL);
    if (hwnd == NULL) {
        return -1;
    }

    /*
        Add the about box menu item to the system menu
     */
    hSysMenu = GetSystemMenu(hwnd, FALSE);
    if (hSysMenu != NULL) {
        AppendMenu(hSysMenu, MF_SEPARATOR, 0, NULL);
    }
    ShowWindow(hwnd, SW_SHOWNORMAL);
    UpdateWindow(hwnd);
    return 0;
}


static void windowsClose()
{
    HINSTANCE   inst;

    inst = websGetInst();
    UnregisterClass(ME_NAME, inst);
}


/*
    Main menu window message handler.
 */
static LRESULT CALLBACK websWindProc(HWND hwnd, UINT msg, UINT wp, LPARAM lp)
{
    switch (msg) {
        case WM_DESTROY:
            PostQuitMessage(0);
            finished++;
            return 0;

        case WM_SYSCOMMAND:
            break;
    }
    return DefWindowProc(hwnd, msg, wp, lp);
}


/*
    Check for Windows Messages
 */
WPARAM checkWindowsMsgLoop()
{
    MSG     msg;

    if (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE)) {
        if (!GetMessage(&msg, NULL, 0, 0) || msg.message == WM_QUIT) {
            return msg.wParam;
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}


/*
    Windows message handler
 */
static LRESULT CALLBACK websAboutProc(HWND hwndDlg, uint msg, uint wp, long lp)
{
    LRESULT    lResult;

    lResult = DefWindowProc(hwndDlg, msg, wp, lp);

    switch (msg) {
        case WM_CREATE:
            break;
        case WM_DESTROY:
            break;
        case WM_COMMAND:
            break;
    }
    return lResult;
}

#endif

/*
    Copyright (c) Embedthis Software. All Rights Reserved.
    This software is distributed under commercial and open source licenses.
    You may use the Embedthis GoAhead open source license or you may acquire
    a commercial license from Embedthis Software. You agree to be fully bound
    by the terms of either license. Consult the LICENSE.md distributed with
    this software for full details and other copyrights.
 */
