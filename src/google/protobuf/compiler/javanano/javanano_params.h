// Protocol Buffers - Google's data interchange format
// Copyright 2010 Google Inc.  All rights reserved.
// http://code.google.com/p/protobuf/
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

// Author: wink@google.com (Wink Saville)

#ifndef PROTOBUF_COMPILER_JAVANANO_JAVANANO_PARAMS_H_
#define PROTOBUF_COMPILER_JAVANANO_JAVANANO_PARAMS_H_

#include <map>
#include <google/protobuf/stubs/strutil.h>

namespace google {
namespace protobuf {
namespace compiler {
namespace javanano {

// Parameters for used by the generators
class Params {
 public:
  typedef map<string, string> NameMap;
 private:
  string empty_;
  string base_name_;
  bool java_multiple_files_;
  NameMap java_packages_;
  NameMap java_outer_classnames_;

 public:
  Params(const string & base_name) :
    empty_(""),
    base_name_(base_name),
    java_multiple_files_(false) {
  }

  const string& base_name() const {
    return base_name_;
  }

  bool has_java_package(const string& file_name) const {
    return java_packages_.find(file_name)
                        != java_packages_.end();
  }
  void set_java_package(const string& file_name,
      const string& java_package) {
    java_packages_[file_name] = java_package;
  }
  const string& java_package(const string& file_name) const {
    NameMap::const_iterator itr;

    itr = java_packages_.find(file_name);
    if  (itr == java_packages_.end()) {
      return empty_;
    } else {
      return itr->second;
    }
  }
  const NameMap& java_packages() {
    return java_packages_;
  }

  bool has_java_outer_classname(const string& file_name) const {
    return java_outer_classnames_.find(file_name)
                        != java_outer_classnames_.end();
  }
  void set_java_outer_classname(const string& file_name,
      const string& java_outer_classname) {
    java_outer_classnames_[file_name] = java_outer_classname;
  }
  const string& java_outer_classname(const string& file_name) const {
    NameMap::const_iterator itr;

    itr = java_outer_classnames_.find(file_name);
    if  (itr == java_outer_classnames_.end()) {
      return empty_;
    } else {
      return itr->second;
    }
  }
  const NameMap& java_outer_classnames() {
    return java_outer_classnames_;
  }

  void set_java_multiple_files(bool value) {
    java_multiple_files_ = value;
  }
  bool java_multiple_files() const {
    return java_multiple_files_;
  }

};

}  // namespace javanano
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
#endif  // PROTOBUF_COMPILER_JAVANANO_JAVANANO_PARAMS_H_
