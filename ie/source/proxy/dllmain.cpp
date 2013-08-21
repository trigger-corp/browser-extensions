#include "stdafx.h"
#include "FrameServer.h"


extern "C" BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID reserverd)
{
    // get path
    wchar_t buf[MAX_PATH];
    ::GetModuleFileName(module, buf, MAX_PATH);
    boost::filesystem::wpath modulePath = boost::filesystem::wpath(buf).parent_path();

    if (reason == DLL_PROCESS_ATTACH) {
        // initialize logger 
        logger->initialize(modulePath);
        logger->debug(L"Proxy::DllMain -> " + modulePath.wstring());

        FrameServer::AddRef(true);

    } else if (reason == DLL_PROCESS_DETACH) {
        FrameServer::Release();

    } else {
        // unknown
    }

    return TRUE;
}

