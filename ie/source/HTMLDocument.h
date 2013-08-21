#ifndef __HTMLDOCUMENT_H__
#define __HTMLDOCUMENT_H__

#include <util.h>

using namespace ATL;


/**
 * IHTMLDocument wrapper
 */
class HTMLDocument 
    : public IDispEventImpl<1, HTMLDocument, &DIID_HTMLDocumentEvents, &LIBID_MSHTML, 1, 1> { 
public:
    HTMLDocument(const CComQIPtr<IWebBrowser2, &IID_IWebBrowser2>& webBrowser2);
    ~HTMLDocument();

BEGIN_SINK_MAP(HTMLDocument)
END_SINK_MAP()

    // TODO - check document type == html for all Inject functions?
    HRESULT InjectDocument(const wstringpointer& content);
    HRESULT InjectScript(const wstringpointer& content);
    HRESULT InjectScriptTag(const wstring& type, const wstring& src);
    HRESULT InjectStyle(const wstringpointer& content);
    HRESULT InjectBody(const wstringpointer& content,
                       BSTR where = HTMLDocument::beforeEnd);
    HRESULT InjectElementById(const wstring& id, const wstringpointer& content,
                              BSTR where = HTMLDocument::beforeEnd);

    HRESULT ClickElementById(const wstring& id);

private:
    HRESULT OnConnect();
    HRESULT OnDisconnect();

    const CComQIPtr<IWebBrowser2, &IID_IWebBrowser2>& m_webBrowser2;

    CComQIPtr<IHTMLDocument2, &IID_IHTMLDocument2> m_htmlDocument2;
    CComQIPtr<IHTMLDocument3, &IID_IHTMLDocument3> m_htmlDocument3;

public:
    static const BSTR beforeBegin;
    static const BSTR afterBegin;
    static const BSTR beforeEnd;
    static const BSTR afterEnd;
    static const BSTR tagHead;
    static const BSTR tagScript;
    static const BSTR tagStyle;
    static const BSTR attrScriptType;
    static const BSTR attrStyleType;

public:
    typedef shared_ptr<HTMLDocument> pointer;
};

#endif /* __HTMLDOCUMENT_H__ */
