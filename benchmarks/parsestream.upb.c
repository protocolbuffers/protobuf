
#include "main.c"

#include <stdlib.h>
#include "upb/bytestream.h"
#include "upb/def.h"
#include "upb/pb/decoder.h"
#include "upb/pb/glue.h"

static char *input_str;
static size_t input_len;
static const upb_msgdef *def;
upb_pipeline pipeline;
static upb_sink *sink;

static void *startsubmsg(void *closure, const void *hd) {
  UPB_UNUSED(closure);
  UPB_UNUSED(hd);
  return input_str;
}

void onmreg(void *c, upb_handlers *h) {
  UPB_UNUSED(c);
  upb_msg_iter i;
  upb_msg_begin(&i, upb_handlers_msgdef(h));
  for(; !upb_msg_done(&i); upb_msg_next(&i)) {
    const upb_fielddef *f = upb_msg_iter_field(&i);
    if (upb_fielddef_type(f) == UPB_TYPE_MESSAGE) {
      upb_handlers_setstartsubmsg(h, f, startsubmsg, NULL, NULL);
    }
  }
}

static bool initialize()
{
  // Initialize upb state, decode descriptor.
  upb_status status = UPB_STATUS_INIT;
  upb_symtab *s = upb_symtab_new(&s);
  upb_load_descriptor_file_into_symtab(s, MESSAGE_DESCRIPTOR_FILE, &status);
  if(!upb_ok(&status)) {
    fprintf(stderr, "Error reading descriptor: %s\n",
            upb_status_getstr(&status));
    return false;
  }

  def = upb_dyncast_msgdef(upb_symtab_lookup(s, MESSAGE_NAME, &def));
  if(!def) {
    fprintf(stderr, "Error finding symbol '%s'.\n", MESSAGE_NAME);
    return false;
  }
  upb_symtab_unref(s, &s);

  // Read the message data itself.
  input_str = upb_readfile(MESSAGE_FILE, &input_len);
  if(input_str == NULL) {
    fprintf(stderr, "Error reading " MESSAGE_FILE "\n");
    return false;
  }

  // Cause all messages to be read, but do nothing when they are.
  const upb_handlers* handlers =
      upb_handlers_newfrozen(def, NULL, &handlers, &onmreg, NULL);
  const upb_handlers* decoder_handlers =
      upb_pbdecoder_gethandlers(handlers, JIT, &decoder_handlers);
  upb_msgdef_unref(def, &def);

  upb_pipeline_init(&pipeline, NULL, 0, upb_realloc, NULL);
  upb_sink *s2 = upb_pipeline_newsink(&pipeline, handlers);
  sink = upb_pipeline_newsink(&pipeline, decoder_handlers);
  upb_pipeline_donateref(&pipeline, decoder_handlers, &decoder_handlers);
  upb_pipeline_donateref(&pipeline, handlers, &handlers);
  upb_pbdecoder *decoder = upb_sink_getobj(sink);
  upb_pbdecoder_resetsink(decoder, s2);
  return true;
}

static void cleanup()
{
  free(input_str);
  upb_pipeline_uninit(&pipeline);
}

static size_t run(int i)
{
  (void)i;
  upb_pipeline_reset(&pipeline);
  if (!upb_bytestream_putstr(sink, input_str, input_len)) {
    fprintf(stderr, "Decode error: %s",
            upb_status_getstr(upb_pipeline_status(&pipeline)));
    return 0;
  }
  return input_len;

}
