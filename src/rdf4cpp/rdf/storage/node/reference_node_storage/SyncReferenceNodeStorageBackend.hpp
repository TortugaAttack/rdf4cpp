#ifndef RDF4CPP_SYNCREFERENCENODESTORAGEBACKEND_HPP
#define RDF4CPP_SYNCREFERENCENODESTORAGEBACKEND_HPP

#include <atomic>
#include <tuple>

#include <rdf4cpp/rdf/storage/node/INodeStorageBackend.hpp>
#include <rdf4cpp/rdf/storage/node/reference_node_storage/BNodeBackend.hpp>
#include <rdf4cpp/rdf/storage/node/reference_node_storage/FallbackLiteralBackend.hpp>
#include <rdf4cpp/rdf/storage/node/reference_node_storage/IRIBackend.hpp>
#include <rdf4cpp/rdf/storage/node/reference_node_storage/SpecializedLiteralBackend.hpp>
#include <rdf4cpp/rdf/storage/node/reference_node_storage/VariableBackend.hpp>
#include <rdf4cpp/rdf/storage/node/reference_node_storage/detail/SyncNodeTypeStorage.hpp>


namespace rdf4cpp::rdf::storage::node::reference_node_storage {

/**
 * Thread-safe reference implementation of a INodeStorageBackend.
 */
struct SyncReferenceNodeStorageBackend : INodeStorageBackend {
private:
    SyncNodeTypeStorage<BNodeBackend> bnode_storage_;
    SyncNodeTypeStorage<IRIBackend> iri_storage_;
    SyncNodeTypeStorage<VariableBackend> variable_storage_;

    SyncNodeTypeStorage<FallbackLiteralBackend> fallback_literal_storage_;

    std::tuple<SyncNodeTypeStorage<SpecializedLiteralBackend<datatypes::xsd::Integer>>,
               SyncNodeTypeStorage<SpecializedLiteralBackend<datatypes::xsd::NonNegativeInteger>>,
               SyncNodeTypeStorage<SpecializedLiteralBackend<datatypes::xsd::PositiveInteger>>,
               SyncNodeTypeStorage<SpecializedLiteralBackend<datatypes::xsd::NonPositiveInteger>>,
               SyncNodeTypeStorage<SpecializedLiteralBackend<datatypes::xsd::NegativeInteger>>,
               SyncNodeTypeStorage<SpecializedLiteralBackend<datatypes::xsd::Long>>,
               SyncNodeTypeStorage<SpecializedLiteralBackend<datatypes::xsd::UnsignedLong>>,

               SyncNodeTypeStorage<SpecializedLiteralBackend<datatypes::xsd::Decimal>>,
               SyncNodeTypeStorage<SpecializedLiteralBackend<datatypes::xsd::Double>>,

               SyncNodeTypeStorage<SpecializedLiteralBackend<datatypes::xsd::Base64Binary>>,
               SyncNodeTypeStorage<SpecializedLiteralBackend<datatypes::xsd::HexBinary>>,

               SyncNodeTypeStorage<SpecializedLiteralBackend<datatypes::xsd::Date>>,
               SyncNodeTypeStorage<SpecializedLiteralBackend<datatypes::xsd::DateTime>>,
               SyncNodeTypeStorage<SpecializedLiteralBackend<datatypes::xsd::DateTimeStamp>>,
               SyncNodeTypeStorage<SpecializedLiteralBackend<datatypes::xsd::GYearMonth>>,
               SyncNodeTypeStorage<SpecializedLiteralBackend<datatypes::xsd::Duration>>,
               SyncNodeTypeStorage<SpecializedLiteralBackend<datatypes::xsd::DayTimeDuration>>,
               SyncNodeTypeStorage<SpecializedLiteralBackend<datatypes::xsd::YearMonthDuration>>> specialized_literal_storage_;

public:
    SyncReferenceNodeStorageBackend() noexcept;

    [[nodiscard]] size_t size() const noexcept override;
    void shrink_to_fit() override;

    [[nodiscard]] bool has_specialized_storage_for(identifier::LiteralType datatype) const noexcept override;

    [[nodiscard]] identifier::NodeID find_or_make_id(view::BNodeBackendView const &view) noexcept override;
    [[nodiscard]] identifier::NodeID find_or_make_id(view::IRIBackendView const &view) noexcept override;
    [[nodiscard]] identifier::NodeID find_or_make_id(view::LiteralBackendView const &view) noexcept override;
    [[nodiscard]] identifier::NodeID find_or_make_id(view::VariableBackendView const &view) noexcept override;

    [[nodiscard]] identifier::NodeID find_id(view::BNodeBackendView const &view) const noexcept override;
    [[nodiscard]] identifier::NodeID find_id(view::IRIBackendView const &view) const noexcept override;
    [[nodiscard]] identifier::NodeID find_id(view::LiteralBackendView const &view) const noexcept override;
    [[nodiscard]] identifier::NodeID find_id(view::VariableBackendView const &view) const noexcept override;

    [[nodiscard]] view::IRIBackendView find_iri_backend_view(identifier::NodeID id) const override;
    [[nodiscard]] view::LiteralBackendView find_literal_backend_view(identifier::NodeID id) const override;
    [[nodiscard]] view::BNodeBackendView find_bnode_backend_view(identifier::NodeID id) const override;
    [[nodiscard]] view::VariableBackendView find_variable_backend_view(identifier::NodeID id) const override;

    bool erase_iri(identifier::NodeID id) noexcept override;
    bool erase_literal(identifier::NodeID id) noexcept override;
    bool erase_bnode(identifier::NodeID id) noexcept override;
    bool erase_variable(identifier::NodeID id) noexcept override;
};

}  // namespace rdf4cpp::rdf::storage::node::reference_node_storage
#endif  //RDF4CPP_SYNCREFERENCENODESTORAGEBACKEND_HPP
