#include "IRI.hpp"
#include <rdf4cpp/writer/TryWrite.hpp>

#include <sstream>

#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>

namespace rdf4cpp {

IRI::IRI(storage::identifier::NodeBackendHandle handle) noexcept : Node(handle) {
}

IRI::IRI() noexcept : Node{storage::identifier::NodeBackendHandle{{}, storage::identifier::RDFNodeType::IRI, {}}} {
}

IRI::IRI(std::string_view iri, storage::DynNodeStoragePtr node_storage)
    : Node{storage::identifier::NodeBackendHandle{node_storage.find_or_make_id(storage::view::IRIBackendView{.identifier = iri}),
                                                  storage::identifier::RDFNodeType::IRI,
                                                  node_storage}} {
}

IRI::IRI(datatypes::registry::DatatypeIDView id, storage::DynNodeStoragePtr node_storage) noexcept
    : IRI{visit(datatypes::registry::DatatypeIDVisitor{
                        [&](storage::identifier::LiteralType const fixed) -> IRI {
                            return IRI{storage::identifier::NodeBackendHandle{storage::identifier::literal_type_to_iri_node_id(fixed),
                                                                              storage::identifier::RDFNodeType::IRI,
                                                                              node_storage}};
                        },
                        [&](std::string_view const dynamic) -> IRI {
                            return IRI{dynamic, node_storage};
                        }},
                id)} {
}

IRI IRI::make_null() noexcept {
    return IRI{};
}

IRI IRI::make(std::string_view iri, storage::DynNodeStoragePtr node_storage) {
    return IRI{iri, node_storage};
}

IRI IRI::make_uuid(storage::DynNodeStoragePtr node_storage) {
    boost::uuids::random_generator_mt19937 gen{};
    boost::uuids::uuid u = gen();
    std::stringstream stream{};
    stream << "urn:uuid:" << u;
    return IRI{stream.view(), node_storage};
}

IRI IRI::to_node_storage(storage::DynNodeStoragePtr node_storage) const {
    if (handle_.storage() == node_storage) {
        return *this;
    }

    auto const node_id = node_storage.find_or_make_id(handle_.iri_backend());
    return IRI{storage::identifier::NodeBackendHandle{node_id, storage::identifier::RDFNodeType::IRI, node_storage}};
}

IRI IRI::try_get_in_node_storage(storage::DynNodeStoragePtr node_storage) const noexcept {
    if (handle_.storage() == node_storage) {
        return *this;
    }

    auto const node_id = node_storage.find_id(handle_.iri_backend());
    if (node_id == storage::identifier::NodeID{}) {
        return IRI{};
    }

    return IRI{storage::identifier::NodeBackendHandle{node_id, storage::identifier::RDFNodeType::IRI, node_storage}};
}

IRI IRI::find(std::string_view iri, storage::DynNodeStoragePtr node_storage) noexcept {
    auto nid = node_storage.find_id(storage::view::IRIBackendView{iri});
    if (nid.null())
        return IRI{};
    return IRI{storage::identifier::NodeBackendHandle{nid, storage::identifier::RDFNodeType::IRI, node_storage}};
}
IRI IRI::find(datatypes::registry::DatatypeIDView id, storage::DynNodeStoragePtr node_storage) noexcept {
    return visit(datatypes::registry::DatatypeIDVisitor{
                             [&](storage::identifier::LiteralType const fixed) -> IRI {
                                 return IRI{storage::identifier::NodeBackendHandle{storage::identifier::literal_type_to_iri_node_id(fixed),
                                                                                   storage::identifier::RDFNodeType::IRI,
                                                                                   node_storage}};
                             },
                             [&](std::string_view const dynamic) -> IRI {
                                 return IRI::find(dynamic, node_storage);
                             }},
                     id);
}


IRI::operator datatypes::registry::DatatypeIDView() const noexcept {
    using namespace storage::identifier;

    auto const id = this->handle_.node_id();
    LiteralType const type = iri_node_id_to_literal_type(id);

    if (type.is_fixed()) {
        return datatypes::registry::DatatypeIDView{type};
    } else {
        return datatypes::registry::DatatypeIDView{this->identifier()};
    }
}

bool IRI::serialize(writer::BufWriterParts const writer) const noexcept {
    if (null()) {
        return rdf4cpp::writer::write_str("null", writer);
    }

    auto const backend = handle_.iri_backend();

    RDF4CPP_DETAIL_TRY_WRITE_STR("<");
    RDF4CPP_DETAIL_TRY_WRITE_STR(backend.identifier);
    RDF4CPP_DETAIL_TRY_WRITE_STR(">");
    return true;
}

IRI::operator std::string() const noexcept {
    return writer::StringWriter::oneshot([this](auto &w) noexcept {
        return this->serialize(w);
    });
}

bool IRI::is_literal() const noexcept { return false; }
bool IRI::is_variable() const noexcept { return false; }
bool IRI::is_blank_node() const noexcept { return false; }
bool IRI::is_iri() const noexcept { return true; }


IRI IRI::default_graph(storage::DynNodeStoragePtr node_storage) {
    return IRI{"", node_storage};
}
std::ostream &operator<<(std::ostream &os, IRI const &iri) {
    writer::BufOStreamWriter w{os};
    iri.serialize(w);
    w.finalize();

    return os;
}
std::string_view IRI::identifier() const noexcept {
    return handle_.iri_backend().identifier;
}

inline namespace shorthands {

IRI operator""_iri(char const *str, size_t const len) {
    return IRI{std::string_view{str, len}};
}

}  // namespace shorthands

}  // namespace rdf4cpp

auto std::formatter<rdf4cpp::IRI>::format(rdf4cpp::IRI n, format_context &ctx) const -> decltype(ctx.out()) {
    rdf4cpp::writer::BufOutputIteratorWriter w{ctx.out()};
    n.serialize(w);
    w.finalize();
    return w.buffer().iter;
}
