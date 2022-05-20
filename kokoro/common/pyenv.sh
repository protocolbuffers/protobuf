# Shared logic to choose a Python version with pyenv.
#
# This file should be `source`d.

# Requested version of Python can be overridden by env variable.
: ${PYTHON_VERSION:=3.9.5}

if pyenv --version >/dev/null ; then
  eval "$(pyenv init -)"
  if ! pyenv global ${PYTHON_VERSION}; then
    echo "Python ${PYTHON_VERSION} is not available. Versions available:" >&2
    pyenv versions >&2
    exit 1
  fi
fi
echo "Using $(python --version || python3 --version)"
