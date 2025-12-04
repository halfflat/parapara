#include <algorithm>
#include <iostream>
#include <string>
#include <type_traits>
#include <vector>
#include <parapara/parapara.h>

namespace P = parapara;

// Record type structs:

struct foobinator {
    double power = 0;
    bool overdrive = false;
};

struct superfoobinator: foobinator {
    int dooper = 1;
    bool boggle = false;
};

struct record {
    double quuxity = 0.;
    foobinator F, G;
    superfoobinator H;
};

// A bit of syntactic sugar (even if it does involve some copies):

template <typename C>
C extend(C&& container, const std::initializer_list<typename C::value_type>& il) {
    std::copy(std::begin(il), std::end(il), std::back_inserter(container));
    return container;
}

// Construct specifications for members of foobinator and superfoobinator:

template <typename X>
std::vector<P::specification<X>> make_foobinator_specifications() {
    return {
        {"power", &X::power, "foo power level"},
        {"overdrive", &X::overdrive, "engage foobinator overdrive"}
    };
}

template <typename X>
std::vector<P::specification<X>> make_superfoobinator_specifications() {
    return extend(make_foobinator_specifications<X>(), {
        {"dooper", &X::dooper, "super foobinator dooper rank"},
        {"boggle", &X::boggle, "boogle?"}
     });
}

// Add specifications for members of record subobjects that have their own sets of specifications.
// Keys and field descriptions are given a prefix as supplied.

template <typename R, typename X>
void add_delegates_for(std::vector<P::specification<R>>& specs, X R::* field_ptr, std::string key_pfx,
    const std::vector<P::specification<X>>& x_specs, std::string desc_pfx = "")
{
    for (const auto& x_spec: x_specs) {
        specs.emplace_back(key_pfx+x_spec.key, field_ptr, x_spec, desc_pfx+x_spec.description);
    }
}

// Construct specifications for members of the record type:

std::vector<P::specification<record>> make_record_specifications() {
    std::vector<P::specification<record>> specs = {
        {"quuxity", &record::quuxity, "scalar ineffable quuxity"}
    };

    auto foobinator_specs = make_foobinator_specifications<foobinator>();
    auto superfoobinator_specs = make_superfoobinator_specifications<superfoobinator>();

    add_delegates_for(specs, &record::F, "F/", foobinator_specs, "F mode: ");
    add_delegates_for(specs, &record::G, "G/", foobinator_specs, "G mode: ");
    add_delegates_for(specs, &record::H, "H/", superfoobinator_specs, "H mode: ");

    return specs;
}

int main() {
    record R = {3.1, {6.25, true}, {6.5, false}, {8.75, true, 10, false}};
    std::vector<P::specification<record>> specs = make_record_specifications();

    P::export_ini(R, specs, P::default_writer(), std::cout);
}

