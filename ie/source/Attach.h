#ifndef __ATTACH_H__
#define __ATTACH_H__

#include <util.h>
#include <generated/Forge_i.h>

#include "AccessibleBrowser.h"

using namespace ATL;


/**
 * Helper: Attach::Native*
 */
namespace Attach {
    HRESULT NativeBackground(const wstring& uuid, 
                             const wstring& url, 
                             bool isVisible,
                             INativeBackground **out,
                             unsigned int *instanceId);
    HRESULT NativeExtensions(const wstring& uuid, 
                             IDispatchEx *htmlWindow2Ex,
                             const wstring& name, 
                             unsigned int instanceId,
                             const wstring& location,
                             INativeExtensions **out);
    HRESULT NativeMessaging(const wstring& uuid, 
                            IDispatchEx *htmlWindow2Ex,
                            const wstring& name, 
                            unsigned int instanceId,
                            INativeMessaging **out);
    HRESULT NativeTabs(IDispatchEx *htmlWindow2Ex,
                       const wstring& name,
                       NativeAccessible *nativeTabs);
    HRESULT NativeControls(const wstring& uuid, 
                           IDispatchEx *htmlWindow2Ex,
                           const wstring& name, 
                           unsigned int instanceId,
                           INativeControls **out);
};

#endif /* __ATTACH_H__ */
