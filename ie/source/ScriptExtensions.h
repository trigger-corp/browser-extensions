#ifndef __SCRIPTEXTENSIONS_H__
#define __SCRIPTEXTENSIONS_H__

#include <util.h>
#include <json_spirit/json_spirit.h>


/**
 * Manifest 
 *
 * Wrapper for manifest.json
 */
class Manifest {
 public:
    typedef shared_ptr<Manifest> pointer;
    typedef std::map<wstring, Manifest::pointer> map;

    class BrowserAction {
    public:
        wstring    default_popup;
        wstring    default_title;
        wstring    default_icon;
        wstringmap default_icons;
    };
    class ContentScript {
    public:
        wstringvector matches;
		wstringvector exclude_matches;
        wstringvector js;
        wstringvector css;
        wstring run_at;
        bool all_frames;
    };
    typedef std::vector<ContentScript> ContentScripts;
    class Logging {
    public:
        wstring level;
        bool console;
        wstring filename;
    };

    wstring name;
    wstring author;
    wstring version;
    wstring description;
    wstring uuid;
    wstring background_page;
    wstringvector permissions;
    ContentScripts content_scripts;
    BrowserAction browser_action;
    Logging logging;

    void dump() {
        logger->info(L"manifest.json: ");
        logger->info(L"  name: " + name);
        logger->info(L"  author: " + author);
        logger->info(L"  version: " + version);
        logger->info(L"  description: " + description);
        logger->info(L"  uuid: " + uuid);
        logger->info(L"  background_page: " + background_page);
        logger->info(L"  permissions: " + std::accumulate(permissions.begin(), 
                                                          permissions.end(), 
                                                          wstring(L", ")));
        logger->info(L"  content_scripts: ");
        ContentScripts::const_iterator content_script = content_scripts.begin();
        for (; content_script != content_scripts.end(); content_script++) {
            logger->info(L"    matches: " + std::accumulate(content_script->matches.begin(), 
                                                            content_script->matches.end(), 
                                                            wstring(L", ")));
            logger->info(L"    matches: " + std::accumulate(content_script->exclude_matches.begin(), 
                                                            content_script->exclude_matches.end(), 
                                                            wstring(L", ")));
            logger->info(L"    js: " + std::accumulate(content_script->js.begin(), 
                                                       content_script->js.end(), 
                                                       wstring(L", ")));
            logger->info(L"    css: " + std::accumulate(content_script->css.begin(), 
                                                        content_script->css.end(), 
                                                        wstring(L", ")));
            logger->info(L"    run_at: " + content_script->run_at);
            logger->info(L"    all_frames: " + boost::lexical_cast<wstring>(content_script->all_frames));
        }
        logger->info(L"  browser_action.default_popup: " + browser_action.default_popup);
        logger->info(L"  browser_action.default_title: " + browser_action.default_title);
        logger->info(L"  browser_action.default_icon: " + browser_action.default_icon);
        logger->info(L"  browser_action.default_icons: ");
        wstringmap::iterator default_icon = browser_action.default_icons.begin();
        for (; default_icon != browser_action.default_icons.end(); default_icon++) {
            logger->info(L"    " + default_icon->first + L": " + default_icon->second);
        }
        logger->info(L"  logging.level: " + logging.level);
        logger->info(L"  logging.console: " + boost::lexical_cast<wstring>(logging.console));
        logger->info(L"  logging.filename: " + boost::lexical_cast<wstring>(logging.filename));
    };
};


/**
 * ScriptExtensions
 */
class ScriptExtensions {
 public:
    ScriptExtensions(const bfs::wpath& path, bool reload = true);
    
    void Reload();
    
    typedef std::map<wstring, wstringvector>   matchmap;     // match -> [ name ]
    typedef std::map<wstring, wstringpointer>  scriptmap;    // name  -> content
    typedef std::pair<wstring, wstringpointer> scriptpair;   // name . content
    typedef std::vector<scriptpair>            scriptvector; // [ name . content ]

    scriptvector read_scripts(const wstringvector& scripts) {
        scriptvector ret;
        wstringvector::const_iterator name = scripts.begin();
        for (; name != scripts.end(); name++) {
            ret.push_back(scriptpair(*name, m_scripts[*name]));
        }
        return ret;        
    }

    scriptvector read_styles(const wstringvector& styles) {
        scriptvector ret;
        wstringvector::const_iterator name = styles.begin();
        for (; name != styles.end(); name++) {
            ret.push_back(scriptpair(*name, m_styles[*name]));
        }
        return ret;        
    }
    
    bfs::wpath pathManifest;
    Manifest::pointer manifest;
    wstringpointer background_page;

 private:
    bfs::wpath m_path;
    scriptmap m_scripts;       // name -> content
    scriptmap m_styles;        // name -> content

 public:
    Manifest::pointer ParseManifest() {
        return this->ParseManifest(this->pathManifest);
    }
    Manifest::pointer ParseManifest(const bfs::wpath& path);
    
 public:    
    typedef shared_ptr<ScriptExtensions> pointer;
};


#endif /* __SCRIPTEXTENSIONS_H__ */
