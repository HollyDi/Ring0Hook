/****************************************************************************************
* Copyright (C) 2015
****************************************************************************************/
#include "EATHook(Ring3).h"

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

	case CTL_EAT_HOOK_RING3:
	{
		if (InputLength >= sizeof(DATA) && InputBuffer)
			Irp->IoStatus.Status = EATHookRing3((DATA*)InputBuffer);
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

NTSTATUS
DefaultPassThrough(PDEVICE_OBJECT  DeviceObject,PIRP Irp)
{
	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp,IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}


NTSTATUS EATHookRing3(PDATA Data)
{
	KAPC_STATE Apc;
	NTSTATUS Status;
	PEPROCESS EProcess = NULL;
	PMMPTE PointerPte;
	UCHAR v1 = 0;
	PHYSICAL_ADDRESS PhysicalAddress = { 0 };
	Status = PsLookupProcessByProcessId((HANDLE)Data->ProcessID, &EProcess);

	if (NT_SUCCESS(Status))
	{
		KeStackAttachProcess(EProcess, &Apc);   //切换进程上下背景文

		DbgPrint("%p\r\n", Data->VirtualAddress);
		PointerPte = MiGetPteAddress(Data->VirtualAddress);

		PhysicalAddress = MmGetPhysicalAddress(PointerPte); 
		DbgPrint("PTE PA: %p\n", PhysicalAddress.QuadPart);
	
		
		
		if (Data->IsHook)
		{
			ReadWritePhyAddr(PhysicalAddress, 1, &v1, READ_PA); 
			DbgPrint("Byte before modify: %x\n", v1);

			SETBIT(v1, 1);

			ReadWritePhyAddr(PhysicalAddress, 1, &v1, WRITE_PA); 
			DbgPrint("Byte after modify: %x\n", v1);
			UCHAR v1[8] = { 0 };
			ReadWritePhyAddr(MmGetPhysicalAddress(Data->VirtualAddress), 8, v1, READ_PA);
			v1[0] = 0xC3;
			ReadWritePhyAddr(MmGetPhysicalAddress(Data->VirtualAddress), 8, v1,
				WRITE_PA);
		}
		else
		{

		}
		KeUnstackDetachProcess(&Apc);
	}
	else
	{
		DbgPrint("PsLookupProcessByProcessId Failed\r\n");
	}

	if (EProcess)
		ObDereferenceObject(EProcess);

	return Status;
}

BOOLEAN ReadWritePhyAddr(PHYSICAL_ADDRESS 
	PhysicalAddress, SIZE_T BufferLength, PVOID BufferData, BOOLEAN IsRead)
{
	PVOID VirtualAddress = MmMapIoSpace(PhysicalAddress, BufferLength, 0);
	if (!MmIsAddressValid(VirtualAddress))
		return FALSE;
	if (IsRead)
	{
		memcpy(BufferData, VirtualAddress, BufferLength);	
	}
	else
	{
		memcpy(VirtualAddress, BufferData, BufferLength);	
	}
	MmUnmapIoSpace(VirtualAddress, BufferLength);
	return TRUE;
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
	DbgPrint("EATHook(Ring3) IS STOPPED!!!");
}