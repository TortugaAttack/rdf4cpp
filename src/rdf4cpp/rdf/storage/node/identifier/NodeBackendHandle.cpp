#include "NodeBackendHandle.hpp"

#include <rdf4cpp/rdf/storage/node/NodeStorage.hpp>

namespace rdf4cpp::rdf::storage::node::identifier {

template<typename out_type, typename in_type>
inline constexpr out_type &unsafe_cast(in_type &in) {
    static_assert(sizeof(in_type) == sizeof(out_type));
    return *reinterpret_cast<out_type *>(std::addressof(in));
}

template<typename out_type, typename in_type>
inline constexpr out_type const &unsafe_cast(in_type const &in) {
    static_assert(sizeof(in_type) == sizeof(out_type));
    return *reinterpret_cast<out_type const *>(std::addressof(in));
}

template<typename out_type, typename in_type>
inline constexpr out_type unsafe_copy_cast(in_type in) {
    static_assert(sizeof(in_type) == sizeof(out_type));
    return *reinterpret_cast<out_type *>(std::addressof(in));
}

struct __attribute__((__packed__)) NodeBackendHandleImpl {
    union __attribute__((__packed__)) {
        uint64_t raw_{};
        struct __attribute__((__packed__)) {
            NodeID node_id_;
            RDFNodeType type_ : 2;
            NodeStorageID::underlying_type storage_id_ : NodeStorageID::width;
            uint8_t free_tagging_bits : 4;

        } fields_;
    };

    constexpr explicit NodeBackendHandleImpl(size_t raw) noexcept : raw_{raw} {}

    constexpr NodeBackendHandleImpl(NodeID node_id, RDFNodeType node_type, NodeStorageID node_storage_id, uint8_t tagging_bits) noexcept
        : fields_{.node_id_ = node_id, .type_ = node_type, .storage_id_ = node_storage_id.value, .free_tagging_bits = tagging_bits} {}

    [[nodiscard]] constexpr NodeStorageID storage_id() const noexcept { return NodeStorageID{fields_.storage_id_}; }
    [[nodiscard]] constexpr NodeID node_id() const noexcept { return fields_.node_id_; }
};

static_assert(sizeof(NodeBackendHandleImpl) == 8);
static_assert(sizeof(NodeBackendHandle) == 8);

RDFNodeType NodeBackendHandle::type() const noexcept {
    return unsafe_cast<NodeBackendHandleImpl>(*this).fields_.type_;
}
bool NodeBackendHandle::is_iri() const noexcept {
    return type() == RDFNodeType::IRI;
}
bool NodeBackendHandle::is_literal() const noexcept {
    return type() == RDFNodeType::Literal;
}
bool NodeBackendHandle::is_blank_node() const noexcept {
    return type() == RDFNodeType::BNode;
}
bool NodeBackendHandle::is_variable() const noexcept {
    return type() == RDFNodeType::Variable;
}
bool NodeBackendHandle::empty() const noexcept {
    return unsafe_cast<NodeBackendHandleImpl>(*this).fields_.node_id_ == NodeID(0);
}

handle::IRIBackendView NodeBackendHandle::iri_backend() const noexcept {
    return NodeStorage::find_iri_backend_view(*this);
}
handle::LiteralBackendView NodeBackendHandle::literal_backend() const noexcept {
    return NodeStorage::find_literal_backend_view(*this);
}
handle::BNodeBackendView NodeBackendHandle::bnode_backend() const noexcept {
    return NodeStorage::find_bnode_backend_view(*this);
}
handle::VariableBackendView NodeBackendHandle::variable_backend() const noexcept {
    return NodeStorage::find_variable_backend_view(*this);
}

NodeID NodeBackendHandle::node_id() const noexcept {
    return unsafe_cast<NodeBackendHandleImpl>(*this).fields_.node_id_;
}
uint8_t NodeBackendHandle::free_tagging_bits() const noexcept {
    return unsafe_cast<NodeBackendHandleImpl>(*this).fields_.free_tagging_bits;
}
NodeStorageID NodeBackendHandle::node_storage_id() const noexcept {
    return unsafe_cast<NodeBackendHandleImpl>(*this).storage_id();
}
NodeBackendHandle::NodeBackendHandle(NodeID node_id, RDFNodeType node_type, NodeStorageID node_storage_id, uint8_t tagging_bits) noexcept
    : raw_(unsafe_copy_cast<uint64_t>(NodeBackendHandleImpl{node_id, node_type, node_storage_id, tagging_bits})) {}

}  // namespace rdf4cpp::rdf::storage::node::identifier