use crate::opaque_pointee::opaque_pointee;
use crate::{upb_MessageValue, upb_MutableMessageValue, CType, RawArena};
use std::ptr::NonNull;

opaque_pointee!(upb_Array);
pub type RawArray = NonNull<upb_Array>;

extern "C" {
    pub fn upb_Array_New(a: RawArena, r#type: CType) -> RawArray;
    pub fn upb_Array_Size(arr: RawArray) -> usize;
    pub fn upb_Array_Set(arr: RawArray, i: usize, val: upb_MessageValue);
    pub fn upb_Array_Get(arr: RawArray, i: usize) -> upb_MessageValue;
    pub fn upb_Array_Append(arr: RawArray, val: upb_MessageValue, arena: RawArena) -> bool;
    pub fn upb_Array_Resize(arr: RawArray, size: usize, arena: RawArena) -> bool;
    pub fn upb_Array_Reserve(arr: RawArray, size: usize, arena: RawArena) -> bool;
    pub fn upb_Array_MutableDataPtr(arr: RawArray) -> *mut std::ffi::c_void;
    pub fn upb_Array_DataPtr(arr: RawArray) -> *const std::ffi::c_void;
    pub fn upb_Array_GetMutable(arr: RawArray, i: usize) -> upb_MutableMessageValue;
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn array_ffi_test() {
        // SAFETY: FFI unit test uses C API under expected patterns.
        unsafe {
            let arena = crate::Arena::new();
            let raw_arena = arena.raw();
            let array = upb_Array_New(raw_arena, CType::Float);

            assert!(upb_Array_Append(array, upb_MessageValue { float_val: 7.0 }, raw_arena));
            assert!(upb_Array_Append(array, upb_MessageValue { float_val: 42.0 }, raw_arena));
            assert_eq!(upb_Array_Size(array), 2);
            assert!(matches!(upb_Array_Get(array, 1), upb_MessageValue { float_val: 42.0 }));

            assert!(upb_Array_Resize(array, 3, raw_arena));
            assert_eq!(upb_Array_Size(array), 3);
            assert!(matches!(upb_Array_Get(array, 2), upb_MessageValue { float_val: 0.0 }));
        }
    }
}
