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

import java.io.IOException;
import java.util.List;
import java.util.Map;

/** A reader of fields from a serialized protobuf message. */
// TODO(nathanmittler): Refactor to allow the reader to allocate properly sized lists.
@ExperimentalApi
interface Reader {
  /** Value used to indicate that the end of input has been reached. */
  int READ_DONE = Integer.MAX_VALUE;

  /** Value used to indicate that the reader does not know the tag about the field. */
  int TAG_UNKNOWN = 0;

  boolean shouldDiscardUnknownFields();

  /**
   * Gets the field number for the current field being read.
   *
   * <p>TODO(liujisi): Rename it to make it more explicit about the side effect on the underlying
   * buffer.
   *
   * @return the current field number or {@link #READ_DONE} if the end of input has been reached.
   */
  int getFieldNumber() throws IOException;

  /**
   * Gets the wire tag of the current field.
   *
   * @return the current wire tag or {@link #TAG_UNKNOWN} if the reader does not know the tag of the
   *     current field.
   */
  int getTag();

  /**
   * Skips the current field and advances the reader to the next field.
   *
   * @return {@code true} if there are more fields or {@code false} if the end of input has been
   *     reached.
   */
  boolean skipField() throws IOException;

  /**
   * Reads and returns the next field of type {@code DOUBLE} and advances the reader to the next
   * field.
   */
  double readDouble() throws IOException;

  /**
   * Reads and returns the next field of type {@code FLOAT} and advances the reader to the next
   * field.
   */
  float readFloat() throws IOException;

  /**
   * Reads and returns the next field of type {@code UINT64} and advances the reader to the next
   * field.
   */
  long readUInt64() throws IOException;

  /**
   * Reads and returns the next field of type {@code INT64} and advances the reader to the next
   * field.
   */
  long readInt64() throws IOException;

  /**
   * Reads and returns the next field of type {@code INT32} and advances the reader to the next
   * field.
   */
  int readInt32() throws IOException;

  /**
   * Reads and returns the next field of type {@code FIXED64} and advances the reader to the next
   * field.
   */
  long readFixed64() throws IOException;

  /**
   * Reads and returns the next field of type {@code FIXED32} and advances the reader to the next
   * field.
   */
  int readFixed32() throws IOException;

  /**
   * Reads and returns the next field of type {@code BOOL} and advances the reader to the next
   * field.
   */
  boolean readBool() throws IOException;

  /**
   * Reads and returns the next field of type {@code STRING} and advances the reader to the next
   * field. If the stream contains malformed UTF-8, replace the offending bytes with the standard
   * UTF-8 replacement character.
   */
  String readString() throws IOException;

  /**
   * Reads and returns the next field of type {@code STRING} and advances the reader to the next
   * field. If the stream contains malformed UTF-8, throw exception {@link
   * InvalidProtocolBufferException}.
   */
  String readStringRequireUtf8() throws IOException;

  // TODO(yilunchong): the lack of other opinions for whether to expose this on the interface
  <T> T readMessageBySchemaWithCheck(Schema<T> schema, ExtensionRegistryLite extensionRegistry)
      throws IOException;

  /**
   * Reads and returns the next field of type {@code MESSAGE} and advances the reader to the next
   * field.
   */
  <T> T readMessage(Class<T> clazz, ExtensionRegistryLite extensionRegistry) throws IOException;

  /**
   * Reads and returns the next field of type {@code GROUP} and advances the reader to the next
   * field.
   *
   * @deprecated groups fields are deprecated.
   */
  @Deprecated
  <T> T readGroup(Class<T> clazz, ExtensionRegistryLite extensionRegistry) throws IOException;

  // TODO(yilunchong): the lack of other opinions for whether to expose this on the interface
  @Deprecated
  <T> T readGroupBySchemaWithCheck(Schema<T> schema, ExtensionRegistryLite extensionRegistry)
      throws IOException;

  /**
   * Reads and returns the next field of type {@code BYTES} and advances the reader to the next
   * field.
   */
  ByteString readBytes() throws IOException;

  /**
   * Reads and returns the next field of type {@code UINT32} and advances the reader to the next
   * field.
   */
  int readUInt32() throws IOException;

  /**
   * Reads and returns the next field of type {@code ENUM} and advances the reader to the next
   * field.
   */
  int readEnum() throws IOException;

  /**
   * Reads and returns the next field of type {@code SFIXED32} and advances the reader to the next
   * field.
   */
  int readSFixed32() throws IOException;

  /**
   * Reads and returns the next field of type {@code SFIXED64} and advances the reader to the next
   * field.
   */
  long readSFixed64() throws IOException;

  /**
   * Reads and returns the next field of type {@code SINT32} and advances the reader to the next
   * field.
   */
  int readSInt32() throws IOException;

  /**
   * Reads and returns the next field of type {@code SINT64} and advances the reader to the next
   * field.
   */
  long readSInt64() throws IOException;

  /**
   * Reads the next field of type {@code DOUBLE_LIST} or {@code DOUBLE_LIST_PACKED} and advances the
   * reader to the next field.
   *
   * @param target the list that will receive the read values.
   */
  void readDoubleList(List<Double> target) throws IOException;

  /**
   * Reads the next field of type {@code FLOAT_LIST} or {@code FLOAT_LIST_PACKED} and advances the
   * reader to the next field.
   *
   * @param target the list that will receive the read values.
   */
  void readFloatList(List<Float> target) throws IOException;

  /**
   * Reads the next field of type {@code UINT64_LIST} or {@code UINT64_LIST_PACKED} and advances the
   * reader to the next field.
   *
   * @param target the list that will receive the read values.
   */
  void readUInt64List(List<Long> target) throws IOException;

  /**
   * Reads the next field of type {@code INT64_LIST} or {@code INT64_LIST_PACKED} and advances the
   * reader to the next field.
   *
   * @param target the list that will receive the read values.
   */
  void readInt64List(List<Long> target) throws IOException;

  /**
   * Reads the next field of type {@code INT32_LIST} or {@code INT32_LIST_PACKED} and advances the
   * reader to the next field.
   *
   * @param target the list that will receive the read values.
   */
  void readInt32List(List<Integer> target) throws IOException;

  /**
   * Reads the next field of type {@code FIXED64_LIST} or {@code FIXED64_LIST_PACKED} and advances
   * the reader to the next field.
   *
   * @param target the list that will receive the read values.
   */
  void readFixed64List(List<Long> target) throws IOException;

  /**
   * Reads the next field of type {@code FIXED32_LIST} or {@code FIXED32_LIST_PACKED} and advances
   * the reader to the next field.
   *
   * @param target the list that will receive the read values.
   */
  void readFixed32List(List<Integer> target) throws IOException;

  /**
   * Reads the next field of type {@code BOOL_LIST} or {@code BOOL_LIST_PACKED} and advances the
   * reader to the next field.
   *
   * @param target the list that will receive the read values.
   */
  void readBoolList(List<Boolean> target) throws IOException;

  /**
   * Reads the next field of type {@code STRING_LIST} and advances the reader to the next field.
   *
   * @param target the list that will receive the read values.
   */
  void readStringList(List<String> target) throws IOException;

  /**
   * Reads the next field of type {@code STRING_LIST} and advances the reader to the next field. If
   * the stream contains malformed UTF-8, throw exception {@link InvalidProtocolBufferException}.
   *
   * @param target the list that will receive the read values.
   */
  void readStringListRequireUtf8(List<String> target) throws IOException;

  /**
   * Reads the next field of type {@code MESSAGE_LIST} and advances the reader to the next field.
   *
   * @param target the list that will receive the read values.
   * @param targetType the type of the elements stored in the {@code target} list.
   */
  <T> void readMessageList(
      List<T> target, Schema<T> schema, ExtensionRegistryLite extensionRegistry) throws IOException;

  <T> void readMessageList(
      List<T> target, Class<T> targetType, ExtensionRegistryLite extensionRegistry)
      throws IOException;

  /**
   * Reads the next field of type {@code GROUP_LIST} and advances the reader to the next field.
   *
   * @param target the list that will receive the read values.
   * @param targetType the type of the elements stored in the {@code target} list.
   * @deprecated groups fields are deprecated.
   */
  @Deprecated
  <T> void readGroupList(
      List<T> target, Class<T> targetType, ExtensionRegistryLite extensionRegistry)
      throws IOException;

  @Deprecated
  <T> void readGroupList(
      List<T> target, Schema<T> targetType, ExtensionRegistryLite extensionRegistry)
      throws IOException;

  /**
   * Reads the next field of type {@code BYTES_LIST} and advances the reader to the next field.
   *
   * @param target the list that will receive the read values.
   */
  void readBytesList(List<ByteString> target) throws IOException;

  /**
   * Reads the next field of type {@code UINT32_LIST} or {@code UINT32_LIST_PACKED} and advances the
   * reader to the next field.
   *
   * @param target the list that will receive the read values.
   */
  void readUInt32List(List<Integer> target) throws IOException;

  /**
   * Reads the next field of type {@code ENUM_LIST} or {@code ENUM_LIST_PACKED} and advances the
   * reader to the next field.
   *
   * @param target the list that will receive the read values.
   */
  void readEnumList(List<Integer> target) throws IOException;

  /**
   * Reads the next field of type {@code SFIXED32_LIST} or {@code SFIXED32_LIST_PACKED} and advances
   * the reader to the next field.
   *
   * @param target the list that will receive the read values.
   */
  void readSFixed32List(List<Integer> target) throws IOException;

  /**
   * Reads the next field of type {@code SFIXED64_LIST} or {@code SFIXED64_LIST_PACKED} and advances
   * the reader to the next field.
   *
   * @param target the list that will receive the read values.
   */
  void readSFixed64List(List<Long> target) throws IOException;

  /**
   * Reads the next field of type {@code SINT32_LIST} or {@code SINT32_LIST_PACKED} and advances the
   * reader to the next field.
   *
   * @param target the list that will receive the read values.
   */
  void readSInt32List(List<Integer> target) throws IOException;

  /**
   * Reads the next field of type {@code SINT64_LIST} or {@code SINT64_LIST_PACKED} and advances the
   * reader to the next field.
   *
   * @param target the list that will receive the read values.
   */
  void readSInt64List(List<Long> target) throws IOException;

  /**
   * Reads the next field of type {@code MAP} and advances the reader to the next field.
   *
   * @param target the mutable map that will receive the read values.
   * @param mapDefaultEntry the default entry of the map field.
   * @param extensionRegistry the extension registry for parsing message value fields.
   */
  <K, V> void readMap(
      Map<K, V> target,
      MapEntryLite.Metadata<K, V> mapDefaultEntry,
      ExtensionRegistryLite extensionRegistry)
      throws IOException;
}
