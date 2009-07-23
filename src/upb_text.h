/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2009 Joshua Haberman.  See LICENSE for details.
 */

#ifndef UPB_TEXT_H_
#define UPB_TEXT_H_

#include "upb.h"

#ifdef __cplusplus
extern "C" {
#endif

struct upb_text_printer {
  int indent_depth;
  bool single_line;
};

INLINE void upb_text_printer_init(struct upb_text_printer *p, bool single_line) {
  p->indent_depth = 0;
  p->single_line = single_line;
}
void upb_text_printval(upb_field_type_t type, union upb_value p, FILE *file);
void upb_text_printfield(struct upb_text_printer *p, struct upb_string name,
                         upb_field_type_t valtype, union upb_value val,
                         FILE *stream);
void upb_text_push(struct upb_text_printer *p, struct upb_string submsg_type,
                   FILE *stream);
void upb_text_pop(struct upb_text_printer *p, FILE *stream);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* UPB_TEXT_H_ */
