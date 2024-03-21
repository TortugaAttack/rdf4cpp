#ifndef RDF4CPP_PRIVATE_SERIALIZEQUAD_HPP
#define RDF4CPP_PRIVATE_SERIALIZEQUAD_HPP

#include <rdf4cpp/writer/OutputFormat.hpp>
#include <rdf4cpp/writer/TryWrite.hpp>
#include <rdf4cpp/writer/SerializationState.hpp>

namespace rdf4cpp::writer {

template<writer::OutputFormat F>
bool write_node(Node const node, writer::BufWriterParts const writer) noexcept {
    if constexpr (writer::format_has_prefix<F>) {
        return node.serialize_short_form(writer);
    } else {
        return node.serialize(writer);
    }
}

template<writer::OutputFormat F>
bool write_pred(Node const pred, writer::BufWriterParts const writer) {
    if constexpr (writer::format_has_prefix<F>) {
        static constexpr storage::identifier::LiteralType rdf_type = datatypes::registry::reserved_datatype_ids[datatypes::registry::rdf_type];

        if (pred.is_iri() && iri_node_id_to_literal_type(pred.backend_handle().node_id()) == rdf_type) {
            return writer::write_str("a", writer);
        }
    }

    return write_node<F>(pred, writer);
}

#define RDF4CPP_DETAIL_TRY_WRITE_NODE(pred) \
    if (!write_node<F>((pred), writer)) {   \
        return false;                       \
    }

#define RDF4CPP_DETAIL_TRY_WRITE_PRED(pred) \
    if (!write_pred<F>((pred), writer)) {   \
        return false;                       \
    }

template<writer::OutputFormat F, typename Q>
bool write_quad(Q const &s, writer::BufWriterParts const writer, writer::SerializationState *const state) noexcept {
    if constexpr (writer::format_has_prefix<F>) {
        if constexpr (writer::format_has_graph<F>) {
            if (s.graph() != state->active_graph) {
                if (!state->flush(writer)) {
                    return false;
                }

                if (!s.graph().null()) {
                    RDF4CPP_DETAIL_TRY_WRITE_NODE(s.graph());
                    RDF4CPP_DETAIL_TRY_WRITE_STR(" {\n");

                    state->active_graph = s.graph();
                }
            }
        }

        if (!state->active_subject.null() && state->active_subject == s.subject()) {
            if (!state->active_predicate.null() && state->active_predicate == s.predicate()) {
                RDF4CPP_DETAIL_TRY_WRITE_STR(" ,\n");
                RDF4CPP_DETAIL_TRY_WRITE_NODE(s.object());
                return true;
            }

            RDF4CPP_DETAIL_TRY_WRITE_STR(" ;\n");
            RDF4CPP_DETAIL_TRY_WRITE_PRED(s.predicate());

            state->active_predicate = s.predicate();
            RDF4CPP_DETAIL_TRY_WRITE_STR(" ");
            RDF4CPP_DETAIL_TRY_WRITE_NODE(s.object());
            return true;
        }

        if (!state->active_subject.null()) {
            RDF4CPP_DETAIL_TRY_WRITE_STR(" .\n");
        }

        state->active_subject = s.subject();
        state->active_predicate = s.predicate();
    }

    RDF4CPP_DETAIL_TRY_WRITE_NODE(s.subject());
    RDF4CPP_DETAIL_TRY_WRITE_STR(" ");
    RDF4CPP_DETAIL_TRY_WRITE_PRED(s.predicate());
    RDF4CPP_DETAIL_TRY_WRITE_STR(" ");
    RDF4CPP_DETAIL_TRY_WRITE_NODE(s.object());

    if constexpr (!writer::format_has_prefix<F>) {
        if constexpr (writer::format_has_graph<F>) {
            if (!s.graph().null()) {
                RDF4CPP_DETAIL_TRY_WRITE_STR(" ");
                RDF4CPP_DETAIL_TRY_WRITE_NODE(s.graph());
            }
        }

        RDF4CPP_DETAIL_TRY_WRITE_STR(" .\n");
    }

    return true;
}

#undef RDF4CPP_DETAIL_TRY_WRITE_NODE
#undef RDF4CPP_DETAIL_TRY_WRITE_PRED

} // namespace rdf4cpp::writer

#endif // RDF4CPP_PRIVATE_SERIALIZEQUAD_HPP
