
#include "main.c"

#include "upb_def.h"
#include "upb_decoder.h"
#include "upb_strstream.h"
#include "upb_glue.h"
#include "upb_msg.h"

static upb_string *input_str;
static upb_msgdef *def;
static upb_msg *msg;
static upb_stringsrc strsrc;
static upb_decoder d;
static upb_handlers h;
static upb_msgpopulator p;

static bool initialize()
{
  // Initialize upb state, decode descriptor.
  upb_status status = UPB_STATUS_INIT;
  upb_symtab *s = upb_symtab_new();
  upb_symtab_add_descriptorproto(s);

  upb_string *fds_str = upb_strreadfile(MESSAGE_DESCRIPTOR_FILE);
  if(fds_str == NULL) {
    fprintf(stderr, "Couldn't read " MESSAGE_DESCRIPTOR_FILE ":"),
    upb_printerr(&status);
    return false;
  }
  upb_parsedesc(s, fds_str, &status);
  upb_string_unref(fds_str);

  if(!upb_ok(&status)) {
    fprintf(stderr, "Error importing " MESSAGE_DESCRIPTOR_FILE ":");
    upb_printerr(&status);
    return false;
  }

  def = upb_dyncast_msgdef(upb_symtab_lookup(s, UPB_STRLIT(MESSAGE_NAME)));
  if(!def) {
    fprintf(stderr, "Error finding symbol '" UPB_STRFMT "'.\n",
            UPB_STRARG(UPB_STRLIT(MESSAGE_NAME)));
    return false;
  }
  upb_symtab_unref(s);

  // Read the message data itself.
  input_str = upb_strreadfile(MESSAGE_FILE);
  if(input_str == NULL) {
    fprintf(stderr, "Error reading " MESSAGE_FILE "\n");
    return false;
  }
  upb_status_uninit(&status);
  msg = upb_msg_new(def);

  upb_stringsrc_init(&strsrc);
  upb_decoder_init(&d, def);
  upb_msgpopulator_init(&p);
  upb_handlers_init(&h);
  return true;
}

static void cleanup()
{
  upb_string_unref(input_str);
  upb_msg_unref(msg, def);
  upb_def_unref(UPB_UPCAST(def));
  upb_stringsrc_uninit(&strsrc);
  upb_decoder_uninit(&d);
  upb_msgpopulator_uninit(&p);
  upb_handlers_uninit(&h);
}

static size_t run(int i)
{
  (void)i;
  upb_status status = UPB_STATUS_INIT;
  upb_msg_clear(msg, def);
  upb_stringsrc_reset(&strsrc, input_str);
  upb_decoder_reset(&d, upb_stringsrc_bytesrc(&strsrc));
  upb_msgpopulator_reset(&p, msg, def);
  upb_handlers_init(&h);
  upb_msgpopulator_register_handlers(&p, &h);
  upb_src *src = upb_decoder_src(&d);
  upb_src_sethandlers(src, &h);

  upb_src_run(src, &status);
  if(!upb_ok(&status)) goto err;
  upb_status_uninit(&status);
  return upb_string_len(input_str);

err:
  fprintf(stderr, "Decode error: ");
  upb_printerr(&status);
  return 0;
}
