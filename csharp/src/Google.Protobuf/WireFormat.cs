#region Copyright notice and license
// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd
#endregion

namespace Google.Protobuf
{
    /// <summary>
    /// This class is used internally by the Protocol Buffer Library and generated
    /// message implementations. It is public only for the sake of those generated
    /// messages. Others should not use this class directly.
    /// <para>
    /// This class contains constants and helper functions useful for dealing with
    /// the Protocol Buffer wire format.
    /// </para>
    /// </summary>
    public static class WireFormat
    {
        /// <summary>
        /// Wire types within protobuf encoding.
        /// </summary>
        public enum WireType : uint
        {
            /// <summary>
            /// Variable-length integer.
            /// </summary>
            Varint = 0,
            /// <summary>
            /// A fixed-length 64-bit value.
            /// </summary>
            Fixed64 = 1,
            /// <summary>
            /// A length-delimited value, i.e. a length followed by that many bytes of data.
            /// </summary>
            LengthDelimited = 2,
            /// <summary>
            /// A "start group" value
            /// </summary>
            StartGroup = 3,
            /// <summary>
            /// An "end group" value
            /// </summary>
            EndGroup = 4,
            /// <summary>
            /// A fixed-length 32-bit value.
            /// </summary>
            Fixed32 = 5
        }

        private const int TagTypeBits = 3;
        private const uint TagTypeMask = (1 << TagTypeBits) - 1;

        /// <summary>
        /// Given a tag value, determines the wire type (lower 3 bits).
        /// </summary>
        public static WireType GetTagWireType(uint tag)
        {
            return (WireType) (tag & TagTypeMask);
        }

        /// <summary>
        /// Given a tag value, determines the field number (the upper 29 bits).
        /// </summary>
        public static int GetTagFieldNumber(uint tag)
        {
            return (int) (tag >> TagTypeBits);
        }

        /// <summary>
        /// Makes a tag value given a field number and wire type.
        /// </summary>
        public static uint MakeTag(int fieldNumber, WireType wireType)
        {
            return (uint) (fieldNumber << TagTypeBits) | (uint) wireType;
        }
    }
}