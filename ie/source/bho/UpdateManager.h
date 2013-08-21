#ifndef __UPDATEMANAGER_H__
#define __UPDATEMANAGER_H__

#include <util.h>
#include <json_spirit/json_spirit.h>

/**
 * UpdateManager
 */
class UpdateManager 
{
 public:
    UpdateManager() {}

    UpdateManager(const wstring& url) 
        : m_url(url) {
    }

    void Check(const wstring& version);
    void OnUpdateInfo(wstring data);

    typedef shared_ptr<UpdateManager> pointer;

 private:
    wstring m_url;
};




#endif /* __UPDATEMANAGER_H__ */
