#include "stdafx.h"
#include "resource.h"
#include <generated/Forge_i.h>
#include "dllmain.h"
#include "xdlldata.h"


/**
 * Used to determine whether the DLL can be unloaded by OLE.
 */
STDAPI DllCanUnloadNow(void)
{
    logger->debug(L"Forge::dllexports::DllCanUnloadNow");

    // clean up
    HRESULT hr;

#ifdef _MERGE_PROXYSTUB
    logger->debug(L"Forge::dllexports::PrxDllCanUnloadNow");
	hr = PrxDllCanUnloadNow();
	if (hr != S_OK) {
        logger->debug(L"Forge::dllexports::PrxDllCanUnloadNow 1");
		return hr;
    }
    logger->debug(L"Forge::dllexports::PrxDllCanUnloadNow 2");
#endif

    hr = _AtlModule.DllCanUnloadNow();
    logger->debug(L"Forge::dllexports::DllCanUnloadNow"
                  L" -> " + logger->parse(hr));    

    return hr;
}


/**
 * Returns a class factory to create an object of the requested type.
 */
STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    LPOLESTR tmp;
    ::StringFromCLSID(rclsid, &tmp);
    wstring requested_clsid = wstring(tmp);
    ::CoTaskMemFree(tmp);
    
    /*logger->debug(L"Forge::dllexports::DllGetClassObject"
                  L" -> " + requested_clsid +
                  L" -> " + _AtlModule.moduleCLSID +
                  L" -> " + boost::lexical_cast<wstring>(ppv));*/

#ifdef _MERGE_PROXYSTUB
	if (PrxDllGetClassObject(rclsid, riid, ppv) == S_OK) {
		return S_OK;
    }
#endif

    return _AtlModule.DllGetClassObject(rclsid, riid, ppv);
}


/**
 * DllRegisterServer - Adds entries to the system registry.
 */
STDAPI DllRegisterServer(void)
{
    logger->debug(L"Forge::dllexports::DllRegisterServer");
    
	// registers object, typelib and all interfaces in typelib
	HRESULT hr = _AtlModule.DllRegisterServer();

#ifdef _MERGE_PROXYSTUB
	if (FAILED(hr))
		return hr;
	hr = PrxDllRegisterServer();
#endif
    
    return hr;
}


/**
 * DllUnregisterServer - Removes entries from the system registry.
 */
STDAPI DllUnregisterServer(void)
{
    logger->debug(L"Forge::dllexports::DllUnregisterServer");
    
	HRESULT hr = _AtlModule.DllUnregisterServer();
#ifdef _MERGE_PROXYSTUB
	if (FAILED(hr)) {
		return hr;
    }
	hr = PrxDllRegisterServer();
	if (FAILED(hr)) {
		return hr;
    }
	hr = PrxDllUnregisterServer();
#endif

    return hr;
}


/**
 * DllInstall - Adds/Removes entries to the system registry per user per machine.
 */
STDAPI DllInstall(BOOL bInstall, LPCWSTR pszCmdLine)
{
    logger->debug(L"Forge::dllexports::DllInstall");
    
	HRESULT hr = E_FAIL;
	static const wchar_t szUserSwitch[] = L"user";
    
	if (pszCmdLine != NULL) {
		if (_wcsnicmp(pszCmdLine, szUserSwitch, _countof(szUserSwitch)) == 0) {
			ATL::AtlSetPerUserRegistration(true);
		}
	}
    
	if (bInstall) {	
		hr = DllRegisterServer();
		if (FAILED(hr))	{
			DllUnregisterServer();
		}
	} else {
		hr = DllUnregisterServer();
	}

    return hr;
}

