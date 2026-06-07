# cpanfile for Protobuf Perl extension

# Build requirements
on 'configure' => sub {
  requires 'ExtUtils::MakeMaker';
  requires 'Config';
  requires 'File::Spec';
  requires 'File::Find';
  requires 'ExtUtils::Embed';
  requires 'JSON::MaybeXS';
  requires 'Template';
  requires 'Path::Tiny';
  requires 'Cwd';
  requires 'Sideload::Build::FileLists'; # Assuming this is on CPAN or in local/lib
  requires 'Devel::PPPort'; # Optional, for generating ppport.h
};

# Runtime requirements
requires 'Moo', '0';
requires 'Type::Tiny', '0';
requires 'Types::Standard', '0';
requires 'MIME::Base64', '0';
requires 'Const::Fast', '0';

# Test requirements
on 'test' => sub {
  requires 'Test::More', '0';
  requires 'Test::MockModule', '0';
  requires 'Test::Valgrind', '0'; # Optional
  requires 'Coro',           '0'; # Optional
  requires 'Capture::Tiny',  '0';
};
