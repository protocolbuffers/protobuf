//
// upb - a minimalist implementation of protocol buffers.
//
// Copyright (c) 2011-2012 Google Inc.  See LICENSE for details.
// Author: Josh Haberman <jhaberman@gmail.com>
//
// This set of handlers can write into a proto2::Message whose reflection class
// is _pi::Proto2Reflection (ie. proto1 messages; while slightly confusing, the
// name "Proto2Reflection" indicates that it is a reflection class implementing
// the proto2 reflection interface, but is used for proto1 generated messages).
//
// Like FieldAccessor this depends on breaking encapsulation, and will need to
// be changed if and when the details of _pi::Proto2Reflection change.
//
// Note that we have received an exception from c-style-artiters regarding
// dynamic_cast<> in this file:
// https://groups.google.com/a/google.com/d/msg/c-style/7Zp_XCX0e7s/I6dpzno4l-MJ

#include "upb/google/proto1.h"

// TODO(haberman): friend upb so that this isn't required.
#define protected public
#include "net/proto2/public/repeated_field.h"
#undef private

// TODO(haberman): friend upb so that this isn't required.
#define private public
#include "net/proto/proto2_reflection.h"
#undef private

#include "net/proto/internal_layout.h"
#include "upb/bytestream.h"
#include "upb/def.h"
#include "upb/google/cord.h"
#include "upb/handlers.h"

template<class T> static T* GetPointer(void *message, size_t offset) {
  return reinterpret_cast<T*>(static_cast<char*>(message) + offset);
}

namespace upb {
namespace google {

class P2R_Handlers {
 public:
  // Returns true if we were able to set an accessor and any other properties
  // of the FieldDef that are necessary to read/write this field to a
  // proto2::Message.
  static bool TrySet(const proto2::FieldDescriptor* proto2_f,
                     const proto2::Message& m,
                     const upb::FieldDef* upb_f, upb::Handlers* h) {
    const proto2::Reflection* base_r = m.GetReflection();
    // See file comment re: dynamic_cast.
    const _pi::Proto2Reflection* r =
        dynamic_cast<const _pi::Proto2Reflection*>(base_r);
    if (!r) return false;
    // Extensions not supported yet.
    if (proto2_f->is_extension()) return false;

    switch (r->GetFieldLayout(proto2_f)->crep) {
#define PRIMITIVE(name, type_name) \
      case _pi::CREP_REQUIRED_ ## name: \
      case _pi::CREP_OPTIONAL_ ## name: \
      case _pi::CREP_REPEATED_ ## name: \
        SetPrimitiveHandlers<type_name>(proto2_f, r, upb_f, h); return true;
      PRIMITIVE(DOUBLE,   double);
      PRIMITIVE(FLOAT,    float);
      PRIMITIVE(INT64,    int64_t);
      PRIMITIVE(UINT64,   uint64_t);
      PRIMITIVE(INT32,    int32_t);
      PRIMITIVE(FIXED64,  uint64_t);
      PRIMITIVE(FIXED32,  uint32_t);
      PRIMITIVE(BOOL,     bool);
#undef PRIMITIVE
      case _pi::CREP_REQUIRED_STRING:
      case _pi::CREP_OPTIONAL_STRING:
      case _pi::CREP_REPEATED_STRING:
        SetStringHandlers(proto2_f, r, upb_f, h);
        return true;
      case _pi::CREP_OPTIONAL_OUTOFLINE_STRING:
        SetOutOfLineStringHandlers(proto2_f, r, upb_f, h);
        return true;
      case _pi::CREP_REQUIRED_CORD:
      case _pi::CREP_OPTIONAL_CORD:
      case _pi::CREP_REPEATED_CORD:
        SetCordHandlers(proto2_f, r, upb_f, h);
        return true;
      case _pi::CREP_REQUIRED_GROUP:
      case _pi::CREP_REQUIRED_FOREIGN:
      case _pi::CREP_REQUIRED_FOREIGN_PROTO2:
        SetRequiredMessageHandlers(proto2_f, m, r, upb_f, h);
        return true;
      case _pi::CREP_OPTIONAL_GROUP:
      case _pi::CREP_REPEATED_GROUP:
      case _pi::CREP_OPTIONAL_FOREIGN:
      case _pi::CREP_REPEATED_FOREIGN:
      case _pi::CREP_OPTIONAL_FOREIGN_PROTO2:
      case _pi::CREP_REPEATED_FOREIGN_PROTO2:
        SetMessageHandlers(proto2_f, m, r, upb_f, h);
        return true;
      case _pi::CREP_OPTIONAL_FOREIGN_WEAK:
      case _pi::CREP_OPTIONAL_FOREIGN_WEAK_PROTO2:
        SetWeakMessageHandlers(proto2_f, m, r, upb_f, h);
        return true;
      default: assert(false); return false;
    }
  }

  // If the field "f" in the message "m" is a weak field, returns the prototype
  // of the submessage (which may be a specific type or may be OpaqueMessage).
  // Otherwise returns NULL.
  static const proto2::Message* GetWeakPrototype(
      const proto2::Message& m,
      const proto2::FieldDescriptor* f) {
    // See file comment re: dynamic_cast.
    const _pi::Proto2Reflection* r =
        dynamic_cast<const _pi::Proto2Reflection*>(m.GetReflection());
    if (!r) return NULL;

    const _pi::Field* field = r->GetFieldLayout(f);
    if (field->crep == _pi::CREP_OPTIONAL_FOREIGN_WEAK) {
      return static_cast<const proto2::Message*>(
          field->weak_layout()->default_instance);
    } else if (field->crep == _pi::CREP_OPTIONAL_FOREIGN_WEAK_PROTO2) {
      return field->proto2_weak_default_instance();
    } else {
      return NULL;
    }
  }

  // If "m" is a message that uses Proto2Reflection, returns the prototype of
  // the submessage (which may be OpaqueMessage for a weak field that is not
  // linked in).  Otherwise returns NULL.
  static const proto2::Message* GetFieldPrototype(
      const proto2::Message& m,
      const proto2::FieldDescriptor* f) {
    // See file comment re: dynamic_cast.
    const proto2::Message* ret = GetWeakPrototype(m, f);
    if (ret) {
      return ret;
    } else if (dynamic_cast<const _pi::Proto2Reflection*>(m.GetReflection())) {
      // Since proto1 has no dynamic message, it must be from the generated
      // factory.
      assert(f->cpp_type() == proto2::FieldDescriptor::CPPTYPE_MESSAGE);
      ret = proto2::MessageFactory::generated_factory()->GetPrototype(
          f->message_type());
      assert(ret);
      return ret;
    } else {
      return NULL;
    }
  }

 private:
  class FieldOffset {
   public:
    FieldOffset(
        const proto2::FieldDescriptor* f,
        const _pi::Proto2Reflection* r)
        : offset_(GetOffset(f, r)),
          is_repeated_(f->is_repeated()) {
      if (!is_repeated_) {
        int64_t hasbit = GetHasbit(f, r);
        hasbyte_ = hasbit / 8;
        mask_ = 1 << (hasbit % 8);
      }
    }

    template<class T> T* GetFieldPointer(void* message) const {
      return GetPointer<T>(message, offset_);
    }

    void SetHasbit(void* message) const {
      assert(!is_repeated_);
      uint8_t* byte = GetPointer<uint8_t>(message, hasbyte_);
      *byte |= mask_;
    }

   private:
    const size_t offset_;
    bool is_repeated_;

    // Only for non-repeated fields.
    int32_t hasbyte_;
    int8_t mask_;
  };

  static upb_selector_t GetSelector(const upb::FieldDef* f,
                                    upb::Handlers::Type type) {
    upb::Handlers::Selector selector;
    bool ok = upb::Handlers::GetSelector(f, type, &selector);
    UPB_ASSERT_VAR(ok, ok);
    return selector;
  }


  static int16_t GetHasbit(const proto2::FieldDescriptor* f,
                           const _pi::Proto2Reflection* r) {
    assert(!f->is_repeated());
    return (r->layout_->has_bit_offset * 8) + r->GetFieldLayout(f)->has_index;
  }

  static uint16_t GetOffset(const proto2::FieldDescriptor* f,
                            const _pi::Proto2Reflection* r) {
    return r->GetFieldLayout(f)->offset;
  }

  // StartSequence /////////////////////////////////////////////////////////////

  static void SetStartSequenceHandler(
      const proto2::FieldDescriptor* proto2_f, const _pi::Proto2Reflection* r,
      const upb::FieldDef* f, upb::Handlers* h) {
    assert(f->IsSequence());
    h->SetStartSequenceHandler(
        f, &PushOffset, new FieldOffset(proto2_f, r),
        &upb::DeletePointer<FieldOffset>);
  }

  static void* PushOffset(void *m, void *fval) {
    const FieldOffset* offset = static_cast<FieldOffset*>(fval);
    return offset->GetFieldPointer<void>(m);
  }

  // Primitive Value (numeric, enum, bool) /////////////////////////////////////

  template <typename T> static void SetPrimitiveHandlers(
      const proto2::FieldDescriptor* proto2_f,
      const _pi::Proto2Reflection* r,
      const upb::FieldDef* f, upb::Handlers* h) {
    if (f->IsSequence()) {
      SetStartSequenceHandler(proto2_f, r, f, h);
      h->SetValueHandler<T>(f, &Append<T>, NULL, NULL);
    } else {
      upb::SetStoreValueHandler<T>(
          f, GetOffset(proto2_f, r), GetHasbit(proto2_f, r), h);
    }
  }

  template <typename T>
  static bool Append(void *_r, void *fval, T val) {
    UPB_UNUSED(fval);
    // Proto1's ProtoArray class derives from proto2::RepeatedField.
    proto2::RepeatedField<T>* r = static_cast<proto2::RepeatedField<T>*>(_r);
    r->Add(val);
    return true;
  }

  // String ////////////////////////////////////////////////////////////////////

  static void SetStringHandlers(
      const proto2::FieldDescriptor* proto2_f,
      const _pi::Proto2Reflection* r,
      const upb::FieldDef* f, upb::Handlers* h) {
    h->SetStringHandler(f, &OnStringBuf, NULL, NULL);
    if (f->IsSequence()) {
      SetStartSequenceHandler(proto2_f, r, f, h);
      h->SetStartStringHandler(f, &StartRepeatedString, NULL, NULL);
    } else {
      h->SetStartStringHandler(
          f, &StartString, new FieldOffset(proto2_f, r),
          &upb::DeletePointer<FieldOffset>);
    }
  }

  static void* StartString(void *m, void *fval, size_t size_hint) {
    UPB_UNUSED(size_hint);
    const FieldOffset* info = static_cast<const FieldOffset*>(fval);
    info->SetHasbit(m);
    string* str = info->GetFieldPointer<string>(m);
    str->clear();
    // reserve() here appears to hurt performance rather than help.
    return str;
  }

  static size_t OnStringBuf(void *_s, void *fval, const char *buf, size_t n) {
    string* s = static_cast<string*>(_s);
    s->append(buf, n);
    return n;
  }

  static void* StartRepeatedString(void *_r, void *fval, size_t size_hint) {
    UPB_UNUSED(fval);
    proto2::RepeatedPtrField<string>* r =
        static_cast<proto2::RepeatedPtrField<string>*>(_r);
    string* str = r->Add();
    // reserve() here appears to hurt performance rather than help.
    return str;
  }

  // Out-of-line string ////////////////////////////////////////////////////////

  static void SetOutOfLineStringHandlers(
      const proto2::FieldDescriptor* proto2_f,
      const _pi::Proto2Reflection* r,
      const upb::FieldDef* f, upb::Handlers* h) {
    // This type is only used for non-repeated string fields.
    assert(!f->IsSequence());
    h->SetStartStringHandler(
        f, &StartOutOfLineString, new FieldOffset(proto2_f, r),
        &upb::DeletePointer<FieldOffset>);
    h->SetStringHandler(f, &OnStringBuf, NULL, NULL);
  }

  static void* StartOutOfLineString(void *m, void *fval, size_t size_hint) {
    const FieldOffset* info = static_cast<const FieldOffset*>(fval);
    info->SetHasbit(m);
    string **str = info->GetFieldPointer<string*>(m);
    if (*str == &::ProtocolMessage::___empty_internal_proto_string_)
      *str = new string();
    (*str)->clear();
    // reserve() here appears to hurt performance rather than help.
    return *str;
  }

  // Cord //////////////////////////////////////////////////////////////////////

  static void SetCordHandlers(
      const proto2::FieldDescriptor* proto2_f,
      const _pi::Proto2Reflection* r,
      const upb::FieldDef* f, upb::Handlers* h) {
    h->SetStringHandler(f, &OnCordBuf, NULL, NULL);
    if (f->IsSequence()) {
      SetStartSequenceHandler(proto2_f, r, f, h);
      h->SetStartStringHandler(f, &StartRepeatedCord, NULL, NULL);
    } else {
      h->SetStartStringHandler(
          f, &StartCord, new FieldOffset(proto2_f, r),
          &upb::DeletePointer<FieldOffset*>);
    }
  }

  static void* StartCord(void *m, void *fval, size_t size_hint) {
    UPB_UNUSED(size_hint);
    UPB_UNUSED(fval);
    const FieldOffset* offset = static_cast<const FieldOffset*>(fval);
    offset->SetHasbit(m);
    Cord* field = offset->GetFieldPointer<Cord>(m);
    field->Clear();
    return field;
  }

  static size_t OnCordBuf(void *_c, void *fval, const char *buf, size_t n) {
    UPB_UNUSED(fval);
    Cord* c = static_cast<Cord*>(_c);
    c->Append(StringPiece(buf, n));
    return true;
  }

  static void* StartRepeatedCord(void *_r, void *fval, size_t size_hint) {
    UPB_UNUSED(size_hint);
    UPB_UNUSED(fval);
    proto2::RepeatedField<Cord>* r =
        static_cast<proto2::RepeatedField<Cord>*>(_r);
    return r->Add();
  }

  // SubMessage ////////////////////////////////////////////////////////////////

  class SubMessageHandlerData : public FieldOffset {
   public:
    SubMessageHandlerData(
        const proto2::Message& prototype,
        const proto2::FieldDescriptor* f,
        const _pi::Proto2Reflection* r)
        : FieldOffset(f, r) {
      prototype_ = GetWeakPrototype(prototype, f);
      if (!prototype_)
        prototype_ = GetFieldPrototype(prototype, f);
    }

    const proto2::Message* prototype() const { return prototype_; }

   private:
    const proto2::Message* prototype_;
  };

  static void SetStartSubMessageHandler(
      const proto2::FieldDescriptor* proto2_f,
      const proto2::Message& m,
      const _pi::Proto2Reflection* r,
      upb::Handlers::StartFieldHandler* handler,
      const upb::FieldDef* f, upb::Handlers* h) {
    h->SetStartSubMessageHandler(
        f, handler,
        new SubMessageHandlerData(m, proto2_f, r),
        &upb::DeletePointer<SubMessageHandlerData>);
  }

  static void SetRequiredMessageHandlers(
      const proto2::FieldDescriptor* proto2_f,
      const proto2::Message& m,
      const _pi::Proto2Reflection* r,
      const upb::FieldDef* f, upb::Handlers* h) {
    if (f->IsSequence()) {
      SetStartSequenceHandler(proto2_f, r, f, h);
      SetStartSubMessageHandler(proto2_f, m, r, &StartRepeatedSubMessage, f, h);
    } else {
      h->SetStartSubMessageHandler(
          f, &StartRequiredSubMessage, new FieldOffset(proto2_f, r),
          &upb::DeletePointer<FieldOffset>);
    }
  }

  static void* StartRequiredSubMessage(void *m, void *fval) {
    const FieldOffset* offset = static_cast<FieldOffset*>(fval);
    offset->SetHasbit(m);
    return offset->GetFieldPointer<void>(m);
  }

  static void SetMessageHandlers(
      const proto2::FieldDescriptor* proto2_f,
      const proto2::Message& m,
      const _pi::Proto2Reflection* r,
      const upb::FieldDef* f, upb::Handlers* h) {
    if (f->IsSequence()) {
      SetStartSequenceHandler(proto2_f, r, f, h);
      SetStartSubMessageHandler(proto2_f, m, r, &StartRepeatedSubMessage, f, h);
    } else {
      SetStartSubMessageHandler(proto2_f, m, r, &StartSubMessage, f, h);
    }
  }

  static void SetWeakMessageHandlers(
      const proto2::FieldDescriptor* proto2_f,
      const proto2::Message& m,
      const _pi::Proto2Reflection* r,
      const upb::FieldDef* f, upb::Handlers* h) {
    if (f->IsSequence()) {
      SetStartSequenceHandler(proto2_f, r, f, h);
      SetStartSubMessageHandler(proto2_f, m, r, &StartRepeatedSubMessage, f, h);
    } else {
      SetStartSubMessageHandler(proto2_f, m, r, &StartWeakSubMessage, f, h);
    }
  }

  static void* StartSubMessage(void *m, void *fval) {
    const SubMessageHandlerData* info =
        static_cast<const SubMessageHandlerData*>(fval);
    info->SetHasbit(m);
    proto2::Message **subm = info->GetFieldPointer<proto2::Message*>(m);
    if (*subm == info->prototype()) *subm = (*subm)->New();
    return *subm;
  }

  static void* StartWeakSubMessage(void *m, void *fval) {
    const SubMessageHandlerData* info =
        static_cast<const SubMessageHandlerData*>(fval);
    info->SetHasbit(m);
    proto2::Message **subm = info->GetFieldPointer<proto2::Message*>(m);
    if (*subm == NULL) {
      *subm = info->prototype()->New();
    }
    return *subm;
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
  static void* StartRepeatedSubMessage(void* _r, void *fval) {
    const SubMessageHandlerData* info =
        static_cast<const SubMessageHandlerData*>(fval);
    proto2::internal::RepeatedPtrFieldBase *r =
        static_cast<proto2::internal::RepeatedPtrFieldBase*>(_r);
    void *submsg = r->AddFromCleared<RepeatedMessageTypeHandler>();
    if (!submsg) {
      submsg = info->prototype()->New();
      r->AddAllocated<RepeatedMessageTypeHandler>(submsg);
    }
    return submsg;
  }
};

bool TrySetProto1WriteHandlers(const proto2::FieldDescriptor* proto2_f,
                               const proto2::Message& m,
                               const upb::FieldDef* upb_f, upb::Handlers* h) {
  return P2R_Handlers::TrySet(proto2_f, m, upb_f, h);
}

const proto2::Message* GetProto1WeakPrototype(
    const proto2::Message& m,
    const proto2::FieldDescriptor* f) {
  return P2R_Handlers::GetWeakPrototype(m, f);
}

const proto2::Message* GetProto1FieldPrototype(
    const proto2::Message& m,
    const proto2::FieldDescriptor* f) {
  return P2R_Handlers::GetFieldPrototype(m, f);
}

}  // namespace google
}  // namespace upb
