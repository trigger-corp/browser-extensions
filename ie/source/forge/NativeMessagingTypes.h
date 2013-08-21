#pragma once

#include <util.h>


using namespace ATL;


/**
 * ComAPI
 */
class ComAPI {
 public:
    ComAPI() {
        logger->debug(L"ComAPI::ComAPI");
    }

    // properties

    // COM
};


/** 
 * Tab : { 
 *     id         : Integer, 
 *     index      : Integer, 
 *     windowId   : Integer, 
 *     url        : String,
 *     selected   : Boolean, 
 *     pinned     : Boolean, 
 *     title      : String (optional), 
 *     favIconUrl : String (optional), 
 *     status     : String (optional), 
 *     incognito  : Boolean
 * } 
 */
class Tab {
 public:
    // properties
    int id;
    int index;
    int windowId;
    bool incognito;
    bool selected;
    bool pinned;
    wstring url;
    wstring title;
    wstring favIconUrl;
    wstring status;

    // getters
    virtual int __stdcall get_id()        { return this->id;    }
    virtual int __stdcall get_index()     { return this->index; }
    virtual int __stdcall get_windowId()  { return this->windowId; }
    virtual int __stdcall get_incognito() { return this->incognito ? -1 : 0; }
    virtual int __stdcall get_selected()  { return this->selected  ? -1 : 0; }
    virtual int __stdcall get_pinned()    { return this->pinned    ? -1 : 0; }
    virtual CComVariant __stdcall get_url()        { return this->url.c_str(); }
    virtual CComVariant __stdcall get_title()      { return this->title.c_str(); }
    virtual CComVariant __stdcall get_favIconUrl() { return this->favIconUrl.c_str(); }
    virtual CComVariant __stdcall get_status()     { return this->status.c_str(); }

    // lifecycle
    Tab();
    Tab(int id, int index, int windowId, 
        bool incognito, bool selected, bool pinned, 
        const wstring& url, const wstring& title, 
        const wstring& favIconUrl, const wstring& status);
    ~Tab() {
        logger->debug(L"Tab::~Tab");
    }

    // COM
    static METHODDATA methods[];
    static INTERFACEDATA Interface;
};


class Callback {
public:
    // lifecycle
    Callback(const wstring& type, IDispatch *callback, IDispatch *error);
    Callback(UINT tabId, const wstring& type, IDispatch *callback, IDispatch *error);
    ~Callback();

    // aliases
    typedef shared_ptr<Callback> pointer;
    typedef std::vector<Callback::pointer> vector;
    typedef std::map<wstring, Callback::vector> map; // uuid -> [ Callback ]

    // interface
    HRESULT Dispatch(const wstring& content, IDispatch *reply);

    // utility
    wstring toString() {
        return 
            L"Callback tabId:" + boost::lexical_cast<wstring>(tabId) +
            L" type:" + type;
    };

    UINT tabId;
    wstring type;
    CComPtr<IDispatch> callback;
    CComPtr<IDispatch> error;
};
