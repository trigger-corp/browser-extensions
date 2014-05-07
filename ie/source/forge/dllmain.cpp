#include "stdafx.h"
#include "resource.h"
#include <generated/Forge_i.h>
#include "dllmain.h"
#include "xdlldata.h"

CForgeModule _AtlModule;

// DLL Entry Point
extern "C" BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved)
{
    BOOL result = FALSE;

    /*#ifdef DEBUG
    logger->output(L"z:\\forge_com.log");
#else
    logger->output(L"forge_com.log");
#endif*/ /* DEBUG */

    ::DisableThreadLibraryCalls(instance);

#ifdef _MERGE_PROXYSTUB
    if (!PrxDllMain(hInstance, dwReason, lpReserved))
        return FALSE;
#endif

    // save module instance handle
    _AtlModule.moduleHandle = instance;
    
    // save module path
    wchar_t buf[MAX_PATH];
    ::GetModuleFileName(instance, buf, MAX_PATH);
    _AtlModule.moduleExec = bfs::wpath(buf);
    _AtlModule.modulePath = bfs::wpath(buf).parent_path();
    
    // save calling process
    ::GetModuleFileName(NULL, buf, MAX_PATH);
    wstring caller(buf);
    transform(caller.begin(), caller.end(), caller.begin(), tolower);
    _AtlModule.callerPath = bfs::wpath(caller);
    
    // detach
    if (reason == DLL_PROCESS_DETACH) {
        logger->debug(L"Forge::DllMain(DLL_PROCESS_DETACH)");
        return _AtlModule.DllMain(reason, reserved); 
    } else if (reason != DLL_PROCESS_ATTACH) {
        logger->debug(L"Forge::DllMain(UNKNOWN)"); 
        return _AtlModule.DllMain(reason, reserved); 
    }

    // initialize logger 
    logger->initialize(_AtlModule.modulePath);
    logger->debug(L"Forge::DllMain"
                  L" -> " + boost::lexical_cast<wstring>(instance) +
                  L" -> " + boost::lexical_cast<wstring>(reason) +
                  L" -> " + boost::lexical_cast<wstring>(reserved) +
                  L" -> " + _AtlModule.moduleExec.wstring());
    
    // attach
    try {
        logger->debug(L"Forge::DllMain(DLL_PROCESS_ATTACH) caller: " +
                      _AtlModule.callerPath.wstring());

        // initialize module manifest
        _AtlModule.moduleCLSID = L"{3187A87B-2274-4924-A644-2E2BB36C6271}";
        _AtlModule.moduleManifest = Manifest::pointer(new Manifest());
        _AtlModule.moduleManifest->name  = L"Forge API COM Server";
        _AtlModule.moduleManifest->uuid  = _AtlModule.moduleCLSID;
        _AtlModule.moduleManifest->logging.level = L"DEBUG";
        _AtlModule.moduleManifest->logging.console = true;
        
        // dump some handy info
        logger->debug(L"Module executable: " + _AtlModule.moduleExec.wstring());
        logger->debug(L"    name:  " + _AtlModule.moduleManifest->name);
        logger->debug(L"    uuid:  " + _AtlModule.moduleManifest->uuid);
        logger->debug(L"   clsid:  " + _AtlModule.moduleCLSID);

        result = _AtlModule.DllMain(reason, reserved); 
        logger->debug(L"Forge::DllMain _AtlModule.DllMain -> " + 
                      boost::lexical_cast<wstring>(result));
        
    } catch (...) {
        result = FALSE;
        logger->debug(L"Forge::DllMain _AtlModule.DllMain -> unknown fatal exception");
    }
    
    logger->debug(L"----------------------------------------------------------\n");

    return result;
}
