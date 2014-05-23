#include <ScriptExtensions.h>
#include "vendor.h"

class CForgeModule : public ATL::CAtlDllModuleT< CForgeModule >
{
public :
    DECLARE_LIBID(LIBID_ForgeLib)
    DECLARE_REGISTRY_APPID_RESOURCEID(IDR_FORGE, VENDOR_UUID_NSTR(VENDOR_UUID_ForgeAppId))

    // We supply additional replacements for registry scripts so we don't have
    // to hardcode GUIDs into the *.rgs files.
    HRESULT AddCommonRGSReplacements(IRegistrarBase *pRegistrar) {
        HRESULT hr = __super::AddCommonRGSReplacements(pRegistrar);
        if (FAILED(hr))
            return hr;

        const struct _map {
            wchar_t *key;
            GUID guid;
        } map[] = {
            { L"LIBID_ForgeLib", LIBID_ForgeLib },
            { L"CLSID_NativeBackground", CLSID_NativeBackground },
            { L"CLSID_NativeControls", CLSID_NativeControls },
            { L"CLSID_NativeExtensions", CLSID_NativeExtensions },
            { L"CLSID_NativeMessaging", CLSID_NativeMessaging },
            { 0, 0 }
        };

        for (const struct _map *m = map; m->key; m++) {
            hr = pRegistrar->AddReplacement(m->key, CComBSTR(m->guid));
            if (FAILED(hr))
                return hr;
        }
        return S_OK;
    }

    bfs::wpath callerPath;
    bfs::wpath moduleExec;
    bfs::wpath modulePath;
    wstring    moduleCLSID;
    GUID       moduleGUID;
    HMODULE    moduleHandle;
    Manifest::pointer moduleManifest;
};

extern class CForgeModule _AtlModule;
