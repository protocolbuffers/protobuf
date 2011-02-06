/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2010 Joshua Haberman.  See LICENSE for details.
 */

#include "upb_glue.h"
#include "upb_msg.h"
#include "upb_decoder.h"
#include "upb_strstream.h"

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
