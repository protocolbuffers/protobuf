/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2008-2009 Joshua Haberman.  See LICENSE for details.
 */

#include <stdlib.h>
#include "descriptor_const.h"
#include "descriptor.h"
#include "upb_def.h"

#define CHECKSRC(x) if(!(x)) goto src_err
#define CHECK(x) if(!(x)) goto err

// A little dynamic array for storing a growing list of upb_defs.
typedef struct {
  upb_def **defs;
  uint32_t len;
  uint32_t size;
} upb_deflist;

static void upb_deflist_init(upb_deflist *l) {
  l->size = 8;
  l->defs = malloc(l->size);
  l->len = 0;
}

static void upb_deflist_uninit(upb_deflist *l) {
  for(uint32_t i = 0; i < l->len; i++)
    if(l->defs[i]) upb_def_unref(l->defs[i]);
  free(l->defs);
}

static void upb_deflist_push(upb_deflist *l, upb_def *d) {
  if(l->len == l->size) {
    l->size *= 2;
    l->defs = realloc(l->defs, l->size);
  }
  l->defs[l->len++] = d;
}

/* Joins strings together, for example:
 *   join("Foo.Bar", "Baz") -> "Foo.Bar.Baz"
 *   join("", "Baz") -> "Baz"
 * Caller owns a ref on the returned string. */
static upb_string *upb_join(upb_string *base, upb_string *name) {
  if (upb_string_len(base) == 0) {
    return upb_string_getref(name);
  } else {
    return upb_string_asprintf(UPB_STRFMT "." UPB_STRFMT,
                               UPB_STRARG(base), UPB_STRARG(name));
  }
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
    for(int i = 0; i < m->num_fields; i++) {
      upb_fielddef *f = &m->fields[i];
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

static void upb_def_init(upb_def *def, upb_def_type type) {
  def->type = type;
  def->is_cyclic = 0;  // We detect this later, after resolving refs.
  def->search_depth = 0;
  def->fqname = NULL;
  upb_atomic_refcount_init(&def->refcount, 1);
}

static void upb_def_uninit(upb_def *def) {
  upb_string_unref(def->fqname);
}


/* upb_unresolveddef **********************************************************/

// Unresolved defs are used as temporary placeholders for a def whose name has
// not been resolved yet.  During the name resolution step, all unresolved defs
// are replaced with pointers to the actual def being referenced.
typedef struct _upb_unresolveddef {
  upb_def base;

  // The target type name.  This may or may not be fully qualified.
  upb_string *name;
} upb_unresolveddef;

// Is passed a ref on the string.
static upb_unresolveddef *upb_unresolveddef_new(upb_string *str) {
  upb_unresolveddef *def = malloc(sizeof(*def));
  upb_def_init(&def->base, UPB_DEF_UNRESOLVED);
  def->name = str;
  return def;
}

static void upb_unresolveddef_free(struct _upb_unresolveddef *def) {
  upb_def_uninit(&def->base);
  free(def);
}


/* upb_enumdef ****************************************************************/

typedef struct {
  upb_strtable_entry e;
  uint32_t value;
} ntoi_ent;

typedef struct {
  upb_inttable_entry e;
  upb_string *string;
} iton_ent;

static void upb_enumdef_free(upb_enumdef *e) {
  upb_strtable_free(&e->ntoi);
  upb_inttable_free(&e->iton);
  upb_def_uninit(&e->base);
  free(e);
}

static bool upb_addenum_val(upb_src *src, upb_enumdef *e, upb_status *status)
{
  int32_t number = -1;
  upb_string *name = NULL;
  upb_fielddef *f;
  while((f = upb_src_getdef(src)) != NULL) {
    switch(f->number) {
      case GOOGLE_PROTOBUF_ENUMVALUEDESCRIPTORPROTO_NUMBER_FIELDNUM:
        CHECKSRC(upb_src_getint32(src, &number));
        break;
      case GOOGLE_PROTOBUF_ENUMVALUEDESCRIPTORPROTO_NAME_FIELDNUM:
        name = upb_string_tryrecycle(name);
        CHECKSRC(upb_src_getstr(src, name));
        break;
      default:
        CHECKSRC(upb_src_skipval(src));
        break;
    }
  }

  if(name == NULL || number == -1) {
    upb_seterr(status, UPB_STATUS_ERROR, "Enum value missing name or number.");
    goto err;
  }
  ntoi_ent ntoi_ent = {{name, 0}, number};
  iton_ent iton_ent = {{number, 0}, name};
  upb_strtable_insert(&e->ntoi, &ntoi_ent.e);
  upb_inttable_insert(&e->iton, &iton_ent.e);
  // We don't unref "name" because we pass our ref to the iton entry of the
  // table.  strtables can ref their keys, but the inttable doesn't know that
  // the value is a string.
  return true;

src_err:
  upb_copyerr(status, upb_src_status(src));
err:
  upb_string_unref(name);
  return false;
}

static bool upb_addenum(upb_src *src, upb_deflist *defs, upb_status *status)
{
  upb_enumdef *e = malloc(sizeof(*e));
  upb_def_init(&e->base, UPB_DEF_ENUM);
  upb_strtable_init(&e->ntoi, 0, sizeof(ntoi_ent));
  upb_inttable_init(&e->iton, 0, sizeof(iton_ent));
  upb_fielddef *f;
  while((f = upb_src_getdef(src)) != NULL) {
    switch(f->number) {
      case GOOGLE_PROTOBUF_ENUMDESCRIPTORPROTO_VALUE_FIELDNUM:
        CHECK(upb_addenum_val(src, e, status));
        break;
      default:
        upb_src_skipval(src);
        break;
    }
  }
  upb_deflist_push(defs, UPB_UPCAST(e));
  return true;

err:
  upb_enumdef_free(e);
  return false;
}

static void fill_iter(upb_enum_iter *iter, ntoi_ent *ent) {
  iter->state = ent;
  iter->name = ent->e.key;
  iter->val = ent->value;
}

void upb_enum_begin(upb_enum_iter *iter, upb_enumdef *e) {
  // We could iterate over either table here; the choice is arbitrary.
  ntoi_ent *ent = upb_strtable_begin(&e->ntoi);
  iter->e = e;
  fill_iter(iter, ent);
}

void upb_enum_next(upb_enum_iter *iter) {
  ntoi_ent *ent = iter->state;
  assert(ent);
  ent = upb_strtable_next(&iter->e->ntoi, &ent->e);
  iter->state = ent;
  if(ent) fill_iter(iter, ent);
}

bool upb_enum_done(upb_enum_iter *iter) {
  return iter->state == NULL;
}


/* upb_fielddef ***************************************************************/

static void upb_fielddef_free(upb_fielddef *f) {
  free(f);
}

static void upb_fielddef_uninit(upb_fielddef *f) {
  upb_string_unref(f->name);
  if(upb_hasdef(f) && f->owned) {
    upb_def_unref(f->def);
  }
}

static bool upb_addfield(upb_src *src, upb_msgdef *m, upb_status *status)
{
  upb_fielddef *f = malloc(sizeof(*f));
  f->def = NULL;
  f->owned = false;
  upb_fielddef *parsed_f;
  int32_t tmp;
  while((parsed_f = upb_src_getdef(src))) {
    switch(parsed_f->number) {
      case GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_FIELDNUM:
        CHECKSRC(upb_src_getint32(src, &tmp));
        f->type = tmp;
        break;
      case GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_LABEL_FIELDNUM:
        CHECKSRC(upb_src_getint32(src, &tmp));
        f->label = tmp;
        break;
      case GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_NUMBER_FIELDNUM:
        CHECKSRC(upb_src_getint32(src, &tmp));
        f->number = tmp;
        break;
      case GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_NAME_FIELDNUM:
        f->name = upb_string_tryrecycle(f->name);
        CHECKSRC(upb_src_getstr(src, f->name));
        break;
      case GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_NAME_FIELDNUM: {
        upb_string *str = upb_string_new();
        CHECKSRC(upb_src_getstr(src, str));
        if(f->def) upb_def_unref(f->def);
        f->def = UPB_UPCAST(upb_unresolveddef_new(str));
        f->owned = true;
        break;
      }
    }
  }
  CHECKSRC(upb_src_eof(src));
  // TODO: verify that all required fields were present.
  assert((f->def != NULL) == upb_hasdef(f));

  // Field was successfully read, add it as a field of the msgdef.
  upb_itof_ent itof_ent = {{f->number, 0}, f};
  upb_ntof_ent ntof_ent = {{f->name, 0}, f};
  upb_inttable_insert(&m->itof, &itof_ent.e);
  upb_strtable_insert(&m->ntof, &ntof_ent.e);
  return true;

src_err:
  upb_copyerr(status, upb_src_status(src));
  upb_fielddef_free(f);
  return false;
}


/* upb_msgdef *****************************************************************/

// Processes a google.protobuf.DescriptorProto, adding defs to "defs."
static bool upb_addmsg(upb_src *src, upb_deflist *defs, upb_status *status)
{
  upb_msgdef *m = malloc(sizeof(*m));
  upb_def_init(&m->base, UPB_DEF_MSG);
  upb_atomic_refcount_init(&m->cycle_refcount, 0);
  upb_inttable_init(&m->itof, 4, sizeof(upb_itof_ent));
  upb_strtable_init(&m->ntof, 4, sizeof(upb_ntof_ent));
  int32_t start_count = defs->len;

  upb_fielddef *f;
  while((f = upb_src_getdef(src)) != NULL) {
    switch(f->number) {
      case GOOGLE_PROTOBUF_DESCRIPTORPROTO_NAME_FIELDNUM:
        m->base.fqname = upb_string_tryrecycle(m->base.fqname);
        CHECKSRC(upb_src_getstr(src, m->base.fqname));
        break;
      case GOOGLE_PROTOBUF_DESCRIPTORPROTO_FIELD_FIELDNUM:
        CHECKSRC(upb_src_startmsg(src));
        CHECK(upb_addfield(src, m, status));
        CHECKSRC(upb_src_endmsg(src));
        break;
      case GOOGLE_PROTOBUF_DESCRIPTORPROTO_NESTED_TYPE_FIELDNUM:
        CHECKSRC(upb_src_startmsg(src));
        CHECK(upb_addmsg(src, defs, status));
        CHECKSRC(upb_src_endmsg(src));
        break;
      case GOOGLE_PROTOBUF_DESCRIPTORPROTO_ENUM_TYPE_FIELDNUM:
        CHECKSRC(upb_src_startmsg(src));
        CHECK(upb_addenum(src, defs, status));
        CHECKSRC(upb_src_endmsg(src));
        break;
      default:
        // TODO: extensions.
        CHECKSRC(upb_src_skipval(src));
    }
  }
  CHECK(upb_src_eof(src));
  if(!m->base.fqname) {
    upb_seterr(status, UPB_STATUS_ERROR, "Encountered message with no name.");
    goto err;
  }
  upb_deflist_qualify(defs, m->base.fqname, start_count);
  upb_deflist_push(defs, UPB_UPCAST(m));
  return true;

src_err:
  upb_copyerr(status, upb_src_status(src));
err:
  upb_msgdef_free(m);
  return false;
}

static void upb_msgdef_free(upb_msgdef *m)
{
  for (upb_field_count_t i = 0; i < m->num_fields; i++)
    upb_fielddef_uninit(&m->fields[i]);
  free(m->fields);
  upb_strtable_free(&m->ntof);
  upb_inttable_free(&m->itof);
  upb_def_uninit(&m->base);
  free(m);
}

static void upb_msgdef_resolve(upb_msgdef *m, upb_fielddef *f, upb_def *def) {
  (void)m;
  if(f->owned) upb_def_unref(f->def);
  f->def = def;
  // We will later make the ref unowned if it is a part of a cycle.
  f->owned = true;
  upb_def_ref(def);
}


/* symtab internal  ***********************************************************/

// Processes a google.protobuf.FileDescriptorProto, adding the defs to "defs".
static bool upb_addfd(upb_src *src, upb_deflist *defs, upb_status *status)
{
  upb_string *package = NULL;
  int32_t start_count = defs->len;
  upb_fielddef *f;
  while((f = upb_src_getdef(src)) != NULL) {
    switch(f->number) {
      case GOOGLE_PROTOBUF_FILEDESCRIPTORPROTO_NAME_FIELDNUM:
        package = upb_string_tryrecycle(package);
        CHECKSRC(upb_src_getstr(src, package));
        break;
      case GOOGLE_PROTOBUF_FILEDESCRIPTORPROTO_MESSAGE_TYPE_FIELDNUM:
        CHECKSRC(upb_src_startmsg(src));
        CHECK(upb_addmsg(src, defs, status));
        CHECKSRC(upb_src_endmsg(src));
        break;
      case GOOGLE_PROTOBUF_FILEDESCRIPTORPROTO_ENUM_TYPE_FIELDNUM:
        CHECKSRC(upb_src_startmsg(src));
        CHECK(upb_addenum(src, defs, status));
        CHECKSRC(upb_src_endmsg(src));
        break;
      default:
        // TODO: services and extensions.
        CHECKSRC(upb_src_skipval(src));
    }
  }
  CHECK(upb_src_eof(src));
  upb_deflist_qualify(defs, package, start_count);
  upb_string_unref(package);
  return true;

src_err:
  upb_copyerr(status, upb_src_status(src));
err:
  upb_string_unref(package);
  return false;
}

/* Search for a character in a string, in reverse. */
static int my_memrchr(char *data, char c, size_t len)
{
  int off = len-1;
  while(off > 0 && data[off] != c) --off;
  return off;
}

typedef struct {
  upb_strtable_entry e;
  upb_def *def;
} upb_symtab_ent;

// Given a symbol and the base symbol inside which it is defined, find the
// symbol's definition in t.
static upb_symtab_ent *upb_resolve(upb_strtable *t,
                                   upb_string *base, upb_string *sym)
{
  if(upb_string_len(base) + upb_string_len(sym) + 1 >= UPB_SYMBOL_MAXLEN ||
     upb_string_len(sym) == 0) return NULL;

  if(upb_string_getrobuf(sym)[0] == UPB_SYMBOL_SEPARATOR) {
    // Symbols starting with '.' are absolute, so we do a single lookup.
    // Slice to omit the leading '.'
    upb_string *sym_str = upb_strslice(sym, 1, upb_string_len(sym) - 1);
    upb_symtab_ent *e = upb_strtable_lookup(t, sym_str);
    upb_string_unref(sym_str);
    return e;
  } else {
    // Remove components from base until we find an entry or run out.
    upb_string *sym_str = upb_string_new();
    int baselen = upb_string_len(base);
    while(1) {
      // sym_str = base[0...base_len] + UPB_SYMBOL_SEPARATOR + sym
      upb_strlen_t len = baselen + upb_string_len(sym) + 1;
      char *buf = upb_string_getrwbuf(sym_str, len);
      memcpy(buf, upb_string_getrobuf(base), baselen);
      buf[baselen] = UPB_SYMBOL_SEPARATOR;
      memcpy(buf + baselen + 1, upb_string_getrobuf(sym), upb_string_len(sym));

      upb_symtab_ent *e = upb_strtable_lookup(t, sym_str);
      if (e) return e;
      else if(baselen == 0) return NULL;  // No more scopes to try.

      baselen = my_memrchr(buf, UPB_SYMBOL_SEPARATOR, baselen);
    }
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
    upb_seterr(status, UPB_STATUS_ERROR, "Type " UPB_STRFMT " was found at "
               "depth %d in the type graph, which exceeds the maximum type "
               "depth of %d.", UPB_UPCAST(m)->fqname, depth,
               UPB_MAX_TYPE_DEPTH);
    return false;
  } else if(UPB_UPCAST(m)->search_depth == 1) {
    // Cycle!
    int cycle_len = depth - 1;
    if(cycle_len > UPB_MAX_TYPE_CYCLE_LEN) {
      upb_seterr(status, UPB_STATUS_ERROR, "Type " UPB_STRFMT " was involved "
                 "in a cycle of length %d, which exceeds the maximum type "
                 "cycle length of %d.", UPB_UPCAST(m)->fqname, cycle_len,
                 UPB_MAX_TYPE_CYCLE_LEN);
    }
    return true;
  } else if(UPB_UPCAST(m)->search_depth > 0) {
    // This was a cycle, but did not originate from the base of our search tree.
    // We'll find it when we call find_cycles() on this node directly.
    return false;
  } else {
    UPB_UPCAST(m)->search_depth = ++depth;
    bool cycle_found = false;
    for(upb_field_count_t i = 0; i < m->num_fields; i++) {
      upb_fielddef *f = &m->fields[i];
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
// resolves all of the unresolved refs for the defs in tmptab.
bool upb_resolverefs(upb_strtable *tmptab, upb_strtable *symtab,
                     upb_status *status)
{
  upb_symtab_ent *e;
  for(e = upb_strtable_begin(tmptab); e; e = upb_strtable_next(tmptab, &e->e)) {
    upb_msgdef *m = upb_dyncast_msgdef(e->def);
    if(!m) continue;
    // Type names are resolved relative to the message in which they appear.
    upb_string *base = e->e.key;

    for(upb_field_count_t i = 0; i < m->num_fields; i++) {
      upb_fielddef *f = &m->fields[i];
      if(!upb_hasdef(f)) continue;  // No resolving necessary.
      upb_string *name = upb_downcast_unresolveddef(f->def)->name;

      // Resolve from either the tmptab (pending adds) or symtab (existing
      // defs).  If both exist, prefer the pending add, because it will be
      // overwriting the existing def.
      upb_symtab_ent *found;
      if(!(found = upb_resolve(tmptab, base, name)) &&
         !(found = upb_resolve(symtab, base, name))) {
        upb_seterr(status, UPB_STATUS_ERROR,
                   "could not resolve symbol '" UPB_STRFMT "'"
                   " in context '" UPB_STRFMT "'",
                   UPB_STRARG(name), UPB_STRARG(base));
        return false;
      }

      // Check the type of the found def.
      upb_field_type_t expected = upb_issubmsg(f) ? UPB_DEF_MSG : UPB_DEF_ENUM;
      if(found->def->type != expected) {
        upb_seterr(status, UPB_STATUS_ERROR, "Unexpected type");
        return false;
      }
      upb_msgdef_resolve(m, f, found->def);
    }
  }

  // Deal with type cycles.
  for(e = upb_strtable_begin(tmptab); e; e = upb_strtable_next(tmptab, &e->e)) {
    upb_msgdef *m = upb_dyncast_msgdef(e->def);
    if(!m) continue;
    // The findcycles() call will decrement the external refcount of the
    if(!upb_symtab_findcycles(m, 0, status)) return false;
    upb_msgdef *open_defs[UPB_MAX_TYPE_CYCLE_LEN];
    upb_cycle_ref_or_unref(m, NULL, open_defs, 0, true);
  }

  return true;
}

// Given a list of defs, a list of extensions (in the future), and a flag
// indicating whether the new defs can overwrite existing defs in the symtab,
// attempts to add the given defs to the symtab.  The whole operation either
// succeeds or fails.  Ownership of "defs" and "exts" is taken.
bool upb_symtab_add_defs(upb_symtab *s, upb_deflist *defs, bool allow_redef,
                         upb_status *status)
{
  upb_rwlock_wrlock(&s->lock);

  // Build a table of the defs we mean to add, for duplicate detection and name
  // resolution.
  upb_strtable tmptab;
  upb_strtable_init(&tmptab, defs->len, sizeof(upb_symtab_ent));
  for (uint32_t i = 0; i < defs->len; i++) {
    upb_def *def = defs->defs[i];
    upb_symtab_ent e = {{def->fqname, 0}, def};

    // Redefinition is never allowed within a single FileDescriptorSet.
    // Additionally, we only allow overwriting of an existing definition if
    // allow_redef is set.
    if (upb_strtable_lookup(&tmptab, def->fqname) ||
        (!allow_redef && upb_strtable_lookup(&s->symtab, def->fqname))) {
      upb_seterr(status, UPB_STATUS_ERROR, "Redefinition of symbol " UPB_STRFMT,
                 UPB_STRARG(def->fqname));
      goto err;
    }

    // Pass ownership from the deflist to the strtable.
    upb_strtable_insert(&tmptab, &e.e);
    defs->defs[i] = NULL;
  }

  // TODO: process the list of extensions by modifying entries from
  // tmptab in-place (copying them from the symtab first if necessary).

  CHECK(upb_resolverefs(&tmptab, &s->symtab, status));

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
      e = upb_strtable_next(&tmptab, &e->e))
    upb_def_unref(e->def);
  upb_strtable_free(&tmptab);
  return false;
}


/* upb_symtab *****************************************************************/

upb_symtab *upb_symtab_new()
{
  upb_symtab *s = malloc(sizeof(*s));
  upb_atomic_refcount_init(&s->refcount, 1);
  upb_rwlock_init(&s->lock);
  upb_strtable_init(&s->symtab, 16, sizeof(upb_symtab_ent));
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

upb_def **upb_symtab_getdefs(upb_symtab *s, int *count, upb_def_type_t type)
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

void upb_symtab_addfds(upb_symtab *s, upb_src *src, upb_status *status)
{
  upb_deflist defs;
  upb_deflist_init(&defs);
  upb_fielddef *f;
  while((f = upb_src_getdef(src)) != NULL) {
    switch(f->number) {
      case GOOGLE_PROTOBUF_FILEDESCRIPTORSET_FILE_FIELDNUM:
        CHECKSRC(upb_src_startmsg(src));
        CHECK(upb_addfd(src, &defs, status));
        CHECKSRC(upb_src_endmsg(src));
        break;
      default:
        CHECKSRC(upb_src_skipval(src));
    }
  }
  CHECKSRC(upb_src_eof(src));
  CHECK(upb_symtab_add_defs(s, &defs, false, status));
  upb_deflist_uninit(&defs);
  return;

src_err:
  upb_copyerr(status, upb_src_status(src));
err:
  upb_deflist_uninit(&defs);
}


/* upb_baredecoder ************************************************************/

// upb_baredecoder is a upb_src that can parse a subset of the protocol buffer
// binary format.  It is only used for bootstrapping.  It can parse without
// having a upb_msgdef, which is why it is useful for bootstrapping the first
// msgdef.  On the downside, it does not support:
//
// * having its input span multiple upb_strings.
// * reading any field of the returned upb_fielddef's except f->number.
// * keeping a pointer to the upb_fielddef* and reading it later (the same
//   upb_fielddef is reused over and over).
// * detecting errors in the input (we trust that our input is known-good).
//
// It also does not support any of the follow protobuf features:
// * packed fields.
// * groups.
// * zig-zag-encoded types like sint32 and sint64.
//
// If descriptor.proto ever changed to use any of these features, this decoder
// would need to be extended to support them.

typedef struct {
  upb_src src;
  upb_string *input;
  upb_strlen_t offset;
  upb_fielddef field;
  upb_wire_type_t wire_type;
  upb_strlen_t delimited_len;
  upb_strlen_t stack[UPB_MAX_NESTING], *top;
  upb_string *str;
} upb_baredecoder;

static uint64_t upb_baredecoder_readv64(upb_baredecoder *d)
{
  const uint8_t *start = (uint8_t*)upb_string_getrobuf(d->input) + d->offset;
  const uint8_t *buf = start;
  uint8_t last = 0x80;
  uint64_t val = 0;
  for(int bitpos = 0; (last & 0x80); buf++, bitpos += 7)
    val |= ((uint64_t)((last = *buf) & 0x7F)) << bitpos;
  d->offset += buf - start;
  return val;
}

static uint32_t upb_baredecoder_readv32(upb_baredecoder *d)
{
  return (uint32_t)upb_baredecoder_readv64(d); // Truncate.
}

static uint64_t upb_baredecoder_readf64(upb_baredecoder *d)
{
  uint64_t val;
  memcpy(&val, upb_string_getrobuf(d->input) + d->offset, 8);
  d->offset += 8;
  return val;
}

static uint32_t upb_baredecoder_readf32(upb_baredecoder *d)
{
  uint32_t val;
  memcpy(&val, upb_string_getrobuf(d->input) + d->offset, 4);
  d->offset += 4;
  return val;
}

static upb_fielddef *upb_baredecoder_getdef(upb_baredecoder *d)
{
  // Detect end-of-submessage.
  if(d->offset >= *d->top) {
    d->src.eof = true;
    return NULL;
  }

  uint32_t key;
  key = upb_baredecoder_readv32(d);
  d->wire_type = key & 0x7;
  d->field.number = key >> 3;
  if(d->wire_type == UPB_WIRE_TYPE_DELIMITED) {
    // For delimited wire values we parse the length now, since we need it in
    // all cases.
    d->delimited_len = upb_baredecoder_readv32(d);
  }
  return &d->field;
}

static bool upb_baredecoder_getval(upb_baredecoder *d, upb_valueptr val)
{
  switch(d->wire_type) {
    case UPB_WIRE_TYPE_VARINT:
      *val.uint64 = upb_baredecoder_readv64(d);
      break;
    case UPB_WIRE_TYPE_32BIT_VARINT:
      *val.uint32 = upb_baredecoder_readv32(d);
      break;
    case UPB_WIRE_TYPE_64BIT:
      *val.uint64 = upb_baredecoder_readf64(d);
      break;
    case UPB_WIRE_TYPE_32BIT:
      *val.uint32 = upb_baredecoder_readf32(d);
      break;
    default:
      assert(false);
  }
  return true;
}

static bool upb_baredecoder_getstr(upb_baredecoder *d, upb_string *str) {
  upb_string_substr(str, d->input, d->offset, d->delimited_len);
  return true;
}

static bool upb_baredecoder_skipval(upb_baredecoder *d)
{
  upb_value val;
  return upb_baredecoder_getval(d, upb_value_addrof(&val));
}

static bool upb_baredecoder_startmsg(upb_baredecoder *d)
{
  *(d->top++) = d->offset + d->delimited_len;
  return true;
}

static bool upb_baredecoder_endmsg(upb_baredecoder *d)
{
  d->offset = *(--d->top);
  return true;
}

static upb_src_vtable upb_baredecoder_src_vtbl = {
  (upb_src_getdef_fptr)&upb_baredecoder_getdef,
  (upb_src_getval_fptr)&upb_baredecoder_getval,
  (upb_src_getstr_fptr)&upb_baredecoder_getstr,
  (upb_src_skipval_fptr)&upb_baredecoder_skipval,
  (upb_src_startmsg_fptr)&upb_baredecoder_startmsg,
  (upb_src_endmsg_fptr)&upb_baredecoder_endmsg,
};

static upb_baredecoder *upb_baredecoder_new(upb_string *str)
{
  upb_baredecoder *d = malloc(sizeof(*d));
  d->input = upb_string_getref(str);
  d->str = upb_string_new();
  d->top = &d->stack[0];
  upb_src_init(&d->src, &upb_baredecoder_src_vtbl);
  return d;
}

static void upb_baredecoder_free(upb_baredecoder *d)
{
  upb_string_unref(d->input);
  upb_string_unref(d->str);
  free(d);
}

static upb_src *upb_baredecoder_src(upb_baredecoder *d)
{
  return &d->src;
}

upb_symtab *upb_get_descriptor_symtab()
{
  // TODO: implement sharing of symtabs, so that successive calls to this
  // function will return the same symtab.
  upb_symtab *symtab = upb_symtab_new();
  // TODO: allow upb_strings to be static or on the stack.
  upb_string *descriptor = upb_strduplen(descriptor_pb, descriptor_pb_len);
  upb_baredecoder *decoder = upb_baredecoder_new(descriptor);
  upb_status status;
  upb_symtab_addfds(symtab, upb_baredecoder_src(decoder), &status);
  assert(upb_ok(&status));
  upb_baredecoder_free(decoder);
  upb_string_unref(descriptor);
  return symtab;
}
