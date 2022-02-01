#include "DefaultNodeStorageBackend.hpp"

#include <functional>

namespace rdf4cpp::rdf::storage::node::default_node_storage {

DefaultNodeStorageBackend::DefaultNodeStorageBackend() : INodeStorageBackend() {
    // TODO: that should be done by (Abstract)NodeContextBackend
    // some iri's like xsd:string are there by default
    for (const auto &[id, iri] : NodeID::predefined_iris) {
        auto [iter, inserted_successfully] = iri_storage_reverse.emplace(std::make_unique<IRIBackend>(iri), id);
        assert(inserted_successfully);
        iri_storage.insert({id, iter->first.get()});
    }
}

template<class Backend_t, bool create_if_not_present, class View_t, class Storage_t, class ReverseStorage_t, class NextIDFromView_func = void *>
inline identifier::NodeID lookup_or_insert_impl(View_t view, std::shared_mutex &mutex, Storage_t &storage,
                                                ReverseStorage_t &reverse_storage,
                                                NextIDFromView_func next_id_func = nullptr) noexcept {
    std::shared_lock<std::shared_mutex> shared_lock{mutex};
    auto found = reverse_storage.find(view);
    if (found == reverse_storage.end()) {
        if constexpr (create_if_not_present) {
            shared_lock.unlock();
            std::unique_lock<std::shared_mutex> unique_lock{mutex};
            // update found (might have changed in the meantime)
            found = reverse_storage.find(view);
            if (found == reverse_storage.end()) {
                identifier::NodeID id = next_id_func(view);
                auto [found2, inserted_successfully] = reverse_storage.emplace(std::make_unique<typename ReverseStorage_t::key_type::element_type>(view), id);
                assert(inserted_successfully);
                found = std::move(found2);
                storage.insert({id, found->first.get()});
                return id;
            } else {
                unique_lock.unlock();
                return found->second;
            }
        } else {
            return {};
        }
    } else {
        shared_lock.unlock();
        return found->second;
    }
}

identifier::NodeID DefaultNodeStorageBackend::find_or_make_id(handle::LiteralBackendView const &view) noexcept {
    return lookup_or_insert_impl<LiteralBackend, true>(
            view, literal_mutex_, literal_storage, literal_storage_reverse,
            [this]([[maybe_unused]] handle::LiteralBackendView const &literal_view) {
                // TODO: actually use LiteralType (therefore, we will need literal_view)
                return identifier::NodeID{next_literal_id++, identifier::LiteralType::OTHER};
            });
}

identifier::NodeID DefaultNodeStorageBackend::find_or_make_id(handle::IRIBackendView const &view) noexcept {
    return lookup_or_insert_impl<IRIBackend, true>(
            view, iri_mutex_, iri_storage, iri_storage_reverse,
            [this]([[maybe_unused]] handle::IRIBackendView const &view) {
                return next_iri_id++;
            });
}

identifier::NodeID DefaultNodeStorageBackend::find_or_make_id(handle::BNodeBackendView const &view) noexcept {
    return lookup_or_insert_impl<BNodeBackend, true>(
            view, bnode_mutex_, bnode_storage, bnode_storage_reverse,
            [this]([[maybe_unused]] handle::BNodeBackendView const &view) {
                return next_bnode_id++;
            });
}
identifier::NodeID DefaultNodeStorageBackend::find_or_make_id(handle::VariableBackendView const &view) noexcept {
    return lookup_or_insert_impl<VariableBackend, true>(
            view, variable_mutex_, variable_storage, variable_storage_reverse,
            [this]([[maybe_unused]] handle::VariableBackendView const &view) {
                return next_variable_id++;
            });
}

identifier::NodeID DefaultNodeStorageBackend::find_id(const handle::BNodeBackendView &view) const noexcept {
    return lookup_or_insert_impl<BNodeBackend, false>(
            view, bnode_mutex_, bnode_storage, bnode_storage_reverse);
}
identifier::NodeID DefaultNodeStorageBackend::find_id(const handle::IRIBackendView &view) const noexcept {
    return lookup_or_insert_impl<IRIBackend, false>(
            view, iri_mutex_, iri_storage, iri_storage_reverse);
}
identifier::NodeID DefaultNodeStorageBackend::find_id(const handle::LiteralBackendView &view) const noexcept {
    return lookup_or_insert_impl<LiteralBackend, false>(
            view, literal_mutex_, literal_storage, literal_storage_reverse);
}
identifier::NodeID DefaultNodeStorageBackend::find_id(const handle::VariableBackendView &view) const noexcept {
    return lookup_or_insert_impl<VariableBackend, false>(
            view, variable_mutex_, variable_storage, variable_storage_reverse);
}

handle::IRIBackendView DefaultNodeStorageBackend::find_iri_backend_view(identifier::NodeID id) const {
    std::shared_lock<std::shared_mutex> shared_lock{iri_mutex_};
    return handle::IRIBackendView(*iri_storage.at(id));
}
handle::LiteralBackendView DefaultNodeStorageBackend::find_literal_backend_view(identifier::NodeID id) const {
    std::shared_lock<std::shared_mutex> shared_lock{literal_mutex_};
    return handle::LiteralBackendView(*literal_storage.at(id));
}
handle::BNodeBackendView DefaultNodeStorageBackend::find_bnode_backend_view(identifier::NodeID id) const {
    std::shared_lock<std::shared_mutex> shared_lock{bnode_mutex_};
    return handle::BNodeBackendView(*bnode_storage.at(id));
}
handle::VariableBackendView DefaultNodeStorageBackend::find_variable_backend_view(identifier::NodeID id) const {
    std::shared_lock<std::shared_mutex> shared_lock{variable_mutex_};
    return handle::VariableBackendView(*variable_storage.at(id));
}
bool DefaultNodeStorageBackend::erase_iri([[maybe_unused]] identifier::NodeID id) const {
    throw std::runtime_error("Deleting nodes is not implemented in DefaultNodeStorageBackend.");
}
bool DefaultNodeStorageBackend::erase_literal([[maybe_unused]] identifier::NodeID id) const {
    throw std::runtime_error("Deleting nodes is not implemented in DefaultNodeStorageBackend.");
}
bool DefaultNodeStorageBackend::erase_bnode([[maybe_unused]] identifier::NodeID id) const {
    throw std::runtime_error("Deleting nodes is not implemented in DefaultNodeStorageBackend.");
}
bool DefaultNodeStorageBackend::erase_variable([[maybe_unused]] identifier::NodeID id) const {
    throw std::runtime_error("Deleting nodes is not implemented in DefaultNodeStorageBackend.");
}
}  // namespace rdf4cpp::rdf::storage::node::default_node_storage