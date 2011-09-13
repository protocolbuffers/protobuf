/*
 * upb - a minimalist implementation of protocol buffers.
 *
 * Copyright (c) 2011 Google Inc.  See LICENSE for details.
 * Author: Josh Haberman <jhaberman@gmail.com>
 *
 * A simple example that demonstrates creating a standard message object
 * and parsing into it, using a dynamic reflection-based approach.
 *
 * Note that with this approach there are no strongly-typed struct or class
 * definitions to use from C -- this is essentially a reflection-based
 * interface.  Note that parsing and serializing are still very fast since
 * they are JIT-based.
 *
 * If this seems a bit verbose, you may prefer an approach that generates
 * strongly-typed struct definitions (upb will likely provide this, but it is
 * not yet implemented).
 *
 * TODO: make this file compiled as part of "make examples"
 * TODO: test that this actually works (WARNING: UNTESTED!)
 */

#include "upb/msg.h"
#include "upb/pb/glue.h"

const char *descfile = "example.proto.pb";
const char *type = "example.SampleMessage";
const char *msgfile = "sample_message.pb";

int main() {
  // First we load the descriptor that describes the message into a upb_msgdef.
  // This could come from a string that is compiled into the program or from a
  // separate file as we do here.  Since defs always live in a symtab, we
  // create one of those also.
  upb_symtab *s = upb_symtab_new();
  upb_status status = UPB_STATUS_INIT;
  if (!upb_load_descriptor_file_into_symtab(s, descfile, &status)) {
    fprintf(stderr, "Couldn't load descriptor file '%s': %s\n", descfile,
            upb_status_getstr(&status));
    return -1;
  }

  const upb_msgdef *md = upb_symtab_lookupmsg(s, type);
  if (!md) {
    fprintf(stderr, "Descriptor did not contain type '%s'\n", type);
    return -1;
  }

  // Parse a file into a new message object.
  void *msg = upb_filetonewmsg(msgfile, md, &status);
  if (!msg) {
    fprintf(stderr, "Error parsing message file '%s': %s\n", msgfile,
            upb_status_getstr(&status));
    return -1;
  }

  // TODO: Inspect some fields.
  return 0;
}
