// Protocol Buffers - Google's data interchange format
// Copyright 2013 Google Inc.  All rights reserved.
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

package com.google.protobuf.nano;

import java.io.IOException;
import java.util.HashMap;
import java.util.Map;
import java.util.Map.Entry;

/**
 * Utility class for maps support.
 */
public final class MapUtil {
  public static interface MapFactory {
    <K, V> Map<K, V> forMap(Map<K, V> oldMap);
  }
  public static void setMapFactory(MapFactory newMapFactory) {
    mapFactory = newMapFactory;
  }

  private static class DefaultMapFactory implements MapFactory {
    public <K, V> Map<K, V> forMap(Map<K, V> oldMap) {
      if (oldMap == null) {
        return new HashMap<K, V>();
      }
      return oldMap;
    }
  }
  private static volatile MapFactory mapFactory = new DefaultMapFactory();

  /**
   * Internal utilities to implement maps for generated messages.
   * Do NOT use it explicitly.
   */
  public static class Internal {
    private static final byte[] emptyBytes = new byte[0];
    private static Object primitiveDefaultValue(int type) {
      switch (type) {
        case InternalNano.TYPE_BOOL:
          return Boolean.FALSE;
        case InternalNano.TYPE_BYTES:
          return emptyBytes;
        case InternalNano.TYPE_STRING:
          return "";
        case InternalNano.TYPE_FLOAT:
          return Float.valueOf(0);
        case InternalNano.TYPE_DOUBLE:
          return Double.valueOf(0);
        case InternalNano.TYPE_ENUM:
        case InternalNano.TYPE_FIXED32:
        case InternalNano.TYPE_INT32:
        case InternalNano.TYPE_UINT32:
        case InternalNano.TYPE_SINT32:
        case InternalNano.TYPE_SFIXED32:
          return Integer.valueOf(0);
        case InternalNano.TYPE_INT64:
        case InternalNano.TYPE_UINT64:
        case InternalNano.TYPE_SINT64:
        case InternalNano.TYPE_FIXED64:
        case InternalNano.TYPE_SFIXED64:
          return Long.valueOf(0L);
        case InternalNano.TYPE_MESSAGE:
        case InternalNano.TYPE_GROUP:
        default:
          throw new IllegalArgumentException(
              "Type: " + type + " is not a primitive type.");
      }
    }

    /**
     * Merges the map entry into the map field. Note this is only supposed to
     * be called by generated messages.
     *
     * @param map the map field; may be null, in which case a map will be
     *        instantiated using the {@link MapUtil.MapFactory}
     * @param input the input byte buffer
     * @param keyType key type, as defined in InternalNano.TYPE_*
     * @param valueType value type, as defined in InternalNano.TYPE_*
     * @param valueClazz class of the value field if the valueType is
     *        TYPE_MESSAGE; otherwise the parameter is ignored and can be null.
     * @param keyTag wire tag for the key
     * @param valueTag wire tag for the value
     * @return the map field
     * @throws IOException
     */
    @SuppressWarnings("unchecked")
    public static final <K, V> Map<K, V> mergeEntry(
        CodedInputByteBufferNano input,
        Map<K, V> map,
        int keyType,
        int valueType,
        Class<V> valueClazz,
        int keyTag,
        int valueTag) throws IOException {
      map = mapFactory.forMap(map);
      final int length = input.readRawVarint32();
      final int oldLimit = input.pushLimit(length);
      byte[] payload = null;
      K key = null;
      V value = null;
      while (true) {
        int tag = input.readTag();
        if (tag == 0) {
          break;
        }
        if (tag == keyTag) {
          key = (K) input.readData(keyType);
        } else if (tag == valueTag) {
          if (valueType == InternalNano.TYPE_MESSAGE) {
            payload = input.readBytes();
          } else {
            value = (V) input.readData(valueType);
          }
        } else {
          if (!input.skipField(tag)) {
            break;
          }
        }
      }
      input.checkLastTagWas(0);
      input.popLimit(oldLimit);

      if (key == null) {
        key = (K) primitiveDefaultValue(keyType);
      }

      // Special case: merge the value when the value is a message.
      if (valueType == InternalNano.TYPE_MESSAGE) {
        MessageNano oldMessageValue = (MessageNano) map.get(key);
        if (oldMessageValue != null) {
          if (payload != null) {
            MessageNano.mergeFrom(oldMessageValue, payload);
          }
          return map;
        }
        // Otherwise, create a new value message.
        try {
          value = valueClazz.newInstance();
        } catch (InstantiationException e) {
          throw new IOException(
              "Unable to create value message " + valueClazz.getName()
              + " in maps.");
        } catch (IllegalAccessException e) {
          throw new IOException(
              "Unable to create value message " + valueClazz.getName()
              + " in maps.");
        }
        if (payload != null) {
          MessageNano.mergeFrom((MessageNano) value, payload);
        }
      }

      if (value == null) {
        value = (V) primitiveDefaultValue(valueType);
      }

      map.put(key, value);
      return map;
    }

    public static <K, V> void serializeMapField(
        CodedOutputByteBufferNano output,
        Map<K, V> map, int number, int keyType, int valueType)
            throws IOException {
      for (Entry<K, V> entry: map.entrySet()) {
        K key = entry.getKey();
        V value = entry.getValue();
        if (key == null || value == null) {
          throw new IllegalStateException(
              "keys and values in maps cannot be null");
        }
        int entrySize =
            CodedOutputByteBufferNano.computeFieldSize(1, keyType, key) +
            CodedOutputByteBufferNano.computeFieldSize(2, valueType, value);
        output.writeTag(number, WireFormatNano.WIRETYPE_LENGTH_DELIMITED);
        output.writeRawVarint32(entrySize);
        output.writeField(1, keyType, key);
        output.writeField(2, valueType, value);
      }
    }

    public static <K, V> int computeMapFieldSize(
        Map<K, V> map, int number, int keyType, int valueType) {
      int size = 0;
      int tagSize = CodedOutputByteBufferNano.computeTagSize(number);
      for (Entry<K, V> entry: map.entrySet()) {
        K key = entry.getKey();
        V value = entry.getValue();
        if (key == null || value == null) {
          throw new IllegalStateException(
              "keys and values in maps cannot be null");
        }
        int entrySize =
            CodedOutputByteBufferNano.computeFieldSize(1, keyType, key) +
            CodedOutputByteBufferNano.computeFieldSize(2, valueType, value);
        size += tagSize + entrySize
            + CodedOutputByteBufferNano.computeRawVarint32Size(entrySize);
      }
      return size;
    }

    private Internal() {}
  }

  private MapUtil() {}
}
