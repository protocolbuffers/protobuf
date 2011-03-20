
#include "main.c"

#include "upb_def.h"
#include "upb_decoder.h"
#include "upb_strstream.h"
#include "upb_glue.h"

static upb_string *input_str;
static upb_msgdef *def;
static upb_decoder decoder;
static upb_stringsrc stringsrc;
upb_handlers handlers;

static bool initialize()
{
  // Initialize upb state, decode descriptor.
  upb_status status = UPB_STATUS_INIT;
  upb_symtab *s = upb_symtab_new();

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

  upb_handlers_init(&handlers, def);
  // Cause all messages to be read, but do nothing when they are.
  upb_register_all(&handlers, NULL, NULL, NULL, NULL, NULL, NULL);
  upb_decoder_init(&decoder, &handlers);
  upb_stringsrc_init(&stringsrc);
  return true;
}

static void cleanup()
{
  upb_string_unref(input_str);
  upb_def_unref(UPB_UPCAST(def));
  upb_decoder_uninit(&decoder);
  upb_stringsrc_uninit(&stringsrc);
  upb_handlers_uninit(&handlers);
}

static size_t run(int i)
{
  (void)i;
  upb_status status = UPB_STATUS_INIT;
  upb_stringsrc_reset(&stringsrc, input_str);
  upb_decoder_reset(&decoder, upb_stringsrc_bytesrc(&stringsrc), NULL);
  upb_decoder_decode(&decoder, &status);
  if(!upb_ok(&status)) goto err;
  return upb_string_len(input_str);

err:
  fprintf(stderr, "Decode error: ");
  upb_printerr(&status);
  return 0;
}
