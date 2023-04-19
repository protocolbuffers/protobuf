// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

/// Tests covering accessors for singular bool, int64, and bytes fields.
use unittest_proto::proto2_unittest::TestAllTypes;

#[test]
fn test_optional_int64_accessors() {
    let mut msg = TestAllTypes::new();
    assert_eq!(msg.optional_int64(), None);

    msg.optional_int64_set(Some(42));
    assert_eq!(msg.optional_int64(), Some(42));

    msg.optional_int64_set(None);
    assert_eq!(msg.optional_int64(), None);
}

#[test]
fn test_optional_bool_accessors() {
    let mut msg = TestAllTypes::new();
    assert_eq!(msg.optional_bool(), None);

    msg.optional_bool_set(Some(true));
    assert_eq!(msg.optional_bool(), Some(true));

    msg.optional_bool_set(None);
    assert_eq!(msg.optional_bool(), None);
}

#[test]
fn test_optional_bytes_accessors() {
    let mut msg = TestAllTypes::new();
    assert_eq!(msg.optional_bytes(), None);

    msg.optional_bytes_set(Some(b"accessors_test"));
    assert_eq!(msg.optional_bytes().unwrap(), b"accessors_test");

    msg.optional_bytes_set(None);
    assert_eq!(msg.optional_bytes(), None);
}
