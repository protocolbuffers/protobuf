/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2008-2009 Joshua Haberman.  See LICENSE for details.
 */

#include "upb_def.h"
#include "descriptor.h"

/* Rounds p up to the next multiple of t. */
#define ALIGN_UP(p, t) ((p) % (t) == 0 ? (p) : (p) + ((t) - ((p) % (t))))

static int div_round_up(int numerator, int denominator) {
  /* cf. http://stackoverflow.com/questions/17944/how-to-round-up-the-result-of-integer-division */
  return numerator > 0 ? (numerator - 1) / denominator + 1 : 0;
}

/* Callback for sorting fields. */
static int compare_fields(const void *e1, const void *e2) {
  const google_protobuf_FieldDescriptorProto *fd1 = *(void**)e1;
  const google_protobuf_FieldDescriptorProto *fd2 = *(void**)e2;
  /* Required fields go before non-required. */
  bool req1 = fd1->label == GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_LABEL_REQUIRED;
  bool req2 = fd2->label == GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_LABEL_REQUIRED;
  if(req1 != req2) {
    return req2 - req1;
  } else {
    /* Within required and non-required field lists, list in number order.
     * TODO: consider ordering by data size to reduce padding. */
    return fd1->number - fd2->number;
  }
}

/* Callback for sorting fields. */
static int compare_fields2(const void *e1, const void *e2) {
  const struct upb_fielddef *f1 = e1;
  const struct upb_fielddef *f2 = e2;
  /* Required fields go before non-required. */
  bool req1 = f1->label == GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_LABEL_REQUIRED;
  bool req2 = f2->label == GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_LABEL_REQUIRED;
  if(req1 != req2) {
    return req2 - req1;
  } else {
    /* Within required and non-required field lists, list in number order.
     * TODO: consider ordering by data size to reduce padding. */
    return f1->number - f2->number;
  }
}

void upb_fielddef_sortfds(google_protobuf_FieldDescriptorProto **fds, size_t num)
{
  qsort(fds, num, sizeof(*fds), compare_fields);
}

void upb_fielddef_sort(struct upb_fielddef *defs, size_t num)
{
  qsort(defs, num, sizeof(*defs), compare_fields2);
}

void upb_def_init(struct upb_def *def, enum upb_def_type type,
                  struct upb_string *fqname) {
  def->type = type;
  def->fqname = fqname;
  upb_string_ref(fqname);
  upb_atomic_refcount_init(&def->refcount, 1);
}

void upb_def_uninit(struct upb_def *def) {
  upb_string_unref(def->fqname);
}

void upb_fielddef_init(struct upb_fielddef *f,
                       struct google_protobuf_FieldDescriptorProto *fd)
{
  f->type = fd->type;
  f->label = fd->label;
  f->number = fd->number;
  f->name = upb_strdup(fd->name);
  f->def = NULL;
  if(fd->set_flags.has.type_name) {
    f->def = (struct upb_def*)upb_unresolveddef_new(fd->type_name);
  }
}

void upb_fielddef_uninit(struct upb_fielddef *f)
{
  upb_string_unref(f->name);
  if(upb_fielddef_hasdef(f)) upb_def_unref(f->def);
}

struct upb_fielddef *upb_fielddef_dup(struct upb_fielddef *f)
{
  struct upb_fielddef *new_f = malloc(sizeof(*new_f));
  new_f->type = f->type;
  new_f->label = f->label;
  new_f->number = f->number;
  new_f->name = upb_strdup(f->name);
  new_f->type = f->type;
  new_f->def = NULL;
  if(upb_fielddef_hasdef(f)) {
    new_f->def = f->def;
    upb_def_ref(new_f->def);
  }
  return new_f;
}

struct upb_msgdef *upb_msgdef_new(struct upb_fielddef *fields, int num_fields,
                                  struct upb_string *fqname)
{
  struct upb_msgdef *m = malloc(sizeof(*m));
  upb_def_init(&m->def, UPB_DEF_MESSAGE, fqname);
  upb_inttable_init(&m->fields_by_num, num_fields,
                    sizeof(struct upb_fieldsbynum_entry));
  upb_strtable_init(&m->fields_by_name, num_fields,
                    sizeof(struct upb_fieldsbyname_entry));

  m->num_fields = num_fields;
  m->set_flags_bytes = div_round_up(m->num_fields, 8);
  // These are incremented in the loop.
  m->num_required_fields = 0;
  m->size = m->set_flags_bytes;
  m->fields = fields;

  size_t max_align = 0;
  for(int i = 0; i < num_fields; i++) {
    struct upb_fielddef *f = &m->fields[i];
    struct upb_type_info *type_info = &upb_type_info[f->type];

    // General alignment rules are: each member must be at an address that is a
    // multiple of that type's alignment.  Also, the size of the structure as
    // a whole must be a multiple of the greatest alignment of any member. */
    f->field_index = i;
    f->byte_offset = ALIGN_UP(m->size, type_info->align);
    m->size = f->byte_offset + type_info->size;
    max_align = UPB_MAX(max_align, type_info->align);
    if(f->label == GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_LABEL_REQUIRED) {
      // We currently rely on the fact that required fields are always sorted
      // to occur before non-required fields.
      m->num_required_fields++;
    }

    // Insert into the tables.  Note that f->ref will be uninitialized, even in
    // the tables' copies of *f, which is why we must update them separately
    // in upb_msg_setref() below.
    struct upb_fieldsbynum_entry nument = {.e = {.key = f->number}, .f = *f};
    struct upb_fieldsbyname_entry strent = {.e = {.key = upb_strdup(f->name)}, .f = *f};
    upb_inttable_insert(&m->fields_by_num, &nument.e);
    upb_strtable_insert(&m->fields_by_name, &strent.e);
  }

  if(max_align > 0) m->size = ALIGN_UP(m->size, max_align);
  return m;
}

void _upb_msgdef_free(struct upb_msgdef *m)
{
  upb_def_uninit(&m->def);
  upb_inttable_free(&m->fields_by_num);
  upb_strtable_free(&m->fields_by_name);
  for (unsigned int i = 0; i < m->num_fields; i++)
    upb_fielddef_uninit(&m->fields[i]);
  free(m->fields);
  free(m);
}

void upb_msgdef_resolve(struct upb_msgdef *m, struct upb_fielddef *f,
                        struct upb_def *def) {
  struct upb_fieldsbynum_entry *int_e = upb_inttable_fast_lookup(
      &m->fields_by_num, f->number, sizeof(struct upb_fieldsbynum_entry));
  struct upb_fieldsbyname_entry *str_e =
      upb_strtable_lookup(&m->fields_by_name, f->name);
  assert(int_e && str_e);
  f->def = def;
  int_e->f.def = def;
  str_e->f.def = def;
  upb_def_ref(def);
}

struct upb_enumdef *upb_enumdef_new(
    struct google_protobuf_EnumDescriptorProto *ed, struct upb_string *fqname)
{
  struct upb_enumdef *e = malloc(sizeof(*e));
  upb_def_init(&e->def, UPB_DEF_ENUM, fqname);
  int num_values = ed->set_flags.has.value ? ed->value->len : 0;
  upb_strtable_init(&e->nametoint, num_values,
                    sizeof(struct upb_enumdef_ntoi_entry));
  upb_inttable_init(&e->inttoname, num_values,
                    sizeof(struct upb_enumdef_iton_entry));

  for(int i = 0; i < num_values; i++) {
    google_protobuf_EnumValueDescriptorProto *value = ed->value->elements[i];
    struct upb_enumdef_ntoi_entry ntoi_entry = {.e = {.key = upb_strdup(value->name)},
                                                .value = value->number};
    struct upb_enumdef_iton_entry iton_entry = {.e = {.key = value->number},
                                                .string = value->name};
    upb_strtable_insert(&e->nametoint, &ntoi_entry.e);
    upb_inttable_insert(&e->inttoname, &iton_entry.e);
  }
  return e;
}

void _upb_enumdef_free(struct upb_enumdef *e) {
  upb_def_uninit(&e->def);
  upb_strtable_free(&e->nametoint);
  upb_inttable_free(&e->inttoname);
  free(e);
}
