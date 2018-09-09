#ifndef __UTIL_H__
#define __UTIL_H__

/**
 * Utilities with global scope
 */


/** macros */
//#define ASSERT _ASSERTE
#ifndef MIN
#define MIN(a, b)  (((a) < (b)) ? (a) : (b))
#endif /* MIN */
#ifndef MAX
#define MAX(a, b)  (((a) > (b)) ? (a) : (b))
#endif /* MAX */


/** Logging */
#include "Logger.h"
extern Logger::pointer logger;


/** Useful namespaces */
namespace bfs = boost::filesystem;
using boost::shared_ptr;


/** Useful definitions */
typedef std::wstring wstring;
typedef shared_ptr<wstring> wstringpointer;
typedef std::vector<wstring> wstringvector;
typedef std::vector<wstringpointer> wstringpvector;
typedef std::vector<unsigned char> bytevector;
typedef std::set<wstring> wstringset;
typedef std::set<unsigned int> uintset;
typedef std::map<wstring, wstring> wstringmap;
typedef std::map<wstring, wstringpointer> wstringpmap;
typedef std::vector<wstringmap> wstringmapvector;
typedef std::vector<bfs::wpath> wpathvector;
typedef std::map<wstring, bfs::wpath> wpathmap;


/** node.js style Javascript callback - TODO use boost::bind */
typedef boost::function<void (wstring data)> AsyncCallback;
//typedef boost::function<void (const wstring& error, 
//                              const wstring& data)> Callback;


/** stl helper for static array initialization */
template<typename T, size_t N> 
T * staticarray_end(T (&ra)[N]) {
    return ra + N;
};


/** string utilities */
static wstring wstring_replace(const wstring& s, wchar_t from, wchar_t to) {
    wstring ret = L"";
    for (wstring::const_iterator i = s.begin(); i != s.end(); i++) {
        if (*i == from) {
            ret.append(1, to);
        } else {
            ret.append(1, *i);
        }
    }
    return ret;
};

static wstring wstring_limit(const wstring& s, size_t maxlen = 160) {
    size_t len = s.length();
    if (len < maxlen) {
        return s;
    }
    return s.substr(0, maxlen / 2) +  L" ... <schnip /> ... " + 
        s.substr(len - maxlen / 2);
}

static bool wstring_match_wild(const wstring& wildcard, const wstring& s) {
    wstring rex = wildcard;
    boost::replace_all(rex, L"\\", L"\\\\");
    boost::replace_all(rex, L"^", L"\\^");
    boost::replace_all(rex, L".", L"\\.");
    boost::replace_all(rex, L"$", L"\\$");
    boost::replace_all(rex, L"|", L"\\|");
    boost::replace_all(rex, L"(", L"\\(");
    boost::replace_all(rex, L")", L"\\)");
    boost::replace_all(rex, L"[", L"\\[");
    boost::replace_all(rex, L"]", L"\\]");
    boost::replace_all(rex, L"/", L"\\/");
    boost::replace_all(rex, L"+", L"\\+");
    boost::replace_all(rex, L"?", L"\\?");
    boost::replace_all(rex, L"*", L".*");
    rex = L"^" + rex + L"$";
    return boost::regex_match(s, boost::wregex(rex, boost::wregex::icase));
}


#include <windows.h>

/** thunkable types */
typedef UINT32  UINTX;
typedef DWORD32 DWORDX;
typedef INT64   INT_PTRX;
typedef UINT32  HWNDX;

static HRESULT GET_MSIE_VERSION(int *major, int *minor)
{
     LONG result;
     HKEY hkey;
     wchar_t buf[100];
     DWORD bufsize = 100;
     DWORD type;

     result = ::RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                              L"SOFTWARE\\Microsoft\\Internet Explorer",
                              0, KEY_QUERY_VALUE, &hkey);
     if (result != ERROR_SUCCESS) {
         return E_FAIL;
     }
     result = ::RegQueryValueEx(hkey, L"Version", NULL,
                                &type, (LPBYTE)buf, &bufsize);
     
     if(result != ERROR_SUCCESS) {
         result = ::RegCloseKey(hkey);
         return E_FAIL;
     }
     result = ::RegCloseKey(hkey);
     
     wstring version = buf;
     size_t majorpos = version.find(L".");

     *major = _wtoi(version.substr(0, majorpos).c_str());
     *minor = 0; // TODO

     return S_OK;
}

#define BreakOnFailed(res) if (FAILED((res))) { break; }
#define BreakOnFailedWithErrorLog(res, message) if (FAILED((res))) { logger->debug((message)); break; }

#define BreakOnNull(ptr, res) if (!(ptr)) { (res) = E_POINTER; break; }
#define BreakOnNullWithErrorLog(ptr, message) if(!(ptr)) { logger->error((message)); break; }

#endif /* __UTIL_H__ */
