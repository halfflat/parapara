#include <iostream>
#include <string>
#include <vector>

#include <parapara/parapara.h>

namespace P = parapara;

P::hopefully<std::string> write_qstring(const std::string& s) {
    
}

int main() {
    parapara::reader R = parapara::default_reader();
    parapara::writer W = parapara::default_writer();

    std::string writer_examples[] = {
        R"(apple badge)",
        R"("apple badge")",
        R"(apple 'badge')",
        R"(apple\n\tbadge)",
    };

    for (const auto& s: writer_examples) {
        std::cout << "value: " << s << "\n repn: " << write_qstring(s) << "\n\n";
    }
}
