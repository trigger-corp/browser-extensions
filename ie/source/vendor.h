#ifndef __VENDOR_H__
#define __VENDOR_H__

/**
 * This files contains things you probably want to change when building your
 * own version of the OpenForge DLLs for Internet Explorer.
 */

// Used in DLL descriptions and various error message dialogs. This also shows
// up in the IE "Manage add-ons" interface. It's used as part of a registry key
// in Preferences.cpp so don't include back slashes.
#define VENDOR_COMPANY_NAME L"OpenForge"

// Used in DLL descriptions, see BHO.rc and Forge.rc
#define VENDOR_PRODUCT_NAME L"OpenForge Extension"
#define VENDOR_PRODUCT_VERSION 0,9,0,1
#define VENDOR_PRODUCT_VERSION_STR L"0.9.0.1"
#define VENDOR_LEGAL_COPYRIGHT L"(c) 2014 OpenForge. All rights reserved."

/**
 * Below are all GUIDs used for COM registration. You probably do _not_
 * want to change them but read on to understand the implications.
 *
 * A trick is used to allow multiple extensions to use the same OpenForge
 * DLLs. The BHO creates a CLSID for itself dynamically in DllMain() based
 * on the uuid in manifest.json. This dynamic CLSID is used to register the
 * BHO in the registry and is accepted in DllGetClassObject() as an alias
 * for the hardcoded, internally used VENDOR_UUID_BrowserHelperObject.
 * This means you can register multiple BHOs from the same DLL by pointing
 * them to different manifest files. They will appear as different
 * extensions in IE and are internally separated by their manifest uuid.
 *
 * Problems arise if you distribute your extension with OpenForge DLLs and
 * the user already has another OpenForge extension installed. Both
 * extensions will share the OpenForge DLLs last registered on the machine
 * which might be a different version than your extension expects or the
 * binaries might have been customized in other ways and break your
 * extension.
 *
 * The solution to this would be to generate new GUIDs for all the defines
 * listed below and rebuild your own DLLs. Your DLLs will be registered
 * separately on the user's machine and not shared with other extensions.
 * However OpenForge was never designed to run like this and assumes in
 * several places that it doesn't have to share namespace and other
 * resources with other instances of itself. If you go this route things
 * _will_ break and you will have to spend considerable effort to resolve
 * these issues. If you are brave enough to undertake this, please send
 * your fixes upstream so this can be resolved for all OpenForge users.
 */

/**
 * Browser Helper Object
 */

// AppId
#define VENDOR_UUID_ForgeBHOAppId 819F7E5E-8815-476D-A004-D5F4F03C8150

// Typelib
#define VENDOR_UUID_ForgeBHOLib A6F64DB8-F0DF-4C25-95FA-1FD20AC302E4

// Objects
#define VENDOR_UUID_BrowserHelperObject E017A723-53B3-4952-895D-ED7C3377C885

// Interfaces
#define VENDOR_UUID_IBrowserHelperObject 5E5BFAA1-40A0-4FD4-8FA9-66DD8D553337

// ElevationPolicy registry keys for frame injection
#define VENDOR_UUID_ELEVATION32 D007D90B-423F-40A5-BE43-05BC2ABCA970
#define VENDOR_UUID_ELEVATION64 D007D90B-423F-40A5-BE43-05BC2ABCA971


/**
 * Forge API
 */

// AppId
#define VENDOR_UUID_ForgeAppId 3187A87B-2274-4924-A644-2E2BB36C6271

// Typelib
#define VENDOR_UUID_ForgeLib BB02A111-23B2-4242-9C8E-B093BD0A2E3C

// Objects
#define VENDOR_UUID_NativeExtensions 71B97DA2-A432-42FA-AD66-28C567704807
#define VENDOR_UUID_NativeMessaging  99E2F3AB-15ED-4F76-8921-2471702C2EF3
#define VENDOR_UUID_NativeBackground 7F2FA86A-181A-4F8F-B853-51F897A91227
#define VENDOR_UUID_NativeControls   5DAAB57B-836A-456C-99D8-A5E0AF03FD94

// Interfaces
#define VENDOR_UUID_INativeExtensions        D462DCAD-F466-413A-BFB1-DB0B5FE5632D
#define VENDOR_UUID_INativeMessaging         E74BB8EA-65C0-4668-A5F0-6A28108DCA84
#define VENDOR_UUID_INativeBackground        10D40766-F80D-478A-AA30-D90A8B49C789
#define VENDOR_UUID_INativeControls          2C938654-2875-404A-8D95-664CC1F36620
#define VENDOR_UUID__INativeExtensionsEvents CFD3A45B-3B01-47D3-8433-5A72D8026392
#define VENDOR_UUID__INativeMessagingEvents  27FBF3C5-1A02-4375-ADC3-132FB74327B1
#define VENDOR_UUID__INativeBackgroundEvents 3F969EF1-4860-4564-87FC-59925DF0EC62

/**
 * Helper macros to get string version of uuid
 */
#define VENDOR_STR(s) L#s
#define VENDOR_UUID_STR(uuid)  L"{" VENDOR_STR(uuid) L"}"
#define VENDOR_UUID_NSTR(uuid)  "{" VENDOR_STR(uuid) L"}" // Needed for DECLARE_REGISTRY_APPID_RESOURCEID

#endif /* __VENDOR_H__ */
