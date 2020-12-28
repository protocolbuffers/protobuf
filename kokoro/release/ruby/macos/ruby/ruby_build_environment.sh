#!/bin/bash

set -ex

set +ex  # rvm script is very verbose and exits with errorcode
source $HOME/.rvm/scripts/rvm
set -e  # rvm commands are very verbose
time rvm install 2.5.0
rvm use 2.5.0
gem install rake-compiler --no-document
gem install bundler --no-document
time rvm install 2.7.0
rvm use 2.7.0 --default
gem install rake-compiler --no-document
gem install bundler --no-document
rvm osx-ssl-certs status all
rvm osx-ssl-certs update all
set -ex

rm -rf ~/.rake-compiler

CROSS_RUBY=$(mktemp tmpfile.XXXXXXXX)

curl https://raw.githubusercontent.com/rake-compiler/rake-compiler/72184e51779b6a3b9b8580b036a052fdc3181ced/tasks/bin/cross-ruby.rake > "$CROSS_RUBY"

# See https://github.com/grpc/grpc/issues/12161 for verconf.h patch details
patch "$CROSS_RUBY" << EOF
--- cross-ruby.rake	2020-12-11 11:17:53.000000000 +0900
+++ patched	2020-12-11 11:18:52.000000000 +0900
@@ -111,10 +111,12 @@
     "--host=#{MINGW_HOST}",
     "--target=#{MINGW_TARGET}",
     "--build=#{RUBY_BUILD}",
-    '--enable-shared',
+    '--enable-static',
+    '--disable-shared',
     '--disable-install-doc',
+    '--without-gmp',
     '--with-ext=',
-    'LDFLAGS=-pipe -s',
+    'LDFLAGS=-pipe',
   ]

   # Force Winsock2 for Ruby 1.8, 1.9 defaults to it
@@ -130,6 +132,7 @@
 # make
 file "#{build_dir}/ruby.exe" => ["#{build_dir}/Makefile"] do |t|
   chdir File.dirname(t.prerequisites.first) do
+    sh "test -s verconf.h || rm -f verconf.h"  # if verconf.h has size 0, make sure it gets re-built by make
     sh MAKE
   end
 end
EOF

MAKE="make -j8"

set +x # rvm commands are very verbose
rvm use 2.7.0
set -x
ruby --version | grep 'ruby 2.7.0'
for v in 3.0.0 2.7.0 ; do
  ccache -c
  rake -f "$CROSS_RUBY" cross-ruby VERSION="$v" HOST=x86_64-darwin11 MAKE="$MAKE"
done
set +x
rvm use 2.5.0
set -x
ruby --version | grep 'ruby 2.5.0'
for v in 2.6.0 2.5.1 2.4.0 2.3.0; do
  ccache -c
  rake -f "$CROSS_RUBY" cross-ruby VERSION="$v" HOST=x86_64-darwin11 MAKE="$MAKE"
done
set +x
rvm use 2.7.0
set -x

sed 's/x86_64-darwin-11/universal-darwin/' ~/.rake-compiler/config.yml > "$CROSS_RUBY"
mv "$CROSS_RUBY" ~/.rake-compiler/config.yml
