
#include "upb_decoder.h"
#include "upb_textprinter.h"
#include "upb_stdio.h"

int main() {
  upb_symtab *symtab = upb_symtab_new();
  upb_symtab_add_descriptorproto(symtab);
  upb_def *fds = upb_symtab_lookup(
      symtab, UPB_STRLIT("google.protobuf.FileDescriptorSet"));

  upb_stdio *in = upb_stdio_new();
  upb_stdio_reset(in, stdin);
  upb_stdio *out = upb_stdio_new();
  upb_stdio_reset(out, stdout);
  upb_decoder *d = upb_decoder_new(upb_downcast_msgdef(fds));
  upb_decoder_reset(d, upb_stdio_bytesrc(in));
  upb_textprinter *p = upb_textprinter_new();
  upb_textprinter_reset(p, upb_stdio_bytesink(out), false);

  upb_status status = UPB_STATUS_INIT;
  upb_streamdata(upb_decoder_src(d), upb_textprinter_sink(p), &status);

  assert(upb_ok(&status));

  upb_stdio_free(in);
  upb_stdio_free(out);
  upb_decoder_free(d);
  upb_textprinter_free(p);
  upb_def_unref(fds);
  upb_symtab_unref(symtab);
}
