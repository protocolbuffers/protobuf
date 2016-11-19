#include <google/protobuf/util/message_hasher.h>

GOOGLE_PROTOBUF_HASH_NAMESPACE_DECLARATION_START

// service declarations

template<typename T>
size_t stdhashfunc(const T& t) {
  return std::hash<T>()(t);
}

using ::google::protobuf::Message;
using ::google::protobuf::Reflection;
using ::google::protobuf::FieldDescriptor;

inline size_t HashForSimpleType(const Message& message, const Reflection& reflection, const FieldDescriptor& field, int index = -1);
inline size_t HashForRepeatedType(const Message& message, const Reflection& reflection, const FieldDescriptor& field);
inline size_t HashForMessageType(const Message& message);

size_t hash<::google::protobuf::Message>::operator()(const ::google::protobuf::Message& m) const {
  return HashForMessageType(m);
}


// implementation

inline size_t HashForMessageType(const Message& message) {
  size_t hash = 1;
  const Reflection *reflection = message.GetReflection();
  std::vector<const FieldDescriptor*> fields;
  reflection->ListFields(message, &fields);
  for (int i = 0; i < fields.size(); ++i) {
    const FieldDescriptor *field = fields[i];
    if (field->is_repeated()) {
      hash ^= HashForRepeatedType(message, *reflection, *field);
    }
    else if (field->type() == FieldDescriptor::TYPE_MESSAGE) {
      if (reflection->HasField(message, field)) {
        hash ^= HashForMessageType(*reflection->MutableMessage(const_cast<Message*>(&message), field));
      }
    }
    else {
      hash ^= HashForSimpleType(message, *reflection, *field);
    }
  }
  return hash;
}


// NOTICE:
// As far as we keep "hash ^= X" operations in this method, it will support protobuf map<k,v> type, because it is 
// similar to "repeated message { key, value }".
inline size_t HashForRepeatedType(const Message& message, const Reflection& reflection, const FieldDescriptor& field) {
  size_t hash = 1;
  int size = reflection.FieldSize(message, &field);
  for (int i = 0; i < size; ++i) {
    if (field.type() == FieldDescriptor::TYPE_MESSAGE)
      hash ^= HashForMessageType(*reflection.MutableRepeatedMessage(const_cast<Message*>(&message), &field, i));
    else
      hash ^= HashForSimpleType(message, reflection, field, i);
  }
  return hash;
}

inline size_t HashForSimpleType(const Message& message, const Reflection& reflection, const FieldDescriptor& field, int index) {
  assert(!field.is_repeated() || index < reflection.FieldSize(message, &field));
  assert(reflection.HasField(message, &field));
  switch (field.cpp_type()) {
#define OUTPUT_FIELD(CPPTYPE, METHOD)                            \
    case FieldDescriptor::CPPTYPE_##CPPTYPE:                     \
      return stdhashfunc(field.is_repeated()                     \
        ? reflection.GetRepeated##METHOD(message, &field, index) \
        : reflection.Get##METHOD(message, &field));              \

    OUTPUT_FIELD(INT32, Int32)
    OUTPUT_FIELD(INT64, Int64)
    OUTPUT_FIELD(UINT32, UInt32)
    OUTPUT_FIELD(UINT64, UInt64)
    OUTPUT_FIELD(FLOAT, Float)
    OUTPUT_FIELD(DOUBLE, Double)
    OUTPUT_FIELD(BOOL, Bool)

#undef OUTPUT_FIELD

    case FieldDescriptor::CPPTYPE_STRING: {
      std::string scratch;
      return stdhashfunc(field.is_repeated()
        ? reflection.GetRepeatedStringReference(message, &field, index, &scratch)
        : reflection.GetStringReference(message, &field, &scratch));
    }

    case FieldDescriptor::CPPTYPE_ENUM:
      return stdhashfunc(field.is_repeated()
        ? reflection.GetRepeatedEnumValue(message, &field, index)
        : reflection.GetEnumValue(message, &field));
  
    default:
      assert(false);
      return 1;
  }
}

GOOGLE_PROTOBUF_HASH_NAMESPACE_DECLARATION_END
