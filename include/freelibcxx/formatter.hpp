#pragma once
#include "freelibcxx/algorithm.hpp"
#include "freelibcxx/assert.hpp"
#include "freelibcxx/optional.hpp"
#include "freelibcxx/span.hpp"
#include "freelibcxx/utils.hpp"
#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace freelibcxx
{
optional<int> str2int(span<const char> in, int base = 10);
optional<unsigned int> str2uint(span<const char> in, int base = 10);

optional<int64_t> str2int64(span<const char> in, int base = 10);
optional<uint64_t> str2uint64(span<const char> in, int base = 10);

optional<int> int2str(span<char> buffer, int val, int base = 10);
optional<int> uint2str(span<char> buffer, unsigned int val, int base = 10);

optional<int> int642str(span<char> buffer, int64_t val, int base = 10);
optional<int> uint642str(span<char> buffer, uint64_t val, int base = 10);

optional<int> double2str(span<char> buffer, double val, int precision);
optional<double> str2double(const char *ptr, char **end);

static inline bool is_valid_char(char ch)
{
    return (ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z');
};

inline optional<int> str2int(span<const char> in, int base)
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

inline optional<unsigned int> str2uint(span<const char> in, int base)
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

inline optional<int64_t> str2int64(span<const char> in, int base)
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
        int64_t num = result;
        char c = *ptr;
        if (c >= '0' && c <= '9')
        {
            num = num * base - (c - '0');
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
                num = num * base - (index + 10);
            }
        }
        if (num > result)
        {
            return nullopt;
        }
        result = num;
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

inline optional<uint64_t> str2uint64(span<const char> in, int base)
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
        uint64_t num = result;
        char c = *ptr;
        if (c >= '0' && c <= '9')
        {
            num = num * base + (c - '0');
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
            num = num * base + (index + 10);
        }
        if (num < result)
        {
            return nullopt;
        }
        result = num;
    }
    return result;
}

inline optional<int> int2str(span<char> buffer, int val, int base) { return int642str(buffer, val, base); }
inline optional<int> uint2str(span<char> buffer, unsigned int val, int base) { return uint642str(buffer, val, base); }

inline optional<int> int642str(span<char> buffer, int64_t val, int base)
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

inline optional<int> uint642str(span<char> buffer, uint64_t val, int base)
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

enum class format_error : uint8_t
{
    none,
    invalid_format,
    argument_mismatch,
    unsupported_type,
};

struct format_result
{
    uint64_t written;
    bool truncated;
    format_error error;
};

namespace format_detail
{
template <typename T> constexpr bool is_string_type()
{
    using type = std::remove_cv_t<std::remove_reference_t<T>>;
    return std::is_same_v<type, char *> || std::is_same_v<type, const char *> ||
           (std::is_array_v<type> && std::is_same_v<std::remove_cv_t<std::remove_extent_t<type>>, char>);
}

template <typename T> constexpr bool is_integer_type()
{
    using type = std::remove_cv_t<std::remove_reference_t<T>>;
    return std::is_integral_v<type> || std::is_enum_v<type>;
}

template <typename T> constexpr bool valid_type_for(char spec)
{
    using type = std::remove_cv_t<std::remove_reference_t<T>>;
    if (spec == 'p')
        return std::is_pointer_v<type> || std::is_null_pointer_v<type>;
    if (spec == 'd' || spec == 'x' || spec == 'X')
        return is_integer_type<T>();
    if (spec != 0)
        return false;
    return is_string_type<T>() || is_integer_type<T>() || std::is_pointer_v<type> || std::is_null_pointer_v<type>;
}

template <typename... Args> constexpr bool valid_argument(size_t index, char spec)
{
    size_t current = 0;
    bool valid = false;
    ((index == current++ ? valid = valid_type_for<Args>(spec) : false), ...);
    return valid;
}

template <typename... Args> consteval bool valid_literal(const char *format, size_t length)
{
    size_t argument = 0;
    for (size_t i = 0; i < length; i++)
    {
        if (format[i] == '{')
        {
            if (i + 1 < length && format[i + 1] == '{')
            {
                i++;
                continue;
            }

            i++;
            char spec = 0;
            bool has_width = false;
            size_t width = 0;
            if (i < length && format[i] == ':')
            {
                i++;
                if (i < length && format[i] == '0')
                {
                    has_width = true;
                    i++;
                }
                while (i < length && format[i] >= '0' && format[i] <= '9')
                {
                    has_width = true;
                    width = width * 10 + static_cast<size_t>(format[i] - '0');
                    if (width > 64)
                        return false;
                    i++;
                }
                if (i < length && format[i] != '}')
                {
                    spec = format[i];
                    i++;
                }
                if ((has_width && (width == 0 || width > 64)) ||
                    (spec != 'd' && spec != 'x' && spec != 'X' && spec != 'p'))
                    return false;
            }
            if (i >= length || format[i] != '}' || argument >= sizeof...(Args) ||
                !valid_argument<Args...>(argument, spec))
                return false;
            argument++;
        }
        else if (format[i] == '}' && (i + 1 >= length || format[i + 1] != '}'))
        {
            return false;
        }
        else if (format[i] == '}')
        {
            i++;
        }
    }
    return argument == sizeof...(Args);
}

struct writer
{
    span<char> destination;
    size_t written = 0;
    bool truncated = false;

    void put(char value)
    {
        if (written < destination.size())
            destination[written++] = value;
        else
            truncated = true;
    }

    void put(const char *value)
    {
        if (value == nullptr)
        {
            put("<null>", 6);
            return;
        }
        while (*value != '\0')
            put(*value++);
    }

    void put(const char *value, size_t length)
    {
        for (size_t i = 0; i < length; i++)
            put(value[i]);
    }
};

template <typename T, bool IsEnum = std::is_enum_v<T>> struct integer_value_type
{
    using type = T;
};

template <typename T> struct integer_value_type<T, true>
{
    using type = std::underlying_type_t<T>;
};

template <typename T> void append_integer(writer &output, T value, char spec, size_t width)
{
    using type = std::remove_cv_t<std::remove_reference_t<T>>;
    using value_type = typename integer_value_type<type>::type;
    using unsigned_type = std::make_unsigned_t<value_type>;
    constexpr bool signed_value = std::is_signed_v<value_type>;
    const int base = spec == 'x' || spec == 'X' ? 16 : 10;
    char buffer[sizeof(uint64_t) * 2 + 2]{};
    size_t length = 0;

    if constexpr (signed_value)
    {
        if (base == 10)
            length = static_cast<size_t>(
                int642str(span<char>(buffer, sizeof(buffer)), static_cast<int64_t>(value), base).value_or(0));
        else
            length = static_cast<size_t>(uint642str(span<char>(buffer, sizeof(buffer)),
                                                    static_cast<uint64_t>(static_cast<unsigned_type>(value)), base)
                                             .value_or(0));
    }
    else
        length = static_cast<size_t>(uint642str(span<char>(buffer, sizeof(buffer)),
                                                static_cast<uint64_t>(static_cast<unsigned_type>(value)), base)
                                         .value_or(0));

    if (spec == 'X')
    {
        for (size_t i = 0; i < length; i++)
        {
            if (buffer[i] >= 'a' && buffer[i] <= 'f')
                buffer[i] = static_cast<char>(buffer[i] - ('a' - 'A'));
        }
    }

    for (size_t i = length; i < width; i++)
        output.put('0');
    output.put(buffer, length);
}

template <typename T> void append_value(writer &output, const T &value, char spec, size_t width)
{
    using type = std::remove_cv_t<std::remove_reference_t<T>>;
    if constexpr (is_string_type<T>())
    {
        output.put(value);
    }
    else if constexpr (std::is_same_v<type, bool>)
    {
        output.put(value ? "true" : "false");
    }
    else if constexpr (std::is_same_v<type, char>)
    {
        if (spec == 0)
            output.put(value);
        else
            append_integer(output, value, spec, width);
    }
    else if constexpr (is_integer_type<T>())
    {
        append_integer(output, value, spec, width);
    }
    else if constexpr (std::is_pointer_v<type> || std::is_null_pointer_v<type>)
    {
        const uint64_t address = [](const T &pointer) {
            if constexpr (std::is_null_pointer_v<type>)
                return uint64_t{0};
            else
                return reinterpret_cast<uint64_t>(pointer);
        }(value);
        output.put("0x");
        append_integer(output, address, 'x', width);
    }
    else
    {
        output.put("<unsupported>");
    }
}

template <size_t Index, typename Head, typename... Tail>
void append_argument(writer &output, size_t wanted, char spec, size_t width, const Head &head, const Tail &...tail)
{
    if (wanted == Index)
        append_value(output, head, spec, width);
    else if constexpr (sizeof...(Tail) != 0)
        append_argument<Index + 1>(output, wanted, spec, width, tail...);
}
} // namespace format_detail

template <typename... Args> struct format_string
{
    const char *value;
    size_t length;

    template <size_t Length>
    consteval format_string(const char (&literal)[Length])
        : value(literal)
        , length(Length - 1)
    {
        if (!format_detail::valid_literal<Args...>(literal, Length - 1))
            __builtin_unreachable();
    }
};

template <typename... Args>
format_result format_to_view(span<char> destination, const char *value, size_t length, const Args &...args)
{
    format_detail::writer output{destination};
    size_t argument = 0;
    for (size_t i = 0; i < length; i++)
    {
        if (value[i] == '{')
        {
            if (i + 1 < length && value[i + 1] == '{')
            {
                output.put('{');
                i++;
                continue;
            }
            i++;
            char spec = 0;
            size_t width = 0;
            bool has_width = false;
            bool has_specifier = false;
            if (i < length && value[i] == ':')
            {
                has_specifier = true;
                i++;
                if (i < length && value[i] == '0')
                {
                    has_width = true;
                    i++;
                }
                while (i < length && value[i] >= '0' && value[i] <= '9')
                {
                    has_width = true;
                    if (width > 64)
                        return {output.written, output.truncated, format_error::invalid_format};
                    width = width * 10 + static_cast<size_t>(value[i] - '0');
                    i++;
                }
                if (i < length && value[i] != '}')
                {
                    spec = value[i];
                    i++;
                }
            }
            if ((has_width && (width == 0 || width > 64)) || (has_specifier && spec == 0) ||
                (i < length && value[i] != '}') || (i >= length || value[i] != '}'))
                return {output.written, output.truncated, format_error::invalid_format};
            if (argument >= sizeof...(Args))
                return {output.written, output.truncated, format_error::argument_mismatch};
            if (!format_detail::valid_argument<Args...>(argument, spec))
                return {output.written, output.truncated, format_error::unsupported_type};
            if constexpr (sizeof...(Args) != 0)
                format_detail::append_argument<0>(output, argument, spec, width, args...);
            argument++;
        }
        else if (value[i] == '}' && i + 1 < length && value[i + 1] == '}')
        {
            output.put('}');
            i++;
        }
        else if (value[i] == '}')
        {
            return {output.written, output.truncated, format_error::invalid_format};
        }
        else
        {
            output.put(value[i]);
        }
    }
    if (argument != sizeof...(Args))
        return {output.written, output.truncated, format_error::argument_mismatch};
    return {output.written, output.truncated, format_error::none};
}

template <typename... Args>
format_result format_to(span<char> destination, format_string<std::type_identity_t<Args>...> format,
                        const Args &...args)
{
    return format_to_view(destination, format.value, format.length, args...);
}
} // namespace freelibcxx
