#include <iostream>
#include <string>
#include <cstdint>
#include <vector>

#include <parapara/parapara.h>

namespace P = parapara;

P::hopefully<std::pair<std::string, std::size_t>> read_qstring_upto_delim(std::string_view v, const std::string& delim = "") {
    using sn_t = std::pair<std::string, std::size_t>;
    constexpr std::size_t npos = std::string_view::npos;

    if (v.empty()) return sn_t{std::string{}, 0};
    if (v[0]!='"') {
        std::size_t n = npos;
        if (!delim.empty()) n = v.find(delim);

        if (n==npos) return sn_t{v, v.length()};
        else return sn_t{std::string(v, 0, n), n};
    }
    else {
        enum state { regular, q1, q2, esc1, esc2, esc3 } state = q1;
        unsigned char octal_esc = 0;
        std::string s;
        s.reserve(v.length());
        std::size_t n = 0;

        for (; n<v.size(); ++n) {
            char c = v[n];
        redo:
            switch (state) {
            case q1:
                state = regular;
                continue;
            case q2:
                break;
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
            break;
        }

        if (state!=q2) return P::read_failure();

        auto tail = v.substr(n, delim.size());
        if (!tail.empty() && tail!=delim) return P::read_failure();

        return sn_t{std::move(s), n};
    }
}

P::hopefully<std::string> read_qstring(std::string_view v) {
    return read_qstring_upto_delim(v).transform([](std::pair<std::string, std::size_t> &&sn) { return sn.first; });
}

template <typename C,
          std::enable_if_t<std::is_constructible_v<C, std::move_iterator<const std::string*>, std::move_iterator<const std::string*>>, int> = 0>
struct read_dsv_qstring {
    std::string delim;      // field separator
    const bool skip_ws;     // if true, skip leading space/tabs in each field

    explicit read_dsv_qstring(std::string delim=",", bool skip_ws = true):
        delim(std::move(delim)), skip_ws(skip_ws) {}

    P::hopefully<C> operator()(std::string_view v) const {
        constexpr auto npos = std::string_view::npos;
        std::vector<std::string> fields;
        while (!v.empty()) {
            if (skip_ws) {
                auto ns = v.find_first_not_of(" \t");
                if (ns!=npos) v.remove_prefix(ns);
            }
            if (auto hsn = read_qstring_upto_delim(v, delim)) {
                fields.emplace_back(std::move(hsn.value().first));
                v.remove_prefix(hsn.value().second);
                if (!v.empty()) v.remove_prefix(delim.length());
            }
            else {
                return P::unexpected(std::move(hsn.error()));
            }
        }

        return fields.empty()? C{}: C{&fields.front(), &fields.front()+fields.size()};
    }
};

template <typename C,
          std::enable_if_t<P::has_value_type_v<C>, int> = 0,
          std::enable_if_t<std::is_convertible_v<typename C::value_type, std::string>, int> = 0>
struct write_dsv_qstring {
    using value_type = typename C::value_type;
    std::string delim;      // field separator

    write_dsv_qstring(std::string delim = ","): delim(std::move(delim)) {}

    P::hopefully<std::string> operator()(const C& fields) const {
        return P::write_dsv<C>{P::write_qstring_conditional{delim}, delim}(fields, {});
    }
};

template <typename C,
          std::enable_if_t<P::has_value_type_v<C>, int> = 0,
          std::enable_if_t<std::is_convertible_v<typename C::value_type, std::string>, int> = 0>
struct write_dsv_qstring_always {
    using value_type = typename C::value_type;
    std::string delim;      // field separator

    write_dsv_qstring_always(std::string delim = ","): delim(std::move(delim)) {}

    P::hopefully<std::string> operator()(const C& fields) const {
        return P::write_dsv<C>{P::write_qstring_always, delim}(fields, {});
    }
};

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

    std::cout << "# Examples checking escaped characters:\n\n";

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

    std::cout << "# Examples checking quoting for delimiters, with delimiter \"---\":\n\n";
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

    std::cout << "# Unquoting -> requoting examples:\n\n";
    std::string reader_examples[] = {
        "",
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

    std::cout << "# Reading csv quoted strings with read_dsv_qstring:\n\n";
    std::string read_dsv_examples[] = {
        R"(foo,bar,,baz)",
        R"(foo,"bar,baz",quux)"
    };
    auto read = read_dsv_qstring<std::vector<std::string>>{};
    for (const auto& s: read_dsv_examples) {
        std::cout << "input: " << s << "\noutput:\n";

        if (auto mes = read(s)) {
            for (const auto& e: mes.value()) std::cout << '\t' << e << '\n';
        }
        else std::cout << "\terror: " << P::explain(mes.error()) << '\n';
    }

    std::cout << "\n# Encoding-decoding round-trip into '---' separated quoted strings:\n";
    std::vector<std::string> rw_dsv_qstring_examples[] = {
        { "", "x", "" },
        { "a", "b", "c" },
        { "a-b-", "-c-d", "-e-f-" },
        { "a-b", "c--d", "e---f" }
    };
    auto r_dsqs = read_dsv_qstring<std::vector<std::string>>{"---"};
    auto w_dsqs = write_dsv_qstring<std::vector<std::string>>{"---"};
    for (const auto& vs: rw_dsv_qstring_examples) {
        std::cout << "\ninput:\n";
        for (const auto& s: vs) std::cout << '\t' << s << '\n';

        std::cout << "quoted '---'-separated encoding:\n";
        if (auto mq = w_dsqs(vs)) {
            std::cout << '\t' << mq.value() << '\n';
            std::cout << "decoded:\n";
            if (auto mvs = r_dsqs(mq.value())) {
                for (const auto& e: mvs.value()) std::cout << '\t' << e << '\n';
            }
            else std::cout << "\terror: " << P::explain(mvs.error()) << '\n';
        }
        else std::cout << "\terror: " << P::explain(mq.error()) << '\n';
    }
}
