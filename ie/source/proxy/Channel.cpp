#include "stdafx.h"
#include "Channel.h"


/**
 * Lifecycle: Channel
 */
Channel::Channel(wchar_t *baseName, DWORDX processId) 
{
    wchar_t name[MAX_PATH];

    int len = 0;
    for (int i = 0; i < STATE_COUNT; i++) {
        lstrcpy(name, baseName);
        len = lstrlen(name);
        lstrcpy(name + len, TEXT("Event"));
        len = lstrlen(name);
        _itot_s(processId, name + len, MAX_PATH - len, 10);
        len = lstrlen(name);
        _tcscpy_s(name + len, MAX_PATH - len, TEXT("_"));
        len = lstrlen(name);
        _itot_s(i, name + len, MAX_PATH - len, 10);
        m_events[ i ] = ::CreateEvent(NULL, FALSE, i == SERVER_AVAILABLE, name);
    }
    
    m_first = ::GetLastError() != ERROR_ALREADY_EXISTS;
    lstrcpy(name, TEXT("Local\\"));
    len = lstrlen(name);
    lstrcpy(name + len, baseName);
    len = lstrlen(name);
    lstrcpy(name + len, TEXT("Sect"));
    len = lstrlen(name);
    _itot_s(processId, name + len, MAX_PATH - len, 10);

    m_section = m_first 
        ? ::CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, SECTION_SIZE, name) 
        : ::OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, name);
}


/**
 * LifeCycle: ~Channel
 */
Channel::~Channel()
{
    for (int i = 0; i < STATE_COUNT; i++) {
        ::CloseHandle(m_events[i]);
    }
    
    ::CloseHandle(m_section);
}


/**
 * Channel::Read
 */
BOOL Channel::Read(LPVOID data, DWORD dataSize, BOOL response/* = FALSE*/)
{
    ::WaitForSingleObject(m_events[response 
                                   ? RESPONSE_AVAILABLE 
                                   : REQUEST_AVAILABLE], 
                          INFINITE);

    LPVOID source = ::MapViewOfFile(m_section, FILE_MAP_ALL_ACCESS, 0, 0, dataSize);
    if (!source) {
        if (!response) {
            ::SetEvent(m_events[SERVER_AVAILABLE]);
        }
        return FALSE;
    }

    ::CopyMemory(data, source, dataSize);
    BOOL ok = ::UnmapViewOfFile(source);

    if (!response) {
        ::SetEvent(m_events[SERVER_AVAILABLE]);
    }
    
    return ok;
}


/**
 * Channel::Write
 */
BOOL Channel::Write(LPVOID data, DWORD dataSize, BOOL response/* = FALSE*/)
{
    if (!response) {
        ::WaitForSingleObject(m_events[SERVER_AVAILABLE], INFINITE);
    }
    
    LPVOID destination = ::MapViewOfFile(m_section, FILE_MAP_ALL_ACCESS, 0, 0, dataSize);
    if (!destination) {
        if(!response) {
            ::SetEvent(m_events[SERVER_AVAILABLE]);
        }
        return FALSE;
    }

    ::CopyMemory(destination, data, dataSize);
    if (::UnmapViewOfFile(destination)) {
        ::SetEvent(m_events[response 
                           ? RESPONSE_AVAILABLE 
                           : REQUEST_AVAILABLE]);
        return TRUE;

    } else {
        ::SetEvent(m_events[(response 
                             ? RESPONSE_AVAILABLE 
                             : SERVER_AVAILABLE)]);
        return FALSE;
    }
}
