// Protocol Buffers - Google's data interchange format
// Copyright 2015 Google Inc.  All rights reserved.
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

#include <nan.h>
#include <string>
#include <sstream>
#include <math.h>
#include <stdlib.h>
#include <errno.h>
#include "int64.h"
#include "util.h"

using namespace v8;
using namespace node;

namespace protobuf_js {

Persistent<Function> Int64::constructor_signed;
Persistent<Value> Int64::prototype_signed;
Persistent<Function> Int64::constructor_unsigned;
Persistent<Value> Int64::prototype_unsigned;

JS_OBJECT_INIT(Int64);

Handle<Function> Int64::MakeInt64Class(std::string name, bool is_signed) {
  NanEscapableScope();
  Local<FunctionTemplate> tpl = NanNew<FunctionTemplate>(New);
  tpl->ReadOnlyPrototype();
  tpl->SetClassName(NanNew<String>(name.c_str()));
  tpl->InstanceTemplate()->SetInternalFieldCount(JS_OBJECT_WRAP_SLOTS);

  tpl->PrototypeTemplate()->Set(
      NanNew<String>("toString"),
      NanNew<FunctionTemplate>(ToString)->GetFunction());

  Local<Function> f = tpl->GetFunction();
  f->Set(
      NanNew<String>("hi"),
      NanNew<FunctionTemplate>(Hi)->GetFunction());
  f->Set(
      NanNew<String>("lo"),
      NanNew<FunctionTemplate>(Lo)->GetFunction());
  f->Set(
      NanNew<String>("join"),
      NanNew<FunctionTemplate>(Join, is_signed ? NanTrue() : NanFalse())->
      GetFunction());
  f->Set(
      NanNew<String>("compare"),
      NanNew<FunctionTemplate>(Compare)->GetFunction());

  return NanEscapeScope(f);
}

void Int64::Init(Handle<Object> exports) {
  NanEscapableScope();

  NanAssignPersistent(constructor_signed,
                      MakeInt64Class("Int64", true));
  NanAssignPersistent(prototype_signed,
                      NanNew(constructor_signed)->NewInstance(0, NULL)->GetPrototype());
  exports->Set(NanNew<String>("Int64"), NanNew(constructor_signed));

  NanAssignPersistent(constructor_unsigned,
                      MakeInt64Class("UInt64", false));
  NanAssignPersistent(prototype_unsigned,
                      NanNew(constructor_unsigned)->NewInstance(0, NULL)->GetPrototype());
  exports->Set(NanNew<String>("UInt64"), NanNew(constructor_unsigned));
}

NAN_METHOD(Int64::New) {
  NanEscapableScope();
  if (args.IsConstructCall()) {
    Int64* self = new Int64();
    self->Wrap<Int64>(args.This());
    if (!self->HandleCtorArgs(args)) {
      NanReturnUndefined();
    }
    NanReturnValue(args.This());
  } else {
    NanThrowError("Must be called as constructor");
    NanReturnUndefined();
  }
}

bool Int64::HandleCtorArgs(_NAN_METHOD_ARGS_TYPE args) {
  if (args.Length() == 0) {
    return true;
  } else if (args.Length() == 1 || args.Length() == 2) {
    return DoSet(args);
  } else {
    NanThrowError("Incorrect number of arguments to Int64/UInt64 constructor");
    return false;
  }
}

bool Int64::DoSet(_NAN_METHOD_ARGS_TYPE args) {
  NanEscapableScope();
  bool is_signed = IsSigned(args.This());
  if (args.Length() == 1) {
    if (args[0]->IsObject() &&
        args[0]->ToObject()->GetPrototype() ==
        Int64::prototype_unsigned) {
      Int64* other = Int64::unwrap(args[0]);
      uint64_t other_val = other->uint64_value();
      if (is_signed) {
        if (other_val > INT64_MAX) {
          NanThrowError("Value out of range");
          return false;
        }
        value_.s = other_val;
      } else {
        value_.u = other_val;
      }
    } else if (args[0]->IsObject() &&
               args[0]->ToObject()->GetPrototype() ==
               Int64::prototype_signed) {
      Int64* other = Int64::unwrap(args[0]);
      int64_t other_val = other->int64_value();
      if (!is_signed) {
        if (other_val < 0) {
          NanThrowError("Value out of range");
          return false;
        }
        value_.u = other_val;
      } else {
        value_.s = other_val;
      }
    } else if (args[0]->IsString()) {
      String::Utf8Value utf8(args[0]->ToString());
      if (is_signed) {
        char* end;
        // Clear errno, else we cannot distinguish between a legal parse of a
        // max/min value and an overflow.
        errno = 0;
        value_.s = strtoll(*utf8, &end, 10);
        if (end == *utf8 || *end != '\0' ||
            ((value_.s == LLONG_MIN || value_.s == LLONG_MAX) &&
             errno == ERANGE)) {
          NanThrowError("Value out of range");
          return false;
        }
      } else {
        char* end;
        // Clear errno, else we cannot distinguish between a legal parse of a
        // max/min value and an overflow.
        errno = 0;
        value_.u = strtoull(*utf8, &end, 10);
        // Explicitly check for a leading '-' -- otherwise, strtoull() seems to
        // just happily give us the unsigned-casted version of the
        // twos-complement without signalling an error.
        if (**utf8 == '-' || end == *utf8 || *end != '\0' ||
            (value_.u == ULLONG_MAX && errno == ERANGE)) {
          NanThrowError("Value out of range");
          return false;
        }
      }
    } else if (args[0]->IsInt32()) {
      if (is_signed) {
        value_.s = args[0]->Int32Value();
      } else {
        if (args[0]->Int32Value() < 0) {
          NanThrowError("Assigning negative value to UInt64 instance");
          return false;
        }
        value_.u = args[0]->Int32Value();
      }
    } else if (args[0]->IsUint32()) {
      if (is_signed) {
        value_.s = args[0]->Uint32Value();
      } else {
        value_.u = args[0]->Uint32Value();
      }
    } else if (args[0]->IsNumber()) {
      double value = args[0]->ToNumber()->Value();
      if (floor(value) != value) {
        NanThrowError("Assigning non-integer value to Int64/UInt64 instance");
        return false;
      }
      if (is_signed) {
        if (value > INT64_MAX) {
          NanThrowError("Assigning too-large value to Int64");
          return false;
        } else if (value < INT64_MIN) {
          NanThrowError("Assigning too-small value to Int64");
          return false;
        }
        value_.s = value;
      } else {
        if (value > UINT64_MAX) {
          NanThrowError("Assigning too-large value to UInt64");
          return false;
        } else if (value < 0) {
          NanThrowError("Assigning too-small value to UInt64");
          return false;
        }
        value_.u = value;
      }
    } else {
      NanThrowError("Unsupported type for assignment to Int64/UInt64");
      return false;
    }
  } else {
    NanThrowError("Wrong number of arguments to Int64/UInt64 assignment");
    return false;
  }

  return true;
}

NAN_METHOD(Int64::Hi) {
  NanEscapableScope();
  if (args.Length() != 1) {
    NanThrowError("Wrong number of arguments");
    NanReturnUndefined();
  }
  Int64* self = Int64::unwrap(args[0]);
  if (!self) {
    NanReturnUndefined();
  }

  bool is_signed = IsSigned(args.This());
  uint64_t raw = is_signed ? static_cast<uint64_t>(self->value_.s) : self->value_.u;
  NanReturnValue(NanNewUInt32(raw >> 32));
}

NAN_METHOD(Int64::Lo) {
  NanEscapableScope();
  if (args.Length() != 1) {
    NanThrowError("Wrong number of arguments");
    NanReturnUndefined();
  }
  Int64* self = Int64::unwrap(args[0]);
  if (!self) {
    NanReturnUndefined();
  }

  bool is_signed = IsSigned(args.This());
  uint64_t raw = is_signed ? static_cast<uint64_t>(self->value_.s) : self->value_.u;
  NanReturnValue(NanNewUInt32(raw & 0xffffffffU));
}

NAN_METHOD(Int64::ToString) {
  NanEscapableScope();
  Int64* self = Int64::unwrap(args.This());
  if (!self) {
    NanReturnUndefined();
  }
  bool is_signed = IsSigned(args.This());
  std::ostringstream os;
  if (is_signed) {
    os << self->value_.s;
    NanReturnValue(NanNew<String>(os.str().c_str()));
  } else {
    os << self->value_.u;
    NanReturnValue(NanNew<String>(os.str().c_str()));
  }
}

NAN_METHOD(Int64::Join) {
  bool is_signed = args.Data()->BooleanValue();
  NanEscapableScope();
  if (args.Length() != 2) {
    NanThrowError("Wrong number of arguments");
    NanReturnUndefined();
  }
  if (!args[0]->IsNumber() || !args[1]->IsNumber()) {
    NanThrowError("Invalid argument types: expected two numbers");
    NanReturnUndefined();
  }
  double hi_val = args[0]->NumberValue();
  double lo_val = args[1]->NumberValue();

  if (floor(hi_val) != hi_val || floor(lo_val) != lo_val) {
    NanThrowError("Invalid argument: expected integral value");
    NanReturnUndefined();
  }
  if (hi_val > UINT32_MAX || hi_val < 0 ||
      lo_val > UINT32_MAX || lo_val < 0) {
    NanThrowError("Invalid argument: out of range (expected uint32 values)");
    NanReturnUndefined();
  }

  uint32_t hi_int = static_cast<uint32_t>(hi_val);
  uint32_t lo_int = static_cast<uint32_t>(lo_val);
  uint64_t value =
      (static_cast<uint64_t>(hi_int) << 32) |
      static_cast<uint64_t>(lo_int);

  Local<Function> obj_ctor =
      NanNew(is_signed ? Int64::constructor_signed :
                         Int64::constructor_unsigned);
  Local<Object> obj = obj_ctor->NewInstance(0, NULL);
  Int64* i64 = Int64::unwrap(obj);
  if (!i64) {
    NanReturnUndefined();
  }

  if (i64->IsSigned()) {
    i64->set_int64_value(static_cast<int64_t>(value));
  } else {
    i64->set_uint64_value(static_cast<uint64_t>(value));
  }

  NanReturnValue(obj);
}

NAN_METHOD(Int64::Compare) {
  NanEscapableScope();
  if (args.Length() != 2) {
    NanThrowError("Wrong number of arguments");
    NanReturnUndefined();
  }

  Int64* lhs = Int64::unwrap(args[0]);
  Int64* rhs = Int64::unwrap(args[1]);
  if (!lhs || !rhs) {
    NanReturnUndefined();
  }

  // Carefully compare while respecting signedness.
  int comparison = 0;
  if (!lhs->IsSigned() && !rhs->IsSigned()) {
    uint64_t lhs_val = lhs->uint64_value();
    uint64_t rhs_val = rhs->uint64_value();
    comparison = (lhs_val < rhs_val) ? -1 :
                 (lhs_val > rhs_val) ?  1 :
                 0;
  } else if (lhs->IsSigned() && rhs->IsSigned()) {
    int64_t lhs_val = lhs->uint64_value();
    int64_t rhs_val = rhs->uint64_value();
    comparison = (lhs_val < rhs_val) ? -1 :
                 (lhs_val > rhs_val) ?  1 :
                 0;
  } else if (lhs->IsSigned() && !rhs->IsSigned()) {
    int64_t lhs_val = lhs->uint64_value();
    uint64_t rhs_val = rhs->uint64_value();
    if (lhs_val < 0) {
      comparison = -1;
    } else {
      uint64_t lhs_unsigned = static_cast<uint64_t>(lhs_val);
      comparison = (lhs_unsigned < rhs_val) ? -1 :
                   (lhs_unsigned > rhs_val) ?  1 :
                   0;
    }
  } else /* !lhs->IsSigned() && rhs->IsSigned() */ {
    uint64_t lhs_val = lhs->uint64_value();
    int64_t rhs_val = rhs->uint64_value();
    if (rhs_val < 0) {
      comparison = 1;
    } else {
      uint64_t rhs_unsigned = static_cast<uint64_t>(rhs_val);
      comparison = (lhs_val < rhs_unsigned) ? -1 :
                   (lhs_val > rhs_unsigned) ?  1 :
                   0;
    }
  }

  NanReturnValue(NanNew<Integer>(comparison));
}

}  // namespace protobuf_js
