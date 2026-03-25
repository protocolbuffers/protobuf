use super::*;
use crate::Enum;

pub trait UpbTypeConversions<Tag>: Proxied {
    fn upb_type() -> upb::CType;

    fn to_message_value(val: View<'_, Self>) -> upb_MessageValue;

    /// # Safety
    /// - `raw_arena` must point to a valid upb arena.
    unsafe fn into_message_value_fuse_if_required(
        raw_arena: RawArena,
        val: Self,
    ) -> upb_MessageValue;

    /// # Safety
    /// - `msg_val` must be the correct variant for `Self`.
    /// - `msg_val` pointers must point to memory valid for `'msg` lifetime.
    /// - If `Self` is a closed enum, then `msg_val.int32_val` must be a valid enum entry.
    unsafe fn from_message_value<'msg>(msg_val: upb_MessageValue) -> View<'msg, Self>;

    /// # Safety
    /// - `raw` must be the correct variant for `Self`.
    /// - `raw` pointers must point to memory valid for `'msg` lifetime.
    #[allow(unused_variables)]
    unsafe fn from_message_mut<'msg>(raw: RawMessage, arena: &'msg Arena) -> Mut<'msg, Self>
    where
        Self: Message,
    {
        panic!("mut_from_message_value is only implemented for messages.")
    }

    /// # Safety
    /// - `src` must be a valid array of `Self`.
    /// - `dest` must be a valid mutable array of `Self`.
    /// - `arena` must point to an arena that will outlive `dest`.
    unsafe fn copy_repeated(src: RawArray, dest: RawArray, arena: RawArena);
}

impl<T> UpbTypeConversions<MessageTag> for T
where
    Self: Message,
    for<'a> View<'a, Self>: MessageViewInterop<'a>,
    for<'a> Mut<'a, Self>: From<MessageMutInner<'a, Self>>,
{
    fn upb_type() -> CType {
        CType::Message
    }

    fn to_message_value(val: View<'_, Self>) -> upb_MessageValue {
        upb_MessageValue { msg_val: Some(val.get_ptr(Private).raw()) }
    }

    unsafe fn into_message_value_fuse_if_required(
        raw_parent_arena: RawArena,
        mut val: Self,
    ) -> upb_MessageValue {
        // SAFETY: The arena memory is not freed due to `ManuallyDrop`.
        let parent_arena =
            std::mem::ManuallyDrop::new(unsafe { Arena::from_raw(raw_parent_arena) });

        parent_arena.fuse(val.get_arena(Private));
        upb_MessageValue { msg_val: Some(val.get_ptr(Private).raw()) }
    }

    unsafe fn from_message_value<'msg>(msg: upb_MessageValue) -> View<'msg, Self> {
        unsafe {
            let raw = msg.msg_val.expect("expected present message value in map");
            View::<Self>::__unstable_wrap_raw_message_unchecked_lifetime(
                raw.as_ptr() as *const std::ffi::c_void
            )
        }
    }

    unsafe fn from_message_mut<'msg>(msg: RawMessage, arena: &'msg Arena) -> Mut<'msg, Self> {
        unsafe { MessageMutInner::<'msg, Self>::wrap_raw(msg, arena).into() }
    }

    unsafe fn copy_repeated(src: RawArray, dest: RawArray, arena: RawArena) {
        // SAFETY:
        // - `src` is a valid `const upb_Array*`.
        // - `dest` is a valid `upb_Array*`.
        // - Elements of `src` and `dest` have minitable `Self::mini_table()`.
        unsafe {
            let size = upb_Array_Size(src);
            if !upb_Array_Resize(dest, size, arena) {
                panic!("upb_Array_Resize failed (alloc should be infallible)");
            }
            for i in 0..size {
                let src_msg =
                    upb_Array_Get(src, i).msg_val.expect("upb_Array* element should not be NULL");
                // Avoid the use of `upb_Array_DeepClone` as it creates an
                // entirely new `upb_Array*` at a new memory address.
                let cloned_msg = upb_Message_DeepClone(src_msg, Self::mini_table(), arena)
                    .expect("upb_Message_DeepClone failed (alloc should be infallible)");
                upb_Array_Set(dest, i, upb_MessageValue { msg_val: Some(cloned_msg) });
            }
        }
    }
}

impl<T> UpbTypeConversions<EnumTag> for T
where
    Self: Enum,
{
    fn upb_type() -> CType {
        CType::Enum
    }

    fn to_message_value(val: View<'_, Self>) -> upb_MessageValue {
        upb_MessageValue { int32_val: val.into() }
    }

    unsafe fn into_message_value_fuse_if_required(
        _raw_parent_arena: RawArena,
        val: Self,
    ) -> upb_MessageValue {
        upb_MessageValue { int32_val: val.into() }
    }

    unsafe fn from_message_value<'msg>(val: upb_MessageValue) -> View<'msg, Self> {
        // SAFETY: The caller guarantees that `val` is the correct variant.
        let result = Self::try_from(unsafe { val.int32_val });
        std::debug_assert!(result.is_ok());
        // SAFETY:
        // - The caller guarantees that `val.int32_val` is valid for this enum.
        unsafe { result.unwrap_unchecked() }
    }

    unsafe fn copy_repeated(src: RawArray, dest: RawArray, arena: RawArena) {
        // SAFETY:
        // - Enum arrays have the same representation as i32 arrays.
        // - The caller guarantees that src and dest are enum arrays and that `arena` will outlive
        //   `dest`.
        unsafe {
            <i32 as UpbTypeConversions<PrimitiveTag>>::copy_repeated(src, dest, arena);
        }
    }
}

macro_rules! impl_upb_type_conversions_for_scalars {
    ($($t:ty, $ufield:ident, $upb_tag:expr, $zero_val:literal;)*) => {
        $(
            impl UpbTypeConversions<PrimitiveTag> for $t {
                #[inline(always)]
                fn upb_type() -> upb::CType {
                    $upb_tag
                }

                #[inline(always)]
                fn to_message_value(val: View<'_, $t>) -> upb_MessageValue {
                    upb_MessageValue { $ufield: val }
                }

                #[inline(always)]
                unsafe fn into_message_value_fuse_if_required(_: RawArena, val: $t) -> upb_MessageValue {
                    <Self as UpbTypeConversions<PrimitiveTag>>::to_message_value(val)
                }

                #[inline(always)]
                unsafe fn from_message_value<'msg>(msg: upb_MessageValue) -> View<'msg, $t> {
                    unsafe { msg.$ufield }
                }

                #[inline(always)]
                unsafe fn copy_repeated(src: RawArray, dest: RawArray, arena: RawArena) {
                    // SAFETY:
                    // - `upb_Array_Resize` is unsafe but assumed to be always sound to call.
                    // - `copy_nonoverlapping` is unsafe but here we guarantee that both pointers
                    //   are valid, the pointers are `#[repr(u8)]`, and the size is correct.
                    unsafe {
                        let len = upb_Array_Size(src);
                        if (!upb_Array_Resize(dest, len, arena)) {
                            panic!("upb_Array_Resize failed (alloc should be infallible)");
                        }
                        ptr::copy_nonoverlapping(
                          upb_Array_DataPtr(src).cast::<u8>(),
                          upb_Array_MutableDataPtr(dest).cast::<u8>(),
                          size_of::<$t>() * len);
                    }
                }
            }
        )*
    };
}

impl_upb_type_conversions_for_scalars!(
    f32, float_val, upb::CType::Float, 0f32;
    f64, double_val, upb::CType::Double, 0f64;
    i32, int32_val, upb::CType::Int32, 0i32;
    u32, uint32_val, upb::CType::UInt32, 0u32;
    i64, int64_val, upb::CType::Int64, 0i64;
    u64, uint64_val, upb::CType::UInt64, 0u64;
    bool, bool_val, upb::CType::Bool, false;
);

impl UpbTypeConversions<PrimitiveTag> for ProtoBytes {
    fn upb_type() -> upb::CType {
        upb::CType::Bytes
    }

    fn to_message_value(val: View<'_, ProtoBytes>) -> upb_MessageValue {
        upb_MessageValue { str_val: val.into() }
    }

    unsafe fn into_message_value_fuse_if_required(
        raw_parent_arena: RawArena,
        val: ProtoBytes,
    ) -> upb_MessageValue {
        // SAFETY: The arena memory is not freed due to `ManuallyDrop`.
        let parent_arena = ManuallyDrop::new(unsafe { Arena::from_raw(raw_parent_arena) });

        let (view, arena) = val.inner.into_raw_parts();
        parent_arena.fuse(&arena);

        upb_MessageValue { str_val: view }
    }

    unsafe fn from_message_value<'msg>(msg: upb_MessageValue) -> View<'msg, ProtoBytes> {
        let _ = unsafe { msg.str_val.as_ref() };
        // We actually need view of ProtoBytes, the original code had:
        // unsafe { msg.str_val.as_ref() }
        // Wait, ProtoBytes view is `&[u8]`. What was the actual code?
        // Ah, it actually was: `unsafe { msg.str_val.as_ref() }`
        // But the return type is `View<'msg, ProtoBytes>`. `View<ProtoBytes>` is `&[u8]`.
        // Let's just return what `msg.str_val.as_ref()` returns, which is `&[u8]`.
        unsafe { msg.str_val.as_ref() }
    }

    unsafe fn copy_repeated(src: RawArray, dest: RawArray, arena: RawArena) {
        unsafe {
            copy_repeated_bytes(src, dest, arena);
        }
    }
}

impl UpbTypeConversions<PrimitiveTag> for ProtoString {
    fn upb_type() -> upb::CType {
        upb::CType::String
    }

    fn to_message_value(val: View<'_, ProtoString>) -> upb_MessageValue {
        upb_MessageValue { str_val: val.as_bytes().into() }
    }

    unsafe fn into_message_value_fuse_if_required(
        raw_arena: RawArena,
        val: ProtoString,
    ) -> upb_MessageValue {
        // SAFETY: `raw_arena` is valid as promised by the caller
        unsafe {
            <ProtoBytes as UpbTypeConversions<PrimitiveTag>>::into_message_value_fuse_if_required(
                raw_arena,
                val.into(),
            )
        }
    }

    unsafe fn from_message_value<'msg>(msg: upb_MessageValue) -> View<'msg, ProtoString> {
        ProtoStr::from_utf8_unchecked(unsafe { msg.str_val.as_ref() })
    }

    unsafe fn copy_repeated(src: RawArray, dest: RawArray, arena: RawArena) {
        unsafe {
            copy_repeated_bytes(src, dest, arena);
        }
    }
}

/// # Safety
/// - `src` must be a valid array of string or bytes.
/// - `dest` must be a valid mutable array of the same type as `src`.
/// - `arena` must point to an arena that will outlive `dest`.
unsafe fn copy_repeated_bytes(src: RawArray, dest: RawArray, arena: RawArena) {
    // SAFETY:
    // - `upb_Array_Resize` is unsafe but assumed to be always sound to call.
    // - `upb_Array` ensures its elements are never uninitialized memory.
    // - The `DataPtr` and `MutableDataPtr` functions return pointers to spans
    //   of memory that are valid for at least `len` elements of PtrAndLen.
    // - `copy_nonoverlapping` is unsafe but here we guarantee that both pointers
    //   are valid, the pointers are `#[repr(u8)]`, and the size is correct.
    // - The bytes held within a valid array are valid.
    unsafe {
        let len = upb_Array_Size(src);
        let arena = ManuallyDrop::new(Arena::from_raw(arena));
        if !upb_Array_Resize(dest, len, arena.raw()) {
            panic!("upb_Array_Resize failed (alloc should be infallible)");
        }
        let src_ptrs: &[PtrAndLen] = slice::from_raw_parts(upb_Array_DataPtr(src).cast(), len);
        let dest_ptrs: &mut [PtrAndLen] =
            slice::from_raw_parts_mut(upb_Array_MutableDataPtr(dest).cast(), len);
        for (src_ptr, dest_ptr) in src_ptrs.iter().zip(dest_ptrs) {
            *dest_ptr = arena.copy_slice_in(src_ptr.as_ref()).unwrap().into();
        }
    }
}
