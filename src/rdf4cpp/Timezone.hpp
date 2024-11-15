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
        std::chrono::hours const h{datatypes::registry::util::from_chars<int32_t, "timezone">(v.substr(0, sep))};
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

using Month = std::chrono::month;
using Day = std::chrono::day;

/**
 * Like std::chrono::year, except it has a greater range.
 * adapted from https://howardhinnant.github.io/date_algorithms.html
 */
struct Year {
private:
    int64_t value_;

public:
    explicit constexpr Year(int64_t value = 0) noexcept : value_{value} {
    }

    constexpr explicit operator int64_t() const noexcept {
        return value_;
    }

    [[nodiscard]] constexpr bool is_leap() const noexcept(noexcept(value_ % 100)) {
        return value_ % 4 == 0 && (value_ % 100 != 0 || value_ % 400 == 0);
    }

    constexpr auto operator<=>(Year const &) const noexcept = default;

    friend constexpr Year operator+(Year const &y, std::chrono::years d) noexcept {
        return Year{y.value_ + d.count()};
    }
    friend constexpr Year operator+(std::chrono::years d, Year const &y) noexcept {
        return Year{y.value_ + d.count()};
    }

    constexpr Year operator+=(std::chrono::years d) noexcept {
        *this = *this + d;
        return *this;
    }

    friend constexpr Year operator-(Year const &y, std::chrono::years d) noexcept {
        return Year{y.value_ - d.count()};
    }
    friend constexpr std::chrono::years operator-(Year const &a, Year const &b) noexcept {
        return std::chrono::years{a.value_ - b.value_};
    }

    constexpr Year operator-=(std::chrono::years d) noexcept {
        *this = *this - d;
        return *this;
    }

    constexpr Year &operator++() noexcept {
        *this += std::chrono::years{1};
        return *this;
    }
    constexpr Year operator++(int) noexcept {
        Year r = *this;
        ++(*this);
        return r;
    }

    constexpr Year &operator--() noexcept {
        *this -= std::chrono::years{1};
        return *this;
    }
    constexpr Year operator--(int) noexcept {
        Year r = *this;
        --(*this);
        return r;
    }

    static constexpr Year max() noexcept {
        return Year{std::numeric_limits<int64_t>::max()};
    }
    static constexpr Year min() noexcept {
        return Year{std::numeric_limits<int64_t>::min()};
    }
};

struct YearMonth {
private:
    Year year_ = Year{0};
    Month month_ = Month{1};

    static constexpr YearMonth create_normalized(int64_t y, int64_t mo) noexcept {
        --mo;
        y += mo / 12;
        mo %= 12;
        if (mo < 0) { // fix result of % being in [-11,11]
            --y;
            mo += 12;
        }
        return YearMonth{Year{y}, std::chrono::month{static_cast<unsigned int>(mo+1)}};
    }

public:
    constexpr YearMonth() noexcept = default;

    constexpr YearMonth(Year y, std::chrono::month m) noexcept : year_{y}, month_{m} {
    }

    [[nodiscard]] constexpr Year year() const noexcept {
        return year_;
    }

    [[nodiscard]] constexpr Month month() const noexcept {
        return month_;
    }

    constexpr auto operator<=>(YearMonth const &) const noexcept = default;

    [[nodiscard]] constexpr bool ok() const noexcept {
        return month_.ok();
    }

    friend constexpr YearMonth operator+(YearMonth const &ym, std::chrono::years d) noexcept {
        return YearMonth{ym.year_ + d, ym.month()};
    }
    friend constexpr YearMonth operator+(std::chrono::years d, YearMonth const &ym) noexcept {
        return YearMonth{ym.year_ + d, ym.month()};
    }

    constexpr YearMonth& operator+=(std::chrono::years d) noexcept {
        *this = *this + d;
        return *this;
    }

    friend constexpr YearMonth operator+(YearMonth const &ym, std::chrono::months d) noexcept {
        return create_normalized(static_cast<int64_t>(ym.year()), static_cast<unsigned int>(ym.month()) + d.count());
    }
    friend constexpr YearMonth operator+(std::chrono::months d, YearMonth const &ym) noexcept {
        return create_normalized(static_cast<int64_t>(ym.year()), static_cast<unsigned int>(ym.month()) + d.count());
    }

    constexpr YearMonth& operator+=(std::chrono::months d) noexcept {
        *this = *this + d;
        return *this;
    }

    friend constexpr YearMonth operator-(YearMonth const &ym, std::chrono::years d) noexcept {
        return {ym.year() - d, ym.month()};
    }
    friend constexpr YearMonth operator-(YearMonth const &ym, std::chrono::months d) noexcept {
        return create_normalized(static_cast<int64_t>(ym.year()), static_cast<unsigned int>(ym.month()) - d.count());
    }

    friend constexpr std::chrono::months operator-(YearMonth const &a, YearMonth const &b) noexcept {
        return (a.year() - b.year()) + (a.month() - b.month());
    }

    constexpr YearMonth& operator-=(std::chrono::years d) noexcept {
        *this = *this - d;
        return *this;
    }
    constexpr YearMonth& operator-=(std::chrono::months d) noexcept {
        *this = *this - d;
        return *this;
    }
};

struct YearMonthDay {
    template<typename P>
    using time_point = std::chrono::time_point<std::chrono::system_clock, std::chrono::duration<P, std::chrono::days::period>>;
    template<typename P>
    using time_point_local = std::chrono::time_point<std::chrono::local_t, std::chrono::duration<P, std::chrono::days::period>>;

private:
    // adapted from https://howardhinnant.github.io/date_algorithms.html
    Year year_ = Year{0};
    Month month_ = Month{1};
    Day day_ = Day{1};

    static constexpr std::chrono::day last_day_in_month(Year year, std::chrono::month month) noexcept {
        assert(month.ok());
        constexpr unsigned char common[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
        auto m = static_cast<unsigned int>(month);
        return std::chrono::day{m != 2 || !year.is_leap() ? common[m - 1] : 29u};
    }

public:
    constexpr YearMonthDay() noexcept  = default;

    constexpr explicit YearMonthDay(std::chrono::year_month_day ymd) noexcept
        : year_(static_cast<int>(ymd.year())), month_(ymd.month()), day_(ymd.day()) {
    }
    constexpr YearMonthDay(Year const &y, Month m, std::chrono::day d) noexcept
        : year_(y), month_(m), day_(d) {
    }
    constexpr YearMonthDay(Year const &y, Month m, std::chrono::last_spec) noexcept
        : year_(y), month_(m), day_(last_day_in_month(y, m)) {
    }
    constexpr YearMonthDay(YearMonth const &ym, Day d) noexcept
        : year_(ym.year()), month_(ym.month()), day_(d) {
    }
    constexpr YearMonthDay(YearMonth const & ym, std::chrono::last_spec) noexcept
        : YearMonthDay(ym.year(), ym.month(), std::chrono::last) {
    }
    template<typename P>
    constexpr explicit YearMonthDay(time_point<P> sd) noexcept(noexcept(P{} + P{} * P{} - P{} / P{})) {
        static_assert(std::numeric_limits<unsigned>::digits >= 18, "This algorithm has not been ported to a 16 bit unsigned integer");
        static_assert(std::numeric_limits<int64_t>::digits >= 20, "This algorithm has not been ported to a 16 bit signed integer");
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
        year_ = Year{static_cast<int64_t>(y + (m <= 2))};
        month_ = std::chrono::month{static_cast<unsigned>(m)};
        day_ = std::chrono::day{static_cast<unsigned>(d)};
    }
    template<typename P>
    constexpr explicit YearMonthDay(time_point_local<P> sd) noexcept(noexcept(P{} + P{} * P{} - P{} / P{}))
        : YearMonthDay(time_point<P>(sd.time_since_epoch())) {
    }

    [[nodiscard]] constexpr Year year() const noexcept {
        return year_;
    }

    [[nodiscard]] constexpr Month month() const noexcept {
        return month_;
    }

    [[nodiscard]] constexpr Day day() const noexcept {
        return day_;
    }

    [[nodiscard]] constexpr time_point<boost::multiprecision::checked_int128_t> to_time_point() const {
        static_assert(std::numeric_limits<unsigned>::digits >= 18, "This algorithm has not been ported to a 16 bit unsigned integer");
        static_assert(std::numeric_limits<int64_t>::digits >= 20, "This algorithm has not been ported to a 16 bit signed integer");
        static_assert(std::numeric_limits<boost::multiprecision::checked_int128_t>::digits >= 20, "This algorithm has not been ported to a 16 bit signed integer");
        boost::multiprecision::checked_int128_t y = static_cast<int64_t>(year_);
        auto m = static_cast<unsigned int>(month_);
        auto d = static_cast<unsigned int>(day_);
        y -= m <= 2;
        boost::multiprecision::checked_int128_t const era = (y >= 0 ? y : y - 399) / 400;
        auto const yoe = static_cast<unsigned>(y - era * 400);                 // [0, 399]
        unsigned const doy = (153 * (m > 2 ? m - 3 : m + 9) + 2) / 5 + d - 1;  // [0, 365]
        unsigned const doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;            // [0, 146096]
        // note that the epoch of system_clock is specified as 00:00:00 Coordinated Universal Time (UTC), Thursday, 1 January 1970
        return time_point<boost::multiprecision::checked_int128_t>{typename time_point<boost::multiprecision::checked_int128_t>::duration{era * 146097 + static_cast<boost::multiprecision::checked_int128_t>(doe) - 719468}};
    }
    [[nodiscard]] constexpr time_point_local<boost::multiprecision::checked_int128_t> to_time_point_local() const{
        return time_point_local<boost::multiprecision::checked_int128_t>{to_time_point().time_since_epoch()};
    }

    [[nodiscard]] constexpr bool ok() const noexcept {
        return month_.ok() && day_.ok() && day_ <= last_day_in_month(year_, month_);
    }

    constexpr auto operator<=>(YearMonthDay const &) const noexcept = default;

    friend constexpr YearMonthDay operator+(YearMonthDay const &ym, std::chrono::years d) noexcept {
        return {ym.year_ + d, ym.month_, ym.day_};
    }
    friend constexpr YearMonthDay operator+(std::chrono::years d, YearMonthDay const &ym) noexcept {
        return {ym.year_ + d, ym.month_, ym.day_};
    }

    constexpr YearMonthDay & operator+=(std::chrono::years d) noexcept {
        *this = *this + d;
        return *this;
    }

    friend constexpr YearMonthDay operator+(YearMonthDay const &d, std::chrono::months m) noexcept {
        return YearMonthDay{YearMonth{d.year_, d.month_} + m, d.day_};
    }
    friend constexpr YearMonthDay operator+(std::chrono::months m, YearMonthDay const &d) noexcept {
        return YearMonthDay{YearMonth{d.year_, d.month_} + m, d.day_};
    }

    constexpr YearMonthDay & operator+=(std::chrono::months d) noexcept {
        *this = *this + d;
        return *this;
    }

    friend constexpr YearMonthDay operator-(YearMonthDay const &ym, std::chrono::years d) noexcept {
        return {ym.year_ - d, ym.month_, ym.day_};
    }
    friend constexpr YearMonthDay operator-(YearMonthDay const &d, std::chrono::months m) noexcept {
        return YearMonthDay{YearMonth{d.year_, d.month_} - m, d.day_};
    }

    constexpr YearMonthDay & operator-=(std::chrono::years d) noexcept {
        *this = *this - d;
        return *this;
    }
    constexpr YearMonthDay & operator-=(std::chrono::months d) noexcept {
        *this = *this - d;
        return *this;
    }
};

using DurationNano = std::chrono::duration<boost::multiprecision::checked_int128_t, std::chrono::nanoseconds::period>;
using TimePoint = std::chrono::time_point<std::chrono::local_t, DurationNano>;
// system_clock does not use leap seconds, as required by rdf (xsd)
using TimePointSys = std::chrono::time_point<std::chrono::system_clock, DurationNano>;
using ZonedTime = std::chrono::zoned_time<DurationNano, Timezone>;

namespace util {

inline constexpr YearMonthDay time_point_replacement_date{Year(1972), std::chrono::December, std::chrono::last};
inline constexpr DurationNano time_point_replacement_time_of_day{0};

constexpr TimePoint construct_timepoint(YearMonthDay const &date, const DurationNano& time_of_day) {
    auto sd = date.to_time_point_local();
    auto ms = static_cast<TimePoint>(sd);
    ms += time_of_day;
    return ms;
}

constexpr std::pair<YearMonthDay, DurationNano> deconstruct_timepoint(TimePoint const &tp) {
    auto days = std::chrono::floor<std::chrono::duration<DurationNano::rep, std::chrono::days::period>>(tp);
    return {YearMonthDay{days}, tp - days};
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
template<typename Policy>
struct dice::hash::dice_hash_overload<Policy, rdf4cpp::Year> {
    static size_t dice_hash(rdf4cpp::Year const &x) noexcept {
        return dice::hash::dice_hash_templates<Policy>::dice_hash(static_cast<int64_t>(x));
    }
};
template<typename Policy>
struct dice::hash::dice_hash_overload<Policy, rdf4cpp::YearMonthDay> {
    static size_t dice_hash(rdf4cpp::YearMonthDay const &x) noexcept {
        return dice::hash::dice_hash_templates<Policy>::dice_hash(std::make_tuple(x.year(), x.month(), x.day()));
    }
};
template<typename Policy>
struct dice::hash::dice_hash_overload<Policy, rdf4cpp::YearMonth> {
    static size_t dice_hash(rdf4cpp::YearMonth const &x) noexcept {
        return dice::hash::dice_hash_templates<Policy>::dice_hash(std::make_tuple(x.year(), x.month()));
    }
};

template<>
struct std::formatter<rdf4cpp::Year> : std::formatter<string_view> {
    inline auto format(rdf4cpp::Year const &p, format_context &ctx) const {
        return std::format_to(ctx.out(), "{:0{}}", static_cast<int64_t>(p), static_cast<int64_t>(p) < 0 ? 5 : 4);
    }
};
template<>
struct std::formatter<rdf4cpp::YearMonthDay> : std::formatter<string_view> {
    inline auto format(rdf4cpp::YearMonthDay const &p, format_context &ctx) const {
        return std::format_to(ctx.out(), "{}-{:%m}-{:%d}", p.year(), p.month(), p.day());
    }
};
template<>
struct std::formatter<rdf4cpp::YearMonth> : std::formatter<string_view> {
    inline auto format(rdf4cpp::YearMonth const &p, format_context &ctx) const {
        return std::format_to(ctx.out(), "{}-{:%m}", p.year(), p.month());
    }
};
template<>
struct std::formatter<rdf4cpp::TimePoint> : std::formatter<string_view> {
    inline auto format(rdf4cpp::TimePoint const &p, format_context &ctx) const {
        auto [date, time] = rdf4cpp::util::deconstruct_timepoint(p);
        return std::format_to(ctx.out(), "{}T{:%H:%M:%S}", date, std::chrono::hh_mm_ss{std::chrono::duration_cast<std::chrono::nanoseconds>(time)});
    }
};
template<>
struct std::formatter<rdf4cpp::ZonedTime> : std::formatter<string_view> {
    inline auto format(rdf4cpp::ZonedTime const &p, format_context &ctx) const {
        return std::format_to(ctx.out(), "{}{}", p.get_local_time(), p.get_time_zone().to_canonical_string());
    }
};

namespace rdf4cpp {
    inline std::ostream &operator<<(std::ostream &os, rdf4cpp::Year const &value) {
        std::format_to(std::ostream_iterator<char, char>{os}, "{}", value);
        return os;
    }
    inline std::ostream &operator<<(std::ostream &os, rdf4cpp::YearMonthDay const &value) {
        std::format_to(std::ostream_iterator<char, char>{os}, "{}", value);
        return os;
    }
    inline std::ostream &operator<<(std::ostream &os, rdf4cpp::YearMonth const &value) {
        std::format_to(std::ostream_iterator<char, char>{os}, "{}", value);
        return os;
    }
}
#endif

#endif  //RDF4CPP_TIMEZONE_HPP
