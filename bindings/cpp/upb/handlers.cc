//
// upb - a minimalist implementation of protocol buffers.
//
// Copyright (c) 2011 Google Inc.  See LICENSE for details.
// Author: Josh Haberman <jhaberman@gmail.com>

#include "handlers.hpp"

#include "def.hpp"

namespace upb {

namespace {

void MessageCallbackWrapper(
    void* closure, upb_mhandlers* mh, const upb_msgdef* m) {
  Handlers::MessageRegistrationVisitor* visitor =
      static_cast<Handlers::MessageRegistrationVisitor*>(closure);
  visitor->OnMessage(static_cast<MessageHandlers*>(mh),
                     static_cast<const MessageDef*>(m));
}

void FieldCallbackWrapper(
    void* closure, upb_fhandlers* fh, const upb_fielddef* f) {
  Handlers::MessageRegistrationVisitor* visitor =
      static_cast<Handlers::MessageRegistrationVisitor*>(closure);
  visitor->OnField(static_cast<FieldHandlers*>(fh),
                   static_cast<const FieldDef*>(f));
}
}  // namepace

MessageHandlers* Handlers::RegisterMessageDef(
    const MessageDef& m, Handlers::MessageRegistrationVisitor* visitor) {
  upb_mhandlers* mh = upb_handlers_regmsgdef(
      this, &m, &MessageCallbackWrapper, &FieldCallbackWrapper, &visitor);
  return static_cast<MessageHandlers*>(mh);
}

}  // namespace upb
