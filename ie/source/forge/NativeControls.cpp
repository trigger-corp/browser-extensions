#include "stdafx.h"
#include "NativeControls.h"
#include <generated/Forge_i.h> /* for: */
#include "dllmain.h"           /*   _AtlModule */
#include <WindowsMessage.h>
#include <proxy/Channel.h>
#include <proxy/Commands.h>

/**
 * Construction
 */
CNativeControls::CNativeControls() : 
    m_frame(NULL), m_frameProxy(NULL)
{
    logger->debug(L"CNativeControls::CNativeControls");
    this->instanceCounter = 1;
}


/** 
 * Destruction
 */
CNativeControls::~CNativeControls()
{
    logger->debug(L"CNativeControls::~CNativeControls");
}

HRESULT CNativeControls::FinalConstruct()
{
    logger->debug(L"CNativeControls::FinalConstruct");
    return S_OK;
}

void CNativeControls::FinalRelease()
{
    logger->debug(L"CNativeControls::FinalRelease");

    if (m_frameProxy) {
        delete m_frameProxy;
        m_frameProxy = NULL;
    }
}


/**
 * INativeControls: load
 */
STDMETHODIMP CNativeControls::load(BSTR _uuid, BSTR _extensionPath, unsigned int instanceId, ULONG hwnd)
{
    wstring uuid             = _uuid;
    bfs::wpath extensionPath = bfs::wpath(_extensionPath);
    this->m_frame            = (HWND)hwnd;

    logger->debug(L"CNativeControls::load"
                  L" -> " + uuid +
                  L" -> " + extensionPath.wstring() +
                  L" -> " + boost::lexical_cast<wstring>(instanceId) +
                  L" -> " + boost::lexical_cast<wstring>(m_frame));

    // load manifest
    ScriptExtensions::pointer scriptExtensions =
        ScriptExtensions::pointer(new ScriptExtensions(extensionPath));
    if (!scriptExtensions->manifest) {
        logger->error(L"Failed to read manifest file: " +
                      scriptExtensions->pathManifest.wstring() + 
                      L" at: " + extensionPath.wstring());
        ::MessageBox(NULL,
                     wstring(L"Failed to read manifest. Please check that "
                             L" manifest.json file is present at " +
                             extensionPath.wstring() +
                             L" and properly configured.").c_str(),
                     L"trigger.io",
                     MB_TASKMODAL | MB_ICONEXCLAMATION);
        // TODO halt init and exit gracefully
        return E_FAIL;
    }
    m_extensionPaths[uuid] = extensionPath;
    m_extensionManifests[uuid] = scriptExtensions->manifest;

    // get toolbar HWND from IEFrame HWND
    HWND toolbar, target;
    toolbar = WindowsMessage::GetToolbar(m_frame, &toolbar, &target);
    if (!toolbar || !target) {
            logger->error(L"CNativeControls::load failed to get ToolbarWindow32");
            return E_FAIL;
    }

    // inject proxy lib into frame process
    m_frameProxy = new FrameProxy(L"NativeControls",
                                  _AtlModule.moduleHandle,
                                  toolbar, target);

    // create popup
    PopupWindow::pointer popup = m_popupWindows[uuid];
    if (!popup) { 
        logger->debug(L"CNativeControls::load creating popup"
                      L" -> " + wstring(uuid));

        // get default url for popup 
        Manifest::pointer manifest = m_extensionManifests[uuid];
        if (!manifest) {
            logger->error(L"CNativeControls::load "
                          L"no manifest for extension"
                          L" -> " + wstring(uuid));
            return E_FAIL;
        }
        bfs::wpath path = m_extensionPaths[uuid] / manifest->browser_action.default_popup;
        wstring url = L"file://" + path.wstring();

        // get a popup parent that doesn't competely tie IE's knickers into a knot
        HWND parent = m_frame;
        parent = ::FindWindowEx(parent, NULL, L"Frame Tab", NULL);
        if (!parent) {
            // In IE7 there is no intermediate "Frame Tab" window. If we didn't find
            // one try getting TabWindowClass directly from under m_frame.
            parent = m_frame;
        }
        parent = ::FindWindowEx(parent, NULL, L"TabWindowClass", NULL);
        if (!parent) { logger->error(L"CNativeControls::load failed: TabWindowClass"); return E_FAIL; }
        /*parent = ::FindWindowEx(parent, NULL, L"Shell DocObject View", NULL);
        if (!parent) { logger->error(L"CNativeControls::load failed: Shell DocObject View"); return E_FAIL; }
        parent = ::FindWindowEx(parent, NULL, L"Internet Explorer_Server", NULL);
        if (!parent) { logger->error(L"CNativeControls::load failed: Internet Explorer_Server"); return E_FAIL; }*/
        
        POINT point; point.x = 0; point.y = 0; 
        popup = PopupWindow::pointer(new PopupWindow(uuid, point, url));
        HWND hwnd;
        hwnd = popup->Create(parent,
                             CRect(point.x,
                                   point.y, 
                                   point.x + PopupWindow::DEFAULT_WIDTH,
                                   point.y + PopupWindow::DEFAULT_HEIGHT),
                             CComBSTR((L"Forge Popup " + wstring(uuid)).c_str()));
        if (!hwnd) {
            logger->error(L"CNativeControls::load "
                          L"failed to create popup");
            return E_FAIL;
        }
        m_popupWindows[uuid] = popup;
    }

    return S_OK;
}


/**
 * INativeControls: unload
 */
STDMETHODIMP CNativeControls::unload(BSTR uuid, unsigned int instanceId)
{
    logger->debug(L"CNativecontrols::unload"
                  L" -> " + wstring(uuid) +
                  L" -> " + boost::lexical_cast<wstring>(instanceId));

    if (m_frameProxy) {
        delete m_frameProxy;
        m_frameProxy = NULL;
    }

    return S_OK;
}


/**
 * INativeControls: NativeControls::popup_visible
 *
 * @param uuid
 * @param isVisible
 */
STDMETHODIMP CNativeControls::popup_visible(BSTR uuid, BOOL isVisible, POINT point)
{
    logger->debug(L"CNativeControls::popup_visible"
                  L" -> " + wstring(uuid) +
                  L" -> " + boost::lexical_cast<wstring>(isVisible) +
                  L" -> " + boost::lexical_cast<wstring>(point.x) +
                  L" -> " + boost::lexical_cast<wstring>(point.y) +
                  L" -> " + boost::lexical_cast<wstring>(m_frame));

    // TODO
    wchar_t buf[MAX_PATH];
    ::GetClassName(m_frame, buf, MAX_PATH);
    if (m_frame == NULL) {
        logger->error(L"!!!! CNativeControls::popup_visible WARNING m_frame is null !!!");
    } else {
        logger->debug(L"CNativeControls::popup_visible m_frame -> " + wstring(buf));
    }

    // get popup
    PopupWindow::pointer popup = m_popupWindows[uuid];
    if (!popup) { 
        logger->debug(L"CNativeControls::popup_visible has no popup"
                      L" -> " + wstring(uuid));
        return E_FAIL;
    }

    // get screen metrics
    DWORD screenWidth = ::GetSystemMetrics(SM_CXSCREEN);
    if (point.x > (int)(screenWidth / 2)) {
        popup->alignment = PopupWindow::left;
    } else {
        popup->alignment = PopupWindow::right;
    }

    // update popup position & visibility
    RECT rect;
    popup->GetClientRect(&rect);
    int width  = rect.right - rect.left;
    int height = rect.bottom - rect.top;
    int origin = point.x;
    if (popup->alignment == PopupWindow::left) {
        origin = origin - (width - (PopupWindow::TAB_SIZE * 3)) - 3;
    } else {
        origin = origin - (PopupWindow::TAB_SIZE * 1) - 3;
    }
    popup->MoveWindow(origin, 
                      point.y, 
                      width, 
                      height);
    popup->hiddenBrowser.buttonPosition = point;
    popup->ShowWindow(SW_SHOW);
    
    return S_OK;
}


/**
 * INativeControls: NativeControls::popup_hwnd
 *
 * @param uuid
 */
STDMETHODIMP CNativeControls::popup_hwnd (BSTR uuid, BOOL *out_visible, ULONG *out_hwnd) 
{
    *out_visible = FALSE;
    *out_hwnd = NULL;
    PopupWindow::pointer popup = m_popupWindows[uuid];
    if (!popup) { 
        logger->error(L"CNativeControls::popup_hwnd has no popup"
                      L" -> " + wstring(uuid));
        return E_FAIL;
    }       
    *out_visible = TRUE;
    *out_hwnd = (ULONG)popup->m_hWnd;
    return S_OK;
}


/**
 * INativeControls: NativeControls::button_setIcon
 *
 * @param uuid
 */
STDMETHODIMP CNativeControls::button_setIcon(BSTR _uuid, BSTR _url, 
                                             IDispatch *success, 
                                             IDispatch *error)
{
    wstring uuid(_uuid), url(_url);

    logger->debug(L"CNativeControls::button_setIcon"
                  L" -> " + uuid +
                  L" -> " + url +
                  L" -> " + boost::lexical_cast<wstring>(success) +
                  L" -> " + boost::lexical_cast<wstring>(error));

    if (!m_frameProxy) {
        logger->debug(L"CNativeControls::button_setIcon invalid FrameProxy");
        return S_OK;
    }

    // strip protocol from filesystem URLs
    wstring protocol(L"file:///");
    if (url.find(protocol) == 0) {
        url = url.erase(0, protocol.size());
    }
    protocol = L"file://";
    if (url.find(protocol) == 0) {
        url = url.erase(0, protocol.size());
    }
    bfs::wpath path(url);

    // check for relative paths
    if (!path.has_root_path()) {
        path = _AtlModule.modulePath / bfs::wpath(L"src") / path;        
    } 
    
    // check if it exists
    if (!bfs::exists(path)) {
        logger->debug(L"CNativeControls::button_setIcon invalid path for icon"
                      L" -> " + path.wstring());
        return CComDispatchDriver(error)
            .Invoke1((DISPID)0, 
                     &CComVariant(L"{ 'message' : 'invalid path for icon' }"));
    }

    // dispatch command via FrameProxy
    button_setIconCommand command(uuid.c_str(), path.wstring().c_str());
    m_frameProxy->Write(command, sizeof(button_setIconCommand));
    
    return CComDispatchDriver(success).Invoke0((DISPID)0);
}


/**
 * INativeControls: NativeControls::button_setURL
 *
 * @param uuid
 */
STDMETHODIMP CNativeControls::button_setURL(BSTR uuid, BSTR url,
                                            IDispatch *success, 
                                            IDispatch *error)
{
    HRESULT hr;

    logger->debug(L"CNativeControls::button_setURL"
                  L" -> " + wstring(uuid) +
                  L" -> " + wstring(url) +
                  L" -> " + boost::lexical_cast<wstring>(success) +
                  L" -> " + boost::lexical_cast<wstring>(error));

    PopupWindow::pointer popup = m_popupWindows[uuid];
    if (!popup) {
        logger->error(L"CNativeControls::button_setURL fatal error "
                      L"no popup window for extension "
                      L" -> " + wstring(uuid));
        return CComDispatchDriver(error)
            .Invoke1((DISPID)0, 
                     &CComVariant(L"{ 'message' : 'fatal error' }"));
    }

    bfs::wpath path = 
        _AtlModule.modulePath / bfs::wpath(L"src") / bfs::wpath(url); 
    logger->debug(L"CNativeControls::button_setURL path: " + path.wstring());

    hr = popup->Navigate(CComBSTR(path.wstring().c_str()));
    if (FAILED(hr)) {
        logger->error(L"CNativeControls::button_setURL error "
                      L" -> " + wstring(uuid) +
                      L" -> " + path.wstring());
        return CComDispatchDriver(error)
            .Invoke1((DISPID)0, &CComVariant(L"{ 'message' : 'failed to set URL' }"));
    }

    // reset onClick
    button_listeners[uuid] = NULL;

    return CComDispatchDriver(success).Invoke0((DISPID)0);
}


/**
 * INativeControls: NativeControls::button_onClicked
 *
 * @param uuid
 */
STDMETHODIMP CNativeControls::button_onClicked(BSTR uuid, IDispatch *callback)
{
    CComQIPtr<IDispatchEx> callbackex(callback);
    if (!callbackex) { 
        logger->error(L"CNativeControls::button_onClicked "
                      L"failed to get IDispatchEx for callback"
                      L" -> " + wstring(uuid) + 
                      L" -> " + boost::lexical_cast<wstring>(callback));
        return E_POINTER;
    }

    this->button_listeners[wstring(uuid)] = callbackex;

    logger->debug(L"CNativeControls::button_onClicked"
                  L" -> " + wstring(uuid) +
                  L" -> " + boost::lexical_cast<wstring>(callback) +
                  L" -> " + boost::lexical_cast<wstring>(this->button_listeners.size()));

    return S_OK;
}


/**
 * INativeControls: NativeControls::button_click
 *
 * @param uuid
 */
STDMETHODIMP CNativeControls::button_click(BSTR uuid, POINT point)
{
    HRESULT hr;

    logger->debug(L"CNativeControls::button_click"
                  L" -> " + wstring(uuid) +
                  L" -> " + boost::lexical_cast<wstring>(point.x) +
                  L" -> " + boost::lexical_cast<wstring>(point.y) +
                  L" -> " + boost::lexical_cast<wstring>(this->button_listeners.size()));

    Listener listener = button_listeners[uuid];
    if (listener) {
        hr = CComDispatchDriver(listener).Invoke0((DISPID)0);
        if (FAILED(hr)) {
            logger->error(L"CNativeControls::button_click "
                          L"failed to invoke listener"
                          L" -> " + wstring(uuid) +
                          L" -> " + boost::lexical_cast<wstring>(listener));
            return hr;
        }
        return S_OK;
    }

    logger->debug(L"CNativeControls::button_click no button listener"
                  L" -> " + wstring(uuid));

    hr = this->popup_visible(uuid, true, point);
    if (FAILED(hr)) {
        logger->error(L"CNativeControls::button_click "
                      L"failed to activate popup"
                      L" -> " + wstring(uuid));
    }

    return S_OK;
}


/**
 * INativeControls: NativeControls::button_setTitle
 *
 * @param uuid
 */
STDMETHODIMP CNativeControls::button_setTitle(BSTR uuid, BSTR title,
                                              IDispatch *success, 
                                              IDispatch *error)
{
    logger->debug(L"CNativeControls::button_setTitle"
                  L" -> " + wstring(uuid) +
                  L" -> " + wstring(title) +
                  L" -> " + boost::lexical_cast<wstring>(success) +
                  L" -> " + boost::lexical_cast<wstring>(error));


    if (!m_frameProxy) {
        logger->debug(L"CNativeControls::button_setTitle invalid FrameProxy");
        return S_OK;
    }

    // dispatch command via FrameProxy
    button_setTitleCommand command(wstring(uuid).c_str(), wstring(title).c_str());
    m_frameProxy->Write(command, sizeof(button_setTitleCommand));

    return CComDispatchDriver(success).Invoke0((DISPID)0);
}


/**
 * INativeControls: NativeControls::button_setBadge
 *
 * @param uuid
 */
STDMETHODIMP CNativeControls::button_setBadge(BSTR uuid, INT number,
                                              IDispatch *success, 
                                              IDispatch *error)
{
    logger->debug(L"CNativeControls::button_setBadge"
                  L" -> " + wstring(uuid) +
                  L" -> " + boost::lexical_cast<wstring>(number) +
                  L" -> " + boost::lexical_cast<wstring>(success) +
                  L" -> " + boost::lexical_cast<wstring>(error));

    if (!m_frameProxy) {
        logger->debug(L"CNativeControls::button_setTitle invalid FrameProxy");
        return S_OK;
    }

    // dispatch command via FrameProxy
    button_setBadgeCommand command(wstring(uuid).c_str(), number);
    m_frameProxy->Write(command, sizeof(button_setBadgeCommand));

    return CComDispatchDriver(success).Invoke0((DISPID)0);
}


/**
 * INativeControls: NativeControls::button_setBadgeBackgroundColor
 *
 * @param uuid
 */
STDMETHODIMP CNativeControls::button_setBadgeBackgroundColor(BSTR uuid, 
                                                             BYTE r, BYTE g, BYTE b, BYTE a,
                                                             IDispatch *success, 
                                                             IDispatch *error)
{
    logger->debug(L"CNativeControls::button_setBadgeBackgroundColor"
                  L" -> " + wstring(uuid) +
                  L" -> " + boost::lexical_cast<wstring>(r) +
                  L" -> " + boost::lexical_cast<wstring>(g) +
                  L" -> " + boost::lexical_cast<wstring>(b) +
                  L" -> " + boost::lexical_cast<wstring>(a) +
                  L" -> " + boost::lexical_cast<wstring>(success) +
                  L" -> " + boost::lexical_cast<wstring>(error));

    if (!m_frameProxy) {
        logger->debug(L"CNativeControls::button_setTitle invalid FrameProxy");
        return S_OK;
    }

    // dispatch command via FrameProxy
    button_setBadgeBackgroundColorCommand command(wstring(uuid).c_str(), r, g, b, a);
    m_frameProxy->Write(command, sizeof(button_setBadgeBackgroundColorCommand));

    return CComDispatchDriver(success).Invoke0((DISPID)0);
}


/**
 * IDispatchImpl::Invoke 
 */
STDMETHODIMP CNativeControls::Invoke(DISPID dispid, 
                                     REFIID riid, 
                                     LCID   lcid, 
                                     WORD   flags, 
                                     DISPPARAMS *params, 
                                     VARIANT    *result, 
                                     EXCEPINFO  *excepinfo, 
                                     UINT       *arg) 
{
    //logger->debug(L"CNativeControls::Invoke");

    if (dispid == DISPID_WINDOWSTATECHANGED) {
        DWORD dflags = params->rgvarg[1].intVal;
        DWORD valid  = params->rgvarg[0].intVal;

        // check whether the event is raised because tab became active
        if ((valid & OLECMDIDF_WINDOWSTATE_USERVISIBLE)  != 0 && 
            (dflags & OLECMDIDF_WINDOWSTATE_USERVISIBLE) != 0 &&
            (valid & OLECMDIDF_WINDOWSTATE_ENABLED)      != 0 && 
            (dflags & OLECMDIDF_WINDOWSTATE_ENABLED)     != 0) {
            logger->debug(L"CNativeControls::Invoke m_frameProxy->SetCurrent");
            if (m_frameProxy) {
                m_frameProxy->SetCurrent();
            }
        }
    }

    return IDispatchImpl<INativeControls, 
                         &IID_INativeControls, 
                         &LIBID_ForgeLib, 
                         1, 
                         0>::Invoke(dispid, riid, lcid, flags, params, 
                                    result, excepinfo, arg);
}

