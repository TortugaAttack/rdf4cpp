#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <rdf4cpp/rdf.hpp>
#include <rdf4cpp/rdf/util/CharMatcher.hpp>
#include <array>

using namespace rdf4cpp::rdf;

TEST_CASE("IRIView") {
    CHECK(IRIView{"example"}.is_relative() == true);
    CHECK(IRIView{"example/foo"}.is_relative() == true);
    CHECK(IRIView{"http://example"}.is_relative() == false);

    CHECK(IRIView{"http://example"}.scheme() == "http");
    CHECK(IRIView{"example/foo"}.scheme() == std::nullopt);
    CHECK(IRIView{":example/foo"}.scheme() == "");

    CHECK(IRIView{"http://example"}.authority() == "example");
    CHECK(IRIView{"http://example/foo"}.authority() == "example");
    CHECK(IRIView{"//example?foo"}.authority() == "example");
    CHECK(IRIView{"http://example#foo"}.authority() == "example");
    CHECK(IRIView{"http:/example"}.authority() == std::nullopt);
    CHECK(IRIView{"http:///foo"}.authority() == "");

    CHECK(IRIView{"http://example"}.path() == "");
    CHECK(IRIView{"http://example/"}.path() == "/");
    CHECK(IRIView{"http://example/a/b/c"}.path() == "/a/b/c");
    CHECK(IRIView{"http://example/a/b/c#d"}.path() == "/a/b/c");
    CHECK(IRIView{"http://example/a/b/c?q"}.path() == "/a/b/c");
    CHECK(IRIView{"abc"}.path() == "abc");
    CHECK(IRIView{"/abc"}.path() == "/abc");
    CHECK(IRIView{"http:abc"}.path() == "abc");
    CHECK(IRIView{"http:/abc"}.path() == "/abc");

    CHECK(IRIView{"http://example?q"}.query() == "q");
    CHECK(IRIView{"http://example/a/b/c?q/d"}.query() == "q/d");
    CHECK(IRIView{"http://example/a/b/c?q/d#f"}.query() == "q/d");
    CHECK(IRIView{"http://example/a/b/c"}.query() == std::nullopt);
    CHECK(IRIView{"http://example/a/b/c#f"}.query() == std::nullopt);
    CHECK(IRIView{"http://example/a/b/c?"}.query() == "");

    CHECK(IRIView{"http://example#f"}.fragment() == "f");
    CHECK(IRIView{"http://example/a/b/c?q/d#f"}.fragment() == "f");
    CHECK(IRIView{"http://example/a/b/c?q/d#"}.fragment() == "");
    CHECK(IRIView{"http://example/a/b/c?q/d"}.fragment() == std::nullopt);

    CHECK(IRIView{"http://example#f"}.to_absolute() == "http://example");
    CHECK(IRIView{"http://example"}.to_absolute() == "http://example");

    CHECK(IRIView{"htt-p://example/path?query#frag"}.quick_validate() == IRIFactoryError::Ok);
    CHECK(IRIView{"htt-p:foo/bar"}.quick_validate() == IRIFactoryError::Ok);
    CHECK(IRIView{"htt-p:#frag"}.quick_validate() == IRIFactoryError::Ok);
    CHECK(IRIView{"example"}.quick_validate() == IRIFactoryError::Relative);
    CHECK(IRIView{"htt?p://example"}.quick_validate() == IRIFactoryError::InvalidScheme);
    CHECK(IRIView{"http://user@example:123"}.quick_validate() == IRIFactoryError::Ok);
    CHECK(IRIView{"http://user@exa@mple:123"}.quick_validate() == IRIFactoryError::InvalidHost);
    CHECK(IRIView{"http://us]er@example:123"}.quick_validate() == IRIFactoryError::InvalidUserinfo);
    CHECK(IRIView{"http://us:er@example:123"}.quick_validate() == IRIFactoryError::Ok);
    CHECK(IRIView{"http://user@example:12a3"}.quick_validate() == IRIFactoryError::InvalidPort);
    CHECK(IRIView{"htt-p://example/pat[h"}.quick_validate() == IRIFactoryError::InvalidPath);
    CHECK(IRIView{"htt-p://example/path?que]ry#frag"}.quick_validate() == IRIFactoryError::InvalidQuery);
    CHECK(IRIView{"htt-p://example/path?query#fra]g"}.quick_validate() == IRIFactoryError::InvalidFragment);
    CHECK(IRIView{"htt-p://exa\U0001f34cmple/pa\U0001f34cth?que\U0001f34cry#fra\U0001f34cg"}.quick_validate() == IRIFactoryError::Ok);
    CHECK(IRIView{"htt\U0001f34cp://example"}.quick_validate() == IRIFactoryError::InvalidScheme);
    CHECK(IRIView{"http://[FEDC:BA98:7654:3210:FEDC:BA98:7654:3210]:80/index.html"}.quick_validate() == IRIFactoryError::Ok);

    // from https://datatracker.ietf.org/doc/html/rfc3986#section-3
    auto [scheme, auth, path, query, frag] = IRIView{"foo://example.com:8042/over/there?name=ferret#nose"}.all_parts();
    CHECK(scheme == "foo");
    CHECK(auth == "example.com:8042");
    CHECK(path == "/over/there");
    CHECK(query == "name=ferret");
    CHECK(frag == "nose");

    CHECK(IRIView{"http://user@example:1234/foo"}.userinfo() == "user");
    CHECK(IRIView{"http://@example:1234/foo"}.userinfo() == "");
    CHECK(IRIView{"http://example:1234/foo"}.userinfo() == std::nullopt);
    CHECK(IRIView{"http://user@example:1234/foo"}.host() == "example");
    CHECK(IRIView{"http://example:1234/foo"}.host() == "example");
    CHECK(IRIView{"http://user@example/foo"}.host() == "example");
    CHECK(IRIView{"http://example/foo"}.host() == "example");
    CHECK(IRIView{"http://user@example:1234/foo"}.port() == "1234");
    CHECK(IRIView{"http://user@example:/foo#:"}.port() == "");
    CHECK(IRIView{"http://user@example/foo"}.port() == std::nullopt);
    CHECK(IRIView{"http:foo"}.userinfo() == std::nullopt);
    CHECK(IRIView{"http:foo"}.host() == std::nullopt);
    CHECK(IRIView{"http:foo"}.port() == std::nullopt);

    auto [userinfo, host, port] = IRIView{"http://user@example:1234/foo"}.all_authority_parts();
    CHECK(userinfo == "user");
    CHECK(host == "example");
    CHECK(port == "1234");

    // from https://datatracker.ietf.org/doc/html/rfc2732#section-2
    CHECK(IRIView{"http://[FEDC:BA98:7654:3210:FEDC:BA98:7654:3210]:80/index.html"}.host() == "[FEDC:BA98:7654:3210:FEDC:BA98:7654:3210]");
    CHECK(IRIView{"http://[FEDC:BA98:7654:3210:FEDC:BA98:7654:3210]:80/index.html"}.port() == "80");
    CHECK(IRIView{"http://[FEDC:BA98:7654:3210:FEDC:BA98:7654:3210]/index.html"}.host() == "[FEDC:BA98:7654:3210:FEDC:BA98:7654:3210]");
    CHECK(IRIView{"http://[FEDC:BA98:7654:3210:FEDC:BA98:7654:3210]/index.html"}.port() == std::nullopt);
}

TEST_CASE("base") {
    // Test cases from: https://datatracker.ietf.org/doc/html/rfc3986#section-5.4
    // these are for URIs, but each URI is also a IRI

    IRIFactory fact{"http://a/b/c/d;p?q"};

    // normal
    CHECK(fact.from_relative("g:h").value().identifier() == "g:h");
    CHECK(fact.from_relative("g").value().identifier() == "http://a/b/c/g");
    CHECK(fact.from_relative("./g").value().identifier() == "http://a/b/c/g");
    CHECK(fact.from_relative("g/").value().identifier() == "http://a/b/c/g/");
    CHECK(fact.from_relative("/g").value().identifier() == "http://a/g");
    CHECK(fact.from_relative("//g").value().identifier() == "http://g");
    CHECK(fact.from_relative("?y").value().identifier() == "http://a/b/c/d;p?y");
    CHECK(fact.from_relative("g?y").value().identifier() == "http://a/b/c/g?y");
    CHECK(fact.from_relative("#s").value().identifier() == "http://a/b/c/d;p?q#s");
    CHECK(fact.from_relative("g#s").value().identifier() == "http://a/b/c/g#s");
    CHECK(fact.from_relative("g?y#s").value().identifier() == "http://a/b/c/g?y#s");
    CHECK(fact.from_relative(";x").value().identifier() == "http://a/b/c/;x");
    CHECK(fact.from_relative("g;x").value().identifier() == "http://a/b/c/g;x");
    CHECK(fact.from_relative("g;x?y#s").value().identifier() == "http://a/b/c/g;x?y#s");
    CHECK(fact.from_relative("").value().identifier() == "http://a/b/c/d;p?q");
    CHECK(fact.from_relative(".").value().identifier() == "http://a/b/c/");
    CHECK(fact.from_relative("./").value().identifier() == "http://a/b/c/");
    CHECK(fact.from_relative("..").value().identifier() == "http://a/b/");
    CHECK(fact.from_relative("../").value().identifier() == "http://a/b/");
    CHECK(fact.from_relative("../g").value().identifier() == "http://a/b/g");
    CHECK(fact.from_relative("../..").value().identifier() == "http://a/");
    CHECK(fact.from_relative("../../").value().identifier() == "http://a/");
    CHECK(fact.from_relative("../../g").value().identifier() == "http://a/g");

    //abnormal
    // more dots
    CHECK(fact.from_relative("../../../g").value().identifier() == "http://a/g");
    CHECK(fact.from_relative("../../../../g").value().identifier() == "http://a/g");
    // not dots
    CHECK(fact.from_relative("/./g").value().identifier() == "http://a/g");
    CHECK(fact.from_relative("/../g").value().identifier() == "http://a/g");
    CHECK(fact.from_relative("g.").value().identifier() == "http://a/b/c/g.");
    CHECK(fact.from_relative(".g").value().identifier() == "http://a/b/c/.g");
    CHECK(fact.from_relative("g..").value().identifier() == "http://a/b/c/g..");
    CHECK(fact.from_relative("..g").value().identifier() == "http://a/b/c/..g");
    // stupid dots
    CHECK(fact.from_relative("./../g").value().identifier() == "http://a/b/g");
    CHECK(fact.from_relative("./g/.").value().identifier() == "http://a/b/c/g/");
    CHECK(fact.from_relative("g/./h").value().identifier() == "http://a/b/c/g/h");
    CHECK(fact.from_relative("g/../h").value().identifier() == "http://a/b/c/h");
    CHECK(fact.from_relative("g;x=1/./y").value().identifier() == "http://a/b/c/g;x=1/y");
    CHECK(fact.from_relative("g;x=1/../y").value().identifier() == "http://a/b/c/y");
    // unexpected dots
    CHECK(fact.from_relative("g?y/./x").value().identifier() == "http://a/b/c/g?y/./x");
    CHECK(fact.from_relative("g?y/../x").value().identifier() == "http://a/b/c/g?y/../x");
    CHECK(fact.from_relative("g#y/./x").value().identifier() == "http://a/b/c/g#y/./x");
    CHECK(fact.from_relative("g#y/../x").value().identifier() == "http://a/b/c/g#y/../x");
}

TEST_CASE("base reassign") {
    IRIFactory fact{"http://example/foo"};

    CHECK(fact.from_relative("bar").value().identifier() == "http://example/bar");

    CHECK(fact.get_base() == "http://example/foo");

    CHECK(fact.set_base("/bar") == IRIFactoryError::Relative);

    CHECK(fact.get_base() == "http://example/foo");

    CHECK(fact.set_base("http://example/bar") == IRIFactoryError::Ok);

    CHECK(fact.get_base() == "http://example/bar");

    CHECK(fact.from_relative("foo").value().identifier() == "http://example/foo");
}

TEST_CASE("prefix") {
    IRIFactory fact{};

    fact.assign_prefix("pre", "http://ex.org/");
    fact.assign_prefix("foo", "http://foo.org/");

    CHECK(fact.from_prefix("pre", "bar").value().identifier() == "http://ex.org/bar");
    CHECK(fact.from_prefix("foo", "bar").value().identifier() == "http://foo.org/bar");
    CHECK(fact.from_prefix("bar", "bar").error() == IRIFactoryError::UnknownPrefix);

    fact.assign_prefix("pre", "http://ex.org/pre2/");
    CHECK(fact.from_prefix("pre", "bar").value().identifier() == "http://ex.org/pre2/bar");

    fact.clear_prefix("pre");
    CHECK(fact.from_prefix("pre", "bar").error() == IRIFactoryError::UnknownPrefix);
}

TEST_CASE("relative prefix") {
    IRIFactory fact{"http://ex.org/"};

    fact.assign_prefix("pre", "pre/");
    fact.assign_prefix("foo", "foo/");

    CHECK(fact.from_prefix("pre", "bar").value().identifier() == "http://ex.org/pre/bar");
    CHECK(fact.from_prefix("foo", "bar").value().identifier() == "http://ex.org/foo/bar");
    CHECK(fact.from_prefix("bar", "bar").error() == IRIFactoryError::UnknownPrefix);

    fact.assign_prefix("pre", "pre2/");
    CHECK(fact.from_prefix("pre", "bar").value().identifier() == "http://ex.org/pre2/bar");

    fact.clear_prefix("pre");
    CHECK(fact.from_prefix("pre", "bar").error() == IRIFactoryError::UnknownPrefix);
}
TEST_CASE("char matcher") {
    using namespace util::char_matcher_detail;

    static constexpr ASCIIPatternMatcher pattern{"abc#"};
    CHECK(pattern.match('a'));
    CHECK(!pattern.match('x'));
    CHECK(!pattern.match('A'));
    CHECK(pattern.match('#'));
    CHECK(!pattern.match(0xFFFF));  // some random unicode

    static constexpr ASCIINumMatcher num{};
    CHECK(num.match('1'));
    CHECK(num.match('9'));
    CHECK(num.match('0'));
    CHECK(!num.match('A'));
    CHECK(!num.match('#'));
    CHECK(!num.match(0xFFFF));

    static constexpr ASCIIAlphaMatcher alp{};
    CHECK(alp.match('a'));
    CHECK(alp.match('z'));
    CHECK(alp.match('A'));
    CHECK(alp.match('Z'));
    CHECK(!alp.match('1'));
    CHECK(!alp.match('#'));
    CHECK(!alp.match(0xFFFF));

    static constexpr auto &alnum = ascii_alphanum_matcher;
    CHECK(alnum.match('a'));
    CHECK(alnum.match('z'));
    CHECK(alnum.match('A'));
    CHECK(alnum.match('Z'));
    CHECK(alnum.match('0'));
    CHECK(alnum.match('9'));
    CHECK(!alnum.match('#'));
    CHECK(!alnum.match(0xFFFF));

    static constexpr UCSCharMatcher ucs{};
    CHECK(ucs.match(0xA0));
    CHECK(ucs.match(0xD7FF));
    CHECK(ucs.match(0xF900));
    CHECK(ucs.match(0xFDCF));
    CHECK(ucs.match(0xFDF0));
    CHECK(ucs.match(0xFFEF));
    CHECK(ucs.match(0x10000));
    CHECK(ucs.match(0x1FFFD));
    CHECK(ucs.match(0x20000));
    CHECK(ucs.match(0x2FFFD));
    CHECK(ucs.match(0x30000));
    CHECK(ucs.match(0x3FFFD));
    CHECK(ucs.match(0x40000));
    CHECK(ucs.match(0x4FFFD));
    CHECK(ucs.match(0x50000));
    CHECK(ucs.match(0x5FFFD));
    CHECK(ucs.match(0x60000));
    CHECK(ucs.match(0x6FFFD));
    CHECK(ucs.match(0x70000));
    CHECK(ucs.match(0x7FFFD));
    CHECK(ucs.match(0x80000));
    CHECK(ucs.match(0x8FFFD));
    CHECK(ucs.match(0x90000));
    CHECK(ucs.match(0x9FFFD));
    CHECK(ucs.match(0xA0000));
    CHECK(ucs.match(0xAFFFD));
    CHECK(ucs.match(0xB0000));
    CHECK(ucs.match(0xBFFFD));
    CHECK(ucs.match(0xC0000));
    CHECK(ucs.match(0xCFFFD));
    CHECK(ucs.match(0xD0000));
    CHECK(ucs.match(0xDFFFD));
    CHECK(ucs.match(0xE0000));
    CHECK(ucs.match(0xEFFFD));
    CHECK(!ucs.match(0xFFFF));

    static constexpr IPrivateMatcher priv{};
    CHECK(priv.match(0xE000));
    CHECK(priv.match(0xF8FF));
    CHECK(priv.match(0xF0000));
    CHECK(priv.match(0xFFFFD));
    CHECK(priv.match(0x100000));
    CHECK(priv.match(0x10FFFD));
    CHECK(!priv.match(0xFFFF));

    CHECK(match(alnum, std::string_view{"abcAbZz093"}));
    CHECK(!match(alnum, std::string_view{"abcAb#Zz093"}));
    std::array<int,3> unic{
            'a', 'b', 'c'
    };
    CHECK(match(alnum, unic));
    unic[1] = 0x30000;
    CHECK(!match(alnum, unic));
    CHECK(match(alnum | ucs, unic));
}
