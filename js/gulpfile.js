var gulp = require('gulp');
var exec = require('child_process').exec;

gulp.task('genproto_closure', function (cb) {
  exec('../src/protoc --js_out=library=testproto_libs,binary:. -I ../src -I . *.proto ../src/google/protobuf/descriptor.proto',
       function (err, stdout, stderr) {
    console.log(stdout);
    console.log(stderr);
    cb(err);
  });
});

gulp.task('genproto_commonjs', function (cb) {
  exec('mkdir -p commonjs_out && ../src/protoc --js_out=import_style=commonjs,binary:commonjs_out -I ../src -I . *.proto ../src/google/protobuf/descriptor.proto',
       function (err, stdout, stderr) {
    console.log(stdout);
    console.log(stderr);
    cb(err);
  });
});

gulp.task('dist', function (cb) {
  // TODO(haberman): minify this more aggressively.
  // Will require proper externs/exports.
  exec('./node_modules/google-closure-library/closure/bin/calcdeps.py -i message.js -i binary/reader.js -i binary/writer.js -p . -p node_modules/google-closure-library/closure -o compiled --compiler_jar node_modules/google-closure-compiler/compiler.jar > google-protobuf.js',
       function (err, stdout, stderr) {
    console.log(stdout);
    console.log(stderr);
    cb(err);
  });
});

gulp.task('deps', ['genproto_closure'], function (cb) {
  exec('./node_modules/google-closure-library/closure/bin/build/depswriter.py *.js binary/*.js > deps.js',
       function (err, stdout, stderr) {
    console.log(stdout);
    console.log(stderr);
    cb(err);
  });
});

gulp.task('test_closure', ['genproto_closure', 'deps'], function (cb) {
  exec('JASMINE_CONFIG_PATH=jasmine.json ./node_modules/.bin/jasmine',
       function (err, stdout, stderr) {
    console.log(stdout);
    console.log(stderr);
    cb(err);
  });
});

gulp.task('test_commonjs', ['genproto_commonjs', 'dist'], function (cb) {
  exec('JASMINE_CONFIG_PATH=jasmine.json cp jasmine_commonjs.json commonjs_out/jasmine.json && cd commonjs_out && ../node_modules/.bin/jasmine',
       function (err, stdout, stderr) {
    console.log(stdout);
    console.log(stderr);
    cb(err);
  });
});
