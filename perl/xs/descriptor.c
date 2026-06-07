#define PERL_NO_GET_CONTEXT
#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"
#include "xs/descriptor.h"

bool PerlUpb_InitDescriptor(pTHX_ SV* module) {
    // Initialization logic for all descriptor types
    // Will call init functions from base.c, message.c, etc.
    return true;
}
