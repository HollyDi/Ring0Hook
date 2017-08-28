/****************************************************************************************
* Copyright (C) 2015
****************************************************************************************/
#include <ntifs.h>

#ifndef CXX_IDTHook_H
#define CXX_IDTHook_H

#define DEVICE_NAME  L"\\Device\\HookDevice"
#define LINK_NAME    L"\\??\\HookLink"
#define CTL_IDT_HOOK	CTL_CODE(FILE_DEVICE_UNKNOWN, 0x803, METHOD_BUFFERED, FILE_ANY_ACCESS)


typedef struct _DATA_ 
{
	BOOLEAN IsHook;

}DATA,*PDATA;


typedef struct _PROCESSOR_THREAD_PARAMETER
{
	KAFFINITY Mask;
	PKSTART_ROUTINE Routine;
	PVOID Parameter;

} PROCESSOR_THREAD_PARAMETER, *PPROCESSOR_THREAD_PARAMETER;

typedef struct _IDT_ENTRY_X64
{
	USHORT OffsetLow;
	USHORT Selector;
	USHORT BitInfor;
	USHORT OffsetMiddle;
	ULONG  OffsetHigh;
	ULONG  Reserved;
} IDT_ENTRY_X64, *PIDT_ENTRY_X64;


#pragma pack(1)
typedef struct _IDT_ENTRY_X86
{
	USHORT OffsetLow; 
	USHORT Selector;
	ULONG  BitInfor;
	USHORT OffsetHigh;
} IDT_ENTRY_X86, *PIDT_ENTRY_X86;
#pragma pack()

#define IDT_ID 0x2E
void ShowInformation();
void  IDTHook911(PVOID Parameter);
NTSTATUS IDTHook(BOOLEAN IsHook);
void  ThreadCallBack(PVOID ThreadParameter);
ULONG64  FixIDT(UCHAR ID, ULONG64 Address);
void   SplitIDT(ULONG64 Number, ULONG *High, USHORT *Middle, USHORT *Low);
ULONG64  CombineIDT(ULONG High, USHORT Middle, USHORT Low);
VOID FakeKiSystemService();
VOID UnHook();
NTSTATUS
ControlPassThrough(PDEVICE_OBJECT  DeviceObject, PIRP Irp);
VOID
UnloadDriver(PDRIVER_OBJECT DriverObject);

NTSTATUS
DefaultPassThrough(PDEVICE_OBJECT  DeviceObject,PIRP Irp);

#endif