/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2010 Joshua Haberman.  See LICENSE for details.
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

  upb_decoder d;
  upb_decoder_init(&d, md);
  upb_decoder_reset(&d, upb_stringsrc_bytesrc(&strsrc));
  upb_src *src = upb_decoder_src(&d);

  upb_msgpopulator p;
  upb_msgpopulator_init(&p);
  upb_msgpopulator_reset(&p, msg, md);

  upb_handlers h;
  upb_handlers_init(&h);
  upb_msgpopulator_register_handlers(&p, &h);
  upb_src_sethandlers(src, &h);

  upb_src_run(src, status);

  upb_stringsrc_uninit(&strsrc);
  upb_decoder_uninit(&d);
  upb_msgpopulator_uninit(&p);
  upb_handlers_uninit(&h);
}

void upb_msgtotext(upb_string *str, upb_msg *msg, upb_msgdef *md,
                   bool single_line) {
  upb_stringsink strsink;
  upb_stringsink_init(&strsink);
  upb_stringsink_reset(&strsink, str);

  upb_textprinter *p = upb_textprinter_new();
  upb_handlers h;
  upb_handlers_init(&h);
  upb_textprinter_reset(p, &h, upb_stringsink_bytesink(&strsink), single_line);

  upb_status status = UPB_STATUS_INIT;
  upb_msg_runhandlers(msg, md, &h, &status);
  // None of {upb_msg_runhandlers, upb_textprinter, upb_stringsink} should be
  // capable of returning an error.
  assert(upb_ok(&status));
  upb_status_uninit(&status);

  upb_stringsink_uninit(&strsink);
  upb_textprinter_free(p);
  upb_handlers_uninit(&h);
}

void upb_parsedesc(upb_symtab *symtab, upb_string *str, upb_status *status) {
  upb_stringsrc strsrc;
  upb_stringsrc_init(&strsrc);
  upb_stringsrc_reset(&strsrc, str);

  upb_decoder d;
  upb_msgdef *fds_msgdef = upb_getfdsdef();
  upb_decoder_init(&d, fds_msgdef);
  upb_decoder_reset(&d, upb_stringsrc_bytesrc(&strsrc));

  upb_symtab_addfds(symtab, upb_decoder_src(&d), status);
  upb_stringsrc_uninit(&strsrc);
  upb_decoder_uninit(&d);
  upb_def_unref(UPB_UPCAST(fds_msgdef));
}
