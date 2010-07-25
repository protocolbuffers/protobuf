
#include "main.c"

#include "upb_def.h"
#include "upb_decoder.h"
#include "upb_strstream.h"

static upb_stringsrc *stringsrc;
static upb_string *input_str;
static upb_string *tmp_str;
static upb_msgdef *def;
static upb_decoder *decoder;

static bool initialize()
{
  // Initialize upb state, decode descriptor.
  upb_status status = UPB_STATUS_INIT;
  upb_symtab *s = upb_symtab_new();
  upb_symtab_add_descriptorproto(s);
  upb_def *fds_def = upb_symtab_lookup(
      s, UPB_STRLIT("google.protobuf.FileDescriptorSet"));
  if (!fds_def) {
    fprintf(stderr, "Couldn't load FileDescriptorSet def");
  }

  upb_string *fds_str = upb_strreadfile(MESSAGE_DESCRIPTOR_FILE);
  if(fds_str == NULL) {
    fprintf(stderr, "Couldn't read " MESSAGE_DESCRIPTOR_FILE ":"),
    upb_printerr(&status);
    return false;
  }

  upb_stringsrc *ssrc = upb_stringsrc_new();
  upb_stringsrc_reset(ssrc, fds_str);
  upb_decoder *d = upb_decoder_new(upb_downcast_msgdef(fds_def));
  upb_decoder_reset(d, upb_stringsrc_bytesrc(ssrc));

  upb_symtab_addfds(s, upb_decoder_src(d), &status);

  if(!upb_ok(&status)) {
    fprintf(stderr, "Error importing " MESSAGE_DESCRIPTOR_FILE ":");
    upb_printerr(&status);
    return false;
  }

  upb_string_unref(fds_str);
  upb_decoder_free(d);
  upb_stringsrc_free(ssrc);
  upb_def_unref(fds_def);

  def = upb_downcast_msgdef(upb_symtab_lookup(s, UPB_STRLIT(MESSAGE_NAME)));
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
  tmp_str = NULL;
  decoder = upb_decoder_new(def);
  stringsrc = upb_stringsrc_new();
  return true;
}

static void cleanup()
{
  upb_string_unref(input_str);
  upb_string_unref(tmp_str);
  upb_def_unref(UPB_UPCAST(def));
  upb_decoder_free(decoder);
  upb_stringsrc_free(stringsrc);
}

static size_t run(int i)
{
  (void)i;
  upb_status status = UPB_STATUS_INIT;
  upb_stringsrc_reset(stringsrc, input_str);
  upb_decoder_reset(decoder, upb_stringsrc_bytesrc(stringsrc));
  upb_src *src = upb_decoder_src(decoder);
  upb_fielddef *f;
  upb_string *str = NULL;
  int depth = 0;
  while(1) {
    while((f = upb_src_getdef(src)) != NULL) {
      if(upb_issubmsg(f)) {
        upb_src_startmsg(src);
        ++depth;
      } else if(upb_isstring(f)) {
        tmp_str = upb_string_tryrecycle(str);
        upb_src_getstr(src, tmp_str);
      } else {
        // Primitive type.
        upb_value val;
        upb_src_getval(src, upb_value_addrof(&val));
      }
    }
    // If we're not EOF now, the loop terminated due to an error.
    if (!upb_src_eof(src)) goto err;
    if (depth == 0) break;
    --depth;
    upb_src_endmsg(src);
  }
  if(!upb_ok(&status)) goto err;
  return upb_string_len(input_str);

err:
  fprintf(stderr, "Decode error");
  upb_printerr(&status);
  return 0;
}
