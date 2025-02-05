// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef PHP_PROTOBUF_PRINT_OPTIONS_H_
#define PHP_PROTOBUF_PRINT_OPTIONS_H_

#define PRESERVE_PROTO_FIELD_NAMES (1 << 0)
#define ALWAYS_PRINT_ENUMS_AS_INTS (1 << 1)

// Registers the PHP PrintOptions class.
void PrintOptions_ModuleInit();

#endif  // PHP_PROTOBUF_PRINT_OPTIONS_H_
