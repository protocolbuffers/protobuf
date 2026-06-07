#include <sys/types.h>
#include <setjmp.h>
#include <stdlib.h>

#include "t/c/upb-perl-test.h"
#include "xs/descriptor_containers/generic_sequence.h"
#include <stdio.h>

// Mock data
int mock_items[] = {100, 200, 300};

int mock_count(const void* parent) { return 3; }
const void* mock_get(const void* parent, int i) {
    if (i < 0 || i >= 3) return NULL;
    return &mock_items[i];
}
SV* mock_wrap(pTHX_ const void* item) {
    int* val = (int*)item;
    return newSViv(*val);
}

static const PerlUpb_GenericSequence_VTable mock_vtable = {
    mock_count, mock_get, mock_wrap
};

int main(int argc, char** argv) {
    PerlInterpreter *my_perl = test_perl_init(argc, argv);

    plan(5);

    SV* parent_sv = newSViv(1);
    SV* seq_sv = PerlUpb_GenericSequence_New(aTHX_ parent_sv, NULL, &mock_vtable);
    ok(seq_sv != NULL, "Created GenericSequence");
    ok(sv_derived_from(seq_sv, "Protobuf::Internals::DescriptorSequence"), "Blessed correctly");

    is(PerlUpb_GenericSequence_Count(aTHX_ seq_sv), 3, "Count is 3");

    SV* val1 = PerlUpb_GenericSequence_GetItem(aTHX_ seq_sv, 1);
    ok(SvIOK(val1), "GetItem returns IOK");
    is(SvIV(val1), 200, "Value at index 1 is 200");
    SvREFCNT_dec(val1);

    // Cleanup
    extern void PerlUpb_GenericSequence_Free(pTHX_ SV* sv);
    PerlUpb_GenericSequence_Free(aTHX_ seq_sv);
    SvREFCNT_dec(seq_sv);
    SvREFCNT_dec(parent_sv);

    test_perl_destroy(my_perl);
    return 0;
}
