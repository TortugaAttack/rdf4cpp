#include "BlankNode.hpp"
#include <rdf4cpp/InvalidNode.hpp>
#include <rdf4cpp/util/CharMatcher.hpp>
#include <rdf4cpp/writer/TryWrite.hpp>
#include <uni_algo/all.h>

namespace rdf4cpp {
BlankNode::BlankNode() noexcept : Node{storage::identifier::NodeBackendHandle{}} {
}

BlankNode::BlankNode(std::string_view identifier, storage::DynNodeStoragePtr node_storage)
    : BlankNode{make_unchecked((validate(identifier), identifier), node_storage)} {
}

BlankNode::BlankNode(storage::identifier::NodeBackendHandle handle) noexcept : Node{handle} {
}

BlankNode BlankNode::make_null() noexcept {
    return BlankNode{};
}

BlankNode BlankNode::make(std::string_view identifier, storage::DynNodeStoragePtr node_storage) {
    return BlankNode{identifier, node_storage};
}

BlankNode BlankNode::make_unchecked(std::string_view identifier, storage::DynNodeStoragePtr node_storage) {
    return BlankNode{storage::identifier::NodeBackendHandle{node_storage.find_or_make_id(storage::view::BNodeBackendView{.identifier = identifier}),
                                                            node_storage}};
}

BlankNode BlankNode::to_node_storage(storage::DynNodeStoragePtr node_storage) const {
    if (handle_.storage() == node_storage || null()) {
        return *this;
    }

    auto const node_id = node_storage.find_or_make_id(handle_.bnode_backend());
    return BlankNode{storage::identifier::NodeBackendHandle{node_id, node_storage}};
}

BlankNode BlankNode::try_get_in_node_storage(storage::DynNodeStoragePtr node_storage) const noexcept {
    if (handle_.storage() == node_storage || null()) {
        return *this;
    }

    auto const node_id = node_storage.find_id(handle_.bnode_backend());
    if (node_id.null()) {
        return BlankNode{};
    }

    return BlankNode{storage::identifier::NodeBackendHandle{node_id, node_storage}};
}

BlankNode BlankNode::find(std::string_view identifier, storage::DynNodeStoragePtr node_storage) noexcept {
    auto nid = node_storage.find_id(storage::view::BNodeBackendView{.identifier = identifier});
    if (nid.null())
        return BlankNode{};
    return BlankNode{storage::identifier::NodeBackendHandle{nid, node_storage}};
}

std::string_view BlankNode::identifier() const noexcept { return handle_.bnode_backend().identifier; }

bool BlankNode::serialize(writer::BufWriterParts const writer) const noexcept {
    if (null()) {
        return rdf4cpp::writer::write_str("null", writer);
    }

    auto const backend = handle_.bnode_backend();

    RDF4CPP_DETAIL_TRY_WRITE_STR("_:");
    RDF4CPP_DETAIL_TRY_WRITE_STR(backend.identifier);
    return true;
}

BlankNode::operator std::string() const noexcept {
    return writer::StringWriter::oneshot([this](auto &w) noexcept {
        return this->serialize(w);
    });
}

bool BlankNode::is_literal() const noexcept { return false; }
bool BlankNode::is_variable() const noexcept { return false; }
bool BlankNode::is_blank_node() const noexcept { return true; }
bool BlankNode::is_iri() const noexcept { return false; }
std::ostream &operator<<(std::ostream &os, BlankNode const &bnode) {
    writer::BufOStreamWriter w{os};
    bnode.serialize(w);
    w.finalize();

    return os;
}

void BlankNode::validate(std::string_view v) {
    using namespace util::char_matcher_detail;
    static constexpr auto first_matcher = ASCIINumMatcher{} | PNCharsUMatcher;
    auto r = v | una::views::utf8;
    auto it = r.begin();
    if (it == r.end()) {
        throw InvalidNode("invalid blank node label (empty string)");
    }
    if (!first_matcher.match(*it)) {
        throw InvalidNode(std::format("invalid blank node label {}", v));
    }
    auto lastchar = *it;
    ++it;
    static constexpr auto pn_matcher = PNCharsMatcher | ASCIIPatternMatcher{"."};
    while (it != r.end()) {
        if (!pn_matcher.match(*it))
        {
            throw InvalidNode(std::format("invalid blank node label {}", v));
        }
        lastchar = *it;
        ++it;
    }
    if (lastchar == '.') {
        throw InvalidNode(std::format("invalid blank node label {}", v));
    }
}

inline namespace shorthands {

BlankNode operator""_bnode(char const *str, size_t len) {
    return BlankNode{std::string_view{str, len}};
}

}  // namespace literals
}  // namespace rdf4cpp

auto std::formatter<rdf4cpp::BlankNode>::format(rdf4cpp::BlankNode n, format_context &ctx) const -> decltype(ctx.out()) {
    rdf4cpp::writer::BufOutputIteratorWriter w{ctx.out()};
    n.serialize(w);
    w.finalize();
    return w.buffer().iter;
}
