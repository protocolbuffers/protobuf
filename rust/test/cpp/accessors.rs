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

/// Tests covering accessors for singular bool and int64 fields.

#[test]
fn test_optional_int64_accessors() {
    let mut msg = unittest_proto::TestAllTypes::new();
    assert_eq!(msg.optional_int64(), 0);
    assert_eq!(msg.has_optional_int64(), false);

    msg.set_optional_int64(42);
    assert_eq!(msg.optional_int64(), 42);
    assert_eq!(msg.has_optional_int64(), true);

    msg.clear_optional_int64();
    assert_eq!(msg.optional_int64(), 0);
    assert_eq!(msg.has_optional_int64(), false);
}

#[test]
fn test_default_int64_accessors() {
    let mut msg = unittest_proto::TestAllTypes::new();
    assert_eq!(msg.default_int64(), 42);
    assert_eq!(msg.has_default_int64(), false);

    msg.set_default_int64(44);
    assert_eq!(msg.default_int64(), 44);
    assert_eq!(msg.has_default_int64(), true);

    msg.clear_default_int64();
    assert_eq!(msg.default_int64(), 42);
    assert_eq!(msg.has_default_int64(), false);
}

#[test]
fn test_optional_bool_accessors() {
    let mut msg = unittest_proto::TestAllTypes::new();
    assert_eq!(msg.optional_bool(), false);
    assert_eq!(msg.has_optional_bool(), false);

    msg.set_optional_bool(true);
    assert_eq!(msg.optional_bool(), true);
    assert_eq!(msg.has_optional_bool(), true);

    msg.clear_optional_bool();
    assert_eq!(msg.optional_bool(), false);
    assert_eq!(msg.has_optional_bool(), false);
}

#[test]
fn test_default_bool_accessors() {
    let mut msg = unittest_proto::TestAllTypes::new();
    assert_eq!(msg.default_bool(), true);
    assert_eq!(msg.has_default_bool(), false);

    msg.set_default_bool(false);
    assert_eq!(msg.default_bool(), false);
    assert_eq!(msg.has_default_bool(), true);

    msg.clear_default_bool();
    assert_eq!(msg.default_bool(), true);
    assert_eq!(msg.has_default_bool(), false);
}

// TODO(b/272728844): Add tests for required primitive fields once we support
// nested messages (TestVerify::Nested::required_int32_64)
