// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

use core::ffi::c_char;

// Transcribed from google3/third_party/upb/upb/base/status.h.
const MAX_MESSAGE_LEN: usize = 511;

#[repr(C)]
#[allow(non_camel_case_types)]
pub struct upb_Status {
    _ok: bool,
    _msg: [c_char; MAX_MESSAGE_LEN],
}

impl upb_Status {
    /// Resets the status to the default cleared state.
    pub const fn cleared() -> Self {
        Self { _ok: true, _msg: [0; MAX_MESSAGE_LEN] }
    }
}

unsafe extern "C" {
    /// Returns a pointer to the NUL-terminated error message stored in `status`.
    ///
    /// # Safety
    /// - `status` must be a valid readable pointer to `upb_Status`.
    pub fn upb_Status_ErrorMessage(status: *const upb_Status) -> *const c_char;
}

#[cfg(test)]
mod tests {
    use super::*;
    use googletest::gtest;

    #[gtest]
    fn assert_status_linked() {
        use crate::assert_linked;
        assert_linked!(upb_Status_ErrorMessage);
    }
}
