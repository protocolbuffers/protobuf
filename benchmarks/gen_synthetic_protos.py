
import sys
import random

base = sys.argv[1]

field_freqs = [
    (('bool', 'optional'), 8.321),
    (('bool', 'repeated'), 0.033),
    (('bytes', 'optional'), 0.809),
    (('bytes', 'repeated'), 0.065),
    (('double', 'optional'), 2.845),
    (('double', 'repeated'), 0.143),
    (('fixed32', 'optional'), 0.084),
    (('fixed32', 'repeated'), 0.012),
    (('fixed64', 'optional'), 0.204),
    (('fixed64', 'repeated'), 0.027),
    (('float', 'optional'), 2.355),
    (('float', 'repeated'), 0.132),
    (('int32', 'optional'), 6.717),
    (('int32', 'repeated'), 0.366),
    (('int64', 'optional'), 9.678),
    (('int64', 'repeated'), 0.425),
    (('sfixed32', 'optional'), 0.018),
    (('sfixed32', 'repeated'), 0.005),
    (('sfixed64', 'optional'), 0.022),
    (('sfixed64', 'repeated'), 0.005),
    (('sint32', 'optional'), 0.026),
    (('sint32', 'repeated'), 0.009),
    (('sint64', 'optional'), 0.018),
    (('sint64', 'repeated'), 0.006),
    (('string', 'optional'), 25.461),
    (('string', 'repeated'), 2.606),
    (('Enum', 'optional'), 6.16),
    (('Enum', 'repeated'), 0.576),
    (('Message', 'optional'), 22.472),
    (('Message', 'repeated'), 7.766),
    (('uint32', 'optional'), 1.289),
    (('uint32', 'repeated'), 0.051),
    (('uint64', 'optional'), 1.044),
    (('uint64', 'repeated'), 0.079),
]

population = [item[0] for item in field_freqs]
weights = [item[1] for item in field_freqs]

def choices(k):
  if sys.version_info >= (3, 6):
    return random.choices(population=population, weights=weights, k=k)
  else:
    print("WARNING: old Python version, field types are not properly weighted!")
    return [random.choice(population) for _ in range(k)]

with open(base + "/100_msgs.proto", "w") as f:
  f.write('syntax = "proto3";\n')
  f.write('package upb_benchmark;\n')
  f.write('message Message {}\n')
  for i in range(2, 101):
    f.write('message Message{i} {{}}\n'.format(i=i))

with open(base + "/200_msgs.proto", "w") as f:
  f.write('syntax = "proto3";\n')
  f.write('package upb_benchmark;\n')
  f.write('message Message {}\n')
  for i in range(2, 501):
    f.write('message Message{i} {{}}\n'.format(i=i))

with open(base + "/100_fields.proto", "w") as f:
  f.write('syntax = "proto2";\n')
  f.write('package upb_benchmark;\n')
  f.write('enum Enum { ZERO = 0; }\n')
  f.write('message Message {\n')
  i = 1
  random.seed(a=0, version=2)
  for field in choices(100):
    field_type, label = field
    f.write('  {label} {field_type} field{i} = {i};\n'.format(i=i, label=label, field_type=field_type))
    i += 1
  f.write('}\n')

with open(base + "/200_fields.proto", "w") as f:
  f.write('syntax = "proto2";\n')
  f.write('package upb_benchmark;\n')
  f.write('enum Enum { ZERO = 0; }\n')
  f.write('message Message {\n')
  i = 1
  random.seed(a=0, version=2)
  for field in choices(200):
    field_type, label = field
    f.write('  {label} {field_type} field{i} = {i};\n'.format(i=i, label=label,field_type=field_type))
    i += 1
  f.write('}\n')
