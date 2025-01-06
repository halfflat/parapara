#include <iostream>
#include <optional>
#include <string>
#include <sstream>
#include <vector>

#include <parapara/parapara.h>

using namespace std::literals;

namespace P = parapara;

int main(int, char** argv) {
    struct params {
        std::string foo;
        int bar = -1;
        std::vector<double> baz;
        std::optional<double> xyzzy;
    } p;

    P::reader R = P::default_reader();

    P::specification<params> specs[] = {
        {"foo", &params::foo, P::validator([](auto s) { return s.size()<=5; }, "maximum foo length 5")},
        {"bar", &params::bar},
        {"baz", &params::baz},
        {"xyzzy", &params::xyzzy},
    };

    auto munge = [](std::string_view s) -> std::string { return s=="quux"? "bar"s: std::string(s); };
    P::specification_map spec_map(specs, munge);

    for (int i = 1; argv[i]; ++i) {
        auto h = import_k_eq_v(p, spec_map, R, argv[i]);
        if (!h) {
            parapara::failure f = h.error();
            f.ctx.source = "argv[" + std::to_string(i) + "]";
            f.ctx.record = argv[i];
            std::cout << explain(f, true) << "\n";
        }
    }

    std::cout << "foo: \"" << p.foo << "\"\n";
    std::cout << "bar: " << p.bar << "\n";
    std::cout << "baz:";
    for (auto x: p.baz) std::cout << " " << x;
    std::cout << "\n";
    std::cout << "xyzzy: ";
    if (p.xyzzy) std::cout << p.xyzzy.value() << "\n";
    else std::cout << "nothing\n";
}
