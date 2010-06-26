/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2008-2009 Joshua Haberman.  See LICENSE for details.
 */

#include <stdlib.h>
#include "descriptor_const.h"
#include "upb_def.h"

/* Rounds p up to the next multiple of t. */
#define ALIGN_UP(p, t) ((p) % (t) == 0 ? (p) : (p) + ((t) - ((p) % (t))))

static int div_round_up(int numerator, int denominator) {
  /* cf. http://stackoverflow.com/questions/17944/how-to-round-up-the-result-of-integer-division */
  return numerator > 0 ? (numerator - 1) / denominator + 1 : 0;
}

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

static void upb_deflist_uninit(upb_deflist *l) { free(l->defs); }

static void upb_deflist_push(upb_deflist *l, upb_def *d) {
  if(l->len == l->size) {
    l->size *= 2;
    l->defs = realloc(l->defs, l->size);
  }
  l->defs[l->len++] = d;
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

static void msgdef_free(upb_msgdef *m);
static void enumdef_free(upb_enumdef *e);
static void unresolveddef_free(struct _upb_unresolveddef *u);

static void def_free(upb_def *def)
{
  switch(def->type) {
    case UPB_DEF_MSG:
      msgdef_free(upb_downcast_msgdef(def));
      break;
    case UPB_DEF_ENUM:
      enumdef_free(upb_downcast_enumdef(def));
      break;
    case UPB_DEF_SVC:
      assert(false);  /* Unimplemented. */
      break;
    case UPB_DEF_EXT:
      assert(false);  /* Unimplemented. */
      break;
    case UPB_DEF_UNRESOLVED:
      unresolveddef_free(upb_downcast_unresolveddef(def));
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
static int cycle_ref_or_unref(upb_msgdef *m, upb_msgdef *cycle_base,
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
        path_count += cycle_ref_or_unref(sub_m, cycle_base, open_defs,
                                         num_open_defs, ref);
      }
    }
    if(ref) {
      upb_atomic_add(&m->cycle_refcount, path_count);
    } else {
      if(upb_atomic_add(&m->cycle_refcount, -path_count))
        def_free(UPB_UPCAST(m));
    }
    return path_count;
  }
}

void _upb_def_reftozero(upb_def *def) {
  if(def->is_cyclic) {
    upb_msgdef *m = upb_downcast_msgdef(def);
    upb_msgdef *open_defs[UPB_MAX_TYPE_CYCLE_LEN];
    cycle_ref_or_unref(m, NULL, open_defs, 0, false);
  } else {
    def_free(def);
  }
}

void _upb_def_cyclic_ref(upb_def *def) {
  upb_msgdef *open_defs[UPB_MAX_TYPE_CYCLE_LEN];
  cycle_ref_or_unref(upb_downcast_msgdef(def), NULL, open_defs, 0, true);
}

static void upb_def_init(upb_def *def, upb_def_type type, upb_string *fqname) {
  def->type = type;
  def->is_cyclic = 0;  // We detect this later, after resolving refs.
  def->search_depth = 0;
  def->fqname = fqname;
  upb_atomic_refcount_init(&def->refcount, 1);
}

static void upb_def_uninit(upb_def *def) {
  upb_string_unref(def->fqname);
}

/* upb_unresolveddef **********************************************************/

typedef struct _upb_unresolveddef {
  upb_def base;
} upb_unresolveddef;

static upb_unresolveddef *upb_unresolveddef_new(upb_string *str) {
  upb_unresolveddef *def = malloc(sizeof(*def));
  upb_def_init(&def->base, UPB_DEF_UNRESOLVED, str);
  return def;
}

static void unresolveddef_free(struct _upb_unresolveddef *def) {
  upb_def_uninit(&def->base);
  free(def);
}

/* upb_fielddef ***************************************************************/

static upb_fielddef *fielddef_new(upb_src *src)
{
  upb_fielddef *f = malloc(sizeof(*f));
  f->def = NULL;
  f->owned = false;
  upb_src_startmsg(src);
  upb_fielddef *parsed_f;
  while((parsed_f = upb_src_getdef(src))) {
    switch(parsed_f->number) {
      case GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_FIELDNUM:
        CHECK(upb_src_getval(src, &f->type));
      case GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_LABEL_FIELDNUM:
        CHECK(upb_src_getval(src, &f->label));
      case GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_NUMBER_FIELDNUM:
        CHECK(upb_src_getval(src, &f->number));
      case GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_NAME_FIELDNUM:
        CHECK(upb_src_getval(src, &f->name));
      case GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPENAME_FIELDNUM:
        CHECK(upb_src_getval(src, &f->type_name));
        f->def = UPB_UPCAST(upb_unresolveddef_new(fd->type_name));
        f->owned = true;
    }
  }
  upb_src_endmsg(src);
  assert((f->def != NULL) == upb_hasdef(f));
  return f;
}

static void fielddef_free(upb_fielddef *f) {
  upb_string_unref(f->name);
  if(upb_hasdef(f) && f->owned) {
    upb_def_unref(f->def);
  }
  free(f);
}

/* upb_msgdef *****************************************************************/

// Processes a google.protobuf.DescriptorProto, adding defs to "deflist."
static void upb_addmsg(upb_src *src, upb_deflist *deflist, upb_status *status)
{
  upb_msgdef *m = malloc(sizeof(*m));
  upb_def_init(&m->base, UPB_DEF_MSG);
  upb_atomic_refcount_init(&m->cycle_refcount, 0);
  upb_inttable_init(&m->itof, num_fields, sizeof(upb_itof_ent));
  upb_strtable_init(&m->ntof, num_fields, sizeof(upb_ntof_ent));
  m->num_fields = 0;
  m->fields = malloc(sizeof(upb_fielddef) * num_fields);
  int32_t start_count = defs->len;

  CHECK(upb_src_startmsg(src));
  upb_fielddef *f;
  while((f = upb_src_getdef(src)) != NULL) {
    switch(f->field_number) {
      case GOOGLE_PROTOBUF_FILEDESCRIPTORPROTO_NAME_FIELDNUM:
        upb_string_unref(m->fqname);
        CHECK(upb_src_getval(src, &m->fqname));
        break;
      case GOOGLE_PROTOBUF_FILEDESCRIPTORPROTO_FIELD_NUM:
        CHECK(upb_addfield(src, m));
        break;
      case GOOGLE_PROTOBUF_FILEDESCRIPTORPROTO_NESTED_TYPE_NUM:
        CHECK(upb_addmsg(src, deflist));
        break;
      case GOOGLE_PROTOBUF_FILEDESCRIPTORPROTO_ENUM_TYPE_NUM:
        CHECK(upb_addenum(src, deflist));
        break;
      default:
        // TODO: extensions.
        upb_src_skipval(src);
    }
  }
  CHECK(upb_src_eof(src) && upb_src_endmsg(src));
  if(!m->fqname) {
    upb_seterr(status, UPB_STATUS_ERROR, "Encountered message with no name.");
    return false;
  }
  upb_qualify(defs, m->fqname, start_count);
  upb_deflist_push(m);
  return true;
}

static void msgdef_free(upb_msgdef *m)
{
  for (upb_field_count_t i = 0; i < m->num_fields; i++)
    fielddef_uninit(&m->fields[i]);
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

/* upb_enumdef ****************************************************************/

typedef struct {
  upb_strtable_entry e;
  uint32_t value;
} ntoi_ent;

typedef struct {
  upb_inttable_entry e;
  upb_strptr string;
} iton_ent;

static void upb_addenum_val(upb_src *src, upb_enumdef *e, upb_status *status)
{
  upb_src_startmsg(src);
  int32_t number = -1;
  upb_string *name = NULL;
  while((f = upb_src_getdef(src)) != NULL) {
    switch(f->field_number) {
      case GOOGLE_PROTOBUF_ENUMVALUEDESCRIPTORPROTO_NUMBER_FIELDNUM:
        upb_src_getval(src, &number);
        break;
      case GOOGLE_PROTOBUF_ENUMVALUDESCRIPTORPROTO_NAME_FIELDNUM:
        upb_src_getval(src, &name);
        break;
      default:
        upb_src_skipval(src);
    }
  }
  upb_src_endmsg(src);
  ntoi_ent ntoi_ent = {{value->name, 0}, value->number};
  iton_ent iton_ent = {{value->number, 0}, value->name};
  upb_strtable_insert(&e->ntoi, &ntoi_ent.e);
  upb_inttable_insert(&e->iton, &iton_ent.e);
}

static void upb_addenum(upb_src *src, upb_deflist *defs, upb_status *status)
{
  upb_enumdef *e = malloc(sizeof(*e));
  upb_def_init(&e->base, UPB_DEF_ENUM, fqname);
  upb_strtable_init(&e->ntoi, 0, sizeof(ntoi_ent));
  upb_inttable_init(&e->iton, 0, sizeof(iton_ent));
  upb_fielddef *f;
  while((f = upb_src_getdef(src)) != NULL) {
    switch(f->field_number) {
      case GOOGLE_PROTOBUF_ENUMDESCRIPTORPROTO_VALUE_FIELDNUM:
        CHECK(upb_addenum_val(src, e, status));
        break;
      default:
        upb_src_skipval(src);
        break;
    }
  }
  upb_deflist_push(e);
}

static void enumdef_free(upb_enumdef *e) {
  upb_strtable_free(&e->ntoi);
  upb_inttable_free(&e->iton);
  upb_def_uninit(&e->base);
  free(e);
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

/* symtab internal  ***********************************************************/

// Processes a google.protobuf.FileDescriptorProto, adding the defs to "defs".
static void upb_addfd(upb_src *src, upb_deflist *defs, upb_status *status)
{
  upb_string *package = NULL;
  int32_t start_count = defs->len;
  upb_fielddef *f;
  while((f = upb_src_getdef(src)) != NULL) {
    switch(f->field_number) {
      case GOOGLE_PROTOBUF_FILEDESCRIPTORPROTO_NAME_FIELDNUM:
        upb_string_unref(package);
        CHECK(upb_src_getval(src, &package));
        break;
      case GOOGLE_PROTOBUF_FILEDESCRIPTORPROTO_MESSAGE_TYPE_NUM:
        CHECK(upb_startmsg(src));
        CHECK(upb_addmsg(src, defs));
        CHECK(upb_endmsg(src));
        break;
      case GOOGLE_PROTOBUF_FILEDESCRIPTORPROTO_ENUM_TYPE_NUM:
        CHECK(upb_startmsg(src));
        CHECK(upb_addenum(src, defs));
        CHECK(upb_endmsg(src));
        break;
      default:
        // TODO: services and extensions.
        upb_src_skipval(src);
    }
  }
  CHECK(upb_src_eof(src));
  upb_qualify(deflist, package, start_count);
  upb_string_unref(package);
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

/* Given a symbol and the base symbol inside which it is defined, find the
 * symbol's definition in t. */
static symtab_ent *resolve(upb_strtable *t, upb_strptr base, upb_strptr symbol)
{
  if(upb_strlen(base) + upb_strlen(symbol) + 1 >= UPB_SYMBOL_MAXLEN ||
     upb_strlen(symbol) == 0) return NULL;

  if(upb_string_getrobuf(symbol)[0] == UPB_SYMBOL_SEPARATOR) {
    // Symbols starting with '.' are absolute, so we do a single lookup.
    // Slice to omit the leading '.'
    upb_strptr sym_str = upb_strslice(symbol, 1, INT_MAX);
    symtab_ent *e = upb_strtable_lookup(t, sym_str);
    upb_string_unref(sym_str);
    return e;
  } else {
    // Remove components from base until we find an entry or run out.
    upb_strptr sym_str = upb_string_new();
    int baselen = upb_strlen(base);
    while(1) {
      // sym_str = base[0...base_len] + UPB_SYMBOL_SEPARATOR + symbol
      upb_strlen_t len = baselen + upb_strlen(symbol) + 1;
      char *buf = upb_string_getrwbuf(sym_str, len);
      memcpy(buf, upb_string_getrobuf(base), baselen);
      buf[baselen] = UPB_SYMBOL_SEPARATOR;
      memcpy(buf + baselen + 1, upb_string_getrobuf(symbol), upb_strlen(symbol));

      symtab_ent *e = upb_strtable_lookup(t, sym_str);
      if (e) return e;
      else if(baselen == 0) return NULL;  /* No more scopes to try. */

      baselen = my_memrchr(buf, UPB_SYMBOL_SEPARATOR, baselen);
    }
  }
}

/* Joins strings together, for example:
 *   join("Foo.Bar", "Baz") -> "Foo.Bar.Baz"
 *   join("", "Baz") -> "Baz"
 * Caller owns a ref on the returned string. */
static upb_string *upb_join(upb_string *base, upb_string *name) {
  upb_strptr joined = upb_strdup(base);
  upb_strlen_t len = upb_strlen(joined);
  if(len > 0) {
    upb_string_getrwbuf(joined, len + 1)[len] = UPB_SYMBOL_SEPARATOR;
  }
  upb_strcat(joined, name);
  return joined;
}

// Performs a pass over the type graph to find all cycles that include m.
static bool upb_symtab_findcycles(upb_msgdef *m, int search_depth, upb_status *status)
{
  if(search_depth > UPB_MAX_TYPE_DEPTH) {
    // We have found a non-cyclic path from the base of the type tree that
    // exceeds the maximum allowed depth.  There are many situations in upb
    // where we recurse over the type tree (like for example, right now) and an
    // absurdly deep tree could cause us to stack overflow on systems with very
    // limited stacks.
    upb_seterr(status, UPB_STATUS_ERROR, "Type " UPB_STRFMT " was found at "
               "depth %d in the type graph, which exceeds the maximum type "
               "depth of %d.", UPB_UPCAST(m)->fqname, search_depth,
               UPB_MAX_TYPE_DEPTH);
    return false;
  } else if(UPB_UPCAST(m)->search_depth == 1) {
    // Cycle!
    int cycle_len = search_depth - 1;
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
    UPB_UPCAST(m)->search_depth = ++search_depth;
    bool cycle_found = false;
    for(upb_field_count_t i = 0; i < m->num_fields; i++) {
      upb_fielddef *f = &m->fields[i];
      if(!upb_issubmsg(f)) continue;
      upb_def *sub_def = f->def;
      upb_msgdef *sub_m = upb_downcast_msgdef(sub_def);
      if(find_cycles(sub_m, search_depth, status)) {
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

// Given a list of defs, a list of extensions (in the future), and a flag
// indicating whether the new defs can overwrite existing defs in the symtab,
// attempts to add the given defs to the symtab.  The whole operation either
// succeeds or fails.  Ownership of "defs" and "exts" is taken.
bool upb_symtab_add_defs(upb_symtab *s, upb_deflist *defs, bool allow_redef,
                         upb_status *status)
{
  // Build a table, for duplicate detection and name resolution.

  {  // Write lock scope.
    // Attempt to resolve all references.
    symtab_ent *e;
    for(e = upb_strtable_begin(addto); e; e = upb_strtable_next(addto, &e->e)) {
      upb_msgdef *m = upb_dyncast_msgdef(e->def);
      if(!m) continue;
      upb_strptr base = e->e.key;
      for(upb_field_count_t i = 0; i < m->num_fields; i++) {
        upb_fielddef *f = &m->fields[i];
        if(!upb_hasdef(f)) continue;  // No resolving necessary.
        upb_strptr name = upb_downcast_unresolveddef(f->def)->name;
        symtab_ent *found = resolve(existingdefs, base, name);
        if(!found) found = resolve(addto, base, name);
        upb_field_type_t expected = upb_issubmsg(f) ? UPB_DEF_MSG : UPB_DEF_ENUM;
        if(!found) {
          upb_seterr(status, UPB_STATUS_ERROR,
                     "could not resolve symbol '" UPB_STRFMT "'"
                     " in context '" UPB_STRFMT "'",
                     UPB_STRARG(name), UPB_STRARG(base));
          return;
        } else if(found->def->type != expected) {
          upb_seterr(status, UPB_STATUS_ERROR, "Unexpected type");
          return;
        }
        upb_msgdef_resolve(m, f, found->def);
      }
    }

    // Deal with type cycles.
    for(e = upb_strtable_begin(addto); e; e = upb_strtable_next(addto, &e->e)) {
      upb_msgdef *m = upb_dyncast_msgdef(e->def);
      if(!m) continue;
      // The findcycles() call will decrement the external refcount of the
      upb_symtab_findcycles(m, 0, status);
      upb_msgdef *open_defs[UPB_MAX_TYPE_CYCLE_LEN];
      cycle_ref_or_unref(m, NULL, open_defs, 0, true);
    }

    // Add all defs to the symtab.
  }
}

/* upb_symtab *****************************************************************/

upb_symtab *upb_symtab_new()
{
  upb_symtab *s = malloc(sizeof(*s));
  upb_atomic_refcount_init(&s->refcount, 1);
  upb_rwlock_init(&s->lock);
  upb_strtable_init(&s->symtab, 16, sizeof(symtab_ent));
  return s;
}

static void free_symtab(upb_strtable *t)
{
  symtab_ent *e;
  for(e = upb_strtable_begin(t); e; e = upb_strtable_next(t, &e->e))
    upb_def_unref(e->def);
  upb_strtable_free(t);
}

void _upb_symtab_free(upb_symtab *s)
{
  free_symtab(&s->symtab);
  free_symtab(&s->psymtab);
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
  symtab_ent *e = upb_strtable_begin(&s->symtab);
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

upb_def *upb_symtab_lookup(upb_symtab *s, upb_strptr sym)
{
  upb_rwlock_rdlock(&s->lock);
  symtab_ent *e = upb_strtable_lookup(&s->symtab, sym);
  upb_def *ret = NULL;
  if(e) {
    ret = e->def;
    upb_def_ref(ret);
  }
  upb_rwlock_unlock(&s->lock);
  return ret;
}


upb_def *upb_symtab_resolve(upb_symtab *s, upb_strptr base, upb_strptr symbol) {
  upb_rwlock_rdlock(&s->lock);
  symtab_ent *e = resolve(&s->symtab, base, symbol);
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
}

void upb_symtab_add_desc(upb_symtab *s, upb_strptr desc, upb_status *status)
{
  upb_msg *fds = upb_msg_new(s->fds_msgdef);
  upb_msg_decodestr(fds, s->fds_msgdef, desc, status);
  if(!upb_ok(status)) return;
  upb_symtab_addfds(s, (google_protobuf_FileDescriptorSet*)fds, status);
  upb_msg_unref(fds, s->fds_msgdef);
  return;
}
