#ifndef RDF4CPP_RDFNODETYPE_HPP
#define RDF4CPP_RDFNODETYPE_HPP

#include <cstdint>
#include <string>
#include <string_view>

namespace rdf4cpp::storage::identifier {
/**
 * Specifies whether an RDF node is a Variable, BlankNode, IRI or Literal.
 * The ordering BNode, IRI, Literal is the same as in SPARQL operator<.
 */
enum struct RDFNodeType : uint8_t {
    BNode = 0,
    IRI,
    Literal,
    Variable
};

constexpr std::string_view to_string_view(RDFNodeType const node_type) noexcept {
    switch (node_type) {
        case RDFNodeType::Variable:
            return "variable";
        case RDFNodeType::BNode:
            return "blank node";
        case RDFNodeType::IRI:
            return "iri";
        case RDFNodeType::Literal:
            return "literal";
        default:
            return "undefined";
    }
}

inline std::string to_string(RDFNodeType const node_type) noexcept {
    return std::string{to_string_view(node_type)};
}

}  // namespace rdf4cpp::storage::identifier


#endif  //RDF4CPP_RDFNODETYPE_HPP
