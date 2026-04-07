use super::*;

// This struct represents a raw minitable pointer. We need it to be Send and Sync so that we can
// store it in a static OnceLock for lazy initialization of minitables. It should not be used for
// any other purpose.
pub struct MiniTableInitPtr(pub MiniTablePtr);
unsafe impl Send for MiniTableInitPtr {}
unsafe impl Sync for MiniTableInitPtr {}

// Same as above, but for enum minitables.
pub struct MiniTableEnumInitPtr(pub MiniTableEnumPtr);
unsafe impl Send for MiniTableEnumInitPtr {}
unsafe impl Sync for MiniTableEnumInitPtr {}

/// # Safety
/// - `mini_descriptor` must be a valid MiniDescriptor.
pub unsafe fn build_mini_table(mini_descriptor: &'static str) -> MiniTablePtr {
    unsafe {
        MiniTablePtr::new(upb_MiniTable_Build(
            mini_descriptor.as_ptr(),
            mini_descriptor.len(),
            THREAD_LOCAL_ARENA.with(|a| a.raw()),
            std::ptr::null_mut(),
        ))
    }
}

/// # Safety
/// - `mini_descriptor` must be a valid enum MiniDescriptor.
pub unsafe fn build_enum_mini_table(mini_descriptor: &'static str) -> MiniTableEnumPtr {
    unsafe {
        MiniTableEnumPtr::new(upb_MiniTableEnum_Build(
            mini_descriptor.as_ptr(),
            mini_descriptor.len(),
            THREAD_LOCAL_ARENA.with(|a| a.raw()),
            std::ptr::null_mut(),
        ))
    }
}

/// # Safety
/// - All arguments must point to valid MiniTables.
pub unsafe fn link_mini_table(
    mini_table: MiniTablePtr,
    submessages: &[MiniTablePtr],
    subenums: &[MiniTableEnumPtr],
) {
    unsafe {
        assert!(upb_MiniTable_Link(
            mini_table,
            submessages.as_ptr(),
            submessages.len(),
            subenums.as_ptr(),
            subenums.len()
        ));
    }
}
