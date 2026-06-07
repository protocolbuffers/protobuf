#define PERL_NO_GET_CONTEXT
#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"
#include "xs/convert.h"

// This file can remain mostly empty, as the logic
// has been moved to the subfiles.
// It's kept for any potential common convert-related
// code that doesn't fit into upb_to_sv or sv_to_upb.