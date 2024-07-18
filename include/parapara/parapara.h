#pragma once
#include <any>
#include <expected>
#include <functional>
#include <iterator>
#include <stdexcept>
#include <string>
#include <string_view>
#include <typeindex>
#include <unordered_map>
#include <variant>
#include <vector>

namespace parapara {

// Header organization:
//
// Section I: Failure handling and exceptions
//
// Section II: Reader and writer helpers
//
// Section III: Reader and writer classes, defaults
//
// Section IV: Parameter specifications, parameter sets
//
// Section V: Validation helpers
//
// Section VI: Importers and exporters


// I. Failure handling and exceptions
// ----------------------------------

// Structs representing failure outcomes. The failure_context struct holds
// information about the data which triggered the error and is intended
// to be used when constructing helpful error messages.

struct failure_context {
    std::string key;     // associated parameter key, if any.
    std::string source;  // e.g. file name
    unsigned nr = 0;     // record number/line number (1-based).
    unsigned cindex = 0; // character index into record/line (1-based).
};

// parameter value failed to parse
struct read_failure {
    failure_context ctx;
};

// parameter value failed to validate
struct invalid_value {
    std::string constraint;  // optionally supplied by a parameter constraint
    failure_context ctx;
};

// missing read or writer for parameter type
struct unsupported_type {
    failure_context ctx;
};

// internal error: something went awry with all the type-erasure
struct internal_type_mismatch {
    failure_context ctx;
};

// key missing from specification set
struct unrecognized_key {
    failure_context ctx;
};

// general parse failure in importer
struct bad_syntax {
    failure_context ctx;
};

using failure = std::variant<read_failure, invalid_value, unsupported_type, internal_type_mismatch, unrecognized_key, bad_syntax>;

inline failure_context context(const failure& f) {
    return std::visit([](auto x) { return x.ctx; }, f);
}

inline failure_context& context(failure& f) {
    return std::visit([](auto& x) -> failure_context& { return x.ctx; }, f);
}

// some names for failure classes

constexpr auto name(read_failure) { return "read failure"; }
constexpr auto name(invalid_value) { return "invalid value"; }
constexpr auto name(unsupported_type) { return "unsupported type"; }
constexpr auto name(internal_type_mismatch) { return "internal error"; }
constexpr auto name(unrecognized_key) { return "unrecognized key"; }
constexpr auto name(bad_syntax) { return "bad syntax"; }
constexpr auto name(failure f) { return std::visit([](const auto& x) { return name(x); }, f); }

// hopefully<T> is the expected alias used for representing results
// from parapara routines that can encode the above failure conditions.
//
// std::expected is the underlying class template until a C++17 or C++20
// work-alike is complete.

template <typename T>
using hopefully = std::expected<T, failure>;

template <typename T>
struct is_hopefully: std::false_type {};

template <typename T>
struct is_hopefully<hopefully<T>>: std::true_type {};

template <typename T>
inline constexpr bool is_hopefully_v = is_hopefully<T>::value;

// Base exception class; other parapara exceptions derive from this.

struct parapara_error: public std::runtime_error {
    parapara_error(const std::string& message): std::runtime_error(message) {}
};

// Thrown when creating parameter set with a duplicate or empty key after canonicalization

struct bad_key_set: parapara_error {
    explicit bad_key_set(std::string key = {}):
        parapara_error("bad parameter key set"), key(std::move(key)) {}

    std::string key; // offending key, if any, that caused a key collision.
};

#if 0
// Do we actually need these?

// Thrown by a parameter importer when encountering misformed input.

struct syntax_error: parapara_error {
    explicit syntax_error(failure_context ctx = {}):
       parapara_error("misformed text in parameter data"), ctx(std::move(ctx)) {}

    failure_context ctx;
};

// Thrown by a parameter importer when encountering an unknown key.

struct syntax_error: parapara_error {
    explicit syntax_error(failure_context ctx = {}):
       parapara_error("unrecognized key"), ctx(std::move(ctx)) {}

    failure_context ctx;
};

// Thrown by a parameter importer if there is a failure in reading or validating a parameter value.

struct read_error: parapara_error {
    explicit read_error(failure_context ctx = {}):
       parapara_error("error parsing parameter value"), ctx(std::move(ctx)) {}

    failure_context ctx;
};

struct value_error: parapara_error {
    explicit value_error(failure_context ctx = {}):
       parapara_error("invalid parameter value"), ctx(std::move(ctx)) {}

    explicit value_error(std::string constraint, failure_context ctx = {}):
       parapara_error("invalid parameter value"), constraint(constraint), ctx(std::move(ctx)) {}

    std::string constraint;
    failure_context ctx;
};
#endif

// II. Reader/writer utilities
// ===========================

// If C++20 is targetted instead of C++17, these ad hoc type traits and std::enable_if guards
// can be replaced with concepts and requires clauses.

template <typename X, typename = void>
struct can_charconv: std::false_type {};

template <typename X>
struct can_charconv<X, std::void_t<decltype(std::from_chars((char *)0, (char *)0, std::declval<X&>()))>>: std::true_type {};

template <typename X>
inline constexpr bool can_charconv_v = can_charconv<X>::value;

// - read a scalar value that is supported by std::from_chars, viz. a standard integer or floating point type.

template <typename T, std::enable_if_t<can_charconv_v<T>, int> = 0>
hopefully<T> read_cc(std::string_view v) {
    T x;
    auto [p, err] = std::from_chars(&v.front(), &v.front()+v.size(), x);

    if (err==std::errc::result_out_of_range) return std::unexpected(invalid_value{});
    if (err!=std::errc() || p!=&v.front()+v.size()) return std::unexpected(read_failure{});
    return x;
}

// - read a string, without any filters

inline hopefully<std::string> read_string(std::string_view v) {
    return std::string(v);
}

// - read a boolean true/false representation, case sensitive

inline hopefully<bool> read_bool_alpha(std::string_view v) {
    if (v=="true") return true;
    else if (v=="false") return false;
    else return std::unexpected{read_failure{}};
}

// - read a delimitter-separated list of items, without delimiter escapes;
//   C represents a container type, constructible from an iterator range.

template <typename X, typename = void>
struct has_value_type: std::false_type {};

template <typename X>
struct has_value_type<X, std::void_t<typename X::value_type>>: std::true_type {};

template <typename X>
inline constexpr bool has_value_type_v = has_value_type<X>::value;

template <typename C,
          std::enable_if_t<has_value_type_v<C>, int> = 0,
          std::enable_if_t<std::is_constructible_v<C, const typename C::value_type*, const typename C::value_type*>, int> = 0>
struct read_dsv {
    using value_type = typename C::value_type;
    std::function<hopefully<value_type> (std::string_view)> read_field;
    std::string delim;      // field separator
    const bool skip_ws;     // if true, skip leading space/tabs in each field

    explicit read_dsv(std::function<hopefully<value_type> (std::string_view)> read_field, std::string delim=",", bool skip_ws = true):
        read_field(std::move(read_field)), delim(delim), skip_ws(skip_ws) {}

    hopefully<C> operator()(std::string_view v) const {
        constexpr auto npos = std::string_view::npos;
        std::vector<value_type> fields;
        while (!v.empty()) {
            if (skip_ws) {
                auto ns = v.find_first_not_of(" \t");
                if (ns!=npos) v.remove_prefix(ns);
            }
            auto d = v.find(delim);
            if (auto hf = read_field(v.substr(0, d))) {
                fields.push_back(std::move(hf.value()));
                if (d!=npos) v.remove_prefix(d+1);
                else break;
            }
            else {
                return std::unexpected(std::move(hf.error()));
            }
        }

        return v.empty()? C{}: C{&fields.front(), &fields.front()+fields.size()};
    }
};

// III. Reader (and later, writer) classes
// =======================================
//
// A reader object maintains a type-indexed collection of type-specific parsers;
// the constructor and add method allows readers to be composed and extended with
// low syntactic overhead.
//
// Readers are used by parameter specifications (see below) to support consistent
// parameter value representations across a parameter set.

namespace detail {

// workaround for stdlibc++ issue 115939
template <typename T>
struct box {
    template <typename... Ts>
    explicit box(Ts&&... ts): value(std::forward<Ts>(ts)...) {}

    T value;
};

template <typename T>
box(T) -> box<T>;

template <typename T>
inline const T& unbox(const box<T>& box) { return box.value; }

template <typename T>
inline T unbox(box<T>&& box) { return std::move(box.value); }

} // namespace detail

template <typename Repn = std::string_view>
struct reader {
    using representation_type = Repn;
    std::unordered_map<std::type_index, std::function<hopefully<detail::box<std::any>> (Repn)>> rmap;

    // typed read from represenation

    template <typename T>
    hopefully<T> read(Repn v) const {
        if (auto i = rmap.find(std::type_index(typeid(T))); i!=rmap.end()) {
            return (i->second)(v).
                transform([](detail::box<std::any> a) { return std::any_cast<T>(unbox(a)); });
        }
        else {
            return std::unexpected(unsupported_type{});
        }
    }

    // type-erased read from representation

    hopefully<std::any> read(std::type_index ti, Repn v) const {
        if (auto i = rmap.find(ti); i!=rmap.end()) {
            return (i->second)(v).transform([](auto a) { return unbox(a); });
        }
        else {
            return std::unexpected(unsupported_type{});
        }
    }

    void add() {}

    // add a read function

    template <typename F,
              typename... Tail,
              std::enable_if_t<std::is_invocable_v<F, Repn>, int> = 0,
              std::enable_if_t<is_hopefully_v<std::invoke_result_t<F, Repn>>, int> = 0>
    void add(F read, Tail&&... tail) {
        using T = typename std::invoke_result_t<F, Repn>::value_type;
        rmap[std::type_index(typeid(T))] = [read = std::move(read)](Repn v) -> hopefully<detail::box<std::any>> {
            return read(v).transform([](auto x) -> detail::box<std::any> { return detail::box(std::any(x)); });
        };
        add(std::forward<Tail>(tail)...);
    }

    // add read functions from another reader

    template <typename... Tail>
    void add(const reader& other, Tail&&... tail) {
        for (const auto& item: other.rmap) {
            rmap.insert_or_assign(item.first, item.second);
        }
        add(std::forward<Tail>(tail)...);
    }

    reader() = default;
    reader(const reader&) = default;
    reader(reader&&) = default;

    // construct a reader from read functions and/or other readers

    template <typename... Fs>
    explicit reader(Fs&&... fs) {
        add(std::forward<Fs>(fs)...);
    }
};

namespace detail {

// default reader uses charconv for numeric scalars, simple string reader,
// true/false boolean read, and comma-delimmited reader for numeric vectors.

reader<std::string_view> make_default_reader() {
    return reader<std::string_view>(
        read_cc<short>,
        read_cc<unsigned short>,
        read_cc<int>,
        read_cc<unsigned int>,
        read_cc<long>,
        read_cc<unsigned long>,
        read_cc<long long>,
        read_cc<unsigned long long>,
        read_cc<float>,
        read_cc<double>,
        read_cc<long double>,
        read_bool_alpha,
        read_string,
        read_dsv<std::vector<short>>{read_cc<short>},
        read_dsv<std::vector<unsigned short>>{read_cc<unsigned short>},
        read_dsv<std::vector<int>>{read_cc<int>},
        read_dsv<std::vector<unsigned int>>{read_cc<unsigned int>},
        read_dsv<std::vector<long>>{read_cc<long>},
        read_dsv<std::vector<unsigned long>>{read_cc<unsigned long>},
        read_dsv<std::vector<long long>>{read_cc<long long>},
        read_dsv<std::vector<unsigned long long>>{read_cc<unsigned long long>},
        read_dsv<std::vector<float>>{read_cc<float>},
        read_dsv<std::vector<double>>{read_cc<double>},
        read_dsv<std::vector<long double>>{read_cc<long double>}
    );
}

} // namespace detail


inline const reader<std::string_view>& default_reader() {
    static reader<std::string_view> s = detail::make_default_reader();
    return s;
}

// IV: Parameter specifications, specification sets
// ================================================

// Note that validators given to a specification do not need to be of type
// parapara::validator<X> for some X.

template <typename V, typename U, typename = void>
struct validates_parameter: std::false_type {};

template <typename V, typename U>
struct validates_parameter<V, U, std::void_t<std::invoke_result_t<V, U>>>:
    std::is_convertible<std::invoke_result_t<V, U>, hopefully<U>> {};

template <typename V, typename U>
inline constexpr bool validates_parameter_v = validates_parameter<V, U>::value;


// TODO Add special case for dealing with fields of type optional<X>.

template <typename Record>
struct specification {
    using record_type = Record;
    std::string key;
    std::string description;
    std::type_index field_type;

    template <typename Field, typename V, std::enable_if_t<validates_parameter_v<V, Field>, int> = 0>
    specification(std::string key, Field Record::* field_ptr, V validate, std::string description = ""):
        key(std::move(key)),
        description(description),
        field_type(std::type_index(typeid(Field))),
        assign_impl_(
            [&key, field_ptr = field_ptr, validate = std::move(validate)] (Record& record, std::any value) -> hopefully<void>
            {
                const Field* fptr = std::any_cast<Field>(&value);
                if (!fptr) return std::unexpected(internal_type_mismatch{});

                return static_cast<hopefully<Field>>(validate(*fptr)).
                        transform([&record, field_ptr](Field x) { record.*field_ptr = std::move(x); });
            })
    {}

    template <typename Field>
    specification(std::string key, Field Record::* field_ptr, std::string description = ""):
        specification(key, field_ptr, [](auto&& x) { return decltype(x)(x); }, description)
    {}

    template <typename Reader = reader<std::string_view>>
    hopefully<void> read(Record& record, typename Reader::representation_type repn, const Reader rdr = default_reader()) const {
        return rdr.read(field_type, repn).and_then([this, &record](auto a) { return assign(record, std::move(a)); });
    }

    hopefully<void> assign(Record& record, std::any value) const {
        return assign_impl_(record, value).transform_error([this](failure f) { context(f).key = key; return f; });
    }

private:
    std::function<hopefully<void> (Record &, std::any)> assign_impl_;
};

// key canonicalization

std::string keys_lc(std::string_view v) {
    std::string out;
    for (char c: v) out.push_back(std::tolower(c));
    return out;
}

std::string keys_lc_nows(std::string_view v) {
    std::string out;
    for (char c: v) {
        if (isspace(c)) continue;
        else out.push_back(std::tolower(c));
    }
    return out;
}

template <typename X>
struct value_type { using type = typename X::value_type; };

template <typename X, std::size_t N>
struct value_type<X [N]> { using type = X; };

template <typename X>
using value_type_t = typename value_type<std::remove_reference_t<X>>::type;

// Specification sets comprise a collection of specifications over the same record type with unique keys
// which optionally are first transformed into a canonical form by a supplied canonicalizer.
//
// They are constructed from an existing container or range of specifications together win an optional
// canonicalizer.

template <typename Record>
struct specification_set {
    // Specification sets can be constructed from a sequence of specification objects and collections of specification objects.
    // If a non-trivial key transformation (canonicalizer) is provided, it must be the first argument to the constructor.

    specification_set() = default;

    template <typename C, std::enable_if_t<std::is_assignable_v<specification<Record>&, value_type_t<C>>, int> = 0>
    specification_set(const C& specs, std::function<std::string (std::string_view)> cify = {}):
        canonicalize_(cify)
    {
        for (specification<Record> s: specs) insert(std::move(s));
    }

    void insert(specification<Record> s) {
        std::string key = s.key;
        if (auto [_, inserted] = set_.emplace(canonicalize(key), std::move(s)); !inserted) throw bad_key_set(key);
    }

    std::string canonicalize(std::string_view v) const {
        return canonicalize_? canonicalize_(v): std::string(v);
    }

    bool contains(std::string_view key) const { return set_.contains(canonicalize(key)); }

    const specification<Record>& at(std::string_view key) const {
        return set_.at(canonicalize(key));
    }

    const specification<Record>* get_if(std::string_view key) const {
        if (auto i = set_.find(canonicalize(key)); i==set_.end()) return nullptr; else return &(i->second);
    }

private:
    std::unordered_map<std::string, specification<Record>> set_;
    std::function<std::string (std::string_view)> canonicalize_;
};

template <typename X>
specification_set(X&) -> specification_set<typename value_type_t<X>::record_type>;

template <typename X>
specification_set(X&, std::function<std::string (std::string_view)>) -> specification_set<typename value_type_t<X>::record_type>;

// V. Validation helpers
// =====================
//
// The validator passed to the specification constructor can be any functional that takes a field value
// of type U to hopefully<U>. The validator class below provides a convenience interface to producing
// such functionals and also allows Kleisli composition via operator|= defined below.

template <typename Predicate = void> struct validator;

template <> struct validator<void> {};

template <typename Predicate>
struct validator: validator<void> {
    Predicate p;
    std::string constraint;

    validator(Predicate p, std::string constraint): p(std::move(p)), constraint(std::move(constraint)) {}

    template <typename A>
    hopefully<A> operator()(const A& a) const { if (p(a)) return a; else return std::unexpected(invalid_value(constraint, {})); }
};

template <typename X>
inline constexpr bool is_validator_v = std::is_base_of_v<validator<>, X>;

template <typename Predicate>
auto require(Predicate p, std::string constraint = "") {
    return validator(std::move(p), std::move(constraint));
}

template <typename Value>
auto minimum(Value v, std::string constraint = "value at least minimum") {
    return validator([v = std::move(v)](auto x) { return x>=v; }, std::move(constraint));
}

template <typename Value>
auto maximum(Value v, std::string constraint = "value at most maximum") {
    return validator([v = std::move(v)](auto x) { return x<=v; }, std::move(constraint));
}

// Operator (right associative) for Kleisli composition in std::expect. ADL will find this operator when the first
// argument is e.g. derived from parapara::validator<>.

template <typename V1, typename V2>
auto operator&=(V1 v1, V2 v2) {
    return [v1, v2](auto x) { return v1(x).and_then(v2); };
}


// VI. Importers (and later, exporters)
// ====================================
//
//
// import_kv_pairs:
//
//     Split into records delimited by \n and then split into two fields k, v separated by the first occurance of `fs`.
//     Assign corresponding field k of record from value v.
//
// import_ini:
//
//     Scan (a particular variant of) an IrI-style configuration, at most one record per line with lines delimited by \n.
//     Comments (lines starting with '#') are skipped.
//     Lines of the form [ _section_ ], ignoring leading and trailing whitespace, set the current section to _section_.
//     Lines of the form _k_ = _v_, ignoring leading and trailing whitespace and whitespace about the '=' sign will assign
//     the corresponding field _section_/_k_ (or just _k_ if _section_ is empty or unset) from value _v_, where / stands for
//     thw key separator. Whitespace surrounding _k_ or _v_ is ignored.


// Actually just remove all the line parsing crud here; have import_k_eq_v for argv items and use streams
// for import_ini.


#if 0
namespace detail {

struct line_iterator {
    using size_type = std::string_view::size_type;
    using difference_type = std::string_view::difference_type;
    using value_type = std::string_view;
    using reference = std::string_view;
    using iterator_tag = std::input_iterator_tag;

    static constexpr size_type npos = std::string_view::npos;

    std::string_view all;
    size_type b = npos, n = npos;

    line_iterator() = default;

    explicit line_iterator(std::string_view s):
        all(s), b(0), n(all.find('\n')) {}

    bool operator==(line_iterator other) const {
        return (b==npos && other.b==npos) || (all.data()==other.all.data() && b==other.b);
    }

    bool operator!=(line_iterator other) const {
        return !(*this==other);
    }

    line_iterator& operator++() {
        if (b==npos || n==npos || b+n>=all.size()-1) {
            b = n = npos;
        }
        else {
            b += n+1;
            n = all.substr(b).find('\n');
        }
        return *this;
    }

    line_iterator operator++(int) {
        line_iterator keep = *this;
        ++*this;
        return keep;
    }

    std::string_view operator*() const {
        return all.substr(b, n);
    }
};

struct lines {
    line_iterator b, e;

    explicit lines(std::string_view v):
        b(line_iterator(v)), e(line_iterator()) {}

    line_iterator begin() const { return b; }
    line_iterator end() const { return e; }
};

} // namespace detail

template <typename Record>
hopefully<void> import_kv_pairs(
    Record &rec, const specification_set<Record>& specs, const reader<std::string_view>& rdr,
    std::string_view text, std::string fs = "=")
{
    constexpr auto npos = std::string_view::npos;

    failure_context ctx;

    for (auto line: detail::lines(text)) {
        std::string_view key, value;
        ++ctx.nr;

        if (line.empty()) continue;

        auto eq = line.find('=');
        if (eq==npos) {
            key = line;
            value = "true"; // special case boolean
        }
        else {
            key = line.substr(0, eq);
            value = line.substr(eq+1);
        }

        auto sp = specs.get_if(key);
        if (!sp) {
            ctx.key = std::string(key);
            return std::unexpected(unrecognized_key(ctx));
        }

        if (auto h = sp->read(rec, value, rdr)) continue;
        else {
            ctx.key = key;
            ctx.offset = eq==npos? 0: eq+1;
            context(h.error()) = ctx;
            return h;
        }
    }
    return {};
}
#endif

template <typename Record>
hopefully<void> import_k_eq_v(
    Record &rec, const specification_set<Record>& specs, const reader<std::string_view>& rdr,
    std::string_view text, std::string eq_token = "=")
{
    constexpr auto npos = std::string_view::npos;

    failure_context ctx;
    std::string_view key, value;

    if (text.empty()) return {};

    auto eq = text.find(eq_token);
    if (eq==npos) {
        key = text;
        value = "true"; // special case boolean
    }
    else {
        key = text.substr(0, eq);
        value = text.substr(eq+1);
    }

    auto sp = specs.get_if(key);
    if (!sp) {
        ctx.key = std::string(key);
        return std::unexpected(unrecognized_key(ctx));
    }

    auto h = sp->read(rec, value, rdr);
    if (!h) {
        ctx.key = key;
        ctx.cindex = eq==npos? 1: eq+2;
        context(h.error()) = ctx;
    }

    return h;
}


template <typename Record>
hopefully<void> import_k_eq_v(Record &rec, const specification_set<Record>& specs, std::string_view text, std::string eq_token = "=") {
    return import_kv_pairs(rec, specs, default_reader(), text, eq_token);
}

} // namespace parapara
