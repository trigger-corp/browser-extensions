#ifndef __PREFERENCES_H__
#define __PREFERENCES_H__

#include <util.h>


class Preferences {
 public:
    Preferences(const wstring& name,
                const wstring& currentUser  = Preferences::CurrentUser,
                const wstring& localMachine = Preferences::LocalMachine);
    ~Preferences();
    
    // preference bundle
    bool IsFirstRunAfterInstall();
    LONG Load();
    wstringpointer Local() { return m_local; }
    LONG Save(const wstringpointer& local);
    LONG CreateDefault();

    // methods
    wstring       get(const wstring& key);
    wstring       set(const wstring& key, const wstring& value);
    wstringvector keys();
    wstringmap    all();
    bool          clear(const wstring& key);
    bool          clear();
    
    static const wstring CurrentUser;
    static const wstring LocalMachine;
    
 private:
    LONG RegistryValue(HKEY key, const wstring& subkey, 
                       wstringpointer *value);
    
    wstringpointer m_default;
    wstringpointer m_local;
    
    wstring m_currentUser;
    wstring m_localMachine;
    HKEY HKCU_IE_WRITEABLE;
    
 public:
    typedef shared_ptr<Preferences> pointer;
};


#endif /* __PREFERENCES_H__ */
