#include <string>
#include <string_view>
#include <vector>
#include <utility>
#include <gtest/gtest.h>
#include <parapara/parapara.h>

using namespace parapara;
using namespace std::literals;

TEST(parapara, rw_dsv) {
    using intvec = std::vector<int>;

    // empty vector/representation
    {
        auto read_intvec = read_dsv<intvec>();
        hopefully<intvec> hv = read_intvec(""sv, default_reader());

        ASSERT_TRUE(hv);
        EXPECT_TRUE(hv.value().empty());

        auto write_intvec = write_dsv<intvec>();
        hopefully<std::string> hs = write_intvec(intvec{}, default_writer());

        ASSERT_TRUE(hs);
        EXPECT_TRUE(hs.value().empty());
    }

    // unit vector/representation
    {
        auto read_intvec = read_dsv<intvec>();
        hopefully<intvec> hv = read_intvec("123"sv, default_reader());

        ASSERT_TRUE(hv);
        ASSERT_EQ(1u, hv.value().size());
        EXPECT_EQ(123, hv.value().at(0));

        auto write_intvec = write_dsv<intvec>();
        hopefully<std::string> hs = write_intvec(intvec{234}, default_writer());

        ASSERT_TRUE(hs);
        EXPECT_EQ("234"s, hs.value());
    }

    // longer vector/representation
    {
        // leading spaces should be removed with default parameter skip_ws = true.
        auto read_intvec = read_dsv<intvec>();
        hopefully<intvec> hv = read_intvec("123, 234, 345"sv, default_reader());

        ASSERT_TRUE(hv);
        ASSERT_EQ(3u, hv.value().size());
        EXPECT_EQ(123, hv.value().at(0));
        EXPECT_EQ(234, hv.value().at(1));
        EXPECT_EQ(345, hv.value().at(2));

        auto write_intvec = write_dsv<intvec>();
        hopefully<std::string> hs = write_intvec(intvec{234, 345, 456}, default_writer());

        ASSERT_TRUE(hs);
        EXPECT_EQ("234,345,456"s, hs.value());
    }

    // vector with separator > 1 byte.
    {
        auto read_intvec = read_dsv<intvec>(u8"‡");
        hopefully<intvec> hv = read_intvec("123‡234‡345"sv, default_reader());

        ASSERT_TRUE(hv);
        ASSERT_EQ(3u, hv.value().size());
        EXPECT_EQ(123, hv.value().at(0));
        EXPECT_EQ(234, hv.value().at(1));
        EXPECT_EQ(345, hv.value().at(2));

        auto write_intvec = write_dsv<intvec>(u8"‡");
        hopefully<std::string> hs = write_intvec(intvec{234, 345, 456}, default_writer());

        ASSERT_TRUE(hs);
        EXPECT_EQ("234‡345‡456"s, hs.value());
    }
}

TEST(parapara, rw_qstring) {
    using namespace std::literals;

    std::pair<std::string, std::string> tests[] = {
        {           R"()",                R"("")" },
        {          R"(')",               R"("'")" },
        {          R"(")",              R"("\"")" },
        {        R"(abc)",             R"("abc")" },
        {        R"(a\c)",            R"("a\\c")" },
        {  "\007\010\011",          R"("\a\b\t")" },
        {  "\012\013\014",          R"("\n\v\f")" },
        {  "\015\016\141",         R"("\r\016a")" },
        {      "a\0b\0c"s,   "\"a\\000b\\000c\""s }
    };

    for (auto& [u, q]: tests) {
        auto mq = write_qstring_always(u);
        ASSERT_TRUE(mq);
        EXPECT_EQ(q, mq.value());
        auto mu = read_qstring(q);
        ASSERT_TRUE(mu);
        EXPECT_EQ(u, mu.value());
    }

    EXPECT_FALSE(read_qstring(R"("half-quoted)"));
    EXPECT_FALSE(read_qstring(R"("quoted"trailing)"));

    EXPECT_EQ("unquoted\\a"s, read_qstring(R"(unquoted\a)").value_or(""s));
}

TEST(parapara, write_qstring_conditional) {
    using namespace std::literals;

    // Without a delimiter test, expect quoting to occur only if
    // 1. There is leading space.
    // 2. There is a character that needs to be escaped.
    // Escaped characters are those with code less than 32, the
    // double quote character '"', and the backslash '\'.

    auto write_qstring_csv = write_qstring_conditional{","};

    std::string expect_unquoted[] = {
        ""s,        // Empty strings stay empty.
        "\177"s,    // [Del] isn't escaped.
        "a  "s,     // Trailing space okay too.
        "a b  d'"s  // Single quote fine.
    };

    for (auto& s: expect_unquoted) {
        auto mq = write_qstring(s);
        ASSERT_TRUE(mq);
        EXPECT_EQ(s, mq.value());
        mq = write_qstring_csv(s);
        ASSERT_TRUE(mq);
        EXPECT_EQ(s, mq.value());
    }

    std::string expect_quoted[] = {
        " fop"s,    // Leading space.
        "a\"b"s,    // Contains quote.
        "\\b"s,     // Contains backslash.
        "a\0b"s,    // Contains other escapable characters ....
        "\001wat"s
    };

    for (auto& s: expect_quoted) {
        auto q = write_qstring_always(s);
        ASSERT_TRUE(q);
        auto mq = write_qstring(s);
        ASSERT_TRUE(mq);
        EXPECT_EQ(q.value(), mq.value());
        mq = write_qstring_csv(s);
        ASSERT_TRUE(mq);
        EXPECT_EQ(q.value(), mq.value());
    }

    // write_qstring_csv should quote strings with commas though.

    EXPECT_EQ("\"a,b,c\""s, write_qstring_csv("a,b,c"s).value());
    EXPECT_EQ("a,b,c"s, write_qstring("a,b,c"s).value());

    // Confirm quoting of strings with ambiguous terminal fragments of
    // multi-character delimiters.

    auto wqsc__ = write_qstring_conditional{"__"};
    EXPECT_EQ("\"a__b\""s, wqsc__("a__b"s).value());
    EXPECT_EQ("\"a_b_\""s, wqsc__("a_b_"s).value());
    EXPECT_EQ("\"_a_b\""s, wqsc__("_a_b"s).value());
    EXPECT_EQ("a_b"s,      wqsc__("a_b"s).value());

    auto wqsc1231 = write_qstring_conditional{"1231"};
    EXPECT_EQ("\"_123\""s, wqsc1231("_123"s).value());
    EXPECT_EQ("_12"s,      wqsc1231("_12"s).value());
    EXPECT_EQ("\"231_\""s, wqsc1231("231_"s).value());
    EXPECT_EQ("31_"s,      wqsc1231("31_"s).value());
}
