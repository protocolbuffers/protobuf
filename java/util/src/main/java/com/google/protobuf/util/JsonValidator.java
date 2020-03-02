package com.google.protobuf.util;

import com.google.protobuf.Descriptors;
import com.google.protobuf.Descriptors.Descriptor;
import com.google.protobuf.Descriptors.FieldDescriptor;
import com.google.protobuf.Descriptors.FieldDescriptor.JavaType;
import com.google.protobuf.Message;
import com.google.protobuf.Value;
import com.google.protobuf.Value.KindCase;

import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.util.HashMap;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;

/**
 * Util class to validate JSON against a predefined protobuf schema.
 * JSON data is parsed into a tree with root type {@link com.google.protobuf.Value}.
 *
 * @author Lawrence He
 */
public final class JsonValidator
{
  /**
   * Validate dynamic JSON data against supplied Java Protobuf class
   */
  public static List<String> validate(Value root, Class<? extends Message> clazz)
  {
    List<String> warnings = new LinkedList<String>();

    if (root == null) {
      warnings.add("Json data is undefined");
      return warnings;
    }

    int depth = 0;

    // traverse two trees at same time
    start(depth, root, clazz, warnings);
    return warnings;
  }

  static Descriptor getMessageDescriptor(Class<? extends Message> protoClass)
  {
    try {
      Method getDescriptor = protoClass.getMethod("getDescriptor");
      return (Descriptor) getDescriptor.invoke((Object) null);
    }
    catch (NoSuchMethodException | IllegalAccessException var2) {
      throw new IllegalArgumentException(var2);
    }
    catch (InvocationTargetException var3) {
      return null;
    }
  }

  static void start(int depth, Value root, Class<? extends Message> clazz, List<String> warnings)
  {
    Descriptors.Descriptor descriptor = getMessageDescriptor(clazz);
    dfs(depth, descriptor.getFields(), root, warnings);
  }

  static void dfs(int depth, List<FieldDescriptor> fieldDescriptors, Value val, List<String> warnings)
  {
    Map<String, Value> m = null;
    if (val.hasStructValue()) {

      m = new HashMap<>(val.getStructValue().getFieldsMap());
    }

    for (FieldDescriptor field : fieldDescriptors) {
      String key = field.getName();
      if (m != null && m.containsKey(key)) {
        helper(++depth, field, m.get(key), warnings);
        depth--;
      }
      else {
        // schema defined but not included
        // possibly "optional"
        // generate warning for now
        // warnings.add(field.getName() + " is not included.");
      }
    }
  }

  static void helper(int depth, FieldDescriptor field, Value val, List<String> warnings)
  {
    if (val.getKindCase().equals(KindCase.NULL_VALUE)) {
      // allow any field to be null for now
      return;
    }
    if (field.getJavaType() == JavaType.MESSAGE) {
      if (!val.hasListValue() && !val.hasStructValue()) {
        warnings.add(field.getName() + " supposed to be object but not.");
      }
      else {
        handleNode(depth, field, val, warnings);
      }
      return;
    }

    if (field.isRepeated()) {
      if (!val.hasListValue()) {
        warnings.add(field.getName() + " supposed to be list but not.");
      }
      return;
    }

    handleConcreteData(field, val, warnings);
  }

  static void handleNode(int depth, FieldDescriptor field, Value node, List<String> warnings)
  {
    if (field.isMapField()) {
      if (!node.hasStructValue()) {
        warnings.add(field.getName() + " supposed to be map but not.");
        return;
      }
      List<FieldDescriptor> fields = field.getMessageType().getFields();
      if (fields.size() != 2) {
        throw new UnsupportedOperationException("Both map key and value are required.");
      }


      for (Entry<String, Value> entry : node.getStructValue().getFieldsMap().entrySet()) {
        helper(++depth, fields.get(1), entry.getValue(), warnings);
      }
      return;
    }

    if (field.isRepeated()) {
      if (!node.hasListValue()) {
        warnings.add(field.getName() + " supposed to be list but not.");
        return;
      }
      for (Value v : node.getListValue().getValuesList()) {
        dfs(depth, field.getMessageType().getFields(), v, warnings);
      }
      return;
    }

    dfs(depth, field.getMessageType().getFields(), node, warnings);
  }

  /**
   * Sample schema data handling
   */
  static void handleConcreteData(FieldDescriptor field, Value val, List<String> warnings)
  {
    JavaType javaType = field.getJavaType();

    switch (field.getJavaType()) {
      case STRING:
        if (!val.getKindCase().equals(KindCase.STRING_VALUE)) {
          warnings.add(field.getName() + " supposed to be string but not.");
        }
        break;
      case INT:
      case LONG:
      case FLOAT:
      case DOUBLE:
        if (!val.getKindCase().equals(KindCase.NUMBER_VALUE)) {
          warnings.add(field.getName() + " supposed to be numeric but not.");
        }
        break;
      case BOOLEAN:
        if (!val.getKindCase().equals(KindCase.BOOL_VALUE)) {
          warnings.add(field.getName() + " supposed to be boolean but not.");
        }
        break;
      default:
        throw new UnsupportedOperationException("Cannot convert protobuf type: unsupported type " + javaType);
    }
  }
}
