
#undef NDEBUG  /* ensure tests always assert. */
#include "upb_string.h"

char static_str[] = "Static string.";

int main() {
  upb_string *str = upb_string_new();
  assert(str != NULL);
  upb_string_unref(str);

  // Can also create a string by tryrecycle(NULL).
  str = upb_string_tryrecycle(NULL);
  assert(str != NULL);

  upb_strcpyc(str, static_str);
  assert(upb_string_len(str) == (sizeof(static_str) - 1));
  const char *robuf = upb_string_getrobuf(str);
  assert(robuf != NULL);
  assert(upb_streqlc(str, static_str));
  upb_string_endread(str);

  upb_string *str2 = upb_string_tryrecycle(str);
  // No other referents, so should return the same string.
  assert(str2 == str);

  // Write a shorter string, the same memory should be reused.
  upb_strcpyc(str, "XX");
  const char *robuf2 = upb_string_getrobuf(str);
  assert(robuf2 == robuf);
  assert(upb_streqlc(str, "XX"));

  // Make string alias part of another string.
  str2 = upb_strdupc("WXYZ");
  str = upb_string_tryrecycle(str);
  upb_string_substr(str, str2, 1, 2);
  assert(upb_string_len(str) == 2);
  assert(upb_string_len(str2) == 4);
  // The two string should be aliasing the same data.
  const char *robuf3 = upb_string_getrobuf(str);
  const char *robuf4 = upb_string_getrobuf(str2);
  assert(robuf3 == robuf4 + 1);
  // The aliased string should have an extra ref.
  assert(upb_atomic_read(&str2->refcount) == 2);

  // Recycling str should eliminate the extra ref.
  str = upb_string_tryrecycle(str);
  assert(upb_atomic_read(&str2->refcount) == 1);

  // Resetting str should reuse its old data.
  upb_strcpyc(str, "XX");
  const char *robuf5 = upb_string_getrobuf(str);
  assert(robuf5 == robuf);

  // Resetting str to something very long should require new data to be
  // allocated.
  str = upb_string_tryrecycle(str);
  const char longstring[] = "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX";
  upb_strcpyc(str, longstring);
  const char *robuf6 = upb_string_getrobuf(str);
  assert(robuf6 != robuf);
  assert(upb_streqlc(str, longstring));

  // Test printf.
  str = upb_string_tryrecycle(str);
  upb_string_printf(str, "Number: %d, String: %s", 5, "YO!");
  assert(upb_streqlc(str, "Number: 5, String: YO!"));

  // Test asprintf
  upb_string *str3 = upb_string_asprintf("Yo %s: " UPB_STRFMT "\n",
                                         "Josh", UPB_STRARG(str));
  const char expected[] = "Yo Josh: Number: 5, String: YO!\n";
  assert(upb_streqlc(str3, expected));

  upb_string_unref(str);
  upb_string_unref(str2);
  upb_string_unref(str3);

  // Unref of NULL is harmless.
  upb_string_unref(NULL);
}
