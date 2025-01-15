#include <gtest/gtest.h>

#include <utility>
#include <vector>

#include <parapara/parapara.h>
#include "common.h"

using std::in_place;
using parapara::defaulted;

TEST(defaulted, ctors) {
    {
        // default ctor

        defaulted<check_in_place> d;

        EXPECT_FALSE(d.is_assigned());
        EXPECT_EQ(0, d.default_value().n_in_place_args);
    }

    {
        // constructing from value type

        using ci = counted<int>;
        ci v{3};

        ci::reset();
        defaulted<ci> d1{v};

        EXPECT_FALSE(d1.is_assigned());
        EXPECT_EQ(3, d1.default_value().inner);
        EXPECT_EQ(1, ci::n_copy_ctor);
        EXPECT_EQ(0, ci::n_copy_assign);
        EXPECT_EQ(0, ci::n_move_ctor);
        EXPECT_EQ(0, ci::n_move_assign);


        counted<int>::reset();
        defaulted<counted<int>> d2{std::move(v)};

        EXPECT_FALSE(d2.is_assigned());
        EXPECT_EQ(3, d2.default_value().inner);
        EXPECT_EQ(0, ci::n_copy_ctor);
        EXPECT_EQ(0, ci::n_copy_assign);
        EXPECT_EQ(1, ci::n_move_ctor);
        EXPECT_EQ(0, ci::n_move_assign);
    }

    {
        // constructing from convertible type

        struct X {
            explicit operator bool() const { return true; }
        };

        defaulted<bool> d1(X{});
        EXPECT_FALSE(d1.is_assigned());
        EXPECT_EQ(true, d1.default_value());

        const X x;

        defaulted<bool> d2(x);
        EXPECT_FALSE(d2.is_assigned());
        EXPECT_EQ(true, d2.default_value());
    }

    {
        // construction in-place

        using cc = counted<check_in_place>;
        cc::reset();

        defaulted<cc> d0(in_place);
        EXPECT_FALSE(d0.is_assigned());
        EXPECT_EQ(0, d0.default_value().inner.n_in_place_args);

        defaulted<cc> d1(in_place, 10);
        EXPECT_FALSE(d1.is_assigned());
        EXPECT_EQ(1, d1.default_value().inner.n_in_place_args);

        defaulted<cc> d2(in_place, 10, 20);
        EXPECT_FALSE(d2.is_assigned());
        EXPECT_EQ(2, d2.default_value().inner.n_in_place_args);

        defaulted<cc> dil1(in_place, {3, 4, 5});
        EXPECT_FALSE(dil1.is_assigned());
        EXPECT_EQ(1, dil1.default_value().inner.n_in_place_args);

        defaulted<cc> dil3(in_place, {3, 4, 5}, 6, 7);
        EXPECT_FALSE(dil3.is_assigned());
        EXPECT_EQ(3, dil3.default_value().inner.n_in_place_args);
    }

    {
        // copy and move constructors

        struct X {};
        using cx = counted<X>;

        defaulted<cx> u1;
        defaulted<cx> v1;
        v1.emplace();
        ASSERT_FALSE(u1.is_assigned());
        ASSERT_TRUE(v1.is_assigned());

        cx::reset();

        defaulted<cx> d1(u1);
        EXPECT_FALSE(d1.is_assigned());
        EXPECT_EQ(1, cx::n_copy_ctor);

        defaulted<cx> d2(std::move(u1));
        EXPECT_FALSE(d2.is_assigned());
        EXPECT_EQ(1, cx::n_move_ctor);

        EXPECT_EQ(1, cx::n_copy_ctor);
        EXPECT_EQ(1, cx::n_move_ctor);
        EXPECT_EQ(0, cx::n_copy_assign);
        EXPECT_EQ(0, cx::n_move_assign);

        cx::reset();

        defaulted<cx> d3(v1);
        EXPECT_TRUE(d3.is_assigned());
        EXPECT_EQ(2, cx::n_copy_ctor);

        defaulted<cx> d4(std::move(v1));
        EXPECT_TRUE(d4.is_assigned());
        EXPECT_EQ(2, cx::n_move_ctor);

        EXPECT_EQ(2, cx::n_copy_ctor);
        EXPECT_EQ(2, cx::n_move_ctor);
        EXPECT_EQ(0, cx::n_copy_assign);
        EXPECT_EQ(0, cx::n_move_assign);
    }
}

TEST(defaulted, assignment) {
    {
        // copy and move assignment from same defaulted type

        struct X {};
        using cx = counted<X>;

        defaulted<cx> u1;
        defaulted<cx> v1;
        v1.emplace();
        ASSERT_FALSE(u1.is_assigned());
        ASSERT_TRUE(v1.is_assigned());

        cx::reset();
        defaulted<cx> d1;
        d1 = u1;
        EXPECT_FALSE(d1.is_assigned());
        EXPECT_EQ(0, cx::n_copy_assign);
        EXPECT_EQ(0, cx::n_move_assign);

        defaulted<cx> d2;
        d2 = std::move(u1);
        EXPECT_FALSE(d2.is_assigned());
        EXPECT_EQ(0, cx::n_copy_assign);
        EXPECT_EQ(0, cx::n_move_assign);

        EXPECT_EQ(0, cx::n_copy_ctor);
        EXPECT_EQ(0, cx::n_move_ctor);
        EXPECT_EQ(0, cx::n_copy_assign);
        EXPECT_EQ(0, cx::n_move_assign);

        cx::reset();

        // Confirm assignment from something holding an assigned value
        // to something without performs direct initialization.

        defaulted<cx> d3;
        d3 = v1;
        EXPECT_EQ(1, cx::n_copy_ctor);
        EXPECT_TRUE(d3.is_assigned());

        defaulted<cx> d4;
        d4 = std::move(v1);
        EXPECT_EQ(1, cx::n_move_ctor);
        EXPECT_TRUE(d4.is_assigned());

        EXPECT_EQ(1, cx::n_copy_ctor);
        EXPECT_EQ(1, cx::n_move_ctor);
        EXPECT_EQ(0, cx::n_copy_assign);
        EXPECT_EQ(0, cx::n_move_assign);

        cx::reset();

        // If lhs already holds an assigned value, assignment will
        // perform an assignment on both the asssigned value and
        // the default value.

        defaulted<cx> d5;
        d5.emplace();
        d5 = v1;
        EXPECT_EQ(1, cx::n_copy_assign);
        EXPECT_TRUE(d5.is_assigned());

        defaulted<cx> d6;
        d6.emplace();
        d6 = std::move(v1);
        EXPECT_EQ(1, cx::n_move_assign);
        EXPECT_TRUE(d6.is_assigned());

        EXPECT_EQ(0, cx::n_copy_ctor);
        EXPECT_EQ(0, cx::n_move_ctor);
        EXPECT_EQ(1, cx::n_copy_assign);
        EXPECT_EQ(1, cx::n_move_assign);

        // Assigning from an unassigned defaulted value should reset().
        // LHS default_value should remain unchanged.

        defaulted<cx> d7;
        d7.emplace();
        d7 = u1;
        EXPECT_FALSE(d7.is_assigned());

        defaulted<cx> d8;
        d8.emplace();
        d8 = std::move(u1);
        EXPECT_FALSE(d8.is_assigned());

        EXPECT_EQ(0, cx::n_copy_ctor);
        EXPECT_EQ(0, cx::n_move_ctor);
        EXPECT_EQ(1, cx::n_copy_assign);
        EXPECT_EQ(1, cx::n_move_assign);

        defaulted<int> y{10}, z{5};
        y = 20;
        ASSERT_EQ(10, y.default_value());
        ASSERT_EQ(20, y.value());
        ASSERT_EQ(5, z.default_value());
        ASSERT_FALSE(z.is_assigned());

        y = z;
        EXPECT_FALSE(y.is_assigned());
        EXPECT_EQ(10, y.default_value());
        EXPECT_EQ(10, y.value());

        y = 20;
        EXPECT_EQ(20, y.value());
        y = std::move(z);
        EXPECT_FALSE(y.is_assigned());
        EXPECT_EQ(10, y.default_value());
        EXPECT_EQ(10, y.value());

        // Assigning from std::optional; if empty, reset().

        defaulted<int> p{10};
        std::optional<int> q;

        p = 20;
        EXPECT_TRUE(p.is_assigned());
        EXPECT_EQ(20, p.value());
        EXPECT_EQ(10, p.default_value());

        p = q;
        EXPECT_FALSE(p.is_assigned());
        EXPECT_EQ(10, p.value());
        EXPECT_EQ(10, p.default_value());

        p = 20;
        p = std::move(q);
        EXPECT_FALSE(p.is_assigned());
        EXPECT_EQ(10, p.value());
        EXPECT_EQ(10, p.default_value());

        p = std::optional<int>{30};
        EXPECT_TRUE(p.is_assigned());
        EXPECT_EQ(30, p.value());
        EXPECT_EQ(10, p.default_value());
    }

    {
        // copy and move assignment of value

        using ci = counted<int>;
        ci i1{1}, i2{2};

        defaulted<ci> d1{i1};
        ci::reset();

        d1 = i2;
        EXPECT_TRUE(d1.is_assigned());
        EXPECT_EQ(2, d1.value().inner);

        EXPECT_EQ(1, ci::n_copy_ctor);
        EXPECT_EQ(0, ci::n_move_ctor);
        EXPECT_EQ(0, ci::n_copy_assign);
        EXPECT_EQ(0, ci::n_move_assign);

        defaulted<ci> d2{i1};
        ci::reset();

        d2 = std::move(i2);
        EXPECT_TRUE(d2.is_assigned());
        EXPECT_EQ(2, d2.value().inner);

        EXPECT_EQ(0, ci::n_copy_ctor);
        EXPECT_EQ(1, ci::n_move_ctor);
        EXPECT_EQ(0, ci::n_copy_assign);
        EXPECT_EQ(0, ci::n_move_assign);

        defaulted<ci> d3{i1};
        d3.emplace();
        ci::reset();

        d3 = i2;
        EXPECT_TRUE(d3.is_assigned());
        EXPECT_EQ(2, d3.value().inner);

        EXPECT_EQ(0, ci::n_copy_ctor);
        EXPECT_EQ(0, ci::n_move_ctor);
        EXPECT_EQ(1, ci::n_copy_assign);
        EXPECT_EQ(0, ci::n_move_assign);

        defaulted<ci> d4{i1};
        d4.emplace();
        ci::reset();

        d4 = std::move(i2);
        EXPECT_TRUE(d4.is_assigned());
        EXPECT_EQ(2, d4.value().inner);

        EXPECT_EQ(0, ci::n_copy_ctor);
        EXPECT_EQ(0, ci::n_move_ctor);
        EXPECT_EQ(0, ci::n_copy_assign);
        EXPECT_EQ(1, ci::n_move_assign);
    }
}

TEST(defaulted, emplace) {
    using cc = counted<check_in_place>;
    cc::reset();

    defaulted<cc> d0;
    d0.emplace();
    EXPECT_TRUE(d0.is_assigned());
    EXPECT_EQ(0, d0.value().inner.n_in_place_args);

    defaulted<cc> d1;
    d1.emplace(10);
    EXPECT_TRUE(d1.is_assigned());
    EXPECT_EQ(1, d1.value().inner.n_in_place_args);

    defaulted<cc> d2;
    d2.emplace(10, 20);
    EXPECT_TRUE(d2.is_assigned());
    EXPECT_EQ(2, d2.value().inner.n_in_place_args);

    defaulted<cc> dil1;
    dil1.emplace({3, 4, 5});
    EXPECT_TRUE(dil1.is_assigned());
    EXPECT_EQ(1, dil1.value().inner.n_in_place_args);

    defaulted<cc> dil3;
    dil3.emplace({3, 4, 5}, 6, 7);
    EXPECT_TRUE(dil3.is_assigned());
    EXPECT_EQ(3, dil3.value().inner.n_in_place_args);

    EXPECT_EQ(0, cc::n_copy_ctor);
    EXPECT_EQ(0, cc::n_move_ctor);
    EXPECT_EQ(0, cc::n_copy_assign);
    EXPECT_EQ(0, cc::n_move_assign);
}

TEST(defaulted, emplace_default) {
    using cc = counted<check_in_place>;
    cc::reset();

    defaulted<cc> d0;
    d0.emplace_default();
    EXPECT_FALSE(d0.is_assigned());
    EXPECT_EQ(0, d0.default_value().inner.n_in_place_args);

    defaulted<cc> d1;
    d1.emplace_default(10);
    EXPECT_FALSE(d1.is_assigned());
    EXPECT_EQ(1, d1.default_value().inner.n_in_place_args);

    defaulted<cc> d2;
    d2.emplace_default(10, 20);
    EXPECT_FALSE(d2.is_assigned());
    EXPECT_EQ(2, d2.default_value().inner.n_in_place_args);

    defaulted<cc> dil1;
    dil1.emplace_default({3, 4, 5});
    EXPECT_FALSE(dil1.is_assigned());
    EXPECT_EQ(1, dil1.default_value().inner.n_in_place_args);

    defaulted<cc> dil3;
    dil3.emplace_default({3, 4, 5}, 6, 7);
    EXPECT_FALSE(dil3.is_assigned());
    EXPECT_EQ(3, dil3.default_value().inner.n_in_place_args);

    EXPECT_EQ(0, cc::n_copy_ctor);
    EXPECT_EQ(0, cc::n_move_ctor);
    EXPECT_EQ(0, cc::n_copy_assign);
    EXPECT_EQ(0, cc::n_move_assign);
}


TEST(defaulted, access) {
    {
        // Basic access semantics

        defaulted<int> d{10};

        EXPECT_FALSE(d.is_assigned());
        EXPECT_EQ(10, d.value());
        EXPECT_EQ(10, d.default_value());

        d = 20;
        EXPECT_TRUE(d.is_assigned());
        EXPECT_EQ(20, d.value());
        EXPECT_EQ(10, d.default_value());

        d.emplace_default(30);
        EXPECT_TRUE(d.is_assigned());
        EXPECT_EQ(20, d.value());
        EXPECT_EQ(30, d.default_value());

        d.reset();
        EXPECT_FALSE(d.is_assigned());
        EXPECT_EQ(30, d.value());
        EXPECT_EQ(30, d.default_value());
    }

    {
        // reference access to default value

        using ci = counted<int>;
        defaulted<ci> d{10};
        ci::reset();

        d.default_value() = ci{20};
        EXPECT_FALSE(d.is_assigned());
        EXPECT_EQ(20, d.default_value().inner);
        EXPECT_EQ(1, ci::n_move_assign);

        ci::reset();
        defaulted<ci> d2(std::move(d).default_value());
        EXPECT_EQ(20, d2.default_value().inner);
        EXPECT_EQ(1, ci::n_move_ctor);
    }

    {
        // const reference access to assigned/default value

        defaulted<int> d{10};

        const int& a1 = d.value();
        const int& b1 = d.value();

        EXPECT_EQ(10, a1);
        EXPECT_EQ(&a1, &b1);

        d = 20;

        const int& a2 = d.value();
        const int& b2 = d.value();

        EXPECT_EQ(20, a2);
        EXPECT_EQ(&a2, &b2);
        EXPECT_NE(&a1, &a2);
    }

    {
        // move semantics for assigned/default value

        using ci = counted<int>;
        defaulted<ci> d{10};
        ASSERT_FALSE(d.is_assigned());

        ci::reset();
        ci a1 = std::move(d).value();
        EXPECT_EQ(10, a1.inner);

        EXPECT_EQ(0, ci::n_copy_ctor);
        EXPECT_EQ(1, ci::n_move_ctor);
        EXPECT_EQ(0, ci::n_copy_assign);
        EXPECT_EQ(0, ci::n_move_assign);

        d = ci{20};
        ASSERT_TRUE(d.is_assigned());

        ci::reset();
        ci a2 = std::move(d).value();
        EXPECT_EQ(20, a2.inner);

        EXPECT_EQ(0, ci::n_copy_ctor);
        EXPECT_EQ(1, ci::n_move_ctor);
        EXPECT_EQ(0, ci::n_copy_assign);
        EXPECT_EQ(0, ci::n_move_assign);
    }
}

