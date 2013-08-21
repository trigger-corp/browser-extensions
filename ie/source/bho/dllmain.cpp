#include "stdafx.h"
#include "resource.h"
#include <generated/BHO_h.h>
#include "dllmain.h"
#include <boost/md5.hpp>

CForgeBHOModule _AtlModule;


/**
 * DLL Entry Point
 */
extern "C" BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved)
{
    BOOL result = FALSE;

    ::DisableThreadLibraryCalls(instance);
    _AtlModule.moduleHandle = instance;
    
    // save addon path
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
        logger->debug(L"BHO::DllMain(DLL_PROCESS_DETACH)");
        return _AtlModule.DllMain(reason, reserved); 
    } else if (reason != DLL_PROCESS_ATTACH) {
        logger->debug(L"BHO::DllMain(UNKNOWN)"); 
        return _AtlModule.DllMain(reason, reserved); 
    }
    
    // attach
    logger->debug(L"BHO::DllMain(DLL_PROCESS_ATTACH) caller: " +
                  _AtlModule.callerPath.wstring());
    if (caller.find(L"explorer.exe") != wstring::npos ) {
        logger->debug(L"BHO::DllMain(DLL_PROCESS_ATTACH) cannot attach to: " + caller);
        return FALSE;
    } else if (caller.find(L"dllhost.exe") != wstring::npos) {
        logger->debug(L"BHO::DllMain(DLL_PROCESS_ATTACH) dllhost.exe");
    } else if (caller.find(L"iexplore.exe") == wstring::npos) {
        logger->debug(L"BHO::DllMain(DLL_PROCESS_ATTACH) registering");
    }

    // initialize logger 
    logger->initialize(_AtlModule.modulePath);
    logger->debug(L"BHO::DllMain"
                  L" -> " + boost::lexical_cast<wstring>(instance) +
                  L" -> " + boost::lexical_cast<wstring>(reason) +
                  L" -> " + boost::lexical_cast<wstring>(reserved) +
                  L" -> " + _AtlModule.moduleExec.wstring());
    
    // read addon manifest.json
    ScriptExtensions::pointer scriptExtensions = ScriptExtensions::pointer
        (new ScriptExtensions(_AtlModule.modulePath, false));
    logger->debug(L"BHO::DllMain reading manifest file: " + 
                  scriptExtensions->pathManifest.wstring() + 
                  L" at path: " +
                  _AtlModule.modulePath.wstring());
    _AtlModule.moduleManifest =
        scriptExtensions->ParseManifest();
    if (!_AtlModule.moduleManifest) {
        logger->debug(L"dllexports::AllRegisterServer failed to read manifest file: " +
                      scriptExtensions->pathManifest.wstring() + 
                      L" at: " + _AtlModule.modulePath.wstring());
        ::MessageBox(NULL,
                     wstring(L"dllexports::AllRegisterServer "
                             L"Failed to register add-on. Please check that the "
                             L"manifest.json file is present at " +
                             _AtlModule.modulePath.wstring() +
                             L" and properly configured.").c_str(),
                     L"trigger.io",
                     MB_TASKMODAL | MB_ICONEXCLAMATION);
        return EXIT_FAILURE;
    }

    // hash uuid (treat as ASCII to reduce post-temporal migraine)
    std::string text(_AtlModule.moduleManifest->uuid.begin(), 
                     _AtlModule.moduleManifest->uuid.end());
    boost::md5 hash(text.c_str());

    // convert to GUID
    const GUID *guid = (GUID*)&(hash.digest().value()[0]);
    LPOLESTR tmp;
    ::StringFromCLSID(*guid, &tmp);
    _AtlModule.moduleCLSID = wstring(tmp);
    ::CoTaskMemFree(tmp);

    // get hex string for debug purposes
    std::string s(hash.digest().hex_str_value());
    wstring md5hex(s.begin(), s.end());
    
    // dump some handy info
    logger->debug(L"Module executable: " + _AtlModule.moduleExec.wstring());
    logger->debug(L"    name:  " + _AtlModule.moduleManifest->name);
    logger->debug(L"    uuid:  " + _AtlModule.moduleManifest->uuid);
    logger->debug(L"     md5:  " + md5hex);
    logger->debug(L"   clsid:  " + _AtlModule.moduleCLSID);
    
    instance;
    result = _AtlModule.DllMain(reason, reserved); 
    logger->debug(L"BHO::DllMain _AtlModule.DllMain -> " + 
                  boost::lexical_cast<wstring>(result));
    
    logger->debug(L"----------------------------------------------------------\n");

    return result;
}
