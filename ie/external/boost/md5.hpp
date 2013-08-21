#ifndef BOOST_MD5_HPP_INCLUDED
#define BOOST_MD5_HPP_INCLUDED

/*
    See http://www.boost.org for updates and documentation.

    Copyright (C) 2002-2003 Stanislav Baranov. Permission to copy, use,
    modify, sell and distribute this software and its documentation is
    granted provided this copyright notice appears in all copies. This
    software is provided "as is" without express or implied warranty,
    and with no claim as to its suitability for any purpose. Derived
    from the RSA Data Security, Inc. MD5 Message-Digest Algorithm.

    Copyright (C) 1991-2, RSA Data Security, Inc. Created 1991. All rights
    reserved. License to copy and use this software is granted provided that
    it is identified as the "RSA Data Security, Inc. MD5 Message-Digest
    Algorithm" in all material mentioning or referencing this software or
    this function. License is also granted to make and use derivative works
    provided that such works are identified as "derived from the RSA Data
    Security, Inc. MD5 Message-Digest Algorithm" in all material mentioning
    or referencing the derived work. RSA Data Security, Inc. makes no
    representations concerning either the merchantability of this software
    or the suitability of this software for any particular purpose. It is
    provided "as is" without express or implied warranty of any kind. These
    notices must be retained in any copies of any part of this documentation
    and/or software.
*/

#include <cstring>  // memset, memcpy, memcmp
#include <iosfwd>  // std::istream and std::ostream

#include <boost/filesystem/path.hpp>

namespace boost
{

/*
    "The [MD5] algorithm takes as input a message of arbitrary length and
    produces as output a 128-bit "fingerprint" or "message digest" of the
    input. It is conjectured that it is computationally infeasible to produce
    two messages having the same message digest, or to produce any message
    having a given prespecified target message digest. ... The MD5 algorithm
    is designed to be quite fast on 32-bit machines." -RFC1321
*/
class md5
{
public:
    // Define these here for now to avoid inclusion of <boost/cstdint.hpp>.
    typedef unsigned char uint8_t;
    typedef unsigned int uint32_t;

    md5();
    ~md5();

    // Constructs a digest for given message data.
    md5(const char* a_str);
    md5(const void* a_data, uint32_t a_data_size);
    md5(boost::filesystem::path const& a_path);
    md5(std::istream& a_istream);
    md5(std::istream& a_istream, uint32_t a_size);

    // Updates the digest with additional message data.
    void update(const char* a_str);
    void update(const void* a_data, uint32_t a_data_size);
    void update(std::istream& a_istream);
    void update(std::istream& a_istream, uint32_t a_size);

    // A message digest.
    class digest_type
    {
    public:
        // A digest value as a 16-byte raw binary array.
        typedef uint8_t value_type[16];

        // A digest value as a 33-byte ascii-hex string.
        typedef char hex_str_value_type[33];

        digest_type()  // Constructs a zero digest.
        :
            the_hex_str_value(0)
        {
            reset();
        }

        digest_type(const value_type& a_value)
        :
            the_hex_str_value(0)
        {
            reset(a_value);
        }

        digest_type(const hex_str_value_type& a_hex_str_value)
        :
            the_hex_str_value(0)
        {
            reset(a_hex_str_value);
        }

        digest_type(const digest_type& a)
        :
            the_hex_str_value(0)
        {
            reset(a.the_value);
        }

        void reset()  // Resets to a zero digest.
        {
            memset(the_value, 0, sizeof(value_type));

            delete[] the_hex_str_value;

            the_hex_str_value = 0;
        }

        void reset(const value_type& a_value)
        {
            memcpy(the_value, a_value, sizeof(value_type));

            delete[] the_hex_str_value;

            the_hex_str_value = 0;
        }

        void reset(const hex_str_value_type& a_hex_str_value);

        digest_type& operator=(const digest_type& a)
        {
            reset(a.the_value);

            return (*this);
        }

        // Gets the digest value.
        const value_type& value() const { return the_value; }
        const hex_str_value_type& hex_str_value() const;

        ~digest_type() { reset(); }

    private:
        value_type the_value;

        mutable hex_str_value_type* the_hex_str_value;
    };

    // Acquires the digest.
    const digest_type& digest();

protected:
    void init();

    // Transforms the next message block and updates the state.
    void process_block(const uint8_t (*a_block)[64]);

private:
    uint32_t the_state[4];
    uint32_t the_count[2];   // Number of bits mod 2^64.
    uint8_t the_buffer[64];  // Input buffer.
    digest_type the_digest;  // The last cached digest.
    bool the_is_dirty;  // Whether the last cached digest is valid.
};

inline std::ostream& operator<<(std::ostream& s, const md5::digest_type& a) {
  s << std::string(a.hex_str_value());

  return (s);
}

inline std::istream& operator>>(std::istream& s, md5::digest_type& a) {
  md5::digest_type::hex_str_value_type v;

  s >> v;

  a.reset(v);

  return (s);
}

inline bool operator==(const md5::digest_type& a, const md5::digest_type& b)
{
    return (memcmp(a.value(), b.value(), 16) == 0);
}

inline bool operator!=(const md5::digest_type& a, const md5::digest_type& b)
{
    return !operator==(a, b);
}

}

#endif
