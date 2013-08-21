#ifndef __BROWSERCONTROL_H__
#define __BROWSERCONTROL_H__

#include <Gdiplus.h>
#include <exdisp.h>
#include <exdispid.h>
using namespace ATL;

#include <util.h>
#include <Attach.h>


/**
 * BrowserControl
 */
class BrowserControl
    : public CComPtr<IWebBrowser2>,
      public CWindowImpl<BrowserControl, CAxWindow>,
      public IDispEventImpl<0, BrowserControl>
{
 public:
    BrowserControl(CWindow *parent, const wstring& uuid, bool resizeParent = false, bool subclass = false);
    virtual ~BrowserControl();

    // Messages
    BEGIN_MSG_MAP(BrowserControl)
        MESSAGE_HANDLER(WM_CREATE,    OnCreate)
    END_MSG_MAP()

    LRESULT OnCreate   (UINT msg, WPARAM wparam, LPARAM lparam, BOOL &handled);

    // Event Sinks
    BEGIN_SINK_MAP(BrowserControl)
        SINK_ENTRY(0, DISPID_BEFORENAVIGATE2,   OnBeforeNavigate2)          
        SINK_ENTRY(0, DISPID_NAVIGATECOMPLETE2, OnNavigateComplete2)
        SINK_ENTRY(0, DISPID_DOCUMENTCOMPLETE,  OnDocumentComplete)
        SINK_ENTRY(0, DISPID_ONQUIT,            OnQuit)
    END_SINK_MAP()       

    // Dwebbrowserevents2
    void __stdcall OnBeforeNavigate2(IDispatch *idispatch, 
                                     VARIANT *url, 
                                     VARIANT *flags, 
                                     VARIANT *target, 
                                     VARIANT *postData, 
                                     VARIANT *headers, 
                                     VARIANT_BOOL *cancel); 
    void __stdcall OnNavigateComplete2(IDispatch *idispatch,
                                       VARIANT *url);
    void __stdcall OnDocumentComplete(IDispatch *idispatch, 
                                      VARIANT *url);
    void __stdcall OnQuit(VOID);

    // helpers
    HRESULT SetContent(const wstringpointer& content);

 protected:
    // Keep COM servers around for the duration
    NativeAccessible::pointer  m_nativeAccessible;
    CComPtr<INativeExtensions> m_nativeExtensions;
    CComPtr<INativeMessaging>  m_nativeMessaging;
    CComPtr<INativeControls>   m_nativeControls;

    CWindow* parent;
    wstring  uuid;
    bool     resizeParent;
public:
    POINT buttonPosition;
    Gdiplus::Color bgcolor; 

private:
    // private helpers
    CComPtr<IUnknown> m_unknown;
    HRESULT EasyAdvise(IUnknown* unknown);
    HRESULT EasyUnadvise();
    HRESULT AttachControl(BOOL events = false);

    // subclass "Internet Explorer_Server"
    class Subclasser : public CWindowImpl {
    public:
        Subclasser(CWindow* parent = NULL) : parent(parent), hwnd_ie(NULL) {
            logger->debug(L"Subclasser::Subclasser"
                          L" -> " + boost::lexical_cast<wstring>(parent));
        }
        ~Subclasser() {
            logger->debug(L"Subclasser::~Subclasser");
            HWND hwnd;
            hwnd = this->UnsubclassWindow();
            if (hwnd != this->hwnd_ie) {
                logger->error(L"Subclasser::~Subclasser failed to unsubclass IE");
            }
            this->hwnd_ie = NULL;
        }
        DECLARE_EMPTY_MSG_MAP();
        static LRESULT CALLBACK WndProcIE(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
            Subclasser *pthis = static_cast<Subclasser*>(reinterpret_cast<void*>(hwnd));
            if (!pthis->parent) {
                logger->debug(L"Subclasser::WndProcIE no parent context");
                return CWindowImpl::WindowProc(hwnd, msg, wparam, lparam);
            }
            if (msg == WM_KILLFOCUS) {
                logger->debug(L"Subclasser::WM_KILLFOCUS");
                HWND hwnd_new = (HWND)wparam;
                if (!::IsChild(pthis->parent->m_hWnd, hwnd_new)) {
                    //pthis->parent->ShowWindow(SW_HIDE);
                    ::SendMessage(pthis->parent->m_hWnd, WM_KILLFOCUS, NULL, NULL);
                }
            } else if (msg == WM_SETFOCUS) {
                logger->debug(L"Subclasser::WM_SETFOCUS");
            }
            return CWindowImpl::WindowProc(hwnd, msg, wparam, lparam);
        };
        virtual WNDPROC GetWindowProc() {
            logger->debug(L"Subclasser::GetWindowProc");
            return Subclasser::WndProcIE;
        };
        HWND hwnd_ie;
        CWindow *parent; // PopupWindow
    };
    bool subclass;
    Subclasser *m_subclasser;
};


#endif /* __BROWSERCONTROL_H__ */
