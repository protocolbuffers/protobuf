/*
 * upb - a minimalist implementation of protocol buffers.
 *
 */

#include <stdlib.h>
#include "descriptor.h"
#include "upb_msg.h"

#define ALIGN_UP(p, t) (t + ((p - 1) & (~t - 1)))

static uint32_t max(uint32_t a, uint32_t b) { return a > b ? a : b; }

static int div_round_up(int numerator, int denominator) {
  /* cf. http://stackoverflow.com/questions/17944/how-to-round-up-the-result-of-integer-division */
  return numerator > 0 ? (numerator - 1) / denominator + 1 : 0;
}

static int compare_fields(const void *e1, const void *e2) {
  const google_protobuf_FieldDescriptorProto *f1  = e1, *f2 = e2;
  /* Required fields go before non-required. */
  if(f1->label == GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_LABEL_REQUIRED &&
     f2->label != GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_LABEL_REQUIRED) {
    return -1;
  } else if(f1->label != GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_LABEL_REQUIRED &&
            f2->label == GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_LABEL_REQUIRED) {
    return 1;
  } else {
    /* Within required and non-required field lists, list in number order. */
    return f1->number - f2->number;
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

  m->fields = malloc(sizeof(struct upb_msg_field) * d->field->len);
  m->num_fields = d->field->len;
  m->set_flags_bytes = div_round_up(m->num_fields, 8);

  /* These are incremented in the loop. */
  m->num_required_fields = 0;
  m->size = m->set_flags_bytes;

  qsort(m->fields, d->field->len, sizeof(struct upb_msg_field), compare_fields);

  size_t max_align = 0;

  for(unsigned int i = 0; i < d->field->len; i++) {
    struct upb_msg_field *f = &m->fields[i];
    google_protobuf_FieldDescriptorProto *fd;  /* TODO */
    struct upb_type_info *type_info = &upb_type_info[f->type];
    f->field_index = i;
    f->type = fd->type;
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
