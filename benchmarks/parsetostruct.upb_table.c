
#include "main.c"

#include "upb_context.h"
#include "upb_msg.h"
#include "upb_mm.h"

static struct upb_context *c;
static struct upb_string *str;
static struct upb_msgdef *def;
static struct upb_msg *msgs[NUM_MESSAGES];

static bool initialize()
{
  /* Initialize upb state, parse descriptor. */
  c = upb_context_new();
  struct upb_string *fds = upb_strreadfile(MESSAGE_DESCRIPTOR_FILE);
  if(!fds) {
    fprintf(stderr, "Couldn't read " MESSAGE_DESCRIPTOR_FILE ".\n");
    return false;
  }
  if(!upb_context_parsefds(c, fds)) {
    fprintf(stderr, "Error importing " MESSAGE_DESCRIPTOR_FILE ".\n");
    return false;
  }
  upb_string_unref(fds);

  char class_name[] = MESSAGE_NAME;
  struct upb_string proto_name;
  proto_name.ptr = class_name;
  proto_name.byte_len = sizeof(class_name)-1;
  struct upb_symtab_entry e;
  upb_status_t success = upb_context_lookup(c, &proto_name, &e);
  if(!success || e.type != UPB_SYM_MESSAGE) {
    fprintf(stderr, "Error finding symbol '" UPB_STRFMT "'.\n",
            UPB_STRARG(&proto_name));
    return false;
  }

  def = e.ref.msg;
  for(int i = 0; i < 32; i++)
    msgs[i] = upb_msg_new(def);

  /* Read the message data itself. */
  str = upb_strreadfile(MESSAGE_FILE);
  if(!str) {
    fprintf(stderr, "Error reading " MESSAGE_FILE "\n");
    return false;
  }
  return true;
}

static void cleanup()
{
  for(int i = 0; i < 32; i++)
    upb_msg_unref(msgs[i]);
  upb_string_unref(str);
  upb_context_unref(c);
}

static size_t run(int i)
{
  upb_status_t status;
  status = upb_msg_parsestr(msgs[i%NUM_MESSAGES], str->ptr, str->byte_len);
  if(status != UPB_STATUS_OK) {
    fprintf(stderr, "Error. :(  error=%d\n", status);
    return 0;
  }
  return str->byte_len;
}
