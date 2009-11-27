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

void upb_msgdef_sortfds(google_protobuf_FieldDescriptorProto **fds, size_t num)
{
  qsort(fds, num, sizeof(void*), compare_fields);
}

void upb_msgdef_init(struct upb_msgdef *m, google_protobuf_DescriptorProto *d,
                     struct upb_string *fqname, bool sort, struct upb_context *c,
                     struct upb_status *status)
{
  (void)status;  // Nothing that can fail at the moment.
  int num_fields = d->set_flags.has.field ? d->field->len : 0;
  upb_inttable_init(&m->fields_by_num, num_fields,
                    sizeof(struct upb_fieldsbynum_entry));
  upb_strtable_init(&m->fields_by_name, num_fields,
                    sizeof(struct upb_fieldsbyname_entry));

  m->fqname = upb_strdup(fqname);
  m->context = c;
  m->num_fields = num_fields;
  m->set_flags_bytes = div_round_up(m->num_fields, 8);
  /* These are incremented in the loop. */
  m->num_required_fields = 0;
  m->size = m->set_flags_bytes;

  m->fields = malloc(sizeof(*m->fields) * m->num_fields);

  /* Create a sorted list of the fields. */
  google_protobuf_FieldDescriptorProto **fds =
      malloc(sizeof(*fds) * m->num_fields);
  for(unsigned int i = 0; i < m->num_fields; i++) {
    /* We count on the caller to keep this pointer alive. */
    fds[i] = d->field->elements[i];
  }
  if(sort) upb_msgdef_sortfds(fds, m->num_fields);

  size_t max_align = 0;
  for(unsigned int i = 0; i < m->num_fields; i++) {
    struct upb_fielddef *f = &m->fields[i];
    google_protobuf_FieldDescriptorProto *fd = fds[i];
    struct upb_type_info *type_info = &upb_type_info[fd->type];

    /* General alignment rules are: each member must be at an address that is a
     * multiple of that type's alignment.  Also, the size of the structure as
     * a whole must be a multiple of the greatest alignment of any member. */
    f->field_index = i;
    f->byte_offset = ALIGN_UP(m->size, type_info->align);
    f->type = fd->type;
    f->label = fd->label;
    f->number = fd->number;
    f->name = upb_strdup(fd->name);
    f->ref.str = fd->type_name;
    m->size = f->byte_offset + type_info->size;
    max_align = UPB_MAX(max_align, type_info->align);
    if(fd->label == GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_LABEL_REQUIRED)
      m->num_required_fields++;

    /* Insert into the tables.  Note that f->ref will be uninitialized, even in
     * the tables' copies of *f, which is why we must update them separately
     * in upb_msg_setref() below. */
    struct upb_fieldsbynum_entry nument = {.e = {.key = fd->number}, .f = *f};
    struct upb_fieldsbyname_entry strent = {.e = {.key = *fd->name}, .f = *f};
    upb_inttable_insert(&m->fields_by_num, &nument.e);
    upb_strtable_insert(&m->fields_by_name, &strent.e);
  }

  if(max_align > 0)
    m->size = ALIGN_UP(m->size, max_align);
  free(fds);
}

void upb_msgdef_free(struct upb_msgdef *m)
{
  upb_inttable_free(&m->fields_by_num);
  upb_strtable_free(&m->fields_by_name);
  upb_string_unref(m->fqname);
  for (unsigned int i = 0; i < m->num_fields; i++) {
    upb_string_unref(m->fields[i].name);
  }
  free(m->fields);
}

void upb_msgdef_setref(struct upb_msgdef *m, struct upb_fielddef *f,
                       union upb_symbol_ref ref) {
  struct upb_fieldsbynum_entry *int_e = upb_inttable_fast_lookup(
      &m->fields_by_num, f->number, sizeof(struct upb_fieldsbynum_entry));
  struct upb_fieldsbyname_entry *str_e =
      upb_strtable_lookup(&m->fields_by_name, f->name);
  assert(int_e && str_e);
  f->ref = ref;
  int_e->f.ref = ref;
  str_e->f.ref = ref;
}


void upb_enumdef_init(struct upb_enumdef *e,
                   struct google_protobuf_EnumDescriptorProto *ed,
                   struct upb_context *c) {
  int num_values = ed->set_flags.has.value ? ed->value->len : 0;
  e->context = c;
  upb_atomic_refcount_init(&e->refcount, 0);
  upb_strtable_init(&e->nametoint, num_values,
                    sizeof(struct upb_enumdef_ntoi_entry));
  upb_inttable_init(&e->inttoname, num_values,
                    sizeof(struct upb_enumdef_iton_entry));

  for(int i = 0; i < num_values; i++) {
    google_protobuf_EnumValueDescriptorProto *value = ed->value->elements[i];
    struct upb_enumdef_ntoi_entry ntoi_entry = {.e = {.key = *value->name},
                                                .value = value->number};
    struct upb_enumdef_iton_entry iton_entry = {.e = {.key = value->number},
                                                .string = value->name};
    upb_strtable_insert(&e->nametoint, &ntoi_entry.e);
    upb_inttable_insert(&e->inttoname, &iton_entry.e);
  }
}

void upb_enumdef_free(struct upb_enumdef *e) {
  upb_strtable_free(&e->nametoint);
  upb_inttable_free(&e->inttoname);
}
