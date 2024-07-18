#include <iostream>
#include <string>
#include <sstream>
#include <vector>

#include <parapara/parapara.h>

std::string explain(const parapara::failure& f) {
    std::stringstream s;
    const auto& ctx = context(f);

    s << name(f);

    // Is there source, line, cindex info?
    if (!ctx.source.empty()) {
        s << ": " << ctx.source;

        if (ctx.nr) s << ":" << ctx.nr;
        if (ctx.cindex) s << ":" << ctx.cindex;
    }

    if (!ctx.key.empty()) {
        s << ": key=\"" << ctx.key << "\"";
    }

    if (auto p = std::get_if<parapara::invalid_value>(&f); p && !p->constraint.empty()) {
        s << ": " << p->constraint;
    }

    return s.str();
}


int main(int argc, char** argv) {
    struct params {
        std::string foo;
        int bar = -1;
        std::vector<double> baz;
    } p;

    parapara::reader R = parapara::default_reader();

    parapara::specification<params> specs[] = {
        {"foo", &params::foo, parapara::validator([](auto s) { return s.size()<=5; }, "maximum length 5")},
        {"bar", &params::bar},
        {"baz", &params::baz}
    };

    parapara::specification_set<params> specset(specs);

    for (int i = 1; argv[i]; ++i) {
        auto h = import_k_eq_v(p, specset, R, argv[i]);
        if (!h) {
            context(h.error()).source = "argv[" + std::to_string(i) + "]";
            std::cout << explain(h.error()) << "\n";
        }
    }

    std::cout << "foo: \"" << p.foo << "\"\n";
    std::cout << "bar: " << p.bar << "\n";
    std::cout << "baz:";
    for (auto x: p.baz) std::cout << " " << x;
    std::cout << "\n";
}
