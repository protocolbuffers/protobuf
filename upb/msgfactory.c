
#include "upb/msgfactory.h"

#include "upb/port_def.inc"

static bool is_power_of_two(size_t val) {
  return (val & (val - 1)) == 0;
}

/* Align up to the given power of 2. */
static size_t align_up(size_t val, size_t align) {
  UPB_ASSERT(is_power_of_two(align));
  return (val + align - 1) & ~(align - 1);
}

static size_t div_round_up(size_t n, size_t d) {
  return (n + d - 1) / d;
}

static size_t upb_msgval_sizeof2(upb_fieldtype_t type) {
  switch (type) {
    case UPB_TYPE_DOUBLE:
    case UPB_TYPE_INT64:
    case UPB_TYPE_UINT64:
      return 8;
    case UPB_TYPE_ENUM:
    case UPB_TYPE_INT32:
    case UPB_TYPE_UINT32:
    case UPB_TYPE_FLOAT:
      return 4;
    case UPB_TYPE_BOOL:
      return 1;
    case UPB_TYPE_MESSAGE:
      return sizeof(void*);
    case UPB_TYPE_BYTES:
    case UPB_TYPE_STRING:
      return sizeof(upb_strview);
  }
  UPB_UNREACHABLE();
}

static uint8_t upb_msg_fielddefsize(const upb_fielddef *f) {
  if (upb_fielddef_isseq(f)) {
    return sizeof(void*);
  } else {
    return upb_msgval_sizeof2(upb_fielddef_type(f));
  }
}


/** upb_msglayout *************************************************************/

static void upb_msglayout_free(upb_msglayout *l) {
  upb_gfree(l);
}

static size_t upb_msglayout_place(upb_msglayout *l, size_t size) {
  size_t ret;

  l->size = align_up(l->size, size);
  ret = l->size;
  l->size += size;
  return ret;
}

static bool upb_msglayout_init(const upb_msgdef *m,
                               upb_msglayout *l,
                               upb_msgfactory *factory) {
  upb_msg_field_iter it;
  upb_msg_oneof_iter oit;
  size_t hasbit;
  size_t submsg_count = 0;
  const upb_msglayout **submsgs;
  upb_msglayout_field *fields;

  for (upb_msg_field_begin(&it, m);
       !upb_msg_field_done(&it);
       upb_msg_field_next(&it)) {
    const upb_fielddef* f = upb_msg_iter_field(&it);
    if (upb_fielddef_issubmsg(f)) {
      submsg_count++;
    }
  }

  memset(l, 0, sizeof(*l));

  fields = upb_gmalloc(upb_msgdef_numfields(m) * sizeof(*fields));
  submsgs = upb_gmalloc(submsg_count * sizeof(*submsgs));

  if ((!fields && upb_msgdef_numfields(m)) ||
      (!submsgs && submsg_count)) {
    /* OOM. */
    upb_gfree(fields);
    upb_gfree(submsgs);
    return false;
  }

  l->field_count = upb_msgdef_numfields(m);
  l->fields = fields;
  l->submsgs = submsgs;

  /* Allocate data offsets in three stages:
   *
   * 1. hasbits.
   * 2. regular fields.
   * 3. oneof fields.
   *
   * OPT: There is a lot of room for optimization here to minimize the size.
   */

  /* Allocate hasbits and set basic field attributes. */
  submsg_count = 0;
  for (upb_msg_field_begin(&it, m), hasbit = 0;
       !upb_msg_field_done(&it);
       upb_msg_field_next(&it)) {
    const upb_fielddef* f = upb_msg_iter_field(&it);
    upb_msglayout_field *field = &fields[upb_fielddef_index(f)];

    field->number = upb_fielddef_number(f);
    field->descriptortype = upb_fielddef_descriptortype(f);
    field->label = upb_fielddef_label(f);

    if (upb_fielddef_issubmsg(f)) {
      const upb_msglayout *sub_layout =
          upb_msgfactory_getlayout(factory, upb_fielddef_msgsubdef(f));
      field->submsg_index = submsg_count++;
      submsgs[field->submsg_index] = sub_layout;
    }

    if (upb_fielddef_haspresence(f) && !upb_fielddef_containingoneof(f)) {
      field->presence = (hasbit++);
    } else {
      field->presence = 0;
    }
  }

  /* Account for space used by hasbits. */
  l->size = div_round_up(hasbit, 8);

  /* Allocate non-oneof fields. */
  for (upb_msg_field_begin(&it, m); !upb_msg_field_done(&it);
       upb_msg_field_next(&it)) {
    const upb_fielddef* f = upb_msg_iter_field(&it);
    size_t field_size = upb_msg_fielddefsize(f);
    size_t index = upb_fielddef_index(f);

    if (upb_fielddef_containingoneof(f)) {
      /* Oneofs are handled separately below. */
      continue;
    }

    fields[index].offset = upb_msglayout_place(l, field_size);
  }

  /* Allocate oneof fields.  Each oneof field consists of a uint32 for the case
   * and space for the actual data. */
  for (upb_msg_oneof_begin(&oit, m); !upb_msg_oneof_done(&oit);
       upb_msg_oneof_next(&oit)) {
    const upb_oneofdef* o = upb_msg_iter_oneof(&oit);
    upb_oneof_iter fit;

    size_t case_size = sizeof(uint32_t);  /* Could potentially optimize this. */
    size_t field_size = 0;
    uint32_t case_offset;
    uint32_t data_offset;

    /* Calculate field size: the max of all field sizes. */
    for (upb_oneof_begin(&fit, o);
         !upb_oneof_done(&fit);
         upb_oneof_next(&fit)) {
      const upb_fielddef* f = upb_oneof_iter_field(&fit);
      field_size = UPB_MAX(field_size, upb_msg_fielddefsize(f));
    }

    /* Align and allocate case offset. */
    case_offset = upb_msglayout_place(l, case_size);
    data_offset = upb_msglayout_place(l, field_size);

    for (upb_oneof_begin(&fit, o);
         !upb_oneof_done(&fit);
         upb_oneof_next(&fit)) {
      const upb_fielddef* f = upb_oneof_iter_field(&fit);
      fields[upb_fielddef_index(f)].offset = data_offset;
      fields[upb_fielddef_index(f)].presence = ~case_offset;
    }
  }

  /* Size of the entire structure should be a multiple of its greatest
   * alignment.  TODO: track overall alignment for real? */
  l->size = align_up(l->size, 8);

  return true;
}


/** upb_msgfactory ************************************************************/

struct upb_msgfactory {
  const upb_symtab *symtab;  /* We own a ref. */
  upb_inttable layouts;
};

upb_msgfactory *upb_msgfactory_new(const upb_symtab *symtab) {
  upb_msgfactory *ret = upb_gmalloc(sizeof(*ret));

  ret->symtab = symtab;
  upb_inttable_init(&ret->layouts, UPB_CTYPE_PTR);

  return ret;
}

void upb_msgfactory_free(upb_msgfactory *f) {
  upb_inttable_iter i;
  upb_inttable_begin(&i, &f->layouts);
  for(; !upb_inttable_done(&i); upb_inttable_next(&i)) {
    upb_msglayout *l = upb_value_getptr(upb_inttable_iter_value(&i));
    upb_msglayout_free(l);
  }

  upb_inttable_uninit(&f->layouts);
  upb_gfree(f);
}

const upb_symtab *upb_msgfactory_symtab(const upb_msgfactory *f) {
  return f->symtab;
}

const upb_msglayout *upb_msgfactory_getlayout(upb_msgfactory *f,
                                              const upb_msgdef *m) {
  upb_value v;
  UPB_ASSERT(upb_symtab_lookupmsg(f->symtab, upb_msgdef_fullname(m)) == m);
  UPB_ASSERT(!upb_msgdef_mapentry(m));

  if (upb_inttable_lookupptr(&f->layouts, m, &v)) {
    UPB_ASSERT(upb_value_getptr(v));
    return upb_value_getptr(v);
  } else {
    /* In case of circular dependency, layout has to be inserted first. */
    upb_msglayout *l = upb_gmalloc(sizeof(*l));
    upb_msgfactory *mutable_f = (void*)f;
    upb_inttable_insertptr(&mutable_f->layouts, m, upb_value_ptr(l));
    UPB_ASSERT(l);
    if (!upb_msglayout_init(m, l, f)) {
      upb_msglayout_free(l);
    }
    return l;
  }
}
