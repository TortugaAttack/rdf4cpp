#include <rdf4cpp/rdf/datatypes/xsd/integers/non_negative/PositiveInteger.hpp>

#include <stdexcept>

namespace rdf4cpp::rdf::datatypes::registry {

template<>
capabilities::Default<xsd_positive_integer>::cpp_type capabilities::Default<xsd_positive_integer>::from_string(std::string_view s) {
    cpp_type ret;

    try {
        ret = cpp_type{s};
    } catch (std::runtime_error const &e) {
        throw std::runtime_error{std::string{"xsd:positiveInteger parsing error: "} + e.what()};
    }

    if (ret < 1) {
        throw std::runtime_error{"xsd:positiveInteger parsing error: found non-positive value"};
    }

    return ret;
}

template<>
bool capabilities::Logical<xsd_positive_integer>::effective_boolean_value(cpp_type const &) noexcept {
    return true;
}

template<>
std::partial_ordering capabilities::Comparable<xsd_positive_integer>::compare(cpp_type const &lhs, cpp_type const &rhs) {
    if (lhs < rhs) {
        return std::partial_ordering::less;
    } else if (rhs < lhs) {
        return std::partial_ordering::greater;
    } else {
        return std::partial_ordering::equivalent;
    }
}

template struct LiteralDatatypeImpl<xsd_positive_integer,
                                    capabilities::Logical,
                                    capabilities::NumericStub,
                                    capabilities::Comparable,
                                    capabilities::Subtype,
                                    capabilities::FixedId>;

}  // namespace rdf4cpp::rdf::datatypes::registry
