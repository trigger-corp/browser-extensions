#pragma once

#include <commctrl.h>
#include <hash_map>
#include <util.h>
#include "Channel.h"


/**
 * FrameServer
 */
class FrameServer 
{
public:
    // lifecycle
    FrameServer(bool startListener);
    static bool AddRef(bool startListener = FALSE);
    static bool Release();
    static FrameServer* GetInstance() { return FrameServer::instance; }
    void load(HWND toolbar, HWND target, 
              const wstring& uuid, const wstring& title, const wstring& icon, 
              DWORDX processId, INT_PTRX proxyClient);
    void unload(DWORDX processId, INT_PTRX proxyClient);

    // interface
    void SetCurrentProxy(DWORDX processId, INT_PTRX proxyClient, HWND toolbar);
    LRESULT SendMessage(UINT msg, WPARAM wparam, LPARAM lparam) { 
        return ::SendMessage(this->m_toolbar, msg, wparam, lparam); 
    }
    LRESULT PostMessage(UINT msg, WPARAM wparam, LPARAM lparam) { 
        return ::PostMessage(this->m_toolbar, msg, wparam, lparam); 
    }
    bool WndProcTarget (LRESULT* lresult, UINT msg, WPARAM wparam, LPARAM lparam);
    bool WndProcToolbar(LRESULT* lresult, UINT msg, WPARAM wparam, LPARAM lparam);

    void OnButtonClick(HWND hwnd, WPARAM wparam, LPARAM lparam);

private:
    static LONG refCount;
    static ATL::CComAutoCriticalSection lock;
    static FrameServer *instance;

    HWND m_toolbar;
    HWND m_target;

public:
    struct Button {
        wstring uuid;
        WORD    idCommand;
        int     iBitmap;
        HICON   icon;
        HWND    toolbar;
        HWND    target;
        Button() : iBitmap(0), 
                   idCommand(0),
                   icon(NULL) { }
        void Destroy() {
            if (icon) ::DestroyIcon(icon);
            icon = NULL;
        }
    };
    typedef stdext::hash_map<wstring, Button> Buttons; // uuid -> Button
    Buttons m_buttons;

private:
    Channel  *m_channel;
    DWORD     m_tabCount;
    DWORDX    m_activeProcessId; 
    INT_PTRX  m_activeProxy;
	HWND      m_activeToolbar;

    typedef std::pair<Channel*, LONG> ClientListener; 
    typedef stdext::hash_map<DWORDX, ClientListener> ClientListeners;
    ClientListeners m_clientListeners;

	typedef stdext::hash_map<wstring, DWORD> ToolbarTabCount;
	ToolbarTabCount m_toolbarTabCount;

    ATL::CComAutoCriticalSection m_clientLock; 

private:
    static DWORD WINAPI ProxyListen(LPVOID param);

    // subclasses
    static LRESULT CALLBACK WndProcTargetS(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
    static LRESULT CALLBACK WndProcToolbarS(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);    
    typedef LRESULT (CALLBACK *WndProcPtr)(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);
    WndProcPtr m_oldWndProcTarget;
    WndProcPtr m_oldWndProcToolbar;
};
