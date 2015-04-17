// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// http://github.com/jskeet/dotnet-protobufs/
// Original C++/Java/Python code:
// http://code.google.com/p/protobuf/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

namespace Google.ProtocolBuffers.Descriptors
{
    /// <summary>
    /// Enumeration of all the possible field types. The odd formatting is to make it very clear
    /// which attribute applies to which value, while maintaining a compact format.
    /// </summary>
    public enum FieldType
    {
        [FieldMapping(MappedType.Double, WireFormat.WireType.Fixed64)] Double,
        [FieldMapping(MappedType.Single, WireFormat.WireType.Fixed32)] Float,
        [FieldMapping(MappedType.Int64, WireFormat.WireType.Varint)] Int64,
        [FieldMapping(MappedType.UInt64, WireFormat.WireType.Varint)] UInt64,
        [FieldMapping(MappedType.Int32, WireFormat.WireType.Varint)] Int32,
        [FieldMapping(MappedType.UInt64, WireFormat.WireType.Fixed64)] Fixed64,
        [FieldMapping(MappedType.UInt32, WireFormat.WireType.Fixed32)] Fixed32,
        [FieldMapping(MappedType.Boolean, WireFormat.WireType.Varint)] Bool,
        [FieldMapping(MappedType.String, WireFormat.WireType.LengthDelimited)] String,
        [FieldMapping(MappedType.Message, WireFormat.WireType.StartGroup)] Group,
        [FieldMapping(MappedType.Message, WireFormat.WireType.LengthDelimited)] Message,
        [FieldMapping(MappedType.ByteString, WireFormat.WireType.LengthDelimited)] Bytes,
        [FieldMapping(MappedType.UInt32, WireFormat.WireType.Varint)] UInt32,
        [FieldMapping(MappedType.Int32, WireFormat.WireType.Fixed32)] SFixed32,
        [FieldMapping(MappedType.Int64, WireFormat.WireType.Fixed64)] SFixed64,
        [FieldMapping(MappedType.Int32, WireFormat.WireType.Varint)] SInt32,
        [FieldMapping(MappedType.Int64, WireFormat.WireType.Varint)] SInt64,
        [FieldMapping(MappedType.Enum, WireFormat.WireType.Varint)] Enum
    }
}