use strict;
use warnings;
use Test::More;
use lib 'lib';
use lib 'tmp';

# Mock Google::Auth and Google::gRPC::Client in BEGIN to ensure compile-time presence
BEGIN {
    package Google::Auth;
    $INC{'Google/Auth.pm'} = 1;
    sub default { bless {}, shift }
    sub get_token { 'mock_access_token_abc123' }

    package Google::gRPC::Client;
    $INC{'Google/gRPC/Client.pm'} = 1;

    our $last_call_args;
    our $mock_response;

    sub new {
        my ($class, %args) = @_;
        return bless \%args, $class;
    }

    sub call {
        my ($self, $args) = @_;
        $last_call_args = $args;
        return $mock_response;
    }
}

package main;

BEGIN {
    eval 'use Google::Cloud::Dataproc::V1::Clusters';
    if ($@) {
        plan skip_all => 'Google::Cloud::Dataproc::V1::Clusters could not be loaded (missing dependencies): ' . $@;
    }
}

subtest 'ClusterControllerClient unary dispatch' => sub {
    my $client = Google::Cloud::Dataproc::V1::Clusters::ClusterControllerClient->new(
        target => 'dataproc.googleapis.com:443',
    );

    isa_ok($client, 'Google::Cloud::Dataproc::V1::Clusters::ClusterControllerClient');
    is($client->target, 'dataproc.googleapis.com:443', 'Target correctly set');
    is($client->credentials->get_token(), 'mock_access_token_abc123', 'Auth token mock works');

    # Prepare mock response
    my $mock_cluster = Google::Cloud::Dataproc::V1::Clusters::Cluster->new(
        project_id   => 'test-project',
        cluster_name => 'test-cluster',
        cluster_uuid => 'uuid-1234-5678',
    );
    $Google::gRPC::Client::mock_response = $mock_cluster;

    # Make the call
    my $res = $client->get_cluster({
        project_id   => 'test-project',
        region       => 'us-central1',
        cluster_name => 'test-cluster',
    });

    # Verify fetcher interaction
    my $called = $Google::gRPC::Client::last_call_args;
    ok($called, 'gRPC fetcher called');
    is($called->{service}, 'google.cloud.dataproc.v1.ClusterController', 'Correct service dispatched');
    is($called->{method}, 'GetCluster', 'Correct method dispatched');

    my $req = $called->{request};
    isa_ok($req, 'Google::Cloud::Dataproc::V1::Clusters::GetClusterRequest');
    is($req->project_id, 'test-project', 'Request project_id correct');
    is($req->region, 'us-central1', 'Request region correct');
    is($req->cluster_name, 'test-cluster', 'Request cluster_name correct');

    # Verify returned response
    isa_ok($res, 'Google::Cloud::Dataproc::V1::Clusters::Cluster');
    is($res->project_id, 'test-project', 'Response project_id matches');
    is($res->cluster_name, 'test-cluster', 'Response cluster_name matches');
    is($res->cluster_uuid, 'uuid-1234-5678', 'Response cluster_uuid matches');
};

done_testing();
