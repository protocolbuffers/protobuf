#include <nan.h>

#include "defs.h"
#include "int64.h"
#include "util.h"
#include "message.h"
#include "repeatedfield.h"
#include "map.h"

using namespace v8;
using namespace node;

namespace protobuf_js {

const upb::Handlers* Descriptor::PbSerializeHandlers() {
  assert(msgdef_->IsFrozen());
  if (!pb_serialize_handlers_.get()) {
    pb_serialize_handlers_ =
        upb::pb::Encoder::NewHandlers(msgdef_.get());
  }
  return pb_serialize_handlers_.get();
}

const upb::Handlers* Descriptor::JsonSerializeHandlers() {
  assert(msgdef_->IsFrozen());
  if (!json_serialize_handlers_.get()) {
    json_serialize_handlers_ =
        upb::json::Printer::NewHandlers(msgdef_.get());
  }
  return json_serialize_handlers_.get();
}

/////////////////////////////////////////////////////////////////////
// Encoding                                                        //
/////////////////////////////////////////////////////////////////////

// ByteBuffer: collects the output of the encoding process.
class ByteBuffer {
 public:
  ByteBuffer() {
    upb_byteshandler_setstring(&handler_, string_handler, NULL);
    sink_.Reset(&handler_, this);
  }

  upb::BytesSink* input() { return &sink_; }

  const std::string& data() const { return data_; }

 private:
  upb::BytesHandler handler_;
  upb::BytesSink sink_;

  std::string data_;

  static size_t string_handler(void* c, const void* hd,
                               const char* buf, size_t n,
                               const upb_bufhandle* handle) {
    ByteBuffer* self = static_cast<ByteBuffer*>(c);
    self->data_.append(buf, n);
    return n;
  }
};

// Message-tree traversal "put" routines: push a message to a sink.

static bool DoEncodeSeq(Local<Object> rptfield_obj,
                        FieldDescriptor* field,
                        upb::Sink* sink, int depth);
static bool DoEncodeMap(Local<Object> map_obj,
                        FieldDescriptor* field,
                        upb::Sink* sink, int depth);
static bool DoEncodeField(Local<Value> value,
                          FieldDescriptor* field,
                          upb::Sink* sink, int depth,
                          bool skip_default_value);
static bool DoEncodeMessage(Descriptor* desc,
                            Local<Value> message_obj,
                            upb::Sink* sink, int depth);

bool DoEncodeSeq(Local<Object> rptfield_obj,
                 FieldDescriptor* field,
                 upb::Sink* sink, int depth) {
  NanEscapableScope();

  RepeatedField* rptfield = RepeatedField::unwrap(rptfield_obj);
  if (!rptfield) {
    NanThrowError("RepeatedField object of unexpected type");
    return false;
  }

  if (rptfield->Length() == 0) {
    return true;
  }

  upb::Handlers::Selector startseq_sel;
  if (!upb::Handlers::GetSelector(field->fielddef(), UPB_HANDLER_STARTSEQ,
                                  &startseq_sel)) {
    NanThrowError("Could not find STARTSEQ handler");
    return false;
  }
  upb::Handlers::Selector endseq_sel;
  if (!upb::Handlers::GetSelector(field->fielddef(), UPB_HANDLER_ENDSEQ,
                                  &endseq_sel)) {
    NanThrowError("Could not find ENDSEQ handler");
    return false;
  }

  upb::Sink subsink;
  sink->StartSequence(startseq_sel, &subsink);

  for (int i = 0; i < rptfield->Length(); i++) {
    Local<Value> elem = NanNew((*rptfield)[i]);
    if (!DoEncodeField(elem, field, &subsink, depth, false)) {
      return false;
    }
  }

  sink->EndSequence(endseq_sel);
  return true;
}

bool DoEncodeMap(Local<Object> map_obj,
                 FieldDescriptor* field,
                 upb::Sink* sink, int depth) {
  NanEscapableScope();

  Map* map = Map::unwrap(map_obj);
  if (!map) {
    NanThrowError("Map object of unexpected type");
    return false;
  }

  if (map->Length() == 0) {
    return true;
  }

  FieldDescriptor* key_field = field->key_field();
  FieldDescriptor* value_field = field->value_field();

#define GETSEL(name, uppercase)                              \
  upb::Handlers::Selector name ## _sel;                      \
  if (!upb::Handlers::GetSelector(field->fielddef(),         \
                                  UPB_HANDLER_ ## uppercase, \
                                  & name ## _sel)) {         \
    NanThrowError("Could not find " # uppercase " handler"); \
    return false;                                            \
  }

  GETSEL(startseq, STARTSEQ);
  GETSEL(endseq, ENDSEQ);
  GETSEL(startsubmsg, STARTSUBMSG);
  GETSEL(endsubmsg, ENDSUBMSG);

#undef GETSEL

  upb::Sink subsink;
  sink->StartSequence(startseq_sel, &subsink);

  for (Map::Iterator it(*map); !it.Done(); it.Next()) {
    Local<Value> key = NanNew(it.Key());
    Local<Value> value = NanNew(it.Value());

    upb::Sink mapentrysink;
    subsink.StartSubMessage(startsubmsg_sel, &mapentrysink);
    mapentrysink.StartMessage();

    if (!DoEncodeField(key, key_field, &mapentrysink, depth + 1, false)) {
      return false;
    }
    if (!DoEncodeField(value, value_field, &mapentrysink, depth + 1, false)) {
      return false;
    }

    upb::Status status;
    mapentrysink.EndMessage(&status);
    subsink.EndSubMessage(endsubmsg_sel);
  }

  sink->EndSequence(endseq_sel);
  return true;
}

bool DoEncodeField(Local<Value> value,
                   FieldDescriptor* field,
                   upb::Sink* sink, int depth,
                   bool skip_default_value) {

  static const int kMaxEncodingDepth = 100;
  if (depth > kMaxEncodingDepth) {
    NanThrowError("Exceeded maximum recursion depth during encoding: "
                  "perhaps a cycle exists in the message graph?");
    return false;
  }

#define GETSEL(selname, seltype)                                              \
  upb::Handlers::Selector selname;                                            \
  if (!upb::Handlers::GetSelector(field->fielddef(), UPB_HANDLER_ ## seltype, \
                             &selname)) {                                     \
    NanThrowError("Could not find selector");                                 \
    return false;                                                             \
  }

  switch (field->fielddef()->type()) {
    case UPB_TYPE_ENUM:
    case UPB_TYPE_INT32: {
      GETSEL(sel, INT32);
      int32_t int32val = value->Int32Value();
      if (!skip_default_value || int32val != 0) {
        sink->PutInt32(sel, int32val);
      }
      break;
    }

    case UPB_TYPE_UINT32: {
      GETSEL(sel, UINT32);
      uint32_t uint32val = value->Uint32Value();
      if (!skip_default_value || uint32val != 0) {
        sink->PutUInt32(sel, uint32val);
      }
      break;
    }

    case UPB_TYPE_INT64: {
      GETSEL(sel, INT64);
      if (!value->IsObject()) {
        NanThrowError("Expected object for int64 field value");
        return false;
      }
      Local<Object> int64_obj = value->ToObject();
      if (int64_obj->GetPrototype() !=
          NanNew(Int64::prototype_signed)) {
        NanThrowError("Expected Int64 for int64 field value");
        return false;
      }
      // Type checked above.
      Int64* i = Int64::unwrap(int64_obj);
      if (!skip_default_value || i->int64_value() != 0) {
        sink->PutInt64(sel, i->int64_value());
      }
      break;
    }

    case UPB_TYPE_UINT64: {
      GETSEL(sel, UINT64);
      if (!value->IsObject()) {
        NanThrowError("Expected object for uint64 field value");
        return false;
      }
      Local<Object> uint64_obj = value->ToObject();
      if (uint64_obj->GetPrototype() !=
          NanNew(Int64::prototype_unsigned)) {
        NanThrowError("Expected UInt64 for uint64 field value");
        return false;
      }
      // Type checked above.
      Int64* i = Int64::unwrap(uint64_obj);
      if (!skip_default_value || i->uint64_value() != 0) {
        sink->PutUInt64(sel, i->uint64_value());
      }
      break;
    }

    case UPB_TYPE_BOOL: {
      GETSEL(sel, BOOL);
      if (!value->IsBoolean()) {
        NanThrowError("Expected bool for bool field value");
        return false;
      }
      bool boolval = value->BooleanValue();
      if (!skip_default_value || boolval) {
        sink->PutBool(sel, boolval);
      }
      break;
    }

    case UPB_TYPE_FLOAT: {
      GETSEL(sel, FLOAT);
      if (!value->IsNumber()) {
        NanThrowError("Expected number for float field value");
        return false;
      }
      float floatval = value->NumberValue();
      if (!skip_default_value || floatval != 0.0) {
        sink->PutFloat(sel, floatval);
      }
      break;
    }

    case UPB_TYPE_DOUBLE: {
      GETSEL(sel, DOUBLE);
      if (!value->IsNumber()) {
        NanThrowError("Expected number for double field value");
        return false;
      }
      double doubleval = value->NumberValue();
      if (!skip_default_value || doubleval != 0.0) {
        sink->PutDouble(sel, doubleval);
      }
      break;
    }

    case UPB_TYPE_STRING: {
      GETSEL(startstr_sel, STARTSTR);
      GETSEL(str_sel, STRING);
      GETSEL(endstr_sel, ENDSTR);
      if (!value->IsString()) {
        NanThrowError("Expected string for string field value");
        return false;
      }

      Local<String> str = NanNew(value->ToString());
      if (!skip_default_value || str->Length() > 0) {
        String::Utf8Value utf8val(str);
        upb::Sink subsink;
        sink->StartString(startstr_sel, utf8val.length(), &subsink);
        subsink.PutStringBuffer(str_sel, *utf8val, utf8val.length(), NULL);
        sink->EndString(endstr_sel);
      }

      break;
    }

    case UPB_TYPE_BYTES: {
      GETSEL(startstr_sel, STARTSTR);
      GETSEL(str_sel, STRING);
      GETSEL(endstr_sel, ENDSTR);
      if (!node::Buffer::HasInstance(value)) {
        NanThrowError("Expected Buffer for bytes field value");
        return false;
      }

      const char* data = node::Buffer::Data(value);
      size_t length = node::Buffer::Length(value);
      if (!skip_default_value || length > 0) {
        upb::Sink subsink;
        sink->StartString(startstr_sel, length, &subsink);
        subsink.PutStringBuffer(str_sel, data, length, NULL);
        sink->EndString(endstr_sel);
      }
      break;
    }

    case UPB_TYPE_MESSAGE: {
      GETSEL(startsubmsg_sel, STARTSUBMSG);
      GETSEL(endsubmsg_sel, ENDSUBMSG);

      if (!value->IsUndefined()) {
        upb::Sink subsink;
        sink->StartSubMessage(startsubmsg_sel, &subsink);
        if (!DoEncodeMessage(field->submsg(), value, &subsink, depth + 1)) {
          return false;
        }
        sink->EndSubMessage(endsubmsg_sel);
      }
      break;
    }
  }

  return true;

#undef GETSEL
}

bool DoEncodeMessage(Descriptor* desc,
                     Local<Value> message_value,
                     upb::Sink* sink, int depth) {
  NanEscapableScope();
  upb::Status status;
  sink->StartMessage();

  if (!message_value->IsObject()) {
    NanThrowError("Expected object for message value");
    return false;
  }

  Local<Object> message_obj = NanNew(message_value->ToObject());
  if (!message_obj->GetPrototype()->IsObject() ||
      message_obj->GetPrototype()->ToObject() != desc->Prototype()) {
    NanThrowError("Expected object of different type for message value");
    return false;
  }

  // Type checked above.
  ProtoMessage* message = ProtoMessage::unwrap(message_obj);

  for (Descriptor::FieldIterator it = message->desc()->fields_begin();
       it != message->desc()->fields_end(); ++it) {
    Local<Object> fieldobj = NanNew(*it);
    FieldDescriptor* field = FieldDescriptor::unwrap(fieldobj);

    // Skip if oneof and the oneof case is not set to this field.
    if (field->oneof()) {
      uint32_t oneof_case = message_obj->GetInternalField(
          field->oneof()->LayoutCaseSlot())->Uint32Value();
      if (oneof_case != field->fielddef()->number()) {
        continue;
      }
    }

    Local<Value> value = message_obj->GetInternalField(field->LayoutSlot());

    if (field->IsMapField()) {
      if (!DoEncodeMap(value->ToObject(), field, sink, depth)) {
        return false;
      }
    } else if (field->fielddef()->IsSequence()) {
      assert(value->IsObject());
      if (!DoEncodeSeq(value->ToObject(), field, sink, depth)) {
        return false;
      }
    } else {
      if (!DoEncodeField(value, field, sink, depth, true)) {
        return false;
      }
    }
  }

  sink->EndMessage(&status);
  return true;
}

// Top-level encode: PB binary format or JSON format.
Handle<Value> EncodeImpl(Local<Value> msgclass, Local<Value> msg,
                         bool is_json) {
  NanEscapableScope();

  if (!msgclass->IsObject()) {
    NanThrowError("Message class parameter is not an object");
    return Handle<Value>();
  }

  Local<Value> desc_value =
      msgclass->ToObject()->Get(NanNew<String>("descriptor"));
  if (!desc_value->IsObject()) {
    NanThrowError("No descriptor property on message class or is not object");
    return Handle<Value>();
  }
  if (!desc_value->IsObject() ||
      !desc_value->ToObject()->GetPrototype()->IsObject() ||
      desc_value->ToObject()->GetPrototype()->ToObject() !=
      NanNew(Descriptor::prototype)) {
    NanThrowError("descriptor object is not an instance of Descriptor");
    return Handle<Value>();
  }

  Local<Object> desc_obj = desc_value->ToObject();
  Descriptor* desc = Descriptor::unwrap(desc_obj);

  if (!msg->IsObject()) {
    NanThrowError("Message parameter is not an object");
    return Handle<Value>();
  }
  Local<Object> msg_obj = msg->ToObject();
  if (!msg_obj->GetPrototype()->IsObject() ||
      msg_obj->GetPrototype()->ToObject() != desc->Prototype()) {
    NanThrowError("Object given to encode() is not of correct type");
    return Handle<Value>();
  }

  ByteBuffer bytebuf;

  if (is_json) {
    upb::json::Printer printer(desc->JsonSerializeHandlers());
    printer.ResetOutput(bytebuf.input());

    if (!DoEncodeMessage(desc, msg, printer.input(), 0)) {
      return Handle<Value>();
    }
    return NanEscapeScope(NanNew<String>(bytebuf.data().data(),
                                         bytebuf.data().size()));
  } else {
    upb::pb::Encoder encoder(desc->PbSerializeHandlers());
    encoder.ResetOutput(bytebuf.input());

    if (!DoEncodeMessage(desc, msg, encoder.input(), 0)) {
      return Handle<Value>();
    }
    return NanEscapeScope(NewNodeBuffer(bytebuf.data().data(),
                                        bytebuf.data().size()));
  }
}

// This implements the `encode` class method that's added to every message
// class.
NAN_METHOD(EncodeMethod) {
  NanEscapableScope();

  if (args.Length() != 1) {
    NanThrowError("Expected one argument: the message instance");
    NanReturnUndefined();
  }

  Local<Value> msgclass = args.This();
  Local<Value> msg = args[0];

  NanReturnValue(NanNew(
              EncodeImpl(msgclass, msg, /* is_json = */ false)));
}

// This implements the top-level `protobuf.encode` method.
NAN_METHOD(EncodeGlobalFunction) {
  NanEscapableScope();

  if (args.Length() != 2) {
    NanThrowError("Expected two arguments: message class and "
                  "message instance");
    NanReturnUndefined();
  }

  Local<Value> msgclass = args[0];
  Local<Value> msg = args[1];

  NanReturnValue(NanNew(
              EncodeImpl(msgclass, msg, /* is_json = */ false)));
}

// This implements the `encodeJson` class method that's added to every message
// class.
NAN_METHOD(EncodeJsonMethod) {
  NanEscapableScope();

  if (args.Length() != 1) {
    NanThrowError("Expected one argument: the message instance");
    NanReturnUndefined();
  }

  Local<Value> msgclass = args.This();
  Local<Value> msg = args[0];

  NanReturnValue(NanNew(
              EncodeImpl(msgclass, msg, /* is_json = */ true)));
}

// This implements the top-level `protobuf.encodeJson` method.
NAN_METHOD(EncodeJsonGlobalFunction) {
  NanEscapableScope();

  if (args.Length() != 2) {
    NanThrowError("Expected two arguments: message class and "
                  "message instance");
    NanReturnUndefined();
  }

  Local<Value> msgclass = args[0];
  Local<Value> msg = args[1];

  NanReturnValue(NanNew(
              EncodeImpl(msgclass, msg, /* is_json = */ true)));
}

/////////////////////////////////////////////////////////////////////
// Decoding                                                        //
/////////////////////////////////////////////////////////////////////

class FieldDescriptorData {
 public:
  FieldDescriptorData(FieldDescriptor* field) : field_(field)  {}
  const FieldDescriptor* field() const { return field_; }
 private:
  FieldDescriptor* field_;
};

class MessageClosure {
 public:
  MessageClosure(ProtoMessage* message, Handle<Object> message_obj)
      : message_(message), repeated_field_(NULL), map_(NULL) {
    NanAssignPersistent(message_obj_, message_obj);
  }

  MessageClosure(RepeatedField* repeated_field)
      : message_(NULL), repeated_field_(repeated_field), map_(NULL)
  { }

  MessageClosure(Map* map)
      : message_(NULL), repeated_field_(NULL), map_(map)
  { }

  bool OnInt32(const FieldDescriptorData* hd, int32_t value) {
    NanEscapableScope();
    SetValue(hd->field(), NanNew<Integer>(value));
    return true;
  }

  bool OnUInt32(const FieldDescriptorData* hd, uint32_t value) {
    NanEscapableScope();
    SetValue(hd->field(), NanNewUInt32(value));
    return true;
  }

  bool OnInt64(const FieldDescriptorData* hd, int64_t value) {
    NanEscapableScope();
    Local<Object> i = NanNew(Int64::constructor_signed)->
        NewInstance(0, NULL);
    Int64::unwrap(i)->set_int64_value(value);
    SetValue(hd->field(), i);
    return true;
  }

  bool OnUInt64(const FieldDescriptorData* hd, uint64_t value) {
    NanEscapableScope();
    Local<Object> i = NanNew(Int64::constructor_unsigned)->
        NewInstance(0, NULL);
    Int64::unwrap(i)->set_uint64_value(value);
    SetValue(hd->field(), i);
    return true;
  }

  bool OnBool(const FieldDescriptorData* hd, bool value) {
    NanEscapableScope();
    SetValue(hd->field(), NanNew(value ? NanTrue() : NanFalse()));
    return true;
  }

  bool OnFloat(const FieldDescriptorData* hd, float value) {
    NanEscapableScope();
    SetValue(hd->field(), NanNew<Number>(value));
    return true;
  }

  bool OnDouble(const FieldDescriptorData* hd, double value) {
    NanEscapableScope();
    SetValue(hd->field(), NanNew<Number>(value));
    return true;
  }

  size_t OnString(const FieldDescriptorData* hd,
                  const char* data,
                  size_t length) {
    string_data_.append(data, length);
    return length;
  }

  void OnEndStr(const FieldDescriptorData* hd) {
    NanEscapableScope();
    Local<Value> val;
    if (hd->field()->fielddef()->type() == UPB_TYPE_BYTES) {
      val = NewNodeBuffer(string_data_.data(),
                          string_data_.size());
    } else {
      val = NanNew<String>(string_data_.data(), string_data_.size());
    }
    SetValue(hd->field(), val);
    string_data_.clear();
  }

  MessageClosure* OnStartSequence(const FieldDescriptorData* hd) {
    NanEscapableScope();
    Local<Value> rptfield_obj = NanNew(message_obj_)->GetInternalField(
        hd->field()->LayoutSlot());
    RepeatedField* rptfield = RepeatedField::unwrap(rptfield_obj->ToObject());
    return new MessageClosure(rptfield);
  }

  MessageClosure* OnStartSubMsg(const FieldDescriptorData* hd) {
    NanEscapableScope();
    Local<Object> msg = const_cast<FieldDescriptor*>(hd->field())->
        submsg()->Constructor()->NewInstance(0, NULL);
    SetValue(hd->field(), msg);
    return new MessageClosure(ProtoMessage::unwrap(msg), msg);
  }

  // StartSeq handler
  MessageClosure* OnStartMap(const FieldDescriptorData* hd) {
    NanEscapableScope();
    Local<Value> map_obj = NanNew(message_obj_)->GetInternalField(
        hd->field()->LayoutSlot());
    Map* map = Map::unwrap(map_obj->ToObject());
    return new MessageClosure(map);
  }

  // StartSubMsg handler
  MessageClosure* OnStartMapEntry(const FieldDescriptorData* hd) {
    // Allocate a new subclosure, because the parser expects to free the closure
    // at the end of the frame.
    MessageClosure* child = new MessageClosure(map_);

    // key and value fields start at default.
    FieldDescriptor* f = const_cast<FieldDescriptor*>(hd->field());
    NanAssignPersistent(child->map_key_data_,
                        ProtoMessage::NewField(f->key_field()));
    NanAssignPersistent(child->map_value_data_,
                        ProtoMessage::NewField(f->value_field()));
    return child;
  }

  // EndMessage handler
  bool OnEndMapEntry(const FieldDescriptorData* hd, upb::Status* st) {
    NanEscapableScope();
    if (map_key_data_.IsEmpty() || map_value_data_.IsEmpty()) {
      NanThrowError("Key or value missing in MapEntry submessage");
      return false;
    }
    return map_->InternalSet(NanNew(map_key_data_), NanNew(map_value_data_),
                             /* allow_copy = */ false);
  }

 private:
  void HandleOneof(const FieldDescriptor* field) {
    // Valid only in singular-field contexts.
    if (!message_) {
      return;
    }
    // If this field is part of a oneof, set the oneof case to indicate that
    // this field is present.
    if (field->oneof()) {
      NanNew(message_obj_)->SetInternalField(
          field->oneof()->LayoutCaseSlot(),
          NanNew<Integer>(field->fielddef()->number()));
    }
  }

  void SetValue(const FieldDescriptor* field, Local<Value> value) {
    NanEscapableScope();
    if (message_) {
      // Singular-field case.
      HandleOneof(field);
      NanNew(message_obj_)->SetInternalField(field->LayoutSlot(), value);
    } else if (repeated_field_) {
      // Repeated-field case.
      repeated_field_->DoPush(value, /* allow_copy = */ false);
    } else if (map_) {
      // Map case. We may be setting either the key or the value for a
      // particular entry. We keep these in the closure context to be added to
      // the map when the map-entry submessage ends.
      switch (field->fielddef()->number()) {
        case UPB_MAPENTRY_KEY:
          NanAssignPersistent(map_key_data_, value);
          break;
        case UPB_MAPENTRY_VALUE:
          NanAssignPersistent(map_value_data_, value);
          break;
        default:
          assert(false);
          break;
      }
    }
  }

  // The closure transitions through several states:
  //
  // - At a message scope. message_ and message_obj_ are non-NULL.
  //
  // - In a repeated field. repeated_field_ is non-NULL.
  //
  // - In a map. map_ is non-NULL. A submessage start clears map_key_data_ and
  //   map_value_data_; a submessage end adds the (key, value) pair to the map.
  //
  // - In the middle of a string field. string_data_ is accumulating the data,
  //   and the other fields are consistent with message scope, repeated-field
  //   scope or map-entry scope.

  // Either message_ is non-NULL (we're in a message context) or repeated_field_
  // is non-NULL (we're in a sequence context).
  ProtoMessage* message_;
  Persistent<Object> message_obj_;

  RepeatedField* repeated_field_;

  Map* map_;
  Persistent<Value> map_key_data_;
  Persistent<Value> map_value_data_;

  std::string string_data_;
};

static void AddHandlersForField(upb::Handlers* h,
                                Descriptor* desc,
                                FieldDescriptor* field) {

  if (field->IsMapField()) {
    // Add the start-map handler that sets up the map closure context at parse
    // time, and the startsubmsg handler to handle each MapEntry, but do not add
    // the ordinary submessage handlers.
    h->SetStartSequenceHandler(
        field->fielddef(), UpbBind(&MessageClosure::OnStartMap,
                                   new FieldDescriptorData(field)));
    h->SetStartSubMessageHandler(
        field->fielddef(), UpbBind(&MessageClosure::OnStartMapEntry,
                                   new FieldDescriptorData(field)));
    return;
  }

  if (field->fielddef()->IsSequence()) {
    h->SetStartSequenceHandler(
        field->fielddef(), UpbBind(&MessageClosure::OnStartSequence,
                                   new FieldDescriptorData(field)));
  }

  switch (field->fielddef()->type()) {
#define T(upbtype, type)                                                      \
    case UPB_TYPE_ ## upbtype :                                               \
      h->Set##type##Handler(field->fielddef(),                                \
                            UpbBind(&MessageClosure::On##type,                \
                            new FieldDescriptorData(field)));                 \
      break;
    T(INT32, Int32);
    T(UINT32, UInt32);
    T(INT64, Int64);
    T(UINT64, UInt64);
    T(ENUM, Int32);
    T(BOOL, Bool);
    T(FLOAT, Float);
    T(DOUBLE, Double);
#undef T

    case UPB_TYPE_STRING:
    case UPB_TYPE_BYTES:
      h->SetStringHandler(field->fielddef(),
                          UpbBind(&MessageClosure::OnString,
                                  new FieldDescriptorData(field)));
      h->SetEndStringHandler(field->fielddef(),
                             UpbBind(&MessageClosure::OnEndStr,
                                     new FieldDescriptorData(field)));
      break;

    case UPB_TYPE_MESSAGE:
      h->SetStartSubMessageHandler(field->fielddef(),
                                   UpbBind(&MessageClosure::OnStartSubMsg,
                                           new FieldDescriptorData(field)));
      break;
  }
}

static void MakeFillHandlerCallback(const void* closure,
                                    upb::Handlers* h) {
  NanEscapableScope();
  DescriptorPool* pool = const_cast<DescriptorPool*>(
      static_cast<const DescriptorPool*>(closure));
  Descriptor* desc = pool->FindDescByDef(h->message_def());

  // For each field, add a handler.
  for (Descriptor::FieldIterator it = desc->fields_begin();
       it != desc->fields_end();
       ++it) {
    FieldDescriptor* field = FieldDescriptor::unwrap(NanNew(*it));

    // If this is a MapEntry message, ensure that we add handlers only for the key
    // and value fields.
    if (desc->msgdef()->mapentry() &&
        field->fielddef()->number() != UPB_MAPENTRY_KEY &&
        field->fielddef()->number() != UPB_MAPENTRY_VALUE) {
      continue;
    }

    AddHandlersForField(h, desc, field);
  }

  // If this is a MapEntry message, add an EndMessage handler to add the entry
  // to the map.
  if (desc->msgdef()->mapentry()) {
    h->SetEndMessageHandler(
        UpbBind(&MessageClosure::OnEndMapEntry,
                NULL));
  }
}

static upb::reffed_ptr<const upb::Handlers> MakeFillHandlers(Descriptor* desc) {
  return upb::Handlers::NewFrozen(desc->msgdef(),
                                  MakeFillHandlerCallback,
                                  desc->pool());
}

const upb::Handlers* Descriptor::FillHandlers() {
  assert(msgdef_->IsFrozen());
  if (!fill_handlers_.get()) {
    fill_handlers_ = MakeFillHandlers(this);
  }
  return fill_handlers_.get();
}

const upb::pb::DecoderMethod* Descriptor::DecoderMethod() {
  assert(msgdef_->IsFrozen());
  if (!decoder_method_.get()) {
    upb::pb::DecoderMethodOptions options(FillHandlers());
    decoder_method_ = upb::pb::DecoderMethod::New(options);
  }
  return decoder_method_.get();
}

// Top-level decode: PB binary format or JSON format.
Handle<Value> DecodeImpl(Local<Value> msgclass, Local<Value> data,
                         bool is_json) {
  NanEscapableScope();

  // Check that the given message class is a valid message class, and extract
  // the descriptor.
  if (!msgclass->IsObject()) {
    NanThrowError("Message class parameter is not an object");
    return Handle<Value>();
  }

  Local<Value> desc_value =
      msgclass->ToObject()->Get(NanNew<String>("descriptor"));
  if (!desc_value->IsObject()) {
    NanThrowError("No descriptor property on message class or is not object");
    return Handle<Value>();
  }
  if (!desc_value->IsObject() ||
      !desc_value->ToObject()->GetPrototype()->IsObject() ||
      desc_value->ToObject()->GetPrototype()->ToObject() !=
      Descriptor::prototype) {
    NanThrowError("descriptor object is not an instance of Descriptor");
    return Handle<Value>();
  }

  Local<Object> desc_obj = desc_value->ToObject();
  Descriptor* desc = Descriptor::unwrap(desc_obj);

  // Allocate a new top-level message object.
  Local<Object> msg_obj = desc->Constructor()->NewInstance(0, NULL);
  ProtoMessage* msg = ProtoMessage::unwrap(msg_obj);

  if (is_json) {
    // Create a decoder with a protobuf wire format decoder method for this
    // message type.
    upb::Status status;
    upb::json::Parser parser(&status);
    upb::Sink sink(desc->FillHandlers(), new MessageClosure(msg, msg_obj));
    parser.ResetOutput(&sink);

    // Push the JSON data to the input BytesSink.
    if (!data->IsString()) {
      NanThrowError("Message JSON data is not a string");
      return Handle<Value>();
    }
    Local<String> data_str = data->ToString();
    if (!upb::BufferSource::PutBuffer(*String::Utf8Value(data_str),
                                      data_str->Length(),
                                      parser.input())) {
      NanThrowError("Decoding failed while pushing JSON data into parser");
      return Handle<Value>();
    }
  } else {
    // Create a decoder with a protobuf wire format decoder method for this
    // message type.
    const upb::pb::DecoderMethod* method = desc->DecoderMethod();
    upb::Status status;
    upb::pb::Decoder decoder(method, &status);
    upb::Sink sink(desc->FillHandlers(), new MessageClosure(msg, msg_obj));
    decoder.ResetOutput(&sink);

    // Push the binary data to the input BytesSink.
    if (!data->IsObject() || !node::Buffer::HasInstance(data)) {
      NanThrowError("Message binary data is not a Buffer");
      return Handle<Value>();
    }
    const char* bytes = node::Buffer::Data(data);
    size_t length = node::Buffer::Length(data);
    if (!upb::BufferSource::PutBuffer(bytes, length, decoder.input())) {
      NanThrowError("Decoding failed while pushing bytes into decoder method");
      return Handle<Value>();
    }
  }

  return NanEscapeScope(msg_obj);
}

// This implements the `decode` class method that's added to every message
// class.
NAN_METHOD(DecodeMethod) {
  NanEscapableScope();

  if (args.Length() != 1) {
    NanThrowError("Expected one argument: the message data");
    NanReturnUndefined();
  }

  Local<Value> msgclass = args.This();
  Local<Value> data = args[0];

  NanReturnValue(NanNew(
              DecodeImpl(msgclass, data, /* is_json = */ false)));
}

// This implements the top-level `protobuf.decode` method.
NAN_METHOD(DecodeGlobalFunction) {
  NanEscapableScope();

  if (args.Length() != 2) {
    NanThrowError("Expected two arguments: message class and "
                  "message data");
    NanReturnUndefined();
  }

  Local<Value> msgclass = args[0];
  Local<Value> data = args[1];

  NanReturnValue(NanNew(
              DecodeImpl(msgclass, data, /* is_json = */ false)));
}

// This implements the `decodeJson` class method that's added to every message
// class.
NAN_METHOD(DecodeJsonMethod) {
  NanEscapableScope();

  if (args.Length() != 1) {
    NanThrowError("Expected one argument: the message data");
    NanReturnUndefined();
  }

  Local<Value> msgclass = args.This();
  Local<Value> data = args[0];

  NanReturnValue(NanNew(
              DecodeImpl(msgclass, data, /* is_json = */ true)));
}

// This implements the top-level `protobuf.decode` method.
NAN_METHOD(DecodeJsonGlobalFunction) {
  NanEscapableScope();

  if (args.Length() != 2) {
    NanThrowError("Expected two arguments: message class and "
                  "message data");
    NanReturnUndefined();
  }

  Local<Value> msgclass = args[0];
  Local<Value> data = args[1];

  NanReturnValue(NanNew(
              DecodeImpl(msgclass, data, /* is_json = */ true)));
}

}  // namespace protobuf_js
