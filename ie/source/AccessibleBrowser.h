#pragma once
#include <oleacc.h>
#include <comdef.h>
#include <util.h>

using namespace ATL;

/**
 * Accessible
 */
class Accessible {
 public:
    // lifecycle
    Accessible(HWND hwnd, long id = 0);
    Accessible(IDispatch *dispatch, long id = 0);
    Accessible(IAccessible *accessible, long id = 0);
    ~Accessible();

    // aliases
    typedef shared_ptr<Accessible> pointer;
    typedef std::vector<pointer> vector;
     
    // methods
    Accessible::vector children();

    // wrapped
    CComPtr<IAccessible> iaccessible;
    long id;
};


/**
 * AccessibleBrowser
 */
class AccessibleBrowser {
 public:
    AccessibleBrowser(HWND ieframe = NULL);
    ~AccessibleBrowser();

    // methods
    wstringvector tabs();

    // utilities
    HRESULT active(IWebBrowser2 **webBrowser2);

 protected:

 private:
    HWND m_hwnd;
    static BOOL CALLBACK EnumWndProc(HWND hwnd, LPARAM param);
    static BOOL CALLBACK EnumChildWndProc(HWND hwnd, LPARAM param);    
};


/**
 * UAC forces us to run these inside the BHO
 *
 * NativeTabs : {
 *     open         : function(url, selected, success, error)
 *     closeCurrent : function(error) 
 * }
 *
 */
class NativeAccessible {
 public:
    virtual void __stdcall open(BSTR url, VARIANT_BOOL selected, 
                                IDispatch *success, IDispatch *error);
    virtual void __stdcall closeCurrent(IDispatch *error);

    // lifecycle
    NativeAccessible(IWebBrowser2 *webBrowser2) {
        m_webBrowser2 = webBrowser2;
    }
    ~NativeAccessible() {
        logger->debug(L"NativeAccessible::~NativeAccessible");
    }

    // COM
    static PARAMDATA _open[];
    static PARAMDATA _dispatch;
    static METHODDATA methods[];
    static INTERFACEDATA Interface;

    // aliases
    typedef shared_ptr<NativeAccessible> pointer;

 private:
    CComPtr<IWebBrowser2> m_webBrowser2;
};

