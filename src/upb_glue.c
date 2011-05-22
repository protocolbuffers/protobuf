/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2010 Google Inc.  See LICENSE for details.
 * Author: Josh Haberman <jhaberman@gmail.com>
 */

#include "upb_glue.h"
#include "upb_msg.h"
#include "upb_decoder.h"
#include "upb_strstream.h"
#include "upb_textprinter.h"

void upb_strtomsg(upb_string *str, upb_msg *msg, upb_msgdef *md,
                  upb_status *status) {
  upb_stringsrc strsrc;
  upb_stringsrc_init(&strsrc);
  upb_stringsrc_reset(&strsrc, str);

  upb_handlers *h = upb_handlers_new();
  upb_msg_reghandlers(h, md);

  upb_decoder d;
  upb_decoder_init(&d, h);
  upb_decoder_reset(&d, upb_stringsrc_bytesrc(&strsrc), msg);
  upb_handlers_unref(h);

  upb_decoder_decode(&d, status);

  upb_stringsrc_uninit(&strsrc);
  upb_decoder_uninit(&d);
}

void upb_msgtotext(upb_string *str, upb_msg *msg, upb_msgdef *md,
                   bool single_line) {
  upb_stringsink strsink;
  upb_stringsink_init(&strsink);
  upb_stringsink_reset(&strsink, str);

  upb_textprinter *p = upb_textprinter_new();
  upb_handlers *h = upb_handlers_new();
  upb_textprinter_reghandlers(h, md);
  upb_textprinter_reset(p, upb_stringsink_bytesink(&strsink), single_line);

  upb_status status = UPB_STATUS_INIT;
  upb_msg_runhandlers(msg, md, h, p, &status);
  // None of {upb_msg_runhandlers, upb_textprinter, upb_stringsink} should be
  // capable of returning an error.
  assert(upb_ok(&status));
  upb_status_uninit(&status);

  upb_stringsink_uninit(&strsink);
  upb_textprinter_free(p);
  upb_handlers_unref(h);
}

void upb_parsedesc(upb_symtab *symtab, upb_string *str, upb_status *status) {
  upb_stringsrc strsrc;
  upb_stringsrc_init(&strsrc);
  upb_stringsrc_reset(&strsrc, str);

  upb_handlers *h = upb_handlers_new();
  upb_defbuilder_reghandlers(h);

  upb_decoder d;
  upb_decoder_init(&d, h);
  upb_handlers_unref(h);
  upb_defbuilder *b = upb_defbuilder_new(symtab);
  upb_decoder_reset(&d, upb_stringsrc_bytesrc(&strsrc), b);

  upb_decoder_decode(&d, status);

  upb_stringsrc_uninit(&strsrc);
  upb_decoder_uninit(&d);
}
