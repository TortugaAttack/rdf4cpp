#ifndef RDF4CPP_XSD_INTEGER_HPP
#define RDF4CPP_XSD_INTEGER_HPP

#include <rdf4cpp/rdf/datatypes/registry/DatatypeMapping.hpp>
#include <rdf4cpp/rdf/datatypes/registry/FixedIdMappings.hpp>
#include <rdf4cpp/rdf/datatypes/registry/LiteralDatatypeImpl.hpp>
#include <rdf4cpp/rdf/datatypes/xsd/Decimal.hpp>

#include <boost/multiprecision/cpp_int.hpp>

namespace rdf4cpp::rdf::datatypes::registry {

#ifndef DOXYGEN_PARSER
template<>
struct DatatypeMapping<xsd_integer> {
    using cpp_datatype = boost::multiprecision::cpp_int;
};

template<>
struct DatatypeSupertypeMapping<xsd_integer> {
    using supertype = xsd::Decimal;
};

template<>
struct DatatypeDivResultMapping<xsd_integer> {
    using op_result = xsd::Decimal;
};

template<>
capabilities::Default<xsd_integer>::cpp_type capabilities::Default<xsd_integer>::from_string(std::string_view s);

template<>
bool capabilities::Logical<xsd_integer>::effective_boolean_value(cpp_type const &value) noexcept;

template<>
nonstd::expected<capabilities::Numeric<xsd_integer>::div_result_cpp_type, DynamicError> capabilities::Numeric<xsd_integer>::div(cpp_type const &lhs, cpp_type const &rhs) noexcept;

template<>
nonstd::expected<capabilities::Numeric<xsd_integer>::abs_result_cpp_type, DynamicError> capabilities::Numeric<xsd_integer>::abs(cpp_type const &operand) noexcept;

template<>
nonstd::expected<capabilities::Numeric<xsd_integer>::round_result_cpp_type, DynamicError> capabilities::Numeric<xsd_integer>::round(cpp_type const &operand) noexcept;

template<>
nonstd::expected<capabilities::Numeric<xsd_integer>::floor_result_cpp_type, DynamicError> capabilities::Numeric<xsd_integer>::floor(cpp_type const &operand) noexcept;

template<>
nonstd::expected<capabilities::Numeric<xsd_integer>::ceil_result_cpp_type, DynamicError> capabilities::Numeric<xsd_integer>::ceil(cpp_type const &operand) noexcept;

template<>
std::partial_ordering capabilities::Comparable<xsd_integer>::compare(cpp_type const &lhs, cpp_type const &rhs) noexcept;

template<>
std::optional<storage::identifier::LiteralID> capabilities::Inlineable<xsd_integer>::try_into_inlined(cpp_type const &value) noexcept;

template<>
capabilities::Inlineable<xsd_integer>::cpp_type capabilities::Inlineable<xsd_integer>::from_inlined(storage::identifier::LiteralID inlined) noexcept;
#endif

extern template struct LiteralDatatypeImpl<xsd_integer,
                                           capabilities::Logical,
                                           capabilities::Numeric,
                                           capabilities::Comparable,
                                           capabilities::Subtype,
                                           capabilities::FixedId,
                                           capabilities::Inlineable>;

}  // namespace rdf4cpp::rdf::datatypes::registry


namespace rdf4cpp::rdf::datatypes::xsd {

struct Integer : registry::LiteralDatatypeImpl<registry::xsd_integer,
                                               registry::capabilities::Logical,
                                               registry::capabilities::Numeric,
                                               registry::capabilities::Comparable,
                                               registry::capabilities::Subtype,
                                               registry::capabilities::FixedId,
                                               registry::capabilities::Inlineable> {};

}  // namespace rdf4cpp::rdf::datatypes::xsd


namespace rdf4cpp::rdf::datatypes::registry::instantiation_detail {

[[maybe_unused]] inline xsd::Integer const xsd_intenger_instance;

} // namespace rdf4cpp::rdf::datatypes::registry::instantiation_detail

#endif  //RDF4CPP_XSD_INTEGER_HPP
