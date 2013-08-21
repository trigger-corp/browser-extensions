#include <ScriptExtensions.h>

class CForgeModule : public ATL::CAtlDllModuleT< CForgeModule >
{
public :
	DECLARE_LIBID(LIBID_ForgeLib)
	DECLARE_REGISTRY_APPID_RESOURCEID(IDR_FORGE, "{3187A87B-2274-4924-A644-2E2BB36C6271}")

    bfs::wpath callerPath;
    bfs::wpath moduleExec;
    bfs::wpath modulePath;
    wstring    moduleCLSID;
    GUID       moduleGUID;
    HMODULE    moduleHandle;
    Manifest::pointer moduleManifest;
};

extern class CForgeModule _AtlModule;
