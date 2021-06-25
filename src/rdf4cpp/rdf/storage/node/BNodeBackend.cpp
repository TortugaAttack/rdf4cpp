
#include "BNodeBackend.hpp"

namespace rdf4cpp::rdf::storage::node {

BNodeBackend::BNodeBackend(std::string identifier) : identifier_(std::move(identifier)) {}
std::strong_ordering BNodeBackend::operator<=>(const BNodeBackend *other) const {
    if (other != nullptr)
        return *this <=> *other;
    else
        return std::strong_ordering::greater;
}
std::string BNodeBackend::as_string([[maybe_unused]] bool quoting) const {
    return "_:" + identifier_;
}
const std::string &BNodeBackend::indentifier() const {
    return identifier_;
}
}  // namespace rdf4cpp::rdf::storage::node