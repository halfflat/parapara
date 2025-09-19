#include <iostream>
#include <optional>
#include <string>
#include <sstream>
#include <vector>

#include <any>
#include <stdexcept>
#include <string_view>
#include <type_traits>
#include <parapara/parapara.h>

struct params {
    int bar;
    double xyzzy;
};

int main(int, char**) {
    namespace P = parapara;

    P::specification<params> specs[] = {
        {"bar", &params::bar, P::at_least(0, "bar ≥ 0")},
        {"xyzzy", &params::xyzzy, P::at_least(1, "xyzzy ≥ 1.0")}
    };

    P::specification_map<params> S(specs);

    params p1 = { 10, 20.0 };
    P::keyed_view view(p1, S);

    std::cout << "typed retrieval by key:\n";
    {
        int bar = view["bar"];
        double xyzzy = view["xyzzy"];

        std::cout << "bar: " << bar << "\nxyzzy: " << xyzzy << '\n';
    }

    std::cout << "\nassignment by key, setting bar = 5, baz = 30.0:\n";
    {
        view["bar"] = 5;
        view["xyzzy"] = 30.0;

        std::cout << "bar: " << p1.bar << "\nxyzzy: " << p1.xyzzy << '\n';
    }

    std::cout << "\ncheck validation by assigning xyzzy = 0:\n";
    {
        try {
            view["xyzzy"] = 0.0;
            std::cout << "(should not get here) xyzzy: " << p1.xyzzy << '\n';
        }
        catch (parapara::parapara_error& e) {
            std::cout << "parapara exception: " << e.what() << '\n';
        }
    }

    params p2 = {-1, -3.0};
    std::cout << "\nrebind view to another record instance\n";
    {
        view.rebind(p2);
        int bar = view["bar"];
        double xyzzy = view["xyzzy"];

        std::cout << "bar: " << bar << "\nxyzzy: " << xyzzy << '\n';
    }

    std::cout << "\n... and confirm that a copy refers to the same record\n";
    {
        P::keyed_view other = view;
        other["bar"] = 50;

        std::cout << "new p2.bar: " << p2.bar << '\n';
    }

    std::cout << "\nChecking const view, too.\n";
    {
        const params cp1 = p1;
        P::keyed_view cview(cp1, S);

        int bar = cview["bar"];
        std::cout << "bar: " << bar << '\n';

        P::const_keyed_view cview2(view);
        double xyzzy = cview2["xyzzy"];
        std::cout << "xyzzy: " << xyzzy << '\n';
    }
}
