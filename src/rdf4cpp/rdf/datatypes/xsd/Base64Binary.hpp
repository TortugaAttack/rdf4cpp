#ifndef RDF4CPP_XSD_BASE64BINARY_HPP
#define RDF4CPP_XSD_BASE64BINARY_HPP

#include <rdf4cpp/rdf/datatypes/registry/DatatypeMapping.hpp>
#include <rdf4cpp/rdf/datatypes/registry/LiteralDatatypeImpl.hpp>

#include <cstddef>
#include <vector>

namespace rdf4cpp::rdf::datatypes::registry {

/**
 * Represents a decoded base64 value as a sequence of bytes.
 * Bytes are stored in the same order as the encoded hextets.
 */
struct Base64BinaryRepr : std::vector<std::byte> {
    /**
     * Constructs a Base64BinaryRepr from an encoded base64 value string
     *
     * @param base64encoded encoded base64 value
     * @return decoded base64 value
     */
    [[nodiscard]] static Base64BinaryRepr from_encoded(std::string_view base64encoded);

    /**
     * Encodes this base64 value into its string representation
     * @return encoded string representation
     */
    [[nodiscard]] std::string to_encoded() const noexcept;

    /**
     * @param n the index of the hextet to extract
     * @return the n-th hextet in this decoded base64 value
     */
    [[nodiscard]] constexpr std::byte hextet(size_t const n) const noexcept {
        auto const triple = n / 4 * 3;
        auto const off = (3 - (n % 4)) * 6;

        uint32_t const bytes = static_cast<uint32_t>((*this)[triple]) << 16
                               | (triple + 1 < this->size() ? static_cast<uint32_t>((*this)[triple + 1]) << 8 : 0)
                               | (triple + 2 < this->size() ? static_cast<uint32_t>((*this)[triple + 2]) : 0);

        return static_cast<std::byte>((bytes >> off) & 0b11'1111);
    }

    /**
     * @return the number of hextets in this decoded base64 value
     * @note count includes padding hextets
     */
    [[nodiscard]] constexpr size_t n_hextets() const noexcept {
        return 4 * (this->size() / 3 + (this->size() % 3 != 0));
    }

    std::strong_ordering operator<=>(Base64BinaryRepr const &) const noexcept = default;
};


inline constexpr util::ConstexprString xsd_base64_binary{"http://www.w3.org/2001/XMLSchema#base64Binary"};

template<>
struct DatatypeMapping<xsd_base64_binary> {
    using cpp_datatype = Base64BinaryRepr;
};

template<>
capabilities::Default<xsd_base64_binary>::cpp_type capabilities::Default<xsd_base64_binary>::from_string(std::string_view s);

template<>
std::string capabilities::Default<xsd_base64_binary>::to_string(cpp_type const &value) noexcept;

extern template struct LiteralDatatypeImpl<xsd_base64_binary>;

} // namespace rdf4cpp::rdf::datatypes::registry

namespace rdf4cpp::rdf::datatypes::xsd {

struct Base64Binary : registry::LiteralDatatypeImpl<registry::xsd_base64_binary> {};

} // namespace rdf4cpp::rdf::datatypes::xsd

namespace rdf4cpp::rdf::datatypes::registry::instantiation_detail {

[[maybe_unused]] inline xsd::Base64Binary const xsd_base64_binary_instance;

} // namespace rdf4cpp::rdf::datatypes::registry::instantiation_detail

#endif //RDF4CPP_XSD_BASE64BINARY_HPP
