//
// upb - a minimalist implementation of protocol buffers.
//
// Copyright (c) 2011-2012 Google Inc.  See LICENSE for details.
// Author: Josh Haberman <jhaberman@gmail.com>

#include <string>
#include <typeinfo>
#include "upb/bytestream.hpp"
#include "upb/def.hpp"
#include "upb/handlers.hpp"
#include "upb/msg.hpp"
#include "upb/proto2_bridge.hpp"

namespace {

static void* GetFieldPointer(void *message, const upb::FieldDef* f) {
  return static_cast<char*>(message) + f->offset();
}

}  // namespace

#ifdef UPB_GOOGLE3

// TODO(haberman): friend upb so that this isn't required.
#define protected public
#include "net/proto2/public/repeated_field.h"
#undef private

#define private public
#include "net/proto/proto2_reflection.h"
#undef private

#include "net/proto2/proto/descriptor.pb.h"
#include "net/proto2/public/descriptor.h"
#include "net/proto2/public/generated_message_reflection.h"
#include "net/proto2/public/lazy_field.h"
#include "net/proto2/public/message.h"
#include "net/proto2/public/string_piece_field_support.h"
#include "net/proto/internal_layout.h"
#include "strings/cord.h"
using ::proto2::Descriptor;
using ::proto2::EnumDescriptor;
using ::proto2::EnumValueDescriptor;
using ::proto2::FieldDescriptor;
using ::proto2::FieldOptions;
using ::proto2::FileDescriptor;
using ::proto2::internal::GeneratedMessageReflection;
using ::proto2::internal::RepeatedPtrFieldBase;
using ::proto2::internal::StringPieceField;
using ::proto2::Message;
using ::proto2::MessageFactory;
using ::proto2::Reflection;
using ::proto2::RepeatedField;
using ::proto2::RepeatedPtrField;

namespace upb {

static const Message* GetPrototypeForField(const Message& m,
                                           const FieldDescriptor* f);

namespace proto2_bridge_google3 { class FieldAccessor; }

using ::upb::proto2_bridge_google3::FieldAccessor;

namespace proto2_bridge_google3 {

static void AssignToCord(const ByteRegion* r, Cord* cord) {
  // TODO(haberman): ref source data if source is a cord.
  cord->Clear();
  uint64_t ofs = r->start_ofs();
  while (ofs < r->end_ofs()) {
    size_t len;
    const char *buf = r->GetPtr(ofs, &len);
    cord->Append(StringPiece(buf, len));
    ofs += len;
  }
}

#else

// TODO(haberman): friend upb so that this isn't required.
#define protected public
#include "google/protobuf/repeated_field.h"
#undef protected

#define private public
#include "google/protobuf/generated_message_reflection.h"
#undef private

#include "google/protobuf/descriptor.h"
#include "google/protobuf/descriptor.pb.h"
#include "google/protobuf/message.h"
using ::google::protobuf::Descriptor;
using ::google::protobuf::EnumDescriptor;
using ::google::protobuf::EnumValueDescriptor;
using ::google::protobuf::FieldDescriptor;
using ::google::protobuf::FieldOptions;
using ::google::protobuf::FileDescriptor;
using ::google::protobuf::internal::GeneratedMessageReflection;
using ::google::protobuf::internal::RepeatedPtrFieldBase;
using ::google::protobuf::Message;
using ::google::protobuf::MessageFactory;
using ::google::protobuf::Reflection;
using ::google::protobuf::RepeatedField;
using ::google::protobuf::RepeatedPtrField;

namespace upb {
static const Message* GetPrototypeForField(const Message& m,
                                           const FieldDescriptor* f);

namespace proto2_bridge_opensource { class FieldAccessor; }

using ::upb::proto2_bridge_opensource::FieldAccessor;

namespace proto2_bridge_opensource {

#endif  // ifdef UPB_GOOGLE3

// Have to define this manually since older versions of proto2 didn't define
// an enum value for STRING.
#define UPB_CTYPE_STRING 0

// The code in this class depends on the internal representation of the proto2
// generated classes, which is an internal implementation detail of proto2 and
// is not a public interface.  As a result, this class's implementation may
// need to be changed if/when proto2 changes its internal representation.  It
// is intended that this class is the only code that depends on these internal,
// non-public interfaces.
//
// This class only works with messages that use GeneratedMessageReflection.
// Other reflection classes will need other accessor implementations.
class FieldAccessor {
 public:
  // Returns true if we were able to set an accessor and any other properties
  // of the FieldDef that are necessary to read/write this field to a
  // proto2::Message.
  static bool TrySet(const FieldDescriptor* proto2_f,
                     const upb::MessageDef* md,
                     upb::FieldDef* upb_f) {
    const Message* prototype = static_cast<const Message*>(md->prototype);
    const Reflection* base_r = prototype->GetReflection();
    const GeneratedMessageReflection* r =
        dynamic_cast<const GeneratedMessageReflection*>(base_r);
    // Old versions of the open-source protobuf release erroneously default to
    // Cord even though that has never been supported in the open-source
    // release.
    int32_t ctype = proto2_f->options().has_ctype() ?
        proto2_f->options().ctype() : UPB_CTYPE_STRING;
    if (!r) return false;
    // Extensions not supported yet.
    if (proto2_f->is_extension()) return false;

    upb_f->set_accessor(GetForFieldDescriptor(proto2_f, ctype));
    upb_f->set_hasbit(GetHasbit(proto2_f, r));
    upb_f->set_offset(GetOffset(proto2_f, r));
    if (upb_f->IsSubmessage()) {
      upb_f->set_subtype_name(proto2_f->message_type()->full_name());
      upb_f->prototype = GetPrototypeForField(*prototype, proto2_f);
    }

    if (upb_f->IsString() && !upb_f->IsSequence() &&
        ctype == UPB_CTYPE_STRING) {
      upb_f->prototype = &r->GetStringReference(*prototype, proto2_f, NULL);
    }
    return true;
  }

  static MessageFactory* GetMessageFactory(const Message& m) {
    const GeneratedMessageReflection* r =
        dynamic_cast<const GeneratedMessageReflection*>(m.GetReflection());
    return r ? r->message_factory_ : NULL;
  }

 private:
  static int64_t GetHasbit(const FieldDescriptor* f,
                           const GeneratedMessageReflection* r) {
    if (f->is_repeated()) {
      // proto2 does not store hasbits for repeated fields.
      return -1;
    } else {
      return (r->has_bits_offset_ * 8) + f->index();
    }
  }

  static uint16_t GetOffset(const FieldDescriptor* f,
                            const GeneratedMessageReflection* r) {
    return r->offsets_[f->index()];
  }

  static AccessorVTable *GetForFieldDescriptor(const FieldDescriptor* f,
                                               int32_t ctype) {
    switch (f->cpp_type()) {
      case FieldDescriptor::CPPTYPE_ENUM:
        // Should handlers validate enum membership to match proto2?
      case FieldDescriptor::CPPTYPE_INT32: return Get<int32_t>();
      case FieldDescriptor::CPPTYPE_INT64: return Get<int64_t>();
      case FieldDescriptor::CPPTYPE_UINT32: return Get<uint32_t>();
      case FieldDescriptor::CPPTYPE_UINT64: return Get<uint64_t>();
      case FieldDescriptor::CPPTYPE_DOUBLE: return Get<double>();
      case FieldDescriptor::CPPTYPE_FLOAT: return Get<float>();
      case FieldDescriptor::CPPTYPE_BOOL: return Get<bool>();
      case FieldDescriptor::CPPTYPE_STRING:
        switch (ctype) {
#ifdef UPB_GOOGLE3
          case FieldOptions::STRING:
            return GetForString<string>();
          case FieldOptions::CORD:
            return GetForCord();
          case FieldOptions::STRING_PIECE:
            return GetForStringPiece();
#else
          case UPB_CTYPE_STRING:
            return GetForString<std::string>();
#endif
          default: return NULL;
        }
      case FieldDescriptor::CPPTYPE_MESSAGE:
#ifdef UPB_GOOGLE3
        if (f->options().lazy()) {
          return NULL;  // Not yet implemented.
        } else {
          return GetForMessage();
        }
#else
        return GetForMessage();
#endif
      default: return NULL;
    }
  }

  // PushOffset handler (used for StartSequence and others)  ///////////////////

  static SubFlow PushOffset(void *m, Value fval) {
    const FieldDef *f = GetValue<const FieldDef*>(fval);
    return UPB_CONTINUE_WITH(GetFieldPointer(m, f));
  }

  // Primitive Value (numeric, enum, bool) /////////////////////////////////////

  template <typename T> static AccessorVTable *Get() {
    static upb_accessor_vtbl vtbl = {
      NULL,  // StartSubMessage handler
      GetValueHandler<T>(),
      &PushOffset,  // StartSequence handler
      NULL,  // StartRepeatedSubMessage handler
      &Append<T>,
      NULL, NULL, NULL, NULL, NULL, NULL};
    return &vtbl;
  }

  template <typename T>
  static Flow Append(void *_r, Value fval, Value val) {
    (void)fval;
    RepeatedField<T>* r = static_cast<RepeatedField<T>*>(_r);
    r->Add(GetValue<T>(val));
    return UPB_CONTINUE;
  }

  // String ////////////////////////////////////////////////////////////////////

  template <typename T> static AccessorVTable *GetForString() {
    static upb_accessor_vtbl vtbl = {
      NULL,  // StartSubMessage handler
      &SetString<T>,
      &PushOffset,  // StartSequence handler
      NULL,  // StartRepeatedSubMessage handler
      &AppendString<T>,
      NULL, NULL, NULL, NULL, NULL, NULL};
    return &vtbl;
  }

  // This needs to be templated because google3 string is not std::string.
  template <typename T> static Flow SetString(void *m, Value fval, Value val) {
    const FieldDef* f = GetValue<const FieldDef*>(fval);
    T **str = static_cast<T**>(GetFieldPointer(m, f));
    // If it points to the default instance, we must create a new instance.
    if (*str == f->prototype) *str = new T();
    GetValue<ByteRegion*>(val)->AssignToString(*str);
    return UPB_CONTINUE;
  }

  template <typename T>
  static Flow AppendString(void *_r, Value fval, Value val) {
    (void)fval;
    RepeatedPtrField<T>* r = static_cast<RepeatedPtrField<T>*>(_r);
    GetValue<ByteRegion*>(val)->AssignToString(r->Add());
    return UPB_CONTINUE;
  }

  // SubMessage ////////////////////////////////////////////////////////////////

  static AccessorVTable *GetForMessage() {
    static upb_accessor_vtbl vtbl = {
      &StartSubMessage,
      NULL,  // Value handler
      &PushOffset,  // StartSequence handler
      &StartRepeatedSubMessage,
      NULL,  // Repeated value handler
      NULL, NULL, NULL, NULL, NULL, NULL};
    return &vtbl;
  }

  static SubFlow StartSubMessage(void *m, Value fval) {
    const FieldDef* f = GetValue<const FieldDef*>(fval);
    void **subm = static_cast<void**>(GetFieldPointer(m, f));
    if (*subm == NULL || *subm == f->prototype) {
      const Message* prototype = static_cast<const Message*>(f->prototype);
      *subm = prototype->New();
    }
    return UPB_CONTINUE_WITH(*subm);
  }

  class RepeatedMessageTypeHandler {
   public:
    typedef void Type;
    // AddAllocated() calls this, but only if other objects are sitting
    // around waiting for reuse, which we will not do.
    static void Delete(Type* t) {
      (void)t;
      assert(false);
    }
  };

  // Closure is a RepeatedPtrField<SubMessageType>*, but we access it through
  // its base class RepeatedPtrFieldBase*.
  static SubFlow StartRepeatedSubMessage(void* _r, Value fval) {
    const FieldDef* f = GetValue<const FieldDef*>(fval);
    RepeatedPtrFieldBase *r = static_cast<RepeatedPtrFieldBase*>(_r);
    void *submsg = r->AddFromCleared<RepeatedMessageTypeHandler>();
    if (!submsg) {
      const Message* prototype = static_cast<const Message*>(f->prototype);
      submsg = prototype->New();
      r->AddAllocated<RepeatedMessageTypeHandler>(submsg);
    }
    return UPB_CONTINUE_WITH(submsg);
  }

  // TODO(haberman): handle Extensions, Unknown Fields.

#ifdef UPB_GOOGLE3
  // Handlers for types/features only included in internal proto2 release:
  // Cord, StringPiece, LazyField, and MessageSet.
  // TODO(haberman): LazyField, MessageSet.

  // Cord //////////////////////////////////////////////////////////////////////

  static AccessorVTable *GetForCord() {
    static upb_accessor_vtbl vtbl = {
      NULL,  // StartSubMessage handler
      &SetCord,
      &PushOffset,  // StartSequence handler
      NULL,  // StartRepeatedSubMessage handler
      &AppendCord,
      NULL, NULL, NULL, NULL, NULL, NULL};
    return &vtbl;
  }

  static Flow SetCord(void *m, Value fval, Value val) {
    const FieldDef* f = GetValue<const FieldDef*>(fval);
    Cord* field = static_cast<Cord*>(GetFieldPointer(m, f));
    AssignToCord(GetValue<ByteRegion*>(val), field);
    return UPB_CONTINUE;
  }

  static Flow AppendCord(void *_r, Value fval, Value val) {
    RepeatedField<Cord>* r = static_cast<RepeatedField<Cord>*>(_r);
    AssignToCord(GetValue<ByteRegion*>(val), r->Add());
    return UPB_CONTINUE;
  }

  // StringPiece ///////////////////////////////////////////////////////////////

  static AccessorVTable *GetForStringPiece() {
    static upb_accessor_vtbl vtbl = {
      NULL,  // StartSubMessage handler
      &SetStringPiece,
      &PushOffset,  // StartSequence handler
      NULL,  // StartRepeatedSubMessage handler
      &AppendStringPiece,
      NULL, NULL, NULL, NULL, NULL, NULL};
    return &vtbl;
  }

  static void AssignToStringPieceField(const ByteRegion* r,
                                       proto2::internal::StringPieceField* f) {
    // TODO(haberman): alias if possible and enabled on the input stream.
    // TODO(haberman): add a method to StringPieceField that lets us avoid
    // this copy/malloc/free.
    char *data = new char[r->Length()];
    r->Copy(r->start_ofs(), r->Length(), data);
    f->CopyFrom(StringPiece(data, r->Length()));
    delete[] data;
  }

  static Flow SetStringPiece(void *m, Value fval, Value val) {
    const FieldDef* f = GetValue<const FieldDef*>(fval);
    StringPieceField* field =
        static_cast<StringPieceField*>(GetFieldPointer(m, f));
    AssignToStringPieceField(GetValue<ByteRegion*>(val), field);
    return UPB_CONTINUE;
  }

  static Flow AppendStringPiece(void* _r, Value fval, Value val) {
    RepeatedPtrField<StringPieceField>* r =
        static_cast<RepeatedPtrField<StringPieceField>*>(_r);
    AssignToStringPieceField(GetValue<ByteRegion*>(val), r->Add());
    return UPB_CONTINUE;
  }

#endif  // UPB_GOOGLE3
};

#ifdef UPB_GOOGLE3

// Proto1 accessor -- only needed inside Google.
class Proto1FieldAccessor {
 public:
  // Returns true if we were able to set an accessor and any other properties
  // of the FieldDef that are necessary to read/write this field to a
  // proto2::Message.
  static bool TrySet(const FieldDescriptor* proto2_f,
                     const upb::MessageDef* md,
                     upb::FieldDef* upb_f) {
    const Message* m = static_cast<const Message*>(md->prototype);
    const proto2::Reflection* base_r = m->GetReflection();
    const _pi::Proto2Reflection* r =
        dynamic_cast<const _pi::Proto2Reflection*>(base_r);
    if (!r) return false;
    // Extensions not supported yet.
    if (proto2_f->is_extension()) return false;

    const _pi::Field* f = r->GetFieldLayout(proto2_f);

    if (f->crep == _pi::CREP_OPTIONAL_FOREIGN_WEAK) {
      // Override the BYTES type that proto2 descriptors have for weak fields.
      upb_f->set_type(UPB_TYPE(MESSAGE));
    }

    if (upb_f->IsSubmessage()) {
      const Message* prototype = upb::GetPrototypeForField(*m, proto2_f);
      upb_f->set_subtype_name(prototype->GetDescriptor()->full_name());
      upb_f->prototype = prototype;
    }

    upb_f->set_accessor(GetForCrep(f->crep));
    upb_f->set_hasbit(GetHasbit(proto2_f, r));
    upb_f->set_offset(GetOffset(proto2_f, r));
    return true;
  }

 private:
  static int16_t GetHasbit(const FieldDescriptor* f,
                           const _pi::Proto2Reflection* r) {
    if (f->is_repeated()) {
      // proto1 does not store hasbits for repeated fields.
      return -1;
    } else {
      return (r->layout_->has_bit_offset * 8) + r->GetFieldLayout(f)->has_index;
    }
  }

  static uint16_t GetOffset(const FieldDescriptor* f,
                            const _pi::Proto2Reflection* r) {
    return r->GetFieldLayout(f)->offset;
  }

  static AccessorVTable *GetForCrep(int crep) {
#define PRIMITIVE(name, type_name) \
    case _pi::CREP_REQUIRED_ ## name: \
    case _pi::CREP_OPTIONAL_ ## name: \
    case _pi::CREP_REPEATED_ ## name: return Get<type_name>();

    switch (crep) {
      PRIMITIVE(DOUBLE,   double);
      PRIMITIVE(FLOAT,    float);
      PRIMITIVE(INT64,    int64_t);
      PRIMITIVE(UINT64,   uint64_t);
      PRIMITIVE(INT32,    int32_t);
      PRIMITIVE(FIXED64,  uint64_t);
      PRIMITIVE(FIXED32,  uint32_t);
      PRIMITIVE(BOOL,     bool);
      case _pi::CREP_REQUIRED_STRING:
      case _pi::CREP_OPTIONAL_STRING:
      case _pi::CREP_REPEATED_STRING: return GetForString();
      case _pi::CREP_OPTIONAL_OUTOFLINE_STRING: return GetForOutOfLineString();
      case _pi::CREP_REQUIRED_CORD:
      case _pi::CREP_OPTIONAL_CORD:
      case _pi::CREP_REPEATED_CORD: return GetForCord();
      case _pi::CREP_REQUIRED_GROUP:
      case _pi::CREP_REQUIRED_FOREIGN:
      case _pi::CREP_REQUIRED_FOREIGN_PROTO2: return GetForRequiredMessage();
      case _pi::CREP_OPTIONAL_GROUP:
      case _pi::CREP_REPEATED_GROUP:
      case _pi::CREP_OPTIONAL_FOREIGN:
      case _pi::CREP_REPEATED_FOREIGN:
      case _pi::CREP_OPTIONAL_FOREIGN_PROTO2:
      case _pi::CREP_REPEATED_FOREIGN_PROTO2: return GetForMessage();
      case _pi::CREP_OPTIONAL_FOREIGN_WEAK: return GetForWeakMessage();
      default: assert(false); return NULL;
    }
#undef PRIMITIVE
  }

  // PushOffset handler (used for StartSequence and others)  ///////////////////

  // We can find a RepeatedField* or a RepeatedPtrField* at f->offset().
  static SubFlow PushOffset(void *m, Value fval) {
    const FieldDef *f = GetValue<const FieldDef*>(fval);
    return UPB_CONTINUE_WITH(GetFieldPointer(m, f));
  }

  // Primitive Value (numeric, enum, bool) /////////////////////////////////////

  template <typename T> static AccessorVTable *Get() {
    static upb_accessor_vtbl vtbl = {
      NULL,  // StartSubMessage handler
      GetValueHandler<T>(),
      &PushOffset,  // StartSequence handler
      NULL,  // StartRepeatedSubMessage handler
      &Append<T>,
      NULL, NULL, NULL, NULL, NULL, NULL};
    return &vtbl;
  }

  template <typename T>
  static Flow Append(void *_r, Value fval, Value val) {
    (void)fval;
    // Proto1's ProtoArray class derives from RepeatedField.
    RepeatedField<T>* r = static_cast<RepeatedField<T>*>(_r);
    r->Add(GetValue<T>(val));
    return UPB_CONTINUE;
  }

  // String ////////////////////////////////////////////////////////////////////

  static AccessorVTable *GetForString() {
    static upb_accessor_vtbl vtbl = {
      NULL,  // StartSubMessage handler
      &SetString,
      &PushOffset,  // StartSequence handler
      NULL,  // StartRepeatedSubMessage handler
      &AppendString,
      NULL, NULL, NULL, NULL, NULL, NULL};
    return &vtbl;
  }

  static Flow SetString(void *m, Value fval, Value val) {
    const FieldDef* f = GetValue<const FieldDef*>(fval);
    string *str = static_cast<string*>(GetFieldPointer(m, f));
    GetValue<ByteRegion*>(val)->AssignToString(str);
    return UPB_CONTINUE;
  }

  static Flow AppendString(void *_r, Value fval, Value val) {
    (void)fval;
    RepeatedPtrField<string>* r = static_cast<RepeatedPtrField<string>*>(_r);
    GetValue<ByteRegion*>(val)->AssignToString(r->Add());
    return UPB_CONTINUE;
  }

  // Out-of-line string ////////////////////////////////////////////////////////

  static AccessorVTable *GetForOutOfLineString() {
    static upb_accessor_vtbl vtbl = {
      NULL, &SetOutOfLineString,
      // This type is only used for non-repeated string fields.
      NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};
    return &vtbl;
  }

  static Flow SetOutOfLineString(void *m, Value fval, Value val) {
    const FieldDef* f = GetValue<const FieldDef*>(fval);
    string **str = static_cast<string**>(GetFieldPointer(m, f));
    if (*str == &::ProtocolMessage::___empty_internal_proto_string_)
      *str = new string();
    GetValue<ByteRegion*>(val)->AssignToString(*str);
    return UPB_CONTINUE;
  }

  // Cord //////////////////////////////////////////////////////////////////////

  static AccessorVTable *GetForCord() {
    static upb_accessor_vtbl vtbl = {
      NULL,  // StartSubMessage handler
      &SetCord,
      &PushOffset,  // StartSequence handler
      NULL,  // StartRepeatedSubMessage handler
      &AppendCord,
      NULL, NULL, NULL, NULL, NULL, NULL};
    return &vtbl;
  }

  static Flow SetCord(void *m, Value fval, Value val) {
    const FieldDef* f = GetValue<const FieldDef*>(fval);
    Cord* field = static_cast<Cord*>(GetFieldPointer(m, f));
    AssignToCord(GetValue<ByteRegion*>(val), field);
    return UPB_CONTINUE;
  }

  static Flow AppendCord(void *_r, Value fval, Value val) {
    RepeatedField<Cord>* r = static_cast<RepeatedField<Cord>*>(_r);
    AssignToCord(GetValue<ByteRegion*>(val), r->Add());
    return UPB_CONTINUE;
  }

  // SubMessage ////////////////////////////////////////////////////////////////

  static AccessorVTable *GetForRequiredMessage() {
    static upb_accessor_vtbl vtbl = {
      &PushOffset,  // StartSubMessage handler
      NULL,  // Value handler
      &PushOffset,  // StartSequence handler
      &StartRepeatedSubMessage,
      NULL,  // Repeated value handler
      NULL, NULL, NULL, NULL, NULL, NULL};
    return &vtbl;
  }

  static AccessorVTable *GetForWeakMessage() {
    static upb_accessor_vtbl vtbl = {
      &StartWeakSubMessage,  // StartSubMessage handler
      NULL,  // Value handler
      &PushOffset,  // StartSequence handler
      &StartRepeatedSubMessage,
      NULL,  // Repeated value handler
      NULL, NULL, NULL, NULL, NULL, NULL};
    return &vtbl;
  }

  static AccessorVTable *GetForMessage() {
    static upb_accessor_vtbl vtbl = {
      &StartSubMessage,
      NULL,  // Value handler
      &PushOffset,  // StartSequence handler
      &StartRepeatedSubMessage,
      NULL,  // Repeated value handler
      NULL, NULL, NULL, NULL, NULL, NULL};
    return &vtbl;
  }

  static SubFlow StartSubMessage(void *m, Value fval) {
    const FieldDef* f = GetValue<const FieldDef*>(fval);
    Message **subm = static_cast<Message**>(GetFieldPointer(m, f));
    if (*subm == f->prototype) *subm = (*subm)->New();
    return UPB_CONTINUE_WITH(*subm);
  }

  static SubFlow StartWeakSubMessage(void *m, Value fval) {
    const FieldDef* f = GetValue<const FieldDef*>(fval);
    Message **subm = static_cast<Message**>(GetFieldPointer(m, f));
    if (*subm == NULL) {
      const Message* prototype = static_cast<const Message*>(f->prototype);
      *subm = prototype->New();
    }
    return UPB_CONTINUE_WITH(*subm);
  }

  class RepeatedMessageTypeHandler {
   public:
    typedef void Type;
    // AddAllocated() calls this, but only if other objects are sitting
    // around waiting for reuse, which we will not do.
    static void Delete(Type* t) {
      (void)t;
      assert(false);
    }
  };

  // Closure is a RepeatedPtrField<SubMessageType>*, but we access it through
  // its base class RepeatedPtrFieldBase*.
  static SubFlow StartRepeatedSubMessage(void* _r, Value fval) {
    const FieldDef* f = GetValue<const FieldDef*>(fval);
    RepeatedPtrFieldBase *r = static_cast<RepeatedPtrFieldBase*>(_r);
    void *submsg = r->AddFromCleared<RepeatedMessageTypeHandler>();
    if (!submsg) {
      const Message* prototype = static_cast<const Message*>(f->prototype);
      submsg = prototype->New();
      r->AddAllocated<RepeatedMessageTypeHandler>(submsg);
    }
    return UPB_CONTINUE_WITH(submsg);
  }
};

#endif

}  // namespace proto2_bridge_{google3,opensource}

static const Message* GetPrototypeForMessage(const Message& m) {
  const Message* ret = NULL;
  MessageFactory* factory = FieldAccessor::GetMessageFactory(m);
  if (factory) {
    // proto2 generated message or DynamicMessage.
    ret = factory->GetPrototype(m.GetDescriptor());
    assert(ret);
  } else {
    // Proto1 message; since proto1 has no dynamic message, it must be
    // from the generated factory.
    ret = MessageFactory::generated_factory()->GetPrototype(m.GetDescriptor());
    assert(ret);  // If NULL, then wasn't a proto1 message, can't handle it.
  }
  assert(ret->GetReflection() == m.GetReflection());
  return ret;
}

static const Message* GetPrototypeForField(const Message& m,
                                           const FieldDescriptor* f) {
#ifdef UPB_GOOGLE3
  if (f->type() == FieldDescriptor::TYPE_BYTES) {
    // Proto1 weak field: the proto2 descriptor says their type is BYTES.
    const _pi::Proto2Reflection* r =
        dynamic_cast<const _pi::Proto2Reflection*>(m.GetReflection());
    assert(r);
    const _pi::Field* field = r->GetFieldLayout(f);
    assert(field->crep == _pi::CREP_OPTIONAL_FOREIGN_WEAK);
    return GetPrototypeForMessage(
        *static_cast<const Message*>(field->weak_layout()->default_instance));
  } else if (dynamic_cast<const _pi::Proto2Reflection*>(m.GetReflection())) {
    // Proto1 message; since proto1 has no dynamic message, it must be from
    // the generated factory.
    const Message* ret =
        MessageFactory::generated_factory()->GetPrototype(f->message_type());
    assert(ret);
    return ret;
  }
#endif
  assert(f->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE);
  // We assume that all submessages (and extensions) will be constructed using
  // the same MessageFactory as this message.  This doesn't cover the case of
  // CodedInputStream::SetExtensionRegistry().
  MessageFactory* factory = FieldAccessor::GetMessageFactory(m);
  assert(factory);  // If neither proto1 nor proto2 we can't handle it.
  const Message* ret = factory->GetPrototype(f->message_type());
  assert(ret);
  return ret;
}

namespace proto2_bridge {

upb::FieldDef* AddFieldDef(const FieldDescriptor* f, upb::MessageDef* md) {
  upb::FieldDef* upb_f = upb::FieldDef::New(&upb_f);
  upb_f->set_number(f->number());
  upb_f->set_name(f->name());
  upb_f->set_label(static_cast<upb::Label>(f->label()));
  upb_f->set_type(static_cast<upb::FieldType>(f->type()));

  if (!FieldAccessor::TrySet(f, md, upb_f)
#ifdef UPB_GOOGLE3
      && !proto2_bridge_google3::Proto1FieldAccessor::TrySet(f, md, upb_f)
#endif
     ) {
    // Unsupported reflection class.
    assert(false);
  }

  if (upb_f->type() == UPB_TYPE(ENUM)) {
    // We set the enum default symbolically.
    upb_f->set_default(f->default_value_enum()->name());
    upb_f->set_subtype_name(f->enum_type()->full_name());
  } else {
    // Set field default for primitive types.  Need to switch on the upb type
    // rather than the proto2 type, because upb_f->type() may have been changed
    // from BYTES to MESSAGE for a weak field.
    switch (upb_types[upb_f->type()].inmemory_type) {
      case UPB_CTYPE_INT32:
        upb_f->set_default(MakeValue(f->default_value_int32()));
        break;
      case UPB_CTYPE_INT64:
        upb_f->set_default(
            MakeValue(static_cast<int64_t>(f->default_value_int64())));
        break;
      case UPB_CTYPE_UINT32:
        upb_f->set_default(MakeValue(f->default_value_uint32()));
        break;
      case UPB_CTYPE_UINT64:
        upb_f->set_default(
            MakeValue(static_cast<uint64_t>(f->default_value_uint64())));
        break;
      case UPB_CTYPE_DOUBLE:
        upb_f->set_default(MakeValue(f->default_value_double()));
        break;
      case UPB_CTYPE_FLOAT:
        upb_f->set_default(MakeValue(f->default_value_float()));
        break;
      case UPB_CTYPE_BOOL:
        upb_f->set_default(MakeValue(f->default_value_bool()));
        break;
      case UPB_CTYPE_BYTEREGION:
        upb_f->set_default(f->default_value_string());
        break;
    }
  }
  return md->AddField(upb_f, &upb_f) ? upb_f : NULL;
}

upb::MessageDef *NewEmptyMessageDef(const Message& m, void *owner) {
  upb::MessageDef *md = upb::MessageDef::New(owner);
  md->set_full_name(m.GetDescriptor()->full_name());
  md->prototype = GetPrototypeForMessage(m);
  return md;
}

upb::EnumDef* NewEnumDef(const EnumDescriptor* desc, void *owner) {
  upb::EnumDef* e = upb::EnumDef::New(owner);
  e->set_full_name(desc->full_name());
  for (int i = 0; i < desc->value_count(); i++) {
    const EnumValueDescriptor* val = desc->value(i);
    bool success = e->AddValue(val->name(), val->number());
    assert(success);
    (void)success;
  }
  return e;
}

void AddAllFields(upb::MessageDef* md) {
  const Descriptor* d =
      static_cast<const Message*>(md->prototype)->GetDescriptor();
  for (int i = 0; i < d->field_count(); i++) {
#ifdef UPB_GOOGLE3
    // Skip lazy fields for now since we can't properly handle them.
    if (d->field(i)->options().lazy()) continue;
#endif
    // Extensions not supported yet.
    if (d->field(i)->is_extension()) continue;
    AddFieldDef(d->field(i), md);
  }
}

upb::MessageDef *NewFullMessageDef(const Message& m, void *owner) {
  upb::MessageDef* md = NewEmptyMessageDef(m, owner);
  AddAllFields(md);
  // TODO(haberman): add unknown field handler and extensions.
  return md;
}

typedef std::map<std::string, upb::Def*> SymbolMap;

static upb::MessageDef* NewFinalMessageDefHelper(const Message& m, void *owner,
                                                 SymbolMap* symbols) {
  upb::MessageDef* md = NewFullMessageDef(m, owner);
  // Must do this before processing submessages to prevent infinite recursion.
  (*symbols)[std::string(md->full_name())] = md->AsDef();

  for (upb::MessageDef::Iterator i(md); !i.Done(); i.Next()) {
    upb::FieldDef* f = i.field();
    if (!f->HasSubDef()) continue;
    SymbolMap::iterator iter = symbols->find(f->subtype_name());
    upb::Def* subdef;
    if (iter != symbols->end()) {
      subdef = iter->second;
    } else {
      const FieldDescriptor* proto2_f =
          m.GetDescriptor()->FindFieldByNumber(f->number());
      if (f->type() == UPB_TYPE(ENUM)) {
        subdef = NewEnumDef(proto2_f->enum_type(), owner)->AsDef();
        (*symbols)[std::string(subdef->full_name())] = subdef;
      } else {
        assert(f->IsSubmessage());
        const Message* prototype = GetPrototypeForField(m, proto2_f);
        subdef = NewFinalMessageDefHelper(*prototype, owner, symbols)->AsDef();
      }
    }
    f->set_subdef(subdef);
  }
  return md;
}

const upb::MessageDef* NewFinalMessageDef(const Message& m, void *owner) {
  SymbolMap symbols;
  upb::MessageDef* ret = NewFinalMessageDefHelper(m, owner, &symbols);

  // Finalize defs.
  std::vector<upb::Def*> defs;
  SymbolMap::iterator iter;
  for (iter = symbols.begin(); iter != symbols.end(); ++iter) {
    defs.push_back(iter->second);
  }
  Status status;
  bool success = Def::Finalize(defs, &status);
  assert(success);
  (void)success;

  // Unref all defs except the top-level one that we are returning.
  for (int i = 0; i < static_cast<int>(defs.size()); i++) {
    if (defs[i] != ret->AsDef()) defs[i]->Unref(owner);
  }

  return ret;
}

}  // namespace proto2_bridge
}  // namespace upb
