
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
'''.format(include=include))

def RefMessage(name):
  print('''
  {{
    {name} proto;
    proto.ParseFromArray(buf, 0);
    proto.SerializePartialToArray(&buf[0], 0);
  }}
  '''.format(name=name))

RefMessage(msg_basename)

for i in range(2, count + 1):
  RefMessage(msg_basename + str(i))

print('''
  return 0;
}''')
