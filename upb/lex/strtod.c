// Protocol Buffers - Google's data interchange format
// Copyright 2023 Google LLC.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "upb/lex/strtod.h"

#include <stdlib.h>
#include <string.h>

// Must be last.
#include "upb/port/def.inc"

// Determine the locale-specific radix character by calling sprintf() to print
// the number 1.5, then stripping off the digits.  As far as I can tell, this
// is the only portable, thread-safe way to get the C library to divulge the
// locale's radix character.  No, localeconv() is NOT thread-safe.

static int GetLocaleRadix(char *data, size_t capacity) {
  char temp[16];
  const int size = snprintf(temp, sizeof(temp), "%.1f", 1.5);
  UPB_ASSERT(temp[0] == '1');
  UPB_ASSERT(temp[size - 1] == '5');
  UPB_ASSERT(size < capacity);
  temp[size - 1] = '\0';
  strcpy(data, temp + 1);
  return size - 2;
}

// Populates a string identical to *input except that the character pointed to
// by pos (which should be '.') is replaced with the locale-specific radix.

static void LocalizeRadix(const char *input, const char *pos, char *output) {
  const int len1 = pos - input;

  char radix[8];
  const int len2 = GetLocaleRadix(radix, sizeof(radix));

  memcpy(output, input, len1);
  memcpy(output + len1, radix, len2);
  strcpy(output + len1 + len2, input + len1 + 1);
}

double _upb_NoLocaleStrtod(const char *str, char **endptr) {
  // We cannot simply set the locale to "C" temporarily with setlocale()
  // as this is not thread-safe.  Instead, we try to parse in the current
  // locale first.  If parsing stops at a '.' character, then this is a
  // pretty good hint that we're actually in some other locale in which
  // '.' is not the radix character.

  char *temp_endptr;
  double result = strtod(str, &temp_endptr);
  if (endptr != NULL) *endptr = temp_endptr;
  if (*temp_endptr != '.') return result;

  // Parsing halted on a '.'.  Perhaps we're in a different locale?  Let's
  // try to replace the '.' with a locale-specific radix character and
  // try again.

  char localized[80];
  LocalizeRadix(str, temp_endptr, localized);
  char *localized_endptr;
  result = strtod(localized, &localized_endptr);
  if ((localized_endptr - &localized[0]) > (temp_endptr - str)) {
    // This attempt got further, so replacing the decimal must have helped.
    // Update endptr to point at the right location.
    if (endptr != NULL) {
      // size_diff is non-zero if the localized radix has multiple bytes.
      int size_diff = strlen(localized) - strlen(str);
      *endptr = (char *)str + (localized_endptr - &localized[0] - size_diff);
    }
  }

  return result;
}
