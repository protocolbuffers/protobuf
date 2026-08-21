"""Protobuf-specific kotlin build rules."""

load("//:protobuf_version.bzl", "PROTOBUF_JAVA_VERSION")
load("//java/osgi:kotlin_osgi.bzl", "osgi_kt_jvm_library")

BUNDLE_DOC_URL = "https://developers.google.com/protocol-buffers/"
BUNDLE_LICENSE = "https://opensource.org/licenses/BSD-3-Clause"

def protobuf_versioned_kt_jvm_library(
        automatic_module_name,
        bundle_description,
        bundle_name,
        bundle_symbolic_name,
        bundle_additional_imports = [],
        bundle_additional_exports = [],
        **kwargs):
    """Extends `kt_jvm_library` to add OSGi headers to the MANIFEST.MF using bndlib

    This macro should be usable as a drop-in replacement for kt_jvm_library.

    The additional arguments are given the bndlib tool to generate an OSGi-compliant manifest file.
    See [bnd documentation](https://bnd.bndtools.org/chapters/110-introduction.html)

    Takes all the args that are standard for a kt_jvm_library target plus the following.
    Args:
        bundle_description: (required) The Bundle-Description header defines a short
            description of this bundle.
        automatic_module_name: (required) The Automatic-Module-Name header that represents
            the name of the module when this bundle is used as an automatic
            module.
        bundle_name: (required) The Bundle-Name header defines a readable name for this
            bundle. This should be a short, human-readable name that can
            contain spaces.
        bundle_symbolic_name: (required) The Bundle-SymbolicName header specifies a
            non-localizable name for this bundle. The bundle symbolic name
            together with a version must identify a unique bundle though it can
            be installed multiple times in a framework. The bundle symbolic
            name should be based on the reverse domain name convention.
        bundle_additional_exports: The Export-Package header contains a
            declaration of exported packages. These are additional export
            package statements to be added before the default wildcard export
            "*;version={$Bundle-Version}".
        bundle_additional_imports: The Import-Package header declares the
            imported packages for this bundle. These are additional import
            package statements to be added before the default wildcard import
            "*".
        **kwargs: Additional key-word arguments that are passed to the internal
            kt_jvm_library target.
    """
    osgi_kt_jvm_library(
        automatic_module_name = automatic_module_name,
        bundle_doc_url = BUNDLE_DOC_URL,
        bundle_license = BUNDLE_LICENSE,
        bundle_version = PROTOBUF_JAVA_VERSION,
        bundle_description = bundle_description,
        bundle_name = bundle_name,
        bundle_symbolic_name = bundle_symbolic_name,
        bundle_additional_exports = bundle_additional_exports,
        bundle_additional_imports = bundle_additional_imports + ["sun.misc;resolution:=optional"],
        **kwargs
    )
