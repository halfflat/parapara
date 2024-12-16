#include <iostream>
#include <list>
#include <vector>

#include <parapara/parapara.h>

int main() {
    parapara::reader R = parapara::default_reader();

    std::string repn = "23.4, 178.9, NaN";
    auto h = R.read<std::vector<float>>(repn);

    if (!h) std::cout << explain(h.error()) << '\n';
    else {
        for (auto& f: h.value()) std::cout << f << '\n';
    }

    parapara::reader S(
        parapara::default_reader(),
        parapara::read_dsv<std::list<double>>(parapara::read_cc<double>, ";")
    );

    std::string repn2 = "23.4; 178.9; NaN; inf";
    auto h2 = S.read<std::list<double>>(repn2);
    if (!h2) std::cout << explain(h2.error()) << '\n';
    else {
        for (auto& f: h2.value()) std::cout << f << '\n';
    }

    struct record {
        int x = 0;
        std::vector<int> xs;
    } rec;

    parapara::specification x_spec{"x", &record::x};
    if (auto hv = x_spec.assign(rec, 7)) {
        std::cout << "ok; record.x = " << rec.x << '\n';
    }
    else {
        std::cout << explain(hv.error()) << '\n';
    }

    parapara::specification xs_spec{"xs", &record::xs};
    if (auto hv = xs_spec.read(rec, "3, 4, 5")) {
        std::cout << "ok; record.xs = ";
        for (auto& f: rec.xs) std::cout << f << ' ';
        std::cout << '\n';
    }
    else {
        std::cout << explain(hv.error()) << '\n';
    }

    parapara::validator require_even([](auto n) { return !(n%2); }, "value is even");

    parapara::specification x_spec2("x", &record::x,
        require_even &=
        parapara::at_least(5) &=
        parapara::at_most(10, "value is at most 10"));

    if (auto hv = x_spec2.assign(rec, 12)) {
        std::cout << "ok; record.x = " << rec.x << '\n';
    }
    else {
        std::cout << explain(hv.error()) << '\n';
    }
}
