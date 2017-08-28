/****************************************************************************************
* Copyright (C) 2015
****************************************************************************************/
#include <ntifs.h>

#ifndef CXX_EATHook(Ring3)_H
#define CXX_EATHook(Ring3)_H

#define DEVICE_NAME  L"\\Device\\HookDevice"
#define LINK_NAME    L"\\??\\HookLink"


#define CTL_EAT_HOOK_RING3 \
	CTL_CODE(FILE_DEVICE_UNKNOWN,0x810,METHOD_BUFFERED,FILE_ANY_ACCESS)


typedef struct _DATA_
{
	BOOLEAN IsHook;
	ULONG   ProcessID;
	PVOID   VirtualAddress;
}DATA, *PDATA;
#define PTE_SHIFT 3
#define _HARDWARE_PTE_WORKING_SET_BITS  11
#define VIRTUAL_ADDRESS_BITS 48
#define VIRTUAL_ADDRESS_MASK ((((ULONG_PTR)1) << VIRTUAL_ADDRESS_BITS) - 1)
typedef struct _MMPTE_HARDWARE {
	ULONGLONG Valid : 1;
	ULONGLONG Dirty1 : 1;      
	ULONGLONG Owner : 1;
	ULONGLONG WriteThrough : 1;
	ULONGLONG CacheDisable : 1;
	ULONGLONG Accessed : 1;
	ULONGLONG Dirty : 1;
	ULONGLONG LargePage : 1;
	ULONGLONG Global : 1;
	ULONGLONG CopyOnWrite : 1; 
	ULONGLONG Unused : 1;  
	ULONGLONG Write : 1;  
	ULONGLONG PageFrameNumber : 28;
	ULONG64 Reserved1 : 24 - (_HARDWARE_PTE_WORKING_SET_BITS + 1);
	ULONGLONG SoftwareWsIndex : _HARDWARE_PTE_WORKING_SET_BITS;
	ULONG64 NoExecute : 1;
} MMPTE_HARDWARE, *PMMPTE_HARDWARE;

typedef struct _MMPTE {
	union {
		MMPTE_HARDWARE Hard;
	} u;
} MMPTE,*PMMPTE;

#define SETBIT(x,y) x|=(1<<y) //将X的第Y位置1
#define CLRBIT(x,y) x&=~(1<<y) //将X的第Y位清0
#define GETBIT(x,y) (x & (1 << y)) //取X的第Y位，返回0或非0
#define READ_PA 1
#define WRITE_PA 0
BOOLEAN ReadWritePhyAddr(PHYSICAL_ADDRESS
	PhysicalAddress, SIZE_T BufferLength, PVOID BufferData, BOOLEAN IsRead);

#define MiGetPteAddress(VirtualAddress) ((PMMPTE)(((((ULONG_PTR)(VirtualAddress) & VIRTUAL_ADDRESS_MASK) >> PTI_SHIFT) << PTE_SHIFT) + PTE_BASE))
VOID
UnloadDriver(PDRIVER_OBJECT DriverObject);

NTSTATUS
DefaultPassThrough(PDEVICE_OBJECT  DeviceObject,PIRP Irp);

NTSTATUS
ControlPassThrough(PDEVICE_OBJECT  DeviceObject, PIRP Irp);
#endif