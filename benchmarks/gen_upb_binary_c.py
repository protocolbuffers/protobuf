#!/usr/bin/python3
# Protocol Buffers - Google's data interchange format
# Copyright 2023 Google LLC.  All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

import sys
import re

include = sys.argv[1]
msg_basename = sys.argv[2]
count = 1

m = re.search(r'(.*\D)(\d+)$', sys.argv[2])
if m:
  msg_basename = m.group(1)
  count = int(m.group(2))

print('''
#include "{include}"

char buf[1];

int main() {{
  upb_Arena *arena = upb_Arena_New();
  size_t size;
'''.format(include=include))

def RefMessage(name):
  print('''
  {{
    {name} *proto = {name}_parse(buf, 1, arena);
    {name}_serialize(proto, arena, &size);
  }}
  '''.format(name=name))

RefMessage(msg_basename)

for i in range(2, count + 1):
  RefMessage(msg_basename + str(i))

print('''
  return 0;
}''')
