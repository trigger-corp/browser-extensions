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

#include <boost/md5.hpp>
#include <cstdio>  // sprintf, sscanf
#include <cassert>
#include <iostream>
#include <fstream>

namespace boost
{

namespace
{
    typedef md5::uint8_t uint8_t;
    typedef md5::uint32_t uint32_t;

    /*
        Pack/unpack between arrays of uint8_t and uint32_t in
        a platform-independent way. Size is a multiple of 4.
    */

    void pack(uint8_t* dst, const uint32_t* src, uint32_t size)
    {
        uint32_t i(0);
        uint32_t j(0);

        while (j < size)
        {
            dst[j+0] = static_cast<uint8_t>((src[i] >>  0) & 0xff);
            dst[j+1] = static_cast<uint8_t>((src[i] >>  8) & 0xff);
            dst[j+2] = static_cast<uint8_t>((src[i] >> 16) & 0xff);
            dst[j+3] = static_cast<uint8_t>((src[i] >> 24) & 0xff);

            ++i;

            j += 4;
        }
    }

    void unpack(uint32_t* dst, const uint8_t* src, uint32_t size)
    {
        uint32_t i(0);
        uint32_t j(0);

        while (j < size)
        {
            dst[i] =
                (static_cast<uint32_t>(src[j+0]) <<  0) |
                (static_cast<uint32_t>(src[j+1]) <<  8) |
                (static_cast<uint32_t>(src[j+2]) << 16) |
                (static_cast<uint32_t>(src[j+3]) << 24);

            ++i;

            j += 4;
        }
    }

    void* secure_memset(void* dst, int c, uint32_t size)
    {
        return memset(dst, c, size);
    }
}

md5::md5()
{
    init();
}

md5::~md5()
{
    // Zeroize sensitive information.
    secure_memset(the_buffer, 0, sizeof(the_buffer));
}

md5::md5(const char* a_str)
{
    init();

    update(a_str);
}

md5::md5(const void* a_data, uint32_t a_data_size)
{
    init();

    update(a_data, a_data_size);
}

md5::md5(boost::filesystem::path const& a_path) {
    std::ifstream s;
    
    s.open(a_path.string().c_str(), std::ios_base::in);
            
    assert(s.good());

    init();

    update(s);

    s.close();
}

md5::md5(std::istream& a_istream)
{
    init();

    update(a_istream);
}

md5::md5(std::istream& a_istream, uint32_t a_size)
{
    init();

    update(a_istream, a_size);
}

void md5::init()
{
    the_is_dirty = true;

    the_count[0] = 0;
    the_count[1] = 0;

    the_state[0] = 0x67452301;
    the_state[1] = 0xefcdab89;
    the_state[2] = 0x98badcfe;
    the_state[3] = 0x10325476;
}

void md5::update(const char* a_str)
{
    update(a_str, static_cast<uint32_t>(strlen(a_str)));  // Optimization possible but not worth it.
}

void md5::update(const void* a_data, uint32_t a_data_size)
{
    // Continuation after finalization is not implemented.
    // It is easy to implement - see comments in digest().
    assert(the_is_dirty);

    if (a_data_size != 0)
        the_is_dirty = true;

    // Compute number of bytes mod 64.
    uint32_t buffer_index = static_cast<uint32_t>((the_count[0] >> 3) & 0x3f);

    // Update number of bits.
    the_count[0] += (static_cast<uint32_t>(a_data_size) << 3);
    if (the_count[0] < (static_cast<uint32_t>(a_data_size) << 3))
        ++the_count[1];

    the_count[1] += (static_cast<uint32_t>(a_data_size) >> 29);

    uint32_t buffer_space = 64-buffer_index;  // Remaining buffer space.

    uint32_t input_index;

    // Transform as many times as possible.
    if (a_data_size >= buffer_space)
    {
        memcpy(the_buffer+buffer_index, a_data, buffer_space);
        process_block(&the_buffer);

        for (input_index = buffer_space; input_index+63 < a_data_size; input_index += 64)
        {
            process_block(reinterpret_cast<const uint8_t (*)[64]>(
                reinterpret_cast<const uint8_t*>(a_data)+input_index));
        }

        buffer_index = 0;
    }
    else
    {
        input_index = 0;
    }

    // Buffer remaining input.
    memcpy(the_buffer+buffer_index, reinterpret_cast<
        const uint8_t*>(a_data)+input_index, a_data_size-input_index);
}

void md5::update(std::istream& a_istream)
{
    uint8_t buffer[1024];

    while (a_istream)
    {
        a_istream.read(reinterpret_cast<char*>(&buffer[0]), sizeof(buffer));

        update(buffer, static_cast<uint32_t>(a_istream.gcount()));
    }
}

void md5::update(std::istream& a_istream, uint32_t a_size)
{
    // TODO
}

const md5::digest_type& md5::digest()
{
    if (the_is_dirty)
    {
        static const uint8_t padding[64] =
        {
            0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0x00, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0x00, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0x00, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
        };

        // Save number of bits.
        uint8_t saved_count[8];
        pack(saved_count, the_count, 8);

        // TODO: State and buffer must be saved here to make continuation possible.

        // Pad out to 56 mod 64.
        uint32_t index = static_cast<uint32_t>((the_count[0] >> 3) & 0x3f);
        uint32_t padding_size = (index < 56) ? (56 - index) : (120 - index);
        update(padding, padding_size);

        // Append size before padding.
        update(saved_count, 8);

        // Store state in digest.
        digest_type::value_type digest_value;
        pack(digest_value, the_state, sizeof(digest_type::value_type));
        the_digest.reset(digest_value);

        // TODO: State and buffer must be restored here to make continuation possible.

        the_is_dirty = false;
    }

    return the_digest;
}

void md5::digest_type::reset(const hex_str_value_type& a_hex_str_value)
{
    delete[] the_hex_str_value;

    the_hex_str_value = 0;

    assert(a_hex_str_value[sizeof(hex_str_value_type) - 1] == '\0');

    for (unsigned int i(0); i < sizeof(value_type); ++i)
    {
        unsigned int value;

        int n = sscanf_s(&a_hex_str_value[i*2], "%02x", &value);

        assert(n == 1 && value <= 0xff);

        the_value[i] = static_cast<uint8_t>(value);
    }
}

const md5::digest_type::hex_str_value_type& md5::digest_type::hex_str_value() const
{
    if (the_hex_str_value == 0)
    {
        // Note: Have to cast because 'new char[33]' returns 'char*', not 'char(*)[33]'.
        the_hex_str_value = reinterpret_cast<hex_str_value_type*>(new hex_str_value_type);
        

        for (unsigned int i(0); i < sizeof(value_type); ++i)
        {
            sprintf_s(&(*the_hex_str_value)[i*2], 3, 
                      "%02x", the_value[i]);
        }

        (*the_hex_str_value)[sizeof(hex_str_value_type) - 1] = '\0';  // Not necessary after sprintf.
    }

    return (*the_hex_str_value);
}

namespace
{
    typedef md5::uint8_t uint8_t;
    typedef md5::uint32_t uint32_t;

    const uint32_t S11(7);
    const uint32_t S12(12);
    const uint32_t S13(17);
    const uint32_t S14(22);
    const uint32_t S21(5);
    const uint32_t S22(9);
    const uint32_t S23(14);
    const uint32_t S24(20);
    const uint32_t S31(4);
    const uint32_t S32(11);
    const uint32_t S33(16);
    const uint32_t S34(23);
    const uint32_t S41(6);
    const uint32_t S42(10);
    const uint32_t S43(15);
    const uint32_t S44(21);

    inline uint32_t rotate_left(uint32_t x, uint32_t n_bits) { return (x << n_bits) | (x >> (32-n_bits)); }

    /*
        Basic MD5 functions.
    */

    inline uint32_t F(uint32_t x, uint32_t y, uint32_t z) { return (x & y) | (~x & z); }
    inline uint32_t G(uint32_t x, uint32_t y, uint32_t z) { return (x & z) | (y & ~z); }
    inline uint32_t H(uint32_t x, uint32_t y, uint32_t z) { return x ^ y ^ z; }
    inline uint32_t I(uint32_t x, uint32_t y, uint32_t z) { return y ^ (x | ~z); }

    /*
        Transformations for rounds 1, 2, 3, and 4.
    */

    inline void FF(uint32_t& a, uint32_t b, uint32_t c, uint32_t d, uint32_t x, uint32_t  s, uint32_t ac)
    {
        a += F(b, c, d) + x + ac;
        a = rotate_left(a, s) + b;
    }

    inline void GG(uint32_t& a, uint32_t b, uint32_t c, uint32_t d, uint32_t x, uint32_t s, uint32_t ac)
    {
        a += G(b, c, d) + x + ac;
        a = rotate_left(a, s) + b;
    }

    inline void HH(uint32_t& a, uint32_t b, uint32_t c, uint32_t d, uint32_t x, uint32_t s, uint32_t ac)
    {
        a += H(b, c, d) + x + ac;
        a = rotate_left(a, s) + b;
    }

    inline void II(uint32_t& a, uint32_t b, uint32_t c, uint32_t d, uint32_t x, uint32_t s, uint32_t ac)
    {
        a += I(b, c, d) + x + ac;
        a = rotate_left(a, s) + b;
    }
}

void md5::process_block(const uint8_t (*a_block)[64])
{
    uint32_t a(the_state[0]);
    uint32_t b(the_state[1]);
    uint32_t c(the_state[2]);
    uint32_t d(the_state[3]);

    /*volatile*/ uint32_t x[16];

    unpack(x, reinterpret_cast<const uint8_t*>(a_block), 64);

    // Round 1.
    FF(a, b, c, d, x[ 0], S11, 0xd76aa478); /*  1 */
    FF(d, a, b, c, x[ 1], S12, 0xe8c7b756); /*  2 */
    FF(c, d, a, b, x[ 2], S13, 0x242070db); /*  3 */
    FF(b, c, d, a, x[ 3], S14, 0xc1bdceee); /*  4 */
    FF(a, b, c, d, x[ 4], S11, 0xf57c0faf); /*  5 */
    FF(d, a, b, c, x[ 5], S12, 0x4787c62a); /*  6 */
    FF(c, d, a, b, x[ 6], S13, 0xa8304613); /*  7 */
    FF(b, c, d, a, x[ 7], S14, 0xfd469501); /*  8 */
    FF(a, b, c, d, x[ 8], S11, 0x698098d8); /*  9 */
    FF(d, a, b, c, x[ 9], S12, 0x8b44f7af); /* 10 */
    FF(c, d, a, b, x[10], S13, 0xffff5bb1); /* 11 */
    FF(b, c, d, a, x[11], S14, 0x895cd7be); /* 12 */
    FF(a, b, c, d, x[12], S11, 0x6b901122); /* 13 */
    FF(d, a, b, c, x[13], S12, 0xfd987193); /* 14 */
    FF(c, d, a, b, x[14], S13, 0xa679438e); /* 15 */
    FF(b, c, d, a, x[15], S14, 0x49b40821); /* 16 */

    // Round 2.
    GG(a, b, c, d, x[ 1], S21, 0xf61e2562); /* 17 */
    GG(d, a, b, c, x[ 6], S22, 0xc040b340); /* 18 */
    GG(c, d, a, b, x[11], S23, 0x265e5a51); /* 19 */
    GG(b, c, d, a, x[ 0], S24, 0xe9b6c7aa); /* 20 */
    GG(a, b, c, d, x[ 5], S21, 0xd62f105d); /* 21 */
    GG(d, a, b, c, x[10], S22,  0x2441453); /* 22 */
    GG(c, d, a, b, x[15], S23, 0xd8a1e681); /* 23 */
    GG(b, c, d, a, x[ 4], S24, 0xe7d3fbc8); /* 24 */
    GG(a, b, c, d, x[ 9], S21, 0x21e1cde6); /* 25 */
    GG(d, a, b, c, x[14], S22, 0xc33707d6); /* 26 */
    GG(c, d, a, b, x[ 3], S23, 0xf4d50d87); /* 27 */
    GG(b, c, d, a, x[ 8], S24, 0x455a14ed); /* 28 */
    GG(a, b, c, d, x[13], S21, 0xa9e3e905); /* 29 */
    GG(d, a, b, c, x[ 2], S22, 0xfcefa3f8); /* 30 */
    GG(c, d, a, b, x[ 7], S23, 0x676f02d9); /* 31 */
    GG(b, c, d, a, x[12], S24, 0x8d2a4c8a); /* 32 */

    // Round 3.
    HH(a, b, c, d, x[ 5], S31, 0xfffa3942); /* 33 */
    HH(d, a, b, c, x[ 8], S32, 0x8771f681); /* 34 */
    HH(c, d, a, b, x[11], S33, 0x6d9d6122); /* 35 */
    HH(b, c, d, a, x[14], S34, 0xfde5380c); /* 36 */
    HH(a, b, c, d, x[ 1], S31, 0xa4beea44); /* 37 */
    HH(d, a, b, c, x[ 4], S32, 0x4bdecfa9); /* 38 */
    HH(c, d, a, b, x[ 7], S33, 0xf6bb4b60); /* 39 */
    HH(b, c, d, a, x[10], S34, 0xbebfbc70); /* 40 */
    HH(a, b, c, d, x[13], S31, 0x289b7ec6); /* 41 */
    HH(d, a, b, c, x[ 0], S32, 0xeaa127fa); /* 42 */
    HH(c, d, a, b, x[ 3], S33, 0xd4ef3085); /* 43 */
    HH(b, c, d, a, x[ 6], S34,  0x4881d05); /* 44 */
    HH(a, b, c, d, x[ 9], S31, 0xd9d4d039); /* 45 */
    HH(d, a, b, c, x[12], S32, 0xe6db99e5); /* 46 */
    HH(c, d, a, b, x[15], S33, 0x1fa27cf8); /* 47 */
    HH(b, c, d, a, x[ 2], S34, 0xc4ac5665); /* 48 */

    // Round 4.
    II(a, b, c, d, x[ 0], S41, 0xf4292244); /* 49 */
    II(d, a, b, c, x[ 7], S42, 0x432aff97); /* 50 */
    II(c, d, a, b, x[14], S43, 0xab9423a7); /* 51 */
    II(b, c, d, a, x[ 5], S44, 0xfc93a039); /* 52 */
    II(a, b, c, d, x[12], S41, 0x655b59c3); /* 53 */
    II(d, a, b, c, x[ 3], S42, 0x8f0ccc92); /* 54 */
    II(c, d, a, b, x[10], S43, 0xffeff47d); /* 55 */
    II(b, c, d, a, x[ 1], S44, 0x85845dd1); /* 56 */
    II(a, b, c, d, x[ 8], S41, 0x6fa87e4f); /* 57 */
    II(d, a, b, c, x[15], S42, 0xfe2ce6e0); /* 58 */
    II(c, d, a, b, x[ 6], S43, 0xa3014314); /* 59 */
    II(b, c, d, a, x[13], S44, 0x4e0811a1); /* 60 */
    II(a, b, c, d, x[ 4], S41, 0xf7537e82); /* 61 */
    II(d, a, b, c, x[11], S42, 0xbd3af235); /* 62 */
    II(c, d, a, b, x[ 2], S43, 0x2ad7d2bb); /* 63 */
    II(b, c, d, a, x[ 9], S44, 0xeb86d391); /* 64 */

    the_state[0] += a;
    the_state[1] += b;
    the_state[2] += c;
    the_state[3] += d;

    // Zeroize sensitive information.
    secure_memset(reinterpret_cast<uint8_t*>(x), 0, sizeof(x));
}

}
