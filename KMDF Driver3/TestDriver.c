#include <ntifs.h>
#include <wdf.h>
#include <ntstrsafe.h>

#define PIPE_NAME L"\\DosDevices\\pipe\\MyProcessMonitorPipe"

#ifndef PROCESS_QUERY_LIMITED_INFORMATION
#define PROCESS_QUERY_LIMITED_INFORMATION 0x1000
#endif

extern NTSTATUS ZwQueryInformationProcess(
    HANDLE ProcessHandle,
    ULONG ProcessInformationClass,
    PVOID ProcessInformation,
    ULONG ProcessInformationLength,
    PULONG ReturnLength
);

HANDLE g_NamedPipe = NULL;

NTSTATUS ConnectToNamedPipe() {
    UNICODE_STRING pipeName;
    OBJECT_ATTRIBUTES objAttr;
    IO_STATUS_BLOCK ioStatus;

    RtlInitUnicodeString(&pipeName, PIPE_NAME);
    InitializeObjectAttributes(&objAttr, &pipeName, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);

    NTSTATUS status = ZwCreateFile(&g_NamedPipe, GENERIC_WRITE, &objAttr, &ioStatus, NULL, FILE_ATTRIBUTE_NORMAL,
        0, FILE_OPEN_IF, FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0);

    if (!NT_SUCCESS(status)) {
        g_NamedPipe = NULL;
    }
    return status;
}

VOID SendToUserMode(WCHAR* message) {
    if (g_NamedPipe == NULL) {
        if (!NT_SUCCESS(ConnectToNamedPipe())) return;
    }

    IO_STATUS_BLOCK ioStatus;
    SIZE_T length = (wcslen(message) + 1) * sizeof(WCHAR);
    NTSTATUS status = ZwWriteFile(g_NamedPipe, NULL, NULL, NULL, &ioStatus, message, (ULONG)length, NULL, NULL);
    if (!NT_SUCCESS(status)) {
        ZwClose(g_NamedPipe);
        g_NamedPipe = NULL;
    }
}
VOID ProcessCreateNotify(HANDLE ParentId, HANDLE ProcessId, BOOLEAN Create) {
    WCHAR message[2048];
    WCHAR cmdLine[512] = { 0 };
    ULONG sessionId = 0;
    WCHAR* imageName = GetProcessImageFileName(ProcessId, cmdLine, sizeof(cmdLine), &sessionId);
    WCHAR parentName[260] = { 0 };
    GetProcessImageFileName(ParentId, parentName, sizeof(parentName), &sessionId);

    if (Create) {
        RtlStringCbPrintfW(message, sizeof(message),
            L"Process Created:\n"
            L"----> ImageName = %s\n"
            L"   --> PID       = %d\n"
            L"    --> PPID      = %d\n"
            L"     --> Parent    = %s\n"
            L"      --> CmdLine   = %s\n"
            L"       --> SessionID = %d\n",
            imageName, (ULONG)(ULONG_PTR)ProcessId, (ULONG)(ULONG_PTR)ParentId, parentName, cmdLine, sessionId);
    }
    else {
        RtlStringCbPrintfW(message, sizeof(message),
            L"Process Terminated:\n"
            L"    PID = %d\n",
            (ULONG)(ULONG_PTR)ProcessId);
    }

    SendToUserMode(message);
}
WCHAR* GetProcessImageFileName(HANDLE ProcessId, WCHAR* cmdLineBuffer, ULONG cmdLineBufferSize, ULONG* sessionId) {
    HANDLE processHandle;
    NTSTATUS status;
    PVOID buffer;
    ULONG returnLength;
    static WCHAR imageName[260];
    RtlZeroMemory(imageName, sizeof(imageName));

    OBJECT_ATTRIBUTES objAttr;
    CLIENT_ID clientId = { ProcessId, NULL };
    InitializeObjectAttributes(&objAttr, NULL, OBJ_KERNEL_HANDLE, NULL, NULL);

    status = ZwOpenProcess(&processHandle, PROCESS_QUERY_LIMITED_INFORMATION, &objAttr, &clientId);
    if (!NT_SUCCESS(status)) {
        wcscpy_s(imageName, ARRAYSIZE(imageName), L"Unknown");
        wcscpy_s(cmdLineBuffer, cmdLineBufferSize / sizeof(WCHAR), L"Unknown");
        return imageName;
    }

    status = ZwQueryInformationProcess(processHandle, 27, NULL, 0, &returnLength);
    if (status != STATUS_INFO_LENGTH_MISMATCH) {
        ZwClose(processHandle);
        wcscpy_s(imageName, ARRAYSIZE(imageName), L"Unknown");
        wcscpy_s(cmdLineBuffer, cmdLineBufferSize / sizeof(WCHAR), L"Unknown");
        return imageName;
    }

    buffer = ExAllocatePoolWithTag(NonPagedPool, returnLength, 'proc');
    if (!buffer) {
        ZwClose(processHandle);
        wcscpy_s(imageName, ARRAYSIZE(imageName), L"Unknown");
        wcscpy_s(cmdLineBuffer, cmdLineBufferSize / sizeof(WCHAR), L"Unknown");
        return imageName;
    }

    status = ZwQueryInformationProcess(processHandle, 27, buffer, returnLength, &returnLength);
    if (NT_SUCCESS(status)) {
        UNICODE_STRING* processName = (UNICODE_STRING*)buffer;
        wcsncpy_s(imageName, ARRAYSIZE(imageName), processName->Buffer, processName->Length / sizeof(WCHAR));
    }
    else {
        wcscpy_s(imageName, ARRAYSIZE(imageName), L"Unknown");
    }

    status = ZwQueryInformationProcess(processHandle, 60, NULL, 0, &returnLength);
    if (status == STATUS_INFO_LENGTH_MISMATCH) {
        PVOID cmdLineBufferPtr = ExAllocatePoolWithTag(NonPagedPool, returnLength, 'cmdl');
        if (cmdLineBufferPtr) {
            status = ZwQueryInformationProcess(processHandle, 60, cmdLineBufferPtr, returnLength, &returnLength);
            if (NT_SUCCESS(status)) {
                UNICODE_STRING* cmdLine = (UNICODE_STRING*)cmdLineBufferPtr;
                wcsncpy_s(cmdLineBuffer, cmdLineBufferSize / sizeof(WCHAR), cmdLine->Buffer, cmdLine->Length / sizeof(WCHAR));
            }
            else {
                wcscpy_s(cmdLineBuffer, cmdLineBufferSize / sizeof(WCHAR), L"Unknown");
            }
            ExFreePool(cmdLineBufferPtr);
        }
    }

    ZwQueryInformationProcess(processHandle, ProcessSessionInformation, sessionId, sizeof(ULONG), &returnLength);
    ZwClose(processHandle);
    ExFreePool(buffer);
    return imageName;
}



VOID DriverUnload(_In_ PDRIVER_OBJECT DriverObject) {
    UNREFERENCED_PARAMETER(DriverObject);
    PsSetCreateProcessNotifyRoutine(ProcessCreateNotify, TRUE);
    if (g_NamedPipe) {
        ZwClose(g_NamedPipe);
        g_NamedPipe = NULL;
    }
}

NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath) {
    UNREFERENCED_PARAMETER(RegistryPath);

    NTSTATUS status = PsSetCreateProcessNotifyRoutine(ProcessCreateNotify, FALSE);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    DriverObject->DriverUnload = DriverUnload;
    return STATUS_SUCCESS;
}
