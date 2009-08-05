
import sys

benchmarks = {}
color_map = {'proto2_compiled': 'FF0000',
             'proto2_table': 'FF00FF',
             'upb_table_byref': '0000FF',
             'upb_table_byval': '00FF00'}
for line in sys.stdin:
  name, val = line.split(': ')
  components = name.split('_')
  benchmark = '_'.join(components[1:3])
  variant = '_'.join(components[3:])
  if benchmark not in benchmarks:
    benchmarks[benchmark] = []
  benchmarks[benchmark].append((variant, int(val)))

def encode(x):
  digits = (range(ord("A"), ord("Z")+1) + range(ord("a"), ord("z")+1) +
            range(ord("0"), ord("9")+1) + [ord("."), ord("-")])
  return chr(digits[x / 64]) + chr(digits[x % 64])

for benchmark, values in benchmarks.items():
  def cmp(a, b):
    return b[1] - a[1]
  values.sort(cmp)
  variants = [x[0] for x in values]
  values = [x[1] for x in values]
  scaling = 400
  encoded_values = [encode((x * 4096 / scaling) - 1) for x in values]
  legend = "chdl=%s" % ("|".join(variants))
  colors = "chco=%s" % ("|".join([color_map[x] for x in variants]))
  data = "chd=e:%s" % ("".join(encoded_values))
  url = "http://chart.apis.google.com/chart?cht=bhs&chs=500x200&chtt=%s+(MB/s)&chxt=x&chxr=0,0,%d&%s" % (benchmark, scaling, "&".join([legend, data, colors]))
  print url
