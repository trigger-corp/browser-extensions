#include "stdafx.h"

#include <SDKDDKVer.h>
#include <windows.h>

#include <stdio.h>
#include <tchar.h>

#include <util.h>


/**
 * Helper: error
 */
bool error(wchar_t *message) 
{
    DWORD error = ::GetLastError();

    logger->error(L"forge.exe error"
                  L" -> " + wstring(message) +
                  L" -> " + boost::lexical_cast<wstring>(error));
    
    wprintf(L"forge.exe error: %s - %d\n", message, error);

    return false;
}


/**
 * SetProcessPrivilege
 */
bool SetProcessPrivilege(bool enable)
{
    logger->debug(L"forge.exe SetProcessPrivilege"
                  L" -> " + boost::lexical_cast<wstring>(enable));

    HANDLE token;
    if (!::OpenProcessToken(::GetCurrentProcess(), 
                            TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY | TOKEN_READ, 
                            &token)) {
        return error(L"SetProcessPrivilege ::GetCurrentProcess");
    }

    LUID luid;
    if (!::LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &luid)) {
        ::CloseHandle(token);
        return error(L"SetProcessPrivilege ::LookupPrivilegeValue");
    }

    TOKEN_PRIVILEGES privileges;
    privileges.PrivilegeCount = 1;
    privileges.Privileges[0].Luid = luid;
    privileges.Privileges[0].Attributes = (enable ? SE_PRIVILEGE_ENABLED : 0);

    if (!::AdjustTokenPrivileges(token, false, &privileges, 0, NULL, NULL)) {
        ::CloseHandle(token);
        return error(L"SetProcessPrivilege ::AdjustTokenPrivileges");
    }

    ::CloseHandle(token);

    return true;
}


/**
 * InjectDLL
 */
bool InjectDLL(DWORD processId, wchar_t *dll)
{
    logger->debug(L"forge.exe InjectDLL"
                  L" -> " + boost::lexical_cast<wstring>(processId) +
                  L" -> " + wstring(dll));

    if (!SetProcessPrivilege(true)) {
        return error(L"InjectDLL SetProcessPrivilege");
    }

    HANDLE process;
    process = ::OpenProcess(PROCESS_ALL_ACCESS, false, processId);
    if (!process) {
        return error(L"InjectDLL ::OpenProcess");
    }

    wchar_t path[MAX_PATH];
    if (!::GetModuleFileName(NULL, path, MAX_PATH)) {
        ::CloseHandle(process);
        return error(L"InjectDLL ::GetModuleFileName");
    }

    wchar_t* sp = wcsrchr(path, L'\\') + 1;
    wcscpy_s(sp, path + MAX_PATH - sp, dll);

    LPVOID address;
    address = ::VirtualAllocEx(process, NULL, sizeof(path), MEM_COMMIT, PAGE_READWRITE);
    if (!address) {
        ::CloseHandle(process);
        return error(L"InjectDLL ::VirtualAllocEx");
    }

    if (!::WriteProcessMemory(process, address, path, sizeof(path), NULL)) {
        ::VirtualFreeEx(process, address, sizeof(path), MEM_RELEASE);
        ::CloseHandle(process);
        return error(L"InjectDLL ::WriteProcessMemory");
    }

    HMODULE module = ::GetModuleHandle(L"Kernel32");
    if (module == NULL) {
        ::CloseHandle(process);
        return error(L"InjectDLL ::GetModuleHandle");
    }

    LPTHREAD_START_ROUTINE LoadLibraryW;
    LoadLibraryW = (LPTHREAD_START_ROUTINE)::GetProcAddress(module, 
                                                            "LoadLibraryW");
    if (LoadLibraryW == NULL) {
        ::CloseHandle(process);
        return error(L"InjectDLL ::GetProcAddress");
    }

    HANDLE thread;
    thread = ::CreateRemoteThread(process, NULL, 0, LoadLibraryW, address, 0, NULL);
    if (!thread) {
        ::VirtualFreeEx(process, address, sizeof(path), MEM_RELEASE);
        ::CloseHandle(process);
        return error(L"InjectDLL ::CreateRemoteThread");
    }

    ::WaitForSingleObject(thread, INFINITE);
    ::VirtualFreeEx(process, address, sizeof(path), MEM_RELEASE);

    //DWORD ec;
    //::GetExitCodeThread( thread, &ec );

    ::CloseHandle(thread);
    ::CloseHandle(process);
    SetProcessPrivilege(false);

    return true;
}


/**
 * main
 */
int _tmain(int argc, wchar_t* argv[])
{
    // get path
    wchar_t buf[MAX_PATH];
    ::GetModuleFileName(NULL, buf, MAX_PATH);
    boost::filesystem::wpath modulePath = boost::filesystem::wpath(buf).parent_path();

    // initialize logger
    logger->initialize(modulePath);
    logger->debug(L"forge.exe _tmain -> " + modulePath.wstring());

    DWORD exitCode = 1;
    if (argc > 0) {
#ifdef _WIN64
        bool ret = InjectDLL((DWORD)_wtoi(argv[0]), L"frame64.dll");
#else
        bool ret = InjectDLL((DWORD)_wtoi(argv[0]), L"frame32.dll");
#endif
        exitCode = ret ? 0 : 2;
    }

    logger->debug(L"forge.exe _tmain exiting"
                  L" -> " + boost::lexical_cast<wstring>(exitCode));

    /**
     * Possible exit codes:
     * 0 - success
     * 1 - invalid/missing command line arguments
     * 2 - DLL injection failed
     */
    return exitCode;
}
