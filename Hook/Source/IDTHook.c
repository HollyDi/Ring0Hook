/****************************************************************************************
* Copyright (C) 2015
****************************************************************************************/
#include "IDTHook.h"
SIZE_T  OldKiSystemService  = 0;





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

	case CTL_IDT_HOOK:
	{
		if (InputLength >= sizeof(DATA) && InputBuffer)
			Irp->IoStatus.Status = IDTHook(((DATA*)InputBuffer)->IsHook);
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

NTSTATUS IDTHook(BOOLEAN IsHook)
{

	KAFFINITY ActiveProcessors;
	KAFFINITY Mask;
	KAFFINITY i;
	HANDLE ThreadHandle = NULL;
	PROCESSOR_THREAD_PARAMETER ThreadParameter;
	NTSTATUS Status;
	PVOID EThread;
	if (KeGetCurrentIrql() > PASSIVE_LEVEL)
	{
		return STATUS_UNSUCCESSFUL;
	}
	// get bitmask of active processors
	ActiveProcessors = KeQueryActiveProcessors();

	for (i = 0; i < sizeof(KAFFINITY) * 8; i++)  //IDT Hook   VT  --->
	{
	
		Mask = 1i64 << i;     

		// check if this processor bit present in mask
		if (ActiveProcessors & Mask)
		{
			ThreadParameter.Mask = Mask;
			ThreadParameter.Parameter = (PVOID)IsHook;
			ThreadParameter.Routine = IDTHook911;    
			// create thread for this processor
			Status = PsCreateSystemThread(&ThreadHandle,
				THREAD_ALL_ACCESS, NULL, NULL, NULL,ThreadCallBack, &ThreadParameter);
			if (NT_SUCCESS(Status))
			{
				// get pointer to thread object
				Status = ObReferenceObjectByHandle(ThreadHandle, THREAD_ALL_ACCESS, NULL, KernelMode, &EThread, NULL);
				if (NT_SUCCESS(Status))
				{
					// waiting for thread termination
					KeWaitForSingleObject(EThread, Executive, KernelMode, FALSE, NULL);
					ObDereferenceObject(EThread);
				}
				else
				{
					DbgPrint("ObReferenceObjectByHandle() Fails; Status: 0x%.8x\n", Status);

					return Status;
				}
				ZwClose(ThreadHandle);

				return Status;
			}
			else
			{
				DbgPrint("PsCreateSystemThread() Fails; Status: 0x%.8x\n", Status);

				return Status;
			}
		}
	}

}

void ThreadCallBack(PVOID ThreadParameter)
{
	PPROCESSOR_THREAD_PARAMETER v1 = (PPROCESSOR_THREAD_PARAMETER)ThreadParameter;
	// bind thread to specific processor
	KeSetSystemAffinityThread(v1->Mask);   //
	// execute payload on this processor
	v1->Routine(v1->Parameter);
}


NTSTATUS
DefaultPassThrough(PDEVICE_OBJECT  DeviceObject,PIRP Irp)
{
	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp,IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}


void IDTHook911(PVOID Parameter)
{
	if (Parameter == TRUE)
	{
		DbgPrint("IDTHook911\r\n");
		OldKiSystemService = FixIDT(IDT_ID, (ULONG64)FakeKiSystemService);
	}
	else
	{
		FixIDT(IDT_ID, (ULONG64)OldKiSystemService);
	}
}
ULONG64 FixIDT(UCHAR ID, ULONG64 FakeAddress)
{
	ULONG64  OldAddress = 0;
	ULONG High = 0;
	USHORT Middle = 0, Low = 0;
	PIDT_ENTRY_X64 TargetAddress;
	PUCHAR v1, v2;
	v1 = (PUCHAR)__readmsr(0xC0000101) + 0x38;
	v2 = (PUCHAR)(*(ULONG64*)v1);
	TargetAddress = (PIDT_ENTRY_X64)(v2 + ID * 16);
	SplitIDT(FakeAddress, &High, &Middle, &Low);
	OldAddress = CombineIDT(TargetAddress->OffsetHigh, TargetAddress->OffsetMiddle, TargetAddress->OffsetLow);
	_disable();
	TargetAddress->OffsetLow = Low;
	TargetAddress->OffsetMiddle = Middle;
	TargetAddress->OffsetHigh = High;
	_enable();
	return OldAddress;
}

void SplitIDT(ULONG64 Number, ULONG *High, USHORT *Middle, USHORT *Low)
{
	memcpy(High, ((PUCHAR)&Number) + 4, 4);
	memcpy(Middle, ((PUCHAR)&Number) + 2, 2);
	memcpy(Low, ((PUCHAR)&Number), 2);
}

ULONG64 CombineIDT(ULONG High, USHORT Middle, USHORT Low)
{
	ULONG64 Number;
	memcpy(((PUCHAR)&Number) + 4, &High, 4);
	memcpy(((PUCHAR)&Number) + 2, &Middle, 2);
	memcpy(((PUCHAR)&Number), &Low, 2);
	return Number;
}
VOID UnHook()
{
	if (OldKiSystemService != NULL)
	{
		FixIDT(IDT_ID, (ULONG64)OldKiSystemService);
	}
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
	
	DbgPrint("IDTHook IS STOPPED!!!");
}


void ShowInformation()
{
	DbgPrint("Fake!!\n");
}