#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include <doctest/doctest.h>
#include <rdf4cpp.hpp>

using namespace rdf4cpp;

/**
 * This function validates the quad pattern
 *
 * @param  graph Node which represents graph
 * @param  sub Node which represents subject
 * @param  pred Node which represents predicate
 * @param  obj Node which represents object
 * @param  result the expected value (valid or not valid) of the quad pattern
 */

static void validate_quad_pattern(Node graph, Node sub, Node pred, Node obj, bool result) {
    auto qpattern = query::QuadPattern{graph, sub, pred, obj};

    SUBCASE("Check validity & output") {
        CHECK(qpattern.graph() == graph);
        CHECK(qpattern.subject() == sub);
        CHECK(qpattern.predicate() == pred);
        CHECK(qpattern.object() == obj);

        if (result) CHECK(qpattern.valid());
        else
            CHECK(not qpattern.valid());

        //checks whether a valid quadpattern have a valid string output
        if (qpattern.valid()) std::cout << qpattern << std::endl;
    }

    SUBCASE("Check iterators") {
        auto b_itr = qpattern.begin();
        CHECK(*b_itr++ == graph);
        CHECK(*b_itr++ == sub);
        CHECK(*b_itr++ == pred);
        CHECK(*b_itr++ == obj);

        auto e_itr = qpattern.end();
        CHECK(*(--e_itr) == obj);
        CHECK(*(--e_itr) == pred);
        CHECK(*(--e_itr) == sub);
        CHECK(*(--e_itr) == graph);
    }

    SUBCASE("Check reverse-iterators") {
        auto rb_itr = qpattern.rbegin();
        CHECK(*rb_itr++ == obj);
        CHECK(*rb_itr++ == pred);
        CHECK(*rb_itr++ == sub);
        CHECK(*rb_itr++ == graph);

        auto re_itr = qpattern.rend();
        CHECK(*(--re_itr) == graph);
        CHECK(*(--re_itr) == sub);
        CHECK(*(--re_itr) == pred);
        CHECK(*(--re_itr) == obj);
    }
    //    }
}

bool valid_graph(Node node) {
    bool valid = not node.null() and
                 (node.is_iri() or node.is_variable());
    return valid;
}

bool valid_subject(Node node) {
    bool valid = not node.null() and
                 (node.is_iri() or node.is_variable() or node.is_blank_node());
    return valid;
}

bool valid_predicate(Node node) {
    bool valid = not node.null() and
                 (node.is_iri() or node.is_variable());
    return valid;
}

bool valid_object(Node node) {
    bool valid = not node.null();
    return valid;
}

TEST_CASE("QuadPattern - Check validity") {
    static std::vector<Node> nodes{
            Node{},
            IRI{"http://example.com/1"},
            IRI{"http://example.com/2"},
            BlankNode{"b1"},
            BlankNode{"b2"},
            Literal::make_simple("l1"),
            Literal::make_simple("l2"),
            query::Variable{"v1"},
            query::Variable{"v2", true},
    };
    for (const auto &graph : nodes) {
        SUBCASE((std::string{"graph: "} + ((not graph.null()) ? static_cast<std::string>(graph) : std::string{"null"})).c_str()) {
            for (const auto &subject : nodes) {
                SUBCASE((std::string{"subject: "} + ((not subject.null()) ? static_cast<std::string>(subject) : std::string{"null"})).c_str()) {
                    for (const auto &predicate : nodes) {
                        SUBCASE((std::string{"predicate: "} + ((not predicate.null()) ? static_cast<std::string>(predicate) : std::string{"null"})).c_str()) {
                            for (const auto &object : nodes) {
                                SUBCASE((std::string{"object: "} + ((not object.null()) ? static_cast<std::string>(object) : std::string{"null"})).c_str()) {
                                    bool valid = valid_graph(graph) and valid_subject(subject) and valid_predicate(predicate) and valid_object(object);
                                    validate_quad_pattern(graph, subject, predicate, object, valid);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

TEST_CASE("QuadPattern - Check for iterators and const-iterators") {

    auto variable1 = query::Variable{"g"};
    auto graph = Node{variable1};

    auto variable2 = query::Variable{"s"};
    auto sub = Node{variable2};

    auto variable3 = query::Variable{"p"};
    auto pred = Node{variable3};

    auto variable4 = query::Variable{"o"};
    auto obj = Node{variable4};

    auto variable5 = query::Variable{"g1"};
    auto graph1 = Node{variable5};

    auto variable6 = query::Variable{"s1"};
    auto sub1 = Node{variable6};

    auto variable7 = query::Variable{"p1"};
    auto pred1 = Node{variable7};

    auto variable8 = query::Variable{"o1"};
    auto obj1 = Node{variable8};

    auto qpattern = query::QuadPattern{graph, sub, pred, obj};

    query::QuadPattern::iterator itr;
    for (itr = qpattern.begin(); itr != qpattern.end(); itr++) {
        if (*itr == graph) *itr = graph1;
        else if (*itr == sub)
            *itr = sub1;
        else if (*itr == pred)
            *itr = pred1;
        else if (*itr == obj)
            *itr = obj1;
    }

    query::QuadPattern::const_reverse_iterator rb_const_itr;
    query::QuadPattern::const_reverse_iterator re_const_itr;

    SUBCASE("Check const-reverse-iterators") {
        rb_const_itr = qpattern.rbegin();
        CHECK(*rb_const_itr++ == obj1);
        CHECK(*rb_const_itr++ == pred1);
        CHECK(*rb_const_itr++ == sub1);
        CHECK(*rb_const_itr++ == graph1);

        re_const_itr = qpattern.rend();
        CHECK(*(--re_const_itr) == graph1);
        CHECK(*(--re_const_itr) == sub1);
        CHECK(*(--re_const_itr) == pred1);
        CHECK(*(--re_const_itr) == obj1);
    }

    query::QuadPattern::const_iterator b_const_itr;
    query::QuadPattern::const_iterator e_const_itr;

    SUBCASE("Check const-iterators") {
        b_const_itr = qpattern.begin();
        CHECK(*b_const_itr++ == graph1);
        CHECK(*b_const_itr++ == sub1);
        CHECK(*b_const_itr++ == pred1);
        CHECK(*b_const_itr++ == obj1);

        e_const_itr = qpattern.end();
        CHECK(*(--e_const_itr) == obj1);
        CHECK(*(--e_const_itr) == pred1);
        CHECK(*(--e_const_itr) == sub1);
        CHECK(*(--e_const_itr) == graph1);
    }
}

TEST_CASE("null-graph string repr") {
    Quad const q{Node{}, IRI{"http://test.com#s"}, IRI{"http://test.com#p"}, IRI{"http://test.com#o"}};
    CHECK_EQ(std::string{q}, "<http://test.com#s> <http://test.com#p> <http://test.com#o> .");
}
