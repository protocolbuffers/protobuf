
#include <stdlib.h>
#include <string.h>
#include "google/protobuf/descriptor.upbdefs.h"
#include "upb/handlers.h"
#include "upb_test.h"

static bool startmsg(void *c, const void *hd) {
  UPB_UNUSED(c);
  UPB_UNUSED(hd);
  return true;
}

static void test_error() {
  /* Test creating handlers of a static msgdef. */
  upb_symtab *s = upb_symtab_new();
  const upb_msgdef *m = google_protobuf_DescriptorProto_getmsgdef(s);
  upb_handlers *h = upb_handlers_new(m, &h);

  /* Attempt to set the same handler twice causes error. */
  ASSERT(upb_ok(upb_handlers_status(h)));
  upb_handlers_setstartmsg(h, &startmsg, NULL);
  ASSERT(upb_ok(upb_handlers_status(h)));
  upb_handlers_setstartmsg(h, &startmsg, NULL);
  ASSERT(!upb_ok(upb_handlers_status(h)));
  ASSERT(!upb_handlers_freeze(&h, 1, NULL));

  /* Clearing the error will let us proceed. */
  upb_handlers_clearerr(h);
  ASSERT(upb_handlers_freeze(&h, 1, NULL));
  ASSERT(upb_handlers_isfrozen(h));

  upb_handlers_unref(h, &h);
  upb_symtab_free(s);
}

int run_tests(int argc, char *argv[]) {
  UPB_UNUSED(argc);
  UPB_UNUSED(argv);
  test_error();
  return 0;
}
