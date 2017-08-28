/****************************************************************************************
* Copyright (C) 2015
****************************************************************************************/
#include <ntifs.h>

#ifndef CXX_IrpHook_H
#define CXX_IrpHook_H

#define DEVICE_NAME  L"\\Device\\HookDevice"
#define LINK_NAME    L"\\??\\HookLink"


#define CTL_IRP_HOOK \
	CTL_CODE(FILE_DEVICE_UNKNOWN,0x830,METHOD_BUFFERED,FILE_ANY_ACCESS)

typedef struct _DATA_
{
	BOOLEAN IsHook;
}DATA, *PDATA;


VOID
UnloadDriver(PDRIVER_OBJECT DriverObject);

NTSTATUS
DefaultPassThrough(PDEVICE_OBJECT  DeviceObject,PIRP Irp);

NTSTATUS
ControlPassThrough(PDEVICE_OBJECT  DeviceObject, PIRP Irp);


NTSTATUS IrpHook(BOOLEAN IsHook);

NTKERNELAPI NTSTATUS ObReferenceObjectByName
(
	IN PUNICODE_STRING ObjectName,
	IN ULONG Attributes,
	IN PACCESS_STATE PassedAccessState OPTIONAL,
	IN ACCESS_MASK DesiredAccess OPTIONAL,
	IN POBJECT_TYPE ObjectType,
	IN KPROCESSOR_MODE AccessMode,
	IN OUT PVOID ParseContext OPTIONAL,
	OUT PVOID *Object
);

typedef NTSTATUS(__fastcall *pfnProcedure)(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS __fastcall FakeWriteProcedure(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS __fastcall FakeReadProcedure(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS  __fastcall FakeSetInformationProcedure(PDEVICE_OBJECT DeviceObject, PIRP Irp);
PDRIVER_OBJECT GetDriverObject(PCWSTR DriverObjectLinkName);
#endif