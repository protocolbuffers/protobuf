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

import static com.google.protobuf.ArrayDecoders.decodeBoolList;
import static com.google.protobuf.ArrayDecoders.decodeBytes;
import static com.google.protobuf.ArrayDecoders.decodeBytesList;
import static com.google.protobuf.ArrayDecoders.decodeDouble;
import static com.google.protobuf.ArrayDecoders.decodeDoubleList;
import static com.google.protobuf.ArrayDecoders.decodeExtensionOrUnknownField;
import static com.google.protobuf.ArrayDecoders.decodeFixed32;
import static com.google.protobuf.ArrayDecoders.decodeFixed32List;
import static com.google.protobuf.ArrayDecoders.decodeFixed64;
import static com.google.protobuf.ArrayDecoders.decodeFixed64List;
import static com.google.protobuf.ArrayDecoders.decodeFloat;
import static com.google.protobuf.ArrayDecoders.decodeFloatList;
import static com.google.protobuf.ArrayDecoders.decodeGroupField;
import static com.google.protobuf.ArrayDecoders.decodeGroupList;
import static com.google.protobuf.ArrayDecoders.decodeMessageField;
import static com.google.protobuf.ArrayDecoders.decodeMessageList;
import static com.google.protobuf.ArrayDecoders.decodePackedBoolList;
import static com.google.protobuf.ArrayDecoders.decodePackedDoubleList;
import static com.google.protobuf.ArrayDecoders.decodePackedFixed32List;
import static com.google.protobuf.ArrayDecoders.decodePackedFixed64List;
import static com.google.protobuf.ArrayDecoders.decodePackedFloatList;
import static com.google.protobuf.ArrayDecoders.decodePackedSInt32List;
import static com.google.protobuf.ArrayDecoders.decodePackedSInt64List;
import static com.google.protobuf.ArrayDecoders.decodePackedVarint32List;
import static com.google.protobuf.ArrayDecoders.decodePackedVarint64List;
import static com.google.protobuf.ArrayDecoders.decodeSInt32List;
import static com.google.protobuf.ArrayDecoders.decodeSInt64List;
import static com.google.protobuf.ArrayDecoders.decodeString;
import static com.google.protobuf.ArrayDecoders.decodeStringList;
import static com.google.protobuf.ArrayDecoders.decodeStringListRequireUtf8;
import static com.google.protobuf.ArrayDecoders.decodeStringRequireUtf8;
import static com.google.protobuf.ArrayDecoders.decodeUnknownField;
import static com.google.protobuf.ArrayDecoders.decodeVarint32;
import static com.google.protobuf.ArrayDecoders.decodeVarint32List;
import static com.google.protobuf.ArrayDecoders.decodeVarint64;
import static com.google.protobuf.ArrayDecoders.decodeVarint64List;
import static com.google.protobuf.ArrayDecoders.skipField;

import com.google.protobuf.ArrayDecoders.Registers;
import com.google.protobuf.ByteString.CodedBuilder;
import com.google.protobuf.FieldSet.FieldDescriptorLite;
import com.google.protobuf.Internal.EnumVerifier;
import com.google.protobuf.Internal.ProtobufList;
import com.google.protobuf.MapEntryLite.Metadata;
import java.io.IOException;
import java.lang.reflect.Field;
import java.util.Arrays;
import java.util.Iterator;
import java.util.List;
import java.util.Map;

/** Schema used for standard messages. */
final class MessageSchema<T> implements Schema<T> {
  private static final int INTS_PER_FIELD = 3;
  private static final int OFFSET_BITS = 20;
  private static final int OFFSET_MASK = 0XFFFFF;
  private static final int FIELD_TYPE_MASK = 0x0FF00000;
  private static final int REQUIRED_MASK = 0x10000000;
  private static final int ENFORCE_UTF8_MASK = 0x20000000;
  private static final int[] EMPTY_INT_ARRAY = new int[0];

  /** An offset applied to the field type ID for scalar fields that are a member of a oneof. */
  static final int ONEOF_TYPE_OFFSET = 51 /* FieldType.MAP + 1 */;

  /**
   * Keep a direct reference to the unsafe object so we don't need to go through the UnsafeUtil
   * wrapper for every call.
   */
  private static final sun.misc.Unsafe UNSAFE = UnsafeUtil.getUnsafe();

  /**
   * Holds all information for accessing the message fields. The layout is as follows (field
   * positions are relative to the offset of the start of the field in the buffer):
   *
   * <p>
   *
   * <pre>
   * [ 0 -    3] unused
   * [ 4 -   31] field number
   * [32 -   33] unused
   * [34 -   34] whether UTF-8 enforcement should be applied to a string field.
   * [35 -   35] whether the field is required
   * [36 -   43] field type / for oneof: field type + {@link #ONEOF_TYPE_OFFSET}
   * [44 -   63] field offset / oneof value field offset
   * [64 -   69] unused
   * [70 -   75] field presence mask shift (unused for oneof/repeated fields)
   * [76 -   95] presence field offset / oneof case field offset / cached size field offset
   * </pre>
   */
  private final int[] buffer;

  /**
   * Holds object references for fields. For each field entry in {@code buffer}, there are two
   * corresponding entries in this array. The content is different from different field types:
   *
   * <pre>
   *   Map fields:
   *     objects[pos] = map default entry instance
   *     objects[pos + 1] = EnumVerifier if map value is enum, or message class reference if map
   *                        value is message.
   *   Message fields:
   *     objects[pos] = null or cached message schema
   *     objects[pos + 1] = message class reference
   *   Enum fields:
   *     objects[pos] = null
   *     objects[pos + 1] = EnumVerifier
   * </pre>
   */
  private final Object[] objects;

  private final int minFieldNumber;
  private final int maxFieldNumber;

  private final MessageLite defaultInstance;
  private final boolean hasExtensions;
  private final boolean lite;
  private final boolean proto3;
  // TODO(xiaofeng): Make both full-runtime and lite-runtime support cached field size.
  private final boolean useCachedSizeField;

  /** Represents [checkInitialized positions, map field positions, repeated field offsets]. */
  private final int[] intArray;

  /**
   * Values at indices 0 -> checkInitializedCount in intArray are positions to check for
   * initialization.
   */
  private final int checkInitializedCount;

  /**
   * Values at indices checkInitializedCount -> repeatedFieldOffsetStart are map positions.
   * Everything after that are repeated field offsets.
   */
  private final int repeatedFieldOffsetStart;

  private final NewInstanceSchema newInstanceSchema;
  private final ListFieldSchema listFieldSchema;
  private final UnknownFieldSchema<?, ?> unknownFieldSchema;
  private final ExtensionSchema<?> extensionSchema;
  private final MapFieldSchema mapFieldSchema;

  private MessageSchema(
      int[] buffer,
      Object[] objects,
      int minFieldNumber,
      int maxFieldNumber,
      MessageLite defaultInstance,
      boolean proto3,
      boolean useCachedSizeField,
      int[] intArray,
      int checkInitialized,
      int mapFieldPositions,
      NewInstanceSchema newInstanceSchema,
      ListFieldSchema listFieldSchema,
      UnknownFieldSchema<?, ?> unknownFieldSchema,
      ExtensionSchema<?> extensionSchema,
      MapFieldSchema mapFieldSchema) {
    this.buffer = buffer;
    this.objects = objects;
    this.minFieldNumber = minFieldNumber;
    this.maxFieldNumber = maxFieldNumber;

    this.lite = defaultInstance instanceof GeneratedMessageLite;
    this.proto3 = proto3;
    this.hasExtensions = extensionSchema != null && extensionSchema.hasExtensions(defaultInstance);
    this.useCachedSizeField = useCachedSizeField;

    this.intArray = intArray;
    this.checkInitializedCount = checkInitialized;
    this.repeatedFieldOffsetStart = mapFieldPositions;

    this.newInstanceSchema = newInstanceSchema;
    this.listFieldSchema = listFieldSchema;
    this.unknownFieldSchema = unknownFieldSchema;
    this.extensionSchema = extensionSchema;
    this.defaultInstance = defaultInstance;
    this.mapFieldSchema = mapFieldSchema;
  }

  static <T> MessageSchema<T> newSchema(
      Class<T> messageClass,
      MessageInfo messageInfo,
      NewInstanceSchema newInstanceSchema,
      ListFieldSchema listFieldSchema,
      UnknownFieldSchema<?, ?> unknownFieldSchema,
      ExtensionSchema<?> extensionSchema,
      MapFieldSchema mapFieldSchema) {
    if (messageInfo instanceof RawMessageInfo) {
      return newSchemaForRawMessageInfo(
          (RawMessageInfo) messageInfo,
          newInstanceSchema,
          listFieldSchema,
          unknownFieldSchema,
          extensionSchema,
          mapFieldSchema);

    } else {
      return newSchemaForMessageInfo(
          (StructuralMessageInfo) messageInfo,
          newInstanceSchema,
          listFieldSchema,
          unknownFieldSchema,
          extensionSchema,
          mapFieldSchema);
    }
  }

  static <T> MessageSchema<T> newSchemaForRawMessageInfo(
      RawMessageInfo messageInfo,
      NewInstanceSchema newInstanceSchema,
      ListFieldSchema listFieldSchema,
      UnknownFieldSchema<?, ?> unknownFieldSchema,
      ExtensionSchema<?> extensionSchema,
      MapFieldSchema mapFieldSchema) {
    final boolean isProto3 = messageInfo.getSyntax() == ProtoSyntax.PROTO3;

    String info = messageInfo.getStringInfo();
    final int length = info.length();
    int i = 0;

    int next = info.charAt(i++);
    if (next >= 0xD800) {
      int result = next & 0x1FFF;
      int shift = 13;
      while ((next = info.charAt(i++)) >= 0xD800) {
        result |= (next & 0x1FFF) << shift;
        shift += 13;
      }
      next = result | (next << shift);
    }
    final int flags = next;

    next = info.charAt(i++);
    if (next >= 0xD800) {
      int result = next & 0x1FFF;
      int shift = 13;
      while ((next = info.charAt(i++)) >= 0xD800) {
        result |= (next & 0x1FFF) << shift;
        shift += 13;
      }
      next = result | (next << shift);
    }
    final int fieldCount = next;

    final int oneofCount;
    final int hasBitsCount;
    final int minFieldNumber;
    final int maxFieldNumber;
    final int numEntries;
    final int mapFieldCount;
    final int repeatedFieldCount;
    final int checkInitialized;
    final int[] intArray;
    int objectsPosition;
    if (fieldCount == 0) {
      oneofCount = 0;
      hasBitsCount = 0;
      minFieldNumber = 0;
      maxFieldNumber = 0;
      numEntries = 0;
      mapFieldCount = 0;
      repeatedFieldCount = 0;
      checkInitialized = 0;
      intArray = EMPTY_INT_ARRAY;
      objectsPosition = 0;
    } else {
      next = info.charAt(i++);
      if (next >= 0xD800) {
        int result = next & 0x1FFF;
        int shift = 13;
        while ((next = info.charAt(i++)) >= 0xD800) {
          result |= (next & 0x1FFF) << shift;
          shift += 13;
        }
        next = result | (next << shift);
      }
      oneofCount = next;

      next = info.charAt(i++);
      if (next >= 0xD800) {
        int result = next & 0x1FFF;
        int shift = 13;
        while ((next = info.charAt(i++)) >= 0xD800) {
          result |= (next & 0x1FFF) << shift;
          shift += 13;
        }
        next = result | (next << shift);
      }
      hasBitsCount = next;

      next = info.charAt(i++);
      if (next >= 0xD800) {
        int result = next & 0x1FFF;
        int shift = 13;
        while ((next = info.charAt(i++)) >= 0xD800) {
          result |= (next & 0x1FFF) << shift;
          shift += 13;
        }
        next = result | (next << shift);
      }
      minFieldNumber = next;

      next = info.charAt(i++);
      if (next >= 0xD800) {
        int result = next & 0x1FFF;
        int shift = 13;
        while ((next = info.charAt(i++)) >= 0xD800) {
          result |= (next & 0x1FFF) << shift;
          shift += 13;
        }
        next = result | (next << shift);
      }
      maxFieldNumber = next;

      next = info.charAt(i++);
      if (next >= 0xD800) {
        int result = next & 0x1FFF;
        int shift = 13;
        while ((next = info.charAt(i++)) >= 0xD800) {
          result |= (next & 0x1FFF) << shift;
          shift += 13;
        }
        next = result | (next << shift);
      }
      numEntries = next;

      next = info.charAt(i++);
      if (next >= 0xD800) {
        int result = next & 0x1FFF;
        int shift = 13;
        while ((next = info.charAt(i++)) >= 0xD800) {
          result |= (next & 0x1FFF) << shift;
          shift += 13;
        }
        next = result | (next << shift);
      }
      mapFieldCount = next;

      next = info.charAt(i++);
      if (next >= 0xD800) {
        int result = next & 0x1FFF;
        int shift = 13;
        while ((next = info.charAt(i++)) >= 0xD800) {
          result |= (next & 0x1FFF) << shift;
          shift += 13;
        }
        next = result | (next << shift);
      }
      repeatedFieldCount = next;

      next = info.charAt(i++);
      if (next >= 0xD800) {
        int result = next & 0x1FFF;
        int shift = 13;
        while ((next = info.charAt(i++)) >= 0xD800) {
          result |= (next & 0x1FFF) << shift;
          shift += 13;
        }
        next = result | (next << shift);
      }
      checkInitialized = next;
      intArray = new int[checkInitialized + mapFieldCount + repeatedFieldCount];
      // Field objects are after a list of (oneof, oneofCase) pairs  + a list of hasbits fields.
      objectsPosition = oneofCount * 2 + hasBitsCount;
    }

    final sun.misc.Unsafe unsafe = UNSAFE;
    final Object[] messageInfoObjects = messageInfo.getObjects();
    int checkInitializedPosition = 0;
    final Class<?> messageClass = messageInfo.getDefaultInstance().getClass();
    int[] buffer = new int[numEntries * INTS_PER_FIELD];
    Object[] objects = new Object[numEntries * 2];

    int mapFieldIndex = checkInitialized;
    int repeatedFieldIndex = checkInitialized + mapFieldCount;

    int bufferIndex = 0;
    while (i < length) {
      final int fieldNumber;
      final int fieldTypeWithExtraBits;
      final int fieldType;

      next = info.charAt(i++);
      if (next >= 0xD800) {
        int result = next & 0x1FFF;
        int shift = 13;
        while ((next = info.charAt(i++)) >= 0xD800) {
          result |= (next & 0x1FFF) << shift;
          shift += 13;
        }
        next = result | (next << shift);
      }
      fieldNumber = next;

      next = info.charAt(i++);
      if (next >= 0xD800) {
        int result = next & 0x1FFF;
        int shift = 13;
        while ((next = info.charAt(i++)) >= 0xD800) {
          result |= (next & 0x1FFF) << shift;
          shift += 13;
        }
        next = result | (next << shift);
      }
      fieldTypeWithExtraBits = next;
      fieldType = fieldTypeWithExtraBits & 0xFF;

      if ((fieldTypeWithExtraBits & 0x400) != 0) {
        intArray[checkInitializedPosition++] = bufferIndex;
      }

      final int fieldOffset;
      final int presenceMaskShift;
      final int presenceFieldOffset;

      // Oneof
      if (fieldType >= ONEOF_TYPE_OFFSET) {
        next = info.charAt(i++);
        if (next >= 0xD800) {
          int result = next & 0x1FFF;
          int shift = 13;
          while ((next = info.charAt(i++)) >= 0xD800) {
            result |= (next & 0x1FFF) << shift;
            shift += 13;
          }
          next = result | (next << shift);
        }
        int oneofIndex = next;

        final int oneofFieldType = fieldType - ONEOF_TYPE_OFFSET;
        if (oneofFieldType == 9 /* FieldType.MESSAGE */
            || oneofFieldType == 17 /* FieldType.GROUP */) {
          objects[bufferIndex / INTS_PER_FIELD * 2 + 1] = messageInfoObjects[objectsPosition++];
        } else if (oneofFieldType == 12 /* FieldType.ENUM */) {
          // proto2
          if ((flags & 0x1) == 0x1) {
            objects[bufferIndex / INTS_PER_FIELD * 2 + 1] = messageInfoObjects[objectsPosition++];
          }
        }

        final java.lang.reflect.Field oneofField;
        int index = oneofIndex * 2;
        Object o = messageInfoObjects[index];
        if (o instanceof java.lang.reflect.Field) {
          oneofField = (java.lang.reflect.Field) o;
        } else {
          oneofField = reflectField(messageClass, (String) o);
          // Memoize java.lang.reflect.Field instances for oneof/hasbits fields, since they're
          // potentially used for many Protobuf fields.  Since there's a 1-1 mapping from the
          // Protobuf field to the Java Field for non-oneofs, there's no benefit for memoizing
          // those.
          messageInfoObjects[index] = oneofField;
        }

        fieldOffset = (int) unsafe.objectFieldOffset(oneofField);

        final java.lang.reflect.Field oneofCaseField;
        index++;
        o = messageInfoObjects[index];
        if (o instanceof java.lang.reflect.Field) {
          oneofCaseField = (java.lang.reflect.Field) o;
        } else {
          oneofCaseField = reflectField(messageClass, (String) o);
          messageInfoObjects[index] = oneofCaseField;
        }

        presenceFieldOffset = (int) unsafe.objectFieldOffset(oneofCaseField);
        presenceMaskShift = 0;
      } else {
        Field field = reflectField(messageClass, (String) messageInfoObjects[objectsPosition++]);
        if (fieldType == 9 /* FieldType.MESSAGE */ || fieldType == 17 /* FieldType.GROUP */) {
          objects[bufferIndex / INTS_PER_FIELD * 2 + 1] = field.getType();
        } else if (fieldType == 27 /* FieldType.MESSAGE_LIST */
            || fieldType == 49 /* FieldType.GROUP_LIST */) {
          objects[bufferIndex / INTS_PER_FIELD * 2 + 1] = messageInfoObjects[objectsPosition++];
        } else if (fieldType == 12 /* FieldType.ENUM */
            || fieldType == 30 /* FieldType.ENUM_LIST */
            || fieldType == 44 /* FieldType.ENUM_LIST_PACKED */) {
          if ((flags & 0x1) == 0x1) {
            objects[bufferIndex / INTS_PER_FIELD * 2 + 1] = messageInfoObjects[objectsPosition++];
          }
        } else if (fieldType == 50 /* FieldType.MAP */) {
          intArray[mapFieldIndex++] = bufferIndex;
          objects[bufferIndex / INTS_PER_FIELD * 2] = messageInfoObjects[objectsPosition++];
          if ((fieldTypeWithExtraBits & 0x800) != 0) {
            objects[bufferIndex / INTS_PER_FIELD * 2 + 1] = messageInfoObjects[objectsPosition++];
          }
        }

        fieldOffset = (int) unsafe.objectFieldOffset(field);
        if ((flags & 0x1) == 0x1 && fieldType <= 17 /* FieldType.GROUP */) {
          next = info.charAt(i++);
          if (next >= 0xD800) {
            int result = next & 0x1FFF;
            int shift = 13;
            while ((next = info.charAt(i++)) >= 0xD800) {
              result |= (next & 0x1FFF) << shift;
              shift += 13;
            }
            next = result | (next << shift);
          }
          int hasBitsIndex = next;

          final java.lang.reflect.Field hasBitsField;
          int index = oneofCount * 2 + hasBitsIndex / 32;
          Object o = messageInfoObjects[index];
          if (o instanceof java.lang.reflect.Field) {
            hasBitsField = (java.lang.reflect.Field) o;
          } else {
            hasBitsField = reflectField(messageClass, (String) o);
            messageInfoObjects[index] = hasBitsField;
          }

          presenceFieldOffset = (int) unsafe.objectFieldOffset(hasBitsField);
          presenceMaskShift = hasBitsIndex % 32;
        } else {
          presenceFieldOffset = 0;
          presenceMaskShift = 0;
        }

        if (fieldType >= 18 && fieldType <= 49) {
          // Field types of repeated fields are in a consecutive range from 18 (DOUBLE_LIST) to
          // 49 (GROUP_LIST).
          intArray[repeatedFieldIndex++] = fieldOffset;
        }
      }

      buffer[bufferIndex++] = fieldNumber;
      buffer[bufferIndex++] =
          ((fieldTypeWithExtraBits & 0x200) != 0 ? ENFORCE_UTF8_MASK : 0)
              | ((fieldTypeWithExtraBits & 0x100) != 0 ? REQUIRED_MASK : 0)
              | (fieldType << OFFSET_BITS)
              | fieldOffset;
      buffer[bufferIndex++] = (presenceMaskShift << OFFSET_BITS) | presenceFieldOffset;
    }

    return new MessageSchema<T>(
        buffer,
        objects,
        minFieldNumber,
        maxFieldNumber,
        messageInfo.getDefaultInstance(),
        isProto3,
        /* useCachedSizeField= */ false,
        intArray,
        checkInitialized,
        checkInitialized + mapFieldCount,
        newInstanceSchema,
        listFieldSchema,
        unknownFieldSchema,
        extensionSchema,
        mapFieldSchema);
  }

  private static java.lang.reflect.Field reflectField(Class<?> messageClass, String fieldName) {
    try {
      return messageClass.getDeclaredField(fieldName);
    } catch (NoSuchFieldException e) {
      // Some Samsung devices lie about what fields are present via the getDeclaredField API so
      // we do the for loop properly that they seem to have messed up...
      java.lang.reflect.Field[] fields = messageClass.getDeclaredFields();
      for (java.lang.reflect.Field field : fields) {
        if (fieldName.equals(field.getName())) {
          return field;
        }
      }

      // If we make it here, the runtime still lies about what we know to be true at compile
      // time. We throw to alert server monitoring for further remediation.
      throw new RuntimeException(
          "Field "
              + fieldName
              + " for "
              + messageClass.getName()
              + " not found. Known fields are "
              + Arrays.toString(fields));
    }
  }

  static <T> MessageSchema<T> newSchemaForMessageInfo(
      StructuralMessageInfo messageInfo,
      NewInstanceSchema newInstanceSchema,
      ListFieldSchema listFieldSchema,
      UnknownFieldSchema<?, ?> unknownFieldSchema,
      ExtensionSchema<?> extensionSchema,
      MapFieldSchema mapFieldSchema) {
    final boolean isProto3 = messageInfo.getSyntax() == ProtoSyntax.PROTO3;
    FieldInfo[] fis = messageInfo.getFields();
    final int minFieldNumber;
    final int maxFieldNumber;
    if (fis.length == 0) {
      minFieldNumber = 0;
      maxFieldNumber = 0;
    } else {
      minFieldNumber = fis[0].getFieldNumber();
      maxFieldNumber = fis[fis.length - 1].getFieldNumber();
    }

    final int numEntries = fis.length;

    int[] buffer = new int[numEntries * INTS_PER_FIELD];
    Object[] objects = new Object[numEntries * 2];

    int mapFieldCount = 0;
    int repeatedFieldCount = 0;
    for (FieldInfo fi : fis) {
      if (fi.getType() == FieldType.MAP) {
        mapFieldCount++;
      } else if (fi.getType().id() >= 18 && fi.getType().id() <= 49) {
        // Field types of repeated fields are in a consecutive range from 18 (DOUBLE_LIST) to
        // 49 (GROUP_LIST).
        repeatedFieldCount++;
      }
    }
    int[] mapFieldPositions = mapFieldCount > 0 ? new int[mapFieldCount] : null;
    int[] repeatedFieldOffsets = repeatedFieldCount > 0 ? new int[repeatedFieldCount] : null;
    mapFieldCount = 0;
    repeatedFieldCount = 0;

    int[] checkInitialized = messageInfo.getCheckInitialized();
    if (checkInitialized == null) {
      checkInitialized = EMPTY_INT_ARRAY;
    }
    int checkInitializedIndex = 0;
    // Fill in the manifest data from the descriptors.
    int fieldIndex = 0;
    for (int bufferIndex = 0; fieldIndex < fis.length; bufferIndex += INTS_PER_FIELD) {
      final FieldInfo fi = fis[fieldIndex];
      final int fieldNumber = fi.getFieldNumber();

      // We found the entry for the next field. Store the entry in the manifest for
      // this field and increment the field index.
      storeFieldData(fi, buffer, bufferIndex, isProto3, objects);

      // Convert field number to index
      if (checkInitializedIndex < checkInitialized.length
          && checkInitialized[checkInitializedIndex] == fieldNumber) {
        checkInitialized[checkInitializedIndex++] = bufferIndex;
      }

      if (fi.getType() == FieldType.MAP) {
        mapFieldPositions[mapFieldCount++] = bufferIndex;
      } else if (fi.getType().id() >= 18 && fi.getType().id() <= 49) {
        // Field types of repeated fields are in a consecutive range from 18 (DOUBLE_LIST) to
        // 49 (GROUP_LIST).
        repeatedFieldOffsets[repeatedFieldCount++] =
            (int) UnsafeUtil.objectFieldOffset(fi.getField());
      }

      fieldIndex++;
    }

    if (mapFieldPositions == null) {
      mapFieldPositions = EMPTY_INT_ARRAY;
    }
    if (repeatedFieldOffsets == null) {
      repeatedFieldOffsets = EMPTY_INT_ARRAY;
    }
    int[] combined =
        new int[checkInitialized.length + mapFieldPositions.length + repeatedFieldOffsets.length];
    System.arraycopy(checkInitialized, 0, combined, 0, checkInitialized.length);
    System.arraycopy(
        mapFieldPositions, 0, combined, checkInitialized.length, mapFieldPositions.length);
    System.arraycopy(
        repeatedFieldOffsets,
        0,
        combined,
        checkInitialized.length + mapFieldPositions.length,
        repeatedFieldOffsets.length);

    return new MessageSchema<T>(
        buffer,
        objects,
        minFieldNumber,
        maxFieldNumber,
        messageInfo.getDefaultInstance(),
        isProto3,
        /* useCachedSizeField= */ true,
        combined,
        checkInitialized.length,
        checkInitialized.length + mapFieldPositions.length,
        newInstanceSchema,
        listFieldSchema,
        unknownFieldSchema,
        extensionSchema,
        mapFieldSchema);
  }

  private static void storeFieldData(
      FieldInfo fi, int[] buffer, int bufferIndex, boolean proto3, Object[] objects) {
    final int fieldOffset;
    final int typeId;
    final int presenceMaskShift;
    final int presenceFieldOffset;

    OneofInfo oneof = fi.getOneof();
    if (oneof != null) {
      typeId = fi.getType().id() + ONEOF_TYPE_OFFSET;
      fieldOffset = (int) UnsafeUtil.objectFieldOffset(oneof.getValueField());
      presenceFieldOffset = (int) UnsafeUtil.objectFieldOffset(oneof.getCaseField());
      presenceMaskShift = 0;
    } else {
      FieldType type = fi.getType();
      fieldOffset = (int) UnsafeUtil.objectFieldOffset(fi.getField());
      typeId = type.id();
      if (!proto3 && !type.isList() && !type.isMap()) {
        presenceFieldOffset = (int) UnsafeUtil.objectFieldOffset(fi.getPresenceField());
        presenceMaskShift = Integer.numberOfTrailingZeros(fi.getPresenceMask());
      } else {
        if (fi.getCachedSizeField() == null) {
          presenceFieldOffset = 0;
          presenceMaskShift = 0;
        } else {
          presenceFieldOffset = (int) UnsafeUtil.objectFieldOffset(fi.getCachedSizeField());
          presenceMaskShift = 0;
        }
      }
    }

    buffer[bufferIndex] = fi.getFieldNumber();
    buffer[bufferIndex + 1] =
        (fi.isEnforceUtf8() ? ENFORCE_UTF8_MASK : 0)
            | (fi.isRequired() ? REQUIRED_MASK : 0)
            | (typeId << OFFSET_BITS)
            | fieldOffset;
    buffer[bufferIndex + 2] = (presenceMaskShift << OFFSET_BITS) | presenceFieldOffset;

    Object messageFieldClass = fi.getMessageFieldClass();
    if (fi.getMapDefaultEntry() != null) {
      objects[bufferIndex / INTS_PER_FIELD * 2] = fi.getMapDefaultEntry();
      if (messageFieldClass != null) {
        objects[bufferIndex / INTS_PER_FIELD * 2 + 1] = messageFieldClass;
      } else if (fi.getEnumVerifier() != null) {
        objects[bufferIndex / INTS_PER_FIELD * 2 + 1] = fi.getEnumVerifier();
      }
    } else {
      if (messageFieldClass != null) {
        objects[bufferIndex / INTS_PER_FIELD * 2 + 1] = messageFieldClass;
      } else if (fi.getEnumVerifier() != null) {
        objects[bufferIndex / INTS_PER_FIELD * 2 + 1] = fi.getEnumVerifier();
      }
    }
  }

  @SuppressWarnings("unchecked")
  @Override
  public T newInstance() {
    return (T) newInstanceSchema.newInstance(defaultInstance);
  }

  @Override
  public boolean equals(T message, T other) {
    final int bufferLength = buffer.length;
    for (int pos = 0; pos < bufferLength; pos += INTS_PER_FIELD) {
      if (!equals(message, other, pos)) {
        return false;
      }
    }

    Object messageUnknown = unknownFieldSchema.getFromMessage(message);
    Object otherUnknown = unknownFieldSchema.getFromMessage(other);
    if (!messageUnknown.equals(otherUnknown)) {
      return false;
    }

    if (hasExtensions) {
      FieldSet<?> messageExtensions = extensionSchema.getExtensions(message);
      FieldSet<?> otherExtensions = extensionSchema.getExtensions(other);
      return messageExtensions.equals(otherExtensions);
    }
    return true;
  }

  private boolean equals(T message, T other, int pos) {
    final int typeAndOffset = typeAndOffsetAt(pos);
    final long offset = offset(typeAndOffset);

    switch (type(typeAndOffset)) {
      case 0: // DOUBLE:
        return arePresentForEquals(message, other, pos)
            && Double.doubleToLongBits(UnsafeUtil.getDouble(message, offset))
                == Double.doubleToLongBits(UnsafeUtil.getDouble(other, offset));
      case 1: // FLOAT:
        return arePresentForEquals(message, other, pos)
            && Float.floatToIntBits(UnsafeUtil.getFloat(message, offset))
                == Float.floatToIntBits(UnsafeUtil.getFloat(other, offset));
      case 2: // INT64:
        return arePresentForEquals(message, other, pos)
            && UnsafeUtil.getLong(message, offset) == UnsafeUtil.getLong(other, offset);
      case 3: // UINT64:
        return arePresentForEquals(message, other, pos)
            && UnsafeUtil.getLong(message, offset) == UnsafeUtil.getLong(other, offset);
      case 4: // INT32:
        return arePresentForEquals(message, other, pos)
            && UnsafeUtil.getInt(message, offset) == UnsafeUtil.getInt(other, offset);
      case 5: // FIXED64:
        return arePresentForEquals(message, other, pos)
            && UnsafeUtil.getLong(message, offset) == UnsafeUtil.getLong(other, offset);
      case 6: // FIXED32:
        return arePresentForEquals(message, other, pos)
            && UnsafeUtil.getInt(message, offset) == UnsafeUtil.getInt(other, offset);
      case 7: // BOOL:
        return arePresentForEquals(message, other, pos)
            && UnsafeUtil.getBoolean(message, offset) == UnsafeUtil.getBoolean(other, offset);
      case 8: // STRING:
        return arePresentForEquals(message, other, pos)
            && SchemaUtil.safeEquals(
                UnsafeUtil.getObject(message, offset), UnsafeUtil.getObject(other, offset));
      case 9: // MESSAGE:
        return arePresentForEquals(message, other, pos)
            && SchemaUtil.safeEquals(
                UnsafeUtil.getObject(message, offset), UnsafeUtil.getObject(other, offset));
      case 10: // BYTES:
        return arePresentForEquals(message, other, pos)
            && SchemaUtil.safeEquals(
                UnsafeUtil.getObject(message, offset), UnsafeUtil.getObject(other, offset));
      case 11: // UINT32:
        return arePresentForEquals(message, other, pos)
            && UnsafeUtil.getInt(message, offset) == UnsafeUtil.getInt(other, offset);
      case 12: // ENUM:
        return arePresentForEquals(message, other, pos)
            && UnsafeUtil.getInt(message, offset) == UnsafeUtil.getInt(other, offset);
      case 13: // SFIXED32:
        return arePresentForEquals(message, other, pos)
            && UnsafeUtil.getInt(message, offset) == UnsafeUtil.getInt(other, offset);
      case 14: // SFIXED64:
        return arePresentForEquals(message, other, pos)
            && UnsafeUtil.getLong(message, offset) == UnsafeUtil.getLong(other, offset);
      case 15: // SINT32:
        return arePresentForEquals(message, other, pos)
            && UnsafeUtil.getInt(message, offset) == UnsafeUtil.getInt(other, offset);
      case 16: // SINT64:
        return arePresentForEquals(message, other, pos)
            && UnsafeUtil.getLong(message, offset) == UnsafeUtil.getLong(other, offset);
      case 17: // GROUP:
        return arePresentForEquals(message, other, pos)
            && SchemaUtil.safeEquals(
                UnsafeUtil.getObject(message, offset), UnsafeUtil.getObject(other, offset));

      case 18: // DOUBLE_LIST:
      case 19: // FLOAT_LIST:
      case 20: // INT64_LIST:
      case 21: // UINT64_LIST:
      case 22: // INT32_LIST:
      case 23: // FIXED64_LIST:
      case 24: // FIXED32_LIST:
      case 25: // BOOL_LIST:
      case 26: // STRING_LIST:
      case 27: // MESSAGE_LIST:
      case 28: // BYTES_LIST:
      case 29: // UINT32_LIST:
      case 30: // ENUM_LIST:
      case 31: // SFIXED32_LIST:
      case 32: // SFIXED64_LIST:
      case 33: // SINT32_LIST:
      case 34: // SINT64_LIST:
      case 35: // DOUBLE_LIST_PACKED:
      case 36: // FLOAT_LIST_PACKED:
      case 37: // INT64_LIST_PACKED:
      case 38: // UINT64_LIST_PACKED:
      case 39: // INT32_LIST_PACKED:
      case 40: // FIXED64_LIST_PACKED:
      case 41: // FIXED32_LIST_PACKED:
      case 42: // BOOL_LIST_PACKED:
      case 43: // UINT32_LIST_PACKED:
      case 44: // ENUM_LIST_PACKED:
      case 45: // SFIXED32_LIST_PACKED:
      case 46: // SFIXED64_LIST_PACKED:
      case 47: // SINT32_LIST_PACKED:
      case 48: // SINT64_LIST_PACKED:
      case 49: // GROUP_LIST:
        return SchemaUtil.safeEquals(
            UnsafeUtil.getObject(message, offset), UnsafeUtil.getObject(other, offset));
      case 50: // MAP:
        return SchemaUtil.safeEquals(
            UnsafeUtil.getObject(message, offset), UnsafeUtil.getObject(other, offset));
      case 51: // ONEOF_DOUBLE:
      case 52: // ONEOF_FLOAT:
      case 53: // ONEOF_INT64:
      case 54: // ONEOF_UINT64:
      case 55: // ONEOF_INT32:
      case 56: // ONEOF_FIXED64:
      case 57: // ONEOF_FIXED32:
      case 58: // ONEOF_BOOL:
      case 59: // ONEOF_STRING:
      case 60: // ONEOF_MESSAGE:
      case 61: // ONEOF_BYTES:
      case 62: // ONEOF_UINT32:
      case 63: // ONEOF_ENUM:
      case 64: // ONEOF_SFIXED32:
      case 65: // ONEOF_SFIXED64:
      case 66: // ONEOF_SINT32:
      case 67: // ONEOF_SINT64:
      case 68: // ONEOF_GROUP:
        return isOneofCaseEqual(message, other, pos)
            && SchemaUtil.safeEquals(
                UnsafeUtil.getObject(message, offset), UnsafeUtil.getObject(other, offset));
      default:
        // Assume it's an empty entry - just go to the next entry.
        return true;
    }
  }

  @Override
  public int hashCode(T message) {
    int hashCode = 0;
    final int bufferLength = buffer.length;
    for (int pos = 0; pos < bufferLength; pos += INTS_PER_FIELD) {
      final int typeAndOffset = typeAndOffsetAt(pos);
      final int entryNumber = numberAt(pos);

      final long offset = offset(typeAndOffset);

      switch (type(typeAndOffset)) {
        case 0: // DOUBLE:
          hashCode =
              (hashCode * 53)
                  + Internal.hashLong(
                      Double.doubleToLongBits(UnsafeUtil.getDouble(message, offset)));
          break;
        case 1: // FLOAT:
          hashCode = (hashCode * 53) + Float.floatToIntBits(UnsafeUtil.getFloat(message, offset));
          break;
        case 2: // INT64:
          hashCode = (hashCode * 53) + Internal.hashLong(UnsafeUtil.getLong(message, offset));
          break;
        case 3: // UINT64:
          hashCode = (hashCode * 53) + Internal.hashLong(UnsafeUtil.getLong(message, offset));
          break;
        case 4: // INT32:
          hashCode = (hashCode * 53) + (UnsafeUtil.getInt(message, offset));
          break;
        case 5: // FIXED64:
          hashCode = (hashCode * 53) + Internal.hashLong(UnsafeUtil.getLong(message, offset));
          break;
        case 6: // FIXED32:
          hashCode = (hashCode * 53) + (UnsafeUtil.getInt(message, offset));
          break;
        case 7: // BOOL:
          hashCode = (hashCode * 53) + Internal.hashBoolean(UnsafeUtil.getBoolean(message, offset));
          break;
        case 8: // STRING:
          hashCode = (hashCode * 53) + ((String) UnsafeUtil.getObject(message, offset)).hashCode();
          break;
        case 9: // MESSAGE:
          {
            int protoHash = 37;
            Object submessage = UnsafeUtil.getObject(message, offset);
            if (submessage != null) {
              protoHash = submessage.hashCode();
            }
            hashCode = (53 * hashCode) + protoHash;
            break;
          }
        case 10: // BYTES:
          hashCode = (hashCode * 53) + UnsafeUtil.getObject(message, offset).hashCode();
          break;
        case 11: // UINT32:
          hashCode = (hashCode * 53) + (UnsafeUtil.getInt(message, offset));
          break;
        case 12: // ENUM:
          hashCode = (hashCode * 53) + (UnsafeUtil.getInt(message, offset));
          break;
        case 13: // SFIXED32:
          hashCode = (hashCode * 53) + (UnsafeUtil.getInt(message, offset));
          break;
        case 14: // SFIXED64:
          hashCode = (hashCode * 53) + Internal.hashLong(UnsafeUtil.getLong(message, offset));
          break;
        case 15: // SINT32:
          hashCode = (hashCode * 53) + (UnsafeUtil.getInt(message, offset));
          break;
        case 16: // SINT64:
          hashCode = (hashCode * 53) + Internal.hashLong(UnsafeUtil.getLong(message, offset));
          break;

        case 17: // GROUP:
          {
            int protoHash = 37;
            Object submessage = UnsafeUtil.getObject(message, offset);
            if (submessage != null) {
              protoHash = submessage.hashCode();
            }
            hashCode = (53 * hashCode) + protoHash;
            break;
          }
        case 18: // DOUBLE_LIST:
        case 19: // FLOAT_LIST:
        case 20: // INT64_LIST:
        case 21: // UINT64_LIST:
        case 22: // INT32_LIST:
        case 23: // FIXED64_LIST:
        case 24: // FIXED32_LIST:
        case 25: // BOOL_LIST:
        case 26: // STRING_LIST:
        case 27: // MESSAGE_LIST:
        case 28: // BYTES_LIST:
        case 29: // UINT32_LIST:
        case 30: // ENUM_LIST:
        case 31: // SFIXED32_LIST:
        case 32: // SFIXED64_LIST:
        case 33: // SINT32_LIST:
        case 34: // SINT64_LIST:
        case 35: // DOUBLE_LIST_PACKED:
        case 36: // FLOAT_LIST_PACKED:
        case 37: // INT64_LIST_PACKED:
        case 38: // UINT64_LIST_PACKED:
        case 39: // INT32_LIST_PACKED:
        case 40: // FIXED64_LIST_PACKED:
        case 41: // FIXED32_LIST_PACKED:
        case 42: // BOOL_LIST_PACKED:
        case 43: // UINT32_LIST_PACKED:
        case 44: // ENUM_LIST_PACKED:
        case 45: // SFIXED32_LIST_PACKED:
        case 46: // SFIXED64_LIST_PACKED:
        case 47: // SINT32_LIST_PACKED:
        case 48: // SINT64_LIST_PACKED:
        case 49: // GROUP_LIST:
          hashCode = (hashCode * 53) + UnsafeUtil.getObject(message, offset).hashCode();
          break;
        case 50: // MAP:
          hashCode = (hashCode * 53) + UnsafeUtil.getObject(message, offset).hashCode();
          break;
        case 51: // ONEOF_DOUBLE:
          if (isOneofPresent(message, entryNumber, pos)) {
            hashCode =
                (hashCode * 53)
                    + Internal.hashLong(Double.doubleToLongBits(oneofDoubleAt(message, offset)));
          }
          break;
        case 52: // ONEOF_FLOAT:
          if (isOneofPresent(message, entryNumber, pos)) {
            hashCode = (hashCode * 53) + Float.floatToIntBits(oneofFloatAt(message, offset));
          }
          break;
        case 53: // ONEOF_INT64:
          if (isOneofPresent(message, entryNumber, pos)) {
            hashCode = (hashCode * 53) + Internal.hashLong(oneofLongAt(message, offset));
          }
          break;
        case 54: // ONEOF_UINT64:
          if (isOneofPresent(message, entryNumber, pos)) {
            hashCode = (hashCode * 53) + Internal.hashLong(oneofLongAt(message, offset));
          }
          break;
        case 55: // ONEOF_INT32:
          if (isOneofPresent(message, entryNumber, pos)) {
            hashCode = (hashCode * 53) + (oneofIntAt(message, offset));
          }
          break;
        case 56: // ONEOF_FIXED64:
          if (isOneofPresent(message, entryNumber, pos)) {
            hashCode = (hashCode * 53) + Internal.hashLong(oneofLongAt(message, offset));
          }
          break;
        case 57: // ONEOF_FIXED32:
          if (isOneofPresent(message, entryNumber, pos)) {
            hashCode = (hashCode * 53) + (oneofIntAt(message, offset));
          }
          break;
        case 58: // ONEOF_BOOL:
          if (isOneofPresent(message, entryNumber, pos)) {
            hashCode = (hashCode * 53) + Internal.hashBoolean(oneofBooleanAt(message, offset));
          }
          break;
        case 59: // ONEOF_STRING:
          if (isOneofPresent(message, entryNumber, pos)) {
            hashCode =
                (hashCode * 53) + ((String) UnsafeUtil.getObject(message, offset)).hashCode();
          }
          break;
        case 60: // ONEOF_MESSAGE:
          if (isOneofPresent(message, entryNumber, pos)) {
            Object submessage = UnsafeUtil.getObject(message, offset);
            hashCode = (53 * hashCode) + submessage.hashCode();
          }
          break;
        case 61: // ONEOF_BYTES:
          if (isOneofPresent(message, entryNumber, pos)) {
            hashCode = (hashCode * 53) + UnsafeUtil.getObject(message, offset).hashCode();
          }
          break;
        case 62: // ONEOF_UINT32:
          if (isOneofPresent(message, entryNumber, pos)) {
            hashCode = (hashCode * 53) + (oneofIntAt(message, offset));
          }
          break;
        case 63: // ONEOF_ENUM:
          if (isOneofPresent(message, entryNumber, pos)) {
            hashCode = (hashCode * 53) + (oneofIntAt(message, offset));
          }
          break;
        case 64: // ONEOF_SFIXED32:
          if (isOneofPresent(message, entryNumber, pos)) {
            hashCode = (hashCode * 53) + (oneofIntAt(message, offset));
          }
          break;
        case 65: // ONEOF_SFIXED64:
          if (isOneofPresent(message, entryNumber, pos)) {
            hashCode = (hashCode * 53) + Internal.hashLong(oneofLongAt(message, offset));
          }
          break;
        case 66: // ONEOF_SINT32:
          if (isOneofPresent(message, entryNumber, pos)) {
            hashCode = (hashCode * 53) + (oneofIntAt(message, offset));
          }
          break;
        case 67: // ONEOF_SINT64:
          if (isOneofPresent(message, entryNumber, pos)) {
            hashCode = (hashCode * 53) + Internal.hashLong(oneofLongAt(message, offset));
          }
          break;
        case 68: // ONEOF_GROUP:
          if (isOneofPresent(message, entryNumber, pos)) {
            Object submessage = UnsafeUtil.getObject(message, offset);
            hashCode = (53 * hashCode) + submessage.hashCode();
          }
          break;
        default:
          // Assume it's an empty entry - just go to the next entry.
          break;
      }
    }

    hashCode = (hashCode * 53) + unknownFieldSchema.getFromMessage(message).hashCode();

    if (hasExtensions) {
      hashCode = (hashCode * 53) + extensionSchema.getExtensions(message).hashCode();
    }

    return hashCode;
  }

  @Override
  public void mergeFrom(T message, T other) {
    if (other == null) {
      throw new NullPointerException();
    }
    for (int i = 0; i < buffer.length; i += INTS_PER_FIELD) {
      // A separate method allows for better JIT optimizations
      mergeSingleField(message, other, i);
    }

    if (!proto3) {
      SchemaUtil.mergeUnknownFields(unknownFieldSchema, message, other);

      if (hasExtensions) {
        SchemaUtil.mergeExtensions(extensionSchema, message, other);
      }
    }
  }

  private void mergeSingleField(T message, T other, int pos) {
    final int typeAndOffset = typeAndOffsetAt(pos);
    final long offset = offset(typeAndOffset);
    final int number = numberAt(pos);

    switch (type(typeAndOffset)) {
      case 0: // DOUBLE:
        if (isFieldPresent(other, pos)) {
          UnsafeUtil.putDouble(message, offset, UnsafeUtil.getDouble(other, offset));
          setFieldPresent(message, pos);
        }
        break;
      case 1: // FLOAT:
        if (isFieldPresent(other, pos)) {
          UnsafeUtil.putFloat(message, offset, UnsafeUtil.getFloat(other, offset));
          setFieldPresent(message, pos);
        }
        break;
      case 2: // INT64:
        if (isFieldPresent(other, pos)) {
          UnsafeUtil.putLong(message, offset, UnsafeUtil.getLong(other, offset));
          setFieldPresent(message, pos);
        }
        break;
      case 3: // UINT64:
        if (isFieldPresent(other, pos)) {
          UnsafeUtil.putLong(message, offset, UnsafeUtil.getLong(other, offset));
          setFieldPresent(message, pos);
        }
        break;
      case 4: // INT32:
        if (isFieldPresent(other, pos)) {
          UnsafeUtil.putInt(message, offset, UnsafeUtil.getInt(other, offset));
          setFieldPresent(message, pos);
        }
        break;
      case 5: // FIXED64:
        if (isFieldPresent(other, pos)) {
          UnsafeUtil.putLong(message, offset, UnsafeUtil.getLong(other, offset));
          setFieldPresent(message, pos);
        }
        break;
      case 6: // FIXED32:
        if (isFieldPresent(other, pos)) {
          UnsafeUtil.putInt(message, offset, UnsafeUtil.getInt(other, offset));
          setFieldPresent(message, pos);
        }
        break;
      case 7: // BOOL:
        if (isFieldPresent(other, pos)) {
          UnsafeUtil.putBoolean(message, offset, UnsafeUtil.getBoolean(other, offset));
          setFieldPresent(message, pos);
        }
        break;
      case 8: // STRING:
        if (isFieldPresent(other, pos)) {
          UnsafeUtil.putObject(message, offset, UnsafeUtil.getObject(other, offset));
          setFieldPresent(message, pos);
        }
        break;
      case 9: // MESSAGE:
        mergeMessage(message, other, pos);
        break;
      case 10: // BYTES:
        if (isFieldPresent(other, pos)) {
          UnsafeUtil.putObject(message, offset, UnsafeUtil.getObject(other, offset));
          setFieldPresent(message, pos);
        }
        break;
      case 11: // UINT32:
        if (isFieldPresent(other, pos)) {
          UnsafeUtil.putInt(message, offset, UnsafeUtil.getInt(other, offset));
          setFieldPresent(message, pos);
        }
        break;
      case 12: // ENUM:
        if (isFieldPresent(other, pos)) {
          UnsafeUtil.putInt(message, offset, UnsafeUtil.getInt(other, offset));
          setFieldPresent(message, pos);
        }
        break;
      case 13: // SFIXED32:
        if (isFieldPresent(other, pos)) {
          UnsafeUtil.putInt(message, offset, UnsafeUtil.getInt(other, offset));
          setFieldPresent(message, pos);
        }
        break;
      case 14: // SFIXED64:
        if (isFieldPresent(other, pos)) {
          UnsafeUtil.putLong(message, offset, UnsafeUtil.getLong(other, offset));
          setFieldPresent(message, pos);
        }
        break;
      case 15: // SINT32:
        if (isFieldPresent(other, pos)) {
          UnsafeUtil.putInt(message, offset, UnsafeUtil.getInt(other, offset));
          setFieldPresent(message, pos);
        }
        break;
      case 16: // SINT64:
        if (isFieldPresent(other, pos)) {
          UnsafeUtil.putLong(message, offset, UnsafeUtil.getLong(other, offset));
          setFieldPresent(message, pos);
        }
        break;
      case 17: // GROUP:
        mergeMessage(message, other, pos);
        break;
      case 18: // DOUBLE_LIST:
      case 19: // FLOAT_LIST:
      case 20: // INT64_LIST:
      case 21: // UINT64_LIST:
      case 22: // INT32_LIST:
      case 23: // FIXED64_LIST:
      case 24: // FIXED32_LIST:
      case 25: // BOOL_LIST:
      case 26: // STRING_LIST:
      case 27: // MESSAGE_LIST:
      case 28: // BYTES_LIST:
      case 29: // UINT32_LIST:
      case 30: // ENUM_LIST:
      case 31: // SFIXED32_LIST:
      case 32: // SFIXED64_LIST:
      case 33: // SINT32_LIST:
      case 34: // SINT64_LIST:
      case 35: // DOUBLE_LIST_PACKED:
      case 36: // FLOAT_LIST_PACKED:
      case 37: // INT64_LIST_PACKED:
      case 38: // UINT64_LIST_PACKED:
      case 39: // INT32_LIST_PACKED:
      case 40: // FIXED64_LIST_PACKED:
      case 41: // FIXED32_LIST_PACKED:
      case 42: // BOOL_LIST_PACKED:
      case 43: // UINT32_LIST_PACKED:
      case 44: // ENUM_LIST_PACKED:
      case 45: // SFIXED32_LIST_PACKED:
      case 46: // SFIXED64_LIST_PACKED:
      case 47: // SINT32_LIST_PACKED:
      case 48: // SINT64_LIST_PACKED:
      case 49: // GROUP_LIST:
        listFieldSchema.mergeListsAt(message, other, offset);
        break;
      case 50: // MAP:
        SchemaUtil.mergeMap(mapFieldSchema, message, other, offset);
        break;
      case 51: // ONEOF_DOUBLE:
      case 52: // ONEOF_FLOAT:
      case 53: // ONEOF_INT64:
      case 54: // ONEOF_UINT64:
      case 55: // ONEOF_INT32:
      case 56: // ONEOF_FIXED64:
      case 57: // ONEOF_FIXED32:
      case 58: // ONEOF_BOOL:
      case 59: // ONEOF_STRING:
        if (isOneofPresent(other, number, pos)) {
          UnsafeUtil.putObject(message, offset, UnsafeUtil.getObject(other, offset));
          setOneofPresent(message, number, pos);
        }
        break;

      case 60: // ONEOF_MESSAGE:
        mergeOneofMessage(message, other, pos);
        break;
      case 61: // ONEOF_BYTES:
      case 62: // ONEOF_UINT32:
      case 63: // ONEOF_ENUM:
      case 64: // ONEOF_SFIXED32:
      case 65: // ONEOF_SFIXED64:
      case 66: // ONEOF_SINT32:
      case 67: // ONEOF_SINT64:
        if (isOneofPresent(other, number, pos)) {
          UnsafeUtil.putObject(message, offset, UnsafeUtil.getObject(other, offset));
          setOneofPresent(message, number, pos);
        }
        break;
      case 68: // ONEOF_GROUP:
        mergeOneofMessage(message, other, pos);
        break;
      default:
        break;
    }
  }

  private void mergeMessage(T message, T other, int pos) {
    final int typeAndOffset = typeAndOffsetAt(pos);
    final long offset = offset(typeAndOffset);

    if (!isFieldPresent(other, pos)) {
      return;
    }

    Object mine = UnsafeUtil.getObject(message, offset);
    Object theirs = UnsafeUtil.getObject(other, offset);
    if (mine != null && theirs != null) {
      Object merged = Internal.mergeMessage(mine, theirs);
      UnsafeUtil.putObject(message, offset, merged);
      setFieldPresent(message, pos);
    } else if (theirs != null) {
      UnsafeUtil.putObject(message, offset, theirs);
      setFieldPresent(message, pos);
    }
  }

  private void mergeOneofMessage(T message, T other, int pos) {
    int typeAndOffset = typeAndOffsetAt(pos);
    int number = numberAt(pos);
    long offset = offset(typeAndOffset);

    if (!isOneofPresent(other, number, pos)) {
      return;
    }

    Object mine = UnsafeUtil.getObject(message, offset);
    Object theirs = UnsafeUtil.getObject(other, offset);
    if (mine != null && theirs != null) {
      Object merged = Internal.mergeMessage(mine, theirs);
      UnsafeUtil.putObject(message, offset, merged);
      setOneofPresent(message, number, pos);
    } else if (theirs != null) {
      UnsafeUtil.putObject(message, offset, theirs);
      setOneofPresent(message, number, pos);
    }
  }

  @Override
  public int getSerializedSize(T message) {
    return proto3 ? getSerializedSizeProto3(message) : getSerializedSizeProto2(message);
  }
  
  @SuppressWarnings("unchecked")
  private int getSerializedSizeProto2(T message) {
    int size = 0;

    final sun.misc.Unsafe unsafe = UNSAFE;
    int currentPresenceFieldOffset = -1;
    int currentPresenceField = 0;
    for (int i = 0; i < buffer.length; i += INTS_PER_FIELD) {
      final int typeAndOffset = typeAndOffsetAt(i);
      final int number = numberAt(i);

      int fieldType = type(typeAndOffset);
      int presenceMaskAndOffset = 0;
      int presenceMask = 0;
      if (fieldType <= 17) {
        presenceMaskAndOffset = buffer[i + 2];
        final int presenceFieldOffset = presenceMaskAndOffset & OFFSET_MASK;
        presenceMask = 1 << (presenceMaskAndOffset >>> OFFSET_BITS);
        if (presenceFieldOffset != currentPresenceFieldOffset) {
          currentPresenceFieldOffset = presenceFieldOffset;
          currentPresenceField = unsafe.getInt(message, (long) presenceFieldOffset);
        }
      } else if (useCachedSizeField
          && fieldType >= FieldType.DOUBLE_LIST_PACKED.id()
          && fieldType <= FieldType.SINT64_LIST_PACKED.id()) {
        presenceMaskAndOffset = buffer[i + 2] & OFFSET_MASK;
      }

      final long offset = offset(typeAndOffset);

      switch (fieldType) {
        case 0: // DOUBLE:
          if ((currentPresenceField & presenceMask) != 0) {
            size += CodedOutputStream.computeDoubleSize(number, 0);
          }
          break;
        case 1: // FLOAT:
          if ((currentPresenceField & presenceMask) != 0) {
            size += CodedOutputStream.computeFloatSize(number, 0);
          }
          break;
        case 2: // INT64:
          if ((currentPresenceField & presenceMask) != 0) {
            size += CodedOutputStream.computeInt64Size(number, unsafe.getLong(message, offset));
          }
          break;
        case 3: // UINT64:
          if ((currentPresenceField & presenceMask) != 0) {
            size += CodedOutputStream.computeUInt64Size(number, unsafe.getLong(message, offset));
          }
          break;
        case 4: // INT32:
          if ((currentPresenceField & presenceMask) != 0) {
            size += CodedOutputStream.computeInt32Size(number, unsafe.getInt(message, offset));
          }
          break;
        case 5: // FIXED64:
          if ((currentPresenceField & presenceMask) != 0) {
            size += CodedOutputStream.computeFixed64Size(number, 0);
          }
          break;
        case 6: // FIXED32:
          if ((currentPresenceField & presenceMask) != 0) {
            size += CodedOutputStream.computeFixed32Size(number, 0);
          }
          break;
        case 7: // BOOL:
          if ((currentPresenceField & presenceMask) != 0) {
            size += CodedOutputStream.computeBoolSize(number, true);
          }
          break;
        case 8: // STRING:
          if ((currentPresenceField & presenceMask) != 0) {
            Object value = unsafe.getObject(message, offset);
            if (value instanceof ByteString) {
              size += CodedOutputStream.computeBytesSize(number, (ByteString) value);
            } else {
              size += CodedOutputStream.computeStringSize(number, (String) value);
            }
          }
          break;
        case 9: // MESSAGE:
          if ((currentPresenceField & presenceMask) != 0) {
            Object value = unsafe.getObject(message, offset);
            size += SchemaUtil.computeSizeMessage(number, value, getMessageFieldSchema(i));
          }
          break;
        case 10: // BYTES:
          if ((currentPresenceField & presenceMask) != 0) {
            ByteString value = (ByteString) unsafe.getObject(message, offset);
            size += CodedOutputStream.computeBytesSize(number, value);
          }
          break;
        case 11: // UINT32:
          if ((currentPresenceField & presenceMask) != 0) {
            size += CodedOutputStream.computeUInt32Size(number, unsafe.getInt(message, offset));
          }
          break;
        case 12: // ENUM:
          if ((currentPresenceField & presenceMask) != 0) {
            size += CodedOutputStream.computeEnumSize(number, unsafe.getInt(message, offset));
          }
          break;
        case 13: // SFIXED32:
          if ((currentPresenceField & presenceMask) != 0) {
            size += CodedOutputStream.computeSFixed32Size(number, 0);
          }
          break;
        case 14: // SFIXED64:
          if ((currentPresenceField & presenceMask) != 0) {
            size += CodedOutputStream.computeSFixed64Size(number, 0);
          }
          break;
        case 15: // SINT32:
          if ((currentPresenceField & presenceMask) != 0) {
            size += CodedOutputStream.computeSInt32Size(number, unsafe.getInt(message, offset));
          }
          break;
        case 16: // SINT64:
          if ((currentPresenceField & presenceMask) != 0) {
            size += CodedOutputStream.computeSInt64Size(number, unsafe.getLong(message, offset));
          }
          break;
        case 17: // GROUP:
          if ((currentPresenceField & presenceMask) != 0) {
            size +=
                CodedOutputStream.computeGroupSize(
                    number,
                    (MessageLite) unsafe.getObject(message, offset),
                    getMessageFieldSchema(i));
          }
          break;
        case 18: // DOUBLE_LIST:
          size +=
              SchemaUtil.computeSizeFixed64List(
                  number, (List<?>) unsafe.getObject(message, offset), false);
          break;
        case 19: // FLOAT_LIST:
          size +=
              SchemaUtil.computeSizeFixed32List(
                  number, (List<?>) unsafe.getObject(message, offset), false);
          break;
        case 20: // INT64_LIST:
          size +=
              SchemaUtil.computeSizeInt64List(
                  number, (List<Long>) unsafe.getObject(message, offset), false);
          break;
        case 21: // UINT64_LIST:
          size +=
              SchemaUtil.computeSizeUInt64List(
                  number, (List<Long>) unsafe.getObject(message, offset), false);
          break;
        case 22: // INT32_LIST:
          size +=
              SchemaUtil.computeSizeInt32List(
                  number, (List<Integer>) unsafe.getObject(message, offset), false);
          break;
        case 23: // FIXED64_LIST:
          size +=
              SchemaUtil.computeSizeFixed64List(
                  number, (List<?>) unsafe.getObject(message, offset), false);
          break;
        case 24: // FIXED32_LIST:
          size +=
              SchemaUtil.computeSizeFixed32List(
                  number, (List<?>) unsafe.getObject(message, offset), false);
          break;
        case 25: // BOOL_LIST:
          size +=
              SchemaUtil.computeSizeBoolList(
                  number, (List<?>) unsafe.getObject(message, offset), false);
          break;
        case 26: // STRING_LIST:
          size +=
              SchemaUtil.computeSizeStringList(number, (List<?>) unsafe.getObject(message, offset));
          break;
        case 27: // MESSAGE_LIST:
          size +=
              SchemaUtil.computeSizeMessageList(
                  number, (List<?>) unsafe.getObject(message, offset), getMessageFieldSchema(i));
          break;
        case 28: // BYTES_LIST:
          size +=
              SchemaUtil.computeSizeByteStringList(
                  number, (List<ByteString>) unsafe.getObject(message, offset));
          break;
        case 29: // UINT32_LIST:
          size +=
              SchemaUtil.computeSizeUInt32List(
                  number, (List<Integer>) unsafe.getObject(message, offset), false);
          break;
        case 30: // ENUM_LIST:
          size +=
              SchemaUtil.computeSizeEnumList(
                  number, (List<Integer>) unsafe.getObject(message, offset), false);
          break;
        case 31: // SFIXED32_LIST:
          size +=
              SchemaUtil.computeSizeFixed32List(
                  number, (List<Integer>) unsafe.getObject(message, offset), false);
          break;
        case 32: // SFIXED64_LIST:
          size +=
              SchemaUtil.computeSizeFixed64List(
                  number, (List<Long>) unsafe.getObject(message, offset), false);
          break;
        case 33: // SINT32_LIST:
          size +=
              SchemaUtil.computeSizeSInt32List(
                  number, (List<Integer>) unsafe.getObject(message, offset), false);
          break;
        case 34: // SINT64_LIST:
          size +=
              SchemaUtil.computeSizeSInt64List(
                  number, (List<Long>) unsafe.getObject(message, offset), false);
          break;
        case 35:
          { // DOUBLE_LIST_PACKED:
            int fieldSize =
                SchemaUtil.computeSizeFixed64ListNoTag(
                    (List<Double>) unsafe.getObject(message, offset));
            if (fieldSize > 0) {
              if (useCachedSizeField) {
                unsafe.putInt(message, (long) presenceMaskAndOffset, fieldSize);
              }
              size +=
                  CodedOutputStream.computeTagSize(number)
                      + CodedOutputStream.computeUInt32SizeNoTag(fieldSize)
                      + fieldSize;
            }
            break;
          }
        case 36:
          { // FLOAT_LIST_PACKED:
            int fieldSize =
                SchemaUtil.computeSizeFixed32ListNoTag(
                    (List<Float>) unsafe.getObject(message, offset));
            if (fieldSize > 0) {
              if (useCachedSizeField) {
                unsafe.putInt(message, (long) presenceMaskAndOffset, fieldSize);
              }
              size +=
                  CodedOutputStream.computeTagSize(number)
                      + CodedOutputStream.computeUInt32SizeNoTag(fieldSize)
                      + fieldSize;
            }
            break;
          }
        case 37:
          { // INT64_LIST_PACKED:
            int fieldSize =
                SchemaUtil.computeSizeInt64ListNoTag(
                    (List<Long>) unsafe.getObject(message, offset));
            if (fieldSize > 0) {
              if (useCachedSizeField) {
                unsafe.putInt(message, (long) presenceMaskAndOffset, fieldSize);
              }
              size +=
                  CodedOutputStream.computeTagSize(number)
                      + CodedOutputStream.computeUInt32SizeNoTag(fieldSize)
                      + fieldSize;
            }
            break;
          }
        case 38:
          { // UINT64_LIST_PACKED:
            int fieldSize =
                SchemaUtil.computeSizeUInt64ListNoTag(
                    (List<Long>) unsafe.getObject(message, offset));
            if (fieldSize > 0) {
              if (useCachedSizeField) {
                unsafe.putInt(message, (long) presenceMaskAndOffset, fieldSize);
              }
              size +=
                  CodedOutputStream.computeTagSize(number)
                      + CodedOutputStream.computeUInt32SizeNoTag(fieldSize)
                      + fieldSize;
            }
            break;
          }
        case 39:
          { // INT32_LIST_PACKED:
            int fieldSize =
                SchemaUtil.computeSizeInt32ListNoTag(
                    (List<Integer>) unsafe.getObject(message, offset));
            if (fieldSize > 0) {
              if (useCachedSizeField) {
                unsafe.putInt(message, (long) presenceMaskAndOffset, fieldSize);
              }
              size +=
                  CodedOutputStream.computeTagSize(number)
                      + CodedOutputStream.computeUInt32SizeNoTag(fieldSize)
                      + fieldSize;
            }
            break;
          }
        case 40:
          { // FIXED64_LIST_PACKED:
            int fieldSize =
                SchemaUtil.computeSizeFixed64ListNoTag(
                    (List<Long>) unsafe.getObject(message, offset));
            if (fieldSize > 0) {
              if (useCachedSizeField) {
                unsafe.putInt(message, (long) presenceMaskAndOffset, fieldSize);
              }
              size +=
                  CodedOutputStream.computeTagSize(number)
                      + CodedOutputStream.computeUInt32SizeNoTag(fieldSize)
                      + fieldSize;
            }
            break;
          }
        case 41:
          { // FIXED32_LIST_PACKED:
            int fieldSize =
                SchemaUtil.computeSizeFixed32ListNoTag(
                    (List<Integer>) unsafe.getObject(message, offset));
            if (fieldSize > 0) {
              if (useCachedSizeField) {
                unsafe.putInt(message, (long) presenceMaskAndOffset, fieldSize);
              }
              size +=
                  CodedOutputStream.computeTagSize(number)
                      + CodedOutputStream.computeUInt32SizeNoTag(fieldSize)
                      + fieldSize;
            }
            break;
          }
        case 42:
          { // BOOL_LIST_PACKED:
            int fieldSize =
                SchemaUtil.computeSizeBoolListNoTag(
                    (List<Boolean>) unsafe.getObject(message, offset));
            if (fieldSize > 0) {
              if (useCachedSizeField) {
                unsafe.putInt(message, (long) presenceMaskAndOffset, fieldSize);
              }
              size +=
                  CodedOutputStream.computeTagSize(number)
                      + CodedOutputStream.computeUInt32SizeNoTag(fieldSize)
                      + fieldSize;
            }
            break;
          }
        case 43:
          { // UINT32_LIST_PACKED:
            int fieldSize =
                SchemaUtil.computeSizeUInt32ListNoTag(
                    (List<Integer>) unsafe.getObject(message, offset));
            if (fieldSize > 0) {
              if (useCachedSizeField) {
                unsafe.putInt(message, (long) presenceMaskAndOffset, fieldSize);
              }
              size +=
                  CodedOutputStream.computeTagSize(number)
                      + CodedOutputStream.computeUInt32SizeNoTag(fieldSize)
                      + fieldSize;
            }
            break;
          }
        case 44:
          { // ENUM_LIST_PACKED:
            int fieldSize =
                SchemaUtil.computeSizeEnumListNoTag(
                    (List<Integer>) unsafe.getObject(message, offset));
            if (fieldSize > 0) {
              if (useCachedSizeField) {
                unsafe.putInt(message, (long) presenceMaskAndOffset, fieldSize);
              }
              size +=
                  CodedOutputStream.computeTagSize(number)
                      + CodedOutputStream.computeUInt32SizeNoTag(fieldSize)
                      + fieldSize;
            }
            break;
          }
        case 45:
          { // SFIXED32_LIST_PACKED:
            int fieldSize =
                SchemaUtil.computeSizeFixed32ListNoTag(
                    (List<Integer>) unsafe.getObject(message, offset));
            if (fieldSize > 0) {
              if (useCachedSizeField) {
                unsafe.putInt(message, (long) presenceMaskAndOffset, fieldSize);
              }
              size +=
                  CodedOutputStream.computeTagSize(number)
                      + CodedOutputStream.computeUInt32SizeNoTag(fieldSize)
                      + fieldSize;
            }
            break;
          }
        case 46:
          { // SFIXED64_LIST_PACKED:
            int fieldSize =
                SchemaUtil.computeSizeFixed64ListNoTag(
                    (List<Long>) unsafe.getObject(message, offset));
            if (fieldSize > 0) {
              if (useCachedSizeField) {
                unsafe.putInt(message, (long) presenceMaskAndOffset, fieldSize);
              }
              size +=
                  CodedOutputStream.computeTagSize(number)
                      + CodedOutputStream.computeUInt32SizeNoTag(fieldSize)
                      + fieldSize;
            }
            break;
          }
        case 47:
          { // SINT32_LIST_PACKED:
            int fieldSize =
                SchemaUtil.computeSizeSInt32ListNoTag(
                    (List<Integer>) unsafe.getObject(message, offset));
            if (fieldSize > 0) {
              if (useCachedSizeField) {
                unsafe.putInt(message, (long) presenceMaskAndOffset, fieldSize);
              }
              size +=
                  CodedOutputStream.computeTagSize(number)
                      + CodedOutputStream.computeUInt32SizeNoTag(fieldSize)
                      + fieldSize;
            }
            break;
          }
        case 48:
          { // SINT64_LIST_PACKED:
            int fieldSize =
                SchemaUtil.computeSizeSInt64ListNoTag(
                    (List<Long>) unsafe.getObject(message, offset));
            if (fieldSize > 0) {
              if (useCachedSizeField) {
                unsafe.putInt(message, (long) presenceMaskAndOffset, fieldSize);
              }
              size +=
                  CodedOutputStream.computeTagSize(number)
                      + CodedOutputStream.computeUInt32SizeNoTag(fieldSize)
                      + fieldSize;
            }
            break;
          }
        case 49: // GROUP_LIST:
          size +=
              SchemaUtil.computeSizeGroupList(
                  number,
                  (List<MessageLite>) unsafe.getObject(message, offset),
                  getMessageFieldSchema(i));
          break;
        case 50: // MAP:
          // TODO(dweis): Use schema cache.
          size +=
              mapFieldSchema.getSerializedSize(
                  number, unsafe.getObject(message, offset), getMapFieldDefaultEntry(i));
          break;
        case 51: // ONEOF_DOUBLE:
          if (isOneofPresent(message, number, i)) {
            size += CodedOutputStream.computeDoubleSize(number, 0);
          }
          break;
        case 52: // ONEOF_FLOAT:
          if (isOneofPresent(message, number, i)) {
            size += CodedOutputStream.computeFloatSize(number, 0);
          }
          break;
        case 53: // ONEOF_INT64:
          if (isOneofPresent(message, number, i)) {
            size += CodedOutputStream.computeInt64Size(number, oneofLongAt(message, offset));
          }
          break;
        case 54: // ONEOF_UINT64:
          if (isOneofPresent(message, number, i)) {
            size += CodedOutputStream.computeUInt64Size(number, oneofLongAt(message, offset));
          }
          break;
        case 55: // ONEOF_INT32:
          if (isOneofPresent(message, number, i)) {
            size += CodedOutputStream.computeInt32Size(number, oneofIntAt(message, offset));
          }
          break;
        case 56: // ONEOF_FIXED64:
          if (isOneofPresent(message, number, i)) {
            size += CodedOutputStream.computeFixed64Size(number, 0);
          }
          break;
        case 57: // ONEOF_FIXED32:
          if (isOneofPresent(message, number, i)) {
            size += CodedOutputStream.computeFixed32Size(number, 0);
          }
          break;
        case 58: // ONEOF_BOOL:
          if (isOneofPresent(message, number, i)) {
            size += CodedOutputStream.computeBoolSize(number, true);
          }
          break;
        case 59: // ONEOF_STRING:
          if (isOneofPresent(message, number, i)) {
            Object value = unsafe.getObject(message, offset);
            if (value instanceof ByteString) {
              size += CodedOutputStream.computeBytesSize(number, (ByteString) value);
            } else {
              size += CodedOutputStream.computeStringSize(number, (String) value);
            }
          }
          break;
        case 60: // ONEOF_MESSAGE:
          if (isOneofPresent(message, number, i)) {
            Object value = unsafe.getObject(message, offset);
            size += SchemaUtil.computeSizeMessage(number, value, getMessageFieldSchema(i));
          }
          break;
        case 61: // ONEOF_BYTES:
          if (isOneofPresent(message, number, i)) {
            size +=
                CodedOutputStream.computeBytesSize(
                    number, (ByteString) unsafe.getObject(message, offset));
          }
          break;
        case 62: // ONEOF_UINT32:
          if (isOneofPresent(message, number, i)) {
            size += CodedOutputStream.computeUInt32Size(number, oneofIntAt(message, offset));
          }
          break;
        case 63: // ONEOF_ENUM:
          if (isOneofPresent(message, number, i)) {
            size += CodedOutputStream.computeEnumSize(number, oneofIntAt(message, offset));
          }
          break;
        case 64: // ONEOF_SFIXED32:
          if (isOneofPresent(message, number, i)) {
            size += CodedOutputStream.computeSFixed32Size(number, 0);
          }
          break;
        case 65: // ONEOF_SFIXED64:
          if (isOneofPresent(message, number, i)) {
            size += CodedOutputStream.computeSFixed64Size(number, 0);
          }
          break;
        case 66: // ONEOF_SINT32:
          if (isOneofPresent(message, number, i)) {
            size += CodedOutputStream.computeSInt32Size(number, oneofIntAt(message, offset));
          }
          break;
        case 67: // ONEOF_SINT64:
          if (isOneofPresent(message, number, i)) {
            size += CodedOutputStream.computeSInt64Size(number, oneofLongAt(message, offset));
          }
          break;
        case 68: // ONEOF_GROUP:
          if (isOneofPresent(message, number, i)) {
            size +=
                CodedOutputStream.computeGroupSize(
                    number,
                    (MessageLite) unsafe.getObject(message, offset),
                    getMessageFieldSchema(i));
          }
          break;
        default:
          // Assume it's an empty entry.
      }
    }

    size += getUnknownFieldsSerializedSize(unknownFieldSchema, message);

    if (hasExtensions) {
      size += extensionSchema.getExtensions(message).getSerializedSize();
    }

    return size;
  }

  private int getSerializedSizeProto3(T message) {
    final sun.misc.Unsafe unsafe = UNSAFE;
    int size = 0;
    for (int i = 0; i < buffer.length; i += INTS_PER_FIELD) {
      final int typeAndOffset = typeAndOffsetAt(i);
      final int fieldType = type(typeAndOffset);
      final int number = numberAt(i);

      final long offset = offset(typeAndOffset);
      final int cachedSizeOffset =
          fieldType >= FieldType.DOUBLE_LIST_PACKED.id()
                  && fieldType <= FieldType.SINT64_LIST_PACKED.id()
              ? buffer[i + 2] & OFFSET_MASK
              : 0;

      switch (fieldType) {
        case 0: // DOUBLE:
          if (isFieldPresent(message, i)) {
            size += CodedOutputStream.computeDoubleSize(number, 0);
          }
          break;
        case 1: // FLOAT:
          if (isFieldPresent(message, i)) {
            size += CodedOutputStream.computeFloatSize(number, 0);
          }
          break;
        case 2: // INT64:
          if (isFieldPresent(message, i)) {
            size += CodedOutputStream.computeInt64Size(number, UnsafeUtil.getLong(message, offset));
          }
          break;
        case 3: // UINT64:
          if (isFieldPresent(message, i)) {
            size +=
                CodedOutputStream.computeUInt64Size(number, UnsafeUtil.getLong(message, offset));
          }
          break;
        case 4: // INT32:
          if (isFieldPresent(message, i)) {
            size += CodedOutputStream.computeInt32Size(number, UnsafeUtil.getInt(message, offset));
          }
          break;
        case 5: // FIXED64:
          if (isFieldPresent(message, i)) {
            size += CodedOutputStream.computeFixed64Size(number, 0);
          }
          break;
        case 6: // FIXED32:
          if (isFieldPresent(message, i)) {
            size += CodedOutputStream.computeFixed32Size(number, 0);
          }
          break;
        case 7: // BOOL:
          if (isFieldPresent(message, i)) {
            size += CodedOutputStream.computeBoolSize(number, true);
          }
          break;
        case 8: // STRING:
          if (isFieldPresent(message, i)) {
            Object value = UnsafeUtil.getObject(message, offset);
            if (value instanceof ByteString) {
              size += CodedOutputStream.computeBytesSize(number, (ByteString) value);
            } else {
              size += CodedOutputStream.computeStringSize(number, (String) value);
            }
          }
          break;
        case 9: // MESSAGE:
          if (isFieldPresent(message, i)) {
            Object value = UnsafeUtil.getObject(message, offset);
            size += SchemaUtil.computeSizeMessage(number, value, getMessageFieldSchema(i));
          }
          break;
        case 10: // BYTES:
          if (isFieldPresent(message, i)) {
            ByteString value = (ByteString) UnsafeUtil.getObject(message, offset);
            size += CodedOutputStream.computeBytesSize(number, value);
          }
          break;
        case 11: // UINT32:
          if (isFieldPresent(message, i)) {
            size += CodedOutputStream.computeUInt32Size(number, UnsafeUtil.getInt(message, offset));
          }
          break;
        case 12: // ENUM:
          if (isFieldPresent(message, i)) {
            size += CodedOutputStream.computeEnumSize(number, UnsafeUtil.getInt(message, offset));
          }
          break;
        case 13: // SFIXED32:
          if (isFieldPresent(message, i)) {
            size += CodedOutputStream.computeSFixed32Size(number, 0);
          }
          break;
        case 14: // SFIXED64:
          if (isFieldPresent(message, i)) {
            size += CodedOutputStream.computeSFixed64Size(number, 0);
          }
          break;
        case 15: // SINT32:
          if (isFieldPresent(message, i)) {
            size += CodedOutputStream.computeSInt32Size(number, UnsafeUtil.getInt(message, offset));
          }
          break;
        case 16: // SINT64:
          if (isFieldPresent(message, i)) {
            size +=
                CodedOutputStream.computeSInt64Size(number, UnsafeUtil.getLong(message, offset));
          }
          break;
        case 17: // GROUP:
          if (isFieldPresent(message, i)) {
            size +=
                CodedOutputStream.computeGroupSize(
                    number,
                    (MessageLite) UnsafeUtil.getObject(message, offset),
                    getMessageFieldSchema(i));
          }
          break;
        case 18: // DOUBLE_LIST:
          size += SchemaUtil.computeSizeFixed64List(number, listAt(message, offset), false);
          break;
        case 19: // FLOAT_LIST:
          size += SchemaUtil.computeSizeFixed32List(number, listAt(message, offset), false);
          break;
        case 20: // INT64_LIST:
          size +=
              SchemaUtil.computeSizeInt64List(number, (List<Long>) listAt(message, offset), false);
          break;
        case 21: // UINT64_LIST:
          size +=
              SchemaUtil.computeSizeUInt64List(number, (List<Long>) listAt(message, offset), false);
          break;
        case 22: // INT32_LIST:
          size +=
              SchemaUtil.computeSizeInt32List(
                  number, (List<Integer>) listAt(message, offset), false);
          break;
        case 23: // FIXED64_LIST:
          size += SchemaUtil.computeSizeFixed64List(number, listAt(message, offset), false);
          break;
        case 24: // FIXED32_LIST:
          size += SchemaUtil.computeSizeFixed32List(number, listAt(message, offset), false);
          break;
        case 25: // BOOL_LIST:
          size += SchemaUtil.computeSizeBoolList(number, listAt(message, offset), false);
          break;
        case 26: // STRING_LIST:
          size += SchemaUtil.computeSizeStringList(number, listAt(message, offset));
          break;
        case 27: // MESSAGE_LIST:
          size +=
              SchemaUtil.computeSizeMessageList(
                  number, listAt(message, offset), getMessageFieldSchema(i));
          break;
        case 28: // BYTES_LIST:
          size +=
              SchemaUtil.computeSizeByteStringList(
                  number, (List<ByteString>) listAt(message, offset));
          break;
        case 29: // UINT32_LIST:
          size +=
              SchemaUtil.computeSizeUInt32List(
                  number, (List<Integer>) listAt(message, offset), false);
          break;
        case 30: // ENUM_LIST:
          size +=
              SchemaUtil.computeSizeEnumList(
                  number, (List<Integer>) listAt(message, offset), false);
          break;
        case 31: // SFIXED32_LIST:
          size += SchemaUtil.computeSizeFixed32List(number, listAt(message, offset), false);
          break;
        case 32: // SFIXED64_LIST:
          size += SchemaUtil.computeSizeFixed64List(number, listAt(message, offset), false);
          break;
        case 33: // SINT32_LIST:
          size +=
              SchemaUtil.computeSizeSInt32List(
                  number, (List<Integer>) listAt(message, offset), false);
          break;
        case 34: // SINT64_LIST:
          size +=
              SchemaUtil.computeSizeSInt64List(number, (List<Long>) listAt(message, offset), false);
          break;
        case 35:
          { // DOUBLE_LIST_PACKED:
            int fieldSize =
                SchemaUtil.computeSizeFixed64ListNoTag(
                    (List<Double>) unsafe.getObject(message, offset));
            if (fieldSize > 0) {
              if (useCachedSizeField) {
                unsafe.putInt(message, (long) cachedSizeOffset, fieldSize);
              }
              size +=
                  CodedOutputStream.computeTagSize(number)
                      + CodedOutputStream.computeUInt32SizeNoTag(fieldSize)
                      + fieldSize;
            }
            break;
          }
        case 36:
          { // FLOAT_LIST_PACKED:
            int fieldSize =
                SchemaUtil.computeSizeFixed32ListNoTag(
                    (List<Float>) unsafe.getObject(message, offset));
            if (fieldSize > 0) {
              if (useCachedSizeField) {
                unsafe.putInt(message, (long) cachedSizeOffset, fieldSize);
              }
              size +=
                  CodedOutputStream.computeTagSize(number)
                      + CodedOutputStream.computeUInt32SizeNoTag(fieldSize)
                      + fieldSize;
            }
            break;
          }
        case 37:
          { // INT64_LIST_PACKED:
            int fieldSize =
                SchemaUtil.computeSizeInt64ListNoTag(
                    (List<Long>) unsafe.getObject(message, offset));
            if (fieldSize > 0) {
              if (useCachedSizeField) {
                unsafe.putInt(message, (long) cachedSizeOffset, fieldSize);
              }
              size +=
                  CodedOutputStream.computeTagSize(number)
                      + CodedOutputStream.computeUInt32SizeNoTag(fieldSize)
                      + fieldSize;
            }
            break;
          }
        case 38:
          { // UINT64_LIST_PACKED:
            int fieldSize =
                SchemaUtil.computeSizeUInt64ListNoTag(
                    (List<Long>) unsafe.getObject(message, offset));
            if (fieldSize > 0) {
              if (useCachedSizeField) {
                unsafe.putInt(message, (long) cachedSizeOffset, fieldSize);
              }
              size +=
                  CodedOutputStream.computeTagSize(number)
                      + CodedOutputStream.computeUInt32SizeNoTag(fieldSize)
                      + fieldSize;
            }
            break;
          }
        case 39:
          { // INT32_LIST_PACKED:
            int fieldSize =
                SchemaUtil.computeSizeInt32ListNoTag(
                    (List<Integer>) unsafe.getObject(message, offset));
            if (fieldSize > 0) {
              if (useCachedSizeField) {
                unsafe.putInt(message, (long) cachedSizeOffset, fieldSize);
              }
              size +=
                  CodedOutputStream.computeTagSize(number)
                      + CodedOutputStream.computeUInt32SizeNoTag(fieldSize)
                      + fieldSize;
            }
            break;
          }
        case 40:
          { // FIXED64_LIST_PACKED:
            int fieldSize =
                SchemaUtil.computeSizeFixed64ListNoTag(
                    (List<Long>) unsafe.getObject(message, offset));
            if (fieldSize > 0) {
              if (useCachedSizeField) {
                unsafe.putInt(message, (long) cachedSizeOffset, fieldSize);
              }
              size +=
                  CodedOutputStream.computeTagSize(number)
                      + CodedOutputStream.computeUInt32SizeNoTag(fieldSize)
                      + fieldSize;
            }
            break;
          }
        case 41:
          { // FIXED32_LIST_PACKED:
            int fieldSize =
                SchemaUtil.computeSizeFixed32ListNoTag(
                    (List<Integer>) unsafe.getObject(message, offset));
            if (fieldSize > 0) {
              if (useCachedSizeField) {
                unsafe.putInt(message, (long) cachedSizeOffset, fieldSize);
              }
              size +=
                  CodedOutputStream.computeTagSize(number)
                      + CodedOutputStream.computeUInt32SizeNoTag(fieldSize)
                      + fieldSize;
            }
            break;
          }
        case 42:
          { // BOOL_LIST_PACKED:
            int fieldSize =
                SchemaUtil.computeSizeBoolListNoTag(
                    (List<Boolean>) unsafe.getObject(message, offset));
            if (fieldSize > 0) {
              if (useCachedSizeField) {
                unsafe.putInt(message, (long) cachedSizeOffset, fieldSize);
              }
              size +=
                  CodedOutputStream.computeTagSize(number)
                      + CodedOutputStream.computeUInt32SizeNoTag(fieldSize)
                      + fieldSize;
            }
            break;
          }
        case 43:
          { // UINT32_LIST_PACKED:
            int fieldSize =
                SchemaUtil.computeSizeUInt32ListNoTag(
                    (List<Integer>) unsafe.getObject(message, offset));
            if (fieldSize > 0) {
              if (useCachedSizeField) {
                unsafe.putInt(message, (long) cachedSizeOffset, fieldSize);
              }
              size +=
                  CodedOutputStream.computeTagSize(number)
                      + CodedOutputStream.computeUInt32SizeNoTag(fieldSize)
                      + fieldSize;
            }
            break;
          }
        case 44:
          { // ENUM_LIST_PACKED:
            int fieldSize =
                SchemaUtil.computeSizeEnumListNoTag(
                    (List<Integer>) unsafe.getObject(message, offset));
            if (fieldSize > 0) {
              if (useCachedSizeField) {
                unsafe.putInt(message, (long) cachedSizeOffset, fieldSize);
              }
              size +=
                  CodedOutputStream.computeTagSize(number)
                      + CodedOutputStream.computeUInt32SizeNoTag(fieldSize)
                      + fieldSize;
            }
            break;
          }
        case 45:
          { // SFIXED32_LIST_PACKED:
            int fieldSize =
                SchemaUtil.computeSizeFixed32ListNoTag(
                    (List<Integer>) unsafe.getObject(message, offset));
            if (fieldSize > 0) {
              if (useCachedSizeField) {
                unsafe.putInt(message, (long) cachedSizeOffset, fieldSize);
              }
              size +=
                  CodedOutputStream.computeTagSize(number)
                      + CodedOutputStream.computeUInt32SizeNoTag(fieldSize)
                      + fieldSize;
            }
            break;
          }
        case 46:
          { // SFIXED64_LIST_PACKED:
            int fieldSize =
                SchemaUtil.computeSizeFixed64ListNoTag(
                    (List<Long>) unsafe.getObject(message, offset));
            if (fieldSize > 0) {
              if (useCachedSizeField) {
                unsafe.putInt(message, (long) cachedSizeOffset, fieldSize);
              }
              size +=
                  CodedOutputStream.computeTagSize(number)
                      + CodedOutputStream.computeUInt32SizeNoTag(fieldSize)
                      + fieldSize;
            }
            break;
          }
        case 47:
          { // SINT32_LIST_PACKED:
            int fieldSize =
                SchemaUtil.computeSizeSInt32ListNoTag(
                    (List<Integer>) unsafe.getObject(message, offset));
            if (fieldSize > 0) {
              if (useCachedSizeField) {
                unsafe.putInt(message, (long) cachedSizeOffset, fieldSize);
              }
              size +=
                  CodedOutputStream.computeTagSize(number)
                      + CodedOutputStream.computeUInt32SizeNoTag(fieldSize)
                      + fieldSize;
            }
            break;
          }
        case 48:
          { // SINT64_LIST_PACKED:
            int fieldSize =
                SchemaUtil.computeSizeSInt64ListNoTag(
                    (List<Long>) unsafe.getObject(message, offset));
            if (fieldSize > 0) {
              if (useCachedSizeField) {
                unsafe.putInt(message, (long) cachedSizeOffset, fieldSize);
              }
              size +=
                  CodedOutputStream.computeTagSize(number)
                      + CodedOutputStream.computeUInt32SizeNoTag(fieldSize)
                      + fieldSize;
            }
            break;
          }
        case 49: // GROUP_LIST:
          size +=
              SchemaUtil.computeSizeGroupList(
                  number, (List<MessageLite>) listAt(message, offset), getMessageFieldSchema(i));
          break;
        case 50: // MAP:
          // TODO(dweis): Use schema cache.
          size +=
              mapFieldSchema.getSerializedSize(
                  number, UnsafeUtil.getObject(message, offset), getMapFieldDefaultEntry(i));
          break;
        case 51: // ONEOF_DOUBLE:
          if (isOneofPresent(message, number, i)) {
            size += CodedOutputStream.computeDoubleSize(number, 0);
          }
          break;
        case 52: // ONEOF_FLOAT:
          if (isOneofPresent(message, number, i)) {
            size += CodedOutputStream.computeFloatSize(number, 0);
          }
          break;
        case 53: // ONEOF_INT64:
          if (isOneofPresent(message, number, i)) {
            size += CodedOutputStream.computeInt64Size(number, oneofLongAt(message, offset));
          }
          break;
        case 54: // ONEOF_UINT64:
          if (isOneofPresent(message, number, i)) {
            size += CodedOutputStream.computeUInt64Size(number, oneofLongAt(message, offset));
          }
          break;
        case 55: // ONEOF_INT32:
          if (isOneofPresent(message, number, i)) {
            size += CodedOutputStream.computeInt32Size(number, oneofIntAt(message, offset));
          }
          break;
        case 56: // ONEOF_FIXED64:
          if (isOneofPresent(message, number, i)) {
            size += CodedOutputStream.computeFixed64Size(number, 0);
          }
          break;
        case 57: // ONEOF_FIXED32:
          if (isOneofPresent(message, number, i)) {
            size += CodedOutputStream.computeFixed32Size(number, 0);
          }
          break;
        case 58: // ONEOF_BOOL:
          if (isOneofPresent(message, number, i)) {
            size += CodedOutputStream.computeBoolSize(number, true);
          }
          break;
        case 59: // ONEOF_STRING:
          if (isOneofPresent(message, number, i)) {
            Object value = UnsafeUtil.getObject(message, offset);
            if (value instanceof ByteString) {
              size += CodedOutputStream.computeBytesSize(number, (ByteString) value);
            } else {
              size += CodedOutputStream.computeStringSize(number, (String) value);
            }
          }
          break;
        case 60: // ONEOF_MESSAGE:
          if (isOneofPresent(message, number, i)) {
            Object value = UnsafeUtil.getObject(message, offset);
            size += SchemaUtil.computeSizeMessage(number, value, getMessageFieldSchema(i));
          }
          break;
        case 61: // ONEOF_BYTES:
          if (isOneofPresent(message, number, i)) {
            size +=
                CodedOutputStream.computeBytesSize(
                    number, (ByteString) UnsafeUtil.getObject(message, offset));
          }
          break;
        case 62: // ONEOF_UINT32:
          if (isOneofPresent(message, number, i)) {
            size += CodedOutputStream.computeUInt32Size(number, oneofIntAt(message, offset));
          }
          break;
        case 63: // ONEOF_ENUM:
          if (isOneofPresent(message, number, i)) {
            size += CodedOutputStream.computeEnumSize(number, oneofIntAt(message, offset));
          }
          break;
        case 64: // ONEOF_SFIXED32:
          if (isOneofPresent(message, number, i)) {
            size += CodedOutputStream.computeSFixed32Size(number, 0);
          }
          break;
        case 65: // ONEOF_SFIXED64:
          if (isOneofPresent(message, number, i)) {
            size += CodedOutputStream.computeSFixed64Size(number, 0);
          }
          break;
        case 66: // ONEOF_SINT32:
          if (isOneofPresent(message, number, i)) {
            size += CodedOutputStream.computeSInt32Size(number, oneofIntAt(message, offset));
          }
          break;
        case 67: // ONEOF_SINT64:
          if (isOneofPresent(message, number, i)) {
            size += CodedOutputStream.computeSInt64Size(number, oneofLongAt(message, offset));
          }
          break;
        case 68: // ONEOF_GROUP:
          if (isOneofPresent(message, number, i)) {
            size +=
                CodedOutputStream.computeGroupSize(
                    number,
                    (MessageLite) UnsafeUtil.getObject(message, offset),
                    getMessageFieldSchema(i));
          }
          break;
        default:
          // Assume it's an empty entry.
      }
    }

    size += getUnknownFieldsSerializedSize(unknownFieldSchema, message);

    return size;
  }

  private <UT, UB> int getUnknownFieldsSerializedSize(
      UnknownFieldSchema<UT, UB> schema, T message) {
    UT unknowns = schema.getFromMessage(message);
    return schema.getSerializedSize(unknowns);
  }

  private static List<?> listAt(Object message, long offset) {
    return (List<?>) UnsafeUtil.getObject(message, offset);
  }

  @SuppressWarnings("unchecked")
  @Override
  // TODO(nathanmittler): Consider serializing oneof fields last so that only one entry per
  // oneof is actually serialized. This would mean that we would violate the serialization order
  // contract. It should also be noted that Go currently does this.
  public void writeTo(T message, Writer writer) throws IOException {
    if (writer.fieldOrder() == Writer.FieldOrder.DESCENDING) {
      writeFieldsInDescendingOrder(message, writer);
    } else {
      if (proto3) {
        writeFieldsInAscendingOrderProto3(message, writer);
      } else {
        writeFieldsInAscendingOrderProto2(message, writer);
      }
    }
  }

  @SuppressWarnings("unchecked")
  private void writeFieldsInAscendingOrderProto2(T message, Writer writer) throws IOException {
    Iterator<? extends Map.Entry<?, ?>> extensionIterator = null;
    Map.Entry nextExtension = null;
    if (hasExtensions) {
      FieldSet<?> extensions = extensionSchema.getExtensions(message);
      if (!extensions.isEmpty()) {
        extensionIterator = extensions.iterator();
        nextExtension = extensionIterator.next();
      }
    }
    int currentPresenceFieldOffset = -1;
    int currentPresenceField = 0;
    final int bufferLength = buffer.length;
    final sun.misc.Unsafe unsafe = UNSAFE;
    for (int pos = 0; pos < bufferLength; pos += INTS_PER_FIELD) {
      final int typeAndOffset = typeAndOffsetAt(pos);
      final int number = numberAt(pos);
      final int fieldType = type(typeAndOffset);

      int presenceMaskAndOffset = 0;
      int presenceMask = 0;
      if (!proto3 && fieldType <= 17) {
        presenceMaskAndOffset = buffer[pos + 2];
        final int presenceFieldOffset = presenceMaskAndOffset & OFFSET_MASK;
        if (presenceFieldOffset != currentPresenceFieldOffset) {
          currentPresenceFieldOffset = presenceFieldOffset;
          currentPresenceField = unsafe.getInt(message, (long) presenceFieldOffset);
        }
        presenceMask = 1 << (presenceMaskAndOffset >>> OFFSET_BITS);
      }

      // Write any extensions that need to be written before the current field.
      while (nextExtension != null && extensionSchema.extensionNumber(nextExtension) <= number) {
        extensionSchema.serializeExtension(writer, nextExtension);
        nextExtension = extensionIterator.hasNext() ? extensionIterator.next() : null;
      }
      final long offset = offset(typeAndOffset);

      switch (fieldType) {
        case 0: // DOUBLE:
          if ((currentPresenceField & presenceMask) != 0) {
            writer.writeDouble(number, doubleAt(message, offset));
          }
          break;
        case 1: // FLOAT:
          if ((currentPresenceField & presenceMask) != 0) {
            writer.writeFloat(number, floatAt(message, offset));
          }
          break;
        case 2: // INT64:
          if ((currentPresenceField & presenceMask) != 0) {
            writer.writeInt64(number, unsafe.getLong(message, offset));
          }
          break;
        case 3: // UINT64:
          if ((currentPresenceField & presenceMask) != 0) {
            writer.writeUInt64(number, unsafe.getLong(message, offset));
          }
          break;
        case 4: // INT32:
          if ((currentPresenceField & presenceMask) != 0) {
            writer.writeInt32(number, unsafe.getInt(message, offset));
          }
          break;
        case 5: // FIXED64:
          if ((currentPresenceField & presenceMask) != 0) {
            writer.writeFixed64(number, unsafe.getLong(message, offset));
          }
          break;
        case 6: // FIXED32:
          if ((currentPresenceField & presenceMask) != 0) {
            writer.writeFixed32(number, unsafe.getInt(message, offset));
          }
          break;
        case 7: // BOOL:
          if ((currentPresenceField & presenceMask) != 0) {
            writer.writeBool(number, booleanAt(message, offset));
          }
          break;
        case 8: // STRING:
          if ((currentPresenceField & presenceMask) != 0) {
            writeString(number, unsafe.getObject(message, offset), writer);
          }
          break;
        case 9: // MESSAGE:
          if ((currentPresenceField & presenceMask) != 0) {
            Object value = unsafe.getObject(message, offset);
            writer.writeMessage(number, value, getMessageFieldSchema(pos));
          }
          break;
        case 10: // BYTES:
          if ((currentPresenceField & presenceMask) != 0) {
            writer.writeBytes(number, (ByteString) unsafe.getObject(message, offset));
          }
          break;
        case 11: // UINT32:
          if ((currentPresenceField & presenceMask) != 0) {
            writer.writeUInt32(number, unsafe.getInt(message, offset));
          }
          break;
        case 12: // ENUM:
          if ((currentPresenceField & presenceMask) != 0) {
            writer.writeEnum(number, unsafe.getInt(message, offset));
          }
          break;
        case 13: // SFIXED32:
          if ((currentPresenceField & presenceMask) != 0) {
            writer.writeSFixed32(number, unsafe.getInt(message, offset));
          }
          break;
        case 14: // SFIXED64:
          if ((currentPresenceField & presenceMask) != 0) {
            writer.writeSFixed64(number, unsafe.getLong(message, offset));
          }
          break;
        case 15: // SINT32:
          if ((currentPresenceField & presenceMask) != 0) {
            writer.writeSInt32(number, unsafe.getInt(message, offset));
          }
          break;
        case 16: // SINT64:
          if ((currentPresenceField & presenceMask) != 0) {
            writer.writeSInt64(number, unsafe.getLong(message, offset));
          }
          break;
        case 17: // GROUP:
          if ((currentPresenceField & presenceMask) != 0) {
            writer.writeGroup(
                number, unsafe.getObject(message, offset), getMessageFieldSchema(pos));
          }
          break;
        case 18: // DOUBLE_LIST:
          SchemaUtil.writeDoubleList(
              numberAt(pos), (List<Double>) unsafe.getObject(message, offset), writer, false);
          break;
        case 19: // FLOAT_LIST:
          SchemaUtil.writeFloatList(
              numberAt(pos), (List<Float>) unsafe.getObject(message, offset), writer, false);
          break;
        case 20: // INT64_LIST:
          SchemaUtil.writeInt64List(
              numberAt(pos), (List<Long>) unsafe.getObject(message, offset), writer, false);
          break;
        case 21: // UINT64_LIST:
          SchemaUtil.writeUInt64List(
              numberAt(pos), (List<Long>) unsafe.getObject(message, offset), writer, false);
          break;
        case 22: // INT32_LIST:
          SchemaUtil.writeInt32List(
              numberAt(pos), (List<Integer>) unsafe.getObject(message, offset), writer, false);
          break;
        case 23: // FIXED64_LIST:
          SchemaUtil.writeFixed64List(
              numberAt(pos), (List<Long>) unsafe.getObject(message, offset), writer, false);
          break;
        case 24: // FIXED32_LIST:
          SchemaUtil.writeFixed32List(
              numberAt(pos), (List<Integer>) unsafe.getObject(message, offset), writer, false);
          break;
        case 25: // BOOL_LIST:
          SchemaUtil.writeBoolList(
              numberAt(pos), (List<Boolean>) unsafe.getObject(message, offset), writer, false);
          break;
        case 26: // STRING_LIST:
          SchemaUtil.writeStringList(
              numberAt(pos), (List<String>) unsafe.getObject(message, offset), writer);
          break;
        case 27: // MESSAGE_LIST:
          SchemaUtil.writeMessageList(
              numberAt(pos),
              (List<?>) unsafe.getObject(message, offset),
              writer,
              getMessageFieldSchema(pos));
          break;
        case 28: // BYTES_LIST:
          SchemaUtil.writeBytesList(
              numberAt(pos), (List<ByteString>) unsafe.getObject(message, offset), writer);
          break;
        case 29: // UINT32_LIST:
          SchemaUtil.writeUInt32List(
              numberAt(pos), (List<Integer>) unsafe.getObject(message, offset), writer, false);
          break;
        case 30: // ENUM_LIST:
          SchemaUtil.writeEnumList(
              numberAt(pos), (List<Integer>) unsafe.getObject(message, offset), writer, false);
          break;
        case 31: // SFIXED32_LIST:
          SchemaUtil.writeSFixed32List(
              numberAt(pos), (List<Integer>) unsafe.getObject(message, offset), writer, false);
          break;
        case 32: // SFIXED64_LIST:
          SchemaUtil.writeSFixed64List(
              numberAt(pos), (List<Long>) unsafe.getObject(message, offset), writer, false);
          break;
        case 33: // SINT32_LIST:
          SchemaUtil.writeSInt32List(
              numberAt(pos), (List<Integer>) unsafe.getObject(message, offset), writer, false);
          break;
        case 34: // SINT64_LIST:
          SchemaUtil.writeSInt64List(
              numberAt(pos), (List<Long>) unsafe.getObject(message, offset), writer, false);
          break;
        case 35: // DOUBLE_LIST_PACKED:
          // TODO(xiaofeng): Make use of cached field size to speed up serialization.
          SchemaUtil.writeDoubleList(
              numberAt(pos), (List<Double>) unsafe.getObject(message, offset), writer, true);
          break;
        case 36: // FLOAT_LIST_PACKED:
          SchemaUtil.writeFloatList(
              numberAt(pos), (List<Float>) unsafe.getObject(message, offset), writer, true);
          break;
        case 37: // INT64_LIST_PACKED:
          SchemaUtil.writeInt64List(
              numberAt(pos), (List<Long>) unsafe.getObject(message, offset), writer, true);
          break;
        case 38: // UINT64_LIST_PACKED:
          SchemaUtil.writeUInt64List(
              numberAt(pos), (List<Long>) unsafe.getObject(message, offset), writer, true);
          break;
        case 39: // INT32_LIST_PACKED:
          SchemaUtil.writeInt32List(
              numberAt(pos), (List<Integer>) unsafe.getObject(message, offset), writer, true);
          break;
        case 40: // FIXED64_LIST_PACKED:
          SchemaUtil.writeFixed64List(
              numberAt(pos), (List<Long>) unsafe.getObject(message, offset), writer, true);
          break;
        case 41: // FIXED32_LIST_PACKED:
          SchemaUtil.writeFixed32List(
              numberAt(pos), (List<Integer>) unsafe.getObject(message, offset), writer, true);

          break;
        case 42: // BOOL_LIST_PACKED:
          SchemaUtil.writeBoolList(
              numberAt(pos), (List<Boolean>) unsafe.getObject(message, offset), writer, true);
          break;
        case 43: // UINT32_LIST_PACKED:
          SchemaUtil.writeUInt32List(
              numberAt(pos), (List<Integer>) unsafe.getObject(message, offset), writer, true);
          break;
        case 44: // ENUM_LIST_PACKED:
          SchemaUtil.writeEnumList(
              numberAt(pos), (List<Integer>) unsafe.getObject(message, offset), writer, true);
          break;
        case 45: // SFIXED32_LIST_PACKED:
          SchemaUtil.writeSFixed32List(
              numberAt(pos), (List<Integer>) unsafe.getObject(message, offset), writer, true);
          break;
        case 46: // SFIXED64_LIST_PACKED:
          SchemaUtil.writeSFixed64List(
              numberAt(pos), (List<Long>) unsafe.getObject(message, offset), writer, true);
          break;
        case 47: // SINT32_LIST_PACKED:
          SchemaUtil.writeSInt32List(
              numberAt(pos), (List<Integer>) unsafe.getObject(message, offset), writer, true);
          break;
        case 48: // SINT64_LIST_PACKED:
          SchemaUtil.writeSInt64List(
              numberAt(pos), (List<Long>) unsafe.getObject(message, offset), writer, true);
          break;
        case 49: // GROUP_LIST:
          SchemaUtil.writeGroupList(
              numberAt(pos),
              (List<?>) unsafe.getObject(message, offset),
              writer,
              getMessageFieldSchema(pos));
          break;
        case 50: // MAP:
          // TODO(dweis): Use schema cache.
          writeMapHelper(writer, number, unsafe.getObject(message, offset), pos);
          break;
        case 51: // ONEOF_DOUBLE:
          if (isOneofPresent(message, number, pos)) {
            writer.writeDouble(number, oneofDoubleAt(message, offset));
          }
          break;
        case 52: // ONEOF_FLOAT:
          if (isOneofPresent(message, number, pos)) {
            writer.writeFloat(number, oneofFloatAt(message, offset));
          }
          break;
        case 53: // ONEOF_INT64:
          if (isOneofPresent(message, number, pos)) {
            writer.writeInt64(number, oneofLongAt(message, offset));
          }
          break;
        case 54: // ONEOF_UINT64:
          if (isOneofPresent(message, number, pos)) {
            writer.writeUInt64(number, oneofLongAt(message, offset));
          }
          break;
        case 55: // ONEOF_INT32:
          if (isOneofPresent(message, number, pos)) {
            writer.writeInt32(number, oneofIntAt(message, offset));
          }
          break;
        case 56: // ONEOF_FIXED64:
          if (isOneofPresent(message, number, pos)) {
            writer.writeFixed64(number, oneofLongAt(message, offset));
          }
          break;
        case 57: // ONEOF_FIXED32:
          if (isOneofPresent(message, number, pos)) {
            writer.writeFixed32(number, oneofIntAt(message, offset));
          }
          break;
        case 58: // ONEOF_BOOL:
          if (isOneofPresent(message, number, pos)) {
            writer.writeBool(number, oneofBooleanAt(message, offset));
          }
          break;
        case 59: // ONEOF_STRING:
          if (isOneofPresent(message, number, pos)) {
            writeString(number, unsafe.getObject(message, offset), writer);
          }
          break;
        case 60: // ONEOF_MESSAGE:
          if (isOneofPresent(message, number, pos)) {
            Object value = unsafe.getObject(message, offset);
            writer.writeMessage(number, value, getMessageFieldSchema(pos));
          }
          break;
        case 61: // ONEOF_BYTES:
          if (isOneofPresent(message, number, pos)) {
            writer.writeBytes(number, (ByteString) unsafe.getObject(message, offset));
          }
          break;
        case 62: // ONEOF_UINT32:
          if (isOneofPresent(message, number, pos)) {
            writer.writeUInt32(number, oneofIntAt(message, offset));
          }
          break;
        case 63: // ONEOF_ENUM:
          if (isOneofPresent(message, number, pos)) {
            writer.writeEnum(number, oneofIntAt(message, offset));
          }
          break;
        case 64: // ONEOF_SFIXED32:
          if (isOneofPresent(message, number, pos)) {
            writer.writeSFixed32(number, oneofIntAt(message, offset));
          }
          break;
        case 65: // ONEOF_SFIXED64:
          if (isOneofPresent(message, number, pos)) {
            writer.writeSFixed64(number, oneofLongAt(message, offset));
          }
          break;
        case 66: // ONEOF_SINT32:
          if (isOneofPresent(message, number, pos)) {
            writer.writeSInt32(number, oneofIntAt(message, offset));
          }
          break;
        case 67: // ONEOF_SINT64:
          if (isOneofPresent(message, number, pos)) {
            writer.writeSInt64(number, oneofLongAt(message, offset));
          }
          break;
        case 68: // ONEOF_GROUP:
          if (isOneofPresent(message, number, pos)) {
            writer.writeGroup(
                number, unsafe.getObject(message, offset), getMessageFieldSchema(pos));
          }
          break;
        default:
          // Assume it's an empty entry - just go to the next entry.
          break;
      }
    }
    while (nextExtension != null) {
      extensionSchema.serializeExtension(writer, nextExtension);
      nextExtension = extensionIterator.hasNext() ? extensionIterator.next() : null;
    }
    writeUnknownInMessageTo(unknownFieldSchema, message, writer);
  }

  @SuppressWarnings("unchecked")
  private void writeFieldsInAscendingOrderProto3(T message, Writer writer) throws IOException {
    Iterator<? extends Map.Entry<?, ?>> extensionIterator = null;
    Map.Entry nextExtension = null;
    if (hasExtensions) {
      FieldSet<?> extensions = extensionSchema.getExtensions(message);
      if (!extensions.isEmpty()) {
        extensionIterator = extensions.iterator();
        nextExtension = extensionIterator.next();
      }
    }

    final int bufferLength = buffer.length;
    for (int pos = 0; pos < bufferLength; pos += INTS_PER_FIELD) {
      final int typeAndOffset = typeAndOffsetAt(pos);
      final int number = numberAt(pos);

      // Write any extensions that need to be written before the current field.
      while (nextExtension != null && extensionSchema.extensionNumber(nextExtension) <= number) {
        extensionSchema.serializeExtension(writer, nextExtension);
        nextExtension = extensionIterator.hasNext() ? extensionIterator.next() : null;
      }

      switch (type(typeAndOffset)) {
        case 0: // DOUBLE:
          if (isFieldPresent(message, pos)) {
            writer.writeDouble(number, doubleAt(message, offset(typeAndOffset)));
          }
          break;
        case 1: // FLOAT:
          if (isFieldPresent(message, pos)) {
            writer.writeFloat(number, floatAt(message, offset(typeAndOffset)));
          }
          break;
        case 2: // INT64:
          if (isFieldPresent(message, pos)) {
            writer.writeInt64(number, longAt(message, offset(typeAndOffset)));
          }
          break;
        case 3: // UINT64:
          if (isFieldPresent(message, pos)) {
            writer.writeUInt64(number, longAt(message, offset(typeAndOffset)));
          }
          break;
        case 4: // INT32:
          if (isFieldPresent(message, pos)) {
            writer.writeInt32(number, intAt(message, offset(typeAndOffset)));
          }
          break;
        case 5: // FIXED64:
          if (isFieldPresent(message, pos)) {
            writer.writeFixed64(number, longAt(message, offset(typeAndOffset)));
          }
          break;
        case 6: // FIXED32:
          if (isFieldPresent(message, pos)) {
            writer.writeFixed32(number, intAt(message, offset(typeAndOffset)));
          }
          break;
        case 7: // BOOL:
          if (isFieldPresent(message, pos)) {
            writer.writeBool(number, booleanAt(message, offset(typeAndOffset)));
          }
          break;
        case 8: // STRING:
          if (isFieldPresent(message, pos)) {
            writeString(number, UnsafeUtil.getObject(message, offset(typeAndOffset)), writer);
          }
          break;
        case 9: // MESSAGE:
          if (isFieldPresent(message, pos)) {
            Object value = UnsafeUtil.getObject(message, offset(typeAndOffset));
            writer.writeMessage(number, value, getMessageFieldSchema(pos));
          }
          break;
        case 10: // BYTES:
          if (isFieldPresent(message, pos)) {
            writer.writeBytes(
                number, (ByteString) UnsafeUtil.getObject(message, offset(typeAndOffset)));
          }
          break;
        case 11: // UINT32:
          if (isFieldPresent(message, pos)) {
            writer.writeUInt32(number, intAt(message, offset(typeAndOffset)));
          }
          break;
        case 12: // ENUM:
          if (isFieldPresent(message, pos)) {
            writer.writeEnum(number, intAt(message, offset(typeAndOffset)));
          }
          break;
        case 13: // SFIXED32:
          if (isFieldPresent(message, pos)) {
            writer.writeSFixed32(number, intAt(message, offset(typeAndOffset)));
          }
          break;
        case 14: // SFIXED64:
          if (isFieldPresent(message, pos)) {
            writer.writeSFixed64(number, longAt(message, offset(typeAndOffset)));
          }
          break;
        case 15: // SINT32:
          if (isFieldPresent(message, pos)) {
            writer.writeSInt32(number, intAt(message, offset(typeAndOffset)));
          }
          break;
        case 16: // SINT64:
          if (isFieldPresent(message, pos)) {
            writer.writeSInt64(number, longAt(message, offset(typeAndOffset)));
          }
          break;
        case 17: // GROUP:
          if (isFieldPresent(message, pos)) {
            writer.writeGroup(
                number,
                UnsafeUtil.getObject(message, offset(typeAndOffset)),
                getMessageFieldSchema(pos));
          }
          break;
        case 18: // DOUBLE_LIST:
          SchemaUtil.writeDoubleList(
              numberAt(pos),
              (List<Double>) UnsafeUtil.getObject(message, offset(typeAndOffset)),
              writer,
              false);
          break;
        case 19: // FLOAT_LIST:
          SchemaUtil.writeFloatList(
              numberAt(pos),
              (List<Float>) UnsafeUtil.getObject(message, offset(typeAndOffset)),
              writer,
              false);
          break;
        case 20: // INT64_LIST:
          SchemaUtil.writeInt64List(
              numberAt(pos),
              (List<Long>) UnsafeUtil.getObject(message, offset(typeAndOffset)),
              writer,
              false);
          break;
        case 21: // UINT64_LIST:
          SchemaUtil.writeUInt64List(
              numberAt(pos),
              (List<Long>) UnsafeUtil.getObject(message, offset(typeAndOffset)),
              writer,
              false);
          break;
        case 22: // INT32_LIST:
          SchemaUtil.writeInt32List(
              numberAt(pos),
              (List<Integer>) UnsafeUtil.getObject(message, offset(typeAndOffset)),
              writer,
              false);
          break;
        case 23: // FIXED64_LIST:
          SchemaUtil.writeFixed64List(
              numberAt(pos),
              (List<Long>) UnsafeUtil.getObject(message, offset(typeAndOffset)),
              writer,
              false);
          break;
        case 24: // FIXED32_LIST:
          SchemaUtil.writeFixed32List(
              numberAt(pos),
              (List<Integer>) UnsafeUtil.getObject(message, offset(typeAndOffset)),
              writer,
              false);
          break;
        case 25: // BOOL_LIST:
          SchemaUtil.writeBoolList(
              numberAt(pos),
              (List<Boolean>) UnsafeUtil.getObject(message, offset(typeAndOffset)),
              writer,
              false);
          break;
        case 26: // STRING_LIST:
          SchemaUtil.writeStringList(
              numberAt(pos),
              (List<String>) UnsafeUtil.getObject(message, offset(typeAndOffset)),
              writer);
          break;
        case 27: // MESSAGE_LIST:
          SchemaUtil.writeMessageList(
              numberAt(pos),
              (List<?>) UnsafeUtil.getObject(message, offset(typeAndOffset)),
              writer,
              getMessageFieldSchema(pos));
          break;
        case 28: // BYTES_LIST:
          SchemaUtil.writeBytesList(
              numberAt(pos),
              (List<ByteString>) UnsafeUtil.getObject(message, offset(typeAndOffset)),
              writer);
          break;
        case 29: // UINT32_LIST:
          SchemaUtil.writeUInt32List(
              numberAt(pos),
              (List<Integer>) UnsafeUtil.getObject(message, offset(typeAndOffset)),
              writer,
              false);
          break;
        case 30: // ENUM_LIST:
          SchemaUtil.writeEnumList(
              numberAt(pos),
              (List<Integer>) UnsafeUtil.getObject(message, offset(typeAndOffset)),
              writer,
              false);
          break;
        case 31: // SFIXED32_LIST:
          SchemaUtil.writeSFixed32List(
              numberAt(pos),
              (List<Integer>) UnsafeUtil.getObject(message, offset(typeAndOffset)),
              writer,
              false);
          break;
        case 32: // SFIXED64_LIST:
          SchemaUtil.writeSFixed64List(
              numberAt(pos),
              (List<Long>) UnsafeUtil.getObject(message, offset(typeAndOffset)),
              writer,
              false);
          break;
        case 33: // SINT32_LIST:
          SchemaUtil.writeSInt32List(
              numberAt(pos),
              (List<Integer>) UnsafeUtil.getObject(message, offset(typeAndOffset)),
              writer,
              false);
          break;
        case 34: // SINT64_LIST:
          SchemaUtil.writeSInt64List(
              numberAt(pos),
              (List<Long>) UnsafeUtil.getObject(message, offset(typeAndOffset)),
              writer,
              false);
          break;
        case 35: // DOUBLE_LIST_PACKED:
          // TODO(xiaofeng): Make use of cached field size to speed up serialization.
          SchemaUtil.writeDoubleList(
              numberAt(pos),
              (List<Double>) UnsafeUtil.getObject(message, offset(typeAndOffset)),
              writer,
              true);
          break;
        case 36: // FLOAT_LIST_PACKED:
          SchemaUtil.writeFloatList(
              numberAt(pos),
              (List<Float>) UnsafeUtil.getObject(message, offset(typeAndOffset)),
              writer,
              true);
          break;
        case 37: // INT64_LIST_PACKED:
          SchemaUtil.writeInt64List(
              numberAt(pos),
              (List<Long>) UnsafeUtil.getObject(message, offset(typeAndOffset)),
              writer,
              true);
          break;
        case 38: // UINT64_LIST_PACKED:
          SchemaUtil.writeUInt64List(
              numberAt(pos),
              (List<Long>) UnsafeUtil.getObject(message, offset(typeAndOffset)),
              writer,
              true);
          break;
        case 39: // INT32_LIST_PACKED:
          SchemaUtil.writeInt32List(
              numberAt(pos),
              (List<Integer>) UnsafeUtil.getObject(message, offset(typeAndOffset)),
              writer,
              true);
          break;
        case 40: // FIXED64_LIST_PACKED:
          SchemaUtil.writeFixed64List(
              numberAt(pos),
              (List<Long>) UnsafeUtil.getObject(message, offset(typeAndOffset)),
              writer,
              true);
          break;
        case 41: // FIXED32_LIST_PACKED:
          SchemaUtil.writeFixed32List(
              numberAt(pos),
              (List<Integer>) UnsafeUtil.getObject(message, offset(typeAndOffset)),
              writer,
              true);

          break;
        case 42: // BOOL_LIST_PACKED:
          SchemaUtil.writeBoolList(
              numberAt(pos),
              (List<Boolean>) UnsafeUtil.getObject(message, offset(typeAndOffset)),
              writer,
              true);
          break;
        case 43: // UINT32_LIST_PACKED:
          SchemaUtil.writeUInt32List(
              numberAt(pos),
              (List<Integer>) UnsafeUtil.getObject(message, offset(typeAndOffset)),
              writer,
              true);
          break;
        case 44: // ENUM_LIST_PACKED:
          SchemaUtil.writeEnumList(
              numberAt(pos),
              (List<Integer>) UnsafeUtil.getObject(message, offset(typeAndOffset)),
              writer,
              true);
          break;
        case 45: // SFIXED32_LIST_PACKED:
          SchemaUtil.writeSFixed32List(
              numberAt(pos),
              (List<Integer>) UnsafeUtil.getObject(message, offset(typeAndOffset)),
              writer,
              true);
          break;
        case 46: // SFIXED64_LIST_PACKED:
          SchemaUtil.writeSFixed64List(
              numberAt(pos),
              (List<Long>) UnsafeUtil.getObject(message, offset(typeAndOffset)),
              writer,
              true);
          break;
        case 47: // SINT32_LIST_PACKED:
          SchemaUtil.writeSInt32List(
              numberAt(pos),
              (List<Integer>) UnsafeUtil.getObject(message, offset(typeAndOffset)),
              writer,
              true);
          break;
        case 48: // SINT64_LIST_PACKED:
          SchemaUtil.writeSInt64List(
              numberAt(pos),
              (List<Long>) UnsafeUtil.getObject(message, offset(typeAndOffset)),
              writer,
              true);
          break;
        case 49: // GROUP_LIST:
          SchemaUtil.writeGroupList(
              numberAt(pos),
              (List<?>) UnsafeUtil.getObject(message, offset(typeAndOffset)),
              writer,
              getMessageFieldSchema(pos));
          break;
        case 50: // MAP:
          // TODO(dweis): Use schema cache.
          writeMapHelper(writer, number, UnsafeUtil.getObject(message, offset(typeAndOffset)), pos);
          break;
        case 51: // ONEOF_DOUBLE:
          if (isOneofPresent(message, number, pos)) {
            writer.writeDouble(number, oneofDoubleAt(message, offset(typeAndOffset)));
          }
          break;
        case 52: // ONEOF_FLOAT:
          if (isOneofPresent(message, number, pos)) {
            writer.writeFloat(number, oneofFloatAt(message, offset(typeAndOffset)));
          }
          break;
        case 53: // ONEOF_INT64:
          if (isOneofPresent(message, number, pos)) {
            writer.writeInt64(number, oneofLongAt(message, offset(typeAndOffset)));
          }
          break;
        case 54: // ONEOF_UINT64:
          if (isOneofPresent(message, number, pos)) {
            writer.writeUInt64(number, oneofLongAt(message, offset(typeAndOffset)));
          }
          break;
        case 55: // ONEOF_INT32:
          if (isOneofPresent(message, number, pos)) {
            writer.writeInt32(number, oneofIntAt(message, offset(typeAndOffset)));
          }
          break;
        case 56: // ONEOF_FIXED64:
          if (isOneofPresent(message, number, pos)) {
            writer.writeFixed64(number, oneofLongAt(message, offset(typeAndOffset)));
          }
          break;
        case 57: // ONEOF_FIXED32:
          if (isOneofPresent(message, number, pos)) {
            writer.writeFixed32(number, oneofIntAt(message, offset(typeAndOffset)));
          }
          break;
        case 58: // ONEOF_BOOL:
          if (isOneofPresent(message, number, pos)) {
            writer.writeBool(number, oneofBooleanAt(message, offset(typeAndOffset)));
          }
          break;
        case 59: // ONEOF_STRING:
          if (isOneofPresent(message, number, pos)) {
            writeString(number, UnsafeUtil.getObject(message, offset(typeAndOffset)), writer);
          }
          break;
        case 60: // ONEOF_MESSAGE:
          if (isOneofPresent(message, number, pos)) {
            Object value = UnsafeUtil.getObject(message, offset(typeAndOffset));
            writer.writeMessage(number, value, getMessageFieldSchema(pos));
          }
          break;
        case 61: // ONEOF_BYTES:
          if (isOneofPresent(message, number, pos)) {
            writer.writeBytes(
                number, (ByteString) UnsafeUtil.getObject(message, offset(typeAndOffset)));
          }
          break;
        case 62: // ONEOF_UINT32:
          if (isOneofPresent(message, number, pos)) {
            writer.writeUInt32(number, oneofIntAt(message, offset(typeAndOffset)));
          }
          break;
        case 63: // ONEOF_ENUM:
          if (isOneofPresent(message, number, pos)) {
            writer.writeEnum(number, oneofIntAt(message, offset(typeAndOffset)));
          }
          break;
        case 64: // ONEOF_SFIXED32:
          if (isOneofPresent(message, number, pos)) {
            writer.writeSFixed32(number, oneofIntAt(message, offset(typeAndOffset)));
          }
          break;
        case 65: // ONEOF_SFIXED64:
          if (isOneofPresent(message, number, pos)) {
            writer.writeSFixed64(number, oneofLongAt(message, offset(typeAndOffset)));
          }
          break;
        case 66: // ONEOF_SINT32:
          if (isOneofPresent(message, number, pos)) {
            writer.writeSInt32(number, oneofIntAt(message, offset(typeAndOffset)));
          }
          break;
        case 67: // ONEOF_SINT64:
          if (isOneofPresent(message, number, pos)) {
            writer.writeSInt64(number, oneofLongAt(message, offset(typeAndOffset)));
          }
          break;
        case 68: // ONEOF_GROUP:
          if (isOneofPresent(message, number, pos)) {
            writer.writeGroup(
                number,
                UnsafeUtil.getObject(message, offset(typeAndOffset)),
                getMessageFieldSchema(pos));
          }
          break;
        default:
          // Assume it's an empty entry - just go to the next entry.
          break;
      }
    }
    while (nextExtension != null) {
      extensionSchema.serializeExtension(writer, nextExtension);
      nextExtension = extensionIterator.hasNext() ? extensionIterator.next() : null;
    }
    writeUnknownInMessageTo(unknownFieldSchema, message, writer);
  }

  @SuppressWarnings("unchecked")
  private void writeFieldsInDescendingOrder(T message, Writer writer) throws IOException {
    writeUnknownInMessageTo(unknownFieldSchema, message, writer);

    Iterator<? extends Map.Entry<?, ?>> extensionIterator = null;
    Map.Entry nextExtension = null;
    if (hasExtensions) {
      FieldSet<?> extensions = extensionSchema.getExtensions(message);
      if (!extensions.isEmpty()) {
        extensionIterator = extensions.descendingIterator();
        nextExtension = extensionIterator.next();
      }
    }

    for (int pos = buffer.length - INTS_PER_FIELD; pos >= 0; pos -= INTS_PER_FIELD) {
      final int typeAndOffset = typeAndOffsetAt(pos);
      final int number = numberAt(pos);

      // Write any extensions that need to be written before the current field.
      while (nextExtension != null && extensionSchema.extensionNumber(nextExtension) > number) {
        extensionSchema.serializeExtension(writer, nextExtension);
        nextExtension = extensionIterator.hasNext() ? extensionIterator.next() : null;
      }

      switch (type(typeAndOffset)) {
        case 0: // DOUBLE:
          if (isFieldPresent(message, pos)) {
            writer.writeDouble(number, doubleAt(message, offset(typeAndOffset)));
          }
          break;
        case 1: // FLOAT:
          if (isFieldPresent(message, pos)) {
            writer.writeFloat(number, floatAt(message, offset(typeAndOffset)));
          }
          break;
        case 2: // INT64:
          if (isFieldPresent(message, pos)) {
            writer.writeInt64(number, longAt(message, offset(typeAndOffset)));
          }
          break;
        case 3: // UINT64:
          if (isFieldPresent(message, pos)) {
            writer.writeUInt64(number, longAt(message, offset(typeAndOffset)));
          }
          break;
        case 4: // INT32:
          if (isFieldPresent(message, pos)) {
            writer.writeInt32(number, intAt(message, offset(typeAndOffset)));
          }
          break;
        case 5: // FIXED64:
          if (isFieldPresent(message, pos)) {
            writer.writeFixed64(number, longAt(message, offset(typeAndOffset)));
          }
          break;
        case 6: // FIXED32:
          if (isFieldPresent(message, pos)) {
            writer.writeFixed32(number, intAt(message, offset(typeAndOffset)));
          }
          break;
        case 7: // BOOL:
          if (isFieldPresent(message, pos)) {
            writer.writeBool(number, booleanAt(message, offset(typeAndOffset)));
          }
          break;
        case 8: // STRING:
          if (isFieldPresent(message, pos)) {
            writeString(number, UnsafeUtil.getObject(message, offset(typeAndOffset)), writer);
          }
          break;
        case 9: // MESSAGE:
          if (isFieldPresent(message, pos)) {
            Object value = UnsafeUtil.getObject(message, offset(typeAndOffset));
            writer.writeMessage(number, value, getMessageFieldSchema(pos));
          }
          break;
        case 10: // BYTES:
          if (isFieldPresent(message, pos)) {
            writer.writeBytes(
                number, (ByteString) UnsafeUtil.getObject(message, offset(typeAndOffset)));
          }
          break;
        case 11: // UINT32:
          if (isFieldPresent(message, pos)) {
            writer.writeUInt32(number, intAt(message, offset(typeAndOffset)));
          }
          break;
        case 12: // ENUM:
          if (isFieldPresent(message, pos)) {
            writer.writeEnum(number, intAt(message, offset(typeAndOffset)));
          }
          break;
        case 13: // SFIXED32:
          if (isFieldPresent(message, pos)) {
            writer.writeSFixed32(number, intAt(message, offset(typeAndOffset)));
          }
          break;
        case 14: // SFIXED64:
          if (isFieldPresent(message, pos)) {
            writer.writeSFixed64(number, longAt(message, offset(typeAndOffset)));
          }
          break;
        case 15: // SINT32:
          if (isFieldPresent(message, pos)) {
            writer.writeSInt32(number, intAt(message, offset(typeAndOffset)));
          }
          break;
        case 16: // SINT64:
          if (isFieldPresent(message, pos)) {
            writer.writeSInt64(number, longAt(message, offset(typeAndOffset)));
          }
          break;
        case 17: // GROUP:
          if (isFieldPresent(message, pos)) {
            writer.writeGroup(
                number,
                UnsafeUtil.getObject(message, offset(typeAndOffset)),
                getMessageFieldSchema(pos));
          }
          break;
        case 18: // DOUBLE_LIST:
          SchemaUtil.writeDoubleList(
              numberAt(pos),
              (List<Double>) UnsafeUtil.getObject(message, offset(typeAndOffset)),
              writer,
              false);
          break;
        case 19: // FLOAT_LIST:
          SchemaUtil.writeFloatList(
              numberAt(pos),
              (List<Float>) UnsafeUtil.getObject(message, offset(typeAndOffset)),
              writer,
              false);
          break;
        case 20: // INT64_LIST:
          SchemaUtil.writeInt64List(
              numberAt(pos),
              (List<Long>) UnsafeUtil.getObject(message, offset(typeAndOffset)),
              writer,
              false);
          break;
        case 21: // UINT64_LIST:
          SchemaUtil.writeUInt64List(
              numberAt(pos),
              (List<Long>) UnsafeUtil.getObject(message, offset(typeAndOffset)),
              writer,
              false);
          break;
        case 22: // INT32_LIST:
          SchemaUtil.writeInt32List(
              numberAt(pos),
              (List<Integer>) UnsafeUtil.getObject(message, offset(typeAndOffset)),
              writer,
              false);
          break;
        case 23: // FIXED64_LIST:
          SchemaUtil.writeFixed64List(
              numberAt(pos),
              (List<Long>) UnsafeUtil.getObject(message, offset(typeAndOffset)),
              writer,
              false);
          break;
        case 24: // FIXED32_LIST:
          SchemaUtil.writeFixed32List(
              numberAt(pos),
              (List<Integer>) UnsafeUtil.getObject(message, offset(typeAndOffset)),
              writer,
              false);
          break;
        case 25: // BOOL_LIST:
          SchemaUtil.writeBoolList(
              numberAt(pos),
              (List<Boolean>) UnsafeUtil.getObject(message, offset(typeAndOffset)),
              writer,
              false);
          break;
        case 26: // STRING_LIST:
          SchemaUtil.writeStringList(
              numberAt(pos),
              (List<String>) UnsafeUtil.getObject(message, offset(typeAndOffset)),
              writer);
          break;
        case 27: // MESSAGE_LIST:
          SchemaUtil.writeMessageList(
              numberAt(pos),
              (List<?>) UnsafeUtil.getObject(message, offset(typeAndOffset)),
              writer,
              getMessageFieldSchema(pos));
          break;
        case 28: // BYTES_LIST:
          SchemaUtil.writeBytesList(
              numberAt(pos),
              (List<ByteString>) UnsafeUtil.getObject(message, offset(typeAndOffset)),
              writer);
          break;
        case 29: // UINT32_LIST:
          SchemaUtil.writeUInt32List(
              numberAt(pos),
              (List<Integer>) UnsafeUtil.getObject(message, offset(typeAndOffset)),
              writer,
              false);
          break;
        case 30: // ENUM_LIST:
          SchemaUtil.writeEnumList(
              numberAt(pos),
              (List<Integer>) UnsafeUtil.getObject(message, offset(typeAndOffset)),
              writer,
              false);
          break;
        case 31: // SFIXED32_LIST:
          SchemaUtil.writeSFixed32List(
              numberAt(pos),
              (List<Integer>) UnsafeUtil.getObject(message, offset(typeAndOffset)),
              writer,
              false);
          break;
        case 32: // SFIXED64_LIST:
          SchemaUtil.writeSFixed64List(
              numberAt(pos),
              (List<Long>) UnsafeUtil.getObject(message, offset(typeAndOffset)),
              writer,
              false);
          break;
        case 33: // SINT32_LIST:
          SchemaUtil.writeSInt32List(
              numberAt(pos),
              (List<Integer>) UnsafeUtil.getObject(message, offset(typeAndOffset)),
              writer,
              false);
          break;
        case 34: // SINT64_LIST:
          SchemaUtil.writeSInt64List(
              numberAt(pos),
              (List<Long>) UnsafeUtil.getObject(message, offset(typeAndOffset)),
              writer,
              false);
          break;
        case 35: // DOUBLE_LIST_PACKED:
          SchemaUtil.writeDoubleList(
              numberAt(pos),
              (List<Double>) UnsafeUtil.getObject(message, offset(typeAndOffset)),
              writer,
              true);
          break;
        case 36: // FLOAT_LIST_PACKED:
          SchemaUtil.writeFloatList(
              numberAt(pos),
              (List<Float>) UnsafeUtil.getObject(message, offset(typeAndOffset)),
              writer,
              true);
          break;
        case 37: // INT64_LIST_PACKED:
          SchemaUtil.writeInt64List(
              numberAt(pos),
              (List<Long>) UnsafeUtil.getObject(message, offset(typeAndOffset)),
              writer,
              true);
          break;
        case 38: // UINT64_LIST_PACKED:
          SchemaUtil.writeUInt64List(
              numberAt(pos),
              (List<Long>) UnsafeUtil.getObject(message, offset(typeAndOffset)),
              writer,
              true);
          break;
        case 39: // INT32_LIST_PACKED:
          SchemaUtil.writeInt32List(
              numberAt(pos),
              (List<Integer>) UnsafeUtil.getObject(message, offset(typeAndOffset)),
              writer,
              true);
          break;
        case 40: // FIXED64_LIST_PACKED:
          SchemaUtil.writeFixed64List(
              numberAt(pos),
              (List<Long>) UnsafeUtil.getObject(message, offset(typeAndOffset)),
              writer,
              true);
          break;
        case 41: // FIXED32_LIST_PACKED:
          SchemaUtil.writeFixed32List(
              numberAt(pos),
              (List<Integer>) UnsafeUtil.getObject(message, offset(typeAndOffset)),
              writer,
              true);

          break;
        case 42: // BOOL_LIST_PACKED:
          SchemaUtil.writeBoolList(
              numberAt(pos),
              (List<Boolean>) UnsafeUtil.getObject(message, offset(typeAndOffset)),
              writer,
              true);
          break;
        case 43: // UINT32_LIST_PACKED:
          SchemaUtil.writeUInt32List(
              numberAt(pos),
              (List<Integer>) UnsafeUtil.getObject(message, offset(typeAndOffset)),
              writer,
              true);
          break;
        case 44: // ENUM_LIST_PACKED:
          SchemaUtil.writeEnumList(
              numberAt(pos),
              (List<Integer>) UnsafeUtil.getObject(message, offset(typeAndOffset)),
              writer,
              true);
          break;
        case 45: // SFIXED32_LIST_PACKED:
          SchemaUtil.writeSFixed32List(
              numberAt(pos),
              (List<Integer>) UnsafeUtil.getObject(message, offset(typeAndOffset)),
              writer,
              true);
          break;
        case 46: // SFIXED64_LIST_PACKED:
          SchemaUtil.writeSFixed64List(
              numberAt(pos),
              (List<Long>) UnsafeUtil.getObject(message, offset(typeAndOffset)),
              writer,
              true);
          break;
        case 47: // SINT32_LIST_PACKED:
          SchemaUtil.writeSInt32List(
              numberAt(pos),
              (List<Integer>) UnsafeUtil.getObject(message, offset(typeAndOffset)),
              writer,
              true);
          break;
        case 48: // SINT64_LIST_PACKED:
          SchemaUtil.writeSInt64List(
              numberAt(pos),
              (List<Long>) UnsafeUtil.getObject(message, offset(typeAndOffset)),
              writer,
              true);
          break;
        case 49: // GROUP_LIST:
          SchemaUtil.writeGroupList(
              numberAt(pos),
              (List<?>) UnsafeUtil.getObject(message, offset(typeAndOffset)),
              writer,
              getMessageFieldSchema(pos));
          break;
        case 50: // MAP:
          // TODO(dweis): Use schema cache.
          writeMapHelper(writer, number, UnsafeUtil.getObject(message, offset(typeAndOffset)), pos);
          break;
        case 51: // ONEOF_DOUBLE:
          if (isOneofPresent(message, number, pos)) {
            writer.writeDouble(number, oneofDoubleAt(message, offset(typeAndOffset)));
          }
          break;
        case 52: // ONEOF_FLOAT:
          if (isOneofPresent(message, number, pos)) {
            writer.writeFloat(number, oneofFloatAt(message, offset(typeAndOffset)));
          }
          break;
        case 53: // ONEOF_INT64:
          if (isOneofPresent(message, number, pos)) {
            writer.writeInt64(number, oneofLongAt(message, offset(typeAndOffset)));
          }
          break;
        case 54: // ONEOF_UINT64:
          if (isOneofPresent(message, number, pos)) {
            writer.writeUInt64(number, oneofLongAt(message, offset(typeAndOffset)));
          }
          break;
        case 55: // ONEOF_INT32:
          if (isOneofPresent(message, number, pos)) {
            writer.writeInt32(number, oneofIntAt(message, offset(typeAndOffset)));
          }
          break;
        case 56: // ONEOF_FIXED64:
          if (isOneofPresent(message, number, pos)) {
            writer.writeFixed64(number, oneofLongAt(message, offset(typeAndOffset)));
          }
          break;
        case 57: // ONEOF_FIXED32:
          if (isOneofPresent(message, number, pos)) {
            writer.writeFixed32(number, oneofIntAt(message, offset(typeAndOffset)));
          }
          break;
        case 58: // ONEOF_BOOL:
          if (isOneofPresent(message, number, pos)) {
            writer.writeBool(number, oneofBooleanAt(message, offset(typeAndOffset)));
          }
          break;
        case 59: // ONEOF_STRING:
          if (isOneofPresent(message, number, pos)) {
            writeString(number, UnsafeUtil.getObject(message, offset(typeAndOffset)), writer);
          }
          break;
        case 60: // ONEOF_MESSAGE:
          if (isOneofPresent(message, number, pos)) {
            Object value = UnsafeUtil.getObject(message, offset(typeAndOffset));
            writer.writeMessage(number, value, getMessageFieldSchema(pos));
          }
          break;
        case 61: // ONEOF_BYTES:
          if (isOneofPresent(message, number, pos)) {
            writer.writeBytes(
                number, (ByteString) UnsafeUtil.getObject(message, offset(typeAndOffset)));
          }
          break;
        case 62: // ONEOF_UINT32:
          if (isOneofPresent(message, number, pos)) {
            writer.writeUInt32(number, oneofIntAt(message, offset(typeAndOffset)));
          }
          break;
        case 63: // ONEOF_ENUM:
          if (isOneofPresent(message, number, pos)) {
            writer.writeEnum(number, oneofIntAt(message, offset(typeAndOffset)));
          }
          break;
        case 64: // ONEOF_SFIXED32:
          if (isOneofPresent(message, number, pos)) {
            writer.writeSFixed32(number, oneofIntAt(message, offset(typeAndOffset)));
          }
          break;
        case 65: // ONEOF_SFIXED64:
          if (isOneofPresent(message, number, pos)) {
            writer.writeSFixed64(number, oneofLongAt(message, offset(typeAndOffset)));
          }
          break;
        case 66: // ONEOF_SINT32:
          if (isOneofPresent(message, number, pos)) {
            writer.writeSInt32(number, oneofIntAt(message, offset(typeAndOffset)));
          }
          break;
        case 67: // ONEOF_SINT64:
          if (isOneofPresent(message, number, pos)) {
            writer.writeSInt64(number, oneofLongAt(message, offset(typeAndOffset)));
          }
          break;
        case 68: // ONEOF_GROUP:
          if (isOneofPresent(message, number, pos)) {
            writer.writeGroup(
                number,
                UnsafeUtil.getObject(message, offset(typeAndOffset)),
                getMessageFieldSchema(pos));
          }
          break;
        default:
          break;
      }
    }
    while (nextExtension != null) {
      extensionSchema.serializeExtension(writer, nextExtension);
      nextExtension = extensionIterator.hasNext() ? extensionIterator.next() : null;
    }
  }

  @SuppressWarnings("unchecked")
  private <K, V> void writeMapHelper(Writer writer, int number, Object mapField, int pos)
      throws IOException {
    if (mapField != null) {
      writer.writeMap(
          number,
          (MapEntryLite.Metadata<K, V>) mapFieldSchema.forMapMetadata(getMapFieldDefaultEntry(pos)),
          (Map<K, V>) mapFieldSchema.forMapData(mapField));
    }
  }

  private <UT, UB> void writeUnknownInMessageTo(
      UnknownFieldSchema<UT, UB> schema, T message, Writer writer) throws IOException {
    schema.writeTo(schema.getFromMessage(message), writer);
  }

  @Override
  public void mergeFrom(T message, Reader reader, ExtensionRegistryLite extensionRegistry)
      throws IOException {
    if (extensionRegistry == null) {
      throw new NullPointerException();
    }
    mergeFromHelper(unknownFieldSchema, extensionSchema, message, reader, extensionRegistry);
  }

  /**
   * A helper method for wildcard capture of {@code unknownFieldSchema}. See:
   * https://docs.oracle.com/javase/tutorial/java/generics/capture.html
   */
  private <UT, UB, ET extends FieldDescriptorLite<ET>> void mergeFromHelper(
      UnknownFieldSchema<UT, UB> unknownFieldSchema,
      ExtensionSchema<ET> extensionSchema,
      T message,
      Reader reader,
      ExtensionRegistryLite extensionRegistry)
      throws IOException {
    UB unknownFields = null;
    FieldSet<ET> extensions = null;
    try {
      while (true) {
        final int number = reader.getFieldNumber();
        final int pos = positionForFieldNumber(number);
        if (pos < 0) {
          if (number == Reader.READ_DONE) {
            return;
          }
          // Check if it's an extension.
          Object extension =
              !hasExtensions
                  ? null
                  : extensionSchema.findExtensionByNumber(
                      extensionRegistry, defaultInstance, number);
          if (extension != null) {
            if (extensions == null) {
              extensions = extensionSchema.getMutableExtensions(message);
            }
            unknownFields =
                extensionSchema.parseExtension(
                    reader,
                    extension,
                    extensionRegistry,
                    extensions,
                    unknownFields,
                    unknownFieldSchema);
            continue;
          }
          if (unknownFieldSchema.shouldDiscardUnknownFields(reader)) {
            if (reader.skipField()) {
              continue;
            }
          } else {
            if (unknownFields == null) {
              unknownFields = unknownFieldSchema.getBuilderFromMessage(message);
            }
            // Unknown field.
            if (unknownFieldSchema.mergeOneFieldFrom(unknownFields, reader)) {
              continue;
            }
          }
          // Done reading.
          return;
        }
        final int typeAndOffset = typeAndOffsetAt(pos);

        try {
          switch (type(typeAndOffset)) {
            case 0: // DOUBLE:
              UnsafeUtil.putDouble(message, offset(typeAndOffset), reader.readDouble());
              setFieldPresent(message, pos);
              break;
            case 1: // FLOAT:
              UnsafeUtil.putFloat(message, offset(typeAndOffset), reader.readFloat());
              setFieldPresent(message, pos);
              break;
            case 2: // INT64:
              UnsafeUtil.putLong(message, offset(typeAndOffset), reader.readInt64());
              setFieldPresent(message, pos);
              break;
            case 3: // UINT64:
              UnsafeUtil.putLong(message, offset(typeAndOffset), reader.readUInt64());
              setFieldPresent(message, pos);
              break;
            case 4: // INT32:
              UnsafeUtil.putInt(message, offset(typeAndOffset), reader.readInt32());
              setFieldPresent(message, pos);
              break;
            case 5: // FIXED64:
              UnsafeUtil.putLong(message, offset(typeAndOffset), reader.readFixed64());
              setFieldPresent(message, pos);
              break;
            case 6: // FIXED32:
              UnsafeUtil.putInt(message, offset(typeAndOffset), reader.readFixed32());
              setFieldPresent(message, pos);
              break;
            case 7: // BOOL:
              UnsafeUtil.putBoolean(message, offset(typeAndOffset), reader.readBool());
              setFieldPresent(message, pos);
              break;
            case 8: // STRING:
              readString(message, typeAndOffset, reader);
              setFieldPresent(message, pos);
              break;
            case 9:
              { // MESSAGE:
                if (isFieldPresent(message, pos)) {
                  Object mergedResult =
                      Internal.mergeMessage(
                          UnsafeUtil.getObject(message, offset(typeAndOffset)),
                          reader.readMessageBySchemaWithCheck(
                              (Schema<T>) getMessageFieldSchema(pos), extensionRegistry));
                  UnsafeUtil.putObject(message, offset(typeAndOffset), mergedResult);
                } else {
                  UnsafeUtil.putObject(
                      message,
                      offset(typeAndOffset),
                      reader.readMessageBySchemaWithCheck(
                          (Schema<T>) getMessageFieldSchema(pos), extensionRegistry));
                  setFieldPresent(message, pos);
                }
                break;
              }
            case 10: // BYTES:
              UnsafeUtil.putObject(message, offset(typeAndOffset), reader.readBytes());
              setFieldPresent(message, pos);
              break;
            case 11: // UINT32:
              UnsafeUtil.putInt(message, offset(typeAndOffset), reader.readUInt32());
              setFieldPresent(message, pos);
              break;
            case 12: // ENUM:
              {
                int enumValue = reader.readEnum();
                EnumVerifier enumVerifier = getEnumFieldVerifier(pos);
                if (enumVerifier == null || enumVerifier.isInRange(enumValue)) {
                  UnsafeUtil.putInt(message, offset(typeAndOffset), enumValue);
                  setFieldPresent(message, pos);
                } else {
                  unknownFields =
                      SchemaUtil.storeUnknownEnum(
                          number, enumValue, unknownFields, unknownFieldSchema);
                }
                break;
              }
            case 13: // SFIXED32:
              UnsafeUtil.putInt(message, offset(typeAndOffset), reader.readSFixed32());
              setFieldPresent(message, pos);
              break;
            case 14: // SFIXED64:
              UnsafeUtil.putLong(message, offset(typeAndOffset), reader.readSFixed64());
              setFieldPresent(message, pos);
              break;
            case 15: // SINT32:
              UnsafeUtil.putInt(message, offset(typeAndOffset), reader.readSInt32());
              setFieldPresent(message, pos);
              break;
            case 16: // SINT64:
              UnsafeUtil.putLong(message, offset(typeAndOffset), reader.readSInt64());
              setFieldPresent(message, pos);
              break;
            case 17:
              { // GROUP:
                if (isFieldPresent(message, pos)) {
                  Object mergedResult =
                      Internal.mergeMessage(
                          UnsafeUtil.getObject(message, offset(typeAndOffset)),
                          reader.readGroupBySchemaWithCheck(
                              (Schema<T>) getMessageFieldSchema(pos), extensionRegistry));
                  UnsafeUtil.putObject(message, offset(typeAndOffset), mergedResult);
                } else {
                  UnsafeUtil.putObject(
                      message,
                      offset(typeAndOffset),
                      reader.readGroupBySchemaWithCheck(
                          (Schema<T>) getMessageFieldSchema(pos), extensionRegistry));
                  setFieldPresent(message, pos);
                }
                break;
              }
            case 18: // DOUBLE_LIST:
              reader.readDoubleList(
                  listFieldSchema.<Double>mutableListAt(message, offset(typeAndOffset)));
              break;
            case 19: // FLOAT_LIST:
              reader.readFloatList(
                  listFieldSchema.<Float>mutableListAt(message, offset(typeAndOffset)));
              break;
            case 20: // INT64_LIST:
              reader.readInt64List(
                  listFieldSchema.<Long>mutableListAt(message, offset(typeAndOffset)));
              break;
            case 21: // UINT64_LIST:
              reader.readUInt64List(
                  listFieldSchema.<Long>mutableListAt(message, offset(typeAndOffset)));
              break;
            case 22: // INT32_LIST:
              reader.readInt32List(
                  listFieldSchema.<Integer>mutableListAt(message, offset(typeAndOffset)));
              break;
            case 23: // FIXED64_LIST:
              reader.readFixed64List(
                  listFieldSchema.<Long>mutableListAt(message, offset(typeAndOffset)));
              break;
            case 24: // FIXED32_LIST:
              reader.readFixed32List(
                  listFieldSchema.<Integer>mutableListAt(message, offset(typeAndOffset)));
              break;
            case 25: // BOOL_LIST:
              reader.readBoolList(
                  listFieldSchema.<Boolean>mutableListAt(message, offset(typeAndOffset)));
              break;
            case 26: // STRING_LIST:
              readStringList(message, typeAndOffset, reader);
              break;
            case 27:
              { // MESSAGE_LIST:
                readMessageList(
                    message,
                    typeAndOffset,
                    reader,
                    (Schema<T>) getMessageFieldSchema(pos),
                    extensionRegistry);
                break;
              }
            case 28: // BYTES_LIST:
              reader.readBytesList(
                  listFieldSchema.<ByteString>mutableListAt(message, offset(typeAndOffset)));
              break;
            case 29: // UINT32_LIST:
              reader.readUInt32List(
                  listFieldSchema.<Integer>mutableListAt(message, offset(typeAndOffset)));
              break;
            case 30: // ENUM_LIST:
              {
                List<Integer> enumList =
                    listFieldSchema.<Integer>mutableListAt(message, offset(typeAndOffset));
                reader.readEnumList(enumList);
                unknownFields =
                    SchemaUtil.filterUnknownEnumList(
                        number,
                        enumList,
                        getEnumFieldVerifier(pos),
                        unknownFields,
                        unknownFieldSchema);
                break;
              }
            case 31: // SFIXED32_LIST:
              reader.readSFixed32List(
                  listFieldSchema.<Integer>mutableListAt(message, offset(typeAndOffset)));
              break;
            case 32: // SFIXED64_LIST:
              reader.readSFixed64List(
                  listFieldSchema.<Long>mutableListAt(message, offset(typeAndOffset)));
              break;
            case 33: // SINT32_LIST:
              reader.readSInt32List(
                  listFieldSchema.<Integer>mutableListAt(message, offset(typeAndOffset)));
              break;
            case 34: // SINT64_LIST:
              reader.readSInt64List(
                  listFieldSchema.<Long>mutableListAt(message, offset(typeAndOffset)));
              break;
            case 35: // DOUBLE_LIST_PACKED:
              reader.readDoubleList(
                  listFieldSchema.<Double>mutableListAt(message, offset(typeAndOffset)));
              break;
            case 36: // FLOAT_LIST_PACKED:
              reader.readFloatList(
                  listFieldSchema.<Float>mutableListAt(message, offset(typeAndOffset)));
              break;
            case 37: // INT64_LIST_PACKED:
              reader.readInt64List(
                  listFieldSchema.<Long>mutableListAt(message, offset(typeAndOffset)));
              break;
            case 38: // UINT64_LIST_PACKED:
              reader.readUInt64List(
                  listFieldSchema.<Long>mutableListAt(message, offset(typeAndOffset)));
              break;
            case 39: // INT32_LIST_PACKED:
              reader.readInt32List(
                  listFieldSchema.<Integer>mutableListAt(message, offset(typeAndOffset)));
              break;
            case 40: // FIXED64_LIST_PACKED:
              reader.readFixed64List(
                  listFieldSchema.<Long>mutableListAt(message, offset(typeAndOffset)));
              break;
            case 41: // FIXED32_LIST_PACKED:
              reader.readFixed32List(
                  listFieldSchema.<Integer>mutableListAt(message, offset(typeAndOffset)));
              break;
            case 42: // BOOL_LIST_PACKED:
              reader.readBoolList(
                  listFieldSchema.<Boolean>mutableListAt(message, offset(typeAndOffset)));
              break;
            case 43: // UINT32_LIST_PACKED:
              reader.readUInt32List(
                  listFieldSchema.<Integer>mutableListAt(message, offset(typeAndOffset)));
              break;
            case 44: // ENUM_LIST_PACKED:
              {
                List<Integer> enumList =
                    listFieldSchema.<Integer>mutableListAt(message, offset(typeAndOffset));
                reader.readEnumList(enumList);
                unknownFields =
                    SchemaUtil.filterUnknownEnumList(
                        number,
                        enumList,
                        getEnumFieldVerifier(pos),
                        unknownFields,
                        unknownFieldSchema);
                break;
              }
            case 45: // SFIXED32_LIST_PACKED:
              reader.readSFixed32List(
                  listFieldSchema.<Integer>mutableListAt(message, offset(typeAndOffset)));
              break;
            case 46: // SFIXED64_LIST_PACKED:
              reader.readSFixed64List(
                  listFieldSchema.<Long>mutableListAt(message, offset(typeAndOffset)));
              break;
            case 47: // SINT32_LIST_PACKED:
              reader.readSInt32List(
                  listFieldSchema.<Integer>mutableListAt(message, offset(typeAndOffset)));
              break;
            case 48: // SINT64_LIST_PACKED:
              reader.readSInt64List(
                  listFieldSchema.<Long>mutableListAt(message, offset(typeAndOffset)));
              break;
            case 49:
              { // GROUP_LIST:
                readGroupList(
                    message,
                    offset(typeAndOffset),
                    reader,
                    (Schema<T>) getMessageFieldSchema(pos),
                    extensionRegistry);
                break;
              }
            case 50: // MAP:
              mergeMap(message, pos, getMapFieldDefaultEntry(pos), extensionRegistry, reader);
              break;
            case 51: // ONEOF_DOUBLE:
              UnsafeUtil.putObject(
                  message, offset(typeAndOffset), Double.valueOf(reader.readDouble()));
              setOneofPresent(message, number, pos);
              break;
            case 52: // ONEOF_FLOAT:
              UnsafeUtil.putObject(
                  message, offset(typeAndOffset), Float.valueOf(reader.readFloat()));
              setOneofPresent(message, number, pos);
              break;
            case 53: // ONEOF_INT64:
              UnsafeUtil.putObject(
                  message, offset(typeAndOffset), Long.valueOf(reader.readInt64()));
              setOneofPresent(message, number, pos);
              break;
            case 54: // ONEOF_UINT64:
              UnsafeUtil.putObject(
                  message, offset(typeAndOffset), Long.valueOf(reader.readUInt64()));
              setOneofPresent(message, number, pos);
              break;
            case 55: // ONEOF_INT32:
              UnsafeUtil.putObject(
                  message, offset(typeAndOffset), Integer.valueOf(reader.readInt32()));
              setOneofPresent(message, number, pos);
              break;
            case 56: // ONEOF_FIXED64:
              UnsafeUtil.putObject(
                  message, offset(typeAndOffset), Long.valueOf(reader.readFixed64()));
              setOneofPresent(message, number, pos);
              break;
            case 57: // ONEOF_FIXED32:
              UnsafeUtil.putObject(
                  message, offset(typeAndOffset), Integer.valueOf(reader.readFixed32()));
              setOneofPresent(message, number, pos);
              break;
            case 58: // ONEOF_BOOL:
              UnsafeUtil.putObject(
                  message, offset(typeAndOffset), Boolean.valueOf(reader.readBool()));
              setOneofPresent(message, number, pos);
              break;
            case 59: // ONEOF_STRING:
              readString(message, typeAndOffset, reader);
              setOneofPresent(message, number, pos);
              break;
            case 60: // ONEOF_MESSAGE:
              if (isOneofPresent(message, number, pos)) {
                Object mergedResult =
                    Internal.mergeMessage(
                        UnsafeUtil.getObject(message, offset(typeAndOffset)),
                        reader.readMessageBySchemaWithCheck(
                            getMessageFieldSchema(pos), extensionRegistry));
                UnsafeUtil.putObject(message, offset(typeAndOffset), mergedResult);
              } else {
                UnsafeUtil.putObject(
                    message,
                    offset(typeAndOffset),
                    reader.readMessageBySchemaWithCheck(
                        getMessageFieldSchema(pos), extensionRegistry));
                setFieldPresent(message, pos);
              }
              setOneofPresent(message, number, pos);
              break;
            case 61: // ONEOF_BYTES:
              UnsafeUtil.putObject(message, offset(typeAndOffset), reader.readBytes());
              setOneofPresent(message, number, pos);
              break;
            case 62: // ONEOF_UINT32:
              UnsafeUtil.putObject(
                  message, offset(typeAndOffset), Integer.valueOf(reader.readUInt32()));
              setOneofPresent(message, number, pos);
              break;
            case 63: // ONEOF_ENUM:
              {
                int enumValue = reader.readEnum();
                EnumVerifier enumVerifier = getEnumFieldVerifier(pos);
                if (enumVerifier == null || enumVerifier.isInRange(enumValue)) {
                  UnsafeUtil.putObject(message, offset(typeAndOffset), enumValue);
                  setOneofPresent(message, number, pos);
                } else {
                  unknownFields =
                      SchemaUtil.storeUnknownEnum(
                          number, enumValue, unknownFields, unknownFieldSchema);
                }
                break;
              }
            case 64: // ONEOF_SFIXED32:
              UnsafeUtil.putObject(
                  message, offset(typeAndOffset), Integer.valueOf(reader.readSFixed32()));
              setOneofPresent(message, number, pos);
              break;
            case 65: // ONEOF_SFIXED64:
              UnsafeUtil.putObject(
                  message, offset(typeAndOffset), Long.valueOf(reader.readSFixed64()));
              setOneofPresent(message, number, pos);
              break;
            case 66: // ONEOF_SINT32:
              UnsafeUtil.putObject(
                  message, offset(typeAndOffset), Integer.valueOf(reader.readSInt32()));
              setOneofPresent(message, number, pos);
              break;
            case 67: // ONEOF_SINT64:
              UnsafeUtil.putObject(
                  message, offset(typeAndOffset), Long.valueOf(reader.readSInt64()));
              setOneofPresent(message, number, pos);
              break;
            case 68: // ONEOF_GROUP:
              UnsafeUtil.putObject(
                  message,
                  offset(typeAndOffset),
                  reader.readGroupBySchemaWithCheck(getMessageFieldSchema(pos), extensionRegistry));
              setOneofPresent(message, number, pos);
              break;
            default:
              // Assume we've landed on an empty entry. Treat it as an unknown field.
              if (unknownFields == null) {
                unknownFields = unknownFieldSchema.newBuilder();
              }
              if (!unknownFieldSchema.mergeOneFieldFrom(unknownFields, reader)) {
                return;
              }
              break;
          }
        } catch (InvalidProtocolBufferException.InvalidWireTypeException e) {
          // Treat fields with an invalid wire type as unknown fields
          // (i.e. same as the default case).
          if (unknownFieldSchema.shouldDiscardUnknownFields(reader)) {
            if (!reader.skipField()) {
              return;
            }
          } else {
            if (unknownFields == null) {
              unknownFields = unknownFieldSchema.getBuilderFromMessage(message);
            }
            if (!unknownFieldSchema.mergeOneFieldFrom(unknownFields, reader)) {
              return;
            }
          }
        }
      }
    } finally {
      for (int i = checkInitializedCount; i < repeatedFieldOffsetStart; i++) {
        unknownFields =
            filterMapUnknownEnumValues(message, intArray[i], unknownFields, unknownFieldSchema);
      }
      if (unknownFields != null) {
        unknownFieldSchema.setBuilderToMessage(message, unknownFields);
      }
    }
  }

  @SuppressWarnings("ReferenceEquality")
  static UnknownFieldSetLite getMutableUnknownFields(Object message) {
    UnknownFieldSetLite unknownFields = ((GeneratedMessageLite) message).unknownFields;
    if (unknownFields == UnknownFieldSetLite.getDefaultInstance()) {
      unknownFields = UnknownFieldSetLite.newInstance();
      ((GeneratedMessageLite) message).unknownFields = unknownFields;
    }
    return unknownFields;
  }

  /** Decodes a map entry key or value. Stores result in registers.object1. */
  private int decodeMapEntryValue(
      byte[] data,
      int position,
      int limit,
      WireFormat.FieldType fieldType,
      Class<?> messageType,
      Registers registers)
      throws IOException {
    switch (fieldType) {
      case BOOL:
        position = decodeVarint64(data, position, registers);
        registers.object1 = registers.long1 != 0;
        break;
      case BYTES:
        position = decodeBytes(data, position, registers);
        break;
      case DOUBLE:
        registers.object1 = decodeDouble(data, position);
        position += 8;
        break;
      case FIXED32:
      case SFIXED32:
        registers.object1 = decodeFixed32(data, position);
        position += 4;
        break;
      case FIXED64:
      case SFIXED64:
        registers.object1 = decodeFixed64(data, position);
        position += 8;
        break;
      case FLOAT:
        registers.object1 = decodeFloat(data, position);
        position += 4;
        break;
      case ENUM:
      case INT32:
      case UINT32:
        position = decodeVarint32(data, position, registers);
        registers.object1 = registers.int1;
        break;
      case INT64:
      case UINT64:
        position = decodeVarint64(data, position, registers);
        registers.object1 = registers.long1;
        break;
      case MESSAGE:
        position =
            decodeMessageField(
                Protobuf.getInstance().schemaFor(messageType), data, position, limit, registers);
        break;
      case SINT32:
        position = decodeVarint32(data, position, registers);
        registers.object1 = CodedInputStream.decodeZigZag32(registers.int1);
        break;
      case SINT64:
        position = decodeVarint64(data, position, registers);
        registers.object1 = CodedInputStream.decodeZigZag64(registers.long1);
        break;
      case STRING:
        position = decodeStringRequireUtf8(data, position, registers);
        break;
      default:
        throw new RuntimeException("unsupported field type.");
    }
    return position;
  }

  /** Decodes a map entry. */
  private <K, V> int decodeMapEntry(
      byte[] data,
      int position,
      int limit,
      MapEntryLite.Metadata<K, V> metadata,
      Map<K, V> target,
      Registers registers)
      throws IOException {
    position = decodeVarint32(data, position, registers);
    final int length = registers.int1;
    if (length < 0 || length > limit - position) {
      throw InvalidProtocolBufferException.truncatedMessage();
    }
    final int end = position + length;
    K key = metadata.defaultKey;
    V value = metadata.defaultValue;
    while (position < end) {
      int tag = data[position++];
      if (tag < 0) {
        position = decodeVarint32(tag, data, position, registers);
        tag = registers.int1;
      }
      final int fieldNumber = tag >>> 3;
      final int wireType = tag & 0x7;
      switch (fieldNumber) {
        case 1:
          if (wireType == metadata.keyType.getWireType()) {
            position =
                decodeMapEntryValue(data, position, limit, metadata.keyType, null, registers);
            key = (K) registers.object1;
            continue;
          }
          break;
        case 2:
          if (wireType == metadata.valueType.getWireType()) {
            position =
                decodeMapEntryValue(
                    data,
                    position,
                    limit,
                    metadata.valueType,
                    metadata.defaultValue.getClass(),
                    registers);
            value = (V) registers.object1;
            continue;
          }
          break;
        default:
          break;
      }
      position = skipField(tag, data, position, limit, registers);
    }
    if (position != end) {
      throw InvalidProtocolBufferException.parseFailure();
    }
    target.put(key, value);
    return end;
  }

  @SuppressWarnings("ReferenceEquality")
  private int parseRepeatedField(
      T message,
      byte[] data,
      int position,
      int limit,
      int tag,
      int number,
      int wireType,
      int bufferPosition,
      long typeAndOffset,
      int fieldType,
      long fieldOffset,
      Registers registers)
      throws IOException {
    ProtobufList<?> list = (ProtobufList<?>) UNSAFE.getObject(message, fieldOffset);
    if (!list.isModifiable()) {
      final int size = list.size();
      list =
          list.mutableCopyWithCapacity(
              size == 0 ? AbstractProtobufList.DEFAULT_CAPACITY : size * 2);
      UNSAFE.putObject(message, fieldOffset, list);
    }
    switch (fieldType) {
      case 18: // DOUBLE_LIST:
      case 35: // DOUBLE_LIST_PACKED:
        if (wireType == WireFormat.WIRETYPE_LENGTH_DELIMITED) {
          position = decodePackedDoubleList(data, position, list, registers);
        } else if (wireType == WireFormat.WIRETYPE_FIXED64) {
          position = decodeDoubleList(tag, data, position, limit, list, registers);
        }
        break;
      case 19: // FLOAT_LIST:
      case 36: // FLOAT_LIST_PACKED:
        if (wireType == WireFormat.WIRETYPE_LENGTH_DELIMITED) {
          position = decodePackedFloatList(data, position, list, registers);
        } else if (wireType == WireFormat.WIRETYPE_FIXED32) {
          position = decodeFloatList(tag, data, position, limit, list, registers);
        }
        break;
      case 20: // INT64_LIST:
      case 21: // UINT64_LIST:
      case 37: // INT64_LIST_PACKED:
      case 38: // UINT64_LIST_PACKED:
        if (wireType == WireFormat.WIRETYPE_LENGTH_DELIMITED) {
          position = decodePackedVarint64List(data, position, list, registers);
        } else if (wireType == WireFormat.WIRETYPE_VARINT) {
          position = decodeVarint64List(tag, data, position, limit, list, registers);
        }
        break;
      case 22: // INT32_LIST:
      case 29: // UINT32_LIST:
      case 39: // INT32_LIST_PACKED:
      case 43: // UINT32_LIST_PACKED:
        if (wireType == WireFormat.WIRETYPE_LENGTH_DELIMITED) {
          position = decodePackedVarint32List(data, position, list, registers);
        } else if (wireType == WireFormat.WIRETYPE_VARINT) {
          position = decodeVarint32List(tag, data, position, limit, list, registers);
        }
        break;
      case 23: // FIXED64_LIST:
      case 32: // SFIXED64_LIST:
      case 40: // FIXED64_LIST_PACKED:
      case 46: // SFIXED64_LIST_PACKED:
        if (wireType == WireFormat.WIRETYPE_LENGTH_DELIMITED) {
          position = decodePackedFixed64List(data, position, list, registers);
        } else if (wireType == WireFormat.WIRETYPE_FIXED64) {
          position = decodeFixed64List(tag, data, position, limit, list, registers);
        }
        break;
      case 24: // FIXED32_LIST:
      case 31: // SFIXED32_LIST:
      case 41: // FIXED32_LIST_PACKED:
      case 45: // SFIXED32_LIST_PACKED:
        if (wireType == WireFormat.WIRETYPE_LENGTH_DELIMITED) {
          position = decodePackedFixed32List(data, position, list, registers);
        } else if (wireType == WireFormat.WIRETYPE_FIXED32) {
          position = decodeFixed32List(tag, data, position, limit, list, registers);
        }
        break;
      case 25: // BOOL_LIST:
      case 42: // BOOL_LIST_PACKED:
        if (wireType == WireFormat.WIRETYPE_LENGTH_DELIMITED) {
          position = decodePackedBoolList(data, position, list, registers);
        } else if (wireType == WireFormat.WIRETYPE_VARINT) {
          position = decodeBoolList(tag, data, position, limit, list, registers);
        }
        break;
      case 26: // STRING_LIST:
        if (wireType == WireFormat.WIRETYPE_LENGTH_DELIMITED) {
          if ((typeAndOffset & ENFORCE_UTF8_MASK) == 0) {
            position = decodeStringList(tag, data, position, limit, list, registers);
          } else {
            position = decodeStringListRequireUtf8(tag, data, position, limit, list, registers);
          }
        }
        break;
      case 27: // MESSAGE_LIST:
        if (wireType == WireFormat.WIRETYPE_LENGTH_DELIMITED) {
          position =
              decodeMessageList(
                  getMessageFieldSchema(bufferPosition),
                  tag,
                  data,
                  position,
                  limit,
                  list,
                  registers);
        }
        break;
      case 28: // BYTES_LIST:
        if (wireType == WireFormat.WIRETYPE_LENGTH_DELIMITED) {
          position = decodeBytesList(tag, data, position, limit, list, registers);
        }
        break;
      case 30: // ENUM_LIST:
      case 44: // ENUM_LIST_PACKED:
        if (wireType == WireFormat.WIRETYPE_LENGTH_DELIMITED) {
          position = decodePackedVarint32List(data, position, list, registers);
        } else if (wireType == WireFormat.WIRETYPE_VARINT) {
          position = decodeVarint32List(tag, data, position, limit, list, registers);
        } else {
          break;
        }
        UnknownFieldSetLite unknownFields = ((GeneratedMessageLite) message).unknownFields;
        if (unknownFields == UnknownFieldSetLite.getDefaultInstance()) {
          // filterUnknownEnumList() expects the unknownFields parameter to be mutable or null.
          // Since we don't know yet whether there exist unknown enum values, we'd better pass
          // null to it instead of allocating a mutable instance. This is also needed to be
          // consistent with the behavior of generated parser/builder.
          unknownFields = null;
        }
        unknownFields =
            SchemaUtil.filterUnknownEnumList(
                number,
                (ProtobufList<Integer>) list,
                getEnumFieldVerifier(bufferPosition),
                unknownFields,
                (UnknownFieldSchema<UnknownFieldSetLite, UnknownFieldSetLite>) unknownFieldSchema);
        if (unknownFields != null) {
          ((GeneratedMessageLite) message).unknownFields = unknownFields;
        }
        break;
      case 33: // SINT32_LIST:
      case 47: // SINT32_LIST_PACKED:
        if (wireType == WireFormat.WIRETYPE_LENGTH_DELIMITED) {
          position = decodePackedSInt32List(data, position, list, registers);
        } else if (wireType == WireFormat.WIRETYPE_VARINT) {
          position = decodeSInt32List(tag, data, position, limit, list, registers);
        }
        break;
      case 34: // SINT64_LIST:
      case 48: // SINT64_LIST_PACKED:
        if (wireType == WireFormat.WIRETYPE_LENGTH_DELIMITED) {
          position = decodePackedSInt64List(data, position, list, registers);
        } else if (wireType == WireFormat.WIRETYPE_VARINT) {
          position = decodeSInt64List(tag, data, position, limit, list, registers);
        }
        break;
      case 49: // GROUP_LIST:
        if (wireType == WireFormat.WIRETYPE_START_GROUP) {
          position =
              decodeGroupList(
                  getMessageFieldSchema(bufferPosition),
                  tag,
                  data,
                  position,
                  limit,
                  list,
                  registers);
        }
        break;
      default:
        break;
    }
    return position;
  }

  private <K, V> int parseMapField(
      T message,
      byte[] data,
      int position,
      int limit,
      int bufferPosition,
      long fieldOffset,
      Registers registers)
      throws IOException {
    final sun.misc.Unsafe unsafe = UNSAFE;
    Object mapDefaultEntry = getMapFieldDefaultEntry(bufferPosition);
    Object mapField = unsafe.getObject(message, fieldOffset);
    if (mapFieldSchema.isImmutable(mapField)) {
      Object oldMapField = mapField;
      mapField = mapFieldSchema.newMapField(mapDefaultEntry);
      mapFieldSchema.mergeFrom(mapField, oldMapField);
      unsafe.putObject(message, fieldOffset, mapField);
    }
    return decodeMapEntry(
        data,
        position,
        limit,
        (Metadata<K, V>) mapFieldSchema.forMapMetadata(mapDefaultEntry),
        (Map<K, V>) mapFieldSchema.forMutableMapData(mapField),
        registers);
  }

  private int parseOneofField(
      T message,
      byte[] data,
      int position,
      int limit,
      int tag,
      int number,
      int wireType,
      int typeAndOffset,
      int fieldType,
      long fieldOffset,
      int bufferPosition,
      Registers registers)
      throws IOException {
    final sun.misc.Unsafe unsafe = UNSAFE;
    final long oneofCaseOffset = buffer[bufferPosition + 2] & OFFSET_MASK;
    switch (fieldType) {
      case 51: // ONEOF_DOUBLE:
        if (wireType == WireFormat.WIRETYPE_FIXED64) {
          unsafe.putObject(message, fieldOffset, decodeDouble(data, position));
          position += 8;
          unsafe.putInt(message, oneofCaseOffset, number);
        }
        break;
      case 52: // ONEOF_FLOAT:
        if (wireType == WireFormat.WIRETYPE_FIXED32) {
          unsafe.putObject(message, fieldOffset, decodeFloat(data, position));
          position += 4;
          unsafe.putInt(message, oneofCaseOffset, number);
        }
        break;
      case 53: // ONEOF_INT64:
      case 54: // ONEOF_UINT64:
        if (wireType == WireFormat.WIRETYPE_VARINT) {
          position = decodeVarint64(data, position, registers);
          unsafe.putObject(message, fieldOffset, registers.long1);
          unsafe.putInt(message, oneofCaseOffset, number);
        }
        break;
      case 55: // ONEOF_INT32:
      case 62: // ONEOF_UINT32:
        if (wireType == WireFormat.WIRETYPE_VARINT) {
          position = decodeVarint32(data, position, registers);
          unsafe.putObject(message, fieldOffset, registers.int1);
          unsafe.putInt(message, oneofCaseOffset, number);
        }
        break;
      case 56: // ONEOF_FIXED64:
      case 65: // ONEOF_SFIXED64:
        if (wireType == WireFormat.WIRETYPE_FIXED64) {
          unsafe.putObject(message, fieldOffset, decodeFixed64(data, position));
          position += 8;
          unsafe.putInt(message, oneofCaseOffset, number);
        }
        break;
      case 57: // ONEOF_FIXED32:
      case 64: // ONEOF_SFIXED32:
        if (wireType == WireFormat.WIRETYPE_FIXED32) {
          unsafe.putObject(message, fieldOffset, decodeFixed32(data, position));
          position += 4;
          unsafe.putInt(message, oneofCaseOffset, number);
        }
        break;
      case 58: // ONEOF_BOOL:
        if (wireType == WireFormat.WIRETYPE_VARINT) {
          position = decodeVarint64(data, position, registers);
          unsafe.putObject(message, fieldOffset, registers.long1 != 0);
          unsafe.putInt(message, oneofCaseOffset, number);
        }
        break;
      case 59: // ONEOF_STRING:
        if (wireType == WireFormat.WIRETYPE_LENGTH_DELIMITED) {
          position = decodeVarint32(data, position, registers);
          final int length = registers.int1;
          if (length == 0) {
            unsafe.putObject(message, fieldOffset, "");
          } else {
            if ((typeAndOffset & ENFORCE_UTF8_MASK) != 0
                && !Utf8.isValidUtf8(data, position, position + length)) {
              throw InvalidProtocolBufferException.invalidUtf8();
            }
            final String value = new String(data, position, length, Internal.UTF_8);
            unsafe.putObject(message, fieldOffset, value);
            position += length;
          }
          unsafe.putInt(message, oneofCaseOffset, number);
        }
        break;
      case 60: // ONEOF_MESSAGE:
        if (wireType == WireFormat.WIRETYPE_LENGTH_DELIMITED) {
          position =
              decodeMessageField(
                  getMessageFieldSchema(bufferPosition), data, position, limit, registers);
          final Object oldValue =
              unsafe.getInt(message, oneofCaseOffset) == number
                  ? unsafe.getObject(message, fieldOffset)
                  : null;
          if (oldValue == null) {
            unsafe.putObject(message, fieldOffset, registers.object1);
          } else {
            unsafe.putObject(
                message, fieldOffset, Internal.mergeMessage(oldValue, registers.object1));
          }
          unsafe.putInt(message, oneofCaseOffset, number);
        }
        break;
      case 61: // ONEOF_BYTES:
        if (wireType == WireFormat.WIRETYPE_LENGTH_DELIMITED) {
          position = decodeBytes(data, position, registers);
          unsafe.putObject(message, fieldOffset, registers.object1);
          unsafe.putInt(message, oneofCaseOffset, number);
        }
        break;
      case 63: // ONEOF_ENUM:
        if (wireType == WireFormat.WIRETYPE_VARINT) {
          position = decodeVarint32(data, position, registers);
          final int enumValue = registers.int1;
          EnumVerifier enumVerifier = getEnumFieldVerifier(bufferPosition);
          if (enumVerifier == null || enumVerifier.isInRange(enumValue)) {
            unsafe.putObject(message, fieldOffset, enumValue);
            unsafe.putInt(message, oneofCaseOffset, number);
          } else {
            // UnknownFieldSetLite requires varint to be represented as Long.
            getMutableUnknownFields(message).storeField(tag, (long) enumValue);
          }
        }
        break;
      case 66: // ONEOF_SINT32:
        if (wireType == WireFormat.WIRETYPE_VARINT) {
          position = decodeVarint32(data, position, registers);
          unsafe.putObject(message, fieldOffset, CodedInputStream.decodeZigZag32(registers.int1));
          unsafe.putInt(message, oneofCaseOffset, number);
        }
        break;
      case 67: // ONEOF_SINT64:
        if (wireType == WireFormat.WIRETYPE_VARINT) {
          position = decodeVarint64(data, position, registers);
          unsafe.putObject(message, fieldOffset, CodedInputStream.decodeZigZag64(registers.long1));
          unsafe.putInt(message, oneofCaseOffset, number);
        }
        break;
      case 68: // ONEOF_GROUP:
        if (wireType == WireFormat.WIRETYPE_START_GROUP) {
          final int endTag = (tag & ~0x7) | WireFormat.WIRETYPE_END_GROUP;
          position =
              decodeGroupField(
                  getMessageFieldSchema(bufferPosition), data, position, limit, endTag, registers);
          final Object oldValue =
              unsafe.getInt(message, oneofCaseOffset) == number
                  ? unsafe.getObject(message, fieldOffset)
                  : null;
          if (oldValue == null) {
            unsafe.putObject(message, fieldOffset, registers.object1);
          } else {
            unsafe.putObject(
                message, fieldOffset, Internal.mergeMessage(oldValue, registers.object1));
          }
          unsafe.putInt(message, oneofCaseOffset, number);
        }
        break;
      default:
        break;
    }
    return position;
  }

  private Schema getMessageFieldSchema(int pos) {
    final int index = pos / INTS_PER_FIELD * 2;
    Schema schema = (Schema) objects[index];
    if (schema != null) {
      return schema;
    }
    schema = Protobuf.getInstance().schemaFor((Class) objects[index + 1]);
    objects[index] = schema;
    return schema;
  }

  private Object getMapFieldDefaultEntry(int pos) {
    return objects[pos / INTS_PER_FIELD * 2];
  }

  private EnumVerifier getEnumFieldVerifier(int pos) {
    return (EnumVerifier) objects[pos / INTS_PER_FIELD * 2 + 1];
  }

  /**
   * Parses a proto2 message or group and returns the position after the message/group. If it's
   * parsing a message (endGroup == 0), returns limit if parsing is successful; It it's parsing a
   * group (endGroup != 0), parsing ends when a tag == endGroup is encountered and the position
   * after that tag is returned.
   */
  int parseProto2Message(
      T message, byte[] data, int position, int limit, int endGroup, Registers registers)
      throws IOException {
    final sun.misc.Unsafe unsafe = UNSAFE;
    int currentPresenceFieldOffset = -1;
    int currentPresenceField = 0;
    int tag = 0;
    int oldNumber = -1;
    int pos = 0;
    while (position < limit) {
      tag = data[position++];
      if (tag < 0) {
        position = decodeVarint32(tag, data, position, registers);
        tag = registers.int1;
      }
      final int number = tag >>> 3;
      final int wireType = tag & 0x7;
      if (number > oldNumber) {
        pos = positionForFieldNumber(number, pos / INTS_PER_FIELD);
      } else {
        pos = positionForFieldNumber(number);
      }
      oldNumber = number;
      if (pos == -1) {
        // need to reset
        pos = 0;
      } else {
        final int typeAndOffset = buffer[pos + 1];
        final int fieldType = type(typeAndOffset);
        final long fieldOffset = offset(typeAndOffset);
        if (fieldType <= 17) {
          // Proto2 optional fields have has-bits.
          final int presenceMaskAndOffset = buffer[pos + 2];
          final int presenceMask = 1 << (presenceMaskAndOffset >>> OFFSET_BITS);
          final int presenceFieldOffset = presenceMaskAndOffset & OFFSET_MASK;
          // We cache the 32-bit has-bits integer value and only write it back when parsing a field
          // using a different has-bits integer.
          if (presenceFieldOffset != currentPresenceFieldOffset) {
            if (currentPresenceFieldOffset != -1) {
              unsafe.putInt(message, (long) currentPresenceFieldOffset, currentPresenceField);
            }
            currentPresenceFieldOffset = presenceFieldOffset;
            currentPresenceField = unsafe.getInt(message, (long) presenceFieldOffset);
          }
          switch (fieldType) {
            case 0: // DOUBLE
              if (wireType == WireFormat.WIRETYPE_FIXED64) {
                UnsafeUtil.putDouble(message, fieldOffset, decodeDouble(data, position));
                position += 8;
                currentPresenceField |= presenceMask;
                continue;
              }
              break;
            case 1: // FLOAT
              if (wireType == WireFormat.WIRETYPE_FIXED32) {
                UnsafeUtil.putFloat(message, fieldOffset, decodeFloat(data, position));
                position += 4;
                currentPresenceField |= presenceMask;
                continue;
              }
              break;
            case 2: // INT64
            case 3: // UINT64
              if (wireType == WireFormat.WIRETYPE_VARINT) {
                position = decodeVarint64(data, position, registers);
                unsafe.putLong(message, fieldOffset, registers.long1);
                currentPresenceField |= presenceMask;
                continue;
              }
              break;
            case 4: // INT32
            case 11: // UINT32
              if (wireType == WireFormat.WIRETYPE_VARINT) {
                position = decodeVarint32(data, position, registers);
                unsafe.putInt(message, fieldOffset, registers.int1);
                currentPresenceField |= presenceMask;
                continue;
              }
              break;
            case 5: // FIXED64
            case 14: // SFIXED64
              if (wireType == WireFormat.WIRETYPE_FIXED64) {
                unsafe.putLong(message, fieldOffset, decodeFixed64(data, position));
                position += 8;
                currentPresenceField |= presenceMask;
                continue;
              }
              break;
            case 6: // FIXED32
            case 13: // SFIXED32
              if (wireType == WireFormat.WIRETYPE_FIXED32) {
                unsafe.putInt(message, fieldOffset, decodeFixed32(data, position));
                position += 4;
                currentPresenceField |= presenceMask;
                continue;
              }
              break;
            case 7: // BOOL
              if (wireType == WireFormat.WIRETYPE_VARINT) {
                position = decodeVarint64(data, position, registers);
                UnsafeUtil.putBoolean(message, fieldOffset, registers.long1 != 0);
                currentPresenceField |= presenceMask;
                continue;
              }
              break;
            case 8: // STRING
              if (wireType == WireFormat.WIRETYPE_LENGTH_DELIMITED) {
                if ((typeAndOffset & ENFORCE_UTF8_MASK) == 0) {
                  position = decodeString(data, position, registers);
                } else {
                  position = decodeStringRequireUtf8(data, position, registers);
                }
                unsafe.putObject(message, fieldOffset, registers.object1);
                currentPresenceField |= presenceMask;
                continue;
              }
              break;
            case 9: // MESSAGE
              if (wireType == WireFormat.WIRETYPE_LENGTH_DELIMITED) {
                position =
                    decodeMessageField(
                        getMessageFieldSchema(pos), data, position, limit, registers);
                if ((currentPresenceField & presenceMask) == 0) {
                  unsafe.putObject(message, fieldOffset, registers.object1);
                } else {
                  unsafe.putObject(
                      message,
                      fieldOffset,
                      Internal.mergeMessage(
                          unsafe.getObject(message, fieldOffset), registers.object1));
                }
                currentPresenceField |= presenceMask;
                continue;
              }
              break;
            case 10: // BYTES
              if (wireType == WireFormat.WIRETYPE_LENGTH_DELIMITED) {
                position = decodeBytes(data, position, registers);
                unsafe.putObject(message, fieldOffset, registers.object1);
                currentPresenceField |= presenceMask;
                continue;
              }
              break;
            case 12: // ENUM
              if (wireType == WireFormat.WIRETYPE_VARINT) {
                position = decodeVarint32(data, position, registers);
                final int enumValue = registers.int1;
                EnumVerifier enumVerifier = getEnumFieldVerifier(pos);
                if (enumVerifier == null || enumVerifier.isInRange(enumValue)) {
                  unsafe.putInt(message, fieldOffset, enumValue);
                  currentPresenceField |= presenceMask;
                } else {
                  // UnknownFieldSetLite requires varint to be represented as Long.
                  getMutableUnknownFields(message).storeField(tag, (long) enumValue);
                }
                continue;
              }
              break;
            case 15: // SINT32
              if (wireType == WireFormat.WIRETYPE_VARINT) {
                position = decodeVarint32(data, position, registers);
                unsafe.putInt(
                    message, fieldOffset, CodedInputStream.decodeZigZag32(registers.int1));
                currentPresenceField |= presenceMask;
                continue;
              }
              break;
            case 16: // SINT64
              if (wireType == WireFormat.WIRETYPE_VARINT) {
                position = decodeVarint64(data, position, registers);
                unsafe.putLong(
                    message, fieldOffset, CodedInputStream.decodeZigZag64(registers.long1));

                currentPresenceField |= presenceMask;
                continue;
              }
              break;
            case 17: // GROUP
              if (wireType == WireFormat.WIRETYPE_START_GROUP) {
                final int endTag = (number << 3) | WireFormat.WIRETYPE_END_GROUP;
                position =
                    decodeGroupField(
                        getMessageFieldSchema(pos), data, position, limit, endTag, registers);
                if ((currentPresenceField & presenceMask) == 0) {
                  unsafe.putObject(message, fieldOffset, registers.object1);
                } else {
                  unsafe.putObject(
                      message,
                      fieldOffset,
                      Internal.mergeMessage(
                          unsafe.getObject(message, fieldOffset), registers.object1));
                }

                currentPresenceField |= presenceMask;
                continue;
              }
              break;
            default:
              break;
          }
        } else if (fieldType == 27) {
          // Handle repeated message fields.
          if (wireType == WireFormat.WIRETYPE_LENGTH_DELIMITED) {
            ProtobufList<?> list = (ProtobufList<?>) unsafe.getObject(message, fieldOffset);
            if (!list.isModifiable()) {
              final int size = list.size();
              list =
                  list.mutableCopyWithCapacity(
                      size == 0 ? AbstractProtobufList.DEFAULT_CAPACITY : size * 2);
              unsafe.putObject(message, fieldOffset, list);
            }
            position =
                decodeMessageList(
                    getMessageFieldSchema(pos), tag, data, position, limit, list, registers);
            continue;
          }
        } else if (fieldType <= 49) {
          // Handle all other repeated fields.
          final int oldPosition = position;
          position =
              parseRepeatedField(
                  message,
                  data,
                  position,
                  limit,
                  tag,
                  number,
                  wireType,
                  pos,
                  typeAndOffset,
                  fieldType,
                  fieldOffset,
                  registers);
          if (position != oldPosition) {
            continue;
          }
        } else if (fieldType == 50) {
          if (wireType == WireFormat.WIRETYPE_LENGTH_DELIMITED) {
            final int oldPosition = position;
            position = parseMapField(message, data, position, limit, pos, fieldOffset, registers);
            if (position != oldPosition) {
              continue;
            }
          }
        } else {
          final int oldPosition = position;
          position =
              parseOneofField(
                  message,
                  data,
                  position,
                  limit,
                  tag,
                  number,
                  wireType,
                  typeAndOffset,
                  fieldType,
                  fieldOffset,
                  pos,
                  registers);
          if (position != oldPosition) {
            continue;
          }
        }
      }
      if (tag == endGroup && endGroup != 0) {
        break;
      }

      if (hasExtensions
          && registers.extensionRegistry != ExtensionRegistryLite.getEmptyRegistry()) {
        position = decodeExtensionOrUnknownField(
            tag, data, position, limit, message, defaultInstance,
            (UnknownFieldSchema<UnknownFieldSetLite, UnknownFieldSetLite>) unknownFieldSchema,
            registers);
      } else {
        position = decodeUnknownField(
            tag, data, position, limit, getMutableUnknownFields(message), registers);
      }
    }
    if (currentPresenceFieldOffset != -1) {
      unsafe.putInt(message, (long) currentPresenceFieldOffset, currentPresenceField);
    }
    UnknownFieldSetLite unknownFields = null;
    for (int i = checkInitializedCount; i < repeatedFieldOffsetStart; i++) {
      unknownFields =
          filterMapUnknownEnumValues(
              message,
              intArray[i],
              unknownFields,
              (UnknownFieldSchema<UnknownFieldSetLite, UnknownFieldSetLite>) unknownFieldSchema);
    }
    if (unknownFields != null) {
      ((UnknownFieldSchema<UnknownFieldSetLite, UnknownFieldSetLite>) unknownFieldSchema)
          .setBuilderToMessage(message, unknownFields);
    }
    if (endGroup == 0) {
      if (position != limit) {
        throw InvalidProtocolBufferException.parseFailure();
      }
    } else {
      if (position > limit || tag != endGroup) {
        throw InvalidProtocolBufferException.parseFailure();
      }
    }
    return position;
  }

  /** Parses a proto3 message and returns the limit if parsing is successful. */
  private int parseProto3Message(
      T message, byte[] data, int position, int limit, Registers registers) throws IOException {
    final sun.misc.Unsafe unsafe = UNSAFE;
    int tag = 0;
    int oldNumber = -1;
    int pos = 0;
    while (position < limit) {
      tag = data[position++];
      if (tag < 0) {
        position = decodeVarint32(tag, data, position, registers);
        tag = registers.int1;
      }
      final int number = tag >>> 3;
      final int wireType = tag & 0x7;
      if (number > oldNumber) {
        pos = positionForFieldNumber(number, pos / INTS_PER_FIELD);
      } else {
        pos = positionForFieldNumber(number);
      }
      oldNumber = number;
      if (pos == -1) {
        // need to reset
        pos = 0;
      } else {
        final int typeAndOffset = buffer[pos + 1];
        final int fieldType = type(typeAndOffset);
        final long fieldOffset = offset(typeAndOffset);
        if (fieldType <= 17) {
          switch (fieldType) {
            case 0: // DOUBLE:
              if (wireType == WireFormat.WIRETYPE_FIXED64) {
                UnsafeUtil.putDouble(message, fieldOffset, decodeDouble(data, position));
                position += 8;
                continue;
              }
              break;
            case 1: // FLOAT:
              if (wireType == WireFormat.WIRETYPE_FIXED32) {
                UnsafeUtil.putFloat(message, fieldOffset, decodeFloat(data, position));
                position += 4;
                continue;
              }
              break;
            case 2: // INT64:
            case 3: // UINT64:
              if (wireType == WireFormat.WIRETYPE_VARINT) {
                position = decodeVarint64(data, position, registers);
                unsafe.putLong(message, fieldOffset, registers.long1);
                continue;
              }
              break;
            case 4: // INT32:
            case 11: // UINT32:
              if (wireType == WireFormat.WIRETYPE_VARINT) {
                position = decodeVarint32(data, position, registers);
                unsafe.putInt(message, fieldOffset, registers.int1);
                continue;
              }
              break;
            case 5: // FIXED64:
            case 14: // SFIXED64:
              if (wireType == WireFormat.WIRETYPE_FIXED64) {
                unsafe.putLong(message, fieldOffset, decodeFixed64(data, position));
                position += 8;
                continue;
              }
              break;
            case 6: // FIXED32:
            case 13: // SFIXED32:
              if (wireType == WireFormat.WIRETYPE_FIXED32) {
                unsafe.putInt(message, fieldOffset, decodeFixed32(data, position));
                position += 4;
                continue;
              }
              break;
            case 7: // BOOL:
              if (wireType == WireFormat.WIRETYPE_VARINT) {
                position = decodeVarint64(data, position, registers);
                UnsafeUtil.putBoolean(message, fieldOffset, registers.long1 != 0);
                continue;
              }
              break;
            case 8: // STRING:
              if (wireType == WireFormat.WIRETYPE_LENGTH_DELIMITED) {
                if ((typeAndOffset & ENFORCE_UTF8_MASK) == 0) {
                  position = decodeString(data, position, registers);
                } else {
                  position = decodeStringRequireUtf8(data, position, registers);
                }
                unsafe.putObject(message, fieldOffset, registers.object1);
                continue;
              }
              break;
            case 9: // MESSAGE:
              if (wireType == WireFormat.WIRETYPE_LENGTH_DELIMITED) {
                position =
                    decodeMessageField(
                        getMessageFieldSchema(pos), data, position, limit, registers);
                final Object oldValue = unsafe.getObject(message, fieldOffset);
                if (oldValue == null) {
                  unsafe.putObject(message, fieldOffset, registers.object1);
                } else {
                  unsafe.putObject(
                      message, fieldOffset, Internal.mergeMessage(oldValue, registers.object1));
                }
                continue;
              }
              break;
            case 10: // BYTES:
              if (wireType == WireFormat.WIRETYPE_LENGTH_DELIMITED) {
                position = decodeBytes(data, position, registers);
                unsafe.putObject(message, fieldOffset, registers.object1);
                continue;
              }
              break;
            case 12: // ENUM:
              if (wireType == WireFormat.WIRETYPE_VARINT) {
                position = decodeVarint32(data, position, registers);
                unsafe.putInt(message, fieldOffset, registers.int1);
                continue;
              }
              break;
            case 15: // SINT32:
              if (wireType == WireFormat.WIRETYPE_VARINT) {
                position = decodeVarint32(data, position, registers);
                unsafe.putInt(
                    message, fieldOffset, CodedInputStream.decodeZigZag32(registers.int1));
                continue;
              }
              break;
            case 16: // SINT64:
              if (wireType == WireFormat.WIRETYPE_VARINT) {
                position = decodeVarint64(data, position, registers);
                unsafe.putLong(
                    message, fieldOffset, CodedInputStream.decodeZigZag64(registers.long1));
                continue;
              }
              break;
            default:
              break;
          }
        } else if (fieldType == 27) {
          // Handle repeated message field.
          if (wireType == WireFormat.WIRETYPE_LENGTH_DELIMITED) {
            ProtobufList<?> list = (ProtobufList<?>) unsafe.getObject(message, fieldOffset);
            if (!list.isModifiable()) {
              final int size = list.size();
              list =
                  list.mutableCopyWithCapacity(
                      size == 0 ? AbstractProtobufList.DEFAULT_CAPACITY : size * 2);
              unsafe.putObject(message, fieldOffset, list);
            }
            position =
                decodeMessageList(
                    getMessageFieldSchema(pos), tag, data, position, limit, list, registers);
            continue;
          }
        } else if (fieldType <= 49) {
          // Handle all other repeated fields.
          final int oldPosition = position;
          position =
              parseRepeatedField(
                  message,
                  data,
                  position,
                  limit,
                  tag,
                  number,
                  wireType,
                  pos,
                  typeAndOffset,
                  fieldType,
                  fieldOffset,
                  registers);
          if (position != oldPosition) {
            continue;
          }
        } else if (fieldType == 50) {
          if (wireType == WireFormat.WIRETYPE_LENGTH_DELIMITED) {
            final int oldPosition = position;
            position = parseMapField(message, data, position, limit, pos, fieldOffset, registers);
            if (position != oldPosition) {
              continue;
            }
          }
        } else {
          final int oldPosition = position;
          position =
              parseOneofField(
                  message,
                  data,
                  position,
                  limit,
                  tag,
                  number,
                  wireType,
                  typeAndOffset,
                  fieldType,
                  fieldOffset,
                  pos,
                  registers);
          if (position != oldPosition) {
            continue;
          }
        }
      }
      position = decodeUnknownField(
          tag, data, position, limit, getMutableUnknownFields(message), registers);
    }
    if (position != limit) {
      throw InvalidProtocolBufferException.parseFailure();
    }
    return position;
  }

  @Override
  public void mergeFrom(T message, byte[] data, int position, int limit, Registers registers)
      throws IOException {
    if (proto3) {
      parseProto3Message(message, data, position, limit, registers);
    } else {
      parseProto2Message(message, data, position, limit, 0, registers);
    }
  }

  @Override
  public void makeImmutable(T message) {
    // Make all repeated/map fields immutable.
    for (int i = checkInitializedCount; i < repeatedFieldOffsetStart; i++) {
      long offset = offset(typeAndOffsetAt(intArray[i]));
      Object mapField = UnsafeUtil.getObject(message, offset);
      if (mapField == null) {
        continue;
      }
      UnsafeUtil.putObject(message, offset, mapFieldSchema.toImmutable(mapField));
    }
    final int length = intArray.length;
    for (int i = repeatedFieldOffsetStart; i < length; i++) {
      listFieldSchema.makeImmutableListAt(message, intArray[i]);
    }
    unknownFieldSchema.makeImmutable(message);
    if (hasExtensions) {
      extensionSchema.makeImmutable(message);
    }
  }

  @SuppressWarnings("unchecked")
  private final <K, V> void mergeMap(
      Object message,
      int pos,
      Object mapDefaultEntry,
      ExtensionRegistryLite extensionRegistry,
      Reader reader)
      throws IOException {
    long offset = offset(typeAndOffsetAt(pos));
    Object mapField = UnsafeUtil.getObject(message, offset);
    // TODO(xiaofeng): Consider creating separate implementations for full and lite. In lite
    // runtime map field will never be null but here we still need to check null because the
    // code is shared by both full and lite. It might be better if full/lite uses different
    // schema implementations.
    if (mapField == null) {
      mapField = mapFieldSchema.newMapField(mapDefaultEntry);
      UnsafeUtil.putObject(message, offset, mapField);
    } else if (mapFieldSchema.isImmutable(mapField)) {
      Object oldMapField = mapField;
      mapField = mapFieldSchema.newMapField(mapDefaultEntry);
      mapFieldSchema.mergeFrom(mapField, oldMapField);
      UnsafeUtil.putObject(message, offset, mapField);
    }
    reader.readMap(
        (Map<K, V>) mapFieldSchema.forMutableMapData(mapField),
        (Metadata<K, V>) mapFieldSchema.forMapMetadata(mapDefaultEntry),
        extensionRegistry);
  }

  private final <UT, UB> UB filterMapUnknownEnumValues(
      Object message, int pos, UB unknownFields, UnknownFieldSchema<UT, UB> unknownFieldSchema) {
    int fieldNumber = numberAt(pos);
    long offset = offset(typeAndOffsetAt(pos));
    Object mapField = UnsafeUtil.getObject(message, offset);
    if (mapField == null) {
      return unknownFields;
    }
    EnumVerifier enumVerifier = getEnumFieldVerifier(pos);
    if (enumVerifier == null) {
      return unknownFields;
    }
    Map<?, ?> mapData = mapFieldSchema.forMutableMapData(mapField);
    // Filter unknown enum values.
    unknownFields =
        filterUnknownEnumMap(
            pos, fieldNumber, mapData, enumVerifier, unknownFields, unknownFieldSchema);
    return unknownFields;
  }

  @SuppressWarnings("unchecked")
  private final <K, V, UT, UB> UB filterUnknownEnumMap(
      int pos,
      int number,
      Map<K, V> mapData,
      EnumVerifier enumVerifier,
      UB unknownFields,
      UnknownFieldSchema<UT, UB> unknownFieldSchema) {
    Metadata<K, V> metadata =
        (Metadata<K, V>) mapFieldSchema.forMapMetadata(getMapFieldDefaultEntry(pos));
    for (Iterator<Map.Entry<K, V>> it = mapData.entrySet().iterator(); it.hasNext(); ) {
      Map.Entry<K, V> entry = it.next();
      if (!enumVerifier.isInRange((Integer) entry.getValue())) {
        if (unknownFields == null) {
          unknownFields = unknownFieldSchema.newBuilder();
        }
        int entrySize =
            MapEntryLite.computeSerializedSize(metadata, entry.getKey(), entry.getValue());
        CodedBuilder codedBuilder = ByteString.newCodedBuilder(entrySize);
        CodedOutputStream codedOutput = codedBuilder.getCodedOutput();
        try {
          MapEntryLite.writeTo(codedOutput, metadata, entry.getKey(), entry.getValue());
        } catch (IOException e) {
          // Writing to ByteString CodedOutputStream should not throw IOException.
          throw new RuntimeException(e);
        }
        unknownFieldSchema.addLengthDelimited(unknownFields, number, codedBuilder.build());
        it.remove();
      }
    }
    return unknownFields;
  }

  @Override
  public final boolean isInitialized(T message) {
    int currentPresenceFieldOffset = -1;
    int currentPresenceField = 0;
    for (int i = 0; i < checkInitializedCount; i++) {
      final int pos = intArray[i];
      final int number = numberAt(pos);

      final int typeAndOffset = typeAndOffsetAt(pos);

      int presenceMaskAndOffset = 0;
      int presenceMask = 0;
      if (!proto3) {
        presenceMaskAndOffset = buffer[pos + 2];
        final int presenceFieldOffset = presenceMaskAndOffset & OFFSET_MASK;
        presenceMask = 1 << (presenceMaskAndOffset >>> OFFSET_BITS);
        if (presenceFieldOffset != currentPresenceFieldOffset) {
          currentPresenceFieldOffset = presenceFieldOffset;
          currentPresenceField = UNSAFE.getInt(message, (long) presenceFieldOffset);
        }
      }

      if (isRequired(typeAndOffset)) {
        if (!isFieldPresent(message, pos, currentPresenceField, presenceMask)) {
          return false;
        }
        // If a required message field is set but has no required fields of it's own, we still
        // proceed and check the message is initialized. It should be fairly cheap to check these
        // messages but is worth documenting.
      }
      // Check nested message and groups.
      switch (type(typeAndOffset)) {
        case 9: // MESSAGE
        case 17: // GROUP
          if (isFieldPresent(message, pos, currentPresenceField, presenceMask)
              && !isInitialized(message, typeAndOffset, getMessageFieldSchema(pos))) {
            return false;
          }
          break;
        case 27: // MESSAGE_LIST
        case 49: // GROUP_LIST
          if (!isListInitialized(message, typeAndOffset, pos)) {
            return false;
          }
          break;
        case 60: // ONEOF_MESSAGE
        case 68: // ONEOF_GROUP
          if (isOneofPresent(message, number, pos)
              && !isInitialized(message, typeAndOffset, getMessageFieldSchema(pos))) {
            return false;
          }
          break;
        case 50: // MAP
          if (!isMapInitialized(message, typeAndOffset, pos)) {
            return false;
          }
          break;
        default:
          break;
      }
    }

    if (hasExtensions) {
      if (!extensionSchema.getExtensions(message).isInitialized()) {
        return false;
      }
    }

    return true;
  }

  private static boolean isInitialized(Object message, int typeAndOffset, Schema schema) {
    Object nested = UnsafeUtil.getObject(message, offset(typeAndOffset));
    return schema.isInitialized(nested);
  }

  private <N> boolean isListInitialized(Object message, int typeAndOffset, int pos) {
    @SuppressWarnings("unchecked")
    List<N> list = (List<N>) UnsafeUtil.getObject(message, offset(typeAndOffset));
    if (list.isEmpty()) {
      return true;
    }

    Schema schema = getMessageFieldSchema(pos);
    for (int i = 0; i < list.size(); i++) {
      N nested = list.get(i);
      if (!schema.isInitialized(nested)) {
        return false;
      }
    }
    return true;
  }

  private boolean isMapInitialized(T message, int typeAndOffset, int pos) {
    Map<?, ?> map = mapFieldSchema.forMapData(UnsafeUtil.getObject(message, offset(typeAndOffset)));
    if (map.isEmpty()) {
      return true;
    }
    Object mapDefaultEntry = getMapFieldDefaultEntry(pos);
    MapEntryLite.Metadata<?, ?> metadata = mapFieldSchema.forMapMetadata(mapDefaultEntry);
    if (metadata.valueType.getJavaType() != WireFormat.JavaType.MESSAGE) {
      return true;
    }
    // TODO(dweis): Use schema cache.
    Schema schema = null;
    for (Object nested : map.values()) {
      if (schema == null) {
        schema = Protobuf.getInstance().schemaFor(nested.getClass());
      }
      if (!schema.isInitialized(nested)) {
        return false;
      }
    }
    return true;
  }

  private void writeString(int fieldNumber, Object value, Writer writer) throws IOException {
    if (value instanceof String) {
      writer.writeString(fieldNumber, (String) value);
    } else {
      writer.writeBytes(fieldNumber, (ByteString) value);
    }
  }

  private void readString(Object message, int typeAndOffset, Reader reader) throws IOException {
    if (isEnforceUtf8(typeAndOffset)) {
      // Enforce valid UTF-8 on the read.
      UnsafeUtil.putObject(message, offset(typeAndOffset), reader.readStringRequireUtf8());
    } else if (lite) {
      // Lite messages use String fields to store strings. Read a string but do not
      // enforce UTF-8
      UnsafeUtil.putObject(message, offset(typeAndOffset), reader.readString());
    } else {
      // Full runtime messages use Objects to store either a String or ByteString. Read
      // the string as a ByteString and do not enforce UTF-8.
      UnsafeUtil.putObject(message, offset(typeAndOffset), reader.readBytes());
    }
  }

  private void readStringList(Object message, int typeAndOffset, Reader reader) throws IOException {
    if (isEnforceUtf8(typeAndOffset)) {
      reader.readStringListRequireUtf8(
          listFieldSchema.<String>mutableListAt(message, offset(typeAndOffset)));
    } else {
      reader.readStringList(listFieldSchema.<String>mutableListAt(message, offset(typeAndOffset)));
    }
  }

  private <E> void readMessageList(
      Object message,
      int typeAndOffset,
      Reader reader,
      Schema<E> schema,
      ExtensionRegistryLite extensionRegistry)
      throws IOException {
    long offset = offset(typeAndOffset);
    reader.readMessageList(
        listFieldSchema.<E>mutableListAt(message, offset), schema, extensionRegistry);
  }

  private <E> void readGroupList(
      Object message,
      long offset,
      Reader reader,
      Schema<E> schema,
      ExtensionRegistryLite extensionRegistry)
      throws IOException {
    reader.readGroupList(
        listFieldSchema.<E>mutableListAt(message, offset), schema, extensionRegistry);
  }

  private int numberAt(int pos) {
    return buffer[pos];
  }

  private int typeAndOffsetAt(int pos) {
    return buffer[pos + 1];
  }

  private int presenceMaskAndOffsetAt(int pos) {
    return buffer[pos + 2];
  }

  private static int type(int value) {
    return (value & FIELD_TYPE_MASK) >>> OFFSET_BITS;
  }

  private static boolean isRequired(int value) {
    return (value & REQUIRED_MASK) != 0;
  }

  private static boolean isEnforceUtf8(int value) {
    return (value & ENFORCE_UTF8_MASK) != 0;
  }

  private static long offset(int value) {
    return value & OFFSET_MASK;
  }

  private static <T> double doubleAt(T message, long offset) {
    return UnsafeUtil.getDouble(message, offset);
  }

  private static <T> float floatAt(T message, long offset) {
    return UnsafeUtil.getFloat(message, offset);
  }

  private static <T> int intAt(T message, long offset) {
    return UnsafeUtil.getInt(message, offset);
  }

  private static <T> long longAt(T message, long offset) {
    return UnsafeUtil.getLong(message, offset);
  }

  private static <T> boolean booleanAt(T message, long offset) {
    return UnsafeUtil.getBoolean(message, offset);
  }

  private static <T> double oneofDoubleAt(T message, long offset) {
    return ((Double) UnsafeUtil.getObject(message, offset)).doubleValue();
  }

  private static <T> float oneofFloatAt(T message, long offset) {
    return ((Float) UnsafeUtil.getObject(message, offset)).floatValue();
  }

  private static <T> int oneofIntAt(T message, long offset) {
    return ((Integer) UnsafeUtil.getObject(message, offset)).intValue();
  }

  private static <T> long oneofLongAt(T message, long offset) {
    return ((Long) UnsafeUtil.getObject(message, offset)).longValue();
  }

  private static <T> boolean oneofBooleanAt(T message, long offset) {
    return ((Boolean) UnsafeUtil.getObject(message, offset)).booleanValue();
  }

  /** Returns true the field is present in both messages, or neither. */
  private boolean arePresentForEquals(T message, T other, int pos) {
    return isFieldPresent(message, pos) == isFieldPresent(other, pos);
  }

  private boolean isFieldPresent(T message, int pos, int presenceField, int presenceMask) {
    if (proto3) {
      return isFieldPresent(message, pos);
    } else {
      return (presenceField & presenceMask) != 0;
    }
  }

  private boolean isFieldPresent(T message, int pos) {
    if (proto3) {
      final int typeAndOffset = typeAndOffsetAt(pos);
      final long offset = offset(typeAndOffset);
      switch (type(typeAndOffset)) {
        case 0: // DOUBLE:
          return UnsafeUtil.getDouble(message, offset) != 0D;
        case 1: // FLOAT:
          return UnsafeUtil.getFloat(message, offset) != 0F;
        case 2: // INT64:
          return UnsafeUtil.getLong(message, offset) != 0L;
        case 3: // UINT64:
          return UnsafeUtil.getLong(message, offset) != 0L;
        case 4: // INT32:
          return UnsafeUtil.getInt(message, offset) != 0;
        case 5: // FIXED64:
          return UnsafeUtil.getLong(message, offset) != 0L;
        case 6: // FIXED32:
          return UnsafeUtil.getInt(message, offset) != 0;
        case 7: // BOOL:
          return UnsafeUtil.getBoolean(message, offset);
        case 8: // STRING:
          Object value = UnsafeUtil.getObject(message, offset);
          if (value instanceof String) {
            return !((String) value).isEmpty();
          } else if (value instanceof ByteString) {
            return !ByteString.EMPTY.equals(value);
          } else {
            throw new IllegalArgumentException();
          }
        case 9: // MESSAGE:
          return UnsafeUtil.getObject(message, offset) != null;
        case 10: // BYTES:
          return !ByteString.EMPTY.equals(UnsafeUtil.getObject(message, offset));
        case 11: // UINT32:
          return UnsafeUtil.getInt(message, offset) != 0;
        case 12: // ENUM:
          return UnsafeUtil.getInt(message, offset) != 0;
        case 13: // SFIXED32:
          return UnsafeUtil.getInt(message, offset) != 0;
        case 14: // SFIXED64:
          return UnsafeUtil.getLong(message, offset) != 0L;
        case 15: // SINT32:
          return UnsafeUtil.getInt(message, offset) != 0;
        case 16: // SINT64:
          return UnsafeUtil.getLong(message, offset) != 0L;
        case 17: // GROUP:
          return UnsafeUtil.getObject(message, offset) != null;
        default:
          throw new IllegalArgumentException();
      }
    } else {
      int presenceMaskAndOffset = presenceMaskAndOffsetAt(pos);
      final int presenceMask = 1 << (presenceMaskAndOffset >>> OFFSET_BITS);
      return (UnsafeUtil.getInt(message, presenceMaskAndOffset & OFFSET_MASK) & presenceMask) != 0;
    }
  }

  private void setFieldPresent(T message, int pos) {
    if (proto3) {
      // Proto3 doesn't have presence fields
      return;
    }
    int presenceMaskAndOffset = presenceMaskAndOffsetAt(pos);
    final int presenceMask = 1 << (presenceMaskAndOffset >>> OFFSET_BITS);
    final long presenceFieldOffset = presenceMaskAndOffset & OFFSET_MASK;
    UnsafeUtil.putInt(
        message,
        presenceFieldOffset,
        UnsafeUtil.getInt(message, presenceFieldOffset) | presenceMask);
  }

  private boolean isOneofPresent(T message, int fieldNumber, int pos) {
    int presenceMaskAndOffset = presenceMaskAndOffsetAt(pos);
    return UnsafeUtil.getInt(message, presenceMaskAndOffset & OFFSET_MASK) == fieldNumber;
  }

  private boolean isOneofCaseEqual(T message, T other, int pos) {
    int presenceMaskAndOffset = presenceMaskAndOffsetAt(pos);
    return UnsafeUtil.getInt(message, presenceMaskAndOffset & OFFSET_MASK)
        == UnsafeUtil.getInt(other, presenceMaskAndOffset & OFFSET_MASK);
  }

  private void setOneofPresent(T message, int fieldNumber, int pos) {
    int presenceMaskAndOffset = presenceMaskAndOffsetAt(pos);
    UnsafeUtil.putInt(message, presenceMaskAndOffset & OFFSET_MASK, fieldNumber);
  }

  private int positionForFieldNumber(final int number) {
    if (number >= minFieldNumber && number <= maxFieldNumber) {
      return slowPositionForFieldNumber(number, 0);
    }
    return -1;
  }

  private int positionForFieldNumber(final int number, final int min) {
    if (number >= minFieldNumber && number <= maxFieldNumber) {
      return slowPositionForFieldNumber(number, min);
    }
    return -1;
  }

  private int slowPositionForFieldNumber(final int number, int min) {
    int max = buffer.length / INTS_PER_FIELD - 1;
    while (min <= max) {
      // Find the midpoint address.
      final int mid = (max + min) >>> 1;
      final int pos = mid * INTS_PER_FIELD;
      final int midFieldNumber = numberAt(pos);
      if (number == midFieldNumber) {
        // Found the field.
        return pos;
      }
      if (number < midFieldNumber) {
        // Search the lower half.
        max = mid - 1;
      } else {
        // Search the upper half.
        min = mid + 1;
      }
    }
    return -1;
  }

  int getSchemaSize() {
    return buffer.length * 3;
  }
}
