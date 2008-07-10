# Protocol Buffers - Google's data interchange format
# Copyright 2008 Google Inc.
# http://code.google.com/p/protobuf/
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

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
