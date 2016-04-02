dnl lines starting with "dnl" are comments

PHP_ARG_ENABLE(protobuf, whether to enable Protobuf extension, [  --enable-protobuf   Enable Protobuf extension])

if test "$PHP_PROTOBUF" != "no"; then

  dnl this defines the extension
  PHP_NEW_EXTENSION(protobuf, upb.c protobuf.c def.c message.c storage.c, $ext_shared)

fi
