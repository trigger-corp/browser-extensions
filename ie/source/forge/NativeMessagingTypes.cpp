#include "stdafx.h"
#include "NativeMessagingTypes.h"


/**
 * Type: Tab
 */
METHODDATA Tab::methods[] = {
    { L"id",         NULL,  1, 0, CC_STDCALL, 0, DISPATCH_PROPERTYGET, VT_I4 },
    { L"index",      NULL,  2, 1, CC_STDCALL, 0, DISPATCH_PROPERTYGET, VT_I4 },
    { L"windowId",   NULL,  3, 2, CC_STDCALL, 0, DISPATCH_PROPERTYGET, VT_I4 },
    { L"incognito",  NULL,  4, 3, CC_STDCALL, 0, DISPATCH_PROPERTYGET, VT_BOOL },
    { L"selected",   NULL,  5, 4, CC_STDCALL, 0, DISPATCH_PROPERTYGET, VT_BOOL },
    { L"pinned",     NULL,  6, 5, CC_STDCALL, 0, DISPATCH_PROPERTYGET, VT_BOOL },
    { L"url",        NULL,  7, 6, CC_STDCALL, 0, DISPATCH_PROPERTYGET, VT_VARIANT },
    { L"title",      NULL,  8, 7, CC_STDCALL, 0, DISPATCH_PROPERTYGET, VT_VARIANT },
    { L"favIconUrl", NULL,  9, 8, CC_STDCALL, 0, DISPATCH_PROPERTYGET, VT_VARIANT },
    { L"status",     NULL, 10, 9, CC_STDCALL, 0, DISPATCH_PROPERTYGET, VT_VARIANT },
};
INTERFACEDATA Tab::Interface = {
    Tab::methods,
    sizeof(Tab::methods) / sizeof(METHODDATA)
};

Tab::Tab() 
    : id(0), index(0), windowId(0), incognito(false),
    selected(false), pinned(false), url(L""), title(L""),
    favIconUrl(L""), status(L"")
{
}

Tab::Tab(int id, int index, int windowId, 
         bool _incognito, bool selected, bool pinned, 
         const wstring& url, const wstring& title, 
         const wstring& favIconUrl, const wstring& status) :
    id(id), index(index), windowId(windowId), incognito(incognito),
    selected(selected), pinned(pinned), url(url), title(title),
    favIconUrl(favIconUrl), status(status)
{
}



/**
 * Type: Callback
 */
Callback::Callback(const wstring& type, IDispatch *callback, IDispatch *error) 
    : tabId(0), type(type), callback(callback), error(error) 
{
}


Callback::Callback(UINT tabId, const wstring& type, IDispatch *callback, IDispatch *error) 
    : tabId(tabId), type(type), callback(callback), error(error) 
{
}


Callback::~Callback() 
{
    logger->debug(L"Callback::~Callback");
}


HRESULT Callback::Dispatch(const wstring& content, IDispatch *reply) 
{
    /*logger->debug(L"Callback::Dispatch"
                  L" -> " + this->toString() +
                  L" -> " + content);*/
    HRESULT hr;
    CComQIPtr<IDispatchEx> callbackex(this->callback);
    hr = CComDispatchDriver(callbackex).Invoke2((DISPID)0,
                                                &CComVariant(content.c_str()),
                                                &CComVariant(reply));
    if (SUCCEEDED(hr)) {
        return S_OK;
    }
    
    wstring e = L"Callback::Dispatch failed"
        L" -> " + logger->parse(hr) +
        L" -> " + content;
    logger->error(e);
    CComQIPtr<IDispatchEx> errorex(this->error);
    hr = CComDispatchDriver(errorex).Invoke1((DISPID)0,
                                             &CComVariant(e.c_str()));
    if (FAILED(hr)) {
        logger->error(L"Callback::Dispatch failed to dispatch error"
                      L" -> " + logger->parse(hr) +
                      L" -> " + e);
    }
    return hr;
}

