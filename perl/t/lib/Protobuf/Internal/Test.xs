#define PERL_NO_GET_CONTEXT
#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

MODULE = Protobuf::Internal::Test  PACKAGE = Protobuf::Internal::Test

PROTOTYPES: ENABLE

void
poke_char(IV address, IV offset, int value)
  CODE:
    char *ptr = (char*)address;
    ptr[offset] = (char)value;
    XSRETURN_EMPTY();

IV
peek_char(IV address, IV offset)
  CODE:
    char *ptr = (char*)address;
    RETVAL = (IV)ptr[offset];
  OUTPUT:
    RETVAL
