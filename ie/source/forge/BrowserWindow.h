#ifndef __BROWSERWINDOW_H__
#define __BROWSERWINDOW_H__
#include <exdisp.h>
#include <exdispid.h>
using namespace ATL;

#include <util.h>
#include "BrowserControl.h"


/**
 * Aliases
 */
class BrowserWindow;
typedef CWindowImpl<BrowserWindow, CWindow, CFrameWinTraits> BrowserWindowImpl;

/**
 * Implementation: BrowserWindow
 */
class BrowserWindow 
    : public BrowserWindowImpl
{
  public:
    BrowserWindow(const wstring& uuid, const wstring& url);
    virtual ~BrowserWindow();

    DECLARE_WND_CLASS_EX(L"BrowserWindow Class", CS_HREDRAW | CS_VREDRAW, COLOR_WINDOW);

    BEGIN_MSG_MAP(BrowserWindow)
        MESSAGE_HANDLER(WM_CREATE,  OnCreate)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
        MESSAGE_HANDLER(WM_SIZE,    OnSize)	  
    END_MSG_MAP()

    virtual LRESULT OnCreate (UINT msg, WPARAM wparam, LPARAM lparam, BOOL& handled);
    virtual LRESULT OnDestroy(UINT msg, WPARAM wparam, LPARAM lparam, BOOL& handled);
    virtual LRESULT OnSize   (UINT msg, WPARAM wparam, LPARAM lparam, BOOL& handled);

 public:
    // aliases
    typedef shared_ptr<BrowserWindow> pointer;
    typedef std::map<std::wstring, BrowserWindow::pointer> map;

    // methods
    HRESULT Navigate(BSTR url) {
        return hiddenBrowser->Navigate(url, 
                                       &CComVariant(navNoHistory), 
                                       NULL, NULL, NULL);
    }

 protected:
    wstring uuid;
    wstring url;
    BrowserControl hiddenBrowser;
};

#endif /* __BROWSERWINDOW_H__ */
