#ifndef RDF4CPP_TIME_HPP
#define RDF4CPP_TIME_HPP

#include <chrono>

#include <rdf4cpp/datatypes/registry/DatatypeMapping.hpp>
#include <rdf4cpp/datatypes/registry/FixedIdMappings.hpp>
#include <rdf4cpp/datatypes/registry/LiteralDatatypeImpl.hpp>
#include <rdf4cpp/Timezone.hpp>
#include <rdf4cpp/datatypes/xsd/time/DateTime.hpp>
#include <rdf4cpp/datatypes/xsd/time/DayTimeDuration.hpp>

namespace rdf4cpp::datatypes::registry {

#ifndef DOXYGEN_PARSER
template<>
struct DatatypeMapping<xsd_time> {
    using cpp_datatype = std::pair<std::chrono::nanoseconds, rdf4cpp::OptionalTimezone>;
};
template<>
struct DatatypeSupertypeMapping<xsd_time> {
    using supertype = xsd::DateTime;
};
template<>
struct DatatypeTimepointDurationOperandMapping<xsd_time> {
    using duration_type = xsd::DayTimeDuration;
};
template<>
struct DatatypeTimepointSubResultMapping<xsd_time> {
    using op_result = xsd::DayTimeDuration;
};


template<>
capabilities::Default<xsd_time>::cpp_type capabilities::Default<xsd_time>::from_string(std::string_view s);

template<>
bool capabilities::Default<xsd_time>::serialize_canonical_string(cpp_type const &value, writer::BufWriterParts writer) noexcept;

template<>
std::optional<storage::identifier::LiteralID> capabilities::Inlineable<xsd_time>::try_into_inlined(cpp_type const &value) noexcept;

template<>
capabilities::Inlineable<xsd_time>::cpp_type capabilities::Inlineable<xsd_time>::from_inlined(storage::identifier::LiteralID inlined) noexcept;

template<>
std::partial_ordering capabilities::Comparable<xsd_time>::compare(cpp_type const &lhs, cpp_type const &rhs) noexcept;

template<>
template<>
capabilities::Subtype<xsd_time>::super_cpp_type<0> capabilities::Subtype<xsd_time>::into_supertype<0>(cpp_type const &value) noexcept;

template<>
template<>
nonstd::expected<capabilities::Subtype<xsd_time>::cpp_type, DynamicError> capabilities::Subtype<xsd_time>::from_supertype<0>(super_cpp_type<0> const &value) noexcept;

template<>
nonstd::expected<capabilities::Timepoint<xsd_time>::timepoint_sub_result_cpp_type, DynamicError>
capabilities::Timepoint<xsd_time>::timepoint_sub(cpp_type const &lhs, cpp_type const &rhs) noexcept;

template<>
nonstd::expected<capabilities::Timepoint<xsd_time>::cpp_type, DynamicError>
capabilities::Timepoint<xsd_time>::timepoint_duration_add(cpp_type const &tp, timepoint_duration_operand_cpp_type const &dur) noexcept;

template<>
nonstd::expected<capabilities::Timepoint<xsd_time>::cpp_type, DynamicError>
capabilities::Timepoint<xsd_time>::timepoint_duration_sub(cpp_type const &tp, timepoint_duration_operand_cpp_type const &dur) noexcept;

#endif

extern template struct LiteralDatatypeImpl<xsd_time,
                                           capabilities::Timepoint,
                                           capabilities::Comparable,
                                           capabilities::FixedId,
                                           capabilities::Inlineable,
                                           capabilities::Subtype>;

}  // namespace rdf4cpp::datatypes::registry

namespace rdf4cpp::datatypes::xsd {

struct Time : registry::LiteralDatatypeImpl<registry::xsd_time,
                                            registry::capabilities::Timepoint,
                                            registry::capabilities::Comparable,
                                            registry::capabilities::FixedId,
                                            registry::capabilities::Inlineable,
                                            registry::capabilities::Subtype> {};

}  // namespace rdf4cpp::datatypes::xsd


namespace rdf4cpp::datatypes::registry::instantiation_detail {

[[maybe_unused]] inline xsd::Time const xsd_Time_instance;

}  // namespace rdf4cpp::datatypes::registry::instantiation_detail

#endif  //RDF4CPP_TIME_HPP
