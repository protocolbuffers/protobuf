#include "t/c/upb-perl-test.h"
#include "xs/protobuf.h"
#include "xs/extension_dict/dict.h"
#include "xs/descriptor/field.h"
#include "xs/descriptor/message.h"

int main(int argc, char** argv) {
    PerlInterpreter *my_perl = test_perl_init(argc, argv);

    plan(8);

    // Mock MessageDef and FieldDef (Extension)
    // In a real test, we'd load these from a pool.
    SV* msg_sv = newSViv(1);
    sv_bless(newRV_noinc(msg_sv), gv_stashpv("Protobuf::Descriptor::MessageDef", GV_ADD));

    // 1. Creation
    SV* dict_sv = PerlUpb_ExtensionDict_New(aTHX_ msg_sv);
    ok(dict_sv != NULL, "PerlUpb_ExtensionDict_New returns non-NULL");
    ok(sv_derived_from(dict_sv, "Protobuf::ExtensionDict"), "Dict SV has correct class");

    // 2. Retrieval of message
    SV* msg_back = PerlUpb_ExtensionDict_GetMessageSV(aTHX_ dict_sv);
    ok(msg_back != NULL && SvRV(msg_back) == SvRV(msg_sv), "ExtensionDict returns correct parent message SV");

    // Cleanup
    extern void PerlUpb_ExtensionDict_Free(pTHX_ SV* sv);
    PerlUpb_ExtensionDict_Free(aTHX_ dict_sv);
    SvREFCNT_dec(dict_sv);
    SvREFCNT_dec(msg_sv);

    TODO {
        ok(0, "Descriptors for extension fields are only inflated on access");
    }

    TODO {
        ok(0, "Retrieving the same extension multiple times returns the same Perl SV");
    }

    TODO {
        ok(0, "Extension data can be safely moved between messages sharing an arena");
    }

    TODO Global Extension Index for ultra-fast extension resolution") {
        ok(0, "Per-interpreter cache allows near-instant extension descriptor retrieval");
    }

    TODO {
        ok(0, "Repeated access to the same extension returns the same Perl object instance");
    }

    test_perl_destroy(my_perl);
    return 0;
}
