#include "Variable.hpp"

#include <rdf4cpp/InvalidNode.hpp>
#include <rdf4cpp/util/CharMatcher.hpp>
#include <rdf4cpp/writer/TryWrite.hpp>
#include <uni_algo/all.h>

namespace rdf4cpp::query {

namespace inlining {
    inline constexpr size_t max_inlined_name_len = (storage::identifier::NodeID::width / 8) - 1; // -1 for anonymous tagging boolean

    struct InlinedRepr {
        char name[max_inlined_name_len];
        bool is_anonymous;

        [[nodiscard]] std::pair<std::string_view, bool> view() && {
            auto const is_anon = std::exchange(is_anonymous, false);
            return std::make_pair(std::string_view{name}, is_anon);
        }

        auto operator<=>(InlinedRepr const &other) const noexcept = default;
    };

    union InlineTransmuter {
        storage::identifier::NodeID node_id;
        InlinedRepr inlined;
    };

    [[nodiscard]] storage::identifier::NodeBackendID try_get_inlined(std::string_view str, bool is_anon) noexcept {
        using namespace storage::identifier;

        if (str.size() > max_inlined_name_len) {
            return NodeBackendID{};
        }

        InlinedRepr inlined_data;
        inlined_data.is_anonymous = is_anon;
        memcpy(&inlined_data.name, str.data(), str.size());
        if (str.size() < max_inlined_name_len) {
            inlined_data.name[str.size()] = '\0'; // if name is shorter than max, put null terminator
        }

        auto const node_id = std::bit_cast<NodeID>(inlined_data);

        return NodeBackendID{node_id, RDFNodeType::Variable, true};
    }

    [[nodiscard]] InlinedRepr from_inlined(storage::identifier::NodeBackendID id) {
        assert(id.is_inlined());
        return std::bit_cast<InlinedRepr>(id.node_id());
    }

} // namespace inlining

Variable::Variable() noexcept : Node{storage::identifier::NodeBackendHandle{{}, storage::identifier::RDFNodeType::Variable, {}}} {
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
    auto const node_backend_id = [&]() {
        if (auto const inlined_id = inlining::try_get_inlined(name, anonymous); !inlined_id.null()) {
            return inlined_id;
        }

        return node_storage.find_or_make_id(storage::view::VariableBackendView{.name = name, .is_anonymous = anonymous});
    }();

    return Variable{storage::identifier::NodeBackendHandle{node_backend_id, node_storage}};
}

Variable Variable::to_node_storage(storage::DynNodeStoragePtr node_storage) const {
    if (handle_.storage() == node_storage || null()) {
        return *this;
    }

    if (handle_.is_inlined()) {
        return Variable{storage::identifier::NodeBackendHandle{handle_.id(), node_storage}};
    }

    auto const node_id = node_storage.find_or_make_id(handle_.variable_backend());
    return Variable{storage::identifier::NodeBackendHandle{node_id, node_storage}};
}

Variable Variable::try_get_in_node_storage(storage::DynNodeStoragePtr node_storage) const noexcept {
    if (handle_.storage() == node_storage || null()) {
        return *this;
    }

    if (handle_.is_inlined()) {
        return Variable{storage::identifier::NodeBackendHandle{handle_.id(), node_storage}};
    }

    auto const node_id = node_storage.find_id(handle_.variable_backend());
    if (node_id.null()) {
        return Variable{};
    }

    return Variable{storage::identifier::NodeBackendHandle{node_id, node_storage}};
}

Variable Variable::find(std::string_view name, bool anonymous, storage::DynNodeStoragePtr node_storage) noexcept {
    auto const nid = [&]() {
        if (auto const inlined_id = inlining::try_get_inlined(name, anonymous); !inlined_id.null()) {
            return inlined_id;
        }

        return node_storage.find_id(storage::view::VariableBackendView{.name = name, .is_anonymous = anonymous});
    }();

    if (nid.null()) {
        return Variable{};
    }

    return Variable{storage::identifier::NodeBackendHandle{nid, node_storage}};
}
Variable Variable::find_named(std::string_view name, storage::DynNodeStoragePtr node_storage) noexcept {
    return find(name, false, node_storage);
}
Variable Variable::find_anonymous(std::string_view name, storage::DynNodeStoragePtr node_storage) noexcept {
    return find(name, true, node_storage);
}

bool Variable::is_anonymous() const {
    if (handle_.is_inlined()) {
        return inlining::from_inlined(handle_.id()).is_anonymous;
    }

    // TODO: encode is_anonymous into variable ID
    return handle_.variable_backend().is_anonymous;
}

CowString Variable::name() const {
    if (handle_.is_inlined()) {
        auto inlined_repr = inlining::from_inlined(handle_.id());
        auto [name, _] = std::move(inlined_repr).view();

        return CowString{CowString::owned, std::string{name}};
    }

    return CowString{CowString::borrowed, handle_.variable_backend().name};
}

bool Variable::serialize(writer::BufWriterParts const writer) const noexcept {
    if (null()) {
        return rdf4cpp::writer::write_str("null", writer);
    }

    auto const run_ser = [&writer](std::string_view name, bool is_anon) {
        if (is_anon) {
            RDF4CPP_DETAIL_TRY_WRITE_STR("_:");
        } else {
            RDF4CPP_DETAIL_TRY_WRITE_STR("?");
        }

        RDF4CPP_DETAIL_TRY_WRITE_STR(name);
        return true;
    };

    if (handle_.is_inlined()) {
        auto backend = inlining::from_inlined(handle_.id());
        auto const [name, is_anon] = std::move(backend).view();

        return run_ser(name, is_anon);
    } else {
        auto const backend = handle_.variable_backend();
        return run_ser(backend.name, backend.is_anonymous);
    }
}

Variable::operator std::string() const {
    return writer::StringWriter::oneshot([this](auto &w) noexcept {
        return this->serialize(w);
    });
}

bool Variable::is_literal() const { return false; }
bool Variable::is_variable() const { return true; }
bool Variable::is_blank_node() const { return false; }
bool Variable::is_iri() const { return false; }

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

std::strong_ordering Variable::order(Variable const &other) const noexcept {
    if (handle_.is_inlined()) {
        if (other.is_inlined()) {
            return inlining::from_inlined(handle_.id()) <=> inlining::from_inlined(other.handle_.id());
        }

        // if this is inlined but other is not it means that the string of this must be shorter than that of other
        // TODO what about anon
        return std::strong_ordering::less;
    }

    return handle_.variable_backend() <=> other.handle_.variable_backend();
}

bool Variable::eq(Variable const &other) const noexcept {
    return order(other) == std::strong_ordering::equal;
}
bool Variable::ne(Variable const &other) const noexcept {
    return !eq(other);
}

}  // namespace rdf4cpp::query

auto std::formatter<rdf4cpp::query::Variable>::format(rdf4cpp::query::Variable n, format_context &ctx) const -> decltype(ctx.out()) {
    rdf4cpp::writer::BufOutputIteratorWriter w{ctx.out()};
    n.serialize(w);
    w.finalize();
    return w.buffer().iter;
}
