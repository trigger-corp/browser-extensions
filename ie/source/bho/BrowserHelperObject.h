#pragma once

#include "resource.h"     
#include <generated/BHO_h.h>
#include <Attach.h>
#include "../ScriptExtensions.h"
#include <proxy/FrameProxy.h>

#if defined(_WIN32_WCE) && !defined(_CE_DCOM) && !defined(_CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA)
#error "Single-threaded COM objects are not properly supported on Windows CE platform, such as the Windows Mobile platforms that do not include full DCOM support. Define _CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA to force ATL to support creating single-thread COM object's and allow use of it's single-threaded COM object implementations. The threading model in your rgs file was set to 'Free' as that is the only threading model supported in non DCOM Windows CE platforms."
#endif

using namespace ATL;


// aliases
class CBrowserHelperObject;
typedef IDispEventSimpleImpl<0, CBrowserHelperObject, &DIID_DWebBrowserEvents2> WebBrowserEvents2;


/**
 * Internet Explorer Browser Helper Object 
 */
class ATL_NO_VTABLE CBrowserHelperObject 
    : public CComObjectRootEx<CComSingleThreadModel>,
      public CComCoClass<CBrowserHelperObject, &CLSID_BrowserHelperObject>,
      public IObjectWithSiteImpl<CBrowserHelperObject>,
      public WebBrowserEvents2,
      public IDispatchImpl<IBrowserHelperObject, &IID_IBrowserHelperObject, &LIBID_ForgeBHOLib, 1, 0> // (frameProxy)
{
 public:
    CBrowserHelperObject();
    
DECLARE_GET_CONTROLLING_UNKNOWN()
DECLARE_REGISTRY_RESOURCEID(IDR_BROWSERHELPEROBJECT)
DECLARE_NOT_AGGREGATABLE(CBrowserHelperObject)
DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CBrowserHelperObject)
    COM_INTERFACE_ENTRY(IObjectWithSite)
END_COM_MAP()

BEGIN_SINK_MAP(CBrowserHelperObject)
    SINK_ENTRY_INFO(0, DIID_DWebBrowserEvents2, DISPID_NAVIGATECOMPLETE2,  OnNavigateComplete2, &OnNavigateComplete2Info)
    SINK_ENTRY_INFO(0, DIID_DWebBrowserEvents2, DISPID_DOCUMENTCOMPLETE,   OnDocumentComplete, &OnDocumentCompleteInfo)
    SINK_ENTRY_INFO(0, DIID_DWebBrowserEvents2, DISPID_WINDOWSTATECHANGED, OnWindowStateChanged, &OnWindowStateChangedInfo)
    SINK_ENTRY_INFO(0, DIID_DWebBrowserEvents2, DISPID_BEFORENAVIGATE2, OnBeforeNavigate2, &OnBeforeNavigate2Info)
    SINK_ENTRY_INFO(0, DIID_DWebBrowserEvents2, DISPID_DOWNLOADBEGIN, OnDownloadBegin, &OnDownloadBeginInfo)
    SINK_ENTRY_INFO(0, DIID_DWebBrowserEvents2, DISPID_DOWNLOADCOMPLETE, OnDownloadComplete, &OnDownloadCompleteInfo)
END_SINK_MAP()

    static _ATL_FUNC_INFO OnNavigateComplete2Info;
    static _ATL_FUNC_INFO OnDocumentCompleteInfo;
    static _ATL_FUNC_INFO OnWindowStateChangedInfo;

    static _ATL_FUNC_INFO OnBeforeNavigate2Info;
    static _ATL_FUNC_INFO OnDownloadBeginInfo;
    static _ATL_FUNC_INFO OnDownloadCompleteInfo;

 public:
    // IObjectWithSite
    STDMETHOD(SetSite)(IUnknown *iunknown);

    // IDispatchImpl 
    STDMETHOD(Invoke)(DISPID dispid, REFIID riid, LCID lcid, WORD flags, 
                      DISPPARAMS *params, VARIANT *result, EXCEPINFO *excepinfo, UINT *arg);

    // DWebBrowserEvents2
    void __stdcall OnNavigateComplete2(IDispatch *idispatch,
                                       VARIANT *url);
    void __stdcall OnDocumentComplete(IDispatch *idispatch, 
                                      VARIANT *url);
    void __stdcall OnWindowStateChanged(DWORD flags, DWORD mask);

	void __stdcall OnAttachForgeExtensions(IDispatch *idispatch, 
										   const wstring& location,
										   const wstring& eventsource);

    void __stdcall OnBeforeNavigate2(IDispatch *idispatch, VARIANT *url, VARIANT *Flags, 
                                     VARIANT *TargetFrameName, VARIANT *PostData, 
                                     VARIANT *Headers, VARIANT_BOOL* Cancel);

    void __stdcall OnDownloadBegin();
    void __stdcall OnDownloadComplete();

 private:
    HRESULT OnConnect(IUnknown *iunknown);
    HRESULT OnDisconnect(IUnknown *iunknown);
    HRESULT OnFirstRunAfterInstall();
    
    // Keep COM servers around for duration of BHO life
    NativeAccessible::pointer  m_nativeAccessible;
    CComPtr<INativeBackground> m_nativeBackground;
    CComPtr<INativeExtensions> m_nativeExtensions;
    CComPtr<INativeMessaging>  m_nativeMessaging;

    FrameProxy *m_frameProxy;

    ScriptExtensions::pointer m_scriptExtensions;
    unsigned int m_instanceId;
    bool m_isConnected;
	bool m_isAttached;

    bool m_isPageNavigation;
    int m_nObjCounter;
    bool m_bIsRefresh;
    wstring m_strUrl;

    void OnRefresh();
    
    // used to filter secondary requests
    CComPtr<IWebBrowser2> m_webBrowser2;

    // track window focus
    struct {
        int     id;
        bool    active;
        wstring url;
        wstring title;
    } m_tabInfo;

    std::pair<wstringvector, wstringvector> 
        MatchManifest(IWebBrowser2 *webBrowser2,
                      const Manifest::pointer& manifest, 
                      const wstring& location);
 public:
    HRESULT FinalConstruct() {
        return S_OK;
    }
    void FinalRelease() { 
        if (m_frameProxy) {
            delete m_frameProxy;
        }
    }
};
OBJECT_ENTRY_AUTO(__uuidof(BrowserHelperObject), CBrowserHelperObject)



