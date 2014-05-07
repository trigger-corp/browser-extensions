#include "stdafx.h"
#include "FrameProxy.h"
#include "Commands.h"


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

    // get os version: http://msdn.microsoft.com/en-us/library/ms724833(v=vs.85).aspx
    OSVERSIONINFO version;
    ::ZeroMemory(&version, sizeof(OSVERSIONINFO));
    version.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    ::GetVersionEx(&version);

    // get cpu arch: http://msdn.microsoft.com/en-us/library/ms724958(v=vs.85).aspx
    SYSTEM_INFO sysinfo;
    ::GetNativeSystemInfo(&sysinfo);

    logger->debug(L"Windows Version " 
                  L" -> dwMajorVersion " + boost::lexical_cast<wstring>(version.dwMajorVersion) +
                  L" -> dwMinorVersion " + boost::lexical_cast<wstring>(version.dwMinorVersion) +
                  L" -> wProcessorArchitecture " + boost::lexical_cast<wstring>(sysinfo.wProcessorArchitecture) +
                  L" -> PROCESSOR_ARCHITECTURE_INTEL " + boost::lexical_cast<wstring>(PROCESSOR_ARCHITECTURE_INTEL) +
                  L" -> PROCESSOR_ARCHITECTURE_AMD64 " + boost::lexical_cast<wstring>(PROCESSOR_ARCHITECTURE_AMD64));
    logger->debug(L"int   IS: " + boost::lexical_cast<wstring>(sizeof(int)));    // 4
    logger->debug(L"INT32 IS: " + boost::lexical_cast<wstring>(sizeof(INT32)));  // 4
    logger->debug(L"INT64 IS: " + boost::lexical_cast<wstring>(sizeof(INT64)));  // 8

    // spawn correct forgeXX.exe for os version and arch
    if (version.dwMajorVersion >= 6 && version.dwMinorVersion >= 2 && 
        sysinfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL) {    // windows 8 x86 
        path = path / L"forge32.exe";
    } else if (version.dwMajorVersion >= 6 && version.dwMinorVersion >= 2) { // Windows 8 x64 - frame process is ALWAYS 64 bit
        path = path / L"forge64.exe";
    } else {                                                                 // everyone else
#ifdef _WIN64
        path = path / L"forge64.exe";
#else
        path = path / L"forge32.exe";
#endif
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
