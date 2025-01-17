// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "python/convert.h"

#include "python/message.h"
#include "python/protobuf.h"
#include "upb/message/compare.h"
#include "upb/message/map.h"
#include "upb/reflection/def.h"
#include "upb/reflection/message.h"
#include "utf8_range.h"

// Must be last.
#include "upb/port/def.inc"

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

const char* upb_FieldDef_TypeString(const upb_FieldDef* f) {
  switch (upb_FieldDef_CType(f)) {
    case kUpb_CType_Double:
      return "double";
    case kUpb_CType_Float:
      return "float";
    case kUpb_CType_Int64:
      return "int64";
    case kUpb_CType_Int32:
      return "int32";
    case kUpb_CType_UInt64:
      return "uint64";
    case kUpb_CType_UInt32:
      return "uint32";
    case kUpb_CType_Enum:
      return "enum";
    case kUpb_CType_Bool:
      return "bool";
    case kUpb_CType_String:
      return "string";
    case kUpb_CType_Bytes:
      return "bytes";
    case kUpb_CType_Message:
      return "message";
  }
  UPB_UNREACHABLE();
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
    if (upb_EnumDef_IsClosed(e) && !upb_EnumDef_CheckNumber(e, i32)) {
      PyErr_Format(PyExc_ValueError, "invalid enumerator %d", (int)i32);
      return false;
    }
    val->int32_val = i32;
    return true;
  }
}

bool PyUpb_IsNumpyNdarray(PyObject* obj, const upb_FieldDef* f) {
  PyObject* type_name_obj =
      PyObject_GetAttrString((PyObject*)Py_TYPE(obj), "__name__");
  bool is_ndarray = false;
  if (!strcmp(PyUpb_GetStrData(type_name_obj), "ndarray")) {
    PyErr_Format(PyExc_TypeError,
                 "%S has type ndarray, but expected one of: %s", obj,
                 upb_FieldDef_TypeString(f));
    is_ndarray = true;
  }
  Py_DECREF(type_name_obj);
  return is_ndarray;
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
      if (!PyFloat_Check(obj) && PyUpb_IsNumpyNdarray(obj, f)) return false;
      val->float_val = PyFloat_AsDouble(obj);
      return !PyErr_Occurred();
    case kUpb_CType_Double:
      if (!PyFloat_Check(obj) && PyUpb_IsNumpyNdarray(obj, f)) return false;
      val->double_val = PyFloat_AsDouble(obj);
      return !PyErr_Occurred();
    case kUpb_CType_Bool:
      if (!PyBool_Check(obj) && PyUpb_IsNumpyNdarray(obj, f)) return false;
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
      if (PyBytes_Check(obj)) {
        // Use the object's bytes if they are valid UTF-8.
        char* ptr;
        if (PyBytes_AsStringAndSize(obj, &ptr, &size) < 0) return false;
        if (!utf8_range_IsValid(ptr, size)) {
          // Invalid UTF-8.  Try to convert the message to a Python Unicode
          // object, even though we know this will fail, just to get the
          // idiomatic Python error message.
          obj = PyUnicode_FromEncodedObject(obj, "utf-8", NULL);
          assert(!obj);
          return false;
        }
        *val = PyUpb_MaybeCopyString(ptr, size, arena);
        return true;
      } else {
        const char* ptr;
        ptr = PyUnicode_AsUTF8AndSize(obj, &size);
        if (PyErr_Occurred()) return false;
        *val = PyUpb_MaybeCopyString(ptr, size, arena);
        return true;
      }
    }
    case kUpb_CType_Message:
      PyErr_Format(PyExc_ValueError, "Message objects may not be assigned");
      return false;
    default:
      PyErr_Format(PyExc_SystemError,
                   "Getting a value from a field of unknown type %d",
                   upb_FieldDef_CType(f));
      return false;
  }
}

bool upb_Message_IsEqualByDef(const upb_Message* msg1, const upb_Message* msg2,
                              const upb_MessageDef* msgdef, int options) {
  const upb_MiniTable* m = upb_MessageDef_MiniTable(msgdef);
  return upb_Message_IsEqual(msg1, msg2, m, options);
}

#include "upb/port/undef.inc"
