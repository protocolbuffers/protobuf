// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <google/protobuf/compiler/scc.h>

#include <google/protobuf/stubs/logging.h>
#include <google/protobuf/stubs/common.h>
#include <google/protobuf/descriptor.h>

namespace google {
namespace protobuf {
namespace compiler {

SCCAnalyzer::NodeData SCCAnalyzer::DFS(const Descriptor* descriptor) {
  // Must not have visited already.
  GOOGLE_DCHECK_EQ(cache_.count(descriptor), 0);

  // Mark visited by inserting in map.
  NodeData& result = cache_[descriptor];
  // Initialize data structures.
  result.index = result.lowlink = index_++;
  stack_.push_back(descriptor);

  // Recurse the fields / nodes in graph
  for (int i = 0; i < descriptor->field_count(); i++) {
    const Descriptor* child = descriptor->field(i)->message_type();
    if (child) {
      if (cache_.count(child) == 0) {
        // unexplored node
        NodeData child_data = DFS(child);
        result.lowlink = std::min(result.lowlink, child_data.lowlink);
      } else {
        NodeData child_data = cache_[child];
        if (child_data.scc == nullptr) {
          // Still in the stack_ so we found a back edge
          result.lowlink = std::min(result.lowlink, child_data.index);
        }
      }
    }
  }
  if (result.index == result.lowlink) {
    // This is the root of a strongly connected component
    SCC* scc = CreateSCC();
    while (true) {
      const Descriptor* scc_desc = stack_.back();
      scc->descriptors.push_back(scc_desc);
      // Remove from stack
      stack_.pop_back();
      cache_[scc_desc].scc = scc;

      if (scc_desc == descriptor) break;
    }

    // The order of descriptors is random and depends how this SCC was
    // discovered. In-order to ensure maximum stability we sort it by name.
    std::sort(scc->descriptors.begin(), scc->descriptors.end(),
              [](const Descriptor* a, const Descriptor* b) {
                return a->full_name() < b->full_name();
              });
    AddChildren(scc);
  }
  return result;
}

void SCCAnalyzer::AddChildren(SCC* scc) {
  std::set<const SCC*> seen;
  for (int i = 0; i < scc->descriptors.size(); i++) {
    const Descriptor* descriptor = scc->descriptors[i];
    for (int j = 0; j < descriptor->field_count(); j++) {
      const Descriptor* child_msg = descriptor->field(j)->message_type();
      if (child_msg) {
        const SCC* child = GetSCC(child_msg);
        if (child == scc) continue;
        if (seen.insert(child).second) {
          scc->children.push_back(child);
        }
      }
    }
  }
}

}  // namespace compiler
}  // namespace protobuf
}  // namespace google
