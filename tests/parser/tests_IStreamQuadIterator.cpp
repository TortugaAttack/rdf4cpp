#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include "rdf4cpp.hpp"
#include <rdf4cpp/storage/reference_node_storage/SyncReferenceNodeStorageBackend.hpp>

#include <iostream>

using namespace rdf4cpp::rdf;
using namespace rdf4cpp::rdf::parser;

TEST_SUITE("IStreamQuadIterator") {

    TEST_CASE("correct data") {
        constexpr char const *triples = "<http://data.semanticweb.org/workshop/admire/2012/paper/12> <http://purl.org/dc/elements/1.1/subject> \"search\" .\n"
                                        "<http://data.semanticweb.org/workshop/admire/2012/paper/12> <http://purl.org/ontology/bibo/authorList> <http://data.semanticweb.org/workshop/admire/2012/paper/12/authorlist> .\n"
                                        "<http://data.semanticweb.org/workshop/admire/2012/paper/12> <http://purl.org/dc/elements/1.1/subject> \"web applications\" .\n"
                                        "<http://data.semanticweb.org/workshop/admire/2012/paper/12> <http://xmlns.com/foaf/0.1/maker> <http://data.semanticweb.org/person/ichiro-fujinaga> .\n";

        std::istringstream iss{triples};

        size_t n = 0;
        for (auto qit = IStreamQuadIterator{iss}; qit != std::default_sentinel; ++qit) {
            CHECK(qit->has_value());
            n += 1;
        }

        CHECK_EQ(n, 4);
    }

    TEST_CASE("parse to different node storage") {
        constexpr char const *triples = "<http://data.semanticweb.org/workshop/admire/2012/paper/12> <http://purl.org/dc/elements/1.1/subject> \"search\" .\n"
                                       "<http://data.semanticweb.org/workshop/admire/2012/paper/12> <http://purl.org/ontology/bibo/authorList> <http://data.semanticweb.org/workshop/admire/2012/paper/12/authorlist> .\n"
                                       "<http://data.semanticweb.org/workshop/admire/2012/paper/12> <http://purl.org/dc/elements/1.1/subject> \"web applications\" .\n"
                                       "<http://data.semanticweb.org/workshop/admire/2012/paper/12> <http://xmlns.com/foaf/0.1/maker> <http://data.semanticweb.org/person/ichiro-fujinaga> .\n";

        std::istringstream iss{triples};

        size_t n = 0;

        storage::NodeStorage auto ns = storage::reference_node_storage::SyncReferenceNodeStorageBackend{};
        IStreamQuadIterator::state_type state{.node_storage = ns};
        for (auto qit = IStreamQuadIterator{iss, ParsingFlags::none(), &state}; qit != std::default_sentinel; ++qit) {
            CHECK(qit->has_value());

            for (auto const term : **qit) {
                CHECK_EQ(term.backend_handle().storage(), storage::DynNodeStorage{ns});
            }

            n += 1;
        }

        CHECK_EQ(n, 4);
    }

    TEST_CASE("correct data with prefix") {
        constexpr char const *triples = "@prefix ex: <http://www.example.org/> ."
                                        "ex:s1 ex:p1 ex:o1 .\n"
                                        "ex:s1 ex:p2 <http://www.example.org/o2> .\n"
                                        "ex:s2 ex:p3 \"test\" .\n";

        std::istringstream iss{triples};
        IStreamQuadIterator qit{iss};
        CHECK_NE(qit, std::default_sentinel);
        CHECK_EQ(qit->value().subject(), IRI{"http://www.example.org/s1"});
        CHECK_EQ(qit->value().predicate(), IRI{"http://www.example.org/p1"});
        CHECK_EQ(qit->value().object(), IRI{"http://www.example.org/o1"});

        ++qit;
        CHECK_NE(qit, std::default_sentinel);
        CHECK_EQ(qit->value().subject(), IRI{"http://www.example.org/s1"});
        CHECK_EQ(qit->value().predicate(), IRI{"http://www.example.org/p2"});
        CHECK_EQ(qit->value().object(), IRI{"http://www.example.org/o2"});

        ++qit;
        CHECK_NE(qit, std::default_sentinel);
        CHECK_EQ(qit->value().subject(), IRI{"http://www.example.org/s2"});
        CHECK_EQ(qit->value().predicate(), IRI{"http://www.example.org/p3"});
        CHECK_EQ(qit->value().object(), Literal::make_simple("test"));

        ++qit;
        CHECK_EQ(qit, std::default_sentinel);
    }

    TEST_CASE("continue after error") {
        constexpr char const *triples = "<http://data.semanticweb.org/workshop/admire/2012/paper/12> <http://purl.org/dc/elements/1.1/subject> \"search\"^^<http://www.w3.org/2001/XMLSchema#string> .\n"
                                        "<http://data.semanticweb.org/workshop/admire/2012/paper/12> <http://purl.org/ontology/bibo/authorList> <http://data.semanticweb.org/workshop/admire/2012/paper/12/authorlist> .\n"
                                        "<http://data.semanticweb.org/workshop/admire/2012/paper/12> http://purl.org/dc/elements/1.1/subject> \"web applications\" .\n"
                                        "<http://data.semanticweb.org/workshop/admire/2012/paper/12> <http://xmlns.com/foaf/0.1/maker> <http://data.semanticweb.org/person/ichiro-fujinaga> .\n";

        SUBCASE("strict") {
            std::istringstream iss{triples};
            IStreamQuadIterator qit{iss, ParsingFlag::Strict};

            CHECK_NE(qit, std::default_sentinel);
            CHECK_EQ(qit->value().subject(), IRI{"http://data.semanticweb.org/workshop/admire/2012/paper/12"});
            CHECK_EQ(qit->value().predicate(), IRI{"http://purl.org/dc/elements/1.1/subject"});
            CHECK_EQ(qit->value().object(), Literal::make_simple("search"));

            ++qit;
            CHECK_NE(qit, std::default_sentinel);
            CHECK_EQ(qit->value().subject(), IRI{"http://data.semanticweb.org/workshop/admire/2012/paper/12"});
            CHECK_EQ(qit->value().predicate(), IRI{"http://purl.org/ontology/bibo/authorList"});
            CHECK_EQ(qit->value().object(), IRI{"http://data.semanticweb.org/workshop/admire/2012/paper/12/authorlist"});

            ++qit;
            CHECK_NE(qit, std::default_sentinel);
            CHECK(!qit->has_value());
            std::cerr << qit->error() << std::endl;

            ++qit;
            CHECK_NE(qit, std::default_sentinel);
            CHECK_EQ(qit->value().subject(), IRI{"http://data.semanticweb.org/workshop/admire/2012/paper/12"});
            CHECK_EQ(qit->value().predicate(), IRI{"http://xmlns.com/foaf/0.1/maker"});
            CHECK_EQ(qit->value().object(), IRI{"http://data.semanticweb.org/person/ichiro-fujinaga"});

            ++qit;
            CHECK_EQ(qit, std::default_sentinel);
        }

        SUBCASE("non-strict") {
            std::istringstream iss{triples};
            IStreamQuadIterator qit{iss};

            CHECK_NE(qit, std::default_sentinel);
            CHECK_EQ(qit->value().subject(), IRI{"http://data.semanticweb.org/workshop/admire/2012/paper/12"});
            CHECK_EQ(qit->value().predicate(), IRI{"http://purl.org/dc/elements/1.1/subject"});
            CHECK_EQ(qit->value().object(), Literal::make_simple("search"));

            ++qit;
            CHECK_NE(qit, std::default_sentinel);
            CHECK_EQ(qit->value().subject(), IRI{"http://data.semanticweb.org/workshop/admire/2012/paper/12"});
            CHECK_EQ(qit->value().predicate(), IRI{"http://purl.org/ontology/bibo/authorList"});
            CHECK_EQ(qit->value().object(), IRI{"http://data.semanticweb.org/workshop/admire/2012/paper/12/authorlist"});

            ++qit;
            CHECK_NE(qit, std::default_sentinel);
            CHECK(!qit->has_value());
            std::cerr << qit->error() << std::endl;

            ++qit;
            CHECK_NE(qit, std::default_sentinel);
            CHECK_EQ(qit->value().subject(), IRI{"http://data.semanticweb.org/workshop/admire/2012/paper/12"});
            CHECK_EQ(qit->value().predicate(), IRI{"http://xmlns.com/foaf/0.1/maker"});
            CHECK_EQ(qit->value().object(), IRI{"http://data.semanticweb.org/person/ichiro-fujinaga"});

            ++qit;
            CHECK_EQ(qit, std::default_sentinel);
        }
    }

    TEST_CASE("continue after error turtle") {
        constexpr char const *triples = "@prefix rdf: <http://www.w3.org/1999/02/22-rdf-syntax-ns#> .\n"
                                        "@prefix dc: <http://purl.org/dc/elements/1.1/> .\n"
                                        "@prefix ex: <http://example.org/stuff/1.0/> .\n"
                                        "\n"
                                        "<http://www.w3.org/TR/rdf-syntax-grammar>\n"
                                        "  dc:title \"RDF/XML Syntax Specification (Revised)\" ;\n"
                                        "  ex:editor [\n"
                                        "    ERRORHERE \"Dave Beckett\";\n"
                                        "    ex:homePage <http://purl.org/net/dajobe/>\n"
                                        "  ] .\n"
                                        "\n"
                                        "<http://www.w3.org/TR/rdf-syntax-grammar>\n"
                                        "  dc:title \"RDF/XML Syntax Specification (Revised)\" ;\n"
                                        "  ex:editor [\n"
                                        "    ex:fullname \"Dave Beckett\";\n"
                                        "  ] .";

        std::istringstream iss{triples};
        IStreamQuadIterator qit{iss, ParsingFlag::KeepBlankNodeIds};

        CHECK_NE(qit, std::default_sentinel);
        CHECK(qit->value() == Quad{IRI{"http://www.w3.org/TR/rdf-syntax-grammar"},
                                   IRI{"http://purl.org/dc/elements/1.1/title"},
                                   Literal::make_typed_from_value<datatypes::xsd::String>("RDF/XML Syntax Specification (Revised)")});


        ++qit;
        CHECK_NE(qit, std::default_sentinel);
        CHECK(qit->value() == Quad{IRI{"http://www.w3.org/TR/rdf-syntax-grammar"},
                                   IRI{"http://example.org/stuff/1.0/editor"},
                                   BlankNode{"b1"}});

        // error here, rest of this spec gets dropped because parsing state is now messed up
        ++qit;
        CHECK(!qit->has_value());

        // second error is a result of parsing state being messed up
        ++qit;
        CHECK(!qit->has_value());

        // start of new spec
        ++qit;
        CHECK_NE(qit, std::default_sentinel);
        CHECK(qit->value() == Quad{IRI{"http://www.w3.org/TR/rdf-syntax-grammar"},
                                   IRI{"http://purl.org/dc/elements/1.1/title"},
                                   Literal::make_typed_from_value<datatypes::xsd::String>("RDF/XML Syntax Specification (Revised)")});

        ++qit;
        CHECK_NE(qit, std::default_sentinel);
        CHECK(qit->value() == Quad{IRI{"http://www.w3.org/TR/rdf-syntax-grammar"},
                                   IRI{"http://example.org/stuff/1.0/editor"},
                                   BlankNode{"b2"}});

        ++qit;
        CHECK_NE(qit, std::default_sentinel);
        CHECK(qit->value() == Quad{BlankNode{"b2"},
                                   IRI{"http://example.org/stuff/1.0/fullname"},
                                   Literal::make_typed_from_value<datatypes::xsd::String>("Dave Beckett")});

        ++qit;
        CHECK_EQ(qit, std::default_sentinel);
    }

    TEST_CASE("invalid literal") {
        constexpr char const *triples = "<http://data.semanticweb.org/workshop/admire/2012/paper/12> <http://purl.org/dc/elements/1.1/subject> \"search\"^^<http://www.w3.org/2001/XMLSchema#int> .\n";

        std::istringstream iss{triples};
        IStreamQuadIterator qit{iss};

        CHECK_NE(qit, std::default_sentinel);
        CHECK(!qit->has_value());
        std::cerr << qit->error() << std::endl;

        ++qit;
        CHECK_EQ(qit, std::default_sentinel);
    }

    TEST_CASE("unknown prefix") {
        constexpr char const *triples = "<http://data.semanticweb.org/workshop/admire/2012/paper/12> prefix:predicate \"search\"^^<http://www.w3.org/2001/XMLSchema#string> .\n";

        std::istringstream iss{triples};
        IStreamQuadIterator qit{iss};

        CHECK_NE(qit, std::default_sentinel);
        CHECK(!qit->has_value());
        std::cerr << qit->error() << std::endl;

        ++qit;
        CHECK_EQ(qit, std::default_sentinel);
    }

    TEST_CASE("curie as literal type") {
        constexpr char const *triples = "@prefix xsd: <http://some-random-url.de#> .\n"
                                        "<http://data.semanticweb.org/workshop/admire/2012/paper/12> <http://purl.org/dc/elements/1.1/subject> \"search\"^^xsd:string .\n";

        std::istringstream iss{triples};
        IStreamQuadIterator qit{iss};

        CHECK_NE(qit, std::default_sentinel);
        CHECK(qit->has_value());
        std::cerr << qit->value() << std::endl;

        ++qit;
        CHECK_EQ(qit, std::default_sentinel);
    }

    TEST_CASE("manually added prefix") {
        constexpr char const *triples = "<http://data.semanticweb.org/workshop/admire/2012/paper/12> prefix:predicate \"search\"^^<http://www.w3.org/2001/XMLSchema#string> .\n";

        std::istringstream iss{triples};
        IStreamQuadIterator::state_type state{
                .iri_factory = IRIFactory{ IRIFactory::prefix_map_type {{"prefix", "https://hello.com#"}} }};

        IStreamQuadIterator qit{iss, ParsingFlags::none(), &state};

        CHECK_NE(qit, std::default_sentinel);
        CHECK(qit->has_value());
        CHECK(qit->value() == Quad{IRI{"http://data.semanticweb.org/workshop/admire/2012/paper/12"},
                                   IRI{"https://hello.com#predicate"},
                                   Literal::make_simple("search")});

        ++qit;
        CHECK_EQ(qit, std::default_sentinel);
    }

    TEST_CASE("no prefix parsing") {
        auto run = [](ParsingFlags flags) {
            constexpr char const *triples = "@base <http://invalid-url.org> .\n"
                                            "@prefix xsd: <http://some-random-url.de#> .\n"
                                            "<http://data.semanticweb.org/workshop/admire/2012/paper/12> <http://purl.org/dc/elements/1.1/subject> \"search\" .\n"
                                            "xsd:subject xsd:predicate \"aaaaa\" .\n";

            std::istringstream iss{triples};
            IStreamQuadIterator qit{iss, flags};

            CHECK_NE(qit, std::default_sentinel);
            CHECK(!qit->has_value());
            std::cout << qit->error() << std::endl;

            ++qit;
            CHECK_NE(qit, std::default_sentinel);
            CHECK(!qit->has_value());
            std::cout << qit->error() << std::endl;

            ++qit;
            CHECK_NE(qit, std::default_sentinel);
            CHECK(qit->has_value());
            CHECK(qit->value() == Quad{IRI{"http://data.semanticweb.org/workshop/admire/2012/paper/12"},
                                       IRI{"http://purl.org/dc/elements/1.1/subject"},
                                       Literal::make_simple("search")});

            ++qit;
            CHECK_NE(qit, std::default_sentinel);
            CHECK(!qit->has_value());
            std::cout << qit->error() << std::endl;

            ++qit;
            CHECK_EQ(qit, std::default_sentinel);
        };

        SUBCASE("No parse prefix") {
            run(ParsingFlag::NoParsePrefix);
        }

        SUBCASE("ntriples") {
            run(ParsingFlag::NTriples);
        }

        SUBCASE("nquads") {
            run(ParsingFlag::NQuads);
        }
    }

    TEST_CASE("no bnode parsing") {
        constexpr char const *triples = "<a> <b> _:bnode .\n";

        std::istringstream iss{triples};
        IStreamQuadIterator qit{iss, ParsingFlag::NoParseBlankNode};

        CHECK_NE(qit, std::default_sentinel);
        CHECK(!qit->has_value());
        CHECK(qit->error().message.find("Encountered blank node while parsing") != std::string::npos);
        std::cout << qit->error() << std::endl;

        ++qit;
        CHECK_EQ(qit, std::default_sentinel);
    }

    TEST_CASE("relative IRIs") {
        // more complex test cases in tests_IRIFactory
        constexpr char const *triples = "@base <http://invalid-url.org> .\n"
                                        "@prefix xsd: </foo/> .\n"
                                        "<http://data.semanticweb.org/workshop/admire/2012/paper/12> <http://purl.org/dc/elements/1.1/subject> </bar> .\n"
                                        "xsd:subject xsd:predicate \"aaaaa\" .\n"
                                        "@base </definitely-relative> . \n";

        std::istringstream iss{triples};
        IStreamQuadIterator qit{iss};

        CHECK(qit != std::default_sentinel);
        CHECK(qit->has_value());
        CHECK(qit->value() == Quad{IRI{"http://data.semanticweb.org/workshop/admire/2012/paper/12"},
                                   IRI{"http://purl.org/dc/elements/1.1/subject"},
                                   IRI{"http://invalid-url.org/bar"}});

        ++qit;

        CHECK(qit != std::default_sentinel);
        CHECK(qit->has_value());
        CHECK(qit->value() == Quad{IRI{"http://invalid-url.org/foo/subject"},
                                   IRI{"http://invalid-url.org/foo/predicate"},
                                   Literal::make_simple("aaaaa")});

        ++qit;

        CHECK(qit != std::default_sentinel);
        CHECK(!qit->has_value());
        std::cout << qit->error() << std::endl;

        ++qit;
        CHECK(qit == std::default_sentinel);
    }

    TEST_CASE("garbage input") {
        std::istringstream iss{"}; SELECT * WHERE { ?s ?p ?o "};

        size_t count = 0;
        for (IStreamQuadIterator qit{iss, ParsingFlag::NoParsePrefix | ParsingFlag::KeepBlankNodeIds}; qit != std::default_sentinel; ++qit) {
            CHECK_FALSE(qit->has_value());
            ++count;
        }

        CHECK_EQ(count, 1);
    }

    TEST_CASE("empty input") {
        SUBCASE("istream") {
            std::istringstream iss{""};

            for (IStreamQuadIterator qit{iss}; qit != std::default_sentinel; ++qit) {
                FAIL("not empty");
            }
        }

        SUBCASE("fopen") {
            static constexpr char const *path = "/tmp/rdf4cpp-istreamquad-iter-empty";

            {
                auto *f = fopen(path, "w");
                fclose(f);
            }

            auto *f = fopen(path, "r");
            for (IStreamQuadIterator qit{f, reinterpret_cast<ReadFunc>(fread), reinterpret_cast<ErrorFunc>(ferror)}; qit != std::default_sentinel; ++qit) {
                FAIL("not empty");
            }

            fclose(f);
            remove(path);
        }
    }

    TEST_CASE("N-Triple") {
        SUBCASE("simple") {
            std::stringstream str{"<http://example/s> <http://example/p> \"string\" ."};

            IStreamQuadIterator it{str, ParsingFlag::NTriples};

            CHECK_NE(it, std::default_sentinel);
            CHECK(it->has_value());
            CHECK(it->value() == Quad{IRI{"http://example/s"},
                                      IRI{"http://example/p"},
                                      Literal::make_simple("string")});

            ++it;
            CHECK_EQ(it, std::default_sentinel);
        }
        SUBCASE("Turtle") {
            constexpr char const *triples = "@base <http://invalid-url.org> .\n"
                                            "@prefix xsd: <http://some-random-url.de#> .\n"
                                            "<http://data.semanticweb.org/workshop/admire/2012/paper/12> <http://purl.org/dc/elements/1.1/subject> \"search\" .\n"
                                            "xsd:subject xsd:predicate \"aaaaa\" .\n";

            std::istringstream iss{triples};
            IStreamQuadIterator qit{iss, ParsingFlag::NTriples};

            CHECK_NE(qit, std::default_sentinel);
            CHECK(!qit->has_value());
            std::cout << qit->error() << std::endl;

            ++qit;
            CHECK_NE(qit, std::default_sentinel);
            CHECK(!qit->has_value());
            std::cout << qit->error() << std::endl;

            ++qit;
            CHECK_NE(qit, std::default_sentinel);
            CHECK(qit->has_value());
            CHECK(qit->value() == Quad{IRI{"http://data.semanticweb.org/workshop/admire/2012/paper/12"},
                                       IRI{"http://purl.org/dc/elements/1.1/subject"},
                                       Literal::make_simple("search")});

            ++qit;
            CHECK_NE(qit, std::default_sentinel);
            CHECK(!qit->has_value());
            std::cout << qit->error() << std::endl;

            ++qit;
            CHECK_EQ(qit, std::default_sentinel);
        }
    }

    TEST_CASE("N-Quads") {
        SUBCASE("simple") {
            std::stringstream str{"<http://example/s> <http://example/p> <http://example/o> <http://example/g> .\n"
                                  "<http://example/s> <http://example/p> <http://example/o>."};

            IStreamQuadIterator it{str, ParsingFlag::NQuads};

            CHECK_NE(it, std::default_sentinel);
            CHECK(it->has_value());
            CHECK(it->value() == Quad{IRI{"http://example/g"},
                                      IRI{"http://example/s"},
                                      IRI{"http://example/p"},
                                      IRI{"http://example/o"}});

            ++it;
            CHECK_NE(it, std::default_sentinel);
            CHECK(it->has_value());
            CHECK(it->value() == Quad{IRI{"http://example/s"},
                                      IRI{"http://example/p"},
                                      IRI{"http://example/o"}});

            ++it;
            CHECK_EQ(it, std::default_sentinel);
        }
        SUBCASE("disallow prefixes") {
            constexpr char const *triples = "@base <http://invalid-url.org> .\n"
                                            "@prefix xsd: <http://some-random-url.de#> .\n"
                                            "<http://data.semanticweb.org/workshop/admire/2012/paper/12> <http://purl.org/dc/elements/1.1/subject> \"search\" <http://example.org/g> .\n"
                                            "xsd:subject xsd:predicate \"aaaaa\" <http://example.org/g> .\n";

            std::istringstream iss{triples};
            IStreamQuadIterator qit{iss, ParsingFlag::NQuads};

            CHECK_NE(qit, std::default_sentinel);
            CHECK(!qit->has_value());
            std::cout << qit->error() << std::endl;

            ++qit;
            CHECK_NE(qit, std::default_sentinel);
            CHECK(!qit->has_value());
            std::cout << qit->error() << std::endl;

            ++qit;
            CHECK_NE(qit, std::default_sentinel);
            CHECK(qit->has_value());
            CHECK(qit->value() == Quad{IRI{"http://example.org/g"},
                                       IRI{"http://data.semanticweb.org/workshop/admire/2012/paper/12"},
                                       IRI{"http://purl.org/dc/elements/1.1/subject"},
                                       Literal::make_simple("search")});

            ++qit;
            CHECK_NE(qit, std::default_sentinel);
            CHECK(!qit->has_value());
            std::cout << qit->error() << std::endl;

            ++qit;
            CHECK_EQ(qit, std::default_sentinel);
        }
    }

    TEST_CASE("TriG") {
        SUBCASE("simple") {
            std::stringstream str{"<http://example/g> {<http://example/s> <http://example/p> <http://example/o> .}"};

            IStreamQuadIterator it{str, ParsingFlag::TriG};

            CHECK_NE(it, std::default_sentinel);
            CHECK(it->has_value());
            CHECK_EQ(it->value(), Quad{IRI{"http://example/g"},
                                       IRI{"http://example/s"},
                                       IRI{"http://example/p"},
                                       IRI{"http://example/o"}});

            ++it;
            CHECK_EQ(it, std::default_sentinel);
        }
        SUBCASE("mixed") {
            std::stringstream str{"@prefix p: <http://a.example/>.\n"
                                  "{p:s <http://a.example/p> p:o .}\n"
                                  "<http://example/graph> {<http://a.example/s> <http://a.example/p> <http://a.example/o> .}"};

            IStreamQuadIterator it{str, ParsingFlag::TriG};

            CHECK_NE(it, std::default_sentinel);
            CHECK(it->has_value());
            CHECK_EQ(it->value(), Quad{IRI{"http://a.example/s"},
                                       IRI{"http://a.example/p"},
                                       IRI{"http://a.example/o"}});
            ++it;
            CHECK_NE(it, std::default_sentinel);
            CHECK(it->has_value());
            CHECK_EQ(it->value(), Quad{IRI{"http://example/graph"},
                                       IRI{"http://a.example/s"},
                                       IRI{"http://a.example/p"},
                                       IRI{"http://a.example/o"}});

            ++it;
            CHECK_EQ(it, std::default_sentinel);
        }
    }

    TEST_CASE("bnode management") {
        constexpr char const *triples = "<http://data.semanticweb.org/workshop/admire/2012/paper/12> <http://purl.org/dc/elements/1.1/subject> _:b1 .\n"
                                        "<http://data.semanticweb.org/workshop/admire/2012/paper/13> <http://purl.org/dc/elements/1.1/subject> _:b1 .\n"
                                        "_:b2 <http://purl.org/dc/elements/1.1/subject> \"Some Subject\" .\n";

        SUBCASE("bnodes") {
            IStreamQuadIterator::state_type state{
                    .blank_node_generator = &bnode_mngt::NodeGenerator::default_instance(),
                    .blank_node_scope_manager = &bnode_mngt::ReferenceNodeScopeManager::default_instance()};

            std::istringstream iss{triples};
            IStreamQuadIterator qit{iss, ParsingFlags::none(), &state};

            CHECK(qit != std::default_sentinel);
            CHECK(qit->has_value());
            CHECK(qit->value().subject() == IRI{"http://data.semanticweb.org/workshop/admire/2012/paper/12"});
            CHECK(qit->value().predicate() == IRI{"http://purl.org/dc/elements/1.1/subject"});

            auto b1_1 = qit->value().object();
            CHECK(b1_1.is_blank_node());
            std::cout << qit->value().object() << std::endl;

            ++qit;
            CHECK(qit != std::default_sentinel);
            CHECK(qit->value().subject() == IRI{"http://data.semanticweb.org/workshop/admire/2012/paper/13"});
            CHECK(qit->value().predicate() == IRI{"http://purl.org/dc/elements/1.1/subject"});

            auto b1_2 = qit->value().object();
            CHECK(b1_1.is_blank_node());
            std::cout << qit->value().object() << std::endl;
            CHECK(b1_1 == b1_2);

            ++qit;
            CHECK(qit != std::default_sentinel);
            auto b2_1 = qit->value().subject();
            CHECK(b2_1.is_blank_node());
            CHECK(b2_1 != b1_1);
            std::cout << b2_1 << std::endl;
            CHECK(qit->value().predicate() == IRI{"http://purl.org/dc/elements/1.1/subject"});
            CHECK(qit->value().object() == Literal::make_simple("Some Subject"));

            ++qit;
            CHECK(qit == std::default_sentinel);
        }

        SUBCASE("skolem iris") {
            auto scope_mng = bnode_mngt::ReferenceNodeScopeManager{};
            auto generator = bnode_mngt::NodeGenerator::new_instance_with_factory<bnode_mngt::SkolemIRIFactory>("http://skolem-iris.org#");

            IStreamQuadIterator::state_type state{.blank_node_generator = &generator,
                                                  .blank_node_scope_manager = &scope_mng};

            std::istringstream iss{triples};
            IStreamQuadIterator qit{iss, ParsingFlags::none(), &state};

            CHECK(qit != std::default_sentinel);
            CHECK(qit->has_value());
            CHECK(qit->value().subject() == IRI{"http://data.semanticweb.org/workshop/admire/2012/paper/12"});
            CHECK(qit->value().predicate() == IRI{"http://purl.org/dc/elements/1.1/subject"});

            auto b1_1 = qit->value().object();
            CHECK(b1_1.is_iri());
            std::cout << qit->value().object() << std::endl;

            ++qit;
            CHECK(qit != std::default_sentinel);
            CHECK(qit->value().subject() == IRI{"http://data.semanticweb.org/workshop/admire/2012/paper/13"});
            CHECK(qit->value().predicate() == IRI{"http://purl.org/dc/elements/1.1/subject"});

            auto b1_2 = qit->value().object();
            CHECK(b1_1.is_iri());
            std::cout << qit->value().object() << std::endl;
            CHECK(b1_1 == b1_2);

            ++qit;
            CHECK(qit != std::default_sentinel);
            auto b2_1 = qit->value().subject();
            CHECK(b2_1.is_iri());
            CHECK(b2_1 != b1_1);
            std::cout << b2_1 << std::endl;
            CHECK(qit->value().predicate() == IRI{"http://purl.org/dc/elements/1.1/subject"});
            CHECK(qit->value().object() == Literal::make_simple("Some Subject"));

            ++qit;
            CHECK(qit == std::default_sentinel);
        }
    }

}
