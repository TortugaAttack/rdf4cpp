#ifndef RDF4CPP_REFERENCEBNODESCOPE_HPP
#define RDF4CPP_REFERENCEBNODESCOPE_HPP

#include <rdf4cpp/rdf/bnode_management/reference_backends/scope/ReferenceScopeBase.hpp>

namespace rdf4cpp::rdf::util {

struct ReferenceBNodeScope final : ReferenceScopeBase<ReferenceBNodeScope> {
private:
    friend ReferenceScopeBase<ReferenceBNodeScope>;
    ReferenceBNodeScope(ReferenceBNodeScope const &other) = default;
public:
    ReferenceBNodeScope() = default;
    ReferenceBNodeScope(ReferenceBNodeScope &&other) noexcept = default;

    [[nodiscard]] storage::node::identifier::NodeBackendHandle make_null_node() const noexcept override;
    [[nodiscard]] storage::node::identifier::NodeBackendHandle make_node(IIdGenerator &generator, storage::node::NodeStorage &node_storage) override;
};

}  //namespace rdf4cpp::rdf::util

#endif  //RDF4CPP_REFERENCEBNODESCOPE_HPP
