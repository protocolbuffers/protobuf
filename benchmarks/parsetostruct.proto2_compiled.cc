
#include "main.c"
#include MESSAGE_HFILE
#include <string>
#include <iostream>
#include <sstream>
#include <fstream>

static std::string str;
MESSAGE_CIDENT msg[NUM_MESSAGES];

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
  return true;
}

static void cleanup()
{
}

static size_t run(int i)
{
  if(!msg[i%NUM_MESSAGES].ParseFromString(str)) {
    fprintf(stderr, "Error parsing with proto2.\n");
    return 0;
  }
  return str.size();
}
