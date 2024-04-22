#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include <doctest/doctest.h>
#include <rdf4cpp.hpp>

using namespace rdf4cpp;

TEST_CASE("decimal capabilities") {
    static_assert(datatypes::LiteralDatatype<datatypes::xsd::Decimal>);
    static_assert(datatypes::NumericLiteralDatatype<datatypes::xsd::Decimal>);
    static_assert(datatypes::LogicalLiteralDatatype<datatypes::xsd::Decimal>);
    static_assert(datatypes::PromotableLiteralDatatype<datatypes::xsd::Decimal>);
    static_assert(datatypes::xsd::Decimal::promotion_rank == 2);
    static_assert(!datatypes::SubtypedLiteralDatatype<datatypes::xsd::Decimal>);
    static_assert(datatypes::ComparableLiteralDatatype<datatypes::xsd::Decimal>);
    static_assert(datatypes::FixedIdLiteralDatatype<datatypes::xsd::Decimal>);
    static_assert(datatypes::InlineableLiteralDatatype<datatypes::xsd::Decimal>);
}

TEST_CASE("Datatype Decimal") {

    constexpr auto correct_type_iri_cstr = "http://www.w3.org/2001/XMLSchema#decimal";

    CHECK(std::string(datatypes::xsd::Decimal::identifier) == correct_type_iri_cstr);

    auto type_iri = IRI(datatypes::xsd::Decimal::identifier);
    CHECK(type_iri.identifier() == correct_type_iri_cstr);

    using type = datatypes::xsd::Decimal::cpp_type;

    std::string rdf_dbl_1_0{"1.0"};
    std::string rdf_dbl_0_0{"0.0"};

    type value = type(1.00);
    auto lit1 = Literal::make_typed_from_value<datatypes::xsd::Decimal>(value);
    CHECK(lit1.value<datatypes::xsd::Decimal>() == value);
    CHECK(lit1.lexical_form() == rdf_dbl_1_0);

    value = type(64582165456988.6235896);
    auto lit2 = Literal::make_typed_from_value<datatypes::xsd::Decimal>(value);
    CHECK(lit2.value<datatypes::xsd::Decimal>() == value);

    value = type(-64524654389.12345678);
    auto lit3 = Literal::make_typed_from_value<datatypes::xsd::Decimal>(value);
    CHECK(lit3.value<datatypes::xsd::Decimal>() == value);

    value = type(1.0);
    auto lit4 = Literal::make_typed(to_string(value), type_iri);
    CHECK(lit4.value<datatypes::xsd::Decimal>() == value);
    CHECK(lit4.lexical_form() == rdf_dbl_1_0);

    value = type{"64582165456988.235046"};
    auto lit5 = Literal::make_typed("64582165456988.235046", type_iri);
    CHECK(lit5.value<datatypes::xsd::Decimal>() == value);

    value = type{1};
    auto lit6 = Literal::make_typed("1.", type_iri);
    CHECK(lit6.value<datatypes::xsd::Decimal>() == value);
    CHECK(lit6.lexical_form() == rdf_dbl_1_0);

    auto lit7 = Literal::make_typed(rdf_dbl_1_0, type_iri);
    CHECK(lit7.value<datatypes::xsd::Decimal>() == value);
    CHECK(lit7.lexical_form() == rdf_dbl_1_0);

    auto lit8 = Literal::make_typed("1.00", type_iri);
    CHECK(lit8.value<datatypes::xsd::Decimal>() == value);
    CHECK(lit8.lexical_form() == rdf_dbl_1_0);

    value = type{std::numeric_limits<double>::max()};
    auto lit9 = Literal::make_typed(to_string(value), type_iri);
    CHECK(lit9.value<datatypes::xsd::Decimal>() == value);

    value = type{"3.111"};
    auto lit10 = Literal::make_typed_from_value<datatypes::xsd::Decimal>(value);
    CHECK(lit10.value<datatypes::xsd::Decimal>() == value);

    value = type{0};
    auto lit11 = Literal::make_typed_from_value<datatypes::xsd::Decimal>(value);
    CHECK(lit11.value<datatypes::xsd::Decimal>() == value);
    CHECK(lit11.lexical_form() == rdf_dbl_0_0);

    value = type{1.0};
    auto lit12 = Literal::make_typed("+1.0000", type_iri);
    CHECK(lit12.value<datatypes::xsd::Decimal>() == value);
    CHECK(lit12.lexical_form() == rdf_dbl_1_0);

    value = type{"0.000000005"};
    auto lit13 = Literal::make_typed_from_value<datatypes::xsd::Decimal>(value);
    CHECK(lit13.value<datatypes::xsd::Decimal>() == value);
    CHECK(lit13.lexical_form() == "0.000000005");

    value = type{6000000000.0};
    auto lit14 = Literal::make_typed_from_value<datatypes::xsd::Decimal>(value);
    CHECK(lit14.value<datatypes::xsd::Decimal>() == value);
    CHECK(lit14.lexical_form() == "6000000000.0");

    CHECK(lit1 != lit2);
    CHECK(lit2 != lit3);
    CHECK(lit1 == lit4);
    CHECK(lit6 == lit7);
    CHECK(lit6 == lit8);
    CHECK(lit7 == lit8);
    CHECK(lit1 == lit12);
    CHECK(lit4 == lit12);
    CHECK(lit6 == lit12);
    CHECK(lit7 == lit12);
    CHECK(lit8 == lit12);
    CHECK(lit6.value<datatypes::xsd::Decimal>() == lit7.value<datatypes::xsd::Decimal>());
    CHECK(lit6.value<datatypes::xsd::Decimal>() == lit8.value<datatypes::xsd::Decimal>());
    CHECK(lit7.value<datatypes::xsd::Decimal>() == lit7.value<datatypes::xsd::Decimal>());

    // suppress warnings regarding attribute ‘nodiscard’
    Literal no_discard_dummy;

    CHECK_THROWS_WITH_AS(no_discard_dummy = Literal::make_typed("NAN", type_iri), "XSD Parsing Error", std::runtime_error);

    CHECK_THROWS_WITH_AS(no_discard_dummy = Literal::make_typed("INF", type_iri), "XSD Parsing Error", std::runtime_error);

    //value = INFINITY;
    //CHECK_THROWS_WITH_AS(no_discard_dummy = Literal::make_typed(to_string(value), type_iri), "XSD Parsing Error", std::runtime_error);

    //value = NAN;
    //CHECK_THROWS_WITH_AS(no_discard_dummy = Literal::make_typed(to_string(value), type_iri), "XSD Parsing Error", std::runtime_error);

    CHECK_THROWS_WITH_AS(no_discard_dummy = Literal::make_typed("454sdsd", type_iri), "XSD Parsing Error", std::runtime_error);

    CHECK_THROWS_WITH_AS(no_discard_dummy = Literal::make_typed("2.225E-307", type_iri), "XSD Parsing Error", std::runtime_error);
}

TEST_CASE("precision") {
    // xsd:decimal requires totalDigits to be >= 18
    // see: https://www.w3.org/TR/2004/REC-xmlschema-2-20041028/#dt-decimal

    using datatypes::xsd::Decimal;
    using cpp_type = Decimal::cpp_type;

    static_assert(std::numeric_limits<cpp_type>::digits10 >= 18);

    cpp_type const i = pow(cpp_type{10}, 18) - cpp_type{1};
    cpp_type const x = i * (cpp_type{1} / pow(cpp_type{10}, 18));

    Literal const lit = Literal::make_typed_from_value<datatypes::xsd::Decimal>(x);
    CHECK(lit.lexical_form() == "0.999999999999999999");
}

TEST_CASE("Datatype Decimal buffer overread UB") {
    std::string const s = "123.456";
    std::string_view const sv{s.data(), 5};

    auto const lit = Literal::make_typed(sv, IRI{datatypes::xsd::Decimal::identifier});
    CHECK(lit.value<datatypes::xsd::Decimal>() == datatypes::xsd::Decimal::cpp_type{"123.4"});
}

TEST_CASE("decimal inlining") {
    auto l = Literal::make_typed<datatypes::xsd::Decimal>("4.2");
    CHECK(l.is_inlined());
    CHECK(static_cast<double>(l.value<datatypes::xsd::Decimal>()) == 4.2);
    l = Literal::make_typed_from_value<datatypes::xsd::Decimal>(datatypes::xsd::Decimal::cpp_type(2L << 32, 0));
    CHECK(!l.is_inlined());
    CHECK(l.value<datatypes::xsd::Decimal>() == datatypes::xsd::Decimal::cpp_type(2L << 32, 0));
}
