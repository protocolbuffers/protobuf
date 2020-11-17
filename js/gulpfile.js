var gulp = require('gulp');
var execFile = require('child_process').execFile;
var glob = require('glob');

function exec(command, cb) {
  execFile('sh', ['-c', command], cb);
}

var protoc = process.env.PROTOC || '../src/protoc';

var wellKnownTypes = [
  '../src/google/protobuf/any.proto',
  '../src/google/protobuf/api.proto',
  '../src/google/protobuf/compiler/plugin.proto',
  '../src/google/protobuf/descriptor.proto',
  '../src/google/protobuf/duration.proto',
  '../src/google/protobuf/empty.proto',
  '../src/google/protobuf/field_mask.proto',
  '../src/google/protobuf/source_context.proto',
  '../src/google/protobuf/struct.proto',
  '../src/google/protobuf/timestamp.proto',
  '../src/google/protobuf/type.proto',
  '../src/google/protobuf/wrappers.proto',
];

var group1Protos = [
  'data.proto',
  'test3.proto',
  'test5.proto',
  'commonjs/test6/test6.proto',
  'test8.proto',
  'test11.proto',
  'test12.proto',
  'test13.proto',
  'test14.proto',
  'test15.proto',
  'testbinary.proto',
  'testempty.proto',
  'test.proto',
  'testlargenumbers.proto',
];

var group2Protos = [
  'proto3_test.proto',
  'test2.proto',
  'test4.proto',
  'commonjs/test7/test7.proto',
];

var group3Protos = [
  'test9.proto',
  'test10.proto'
];


gulp.task('genproto_well_known_types_closure', function (cb) {
  exec(protoc + ' --js_out=one_output_file_per_input_file,binary:. -I ../src -I . ' + wellKnownTypes.join(' '),
       function (err, stdout, stderr) {
    console.log(stdout);
    console.log(stderr);
    cb(err);
  });
});

gulp.task('genproto_group1_closure', function (cb) {
  exec(protoc + ' --js_out=library=testproto_libs1,binary:.  -I ../src -I . ' + group1Protos.join(' '),
       function (err, stdout, stderr) {
    console.log(stdout);
    console.log(stderr);
    cb(err);
  });
});

gulp.task('genproto_group2_closure', function(cb) {
  exec(
      protoc +
          ' --experimental_allow_proto3_optional --js_out=library=testproto_libs2,binary:.  -I ../src -I . -I commonjs ' +
          group2Protos.join(' '),
      function(err, stdout, stderr) {
        console.log(stdout);
        console.log(stderr);
        cb(err);
      });
});

gulp.task('genproto_well_known_types_commonjs', function (cb) {
  exec('mkdir -p commonjs_out && ' + protoc + ' --js_out=import_style=commonjs,binary:commonjs_out -I ../src ' + wellKnownTypes.join(' '),
       function (err, stdout, stderr) {
    console.log(stdout);
    console.log(stderr);
    cb(err);
  });
});

gulp.task('genproto_group1_commonjs', function (cb) {
  exec('mkdir -p commonjs_out && ' + protoc + ' --js_out=import_style=commonjs,binary:commonjs_out -I ../src -I commonjs -I . ' + group1Protos.join(' '),
       function (err, stdout, stderr) {
    console.log(stdout);
    console.log(stderr);
    cb(err);
  });
});

gulp.task('genproto_group2_commonjs', function(cb) {
  exec(
      'mkdir -p commonjs_out && ' + protoc +
          ' --experimental_allow_proto3_optional --js_out=import_style=commonjs,binary:commonjs_out -I ../src -I commonjs -I . ' +
          group2Protos.join(' '),
      function(err, stdout, stderr) {
        console.log(stdout);
        console.log(stderr);
        cb(err);
      });
});

gulp.task('genproto_commonjs_wellknowntypes', function (cb) {
  exec('mkdir -p commonjs_out/node_modules/google-protobuf && ' + protoc + ' --js_out=import_style=commonjs,binary:commonjs_out/node_modules/google-protobuf -I ../src ' + wellKnownTypes.join(' '),
       function (err, stdout, stderr) {
    console.log(stdout);
    console.log(stderr);
    cb(err);
  });
});

gulp.task('genproto_wellknowntypes', function (cb) {
  exec(protoc + ' --js_out=import_style=commonjs,binary:. -I ../src ' + wellKnownTypes.join(' '),
       function (err, stdout, stderr) {
    console.log(stdout);
    console.log(stderr);
    cb(err);
  });
});
gulp.task('genproto_group3_commonjs_strict', function (cb) {
  exec('mkdir -p commonjs_out && ' + protoc + ' --js_out=import_style=commonjs_strict,binary:commonjs_out -I ../src -I commonjs -I . ' + group3Protos.join(' '),
       function (err, stdout, stderr) {
    console.log(stdout);
    console.log(stderr);
    cb(err);
  });
});


function getClosureBuilderCommand(exportsFile, outputFile) {
  return './node_modules/google-closure-library/closure/bin/build/closurebuilder.py ' +
  '--root node_modules ' +
  '-o compiled ' +
  '--compiler_jar node_modules/google-closure-compiler-java/compiler.jar ' +
  '-i ' + exportsFile + ' ' +
  'map.js message.js binary/arith.js binary/constants.js binary/decoder.js ' +
  'binary/encoder.js binary/reader.js binary/utils.js binary/writer.js ' +
  exportsFile  + ' > ' + outputFile;
}

gulp.task('dist', gulp.series(['genproto_wellknowntypes'], function(cb) {
  // TODO(haberman): minify this more aggressively.
  // Will require proper externs/exports.
  exec(getClosureBuilderCommand('commonjs/export.js', 'google-protobuf.js'),
       function (err, stdout, stderr) {
    console.log(stdout);
    console.log(stderr);
    cb(err);
  });
}));

gulp.task('commonjs_asserts', function (cb) {
  exec('mkdir -p commonjs_out/test_node_modules && ' +
       getClosureBuilderCommand(
           'commonjs/export_asserts.js',
           'commonjs_out/test_node_modules/closure_asserts_commonjs.js'),
       function (err, stdout, stderr) {
    console.log(stdout);
    console.log(stderr);
    cb(err);
  });
});

gulp.task('commonjs_testdeps', function (cb) {
  exec('mkdir -p commonjs_out/test_node_modules && ' +
       getClosureBuilderCommand(
           'commonjs/export_testdeps.js',
           'commonjs_out/test_node_modules/testdeps_commonjs.js'),
       function (err, stdout, stderr) {
    console.log(stdout);
    console.log(stderr);
    cb(err);
  });
});

gulp.task(
    'make_commonjs_out',
    gulp.series(
        [
          'dist', 'genproto_well_known_types_commonjs',
          'genproto_group1_commonjs', 'genproto_group2_commonjs',
          'genproto_commonjs_wellknowntypes', 'commonjs_asserts',
          'commonjs_testdeps', 'genproto_group3_commonjs_strict'
        ],
        function(cb) {
          // TODO(haberman): minify this more aggressively.
          // Will require proper externs/exports.
          var cmd =
              'mkdir -p commonjs_out/binary && mkdir -p commonjs_out/test_node_modules && ';
          function addTestFile(file) {
            cmd += 'node commonjs/rewrite_tests_for_commonjs.js < ' + file +
                ' > commonjs_out/' + file + '&& ';
          }

          glob.sync('*_test.js').forEach(addTestFile);
          glob.sync('binary/*_test.js').forEach(addTestFile);

          exec(
              cmd + 'cp commonjs/jasmine.json commonjs_out/jasmine.json && ' +
                  'cp google-protobuf.js commonjs_out/test_node_modules && ' +
                  'cp commonjs/strict_test.js commonjs_out/strict_test.js &&' +
                  'cp commonjs/import_test.js commonjs_out/import_test.js',
              function(err, stdout, stderr) {
                console.log(stdout);
                console.log(stderr);
                cb(err);
              });
        }));

gulp.task(
    'deps',
    gulp.series(
        [
          'genproto_well_known_types_closure', 'genproto_group1_closure',
          'genproto_group2_closure'
        ],
        function(cb) {
          exec(
              './node_modules/google-closure-library/closure/bin/build/depswriter.py binary/arith.js binary/constants.js binary/decoder.js binary/encoder.js binary/reader.js binary/utils.js binary/writer.js debug.js map.js message.js node_loader.js test_bootstrap.js > deps.js',
              function(err, stdout, stderr) {
                console.log(stdout);
                console.log(stderr);
                cb(err);
              });
        }));

gulp.task(
    'test_closure',
    gulp.series(
        [
          'genproto_well_known_types_closure', 'genproto_group1_closure',
          'genproto_group2_closure', 'deps'
        ],
        function(cb) {
          exec(
              'JASMINE_CONFIG_PATH=jasmine.json ./node_modules/.bin/jasmine',
              function(err, stdout, stderr) {
                console.log(stdout);
                console.log(stderr);
                cb(err);
              });
        }));

gulp.task('test_commonjs', gulp.series(['make_commonjs_out'], function(cb) {
  exec('cd commonjs_out && JASMINE_CONFIG_PATH=jasmine.json NODE_PATH=test_node_modules ../node_modules/.bin/jasmine',
       function (err, stdout, stderr) {
    console.log(stdout);
    console.log(stderr);
    cb(err);
  });
}));

gulp.task('test', gulp.series(['test_closure', 'test_commonjs'], function(cb) {
  cb();
}));
