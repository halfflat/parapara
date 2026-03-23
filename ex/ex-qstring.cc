#include <iostream>
#include <string>
#include <cstdint>
#include <vector>

#include <parapara/parapara.h>

namespace P = parapara;

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
    auto read = P::read_dsv_qstring<std::vector<std::string>>{};
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

    auto r_dsqs = P::read_dsv_qstring<std::vector<std::string>>{"---"};
    auto w_dsqs = P::write_dsv_qstring<std::vector<std::string>>{"---"};

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
