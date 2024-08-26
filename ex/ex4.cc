#include <iostream>
#include <optional>
#include <string>
#include <sstream>
#include <vector>

#include <parapara/parapara.h>

namespace P = parapara;

struct params {
    int bar;
    std::optional<double> xyzzy;
};

int main(int, char**) {
    params p1{
        -1,  // should fail validation
        2.0  // should be fine
    };

    params p2{
        -2,  // should fail validation
        0    // should fail validation
    };

    params p3{
        3,   // should be fine
        std::nullopt  // should be fine
    };

    P::specification<params> specs[] = {
        {"bar", &params::bar, P::minimum(0, "bar ≥ 0")},
        {"xyzzy", &params::xyzzy, P::minimum(1, "xyzzy ≥ 1.0")}
    };

    std::cout << "checking record p1:\n";
    for (auto& error: P::validate(p1, specs)) {
        std::cout << error.ctx.key << ": " << explain(error);
    }

    std::cout << "\nchecking record p2:\n";
    for (auto& error: P::validate(p2, specs)) {
        std::cout << error.ctx.key << ": " << explain(error);
    }

    std::cout << "\nchecking record p3:\n";
    for (auto& error: P::validate(p3, specs)) {
        std::cout << error.ctx.key << ": " << explain(error);
    }

    std::cout << "\nusing specification set:\n\n";

    P::specification_set<params> S(specs);
    P::writer W = P::default_writer();

    auto report = [&S, &W](const params& rec, const P::failure& error) {
        std::cout << error.ctx.key << '=' << S.write(rec, error.ctx.key, W).value_or("?") << ": " << explain(error);
    };

    std::cout << "checking record p1:\n";
    for (auto& error: P::validate(p1, specs)) report(p1, error);

    std::cout << "\nchecking record p2:\n";
    for (auto& error: P::validate(p2, specs)) report(p2, error);

    std::cout << "\nchecking record p3:\n";
    for (auto& error: P::validate(p3, specs)) report(p3, error);

}
