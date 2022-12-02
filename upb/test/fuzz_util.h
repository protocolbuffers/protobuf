/*
 * Copyright (c) 2009-2022, Google LLC
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

#ifndef UPB_TEST_FUZZ_UTIL_H_
#define UPB_TEST_FUZZ_UTIL_H_

#include <string>
#include <vector>

#include "upb/mini_table/extension_registry.h"
#include "upb/mini_table/types.h"

namespace upb {
namespace fuzz {

struct MiniTableFuzzInput {
  // MiniDescripotrs for N messages, in the format accepted by
  // upb_MiniTable_Build().
  std::vector<std::string> mini_descriptors;

  // MiniDescripotrs for N enums, in the format accepted by
  // upb_MiniTableEnum_Build().
  std::vector<std::string> enum_mini_descriptors;

  // A MiniDescriptor for N extensions, in the format accepted by
  // upb_MiniTableExtension_Build().
  std::string extensions;

  // Integer indexes into the message or enum mini tables lists.  These specify
  // which message or enum to use for each sub-message or enum field.  We mod
  // by the total number of enums or messages so that any link value can be
  // valid.
  std::vector<uint32_t> links;
};

// Builds an arbitrary mini table corresponding to the random data in `input`.
// This function should be capable of producing any mini table that can
// successfully build, and any topology of messages and enums (including
// cycles).
//
// As currently written, it effectively fuzzes the mini descriptor parser also,
// and can therefore trigger any bugs in that parser. To better isolate these
// two, we may want to change this implementation to use the mini descriptor
// builder API so we are producing mini descriptors in a known good format. That
// would mostly eliminate the chance of crashing the mini descriptor parser
// itself.
//
// TODO: maps.  If we give maps some space in the regular encoding instead of
// using a separate function, we could get that for free.
const upb_MiniTable* BuildMiniTable(const MiniTableFuzzInput& input,
                                    upb_ExtensionRegistry** exts,
                                    upb_Arena* arena);

}  // namespace fuzz
}  // namespace upb

#endif  // UPB_TEST_FUZZ_UTIL_H_
