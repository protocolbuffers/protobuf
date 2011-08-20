
#include "main.c"

#include "upb/bytestream.h"
#include "upb/def.h"
#include "upb/msg.h"
#include "upb/pb/decoder.h"
#include "upb/pb/glue.h"

static upb_msgdef *def;
static size_t len;
static void *msg[NUM_MESSAGES];
static upb_stringsrc strsrc;
static upb_decoder d;

static bool initialize()
{
  // Initialize upb state, decode descriptor.
  upb_status status = UPB_STATUS_INIT;
  upb_symtab *s = upb_symtab_new();
  upb_read_descriptorfile(s, MESSAGE_DESCRIPTOR_FILE, &status);
  if(!upb_ok(&status)) {
    upb_status_print(&status, stderr);
    return false;
  }

  def = upb_dyncast_msgdef(upb_symtab_lookup(s, MESSAGE_NAME));
  if(!def) {
    fprintf(stderr, "Error finding symbol '%s'.\n", MESSAGE_NAME);
    return false;
  }
  upb_symtab_unref(s);

  // Read the message data itself.
  char *str = upb_readfile(MESSAGE_FILE, &len);
  if(str == NULL) {
    fprintf(stderr, "Error reading " MESSAGE_FILE "\n");
    return false;
  }
  upb_status_uninit(&status);
  for (int i = 0; i < NUM_MESSAGES; i++)
    msg[i] = upb_stdmsg_new(def);

  upb_stringsrc_init(&strsrc);
  upb_stringsrc_reset(&strsrc, str, len);
  upb_handlers *h = upb_handlers_new();
  upb_accessors_reghandlers(h, def);
  if (!JIT) h->should_jit = false;
  upb_decoder_initforhandlers(&d, h);
  upb_handlers_unref(h);

  if (!BYREF) {
    // TODO: use byref/byval accessors.
  }
  return true;
}

static void cleanup()
{
  for (int i = 0; i < NUM_MESSAGES; i++)
    upb_stdmsg_free(msg[i], def);
  upb_def_unref(UPB_UPCAST(def));
  upb_stringsrc_uninit(&strsrc);
  upb_decoder_uninit(&d);
}

static size_t run(int i)
{
  upb_status status = UPB_STATUS_INIT;
  i %= NUM_MESSAGES;
  upb_msg_clear(msg[i], def);
  upb_decoder_reset(&d, upb_stringsrc_bytesrc(&strsrc),
                    0, UPB_NONDELIMITED, msg[i]);
  upb_decoder_decode(&d, &status);
  if(!upb_ok(&status)) goto err;
  return len;

err:
  fprintf(stderr, "Decode error: ");
  upb_status_print(&status, stderr);
  return 0;
}
