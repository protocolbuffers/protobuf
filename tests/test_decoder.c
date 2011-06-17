
#include "upb_decoder.h"
#include "upb_textprinter.h"
#include "upb_stdio.h"
#include "upb_glue.h"

int main(int argc, char *argv[]) {
  if (argc < 3) {
    fprintf(stderr, "Usage: test_decoder <descfile> <msgname>\n");
    return 1;
  }

  upb_symtab *symtab = upb_symtab_new();
  upb_string *desc = upb_strreadfile(argv[1]);
  if (!desc) {
    fprintf(stderr, "Couldn't open descriptor file: %s\n", argv[1]);
    return 1;
  }

  upb_status status = UPB_STATUS_INIT;
  upb_read_descriptor(symtab, desc, &status);
  if (!upb_ok(&status)) {
    fprintf(stderr, "Error parsing descriptor: ");
    upb_printerr(&status);
    return 1;
  }
  upb_string_unref(desc);

  upb_string *name = upb_strdupc(argv[2]);
  upb_def *md = upb_symtab_lookup(symtab, name);
  upb_string_unref(name);
  if (!md) {
    fprintf(stderr, "Descriptor did not contain message: %s\n", argv[2]);
    return 1;
  }

  upb_msgdef *m = upb_dyncast_msgdef(md);
  if (!m) {
    fprintf(stderr, "Def was not a msgdef.\n");
    return 1;
  }

  upb_stdio *in = upb_stdio_new();
  upb_stdio_reset(in, stdin);
  upb_stdio *out = upb_stdio_new();
  upb_stdio_reset(out, stdout);

  upb_handlers *handlers = upb_handlers_new();
  upb_textprinter *p = upb_textprinter_new();
  upb_textprinter_reset(p, upb_stdio_bytesink(out), false);
  upb_textprinter_reghandlers(handlers, m);

  upb_decoder d;
  upb_decoder_initforhandlers(&d, handlers);
  upb_decoder_reset(&d, upb_stdio_bytesrc(in), p);

  upb_clearerr(&status);
  upb_decoder_decode(&d, &status);

  if (!upb_ok(&status)) {
    fprintf(stderr, "Error parsing input: ");
    upb_printerr(&status);
  }

  upb_status_uninit(&status);
  upb_stdio_free(in);
  upb_stdio_free(out);
  upb_decoder_uninit(&d);
  upb_textprinter_free(p);
  upb_def_unref(UPB_UPCAST(m));
  upb_symtab_unref(symtab);

  // Prevent C library from holding buffers open, so Valgrind doesn't see
  // memory leaks.
  fclose(stdin);
  fclose(stdout);
}
