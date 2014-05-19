#include "stdafx.h"
#include "UpdateManager.h"
#include "vendor.h"

#include "HTTP.h"

/**
 * Check for updates
 */
void UpdateManager::Check(const wstring& version)
{
    HRESULT hr;

    logger->debug(L"UpdateManager::Check(" + version + L")");

    CComObject<BindStatusCallback> *bindStatusCallback;
    hr = CComObject<BindStatusCallback>::CreateInstance(&bindStatusCallback);    
    if (FAILED(hr)) {
        logger->error(L"UpdateManage::Check could not create BindStatusCallback instance");
        return;
    }

    wstring url = m_url + L"?version=" + version + L"&user_name=23"; 
    AsyncCallback f = std::bind1st(std::mem_fun(&UpdateManager::OnUpdateInfo), this);
    hr = bindStatusCallback->StartAsyncDownload
        (new HTTP(url, f),
         (BindStatusCallback::ATL_PDATAAVAILABLE1)&HTTP::OnData,
         CComBSTR(url.c_str()), 
         NULL, false);
    if (FAILED(hr)) {
        logger->error(L"UpdateManager::Check failed to start async download");
    }
}


/**
 * Event: OnUpdateInfo
 */
void UpdateManager::OnUpdateInfo(wstring data)
{
    logger->debug(L"UpdateManager::OnUpdateInfo -> " + data);

    std::string json;
    json.assign(data.begin(), data.end());
    if (json.empty()) {
        logger->debug(L"UpdateManager::OnUpdateInfo received empty reply");
        return;
    }

    bool updateAvailable = false;
    std::string url;
    try {
        json_spirit::Value v;
        json_spirit::read(json, v);
        json_spirit::Object obj = v.get_obj();
		updateAvailable = json_util::find_bool(obj, "success");
        if (updateAvailable) {
			url = json_util::find_str(obj, "url");
        } 
    } catch (...) {
        logger->debug(L"UpdateManager::OnUpdateInfo caught exception");
        return;
    }
    
    if (updateAvailable && !url.empty()) {
        UINT result;
        result = ::MessageBox(NULL, 
                              L"Search Sidebar for Internet Explorer: "
                              L"update available, do you wish to install it now?", 
                              VENDOR_COMPANY_NAME, MB_YESNO | MB_ICONQUESTION);
        if (result == IDYES) {
            ::ShellExecuteA(NULL, "open", url.c_str(), NULL, NULL, SW_NORMAL);
        }
    }
}

