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

import static com.google.protobuf.Proto2Manifest.FIELD_LENGTH;
import static com.google.protobuf.Proto2Manifest.offset;
import static com.google.protobuf.Proto2Manifest.type;

import java.util.Collections;

/** Abtract base class for standard (i.e. non-lite) proto2 schemas. */
abstract class AbstractProto2StandardSchema<T> extends AbstractProto2Schema<T> {
  final Class<?> messageClass;

  public AbstractProto2StandardSchema(Class<T> messageClass, Proto2Manifest manifest) {
    super(manifest);
    this.messageClass = messageClass;
  }

  @SuppressWarnings("unchecked")
  @Override
  public final T newInstance() {
    T msg = (T) UnsafeUtil.allocateInstance(messageClass);
    for (long pos = manifest.address; pos < manifest.limit; pos += FIELD_LENGTH) {
      final int typeAndOffset = manifest.typeAndOffsetAt(pos);
      switch (type(typeAndOffset)) {
        case 0: //DOUBLE:
        case 1: //FLOAT:
        case 2: //INT64:
        case 3: //UINT64:
        case 4: //INT32:
        case 5: //FIXED64:
        case 6: //FIXED32:
        case 7: //BOOL:
        case 9: //MESSAGE:
        case 11: //UINT32:
        case 12: //ENUM:
        case 13: //SFIXED32:
        case 14: //SFIXED64:
        case 15: //SINT32:
        case 16: //SINT64:
          // Do nothing, just use default values.
          break;
        case 8: //STRING:
          UnsafeUtil.putObject(msg, offset(typeAndOffset), "");
          break;
        case 10: //BYTES:
          UnsafeUtil.putObject(msg, offset(typeAndOffset), ByteString.EMPTY);
          break;
        case 17: //DOUBLE_LIST:
        case 18: //FLOAT_LIST:
        case 19: //INT64_LIST:
        case 20: //UINT64_LIST:
        case 21: //INT32_LIST:
        case 22: //FIXED64_LIST:
        case 23: //FIXED32_LIST:
        case 24: //BOOL_LIST:
        case 26: //MESSAGE_LIST:
        case 27: //BYTES_LIST:
        case 28: //UINT32_LIST:
        case 29: //ENUM_LIST:
        case 30: //SFIXED32_LIST:
        case 31: //SFIXED64_LIST:
        case 32: //SINT32_LIST:
        case 33: //SINT64_LIST:
        case 34: //DOUBLE_LIST_PACKED:
        case 35: //FLOAT_LIST_PACKED:
        case 36: //INT64_LIST_PACKED:
        case 37: //UINT64_LIST_PACKED:
        case 38: //INT32_LIST_PACKED:
        case 39: //FIXED64_LIST_PACKED:
        case 40: //FIXED32_LIST_PACKED:
        case 41: //BOOL_LIST_PACKED:
        case 42: //UINT32_LIST_PACKED:
        case 43: //ENUM_LIST_PACKED:
        case 44: //SFIXED32_LIST_PACKED:
        case 45: //SFIXED64_LIST_PACKED:
        case 46: //SINT32_LIST_PACKED:
        case 47: //SINT64_LIST_PACKED:
          UnsafeUtil.putObject(msg, offset(typeAndOffset), Collections.emptyList());
          break;
        case 25: //STRING_LIST:
          UnsafeUtil.putObject(msg, offset(typeAndOffset), LazyStringArrayList.EMPTY);
          break;
        case -4: //GROUP
          break;
        case -3: //GROUP_LIST
          UnsafeUtil.putObject(msg, offset(typeAndOffset), Collections.emptyList());
          break;
        default:
          break;
      }
    }

    // Initialize the base class fields.
    SchemaUtil.initBaseClassFields(msg);
    return msg;
  }
}
