#pragma once
#include "resource.h"       
#include <generated/Forge_i.h>
#include <util.h>
#include "PopupWindow.h"
#include <proxy/FrameProxy.h>
#include <ScriptExtensions.h> // for BrowserAction

#if defined(_WIN32_WCE) && !defined(_CE_DCOM) && !defined(_CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA)
#error "Single-threaded COM objects are not properly supported on Windows CE platform, such as the Windows Mobile platforms that do not include full DCOM support. Define _CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA to force ATL to support creating single-thread COM object's and allow use of it's single-threaded COM object implementations. The threading model in your rgs file was set to 'Free' as that is the only threading model supported in non DCOM Windows CE platforms."
#endif

using namespace ATL;


/**
 * Implementation: INativeControls
 */
class ATL_NO_VTABLE CNativeControls :
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CNativeControls, &CLSID_NativeControls>,
    public IObjectWithSiteImpl<CNativeControls>,
    public IDispatchImpl<INativeControls, &IID_INativeControls, &LIBID_ForgeLib, /*wMajor =*/ 1, /*wMinor =*/ 0>
{
public:
    CNativeControls();
    ~CNativeControls();

DECLARE_REGISTRY_RESOURCEID(IDR_NATIVECONTROLS)
DECLARE_NOT_AGGREGATABLE(CNativeControls)
DECLARE_CLASSFACTORY_SINGLETON(CNativeControls)

BEGIN_COM_MAP(CNativeControls)
    COM_INTERFACE_ENTRY(INativeControls)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IObjectWithSite)
END_COM_MAP()

DECLARE_PROTECT_FINAL_CONSTRUCT()
    HRESULT FinalConstruct();
    void FinalRelease();

public:
    // IDispatchImpl
    STDMETHOD(Invoke)(DISPID dispid, REFIID riid, LCID lcid, WORD flags, 
                      DISPPARAMS *params, VARIANT *result, EXCEPINFO *excepinfo, UINT *arg);

    // INativeControls
    STDMETHOD(load)             (BSTR uuid, BSTR extensionPath, unsigned int instanceId, ULONG hwnd);
    STDMETHOD(unload)           (BSTR uuid, unsigned int instanceId);
    STDMETHOD(popup_visible)    (BSTR uuid, BOOL isVisible, POINT point);
    STDMETHOD(button_setIcon)   (BSTR uuid, BSTR url, 
                                 IDispatch *success, IDispatch *error);
    STDMETHOD(button_setURL)    (BSTR uuid, BSTR url,
                                 IDispatch *success, IDispatch *error);
    STDMETHOD(button_onClicked) (BSTR uuid, IDispatch *callback);
    STDMETHOD(button_click)     (BSTR uuid, POINT point);
    STDMETHOD(button_setTitle)  (BSTR uuid, BSTR title,
                                 IDispatch *success, IDispatch *error);
    STDMETHOD(button_setBadge)  (BSTR uuid, INT number,
                                 IDispatch *success, IDispatch *error);
    STDMETHOD(button_setBadgeBackgroundColor) (BSTR uuid, BYTE r, BYTE g, BYTE b, BYTE a,
                                               IDispatch *success, IDispatch *error);
    STDMETHOD(popup_hwnd)       (BSTR uuid, BOOL *out_visible, ULONG *out_hwnd);

 private:
    unsigned int instanceCounter;

    // types & aliases
    typedef CComPtr<IDispatchEx> Listener;
    typedef std::map<wstring, Listener> Listeners; 

    // members
    Listeners        button_listeners;     // uuid -> Listener
    PopupWindow::map m_popupWindows;       // uuid -> PopupWindow
    wpathmap         m_extensionPaths;     // uuid -> Path
    Manifest::map    m_extensionManifests; // uuid -> Manifest

    // IEFrame IPC
    HWND       m_frame;
    FrameProxy *m_frameProxy;
};

OBJECT_ENTRY_AUTO(__uuidof(NativeControls), CNativeControls)

