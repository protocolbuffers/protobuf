PHP_ARG_ENABLE(protobuf, whether to enable Protobuf extension, [  --enable-protobuf   Enable Protobuf extension])

if test "$PHP_PROTOBUF" != "no"; then

  PHP_NEW_EXTENSION(
    protobuf,
    arena.c array.c bundled_php.c convert.c def.c map.c message.c names.c php-upb.c protobuf.c,
    $ext_shared)

fi
