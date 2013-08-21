// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently,
// but are changed infrequently

#pragma once

#ifndef STRICT
#define STRICT
#endif

// bho
#include "targetver.h"
#include "resource.h"

// afx
//#include <afxwin.h>
//#include <afxcmn.h>

// atl
#define _ATL_APARTMENT_THREADED
#define _ATL_NO_AUTOMATIC_NAMESPACE
#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS	// some CString constructors will be explicit
#define ATL_NO_ASSERT_ON_DESTROY_NONEXISTENT_WINDOW
#include <atlbase.h>
#include <atlcom.h>
#include <atlctl.h>
#include <atltypes.h>
#include <atlsafe.h>

// win32
#define WIN32_LEAN_AND_MEAN
#include <winreg.h>
#include <msxml6.h>

// internet explorer
#include <mshtml.h>
#include <mshtmdid.h>
#include <exdispid.h>

// std
#include <sstream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <numeric>
#include <functional>

// boost
#include <boost/shared_ptr.hpp>
#include <boost/filesystem.hpp>
#include <boost/function.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/bind.hpp>
#include <boost/regex.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/join.hpp>

// json_string
#include <json_spirit/json_spirit.h>
