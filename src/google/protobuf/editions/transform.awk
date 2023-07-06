# Protocol Buffers - Google's data interchange format
# Copyright 2023 Google Inc.  All rights reserved.
# https://developers.google.com/protocol-buffers/
# 
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
# 
#     * Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above
# copyright notice, this list of conditions and the following disclaimer
# in the documentation and/or other materials provided with the
# distribution.
#     * Neither the name of Google Inc. nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
# 
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

noop = 0

function join(array, len, sep)
{
    if(len== 0) {
      return ""
    }
    result = array[1]
    for (i = 2; i <= len; i++)
        result = result sep array[i]
    return result
}

function strip_option(option, options_list)
{
  # First try to strip out matching commas
  sub("\\<" option "\\s*,", "", options_list)
  sub(",\\s*" option "\\>", "", options_list)
  # Fallback to just stripping the option
  sub(option, "", options_list)
  return options_list
}

function transform_field(field)
{
  if (!match(field, /\w+\s*=\s*[0-9-]+/)) {
    return field
  }
  if (match(field, /(.*[0-9])\s*\[(.*)\];(.*)/, arr)) {
    field_def = arr[1]
    existing_options = arr[2]
    field_comments = arr[3]
  } else {
    match(field, /(.*);(.*)/, arr)
    field_def = arr[1]
    field_comments = arr[2]
    existing_options = 0
  }
  num_options = 0

  if(syntax == 2) {
    sub(/\<optional\s*/, "", field_def)
    sub(/\<packed = true\>/, "features.repeated_field_encoding = PACKED", existing_options)
    existing_options = strip_option("packed = false", existing_options)
    existing_options = strip_option("enforce_utf8 = (true|false)", existing_options)
    if (match(field_def, /^\s*required\>/)) {
      sub(/\<required\s*/, "", field_def)
      options[++num_options] = "features.field_presence = LEGACY_REQUIRED"
    }
    if (disable_utf8 && match(field_def, /^\s*(string|repeated\s*string|map<string,\s*string>)/)) {
      options[++num_options] = "features.string_field_validation = NONE"
    }
  }

  if(syntax == 3) {
    if (disable_utf8 && match(field_def, /^\s*(string|repeated\s*string|map<string,\s*string>)/)) {
      options[++num_options] = "features.string_field_validation = NONE"
    } else {
      sub(/\<enforce_utf8 = false\>/, "features.string_field_validation = HINT", existing_options)
    }
    sub(/\<packed = false\>/, "features.repeated_field_encoding = EXPANDED", existing_options)
    existing_options = strip_option("packed = true", existing_options)
    existing_options = strip_option("enforce_utf8 = (true|false)", existing_options)
    if (match($0, /\<optional\>/)) {
      sub(/\<optional /, "", field_def)
      options[++num_options] = "features.field_presence = EXPLICIT"
    }
  }

  if (existing_options && num_options > 0) {
    ret = field_def " [" existing_options ", " join(options, num_options, ",") "];" field_comments
  } else if (existing_options) {
    ret = field_def " [" existing_options "];" field_comments
  } else if(num_options > 0) {
    ret = field_def " [" join(options, num_options, ",") "];" field_comments
  } else {
    ret = field_def ";" field_comments
  }
  delete options
  return ret
}

# Skip comments
/^\s*\/\// {
  print $0
  next
}

/syntax = "proto2"/ {
  print "edition = \"2023\";"
  print "import \"third_party/protobuf/cpp_features.proto\";"
  print "option features.enum_type = CLOSED;"
  print "option features.repeated_field_encoding = EXPANDED;"
  print "option features.string_field_validation = HINT;"
  print "option features.json_format = LEGACY_BEST_EFFORT;"
  print "option features.(pb.cpp).legacy_closed_enum = true;"
  syntax = 2
  next
}

/syntax = "proto3"/ {
  print "edition = \"2023\";"
  print "option features.field_presence = IMPLICIT;"
  syntax = 3
  next
}

# utf8 validation handling
/option (cc_utf8_verification\s*=\s*false)/ {
  disable_utf8 = 1
  # Strip this option and replace with feature setting.
  next;
}

# Group handling.
/\<group \w* = [0-9]* {/, /}/ {
  if (match($0, /\<group\>/)) {
    match($0, /(\s*)(\w*)\s*group\s*(\w*)\s*=\s*([0-9]*)\s*{(\s*[^}]*}?)/, arr)
    if (arr[0, "length"] == 0) {
      print("[ERROR] Invalid group match: ", $0)
      exit
    }
    current_group_whitespace = arr[1]
    current_group_presence = arr[2]
    current_group_name = arr[3]
    current_group_number = arr[4]
    current_group_extra = transform_field(arr[5])

    print current_group_whitespace "message", current_group_name, "{" current_group_extra
  } else if (match($0, /}/)) {
    print $0
  }
  if (match($0, /}/)) {
    $0 = current_group_whitespace current_group_presence " " current_group_name " " tolower(current_group_name) " = " current_group_number " [features.message_encoding = DELIMITED];"
  } else if (match($0, /\<group\>/)) {
    next
  }
}

# Skip declarations we don't transform.
/^\s*(message|enum|oneof|extend|service)\s+(\w|\.)*\s*{/ {
  noop = 1
}
/^\s*rpc\s+\w*\s*\(/ {
  noop = 1
}
/^\s*(}|\/\/|$)/ {
  noop = 1
}
/^\s*(extensions|package|reserved|import) .*;/ {
  noop = 1
}
/^\s*option\s+[a-zA-Z0-9._()]*\s*=.*;/ {
  noop = 1
}
/\s*extensions.* \[/, /\]/ {
  noop = 1
}
noop {
  print $0
  next
}

/./, /;/ {
  if (buffered_field != "") {
    sub(/\s+/, " ", $0)
  }
  buffered_field = (buffered_field $0)
  if (!match($0, /;/)) {
    next
  }
  print(transform_field(buffered_field))
  buffered_field = ""
}
