# Shared logic to choose a Python version with pyenv.
#
# This file should be `source`d.

if pyenv --version >/dev/null ; then
  eval "$(pyenv init -)"
  pyenv global 3.9.5
fi
python --version || python3 --version
