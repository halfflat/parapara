% PARAPARA::KEYED_VIEW(3) Parapara Manual

# NAME

parapara::keyed_view\<bool const_view\> - mutable and immutable label-keyed views of a record

# SYNOPSIS

```
#include <parapara/parapara.h>
```

## Public Member Functions

For mutable views `parapara::keyed_view<false>`:

template \<typename R> explicit **keyed_view**(R&, parapara::specification_map\<R>)

template \<typename R> keyed_view& **rebind**(R&)

)proxy-type* **operator[]**(std::string_view key)

For immutable views `parapara::keyed_view<true>`:

template \<typename R> explicit **keyed_view**(const R&, parapara::specification_map\<R>)

template \<typename R> **keyed_view**(const keyed_view\<false>&)

template \<typename R> keyed_view& **rebind**(const R&)

*proxy-type* **operator[]**(std::string_view key)

## Aliases and deduction rules

```
namespace parapara {
    using mutable_keyed_view = keyed_view<false>;
    using const_keyed_view = keyed_view<true>;

    template <typename Record>
    keyed_view(Record&, specification_map<Record>) -> keyed_view<false>;

    template <typename Record>
    keyed_view(const Record&, specification_map<Record>) -> keyed_view<true>;
}
```

# DESCRIPTION

Both mutable `parapara::keyed_view<false>` and immutable `parapara::keyed_view<true>`
classes present a type-erased and label-index associative array view of a structure
value whose fields are labelled according to a `parapara::specification_map`.

Given a `parapara::specification_map<R>` *map* for some struct *R*, a `keyed_view` constructed
from an object *r* of type *R* and the map *map* can be indexed by the labels of `parapara::specification`s
in *map* to give proxy objects that refer to the corresponding members of *r* and which can then
be implicitly converted to the type of that field or, for mutable `keyed_view`s, assigned
to, triggering any validation associated with that specification.

A mutable `keyed_view` can be implicitly converted to an immutable `keyed_view` (but not
vice versa) and the deduction rules provided will create a mutable or immutable view
according to the constness of the supplied record object given to the constructor.

`keyed_view` objects maintain a copy of the supplied `specificaion_map` but hold a
non-owning reference to the supplied record object. They can only be used safely during
the lifetime of that object. They can also be rebound to another instance of the record
type, but cannot be rebound to objects of a different type.

## Constructors

**template \<typename R> keyed_value(R& r, parapara::specification_map\<R>)**
 ~  Construct a mutable (`keyed_value<false>`) view of the object *r* indexed by the `parapara::specification` objects within the
supplied `parapara::specification_map<R>`. The view maintains a non-owning reference to *r*.

**template \<typename R> keyed_value(const R& r, parapara::specification_map\<R>)**
 ~  Construct an immutable (`keyed_value<true>`) view of the object *r* indexed by the `parapara::specification` objects within the
supplied `parapara::specification_map<R>`. The view maintains a non-owning reference to *r*.

**template \<typename R> keyed_view\<true>::keyed_value(const keyed_view\<false>&)**
 ~  Construct an immutable `keyed_value<true>` view from a mutable `keyed_value<false>` view.

## Value access

** *proxy-type* keyed_value::operator\[](std::string_view key)**
 ~  If the `key` is a valid label in the specification map, return a proxy object representing the corresponding member
of the viewed object which can be implicitly converted to that member's type, or for mutable views, be assigned to from
a value of that member's type. Implicit conversions to or assignments from other types will raise a `std::bad_any_cast`.
exception. Members of type `std::optional<X>` are treated differently: the proxy will implicitly convert to a value of
type `X` if the member contains a value, and can be assigned to from values of type `X`. Assigned values are processed
by any validation associated with the specification in the map; a validation failure will throw a `parapara::validation_failed`
exception. A `std::out_of_range` exception will be thrown if `key` is not a valid label.

## Mutating operations

**template \<typename R> keyed_view\<false>::rebind(R& s)**
 ~  Change the referenced object to `s`. Will throw a `std::bad_any_cast` exception if `s` is not of the same type as the
object used in the construction of the `keyed_view`.

**template \<typename R> keyed_view\<true>::rebind(const R& s)**
 ~  Change the referenced object to `s`. Will throw a `std::bad_any_cast` exception if `s` is not of the same type as the
object used in the construction of the `keyed_view`.

# EXAMPLES

Given the following definitions of a record struct and a specification map:
```
    struct record {
        int a;
        std::string s;
    };

    parapara::specification<record> specs[] = {
        {"a", &record::a, parapara::at_least(0, "a ≥ 0")},
        {"s", &record::s}
    };

    parapara::specification_map<record> S(specs);
```

One can produce a mutating view of a record object *r* with:
```
    record r = { 3, "fish" };
    parapara::keyed_view view(r, S);
```

The members of *r* can then be access by the labels "a" and "s" through the view, with the caveat that types
must match exactly:
```
    using namespace std::string_literals;

    view["a"] = 4;             // sets r.a to 4.
    std::string s = view["s"]; // sets s to r.s.
    view["s"] = "cat";         // throws an exception: r.s is not of type const char*.
    view["s"] = "cat"s;        // sets r.s to std::string("cat").
```

Assignments will trigger any validation associated with a specification:
```
    view["a"] = -1;            // throws parapara::validation_failure
```

# SEE ALSO

**parapara::specification(3type)**,
**parapara::specification_map(3type)**,
**parapara(7)**


