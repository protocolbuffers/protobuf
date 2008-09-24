# Protocol Buffers - Google's data interchange format
# Copyright 2008 Google Inc.  All rights reserved.
# http://code.google.com/p/protobuf/
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

"""Contains routines for printing protocol messages in text format."""

__author__ = 'kenton@google.com (Kenton Varda)'

import cStringIO

from google.protobuf import descriptor

__all__ = [ 'MessageToString', 'PrintMessage', 'PrintField', 'PrintFieldValue' ]

def MessageToString(message):
  out = cStringIO.StringIO()
  PrintMessage(message, out)
  result = out.getvalue()
  out.close()
  return result

def PrintMessage(message, out, indent = 0):
  for field, value in message.ListFields():
    if field.label == descriptor.FieldDescriptor.LABEL_REPEATED:
      for element in value:
        PrintField(field, element, out, indent)
    else:
      PrintField(field, value, out, indent)

def PrintField(field, value, out, indent = 0):
  """Print a single field name/value pair.  For repeated fields, the value
  should be a single element."""

  out.write(' ' * indent);
  if field.is_extension:
    out.write('[')
    if (field.containing_type.GetOptions().message_set_wire_format and
        field.type == descriptor.FieldDescriptor.TYPE_MESSAGE and
        field.message_type == field.extension_scope and
        field.label == descriptor.FieldDescriptor.LABEL_OPTIONAL):
      out.write(field.message_type.full_name)
    else:
      out.write(field.full_name)
    out.write(']')
  elif field.type == descriptor.FieldDescriptor.TYPE_GROUP:
    # For groups, use the capitalized name.
    out.write(field.message_type.name)
  else:
    out.write(field.name)

  if field.cpp_type != descriptor.FieldDescriptor.CPPTYPE_MESSAGE:
    # The colon is optional in this case, but our cross-language golden files
    # don't include it.
    out.write(': ')

  PrintFieldValue(field, value, out, indent)
  out.write('\n')

def PrintFieldValue(field, value, out, indent = 0):
  """Print a single field value (not including name).  For repeated fields,
  the value should be a single element."""

  if field.cpp_type == descriptor.FieldDescriptor.CPPTYPE_MESSAGE:
    out.write(' {\n')
    PrintMessage(value, out, indent + 2)
    out.write(' ' * indent + '}')
  elif field.cpp_type == descriptor.FieldDescriptor.CPPTYPE_ENUM:
    out.write(field.enum_type.values_by_number[value].name)
  elif field.cpp_type == descriptor.FieldDescriptor.CPPTYPE_STRING:
    out.write('\"')
    out.write(_CEscape(value))
    out.write('\"')
  elif field.cpp_type == descriptor.FieldDescriptor.CPPTYPE_BOOL:
    if value:
      out.write("true")
    else:
      out.write("false")
  else:
    out.write(str(value))

# text.encode('string_escape') does not seem to satisfy our needs as it
# encodes unprintable characters using two-digit hex escapes whereas our
# C++ unescaping function allows hex escapes to be any length.  So,
# "\0011".encode('string_escape') ends up being "\\x011", which will be
# decoded in C++ as a single-character string with char code 0x11.
def _CEscape(text):
  def escape(c):
    o = ord(c)
    if o == 10: return r"\n"   # optional escape
    if o == 13: return r"\r"   # optional escape
    if o ==  9: return r"\t"   # optional escape
    if o == 39: return r"\'"   # optional escape

    if o == 34: return r'\"'   # necessary escape
    if o == 92: return r"\\"   # necessary escape

    if o >= 127 or o < 32: return "\\%03o" % o # necessary escapes
    return c
  return "".join([escape(c) for c in text])
