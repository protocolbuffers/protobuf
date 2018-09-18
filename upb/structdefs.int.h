/*
** This file contains definitions of structs that should be considered private
** and NOT stable across versions of upb.
**
** The only reason they are declared here and not in .c files is to allow upb
** and the application (if desired) to embed statically-initialized instances
** of structures like defs.
**
** If you include this file, all guarantees of ABI compatibility go out the
** window!  Any code that includes this file needs to recompile against the
** exact same version of upb that they are linking against.
**
** You also need to recompile if you change the value of the UPB_DEBUG_REFS
** flag.
*/

#include "upb/def.h"

#ifndef UPB_STATICINIT_H_
#define UPB_STATICINIT_H_

#ifdef __cplusplus
/* Because of how we do our typedefs, this header can't be included from C++. */
#error This file cannot be included from C++
#endif

/* upb_refcounted *************************************************************/


/* upb_def ********************************************************************/

struct upb_def {
  upb_refcounted base;

  const char *fullname;
  const upb_filedef* file;
  char type;  /* A upb_deftype_t (char to save space) */

  /* Used as a flag during the def's mutable stage.  Must be false unless
   * it is currently being used by a function on the stack.  This allows
   * us to easily determine which defs were passed into the function's
   * current invocation. */
  bool came_from_user;
};

#define UPB_DEF_INIT(name, type, vtbl, refs, ref2s) \
    { UPB_REFCOUNT_INIT(vtbl, refs, ref2s), name, NULL, type, false }


/* upb_fielddef ***************************************************************/

struct upb_fielddef {
  upb_def base;

  union {
    int64_t sint;
    uint64_t uint;
    double dbl;
    float flt;
    void *bytes;
  } defaultval;
  union {
    const upb_msgdef *def;  /* If !msg_is_symbolic. */
    char *name;             /* If msg_is_symbolic. */
  } msg;
  union {
    const upb_def *def;  /* If !subdef_is_symbolic. */
    char *name;          /* If subdef_is_symbolic. */
  } sub;  /* The msgdef or enumdef for this field, if upb_hassubdef(f). */
  bool subdef_is_symbolic;
  bool msg_is_symbolic;
  const upb_oneofdef *oneof;
  bool default_is_string;
  bool type_is_set_;     /* False until type is explicitly set. */
  bool is_extension_;
  bool lazy_;
  bool packed_;
  upb_intfmt_t intfmt;
  bool tagdelim;
  upb_fieldtype_t type_;
  upb_label_t label_;
  uint32_t number_;
  uint32_t selector_base;  /* Used to index into a upb::Handlers table. */
  uint32_t index_;
};

extern const struct upb_refcounted_vtbl upb_fielddef_vtbl;

#define UPB_FIELDDEF_INIT(label, type, intfmt, tagdelim, is_extension, lazy,   \
                          packed, name, num, msgdef, subdef, selector_base,    \
                          index, defaultval, refs, ref2s)                      \
  {                                                                            \
    UPB_DEF_INIT(name, UPB_DEF_FIELD, &upb_fielddef_vtbl, refs, ref2s),        \
        defaultval, {msgdef}, {subdef}, NULL, false, false,                    \
        type == UPB_TYPE_STRING || type == UPB_TYPE_BYTES, true, is_extension, \
        lazy, packed, intfmt, tagdelim, type, label, num, selector_base, index \
  }


/* upb_msgdef *****************************************************************/

struct upb_msgdef {
  upb_def base;

  size_t selector_count;
  uint32_t submsg_field_count;

  /* Tables for looking up fields by number and name. */
  upb_inttable itof;  /* int to field */
  upb_strtable ntof;  /* name to field/oneof */

  /* Is this a map-entry message? */
  bool map_entry;

  /* Whether this message has proto2 or proto3 semantics. */
  upb_syntax_t syntax;

  /* Type of well known type message. UPB_WELLKNOWN_UNSPECIFIED for
   * non-well-known message. */
  upb_wellknowntype_t well_known_type;

  /* TODO(haberman): proper extension ranges (there can be multiple). */
};

extern const struct upb_refcounted_vtbl upb_msgdef_vtbl;

/* TODO: also support static initialization of the oneofs table. This will be
 * needed if we compile in descriptors that contain oneofs. */
#define UPB_MSGDEF_INIT(name, selector_count, submsg_field_count, itof, ntof, \
                        map_entry, syntax, well_known_type, refs, ref2s)      \
  {                                                                           \
    UPB_DEF_INIT(name, UPB_DEF_MSG, &upb_fielddef_vtbl, refs, ref2s),         \
        selector_count, submsg_field_count, itof, ntof, map_entry, syntax,    \
        well_known_type                                                       \
  }


/* upb_enumdef ****************************************************************/

struct upb_enumdef {
  upb_def base;

  upb_strtable ntoi;
  upb_inttable iton;
  int32_t defaultval;
};

extern const struct upb_refcounted_vtbl upb_enumdef_vtbl;

#define UPB_ENUMDEF_INIT(name, ntoi, iton, defaultval, refs, ref2s) \
  { UPB_DEF_INIT(name, UPB_DEF_ENUM, &upb_enumdef_vtbl, refs, ref2s), ntoi,    \
    iton, defaultval }


/* upb_oneofdef ***************************************************************/

struct upb_oneofdef {
  upb_refcounted base;

  uint32_t index;  /* Index within oneofs. */
  const char *name;
  upb_strtable ntof;
  upb_inttable itof;
  const upb_msgdef *parent;
};

extern const struct upb_refcounted_vtbl upb_oneofdef_vtbl;

#define UPB_ONEOFDEF_INIT(name, ntof, itof, refs, ref2s) \
  { UPB_REFCOUNT_INIT(&upb_oneofdef_vtbl, refs, ref2s), 0, name, ntof, itof }


/* upb_symtab *****************************************************************/

struct upb_symtab {
  upb_refcounted base;

  upb_strtable symtab;
};

struct upb_filedef {
  upb_refcounted base;

  const char *name;
  const char *package;
  const char *phpprefix;
  const char *phpnamespace;
  upb_syntax_t syntax;

  upb_inttable defs;
  upb_inttable deps;
};

extern const struct upb_refcounted_vtbl upb_filedef_vtbl;

#endif  /* UPB_STATICINIT_H_ */
