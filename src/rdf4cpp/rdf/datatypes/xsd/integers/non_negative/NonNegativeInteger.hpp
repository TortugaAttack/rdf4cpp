#ifndef RDF4CPP_XSD_NONNEGATIVEINTEGER_HPP
#define RDF4CPP_XSD_NONNEGATIVEINTEGER_HPP

#include <rdf4cpp/rdf/datatypes/registry/DatatypeMapping.hpp>
#include <rdf4cpp/rdf/datatypes/registry/LiteralDatatypeImpl.hpp>
#include <rdf4cpp/rdf/datatypes/xsd/integers/signed/Integer.hpp>

#include <stdexcept>

#include <boost/multiprecision/cpp_int.hpp>

namespace rdf4cpp::rdf::datatypes::registry {

template<>
struct DatatypeMapping<xsd_non_negative_integer> {
    using cpp_datatype = boost::multiprecision::cpp_int;
};

template<>
struct DatatypeSupertypeMapping<xsd_non_negative_integer> {
    using supertype = xsd::Integer;
};

template<>
struct DatatypeNumericStubMapping<xsd_non_negative_integer> {
    using numeric_impl_type = xsd::Integer;
};

template<>
inline capabilities::Default<xsd_non_negative_integer>::cpp_type capabilities::Default<xsd_non_negative_integer>::from_string(std::string_view s) {
    cpp_type ret;

    try {
        ret = cpp_type{s};
    } catch (std::runtime_error const &e) {
        throw std::runtime_error{std::string{"xsd:nonNegativeInteger parsing error: "} + e.what()};
    }

    if (ret < 0) {
        throw std::runtime_error{"xsd:nonNegativeInteger parsing error: found negative value"};
    }

    return ret;
}

template<>
inline std::string capabilities::Default<xsd_non_negative_integer>::to_string(cpp_type const &value) {
    return boost::multiprecision::to_string(value);
}

template<>
inline bool capabilities::Logical<xsd_non_negative_integer>::effective_boolean_value(cpp_type const &value) noexcept {
    return value != 0;
}

template<>
inline std::partial_ordering capabilities::Comparable<xsd_non_negative_integer>::compare(cpp_type const &lhs, cpp_type const &rhs) {
    if (lhs < rhs) {
        return std::partial_ordering::less;
    } else if (rhs < lhs) {
        return std::partial_ordering::greater;
    } else {
        return std::partial_ordering::equivalent;
    }
}

}  // namespace rdf4cpp::rdf::datatypes::registry


namespace rdf4cpp::rdf::datatypes::xsd {

struct NonNegativeInteger : registry::LiteralDatatypeImpl<registry::xsd_non_negative_integer,
                                                          registry::capabilities::Logical,
                                                          registry::capabilities::NumericStub,
                                                          registry::capabilities::Comparable,
                                                          registry::capabilities::Subtype,
                                                          registry::capabilities::FixedId> {
};

} // namespace rdf4cpp::rdf::datatypes::xsd

#endif  //RDF4CPP_XSD_NONNEGATIVEINTEGER_HPP
