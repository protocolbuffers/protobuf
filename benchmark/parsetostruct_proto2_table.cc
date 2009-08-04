
#include "main.c"
#include <google/protobuf/dynamic_message.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include MESSAGE_HFILE

static std::string str;
static google::protobuf::DynamicMessageFactory factory;
static google::protobuf::Message *msg;

static bool initialize()
{
  /* Read the message data itself. */
  std::ifstream stream(MESSAGE_FILE);
  if(!stream.is_open()) {
    fprintf(stderr, "Error opening " MESSAGE_FILE ".\n");
    return false;
  }
  std::stringstream stringstream;
  stringstream << stream.rdbuf();
  str = stringstream.str();

  /* Create the DynamicMessage. */
  const google::protobuf::Message *dynamic_msg_prototype =
      factory.GetPrototype(MESSAGE_CIDENT::descriptor());
  msg = dynamic_msg_prototype->New();
  return true;
}

static void cleanup()
{
  delete msg;
}

static size_t run()
{
  if(!msg->ParseFromString(str)) {
    fprintf(stderr, "Error parsing with proto2.\n");
    return 0;
  }
  return str.size();
}
