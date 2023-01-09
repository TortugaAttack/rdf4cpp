#include "IRI.hpp"

namespace rdf4cpp::rdf {

IRI::IRI(Node::NodeBackendHandle handle) noexcept : Node(handle) {}
IRI::IRI() noexcept : Node(NodeBackendHandle{{}, storage::node::identifier::RDFNodeType::IRI, {}}) {}
IRI::IRI(std::string_view iri, Node::NodeStorage &node_storage)
    : Node(NodeBackendHandle{node_storage.find_or_make_id(storage::node::view::IRIBackendView{.identifier = iri}),
                             storage::node::identifier::RDFNodeType::IRI,
                             node_storage.id()}) {}

IRI::IRI(datatypes::registry::DatatypeIDView id, Node::NodeStorage &node_storage) noexcept
    : IRI{visit(datatypes::registry::DatatypeIDVisitor{
                        [&](storage::node::identifier::LiteralType const fixed) -> IRI {
                            return IRI{NodeBackendHandle{NodeID{static_cast<uint64_t>(fixed.to_underlying())},
                                                         storage::node::identifier::RDFNodeType::IRI,
                                                         node_storage.id()}};
                        },
                        [&](std::string_view const dynamic) -> IRI {
                            return IRI{dynamic, node_storage};
                        }},
                id)} {
}

IRI::operator datatypes::registry::DatatypeIDView() const noexcept {
    using namespace storage::node::identifier;

    auto const id = this->handle_.node_id();
    LiteralType const type = iri_node_id_to_literal_type(id);

    if (type.is_fixed()) {
        return datatypes::registry::DatatypeIDView{type};
    } else {
        return datatypes::registry::DatatypeIDView{this->identifier()};
    }
}

IRI::operator std::string() const noexcept { return handle_.iri_backend().n_string(); }

bool IRI::is_literal() const noexcept { return false; }
bool IRI::is_variable() const noexcept { return false; }
bool IRI::is_blank_node() const noexcept { return false; }
bool IRI::is_iri() const noexcept { return true; }


IRI IRI::default_graph(NodeStorage &node_storage) {
    return IRI("", node_storage);
}
std::ostream &operator<<(std::ostream &os, const IRI &iri) {
    os << static_cast<std::string>(iri);
    return os;
}
std::string_view IRI::identifier() const noexcept {
    return handle_.iri_backend().identifier;
}


inline namespace literals {

IRI operator""_iri(char const *str, size_t const len) {
    return IRI{std::string_view{str, len}};
}

}  // namespace literals
}  // namespace rdf4cpp::rdf