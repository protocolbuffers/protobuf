PHP_ARG_ENABLE(protobuf, whether to enable Protobuf extension, [  --enable-protobuf   Enable Protobuf extension])

if test "$PHP_PROTOBUF" != "no"; then

  PHP_NEW_EXTENSION(
    protobuf,
    array.c def.c encode_decode.c map.c message.c protobuf.c storage.c type_check.c upb.c utf8.c,
    $ext_shared)

fi
