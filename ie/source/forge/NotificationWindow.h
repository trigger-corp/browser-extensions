#pragma once

#include <util.h>

typedef CWinTraits <WS_DISABLED | WS_POPUP, 
                    WS_EX_LAYERED    |
                    WS_EX_TOOLWINDOW |
                    WS_EX_TOPMOST    |
                    WS_EX_TRANSPARENT>  NotificationWinTraits;


/**
 * NotificationWindow
 */
class NotificationWindow 
    : public CWindowImpl <NotificationWindow, CWindow, NotificationWinTraits>
{
 public:
    NotificationWindow();
    virtual ~NotificationWindow();

    void Show(const wstring& icon, const wstring& title, const wstring& message);
    void Show(); 
    void Hide();

    DECLARE_WND_CLASS_EX(_T("Forge NotificationWindow Class"), 
                         CS_HREDRAW | CS_VREDRAW, COLOR_WINDOW);

    BEGIN_MSG_MAP(NotificationWindow)
        MESSAGE_HANDLER(WM_PAINT,   OnPaint)	  
        MESSAGE_HANDLER(WM_CLOSE,   OnClose)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
        MESSAGE_HANDLER(WM_TIMER,   OnTimer);
    END_MSG_MAP()

  public:
    static void Notification(const wstring& icon, 
                             const wstring& title,
                             const wstring& message) {
        if (!theNotification) {
            theNotification = new NotificationWindow();
        }
        theNotification->Show(icon, title, message);
    }
    static NotificationWindow* StupidHack() {
        theNotification = new NotificationWindow();
        theNotification->Initialize();
      return theNotification;
    }
    
protected:
    static NotificationWindow*   theNotification;
    
protected:
    LRESULT OnSize   (UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnPaint  (UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnClose  (UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnTimer  (UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
  
    void Initialize();

  private:
    wstring m_icon;
    wstring m_title;
    wstring m_message;
    bool    m_isInitialized;    
    static  const int IDT_TIMER = 777;

  public:
    static const int DEFAULT_WIDTH  = 300;
    static const int DEFAULT_HEIGHT = 50;
    static const int DEFAULT_INSET  = 15;
    static const int DEFAULT_ALPHA  = 230;
};
