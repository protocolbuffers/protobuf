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

#include "upb/upb.hpp"

namespace upb {

typedef upb_fieldtype_t FieldType;
typedef upb_flow_t Flow;
typedef upb_sflow_t SubFlow;
class MessageHandlers;
class MessageDef;
class FieldDef;

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
  int32_t GetHasbit() const { return upb_fhandlers_gethasbit(this); }
  void SetHasbit(int32_t bit) { upb_fhandlers_sethasbit(this, bit); }

 private:
  UPB_DISALLOW_CONSTRUCT_AND_DESTRUCT(FieldHandlers);
};

class MessageHandlers : public upb_mhandlers {
 public:
  typedef upb_startmsg_handler StartMessageHandler;
  typedef upb_endmsg_handler EndMessageHandler;

  static MessageHandlers* Cast(upb_mhandlers* mh) {
    return static_cast<MessageHandlers*>(mh);
  }
  static const MessageHandlers* Cast(const upb_mhandlers* mh) {
    return static_cast<const MessageHandlers*>(mh);
  }

  // The MessageHandlers will live at least as long as the upb::Handlers to
  // which it belongs, but can be Ref'd/Unref'd to make it live longer (which
  // will prolong the life of the underlying upb::Handlers also).
  void Ref()    { upb_mhandlers_ref(this); }
  void Unref()  { upb_mhandlers_unref(this); }

  // Functions to set this message's handlers.
  // These return "this" so they can be conveniently chained, eg.
  //   handlers->NewMessageHandlers()
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
    (void)name;
    return static_cast<FieldHandlers*>(
        upb_mhandlers_newfhandlers_subm(this, n, type, repeated, subm));
  }

 private:
  UPB_DISALLOW_CONSTRUCT_AND_DESTRUCT(MessageHandlers);
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

  // Convenience function for registering handlers for all messages and fields
  // in a MessageDef and all its children.  For every registered message,
  // OnMessage will be called on the visitor with newly-created MessageHandlers
  // and MessageDef. Likewise with OnField will be called with newly-created
  // FieldHandlers and FieldDef for each field.
  class MessageRegistrationVisitor {
   public:
    virtual ~MessageRegistrationVisitor() {}
    virtual void OnMessage(MessageHandlers* mh, const MessageDef* m) = 0;
    virtual void OnField(FieldHandlers* fh, const FieldDef* f) = 0;
  };
  MessageHandlers* RegisterMessageDef(const MessageDef& m,
                                      MessageRegistrationVisitor* visitor);

 private:
  UPB_DISALLOW_CONSTRUCT_AND_DESTRUCT(Handlers);
};

inline MessageHandlers* FieldHandlers::GetMessageHandlers() const {
  return static_cast<MessageHandlers*>(upb_fhandlers_getmsg(this));
}

inline MessageHandlers* FieldHandlers::GetSubMessageHandlers() const {
  return static_cast<MessageHandlers*>(upb_fhandlers_getsubmsg(this));
}

}  // namespace upb

#endif
