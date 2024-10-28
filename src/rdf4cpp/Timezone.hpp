#ifndef RDF4CPP_TIMEZONE_HPP
#define RDF4CPP_TIMEZONE_HPP

#include <chrono>
#include <format>
#include <string_view>

#include <dice/hash.hpp>

#include <rdf4cpp/datatypes/rdf.hpp>
#include <rdf4cpp/datatypes/registry/util/CharConvExt.hpp>

#include <boost/multiprecision/cpp_int.hpp>

namespace rdf4cpp {
struct Timezone {
    // heavily inspired by https://howardhinnant.github.io/date/tz.html#Examples

    std::chrono::minutes offset = std::chrono::minutes{0};

    static constexpr const char *begin_tokens = "Z+-";

    constexpr Timezone() = default;

    inline explicit Timezone(const std::chrono::time_zone *tz, std::chrono::time_point<std::chrono::system_clock> n = std::chrono::system_clock::now())
        : offset(std::chrono::duration_cast<std::chrono::minutes>(tz->get_info(n).offset)) {
    }

    constexpr explicit Timezone(std::chrono::hours h) noexcept
        : offset(h) {}

    constexpr explicit Timezone(std::chrono::minutes h) noexcept
        : offset(h) {}

    constexpr auto operator<=>(const Timezone &) const noexcept = default;

    static constexpr Timezone parse(std::string_view v, std::string_view dt) {
        Timezone tz{};
        if (v == "Z") {
            return tz;
        }
        bool negative = false;
        if (v[0] == '-') {
            negative = true;
        }
        v = v.substr(1);
        auto sep = v.find(':');
        if (sep == std::string::npos) {
            throw InvalidNode{std::format("{} parsing error: timezone expected :", dt)};
        }
        std::chrono::hours h{datatypes::registry::util::from_chars<int32_t, "timezone">(v.substr(0, sep))};
        tz.offset = std::chrono::minutes{datatypes::registry::util::from_chars<int32_t, "timezone">(v.substr(sep + 1))} + std::chrono::minutes{h};
        if (negative) {
            tz.offset *= -1;
        }
        if (tz.offset.count() < -840 || tz.offset.count() > 840) {
            throw InvalidNode{std::format("{} parsing error: timezone offset too big", dt)};
        }
        return tz;
    }

    static constexpr std::optional<Timezone> parse_optional(std::string_view &s, std::string_view dt) {
        auto p = s.find_first_of(begin_tokens, 1);
        if (p == 0 || p == std::string::npos)
            return std::nullopt;
        auto pre = s.substr(0, p);
        auto tz = parse(s.substr(p), dt);
        s = pre;
        return tz;
    }

    // sign, hours, :, minutes
    static constexpr size_t max_canonical_string_chars = 1+(std::numeric_limits<int64_t>::digits10+1)+1+2;
    template<std::output_iterator<char> T>
    T to_canonical_string(T o) const noexcept {
        if (offset == std::chrono::minutes{0}) {
            *o = 'Z';
            ++o;
            return o;
        }
        auto h = std::chrono::floor<std::chrono::hours>(std::chrono::abs(offset));
        auto m = std::chrono::abs(offset) - h;
        return std::format_to(o, "{}{:02}:{:02}", offset >= std::chrono::minutes{0} ? '+' : '-', h.count(), m.count());
    }
    [[nodiscard]] std::string to_canonical_string() const noexcept {
        std::string buf{};
        buf.reserve(max_canonical_string_chars);
        to_canonical_string(std::back_inserter(buf));
        return buf;
    }

    [[nodiscard]] const std::chrono::time_zone *get_tz(std::chrono::time_point<std::chrono::system_clock> n = std::chrono::system_clock::now()) const {
        for (const auto &tz : std::chrono::get_tzdb().zones) {
            if (tz.get_info(n).offset == std::chrono::seconds(offset)) {
                return &tz;
            }
        }
        return nullptr;
    }

    template<typename Duration>
    [[nodiscard]] auto to_sys(const std::chrono::local_time<Duration> &tp) const noexcept {
        return std::chrono::sys_time<std::common_type_t<Duration, std::chrono::seconds>>{(tp - offset).time_since_epoch()};
    }

    template<typename Duration>
    [[nodiscard]] auto to_local(const std::chrono::sys_time<Duration> &tp) const noexcept {
        return std::chrono::local_time<std::common_type_t<Duration, std::chrono::seconds>>{(tp + offset).time_since_epoch()};
    }

    template<typename Duration>
    [[nodiscard]] std::chrono::sys_info get_info(const std::chrono::sys_time<Duration> &) const noexcept {
        return std::chrono::sys_info{
                std::chrono::sys_seconds{std::chrono::seconds{0L}},
                std::chrono::sys_seconds{std::chrono::seconds{std::numeric_limits<int64_t>::max()}},
                offset,
                std::chrono::minutes{0},
                to_canonical_string()};
    }

    const Timezone *operator->() const noexcept {
        return this;
    }

    static constexpr Timezone max_value() noexcept {
        return Timezone{std::chrono::hours{14}};
    };
    static constexpr Timezone min_value() noexcept {
        return Timezone{std::chrono::hours{-14}};
    };
};

using OptionalTimezone = std::optional<Timezone>;

template<typename I = int64_t>
struct Year {
    // adapted from https://howardhinnant.github.io/date_algorithms.html
    I year = 0;

    [[nodiscard]] constexpr bool is_leap() const noexcept(noexcept(year % 100)) {
        return year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
    }

    constexpr auto operator<=>(Year const &) const noexcept = default;
};

template<typename Y = int64_t>
struct Date {
    // adapted from https://howardhinnant.github.io/date_algorithms.html
    Year<Y> year = Year<Y>{0};
    std::chrono::month month = std::chrono::month{1};
    std::chrono::day day = std::chrono::day{1};

    template<typename P>
    using time_point = std::chrono::time_point<std::chrono::system_clock, std::chrono::duration<P, std::chrono::days::period>>;
    template<typename P>
    using time_point_local = std::chrono::time_point<std::chrono::local_t, std::chrono::duration<P, std::chrono::days::period>>;

private:
    static constexpr bool arithmetic_noexcept = noexcept(Y{} + Y{}) && noexcept(Y{} - Y{}) && noexcept(Y{} / Y{}) && noexcept(Y{} * Y{});

    static constexpr std::chrono::day last_day_in_month(Year<Y> year, std::chrono::month month) noexcept(noexcept(year.is_leap())) {
        assert(month.ok());
        constexpr unsigned char common[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
        auto m = static_cast<unsigned int>(month);
        return std::chrono::day{m != 2 || !year.is_leap() ? common[m - 1] : 29u};
    }

public:
    constexpr Date() noexcept  = default;
    constexpr explicit Date(std::chrono::year_month_day ymd) noexcept(noexcept(Year<Y>(static_cast<int>(ymd.year()))))
        : year(static_cast<int>(ymd.year())), month(ymd.month()), day(ymd.day()) {
    }
    constexpr Date(Year<Y> const &y, std::chrono::month m, std::chrono::day d) noexcept
        : year(y), month(m), day(d) {
    }
    constexpr Date(Year<Y> const &y, std::chrono::month m, std::chrono::last_spec) noexcept(noexcept(last_day_in_month(y, m)))
        : year(y), month(m), day(last_day_in_month(y, m)) {
    }
    template<typename P>
    constexpr explicit Date(time_point<P> sd) noexcept(arithmetic_noexcept) {
        static_assert(std::numeric_limits<unsigned>::digits >= 18, "This algorithm has not been ported to a 16 bit unsigned integer");
        static_assert(std::numeric_limits<Y>::digits >= 20, "This algorithm has not been ported to a 16 bit signed integer");
        static_assert(std::numeric_limits<P>::digits >= 20, "This algorithm has not been ported to a 16 bit signed integer");
        P z = sd.time_since_epoch().count();
        z += 719468;
        P const era = (z >= 0 ? z : z - 146096) / 146097;
        auto const doe = static_cast<P>(z - era * 146097);                    // [0, 146096]
        auto const yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;  // [0, 399]
        P const y = static_cast<P>(yoe) + era * 400;
        auto const doy = doe - (365 * yoe + yoe / 4 - yoe / 100);  // [0, 365]
        auto const mp = (5 * doy + 2) / 153;                       // [0, 11]
        auto const d = doy - (153 * mp + 2) / 5 + 1;               // [1, 31]
        auto const m = mp < 10 ? mp + 3 : mp - 9;                  // [1, 12]
        year = Year<Y>{static_cast<Y>(y + (m <= 2))};
        month = std::chrono::month{static_cast<unsigned>(m)};
        day = std::chrono::day{static_cast<unsigned>(d)};
    }
    template<typename P>
    constexpr explicit Date(time_point_local<P> sd) noexcept(arithmetic_noexcept)
        : Date(time_point<P>(sd.time_since_epoch())) {
    }

    template<typename P = boost::multiprecision::checked_int128_t>
    [[nodiscard]] constexpr time_point<P> to_time_point() const noexcept(arithmetic_noexcept) {
        static_assert(std::numeric_limits<unsigned>::digits >= 18, "This algorithm has not been ported to a 16 bit unsigned integer");
        static_assert(std::numeric_limits<Y>::digits >= 20, "This algorithm has not been ported to a 16 bit signed integer");
        static_assert(std::numeric_limits<Y>::digits >= 20, "This algorithm has not been ported to a 16 bit signed integer");
        P y = year.year;
        auto m = static_cast<unsigned int>(month);
        auto d = static_cast<unsigned int>(day);
        y -= m <= 2;
        P const era = (y >= 0 ? y : y - 399) / 400;
        auto const yoe = static_cast<unsigned>(y - era * 400);                 // [0, 399]
        unsigned const doy = (153 * (m > 2 ? m - 3 : m + 9) + 2) / 5 + d - 1;  // [0, 365]
        unsigned const doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;            // [0, 146096]
        // note that the epoch of system_clock is specified as 00:00:00 Coordinated Universal Time (UTC), Thursday, 1 January 1970
        return time_point<P>{typename time_point<P>::duration{era * 146097 + static_cast<P>(doe) - 719468}};
    }
    template<typename P = boost::multiprecision::checked_int128_t>
    [[nodiscard]] constexpr time_point_local<P> to_time_point_local() const noexcept(arithmetic_noexcept) {
        return time_point_local<P>{to_time_point<P>().time_since_epoch()};
    }

    [[nodiscard]] constexpr bool ok() const noexcept(noexcept(last_day_in_month(year, month))) {
        return month.ok() && day.ok() && day <= last_day_in_month(year, month);
    }

    constexpr auto operator<=>(Date const &) const noexcept = default;
};

template<typename Y = int64_t>
struct YearMonth {
    Year<Y> year = Year<Y>{0};
    std::chrono::month month = std::chrono::month{1};

    constexpr auto operator<=>(YearMonth const &) const noexcept = default;
};

using DurationNano = std::chrono::duration<boost::multiprecision::checked_int128_t, std::chrono::nanoseconds::period>;
using TimePoint = std::chrono::time_point<std::chrono::local_t, DurationNano>;
// system_clock does not use leap seconds, as required by rdf (xsd)
using TimePointSys = std::chrono::time_point<std::chrono::system_clock, DurationNano>;
using ZonedTime = std::chrono::zoned_time<DurationNano, Timezone>;

namespace util {

inline constexpr Date<> time_point_replacement_date{Year<>(1972), std::chrono::December, std::chrono::last};
inline constexpr DurationNano time_point_replacement_time_of_day{0};

constexpr TimePoint construct_timepoint(Date<> const &date, DurationNano time_of_day) {
    auto sd = date.to_time_point_local();
    auto ms = static_cast<TimePoint>(sd);
    ms += time_of_day;
    return ms;
}

constexpr std::pair<Date<>, DurationNano> deconstruct_timepoint(TimePoint const &tp) {
    auto days = std::chrono::floor<std::chrono::days>(tp);
    return {Date{days}, tp - days};
}

} // namespace util

}  // namespace rdf4cpp

namespace std::chrono {
    template<>
    struct zoned_traits<::rdf4cpp::Timezone> {
        static ::rdf4cpp::Timezone default_zone() noexcept {
            return ::rdf4cpp::Timezone{};
        }
    };
} // namespace std::chrono

#ifndef DOXYGEN_PARSER
template<typename Policy>
struct dice::hash::dice_hash_overload<Policy, rdf4cpp::Timezone> {
    static size_t dice_hash(rdf4cpp::Timezone const &x) noexcept {
        auto off = x.offset.count();
        return dice::hash::dice_hash_templates<Policy>::dice_hash(off);
    }
};
template<typename Policy>
struct dice::hash::dice_hash_overload<Policy, rdf4cpp::OptionalTimezone> {
    static size_t dice_hash(rdf4cpp::OptionalTimezone const &x) noexcept {
        auto off = x.has_value() ? x->offset.count() : std::chrono::minutes{std::chrono::hours{15}}.count();
        return dice::hash::dice_hash_templates<Policy>::dice_hash(off);
    }
};
template<typename Policy>
struct dice::hash::dice_hash_overload<Policy, ::boost::multiprecision::checked_int128_t> {
    static size_t dice_hash(::boost::multiprecision::cpp_int const &x) noexcept {
        return dice::hash::dice_hash_templates<Policy>::dice_hash(::boost::multiprecision::hash_value(x));
    }
};
template<typename Policy, typename Y>
struct dice::hash::dice_hash_overload<Policy, rdf4cpp::Year<Y>> {
    static size_t dice_hash(rdf4cpp::Year<Y> const &x) noexcept {
        return dice::hash::dice_hash_templates<Policy>::dice_hash(x.year);
    }
};
template<typename Policy, typename Y>
struct dice::hash::dice_hash_overload<Policy, rdf4cpp::Date<Y>> {
    static size_t dice_hash(rdf4cpp::Date<Y> const &x) noexcept {
        return dice::hash::dice_hash_templates<Policy>::dice_hash(std::tie(x.year, x.month, x.day));
    }
};
template<typename Policy, typename Y>
struct dice::hash::dice_hash_overload<Policy, rdf4cpp::YearMonth<Y>> {
    static size_t dice_hash(rdf4cpp::YearMonth<Y> const &x) noexcept {
        return dice::hash::dice_hash_templates<Policy>::dice_hash(std::tie(x.year, x.month));
    }
};

template<typename Y>
struct std::formatter<rdf4cpp::Year<Y>> : std::formatter<string_view> {
    auto format(rdf4cpp::Year<Y> const &p, format_context &ctx) const {
        return std::format_to(ctx.out(), "{:0{}}", p.year, p.year < 0 ? 5 : 4);
    }
};
template<typename Y>
struct std::formatter<rdf4cpp::Date<Y>> : std::formatter<string_view> {
    auto format(rdf4cpp::Date<Y> const &p, format_context &ctx) const {
        return std::format_to(ctx.out(), "{}-{:%m}-{:%d}", p.year, p.month, p.day);
    }
};
template<typename Y>
struct std::formatter<rdf4cpp::YearMonth<Y>> : std::formatter<string_view> {
    auto format(rdf4cpp::YearMonth<Y> const &p, format_context &ctx) const {
        return std::format_to(ctx.out(), "{}-{:%m}", p.year, p.month);
    }
};
template<>
struct std::formatter<rdf4cpp::TimePoint > : std::formatter<string_view> {
    auto format(rdf4cpp::TimePoint const &p, format_context &ctx) const {
        auto [date, time] = rdf4cpp::util::deconstruct_timepoint(p);
        return std::format_to(ctx.out(), "{}T{:%H:%M:%S}", date, std::chrono::hh_mm_ss{std::chrono::duration_cast<std::chrono::nanoseconds>(time)});
    }
};
template<>
struct std::formatter<rdf4cpp::ZonedTime > : std::formatter<string_view> {
    auto format(rdf4cpp::ZonedTime const &p, format_context &ctx) const {
        return std::format_to(ctx.out(), "{}{}", p.get_local_time(), p.get_time_zone().to_canonical_string());
    }
};

template<typename Y>
std::ostream& operator<< (std::ostream& os, const rdf4cpp::Year<Y>& value) {
    os << std::format("{}", value);
    return os;
}
template<typename Y>
std::ostream& operator<< (std::ostream& os, const rdf4cpp::Date<Y>& value) {
    os << std::format("{}", value);
    return os;
}
template<typename Y>
std::ostream& operator<< (std::ostream& os, const rdf4cpp::YearMonth<Y>& value) {
    os << std::format("{}", value);
    return os;
}
#endif

#endif  //RDF4CPP_TIMEZONE_HPP
