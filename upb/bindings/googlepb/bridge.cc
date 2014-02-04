//
// upb - a minimalist implementation of protocol buffers.
//
// Copyright (c) 2011-2012 Google Inc.  See LICENSE for details.
// Author: Josh Haberman <jhaberman@gmail.com>
//
// IMPORTANT NOTE!  Inside Google, This file is compiled TWICE, once with
// UPB_GOOGLE3 defined and once without!  This allows us to provide
// functionality against proto2 and protobuf opensource both in a single binary
// without the two conflicting.  However we must be careful not to violate the
// ODR.

#include "upb/bindings/googlepb/bridge.h"

#include <stdio.h>
#include <map>
#include <string>
#include "upb/def.h"
#include "upb/bindings/googlepb/proto1.h"
#include "upb/bindings/googlepb/proto2.h"
#include "upb/handlers.h"

#define ASSERT_STATUS(status) do { \
  if (!upb_ok(status)) { \
    fprintf(stderr, "upb status failure: %s\n", upb_status_errmsg(status)); \
    assert(upb_ok(status)); \
  } \
  } while (0)

#ifdef UPB_GOOGLE3
#include "net/proto2/public/descriptor.h"
#include "net/proto2/public/message.h"
#include "net/proto2/proto/descriptor.pb.h"
namespace goog = ::proto2;
#else
#include "google/protobuf/descriptor.h"
#include "google/protobuf/message.h"
#include "google/protobuf/descriptor.pb.h"
namespace goog = ::google::protobuf;
#endif

namespace {

const goog::Message* GetPrototype(const goog::Message& m,
                                  const goog::FieldDescriptor* f) {
  const goog::Message* ret = NULL;
#ifdef UPB_GOOGLE3
  ret = upb::google::GetProto1WeakPrototype(m, f);
  if (ret) return ret;
#endif

  if (f->cpp_type() == goog::FieldDescriptor::CPPTYPE_MESSAGE) {
    ret = upb::google::GetFieldPrototype(m, f);
#ifdef UPB_GOOGLE3
    if (!ret) ret = upb::google::GetProto1FieldPrototype(m, f);
#endif
    assert(ret);
  }
  return ret;
}

}  // namespace

namespace upb {
namespace googlepb {


/* DefBuilder  ****************************************************************/

const EnumDef* DefBuilder::GetEnumDef(const goog::EnumDescriptor* ed) {
  const EnumDef* cached = FindInCache<EnumDef>(ed);
  if (cached) return cached;

  EnumDef* e = AddToCache(ed, EnumDef::New());

  Status status;
  e->set_full_name(ed->full_name(), &status);
  for (int i = 0; i < ed->value_count(); i++) {
    const goog::EnumValueDescriptor* val = ed->value(i);
    bool success = e->AddValue(val->name(), val->number(), &status);
    UPB_ASSERT_VAR(success, success);
  }

  e->Freeze(&status);

  ASSERT_STATUS(&status);
  return e;
}

const MessageDef* DefBuilder::GetMaybeUnfrozenMessageDef(
    const goog::Descriptor* d, const goog::Message* m) {
  const MessageDef* cached = FindInCache<MessageDef>(d);
  if (cached) return cached;

  MessageDef* md = AddToCache(d, MessageDef::New());
  to_freeze_.push_back(upb::upcast(md));

  Status status;
  md->set_full_name(d->full_name(), &status);
  ASSERT_STATUS(&status);

  // Find all regular fields and extensions for this message.
  std::vector<const goog::FieldDescriptor*> fields;
  d->file()->pool()->FindAllExtensions(d, &fields);
  for (int i = 0; i < d->field_count(); i++) {
    fields.push_back(d->field(i));
  }

  for (int i = 0; i < fields.size(); i++) {
    const goog::FieldDescriptor* proto2_f = fields[i];
    assert(proto2_f);
    md->AddField(NewFieldDef(proto2_f, m), &status);
  }
  ASSERT_STATUS(&status);
  return md;
}

reffed_ptr<FieldDef> DefBuilder::NewFieldDef(const goog::FieldDescriptor* f,
                                             const goog::Message* m) {
  const goog::Message* subm = NULL;
  const goog::Message* weak_prototype = NULL;

  if (m) {
#ifdef UPB_GOOGLE3
    weak_prototype = upb::google::GetProto1WeakPrototype(*m, f);
#endif
    subm = GetPrototype(*m, f);
  }

  reffed_ptr<FieldDef> upb_f(FieldDef::New());
  Status status;
  upb_f->set_number(f->number(), &status);
  upb_f->set_label(FieldDef::ConvertLabel(f->label()));
#ifdef UPB_GOOGLE3
  upb_f->set_lazy(f->options().lazy());
#endif

  if (f->is_extension()) {
    upb_f->set_name(f->full_name(), &status);
    upb_f->set_is_extension(true);
  } else {
    upb_f->set_name(f->name(), &status);
  }

  // For weak fields, weak_prototype will be non-NULL even though the proto2
  // descriptor does not indicate a submessage field.
  upb_f->set_descriptor_type(weak_prototype
                                 ? UPB_DESCRIPTOR_TYPE_MESSAGE
                                 : FieldDef::ConvertDescriptorType(f->type()));

  switch (upb_f->type()) {
    case UPB_TYPE_INT32:
      upb_f->set_default_int32(f->default_value_int32());
      break;
    case UPB_TYPE_INT64:
      upb_f->set_default_int64(f->default_value_int64());
      break;
    case UPB_TYPE_UINT32:
      upb_f->set_default_uint32(f->default_value_uint32());
      break;
    case UPB_TYPE_UINT64:
      upb_f->set_default_uint64(f->default_value_uint64());
      break;
    case UPB_TYPE_DOUBLE:
      upb_f->set_default_double(f->default_value_double());
      break;
    case UPB_TYPE_FLOAT:
      upb_f->set_default_float(f->default_value_float());
      break;
    case UPB_TYPE_BOOL:
      upb_f->set_default_bool(f->default_value_bool());
      break;
    case UPB_TYPE_STRING:
    case UPB_TYPE_BYTES:
      upb_f->set_default_string(f->default_value_string(), &status);
      break;
    case UPB_TYPE_MESSAGE: {
      const goog::Descriptor* subd =
          subm ? subm->GetDescriptor() : f->message_type();
      upb_f->set_message_subdef(GetMaybeUnfrozenMessageDef(subd, subm),
                                &status);
      break;
    }
    case UPB_TYPE_ENUM:
      // We set the enum default numerically.
      upb_f->set_default_int32(f->default_value_enum()->number());
      upb_f->set_enum_subdef(GetEnumDef(f->enum_type()), &status);
      break;
  }

  ASSERT_STATUS(&status);
  return upb_f;
}

void DefBuilder::Freeze() {
  upb::Status status;
  upb::Def::Freeze(to_freeze_, &status);
  ASSERT_STATUS(&status);
  to_freeze_.clear();
}

const MessageDef* DefBuilder::GetMessageDef(const goog::Descriptor* d) {
  const MessageDef* ret = GetMaybeUnfrozenMessageDef(d, NULL);
  Freeze();
  return ret;
}

const MessageDef* DefBuilder::GetMessageDefExpandWeak(
    const goog::Message& m) {
  const MessageDef* ret = GetMaybeUnfrozenMessageDef(m.GetDescriptor(), &m);
  Freeze();
  return ret;
}


/* CodeCache  *****************************************************************/

const Handlers* CodeCache::GetMaybeUnfrozenWriteHandlers(
    const MessageDef* md, const goog::Message& m) {
  const Handlers* cached = FindInCache(md);
  if (cached) return cached;

  Handlers* h = AddToCache(md, upb::Handlers::New(md));
  to_freeze_.push_back(h);
  const goog::Descriptor* d = m.GetDescriptor();

  for (upb::MessageDef::const_iterator i = md->begin(); i != md->end(); ++i) {
    const FieldDef* upb_f = *i;

    const goog::FieldDescriptor* proto2_f =
        d->FindFieldByNumber(upb_f->number());
    if (!proto2_f) {
      proto2_f = d->file()->pool()->FindExtensionByNumber(d, upb_f->number());
    }
    assert(proto2_f);

    if (!upb::google::TrySetWriteHandlers(proto2_f, m, upb_f, h)
#ifdef UPB_GOOGLE3
        && !upb::google::TrySetProto1WriteHandlers(proto2_f, m, upb_f, h)
#endif
        ) {
      // Unsupported reflection class.
      //
      // Should we fall back to using the public Reflection interface in this
      // case?  It's unclear whether it's supported behavior for users to
      // create their own Reflection classes.
      assert(false);
    }

    if (upb_f->type() == UPB_TYPE_MESSAGE) {
      const goog::Message* prototype = GetPrototype(m, proto2_f);
      assert(prototype);
      const upb::Handlers* sub_handlers =
          GetMaybeUnfrozenWriteHandlers(upb_f->message_subdef(), *prototype);
      h->SetSubHandlers(upb_f, sub_handlers);
    }
  }

  return h;
}

const Handlers* CodeCache::GetWriteHandlers(const goog::Message& m) {
  const MessageDef* md = def_builder_.GetMessageDefExpandWeak(m);
  const Handlers* ret = GetMaybeUnfrozenWriteHandlers(md, m);
  upb::Status status;
  upb::Handlers::Freeze(to_freeze_, &status);
  ASSERT_STATUS(&status);
  to_freeze_.clear();
  return ret;
}

upb::reffed_ptr<const upb::Handlers> NewWriteHandlers(const goog::Message& m) {
  CodeCache cache;
  return upb::reffed_ptr<const upb::Handlers>(cache.GetWriteHandlers(m));
}

}  // namespace googlepb
}  // namespace upb
