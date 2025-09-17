#include <string>
#include <utility>
#include <gtest/gtest.h>
#include <parapara/parapara.h>

using namespace parapara;

template <typename V>
using repn_tbl = std::pair<V, std::string>[];

template <typename V>
using as_array = V[];

TEST(parapara, rw_numeric) {
    auto expect_repn = [](const auto& tbl, bool skip_write_fallback = false) {
        for (auto [n, repn]: tbl) {
            using T = decltype(n);

            {
                hopefully<T> hn = read_numeric_fallback<T>(repn);
                EXPECT_TRUE(hn) << "read_numeric_fallback failed on '" << repn << '"';
                if (hn) EXPECT_EQ(n, hn.value());
            }

            if constexpr (can_from_chars_v<T>) {
                hopefully<T> hn = read_cc<T>(repn);
                EXPECT_TRUE(hn);
                if (hn) EXPECT_EQ(n, hn.value());
            }

            if (!skip_write_fallback) {
                hopefully<std::string> hs = write_numeric_fallback(n);
                EXPECT_TRUE(hs) << "write_numeric_fallback failed on '" << repn << '"';
                if (hs) EXPECT_EQ(repn, hs.value());
            }

            if constexpr (can_to_chars_v<T>) {
                hopefully<std::string> hs = write_cc(n);
                EXPECT_TRUE(hs);
                if (hs) EXPECT_EQ(repn, hs.value());
            }
        }
    };

    {
        SCOPED_TRACE("numeric type: short");
        expect_repn(repn_tbl<short>{
                {0, "0"},
                {-32767, "-32767"},
                {32767, "32767"}
                });
    }

    {
        SCOPED_TRACE("numeric type: unsigned short");
        expect_repn(repn_tbl<unsigned short>{
                {0, "0"},
                {32767, "32767"},
                {65535, "65535"}
                });
    }

    {
        SCOPED_TRACE("numeric type: int");
        expect_repn(repn_tbl<int>{
                {0, "0"},
                {-32767, "-32767"},
                {32767, "32767"}
                });
    }

    {
        SCOPED_TRACE("numeric type: unsigned int");
        expect_repn(repn_tbl<unsigned int>{
                {0, "0"},
                {32767, "32767"},
                {65535, "65535"}
                });
    }

    {
        SCOPED_TRACE("numeric type: long");
        expect_repn(repn_tbl<long>{
                {0, "0"},
                {-2147483647l, "-2147483647"},
                {2147483647l, "2147483647"}
                });
    }

    {
        SCOPED_TRACE("numeric type: unsigned long");
        expect_repn(repn_tbl<unsigned long>{
                {0, "0"},
                {2147483647ul, "2147483647"},
                {4294967295ul, "4294967295"}
                });
    }

    {
        SCOPED_TRACE("numeric type: long long");
        expect_repn(repn_tbl<long long>{
                {0, "0"},
                {-2147483647ll, "-2147483647"},
                {2147483647ll, "2147483647"}
                });
    }

    {
        SCOPED_TRACE("numeric type: unsigned long long");
        expect_repn(repn_tbl<unsigned long long>{
            {0, "0"},
            {2147483647ull, "2147483647"},
            {4294967295ull, "4294967295"}
        });
    }

    // Skip write fallback tests for floating point types as
    // the fallback routine doesn't guarantee shortest representation.
    {
        SCOPED_TRACE("numeric type: float");
        expect_repn(repn_tbl<float>{
                {0., "0"},
                {-0.25f, "-0.25"},
                {83000000.f, "8.3e+07"}
                }, true);
    }

    {
        SCOPED_TRACE("numeric type: double");
        expect_repn(repn_tbl<double>{
                {0., "0"},
                {-1./16384., "-6.103515625e-05"},
                {21474000000., "2.1474e+10"}
                }, true);
    }
}

TEST(parapara, rw_numeric_roundtrip) {
    auto verify_roundtrip = [](const auto& ns) {
        for (auto n: ns) {
            using T = decltype(n);
            {
                hopefully<std::string> hs = write_numeric_fallback(n);
                EXPECT_TRUE(hs) << "write_numeric_fallback failed on " << n;

                if (hs) {
                    hopefully<T> hn = read_numeric_fallback<T>(hs.value());
                    EXPECT_TRUE(hn) << "read_numeric_fallback failed on '" << hs.value() << "'";
                    EXPECT_EQ(n, hn.value());
                }
            }

            if constexpr (can_to_chars_v<T> && can_from_chars_v<T>) {
                hopefully<std::string> hs = write_cc(n);
                EXPECT_TRUE(hs) << "write_cc failed on " << n;

                if (hs) {
                    hopefully<T> hn = read_cc<T>(hs.value());
                    EXPECT_TRUE(hn) << "read_cc failed on '" << hs.value() << "'";
                    EXPECT_EQ(n, hn.value());
                }
            }
        }
    };

    {
        SCOPED_TRACE("numeric type: float");
        verify_roundtrip(as_array<float>{-3.1f, -1.2345e30f, std::numeric_limits<float>::infinity()});
    }
    {
        SCOPED_TRACE("numeric type: double");
        verify_roundtrip(as_array<double>{-3.1f, -1.2345e-240, std::numeric_limits<double>::infinity()});
    }
    {
        SCOPED_TRACE("numeric type: long double");
        verify_roundtrip(as_array<long double>{-3.1l, -1.2345e-240l, std::numeric_limits<long double>::infinity()});
    }
}
