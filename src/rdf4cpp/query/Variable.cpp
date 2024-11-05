#include "Variable.hpp"

#include <rdf4cpp/InvalidNode.hpp>
#include <rdf4cpp/util/CharMatcher.hpp>
#include <rdf4cpp/writer/TryWrite.hpp>
#include <uni_algo/all.h>

namespace rdf4cpp::query {
Variable::Variable() noexcept : Node{storage::identifier::NodeBackendHandle{}} {
}

Variable::Variable(std::string_view name, bool anonymous, storage::DynNodeStoragePtr node_storage)
    : Variable{make_unchecked((validate(name, anonymous), name), anonymous, node_storage)} {}

Variable::Variable(storage::identifier::NodeBackendHandle handle) noexcept : Node{handle} {}

Variable Variable::make_named(std::string_view name, storage::DynNodeStoragePtr node_storage) {
    return Variable{name, false, node_storage};
}

Variable Variable::make_anonymous(std::string_view name, storage::DynNodeStoragePtr node_storage) {
    return Variable{name, true, node_storage};
}

Variable Variable::make_unchecked(std::string_view name, bool anonymous, storage::DynNodeStoragePtr node_storage) {
    return Variable{storage::identifier::NodeBackendHandle{node_storage.find_or_make_id(storage::view::VariableBackendView{.name = name, .is_anonymous = anonymous}),
                                                           node_storage}};
}

Variable Variable::to_node_storage(storage::DynNodeStoragePtr node_storage) const {
    if (handle_.storage() == node_storage || null()) {
        return *this;
    }

    auto const node_id = node_storage.find_or_make_id(handle_.variable_backend());
    return Variable{storage::identifier::NodeBackendHandle{node_id, node_storage}};
}

Variable Variable::try_get_in_node_storage(storage::DynNodeStoragePtr node_storage) const noexcept {
    if (handle_.storage() == node_storage || null()) {
        return *this;
    }

    auto const node_id = node_storage.find_id(handle_.variable_backend());
    if (node_id.null()) {
        return Variable{};
    }

    return Variable{storage::identifier::NodeBackendHandle{node_id, node_storage}};
}

Variable Variable::find(std::string_view name, bool anonymous, storage::DynNodeStoragePtr node_storage) noexcept {
    auto nid = node_storage.find_id(storage::view::VariableBackendView{.name = name, .is_anonymous = anonymous});

    if (nid.null())
        return Variable{};

    return Variable{storage::identifier::NodeBackendHandle{nid, node_storage}};
}
Variable Variable::find_named(std::string_view name, storage::DynNodeStoragePtr node_storage) noexcept {
    return find(name, false, node_storage);
}
Variable Variable::find_anonymous(std::string_view name, storage::DynNodeStoragePtr node_storage) noexcept {
    return find(name, true, node_storage);
}

bool Variable::is_anonymous() const {
    // TODO: encode is_anonymous into variable ID
    return this->handle_.variable_backend().is_anonymous;
}
std::string_view Variable::name() const {
    return this->handle_.variable_backend().name;
}

bool Variable::serialize(writer::BufWriterParts const writer) const noexcept {
    if (null()) {
        return rdf4cpp::writer::write_str("null", writer);
    }

    auto const backend = handle_.variable_backend();

    if (backend.is_anonymous) {
        RDF4CPP_DETAIL_TRY_WRITE_STR("_:");
    } else {
        RDF4CPP_DETAIL_TRY_WRITE_STR("?");
    }

    RDF4CPP_DETAIL_TRY_WRITE_STR(backend.name);
    return true;
}

Variable::operator std::string() const {
    return writer::StringWriter::oneshot([this](auto &w) noexcept {
        return this->serialize(w);
    });
}

std::ostream &operator<<(std::ostream &os, Variable const &variable) {
    writer::BufOStreamWriter w{os};
    variable.serialize(w);
    w.finalize();

    return os;
}

void Variable::validate(std::string_view n, bool anonymous) {
    if (anonymous) {
        return BlankNode::validate(n);
    }

    using namespace util::char_matcher_detail;
    static constexpr auto first_matcher = ASCIINumMatcher{} | PNCharsUMatcher;
    auto r = n | una::views::utf8;
    auto it = r.begin();
    if (it == r.end()) {
        throw InvalidNode("invalid blank node label (empty string)");
    }
    if (!first_matcher.match(*it)) {
        throw InvalidNode(std::format("invalid blank node label {}", n));
    }
    ++it;

    static constexpr auto matcher = ASCIINumMatcher{} | PNCharsUMatcher | PNChars_UnicodePartMatcher{};
    while (it != r.end()) {
        if (!matcher.match(*it)) {
            throw InvalidNode(std::format("invalid blank node label {}", n));
        }
        ++it;
    }
}

}  // namespace rdf4cpp::query

auto std::formatter<rdf4cpp::query::Variable>::format(rdf4cpp::query::Variable n, format_context &ctx) const -> decltype(ctx.out()) {
    rdf4cpp::writer::BufOutputIteratorWriter w{ctx.out()};
    n.serialize(w);
    w.finalize();
    return w.buffer().iter;
}
