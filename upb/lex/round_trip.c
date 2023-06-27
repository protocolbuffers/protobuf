/*
 * Copyright (c) 2009-2021, Google LLC
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Google LLC nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL Google LLC BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "upb/lex/round_trip.h"

#include <float.h>
#include <stdlib.h>

// Must be last.
#include "upb/port/def.inc"

/* Miscellaneous utilities ****************************************************/

static void upb_FixLocale(char* p) {
  /* printf() is dependent on locales; sadly there is no easy and portable way
   * to avoid this. This little post-processing step will translate 1,2 -> 1.2
   * since JSON needs the latter. Arguably a hack, but it is simple and the
   * alternatives are far more complicated, platform-dependent, and/or larger
   * in code size. */
  for (; *p; p++) {
    if (*p == ',') *p = '.';
  }
}

void _upb_EncodeRoundTripDouble(double val, char* buf, size_t size) {
  assert(size >= kUpb_RoundTripBufferSize);
  snprintf(buf, size, "%.*g", DBL_DIG, val);
  if (strtod(buf, NULL) != val) {
    snprintf(buf, size, "%.*g", DBL_DIG + 2, val);
    assert(strtod(buf, NULL) == val);
  }
  upb_FixLocale(buf);
}

void _upb_EncodeRoundTripFloat(float val, char* buf, size_t size) {
  assert(size >= kUpb_RoundTripBufferSize);
  snprintf(buf, size, "%.*g", FLT_DIG, val);
  if (strtof(buf, NULL) != val) {
    snprintf(buf, size, "%.*g", FLT_DIG + 3, val);
    assert(strtof(buf, NULL) == val);
  }
  upb_FixLocale(buf);
}
