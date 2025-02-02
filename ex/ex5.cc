#include <iomanip>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>

#include <parapara/parapara.h>

using namespace std::literals;

namespace P = parapara;

// Demo reporting multiple validation or syntax errors in INI importer.
// Also exercises delegating specifications for subobjects.

const char* ini_text =
    "# Validators require odd values in [odd] and even values in [even]\n"
    "\n"
    "   flag  \n"
    "\n"
    "# Section heading without ']' should give bad_syntax error\n"
    "[oops\n"
    "\n"
    "[odd]\n"
    "\n"
    "a = 3\n"
    "b = 4\n"
    "c = 5\n"
    "\n"
    "[even]"
    "\n"
    "a = 3\n"
    "b = 4\n"
    "c = 5\n";

int main(int, char**) {
    struct abc {
        int a = 0;
        int b = 0;
        int c = 0;
    };

    auto assert_even = P::require([](auto n) { return n%2==0; }, "must be even");
    auto assert_odd = P::require([](auto n) { return n%2!=0; }, "must be odd");

    // Fields of abc with odd constraint.
    P::specification<abc> abc_odd[] = {
        {"a", &abc::a, assert_odd},
        {"b", &abc::b, assert_odd},
        {"c", &abc::c, assert_odd},
    };

    // Fields of abc with even constraint.
    P::specification<abc> abc_even[] = {
        {"a", &abc::a, assert_even},
        {"b", &abc::b, assert_even},
        {"c", &abc::c, assert_even},
    };

    struct params {
        bool flag = false;
        abc odd;
        abc even;
    } p;

    std::vector<P::specification<params>> specs = {
        P::specification<params>("flag", &params::flag, [](bool v) { return v; }, "Some description")
    };

    for (const auto& s: abc_odd) {
        specs.emplace_back("odd/"+s.key, &params::odd, s);
    }
    for (const auto& s: abc_even) {
        specs.emplace_back("even/"+s.key, &params::even, s);
    }

    P::source_context ctx;
    ctx.source = "ini_text";
    std::stringstream in(ini_text);

    P::specification_map spec_map(specs);
    P::ini_importer importer{in, ctx};
    while (importer) {
        auto h = importer.run_one(p, spec_map);
        if (h && h.value()==P::ini_record_kind::section) {
            std::cout << "Checking section [" << importer.section() << "]\n";
        }
        if (!h) {
            std::cout << P::explain(h.error(), true) << '\n';
        }
    }

    std::cout << "Record values by key:\n";
    for (const auto& s: specs) {
        std::cout << s.key << '\t' << s.write(p).value() << '\n';
    }
}
