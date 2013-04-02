// Protocol Buffers - Google's data interchange format
// Copyright 2013 Google Inc.  All rights reserved.
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

package com.google.protobuf;

import com.google.protobuf.nano.InternalNano;
import com.google.protobuf.nano.MessageNano;
import com.google.protobuf.nano.NanoOuterClass;
import com.google.protobuf.nano.NanoOuterClass.TestAllTypesNano;
import com.google.protobuf.nano.RecursiveMessageNano;
import com.google.protobuf.nano.SimpleMessageNano;
import com.google.protobuf.nano.UnittestImportNano;
import com.google.protobuf.nano.CodedInputByteBufferNano;

import junit.framework.TestCase;

/**
 * Test nano runtime.
 *
 * @author ulas@google.com Ulas Kirazci
 */
public class NanoTest extends TestCase {
  public void setUp() throws Exception {
  }

  public void testSimpleMessageNano() throws Exception {
    SimpleMessageNano msg = new SimpleMessageNano();
    assertEquals(123, msg.d);
    assertEquals(null, msg.nestedMsg);
    assertEquals(SimpleMessageNano.BAZ, msg.defaultNestedEnum);

    msg.d = 456;
    assertEquals(456, msg.d);

    SimpleMessageNano.NestedMessage nestedMsg = new SimpleMessageNano.NestedMessage();
    nestedMsg.bb = 2;
    assertEquals(2, nestedMsg.bb);
    msg.nestedMsg = nestedMsg;
    assertEquals(2, msg.nestedMsg.bb);

    msg.defaultNestedEnum = SimpleMessageNano.BAR;
    assertEquals(SimpleMessageNano.BAR, msg.defaultNestedEnum);

    byte [] result = MessageNano.toByteArray(msg);
    int msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 9);
    assertEquals(result.length, msgSerializedSize);

    SimpleMessageNano newMsg = SimpleMessageNano.parseFrom(result);
    assertEquals(456, newMsg.d);
    assertEquals(2, msg.nestedMsg.bb);
    assertEquals(SimpleMessageNano.BAR, msg.defaultNestedEnum);
  }

  public void testRecursiveMessageNano() throws Exception {
    RecursiveMessageNano msg = new RecursiveMessageNano();
    assertTrue(msg.repeatedRecursiveMessageNano.length == 0);

    RecursiveMessageNano msg1 = new RecursiveMessageNano();
    msg1.id = 1;
    assertEquals(1, msg1.id);
    RecursiveMessageNano msg2 = new RecursiveMessageNano();
    msg2.id = 2;
    RecursiveMessageNano msg3 = new RecursiveMessageNano();
    msg3.id = 3;

    RecursiveMessageNano.NestedMessage nestedMsg = new RecursiveMessageNano.NestedMessage();
    nestedMsg.a = msg1;
    assertEquals(1, nestedMsg.a.id);

    msg.id = 0;
    msg.nestedMessage = nestedMsg;
    msg.optionalRecursiveMessageNano = msg2;
    msg.repeatedRecursiveMessageNano = new RecursiveMessageNano[] { msg3 };

    byte [] result = MessageNano.toByteArray(msg);
    int msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 16);
    assertEquals(result.length, msgSerializedSize);

    RecursiveMessageNano newMsg = RecursiveMessageNano.parseFrom(result);
    assertEquals(1, newMsg.repeatedRecursiveMessageNano.length);

    assertEquals(0, newMsg.id);
    assertEquals(1, newMsg.nestedMessage.a.id);
    assertEquals(2, newMsg.optionalRecursiveMessageNano.id);
    assertEquals(3, newMsg.repeatedRecursiveMessageNano[0].id);
  }

  public void testNanoRequiredInt32() throws Exception {
    TestAllTypesNano msg = new TestAllTypesNano();
    msg.id = 123;
    assertEquals(123, msg.id);
    msg.clear().id = 456;
    assertEquals(456, msg.id);
    msg.clear();

    msg.id = 123;
    byte [] result = MessageNano.toByteArray(msg);
    int msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 3);
    assertEquals(result.length, msgSerializedSize);

    TestAllTypesNano newMsg = TestAllTypesNano.parseFrom(result);
    assertEquals(123, newMsg.id);
  }

  public void testNanoOptionalInt32() throws Exception {
    TestAllTypesNano msg = new TestAllTypesNano();
    msg.optionalInt32 = 123;
    assertEquals(123, msg.optionalInt32);
    msg.clear()
       .optionalInt32 = 456;
    assertEquals(456, msg.optionalInt32);
    msg.clear();

    msg.optionalInt32 = 123;
    byte [] result = MessageNano.toByteArray(msg);
    int msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 5);
    assertEquals(result.length, msgSerializedSize);

    TestAllTypesNano newMsg = TestAllTypesNano.parseFrom(result);
    assertEquals(123, newMsg.optionalInt32);
  }

  public void testNanoOptionalInt64() throws Exception {
    TestAllTypesNano msg = new TestAllTypesNano();
    msg.optionalInt64 = 123;
    assertEquals(123, msg.optionalInt64);
    msg.clear()
       .optionalInt64 = 456;
    assertEquals(456, msg.optionalInt64);
    msg.clear();
    assertEquals(0, msg.optionalInt64);

    msg.optionalInt64 = 123;
    byte [] result = MessageNano.toByteArray(msg);
    int msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 5);
    assertEquals(result.length, msgSerializedSize);

    TestAllTypesNano newMsg = TestAllTypesNano.parseFrom(result);
    assertEquals(123, newMsg.optionalInt64);
  }

  public void testNanoOptionalUint32() throws Exception {
    TestAllTypesNano msg = new TestAllTypesNano();
    msg.optionalUint32 = 123;
    assertEquals(123, msg.optionalUint32);
    msg.clear()
       .optionalUint32 = 456;
    assertEquals(456, msg.optionalUint32);
    msg.clear();
    assertEquals(0, msg.optionalUint32);

    msg.optionalUint32 = 123;
    byte [] result = MessageNano.toByteArray(msg);
    int msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 5);
    assertEquals(result.length, msgSerializedSize);

    TestAllTypesNano newMsg = TestAllTypesNano.parseFrom(result);
    assertEquals(123, newMsg.optionalUint32);
  }

  public void testNanoOptionalUint64() throws Exception {
    TestAllTypesNano msg = new TestAllTypesNano();
    msg.optionalUint64 = 123;
    assertEquals(123, msg.optionalUint64);
    msg.clear()
       .optionalUint64 = 456;
    assertEquals(456, msg.optionalUint64);
    msg.clear();
    assertEquals(0, msg.optionalUint64);

    msg.optionalUint64 = 123;
    byte [] result = MessageNano.toByteArray(msg);
    int msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 5);
    assertEquals(result.length, msgSerializedSize);

    TestAllTypesNano newMsg = TestAllTypesNano.parseFrom(result);
    assertEquals(123, newMsg.optionalUint64);
  }

  public void testNanoOptionalSint32() throws Exception {
    TestAllTypesNano msg = new TestAllTypesNano();
    msg.optionalSint32 = 123;
    assertEquals(123, msg.optionalSint32);
    msg.clear()
       .optionalSint32 = 456;
    assertEquals(456, msg.optionalSint32);
    msg.clear();
    assertEquals(0, msg.optionalSint32);

    msg.optionalSint32 = -123;
    byte [] result = MessageNano.toByteArray(msg);
    int msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 6);
    assertEquals(result.length, msgSerializedSize);

    TestAllTypesNano newMsg = TestAllTypesNano.parseFrom(result);
    assertEquals(-123, newMsg.optionalSint32);
  }

  public void testNanoOptionalSint64() throws Exception {
    TestAllTypesNano msg = new TestAllTypesNano();
    msg.optionalSint64 = 123;
    assertEquals(123, msg.optionalSint64);
    msg.clear()
       .optionalSint64 = 456;
    assertEquals(456, msg.optionalSint64);
    msg.clear();
    assertEquals(0, msg.optionalSint64);

    msg.optionalSint64 = -123;
    byte [] result = MessageNano.toByteArray(msg);
    int msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 6);
    assertEquals(result.length, msgSerializedSize);

    TestAllTypesNano newMsg = TestAllTypesNano.parseFrom(result);
    assertEquals(-123, newMsg.optionalSint64);
  }

  public void testNanoOptionalFixed32() throws Exception {
    TestAllTypesNano msg = new TestAllTypesNano();
    msg.optionalFixed32 = 123;
    assertEquals(123, msg.optionalFixed32);
    msg.clear()
       .optionalFixed32 = 456;
    assertEquals(456, msg.optionalFixed32);
    msg.clear();
    assertEquals(0, msg.optionalFixed32);

    msg.optionalFixed32 = 123;
    byte [] result = MessageNano.toByteArray(msg);
    int msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 8);
    assertEquals(result.length, msgSerializedSize);

    TestAllTypesNano newMsg = TestAllTypesNano.parseFrom(result);
    assertEquals(123, newMsg.optionalFixed32);
  }

  public void testNanoOptionalFixed64() throws Exception {
    TestAllTypesNano msg = new TestAllTypesNano();
    msg.optionalFixed64 = 123;
    assertEquals(123, msg.optionalFixed64);
    msg.clear()
       .optionalFixed64 = 456;
    assertEquals(456, msg.optionalFixed64);
    msg.clear();
    assertEquals(0, msg.optionalFixed64);

    msg.optionalFixed64 = 123;
    byte [] result = MessageNano.toByteArray(msg);
    int msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 12);
    assertEquals(result.length, msgSerializedSize);

    TestAllTypesNano newMsg = TestAllTypesNano.parseFrom(result);
    assertEquals(123, newMsg.optionalFixed64);
  }

  public void testNanoOptionalSfixed32() throws Exception {
    TestAllTypesNano msg = new TestAllTypesNano();
    msg.optionalSfixed32 = 123;
    assertEquals(123, msg.optionalSfixed32);
    msg.clear()
       .optionalSfixed32 = 456;
    assertEquals(456, msg.optionalSfixed32);
    msg.clear();
    assertEquals(0, msg.optionalSfixed32);

    msg.optionalSfixed32 = 123;
    byte [] result = MessageNano.toByteArray(msg);
    int msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 8);
    assertEquals(result.length, msgSerializedSize);

    TestAllTypesNano newMsg = TestAllTypesNano.parseFrom(result);
    assertEquals(123, newMsg.optionalSfixed32);
  }

  public void testNanoOptionalSfixed64() throws Exception {
    TestAllTypesNano msg = new TestAllTypesNano();
    msg.optionalSfixed64 = 123;
    assertEquals(123, msg.optionalSfixed64);
    msg.clear()
       .optionalSfixed64 = 456;
    assertEquals(456, msg.optionalSfixed64);
    msg.clear();
    assertEquals(0, msg.optionalSfixed64);

    msg.optionalSfixed64 = -123;
    byte [] result = MessageNano.toByteArray(msg);
    int msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 12);
    assertEquals(result.length, msgSerializedSize);

    TestAllTypesNano newMsg = TestAllTypesNano.parseFrom(result);
    assertEquals(-123, newMsg.optionalSfixed64);
  }

  public void testNanoOptionalFloat() throws Exception {
    TestAllTypesNano msg = new TestAllTypesNano();
    msg.optionalFloat = 123f;
    assertTrue(123.0f == msg.optionalFloat);
    msg.clear()
       .optionalFloat = 456.0f;
    assertTrue(456.0f == msg.optionalFloat);
    msg.clear();
    assertTrue(0.0f == msg.optionalFloat);

    msg.optionalFloat = -123.456f;
    byte [] result = MessageNano.toByteArray(msg);
    int msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 8);
    assertEquals(result.length, msgSerializedSize);

    TestAllTypesNano newMsg = TestAllTypesNano.parseFrom(result);
    assertTrue(-123.456f == newMsg.optionalFloat);
  }

  public void testNanoOptionalDouble() throws Exception {
    TestAllTypesNano msg = new TestAllTypesNano();
    msg.optionalDouble = 123;
    assertTrue(123.0 == msg.optionalDouble);
    msg.clear()
       .optionalDouble = 456.0;
    assertTrue(456.0 == msg.optionalDouble);
    msg.clear();
    assertTrue(0.0 == msg.optionalDouble);

    msg.optionalDouble = -123.456;
    byte [] result = MessageNano.toByteArray(msg);
    int msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 12);
    assertEquals(result.length, msgSerializedSize);

    TestAllTypesNano newMsg = TestAllTypesNano.parseFrom(result);
    assertTrue(-123.456 == newMsg.optionalDouble);
  }

  public void testNanoOptionalBool() throws Exception {
    TestAllTypesNano msg = new TestAllTypesNano();
    msg.optionalBool = true;
    assertTrue(msg.optionalBool);
    msg.clear()
       .optionalBool = true;
    assertTrue(msg.optionalBool);
    msg.clear();
    assertFalse(msg.optionalBool);

    msg.optionalBool = true;
    byte [] result = MessageNano.toByteArray(msg);
    int msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 5);
    assertEquals(result.length, msgSerializedSize);

    TestAllTypesNano newMsg = TestAllTypesNano.parseFrom(result);
    assertTrue(newMsg.optionalBool);
  }

  public void testNanoOptionalString() throws Exception {
    TestAllTypesNano msg = new TestAllTypesNano();
    msg.optionalString = "hello";
    assertEquals("hello", msg.optionalString);
    msg.clear();
    assertTrue(msg.optionalString.isEmpty());
    msg.clear()
       .optionalString = "hello2";
    assertEquals("hello2", msg.optionalString);
    msg.clear();
    assertTrue(msg.optionalString.isEmpty());

    msg.optionalString = "bye";
    byte [] result = MessageNano.toByteArray(msg);
    int msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 8);
    assertEquals(result.length, msgSerializedSize);

    TestAllTypesNano newMsg = TestAllTypesNano.parseFrom(result);
    assertTrue(newMsg.optionalString != null);
    assertEquals("bye", newMsg.optionalString);
  }

  public void testNanoOptionalBytes() throws Exception {
    TestAllTypesNano msg = new TestAllTypesNano();
    assertFalse(msg.optionalBytes.length > 0);
    msg.optionalBytes = InternalNano.copyFromUtf8("hello");
    assertTrue(msg.optionalBytes.length > 0);
    assertEquals("hello", new String(msg.optionalBytes, "UTF-8"));
    msg.clear();
    assertFalse(msg.optionalBytes.length > 0);
    msg.clear()
       .optionalBytes = InternalNano.copyFromUtf8("hello");
    assertTrue(msg.optionalBytes.length > 0);
    msg.clear();
    assertFalse(msg.optionalBytes.length > 0);

    msg.optionalBytes = InternalNano.copyFromUtf8("bye");
    byte [] result = MessageNano.toByteArray(msg);
    int msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 8);
    assertEquals(result.length, msgSerializedSize);

    TestAllTypesNano newMsg = TestAllTypesNano.parseFrom(result);
    assertTrue(newMsg.optionalBytes.length > 0);
    assertEquals("bye", new String(newMsg.optionalBytes, "UTF-8"));
  }

  public void testNanoOptionalGroup() throws Exception {
    TestAllTypesNano msg = new TestAllTypesNano();
    TestAllTypesNano.OptionalGroup grp = new TestAllTypesNano.OptionalGroup();
    grp.a = 1;
    assertFalse(msg.optionalGroup != null);
    msg.optionalGroup = grp;
    assertTrue(msg.optionalGroup != null);
    assertEquals(1, msg.optionalGroup.a);
    msg.clear();
    assertFalse(msg.optionalGroup != null);
    msg.clear()
       .optionalGroup = new TestAllTypesNano.OptionalGroup();
    msg.optionalGroup.a = 2;
    assertTrue(msg.optionalGroup != null);
    msg.clear();
    assertFalse(msg.optionalGroup != null);

    msg.optionalGroup = grp;
    byte [] result = MessageNano.toByteArray(msg);
    int msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 10);
    assertEquals(result.length, msgSerializedSize);

    TestAllTypesNano newMsg = TestAllTypesNano.parseFrom(result);
    assertTrue(newMsg.optionalGroup != null);
    assertEquals(1, newMsg.optionalGroup.a);
  }

  public void testNanoOptionalNestedMessage() throws Exception {
    TestAllTypesNano msg = new TestAllTypesNano();
    TestAllTypesNano.NestedMessage nestedMsg = new TestAllTypesNano.NestedMessage();
    nestedMsg.bb = 1;
    assertFalse(msg.optionalNestedMessage != null);
    msg.optionalNestedMessage = nestedMsg;
    assertTrue(msg.optionalNestedMessage != null);
    assertEquals(1, msg.optionalNestedMessage.bb);
    msg.clear();
    assertFalse(msg.optionalNestedMessage != null);
    msg.clear()
       .optionalNestedMessage = new TestAllTypesNano.NestedMessage();
    msg.optionalNestedMessage.bb = 2;
    assertTrue(msg.optionalNestedMessage != null);
    msg.clear();
    assertFalse(msg.optionalNestedMessage != null);

    msg.optionalNestedMessage = nestedMsg;
    byte [] result = MessageNano.toByteArray(msg);
    int msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 8);
    assertEquals(result.length, msgSerializedSize);

    TestAllTypesNano newMsg = TestAllTypesNano.parseFrom(result);
    assertTrue(newMsg.optionalNestedMessage != null);
    assertEquals(1, newMsg.optionalNestedMessage.bb);
  }

  public void testNanoOptionalForeignMessage() throws Exception {
    TestAllTypesNano msg = new TestAllTypesNano();
    NanoOuterClass.ForeignMessageNano nestedMsg = new NanoOuterClass.ForeignMessageNano();
    nestedMsg.c = 1;
    assertFalse(msg.optionalForeignMessage != null);
    msg.optionalForeignMessage = nestedMsg;
    assertTrue(msg.optionalForeignMessage != null);
    assertEquals(1, msg.optionalForeignMessage.c);
    msg.clear();
    assertFalse(msg.optionalForeignMessage != null);
    msg.clear()
       .optionalForeignMessage = new NanoOuterClass.ForeignMessageNano();
    msg.optionalForeignMessage.c = 2;
    assertTrue(msg.optionalForeignMessage != null);
    msg.clear();
    assertFalse(msg.optionalForeignMessage != null);

    msg.optionalForeignMessage = nestedMsg;
    byte [] result = MessageNano.toByteArray(msg);
    int msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 8);
    assertEquals(result.length, msgSerializedSize);

    TestAllTypesNano newMsg = TestAllTypesNano.parseFrom(result);
    assertTrue(newMsg.optionalForeignMessage != null);
    assertEquals(1, newMsg.optionalForeignMessage.c);
  }

  public void testNanoOptionalImportMessage() throws Exception {
    TestAllTypesNano msg = new TestAllTypesNano();
    UnittestImportNano.ImportMessageNano nestedMsg = new UnittestImportNano.ImportMessageNano();
    nestedMsg.d = 1;
    assertFalse(msg.optionalImportMessage != null);
    msg.optionalImportMessage = nestedMsg;
    assertTrue(msg.optionalImportMessage != null);
    assertEquals(1, msg.optionalImportMessage.d);
    msg.clear();
    assertFalse(msg.optionalImportMessage != null);
    msg.clear()
       .optionalImportMessage = new UnittestImportNano.ImportMessageNano();
    msg.optionalImportMessage.d = 2;
    assertTrue(msg.optionalImportMessage != null);
    msg.clear();
    assertFalse(msg.optionalImportMessage != null);

    msg.optionalImportMessage = nestedMsg;
    byte [] result = MessageNano.toByteArray(msg);
    int msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 8);
    assertEquals(result.length, msgSerializedSize);

    TestAllTypesNano newMsg = TestAllTypesNano.parseFrom(result);
    assertTrue(newMsg.optionalImportMessage != null);
    assertEquals(1, newMsg.optionalImportMessage.d);
  }

  public void testNanoOptionalNestedEnum() throws Exception {
    TestAllTypesNano msg = new TestAllTypesNano();
    msg.optionalNestedEnum = TestAllTypesNano.BAR;
    assertEquals(TestAllTypesNano.BAR, msg.optionalNestedEnum);
    msg.clear()
       .optionalNestedEnum = TestAllTypesNano.BAZ;
    assertEquals(TestAllTypesNano.BAZ, msg.optionalNestedEnum);
    msg.clear();
    assertEquals(TestAllTypesNano.FOO, msg.optionalNestedEnum);

    msg.optionalNestedEnum = TestAllTypesNano.BAR;
    byte [] result = MessageNano.toByteArray(msg);
    int msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 6);
    assertEquals(result.length, msgSerializedSize);

    TestAllTypesNano newMsg = TestAllTypesNano.parseFrom(result);
    assertEquals(TestAllTypesNano.BAR, newMsg.optionalNestedEnum);
  }

  public void testNanoOptionalForeignEnum() throws Exception {
    TestAllTypesNano msg = new TestAllTypesNano();
    msg.optionalForeignEnum = NanoOuterClass.FOREIGN_NANO_BAR;
    assertEquals(NanoOuterClass.FOREIGN_NANO_BAR, msg.optionalForeignEnum);
    msg.clear()
       .optionalForeignEnum = NanoOuterClass.FOREIGN_NANO_BAZ;
    assertEquals(NanoOuterClass.FOREIGN_NANO_BAZ, msg.optionalForeignEnum);
    msg.clear();
    assertEquals(NanoOuterClass.FOREIGN_NANO_FOO, msg.optionalForeignEnum);

    msg.optionalForeignEnum = NanoOuterClass.FOREIGN_NANO_BAR;
    byte [] result = MessageNano.toByteArray(msg);
    int msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 6);
    assertEquals(result.length, msgSerializedSize);

    TestAllTypesNano newMsg = TestAllTypesNano.parseFrom(result);
    assertEquals(NanoOuterClass.FOREIGN_NANO_BAR, newMsg.optionalForeignEnum);
  }

  public void testNanoOptionalImportEnum() throws Exception {
    TestAllTypesNano msg = new TestAllTypesNano();
    msg.optionalImportEnum = UnittestImportNano.IMPORT_NANO_BAR;
    assertEquals(UnittestImportNano.IMPORT_NANO_BAR, msg.optionalImportEnum);
    msg.clear()
       .optionalImportEnum = UnittestImportNano.IMPORT_NANO_BAZ;
    assertEquals(UnittestImportNano.IMPORT_NANO_BAZ, msg.optionalImportEnum);
    msg.clear();
    assertEquals(UnittestImportNano.IMPORT_NANO_FOO, msg.optionalImportEnum);

    msg.optionalImportEnum = UnittestImportNano.IMPORT_NANO_BAR;
    byte [] result = MessageNano.toByteArray(msg);
    int msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 6);
    assertEquals(result.length, msgSerializedSize);

    TestAllTypesNano newMsg = TestAllTypesNano.parseFrom(result);
    assertEquals(UnittestImportNano.IMPORT_NANO_BAR, newMsg.optionalImportEnum);
  }

  public void testNanoOptionalStringPiece() throws Exception {
    TestAllTypesNano msg = new TestAllTypesNano();
    msg.optionalStringPiece = "hello";
    assertEquals("hello", msg.optionalStringPiece);
    msg.clear();
    assertTrue(msg.optionalStringPiece.isEmpty());
    msg.clear()
       .optionalStringPiece = "hello2";
    assertEquals("hello2", msg.optionalStringPiece);
    msg.clear();
    assertTrue(msg.optionalStringPiece.isEmpty());

    msg.optionalStringPiece = "bye";
    byte [] result = MessageNano.toByteArray(msg);
    int msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 9);
    assertEquals(result.length, msgSerializedSize);

    TestAllTypesNano newMsg = TestAllTypesNano.parseFrom(result);
    assertTrue(newMsg.optionalStringPiece != null);
    assertEquals("bye", newMsg.optionalStringPiece);
  }

  public void testNanoOptionalCord() throws Exception {
    TestAllTypesNano msg = new TestAllTypesNano();
    msg.optionalCord = "hello";
    assertEquals("hello", msg.optionalCord);
    msg.clear();
    assertTrue(msg.optionalCord.isEmpty());
    msg.clear()
       .optionalCord = "hello2";
    assertEquals("hello2", msg.optionalCord);
    msg.clear();
    assertTrue(msg.optionalCord.isEmpty());

    msg.optionalCord = "bye";
    byte [] result = MessageNano.toByteArray(msg);
    int msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 9);
    assertEquals(result.length, msgSerializedSize);

    TestAllTypesNano newMsg = TestAllTypesNano.parseFrom(result);
    assertTrue(newMsg.optionalCord != null);
    assertEquals("bye", newMsg.optionalCord);
  }

  public void testNanoRepeatedInt32() throws Exception {
    TestAllTypesNano msg = new TestAllTypesNano();
    assertEquals(0, msg.repeatedInt32.length);
    msg.repeatedInt32 = new int[] { 123, 789, 456 };
    assertEquals(789, msg.repeatedInt32[1]);
    assertEquals(456, msg.repeatedInt32[2]);
    msg.clear();
    assertEquals(0, msg.repeatedInt32.length);
    msg.clear()
       .repeatedInt32 = new int[] { 456 };
    assertEquals(1, msg.repeatedInt32.length);
    assertEquals(456, msg.repeatedInt32[0]);
    msg.clear();
    assertEquals(0, msg.repeatedInt32.length);

    // Test 1 entry
    msg.clear()
       .repeatedInt32 = new int[] { 123 };
    assertEquals(1, msg.repeatedInt32.length);
    byte [] result = MessageNano.toByteArray(msg);
    int msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 6);
    assertEquals(result.length, msgSerializedSize);
    TestAllTypesNano newMsg = TestAllTypesNano.parseFrom(result);
    assertEquals(1, newMsg.repeatedInt32.length);
    assertEquals(123, newMsg.repeatedInt32[0]);

    // Test 2 entries
    msg.clear()
       .repeatedInt32 = new int[] { 123, 456 };
    assertEquals(2, msg.repeatedInt32.length);
    result = MessageNano.toByteArray(msg);
    msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 10);
    assertEquals(result.length, msgSerializedSize);

    newMsg = TestAllTypesNano.parseFrom(result);
    assertEquals(2, newMsg.repeatedInt32.length);
    assertEquals(123, newMsg.repeatedInt32[0]);
    assertEquals(456, newMsg.repeatedInt32[1]);
  }

  public void testNanoRepeatedInt64() throws Exception {
    TestAllTypesNano msg = new TestAllTypesNano();
    assertEquals(0, msg.repeatedInt64.length);
    msg.repeatedInt64 = new long[] { 123, 789, 456 };
    assertEquals(789, msg.repeatedInt64[1]);
    assertEquals(456, msg.repeatedInt64[2]);
    msg.clear();
    assertEquals(0, msg.repeatedInt64.length);
    msg.clear()
       .repeatedInt64 = new long[] { 456 };
    assertEquals(1, msg.repeatedInt64.length);
    assertEquals(456, msg.repeatedInt64[0]);
    msg.clear();
    assertEquals(0, msg.repeatedInt64.length);

    // Test 1 entry
    msg.clear()
       .repeatedInt64 = new long[] { 123 };
    assertEquals(1, msg.repeatedInt64.length);
    byte [] result = MessageNano.toByteArray(msg);
    int msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 6);
    assertEquals(result.length, msgSerializedSize);
    TestAllTypesNano newMsg = TestAllTypesNano.parseFrom(result);
    assertEquals(1, newMsg.repeatedInt64.length);
    assertEquals(123, newMsg.repeatedInt64[0]);

    // Test 2 entries
    msg.clear()
       .repeatedInt64 = new long[] { 123, 456 };
    assertEquals(2, msg.repeatedInt64.length);
    result = MessageNano.toByteArray(msg);
    msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 10);
    assertEquals(result.length, msgSerializedSize);

    newMsg = TestAllTypesNano.parseFrom(result);
    assertEquals(2, newMsg.repeatedInt64.length);
    assertEquals(123, newMsg.repeatedInt64[0]);
    assertEquals(456, newMsg.repeatedInt64[1]);
  }

  public void testNanoRepeatedUint32() throws Exception {
    TestAllTypesNano msg = new TestAllTypesNano();
    assertEquals(0, msg.repeatedUint32.length);
    msg.repeatedUint32 = new int[] { 123, 789, 456 };
    assertEquals(789, msg.repeatedUint32[1]);
    assertEquals(456, msg.repeatedUint32[2]);
    msg.clear();
    assertEquals(0, msg.repeatedUint32.length);
    msg.clear()
       .repeatedUint32 = new int[] { 456 };
    assertEquals(1, msg.repeatedUint32.length);
    assertEquals(456, msg.repeatedUint32[0]);
    msg.clear();
    assertEquals(0, msg.repeatedUint32.length);

    // Test 1 entry
    msg.clear()
       .repeatedUint32 = new int[] { 123 };
    assertEquals(1, msg.repeatedUint32.length);
    byte [] result = MessageNano.toByteArray(msg);
    int msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 6);
    assertEquals(result.length, msgSerializedSize);
    TestAllTypesNano newMsg = TestAllTypesNano.parseFrom(result);
    assertEquals(1, newMsg.repeatedUint32.length);
    assertEquals(123, newMsg.repeatedUint32[0]);

    // Test 2 entries
    msg.clear()
       .repeatedUint32 = new int[] { 123, 456 };
    assertEquals(2, msg.repeatedUint32.length);
    result = MessageNano.toByteArray(msg);
    msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 10);
    assertEquals(result.length, msgSerializedSize);

    newMsg = TestAllTypesNano.parseFrom(result);
    assertEquals(2, newMsg.repeatedUint32.length);
    assertEquals(123, newMsg.repeatedUint32[0]);
    assertEquals(456, newMsg.repeatedUint32[1]);
  }

  public void testNanoRepeatedUint64() throws Exception {
    TestAllTypesNano msg = new TestAllTypesNano();
    assertEquals(0, msg.repeatedUint64.length);
    msg.repeatedUint64 = new long[] { 123, 789, 456 };
    assertEquals(789, msg.repeatedUint64[1]);
    assertEquals(456, msg.repeatedUint64[2]);
    msg.clear();
    assertEquals(0, msg.repeatedUint64.length);
    msg.clear()
       .repeatedUint64 = new long[] { 456 };
    assertEquals(1, msg.repeatedUint64.length);
    assertEquals(456, msg.repeatedUint64[0]);
    msg.clear();
    assertEquals(0, msg.repeatedUint64.length);

    // Test 1 entry
    msg.clear()
       .repeatedUint64 = new long[] { 123 };
    assertEquals(1, msg.repeatedUint64.length);
    byte [] result = MessageNano.toByteArray(msg);
    int msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 6);
    assertEquals(result.length, msgSerializedSize);
    TestAllTypesNano newMsg = TestAllTypesNano.parseFrom(result);
    assertEquals(1, newMsg.repeatedUint64.length);
    assertEquals(123, newMsg.repeatedUint64[0]);

    // Test 2 entries
    msg.clear()
       .repeatedUint64 = new long[] { 123, 456 };
    assertEquals(2, msg.repeatedUint64.length);
    result = MessageNano.toByteArray(msg);
    msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 10);
    assertEquals(result.length, msgSerializedSize);

    newMsg = TestAllTypesNano.parseFrom(result);
    assertEquals(2, newMsg.repeatedUint64.length);
    assertEquals(123, newMsg.repeatedUint64[0]);
    assertEquals(456, newMsg.repeatedUint64[1]);
  }

  public void testNanoRepeatedSint32() throws Exception {
    TestAllTypesNano msg = new TestAllTypesNano();
    assertEquals(0, msg.repeatedSint32.length);
    msg.repeatedSint32 = new int[] { 123, 789, 456 };
    assertEquals(789, msg.repeatedSint32[1]);
    assertEquals(456, msg.repeatedSint32[2]);
    msg.clear();
    assertEquals(0, msg.repeatedSint32.length);
    msg.clear()
       .repeatedSint32 = new int[] { 456 };
    assertEquals(1, msg.repeatedSint32.length);
    assertEquals(456, msg.repeatedSint32[0]);
    msg.clear();
    assertEquals(0, msg.repeatedSint32.length);

    // Test 1 entry
    msg.clear()
       .repeatedSint32 = new int[] { 123 };
    assertEquals(1, msg.repeatedSint32.length);
    byte [] result = MessageNano.toByteArray(msg);
    int msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 7);
    assertEquals(result.length, msgSerializedSize);
    TestAllTypesNano newMsg = TestAllTypesNano.parseFrom(result);
    assertEquals(1, newMsg.repeatedSint32.length);
    assertEquals(123, newMsg.repeatedSint32[0]);

    // Test 2 entries
    msg.clear()
       .repeatedSint32 = new int[] { 123, 456 };
    assertEquals(2, msg.repeatedSint32.length);
    result = MessageNano.toByteArray(msg);
    msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 11);
    assertEquals(result.length, msgSerializedSize);

    newMsg = TestAllTypesNano.parseFrom(result);
    assertEquals(2, newMsg.repeatedSint32.length);
    assertEquals(123, newMsg.repeatedSint32[0]);
    assertEquals(456, newMsg.repeatedSint32[1]);
  }

  public void testNanoRepeatedSint64() throws Exception {
    TestAllTypesNano msg = new TestAllTypesNano();
    assertEquals(0, msg.repeatedSint64.length);
    msg.repeatedSint64 = new long[] { 123, 789, 456 };
    assertEquals(789, msg.repeatedSint64[1]);
    assertEquals(456, msg.repeatedSint64[2]);
    msg.clear();
    assertEquals(0, msg.repeatedSint64.length);
    msg.clear()
       .repeatedSint64 = new long[] { 456 };
    assertEquals(1, msg.repeatedSint64.length);
    assertEquals(456, msg.repeatedSint64[0]);
    msg.clear();
    assertEquals(0, msg.repeatedSint64.length);

    // Test 1 entry
    msg.clear()
       .repeatedSint64 = new long[] { 123 };
    assertEquals(1, msg.repeatedSint64.length);
    byte [] result = MessageNano.toByteArray(msg);
    int msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 7);
    assertEquals(result.length, msgSerializedSize);
    TestAllTypesNano newMsg = TestAllTypesNano.parseFrom(result);
    assertEquals(1, newMsg.repeatedSint64.length);
    assertEquals(123, newMsg.repeatedSint64[0]);

    // Test 2 entries
    msg.clear()
       .repeatedSint64 = new long[] { 123, 456 };
    assertEquals(2, msg.repeatedSint64.length);
    result = MessageNano.toByteArray(msg);
    msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 11);
    assertEquals(result.length, msgSerializedSize);

    newMsg = TestAllTypesNano.parseFrom(result);
    assertEquals(2, newMsg.repeatedSint64.length);
    assertEquals(123, newMsg.repeatedSint64[0]);
    assertEquals(456, newMsg.repeatedSint64[1]);
  }

  public void testNanoRepeatedFixed32() throws Exception {
    TestAllTypesNano msg = new TestAllTypesNano();
    assertEquals(0, msg.repeatedFixed32.length);
    msg.repeatedFixed32 = new int[] { 123, 789, 456 };
    assertEquals(789, msg.repeatedFixed32[1]);
    assertEquals(456, msg.repeatedFixed32[2]);
    msg.clear();
    assertEquals(0, msg.repeatedFixed32.length);
    msg.clear()
       .repeatedFixed32 = new int[] { 456 };
    assertEquals(1, msg.repeatedFixed32.length);
    assertEquals(456, msg.repeatedFixed32[0]);
    msg.clear();
    assertEquals(0, msg.repeatedFixed32.length);

    // Test 1 entry
    msg.clear()
       .repeatedFixed32 = new int[] { 123 };
    assertEquals(1, msg.repeatedFixed32.length);
    byte [] result = MessageNano.toByteArray(msg);
    int msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 9);
    assertEquals(result.length, msgSerializedSize);
    TestAllTypesNano newMsg = TestAllTypesNano.parseFrom(result);
    assertEquals(1, newMsg.repeatedFixed32.length);
    assertEquals(123, newMsg.repeatedFixed32[0]);

    // Test 2 entries
    msg.clear()
       .repeatedFixed32 = new int[] { 123, 456 };
    assertEquals(2, msg.repeatedFixed32.length);
    result = MessageNano.toByteArray(msg);
    msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 15);
    assertEquals(result.length, msgSerializedSize);

    newMsg = TestAllTypesNano.parseFrom(result);
    assertEquals(2, newMsg.repeatedFixed32.length);
    assertEquals(123, newMsg.repeatedFixed32[0]);
    assertEquals(456, newMsg.repeatedFixed32[1]);
  }

  public void testNanoRepeatedFixed64() throws Exception {
    TestAllTypesNano msg = new TestAllTypesNano();
    assertEquals(0, msg.repeatedFixed64.length);
    msg.repeatedFixed64 = new long[] { 123, 789, 456 };
    assertEquals(789, msg.repeatedFixed64[1]);
    assertEquals(456, msg.repeatedFixed64[2]);
    msg.clear();
    assertEquals(0, msg.repeatedFixed64.length);
    msg.clear()
       .repeatedFixed64 = new long[] { 456 };
    assertEquals(1, msg.repeatedFixed64.length);
    assertEquals(456, msg.repeatedFixed64[0]);
    msg.clear();
    assertEquals(0, msg.repeatedFixed64.length);

    // Test 1 entry
    msg.clear()
       .repeatedFixed64 = new long[] { 123 };
    assertEquals(1, msg.repeatedFixed64.length);
    byte [] result = MessageNano.toByteArray(msg);
    int msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 13);
    assertEquals(result.length, msgSerializedSize);
    TestAllTypesNano newMsg = TestAllTypesNano.parseFrom(result);
    assertEquals(1, newMsg.repeatedFixed64.length);
    assertEquals(123, newMsg.repeatedFixed64[0]);

    // Test 2 entries
    msg.clear()
       .repeatedFixed64 = new long[] { 123, 456 };
    assertEquals(2, msg.repeatedFixed64.length);
    result = MessageNano.toByteArray(msg);
    msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 23);
    assertEquals(result.length, msgSerializedSize);

    newMsg = TestAllTypesNano.parseFrom(result);
    assertEquals(2, newMsg.repeatedFixed64.length);
    assertEquals(123, newMsg.repeatedFixed64[0]);
    assertEquals(456, newMsg.repeatedFixed64[1]);
  }

  public void testNanoRepeatedSfixed32() throws Exception {
    TestAllTypesNano msg = new TestAllTypesNano();
    assertEquals(0, msg.repeatedSfixed32.length);
    msg.repeatedSfixed32 = new int[] { 123, 789, 456 };
    assertEquals(789, msg.repeatedSfixed32[1]);
    assertEquals(456, msg.repeatedSfixed32[2]);
    msg.clear();
    assertEquals(0, msg.repeatedSfixed32.length);
    msg.clear()
       .repeatedSfixed32 = new int[] { 456 };
    assertEquals(1, msg.repeatedSfixed32.length);
    assertEquals(456, msg.repeatedSfixed32[0]);
    msg.clear();
    assertEquals(0, msg.repeatedSfixed32.length);

    // Test 1 entry
    msg.clear()
       .repeatedSfixed32 = new int[] { 123 };
    assertEquals(1, msg.repeatedSfixed32.length);
    byte [] result = MessageNano.toByteArray(msg);
    int msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 9);
    assertEquals(result.length, msgSerializedSize);
    TestAllTypesNano newMsg = TestAllTypesNano.parseFrom(result);
    assertEquals(1, newMsg.repeatedSfixed32.length);
    assertEquals(123, newMsg.repeatedSfixed32[0]);

    // Test 2 entries
    msg.clear()
       .repeatedSfixed32 = new int[] { 123, 456 };
    assertEquals(2, msg.repeatedSfixed32.length);
    result = MessageNano.toByteArray(msg);
    msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 15);
    assertEquals(result.length, msgSerializedSize);

    newMsg = TestAllTypesNano.parseFrom(result);
    assertEquals(2, newMsg.repeatedSfixed32.length);
    assertEquals(123, newMsg.repeatedSfixed32[0]);
    assertEquals(456, newMsg.repeatedSfixed32[1]);
  }

  public void testNanoRepeatedSfixed64() throws Exception {
    TestAllTypesNano msg = new TestAllTypesNano();
    assertEquals(0, msg.repeatedSfixed64.length);
    msg.repeatedSfixed64 = new long[] { 123, 789, 456 };
    assertEquals(789, msg.repeatedSfixed64[1]);
    assertEquals(456, msg.repeatedSfixed64[2]);
    msg.clear();
    assertEquals(0, msg.repeatedSfixed64.length);
    msg.clear()
       .repeatedSfixed64 = new long[] { 456 };
    assertEquals(1, msg.repeatedSfixed64.length);
    assertEquals(456, msg.repeatedSfixed64[0]);
    msg.clear();
    assertEquals(0, msg.repeatedSfixed64.length);

    // Test 1 entry
    msg.clear()
       .repeatedSfixed64 = new long[] { 123 };
    assertEquals(1, msg.repeatedSfixed64.length);
    byte [] result = MessageNano.toByteArray(msg);
    int msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 13);
    assertEquals(result.length, msgSerializedSize);
    TestAllTypesNano newMsg = TestAllTypesNano.parseFrom(result);
    assertEquals(1, newMsg.repeatedSfixed64.length);
    assertEquals(123, newMsg.repeatedSfixed64[0]);

    // Test 2 entries
    msg.clear()
       .repeatedSfixed64 = new long[] { 123, 456 };
    assertEquals(2, msg.repeatedSfixed64.length);
    result = MessageNano.toByteArray(msg);
    msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 23);
    assertEquals(result.length, msgSerializedSize);

    newMsg = TestAllTypesNano.parseFrom(result);
    assertEquals(2, newMsg.repeatedSfixed64.length);
    assertEquals(123, newMsg.repeatedSfixed64[0]);
    assertEquals(456, newMsg.repeatedSfixed64[1]);
  }

  public void testNanoRepeatedFloat() throws Exception {
    TestAllTypesNano msg = new TestAllTypesNano();
    assertEquals(0, msg.repeatedFloat.length);
    msg.repeatedFloat = new float[] { 123f, 789f, 456f };
    assertEquals(789f, msg.repeatedFloat[1]);
    assertEquals(456f, msg.repeatedFloat[2]);
    msg.clear();
    assertEquals(0, msg.repeatedFloat.length);
    msg.clear()
       .repeatedFloat = new float[] { 456f };
    assertEquals(1, msg.repeatedFloat.length);
    assertEquals(456f, msg.repeatedFloat[0]);
    msg.clear();
    assertEquals(0, msg.repeatedFloat.length);

    // Test 1 entry
    msg.clear()
       .repeatedFloat = new float[] { 123f };
    assertEquals(1, msg.repeatedFloat.length);
    byte [] result = MessageNano.toByteArray(msg);
    int msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 9);
    assertEquals(result.length, msgSerializedSize);
    TestAllTypesNano newMsg = TestAllTypesNano.parseFrom(result);
    assertEquals(1, newMsg.repeatedFloat.length);
    assertEquals(123f, newMsg.repeatedFloat[0]);

    // Test 2 entries
    msg.clear()
       .repeatedFloat = new float[] { 123f, 456f };
    assertEquals(2, msg.repeatedFloat.length);
    result = MessageNano.toByteArray(msg);
    msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 15);
    assertEquals(result.length, msgSerializedSize);

    newMsg = TestAllTypesNano.parseFrom(result);
    assertEquals(2, newMsg.repeatedFloat.length);
    assertEquals(123f, newMsg.repeatedFloat[0]);
    assertEquals(456f, newMsg.repeatedFloat[1]);
  }

  public void testNanoRepeatedDouble() throws Exception {
    TestAllTypesNano msg = new TestAllTypesNano();
    assertEquals(0, msg.repeatedDouble.length);
    msg.repeatedDouble = new double[] { 123.0, 789.0, 456.0 };
    assertEquals(789.0, msg.repeatedDouble[1]);
    assertEquals(456.0, msg.repeatedDouble[2]);
    msg.clear();
    assertEquals(0, msg.repeatedDouble.length);
    msg.clear()
       .repeatedDouble = new double[] { 456.0 };
    assertEquals(1, msg.repeatedDouble.length);
    assertEquals(456.0, msg.repeatedDouble[0]);
    msg.clear();
    assertEquals(0, msg.repeatedDouble.length);

    // Test 1 entry
    msg.clear()
       .repeatedDouble = new double[] { 123.0 };
    assertEquals(1, msg.repeatedDouble.length);
    byte [] result = MessageNano.toByteArray(msg);
    int msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 13);
    assertEquals(result.length, msgSerializedSize);
    TestAllTypesNano newMsg = TestAllTypesNano.parseFrom(result);
    assertEquals(1, newMsg.repeatedDouble.length);
    assertEquals(123.0, newMsg.repeatedDouble[0]);

    // Test 2 entries
    msg.clear()
       .repeatedDouble = new double[] { 123.0, 456.0 };
    assertEquals(2, msg.repeatedDouble.length);
    result = MessageNano.toByteArray(msg);
    msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 23);
    assertEquals(result.length, msgSerializedSize);

    newMsg = TestAllTypesNano.parseFrom(result);
    assertEquals(2, newMsg.repeatedDouble.length);
    assertEquals(123.0, newMsg.repeatedDouble[0]);
    assertEquals(456.0, newMsg.repeatedDouble[1]);
  }

  public void testNanoRepeatedBool() throws Exception {
    TestAllTypesNano msg = new TestAllTypesNano();
    assertEquals(0, msg.repeatedBool.length);
    msg.repeatedBool = new boolean[] { false, true, false };
    assertTrue(msg.repeatedBool[1]);
    assertFalse(msg.repeatedBool[2]);
    msg.clear();
    assertEquals(0, msg.repeatedBool.length);
    msg.clear()
       .repeatedBool = new boolean[] { true };
    assertEquals(1, msg.repeatedBool.length);
    assertTrue(msg.repeatedBool[0]);
    msg.clear();
    assertEquals(0, msg.repeatedBool.length);

    // Test 1 entry
    msg.clear()
       .repeatedBool = new boolean[] { false };
    assertEquals(1, msg.repeatedBool.length);
    byte [] result = MessageNano.toByteArray(msg);
    int msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 6);
    assertEquals(result.length, msgSerializedSize);
    TestAllTypesNano newMsg = TestAllTypesNano.parseFrom(result);
    assertEquals(1, newMsg.repeatedBool.length);
    assertFalse(newMsg.repeatedBool[0]);

    // Test 2 entries
    msg.clear()
       .repeatedBool = new boolean[] { true, false };
    assertEquals(2, msg.repeatedBool.length);
    result = MessageNano.toByteArray(msg);
    msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 9);
    assertEquals(result.length, msgSerializedSize);

    newMsg = TestAllTypesNano.parseFrom(result);
    assertEquals(2, newMsg.repeatedBool.length);
    assertTrue(newMsg.repeatedBool[0]);
    assertFalse(newMsg.repeatedBool[1]);
  }

  public void testNanoRepeatedString() throws Exception {
    TestAllTypesNano msg = new TestAllTypesNano();
    assertEquals(0, msg.repeatedString.length);
    msg.repeatedString = new String[] { "hello", "bye", "boo" };
    assertEquals("bye", msg.repeatedString[1]);
    assertEquals("boo", msg.repeatedString[2]);
    msg.clear();
    assertEquals(0, msg.repeatedString.length);
    msg.clear()
       .repeatedString = new String[] { "boo" };
    assertEquals(1, msg.repeatedString.length);
    assertEquals("boo", msg.repeatedString[0]);
    msg.clear();
    assertEquals(0, msg.repeatedString.length);

    // Test 1 entry
    msg.clear()
       .repeatedString = new String[] { "" };
    assertEquals(1, msg.repeatedString.length);
    byte [] result = MessageNano.toByteArray(msg);
    int msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 6);
    assertEquals(result.length, msgSerializedSize);
    TestAllTypesNano newMsg = TestAllTypesNano.parseFrom(result);
    assertEquals(1, newMsg.repeatedString.length);
    assertTrue(newMsg.repeatedString[0].isEmpty());

    // Test 2 entries
    msg.clear()
       .repeatedString = new String[] { "hello", "world" };
    assertEquals(2, msg.repeatedString.length);
    result = MessageNano.toByteArray(msg);
    msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 19);
    assertEquals(result.length, msgSerializedSize);

    newMsg = TestAllTypesNano.parseFrom(result);
    assertEquals(2, newMsg.repeatedString.length);
    assertEquals("hello", newMsg.repeatedString[0]);
    assertEquals("world", newMsg.repeatedString[1]);
  }

  public void testNanoRepeatedBytes() throws Exception {
    TestAllTypesNano msg = new TestAllTypesNano();
    assertEquals(0, msg.repeatedBytes.length);
    msg.repeatedBytes = new byte[][] {
        InternalNano.copyFromUtf8("hello"),
        InternalNano.copyFromUtf8("bye"),
        InternalNano.copyFromUtf8("boo")
    };
    assertEquals("bye", new String(msg.repeatedBytes[1], "UTF-8"));
    assertEquals("boo", new String(msg.repeatedBytes[2], "UTF-8"));
    msg.clear();
    assertEquals(0, msg.repeatedBytes.length);
    msg.clear()
       .repeatedBytes = new byte[][] { InternalNano.copyFromUtf8("boo") };
    assertEquals(1, msg.repeatedBytes.length);
    assertEquals("boo", new String(msg.repeatedBytes[0], "UTF-8"));
    msg.clear();
    assertEquals(0, msg.repeatedBytes.length);

    // Test 1 entry
    msg.clear()
       .repeatedBytes = new byte[][] { InternalNano.copyFromUtf8("") };
    assertEquals(1, msg.repeatedBytes.length);
    byte [] result = MessageNano.toByteArray(msg);
    int msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 6);
    assertEquals(result.length, msgSerializedSize);
    TestAllTypesNano newMsg = TestAllTypesNano.parseFrom(result);
    assertEquals(1, newMsg.repeatedBytes.length);
    assertTrue(newMsg.repeatedBytes[0].length == 0);

    // Test 2 entries
    msg.clear()
       .repeatedBytes = new byte[][] {
      InternalNano.copyFromUtf8("hello"),
      InternalNano.copyFromUtf8("world")
    };
    assertEquals(2, msg.repeatedBytes.length);
    result = MessageNano.toByteArray(msg);
    msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 19);
    assertEquals(result.length, msgSerializedSize);

    newMsg = TestAllTypesNano.parseFrom(result);
    assertEquals(2, newMsg.repeatedBytes.length);
    assertEquals("hello", new String(newMsg.repeatedBytes[0], "UTF-8"));
    assertEquals("world", new String(newMsg.repeatedBytes[1], "UTF-8"));
  }

  public void testNanoRepeatedGroup() throws Exception {
    TestAllTypesNano msg = new TestAllTypesNano();
    TestAllTypesNano.RepeatedGroup group0 =
      new TestAllTypesNano.RepeatedGroup();
    group0.a = 0;
    TestAllTypesNano.RepeatedGroup group1 =
      new TestAllTypesNano.RepeatedGroup();
    group1.a = 1;
    TestAllTypesNano.RepeatedGroup group2 =
      new TestAllTypesNano.RepeatedGroup();
    group2.a = 2;

    msg.repeatedGroup = new TestAllTypesNano.RepeatedGroup[] { group0, group1, group2 };
    assertEquals(3, msg.repeatedGroup.length);
    assertEquals(0, msg.repeatedGroup[0].a);
    assertEquals(1, msg.repeatedGroup[1].a);
    assertEquals(2, msg.repeatedGroup[2].a);
    msg.clear();
    assertEquals(0, msg.repeatedGroup.length);
    msg.clear()
       .repeatedGroup = new TestAllTypesNano.RepeatedGroup[] { group1 };
    assertEquals(1, msg.repeatedGroup.length);
    assertEquals(1, msg.repeatedGroup[0].a);
    msg.clear();
    assertEquals(0, msg.repeatedGroup.length);

    // Test 1 entry
    msg.clear()
       .repeatedGroup = new TestAllTypesNano.RepeatedGroup[] { group0 };
    assertEquals(1, msg.repeatedGroup.length);
    byte [] result = MessageNano.toByteArray(msg);
    int msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 7);
    assertEquals(result.length, msgSerializedSize);
    TestAllTypesNano newMsg = TestAllTypesNano.parseFrom(result);
    assertEquals(1, newMsg.repeatedGroup.length);
    assertEquals(0, newMsg.repeatedGroup[0].a);

    // Test 2 entries
    msg.clear()
       .repeatedGroup = new TestAllTypesNano.RepeatedGroup[] { group0, group1 };
    assertEquals(2, msg.repeatedGroup.length);
    result = MessageNano.toByteArray(msg);
    msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 14);
    assertEquals(result.length, msgSerializedSize);

    newMsg = TestAllTypesNano.parseFrom(result);
    assertEquals(2, newMsg.repeatedGroup.length);
    assertEquals(0, newMsg.repeatedGroup[0].a);
    assertEquals(1, newMsg.repeatedGroup[1].a);
  }

  public void testNanoRepeatedNestedMessage() throws Exception {
    TestAllTypesNano msg = new TestAllTypesNano();
    TestAllTypesNano.NestedMessage nestedMsg0 =
      new TestAllTypesNano.NestedMessage();
    nestedMsg0.bb = 0;
    TestAllTypesNano.NestedMessage nestedMsg1 =
      new TestAllTypesNano.NestedMessage();
    nestedMsg1.bb = 1;
    TestAllTypesNano.NestedMessage nestedMsg2 =
      new TestAllTypesNano.NestedMessage();
    nestedMsg2.bb = 2;

    msg.repeatedNestedMessage =
        new TestAllTypesNano.NestedMessage[] { nestedMsg0, nestedMsg1, nestedMsg2 };
    assertEquals(3, msg.repeatedNestedMessage.length);
    assertEquals(0, msg.repeatedNestedMessage[0].bb);
    assertEquals(1, msg.repeatedNestedMessage[1].bb);
    assertEquals(2, msg.repeatedNestedMessage[2].bb);
    msg.clear();
    assertEquals(0, msg.repeatedNestedMessage.length);
    msg.clear()
       .repeatedNestedMessage = new TestAllTypesNano.NestedMessage[] { nestedMsg1 };
    assertEquals(1, msg.repeatedNestedMessage.length);
    assertEquals(1, msg.repeatedNestedMessage[0].bb);
    msg.clear();
    assertEquals(0, msg.repeatedNestedMessage.length);

    // Test 1 entry
    msg.clear()
       .repeatedNestedMessage = new TestAllTypesNano.NestedMessage[] { nestedMsg0 };
    assertEquals(1, msg.repeatedNestedMessage.length);
    byte [] result = MessageNano.toByteArray(msg);
    int msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 6);
    assertEquals(result.length, msgSerializedSize);
    TestAllTypesNano newMsg = TestAllTypesNano.parseFrom(result);
    assertEquals(1, newMsg.repeatedNestedMessage.length);
    assertEquals(0, newMsg.repeatedNestedMessage[0].bb);

    // Test 2 entries
    msg.clear()
       .repeatedNestedMessage = new TestAllTypesNano.NestedMessage[] { nestedMsg0, nestedMsg1 };
    assertEquals(2, msg.repeatedNestedMessage.length);
    result = MessageNano.toByteArray(msg);
    msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 11);
    assertEquals(result.length, msgSerializedSize);

    newMsg = TestAllTypesNano.parseFrom(result);
    assertEquals(2, newMsg.repeatedNestedMessage.length);
    assertEquals(0, newMsg.repeatedNestedMessage[0].bb);
    assertEquals(1, newMsg.repeatedNestedMessage[1].bb);
  }

  public void testNanoRepeatedForeignMessage() throws Exception {
    TestAllTypesNano msg = new TestAllTypesNano();
    NanoOuterClass.ForeignMessageNano foreignMsg0 =
      new NanoOuterClass.ForeignMessageNano();
    foreignMsg0.c = 0;
    NanoOuterClass.ForeignMessageNano foreignMsg1 =
      new NanoOuterClass.ForeignMessageNano();
    foreignMsg1.c = 1;
    NanoOuterClass.ForeignMessageNano foreignMsg2 =
      new NanoOuterClass.ForeignMessageNano();
    foreignMsg2.c = 2;

    msg.repeatedForeignMessage =
        new NanoOuterClass.ForeignMessageNano[] { foreignMsg0, foreignMsg1, foreignMsg2 };
    assertEquals(3, msg.repeatedForeignMessage.length);
    assertEquals(0, msg.repeatedForeignMessage[0].c);
    assertEquals(1, msg.repeatedForeignMessage[1].c);
    assertEquals(2, msg.repeatedForeignMessage[2].c);
    msg.clear();
    assertEquals(0, msg.repeatedForeignMessage.length);
    msg.clear()
       .repeatedForeignMessage = new NanoOuterClass.ForeignMessageNano[] { foreignMsg1 };
    assertEquals(1, msg.repeatedForeignMessage.length);
    assertEquals(1, msg.repeatedForeignMessage[0].c);
    msg.clear();
    assertEquals(0, msg.repeatedForeignMessage.length);

    // Test 1 entry
    msg.clear()
       .repeatedForeignMessage = new NanoOuterClass.ForeignMessageNano[] { foreignMsg0 };
    assertEquals(1, msg.repeatedForeignMessage.length);
    byte [] result = MessageNano.toByteArray(msg);
    int msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 6);
    assertEquals(result.length, msgSerializedSize);
    TestAllTypesNano newMsg = TestAllTypesNano.parseFrom(result);
    assertEquals(1, newMsg.repeatedForeignMessage.length);
    assertEquals(0, newMsg.repeatedForeignMessage[0].c);

    // Test 2 entries
    msg.clear()
       .repeatedForeignMessage = new NanoOuterClass.ForeignMessageNano[] { foreignMsg0, foreignMsg1 };
    assertEquals(2, msg.repeatedForeignMessage.length);
    result = MessageNano.toByteArray(msg);
    msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 11);
    assertEquals(result.length, msgSerializedSize);

    newMsg = TestAllTypesNano.parseFrom(result);
    assertEquals(2, newMsg.repeatedForeignMessage.length);
    assertEquals(0, newMsg.repeatedForeignMessage[0].c);
    assertEquals(1, newMsg.repeatedForeignMessage[1].c);
  }

  public void testNanoRepeatedImportMessage() throws Exception {
    TestAllTypesNano msg = new TestAllTypesNano();
    UnittestImportNano.ImportMessageNano foreignMsg0 =
      new UnittestImportNano.ImportMessageNano();
    foreignMsg0.d = 0;
    UnittestImportNano.ImportMessageNano foreignMsg1 =
      new UnittestImportNano.ImportMessageNano();
    foreignMsg1.d = 1;
    UnittestImportNano.ImportMessageNano foreignMsg2 =
      new UnittestImportNano.ImportMessageNano();
    foreignMsg2.d = 2;

    msg.repeatedImportMessage =
        new UnittestImportNano.ImportMessageNano[] { foreignMsg0, foreignMsg1, foreignMsg2 };
    assertEquals(3, msg.repeatedImportMessage.length);
    assertEquals(0, msg.repeatedImportMessage[0].d);
    assertEquals(1, msg.repeatedImportMessage[1].d);
    assertEquals(2, msg.repeatedImportMessage[2].d);
    msg.clear();
    assertEquals(0, msg.repeatedImportMessage.length);
    msg.clear()
       .repeatedImportMessage = new UnittestImportNano.ImportMessageNano[] { foreignMsg1 };
    assertEquals(1, msg.repeatedImportMessage.length);
    assertEquals(1, msg.repeatedImportMessage[0].d);
    msg.clear();
    assertEquals(0, msg.repeatedImportMessage.length);

    // Test 1 entry
    msg.clear()
       .repeatedImportMessage = new UnittestImportNano.ImportMessageNano[] { foreignMsg0 };
    assertEquals(1, msg.repeatedImportMessage.length);
    byte [] result = MessageNano.toByteArray(msg);
    int msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 6);
    assertEquals(result.length, msgSerializedSize);
    TestAllTypesNano newMsg = TestAllTypesNano.parseFrom(result);
    assertEquals(1, newMsg.repeatedImportMessage.length);
    assertEquals(0, newMsg.repeatedImportMessage[0].d);

    // Test 2 entries
    msg.clear()
       .repeatedImportMessage = new UnittestImportNano.ImportMessageNano[] { foreignMsg0, foreignMsg1 };
    assertEquals(2, msg.repeatedImportMessage.length);
    result = MessageNano.toByteArray(msg);
    msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 11);
    assertEquals(result.length, msgSerializedSize);

    newMsg = TestAllTypesNano.parseFrom(result);
    assertEquals(2, newMsg.repeatedImportMessage.length);
    assertEquals(0, newMsg.repeatedImportMessage[0].d);
    assertEquals(1, newMsg.repeatedImportMessage[1].d);
  }

  public void testNanoRepeatedNestedEnum() throws Exception {
    TestAllTypesNano msg = new TestAllTypesNano();
    msg.repeatedNestedEnum = new int[] {
        TestAllTypesNano.FOO,
        TestAllTypesNano.BAR,
        TestAllTypesNano.BAZ
    };
    assertEquals(3, msg.repeatedNestedEnum.length);
    assertEquals(TestAllTypesNano.FOO, msg.repeatedNestedEnum[0]);
    assertEquals(TestAllTypesNano.BAR, msg.repeatedNestedEnum[1]);
    assertEquals(TestAllTypesNano.BAZ, msg.repeatedNestedEnum[2]);
    msg.clear();
    assertEquals(0, msg.repeatedNestedEnum.length);
    msg.clear()
       .repeatedNestedEnum = new int[] { TestAllTypesNano.BAR };
    assertEquals(1, msg.repeatedNestedEnum.length);
    assertEquals(TestAllTypesNano.BAR, msg.repeatedNestedEnum[0]);
    msg.clear();
    assertEquals(0, msg.repeatedNestedEnum.length);

    // Test 1 entry
    msg.clear()
       .repeatedNestedEnum = new int[] { TestAllTypesNano.FOO };
    byte [] result = MessageNano.toByteArray(msg);
    int msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 6);
    assertEquals(result.length, msgSerializedSize);
    TestAllTypesNano newMsg = TestAllTypesNano.parseFrom(result);
    assertEquals(1, newMsg.repeatedNestedEnum.length);
    assertEquals(TestAllTypesNano.FOO, msg.repeatedNestedEnum[0]);

    // Test 2 entries
    msg.clear()
       .repeatedNestedEnum = new int[] { TestAllTypesNano.FOO, TestAllTypesNano.BAR };
    assertEquals(2, msg.repeatedNestedEnum.length);
    result = MessageNano.toByteArray(msg);
    msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 9);
    assertEquals(result.length, msgSerializedSize);

    newMsg = TestAllTypesNano.parseFrom(result);
    assertEquals(2, newMsg.repeatedNestedEnum.length);
    assertEquals(TestAllTypesNano.FOO, msg.repeatedNestedEnum[0]);
    assertEquals(TestAllTypesNano.BAR, msg.repeatedNestedEnum[1]);
  }

  public void testNanoRepeatedForeignEnum() throws Exception {
    TestAllTypesNano msg = new TestAllTypesNano();
    msg.repeatedForeignEnum = new int[] {
        NanoOuterClass.FOREIGN_NANO_FOO,
        NanoOuterClass.FOREIGN_NANO_BAR,
        NanoOuterClass.FOREIGN_NANO_BAZ
    };
    assertEquals(3, msg.repeatedForeignEnum.length);
    assertEquals(NanoOuterClass.FOREIGN_NANO_FOO, msg.repeatedForeignEnum[0]);
    assertEquals(NanoOuterClass.FOREIGN_NANO_BAR, msg.repeatedForeignEnum[1]);
    assertEquals(NanoOuterClass.FOREIGN_NANO_BAZ, msg.repeatedForeignEnum[2]);
    msg.clear();
    assertEquals(0, msg.repeatedForeignEnum.length);
    msg.clear()
       .repeatedForeignEnum = new int[] { NanoOuterClass.FOREIGN_NANO_BAR };
    assertEquals(1, msg.repeatedForeignEnum.length);
    assertEquals(NanoOuterClass.FOREIGN_NANO_BAR, msg.repeatedForeignEnum[0]);
    msg.clear();
    assertEquals(0, msg.repeatedForeignEnum.length);

    // Test 1 entry
    msg.clear()
       .repeatedForeignEnum = new int[] { NanoOuterClass.FOREIGN_NANO_FOO };
    byte [] result = MessageNano.toByteArray(msg);
    int msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 6);
    assertEquals(result.length, msgSerializedSize);
    TestAllTypesNano newMsg = TestAllTypesNano.parseFrom(result);
    assertEquals(1, newMsg.repeatedForeignEnum.length);
    assertEquals(NanoOuterClass.FOREIGN_NANO_FOO, msg.repeatedForeignEnum[0]);

    // Test 2 entries
    msg.clear()
       .repeatedForeignEnum = new int[] {
      NanoOuterClass.FOREIGN_NANO_FOO,
      NanoOuterClass.FOREIGN_NANO_BAR
    };
    assertEquals(2, msg.repeatedForeignEnum.length);
    result = MessageNano.toByteArray(msg);
    msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 9);
    assertEquals(result.length, msgSerializedSize);

    newMsg = TestAllTypesNano.parseFrom(result);
    assertEquals(2, newMsg.repeatedForeignEnum.length);
    assertEquals(NanoOuterClass.FOREIGN_NANO_FOO, msg.repeatedForeignEnum[0]);
    assertEquals(NanoOuterClass.FOREIGN_NANO_BAR, msg.repeatedForeignEnum[1]);
  }

  public void testNanoRepeatedImportEnum() throws Exception {
    TestAllTypesNano msg = new TestAllTypesNano();
    msg.repeatedImportEnum = new int[] {
        UnittestImportNano.IMPORT_NANO_FOO,
        UnittestImportNano.IMPORT_NANO_BAR,
        UnittestImportNano.IMPORT_NANO_BAZ
    };
    assertEquals(3, msg.repeatedImportEnum.length);
    assertEquals(UnittestImportNano.IMPORT_NANO_FOO, msg.repeatedImportEnum[0]);
    assertEquals(UnittestImportNano.IMPORT_NANO_BAR, msg.repeatedImportEnum[1]);
    assertEquals(UnittestImportNano.IMPORT_NANO_BAZ, msg.repeatedImportEnum[2]);
    msg.clear();
    assertEquals(0, msg.repeatedImportEnum.length);
    msg.clear()
       .repeatedImportEnum = new int[] { UnittestImportNano.IMPORT_NANO_BAR };
    assertEquals(1, msg.repeatedImportEnum.length);
    assertEquals(UnittestImportNano.IMPORT_NANO_BAR, msg.repeatedImportEnum[0]);
    msg.clear();
    assertEquals(0, msg.repeatedImportEnum.length);

    // Test 1 entry
    msg.clear()
       .repeatedImportEnum = new int[] { UnittestImportNano.IMPORT_NANO_FOO };
    byte [] result = MessageNano.toByteArray(msg);
    int msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 6);
    assertEquals(result.length, msgSerializedSize);
    TestAllTypesNano newMsg = TestAllTypesNano.parseFrom(result);
    assertEquals(1, newMsg.repeatedImportEnum.length);
    assertEquals(UnittestImportNano.IMPORT_NANO_FOO, msg.repeatedImportEnum[0]);

    // Test 2 entries
    msg.clear()
       .repeatedImportEnum = new int[] {
      UnittestImportNano.IMPORT_NANO_FOO,
      UnittestImportNano.IMPORT_NANO_BAR
    };
    assertEquals(2, msg.repeatedImportEnum.length);
    result = MessageNano.toByteArray(msg);
    msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 9);
    assertEquals(result.length, msgSerializedSize);

    newMsg = TestAllTypesNano.parseFrom(result);
    assertEquals(2, newMsg.repeatedImportEnum.length);
    assertEquals(UnittestImportNano.IMPORT_NANO_FOO, msg.repeatedImportEnum[0]);
    assertEquals(UnittestImportNano.IMPORT_NANO_BAR, msg.repeatedImportEnum[1]);
  }

  public void testNanoRepeatedStringPiece() throws Exception {
    TestAllTypesNano msg = new TestAllTypesNano();
    assertEquals(0, msg.repeatedStringPiece.length);
    msg.repeatedStringPiece = new String[] { "hello", "bye", "boo" };
    assertEquals("bye", msg.repeatedStringPiece[1]);
    assertEquals("boo", msg.repeatedStringPiece[2]);
    msg.clear();
    assertEquals(0, msg.repeatedStringPiece.length);
    msg.clear()
       .repeatedStringPiece = new String[] { "boo" };
    assertEquals(1, msg.repeatedStringPiece.length);
    assertEquals("boo", msg.repeatedStringPiece[0]);
    msg.clear();
    assertEquals(0, msg.repeatedStringPiece.length);

    // Test 1 entry
    msg.clear()
       .repeatedStringPiece = new String[] { "" };
    assertEquals(1, msg.repeatedStringPiece.length);
    byte [] result = MessageNano.toByteArray(msg);
    int msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 6);
    assertEquals(result.length, msgSerializedSize);
    TestAllTypesNano newMsg = TestAllTypesNano.parseFrom(result);
    assertEquals(1, newMsg.repeatedStringPiece.length);
    assertTrue(newMsg.repeatedStringPiece[0].isEmpty());

    // Test 2 entries
    msg.clear()
       .repeatedStringPiece = new String[] { "hello", "world" };
    assertEquals(2, msg.repeatedStringPiece.length);
    result = MessageNano.toByteArray(msg);
    msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 19);
    assertEquals(result.length, msgSerializedSize);

    newMsg = TestAllTypesNano.parseFrom(result);
    assertEquals(2, newMsg.repeatedStringPiece.length);
    assertEquals("hello", newMsg.repeatedStringPiece[0]);
    assertEquals("world", newMsg.repeatedStringPiece[1]);
  }

  public void testNanoRepeatedCord() throws Exception {
    TestAllTypesNano msg = new TestAllTypesNano();
    assertEquals(0, msg.repeatedCord.length);
    msg.repeatedCord = new String[] { "hello", "bye", "boo" };
    assertEquals("bye", msg.repeatedCord[1]);
    assertEquals("boo", msg.repeatedCord[2]);
    msg.clear();
    assertEquals(0, msg.repeatedCord.length);
    msg.clear()
       .repeatedCord = new String[] { "boo" };
    assertEquals(1, msg.repeatedCord.length);
    assertEquals("boo", msg.repeatedCord[0]);
    msg.clear();
    assertEquals(0, msg.repeatedCord.length);

    // Test 1 entry
    msg.clear()
       .repeatedCord = new String[] { "" };
    assertEquals(1, msg.repeatedCord.length);
    byte [] result = MessageNano.toByteArray(msg);
    int msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 6);
    assertEquals(result.length, msgSerializedSize);
    TestAllTypesNano newMsg = TestAllTypesNano.parseFrom(result);
    assertEquals(1, newMsg.repeatedCord.length);
    assertTrue(newMsg.repeatedCord[0].isEmpty());

    // Test 2 entries
    msg.clear()
       .repeatedCord = new String[] { "hello", "world" };
    assertEquals(2, msg.repeatedCord.length);
    result = MessageNano.toByteArray(msg);
    msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 19);
    assertEquals(result.length, msgSerializedSize);

    newMsg = TestAllTypesNano.parseFrom(result);
    assertEquals(2, newMsg.repeatedCord.length);
    assertEquals("hello", newMsg.repeatedCord[0]);
    assertEquals("world", newMsg.repeatedCord[1]);
  }

  public void testNanoRepeatedPackedInt32() throws Exception {
    TestAllTypesNano msg = new TestAllTypesNano();
    assertEquals(0, msg.repeatedPackedInt32.length);
    msg.repeatedPackedInt32 = new int[] { 123, 789, 456 };
    assertEquals(789, msg.repeatedPackedInt32[1]);
    assertEquals(456, msg.repeatedPackedInt32[2]);
    msg.clear();
    assertEquals(0, msg.repeatedPackedInt32.length);
    msg.clear()
       .repeatedPackedInt32 = new int[] { 456 };
    assertEquals(1, msg.repeatedPackedInt32.length);
    assertEquals(456, msg.repeatedPackedInt32[0]);
    msg.clear();
    assertEquals(0, msg.repeatedPackedInt32.length);

    // Test 1 entry
    msg.clear()
       .repeatedPackedInt32 = new int[] { 123 };
    assertEquals(1, msg.repeatedPackedInt32.length);
    byte [] result = MessageNano.toByteArray(msg);
    int msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 7);
    assertEquals(result.length, msgSerializedSize);
    TestAllTypesNano newMsg = TestAllTypesNano.parseFrom(result);
    assertEquals(1, newMsg.repeatedPackedInt32.length);
    assertEquals(123, newMsg.repeatedPackedInt32[0]);

    // Test 2 entries
    msg.clear()
       .repeatedPackedInt32 = new int[] { 123, 456 };
    assertEquals(2, msg.repeatedPackedInt32.length);
    result = MessageNano.toByteArray(msg);
    msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 9);
    assertEquals(result.length, msgSerializedSize);

    newMsg = TestAllTypesNano.parseFrom(result);
    assertEquals(2, newMsg.repeatedPackedInt32.length);
    assertEquals(123, newMsg.repeatedPackedInt32[0]);
    assertEquals(456, newMsg.repeatedPackedInt32[1]);
  }

  public void testNanoRepeatedPackedSfixed64() throws Exception {
    TestAllTypesNano msg = new TestAllTypesNano();
    assertEquals(0, msg.repeatedPackedSfixed64.length);
    msg.repeatedPackedSfixed64 = new long[] { 123, 789, 456 };
    assertEquals(789, msg.repeatedPackedSfixed64[1]);
    assertEquals(456, msg.repeatedPackedSfixed64[2]);
    msg.clear();
    assertEquals(0, msg.repeatedPackedSfixed64.length);
    msg.clear()
       .repeatedPackedSfixed64 = new long[] { 456 };
    assertEquals(1, msg.repeatedPackedSfixed64.length);
    assertEquals(456, msg.repeatedPackedSfixed64[0]);
    msg.clear();
    assertEquals(0, msg.repeatedPackedSfixed64.length);

    // Test 1 entry
    msg.clear()
       .repeatedPackedSfixed64 = new long[] { 123 };
    assertEquals(1, msg.repeatedPackedSfixed64.length);
    byte [] result = MessageNano.toByteArray(msg);
    int msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 14);
    assertEquals(result.length, msgSerializedSize);
    TestAllTypesNano newMsg = TestAllTypesNano.parseFrom(result);
    assertEquals(1, newMsg.repeatedPackedSfixed64.length);
    assertEquals(123, newMsg.repeatedPackedSfixed64[0]);

    // Test 2 entries
    msg.clear()
       .repeatedPackedSfixed64 = new long[] { 123, 456 };
    assertEquals(2, msg.repeatedPackedSfixed64.length);
    result = MessageNano.toByteArray(msg);
    msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 22);
    assertEquals(result.length, msgSerializedSize);

    newMsg = TestAllTypesNano.parseFrom(result);
    assertEquals(2, newMsg.repeatedPackedSfixed64.length);
    assertEquals(123, newMsg.repeatedPackedSfixed64[0]);
    assertEquals(456, newMsg.repeatedPackedSfixed64[1]);
  }

  public void testNanoRepeatedPackedNestedEnum() throws Exception {
    TestAllTypesNano msg = new TestAllTypesNano();
    msg.repeatedPackedNestedEnum = new int[] {
        TestAllTypesNano.FOO,
        TestAllTypesNano.BAR,
        TestAllTypesNano.BAZ
    };
    assertEquals(3, msg.repeatedPackedNestedEnum.length);
    assertEquals(TestAllTypesNano.FOO, msg.repeatedPackedNestedEnum[0]);
    assertEquals(TestAllTypesNano.BAR, msg.repeatedPackedNestedEnum[1]);
    assertEquals(TestAllTypesNano.BAZ, msg.repeatedPackedNestedEnum[2]);
    msg.clear();
    assertEquals(0, msg.repeatedPackedNestedEnum.length);
    msg.clear()
       .repeatedPackedNestedEnum = new int[] { TestAllTypesNano.BAR };
    assertEquals(1, msg.repeatedPackedNestedEnum.length);
    assertEquals(TestAllTypesNano.BAR, msg.repeatedPackedNestedEnum[0]);
    msg.clear();
    assertEquals(0, msg.repeatedPackedNestedEnum.length);

    // Test 1 entry
    msg.clear()
       .repeatedPackedNestedEnum = new int[] { TestAllTypesNano.FOO };
    byte [] result = MessageNano.toByteArray(msg);
    int msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 7);
    assertEquals(result.length, msgSerializedSize);
    TestAllTypesNano newMsg = TestAllTypesNano.parseFrom(result);
    assertEquals(1, newMsg.repeatedPackedNestedEnum.length);
    assertEquals(TestAllTypesNano.FOO, msg.repeatedPackedNestedEnum[0]);

    // Test 2 entries
    msg.clear()
       .repeatedPackedNestedEnum = new int[] { TestAllTypesNano.FOO, TestAllTypesNano.BAR };
    assertEquals(2, msg.repeatedPackedNestedEnum.length);
    result = MessageNano.toByteArray(msg);
    msgSerializedSize = msg.getSerializedSize();
    //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
    assertTrue(msgSerializedSize == 8);
    assertEquals(result.length, msgSerializedSize);

    newMsg = TestAllTypesNano.parseFrom(result);
    assertEquals(2, newMsg.repeatedPackedNestedEnum.length);
    assertEquals(TestAllTypesNano.FOO, msg.repeatedPackedNestedEnum[0]);
    assertEquals(TestAllTypesNano.BAR, msg.repeatedPackedNestedEnum[1]);
  }

  public void testNanoRepeatedInt32ReMerge() throws Exception {
    TestAllTypesNano msg = new TestAllTypesNano();
    msg.repeatedInt32 = new int[] { 234 };
    byte [] result1 = MessageNano.toByteArray(msg);

    msg.clear().optionalInt32 = 789;
    byte [] result2 = MessageNano.toByteArray(msg);

    msg.clear().repeatedInt32 = new int[] { 123, 456 };
    byte [] result3 = MessageNano.toByteArray(msg);

    // Concatenate the three serializations and read as one message.
    byte [] result = new byte[result1.length + result2.length + result3.length];
    System.arraycopy(result1, 0, result, 0, result1.length);
    System.arraycopy(result2, 0, result, result1.length, result2.length);
    System.arraycopy(result3, 0, result, result1.length + result2.length, result3.length);

    TestAllTypesNano newMsg = TestAllTypesNano.parseFrom(result);
    assertEquals(789, newMsg.optionalInt32);
    assertEquals(3, newMsg.repeatedInt32.length);
    assertEquals(234, newMsg.repeatedInt32[0]);
    assertEquals(123, newMsg.repeatedInt32[1]);
    assertEquals(456, newMsg.repeatedInt32[2]);
  }

  public void testNanoRepeatedNestedEnumReMerge() throws Exception {
    TestAllTypesNano msg = new TestAllTypesNano();
    msg.repeatedNestedEnum = new int[] { TestAllTypesNano.FOO };
    byte [] result1 = MessageNano.toByteArray(msg);

    msg.clear().optionalInt32 = 789;
    byte [] result2 = MessageNano.toByteArray(msg);

    msg.clear().repeatedNestedEnum = new int[] { TestAllTypesNano.BAR, TestAllTypesNano.FOO };
    byte [] result3 = MessageNano.toByteArray(msg);

    // Concatenate the three serializations and read as one message.
    byte [] result = new byte[result1.length + result2.length + result3.length];
    System.arraycopy(result1, 0, result, 0, result1.length);
    System.arraycopy(result2, 0, result, result1.length, result2.length);
    System.arraycopy(result3, 0, result, result1.length + result2.length, result3.length);

    TestAllTypesNano newMsg = TestAllTypesNano.parseFrom(result);
    assertEquals(789, newMsg.optionalInt32);
    assertEquals(3, newMsg.repeatedNestedEnum.length);
    assertEquals(TestAllTypesNano.FOO, newMsg.repeatedNestedEnum[0]);
    assertEquals(TestAllTypesNano.BAR, newMsg.repeatedNestedEnum[1]);
    assertEquals(TestAllTypesNano.FOO, newMsg.repeatedNestedEnum[2]);
  }

  public void testNanoRepeatedNestedMessageReMerge() throws Exception {
    TestAllTypesNano msg = new TestAllTypesNano();
    TestAllTypesNano.NestedMessage nestedMsg0 =
      new TestAllTypesNano.NestedMessage();
    nestedMsg0.bb = 0;
    TestAllTypesNano.NestedMessage nestedMsg1 =
      new TestAllTypesNano.NestedMessage();
    nestedMsg1.bb = 1;
    TestAllTypesNano.NestedMessage nestedMsg2 =
      new TestAllTypesNano.NestedMessage();
    nestedMsg2.bb = 2;

    msg.repeatedNestedMessage = new TestAllTypesNano.NestedMessage[] { nestedMsg0 };
    byte [] result1 = MessageNano.toByteArray(msg);

    msg.clear().optionalInt32 = 789;
    byte [] result2 = MessageNano.toByteArray(msg);

    msg.clear().repeatedNestedMessage =
        new TestAllTypesNano.NestedMessage[] { nestedMsg1, nestedMsg2 };
    byte [] result3 = MessageNano.toByteArray(msg);

    // Concatenate the three serializations and read as one message.
    byte [] result = new byte[result1.length + result2.length + result3.length];
    System.arraycopy(result1, 0, result, 0, result1.length);
    System.arraycopy(result2, 0, result, result1.length, result2.length);
    System.arraycopy(result3, 0, result, result1.length + result2.length, result3.length);

    TestAllTypesNano newMsg = TestAllTypesNano.parseFrom(result);
    assertEquals(789, newMsg.optionalInt32);
    assertEquals(3, newMsg.repeatedNestedMessage.length);
    assertEquals(nestedMsg0.bb, newMsg.repeatedNestedMessage[0].bb);
    assertEquals(nestedMsg1.bb, newMsg.repeatedNestedMessage[1].bb);
    assertEquals(nestedMsg2.bb, newMsg.repeatedNestedMessage[2].bb);
  }

  public void testNanoDefaults() throws Exception {
    TestAllTypesNano msg = new TestAllTypesNano();
    for (int i = 0; i < 2; i++) {
      assertEquals(41, msg.defaultInt32);
      assertEquals(42, msg.defaultInt64);
      assertEquals(43, msg.defaultUint32);
      assertEquals(44, msg.defaultUint64);
      assertEquals(-45, msg.defaultSint32);
      assertEquals(46, msg.defaultSint64);
      assertEquals(47, msg.defaultFixed32);
      assertEquals(48, msg.defaultFixed64);
      assertEquals(49, msg.defaultSfixed32);
      assertEquals(-50, msg.defaultSfixed64);
      assertTrue(51.5f == msg.defaultFloat);
      assertTrue(52.0e3 == msg.defaultDouble);
      assertEquals(true, msg.defaultBool);
      assertEquals("hello", msg.defaultString);
      assertEquals("world", new String(msg.defaultBytes, "UTF-8"));
      assertEquals("dünya", msg.defaultStringNonascii);
      assertEquals("dünyab", new String(msg.defaultBytesNonascii, "UTF-8"));
      assertEquals(TestAllTypesNano.BAR, msg.defaultNestedEnum);
      assertEquals(NanoOuterClass.FOREIGN_NANO_BAR, msg.defaultForeignEnum);
      assertEquals(UnittestImportNano.IMPORT_NANO_BAR, msg.defaultImportEnum);

      // Default values are not output, except for required fields.
      byte [] result = MessageNano.toByteArray(msg);
      int msgSerializedSize = msg.getSerializedSize();
      //System.out.printf("mss=%d result.length=%d\n", msgSerializedSize, result.length);
      assertTrue(msgSerializedSize == 3);
      assertEquals(result.length, msgSerializedSize);
      msg.clear();
    }
  }

  /**
   * Test that a bug in skipRawBytes() has been fixed:  if the skip skips
   * exactly up to a limit, this should not break things.
   */
  public void testSkipRawBytesBug() throws Exception {
    byte[] rawBytes = new byte[] { 1, 2 };
    CodedInputByteBufferNano input = CodedInputByteBufferNano.newInstance(rawBytes);

    int limit = input.pushLimit(1);
    input.skipRawBytes(1);
    input.popLimit(limit);
    assertEquals(2, input.readRawByte());
  }

  /**
   * Test that a bug in skipRawBytes() has been fixed:  if the skip skips
   * past the end of a buffer with a limit that has been set past the end of
   * that buffer, this should not break things.
   */
  public void testSkipRawBytesPastEndOfBufferWithLimit() throws Exception {
    byte[] rawBytes = new byte[] { 1, 2, 3, 4, 5 };
    CodedInputByteBufferNano input = CodedInputByteBufferNano.newInstance(rawBytes);

    int limit = input.pushLimit(4);
    // In order to expose the bug we need to read at least one byte to prime the
    // buffer inside the CodedInputStream.
    assertEquals(1, input.readRawByte());
    // Skip to the end of the limit.
    input.skipRawBytes(3);
    assertTrue(input.isAtEnd());
    input.popLimit(limit);
    assertEquals(5, input.readRawByte());
  }
}
