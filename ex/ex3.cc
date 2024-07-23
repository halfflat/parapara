#include <iomanip>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>

#include <parapara/parapara.h>

using namespace std::literals;

namespace P = parapara;

const char* ini_text =
    "# A comment\n"
    "   # Another comment with leading ws\n"
    "\n"
    "   foo = two words  \n"
    "   bar = hash#\n"
    "   baz = 2.8, 99\n"
    "xy ZZ y = 12\n"
    "\n"
    "[ blurgle ]\n"
    "\n"
    "   baz\n"
    "   quux = 1,3,4\n"
    "\n"
    "[ zoinks ]\n"
    "   zoinks!\n"
    "\n";

int main(int, char**) {
    struct params {
        std::string foo;
        std::string bar;
        std::vector<double> baz;
        int xyzzy = -1;
        std::optional<bool> blurgle_baz;
        std::vector<int> quux;
        bool zoinks = false;
    } p;

    P::reader R = P::default_reader();

    P::specification<params> specs[] = {
        {"foo", &params::foo,
            P::require([](auto s) { return s.size()<=10; }, "maximum foo length 10"),
            "short name for foo (maximum 10 characters)"},
        {"baz", &params::baz, "bazziness vector:\n  0-3: not very bazzy\n  3+ : quite bazzy indeed"},
        {"bar", &params::bar},
        {"xyzzy", &params::xyzzy},
        {"blurgle.baz", &params::blurgle_baz, "always blurgle bazzes?"},
        {"zoinks.zoinks!", &params::zoinks,
             P::require([](bool x) { return x; }, "zoinks! must be true"),
             "zoinks!?"},
        {"blurgle.quux", &params::quux, ""}
    };

    P::specification_set specset(specs, parapara::keys_lc_nows);

    std::stringstream in(ini_text);
    auto h = import_ini(p, specset, R, in, ".");
    if (!h) {
        h.error().ctx.source = "<ini_text>";
        std::cout << explain(h.error(), true) << "\n";
    }

    if (auto h = P::export_ini(p, specs, P::default_writer(), std::cout, "."); !h) {
        std::cout << explain(h.error()) << '\n';
    }
}
