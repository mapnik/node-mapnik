#ifndef __NODE_MAPNIK_PBF_HPP__
#define __NODE_MAPNIK_PBF_HPP__

#include <stdint.h>
#include <stdexcept>
#include <string>
#include <cstring>
#include <cassert>
#include <sstream>

namespace pbf {

#ifdef _WINDOWS
    // TODO - windows support for force inline? http://msdn.microsoft.com/en-us/library/z8y1yy88.aspx
    #define PBF_INLINE
#else
    #define FORCEINLINE inline __attribute__((always_inline))
    #define NOINLINE __attribute__((noinline))
    #define PBF_INLINE FORCEINLINE
#endif

class message {
    typedef const char * value_type;
    value_type data_;
    value_type end_;
public:
    uint64_t value;
    uint32_t tag;

    PBF_INLINE message(value_type data, std::size_t length);

    PBF_INLINE bool next();
    PBF_INLINE uint64_t varint();
    PBF_INLINE int64_t svarint();
    PBF_INLINE std::string string();
    PBF_INLINE float float32();
    PBF_INLINE double float64();
    PBF_INLINE int64_t int64();
    PBF_INLINE bool boolean();
    PBF_INLINE void skip();
    PBF_INLINE void skipValue(uint64_t val);
    PBF_INLINE void skipBytes(uint64_t bytes);
    PBF_INLINE value_type getData();
};

message::message(value_type data, std::size_t length)
    : data_(data),
      end_(data + length)
{
}

bool message::next()
{
    if (data_ < end_) {
        value = varint();
        tag = static_cast<uint32_t>(value >> 3);
        return true;
    }
    return false;
}


/*
 * varint adapted from upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2008-2011 Google Inc.  See LICENSE for details.
 * Author: Josh Haberman <jhaberman@gmail.com>
 */

uint64_t message::varint()
{
    int8_t byte(0x80);
    uint64_t result = 0;
    int bitpos;
    for (bitpos = 0; bitpos < 70 && (byte & 0x80); bitpos += 7) {
        if (data_ >= end_) {
            throw std::runtime_error("unterminated varint, unexpected end of buffer");
        }
        result |= ((uint64_t)(byte = *data_) & 0x7F) << bitpos;
        data_++;
    }
    if (bitpos == 70 && (byte & 0x80)) {
        throw std::runtime_error("unterminated varint (too long)");
    }

    return result;
}

int64_t message::svarint()
{
    uint64_t n = varint();
    return (n >> 1) ^ -static_cast<int64_t>((n & 1));
}

std::string message::string()
{
    uint64_t len = varint();
    value_type string = static_cast<value_type>(data_);
    skipBytes(len);
    return std::string(string, len);
}

float message::float32()
{
    skipBytes(4);
    float result;
    std::memcpy(&result, data_ - 4, 4);
    return result;
}
double message::float64()
{
    skipBytes(8);
    double result;
    std::memcpy(&result, data_ - 8, 8);
    return result;
}

int64_t message::int64()
{
    return (int64_t)varint();
}

bool message::boolean()
{
    skipBytes(1);
    return *(bool *)(data_ - 1);
}

void message::skip()
{
    skipValue(value);
}

void message::skipValue(uint64_t val)
{
    switch (val & 0x7) {
        case 0: // varint
            varint();
            break;
        case 1: // 64 bit
            skipBytes(8);
            break;
        case 2: // string/message
            skipBytes(varint());
            break;
        case 5: // 32 bit
            skipBytes(4);
            break;
        default:
            std::stringstream msg;
            msg << "cannot skip unknown type " << (val & 0x7);
            throw std::runtime_error(msg.str());
    }
}

void message::skipBytes(uint64_t bytes)
{
    data_ += bytes;
    if (data_ > end_) {
        throw std::runtime_error("unexpected end of buffer");
    }
}

message::value_type message::getData()
{
  return data_;
}

}

#endif // __NODE_MAPNIK_PBF_HPP__
