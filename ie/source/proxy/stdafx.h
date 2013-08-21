// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently,
// but are changed infrequently

#pragma once

#ifndef STRICT
#define STRICT
#endif

// atl
#include <atlcore.h>
#include <atlbase.h>

// win32 
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>

// std
#include <sstream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <numeric>
#include <functional>
#include <hash_map>

// boost
#include <boost/shared_ptr.hpp>
#include <boost/filesystem.hpp>
#include <boost/function.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/bind.hpp>
#include <boost/regex.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/join.hpp>
