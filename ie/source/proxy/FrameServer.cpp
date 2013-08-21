#include "stdafx.h"
#include "FrameServer.h"
#include "FrameProxy.h"
#include "Commands.h"
#include <WindowsMessage.h>


/**
 * Static initialization
 */
LONG FrameServer::refCount = 0;
FrameServer *FrameServer::instance = 0;
ATL::CComAutoCriticalSection FrameServer::lock;


/**
 * Lifecycle
 */
FrameServer::FrameServer(bool startListener)
    : m_channel(NULL),
      m_tabCount(0),
      m_activeProcessId(0),
      m_activeProxy(NULL)
{
    logger->debug(L"FrameServer::FrameServer"
                  L" -> " + boost::lexical_cast<wstring>(startListener));

    if (startListener) {
        HANDLE thread = ::CreateThread(NULL, 0, FrameServer::ProxyListen, this, 0, NULL);
        ::CloseHandle(thread);
    }
}


/**
 * Lifecycle: Reference counting
 */
bool FrameServer::AddRef(bool startListener)
{
    logger->debug(L"FrameServer::AddRef"
                  L" -> " + boost::lexical_cast<wstring>(startListener) +
                  L" -> " + boost::lexical_cast<wstring>(FrameServer::refCount));

    ATL::CComCritSecLock<CComAutoCriticalSection> lock(FrameServer::lock, true);

    if (++FrameServer::refCount == 1) {
        FrameServer::instance = new FrameServer(startListener);
        return true;
    }

    return false;
}


/**
 * Lifecycle: Reference counting
 */
bool FrameServer::Release()
{
    logger->debug(L"FrameServer::Release"
                  L" -> " + boost::lexical_cast<wstring>(FrameServer::refCount));

    ATL::CComCritSecLock<CComAutoCriticalSection> lock(FrameServer::lock, true);

    if (++FrameServer::refCount == 0) {
        delete FrameServer::instance;
        FrameServer::instance = NULL;
        return true;
    }

    return false;
}


/**
 * Lifecycle: load
 */
void FrameServer::load(HWND toolbar, HWND target, 
                       const wstring& uuid, const wstring& title, const wstring& icon, 
                       DWORDX processId, INT_PTRX proxy)
{
    logger->debug(L"FrameServer::load"
                  L" -> " + boost::lexical_cast<wstring>(toolbar) +
                  L" -> " + boost::lexical_cast<wstring>(target) +
                  L" -> " + uuid +
                  L" -> " + title +
                  L" -> " + icon +
                  L" -> " + boost::lexical_cast<wstring>(processId) +
                  L" -> " + boost::lexical_cast<wstring>(proxy));

    ATL::CComCritSecLock<CComAutoCriticalSection> lock(m_clientLock, true);

    if (processId != ::GetCurrentProcessId() && 
        m_clientListeners.find(processId) == m_clientListeners.end()) {
        // add IPC channel for proxy if it is in a different process 
        m_clientListeners[processId] = ClientListener(new Channel(L"IeBarMsgPoint", processId), 1);
        if (m_clientListeners.size() == 1) {
            m_activeProcessId = processId;
            m_activeProxy = proxy;
        }
    }

    // only initialize the first time
    if (++m_tabCount != 1) {
        logger->debug(L"FrameServer::load already initialized");
        lock.Unlock();
        return;
    }

    m_toolbar = toolbar;
    m_target  = target;
    
    // subclass windows
    m_oldWndProcToolbar = (WndProcPtr)::GetWindowLongPtr(m_toolbar, GWLP_WNDPROC);
    ::SetWindowLongPtr(m_toolbar, GWLP_WNDPROC, (LONG_PTR)WndProcToolbarS);
    m_oldWndProcTarget = (WndProcPtr)::GetWindowLongPtr(m_target, GWLP_WNDPROC);
    ::SetWindowLongPtr(m_target, GWLP_WNDPROC, (LONG_PTR)WndProcTargetS);

    lock.Unlock();
    
    // add button
    HRESULT hr;
    Button  button;
    hr = button_addCommand(uuid.c_str(), title.c_str(), icon.c_str()).exec(toolbar, target, &button);
    if (FAILED(hr)) {
        logger->error(L"FrameServer::load failed to create button"
                      L" -> " + logger->parse(hr));
        ::MessageBox(NULL,
                     wstring(L"Forge could not create button. Please check that "
                             L"your icon file is a 16x16 bitmap in .ico format: "
                             L"'" + icon + L"'").c_str(),
                     L"trigger.io",
                     MB_TASKMODAL | MB_ICONEXCLAMATION);
        return;
    }

    lock.Lock();
    m_buttons[uuid] = button;
    lock.Unlock();
    
}


/**
 * Lifecycle: unload
 */
void FrameServer::unload(DWORDX processId, INT_PTRX proxy)
{
    logger->debug(L"FrameServer::unload"
                  L" -> " + boost::lexical_cast<wstring>(processId) +
                  L" -> " + boost::lexical_cast<wstring>(proxy));

    ATL::CComCritSecLock<CComAutoCriticalSection> lock(m_clientLock, true);
    
    if (processId != ::GetCurrentProcessId()) {
        // destory IPC channel between proxy and server if they are in different processes
        ClientListener& entry = m_clientListeners[processId];
        if (--entry.second == 0) {
            delete entry.first;
            m_clientListeners.erase(processId);
        }
    }

    // clear all the things when tabCount hits zero
    if (--m_tabCount == 0) {
        // reverse subclassing 
        ::SetWindowLongPtr(m_toolbar, GWLP_WNDPROC, (LONG_PTR)m_oldWndProcToolbar);
        ::SetWindowLongPtr(m_target, GWLP_WNDPROC, (LONG_PTR)m_oldWndProcTarget);
        m_toolbar = m_target = NULL;
        
        // remove buttons
        Buttons::iterator i;
        for (i = m_buttons.begin(); i != m_buttons.end(); i++) {
            i->second.Destroy();
        }
        m_buttons.clear();
        
        // destory IPC channel which receives requests
        if (m_channel) {
            delete m_channel;
            m_channel = NULL;
        }
    }
}


/**
 * Interface: 
 */
void FrameServer::SetCurrentProxy(DWORDX processId, INT_PTRX proxy)
{
    /*logger->debug(L"FrameServer::SetCurrentProxy"
                  L" -> " + boost::lexical_cast<wstring>(processId) +
                  L" -> " + boost::lexical_cast<wstring>(proxy));*/
    //L" -> " + boost::lexical_cast<wstring>(m_clientLock));

    ATL::CComCritSecLock<CComAutoCriticalSection> lock(m_clientLock, true);
    
    // store current proxy  
    m_activeProcessId = processId;
    m_activeProxy = proxy;
    //logger->debug(L"FrameServer::SetCurrentProxy fin");
}


/**
 * WndProcTarget
 */
bool FrameServer::WndProcTarget(LRESULT* lresult, UINT msg, WPARAM wparam, LPARAM lparam)
{
    /*logger->debug(L"FrameServer::WndProcTarget"
                  L" -> " + boost::lexical_cast<wstring>(lresult) +
                  L" -> " + boost::lexical_cast<wstring>(msg) +
                  L" -> " + boost::lexical_cast<wstring>(wparam) +
                  L" -> " + boost::lexical_cast<wstring>(lparam));*/

    if (msg == WM_COMMAND) {
        // should we use IPC or access client proxy directly 
        if (m_activeProcessId != ::GetCurrentProcessId()) {
            ATL::CComCritSecLock<CComAutoCriticalSection> lock(m_clientLock, true);
            
            // send notification to listening thread of process which owns client proxy
            ClientListeners::iterator i = m_clientListeners.find(m_activeProcessId);
            if (i != m_clientListeners.end()) {
                ForwardedMessage msg(m_activeProxy, msg, wparam, lparam);
                i->second.first->Write(&msg, sizeof(msg), false);
            }
        } else {
            ((FrameProxy*)m_activeProxy)->WndProcTarget(lresult, msg, wparam, lparam);
        }
    }

    return false;
}


/**
 * WndProcToolbar
 */
bool FrameServer::WndProcToolbar(LRESULT* lresult, UINT msg, WPARAM wparam, LPARAM lparam)
{
    /*logger->debug(L"FrameServer::WndProcToolbar"
                  L" -> " + boost::lexical_cast<wstring>(lresult) +
                  L" -> " + boost::lexical_cast<wstring>(msg) +
                  L" -> " + boost::lexical_cast<wstring>(wparam) +
                  L" -> " + boost::lexical_cast<wstring>(lparam));*/

    return false;
}


/**
 * ProxyListen
 */
DWORD FrameServer::ProxyListen(LPVOID param)
{
    HRESULT hr;

    logger->debug(L"FrameServer::ProxyListen"
                  L" -> " + boost::lexical_cast<wstring>(param));

    FrameServer* pThis = (FrameServer*)param;

    pThis->m_channel = new Channel(L"IeBarListner", ::GetCurrentProcessId());
    while (pThis->m_channel) {
        char buffer[Channel::SECTION_SIZE];
        if (!pThis->m_channel->Read(buffer, Channel::SECTION_SIZE)) {
            break;
        }

        UINTX type = *(UINTX*)buffer;
        logger->debug(L"FrameServer::ProxyListen" 
                      L" -> " + boost::lexical_cast<wstring>(type));

        switch(type) {

        case SendMessageCommand::COMMAND_TYPE: {
            SendMessageCommand *command = (SendMessageCommand*)buffer;
            pThis->SendMessage(command->msg, command->wparam, command->lparam);
        }
            break;

        case PostMessageCommand::COMMAND_TYPE: {
            PostMessageCommand *command = (PostMessageCommand*)buffer;
            pThis->PostMessage(command->msg, command->wparam, command->lparam);
        }
            break;

        case LoadCommand::COMMAND_TYPE: {
            LoadCommand *command = (LoadCommand*)buffer;
            logger->debug(L"FrameServer::ProxyListen LoadCommand"
                          L" -> " + wstring(command->uuid) +
                          L" -> " + wstring(command->title) +
                          L" -> " + wstring(command->icon) +
                          L" -> " + boost::lexical_cast<wstring>(command->addressBarWnd) +
                          L" -> " + boost::lexical_cast<wstring>(command->commandTargetWnd));
            pThis->load(reinterpret_cast<HWND>(command->addressBarWnd), 
						reinterpret_cast<HWND>(command->commandTargetWnd), 
                        command->uuid, command->title, command->icon,
                        command->processId, command->proxy);
        }
            break;

        case UnloadCommand::COMMAND_TYPE: {
            UnloadCommand *command = (UnloadCommand*)buffer;
            pThis->unload(command->processId, command->proxy);
        }
            break;

        case SelectTabCommand::COMMAND_TYPE: {
            SelectTabCommand *command = (SelectTabCommand*)buffer;
            pThis->SetCurrentProxy(command->processId, command->proxy);
        }
            break;

        case button_addCommand::COMMAND_TYPE: {
            Button button;
            button_addCommand *command = (button_addCommand*)buffer;
            hr = command->exec(pThis->m_toolbar, pThis->m_target, &button);
            if (FAILED(hr)) {
                logger->error(L"FrameServer::ProxyListen button_setIconCommand failed"
                              L" -> " + logger->parse(hr));
            }
            pThis->m_buttons[button.uuid] = button;
        }
            break;

        case button_setIconCommand::COMMAND_TYPE: {
            button_setIconCommand *command = (button_setIconCommand*)buffer;
            Button button = pThis->m_buttons[command->uuid];
            hr = command->exec(pThis->m_toolbar, pThis->m_target, 
                               button.idCommand, button.iBitmap);
            if (FAILED(hr)) {
                logger->error(L"FrameServer::ProxyListen button_setIconCommand failed"
                              L" -> " + logger->parse(hr));
            }
        }
            break;

        case button_setTitleCommand::COMMAND_TYPE: {
            button_setTitleCommand *command = (button_setTitleCommand*)buffer;
            Button button = pThis->m_buttons[command->uuid];
            hr = command->exec(pThis->m_toolbar, pThis->m_target, button.idCommand);
            if (FAILED(hr)) {
                logger->error(L"FrameServer::ProxyListen button_setTitleCommand failed"
                              L" -> " + logger->parse(hr));
            }
        }
            break;

        case button_setBadgeCommand::COMMAND_TYPE: {
            button_setBadgeCommand *command = (button_setBadgeCommand*)buffer;
            Button button = pThis->m_buttons[command->uuid];
            hr = command->exec(pThis->m_toolbar, pThis->m_target, button.idCommand);
            if (FAILED(hr)) {
                logger->error(L"FrameServer::ProxyListen button_setBadgeCommand failed"
                              L" -> " + logger->parse(hr));
            }
        }
            break;

        case button_setBadgeBackgroundColorCommand::COMMAND_TYPE: {
            button_setBadgeBackgroundColorCommand *command = (button_setBadgeBackgroundColorCommand*)buffer;
            Button button = pThis->m_buttons[command->uuid];
            hr = command->exec(pThis->m_toolbar, pThis->m_target, button.idCommand);
            if (FAILED(hr)) {
                logger->error(L"FrameServer::ProxyListen button_setBadgeBackgroundColorCommand failed"
                              L" -> " + logger->parse(hr));
            }
        }
            break;
            
        }
    }

    return 0;
}


/**
 * Subclass: WndProcTargetS
 */
LRESULT FrameServer::WndProcTargetS(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    /*logger->debug(L"FrameServer::WndProcTargetS"
                  L" -> " + boost::lexical_cast<wstring>(hwnd) +
                  L" -> " + boost::lexical_cast<wstring>(msg) +
                  L" -> " + boost::lexical_cast<wstring>(wparam) +
                  L" -> " + boost::lexical_cast<wstring>(lparam));*/

    // capture button clicks
    if (msg == WM_COMMAND) { 
        FrameServer::instance->OnButtonClick(hwnd, wparam, lparam);
    } 
    
    LRESULT result = 0;
    if (FrameServer::instance->WndProcTarget(&result, msg, wparam, lparam)) {
        return result;
    }
    
    return CallWindowProc(FrameServer::instance->m_oldWndProcTarget, hwnd, msg, wparam, lparam);
}


/**
 * Subclass: WndProcToolbarS
 */
LRESULT FrameServer::WndProcToolbarS(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    if (msg == WM_COMMAND) { // WM_CLICK
        logger->debug(L"FrameServer::WndProcToolbarS WM_COMMAND"
                      L" -> " + boost::lexical_cast<wstring>(hwnd) +
                      L" -> " + boost::lexical_cast<wstring>(msg) +
                      L" -> " + boost::lexical_cast<wstring>(wparam) +
                      L" -> " + boost::lexical_cast<wstring>(lparam));
    }

    LRESULT result = 0;
    if (FrameServer::instance->WndProcToolbar(&result, msg, wparam, lparam)) {
        return result;
    }
    
    return CallWindowProc(FrameServer::instance->m_oldWndProcToolbar, hwnd, msg, wparam, lparam);
}


/**
 * FrameServer::OnButtonClick
 */
void FrameServer::OnButtonClick(HWND hwnd, WPARAM wparam, LPARAM lparam)
{
    logger->debug(L"FrameServer::OnButtonClick"
                  L" -> " + boost::lexical_cast<wstring>(hwnd) +
                  L" -> " + boost::lexical_cast<wstring>(wparam) +
                  L" -> " + boost::lexical_cast<wstring>(lparam));

    // the button, do we know it?
    Button button;
    Buttons::const_iterator i;
    for (i = m_buttons.begin(); i != m_buttons.end(); i++) {
        button = i->second;
        if (button.idCommand == wparam) break;
        else button.idCommand = 0;
    }
    if (button.idCommand == 0) { 
        logger->warn(L"FrameServer::OnButtonClick no button found for idCommand"
                     L" -> " + boost::lexical_cast<wstring>(wparam));
        return;
    }

    // get button rect
    RECT rect;
    memset(&rect, 0, sizeof(RECT));
    if (!WindowsMessage::tb_getrect(button.toolbar, button.idCommand, &rect)) {
        logger->error(L"FrameServer::OnButtonClick"
                      L" -> " + boost::lexical_cast<wstring>(button.idCommand) +
                      L" -> tb_getrect failed");
        return;
    }
    POINT point = { rect.left, rect.bottom };
    ::ClientToScreen(button.toolbar, &point);
    logger->debug(L"FrameServer::OnButtonClick"
                  L" -> " + boost::lexical_cast<wstring>(button.idCommand) +
                  L" -> " + boost::lexical_cast<wstring>(rect.left) +
                  L" -> " + boost::lexical_cast<wstring>(rect.top) +
                  L" -> " + boost::lexical_cast<wstring>(rect.right) +
                  L" -> " + boost::lexical_cast<wstring>(rect.bottom) +
                  L" -> " + boost::lexical_cast<wstring>(point.x) +
                  L" -> " + boost::lexical_cast<wstring>(point.y));
    
    // notify FrameProxy
    if (m_activeProcessId != ::GetCurrentProcessId()) {
        ATL::CComCritSecLock<CComAutoCriticalSection> lock(m_clientLock, true);
        ClientListeners::iterator i = m_clientListeners.find(m_activeProcessId);
        if (i != m_clientListeners.end()) {
            button_onClickCommand command(button.uuid.c_str(),
                                          point,
                                          m_activeProcessId, m_activeProxy);
            i->second.first->Write(&command, sizeof(button_onClickCommand), false);
        }
    }
}

