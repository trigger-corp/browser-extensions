#include "stdafx.h"
#include "Logger.h"

#include <ScriptExtensions.h>

// TODO  http://msdn.microsoft.com/en-us/library/ms679351(VS.85).aspx

/**
 * Logger:::Logger
 */
Logger::Logger(Level level, const std::wstring& filename)
    : m_level(level), 
      m_filename(filename)
{
    if (m_filename == L"") {
        this->enabled = false;
    } else {
        this->enabled = true;
    }
}


/**
 * Logger::initialize
 */
void Logger::initialize(const boost::filesystem::wpath& path)
{
    this->debug(L"Logger::initalize"
                L" -> " + path.wstring());

    // read addon manifest.json
    ScriptExtensions::pointer scriptExtensions = ScriptExtensions::pointer
        (new ScriptExtensions(path, false));
    Manifest::pointer manifest = scriptExtensions->ParseManifest();
    if (!manifest) {
        this->debug(L"Logger::Logger could not read manifest");
        this->enabled = false;
    } else if (manifest->logging.filename != L"") {
        // Replace environment variables in path so %LOCALAPPDATA%Low can be
        // used which is the only place where the low priviledged BHO process
        // can create files.
        wchar_t expandedPath[MAX_PATH];
        DWORD len = ::ExpandEnvironmentStrings(manifest->logging.filename.c_str(),
                                               expandedPath, MAX_PATH);
        if (len > 0 && len <= MAX_PATH) {
            m_filename = expandedPath;
        }
        else {
            this->error(L"Logger::Logger failed to expand environment variables in path");
            m_filename = manifest->logging.filename;
        }

        this->debug(L"Logger::Logger using endpoint: " + m_filename);
        this->enabled = true;
    } else {
        this->enabled = false;
    }
}


/**
 * Logger::write
 */
void Logger::write(const std::wstring& message, Logger::Level level)
{
    if (!this->enabled) {
        return;
    }

    if (level <= m_level) {

#ifdef DEBUGGER
        if (level == Logger::ERR) {
            ::OutputDebugString(L"ERROR ");  
        }
        ::OutputDebugString(message.c_str());  
        ::OutputDebugString(L"\n");
#endif /* DEBUGGER */

        if (m_filename != L"") {
            std::wofstream fs;
            fs.open(m_filename, std::ios::out | std::ios::app);
            if (level == Logger::ERR) {
                fs << L"[ERROR] ";
            }
            fs << message << std::endl << std::flush;
            fs.close();
        }
    }
}


/**
 * Logger::parse(HRESULT)
 */
std::wstring Logger::parse(HRESULT hr) 
{
    HRESULT result;

    wchar_t* buf = NULL;
    result = ::FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | 
                              FORMAT_MESSAGE_FROM_SYSTEM, NULL, hr,
                              MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                              reinterpret_cast<wchar_t*>(&buf), 0, NULL);
    std::wstring hrstr(buf != NULL ? buf : L"unknown error");
    ::LocalFree(reinterpret_cast<HLOCAL>(buf));
    
    std::wstringstream hrhex;
    hrhex << L"0x" << std::hex << hr;
    
    return hrhex.str() + L" -> " + hrstr;
}


/**
 * Logger::parse(IDispatch*)
 *
 * Adapted from: http://spec.winprog.org/typeinf2/
 */
std::wstring Logger::parse(IDispatch *object)
{
    std::wstringstream output;

    HRESULT hr;

    // query object typeinfo
    CComPtr<ITypeInfo> typeinfo;
    hr = object->GetTypeInfo(0, 0, &typeinfo);
    if (FAILED(hr)) { 
        return L"no typeinfo for object -> " + this->parse(hr);
    }

    TYPEATTR *typeAttr;
    typeinfo->GetTypeAttr(&typeAttr);

    CComBSTR interfaceName;
    hr = typeinfo->GetDocumentation(-1, &interfaceName, 0, 0, 0);
    if (FAILED(hr)) {
        output << L"    interface: unknown default interface";
    } else {
        output << L"    interface: " + std::wstring(interfaceName);
    }

    // properties
    output << L"    property count: " 
           << boost::lexical_cast<std::wstring>(typeAttr->cVars);
    for (UINT curValue(0); curValue < typeAttr->cVars; ++curValue) {
        VARDESC *varDesc;
        hr = typeinfo->GetVarDesc(curValue, &varDesc);
        if (FAILED(hr)) { 
            output << L"    could not read property descriptor";
            continue; 
        }                              

        CComBSTR name;
        CComVariant value;
        hr = typeinfo->GetDocumentation(varDesc->memid, &name, 0, 0, 0);
        if (FAILED(hr)) {
            name = L"unknown";
        } else {
            hr = CComPtr<IDispatch>(object).GetPropertyByName(name, &value);
            if (FAILED(hr)) {
                output << L"    could not read property value for: '" 
                       << std::wstring(name) << L"'";
                continue;
            }
        }

        output << L"    " 
               << stringify(varDesc, typeinfo) << L"\t: " 
               << stringify(value.vt) << L" " 
               << (value.vt == VT_BSTR 
                   ? L"\"" + limit(value.bstrVal) + L"\"" 
                   : L"[object]");

        if (value.vt == VT_DISPATCH) {
            output << this->parse(value.pdispVal);
        }
        typeinfo->ReleaseVarDesc(varDesc);             
    }

    // methods
    output << L"    method count: " 
           << boost::lexical_cast<std::wstring>(typeAttr->cFuncs)
           << L"\t";

    for (int curFunc(0); curFunc < typeAttr->cFuncs; ++curFunc) {
        FUNCDESC *funcDesc;
        hr = typeinfo->GetFuncDesc(curFunc, &funcDesc);
        CComBSTR methodName;
        hr |= typeinfo->GetDocumentation(funcDesc->memid, &methodName, 0, 0, 0);
        if (FAILED(hr)) { 
            output << L"    name error";
            typeinfo->ReleaseFuncDesc(funcDesc); 
            continue; 
        }

        output << stringify(&funcDesc->elemdescFunc.tdesc, typeinfo) 
               << L" " << std::wstring(methodName) << L"(";
        for (int curParam(0); curParam < funcDesc->cParams; ++curParam) {
            output << stringify(&funcDesc->lprgelemdescParam[curParam].tdesc,
                                typeinfo);
            if (curParam < funcDesc->cParams - 1) {
                output << L", ";
            }
        }
        output << L")";
        switch(funcDesc->invkind) {
        case INVOKE_PROPERTYGET:    output << L" propget";    break;
        case INVOKE_PROPERTYPUT:    output << L" propput";    break;
        case INVOKE_PROPERTYPUTREF: output << L" propputref"; break;
        }
        typeinfo->ReleaseFuncDesc(funcDesc);
    }

    // cleanup
    typeinfo->ReleaseTypeAttr(typeAttr);

    return output.str();
}


/**
 * Logger::stringify(VARDESC)
 */
std::wstring Logger::stringify(VARDESC *varDesc, ITypeInfo *pti) 
{
    HRESULT hr;
    std::wstringstream output;

    CComPtr<ITypeInfo> pTypeInfo(pti);
    if (varDesc->varkind == VAR_CONST) {
        output << L"const ";
    }
    output << stringify(&varDesc->elemdescVar.tdesc, pTypeInfo);

    CComBSTR bstrName;
    hr = pTypeInfo->GetDocumentation(varDesc->memid, &bstrName, 0, 0, 0);
    if (FAILED(hr)) {
        return L"UnknownName";
    }
    output << ' '<< bstrName;
    if (varDesc->varkind != VAR_CONST) {
        return output.str();
    }
    output << L" = ";

    CComVariant variant;
    hr = ::VariantChangeType(&variant, varDesc->lpvarValue, 0, VT_BSTR);
    if (FAILED(hr)) {
        output << L"???";
    } else {
        output << variant.bstrVal;
    }

    return output.str();
}


/**
 * Logger::stringify(TYPEDESC)
 */
std::wstring Logger::stringify(TYPEDESC *typeDesc, ITypeInfo *pTypeInfo) 
{
    std::wstringstream output;

    if (typeDesc->vt == VT_PTR) {
        output << stringify(typeDesc->lptdesc, pTypeInfo) << '*';
        return output.str();

    } else if (typeDesc->vt == VT_SAFEARRAY) {
        output << L"SAFEARRAY("
            << stringify(typeDesc->lptdesc, pTypeInfo) << ')';
        return output.str();

    } else if (typeDesc->vt == VT_CARRAY) {
        output << stringify(&typeDesc->lpadesc->tdescElem, pTypeInfo);
        for (int dim(0); typeDesc->lpadesc->cDims; ++dim) {
            output << '['<< typeDesc->lpadesc->rgbounds[dim].lLbound << "..."
                << (typeDesc->lpadesc->rgbounds[dim].cElements + 
                    typeDesc->lpadesc->rgbounds[dim].lLbound - 1) << ']';
        }
        return output.str();

    } else if (typeDesc->vt == VT_USERDEFINED) {
        output << stringify(typeDesc->hreftype, pTypeInfo);
        return output.str();
    }
    
    return stringify(typeDesc->vt);
}


/**
 * Logger::stringify(HREFTYPE)
 */
std::wstring Logger::stringify(HREFTYPE refType, ITypeInfo *pti) 
{
    CComPtr<ITypeInfo> pTypeInfo(pti);
    CComPtr<ITypeInfo> pCustTypeInfo;
    HRESULT hr(pTypeInfo->GetRefTypeInfo(refType, &pCustTypeInfo));
    if (FAILED(hr)) {
        return L"UnknownCustomType";
    }

    CComBSTR bstrType;
    hr = pCustTypeInfo->GetDocumentation(-1, &bstrType, 0, 0, 0);
    if (FAILED(hr)) {
        return L"UnknownCustomType";
    }

    return std::wstring(bstrType);
}


/**
 * Logger::stringify(VT)
 */ 
std::wstring Logger::stringify(int vt) 
{
    switch(vt) { // VARIANT/VARIANTARG compatible types
    case VT_I2:       return L"short";
    case VT_I4:       return L"long";
    case VT_R4:       return L"float";
    case VT_R8:       return L"double";
    case VT_CY:       return L"CY";
    case VT_DATE:     return L"DATE";
    case VT_BSTR:     return L"BSTR";
    case VT_DISPATCH: return L"IDispatch*";
    case VT_ERROR:    return L"SCODE";
    case VT_BOOL:     return L"VARIANT_BOOL";
    case VT_VARIANT:  return L"VARIANT";
    case VT_UNKNOWN:  return L"IUnknown*";
    case VT_UI1:      return L"BYTE";
    case VT_DECIMAL:  return L"DECIMAL";
    case VT_I1:       return L"char";
    case VT_UI2:      return L"USHORT";
    case VT_UI4:      return L"ULONG";
    case VT_I8:       return L"__int64";
    case VT_UI8:      return L"unsigned __int64";
    case VT_INT:      return L"int";
    case VT_UINT:     return L"UINT";
    case VT_HRESULT:  return L"HRESULT";
    case VT_VOID:     return L"void";
    case VT_LPSTR:    return L"char*";
    case VT_LPWSTR:   return L"wchar_t*";
    }
    return L"unknown type";    
}
