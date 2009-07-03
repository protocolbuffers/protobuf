/*
 * upb - a minimalist implementation of protocol buffers.
 *
 */

#include <stdlib.h>
#include "descriptor.h"
#include "upb_msg.h"
#include "upb_parse.h"

#define ALIGN_UP(p, t) (t + ((p - 1) & (~t - 1)))

static int div_round_up(int numerator, int denominator) {
  /* cf. http://stackoverflow.com/questions/17944/how-to-round-up-the-result-of-integer-division */
  return numerator > 0 ? (numerator - 1) / denominator + 1 : 0;
}

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

bool upb_msg_init(struct upb_msg *m, struct google_protobuf_DescriptorProto *d)
{
  /* TODO: more complete validation.
   * TODO: re-enable this check when we properly set this flag. */
  //if(!d->set_flags.has.field) return false;

  upb_inttable_init(&m->fields_by_num, d->field->len,
                    sizeof(struct upb_fieldsbynum_entry));
  upb_strtable_init(&m->fields_by_name, d->field->len,
                    sizeof(struct upb_fieldsbyname_entry));

  m->descriptor = d;
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
  free(m->field_descriptors);
}

void *upb_msg_new(struct upb_msg *m)
{
  void *msg = malloc(m->size);
  memset(msg, 0, m->size);  /* Clear all pointers, values, and set bits. */
  return msg;
}

//void upb_msg_free(void *msg, struct upb_msg *m, bool free_submsgs);

void upb_msg_ref(struct upb_msg *m, struct upb_msg_field *f,
                 union upb_symbol_ref ref) {
  struct google_protobuf_FieldDescriptorProto *d =
      upb_msg_field_descriptor(f, m);
  struct upb_fieldsbynum_entry *int_e = upb_inttable_lookup(
      &m->fields_by_num, d->number, sizeof(struct upb_fieldsbynum_entry));
  struct upb_fieldsbyname_entry *str_e =
      upb_strtable_lookup(&m->fields_by_name, d->name);
  assert(int_e && str_e);
  f->ref = ref;
  int_e->f.ref = ref;
  str_e->f.ref = ref;
}

struct mm_upb_string {
  struct upb_string s;
  uint32_t size;
  char *data;
};

struct mm_upb_array {
  struct upb_array a;
  uint32_t size;
  char *data;
};

static uint32_t round_up_to_pow2(uint32_t v)
{
#ifdef __GNUC__
  return (1U<<31) >> (__builtin_clz(v-1)+1);
#else
  /* cf. http://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2 */
  v--;
  v |= v >> 1;
  v |= v >> 2;
  v |= v >> 4;
  v |= v >> 8;
  v |= v >> 16;
  v++;
  return v;
#endif
}

void upb_msg_reuse_str(struct upb_string **str, uint32_t size)
{
  if(!*str) *str = malloc(sizeof(struct mm_upb_string));
  struct mm_upb_string *s = (void*)*str;
  if(s->size < size) {
    size = max(16, round_up_to_pow2(size));
    s->data = realloc(s->data, size);
    s->size = size;
  }
  s->s.ptr = s->data;
}

void upb_msg_reuse_array(struct upb_array **arr, uint32_t n, upb_field_type_t t)
{
  if(!*arr) *arr = malloc(sizeof(struct mm_upb_array));
  struct mm_upb_array *a = (void*)*arr;
  if(a->size < n) {
    n = max(16, round_up_to_pow2(n));
    a->a.elements._void = realloc(a->a.elements._void, n*upb_type_info[t].size);
    a->size = n;
  }
}

void upb_msg_reuse_strref(struct upb_string **str) { upb_msg_reuse_str(str, 0); }

void upb_msg_reuse_submsg(void **msg, struct upb_msg *m)
{
  if(!*msg) *msg = malloc(m->size);
  upb_msg_clear(*msg, m);  /* Clears set bits, leaves pointers. */
}

/* Parser. */

struct parse_frame_data {
  struct upb_msg *m;
  void *data;
};

static void set_frame_data(struct upb_parse_state *s, struct upb_msg *m,
                           void *data)
{
  struct parse_frame_data *frame = (void*)s->top->user_data;
  frame->m = m;
  frame->data = data;
}

static upb_field_type_t tag_cb(struct upb_parse_state *s, struct upb_tag *tag,
                               void **user_field_desc)
{
  struct parse_frame_data *frame = (void*)s->top->user_data;
  struct upb_msg_field *f = upb_msg_fieldbynum(frame->m, tag->field_number);
  if(!f || !upb_check_type(tag->wire_type, f->type))
    return 0;  /* Skip unknown or fields of the wrong type. */
  *user_field_desc = f;
  return f->type;
}

static union upb_value_ptr get_value_ptr(void *data, struct upb_msg_field *f)
{
  union upb_value_ptr p = upb_msg_get_ptr(data, f);
  if(f->label == GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_LABEL_REPEATED) {
    size_t len = upb_msg_is_set(data, f) ? (*p.array)->len : 0;
    upb_msg_reuse_array(p.array, len, f->type);
    (*p.array)->len = len + 1;
    p = upb_array_getelementptr(*p.array, len, f->type);
  }
  return p;
}

static upb_status_t value_cb(struct upb_parse_state *s, void **buf, void *end,
                             void *user_field_desc)
{
  struct parse_frame_data *frame = (void*)s->top->user_data;
  struct upb_msg_field *f = user_field_desc;
  union upb_value_ptr p = get_value_ptr(frame->data, f);
  UPB_CHECK(upb_parse_value(buf, end, f->type, p));
  return UPB_STATUS_OK;
}

static upb_status_t str_cb(struct upb_parse_state *_s, struct upb_string *str,
                           void *user_field_desc)
{
  struct upb_msg_parse_state *s = (void*)_s;
  struct parse_frame_data *frame = (void*)s->s.top->user_data;
  struct upb_msg_field *f = user_field_desc;
  union upb_value_ptr p = get_value_ptr(frame->data, f);
  if(s->byref) {
    upb_msg_reuse_strref(p.string);
    **p.string = *str;
  } else {
    upb_msg_reuse_str(p.string, str->byte_len);
    upb_strcpy(*p.string, str);
  }
  return UPB_STATUS_OK;
}

static void submsg_start_cb(struct upb_parse_state *_s, void *user_field_desc)
{
  struct upb_msg_parse_state *s = (void*)_s;
  struct upb_msg_field *f = user_field_desc;
  struct parse_frame_data *frame = (void*)s->s.top->user_data;
  union upb_value_ptr p = upb_msg_get_ptr(frame->data, f);
  upb_msg_reuse_submsg(*p.message, f->ref.msg);
  if(!s->merge) upb_msg_clear(frame->data, f->ref.msg);
  set_frame_data(&s->s, f->ref.msg, *p.message);
}


void upb_msg_parse_init(struct upb_msg_parse_state *s, void *msg,
                        struct upb_msg *m, bool merge, bool byref)
{
  upb_parse_init(&s->s, sizeof(struct parse_frame_data));
  s->merge = merge;
  s->byref = byref;
  if(!merge && msg == NULL) msg = upb_msg_new(m);
  set_frame_data(&s->s, m, msg);
  s->s.tag_cb = tag_cb;
  s->s.value_cb = value_cb;
  s->s.str_cb = str_cb;
  s->s.submsg_start_cb = submsg_start_cb;
}

void upb_msg_parse_free(struct upb_msg_parse_state *s)
{
  upb_parse_free(&s->s);
}

upb_status_t upb_msg_parse(struct upb_msg_parse_state *s,
                           void *data, size_t len, size_t *read)
{
  return upb_parse(&s->s, data, len, read);
}

void *upb_alloc_and_parse(struct upb_msg *m, struct upb_string *str, bool byref)
{
  struct upb_msg_parse_state s;
  void *msg = upb_msg_new(m);
  upb_msg_parse_init(&s, msg, m, false, byref);
  size_t read;
  upb_status_t status = upb_msg_parse(&s, str->ptr, str->byte_len, &read);
  if(status == UPB_STATUS_OK && read == str->byte_len) {
    return msg;
  } else {
    upb_msg_free(msg);
    return NULL;
  }
}
