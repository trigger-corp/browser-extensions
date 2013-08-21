#include "stdafx.h"
#include "NativeBackground.h"


/**
 * Construction
 */
CNativeBackground::CNativeBackground()
    : isVisible(FALSE)
{
    this->instanceCounter = 1; // 0 -> background
}


/** 
 * Destruction
 */
CNativeBackground::~CNativeBackground()
{
    logger->debug(L"CNativeBackground::~CNativeBackground");
}

HRESULT CNativeBackground::FinalConstruct()
{
    return S_OK;
}

void CNativeBackground::FinalRelease()
{
    logger->debug(L"CNativeBackground::FinalRelease");
}


/**
 * InterfaceSupportsErrorInfo
 */
STDMETHODIMP CNativeBackground::InterfaceSupportsErrorInfo(REFIID riid)
{
    static const IID* const arr[] = {
        &IID_INativeBackground
    };
    for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++) {
        if (InlineIsEqualGUID(*arr[i],riid)) {
            return S_OK;
        }
    }
    return S_FALSE;
}


/**
 * Method: NativeBackground::load
 *
 * @param uuid
 * @param instance
 * @param url
 */
STDMETHODIMP CNativeBackground::load(BSTR uuid, BSTR url, BOOL isVisible, unsigned int *instanceId)
{
    *instanceId = this->instanceCounter++;
    m_clients[uuid].insert(*instanceId);

    logger->debug(L"CNativeBackground::load"
                  L" -> " + wstring(uuid) +
                  L" -> " + wstring(url) +
                  L" -> " + boost::lexical_cast<wstring>(isVisible) +
                  L" -> " + boost::lexical_cast<wstring>(*instanceId));

    // check if we're attached
    BrowserWindow::pointer crouchingWindow = m_crouchingWindows[uuid];
    if (crouchingWindow) {
        logger->debug(L"CNativeBackground::load already attached");
        return S_OK;
    }

    // create, attach & load
    HWND hwnd;
    crouchingWindow = BrowserWindow::pointer(new BrowserWindow(uuid, url));
    hwnd = crouchingWindow->Create(NULL, 
                                   CRect(0, 0, 640, 780),
                                   uuid);
    if (!hwnd) {
        logger->debug(L"CNativeBackground::load failed to create window");
        return E_FAIL;
    }    
    
    /*HRESULT hr;
    hr = crouchingWindow->Navigate(url);
    if (FAILED(hr)) {
        logger->debug(L"NativeBackground::load background page failed to load"
                      L" -> " + wstring(uuid) +
                      L" -> " + wstring(url) +
                      L" -> " + logger->parse(hr));
        return hr;
    }*/
    
    // initial visibility
    crouchingWindow->ShowWindow(isVisible ? SW_SHOW : SW_HIDE);
    this->isVisible = isVisible;

    // save it
    m_crouchingWindows[uuid] = crouchingWindow;

    return S_OK;
}

/**
 * Method: NativeBackground::unload
 *
 * @param uuid
 * @param instance
 */
STDMETHODIMP CNativeBackground::unload(BSTR uuid, unsigned int instanceId)
{
    logger->debug(L"CNativeBackground::unload"
                  L" -> " + wstring(uuid) +
                  L" -> " + boost::lexical_cast<wstring>(instanceId));

    m_clients[uuid].erase(instanceId);
    if (m_clients[uuid].empty()) {
        logger->debug(L"CNativeBackground::unload "
                      L"shutting down console -> " + wstring(uuid));
        m_crouchingWindows.erase(uuid);
        m_clients.erase(uuid);
    }

    logger->debug(L"CNativeBackground::unload clients remaining: " +
                  boost::lexical_cast<wstring>(m_clients.size()));

    if (m_clients.empty()) {
        return this->shutdown();
    } 

    return S_OK;
}


/**
 * Helper: shutdown
 */
HRESULT CNativeBackground::shutdown()
{
    logger->debug(L"CNativeBackground::shutdown "
                  L" -> " + boost::lexical_cast<wstring>(m_dwRef));
    
   return S_OK;
}


/**
 * Method: NativeBackground::visible
 *
 * @param uuid
 * @param isVisible
 */
STDMETHODIMP CNativeBackground::visible(BSTR uuid, BOOL isVisible)
{
    logger->debug(L"CNativeBackground::visible"
                  L" -> " + wstring(uuid) +
                  L" -> " + boost::lexical_cast<wstring>(isVisible));

    return S_OK;
}


