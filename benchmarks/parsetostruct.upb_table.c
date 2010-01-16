
#include "main.c"

#include "upb_data.h"
#include "upb_def.h"
#include "upb_decoder.h"

static struct upb_symtab *s;
static upb_strptr str;
static struct upb_msgdef *def;
static upb_msg *msgs[NUM_MESSAGES];
static upb_decoder *decoder;
static upb_msgsink *sink;

static bool initialize()
{
  // Initialize upb state, decode descriptor.
  struct upb_status status = UPB_STATUS_INIT;
  s = upb_symtab_new();
  upb_strptr fds = upb_strreadfile(MESSAGE_DESCRIPTOR_FILE);
  if(upb_string_isnull(fds)) {
    fprintf(stderr, "Couldn't read " MESSAGE_DESCRIPTOR_FILE ": %s.\n",
            status.msg);
    return false;
  }
  upb_symtab_add_desc(s, fds, &status);
  if(!upb_ok(&status)) {
    fprintf(stderr, "Error importing " MESSAGE_DESCRIPTOR_FILE ": %s.\n",
            status.msg);
    return false;
  }
  upb_string_unref(fds);

  upb_strptr proto_name = upb_strdupc(MESSAGE_NAME);
  def = upb_downcast_msgdef(upb_symtab_lookup(s, proto_name));
  if(!def) {
    fprintf(stderr, "Error finding symbol '" UPB_STRFMT "'.\n",
            UPB_STRARG(proto_name));
    return false;
  }
  upb_string_unref(proto_name);

  for(int i = 0; i < NUM_MESSAGES; i++)
    msgs[i] = upb_msg_new(def);

  // Read the message data itself.
  str = upb_strreadfile(MESSAGE_FILE);
  if(upb_string_isnull(str)) {
    fprintf(stderr, "Error reading " MESSAGE_FILE "\n");
    return false;
  }
  decoder = upb_decoder_new(def);
  sink = upb_msgsink_new(def);
  return true;
}

static void cleanup()
{
  for(int i = 0; i < NUM_MESSAGES; i++)
    upb_msg_unref(msgs[i], def);
  upb_string_unref(str);
  upb_symtab_unref(s);
  upb_decoder_free(decoder);
  upb_msgsink_free(sink);
}

static size_t run(int i)
{
  struct upb_status status = UPB_STATUS_INIT;
  upb_msg *msg = msgs[i%NUM_MESSAGES];
  upb_msgsink_reset(sink, msg);
  upb_decoder_reset(decoder, upb_msgsink_sink(sink));
  upb_msg_clear(msg, def);
  size_t decoded = upb_decoder_decode(decoder, str, &status);
  if(!upb_ok(&status) || decoded != upb_strlen(str)) {
    fprintf(stderr, "Decode error: %s\n", status.msg);
    return 0;
  }
  return upb_strlen(str);
}
