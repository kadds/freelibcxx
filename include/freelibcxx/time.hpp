#pragma once
#include "freelibcxx/allocator.hpp"
#include "freelibcxx/extern.hpp"
#include "freelibcxx/formatter.hpp"
#include "freelibcxx/iterator.hpp"
#include "freelibcxx/optional.hpp"
#include "freelibcxx/span.hpp"
#include "freelibcxx/tuple.hpp"
#include <cstdint>
namespace freelibcxx
{
struct tm_t
{
    uint16_t nanoseconds;  ///< 0 - 1000
    uint16_t microseconds; ///< 0 - 1000
    uint16_t milliseconds; ///< 0 - 1000
    uint8_t seconds;       ///< 0 - 59
    uint8_t minutes;       ///< 0 - 59
    uint8_t hours;         ///< 0 - 23
    uint8_t mday;          ///< 1 - 31
    uint8_t month;         ///< 0 - 11
    uint8_t wday;          ///< 0 - 6 // sunday .. saturday
    uint16_t yday;         ///< 0 - 364(365)
    uint16_t year;         ///< 0 - 20000? year

    tm_t() { memset(this, 0, sizeof(tm_t)); }

    // Y-m-d H:M:S
    // like strftime
    span<char> format(span<char> buf, const char *format = "Y-m-d H:M:S");

    // like strptime
    static optional<tm_t> from(span<const char> buf, const char *format = "Y-m-d H:M:S");

    int64_t to_posix_seconds();

    int64_t to_posix_microseconds();

    static optional<tm_t> from_posix_seconds(int64_t timestamp);
};

namespace detail
{
constexpr unsigned month_table[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
constexpr unsigned leap_month_table[] = {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
constexpr unsigned cv_month_table[] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365};
constexpr unsigned cv_leap_month_table[] = {0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366};

inline void format_fixed(span<char> &buf, int fix, uint64_t val)
{
    int len = uint642str(buf, val, 10).value_or(0);
    if (len < fix)
    {
        auto prefix = fix - len;
        memmove(buf.get() + prefix, buf.get(), len);
        for (auto i = 0; i < prefix; i++)
        {
            buf[i] = '0';
        }
    }
    buf = buf.subspan(0, fix);
}

// from http://howardhinnant.github.io/date_algorithms.html

// Returns number of days since civil 1970-01-01.  Negative values indicate
//    days prior to 1970-01-01.
// Preconditions:  y-m-d represents a date in the civil (Gregorian) calendar
//                 m is in [1, 12]
//                 d is in [1, last_day_of_month(y, m)]
//                 y is "approximately" in
//                   [numeric_limits<Int>::min()/366, numeric_limits<Int>::max()/366]
//                 Exact range of validity is:
//                 [civil_from_days(numeric_limits<Int>::min()),
//                  civil_from_days(numeric_limits<Int>::max()-719468)]
template <class Int> constexpr Int days_from_civil(Int y, unsigned m, unsigned d) noexcept
{
    static_assert(std::numeric_limits<unsigned>::digits >= 18,
                  "This algorithm has not been ported to a 16 bit unsigned integer");
    static_assert(std::numeric_limits<Int>::digits >= 20,
                  "This algorithm has not been ported to a 16 bit signed integer");
    y -= m <= 2;
    const Int era = (y >= 0 ? y : y - 399) / 400;
    const unsigned yoe = static_cast<unsigned>(y - era * 400);            // [0, 399]
    const unsigned doy = (153 * (m > 2 ? m - 3 : m + 9) + 2) / 5 + d - 1; // [0, 365]
    const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;           // [0, 146096]
    return era * 146097 + static_cast<Int>(doe) - 719468;
}

// Returns year/month/day triple in civil calendar
// Preconditions:  z is number of days since 1970-01-01 and is in the range:
//                   [numeric_limits<Int>::min(), numeric_limits<Int>::max()-719468].
template <class Int> constexpr std::tuple<Int, unsigned, unsigned> civil_from_days(Int z) noexcept
{
    static_assert(std::numeric_limits<unsigned>::digits >= 18,
                  "This algorithm has not been ported to a 16 bit unsigned integer");
    static_assert(std::numeric_limits<Int>::digits >= 20,
                  "This algorithm has not been ported to a 16 bit signed integer");
    z += 719468;
    const Int era = (z >= 0 ? z : z - 146096) / 146097;
    const unsigned doe = static_cast<unsigned>(z - era * 146097);               // [0, 146096]
    const unsigned yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365; // [0, 399]
    const Int y = static_cast<Int>(yoe) + era * 400;
    const unsigned doy = doe - (365 * yoe + yoe / 4 - yoe / 100); // [0, 365]
    const unsigned mp = (5 * doy + 2) / 153;                      // [0, 11]
    const unsigned d = doy - (153 * mp + 2) / 5 + 1;              // [1, 31]
    const unsigned m = mp < 10 ? mp + 3 : mp - 9;                 // [1, 12]
    return std::tuple<Int, unsigned, unsigned>(y + (m <= 2), m, d);
}

// Returns: true if y is a leap year in the civil calendar, else false
template <class Int> constexpr bool is_leap(Int y) noexcept { return y % 4 == 0 && (y % 100 != 0 || y % 400 == 0); }

// Preconditions: m is in [1, 12]
// Returns: The number of days in the month m of common year
// The result is always in the range [28, 31].
constexpr unsigned last_day_of_month_common_year(unsigned m) noexcept { return month_table[m - 1]; }

// Returns day of week in civil calendar [0, 6] -> [Sun, Sat]
// Preconditions:  z is number of days since 1970-01-01 and is in the range:
//                   [numeric_limits<Int>::min(), numeric_limits<Int>::max()-4].
template <class Int> constexpr unsigned weekday_from_days(Int z) noexcept
{
    return static_cast<unsigned>(z >= -4 ? (z + 4) % 7 : (z + 5) % 7 + 6);
}

// Preconditions: x <= 6 && y <= 6
// Returns: The number of days from the weekday y to the weekday x.
// The result is always in the range [0, 6].
constexpr unsigned weekday_difference(unsigned x, unsigned y) noexcept
{
    x -= y;
    return x <= 6 ? x : x + 7;
}

// Preconditions: wd <= 6
// Returns: The weekday following wd
// The result is always in the range [0, 6].
constexpr unsigned next_weekday(unsigned wd) noexcept { return wd < 6 ? wd + 1 : 0; }

// Preconditions: wd <= 6
// Returns: The weekday prior to wd
// The result is always in the range [0, 6].
inline constexpr unsigned prev_weekday(unsigned wd) noexcept { return wd > 0 ? wd - 1 : 6; }

} // namespace detail

inline span<char> tm_t::format(span<char> buf, const char *format)
{
    char *b = buf.get();
    int64_t len = buf.size();
    char fmt_buf[8];

    while (*format != 0 && len > 0)
    {
        span<char> fmt_span(fmt_buf, 8);
        auto c = *format;
        switch (c)
        {
            case 'Y':
                detail::format_fixed(fmt_span, 4, year);
                break;
            case 'm':
                detail::format_fixed(fmt_span, 2, month + 1);
                break;
            case 'd':
                detail::format_fixed(fmt_span, 2, mday);
                break;
            case 'H':
                detail::format_fixed(fmt_span, 2, hours);
                break;
            case 'M':
                detail::format_fixed(fmt_span, 2, minutes);
                break;
            case 'S':
                detail::format_fixed(fmt_span, 2, seconds);
                break;
            default:
                fmt_span = fmt_span.subspan(0, 1);
                fmt_span[0] = c;
                break;
        }
        if (len < fmt_span.size())
        {
            return buf;
        }

        memcpy(b, fmt_span.get(), fmt_span.size());
        len -= fmt_span.size();
        b += fmt_span.size();

        format++;
    }
    return buf.subspan(0, buf.size() - len);
}

inline optional<tm_t> tm_t::from(span<const char> buf, const char *format)
{
    tm_t t;

    const char *b = buf.get();
    int64_t len = buf.size();

    while (*format != 0 && len > 0)
    {
        span<const char> span(b, 2);
        auto c = *format;
        switch (c)
        {
            case 'Y':
                span = ::freelibcxx::span(b, 4);
                if (auto v = str2uint64(span); v.has_value())
                {
                    t.year = v.value();
                    break;
                }
                return nullopt;
            case 'm':
                if (auto v = str2uint64(span); v.has_value())
                {
                    t.month = v.value() - 1;
                    break;
                }
                return nullopt;
            case 'd':
                if (auto v = str2uint64(span); v.has_value())
                {
                    t.mday = v.value();
                    break;
                }
                return nullopt;
            case 'H':
                if (auto v = str2uint64(span); v.has_value())
                {
                    t.hours = v.value();
                    break;
                }
                return nullopt;
            case 'M':
                if (auto v = str2uint64(span); v.has_value())
                {
                    t.minutes = v.value();
                    break;
                }
                return nullopt;
            case 'S':
                if (auto v = str2uint64(span); v.has_value())
                {
                    t.seconds = v.value();
                    break;
                }
                return nullopt;
            default:
                span = span.subspan(0, 1);
                if (c != *b)
                {
                    return nullopt;
                }
                break;
        }
        if (len < span.size())
        {
            return nullopt;
        }

        len -= span.size();
        b += span.size();
        format++;
    }
    if (t.seconds >= 60 || t.minutes >= 60 || t.hours >= 24 || t.month >= 12 || t.mday == 0)
    {
        return nullopt;
    }

    if (detail::is_leap(t.year))
    {
        if (detail::leap_month_table[t.month] < t.mday)
        {
            return nullopt;
        }
        t.yday = detail::cv_leap_month_table[t.month] + t.mday;
    }
    else
    {
        if (detail::month_table[t.month] < t.mday)
        {
            return nullopt;
        }
        t.yday = detail::cv_month_table[t.month] + t.mday;
    }

    t.wday = detail::weekday_from_days(t.to_posix_seconds() / (60 * 60 * 24));
    return t;
}

inline int64_t tm_t::to_posix_seconds()
{
    // from linux
    //
    /*
    int64_t y = year;
    int64_t m = month + 1;
    if (0 >= (m -= 2))
    {
        m += 12;
        y--;
    }
    int64_t d = y / 4 - y / 100 + y / 400 + 367 * m / 12 + mday + y * 365 - 719499;
    return (d * 60 * 60 * 24) + (hour * 60 * 60) + (minute * 60) + second;
    */

    int64_t days = detail::days_from_civil((int)year, (int)month + 1, (int)mday);
    return (days * 60 * 60 * 24) + hours * 60 * 60 + minutes * 60 + seconds;
}

inline int64_t tm_t::to_posix_microseconds()
{
    return to_posix_seconds() * 1000000 + milliseconds * 1000 + microseconds;
}

inline optional<tm_t> tm_t::from_posix_seconds(int64_t timestamp)
{
    tm_t t;

    const int64_t timestamp_per_day = 60 * 60 * 24;
    const int64_t total_days = timestamp / timestamp_per_day;
    const int64_t secs = timestamp % timestamp_per_day;

    const auto [y, m, d] = detail::civil_from_days(total_days);
    int wday = detail::weekday_from_days(total_days);

    t.year = y;
    t.month = m - 1;
    t.mday = d;

    t.seconds = secs % 60;
    t.minutes = (secs / 60) % 60;
    t.hours = secs / (60 * 60);
    t.wday = wday;
    return t;
}

} // namespace freelibcxx
