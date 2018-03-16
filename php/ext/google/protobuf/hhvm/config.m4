PHP_ARG_ENABLE(protobuf, whether to enable Protobuf extension, [  --enable-protobuf   Enable Protobuf extension])

if test "$PHP_PROTOBUF" != "no"; then
  PHP_REQUIRE_CXX()
  PHP_ADD_LIBRARY_WITH_PATH(stdc++, "", PROTOBUF_SHARED_LIBADD)
  CFLAGS+=" -Wall -Werror -std=c11 -g -O0"
  CXXFLAGS+=" -std=c++11 -fno-exceptions -fno-rtti -g -O0"

  PHP_SUBST(PROTOBUF_SHARED_LIBADD)
  PHP_NEW_EXTENSION(
    protobuf,
    protobuf.cpp upb.c arena_php.cpp arena.cpp array_php.cpp array.cpp def_php.cpp def.cpp map_php.cpp map.cpp message_php.cpp message.cpp type_check_php.cpp,
    $ext_shared)
fi
