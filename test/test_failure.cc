#include <any>
#include <string>

#include <gtest/gtest.h>
#include <parapara/parapara.h>

using namespace parapara;

TEST(parapara, failure_context) {
    // Test field overriding with operatore +

    failure_context f = { "key", "source", "record", 10, 20 };
    failure_context g = { "", "g_source", "", 0, 30 };
    failure_context h = { "h_key", "", "h_record", 15, 0 };

    auto eq = [](const failure_context& a, const failure_context& b) {
        return a.key==b.key && a.source==b.source && a.record==b.record && a.nr==b.nr && a.cindex==b.cindex;
    };

    failure_context fg = f;
    fg += g;

    EXPECT_TRUE(eq(fg, f+g));
    EXPECT_TRUE(eq(fg, failure_context{ "key", "g_source", "record", 10, 30 }));

    failure_context fh = f;
    fh += h;

    EXPECT_TRUE(eq(fh, f+h));
    EXPECT_TRUE(eq(fh, failure_context{ "h_key", "source", "h_record", 15, 20 }));
}

TEST(parapara, failure_helpers) {
    const failure_context c0 = { "", "", "", 0, 0 };
    const failure_context c1 = { "key", "source", "record", 10, 20 };

    auto eq = [](const failure_context& a, const failure_context& b) {
        return a.key==b.key && a.source==b.source && a.record==b.record && a.nr==b.nr && a.cindex==b.cindex;
    };

    auto f1 = internal_error().error();
    EXPECT_EQ(failure::internal_error, f1.error);
    EXPECT_TRUE(eq(c0,f1.ctx));

    auto f2 = internal_error(c1).error();
    EXPECT_EQ(failure::internal_error, f2.error);
    EXPECT_TRUE(eq(c1,f2.ctx));

    auto f3 = read_failure().error();
    EXPECT_EQ(failure::read_failure, f3.error);
    EXPECT_TRUE(eq(c0,f3.ctx));

    auto f4 = read_failure(c1).error();
    EXPECT_EQ(failure::read_failure, f4.error);
    EXPECT_TRUE(eq(c1,f4.ctx));

    auto f5 = invalid_value().error();
    EXPECT_EQ(failure::invalid_value, f5.error);
    EXPECT_TRUE(eq(c0,f5.ctx));
    EXPECT_TRUE(f5.data.has_value());
    const std::string* f5_data_ptr = std::any_cast<std::string>(&(f5.data));
    ASSERT_NE(nullptr, f5_data_ptr);
    EXPECT_EQ("", *f5_data_ptr);

    auto f6 = invalid_value("constraint", c1).error();
    EXPECT_EQ(failure::invalid_value, f6.error);
    EXPECT_TRUE(eq(c1,f6.ctx));
    EXPECT_TRUE(f5.data.has_value());
    const std::string* f6_data_ptr = std::any_cast<std::string>(&(f6.data));
    ASSERT_NE(nullptr, f6_data_ptr);
    EXPECT_EQ("constraint", *f6_data_ptr);

    auto f7 = unrecognized_key().error();
    EXPECT_EQ(failure::unrecognized_key, f7.error);
    EXPECT_TRUE(eq(c0,f7.ctx));
    EXPECT_EQ("", f7.ctx.key);

    auto f8 = unrecognized_key("", c1).error();
    EXPECT_EQ(failure::unrecognized_key, f8.error);
    EXPECT_TRUE(eq(c1,f8.ctx));

    auto f9 = unrecognized_key("other key", c1).error();
    EXPECT_EQ(failure::unrecognized_key, f9.error);

    failure_context c9bis = c1;
    c9bis.key = "other key";
    EXPECT_TRUE(eq(c9bis,f9.ctx));

    auto f10 = bad_syntax().error();
    EXPECT_EQ(failure::bad_syntax, f10.error);
    EXPECT_TRUE(eq(c0,f10.ctx));

    auto f11 = bad_syntax(c1).error();
    EXPECT_EQ(failure::bad_syntax, f11.error);
    EXPECT_TRUE(eq(c1,f11.ctx));

    auto f12 = empty_optional().error();
    EXPECT_EQ(failure::empty_optional, f12.error);
    EXPECT_TRUE(eq(c0,f12.ctx));
    EXPECT_EQ("", f12.ctx.key);

    auto f13 = empty_optional("", c1).error();
    EXPECT_EQ(failure::empty_optional, f13.error);
    EXPECT_TRUE(eq(c1,f13.ctx));

    auto f14 = empty_optional("quux", c1).error();
    EXPECT_EQ(failure::empty_optional, f14.error);

    failure_context c14bis = c1;
    c14bis.key = "quux";
    EXPECT_TRUE(eq(c14bis,f14.ctx));

}

TEST(parapara, explain) {
    auto f1 = read_failure({"", "foo.inp", "quibbity seven", 3, 10}).error();
    EXPECT_EQ("foo.inp:3:10: read failure\n", explain(f1));
    EXPECT_EQ("foo.inp:3:10: read failure\n" \
              "    3 | quibbity seven\n"     \
              "      |          ^\n",
              explain(f1, true));


    auto f2 = unrecognized_key("zoinks", {"x", "foo.inp", "  zoinks = fish cakes", 0, 3}).error();
    EXPECT_EQ("foo.inp:3: unrecognized key \"zoinks\"\n", explain(f2));
    EXPECT_EQ("foo.inp:3: unrecognized key \"zoinks\"\n"
              "      |   zoinks = fish cakes\n"
              "      |   ^\n",
              explain(f2, true));

    auto f3 = invalid_value("no fish", {"zoinks", "argv[4]", "zoinks=fish", 0, 8}).error();
    EXPECT_EQ("argv[4]:8: invalid value: constraint: no fish\n", explain(f3));
    EXPECT_EQ("argv[4]:8: invalid value: constraint: no fish\n"
              "      | zoinks=fish\n"
              "      |        ^\n",
              explain(f3, true));


    auto f4 = unsupported_type({"", "foo.inp", "", 0, 0}).error();
    EXPECT_EQ("foo.inp: unsupported type\n", explain(f4));
    EXPECT_EQ("foo.inp: unsupported type\n", explain(f4, true));

    auto f5 = bad_syntax({"", "foo.inp", "not bad just parsed that way", 123456789, 1}).error();
    EXPECT_EQ("foo.inp:123456789:1: bad syntax\n", explain(f5));
    EXPECT_EQ("foo.inp:123456789:1: bad syntax\n"
              "123456789 | not bad just parsed that way\n"
              "          | ^\n",
              explain(f5, true));

    // empty optional is treated as a form of internal error for explanations
    auto f6 = empty_optional("zoinks", {"x", "foo.inp", "", 1, 1}).error();
    EXPECT_EQ("foo.inp:1:1: internal error\n", explain(f6));

    auto f7 = internal_error({"zoinks", "bar.inp", "", 1, 1}).error();
    EXPECT_EQ("bar.inp:1:1: internal error\n", explain(f7));
}
