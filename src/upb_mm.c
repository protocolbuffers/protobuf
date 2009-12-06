/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2009 Joshua Haberman.  See LICENSE for details.
 */

#include "upb_mm.h"
#include "upb_string.h"
#include "upb_array.h"
#include "upb_msg.h"

static void upb_mm_destroy(union upb_value_ptr p, upb_mm_ptrtype type)
{
  if(*p.msg) {
    union upb_mmptr mmptr = upb_mmptr_read(p, type);
    upb_mm_unref(mmptr, type);
  }
}

void upb_msg_destroy(struct upb_msg *msg) {
  for(upb_field_count_t i = 0; i < msg->def->num_fields; i++) {
    struct upb_fielddef *f = &msg->def->fields[i];
    if(!upb_msg_isset(msg, f) || !upb_field_ismm(f)) continue;
    upb_mm_destroy(upb_msg_getptr(msg, f), upb_field_ptrtype(f));
  }
  upb_def_unref(UPB_UPCAST(msg->def));
  free(msg);
}

void upb_array_destroy(struct upb_array *arr)
{
  if(upb_elem_ismm(arr->fielddef)) {
    upb_arraylen_t i;
    /* Unref elements. */
    for(i = 0; i < arr->size; i++) {
      union upb_value_ptr p = upb_array_getelementptr(arr, i);
      upb_mm_destroy(p, upb_elem_ptrtype(arr->fielddef));
    }
  }
  if(arr->size != 0) free(arr->elements._void);
  free(arr);
}

static union upb_mmptr upb_mm_newptr(upb_mm_ptrtype type,
                                     struct upb_fielddef *f)
{
  union upb_mmptr p = {NULL};
  switch(type) {
    case UPB_MM_MSG_REF: p.msg = upb_msg_new(upb_downcast_msgdef(f->def));
    case UPB_MM_STR_REF: p.str = upb_string_new();
    case UPB_MM_ARR_REF: p.arr = upb_array_new(f);
    default: assert(false); break;
  }
  return p;
}

static struct upb_mm_ref *find_or_create_ref(struct upb_mm_ref *fromref,
                                             struct upb_mm *mm,
                                             union upb_mmptr p, upb_mm_ptrtype type,
                                             bool *created)
{
  struct upb_mmhead *head = upb_mmhead_addr(p, type);
  struct upb_mm_ref **ref = &head->refs;
  while(*ref && (*ref)->mm <= mm) {
    if((*ref)->mm == mm) {
      *created = false;
      return *ref;
    }
    ref = &((*ref)->next);
  }
  *created = true;
  struct upb_mm_ref *newref = mm->newref_cb(fromref, p, type);
  newref->p = p;
  newref->type = type;
  newref->mm = mm;
  newref->next = *ref;
  *ref = newref;
  return newref;
}

struct upb_mm_ref *upb_mm_getref(union upb_mmptr p, upb_mm_ptrtype type,
                                 struct upb_mm *mm, bool *created)
{
  return find_or_create_ref(NULL, mm, p, type, created);
}

struct upb_mm_ref *upb_mm_newmsg_ref(struct upb_msgdef *def, struct upb_mm *mm)
{
  struct upb_msg *msg = upb_msg_new(def);
  union upb_mmptr mmptr = {.msg = msg};
  bool created;
  struct upb_mm_ref *ref = find_or_create_ref(NULL, mm, mmptr, UPB_MM_MSG_REF, &created);
  upb_mm_unref(mmptr, UPB_MM_MSG_REF);  /* Shouldn't have any counted refs. */
  assert(created);
  return ref;
}

struct upb_mm_ref *upb_mm_getfieldref(struct upb_mm_ref *msgref,
                                      struct upb_fielddef *f,
                                      bool *refcreated)
{
  assert(upb_field_ismm(f));
  upb_mm_ptrtype ptrtype = upb_field_ptrtype(f);
  struct upb_msg *msg = msgref->p.msg;
  union upb_mmptr val;
  union upb_value_ptr p = upb_msg_getptr(msg, f);

  /* Create the upb value if it doesn't already exist. */
  if(!upb_msg_isset(msg, f)) {
    upb_msg_set(msg, f);
    val = upb_mm_newptr(ptrtype, f);
    upb_mmptr_write(p, val, ptrtype);
  } else {
    val = upb_mmptr_read(p, ptrtype);
  }

  return find_or_create_ref(msgref, msgref->mm, val, ptrtype, refcreated);
}

struct upb_mm_ref *upb_mm_getelemref(struct upb_mm_ref *arrref, upb_arraylen_t i,
                                     bool *refcreated)
{
  struct upb_array *arr = arrref->p.arr;
  struct upb_fielddef *f = arr->fielddef;
  assert(upb_elem_ismm(f));
  assert(i < arr->len);
  union upb_value_ptr p = upb_array_getelementptr(arr, i);
  upb_mm_ptrtype type = upb_elem_ptrtype(f);
  union upb_mmptr val = upb_mmptr_read(p, type);
  return find_or_create_ref(arrref, arrref->mm, val, type, refcreated);
}

void upb_mm_release(struct upb_mm_ref *ref)
{
  struct upb_mm_ref **ref_head = (void*)ref->p.msg;
  struct upb_mm_ref **ref_elem = ref_head;
  struct upb_mm *mm = ref->mm;
  while(true) {
    assert(*ref_elem);  /* Client asserts r->mm is in the list. */
    if((*ref_elem)->mm == mm) {
      *ref_elem = (*ref_elem)->next;  /* Remove from the list. */
      break;
    }
  }

  if(upb_mmhead_norefs(&ref->p.msg->mmhead)) {
    /* Destroy the dynamic object. */
    switch(ref->type) {
      case UPB_MM_MSG_REF:
        upb_msg_destroy(ref->p.msg);
        break;
      case UPB_MM_ARR_REF:
        upb_array_destroy(ref->p.arr);
        break;
      case UPB_MM_STR_REF:
        upb_string_destroy(ref->p.str);
        break;
      default: assert(false); break;
    }
  }
}

void upb_mm_msg_set(struct upb_mm_ref *from_msg_ref, struct upb_mm_ref *to_ref,
                    struct upb_fielddef *f)
{
  assert(upb_field_ismm(f));
  union upb_mmptr fromval = from_msg_ref->p;
  union upb_mmptr toval = to_ref->p;
  union upb_value_ptr field_p = upb_msg_getptr(fromval.msg, f);
  upb_mm_ptrtype type = upb_field_ptrtype(f);
  if(upb_msg_isset(fromval.msg, f)) {
    union upb_mmptr existingval = upb_mmptr_read(field_p, type);
    if(existingval.msg == toval.msg)
      return;  /* Setting to its existing value, do nothing. */
    upb_mm_unref(existingval, type);
  }
  upb_msg_set(fromval.msg, f);
  upb_mmptr_write(field_p, toval, type);
  upb_mm_ref(toval, type);
}

void upb_mm_msgclear(struct upb_mm_ref *from_msg_ref, struct upb_fielddef *f)
{
  assert(upb_field_ismm(f));
  union upb_mmptr fromval = from_msg_ref->p;
  upb_mm_ptrtype type = upb_field_ptrtype(f);
  if(upb_msg_isset(fromval.msg, f)) {
    union upb_value_ptr field_p = upb_msg_getptr(fromval.msg, f);
    union upb_mmptr existingval = upb_mmptr_read(field_p, type);
    upb_msg_unset(fromval.msg, f);
    upb_mm_unref(existingval, type);
  }
}

void upb_mm_msgclear_all(struct upb_mm_ref *from)
{
  struct upb_msgdef *def = from->p.msg->def;
  for(upb_field_count_t i = 0; i < def->num_fields; i++) {
    struct upb_fielddef *f = &def->fields[i];
    if(!upb_field_ismm(f)) continue;
    upb_mm_msgclear(from, f);
  }
}

void upb_mm_arr_set(struct upb_mm_ref *from, struct upb_mm_ref *to,
                    upb_arraylen_t i, upb_field_type_t type)
{
  (void)from;
  (void)to;
  (void)i;
  (void)type;
}
