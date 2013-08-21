#pragma once
#include "resource.h"      
#include <generated/Forge_i.h>
#include "_INativeMessagingEvents_CP.h"
#include <util.h>
#include "NativeMessagingTypes.h"

#if defined(_WIN32_WCE) && !defined(_CE_DCOM) && !defined(_CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA)
#error "Single-threaded COM objects are not properly supported on Windows CE platform, such as the Windows Mobile platforms that do not include full DCOM support. Define _CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA to force ATL to support creating single-thread COM object's and allow use of it's single-threaded COM object implementations. The threading model in your rgs file was set to 'Free' as that is the only threading model supported in non DCOM Windows CE platforms."
#endif

using namespace ATL;


/**
 * Implementation: INativeMessaging
 */
class ATL_NO_VTABLE CNativeMessaging :
    public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CNativeMessaging, &CLSID_NativeMessaging>,
	public ISupportErrorInfo,
	public IConnectionPointContainerImpl<CNativeMessaging>,
	public CProxy_INativeMessagingEvents<CNativeMessaging>,
	public IObjectWithSiteImpl<CNativeMessaging>,
	public IDispatchImpl<INativeMessaging, &IID_INativeMessaging, &LIBID_ForgeLib, /*wMajor =*/ 1, /*wMinor =*/ 0>
{
public:
	CNativeMessaging();
    ~CNativeMessaging();

DECLARE_REGISTRY_RESOURCEID(IDR_NATIVEMESSAGING)

BEGIN_COM_MAP(CNativeMessaging)
	COM_INTERFACE_ENTRY(INativeMessaging)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
	COM_INTERFACE_ENTRY(IConnectionPointContainer)
	COM_INTERFACE_ENTRY(IObjectWithSite)
END_COM_MAP()

DECLARE_CLASSFACTORY_SINGLETON(CNativeMessaging)

BEGIN_CONNECTION_POINT_MAP(CNativeMessaging)
	CONNECTION_POINT_ENTRY(__uuidof(_INativeMessagingEvents))
END_CONNECTION_POINT_MAP()

    // ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

DECLARE_PROTECT_FINAL_CONSTRUCT()
    HRESULT FinalConstruct();
    void FinalRelease();

 public:
    // INativeMessaging
    STDMETHOD(load)         (BSTR uuid, unsigned int instanceId);
    STDMETHOD(unload)       (BSTR uuid, unsigned int instanceId);
    STDMETHOD(tabs_set)     (BSTR uuid, UINT instanceId, BSTR url, BSTR title, BOOL focused);
    STDMETHOD(tabs_active)  (BSTR uuid, IDispatch *callback, UINT *out_tabId);
    STDMETHOD(fg_listen)(BSTR uuid, UINT tabId, BSTR type, IDispatch *callback, IDispatch * error);
    STDMETHOD(fg_broadcast)(BSTR uuid, BSTR type, BSTR content, IDispatch *callback, IDispatch *error);
    STDMETHOD(fg_toFocussed)(BSTR uuid, BSTR type, BSTR content, IDispatch *callback, IDispatch *error);
    STDMETHOD(fg_broadcastBackground)(BSTR uuid, BSTR type, BSTR content, IDispatch *callback, IDispatch *error);
    STDMETHOD(bg_listen)(BSTR uuid, BSTR type, IDispatch *callback, IDispatch *error);
    STDMETHOD(bg_broadcast)(BSTR uuid, BSTR type, BSTR content, IDispatch *callback, IDispatch *error);
    STDMETHOD(bg_toFocussed)(BSTR uuid, BSTR type, BSTR content, IDispatch *callback, IDispatch *error);

 protected:
    HRESULT shutdown();

 private:
    Callback::map bg_callbacks;
    Callback::map fg_callbacks;

    // tabs
    Tab m_activeTab;

    std::map<wstring, uintset> m_clients; // { uuid, [instanceId] }
};

OBJECT_ENTRY_AUTO(__uuidof(NativeMessaging), CNativeMessaging)
