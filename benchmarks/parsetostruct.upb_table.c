
#include "main.c"

#include "upb_context.h"
#include "upb_msg.h"
#include "upb_mm.h"

static struct upb_context *c;
static struct upb_string *str;
static struct upb_msgdef *def;
static struct upb_msg *msgs[NUM_MESSAGES];
static struct upb_msgparser *mp;

static bool initialize()
{
  // Initialize upb state, parse descriptor.
  struct upb_status status = UPB_STATUS_INIT;
  c = upb_context_new();
  struct upb_string *fds = upb_strreadfile(MESSAGE_DESCRIPTOR_FILE);
  if(!fds) {
    fprintf(stderr, "Couldn't read " MESSAGE_DESCRIPTOR_FILE ": %s.\n",
            status.msg);
    return false;
  }
  upb_context_parsefds(c, fds, &status);
  if(!upb_ok(&status)) {
    fprintf(stderr, "Error importing " MESSAGE_DESCRIPTOR_FILE ": %s.\n",
            status.msg);
    return false;
  }
  upb_string_unref(fds);

  struct upb_string *proto_name = upb_strdupc(MESSAGE_NAME);
  def = upb_downcast_msgdef(upb_context_lookup(c, proto_name));
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
  if(!str) {
    fprintf(stderr, "Error reading " MESSAGE_FILE "\n");
    return false;
  }
  mp = upb_msgparser_new(def);
  return true;
}

static void cleanup()
{
  for(int i = 0; i < NUM_MESSAGES; i++)
    upb_msg_unref(msgs[i]);
  upb_string_unref(str);
  upb_context_unref(c);
  upb_msgparser_free(mp);
}

static size_t run(int i)
{
  struct upb_status status = UPB_STATUS_INIT;
  struct upb_msg *msg = msgs[i%NUM_MESSAGES];
  upb_msgparser_reset(mp, msg, false);
  upb_msg_clear(msg);
  upb_msgparser_parse(mp, str->ptr, str->byte_len, &status);
  if(!upb_ok(&status)) {
    fprintf(stderr, "Parse error: %s\n", status.msg);
    return 0;
  }
  return str->byte_len;
}
