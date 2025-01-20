// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef PHP_PROTOBUF_OPTIONS_H_
#define PHP_PROTOBUF_OPTIONS_H_

#define JSON_ENCODE_EMIT_DEFAULTS (1 << 0)
#define JSON_ENCODE_FORMAT_ENUMS_AS_INTEGERS (1 << 1)
#define JSON_ENCODE_PRESERVE_PROTO_FIELDNAMES (1 << 2)

// Registers the PHP Options class.
void Options_ModuleInit();

#endif  // PHP_PROTOBUF_OPTIONS_H_
