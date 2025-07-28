package com.google.protobuf;

import java.io.IOException;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;

/** */
final class FieldSetMutable {
  // Avoid iterator allocation.
  @SuppressWarnings({"unchecked", "ForeachList", "ForeachListWithUserVar"})
  private static <T extends FieldSet.FieldDescriptorLite<T>> Object convertToImmutable(
      T descriptor, Object value) {
    if (descriptor.getLiteJavaType() == WireFormat.JavaType.MESSAGE) {
      if (descriptor.isRepeated()) {
        List<Object> mutableMessages = (List<Object>) value;
        int mutableMessagesSize = mutableMessages.size();
        List<Object> immutableMessages = new ArrayList<>(mutableMessagesSize);
        for (int i = 0; i < mutableMessagesSize; i++) {
          Object mutableMessage = mutableMessages.get(i);
          immutableMessages.add(((MutableMessageLite) mutableMessage).immutableCopy());
        }
        return immutableMessages;
      } else {
        if (value instanceof LazyField) {
          return ((MutableMessageLite) ((LazyField) value).getValue()).immutableCopy();
        }
        return ((MutableMessageLite) value).immutableCopy();
      }
    } else if (descriptor.getLiteJavaType() == WireFormat.JavaType.BYTE_STRING) {
      if (descriptor.isRepeated()) {
        List<Object> mutableFields = (List<Object>) value;
        int mutableFieldsSize = mutableFields.size();
        List<Object> immutableFields = new ArrayList<>(mutableFieldsSize);
        for (int i = 0; i < mutableFieldsSize; i++) {
          Object mutableField = mutableFields.get(i);
          immutableFields.add(ByteString.copyFrom((byte[]) mutableField));
        }
        return immutableFields;
      } else {
        return ByteString.copyFrom((byte[]) value);
      }
    } else {
      return value;
    }
  }

  // Avoid iterator allocation.
  @SuppressWarnings({"unchecked", "ForeachList", "ForeachListWithUserVar"})
  private static <T extends FieldSet.FieldDescriptorLite<T>> Object convertToMutable(
      T descriptor, Object value) {
    if (descriptor.getLiteJavaType() == WireFormat.JavaType.MESSAGE) {
      if (descriptor.isRepeated()) {
        List<Object> immutableMessages = (List<Object>) value;
        int immutableMessagesSize = immutableMessages.size();
        List<Object> mutableMessages = new ArrayList<>(immutableMessagesSize);
        for (int i = 0; i < immutableMessagesSize; i++) {
          Object immutableMessage = immutableMessages.get(i);
          mutableMessages.add(((MessageLite) immutableMessage).mutableCopy());
        }
        return mutableMessages;
      } else {
        if (value instanceof LazyField) {
          return ((LazyField) value).getValue().mutableCopy();
        }
        return ((MessageLite) value).mutableCopy();
      }
    } else if (descriptor.getLiteJavaType() == WireFormat.JavaType.BYTE_STRING) {
      if (descriptor.isRepeated()) {
        List<Object> immutableFields = (List<Object>) value;
        int immutableFieldsSize = immutableFields.size();
        List<Object> mutableFields = new ArrayList<>(immutableFieldsSize);
        for (int i = 0; i < immutableFieldsSize; i++) {
          Object immutableField = immutableFields.get(i);
          mutableFields.add(((ByteString) immutableField).toByteArray());
        }
        return mutableFields;
      } else {
        return ((ByteString) value).toByteArray();
      }
    } else {
      return value;
    }
  }

  /**
   * Creates a new FieldSet with all contained mutable messages converted to immutable messages.
   *
   * @return the newly cloned FieldSet. Note that the returned FieldSet is mutable.
   */
  public static <T extends FieldSet.FieldDescriptorLite<T>>
      FieldSet<T> cloneWithAllFieldsToImmutable(FieldSet<T> fieldSet) {
    FieldSet<T> clone = FieldSet.newFieldSet();
    int n = fieldSet.fields.getNumArrayEntries(); // Optimisation: hoist out of hot loop.
    for (int i = 0; i < n; i++) {
      Map.Entry<T, Object> entry = fieldSet.fields.getArrayEntryAt(i);
      T descriptor = entry.getKey();
      clone.setField(descriptor, convertToImmutable(descriptor, entry.getValue()));
    }
    for (Map.Entry<T, Object> entry : fieldSet.fields.getOverflowEntries()) {
      T descriptor = entry.getKey();
      clone.setField(descriptor, convertToImmutable(descriptor, entry.getValue()));
    }
    // Lazy fields are parsed out in the conversion.
    clone.hasLazyField = false;
    return clone;
  }

  /**
   * Creates a new FieldSet with all contained immutable messages converted to mutable messages.
   *
   * @return the newly cloned FieldSet. Note that the returned FieldSet is mutable.
   */
  public static <T extends FieldSet.FieldDescriptorLite<T>> FieldSet<T> cloneWithAllFieldsToMutable(
      FieldSet<T> fieldSet) {
    FieldSet<T> clone = FieldSet.newFieldSet();
    int n = fieldSet.fields.getNumArrayEntries(); // Optimisation: hoist out of hot loop.
    for (int i = 0; i < n; i++) {
      Map.Entry<T, Object> entry = fieldSet.fields.getArrayEntryAt(i);
      T descriptor = entry.getKey();
      clone.setField(descriptor, convertToMutable(descriptor, entry.getValue()));
    }
    for (Map.Entry<T, Object> entry : fieldSet.fields.getOverflowEntries()) {
      T descriptor = entry.getKey();
      clone.setField(descriptor, convertToMutable(descriptor, entry.getValue()));
    }
    // Lazy fields are parsed out in the conversion.
    clone.hasLazyField = false;
    return clone;
  }

  /**
   * Like {@link FieldSet#readPrimitiveField(CodedInputStream,WireFormat.FieldType)} but used for
   * mutable messages.
   */
  public static Object readPrimitiveFieldForMutable(
      CodedInputStream input, final WireFormat.FieldType type, boolean checkUtf8)
      throws IOException {
    if (type == WireFormat.FieldType.BYTES) {
      return input.readByteArray();
    } else {
      return FieldSet.readPrimitiveField(input, type, checkUtf8);
    }
  }
}
