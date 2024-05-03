
# Refcounting Tips

One of the trickiest parts of the C extension for PHP is getting the refcounting
right.  These are some notes about the basics of what you should know,
especially if you're not super familiar with PHP's C API.

These notes cover the same general material as [the Memory Management chapter of
the PHP internal's
book](https://www.phpinternalsbook.com/php7/zvals/memory_management.html), but
calls out some points that were not immediately clear to me.

##  Zvals

In the PHP C API, the `zval` type is roughly analogous to a variable in PHP, eg:

```php
    // Think of $a as a "zval".
    $a = [];
```

The equivalent PHP C code would be:

```c
    zval a;
    ZVAL_NEW_ARR(&a);  // Allocates and assigns a new array.
```

PHP is reference counted, so each variable -- and thus each zval -- will have a
reference on whatever it points to (unless its holding a data type that isn't
refcounted at all, like numbers). Since the zval owns a reference, it must be
explicitly destroyed in order to release this reference.

```c
    zval a;
    ZVAL_NEW_ARR(&a);

    // The destructor for a zval, this must be called or the ref will be leaked.
    zval_ptr_dtor(&a);
```

Whenever you see a `zval`, you can assume it owns a ref (or is storing a
non-refcounted type). If you see a `zval*`, which is also quite common, then
this is *pointing to* something that owns a ref, but it does not own a ref
itself.

The [`ZVAL_*` family of
macros](https://github.com/php/php-src/blob/4030a00e8b6453aff929362bf9b25c193f72c94a/Zend/zend_types.h#L883-L1109)
initializes a `zval` from a specific value type.  A few examples:

* `ZVAL_NULL(&zv)`: initializes the value to `null`
* `ZVAL_LONG(&zv, 5)`: initializes a `zend_long` (integer) value
* `ZVAL_ARR(&zv, arr)`: initializes a `zend_array*` value (refcounted)
* `ZVAL_OBJ(&zv, obj)`: initializes a `zend_object*` value (refcounted)

Note that all of our custom objects (messages, repeated fields, descriptors,
etc) are `zend_object*`.

The variants that initialize from a refcounted type do *not* increase the
refcount. This makes them suitable for initializing from a newly-created object:

```c
    zval zv;
    ZVAL_OBJ(&zv, CreateObject());
```

Once in a while, we want to initialize a `zval` while also increasing the
reference count. For this we can use `ZVAL_OBJ_COPY()`:

```c
zend_object *some_global;

void GetGlobal(zval *zv) {
    // We want to create a new ref to an existing object.
    ZVAL_OBJ_COPY(zv, some_global);
}
```

## Transferring references

A `zval`'s ref must be released at some point. While `zval_ptr_dtor()` is the
simplest way of releasing a ref, it is not the most common (at least in our code
base). More often, we are returning the `zval` back to PHP from C.

```c
    zval zv;
    InitializeOurZval(&zv);
    // Returns the value of zv to the caller and donates our ref.
    RETURN_COPY_VALUE(&zv);
```

The `RETURN_COPY_VALUE()` macro (standard in PHP 8.x, and polyfilled in earlier
versions) is the most common way we return a value back to PHP, because it
donates our `zval`'s refcount to the caller, and thus saves us from needing to
destroy our `zval` explicitly. This is ideal when we have a full `zval` to
return.

Once in a while we have a `zval*` to return instead. For example when we parse
parameters to our function and ask for a `zval`, PHP will give us pointers to
the existing `zval` structures instead of creating new ones.

```c
    zval *val;
    if (zend_parse_parameters(ZEND_NUM_ARGS(), "z", &val) == FAILURE) {
      return;
    }
    // Returns a copy of this zval, adding a ref in the process.
    RETURN_COPY(val);
```

When we use `RETURN_COPY`, the refcount is increased; this is perfect for
returning a `zval*` when we do not own a ref on it.
