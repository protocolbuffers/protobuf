#!/bin/bash

cd `dirname $0`
CXXFLAGS="-O3 -msse3 -I../src -I../descriptor -Wall"
CFLAGS="-std=c99 $CXXFLAGS"
set -e
set -v

gcc -DMESSAGE_NAME=\"benchmarks.SpeedMessage1\" \
    -DMESSAGE_DESCRIPTOR_FILE=\"google_messages.proto.pb\" \
    -DMESSAGE_FILE=\"google_message1.dat\" \
    -DBYREF=false \
    $CFLAGS \
    parsetostruct_upb_table.c -o b_parsetostruct_googlemessage1_upb_table_byval ../src/libupb.a

gcc -DMESSAGE_NAME=\"benchmarks.SpeedMessage1\" \
    -DMESSAGE_DESCRIPTOR_FILE=\"google_messages.proto.pb\" \
    -DMESSAGE_FILE=\"google_message1.dat\" \
    -DBYREF=true \
    $CFLAGS \
    parsetostruct_upb_table.c -o b_parsetostruct_googlemessage1_upb_table_byref ../src/libupb.a

gcc -DMESSAGE_NAME=\"benchmarks.SpeedMessage2\" \
    -DMESSAGE_DESCRIPTOR_FILE=\"google_messages.proto.pb\" \
    -DMESSAGE_FILE=\"google_message2.dat\" \
    -DBYREF=false \
    $CFLAGS \
    parsetostruct_upb_table.c -o b_parsetostruct_googlemessage2_upb_table_byval ../src/libupb.a

gcc -DMESSAGE_NAME=\"benchmarks.SpeedMessage2\" \
    -DMESSAGE_DESCRIPTOR_FILE=\"google_messages.proto.pb\" \
    -DMESSAGE_FILE=\"google_message2.dat\" \
    -DBYREF=true \
    $CFLAGS \
    parsetostruct_upb_table.c -o b_parsetostruct_googlemessage2_upb_table_byref ../src/libupb.a

g++ -DMESSAGE_CIDENT="benchmarks::SpeedMessage2" \
    -DMESSAGE_FILE=\"google_message2.dat\" \
    -DMESSAGE_HFILE=\"google_messages.pb.h\" \
    $CXXFLAGS \
    parsetostruct_proto2_table.cc -o b_parsetostruct_googlemessage2_proto2_table -lprotobuf -lpthread google_messages.pb.o

g++ -DMESSAGE_CIDENT="benchmarks::SpeedMessage2" \
    -DMESSAGE_FILE=\"google_message2.dat\" \
    -DMESSAGE_HFILE=\"google_messages.pb.h\" \
    $CXXFLAGS \
    parsetostruct_proto2_compiled.cc -o b_parsetostruct_googlemessage2_proto2_compiled -lprotobuf -lpthread google_messages.pb.o

g++ -DMESSAGE_CIDENT="benchmarks::SpeedMessage1" \
    -DMESSAGE_FILE=\"google_message1.dat\" \
    -DMESSAGE_HFILE=\"google_messages.pb.h\" \
    $CXXFLAGS \
    parsetostruct_proto2_table.cc -o b_parsetostruct_googlemessage1_proto2_table -lprotobuf -lpthread google_messages.pb.o

g++ -DMESSAGE_CIDENT="benchmarks::SpeedMessage1" \
    -DMESSAGE_FILE=\"google_message1.dat\" \
    -DMESSAGE_HFILE=\"google_messages.pb.h\" \
    $CXXFLAGS \
    parsetostruct_proto2_compiled.cc -o b_parsetostruct_googlemessage1_proto2_compiled -lprotobuf -lpthread google_messages.pb.o
