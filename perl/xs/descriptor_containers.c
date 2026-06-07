#define PERL_NO_GET_CONTEXT
#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"
#include "xs/descriptor_containers.h"

bool PerlUpb_InitDescriptorContainers(pTHX_ SV* module) {
    // Initialization logic for all container types
    return true;
}