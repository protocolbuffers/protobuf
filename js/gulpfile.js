var gulp = require('gulp');
var exec = require('child_process').exec;

gulp.task('genproto', function (cb) {
  exec('../src/protoc --js_out=library=testproto_libs,binary:. -I ../src -I . *.proto ../src/google/protobuf/descriptor.proto',
       function (err, stdout, stderr) {
    console.log(stdout);
    console.log(stderr);
    cb(err);
  });
})

gulp.task('deps', ['genproto'], function (cb) {
  exec('./node_modules/google-closure-library/closure/bin/build/depswriter.py *.js binary/*.js > deps.js',
       function (err, stdout, stderr) {
    console.log(stdout);
    console.log(stderr);
    cb(err);
  });
})

gulp.task('test', ['genproto', 'deps'], function (cb) {
  exec('JASMINE_CONFIG_PATH=jasmine.json ./node_modules/.bin/jasmine',
       function (err, stdout, stderr) {
    console.log(stdout);
    console.log(stderr);
    cb(err);
  });
});
