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
