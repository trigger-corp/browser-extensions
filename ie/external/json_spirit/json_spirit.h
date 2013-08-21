#ifndef JASON_SPIRIT
#define JASON_SPIRIT

/* Copyright (c) 2007-2008 John W Wilkinson

   This source code can be used for any purpose as long as
   this comment is retained. */

// json spirit version 2.06

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
#endif

#include "json_spirit_value.h"
#include "json_spirit_reader.h"
#include "json_spirit_writer.h"


/**
 * json_spirit convenience functions 
 */
class json_util {
 public:
    static bool same_name(const json_spirit::Pair& pair, 
                          const std::string& name ) {
        return pair.name_ == name;
    }

    static bool wsame_name(const json_spirit::wPair& pair, 
                           const std::wstring& name )   {
        return pair.name_ == name;
    }

    static const json_spirit::Value& find_value(const json_spirit::Object& obj, 
                                                const std::string& name) {
        json_spirit::Object::const_iterator i;
        i = find_if(obj.begin(), obj.end(), boost::bind(same_name, _1, 
                                                        boost::ref(name)));
        return (i != obj.end()) ? i->value_ : json_spirit::Value::null;
    }

    static const json_spirit::wValue& wfind_value(const json_spirit::wObject& obj, 
                                                 const std::wstring& name) {
        json_spirit::wObject::const_iterator i;
        i = find_if(obj.begin(), obj.end(), boost::bind(wsame_name, _1, 
                                                        boost::ref(name)));
        return (i != obj.end()) ? i->value_ : json_spirit::wValue::null;
    }

    static bool exists(const json_spirit::Object& obj, 
                       const std::string& name) {
        json_spirit::Object::const_iterator i;
        i = find_if(obj.begin(), obj.end(), boost::bind(same_name, _1, 
                                                        boost::ref(name)));
        return i != obj.end();
    }

    static bool wexists(const json_spirit::wObject& obj, 
                        const std::wstring& name)   {
        json_spirit::wObject::const_iterator i;
        i = find_if(obj.begin(), obj.end(), boost::bind(wsame_name, _1, 
                                                        boost::ref(name)));
        return i != obj.end();
    }

    static int find_int(const json_spirit::Object& obj, 
                        const std::string& name) {
        json_spirit::Value v = find_value(obj, name);
        return v == json_spirit::Value::null ? -1 : v.get_int();
    }

    static int wfind_int(const json_spirit::wObject& obj, 
                         const std::wstring& name) {
        json_spirit::wValue v = wfind_value(obj, name);
        return v == json_spirit::wValue::null ? -1 : v.get_int();
    }

    static bool find_bool(const json_spirit::Object& obj, 
                          const std::string& name) {
        json_spirit::Value v = find_value(obj, name);
        return v == json_spirit::Value::null ? false : v.get_bool();
    }

    static bool wfind_bool(const json_spirit::wObject& obj, 
                           const std::wstring& name) {
        json_spirit::wValue v = wfind_value(obj, name);
        return v == json_spirit::wValue::null ? false : v.get_bool();
    }

    static std::string find_str(const json_spirit::Object& obj, 
                                const std::string& name) {
        json_spirit::Value v = find_value(obj, name);
        return v == json_spirit::Value::null ? "" : v.get_str();
    }

    static std::wstring wfind_str(const json_spirit::wObject& obj, 
                                  const std::wstring& name) {
        json_spirit::wValue v = wfind_value(obj, name);
        return v == json_spirit::wValue::null ? L"" : v.get_str();
    }

    static json_spirit::Array find_array(const json_spirit::Object& obj, 
                                         const std::string& name) {
        json_spirit::Value v = find_value(obj, name);
        return v == json_spirit::Value::null 
            ? json_spirit::Array() 
            : v.get_array();
    }

    static json_spirit::wArray wfind_array(const json_spirit::wObject& obj, 
                                           const std::wstring& name) {
        json_spirit::wValue v = wfind_value(obj, name);
        return v == json_spirit::wValue::null 
            ? json_spirit::wArray() : v.get_array();
    }

    static std::vector<std::string> find_strarray(const json_spirit::Object& obj, 
                                                  const std::string& name) {
        json_spirit::Value v = find_value(obj, name);
        return strings_to_vector(v == json_spirit::Value::null 
                                 ? json_spirit::Array() 
                                 : v.get_array());
    }

    static std::vector<std::wstring> wfind_strarray(const json_spirit::wObject& obj, 
                                                    const std::wstring& name) {
        json_spirit::wValue v = wfind_value(obj, name);
        return wstrings_to_vector(v == json_spirit::wValue::null
                                  ? json_spirit::wArray() 
                                  : v.get_array());
    }

    static std::map<std::string, std::string> find_strmap(const json_spirit::Object& obj, 
                                                          const std::string& name) {
        std::map<std::string, std::string> ret;
        json_spirit::Object o = find_obj(obj, name);
        json_spirit::Object::iterator i;
        for (i = o.begin(); i != o.end(); i++) {
            std::string key   = i->name_;
            std::string value = i->value_.get_str();
            ret[key] = value;
        }
        return ret;
    }

    static std::map<std::wstring, std::wstring> wfind_strmap(const json_spirit::wObject& obj, 
                                                             const std::wstring& name) {
        std::map<std::wstring, std::wstring> ret;
        json_spirit::wObject o = wfind_obj(obj, name);
        json_spirit::wObject::iterator i;
        for (i = o.begin(); i != o.end(); i++) {
            std::wstring key   = i->name_;
            std::wstring value = i->value_.get_str();
            ret[key] = value;
        }
        return ret;
    }

    static json_spirit::Object find_obj(const json_spirit::Object& obj, 
                                           const std::string& name) {
        json_spirit::Value v = find_value(obj, name);
        return v == json_spirit::Value::null 
            ? json_spirit::Object()
            : v.get_obj();
    }

    static json_spirit::wObject wfind_obj(const json_spirit::wObject& obj, 
                                          const std::wstring& name) {
        json_spirit::wValue v = wfind_value(obj, name);
        return v == json_spirit::wValue::null 
            ? json_spirit::wObject() 
            : v.get_obj();
    }

    static std::vector<std::string> strings_to_vector(const json_spirit::Array array) {
        std::vector<std::string> ret;
        json_spirit::Array::const_iterator i;
        for (i = array.begin(); i != array.end(); i++) {
            ret.push_back(i->get_str());
        }
        return ret;
    }

    static std::vector<std::wstring> wstrings_to_vector(const json_spirit::wArray array) {
        std::vector<std::wstring> ret;
        json_spirit::wArray::const_iterator i;
        for (i = array.begin(); i != array.end(); i++) {
            ret.push_back(i->get_str());
        }
        return ret;
    }
};

#endif
