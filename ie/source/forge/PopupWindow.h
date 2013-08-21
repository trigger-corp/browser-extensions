#ifndef __POPUPWINDOW_H__ 
#define __POPUPWINDOW_H__ 

#include <util.h>
#include "BrowserControl.h"


/**
 * Aliases
 */
class PopupWindow;
typedef CWinTraits<WS_POPUP,
                   WS_EX_LAYERED     | 
                   WS_EX_TOOLWINDOW  |
                   WS_EX_TOPMOST> PopupWinTraits;
typedef CWindowImpl<PopupWindow, CWindow, PopupWinTraits> PopupWindowImpl;


/**
 * Implementation: PopupWindow
 */
class PopupWindow
    : public PopupWindowImpl
{
public:
    PopupWindow(const wstring& uuid, POINT point, const wstring& url = L"about:blank");
    virtual ~PopupWindow();

    // TODO http://msdn.microsoft.com/en-us/library/ms997507.aspx
    DECLARE_WND_CLASS_EX(L"PopupWindow Class", 
                         CS_HREDRAW | 
                         CS_VREDRAW, // | CS_DROPSHADOW  // TODO
                         COLOR_WINDOW);

    BEGIN_MSG_MAP(PopupWindow)
        MESSAGE_HANDLER(WM_CREATE,    OnCreate)
        MESSAGE_HANDLER(WM_DESTROY,   OnDestroy)
        MESSAGE_HANDLER(WM_SIZE,      OnSize)     
        MESSAGE_HANDLER(WM_PAINT,     OnPaint)   
        MESSAGE_HANDLER(WM_KILLFOCUS, OnKillFocus) 
        MESSAGE_HANDLER(WM_SETFOCUS, OnSetFocus) 
    END_MSG_MAP()

    virtual LRESULT OnCreate (UINT msg, WPARAM wparam, LPARAM lparam, BOOL& handled);
    virtual LRESULT OnDestroy(UINT msg, WPARAM wparam, LPARAM lparam, BOOL& handled);
    virtual LRESULT OnSize   (UINT msg, WPARAM wparam, LPARAM lparam, BOOL& handled);

    LRESULT OnPaint    (UINT msg, WPARAM wparam, LPARAM lparam, BOOL& handled);
    LRESULT OnKillFocus(UINT msg, WPARAM wparam, LPARAM lparam, BOOL& handled);
    LRESULT OnSetFocus(UINT msg, WPARAM wparam, LPARAM lparam, BOOL& handled);

public:
    // aliases
    typedef shared_ptr<PopupWindow> pointer;
    typedef std::map<std::wstring, PopupWindow::pointer> map;

    // methods
    HRESULT Navigate(BSTR url) {
        return hiddenBrowser->Navigate(url, 
                                       &CComVariant(navNoHistory), 
                                       NULL, NULL, NULL);
    }

    // constants
    static const int DEFAULT_WIDTH  = 200;
    static const int DEFAULT_HEIGHT = 100;
    static const int DEFAULT_INSET  = 15;
    static const int DEFAULT_ALPHA  = 100;
    static const int TAB_SIZE = 15;

protected:
    wstring uuid;
    wstring url;

public:
    enum Alignment { left, right };
    Alignment alignment;

public:
    BrowserControl hiddenBrowser;
};

#endif /* __BROWSERWINDOW_H__ */
