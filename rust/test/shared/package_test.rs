// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

//! Tests covering proto packages.

use googletest::prelude::*;

#[gtest]
fn test_message_packages() {
    // empty package, message declared in the first .proto source
    let _: no_package_rust_proto::MsgWithoutPackage;
    // empty package, message declared in other .proto source
    let _: no_package_rust_proto::OtherMsgWithoutPackage;
    // empty package, import public of a message
    let _: no_package_rust_proto::ImportedMsgWithoutPackage;

    // package, message declared in the first .proto source
    let _: package_rust_proto::MsgWithPackage;
    // package, message declared in the other .proto source with the same package
    let _: package_rust_proto::OtherMsgWithPackage;
    // package, message declared in the other .proto source with a different package
    let _: package_rust_proto::OtherMsgInDifferentPackage;
    // package, import public of a message
    let _: package_rust_proto::ImportedMsgWithPackage;
}

#[gtest]
fn test_enum_packages() {
    // empty package, enum declared in the first .proto source
    let _: no_package_rust_proto::EnumWithoutPackage;
    // empty package, enum declared in other .proto source
    let _: no_package_rust_proto::OtherEnumWithoutPackage;
    // empty package, import public of a enum
    let _: no_package_rust_proto::ImportedEnumWithoutPackage;

    // package, enum declared in the first .proto source
    let _: package_rust_proto::EnumWithPackage;
    // package, enum declared in the other .proto source with the same package
    let _: package_rust_proto::OtherEnumWithPackage;
    // package, enum declared in the other .proto source with a different package
    let _: package_rust_proto::OtherEnumInDifferentPackage;
    // package, import public of a enum
    let _: package_rust_proto::ImportedEnumWithPackage;
}
