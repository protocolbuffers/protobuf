//
// upb - a minimalist implementation of protocol buffers.
//
// Copyright (c) 2011 Google Inc.  See LICENSE for details.
// Author: Josh Haberman <jhaberman@gmail.com>
// Routines for reading and writing message data to an in-memory structure,
// similar to a C struct.
//
// upb does not define one single message object that everyone must use.
// Rather it defines an abstract interface for reading and writing members
// of a message object, and all of the parsers and serializers use this
// abstract interface.  This allows upb's parsers and serializers to be used
// regardless of what memory management scheme or synchronization model the
// application is using.
//
// A standard set of accessors is provided for doing simple reads and writes at
// a known offset into the message.  These accessors should be used when
// possible, because they are specially optimized -- for example, the JIT can
// recognize them and emit specialized code instead of having to call the
// function at all.  The application can substitute its own accessors when the
// standard accessors are not suitable.

#ifndef UPB_MSG_HPP
#define UPB_MSG_HPP

#include "upb/msg.h"
#include "upb/handlers.hpp"

namespace upb {

typedef upb_accessor_vtbl AccessorVTable;

// Registers handlers for writing into a message of the given type using
// whatever accessors it has defined.
inline MessageHandlers* RegisterWriteHandlers(upb::Handlers* handlers,
                                              const upb::MessageDef* md) {
  return MessageHandlers::Cast(
      upb_accessors_reghandlers(handlers, md));
}

template <typename T> static FieldHandlers::ValueHandler* GetValueHandler();

// A handy templated function that will retrieve a value handler for a given
// C++ type.
#define GET_VALUE_HANDLER(type, ctype) \
    template <> \
    FieldHandlers::ValueHandler* GetValueHandler<ctype>() { \
      return &upb_stdmsg_set ## type; \
    }

GET_VALUE_HANDLER(double, double);
GET_VALUE_HANDLER(float, float);
GET_VALUE_HANDLER(uint64, uint64_t);
GET_VALUE_HANDLER(uint32, uint32_t);
GET_VALUE_HANDLER(int64, int64_t);
GET_VALUE_HANDLER(int32, int32_t);
GET_VALUE_HANDLER(bool, bool);
#undef GET_VALUE_HANDLER

}  // namespace

#endif
