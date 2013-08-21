#include "stdafx.h"
#include "AccessibleBrowser.h"


/**
 * Construction: Accessible
 */
Accessible::Accessible(HWND hwnd, long id)
    : id (id)
{
    HRESULT hr;
    hr = ::AccessibleObjectFromWindow(hwnd, OBJID_WINDOW, IID_IAccessible, 
                                      (void**)&iaccessible);
    if (FAILED(hr) || !iaccessible) {
        logger->warn(L"Accessible::Accessible invalid hwnd");
    }
}

Accessible::Accessible(IDispatch *dispatch, long id)
    : iaccessible(CComQIPtr<IAccessible, &IID_IAccessible>(dispatch)),
      id(id)
{
    if (!iaccessible) {
        logger->warn(L"Accessible::Accessible invalid dispatch");
    }
}

Accessible::Accessible(IAccessible *iaccessible, long id)
    : iaccessible(iaccessible), id(id)
{
}

/** 
 * Destruction: Accessible
 */
Accessible::~Accessible()
{
}


/**
 * Accessible::children
 */
Accessible::vector Accessible::children() 
{
    HRESULT hr;
    Accessible::vector ret;

    if (!this->iaccessible) {
        logger->error(L"Accessible::children invalid IAccessible");
        return ret;
    }

    // count children
    long count;
    hr = this->iaccessible->get_accChildCount(&count);
    if (FAILED(hr)) {
        logger->debug(L"Accessible::children failed to get child count"
                      L" -> " + logger->parse(hr));
        return ret;
    }
    
    // get accessors
    CComVariant* accessors = new CComVariant[count];
    long countObtained;
    hr = ::AccessibleChildren(this->iaccessible, 0, count,
                              accessors, &countObtained);
    if (FAILED(hr)) {
        logger->debug(L"Accessible::children failed to get accessors"
                      L" -> " + logger->parse(hr));
        return ret;
    }

    // iterate through accessors
    for (long n = 0; n < countObtained; n++) {
        CComVariant v = accessors[n];
        if (v.vt != VT_DISPATCH) {
            /*logger->debug(L"Accessible::children not an IAccessible"
              L" -> " + boost::lexical_cast<wstring>(n));*/
            continue;
        } 
        Accessible::pointer accessor =
            Accessible::pointer(new Accessible(v.pdispVal, n + 1));
        
        CComBSTR name;
        accessor->iaccessible->get_accName(CComVariant(CHILDID_SELF),
                                           &name);
        if (name) logger->debug(L"Accessible::children -> " + wstring(name));

        ret.push_back(accessor);
    }
    
    return ret;
}



/**
 * Construction: AccessibleBrowser
 */
AccessibleBrowser::AccessibleBrowser(HWND hwnd)
    : m_hwnd(hwnd)
{
}


/** 
 * Destruction: AccessibleBrowser
 */
AccessibleBrowser::~AccessibleBrowser()
{
}


/**
 * AccessibleBrowser::tabs
 */
wstringvector AccessibleBrowser::tabs()
{
    HRESULT hr;
    wstringvector ret;

    // get IEFrame HWND if necessary
    if (m_hwnd == NULL) {
        logger->debug(L"AccessibleBrowser::tabs calling EnumWindows for IE hwnd");
        ::EnumWindows(AccessibleBrowser::EnumWndProc, (LPARAM)&m_hwnd);
        if (!m_hwnd) {
            logger->debug(L"AccessibleBrowser::tabs could not get active tab");
            return ret;
        }
    }

    logger->debug(L"AccessibleBrowser::tabs active tab" 
                  L" -> " + boost::lexical_cast<wstring>(m_hwnd));


    // Get DirectUIHWND for IEFrame
    // TODO test on IE9
    HWND hwnd;
    hwnd = ::FindWindowEx(m_hwnd, NULL, L"CommandBarClass", NULL);
    if (!hwnd) { logger->error(L"AccessibleBrowser::tabs CommandBarClass failed"); return ret; }
    hwnd = ::FindWindowEx(hwnd, NULL, L"ReBarWindow32", NULL);
    if (!hwnd) { logger->error(L"AccessibleBrowser::tabs ReBarWindow32 failed"); return ret; }
    hwnd = ::FindWindowEx(hwnd, NULL, L"TabBandClass", NULL);
    if (!hwnd) { logger->error(L"AccessibleBrowser::tabs TabBandClass failed"); return ret; }
    hwnd = ::FindWindowEx(hwnd, NULL, L"DirectUIHWND", NULL);
    if (!hwnd) { logger->error(L"AccessibleBrowser::tabs DirectUIHWND failed"); return ret; }

    // get IAccessible for IE
    CComPtr<IAccessible> iaccessible;
    hr = ::AccessibleObjectFromWindow(hwnd, OBJID_WINDOW, IID_IAccessible, 
                                      (void**)&iaccessible);
    if (FAILED(hr) || !iaccessible) {
        logger->debug(L"AccessibleBrowser::tabs "
                      L"failed to get IAccessible for IE"
                      L" -> " + logger->parse(hr));
        return ret;
    }

    // iterate through accessors
    Accessible::vector accessors = Accessible(iaccessible).children();
    Accessible::vector::const_iterator accessor;
    for (accessor = accessors.begin(); accessor != accessors.end(); accessor++) {
        Accessible::vector children = (*accessor)->children();
        Accessible::vector::const_iterator child;
        for (child = children.begin(); child != children.end(); child++) {
            Accessible::vector tabs = (*child)->children();
            Accessible::vector::const_iterator tab;
            for (tab = tabs.begin(); tab != tabs.end(); tab++) {
                CComBSTR name;
                hr = (*tab)->iaccessible->get_accName(CComVariant(CHILDID_SELF),
                                                      &name);
                if (FAILED(hr) || !name) {
                    logger->debug(L"AccessibleBrowser::tabs "
                                  L"could not get tab name"
                                  L" -> " + boost::lexical_cast<wstring>((*tab)->id) +
                                  L" -> " + logger->parse(hr));
                    continue;
                }
                logger->debug(L"AccessibleBrowser::tabs "
                              L" -> " + boost::lexical_cast<wstring>((*tab)->id) +
                              L" -> " + wstring(name));

                ret.push_back(wstring(name));

                // construct a Tab object //////////////////////////////////
                /*Tab tab((*tab)->id, 0, 0,      // TODO id, index, windowId 
                        false, false, false,   // TODO incognito, selected, pinned
                        wstring(L"TODO"),      // TODO url
                        wstring(name),         // title
                        L"/TODO.ico",          // TODO  favIconUrl
                        L"TODO");              // TODO  status: loading | complete
                CComPtr<ITypeInfo> tabT;
                hr = ::CreateDispTypeInfo(&Tab::Interface, LOCALE_SYSTEM_DEFAULT,
                                          &tabT);
                if (FAILED(hr) || !tabT) {
                    logger->error(L"NativeExtensions::tabs "
                                  L"failed to create tabT"
                                  L" -> " + logger->parse(hr));
                    return ret;
                }
                CComPtr<IUnknown> tabI;
                hr = ::CreateStdDispatch(NULL, &tab, tabT, &tabI);
                if (FAILED(hr) || !tabI) {
                    logger->error(L"NativeExtensions::tabs "
                                  L"failed to create tabI"
                                  L" -> " + logger->parse(hr));
                    return ret;
                }
                
                // invoke callback on Tab object
                hr = CComDispatchDriver(callback).Invoke1((DISPID)0, &CComVariant(tabI));
                if (FAILED(hr)) {
                    logger->debug(L"AccessibleBrowser::tabs" 
                                  L" -> failed to invoke callback"
                                  L" -> " + logger->parse(hr));
                    continue;
                    }*/
                ////////////////////////////////////////////////////////////

            } // for every tab
        } // for every child
    } // for every accessor 

    return ret;
}

/**
 * Helper: EnumWndProc
 */
BOOL CALLBACK AccessibleBrowser::EnumWndProc(HWND hwnd, LPARAM param)
{
    BOOL ret; 
    wchar_t buf[MAX_PATH];
    wstring windowtext, windowclass;
    ret = ::GetWindowText(hwnd, buf, MAX_PATH);
    windowtext = buf;
    ret = ::RealGetWindowClass(hwnd, buf, MAX_PATH);
    windowclass = buf;    
    if (windowclass == L"IEFrame") {
        logger->debug(L"AccessibleBrowser::EnumWndProc "
                      L" -> " + boost::lexical_cast<wstring>(hwnd) +
                      L" -> " + windowtext +
                      L" -> " + windowclass);
        *(HWND*)param = hwnd;
        return false;
    }
    return true;
}

/**
 * Helper::EnumChildWndProc
 */
BOOL CALLBACK AccessibleBrowser::EnumChildWndProc(HWND hwnd, LPARAM param)
{
    wstring windowtext, windowclass;
    wchar_t buf[MAX_PATH];
    ::GetClassName(hwnd, buf, MAX_PATH);
    windowclass = buf;
    logger->debug(L"AccessibleBrowser::EnumChildWndProc "
                  L" -> " + boost::lexical_cast<wstring>(hwnd) +
                  L" -> " + windowtext +
                  L" -> " + windowclass);
    if (windowclass == L"Internet Explorer_Server") {
        *(HWND*)param = hwnd;
        return false;
    }
    return true;
}


/**
 * Utility: active
 */
#include <SHLGUID.h>  /* for SID_SWebBrowserApp */
HRESULT AccessibleBrowser::active(IWebBrowser2 **webBrowser2) 
{
    logger->debug(L"AccessibleBrowser::active");


    HINSTANCE instance;
    instance = ::LoadLibrary(L"OLEACC.DLL");
    if (instance == NULL) {
        logger->error(L"AccessibleBrowser::active "
                      L"could not load OLEACC.DLL");
        return E_FAIL;
    }

    HRESULT hr;

    // get "IEFrame"
    HWND ieframe = NULL;
    // gets into a race condition with the rest of the startup code
    //hr = ::EnumWindows(AccessibleBrowser::EnumWndProc, (LPARAM)&ieframe);
    ieframe = ::FindWindowEx(NULL, NULL, L"IEFrame", NULL);
    if (!ieframe) {
        logger->error(L"AccessibleBrowser::active "
                      L"could not get IEFrame");
        ::FreeLibrary(instance);
        return E_FAIL;
    }

    // get "Internet Explorer_Server"
    HWND hwnd = ieframe;
    // gets into a race condition with the rest of the startup code
    //hr = ::EnumChildWindows(ieframe, AccessibleBrowser::EnumChildWndProc, (LPARAM)&hwnd);
    hwnd = ::FindWindowEx(hwnd, NULL, L"Frame Tab", NULL);
    if (!hwnd) { logger->error(L"AccessibleBrowser::active failed: Frame Tab"); return E_FAIL; }
    hwnd = ::FindWindowEx(hwnd, NULL, L"TabWindowClass", NULL);
    if (!hwnd) { logger->error(L"AccessibleBrowser::active failed: TabWindowClass"); return E_FAIL; }
    hwnd = ::FindWindowEx(hwnd, NULL, L"Shell DocObject View", NULL);
    if (!hwnd) { logger->error(L"AccessibleBrowser::active failed: Shell DocObject View"); return E_FAIL; }
    hwnd = ::FindWindowEx(hwnd, NULL, L"Internet Explorer_Server", NULL);
    if (!hwnd) {
        logger->error(L"AccessibleBrowser::active "
                      L"could not get Internet Explorer_Server");
        ::FreeLibrary(instance);
        return E_FAIL;
    }

    UINT msg;
    msg = ::RegisterWindowMessage(L"WM_HTML_GETOBJECT");

    LRESULT result, ret;
    ret = ::SendMessageTimeout(hwnd, msg, 0L, 0L,
                               SMTO_ABORTIFHUNG, 1000,
                               reinterpret_cast<PDWORD_PTR>(&result));
    if (!ret || !result) {
        wchar_t *buf;
        DWORD error = GetLastError();
        FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                      NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&buf, 0, NULL);
        logger->error(L"AccessibleBrowser::active "
                      L"WM_HTML_GETOBJECT failed"
                      L" -> " + wstring(buf) +
                      L" -> " + boost::lexical_cast<wstring>(msg) +
                      L" -> " + boost::lexical_cast<wstring>(result));
        ::FreeLibrary(instance);
        return E_FAIL;
    }

    LPFNOBJECTFROMLRESULT object;
    object = reinterpret_cast<LPFNOBJECTFROMLRESULT>
        (::GetProcAddress(instance, "ObjectFromLresult"));
    if (object == NULL) {
        logger->error(L"AccessibleBrowser::active "
                      L"could not get HTML Object");
        ::FreeLibrary(instance);
        return E_FAIL;
    }

    CComPtr<IHTMLDocument2> htmlDocument2;
    hr = (*object)(result, IID_IHTMLDocument, 0, reinterpret_cast<void**>(&htmlDocument2));
    if (FAILED(hr)) {
        logger->error(L"AccessibleBrowser::active "
                      L"could not get IHTMLDocument"
                      L" -> " + logger->parse(hr));
        ::FreeLibrary(instance);
        return hr;
    }

    CComQIPtr<IServiceProvider> serviceProvider(htmlDocument2);
    if (!serviceProvider) {
        logger->error(L"AccessibleBrowser::active "
                      L"could not get IServiceProvider"
                      L" -> " + logger->parse(hr));
        ::FreeLibrary(instance);
        return E_FAIL;
    }

    hr = serviceProvider->QueryService(SID_SWebBrowserApp, IID_IWebBrowser2, 
                                       reinterpret_cast<void**>(webBrowser2));
    if (FAILED(hr)) {
        logger->error(L"AccessibleBrowser::active "
                      L"could not get IWebBrowser2"
                      L" -> " + logger->parse(hr));
        ::FreeLibrary(instance);
        return hr;
    }

    ::FreeLibrary(instance);    
    return S_OK;
}


/**
 * Type: NativeAccessible
 */
PARAMDATA NativeAccessible::_open [] = { 
    { L"bstr",     VT_BSTR }, 
    { L"bool",     VT_BOOL },
    { L"dispatch", VT_DISPATCH },
    { L"dispatch", VT_DISPATCH }
};
PARAMDATA NativeAccessible::_dispatch = { L"dispatch", VT_DISPATCH };
METHODDATA NativeAccessible::methods[] = {
    { L"open",         NativeAccessible::_open,      1, 0, CC_STDCALL, 4, DISPATCH_METHOD, VT_EMPTY },
    { L"closeCurrent", &NativeAccessible::_dispatch, 2, 1, CC_STDCALL, 1, DISPATCH_METHOD, VT_EMPTY },
};
INTERFACEDATA NativeAccessible::Interface = {
    NativeAccessible::methods,
    sizeof(NativeAccessible::methods) / sizeof(METHODDATA)
};


/**
 * Method: NativeAccessible::open
 *
 * @param url
 * @param selected
 * @param success
 * @param error
 */
void __stdcall NativeAccessible::open(BSTR url, VARIANT_BOOL selected, 
                                      IDispatch *success, IDispatch *error)
{
    logger->debug(L"NativeTabs::open"
                  L" -> " + wstring(url) +
                  L" -> " + boost::lexical_cast<wstring>(selected == -1) +
                  L" -> " + boost::lexical_cast<wstring>(success) +
                  L" -> " + boost::lexical_cast<wstring>(error));

    HRESULT hr;

    CComPtr<IWebBrowser2> webBrowser2;
    hr = AccessibleBrowser().active(&webBrowser2);
    if (FAILED(hr)) {
        logger->debug(L"NativeAccessible::open failed" 
                      L" -> " + boost::lexical_cast<wstring>(hr) +
                      L" -> " + wstring(url) + 
                      L" -> " + logger->parse(hr));
        CComDispatchDriver(error)
            .Invoke1((DISPID)0, &CComVariant(L"failed to get active window"));
        return;
    }

    BrowserNavConstants flags;
    flags = (selected == -1 ? navOpenInBackgroundTab : navOpenInNewTab); 
    hr = webBrowser2->Navigate2(&CComVariant(url), 
                                &CComVariant(flags, VT_I4), 
                                &CComVariant(NULL), 
                                &CComVariant(NULL), 
                                &CComVariant(NULL));
    if (FAILED(hr)) {
        logger->debug(L"NativeAccessible::open failed" 
                      L" -> " + boost::lexical_cast<wstring>(hr) +
                      L" -> " + wstring(url) + 
                      L" -> " + logger->parse(hr));
        CComDispatchDriver(error)
            .Invoke1((DISPID)0, &CComVariant(L"failed to open tab"));
        return;
    }

    hr = CComDispatchDriver(success).Invoke0((DISPID)0);
    if (FAILED(hr)) {
        logger->debug(L"CNativeExtensions::tabs_open "
                      L"failed to invoke success callback" 
                      L" -> " + wstring(url) +
                      L" -> " + logger->parse(hr));
        return;
    }
}



/** 
 * NativeAccessible::closeCurrent
 *
 * @param error
 */
void __stdcall NativeAccessible::closeCurrent(IDispatch *error) {
    logger->debug(L"NativeTabs::closeCurrent"
                  L" -> " + boost::lexical_cast<wstring>(error));
    HRESULT hr;
    hr = m_webBrowser2->Quit();
    if (FAILED(hr)) {
        logger->debug(L"NativeAccessible::closeCurrent "
                      L"failed to close current tab"
                      L" -> " + logger->parse(hr));
        CComDispatchDriver(error)
            .Invoke1((DISPID)0, &CComVariant(L"failed to close current tab"));

    }
}


