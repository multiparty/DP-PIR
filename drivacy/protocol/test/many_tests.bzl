# Copyright 2020 multiparty.org
def _binary_test_impl(ctx):
    ctx.actions.write(
        is_executable = True,
        output = ctx.outputs.executable,
        content = """
            valgrind ./{binary} {cmdargs}
        """.format(
            binary=ctx.file.binary.short_path,
            cmdargs=ctx.attr.cmdargs,
        ),
    )

    return DefaultInfo(
        runfiles = ctx.runfiles(files = [ctx.file.binary]),
    )

binary_test = rule(
    doc = "Runs a binary file as a test. ",
    implementation = _binary_test_impl,
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


# Create a test for given configuration parameters.
def many_tests(binary, params_list):
    for params in params_list:
        strs = [str(p) for p in params]
        name = '-'.join(strs)
        cmdargs = ' '.join(strs)
        binary_test(
            name = "shuffle-test-{}".format(name),
            binary = binary,
            cmdargs = cmdargs,
        )
