/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2009 Joshua Haberman.  See LICENSE for details.
 *
 * This file implements an interface to Python that is compatible
 * (as much as possible) with proto1 (the first implementation of
 * protocol buffers, which is only released internally to Google).
 */

#include <Python.h>
#include <stddef.h>
#include "upb_mm.h"
#include "definition.h"

/* Opcodes that describe all of the operations you can perform on a field of a
 * protobuf from Python.  For example, foo.has_bar() uses opcode OP_HAS. */
typedef enum {
  /* For non-repeated fields. */
  OP_HAS,
  /* For non-repeated fields that are not submessages. */
  OP_SET,
  /* For non-repeated message fields. */
  OP_MUTABLE,

  /* For repeated fields. */
  OP_SIZE, OP_LIST, OP_ADD,

  /* For all types of fields. */
  OP_GET, OP_CLEAR
} PyUpb_PbBoundFieldOpCode;

const char *opcode_names[] = {
  "OP_HAS", "OP_SET", "OP_MUTABLE", "OP_SIZE", "OP_LIST", "OP_ADD", "OP_GET", "OP_CLEAR"
};

/* Structures for the Python objects we define. */

/* Callable that will create a new message object of a specific type.  In this
 * sense it sort of "pretends" to be a type, but it is not actually a type. */
typedef struct {
  PyObject_HEAD;
  PyUpb_MsgDef *def;
} PyUpb_PbMsgCreator;

/* Message object.  All messages use this structure and have the same Python
 * type (even if their .proto types are different).  The type dictionary for
 * this type does not include field accessors -- those are dynamically looked
 * up in msg_getattro. */
typedef struct {
  PyObject_HEAD;
  struct upb_mm_ref ref;
  PyUpb_MsgDef *def;
} PyUpb_PbMsg;

/* Represents a "bound" operation like obj.has_foo, that will perform the
 * operation when called.  This is necessary because proto1 has all of its
 * operations modeled as methods, so one calls obj.has_foo(), not obj.has_foo
 * alone. */
typedef struct {
  PyObject_HEAD;
  PyUpb_PbMsg *msg;
  struct upb_msg_fielddef *f;
  PyUpb_PbBoundFieldOpCode code;
} PyUpb_PbBoundFieldOp;

static PyTypeObject PyUpb_PbMsgCreatorType;
static PyTypeObject PyUpb_PbMsgType;
static PyTypeObject PyUpb_PbBoundFieldOpType;

#define Check_MsgCreator(obj) \
  (void*)obj; do { \
    if(!PyObject_TypeCheck(obj, &PyUpb_PbMsgCreatorType)) { \
      PyErr_SetString(PyExc_TypeError, "must be a MessageCreator"); \
      return NULL; \
    } \
  } while(0)

#define Check_Message(obj) \
  (void*)obj; do { \
    if(!PyObject_TypeCheck(obj, &PyUpb_PbMsgType)) { \
      PyErr_SetString(PyExc_TypeError, "must be a Message"); \
      return NULL; \
    } \
  } while(0)

#define Check_BoundFieldOp(obj) \
  (void*)obj; do { \
    if(!PyObject_TypeCheck(obj, &PyUpb_PbBoundFieldOpType)) { \
      PyErr_SetString(PyExc_TypeError, "must be a BoundFieldOp"); \
      return NULL; \
    } \
  } while(0)

#define EXPECT_NO_ARGS if(!PyArg_ParseTuple(args, "")) return NULL;
#define MMREF_TO_PYOBJ(mmref) (PyObject*)((char*)(mmref)-offsetof(PyUpb_PbMsg, ref))

static struct upb_mm_ref *NewPyRef(struct upb_mm_ref *fromref,
                                   union upb_mmptr p, upb_mm_ptrtype type)
{
  (void)fromref;  /* Don't care. */
  struct upb_mm_ref *ref = NULL;
  switch(type) {
    case UPB_MM_MSG_REF: {
      PyUpb_PbMsg *msg = (void*)PyUpb_PbMsgType.tp_alloc(&PyUpb_PbMsgType, 0);
      msg->def = get_or_create_msgdef(p.msg->def);  /* gets a ref. */
      ref = &msg->ref;
      break;
    }
    case UPB_MM_STR_REF: {
    }
    case UPB_MM_ARR_REF: {
    }
    default: assert(false); abort(); break;  /* Shouldn't happen. */
  }
  return ref;
}

struct upb_mm pymm = {NewPyRef};

/* upb.pb.BoundFieldOp ********************************************************/

static PyObject *upb_to_py(union upb_value_ptr p, upb_field_type_t type)
{
  switch(type) {
    default:
      PyErr_SetString(PyExc_RuntimeError, "internal: unexpected type");
      return NULL;
    case GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_DOUBLE:
      return PyFloat_FromDouble(*p._double);
    case GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_FLOAT:
      return PyFloat_FromDouble(*p._float);
    case GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_INT64:
    case GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_SINT64:
    case GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_SFIXED64:
      return PyLong_FromLongLong(*p.int64);
    case GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_UINT64:
    case GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_FIXED64:
      return PyLong_FromUnsignedLongLong(*p.uint64);
    case GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_SFIXED32:
    case GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_SINT32:
    case GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_INT32:
    case GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_ENUM:
#if PY_MAJOR_VERSION >= 3
      return PyLong_FromLong(*p.int32);
#else
      return PyInt_FromLong(*p.int32);
#endif

    case GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_FIXED32:
    case GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_UINT32:
      return PyLong_FromLong(*p.uint32);
    case GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_BOOL:
      RETURN_BOOL(*p._bool);
    case GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_STRING:
    case GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_BYTES:
      /* Py3k will distinguish between these two. */
      return PyString_FromStringAndSize((*p.str)->ptr, (*p.str)->byte_len);
    case GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_GROUP:
    case GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_MESSAGE: {
      union upb_mmptr mmptr = upb_mmptr_read(p, UPB_MM_MSG_REF);
      bool created;
      struct upb_mm_ref *ref = upb_mm_getref(mmptr, UPB_MM_MSG_REF, &pymm, &created);
      PyObject *obj = MMREF_TO_PYOBJ(ref);
      if(!created) Py_INCREF(obj);
      return obj;
    }
  }
}

static long convert_to_long(PyObject *val, long lobound, long hibound, bool *ok)
{
  PyObject *o = PyNumber_Int(val);
  if(!o) {
    PyErr_SetString(PyExc_OverflowError, "could not convert to long");
    *ok = false;
    return -1;
  }
  long longval = PyInt_AS_LONG(o);
  if(longval > hibound || longval < lobound) {
    PyErr_SetString(PyExc_OverflowError, "value outside type bounds");
    *ok = false;
    return -1;
  }
  *ok = true;
  return longval;
}

static void set_upbscalarfield(union upb_value_ptr p, PyObject *val,
                               upb_field_type_t type)
{
  switch(type) {
    default:
      PyErr_SetString(PyExc_RuntimeError, "internal error");
      return;
    case GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_DOUBLE: {
      PyObject *o = PyNumber_Float(val);
      if(!o) {
        PyErr_SetString(PyExc_ValueError, "could not convert to double");
        return;
      }
      *p._double = PyFloat_AS_DOUBLE(o);
      return;
    }
    case GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_FLOAT: {
      PyObject *o = PyNumber_Float(val);
      if(!o) {
        PyErr_SetString(PyExc_ValueError, "could not convert to float");
        return;
      }
      *p._float = PyFloat_AS_DOUBLE(o);
      return;
    }
    case GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_INT64:
    case GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_SINT64:
    case GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_SFIXED64: {
#if LONG_MAX >= INT64_MAX
      bool ok;
      long longval = convert_to_long(val, INT64_MIN, INT64_MAX, &ok);
      if(ok) *p.int32 = longval;
      return;
#else
      PyObject *o = PyNumber_Long(val);
      if(!o) {
        PyErr_SetString(PyExc_ValueError, "could not convert to int64");
        return;
      }
      *p.int64 = PyLong_AsLongLong(o);
      return;
#endif
    }
    case GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_UINT64:
    case GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_FIXED64: {
      PyObject *o = PyNumber_Long(val);
      if(!o) {
        PyErr_SetString(PyExc_ValueError, "could not convert to uint64");
        return;
      }
      *p.uint64 = PyLong_AsUnsignedLongLong(o);
      return;
    }
    case GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_SFIXED32:
    case GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_SINT32:
    case GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_INT32:
    case GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_ENUM: {
      bool ok;
      long longval = convert_to_long(val, INT32_MIN, INT32_MAX, &ok);
      if(ok) *p.int32 = longval;
      return;
    }

    case GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_FIXED32:
    case GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_UINT32: {
#if LONG_MAX >= UINT32_MAX
      bool ok;
      long longval = convert_to_long(val, 0, UINT32_MAX, &ok);
      if(ok) *p.int32 = longval;
      return;
#else
      PyObject *o = PyNumber_Long(val);
      if(!o) {
        PyErr_SetString(PyExc_ValueError, "could not convert to uint32");
        return;
      }
      *p.uint32 = PyLong_AsUnsignedLong(o);
      return;
#endif
    }

    case GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_BOOL:
      if(!PyBool_Check(val)) {
        PyErr_SetString(PyExc_ValueError, "should be true or false");
        return;
      }
      if(val == Py_True) *p._bool = true;
      else if(val == Py_False) *p._bool = false;
      else PyErr_SetString(PyExc_RuntimeError, "not true or false?");
      return;

    case GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_STRING:
    case GOOGLE_PROTOBUF_FIELDDESCRIPTORPROTO_TYPE_BYTES: {
      size_t len = PyString_GET_SIZE(val);
      upb_string_resize(*p.str, len);
      memcpy((*p.str)->ptr, PyString_AS_STRING(val), len);
      return;
    }
  }
}

static bool check_py_type(PyObject *obj, upb_field_type_t type)
{
  /* TODO */
  return true;
}

PyObject* fieldop_call(PyObject *callable, PyObject *args, PyObject *kw)
{
  PyUpb_PbBoundFieldOp *op = Check_BoundFieldOp(callable);
  PyUpb_PbMsg *pymsg = op->msg;
  struct upb_mm_ref *msgref = &(pymsg->ref);
  struct upb_msg *msg = pymsg->ref.p.msg;
  struct upb_msg_fielddef *f = op->f;
  union upb_value_ptr p = upb_msg_getptr(msg, f);
  switch(op->code) {
    case OP_HAS:
      /* obj.has_foo() */
      EXPECT_NO_ARGS;
      RETURN_BOOL(upb_msg_isset(msg, f));
    case OP_SET: {
      PyObject *val;
      if(upb_isarray(f)) {
        /* obj.set_repeatedfoo(i, val) */
        int i;
        if(!PyArg_ParseTuple(args, "iO", &i, &val)) return NULL;
        if(!upb_msg_isset(msg, f) || i >= (*p.arr)->len) {
          PyErr_SetString(PyExc_IndexError, "assignment to invalid index");
          return NULL;
        }
        p = upb_array_getelementptr(*p.arr, i);
      } else {
        /* obj.set_foo(val) */
        if(!PyArg_ParseTuple(args, "O", &val)) return NULL;
      }
      set_upbscalarfield(p, val, f->type);
      if(PyErr_Occurred()) return NULL;
      Py_RETURN_NONE;
    }
    case OP_MUTABLE: {
      /* obj.mutable_scalarmsg() */
      EXPECT_NO_ARGS;
      bool created;
      PyObject *obj = MMREF_TO_PYOBJ(upb_mm_getfieldref(msgref, f, &created));
      if(!created) Py_INCREF(obj);
      return obj;
    }

    /* For repeated fields. */
    case OP_SIZE: {
      /* obj.repeatedfoo_size() */
      EXPECT_NO_ARGS;
      long len =
          upb_msg_isset(msg, f) ? (*upb_msg_getptr(msg, f).arr)->len : 0;
      return PyInt_FromLong(len);
    }
    case OP_LIST:
      /* obj.repeatedfoo_list() */
    case OP_ADD: {
      /* Parse/Verify the args. */
      PyObject *val;
      if(upb_issubmsg(f)) {
        /* obj.add_submsgfoo()  # returns the new submsg */
        EXPECT_NO_ARGS;
      } else {
        /* obj.add_scalarfoo(val) */
        if(!PyArg_ParseTuple(args, "O", &val)) return NULL;
        if(!check_py_type(val, f->type)) return NULL;
      }

      upb_arraylen_t len = (*p.arr)->len;
      union upb_value_ptr elem_p = upb_array_getelementptr(*p.arr, len);
      upb_array_append(*p.arr);

      if(upb_issubmsg(f)) {
        /* string or submsg. */
        bool created;
        upb_mm_ptrtype type = upb_elem_ptrtype(f);
        union upb_mmptr mmptr = upb_mmptr_read(elem_p, type);
        struct upb_mm_ref *valref = upb_mm_getref(mmptr, type, &pymm, &created);
        assert(created);
        PyObject *obj = MMREF_TO_PYOBJ(valref);
        return obj;
      } else {
        set_upbscalarfield(elem_p, val, f->type);
        if(PyErr_Occurred()) return NULL;
        Py_RETURN_NONE;
      }
    }

    /* For all fields. */
    case OP_GET: {
      if(upb_isarray(f)) {
        /* obj.repeatedfoo(i) */
        int i;
        if(!PyArg_ParseTuple(args, "i", &i)) return NULL;
        if(!upb_msg_isset(msg, f) || i >= (*p.arr)->len) {
          PyErr_SetString(PyExc_IndexError, "get from invalid index");
          return NULL;
        }
        p = upb_array_getelementptr(*p.arr, i);
      } else {
        /* obj.foo() */
        EXPECT_NO_ARGS;
      }
      return upb_to_py(p, f->type);
    }
    case OP_CLEAR:
      /* obj.clear_foo() */
      EXPECT_NO_ARGS;
      upb_mm_msgclear(msgref, f);
      Py_RETURN_NONE;

    default:
      PyErr_SetString(PyExc_RuntimeError, "invalid bound field opcode.");
      return NULL;
  }
}

static void fieldop_dealloc(PyObject *obj)
{
  PyUpb_PbBoundFieldOp *op = (void*)obj;
  Py_DECREF(op->msg);
  obj->ob_type->tp_free(obj);
}

static PyObject *fieldop_repr(PyObject *obj)
{
  PyUpb_PbBoundFieldOp *op = Check_BoundFieldOp(obj);
  struct upb_string *name = op->msg->def->def->descriptor->name;
  /* Need to get a NULL-terminated copy of name since PyString_FromFormat
   * doesn't support ptr+len. */
  PyObject *nameobj = PyString_FromStringAndSize(name->ptr, name->byte_len);
  struct google_protobuf_FieldDescriptorProto *fd =
      upb_msg_field_descriptor(op->f, op->msg->def->def);
  PyObject *fieldnameobj = PyString_FromStringAndSize(fd->name->ptr, fd->name->byte_len);
  PyObject *ret =
      PyString_FromFormat("<upb.pb.BoundFieldOp field='%s', op=%s, msgtype='%s'>",
                          PyString_AS_STRING(fieldnameobj),
                          opcode_names[op->code], PyString_AS_STRING(nameobj));
  Py_DECREF(nameobj);
  Py_DECREF(fieldnameobj);
  return ret;
}

static PyTypeObject PyUpb_PbBoundFieldOpType = {
  PyObject_HEAD_INIT(NULL)
  0,                                      /* ob_size */
  "upb.pb.BoundFieldOp",                  /* tp_name */
  sizeof(PyUpb_PbBoundFieldOp),           /* tp_basicsize */
  0,                                      /* tp_itemsize */
  fieldop_dealloc,                        /* tp_dealloc */
  0,                                      /* tp_print */
  0,                                      /* tp_getattr */
  0,                                      /* tp_setattr */
  0,                                      /* tp_compare */
  fieldop_repr,                           /* tp_repr */
  0,                                      /* tp_as_number */
  0,                                      /* tp_as_sequence */
  0,                                      /* tp_as_mapping */
  0,                                      /* tp_hash */
  fieldop_call,                           /* tp_call */
  0,                                      /* tp_str */
  0,                                      /* tp_getattro */
  0,                                      /* tp_setattro */
  0,                                      /* tp_as_buffer */
  Py_TPFLAGS_DEFAULT,                     /* tp_flags */
  0,                                      /* tp_doc */
  0,                                      /* tp_traverse */
  0,                                      /* tp_clear */
  0,                                      /* tp_richcompare */
  0,                                      /* tp_weaklistoffset */
  0,                                      /* tp_iter */
  0,                                      /* tp_iternext */
  0,                                      /* tp_methods */
  0,                                      /* tp_members */
  0,                                      /* tp_getset */
  0,                                      /* tp_base */
  0,                                      /* tp_dict */
  0,                                      /* tp_descr_get */
  0,                                      /* tp_descr_set */
  0,                                      /* tp_dictoffset */
  0,                                      /* tp_init */
  0,                                      /* tp_alloc */
  0, /* Can't be created from Python. */  /* tp_new */
  0,                                      /* tp_free */
};

/* upb.pb.Message *************************************************************/

#define Check_SameProtoType(obj1, obj2) \
  do { \
    if(self->ob_type != other->ob_type) { \
      PyErr_SetString(PyExc_TypeError, "other must be of the same type"); \
      return NULL; \
    } \
  } while(0);

static PyObject *msg_clear(PyObject *self, PyObject *args)
{
  (void)args;
  PyUpb_PbMsg *msg = Check_Message(self);
  upb_mm_msgclear_all(&msg->ref);
  Py_RETURN_NONE;
}

//static PyObject *msg_encode(PyObject *self, PyObject *args)
//{
//  (void)args;
//  PyUpb_PbMsg *msg = Check_Message(self);
//  struct upb_msgsizes *sizes = upb_msgsizes_new();
//  struct upb_msg *upb_msg = msg->ref.p.msg;
//  upb_msgsizes_read(sizes, upb_msg);
//
//  size_t size = upb_msgsizes_totalsize(sizes);
//  PyObject *str = PyString_FromStringAndSize(NULL, size);
//  if(!str) return NULL;
//  char *strbuf = PyString_AS_STRING(str);
//
//  bool success = upb_msg_serialize_all(upb_msg, sizes, strbuf);
//  upb_msgsizes_free(sizes);
//  if(success) {
//    return str;
//  } else {
//    /* TODO: better error than TypeError. */
//    PyErr_SetString(PyExc_TypeError, "Error serializing protobuf.");
//    return NULL;
//  }
//}

static PyObject *msg_equals(PyObject *self, PyObject *other)
{
  PyUpb_PbMsg *msg1 = Check_Message(self);
  PyUpb_PbMsg *msg2 = Check_Message(other);
  Check_SameProtoType(msg1, msg2);
  RETURN_BOOL(upb_msg_eql(msg1->ref.p.msg, msg2->ref.p.msg, true))
}

static PyObject *msg_isinitialized(PyObject *self, PyObject *args)
{
  (void)args;
  PyUpb_PbMsg *msg = Check_Message(self);
  RETURN_BOOL(upb_msg_all_required_fields_set(msg->ref.p.msg))
}

static PyObject *msg_parsefromstring(PyObject *self, PyObject *args)
{
  PyUpb_PbMsg *msg = Check_Message(self);
  char *strdata;
  size_t strlen;
  if(!PyArg_ParseTuple(args, BYTES_FORMAT, &strdata, &strlen))
    return NULL;

  if(upb_msg_parsestr(msg->ref.p.msg, strdata, strlen) != UPB_STATUS_OK) {
    /* TODO: better error than TypeError. */
    PyErr_SetString(PyExc_TypeError, "error parsing protobuf");
    return NULL;
  }
  Py_RETURN_NONE;
}

static PyObject *msg_mergefromstring(PyObject *self, PyObject *args)
{
  PyUpb_PbMsg *msg = Check_Message(self);
  char *strdata;
  size_t strlen;
  if(!PyArg_ParseTuple(args, BYTES_FORMAT, &strdata, &strlen))
    return NULL;

  if(upb_msg_parsestr(msg->ref.p.msg, strdata, strlen) != UPB_STATUS_OK) {
    /* TODO: better error than TypeError. */
    PyErr_SetString(PyExc_TypeError, "error parsing protobuf");
    return NULL;
  }
  Py_RETURN_NONE;
}

/* Commented-out methods are TODO. */
static PyMethodDef msg_methods[] = {
  {"Clear", msg_clear, METH_NOARGS,
   "Erases all data from the ProtocolMessage, reseting fields to their defaults"
  },
  //{"CopyFrom", msg_copyfrom, METH_O,
  // "Copies data from another ProtocolMessage."
  //},
  //{"Encode", msg_encode, METH_NOARGS,
  // "Returns a string representing the ProtocolMessage."
  //},
  {"Equals", msg_equals, METH_O,
   "Returns true if the given ProtocolMessage has the same type and value."
  },
  {"IsInitialized", msg_isinitialized, METH_NOARGS,
   "Returns true iff all required fields have been set."
  },
  //{"Merge", msg_merge, METH_O,
  // "Merges data from the given Decoder."
  //},
  //{"MergeFrom", msg_mergefrom, METH_O,
  // "Merges data from another ProtocolMessage of the same type."
  //},
  {"MergeFromString", msg_mergefromstring, METH_VARARGS,
   "Merges data from the given string.  Raises an exception if this does not "
   "result in the ProtocolMessage being initialized."
  },
  //{"Output", msg_output, METH_O,
  // "Writes the ProtocolMessage to the given encoder."
  //},
  //{"OutputUnchecked", msg_output, METH_O,
  // "Writes the ProtocolMessage to the given encoder, without checking "
  // "initialization"
  //},
  //{"Parse", msg_parse, METH_O,
  // "Parses data from the given Decoder."
  //},
  //{"ParseASCII", msg_parseascii, METH_VARARGS,
  // "Parses a string generated by ToASCII.  Raises a ValueError if unknown "
  // "fields are encountered."
  //},
  //{"ParseASCIIIgnoreUnknown", msg_parseascii, METH_VARARGS,
  // "Parses a string generated by ToASCII.  Ignores unknown fields."
  //},
  {"ParseFromString", msg_parsefromstring, METH_VARARGS,
   "Parses data from the given string.  Raises an exception if this does not "
   "result in the ProtocolMessage being initialized."
  },
  //{"ToASCII", msg_toascii, METH_NOARGS,
  // "Returns the ProtocolMessage as a human-readable ASCII string."
  //},
  //{"ToCompactASCII", msg_tocompactascii, METH_NOARGS,
  // "Returns the ProtocolMessage as a human-readable ASCII string that uses "
  // "tag numbers instead of field names."
  //},
  //{"ToShortASCII", msg_toshortascii, METH_NOARGS,
  // "Returns the ProtocolMessage as a human-readable ASCII string, all on one
  // "line."
  //},
  //{"TryMerge", msg_trymerge, METH_O,
  // "Merges data from the given decoder.
  //}
  {NULL, NULL}
};

static bool starts_with(struct upb_string *str, struct upb_string *prefix,
                        struct upb_string *out_str)
{
  if(str->byte_len < prefix->byte_len) return false;
  if(memcmp(str->ptr, prefix->ptr, prefix->byte_len) == 0) {
    out_str->ptr = str->ptr + prefix->byte_len;
    out_str->byte_len = str->byte_len - prefix->byte_len;
    return true;
  } else {
    return false;
  }
}

static bool ends_with(struct upb_string *str, struct upb_string *suffix,
                      struct upb_string *out_str)
{
  if(str->byte_len < suffix->byte_len) return false;
  if(memcmp(str->ptr + str->byte_len - suffix->byte_len, suffix->ptr, suffix->byte_len) == 0) {
    out_str->ptr = str->ptr;
    out_str->byte_len = str->byte_len - suffix->byte_len;
    return true;
  } else {
    return false;
  }
}

PyObject *PyUpb_NewPbBoundFieldOp(PyUpb_PbMsg *msgobj, struct upb_msg_fielddef *f,
                                  PyUpb_PbBoundFieldOpCode code)
{
  /* Type check that this operation on a field of this type makes sense.  */
  if(upb_isarray(f)) {
    switch(code) {
      case OP_HAS:
      case OP_SET:
      case OP_MUTABLE:
        return NULL;
      default: break;
    }
  } else {
    if(upb_issubmsg(f)) {
      switch(code) {
        case OP_SET:
        case OP_SIZE:
        case OP_LIST:
        case OP_ADD:
          return NULL;
        default: break;
      }
    } else {
      switch(code) {
        case OP_MUTABLE:
        case OP_SIZE:
        case OP_LIST:
        case OP_ADD:
          return NULL;
        default: break;
      }
    }
  }

  PyUpb_PbBoundFieldOp *op =
      (void*)PyUpb_PbBoundFieldOpType.tp_alloc(&PyUpb_PbBoundFieldOpType, 0);
  op->msg = msgobj;
  op->f = f;
  op->code = code;
  Py_INCREF(op->msg);
  return (PyObject*)op;
}

PyObject* msg_getattro(PyObject *obj, PyObject *attr_name)
{
  /* Each protobuf field results in a set of four methods for a scalar or five
   * methods for an array.  To avoid putting 4f entries in our type dict, we
   * dynamically scan the method to see if it is of these forms, and if so,
   * look it up in the hash table that upb already keeps.
   *
   * If these repeated comparisons showed up as being a hot spot in a profile,
   * there are several ways this dispatch could be optimized. */
  static struct upb_string set = {.ptr = "set_", .byte_len = 4};
  static struct upb_string has = {.ptr = "has_", .byte_len = 4};
  static struct upb_string clear = {.ptr = "clear_", .byte_len = 6};
  static struct upb_string size = {.ptr = "_size", .byte_len = 5};
  static struct upb_string mutable = {.ptr = "mutable_", .byte_len = 8};
  static struct upb_string add = {.ptr = "add_", .byte_len = 4};
  static struct upb_string list = {.ptr = "_list", .byte_len = 5};

  struct upb_string str;
  Py_ssize_t len;
  PyString_AsStringAndSize(attr_name, &str.ptr, &len);
  if(len > UINT32_MAX) {
    PyErr_SetString(PyExc_TypeError,
                    "Wow, that's a long attribute name you've got there.");
    return NULL;
  }
  str.byte_len = (uint32_t)len;
  PyUpb_PbMsg *msgobj = Check_Message(obj);
  struct upb_msgdef *def = msgobj->ref.p.msg->def;

  /* This can be a field reference iff the first letter is lowercase, because
   * generic methods (eg. IsInitialized()) all start with uppercase. */
  if(islower(str.ptr[0])) {
    PyUpb_PbBoundFieldOpCode opcode;
    struct upb_string field_name;
    if(starts_with(&str, &has, &field_name))
      opcode = OP_HAS;
    else if(starts_with(&str, &set, &field_name))
      opcode = OP_SET;
    else if(starts_with(&str, &mutable, &field_name))
      opcode = OP_MUTABLE;
    else if(ends_with(&str, &size, &field_name))
      opcode = OP_SIZE;
    else if(ends_with(&str, &list, &field_name))
      opcode = OP_LIST;
    else if(starts_with(&str, &add, &field_name))
      opcode = OP_ADD;
    else if(starts_with(&str, &clear, &field_name))
      opcode = OP_CLEAR;
    else {
      /* Could be a plain field reference (eg. obj.field(i)). */
      opcode = OP_GET;
      field_name = str;
    }
    struct upb_msg_fielddef *f = upb_msg_fieldbyname(def, &field_name);
    if(f) {
      PyObject *op = PyUpb_NewPbBoundFieldOp(msgobj, f, opcode);
      if(op) return op;
    }
  }

  /* Fall back on regular attribute lookup. */
  return PyObject_GenericGetAttr(obj, attr_name);
}

static void msg_dealloc(PyObject *obj)
{
  PyUpb_PbMsg *msg = (void*)obj;
  upb_mm_release(&msg->ref);
  Py_DECREF(msg->def);
  obj->ob_type->tp_free(obj);
}

static PyTypeObject PyUpb_PbMsgType = {
  PyObject_HEAD_INIT(NULL)
  0,                                      /* ob_size */
  "upb.pb.Message",                       /* tp_name */
  sizeof(PyUpb_PbMsg),                    /* tp_basicsize */
  0,                                      /* tp_itemsize */
  msg_dealloc,                            /* tp_dealloc */
  0,                                      /* tp_print */
  0,                                      /* tp_getattr */
  0,                                      /* tp_setattr */
  0,                                      /* tp_compare */
  0,                                      /* tp_repr (TODO) */
  0,                                      /* tp_as_number */
  0,                                      /* tp_as_sequence */
  0,                                      /* tp_as_mapping */
  0,                                      /* tp_hash */
  0,                                      /* tp_call */
  0,                                      /* tp_str */
  msg_getattro,                           /* tp_getattro */
  0, /* Not allowed. */                   /* tp_setattro */
  0,                                      /* tp_as_buffer */
  Py_TPFLAGS_DEFAULT,                     /* tp_flags */
  0,                                      /* tp_doc */
  0,                                      /* tp_traverse (TODO) */
  0,                                      /* tp_clear (TODO) */
  0,                                      /* tp_richcompare */
  0,                                      /* tp_weaklistoffset */
  0,                                      /* tp_iter */
  0,                                      /* tp_iternext */
  msg_methods,                            /* tp_methods */
  0,                                      /* tp_members */
  0,                                      /* tp_getset */
  0,                                      /* tp_base */
  0,                                      /* tp_dict */
  0,                                      /* tp_descr_get */
  0,                                      /* tp_descr_set */
  0,                                      /* tp_dictoffset */
  0,                                      /* tp_init */
  0,                                      /* tp_alloc */
  0, /* Can't be created from Python. */  /* tp_new */
  0,                                      /* tp_free */
};

/* upb.pb.MessageCreator ******************************************************/

static PyObject *creator_call(PyObject *callable, PyObject *args, PyObject *kw)
{
  PyUpb_PbMsgCreator *creator = Check_MsgCreator(callable);
  return MMREF_TO_PYOBJ(upb_mm_newmsg_ref(creator->def->def, &pymm));
}

static PyObject *creator_repr(PyObject *obj)
{
  PyUpb_PbMsgCreator *creator = Check_MsgCreator(obj);
  struct upb_string *name = creator->def->def->descriptor->name;
  /* Need to get a NULL-terminated copy of name since PyString_FromFormat
   * doesn't support ptr+len. */
  PyObject *nameobj = PyString_FromStringAndSize(name->ptr, name->byte_len);
  PyObject *ret = PyString_FromFormat("<upb.pb.MessageCreator for '%s'>",
                                      PyString_AS_STRING(nameobj));
  Py_DECREF(nameobj);
  return ret;
}

static void creator_dealloc(PyObject *obj)
{
  PyUpb_PbMsgCreator *creator = (void*)obj;
  Py_DECREF(creator->def);
  obj->ob_type->tp_free(obj);
}

static PyObject *creator_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
  PyUpb_PbMsgCreator *creator = (void*)type->tp_alloc(type, 0);
  PyUpb_MsgDef *def;
  if(!PyArg_ParseTuple(args, "O!", &PyUpb_MsgDefType, &def)) return NULL;
  creator->def = def;
  Py_INCREF(creator->def);
  return (PyObject*)creator;
}

static PyTypeObject PyUpb_PbMsgCreatorType = {
  PyObject_HEAD_INIT(NULL)
  0,                                      /* ob_size */
  "upb.pb.MessageCreator",                /* tp_name */
  sizeof(PyUpb_PbMsgCreator),             /* tp_basicsize */
  0,                                      /* tp_itemsize */
  creator_dealloc,                        /* tp_dealloc */
  0,                                      /* tp_print */
  0,                                      /* tp_getattr */
  0,                                      /* tp_setattr */
  0,                                      /* tp_compare */
  creator_repr,                           /* tp_repr */
  0,                                      /* tp_as_number */
  0,                                      /* tp_as_sequence */
  0,                                      /* tp_as_mapping */
  0,                                      /* tp_hash */
  creator_call,                           /* tp_call */
  0,                                      /* tp_str */
  0,                                      /* tp_getattro */
  0,                                      /* tp_setattro */
  0,                                      /* tp_as_buffer */
  Py_TPFLAGS_DEFAULT,                     /* tp_flags */
  0,                                      /* tp_doc */
  0,                                      /* tp_traverse */
  0,                                      /* tp_clear */
  0,                                      /* tp_richcompare */
  0,                                      /* tp_weaklistoffset */
  0,                                      /* tp_iter */
  0,                                      /* tp_iternext */
  0,                                      /* tp_methods */
  0,                                      /* tp_members */
  0,                                      /* tp_getset */
  0,                                      /* tp_base */
  0,                                      /* tp_dict */
  0,                                      /* tp_descr_get */
  0,                                      /* tp_descr_set */
  0,                                      /* tp_dictoffset */
  0,                                      /* tp_init */
  0,                                      /* tp_alloc */
  creator_new,                            /* tp_new */
  0,                                      /* tp_free */
};

/* upb.pb module **************************************************************/

static PyMethodDef methods[] = {
  {NULL, NULL}
};

PyMODINIT_FUNC
initpb(void)
{
  if(PyType_Ready(&PyUpb_PbBoundFieldOpType) < 0) return;
  if(PyType_Ready(&PyUpb_PbMsgType) < 0) return;
  if(PyType_Ready(&PyUpb_PbMsgCreatorType) < 0) return;

  /* PyModule_AddObject steals a reference.  These objects are statically
   * allocated and must not be deleted, so we increment their refcount. */
  Py_INCREF(&PyUpb_PbBoundFieldOpType);
  Py_INCREF(&PyUpb_PbMsgType);
  Py_INCREF(&PyUpb_PbMsgCreatorType);

  PyObject *mod = Py_InitModule("upb.cext.pb", methods);
  PyModule_AddObject(mod, "BoundFieldOp", (PyObject*)&PyUpb_PbBoundFieldOpType);
  PyModule_AddObject(mod, "Message", (PyObject*)&PyUpb_PbMsgType);
  PyModule_AddObject(mod, "MessageCreator", (PyObject*)&PyUpb_PbMsgCreatorType);
}
