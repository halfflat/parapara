#include <iostream>
#include <string>
#include <cstdint>
#include <vector>

#include <parapara/parapara.h>

namespace P = parapara;

#if 0
P::hopefully<std::string> read_qstring(std::string_view v) {
    if (v.empty() || v[0] !='"') return std::string(v);

    enum state { regular, q1, q2, esc1, esc2, esc3 } state = q1;
    unsigned char octal_esc = 0;
    std::string s;
    s.reserve(v.length());

    for (auto c: v) {
    redo:
        switch (state) {
        case q1:
            state = regular;
            continue;
        case regular:
            switch (c) {
            case '"':
                state = q2;
                continue;
            case '\\':
                state = esc1;
                continue;
            }
            s.push_back(c);
            continue;
        case q2:
            return P::read_failure();
        case esc1:
            switch (c) {
            case 'a':
                c = 7; break;
            case 'b':
                c = 8; break;
            case 't':
                c = 9; break;
            case 'n':
                c = 10; break;
            case 'v':
                c = 11; break;
            case 'f':
                c = 12; break;
            case 'r':
                c = 13; break;
            default:
                if (c>='0' && c<='7') {
                    octal_esc = c-'0';
                    state = esc2;
                    continue;
                }
            }
            s.push_back(c);
            state = regular;
            continue;
        case esc2:
            if (c>='0' && c<='7') {
                octal_esc = 8*octal_esc + (c-'0');
                state = esc3;
                continue;
            }
            s.push_back(octal_esc);
            state = regular;
            goto redo;
        case esc3:
            if (c>='0' && c<='7') {
                octal_esc = 8*octal_esc + (c-'0');
                s.push_back(octal_esc);
                state = regular;
                continue;
            }
            s.push_back(octal_esc);
            state = regular;
            goto redo;
        }
    }
    if (state!=q2) return P::read_failure();
    else return s;
}

struct write_qstring {
    bool always_quote_ = false;
    std::string delim_;

    explicit write_qstring(bool always_quote = false):
        always_quote_(always_quote)
    {}

    explicit write_qstring(std::string delim):
        always_quote_(false), delim_(std::move(delim))
    {}

    P::hopefully<std::string> operator()(const std::string& s) const {
        constexpr auto npos = std::string_view::npos;

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

        bool quote = always_quote_;

        if (s.empty()) return quote? "\"\"": "";

        // The scratch string is used to test for ambiguous terminal deliminator substrings
        // and for staging a quoted and escaped output string.

        std::string scratch;
        scratch.reserve(2+4*s.length()+2*delim_.length());

        // Quote if string has leading space.

        quote |= s[0]==' ';

        // Quote if string contains delim or partially contains delim in a manner
        // that could lead to ambiguous determination of extent if prefixed or suffixed
        // by delim.

        if (!quote) {
            switch (auto dlen = delim_.length()) {
            case 0:
                break;
            case 1:
                quote |= s.find(delim_[0]) != npos;
                break;
            default:
                scratch.assign(delim_, 1);
                scratch.append(s);
                scratch.append(delim_, 0, dlen-1);
                quote |= scratch.find(delim_) != npos;
            }
        }

        // Quote and escape special characters from esc_tbl if present.

        scratch.assign("\"");
        for (char c: s) {
            std::uint8_t u = c;
            if (u<128 && esc_tbl[u][0]) {
                quote = true;
                scratch.push_back('\\');
                scratch.append(esc_tbl[u]);
            }
            else scratch.push_back(u);
        }
        scratch.push_back('"');

        return quote? scratch: std::string(s);
    }
};
#endif

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
        std::cout << "value: " << s << "\n repn: " << P::write_qstring(s).value() << "\n\n";
    }

    std::cout << "Examples checking quoting for delimiters, with delimiter \"an'a\":\n\n";

    std::string writer_examples_b[] = {
        "cake",
        "cake an'a banana",
        "a coffee",
        "an apple",
        "n'a coffee",
        "cake a",
        "cake an",
        "cake an'",
        "cake and",
    };

    for (const auto& s: writer_examples_b) {
        std::cout << "value: " << s << "\n repn: " << P::write_qstring_conditional{"an'a"}(s).value() << "\n\n";
    }

    std::cout << "Examples checking quoting for delimiters, with delimiter \"---\":\n\n";
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
        std::cout << "value: " << s << "\n repn: " << P::write_qstring_conditional{"---"}(s).value() << "\n\n";
    }

    std::cout << "Unquoting -> requoting examples:\n\n";
    std::string reader_examples[] = {
        R"("\7\07\007")",
        R"("a \"quoted\" bit")",
        R"("\weird\ escapes\!")",
        R"(not actu\ally quoted")",
        "\"embedded NUL\\0!\"",
        R"("no closing quote)",
        R"("stuff after" quote)"
    };
    for (const auto& s: reader_examples) {
        std::cout << "encoded:    " << s << "\nre-encoded: " << P::read_qstring(s).and_then(P::write_qstring).value_or("<read error>") << "\n\n";
    };
}
