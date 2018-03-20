#include <unordered_map>

#include "protobuf_hhvm.h"

// -----------------------------------------------------------------------------
// Class lookup util
// -----------------------------------------------------------------------------

static std::map<const Class*, const upb_def*>* class2def;
static std::map<const upb_def*, const Class*>* def2class;

static Class* lookup_class(const char* name) {
  StringData* name_php = StringData::Make(name, CopyStringMode::CopyString);
  Class* klass = Unit::loadClass(name_php);
  assert(klass != NULL);
  return klass;
}

void register_upbdef(const char* classname, const upb_def* def) {
  const Class* klass = lookup_class(classname);
  assert(klass != NULL);
  (*class2def)[klass] = def;
  (*def2class)[def] = klass;
}

const upb_msgdef* class2msgdef(const Class* klass) {
  const upb_def* def = (*class2def)[klass];
  assert(def->type == UPB_DEF_MSG);
  return upb_downcast_msgdef(def);
}

const Class* msgdef2class(const upb_msgdef* msgdef) {
  const Class* klass = (*def2class)[upb_msgdef_upcast(msgdef)];
  assert(klass != NULL);
  return klass;
}

// -----------------------------------------------------------------------------
// Extension setup
// -----------------------------------------------------------------------------

class ProtobufExtension : public Extension {
 public:
  ProtobufExtension(): Extension("protobuf", "1.0") {}

  void moduleInit() override {
    protobuf_module = new ProtobufModule();

    PROTO_INIT_CLASS(Arena);
    PROTO_INIT_CLASS(InternalDescriptorPool);
    PROTO_INIT_CLASS(MapField);
    PROTO_INIT_CLASS(MapFieldIter);
    PROTO_INIT_CLASS(Message);
    PROTO_INIT_CLASS(RepeatedField);
    PROTO_INIT_CLASS(RepeatedFieldIter);
    PROTO_INIT_CLASS(Util);

    loadSystemlib();
  }

  void requestInit() override {
    class2def = new std::map<const Class*, const upb_def*>();
    def2class = new std::map<const upb_def*, const Class*>();
    Class* internal_descriptor_pool_class =
        lookup_class("Google\\Protobuf\\Internal\\DescriptorPool");
    internal_generated_pool = Object(internal_descriptor_pool_class);
    internal_generated_pool_cpp = 
        Native::data<InternalDescriptorPool>(internal_generated_pool);
    message_factory = upb_msgfactory_new(
        internal_generated_pool_cpp->symtab);
  }

  void requestShutdown() override {
    delete class2def;
    delete def2class;
    upb_msgfactory_free(message_factory);
  }

 private:
 
} s_protobuf_extension;

HHVM_GET_MODULE(protobuf);

/////////////////////////////////////////////////////////////////////////////
