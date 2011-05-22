/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2008-2009 Google Inc.  See LICENSE for details.
 * Author: Josh Haberman <jhaberman@gmail.com>
 */

#include <stdlib.h>
#include <stddef.h>
#include <errno.h>
#include "upb_def.h"
#include "upb_msg.h"

#define alignof(t) offsetof(struct { char c; t x; }, x)

static int upb_div_round_up(int numerator, int denominator) {
  /* cf. http://stackoverflow.com/questions/17944/how-to-round-up-the-result-of-integer-division */
  return numerator > 0 ? (numerator - 1) / denominator + 1 : 0;
}

/* Joins strings together, for example:
 *   join("Foo.Bar", "Baz") -> "Foo.Bar.Baz"
 *   join("", "Baz") -> "Baz"
 * Caller owns a ref on the returned string. */
static upb_string *upb_join(upb_string *base, upb_string *name) {
  if (!base || upb_string_len(base) == 0) {
    return upb_string_getref(name);
  } else {
    return upb_string_asprintf(UPB_STRFMT "." UPB_STRFMT,
                               UPB_STRARG(base), UPB_STRARG(name));
  }
}

/* Search for a character in a string, in reverse. */
static int my_memrchr(char *data, char c, size_t len)
{
  int off = len-1;
  while(off > 0 && data[off] != c) --off;
  return off;
}

/* upb_def ********************************************************************/

// Defs are reference counted, but can have cycles when types are
// self-recursive or mutually recursive, so we need to be capable of collecting
// the cycles.  In our situation defs are immutable (so cycles cannot be
// created or destroyed post-initialization).  We need to be thread-safe but
// want to avoid locks if at all possible and rely only on atomic operations.
//
// Our scheme is as follows.  First we give each def a flag indicating whether
// it is part of a cycle or not.  Because defs are immutable, this flag will
// never change.  For acyclic defs, we can use a naive algorithm and avoid the
// overhead of dealing with cycles.  Most defs will be acyclic, and most cycles
// will be very short.
//
// For defs that participate in cycles we keep two reference counts.  One
// tracks references that come from outside the cycle (we call these external
// references), and is incremented and decremented like a regular refcount.
// The other is a cycle refcount, and works as follows.  Every cycle is
// considered distinct, even if two cycles share members.  For example, this
// graph has two distinct cycles:
//
//   A-->B-->C
//   ^   |   |
//   +---+---+
//
// The cycles in this graph are AB and ABC.  When A's external refcount
// transitions from 0->1, we say that A takes "cycle references" on both
// cycles.  Taking a cycle reference means incrementing the cycle refcount of
// all defs in the cycle.  Since A and B are common to both cycles, A and B's
// cycle refcounts will be incremented by two, and C's will be incremented by
// one.  Likewise, when A's external refcount transitions from 1->0, we
// decrement A and B's cycle refcounts by two and C's by one.  We collect a
// cyclic type when its cycle refcount drops to zero.  A precondition for this
// is that the external refcount has dropped to zero also.
//
// This algorithm is relatively cheap, since it only requires extra work when
// the external refcount on a cyclic type transitions from 0->1 or 1->0.

static void upb_msgdef_free(upb_msgdef *m);
static void upb_enumdef_free(upb_enumdef *e);
static void upb_unresolveddef_free(struct _upb_unresolveddef *u);

static void upb_def_free(upb_def *def)
{
  switch(def->type) {
    case UPB_DEF_MSG:
      upb_msgdef_free(upb_downcast_msgdef(def));
      break;
    case UPB_DEF_ENUM:
      upb_enumdef_free(upb_downcast_enumdef(def));
      break;
    case UPB_DEF_SVC:
      assert(false);  /* Unimplemented. */
      break;
    case UPB_DEF_UNRESOLVED:
      upb_unresolveddef_free(upb_downcast_unresolveddef(def));
      break;
    default:
      assert(false);
  }
}

// Depth-first search for all cycles that include cycle_base.  Returns the
// number of paths from def that lead to cycle_base, which is equivalent to the
// number of cycles def is in that include cycle_base.
//
// open_defs tracks the set of nodes that are currently being visited in the
// search so we can stop the search if we detect a cycles that do not involve
// cycle_base.  We can't color the nodes as we go by writing to a member of the
// def, because another thread could be performing the search concurrently.
static int upb_cycle_ref_or_unref(upb_msgdef *m, upb_msgdef *cycle_base,
                                  upb_msgdef **open_defs, int num_open_defs,
                                  bool ref) {
  bool found = false;
  for(int i = 0; i < num_open_defs; i++) {
    if(open_defs[i] == m) {
      // We encountered a cycle that did not involve cycle_base.
      found = true;
      break;
    }
  }

  if(found || num_open_defs == UPB_MAX_TYPE_CYCLE_LEN) {
    return 0;
  } else if(m == cycle_base) {
    return 1;
  } else {
    int path_count = 0;
    if(cycle_base == NULL) {
      cycle_base = m;
    } else {
      open_defs[num_open_defs++] = m;
    }
    upb_msg_iter iter = upb_msg_begin(m);
    for(; !upb_msg_done(iter); iter = upb_msg_next(m, iter)) {
      upb_fielddef *f = upb_msg_iter_field(iter);
      upb_def *def = f->def;
      if(upb_issubmsg(f) && def->is_cyclic) {
        upb_msgdef *sub_m = upb_downcast_msgdef(def);
        path_count += upb_cycle_ref_or_unref(sub_m, cycle_base, open_defs,
                                         num_open_defs, ref);
      }
    }
    if(ref) {
      upb_atomic_add(&m->cycle_refcount, path_count);
    } else {
      if(upb_atomic_add(&m->cycle_refcount, -path_count))
        upb_def_free(UPB_UPCAST(m));
    }
    return path_count;
  }
}

void _upb_def_reftozero(upb_def *def) {
  if(def->is_cyclic) {
    upb_msgdef *m = upb_downcast_msgdef(def);
    upb_msgdef *open_defs[UPB_MAX_TYPE_CYCLE_LEN];
    upb_cycle_ref_or_unref(m, NULL, open_defs, 0, false);
  } else {
    upb_def_free(def);
  }
}

void _upb_def_cyclic_ref(upb_def *def) {
  upb_msgdef *open_defs[UPB_MAX_TYPE_CYCLE_LEN];
  upb_cycle_ref_or_unref(upb_downcast_msgdef(def), NULL, open_defs, 0, true);
}

static void upb_def_init(upb_def *def, upb_deftype type) {
  def->type = type;
  def->is_cyclic = 0;  // We detect this later, after resolving refs.
  def->search_depth = 0;
  def->fqname = NULL;
  upb_atomic_init(&def->refcount, 1);
}

static void upb_def_uninit(upb_def *def) {
  upb_string_unref(def->fqname);
}


/* upb_defbuilder  ************************************************************/

// A upb_defbuilder builds a list of defs by handling a parse of a protobuf in
// the format defined in descriptor.proto.  The output of a upb_defbuilder is
// a list of upb_def* that possibly contain unresolved references.
//
// We use a separate object (upb_defbuilder) instead of having the defs handle
// the parse themselves because we need to store state that is only necessary
// during the building process itself.
//
// All of the handlers registration in this file must be done using the
// low-level upb_register_typed_* interface, since we might not have a msgdef
// yet (in the case of bootstrapping).  This makes it more laborious than it
// will be for real users.

// upb_deflist: A little dynamic array for storing a growing list of upb_defs.
typedef struct {
  upb_def **defs;
  uint32_t len;
  uint32_t size;
} upb_deflist;

static void upb_deflist_init(upb_deflist *l) {
  l->size = 8;
  l->defs = malloc(l->size * sizeof(void*));
  l->len = 0;
}

static void upb_deflist_uninit(upb_deflist *l) {
  for(uint32_t i = 0; i < l->len; i++) upb_def_unref(l->defs[i]);
  free(l->defs);
}

static void upb_deflist_push(upb_deflist *l, upb_def *d) {
  if(l->len == l->size) {
    l->size *= 2;
    l->defs = realloc(l->defs, l->size * sizeof(void*));
  }
  l->defs[l->len++] = d;
}

static upb_def *upb_deflist_last(upb_deflist *l) {
  return l->defs[l->len-1];
}

// Qualify the defname for all defs starting with offset "start" with "str".
static void upb_deflist_qualify(upb_deflist *l, upb_string *str, int32_t start) {
  for(uint32_t i = start; i < l->len; i++) {
    upb_def *def = l->defs[i];
    upb_string *name = def->fqname;
    def->fqname = upb_join(str, name);
    upb_string_unref(name);
  }
}

// We keep a stack of all the messages scopes we are currently in, as well as
// the top-level file scope.  This is necessary to correctly qualify the
// definitions that are contained inside.  "name" tracks the name of the
// message or package (a bare name -- not qualified by any enclosing scopes).
typedef struct {
  upb_string *name;
  // Index of the first def that is under this scope.  For msgdefs, the
  // msgdef itself is at start-1.
  int start;
} upb_defbuilder_frame;

struct _upb_defbuilder {
  upb_deflist defs;
  upb_defbuilder_frame stack[UPB_MAX_TYPE_DEPTH];
  int stack_len;
  upb_status status;
  upb_symtab *symtab;

  uint32_t number;
  upb_string *name;
  bool saw_number;
  bool saw_name;

  upb_string *default_string;

  upb_fielddef *f;
};

// Forward declares for top-level file descriptors.
static upb_mhandlers *upb_msgdef_register_DescriptorProto(upb_handlers *h);
static upb_mhandlers * upb_enumdef_register_EnumDescriptorProto(upb_handlers *h);

upb_defbuilder *upb_defbuilder_new(upb_symtab *s) {
  upb_defbuilder *b = malloc(sizeof(*b));
  upb_deflist_init(&b->defs);
  upb_status_init(&b->status);
  b->symtab = s;
  b->stack_len = 0;
  b->name = NULL;
  b->default_string = NULL;
  return b;
}

static void upb_defbuilder_free(upb_defbuilder *b) {
  upb_string_unref(b->name);
  upb_status_uninit(&b->status);
  upb_deflist_uninit(&b->defs);
  upb_string_unref(b->default_string);
  while (b->stack_len > 0) {
    upb_defbuilder_frame *f = &b->stack[--b->stack_len];
    upb_string_unref(f->name);
  }
  free(b);
}

static upb_msgdef *upb_defbuilder_top(upb_defbuilder *b) {
  if (b->stack_len <= 1) return NULL;
  int index = b->stack[b->stack_len-1].start - 1;
  assert(index >= 0);
  return upb_downcast_msgdef(b->defs.defs[index]);
}

static upb_def *upb_defbuilder_last(upb_defbuilder *b) {
  return upb_deflist_last(&b->defs);
}

// Start/end handlers for FileDescriptorProto and DescriptorProto (the two
// entities that have names and can contain sub-definitions.
void upb_defbuilder_startcontainer(upb_defbuilder *b) {
  upb_defbuilder_frame *f = &b->stack[b->stack_len++];
  f->start = b->defs.len;
  f->name = NULL;
}

void upb_defbuilder_endcontainer(upb_defbuilder *b) {
  upb_defbuilder_frame *f = &b->stack[--b->stack_len];
  upb_deflist_qualify(&b->defs, f->name, f->start);
  upb_string_unref(f->name);
}

void upb_defbuilder_setscopename(upb_defbuilder *b, upb_string *str) {
  upb_defbuilder_frame *f = &b->stack[b->stack_len-1];
  upb_string_unref(f->name);
  f->name = upb_string_getref(str);
}

// Handlers for google.protobuf.FileDescriptorProto.
static upb_flow_t upb_defbuilder_FileDescriptorProto_startmsg(void *_b) {
  upb_defbuilder *b = _b;
  upb_defbuilder_startcontainer(b);
  return UPB_CONTINUE;
}

static void upb_defbuilder_FileDescriptorProto_endmsg(void *_b,
                                                      upb_status *status) {
  (void)status;
  upb_defbuilder *b = _b;
  upb_defbuilder_endcontainer(b);
}

static upb_flow_t upb_defbuilder_FileDescriptorProto_package(void *_b,
                                                             upb_value fval,
                                                             upb_value val) {
  (void)fval;
  upb_defbuilder *b = _b;
  upb_defbuilder_setscopename(b, upb_value_getstr(val));
  return UPB_CONTINUE;
}

static upb_mhandlers *upb_defbuilder_register_FileDescriptorProto(
    upb_handlers *h) {
  upb_mhandlers *m = upb_handlers_newmhandlers(h);
  upb_mhandlers_setstartmsg(m, &upb_defbuilder_FileDescriptorProto_startmsg);
  upb_mhandlers_setendmsg(m, &upb_defbuilder_FileDescriptorProto_endmsg);

#define FNUM(field) GOOGLE_PROTOBUF_FILEDESCRIPTORPROTO_ ## field ## __FIELDNUM
#define FTYPE(field) GOOGLE_PROTOBUF_FILEDESCRIPTORPROTO_ ## field ## __FIELDTYPE
  upb_fhandlers *f =
      upb_mhandlers_newfhandlers(m, FNUM(PACKAGE), FTYPE(PACKAGE), false);
  upb_fhandlers_setvalue(f, &upb_defbuilder_FileDescriptorProto_package);

  upb_mhandlers_newfhandlers_subm(m, FNUM(MESSAGE_TYPE), FTYPE(MESSAGE_TYPE), true,
                                  upb_msgdef_register_DescriptorProto(h));
  upb_mhandlers_newfhandlers_subm(m, FNUM(ENUM_TYPE), FTYPE(ENUM_TYPE), true,
                                  upb_enumdef_register_EnumDescriptorProto(h));
  // TODO: services, extensions
  return m;
}
#undef FNUM
#undef FTYPE

// Handlers for google.protobuf.FileDescriptorSet.
static bool upb_symtab_add_defs(upb_symtab *s, upb_def **defs, int num_defs,
                                bool allow_redef, upb_status *status);

static void upb_defbuilder_FileDescriptorSet_onendmsg(void *_b,
                                                       upb_status *status) {
  upb_defbuilder *b = _b;
  if (upb_ok(status))
    upb_symtab_add_defs(b->symtab, b->defs.defs, b->defs.len, false, status);
  upb_defbuilder_free(b);
}

static upb_mhandlers *upb_defbuilder_register_FileDescriptorSet(upb_handlers *h) {
  upb_mhandlers *m = upb_handlers_newmhandlers(h);
  upb_mhandlers_setendmsg(m, upb_defbuilder_FileDescriptorSet_onendmsg);

#define FNUM(field) GOOGLE_PROTOBUF_FILEDESCRIPTORSET_ ## field ## __FIELDNUM
#define FTYPE(field) GOOGLE_PROTOBUF_FILEDESCRIPTORSET_ ## field ## __FIELDTYPE
  upb_mhandlers_newfhandlers_subm(m, FNUM(FILE), FTYPE(FILE), true,
                                   upb_defbuilder_register_FileDescriptorProto(h));
  return m;
}
#undef FNUM
#undef FTYPE

upb_mhandlers *upb_defbuilder_reghandlers(upb_handlers *h) {
  h->should_jit = false;
  return upb_defbuilder_register_FileDescriptorSet(h);
}


/* upb_unresolveddef **********************************************************/

// Unresolved defs are used as temporary placeholders for a def whose name has
// not been resolved yet.  During the name resolution step, all unresolved defs
// are replaced with pointers to the actual def being referenced.
typedef struct _upb_unresolveddef {
  upb_def base;

  // The target type name.  This may or may not be fully qualified.  It is
  // tempting to want to use base.fqname for this, but that will be qualified
  // which is inappropriate for a name we still have to resolve.
  upb_string *name;
} upb_unresolveddef;

// Is passed a ref on the string.
static upb_unresolveddef *upb_unresolveddef_new(upb_string *str) {
  upb_unresolveddef *def = malloc(sizeof(*def));
  upb_def_init(&def->base, UPB_DEF_UNRESOLVED);
  def->name = upb_string_getref(str);
  return def;
}

static void upb_unresolveddef_free(struct _upb_unresolveddef *def) {
  upb_string_unref(def->name);
  upb_def_uninit(&def->base);
  free(def);
}


/* upb_enumdef ****************************************************************/

static void upb_enumdef_free(upb_enumdef *e) {
  upb_enum_iter i;
  for(i = upb_enum_begin(e); !upb_enum_done(i); i = upb_enum_next(e, i)) {
    // Frees the ref taken when the string was parsed.
    upb_string_unref(upb_enum_iter_name(i));
  }
  upb_strtable_free(&e->ntoi);
  upb_inttable_free(&e->iton);
  upb_def_uninit(&e->base);
  free(e);
}

// google.protobuf.EnumValueDescriptorProto.
static upb_flow_t upb_enumdef_EnumValueDescriptorProto_startmsg(void *_b) {
  upb_defbuilder *b = _b;
  b->saw_number = false;
  b->saw_name = false;
  return UPB_CONTINUE;
}

static upb_flow_t upb_enumdef_EnumValueDescriptorProto_name(void *_b,
                                                            upb_value fval,
                                                            upb_value val) {
  (void)fval;
  upb_defbuilder *b = _b;
  upb_string_unref(b->name);
  b->name = upb_string_getref(upb_value_getstr(val));
  b->saw_name = true;
  return UPB_CONTINUE;
}

static upb_flow_t upb_enumdef_EnumValueDescriptorProto_number(void *_b,
                                                              upb_value fval,
                                                              upb_value val) {
  (void)fval;
  upb_defbuilder *b = _b;
  b->number = upb_value_getint32(val);
  b->saw_number = true;
  return UPB_CONTINUE;
}

static void upb_enumdef_EnumValueDescriptorProto_endmsg(void *_b,
                                                        upb_status *status) {
  upb_defbuilder *b = _b;
  if(!b->saw_number || !b->saw_name) {
    upb_seterr(status, UPB_ERROR, "Enum value missing name or number.");
    return;
  }
  upb_enumdef *e = upb_downcast_enumdef(upb_defbuilder_last(b));
  if (upb_inttable_count(&e->iton) == 0) {
    // The default value of an enum (in the absence of an explicit default) is
    // its first listed value.
    e->default_value = b->number;
  }
  upb_ntoi_ent ntoi_ent = {{b->name, 0}, b->number};
  upb_iton_ent iton_ent = {0, b->name};
  upb_strtable_insert(&e->ntoi, &ntoi_ent.e);
  upb_inttable_insert(&e->iton, b->number, &iton_ent);
  // We don't unref "name" because we pass our ref to the iton entry of the
  // table.  strtables can ref their keys, but the inttable doesn't know that
  // the value is a string.
  b->name = NULL;
}

static upb_mhandlers *upb_enumdef_register_EnumValueDescriptorProto(
    upb_handlers *h) {
  upb_mhandlers *m = upb_handlers_newmhandlers(h);
  upb_mhandlers_setstartmsg(m, &upb_enumdef_EnumValueDescriptorProto_startmsg);
  upb_mhandlers_setendmsg(m, &upb_enumdef_EnumValueDescriptorProto_endmsg);

#define FNUM(f) GOOGLE_PROTOBUF_ENUMVALUEDESCRIPTORPROTO_ ## f ## __FIELDNUM
#define FTYPE(f) GOOGLE_PROTOBUF_ENUMVALUEDESCRIPTORPROTO_ ## f ## __FIELDTYPE
  upb_fhandlers *f;
  f = upb_mhandlers_newfhandlers(m, FNUM(NAME), FTYPE(NAME), false);
  upb_fhandlers_setvalue(f, &upb_enumdef_EnumValueDescriptorProto_name);

  f = upb_mhandlers_newfhandlers(m, FNUM(NUMBER), FTYPE(NUMBER), false);
  upb_fhandlers_setvalue(f, &upb_enumdef_EnumValueDescriptorProto_number);
  return m;
}
#undef FNUM
#undef FTYPE

// google.protobuf.EnumDescriptorProto.
static upb_flow_t upb_enumdef_EnumDescriptorProto_startmsg(void *_b) {
  upb_defbuilder *b = _b;
  upb_enumdef *e = malloc(sizeof(*e));
  upb_def_init(&e->base, UPB_DEF_ENUM);
  upb_strtable_init(&e->ntoi, 0, sizeof(upb_ntoi_ent));
  upb_inttable_init(&e->iton, 0, sizeof(upb_iton_ent));
  upb_deflist_push(&b->defs, UPB_UPCAST(e));
  return UPB_CONTINUE;
}

static void upb_enumdef_EnumDescriptorProto_endmsg(void *_b, upb_status *status) {
  upb_defbuilder *b = _b;
  upb_enumdef *e = upb_downcast_enumdef(upb_defbuilder_last(b));
  if (upb_defbuilder_last((upb_defbuilder*)_b)->fqname == NULL) {
    upb_seterr(status, UPB_ERROR, "Enum had no name.");
    return;
  }
  if (upb_inttable_count(&e->iton) == 0) {
    upb_seterr(status, UPB_ERROR, "Enum had no values.");
    return;
  }
}

static upb_flow_t upb_enumdef_EnumDescriptorProto_name(void *_b,
                                                       upb_value fval,
                                                       upb_value val) {
  (void)fval;
  upb_defbuilder *b = _b;
  upb_enumdef *e = upb_downcast_enumdef(upb_defbuilder_last(b));
  upb_string_unref(e->base.fqname);
  e->base.fqname = upb_string_getref(upb_value_getstr(val));
  return UPB_CONTINUE;
}

static upb_mhandlers *upb_enumdef_register_EnumDescriptorProto(upb_handlers *h) {
  upb_mhandlers *m = upb_handlers_newmhandlers(h);
  upb_mhandlers_setstartmsg(m, &upb_enumdef_EnumDescriptorProto_startmsg);
  upb_mhandlers_setendmsg(m, &upb_enumdef_EnumDescriptorProto_endmsg);

#define FNUM(f) GOOGLE_PROTOBUF_ENUMDESCRIPTORPROTO_ ## f ## __FIELDNUM
#define FTYPE(f) GOOGLE_PROTOBUF_ENUMDESCRIPTORPROTO_ ## f ## __FIELDTYPE
  upb_fhandlers *f =
      upb_mhandlers_newfhandlers(m, FNUM(NAME), FTYPE(NAME), false);
  upb_fhandlers_setvalue(f, &upb_enumdef_EnumDescriptorProto_name);

  upb_mhandlers_newfhandlers_subm(m, FNUM(VALUE), FTYPE(VALUE), true,
                               upb_enumdef_register_EnumValueDescriptorProto(h));
  return m;
}
#undef FNUM
#undef FTYPE

upb_enum_iter upb_enum_begin(upb_enumdef *e) {
  // We could iterate over either table here; the choice is arbitrary.
  return upb_inttable_begin(&e->iton);
}

upb_enum_iter upb_enum_next(upb_enumdef *e, upb_enum_iter iter) {
  return upb_inttable_next(&e->iton, iter);
}

upb_string *upb_enumdef_iton(upb_enumdef *def, upb_enumval_t num) {
  upb_iton_ent *e =
      (upb_iton_ent*)upb_inttable_fastlookup(&def->iton, num, sizeof(*e));
  return e ? e->string : NULL;
}

bool upb_enumdef_ntoi(upb_enumdef *def, upb_string *name, upb_enumval_t *num) {
  upb_ntoi_ent *e = (upb_ntoi_ent*)upb_strtable_lookup(&def->ntoi, name);
  if (!e) return false;
  *num = e->value;
  return true;
}


/* upb_fielddef ***************************************************************/

static void upb_fielddef_free(upb_fielddef *f) {
  if (upb_isstring(f)) {
    upb_string_unref(upb_value_getstr(f->default_value));
  } else if (upb_issubmsg(f)) {
    upb_msg *m = upb_value_getmsg(f->default_value);
    assert(m);
    // We cheat a bit here.  We need to unref msg, but we don't have a reliable
    // way of accessing the msgdef (which is required by upb_msg_unref()),
    // because f->def may have already been collected as part of a cycle if
    // this is an unowned ref.  But we know that default messages never contain
    // references to other messages, and their only string references are to
    // the singleton empty string, so we can safely unref+free msg directly.
    if (upb_atomic_unref(&m->refcount)) free(m);
  }
  upb_string_unref(f->name);
  if(f->owned) {
    upb_def_unref(f->def);
  }
  free(f);
}

static bool upb_fielddef_resolve(upb_fielddef *f, upb_def *def, upb_status *s) {
  if(f->owned) upb_def_unref(f->def);
  f->def = def;
  // We will later make the ref unowned if it is a part of a cycle.
  f->owned = true;
  upb_def_ref(def);
  if (upb_issubmsg(f)) {
    upb_msgdef *md = upb_downcast_msgdef(def);
    upb_value_setmsg(&f->default_value, upb_msg_getref(md->default_message));
  } else if (f->type == UPB_TYPE(ENUM)) {
    upb_string *str = upb_value_getstr(f->default_value);
    assert(str);  // Should point to either a real default or the empty string.
    upb_enumdef *e = upb_downcast_enumdef(f->def);
    upb_enumval_t val = 0;
    if (str == upb_emptystring()) {
      upb_value_setint32(&f->default_value, e->default_value);
    } else {
      bool success = upb_enumdef_ntoi(e, str, &val);
      upb_string_unref(str);
      if (!success) {
        upb_seterr(s, UPB_ERROR, "Default enum value (" UPB_STRFMT ") is not a "
                   "member of the enum", UPB_STRARG(str));
        return false;
      }
      upb_value_setint32(&f->default_value, val);
    }
  }
  return true;
}

static upb_flow_t upb_fielddef_startmsg(void *_b) {
  upb_defbuilder *b = _b;
  upb_fielddef *f = malloc(sizeof(*f));
  f->number = -1;
  f->name = NULL;
  f->def = NULL;
  f->owned = false;
  f->msgdef = upb_defbuilder_top(b);
  b->f = f;
  return UPB_CONTINUE;
}

// Converts the default value in string "dstr" into "d".  Passes a ref on dstr.
// Returns true on success.
static bool upb_fielddef_setdefault(upb_string *dstr, upb_value *d, int type) {
  bool success = true;
  if (type == UPB_TYPE(STRING) || type == UPB_TYPE(BYTES) || type == UPB_TYPE(ENUM)) {
    // We'll keep the ref we had on it.  We include enums in this case because
    // we need the enumdef to resolve the name, but we may not have it yet.
    // We'll resolve it later.
    if (dstr) {
      upb_value_setstr(d, dstr);
    } else {
      upb_value_setstr(d, upb_emptystring());
    }
  } else if (type == UPB_TYPE(MESSAGE) || type == UPB_TYPE(GROUP)) {
    // We don't expect to get a default value.
    upb_string_unref(dstr);
    if (dstr != NULL) success = false;
  } else {
    // The strto* functions need the string to be NULL-terminated.
    char *strz = upb_string_isempty(dstr) ? NULL : upb_string_newcstr(dstr);
    char *end;
    upb_string_unref(dstr);
    switch (type) {
      case UPB_TYPE(INT32):
      case UPB_TYPE(SINT32):
      case UPB_TYPE(SFIXED32):
        if (strz) {
          long val = strtol(strz, &end, 0);
          if (val > INT32_MAX || val < INT32_MIN || errno == ERANGE || *end)
            success = false;
          else
            upb_value_setint32(d, val);
        } else {
          upb_value_setint32(d, 0);
        }
        break;
      case UPB_TYPE(INT64):
      case UPB_TYPE(SINT64):
      case UPB_TYPE(SFIXED64):
        if (strz) {
          upb_value_setint64(d, strtoll(strz, &end, 0));
          if (errno == ERANGE || *end) success = false;
        } else {
          upb_value_setint64(d, 0);
        }
        break;
      case UPB_TYPE(UINT32):
      case UPB_TYPE(FIXED32):
        if (strz) {
          unsigned long val = strtoul(strz, &end, 0);
          if (val > UINT32_MAX || errno == ERANGE || *end)
            success = false;
          else
            upb_value_setuint32(d, val);
        } else {
          upb_value_setuint32(d, 0);
        }
        break;
      case UPB_TYPE(UINT64):
      case UPB_TYPE(FIXED64):
        if (strz) {
          upb_value_setuint64(d, strtoull(strz, &end, 0));
          if (errno == ERANGE || *end) success = false;
        } else {
          upb_value_setuint64(d, 0);
        }
        break;
      case UPB_TYPE(DOUBLE):
        if (strz) {
          upb_value_setdouble(d, strtod(strz, &end));
          if (errno == ERANGE || *end) success = false;
        } else {
          upb_value_setdouble(d, 0.0);
        }
        break;
      case UPB_TYPE(FLOAT):
        if (strz) {
          upb_value_setfloat(d, strtof(strz, &end));
          if (errno == ERANGE || *end) success = false;
        } else {
          upb_value_setfloat(d, 0.0);
        }
        break;
      case UPB_TYPE(BOOL):
        if (!strz || strcmp(strz, "false") == 0)
          upb_value_setbool(d, false);
        else if (strcmp(strz, "true") == 0)
          upb_value_setbool(d, true);
        else
          success = false;
        break;
    }
    free(strz);
  }
  return success;
}

static void upb_fielddef_endmsg(void *_b, upb_status *status) {
  upb_defbuilder *b = _b;
  upb_fielddef *f = b->f;
  // TODO: verify that all required fields were present.
  assert(f->number != -1 && f->name != NULL);
  assert((f->def != NULL) == upb_hasdef(f));

  // Field was successfully read, add it as a field of the msgdef.
  upb_msgdef *m = upb_defbuilder_top(b);
  upb_itof_ent itof_ent = {0, f->type, upb_types[f->type].native_wire_type, f};
  upb_ntof_ent ntof_ent = {{f->name, 0}, f};
  upb_inttable_insert(&m->itof, f->number, &itof_ent);
  upb_strtable_insert(&m->ntof, &ntof_ent.e);

  upb_string *dstr = b->default_string;
  b->default_string = NULL;
  if (!upb_fielddef_setdefault(dstr, &f->default_value, f->type)) {
    // We don't worry too much about giving a great error message since the
    // compiler should have ensured this was correct.
    upb_seterr(status, UPB_ERROR, "Error converting default value.");
    return;
  }
}

static upb_flow_t upb_fielddef_ontype(void *_b, upb_value fval, upb_value val) {
  (void)fval;
  upb_defbuilder *b = _b;
  b->f->type = upb_value_getint32(val);
  return UPB_CONTINUE;
}

static upb_flow_t upb_fielddef_onlabel(void *_b, upb_value fval, upb_value val) {
  (void)fval;
  upb_defbuilder *b = _b;
  b->f->label = upb_value_getint32(val);
  return UPB_CONTINUE;
}

static upb_flow_t upb_fielddef_onnumber(void *_b, upb_value fval, upb_value val) {
  (void)fval;
  upb_defbuilder *b = _b;
  b->f->number = upb_value_getint32(val);
  return UPB_CONTINUE;
}

static upb_flow_t upb_fielddef_onname(void *_b, upb_value fval, upb_value val) {
  (void)fval;
  upb_defbuilder *b = _b;
  upb_string_unref(b->f->name);
  b->f->name = upb_string_getref(upb_value_getstr(val));
  return UPB_CONTINUE;
}

static upb_flow_t upb_fielddef_ontypename(void *_b, upb_value fval,
                                          upb_value val) {
  (void)fval;
  upb_defbuilder *b = _b;
  upb_def_unref(b->f->def);
  b->f->def = UPB_UPCAST(upb_unresolveddef_new(upb_value_getstr(val)));
  b->f->owned = true;
  return UPB_CONTINUE;
}

static upb_flow_t upb_fielddef_ondefaultval(void *_b, upb_value fval,
                                            upb_value val) {
  (void)fval;
  upb_defbuilder *b = _b;
  // Have to convert from string to the correct type, but we might not know the
  // type yet.
  upb_string_unref(b->default_string);
  b->default_string = upb_string_getref(upb_value_getstr(val));
  return UPB_CONTINUE;
}

static upb_mhandlers *upb_fielddef_register_FieldDescriptorProto(
    upb_handlers *h) {
  upb_mhandlers *m = upb_handlers_newmhandlers(h);
  upb_mhandlers_setstartmsg(m, &upb_fielddef_startmsg);
  upb_mhandlers_setendmsg(m, &upb_fielddef_endmsg);

#define FIELD(name, handler) \
  upb_fhandlers_setvalue( \
      upb_mhandlers_newfhandlers(m, \
          GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_ ## name ## __FIELDNUM, \
          GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_ ## name ## __FIELDTYPE, \
          false), \
      handler);
  FIELD(TYPE, &upb_fielddef_ontype);
  FIELD(LABEL, &upb_fielddef_onlabel);
  FIELD(NUMBER, &upb_fielddef_onnumber);
  FIELD(NAME, &upb_fielddef_onname);
  FIELD(TYPE_NAME, &upb_fielddef_ontypename);
  FIELD(DEFAULT_VALUE, &upb_fielddef_ondefaultval);
  return m;
}
#undef FNUM
#undef FTYPE


/* upb_msgdef *****************************************************************/

static int upb_compare_typed_fields(upb_fielddef *f1, upb_fielddef *f2) {
  // Sort by data size (ascending) to reduce padding.
  size_t size1 = upb_types[f1->type].size;
  size_t size2 = upb_types[f2->type].size;
  if (size1 != size2) return size1 - size2;
  // Otherwise return in number order (just so we get a reproduceable order.
  return f1->number - f2->number;
}

static int upb_compare_fields(const void *f1, const void *f2) {
  return upb_compare_typed_fields(*(void**)f1, *(void**)f2);
}

// google.protobuf.DescriptorProto.
static upb_flow_t upb_msgdef_startmsg(void *_b) {
  upb_defbuilder *b = _b;
  upb_msgdef *m = malloc(sizeof(*m));
  upb_def_init(&m->base, UPB_DEF_MSG);
  upb_atomic_init(&m->cycle_refcount, 0);
  upb_inttable_init(&m->itof, 4, sizeof(upb_itof_ent));
  upb_strtable_init(&m->ntof, 4, sizeof(upb_ntof_ent));
  m->default_message = NULL;
  upb_deflist_push(&b->defs, UPB_UPCAST(m));
  upb_defbuilder_startcontainer(b);
  return UPB_CONTINUE;
}

static void upb_msgdef_endmsg(void *_b, upb_status *status) {
  upb_defbuilder *b = _b;
  upb_msgdef *m = upb_defbuilder_top(b);
  if(!m->base.fqname) {
    upb_seterr(status, UPB_ERROR, "Encountered message with no name.");
    return;
  }

  upb_inttable_compact(&m->itof);
  // Create an ordering over the fields.
  int n = upb_msgdef_numfields(m);
  upb_fielddef **sorted_fields = malloc(sizeof(upb_fielddef*) * n);
  int field = 0;
  upb_msg_iter i;
  for (i = upb_msg_begin(m); !upb_msg_done(i); i = upb_msg_next(m, i)) {
    sorted_fields[field++] = upb_msg_iter_field(i);
  }
  qsort(sorted_fields, n, sizeof(*sorted_fields), upb_compare_fields);

  // Assign offsets in the msg.
  m->set_flags_bytes = upb_div_round_up(n, 8);
  m->size = sizeof(upb_atomic_t) + m->set_flags_bytes;

  size_t max_align = 0;
  for (int i = 0; i < n; i++) {
    upb_fielddef *f = sorted_fields[i];
    const upb_type_info *type_info = &upb_types[f->type];

    // This identifies the set bit.  When we implement is_initialized (a
    // general check about whether all required bits are set) we will probably
    // want to use a different ordering that puts all the required bits
    // together.
    f->field_index = i;
    f->set_bit_mask = 1 << (i % 8);
    f->set_bit_offset = i / 8;

    size_t size, align;
    if (upb_isarray(f)) {
      size = sizeof(void*);
      align = alignof(void*);
    } else {
      size = type_info->size;
      align = type_info->align;
    }
    // General alignment rules are: each member must be at an address that is a
    // multiple of that type's alignment.  Also, the size of the structure as a
    // whole must be a multiple of the greatest alignment of any member.
    size_t offset = upb_align_up(m->size, align);
    // Offsets are relative to the end of the refcount.
    f->byte_offset = offset - sizeof(upb_atomic_t);
    m->size = offset + size;
    max_align = UPB_MAX(max_align, align);
  }
  free(sorted_fields);

  if (max_align > 0) m->size = upb_align_up(m->size, max_align);

  // Create default message instance, an immutable message with all default
  // values set (except submessages, which are simply marked as unset).  We
  // could alternatively leave all set bits unset, but this would make
  // upb_msg_get() take its unexpected branch more often for no good reason.
  m->default_message = upb_msg_new(m);
  for (i = upb_msg_begin(m); !upb_msg_done(i); i = upb_msg_next(m, i)) {
    upb_fielddef *f = upb_msg_iter_field(i);
    if (!upb_issubmsg(f) && !f->type == UPB_TYPE(ENUM)) {
      upb_msg_set(m->default_message, f, f->default_value);
    }
  }

  upb_defbuilder_endcontainer(b);
}

static upb_flow_t upb_msgdef_onname(void *_b, upb_value fval, upb_value val) {
  (void)fval;
  upb_defbuilder *b = _b;
  assert(val.type == UPB_TYPE(STRING));
  upb_msgdef *m = upb_defbuilder_top(b);
  upb_string_unref(m->base.fqname);
  m->base.fqname = upb_string_getref(upb_value_getstr(val));
  upb_defbuilder_setscopename(b, upb_value_getstr(val));
  return UPB_CONTINUE;
}

static upb_mhandlers *upb_msgdef_register_DescriptorProto(upb_handlers *h) {
  upb_mhandlers *m = upb_handlers_newmhandlers(h);
  upb_mhandlers_setstartmsg(m, &upb_msgdef_startmsg);
  upb_mhandlers_setendmsg(m, &upb_msgdef_endmsg);

#define FNUM(f) GOOGLE_PROTOBUF_DESCRIPTORPROTO_ ## f ## __FIELDNUM
#define FTYPE(f) GOOGLE_PROTOBUF_DESCRIPTORPROTO_ ## f ## __FIELDTYPE
  upb_fhandlers *f =
      upb_mhandlers_newfhandlers(m, FNUM(NAME), FTYPE(NAME), false);
  upb_fhandlers_setvalue(f, &upb_msgdef_onname);

  upb_mhandlers_newfhandlers_subm(m, FNUM(FIELD), FTYPE(FIELD), true,
                                  upb_fielddef_register_FieldDescriptorProto(h));
  upb_mhandlers_newfhandlers_subm(m, FNUM(ENUM_TYPE), FTYPE(ENUM_TYPE), true,
                                  upb_enumdef_register_EnumDescriptorProto(h));

  // DescriptorProto is self-recursive, so we must link the definition.
  upb_mhandlers_newfhandlers_subm(
      m, FNUM(NESTED_TYPE), FTYPE(NESTED_TYPE), true, m);

  // TODO: extensions.
  return m;
}
#undef FNUM
#undef FTYPE

static void upb_msgdef_free(upb_msgdef *m)
{
  upb_msg_unref(m->default_message, m);
  upb_msg_iter i;
  for(i = upb_msg_begin(m); !upb_msg_done(i); i = upb_msg_next(m, i))
    upb_fielddef_free(upb_msg_iter_field(i));
  upb_strtable_free(&m->ntof);
  upb_inttable_free(&m->itof);
  upb_def_uninit(&m->base);
  free(m);
}

upb_msg_iter upb_msg_begin(upb_msgdef *m) {
  return upb_inttable_begin(&m->itof);
}

upb_msg_iter upb_msg_next(upb_msgdef *m, upb_msg_iter iter) {
  return upb_inttable_next(&m->itof, iter);
}

/* upb_symtab adding defs *****************************************************/

// This is a self-contained group of functions that, given a list of upb_defs
// whose references are not yet resolved, resolves references and adds them
// atomically to a upb_symtab.

typedef struct {
  upb_strtable_entry e;
  upb_def *def;
} upb_symtab_ent;

// Given a symbol and the base symbol inside which it is defined, find the
// symbol's definition in t.
static upb_symtab_ent *upb_resolve(upb_strtable *t,
                                   upb_string *base, upb_string *sym)
{
  if(upb_string_len(sym) == 0) return NULL;
  if(upb_string_getrobuf(sym)[0] == UPB_SYMBOL_SEPARATOR) {
    // Symbols starting with '.' are absolute, so we do a single lookup.
    // Slice to omit the leading '.'
    upb_string *sym_str = upb_strslice(sym, 1, upb_string_len(sym) - 1);
    upb_symtab_ent *e = upb_strtable_lookup(t, sym_str);
    upb_string_unref(sym_str);
    return e;
  } else {
    // Remove components from base until we find an entry or run out.
    // TODO: This branch is totally broken, but currently not used.
    upb_string *sym_str = upb_string_new();
    int baselen = upb_string_len(base);
    upb_symtab_ent *ret = NULL;
    while(1) {
      // sym_str = base[0...base_len] + UPB_SYMBOL_SEPARATOR + sym
      upb_strlen_t len = baselen + upb_string_len(sym) + 1;
      char *buf = upb_string_getrwbuf(sym_str, len);
      memcpy(buf, upb_string_getrobuf(base), baselen);
      buf[baselen] = UPB_SYMBOL_SEPARATOR;
      memcpy(buf + baselen + 1, upb_string_getrobuf(sym), upb_string_len(sym));

      upb_symtab_ent *e = upb_strtable_lookup(t, sym_str);
      if (e) {
        ret = e;
        break;
      } else if(baselen == 0) {
        // No more scopes to try.
        ret = NULL;
        break;
      }
      baselen = my_memrchr(buf, UPB_SYMBOL_SEPARATOR, baselen);
    }
    upb_string_unref(sym_str);
    return ret;
  }
}

// Performs a pass over the type graph to find all cycles that include m.
static bool upb_symtab_findcycles(upb_msgdef *m, int depth, upb_status *status)
{
  if(depth > UPB_MAX_TYPE_DEPTH) {
    // We have found a non-cyclic path from the base of the type tree that
    // exceeds the maximum allowed depth.  There are many situations in upb
    // where we recurse over the type tree (like for example, right now) and an
    // absurdly deep tree could cause us to stack overflow on systems with very
    // limited stacks.
    upb_seterr(status, UPB_ERROR, "Type " UPB_STRFMT " was found at "
               "depth %d in the type graph, which exceeds the maximum type "
               "depth of %d.", UPB_UPCAST(m)->fqname, depth,
               UPB_MAX_TYPE_DEPTH);
    return false;
  } else if(UPB_UPCAST(m)->search_depth == 1) {
    // Cycle!
    int cycle_len = depth - 1;
    if(cycle_len > UPB_MAX_TYPE_CYCLE_LEN) {
      upb_seterr(status, UPB_ERROR, "Type " UPB_STRFMT " was involved "
                 "in a cycle of length %d, which exceeds the maximum type "
                 "cycle length of %d.", UPB_UPCAST(m)->fqname, cycle_len,
                 UPB_MAX_TYPE_CYCLE_LEN);
      return false;
    }
    return true;
  } else if(UPB_UPCAST(m)->search_depth > 0) {
    // This was a cycle, but did not originate from the base of our search tree.
    // We'll find it when we call find_cycles() on this node directly.
    return false;
  } else {
    UPB_UPCAST(m)->search_depth = ++depth;
    bool cycle_found = false;
    upb_msg_iter i;
    for(i = upb_msg_begin(m); !upb_msg_done(i); i = upb_msg_next(m, i)) {
      upb_fielddef *f = upb_msg_iter_field(i);
      if(!upb_issubmsg(f)) continue;
      upb_def *sub_def = f->def;
      upb_msgdef *sub_m = upb_downcast_msgdef(sub_def);
      if(upb_symtab_findcycles(sub_m, depth, status)) {
        cycle_found = true;
        UPB_UPCAST(m)->is_cyclic = true;
        if(f->owned) {
          upb_atomic_unref(&sub_def->refcount);
          f->owned = false;
        }
      }
    }
    UPB_UPCAST(m)->search_depth = 0;
    return cycle_found;
  }
}

// Given a table of pending defs "tmptab" and a table of existing defs "symtab",
// resolves all of the unresolved refs for the defs in tmptab.  Also resolves
// default values for enumerations and submessages.
bool upb_resolverefs(upb_strtable *tmptab, upb_strtable *symtab,
                     upb_status *status)
{
  upb_symtab_ent *e;
  for(e = upb_strtable_begin(tmptab); e; e = upb_strtable_next(tmptab, &e->e)) {
    upb_msgdef *m = upb_dyncast_msgdef(e->def);
    if(!m) continue;
    // Type names are resolved relative to the message in which they appear.
    upb_string *base = e->e.key;

    upb_msg_iter i;
    for(i = upb_msg_begin(m); !upb_msg_done(i); i = upb_msg_next(m, i)) {
      upb_fielddef *f = upb_msg_iter_field(i);
      if(!upb_hasdef(f)) continue;  // No resolving necessary.
      upb_string *name = upb_downcast_unresolveddef(f->def)->name;

      // Resolve from either the tmptab (pending adds) or symtab (existing
      // defs).  If both exist, prefer the pending add, because it will be
      // overwriting the existing def.
      upb_symtab_ent *found;
      if(!(found = upb_resolve(tmptab, base, name)) &&
         !(found = upb_resolve(symtab, base, name))) {
        upb_seterr(status, UPB_ERROR,
                   "could not resolve symbol '" UPB_STRFMT "'"
                   " in context '" UPB_STRFMT "'",
                   UPB_STRARG(name), UPB_STRARG(base));
        return false;
      }

      // Check the type of the found def.
      upb_fieldtype_t expected = upb_issubmsg(f) ? UPB_DEF_MSG : UPB_DEF_ENUM;
      if(found->def->type != expected) {
        upb_seterr(status, UPB_ERROR, "Unexpected type");
        return false;
      }
      if (!upb_fielddef_resolve(f, found->def, status)) return false;
    }
  }

  // Deal with type cycles.
  for(e = upb_strtable_begin(tmptab); e; e = upb_strtable_next(tmptab, &e->e)) {
    upb_msgdef *m = upb_dyncast_msgdef(e->def);
    if(!m) continue;
    // The findcycles() call will decrement the external refcount of the
    upb_symtab_findcycles(m, 0, status);
    upb_msgdef *open_defs[UPB_MAX_TYPE_CYCLE_LEN];
    upb_cycle_ref_or_unref(m, NULL, open_defs, 0, true);
  }

  return true;
}

// Given a list of defs, a list of extensions (in the future), and a flag
// indicating whether the new defs can overwrite existing defs in the symtab,
// attempts to add the given defs to the symtab.  The whole operation either
// succeeds or fails.  Ownership of "defs" and "exts" is taken.
static bool upb_symtab_add_defs(upb_symtab *s, upb_def **defs, int num_defs,
                                bool allow_redef, upb_status *status)
{
  upb_rwlock_wrlock(&s->lock);

  // Build a table of the defs we mean to add, for duplicate detection and name
  // resolution.
  upb_strtable tmptab;
  upb_strtable_init(&tmptab, num_defs, sizeof(upb_symtab_ent));
  for (int i = 0; i < num_defs; i++) {
    upb_def *def = defs[i];
    upb_symtab_ent e = {{def->fqname, 0}, def};

    // Redefinition is never allowed within a single FileDescriptorSet.
    // Additionally, we only allow overwriting of an existing definition if
    // allow_redef is set.
    if (upb_strtable_lookup(&tmptab, def->fqname) ||
        (!allow_redef && upb_strtable_lookup(&s->symtab, def->fqname))) {
      upb_seterr(status, UPB_ERROR, "Redefinition of symbol " UPB_STRFMT,
                 UPB_STRARG(def->fqname));
      goto err;
    }

    // Pass ownership from the deflist to the strtable.
    upb_strtable_insert(&tmptab, &e.e);
    defs[i] = NULL;
  }

  // TODO: process the list of extensions by modifying entries from
  // tmptab in-place (copying them from the symtab first if necessary).

  if (!upb_resolverefs(&tmptab, &s->symtab, status)) goto err;

  // The defs in tmptab have been vetted, and can be added to the symtab
  // without causing errors.  Now add all tmptab defs to the symtab,
  // overwriting (and releasing a ref on) any existing defs with the same
  // names.  Ownership for tmptab defs passes from the tmptab to the symtab.
  upb_symtab_ent *tmptab_e;
  for(tmptab_e = upb_strtable_begin(&tmptab); tmptab_e;
      tmptab_e = upb_strtable_next(&tmptab, &tmptab_e->e)) {
    upb_symtab_ent *symtab_e =
        upb_strtable_lookup(&s->symtab, tmptab_e->def->fqname);
    if(symtab_e) {
      upb_def_unref(symtab_e->def);
      symtab_e->def = tmptab_e->def;
    } else {
      upb_strtable_insert(&s->symtab, &tmptab_e->e);
    }
  }

  upb_rwlock_unlock(&s->lock);
  upb_strtable_free(&tmptab);
  return true;

err:
  // We need to free all defs from "tmptab."
  upb_rwlock_unlock(&s->lock);
  for(upb_symtab_ent *e = upb_strtable_begin(&tmptab); e;
      e = upb_strtable_next(&tmptab, &e->e)) {
    upb_def_unref(e->def);
  }
  upb_strtable_free(&tmptab);
  return false;
}


/* upb_symtab public interface ************************************************/

upb_symtab *upb_symtab_new()
{
  upb_symtab *s = malloc(sizeof(*s));
  upb_atomic_init(&s->refcount, 1);
  upb_rwlock_init(&s->lock);
  upb_strtable_init(&s->symtab, 16, sizeof(upb_symtab_ent));
  s->fds_msgdef = NULL;
  return s;
}

static void upb_free_symtab(upb_strtable *t)
{
  upb_symtab_ent *e;
  for(e = upb_strtable_begin(t); e; e = upb_strtable_next(t, &e->e))
    upb_def_unref(e->def);
  upb_strtable_free(t);
}

void _upb_symtab_free(upb_symtab *s)
{
  upb_free_symtab(&s->symtab);
  upb_rwlock_destroy(&s->lock);
  free(s);
}

upb_def **upb_symtab_getdefs(upb_symtab *s, int *count, upb_deftype_t type)
{
  upb_rwlock_rdlock(&s->lock);
  int total = upb_strtable_count(&s->symtab);
  // We may only use part of this, depending on how many symbols are of the
  // correct type.
  upb_def **defs = malloc(sizeof(*defs) * total);
  upb_symtab_ent *e = upb_strtable_begin(&s->symtab);
  int i = 0;
  for(; e; e = upb_strtable_next(&s->symtab, &e->e)) {
    upb_def *def = e->def;
    assert(def);
    if(type == UPB_DEF_ANY || def->type == type)
      defs[i++] = def;
  }
  upb_rwlock_unlock(&s->lock);
  *count = i;
  for(i = 0; i < *count; i++)
    upb_def_ref(defs[i]);
  return defs;
}

upb_def *upb_symtab_lookup(upb_symtab *s, upb_string *sym)
{
  upb_rwlock_rdlock(&s->lock);
  upb_symtab_ent *e = upb_strtable_lookup(&s->symtab, sym);
  upb_def *ret = NULL;
  if(e) {
    ret = e->def;
    upb_def_ref(ret);
  }
  upb_rwlock_unlock(&s->lock);
  return ret;
}


upb_def *upb_symtab_resolve(upb_symtab *s, upb_string *base, upb_string *symbol) {
  upb_rwlock_rdlock(&s->lock);
  upb_symtab_ent *e = upb_resolve(&s->symtab, base, symbol);
  upb_def *ret = NULL;
  if(e) {
    ret = e->def;
    upb_def_ref(ret);
  }
  upb_rwlock_unlock(&s->lock);
  return ret;
}
