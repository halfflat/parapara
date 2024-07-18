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
    "   baz = 2.8\n"
    "xy ZZ y = 12\n"
    "\n"
    "[ blurgle ]\n"
    "\n"
    "   baz\n";

int main(int, char**) {
    struct params {
        std::string foo;
        std::string bar;
        std::vector<double> baz;
        int xyzzy = -1;
        std::optional<bool> blurgle_baz;
    } p;

    P::reader R = P::default_reader();

    P::specification<params> specs[] = {
        {"foo", &params::foo, P::validator([](auto s) { return s.size()<=10; }, "maximum foo length 10")},
        {"bar", &params::bar},
        {"baz", &params::baz},
        {"xyzzy", &params::xyzzy},
        {"blurgle.baz", &params::blurgle_baz}
    };

    P::specification_set specset(specs, parapara::keys_lc_nows);

    std::stringstream in(ini_text);
    auto h = import_ini(p, specset, R, in, ".");
    if (!h) {
        h.error().ctx.source = "<ini_text>";
        std::cout << explain(h.error(), true) << "\n";
    }

    std::cout << "foo: \"" << p.foo << "\"\n";
    std::cout << "bar: \"" << p.bar << "\"\n";
    std::cout << "baz:";
    for (auto x: p.baz) std::cout << " " << x;
    std::cout << "\n";
    std::cout << "xyzzy: " << p.xyzzy << "\n";
    std::cout << "blurgle.baz: ";
    if (p.blurgle_baz.has_value()) std::cout << std::boolalpha << p.blurgle_baz.value() << "\n";
    else std::cout << "nothing\n";
}
