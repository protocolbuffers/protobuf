// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

//! Kernel-agnostic logic for the Rust Protobuf runtime that should not be
//! exposed to through the `protobuf` path but must be public for use by
//! generated code.

// Used by the proto! macro
pub use paste::paste;

pub use crate::r#enum::Enum;
pub use crate::vtable::{
    new_vtable_field_entry, BytesMutVTable, BytesOptionalMutVTable, PrimitiveOptionalMutVTable,
    PrimitiveVTable, PrimitiveWithRawVTable, ProxiedWithRawOptionalVTable, ProxiedWithRawVTable,
    RawVTableMutator, RawVTableOptionalMutatorData,
};
pub use crate::ProtoStr;

// TODO: Temporarily re-export these symbols which are now under
// __runtime under __internal since some external callers using it through
// __internal.
pub use crate::__runtime::{PtrAndLen, RawMap, RawMessage, RawRepeatedField};

/// Used to protect internal-only items from being used accidentally.
pub struct Private;
