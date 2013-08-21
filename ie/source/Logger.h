#ifndef __LOGGER_H__
#define __LOGGER_H__

using namespace ATL;

class Logger {
 public:
    enum Level {
        OFF = 0,
        CRITICAL,
        ERR,
        WARN,
        INFO,
        DBG,
        ALL
    };
    
    Logger(Level level, const std::wstring& filename = L"");
    
    void initialize(const boost::filesystem::wpath& path);

    bool enabled;
    
    std::wstring info(const std::wstring& message) {
        write(message, Logger::INFO);
        return message;
    }
    std::wstring warn(const std::wstring& message) {
        write(message, Logger::WARN);
        return message;
    }
    std::wstring error(const std::wstring& message) {
        write(message, Logger::ERR);
        return message;
    }
    std::wstring debug(const std::wstring& message) {
        write(message, Logger::DBG);
        return message;
    }

    // handy type parsers
    std::wstring parse(HRESULT hr);
    std::wstring parse(IDispatch *object);
    
    std::wstring GetLastError() {
        wchar_t *buf;
        DWORD error = ::GetLastError();
        FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                      FORMAT_MESSAGE_IGNORE_INSERTS,
                      NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), 
                      (LPTSTR)&buf, 0, NULL);
        return std::wstring(buf);
    }
    

 private:
    void write(const std::wstring& message, Level level = Logger::DBG);
    std::wstring m_filename;
    Level m_level;

 public:    
    typedef boost::shared_ptr<Logger> pointer;

    // helpers
    std::wstring stringify(TYPEDESC *typeDesc, ITypeInfo *pTypeInfo);
    std::wstring stringify(HREFTYPE refType, ITypeInfo *pti);
    std::wstring stringify(VARDESC *varDesc, ITypeInfo *pti);
    std::wstring stringify(int vt);

 protected:
    std::wstring limit(const std::wstring& s, size_t maxlen = 160) {
        size_t len = s.length();
        if (len < maxlen) {
            return s;
        }
        return s.substr(0, maxlen / 2) +  L" ... <schnip /> ... " + 
            s.substr(len - maxlen / 2);
    }
};


#endif /* __LOGGER_H__ */
