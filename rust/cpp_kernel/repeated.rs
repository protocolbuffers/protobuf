use super::*;

unsafe extern "C" {
    pub fn proto2_rust_RepeatedField_Message_new() -> RawRepeatedField;
    pub fn proto2_rust_RepeatedField_Message_free(field: RawRepeatedField);
    pub fn proto2_rust_RepeatedField_Message_size(field: RawRepeatedField) -> usize;
    pub fn proto2_rust_RepeatedField_Message_get(
        field: RawRepeatedField,
        index: usize,
    ) -> RawMessage;
    pub fn proto2_rust_RepeatedField_Message_get_mut(
        field: RawRepeatedField,
        index: usize,
    ) -> RawMessage;
    pub fn proto2_rust_RepeatedField_Message_add(
        field: RawRepeatedField,
        prototype: RawMessage,
    ) -> RawMessage;
    pub fn proto2_rust_RepeatedField_Message_set(
        field: RawRepeatedField,
        index: usize,
        prototype: RawMessage,
    ) -> RawMessage;
    pub fn proto2_rust_RepeatedField_Message_clear(field: RawRepeatedField);
    pub fn proto2_rust_RepeatedField_Message_copy_from(
        dst: RawRepeatedField,
        src: RawRepeatedField,
    );
    pub fn proto2_rust_RepeatedField_Message_reserve(field: RawRepeatedField, additional: usize);
}

/// The raw type-erased version of an owned `Repeated`.
#[derive(Debug)]
#[doc(hidden)]
pub struct InnerRepeated {
    raw: RawRepeatedField,
}

impl InnerRepeated {
    pub fn as_mut(&mut self) -> InnerRepeatedMut<'_> {
        InnerRepeatedMut::new(self.raw)
    }

    pub fn raw(&self) -> RawRepeatedField {
        self.raw
    }

    /// # Safety
    /// - `raw` must be a valid `proto2::RepeatedField*` or
    ///   `proto2::RepeatedPtrField*`.
    pub unsafe fn from_raw(raw: RawRepeatedField) -> Self {
        Self { raw }
    }
}

/// The raw type-erased pointer version of `RepeatedMut`.
///
/// Contains a `proto2::RepeatedField*` or `proto2::RepeatedPtrField*`.
#[derive(Clone, Copy, Debug)]
#[doc(hidden)]
pub struct InnerRepeatedMut<'msg> {
    pub(crate) raw: RawRepeatedField,
    _phantom: PhantomData<&'msg ()>,
}

impl<'msg> InnerRepeatedMut<'msg> {
    #[doc(hidden)]
    pub fn new(raw: RawRepeatedField) -> Self {
        InnerRepeatedMut { raw, _phantom: PhantomData }
    }
}

pub trait CppTypeConversions: Proxied {
    type InsertElemType;
    type ElemType;

    fn elem_to_view<'msg>(v: Self::ElemType) -> View<'msg, Self>;
    fn into_insertelem(v: Self) -> Self::InsertElemType;
}

macro_rules! impl_cpp_type_conversions_for_scalars {
    ($($t:ty),* $(,)?) => {
        $(
            impl CppTypeConversions for $t {
                type InsertElemType = Self;
                type ElemType = Self;

                fn elem_to_view<'msg>(v: Self) -> View<'msg, Self> {
                    v
                }

                fn into_insertelem(v: Self) -> Self {
                    v
                }
            }
        )*
    }
}

impl_cpp_type_conversions_for_scalars!(i32, u32, i64, u64, f32, f64, bool);

impl CppTypeConversions for ProtoString {
    type InsertElemType = CppStdString;
    type ElemType = PtrAndLen;

    fn elem_to_view<'msg>(v: PtrAndLen) -> View<'msg, ProtoString> {
        ptrlen_to_str(v)
    }

    fn into_insertelem(v: Self) -> CppStdString {
        v.into_inner(Private).into_raw()
    }
}

impl CppTypeConversions for ProtoBytes {
    type InsertElemType = CppStdString;
    type ElemType = PtrAndLen;

    fn elem_to_view<'msg>(v: Self::ElemType) -> View<'msg, Self> {
        ptrlen_to_bytes(v)
    }

    fn into_insertelem(v: Self) -> CppStdString {
        v.into_inner(Private).into_raw()
    }
}

unsafe impl<T> Singular for T
where
    Self: MutProxied + Message,
    for<'a> View<'a, Self>: From<MessageViewInner<'a, Self>> + std::default::Default,
    for<'a> Mut<'a, Self>: From<MessageMutInner<'a, Self>>,
{
    fn repeated_new(_private: Private) -> Repeated<Self> {
        // SAFETY:
        // - The thunk returns an unaliased and valid `RepeatedPtrField*`
        unsafe {
            Repeated::from_inner(
                Private,
                InnerRepeated::from_raw(proto2_rust_RepeatedField_Message_new()),
            )
        }
    }

    unsafe fn repeated_free(_private: Private, f: &mut Repeated<Self>) {
        // SAFETY
        // - `f.raw()` is a valid `RepeatedPtrField*`.
        unsafe { proto2_rust_RepeatedField_Message_free(f.as_view().as_raw(Private)) }
    }

    fn repeated_len(_private: Private, f: View<Repeated<Self>>) -> usize {
        // SAFETY: `f.as_raw()` is a valid `RepeatedPtrField*`.
        unsafe { proto2_rust_RepeatedField_Message_size(f.as_raw(Private)) }
    }

    unsafe fn repeated_set_unchecked(
        _private: Private,
        mut f: Mut<Repeated<Self>>,
        i: usize,
        v: impl IntoProxied<Self>,
    ) {
        // SAFETY:
        // - `f.as_raw()` is a valid `RepeatedPtrField*`.
        // - `i < len(f)` is promised by caller.
        // - The second argument below is a valid `const Message&`.
        unsafe {
            proto2_rust_Message_copy_from(
                proto2_rust_RepeatedField_Message_get_mut(f.as_raw(Private), i),
                v.into_proxied(Private).get_raw_message(Private),
            );
        }
    }

    unsafe fn repeated_get_unchecked(
        _private: Private,
        f: View<Repeated<Self>>,
        i: usize,
    ) -> View<Self> {
        // SAFETY:
        // - `f.as_raw()` is a valid `const RepeatedPtrField&`.
        // - `i < len(f)` is promised by caller.
        let msg = unsafe { proto2_rust_RepeatedField_Message_get(f.as_raw(Private), i) };
        let inner = unsafe { MessageViewInner::wrap_raw(msg) };
        inner.into()
    }

    unsafe fn repeated_get_mut_unchecked(
        _private: Private,
        mut f: Mut<Repeated<Self>>,
        i: usize,
    ) -> Mut<Self> {
        // SAFETY:
        // - `f.as_raw()` is a valid `RepeatedPtrField*`.
        // - `i < len(f)` is promised by caller.
        let msg = unsafe { proto2_rust_RepeatedField_Message_get_mut(f.as_raw(Private), i) };
        let inner = unsafe { MessageMutInner::wrap_raw(msg) };
        inner.into()
    }

    fn repeated_clear(_private: Private, mut f: Mut<Repeated<Self>>) {
        // SAFETY:
        // - `f.as_raw()` is a valid `RepeatedPtrField*`.
        unsafe { proto2_rust_RepeatedField_Message_clear(f.as_raw(Private)) };
    }

    fn repeated_push(_private: Private, mut f: Mut<Repeated<Self>>, v: impl IntoProxied<Self>) {
        // SAFETY:
        // - `f.as_raw()` is a valid `RepeatedPtrField*`.
        // - The second argument below is a valid `const Message&`.
        unsafe {
            let prototype =
                <View<Self> as std::default::Default>::default().get_raw_message(Private);
            let new_elem = proto2_rust_RepeatedField_Message_add(f.as_raw(Private), prototype);
            proto2_rust_Message_copy_from(
                new_elem,
                v.into_proxied(Private).get_raw_message(Private),
            );
        }
    }

    fn repeated_copy_from(
        _private: Private,
        src: View<Repeated<Self>>,
        mut dest: Mut<Repeated<Self>>,
    ) {
        // SAFETY:
        // - `dest.as_raw()` is a valid `RepeatedPtrField*`.
        // - `src.as_raw()` is a valid `const RepeatedPtrField&`.
        unsafe {
            proto2_rust_RepeatedField_Message_copy_from(dest.as_raw(Private), src.as_raw(Private));
        }
    }

    fn repeated_reserve(_private: Private, mut f: Mut<Repeated<Self>>, additional: usize) {
        // SAFETY:
        // - `f.as_raw()` is a valid `RepeatedPtrField*`.
        unsafe { proto2_rust_RepeatedField_Message_reserve(f.as_raw(Private), additional) }
    }
}

macro_rules! impl_repeated_primitives {
    (@impl $($t:ty => [
        $new_thunk:ident,
        $free_thunk:ident,
        $add_thunk:ident,
        $size_thunk:ident,
        $get_thunk:ident,
        $set_thunk:ident,
        $clear_thunk:ident,
        $copy_from_thunk:ident,
        $reserve_thunk:ident $(,)?
    ]),* $(,)?) => {
        $(
            unsafe extern "C" {
                fn $new_thunk() -> RawRepeatedField;
                fn $free_thunk(f: RawRepeatedField);
                fn $add_thunk(f: RawRepeatedField, v: <$t as CppTypeConversions>::InsertElemType);
                fn $size_thunk(f: RawRepeatedField) -> usize;
                fn $get_thunk(
                    f: RawRepeatedField,
                    i: usize) -> <$t as CppTypeConversions>::ElemType;
                fn $set_thunk(
                    f: RawRepeatedField,
                    i: usize,
                    v: <$t as CppTypeConversions>::InsertElemType);
                fn $clear_thunk(f: RawRepeatedField);
                fn $copy_from_thunk(src: RawRepeatedField, dst: RawRepeatedField);
                fn $reserve_thunk(
                    f: RawRepeatedField,
                    additional: usize);
            }

            unsafe impl Singular for $t {
                #[allow(dead_code)]
                #[inline]
                fn repeated_new(_: Private) -> Repeated<$t> {
                    Repeated::from_inner(Private, InnerRepeated {
                        raw: unsafe { $new_thunk() }
                    })
                }
                #[allow(dead_code)]
                #[inline]
                unsafe fn repeated_free(_: Private, f: &mut Repeated<$t>) {
                    unsafe { $free_thunk(f.as_mut().as_raw(Private)) }
                }
                #[inline]
                fn repeated_len(_private: Private, f: View<Repeated<$t>>) -> usize {
                    unsafe { $size_thunk(f.as_raw(Private)) }
                }
                #[inline]
                fn repeated_push(_private: Private, mut f: Mut<Repeated<$t>>, v: impl IntoProxied<$t>) {
                    unsafe { $add_thunk(f.as_raw(Private), <$t as CppTypeConversions>::into_insertelem(v.into_proxied(Private))) }
                }
                #[inline]
                fn repeated_clear(_private: Private, mut f: Mut<Repeated<$t>>) {
                    unsafe { $clear_thunk(f.as_raw(Private)) }
                }
                #[inline]
                unsafe fn repeated_get_unchecked(_private: Private, f: View<Repeated<$t>>, i: usize) -> View<$t> {
                    unsafe {
                        <$t as CppTypeConversions>::elem_to_view(
                        $get_thunk(f.as_raw(Private), i))
                    }
                }
                #[inline]
                unsafe fn repeated_set_unchecked(_private: Private, mut f: Mut<Repeated<$t>>, i: usize, v: impl IntoProxied<$t>) {
                    unsafe { $set_thunk(f.as_raw(Private), i, <$t as CppTypeConversions>::into_insertelem(v.into_proxied(Private))) }
                }
                #[inline]
                fn repeated_copy_from(_private: Private, src: View<Repeated<$t>>, mut dest: Mut<Repeated<$t>>) {
                    unsafe { $copy_from_thunk(src.as_raw(Private), dest.as_raw(Private)) }
                }
                #[inline]
                fn repeated_reserve(_private: Private, mut f: Mut<Repeated<$t>>, additional: usize) {
                    unsafe { $reserve_thunk(f.as_raw(Private), additional) }
                }
            }
        )*
    };
    ($($t:ty),* $(,)?) => {
        paste!{
            impl_repeated_primitives!(@impl $(
                $t => [
                    [< proto2_rust_RepeatedField_ $t _new >],
                    [< proto2_rust_RepeatedField_ $t _free >],
                    [< proto2_rust_RepeatedField_ $t _add >],
                    [< proto2_rust_RepeatedField_ $t _size >],
                    [< proto2_rust_RepeatedField_ $t _get >],
                    [< proto2_rust_RepeatedField_ $t _set >],
                    [< proto2_rust_RepeatedField_ $t _clear >],
                    [< proto2_rust_RepeatedField_ $t _copy_from >],
                    [< proto2_rust_RepeatedField_ $t _reserve >],
                ],
            )*);
        }
    };
}

impl_repeated_primitives!(i32, u32, i64, u64, f32, f64, bool, ProtoString, ProtoBytes);

/// Cast a `RepeatedView<SomeEnum>` to `RepeatedView<c_int>`.
pub fn cast_enum_repeated_view<E: Enum>(repeated: RepeatedView<E>) -> RepeatedView<c_int> {
    // SAFETY: the implementer of `Enum` has promised that this
    // raw repeated is a type-erased `proto2::RepeatedField<int>*`.
    unsafe { RepeatedView::from_raw(Private, repeated.as_raw(Private)) }
}

/// Cast a `RepeatedMut<SomeEnum>` to `RepeatedMut<c_int>`.
///
/// Writing an unknown value is sound because all enums
/// are representationally open.
pub fn cast_enum_repeated_mut<E: Enum>(mut repeated: RepeatedMut<E>) -> RepeatedMut<c_int> {
    // SAFETY: the implementer of `Enum` has promised that this
    // raw repeated is a type-erased `proto2::RepeatedField<int>*`.
    unsafe {
        RepeatedMut::from_inner(
            Private,
            InnerRepeatedMut { raw: repeated.as_raw(Private), _phantom: PhantomData },
        )
    }
}

/// Cast a `RepeatedMut<SomeEnum>` to `RepeatedMut<c_int>` and call
/// repeated_reserve.
pub fn reserve_enum_repeated_mut<E: Enum>(repeated: RepeatedMut<E>, additional: usize) {
    let int_repeated = cast_enum_repeated_mut(repeated);
    Singular::repeated_reserve(Private, int_repeated, additional);
}

pub fn new_enum_repeated<E: Enum>() -> Repeated<E> {
    let int_repeated = Repeated::<c_int>::new();
    let raw = int_repeated.inner.raw();
    std::mem::forget(int_repeated);
    unsafe { Repeated::from_inner(Private, InnerRepeated::from_raw(raw)) }
}

/// Cast a `RepeatedMut<SomeEnum>` to `RepeatedMut<c_int>` and call
/// repeated_free.
/// # Safety
/// - The passed in `&mut Repeated<E>` must not be used after this function is
///   called.
pub unsafe fn free_enum_repeated<E: Enum>(repeated: &mut Repeated<E>) {
    unsafe {
        let mut int_r: Repeated<c_int> =
            Repeated::from_inner(Private, InnerRepeated::from_raw(repeated.inner.raw()));
        Singular::repeated_free(Private, &mut int_r);
        std::mem::forget(int_r);
    }
}
