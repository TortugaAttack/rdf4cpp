#include <rdf4cpp/rdf/parser/private/IStreamQuadIteratorSerdImpl.hpp>

namespace rdf4cpp::rdf::parser {

std::string_view IStreamQuadIterator::Impl::node_into_string_view(SerdNode const *node) noexcept {
    return std::string_view{reinterpret_cast<char const *>(node->buf), static_cast<size_t>(node->n_bytes)};
}

ParsingError::Type IStreamQuadIterator::Impl::parsing_error_type_from_serd(SerdStatus const st) noexcept {
    switch (st) {
        case SerdStatus::SERD_ERR_BAD_SYNTAX:
            return ParsingError::Type::BadSyntax;
        case SerdStatus::SERD_ERR_BAD_CURIE:
            return ParsingError::Type::BadCurie;
        case SerdStatus::SERD_FAILURE:
            return ParsingError::Type::EofReached;
        default:
            assert(false);
    }
}

void IStreamQuadIterator::Impl::destroy_serd_reader(SerdReader *reader) noexcept {
    serd_reader_end_stream(reader);
    serd_reader_free(reader);
}

BlankNode IStreamQuadIterator::Impl::get_bnode(SerdNode const *node) {
    return BlankNode{node_into_string_view(node), this->node_storage.get()};
}

IRI IStreamQuadIterator::Impl::get_uri(SerdNode const *node) {
    return IRI{node_into_string_view(node), this->node_storage.get()};
}

nonstd::expected<IRI, SerdStatus> IStreamQuadIterator::Impl::get_prefixed_uri(SerdNode const *node) const {
    auto const uri_node_view = node_into_string_view(node);
    auto const sep_pos = uri_node_view.find(':');
    auto const prefix = uri_node_view.substr(0, sep_pos);
    auto const suffix = uri_node_view.substr(sep_pos + 1);

    if (auto const prefix_it = this->prefixes.find(prefix); prefix_it != this->prefixes.end()) {
        std::ostringstream oss;
        oss << prefix_it->second << suffix;
        return IRI{oss.view(), this->node_storage.get()};
    } else {
        return nonstd::make_unexpected(SerdStatus::SERD_ERR_BAD_CURIE);
    }
}

Literal IStreamQuadIterator::Impl::get_literal(SerdNode const *literal, SerdNode const *datatype, SerdNode const *lang) {
    auto const literal_value = node_into_string_view(literal);

    if (datatype != nullptr) {
        return Literal{literal_value, IRI{node_into_string_view(datatype), this->node_storage.get()}, this->node_storage.get()};
    } else if (lang != nullptr) {
        return Literal{literal_value, node_into_string_view(lang), this->node_storage.get()};
    } else {
        return Literal{literal_value, this->node_storage.get()};
    }
}

SerdStatus IStreamQuadIterator::Impl::on_error(void *voided_self, SerdError const *error) noexcept {
    auto *self = reinterpret_cast<Impl *>(voided_self);

    self->istream.col_off += error->col;

    auto const buf_sz = vsnprintf(nullptr, 0, error->fmt, *error->args);
    std::string message;
    message.resize(buf_sz);
    vsnprintf(message.data(), message.size(), error->fmt, *error->args);

    self->last_error = ParsingError{
            .error_type = parsing_error_type_from_serd(error->status),
            .line = self->istream.line_off,
            .col = self->istream.col_off,
            .message = message};

    return SerdStatus::SERD_SUCCESS;
}

SerdStatus IStreamQuadIterator::Impl::on_base(void *voided_self, const SerdNode *uri) noexcept {
    auto *self = reinterpret_cast<Impl *>(voided_self);
    self->prefixes.emplace(std::piecewise_construct,
                           std::forward_as_tuple(""),
                           std::forward_as_tuple(reinterpret_cast<char const *>(uri->buf), uri->n_bytes));
    return SERD_SUCCESS;
}

SerdStatus IStreamQuadIterator::Impl::on_prefix(void *voided_self, SerdNode const *name, SerdNode const *uri) noexcept {
    auto *self = reinterpret_cast<Impl *>(voided_self);
    self->prefixes.emplace(std::piecewise_construct,
                           std::forward_as_tuple(reinterpret_cast<char const *>(name->buf), name->n_bytes),
                           std::forward_as_tuple(reinterpret_cast<char const *>(uri->buf), uri->n_bytes));
    return SERD_SUCCESS;
}

SerdStatus IStreamQuadIterator::Impl::on_stmt(void *voided_self,
                                              SerdStatementFlags,
                                              SerdNode const *graph,
                                              SerdNode const *subj,
                                              SerdNode const *pred,
                                              SerdNode const *obj,
                                              SerdNode const *obj_datatype,
                                              SerdNode const *obj_lang) {

    auto *self = reinterpret_cast<Impl *>(voided_self);

    auto const graph_node = [&]() -> nonstd::expected<Node, SerdStatus> {
        if (graph != nullptr) {
            switch (graph->type) {
                case SERD_CURIE:
                    return self->get_prefixed_uri(pred);
                case SERD_URI:
                    return self->get_uri(subj);
                case SERD_BLANK:
                    return self->get_bnode(subj);
                default:
                    return nonstd::make_unexpected(SERD_ERR_BAD_SYNTAX);
            }
        } else {
            return IRI::default_graph();
        }
    }();

    if (!graph_node.has_value()) {
        return graph_node.error();
    }

    auto const subj_node = [&]() -> nonstd::expected<Node, SerdStatus> {
        switch (subj->type) {
            case SERD_CURIE:
                return self->get_prefixed_uri(subj);
            case SERD_URI:
                return self->get_uri(subj);
            case SERD_BLANK:
                return self->get_bnode(subj);
            default:
                return nonstd::make_unexpected(SERD_ERR_BAD_SYNTAX);
        }
    }();

    if (!subj_node.has_value()) {
        return subj_node.error();
    }

    auto const pred_node = [&]() -> nonstd::expected<Node, SerdStatus> {
        switch (pred->type) {
            case SERD_CURIE:
                return self->get_prefixed_uri(pred);
            case SERD_URI:
                return self->get_uri(pred);
            default:
                return nonstd::make_unexpected(SERD_ERR_BAD_SYNTAX);
        }
    }();

    if (!pred_node.has_value()) {
        return pred_node.error();
    }

    auto const obj_node = [&]() -> nonstd::expected<Node, SerdStatus> {
        switch (obj->type) {
            case SERD_CURIE:
                return self->get_prefixed_uri(pred);
            case SERD_LITERAL:
                return self->get_literal(obj, obj_datatype, obj_lang);
            case SERD_BLANK:
                return self->get_bnode(obj);
            case SERD_URI:
                return self->get_uri(obj);
            default:
                return nonstd::make_unexpected(SERD_ERR_BAD_SYNTAX);
        }
    }();

    if (!obj_node.has_value()) {
        return obj_node.error();
    }

    self->quad_buffer.emplace_back(*graph_node, *subj_node, *pred_node, *obj_node);
    return SERD_SUCCESS;
}


void IStreamQuadIterator::Impl::start_stream() noexcept {
    auto *read_fptr = this->end_at_first_error ? &util::IStreamAdaptor::read<false> : &util::IStreamAdaptor::read<true>;
    serd_reader_start_source_stream(this->reader.get(), read_fptr, &util::IStreamAdaptor::is_ok, &this->istream, nullptr, 1);
}

void IStreamQuadIterator::Impl::try_skip_error() {
    serd_reader_end_stream(this->reader.get());

    std::string buf;
    std::getline(this->istream.istream.get(), buf, '.');

    size_t last_newline_pos = std::string::npos;
    size_t newline_count = 0;

    for (size_t i = 0; i < buf.size(); ++i) {
        if (buf[i] == '\n') {
            last_newline_pos = i;
            newline_count += 1;
        }
    }

    this->istream.line_off += newline_count;
    this->istream.col_off += buf.size() - last_newline_pos;

    this->start_stream();
}

IStreamQuadIterator::Impl::Impl(std::istream &istream, bool strict, bool end_at_first_error, storage::node::NodeStorage &node_storage) noexcept
    : istream{std::ref(istream)},
      node_storage{node_storage},
      reader{serd_reader_new(SerdSyntax::SERD_TURTLE, this, nullptr, &Impl::on_base, &Impl::on_prefix, &Impl::on_stmt, nullptr), &Impl::destroy_serd_reader},
      end_at_first_error{end_at_first_error} {

    serd_reader_set_strict(this->reader.get(), strict);
    serd_reader_set_error_sink(this->reader.get(), &Impl::on_error, this);
    this->start_stream();
}

std::optional<nonstd::expected<Quad, ParsingError>> IStreamQuadIterator::Impl::next() {
    if (this->is_at_end()) {
        return std::nullopt;
    }

    if (this->quad_buffer.empty()) {
        serd_reader_read_chunk(this->reader.get());

        if (this->quad_buffer.empty()) {
            auto ret = nonstd::make_unexpected(this->last_error);

            if (this->end_at_first_error) {
                this->end_flag = true;
            } else {
                this->try_skip_error();
            }

            return ret;
        }
    }

    auto ret = this->quad_buffer.front();
    this->quad_buffer.pop_front();
    return ret;
}

}  // namespace rdf4cpp::rdf::parser
