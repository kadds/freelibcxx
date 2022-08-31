#pragma once
#include "freelibcxx/optional.hpp"
#include "freelibcxx/span.hpp"

namespace freelibcxx
{

inline optional<char32_t> utf8_to_unicode(span<const char> data)
{
    unsigned char byte0 = static_cast<unsigned char>(data[0]);

    if (!(byte0 & 0x80))
    {
        // single byte
        return data[0];
    }

    int bytes = 2;
    char32_t code = 0;
    if (byte0 >= 0b1110'0000)
    {
        bytes++;
        if (byte0 >= 0b1111'0000)
        {
            bytes++;
            if (byte0 >= 0b1111'1000) [[unlikely]]
            {
                return nullopt;
            }
        }
    }
    if (data.size() < (size_t)bytes) [[unlikely]]
    {
        return nullopt;
    }

    for (int i = 1; i < bytes; i++)
    {
        code <<= 6;
        unsigned char d = static_cast<unsigned char>(data[i]);
        if ((d & 0b1100'0000) != 0b1000'0000) [[unlikely]]
        {
            return nullopt;
        }

        code |= d & 0b0011'1111;
    }

    char32_t first_byte = (byte0 & ((1 << (8 - bytes)) - 1));
    code |= first_byte << (6 * (bytes - 1));

    return code;
}

inline optional<span<const char>> advance_utf8(span<const char> &utf8stream)
{
    span<const char> token;
    for (size_t i = 0; i < utf8stream.size(); i++)
    {
        unsigned char ch = utf8stream[i];
        if (ch < 0x80 || (ch & 0b1110'0000) == 0b1100'0000 || (ch & 0b1111'0000) == 0b1110'0000 ||
            (ch & 0b1111'1000) == 0b1111'0000)
        {
            auto span = utf8stream.subspan(0, i + 1);
            utf8stream = utf8stream.subspan(i + 1);
            return span;
        }
    }

    auto span = utf8stream;
    utf8stream = utf8stream.subspan(utf8stream.size());
    return span;
}

inline optional<span<char>> unicode_to_utf8(char32_t codepoint, span<char> buffer)
{
    if (codepoint > 0x10'FFFF)
    {
        return nullopt;
    }
    if (codepoint > 0xFFFF)
    {
        if (buffer.size() < 4)
        {
            return nullopt;
        }
        buffer.get()[3] = static_cast<char>(0b1000'0000 | ((unsigned char)codepoint & 0b11'1111));
        codepoint >>= 6;
        buffer.get()[2] = static_cast<char>(0b1000'0000 | ((unsigned char)codepoint & 0b11'1111));
        codepoint >>= 6;
        buffer.get()[1] = static_cast<char>(0b1000'0000 | ((unsigned char)codepoint & 0b11'1111));
        codepoint >>= 6;
        buffer.get()[0] = static_cast<char>(0b1111'0000 | (unsigned char)codepoint);
        return buffer.subspan(0, 4);
    }
    else if (codepoint > 0x07FF)
    {
        if (buffer.size() < 3)
        {
            return nullopt;
        }
        buffer.get()[2] = static_cast<char>(0b1000'0000 | ((unsigned char)codepoint & 0b11'1111));
        codepoint >>= 6;
        buffer.get()[1] = static_cast<char>(0b1000'0000 | ((unsigned char)codepoint & 0b11'1111));
        codepoint >>= 6;
        buffer.get()[0] = static_cast<char>(0b1110'0000 | (unsigned char)codepoint);
        return buffer.subspan(0, 3);
    }
    else if (codepoint > 0x007F)
    {
        if (buffer.size() < 2)
        {
            return nullopt;
        }
        buffer.get()[1] = static_cast<char>(0b1000'0000 | ((unsigned char)codepoint & 0b11'1111));
        codepoint >>= 6;
        buffer.get()[0] = static_cast<char>(0b1100'0000 | (unsigned char)codepoint);
        return buffer.subspan(0, 2);
    }
    else
    {
        if (buffer.size() < 1)
        {
            return nullopt;
        }
        buffer.get()[0] = static_cast<char>(codepoint);
        return buffer.subspan(0, 1);
    }

    return buffer;
}

} // namespace freelibcxx
