#include <rdf4cpp/rdf/parser/IStreamQuadIterator.hpp>
#include <rdf4cpp/rdf/parser/SerdIteratorImpl.ipp>

namespace rdf4cpp::rdf::parser {

bool IStreamQuadIterator::is_at_end() const {
    return this->impl == nullptr || this->impl->is_at_end();
}

IStreamQuadIterator::IStreamQuadIterator() noexcept
    : IStreamQuadIterator{std::default_sentinel} {
}

IStreamQuadIterator::IStreamQuadIterator(std::default_sentinel_t) noexcept
    : impl{nullptr} {
}

IStreamQuadIterator::IStreamQuadIterator(std::istream &istream, ParsingFlags const flags, storage::node::NodeStorage &node_storage)
    : impl{std::make_unique<Impl>(istream, flags.contains(ParsingFlag::Strict), flags.contains(ParsingFlag::StopOnFirstError), node_storage)} {

    ++*this;
}

IStreamQuadIterator::~IStreamQuadIterator() = default;

IStreamQuadIterator::reference IStreamQuadIterator::operator*() const noexcept {
    return this->cur;
}

IStreamQuadIterator::pointer IStreamQuadIterator::operator->() const noexcept {
    return &this->cur;
}

IStreamQuadIterator &IStreamQuadIterator::operator++() noexcept {
    if (auto maybe_value = this->impl->next(); maybe_value.has_value()) {
        this->cur = std::move(*maybe_value);
    }

    return *this;
}

bool IStreamQuadIterator::operator==(IStreamQuadIterator const &other) const noexcept {
    return (this->is_at_end() && other.is_at_end()) || this->impl == other.impl;
}

bool IStreamQuadIterator::operator!=(IStreamQuadIterator const &other) const noexcept {
    return !(*this == other);
}

} // namespace rdf4cpp::rdf::parser
