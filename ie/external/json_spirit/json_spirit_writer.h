#ifndef JASON_SPIRIT_WRITER
#define JASON_SPIRIT_WRITER

/* Copyright (c) 2007-2008 John W Wilkinson

   This source code can be used for any purpose as long as
   this comment is retained. */

// json spirit version 2.06

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
#endif

#include "json_spirit_value.h"

namespace json_spirit
{
    // functions to convert JSON Values to text, 
    // the "formatted" versions add whitespace to format the output nicely

    void         write          ( const Value&   value, std::ostream&  os );
    void         write_formatted( const Value&   value, std::ostream&  os );
    std::string  write          ( const Value&   value );
    std::string  write_formatted( const Value&   value );

#ifndef BOOST_NO_STD_WSTRING
    std::wstring write          ( const wValue&  value );
    std::wstring write_formatted( const wValue&  value );
    void         write          ( const wValue&  value, std::wostream& os );
    void         write_formatted( const wValue&  value, std::wostream& os );
#endif
}

#endif
