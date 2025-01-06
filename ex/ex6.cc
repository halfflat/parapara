#include <iomanip>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>

#include <parapara/parapara.h>

using namespace std::literals;

namespace P = parapara;

// Demo for alternative INI-like syntax, using custom_ini_parser defined below
// instead of parapara::simple_ini_parser.

const char* ini_text =
    "// Comments introduced with //\n"
    " a = 3   // Comments can come at end\n"
    " b = 'fish // bar' // Values can be optionally quoted\n"
    "\n"
    "[top]\n"
    " a = 1\n"
    "[./sub] // Can finangle 'relative' section names\n"
    " a = 2\n"
    "[./subsub]\n"
    " a = 3\n"
    "[../../sub2]\n"
    " a = 4\n";

// Custom ini parser:
// * Comments are introduced with // and can follow a key assignment
// * Values in key assignment can optionally be quoted with '

using P::ini_record;
using P::ini_record_kind;

ini_record custom_ini_parser(std::string_view v) {
    using token = ini_record::token;
    using size_type = std::string_view::size_type;

    constexpr size_type npos = std::string_view::npos;
    constexpr std::string_view ws{" \t\f\v\r\n"};

    auto comment_at = [](std::string_view v, size_type p) { return v.substr(p, 2)=="//"; };
    size_type b = v.find_first_not_of(ws);

    // empty or comment?
    if (b==npos || comment_at(v, b)) return ini_record{ini_record_kind::empty};

    // section heading?
    if (v[b]=='[') {
        size_type e = v.find(']');

        // check for malformed heading
        if (e==npos) return {ini_record_kind::syntax_error, token("", b+1)};

        if (e+1<v.length()) {
            auto epilogue = v.find_first_not_of(ws, e+1);
            if (epilogue!=npos && !comment_at(v, epilogue)) {
                return {ini_record_kind::syntax_error, token("", epilogue)};
            }
        }

        b = v.find_first_not_of(ws, b+1);
        e = v.find_last_not_of(ws, e-1);

        return {ini_record_kind::section, token(v.substr(b, e>=b? e+1-b: 0), b+1)};
    }

    // expect key first, followed by ws and eol, =, or //.
    size_type j = std::min(v.find('=', b), v.find("//", b));
    token key_token{v.substr(b, j==b? 0: v.find_last_not_of(ws, j-1)+1-b), b+1};

    // key without value?
    if (j==npos || v[j]!='=') {
        return {ini_record_kind::key, key_token};
    }

    // skip to text after =, look for value
    size_type eq = j;
    size_type value_cindex = eq;

    if (j<v.length()) {
        j = v.find_first_not_of(ws, j+1);
        if (j!=npos && !comment_at(v, j)) {
            value_cindex = j+1;

            // if value is not quoted, take text up to eol or first eol, discarding trailing ws
            if (v[j]!='\'') {
                size_type end = v.find("//", j);
                if (end!=npos) --end;

                return {ini_record_kind::key_value, key_token,
                        token{v.substr(j, v.find_last_not_of(ws, end)-j+1), value_cindex}};
            }
            else {
                // quoted value; take until next unescaped '
                std::string value;
                size_type epilogue = npos;
                bool esc = false;

                for (size_type i = j+1; i<v.length(); ++i) {
                    if (esc) {
                        value += v[i];
                        esc = false;
                    }
                    else if (v[i]=='\'') {
                        epilogue = i+1;
                        break;
                    }
                    else if (v[i]=='\\') {
                        esc = true;
                    }
                    else {
                        value += v[i];
                    }
                }

                // unterminated quoted value or escaped eol?
                if (epilogue==npos || esc) {
                    return {ini_record_kind::syntax_error, token("", j)};
                }

                // extra stuff following value that is not a comment?
                if (epilogue<v.length()) {
                    epilogue = v.find_first_not_of(ws, epilogue);
                    if (epilogue!=npos && !comment_at(v, epilogue)) {
                        return {ini_record_kind::syntax_error, token("", epilogue)};
                    }
                }

                return {ini_record_kind::key_value, key_token, token{value, value_cindex}};
            }
        }
    }
    // key with empty value
    return {ini_record_kind::key_value, key_token, token{"", eq}};
}

// Use a custom line-by-line ini importer to handle relative section headings
template <typename Record>
P::hopefully<void> custom_import_ini(Record& rec, const P::specification_map<Record>& specs, std::istream& in)
{
    constexpr auto npos = std::string_view::npos;

    P::ini_style_importer importer(custom_ini_parser, in);
    while (importer) {
        std::string prev_sec{importer.section()};
        auto h = importer.run_one(rec, specs, P::default_reader(), "/");

        if (!h) {
            std::cout << P::explain(h.error(), true) << '\n';
            continue;
        }

        if (h.value() != P::ini_record_kind::section) continue;

        std::string_view new_sec = importer.section();
        if (new_sec.substr(0, 2)=="./") {
            std::string section{prev_sec};
            section += new_sec.substr(1);
            importer.section(section);
        }
        else if (new_sec.substr(0, 3)=="../") {
            std::string_view prefix(prev_sec);
            do {
                new_sec.remove_prefix(3);
                if (auto tail = prefix.rfind('/'); tail != npos) {
                    prefix = prefix.substr(0, tail);
                }
            } while (new_sec.substr(0, 3)=="../");

            std::string section{prefix};
            section += "/";
            section += new_sec;
            importer.section(section);
        }
    }
    return {};
}

int main(int, char**) {
    struct params {
        int a = 0;
        std::string b;
        bool top = false;
        int top_a = 0;
        int top_sub_a = 0;
        int top_sub_subsub_a = 0;
        int top_sub2_a = 0;
    } p;

    P::specification<params> specs[] = {
        {"a", &params::a},
        {"b", &params::b},
        {"top", &params::top},
        {"top/a", &params::top_a},
        {"top/sub/a", &params::top_sub_a},
        {"top/sub/subsub/a", &params::top_sub_subsub_a},
        {"top/sub2/a", &params::top_sub2_a}
    };

    P::source_context ctx;
    ctx.source = "ini_text";
    std::stringstream in(ini_text);

    P::specification_map spec_map(specs);
    custom_import_ini(p, spec_map, in);

    std::cout << "Record values by key:\n";
    for (const auto& s: specs) {
        std::cout << s.key << '\t' << s.write(p).value() << '\n';
    }
}
