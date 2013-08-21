#include "stdafx.h"
#include "Preferences.h"

#include <generated/Forge_i.h> /* for: */
#include "dllmain.h"           /*   _AtlModule */

#include <iepmapi.h> /* for IEGetWriteableHKCU */


/**
 * TODO - vendorspec
 */
const wstring Preferences::CurrentUser  = 
    L"trigger.io\\";
const wstring Preferences::LocalMachine =
    L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\";



/**
 * Lifecycle: Construction
 *
 * @subkey registry subkey for application preferences
 */
Preferences::Preferences(const wstring& name, 
                         const wstring& currentUser, 
                         const wstring& localMachine)
    : m_currentUser(currentUser + name),
      m_localMachine(localMachine + name)
{
    /*logger->debug(L"Preferences::Preferences"
                  L" -> " + m_currentUser +
                  L" -> " + m_localMachine);*/

    HRESULT hr;
    hr = ::IEGetWriteableHKCU(&HKCU_IE_WRITEABLE);    
    if (FAILED(hr)) {
        logger->debug(L"Preferences::Preferences could not open HKCU_IE_WRITEABLE");
        HKCU_IE_WRITEABLE = HKEY_CURRENT_USER;
    }
}


/**
 * Lifecycle: Destruction
 */
Preferences::~Preferences()
{
    //logger->debug(L"Preferences::~Preferences");
    HRESULT hr;
    hr = ::RegCloseKey(HKCU_IE_WRITEABLE);    
    if (FAILED(hr)) {
        logger->debug(L"Preferences::~Preferences could not close HKCU_IE_WRITEABLE");
    }
}


/**
 * Load preferences from registry
 */
LONG Preferences::Load()
{
	HKEY key;
    LONG result;

    result = ::RegOpenKeyEx(HKCU_IE_WRITEABLE, m_currentUser.c_str(), 
                            NULL, KEY_READ, &key);
    if (result != ERROR_SUCCESS) {
        goto done;
    }
    
    result = this->RegistryValue(key, L"Data", &m_local);
    if (result != ERROR_SUCCESS) {
        goto done;
    }

    result = this->RegistryValue(key, L"Default", &m_default);
    if (result != ERROR_SUCCESS) {
        goto done;
    }

done:        
    ::RegCloseKey(key);
    return result;
}


/** 
 * Save local preferences to registry
 */
LONG Preferences::Save(const wstringpointer& local)
{
	HKEY key;
    LONG result;

    if (!local || local->length() == 0) {
        return ERROR_SUCCESS;
    }

    result = ::RegCreateKeyEx(HKCU_IE_WRITEABLE, m_currentUser.c_str(), 
                              NULL, NULL, 
                              REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE,
                              NULL, &key, NULL);
    if (result != ERROR_SUCCESS) {
        goto done;
    }

    result = ::RegSetValueEx(key, L"Data", 
                             NULL, REG_SZ, 
                             (BYTE*)local->c_str(), 
                             (DWORD)(local->length() * 2) + 1);
    if (result != ERROR_SUCCESS) {
        goto done;
    }
    m_local = local;
    
done:
    ::RegCloseKey(key);
    return result;
}


/**
 * Check if plugin registry entries have been created
 */
bool Preferences::IsFirstRunAfterInstall()
{
    LONG result;
    HKEY key;
    result = (::RegOpenKeyExW(HKCU_IE_WRITEABLE, m_currentUser.c_str(), 
                              NULL, KEY_READ, &key));
    ::RegCloseKey(key);
    return (result != ERROR_SUCCESS);
}


/**
 * Create default registry entries for plugin
 */
LONG Preferences::CreateDefault()
{
    LONG result;
    HKEY keyLocalMachine;
    HKEY keyCurrentUser;
    wstringpointer path;
    std::wifstream stream;
    wstringpointer defaults;

    // read defaults file from installation directory
    result = ::RegOpenKeyEx(HKEY_LOCAL_MACHINE, m_localMachine.c_str(), 
                            NULL, KEY_READ, &keyLocalMachine);
    if (result != ERROR_SUCCESS) {
        logger->warn(L"Preferences::CreateDefault cannot read: HKLM\\" + m_localMachine);
        goto missing_registry_entries;
    }
    result = this->RegistryValue(keyLocalMachine, L"Path", &path);
    if (result != ERROR_SUCCESS) {
        logger->warn(L"Preferences::CreateDefault cannot read path value");
        goto missing_registry_entries;
    }
    goto store_defaults;

    // if app installer hasn't run, e.g. dev environment
 missing_registry_entries:
    path = wstringpointer(new wstring((_AtlModule.modulePath / L"defaults").wstring()));
    logger->debug(L"Preferences::CreateDefault falling back to defaults file"
                  L" -> " + *path);
    
 store_defaults:    
    if (!bfs::exists(*path)) {
        result = ERROR_FILE_NOT_FOUND;
        logger->error(L"Preferences::CreateDefault defaults file doesn't exist: " + *path);
        goto done;
    }
    stream = std::wifstream(*path);
    defaults = wstringpointer(new wstring((std::istreambuf_iterator<wchar_t>(stream)),
                                          (std::istreambuf_iterator<wchar_t>())));

    // store defaults file in registry
    result = ::RegCreateKeyExW(HKCU_IE_WRITEABLE, m_currentUser.c_str(), 
                               NULL, NULL, 
                               REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE, 
                               NULL, &keyCurrentUser, NULL);
    if (result != ERROR_SUCCESS) {
        logger->error(L"Preferences::CreateDefault could not create registry defaults key"
                      L" -> " + m_currentUser);
        goto done;
    }
    result = ::RegSetValueEx(keyCurrentUser, L"Default",
                             NULL, REG_SZ, 
                             (BYTE*)defaults->c_str(), 
                             (DWORD)(defaults->length() * 2) + 1);
    if (result != ERROR_SUCCESS) {
        logger->error(L"Preferences::CreateDefault cannot store registry defaults value");
    }

    if (keyCurrentUser)  ::RegCloseKey(keyCurrentUser);
    if (keyLocalMachine) ::RegCloseKey(keyLocalMachine);

 done:
    logger->debug(L"Preferences::CreateDefault set default registry entry");
    return result;
}


/**
 * Query Registry
 */
LONG Preferences::RegistryValue(HKEY key, const wstring& subkey, 
                                wstringpointer *value)
{
    DWORD bufsize = 1024;
    DWORD readsize = bufsize;
    WCHAR *buf = (WCHAR*)calloc(bufsize, sizeof(WCHAR));
    LONG  result;

    while ((result = ::RegQueryValueEx(key, subkey.c_str(),
                                       NULL, NULL,
                                       (LPBYTE)buf, &readsize)) 
           == ERROR_MORE_DATA) {
        bufsize *= 2;
        buf = (WCHAR*)realloc(buf, bufsize);
        memset(buf, 0, sizeof(WCHAR));
        readsize = bufsize;
    }
    *value = wstringpointer(new wstring(buf));
    free(buf);

    return result;
}


/**
 * Preferences::get
 */
wstring Preferences::get(const wstring& key)
{
    wstringpointer value;
    wstring ret = L"null";

    if (this->IsFirstRunAfterInstall()) {
        logger->debug(L"Preferences::get registry not configured");
        return ret;
    }

    HKEY hkey;
    if (::RegOpenKeyEx(HKCU_IE_WRITEABLE, m_currentUser.c_str(), 
                       NULL, KEY_READ, &hkey) != ERROR_SUCCESS) {
        logger->debug(L"Preferences::get could not open key"
                      L" -> " + key);
        goto done;
    }

    if (this->RegistryValue(hkey, key, &value) != ERROR_SUCCESS) {
        logger->debug(L"Preferences::get could not read value"
                      L" -> " + key);
        goto done;
    }
    ret = *value;

 done:
    ::RegCloseKey(hkey);
    return ret;
}


/**
 * Preferences::set
 */
wstring Preferences::set(const wstring& key, const wstring& value)
{
    wstring ret = L"null";

    logger->debug(L"Preferences::set"
                  L" -> " + key +
                  L" -> " + wstring_limit(value));

    if (this->IsFirstRunAfterInstall()) {
        logger->debug(L"Preferences::set configuring registry");
        LONG result;
        result = this->CreateDefault();
        if (result != ERROR_SUCCESS) {
            logger->debug(L"Preferences::set could not configure registry");
            return ret;
        }
    }

    HKEY hkey;
    if (::RegOpenKeyEx(HKCU_IE_WRITEABLE, m_currentUser.c_str(), 
                       NULL, KEY_WRITE, &hkey) != ERROR_SUCCESS) {
        logger->debug(L"Preferences::set could not open key"
                      L" -> " + m_currentUser);
        goto done;
    }

    LONG result = ::RegSetValueEx(hkey, key.c_str(), 
                                  NULL, REG_SZ, 
                                  (BYTE*)value.c_str(), 
                                  (DWORD)(value.length() * 2) + 1);
    if (result != ERROR_SUCCESS) {
        logger->debug(L"Preferences::set could not set value"
                      L" -> " + key +
                      L" -> " + wstring_limit(value));
        goto done;
    }
    ret = value;

 done:
    ::RegCloseKey(hkey);
    return ret;
}


/**
 * Preferences::keys
 */
wstringvector Preferences::keys()
{
    wstringvector ret;

    logger->debug(L"Preferences::keys"
                  L" -> " + m_currentUser);

    HKEY hkey;
    if (::RegOpenKeyEx(HKCU_IE_WRITEABLE, m_currentUser.c_str(), 
                       NULL, KEY_READ, &hkey) != ERROR_SUCCESS) {
        logger->error(L"Preferences::keys could not open registry key");
        return ret;
    }

    wchar_t key[MAX_PATH + 1], value[MAX_PATH + 1];
    DWORD key_length = sizeof(key) / sizeof(*key);
    DWORD value_length = key_length;
    DWORD type = 0, index = 0;
    while (index < 0x1000 &&
           ::RegEnumValue(hkey, index, key, &key_length, NULL, &type, 
                          reinterpret_cast<LPBYTE>(value), &value_length)  
           != ERROR_NO_MORE_ITEMS) {
        if (type == REG_SZ && wstring(key) != L"Default") { // TODO @deprecate Default key
            ret.push_back(key);
        }
        key_length = sizeof(key) / sizeof(*key);
        value_length = key_length;
        index++;
    }
    ::RegCloseKey(hkey);

    return ret;
}


/**
 * Preferences::all
 */
wstringmap Preferences::all()
{
    wstringmap ret;

    HKEY hkey;
    if (::RegOpenKeyEx(HKCU_IE_WRITEABLE, m_currentUser.c_str(), 
                       NULL, KEY_READ, &hkey) != ERROR_SUCCESS) {
        logger->error(L"Preferences::keys could not open registry key");
        return ret;
    }

    wchar_t key[MAX_PATH + 1], value[MAX_PATH + 1];
    DWORD key_length = sizeof(key) / sizeof(*key);
    DWORD value_length = key_length;
    DWORD type = 0, index = 0;
    while (index < 0x1000 &&
           ::RegEnumValue(hkey, index, key, &key_length, NULL, &type, 
                          reinterpret_cast<LPBYTE>(value), &value_length)  
           != ERROR_NO_MORE_ITEMS) {
        if (type == REG_SZ && wstring(key) != L"Default") {
            ret[key] = value;
        }
        key_length = sizeof(key) / sizeof(*key);
        value_length = key_length;
        index++;
    }
    ::RegCloseKey(hkey);

    return ret;
}


/**
 * Preferences::clear
 */
bool Preferences::clear(const wstring& name)
{
    logger->debug(L"Preferences::clear"
                  L" -> " + m_currentUser + 
                  L" -> " + name);
    
    HKEY hkey;
    if (::RegOpenKeyEx(HKCU_IE_WRITEABLE, m_currentUser.c_str(), 
                       NULL, KEY_SET_VALUE, &hkey) != ERROR_SUCCESS) {
        logger->error(L"Preferences::clear could not open registry key");
        return false;
    }

    LONG result = ::RegDeleteValue(hkey, name.c_str());
    ::RegCloseKey(hkey);

    return result == ERROR_SUCCESS;
}


bool Preferences::clear()
{
    logger->debug(L"Preferences::clear"
                  L" -> " + m_currentUser);

    HKEY hkey;
    if (::RegOpenKeyEx(HKCU_IE_WRITEABLE, m_currentUser.c_str(), 
                       NULL, KEY_SET_VALUE, &hkey) != ERROR_SUCCESS) {
        logger->error(L"Preferences::clear could not open registry key");
        return false;
    }
    
    bool ret = true;

    wstringvector keys = this->keys();
    wstringvector::const_iterator i;
    for (i = keys.begin(); i != keys.end(); i++) {
        ret &= (::RegDeleteValue(hkey, i->c_str()) == ERROR_SUCCESS);
    }    
    ::RegCloseKey(hkey);
    
    return ret;
}
