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

import static com.google.protobuf.FieldInfo.forField;
import static com.google.protobuf.FieldInfo.forPresenceCheckedField;

import com.google.protobuf.Descriptors.Descriptor;
import com.google.protobuf.Descriptors.FieldDescriptor;
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.util.List;

/** A factory for message info based on protobuf descriptors for a {@link GeneratedMessage}. */
@ExperimentalApi
public final class DescriptorMessageInfoFactory implements MessageInfoFactory {
  private static final String GET_DEFAULT_INSTANCE_METHOD_NAME = "getDefaultInstance";
  private static final DescriptorMessageInfoFactory instance = new DescriptorMessageInfoFactory();

  // Disallow construction - it's a singleton.
  private DescriptorMessageInfoFactory() {
  }

  public static DescriptorMessageInfoFactory getInstance() {
    return instance;
  }

  @Override
  public MessageInfo messageInfoFor(Class<?> messageType) {
    if (!GeneratedMessage.class.isAssignableFrom(messageType)) {
      throw new IllegalArgumentException("Only generated protobuf messages are supported");
    }

    return convert(messageType, descriptorForType(messageType));
  }

  private static Descriptor descriptorForType(Class<?> messageType) {
    try {
      Method method = messageType.getDeclaredMethod(GET_DEFAULT_INSTANCE_METHOD_NAME);
      GeneratedMessage message = (GeneratedMessage) method.invoke(null);
      return message.getDescriptorForType();
    } catch (Exception e) {
      throw new IllegalArgumentException(
          "Unable to get default instance for message class " + messageType.getName(), e);
    }
  }

  private static MessageInfo convert(Class<?> messageType, Descriptor desc) {
    switch (desc.getFile().getSyntax()) {
      case PROTO2:
        return convertProto2(messageType, desc);
      case PROTO3:
        return convertProto3(messageType, desc);
      default:
        throw new IllegalArgumentException("Unsupported syntax: " + desc.getFile().getSyntax());
    }
  }

  private static MessageInfo convertProto2(Class<?> messageType, Descriptor desc) {
    List<FieldDescriptor> fieldDescriptors = desc.getFields();
    MessageInfo.Builder builder = MessageInfo.newBuilder(fieldDescriptors.size());
    builder.withSyntax(ProtoSyntax.PROTO2);

    int bitFieldIndex = 0;
    int presenceMask = 1;
    Field bitField = bitField(messageType, bitFieldIndex++);

    // Fields in the descriptor are ordered by the index position in which they appear in the
    // proto file. This is the same order used to determine the presence mask used in the
    // bitFields. So to determine the appropriate presence mask to be used for a field, we simply
    // need to shift the presence mask whenever a presence-checked field is encountered.
    for (int i = 0; i < fieldDescriptors.size(); ++i) {
      FieldDescriptor fd = fieldDescriptors.get(i);
      Field field = field(messageType, fd);
      int number = fd.getNumber();
      FieldType type = getFieldType(fd);

      if (fd.isRepeated()) {
        // Repeated fields are not presence-checked.
        builder.add(forField(field, number, type));
        continue;
      }

      // It's a presence-checked field.
      builder.add(forPresenceCheckedField(field, number, type, bitField, presenceMask));

      // Update the presence mask/bitfield
      presenceMask <<= 1;
      if (presenceMask == 0) {
        // We've assigned all of the bits in the current bitField. Advance to the next one.
        bitField = bitField(messageType, bitFieldIndex++);
      }
    }

    return builder.build();
  }

  private static MessageInfo convertProto3(Class<?> messageType, Descriptor desc) {
    List<FieldDescriptor> fieldDescriptors = desc.getFields();
    MessageInfo.Builder builder = MessageInfo.newBuilder(fieldDescriptors.size());
    builder.withSyntax(ProtoSyntax.PROTO3);
    for (int i = 0; i < fieldDescriptors.size(); ++i) {
      FieldDescriptor fd = fieldDescriptors.get(i);
      builder.add(forField(field(messageType, fd), fd.getNumber(), getFieldType(fd)));
    }

    return builder.build();
  }

  private static FieldType getFieldType(FieldDescriptor fd) {
    switch (fd.getType()) {
      case BOOL:
        if (!fd.isRepeated()) {
          return FieldType.BOOL;
        }
        return fd.isPacked() ? FieldType.BOOL_LIST_PACKED : FieldType.BOOL_LIST;
      case BYTES:
        return fd.isRepeated() ? FieldType.BYTES_LIST : FieldType.BYTES;
      case DOUBLE:
        if (!fd.isRepeated()) {
          return FieldType.DOUBLE;
        }
        return fd.isPacked() ? FieldType.DOUBLE_LIST_PACKED : FieldType.DOUBLE_LIST;
      case ENUM:
        if (!fd.isRepeated()) {
          return FieldType.ENUM;
        }
        return fd.isPacked() ? FieldType.ENUM_LIST_PACKED : FieldType.ENUM_LIST;
      case FIXED32:
        if (!fd.isRepeated()) {
          return FieldType.FIXED32;
        }
        return fd.isPacked() ? FieldType.FIXED32_LIST_PACKED : FieldType.FIXED32_LIST;
      case FIXED64:
        if (!fd.isRepeated()) {
          return FieldType.FIXED64;
        }
        return fd.isPacked() ? FieldType.FIXED64_LIST_PACKED : FieldType.FIXED64_LIST;
      case FLOAT:
        if (!fd.isRepeated()) {
          return FieldType.FLOAT;
        }
        return fd.isPacked() ? FieldType.FLOAT_LIST_PACKED : FieldType.FLOAT_LIST;
      case GROUP:
        return fd.isRepeated() ? FieldType.GROUP_LIST : FieldType.GROUP;
      case INT32:
        if (!fd.isRepeated()) {
          return FieldType.INT32;
        }
        return fd.isPacked() ? FieldType.INT32_LIST_PACKED : FieldType.INT32_LIST;
      case INT64:
        if (!fd.isRepeated()) {
          return FieldType.INT64;
        }
        return fd.isPacked() ? FieldType.INT64_LIST_PACKED : FieldType.INT64_LIST;
      case MESSAGE:
        // TODO(nathanmittler): Add support for maps.
        return fd.isRepeated() ? FieldType.MESSAGE_LIST : FieldType.MESSAGE;
      case SFIXED32:
        if (!fd.isRepeated()) {
          return FieldType.SFIXED32;
        }
        return fd.isPacked() ? FieldType.SFIXED32_LIST_PACKED : FieldType.SFIXED32_LIST;
      case SFIXED64:
        if (!fd.isRepeated()) {
          return FieldType.SFIXED64;
        }
        return fd.isPacked() ? FieldType.SFIXED64_LIST_PACKED : FieldType.SFIXED64_LIST;
      case SINT32:
        if (!fd.isRepeated()) {
          return FieldType.SINT32;
        }
        return fd.isPacked() ? FieldType.SINT32_LIST_PACKED : FieldType.SINT32_LIST;
      case SINT64:
        if (!fd.isRepeated()) {
          return FieldType.SINT64;
        }
        return fd.isPacked() ? FieldType.SINT64_LIST_PACKED : FieldType.SINT64_LIST;
      case STRING:
        return fd.isRepeated() ? FieldType.STRING_LIST : FieldType.STRING;
      case UINT32:
        if (!fd.isRepeated()) {
          return FieldType.UINT32;
        }
        return fd.isPacked() ? FieldType.UINT32_LIST_PACKED : FieldType.UINT32_LIST;
      case UINT64:
        if (!fd.isRepeated()) {
          return FieldType.UINT64;
        }
        return fd.isPacked() ? FieldType.UINT64_LIST_PACKED : FieldType.UINT64_LIST;
      default:
        throw new IllegalArgumentException("Unsupported field type: " + fd.getType());
    }
  }

  private static Field bitField(Class<?> messageType, int index) {
    return field(messageType, "bitField" + index + "_");
  }

  private static Field field(Class<?> messageType, FieldDescriptor fd) {
    return field(messageType, getFieldName(fd));
  }

  private static Field field(Class<?> messageType, String fieldName) {
    try {
      return messageType.getDeclaredField(fieldName);
    } catch (Exception e) {
      throw new IllegalArgumentException(
          "Unable to find field " + fieldName + " in message class " + messageType.getName());
    }
  }

  // This method must match exactly with the corresponding function in protocol compiler.
  // See: https://github.com/google/protobuf/blob/v3.0.0/src/google/protobuf/compiler/java/java_helpers.cc#L153
  private static String getFieldName(FieldDescriptor fd) {
    String snakeCase =
        fd.getType() == FieldDescriptor.Type.GROUP ? fd.getMessageType().getName() : fd.getName();
    StringBuilder sb = new StringBuilder(snakeCase.length() + 1);
    boolean capNext = false;
    for (int ctr = 0; ctr < snakeCase.length(); ctr++) {
      char next = snakeCase.charAt(ctr);
      if (next == '_') {
        capNext = true;
      } else if (capNext) {
        sb.append(Character.toUpperCase(next));
        capNext = false;
      } else if (ctr == 0) {
        sb.append(Character.toLowerCase(next));
      } else {
        sb.append(next);
      }
    }
    sb.append('_');
    return sb.toString();
  }
}
