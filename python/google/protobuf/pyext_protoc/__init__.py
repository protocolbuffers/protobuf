# TODO: If we receive bytes objects, decode them.
class ProtocError(Exception):
  def __init__(self, filename, line, column, message):
    self.filename = filename.decode("ascii") if isinstance(filename, bytes) else filename
    self.line = line
    self.column = column
    self.message = message.decode("ascii") if isinstance(filename, bytes) else message

  def __repr__(self):
    return "ProtocError(filename=\"{}\", line={}, column={}, message=\"{}\")".format(self.filename, self.line, self.column, self.message)

  def __str__(self):
    return "{}:{}:{} error: {}".format(self.filename, self.line, self.column, self.message)


class ProtocWarning(Warning):
  def __init__(self, filename, line, column, message):
    self.filename = filename.decode("ascii") if isinstance(filename, bytes) else filename
    self.line = line
    self.column = column
    self.message = message.decode("ascii") if isinstance(filename, bytes) else message

  def __repr__(self):
    return "ProtocWarning(filename=\"{}\", line={}, column={}, message=\"{}\")".format(self.filename, self.line, self.column, self.message)

  # TODO: Maybe come up with something better than this
  __str__ = __repr__


class ProtocErrors(Exception):
  def __init__(self, errors):
    self._errors = errors

  def errors(self):
    return self._errors

  def __repr__(self):
    return "ProtocErrors[{}]".join(repr(err) for err in self._errors)

  def __str__(self):
    return "\n".join(str(err) for err in self._errors)
