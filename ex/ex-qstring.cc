#include <iostream>
#include <string>
#include <cstdint>
#include <vector>

#include <parapara/parapara.h>

namespace P = parapara;

P::hopefully<std::string> write_qstring(const std::string& s, const std::string& delim = "") {
    using strsize_t = std::string::size_type;

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

    // quote and escape special characters from esc_tbl.

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

    // quote if string contains delim or begins with a non-empty tail of delim
    // or ends with a non-empty prefix of delim. It is over-cautious for utf8
    // encoded strings.

    if (!esc && !delim.empty()) {
        constexpr strsize_t npos = std::string_view::npos;
        std::string_view sv(s);
        std::string_view dv(delim);

        strsize_t sv_sz = sv.length();
        strsize_t dv_sz = dv.length();

        if (dv_sz==1) {
            esc |= sv.find(dv[0]) != npos;
        }
        else {
            strsize_t max_subdelim = std::min(sv_sz, dv_sz-1);
            for (strsize_t k = 1; k <= max_subdelim && !esc; ++k) {
                esc |= dv.substr(dv_sz-k) == sv.substr(0, k);
            }
 
            for (strsize_t k = 1; k <= max_subdelim && !esc; ++k) {
                esc |= dv.substr(0, k) == sv.substr(sv_sz-k);
            }

            if (!esc) esc |= sv.find(dv) != npos;
        }
    }

    return esc? q: s;
}



int main() {
    parapara::reader R = parapara::default_reader();
    parapara::writer W = parapara::default_writer();

    std::string writer_examples_a[] = {
        R"(apple badge)",
        R"("apple badge")",
        R"(apple 'badge')",
        R"(apple\n\tbadge)",
        "apple\n\tbadge",
        R"( apple badge)",
        "\n\rapple\037 badge"
    };

    std::cout << "Examples checking escaped characters:\n\n";

    for (const auto& s: writer_examples_a) {
        std::cout << "value: " << s << "\n repn: " << write_qstring(s).value() << "\n\n";
    }

    std::cout << "Examples checking quoting for delimiters, with delimiter 'and':\n";

    std::string writer_examples_b[] = {
        "cake",
        "cake and coffee",
        "a coffee",
        "an coffee",
        "and coffee",
        "d coffee",
        "nd coffee",
        "cake a",
        "cake an",
        "cake and",
        "cake nd",
        "cake d",
    };

    for (const auto& s: writer_examples_b) {
        std::cout << "value: " << s << "\n repn: " << write_qstring(s, "and").value() << "\n\n";
    }

    std::cout << "Examples checking quoting for delimiters, with delimiter '---':\n\n";
    std::string writer_examples_c[] = {
        "pomme-frites",
        "pomme--frites",
        "pomme---frites",
        "-pomme frites",
        "--pomme frites",
        "---pomme frites",
        "pomme frites-",
        "pomme frites--",
        "pomme frites---",
    };

    for (const auto& s: writer_examples_c) {
        std::cout << "value: " << s << "\n repn: " << write_qstring(s, "---").value() << "\n\n";
    }

}
