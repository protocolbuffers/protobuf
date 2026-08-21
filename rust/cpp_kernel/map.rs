use super::*;

unsafe extern "C" {
    fn proto2_rust_thunk_UntypedMapIterator_increment(iter: &mut UntypedMapIterator);

    pub fn proto2_rust_map_new(key_prototype: FfiMapValue, value_prototype: FfiMapValue) -> RawMap;
    pub fn proto2_rust_map_free(m: RawMap);
    pub fn proto2_rust_map_clear(m: RawMap);
    pub fn proto2_rust_map_size(m: RawMap) -> usize;
    pub fn proto2_rust_map_iter(m: RawMap) -> UntypedMapIterator;
}

/// A trait implemented by types which are allowed as keys in maps.
/// This is all types for fields except for repeated, maps, bytes, messages, enums and floating point types.
/// This trait is defined separately in cpp.rs and upb.rs to be able to set better subtrait bounds.
#[doc(hidden)]
pub trait MapKey: Proxied + FfiMapKey + CppMapTypeConversions + SealedInternal {}

#[derive(Debug)]
#[doc(hidden)]
pub struct InnerMap {
    pub(crate) raw: RawMap,
}

impl InnerMap {
    pub fn new(raw: RawMap) -> Self {
        Self { raw }
    }

    pub fn as_mut(&mut self) -> InnerMapMut<'_> {
        InnerMapMut { raw: self.raw, _phantom: PhantomData }
    }
}

#[derive(Clone, Copy, Debug)]
#[doc(hidden)]
pub struct InnerMapMut<'msg> {
    pub(crate) raw: RawMap,
    _phantom: PhantomData<&'msg ()>,
}

#[doc(hidden)]
impl<'msg> InnerMapMut<'msg> {
    pub fn new(raw: RawMap) -> Self {
        InnerMapMut { raw, _phantom: PhantomData }
    }

    pub fn as_raw(&self) -> RawMap {
        self.raw
    }
}

/// An untyped iterator in a map, produced via `.cbegin()` on a typed map.
///
/// This struct is ABI-compatible with `proto2::internal::UntypedMapIterator`.
/// It is trivially constructible and destructible.
#[repr(C)]
#[doc(hidden)]
pub struct UntypedMapIterator {
    node: *mut c_void,
    map: *const c_void,
    bucket_index: u32,
}

impl UntypedMapIterator {
    /// Returns `true` if this iterator is at the end of the map.
    fn at_end(&self) -> bool {
        // This behavior is verified via test `IteratorNodeFieldIsNullPtrAtEnd`.
        self.node.is_null()
    }

    /// Assumes that the map iterator is for the input types, gets the current
    /// entry, and moves the iterator forward to the next entry.
    ///
    /// Conversion to and from FFI types is provided by the user.
    /// This is a helper function for implementing
    /// `MapValue::iter_next`.
    ///
    /// # Safety
    /// - The backing map must be valid and not be mutated for `'a`.
    /// - The thunk must be safe to call if the iterator is not at the end of
    ///   the map.
    /// - The thunk must always write to the `key` and `value` fields, but not
    ///   read from them.
    /// - The get thunk must not move the iterator forward or backward.
    #[inline(always)]
    pub unsafe fn next_unchecked<'a, K, V, FfiKey, FfiValue>(
        &mut self,

        iter_get_thunk: unsafe fn(
            iter: &mut UntypedMapIterator,
            key: *mut FfiKey,
            value: *mut FfiValue,
        ),
        from_ffi_key: impl FnOnce(FfiKey) -> View<'a, K>,
        from_ffi_value: impl FnOnce(FfiValue) -> View<'a, V>,
    ) -> Option<(View<'a, K>, View<'a, V>)>
    where
        K: MapKey + 'a,
        V: MapValue + 'a,
    {
        if self.at_end() {
            return None;
        }
        let mut ffi_key = MaybeUninit::uninit();
        let mut ffi_value = MaybeUninit::uninit();
        // SAFETY:
        // - The backing map outlives `'a`.
        // - The iterator is not at the end (node is non-null).
        // - `ffi_key` and `ffi_value` are not read (as uninit) as promised by the
        //   caller.
        unsafe { (iter_get_thunk)(self, ffi_key.as_mut_ptr(), ffi_value.as_mut_ptr()) }

        // SAFETY:
        // - The backing map is alive as promised by the caller.
        // - `self.at_end()` is false and the `get` does not change that.
        // - `UntypedMapIterator` has the same ABI as
        //   `proto2::internal::UntypedMapIterator`. It is statically checked to be:
        //   - Trivially copyable.
        //   - Trivially destructible.
        //   - Standard layout.
        //   - The size and alignment of the Rust type above.
        //   - With the `node_` field first.
        unsafe { proto2_rust_thunk_UntypedMapIterator_increment(self) }

        // SAFETY:
        // - The `get` function always writes valid values to `ffi_key` and `ffi_value`
        //   as promised by the caller.
        unsafe {
            Some((from_ffi_key(ffi_key.assume_init()), from_ffi_value(ffi_value.assume_init())))
        }
    }
}

// LINT.IfChange(map_ffi)
#[doc(hidden)]
#[repr(u8)]
#[derive(Debug, PartialEq)]
// Copy of UntypedMapBase::TypeKind
pub enum FfiMapValueTag {
    Bool,
    U32,
    U64,
    F32,
    F64,
    String,
    Message,
}
// For the purposes of FFI, we treat all integral types of a given size the same
// way. For example, u32 and i32 values are all represented as a u32.
// Likewise, u64 and i64 values are all stored in a u64.
#[doc(hidden)]
#[repr(C)]
pub union FfiMapValueUnion {
    pub b: bool,
    pub u: u32,
    pub uu: u64,
    pub f: f32,
    pub ff: f64,
    // Generally speaking, if s is set then it should not be None. However, we
    // do set it to None in the special case where the FfiMapValue is just a
    // "prototype" (see below). In that scenario, we just want to indicate the
    // value type without having to allocate a real C++ std::string.
    pub s: Option<CppStdString>,
    pub m: RawMessage,
}

// We use this tagged union to represent map values for the purposes of FFI.
#[doc(hidden)]
#[repr(C)]
pub struct FfiMapValue {
    pub tag: FfiMapValueTag,
    pub val: FfiMapValueUnion,
}
// LINT.ThenChange(//depot/google3/third_party/protobuf/rust/cpp_kernel/map.cc:
// map_ffi)

impl FfiMapValue {
    fn make_bool(b: bool) -> Self {
        FfiMapValue { tag: FfiMapValueTag::Bool, val: FfiMapValueUnion { b } }
    }

    pub fn make_u32(u: u32) -> Self {
        FfiMapValue { tag: FfiMapValueTag::U32, val: FfiMapValueUnion { u } }
    }

    fn make_u64(uu: u64) -> Self {
        FfiMapValue { tag: FfiMapValueTag::U64, val: FfiMapValueUnion { uu } }
    }

    pub fn make_f32(f: f32) -> Self {
        FfiMapValue { tag: FfiMapValueTag::F32, val: FfiMapValueUnion { f } }
    }

    fn make_f64(ff: f64) -> Self {
        FfiMapValue { tag: FfiMapValueTag::F64, val: FfiMapValueUnion { ff } }
    }

    fn make_string(s: CppStdString) -> Self {
        FfiMapValue { tag: FfiMapValueTag::String, val: FfiMapValueUnion { s: Some(s) } }
    }

    pub fn make_message(m: RawMessage) -> Self {
        FfiMapValue { tag: FfiMapValueTag::Message, val: FfiMapValueUnion { m } }
    }
}

pub trait CppMapTypeConversions: Proxied {
    // We have a notion of a map value "prototype", which is a FfiMapValue that
    // contains just enough information to indicate the value type of the map.
    // We need this on the C++ side to be able to determine size and offset
    // information about the map entry. For messages, the prototype is
    // the message default instance. For all other types, it is just a FfiMapValue
    // with the appropriate tag.
    fn get_prototype() -> FfiMapValue;

    fn to_map_value(self) -> FfiMapValue;

    /// # Safety
    /// - `value` must store the correct type for `Self`. If it is a string or
    ///   bytes, then it must not be None. If `Self` is a closed enum, then
    ///   `value` must store a valid value for that enum. If `Self` is a
    ///   message, then `value` must store a message of the same type.
    /// - The value must be valid for `'a` lifetime.
    unsafe fn from_map_value<'a>(value: FfiMapValue) -> View<'a, Self>;

    /// # Safety
    /// - `value` must store a message of the same type as `Self`.
    /// - `value` must be valid and have exclusive mutable access for `'a` lifetime.
    #[allow(unused_variables)]
    unsafe fn mut_from_map_value<'a>(value: FfiMapValue) -> Mut<'a, Self>
    where
        Self: Message,
    {
        panic!("mut_from_map_value is only implemented for messages")
    }
}

impl CppMapTypeConversions for u32 {
    fn get_prototype() -> FfiMapValue {
        FfiMapValue::make_u32(0)
    }
    fn to_map_value(self) -> FfiMapValue {
        FfiMapValue::make_u32(self)
    }
    unsafe fn from_map_value<'a>(value: FfiMapValue) -> View<'a, Self> {
        debug_assert_eq!(value.tag, FfiMapValueTag::U32);
        unsafe { value.val.u }
    }
}

impl CppMapTypeConversions for i32 {
    fn get_prototype() -> FfiMapValue {
        FfiMapValue::make_u32(0)
    }
    fn to_map_value(self) -> FfiMapValue {
        FfiMapValue::make_u32(self as u32)
    }
    unsafe fn from_map_value<'a>(value: FfiMapValue) -> View<'a, Self> {
        debug_assert_eq!(value.tag, FfiMapValueTag::U32);
        unsafe { value.val.u as i32 }
    }
}

impl CppMapTypeConversions for u64 {
    fn get_prototype() -> FfiMapValue {
        FfiMapValue::make_u64(0)
    }
    fn to_map_value(self) -> FfiMapValue {
        FfiMapValue::make_u64(self)
    }
    unsafe fn from_map_value<'a>(value: FfiMapValue) -> View<'a, Self> {
        debug_assert_eq!(value.tag, FfiMapValueTag::U64);
        unsafe { value.val.uu }
    }
}

impl CppMapTypeConversions for i64 {
    fn get_prototype() -> FfiMapValue {
        FfiMapValue::make_u64(0)
    }
    fn to_map_value(self) -> FfiMapValue {
        FfiMapValue::make_u64(self as u64)
    }
    unsafe fn from_map_value<'a>(value: FfiMapValue) -> View<'a, Self> {
        debug_assert_eq!(value.tag, FfiMapValueTag::U64);
        unsafe { value.val.uu as i64 }
    }
}

impl CppMapTypeConversions for f32 {
    fn get_prototype() -> FfiMapValue {
        FfiMapValue::make_f32(0f32)
    }
    fn to_map_value(self) -> FfiMapValue {
        FfiMapValue::make_f32(self)
    }
    unsafe fn from_map_value<'a>(value: FfiMapValue) -> View<'a, Self> {
        debug_assert_eq!(value.tag, FfiMapValueTag::F32);
        unsafe { value.val.f }
    }
}

impl CppMapTypeConversions for f64 {
    fn get_prototype() -> FfiMapValue {
        FfiMapValue::make_f64(0.0)
    }
    fn to_map_value(self) -> FfiMapValue {
        FfiMapValue::make_f64(self)
    }
    unsafe fn from_map_value<'a>(value: FfiMapValue) -> View<'a, Self> {
        debug_assert_eq!(value.tag, FfiMapValueTag::F64);
        unsafe { value.val.ff }
    }
}

impl CppMapTypeConversions for bool {
    fn get_prototype() -> FfiMapValue {
        FfiMapValue::make_bool(false)
    }
    fn to_map_value(self) -> FfiMapValue {
        FfiMapValue::make_bool(self)
    }
    unsafe fn from_map_value<'a>(value: FfiMapValue) -> View<'a, Self> {
        debug_assert_eq!(value.tag, FfiMapValueTag::Bool);
        unsafe { value.val.b }
    }
}

impl CppMapTypeConversions for ProtoString {
    fn get_prototype() -> FfiMapValue {
        FfiMapValue { tag: FfiMapValueTag::String, val: FfiMapValueUnion { s: None } }
    }

    fn to_map_value(self) -> FfiMapValue {
        FfiMapValue::make_string(protostr_into_cppstdstring(self))
    }

    unsafe fn from_map_value<'a>(value: FfiMapValue) -> &'a ProtoStr {
        debug_assert_eq!(value.tag, FfiMapValueTag::String);
        unsafe { ptrlen_to_str(proto2_rust_cpp_string_to_view(value.val.s.unwrap())) }
    }
}

impl CppMapTypeConversions for ProtoBytes {
    fn get_prototype() -> FfiMapValue {
        FfiMapValue { tag: FfiMapValueTag::String, val: FfiMapValueUnion { s: None } }
    }

    fn to_map_value(self) -> FfiMapValue {
        FfiMapValue::make_string(protobytes_into_cppstdstring(self))
    }

    unsafe fn from_map_value<'a>(value: FfiMapValue) -> &'a [u8] {
        debug_assert_eq!(value.tag, FfiMapValueTag::String);
        unsafe { proto2_rust_cpp_string_to_view(value.val.s.unwrap()).as_ref() }
    }
}

// This trait encapsulates functionality that is specific to each map key type.
// We need this primarily so that we can call the appropriate FFI function for
// the key type.
#[doc(hidden)]
pub trait FfiMapKey
where
    Self: Proxied,
{
    type FfiKey;

    fn to_view<'a>(key: Self::FfiKey) -> View<'a, Self>;

    unsafe fn insert(m: RawMap, key: View<'_, Self>, value: FfiMapValue) -> bool;

    unsafe fn get(m: RawMap, key: View<'_, Self>, value: *mut FfiMapValue) -> bool;

    unsafe fn iter_get(
        iter: &mut UntypedMapIterator,
        key: *mut Self::FfiKey,
        value: *mut FfiMapValue,
    );

    unsafe fn remove(m: RawMap, key: View<'_, Self>) -> bool;
}

macro_rules! generate_map_key_impl {
    ( $($key:ty, $mutable_ffi_key:ty, $to_ffi:expr, $from_ffi:expr;)* ) => {
        paste! {
        $(
        impl FfiMapKey for $key {
            type FfiKey = $mutable_ffi_key;

            #[inline]
            fn to_view<'a>(key: Self::FfiKey) -> View<'a, Self> {
                $from_ffi(key)
            }

            #[inline]
            unsafe fn insert(
                m: RawMap,
                key: View<'_, Self>,
                value: FfiMapValue,
            ) -> bool {
                unsafe { [< proto2_rust_map_insert_ $key >](m, $to_ffi(key), value) }
            }

            #[inline]
            unsafe fn get(
                m: RawMap,
                key: View<'_, Self>,
                value: *mut FfiMapValue,
            ) -> bool {
                unsafe { [< proto2_rust_map_get_ $key >](m, $to_ffi(key), value) }
            }

            #[inline]
            unsafe fn iter_get(
                iter: &mut UntypedMapIterator,
                key: *mut Self::FfiKey,
                value: *mut FfiMapValue,
            ) {
                unsafe { [< proto2_rust_map_iter_get_ $key >](iter, key, value) }
            }

            #[inline]
            unsafe fn remove(m: RawMap, key: View<'_, Self>) -> bool {
                unsafe { [< proto2_rust_map_remove_ $key >](m, $to_ffi(key)) }
            }
        }
        )*
        }
    }
}

generate_map_key_impl!(
    bool, bool, identity, identity;
    i32, i32, identity, identity;
    u32, u32, identity, identity;
    i64, i64, identity, identity;
    u64, u64, identity, identity;
    ProtoString, PtrAndLen, str_to_ptrlen, ptrlen_to_str;
);

impl<Value> MapValue for Value
where
    Value: Singular + CppMapTypeConversions,
{
    fn map_new<Key: MapKey>(_private: Private) -> Map<Key, Self> {
        unsafe {
            Map::from_inner(
                Private,
                InnerMap::new(proto2_rust_map_new(Key::get_prototype(), Value::get_prototype())),
            )
        }
    }

    unsafe fn map_free<Key: MapKey>(_private: Private, map: &mut Map<Key, Self>) {
        unsafe {
            proto2_rust_map_free(map.as_raw(Private));
        }
    }

    fn map_clear<Key: MapKey>(_private: Private, mut map: MapMut<Key, Self>) {
        unsafe {
            proto2_rust_map_clear(map.as_raw(Private));
        }
    }

    fn map_len<Key: MapKey>(_private: Private, map: MapView<Key, Self>) -> usize {
        unsafe { proto2_rust_map_size(map.as_raw(Private)) }
    }

    fn map_insert<Key: MapKey>(
        _private: Private,
        mut map: MapMut<Key, Self>,
        key: View<'_, Key>,
        value: impl IntoProxied<Self>,
    ) -> bool {
        unsafe { Key::insert(map.as_raw(Private), key, value.into_proxied(Private).to_map_value()) }
    }

    fn map_get<'a, Key: MapKey>(
        _private: Private,
        map: MapView<'a, Key, Self>,
        key: View<'_, Key>,
    ) -> Option<View<'a, Self>> {
        let mut value = std::mem::MaybeUninit::uninit();
        let found = unsafe { Key::get(map.as_raw(Private), key, value.as_mut_ptr()) };
        if !found {
            return None;
        }
        unsafe { Some(Self::from_map_value(value.assume_init())) }
    }

    fn map_get_mut<'a, Key: MapKey>(
        _private: Private,
        mut map: MapMut<'a, Key, Self>,
        key: View<'_, Key>,
    ) -> Option<Mut<'a, Self>>
    where
        Value: Message,
    {
        let mut value = std::mem::MaybeUninit::uninit();
        let found = unsafe { Key::get(map.as_raw(Private), key, value.as_mut_ptr()) };
        if !found {
            return None;
        }
        // SAFETY: `value` has been initialized because it was found.
        // - `value` is a message as required by the trait.
        // - `value` is valid for the `'a` lifetime of the `MapMut`.
        unsafe { Some(Self::mut_from_map_value(value.assume_init())) }
    }

    fn map_remove<Key: MapKey>(
        _private: Private,
        mut map: MapMut<Key, Self>,
        key: View<'_, Key>,
    ) -> bool {
        unsafe { Key::remove(map.as_raw(Private), key) }
    }

    fn map_iter<Key: MapKey>(_private: Private, map: MapView<Key, Self>) -> MapIter<Key, Self> {
        // SAFETY:
        // - The backing map for `map.as_raw` is valid for at least '_.
        // - A View that is live for '_ guarantees the backing map is unmodified for '_.
        // - The `iter` function produces an iterator that is valid for the key and
        //   value types, and live for at least '_.
        unsafe { MapIter::from_raw(Private, proto2_rust_map_iter(map.as_raw(Private))) }
    }

    fn map_iter_next<'a, Key: MapKey>(
        _private: Private,
        iter: &mut MapIter<'a, Key, Self>,
    ) -> Option<(View<'a, Key>, View<'a, Self>)> {
        // SAFETY:
        // - The `MapIter` API forbids the backing map from being mutated for 'a, and
        //   guarantees that it's the correct key and value types.
        // - The thunk is safe to call as long as the iterator isn't at the end.
        // - The thunk always writes to key and value fields and does not read.
        // - The thunk does not increment the iterator.
        unsafe {
            iter.as_raw_mut(Private).next_unchecked::<Key, Self, _, _>(
                |iter, key, value| Key::iter_get(iter, key, value),
                |ffi_key| Key::to_view(ffi_key),
                |value| Self::from_map_value(value),
            )
        }
    }
}

macro_rules! impl_map_primitives {
    (@impl $(($rust_type:ty, $cpp_type:ty) => [
        $insert_thunk:ident,
        $get_thunk:ident,
        $iter_get_thunk:ident,
        $remove_thunk:ident,
    ]),* $(,)?) => {
        $(
            unsafe extern "C" {
                pub fn $insert_thunk(
                    m: RawMap,
                    key: $cpp_type,
                    value: FfiMapValue,
                ) -> bool;
                pub fn $get_thunk(
                    m: RawMap,
                    key: $cpp_type,
                    value: *mut FfiMapValue,
                ) -> bool;
                pub fn $iter_get_thunk(
                    iter: &mut UntypedMapIterator,
                    key: *mut $cpp_type,
                    value: *mut FfiMapValue,
                );
                pub fn $remove_thunk(m: RawMap, key: $cpp_type) -> bool;
            }
        )*
    };
    ($($rust_type:ty, $cpp_type:ty;)* $(,)?) => {
        paste!{
            impl_map_primitives!(@impl $(
                    ($rust_type, $cpp_type) => [
                    [< proto2_rust_map_insert_ $rust_type >],
                    [< proto2_rust_map_get_ $rust_type >],
                    [< proto2_rust_map_iter_get_ $rust_type >],
                    [< proto2_rust_map_remove_ $rust_type >],
                ],
            )*);
        }
    };
}

impl_map_primitives!(
    i32, i32;
    u32, u32;
    i64, i64;
    u64, u64;
    bool, bool;
    ProtoString, PtrAndLen;
);
