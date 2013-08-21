#include "stdafx.h"
#include "BrowserControl.h"
#include <HTMLDocument.h>

#include "PopupWindow.h"


/**
 * LifeCycle: Constructor
 */
BrowserControl::BrowserControl(CWindow *parent, const wstring& uuid, bool resizeParent, bool subclass)
    : parent(parent),
      uuid(uuid),
      resizeParent(resizeParent),
      subclass(subclass), m_subclasser(NULL),
      bgcolor(255,255,255)
{
    /*logger->debug(L"BrowserControl::BrowserControl"
                  L" -> " + boost::lexical_cast<wstring>(parent) +
                  L" -> " + boost::lexical_cast<wstring>(uuid) +
                  L" -> " + boost::lexical_cast<wstring>(resizeParent) +
                  L" -> " + boost::lexical_cast<wstring>(subclass));*/
}


/**
 * LifeCycle: Destructor
 */
BrowserControl::~BrowserControl()
{
    HRESULT hr;

    logger->debug(L"BrowserControl::~BrowserControl");

    if (this->subclass && m_subclasser) {
        delete m_subclasser;
    }

    hr = this->EasyUnadvise();
    if (FAILED(hr)) {
        logger->warn(L"BrowserControl::~BrowserControl failed to unadvise" 
                     L" -> " + logger->parse(hr));
    }

    // NativeMessaging
    logger->debug(L"BrowserControl::~BrowserControl release NativeMessaging");
    if (m_nativeMessaging) {
        hr = m_nativeMessaging->unload(CComBSTR(this->uuid.c_str()), 0);
        if (FAILED(hr)) {
            logger->debug(L"BrowserControl::~BrowserControl failed to release NativeMessaging"
                          L" -> " + logger->parse(hr));
        }
    }

    // NativeControls
    logger->debug(L"BrowserControl::~BrowserControl release NativeControls");
    if (m_nativeControls) {
        hr = m_nativeControls->unload(CComBSTR(this->uuid.c_str()), 0);
        if (FAILED(hr)) {
            logger->debug(L"BrowserControl::~BrowserControl failed to release NativeControls"
                          L" -> " + logger->parse(hr));
        }
    }

    // BrowserControl
    hr = this->DestroyWindow();
    if (FAILED(hr)) {
        logger->warn(L"BrowserControl::~BrowserControl failed to destroy window");
    }
    this->m_hWnd = NULL;

    logger->debug(L"BrowserControl::~BrowserControl done");
}


/** 
 * Message: On
 */
LRESULT BrowserControl::OnCreate(UINT msg, WPARAM wparam, LPARAM lparam, BOOL &handled) 
{
    LRESULT lr;
    lr = this->DefWindowProc(msg, wparam, lparam);
    this->AttachControl(true);
    return lr;
}


/** 
 * Event: BrowserControl::OnBeforeNavigate2
 */
void __stdcall BrowserControl::OnBeforeNavigate2(IDispatch *idispatch, 
                                                 VARIANT *url, 
                                                 VARIANT *flags, 
                                                 VARIANT *target, 
                                                 VARIANT *postData, 
                                                 VARIANT *headers, 
                                                 VARIANT_BOOL *cancel)
{
}


/** 
 * Event: BrowserControl::OnNavigateComplete2
 */
void __stdcall BrowserControl::OnNavigateComplete2(IDispatch *idispatch, 
                                                   VARIANT *url)
{
    HRESULT hr;

    logger->debug(L"BrowserControl::OnNavigateComplete2"
                  L" -> " + wstring(url->bstrVal));

    // Do nothing for blank pages 
    wstring location(url->bstrVal);
    if (location.find(L"about:") == 0) {
        logger->debug(L"BrowserControl::OnNavigateComplete2 boring url");
        return;
    }

    // Get me some fiddlies
    CComQIPtr<IWebBrowser2, &IID_IWebBrowser2> webBrowser2(idispatch);
    if (!webBrowser2) {
        logger->error(L"BrowserControl::OnNavigateComplete2 no valid IWebBrowser2");
        return;
    }
    CComPtr<IDispatch> disp;
    webBrowser2->get_Document(&disp);
    if (!disp) {
        logger->error(L"BrowserControl::OnNavigateComplete2 get_Document failed");
        return;
    }
    CComQIPtr<IHTMLDocument2, &IID_IHTMLDocument2> htmlDocument2(disp);
    if (!htmlDocument2) {
        logger->error(L"BrowserControl::OnNavigateComplete2 IHTMLDocument2 failed");
        return;
    }
    CComQIPtr<IHTMLWindow2, &IID_IHTMLWindow2> htmlWindow2;
    htmlDocument2->get_parentWindow(&htmlWindow2);    
    if (!htmlWindow2) {
        logger->error(L"BrowserControl::OnNavigateComplete2 IHTMLWindow2 failed");
        return;
    }
    CComQIPtr<IDispatchEx> htmlWindow2Ex(htmlWindow2); 
    if (!htmlWindow2Ex) {
        logger->error(L"BrowserControl::OnNavigateComplete2 IHTMLWindow2Ex failed");
        return;
    }

    // Attach NativeAccessible (forge.tabs.*)
    if (m_nativeAccessible) {
        logger->error(L"BrowserControl::OnNavigateComplete2 resetting nativeAccessible");
        m_nativeAccessible.reset();
    }
    m_nativeAccessible = NativeAccessible::pointer(new NativeAccessible(webBrowser2));
    hr = Attach::NativeTabs(htmlWindow2Ex, 
                            L"accessible",
                            m_nativeAccessible.get());
    if (FAILED(hr)) {
        logger->error(L"BrowserControl::OnNavigateComplete2 "
                      L"failed to attach NativeAccessible"
                      L" -> " + logger->parse(hr));
    } 

    // Attach NativeExtensions
    hr = Attach::NativeExtensions(this->uuid,
                                  htmlWindow2Ex, 
                                  L"extensions", 
                                  0, // tabId 0 is BG Page
								  location,
                                  &m_nativeExtensions.p); // "T** operator&() throw()" asserts on p==NULL
    if (FAILED(hr)) {
        logger->error(L"BrowserControl::OnNavigateComplete2 "
                      L"failed to attach NativeExtensions"
                      L" -> " + logger->parse(hr));
    }

    // Attach NativeMessaging
    hr = Attach::NativeMessaging(this->uuid,
                                 htmlWindow2Ex, 
                                 L"messaging", 
                                 0, // instanceId == 0 is a console window
                                 &m_nativeMessaging.p); // "T** operator&() throw()" asserts on p==NULL
    if (FAILED(hr)) {
        logger->error(L"BrowserControl::OnNavigateComplete2 "
                      L"failed to attach NativeMessaging"
                      L" -> " + logger->parse(hr));
    } 

    // Attach NativeControls
    hr = Attach::NativeControls(this->uuid,
                                htmlWindow2Ex, 
                                L"controls", 
                                0, // instanceId == 0 is a console window
                                &m_nativeControls.p); // "T** operator&() throw()" asserts on p==NULL
    if (FAILED(hr)) {
        logger->error(L"BrowserControl::OnNavigateComplete2 "
                      L"failed to attach NativeControls"
                      L" -> " + logger->parse(hr));
    } 
}


/**
 * Event: OnDocumentComplete
 */
void __stdcall BrowserControl::OnDocumentComplete(IDispatch *idispatch, 
                                                  VARIANT *url)
{
    HRESULT hr;

    logger->debug(L"BrowserControl::OnDocumentComplete"
                  L" -> " + wstring(url->bstrVal ? url->bstrVal : L"NIL"));

    // Get me some fiddlies
    CComQIPtr<IWebBrowser2, &IID_IWebBrowser2> webBrowser2(idispatch);
    if (!webBrowser2) {
        logger->error(L"BrowserControl::OnDocumentComplete no valid IWebBrowser2");
        return;
    }
    CComPtr<IDispatch> disp;
    webBrowser2->get_Document(&disp);
    if (!disp) {
        logger->error(L"BrowserControl::OnDocumentComplete get_Document failed");
        return;
    }
    CComQIPtr<IHTMLDocument2, &IID_IHTMLDocument2> htmlDocument2(disp);
    if (!htmlDocument2) {
        logger->error(L"BrowserControl::OnDocumentComplete IHTMLDocument2 failed");
        return;
    }

    // subclass if necessary
    if (this->subclass && !m_subclasser) {
        HWND hwnd = this->m_hWnd;
        wchar_t buf[MAX_PATH];
        while ((hwnd = ::GetWindow(hwnd, GW_CHILD)) != NULL) { // TODO use ::EnumChildWindows
            ::GetClassName(hwnd, buf, MAX_PATH);
            if (wstring(buf) == L"Internet Explorer_Server") {
                m_subclasser = new Subclasser(this->parent); // parent is PopupWindow
                if (m_subclasser->SubclassWindow(hwnd)) {
                    m_subclasser->hwnd_ie = hwnd;
                } else {
                    logger->error(L"BrowserControl::OnDocumentComplete failed to subclass IE");
                }
                break;
            }
        }
    }

    // resize parent if needed
    if (!this->resizeParent) {
        return;
    }

    // Get page size
    CComPtr<IHTMLElement> htmlElement;
    hr = htmlDocument2->get_body(&htmlElement);
    if (FAILED(hr) || !htmlElement) {
        logger->error(L"BrowserControl::OnDocumentComplete "
                      L"could not get body");
        return;
    }
    CComQIPtr<IHTMLElement2, &IID_IHTMLElement2> htmlElement2(htmlElement);
    if (!htmlElement2) {
        logger->error(L"BrowserControl::OnDocumentComplete "
                      L"could not get IHTMLElement2");
        return;
    }
    long width, height;
    htmlElement2->get_scrollWidth(&width);
    htmlElement2->get_scrollHeight(&height);
    width  = MIN(800, width);       // approx chrome's maximum size
    height = MIN(500, height);      // 

    // Get page color
    CComVariant v;
    hr = htmlDocument2->get_bgColor(&v);
    if (FAILED(hr)) {
        logger->debug(L"BrowserControl::OnDocumentComplete "
                      L"failed to get background color");
        this->bgcolor = Gdiplus::Color(255,255,255);
    } else {
        wstring colorstring = wstring(v.bstrVal).substr(1);
        DWORD rgb = std::wcstoul(colorstring.c_str(), NULL, 16);
        this->bgcolor = Gdiplus::Color(GetBValue(rgb), 
                                       GetGValue(rgb), 
                                       GetRValue(rgb));
    } 

    // resize popup
    PopupWindow* popup = static_cast<PopupWindow*>(parent);
    width  += 2; // We're 2 pixels smaller than the parent
    height += ((PopupWindow::DEFAULT_INSET * 2) + 1); 
    int origin = buttonPosition.x;
    if (popup->alignment == PopupWindow::left) {
        origin = origin - (width - (PopupWindow::TAB_SIZE * 3)) - 3;
    } else {
        origin = origin - (PopupWindow::TAB_SIZE * 1) - 3;
    }
    parent->MoveWindow(origin, 
                       buttonPosition.y, 
                       width,
                       height,
                       TRUE);
}


/**
 * Event: OnQuit
 */
void __stdcall BrowserControl::OnQuit()
{
    logger->debug(L"BrowserControl::OnQuit");
}


/**
 * Helper: BrowserControl::SetContent
 */
HRESULT BrowserControl::SetContent(const wstringpointer& content)
{
    HRESULT hr;

    logger->debug(L"BrowserControl::SetContent -> " + wstring_limit(*content));

    CComPtr<IDispatch> disp;
    hr = (*this)->get_Document(&disp);
    if (FAILED(hr)) {
        logger->error(L"BrowserControl::SetContent "   
                      L"could not retrieve browser document");
        return hr;
    }
    CComQIPtr<IHTMLDocument2, &IID_IHTMLDocument2> document(disp);

    // gumph
    VARIANT*   param;
    SAFEARRAY* sfArray;
    sfArray = ::SafeArrayCreateVector(VT_VARIANT, 0, 1);
    BSTR       _content = ::SysAllocString((*content).c_str());
    if ((sfArray == NULL) || (document == NULL) || (_content == NULL)) {
        goto cleanup;
    }
    hr = ::SafeArrayAccessData(sfArray, (LPVOID*)&param);
    if (FAILED(hr)) {
        logger->error(L"BrowserControl::SetContent "
                      L"Could not access safe array");
        goto cleanup;
    }
    param->vt      = VT_BSTR;
    param->bstrVal = _content;
    hr = ::SafeArrayUnaccessData(sfArray);
    if (FAILED(hr)) {
        logger->error(L"BrowserControl::SetContent "   
                      L"Could not unaccess safe array");
        goto cleanup;
    }
    
    // clear current data
    hr = document->clear();
    
    // write it
    hr = document->write(sfArray);
    if (FAILED(hr)) {
        logger->error(L"BrowserControl::SetContent "   
                      L"Could not update document");
        goto cleanup;
    }
    
    // close stream
    hr = document->close();
    if (FAILED(hr)) {
        logger->error(L"BrowserControl::SetContent "   
                      L"Could not close document");
        goto cleanup;
    }
    
 cleanup:
    // clean up behind ourselves
    if (_content != NULL) {
        ::SysFreeString(_content);
    }
    if (sfArray != NULL) {
        ::SafeArrayDestroy(sfArray);
    }

    return hr;
}


/**
 * Helper: AttachControl
 */
HRESULT BrowserControl::AttachControl(BOOL events) 
{
    HRESULT hr;
    
    if (!this->m_hWnd) {
        logger->error(L"BrowserControl::AttachControl no parent");
        return E_FAIL;
    }
    
    CComPtr<IUnknown> unknown;
    hr = ::AtlAxGetControl(this->m_hWnd, &unknown);
    if (FAILED(hr)) {
        logger->error(L"BrowserControl::AttachControl AtlAxGetControl failed"
                      L" -> " + logger->parse(hr));
        return hr;
    }
    
    hr = unknown->QueryInterface( __uuidof(IWebBrowser2), 
                                  (void**)(CComPtr<IWebBrowser2>*)this);
    if (FAILED(hr)) {
        logger->error(L"BrowserControl::AttachControl QueryInterface failed"
                      L" -> " + logger->parse(hr));
        return hr;
    }
    
    if (!events) {
        return S_OK;
    }
    
    hr = this->EasyAdvise(unknown);
    if (FAILED(hr)) {
        logger->error(L"BrowserControl::AttachControl Advise failed"
                      L" -> " + logger->parse(hr));
        return hr;
    }        
    
    return S_OK;
}


/**
 * Helper: EasyAdvise
 */
HRESULT BrowserControl::EasyAdvise(IUnknown* unknown) {
    m_unknown = unknown;
    ::AtlGetObjectSourceInterface(unknown, &m_libid, &m_iid, &m_wMajorVerNum, &m_wMinorVerNum);
    return this->DispEventAdvise(unknown, &m_iid);
}


/**
 * Helper: EasyUnadvice
 */
HRESULT BrowserControl::EasyUnadvise() 
{
    logger->debug(L"BrowserControl::EasyUnadvise");
    ::AtlGetObjectSourceInterface(m_unknown, &m_libid, &m_iid, &m_wMajorVerNum, &m_wMinorVerNum);
    return this->DispEventUnadvise(m_unknown, &m_iid);
}
