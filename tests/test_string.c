
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
  assert(memcmp(robuf, static_str, upb_string_len(str)) == 0);
  upb_string_endread(str);

  upb_string *str2 = upb_string_tryrecycle(str);
  // No other referents, so should return the same string.
  assert(str2 == str);

  // Write a shorter string, the same memory should be reused.
  upb_strcpyc(str, "XX");
  const char *robuf2 = upb_string_getrobuf(str);
  assert(robuf2 == robuf);
  assert(memcmp(robuf2, "XX", 2) == 0);

  // Make string alias part of another string.
  str2 = upb_strdupc("WXYZ");
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

  upb_string_unref(str);
  upb_string_unref(str2);
}
