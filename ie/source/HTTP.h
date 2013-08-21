#ifndef __HTTP_H__
#define __HTTP_H__

#include <util.h>
#include <json_spirit/json_spirit.h>

using namespace ATL;

// forward declarations
class HTTP;

/**
 * Implementation: IBindStatusCallback
 */
template <class T, 
          int flags = BINDF_ASYNCHRONOUS | 
                      BINDF_ASYNCSTORAGE |
                      BINDF_GETNEWESTVERSION>
class ATL_NO_VTABLE HTTPBindStatusCallback 
    : public CBindStatusCallback <T, flags>,
      public IHttpNegotiate 
{
 public:
    BEGIN_COM_MAP(HTTPBindStatusCallback<T>)
        COM_INTERFACE_ENTRY(IBindStatusCallback)
        COM_INTERFACE_ENTRY(IHttpNegotiate)             
    END_COM_MAP()    

     // aliases
     typedef void (T::*ATL_PDATAAVAILABLE1)(CBindStatusCallback<T, flags>* pbsc, 
                                            BYTE* pBytes, DWORD dwSize);
     typedef void (T::*ATL_PDATAAVAILABLE2)(HTTPBindStatusCallback<T, flags>* pbsc, 
                                            BYTE* pBytes, DWORD dwSize);

     HTTPBindStatusCallback() 
         : verb(BINDVERB_GET),
           method(NULL),
           data(NULL),
           datasize(0) {
         //logger->debug(L"HTTPBindStatusCallback::HTTPBindStatusCallback");
     }; 
 
     ~HTTPBindStatusCallback() {
         //logger->debug(L"HTTPBindStatusCallback::~HTTPBindStatusCallback");
         if (this->data) {
             ::GlobalFree(this->data);
             this->data = NULL;
         }
     }

     HTTP* http;

     /** 
      * Method: Async
      *
      * Perform an async HTTP request
      */
     HRESULT Async(_In_ T* pT, 
                   _In_ ATL_PDATAAVAILABLE callback, 
                   _In_z_ BSTR method, 
                   _In_z_ BSTR url, 
                   _In_z_ BSTR data, 
                   _In_z_ BSTR contentType, 
                   _Inout_opt_ IUnknown* container = NULL, 
                   _In_ BOOL relative = FALSE) {
         /*logger->debug(L"HTTPBindStatusCallback::StartAsyncDownload"
                       L" -> " + wstring(method) +
                       L" -> " + wstring(url));*/

         if (pT) {
             this->http = (HTTP*)pT;
         }

         this->method = method;
         this->contentType = contentType;
         
         // convert POST data to ASCII
         if (this->data) {
             ::GlobalFree(this->data);
             this->data = NULL;
         }
         wstring s(data);
         std::string asciidata(s.begin(), s.end());
         this->datasize = static_cast<DWORD>(asciidata.length());
         this->data = static_cast<byte*>(::GlobalAlloc(GPTR, this->datasize));
         if (!this->data) {
             DWORD error_code = ::GetLastError();
             HRESULT hr = error_code != NO_ERROR ? HRESULT_FROM_WIN32(error_code) : E_FAIL;
             logger->debug(L"HTTPBindStatusCallback::StartAsyncDownload "
                           L"::GlobalAlloc failed " 
                           L" -> " + logger->parse(hr));
             return hr;
         }
         memcpy(this->data, asciidata.c_str(), this->datasize);

         wstring verb(method);
         if (verb == L"GET") {
             this->verb = BINDVERB_GET;
         } else if (verb == L"PUT") {
             this->verb = BINDVERB_PUT;
         } else if (verb == L"POST") {
             this->verb = BINDVERB_POST;
         } else if (verb == L"DELETE") {
             this->verb = BINDVERB_CUSTOM; // TODO 
         } else {
             this->verb = BINDVERB_CUSTOM;
         }

         return CBindStatusCallback<T, flags>::StartAsyncDownload
             (pT, callback, url, container, relative);
     }


     /**
      * IBindStatusCallback
      */
     STDMETHOD(OnStartBinding)(DWORD reserved, IBinding *binding) {
         //logger->debug(L"HTTPBindStatusCallback::OnStartBinding");
         // broken microsoft api's are broken, see:
         // http://groups.google.com/groups?as_umsgid=08ee01c177c7$e8ca3dd0$3aef2ecf@TKMSFTNGXA09
         HRESULT hr;
         hr = ::CoInitializeEx(NULL, COINIT_APARTMENTTHREADED); 
         if (FAILED(hr)) {
             logger->error(L"HTTP::OnBindStatusCallback CoinitializeEx failed"
                           L" -> " + logger->parse(hr));
             return hr;
         }
         hr = CBindStatusCallback<T, flags>::OnStartBinding(reserved, binding);
         if (FAILED(hr)) {
             logger->error(L"HTTP::OnBindStatusCallback CBindStatusCallback failed"
                           L" -> " + logger->parse(hr));
         }
         ::CoUninitialize();
         return hr;
     }

     STDMETHOD(BeginningTransaction)(LPCWSTR url, LPCWSTR headers, DWORD reserved, 
                                     LPWSTR *additionalHeaders)  {
         logger->debug(L"HTTPBindStatusCallback::BeginningTransaction"
                       L" -> " + boost::lexical_cast<wstring>(url) +
                       L" -> " + boost::lexical_cast<wstring>(headers));

         if (!additionalHeaders) {
             logger->error(L"HTTPBindStatusCallback::BeginningTransaction "
                           L"could not set additional headers");
             return E_POINTER;
         }

         // set Content-Type header
         wstring headerString = L"";
         if (this->contentType) {
             headerString += L"Content-Type: " + wstring(this->contentType) + L"\n";
         }

         // set additional headers
         if (this->http) {
             wstringmap::iterator i = this->http->headers.begin();
             for (; i != this->http->headers.end(); i++) {
                 logger->debug(L"    Setting header: " + i->first + L" -> " + i->second);
                 headerString += i->first + L": " + i->second + L"\n";
             }
         }
         
         *additionalHeaders = NULL;
         LPWSTR p = (LPWSTR)::CoTaskMemAlloc((headerString.length()+1) * sizeof(WCHAR));
         if (!p) {
             logger->error(L"HTTPBindStatusCallback::BeginningTransaction "
                           L"out of memory");
             return E_OUTOFMEMORY;
         }
         wcscpy_s(p, (headerString.length()+1 * sizeof(WCHAR)), headerString.c_str());
         *additionalHeaders = p;

         return S_OK;
     }

     STDMETHOD(OnResponse)(DWORD responseCode, LPCWSTR responseHeaders, 
                           LPCWSTR requestHeaders, LPWSTR *extRequestHeaders) {
         /*logger->debug(L"HTTPBindStatusCallback::OnResponse"
                       L" -> " + boost::lexical_cast<wstring>(responseCode) +
                       L" -> " + boost::lexical_cast<wstring>(responseHeaders));*/

         if (responseCode < 400) { 
             return S_OK;
         }

         if (this->http && this->http->error) {
             wstring json = L"{\"statusCode\": \"" +
                 boost::lexical_cast<wstring>(responseCode) +
                 L"\"}";
             logger->debug(L"HTTPBindStatusCallback::OnResponse"
                           L" -> " + boost::lexical_cast<wstring>(this->http->error));
             HRESULT hr = CComDispatchDriver(this->http->error)
                 .Invoke1((DISPID)0, &CComVariant(json.c_str()));
             if (FAILED(hr)) {
                 logger->error(L"HTTPBindStatusCallback::OnResponse Failed to invoke error callback"
                               L" -> " + logger->parse(hr));
             }
         }

         return E_FAIL;
     }

     STDMETHOD(GetBindInfo)(DWORD *bindf, BINDINFO *bindinfo) {
         //logger->debug(L"HTTPBindStatusCallback::GetBindInfo");

         if (bindinfo == NULL || bindinfo->cbSize == 0 || bindf == NULL) {
             return E_INVALIDARG;
         }
         
         ULONG cbSize = bindinfo->cbSize;         // remember incoming cbSize       
         memset(bindinfo, 0, sizeof(*bindinfo));  // zero out structure
         bindinfo->cbSize = cbSize;               // restore cbSize
         bindinfo->dwBindVerb = this->verb;       // set verb
         *bindf = flags;                          // set flags

         switch (this->verb) {
         case BINDVERB_GET:
             break;
         case BINDVERB_POST:
         case BINDVERB_PUT:
             bindinfo->stgmedData.tymed = TYMED_HGLOBAL;
             bindinfo->stgmedData.hGlobal = this->data; 
             bindinfo->stgmedData.pUnkForRelease = 
                 static_cast<IBindStatusCallback*>(this);
             AddRef();
             bindinfo->cbstgmedData = this->datasize; 
             break;
         default:
             logger->debug(L"HTTPBindStatusCallback::GetBindInfo "
                           L"does not support HTTP method: " + 
                           boost::lexical_cast<wstring>(this->method));
             return E_FAIL;
         };

         return S_OK;
     }


 private:
    BSTR method;
    byte *data;
    DWORD datasize; 
    BSTR contentType;
    BINDVERB verb;
};


class HTTP;
typedef HTTPBindStatusCallback<HTTP, BINDF_ASYNCHRONOUS | 
                                     BINDF_ASYNCSTORAGE | 
                                     BINDF_GETNEWESTVERSION> BindStatusCallback;


/**
 * HTTP
 */
class HTTP 
    : public CComObjectRootEx<CComSingleThreadModel> { 
 public:

    HTTP(const wstring& url, const AsyncCallback& callback_std) 
        : method(L"GET"),
          url(url),
          callback_std(callback_std),
          callback(NULL) 
    {
        this->bytes = shared_ptr<bytevector>(new bytevector);
        this->size = 0;
    }

    HTTP(const wstring& url, IDispatch *callback) 
        : method(L"GET"),
          url(url),
          callback(callback) 
    {
        if (this->callback) this->callback->AddRef();
        this->bytes = shared_ptr<bytevector>(new bytevector);
        this->size = 0;
    }

    HTTP(const wstring& method, const wstring& url, const wstring& data, const wstring& contentType, const wstring& json_headers,
         IDispatch *callback, IDispatch *error) 
        : method(method),
          url(url),
          data(data),
          contentType(contentType),
          callback_std(NULL),
          callback(callback),
          error(error) 
    {
        if (this->callback) this->callback->AddRef();
        if (this->error)    this->error->AddRef();
        this->bytes = shared_ptr<bytevector>(new bytevector);
        this->size = 0;

        // parse headers
        if (json_headers != L"") {
            logger->debug(L"HTTP::HTTP Parsing headers -> " + json_headers);
            json_spirit::wValue v;
            json_spirit::read(json_headers, v);
            json_spirit::wObject o = v.get_obj();
            json_spirit::wObject::iterator i;
            for (i = o.begin(); i != o.end(); i++) {
                std::wstring key   = i->name_;
                std::wstring value = i->value_.get_str();
                this->headers[key] = value;
            }
        }
    }
    
    ~HTTP() {
        logger->debug(L"HTTP::~HTTP");
        if (this->callback) this->callback->Release();
        if (this->error)    this->error->Release();
    }

    void OnData(BindStatusCallback *caller, BYTE *bytes, DWORD size);

 public:// private:
    wstring method;
    wstring url;
    wstring data;
    wstring contentType;
    wstringmap headers;

    IDispatch *callback;
    IDispatch *error; // TODO implement all failure cases
    AsyncCallback callback_std;

    shared_ptr<bytevector> bytes; 
    DWORD size;
};


#endif // __HTTP_H__
