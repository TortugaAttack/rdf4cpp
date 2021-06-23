#include "IRIBackend.h"

namespace rdf4cpp::rdf::graph::node_storage {

IRIBackend::IRIBackend(std::string iri) : iri(std::move(iri)) {}
std::strong_ordering IRIBackend::operator<=>(const IRIBackend *other) const {
    if (other != nullptr)
        return *this <=> *other;
    else
        return std::strong_ordering::greater;
}
std::string IRIBackend::as_string(bool quoting) const {
    if (quoting)
        return "<" + iri + ">";
    else
        return iri;
}
}  // namespace rdf4cpp::rdf::graph::node_storage