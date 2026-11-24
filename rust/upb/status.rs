// Protocol Buffers - Google's data interchange format
// Copyright 2026 Google LLC
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

use super::sys::base::status::{upb_Status, upb_Status_ErrorMessage};
use core::ffi::CStr;

/// Stack-allocated status buffer for receiving error messages from upb C APIs.
pub struct Status {
    raw: upb_Status,
}

impl Status {
    pub fn new() -> Self {
        Self { raw: upb_Status::cleared() }
    }

    pub fn as_mut_ptr(&mut self) -> *mut upb_Status {
        &mut self.raw
    }

    /// Returns the recorded error message, or an empty string if OK.
    pub fn error_message(&self) -> String {
        // SAFETY: `self.raw` is a valid initialized `upb_Status` whose NUL-terminated message
        // buffer lives for the duration of `&self`.
        let c_str = unsafe { CStr::from_ptr(upb_Status_ErrorMessage(&self.raw)) };
        c_str.to_string_lossy().into_owned()
    }
}

impl Default for Status {
    fn default() -> Self {
        Self::new()
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use googletest::prelude::*;

    #[gtest]
    fn test_new_status_has_empty_error_message() {
        let status = Status::new();

        expect_eq!(status.error_message(), "");
    }
}
