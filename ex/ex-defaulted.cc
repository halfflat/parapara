#include <iostream>
#include <string>

#include <parapara/parapara.h>

// Demo defaulted value read/write.

namespace P = parapara;
using P::defaulted;

// 'fish' has custom reader, writer.
struct fish {
    enum { michi = 0, kingyo = 3, medaka = 4 } variety = michi;
};

constexpr fish unknown_fish = { fish::michi };
constexpr fish goldfish = { fish::kingyo };
constexpr fish rice_fish = { fish::medaka };

P::hopefully<fish> read_fish(std::string_view v) {
    if (v == "goldfish") return goldfish;
    else if (v == "rice fish") return rice_fish;
    else if (v == "unknown") return unknown_fish;
    else return P::invalid_value();
}

P::hopefully<std::string> write_fish(const fish& f) {
    switch (f.variety) {
    case fish::michi: return "unknown";
    case fish::kingyo: return "goldfish";
    case fish::medaka: return "rice fish";
    default: return P::invalid_value();
    }
}

std::ostream& operator<<(std::ostream& o, const fish& f) {
    if (auto h = write_fish(f)) return o << h.value();
    else return o << "invalid fish"; // poor thing
}


struct conf {
    defaulted<int> count{10};
    defaulted<std::string> label{"unfashionable"};
    defaulted<fish> barry{rice_fish};
};

std::ostream& operator<<(std::ostream& o, const conf& c) {
    return o << "{ count: " << c.count.value() << "; label: '" << c.label.value() << "'; barry: " << c.barry.value() << " }";
}

int main() {
    P::specification<conf> specs[] = {
        {"count", &conf::count, "Integral count, by default: 10"},
        {"label", &conf::label, "Label, by default: 'unfashionable'"},
        {"barry", &conf::barry, "Barry the fish, by default: rice fish"}
    };

    P::reader rdr(P::default_reader(),
        // customize defaulted readers - use 'default' to indicate default value except for barry
        P::read_defaulted<int>("default"),
        P::read_defaulted<std::string>("default"),
        P::read_defaulted<fish>(read_fish, "bazza")
    );

    P::writer wtr(P::default_writer(),
        // customize defaulted readers - use 'default' to indicate default value except for barry
        P::write_defaulted<int>("default"),
        P::write_defaulted<std::string>("default"),
        P::write_defaulted<fish>(write_fish, "bazza")
    );

    P::specification_map spec_map(specs);

    conf C;

    std::cout << "initial state:\n" << C << '\n';
    std::cout << "\nini-style representation:\n";
    P::export_ini(C, specs, wtr, std::cout);

    auto h = spec_map.read(C, "count", "20", rdr);
    if (h) h = spec_map.read(C, "label", "smart", rdr);
    if (h) h = spec_map.read(C, "barry", "goldfish", rdr);

    if (!h) {
        std::cout << P::explain(h.error()) << '\n';
        return 1;
    }

    std::cout << "\n\nafter explicit assignments:\n" << C << '\n';
    std::cout << "\nini-style representation:\n";
    P::export_ini(C, specs, wtr, std::cout);

    h = spec_map.read(C, "count", "default", rdr);
    if (h) h = spec_map.read(C, "label", "default", rdr);
    if (h) h = spec_map.read(C, "barry", "bazza", rdr);

    if (!h) {
        std::cout << P::explain(h.error()) << '\n';
        return 0;
    }

    std::cout << "\n\nafter assignment from 'default' (or 'bazza' for barry):\n" << C << '\n';
    std::cout << "\nini-style representation:\n";
    P::export_ini(C, specs, wtr, std::cout);

    C.count.default_value() = -10;
    C.label.default_value() = "shonky";
    C.barry.default_value() = unknown_fish;

    std::cout << "\n\nafter changing default value in fields:\n" << C << '\n';
}
