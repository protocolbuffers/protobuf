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

#ifndef GOOGLE_PROTOBUF_COMPILER_OBJECTIVEC_FIELD_H__
#define GOOGLE_PROTOBUF_COMPILER_OBJECTIVEC_FIELD_H__

#include <map>
#include <string>
#include <google/protobuf/compiler/objectivec/objectivec_helpers.h>
#include <google/protobuf/stubs/common.h>
#include <google/protobuf/descriptor.h>

namespace google {
namespace protobuf {

namespace io {
class Printer;  // printer.h
}  // namespace io

namespace compiler {
namespace objectivec {

class FieldGenerator {
 public:
  static FieldGenerator* Make(const FieldDescriptor* field,
                              const Options& options);

  virtual ~FieldGenerator();

  // Exposed for subclasses to fill in.
  virtual void GenerateFieldStorageDeclaration(io::Printer* printer) const = 0;
  virtual void GeneratePropertyDeclaration(io::Printer* printer) const = 0;
  virtual void GeneratePropertyImplementation(io::Printer* printer) const = 0;

  // Called by GenerateFieldDescription, exposed for classes that need custom
  // generation.

  // Exposed for subclasses to extend, base does nothing.
  virtual void GenerateCFunctionDeclarations(io::Printer* printer) const;
  virtual void GenerateCFunctionImplementations(io::Printer* printer) const;

  // Exposed for subclasses, should always call it on the parent class also.
  virtual void DetermineForwardDeclarations(std::set<string>* fwd_decls) const;

  // Used during generation, not intended to be extended by subclasses.
  void GenerateFieldDescription(
      io::Printer* printer, bool include_default) const;
  void GenerateFieldNumberConstant(io::Printer* printer) const;

  // Exposed to get and set the has bits information.
  virtual bool RuntimeUsesHasBit(void) const = 0;
  void SetRuntimeHasBit(int has_index);
  void SetNoHasBit(void);
  virtual int ExtraRuntimeHasBitsNeeded(void) const;
  virtual void SetExtraRuntimeHasBitsBase(int index_base);
  void SetOneofIndexBase(int index_base);

  string variable(const char* key) const {
    return variables_.find(key)->second;
  }

  bool needs_textformat_name_support() const {
    const string& field_flags = variable("fieldflags");
    return field_flags.find("GPBFieldTextFormatNameCustom") != string::npos;
  }
  string generated_objc_name() const { return variable("name"); }
  string raw_field_name() const { return variable("raw_field_name"); }

 protected:
  FieldGenerator(const FieldDescriptor* descriptor, const Options& options);

  virtual void FinishInitialization(void);
  virtual bool WantsHasProperty(void) const = 0;

  const FieldDescriptor* descriptor_;
  std::map<string, string> variables_;

 private:
  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(FieldGenerator);
};

class SingleFieldGenerator : public FieldGenerator {
 public:
  virtual ~SingleFieldGenerator();

  virtual void GenerateFieldStorageDeclaration(io::Printer* printer) const;
  virtual void GeneratePropertyDeclaration(io::Printer* printer) const;

  virtual void GeneratePropertyImplementation(io::Printer* printer) const;

  virtual bool RuntimeUsesHasBit(void) const;

 protected:
  SingleFieldGenerator(const FieldDescriptor* descriptor,
                       const Options& options);
  virtual bool WantsHasProperty(void) const;

 private:
  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(SingleFieldGenerator);
};

// Subclass with common support for when the field ends up as an ObjC Object.
class ObjCObjFieldGenerator : public SingleFieldGenerator {
 public:
  virtual ~ObjCObjFieldGenerator();

  virtual void GenerateFieldStorageDeclaration(io::Printer* printer) const;
  virtual void GeneratePropertyDeclaration(io::Printer* printer) const;

 protected:
  ObjCObjFieldGenerator(const FieldDescriptor* descriptor,
                        const Options& options);

 private:
  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(ObjCObjFieldGenerator);
};

class RepeatedFieldGenerator : public ObjCObjFieldGenerator {
 public:
  virtual ~RepeatedFieldGenerator();

  virtual void GenerateFieldStorageDeclaration(io::Printer* printer) const;
  virtual void GeneratePropertyDeclaration(io::Printer* printer) const;

  virtual void GeneratePropertyImplementation(io::Printer* printer) const;

  virtual bool RuntimeUsesHasBit(void) const;

 protected:
  RepeatedFieldGenerator(const FieldDescriptor* descriptor,
                         const Options& options);
  virtual void FinishInitialization(void);
  virtual bool WantsHasProperty(void) const;

 private:
  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(RepeatedFieldGenerator);
};

// Convenience class which constructs FieldGenerators for a Descriptor.
class FieldGeneratorMap {
 public:
  FieldGeneratorMap(const Descriptor* descriptor, const Options& options);
  ~FieldGeneratorMap();

  const FieldGenerator& get(const FieldDescriptor* field) const;
  const FieldGenerator& get_extension(int index) const;

  // Assigns the has bits and returns the number of bits needed.
  int CalculateHasBits(void);

  void SetOneofIndexBase(int index_base);

  // Check if any field of this message has a non zero default.
  bool DoesAnyFieldHaveNonZeroDefault(void) const;

 private:
  const Descriptor* descriptor_;
  scoped_array<scoped_ptr<FieldGenerator> > field_generators_;
  scoped_array<scoped_ptr<FieldGenerator> > extension_generators_;

  GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(FieldGeneratorMap);
};
}  // namespace objectivec
}  // namespace compiler
}  // namespace protobuf
}  // namespace google
#endif  // GOOGLE_PROTOBUF_COMPILER_OBJECTIVEC_FIELD_H__
