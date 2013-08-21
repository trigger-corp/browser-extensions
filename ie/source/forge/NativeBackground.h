#pragma once
#include "resource.h"      
#include <generated/Forge_i.h>
#include "_INativeBackgroundEvents_CP.h"
#include <util.h>
#include "BrowserWindow.h"

#if defined(_WIN32_WCE) && !defined(_CE_DCOM) && !defined(_CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA)
#error "Single-threaded COM objects are not properly supported on Windows CE platform, such as the Windows Mobile platforms that do not include full DCOM support. Define _CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA to force ATL to support creating single-thread COM object's and allow use of it's single-threaded COM object implementations. The threading model in your rgs file was set to 'Free' as that is the only threading model supported in non DCOM Windows CE platforms."
#endif

using namespace ATL;


/**
 * Implementation: INativeBackground
 */
class ATL_NO_VTABLE CNativeBackground :
    public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CNativeBackground, &CLSID_NativeBackground>,
	public ISupportErrorInfo,
	public IConnectionPointContainerImpl<CNativeBackground>,
	public CProxy_INativeBackgroundEvents<CNativeBackground>,
	public IObjectWithSiteImpl<CNativeBackground>,
	public IDispatchImpl<INativeBackground, &IID_INativeBackground, &LIBID_ForgeLib, /*wMajor =*/ 1, /*wMinor =*/ 0>
{
public:
	CNativeBackground();
    ~CNativeBackground();

DECLARE_REGISTRY_RESOURCEID(IDR_NATIVEBACKGROUND)
DECLARE_CLASSFACTORY_SINGLETON(CNativeBackground)

BEGIN_COM_MAP(CNativeBackground)
	COM_INTERFACE_ENTRY(INativeBackground)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
	COM_INTERFACE_ENTRY(IConnectionPointContainer)
	COM_INTERFACE_ENTRY(IObjectWithSite)
END_COM_MAP()

BEGIN_CONNECTION_POINT_MAP(CNativeBackground)
	CONNECTION_POINT_ENTRY(__uuidof(_INativeBackgroundEvents))
END_CONNECTION_POINT_MAP()

DECLARE_PROTECT_FINAL_CONSTRUCT()
    HRESULT FinalConstruct();
    void FinalRelease();

 public:
    // ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

    // INativeBackground
    STDMETHOD(load)    (BSTR uuid, BSTR url, BOOL isVisible, unsigned int *instanceId);
    STDMETHOD(unload)  (BSTR uuid, unsigned int instanceId);
    STDMETHOD(visible) (BSTR uuid, BOOL isVisible);

    BrowserWindow::map m_crouchingWindows;
    std::map<wstring, uintset> m_clients; // { uuid, [instanceId] }

    unsigned int instanceCounter;

 protected:
    HRESULT shutdown();
    
 private:
    BOOL isVisible;
};

OBJECT_ENTRY_AUTO(__uuidof(NativeBackground), CNativeBackground)
