#pragma once
#pragma comment( lib, "WinInet.lib")
#include "resource.h"      
#include <generated/Forge_i.h>
#include "_INativeExtensionsEvents_CP.h"
#include <util.h>

#if defined(_WIN32_WCE) && !defined(_CE_DCOM) && !defined(_CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA)
#error "Single-threaded COM objects are not properly supported on Windows CE platform, such as the Windows Mobile platforms that do not include full DCOM support. Define _CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA to force ATL to support creating single-thread COM object's and allow use of it's single-threaded COM object implementations. The threading model in your rgs file was set to 'Free' as that is the only threading model supported in non DCOM Windows CE platforms."
#endif

using namespace ATL;


/**
 * Implementation: INativeExtensions
 */
class ATL_NO_VTABLE CNativeExtensions :
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CNativeExtensions, &CLSID_NativeExtensions>,
    public ISupportErrorInfo,
    public IConnectionPointContainerImpl<CNativeExtensions>,
    public CProxy_INativeExtensionsEvents<CNativeExtensions>,
    public IObjectWithSiteImpl<CNativeExtensions>,
    public IDispatchImpl<INativeExtensions, &IID_INativeExtensions, &LIBID_ForgeLib, /*wMajor =*/ 1, /*wMinor =*/ 0>
{
 public:
    CNativeExtensions();
    ~CNativeExtensions();

DECLARE_REGISTRY_RESOURCEID(IDR_NATIVEEXTENSIONS)

BEGIN_COM_MAP(CNativeExtensions)
    COM_INTERFACE_ENTRY(INativeExtensions)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(ISupportErrorInfo)
    COM_INTERFACE_ENTRY(IConnectionPointContainer)
    COM_INTERFACE_ENTRY(IObjectWithSite)
END_COM_MAP()

BEGIN_CONNECTION_POINT_MAP(CNativeExtensions)
    CONNECTION_POINT_ENTRY(__uuidof(_INativeExtensionsEvents))
END_CONNECTION_POINT_MAP()
    
    // ISupportsErrorInfo
    STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

DECLARE_PROTECT_FINAL_CONSTRUCT()
    HRESULT FinalConstruct();
    void FinalRelease();

 public:
    // INativeExtensions 
    STDMETHOD(log)          (BSTR uuid, BSTR message);
    STDMETHOD(prefs_get)    (BSTR uuid, BSTR name, 
                             IDispatch *success, IDispatch *error);
    STDMETHOD(prefs_set)    (BSTR uuid, BSTR name, BSTR value, 
                             IDispatch *success, IDispatch *error);
    STDMETHOD(prefs_keys)   (BSTR uuid, 
                             IDispatch *success, IDispatch *error);
    STDMETHOD(prefs_all)    (BSTR uuid, 
                             IDispatch *success, IDispatch *error);
    STDMETHOD(prefs_clear)  (BSTR uuid, BSTR name,
                             IDispatch *success, IDispatch *error);
    STDMETHOD(getURL)       (BSTR url, BSTR *out_url);
    STDMETHOD(xhr)          (BSTR method, BSTR url, BSTR data, BSTR contentType, BSTR headers,
                             IDispatch *success, IDispatch *error);
    STDMETHOD(notification) (BSTR icon, BSTR title, BSTR text,
                             BOOL *out_success);
    STDMETHOD(cookies_get)	(BSTR url, BSTR name,
                             IDispatch *success, IDispatch *error);
    STDMETHOD(cookies_remove)	(BSTR url, BSTR name,
                                 BOOL *out_success);

    STDMETHOD(set_tabId)    (UINT tabId) {
        this->tabId = tabId;
        return S_OK;
    }
    STDMETHOD(get_tabId)    (UINT *out_tabId) {
        *out_tabId = this->tabId;
        return S_OK;
    }    
    STDMETHOD(set_location) (BSTR location) {
        this->location = location;
        return S_OK;
    }
    STDMETHOD(get_location) (BSTR *out_location) {
        *out_location = CComBSTR(this->location.c_str());
        return S_OK;
    }    
 private:
    UINT tabId;
    wstring location;
};

OBJECT_ENTRY_AUTO(__uuidof(NativeExtensions), CNativeExtensions)
