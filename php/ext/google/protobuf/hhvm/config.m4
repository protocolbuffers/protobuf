PHP_ARG_ENABLE(protobuf, whether to enable Protobuf extension, [  --enable-protobuf   Enable Protobuf extension])

if test "$PHP_PROTOBUF" != "no"; then
  PHP_REQUIRE_CXX()
  PHP_NEW_EXTENSION(
    protobuf,
    protobuf.cpp def.cpp upb.c,
    $ext_shared)
fi
