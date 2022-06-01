#! /bin/sh

# This script takes the result of "make dist" and:
# 1) Unpacks it.
# 2) Ensures all contents are user-writable.  Some version control systems
#    keep code read-only until you explicitly ask to edit it, and the normal
#    "make dist" process does not correct for this, so the result is that
#    the entire dist is still marked read-only when unpacked, which is
#    annoying.  So, we fix it.
# 3) Convert MSVC project files to MSVC 2005, so that anyone who has version
#    2005 *or* 2008 can open them.  (In version control, we keep things in
#    MSVC 2008 format since that's what we use in development.)
# 4) Uses the result to create .tar.gz, .tar.bz2, and .zip versions and
#    deposits them in the "dist" directory.  In the .zip version, all
#    non-testdata .txt files are converted to Windows-style line endings.
# 5) Cleans up after itself.

if [ "$1" == "" ]; then
  echo "USAGE:  $0 DISTFILE" >&2
  exit 1
fi

if [ ! -e $1 ]; then
  echo $1": File not found." >&2
  exit 1
fi

set -ex

LANGUAGES="cpp csharp java objectivec python ruby php all"
BASENAME=`basename $1 .tar.gz`
VERSION=${BASENAME:9}

# Create a directory called "dist", copy the tarball there and unpack it.
mkdir dist
cp $1 dist
cd dist
tar zxvf $BASENAME.tar.gz
rm $BASENAME.tar.gz

# Set the entire contents to be user-writable.
chmod -R u+w $BASENAME
cd $BASENAME

for LANG in $LANGUAGES; do
  # Build the dist again in .tar.gz
  ./configure DIST_LANG=$LANG
  make dist-gzip
  mv $BASENAME.tar.gz ../protobuf-$LANG-$VERSION.tar.gz
done

# Convert all text files to use DOS-style line endings, then build a .zip
# distribution.
todos *.txt */*.txt

for LANG in $LANGUAGES; do
  # Build the dist again in .zip
  ./configure DIST_LANG=$LANG
  make dist-zip
  mv $BASENAME.zip ../protobuf-$LANG-$VERSION.zip
done

cd ..
rm -rf $BASENAME
