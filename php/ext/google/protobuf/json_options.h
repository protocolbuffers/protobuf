// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef PHP_PROTOBUF_JSON_OPTIONS_H_
#define PHP_PROTOBUF_JSON_OPTIONS_H_

// Define constants
#define JSON_SERIALIZE_OPTIONS_EMIT_DEFAULTS "EMIT_DEFAULTS"
#define JSON_SERIALIZE_KEY_EMIT_DEFAULTS "emit_defaults"

#define JSON_SERIALIZE_OPTIONS_PRESERVE_PROTO_FIELD_NAMES "PRESERVE_PROTO_FIELD_NAMES"
#define JSON_SERIALIZE_KEY_PRESERVE_PROTO_FIELD_NAMES "preserve_proto_field_names"
#include <php.h>

void JsonOptions_ModuleInit();

#endif  // PHP_PROTOBUF_DEF_H_
