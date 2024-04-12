#!/bin/bash

set -ex

function get_source_version() {
  grep "__version__ = '.*'" python/google/protobuf/__init__.py | sed -r "s/__version__ = '(.*)'/\1/"
}

function run_install_test() {
  local VERSION=$1
  local PYTHON=$2
  local PYPI=$3

  virtualenv -p `which $PYTHON` test-venv

  # Intentionally put a broken protoc in the path to make sure installation
  # doesn't require protoc installed.
  touch test-venv/bin/protoc
  chmod +x test-venv/bin/protoc

  source test-venv/bin/activate
  (pip install -i ${PYPI} protobuf==${VERSION} --no-cache-dir) || (retry_pip_install ${PYPI} ${VERSION})
  deactivate
  rm -fr test-venv
}

function retry_pip_install() {
  local PYPI=$1
  local VERSION=$2
  
  read -p "pip install failed, possibly due to delay between upload and availability on pip. Retry? [y/n]" -r
  echo
  if [[ ! $REPLY =~ ^[Yy]$ ]]; then
    exit 1
  fi

  (pip install -i ${PYPI} protobuf==${VERSION} --no-cache-dir) || (retry_pip_install ${PYPI} ${VERSION})
}


[ $# -lt 1 ] && {
  echo "Usage: $0 VERSION ["
  echo ""
  echo "Examples:"
  echo "  Test 3.3.0 release using version number 3.3.0.dev1:"
  echo "    $0 3.0.0 dev1"
  echo "  Actually release 3.3.0 to PyPI:"
  echo "    $0 3.3.0"
  exit 1
}
VERSION=$1
DEV=$2

# Make sure we are in a protobuf source tree.
[ -f "python/google/protobuf/__init__.py" ] || {
  echo "This script must be ran under root of protobuf source tree."
  exit 1
}

# Make sure all files are world-readable.
find python -type d -exec chmod a+r,a+x {} +
find python -type f -exec chmod a+r {} +
umask 0022

# Check that the supplied version number matches what's inside the source code.
SOURCE_VERSION=`get_source_version`

[ "${VERSION}" == "${SOURCE_VERSION}" -o "${VERSION}.${DEV}" == "${SOURCE_VERSION}" ] || {
  echo "Version number specified on the command line ${VERSION} doesn't match"
  echo "the actual version number in the source code: ${SOURCE_VERSION}"
  exit 1
}

TESTING_ONLY=1
TESTING_VERSION=${VERSION}.${DEV}
if [ -z "${DEV}" ]; then
  read -p "You are releasing ${VERSION} to PyPI. Are you sure? [y/n]" -r
  echo
  if [[ ! $REPLY =~ ^[Yy]$ ]]; then
    exit 1
  fi
  TESTING_ONLY=0
  TESTING_VERSION=${VERSION}
else
  # Use dev version number for testing.
  sed -i -r "s/__version__ = '.*'/__version__ = '${VERSION}.${DEV}'/" python/google/protobuf/__init__.py
fi

# Copy LICENSE
cp LICENSE python/LICENSE

cd python

# Run tests locally.
python3 setup.py build
python3 setup.py test

# Deploy source package to testing PyPI
python3 setup.py sdist
twine upload --skip-existing -r testpypi -u protobuf-wheel-test dist/*

# Sleep to allow time for distribution to be available on pip.
sleep 5m

# Test locally.
run_install_test ${TESTING_VERSION} python3 https://test.pypi.org/simple

# Deploy egg/wheel packages to testing PyPI and test again.
python3 setup.py clean build bdist_wheel
twine upload --skip-existing -r testpypi -u protobuf-wheel-test dist/*
sleep 5m
run_install_test ${TESTING_VERSION} python3 https://test.pypi.org/simple

echo "All install tests have passed using testing PyPI."

if [ $TESTING_ONLY -eq 0 ]; then
  read -p "Publish to PyPI? [y/n]" -r
  echo
  if [[ ! $REPLY =~ ^[Yy]$ ]]; then
    exit 1
  fi
  echo "Publishing to PyPI..."
  # Be sure to run build before sdist, because otherwise sdist will not include
  # well-known types.
  python3 setup.py clean build sdist
  twine upload --skip-existing -u protobuf-packages dist/*
  # Be sure to run clean before bdist_xxx, because otherwise bdist_xxx will
  # include files you may not want in the package. E.g., if you have built
  # and tested with --cpp_implemenation, bdist_xxx will include the _message.so
  # file even when you no longer pass the --cpp_implemenation flag. See:
  #   https://github.com/protocolbuffers/protobuf/issues/3042
  python3 setup.py clean build bdist_wheel
  twine upload --skip-existing -u protobuf-packages dist/*
else
  # Set the version number back (i.e., remove dev suffix).
  sed -i -r "s/__version__ = '.*'/__version__ = '${VERSION}'/" google/protobuf/__init__.py
fi
