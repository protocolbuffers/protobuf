# Protobuf Global Extension Registry

This file contains a global registry of known extensions for descriptor.proto,
so that any developer who wishes to use multiple 3rd party projects, each with
their own extensions, can be confident that there won't be collisions in
extension numbers.

If you need an extension number for your custom option (see [custom options](
https://developers.google.com/protocol-buffers/docs/proto#customoptions)),
please [send us a pull request](https://github.com/protocolbuffers/protobuf/pulls) to
add an entry to this doc, or [create an issue](https://github.com/protocolbuffers/protobuf/issues)
with info about your project (name and website) so we can add an entry for you.

## Existing Registered Extensions

1.  C# port of protocol buffers

    *   Website: https://github.com/jskeet/protobuf-csharp-port
    *   Extensions: 1000

1.  Perl/XS port of protocol buffers

    *   Website: http://code.google.com/p/protobuf-perlxs
    *   Extensions: 1001

1.  Objective-C port of protocol buffers

    *   Website: http://code.google.com/p/protobuf-objc
    *   Extensions: 1002

1.  Google Wave Federation Protocol open-source release (FedOne)

    *   Website: http://code.google.com/p/wave-protocol
    *   Extensions: 1003

1.  PHP code generator plugin

    *   Website: ???
    *   Extensions: 1004

1.  GWT code generator plugin (third-party!)

    *   Website: http://code.google.com/p/protobuf-gwt/
    *   Extensions: 1005

1.  Unix Domain RPC code generator plugin

    *   Website: http://go/udrpc
    *   Extensions: 1006

1.  Object-C generator plugin (Plausible Labs)

    *   Website: http://www.plausible.coop
    *   Extensions: 1007

1.  TBD (code42.com)

    *   Website: ???
    *   Extensions: 1008

1.  Goby Underwater Autonomy Project

    *   Website: https://github.com/GobySoft/goby
    *   Extensions: 1009

1.  Nanopb

    *   Website: http://kapsi.fi/~jpa/nanopb
    *   Extensions: 1010

1.  Bluefin AUV Communication Extensions

    *   Website: http://www.bluefinrobotics.com
    *   Extensions: 1011

1.  Dynamic Compact Control Language

    *   Website: http://github.com/GobySoft/dccl
    *   Extensions: 1012

1.  ScaleOut StateServer® Native C++ API

    *   Website: http://www.scaleoutsoftware.com
    *   Extensions: 1013

1.  FoundationDB SQL Layer

    *   Website: https://github.com/FoundationDB/sql-layer
    *   Extensions: 1014

1.  Fender

    *   Website: https://github.com/hassox/fender
    *   Extensions: 1015

1.  Vortex

    *   Website: http://www.prismtech.com/vortex
    *   Extensions: 1016

1.  tresorit

    *   Website: https://tresorit.com/
    *   Extensions: 1017

1.  CRIU (Checkpoint Restore In Userspace)

    *   Website: http://criu.org/Main_Page
    *   Extensions: 1018

1.  protobuf-c

    *   Website: https://github.com/protobuf-c/protobuf-c
    *   Extensions: 1019

1.  ScalaPB

    *   Website: https://scalapb.github.io/
    *   Extensions: 1020

1.  protoc-gen-bq-schema

    *   Website: https://github.com/GoogleCloudPlatform/protoc-gen-bq-schema
    *   Extensions: 1021

1.  grpc-gateway

    *   Website: https://github.com/gengo/grpc-gateway
    *   Extensions: 1022

1.  Certificate Transparency

    *   Website: https://github.com/google/certificate-transparency
    *   Extensions: 1023

1.  JUNOS Router Telemetry

    *   Website: http://www.juniper.net
    *   Extensions: 1024

1.  Spine Event Engine

    *   Website: https://github.com/SpineEventEngine/core-java
    *   Extensions: 1025

1.  Aruba cloud platform

    *   Website: ???
    *   Extensions: 1026 -> 1030

1.  Voltha

    *   Website: ???
    *   Extensions: 1031 -> 1033

1.  gator

    *   Website: ???
    *   Extensions: 1034

1.  protoc-gen-flowtypes

    *   Website:
        https://github.com/tmc/grpcutil/tree/master/protoc-gen-flowtypes
    *   Extensions: 1035

1.  ProfaneDB

    *   Website: https://gitlab.com/ProfaneDB/ProfaneDB
    *   Extensions: 1036

1.  protobuf-net

    *   Website: https://github.com/mgravell/protobuf-net
    *   Extensions: 1037

1.  FICO / StreamEngine

    *   Website: http://www.fico.com/
    *   Extensions: 1038

1.  GopherJS

    *   Website: https://github.com/johanbrandhorst/protobuf
    *   Extensions: 1039

1.  ygot

    *   Website: https://github.com/openconfig/ygot
    *   Extensions: 1040, 1179, 1180

1.  go-grpcmw

    *   Website: https://github.com/MarquisIO/go-grpcmw
    *   Extensions: 1041

1.  grpc-gateway protoc-gen-swagger

    *   Website: https://github.com/grpc-ecosystem/grpc-gateway
    *   Extensions: 1042

1.  AN Message

    *   Website: TBD
    *   Extensions: 1043

1.  protofire

    *   Website: https://github.com/ribrdb/protofire
    *   Extensions: 1044

1.  Gravity

    *   Website: https://github.com/aphysci/gravity
    *   Extensions: 1045

1.  SEMI Standards – I&C Technical Committee

    *   Website:
        http://downloads.semi.org/web/wstdsbal.nsf/9c2b317e76523cca88257641005a47f5/88a5863a580e323088256e7b00707489!OpenDocument
    *   Extensions: 1046

1.  Elixir plugin

    *   Website: https://github.com/tony612/grpc-elixir
    *   Extensions: 1047

1.  API client generators

    *   Website: ???
    *   Extensions: 1048-1056

1.  Netifi Proteus

    *   Website: https://github.com/netifi-proteus
    *   Extensions: 1057

1.  CGSN Mooring Project

    *   Website: https://bitbucket.org/ooicgsn/cgsn-mooring
    *   Extensions: 1058

1.  Container Storage Interface

    *   Website: https://github.com/container-storage-interface/spec
    *   Extensions: 1059-1069

1.  TwirpQL Plugin

    *   Website: https://twirpql.dev
    *   Extensions: 1070

1.  Protoc-gen-validate

    *   Website: https://github.com/bufbuild/protoc-gen-validate
    *   Extensions: 1071

1.  Protokt

    *   Website: https://github.com/open-toast/protokt
    *   Extensions: 1072

1.  Dart port of protocol buffers

    *   Website https://github.com/dart-lang/protobuf
    *   Extensions: 1073

1.  Ocaml-protoc-plugin

    *   Website: https://github.com/issuu/ocaml-protoc-plugin
    *   Extensions: 1074

1.  Analyze Re Graphene

    *   Website: https://analyzere.com
    *   Extensions: 1075

1.  Wire since and until

    *   Website: https://square.github.io/wire/
    *   Extensions: 1076, 1077

1.  Bazel, Failure Details

    *   Website: https://github.com/bazelbuild/bazel
    *   Extensions: 1078

1.  grpc-graphql-gateway

    *   Website: https://github.com/ysugimoto/grpc-graphql-gateway
    *   Extensions: 1079

1.  Cloudstate

    *   Website: https://cloudstate.io
    *   Extensions: 1080-1084

1.  SummaFT protoc-plugins

    *   Website: https://summaft.com/
    *   Extensions: 1085

1.  ADLINK EdgeSDK

    *   Website: https://www.adlinktech.com/en/Edge-SDK-IoT
    *   Extensions: 1086

1.  Wire wire_package

    *   Website: https://square.github.io/wire/
    *   Extensions: 1087

1.  Confluent Schema Registry

    *   Website: https://github.com/confluentinc/schema-registry
    *   Extensions: 1088

1.  ScalaPB Validate

    *   Website: https://scalapb.github.io/docs/validation
    *   Extension: 1089

1.  Astounding (Currently Private)

    *   Website: https://github.com/PbPipes/Astounding
    *   Extension: 1090

1.  Protoc-gen-psql

    *   Website: https://github.com/Intrinsec/protoc-gen-psql
    *   Extension: 1091-1101

1.  Protoc-gen-sanitize

    *   Website: https://github.com/Intrinsec/protoc-gen-sanitize
    *   Extension: 1102-1106

1.  Coach Client Connect (planned release in March 2021)

    *   Website: https://www.coachclientconnect.com
    *   Extension: 1107

1.  Kratos API Errors

    *   Website: https://go-kratos.dev
    *   Extension: 1108

1.  Glitchdot (Currently Private)

    *   Website: https://go.glitchdot.com
    *   Extension: 1109

1.  eigr/protocol

    *   Website: https://eigr.io
    *   Extension: 1110-1114

1.  Container Object Storage Interface (COSI)

    *   Website:
        https://github.com/kubernetes-sigs/container-object-storage-interface-spec
    *   Extension: 1115-1124

1.  Protoc-gen-jsonschema

    *   Website: https://github.com/chrusty/protoc-gen-jsonschema
    *   Extension: 1125-1129

1.  Protoc-gen-checker

    *   Website: https://github.com/Intrinsec/protoc-gen-checker
    *   Extension: 1130-1139

1.  Protoc-gen-go-svc

    *   Website: https://github.com/dane/protoc-gen-go-svc
    *   Extension: 1140

1.  Embedded Proto

    *   Website: https://EmbeddedProto.com
    *   Extension: 1141

1.  Protoc-gen-fieldmask

    *   Website: https://github.com/yeqown/protoc-gen-fieldmask
    *   Extension: 1142

1.  Google Gnostic

    *   Website: https://github.com/google/gnostic
    *   Extension: 1143

1.  Protoc-gen-go-micro

    *   Website: https://github.com/unistack-org/protoc-gen-go-micro
    *   Extension: 1144

1.  Protoc-gen-authz

    *   Website: https://github.com/Neakxs/protoc-gen-authz
    *   Extension: 1145

1.  Protonium

    *   Website: https://github.com/zyp/protonium
    *   Extension: 1146

1.  Protoc-gen-xo

    *   Website: https://github.com/xo/ecosystem
    *   Extension: 1147

1.  Ballerina gRPC

    *   Website: https://github.com/ballerina-platform/module-ballerina-grpc
    *   Extension: 1148

1.  Protoc-gen-referential-integrity

    *   Website:
        https://github.com/ComponentCorp/protoc-gen-referential-integrity
    *   Extension: 1149

1.  Oclea Service Layer RPC

    *   Website: https://oclea.com/
    *   Extension: 1150

1.  mypy-protobuf

    *   Website: https://github.com/nipunn1313/mypy-protobuf
    *   Extension: 1151-1154

1.  Pigweed protobuf compiler

    *   Website: https://pigweed.dev/pw_protobuf
    *   Extension: 1155

1.  Perfetto

    *   Website: https://perfetto.dev
    *   Extension: 1156

1.  Buf

    *   Website: http://buf.build/
    *   Extension: 1157-1166

1.  Connect

    *   Website: http://connect.build/
    *   Extension: 1167-1176

1.  protocel

    *   Website: https://github.com/Neakxs/protocel
    *   Extension: 1177-1178

1.  Cybozu

    *   Website: https://github.com/cybozu/protobuf
    *   Extension: 1179

1.  EngFlow

    *   Website: https://github.com/EngFlow/engflowapis
    *   Extensions: 1181

1.  Proto-telemetry

    *   Website: https://github.com/clly/proto-telemetry
    *   Extensions: 1182

1.  Digital Twins Definition Language (DTDL)

    *   Website: https://github.com/Azure/opendigitaltwins-dtdl
    *   Extensions: 1183

1.  RabbitMQ

    *   Website: https://github.com/guihouchang/protoc-gen-go-event
    *   Extensions: 1184

1.  Wire use_array

    *   Website: https://square.github.io/wire/
    *   Extensions: 1185
