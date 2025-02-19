#include <windows.h>
#include <stdio.h>

#define PIPE_NAME L"\\\\.\\pipe\\MyProcessMonitorPipe"

const char* BANNER =
"  _ _____ _     _  _        _    \n"
"  (_)___ /| |__ | || |   ___| | __\n"
"  | | |_ \\| '_ \\| || |_ / __| |/ /\n"
"  | |___) | | | |__   _| (__|   < \n"
" _/ |____/|_| |_|  |_|  \\___|_|\\_\\\n"
"|__/                               \n"
"[+] Say hi from Kernel!";

void StartNamedPipeServer() {
    SetConsoleOutputCP(CP_UTF8);
    printf("%s\n", BANNER);
    wprintf(L"[+] Creating named pipe: \\\\.\\pipe\\MyProcessMonitorPipe \n");

    HANDLE hPipe;
    WCHAR buffer[512];
    DWORD bytesRead;

    hPipe = CreateNamedPipeW(
        PIPE_NAME,
        PIPE_ACCESS_INBOUND,
        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
        1,
        512, 512, 0, NULL
    );

    if (hPipe == INVALID_HANDLE_VALUE) {
        wprintf(L"[-] Failed to create named pipe! Error: %d\n", GetLastError());
        return;
    }

    wprintf(L"[+] Waiting for client connection...\n");

    if (!ConnectNamedPipe(hPipe, NULL)) {
        wprintf(L"[-] Error waiting for Kernel Driver: %d\n", GetLastError());
        CloseHandle(hPipe);
        return;
    }

    wprintf(L"[+] Driver connected.\n");

    while (true) {
        memset(buffer, 0, sizeof(buffer));

        if (ReadFile(hPipe, buffer, sizeof(buffer) - sizeof(WCHAR), &bytesRead, NULL) && bytesRead > 0) {
            buffer[bytesRead / sizeof(WCHAR)] = L'\0';
            wprintf(L"[+] %s", buffer);
        }
        else {
            wprintf(L"[!] Read failed or client disconnected. Error: %d\n", GetLastError());
            break;
        }
    }

    CloseHandle(hPipe);
}

int main() {
    StartNamedPipeServer();
    return 0;
}
