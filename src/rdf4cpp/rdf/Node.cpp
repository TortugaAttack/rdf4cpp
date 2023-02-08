#include "Node.hpp"

#include <cassert>
#include <rdf4cpp/rdf/BlankNode.hpp>
#include <rdf4cpp/rdf/IRI.hpp>
#include <rdf4cpp/rdf/Literal.hpp>
#include <rdf4cpp/rdf/query/Variable.hpp>

namespace rdf4cpp::rdf {

Node::Node(Node::NodeBackendHandle id) noexcept : handle_(id) {}

Node Node::make_null() noexcept {
    return Node{};
}

Node Node::to_node_storage(Node::NodeStorage &node_storage) const noexcept {
    if (this->backend_handle().node_storage_id() == node_storage.id())
        return *this;
    else {
        NodeID node_id = [&]() {
            switch (this->backend_handle().type()) {

                case RDFNodeType::Variable: {
                    return node_storage.find_or_make_id(NodeStorage::find_variable_backend_view(backend_handle()));
                }
                case RDFNodeType::BNode: {
                    return node_storage.find_or_make_id(NodeStorage::find_bnode_backend_view(backend_handle()));
                }
                case RDFNodeType::IRI: {
                    return node_storage.find_or_make_id(NodeStorage::find_iri_backend_view(backend_handle()));
                }
                case RDFNodeType::Literal: {
                    // retrieve the literal_view
                    auto literal_view = NodeStorage::find_literal_backend_view(backend_handle());
                    // exchange the datatype in literal_view for one managed by the new node_storage (the IRI of the datatype must live within the same NodeStorage as the Literal it is used for)
                    auto dtype_iri_view = NodeStorage::find_iri_backend_view(NodeBackendHandle{literal_view.datatype_id, storage::node::identifier::RDFNodeType::IRI, backend_handle().node_storage_id()});
                    literal_view.datatype_id = node_storage.find_or_make_id(dtype_iri_view);
                    // find or make the requested node
                    return node_storage.find_or_make_id(literal_view);
                }
                default:
                    return NodeID{};
            }
        }();
        return Node{NodeBackendHandle{node_id, backend_handle().type(), node_storage.id()}};
    }
}

Node::operator std::string() const noexcept {
    switch (handle_.type()) {
        case RDFNodeType::IRI:
            return handle_.iri_backend().n_string();
        case RDFNodeType::BNode:
            return handle_.bnode_backend().n_string();
        case RDFNodeType::Literal: {
            return static_cast<std::string>(Literal{handle_});
        }
        case RDFNodeType::Variable:
            return handle_.variable_backend().n_string();
    }
    return "";
}

bool Node::is_literal() const noexcept {
    return handle_.is_literal();
}
bool Node::is_variable() const noexcept {
    return handle_.is_variable();
}
bool Node::is_blank_node() const noexcept {
    return handle_.is_blank_node();
}
bool Node::is_iri() const noexcept {
    return handle_.is_iri();
}

bool Node::is_inlined() const noexcept {
    return handle_.is_inlined();
}

std::weak_ordering Node::operator<=>(const Node &other) const noexcept {
    if (this->handle_ == other.handle_){
        return std::strong_ordering::equivalent;
    }

    if (this->null() && other.null()) {
        return this->handle_.type() <=> other.handle_.type();
    }

    // unbound
    if (this->null()) {
        return std::strong_ordering::less;
    } else if (other.null()) {
        return std::strong_ordering::greater;
    }

    // different type
    if (std::strong_ordering type_comp = this->handle_.type() <=> other.handle_.type(); type_comp != std::strong_ordering::equivalent){
        return type_comp;
    } else {
        switch (this->handle_.type()) {
            case RDFNodeType::IRI:
                return this->handle_.iri_backend() <=> other.handle_.iri_backend();
            case RDFNodeType::BNode:
                return this->handle_.bnode_backend() <=> other.handle_.bnode_backend();
            case RDFNodeType::Literal:
                return Literal{handle_}.compare_with_extensions(Literal{other.handle_});
            case RDFNodeType::Variable:
                return this->handle_.variable_backend() <=> other.handle_.variable_backend();
            default:{
                assert(false); // this will never be reached because RDFNodeType has only 4 values.
                return std::strong_ordering::less;
            }
        }
    }
}

bool Node::operator==(const Node &other) const noexcept {
    return *this <=> other == std::strong_ordering::equivalent;
}

BlankNode Node::as_blank_node() const noexcept {
    if (!this->is_blank_node()) {
        return BlankNode{};
    }

    return BlankNode{handle_};
}

IRI Node::as_iri() const noexcept {
    if (!this->is_iri()) {
        return IRI{};
    }

    return IRI{handle_};
}

Literal Node::as_literal() const noexcept {
    if (!this->is_literal()) {
        return Literal{};
    }

    return Literal{handle_};
}

query::Variable Node::as_variable() const noexcept {
    if (!this->is_variable()) {
        return query::Variable{};
    }

    return query::Variable{handle_};
}

bool Node::null() const noexcept {
    return handle_.null();
}
std::ostream &operator<<(std::ostream &os, const Node &node) {
    return os << static_cast<std::string>(node);
}
const Node::NodeBackendHandle &Node::backend_handle() const noexcept {
    return handle_;
}
Node::NodeBackendHandle &Node::backend_handle() noexcept {
    return handle_;
}

util::TriBool Node::ebv() const noexcept {
    if (this->null() || !this->is_literal()) {
        return util::TriBool::Err;
    }

    return Literal{handle_}.ebv();
}

Literal Node::as_ebv(NodeStorage &node_storage) const noexcept {
    return this->as_literal().as_ebv(node_storage);
}

}  // namespace rdf4cpp::rdf
