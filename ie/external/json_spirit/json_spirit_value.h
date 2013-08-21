#ifndef JASON_SPIRIT_VALUE
#define JASON_SPIRIT_VALUE

/* Copyright (c) 2007-2008 John W Wilkinson

   This source code can be used for any purpose as long as
   this comment is retained. */

// json spirit version 2.06

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
#endif

#include <boost/config.hpp> 
#include <boost/cstdint.hpp> 
#include <boost/shared_ptr.hpp> 
#include <vector>
#include <string>
#include <cassert>

namespace json_spirit
{
    enum Value_type{ obj_type, array_type, str_type, bool_type, int_type, real_type, null_type };

    template< class String > struct Pair_impl;

    template< class String >        // eg std::string or std::wstring
    class Value_impl
    {
    public:

        typedef String                              String_type;
        typedef Pair_impl< String >                 Pair;
        typedef std::vector< Pair >                 Object;
        typedef std::vector< Value_impl< String > > Array;
        typedef typename String::const_pointer      Const_str_ptr;  // eg const char*

        Value_impl();  // creates null value
        Value_impl( Const_str_ptr  value ); 
        Value_impl( const String&  value );
        Value_impl( const Object&  value );
        Value_impl( const Array&   value );
        Value_impl( bool           value );
        Value_impl( int            value );
        Value_impl( boost::int64_t value );
        Value_impl( double         value );

        Value_impl( const Value_impl& other );

        bool operator==( const Value_impl& lhs ) const;

        Value_impl& operator=( const Value_impl& lhs );

        Value_type type() const;

        const String&  get_str()   const;
        const Object&  get_obj()   const;
        const Array&   get_array() const;
        bool           get_bool()  const;
        int            get_int()   const;
        boost::int64_t get_int64() const;
        double         get_real()  const;

        Object& get_obj();
        Array&  get_array();

        template< typename T > T get_value() const;  // example usage: int    i = value.get_value< int >();
                                                     // or             double d = value.get_value< double >();

        static const Value_impl null;

    private:

        Value_type type_;

        typedef boost::shared_ptr< Object > Object_ptr;
        typedef boost::shared_ptr< Array > Array_ptr;

        String str_;
        Object_ptr obj_p_;
        Array_ptr array_p_;
        bool bool_;
        boost::int64_t i_;
        double d_;
    };

    template< class String >
    struct Pair_impl
    {
        Pair_impl( const String& name, const Value_impl< String >& value );

        bool operator==( const Pair_impl< String >& lhs ) const;

        String name_;
        Value_impl< String > value_;
    };

    // typedefs for ASCII, compatible with JSON Spirit v1.02

    typedef Value_impl< std::string > Value;
    typedef Pair_impl < std::string > Pair;
    typedef Value::Object Object;
    typedef Value::Array Array;

    // typedefs for Unicode, new for JSON Spirit v2.00

#ifndef BOOST_NO_STD_WSTRING
    typedef Value_impl< std::wstring > wValue;
    typedef Pair_impl < std::wstring > wPair;
    typedef wValue::Object wObject;
    typedef wValue::Array wArray;
#endif

    ///////////////////////////////////////////////////////////////////////////////////////////////
    //
    // implementation

    template< class String >
    const Value_impl< String > Value_impl< String >::null;

    template< class String >
    Value_impl< String >::Value_impl()
    :   type_( null_type )
    {
    }

    template< class String >
    Value_impl< String >::Value_impl( const Const_str_ptr value )
    :   type_( str_type )
    ,   str_( value )
    {
    }

    template< class String >
    Value_impl< String >::Value_impl( const String& value )
    :   type_( str_type )
    ,   str_( value )
    {
    }

    template< class String >
    Value_impl< String >::Value_impl( const Object& value )
    :   type_( obj_type )
    ,   obj_p_( new Object( value ) )
    {
    }

    template< class String >
    Value_impl< String >::Value_impl( const Array& value )
    :   type_( array_type )
    ,   array_p_( new Array( value ) )
    {
    }

    template< class String >
    Value_impl< String >::Value_impl( bool value )
    :   type_( bool_type )
    ,   bool_( value )
    {
    }

    template< class String >
    Value_impl< String >::Value_impl( int value )
    :   type_( int_type )
    ,   i_( value )
    {
    }

    template< class String >
    Value_impl< String >::Value_impl( boost::int64_t value )
    :   type_( int_type )
    ,   i_( value )
    {
    }

    template< class String >
    Value_impl< String >::Value_impl( double value )
    :   type_( real_type )
    ,   d_( value )
    {
    }

    template< class String >
    Value_impl< String >::Value_impl( const Value_impl< String >& other )
    :   type_( other.type() )
    {
        switch( type_ )
        {
            case str_type:   str_     = other.get_str();                               break;
            case obj_type:   obj_p_   = Object_ptr( new Object( other.get_obj() ) );   break;
            case array_type: array_p_ = Array_ptr ( new Array ( other.get_array() ) ); break;
            case bool_type:  bool_    = other.get_bool();                              break;
            case int_type:   i_       = other.get_int64();                             break;
            case real_type:  d_       = other.get_real();                              break;
            case null_type:                                                            break;
            default: assert( false );
        };
    }

    template< class String >
    Value_impl< String >& Value_impl< String >::operator=( const Value_impl& lhs )
    {
        Value_impl tmp( lhs );

        std::swap( type_, tmp.type_ );
        std::swap( bool_, tmp.bool_ );
        std::swap( i_,    tmp.i_ );
        std::swap( d_,    tmp.d_ );
        str_    .swap( tmp.str_ );
        obj_p_  .swap( tmp.obj_p_ );
        array_p_.swap( tmp.array_p_ );

        return *this;
    }

    template< class String >
    bool Value_impl< String >::operator==( const Value_impl& lhs ) const
    {
        if( this == &lhs ) return true;

        if( type() != lhs.type() ) return false;

        switch( type_ )
        {
            case str_type:   return get_str()   == lhs.get_str();
            case obj_type:   return get_obj()   == lhs.get_obj();
            case array_type: return get_array() == lhs.get_array();
            case bool_type:  return get_bool()  == lhs.get_bool();
            case int_type:   return get_int64() == lhs.get_int64();
            case real_type:  return get_real()  == lhs.get_real();
            case null_type:  return true;
        };

        assert( false );

        return false; 
    }

    template< class String >
    Value_type Value_impl< String >::type() const
    {
        return type_;
    }

    template< class String >
    const String& Value_impl< String >::get_str() const
    {
        assert( type() == str_type );

        return str_;
    }

    template< class String >
    const typename Value_impl< String >::Object& Value_impl< String >::get_obj() const
    {
        assert( type() == obj_type );

        return *obj_p_;
    }
     
    template< class String >
    const typename Value_impl< String >::Array& Value_impl< String >::get_array() const
    {
        assert( type() == array_type );

        return *array_p_;
    }
     
    template< class String >
    bool Value_impl< String >::get_bool() const
    {
        assert( type() == bool_type );

        return bool_;
    }
     
    template< class String >
    int Value_impl< String >::get_int() const
    {
        assert( type() == int_type );

        return static_cast< int >( i_ );
    }
    
    template< class String >
    boost::int64_t Value_impl< String >::get_int64() const
    {
        assert( type() == int_type );

        return i_;
    }

    template< class String >
    double Value_impl< String >::get_real() const
    {
        assert( type() == real_type );

        return d_;
    }

    template< class String >
    typename Value_impl< String >::Object& Value_impl< String >::get_obj()
    {
        assert( type() == obj_type );

        return *obj_p_;
    }

    template< class String >
    typename Value_impl< String >::Array& Value_impl< String >::get_array()
    {
        assert( type() == array_type );

        return *array_p_;
    }

    template< class String >
    Pair_impl< String >::Pair_impl( const String& name, const Value_impl< String >& value )
    :   name_( name )
    ,   value_( value )
    {
    }

    template< class String >
    bool Pair_impl< String >::operator==( const Pair_impl& lhs ) const
    {
        if( this == &lhs ) return true;

        return ( name_ == lhs.name_ ) && ( value_ == lhs.value_ );
    }

    // converts a C string, ie. 8 bit char array, to a string object
    //
    template < class String_t >
    String_t to_str( const char* c_str )
    {
        String_t result;

        for( const char* p = c_str; *p != 0; ++p )
        {
            result += *p;
        }

        return result;
    }

    //

    namespace internal_
    {
        template< typename T >
        struct Type_to_type
        {
        };

        template< class Value > 
        int get_value( const Value& value, Type_to_type< int > )
        {
            return value.get_int();
        }
       
        template< class Value > 
        boost::int64_t get_value( const Value& value, Type_to_type< boost::int64_t > )
        {
            return value.get_int64();
        }
       
        template< class Value > 
        double get_value( const Value& value, Type_to_type< double > )
        {
            return value.get_real();
        }
       
        template< class Value > 
        typename Value::String_type get_value( const Value& value, Type_to_type< typename Value::String_type > )
        {
            return value.get_str();
        }
       
        template< class Value > 
        typename Value::Array get_value( const Value& value, Type_to_type< typename Value::Array > )
        {
            return value.get_array();
        }
       
        template< class Value > 
        typename Value::Object get_value( const Value& value, Type_to_type< typename Value::Object > )
        {
            return value.get_obj();
        }
       
        template< class Value > 
        bool get_value( const Value& value, Type_to_type< bool > )
        {
            return value.get_bool();
        }
    }

    template< class String >
    template< typename T > 
    T Value_impl< String >::get_value() const
    {
        return internal_::get_value( *this, internal_::Type_to_type< T >() );
    }
}

#endif
