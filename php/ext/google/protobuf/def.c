#include "protobuf.h"

// -----------------------------------------------------------------------------
// Common Utilities
// -----------------------------------------------------------------------------

void check_upb_status(const upb_status* status, const char* msg) {
  if (!upb_ok(status)) {
    zend_error("%s: %s\n", msg, upb_status_errmsg(status));
  }
}


static upb_def *check_notfrozen(const upb_def *def) {
  if (upb_def_isfrozen(def)) {
    zend_error(E_ERROR,
               "Attempt to modify a frozen descriptor. Once descriptors are "
               "added to the descriptor pool, they may not be modified.");
  }
  return (upb_def *)def;
}

static upb_msgdef *check_msgdef_notfrozen(const upb_msgdef *def) {
  return upb_downcast_msgdef_mutable(check_notfrozen((const upb_def *)def));
}

static upb_fielddef *check_fielddef_notfrozen(const upb_fielddef *def) {
  return upb_downcast_fielddef_mutable(check_notfrozen((const upb_def *)def));
}

#define PROTOBUF_WRAP_INTERN(wrapper, intern, intern_dtor)            \
  Z_TYPE_P(wrapper) = IS_OBJECT;                                      \
  Z_OBJVAL_P(wrapper)                                                 \
      .handle = zend_objects_store_put(                               \
      intern, (zend_objects_store_dtor_t)zend_objects_destroy_object, \
      intern_dtor, NULL TSRMLS_CC);                                   \
  Z_OBJVAL_P(wrapper).handlers = zend_get_std_object_handlers();

#define PROTOBUF_SETUP_ZEND_WRAPPER(class_name, class_name_lower, wrapper,    \
                                    intern)                                   \
  Z_TYPE_P(wrapper) = IS_OBJECT;                                              \
  class_name *intern = ALLOC(class_name);                                     \
  memset(intern, 0, sizeof(class_name));                                      \
  class_name_lower##_init_c_instance(intern TSRMLS_CC);                       \
  Z_OBJVAL_P(wrapper)                                                         \
      .handle = zend_objects_store_put(intern, NULL, class_name_lower##_free, \
                                       NULL TSRMLS_CC);                       \
  Z_OBJVAL_P(wrapper).handlers = zend_get_std_object_handlers();

#define PROTOBUF_CREATE_ZEND_WRAPPER(class_name, class_name_lower, wrapper, \
                                     intern)                                \
  MAKE_STD_ZVAL(wrapper);                                                   \
  PROTOBUF_SETUP_ZEND_WRAPPER(class_name, class_name_lower, wrapper, intern);

#define DEFINE_CLASS(name, name_lower, string_name)                          \
  zend_class_entry *name_lower##_type;                                       \
  void name_lower##_init(TSRMLS_D) {                                         \
    zend_class_entry class_type;                                             \
    INIT_CLASS_ENTRY(class_type, string_name, name_lower##_methods);         \
    name_lower##_type = zend_register_internal_class(&class_type TSRMLS_CC); \
    name_lower##_type->create_object = name_lower##_create;                  \
  }                                                                          \
  name *php_to_##name_lower(zval *val TSRMLS_DC) {                           \
    return (name *)zend_object_store_get_object(val TSRMLS_CC);              \
  }                                                                          \
  void name_lower##_free(void *object TSRMLS_DC) {                           \
    name *intern = (name *)object;                                           \
    name_lower##_free_c(intern TSRMLS_CC);                                   \
    efree(object);                                                           \
  }                                                                          \
  zend_object_value name_lower##_create(zend_class_entry *ce TSRMLS_DC) {    \
    zend_object_value return_value;                                          \
    name *intern = (name *)emalloc(sizeof(name));                            \
    memset(intern, 0, sizeof(name));                                         \
    name_lower##_init_c_instance(intern TSRMLS_CC);                          \
    return_value.handle = zend_objects_store_put(                            \
        intern, (zend_objects_store_dtor_t)zend_objects_destroy_object,      \
        name_lower##_free, NULL TSRMLS_CC);                                  \
    return_value.handlers = zend_get_std_object_handlers();                  \
    return return_value;                                                     \
  }

// -----------------------------------------------------------------------------
// DescriptorPool
// -----------------------------------------------------------------------------

static zend_function_entry descriptor_pool_methods[] = {
  PHP_ME(DescriptorPool, addMessage, NULL, ZEND_ACC_PUBLIC)
  PHP_ME(DescriptorPool, finalize, NULL, ZEND_ACC_PUBLIC)
  ZEND_FE_END
};

DEFINE_CLASS(DescriptorPool, descriptor_pool,
             "Google\\Protobuf\\DescriptorPool");

DescriptorPool *generated_pool;  // The actual generated pool

ZEND_FUNCTION(get_generated_pool) {
  if (PROTOBUF_G(generated_pool) == NULL) {
    MAKE_STD_ZVAL(PROTOBUF_G(generated_pool));
    Z_TYPE_P(PROTOBUF_G(generated_pool)) = IS_OBJECT;
    generated_pool = ALLOC(DescriptorPool);
    descriptor_pool_init_c_instance(generated_pool TSRMLS_CC);
    Z_OBJ_HANDLE_P(PROTOBUF_G(generated_pool)) = zend_objects_store_put(
        generated_pool, NULL,
        (zend_objects_free_object_storage_t)descriptor_pool_free, NULL TSRMLS_CC);
    Z_OBJ_HT_P(PROTOBUF_G(generated_pool)) = zend_get_std_object_handlers();
  }
  RETURN_ZVAL(PROTOBUF_G(generated_pool), 1, 0);
}

void descriptor_pool_init_c_instance(DescriptorPool* pool TSRMLS_DC) {
  zend_object_std_init(&pool->std, descriptor_pool_type TSRMLS_CC);
  pool->symtab = upb_symtab_new(&pool->symtab);

  ALLOC_HASHTABLE(pool->pending_list);
  zend_hash_init(pool->pending_list, 1, NULL, ZVAL_PTR_DTOR, 0);
}

void descriptor_pool_free_c(DescriptorPool *pool TSRMLS_DC) {
  upb_symtab_unref(pool->symtab, &pool->symtab);
  zend_hash_destroy(pool->pending_list);
  FREE_HASHTABLE(pool->pending_list);
}

PHP_METHOD(DescriptorPool, addMessage) {
  char *name = NULL;
  int str_len;
  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &name, &str_len) ==
      FAILURE) {
    return;
  }

  zval* retval = NULL;
  PROTOBUF_CREATE_ZEND_WRAPPER(MessageBuilderContext, message_builder_context,
                               retval, context);

  MAKE_STD_ZVAL(context->pool);
  ZVAL_ZVAL(context->pool, getThis(), 1, 0);

  Descriptor *desc = php_to_descriptor(context->descriptor TSRMLS_CC);
  Descriptor_name_set(desc, name);

  RETURN_ZVAL(retval, 0, 1);
}

static void validate_msgdef(const upb_msgdef* msgdef) {
  // Verify that no required fields exist. proto3 does not support these.
  upb_msg_field_iter it;
  for (upb_msg_field_begin(&it, msgdef);
       !upb_msg_field_done(&it);
       upb_msg_field_next(&it)) {
    const upb_fielddef* field = upb_msg_iter_field(&it);
    if (upb_fielddef_label(field) == UPB_LABEL_REQUIRED) {
      zend_error(E_ERROR, "Required fields are unsupported in proto3.");
    }
  }
}

PHP_METHOD(DescriptorPool, finalize) {
  DescriptorPool *self = php_to_descriptor_pool(getThis() TSRMLS_CC);
  Bucket *temp;
  int i, num;

  num = zend_hash_num_elements(self->pending_list);
  upb_def **defs = emalloc(sizeof(upb_def *) * num);

  for (i = 0, temp = self->pending_list->pListHead; temp != NULL;
       temp = temp->pListNext) {
    zval *def_php = *(zval **)temp->pData;
    Descriptor* desc = php_to_descriptor(def_php TSRMLS_CC);
    defs[i] = (upb_def *)desc->msgdef;
    validate_msgdef((const upb_msgdef *)defs[i++]);
  }

  CHECK_UPB(upb_symtab_add(self->symtab, (upb_def **)defs, num, NULL, &status),
            "Unable to add defs to DescriptorPool");

  for (temp = self->pending_list->pListHead; temp != NULL;
       temp = temp->pListNext) {
    // zval *def_php = *(zval **)temp->pData;
    // Descriptor* desc = php_to_descriptor(def_php TSRMLS_CC);
    build_class_from_descriptor((zval *)temp->pDataPtr TSRMLS_CC);
  }

  FREE(defs);
  zend_hash_destroy(self->pending_list);
  zend_hash_init(self->pending_list, 1, NULL, ZVAL_PTR_DTOR, 0);
}

// -----------------------------------------------------------------------------
// Descriptor
// -----------------------------------------------------------------------------

static zend_function_entry descriptor_methods[] = {
  ZEND_FE_END
};

DEFINE_CLASS(Descriptor, descriptor, "Google\\Protobuf\\Descriptor");

void descriptor_free_c(Descriptor *self TSRMLS_DC) {
  upb_msg_field_iter iter;
  upb_msg_field_begin(&iter, self->msgdef);
  while (!upb_msg_field_done(&iter)) {
    upb_fielddef *fielddef = upb_msg_iter_field(&iter);
    upb_fielddef_unref(fielddef, &fielddef);
    upb_msg_field_next(&iter);
  }
  upb_msgdef_unref(self->msgdef, &self->msgdef);
  if (self->layout) {
    free_layout(self->layout);
  }
}

static void descriptor_add_field(Descriptor *desc,
                                 const upb_fielddef *fielddef) {
  upb_msgdef *mut_def = check_msgdef_notfrozen(desc->msgdef);
  upb_fielddef *mut_field_def = check_fielddef_notfrozen(fielddef);
  CHECK_UPB(upb_msgdef_addfield(mut_def, mut_field_def, NULL, &status),
            "Adding field to Descriptor failed");
  // add_def_obj(fielddef, obj);
}

void descriptor_init_c_instance(Descriptor* desc TSRMLS_DC) {
  zend_object_std_init(&desc->std, descriptor_type TSRMLS_CC);
  desc->msgdef = upb_msgdef_new(&desc->msgdef);
  desc->layout = NULL;
  // MAKE_STD_ZVAL(intern->klass);
  // ZVAL_NULL(intern->klass);
  desc->pb_serialize_handlers = NULL;
}

void Descriptor_name_set(Descriptor *desc, const char *name) {
  upb_msgdef *mut_def = check_msgdef_notfrozen(desc->msgdef);
  CHECK_UPB(upb_msgdef_setfullname(mut_def, name, &status),
            "Error setting Descriptor name");
}

// -----------------------------------------------------------------------------
// FieldDescriptor
// -----------------------------------------------------------------------------

static void field_descriptor_name_set(const upb_fielddef* fielddef,
                                      const char *name) {
  upb_fielddef *mut_def = check_fielddef_notfrozen(fielddef);
  CHECK_UPB(upb_fielddef_setname(mut_def, name, &status),
            "Error setting FieldDescriptor name");
}

static void field_descriptor_label_set(const upb_fielddef* fielddef,
                                       upb_label_t upb_label) {
  upb_fielddef *mut_def = check_fielddef_notfrozen(fielddef);
  upb_fielddef_setlabel(mut_def, upb_label);
}

upb_fieldtype_t string_to_descriptortype(const char *type) {
#define CONVERT(upb, str)   \
  if (!strcmp(type, str)) { \
    return UPB_DESCRIPTOR_TYPE_##upb;  \
  }

  CONVERT(FLOAT, "float");
  CONVERT(DOUBLE, "double");
  CONVERT(BOOL, "bool");
  CONVERT(STRING, "string");
  CONVERT(BYTES, "bytes");
  CONVERT(MESSAGE, "message");
  CONVERT(GROUP, "group");
  CONVERT(ENUM, "enum");
  CONVERT(INT32, "int32");
  CONVERT(INT64, "int64");
  CONVERT(UINT32, "uint32");
  CONVERT(UINT64, "uint64");
  CONVERT(SINT32, "sint32");
  CONVERT(SINT64, "sint64");
  CONVERT(FIXED32, "fixed32");
  CONVERT(FIXED64, "fixed64");
  CONVERT(SFIXED32, "sfixed32");
  CONVERT(SFIXED64, "sfixed64");

#undef CONVERT

  zend_error(E_ERROR, "Unknown field type.");
  return 0;
}

static void field_descriptor_type_set(const upb_fielddef* fielddef,
                                      const char *type) {
  upb_fielddef *mut_def = check_fielddef_notfrozen(fielddef);
  upb_fielddef_setdescriptortype(mut_def, string_to_descriptortype(type));
}

static void field_descriptor_number_set(const upb_fielddef* fielddef,
                                        int number) {
  upb_fielddef *mut_def = check_fielddef_notfrozen(fielddef);
  CHECK_UPB(upb_fielddef_setnumber(mut_def, number, &status),
            "Error setting field number");
}

// -----------------------------------------------------------------------------
// MessageBuilderContext
// -----------------------------------------------------------------------------

static zend_function_entry message_builder_context_methods[] = {
    PHP_ME(MessageBuilderContext, finalizeToPool, NULL, ZEND_ACC_PUBLIC)
    PHP_ME(MessageBuilderContext, optional, NULL, ZEND_ACC_PUBLIC)
    {NULL, NULL, NULL}
};

DEFINE_CLASS(MessageBuilderContext, message_builder_context,
             "Google\\Protobuf\\Internal\\MessageBuilderContext");

void message_builder_context_free_c(MessageBuilderContext *context TSRMLS_DC) {
  zval_ptr_dtor(&context->descriptor);
  zval_ptr_dtor(&context->pool);
}

void message_builder_context_init_c_instance(
    MessageBuilderContext *context TSRMLS_DC) {
  zend_object_std_init(&context->std, message_builder_context_type TSRMLS_CC);
  PROTOBUF_CREATE_ZEND_WRAPPER(Descriptor, descriptor, context->descriptor,
                               desc);
}

static void msgdef_add_field(Descriptor *desc, upb_label_t upb_label,
                             const char *name, const char *type, int number,
                             const char *type_class) {
  upb_fielddef *fielddef = upb_fielddef_new(&fielddef);
  upb_fielddef_setpacked(fielddef, false);

  field_descriptor_label_set(fielddef, upb_label);
  field_descriptor_name_set(fielddef, name);
  field_descriptor_type_set(fielddef, type);
  field_descriptor_number_set(fielddef, number);

// //   if (type_class != Qnil) {
// //     if (TYPE(type_class) != T_STRING) {
// //       rb_raise(rb_eArgError, "Expected string for type class");
// //     }
// //     // Make it an absolute type name by prepending a dot.
// //     type_class = rb_str_append(rb_str_new2("."), type_class);
// //     rb_funcall(fielddef, rb_intern("submsg_name="), 1, type_class);
// //   }
  descriptor_add_field(desc, fielddef);
}

PHP_METHOD(MessageBuilderContext, optional) {
  MessageBuilderContext *self = php_to_message_builder_context(getThis() TSRMLS_CC);
  Descriptor *desc = php_to_descriptor(self->descriptor TSRMLS_CC);
  // VALUE name, type, number, type_class;
  const char *name, *type, *type_class;
  int number, name_str_len, type_str_len, type_class_str_len;
  if (ZEND_NUM_ARGS() == 3) {
    if (zend_parse_parameters(3 TSRMLS_CC, "ssl", &name,
                              &name_str_len, &type, &type_str_len, &number) == FAILURE) {
      return;
    }
  } else {
    if (zend_parse_parameters(4 TSRMLS_CC, "ssls", &name,
                              &name_str_len, &type, &type_str_len, &number, &type_class,
                              &type_class_str_len) == FAILURE) {
      return;
    }
  }

  msgdef_add_field(desc, UPB_LABEL_OPTIONAL, name, type, number, type_class);

  zval_copy_ctor(getThis());
  RETURN_ZVAL(getThis(), 1, 0);
}

PHP_METHOD(MessageBuilderContext, finalizeToPool) {
  MessageBuilderContext *self = php_to_message_builder_context(getThis() TSRMLS_CC);
  DescriptorPool *pool = php_to_descriptor_pool(self->pool TSRMLS_CC);
  Descriptor* desc = php_to_descriptor(self->descriptor TSRMLS_CC);

  Z_ADDREF_P(self->descriptor);
  zend_hash_next_index_insert(pool->pending_list, &self->descriptor,
                              sizeof(zval *), NULL);
  RETURN_ZVAL(self->pool, 1, 0);
}
