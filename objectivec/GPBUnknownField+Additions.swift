// Protocol Buffers - Google's data interchange format
// Copyright 2024 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

/// Swift specific additions to simplify usage.
extension GPBUnknownField {

  /// The value of the field in a type-safe manner.
  public enum Value: Equatable {
    case varint(UInt64)
    case fixed32(UInt32)
    case fixed64(UInt64)
    case lengthDelimited(Data)  // length prefixed
    case group(GPBUnknownFields)  // tag delimited
  }

  /// The value of the field in a type-safe manner.
  ///
  /// - Note: This is only valid for non-legacy fields.
  public var value: Value {
    switch type {
    case .varint:
      return .varint(varint)
    case .fixed32:
      return .fixed32(fixed32)
    case .fixed64:
      return .fixed64(fixed64)
    case .lengthDelimited:
      return .lengthDelimited(lengthDelimited)
    case .group:
      return .group(group)
    case .legacy:
      fatalError("`value` not valid for Legacy fields.")
    @unknown default:
      fatalError("Internal error: Unknown field type: \(type)")
    }
  }

}
