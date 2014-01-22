/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2009 Google Inc.  See LICENSE for details.
 * Author: Josh Haberman <jhaberman@gmail.com>
 */

#ifndef UPB_TEXT_H_
#define UPB_TEXT_H_

#include "upb/sink.h"

#ifdef __cplusplus
namespace upb {
namespace pb {
class TextPrinter;
}  // namespace pb
}  // namespace upb

typedef upb::pb::TextPrinter upb_textprinter;
#else
struct upb_textprinter;
typedef struct upb_textprinter upb_textprinter;
#endif

#ifdef __cplusplus
class upb::pb::TextPrinter {
 public:
  // The given handlers must have come from NewHandlers().  It must outlive the
  // TextPrinter.
  explicit TextPrinter(const upb::Handlers* handlers);

  void SetSingleLineMode(bool single_line);

  bool ResetOutput(BytesSink* output);
  Sink* input();

  // If handler caching becomes a requirement we can add a code cache as in
  // decoder.h
  static reffed_ptr<const Handlers> NewHandlers(const MessageDef* md);

 private:
#else
struct upb_textprinter {
#endif
  upb_sink input_;
  upb_bytessink *output_;
  int indent_depth_;
  bool single_line_;
  void *subc;
};

#ifdef __cplusplus
extern "C" {
#endif

// C API.
void upb_textprinter_init(upb_textprinter *p, const upb_handlers *h);
void upb_textprinter_uninit(upb_textprinter *p);
bool upb_textprinter_resetoutput(upb_textprinter *p, upb_bytessink *output);
void upb_textprinter_setsingleline(upb_textprinter *p, bool single_line);
upb_sink *upb_textprinter_input(upb_textprinter *p);

const upb_handlers *upb_textprinter_newhandlers(const upb_msgdef *m,
                                                const void *owner);

#ifdef __cplusplus
}  /* extern "C" */

namespace upb {
namespace pb {
inline TextPrinter::TextPrinter(const upb::Handlers* handlers) {
  upb_textprinter_init(this, handlers);
}
inline void TextPrinter::SetSingleLineMode(bool single_line) {
  upb_textprinter_setsingleline(this, single_line);
}
inline bool TextPrinter::ResetOutput(BytesSink* output) {
  return upb_textprinter_resetoutput(this, output);
}
inline Sink* TextPrinter::input() {
  return upb_textprinter_input(this);
}
inline reffed_ptr<const Handlers> TextPrinter::NewHandlers(
    const MessageDef *md) {
  const Handlers* h = upb_textprinter_newhandlers(md, &h);
  return reffed_ptr<const Handlers>(h, &h);
}
}  // namespace pb
}  // namespace upb

#endif

#endif  /* UPB_TEXT_H_ */
