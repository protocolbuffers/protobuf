/*
 * Copyright (c) 2009-2021, Google LLC
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Google LLC nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL Google LLC BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "python/convert.h"

#include "python/message.h"
#include "python/protobuf.h"
#include "upb/reflection.h"
#include "upb/util/compare.h"

PyObject* PyUpb_UpbToPy(upb_msgval val, const upb_fielddef *f, PyObject *arena) {
  switch (upb_fielddef_type(f)) {
    case UPB_TYPE_ENUM:
    case UPB_TYPE_INT32:
      return PyLong_FromLong(val.int32_val);
    case UPB_TYPE_INT64:
      return PyLong_FromLongLong(val.int64_val);
    case UPB_TYPE_UINT32:
      return PyLong_FromSize_t(val.uint32_val);
    case UPB_TYPE_UINT64:
      return PyLong_FromUnsignedLongLong(val.uint64_val);
    case UPB_TYPE_FLOAT:
      return PyFloat_FromDouble(val.float_val);
    case UPB_TYPE_DOUBLE:
      return PyFloat_FromDouble(val.double_val);
    case UPB_TYPE_BOOL:
      return PyBool_FromLong(val.bool_val);
    case UPB_TYPE_BYTES:
      return PyBytes_FromStringAndSize(val.str_val.data, val.str_val.size);
    case UPB_TYPE_STRING:
      return PyUnicode_DecodeUTF8(val.str_val.data, val.str_val.size, NULL);
    case UPB_TYPE_MESSAGE:
      return PyUpb_CMessage_Get((upb_msg*)val.msg_val,
                                upb_fielddef_msgsubdef(f), arena);
    default:
      PyErr_Format(PyExc_SystemError,
                   "Getting a value from a field of unknown type %d",
                   upb_fielddef_type(f));
      return NULL;
  }
}

static bool PyUpb_GetInt64(PyObject *obj, int64_t *val) {
  // We require that the value is either an integer or has an __index__
  // conversion.
  if (!PyIndex_Check(obj)) {
    PyErr_Format(PyExc_TypeError, "Expected integer: %S", obj);
  }
  // If the value is already a Python long, PyLong_AsLongLong() retrieves it.
  // Otherwise is converts to integer using __int__.
  *val = PyLong_AsLongLong(obj);
  if (!PyErr_Occurred()) return true;
  if (PyErr_ExceptionMatches(PyExc_OverflowError)) {
    // Rewrite OverflowError -> ValueError.
    // But don't rewrite other errors such as TypeError.
    PyErr_Clear();
    PyErr_Format(PyExc_ValueError, "Value out of range: %S", obj);
  }
  return false;
}

static bool PyUpb_GetUint64(PyObject *obj, uint64_t *val) {
  // We require that the value is either an integer or has an __index__
  // conversion.
  if (!PyIndex_Check(obj)) {
    PyErr_Format(PyExc_TypeError, "Expected integer: %S", obj);
  }
  if (PyLong_Check(obj)) {
    *val = PyLong_AsUnsignedLongLong(obj);
  } else if (PyIndex_Check(obj)) {
    PyObject* casted = PyNumber_Long(obj);
    if (!casted) return false;
    *val = PyLong_AsUnsignedLongLong(casted);
    Py_DECREF(casted);
  } else {
    PyErr_Format(PyExc_TypeError, "Expected integer: %S", obj);
    return false;
  }
  if (!PyErr_Occurred()) return true;
  PyErr_Clear();
  PyErr_Format(PyExc_ValueError, "Value out of range: %S", obj);
  return false;
}

static bool PyUpb_GetInt32(PyObject *obj, int32_t *val) {
  int64_t i64;
  if (!PyUpb_GetInt64(obj, &i64)) return false;
  if (i64 < INT32_MIN || i64 > INT32_MAX) {
    PyErr_Format(PyExc_ValueError, "Value out of range: %S", obj);
    return false;
  }
  *val = i64;
  return true;
}

static bool PyUpb_GetUint32(PyObject *obj, uint32_t *val) {
  uint64_t u64;
  if (!PyUpb_GetUint64(obj, &u64)) return false;
  if (u64 > UINT32_MAX) {
    PyErr_Format(PyExc_ValueError, "Value out of range: %S", obj);
    return false;
  }
  *val = u64;
  return true;
}

// If `arena` is specified, copies the string data into the given arena.
// Otherwise aliases the given data.
static upb_msgval PyUpb_MaybeCopyString(const char *ptr, size_t size,
                                        upb_arena *arena) {
  upb_msgval ret;
  ret.str_val.size = size;
  if (arena) {
    char *buf = upb_arena_malloc(arena, size);
    memcpy(buf, ptr, size);
    ret.str_val.data = buf;
  } else {
    ret.str_val.data = ptr;
  }
  return ret;
}

static bool PyUpb_PyToUpbEnum(PyObject *obj, const upb_enumdef *e,
                              upb_msgval *val) {
  if (PyUnicode_Check(obj)) {
    Py_ssize_t size;
    const char *name = PyUnicode_AsUTF8AndSize(obj, &size);
    const upb_enumvaldef *ev = upb_enumdef_lookupname(e, name, size);
    if (!ev) {
      PyErr_Format(PyExc_ValueError, "unknown enum label \"%s\"", name);
      return false;
    }
    val->int32_val = upb_enumvaldef_number(ev);
    return true;
  } else {
    int32_t i32;
    if (!PyUpb_GetInt32(obj, &i32)) return false;
    if (upb_filedef_syntax(upb_enumdef_file(e)) == UPB_SYNTAX_PROTO2 &&
        !upb_enumdef_checknum(e, i32)) {
      PyErr_Format(PyExc_ValueError, "invalid enumerator %d", (int)i32);
      return false;
    }
    val->int32_val = i32;
    return true;
  }
}

bool PyUpb_PyToUpb(PyObject *obj, const upb_fielddef *f, upb_msgval *val,
                   upb_arena *arena) {
  switch (upb_fielddef_type(f)) {
    case UPB_TYPE_ENUM:
      return PyUpb_PyToUpbEnum(obj, upb_fielddef_enumsubdef(f), val);
    case UPB_TYPE_INT32:
      return PyUpb_GetInt32(obj, &val->int32_val);
    case UPB_TYPE_INT64:
      return PyUpb_GetInt64(obj, &val->int64_val);
    case UPB_TYPE_UINT32:
      return PyUpb_GetUint32(obj, &val->uint32_val);
    case UPB_TYPE_UINT64:
      return PyUpb_GetUint64(obj, &val->uint64_val);
    case UPB_TYPE_FLOAT:
      val->float_val = PyFloat_AsDouble(obj);
      return !PyErr_Occurred();
    case UPB_TYPE_DOUBLE:
      val->double_val = PyFloat_AsDouble(obj);
      return !PyErr_Occurred();
    case UPB_TYPE_BOOL:
      val->bool_val = PyLong_AsLong(obj);
      return !PyErr_Occurred();
    case UPB_TYPE_BYTES: {
      char *ptr;
      Py_ssize_t size;
      if (PyBytes_AsStringAndSize(obj, &ptr, &size) < 0) return false;
      *val = PyUpb_MaybeCopyString(ptr, size, arena);
      return true;
    }
    case UPB_TYPE_STRING: {
      Py_ssize_t size;
      const char *ptr;
      PyObject *unicode = NULL;
      if (PyBytes_Check(obj)) {
        unicode = obj = PyUnicode_FromEncodedObject(obj, "utf-8", NULL);
        if (!obj) return false;
      }
      ptr = PyUnicode_AsUTF8AndSize(obj, &size);
      if (PyErr_Occurred()) {
        Py_XDECREF(unicode);
        return false;
      }
      *val = PyUpb_MaybeCopyString(ptr, size, arena);
      Py_XDECREF(unicode);
      return true;
    }
    case UPB_TYPE_MESSAGE:
      PyErr_Format(
          PyExc_ValueError, "Message objects may not be assigned",
          upb_fielddef_type(f));
      return false;
    default:
      PyErr_Format(
          PyExc_SystemError, "Getting a value from a field of unknown type %d",
          upb_fielddef_type(f));
      return false;
  }
}

bool PyUpb_Message_IsEqual(const upb_msg *msg1, const upb_msg *msg2,
                           const upb_msgdef *m);

// -----------------------------------------------------------------------------
// Equal
// -----------------------------------------------------------------------------

bool PyUpb_ValueEq(upb_msgval val1, upb_msgval val2, const upb_fielddef *f) {
  switch (upb_fielddef_type(f)) {
    case UPB_TYPE_BOOL:
      return val1.bool_val == val2.bool_val;
    case UPB_TYPE_INT32:
    case UPB_TYPE_UINT32:
    case UPB_TYPE_ENUM:
      return val1.int32_val == val2.int32_val;
    case UPB_TYPE_INT64:
    case UPB_TYPE_UINT64:
      return val1.int64_val == val2.int64_val;
    case UPB_TYPE_FLOAT:
      return val1.float_val == val2.float_val;
    case UPB_TYPE_DOUBLE:
      return val1.double_val == val2.double_val;
    case UPB_TYPE_STRING:
    case UPB_TYPE_BYTES:
      return val1.str_val.size == val2.str_val.size &&
          memcmp(val1.str_val.data, val2.str_val.data, val1.str_val.size) == 0;
    case UPB_TYPE_MESSAGE:
      return PyUpb_Message_IsEqual(val1.msg_val, val2.msg_val,
                                   upb_fielddef_msgsubdef(f));
    default:
      return false;
  }
}

bool PyUpb_Map_IsEqual(const upb_map *map1, const upb_map *map2,
                       const upb_fielddef *f) {
  assert(upb_fielddef_ismap(f));
  if (map1 == map2) return true;

  size_t size1 = map1 ? upb_map_size(map1) : 0;
  size_t size2 = map2 ? upb_map_size(map2) : 0;
  if (size1 != size2) return false;
  if (size1 == 0) return true;

  const upb_msgdef *entry_m = upb_fielddef_msgsubdef(f);
  const upb_fielddef *val_f = upb_msgdef_field(entry_m, 1);
  size_t iter = UPB_MAP_BEGIN;

  while (upb_mapiter_next(map1, &iter)) {
    upb_msgval key = upb_mapiter_key(map1, iter);
    upb_msgval val1 = upb_mapiter_value(map1, iter);
    upb_msgval val2;
    if (!upb_map_get(map2, key, &val2)) return false;
    if (!PyUpb_ValueEq(val1, val2, val_f)) return false;
  }

  return true;
}

static bool PyUpb_ArrayElem_IsEqual(const upb_array *arr1,
                                    const upb_array *arr2, size_t i,
                                    const upb_fielddef *f) {
  assert(i < upb_array_size(arr1));
  assert(i < upb_array_size(arr2));
  upb_msgval val1 = upb_array_get(arr1, i);
  upb_msgval val2 = upb_array_get(arr2, i);
  return PyUpb_ValueEq(val1, val2, f);
}

bool PyUpb_Array_IsEqual(const upb_array *arr1, const upb_array *arr2,
                         const upb_fielddef *f) {
  assert(upb_fielddef_isseq(f) && !upb_fielddef_ismap(f));
  if (arr1 == arr2) return true;

  size_t n1 = arr1 ? upb_array_size(arr1) : 0;
  size_t n2 = arr2 ? upb_array_size(arr2) : 0;
  if (n1 != n2) return false;

  // Half the length rounded down.  Important: the empty list rounds to 0.
  size_t half = n1 / 2;

  // Search from the ends-in.  We expect differences to more quickly manifest
  // at the ends than in the middle.  If the length is odd we will miss the
  // middle element.
  for (size_t i = 0; i < half; i++) {
    if (!PyUpb_ArrayElem_IsEqual(arr1, arr2, i, f)) return false;
    if (!PyUpb_ArrayElem_IsEqual(arr1, arr2, n1 - 1 - i, f)) return false;
  }

  // For an odd-lengthed list, pick up the middle element.
  if (n1 & 1) {
    if (!PyUpb_ArrayElem_IsEqual(arr1, arr2, half, f)) return false;
  }

  return true;
}

bool PyUpb_Message_IsEqual(const upb_msg *msg1, const upb_msg *msg2,
                           const upb_msgdef *m) {
  if (msg1 == msg2) return true;
  if (upb_msg_extcount(msg1) != upb_msg_extcount(msg2)) return false;

  // Compare messages field-by-field.  This is slightly tricky, because while
  // we can iterate over normal fields in a predictable order, the extension
  // order is unpredictable and may be different between msg1 and msg2.
  // So we use the following strategy:
  //   1. Iterate over all msg1 fields (including extensions).
  //   2. For non-extension fields, we find the corresponding field by simply
  //      using upb_msg_next(msg2).  If the two messages have the same set of
  //      fields, this will yield the same field.
  //   3. For extension fields, we have to actually search for the corresponding
  //      field, which we do with upb_msg_get(msg2, ext_f1).
  //   4. Once iteration over msg1 is complete, we call upb_msg_next(msg2) one
  //      final time to verify that we have visited all of msg2's regular fields
  //      (we pass NULL for ext_dict so that iteration will *not* return
  //      extensions).
  //
  // We don't need to visit all of msg2's extensions, because we verified up
  // front that both messages have the same number of extensions.
  const upb_symtab* symtab = upb_filedef_symtab(upb_msgdef_file(m));
  const upb_fielddef *f1, *f2;
  upb_msgval val1, val2;
  size_t iter1 = UPB_MSG_BEGIN;
  size_t iter2 = UPB_MSG_BEGIN;
  while (upb_msg_next(msg1, m, symtab, &f1, &val1, &iter1)) {
    if (upb_fielddef_isextension(f1)) {
      val2 = upb_msg_get(msg2, f1);
    } else {
      if (!upb_msg_next(msg2, m, NULL, &f2, &val2, &iter2) || f1 != f2) {
        return false;
      }
    }

    if (upb_fielddef_ismap(f1)) {
      if (!PyUpb_Map_IsEqual(val1.map_val, val2.map_val, f1)) return false;
    } else if (upb_fielddef_isseq(f1)) {
      if (!PyUpb_Array_IsEqual(val1.array_val, val2.array_val, f1)) {
        return false;
      }
    } else {
      if (!PyUpb_ValueEq(val1, val2, f1)) return false;
    }
  }

  if (upb_msg_next(msg2, m, NULL, &f2, &val2, &iter2)) return false;

  size_t usize1, usize2;
  const char *uf1 = upb_msg_getunknown(msg1, &usize1);
  const char *uf2 = upb_msg_getunknown(msg2, &usize2);
  // 100 is arbitrary, we're trying to prevent stack overflow but it's not
  // obvious how deep we should allow here.
  return upb_Message_UnknownFieldsAreEqual(uf1, usize1, uf2, usize2, 100) ==
         kUpb_UnknownCompareResult_Equal;
}
