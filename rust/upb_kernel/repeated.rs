use super::*;

/// The raw type-erased version of an owned `Repeated`.
#[derive(Debug)]
#[doc(hidden)]
pub struct InnerRepeated {
    raw: RawRepeatedField,
    arena: Arena,
}

impl InnerRepeated {
    pub fn as_mut(&mut self) -> InnerRepeatedMut<'_> {
        InnerRepeatedMut::new(self.raw, &self.arena)
    }

    pub fn raw(&self) -> RawRepeatedField {
        self.raw
    }

    pub fn arena(&self) -> &Arena {
        &self.arena
    }

    /// # Safety
    /// - `raw` must be a valid `RawRepeatedField`
    pub unsafe fn from_raw_parts(raw: RawRepeatedField, arena: Arena) -> Self {
        Self { raw, arena }
    }
}

/// The raw type-erased pointer version of `RepeatedMut`.
#[derive(Clone, Copy, Debug)]
#[doc(hidden)]
pub struct InnerRepeatedMut<'msg> {
    pub(crate) raw: RawRepeatedField,
    arena: &'msg Arena,
}

impl<'msg> InnerRepeatedMut<'msg> {
    #[doc(hidden)]
    pub fn new(raw: RawRepeatedField, arena: &'msg Arena) -> Self {
        InnerRepeatedMut { raw, arena }
    }
}

unsafe impl<T> Singular for T
where
    T: EntityType + UpbTypeConversions<T::Tag>,
{
    fn repeated_new(_private: Private) -> Repeated<Self> {
        let arena = Arena::new();
        Repeated::from_inner(Private, unsafe {
            InnerRepeated::from_raw_parts(upb_Array_New(arena.raw(), T::upb_type()), arena)
        })
    }

    unsafe fn repeated_free(_private: Private, _repeated: &mut Repeated<Self>) {
        // No-op: the memory will be dropped by the arena.
    }

    fn repeated_len(_private: Private, repeated: View<Repeated<Self>>) -> usize {
        // SAFETY: `repeated.as_raw()` is a valid `upb_Array*`.
        unsafe { upb_Array_Size(repeated.as_raw(Private)) }
    }

    fn repeated_push(
        _private: Private,
        mut repeated: Mut<Repeated<Self>>,
        val: impl IntoProxied<Self>,
    ) {
        // SAFETY:
        // - `repeated.as_raw()` is a valid `upb_Array*`.
        // - `msg_ptr` is a valid `upb_Message*`.
        unsafe {
            upb_Array_Append(
                repeated.as_raw(Private),
                T::into_message_value_fuse_if_required(
                    repeated.raw_arena(Private),
                    val.into_proxied(Private),
                ),
                repeated.raw_arena(Private),
            );
        };
    }

    fn repeated_clear(_private: Private, mut repeated: Mut<Repeated<Self>>) {
        // SAFETY:
        // - `repeated.as_raw()` is a valid `upb_Array*`.
        unsafe { upb_Array_Resize(repeated.as_raw(Private), 0, repeated.raw_arena(Private)) };
    }

    unsafe fn repeated_get_unchecked<'a>(
        _private: Private,
        repeated: View<'a, Repeated<Self>>,
        index: usize,
    ) -> View<'a, Self> {
        // SAFETY:
        // - `repeated.as_raw()` is a valid `const upb_Array*`.
        // - `index < len(repeated)` is promised by the caller.
        let val = unsafe { upb_Array_Get(repeated.as_raw(Private), index) };
        // SAFETY:
        // - `val` has the correct variant for Self.
        // - `val` is valid for `'a` lifetime.
        unsafe { Self::from_message_value(val) }
    }

    unsafe fn repeated_get_mut_unchecked<'a>(
        _private: Private,
        mut repeated: Mut<'a, Repeated<Self>>,
        index: usize,
    ) -> Mut<'a, Self>
    where
        Self: Message,
    {
        // SAFETY:
        // - `repeated.as_raw()` is a valid `upb_Array*`.
        // - `repeated` is a an array of message-valued elements.
        // - `index < len(repeated)` is promised by the caller.
        let val = unsafe { upb_Array_GetMutable(repeated.as_raw(Private), index) };
        // SAFETY:
        // - `val` is the correct variant for `Self`.
        // - `val` is valid for `'a` lifetime.
        unsafe { Self::from_message_mut(val, repeated.arena(Private)) }
    }

    unsafe fn repeated_set_unchecked(
        _private: Private,
        mut repeated: Mut<Repeated<Self>>,
        index: usize,
        val: impl IntoProxied<Self>,
    ) {
        unsafe {
            upb_Array_Set(
                repeated.as_raw(Private),
                index,
                T::into_message_value_fuse_if_required(
                    repeated.raw_arena(Private),
                    val.into_proxied(Private),
                ),
            )
        }
    }

    fn repeated_copy_from(
        _private: Private,
        src: View<Repeated<Self>>,
        mut dest: Mut<Repeated<Self>>,
    ) {
        // SAFETY:
        // - `src.as_raw()` and `dest.as_raw()` are both valid arrays of `Self`.
        // - `dest.as_raw()` is mutable.
        // - `dest.raw_arena()` will outlive `dest.as_raw()`.
        unsafe {
            Self::copy_repeated(src.as_raw(Private), dest.as_raw(Private), dest.raw_arena(Private));
        }
    }

    fn repeated_reserve(_private: Private, mut repeated: Mut<Repeated<Self>>, additional: usize) {
        // SAFETY:
        // - `repeated.as_raw()` is a valid `upb_Array*`.
        unsafe {
            let size = upb_Array_Size(repeated.as_raw(Private));
            upb_Array_Reserve(
                repeated.as_raw(Private),
                size + additional,
                repeated.raw_arena(Private),
            );
        }
    }
}

impl<'msg, T> RepeatedMut<'msg, T> {
    // Returns a `RawArena` which is live for at least `'msg`
    #[doc(hidden)]
    pub fn raw_arena(&mut self, _private: Private) -> RawArena {
        self.inner.arena.raw()
    }

    // Returns an `Arena` which is live for at least `'msg`
    #[doc(hidden)]
    pub fn arena(&self, _private: Private) -> &'msg Arena {
        self.inner.arena
    }
}

/// Returns a static empty RepeatedView.
pub fn empty_array<T: Singular>() -> RepeatedView<'static, T> {
    // TODO: Consider creating a static empty array in C.

    // Use `i32` for a shared empty repeated for all repeated types in the program.
    static EMPTY_REPEATED_VIEW: OnceLock<Repeated<i32>> = OnceLock::new();

    // SAFETY:
    // - Because the repeated is never mutated, the repeated type is unused and
    //   therefore valid for `T`.
    unsafe {
        RepeatedView::from_raw(
            Private,
            EMPTY_REPEATED_VIEW.get_or_init(Repeated::new).as_view().as_raw(Private),
        )
    }
}
