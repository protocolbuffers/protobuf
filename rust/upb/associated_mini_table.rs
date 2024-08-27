// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

use super::upb_MiniTable;

/// A trait for types which have a constant associated MiniTable (e.g.
/// generated messages, and their mut and view proxy types).
///
/// upb_Message in C is effectively a DST type, where instances are created with
/// a MiniTable (and have a size dependent on the given MiniTable). Many upb
/// operations on the upb_Message* will require the same upb_MiniTable* to be
/// passed in as a parameter, which is referred to as 'the associated MiniTable
/// for the upb_Message instance' in safety comments.
///
/// This trait is a way to associate a MiniTable with Rust types
/// which hold upb_Message* to simplify ensuring the upb C invariants
/// are maintained.
///
/// Note that this would prefer to be a `const MINI_TABLE: *const upb_MiniTable`
/// to statically associate a single MiniTable, but as long as the MiniTable is
/// an extern "C" we cannot do that without the unstable `const_refs_to_static`.
/// After that feature is stabilized (or if we move the MiniTable generation to
/// .rs) this will be switched.
///
/// SAFETY:
/// - The MiniTable pointer must be from Protobuf code generation and follow the
///   corresponding invariants associated with upb's C API (the pointer should
///   always have the same non-null value, the underlying pointee should never
///   be modified and should have 'static lifetime).
pub unsafe trait AssociatedMiniTable {
    fn mini_table() -> *const upb_MiniTable;
}
