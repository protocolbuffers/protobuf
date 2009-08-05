
#include "main.c"

#include "upb_context.h"
#include "upb_msg.h"

static struct upb_context c;
static struct upb_string str;
static struct upb_msg_parse_state s;
static struct upb_msg *m;
static void *data[NUM_MESSAGES];

static bool initialize()
{
  /* Initialize upb state, parse descriptor. */
  upb_context_init(&c);
  struct upb_string fds;
  if(!upb_strreadfile(MESSAGE_DESCRIPTOR_FILE, &fds)) {
    fprintf(stderr, "Couldn't read " MESSAGE_DESCRIPTOR_FILE ".\n");
    return false;
  }
  if(!upb_context_parsefds(&c, &fds)) {
    fprintf(stderr, "Error importing " MESSAGE_DESCRIPTOR_FILE ".\n");
    return false;
  }
  upb_strfree(fds);

  char class_name[] = MESSAGE_NAME;
  struct upb_string proto_name;
  proto_name.ptr = class_name;
  proto_name.byte_len = sizeof(class_name)-1;
  struct upb_symtab_entry *e = upb_context_lookup(&c, &proto_name);
  if(!e || e->type != UPB_SYM_MESSAGE) {
    fprintf(stderr, "Error finding symbol '" UPB_STRFMT "'.\n",
            UPB_STRARG(proto_name));
    return false;
  }

  m = e->ref.msg;
  for(int i = 0; i < 32; i++)
    data[i] = upb_msgdata_new(m);

  /* Read the message data itself. */
  if(!upb_strreadfile(MESSAGE_FILE, &str)) {
    fprintf(stderr, "Error reading " MESSAGE_FILE "\n");
    return false;
  }
  return true;
}

static void cleanup()
{
  for(int i = 0; i < 32; i++)
    upb_msgdata_free(data[i], m, true);
  upb_strfree(str);
  upb_context_free(&c);
}

static size_t run(int i)
{
  size_t read;
  upb_msg_parse_reset(&s, data[i%NUM_MESSAGES], m, false, BYREF);
  upb_status_t status = upb_msg_parse(&s, str.ptr, str.byte_len, &read);
  if(status != UPB_STATUS_OK && read != str.byte_len) {
    fprintf(stderr, "Error. :(  error=%d, read=%zu\n", status, read);
    return 0;
  }
  return read;
}
