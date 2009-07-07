
#include "upb_context.h"
#include "upb_msg.h"

int main ()
{
  struct upb_context c;
  upb_context_init(&c);
  struct upb_string fds;
  if(!upb_strreadfile("google_speed.proto.bin", &fds)) {
    fprintf(stderr, "Couldn't read google_speed.proto.bin.\n");
    return 1;
  }
  if(!upb_context_parsefds(&c, &fds)) {
    fprintf(stderr, "Error parsing or resolving proto.\n");
    return 1;
  }
  upb_strfree(fds);
  struct upb_string proto_name = UPB_STRLIT("benchmarks.SpeedMessage2");
  struct upb_symtab_entry *e = upb_context_lookup(&c, &proto_name);
  if(!e || e->type != UPB_SYM_MESSAGE) {
    fprintf(stderr, "Error finding symbol '" UPB_STRFMT "'.\n",
            UPB_STRARG(proto_name));
    return 1;
  }

  struct upb_msg *m = e->ref.msg;
  struct upb_msg_parse_state s;
  void *data = upb_msgdata_new(m);
  upb_msg_parse_init(&s, data, m, false, true);
  size_t read;
  struct upb_string str;
  if(!upb_strreadfile("google_message2.dat", &str)) {
    fprintf(stderr, "Error reading google_message2.dat\n");
    return 1;
  }
  upb_status_t status = upb_msg_parse(&s, str.ptr, str.byte_len, &read);
  upb_msg_parse_free(&s);
  upb_msgdata_free(data, m, true);
  upb_context_free(&c);
  upb_strfree(str);
  if(status == UPB_STATUS_OK && read == str.byte_len) {
    fprintf(stderr, "Success!\n");
    return 0;
  } else {
    fprintf(stderr, "Error. :(  error=%d, read=%d\n", status, read);
    return 1;
  }
}
