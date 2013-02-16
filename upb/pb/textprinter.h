/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2009 Google Inc.  See LICENSE for details.
 * Author: Josh Haberman <jhaberman@gmail.com>
 */

#ifndef UPB_TEXT_H_
#define UPB_TEXT_H_

#include "upb/bytestream.h"
#include "upb/handlers.h"

#ifdef __cplusplus
extern "C" {
#endif

struct _upb_textprinter;
typedef struct _upb_textprinter upb_textprinter;

upb_textprinter *upb_textprinter_new();
void upb_textprinter_free(upb_textprinter *p);
void upb_textprinter_reset(upb_textprinter *p, upb_bytesink *sink,
                           bool single_line);
const upb_handlers *upb_textprinter_newhandlers(const void *owner,
                                                const upb_msgdef *m);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* UPB_TEXT_H_ */
