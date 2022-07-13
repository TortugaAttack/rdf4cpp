/**
 * @file Registers xsd:int with DatatypeRegistry
 */

#ifndef RDF4CPP_XSD_INT_HPP
#define RDF4CPP_XSD_INT_HPP

#include <cstdint>
#include <ostream>
#include <charconv>

#include <rdf4cpp/rdf/datatypes/registry/DatatypeMapping.hpp>
#include <rdf4cpp/rdf/datatypes/registry/LiteralDatatypeImpl.hpp>

namespace rdf4cpp::rdf::datatypes::registry {
/*
 * Name of the datatype. This is kept so that we won't need to type it over and over again.
 */
constexpr static registry::ConstexprString xsd_int{"http://www.w3.org/2001/XMLSchema#int"};

/**
 * Defines the mapping between the LiteralDatatype IRI and the C++ datatype.
 */
template<>
struct DatatypeMapping<xsd_int> {
    using cpp_datatype = int32_t;
};

template<>
struct DatatypeSupertypeMapping<xsd_int> {
    using supertype = xsd::Integer;
};

/**
 * Specialisation of from_string template function.
 */
template<>
inline capabilities::Default<xsd_int>::cpp_type capabilities::Default<xsd_int>::from_string(std::string_view s) {
    if (s.starts_with('+')) {
        s.remove_prefix(1);
    }

    cpp_type value;
    auto const parse_res = std::from_chars(s.data(), s.data() + s.size(), value);

    if (parse_res.ptr != s.data() + s.size()) {
        throw std::runtime_error("XSD Parsing Error");
    } else {
        return value;
    }
}

template<>
inline bool capabilities::Logical<xsd_int>::effective_boolean_value(cpp_type const &value) {
    return value != 0;
}
}  // namespace rdf4cpp::rdf::datatypes::registry


namespace rdf4cpp::rdf::datatypes::xsd {
/**
 * Implementation of xsd::integer
 */
using Int = registry::LiteralDatatypeImpl<registry::xsd_int,
                                          registry::capabilities::Logical,
                                          registry::capabilities::Numeric,
                                          registry::capabilities::Subtype>;
}  // namespace rdf4cpp::rdf::datatypes::xsd

#endif  //RDF4CPP_XSD_INT_HPP
