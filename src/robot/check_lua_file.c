#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include 	"goahead.h"
#include	"cJSON.h"
#include 	"tools.h"
#include 	"robot_socket.h"
#include 	"check_lua_file.h"

lua_State *luaEnv = NULL;
char error_info[ERROR_SIZE] = {0};
static int pcall_lua(void *arg);
static void timeout_break(lua_State* L, lua_Debug* ar);
extern char lua_filename[FILENAME_SIZE];

//待Lua调用的C注册函数。
static int MoveJ(lua_State* L)
{
	int argc = lua_gettop(L);

	//printf("argc = %d\n", argc);
	if (argc != 29 && argc != 2) {
		luaL_argerror(L, argc, "Error number of parameters");
	}
	//返回值用于提示该C函数的返回值数量，即压入栈中的返回值数量。
	return 1;
}

static int SplinePTP(lua_State* L)
{
	int argc = lua_gettop(L);

	if (argc != 17) {
		luaL_argerror(L, argc, "Error number of parameters");
	}
	return 1;
}

static int ExtAxisMoveJ(lua_State* L)
{
	int argc = lua_gettop(L);

	if (argc != 6 & argc != 3) {
		luaL_argerror(L, argc, "Error number of parameters");
	}
	return 1;
}

static int MoveC(lua_State* L)
{
	int argc = lua_gettop(L);

	if (argc != 37) {
		luaL_argerror(L, argc, "Error number of parameters");
	}
	return 1;
}

static int SplineCIRC(lua_State* L)
{
	int argc = lua_gettop(L);

	if (argc != 33) {
		luaL_argerror(L, argc, "Error number of parameters");
	}
	return 1;
}

static int MoveL(lua_State* L)
{
	int argc = lua_gettop(L);

	if (argc != 9 && argc != 29 && argc != 2) {
		luaL_argerror(L, argc, "Error number of parameters");
	}
	return 1;
}

static int SplineLINE(lua_State* L)
{
	int argc = lua_gettop(L);

	if (argc != 17) {
		luaL_argerror(L, argc, "Error number of parameters");
	}
	return 1;
}

static int SetDO(lua_State* L)
{
	int argc = lua_gettop(L);

	if (argc != 3) {
		luaL_argerror(L, argc, "Error number of parameters");
	}
	return 1;
}

static int SetToolDO(lua_State* L)
{
	int argc = lua_gettop(L);

	if (argc != 3) {
		luaL_argerror(L, argc, "Error number of parameters");
	}
	return 1;
}

static int GetDI(lua_State* L)
{
	int argc = lua_gettop(L);

	if (argc != 1) {
		luaL_argerror(L, argc, "Error number of parameters");
	}
	return 1;
}

static int GetToolDI(lua_State* L)
{
	int argc = lua_gettop(L);

	if (argc != 1) {
		luaL_argerror(L, argc, "Error number of parameters");
	}
	return 1;
}

static int SetAO(lua_State* L)
{
	int argc = lua_gettop(L);

	if (argc != 2) {
		luaL_argerror(L, argc, "Error number of parameters");
	}
	return 1;
}

static int SetToolAO(lua_State* L)
{
	int argc = lua_gettop(L);

	if (argc != 2) {
		luaL_argerror(L, argc, "Error number of parameters");
	}
	return 1;
}

static int GetAI(lua_State* L)
{
	int argc = lua_gettop(L);

	if (argc != 1) {
		luaL_argerror(L, argc, "Error number of parameters");
	}
	return 1;
}

static int GetToolAI(lua_State* L)
{
	int argc = lua_gettop(L);

	if (argc != 1) {
		luaL_argerror(L, argc, "Error number of parameters");
	}
	return 1;
}

static int WaitDI(lua_State* L)
{
	int argc = lua_gettop(L);

	if (argc != 4) {
		luaL_argerror(L, argc, "Error number of parameters");
	}
	return 1;
}

static int WaitToolDI(lua_State* L)
{
	int argc = lua_gettop(L);

	if (argc != 4) {
		luaL_argerror(L, argc, "Error number of parameters");
	}
	return 1;
}

static int WaitAI(lua_State* L)
{
	int argc = lua_gettop(L);

	if (argc != 5) {
		luaL_argerror(L, argc, "Error number of parameters");
	}
	return 1;
}

static int WaitToolAI(lua_State* L)
{
	int argc = lua_gettop(L);

	if (argc != 5) {
		luaL_argerror(L, argc, "Error number of parameters");
	}
	return 1;
}

static int MoveTPD(lua_State* L)
{
	int argc = lua_gettop(L);

	if (argc != 3) {
		luaL_argerror(L, argc, "Error number of parameters");
	}
	return 1;
}

static int MoveGripper(lua_State* L)
{
	int argc = lua_gettop(L);

	if (argc != 5) {
		luaL_argerror(L, argc, "Error number of parameters");
	}
	return 1;
}

static int SetToolList(lua_State* L)
{
	int argc = lua_gettop(L);

	if (argc != 9) {
		luaL_argerror(L, argc, "Error number of parameters");
	}
	return 1;
}

static int SetWobjList(lua_State* L)
{
	int argc = lua_gettop(L);

	if (argc != 7) {
		luaL_argerror(L, argc, "Error number of parameters");
	}
	return 1;
}

static int SetExToolList(lua_State* L)
{
	int argc = lua_gettop(L);

	if (argc != 13) {
		luaL_argerror(L, argc, "Error number of parameters");
	}
	return 1;
}

static int WeaveStart(lua_State* L)
{
	int argc = lua_gettop(L);

	if (argc != 1) {
		luaL_argerror(L, argc, "Error number of parameters");
	}
	return 1;
}

static int WeaveEnd(lua_State* L)
{
	int argc = lua_gettop(L);

	if (argc != 1) {
		luaL_argerror(L, argc, "Error number of parameters");
	}
	return 1;
}

static int ARCStart(lua_State* L)
{
	int argc = lua_gettop(L);

	if (argc != 2) {
		luaL_argerror(L, argc, "Error number of parameters");
	}
	return 1;
}

static int ARCEnd(lua_State* L)
{
	int argc = lua_gettop(L);

	if (argc != 2) {
		luaL_argerror(L, argc, "Error number of parameters");
	}
	return 1;
}

static int LTLaserOn(lua_State* L)
{
	int argc = lua_gettop(L);

	if (argc != 1) {
		luaL_argerror(L, argc, "Error number of parameters");
	}
	return 1;
}

static int LTLaserOff(lua_State* L)
{
	int argc = lua_gettop(L);

	if (argc != 0) {
		luaL_argerror(L, argc, "Error number of parameters");
	}
	return 1;
}

static int LTSearchStart(lua_State* L)
{
	int argc = lua_gettop(L);

	if (argc != 5) {
		luaL_argerror(L, argc, "Error number of parameters");
	}
	return 1;
}

static int LTSearchStop(lua_State* L)
{
	int argc = lua_gettop(L);

	if (argc != 0) {
		luaL_argerror(L, argc, "Error number of parameters");
	}
	return 1;
}

static int PostureAdjustOn(lua_State* L)
{
	int argc = lua_gettop(L);

	if (argc != 17) {
		luaL_argerror(L, argc, "Error number of parameters");
	}
	return 1;
}

static int PostureAdjustOff(lua_State* L)
{
	int argc = lua_gettop(L);

	if (argc != 1) {
		luaL_argerror(L, argc, "Error number of parameters");
	}
	return 1;
}

static int ConveyorIODetect(lua_State* L)
{
	int argc = lua_gettop(L);

	if (argc != 1) {
		luaL_argerror(L, argc, "Error number of parameters");
	}
	return 1;
}

static int Mode(lua_State* L)
{
	int argc = lua_gettop(L);

	if (argc != 1) {
		luaL_argerror(L, argc, "Error number of parameters");
	}
	return 1;
}

/*
static int SegmentWeldStart(lua_State* L)
{
	int argc = lua_gettop(L);

	if (argc != 6) {
		luaL_argerror(L, argc, "Error number of parameters");
	}
	return 1;
}
*/

static int RegisterVar(lua_State* L)
{
	int argc = lua_gettop(L);

	if (argc < 1 || argc > 6) {
		luaL_argerror(L, argc, "Error number of parameters");
	}
	return 1;
}

static int LTTrackOn(lua_State* L)
{
	int argc = lua_gettop(L);

	if (argc != 1) {
		luaL_argerror(L, argc, "Error number of parameters");
	}
	return 1;
}

static int LTTrackOff(lua_State* L)
{
	int argc = lua_gettop(L);

	if (argc != 0) {
		luaL_argerror(L, argc, "Error number of parameters");
	}
	return 1;
}

static int LaserSensorRecord(lua_State* L)
{
	int argc = lua_gettop(L);

	if (argc != 2) {
		luaL_argerror(L, argc, "Error number of parameters");
	}
	return 1;
}

static int LaserRecordPoint(lua_State* L)
{
	int argc = lua_gettop(L);

	if (argc != 2) {
		luaL_argerror(L, argc, "Error number of parameters");
	}
	return 1;
}

static int ExtAxisSetHoming(lua_State* L)
{
	int argc = lua_gettop(L);

	if (argc != 5) {
		luaL_argerror(L, argc, "Error number of parameters");
	}
	return 1;
}

static int ExtAxisServoOn(lua_State* L)
{
	int argc = lua_gettop(L);

	if (argc != 2) {
		luaL_argerror(L, argc, "Error number of parameters");
	}
	return 1;
}

static int SPLCSetDO(lua_State* L)
{
	int argc = lua_gettop(L);

	if (argc != 2) {
		luaL_argerror(L, argc, "Error number of parameters");
	}
	return 1;
}

static int SPLCSetToolDO(lua_State* L)
{
	int argc = lua_gettop(L);

	if (argc != 2) {
		luaL_argerror(L, argc, "Error number of parameters");
	}
	return 1;
}

static int SPLCSetAO(lua_State* L)
{
	int argc = lua_gettop(L);

	if (argc != 2) {
		luaL_argerror(L, argc, "Error number of parameters");
	}
	return 1;
}

static int SPLCSetToolAO(lua_State* L)
{
	int argc = lua_gettop(L);

	if (argc != 2) {
		luaL_argerror(L, argc, "Error number of parameters");
	}
	return 1;
}

static int SprayStart(lua_State* L)
{
	int argc = lua_gettop(L);

	if (argc != 0) {
		luaL_argerror(L, argc, "Error number of parameters");
	}
	return 1;
}

static int SprayStop(lua_State* L)
{
	int argc = lua_gettop(L);

	if (argc != 0) {
		luaL_argerror(L, argc, "Error number of parameters");
	}
	return 1;
}

static int PowerCleanStart(lua_State* L)
{
	int argc = lua_gettop(L);

	if (argc != 0) {
		luaL_argerror(L, argc, "Error number of parameters");
	}
	return 1;
}

static int PowerCleanStop(lua_State* L)
{
	int argc = lua_gettop(L);

	if (argc != 0) {
		luaL_argerror(L, argc, "Error number of parameters");
	}
	return 1;
}

static int LoadPosSensorDriver(lua_State* L)
{
	int argc = lua_gettop(L);

	if (argc != 1) {
		luaL_argerror(L, argc, "Error number of parameters");
	}
	return 1;
}

static int UnloadPosSensorDriver(lua_State* L)
{
	int argc = lua_gettop(L);

	if (argc != 0) {
		luaL_argerror(L, argc, "Error number of parameters");
	}
	return 1;
}

static int ConveyorGetTrackData(lua_State* L)
{
	int argc = lua_gettop(L);

	if (argc != 0) {
		luaL_argerror(L, argc, "Error number of parameters");
	}
	return 1;
}

static int ConveyorTrackStart(lua_State* L)
{
	int argc = lua_gettop(L);

	if (argc != 1) {
		luaL_argerror(L, argc, "Error number of parameters");
	}
	return 1;
}

static int ConveyorTrackEnd(lua_State* L)
{
	int argc = lua_gettop(L);

	if (argc != 0) {
		luaL_argerror(L, argc, "Error number of parameters");
	}
	return 1;
}

static int GetSysVarValue(lua_State* L)
{
	int argc = lua_gettop(L);

	if (argc != 1) {
		luaL_argerror(L, argc, "Error number of parameters");
	}
	return 1;
}

static int SetSysVarValue(lua_State* L)
{
	int argc = lua_gettop(L);

	if (argc != 2) {
		luaL_argerror(L, argc, "Error number of parameters");
	}
	return 1;
}

static int StartJOG(lua_State* L)
{
	int argc = lua_gettop(L);

	if (argc != 6) {
		luaL_argerror(L, argc, "Error number of parameters");
	}
	return 1;
}

static int ServoJ(lua_State* L)
{
	int argc = lua_gettop(L);

	if (argc != 11) {
		luaL_argerror(L, argc, "Error number of parameters");
	}
	return 1;
}

static int Pause(lua_State* L)
{
	int argc = lua_gettop(L);

	if (argc != 1) {
		luaL_argerror(L, argc, "Error number of parameters");
	}
	return 1;
}

static int Stop(lua_State* L)
{
	int argc = lua_gettop(L);

	if (argc != 0) {
		luaL_argerror(L, argc, "Error number of parameters");
	}
	return 1;
}

static int VMoveJ(lua_State* L)
{
	int argc = lua_gettop(L);

	if (argc != 9) {
		luaL_argerror(L, argc, "Error number of parameters");
	}
	return 1;
}

static int VMoveL(lua_State* L)
{
	int argc = lua_gettop(L);

	if (argc != 9) {
		luaL_argerror(L, argc, "Error number of parameters");
	}
	return 1;
}

static int SPLCGetDI(lua_State* L)
{
	int argc = lua_gettop(L);

	if (argc != 3) {
		luaL_argerror(L, argc, "Error number of parameters");
	}
	return 1;
}

static int SPLCGetToolDI(lua_State* L)
{
	int argc = lua_gettop(L);

	if (argc != 3) {
		luaL_argerror(L, argc, "Error number of parameters");
	}
	return 1;
}

static int SPLCGetAI(lua_State* L)
{
	int argc = lua_gettop(L);

	if (argc != 4) {
		luaL_argerror(L, argc, "Error number of parameters");
	}
	return 1;
}

static int SPLCGetToolAI(lua_State* L)
{
	int argc = lua_gettop(L);

	if (argc != 4) {
		luaL_argerror(L, argc, "Error number of parameters");
	}
	return 1;
}

static int WaitMs(lua_State* L)
{
	int argc = lua_gettop(L);

	if (argc != 1) {
		luaL_argerror(L, argc, "Error number of parameters");
	}
	return 1;
}

static int SetSpeed(lua_State* L)
{
	int argc = lua_gettop(L);

	if (argc != 1) {
		luaL_argerror(L, argc, "Error number of parameters");
	}
	return 1;
}

static int SetLoadWeight(lua_State* L)
{
	int argc = lua_gettop(L);

	if (argc != 1) {
		luaL_argerror(L, argc, "Error number of parameters");
	}
	return 1;
}

static int SetLoadCoord(lua_State* L)
{
	int argc = lua_gettop(L);

	if (argc != 3) {
		luaL_argerror(L, argc, "Error number of parameters");
	}
	return 1;
}

static int SplineStart(lua_State* L)
{
	int argc = lua_gettop(L);

	if (argc != 0) {
		luaL_argerror(L, argc, "Error number of parameters");
	}
	return 1;
}

static int SplineEnd(lua_State* L)
{
	int argc = lua_gettop(L);

	if (argc != 0) {
		luaL_argerror(L, argc, "Error number of parameters");
	}
	return 1;
}

static int ActGripper(lua_State* L)
{
	int argc = lua_gettop(L);

	if (argc != 2) {
		luaL_argerror(L, argc, "Error number of parameters");
	}
	return 1;
}

static int FT_Guard(lua_State* L)
{
	int argc = lua_gettop(L);

	if (argc != 14) {
		luaL_argerror(L, argc, "Error number of parameters");
	}
	return 1;
}

static int FT_Control(lua_State* L)
{
	int argc = lua_gettop(L);

	if (argc != 22) {
		luaL_argerror(L, argc, "Error number of parameters");
	}
	return 1;
}

static int MultilayerOffsetTrsfToBase(lua_State* L)
{
	int argc = lua_gettop(L);

	if (argc != 12) {
		luaL_argerror(L, argc, "Error number of parameters");
	}
	return 1;
}

static int GetSystemClock(lua_State* L)
{
	int argc = lua_gettop(L);

	if (argc != 0) {
		luaL_argerror(L, argc, "Error number of parameters");
	}
	return 1;
}

static int SocketOpen(lua_State* L)
{
	int argc = lua_gettop(L);

	if (argc != 3) {
		luaL_argerror(L, argc, "Error number of parameters");
	}
	return 1;
}

static int SocketClose(lua_State* L)
{
	int argc = lua_gettop(L);

	if (argc != 1) {
		luaL_argerror(L, argc, "Error number of parameters");
	}
	return 1;
}

static int SocketReadString(lua_State* L)
{
	int argc = lua_gettop(L);

	if (argc != 1) {
		luaL_argerror(L, argc, "Error number of parameters");
	}
	return 1;
}

static int SocketSendString(lua_State* L)
{
	int argc = lua_gettop(L);

	if (argc != 2) {
		luaL_argerror(L, argc, "Error number of parameters");
	}
	return 1;
}

static int SocketReadAsciiFloat(lua_State* L)
{
	int argc = lua_gettop(L);

	if (argc != 2) {
		luaL_argerror(L, argc, "Error number of parameters");
	}
	return 1;
}

static int GetActualJointPosDegree(lua_State* L)
{
	int argc = lua_gettop(L);

	if (argc != 0) {
		luaL_argerror(L, argc, "Error number of parameters");
	}
	return 1;
}

static int GetActualJointPosRadian(lua_State* L)
{
	int argc = lua_gettop(L);

	if (argc != 0) {
		luaL_argerror(L, argc, "Error number of parameters");
	}
	return 1;
}

static int GetActualJointSpeedsDegree(lua_State* L)
{
	int argc = lua_gettop(L);

	if (argc != 0) {
		luaL_argerror(L, argc, "Error number of parameters");
	}
	return 1;
}

static int GetActualJointSpeedsRadian(lua_State* L)
{
	int argc = lua_gettop(L);

	if (argc != 0) {
		luaL_argerror(L, argc, "Error number of parameters");
	}
	return 1;
}

static int GetActualTCPPose(lua_State* L)
{
	int argc = lua_gettop(L);

	if (argc != 0) {
		luaL_argerror(L, argc, "Error number of parameters");
	}
	return 1;
}

static int GetActualTCPNum(lua_State* L)
{
	int argc = lua_gettop(L);

	if (argc != 0) {
		luaL_argerror(L, argc, "Error number of parameters");
	}
	return 1;
}

static int GetActualToolFlangePose(lua_State* L)
{
	int argc = lua_gettop(L);

	if (argc != 0) {
		luaL_argerror(L, argc, "Error number of parameters");
	}
	return 1;
}

static int GetInverseKin(lua_State* L)
{
	int argc = lua_gettop(L);

	if (argc != 8) {
		luaL_argerror(L, argc, "Error number of parameters");
	}
	return 1;
}

static int GetForwardKin(lua_State* L)
{
	int argc = lua_gettop(L);

	if (argc != 6) {
		luaL_argerror(L, argc, "Error number of parameters");
	}
	return 1;
}

static int GetJointTorques(lua_State* L)
{
	int argc = lua_gettop(L);

	if (argc != 0) {
		luaL_argerror(L, argc, "Error number of parameters");
	}
	return 1;
}

static int GetTargetPayload(lua_State* L)
{
	int argc = lua_gettop(L);

	if (argc != 0) {
		luaL_argerror(L, argc, "Error number of parameters");
	}
	return 1;
}

static int GetTargetPayloadCog(lua_State* L)
{
	int argc = lua_gettop(L);

	if (argc != 0) {
		luaL_argerror(L, argc, "Error number of parameters");
	}
	return 1;
}

static int GetTargetTCPPose(lua_State* L)
{
	int argc = lua_gettop(L);

	if (argc != 0) {
		luaL_argerror(L, argc, "Error number of parameters");
	}
	return 1;
}

static int GetTCPOffset(lua_State* L)
{
	int argc = lua_gettop(L);

	if (argc != 0) {
		luaL_argerror(L, argc, "Error number of parameters");
	}
	return 1;
}

static int CalPointDist(lua_State* L)
{
	int argc = lua_gettop(L);

	if (argc != 12) {
		luaL_argerror(L, argc, "Error number of parameters");
	}
	return 1;
}

static int CalPoseAdd(lua_State* L)
{
	int argc = lua_gettop(L);

	if (argc != 12) {
		luaL_argerror(L, argc, "Error number of parameters");
	}
	return 1;
}

static int CalPoseDist(lua_State* L)
{
	int argc = lua_gettop(L);

	if (argc != 12) {
		luaL_argerror(L, argc, "Error number of parameters");
	}
	return 1;
}

static int CalPoseInv(lua_State* L)
{
	int argc = lua_gettop(L);

	if (argc != 6) {
		luaL_argerror(L, argc, "Error number of parameters");
	}
	return 1;
}

static int CalPoseSub(lua_State* L)
{
	int argc = lua_gettop(L);

	if (argc != 12) {
		luaL_argerror(L, argc, "Error number of parameters");
	}
	return 1;
}

static int CalPoseTrans(lua_State* L)
{
	int argc = lua_gettop(L);

	if (argc != 12) {
		luaL_argerror(L, argc, "Error number of parameters");
	}
	return 1;
}

static int pcall_lua(void *arg)
{
	int error;
	int result;

	luaEnv = luaL_newstate();; /* opens Lua */
	luaL_openlibs(luaEnv);

	lua_register(luaEnv, "MoveJ", MoveJ);
	lua_register(luaEnv, "SplinePTP", SplinePTP);
	lua_register(luaEnv, "ExtAxisMoveJ", ExtAxisMoveJ);
	lua_register(luaEnv, "MoveC", MoveC);
	lua_register(luaEnv, "SplineCIRC", SplineCIRC);
	lua_register(luaEnv, "MoveL", MoveL);
	lua_register(luaEnv, "SplineLINE", SplineLINE);
	lua_register(luaEnv, "SetDO", SetDO);
	lua_register(luaEnv, "SetToolDO", SetToolDO);
	lua_register(luaEnv, "GetDI", GetDI);
	lua_register(luaEnv, "GetToolDI", GetToolDI);
	lua_register(luaEnv, "SetAO", SetAO);
	lua_register(luaEnv, "SetToolAO", SetToolAO);
	lua_register(luaEnv, "GetAI", GetAI);
	lua_register(luaEnv, "GetToolAI", GetToolAI);
	lua_register(luaEnv, "WaitDI", WaitDI);
	lua_register(luaEnv, "WaitToolDI", WaitToolDI);
	lua_register(luaEnv, "WaitAI", WaitAI);
	lua_register(luaEnv, "WaitToolAI", WaitToolAI);
	lua_register(luaEnv, "MoveTPD", MoveTPD);
	lua_register(luaEnv, "MoveGripper", MoveGripper);
	lua_register(luaEnv, "SetToolList", SetToolList);
	lua_register(luaEnv, "SetWobjList", SetWobjList);
	lua_register(luaEnv, "SetExToolList", SetExToolList);
	lua_register(luaEnv, "WeaveStart", WeaveStart);
	lua_register(luaEnv, "WeaveEnd", WeaveEnd);
	lua_register(luaEnv, "ARCStart", ARCStart);
	lua_register(luaEnv, "ARCEnd", ARCEnd);
	lua_register(luaEnv, "LTLaserOn", LTLaserOn);
	lua_register(luaEnv, "LTLaserOff", LTLaserOff);
	lua_register(luaEnv, "LTSearchStart", LTSearchStart);
	lua_register(luaEnv, "LTSearchStop", LTSearchStop);
	lua_register(luaEnv, "PostureAdjustOn", PostureAdjustOn);
	lua_register(luaEnv, "PostureAdjustOff", PostureAdjustOff);
	lua_register(luaEnv, "ConveyorIODetect", ConveyorIODetect);
	lua_register(luaEnv, "Mode", Mode);
	//lua_register(luaEnv, "SegmentWeldStart", SegmentWeldStart);
	lua_register(luaEnv, "RegisterVar", RegisterVar);
	lua_register(luaEnv, "LTTrackOn", LTTrackOn);
	lua_register(luaEnv, "LTTrackOff", LTTrackOff);
	lua_register(luaEnv, "LaserSensorRecord", LaserSensorRecord);
	lua_register(luaEnv, "LaserRecordPoint", LaserRecordPoint);
	lua_register(luaEnv, "ExtAxisSetHoming", ExtAxisSetHoming);
	lua_register(luaEnv, "ExtAxisServoOn", ExtAxisServoOn);
	lua_register(luaEnv, "SPLCSetDO", SPLCSetDO);
	lua_register(luaEnv, "SPLCSetToolDO", SPLCSetToolDO);
	lua_register(luaEnv, "SPLCSetAO", SPLCSetAO);
	lua_register(luaEnv, "SPLCSetToolAO", SPLCSetToolAO);
	lua_register(luaEnv, "SprayStart", SprayStart);
	lua_register(luaEnv, "SprayStop", SprayStop);
	lua_register(luaEnv, "PowerCleanStart", PowerCleanStart);
	lua_register(luaEnv, "PowerCleanStop", PowerCleanStop);
	lua_register(luaEnv, "LoadPosSensorDriver", LoadPosSensorDriver);
	lua_register(luaEnv, "UnloadPosSensorDriver", UnloadPosSensorDriver);
	lua_register(luaEnv, "ConveyorGetTrackData", ConveyorGetTrackData);
	lua_register(luaEnv, "ConveyorTrackStart", ConveyorTrackStart);
	lua_register(luaEnv, "ConveyorTrackEnd", ConveyorTrackEnd);
	lua_register(luaEnv, "GetSysVarValue", GetSysVarValue);
	lua_register(luaEnv, "SetSysVarValue", SetSysVarValue);
	lua_register(luaEnv, "ServoJ", ServoJ);
	lua_register(luaEnv, "Pause", Pause);
	lua_register(luaEnv, "Stop", Stop);
	lua_register(luaEnv, "SplineStart", SplineStart);
	lua_register(luaEnv, "SplineEnd", SplineEnd);
	lua_register(luaEnv, "ActGripper", ActGripper);
	lua_register(luaEnv, "FT_Guard", FT_Guard);
	lua_register(luaEnv, "FT_Control", FT_Control);
	lua_register(luaEnv, "MultilayerOffsetTrsfToBase", MultilayerOffsetTrsfToBase);
	lua_register(luaEnv, "GetSystemClock", GetSystemClock);

	/** 机器人 程序示教 尚未实现 */
	lua_register(luaEnv, "StartJOG", StartJOG);
	lua_register(luaEnv, "VMoveJ", VMoveJ);
	lua_register(luaEnv, "VMoveL", VMoveL);
	lua_register(luaEnv, "SPLCGetDI", SPLCGetDI);
	lua_register(luaEnv, "SPLCGetToolDI", SPLCGetToolDI);
	lua_register(luaEnv, "SPLCGetAI", SPLCGetAI);
	lua_register(luaEnv, "SPLCGetToolAI", SPLCGetToolAI);
	lua_register(luaEnv, "WaitMs", WaitMs);
	lua_register(luaEnv, "SetSpeed", SetSpeed);
	lua_register(luaEnv, "SetLoadWeight", SetLoadWeight);
	lua_register(luaEnv, "SetLoadCoord", SetLoadCoord);

	lua_register(luaEnv, "SocketOpen", SocketOpen);
	lua_register(luaEnv, "SocketClose", SocketClose);
	lua_register(luaEnv, "SocketReadString", SocketReadString);
	lua_register(luaEnv, "SocketSendString", SocketSendString);
	lua_register(luaEnv, "SocketReadAsciiFloat", SocketReadAsciiFloat);

	lua_register(luaEnv, "GetActualJointPosDegree", GetActualJointPosDegree);
	lua_register(luaEnv, "GetActualJointPosRadian", GetActualJointPosRadian);
	lua_register(luaEnv, "GetActualJointSpeedsDegree", GetActualJointSpeedsDegree);
	lua_register(luaEnv, "GetActualJointSpeedsRadian", GetActualJointSpeedsRadian);
	lua_register(luaEnv, "GetActualTCPPose", GetActualTCPPose);
	lua_register(luaEnv, "GetActualTCPNum", GetActualTCPNum);
	lua_register(luaEnv, "GetActualToolFlangePose", GetActualToolFlangePose);
	lua_register(luaEnv, "GetInverseKin", GetInverseKin);
	lua_register(luaEnv, "GetForwardKin", GetForwardKin);
	lua_register(luaEnv, "GetJointTorques", GetJointTorques);
	lua_register(luaEnv, "GetTargetPayload", GetTargetPayload);
	lua_register(luaEnv, "GetTargetPayloadCog", GetTargetPayloadCog);
	lua_register(luaEnv, "GetTargetTCPPose", GetTargetTCPPose);
	lua_register(luaEnv, "GetTCPOffset", GetTCPOffset);

	lua_register(luaEnv, "CalPointDist", CalPointDist);
	lua_register(luaEnv, "CalPoseAdd", CalPoseAdd);
	lua_register(luaEnv, "CalPoseDist", CalPoseDist);
	lua_register(luaEnv, "CalPoseInv", CalPoseInv);
	lua_register(luaEnv, "CalPoseSub", CalPoseSub);
	lua_register(luaEnv, "CalPoseTrans", CalPoseTrans);


	//printf("lua_filename = %s\n", lua_filename);

	result = luaL_loadfile(luaEnv, lua_filename);

	printf("result = %d\n", result);

	if (result != LUA_OK) {
		//printf("lua file is not exist or illegal character exists in file\n");
		strcpy(error_info, "lua file is not exist or illegal character exists in file");

		lua_close(luaEnv);
		return FAIL;
	}

	error = lua_pcall(luaEnv, 0, 0, 0);
	//error = lua_pcall (luaEnv, 0, LUA_MULTRET, 0);

	printf("error = %d\n", error);

	if (error != LUA_OK) {
		strcpy(error_info, lua_tostring(luaEnv, -1));

		//fprintf(stderr, "%s", error_info);
		//printf("error_info = %s\n", error_info);
		lua_pop(luaEnv, 1); //pop error message from the stack

		lua_close(luaEnv);
		return FAIL;
	}

	strcpy(error_info, "success");
	lua_close(luaEnv);

	return SUCCESS;
}

static void timeout_break(lua_State* L, lua_Debug* ar)
{
	lua_sethook(L, NULL, 0, 0);
	// 钩子从设置到执行, 需要一段时间, 所以要检测是否仍在执行那个超时的脚本
	luaL_error(L, "timeout");
	//luaL_error(L, "success");
}

int check_lua_file()
{
	pthread_t t_pcall_lua;
	int mask = 0;
	int i = 0;
	//clock_t time_now,  time_begin;

	//time_begin = clock();
	//printf("time_begin = %d\n", time_begin);

	memset(error_info, 0, ERROR_SIZE);

	/* create pcall lua thread */
	if (pthread_create(&t_pcall_lua, NULL, (void *)&pcall_lua, NULL)) {
		perror("pthread_create");
	}

	/* 检测最多 50 ms, 使用 pcall 运行 lua，检测是否有语法错误 */
	for (i = 0; i < 5; i++) {
		if (strcmp(error_info, "") != 0) {
			//printf("before join pcall 1 \n");
			/* 线程挂起, 主线程要等到创建的线程返回了，获取该线程的返回值后主线程才退出 */
			if (pthread_join(t_pcall_lua, NULL)) {
				perror("pthread_join");
			}
			printf("error_info = %s\n", error_info);
			//printf("error_info 1 = %s\n", error_info);

			if (strcmp(error_info, "success") == 0) {

				return SUCCESS;
			} else {

				return FAIL ;
			}
		}
		//printf("i = %d\n", i);

		//delay(100);
		usleep(10000);

		//time_now = clock();
		//printf("time_now = %d\n", time_now);
		//printf("spend time: %lf\n", (double)((time_now - time_begin)/1000));
	}

    mask = LUA_MASKCALL | LUA_MASKRET | LUA_MASKLINE | LUA_MASKCOUNT;
    lua_sethook(luaEnv, timeout_break, mask, 1);

	//printf("before join pcall 2 \n");
	/* 线程挂起, 主线程要等到创建的线程返回了，获取该线程的返回值后主线程才退出 */
	if (pthread_join(t_pcall_lua, NULL)) {
		perror("pthread_join");
	}
	//printf("error_info 2 = %s\n", error_info);

	return SUCCESS;
}
