This directory contains all of our automatically triggered workflows.

# Test runner

Our top level `test_runner.yml` is responsible for kicking off all tests, which
are represented as reusable workflows.  This is carefully constructed to satisfy
the design laid out in go/protobuf-gha-protected-resources (see below), and
duplicating it across every workflow file would be difficult to maintain.  As an
added bonus, we can manually dispatch our full test suite with a single button
and monitor the progress of all of them simultaneously in GitHub's actions UI.

There are five ways our test suite can be triggered:

- **Post-submit tests** (`push`): These are run over newly submitted code
that we can assume has been thoroughly reviewed.  There are no additional
security concerns here and these jobs can be given highly privileged access to
our internal resources and caches.

- **Pre-submit tests from a branch** (`push_request`): These are run over
every PR as changes are made.  Since they are coming from branches in our
repository, they have secret access by default and can also be given highly
privileged access.  However, we expect *many* of these events per change,
and likely many from abandoned/exploratory changes.  Given the much higher
frequency, we restrict the ability to *write* to our more expensive caches.

- **Pre-submit tests from a fork** (`push_request_target`): These are run
over every PR from a forked repository as changes are made.  These have much
more restricted access, since they could be coming from anywhere.  To protect
our secret keys and our resources, tests will not run until a commit has been
labeled `safe to submit`.  Further commits will require further approvals to
run our test suite.  Once marked as safe, we will provide read-only access to
our caches and Docker images, but will generally disallow any writes to shared
resources.

- **Continuous tests** (`schedule`): These are run on a fixed schedule.  We
currently have them set up to run daily, and can help identify non-hermetic
issues in tests that don't get run often (such as due to test caching) or during
slow periods like weekends and holidays.  Similar to post-submit tests, these
are run over submitted code and are highly privileged in the resources they
can use.

- **Manual testing** (`workflow_dispatch`): Our test runner can be triggered
manually over any branch.  This is treated similarly to pre-submit tests,
which should be highly privileged because they can only be triggered by the
protobuf team.

# Staleness handling

While Bazel handles code generation seamlessly, we do support build systems that
don't.  There are a handful of cases where we need to check in generated files
that can become stale over time.  In order to provide a good developer
experience, we've implemented a system to make this more manageable.

- Stale files should have a corresponding `staleness_test` Bazel target.  This
should be marked `manual` to avoid getting picked up in CI, but will fail if
files become stale.  It also provides a `--fix` flag to update the stale files.

- Bazel tests will never depend on the checked-in versions, and will generate
new ones on-the-fly during build.

- Non-Bazel tests will always regenerate necessary files before starting.  This
is done using our `bash` and `docker` actions, which should be used for any
non-Bazel tests.  This way, no tests will fail due to stale files.

- A post-submit job will immediately regenerate any stale files and commit them
if they've changed.

- A scheduled job will run late at night every day to make sure the post-submit
is working as expected (that is, it will run all the staleness tests).

The `regenerate_stale_files.sh` script is the central script responsible for all
the re-generation of stale files.

# Forked PRs

Because we need secret access to run our tests, we use the `pull_request_target`
event for PRs coming from forked repositories.  We do checkout the code from the
PR's head, but the workflow files themselves are always fetched from the *base*
branch (that is, the branch we're merging to).  Therefore, any changes to these
files won't be tested, so we explicitly ban PRs that touch these files.

# Caches

We have a number of different caching strategies to help speed up tests.  These
live either in GCP buckets or in our GitHub repository cache.  The former has
a lot of resources available and we don't have to worry as much about bloat.
On the other hand, the GitHub repository cache is limited to 10GB, and will
start pruning old caches when it exceeds that threshold.  Therefore, we need
to be very careful about the size and quantity of our caches in order to
maximize the gains.

## Bazel remote cache

As described in https://bazel.build/remote/caching, remote caching allows us to
offload a lot of our build steps to a remote server that holds a cache of
previous builds.  We use our GCP project for this storage, and configure
*every* Bazel call to use it.  This provides substantial performance
improvements at minimal cost.

We do not allow forked PRs to upload updates to our Bazel caches, but they
do use them.  Every other event is given read/write access to the caches.
Because Bazel behaves poorly under certain environment changes (such as
toolchain, operating system), we try to use finely-grained caches.  Each job
should typically have its own cache to avoid cross-pollution.

## Bazel repository cache

When Bazel starts up, it downloads all the external dependencies for a given
build and stores them in the repository cache.  This cache is *separate* from
the remote cache, and only exists locally.  Because we have so many Bazel
dependencies, this can be a source of frequent flakes due to network issues.

To avoid this, we keep a cached version of the repository cache in GitHub's
action cache.  Our full set of repository dependencies ends up being ~300MB,
which is fairly expensive given our 10GB maximum.  The most expensive ones seem
to come from Java, which has some very large downstream dependencies.

Given the cost, we take a more conservative approach for this cache.  Only push
events will ever write to this cache, but all events can read from them.
Additionally, we only store three caches for any given commit, one per platform.
This means that multiple jobs are trying to update the same cache, leading to a
race. GitHub rejects all but one of these updates, so we designed the system so
that caches are only updated if they've actually changed.  That way, over time
(and multiple pushes) the repository caches will incrementally grow to encompass
all of our dependencies.  A scheduled job will run monthly to clear these caches
to prevent unbounded growth as our dependencies evolve.

## ccache

In order to speed up non-Bazel builds to be on par with Bazel, we make use of
[ccache](https://ccache.dev/).  This intercepts all calls to the compiler, and
caches the result.  Subsequent calls with a cache-hit will very quickly
short-circuit and return the already computed result.  This has minimal affect
on any *single* job, since we typically only run a single build.  However, by
caching the ccache results in GitHub's action cache we can substantially
decrease the build time of subsequent runs.

One useful feature of ccache is that you can set a maximum cache size, and it
will automatically prune older results to keep below that limit.  On Linux and
Mac cmake builds, we generally get 30MB caches and set a 100MB cache limit.  On
Windows, with debug symbol stripping we get ~70MB and set a 200MB cache limit.

Because CMake build tend to be our slowest, bottlenecking the entire CI process,
we use a fairly expensive strategy with ccache.  All events will cache their
ccache directory, keyed by the commit and the branch.  This means that each
PR and each branch will write its own set of caches.  When looking up which
cache to use initially, each job will first look for a recent cache in its
current branch.  If it can't find one, it will accept a cache from the base
branch (for example, PRs will initially use the latest cache from their target
branch).

While the ccache caches quickly over-run our GitHub action cache, they also
quickly become useless.  Since GitHub prunes caches based on the time they were
last used, this just means that we'll see quicker turnover.

## sccache

An alternative to ccache is [sccache](https://github.com/mozilla/sccache). The
two tools are very similar in function, but sccache requires (and allows) much
less configuration and supports GCS storage right out of the box. By hooking
this up to our project that we already use for Bazel caching, we're able to get
even bigger CMake wins in CI because we're no longer constrained by GitHub's
10GB cache limit.

Similar to the Bazel remote cache, we give read access to every CI run, but
disallow writing in PRs from forks.

## Bazelisk

Bazelisk will automatically download a pinned version of Bazel on first use.
This can lead to flakes, and to avoid that we cache the result keyed on the
Bazel version.  Only push events will write to this cache, but it's unlikely
to change very often.

## Docker images

Instead of downloading a fresh Docker image for every test run, we can save it
as a tar and cache it using `docker image save` and later restore using
`docker image load`.  This can decrease download times and also reduce flakes.
Note, Docker's load can actually be significantly slower than a pull in certain
situations.  Therefore, we should reserve this strategy for only Docker images
that are causing noticeable flakes.

## Pip dependencies

The actions/setup-python action we use for Python supports automated caching
of pip dependencies.  We enable this to avoid having to download these
dependencies on every run, which can lead to flakes.

# Custom actions

We've defined a number of custom actions to abstract out shared pieces of our
workflows.

- **Bazel** use this for running all Bazel tests.  It can take either a single
Bazel command or a more general bash command.  In the latter case, it provides
environment variables for running Bazel with all our standardized settings.

- **Bazel-Docker** nearly identical to the **Bazel** action, this additionally
runs everything in a specified Docker image.

- **Bash** use this for running non-Bazel tests.  It takes a bash command and
runs it verbatim.  It also handles the regeneration of stale files (which does
use Bazel), which non-Bazel tests might depend on.

- **Docker** nearly identical to the **Bash** action, this additionally runs
everything in a specified Docker image.

- **ccache** this sets up a ccache environment, and initializes some
environment variables for standardized usage of ccache.

- **Cross-compile protoc** this abstracts out the compilation of protoc using
our cross-compilation infrastructure.  It will set a `PROTOC` environment
variable that gets automatically picked up by a lot of our infrastructure.
This is most useful in conjunction with the **Bash** action with non-Bazel
tests.
