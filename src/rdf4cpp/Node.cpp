#include "Node.hpp"

#include <cassert>
#include <rdf4cpp/BlankNode.hpp>
#include <rdf4cpp/IRI.hpp>
#include <rdf4cpp/Literal.hpp>
#include <rdf4cpp/query/Variable.hpp>


namespace rdf4cpp {

Node::Node(storage::identifier::NodeBackendHandle id) noexcept : handle_(id) {}

Node Node::make_null() noexcept {
    return Node{};
}

Node Node::to_node_storage(storage::DynNodeStoragePtr node_storage) const {
    switch (handle_.type()) {
        case storage::identifier::RDFNodeType::Variable: {
            return query::Variable{handle_}.to_node_storage(node_storage);
        }
        case storage::identifier::RDFNodeType::BNode: {
            return BlankNode{handle_}.to_node_storage(node_storage);
        }
        case storage::identifier::RDFNodeType::IRI: {
            return IRI{handle_}.to_node_storage(node_storage);
        }
        case storage::identifier::RDFNodeType::Literal: {
            return Literal{handle_}.to_node_storage(node_storage);
        }
        default: {
            assert(false);
            __builtin_unreachable();
        }
    }
}

Node Node::try_get_in_node_storage(storage::DynNodeStoragePtr node_storage) const noexcept {
    switch (handle_.type()) {
        case storage::identifier::RDFNodeType::Variable: {
            return query::Variable{handle_}.try_get_in_node_storage(node_storage);
        }
        case storage::identifier::RDFNodeType::BNode: {
            return BlankNode{handle_}.try_get_in_node_storage(node_storage);
        }
        case storage::identifier::RDFNodeType::IRI: {
            return IRI{handle_}.try_get_in_node_storage(node_storage);
        }
        case storage::identifier::RDFNodeType::Literal: {
            return Literal{handle_}.try_get_in_node_storage(node_storage);
        }
        default: {
            assert(false);
            __builtin_unreachable();
        }
    }
}

bool Node::serialize(writer::BufWriterParts const writer, NodeSerializationOpts opts) const noexcept {
    if (null()) {
        return rdf4cpp::writer::write_str("null", writer);
    }

    switch (handle_.type()) {
        [[likely]] case storage::identifier::RDFNodeType::IRI: {
            return IRI{handle_}.serialize(writer);
        }
        case storage::identifier::RDFNodeType::Variable: {
            return query::Variable{handle_}.serialize(writer);
        }
        case storage::identifier::RDFNodeType::BNode: {
            return BlankNode{handle_}.serialize(writer);
        }
        case storage::identifier::RDFNodeType::Literal: {
            return Literal{handle_}.serialize(writer, opts);
        }
        default: {
            assert(false);
            __builtin_unreachable();
        }
    }
}

Node::operator std::string() const noexcept {
    switch (handle_.type()) {
        [[likely]] case storage::identifier::RDFNodeType::IRI: {
            return std::string{IRI{handle_}};
        }
        case storage::identifier::RDFNodeType::Variable: {
            return std::string{query::Variable{handle_}};
        }
        case storage::identifier::RDFNodeType::BNode: {
            return std::string{BlankNode{handle_}};
        }
        case storage::identifier::RDFNodeType::Literal: {
            return std::string{Literal{handle_}};
        }
        default: {
            assert(false);
            __builtin_unreachable();
        }
    }
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

template<typename T>
[[nodiscard]] static std::partial_ordering eq_else_unordered(T const &a, T const &b) noexcept {
    if (a == b) {
        return std::partial_ordering::equivalent;
    }

    return std::partial_ordering::unordered;
}

std::partial_ordering Node::compare(Node const &other) const noexcept{
    if (handle_ == other.handle_) {
        return std::partial_ordering::equivalent;
    }

    if (handle_.type() != other.handle_.type()) {
        return std::partial_ordering::unordered;
    }

    // unbound
    if (null()) {
        return std::weak_ordering::less;
    } else if (other.null()) {
        return std::weak_ordering::greater;
    }

    using storage::identifier::RDFNodeType;
    switch (handle_.type()) {
        case RDFNodeType::Literal: {
            return Literal{handle_}.compare(Literal{other.handle_});
        }
        case RDFNodeType::IRI: {
            return eq_else_unordered(handle_.iri_backend(), other.handle_.iri_backend());
        }
        case RDFNodeType::BNode: {
            return eq_else_unordered(handle_.bnode_backend(), other.handle_.bnode_backend());
        }
        case RDFNodeType::Variable: {
            return eq_else_unordered(handle_.variable_backend(), other.handle_.variable_backend());
        }
        default: {
            assert(false); // unreachable
            return std::partial_ordering::unordered;
        }
    }
}

std::weak_ordering Node::order(Node const &other) const noexcept {
    if (this->handle_ == other.handle_) {
        return std::weak_ordering::equivalent;
    }

    if (this->null() && other.null()) {
        return this->handle_.type() <=> other.handle_.type();
    }

    // unbound
    if (this->null()) {
        return std::weak_ordering::less;
    } else if (other.null()) {
        return std::weak_ordering::greater;
    }

    // different type
    if (std::strong_ordering const type_comp = this->handle_.type() <=> other.handle_.type(); type_comp != std::strong_ordering::equivalent){
        return type_comp;
    } else {
        switch (this->handle_.type()) {
            case storage::identifier::RDFNodeType::IRI:
                return this->handle_.iri_backend() <=> other.handle_.iri_backend();
            case storage::identifier::RDFNodeType::BNode:
                return this->handle_.bnode_backend() <=> other.handle_.bnode_backend();
            case storage::identifier::RDFNodeType::Literal:
                return Literal{handle_}.order(Literal{other.handle_});
            case storage::identifier::RDFNodeType::Variable:
                return this->handle_.variable_backend() <=> other.handle_.variable_backend();
            default:{
                assert(false); // this will never be reached because RDFNodeType has only 4 values.
                return std::strong_ordering::less;
            }
        }
    }
}

TriBool Node::eq(Node const &other) const noexcept {
    return util::partial_weak_ordering_eq(compare(other), std::weak_ordering::equivalent);
}
bool Node::order_eq(Node const &other) const noexcept {
    return order(other) == std::weak_ordering::equivalent;
}
TriBool Node::ne(Node const &other) const noexcept {
    return !util::partial_weak_ordering_eq(compare(other), std::weak_ordering::equivalent);
}
bool Node::order_ne(Node const &other) const noexcept {
    return order(other) != std::weak_ordering::equivalent;
}
TriBool Node::lt(Node const &other) const noexcept {
    return util::partial_weak_ordering_eq(compare(other), std::weak_ordering::less);
}
bool Node::order_lt(Node const &other) const noexcept {
    return order(other) == std::weak_ordering::less;
}
TriBool Node::le(Node const &other) const noexcept {
    return !util::partial_weak_ordering_eq(compare(other), std::weak_ordering::greater);
}
bool Node::order_le(Node const &other) const noexcept {
    return order(other) != std::weak_ordering::greater;
}
TriBool Node::gt(Node const &other) const noexcept {
    return util::partial_weak_ordering_eq(compare(other), std::weak_ordering::greater);
}
bool Node::order_gt(Node const &other) const noexcept {
    return order(other) == std::weak_ordering::greater;
}
TriBool Node::ge(Node const &other) const noexcept {
    return !util::partial_weak_ordering_eq(compare(other), std::weak_ordering::less);
}
bool Node::order_ge(Node const &other) const noexcept {
    return order(other) != std::weak_ordering::less;
}

Literal Node::as_eq(Node const &other, storage::DynNodeStoragePtr node_storage) const noexcept {
    return Literal::make_boolean(eq(other), select_node_storage(node_storage));
}
Literal Node::as_order_eq(Node const &other, storage::DynNodeStoragePtr node_storage) const noexcept {
    return Literal::make_boolean(order_eq(other), select_node_storage(node_storage));
}
Literal Node::as_ne(Node const &other, storage::DynNodeStoragePtr node_storage) const noexcept {
    return Literal::make_boolean(ne(other), select_node_storage(node_storage));
}
Literal Node::as_order_ne(Node const &other, storage::DynNodeStoragePtr node_storage) const noexcept {
    return Literal::make_boolean(order_ne(other), select_node_storage(node_storage));
}
Literal Node::as_lt(Node const &other, storage::DynNodeStoragePtr node_storage) const noexcept {
    return Literal::make_boolean(lt(other), select_node_storage(node_storage));
}
Literal Node::as_order_lt(Node const &other, storage::DynNodeStoragePtr node_storage) const noexcept {
    return Literal::make_boolean(order_lt(other), select_node_storage(node_storage));
}
Literal Node::as_le(Node const &other, storage::DynNodeStoragePtr node_storage) const noexcept {
    return Literal::make_boolean(le(other), select_node_storage(node_storage));
}
Literal Node::as_order_le(Node const &other, storage::DynNodeStoragePtr node_storage) const noexcept {
    return Literal::make_boolean(order_le(other), select_node_storage(node_storage));
}
Literal Node::as_gt(Node const &other, storage::DynNodeStoragePtr node_storage) const noexcept {
    return Literal::make_boolean(gt(other), select_node_storage(node_storage));
}
Literal Node::as_order_gt(Node const &other, storage::DynNodeStoragePtr node_storage) const noexcept {
    return Literal::make_boolean(order_gt(other), select_node_storage(node_storage));
}
Literal Node::as_ge(Node const &other, storage::DynNodeStoragePtr node_storage) const noexcept {
    return Literal::make_boolean(ge(other), select_node_storage(node_storage));
}
Literal Node::as_order_ge(Node const &other, storage::DynNodeStoragePtr node_storage) const noexcept {
    return Literal::make_boolean(order_ge(other), select_node_storage(node_storage));
}

std::partial_ordering Node::operator<=>(Node const &other) const noexcept {
    return order(other);
}

bool Node::operator==(const Node &other) const noexcept {
    return order_eq(other);
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
std::ostream &operator<<(std::ostream &os, Node const &node) {
    writer::BufOStreamWriter w{os};
    node.serialize(w);
    w.finalize();

    return os;
}
storage::identifier::NodeBackendHandle const &Node::backend_handle() const noexcept {
    return handle_;
}
storage::identifier::NodeBackendHandle &Node::backend_handle() noexcept {
    return handle_;
}

TriBool Node::ebv() const noexcept {
    if (this->null() || !this->is_literal()) {
        return TriBool::Err;
    }

    return Literal{handle_}.ebv();
}

Literal Node::as_ebv(storage::DynNodeStoragePtr node_storage) const noexcept {
    return this->as_literal().as_ebv(node_storage);
}

Node::operator bool() const noexcept {
    return ebv();
}

}  // namespace rdf4cpp

auto std::formatter<rdf4cpp::Node>::format(rdf4cpp::Node n, format_context &ctx) const -> decltype(ctx.out()) {
    rdf4cpp::writer::BufOutputIteratorWriter w{ctx.out()};
    n.serialize(w);
    w.finalize();
    return w.buffer().iter;
}
