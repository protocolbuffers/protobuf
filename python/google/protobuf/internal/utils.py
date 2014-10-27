"""
This module abstracts all the bytes<-->string conversions so that the python 2
and 3 code everywhere else is similar.  This also has a few simple functions
that deal with the fact that bytes are different between the 2 versions even
when using from __future__ import unicode_literals.  For example:
Python 2:
    b'mybytes'[0] --> b'm' (type str)
Python 3:
    b'mybytes'[0] --> 109 (type int)
So in some places we get an index of a bytes object and we have to make sure
the behaviour is the same in both versions of python so functions here take
care of that.
"""

import sys

PY2 = sys.version_info[0] == 2
PY3 = sys.version_info[0] == 3

try:
  import cmp
except ImportError:
  #No cmp function available, probably Python 3
  def cmp(a, b):
    return (a > b) - (a < b)

try:
  callable = callable
except NameError:
  def callable(obj):
    return any("__call__" in klass.__dict__ for klass in type(obj).__mro__)

def string_to_bytestr(string):
  """
  Convert a string to a bytes object.  This encodes the object as well which
  will typically change ord on each element & change the length (i.e. 1 char
  could become 1/2/3 bytes)
  """
  return string.encode('utf-8')

if sys.version_info >= (3,):
  #some constants that are python2 only
  unicode = str
  long = int
  range = range
  unichr = chr

  def b(s):
    return s.encode("latin-1")

  def u(s):
    return s

  def iteritems(d):
    return d.items()

  from io import BytesIO as SimIO

  def string_to_bytes(text):
    """
    Convert a string to a bytes object.  This is a raw conversion
    so that ord() on each element remains unchanged.
    Input type: string
    Output type: bytes
    """
    return bytes([ord(c) for c in text])

  def bytes_to_string(byte_array):
    """
    Inverse of string_to_bytes.
    """
    return u('').join([chr(b) for b in byte_array])

  def string_to_bytestr(string):
    """
    Convert a string to a bytes object.  This encodes the object as well which
    will typically change ord on each element & change the length (i.e. 1 char
    could become 1/2/3 bytes)
    """
    return string.encode('utf-8')

  def bytestr_to_string(bytestr):
    """
    Inverse of string_to_bytestr.
    """
    return bytes([c for c in bytestr]).decode('utf-8')

  def byte_chr(bytes_str):
    """
    This converts a *single* input byte to a bytes object.  Usually used in
    conjuction with b'mybytes'[i].  See module description.
    Input: 2: string/pseudo-bytes 3: int
    Output: bytes
    """
    return bytes([bytes_str])

  def bytestr(val):
    """
    Convert a *single* integer to a bytes object.  Usually used like
    bytestr(int).
    Input: int
    Output: bytes
    """
    return bytes([val])

  def bytes_as_num(val):
    """
    Python 2:
        b'mybytes'[0] --> b'm' (type str)
    Python 3:
        b'mybytes'[0] --> 109 (type int)
    Given a number, returns it. For Python2, we use ord for the conversion.
    """
    return val

  def num_as_byte(val):
    """
    Converts an element of a byte string to a byte, effectively an inverse of bytes_as_num
    """
    return bytes([val])
else:
  #some constants that are python2 only
  range = xrange
  unicode = unicode
  long = long
  unichr = unichr

  def b(s):
    return s

  # Workaround for standalone backslash
  def u(s):
    return unicode(s.replace(r'\\', r'\\\\'), "unicode_escape")

  def iteritems(d):
    return d.iteritems()

  try:
    from cStringIO import StringIO as SimIO
  except:
    from StringIO import StringIO as SimIO

  def string_to_bytes(text):
    """
    See other implementation for notes
    """
    return "".join([c for c in text])

  def bytes_to_string(byte_array):
    """
    See other implementation for notes
    """
    return ''.join([b for b in byte_array])

  def bytestr_to_string(bytestr):
    """
    See other implementation for notes
    """
    return unicode(bytestr, 'utf-8')

  def byte_chr(bytes_str):
    """
    See other implementation for notes
    """
    return bytes_str

  def bytestr(val):
    """
    See other implementation for notes
    """
    return chr(val)

  def bytes_as_num(val):
    """
    Python 2:
        b'mybytes'[0] --> b'm' (type str)
    Python 3:
        b'mybytes'[0] --> 109 (type int)
    Given a number, returns it. For Python2, we use ord for the conversion.
    """
    return ord(val)

  def num_as_byte(val):
    """
    Converts an element of a byte string to a byte, effectively an inverse of bytes_as_num
    """
    return val


if sys.version_info >= (2, 6):
  bytes = bytes
else:
  bytes = str

if sys.version_info >= (3, 0):
  import builtins
  print_ = getattr(builtins, 'print', None)
elif sys.version_info >= (2, 6):
  import __builtin__
  print_ = getattr(__builtin__, 'print', None)
else:
  def print_(*args, **kwargs):
    """The new-style print function for Python 2.4 and 2.5."""
    fp = kwargs.pop("file", sys.stdout)
    if fp is None:
      return
    def write(data):
      if not isinstance(data, basestring):
        data = str(data)
      # If the file has an encoding, encode unicode with it.
      if (isinstance(fp, file) and
            isinstance(data, unicode) and
              fp.encoding is not None):
        errors = getattr(fp, "errors", None)
        if errors is None:
          errors = "strict"
        data = data.encode(fp.encoding, errors)
      fp.write(data)
    want_unicode = False
    sep = kwargs.pop("sep", None)
    if sep is not None:
      if isinstance(sep, unicode):
        want_unicode = True
      elif not isinstance(sep, str):
        raise TypeError("sep must be None or a string")
    end = kwargs.pop("end", None)
    if end is not None:
      if isinstance(end, unicode):
        want_unicode = True
      elif not isinstance(end, str):
        raise TypeError("end must be None or a string")
    if kwargs:
      raise TypeError("invalid keyword arguments to print()")
    if not want_unicode:
      for arg in args:
        if isinstance(arg, unicode):
          want_unicode = True
          break
    if want_unicode:
      newline = unicode("\n")
      space = unicode(" ")
    else:
      newline = "\n"
      space = " "
    if sep is None:
      sep = space
    if end is None:
      end = newline
    for i, arg in enumerate(args):
      if i:
        write(sep)
      write(arg)
    write(end)