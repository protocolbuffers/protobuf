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

#ifndef PHP_PROTOBUF_DEF_H_
#define PHP_PROTOBUF_DEF_H_

#include <php.h>

#include "php-upb.h"

// Initializes the Def module, which defines all of the descriptor classes.
void Def_ModuleInit();

// Creates a new DescriptorPool to wrap the given symtab, which must not be
// NULL.
void DescriptorPool_CreateWithSymbolTable(zval *zv, upb_symtab *symtab);

upb_symtab *DescriptorPool_GetSymbolTable();

// Returns true if the global descriptor pool already has the given filename.
bool DescriptorPool_HasFile(const char *filename);

// Adds the given descriptor with the given filename to the global pool.
void DescriptorPool_AddDescriptor(const char *filename, const char *data, int size);

typedef struct Descriptor {
  zend_object std;
  const upb_msgdef *msgdef;
  zend_class_entry *class_entry;
} Descriptor;

// Gets or creates a Descriptor* for the given class entry, upb_msgdef, or
// upb_fielddef. The returned Descriptor* will live for the entire request,
// so no ref is necessary to keep it alive. The caller does *not* own a ref
// on the returned object.
Descriptor* Descriptor_GetFromClassEntry(zend_class_entry *ce);
Descriptor* Descriptor_GetFromMessageDef(const upb_msgdef *m);
Descriptor* Descriptor_GetFromFieldDef(const upb_fielddef *f);

// Packages up a upb_fieldtype_t with a Descriptor, since many functions need
// both.
typedef struct {
  upb_fieldtype_t type;
  const Descriptor *desc;  // When type == UPB_TYPE_MESSAGE.
} TypeInfo;

static inline TypeInfo TypeInfo_Get(const upb_fielddef *f) {
  TypeInfo ret = {upb_fielddef_type(f), Descriptor_GetFromFieldDef(f)};
  return ret;
}

static inline TypeInfo TypeInfo_FromType(upb_fieldtype_t type) {
  TypeInfo ret = {type};
  return ret;
}

static inline bool TypeInfo_Eq(TypeInfo a, TypeInfo b) {
  if (a.type != b.type) return false;
  if (a.type == UPB_TYPE_MESSAGE && a.desc != b.desc) return false;
  return true;
}

#endif  // PHP_PROTOBUF_DEF_H_
