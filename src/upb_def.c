/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2008-2009 Joshua Haberman.  See LICENSE for details.
 */

#include "descriptor.h"
#include "upb_def.h"
#include "upb_mm.h"
#include "upb_msg.h"

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
// The other is a count of how many defs within any of our cycles have external
// references.  When a def that is part of a cycle has an external reference,
// we say that it has a "cycle reference" on every def that it is part of a
// cycle with.  We free the defs with cycles when the cycle refcount drops to
// zero (which cannot happen unless the external refcount drops to zero first).
//
// Note that all refs in a cycle will have the same cycle refcount, except for
// ones that belong to more than one cycle which will have the sum of the
// cycle refcounts for all their cycles.
//
// This algorithm is relatively cheap, since it only requires extra work on
// initialization and when the regular refcount drops to zero.

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
static int cycle_unref(struct upb_def *def, struct upb_def *cycle_base,
                       struct upb_def **open_defs, int num_open_defs) {
  for(int i = 0; i < num_open_defs; i++) {
    if(open_defs[i] == def) {
      // We encountered a cycle that did not involve cycle_base.
      return 0;
    }
  }
  if(def == cycle_base) {
    return 1;
  } else {
    int path_count = 0;
    if(cycle_base == NULL) {
      cycle_base = def;
    } else {
      open_defs[num_open_defs++] = def;
    }
    struct upb_msgdef *m = upb_downcast_msgdef(def);
    assert(m);
    for(int i = 0; i < m->num_fields; i++) {
      struct upb_fielddef *f = &m->fields[i];
      if(upb_issubmsg(f) && f->def->max_cycle_len > 0)
        path_count += cycle_unref(f->def, cycle_base, open_defs, num_open_defs);
    }
    fprintf(stderr, "removing %d cycle refs from " UPB_STRFMT "\n", path_count, UPB_STRARG(def->fqname));
    if(upb_atomic_add(&def->cycle_refcount, -path_count))
      def_free(def);
    return path_count;
  }
}

void _upb_def_reftozero(struct upb_def *def) {
  if(def->max_cycle_len > 0) {
    fprintf(stderr, "cycle unreffing " UPB_STRFMT ", max len=%d\n", UPB_STRARG(def->fqname), def->max_cycle_len);
    // Traverse the graph, decrementing the cycle refcounts of all cyclic defs
    // and deleting any of them that fall to zero.
    //
    // We track the set of nodes that are currently being visited in the search
    // so we can detect cycles that do not involve this def and stop the
    // search.  We can't color the nodes as we go by writing to a member of the
    // def, because another thread could be performing the search concurrently.
    struct upb_def *open_defs[UPB_MAX_TYPE_CYCLE_LEN];
    cycle_unref(def, NULL, open_defs, 0);
  } else {
    def_free(def);
  }
}

void upb_def_init(struct upb_def *def, enum upb_def_type type,
                  struct upb_string *fqname) {
  def->type = type;
  def->max_cycle_len = 0;  // We increment this later, after resolving refs.
  def->visiting_submsg = 0;
  def->fqname = fqname;
  upb_string_ref(fqname);
  upb_atomic_refcount_init(&def->refcount, 1);
  upb_atomic_refcount_init(&def->cycle_refcount, 0);
}

void upb_def_uninit(struct upb_def *def) {
  upb_string_unref(def->fqname);
}

/* upb_unresolveddef **********************************************************/

struct upb_unresolveddef {
  struct upb_def base;
  struct upb_string *name;
};

static struct upb_unresolveddef *upb_unresolveddef_new(struct upb_string *str) {
  struct upb_unresolveddef *def = malloc(sizeof(*def));
  struct upb_string *name = upb_strdup(str);
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
  f->name = upb_strdup(fd->name);
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
  dst->name = upb_strdup(src->name);
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
                                     struct upb_string *fqname,
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
  fprintf(stderr, "Created msg: " UPB_STRFMT "\n", UPB_STRARG(fqname));
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
    struct upb_ntof_ent ntof_ent = {{upb_strdup(f->name), 0}, f};
    upb_inttable_insert(&m->itof, &itof_ent.e);
    upb_strtable_insert(&m->ntof, &ntof_ent.e);
  }

  if(max_align > 0) m->size = ALIGN_UP(m->size, max_align);
  return m;
}

static void msgdef_free(struct upb_msgdef *m)
{
  fprintf(stderr, "Freed msg: " UPB_STRFMT "\n", UPB_STRARG(UPB_UPCAST(m)->fqname));
  for (upb_field_count_t i = 0; i < m->num_fields; i++)
    fielddef_uninit(&m->fields[i]);
  free(m->fields);

  // Free refs from the strtable.
  // TODO: once memory management for data is more developed, let the table
  // handle freeing the refs itself.
  struct upb_strtable_entry *e = upb_strtable_begin(&m->ntof);
  for(; e; e = upb_strtable_next(&m->ntof, e)) {
    upb_string_unref(e->key);
  }
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
  struct upb_string *string;
};

static struct upb_enumdef *enumdef_new(google_protobuf_EnumDescriptorProto *ed,
                                       struct upb_string *fqname)
{
  struct upb_enumdef *e = malloc(sizeof(*e));
  upb_def_init(&e->base, UPB_DEF_ENUM, fqname);
  int num_values = ed->set_flags.has.value ? ed->value->len : 0;
  upb_strtable_init(&e->ntoi, num_values, sizeof(struct ntoi_ent));
  upb_inttable_init(&e->iton, num_values, sizeof(struct iton_ent));

  for(int i = 0; i < num_values; i++) {
    google_protobuf_EnumValueDescriptorProto *value = ed->value->elements[i];
    struct ntoi_ent ntoi_ent = {{upb_strdup(value->name), 0}, value->number};
    struct iton_ent iton_ent = {{value->number, 0}, value->name};
    upb_strtable_insert(&e->ntoi, &ntoi_ent.e);
    upb_inttable_insert(&e->iton, &iton_ent.e);
  }
  return e;
}

static void enumdef_free(struct upb_enumdef *e) {
  // Free refs from the strtable.
  // TODO: once memory management for data is more developed, let the table
  // handle freeing the refs itself.
  struct upb_strtable_entry *ent = upb_strtable_begin(&e->ntoi);
  for(; ent; ent = upb_strtable_next(&e->ntoi, ent)) {
    upb_string_unref(ent->key);
  }

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
                                  struct upb_string *base,
                                  struct upb_string *symbol)
{
  if(base->byte_len + symbol->byte_len + 1 >= UPB_SYMBOL_MAXLEN ||
     symbol->byte_len == 0) return NULL;

  if(symbol->ptr[0] == UPB_SYMBOL_SEPARATOR) {
    /* Symbols starting with '.' are absolute, so we do a single lookup. */
    struct upb_string sym_str = {.ptr = symbol->ptr+1,
                                 .byte_len = symbol->byte_len-1};
    return upb_strtable_lookup(t, &sym_str);
  } else {
    /* Remove components from base until we find an entry or run out. */
    char sym[UPB_SYMBOL_MAXLEN+1];
    struct upb_string sym_str = {.ptr = sym};
    int baselen = base->byte_len;
    while(1) {
      /* sym_str = base[0...base_len] + UPB_SYMBOL_SEPARATOR + symbol */
      memcpy(sym, base->ptr, baselen);
      sym[baselen] = UPB_SYMBOL_SEPARATOR;
      memcpy(sym + baselen + 1, symbol->ptr, symbol->byte_len);
      sym_str.byte_len = baselen + symbol->byte_len + 1;

      struct symtab_ent *e = upb_strtable_lookup(t, &sym_str);
      if (e) return e;
      else if(baselen == 0) return NULL;  /* No more scopes to try. */

      baselen = my_memrchr(base->ptr, UPB_SYMBOL_SEPARATOR, baselen);
    }
  }
}

/* Joins strings together, for example:
 *   join("Foo.Bar", "Baz") -> "Foo.Bar.Baz"
 *   join("", "Baz") -> "Baz"
 * Caller owns the returned string and must free it. */
static struct upb_string *join(struct upb_string *base, struct upb_string *name) {
  size_t len = base->byte_len + name->byte_len;
  if(base->byte_len > 0) len++;  /* For the separator. */
  struct upb_string *joined = upb_string_new();
  upb_string_resize(joined, len);
  if(base->byte_len > 0) {
    /* nested_base = base + '.' +  d->name */
    memcpy(joined->ptr, base->ptr, base->byte_len);
    joined->ptr[base->byte_len] = UPB_SYMBOL_SEPARATOR;
    memcpy(&joined->ptr[base->byte_len+1], name->ptr, name->byte_len);
  } else {
    memcpy(joined->ptr, name->ptr, name->byte_len);
  }
  return joined;
}

static struct upb_string *try_define(struct upb_strtable *t,
                                     struct upb_string *base,
                                     struct upb_string *name,
                                     struct upb_status *status)
{
  if(!name) {
    upb_seterr(status, UPB_STATUS_ERROR,
               "symbol in context '" UPB_STRFMT "' does not have a name",
               UPB_STRARG(base));
    return NULL;
  }
  struct upb_string *fqname = join(base, name);
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
                        struct upb_string *base,
                        struct upb_status *status)
{
  struct upb_string *name = ed->set_flags.has.name ? ed->name : NULL;
  struct upb_string *fqname = try_define(t, base, name, status);
  if(!fqname) return;

  struct symtab_ent e;
  e.e.key = fqname;  // Donating our ref to the table.
  e.def = UPB_UPCAST(enumdef_new(ed, fqname));
  upb_strtable_insert(t, &e.e);
}

static void insert_message(struct upb_strtable *t,
                           google_protobuf_DescriptorProto *d,
                           struct upb_string *base, bool sort,
                           struct upb_status *status)
{
  struct upb_string *name = d->set_flags.has.name ? d->name : NULL;
  struct upb_string *fqname = try_define(t, base, name, status);
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

// Traverses the cycle again to count its length and adjust reference counts.
// Returns the total length of the cycle.
static int process_cycle(struct upb_msgdef *m, struct upb_msgdef *cycle_base,
                         int partial_cycle_len, struct upb_status *status) {
  if(m == cycle_base && partial_cycle_len > 0) {
    return partial_cycle_len;
  } else {
    struct upb_def *def = UPB_UPCAST(m);
    partial_cycle_len++;
    struct upb_fielddef *f = &m->fields[def->visiting_submsg - 1];
    struct upb_msgdef *sub_m = upb_downcast_msgdef(f->def);
    assert(sub_m);
    int cycle_len = process_cycle(sub_m, cycle_base, partial_cycle_len, status);
    if(cycle_len > UPB_MAX_TYPE_CYCLE_LEN) {
      upb_seterr(status, UPB_STATUS_ERROR, "Cycle in type " UPB_STRFMT " is of "
                 "length %d, exceeds maximum type cycle length of %d",
                 cycle_len, UPB_MAX_TYPE_CYCLE_LEN);
      return 0;
    }

    // Since we know that each def has exactly one external ref (the symtab's
    // ref), the number of defs in the cycle is the number of cycle refs we
    // should add to each def in the cycle.
    upb_atomic_add(&def->cycle_refcount, cycle_len);

    // Remove the regular ref that comes from the cycle.
    fprintf(stderr, UPB_STRFMT "\n", UPB_STRARG(def->fqname));
    if(f->owned) {
      bool zero = upb_atomic_unref(&f->def->refcount);
      f->owned = false;
      assert(!zero);  // The symtab should hold an external ref.
      (void)zero;     // Silence "zero not used" warnings on non-debug builds.
    }
    def->max_cycle_len = UPB_MAX(def->max_cycle_len, cycle_len);

    return cycle_len;
  }
}

// Depth-first search, detecting and marking cycles.
static void find_cycles(struct upb_def *def, struct upb_status *status) {
  struct upb_msgdef *m = upb_downcast_msgdef(def);
  if(!m) return;
  if(def->visiting_submsg) {
    // Cycle detected!
    fprintf(stderr, "Found cycle in " UPB_STRFMT "\n", UPB_STRARG(def->fqname));
    process_cycle(m, m, 0, status);
  } else if(def->max_cycle_len > 0) {
    // This def is part of at least one cycle, but we have already discovered
    // this node (and consequently all of its cycles) by some other path.
  } else {
    for(int i = 0; i < m->num_fields; i++) {
      struct upb_fielddef *f = &m->fields[i];
      // Mark the path we are currently on so we can easily retrace it in
      // process_cycle().
      UPB_UPCAST(m)->visiting_submsg = i + 1;
      if(upb_issubmsg(f)) find_cycles(f->def, status);
    }
    UPB_UPCAST(m)->visiting_submsg = 0;
  }
}

void addfd(struct upb_strtable *addto, struct upb_strtable *existingdefs,
           google_protobuf_FileDescriptorProto *fd, bool sort,
           struct upb_status *status)
{
  struct upb_string *pkg;
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
    struct upb_msgdef *m = upb_downcast_msgdef(e->def);
    if(!m) continue;
    struct upb_string *base = e->e.key;
    for(upb_field_count_t i = 0; i < m->num_fields; i++) {
      struct upb_fielddef *f = &m->fields[i];
      if(!upb_hasdef(f)) continue;  // No resolving necessary.
      struct upb_string *name = upb_downcast_unresolveddef(f->def)->name;
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

  // Find cycles and update refcounts appropriately.
  for(e = upb_strtable_begin(addto); e; e = upb_strtable_next(addto, &e->e)) {
    // TODO: make sure we don't leak memory if this fails due to excessive
    // cycle len.
    find_cycles(e->def, status);
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
    return NULL;  /* Indicates that upb is buggy or corrupt. */
  }
  struct upb_string name = UPB_STRLIT("google.protobuf.FileDescriptorSet");
  struct symtab_ent *e = upb_strtable_lookup(&s->psymtab, &name);
  assert(e);
  s->fds_msgdef = upb_downcast_msgdef(e->def);
  return s;
}

static void free_symtab(struct upb_strtable *t)
{
  struct symtab_ent *e = upb_strtable_begin(t);
  for(; e; e = upb_strtable_next(t, &e->e)) {
    upb_def_unref(e->def);
    upb_string_unref(e->e.key);
  }
  upb_strtable_free(t);
}

void _upb_symtab_free(struct upb_symtab *s)
{
  free_symtab(&s->symtab);
  free_symtab(&s->psymtab);
  upb_rwlock_destroy(&s->lock);
  free(s);
}

struct upb_def **upb_symtab_getandref_defs(struct upb_symtab *s, int *count)
{
  upb_rwlock_wrlock(&s->lock);
  *count = upb_strtable_count(&s->symtab);
  struct upb_def **defs = malloc(sizeof(*defs) * (*count));
  struct symtab_ent *e = upb_strtable_begin(&s->symtab);
  int i = 0;
  for(; e; e = upb_strtable_next(&s->symtab, &e->e), i++) {
    assert(e->def);
    defs[i] = e->def;
    upb_def_ref(defs[i]);
  }
  assert(*count == i);
  upb_rwlock_unlock(&s->lock);
  return defs;
}

struct upb_def *upb_symtab_lookup(struct upb_symtab *s,
                                   struct upb_string *sym)
{
  upb_rwlock_rdlock(&s->lock);
  struct symtab_ent *e = upb_strtable_lookup(&s->symtab, sym);
  upb_rwlock_unlock(&s->lock);
  return e ? e->def : NULL;
}


struct upb_def *upb_symtab_resolve(struct upb_symtab *s,
                                    struct upb_string *base,
                                    struct upb_string *symbol) {
  upb_rwlock_rdlock(&s->lock);
  struct symtab_ent *e = resolve(&s->symtab, base, symbol);
  upb_rwlock_unlock(&s->lock);
  return e ? e->def : NULL;
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

    // Everything was successfully added, copy from the tmp symtable.
    struct symtab_ent *e;
    {
      upb_rwlock_wrlock(&s->lock);
      for(e = upb_strtable_begin(&tmp); e; e = upb_strtable_next(&tmp, &e->e))
        upb_strtable_insert(&s->symtab, &e->e);
      upb_rwlock_unlock(&s->lock);
    }
    upb_strtable_free(&tmp);
  }
  return;
}

void upb_symtab_add_desc(struct upb_symtab *s, struct upb_string *desc,
                         struct upb_status *status)
{
  struct upb_msg *fds = upb_msg_new(s->fds_msgdef);
  upb_msg_parsestr(fds, desc->ptr, desc->byte_len, status);
  if(!upb_ok(status)) return;
  upb_symtab_addfds(s, (google_protobuf_FileDescriptorSet*)fds, status);
  upb_msg_unref(fds);
  return;
}
