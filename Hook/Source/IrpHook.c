/****************************************************************************************
* Copyright (C) 2015
****************************************************************************************/
#include "IrpHook.h"


extern POBJECT_TYPE *IoDriverObjectType;
PDRIVER_OBJECT NtfsDriverObject = NULL;
pfnProcedure OldReadProcedure  = NULL;
pfnProcedure OldWriteProcedure = NULL;
pfnProcedure OldSetInformationProcedure = NULL;
NTSTATUS
DriverEntry(PDRIVER_OBJECT  DriverObject,PUNICODE_STRING  RegisterPath)
{
	PDEVICE_OBJECT  DeviceObject;
	NTSTATUS        Status;
	int             i = 0;

	UNICODE_STRING  DeviceName;
	UNICODE_STRING  LinkName;

	RtlInitUnicodeString(&DeviceName,DEVICE_NAME);
	RtlInitUnicodeString(&LinkName,LINK_NAME);

	//创建设备对象;

	Status = IoCreateDevice(DriverObject,0,
	&DeviceName,FILE_DEVICE_UNKNOWN,0,FALSE,&DeviceObject);
	if (!NT_SUCCESS(Status))
	{
		return Status;
	}

	Status = IoCreateSymbolicLink(&LinkName,&DeviceName);

	for (i = 0; i<IRP_MJ_MAXIMUM_FUNCTION; i++)
	{
		DriverObject->MajorFunction[i] = DefaultPassThrough;
	}

	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = ControlPassThrough;
	DriverObject->DriverUnload = UnloadDriver;



	
	
	return STATUS_SUCCESS;
}

NTSTATUS
ControlPassThrough(PDEVICE_OBJECT  DeviceObject, PIRP Irp)
{
	
	NTSTATUS  Status = STATUS_SUCCESS;
	PVOID     InputBuffer = NULL;
	PVOID     OutputBuffer = NULL;
	ULONG_PTR InputLength = 0;
	ULONG_PTR OutputLength = 0;
	ULONG_PTR IoControlCode = 0;
	PIO_STACK_LOCATION   IrpStack = NULL;

	IrpStack = IoGetCurrentIrpStackLocation(Irp);
	InputBuffer = Irp->AssociatedIrp.SystemBuffer;
	OutputBuffer = Irp->AssociatedIrp.SystemBuffer;
	InputLength = IrpStack->Parameters.DeviceIoControl.InputBufferLength;
	OutputLength = IrpStack->Parameters.DeviceIoControl.OutputBufferLength;
	IoControlCode = IrpStack->Parameters.DeviceIoControl.IoControlCode;


	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;

	switch (IoControlCode)
	{

	case CTL_IRP_HOOK:
	{
		if (InputLength >= sizeof(DATA) && InputBuffer)
			Irp->IoStatus.Status = IrpHook(((DATA*)InputBuffer)->IsHook);
		else
			Irp->IoStatus.Status = STATUS_INFO_LENGTH_MISMATCH;

		break;
	}

	default:
	{
		Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
		break;
	}
	}

	Status = Irp->IoStatus.Status;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return Status;
}


VOID UnHook()
{
	if (OldReadProcedure!=NULL&&OldWriteProcedure!=NULL)
	{
		NtfsDriverObject->MajorFunction[IRP_MJ_READ] = (pfnProcedure)OldReadProcedure;
		NtfsDriverObject->MajorFunction[IRP_MJ_WRITE] = (pfnProcedure)OldWriteProcedure;
		NtfsDriverObject->MajorFunction[IRP_MJ_SET_INFORMATION] = (pfnProcedure)OldSetInformationProcedure;
	}
}

NTSTATUS IrpHook(BOOLEAN IsHook)
{
	WCHAR v1[] = L"\\FileSystem\\Ntfs";
	NtfsDriverObject = GetDriverObject(v1);
	DbgPrint("DISK DriverObject: %p\n", v1);


	if (IsHook)
	{
		OldReadProcedure  = NtfsDriverObject->MajorFunction[IRP_MJ_READ];
		OldWriteProcedure = NtfsDriverObject->MajorFunction[IRP_MJ_WRITE];
		OldSetInformationProcedure = NtfsDriverObject->MajorFunction[IRP_MJ_SET_INFORMATION];
		NtfsDriverObject->MajorFunction[IRP_MJ_READ] = (pfnProcedure)FakeReadProcedure;
		NtfsDriverObject->MajorFunction[IRP_MJ_WRITE] = (pfnProcedure)FakeWriteProcedure;
		NtfsDriverObject->MajorFunction[IRP_MJ_SET_INFORMATION] = (pfnProcedure)FakeSetInformationProcedure;
	}
	else
	{
		NtfsDriverObject->MajorFunction[IRP_MJ_READ] = (pfnProcedure)OldReadProcedure;
		NtfsDriverObject->MajorFunction[IRP_MJ_WRITE] = (pfnProcedure)OldWriteProcedure;
		NtfsDriverObject->MajorFunction[IRP_MJ_SET_INFORMATION] = (pfnProcedure)OldSetInformationProcedure;
	}
}

PDRIVER_OBJECT GetDriverObject(PCWSTR DriverObjectLinkName)
{
	NTSTATUS                      Status = STATUS_SUCCESS;
	PDRIVER_OBJECT                DriverObject = NULL;
	UNICODE_STRING                LinkName;
	RtlInitUnicodeString(&LinkName, DriverObjectLinkName);



	Status = ObReferenceObjectByName(
		&LinkName,
		OBJ_CASE_INSENSITIVE,
		NULL,
		0,
		*IoDriverObjectType,
		KernelMode,
		NULL,
		(PVOID*)&DriverObject);
	if (!NT_SUCCESS(Status))
		return NULL;
	else
		return DriverObject;
}


NTSTATUS
DefaultPassThrough(PDEVICE_OBJECT  DeviceObject,PIRP Irp)
{
	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp,IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}



NTSTATUS __fastcall FakeReadProcedure(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	NTSTATUS Status = STATUS_SUCCESS;
	IO_STACK_LOCATION *IrpSp = IoGetCurrentIrpStackLocation(Irp);
	LARGE_INTEGER ReadBytesOffset = IrpSp->Parameters.Read.ByteOffset;
	ULONG ReadLength = IrpSp->Parameters.Read.Length;
	return OldReadProcedure(DeviceObject, Irp);
}


NTSTATUS  __fastcall FakeSetInformationProcedure(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	NTSTATUS              Status;
	PIO_STACK_LOCATION    IrpSp;
	PFILE_OBJECT          FileObject = NULL;
	WCHAR*                v1 = NULL;



	IrpSp = IoGetCurrentIrpStackLocation(Irp);

	FileObject = IrpSp->FileObject;
	__try
	{
		v1 = wcsrchr(FileObject->FileName.Buffer,L'\\');
	if (v1 != NULL)
	{
		v1++;
	}

	if (!_wcsnicmp(v1,L"Text.txt",wcslen(L"Text.txt") * 2))
	{
		if ((IrpSp->Parameters.SetFile.FileInformationClass == FileRenameInformation))
		{
			Irp->IoStatus.Status = STATUS_SUCCESS;
			Irp->IoStatus.Information = 0;
			IofCompleteRequest(Irp, 0);
			return STATUS_ACCESS_DENIED;
		}
	}

	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		goto _FunctionRet;
	}

_FunctionRet:
	return OldSetInformationProcedure(DeviceObject, Irp);
}





NTSTATUS __fastcall FakeWriteProcedure(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	NTSTATUS Status = STATUS_SUCCESS;
	IO_STACK_LOCATION *IrpSp = IoGetCurrentIrpStackLocation(Irp);
	LARGE_INTEGER WriteBytesOffset = IrpSp->Parameters.Write.ByteOffset;
	ULONG WriteLength = IrpSp->Parameters.Write.Length;
	return OldWriteProcedure(DeviceObject, Irp);
}


VOID
UnloadDriver(PDRIVER_OBJECT DriverObject)
{
	UNICODE_STRING  LinkName;
	PDEVICE_OBJECT	NextDeviceObject    = NULL;
	PDEVICE_OBJECT  CurrentDeviceObject = NULL;
	RtlInitUnicodeString(&LinkName,LINK_NAME);

	IoDeleteSymbolicLink(&LinkName);
	CurrentDeviceObject = DriverObject->DeviceObject;
	while (CurrentDeviceObject != NULL) 
	{
	
		NextDeviceObject = CurrentDeviceObject->NextDevice;
		IoDeleteDevice(CurrentDeviceObject);
		CurrentDeviceObject = NextDeviceObject;
	}

	UnHook();
	DbgPrint("IrpHook IS STOPPED!!!");
}