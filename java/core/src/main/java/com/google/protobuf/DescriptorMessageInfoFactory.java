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
import static com.google.protobuf.FieldInfo.forFieldWithEnumVerifier;
import static com.google.protobuf.FieldInfo.forMapField;
import static com.google.protobuf.FieldInfo.forOneofMemberField;
import static com.google.protobuf.FieldInfo.forPackedField;
import static com.google.protobuf.FieldInfo.forPackedFieldWithEnumVerifier;
import static com.google.protobuf.FieldInfo.forProto2OptionalField;
import static com.google.protobuf.FieldInfo.forProto2RequiredField;
import static com.google.protobuf.FieldInfo.forRepeatedMessageField;

import com.google.protobuf.Descriptors.Descriptor;
import com.google.protobuf.Descriptors.FieldDescriptor;
import com.google.protobuf.Descriptors.FieldDescriptor.Type;
import com.google.protobuf.Descriptors.OneofDescriptor;
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.Stack;
import java.util.concurrent.ConcurrentHashMap;

/** A factory for message info based on protobuf descriptors for a {@link GeneratedMessageV3}. */
@ExperimentalApi
final class DescriptorMessageInfoFactory implements MessageInfoFactory {
  private static final String GET_DEFAULT_INSTANCE_METHOD_NAME = "getDefaultInstance";
  private static final DescriptorMessageInfoFactory instance = new DescriptorMessageInfoFactory();
  private static final Set<String> specialFieldNames =
      new HashSet<>(Arrays.asList("cached_size", "serialized_size", "class"));

  // Disallow construction - it's a singleton.
  private DescriptorMessageInfoFactory() {}

  public static DescriptorMessageInfoFactory getInstance() {
    return instance;
  }

  @Override
  public boolean isSupported(Class<?> messageType) {
    return GeneratedMessageV3.class.isAssignableFrom(messageType);
  }

  @Override
  public MessageInfo messageInfoFor(Class<?> messageType) {
    if (!GeneratedMessageV3.class.isAssignableFrom(messageType)) {
      throw new IllegalArgumentException("Unsupported message type: " + messageType.getName());
    }

    return convert(messageType, descriptorForType(messageType));
  }

  private static Message getDefaultInstance(Class<?> messageType) {
    try {
      Method method = messageType.getDeclaredMethod(GET_DEFAULT_INSTANCE_METHOD_NAME);
      return (Message) method.invoke(null);
    } catch (Exception e) {
      throw new IllegalArgumentException(
          "Unable to get default instance for message class " + messageType.getName(), e);
    }
  }

  private static Descriptor descriptorForType(Class<?> messageType) {
    return getDefaultInstance(messageType).getDescriptorForType();
  }

  private static MessageInfo convert(Class<?> messageType, Descriptor messageDescriptor) {
    switch (messageDescriptor.getFile().getSyntax()) {
      case PROTO2:
        return convertProto2(messageType, messageDescriptor);
      case PROTO3:
        return convertProto3(messageType, messageDescriptor);
      default:
        throw new IllegalArgumentException(
            "Unsupported syntax: " + messageDescriptor.getFile().getSyntax());
    }
  }

  /**
   * A helper class to determine whether a message type needs to implement {@code isInitialized()}.
   *
   * <p>If a message type doesn't have any required fields or extensions (directly and
   * transitively), it doesn't need to implement isInitialized() and can always return true there.
   * It's a bit tricky to determine whether a type has transitive required fields because protobuf
   * allows cycle references within the same .proto file (e.g., message Foo has a Bar field, and
   * message Bar has a Foo field). For that we use Tarjan's strongly connected components algorithm
   * to classify messages into strongly connected groups. Messages in the same group are
   * transitively including each other, so they should either all have transitive required fields
   * (or extensions), or none have.
   *
   * <p>This class is thread-safe.
   */
  static class IsInitializedCheckAnalyzer {

    private final Map<Descriptor, Boolean> resultCache =
        new ConcurrentHashMap<Descriptor, Boolean>();

    // The following data members are part of Tarjan's SCC algorithm. See:
    //   https://en.wikipedia.org/wiki/Tarjan%27s_strongly_connected_components_algorithm
    private int index = 0;
    private final Stack<Node> stack = new Stack<Node>();
    private final Map<Descriptor, Node> nodeCache = new HashMap<Descriptor, Node>();

    public boolean needsIsInitializedCheck(Descriptor descriptor) {
      Boolean cachedValue = resultCache.get(descriptor);
      if (cachedValue != null) {
        return cachedValue;
      }
      synchronized (this) {
        // Double-check the cache because some other thread may have updated it while we
        // were acquiring the lock.
        cachedValue = resultCache.get(descriptor);
        if (cachedValue != null) {
          return cachedValue;
        }
        return dfs(descriptor).component.needsIsInitializedCheck;
      }
    }

    private static class Node {
      final Descriptor descriptor;
      final int index;
      int lowLink;
      StronglyConnectedComponent component; // null if the node is still on stack.

      Node(Descriptor descriptor, int index) {
        this.descriptor = descriptor;
        this.index = index;
        this.lowLink = index;
        this.component = null;
      }
    }

    private static class StronglyConnectedComponent {
      final List<Descriptor> messages = new ArrayList<Descriptor>();
      boolean needsIsInitializedCheck = false;
    }

    private Node dfs(Descriptor descriptor) {
      Node result = new Node(descriptor, index++);
      stack.push(result);
      nodeCache.put(descriptor, result);

      // Recurse the fields / nodes in graph
      for (FieldDescriptor field : descriptor.getFields()) {
        if (field.getJavaType() == FieldDescriptor.JavaType.MESSAGE) {
          Node child = nodeCache.get(field.getMessageType());
          if (child == null) {
            // Unexplored node
            child = dfs(field.getMessageType());
            result.lowLink = Math.min(result.lowLink, child.lowLink);
          } else {
            if (child.component == null) {
              // Still in the stack so we found a back edge.
              result.lowLink = Math.min(result.lowLink, child.lowLink);
            }
          }
        }
      }

      if (result.index == result.lowLink) {
        // This is the root of a strongly connected component.
        StronglyConnectedComponent component = new StronglyConnectedComponent();
        while (true) {
          Node node = stack.pop();
          node.component = component;
          component.messages.add(node.descriptor);
          if (node == result) {
            break;
          }
        }

        analyze(component);
      }

      return result;
    }

    // Determine whether messages in this SCC needs isInitialized check.
    private void analyze(StronglyConnectedComponent component) {
      boolean needsIsInitializedCheck = false;
      loop:
      for (Descriptor descriptor : component.messages) {
        if (descriptor.isExtendable()) {
          needsIsInitializedCheck = true;
          break;
        }

        for (FieldDescriptor field : descriptor.getFields()) {
          if (field.isRequired()) {
            needsIsInitializedCheck = true;
            break loop;
          }

          if (field.getJavaType() == FieldDescriptor.JavaType.MESSAGE) {
            // Since we are analyzing the graph bottom-up, all referenced fields should either be
            // in this same component or in a different already-analyzed component.
            Node node = nodeCache.get(field.getMessageType());
            if (node.component != component) {
              if (node.component.needsIsInitializedCheck) {
                needsIsInitializedCheck = true;
                break loop;
              }
            }
          }
        }
      }

      component.needsIsInitializedCheck = needsIsInitializedCheck;

      for (Descriptor descriptor : component.messages) {
        resultCache.put(descriptor, component.needsIsInitializedCheck);
      }
    }
  }

  private static IsInitializedCheckAnalyzer isInitializedCheckAnalyzer =
      new IsInitializedCheckAnalyzer();

  private static boolean needsIsInitializedCheck(Descriptor descriptor) {
    return isInitializedCheckAnalyzer.needsIsInitializedCheck(descriptor);
  }

  private static StructuralMessageInfo convertProto2(
      Class<?> messageType, Descriptor messageDescriptor) {
    List<FieldDescriptor> fieldDescriptors = messageDescriptor.getFields();
    StructuralMessageInfo.Builder builder =
        StructuralMessageInfo.newBuilder(fieldDescriptors.size());
    builder.withDefaultInstance(getDefaultInstance(messageType));
    builder.withSyntax(ProtoSyntax.PROTO2);
    builder.withMessageSetWireFormat(messageDescriptor.getOptions().getMessageSetWireFormat());

    OneofState oneofState = new OneofState();
    int bitFieldIndex = 0;
    int presenceMask = 1;
    Field bitField = null;

    // Fields in the descriptor are ordered by the index position in which they appear in the
    // proto file. This is the same order used to determine the presence mask used in the
    // bitFields. So to determine the appropriate presence mask to be used for a field, we simply
    // need to shift the presence mask whenever a presence-checked field is encountered.
    for (int i = 0; i < fieldDescriptors.size(); ++i) {
      final FieldDescriptor fd = fieldDescriptors.get(i);
      boolean enforceUtf8 = fd.getFile().getOptions().getJavaStringCheckUtf8();
      Internal.EnumVerifier enumVerifier = null;
      if (fd.getJavaType() == Descriptors.FieldDescriptor.JavaType.ENUM) {
        enumVerifier =
            new Internal.EnumVerifier() {
              @Override
              public boolean isInRange(int number) {
                return fd.getEnumType().findValueByNumber(number) != null;
              }
            };
      }
      if (fd.getContainingOneof() != null) {
        // Build a oneof member field.
        builder.withField(buildOneofMember(messageType, fd, oneofState, enforceUtf8, enumVerifier));
      } else {
        Field field = field(messageType, fd);
        int number = fd.getNumber();
        FieldType type = getFieldType(fd);

        if (fd.isMapField()) {
          // Map field points to an auto-generated message entry type with the definition:
          //   message MapEntry {
          //     K key = 1;
          //     V value = 2;
          //   }
          final FieldDescriptor valueField = fd.getMessageType().findFieldByNumber(2);
          if (valueField.getJavaType() == Descriptors.FieldDescriptor.JavaType.ENUM) {
            enumVerifier =
                new Internal.EnumVerifier() {
                  @Override
                  public boolean isInRange(int number) {
                    return valueField.getEnumType().findValueByNumber(number) != null;
                  }
                };
          }
          builder.withField(
              forMapField(
                  field,
                  number,
                  SchemaUtil.getMapDefaultEntry(messageType, fd.getName()),
                  enumVerifier));
          continue;
        }

        if (fd.isRepeated()) {
          // Repeated fields are not presence-checked.
          if (enumVerifier != null) {
            if (fd.isPacked()) {
              builder.withField(
                  forPackedFieldWithEnumVerifier(
                      field, number, type, enumVerifier, cachedSizeField(messageType, fd)));
            } else {
              builder.withField(forFieldWithEnumVerifier(field, number, type, enumVerifier));
            }
          } else if (fd.getJavaType() == FieldDescriptor.JavaType.MESSAGE) {
            builder.withField(
                forRepeatedMessageField(
                    field, number, type, getTypeForRepeatedMessageField(messageType, fd)));
          } else {
            if (fd.isPacked()) {
              builder.withField(
                  forPackedField(field, number, type, cachedSizeField(messageType, fd)));
            } else {
              builder.withField(forField(field, number, type, enforceUtf8));
            }
          }
          continue;
        }

        if (bitField == null) {
          // Lazy-create the next bitfield since we know it must exist.
          bitField = bitField(messageType, bitFieldIndex);
        }

        // It's a presence-checked field.
        if (fd.isRequired()) {
          builder.withField(
              forProto2RequiredField(
                  field, number, type, bitField, presenceMask, enforceUtf8, enumVerifier));
        } else {
          builder.withField(
              forProto2OptionalField(
                  field, number, type, bitField, presenceMask, enforceUtf8, enumVerifier));
        }
      }

      // Update the presence mask for the next iteration. If the shift clears out the mask, we will
      // go to the next bitField.
      presenceMask <<= 1;
      if (presenceMask == 0) {
        bitField = null;
        presenceMask = 1;
        bitFieldIndex++;
      }
    }

    List<Integer> fieldsToCheckIsInitialized = new ArrayList<Integer>();
    for (int i = 0; i < fieldDescriptors.size(); ++i) {
      FieldDescriptor fd = fieldDescriptors.get(i);
      if (fd.isRequired()
          || (fd.getJavaType() == FieldDescriptor.JavaType.MESSAGE
              && needsIsInitializedCheck(fd.getMessageType()))) {
        fieldsToCheckIsInitialized.add(fd.getNumber());
      }
    }
    int[] numbers = new int[fieldsToCheckIsInitialized.size()];
    for (int i = 0; i < fieldsToCheckIsInitialized.size(); i++) {
      numbers[i] = fieldsToCheckIsInitialized.get(i);
    }
    builder.withCheckInitialized(numbers);

    return builder.build();
  }

  private static StructuralMessageInfo convertProto3(
      Class<?> messageType, Descriptor messageDescriptor) {
    List<FieldDescriptor> fieldDescriptors = messageDescriptor.getFields();
    StructuralMessageInfo.Builder builder =
        StructuralMessageInfo.newBuilder(fieldDescriptors.size());
    builder.withDefaultInstance(getDefaultInstance(messageType));
    builder.withSyntax(ProtoSyntax.PROTO3);

    OneofState oneofState = new OneofState();
    boolean enforceUtf8 = true;
    for (int i = 0; i < fieldDescriptors.size(); ++i) {
      FieldDescriptor fd = fieldDescriptors.get(i);
      if (fd.getContainingOneof() != null) {
        // Build a oneof member field.
        builder.withField(buildOneofMember(messageType, fd, oneofState, enforceUtf8, null));
        continue;
      }
      if (fd.isMapField()) {
        builder.withField(
            forMapField(
                field(messageType, fd),
                fd.getNumber(),
                SchemaUtil.getMapDefaultEntry(messageType, fd.getName()),
                null));
        continue;
      }
      if (fd.isRepeated() && fd.getJavaType() == FieldDescriptor.JavaType.MESSAGE) {
        builder.withField(
            forRepeatedMessageField(
                field(messageType, fd),
                fd.getNumber(),
                getFieldType(fd),
                getTypeForRepeatedMessageField(messageType, fd)));
        continue;
      }
      if (fd.isPacked()) {
        builder.withField(
            forPackedField(
                field(messageType, fd),
                fd.getNumber(),
                getFieldType(fd),
                cachedSizeField(messageType, fd)));
      } else {
        builder.withField(
            forField(field(messageType, fd), fd.getNumber(), getFieldType(fd), enforceUtf8));
      }
    }

    return builder.build();
  }

  /** Builds info for a oneof member field. */
  private static FieldInfo buildOneofMember(
      Class<?> messageType,
      FieldDescriptor fd,
      OneofState oneofState,
      boolean enforceUtf8,
      Internal.EnumVerifier enumVerifier) {
    OneofInfo oneof = oneofState.getOneof(messageType, fd.getContainingOneof());
    FieldType type = getFieldType(fd);
    Class<?> oneofStoredType = getOneofStoredType(messageType, fd, type);
    return forOneofMemberField(
        fd.getNumber(), type, oneof, oneofStoredType, enforceUtf8, enumVerifier);
  }

  private static Class<?> getOneofStoredType(
      Class<?> messageType, FieldDescriptor fd, FieldType type) {
    switch (type.getJavaType()) {
      case BOOLEAN:
        return Boolean.class;
      case BYTE_STRING:
        return ByteString.class;
      case DOUBLE:
        return Double.class;
      case FLOAT:
        return Float.class;
      case ENUM:
      case INT:
        return Integer.class;
      case LONG:
        return Long.class;
      case STRING:
        return String.class;
      case MESSAGE:
        return getOneofStoredTypeForMessage(messageType, fd);
      default:
        throw new IllegalArgumentException("Invalid type for oneof: " + type);
    }
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
        if (fd.isMapField()) {
          return FieldType.MAP;
        }
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

  private static Field cachedSizeField(Class<?> messageType, FieldDescriptor fd) {
    return field(messageType, getCachedSizeFieldName(fd));
  }

  private static Field field(Class<?> messageType, String fieldName) {
    try {
      return messageType.getDeclaredField(fieldName);
    } catch (Exception e) {
      throw new IllegalArgumentException(
          "Unable to find field " + fieldName + " in message class " + messageType.getName());
    }
  }

  static String getFieldName(FieldDescriptor fd) {
    String name = (fd.getType() == FieldDescriptor.Type.GROUP)
                  ? fd.getMessageType().getName()
                  : fd.getName();
    String suffix = specialFieldNames.contains(name) ? "__" : "_";
    return snakeCaseToCamelCase(name) + suffix;
  }

  private static String getCachedSizeFieldName(FieldDescriptor fd) {
    return snakeCaseToCamelCase(fd.getName()) + "MemoizedSerializedSize";
  }

  /**
   * This method must match exactly with the corresponding function in protocol compiler. See:
   * https://github.com/google/protobuf/blob/v3.0.0/src/google/protobuf/compiler/java/java_helpers.cc#L153
   */
  private static String snakeCaseToCamelCase(String snakeCase) {
    StringBuilder sb = new StringBuilder(snakeCase.length() + 1);
    boolean capNext = false;
    for (int ctr = 0; ctr < snakeCase.length(); ctr++) {
      char next = snakeCase.charAt(ctr);
      if (next == '_') {
        capNext = true;
      } else if (Character.isDigit(next)) {
        sb.append(next);
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
    return sb.toString();
  }

  /**
   * Inspects the message to identify the stored type for a message field that is part of a oneof.
   */
  private static Class<?> getOneofStoredTypeForMessage(Class<?> messageType, FieldDescriptor fd) {
    try {
      String name = fd.getType() == Type.GROUP ? fd.getMessageType().getName() : fd.getName();
      Method getter = messageType.getDeclaredMethod(getterForField(name));
      return getter.getReturnType();
    } catch (Exception e) {
      throw new RuntimeException(e);
    }
  }

  /** Inspects the message to identify the message type of a repeated message field. */
  private static Class<?> getTypeForRepeatedMessageField(Class<?> messageType, FieldDescriptor fd) {
    try {
      String name = fd.getType() == Type.GROUP ? fd.getMessageType().getName() : fd.getName();
      Method getter = messageType.getDeclaredMethod(getterForField(name), int.class);
      return getter.getReturnType();
    } catch (Exception e) {
      throw new RuntimeException(e);
    }
  }

  /** Constructs the name of the get method for the given field in the proto. */
  private static String getterForField(String snakeCase) {
    String camelCase = snakeCaseToCamelCase(snakeCase);
    StringBuilder builder = new StringBuilder("get");
    // Capitalize the first character in the field name.
    builder.append(Character.toUpperCase(camelCase.charAt(0)));
    builder.append(camelCase.substring(1, camelCase.length()));
    return builder.toString();
  }

  private static final class OneofState {
    private OneofInfo[] oneofs = new OneofInfo[2];

    OneofInfo getOneof(Class<?> messageType, OneofDescriptor desc) {
      int index = desc.getIndex();
      if (index >= oneofs.length) {
        // Grow the array.
        oneofs = Arrays.copyOf(oneofs, index * 2);
      }
      OneofInfo info = oneofs[index];
      if (info == null) {
        info = newInfo(messageType, desc);
        oneofs[index] = info;
      }
      return info;
    }

    private static OneofInfo newInfo(Class<?> messageType, OneofDescriptor desc) {
      String camelCase = snakeCaseToCamelCase(desc.getName());
      String valueFieldName = camelCase + "_";
      String caseFieldName = camelCase + "Case_";

      return new OneofInfo(
          desc.getIndex(), field(messageType, caseFieldName), field(messageType, valueFieldName));
    }
  }
}
