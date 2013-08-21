#ifndef __REGISTRY_H__
#define __REGISTRY_H__

#include <util.h>

class Registry {
 public:
    Registry(HKEY root,  
             const wstring& subkey, 
             const wstring& name  = L"", 
             const wstring& value = L"") 
        : root(root), 
          subkey(subkey), 
          name(name), 
          value(value), type(Registry::string) { 
              /*logger->debug(L"Registry::Registry"
                      L" -> " + boost::lexical_cast<wstring>(this->root) +
                      L" -> " + this->subkey +
                      L" -> " + this->name +
                      L" -> " + this->value);*/
        this->key = NULL;
    }

    Registry(HKEY root,  
             const wstring& subkey, 
             const wstring& name, 
             DWORD value) 
        : root(root), 
          subkey(subkey), 
          name(name), 
          value_dword(value), type(Registry::dword) { 
              /*logger->debug(L"Registry::Registry"
                      L" -> " + boost::lexical_cast<wstring>(this->root) +
                      L" -> " + this->subkey +
                      L" -> " + this->name +
                      L" -> " + this->value);*/
        this->key = NULL;
    }

    ~Registry() {
        if (this->key != NULL && (::RegCloseKey(this->key) != ERROR_SUCCESS)) {
            logger->debug(L"Registry::~Registry error closing key"
                          L" -> " + boost::lexical_cast<wstring>(root) +
                          L" -> " + subkey);
        }
    }

    bool open();    
    bool create();
    bool set();
    bool del();

    typedef shared_ptr<Registry> pointer;

 protected:
    enum Type { string, dword };

    HKEY root;
    wstring subkey;
    wstring name;
    wstring value;
    DWORD   value_dword;
    Type    type;
    HKEY    key;

    bool rdel(HKEY hKeyRoot, LPTSTR lpSubKey);


};


#endif /* __REGISTRY_H__ */


