#ifndef RDF4CPP_BNODEBACKENDHANDLE_HPP
#define RDF4CPP_BNODEBACKENDHANDLE_HPP

#include <compare>
#include <string>
#include <string_view>

namespace rdf4cpp::rdf::util {
struct WeakNodeScope;
}

namespace rdf4cpp::rdf::storage::node::view {

struct BNodeBackendView {
    std::string_view identifier;
    rdf4cpp::rdf::util::WeakNodeScope const *scope;

    /**
     * N-Triples conform string representation.
     * @return N-Triples conform string representation.
     */
    [[nodiscard]] std::string n_string() const noexcept;
    std::strong_ordering operator<=>(BNodeBackendView const &) const noexcept = default;

    [[nodiscard]] size_t hash() const noexcept;
};
}  // namespace rdf4cpp::rdf::storage::node::view

template<>
struct std::hash<rdf4cpp::rdf::storage::node::view::BNodeBackendView> {
    size_t operator()(rdf4cpp::rdf::storage::node::view::BNodeBackendView const &x) const noexcept;
};

#endif  //RDF4CPP_BNODEBACKENDHANDLE_HPP
