#ifndef RDF4CPP_UNSYNCNODETYPESTORAGE_HPP
#define RDF4CPP_UNSYNCNODETYPESTORAGE_HPP

#include <dice/hash.hpp>
#include <dice/sparse-map/sparse_map.hpp>
#include <rdf4cpp/rdf/storage/node/identifier/NodeID.hpp>
#include <rdf4cpp/rdf/storage/node/reference_node_storage/detail/BiDirFlatMap.hpp>

#include <memory>

namespace rdf4cpp::rdf::storage::node::reference_node_storage {

/**
 * Storage for one of the Node Backend types. Includes bidirectional mappings between the Backend type and identifier::NodeID,
 * the mappings are not synchronized and are not intended to be thread-safe.
 * @tparam BackendType_t one of BNodeBackend, IRIBackend, FallbackLiteralBackend, SpecializedLiteralBackend and VariableBackend.
 */
template<typename BackendType_t>
struct UnsyncNodeTypeStorage {
    using backend_type = BackendType_t;
    using backend_view_type = typename backend_type::view_type;

private:
    struct DefaultBackendTypeEqual {
        using is_transparent = void;

        bool operator()(backend_view_type const &lhs, backend_type const &rhs) const noexcept {
            return lhs == static_cast<backend_view_type>(rhs);
        }

        bool operator()(backend_type const &lhs, backend_view_type const &rhs) const noexcept {
            return static_cast<backend_view_type>(lhs) == rhs;
        }
    };

    template<typename T>
    struct SelectBackendTypeEqual {
        using type = DefaultBackendTypeEqual;
    };

    template<typename T> requires requires { typename T::equal; }
    struct SelectBackendTypeEqual<T> {
        using type = typename T::equal;
    };

    struct DefaultBackendTypeHash {
        using is_transparent = void;

        [[nodiscard]] size_t operator()(backend_view_type const &x) const noexcept {
            return x.hash();
        }
    };

    template<typename T>
    struct SelectBackendTypeHash {
        using type = DefaultBackendTypeHash;
    };

    template<typename T> requires requires { typename T::hasher; }
    struct SelectBackendTypeHash<T> {
        using type = typename T::hasher;
    };

public:
    using backend_equal = typename SelectBackendTypeEqual<backend_type>::type;
    using backend_hasher = typename SelectBackendTypeHash<backend_type>::type;
    using backend_id_type = typename backend_type::id_type;

    /**
     * Translates the given BackenId into a NodeID
     */
    static identifier::NodeID to_node_id(backend_id_type const id, [[maybe_unused]] backend_view_type const &view) noexcept {
        if constexpr (requires { backend_type::to_node_id(id, view); }) {
            return backend_type::to_node_id(id, view);
        } else {
            return id;
        }
    }

    static backend_id_type to_backend_id(identifier::NodeID const id) noexcept {
        if constexpr (requires { backend_type::to_backend_id(id); }) {
            return backend_type::to_backend_id(id);
        } else {
            return id;
        }
    }

    detail::BiDirFlatMap<backend_id_type, backend_type, backend_view_type, backend_hasher, backend_equal> mapping;
};

}  // namespace rdf4cpp::rdf::storage::node::reference_node_storage

#endif  //RDF4CPP_UNSYNCNODETYPESTORAGE_HPP
