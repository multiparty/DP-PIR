# Copyright 2020 multiparty.org
# A rule for running c++ test files with valgrind attached.
def _valgrind_test_impl(ctx):
    ctx.actions.write(
        is_executable = True,
        output = ctx.outputs.executable,
        content = """
            valgrind ./{binary} {cmdargs} &> {name}.tmp
            status=0
            if [[ $(grep "definitely lost: 0" {name}.tmp | wc -l) -ne 1 ]]; then
              status=1
            fi
            if [[ $(grep "indirectly lost: 0" {name}.tmp | wc -l) -ne 1 ]]; then
              status=1
            fi
            if [[ $(grep "possibly lost: 0" {name}.tmp | wc -l) -ne 1 ]]; then
              status=1
            fi
            cat {name}.tmp
            exit $status
        """.format(
            name=ctx.attr.name,
            binary=ctx.file.binary.short_path,
            cmdargs=ctx.attr.cmdargs,
        ),
    )

    return DefaultInfo(
        runfiles = ctx.runfiles(files = [ctx.file.binary]),
    )

valgrind_test = rule(
    doc = "Tests a c++ file with valgrind on.",
    implementation = _valgrind_test_impl,
    test = True,
    attrs = {
        "binary": attr.label(
            doc = "The binary to run.",
            mandatory = True,
            allow_single_file = True,
        ),
        "cmdargs": attr.string(
            doc = "Command line arguments list.",
            mandatory = False,
        ),
    },
)

def many_tests(binary, params_list):
    for params in params_list:
        strs = [str(p) for p in params]
        name = '-'.join(strs)
        cmdargs = ' '.join(strs)
        valgrind_test(
            name = "shuffle-test-{}".format(name),
            binary = binary,
            cmdargs = cmdargs,
        )
