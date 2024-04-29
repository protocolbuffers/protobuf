# C style guide

<!--*
# Document freshness: For more information, see go/fresh-source.
freshness: { owner: 'haberman' reviewed: '2022-05-08' }
*-->

Since upb is written in pure C, we supplement the
[Google C++ style guide](https://google.github.io/styleguide/cppguide.html) with
some C-specific guidance.

Everything written here is intended to follow the spirit of the C++ style guide.

upb is currently inconsistent about following these conventions. It is intended
that all code will be updated to match these guidelines. The priority is
converting public interfaces as these are more difficult to change later.

## Naming

### Functions and Types

C does not have namespaces. Anywhere you would normally use a namespace
separator (`::`) in C++, we use an underscore (`_`) in C:

```c++
// C equivalent for upb::Arena::New()
upb_Arena* upb_Arena_New();
```

Since we rely on `_` to be our namespace separator, we never use it to merely
separate words in function or type names:

```c++
// BAD: this would be interpreted as upb::FieldDef::has::default().
bool upb_FieldDef_has_default(const upb_FieldDef* f);

// GOOD: this is equivalent to upb::FieldDef::HasDefault().
bool upb_FieldDef_HasDefault(const upb_FieldDef* f);
```

For multi-word namespaces, we use `PascalCase`:

```c++
// `PyUpb` is the namespace.
PyObject* PyUpb_CMessage_GetAttr(PyObject* _self, PyObject* attr);
```

### Private Functions and Members

Since we do not have `private` in C++, we use a leading underscore convention
to mark internal functions and variables that should only be accessed from
upb:

```c++
// Internal-only function.
int64_t _upb_Int64_FromLL();

// Internal-only members. Underscore prefixes are only necessary when the
// structure is defined in a header file.
typedef struct {
  const int32_t* _values;  // List of values <0 or >63
  uint64_t _mask;          // Bits are set for acceptable value 0 <= x < 64
  int _value_count;
} upb_MiniTableEnum;
```
