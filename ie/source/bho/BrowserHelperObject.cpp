#include "stdafx.h"
#include "BrowserHelperObject.h"
#include "HTMLDocument.h"
#include "AccessibleBrowser.h"
#include "dllmain.h"
#include "vendor.h"
#include <util.h>
#include <WindowsMessage.h>
#include <proxy/Commands.h>


/**
 * Static initialization 
 */
_ATL_FUNC_INFO CBrowserHelperObject::OnNavigateComplete2Info = {
    CC_STDCALL, VT_EMPTY, 2, {
        VT_DISPATCH,
        VT_VARIANT | VT_BYREF
    }
};
_ATL_FUNC_INFO CBrowserHelperObject::OnDocumentCompleteInfo = {
    CC_STDCALL, VT_EMPTY, 2, {
        VT_DISPATCH,
        VT_VARIANT | VT_BYREF
    }
};
_ATL_FUNC_INFO CBrowserHelperObject::OnWindowStateChangedInfo = {
    CC_STDCALL, VT_EMPTY, 2, {
        VT_UI4,
        VT_UI4
    }
};

_ATL_FUNC_INFO CBrowserHelperObject::OnBeforeNavigate2Info = {
    CC_STDCALL, VT_EMPTY, 7, {
        VT_DISPATCH,
        VT_VARIANT | VT_BYREF,
        VT_VARIANT | VT_BYREF,
        VT_VARIANT | VT_BYREF,
        VT_VARIANT | VT_BYREF,
        VT_VARIANT | VT_BYREF,
        VT_BOOL | VT_BYREF
    }
};

_ATL_FUNC_INFO CBrowserHelperObject::OnDownloadBeginInfo = {
    CC_STDCALL, VT_EMPTY, 0
};

_ATL_FUNC_INFO CBrowserHelperObject::OnDownloadCompleteInfo = {
    CC_STDCALL, VT_EMPTY, 0
};



/**
 * Construction
 */
CBrowserHelperObject::CBrowserHelperObject()
    : m_isConnected(false),
	  m_isAttached(false),
      m_nativeAccessible(),
      m_frameProxy(NULL),
      m_isPageNavigation(false),
      m_nObjCounter(0),
      m_bIsRefresh(false)
{
    logger->debug(L"\n----------------------------------------------------------");
    logger->debug(L"BrowserHelperObject Initializing...");

    logger->debug(L"BrowserHelperObject::BrowserHelperObject"
                  L" -> " + _AtlModule.moduleManifest->uuid +
                  L" -> " + _AtlModule.moduleCLSID);

    // load script extensions - TODO user level error handling for load failure
    m_scriptExtensions = 
        ScriptExtensions::pointer(new ScriptExtensions(_AtlModule.modulePath));
    if (!m_scriptExtensions->manifest) {
        logger->error(L"Failed to read manifest file: " +
                      m_scriptExtensions->pathManifest.wstring() + 
                      L" at: " + _AtlModule.modulePath.wstring());
        ::MessageBox(NULL,
                     wstring(L"Failed to read manifest. Please check that "
                             L" manifest.json file is present at " +
                             _AtlModule.modulePath.wstring() +
                             L" and properly configured.").c_str(),
                     VENDOR_COMPANY_NAME,
                     MB_TASKMODAL | MB_ICONEXCLAMATION);
        // TODO halt BHO init and exit gracefully
        return;
    }
}


/**
 * Interface: IObjectWithSite::SetSite
 */
STDMETHODIMP CBrowserHelperObject::SetSite(IUnknown *unknown)
{
    HRESULT hr;
    if (unknown == NULL) {
        hr = this->OnDisconnect(unknown);
    } else {
        hr = this->OnConnect(unknown);
    }
    if (FAILED(hr)) {
        logger->error(L"BrowserHelperObject::SetSite failed"
                      L" -> " + logger->parse(hr));
        return hr;
    }
    return IObjectWithSiteImpl<CBrowserHelperObject>::SetSite(unknown);
}


/**
 * Event: OnConnect
 */
HRESULT CBrowserHelperObject::OnConnect(IUnknown *unknown) 
{
    HRESULT hr;

    logger->debug(L"BrowserHelperObject::OnConnect" 
                  L" -> " + _AtlModule.moduleManifest->uuid);

    if (m_isConnected) {
        logger->debug(L"  -> OK already connected");
        return S_OK;
    }

    // helpful values
    wstring uuid = _AtlModule.moduleManifest->uuid;
    wstring path = (_AtlModule.modulePath / 
                    m_scriptExtensions->manifest->background_page).wstring();

    // browsers and interfaces and sinks oh my
    CComQIPtr<IServiceProvider> sp(unknown);
    hr = sp->QueryService(IID_IWebBrowserApp, IID_IWebBrowser2, 
                          (void**)&m_webBrowser2);
    if (FAILED(hr)) {
        logger->debug(L"CBrowserHelperObject::SetSite "
                      L"failed to get IWebBrowser2"
                      L" -> " + logger->parse(hr));
        return hr;
    }
    hr = this->DispEventAdvise(m_webBrowser2);
    if (FAILED(hr)) {
        logger->error(L"BrowserHelperObject::OnConnect "
                      L"failed to sink DWebBrowserEvents2"
                      L" -> " + logger->parse(hr));
        return hr;
    } 
    m_isConnected = true; // TODO deprecate

    // do we need to support a toolbar button for this extension?
    CComPtr<INativeControls> nativeControls;
    wstring    default_title;
    bfs::wpath default_icon;
    default_title = m_scriptExtensions->manifest->browser_action.default_title;
    default_icon  = (_AtlModule.modulePath / 
                     m_scriptExtensions->manifest->browser_action.default_icon).wstring();
    if (m_scriptExtensions->manifest->browser_action.default_icon == L"" ||
        !bfs::exists(default_icon)) {
        logger->info(L"CBrowserHelperObject::OnConnect skipping forge.button.* support");
        goto nativebackground; // return S_OK;
    }

    // get browser HWND
    SHANDLE_PTR phwnd;
    hr = m_webBrowser2->get_HWND(&phwnd);
    if (FAILED(hr)) {
        logger->error(L"CBrowserHelperObject::OnConnect failed to get browser hwnd");
        return hr;
    }
    HWND hwnd = reinterpret_cast<HWND>(phwnd);

    HWND toolbar, target;
    if (!WindowsMessage::GetToolbar(hwnd, &toolbar, &target)) {
        logger->error(L"CBrowserHelperObject::OnConnect failed to get toolbar hwnd");
        goto nativebackground; // TODO this is the place to do any haxoring if we decided to force.enable the CommandBar
    }

    // inject proxy lib into frame process 
    if (!m_frameProxy) {
        m_frameProxy = new FrameProxy(uuid, 
                                      _AtlModule.moduleHandle, 
                                      toolbar, 
                                      target,
                                      default_title, 
                                      default_icon.wstring());
        if (!m_frameProxy->IsOnline()) {
            m_frameProxy = NULL;
            logger->error(L"CBrowserHelperObject::OnConnect failed to connect to FrameProxy");
            goto nativebackground;
        }
    }

    // inform NativeControls of browser's HWND - TODO s/load/advise
    logger->debug(L"CBrowserHelperObject::OnConnect getting NativeControls");
    hr = ::CoInitializeEx(NULL, COINIT_APARTMENTTHREADED); 
    hr = ::CoCreateInstance(CLSID_NativeControls, 
                            NULL,
                            CLSCTX_LOCAL_SERVER, 
                            IID_INativeControls,
                            (LPVOID*)&nativeControls);
    if (FAILED(hr)) {
        logger->error(L"CBrowserHelperObject::OnConnect "
                      L"failed to obtain NativeControls"
                      L" -> " + logger->parse(hr));
        ::CoUninitialize();
        return hr;
    }
    hr = nativeControls->load(CComBSTR(uuid.c_str()), 
                              CComBSTR(_AtlModule.modulePath.wstring().c_str()),
                              m_instanceId, (ULONG)hwnd);
    if (FAILED(hr)) {
        logger->error(L"CBrowserHelperObject::OnConnect "
                      L"failed to inform NativeControls"
                      L" -> " + logger->parse(hr));
        ::CoUninitialize();
        return hr;
    }
    ::CoUninitialize();

 nativebackground:
    // attach background page
    hr = Attach::NativeBackground(uuid, path,
                                  _AtlModule.moduleManifest->logging.console,
                                  &m_nativeBackground.p, // "T** operator&() throw()" asserts on p==NULL
                                  &m_instanceId); 
    if (FAILED(hr)) {
        logger->error(L"BrowserHelperObject::OnConnect "
                      L"failed to attach background page"
                      L" -> " + logger->parse(hr));
        return hr;
    }         

    return S_OK;
}


/**
 * Event: OnDisconnect
 */
HRESULT CBrowserHelperObject::OnDisconnect(IUnknown *unknown) 
{
    logger->debug(L"BrowserHelperObject::OnDisconnect()"
                  L" -> " + boost::lexical_cast<wstring>(m_instanceId) +
                  L" -> " + _AtlModule.moduleManifest->uuid);
    
    HRESULT hr;

    // NativeExtensions
    //logger->debug(L"BrowserHelperObject::OnDisconnect() release NativeExtensions");

    // NativeMessaging
    logger->debug(L"BrowserHelperObject::OnDisconnect() release NativeMessaging");
    if (m_nativeMessaging) {
        m_nativeMessaging->unload(CComBSTR(_AtlModule.moduleManifest->uuid.c_str()), 
                                  m_instanceId);
    }

    // NativeAccessible
    logger->debug(L"BrowserHelperObject::OnDisconnect() release NativeAccessible");
    if (m_nativeAccessible) m_nativeAccessible.reset();

    // NativeBackground
    logger->debug(L"BrowserHelperObject::OnDisconnect() release NativeBackground"); 
    if (m_nativeBackground) {
        hr = m_nativeBackground->unload(CComBSTR(_AtlModule.moduleManifest->uuid.c_str()), 
                                        m_instanceId);
        if (FAILED(hr)) {
            logger->warn(L"BrowserHelperObject::OnDisconnect "
                         L"failed to unload NativeBackground"
                         L" -> " + boost::lexical_cast<wstring>(m_instanceId) +
                         L" -> " + logger->parse(hr));
        }
    }

    // BrowserHelperObject    
    logger->debug(L"BrowserHelperObject::OnDisconnect() release BHO");    
    if (m_webBrowser2 && m_isConnected) {
        hr = this->DispEventUnadvise(m_webBrowser2); 
        if (hr == CONNECT_E_NOCONNECTION) { 
            // IE7 disconnects our connection points _before_ calling us with SetSite(NULL)
            logger->warn(L"BrowserHelperObject::OnDisconnect DispEventUnadvise bug");
            hr = S_OK; 
        } else if (FAILED(hr)) { 
            logger->error(L"BrowserHelperObject::OnDisconnect failed to unsink events"
                          L" -> " + logger->parse(hr));
        } else {
            logger->debug(L"BrowserHelperObject::OnDisconnect unsunk events");
        }
        m_isConnected = false;
    }

    logger->debug(L"BrowserHelperObject::OnDisconnect() done");    

    return S_OK;
}


/**
 * Helper: MatchManifest -> IWebBrowser2 -> Manifest -> location -> (styles . scripts)
 */
std::pair<wstringvector, wstringvector> 
CBrowserHelperObject::MatchManifest(IWebBrowser2 *webBrowser2,
                                    const Manifest::pointer& manifest, 
                                    const wstring& location)
{
    // Do nothing for special urls
    if (!(location.find(L"http:")  == 0 ||
          location.find(L"https:") == 0 ||
          location.find(L"file:")  == 0 /*||
		  location.find(L"about:") == 0*/)) {
        logger->debug(L"BrowserHelperObject::MatchManifest boring url"
                      L" -> " + location);        
        return std::pair<wstringvector, wstringvector>();
    }

    // is this a html document?
    CComPtr<IDispatch> disp;
    webBrowser2->get_Document(&disp);
    if (!disp) {
        logger->debug(L"BrowserHelperObject::MatchManifest "
                      L"no document");
        return std::pair<wstringvector, wstringvector>();
    }
    CComQIPtr<IHTMLDocument2, &IID_IHTMLDocument2> htmlDocument2(disp);
    if (!htmlDocument2) {
        logger->debug(L"BrowserHelperObject::MatchManifest "
                      L"not an IHTMLDocument2");
        return std::pair<wstringvector, wstringvector>();
    }    

    // is this an iframe ?
    wstring locationtop = location;
    bool is_not_iframe = m_webBrowser2.IsEqualObject(webBrowser2);
    if (is_not_iframe) {
    } else {
        CComPtr<IDispatch> disp_parent;
        HRESULT hr = webBrowser2->get_Parent(&disp_parent);
        if (FAILED(hr) || !disp_parent) {
            logger->debug(L"BrowserHelperObject::MatchManifest not an iframe 1"
                          L" -> " + location);
            return std::pair<wstringvector, wstringvector>();
        }
        CComQIPtr<IHTMLDocument2, &IID_IHTMLDocument2> htmlDocument2_parent(disp_parent);
        if (!htmlDocument2_parent) {
            logger->debug(L"BrowserHelperObject::MatchManifest not an iframe 2"
                          L" -> " + location);
            return std::pair<wstringvector, wstringvector>();
        }
        CComBSTR bstr;
        htmlDocument2_parent->get_URL(&bstr);
        locationtop = bstr;
    }

    // check content_scripts.matches, content_scripts.all_frames
    std::pair<wstringvector, wstringvector> ret;
    Manifest::ContentScripts::const_iterator script = manifest->content_scripts.begin();
    for (; script != manifest->content_scripts.end(); script++) {

		// check for exclude_matches
		bool exclude = false;
		wstringvector::const_iterator exclude_match = script->exclude_matches.begin();
		for (; exclude_match != script->exclude_matches.end(); exclude_match++) {
			if (wstring_match_wild(*exclude_match, location)) {
				logger->debug(L"BrowserHelperObject::MatchManifest exclude "
							  L" -> " + *exclude_match + L" -> " + location);
				exclude = true;
			}
		}

		// check for matches and add if, and only if there are no excludes for this activation
        wstringvector::const_iterator match = script->matches.begin();
        for (; match != script->matches.end(); match++) {
            bool matches = wstring_match_wild(*match, location);
            if ((matches && is_not_iframe && !exclude) ||        // matches top-level ||
                (matches && script->all_frames && !exclude)) {   // matches iframe  
                ret.first.insert(ret.first.end(), script->css.begin(), script->css.end());
                ret.second.insert(ret.second.end(), script->js.begin(), script->js.end());
            }
        }
    }

    return ret;
}


/**
 * Event: OnNavigateComplete2
 */
void __stdcall CBrowserHelperObject::OnNavigateComplete2(IDispatch *dispatch,
                                                         VARIANT   *url) 
{
    CComQIPtr<IWebBrowser2, &IID_IWebBrowser2> webBrowser2(dispatch);
    if (!webBrowser2) {
        logger->debug(L"BrowserHelperObject::OnNavigateComplete2 "
                      L"failed to obtain IWebBrowser2");
        return;
    }

    // VARIANT *url chops off file:\\\ for local filesystem
    CComBSTR bstr;
    webBrowser2->get_LocationURL(&bstr); 
    wstring location(bstr);
	if (location == L"" && url->bstrVal) { // get_LocationURL fails in the most egregious of ways
		location = url->bstrVal;
	}

    // match location against manifest 
    Manifest::pointer manifest = _AtlModule.moduleManifest;
    std::pair<wstringvector, wstringvector> match = this->MatchManifest(webBrowser2, manifest, location);
    if (match.first.size() == 0 && match.second.size() == 0) {
        logger->debug(L"BrowserHelperObject::OnNavigateComplete2 not interested"
                      L" -> " + manifest->uuid + 
					  L" -> " + wstring(url->bstrVal) +
                      L" -> " + location);
        return;
    }

    logger->debug(L"BrowserHelperObject::OnNavigateComplete2"
                  L" -> " + manifest->uuid +
                  L" -> " + location);

	// attach forge extensions
	this->OnAttachForgeExtensions(dispatch, location, L"OnNavigateComplete2");
}


/**
 * Event: OnDocumentComplete
 */
void __stdcall CBrowserHelperObject::OnDocumentComplete(IDispatch *dispatch,
                                                        VARIANT   *url)
{

	m_isPageNavigation = false;

    Manifest::pointer manifest = _AtlModule.moduleManifest;

    CComQIPtr<IWebBrowser2, &IID_IWebBrowser2> webBrowser2(dispatch);
    if (!webBrowser2) {
        logger->debug(L"BrowserHelperObject::OnDocumentComplete "
                      L"failed to obtain IWebBrowser2");
        return;
    } 

    // VARIANT *url chops off file:\\\ for local filesystem
    CComBSTR bstr;
    webBrowser2->get_LocationURL(&bstr); 
    wstring location(bstr);
	if (location == L"" && url->bstrVal) { // get_LocationURL fails in the most egregious of ways
		location = url->bstrVal;
	}
	if (location == L"" && url->bstrVal == NULL) {
        logger->debug(L"BrowserHelperObject::OnDocumentComplete "
					  L"blank location, not interested"
                      L" -> " + manifest->uuid +
					  L" -> " + location);
		return;
	}

    // match location against manifest 
    std::pair<wstringvector, wstringvector> match = this->MatchManifest(webBrowser2, manifest, location);
    if (match.first.size() == 0 && match.second.size() == 0) {
        logger->debug(L"BrowserHelperObject::OnDocumentComplete not interested"
                      L" -> " + manifest->uuid +
					  L" -> " + wstring(url->bstrVal) +
                      L" -> " + location);
        return;
    }

    logger->debug(L"BrowserHelperObject::OnDocumentComplete"
                  L" -> " + manifest->uuid +
				  L" -> " + wstring(url->bstrVal) +
                  L" -> " + location);

    // get IHTMLWindow2
    CComPtr<IDispatch> disp;
    webBrowser2->get_Document(&disp);
    if (!disp) {
        logger->debug(L"BrowserHelperObject::OnDocumentComplete get_Document failed");
        return;
    }
    CComQIPtr<IHTMLDocument2, &IID_IHTMLDocument2> htmlDocument2(disp);
    if (!htmlDocument2) {
        logger->debug(L"BrowserHelperObject::OnDocumentComplete IHTMLDocument2 failed");
        return;
    }
    CComQIPtr<IHTMLWindow2, &IID_IHTMLWindow2> htmlWindow2;
    htmlDocument2->get_parentWindow(&htmlWindow2);    
    if (!htmlWindow2) {
        logger->debug(L"BrowserHelperObject::OnDocumentComplete IHTMLWindow2 failed");
        return;
    }

	// attach forge extensions when pages like target=_blank didn't trigger navComplete event
	if (!m_isAttached) {
		this->OnAttachForgeExtensions(dispatch, location, L"OnDocumentComplete");
	}

    // Inject styles
    wstringset dupes;
    HTMLDocument document(webBrowser2); 
    ScriptExtensions::scriptvector matches = m_scriptExtensions->read_styles(match.first);
    ScriptExtensions::scriptvector::const_iterator i = matches.begin();
    for (; i != matches.end(); i++) {
        if (dupes.find(i->first) != dupes.end()) {
            logger->debug(L"BrowserHelperObject::OnDocumentComplete already injected -> " + i->first);
            continue;
        }
        wstringpointer style = i->second;
        if (!style) {
            logger->debug(L"BrowserHelperObject::OnDocumentComplete invalid stylesheet -> " + i->first);
            continue;
        }
        HRESULT hr;
        hr = document.InjectStyle(style);
        if (FAILED(hr)) {
            logger->error(L"BrowserHelperObject::OnDocumentComplete failed to inject style"
                          L" -> " + i->first +
                          L" -> " + logger->parse(hr));
            continue;
        }
        dupes.insert(i->first);
        logger->debug(L"BrowserHelperObject::OnDocumentComplete injected: " + i->first);
    }    

    // Inject scripts
    dupes.clear();
    matches = m_scriptExtensions->read_scripts(match.second);
    i = matches.begin();
    for (; i != matches.end(); i++) {
        if (dupes.find(i->first) != dupes.end()) {
            logger->debug(L"BrowserHelperObject::OnDocumentComplete already injected -> " + i->first);
            continue;
        }
        wstringpointer script = i->second;
        if (!script) {
            logger->debug(L"BrowserHelperObject::OnDocumentComplete invalid script -> " + i->first);
            continue;
        }
        HRESULT hr;
        //hr = document.InjectScript(script);
        //hr = document.InjectScriptTag(HTMLDocument::attrScriptType, i->first);
        CComVariant ret;
        hr = htmlWindow2->execScript(CComBSTR(script->c_str()), L"javascript", &ret);
        if (FAILED(hr)) {
            logger->error(L"BrowserHelperObject::OnDocumentComplete failed to inject script"
                          L" -> " + i->first +
                          L" -> " + logger->parse(hr));
            continue;
        }
        dupes.insert(i->first);
        logger->debug(L"BrowserHelperObject::OnDocumentComplete injected"
                      L" -> " + location +
                      L" -> " + i->first);
    }

    /*// Test in-process IAccessible access
    SHANDLE_PTR phwnd;
    hr = webBrowser2->get_HWND(&phwnd);
    HWND hwnd = reinterpret_cast<HWND>(phwnd);
    AccessibleBrowser ab(hwnd);
    wstringvector tabs = ab.tabs();
    for (wstringvector::const_iterator tab = tabs.begin(); tab != tabs.end(); tab++) {
        logger->debug(L"BrowserHelperObject::OnDocumentComplete -> " + *tab);
        }*/
}


/**
 * Event: OnAttachForgeExtensions
 */
void __stdcall CBrowserHelperObject::OnAttachForgeExtensions(IDispatch *dispatch, 
															 const wstring& location,
															 const wstring& eventsource)
{
	m_isAttached = false;

	CComQIPtr<IWebBrowser2, &IID_IWebBrowser2> webBrowser2(dispatch);
    if (!webBrowser2) {
        logger->debug(L"BrowserHelperObject::OnAttachForgeExtensions "
                      L"failed to obtain IWebBrowser2");
        return;
    } 

	logger->debug(L"BrowserHelperObject::OnAttachForgeExtensions"
				  L" -> " + boost::lexical_cast<wstring>(dispatch) +
				  L" -> " + location +
				  L" -> " + eventsource);

    Manifest::pointer manifest = _AtlModule.moduleManifest;

    // get interfaces
    CComPtr<IDispatch> disp;
    webBrowser2->get_Document(&disp);
    if (!disp) {
        logger->debug(L"BrowserHelperObject::OnAttachForgeExtensions get_Document failed");
        return;
    }
	logger->debug(L"BrowserHelperObject::OnAttachForgeExtensions "
				  L" IDispatch -> " + boost::lexical_cast<wstring>(disp)); 
    CComQIPtr<IHTMLDocument2, &IID_IHTMLDocument2> htmlDocument2(disp);
    if (!htmlDocument2) {
        logger->debug(L"BrowserHelperObject::OnAttachForgeExtensions IHTMLDocument2 failed");
        return;
    }
	logger->debug(L"BrowserHelperObject::OnAttachForgeExtensions "
				  L" IHTMLDocument2 -> " + boost::lexical_cast<wstring>(htmlDocument2)); 
    CComQIPtr<IHTMLWindow2, &IID_IHTMLWindow2> htmlWindow2;
    htmlDocument2->get_parentWindow(&htmlWindow2);    
    if (!htmlWindow2) {
        logger->debug(L"BrowserHelperObject::OnAttachForgeExtensions IHTMLWindow2 failed");
        return;
    }
	logger->debug(L"BrowserHelperObject::OnAttachForgeExtensions "
				  L" IHTMLWindow2 -> " + boost::lexical_cast<wstring>(htmlWindow2)); 
    CComQIPtr<IDispatchEx> htmlWindow2Ex(htmlWindow2);
    if (!htmlWindow2Ex) {
        logger->debug(L"BrowserHelperObject::OnAttachForgeExtensions IHTMLWindow2Ex failed");
        return;
    }

    HRESULT hr;

    // Attach NativeAccessible (forge.tabs.*)
    if (m_nativeAccessible) {
        logger->error(L"BrowserHelperObject::OnAttachForgeExtensions resetting nativeAccessible");
        m_nativeAccessible.reset();
    }
    m_nativeAccessible = NativeAccessible::pointer(new NativeAccessible(webBrowser2));
    hr = Attach::NativeTabs(htmlWindow2Ex, 
                            L"accessible",
                            m_nativeAccessible.get());
    if (FAILED(hr)) {
        logger->error(L"BrowserHelperObject::OnAttachForgeExtensions "
                      L"failed to attach NativeExtensions"
                      L" -> " + logger->parse(hr));
        return;
    }    

    // Attach NativeExtensions
    hr = Attach::NativeExtensions(manifest->uuid,
                                  htmlWindow2Ex, 
                                  L"extensions", 
                                  m_instanceId,
                                  location,
                                  &m_nativeExtensions.p); // "T** operator&() throw()" asserts on p==NULL
    if (FAILED(hr)) {
        logger->error(L"BrowserHelperObject::OnAttachForgeExtensions "
                      L"failed to attach NativeExtensions"
                      L" -> " + logger->parse(hr));
        return;
    }
	logger->debug(L"BrowserHelperObject::OnAttachForgeExtensions "
				  L"attached NativeExtensions");

    // Attach NativeMessaging
    hr = Attach::NativeMessaging(manifest->uuid,
                                 htmlWindow2Ex, 
                                 L"messaging", 
                                 m_instanceId,
                                 &m_nativeMessaging.p); // "T** operator&() throw()" asserts on p==NULL
    if (FAILED(hr)) {
        logger->error(L"BrowserHelperObject::OnAttachForgeExtensions "
                      L"failed to attach NativeMessaging"
                      L" -> " + logger->parse(hr));
        return;
    }

	logger->debug(L"BrowserHelperObject::OnAttachForgeExtensions "
				  L"attached NativeMessaging");

	m_isAttached = true;
}


/**
 * Event: OnWindowStateChanged
 */
void __stdcall CBrowserHelperObject::OnWindowStateChanged(DWORD flags, DWORD mask)
{
    CComBSTR url;
    m_webBrowser2->get_LocationURL(&url);
    CComBSTR title;
    m_webBrowser2->get_LocationName(&title);
    /*logger->debug(L"CBrowserHelperObject::OnWindowStateChanged" 
                  L" -> " + boost::lexical_cast<wstring>(flags) +
                  L" -> " + boost::lexical_cast<wstring>(m_instanceId) +
                  L" -> " + wstring(url) +
                  L" -> " + wstring(title));*/
        
    bool focused = false;
    switch (flags) {
    case OLECMDIDF_WINDOWSTATE_USERVISIBLE:   break;
    case OLECMDIDF_WINDOWSTATE_ENABLED:       break;
    case 3:   // MSDN docs are _WRONG_
        focused = true;
        break;
    }

    m_tabInfo.id     = m_instanceId;
    m_tabInfo.active = focused;
    m_tabInfo.url    = url;
    m_tabInfo.title  = title;

    // update tab info
    if (m_nativeMessaging) {
        HRESULT hr;
        hr = m_nativeMessaging->tabs_set(CComBSTR(_AtlModule.moduleManifest->uuid.c_str()), 
                                         m_tabInfo.id, 
                                         CComBSTR(m_tabInfo.url.c_str()), 
                                         CComBSTR(m_tabInfo.title.c_str()), 
                                         m_tabInfo.active);
        if (FAILED(hr)) {
            logger->error(L"BrowserHelperObject::OnWindowStateChanged "
                          L"tabs_Set failed "
                          L" -> " + boost::lexical_cast<wstring>(m_tabInfo.id) +
                          L" -> " + m_tabInfo.url +
                      L" -> " + m_tabInfo.title + 
                          L" -> " + boost::lexical_cast<wstring>(m_tabInfo.active) +
                          L" -> " + logger->parse(hr));
            return;
        }
    }
}


/**
* Event: OnBeforeNavigate2
*/
void __stdcall CBrowserHelperObject::OnBeforeNavigate2(IDispatch *idispatch, VARIANT *url, VARIANT *Flags, VARIANT *TargetFrameName, VARIANT *PostData, VARIANT *Headers, VARIANT_BOOL* Cancel)
{
  // add page flag so we can compare in DownloadBegin function
  m_isPageNavigation = true;

  LPDISPATCH lpWBDisp = nullptr;
  HRESULT hr = S_OK;
  for (;;) {
    BreakOnNull(url, hr);
    // capture relevant information on the URL etc we are about to load.
    BSTR strUrl = (BSTR)url->bstrVal;
    BreakOnNull(m_webBrowser2, hr);
    // get web browser Dispatch to see if this is the top level URL call
    m_webBrowser2->QueryInterface(IID_IDispatch, (void**)&lpWBDisp);
    BreakOnNull(lpWBDisp, hr);
    if (idispatch == lpWBDisp && !m_bIsRefresh) {
      // Top-level Window object, so store URL for a refresh request
      m_strUrl = strUrl;
    }
    lpWBDisp->Release();
    break;
  }
}


/**
* Event: OnDownloadBegin
*/
void __stdcall CBrowserHelperObject::OnDownloadBegin()
{
  if (!m_isPageNavigation)
    m_bIsRefresh = true;
  
  ++m_nObjCounter;
}

/**
* Event: OnDownloadComplete
*/
void __stdcall CBrowserHelperObject::OnDownloadComplete()
{
  // decrease counter to compare with DownloadBegin
  --m_nObjCounter;

  // if m_nObjCounter is Zero and we are in Refresh mode we know that the refreshed page has loaded.
  if (m_bIsRefresh && m_nObjCounter == 0) {
    OnRefresh();
    m_bIsRefresh = false;
  }
}


 // Implement refresh(F5 click, etc) based on
// http://www.codeproject.com/Articles/3632/Detecting-the-IE-Refresh-button-using-IWebBrowser2
void CBrowserHelperObject::OnRefresh()
{
   auto manifest = _AtlModule.moduleManifest;
   wstring location;
   wstringset dupes;

   CComBSTR bstr = nullptr;
   CComPtr<IDispatch> disp = nullptr;
   CComQIPtr<IHTMLWindow2, &IID_IHTMLWindow2> htmlWindow2 = nullptr;
   
   HRESULT hr = S_OK;
   
   for (;;) {
     // VARIANT *url chops off file:\\\ for local filesystem    
     CComQIPtr<IWebBrowser2, &IID_IWebBrowser2> webBrowser2(m_webBrowser2);
     BreakOnNull(webBrowser2, hr);
     hr = webBrowser2->get_LocationURL(&bstr);
     BreakOnFailed(hr);
     location = wstring(bstr); // was m_strUrl
     if (location.empty()) {
       logger->debug(L"BrowserHelperObject::OnRefresh blank location, not interested  -> " + manifest->uuid + L" -> " + location);
       break;
     }

     // match location against manifest 
     auto& match = MatchManifest(webBrowser2, manifest, location);
     if (match.first.empty() && match.second.empty()) {
       logger->debug(L"BrowserHelperObject::OnRefresh not interested -> " + manifest->uuid + L" -> " + location);
       break;
     }
     logger->debug(L"BrowserHelperObject::OnRefresh -> " + manifest->uuid + L" -> " + location);

     // get IHTMLWindow2
     hr = webBrowser2->get_Document(&disp);
     BreakOnNullWithErrorLog(disp, L"BrowserHelperObject::OnRefresh get_Document failed");
     BreakOnFailed(hr);

     CComQIPtr<IHTMLDocument2, &IID_IHTMLDocument2> htmlDocument2(disp);
     BreakOnNullWithErrorLog(htmlDocument2, L"BrowserHelperObject::OnRefresh IHTMLDocument2 failed");

     hr = htmlDocument2->get_parentWindow(&htmlWindow2);
     BreakOnNullWithErrorLog(htmlWindow2, L"BrowserHelperObject::OnRefresh IHTMLWindow2 failed");
     BreakOnFailed(hr);

     // attach forge extensions when pages like target=_blank didn't trigger navComplete event
     if (!m_isAttached)
       OnAttachForgeExtensions(m_webBrowser2, location, L"OnRefresh"); // was L"OnDocumentComplete"

     // Inject styles
     HTMLDocument document(webBrowser2);
     auto& matches = m_scriptExtensions->read_styles(match.first);
     for (auto& i : matches) {
       if (dupes.find(i.first) != dupes.end()) {
         logger->debug(L"BrowserHelperObject::OnRefresh already injected -> " + i.first);
         continue;
       }
       auto style = i.second;
       if (!style) {
         logger->debug(L"BrowserHelperObject::OnRefresh invalid stylesheet -> " + i.first);
         continue;
       }
       hr = document.InjectStyle(style);
       if (FAILED(hr)) {
         logger->error(L"BrowserHelperObject::OnRefresh failed to inject style -> " + i.first + L" -> " + logger->parse(hr));
         continue;
       }
       dupes.insert(i.first);
       logger->debug(L"BrowserHelperObject::OnRefresh injected: " + i.first);
     }

     // Inject scripts
     dupes.clear();
     matches = m_scriptExtensions->read_scripts(match.second);
     for (auto& i : matches) {
       if (dupes.find(i.first) != dupes.end()) {
         logger->debug(L"BrowserHelperObject::OnRefresh already injected -> " + i.first);
         continue;
       }
       auto script = i.second;
       if (!script) {
         logger->debug(L"BrowserHelperObject::OnRefresh invalid script -> " + i.first);
         continue;
       }
       CComVariant ret;
       hr = htmlWindow2->execScript(CComBSTR(script->c_str()), L"javascript", &ret);
       if (FAILED(hr)) {
         logger->error(L"BrowserHelperObject::OnRefresh failed to inject script -> " + i.first + L" -> " + logger->parse(hr));
         continue;
       }
       dupes.insert(i.first);
       logger->debug(L"BrowserHelperObject::OnRefresh injected -> " + location + L" -> " + i.first);
     }

     break;
   }   
}


/**
 * IDispatchImpl (for FrameProxy)
 */
STDMETHODIMP CBrowserHelperObject::Invoke(DISPID dispid, 
                                          REFIID riid, 
                                          LCID   lcid, 
                                          WORD   flags, 
                                          DISPPARAMS *params, 
                                          VARIANT    *result, 
                                          EXCEPINFO  *excepinfo, 
                                          UINT       *arg) 
{
    if (dispid == DISPID_WINDOWSTATECHANGED) {
        DWORD dflags = params->rgvarg[1].intVal;
        DWORD valid  = params->rgvarg[0].intVal;
        
        // check whether the event is raised because tab became active
        if ((valid & OLECMDIDF_WINDOWSTATE_USERVISIBLE)  != 0 && 
            (dflags & OLECMDIDF_WINDOWSTATE_USERVISIBLE) != 0 &&
            (valid & OLECMDIDF_WINDOWSTATE_ENABLED)      != 0 && 
            (dflags & OLECMDIDF_WINDOWSTATE_ENABLED)     != 0) {
            //logger->debug(L"CBrowserHelperObject::Invoke m_frameProxy->SetCurrent");
            if (m_frameProxy) {
                m_frameProxy->SetCurrent();
            }
        }
	} else if (dispid == DISPID_COMMANDSTATECHANGE) {
		// if we hover over a window that wasn't active and click on the extension icon
		if (m_frameProxy) {
            m_frameProxy->SetCurrent();
        }
	}

    // forward invocation
    HRESULT hr;
    hr = WebBrowserEvents2::Invoke(dispid, riid, lcid, flags, params, 
                                   result, excepinfo, arg);
    if (FAILED(hr)) {
        logger->error(L"CBrowserHelperObject::Invoke failed"
                      L" -> " + logger->parse(hr));
    }

    return hr;
}

