#include <string>

#include "protobuf_cpp.h"
#include "upb.h"

using std::string;

// -----------------------------------------------------------------------------
// Define global methods
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// Define static methods
// -----------------------------------------------------------------------------

static bool is_reserved_name(const char* name) {
  return protobuf_module->IsReservedName(name);
}

static void classname_no_prefix(const char *fullname, const char *package_name,
                                char *class_name) {
  size_t i = 0, j;
  bool first_char = true, is_reserved = false;
  size_t pkg_name_len = package_name == NULL ? 0 : strlen(package_name);
  size_t message_name_start = package_name == NULL ? 0 : pkg_name_len + 1;
  size_t message_len = (strlen(fullname) - message_name_start);

  // Submessage is concatenated with its containing messages by '_'.
  for (j = message_name_start; j < message_name_start + message_len; j++) {
    if (fullname[j] == '.') {
      class_name[i++] = '_';
    } else {
      class_name[i++] = fullname[j];
    }
  }
}

static const char *classname_prefix(const char *classname,
                                    const char *prefix_given,
                                    const char *package_name) {
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

  return "";
}

static void convert_to_class_name_inplace(const char *package,
                                          const char *namespace_given,
                                          const char *prefix, char *classname) {
  size_t prefix_len = prefix == NULL ? 0 : strlen(prefix);
  size_t classname_len = strlen(classname);
  int i = 0, j;
  bool first_char = true;

  size_t package_len = package == NULL ? 0 : strlen(package);
  size_t namespace_given_len =
      namespace_given == NULL ? 0 : strlen(namespace_given);
  bool use_namespace_given = namespace_given != NULL;
  size_t namespace_len =
      use_namespace_given ? namespace_given_len : package_len;

  int offset = namespace_len != 0 ? 2 : 0;

  for (j = 0; j < classname_len; j++) {
    classname[namespace_len + prefix_len + classname_len + offset - 1 - j] =
        classname[classname_len - j - 1];
  }

  if (namespace_len != 0) {
    classname[i++] = '\\';
    for (j = 0; j < namespace_len; j++) {
      if (use_namespace_given) {
        classname[i++] = namespace_given[j];
        continue;
      }
      // php packages are divided by '\'.
      if (package[j] == '.') {
        classname[i++] = '\\';
        first_char = true;
      } else if (first_char) {
        // PHP package uses camel case.
        if (package[j] < 'A' || package[j] > 'Z') {
          classname[i++] = package[j] + 'A' - 'a';
        } else {
          classname[i++] = package[j];
        }
        first_char = false;
      } else {
        classname[i++] = package[j];
      }
    }
    classname[i++] = '\\';
  }

  memcpy(classname + i, prefix, prefix_len);
}

// -----------------------------------------------------------------------------
// ProtobufModule
// -----------------------------------------------------------------------------

ProtobufModule* protobuf_module;

static const char * kReservedNames[] = {
    "abstract",   "and",        "array",        "as",           "break",
    "callable",   "case",       "catch",        "class",        "clone",
    "const",      "continue",   "declare",      "default",      "die",
    "do",         "echo",       "else",         "elseif",       "empty",
    "enddeclare", "endfor",     "endforeach",   "endif",        "endswitch",
    "endwhile",   "eval",       "exit",         "extends",      "final",
    "for",        "foreach",    "function",     "global",       "goto",
    "if",         "implements", "include",      "include_once", "instanceof",
    "insteadof",  "interface",  "isset",        "list",         "namespace",
    "new",        "or",         "print",        "private",      "protected",
    "public",     "require",    "require_once", "return",       "static",
    "switch",     "throw",      "trait",        "try",          "unset",
    "use",        "var",        "while",        "xor",          "int",
    "float",      "bool",       "string",       "true",         "false",
    "null",       "void",       "iterable"
};

ProtobufModule::ProtobufModule() {
  int kReservedNamesSize = 73;
  for (int i = 0; i < kReservedNamesSize; i++) {
//     printf("%s\n", kReservedNames[0]);
//     this->reserved_names.insert(kReservedNames[i]);
  }
}

bool ProtobufModule::IsReservedName(const string& name) {
  // return this->reserved_names.find(name) != this->reserved_names.end();
  return false;
}

// -----------------------------------------------------------------------------
// InternalDescriptorPool
// -----------------------------------------------------------------------------

InternalDescriptorPool* internal_generated_pool_cpp;
upb_msgfactory* message_factory;

void InternalDescriptorPool_init_c_instance(
    InternalDescriptorPool *pool TSRMLS_DC) {
  pool->symtab = upb_symtab_new();
}

void InternalDescriptorPool_free_c(
    InternalDescriptorPool *pool TSRMLS_DC) {
  upb_symtab_free(pool->symtab);
}

void InternalDescriptorPool_add_generated_file(
    InternalDescriptorPool* pool, const char* data, int data_len) {
  upb_filedef **files;
  size_t i;

  CHECK_UPB(files = upb_loaddescriptor(data, data_len, &pool, &status),
            "Parse binary descriptors to internal descriptors failed");

  // This method is called only once in each file.
  assert(files[0] != NULL);
  assert(files[1] == NULL);

  CHECK_UPB(upb_symtab_addfile(pool->symtab, files[0], &status),
            "Unable to add file to DescriptorPool");

  // For each enum/message, we need its PHP class, upb descriptor and its PHP
  // wrapper. These information are needed later for encoding, decoding and type
  // checking. However, sometimes we just have one of them. In order to find
  // them quickly, here, we store the mapping for them.
  for (i = 0; i < upb_filedef_defcount(files[0]); i++) {
    const upb_def *def = upb_filedef_def(files[0], i);
    switch (upb_def_type(def)) {
      case UPB_DEF_MSG: {
        const upb_msgdef *msgdef = upb_downcast_msgdef(def);
        const char *fullname = upb_msgdef_fullname(msgdef);
        const char *php_namespace = upb_filedef_phpnamespace(files[0]);
        const char *prefix_given = upb_filedef_phpprefix(files[0]);
        size_t classname_len = strlen(fullname) + 5;
        if (prefix_given != NULL) {
          classname_len += strlen(prefix_given);
        }
        if (php_namespace != NULL) {
          classname_len += strlen(php_namespace);
        }
        char *classname = ALLOC_N(char, classname_len);
        memset(classname, 0, classname_len);
        const char *package = upb_filedef_package(files[0]);
        classname_no_prefix(fullname, package, classname);
        const char *prefix = classname_prefix(classname, prefix_given, package);
        convert_to_class_name_inplace(package, php_namespace, prefix, classname);
        register_upbdef(classname, def);
        FREE(classname);
        break;
      }
// #define CASE_TYPE(def_type, def_type_lower, desc_type, desc_type_lower)        \
//   case UPB_DEF_##def_type: {                                                   \
//     const upb_##def_type_lower *def_type_lower =                               \
//         upb_downcast_##def_type_lower(def);                                    \
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
//     char *classname = ALLOC_N(char, classname_len);                            \
//     const char *package = upb_filedef_package(files[0]);                       \
//     classname_no_prefix(fullname, package, classname);                         \
//     const char *prefix = classname_prefix(classname, prefix_given, package);   \
//     convert_to_class_name_inplace(package, php_namespace, prefix, classname);  \
//     PROTO_CLASS* klass = lookup_class(classname);                              \
//     add_class2def(klass, def);                                                 \
//     FREE(classname);                                                           \
//     break;                                                                     \
//   }
// 
//       CASE_TYPE(MSG, msgdef, Descriptor, descriptor)
//       CASE_TYPE(ENUM, enumdef, EnumDescriptor, enum_descriptor)
// #undef CASE_TYPE
// 
      default:
        break;
    }
  }
// 
//   for (i = 0; i < upb_filedef_defcount(files[0]); i++) {
//     const upb_def *def = upb_filedef_def(files[0], i);
//     if (upb_def_type(def) == UPB_DEF_MSG) {
//       const upb_msgdef *msgdef = upb_downcast_msgdef(def);
// //       PHP_PROTO_HASHTABLE_VALUE desc_php = get_def_obj(msgdef);
// //       build_class_from_descriptor(desc_php TSRMLS_CC);
//     }
//   }
// 
//   upb_msgfactory_free(factory);
//   upb_filedef_unref(files[0], &pool);
  upb_filedef_unref(files[0], &pool);
  upb_gfree(files);
}
