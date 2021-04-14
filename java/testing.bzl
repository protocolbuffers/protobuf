"""
Generates a side-car JUnit suite test runner class for each
input src.
"""
_template = """import org.junit.runners.Suite;
import org.junit.runner.RunWith;

@RunWith(Suite.class)
@Suite.SuiteClasses({%s})
public class %s {}
"""

_pkg = "com.google.protobuf"

def _as_classname(fname):
    path_name = [x.path for x in fname.files.to_list()][0]
    file_name = path_name.split("/")[-1]
    return ".".join([_pkg, file_name.split(".")[0]]) + ".class"

def _gen_suite_impl(ctx):
    classes = ",".join(
        [_as_classname(x) for x in ctx.attr.srcs],
    )
    ctx.actions.write(output = ctx.outputs.out, content = _template % (
        classes,
        ctx.attr.outname,
    ))

_gen_suite = rule(
    attrs = {
        "srcs": attr.label_list(allow_files = True),
        "outname": attr.string(),
    },
    outputs = {"out": "%{name}.java"},
    implementation = _gen_suite_impl,
)

def junit_tests(name, srcs, data, deps, **kwargs):
    testlib_name = "%s_lib" % name
    native.java_library(
      name = testlib_name,
      srcs = srcs,
      deps = deps,
      resources = data,
      data = data,
    )

    test_names = []
    prefix = name.replace("-", "_") + "TestSuite"
    for src in srcs:
      test_name = src.rsplit("/", 1)[1].split(".")[0]
      test_names = test_names + [test_name]
      suite_name = prefix + '_' + test_name
      _gen_suite(
          name = suite_name,
          srcs = [src],
          outname = suite_name,
      )
      native.java_test(
          name = test_name,
          test_class = suite_name,
          srcs = [src] + [":" + suite_name],
          deps = deps + [":%s" % testlib_name],
          **kwargs
      )

    native.test_suite(
        name = name,
        tests = test_names,
    )
