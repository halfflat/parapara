#include <iostream>
#include <string>

#include <parapara/parapara.h>

// Demo defaulted value read/write.

namespace P = parapara;
using P::defaulted;

struct conf {
    defaulted<int> count{10};
    defaulted<std::string> label{"unfashionable"};
};

std::ostream& operator<<(std::ostream& o, const conf& c) {
    return o << "{ count: " << c.count.value() << "; label: '" << c.label.value() << "' }";
}

int main() {
    P::specification<conf> specs[] = {
        {"count", &conf::count, "Integral count, by default 10"},
        {"label", &conf::label, "Label, by default 'unfashionable'"}
    };

    P::reader rdr(P::default_reader(),
        // customize defaulted readers - use 'default' to indicate default value
        P::read_defaulted<int>("default"),
        P::read_defaulted<std::string>("default")
    );

    P::specification_map spec_map(specs);

    conf C;

    std::cout << "initial state:\n" << C << '\n';

    auto h = spec_map.read(C, "count", "20", rdr);
    if (h) h = spec_map.read(C, "label", "smart", rdr);

    if (!h) {
        std::cout << P::explain(h.error()) << '\n';
        return 0;
    }

    std::cout << "\nafter explicit assignments:\n" << C << '\n';

    h = spec_map.read(C, "count", "default", rdr);
    if (h) h = spec_map.read(C, "label", "default", rdr);

    if (!h) {
        std::cout << P::explain(h.error()) << '\n';
        return 0;
    }

    std::cout << "\nafter assignment from 'default':\n" << C << '\n';

    C.count.default_value() = -10;
    C.label.default_value() = "shonky";

    std::cout << "\nafter changing default value in fields:\n" << C << '\n';
}
