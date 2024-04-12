// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
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

package com.google.protobuf;

import static com.google.common.truth.Truth.assertThat;

import com.google.protobuf.PackedFieldTestProto.TestAllTypes;
import com.google.protobuf.PackedFieldTestProto.TestAllTypes.NestedEnum;
import com.google.protobuf.PackedFieldTestProto.TestUnpackedTypes;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/** Tests primitive repeated fields in proto3 are packed in wire format. */
@RunWith(JUnit4.class)
public class PackedFieldTest {
  static final ByteString expectedPackedRawBytes =
      ByteString.copyFrom(
          new byte[] {
            (byte) 0xFA,
            0x01,
            0x01,
            0x01, // repeated int32
            (byte) 0x82,
            0x02,
            0x01,
            0x01, // repeated int64
            (byte) 0x8A,
            0x02,
            0x01,
            0x01, // repeated uint32
            (byte) 0x92,
            0x02,
            0x01,
            0x01, // repeated uint64
            (byte) 0x9A,
            0x02,
            0x01,
            0x02, // repeated sint32
            (byte) 0xA2,
            0x02,
            0x01,
            0x02, // repeated sint64
            (byte) 0xAA,
            0x02,
            0x04,
            0x01,
            0x00,
            0x00,
            0x00, // repeated fixed32
            (byte) 0xB2,
            0x02,
            0x08,
            0x01,
            0x00,
            0x00,
            0x00, // repeated fixed64
            0x00,
            0x00,
            0x00,
            0x00,
            (byte) 0xBA,
            0x02,
            0x04,
            0x01,
            0x00,
            0x00,
            0x00, // repeated sfixed32
            (byte) 0xC2,
            0x02,
            0x08,
            0x01,
            0x00,
            0x00,
            0x00, // repeated sfixed64
            0x00,
            0x00,
            0x00,
            0x00,
            (byte) 0xCA,
            0x02,
            0x04,
            0x00,
            0x00,
            (byte) 0x80,
            0x3f, // repeated float
            (byte) 0xD2,
            0x02,
            0x08,
            0x00,
            0x00,
            0x00,
            0x00, // repeated double
            0x00,
            0x00,
            (byte) 0xf0,
            0x3f,
            (byte) 0xDA,
            0x02,
            0x01,
            0x01, // repeated bool
            (byte) 0x9A,
            0x03,
            0x01,
            0x01 // repeated nested enum
          });

  static final ByteString expectedUnpackedRawBytes =
      ByteString.copyFrom(
          new byte[] {
            0x08,
            0x01, // repeated int32
            0x10,
            0x01, // repeated int64
            0x18,
            0x01, // repeated uint32
            0x20,
            0x01, // repeated uint64
            0x28,
            0x02, // repeated sint32
            0x30,
            0x02, // repeated sint64
            0x3D,
            0x01,
            0x00,
            0x00,
            0x00, // repeated fixed32
            0x41,
            0x01,
            0x00,
            0x00,
            0x00, // repeated fixed64
            0x00,
            0x00,
            0x00,
            0x00,
            0x4D,
            0x01,
            0x00,
            0x00,
            0x00, // repeated sfixed32
            0x51,
            0x01,
            0x00,
            0x00,
            0x00, // repeated sfixed64
            0x00,
            0x00,
            0x00,
            0x00,
            0x5D,
            0x00,
            0x00,
            (byte) 0x80,
            0x3f, // repeated float
            0x61,
            0x00,
            0x00,
            0x00,
            0x00, // repeated double
            0x00,
            0x00,
            (byte) 0xf0,
            0x3f,
            0x68,
            0x01, // repeated bool
            0x70,
            0x01, // repeated nested enum
          });

  @Test
  public void testPackedGeneratedMessage() throws Exception {
    TestAllTypes message = TestAllTypes.parseFrom(expectedPackedRawBytes);
    assertThat(message.toByteString()).isEqualTo(expectedPackedRawBytes);
  }

  @Test
  public void testPackedDynamicMessageSerialize() throws Exception {
    DynamicMessage message =
        DynamicMessage.parseFrom(TestAllTypes.getDescriptor(), expectedPackedRawBytes);
    assertThat(message.toByteString()).isEqualTo(expectedPackedRawBytes);
  }

  @Test
  public void testUnpackedGeneratedMessage() throws Exception {
    TestUnpackedTypes message = TestUnpackedTypes.parseFrom(expectedUnpackedRawBytes);
    assertThat(message.toByteString()).isEqualTo(expectedUnpackedRawBytes);
  }

  @Test
  public void testUnPackedDynamicMessageSerialize() throws Exception {
    DynamicMessage message =
        DynamicMessage.parseFrom(TestUnpackedTypes.getDescriptor(), expectedUnpackedRawBytes);
    assertThat(message.toByteString()).isEqualTo(expectedUnpackedRawBytes);
  }

  // Make sure we haven't screwed up the code generation for packing fields by default.
  @Test
  public void testPackedSerialization() throws Exception {
    TestAllTypes message =
        TestAllTypes.newBuilder()
            .addRepeatedInt32(1234)
            .addRepeatedNestedEnum(NestedEnum.BAR)
            .build();

    CodedInputStream in = CodedInputStream.newInstance(message.toByteArray());

    while (!in.isAtEnd()) {
      int tag = in.readTag();
      assertThat(WireFormat.getTagWireType(tag)).isEqualTo(WireFormat.WIRETYPE_LENGTH_DELIMITED);
      in.skipField(tag);
    }
  }
}
