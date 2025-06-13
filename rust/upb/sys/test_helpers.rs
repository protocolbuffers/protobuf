#![cfg(test)]

use super::mem::arena::{upb_Arena_Free, upb_Arena_New, RawArena};

#[macro_export]
/// Force a compiler error if the passed in function is not linked.
macro_rules! assert_linked {
    ($($vals:tt)+) => {
        let _ = std::hint::black_box($($vals)+ as *const ());
    }
}

pub struct TestArena {
    raw: RawArena,
}

impl TestArena {
    pub fn new() -> Self {
        TestArena { raw: unsafe { upb_Arena_New() }.unwrap() }
    }
    pub fn raw(&self) -> RawArena {
        self.raw
    }
}

impl std::ops::Drop for TestArena {
    fn drop(&mut self) {
        unsafe { upb_Arena_Free(self.raw) }
    }
}
