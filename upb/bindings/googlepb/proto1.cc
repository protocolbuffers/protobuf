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

#include "upb/bindings/googlepb/proto1.int.h"

#include <memory>

// TEMPORARY measure until we update the friend declarations in proto1.
// Can't do in a single CL because of components.
#define private public
#define protected public
#include "net/proto2/public/repeated_field.h"
#include "net/proto/internal_layout.h"
#include "net/proto/proto2_reflection.h"
#undef private
#undef protected

#include "upb/def.h"
#include "upb/handlers.h"
#include "upb/shim/shim.h"
#include "upb/sink.h"

// Unconditionally evaluate, but also assert in debug mode.
#define CHKRET(x) do { bool ok = (x); UPB_UNUSED(ok); assert(ok); } while (0)

template <class T> static T* GetPointer(void* message, size_t offset) {
  return reinterpret_cast<T*>(static_cast<char*>(message) + offset);
}

namespace upb {
namespace googlepb {

class P2R_Handlers {
 public:
  // Returns true if we were able to set an accessor and any other properties
  // of the FieldDef that are necessary to read/write this field to a
  // proto2::Message.
  static bool TrySet(const proto2::FieldDescriptor* proto2_f,
                     const proto2::Message& m, const upb::FieldDef* upb_f,
                     upb::Handlers* h) {
    const proto2::Reflection* base_r = m.GetReflection();
    // See file comment re: dynamic_cast.
    const _pi::Proto2Reflection* r =
        dynamic_cast<const _pi::Proto2Reflection*>(base_r);
    if (!r) return false;
    // Extensions don't exist in proto1.
    assert(!proto2_f->is_extension());

#define PRIMITIVE(name, type_name)                                             \
  case _pi::CREP_REQUIRED_##name:                                              \
  case _pi::CREP_OPTIONAL_##name:                                              \
  case _pi::CREP_REPEATED_##name:                                              \
    SetPrimitiveHandlers<type_name>(proto2_f, r, upb_f, h);                    \
    return true;

    switch (r->GetFieldLayout(proto2_f)->crep) {
      PRIMITIVE(DOUBLE, double);
      PRIMITIVE(FLOAT, float);
      PRIMITIVE(INT64, int64_t);
      PRIMITIVE(UINT64, uint64_t);
      PRIMITIVE(INT32, int32_t);
      PRIMITIVE(FIXED64, uint64_t);
      PRIMITIVE(FIXED32, uint32_t);
      PRIMITIVE(BOOL, bool);
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
      default:
        assert(false);
        return false;
    }
  }

#undef PRIMITIVE

  // If the field "f" in the message "m" is a weak field, returns the prototype
  // of the submessage (which may be a specific type or may be OpaqueMessage).
  // Otherwise returns NULL.
  static const proto2::Message* GetWeakPrototype(
      const proto2::Message& m, const proto2::FieldDescriptor* f) {
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
      const proto2::Message& m, const proto2::FieldDescriptor* f) {
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
    FieldOffset(const proto2::FieldDescriptor* f,
                const _pi::Proto2Reflection* r)
        : offset_(GetOffset(f, r)), is_repeated_(f->is_repeated()) {
      if (!is_repeated_) {
        int64_t hasbit = GetHasbit(f, r);
        hasbyte_ = hasbit / 8;
        mask_ = 1 << (hasbit % 8);
      }
    }

    template <class T> T* GetFieldPointer(proto2::Message* message) const {
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

  template <class T>
  static void SetStartRepeatedField(
      const proto2::FieldDescriptor* proto2_f, const _pi::Proto2Reflection* r,
      const upb::FieldDef* f, upb::Handlers* h) {
    CHKRET(h->SetStartSequenceHandler(
        f, UpbBindT(PushOffset<proto2::RepeatedField<T> >,
                    new FieldOffset(proto2_f, r))));
  }

  template <class T>
  static void SetStartRepeatedPtrField(
      const proto2::FieldDescriptor* proto2_f, const _pi::Proto2Reflection* r,
      const upb::FieldDef* f, upb::Handlers* h) {
    CHKRET(h->SetStartSequenceHandler(
        f, UpbBindT(PushOffset<proto2::RepeatedPtrField<T> >,
                    new FieldOffset(proto2_f, r))));
  }

  static void SetStartRepeatedSubmessageField(
      const proto2::FieldDescriptor* proto2_f, const _pi::Proto2Reflection* r,
      const upb::FieldDef* f, upb::Handlers* h) {
    CHKRET(h->SetStartSequenceHandler(
        f, UpbBind(PushOffset<proto2::internal::RepeatedPtrFieldBase>,
                   new FieldOffset(proto2_f, r))));
  }

  template <class T>
  static T* PushOffset(proto2::Message* m, const FieldOffset* offset) {
    return offset->GetFieldPointer<T>(m);
  }

  // Primitive Value (numeric, enum, bool) /////////////////////////////////////

  template <typename T>
  static void SetPrimitiveHandlers(const proto2::FieldDescriptor* proto2_f,
                                   const _pi::Proto2Reflection* r,
                                   const upb::FieldDef* f, upb::Handlers* h) {
    if (f->IsSequence()) {
      SetStartRepeatedField<T>(proto2_f, r, f, h);
      CHKRET(h->SetValueHandler<T>(f, UpbMakeHandlerT(Append<T>)));
    } else {
      CHKRET(
          upb::Shim::Set(h, f, GetOffset(proto2_f, r), GetHasbit(proto2_f, r)));
    }
  }

  template <typename T>
  static void Append(proto2::RepeatedField<T>* r, T val) {
    // Proto1's ProtoArray class derives from proto2::RepeatedField.
    r->Add(val);
  }

  // String ////////////////////////////////////////////////////////////////////

  static void SetStringHandlers(const proto2::FieldDescriptor* proto2_f,
                                const _pi::Proto2Reflection* r,
                                const upb::FieldDef* f, upb::Handlers* h) {
    h->SetStringHandler(f, UpbMakeHandler(OnStringBuf));
    if (f->IsSequence()) {
      SetStartRepeatedPtrField<string>(proto2_f, r, f, h);
      CHKRET(h->SetStartStringHandler(f, UpbMakeHandler(StartRepeatedString)));
    } else {
      CHKRET(h->SetStartStringHandler(
          f, UpbBind(StartString, new FieldOffset(proto2_f, r))));
    }
  }

  static string* StartString(proto2::Message* m, const FieldOffset* info,
                             size_t size_hint) {
    info->SetHasbit(m);
    string* str = info->GetFieldPointer<string>(m);
    str->clear();
    // reserve() here appears to hurt performance rather than help.
    return str;
  }

  static void OnStringBuf(string* s, const char* buf, size_t n) {
    s->append(buf, n);
  }

  static string* StartRepeatedString(proto2::RepeatedPtrField<string>* r,
                                     size_t size_hint) {
    string* str = r->Add();
    // reserve() here appears to hurt performance rather than help.
    return str;
  }

  // Out-of-line string ////////////////////////////////////////////////////////

  static void SetOutOfLineStringHandlers(
      const proto2::FieldDescriptor* proto2_f, const _pi::Proto2Reflection* r,
      const upb::FieldDef* f, upb::Handlers* h) {
    // This type is only used for non-repeated string fields.
    assert(!f->IsSequence());
    CHKRET(h->SetStartStringHandler(
        f, UpbBind(StartOutOfLineString, new FieldOffset(proto2_f, r))));
    CHKRET(h->SetStringHandler(f, UpbMakeHandler(OnStringBuf)));
  }

  static string* StartOutOfLineString(proto2::Message* m,
                                      const FieldOffset* info,
                                      size_t size_hint) {
    info->SetHasbit(m);
    string** str = info->GetFieldPointer<string*>(m);
    if (*str == &::proto2::internal::GetEmptyString())
      *str = new string();
    (*str)->clear();
    // reserve() here appears to hurt performance rather than help.
    return *str;
  }

  // Cord //////////////////////////////////////////////////////////////////////

  static void SetCordHandlers(const proto2::FieldDescriptor* proto2_f,
                              const _pi::Proto2Reflection* r,
                              const upb::FieldDef* f, upb::Handlers* h) {
    if (f->IsSequence()) {
      SetStartRepeatedField<Cord>(proto2_f, r, f, h);
      CHKRET(h->SetStartStringHandler(f, UpbMakeHandler(StartRepeatedCord)));
    } else {
      CHKRET(h->SetStartStringHandler(
          f, UpbBind(StartCord, new FieldOffset(proto2_f, r))));
    }
    CHKRET(h->SetStringHandler(f, UpbMakeHandler(OnCordBuf)));
  }

  static Cord* StartCord(proto2::Message* m, const FieldOffset* offset,
                         size_t size_hint) {
    UPB_UNUSED(size_hint);
    offset->SetHasbit(m);
    Cord* field = offset->GetFieldPointer<Cord>(m);
    field->Clear();
    return field;
  }

  static void OnCordBuf(Cord* c, const char* buf, size_t n) {
    c->Append(StringPiece(buf, n));
  }

  static Cord* StartRepeatedCord(proto2::RepeatedField<Cord>* r,
                                 size_t size_hint) {
    UPB_UNUSED(size_hint);
    return r->Add();
  }

  // SubMessage ////////////////////////////////////////////////////////////////

  class SubMessageHandlerData : public FieldOffset {
   public:
    SubMessageHandlerData(const proto2::Message& prototype,
                          const proto2::FieldDescriptor* f,
                          const _pi::Proto2Reflection* r)
        : FieldOffset(f, r) {
      prototype_ = GetWeakPrototype(prototype, f);
      if (!prototype_) prototype_ = GetFieldPrototype(prototype, f);
    }

    const proto2::Message* prototype() const { return prototype_; }

   private:
    const proto2::Message* prototype_;
  };

  static void SetRequiredMessageHandlers(
      const proto2::FieldDescriptor* proto2_f, const proto2::Message& m,
      const _pi::Proto2Reflection* r, const upb::FieldDef* f,
      upb::Handlers* h) {
    if (f->IsSequence()) {
      SetStartRepeatedSubmessageField(proto2_f, r, f, h);
      CHKRET(h->SetStartSubMessageHandler(
          f, UpbBind(StartRepeatedSubMessage,
                     new SubMessageHandlerData(m, proto2_f, r))));
    } else {
      CHKRET(h->SetStartSubMessageHandler(
          f, UpbBind(StartRequiredSubMessage, new FieldOffset(proto2_f, r))));
    }
  }

  static proto2::Message* StartRequiredSubMessage(proto2::Message* m,
                                                  const FieldOffset* offset) {
    offset->SetHasbit(m);
    return offset->GetFieldPointer<proto2::Message>(m);
  }

  static void SetMessageHandlers(const proto2::FieldDescriptor* proto2_f,
                                 const proto2::Message& m,
                                 const _pi::Proto2Reflection* r,
                                 const upb::FieldDef* f, upb::Handlers* h) {
    std::unique_ptr<SubMessageHandlerData> data(
        new SubMessageHandlerData(m, proto2_f, r));
    if (f->IsSequence()) {
      SetStartRepeatedSubmessageField(proto2_f, r, f, h);
      CHKRET(h->SetStartSubMessageHandler(
          f, UpbBind(StartRepeatedSubMessage, data.release())));
    } else {
      CHKRET(h->SetStartSubMessageHandler(
          f, UpbBind(StartSubMessage, data.release())));
    }
  }

  static void SetWeakMessageHandlers(const proto2::FieldDescriptor* proto2_f,
                                     const proto2::Message& m,
                                     const _pi::Proto2Reflection* r,
                                     const upb::FieldDef* f, upb::Handlers* h) {
    std::unique_ptr<SubMessageHandlerData> data(
        new SubMessageHandlerData(m, proto2_f, r));
    if (f->IsSequence()) {
      SetStartRepeatedSubmessageField(proto2_f, r, f, h);
      CHKRET(h->SetStartSubMessageHandler(
          f, UpbBind(StartRepeatedSubMessage, data.release())));
    } else {
      CHKRET(h->SetStartSubMessageHandler(
          f, UpbBind(StartWeakSubMessage, data.release())));
    }
  }

  static void* StartSubMessage(proto2::Message* m,
                               const SubMessageHandlerData* info) {
    info->SetHasbit(m);
    proto2::Message** subm = info->GetFieldPointer<proto2::Message*>(m);
    if (*subm == info->prototype()) *subm = (*subm)->New();
    return *subm;
  }

  static void* StartWeakSubMessage(proto2::Message* m,
                                   const SubMessageHandlerData* info) {
    info->SetHasbit(m);
    proto2::Message** subm = info->GetFieldPointer<proto2::Message*>(m);
    if (*subm == NULL) {
      *subm = info->prototype()->New();
    }
    return *subm;
  }

  class RepeatedMessageTypeHandler {
   public:
    typedef proto2::Message Type;
    // AddAllocated() calls this, but only if other objects are sitting
    // around waiting for reuse, which we will not do.
    static void Delete(Type* t) {
      UPB_UNUSED(t);
      assert(false);
    }
  };

  // Closure is a RepeatedPtrField<SubMessageType>*, but we access it through
  // its base class RepeatedPtrFieldBase*.
  static proto2::Message* StartRepeatedSubMessage(
      proto2::internal::RepeatedPtrFieldBase* r,
      const SubMessageHandlerData* info) {
    proto2::Message* submsg = r->AddFromCleared<RepeatedMessageTypeHandler>();
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
  return googlepb::P2R_Handlers::TrySet(proto2_f, m, upb_f, h);
}

const proto2::Message* GetProto1FieldPrototype(
    const proto2::Message& m, const proto2::FieldDescriptor* f) {
  const proto2::Message *weak = googlepb::P2R_Handlers::GetWeakPrototype(m, f);
  if (weak) return weak;
  if (f->cpp_type() != proto2::FieldDescriptor::CPPTYPE_MESSAGE) {
    return NULL;
  }
  return googlepb::P2R_Handlers::GetFieldPrototype(m, f);
}

}  // namespace googlepb
}  // namespace upb
