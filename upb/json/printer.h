/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2014 Google Inc.  See LICENSE for details.
 * Author: Josh Haberman <jhaberman@gmail.com>
 *
 * upb::json::Printer allows you to create handlers that emit JSON
 * according to a specific protobuf schema.
 */

#ifndef UPB_JSON_TYPED_PRINTER_H_
#define UPB_JSON_TYPED_PRINTER_H_

#include "upb/sink.h"

#ifdef __cplusplus
namespace upb {
namespace json {
class Printer;
}  // namespace json
}  // namespace upb
#endif

UPB_DECLARE_TYPE(upb::json::Printer, upb_json_printer);


/* upb::json::Printer *********************************************************/

// Prints an incoming stream of data to a BytesSink in JSON format.
UPB_DEFINE_CLASS0(upb::json::Printer,
 public:
  Printer(const upb::Handlers* handlers);
  ~Printer();

  // Resets the state of the printer, so that it will expect to begin a new
  // document.
  void Reset();

  // Resets the output pointer which will serve as our closure.  Implies
  // Reset().
  void ResetOutput(BytesSink* output);

  // The input to the printer.
  Sink* input();

  // Returns handlers for printing according to the specified schema.
  static reffed_ptr<const Handlers> NewHandlers(const upb::MessageDef* md);
,
UPB_DEFINE_STRUCT0(upb_json_printer,
  upb_sink input_;
  // Pointer to yajl_gen; void* here so we don't have to include YAJL headers.
  void *yajl_gen_;
  void *subc_;
  upb_bytessink *output_;
  // We track the depth so that we know when to emit startstr/endstr on the
  // output.
  int depth_;
));

UPB_BEGIN_EXTERN_C  // {

// Native C API.

void upb_json_printer_init(upb_json_printer *p, const upb_handlers *h);
void upb_json_printer_uninit(upb_json_printer *p);
void upb_json_printer_reset(upb_json_printer *p);
void upb_json_printer_resetoutput(upb_json_printer *p, upb_bytessink *output);
upb_sink *upb_json_printer_input(upb_json_printer *p);
const upb_handlers *upb_json_printer_newhandlers(const upb_msgdef *md,
                                                 const void *owner);

UPB_END_EXTERN_C  // }

#ifdef __cplusplus

namespace upb {
namespace json {
inline Printer::Printer(const upb::Handlers* handlers) {
  upb_json_printer_init(this, handlers);
}
inline Printer::~Printer() { upb_json_printer_uninit(this); }
inline void Printer::Reset() { upb_json_printer_reset(this); }
inline void Printer::ResetOutput(BytesSink* output) {
  upb_json_printer_resetoutput(this, output);
}
inline Sink* Printer::input() { return upb_json_printer_input(this); }
inline reffed_ptr<const Handlers> Printer::NewHandlers(
    const upb::MessageDef *md) {
  const Handlers* h = upb_json_printer_newhandlers(md, &h);
  return reffed_ptr<const Handlers>(h, &h);
}
}  // namespace json
}  // namespace upb

#endif

#endif  // UPB_JSON_TYPED_PRINTER_H_
