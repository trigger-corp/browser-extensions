#include "stdafx.h"
#include "Registry.h"


/**
 * open key
 */
bool Registry::open() {
    LONG result;
    result = ::RegOpenKeyEx(this->root, this->subkey.c_str(), 0,
                            KEY_READ | KEY_WRITE,
                            &this->key);
    if (result != ERROR_SUCCESS) {
        logger->debug(L"Registry::open failed"
                      L" -> " + boost::lexical_cast<wstring>(this->root) +
                      L" -> " + this->subkey);
        return false;
    }
    return true;
}


/** 
 * create key if it doesn't exist
 */
bool Registry::create() 
{
    if (this->open()) {
        logger->debug(L"Registry::create key exists"
                      L" -> " + boost::lexical_cast<wstring>(this->root) +
                      L" -> " + this->subkey);
        return true;
    }

    logger->debug(L"Registry::create"
                  L" -> " + boost::lexical_cast<wstring>(this->root) +
                  L" -> " + this->subkey);

    LONG result;
    result = ::RegCreateKeyEx(this->root, this->subkey.c_str(),
                              NULL,
                              NULL,
                              REG_OPTION_NON_VOLATILE,
                              KEY_READ | KEY_WRITE,
                              NULL,
                              &this->key,
                              NULL);
    if (result != ERROR_SUCCESS) {
        logger->debug(L"Registry::create failed"
                      L" -> " + boost::lexical_cast<wstring>(this->root) +
                      L" -> " + this->subkey);    
        return false;
    }

    return true;
}


/**
 * set key 
 */
bool Registry::set() 
{
    logger->debug(L"Registry::set"
                  L" -> " + boost::lexical_cast<wstring>(this->root) +
                  L" -> " + this->subkey +    
                  L" -> " + this->name +
                  L" -> " + this->value);    

    if (!this->open()) {
        return false;
    }

    LONG result;
    switch (this->type) {
    case Registry::string:
        result = ::RegSetValueEx(key, name.c_str(), 
                                 NULL, 
                                 REG_SZ, 
                                 (BYTE*)value.c_str(),
                                 (DWORD)((value.length() * 2) + 1));
        break;
    case Registry::dword:
        result = ::RegSetValueEx(key, name.c_str(), 
                                 NULL, 
                                 REG_DWORD, 
                                 (BYTE*)&value_dword,
                                 sizeof(DWORD));
        break;
    default:
        logger->error(L"Registry::set unknown type"
                      L" -> " + boost::lexical_cast<wstring>(this->type));
        return false;
        break;
    }

    if (result != ERROR_SUCCESS) {
        logger->debug(L"Registry::set failed"
                      L" -> " + boost::lexical_cast<wstring>(this->root) +
                      L" -> " + this->subkey +    
                      L" -> " + this->name +
                      L" -> " + this->value);    
        return false;
    }

    return true;
}


/** 
 * delete key
 */
#include <strsafe.h> /* for StringCchCopy */
bool Registry::del() 
{
    logger->debug(L"Registry::del"
                  L" -> " + boost::lexical_cast<wstring>(this->root) +
                  L" -> " + this->subkey);    

    if (!this->open()) {
        return false;
    }

    TCHAR buf[MAX_PATH * 2];
    StringCchCopy(buf, MAX_PATH * 2, subkey.c_str());

    return this->rdel(this->root, buf) != FALSE;
}


// from: http://msdn.microsoft.com/en-us/library/ms724235(VS.85).aspx
bool Registry::rdel(HKEY hKeyRoot, LPTSTR lpSubKey)
{
    logger->debug(L"Registry::rdel -> " + wstring(lpSubKey));

    LPTSTR lpEnd;
    LONG result;
    DWORD size;
    TCHAR szName[MAX_PATH];
    HKEY hKey;
    FILETIME ftWrite;

    // First, see if we can delete the key without having to recurse.
    result = ::RegDeleteKey(hKeyRoot, lpSubKey);
    if (result == ERROR_SUCCESS) {
        return true;
    }

    result = ::RegOpenKeyEx(hKeyRoot, lpSubKey, 0, KEY_READ, &hKey);
    if (result == ERROR_FILE_NOT_FOUND) {
        return true;
    } else if (result != ERROR_SUCCESS) {
        logger->debug(L"Registry::rdel error opening key");
        return false;
    }

    // Check for an ending slash and add one if it is missing.
    lpEnd = lpSubKey + lstrlen(lpSubKey);
    if (*(lpEnd - 1) != TEXT('\\')) {
        *lpEnd =  TEXT('\\');
        lpEnd++;
        *lpEnd =  TEXT('\0');
    }

    // Enumerate the keys
    size = MAX_PATH;
    result = ::RegEnumKeyEx(hKey, 0, szName, &size, NULL,
                            NULL, NULL, &ftWrite);
    if (result == ERROR_SUCCESS) {
        do {
            StringCchCopy(lpEnd, MAX_PATH*2, szName);
            if (!this->rdel(hKeyRoot, lpSubKey)) {
                break;
            }
            size = MAX_PATH;
            result = ::RegEnumKeyEx(hKey, 0, szName, &size, NULL,
                                    NULL, NULL, &ftWrite);
        } while (result == ERROR_SUCCESS);
    }
    lpEnd--;
    *lpEnd = TEXT('\0');
    ::RegCloseKey(hKey);

    // Try again to delete the key.
    result = ::RegDeleteKey(hKeyRoot, lpSubKey);

    return (result == ERROR_SUCCESS);
}
