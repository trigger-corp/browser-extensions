#include "stdafx.h"
#include "FrameProxy.h"
#include "Commands.h"

/**
 * Static helper: Is64BitProcess
 */
bool FrameProxy::Is64BitProcess(DWORD processId)
{
    logger->debug(L"FrameProxy::Is64BitProcess");

    // get os version: http://msdn.microsoft.com/en-us/library/ms724833(v=vs.85).aspx
    OSVERSIONINFO version;
    ::ZeroMemory(&version, sizeof(OSVERSIONINFO));
    version.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    ::GetVersionEx(&version);

    // get cpu arch: http://msdn.microsoft.com/en-us/library/ms724958(v=vs.85).aspx
    SYSTEM_INFO sysinfo;
    ::GetNativeSystemInfo(&sysinfo);

    logger->debug(L"FrameProxy::Is64BitProcess Windows Version"
                  L" -> dwMajorVersion " + boost::lexical_cast<wstring>(version.dwMajorVersion) +
                  L" -> dwMinorVersion " + boost::lexical_cast<wstring>(version.dwMinorVersion) +
                  L" -> wProcessorArchitecture " + boost::lexical_cast<wstring>(sysinfo.wProcessorArchitecture) +
                  L" -> PROCESSOR_ARCHITECTURE_INTEL " + boost::lexical_cast<wstring>(PROCESSOR_ARCHITECTURE_INTEL) +
                  L" -> PROCESSOR_ARCHITECTURE_AMD64 " + boost::lexical_cast<wstring>(PROCESSOR_ARCHITECTURE_AMD64));
    logger->debug(L"FrameProxy::Is64BitProcess type sizes"
                  L" -> int " + boost::lexical_cast<wstring>(sizeof(int)) +      // 4
                  L" -> INT32 " + boost::lexical_cast<wstring>(sizeof(INT32)) +  // 4
                  L" -> INT64 " + boost::lexical_cast<wstring>(sizeof(INT64)));  // 8

    // is WOW64 process: http://msdn.microsoft.com/en-us/library/ms684139.aspx
    BOOL isWow64 = FALSE;
    typedef BOOL(WINAPI *LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);
    LPFN_ISWOW64PROCESS fnIsWow64Process = (LPFN_ISWOW64PROCESS) ::GetProcAddress(
        ::GetModuleHandle(L"kernel32"), "IsWow64Process");

    if (fnIsWow64Process) {
        DWORD access = version.dwMajorVersion >= 6 ? PROCESS_QUERY_LIMITED_INFORMATION
                                                   : PROCESS_QUERY_INFORMATION;
        HANDLE process = ::OpenProcess(access, false, processId);
        if (process) {
            if (!fnIsWow64Process(process, &isWow64)) {
                DWORD error = ::GetLastError();
                logger->error(L"FrameProxy::Is64BitProcess IsWow64Process failed"
                              L" -> " + boost::lexical_cast<wstring>(error));
                isWow64 = FALSE;
            }
            ::CloseHandle(process);
        }
        else {
            DWORD error = ::GetLastError();
            logger->error(L"FrameProxy::Is64BitProcess failed to open process"
                          L" -> " + boost::lexical_cast<wstring>(error));
        }
    }
    else {
        // This happens for windows versions earlier than XP SP2.
        logger->info(L"FrameProxy::Is64BitProcess IsWow64Process missing");
    }

    logger->debug(L"FrameProxy::Is64BitProcess isWow64"
                  L" -> " + boost::lexical_cast<wstring>(isWow64));

    // If we are on a 32 bit CPU the process must be 32 bit.
    if (sysinfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL) {
        return false;
    }

    // If the CPU is 64 bit the process can either be native 64 bit or 32 bit
    // running under WOW64 emulation.
    if (isWow64) {
        return false; // 32 bit WOW64 process
    }
    else {
        return true; // Native 64 bit process
    }
}

/**
 * Static helper: InjectDLL
 */
bool FrameProxy::InjectDLL(HINSTANCE instance, DWORD processId)
{
    logger->debug(L"FrameProxy::InjectDLL");

    STARTUPINFO startupInfo;
    ::ZeroMemory(&startupInfo, sizeof(startupInfo));
    startupInfo.cb = sizeof(startupInfo);
    startupInfo.dwFlags |= STARTF_USESHOWWINDOW;
    startupInfo.wShowWindow = FALSE;
    
    PROCESS_INFORMATION processInfo;
    ::ZeroMemory(&processInfo, sizeof(processInfo));
    
    wchar_t params[MAX_PATH];
    _itow_s(processId, params, MAX_PATH, 10);
    
    // get module path
    wchar_t buf[MAX_PATH];
    ::GetModuleFileName(instance, buf, MAX_PATH);
    bfs::wpath path = bfs::wpath(buf).parent_path();

    if (Is64BitProcess(processId)) {
        path = path / L"forge64.exe";
    }
    else {
        path = path / L"forge32.exe";
    }

    logger->debug(L"FrameProxy::InjectDLL spawning process"
                  L" -> " + path.wstring());
                  
    if (!::CreateProcess(path.wstring().c_str(), params, 
                         NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, 
                         &startupInfo, &processInfo)) {
        logger->error(L"FrameProxy::InjectDLL failed to create process"
                      L" -> " + path.wstring());
        return false;
    }
    
    ::WaitForSingleObject(processInfo.hProcess, INFINITE);
    
    DWORD exitCode = 0;
    if (!::GetExitCodeProcess(processInfo.hProcess, &exitCode)) {
        DWORD error = ::GetLastError();
        logger->warn(L"FrameProxy::InjectDLL failed to get process exit code"
                     L" -> " + boost::lexical_cast<wstring>(error));
        exitCode = 0; // TODO: Should this be an error?
    }

    ::CloseHandle(processInfo.hThread);
    ::CloseHandle(processInfo.hProcess);
    
    if (exitCode != 0) {
        logger->error(L"FrameProxy::InjectDLL spawned process failed"
                      L" -> " + boost::lexical_cast<wstring>(exitCode));
        return false;
    }

    return true;
}



/**
 * Lifecycle: FrameProxy
 */
FrameProxy::FrameProxy(const wstring& uuid, HINSTANCE instance, 
                       HWND toolbar, HWND target, 
                       const wstring& title, const wstring& icon)
    : uuid(uuid),
      isOnline(false),
      m_commandChannel(NULL),
      m_messageChannel(NULL)
{
    logger->debug(L"FrameProxy::FrameProxy"
                  L" -> " + uuid +
                  L" -> " + title +
                  L" -> " + icon +
                  L" -> " + boost::lexical_cast<wstring>(instance) +
                  L" -> " + boost::lexical_cast<wstring>(toolbar) +
                  L" -> " + boost::lexical_cast<wstring>(target));

    DWORD processId = 0;
    ::GetWindowThreadProcessId(toolbar, &processId);
    if (processId == ::GetCurrentProcessId()) {
        logger->debug(L"FrameProxy::FrameProxy invoking FrameServer::load directly"
                      L" -> " + uuid +
                      L" -> " + title +
                      L" -> " + icon);
        FrameServer::AddRef();
        m_frameServer = FrameServer::GetInstance();
        m_frameServer->load(toolbar, target, 
                            uuid, title, icon,
                            ::GetCurrentProcessId(), (INT_PTRX)this);
        return;
    }

    m_frameServer = NULL;
    m_commandChannel = new Channel(L"IeBarListner", processId);
    m_messageChannel = new Channel(L"IeBarMsgPoint", ::GetCurrentProcessId());
    if (m_messageChannel->IsFirst()) {
      HANDLE thread = ::CreateThread(NULL, 0, MessageHandlerListener, m_messageChannel, 0, NULL);
      ::CloseHandle(thread);
    }
    if (m_commandChannel->IsFirst()) {
        this->isOnline = this->InjectDLL(instance, processId);
    } else {
        this->isOnline = true; // already conencted
    }

    if (this->isOnline && uuid != L"NativeControls") { // @ugly
        logger->debug(L"FrameProxy::FrameProxy sending LoadCommand message");
        LoadCommand command(reinterpret_cast<HWNDX>(toolbar), 
                            reinterpret_cast<HWNDX>(target), 
                            uuid, title, icon,
                            ::GetCurrentProcessId(), (INT_PTRX)this);
        m_commandChannel->Write(&command, sizeof(command));
    }
}


/** 
 * Lifecycle: ~FrameProxy
 */
FrameProxy::~FrameProxy() 
{
    logger->debug(L"FrameProxy::~FrameProxy");
}


/**
 *
 */
void FrameProxy::SetCurrent()
{
    /*logger->debug(L"FrameProxy::SetCurrent"
                  L" -> " + boost::lexical_cast<wstring>(m_frameServer) +
                  L" -> " + boost::lexical_cast<wstring>(m_commandChannel));*/

    if (m_frameServer) {
        logger->debug(L"FrameProxy::SetCurrent direct call");
        return m_frameServer->SetCurrentProxy(::GetCurrentProcessId(), (INT_PTRX)this);
    } 
      
    //logger->debug(L"FrameProxy::SetCurrent Write");
    SelectTabCommand command(::GetCurrentProcessId(), (INT_PTRX)this);
    m_commandChannel->Write(&command, sizeof(command));
    //logger->debug(L"FrameProxy::SetCurrent Write fin");
}


/**
 * FrameProxy::SendMessage
 */
LRESULT FrameProxy::SendMessage(UINT message, WPARAM wparam, LPARAM lparam)
{
    logger->debug(L"FrameProxy::SendMessage" 
                  L" -> " + boost::lexical_cast<wstring>(message) +
                  L" -> " + boost::lexical_cast<wstring>(wparam)  +
                  L" -> " + boost::lexical_cast<wstring>(lparam));

    if (m_frameServer) {
        return m_frameServer->SendMessage(message, wparam, lparam);
    }

    SendMessageCommand command(message, wparam, lparam);
    m_commandChannel->Write(&command, sizeof(command));

    LRESULT result;
    m_commandChannel->Read(&result, sizeof(result), TRUE);

    return result;
}


/**
 * FrameProxy::PostMessage
 */
LRESULT FrameProxy::PostMessage(UINT message, WPARAM wparam, LPARAM lparam)
{
    logger->debug(L"FrameProxy::PostMessage" 
                  L" -> " + boost::lexical_cast<wstring>(message) +
                  L" -> " + boost::lexical_cast<wstring>(wparam)  +
                  L" -> " + boost::lexical_cast<wstring>(lparam));

    if (m_frameServer) {
        return m_frameServer->PostMessage(message, wparam, lparam);
    }
    
    PostMessageCommand command(message, wparam, lparam);
    m_commandChannel->Write(&command, sizeof(command));
    
    return 0;
}


/**
 * FrameProxy::MessageHandlerListener
 */
DWORD FrameProxy::MessageHandlerListener(LPVOID param)
{
    logger->debug(L"FrameProxy::MessageHandlerListener" 
                  L" -> " + boost::lexical_cast<wstring>(param));

    Channel *channel = (Channel*)param;
    char     buffer[Channel::SECTION_SIZE];

    while (channel->Read(buffer, Channel::SECTION_SIZE)) {
        
        UINTX type = *(UINTX*)buffer;
        logger->debug(L"FrameProxy::MessageHandlerListener type" 
                      L" -> " + boost::lexical_cast<wstring>(type));
        
        switch(type) {
        case ForwardedMessage::COMMAND_TYPE: {
            logger->debug(L"FrameProxy::MessageHandlerListener ForwardedMessage");
            ForwardedMessage *message = (ForwardedMessage*)buffer;
            FrameProxy       *proxy   = (FrameProxy*)message->proxy;
            LRESULT result;
            proxy->WndProcTarget(&result, 
                                 message->msg, 
                                 message->wparam, 
                                 message->lparam);

        }
            break;

        case button_onClickCommand::COMMAND_TYPE: {
            logger->debug(L"FrameProxy::MessageHandlerListener button_onClickCommand");
            button_onClickCommand *command = (button_onClickCommand*)buffer;
            command->exec();
        }
            break;
                        
        default:
            logger->debug(L"FrameProxy::MessageHandlerListener unknown type"
                          L" -> " + boost::lexical_cast<wstring>(type));
            ForwardedMessage *message = (ForwardedMessage*)buffer;
            FrameProxy       *proxy   = (FrameProxy*)message->proxy;
            LRESULT result;
            proxy->WndProcTarget(&result, 
                                 message->msg, 
                                 message->wparam, 
                                 message->lparam);

            break;
        }
    }
    
    return 0;
}


/**
 * 
 */
bool FrameProxy::WndProcTarget(LRESULT *lresult, UINT message, 
                               WPARAM wparam, LPARAM lparam) 
{
    logger->debug(L"FrameProxy::WndProcTarget"
                  L" -> " + boost::lexical_cast<wstring>(lresult) +
                  L" -> " + boost::lexical_cast<wstring>(message) +
                  L" -> " + boost::lexical_cast<wstring>(wparam) +
                  L" -> " + boost::lexical_cast<wstring>(lparam));

    switch (message) {
    case WM_COMMAND: {
        logger->debug(L"FrameProxy::WndProcTarget WM_COMMAND"
                      L" -> " + boost::lexical_cast<wstring>(wparam) +
                      L" -> " + boost::lexical_cast<wstring>(lparam));
    }
        break;
    case 0xdeadbeef: {
        logger->debug(L"FrameProxy::WndProcTarget Got some reply");
    }
        break;
    default:
        logger->debug(L"unknown message"
                      L" -> " + boost::lexical_cast<wstring>(message));
        break;
    };
    
    return 0;

}
