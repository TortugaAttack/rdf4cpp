#include <rdf4cpp/rdf/datatypes/xsd/time/Day.hpp>

#include <rdf4cpp/rdf/datatypes/registry/util/CharConvExt.hpp>

namespace rdf4cpp::rdf::datatypes::registry {

template<>
capabilities::Default<xsd_gDay>::cpp_type capabilities::Default<xsd_gDay>::from_string(std::string_view s) {
    auto tz = Timezone::try_parse(s);
    auto day = parse_date_time_fragment<std::chrono::day, unsigned int, '\0'>(tz.second);
    if (!day.ok())
        throw std::invalid_argument("invalid day");

    return std::make_pair(day, tz.first);
}

template<>
std::string capabilities::Default<xsd_gDay>::to_canonical_string(const cpp_type &value) noexcept {
    auto str = std::format("{:%d}", value.first);
    if (value.second.has_value())
        str += value.second->to_canonical_string();
    return str;
}

template<>
std::partial_ordering capabilities::Comparable<xsd_gDay>::compare(cpp_type const &lhs, cpp_type const &rhs) noexcept {
    return TimeComparer<std::chrono::day>::compare(lhs.first, lhs.second, rhs.first, rhs.second);
}

using IHelp = InliningHelper<std::chrono::day>;
static_assert(numberOfBits(31u) == 5);
static_assert(sizeof(std::chrono::day) == 1);
static_assert(sizeof(IHelp) * 8 < storage::node::identifier::LiteralID::width);

template<>
std::optional<storage::node::identifier::LiteralID> capabilities::Inlineable<xsd_gDay>::try_into_inlined(cpp_type const &value) noexcept {
    IHelp i{value.first, value.second};
    return util::pack<storage::node::identifier::LiteralID>(i);
}

template<>
capabilities::Inlineable<xsd_gDay>::cpp_type capabilities::Inlineable<xsd_gDay>::from_inlined(storage::node::identifier::LiteralID inlined) noexcept {
    auto i = util::unpack<IHelp>(inlined);
    return std::make_pair(i.time_value, i.decode_tz());
}

template struct LiteralDatatypeImpl<xsd_gDay,
                                    capabilities::Comparable,
                                    capabilities::FixedId,
                                    capabilities::Inlineable>;

template<>
TimePoint to_point_on_timeline<std::chrono::day>(std::chrono::day t) {
    return construct(TimePointReplacementDate.year() / TimePointReplacementDate.month() / t, TimePointReplacementTimeOfDay);
}

}  // namespace rdf4cpp::rdf::datatypes::registry