#include <ObjBase.h>
#include <ScriptExtensions.h>
#include "UpdateManager.h"


/**
 * Main module class
 */
class CForgeBHOModule : public ATL::CAtlDllModuleT< CForgeBHOModule > 
{
 public :
    DECLARE_LIBID(LIBID_ForgeBHOLib)
    DECLARE_REGISTRY_APPID_RESOURCEID(IDR_FORGEBHO, "{819F7E5E-8815-476D-A004-D5F4F03C8150}")

    bfs::wpath callerPath;
    bfs::wpath moduleExec;
    bfs::wpath modulePath;
    wstring    moduleCLSID;
    GUID       moduleGUID;
    HMODULE    moduleHandle;
    Manifest::pointer moduleManifest;
};

extern class CForgeBHOModule _AtlModule;
