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

use unittest_proto::proto2_unittest::TestAllTypes;

#[test]
fn serialize_deserialize_message() {
    let mut msg = TestAllTypes::new();
    msg.optional_int64_set(Some(42));
    msg.optional_bool_set(Some(true));
    msg.optional_bytes_set(Some(b"serialize deserialize test"));

    let serialized = msg.serialize();

    let mut msg2 = TestAllTypes::new();
    assert!(msg2.deserialize(&serialized).is_ok());

    assert_eq!(msg.optional_int64(), msg2.optional_int64());
    assert_eq!(msg.optional_bool(), msg2.optional_bool());
    assert_eq!(msg.optional_bytes(), msg2.optional_bytes());
}

#[test]
fn deserialize_empty() {
    let mut msg = TestAllTypes::new();
    assert!(msg.deserialize(&[]).is_ok());
}

#[test]
fn deserialize_error() {
    let mut msg = TestAllTypes::new();
    let data = b"not a serialized proto";
    assert!(msg.deserialize(&*data).is_err());
}
