#include <rdf4cpp/rdf/storage/node/reference_node_storage/detail/SpecializationDetail.hpp>
#include <rdf4cpp/rdf/storage/node/reference_node_storage/UnsyncReferenceNodeStorageBackend.hpp>

namespace rdf4cpp::rdf::storage::node::reference_node_storage {

UnsyncReferenceNodeStorageBackend::UnsyncReferenceNodeStorageBackend() noexcept {
    iri_storage_.mapping.reserve(identifier::NodeID::min_iri_id);

    // some iri's like xsd:string are there by default
    for (const auto &[iri, literal_type] : datatypes::registry::reserved_datatype_ids) {
        auto const id = literal_type.to_underlying();
        iri_storage_.mapping.insert_assume_not_present_at(view::IRIBackendView{.identifier = iri}, identifier::NodeID{id});
    }
}

size_t UnsyncReferenceNodeStorageBackend::size() const noexcept {
    return iri_storage_.mapping.size() +
           bnode_storage_.mapping.size() +
           variable_storage_.mapping.size() +
           fallback_literal_storage_.mapping.size() +
           specialization_detail::tuple_fold(specialized_literal_storage_, 0, [](auto acc, auto const &storage) noexcept {
               return acc + storage.mapping.size();
           });
}

bool UnsyncReferenceNodeStorageBackend::has_specialized_storage_for(identifier::LiteralType const datatype) const noexcept {
    static constexpr auto specialization_lut = specialization_detail::make_storage_specialization_lut<decltype(specialized_literal_storage_)>();
    return specialization_lut[datatype.to_underlying()];
}

/**
 * Unsynchronized lookup (and creation) of IDs by a provided view of a Node Backend.
 * @tparam create_if_not_present enables code for creating non-existing Node Backends
 * @param view contains the data of the requested Node Backend
 * @param storage the storage where the Node Backend is looked up
 * @return the NodeID for the looked up Node Backend. Result is the null-id if there was no matching Node Backend.
 */
template<bool create_if_not_present, typename Storage>
static identifier::NodeID lookup_or_insert_impl(typename Storage::BackendView const &view,
                                                Storage &storage) noexcept {

    if (auto const id = storage.mapping.lookup_id(view); id != typename Storage::BackendId{}) {
        return Storage::to_node_id(id, view);
    }

    if constexpr (!create_if_not_present) {
        return identifier::NodeID{};
    } else {
        auto const id = storage.mapping.insert_assume_not_present(view);
        return Storage::to_node_id(id, view);
    }
}

identifier::NodeID UnsyncReferenceNodeStorageBackend::find_or_make_id(view::LiteralBackendView const &view) noexcept {
    return view.visit(
            [this](view::LexicalFormLiteralBackendView const &lexical) noexcept {
                auto const datatype = identifier::iri_node_id_to_literal_type(lexical.datatype_id);
                assert(!this->has_specialized_storage_for(datatype));

                return lookup_or_insert_impl<true>(lexical, fallback_literal_storage_);
            },
            [this](view::ValueLiteralBackendView const &any) noexcept {
                assert(this->has_specialized_storage_for(any.datatype));

                return specialization_detail::visit_specialized(specialized_literal_storage_, any.datatype, [&](auto &storage) noexcept {
                    return lookup_or_insert_impl<true>(any, storage);
                });
            });
}

identifier::NodeID UnsyncReferenceNodeStorageBackend::find_or_make_id(view::IRIBackendView const &view) noexcept {
    return lookup_or_insert_impl<true>(view, iri_storage_);
}

identifier::NodeID UnsyncReferenceNodeStorageBackend::find_or_make_id(view::BNodeBackendView const &view) noexcept {
    return lookup_or_insert_impl<true>(view, bnode_storage_);
}

identifier::NodeID UnsyncReferenceNodeStorageBackend::find_or_make_id(view::VariableBackendView const &view) noexcept {
    return lookup_or_insert_impl<true>(view, variable_storage_);
}

identifier::NodeID UnsyncReferenceNodeStorageBackend::find_id(view::BNodeBackendView const &view) const noexcept {
    return lookup_or_insert_impl<false>(view, bnode_storage_);
}

identifier::NodeID UnsyncReferenceNodeStorageBackend::find_id(view::IRIBackendView const &view) const noexcept {
    return lookup_or_insert_impl<false>(view, iri_storage_);
}

identifier::NodeID UnsyncReferenceNodeStorageBackend::find_id(view::LiteralBackendView const &view) const noexcept {
    return view.visit(
            [this](view::LexicalFormLiteralBackendView const &lexical) {
                assert(!this->has_specialized_storage_for(identifier::iri_node_id_to_literal_type(lexical.datatype_id)));
                return lookup_or_insert_impl<false>(lexical, fallback_literal_storage_);
            },
            [this](view::ValueLiteralBackendView const &any) {
                return specialization_detail::visit_specialized(specialized_literal_storage_, any.datatype, [this, &any](auto const &storage) {
                    assert(this->has_specialized_storage_for(any.datatype));
                    (void) this;

                    return lookup_or_insert_impl<false>(any, storage);
                });
            });
}

identifier::NodeID UnsyncReferenceNodeStorageBackend::find_id(view::VariableBackendView const &view) const noexcept {
    return lookup_or_insert_impl<false>(view, variable_storage_);
}

template<typename NodeTypeStorage>
static typename NodeTypeStorage::BackendView find_backend_view(NodeTypeStorage &storage, identifier::NodeID const id) {
    auto const val_ptr = storage.mapping.lookup_value(NodeTypeStorage::to_backend_id(id));
    if (val_ptr == nullptr) {
        throw std::out_of_range{"Did not find node for given id"};
    }

    return static_cast<typename NodeTypeStorage::BackendView>(*val_ptr);
}

view::IRIBackendView UnsyncReferenceNodeStorageBackend::find_iri_backend_view(identifier::NodeID const id) const {
    return find_backend_view(iri_storage_, id);
}

view::LiteralBackendView UnsyncReferenceNodeStorageBackend::find_literal_backend_view(identifier::NodeID const id) const {
    if (id.literal_type().is_fixed() && this->has_specialized_storage_for(id.literal_type())) {
        return specialization_detail::visit_specialized(specialized_literal_storage_, id.literal_type(), [id](auto const &storage) {
            return find_backend_view(storage, id);
        });
    }

    return find_backend_view(fallback_literal_storage_, id);
}

view::BNodeBackendView UnsyncReferenceNodeStorageBackend::find_bnode_backend_view(identifier::NodeID const id) const {
    return find_backend_view(bnode_storage_, id);
}

view::VariableBackendView UnsyncReferenceNodeStorageBackend::find_variable_backend_view(identifier::NodeID const id) const {
    return find_backend_view(variable_storage_, id);
}

template<typename Storage>
static bool erase_impl(Storage &storage, identifier::NodeID const id) noexcept {
    auto const backend_id = Storage::to_backend_id(id);

    if (storage.mapping.lookup_value(backend_id) == nullptr) {
        return false;
    }

    storage.mapping.erase_assume_present(backend_id);
    return true;
}

bool UnsyncReferenceNodeStorageBackend::erase_iri(identifier::NodeID const id) noexcept {
    return erase_impl(iri_storage_, id);
}

bool UnsyncReferenceNodeStorageBackend::erase_literal(identifier::NodeID const id) noexcept {
    if (id.literal_type().is_fixed() && this->has_specialized_storage_for(id.literal_type())) {
        return specialization_detail::visit_specialized(specialized_literal_storage_, id.literal_type(), [id](auto &storage) noexcept {
            return erase_impl(storage, id);
        });
    }

    return erase_impl(fallback_literal_storage_, id);
}

bool UnsyncReferenceNodeStorageBackend::erase_bnode(identifier::NodeID const id) noexcept {
    return erase_impl(bnode_storage_, id);
}

bool UnsyncReferenceNodeStorageBackend::erase_variable(identifier::NodeID const id) noexcept {
    return erase_impl(variable_storage_, id);
}

}  // namespace rdf4cpp::rdf::storage::node::reference_node_storage