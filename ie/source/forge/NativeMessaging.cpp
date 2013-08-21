#include "stdafx.h"
#include "NativeMessaging.h"


/**
 * Construction
 */
CNativeMessaging::CNativeMessaging()
{
}


/** 
 * Destruction
 */
CNativeMessaging::~CNativeMessaging()
{
    logger->debug(L"CNativeMessaging::~CNativeMessaging");
}

HRESULT CNativeMessaging::FinalConstruct()
{
    return S_OK;
}

void CNativeMessaging::FinalRelease()
{
    logger->debug(L"CNativeMessaging::FinalRelease");
}


/**
 * InterfaceSupportsErrorInfo
 */
STDMETHODIMP CNativeMessaging::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* const arr[] = 
	{
		&IID_INativeMessaging
	};

	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}


/**
 * Method: NativeMessaging::tabs_set
 *
 * Called by the browser whenever the active tab changes 
 *
 * @param uuid
 * @param tabInfo
 */
STDMETHODIMP CNativeMessaging::tabs_set(BSTR uuid, 
                                        UINT instanceId,
                                        BSTR url, BSTR title, BOOL focused)
{
    /*logger->debug(L"NativeMessaging::tabs_set"
                  L" -> " + wstring(uuid) +
                  L" -> " + boost::lexical_cast<wstring>(instanceId) +
                  L" -> " + wstring(url) +
                  L" -> " + wstring(title) +
                  L" -> " + boost::lexical_cast<wstring>(focused));*/

    if (focused) {
        m_activeTab.id       = instanceId;
        m_activeTab.url      = url;
        m_activeTab.title    = title;
        m_activeTab.selected = focused ? true : false;
    }

    // TODO save tab info

    return S_OK;
}


/**
 * Method: NativeMessaging::tabs_active
 *
 * @param uuid
 *
 * @returns Tab
 */
STDMETHODIMP CNativeMessaging::tabs_active(BSTR uuid, IDispatch *callback, UINT *out_tabId)
{
    HRESULT hr;

    /*logger->debug(L"NativeMessaging::tabs_active"
                  L" -> " + wstring(uuid) +
                  L" -> " + boost::lexical_cast<wstring>(callback));*/

    *out_tabId = m_activeTab.id;

    if (!callback) {
        logger->error(L"NativeMessaging::tabs_active no callback");
        return S_OK;
    }

    CComPtr<ITypeInfo> tabT;
    hr = ::CreateDispTypeInfo(&Tab::Interface, LOCALE_SYSTEM_DEFAULT,
                              &tabT);
    if (FAILED(hr) || !tabT) {
        logger->error(L"NativeMessaging::tabs_active "
                      L"failed to create tabT"
                      L" -> " + logger->parse(hr));
        return hr;
    }
    CComPtr<IUnknown> tabI;
    hr = ::CreateStdDispatch(NULL, &m_activeTab, tabT, &tabI);
    if (FAILED(hr) || !tabI) {
        logger->error(L"NativeMessaging::tabs_active "
                      L"failed to create tabI"
                      L" -> " + logger->parse(hr));
        return hr;
    }

    hr = CComDispatchDriver(callback).Invoke1((DISPID)0, &CComVariant(tabI));
    if (FAILED(hr)) {    
        logger->error(L"NativeMessaging::tabs_active "
                      L"failed to invoke callback"
                      L" -> " + logger->parse(hr));
    }

    return hr;
}


/**
 * Method: NativeMessaging::load
 *
 * @param uuid
 * @param instance
 */
STDMETHODIMP CNativeMessaging::load(BSTR uuid, unsigned int instanceId)
{
    /*logger->debug(L"CNativeMessaging::load"
                  L" -> " + wstring(uuid) +
                  L" -> " + boost::lexical_cast<wstring>(instanceId));*/
    
    m_clients[uuid].insert(instanceId);

    return S_OK;
}


/**
 * Method: NativeMessaging::unload
 *
 * @param uuid
 * @param instance
 */
struct delete_callback { 
    delete_callback(UINT tabId) : tabId(tabId) {}
    UINT tabId;
    bool operator()(Callback::pointer callback) const {
        logger->debug(L"CNativeMessaging::unload checking callback "
                      L" -> " + boost::lexical_cast<wstring>(tabId) +
                      L" -> " + callback->toString());
        if (callback->tabId == tabId) {
            logger->debug(L"\terased");
            return true;
        }
        return false;
    }
};

STDMETHODIMP CNativeMessaging::unload(BSTR uuid, unsigned int instanceId)
{
    logger->debug(L"CNativeMessaging::unload"
                  L" -> " + wstring(uuid) +
                  L" -> " + boost::lexical_cast<wstring>(instanceId));

    // clean up any callbacks registered for this instance
    Callback::vector v = fg_callbacks[uuid];
    v.erase(std::remove_if(v.begin(), v.end(), delete_callback(instanceId)), v.end());
    fg_callbacks[uuid] = v;

    m_clients[uuid].erase(instanceId);
    if (m_clients[uuid].empty()) {
        logger->debug(L"CNativeMessaging::unload "
                      L"shutting down extension"
                      L" -> " + wstring(uuid));
        this->bg_callbacks.erase(uuid);
        this->fg_callbacks.erase(uuid);
        m_clients.erase(uuid);
        logger->debug(L"CNativeMessaging::unload done");
    }

    logger->debug(L"CNativeMessaging::unload clients remaining: " +
                  boost::lexical_cast<wstring>(m_clients.size()));

    if (m_clients.empty()) {
        return this->shutdown();
    } 

    return S_OK;
}


/**
 * Helper: shutdown
 */
HRESULT CNativeMessaging::shutdown()
{
    logger->debug(L"CNativeMessaging::shutdown "
                  L" -> " + boost::lexical_cast<wstring>(m_dwRef));

    this->bg_callbacks.clear();
    this->fg_callbacks.clear();
    
    return S_OK;
}


/**
 * Method: fg_listen
 *
 * FG <= BG
 */
STDMETHODIMP CNativeMessaging::fg_listen(BSTR _uuid, UINT tabId,
                                         BSTR _type, 
                                         IDispatch *callback, IDispatch *error)
{
    wstring uuid(_uuid), type(_type);

    /*logger->debug(L"CNativeMessaging::fg_listen"
                  L" -> " + uuid +
                  L" -> " + boost::lexical_cast<wstring>(tabId) +
                  L" -> " + type +
                  L" -> " + boost::lexical_cast<wstring>(callback) +
                  L" -> " + boost::lexical_cast<wstring>(error));*/

    this->fg_callbacks[uuid].push_back
        (Callback::pointer(new Callback(tabId, type, callback, error)));

    return S_OK;
}


/**
 * Method: fg_broadcast
 *
 * FG => FG(*)
 */
STDMETHODIMP CNativeMessaging::fg_broadcast(BSTR _uuid, 
                                            BSTR _type, BSTR _content, 
                                            IDispatch *callback, IDispatch *error)
{
    wstring uuid(_uuid), type(_type), content(_content);

    /*logger->debug(L"CNativeMessaging::fg_broadcast"
                  L" -> " + uuid +
                  L" -> " + type +
                  L" -> " + content +
                  L" -> " + boost::lexical_cast<wstring>(callback) +
                  L" -> " + boost::lexical_cast<wstring>(error));*/

    HRESULT hr;
    Callback::vector v = fg_callbacks[uuid];
    for (Callback::vector::const_iterator i = v.begin(); i != v.end(); i++) {
        Callback::pointer fg_callback = *i;
        if (fg_callback->type == L"*" || 
            fg_callback->type == type) {
            hr = fg_callback->Dispatch(content, callback);
        }
    }    

    return S_OK;
}


/**
 * Method: fg_toFocussed
 *
 * FG => FG(1)
 */
STDMETHODIMP CNativeMessaging::fg_toFocussed(BSTR _uuid, 
                                             BSTR _type, BSTR _content, 
                                             IDispatch *callback, IDispatch *error)
{
    wstring uuid(_uuid), type(_type), content(_content);

    /*logger->debug(L"CNativeMessaging::fg_toFocussed"
                  L" -> " + uuid +
                  L" -> " + type +
                  L" -> " + content +
                  L" -> " + boost::lexical_cast<wstring>(callback) +
                  L" -> " + boost::lexical_cast<wstring>(error));*/

    HRESULT hr;
    Callback::vector v = fg_callbacks[uuid];
    for (Callback::vector::const_iterator i = v.begin(); i != v.end(); i++) {
        Callback::pointer fg_callback = *i;
        if (fg_callback->tabId != m_activeTab.id) continue; 
        if (fg_callback->type == L"*" || 
            fg_callback->type == type) {
            hr = fg_callback->Dispatch(content, callback);
        }
    }    

    return S_OK;
}

/**
 * Method: fg_broadcastBackround
 *
 * FG => BG
 */
STDMETHODIMP CNativeMessaging::fg_broadcastBackground(BSTR _uuid, 
                                                      BSTR _type, BSTR _content, 
                                                      IDispatch *callback, IDispatch *error)
{
    wstring uuid(_uuid), type(_type), content(_content);

    /*logger->debug(L"CNativeMessaging::fg_broadcastBackground"
                  L" -> " + uuid +
                  L" -> " + type +
                  L" -> " + content +
                  L" -> " + boost::lexical_cast<wstring>(callback) +
                  L" -> " + boost::lexical_cast<wstring>(error));*/

    HRESULT hr;
    Callback::vector v = bg_callbacks[uuid];
    for (Callback::vector::const_iterator i = v.begin(); i != v.end(); i++) {
        Callback::pointer bg_callback = *i;
        if (bg_callback->type == L"*" && type == L"bridge") continue;
        if (bg_callback->type == L"*" || 
            bg_callback->type == type) {
            hr = bg_callback->Dispatch(content, callback);
        }
    }

    return S_OK;
}


/**
 * Method: bg_listen -> type -> (String -> (String))
 *
 * BG <= FG
 */
STDMETHODIMP CNativeMessaging::bg_listen(BSTR _uuid, 
                                         BSTR _type, 
                                         IDispatch *callback, IDispatch *error)
{
    wstring uuid(_uuid), type(_type);

    /*logger->debug(L"CNativeMessaging::bg_listen"
                  L" -> " + uuid +
                  L" -> " + type +
                  L" -> " + boost::lexical_cast<wstring>(callback) +
                  L" -> " + boost::lexical_cast<wstring>(error));*/

    this->bg_callbacks[uuid].push_back
        (Callback::pointer(new Callback(type, callback, error)));

    return S_OK;
}


/**
 * Method: bg_broadcast
 *
 * BG => FG(*)
 */
STDMETHODIMP CNativeMessaging::bg_broadcast(BSTR _uuid, 
                                            BSTR _type, BSTR _content, 
                                            IDispatch *callback, IDispatch *error)
{
    wstring uuid(_uuid), type(_type), content(_content);
    /*logger->debug(L"CNativeMessaging::bg_broadcast"
                  L" -> " + uuid +
                  L" -> " + type +
                  L" -> " + content +
                  L" -> " + boost::lexical_cast<wstring>(callback) +
                  L" -> " + boost::lexical_cast<wstring>(error));*/

    HRESULT hr;
    Callback::vector v = fg_callbacks[uuid];
    for (Callback::vector::const_iterator i = v.begin(); i != v.end(); i++) {
        Callback::pointer fg_callback = *i;
        if (fg_callback->type == L"*" || 
            fg_callback->type == type) {
            hr = fg_callback->Dispatch(content, callback);
        }
    }    

    return S_OK;
}


/**
 * Method: bg_toFocussed
 *
 * BG => FG(1)
 */
STDMETHODIMP CNativeMessaging::bg_toFocussed(BSTR _uuid, 
                                             BSTR _type, BSTR _content, 
                                             IDispatch *callback, IDispatch *error)
{
    wstring uuid(_uuid), type(_type), content(_content);
    /*logger->debug(L"CNativeMessaging::bg_toFocussed"
                  L" -> " + uuid +
                  L" -> " + type +
                  L" -> " + content +
                  L" -> " + boost::lexical_cast<wstring>(callback) +
                  L" -> " + boost::lexical_cast<wstring>(error));*/
    
    HRESULT hr;
    Callback::vector v = fg_callbacks[uuid];
    for (Callback::vector::const_iterator i = v.begin(); i != v.end(); i++) {
        Callback::pointer fg_callback = *i;
        if (fg_callback->tabId != m_activeTab.id) continue; 
        if (fg_callback->type == L"*" || 
            fg_callback->type == type) {
            hr = fg_callback->Dispatch(content, callback);
        }
    }    

    return S_OK;
}
