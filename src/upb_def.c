/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2008-2009 Joshua Haberman.  See LICENSE for details.
 */

#include <stdlib.h>
#include <limits.h>
#include "descriptor.h"
#include "upb_def.h"
#include "upb_data.h"

/* Rounds p up to the next multiple of t. */
#define ALIGN_UP(p, t) ((p) % (t) == 0 ? (p) : (p) + ((t) - ((p) % (t))))

static int div_round_up(int numerator, int denominator) {
  /* cf. http://stackoverflow.com/questions/17944/how-to-round-up-the-result-of-integer-division */
  return numerator > 0 ? (numerator - 1) / denominator + 1 : 0;
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

static void msgdef_free(struct upb_msgdef *m);
static void enumdef_free(struct upb_enumdef *e);
static void unresolveddef_free(struct upb_unresolveddef *u);

static void def_free(struct upb_def *def)
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
static int cycle_ref_or_unref(struct upb_msgdef *m,
                              struct upb_msgdef *cycle_base,
                              struct upb_msgdef **open_defs, int num_open_defs,
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
      struct upb_fielddef *f = &m->fields[i];
      struct upb_def *def = f->def;
      if(upb_issubmsg(f) && def->is_cyclic) {
        struct upb_msgdef *sub_m = upb_downcast_msgdef(def);
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

void _upb_def_reftozero(struct upb_def *def) {
  if(def->is_cyclic) {
    struct upb_msgdef *m = upb_downcast_msgdef(def);
    struct upb_msgdef *open_defs[UPB_MAX_TYPE_CYCLE_LEN];
    cycle_ref_or_unref(m, NULL, open_defs, 0, false);
  } else {
    def_free(def);
  }
}

void _upb_def_cyclic_ref(struct upb_def *def) {
  struct upb_msgdef *open_defs[UPB_MAX_TYPE_CYCLE_LEN];
  cycle_ref_or_unref(upb_downcast_msgdef(def), NULL, open_defs, 0, true);
}

static void upb_def_init(struct upb_def *def, enum upb_def_type type,
                         upb_string *fqname) {
  def->type = type;
  def->is_cyclic = 0;  // We detect this later, after resolving refs.
  def->search_depth = 0;
  def->fqname = upb_string_getref(fqname, UPB_REF_FROZEN);
  upb_atomic_refcount_init(&def->refcount, 1);
}

static void upb_def_uninit(struct upb_def *def) {
  upb_string_unref(def->fqname);
}

/* upb_unresolveddef **********************************************************/

struct upb_unresolveddef {
  struct upb_def base;
  upb_string *name;
};

static struct upb_unresolveddef *upb_unresolveddef_new(upb_string *str) {
  struct upb_unresolveddef *def = malloc(sizeof(*def));
  upb_string *name = upb_string_getref(str, UPB_REF_THREADUNSAFE_READONLY);
  upb_def_init(&def->base, UPB_DEF_UNRESOLVED, name);
  def->name = name;
  return def;
}

static void unresolveddef_free(struct upb_unresolveddef *def) {
  upb_string_unref(def->name);
  upb_def_uninit(&def->base);
  free(def);
}

/* upb_fielddef ***************************************************************/

static void fielddef_init(struct upb_fielddef *f,
                          google_protobuf_FieldDescriptorProto *fd)
{
  f->type = fd->type;
  f->label = fd->label;
  f->number = fd->number;
  f->name = upb_string_getref(fd->name, UPB_REF_FROZEN);
  f->def = NULL;
  f->owned = false;
  assert(fd->set_flags.has.type_name == upb_hasdef(f));
  if(fd->set_flags.has.type_name) {
    f->def = UPB_UPCAST(upb_unresolveddef_new(fd->type_name));
    f->owned = true;
  }
}

static struct upb_fielddef *fielddef_new(
    google_protobuf_FieldDescriptorProto *fd) {
  struct upb_fielddef *f = malloc(sizeof(*f));
  fielddef_init(f, fd);
  return f;
}

static void fielddef_uninit(struct upb_fielddef *f)
{
  upb_string_unref(f->name);
  if(upb_hasdef(f) && f->owned) {
    upb_def_unref(f->def);
  }
}

static void fielddef_copy(struct upb_fielddef *dst, struct upb_fielddef *src)
{
  *dst = *src;
  dst->name = upb_string_getref(src->name, UPB_REF_FROZEN);
  if(upb_hasdef(src)) {
    upb_def_ref(dst->def);
    dst->owned = true;
  }
}

// Callback for sorting fields.
static int compare_fields(struct upb_fielddef *f1, struct upb_fielddef *f2) {
  // Required fields go before non-required.
  bool req1 = f1->label == GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_LABEL_REQUIRED;
  bool req2 = f2->label == GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_LABEL_REQUIRED;
  if(req1 != req2) {
    return req2 - req1;
  } else {
    // Within required and non-required field lists, list in number order.
    // TODO: consider ordering by data size to reduce padding. */
    return f1->number - f2->number;
  }
}

static int compare_fielddefs(const void *e1, const void *e2) {
  return compare_fields(*(void**)e1, *(void**)e2);
}

static int compare_fds(const void *e1, const void *e2) {
  struct upb_fielddef f1, f2;
  fielddef_init(&f1, *(void**)e1);
  fielddef_init(&f2, *(void**)e2);
  int ret = compare_fields(&f1, &f2);
  fielddef_uninit(&f1);
  fielddef_uninit(&f2);
  return ret;
}

void upb_fielddef_sortfds(google_protobuf_FieldDescriptorProto **fds, size_t num)
{
  qsort(fds, num, sizeof(*fds), compare_fds);
}

static void fielddef_sort(struct upb_fielddef **defs, size_t num)
{
  qsort(defs, num, sizeof(*defs), compare_fielddefs);
}

/* upb_msgdef *****************************************************************/

static struct upb_msgdef *msgdef_new(struct upb_fielddef **fields,
                                     int num_fields,
                                     upb_string *fqname,
                                     struct upb_status *status)
{
  if(num_fields > UPB_MAX_FIELDS) {
    upb_seterr(status, UPB_STATUS_ERROR,
               "Tried to create a msgdef with more than %d fields", num_fields);
    free(fields);
    return NULL;
  }
  struct upb_msgdef *m = malloc(sizeof(*m));
  upb_def_init(&m->base, UPB_DEF_MSG, fqname);
  upb_atomic_refcount_init(&m->cycle_refcount, 0);
  upb_inttable_init(&m->itof, num_fields, sizeof(struct upb_itof_ent));
  upb_strtable_init(&m->ntof, num_fields, sizeof(struct upb_ntof_ent));

  m->num_fields = num_fields;
  m->set_flags_bytes = div_round_up(m->num_fields, 8);
  // These are incremented in the loop.
  m->num_required_fields = 0;
  m->size = m->set_flags_bytes;
  m->fields = malloc(sizeof(struct upb_fielddef) * num_fields);

  size_t max_align = 0;
  for(int i = 0; i < num_fields; i++) {
    struct upb_fielddef *f = &m->fields[i];
    struct upb_type_info *type_info = &upb_type_info[fields[i]->type];
    fielddef_copy(f, fields[i]);

    // General alignment rules are: each member must be at an address that is a
    // multiple of that type's alignment.  Also, the size of the structure as
    // a whole must be a multiple of the greatest alignment of any member. */
    f->field_index = i;
    f->byte_offset = ALIGN_UP(m->size, type_info->align);
    m->size = f->byte_offset + type_info->size;
    max_align = UPB_MAX(max_align, type_info->align);
    if(f->label == UPB_LABEL(REQUIRED)) {
      // We currently rely on the fact that required fields are always sorted
      // to occur before non-required fields.
      m->num_required_fields++;
    }

    // Insert into the tables.
    struct upb_itof_ent itof_ent = {{f->number, 0}, f};
    struct upb_ntof_ent ntof_ent = {{f->name, 0}, f};
    upb_inttable_insert(&m->itof, &itof_ent.e);
    upb_strtable_insert(&m->ntof, &ntof_ent.e);
  }

  if(max_align > 0) m->size = ALIGN_UP(m->size, max_align);
  return m;
}

static void msgdef_free(struct upb_msgdef *m)
{
  for (upb_field_count_t i = 0; i < m->num_fields; i++)
    fielddef_uninit(&m->fields[i]);
  free(m->fields);
  upb_strtable_free(&m->ntof);
  upb_inttable_free(&m->itof);
  upb_def_uninit(&m->base);
  free(m);
}

static void upb_msgdef_resolve(struct upb_msgdef *m, struct upb_fielddef *f,
                               struct upb_def *def) {
  (void)m;
  if(f->owned) upb_def_unref(f->def);
  f->def = def;
  // We will later make the ref unowned if it is a part of a cycle.
  f->owned = true;
  upb_def_ref(def);
}

/* upb_enumdef ****************************************************************/

struct ntoi_ent {
  struct upb_strtable_entry e;
  uint32_t value;
};

struct iton_ent {
  struct upb_inttable_entry e;
  upb_string *string;
};

static struct upb_enumdef *enumdef_new(google_protobuf_EnumDescriptorProto *ed,
                                       upb_string *fqname)
{
  struct upb_enumdef *e = malloc(sizeof(*e));
  upb_def_init(&e->base, UPB_DEF_ENUM, fqname);
  int num_values = ed->set_flags.has.value ? ed->value->len : 0;
  upb_strtable_init(&e->ntoi, num_values, sizeof(struct ntoi_ent));
  upb_inttable_init(&e->iton, num_values, sizeof(struct iton_ent));

  for(int i = 0; i < num_values; i++) {
    google_protobuf_EnumValueDescriptorProto *value = ed->value->elements[i];
    struct ntoi_ent ntoi_ent = {{value->name, 0}, value->number};
    struct iton_ent iton_ent = {{value->number, 0}, value->name};
    upb_strtable_insert(&e->ntoi, &ntoi_ent.e);
    upb_inttable_insert(&e->iton, &iton_ent.e);
  }
  return e;
}

static void enumdef_free(struct upb_enumdef *e) {
  upb_strtable_free(&e->ntoi);
  upb_inttable_free(&e->iton);
  upb_def_uninit(&e->base);
  free(e);
}

static void fill_iter(struct upb_enum_iter *iter, struct ntoi_ent *ent) {
  iter->state = ent;
  iter->name = ent->e.key;
  iter->val = ent->value;
}

void upb_enum_begin(struct upb_enum_iter *iter, struct upb_enumdef *e) {
  // We could iterate over either table here; the choice is arbitrary.
  struct ntoi_ent *ent = upb_strtable_begin(&e->ntoi);
  iter->e = e;
  fill_iter(iter, ent);
}

void upb_enum_next(struct upb_enum_iter *iter) {
  struct ntoi_ent *ent = iter->state;
  assert(ent);
  ent = upb_strtable_next(&iter->e->ntoi, &ent->e);
  iter->state = ent;
  if(ent) fill_iter(iter, ent);
}

bool upb_enum_done(struct upb_enum_iter *iter) {
  return iter->state == NULL;
}

/* symtab internal  ***********************************************************/

struct symtab_ent {
  struct upb_strtable_entry e;
  struct upb_def *def;
};

/* Search for a character in a string, in reverse. */
static int my_memrchr(char *data, char c, size_t len)
{
  int off = len-1;
  while(off > 0 && data[off] != c) --off;
  return off;
}

/* Given a symbol and the base symbol inside which it is defined, find the
 * symbol's definition in t. */
static struct symtab_ent *resolve(struct upb_strtable *t,
                                  upb_string *base,
                                  upb_string *symbol)
{
  if(upb_strlen(base) + upb_strlen(symbol) + 1 >= UPB_SYMBOL_MAXLEN ||
     upb_strlen(symbol) == 0) return NULL;

  if(upb_string_getrobuf(symbol)[0] == UPB_SYMBOL_SEPARATOR) {
    // Symbols starting with '.' are absolute, so we do a single lookup.
    // Slice to omit the leading '.'
    upb_string *sym_str = upb_strslice(symbol, 1, INT_MAX);
    struct symtab_ent *e = upb_strtable_lookup(t, sym_str);
    upb_string_unref(sym_str);
    return e;
  } else {
    // Remove components from base until we find an entry or run out.
    upb_string *sym_str = upb_string_new();
    int baselen = upb_strlen(base);
    while(1) {
      // sym_str = base[0...base_len] + UPB_SYMBOL_SEPARATOR + symbol
      upb_strlen_t len = baselen + upb_strlen(symbol) + 1;
      char *buf = upb_string_getrwbuf(sym_str, len);
      memcpy(buf, upb_string_getrobuf(base), baselen);
      buf[baselen] = UPB_SYMBOL_SEPARATOR;
      memcpy(buf + baselen + 1, upb_string_getrobuf(symbol), upb_strlen(symbol));

      struct symtab_ent *e = upb_strtable_lookup(t, sym_str);
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
static upb_string *join(upb_string *base, upb_string *name) {
  upb_string *joined = upb_strdup(base);
  upb_strlen_t len = upb_strlen(joined);
  if(len > 0) {
    upb_string_getrwbuf(joined, len + 1)[len] = UPB_SYMBOL_SEPARATOR;
  }
  upb_strcat(joined, name);
  return joined;
}

static upb_string *try_define(struct upb_strtable *t, upb_string *base,
                              upb_string *name, struct upb_status *status)
{
  if(!name) {
    upb_seterr(status, UPB_STATUS_ERROR,
               "symbol in context '" UPB_STRFMT "' does not have a name",
               UPB_STRARG(base));
    return NULL;
  }
  upb_string *fqname = join(base, name);
  if(upb_strtable_lookup(t, fqname)) {
    upb_seterr(status, UPB_STATUS_ERROR,
               "attempted to redefine symbol '" UPB_STRFMT "'",
               UPB_STRARG(fqname));
    upb_string_unref(fqname);
    return NULL;
  }
  return fqname;
}

static void insert_enum(struct upb_strtable *t,
                        google_protobuf_EnumDescriptorProto *ed,
                        upb_string *base,
                        struct upb_status *status)
{
  upb_string *name = ed->set_flags.has.name ? ed->name : NULL;
  upb_string *fqname = try_define(t, base, name, status);
  if(!fqname) return;

  struct symtab_ent e;
  e.e.key = fqname;  // Donating our ref to the table.
  e.def = UPB_UPCAST(enumdef_new(ed, fqname));
  upb_strtable_insert(t, &e.e);
}

static void insert_message(struct upb_strtable *t,
                           google_protobuf_DescriptorProto *d,
                           upb_string *base, bool sort,
                           struct upb_status *status)
{
  upb_string *name = d->set_flags.has.name ? d->name : NULL;
  upb_string *fqname = try_define(t, base, name, status);
  if(!fqname) return;

  int num_fields = d->set_flags.has.field ? d->field->len : 0;
  struct symtab_ent e;
  e.e.key = fqname;  // Donating our ref to the table.
  struct upb_fielddef **fielddefs = malloc(sizeof(*fielddefs) * num_fields);
  for (int i = 0; i < num_fields; i++) {
    google_protobuf_FieldDescriptorProto *fd = d->field->elements[i];
    fielddefs[i] = fielddef_new(fd);
  }
  if(sort) fielddef_sort(fielddefs, d->field->len);
  e.def = UPB_UPCAST(msgdef_new(fielddefs, d->field->len, fqname, status));
  for (int i = 0; i < num_fields; i++) {
    fielddef_uninit(fielddefs[i]);
    free(fielddefs[i]);
  }
  free(fielddefs);

  if(!upb_ok(status)) return;

  upb_strtable_insert(t, &e.e);

  /* Add nested messages and enums. */
  if(d->set_flags.has.nested_type)
    for(unsigned int i = 0; i < d->nested_type->len; i++)
      insert_message(t, d->nested_type->elements[i], fqname, sort, status);

  if(d->set_flags.has.enum_type)
    for(unsigned int i = 0; i < d->enum_type->len; i++)
      insert_enum(t, d->enum_type->elements[i], fqname, status);
}

static bool find_cycles(struct upb_msgdef *m, int search_depth,
                        struct upb_status *status)
{
  if(search_depth > UPB_MAX_TYPE_DEPTH) {
    // There are many situations in upb where we recurse over the type tree
    // (like for example, right now) and an absurdly deep tree could cause us
    // to stack overflow on systems with very limited stacks.
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
      struct upb_fielddef *f = &m->fields[i];
      if(!upb_issubmsg(f)) continue;
      struct upb_def *sub_def = f->def;
      struct upb_msgdef *sub_m = upb_downcast_msgdef(sub_def);
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

static void addfd(struct upb_strtable *addto, struct upb_strtable *existingdefs,
                  google_protobuf_FileDescriptorProto *fd, bool sort,
                  struct upb_status *status)
{
  upb_string *pkg;
  // Temporary hack until the static data is integrated into our
  // memory-management scheme.
  bool should_unref;
  if(fd->set_flags.has.package) {
    pkg = fd->package;
    should_unref = false;
  } else {
    pkg = upb_string_new();
    should_unref = true;
  }

  if(fd->set_flags.has.message_type)
    for(unsigned int i = 0; i < fd->message_type->len; i++)
      insert_message(addto, fd->message_type->elements[i], pkg, sort, status);

  if(fd->set_flags.has.enum_type)
    for(unsigned int i = 0; i < fd->enum_type->len; i++)
      insert_enum(addto, fd->enum_type->elements[i], pkg, status);

  if(should_unref) upb_string_unref(pkg);

  if(!upb_ok(status)) {
    // TODO: make sure we don't leak any memory in this case.
    return;
  }

  /* TODO: handle extensions and services. */

  // Attempt to resolve all references.
  struct symtab_ent *e;
  for(e = upb_strtable_begin(addto); e; e = upb_strtable_next(addto, &e->e)) {
    struct upb_msgdef *m = upb_dyncast_msgdef(e->def);
    if(!m) continue;
    upb_string *base = e->e.key;
    for(upb_field_count_t i = 0; i < m->num_fields; i++) {
      struct upb_fielddef *f = &m->fields[i];
      if(!upb_hasdef(f)) continue;  // No resolving necessary.
      upb_string *name = upb_downcast_unresolveddef(f->def)->name;
      struct symtab_ent *found = resolve(existingdefs, base, name);
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
    struct upb_msgdef *m = upb_dyncast_msgdef(e->def);
    if(!m) continue;

    // Do an initial pass over the graph to check that there are no cycles
    // longer than the maximum length.  We also mark all cyclic defs as such,
    // and decrement refs on cyclic defs.
    find_cycles(m, 0, status);
    struct upb_msgdef *open_defs[UPB_MAX_TYPE_CYCLE_LEN];
    cycle_ref_or_unref(m, NULL, open_defs, 0, true);
  }
}

/* upb_symtab *****************************************************************/

struct upb_symtab *upb_symtab_new()
{
  struct upb_symtab *s = malloc(sizeof(*s));
  upb_atomic_refcount_init(&s->refcount, 1);
  upb_rwlock_init(&s->lock);
  upb_strtable_init(&s->symtab, 16, sizeof(struct symtab_ent));
  upb_strtable_init(&s->psymtab, 16, sizeof(struct symtab_ent));

  // Add descriptor.proto types to private symtable so we can parse descriptors.
  google_protobuf_FileDescriptorProto *fd =
      upb_file_descriptor_set->file->elements[0];  // We know there is only 1.
  struct upb_status status = UPB_STATUS_INIT;
  addfd(&s->psymtab, &s->symtab, fd, false, &status);
  if(!upb_ok(&status)) {
    fprintf(stderr, "Failed to initialize upb: %s.\n", status.msg);
    assert(false);
    return NULL;  // Indicates that upb is buggy or corrupt.
  }
  upb_string name = UPB_STRLIT("google.protobuf.FileDescriptorSet");
  struct symtab_ent *e = upb_strtable_lookup(&s->psymtab, &name);
  assert(e);
  s->fds_msgdef = upb_downcast_msgdef(e->def);
  return s;
}

static void free_symtab(struct upb_strtable *t)
{
  struct symtab_ent *e = upb_strtable_begin(t);
  for(; e; e = upb_strtable_next(t, &e->e))
    upb_def_unref(e->def);
  upb_strtable_free(t);
}

void _upb_symtab_free(struct upb_symtab *s)
{
  free_symtab(&s->symtab);
  free_symtab(&s->psymtab);
  upb_rwlock_destroy(&s->lock);
  free(s);
}

struct upb_def **upb_symtab_getdefs(struct upb_symtab *s, int *count,
                                    upb_def_type_t type)
{
  upb_rwlock_rdlock(&s->lock);
  int total = upb_strtable_count(&s->symtab);
  // We may only use part of this, depending on how many symbols are of the
  // correct type.
  struct upb_def **defs = malloc(sizeof(*defs) * total);
  struct symtab_ent *e = upb_strtable_begin(&s->symtab);
  int i = 0;
  for(; e; e = upb_strtable_next(&s->symtab, &e->e)) {
    struct upb_def *def = e->def;
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

struct upb_def *upb_symtab_lookup(struct upb_symtab *s, upb_string *sym)
{
  upb_rwlock_rdlock(&s->lock);
  struct symtab_ent *e = upb_strtable_lookup(&s->symtab, sym);
  struct upb_def *ret = NULL;
  if(e) {
    ret = e->def;
    upb_def_ref(ret);
  }
  upb_rwlock_unlock(&s->lock);
  return ret;
}


struct upb_def *upb_symtab_resolve(struct upb_symtab *s, upb_string *base,
                                   upb_string *symbol) {
  upb_rwlock_rdlock(&s->lock);
  struct symtab_ent *e = resolve(&s->symtab, base, symbol);
  struct upb_def *ret = NULL;
  if(e) {
    ret = e->def;
    upb_def_ref(ret);
  }
  upb_rwlock_unlock(&s->lock);
  return ret;
}

void upb_symtab_addfds(struct upb_symtab *s,
                        google_protobuf_FileDescriptorSet *fds,
                        struct upb_status *status)
{
  if(fds->set_flags.has.file) {
    // Insert new symbols into a temporary table until we have verified that
    // the descriptor is valid.
    struct upb_strtable tmp;
    upb_strtable_init(&tmp, 0, sizeof(struct symtab_ent));

    {  // Read lock scope
      upb_rwlock_rdlock(&s->lock);
      for(uint32_t i = 0; i < fds->file->len; i++) {
        addfd(&tmp, &s->symtab, fds->file->elements[i], true, status);
        if(!upb_ok(status)) {
          free_symtab(&tmp);
          upb_rwlock_unlock(&s->lock);
          return;
        }
      }
      upb_rwlock_unlock(&s->lock);
    }

    // Everything was successfully added, copy from the tmp symtable.
    {  // Write lock scope
      upb_rwlock_wrlock(&s->lock);
      struct symtab_ent *e;
      for(e = upb_strtable_begin(&tmp); e; e = upb_strtable_next(&tmp, &e->e)) {
        // We checked for duplicates when we had only the read lock, but it is
        // theoretically possible that a duplicate symbol when we dropped the
        // read lock to acquire a write lock.
        if(upb_strtable_lookup(&s->symtab, e->e.key)) {
          upb_seterr(status, UPB_STATUS_ERROR, "Attempted to insert duplicate "
                     "symbol: " UPB_STRFMT, UPB_STRARG(e->e.key));
          // To truly handle this situation we would need to remove any symbols
          // from tmp that were successfully inserted into s->symtab.  Because
          // this case is exceedingly unlikely, and because our hashtable
          // doesn't support deletions right now, we leave them in there, which
          // means we must not call free_symtab(&s->symtab), so we will leak it.
          break;
        }
        upb_strtable_insert(&s->symtab, &e->e);
      }
      upb_rwlock_unlock(&s->lock);
    }
    upb_strtable_free(&tmp);
  }
  return;
}

void upb_symtab_add_desc(struct upb_symtab *s, upb_string *desc,
                         struct upb_status *status)
{
  upb_msg *fds = upb_msg_new(s->fds_msgdef);
  upb_msg_parsestr(fds, s->fds_msgdef, desc, status);
  if(!upb_ok(status)) return;
  upb_symtab_addfds(s, (google_protobuf_FileDescriptorSet*)fds, status);
  upb_msg_unref(fds, s->fds_msgdef);
  return;
}
