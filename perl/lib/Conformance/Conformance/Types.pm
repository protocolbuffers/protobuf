package Conformance::Conformance::Types;

use strict;
use warnings;

use Type::Library -base;
use Type::Utils -all;
use Types::Standard -types;

declare 'WireFormat',
    as (Int | Str);

declare 'TestCategory',
    as (Int | Str);

declare 'TestStatus',
    as InstanceOf['Conformance::Conformance::TestStatus'];

coerce 'TestStatus',
    from HashRef, via { 'Conformance::Conformance::TestStatus'->new($_) };

declare 'RepeatedTestStatus',
    as ArrayRef[TestStatus()];

coerce 'RepeatedTestStatus',
    from ArrayRef[HashRef], via { [ map { 'Conformance::Conformance::TestStatus'->new($_) } @$_ ] };

declare 'MapStringTestStatus',
    as HashRef[TestStatus()];

declare 'FailureSet',
    as InstanceOf['Conformance::Conformance::FailureSet'];

coerce 'FailureSet',
    from HashRef, via { 'Conformance::Conformance::FailureSet'->new($_) };

declare 'RepeatedFailureSet',
    as ArrayRef[FailureSet()];

coerce 'RepeatedFailureSet',
    from ArrayRef[HashRef], via { [ map { 'Conformance::Conformance::FailureSet'->new($_) } @$_ ] };

declare 'MapStringFailureSet',
    as HashRef[FailureSet()];

declare 'ConformanceRequest',
    as InstanceOf['Conformance::Conformance::ConformanceRequest'];

coerce 'ConformanceRequest',
    from HashRef, via { 'Conformance::Conformance::ConformanceRequest'->new($_) };

declare 'RepeatedConformanceRequest',
    as ArrayRef[ConformanceRequest()];

coerce 'RepeatedConformanceRequest',
    from ArrayRef[HashRef], via { [ map { 'Conformance::Conformance::ConformanceRequest'->new($_) } @$_ ] };

declare 'MapStringConformanceRequest',
    as HashRef[ConformanceRequest()];

declare 'ConformanceResponse',
    as InstanceOf['Conformance::Conformance::ConformanceResponse'];

coerce 'ConformanceResponse',
    from HashRef, via { 'Conformance::Conformance::ConformanceResponse'->new($_) };

declare 'RepeatedConformanceResponse',
    as ArrayRef[ConformanceResponse()];

coerce 'RepeatedConformanceResponse',
    from ArrayRef[HashRef], via { [ map { 'Conformance::Conformance::ConformanceResponse'->new($_) } @$_ ] };

declare 'MapStringConformanceResponse',
    as HashRef[ConformanceResponse()];

declare 'JspbEncodingConfig',
    as InstanceOf['Conformance::Conformance::JspbEncodingConfig'];

coerce 'JspbEncodingConfig',
    from HashRef, via { 'Conformance::Conformance::JspbEncodingConfig'->new($_) };

declare 'RepeatedJspbEncodingConfig',
    as ArrayRef[JspbEncodingConfig()];

coerce 'RepeatedJspbEncodingConfig',
    from ArrayRef[HashRef], via { [ map { 'Conformance::Conformance::JspbEncodingConfig'->new($_) } @$_ ] };

declare 'MapStringJspbEncodingConfig',
    as HashRef[JspbEncodingConfig()];

1;
