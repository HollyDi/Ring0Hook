/****************************************************************************************
* Copyright (C) 2015
****************************************************************************************/
#include "ObjectHook.h"

extern POBJECT_TYPE *IoDeviceObjectType;

pfnParseProcedure OldParseProcedure = NULL;
BOOLEAN bOk = FALSE;
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

	DriverObject->DriverUnload = UnloadDriver;

	if (ObjectHook(NULL)==STATUS_SUCCESS)
	{
		bOk = TRUE;
	}
	
	return STATUS_SUCCESS;
}

NTSTATUS ObjectHook(PVOID FunctionAddress)
{
	POBJECT_TYPE ObjectType = NULL;
	ObjectType = *IoDeviceObjectType;


	//在这里可以使用两种方法获得该对象类型的地址
	DbgPrint("%p\r\n", *IoDeviceObjectType);
	DbgPrint("%p\r\n", *((POBJECT_TYPE*)(GetNtosFunctionAddress(L"IoDeviceObjectType"))));

	if (!FunctionAddress)
	{
		OldParseProcedure = ObjectType->TypeInfo.ParseProcedure;
		if (MmIsAddressValid(OldParseProcedure))
		{
			ObjectType->TypeInfo.ParseProcedure = FakeParseProcedure;


			return STATUS_SUCCESS;
		}
	}
	else
	{
		ObjectType->TypeInfo.ParseProcedure = FunctionAddress;

		return STATUS_SUCCESS;
	}

	return STATUS_UNSUCCESSFUL;
}




NTSTATUS __fastcall FakeParseProcedure(PVOID RootDirectory, POBJECT_TYPE ObjectType, PACCESS_STATE AccessState,
	KPROCESSOR_MODE AccessCheckMode, ULONG Attributes, PUNICODE_STRING ObjectName,
	PUNICODE_STRING RemainingName, //原来程序的路径
	PVOID ParseContext,
	PSECURITY_QUALITY_OF_SERVICE SecurityQos,
	PVOID *Object)
{
	WCHAR v1[260] = L"\\Text.txt";     //这里是文件路径不包含盘符
	UNICODE_STRING v2;
	UNICODE_STRING v3;
	char *ImageFileName = NULL;

	//参数无效的话，就不进行过滤！
	if (!ARGUMENT_PRESENT(RemainingName)) {
		goto _FunctionRet;
	}
	//进一步效验是否可以访问
	if (ValidateUnicodeString(RemainingName))
	{
		RtlInitUnicodeString(&v2, v1);

		DbgPrint("%wZ\r\n", RemainingName);
		if (RtlCompareUnicodeString(RemainingName, &v2, TRUE) == 0)
		{
			DbgPrint("%wZ\r\n", RemainingName);

			ImageFileName = (char *)PsGetProcessImageFileName(PsGetCurrentProcess());
			if (_stricmp(ImageFileName, "explorer.exe") == 0) //排除一下这个进程
			{
				goto _FunctionRet;
			}
			//替换文件路径
			RtlInitUnicodeString(&v3, L"\\Text1.txt");
			return OldParseProcedure(
				RootDirectory,
				ObjectType,
				AccessState,
				AccessCheckMode,
				Attributes,
				ObjectName,
				&v3,
				ParseContext,
				SecurityQos,
				Object);
		}
	}
_FunctionRet:
	return OldParseProcedure(
		RootDirectory,
		ObjectType,
		AccessState,
		AccessCheckMode,
		Attributes,
		ObjectName,
		RemainingName,
		ParseContext,
		SecurityQos,
		Object);
}

BOOLEAN ValidateUnicodeString(PUNICODE_STRING UnicodeString)
{
	ULONG i;

	__try
	{
		if (!MmIsAddressValid(UnicodeString))
		{
			return FALSE;
		}
		if (UnicodeString->Buffer == NULL || UnicodeString->Length == 0)
		{
			return FALSE;
		}
		for (i = 0; i < UnicodeString->Length; i++)
		{
			if (!MmIsAddressValid((PUCHAR)UnicodeString->Buffer + i))
			{
				return FALSE;
			}
		}

	}
	__except (EXCEPTION_EXECUTE_HANDLER) {

	}
	return TRUE;
}


SIZE_T GetNtosFunctionAddress(PCWSTR FunctionName)
{
	UNICODE_STRING UnicodeFunctionName;
	RtlInitUnicodeString(&UnicodeFunctionName, FunctionName);
	return (SIZE_T)MmGetSystemRoutineAddress(&UnicodeFunctionName);
}

NTSTATUS
DefaultPassThrough(PDEVICE_OBJECT  DeviceObject,PIRP Irp)
{
	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp,IO_NO_INCREMENT);

	return STATUS_SUCCESS;
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

	if (ObjectHook(OldParseProcedure)==STATUS_SUCCESS)
	{
		bOk = FALSE;
	}
	DbgPrint("ObjectHook IS STOPPED!!!");
}