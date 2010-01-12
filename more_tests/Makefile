# Additional tests to run before releasing a package.
#
# Run like:
#   make PACKAGE=/path/to/protobuf-VERSION.tar.gz
#
# Some of these tests require tools or make assumptions that may not be
# available on end-user machines, so these cannot be part of "make check".  For
# example, we test that the headers compile with strict warning settings, but
# since different compilers produce wildly different warnings we cannot assume
# that this test will pass everywhere.  If we ran it as part of "make check",
# it could unnecessarily block users from running the real tests just because
# their compiler produces some extra warnings that probably aren't a big deal.
# So we run it separately. 

all: header_warning_test

clean:
	rm -rf src target header_warning_test.cc header_warning_test.o header_warning_test

# Unpack the package into src, then install it into target.
PACKAGE=protobuf.tar.gz

src: $(PACKAGE)
	tar zxvf $(PACKAGE)
	mv `basename $(PACKAGE) .tar.gz` src

target: src
	(cd src && ./configure --prefix=$$PWD/../target --disable-shared)
	(cd src && make -j4 check)
	(cd src && make install)

# Verify that headers produce no warnings even under strict settings.
header_warning_test.cc: target
	( (cd target/include && find google/protobuf -name '*.h') | \
	  awk '{print "#include \""$$1"\""} ' > header_warning_test.cc )

header_warning_test: header_warning_test.cc
	# TODO(kenton):  Consider adding -pedantic and -Weffc++.  Currently these
	#   produce tons of extra warnings so we'll need to do some work first.
	g++ -Itarget/include -Wall -Werror -Wsign-compare -O2 -c header_warning_test.cc
	touch header_warning_test
