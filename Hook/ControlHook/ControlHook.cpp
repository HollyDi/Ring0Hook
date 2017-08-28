// ControlHook.cpp : 定义控制台应用程序的入口点。
//

#include "ControlHook.h"
#include <iostream>
#include <windows.h>

using namespace std;

int main()
{
	//IrpHook();
	IDTHook();
    return 0;
}

BOOL IDTHook()
{
	HANDLE DeviceHandle = NULL;
	DeviceHandle = OpenDevice(L"\\\\.\\HookLink");
	if (DeviceHandle == (HANDLE)-1)
	{
		return FALSE;
	}
	DATA  Data;
	Data.IsHook = 1;
	SendIoControlCode(DeviceHandle, Data, CTL_IDT_HOOK);
	printf("Input Any To UnHook\r\n");
	getchar();
	getchar();
	Data.IsHook = 0;
	SendIoControlCode(DeviceHandle, Data, CTL_IDT_HOOK);
	CloseHandle(DeviceHandle);
	return TRUE;
}

BOOL  IrpHook()
{
	
	HANDLE DeviceHandle = NULL;
	DeviceHandle = OpenDevice(L"\\\\.\\HookLink");
	if (DeviceHandle == (HANDLE)-1)
	{
		return FALSE;
	}
	DATA  Data;
	Data.IsHook = 1;
	SendIoControlCode(DeviceHandle, Data, CTL_IRP_HOOK);
	printf("Input Any To UnHook\r\n");
	getchar();
	getchar();
	Data.IsHook = 0;
	SendIoControlCode(DeviceHandle, Data, CTL_IRP_HOOK);
	CloseHandle(DeviceHandle);


	return TRUE;
}



HANDLE
OpenDevice(LPCTSTR LinkPath)
{
	HANDLE DeviceHandle = CreateFile(LinkPath,
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);

	if (DeviceHandle == INVALID_HANDLE_VALUE)
	{
		cout << "Open Device Error" << endl;

		return (HANDLE)-1;
	}

	return DeviceHandle;
}


BOOL
SendIoControlCode(HANDLE DeviceHandle, DATA  Data, ULONG IoControlCode)
{
	DWORD ReturnLength = 0;
	BOOL  bRet = 0;
	bRet = DeviceIoControl(DeviceHandle, IoControlCode,
		&Data,
		sizeof(DATA),
		NULL,
		0,
		&ReturnLength,
		NULL);
	if (bRet == 0)
	{
		cout << "Send IoCode Error" << endl;
		return FALSE;
	}
	return TRUE;
}



