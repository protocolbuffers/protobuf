// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

/// Tests covering nested messages.

#[test]
fn test_nested_messages_accessible() {
    let _parent: unittest_proto::proto2_unittest::TestAllTypes;
    let _child: unittest_proto::proto2_unittest::TestAllTypes_::NestedMessage;
    unittest_proto::proto2_unittest::TestChildExtensionData_::
    NestedTestAllExtensionsData_::NestedDynamicExtensions::new();
}
