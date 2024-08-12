// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

//! Tests covering codegen of import public statements.

use googletest::prelude::*;

#[gtest]
fn test_import_public_types_are_reexported() {
    let _: import_public_rust_proto::PrimarySrcPubliclyImportedMsg;
    let _: import_public_rust_proto::PrimarySrcPubliclyImportedMsgView;
    let _: import_public_rust_proto::PrimarySrcPubliclyImportedMsgMut;

    let _: import_public_rust_proto::PrimarySrcPubliclyImportedEnum;

    let _: import_public_rust_proto::GrandparentMsg;
    let _: import_public_rust_proto::GrandparentMsgView;
    let _: import_public_rust_proto::GrandparentMsgMut;

    let _: import_public_rust_proto::GrandparentEnum;

    let _: import_public_rust_proto::NonPrimarySrcPubliclyImportedMsg1;
    let _: import_public_rust_proto::NonPrimarySrcPubliclyImportedMsg1View;
    let _: import_public_rust_proto::NonPrimarySrcPubliclyImportedMsg1Mut;

    let _: import_public_rust_proto::NonPrimarySrcPubliclyImportedEnum1;

    let _: import_public_rust_proto::NonPrimarySrcPubliclyImportedMsg2;
    let _: import_public_rust_proto::NonPrimarySrcPubliclyImportedMsg2View;
    let _: import_public_rust_proto::NonPrimarySrcPubliclyImportedMsg2Mut;

    let _: import_public_rust_proto::NonPrimarySrcPubliclyImportedEnum2;
}
