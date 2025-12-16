#include <iostream>
#include <string>
#include <cstdint>
#include <vector>

#include <parapara/parapara.h>

namespace P = parapara;

P::hopefully<std::string> write_qstring(const std::string& s, const std::string& delim = "") {
    // \? is explicitly omitted: avoiding trigraphs is not relevant.
    // \' is explicitly omitted: unnecessary in double-quoted string representation.
    constexpr char esc_tbl[128][4] = {
        // 0x00
        "000", "001", "002", "003", "004", "005", "006", "a",
        "b",   "t",   "n",   "v",   "f",   "r",   "016", "017",
        // 0x10
        "020", "021", "022", "023", "024", "025", "026", "027",
        "030", "031", "032", "033", "034", "035", "036", "037",
        // 0x20
        "",    "",    "\"",  "",    "",    "",    "",    "",
        "",    "",    "",    "",    "",    "",    "",    "",
        // 0x30
        "",    "",    "",    "",    "",    "",    "",    "",
        "",    "",    "",    "",    "",    "",    "",    "",
        // 0x40
        "",    "",    "",    "",    "",    "",    "",    "",
        "",    "",    "",    "",    "",    "",    "",    "",
        // 0x50
        "",    "",    "",    "",    "",    "",    "",    "",
        "",    "",    "",    "",    "\\",  "",    "",    ""
        // remainder is all zero
    };

    if (s.empty()) return s;

    bool esc = false;
    std::string q;
    q.reserve(2+2*s.length());
    q.push_back('"');

    // quote if leading space
    esc |= s[0]==' ';

    // quote if string contains 

    for (char c: s) {
        std::uint8_t u = c;
        if (u<128 && esc_tbl[u][0]) {
            esc = true;
            q.push_back('\\');
            q.append(esc_tbl[u]);
        }
        else q.push_back(u);
    }
    q.push_back('"');

    return esc? q: s;
}



int main() {
    parapara::reader R = parapara::default_reader();
    parapara::writer W = parapara::default_writer();

    std::string writer_examples[] = {
        R"(apple badge)",
        R"("apple badge")",
        R"(apple 'badge')",
        R"(apple\n\tbadge)",
        "apple\n\tbadge",
        R"( apple badge)",
        "\n\rapple\037 badge"
    };

    for (const auto& s: writer_examples) {
        std::cout << "value: " << s << "\n repn: " << write_qstring(s).value() << "\n\n";
    }
}
