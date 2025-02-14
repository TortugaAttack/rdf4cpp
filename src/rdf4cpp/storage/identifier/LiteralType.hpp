#ifndef RDF4CPP_LITERALTYPE_HPP
#define RDF4CPP_LITERALTYPE_HPP

#include <cassert>
#include <compare>
#include <cstddef>
#include <cstdint>
#include <rdf4cpp/TriBool.hpp>

namespace rdf4cpp::storage::identifier {

enum struct LiteralTypeTag : uint8_t {
    Default = 0b00,
    Numeric = 0b01,
    Chrono = 0b10,
};

/**
 * <p>A literal type specifies the type of a literal. Types, which are not available within this enum class MUST be specified as LiteralType::OTHER.</p>
 * <p>The purpose of this type is to provide a short-cut for common types like xsd:string or xsd:float when used within NodeID or NodeBackendHandle.
 * If a LiteralType instance is part of an identifier or view for a Literal stored in NodeStorage, it must not contradict the type information stored therein. </p>
 */
struct __attribute__((__packed__)) LiteralType {
    using underlying_type = uint8_t;
    static constexpr size_t width = 6;
    static constexpr size_t numeric_tagging_bit_shift = 4;

private:
    uint8_t underlying_: width;

public:
    constexpr LiteralType() noexcept = default;

    explicit constexpr LiteralType(underlying_type const underlying) noexcept : underlying_{underlying} {
        assert((underlying & 0b1100'0000) == 0);
    }

    static constexpr LiteralType other() noexcept {
        return LiteralType{0};
    }

    static constexpr LiteralType from_parts(LiteralTypeTag tag, underlying_type const type_id) noexcept {
        assert(tag != LiteralTypeTag::Default || type_id != 0);
        assert((type_id & 0b1111'0000) == 0);
        return LiteralType{static_cast<underlying_type>(type_id | (static_cast<underlying_type>(tag) << numeric_tagging_bit_shift))};
    }

    [[nodiscard]] constexpr bool is_fixed() const noexcept {
        return *this != LiteralType::other();
    }

    [[nodiscard]] constexpr TriBool is_numeric() const noexcept {
        if (!is_fixed()) {
            return TriBool::Err;
        }

        return static_cast<LiteralTypeTag>((underlying_ >> numeric_tagging_bit_shift) & 0b11) == LiteralTypeTag::Numeric;
    }

    [[nodiscard]] constexpr TriBool is_chrono() const noexcept {
        if (!is_fixed()) {
            return TriBool::Err;
        }

        return static_cast<LiteralTypeTag>((underlying_ >> numeric_tagging_bit_shift) & 0b11)  == LiteralTypeTag::Chrono;
    }

    [[nodiscard]] constexpr uint8_t type_id() const noexcept {
        return underlying_ & ~(0b11 << numeric_tagging_bit_shift);
    }

    [[nodiscard]] constexpr underlying_type to_underlying() const noexcept {
        return underlying_;
    }

    explicit constexpr operator underlying_type() const noexcept {
        return underlying_;
    }

    constexpr std::strong_ordering operator<=>(LiteralType const &other) const noexcept = default;
};

}  // namespace rdf4cpp::storage::identifier

#endif  //RDF4CPP_LITERALTYPE_HPP
