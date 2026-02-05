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
