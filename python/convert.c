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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL Google LLC BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
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

PyObject* PyUpb_UpbToPy(upb_MessageValue val, const upb_FieldDef* f,
                        PyObject* arena) {
  switch (upb_FieldDef_CType(f)) {
    case kUpb_CType_Enum:
    case kUpb_CType_Int32:
      return PyLong_FromLong(val.int32_val);
    case kUpb_CType_Int64:
      return PyLong_FromLongLong(val.int64_val);
    case kUpb_CType_UInt32:
      return PyLong_FromSize_t(val.uint32_val);
    case kUpb_CType_UInt64:
      return PyLong_FromUnsignedLongLong(val.uint64_val);
    case kUpb_CType_Float:
      return PyFloat_FromDouble(val.float_val);
    case kUpb_CType_Double:
      return PyFloat_FromDouble(val.double_val);
    case kUpb_CType_Bool:
      return PyBool_FromLong(val.bool_val);
    case kUpb_CType_Bytes:
      return PyBytes_FromStringAndSize(val.str_val.data, val.str_val.size);
    case kUpb_CType_String: {
      PyObject* ret =
          PyUnicode_DecodeUTF8(val.str_val.data, val.str_val.size, NULL);
      // If the string can't be decoded in UTF-8, just return a bytes object
      // that contains the raw bytes. This can't happen if the value was
      // assigned using the members of the Python message object, but can happen
      // if the values were parsed from the wire (binary).
      if (ret == NULL) {
        PyErr_Clear();
        ret = PyBytes_FromStringAndSize(val.str_val.data, val.str_val.size);
      }
      return ret;
    }
    case kUpb_CType_Message:
      return PyUpb_Message_Get((upb_Message*)val.msg_val,
                               upb_FieldDef_MessageSubDef(f), arena);
    default:
      PyErr_Format(PyExc_SystemError,
                   "Getting a value from a field of unknown type %d",
                   upb_FieldDef_CType(f));
      return NULL;
  }
}

static bool PyUpb_GetInt64(PyObject* obj, int64_t* val) {
  // We require that the value is either an integer or has an __index__
  // conversion.
  obj = PyNumber_Index(obj);
  if (!obj) return false;
  // If the value is already a Python long, PyLong_AsLongLong() retrieves it.
  // Otherwise is converts to integer using __int__.
  *val = PyLong_AsLongLong(obj);
  bool ok = true;
  if (PyErr_Occurred()) {
    assert(PyErr_ExceptionMatches(PyExc_OverflowError));
    PyErr_Clear();
    PyErr_Format(PyExc_ValueError, "Value out of range: %S", obj);
    ok = false;
  }
  Py_DECREF(obj);
  return ok;
}

static bool PyUpb_GetUint64(PyObject* obj, uint64_t* val) {
  // We require that the value is either an integer or has an __index__
  // conversion.
  obj = PyNumber_Index(obj);
  if (!obj) return false;
  *val = PyLong_AsUnsignedLongLong(obj);
  bool ok = true;
  if (PyErr_Occurred()) {
    assert(PyErr_ExceptionMatches(PyExc_OverflowError));
    PyErr_Clear();
    PyErr_Format(PyExc_ValueError, "Value out of range: %S", obj);
    ok = false;
  }
  Py_DECREF(obj);
  return ok;
}

static bool PyUpb_GetInt32(PyObject* obj, int32_t* val) {
  int64_t i64;
  if (!PyUpb_GetInt64(obj, &i64)) return false;
  if (i64 < INT32_MIN || i64 > INT32_MAX) {
    PyErr_Format(PyExc_ValueError, "Value out of range: %S", obj);
    return false;
  }
  *val = i64;
  return true;
}

static bool PyUpb_GetUint32(PyObject* obj, uint32_t* val) {
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
static upb_MessageValue PyUpb_MaybeCopyString(const char* ptr, size_t size,
                                              upb_Arena* arena) {
  upb_MessageValue ret;
  ret.str_val.size = size;
  if (arena) {
    char* buf = upb_Arena_Malloc(arena, size);
    memcpy(buf, ptr, size);
    ret.str_val.data = buf;
  } else {
    ret.str_val.data = ptr;
  }
  return ret;
}

static bool PyUpb_PyToUpbEnum(PyObject* obj, const upb_EnumDef* e,
                              upb_MessageValue* val) {
  if (PyUnicode_Check(obj)) {
    Py_ssize_t size;
    const char* name = PyUnicode_AsUTF8AndSize(obj, &size);
    const upb_EnumValueDef* ev =
        upb_EnumDef_FindValueByNameWithSize(e, name, size);
    if (!ev) {
      PyErr_Format(PyExc_ValueError, "unknown enum label \"%s\"", name);
      return false;
    }
    val->int32_val = upb_EnumValueDef_Number(ev);
    return true;
  } else {
    int32_t i32;
    if (!PyUpb_GetInt32(obj, &i32)) return false;
    if (upb_FileDef_Syntax(upb_EnumDef_File(e)) == kUpb_Syntax_Proto2 &&
        !upb_EnumDef_CheckNumber(e, i32)) {
      PyErr_Format(PyExc_ValueError, "invalid enumerator %d", (int)i32);
      return false;
    }
    val->int32_val = i32;
    return true;
  }
}

bool PyUpb_PyToUpb(PyObject* obj, const upb_FieldDef* f, upb_MessageValue* val,
                   upb_Arena* arena) {
  switch (upb_FieldDef_CType(f)) {
    case kUpb_CType_Enum:
      return PyUpb_PyToUpbEnum(obj, upb_FieldDef_EnumSubDef(f), val);
    case kUpb_CType_Int32:
      return PyUpb_GetInt32(obj, &val->int32_val);
    case kUpb_CType_Int64:
      return PyUpb_GetInt64(obj, &val->int64_val);
    case kUpb_CType_UInt32:
      return PyUpb_GetUint32(obj, &val->uint32_val);
    case kUpb_CType_UInt64:
      return PyUpb_GetUint64(obj, &val->uint64_val);
    case kUpb_CType_Float:
      val->float_val = PyFloat_AsDouble(obj);
      return !PyErr_Occurred();
    case kUpb_CType_Double:
      val->double_val = PyFloat_AsDouble(obj);
      return !PyErr_Occurred();
    case kUpb_CType_Bool:
      val->bool_val = PyLong_AsLong(obj);
      return !PyErr_Occurred();
    case kUpb_CType_Bytes: {
      char* ptr;
      Py_ssize_t size;
      if (PyBytes_AsStringAndSize(obj, &ptr, &size) < 0) return false;
      *val = PyUpb_MaybeCopyString(ptr, size, arena);
      return true;
    }
    case kUpb_CType_String: {
      Py_ssize_t size;
      const char* ptr;
      PyObject* unicode = NULL;
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
    case kUpb_CType_Message:
      PyErr_Format(PyExc_ValueError, "Message objects may not be assigned",
                   upb_FieldDef_CType(f));
      return false;
    default:
      PyErr_Format(PyExc_SystemError,
                   "Getting a value from a field of unknown type %d",
                   upb_FieldDef_CType(f));
      return false;
  }
}

bool upb_Message_IsEqual(const upb_Message* msg1, const upb_Message* msg2,
                         const upb_MessageDef* m);

// -----------------------------------------------------------------------------
// Equal
// -----------------------------------------------------------------------------

bool PyUpb_ValueEq(upb_MessageValue val1, upb_MessageValue val2,
                   const upb_FieldDef* f) {
  switch (upb_FieldDef_CType(f)) {
    case kUpb_CType_Bool:
      return val1.bool_val == val2.bool_val;
    case kUpb_CType_Int32:
    case kUpb_CType_UInt32:
    case kUpb_CType_Enum:
      return val1.int32_val == val2.int32_val;
    case kUpb_CType_Int64:
    case kUpb_CType_UInt64:
      return val1.int64_val == val2.int64_val;
    case kUpb_CType_Float:
      return val1.float_val == val2.float_val;
    case kUpb_CType_Double:
      return val1.double_val == val2.double_val;
    case kUpb_CType_String:
    case kUpb_CType_Bytes:
      return val1.str_val.size == val2.str_val.size &&
             memcmp(val1.str_val.data, val2.str_val.data, val1.str_val.size) ==
                 0;
    case kUpb_CType_Message:
      return upb_Message_IsEqual(val1.msg_val, val2.msg_val,
                                 upb_FieldDef_MessageSubDef(f));
    default:
      return false;
  }
}

bool PyUpb_Map_IsEqual(const upb_Map* map1, const upb_Map* map2,
                       const upb_FieldDef* f) {
  assert(upb_FieldDef_IsMap(f));
  if (map1 == map2) return true;

  size_t size1 = map1 ? upb_Map_Size(map1) : 0;
  size_t size2 = map2 ? upb_Map_Size(map2) : 0;
  if (size1 != size2) return false;
  if (size1 == 0) return true;

  const upb_MessageDef* entry_m = upb_FieldDef_MessageSubDef(f);
  const upb_FieldDef* val_f = upb_MessageDef_Field(entry_m, 1);
  size_t iter = kUpb_Map_Begin;

  while (upb_MapIterator_Next(map1, &iter)) {
    upb_MessageValue key = upb_MapIterator_Key(map1, iter);
    upb_MessageValue val1 = upb_MapIterator_Value(map1, iter);
    upb_MessageValue val2;
    if (!upb_Map_Get(map2, key, &val2)) return false;
    if (!PyUpb_ValueEq(val1, val2, val_f)) return false;
  }

  return true;
}

static bool PyUpb_ArrayElem_IsEqual(const upb_Array* arr1,
                                    const upb_Array* arr2, size_t i,
                                    const upb_FieldDef* f) {
  assert(i < upb_Array_Size(arr1));
  assert(i < upb_Array_Size(arr2));
  upb_MessageValue val1 = upb_Array_Get(arr1, i);
  upb_MessageValue val2 = upb_Array_Get(arr2, i);
  return PyUpb_ValueEq(val1, val2, f);
}

bool PyUpb_Array_IsEqual(const upb_Array* arr1, const upb_Array* arr2,
                         const upb_FieldDef* f) {
  assert(upb_FieldDef_IsRepeated(f) && !upb_FieldDef_IsMap(f));
  if (arr1 == arr2) return true;

  size_t n1 = arr1 ? upb_Array_Size(arr1) : 0;
  size_t n2 = arr2 ? upb_Array_Size(arr2) : 0;
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

bool upb_Message_IsEqual(const upb_Message* msg1, const upb_Message* msg2,
                         const upb_MessageDef* m) {
  if (msg1 == msg2) return true;
  if (upb_Message_ExtensionCount(msg1) != upb_Message_ExtensionCount(msg2))
    return false;

  // Compare messages field-by-field.  This is slightly tricky, because while
  // we can iterate over normal fields in a predictable order, the extension
  // order is unpredictable and may be different between msg1 and msg2.
  // So we use the following strategy:
  //   1. Iterate over all msg1 fields (including extensions).
  //   2. For non-extension fields, we find the corresponding field by simply
  //      using upb_Message_Next(msg2).  If the two messages have the same set
  //      of fields, this will yield the same field.
  //   3. For extension fields, we have to actually search for the corresponding
  //      field, which we do with upb_Message_Get(msg2, ext_f1).
  //   4. Once iteration over msg1 is complete, we call upb_Message_Next(msg2)
  //   one
  //      final time to verify that we have visited all of msg2's regular fields
  //      (we pass NULL for ext_dict so that iteration will *not* return
  //      extensions).
  //
  // We don't need to visit all of msg2's extensions, because we verified up
  // front that both messages have the same number of extensions.
  const upb_DefPool* symtab = upb_FileDef_Pool(upb_MessageDef_File(m));
  const upb_FieldDef *f1, *f2;
  upb_MessageValue val1, val2;
  size_t iter1 = kUpb_Message_Begin;
  size_t iter2 = kUpb_Message_Begin;
  while (upb_Message_Next(msg1, m, symtab, &f1, &val1, &iter1)) {
    if (upb_FieldDef_IsExtension(f1)) {
      val2 = upb_Message_Get(msg2, f1);
    } else {
      if (!upb_Message_Next(msg2, m, NULL, &f2, &val2, &iter2) || f1 != f2) {
        return false;
      }
    }

    if (upb_FieldDef_IsMap(f1)) {
      if (!PyUpb_Map_IsEqual(val1.map_val, val2.map_val, f1)) return false;
    } else if (upb_FieldDef_IsRepeated(f1)) {
      if (!PyUpb_Array_IsEqual(val1.array_val, val2.array_val, f1)) {
        return false;
      }
    } else {
      if (!PyUpb_ValueEq(val1, val2, f1)) return false;
    }
  }

  if (upb_Message_Next(msg2, m, NULL, &f2, &val2, &iter2)) return false;

  size_t usize1, usize2;
  const char* uf1 = upb_Message_GetUnknown(msg1, &usize1);
  const char* uf2 = upb_Message_GetUnknown(msg2, &usize2);
  // 100 is arbitrary, we're trying to prevent stack overflow but it's not
  // obvious how deep we should allow here.
  return upb_Message_UnknownFieldsAreEqual(uf1, usize1, uf2, usize2, 100) ==
         kUpb_UnknownCompareResult_Equal;
}
