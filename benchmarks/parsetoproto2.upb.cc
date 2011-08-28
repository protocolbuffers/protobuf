// This file is a crime against software engineering.  It breaks the
// encapsulation of proto2 in numerous ways, violates the C++ standard
// in others, and generally deserves to have comtempt and scorn heaped
// upon it.
//
// Its purpose is to get an accurate benchmark for how fast upb can
// parse into proto2 data structures.  To add proper support for this
// functionality, proto2 would need to expose actual support for the
// operations we are trying to perform here.

#define __STDC_LIMIT_MACROS 1
#include "main.c"

#include <stdint.h>
#include "upb/bytestream.h"
#include "upb/def.h"
#include "upb/msg.h"
#include "upb/pb/decoder.h"
#include "upb/pb/glue.h"

// Need to violate the encapsulation of GeneratedMessageReflection -- see below.
#define private public
#include MESSAGE_HFILE
#include <google/protobuf/descriptor.h>
#undef private

static size_t len;
MESSAGE_CIDENT msg[NUM_MESSAGES];
MESSAGE_CIDENT msg2;
static upb_stringsrc strsrc;
static upb_decoder d;
upb_msgdef *def;

#define PROTO2_APPEND(type, ctype) \
  upb_flow_t proto2_append_ ## type(void *_r, upb_value fval, upb_value val) { \
    (void)fval; \
    typedef google::protobuf::RepeatedField<ctype> R; \
    R *r = (R*)_r; \
    r->Add(upb_value_get ## type(val)); \
    return UPB_CONTINUE; \
  }

PROTO2_APPEND(double, double)
PROTO2_APPEND(float, float)
PROTO2_APPEND(uint64, uint64_t)
PROTO2_APPEND(int64, int64_t)
PROTO2_APPEND(int32, int32_t)
PROTO2_APPEND(uint32, uint32_t)
PROTO2_APPEND(bool, bool)

upb_flow_t proto2_setstr(void *m, upb_value fval, upb_value val) {
  assert(m != NULL);
  upb_fielddef *f = upb_value_getfielddef(fval);
  std::string **str = (std::string**)UPB_INDEX(m, f->offset, 1);
  if (*str == f->default_ptr) *str = new std::string;
  upb_strref *ref = upb_value_getstrref(val);
  // XXX: only supports contiguous strings atm.
  (*str)->assign(ref->ptr, ref->len);
  return UPB_CONTINUE;
}

upb_flow_t proto2_append_str(void *_r, upb_value fval, upb_value val) {
  assert(_r != NULL);
  typedef google::protobuf::RepeatedPtrField<std::string> R;
  (void)fval;
  R *r = (R*)_r;
  upb_strref *ref = upb_value_getstrref(val);
  // XXX: only supports contiguous strings atm.
  r->Add()->assign(ref->ptr, ref->len);
  return UPB_CONTINUE;
}

upb_sflow_t proto2_startseq(void *m, upb_value fval) {
  assert(m != NULL);
  upb_fielddef *f = upb_value_getfielddef(fval);
  return UPB_CONTINUE_WITH(UPB_INDEX(m, f->offset, 1));
}

upb_sflow_t proto2_startsubmsg(void *m, upb_value fval) {
  assert(m != NULL);
  upb_fielddef *f = upb_value_getfielddef(fval);
  google::protobuf::Message *prototype = (google::protobuf::Message*)f->prototype;
  void **subm = (void**)UPB_INDEX(m, f->offset, 1);
  if (*subm == NULL || *subm == f->default_ptr)
    *subm = prototype->New();
  assert(*subm != NULL);
  return UPB_CONTINUE_WITH(*subm);
}

class UpbRepeatedPtrField : public google::protobuf::internal::RepeatedPtrFieldBase {
 public:
  class TypeHandler {
   public:
    typedef void Type;
    // AddAllocated() calls this, but only if other objects are sitting
    // around waiting for reuse, which we will not do.
    static void Delete(Type*) { assert(false); }
  };
  void *Add(google::protobuf::Message *m) {
    void *submsg = RepeatedPtrFieldBase::AddFromCleared<TypeHandler>();
    if (!submsg) {
      submsg = m->New();
      RepeatedPtrFieldBase::AddAllocated<TypeHandler>(submsg);
    }
    return submsg;
  }
};

upb_sflow_t proto2_startsubmsg_r(void *_r, upb_value fval) {
  assert(_r != NULL);
  // Compared to the other writers, this implementation is particularly sketchy.
  // The object we are modifying is a RepeatedPtrField<SubType>*, but we can't
  // properly declare that templated pointer because we don't have access to
  // that type at compile-time (and wouldn't want to create a separate callback
  // for each type anyway).  Instead we access the pointer as a
  // RepeatedPtrFieldBase, which is indeed a superclass of RepeatedPtrField.
  // But we can't properly declare a TypeHandler for the submessage's type,
  // for the same reason that we can't create a RepeatedPtrField<SubType>*.
  // Instead we treat it as a void*, and create the submessage using
  // google::protobuf::Message::New() if we need to.
  class TypeHandler {
   public:
    typedef void Type;
  };
  upb_fielddef *f = upb_value_getfielddef(fval);
  UpbRepeatedPtrField *r = (UpbRepeatedPtrField*)_r;
  void *submsg = r->Add((google::protobuf::Message*)f->prototype);
  assert(submsg != NULL);
  return UPB_CONTINUE_WITH(submsg);
}

#define PROTO2MSG(type, size) { static upb_accessor_vtbl vtbl = { \
    &proto2_startsubmsg, \
    &upb_stdmsg_set ## type, \
    &proto2_startseq, \
    &proto2_startsubmsg_r, \
    &proto2_append_ ## type, \
    NULL, NULL, NULL, NULL, NULL, NULL}; \
  return &vtbl; }

static upb_accessor_vtbl *proto2_accessor(upb_fielddef *f) {
  switch (f->type) {
    case UPB_TYPE(DOUBLE): PROTO2MSG(double, 8)
    case UPB_TYPE(FLOAT): PROTO2MSG(float, 4)
    case UPB_TYPE(UINT64):
    case UPB_TYPE(FIXED64): PROTO2MSG(uint64, 8)
    case UPB_TYPE(INT64):
    case UPB_TYPE(SFIXED64):
    case UPB_TYPE(SINT64): PROTO2MSG(int64, 8)
    case UPB_TYPE(INT32):
    case UPB_TYPE(SINT32):
    case UPB_TYPE(ENUM):
    case UPB_TYPE(SFIXED32): PROTO2MSG(int32, 4)
    case UPB_TYPE(UINT32):
    case UPB_TYPE(FIXED32): PROTO2MSG(uint32, 4)
    case UPB_TYPE(BOOL): PROTO2MSG(bool, 1)
    case UPB_TYPE(STRING):
    case UPB_TYPE(BYTES):
    case UPB_TYPE(GROUP):
    case UPB_TYPE(MESSAGE): {
        static upb_accessor_vtbl vtbl = {
        &proto2_startsubmsg,
        &proto2_setstr,
        &proto2_startseq,
        &proto2_startsubmsg_r,
        &proto2_append_str,
        NULL, NULL, NULL, NULL, NULL, NULL};
        return &vtbl;
    }
  }
  return NULL;
}

static void layout_msgdef_from_proto2(upb_msgdef *upb_md,
                                      const google::protobuf::Message *m,
                                      const google::protobuf::Descriptor *proto2_d) {
  // Hack: we break the encapsulation of GeneratedMessageReflection to get at
  // the offsets we need.  If/when we do this for real, we will need
  // GeneratedMessageReflection to expose those offsets publicly.
  const google::protobuf::internal::GeneratedMessageReflection *r =
      (google::protobuf::internal::GeneratedMessageReflection*)m->GetReflection();
  for (int i = 0; i < proto2_d->field_count(); i++) {
    const google::protobuf::FieldDescriptor *proto2_f = proto2_d->field(i);
    upb_fielddef *upb_f = upb_msgdef_itof(upb_md, proto2_f->number());
    assert(upb_f);

    // Encapsulation violation BEGIN
    uint32_t data_offset = r->offsets_[proto2_f->index()];
    uint32_t hasbit = (r->has_bits_offset_ * 8) + proto2_f->index();
    // Encapsulation violation END

    if (upb_isseq(upb_f)) {
      // proto2 does not store hasbits for repeated fields.
      upb_f->hasbit = -1;
    } else {
      upb_f->hasbit = hasbit;
    }
    upb_f->offset = data_offset;
    upb_fielddef_setaccessor(upb_f, proto2_accessor(upb_f));

    if (upb_isstring(upb_f) && !upb_isseq(upb_f)) {
      upb_f->default_ptr = &r->GetStringReference(*m, proto2_f, NULL);
    } else if (upb_issubmsg(upb_f)) {
      // XXX: skip leading "."
      const google::protobuf::Descriptor *subm_descriptor =
          google::protobuf::DescriptorPool::generated_pool()->
              FindMessageTypeByName(upb_fielddef_typename(upb_f) + 1);
      assert(subm_descriptor);
      upb_f->prototype = google::protobuf::MessageFactory::generated_factory()->GetPrototype(subm_descriptor);
      if (!upb_isseq(upb_f))
        upb_f->default_ptr = &r->GetMessage(*m, proto2_f);
    }
  }
}

static bool initialize()
{
  // Initialize upb state, decode descriptor.
  upb_status status = UPB_STATUS_INIT;
  upb_symtab *s = upb_symtab_new();

  char *data = upb_readfile(MESSAGE_DESCRIPTOR_FILE, &len);
  if (!data) {
    fprintf(stderr, "Couldn't read file: " MESSAGE_DESCRIPTOR_FILE);
    return false;
  }
  int n;
  upb_def **defs = upb_load_descriptor(data, len, &n, &status);
  free(data);
  if(!upb_ok(&status)) {
    fprintf(stderr, "Error reading descriptor: %s\n",
            upb_status_getstr(&status));
    return false;
  }

  // Setup offsets and accessors to properly write into a proto2 generated
  // class.
  for (int i = 0; i < n; i++) {
    upb_def *def = defs[i];
    upb_msgdef *upb_md = upb_dyncast_msgdef(def);
    if (!upb_md) continue;
    const google::protobuf::Descriptor *proto2_md =
        google::protobuf::DescriptorPool::generated_pool()->
            FindMessageTypeByName(upb_def_fqname(def));
    if (!proto2_md) abort();
    const google::protobuf::Message *proto2_m =
        google::protobuf::MessageFactory::generated_factory()->GetPrototype(proto2_md);
    layout_msgdef_from_proto2(upb_md, proto2_m, proto2_md);
  }

  upb_symtab_add(s, defs, n, &status);
  if(!upb_ok(&status)) {
    fprintf(stderr, "Error reading adding to symtab: %s\n",
            upb_status_getstr(&status));
    return false;
  }
  for(int i = 0; i < n; i++) upb_def_unref(defs[i]);
  free(defs);

  def = upb_dyncast_msgdef(upb_symtab_lookup(s, MESSAGE_NAME));
  if(!def) {
    fprintf(stderr, "Error finding symbol '%s'.\n", MESSAGE_NAME);
    return false;
  }
  upb_symtab_unref(s);

  // Read the message data itself.
  char *str = upb_readfile(MESSAGE_FILE, &len);
  if(str == NULL) {
    fprintf(stderr, "Error reading " MESSAGE_FILE "\n");
    return false;
  }
  upb_status_uninit(&status);

  msg2.ParseFromArray(str, len);

  upb_stringsrc_init(&strsrc);
  upb_stringsrc_reset(&strsrc, str, len);
  upb_handlers *h = upb_handlers_new();
  upb_accessors_reghandlers(h, def);
  if (!JIT) h->should_jit = false;
  upb_decoder_initforhandlers(&d, h);
  upb_handlers_unref(h);

  return true;
}

static void cleanup() {
  upb_stringsrc_uninit(&strsrc);
  upb_decoder_uninit(&d);
  upb_def_unref(UPB_UPCAST(def));
}

static size_t run(int i)
{
  (void)i;
  upb_status status = UPB_STATUS_INIT;
  msg[i % NUM_MESSAGES].Clear();
  upb_decoder_reset(&d, upb_stringsrc_bytesrc(&strsrc),
                    0, UPB_NONDELIMITED, &msg[i % NUM_MESSAGES]);
  upb_decoder_decode(&d, &status);
  if(!upb_ok(&status)) goto err;
  return len;

err:
  fprintf(stderr, "Decode error: %s", upb_status_getstr(&status));
  return 0;
}
