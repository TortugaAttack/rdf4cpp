#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include <rdf4cpp/rdf.hpp>

using namespace rdf4cpp::rdf;

TEST_SUITE("Serialize") {
    template<typename NodeT>
    void run_ser_test(NodeT node) {
        auto expected = std::string{node};

        StringSerializer ser;
        node.serialize(ser);
        auto result = ser.finalize();

        CHECK_EQ(expected, result);
    }

    TEST_CASE("Serialize Variable") {
        SUBCASE("named") {
            auto var = query::Variable::make_named("x");
            run_ser_test(var);
        }

        SUBCASE("named") {
            auto var = query::Variable::make_anonymous("y");
            run_ser_test(var);
        }
    }

    TEST_CASE("Serialize IRI") {
        auto iri = IRI::make("http://url.com#some-iri");
        run_ser_test(iri);
    }

    TEST_CASE("Serialize BNode") {
        auto bnode = BlankNode::make("_:123abc");
        run_ser_test(bnode);
    }

    TEST_CASE("Serialize Literal") {
        using namespace rdf4cpp::rdf::datatypes;

        SUBCASE("xsd:string") {
            auto lit = Literal::make_simple("simple");
            run_ser_test(lit);
        }
        SUBCASE("non-inlined xsd:integer") {
            auto lit = Literal::make_typed_from_value<xsd::Integer>(xsd::Integer::cpp_type{"87960930222089"});
            run_ser_test(lit);
        }
        SUBCASE("inlined xsd:int") {
            auto lit = Literal::make_typed_from_value<xsd::Int>(754);
            run_ser_test(lit);
        }
        SUBCASE("rdf:langString, inlined language") {
            auto lit = Literal::make_lang_tagged("spherical cow", "en");
            run_ser_test(lit);
        }
        SUBCASE("rdf:langString, non-inlined language") {
            auto lit = Literal::make_lang_tagged("spherical cow", "nonexistent");
            run_ser_test(lit);
        }
        SUBCASE("unknown datatype") {
            auto lit = Literal::make_typed("very value", IRI::make("http://datatypes.com#dt"));
            run_ser_test(lit);
        }
    }

    TEST_CASE("Reserialize NTriples") {
        using namespace rdf4cpp::rdf::parser;

        constexpr char const *triples = "<http://url.com/subj#1> <http://url.com#pred> \"search\" .\n" // xsd:string
                                        "<http://url.com/subj#2> <http://url.com#pred> \"8796093022208\"^^<http://www.w3.org/2001/XMLSchema#integer> .\n" // non-inlined xsd:integer
                                        "<http://url.com/subj#3> <http://url.com#pred> \"123\"^^<http://www.w3.org/2001/XMLSchema#int> .\n" // inlined xsd:int
                                        "<http://url.com/subj#4> <http://url.com#pred> _:123 .\n" // blank node
                                        "<http://url.com/subj#5> <http://url.com#pred> \"Spherical Cow\"@en .\n" // rdf:langString, inlined language
                                        "<http://url.com/subj#6> <http://url.com#pred> \"Spherical Cow\"@jk .\n" // rdf:langString, non-inlined language
                                        "<http://url.com/subj#7> <http://url.com#pred> \"Spherical Cow\"^^<http://unknow-datatype.org#dt> .\n"; // unknown datatype

        std::istringstream iss{triples};
        StringSerializer ser{4096};

        for (IStreamQuadIterator qit{iss}; qit != std::default_sentinel; ++qit) {
            REQUIRE(qit->has_value());
            (*qit)->subject().serialize(ser);
            serialize_str(" ", ser);
            (*qit)->predicate().serialize(ser);
            serialize_str(" ", ser);
            (*qit)->object().serialize(ser);
            serialize_str(" .\n", ser);
        }

        auto result = ser.finalize();

        CHECK_EQ(result, std::string_view{triples});
    }
}
