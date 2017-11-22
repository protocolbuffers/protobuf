// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "protobuf.h"

// Forward declare.
static void internal_descriptor_pool_free_c(
    InternalDescriptorPool *object TSRMLS_DC);
static void internal_descriptor_pool_init_c_instance(
    InternalDescriptorPool *pool TSRMLS_DC);

// -----------------------------------------------------------------------------
// InternalDescriptorPool
// -----------------------------------------------------------------------------

static zend_function_entry internal_descriptor_pool_methods[] = {
  PHP_ME(InternalDescriptorPool, internalAddGeneratedFile, NULL, ZEND_ACC_PUBLIC)
  ZEND_FE_END
};

static void internal_descriptor_pool_init_c_instance(
    InternalDescriptorPool *pool TSRMLS_DC) {
  pool->symtab = upb_symtab_new();

  ALLOC_HASHTABLE(pool->pending_list);
  zend_hash_init(pool->pending_list, 1, NULL, ZVAL_PTR_DTOR, 0);
}

static void internal_descriptor_pool_free_c(
    InternalDescriptorPool *pool TSRMLS_DC) {
  upb_symtab_free(pool->symtab);

  zend_hash_destroy(pool->pending_list);
  FREE_HASHTABLE(pool->pending_list);
}

DEFINE_CLASS(InternalDescriptorPool, internal_descriptor_pool,
             "Google\\Protobuf\\Internal\\DescriptorPool");

// static void classname_no_prefix(const char *fullname, const char *package_name,
//                                 char *class_name) {
//   size_t i = 0, j;
//   bool first_char = true, is_reserved = false;
//   size_t pkg_name_len = package_name == NULL ? 0 : strlen(package_name);
//   size_t message_name_start = package_name == NULL ? 0 : pkg_name_len + 1;
//   size_t message_len = (strlen(fullname) - message_name_start);
// 
//   // Submessage is concatenated with its containing messages by '_'.
//   for (j = message_name_start; j < message_name_start + message_len; j++) {
//     if (fullname[j] == '.') {
//       class_name[i++] = '_';
//     } else {
//       class_name[i++] = fullname[j];
//     }
//   }
// }
// 
// static const char *classname_prefix(const char *classname,
//                                     const char *prefix_given,
//                                     const char *package_name) {
//   size_t i;
//   bool is_reserved = false;
// 
//   if (prefix_given != NULL && strcmp(prefix_given, "") != 0) {
//     return prefix_given;
//   }
// 
//   char* lower = ALLOC_N(char, strlen(classname) + 1);
//   i = 0;
//   while(classname[i]) {
//     lower[i] = (char)tolower(classname[i]);
//     i++;
//   }
//   lower[i] = 0;
// 
//   is_reserved = is_reserved_name(lower);
//   FREE(lower);
// 
//   if (is_reserved) {
//     if (package_name != NULL && strcmp("google.protobuf", package_name) == 0) {
//       return "GPB";
//     } else {
//       return "PB";
//     }
//   }
// 
//   return "";
// }
// 
// static void convert_to_class_name_inplace(const char *package,
//                                           const char *namespace_given,
//                                           const char *prefix, char *classname) {
//   size_t prefix_len = prefix == NULL ? 0 : strlen(prefix);
//   size_t classname_len = strlen(classname);
//   int i = 0, j;
//   bool first_char = true;
// 
//   size_t package_len = package == NULL ? 0 : strlen(package);
//   size_t namespace_given_len =
//       namespace_given == NULL ? 0 : strlen(namespace_given);
//   bool use_namespace_given = namespace_given != NULL;
//   size_t namespace_len =
//       use_namespace_given ? namespace_given_len : package_len;
// 
//   int offset = namespace_len != 0 ? 2 : 0;
// 
//   for (j = 0; j < classname_len; j++) {
//     classname[namespace_len + prefix_len + classname_len + offset - 1 - j] =
//         classname[classname_len - j - 1];
//   }
// 
//   if (namespace_len != 0) {
//     classname[i++] = '\\';
//     for (j = 0; j < namespace_len; j++) {
//       if (use_namespace_given) {
//         classname[i++] = namespace_given[j];
//         continue;
//       }
//       // php packages are divided by '\'.
//       if (package[j] == '.') {
//         classname[i++] = '\\';
//         first_char = true;
//       } else if (first_char) {
//         // PHP package uses camel case.
//         if (package[j] < 'A' || package[j] > 'Z') {
//           classname[i++] = package[j] + 'A' - 'a';
//         } else {
//           classname[i++] = package[j];
//         }
//         first_char = false;
//       } else {
//         classname[i++] = package[j];
//       }
//     }
//     classname[i++] = '\\';
//   }
// 
//   memcpy(classname + i, prefix, prefix_len);
// }

void internal_add_generated_file(const char *data, PHP_PROTO_SIZE data_len,
                                 InternalDescriptorPool *pool TSRMLS_DC) {
//   upb_filedef **files;
//   size_t i;
// 
//   CHECK_UPB(files = upb_loaddescriptor(data, data_len, &pool, &status),
//             "Parse binary descriptors to internal descriptors failed");
// 
//   // This method is called only once in each file.
//   assert(files[0] != NULL);
//   assert(files[1] == NULL);
// 
//   CHECK_UPB(upb_symtab_addfile(pool->symtab, files[0], &status),
//             "Unable to add file to DescriptorPool");
// 
//   // For each enum/message, we need its PHP class, upb descriptor and its PHP
//   // wrapper. These information are needed later for encoding, decoding and type
//   // checking. However, sometimes we just have one of them. In order to find
//   // them quickly, here, we store the mapping for them.
//   for (i = 0; i < upb_filedef_defcount(files[0]); i++) {
//     const upb_def *def = upb_filedef_def(files[0], i);
//     switch (upb_def_type(def)) {
// #define CASE_TYPE(def_type, def_type_lower, desc_type, desc_type_lower)        \
//   case UPB_DEF_##def_type: {                                                   \
//     CREATE_HASHTABLE_VALUE(desc, desc_php, desc_type, desc_type_lower##_type); \
//     const upb_##def_type_lower *def_type_lower =                               \
//         upb_downcast_##def_type_lower(def);                                    \
//     desc->def_type_lower = def_type_lower;                                     \
//     add_def_obj(desc->def_type_lower, desc_php);                               \
//     /* Unlike other messages, MapEntry is shared by all map fields and doesn't \
//      * have generated PHP class.*/                                             \
//     if (upb_def_type(def) == UPB_DEF_MSG &&                                    \
//         upb_msgdef_mapentry(upb_downcast_msgdef(def))) {                       \
//       break;                                                                   \
//     }                                                                          \
//     /* Prepend '.' to package name to make it absolute. In the 5 additional    \
//      * bytes allocated, one for '.', one for trailing 0, and 3 for 'GPB' if    \
//      * given message is google.protobuf.Empty.*/                               \
//     const char *fullname = upb_##def_type_lower##_fullname(def_type_lower);    \
//     const char *php_namespace = upb_filedef_phpnamespace(files[0]);            \
//     const char *prefix_given = upb_filedef_phpprefix(files[0]);                \
//     size_t classname_len = strlen(fullname) + 5;                               \
//     if (prefix_given != NULL) {                                                \
//       classname_len += strlen(prefix_given);                                   \
//     }                                                                          \
//     if (php_namespace != NULL) {                                               \
//       classname_len += strlen(php_namespace);                                  \
//     }                                                                          \
//     char *classname = ecalloc(sizeof(char), classname_len);                    \
//     const char *package = upb_filedef_package(files[0]);                       \
//     classname_no_prefix(fullname, package, classname);                         \
//     const char *prefix = classname_prefix(classname, prefix_given, package);   \
//     convert_to_class_name_inplace(package, php_namespace, prefix, classname);  \
//     PHP_PROTO_CE_DECLARE pce;                                                  \
//     if (php_proto_zend_lookup_class(classname, strlen(classname), &pce) ==     \
//         FAILURE) {                                                             \
//       zend_error(E_ERROR, "Generated message class %s hasn't been defined",    \
//                  classname);                                                   \
//       return;                                                                  \
//     } else {                                                                   \
//       desc->klass = PHP_PROTO_CE_UNREF(pce);                                   \
//     }                                                                          \
//     add_ce_obj(desc->klass, desc_php);                                         \
//     add_proto_obj(upb_##def_type_lower##_fullname(desc->def_type_lower),       \
//                   desc_php);                                                   \
//     efree(classname);                                                          \
//     break;                                                                     \
//   }
// 
//       CASE_TYPE(MSG, msgdef, Descriptor, descriptor)
//       CASE_TYPE(ENUM, enumdef, EnumDescriptor, enum_descriptor)
// #undef CASE_TYPE
// 
//       default:
//         break;
//     }
//   }
// 
//   for (i = 0; i < upb_filedef_defcount(files[0]); i++) {
//     const upb_def *def = upb_filedef_def(files[0], i);
//     if (upb_def_type(def) == UPB_DEF_MSG) {
//       const upb_msgdef *msgdef = upb_downcast_msgdef(def);
//       PHP_PROTO_HASHTABLE_VALUE desc_php = get_def_obj(msgdef);
//       build_class_from_descriptor(desc_php TSRMLS_CC);
//     }
//   }
// 
//   upb_filedef_unref(files[0], &pool);
//   upb_gfree(files);
}

PHP_METHOD(InternalDescriptorPool, internalAddGeneratedFile) {
  char *data = NULL;
  PHP_PROTO_SIZE data_len;
  upb_filedef **files;
  size_t i;

  if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &data, &data_len) ==
      FAILURE) {
    return;
  }

  InternalDescriptorPool *pool = UNBOX(InternalDescriptorPool, getThis());
  internal_add_generated_file(data, data_len, pool TSRMLS_CC);
  RETURN_BOOL(true);
}
