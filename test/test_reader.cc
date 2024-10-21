#include <gtest/gtest.h>
#include <parapara/parapara.h>

using namespace parapara;

TEST(parapara, basic_reader) {
    struct reader<> R;

    std::string fish = "fish";

    auto v = R.read<int>(fish);
    ASSERT_FALSE(v);
    auto ve = v.error();
    EXPECT_EQ(failure::unsupported_type, ve.error);

    auto w = R.read(typeid(int), fish);
    ASSERT_FALSE(w);
    auto we = w.error();
    EXPECT_EQ(failure::unsupported_type, we.error);

    auto as_int = [](std::string_view v) -> hopefully<int> { return static_cast<int>(v.size()); };
    R.add(as_int);

    v = R.read<int>(fish);
    ASSERT_TRUE(v);
    EXPECT_EQ(4, v.value());

    w = R.read(typeid(int), fish);
    ASSERT_TRUE(w);
    EXPECT_EQ(4, any_cast<int>(w.value()));

    // TODO tests here:
    // 1. Adding read functions that take the read dictionary
    // 2. Adding multiple readers, overwriting readers
    // 3. Adding sets of read functions from another reader
    // 4. Explicit construction wither readers, read functions
}

// Add tests for readers with non-default representation type

