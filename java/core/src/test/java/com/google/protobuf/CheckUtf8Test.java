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

import proto2_test_check_utf8.TestCheckUtf8.BytesWrapper;
import proto2_test_check_utf8.TestCheckUtf8.StringWrapper;
import proto2_test_check_utf8_size.TestCheckUtf8Size.BytesWrapperSize;
import proto2_test_check_utf8_size.TestCheckUtf8Size.StringWrapperSize;

import junit.framework.TestCase;

/**
 * Test that protos generated with file option java_string_check_utf8 do in
 * fact perform appropriate UTF-8 checks.
 *
 * @author jbaum@google.com (Jacob Butcher)
 */
public class CheckUtf8Test extends TestCase {

  private static final String UTF8_BYTE_STRING_TEXT = "some text";
  private static final ByteString UTF8_BYTE_STRING =
      ByteString.copyFromUtf8(UTF8_BYTE_STRING_TEXT);
  private static final ByteString NON_UTF8_BYTE_STRING =
      ByteString.copyFrom(new byte[]{(byte) 0x80}); // A lone continuation byte.

  public void testBuildRequiredStringWithGoodUtf8() throws Exception {
    assertEquals(UTF8_BYTE_STRING_TEXT,
                 StringWrapper.newBuilder().setReqBytes(UTF8_BYTE_STRING).getReq());
  }

  public void testParseRequiredStringWithGoodUtf8() throws Exception {
    ByteString serialized =
        BytesWrapper.newBuilder().setReq(UTF8_BYTE_STRING).build().toByteString();
    assertEquals(UTF8_BYTE_STRING_TEXT, StringWrapper.parser().parseFrom(serialized).getReq());
  }

  public void testBuildRequiredStringWithBadUtf8() throws Exception {
    try {
      StringWrapper.newBuilder().setReqBytes(NON_UTF8_BYTE_STRING);
      fail("Expected IllegalArgumentException for non UTF-8 byte string.");
    } catch (IllegalArgumentException exception) {
      assertEquals("Byte string is not UTF-8.", exception.getMessage());
    }
  }

  public void testBuildOptionalStringWithBadUtf8() throws Exception {
    try {
      StringWrapper.newBuilder().setOptBytes(NON_UTF8_BYTE_STRING);
      fail("Expected IllegalArgumentException for non UTF-8 byte string.");
    } catch (IllegalArgumentException exception) {
      assertEquals("Byte string is not UTF-8.", exception.getMessage());
    }
  }

  public void testBuildRepeatedStringWithBadUtf8() throws Exception {
    try {
      StringWrapper.newBuilder().addRepBytes(NON_UTF8_BYTE_STRING);
      fail("Expected IllegalArgumentException for non UTF-8 byte string.");
    } catch (IllegalArgumentException exception) {
      assertEquals("Byte string is not UTF-8.", exception.getMessage());
    }
  }

  public void testParseRequiredStringWithBadUtf8() throws Exception {
    ByteString serialized =
        BytesWrapper.newBuilder().setReq(NON_UTF8_BYTE_STRING).build().toByteString();
    try {
      StringWrapper.parser().parseFrom(serialized);
      fail("Expected InvalidProtocolBufferException for non UTF-8 byte string.");
    } catch (InvalidProtocolBufferException exception) {
      assertEquals("Protocol message had invalid UTF-8.", exception.getMessage());
    }
  }

  public void testBuildRequiredStringWithBadUtf8Size() throws Exception {
    try {
      StringWrapperSize.newBuilder().setReqBytes(NON_UTF8_BYTE_STRING);
      fail("Expected IllegalArgumentException for non UTF-8 byte string.");
    } catch (IllegalArgumentException exception) {
      assertEquals("Byte string is not UTF-8.", exception.getMessage());
    }
  }

  public void testBuildOptionalStringWithBadUtf8Size() throws Exception {
    try {
      StringWrapperSize.newBuilder().setOptBytes(NON_UTF8_BYTE_STRING);
      fail("Expected IllegalArgumentException for non UTF-8 byte string.");
    } catch (IllegalArgumentException exception) {
      assertEquals("Byte string is not UTF-8.", exception.getMessage());
    }
  }

  public void testBuildRepeatedStringWithBadUtf8Size() throws Exception {
    try {
      StringWrapperSize.newBuilder().addRepBytes(NON_UTF8_BYTE_STRING);
      fail("Expected IllegalArgumentException for non UTF-8 byte string.");
    } catch (IllegalArgumentException exception) {
      assertEquals("Byte string is not UTF-8.", exception.getMessage());
    }
  }

  public void testParseRequiredStringWithBadUtf8Size() throws Exception {
    ByteString serialized =
        BytesWrapperSize.newBuilder().setReq(NON_UTF8_BYTE_STRING).build().toByteString();
    try {
      StringWrapperSize.parser().parseFrom(serialized);
      fail("Expected InvalidProtocolBufferException for non UTF-8 byte string.");
    } catch (InvalidProtocolBufferException exception) {
      assertEquals("Protocol message had invalid UTF-8.", exception.getMessage());
    }
  }

}
