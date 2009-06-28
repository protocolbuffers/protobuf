/*
 * upb - a minimalist implementation of protocol buffers.
 *
 */

#include <stdlib.h>
#include "descriptor.h"
#include "upb_msg.h"
#include "upb_parse.h"

#define ALIGN_UP(p, t) (t + ((p - 1) & (~t - 1)))

static uint32_t max(uint32_t a, uint32_t b) { return a > b ? a : b; }

static int div_round_up(int numerator, int denominator) {
  /* cf. http://stackoverflow.com/questions/17944/how-to-round-up-the-result-of-integer-division */
  return numerator > 0 ? (numerator - 1) / denominator + 1 : 0;
}

static int compare_fields(const void *e1, const void *e2) {
  const google_protobuf_FieldDescriptorProto *fd1 = e1, *fd2 = e2;
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

bool upb_msg_init(struct upb_msg *m, struct google_protobuf_DescriptorProto *d)
{
  /* TODO: more complete validation. */
  if(!d->set_flags.has.field) return false;

  upb_inttable_init(&m->fields_by_num, d->field->len,
                    sizeof(struct upb_fieldsbynum_entry));
  upb_strtable_init(&m->fields_by_name, d->field->len,
                    sizeof(struct upb_fieldsbyname_entry));

  m->num_fields = d->field->len;
  m->set_flags_bytes = div_round_up(m->num_fields, 8);
  /* These are incremented in the loop. */
  m->num_required_fields = 0;
  m->size = m->set_flags_bytes;

  m->fields = malloc(sizeof(*m->fields) * m->num_fields);
  m->field_descriptors = malloc(sizeof(*m->field_descriptors) * m->num_fields);
  for(unsigned int i = 0; i < m->num_fields; i++) {
    /* We count on the caller to keep this pointer alive. */
    m->field_descriptors[i] = d->field->elements[i];
  }
  qsort(m->field_descriptors, m->num_fields, sizeof(void*), compare_fields);

  size_t max_align = 0;

  for(unsigned int i = 0; i < m->num_fields; i++) {
    struct upb_msg_field *f = &m->fields[i];
    google_protobuf_FieldDescriptorProto *fd = m->field_descriptors[i];
    struct upb_type_info *type_info = &upb_type_info[fd->type];

    /* General alignment rules are: each member must be at an address that is a
     * multiple of that type's alignment.  Also, the size of the structure as
     * a whole must be a multiple of the greatest alignment of any member. */
    f->field_index = i;
    f->byte_offset = ALIGN_UP(m->size, type_info->align);
    m->size = f->byte_offset + type_info->size;
    max_align = max(max_align, type_info->align);
    if(fd->label == GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_LABEL_REQUIRED)
      m->num_required_fields++;

    /* Insert into the tables.  Note that f->ref will be uninitialized, even in
     * the tables' copies of *f, which is why we must update them separately
     * when the references are resolved. */
    struct upb_fieldsbynum_entry nument = {.e = {.key = fd->number}, .f = *f};
    struct upb_fieldsbyname_entry strent = {.e = {.key = *fd->name}, .f = *f};
    upb_inttable_insert(&m->fields_by_num, &nument.e);
    upb_strtable_insert(&m->fields_by_name, &strent.e);
  }

  m->size = ALIGN_UP(m->size, max_align);
  return true;
}

void upb_msg_free(struct upb_msg *m)
{
  upb_inttable_free(&m->fields_by_num);
  upb_strtable_free(&m->fields_by_name);
  free(m->fields);
}

#if 0
struct parse_frame_data {
  struct upb_msg *m;
  void *data;
};

static void set_frame_data(struct upb_parse_state *s, struct upb_msg *m)
{
}

static upb_field_type_t tag_cb(struct upb_parse_state *s, struct upb_tag *tag,
                               void **user_field_desc)
{
  struct upb_msg *m = (struct upb_msg*)s->top->user_data;
  struct upb_msg_field *f = upb_msg_fieldbynum(m, tag->field_number);
  if(!f || !upb_check_type(tag->wire_type, f->type))
    return 0;  /* Skip unknown or fields of the wrong type. */
  *user_field_desc = f->ref.msg;
  return f->type;
}

static void value_cb(struct upb_parse_state *s, union upb_value *v,
                     void *str, void *user_field_desc)
{
  *user_field_desc = f->ref.msg;
}

static void submsg_start_cb(struct upb_parse_state *s, void *user_field_desc)
{
  set_frame_data(s, user_field_desc);
}

void *upb_msg_parse(struct upb_msg *m, struct upb_string *str)
{
  struct upb_parse_state s;
  upb_parse_state_init(&s, sizeof(struct parse_frame_data));
  set_frame_data(&s, m);
  s.tag_cb = tag_cb;
  s.value_cb = value_cb;
  s.submsg_start_cb = submsg_start_cb;
}
#endif
