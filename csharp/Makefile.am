pkgconfigdir = $(libdir)/pkgconfig

pkgconfig_DATA = google-pb.pc

noinst_SCRIPTS = Google.ProtocolBuffers.dll

Google.ProtocolBuffers.dll_sources = \
	$(srcdir)/ProtocolBuffers/AbstractBuilder.cs			\
	$(srcdir)/ProtocolBuffers/AbstractMessage.cs			\
	$(srcdir)/ProtocolBuffers/ByteString.cs				\
	$(srcdir)/ProtocolBuffers/CodedInputStream.cs			\
	$(srcdir)/ProtocolBuffers/CodedOutputStream.cs			\
	$(srcdir)/ProtocolBuffers/Collections/Dictionaries.cs		\
	$(srcdir)/ProtocolBuffers/Collections/Lists.cs			\
	$(srcdir)/ProtocolBuffers/Collections/ReadOnlyDictionary.cs	\
	$(srcdir)/ProtocolBuffers/DescriptorProtos/DescriptorProtoFile.cs \
	$(srcdir)/ProtocolBuffers/DescriptorProtos/IDescriptorProto.cs  \
	$(srcdir)/ProtocolBuffers/DescriptorProtos/PartialClasses.cs    \
	$(srcdir)/ProtocolBuffers/Descriptors/DescriptorBase.cs		\
	$(srcdir)/ProtocolBuffers/Descriptors/DescriptorPool.cs		\
	$(srcdir)/ProtocolBuffers/Descriptors/DescriptorUtil.cs		\
	$(srcdir)/ProtocolBuffers/Descriptors/DescriptorValidationException.cs		\
	$(srcdir)/ProtocolBuffers/Descriptors/EnumDescriptor.cs		\
	$(srcdir)/ProtocolBuffers/Descriptors/EnumDescriptorIndexAttribute.cs		\
	$(srcdir)/ProtocolBuffers/Descriptors/EnumValueDescriptor.cs	\
	$(srcdir)/ProtocolBuffers/Descriptors/FieldDescriptor.cs	\
	$(srcdir)/ProtocolBuffers/Descriptors/FieldMappingAttribute.cs	\
	$(srcdir)/ProtocolBuffers/Descriptors/FieldType.cs		\
	$(srcdir)/ProtocolBuffers/Descriptors/FileDescriptor.cs		\
	$(srcdir)/ProtocolBuffers/Descriptors/IndexedDescriptorBase.cs	\
	$(srcdir)/ProtocolBuffers/Descriptors/MappedType.cs		\
	$(srcdir)/ProtocolBuffers/Descriptors/MethodDescriptor.cs	\
	$(srcdir)/ProtocolBuffers/Descriptors/MessageDescriptor.cs	\
	$(srcdir)/ProtocolBuffers/Descriptors/PackageDescriptor.cs	\
	$(srcdir)/ProtocolBuffers/Descriptors/ServiceDescriptor.cs	\
	$(srcdir)/ProtocolBuffers/ExtensionInfo.cs			\
	$(srcdir)/ProtocolBuffers/ExtensionRegistry.cs			\
	$(srcdir)/ProtocolBuffers/FieldAccess/Delegates.cs		\
	$(srcdir)/ProtocolBuffers/FieldAccess/FieldAccessorTable.cs	\
	$(srcdir)/ProtocolBuffers/FieldAccess/IFieldAccessor.cs		\
	$(srcdir)/ProtocolBuffers/FieldSet.cs				\
	$(srcdir)/ProtocolBuffers/GeneratedBuilder.cs			\
	$(srcdir)/ProtocolBuffers/GeneratedExtension.cs			\
	$(srcdir)/ProtocolBuffers/GeneratedMessage.cs			\
	$(srcdir)/ProtocolBuffers/IBuilder.cs				\
	$(srcdir)/ProtocolBuffers/IMessage.cs				\
	$(srcdir)/ProtocolBuffers/InvalidProtocolBufferException.cs	\
	$(srcdir)/ProtocolBuffers/Properties/AssemblyInfo.cs		\
	$(srcdir)/ProtocolBuffers/TextFormat.cs				\
	$(srcdir)/ProtocolBuffers/UninitializedMessageException.cs	\
	$(srcdir)/ProtocolBuffers/UnknownField.cs			\
	$(srcdir)/ProtocolBuffers/UnknownFieldSet.cs			\
	$(srcdir)/ProtocolBuffers/WireFormat.cs

Google.ProtocolBuffers.dll: $(Google.ProtocolBuffers.dll_sources)
	cp $(srcdir)/ProtocolBuffers/Properties/Google.ProtocolBuffers.snk .
	gmcs -codepage:utf8 -debug -target:library -out:Google.ProtocolBuffers.dll \
        $(Google.ProtocolBuffers.dll_sources) -keyfile Google.ProtocolBuffers.snk

install-data-local: Google.ProtocolBuffers.dll
	gacutil -i Google.ProtocolBuffers.dll -package google

EXTRA_DIST = $(Google.ProtocolBuffers.dll_sources) google-pb.pc.in

google-pb.pc: Makefile.am google-pb.pc.in
	sed "s,@PREFIX@,${prefix}," < $(srcdir)/google-pb.pc.in > google-pb.pc

