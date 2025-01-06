% PARAPARA::FAILURE(3) Parapara Manual

# NAME

parapara::failure, parapara::hopefully - return value types for parapara failure handling

# SYNOPSIS

```
#include <parapara/parapara.h>

namespace parapara {
using std::expected;
using std::unexpected;

struct failure {
    enum error_enum {
        internal_error = 0,
        read_failure,
        invalid_value,
        unsupported_type,
        unrecognized_key,
        bad_syntax,
        empty_optional
    } error;

    source_context ctx;
    std::any data;
};

template <typename T>
using hopefully = expected<T, parapara::failure>
}
```

# DESCRIPTION

`parapara::failure` represents an error return from various parapara functions and methods. Its fields are:

error
  ~ An enum value describing the type of failure represented (see below).

ctx
  ~ Information regarding the context of the error in an associated string or file.

data
  ~ Type-erased information pertinent to the type of failure given by the `error` field.


Failure error_enum values and their corresponding `data` member values (if not empty):

internal_error
  ~ A failure of logic internal to parapara. This is always indicates a bug in parapara implementation.

read_failure
  ~ Indicates that a read function, such as one supplied in a `parapara::reader`, has failed to properly parse
    a representation.

invalid_value
  ~ Returned when a validation test fails. Validation functions made with `parapara::validator` will always
    return these sorts of failure, with the `data` field holding a string value describing the constraint
    which was not satisfied.

unsupported_type
  ~ Returned when a supplied reader does not contain a read function for the specification field type.

unrecognized_key
  ~ A key was not bound to a specification in a `parapara::specification_map`.

bad_syntax
  ~ Returned by importers when there is a syntax error in the imported representation.

empty_optional
  ~ Returned when a specification `retrieve` or `write` method is invoked on a record where the corresponding
    field value is an empty `std::optional`.


`parapara::hopefully<T>` is an alias for `parapara::expected<T, parapara::failure>` and is a commonly used return type
for parapara functions and methods.

`parapara::expected<T, E>` is an alias for `std::expected<T, E>` if available; it otherwise aliases a backported
workalike `backport::expected`.


# SEE ALSO

**parapara::specification(3)**,
**parapara::specification_map(3)**,
**parapara::reader(3type)**
**parapara::source_context(3type)**,
**parapara::validator(3type)**,
**parapara(7)**

