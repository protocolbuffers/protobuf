PHP_ARG_ENABLE(protobuf, whether to enable Protobuf extension, [  --enable-protobuf   Enable Protobuf extension])

if test "$PHP_PROTOBUF" != "no"; then

  PHP_NEW_EXTENSION(
    protobuf,
    arena.c array.c convert.c def.c map.c message.c names.c php-upb.c protobuf.c third_party/utf8_range/naive.c third_party/utf8_range/range2-neon.c third_party/utf8_range/range2-sse.c,
    $ext_shared, , -std=gnu99)
  PHP_ADD_BUILD_DIR($ext_builddir/third_party/utf8_range)

fi
