/****************************************************************************************
* Copyright (C) 2015
****************************************************************************************/
#include <ntifs.h>
#ifndef CXX_ObjectHook_H
#define CXX_ObjectHook_H

#define DEVICE_NAME  L"\\Device\\HookDevice"
#define LINK_NAME    L"\\??\\HookLink"


/*
	kd > dt _OBJECT_TYPE_INITIALIZER
	dtx is unsupported for this scenario.It only recognizes dtx[<type>][<address>] with - a, -h, and -r.Reverting to dt.
	ntdll!_OBJECT_TYPE_INITIALIZER
	+ 0x000 Length           : Uint2B
	+ 0x002 ObjectTypeFlags : UChar
	+ 0x002 CaseInsensitive : Pos 0, 1 Bit
	+ 0x002 UnnamedObjectsOnly : Pos 1, 1 Bit
	+ 0x002 UseDefaultObject : Pos 2, 1 Bit
	+ 0x002 SecurityRequired : Pos 3, 1 Bit
	+ 0x002 MaintainHandleCount : Pos 4, 1 Bit
	+ 0x002 MaintainTypeList : Pos 5, 1 Bit
	+ 0x002 SupportsObjectCallbacks : Pos 6, 1 Bit
	+ 0x004 ObjectTypeCode : Uint4B
	+ 0x008 InvalidAttributes : Uint4B
	+ 0x00c GenericMapping : _GENERIC_MAPPING
	+ 0x01c ValidAccessMask : Uint4B
	+ 0x020 RetainAccess : Uint4B
	+ 0x024 PoolType : _POOL_TYPE
	+ 0x028 DefaultPagedPoolCharge : Uint4B
	+ 0x02c DefaultNonPagedPoolCharge : Uint4B
	+ 0x030 DumpProcedure : Ptr64     void
	+ 0x038 OpenProcedure : Ptr64     long
	+ 0x040 CloseProcedure : Ptr64     void
	+ 0x048 DeleteProcedure : Ptr64     void
	+ 0x050 ParseProcedure : Ptr64     long
	+ 0x058 SecurityProcedure : Ptr64     long
	+ 0x060 QueryNameProcedure : Ptr64     long
	+ 0x068 OkayToCloseProcedure : Ptr64     unsigned char
*/
typedef struct _OBJECT_TYPE_INITIALIZER {
	USHORT Length;
	BOOLEAN ObjectTypeFlags;
	BOOLEAN CaseInsensitive;
	ULONG ObjectTypeCode;
	ULONG InvalidAttributes;
	GENERIC_MAPPING GenericMapping;
	ULONG ValidAccessMask;
	ULONG RetainAccess;
	POOL_TYPE PoolType;
	ULONG DefaultPagedPoolCharge;
	ULONG DefaultNonPagedPoolCharge;
	PVOID DumpProcedure;
	PVOID OpenProcedure;
	PVOID CloseProcedure;
	PVOID DeleteProcedure;
	PVOID ParseProcedure;
	PVOID SecurityProcedure;
	PVOID QueryNameProcedure;
	PVOID OkayToCloseProcedure;
} OBJECT_TYPE_INITIALIZER, *POBJECT_TYPE_INITIALIZER;
/*
	kd> dt _object_type
	dtx is unsupported for this scenario.  It only recognizes dtx [<type>] [<address>] with -a, -h, and -r.  Reverting to dt.
	ntdll!_OBJECT_TYPE
	+0x000 TypeList         : _LIST_ENTRY
	+0x010 Name             : _UNICODE_STRING
	+0x020 DefaultObject    : Ptr64 Void
	+0x028 Index            : UChar
	+0x02c TotalNumberOfObjects : Uint4B
	+0x030 TotalNumberOfHandles : Uint4B
	+0x034 HighWaterNumberOfObjects : Uint4B
	+0x038 HighWaterNumberOfHandles : Uint4B
	+0x040 TypeInfo         : _OBJECT_TYPE_INITIALIZER
	+0x0b0 TypeLock         : _EX_PUSH_LOCK
	+0x0b8 Key              : Uint4B
	+0x0c0 CallbackList     : _LIST_ENTRY
*/

typedef struct _OBJECT_TYPE {
	LIST_ENTRY TypeList;
	UNICODE_STRING Name;
	PVOID DefaultObject;
	ULONG Index;
	ULONG TotalNumberOfObjects;
	ULONG TotalNumberOfHandles;
	ULONG HighWaterNumberOfObjects;
	ULONG HighWaterNumberOfHandles;
	OBJECT_TYPE_INITIALIZER TypeInfo;
	ULONG64 TypeLock;
	ULONG   Key;
	LIST_ENTRY CallBackList;
} OBJECT_TYPE, *POBJECT_TYPE;


VOID
UnloadDriver(PDRIVER_OBJECT DriverObject);

NTSTATUS
DefaultPassThrough(PDEVICE_OBJECT  DeviceObject,PIRP Irp);

NTSTATUS
ControlPassThrough(PDEVICE_OBJECT  DeviceObject, PIRP Irp);

BOOLEAN ValidateUnicodeString(PUNICODE_STRING UnicodeString);

typedef NTSTATUS(__fastcall *pfnParseProcedure) (
	PVOID RootDirectory,
	POBJECT_TYPE ObjectType,
	PACCESS_STATE AccessState,
	KPROCESSOR_MODE AccessCheckMode,
	ULONG Attributes,
	PUNICODE_STRING ObjectName,
	PUNICODE_STRING RemainingName,
	PVOID ParseContext,
	PSECURITY_QUALITY_OF_SERVICE SecurityQos,
	PVOID *Object);


NTSTATUS __fastcall FakeParseProcedure(PVOID RootDirectory, POBJECT_TYPE ObjectType, PACCESS_STATE AccessState,
	KPROCESSOR_MODE AccessCheckMode, ULONG Attributes, PUNICODE_STRING ObjectName,
	PUNICODE_STRING RemainingName, //原来程序的路径
	PVOID ParseContext,
	PSECURITY_QUALITY_OF_SERVICE SecurityQos,
	PVOID *Object);
UCHAR *PsGetProcessImageFileName(IN PEPROCESS Process);
SIZE_T GetNtosFunctionAddress(PCWSTR FunctionName);
#endif