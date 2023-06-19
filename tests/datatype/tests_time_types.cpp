#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Woverloaded-shift-op-parentheses"
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include <doctest/doctest.h>
#include <format>
#include <rdf4cpp/rdf.hpp>

template<class Datatype>
void basic_test(typename Datatype::cpp_type a, std::string_view b, std::partial_ordering res, bool skip_string_comp = false) {
    using namespace rdf4cpp::rdf;
    // better output on failure than CHECK(a <=> b == res)
    if (res == std::partial_ordering::equivalent) {
        const auto lit_a = Literal::make_typed_from_value<Datatype>(a);
        const auto lit_b = Literal::make_typed<Datatype>(b);
        if (!skip_string_comp)
            CHECK(lit_a.lexical_form() == b);
        CHECK(lit_a == lit_b);
        CHECK(a == lit_b.template value<Datatype>());
    } else if (res == std::partial_ordering::less) {
        CHECK(Literal::make_typed_from_value<Datatype>(a) < Literal::make_typed<Datatype>(b));
    }
    else if (res == std::partial_ordering::greater) {
        CHECK(Literal::make_typed_from_value<Datatype>(a) > Literal::make_typed<Datatype>(b));
    }
}

TEST_CASE("timezone") {
    using namespace rdf4cpp::rdf::datatypes::registry;
    Timezone tz{};
    tz.offset = std::chrono::minutes{60};
    Timezone zero_tz{};
    auto d = std::chrono::sys_days{std::chrono::day{1} / 2 / 2042} + std::chrono::hours{5} + std::chrono::minutes{30} + std::chrono::seconds{15};
    std::chrono::zoned_time d_in0{&zero_tz, d};
    std::chrono::zoned_time d_in60{&tz, d_in0};
    CHECK(std::format("{:%Y-%m-%d-%H-%M-%S-%z}", d_in60) == "2042-02-01-06-30-15-+0100");
}

TEST_CASE("datatype gYear") {
    using namespace rdf4cpp::rdf;

    CHECK(std::string(datatypes::xsd::GYear::identifier) == "http://www.w3.org/2001/XMLSchema#gYear");

    rdf4cpp::rdf::datatypes::registry::OptionalTimezone tz = std::nullopt;
    basic_test<datatypes::xsd::GYear>(std::make_pair(std::chrono::year{500}, tz), "0500", std::partial_ordering::equivalent);
    basic_test<datatypes::xsd::GYear>(std::make_pair(std::chrono::year{500}, tz), "0501", std::partial_ordering::less);
    basic_test<datatypes::xsd::GYear>(std::make_pair(std::chrono::year{500}, datatypes::registry::Timezone{std::chrono::hours{1}}), "0500+1:00", std::partial_ordering::equivalent);
    basic_test<datatypes::xsd::GYear>(std::make_pair(std::chrono::year{500}, datatypes::registry::Timezone{std::chrono::minutes{-65}}), "0500-1:05", std::partial_ordering::equivalent);
}

TEST_CASE("datatype gMonth") {
    using namespace rdf4cpp::rdf;

    CHECK(std::string(datatypes::xsd::GMonth::identifier) == "http://www.w3.org/2001/XMLSchema#gMonth");

    rdf4cpp::rdf::datatypes::registry::OptionalTimezone tz = std::nullopt;
    basic_test<datatypes::xsd::GMonth>(std::make_pair(std::chrono::May, tz), "05", std::partial_ordering::equivalent);
    basic_test<datatypes::xsd::GMonth>(std::make_pair(std::chrono::April, tz), "05", std::partial_ordering::less);
    basic_test<datatypes::xsd::GMonth>(std::make_pair(std::chrono::May, datatypes::registry::Timezone{std::chrono::hours{1}}), "05+1:00", std::partial_ordering::equivalent);
    basic_test<datatypes::xsd::GMonth>(std::make_pair(std::chrono::May, datatypes::registry::Timezone{std::chrono::minutes{-65}}), "05-1:05", std::partial_ordering::equivalent);
}

TEST_CASE("datatype gDay") {
    using namespace rdf4cpp::rdf;

    CHECK(std::string(datatypes::xsd::GDay::identifier) == "http://www.w3.org/2001/XMLSchema#gDay");

    rdf4cpp::rdf::datatypes::registry::OptionalTimezone tz = std::nullopt;
    basic_test<datatypes::xsd::GDay>(std::make_pair(std::chrono::day{5}, tz), "05", std::partial_ordering::equivalent);
    basic_test<datatypes::xsd::GDay>(std::make_pair(std::chrono::day{4}, tz), "05", std::partial_ordering::less);
    basic_test<datatypes::xsd::GDay>(std::make_pair(std::chrono::day{5}, datatypes::registry::Timezone{std::chrono::hours{1}}), "05+1:00", std::partial_ordering::equivalent);
    basic_test<datatypes::xsd::GDay>(std::make_pair(std::chrono::day{5}, datatypes::registry::Timezone{std::chrono::minutes{-65}}), "05-1:05", std::partial_ordering::equivalent);
}

TEST_CASE("datatype gYearMonth") {
    using namespace rdf4cpp::rdf;

    CHECK(std::string(datatypes::xsd::GYearMonth::identifier) == "http://www.w3.org/2001/XMLSchema#gYearMonth");

    rdf4cpp::rdf::datatypes::registry::OptionalTimezone tz = std::nullopt;
    basic_test<datatypes::xsd::GYearMonth>(std::make_pair(std::chrono::year{2042} / 5, tz), "2042-05", std::partial_ordering::equivalent);
    basic_test<datatypes::xsd::GYearMonth>(std::make_pair(std::chrono::year{2042} / 4, tz), "2042-05", std::partial_ordering::less);
    basic_test<datatypes::xsd::GYearMonth>(std::make_pair(std::chrono::year{2041} / 6, tz), "2042-05", std::partial_ordering::less);
    basic_test<datatypes::xsd::GYearMonth>(std::make_pair(std::chrono::year{2042} / 5, datatypes::registry::Timezone{std::chrono::hours{1}}), "2042-05+1:00", std::partial_ordering::equivalent);
    basic_test<datatypes::xsd::GYearMonth>(std::make_pair(std::chrono::year{2042} / 5, datatypes::registry::Timezone{std::chrono::minutes{-65}}), "2042-05-1:05", std::partial_ordering::equivalent);
}

TEST_CASE("datatype gMonthDay") {
    using namespace rdf4cpp::rdf;

    CHECK(std::string(datatypes::xsd::GMonthDay::identifier) == "http://www.w3.org/2001/XMLSchema#gMonthDay");

    rdf4cpp::rdf::datatypes::registry::OptionalTimezone tz = std::nullopt;
    basic_test<datatypes::xsd::GMonthDay>(std::make_pair(std::chrono::month{5} / 5, tz), "05-05", std::partial_ordering::equivalent);
    basic_test<datatypes::xsd::GMonthDay>(std::make_pair(std::chrono::month{5} / 4, tz), "05-05", std::partial_ordering::less);
    basic_test<datatypes::xsd::GMonthDay>(std::make_pair(std::chrono::month{3} / 6, tz), "04-05", std::partial_ordering::less);
    basic_test<datatypes::xsd::GMonthDay>(std::make_pair(std::chrono::month{5} / 5, datatypes::registry::Timezone{std::chrono::hours{1}}), "05-05+1:00", std::partial_ordering::equivalent);
    basic_test<datatypes::xsd::GMonthDay>(std::make_pair(std::chrono::month{5} / 5, datatypes::registry::Timezone{std::chrono::minutes{-65}}), "05-05-1:05", std::partial_ordering::equivalent);
}

TEST_CASE("datatype date") {
    using namespace rdf4cpp::rdf;

    CHECK(std::string(datatypes::xsd::Date::identifier) == "http://www.w3.org/2001/XMLSchema#date");

    rdf4cpp::rdf::datatypes::registry::OptionalTimezone tz = std::nullopt;
    basic_test<datatypes::xsd::Date>(std::make_pair(std::chrono::year{2042} / 5 / 1, tz), "2042-05-01", std::partial_ordering::equivalent);
    basic_test<datatypes::xsd::Date>(std::make_pair(std::chrono::year{2042} / 4 / 1, tz), "2042-05-01", std::partial_ordering::less);
    basic_test<datatypes::xsd::Date>(std::make_pair(std::chrono::year{2041} / 6 / 1, tz), "2042-05-01", std::partial_ordering::less);
    basic_test<datatypes::xsd::Date>(std::make_pair(std::chrono::year{2042} / 5 / 1, datatypes::registry::Timezone{std::chrono::hours{1}}), "2042-05-01+1:00", std::partial_ordering::equivalent);
    basic_test<datatypes::xsd::Date>(std::make_pair(std::chrono::year{2042} / 5 / 1, datatypes::registry::Timezone{std::chrono::minutes{-65}}), "2042-05-01-1:05", std::partial_ordering::equivalent);
}

TEST_CASE("datatype time") {
    using namespace rdf4cpp::rdf;

    CHECK(std::string(datatypes::xsd::Time::identifier) == "http://www.w3.org/2001/XMLSchema#time");

    rdf4cpp::rdf::datatypes::registry::OptionalTimezone tz = std::nullopt;
    basic_test<datatypes::xsd::Time>(std::make_pair(std::chrono::minutes{50}, tz), "00:50:00.000", std::partial_ordering::equivalent);
    basic_test<datatypes::xsd::Time>(std::make_pair(std::chrono::hours{12} + std::chrono::minutes{34} + std::chrono::seconds{56} + std::chrono::milliseconds{789}, tz), "12:34:56.789", std::partial_ordering::equivalent);
    basic_test<datatypes::xsd::Time>(std::make_pair(std::chrono::minutes{50} + std::chrono::milliseconds{100}, tz), "00:50:00.1", std::partial_ordering::equivalent, true);
    basic_test<datatypes::xsd::Time>(std::make_pair(std::chrono::minutes{50} + std::chrono::milliseconds{123}, tz), "00:50:00.12345", std::partial_ordering::equivalent, true);
    basic_test<datatypes::xsd::Time>(std::make_pair(std::chrono::minutes{42}, tz), "00:50:00.000", std::partial_ordering::less);
    basic_test<datatypes::xsd::Time>(std::make_pair(std::chrono::minutes{50}, datatypes::registry::Timezone{std::chrono::hours{1}}), "00:50:00.000+1:00", std::partial_ordering::equivalent);
    basic_test<datatypes::xsd::Time>(std::make_pair(std::chrono::minutes{50}, datatypes::registry::Timezone{std::chrono::minutes{-65}}), "00:50:00.000-1:05", std::partial_ordering::equivalent);
}

TEST_CASE("datatype dateTime") {
    using namespace rdf4cpp::rdf;

    CHECK(std::string(datatypes::xsd::DateTime::identifier) == "http://www.w3.org/2001/XMLSchema#dateTime");

    rdf4cpp::rdf::datatypes::registry::OptionalTimezone tz = std::nullopt;
    basic_test<datatypes::xsd::DateTime>(std::make_pair(datatypes::registry::DateTime{std::chrono::year{2042} / 5 / 1, std::chrono::minutes{50}}, tz), "2042-05-01T00:50:00.000", std::partial_ordering::equivalent);
    basic_test<datatypes::xsd::DateTime>(std::make_pair(datatypes::registry::DateTime{std::chrono::year{2042} / 5 / 1, std::chrono::hours{12} + std::chrono::minutes{34} + std::chrono::seconds{56} + std::chrono::milliseconds{789}}, tz), "2042-05-01T12:34:56.789", std::partial_ordering::equivalent);
    basic_test<datatypes::xsd::DateTime>(std::make_pair(datatypes::registry::DateTime{std::chrono::year{2042} / 5 / 1, std::chrono::minutes{50} + std::chrono::milliseconds{100}}, tz), "2042-05-01T00:50:00.1", std::partial_ordering::equivalent, true);
    basic_test<datatypes::xsd::DateTime>(std::make_pair(datatypes::registry::DateTime{std::chrono::year{2042} / 5 / 1, std::chrono::minutes{50} + std::chrono::milliseconds{123}}, tz), "2042-05-01T00:50:00.12345", std::partial_ordering::equivalent, true);
    basic_test<datatypes::xsd::DateTime>(std::make_pair(datatypes::registry::DateTime{std::chrono::year{2042} / 5 / 1, std::chrono::minutes{42}}, tz), "2042-05-01T00:50:00.000", std::partial_ordering::less);
    basic_test<datatypes::xsd::DateTime>(std::make_pair(datatypes::registry::DateTime{std::chrono::year{2042} / 5 / 1, std::chrono::minutes{50}}, datatypes::registry::Timezone{std::chrono::hours{1}}), "2042-05-01T00:50:00.000+1:00", std::partial_ordering::equivalent);
    basic_test<datatypes::xsd::DateTime>(std::make_pair(datatypes::registry::DateTime{std::chrono::year{2042} / 5 / 1, std::chrono::minutes{50}}, datatypes::registry::Timezone{std::chrono::minutes{-65}}), "2042-05-01T00:50:00.000-1:05", std::partial_ordering::equivalent);
}

TEST_CASE("datatype dateTimeStamp") {
    using namespace rdf4cpp::rdf;

    CHECK(std::string(datatypes::xsd::DateTimeStamp::identifier) == "http://www.w3.org/2001/XMLSchema#dateTimeStamp");

    datatypes::registry::Timezone tz{std::chrono::hours{0}};
    basic_test<datatypes::xsd::DateTimeStamp>(std::make_pair(datatypes::registry::DateTime{std::chrono::year{2042} / 5 / 1, std::chrono::minutes{50}}, tz), "2042-05-01T00:50:00.000Z", std::partial_ordering::equivalent);
    basic_test<datatypes::xsd::DateTimeStamp>(std::make_pair(datatypes::registry::DateTime{std::chrono::year{2042} / 5 / 1, std::chrono::hours{12} + std::chrono::minutes{34} + std::chrono::seconds{56} + std::chrono::milliseconds{789}}, tz), "2042-05-01T12:34:56.789Z", std::partial_ordering::equivalent);
    basic_test<datatypes::xsd::DateTimeStamp>(std::make_pair(datatypes::registry::DateTime{std::chrono::year{2042} / 5 / 1, std::chrono::minutes{50} + std::chrono::milliseconds{100}}, tz), "2042-05-01T00:50:00.1Z", std::partial_ordering::equivalent, true);
    basic_test<datatypes::xsd::DateTimeStamp>(std::make_pair(datatypes::registry::DateTime{std::chrono::year{2042} / 5 / 1, std::chrono::minutes{50} + std::chrono::milliseconds{123}}, tz), "2042-05-01T00:50:00.12345Z", std::partial_ordering::equivalent, true);
    basic_test<datatypes::xsd::DateTimeStamp>(std::make_pair(datatypes::registry::DateTime{std::chrono::year{2042} / 5 / 1, std::chrono::minutes{42}}, tz), "2042-05-01T00:50:00.000Z", std::partial_ordering::less);
    basic_test<datatypes::xsd::DateTimeStamp>(std::make_pair(datatypes::registry::DateTime{std::chrono::year{2042} / 5 / 1, std::chrono::minutes{50}}, datatypes::registry::Timezone{std::chrono::hours{1}}), "2042-05-01T00:50:00.000+1:00", std::partial_ordering::equivalent);
    basic_test<datatypes::xsd::DateTimeStamp>(std::make_pair(datatypes::registry::DateTime{std::chrono::year{2042} / 5 / 1, std::chrono::minutes{50}}, datatypes::registry::Timezone{std::chrono::minutes{-65}}), "2042-05-01T00:50:00.000-1:05", std::partial_ordering::equivalent);
}

#pragma clang diagnostic pop