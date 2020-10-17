
#include <time.h>

#include "examples/bazel/foo.upb.h"

int main() {
  upb_arena *arena = upb_arena_new();
  Foo* foo = Foo_new(arena);
  const char greeting[] = "Hello, World!\n";

  Foo_set_time(foo, time(NULL));
  /* Warning: the proto will not copy this, the string data must outlive
   * the proto. */
  Foo_set_greeting(foo, upb_strview_makez(greeting));

  upb_arena_free(arena);
}
