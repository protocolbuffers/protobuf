// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

/// Tests covering proto packages.

#[test]
fn test_packages() {
    // empty package, message declared in the first .proto source
    let _foo: no_package_proto::MsgWithoutPackage;
    // empty package, message declared in other .proto source
    let _foo: no_package_proto::OtherMsgWithoutPackage;
    // empty package, import public of a message
    let _foo: no_package_proto::ImportedMsgWithoutPackage;

    // package, message declared in the first .proto source
    let _foo: package_proto::testing_packages::MsgWithPackage;
    // package, message declared in the other .proto source with the same package
    let _foo: package_proto::testing_packages::OtherMsgWithPackage;
    // package, message declared in the other .proto source with a different package
    let _foo: package_proto::testing_other_packages::OtherMsgInDifferentPackage;
    // package, import public of a message
    let _foo: package_proto::testing_packages::ImportedMsgWithPackage;
}
