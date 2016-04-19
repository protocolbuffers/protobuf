
#include "upb/pb/glue.h"

#include "upb/descriptor/reader.h"
#include "upb/pb/decoder.h"

upb_filedef **upb_loaddescriptor(const char *buf, size_t n, const void *owner,
                                 upb_status *status) {
  /* Create handlers. */
  const upb_pbdecodermethod *decoder_m;
  const upb_handlers *reader_h = upb_descreader_newhandlers(&reader_h);
  upb_env env;
  upb_pbdecodermethodopts opts;
  upb_pbdecoder *decoder;
  upb_descreader *reader;
  bool ok;
  size_t i;
  upb_filedef **ret = NULL;

  upb_pbdecodermethodopts_init(&opts, reader_h);
  decoder_m = upb_pbdecodermethod_new(&opts, &decoder_m);

  upb_env_init(&env);
  upb_env_reporterrorsto(&env, status);

  reader = upb_descreader_create(&env, reader_h);
  decoder = upb_pbdecoder_create(&env, decoder_m, upb_descreader_input(reader));

  /* Push input data. */
  ok = upb_bufsrc_putbuf(buf, n, upb_pbdecoder_input(decoder));

  if (!ok) {
    goto cleanup;
  }

  ret = upb_gmalloc(sizeof (*ret) * (upb_descreader_filecount(reader) + 1));

  if (!ret) {
    goto cleanup;
  }

  for (i = 0; i < upb_descreader_filecount(reader); i++) {
    ret[i] = upb_descreader_file(reader, i);
    upb_filedef_ref(ret[i], owner);
  }

  ret[i] = NULL;

cleanup:
  upb_env_uninit(&env);
  upb_handlers_unref(reader_h, &reader_h);
  upb_pbdecodermethod_unref(decoder_m, &decoder_m);
  return ret;
}
