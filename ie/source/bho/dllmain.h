#include <ObjBase.h>
#include <ScriptExtensions.h>
#include "UpdateManager.h"
#include "vendor.h"


/**
 * Main module class
 */
class CForgeBHOModule : public ATL::CAtlDllModuleT< CForgeBHOModule > 
{
 public :
    DECLARE_LIBID(LIBID_ForgeBHOLib)
    DECLARE_REGISTRY_APPID_RESOURCEID(IDR_FORGEBHO, VENDOR_UUID_NSTR(VENDOR_UUID_ForgeBHOAppId))

    bfs::wpath callerPath;
    bfs::wpath moduleExec;
    bfs::wpath modulePath;
    wstring    moduleCLSID;
    GUID       moduleGUID;
    HMODULE    moduleHandle;
    Manifest::pointer moduleManifest;
};

extern class CForgeBHOModule _AtlModule;
