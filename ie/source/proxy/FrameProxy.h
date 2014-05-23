#pragma once

#include <util.h>
#include "FrameServer.h"
#include "Channel.h"


/**
 * FrameProxy
 *
 * Adapted from: http://www.codeproject.com/Articles/168191/Manipulating-Buttons-in-Internet-Explorer-s-Addres
 */
class FrameProxy
{                                                               
public:
    FrameProxy(const wstring& uuid, HINSTANCE instance, 
               HWND toolbar, HWND target,
               const wstring& title = L"", 
               const wstring& icon = L"");
    virtual ~FrameProxy();
    
    void SetCurrent();

    bool IsOnline() { return this->isOnline; }
    
    LRESULT SendMessage(UINT message, WPARAM wparam, LPARAM lparam);
    LRESULT PostMessage(UINT message, WPARAM wparam, LPARAM lparam);

    BOOL Read(TabCommand& command, DWORD dataSize, BOOL response = FALSE) {
        logger->debug(L"FrameProxy::Read"
                      L" -> " + boost::lexical_cast<wstring>(dataSize));
        command.processId = ::GetCurrentProcessId();
        command.proxy = (INT_PTRX)this;
        return m_commandChannel->Read(&command, dataSize);
    }
    BOOL Write(TabCommand& command, DWORD dataSize, BOOL response = FALSE) {
        logger->debug(L"FrameProxy::Write"
                      L" -> " + boost::lexical_cast<wstring>(dataSize));
        command.processId = ::GetCurrentProcessId();
        command.proxy = (INT_PTRX)this;
        return m_commandChannel->Write(&command, dataSize, response);
    }

    virtual bool WndProcTarget(LRESULT *lresult, UINT message, 
                               WPARAM wparam, LPARAM lparam);

    // static helpers
    static bool InjectDLL(HINSTANCE instance, DWORD processId);
    static bool Is64BitProcess(DWORD processId);
    
private:
    static DWORD WINAPI MessageHandlerListener(LPVOID param);

    FrameServer *m_frameServer;
    Channel     *m_commandChannel;
    Channel     *m_messageChannel;

    wstring uuid;
    bool isOnline;
};
