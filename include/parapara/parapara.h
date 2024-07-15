#pragma once
#include <any>
#include <expected>
#include <functional>
#include <iterator>
#include <ranges>
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
    unsigned nr = 0;     // record number/line number.
    unsigned offset = 0; // offset into record or line.
};

struct read_failure {
    failure_context ctx;
};

struct invalid_value {
    std::string constraint;  // optionally supplied by a parameter constraint
    failure_context ctx;
};

struct unsupported_type {
    failure_context ctx;
};

struct internal_type_mismatch {
    failure_context ctx;
};

using failure = std::variant<read_failure, invalid_value, unsupported_type, internal_type_mismatch>;

inline failure_context context(const failure& f) {
    return std::visit([](auto x) { return x.ctx; }, f);
}

inline failure_context& context(failure& f) {
    return std::visit([](auto& x) -> failure_context& { return x.ctx; }, f);
}

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

// Thrown by a parameter importer when encountering misformed input.

struct syntax_error: parapara_error {
    explicit syntax_error(failure_context ctx = {}):
       parapara_error("misformed text in parameter data"), ctx(std::move(ctx)) {}

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

// Section IV: Parameter specifications, parameter sets
// ====================================================

// TODO replace concepts, requires with enable_if clauses

template <typename V, typename U>
concept ParameterValidatorFor = requires (V v, U u) {
    {v(std::declval<U>())} -> std::convertible_to<hopefully<U>>;
};


// TODO Add special case for dealing with fields of type optional<X>.

template <typename Record>
struct specification {
    std::string key;
    std::string description;
    const std::type_index field_type;

    template <typename Field, typename V>
    requires ParameterValidatorFor<V, Field>
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
    hopefully<void> read(Record& record, typename Reader::representation_type repn, const Reader rdr = default_reader()) {
        return rdr.read(field_type, repn).and_then([this, &record](auto a) { return assign(record, std::move(a)); });
    }

    hopefully<void> assign(Record& record, std::any value) {
        return assign_impl_(record, value).transform_error([this](failure f) { context(f).key = key; return f; });
    }

private:
    std::function<hopefully<void> (Record &, std::any)> assign_impl_;
};

// key canonicalization

std::string keys_lc(std::string_view v) {
    using std::views::transform;

    std::string out;
    std::ranges::copy(
        v |
        transform([](unsigned char c) { return std::tolower(c); }),
        std::back_inserter(out));
    return out;
}

std::string keys_lc_nows(std::string_view v) {
    using std::views::transform;
    using std::views::filter;

    std::string out;
    std::ranges::copy(v |
        filter([](unsigned char c) { return !std::isspace(c); }) |
        transform([](unsigned char c) { return std::tolower(c); }),
        std::back_inserter(out));
    return out;
}

template <typename Record>
struct specification_set {
    std::unordered_map<std::string, specification<Record>> set;

    template <typename Collection> // imagine I've put a requires clause on this
    explicit specification_set(const Collection& c, std::function<std::string (std::string_view)> cify = {}):
        canonicalize_(cify)
    {
        for (const specification<Record>& spec: c) {
            std::string k = canonicalize(c.key);
            auto [_, inserted] = set_.emplace(k, spec);

            if (!inserted) throw bad_key_set(c.key);
        }
    }

    std::string canonicalize(std::string_view v) const { return canonicalize_? canonicalize_(v): std::string(v); }

    bool contains(std::string_view key) const { return set_.contains(canonicalize(key)); }
    const specification<Record>& at(std::string_view key) const { return set_.at(canonicalize(key)); }
    const specification<Record>* get_if(std::string_view key) const {
        auto i = set_.find(canonicalize(key));
        if (i==set_.end()) return nullptr;
        else return &(*i);
    }

private:
    std::unordered_map<std::string, specification<Record>> set_;
    std::function<std::string (std::string_view)> canonicalize_;
};

// Validator chaining, default validators
// (viz. operator>>= overloads, wrappers (parapara::assert) for value predicates, interval checks.

template <typename Predicate>
auto assert(Predicate p, std::string constraint = "") {
    return [p, constraint](auto value) -> hopefully<decltype(value)> { if (p(value)) return value; else return std::unexpected(invalid_value{constraint, {}}); };
}

template <typename Value>
auto minimum(Value v, std::string constraint = "value at least minimum") {
    return [v, constraint](auto value) -> hopefully<decltype(value)> { if (value>=v) return value; else return std::unexpected(invalid_value{constraint, {}}); };
}

template <typename Value>
auto maximum(Value v, std::string constraint = "value at most maximum") {
    return [v, constraint](auto value) -> hopefully<decltype(value)> { if (value<=v) return value; else return std::unexpected(invalid_value{constraint, {}}); };
}

// Put some guards on this!
template <typename V1, typename V2>
auto operator>>=(V1 v1, V2 v2) {
    return [v1, v2](auto hv) { return hv.and_then(v1).and_then(v2); };
}


} // namespace parapara
