// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "hphp/runtime/vm/native-prop-handler.h"

#include "protobuf_hhvm.h"
#include "upb.h"

// -----------------------------------------------------------------------------
// Define static methods
// -----------------------------------------------------------------------------

upb_msgval tomsgval(const Variant& value, upb_fieldtype_t type) {
  switch (type) {
    case UPB_TYPE_INT32:
    case UPB_TYPE_ENUM:
      return upb_msgval_int32(value.toInt32());
    case UPB_TYPE_INT64:
      return upb_msgval_int64(value.toInt64());
    case UPB_TYPE_UINT32:
      return upb_msgval_uint32(value.toInt32());
    case UPB_TYPE_UINT64:
      return upb_msgval_uint64(value.toInt64());
    case UPB_TYPE_DOUBLE:
      return upb_msgval_double(value.toDouble());
    case UPB_TYPE_FLOAT:
      return upb_msgval_float(value.toDouble());
    case UPB_TYPE_BOOL:
      return upb_msgval_bool(value.toBoolean());
    case UPB_TYPE_STRING:
    case UPB_TYPE_BYTES: {
      const String& str = value.toCStrRef();
      return upb_msgval_makestr(str.data(), str.size());
    }
    case UPB_TYPE_MESSAGE: {
      Object obj = value.toObject();
      const Message* message = Native::data<Message>(obj);
      return upb_msgval_msg(message->msg);
    }
    default:
      break;
  }
  UPB_UNREACHABLE();
}

Variant tophpval(const upb_msgval& msgval,
                 upb_fieldtype_t type,
                 Class* klass) {
  switch (type) {
    case UPB_TYPE_INT32:
    case UPB_TYPE_ENUM:
      return Variant(upb_msgval_getint32(msgval));
    case UPB_TYPE_INT64:
      return Variant(upb_msgval_getint64(msgval));
    case UPB_TYPE_UINT32:
      return Variant(static_cast<int32_t>(upb_msgval_getuint32(msgval)));
    case UPB_TYPE_UINT64:
      return Variant(static_cast<int64_t>(upb_msgval_getuint64(msgval)));
    case UPB_TYPE_DOUBLE:
      return Variant(upb_msgval_getdouble(msgval));
    case UPB_TYPE_FLOAT:
      return Variant(upb_msgval_getfloat(msgval));
    case UPB_TYPE_BOOL:
      return Variant(upb_msgval_getbool(msgval));
    case UPB_TYPE_STRING:
    case UPB_TYPE_BYTES: {
      upb_stringview str = upb_msgval_getstr(msgval);
      if (str.size == 0) {
        return String("", 0, CopyString);
      } else {
        return String(str.data, str.size, CopyString);
      }
    }
    case UPB_TYPE_MESSAGE: {
      const upb_msg *msg = upb_msgval_getmsg(msgval);
      if (msg == NULL) return Variant(Variant::NullInit{});
      Object message = Object(klass);
      Message* intern = Native::data<Message>(message);
      const upb_msgdef* subdef = class2msgdef(klass);
      Message_wrap(intern, const_cast<upb_msg*>(msg), subdef);
      return message;
    }
    default:
      break;
  }

  return uninit_null();;
}

static Variant Message_get(const Object& obj, const String& name) {
  Message* self = Native::data<Message>(obj);
  const upb_fielddef* f = upb_msgdef_ntof(
      self->msgdef, name.data(), name.size());
  assert(f != NULL);
  int field_index = upb_fielddef_index(f);
  upb_fieldtype_t type = upb_fielddef_type(f);

  upb_msgval msgval = upb_msg_get(self->msg, field_index, self->layout);

  if (upb_fielddef_isseq(f)) {
    Object array = Object(Unit::loadClass(s_RepeatedField.get()));
    RepeatedField *intern = Native::data<RepeatedField>(array);

    Class* klass = NULL;
    if (upb_fielddef_issubmsg(f)) {
      const upb_msgdef *subdef = upb_fielddef_msgsubdef(f);
      klass = const_cast<Class*>(msgdef2class(subdef));
    }

    const upb_array *arr = upb_msgval_getarr(msgval);
    if (arr == NULL) {
      RepeatedField___construct(intern, upb_fielddef_descriptortype(f), klass);
      upb_msg_set(self->msg, field_index,
                  upb_msgval_arr(intern->array), self->layout);
    } else {
      RepeatedField_wrap(intern, const_cast<upb_array*>(arr), klass);
    }

    return array;
  } else if (upb_fielddef_ismap(f)) {
  } else {
    Class* klass = NULL;
    if (upb_fielddef_issubmsg(f)) {
      const upb_msgdef *subdef = upb_fielddef_msgsubdef(f);
      klass = const_cast<Class*>(msgdef2class(subdef));
    }
    return tophpval(msgval, type, klass);
  }
}
static Variant Message_set(const Object& obj, const String& name,
                           const Variant& value) {
  Message* self = Native::data<Message>(obj);
  const upb_fielddef* f = upb_msgdef_ntof(
      self->msgdef, name.data(), name.size());
  assert(f != NULL);
  int field_index = upb_fielddef_index(f);
  upb_fieldtype_t type = upb_fielddef_type(f);

  if (upb_fielddef_isseq(f)) {
    RepeatedField *arr = Native::data<RepeatedField>(value.toObject());
    upb_msgval msgval;
    upb_msgval_setarr(&msgval, arr->array);
    upb_msg_set(self->msg, field_index, msgval, self->layout);
  } else if (upb_fielddef_ismap(f)) {
    MapField *map = Native::data<MapField>(value.toObject());
    upb_msgval msgval;
    upb_msgval_setmap(&msgval, map->map);
    upb_msg_set(self->msg, field_index, msgval, self->layout);
  } else {
    upb_msgval msgval = tomsgval(value, type);
    upb_msg_set(self->msg, field_index, msgval, self->layout);
  }

  return uninit_null();;
}
static Variant Message_isset(const Object& obj, const String& name) {
  return uninit_null();;
}
static Variant Message_unset(const Object& obj, const String& name) {
  return uninit_null();;
}

// -----------------------------------------------------------------------------
// Message
// -----------------------------------------------------------------------------

void HHVM_METHOD(Message, __construct);
String HHVM_METHOD(Message, serializeToString);
void HHVM_METHOD(Message, mergeFromString, const String& data);

const StaticString s_Message("Google\\Protobuf\\Internal\\Message");

void Message_init() {
  // Register methods
  HHVM_NAMED_ME(Google\\Protobuf\\Internal\\Message,
                __construct, HHVM_MN(Message, __construct));
  HHVM_NAMED_ME(Google\\Protobuf\\Internal\\Message,
                serializeToString, HHVM_MN(Message, serializeToString));
  HHVM_NAMED_ME(Google\\Protobuf\\Internal\\Message,
                mergeFromString, HHVM_MN(Message, mergeFromString));

  // Register class
  Native::registerNativeDataInfo<Message>(s_Message.get()); 

  // Register handlers
  Native::registerNativePropHandler(s_Message,
                            &Message_get,
                            &Message_set,
                            &Message_isset,
                            &Message_unset);
}

void HHVM_METHOD(Message, __construct) {
  Message* intern = Native::data<Message>(this_);
  Class* msg_class = this_->getVMClass();
  const upb_msgdef* msgdef = class2msgdef(msg_class);
  Message___construct(intern, msgdef);
}

String HHVM_METHOD(Message, serializeToString) {
  Message* intern = Native::data<Message>(this_);
  size_t size;
  const char* data = Message_serializeToString(intern, &size);
  return String(data, size, CopyString);
}

void HHVM_METHOD(Message, mergeFromString, const String& data) {
  Message* intern = Native::data<Message>(this_);
  Message_mergeFromString(intern, data.c_str(), data.length());
}
