//
// upb - a minimalist implementation of protocol buffers.
//
// Copyright (c) 2011-2012 Google Inc.  See LICENSE for details.
// Author: Josh Haberman <jhaberman@gmail.com>
//
// IMPORTANT NOTE!  This file is compiled TWICE, once with UPB_GOOGLE3 defined
// and once without!  This allows us to provide functionality against proto2
// and protobuf opensource both in a single binary without the two conflicting.
// However we must be careful not to violate the ODR.

#include "upb/google/bridge.h"

#include <map>
#include <string>
#include "upb/def.h"
#include "upb/google/proto1.h"
#include "upb/google/proto2.h"
#include "upb/handlers.h"

namespace upb {
namespace proto2_bridge_google3 { class Defs; }
namespace proto2_bridge_opensource { class Defs; }
}  // namespace upb

#ifdef UPB_GOOGLE3
#include "net/proto2/public/descriptor.h"
#include "net/proto2/public/message.h"
#include "net/proto2/proto/descriptor.pb.h"
namespace goog = ::proto2;
namespace me = ::upb::proto2_bridge_google3;
#else
#include "google/protobuf/descriptor.h"
#include "google/protobuf/message.h"
#include "google/protobuf/descriptor.pb.h"
namespace goog = ::google::protobuf;
namespace me = ::upb::proto2_bridge_opensource;
#endif

class me::Defs {
 public:
  void OnMessage(Handlers* h) {
    const upb::MessageDef* md = h->message_def();
    const goog::Message& m = *message_map_[md];
    const goog::Descriptor* d = m.GetDescriptor();
    for (upb::MessageDef::ConstIterator i(md); !i.Done(); i.Next()) {
      const upb::FieldDef* upb_f = i.field();
      const goog::FieldDescriptor* proto2_f =
          d->FindFieldByNumber(upb_f->number());
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
    }
  }

  static void StaticOnMessage(void *closure, upb::Handlers* handlers) {
    me::Defs* defs = static_cast<me::Defs*>(closure);
    defs->OnMessage(handlers);
  }

  void AddSymbol(const std::string& name, upb::Def* def) {
    assert(symbol_map_.find(name) == symbol_map_.end());
    symbol_map_[name] = def;
  }

  void AddMessage(const goog::Message* m, upb::MessageDef* md) {
    assert(message_map_.find(md) == message_map_.end());
    message_map_[md] = m;
    AddSymbol(m->GetDescriptor()->full_name(), md->Upcast());
  }

  upb::Def* FindSymbol(const std::string& name) {
    SymbolMap::iterator iter = symbol_map_.find(name);
    return iter != symbol_map_.end() ? iter->second : NULL;
  }

  void Flatten(std::vector<upb::Def*>* defs) {
    SymbolMap::iterator iter;
    for (iter = symbol_map_.begin(); iter != symbol_map_.end(); ++iter) {
      defs->push_back(iter->second);
    }
  }

 private:
  // Maps a new upb::MessageDef* to a corresponding proto2 Message* whose
  // derived class is of the correct type according to the message the user
  // gave us.
  typedef std::map<const upb::MessageDef*, const goog::Message*> MessageMap;
  MessageMap message_map_;

  // Maps a type name to a upb Def we have constructed to represent it.
  typedef std::map<std::string, upb::Def*> SymbolMap;
  SymbolMap symbol_map_;
};

namespace upb {
namespace google {

// For submessage fields, stores a pointer to an instance of the submessage in
// *subm (but it is *not* guaranteed to be a prototype).
FieldDef* AddFieldDef(const goog::Message& m, const goog::FieldDescriptor* f,
                      upb::MessageDef* md, const goog::Message** subm) {
  // To parse weak submessages effectively, we need to represent them in the
  // upb::Def schema even though they are not reflected in the proto2
  // descriptors (weak fields are represented as FieldDescriptor::TYPE_BYTES).
  const goog::Message* weak_prototype = NULL;
#ifdef UPB_GOOGLE3
  weak_prototype = upb::google::GetProto1WeakPrototype(m, f);
#endif

  upb::FieldDef* upb_f = upb::FieldDef::New(&upb_f);
  upb_f->set_number(f->number());
  upb_f->set_name(f->name());
  upb_f->set_label(static_cast<upb::FieldDef::Label>(f->label()));
  upb_f->set_type(weak_prototype ?
      UPB_TYPE_MESSAGE : static_cast<upb::FieldDef::Type>(f->type()));

  if (weak_prototype) {
    upb_f->set_subdef_name(weak_prototype->GetDescriptor()->full_name());
  } else if (upb_f->IsSubMessage()) {
    upb_f->set_subdef_name(f->message_type()->full_name());
  } else if (upb_f->type() == UPB_TYPE(ENUM)) {
    // We set the enum default numerically.
    upb_f->set_default_value(
        MakeValue(static_cast<int32_t>(f->default_value_enum()->number())));
    upb_f->set_subdef_name(f->enum_type()->full_name());
  } else {
    // Set field default for primitive types.  Need to switch on the upb type
    // rather than the proto2 type, because upb_f->type() may have been changed
    // from BYTES to MESSAGE for a weak field.
    switch (upb_types[upb_f->type()].inmemory_type) {
      case UPB_CTYPE_INT32:
        upb_f->set_default_value(MakeValue(f->default_value_int32()));
        break;
      case UPB_CTYPE_INT64:
        upb_f->set_default_value(
            MakeValue(static_cast<int64_t>(f->default_value_int64())));
        break;
      case UPB_CTYPE_UINT32:
        upb_f->set_default_value(MakeValue(f->default_value_uint32()));
        break;
      case UPB_CTYPE_UINT64:
        upb_f->set_default_value(
            MakeValue(static_cast<uint64_t>(f->default_value_uint64())));
        break;
      case UPB_CTYPE_DOUBLE:
        upb_f->set_default_value(MakeValue(f->default_value_double()));
        break;
      case UPB_CTYPE_FLOAT:
        upb_f->set_default_value(MakeValue(f->default_value_float()));
        break;
      case UPB_CTYPE_BOOL:
        upb_f->set_default_value(MakeValue(f->default_value_bool()));
        break;
      case UPB_CTYPE_BYTEREGION:
        upb_f->set_default_string(f->default_value_string());
        break;
    }
  }
  bool ok = md->AddField(upb_f, &upb_f);
  UPB_ASSERT_VAR(ok, ok);

  if (weak_prototype) {
    *subm = weak_prototype;
  } else if (f->cpp_type() == goog::FieldDescriptor::CPPTYPE_MESSAGE) {
    *subm = upb::google::GetFieldPrototype(m, f);
#ifdef UPB_GOOGLE3
    if (!*subm)
      *subm = upb::google::GetProto1FieldPrototype(m, f);
#endif
    assert(*subm);
  }

  return upb_f;
}

upb::EnumDef* NewEnumDef(const goog::EnumDescriptor* desc, void *owner) {
  upb::EnumDef* e = upb::EnumDef::New(owner);
  e->set_full_name(desc->full_name());
  for (int i = 0; i < desc->value_count(); i++) {
    const goog::EnumValueDescriptor* val = desc->value(i);
    bool success = e->AddValue(val->name(), val->number(), NULL);
    UPB_ASSERT_VAR(success, success);
  }
  return e;
}

static upb::MessageDef* NewMessageDef(const goog::Message& m, void *owner,
                                      me::Defs* defs) {
  upb::MessageDef* md = upb::MessageDef::New(owner);
  md->set_full_name(m.GetDescriptor()->full_name());

  // Must do this before processing submessages to prevent infinite recursion.
  defs->AddMessage(&m, md);

  const goog::Descriptor* d = m.GetDescriptor();
  for (int i = 0; i < d->field_count(); i++) {
    const goog::FieldDescriptor* proto2_f = d->field(i);

#ifdef UPB_GOOGLE3
    // Skip lazy fields for now since we can't properly handle them.
    if (proto2_f->options().lazy()) continue;
#endif
    // Extensions not supported yet.
    if (proto2_f->is_extension()) continue;

    const goog::Message* subm_prototype;
    upb::FieldDef* f = AddFieldDef(m, proto2_f, md, &subm_prototype);

    if (!f->HasSubDef()) continue;

    upb::Def* subdef = defs->FindSymbol(f->subdef_name());
    if (!subdef) {
      if (f->type() == UPB_TYPE(ENUM)) {
        subdef = NewEnumDef(proto2_f->enum_type(), owner)->Upcast();
        defs->AddSymbol(subdef->full_name(), subdef);
      } else {
        assert(f->IsSubMessage());
        assert(subm_prototype);
        subdef = NewMessageDef(*subm_prototype, owner, defs)->Upcast();
      }
    }
    f->set_subdef(subdef);
  }

  return md;
}

const upb::Handlers* NewWriteHandlers(const goog::Message& m, void *owner) {
  me::Defs defs;
  const upb::MessageDef* md = NewMessageDef(m, owner, &defs);

  std::vector<upb::Def*> defs_vec;
  defs.Flatten(&defs_vec);
  Status status;
  bool success = Def::Freeze(defs_vec, &status);
  UPB_ASSERT_VAR(success, success);

  const upb::Handlers* ret =
      upb::Handlers::NewFrozen(md, owner, me::Defs::StaticOnMessage, &defs);

  // Unref all defs, since they're now ref'd by the handlers.
  for (int i = 0; i < static_cast<int>(defs_vec.size()); i++) {
    defs_vec[i]->Unref(owner);
  }

  return ret;
}

}  // namespace google
}  // namespace upb
