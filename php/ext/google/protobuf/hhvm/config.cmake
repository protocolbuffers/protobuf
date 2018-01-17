set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ggdb -O0")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -ggdb -O0")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -ggdb -O0")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ggdb -O0")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -ggdb -O0")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -ggdb -O0")

HHVM_EXTENSION(protobuf ext_protobuf.cpp
               def_hhvm.cpp def.cpp
               message_hhvm.cpp message.cpp
               type_check_hhvm.cpp
               array_hhvm.cpp array.cpp
               map_hhvm.cpp map.cpp
               upb.c)
HHVM_SYSTEMLIB(protobuf ext_protobuf.php)
