#include "freelibcxx/formatter.hpp"
#include "freelibcxx/algorithm.hpp"
#include "freelibcxx/assert.hpp"
#include "freelibcxx/optional.hpp"
#include "freelibcxx/utils.hpp"
#include <limits>

namespace freelibcxx
{
static inline bool is_valid_char(char ch)
{
    return (ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z');
};

optional<int> str2int(span<const char> in, int base)
{
    auto result = str2int64(in, base);
    if (result.has_value())
    {
        int64_t value = result.value();
        if (in_range_of_type<int>(value))
        {
            return static_cast<int>(value);
        }
    }
    return nullopt;
}

optional<unsigned int> str2uint(span<const char> in, int base)
{
    auto result = str2int64(in, base);
    if (result.has_value())
    {
        uint64_t value = result.value();
        if (in_range_of_type<unsigned int>(value))
        {
            return static_cast<unsigned int>(value);
        }
    }
    return nullopt;
}

optional<int64_t> str2int64(span<const char> in, int base)
{
    CXXASSERT(base >= 2 && base <= 32);
    int64_t result = 0;
    int fac = 1;
    auto ptr = in.get();
    auto size = in.size();
    if (size == 0)
    {
        return nullopt;
    }
    for (; size > 0; size--, ptr++)
    {
        int64_t tmp_result = result;
        char c = *ptr;
        if (c >= '0' && c <= '9')
        {
            tmp_result = tmp_result * base - (c - '0');
        }
        else
        {
            if (c == '-' || c == '+')
            {
                if (size == in.size())
                {
                    fac = c == '-' ? -1 : 1;
                }
                else
                {
                    return nullopt;
                }
            }
            else
            {
                if (!is_valid_char(c))
                {
                    return nullopt;
                }
                c = tolower(c);
                int index = c - 'a';
                if (index + 10 >= base)
                {
                    return nullopt;
                }
                tmp_result = tmp_result * base - (index + 10);
            }
        }
        if (tmp_result > result)
        {
            return nullopt;
        }
        result = tmp_result;
    }
    if (fac == 1)
    {
        if (std::numeric_limits<int64_t>::min() == result)
        {
            return nullopt;
        }
        return result * -1;
    }
    // fac = -1
    return result;
}

optional<uint64_t> str2uint64(span<const char> in, int base)
{
    CXXASSERT(base >= 2 && base <= 32);
    uint64_t result = 0;
    auto ptr = in.get();
    auto size = in.size();
    if (size == 0)
    {
        return nullopt;
    }
    for (; size > 0; size--, ptr++)
    {
        uint64_t tmp_result = result;
        char c = *ptr;
        if (c >= '0' && c <= '9')
        {
            tmp_result = tmp_result * base + (c - '0');
        }
        else
        {
            if (!is_valid_char(c))
            {
                return nullopt;
            }
            c = tolower(c);
            int index = c - 'a';
            if (index + 10 >= base)
            {
                return nullopt;
            }
            tmp_result = tmp_result * base + (index + 10);
        }
        if (tmp_result < result)
        {
            return nullopt;
        }
        result = tmp_result;
    }
    return result;
}

optional<int> int2str(span<char> buffer, int val, int base) { return int642str(buffer, val, base); }
optional<int> uint2str(span<char> buffer, unsigned int val, int base) { return uint642str(buffer, val, base); }

optional<int> int642str(span<char> buffer, int64_t val, int base)
{
    CXXASSERT(base >= 2 && base <= 32);
    int offset = 0;
    int size = buffer.size();
    if (size == 0)
    {
        return nullopt;
    }
    char *ptr = buffer.get();
    int64_t old_val = val;

    do
    {
        int rest = val % base;
        val /= base;
        if (rest < 0)
        {
            rest = -rest;
        }
        if (rest < 10)
        {
            ptr[offset] = '0' + rest;
        }
        else
        {
            ptr[offset] = 'a' + rest - 10;
        }

        offset++;
        if (offset >= size)
        {
            return nullopt;
        }
    } while (val != 0);

    if (old_val < 0)
    {
        ptr[offset] = '-';
        offset++;
        if (offset >= size)
        {
            return nullopt;
        }
    }

    reverse(ptr, ptr + offset);
    return offset;
}

optional<int> uint642str(span<char> buffer, uint64_t val, int base)
{
    CXXASSERT(base >= 2 && base <= 32);
    int offset = 0;
    int size = buffer.size();
    char *ptr = buffer.get();
    if (size == 0)
    {
        return nullopt;
    }

    do
    {
        int rest = val % base;
        val /= base;
        if (rest < 10)
        {
            ptr[offset] = '0' + rest;
        }
        else
        {
            ptr[offset] = 'a' + rest - 10;
        }

        offset++;
        if (offset >= size)
        {
            return nullopt;
        }
    } while (val != 0);

    // reverse
    reverse(ptr, ptr + offset);

    return offset;
}

} // namespace freelibcxx