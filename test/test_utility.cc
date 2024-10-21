#include <type_traits>
#include <gtest/gtest.h>
#include <parapara/parapara.h>

using namespace parapara;

TEST(parapara, any_ptr) {
    EXPECT_FALSE(any_ptr{});
    EXPECT_FALSE(any_ptr{}.has_value());
    EXPECT_FALSE(any_ptr(nullptr));
    EXPECT_FALSE(any_ptr(nullptr).has_value());
    EXPECT_FALSE(any_ptr((char *)0));
    EXPECT_FALSE(any_ptr((char *)0).has_value());

    int a = 0;
    EXPECT_TRUE(any_ptr("abc"));
    EXPECT_TRUE(any_ptr("abc").has_value());
    EXPECT_TRUE(any_ptr(&a));
    EXPECT_TRUE(any_ptr(&a).has_value());
    EXPECT_TRUE(any_ptr((void *)&a));
    EXPECT_TRUE(any_ptr((void *)&a).has_value());

    any_ptr p(&a);
    any_ptr q = p;

    EXPECT_TRUE(q);
    EXPECT_EQ(&a, q.as<int*>());
    q.reset();
    EXPECT_FALSE(q);
    q.reset(&a);
    EXPECT_TRUE(q);
    EXPECT_EQ(&a, q.as<int*>());
    q.reset(nullptr);
    EXPECT_FALSE(q);

    double b;
    q = &b;
    EXPECT_TRUE(q);
    EXPECT_EQ(&b, q.as<double*>());
    EXPECT_EQ(nullptr, q.as<int*>());
    q.reset();
    EXPECT_EQ(&a, (q = &a).as<int*>());

    const int ac = 3;
    EXPECT_EQ(typeid(int*), any_ptr{&a}.type());
    EXPECT_EQ(typeid(const int*), any_ptr{&ac}.type());

    a = 10;
    *(any_ptr{&a}.as<int*>()) = 20;
    EXPECT_EQ(20, a);

    // this is parapara::any_cast not std::any_cast ...
    p.reset(&a);
    EXPECT_EQ(&a, any_cast<int*>(p));
    EXPECT_EQ(nullptr, any_cast<const int*>(p));
    EXPECT_EQ(nullptr, any_cast<double*>(p));
}

namespace {
void fv0() {}
void fv1(double) {}
void fv2(const char*, double) {}

struct cv0 { void operator()() {} };
struct cv0c { void operator()() const {} };

struct ci2 { int operator()(const char*, double) { return 3; } };
struct ci2c { int operator()(const char*, double) const { return 3; } };

struct mf3 {
    double f(int, const char*, double) { return 1.0; }
    double fc(int, const char*, double) const { return 1.0; }
};
}

TEST(parapara, n_args) {
    EXPECT_EQ(0, n_args<decltype(fv0)>);
    EXPECT_EQ(1, n_args<decltype(fv1)>);
    EXPECT_EQ(2, n_args<decltype(fv2)>);

    EXPECT_EQ(0, n_args<cv0>);
    EXPECT_EQ(0, n_args<cv0c>);
    EXPECT_EQ(2, n_args<ci2>);
    EXPECT_EQ(2, n_args<ci2c>);

    EXPECT_EQ(3, n_args<decltype(&mf3::f)>);
    EXPECT_EQ(3, n_args<decltype(&mf3::fc)>);

    EXPECT_TRUE((has_n_args_v<decltype(fv2), 2>));
    EXPECT_FALSE((has_n_args_v<decltype(fv2), 0>));

    EXPECT_TRUE((has_n_args_v<ci2c, 2>));
    EXPECT_FALSE((has_n_args_v<ci2c, 1>));

    EXPECT_TRUE((has_n_args_v<decltype(&mf3::f), 3>));
    EXPECT_FALSE((has_n_args_v<decltype(&mf3::f), 0>));

    int foo; // n_args<int> should be an incomplete type
    EXPECT_FALSE((has_n_args_v<decltype(foo), 0>));
    EXPECT_FALSE((has_n_args_v<decltype(foo), 1>));
}

TEST(pararapara, nth_argument) {
    EXPECT_TRUE((std::is_same_v<double, nth_argument_t<0, decltype(fv1)>>));

    EXPECT_TRUE((std::is_same_v<const char*, nth_argument_t<0, ci2c>>));
    EXPECT_TRUE((std::is_same_v<double, nth_argument_t<1, ci2c>>));

    EXPECT_TRUE((std::is_same_v<int, nth_argument_t<0, decltype(&mf3::f)>>));
    EXPECT_TRUE((std::is_same_v<const char*, nth_argument_t<1, decltype(&mf3::f)>>));
    EXPECT_TRUE((std::is_same_v<double, nth_argument_t<2, decltype(&mf3::f)>>));

    EXPECT_TRUE((std::is_same_v<int, return_value_t<ci2c>>));
    EXPECT_TRUE((std::is_same_v<int, return_value_t<ci2c>>));

    EXPECT_TRUE((std::is_same_v<void, return_value_t<decltype(fv0)>>));
    EXPECT_TRUE((std::is_same_v<void, return_value_t<decltype(fv1)>>));
    EXPECT_TRUE((std::is_same_v<double, return_value_t<decltype(&mf3::fc)>>));
}
