// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "python/repeated.h"

#include "google/protobuf/breaking_changes.h"
#include "python/convert.h"
#include "python/message.h"
#include "python/protobuf.h"

// Must be last.
#include "upb/port/def.inc"

#define PyUpb_SUPPORT_BUFFER_VIEW 0

static PyObject* PyUpb_RepeatedCompositeContainer_Append(PyObject* _self,
                                                         PyObject* value);
static PyObject* PyUpb_RepeatedScalarContainer_Append(PyObject* _self,
                                                      PyObject* value);

// Wrapper for a repeated field.
typedef struct {
  // clang-format off
  PyObject_HEAD
  PyObject* arena;
  // clang-format on
  // The field descriptor (PyObject*).
  // The low bit indicates whether the container is reified (see ptr below).
  //   - low bit set: repeated field is a stub (no underlying data).
  //   - low bit clear: repeated field is reified (points to upb_Array).
  uintptr_t field;
  union {
    PyObject* parent;  // stub: owning pointer to parent message.
    upb_Array* arr;    // reified: the data for this array.
  } ptr;
  int version;
} PyUpb_RepeatedContainer;

static bool PyUpb_RepeatedContainer_IsStub(PyUpb_RepeatedContainer* self) {
  return self->field & 1;
}

static PyObject* PyUpb_RepeatedContainer_GetFieldDescriptor(
    PyUpb_RepeatedContainer* self) {
  return (PyObject*)(self->field & ~(uintptr_t)1);
}

static const upb_FieldDef* PyUpb_RepeatedContainer_GetField(
    PyUpb_RepeatedContainer* self) {
  return PyUpb_FieldDescriptor_GetDef(
      PyUpb_RepeatedContainer_GetFieldDescriptor(self));
}

// If the repeated field is reified, returns it.  Otherwise, returns NULL.
// If NULL is returned, the object is empty and has no underlying data.
static upb_Array* PyUpb_RepeatedContainer_GetIfReified(
    PyUpb_RepeatedContainer* self) {
  return PyUpb_RepeatedContainer_IsStub(self) ? NULL : self->ptr.arr;
}

upb_Array* PyUpb_RepeatedContainer_Reify(PyObject* _self, upb_Array* arr,
                                         PyUpb_WeakMap* subobj_map,
                                         intptr_t iter) {
  PyUpb_RepeatedContainer* self = (PyUpb_RepeatedContainer*)_self;
  assert(PyUpb_RepeatedContainer_IsStub(self));
  const upb_FieldDef* f = PyUpb_RepeatedContainer_GetField(self);
  if (!arr) {
    upb_Arena* arena = PyUpb_Arena_Get(self->arena);
    arr = upb_Array_New(arena, upb_FieldDef_CType(f));
  }
  if (subobj_map) {
    PyUpb_WeakMap_DeleteIter(subobj_map, &iter);
  } else {
    if (!PyUpb_Message_SetConcreteSubobj(
            self->ptr.parent, f, (upb_MessageValue){.array_val = arr})) {
      return NULL;
    }
  }
  PyUpb_ObjCache_Add(arr, &self->ob_base);
  Py_DECREF(self->ptr.parent);
  self->ptr.arr = arr;  // Overwrites self->ptr.parent.
  self->field &= ~(uintptr_t)1;
  assert(!PyUpb_RepeatedContainer_IsStub(self));
  return arr;
}

bool PyUpb_RepeatedContainer_IsFrozen(PyUpb_RepeatedContainer* self) {
  if (PyUpb_RepeatedContainer_IsStub(self)) {
    return PyUpb_Message_IsFrozen(self->ptr.parent);
  } else {
    return upb_Array_IsFrozen(self->ptr.arr);
  }
}

upb_Array* PyUpb_RepeatedContainer_AssureWritable(PyObject* _self) {
  PyUpb_RepeatedContainer* self = (PyUpb_RepeatedContainer*)_self;
  if (PyUpb_RepeatedContainer_IsFrozen(self)) {
    return (upb_Array*)PyUpb_SetFrozenErrorWithMsg("Container is immutable");
  }

  self->version++;
  upb_Array* arr = PyUpb_RepeatedContainer_GetIfReified(self);
  if (arr) return arr;  // Already writable.

  return PyUpb_RepeatedContainer_Reify((PyObject*)self, NULL, NULL, 0);
}

void PyUpb_RepeatedContainer_Invalidate(PyObject* obj) {
  PyUpb_RepeatedContainer* self = (PyUpb_RepeatedContainer*)obj;
  self->version++;
}

static bool PyUpb_RepeatedContainer_AssureVersionUnchanged(
    PyUpb_RepeatedContainer* self, int version, const char* op) {
  if (self->version != version) {
    PyErr_Format(PyExc_RuntimeError, "Repeated field modified during %s", op);
    return false;
  }
  return true;
}

static void PyUpb_RepeatedContainer_Dealloc(PyObject* _self) {
  PyUpb_RepeatedContainer* self = (PyUpb_RepeatedContainer*)_self;
  Py_DECREF(self->arena);
  if (PyUpb_RepeatedContainer_IsStub(self)) {
    PyUpb_Message_CacheDelete(self->ptr.parent,
                              PyUpb_RepeatedContainer_GetField(self));
    Py_DECREF(self->ptr.parent);
  } else {
    PyUpb_ObjCache_Delete(self->ptr.arr);
  }
  Py_DECREF(PyUpb_RepeatedContainer_GetFieldDescriptor(self));
  PyUpb_Dealloc(self);
}

static PyTypeObject* PyUpb_RepeatedContainer_GetClass(const upb_FieldDef* f) {
  assert(upb_FieldDef_IsRepeated(f) && !upb_FieldDef_IsMap(f));
  PyUpb_ModuleState* state = PyUpb_ModuleState_MaybeGet();
  if (!state) return NULL;
  return upb_FieldDef_IsSubMessage(f) ? state->repeated_composite_container_type
                                      : state->repeated_scalar_container_type;
}

static Py_ssize_t PyUpb_RepeatedContainer_Length(PyObject* self) {
  upb_Array* arr =
      PyUpb_RepeatedContainer_GetIfReified((PyUpb_RepeatedContainer*)self);
  return arr ? upb_Array_Size(arr) : 0;
}

PyObject* PyUpb_RepeatedContainer_NewStub(PyObject* parent,
                                          const upb_FieldDef* f,
                                          PyObject* arena) {
  PyTypeObject* cls = PyUpb_RepeatedContainer_GetClass(f);
  if (!cls) {
    PyErr_SetString(PyExc_RuntimeError, "Interpreter is finalizing");
    return NULL;
  }
  PyUpb_RepeatedContainer* repeated = (void*)PyType_GenericAlloc(cls, 0);
  repeated->arena = arena;
  repeated->field = (uintptr_t)PyUpb_FieldDescriptor_Get(f) | 1;
  repeated->ptr.parent = parent;
  repeated->version = 0;
  Py_INCREF(arena);
  Py_INCREF(parent);
  return &repeated->ob_base;
}

PyObject* PyUpb_RepeatedContainer_GetOrCreateWrapper(upb_Array* arr,
                                                     const upb_FieldDef* f,
                                                     PyObject* arena) {
  PyObject* ret = PyUpb_ObjCache_Get(arr);
  if (ret) return ret;

  PyTypeObject* cls = PyUpb_RepeatedContainer_GetClass(f);
  if (!cls) {
    PyErr_SetString(PyExc_RuntimeError, "Interpreter is finalizing");
    return NULL;
  }
  PyUpb_RepeatedContainer* repeated = (void*)PyType_GenericAlloc(cls, 0);
  if (repeated == NULL) return NULL;
  repeated->arena = arena;
  repeated->field = (uintptr_t)PyUpb_FieldDescriptor_Get(f);
  repeated->ptr.arr = arr;
  repeated->version = 0;
  ret = &repeated->ob_base;
  Py_INCREF(arena);
  PyUpb_ObjCache_Add(arr, ret);
  return ret;
}

static PyObject* PyUpb_RepeatedContainer_MergeFrom(PyObject* _self,
                                                   PyObject* args);

PyObject* PyUpb_RepeatedContainer_DeepCopy(PyObject* _self, PyObject* value) {
  PyUpb_RepeatedContainer* self = (PyUpb_RepeatedContainer*)_self;
  PyUpb_RepeatedContainer* clone =
      (void*)PyType_GenericAlloc(Py_TYPE(_self), 0);
  if (clone == NULL) return NULL;
  const upb_FieldDef* f = PyUpb_RepeatedContainer_GetField(self);
  clone->arena = PyUpb_Arena_New();
  clone->field = (uintptr_t)PyUpb_FieldDescriptor_Get(f);
  clone->ptr.arr =
      upb_Array_New(PyUpb_Arena_Get(clone->arena), upb_FieldDef_CType(f));
  clone->version = 0;
  PyUpb_ObjCache_Add(clone->ptr.arr, (PyObject*)clone);
  PyObject* result = PyUpb_RepeatedContainer_MergeFrom((PyObject*)clone, _self);
  if (!result) {
    Py_DECREF(clone);
    return NULL;
  }
  Py_DECREF(result);
  return (PyObject*)clone;
}

#if PyUpb_SUPPORT_BUFFER_VIEW
static bool IsContiguous1DAndMatchesFieldType(const Py_buffer* view,
                                              const upb_FieldDef* field) {
  const char* format = view->format;
  if (format == NULL || view->ndim != 1 ||
      (view->strides != NULL && view->itemsize != view->strides[0])) {
    return false;
  }

  const char fmt = format[0];
  // Always allow objects.
  if (fmt == 'O') {
    return true;
  }

  switch (upb_FieldDef_CType(field)) {
    case kUpb_CType_Int32:
    case kUpb_CType_Enum:
      return view->itemsize == 4 && (fmt == 'i' || fmt == 'l');
    case kUpb_CType_Int64:
      return view->itemsize == 8 && (fmt == 'q' || fmt == 'l');
    case kUpb_CType_UInt32:
      return view->itemsize == 4 && (fmt == 'I');
    case kUpb_CType_UInt64:
      return view->itemsize == 8 && (fmt == 'Q');
    case kUpb_CType_Float:
      return view->itemsize == 4 && (fmt == 'f');
    case kUpb_CType_Double:
      return view->itemsize == 8 && (fmt == 'd');
    case kUpb_CType_Bool:
      return view->itemsize == 1 && (fmt == '?' || fmt == 'B');
    default:
      return false;
  }
}
#endif

// When `size_cb_accurate` is true, it is invoked exactly once with
// the accurate size. Otherwise, it is invoked in a best-effort manner where the
// size may be smaller/larger than the actual size of the input or not called at
// all.
typedef bool (*PyUpb_SizeCb)(Py_ssize_t size, void* ctx);

// Callback invoked once per converted element during iteration.
// Returns false to stop iteration (caller should set a Python error first).
typedef bool (*PyUpb_ElemCb)(upb_MessageValue val, void* ctx);

// Callback invoked once with a contiguous raw buffer of matching type.
// `data` points to `count` elements each of size `itemsize`.
// Returns false on error.
typedef bool (*PyUpb_BulkCb)(const void* data, Py_ssize_t count,
                             Py_ssize_t itemsize, void* ctx);

// Iterates the elements of a Python iterable `value`, converting each to
// upb_MessageValue via PyUpb_PyToUpb, and invoking `elem_cb` per element.
//
// When PyUpb_SUPPORT_BUFFER_VIEW is enabled and if `bulk_cb` is non-NULL and
// `value` exposes a contiguous buffer whose element type matches `field`,
// `bulk_cb` is called once with the raw data pointer. Otherwise size_cb is
// called once with the number of elements in `value`, and then `elem_cb` is
// called for each element.
//
// Returns whether a Python error occurred.
static bool PyUpb_IterInput(PyObject* value, const upb_FieldDef* field,
                            upb_Arena* arena, PyUpb_SizeCb size_cb,
                            PyUpb_ElemCb elem_cb, PyUpb_BulkCb bulk_cb,
                            void* ctx) {
#if PyUpb_SUPPORT_BUFFER_VIEW
  Py_buffer view;
  if (PyObject_GetBuffer(value, &view, PyBUF_RECORDS_RO) == 0) {
    if (IsContiguous1DAndMatchesFieldType(&view, field)) {
      Py_ssize_t count = view.len / view.itemsize;
      if (count == 0) {
        PyBuffer_Release(&view);
        return bulk_cb(NULL, 0, 0, ctx);
      }
      if (view.format[0] != 'O') {
        if (upb_FieldDef_CType(field) == kUpb_CType_Enum) {
          const upb_EnumDef* e = upb_FieldDef_EnumSubDef(field);
          if (upb_EnumDef_IsClosed(e)) {
            const int32_t* i32 = (const int32_t*)view.buf;
            for (Py_ssize_t i = 0; i < count; i++) {
              if (!upb_EnumDef_CheckNumber(e, i32[i])) {
                PyErr_Format(PyExc_ValueError, "invalid enumerator %d",
                             (int)i32[i]);
                PyBuffer_Release(&view);
                return false;
              }
            }
          }
        }
        bool ok = bulk_cb(view.buf, count, view.itemsize, ctx);
        PyBuffer_Release(&view);
        return ok;
      }
      PyObject** objs = (PyObject**)view.buf;
      if (!size_cb(count, ctx)) {
        PyBuffer_Release(&view);
        return false;
      }
      for (Py_ssize_t i = 0; i < count; i++) {
        PyObject* item = objs[i] ? objs[i] : Py_None;
        upb_MessageValue msgval;
        if (!PyUpb_PyToUpb(item, field, &msgval, arena)) {
          PyBuffer_Release(&view);
          return false;
        }
        if (!elem_cb(msgval, ctx)) {
          PyBuffer_Release(&view);
          return false;
        }
      }
      PyBuffer_Release(&view);
      return true;
    }
    PyBuffer_Release(&view);
  } else {
    PyErr_Clear();
  }
#else
  (void)bulk_cb;
#endif
  PyObject* iter = NULL;
  PyObject* materialized = PySequence_Fast(value, "must assign iterable");
  if (!materialized) goto done;
  Py_ssize_t size = PyObject_Length(materialized);
  if (!size_cb(size, ctx)) goto done;

  iter = PyObject_GetIter(materialized);
  if (!iter) goto done;

  PyObject* item;
  while ((item = PyIter_Next(iter)) != NULL) {
    upb_MessageValue msgval;
    bool ok = PyUpb_PyToUpb(item, field, &msgval, arena);
    Py_DECREF(item);
    if (!ok) {
      goto done;
    }
    if (!elem_cb(msgval, ctx)) {
      goto done;
    }
  }
done:
  Py_XDECREF(materialized);
  Py_XDECREF(iter);
  return !PyErr_Occurred();
}

typedef struct {
  upb_Array* arr;
  upb_Arena* arena;
  Py_ssize_t size_hint;
  PyUpb_RepeatedContainer* self;
  int version;
} PyUpb_ExtendCtx;

static bool PyUpb_ExtendSizeCb(Py_ssize_t size, void* vctx) {
  PyUpb_ExtendCtx* ctx = (PyUpb_ExtendCtx*)vctx;
  if (!PyUpb_RepeatedContainer_AssureVersionUnchanged(ctx->self, ctx->version,
                                                      "extend")) {
    return false;
  }
  ctx->size_hint = size;
  size_t old_size = upb_Array_Size(ctx->arr);
  if (size > 0 && ((size_t)size <= SIZE_MAX - old_size)) {
    upb_Array_Reserve(ctx->arr, old_size + size, ctx->arena);
  }
  return true;
}

static bool PyUpb_ExtendElemCb(upb_MessageValue val, void* vctx) {
  PyUpb_ExtendCtx* ctx = (PyUpb_ExtendCtx*)vctx;
  if (!PyUpb_RepeatedContainer_AssureVersionUnchanged(ctx->self, ctx->version,
                                                      "extend")) {
    return false;
  }
  return upb_Array_Append(ctx->arr, val, ctx->arena);
}

typedef enum {
  kDisjoint,
  kSubset,
  kOverlap,
} PyUpb_OverlapType;

static PyUpb_OverlapType PyUpb_ArrayOverlaps(const upb_Array* arr,
                                             const void* range_start,
                                             size_t range_size,
                                             size_t itemsize) {
  const char* src_start = upb_Array_DataPtr(arr);
  const char* src_end = src_start + upb_Array_Size(arr) * itemsize;
  const char* dst_start = (const char*)range_start;
  const char* dst_end = dst_start + range_size * itemsize;
  if (src_end <= dst_start || src_start >= dst_end) {
    return kDisjoint;
  }
  if (src_start >= dst_start && src_end <= dst_end) {
    return kSubset;
  }
  return kOverlap;
}

static bool PyUpb_ExtendBulkCb(const void* data, Py_ssize_t count,
                               Py_ssize_t itemsize, void* vctx) {
  PyUpb_ExtendCtx* ctx = (PyUpb_ExtendCtx*)vctx;
  if (!PyUpb_RepeatedContainer_AssureVersionUnchanged(ctx->self, ctx->version,
                                                      "extend")) {
    return false;
  }
  size_t old_size = upb_Array_Size(ctx->arr);
  char* dst;
  switch (PyUpb_ArrayOverlaps(ctx->arr, data, count, itemsize)) {
    case kDisjoint:
      upb_Array_Resize(ctx->arr, old_size + count, ctx->arena);
      dst = upb_Array_MutableDataPtr(ctx->arr);
      break;
    case kSubset: {
      char* old_dst = upb_Array_MutableDataPtr(ctx->arr);
      upb_Array_Resize(ctx->arr, old_size + count, ctx->arena);
      dst = upb_Array_MutableDataPtr(ctx->arr);
      if (old_dst != dst) {
        data = dst + ((const char*)data - old_dst);
      }
    } break;
    case kOverlap:
      PyErr_SetString(PyExc_ValueError,
                      "Extending with overlapping sequence not supported");
      return false;
  }
  memcpy(dst + old_size * itemsize, data, count * itemsize);
  return true;
}

PyObject* PyUpb_RepeatedContainer_Extend(PyObject* _self, PyObject* value) {
  PyUpb_RepeatedContainer* self = (PyUpb_RepeatedContainer*)_self;
  const upb_FieldDef* f = PyUpb_RepeatedContainer_GetField(self);
  upb_Array* arr = PyUpb_RepeatedContainer_AssureWritable(_self);
  if (!arr) return NULL;
  upb_Arena* arena = PyUpb_Arena_Get(self->arena);
  int version = self->version;

  if (upb_FieldDef_IsSubMessage(f)) {
    // Composite fields: iterate and append via MergeFrom.
    // Buffer views don't apply to sub-messages.
    PyObject* iter = PyObject_GetIter(value);
    if (!iter) return NULL;
    PyObject* item;
    while ((item = PyIter_Next(iter)) != NULL) {
      if (!PyUpb_RepeatedContainer_AssureVersionUnchanged(self, version,
                                                          "extend")) {
        Py_DECREF(item);
        Py_DECREF(iter);
        return NULL;
      }
      PyObject* ret = PyUpb_RepeatedCompositeContainer_Append(_self, item);
      Py_DECREF(item);
      if (!ret) {
        Py_DECREF(iter);
        return NULL;
      }
      Py_DECREF(ret);
      version = self->version;
    }
    Py_DECREF(iter);
    if (PyErr_Occurred()) return NULL;
    if (!PyUpb_RepeatedContainer_AssureVersionUnchanged(self, version,
                                                        "extend")) {
      return NULL;
    }
    Py_RETURN_NONE;
  }
  size_t old_size = upb_Array_Size(arr);
  PyUpb_ExtendCtx ctx = {arr, arena, 0, self, self->version};
  if (!PyUpb_IterInput(value, f, arena, PyUpb_ExtendSizeCb, PyUpb_ExtendElemCb,
                       PyUpb_ExtendBulkCb, &ctx)) {
    if (self->version == ctx.version) {
      upb_Array_Resize(arr, old_size, NULL);
    }
    return NULL;
  }
  Py_RETURN_NONE;
}

static PyObject* PyUpb_RepeatedContainer_Item(PyObject* _self,
                                              Py_ssize_t index) {
  PyUpb_RepeatedContainer* self = (PyUpb_RepeatedContainer*)_self;
  upb_Array* arr = PyUpb_RepeatedContainer_GetIfReified(self);
  Py_ssize_t size = arr ? upb_Array_Size(arr) : 0;
  if (index < 0 || index >= size) {
    PyErr_Format(PyExc_IndexError, "list index (%zd) out of range", index);
    return NULL;
  }
  const upb_FieldDef* f = PyUpb_RepeatedContainer_GetField(self);
  return PyUpb_UpbToPy(upb_Array_Get(arr, index), f, self->arena);
}

PyObject* PyUpb_RepeatedContainer_ToList(PyObject* _self) {
  PyUpb_RepeatedContainer* self = (PyUpb_RepeatedContainer*)_self;
  upb_Array* arr = PyUpb_RepeatedContainer_GetIfReified(self);
  if (!arr) return PyList_New(0);

  const upb_FieldDef* f = PyUpb_RepeatedContainer_GetField(self);
  size_t n = upb_Array_Size(arr);
  PyObject* list = PyList_New(n);
  for (size_t i = 0; i < n; i++) {
    PyObject* val = PyUpb_UpbToPy(upb_Array_Get(arr, i), f, self->arena);
    if (!val) {
      Py_DECREF(list);
      return NULL;
    }
    PyList_SetItem(list, i, val);
  }
  return list;
}

static PyObject* PyUpb_RepeatedContainer_Repr(PyObject* _self) {
  PyObject* list = PyUpb_RepeatedContainer_ToList(_self);
  if (!list) return NULL;
  assert(!PyErr_Occurred());
  PyObject* repr = PyObject_Repr(list);
  Py_DECREF(list);
  return repr;
}

static PyObject* PyUpb_RepeatedContainer_RichCompare(PyObject* _self,
                                                     PyObject* _other,
                                                     int opid) {
  if (opid != Py_EQ && opid != Py_NE) {
    Py_INCREF(Py_NotImplemented);
    return Py_NotImplemented;
  }
  PyObject* list1 = PyUpb_RepeatedContainer_ToList(_self);
  PyObject* list2 = _other;
  PyObject* del = NULL;
  if (PyObject_TypeCheck(_other, _self->ob_type)) {
    del = list2 = PyUpb_RepeatedContainer_ToList(_other);
  }
  PyObject* ret = PyObject_RichCompare(list1, list2, opid);
  Py_DECREF(list1);
  Py_XDECREF(del);
  return ret;
}

static PyObject* PyUpb_RepeatedContainer_Subscript(PyObject* _self,
                                                   PyObject* key) {
  PyUpb_RepeatedContainer* self = (PyUpb_RepeatedContainer*)_self;
  upb_Array* arr = PyUpb_RepeatedContainer_GetIfReified(self);
  Py_ssize_t size = arr ? upb_Array_Size(arr) : 0;
  Py_ssize_t idx, count, step;
  if (!PyUpb_IndexToRange(key, size, &idx, &count, &step)) return NULL;
  const upb_FieldDef* f = PyUpb_RepeatedContainer_GetField(self);
  if (step == 0) {
    return PyUpb_UpbToPy(upb_Array_Get(arr, idx), f, self->arena);
  } else {
    PyObject* list = PyList_New(count);
    for (Py_ssize_t i = 0; i < count; i++, idx += step) {
      upb_MessageValue msgval = upb_Array_Get(self->ptr.arr, idx);
      PyObject* item = PyUpb_UpbToPy(msgval, f, self->arena);
      if (!item) {
        Py_DECREF(list);
        return NULL;
      }
      PyList_SetItem(list, i, item);
    }
    return list;
  }
}

typedef struct {
  upb_Array* arr;
  upb_Arena* arena;
  Py_ssize_t index;
  Py_ssize_t step;
  Py_ssize_t count;
  PyUpb_RepeatedContainer* self;
  int version;
} PyUpb_SetSubscriptCtx;

static bool PyUpb_SetSubscriptSizeCb(Py_ssize_t seq_size, void* vctx) {
  PyUpb_SetSubscriptCtx* ctx = (PyUpb_SetSubscriptCtx*)vctx;
  if (!PyUpb_RepeatedContainer_AssureVersionUnchanged(ctx->self, ctx->version,
                                                      "assignment")) {
    return false;
  }
  if (seq_size != ctx->count) {
    if (ctx->step == 1) {
      // We must shift the tail elements (either right or left).
      size_t tail = upb_Array_Size(ctx->arr) - (ctx->index + ctx->count);
      upb_Array_Resize(ctx->arr, ctx->index + seq_size + tail, ctx->arena);
      upb_Array_Move(ctx->arr, ctx->index + seq_size, ctx->index + ctx->count,
                     tail);
      ctx->count = seq_size;
    } else {
      PyErr_Format(PyExc_ValueError,
                   "attempt to assign sequence of  %zd to extended slice "
                   "of size %zd",
                   seq_size, ctx->count);
      return false;
    }
  }
  return true;
}

static bool PyUpb_SetSubscriptElemCb(upb_MessageValue val, void* vctx) {
  PyUpb_SetSubscriptCtx* ctx = (PyUpb_SetSubscriptCtx*)vctx;
  if (!PyUpb_RepeatedContainer_AssureVersionUnchanged(ctx->self, ctx->version,
                                                      "assignment")) {
    return false;
  }
  upb_Array_Set(ctx->arr, ctx->index, val);
  ctx->index += ctx->step;
  return true;
}

static void PyUpb_Reverse(char* start, char* end, size_t itemsize) {
  char tmp[16];
  assert(itemsize <= sizeof(tmp));
  while (start < end) {
    memcpy(tmp, start, itemsize);
    memcpy(start, end, itemsize);
    memcpy(end, tmp, itemsize);
    start += itemsize;
    end -= itemsize;
  }
}

static void PyUpb_Rotate(char* first, char* middle, char* last,
                         size_t itemsize) {
  if (first == middle || middle == last) return;
  PyUpb_Reverse(first, middle - itemsize, itemsize);
  PyUpb_Reverse(middle, last - itemsize, itemsize);
  PyUpb_Reverse(first, last - itemsize, itemsize);
}

static bool PyUpb_SetSubscriptBulkCb(const void* data, Py_ssize_t count,
                                     Py_ssize_t itemsize, void* vctx) {
  PyUpb_SetSubscriptCtx* ctx = (PyUpb_SetSubscriptCtx*)vctx;
  if (!PyUpb_RepeatedContainer_AssureVersionUnchanged(ctx->self, ctx->version,
                                                      "assignment")) {
    return false;
  }
  PyUpb_OverlapType overlap =
      PyUpb_ArrayOverlaps(ctx->arr, data, count, itemsize);

  if (overlap == kOverlap) {
    PyErr_SetString(PyExc_ValueError,
                    "Assigning with overlapping sequence not supported");
    return false;
  }

  size_t old_size = upb_Array_Size(ctx->arr);
  size_t tail = old_size - (ctx->index + ctx->count);
  char* dst = upb_Array_MutableDataPtr(ctx->arr);
  char* target = dst + ctx->index * itemsize;

  if (overlap == kDisjoint) {
    if (count != ctx->count) {
      if (ctx->step != 1) {
        PyErr_Format(
            PyExc_ValueError,
            "attempt to assign sequence of %zd to extended slice of size %zd",
            count, ctx->count);
        return false;
      }
      upb_Array_Resize(ctx->arr, ctx->index + count + tail, ctx->arena);
      dst = upb_Array_MutableDataPtr(ctx->arr);
      upb_Array_Move(ctx->arr, ctx->index + count, ctx->index + ctx->count,
                     tail);
    }
    if (ctx->step == 1) {
      memcpy(dst + ctx->index * itemsize, data, count * itemsize);
    } else {
      const char* src = (const char*)data;
      for (Py_ssize_t i = 0; i < count; i++) {
        memcpy(dst + (ctx->index + i * ctx->step) * itemsize,
               src + i * itemsize, itemsize);
      }
    }
    return true;
  }

  if (count != ctx->count && ctx->step != 1) {
    PyErr_Format(
        PyExc_ValueError,
        "attempt to assign sequence of %zd to extended slice of size %zd",
        count, ctx->count);
    return false;
  }

  // Fast paths for subset assignment when not growing the array
  // (count <= ctx->count).
  if (count <= ctx->count) {
    if (data != target) {
      memmove(target, data, count * itemsize);
    }
    if (count < ctx->count) {
      upb_Array_Move(ctx->arr, ctx->index + count, ctx->index + ctx->count,
                     tail);
      upb_Array_Resize(ctx->arr, ctx->index + count + tail, ctx->arena);
    }
    return true;
  }

  // General case (growing subset assignment: append + rotate + memmove +
  // truncate).

  // Append to the end of the array.
  char* old_dst = upb_Array_MutableDataPtr(ctx->arr);
  upb_Array_Resize(ctx->arr, old_size + count, ctx->arena);
  dst = upb_Array_MutableDataPtr(ctx->arr);
  if (old_dst != dst) {
    data = dst + ((const char*)data - old_dst);
  }
  memcpy(dst + old_size * itemsize, data, count * itemsize);

  //
  if (ctx->step == 1) {
    char* first = dst + (ctx->index + ctx->count) * itemsize;
    char* middle = dst + old_size * itemsize;
    char* last = dst + (old_size + count) * itemsize;

    PyUpb_Rotate(first, middle, last, itemsize);

    if (ctx->count > 0) {
      memmove(dst + ctx->index * itemsize,
              dst + (ctx->index + ctx->count) * itemsize,
              (count + tail) * itemsize);
    }
  } else {
    const char* src = dst + old_size * itemsize;
    for (Py_ssize_t i = 0; i < count; i++) {
      memcpy(dst + (ctx->index + i * ctx->step) * itemsize, src + i * itemsize,
             itemsize);
    }
  }

  upb_Array_Resize(ctx->arr, ctx->index + count + tail, ctx->arena);
  return true;
}

static int PyUpb_RepeatedContainer_SetSubscript(
    PyUpb_RepeatedContainer* self, upb_Array* arr, const upb_FieldDef* f,
    Py_ssize_t idx, Py_ssize_t count, Py_ssize_t step, PyObject* value) {
  upb_Arena* arena = PyUpb_Arena_Get(self->arena);
  if (upb_FieldDef_IsSubMessage(f)) {
    PyErr_SetString(PyExc_TypeError, "does not support assignment");
    return -1;
  }
  int version = self->version;

  if (step == 0) {
    // Set single value.
    upb_MessageValue msgval;
    if (!PyUpb_PyToUpb(value, f, &msgval, arena)) return -1;
    if (!PyUpb_RepeatedContainer_AssureVersionUnchanged(self, version,
                                                        "assignment")) {
      return -1;
    }
    upb_Array_Set(arr, idx, msgval);
    return 0;
  }
  Py_ssize_t old_size = (Py_ssize_t)upb_Array_Size(arr);
  Py_ssize_t tail = old_size - (idx + count);
  if (step == 1 && count == 0 && tail == 0) {
    PyObject* ret = PyUpb_RepeatedContainer_Extend((PyObject*)self, value);
    if (!ret) return -1;
    Py_DECREF(ret);
    return 0;
  }
  PyUpb_SetSubscriptCtx ctx = {arr, arena, idx, step, count, self, version};
  if (!PyUpb_IterInput(value, f, arena, PyUpb_SetSubscriptSizeCb,
                       PyUpb_SetSubscriptElemCb, PyUpb_SetSubscriptBulkCb,
                       &ctx)) {
    return -1;
  }
  return 0;
}

static int PyUpb_RepeatedContainer_DeleteSubscript(upb_Array* arr,
                                                   Py_ssize_t idx,
                                                   Py_ssize_t count,
                                                   Py_ssize_t step) {
  // Normalize direction: deletion is order-independent.
  Py_ssize_t start = idx;
  if (step < 0) {
    Py_ssize_t end = start + step * (count - 1);
    start = end;
    step = -step;
  }

  size_t dst = start;
  size_t src;
  if (step > 1) {
    // Move elements between steps:
    //
    //        src
    //         |
    // |------X---X---X---X------------------------------|
    //        |
    //       dst           <-------- tail -------------->
    src = start + 1;
    for (Py_ssize_t i = 1; i < count; i++, dst += step - 1, src += step) {
      upb_Array_Move(arr, dst, src, step);
    }
  } else {
    src = start + count;
  }

  // Move tail.
  size_t tail = upb_Array_Size(arr) - src;
  size_t new_size = dst + tail;
  assert(new_size == upb_Array_Size(arr) - count);
  upb_Array_Move(arr, dst, src, tail);
  upb_Array_Resize(arr, new_size, NULL);
  return 0;
}

static int PyUpb_RepeatedContainer_AssignSubscript(PyObject* _self,
                                                   PyObject* key,
                                                   PyObject* value) {
  PyUpb_RepeatedContainer* self = (PyUpb_RepeatedContainer*)_self;
  const upb_FieldDef* f = PyUpb_RepeatedContainer_GetField(self);
  upb_Array* arr = PyUpb_RepeatedContainer_AssureWritable(_self);
  if (!arr) return -1;
  Py_ssize_t size = arr ? upb_Array_Size(arr) : 0;
  Py_ssize_t idx, count, step;
  if (!PyUpb_IndexToRange(key, size, &idx, &count, &step)) return -1;
  if (value) {
    return PyUpb_RepeatedContainer_SetSubscript(self, arr, f, idx, count, step,
                                                value);
  } else {
    return PyUpb_RepeatedContainer_DeleteSubscript(arr, idx, count, step);
  }
}

static PyObject* PyUpb_RepeatedContainer_Pop(PyObject* _self, PyObject* args) {
  PyUpb_RepeatedContainer* self = (PyUpb_RepeatedContainer*)_self;
  Py_ssize_t index = -1;
  if (!PyArg_ParseTuple(args, "|n", &index)) return NULL;
  upb_Array* arr = PyUpb_RepeatedContainer_AssureWritable(_self);
  if (!arr) return NULL;
  int version = self->version;
  size_t size = upb_Array_Size(arr);
  if (index < 0) index += size;
#if PROTOBUF_PY_FUTURE_REMOVE_POP_CLAMP
#else
  if (index >= size) {
    PyErr_WarnEx(PyExc_FutureWarning, "pop index out of range", 1);
    index = size - 1;
  }
#endif  // PROTOBUF_PY_FUTURE_REMOVE_POP_CLAMP
  PyObject* ret = PyUpb_RepeatedContainer_Item(_self, index);
  if (!ret) return NULL;
  if (!PyUpb_RepeatedContainer_AssureVersionUnchanged(self, version, "pop")) {
    Py_DECREF(ret);
    return NULL;
  }
  upb_Array_Delete(self->ptr.arr, index, 1);
  return ret;
}

static PyObject* PyUpb_RepeatedContainer_Remove(PyObject* _self,
                                                PyObject* value) {
  upb_Array* arr = PyUpb_RepeatedContainer_AssureWritable(_self);
  if (!arr) return NULL;
  PyUpb_RepeatedContainer* self = (PyUpb_RepeatedContainer*)_self;
  int version = self->version;
  Py_ssize_t match_index = -1;
  Py_ssize_t n = PyUpb_RepeatedContainer_Length(_self);
  for (Py_ssize_t i = 0; i < n; ++i) {
    PyObject* elem = PyUpb_RepeatedContainer_Item(_self, i);
    if (!elem) return NULL;
    int eq = PyObject_RichCompareBool(elem, value, Py_EQ);
    Py_DECREF(elem);
    if (eq) {
      match_index = i;
      break;
    }
    if (!PyUpb_RepeatedContainer_AssureVersionUnchanged(self, version,
                                                        "remove")) {
      return NULL;
    }
  }
  if (!PyUpb_RepeatedContainer_AssureVersionUnchanged(self, version,
                                                      "remove")) {
    return NULL;
  }
  if (match_index == -1) {
    PyErr_SetString(PyExc_ValueError, "remove(x): x not in container");
    return NULL;
  }
  if (PyUpb_RepeatedContainer_DeleteSubscript(arr, match_index, 1, 1) < 0) {
    return NULL;
  }
  Py_RETURN_NONE;
}

// A helper function used only for Sort().
static bool PyUpb_RepeatedContainer_Assign(PyObject* _self, PyObject* list) {
  PyUpb_RepeatedContainer* self = (PyUpb_RepeatedContainer*)_self;
  const upb_FieldDef* f = PyUpb_RepeatedContainer_GetField(self);
  upb_Array* arr = PyUpb_RepeatedContainer_AssureWritable(_self);
  int version = self->version;
  Py_ssize_t size = PyList_Size(list);
  bool submsg = upb_FieldDef_IsSubMessage(f);
  upb_Arena* arena = PyUpb_Arena_Get(self->arena);
  for (Py_ssize_t i = 0; i < size; ++i) {
    PyObject* obj = PyList_GetItem(list, i);
    upb_MessageValue msgval;
    if (submsg) {
      msgval.msg_val = PyUpb_Message_GetIfReified(obj);
      assert(msgval.msg_val);
    } else {
      if (!PyUpb_PyToUpb(obj, f, &msgval, arena)) return false;
      if (!PyUpb_RepeatedContainer_AssureVersionUnchanged(self, version,
                                                          "sort")) {
        return false;
      }
    }
    upb_Array_Set(arr, i, msgval);
  }
  return true;
}

static PyObject* PyUpb_RepeatedContainer_Sort(PyObject* pself, PyObject* args,
                                              PyObject* kwds) {
  // Support the old sort_function argument for backwards
  // compatibility.
  if (kwds != NULL) {
    PyObject* sort_func = PyDict_GetItemString(kwds, "sort_function");
    if (sort_func != NULL) {
      // Must set before deleting as sort_func is a borrowed reference
      // and kwds might be the only thing keeping it alive.
      if (PyDict_SetItemString(kwds, "cmp", sort_func) == -1) return NULL;
      if (PyDict_DelItemString(kwds, "sort_function") == -1) return NULL;
    }
  }

  if (PyUpb_RepeatedContainer_IsFrozen((PyUpb_RepeatedContainer*)pself)) {
    return PyUpb_SetFrozenErrorWithMsg("Container is immutable");
  }

  // TODO:b/517235198 - Reify even for empty sequences.
  if (PyUpb_RepeatedContainer_Length(pself) == 0) Py_RETURN_NONE;

  upb_Array* arr = PyUpb_RepeatedContainer_AssureWritable(pself);
  if (!arr) return NULL;
  PyUpb_RepeatedContainer* self = (PyUpb_RepeatedContainer*)pself;
  int version = self->version;

  PyObject* ret = NULL;
  PyObject* full_slice = NULL;
  PyObject* list = NULL;
  PyObject* m = NULL;
  PyObject* res = NULL;

  if ((full_slice = PySlice_New(NULL, NULL, NULL)) &&
      (list = PyUpb_RepeatedContainer_Subscript(pself, full_slice)) &&
      (m = PyObject_GetAttrString(list, "sort")) &&
      (res = PyObject_Call(m, args, kwds))) {
    if (PyUpb_RepeatedContainer_AssureVersionUnchanged(self, version, "sort") &&
        PyUpb_RepeatedContainer_Assign(pself, list)) {
      Py_INCREF(Py_None);
      ret = Py_None;
    }
  }

  Py_XDECREF(full_slice);
  Py_XDECREF(list);
  Py_XDECREF(m);
  Py_XDECREF(res);
  return ret;
}

static PyObject* PyUpb_RepeatedContainer_Reverse(PyObject* _self) {
  upb_Array* arr = PyUpb_RepeatedContainer_AssureWritable(_self);
  if (!arr) return NULL;
  size_t n = upb_Array_Size(arr);
  size_t half = n / 2;  // Rounds down.
  for (size_t i = 0; i < half; i++) {
    size_t i2 = n - i - 1;
    upb_MessageValue val1 = upb_Array_Get(arr, i);
    upb_MessageValue val2 = upb_Array_Get(arr, i2);
    upb_Array_Set(arr, i, val2);
    upb_Array_Set(arr, i2, val1);
  }
  Py_RETURN_NONE;
}

static PyObject* PyUpb_RepeatedContainer_Clear(PyObject* _self) {
  Py_ssize_t size = PyUpb_RepeatedContainer_Length(_self);
  // TODO: b/517235198 - Reify even for empty sequences.
  if (size == 0) Py_RETURN_NONE;

  PyUpb_RepeatedContainer* self = (PyUpb_RepeatedContainer*)_self;
  upb_Array* arr = PyUpb_RepeatedContainer_AssureWritable(_self);
  if (!arr) return NULL;
  upb_Array_Delete(self->ptr.arr, 0, size);
  Py_RETURN_NONE;
}

static PyObject* PyUpb_RepeatedContainer_MergeFrom(PyObject* _self,
                                                   PyObject* args) {
  return PyUpb_RepeatedContainer_Extend(_self, args);
}

// -----------------------------------------------------------------------------
// RepeatedCompositeContainer
// -----------------------------------------------------------------------------

static PyObject* PyUpb_RepeatedCompositeContainer_AppendNew(PyObject* _self) {
  PyUpb_RepeatedContainer* self = (PyUpb_RepeatedContainer*)_self;
  upb_Array* arr = PyUpb_RepeatedContainer_AssureWritable(_self);
  if (!arr) return NULL;
  const upb_FieldDef* f = PyUpb_RepeatedContainer_GetField(self);
  upb_Arena* arena = PyUpb_Arena_Get(self->arena);
  const upb_MessageDef* m = upb_FieldDef_MessageSubDef(f);
  const upb_MiniTable* layout = upb_MessageDef_MiniTable(m);
  upb_Message* msg = upb_Message_New(layout, arena);
  upb_MessageValue msgval = {.msg_val = msg};
  upb_Array_Append(arr, msgval, arena);
  return PyUpb_Message_Get(msg, m, self->arena);
}

PyObject* PyUpb_RepeatedCompositeContainer_Add(PyObject* _self, PyObject* args,
                                               PyObject* kwargs) {
  PyUpb_RepeatedContainer* self = (PyUpb_RepeatedContainer*)_self;
  int version = self->version;
  PyObject* py_msg = PyUpb_RepeatedCompositeContainer_AppendNew(_self);
  if (!py_msg) return NULL;
  if (PyUpb_Message_InitAttributes(py_msg, args, kwargs) < 0) {
    Py_DECREF(py_msg);
    if (self->version == version + 1 && upb_Array_Size(self->ptr.arr) > 0) {
      upb_Array_Delete(self->ptr.arr, upb_Array_Size(self->ptr.arr) - 1, 1);
    }
    return NULL;
  }
  if (!PyUpb_RepeatedContainer_AssureVersionUnchanged(self, version + 1,
                                                      "add")) {
    Py_DECREF(py_msg);
    return NULL;
  }
  return py_msg;
}

static PyObject* PyUpb_RepeatedCompositeContainer_Append(PyObject* _self,
                                                         PyObject* value) {
  if (!PyUpb_Message_Verify(value)) return NULL;
  PyUpb_RepeatedContainer* self = (PyUpb_RepeatedContainer*)_self;
  int version = self->version;
  PyObject* py_msg = PyUpb_RepeatedCompositeContainer_AppendNew(_self);
  if (!py_msg) return NULL;
  PyObject* none = PyUpb_Message_MergeFrom(py_msg, value);
  if (!none) {
    Py_DECREF(py_msg);
    if (self->version == version + 1 && upb_Array_Size(self->ptr.arr) > 0) {
      upb_Array_Delete(self->ptr.arr, upb_Array_Size(self->ptr.arr) - 1, 1);
    }
    return NULL;
  }
  Py_DECREF(none);
  if (!PyUpb_RepeatedContainer_AssureVersionUnchanged(self, version + 1,
                                                      "append")) {
    Py_DECREF(py_msg);
    return NULL;
  }
  return py_msg;
}

static PyObject* PyUpb_RepeatedContainer_Insert(PyObject* _self,
                                                PyObject* args) {
  PyUpb_RepeatedContainer* self = (PyUpb_RepeatedContainer*)_self;
  Py_ssize_t index;
  PyObject* value;
  if (!PyArg_ParseTuple(args, "nO", &index, &value)) return NULL;
  upb_Array* arr = PyUpb_RepeatedContainer_AssureWritable(_self);
  if (!arr) return NULL;
  int version = self->version;

  // Normalize index.
  Py_ssize_t size = upb_Array_Size(arr);
  if (index < 0) index += size;
  if (index < 0) index = 0;
  if (index > size) index = size;

  const upb_FieldDef* f = PyUpb_RepeatedContainer_GetField(self);
  upb_MessageValue msgval;
  upb_Arena* arena = PyUpb_Arena_Get(self->arena);
  if (upb_FieldDef_IsSubMessage(f)) {
    // Create message.
    const upb_MessageDef* m = upb_FieldDef_MessageSubDef(f);
    const upb_MiniTable* layout = upb_MessageDef_MiniTable(m);
    upb_Message* msg = upb_Message_New(layout, arena);
    PyObject* py_msg = PyUpb_Message_Get(msg, m, self->arena);
    PyObject* ret = PyUpb_Message_MergeFrom(py_msg, value);
    Py_DECREF(py_msg);
    if (!ret) return NULL;
    Py_DECREF(ret);
    msgval.msg_val = msg;
  } else {
    if (!PyUpb_PyToUpb(value, f, &msgval, arena)) return NULL;
  }

  if (!PyUpb_RepeatedContainer_AssureVersionUnchanged(self, version,
                                                      "insert")) {
    return NULL;
  }

  upb_Array_Insert(arr, index, 1, arena);
  upb_Array_Set(arr, index, msgval);

  Py_RETURN_NONE;
}

static PyMethodDef PyUpb_RepeatedCompositeContainer_Methods[] = {
    {"__deepcopy__", PyUpb_RepeatedContainer_DeepCopy, METH_VARARGS,
     "Makes a deep copy of the class."},
    {"add", (PyCFunction)PyUpb_RepeatedCompositeContainer_Add,
     METH_VARARGS | METH_KEYWORDS, "Adds an object to the repeated container."},
    {"append", PyUpb_RepeatedCompositeContainer_Append, METH_O,
     "Appends a message to the end of the repeated container."},
    {"insert", PyUpb_RepeatedContainer_Insert, METH_VARARGS,
     "Inserts a message before the specified index."},
    {"extend", PyUpb_RepeatedContainer_Extend, METH_O,
     "Adds objects to the repeated container."},
    {"pop", PyUpb_RepeatedContainer_Pop, METH_VARARGS,
     "Removes an object from the repeated container and returns it."},
    {"remove", PyUpb_RepeatedContainer_Remove, METH_O,
     "Removes an object from the repeated container."},
    {"sort", (PyCFunction)PyUpb_RepeatedContainer_Sort,
     METH_VARARGS | METH_KEYWORDS, "Sorts the repeated container."},
    {"reverse", (PyCFunction)PyUpb_RepeatedContainer_Reverse, METH_NOARGS,
     "Reverses elements order of the repeated container."},
    {"clear", (PyCFunction)PyUpb_RepeatedContainer_Clear, METH_NOARGS,
     "Clears repeated container."},
    {"MergeFrom", PyUpb_RepeatedContainer_MergeFrom, METH_O,
     "Adds objects to the repeated container."},
    {NULL, NULL}};

static PyType_Slot PyUpb_RepeatedCompositeContainer_Slots[] = {
    {Py_tp_dealloc, PyUpb_RepeatedContainer_Dealloc},
    {Py_tp_methods, PyUpb_RepeatedCompositeContainer_Methods},
    {Py_sq_length, PyUpb_RepeatedContainer_Length},
    {Py_sq_item, PyUpb_RepeatedContainer_Item},
    {Py_mp_length, PyUpb_RepeatedContainer_Length},
    {Py_tp_repr, PyUpb_RepeatedContainer_Repr},
    {Py_mp_subscript, PyUpb_RepeatedContainer_Subscript},
    {Py_mp_ass_subscript, PyUpb_RepeatedContainer_AssignSubscript},
    {Py_tp_new, PyUpb_Forbidden_New},
    {Py_tp_richcompare, PyUpb_RepeatedContainer_RichCompare},
    {Py_tp_hash, PyObject_HashNotImplemented},
    {0, NULL}};

static PyType_Spec PyUpb_RepeatedCompositeContainer_Spec = {
    PYUPB_MODULE_NAME ".RepeatedCompositeContainer",
    sizeof(PyUpb_RepeatedContainer),
    0,  // tp_itemsize
    Py_TPFLAGS_DEFAULT,
    PyUpb_RepeatedCompositeContainer_Slots,
};

// -----------------------------------------------------------------------------
// RepeatedScalarContainer
// -----------------------------------------------------------------------------

static PyObject* PyUpb_RepeatedScalarContainer_Append(PyObject* _self,
                                                      PyObject* value) {
  PyUpb_RepeatedContainer* self = (PyUpb_RepeatedContainer*)_self;
  upb_Array* arr = PyUpb_RepeatedContainer_AssureWritable(_self);
  if (!arr) return NULL;
  int version = self->version;
  upb_Arena* arena = PyUpb_Arena_Get(self->arena);
  const upb_FieldDef* f = PyUpb_RepeatedContainer_GetField(self);
  upb_MessageValue msgval;
  if (!PyUpb_PyToUpb(value, f, &msgval, arena)) {
    return NULL;
  }
  if (!PyUpb_RepeatedContainer_AssureVersionUnchanged(self, version,
                                                      "append")) {
    return NULL;
  }
  upb_Array_Append(arr, msgval, arena);
  Py_RETURN_NONE;
}

static int PyUpb_RepeatedScalarContainer_AssignItem(PyObject* _self,
                                                    Py_ssize_t index,
                                                    PyObject* item) {
  PyUpb_RepeatedContainer* self = (PyUpb_RepeatedContainer*)_self;
  upb_Array* arr = PyUpb_RepeatedContainer_AssureWritable(_self);
  if (!arr) return -1;
  int version = self->version;
  Py_ssize_t size = upb_Array_Size(arr);
  if (index < 0 || index >= size) {
    PyErr_Format(PyExc_IndexError, "list index (%zd) out of range", index);
    return -1;
  }
  const upb_FieldDef* f = PyUpb_RepeatedContainer_GetField(self);
  upb_MessageValue msgval;
  upb_Arena* arena = PyUpb_Arena_Get(self->arena);
  if (!PyUpb_PyToUpb(item, f, &msgval, arena)) {
    return -1;
  }
  if (!PyUpb_RepeatedContainer_AssureVersionUnchanged(self, version,
                                                      "assignment")) {
    return -1;
  }
  upb_Array_Set(self->ptr.arr, index, msgval);
  return 0;
}

static PyObject* PyUpb_RepeatedScalarContainer_Reduce(PyObject* unused_self,
                                                      PyObject* unused_other) {
  PyObject* pickle_module = PyImport_ImportModule("pickle");
  if (!pickle_module) return NULL;
  PyObject* pickle_error = PyObject_GetAttrString(pickle_module, "PickleError");
  Py_DECREF(pickle_module);
  if (!pickle_error) return NULL;
  PyErr_Format(pickle_error,
               "can't pickle repeated message fields, convert to list first");
  Py_DECREF(pickle_error);
  return NULL;
}

char* GetDefaultDTypeStr(upb_CType cpp_type) {
  switch (cpp_type) {
    case kUpb_CType_Float:
      return "f4";
    case kUpb_CType_Int32:
      return "i4";
    case kUpb_CType_Int64:
      return "i8";
    case kUpb_CType_UInt32:
      return "u4";
    case kUpb_CType_UInt64:
      return "u8";
    case kUpb_CType_Double:
      return "f8";
    case kUpb_CType_Bool:
      return "?";  // Boolean is '?' per official docs.
    case kUpb_CType_Enum:
      return "i4";
    case kUpb_CType_String:
    case kUpb_CType_Bytes:
    case kUpb_CType_Message:
      return NULL;
  }
  return NULL;
}

PyObject* CreateArrayFromView(PyObject* _self, PyObject* np_module) {
  PyUpb_RepeatedContainer* self = (PyUpb_RepeatedContainer*)(_self);
  upb_Array* arr = PyUpb_RepeatedContainer_GetIfReified(self);
  size_t size = arr ? upb_Array_Size(arr) : 0;
  const upb_FieldDef* f = PyUpb_RepeatedContainer_GetField(self);
  Py_ssize_t out_buffer_size_bytes;
  const char* out_dtype = GetDefaultDTypeStr(upb_FieldDef_CType(f));
  switch (upb_FieldDef_CType(f)) {
    case kUpb_CType_Float: {
      out_buffer_size_bytes = sizeof(float) * size;
      break;
    }
    case kUpb_CType_Int32: {
      out_buffer_size_bytes = 4 * size;
      break;
    }
    case kUpb_CType_Int64: {
      out_buffer_size_bytes = 8 * size;
      break;
    }
    case kUpb_CType_UInt32: {
      out_buffer_size_bytes = 4 * size;
      break;
    }
    case kUpb_CType_UInt64: {
      out_buffer_size_bytes = 8 * size;
      break;
    }
    case kUpb_CType_Double: {
      out_buffer_size_bytes = sizeof(double) * size;
      break;
    }
    case kUpb_CType_Bool: {
      out_buffer_size_bytes = sizeof(bool) * size;
      break;
    }
    case kUpb_CType_Enum: {
      out_buffer_size_bytes = 4 * size;
      break;
    }
    case kUpb_CType_Message:
    case kUpb_CType_Bytes:
    case kUpb_CType_String: {
      PyErr_Format(PyExc_SystemError,
                   "Code should never reach here: cpp type "
                   "should not be message nor string in CreateArrayFromView.");
      return NULL;
    }
  }
  if (out_buffer_size_bytes == 0) {
    return PyObject_CallMethod(np_module, "empty", "is", 0, out_dtype);
  }
  const void* out_ptr = upb_Array_DataPtr(arr);
  PyObject* view = PyMemoryView_FromMemory(
      (char*)(const char*)out_ptr, out_buffer_size_bytes, 0x100 /*PyBUF_READ*/);
  PyObject* return_value =
      PyObject_CallMethod(np_module, "frombuffer", "Os", view, out_dtype);
  Py_DECREF(view);
  return return_value;
}

// CPython equivalent of nparray.astype(dtype_requested, copy=copy).
PyObject* NpArrayAsType(PyObject* nparray, PyObject* dtype_requested,
                        bool copy) {
  PyObject* astype_args = Py_BuildValue("(O)", dtype_requested);
  PyObject* keywords = PyDict_New();
  PyDict_SetItemString(keywords, "copy", copy ? Py_True : Py_False);
  PyObject* np_array_astype = PyObject_GetAttrString(nparray, "astype");

  PyObject* return_value =
      PyObject_Call(np_array_astype, astype_args, keywords);
  Py_DECREF(np_array_astype);
  Py_DECREF(keywords);
  Py_DECREF(astype_args);
  return return_value;
}

PyObject* ConstructArrayByIteration(PyObject* _self, PyObject* np_module) {
  PyUpb_RepeatedContainer* self = (PyUpb_RepeatedContainer*)_self;
  upb_Array* arr = PyUpb_RepeatedContainer_GetIfReified(self);
  Py_ssize_t array_length = PyUpb_RepeatedContainer_Length(_self);
  const upb_FieldDef* f = PyUpb_RepeatedContainer_GetField(self);
  PyObject* nparray =
      PyObject_CallMethod(np_module, "empty", "is", array_length, "O");
  PyObject* arena = self->arena;
  for (Py_ssize_t i = 0; i < array_length; ++i) {
    upb_MessageValue msgval = upb_Array_Get(arr, i);
    PyObject* item = PyUpb_UpbToPy(msgval, f, arena);
    PySequence_SetItem(nparray, i, item);
    Py_DECREF(item);
  }
  return nparray;
}

static PyObject* PyUpb_RepeatedScalarContainer_AsNpArray(PyObject* _self,
                                                         PyObject* args,
                                                         PyObject* kwargs) {
  static const char* kwlist[] = {"dtype", "copy", NULL};
  PyObject* dtype_requested = NULL;
  PyObject* copy = NULL;
  PyObject* return_value = NULL;
  PyObject* default_dtype = NULL;
  PyObject* nparray = NULL;
  if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|OO", (char**)kwlist,
                                   &dtype_requested, &copy)) {
    return NULL;
  }

  PyObject* np_module = PyImport_ImportModule("numpy");
  if (np_module == NULL) {
    PyErr_Format(PyExc_ImportError, "Unable to import numpy.");
    return NULL;
  }
  PyUpb_RepeatedContainer* self = (PyUpb_RepeatedContainer*)(_self);
  const upb_FieldDef* f = PyUpb_RepeatedContainer_GetField(self);
  upb_CType c_type = upb_FieldDef_CType(f);

  // string repeated
  if (c_type == kUpb_CType_String || c_type == kUpb_CType_Bytes) {
    nparray = ConstructArrayByIteration(_self, np_module);
    if (dtype_requested == NULL) {
      if (c_type == kUpb_CType_String) {
        dtype_requested = PyObject_CallMethod(np_module, "dtype", "s", "str");
      } else {
        dtype_requested = PyObject_CallMethod(np_module, "dtype", "s", "S");
      }
      default_dtype = dtype_requested;
    }
    return_value = NpArrayAsType(nparray, dtype_requested, false);
    goto ret;
  }

  // non-string scalar repeated
  if (dtype_requested == NULL) {
    char* returned_dtype = GetDefaultDTypeStr(c_type);
    if (returned_dtype == NULL) {
      PyErr_SetString(PyExc_SystemError,
                      "Implementation error: Unhandled case in AsNpArray.");
      return NULL;
    }
    if (PyUpb_RepeatedContainer_Length(_self) == 0) {
      return_value =
          PyObject_CallMethod(np_module, "empty", "is", 0, returned_dtype);
      goto ret;
    }

    dtype_requested =
        PyObject_CallMethod(np_module, "dtype", "s", returned_dtype);
    // For ref-counting.
    default_dtype = dtype_requested;
  }
  nparray = CreateArrayFromView(_self, np_module);
  if (nparray == NULL) {
    goto ret;
  }

  return_value = NpArrayAsType(nparray, dtype_requested, true);

ret:
  Py_XDECREF(default_dtype);
  Py_XDECREF(nparray);
  Py_DECREF(np_module);
  assert(!PyErr_Occurred());
  return return_value;
}

static PyMethodDef PyUpb_RepeatedScalarContainer_Methods[] = {
    {"__deepcopy__", PyUpb_RepeatedContainer_DeepCopy, METH_VARARGS,
     "Makes a deep copy of the class."},
    {"__array__", (PyCFunction)PyUpb_RepeatedScalarContainer_AsNpArray,
     METH_VARARGS | METH_KEYWORDS, "Returns a np.array."},
    {"__reduce__", PyUpb_RepeatedScalarContainer_Reduce, METH_NOARGS,
     "Outputs picklable representation of the repeated field."},
    {"append", PyUpb_RepeatedScalarContainer_Append, METH_O,
     "Appends an object to the repeated container."},
    {"extend", PyUpb_RepeatedContainer_Extend, METH_O,
     "Appends objects to the repeated container."},
    {"insert", PyUpb_RepeatedContainer_Insert, METH_VARARGS,
     "Inserts an object at the specified position in the container."},
    {"pop", PyUpb_RepeatedContainer_Pop, METH_VARARGS,
     "Removes an object from the repeated container and returns it."},
    {"remove", PyUpb_RepeatedContainer_Remove, METH_O,
     "Removes an object from the repeated container."},
    {"sort", (PyCFunction)PyUpb_RepeatedContainer_Sort,
     METH_VARARGS | METH_KEYWORDS, "Sorts the repeated container."},
    {"reverse", (PyCFunction)PyUpb_RepeatedContainer_Reverse, METH_NOARGS,
     "Reverses elements order of the repeated container."},
    {"clear", (PyCFunction)PyUpb_RepeatedContainer_Clear, METH_NOARGS,
     "Clears repeated container."},
    {"MergeFrom", PyUpb_RepeatedContainer_MergeFrom, METH_O,
     "Merges a repeated container into the current container."},
    {NULL, NULL}};

static PyType_Slot PyUpb_RepeatedScalarContainer_Slots[] = {
    {Py_tp_dealloc, PyUpb_RepeatedContainer_Dealloc},
    {Py_tp_methods, PyUpb_RepeatedScalarContainer_Methods},
    {Py_tp_new, PyUpb_Forbidden_New},
    {Py_tp_repr, PyUpb_RepeatedContainer_Repr},
    {Py_sq_length, PyUpb_RepeatedContainer_Length},
    {Py_sq_item, PyUpb_RepeatedContainer_Item},
    {Py_sq_ass_item, PyUpb_RepeatedScalarContainer_AssignItem},
    {Py_mp_length, PyUpb_RepeatedContainer_Length},
    {Py_mp_subscript, PyUpb_RepeatedContainer_Subscript},
    {Py_mp_ass_subscript, PyUpb_RepeatedContainer_AssignSubscript},
    {Py_tp_richcompare, PyUpb_RepeatedContainer_RichCompare},
    {Py_tp_hash, PyObject_HashNotImplemented},
    {0, NULL}};

static PyType_Spec PyUpb_RepeatedScalarContainer_Spec = {
    PYUPB_MODULE_NAME ".RepeatedScalarContainer",
    sizeof(PyUpb_RepeatedContainer),
    0,  // tp_itemsize
    Py_TPFLAGS_DEFAULT,
    PyUpb_RepeatedScalarContainer_Slots,
};

// -----------------------------------------------------------------------------
// Top Level
// -----------------------------------------------------------------------------

static bool PyUpb_Repeated_RegisterAsSequence(PyUpb_ModuleState* state) {
  PyObject* collections = NULL;
  PyObject* seq = NULL;
  PyObject* ret1 = NULL;
  PyObject* ret2 = NULL;
  PyTypeObject* type1 = state->repeated_scalar_container_type;
  PyTypeObject* type2 = state->repeated_composite_container_type;

  bool ok = (collections = PyImport_ImportModule("collections.abc")) &&
            (seq = PyObject_GetAttrString(collections, "MutableSequence")) &&
            (ret1 = PyObject_CallMethod(seq, "register", "O", type1)) &&
            (ret2 = PyObject_CallMethod(seq, "register", "O", type2));

  Py_XDECREF(collections);
  Py_XDECREF(seq);
  Py_XDECREF(ret1);
  Py_XDECREF(ret2);
  return ok;
}

bool PyUpb_Repeated_Init(PyObject* m) {
  PyUpb_ModuleState* state = PyUpb_ModuleState_GetFromModule(m);

  state->repeated_composite_container_type =
      PyUpb_AddClass(m, &PyUpb_RepeatedCompositeContainer_Spec);
  state->repeated_scalar_container_type =
      PyUpb_AddClass(m, &PyUpb_RepeatedScalarContainer_Spec);

  return state->repeated_composite_container_type &&
         state->repeated_scalar_container_type &&
         PyUpb_Repeated_RegisterAsSequence(state);
}

#include "upb/port/undef.inc"
