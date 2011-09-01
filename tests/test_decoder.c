
#include <stdlib.h>
#include "upb/bytestream.h"
#include "upb/pb/decoder.h"
#include "upb/pb/glue.h"
#include "upb/pb/textprinter.h"

int main(int argc, char *argv[]) {
  if (argc < 3) {
    fprintf(stderr, "Usage: test_decoder <descfile> <msgname>\n");
    return 1;
  }

  upb_symtab *symtab = upb_symtab_new();
  size_t desc_len;
  const char *desc = upb_readfile(argv[1], &desc_len);
  if (!desc) {
    fprintf(stderr, "Couldn't open descriptor file: %s\n", argv[1]);
    return 1;
  }

  upb_status status = UPB_STATUS_INIT;
  upb_read_descriptor(symtab, desc, desc_len, &status);
  if (!upb_ok(&status)) {
    fprintf(stderr, "Error parsing descriptor: %s", upb_status_getstr(&status));
    return 1;
  }
  free((void*)desc);

  upb_def *md = upb_symtab_lookup(symtab, argv[2]);
  if (!md) {
    fprintf(stderr, "Descriptor did not contain message: %s\n", argv[2]);
    return 1;
  }

  upb_msgdef *m = upb_dyncast_msgdef(md);
  if (!m) {
    fprintf(stderr, "Def was not a msgdef.\n");
    return 1;
  }

  upb_stdio in, out;
  upb_stdio_init(&in);
  upb_stdio_init(&out);
  upb_stdio_reset(&in, stdin);
  upb_stdio_reset(&out, stdout);

  upb_handlers *handlers = upb_handlers_new();
  upb_textprinter *p = upb_textprinter_new();
  upb_textprinter_reset(p, upb_stdio_bytesink(&out), false);
  upb_textprinter_reghandlers(handlers, m);

  upb_decoder d;
  upb_decoder_init(&d, handlers);
  upb_decoder_reset(&d, upb_stdio_bytesrc(&in), 0, UPB_NONDELIMITED, p);

  upb_status_clear(&status);
  upb_decoder_decode(&d, &status);

  if (!upb_ok(&status)) {
    fprintf(stderr, "Error parsing input: %s", upb_status_getstr(&status));
  }

  upb_status_uninit(&status);
  upb_stdio_uninit(&in);
  upb_stdio_uninit(&out);
  upb_decoder_uninit(&d);
  upb_textprinter_free(p);
  upb_def_unref(UPB_UPCAST(m));
  upb_symtab_unref(symtab);

  // Prevent C library from holding buffers open, so Valgrind doesn't see
  // memory leaks.
  fclose(stdin);
  fclose(stdout);
}
