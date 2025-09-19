% PARAPARA::PARAPARA_ERROR(3) Parapara Manual

# NAME

parapara::parapara_error - base class for exceptions thrown by parapa ra

# SYNOPSIS

```
#include <parapara/parapara.h>

namespace parapara {
struct parapara_error: public std::runtime_error {
    parapara_error(const std::string& message);
};

struct bad_key_set: parapara_error {
    explicit bad_key_set(std::string key = {});
};

struct validation_failed: parapara_error {
    explicit validation_failed(const std::string& message);
};
```

# DESCRIPTION

Exceptions thrown by parapara are either standard library exceptions or are of type `parapara_error` or a class
derived from `parapara_error`.

Most but not all parapara interfaces report failure through return values of type `parapara::hopefully<T>`, a type alias
for `parapara::expected<T, parapara::failure>`. The following components will throw exceptions;

* `parapara::specification_map` will throw `parapara::bad_key_set` on construction if there are duplicate keys.
* `parapara::specification_map::at()` will throw `std::out_of_range` if the requested key is not present.
* Run-time type checks performed in member access, assignment or rebinding in `parapara::keyed_view` will throw
`std::bad_cast`.
* Assignment to a object member through `parapara::keyed_view` which fails a validation test will throw
`parapara::validation_failed`.

# SEE ALSO

**parapara::hopefully(3type)**,
**parapara::failure(3type)**
**parapara(7)**

