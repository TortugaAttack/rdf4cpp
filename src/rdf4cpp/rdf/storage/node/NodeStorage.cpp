#include "NodeStorage.hpp"

namespace rdf4cpp::rdf::storage::node {
NodeStorage NodeStorage::default_instance_ = {};
INodeStorageBackend *NodeStorage::lookup_backend_instance(NodeStorageID id) {
    std::call_once(default_init_once_flag, []() {
        if (node_context_instances[0] == nullptr) {
            default_instance_ = new_instance();
            default_node_context_id = default_instance_.id();
            node_context_instances[0] = default_instance_.backend_;
        } else {
            default_instance_ = NodeStorage(node_context_instances[0]);
            default_node_context_id = node_context_instances[0]->manager_id;
        }
    });
    return node_context_instances[id.value];
}
NodeStorage &NodeStorage::primary_instance() {
    std::call_once(default_init_once_flag, []() {
        if (node_context_instances[0] == nullptr) {
            default_instance_ = new_instance();
            default_node_context_id = default_instance_.id();
            node_context_instances[0] = default_instance_.backend_;
        } else {
            default_instance_ = NodeStorage(node_context_instances[0]);
            default_node_context_id = node_context_instances[0]->manager_id;
        }
    });
    return default_instance_;
}
void NodeStorage::primary_instance(const NodeStorage &node_context) {
    default_instance_ = node_context;
    default_node_context_id = node_context.id();
}
NodeStorage NodeStorage::new_instance() {
    return NodeStorage(new DefaultNodeStorageBackend());
}
NodeStorage NodeStorage::register_backend(INodeStorageBackend *backend_instance) {
    if (backend_instance == nullptr)
        throw std::runtime_error("Backend instance must not be null.");

    auto &stored_instance = node_context_instances[backend_instance->manager_id.value];

    if (stored_instance != backend_instance and stored_instance != nullptr)
        throw std::runtime_error("A NodeStorage with manager_id " + std::to_string(backend_instance->manager_id.value) + " is already registered.");

    node_context_instances[backend_instance->manager_id.value] = backend_instance;
    return NodeStorage(backend_instance);
}
void NodeStorage::unregister_backend(INodeStorageBackend *backend_instance) {
    if (backend_instance == nullptr)
        throw std::runtime_error("Backend instance must not be null.");
    node_context_instances[backend_instance->manager_id.value] = nullptr;
}
std::optional<NodeStorage> NodeStorage::lookup_instance(NodeStorageID id) {
    // TODO: would be better if we returned a reference here, i.e. std::optional<std::reference_wrapper<NodeStorage>>
    INodeStorageBackend *backend = lookup_backend_instance(id);
    if (backend != nullptr) {
        backend->inc_use_count();
        return NodeStorage(backend);
    } else {
        return std::nullopt;
    }
}
NodeStorage::~NodeStorage() {
    if (backend_ != nullptr) backend_->dec_use_count();
}
NodeStorage::NodeStorage(NodeStorage &&other) noexcept {
    if (this->backend_ != other.backend_) {
        if (this->backend_ != nullptr)
            this->backend_->dec_use_count();
        this->backend_ = other.backend_;
        if (this->backend_ != nullptr)
            this->backend_->inc_use_count();
    } else {
        if (other.backend_ != nullptr) {
            other.backend_->dec_use_count();
        }
    }

    other.backend_ = nullptr;
}
NodeStorage::NodeStorage(const NodeStorage &node_context) noexcept : backend_(node_context.backend_) {
    if (this->backend_ != nullptr) node_context.backend_->inc_use_count();
}
NodeStorage &NodeStorage::operator=(const NodeStorage &other) noexcept {
    if (this == &other)
        return *this;
    else if (this->backend_ != other.backend_) {
        if (this->backend_ != nullptr)
            this->backend_->dec_use_count();
        this->backend_ = other.backend_;
        if (this->backend_ != nullptr)
            this->backend_->inc_use_count();
    }
    return *this;
}
NodeStorage &NodeStorage::operator=(NodeStorage &&other) noexcept {
    if (this->backend_ != other.backend_) {
        if (this->backend_ != nullptr)
            this->backend_->dec_use_count();
        this->backend_ = other.backend_;
        if (this->backend_ != nullptr)
            this->backend_->inc_use_count();
    } else {
        if (other.backend_ != nullptr) {
            other.backend_->dec_use_count();
        }
    }

    other.backend_ = nullptr;
    return *this;
}
size_t NodeStorage::use_count() const noexcept {
    return backend_->use_count_;
}
size_t NodeStorage::nodes_in_use() const noexcept {
    return backend_->nodes_in_use_;
}
NodeStorageID NodeStorage::id() const noexcept {
    return backend_->manager_id;
}
std::pair<LiteralBackend *, NodeID> NodeStorage::get_string_literal(std::string_view lexical_form) {
    return backend_->get_string_literal(lexical_form);
}
std::pair<LiteralBackend *, NodeID> NodeStorage::get_typed_literal(std::string_view lexical_form, std::string_view datatype) {
    return backend_->get_typed_literal(lexical_form, datatype);
}
std::pair<LiteralBackend *, NodeID> NodeStorage::get_typed_literal(std::string_view lexical_form, const NodeID &datatype_id) {
    return backend_->get_typed_literal(lexical_form, datatype_id);
}
std::pair<LiteralBackend *, NodeID> NodeStorage::get_lang_literal(std::string_view lexical_form, std::string_view lang) {
    return backend_->get_lang_literal(lexical_form, lang);
}
std::pair<IRIBackend *, NodeID> NodeStorage::get_iri(std::string_view iri) {
    return backend_->get_iri(iri);
}
std::pair<VariableBackend *, NodeID> NodeStorage::get_variable(std::string_view identifier, bool anonymous) {
    return backend_->get_variable(identifier, anonymous);
}
std::pair<BNodeBackend *, NodeID> NodeStorage::get_bnode(std::string_view identifier) {
    return backend_->get_bnode(identifier);
}
IRIBackend *NodeStorage::lookup_iri(NodeID id) {
    INodeStorageBackend *backend = NodeStorage::lookup_backend_instance(id.manager_id());
    // TODO: check against nullptr
    return backend->lookup_iri(id.node_id());
}
LiteralBackend *NodeStorage::lookup_literal(NodeID id) {
    INodeStorageBackend *backend = NodeStorage::lookup_backend_instance(id.manager_id());
    // TODO: check against nullptr
    return backend->lookup_literal(id.node_id());
}
BNodeBackend *NodeStorage::lookup_bnode(NodeID id) {
    INodeStorageBackend *backend = NodeStorage::lookup_backend_instance(id.manager_id());
    // TODO: check against nullptr
    return backend->lookup_bnode(id.node_id());
}
VariableBackend *NodeStorage::lookup_variable(NodeID id) {
    INodeStorageBackend *backend = NodeStorage::lookup_backend_instance(id.manager_id());
    // TODO: check against nullptr
    return backend->lookup_variable(id.node_id());
}
}  // namespace rdf4cpp::rdf::storage::node