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

#ifndef GOOGLE_PROTOBUF_COMPILER_SCC_H__
#define GOOGLE_PROTOBUF_COMPILER_SCC_H__

#include <map>

#include <google/protobuf/descriptor.h>

namespace google {
namespace protobuf {
namespace compiler {

// Description of each strongly connected component. Note that the order
// of both the descriptors in this SCC and the order of children is
// deterministic.
struct SCC {
  std::vector<const Descriptor*> descriptors;
  std::vector<const SCC*> children;

  const Descriptor* GetRepresentative() const { return descriptors[0]; }
};

// This class is used for analyzing the SCC for each message, to ensure linear
// instead of quadratic performance, if we do this per message we would get
// O(V*(V+E)).
class LIBPROTOC_EXPORT SCCAnalyzer {
 public:
  explicit SCCAnalyzer() : index_(0) {}

  const SCC* GetSCC(const Descriptor* descriptor) {
    if (cache_.count(descriptor)) return cache_[descriptor].scc;
    return DFS(descriptor).scc;
  }

 private:
  struct NodeData {
    const SCC* scc;  // if null it means its still on the stack
    int index;
    int lowlink;
  };

  std::map<const Descriptor*, NodeData> cache_;
  std::vector<const Descriptor*> stack_;
  int index_;
  std::vector<std::unique_ptr<SCC>> garbage_bin_;

  SCC* CreateSCC() {
    garbage_bin_.emplace_back(new SCC());
    return garbage_bin_.back().get();
  }

  // Tarjan's Strongly Connected Components algo
  NodeData DFS(const Descriptor* descriptor);

  // Add the SCC's that are children of this SCC to its children.
  void AddChildren(SCC* scc);

  // This is necessary for compiler bug in msvc2015.
  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(SCCAnalyzer);
};

}  // namespace compiler
}  // namespace protobuf
}  // namespace google

#endif  // GOOGLE_PROTOBUF_COMPILER_SCC_H__
