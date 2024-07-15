#include <iostream>
#include <list>
#include <vector>

#include <parapara/parapara.h>

std::string failure_name(parapara::failure f) {
    if (std::get_if<parapara::read_failure>(&f)) return "read failure";
    if (std::get_if<parapara::invalid_value>(&f)) return "invalid value";
    if (std::get_if<parapara::unsupported_type>(&f)) return "unsupported type";
    if (std::get_if<parapara::internal_type_mismatch>(&f)) return "internal error";
    return "?";
}

int main() {
    parapara::reader R = parapara::default_reader();

    std::string repn = "23.4, 178.9, NaN";
    auto h = R.read<std::vector<float>>(repn);

    if (!h) std::cout << "failure\n";
    else {
        for (auto& f: h.value()) std::cout << f << '\n';
    }

    parapara::reader S(
        parapara::default_reader(),
        parapara::read_dsv<std::list<double>>(parapara::read_cc<double>, ";")
    );

    std::string repn2 = "23.4; 178.9; NaN; inf";
    auto h2 = S.read<std::list<double>>(repn2);
    if (!h2) std::cout << "failure\n";
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
        std::cout << "failure: context key: " << context(hv.error()).key << '\n';
    }

    parapara::specification xs_spec{"xs", &record::xs};
    if (auto hv = xs_spec.read(rec, "3, 4, 5")) {
        std::cout << "ok; record.xs = ";
        for (auto& f: rec.xs) std::cout << f << ' ';
        std::cout << '\n';
    }
    else {
        std::cout << "failure: " << failure_name(hv.error()) << "; context key: " << context(hv.error()).key << '\n';
    }

    parapara::specification x_spec2("x", &record::x,
        parapara::assert([](int n) { return n%2==0; }, "value is even"));
    if (auto hv = x_spec2.assign(rec, 8)) {
        std::cout << "ok; record.x = " << rec.x << '\n';
    }
    else {
        std::cout << "failure: " << failure_name(hv.error()) << "; context key: " << context(hv.error()).key << '\n';
    }
}
