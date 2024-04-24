#include "Decimal.hpp"

#include <cmath>
#include <sstream>
#include <stdexcept>
#include <regex>

namespace rdf4cpp::datatypes::registry {

#ifndef DOXYGEN_PARSER
template<>
capabilities::Default<xsd_decimal>::cpp_type capabilities::Default<xsd_decimal>::from_string(std::string_view s) {
    // https://www.w3.org/TR/xmlschema11-2/#decimal
    static std::regex const decimal_regex{R"#((\+|-)?([0-9]+(\.[0-9]*)?|\.[0-9]+))#", std::regex_constants::optimize};

    if (!std::regex_match(s.begin(), s.end(), decimal_regex)) {
        throw std::runtime_error{"XSD Parsing Error"};
    }

    if (s.starts_with('+')) {
        s.remove_prefix(1);
    }

    return cpp_type{s};
}

template<>
bool capabilities::Default<xsd_decimal>::serialize_canonical_string(cpp_type const &value, writer::BufWriterParts writer) noexcept {
    auto const s = static_cast<std::string>(value);
    return writer::write_str(s, writer);
}

template<>
bool capabilities::Default<xsd_decimal>::serialize_simplified_string(cpp_type const &value, writer::BufWriterParts writer) noexcept {
    cpp_type v = value;
    v.normalize();
    if (v.get_exponent() == 0) {
        auto const s = static_cast<boost::multiprecision::cpp_int>(v).str();
        return writer::write_str(s, writer);
    } else {
        auto const s = static_cast<std::string>(v);
        return writer::write_str(s, writer);
    }
}

template<>
nonstd::expected<capabilities::Numeric<xsd_decimal>::add_result_cpp_type, DynamicError> capabilities::Numeric<xsd_decimal>::add(cpp_type const &lhs, cpp_type const &rhs) noexcept {
    // https://www.w3.org/TR/xpath-functions/#op.numeric
    // decimal needs overflow protection
    auto r = lhs.add_checked(rhs);
    if (r.has_value())
        return r.value();
    else
        return nonstd::make_unexpected(DynamicError::OverOrUnderFlow);
}

template<>
nonstd::expected<capabilities::Numeric<xsd_decimal>::sub_result_cpp_type, DynamicError> capabilities::Numeric<xsd_decimal>::sub(cpp_type const &lhs, cpp_type const &rhs) noexcept {
    // https://www.w3.org/TR/xpath-functions/#op.numeric
    // decimal needs overflow protection

    auto r = lhs.sub_checked(rhs);
    if (r.has_value())
        return r.value();
    else
        return nonstd::make_unexpected(DynamicError::OverOrUnderFlow);
}

template<>
nonstd::expected<capabilities::Numeric<xsd_decimal>::div_result_cpp_type, DynamicError> capabilities::Numeric<xsd_decimal>::div(cpp_type const &lhs, cpp_type const &rhs) noexcept {
    // https://www.w3.org/TR/xpath-functions/#func-numeric-divide
    // decimal needs error (and cpp_type is not integral) && overflow protection

    if (rhs == 0) {
        return nonstd::make_unexpected(DynamicError::DivideByZero);
    }

    auto r = lhs.div_checked(rhs, 1000);
    if (r.has_value())
        return r.value();
    else
        return nonstd::make_unexpected(DynamicError::OverOrUnderFlow);
}

template<>
nonstd::expected<capabilities::Numeric<xsd_decimal>::mul_result_cpp_type, DynamicError> capabilities::Numeric<xsd_decimal>::mul(cpp_type const &lhs, cpp_type const &rhs) noexcept {
    // https://www.w3.org/TR/xpath-functions/#op.numeric
    // decimal needs overflow protection

    auto r = lhs.mul_checked(rhs);
    if (r.has_value())
        return r.value();
    else
        return nonstd::make_unexpected(DynamicError::OverOrUnderFlow);
}

template<>
nonstd::expected<capabilities::Numeric<xsd_decimal>::abs_result_cpp_type, DynamicError> capabilities::Numeric<xsd_decimal>::neg(cpp_type const &operand) noexcept {
    auto r = operand.unary_minus_checked();
    if (r.has_value())
        return r.value();
    else
        return nonstd::make_unexpected(DynamicError::OverOrUnderFlow);
}

template<>
nonstd::expected<capabilities::Numeric<xsd_decimal>::abs_result_cpp_type, DynamicError> capabilities::Numeric<xsd_decimal>::abs(cpp_type const &operand) noexcept {
    auto r = operand.abs_checked();
    if (r.has_value())
        return r.value();
    else
        return nonstd::make_unexpected(DynamicError::OverOrUnderFlow);
}

template<>
nonstd::expected<capabilities::Numeric<xsd_decimal>::round_result_cpp_type, DynamicError> capabilities::Numeric<xsd_decimal>::round(cpp_type const &operand) noexcept {
    return operand.round(rdf4cpp::RoundingMode::Round);
}

template<>
nonstd::expected<capabilities::Numeric<xsd_decimal>::floor_result_cpp_type, DynamicError> capabilities::Numeric<xsd_decimal>::floor(cpp_type const &operand) noexcept {
    return operand.round(rdf4cpp::RoundingMode::Floor);
}

template<>
nonstd::expected<capabilities::Numeric<xsd_decimal>::ceil_result_cpp_type, DynamicError> capabilities::Numeric<xsd_decimal>::ceil(cpp_type const &operand) noexcept {
    return operand.round(rdf4cpp::RoundingMode::Ceil);
}

template<>
bool capabilities::Logical<xsd_decimal>::effective_boolean_value(cpp_type const &value) noexcept {
    return value != 0;
}

template<>
std::partial_ordering capabilities::Comparable<xsd_decimal>::compare(cpp_type const &lhs, cpp_type const &rhs) noexcept {
    return lhs <=> rhs;
}

struct __attribute__((__packed__)) InlinedDecimal {
    static constexpr size_t exponent_size = 10;
    static constexpr size_t unscaled_size = storage::identifier::LiteralID::width - 10;

    uint64_t unscaled_value : unscaled_size;
    uint32_t exponent : exponent_size;
    uint32_t padding : 64 - unscaled_size - exponent_size = 0;
};
static_assert(sizeof(InlinedDecimal) == sizeof(uint64_t));

template<>
std::optional<storage::identifier::LiteralID> capabilities::Inlineable<xsd_decimal>::try_into_inlined(cpp_type const &value) noexcept {
    auto big_unscaled_value = value.get_unscaled_value();
    auto exponent = value.get_exponent();

    cpp_type::normalize(big_unscaled_value, exponent);

    auto const unscaled_value = static_cast<int64_t>(big_unscaled_value);
    if (big_unscaled_value != unscaled_value) {
        // unscaled value > 64 bit, cannot fit
        return std::nullopt;
    }

    auto const unscaled_inlined = util::try_pack_integral<uint64_t, InlinedDecimal::unscaled_size>(unscaled_value);
    if (!unscaled_inlined.has_value()) {
        return std::nullopt;
    }

    auto const exponent_inlined = util::try_pack_integral<uint32_t, InlinedDecimal::exponent_size>(exponent);
    if (!exponent_inlined.has_value()) {
        return std::nullopt;
    }

    InlinedDecimal const inlined{*unscaled_inlined, *exponent_inlined};
    assert(inlined.padding == 0);
    return util::pack<storage::identifier::LiteralID>(inlined);
}

template<>
capabilities::Inlineable<xsd_decimal>::cpp_type capabilities::Inlineable<xsd_decimal>::from_inlined(storage::identifier::LiteralID inlined) noexcept {
    auto const data = util::unpack<InlinedDecimal>(inlined);
    assert(data.padding == 0);
    return cpp_type{data.unscaled_value, data.exponent};
}
#endif

template struct LiteralDatatypeImpl<xsd_decimal,
                                    capabilities::Logical,
                                    capabilities::Numeric,
                                    capabilities::Comparable,
                                    capabilities::Promotable,
                                    capabilities::FixedId,
                                    capabilities::Inlineable>;

}  // namespace rdf4cpp::datatypes::registry
