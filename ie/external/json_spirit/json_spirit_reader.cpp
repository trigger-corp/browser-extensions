/* Copyright (c) 2007-2008 John W Wilkinson

   This source code can be used for any purpose as long as
   this comment is retained. */

// json spirit version 2.06
#include "stdafx.h"
#include "json_spirit_reader.h"
#include "json_spirit_value.h"

//#define BOOST_SPIRIT_THREADSAFE  // uncomment for multithreaded use, requires linking to boost.thead

#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/spirit/include/classic_core.hpp>
#include <boost/spirit/include/classic_confix.hpp>
#include <boost/spirit/include/classic_escape_char.hpp>
#include <boost/spirit/include/classic_lists.hpp>
using namespace BOOST_SPIRIT_CLASSIC_NS;

using namespace json_spirit;
using namespace std;
using namespace boost; 
using namespace boost::spirit;


namespace
{
    const int_parser< int64_t > int64_p = int_parser< int64_t >();

    // this class solely allows its inner classes and methods to
    // be conveniently templatised by Value type and dependent types
    //
    template< class Value_t >
    struct Reader
    {
        typedef typename Value_t::String_type     String_t;
        typedef typename Value_t::Object          Object_t;
        typedef typename Value_t::Array           Array_t;
        typedef typename String_t::value_type     Char_t;
        typedef typename String_t::const_iterator Iter_t;
        typedef Pair_impl< String_t >             Pair_t;
        typedef std::basic_istream< Char_t >      Istream_t;

        // this class's methods get called by the spirit parse resulting
        // in the creation of a JSON object or array
        //
        class Semantic_actions 
        {
        public:

            Semantic_actions( Value_t& value )
            :   value_( value )
            ,   current_p_( 0 )
            {
            }

            void begin_obj( Char_t c )
            {
                assert( c == '{' );

                begin_compound< Object_t >();
            }

            void end_obj( Char_t c )
            {
                assert( c == '}' );

                end_compound();
            }

            void begin_array( Char_t c )
            {
                assert( c == '[' );
         
                begin_compound< Array_t >();
           }

            void end_array( Char_t c )
            {
                assert( c == ']' );

                end_compound();
            }

            void new_name( Iter_t str, Iter_t end )
            {
                assert( current_p_->type() == obj_type );

                name_ = get_str( str, end );
            }

            void new_str( Iter_t str, Iter_t end )
            {
                add_to_current( get_str( str, end ) );
            }

            void new_true( Iter_t str, Iter_t end )
            {
                assert( is_eq( str, end, "true" ) );

                add_to_current( true );
            }

            void new_false( Iter_t str, Iter_t end )
            {
                assert( is_eq( str, end, "false" ) );

                add_to_current( false );
            }

            void new_null( Iter_t str, Iter_t end )
            {
                assert( is_eq( str, end, "null" ) );

                add_to_current( Value_t() );
            }

            void new_int( int64_t i )
            {
                add_to_current( i );
            }

            void new_real( double d )
            {
                add_to_current( d );
            }

        private:

            void add_first( const Value_t& value )
            {
                assert( current_p_ == 0 );

                value_ = value;
                current_p_ = &value_;
            }

            template< class Array_or_obj >
            void begin_compound()
            {
                if( current_p_ == 0 )
                {
                    add_first( Array_or_obj() );
                }
                else
                {
                    stack_.push_back( current_p_ );

                    Array_or_obj new_array_or_obj;   // avoid copy by building new array or object in place

                    add_to_current( new_array_or_obj );

                    if( current_p_->type() == array_type )
                    {
                        current_p_ = &current_p_->get_array().back(); 
                    }
                    else
                    {
                        current_p_ = &current_p_->get_obj().back().value_; 
                    }
                }
            }

            void end_compound()
            {
                if( current_p_ != &value_ )
                {
                    current_p_ = stack_.back();
                    
                    stack_.pop_back();
                }    
            }

            void add_to_current( const Value_t& value )
            {
                if( !current_p_ )
                {
                    add_first( value );
                }
                else if( current_p_->type() == array_type )
                {
                    current_p_->get_array().push_back( value );
                }
                else  if( current_p_->type() == obj_type )
                {
                    current_p_->get_obj().push_back( Pair_t( name_, value ) );
                }
            }

            Value_t& value_;             // this is the object or array that is being created
            Value_t* current_p_;         // the child object or array that is currently being constructed

            vector< Value_t* > stack_;   // previous child objects and arrays

            String_t name_;              // of current name/value pair
        };

        // the spirit grammer 
        //
        class Json_grammer : public grammar< Json_grammer >
        {
        public:

            Json_grammer( Semantic_actions& semantic_actions )
            :   actions_( semantic_actions )
            {
            }

            template< typename ScannerT >
            struct definition
            {
                definition( const Json_grammer& self )
                {
                    // first we convert the semantic action class methods to functors with the 
                    // parameter signature expected by spirit

                    typedef boost::function< void( Char_t )         > Char_action;
                    typedef boost::function< void( Iter_t, Iter_t ) > Str_action;
                    typedef boost::function< void( double )         > Real_action;
                    typedef boost::function< void( int64_t )        > Int_action;

                    Char_action begin_obj  ( boost::bind( &Semantic_actions::begin_obj,   &self.actions_, _1 ) );
                    Char_action end_obj    ( boost::bind( &Semantic_actions::end_obj,     &self.actions_, _1 ) );
                    Char_action begin_array( boost::bind( &Semantic_actions::begin_array, &self.actions_, _1 ) );
                    Char_action end_array  ( boost::bind( &Semantic_actions::end_array,   &self.actions_, _1 ) );
                    Str_action  new_name   ( boost::bind( &Semantic_actions::new_name,    &self.actions_, _1, _2 ) );
                    Str_action  new_str    ( boost::bind( &Semantic_actions::new_str,     &self.actions_, _1, _2 ) );
                    Str_action  new_true   ( boost::bind( &Semantic_actions::new_true,    &self.actions_, _1, _2 ) );
                    Str_action  new_false  ( boost::bind( &Semantic_actions::new_false,   &self.actions_, _1, _2 ) );
                    Str_action  new_null   ( boost::bind( &Semantic_actions::new_null,    &self.actions_, _1, _2 ) );
                    Real_action new_real   ( boost::bind( &Semantic_actions::new_real,    &self.actions_, _1 ) );
                    Int_action  new_int    ( boost::bind( &Semantic_actions::new_int,     &self.actions_, _1 ) );

                    // actual grammer

                    json_
                        = value_ >> end_p
                        ;

                    object_ 
                        = confix_p
                          ( 
                              ch_p('{')[ begin_obj ], 
                              !members_, 
                              ch_p('}')[ end_obj ] 
                          )
                        ;

                    members_
                        = pair_ >> *( ',' >> pair_ )
                        ;

                    pair_
                        = string_[ new_name ] 
                        >> ':' 
                        >> value_
                        ;

                    value_
                        = string_[ new_str ] 
                        | number_ 
                        | object_ 
                        | array_ 
                        | str_p( "true" ) [ new_true  ] 
                        | str_p( "false" )[ new_false ] 
                        | str_p( "null" ) [ new_null  ]
                        ;

                    array_
                        = confix_p
                          ( 
                              ch_p('[')[ begin_array ], 
                              !elements_, 
                              ch_p(']')[ end_array ] 
                          )
                        ;

                    elements_
                        = value_ >> *( ',' >> value_ )
                        ;

                    string_
                        = lexeme_d // this causes white space inside a string to be retained
                          [
                              confix_p
                              ( 
                                  '"', 
                                  *lex_escape_ch_p,
                                  '"'
                              ) 
                          ]
                        ;

                    number_
                        = strict_real_p[ new_real ] 
                        | int64_p      [ new_int  ]
                        ;
                }

                rule< ScannerT > json_, object_, members_, pair_, array_, elements_, value_, string_, number_;

                const rule< ScannerT >& start() const { return json_; }
            };

            Semantic_actions& actions_;
        };

        static bool is_eq( Iter_t str, Iter_t end, const char* c_str )
        {
            const String_t s1( str, end );
            const string s2( c_str );

            if( s1.length() != s2.length() ) return false;

            for( typename String_t::size_type i = 0; i < s1.length(); ++i )
            {
                if( s1[i] != s2[i] ) return false;
            }

            return true;
        }

        static Char_t hex_to_num( const Char_t c )
        {
            if( ( c >= '0' ) && ( c <= '9' ) ) return c - '0';
            if( ( c >= 'a' ) && ( c <= 'f' ) ) return c - 'a' + 10;
            if( ( c >= 'A' ) && ( c <= 'F' ) ) return c - 'A' + 10;
            return 0;
        }

        static Char_t hex_str_to_char( Iter_t& str )
        {
            const Char_t c1( *( ++str ) );
            const Char_t c2( *( ++str ) );

            return hex_to_num( c1 ) * 0x10 + hex_to_num( c2 );
        }       

        static Char_t unicode_str_to_char( Iter_t& str )
        {
            const Char_t c1( *( ++str ) );
            const Char_t c2( *( ++str ) );
            const Char_t c3( *( ++str ) );
            const Char_t c4( *( ++str ) );

            return hex_to_num( c1 ) * 0x1000 + hex_to_num( c2 ) * 0x100 + hex_to_num( c3 ) * 0x10 + hex_to_num( c4 );
        }

        static String_t substitute_esc_chars( Iter_t str, Iter_t end )
        {
            if( end - str < 2 ) return String_t( str, end );

            String_t result;

            Iter_t end_minus_1( end - 1 );

            for( Iter_t i = str; i < end; ++i )
            {
                const Char_t c1( *i );

                if( ( c1 == '\\' ) && ( i != end_minus_1 ) )
                {
                    ++i;
                 
                    const Char_t c2( *i );

                    switch( c2 )
                    {
                        case 't':  result += '\t'; break;
                        case 'b':  result += '\b'; break;
                        case 'f':  result += '\f'; break;
                        case 'n':  result += '\n'; break;
                        case 'r':  result += '\r'; break;
                        case '\\': result += '\\'; break;
                        case '/':  result += '/';  break;
                        case '"':  result += '"';  break;
                        case 'x':  
                        {
                            if( end - i >= 3 )  //  expecting "xHH..."
                            {
                                result += hex_str_to_char( i );  
                            }
                            break;
                        }
                        case 'u':  
                        {
                            if( end - i >= 5 )  //  expecting "uHHHH..."
                            {
                                result += unicode_str_to_char( i );  
                            }
                            break;
                        }
                    }
                }
                else
                {
                    result += c1;
                }
            }

            return result;
        }

        static String_t get_str( Iter_t str, Iter_t end )
        {
            assert( end - str >= 2 );

            Iter_t str_without_quotes( str + 1 );
            Iter_t end_without_quotes( end - 1 );

            return substitute_esc_chars( str_without_quotes, end_without_quotes );
        }

        static void stream_to_string( Istream_t& is, String_t& s )
        {
            typedef istream_iterator< Char_t, Char_t > istream_iter;

            is.unsetf( ios::skipws );

            copy( istream_iter( is ), istream_iter(), back_inserter( s ) );
        }

        static bool read_string( const String_t& s, Value_t& value )
        {
            Semantic_actions semantic_actions( value );
         
            parse_info< Iter_t > info = parse( s.begin(), s.end(), 
                                               Json_grammer( semantic_actions ), 
                                               space_p );

            return info.full;
        }

        static bool read_stream( Istream_t& is, Value_t& value )
        {
            String_t s;

            stream_to_string( is, s );
          
            return read_string( s, value );
        }
    };
}

bool json_spirit::read( const std::string& s, Value& value )
{
    return Reader< Value >::read_string( s, value );
}

bool json_spirit::read( std::istream& is, Value& value )
{
    return Reader< Value >::read_stream( is, value );
}

#ifndef BOOST_NO_STD_WSTRING

bool json_spirit::read( const std::wstring& s, wValue& value )
{
    return Reader< wValue >::read_string( s, value );
}

bool json_spirit::read( std::wistream& is, wValue& value )
{
    return Reader< wValue >::read_stream( is, value );
}

#endif
