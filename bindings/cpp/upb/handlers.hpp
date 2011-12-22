//
// upb - a minimalist implementation of protocol buffers.
//
// Copyright (c) 2011 Google Inc.  See LICENSE for details.
// Author: Josh Haberman <jhaberman@gmail.com>
//
// upb::Handlers is a generic visitor-like interface for iterating over a
// stream of protobuf data.  You can register function pointers that will be
// called for each message and/or field as the data is being parsed or iterated
// over, without having to know the source format that we are parsing from.
// This decouples the parsing logic from the processing logic.

#ifndef UPB_HANDLERS_HPP
#define UPB_HANDLERS_HPP

#include "upb/handlers.h"

namespace upb {

typedef upb_fieldtype_t FieldType;
typedef upb_flow_t Flow;
class MessageHandlers;

class FieldHandlers : public upb_fhandlers {
 public:
  typedef upb_value_handler ValueHandler;
  typedef upb_startfield_handler StartFieldHandler;
  typedef upb_endfield_handler EndFieldHandler;

  // The FieldHandlers will live at least as long as the upb::Handlers to
  // which it belongs, but can be Ref'd/Unref'd to make it live longer (which
  // will prolong the life of the underlying upb::Handlers also).
  void Ref()   { upb_fhandlers_ref(this); }
  void Unref() { upb_fhandlers_unref(this); }

  // Functions to set this field's handlers.
  // These return "this" so they can be conveniently chained, eg.
  //   message_handlers->NewField(...)
  //       ->SetStartSequenceHandler(&StartSequence),
  //       ->SetEndSequenceHandler(&EndSequence),
  //       ->SetValueHandler(&Value);
  FieldHandlers* SetValueHandler(ValueHandler* h) {
    upb_fhandlers_setvalue(this, h); return this;
  }
  FieldHandlers* SetStartSequenceHandler(StartFieldHandler* h) {
    upb_fhandlers_setstartseq(this, h); return this;
  }
  FieldHandlers* SetEndSequenceHandler(EndFieldHandler* h) {
    upb_fhandlers_setendseq(this, h); return this;
  }
  FieldHandlers* SetStartSubmessageHandler(StartFieldHandler* h) {
    upb_fhandlers_setstartsubmsg(this, h); return this;
  }
  FieldHandlers* SetEndSubmessageHandler(EndFieldHandler* h) {
    upb_fhandlers_setendsubmsg(this, h); return this;
  }

  // Get/Set the field's bound value, which will be passed to its handlers.
  Value GetBoundValue() const { return upb_fhandlers_getfval(this); }
  FieldHandlers* SetBoundValue(Value val) {
    upb_fhandlers_setfval(this, val); return this;
  }

  // Returns the MessageHandlers to which we belong.
  MessageHandlers* GetMessageHandlers() const;
  // Returns the MessageHandlers for this field's submessage (invalid to call
  // unless this field's type UPB_TYPE(MESSAGE) or UPB_TYPE(GROUP).
  MessageHandlers* GetSubMessageHandlers() const;
  // If set to >=0, the given hasbit will be set after the value callback is
  // called (offset relative to the current closure).
  int32_t GetValueHasbit() const { return upb_fhandlers_getvaluehasbit(this); }
  void SetValueHasbit(int32_t bit) { upb_fhandlers_setvaluehasbit(this, bit); }

 private:
  FieldHandlers();  // Only created by upb::Handlers.
  ~FieldHandlers(); // Only destroyed by refcounting.
};

class MessageHandlers : public upb_mhandlers {
 public:
  typedef upb_startmsg_handler StartMessageHandler;
  typedef upb_endmsg_handler EndMessageHandler;

  // The MessageHandlers will live at least as long as the upb::Handlers to
  // which it belongs, but can be Ref'd/Unref'd to make it live longer (which
  // will prolong the life of the underlying upb::Handlers also).
  void Ref()    { upb_mhandlers_ref(this); }
  void Unref()  { upb_mhandlers_unref(this); }

  // Functions to set this message's handlers.
  // These return "this" so they can be conveniently chained, eg.
  //   handlers->NewMessage()
  //       ->SetStartMessageHandler(&StartMessage)
  //       ->SetEndMessageHandler(&EndMessage);
  MessageHandlers* SetStartMessageHandler(StartMessageHandler* h) {
    upb_mhandlers_setstartmsg(this, h); return this;
  }
  MessageHandlers* SetEndMessageHandler(EndMessageHandler* h) {
    upb_mhandlers_setendmsg(this, h); return this;
  }

  // Functions to create new FieldHandlers for this message.
  FieldHandlers* NewFieldHandlers(uint32_t fieldnum, FieldType type,
                                  bool repeated) {
    return static_cast<FieldHandlers*>(
        upb_mhandlers_newfhandlers(this, fieldnum, type, repeated));
  }

  // Like the previous but for MESSAGE or GROUP fields.  For GROUP fields, the
  // given submessage must not have any fields with this field number.
  FieldHandlers* NewFieldHandlersForSubmessage(uint32_t n, const char *name,
                                               FieldType type, bool repeated,
                                               MessageHandlers* subm) {
    return static_cast<FieldHandlers*>(
        upb_mhandlers_newfhandlers_subm(this, n, type, repeated, subm));
  }

 private:
  MessageHandlers();  // Only created by upb::Handlers.
  ~MessageHandlers(); // Only destroyed by refcounting.
};

class Handlers : public upb_handlers {
 public:
  // Creates a new Handlers instance.
  static Handlers* New() { return static_cast<Handlers*>(upb_handlers_new()); }

  void Ref()   { upb_handlers_ref(this); }
  void Unref() { upb_handlers_unref(this); }

  // Returns a new MessageHandlers object.  The first such message that is
  // obtained will be the top-level message for this Handlers object.
  MessageHandlers* NewMessageHandlers() {
    return static_cast<MessageHandlers*>(upb_handlers_newmhandlers(this));
  }

 private:
  Handlers();  // Only created by Handlers::New().
  ~Handlers(); // Only destroyed by refcounting.
};


MessageHandlers* FieldHandlers::GetMessageHandlers() const {
  return static_cast<MessageHandlers*>(upb_fhandlers_getmsg(this));
}

MessageHandlers* FieldHandlers::GetSubMessageHandlers() const {
  return static_cast<MessageHandlers*>(upb_fhandlers_getsubmsg(this));
}

}  // namespace upb

#endif
