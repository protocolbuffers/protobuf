
#include "google/protobuf/empty.pb.h"

char buf[1];

int main() {
  google::protobuf::Empty proto;
  proto.ParseFromArray(buf, 1);
  proto.SerializeToArray(buf, 1);
}
