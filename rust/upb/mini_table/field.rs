// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// Canonical struct and semantics are defined in
// third_party/upb/upb/mini_table/internal/field.h

// We have some dead code that has been ported from C but is not yet used.
#![allow(dead_code)]

use core::mem;

pub enum CType {
    Bool = 1,
    Float = 2,
    Int32 = 3,
    UInt32 = 4,
    ClosedEnum = 5,
    Message = 6,
    Double = 7,
    Int64 = 8,
    UInt64 = 9,
    String = 10,
    Bytes = 11,
}

// Descriptor types, as defined in descriptor.proto.
pub enum FieldType {
    Double = 1,
    Float = 2,
    Int64 = 3,
    UInt64 = 4,
    Int32 = 5,
    Fixed64 = 6,
    Fixed32 = 7,
    Bool = 8,
    String = 9,
    Group = 10,
    Message = 11,
    Bytes = 12,
    UInt32 = 13,
    Enum = 14,
    SFixed32 = 15,
    SFixed64 = 16,
    SInt32 = 17,
    SInt64 = 18,
}

#[repr(C)]
pub struct MiniTableField {
    number: u32,
    offset: u16,
    presence: u16,
    submsg_index: u16,
    descriptortype: u8,
    mode: u8,
}

const NO_SUBMSG_INDEX: u16 = 0xFFFF;
const FIELD_REP_SHIFT: u16 = 6;

enum LabelFlags {
    Packed = 0x4,
    Extension = 0x8,
    Alternate = 0xF,
}

pub(crate) enum FieldMode {
    Map = 0,
    Array = 1,
    Scalar = 2,
}

pub(crate) enum FieldRep {
    Size1 = 0,
    Size4 = 1,
    StringView = 2,
    Size8 = 3,
}

#[cfg(target_pointer_width = "32")]
const NATIVE_POINTER_REP: FieldRep = FieldRep::Size4;

#[cfg(target_pointer_width = "64")]
const NATIVE_POINTER_REP: FieldRep = FieldRep::Size8;

impl MiniTableField {
    pub(crate) fn mode(&self) -> FieldMode {
        unsafe { mem::transmute(self.mode & 3) }
    }

    pub(crate) fn rep(&self) -> FieldRep {
        unsafe { mem::transmute(self.mode >> FIELD_REP_SHIFT) }
    }

    pub fn is_extension(&self) -> bool {
        self.mode & (LabelFlags::Extension as u8) != 0
    }

    pub fn is_packed(&self) -> bool {
        self.mode & (LabelFlags::Packed as u8) != 0
    }

    fn is_alternate(&self) -> bool {
        self.mode & (LabelFlags::Alternate as u8) != 0
    }

    pub fn field_type(&self) -> FieldType {
        let t = self.descriptortype;
        if self.is_alternate() {
            if t == FieldType::Int32 as u8 {
                FieldType::Enum
            } else if t == FieldType::Bytes as u8 {
                FieldType::String
            } else {
                panic!("Unsupported alternate field type: {}", t);
            }
        } else {
            unsafe { mem::transmute::<u8, FieldType>(t) }
        }
    }

    pub fn c_type(&self) -> CType {
        match self.field_type() {
            FieldType::Double => CType::Double,
            FieldType::Float => CType::Float,
            FieldType::Int64 => CType::Int64,
            FieldType::UInt64 => CType::UInt64,
            FieldType::Int32 => CType::Int32,
            FieldType::Fixed64 => CType::UInt64,
            FieldType::Fixed32 => CType::UInt32,
            FieldType::Bool => CType::Bool,
            FieldType::String => CType::String,
            FieldType::Group => CType::Message,
            FieldType::Message => CType::Message,
            FieldType::Bytes => CType::Bytes,
            FieldType::UInt32 => CType::UInt32,
            FieldType::Enum => CType::ClosedEnum,
            FieldType::SFixed32 => CType::Int32,
            FieldType::SFixed64 => CType::Int64,
            FieldType::SInt32 => CType::Int32,
            FieldType::SInt64 => CType::Int64,
        }
    }

    pub(super) fn submsg_index(&self) -> usize {
        debug_assert!(self.submsg_index != NO_SUBMSG_INDEX);
        self.submsg_index as usize
    }
}
