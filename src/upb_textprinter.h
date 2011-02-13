/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2009 Joshua Haberman.  See LICENSE for details.
 */

#ifndef UPB_TEXT_H_
#define UPB_TEXT_H_

#include "upb_stream.h"

#ifdef __cplusplus
extern "C" {
#endif

struct _upb_textprinter;
typedef struct _upb_textprinter upb_textprinter;

upb_textprinter *upb_textprinter_new();
void upb_textprinter_free(upb_textprinter *p);
void upb_textprinter_reset(upb_textprinter *p, upb_handlers *handlers,
                           upb_bytesink *sink, bool single_line);
void upb_textprinter_sethandlers(upb_textprinter *p, upb_handlers *h);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* UPB_TEXT_H_ */
