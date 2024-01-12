// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

//! Tests covering proto packages.

#[test]
fn test_message_packages() {
    // empty package, message declared in the first .proto source
    let _: no_package_proto::MsgWithoutPackage;
    // empty package, message declared in other .proto source
    let _: no_package_proto::OtherMsgWithoutPackage;
    // empty package, import public of a message
    let _: no_package_proto::ImportedMsgWithoutPackage;

    // package, message declared in the first .proto source
    let _: package_proto::testing_packages::MsgWithPackage;
    // package, message declared in the other .proto source with the same package
    let _: package_proto::testing_packages::OtherMsgWithPackage;
    // package, message declared in the other .proto source with a different package
    let _: package_proto::testing_other_packages::OtherMsgInDifferentPackage;
    // package, import public of a message
    let _: package_proto::testing_packages::ImportedMsgWithPackage;
}

#[test]
fn test_enum_packages() {
    // empty package, enum declared in the first .proto source
    let _: no_package_proto::EnumWithoutPackage;
    // empty package, enum declared in other .proto source
    let _: no_package_proto::OtherEnumWithoutPackage;
    // empty package, import public of a enum
    let _: no_package_proto::ImportedEnumWithoutPackage;

    // package, enum declared in the first .proto source
    let _: package_proto::testing_packages::EnumWithPackage;
    // package, enum declared in the other .proto source with the same package
    let _: package_proto::testing_packages::OtherEnumWithPackage;
    // package, enum declared in the other .proto source with a different package
    let _: package_proto::testing_other_packages::OtherEnumInDifferentPackage;
    // package, import public of a enum
    let _: package_proto::testing_packages::ImportedEnumWithPackage;
}
