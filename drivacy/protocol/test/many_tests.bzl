# Copyright 2020 multiparty.org
def _binary_test_impl(ctx):
    ctx.actions.write(
        is_executable = True,
        output = ctx.outputs.executable,
        content = """
            valgrind ./{binary} {cmdargs}
        """.format(
            binary=ctx.file.binary.short_path,
            cmdargs=" ".join(ctx.attr.cmdargs),
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
        "cmdargs": attr.string_list(
            doc = "Command line arguments list.",
            mandatory = False,
        ),
    },
)


# Create a test for given configuration parameters.
def many_tests(binary, args, params_list):
    for params in params_list:
        if len(args) != len(params):
            fail("args and params need to have the same length")

        cmdargs = [
          "--{}={}".format(arg, param) for (arg, param) in zip(args, params)
        ]
        binary_test(
            name = "shuffle-test-{}".format("-".join([str(p) for p in params])),
            binary = binary,
            cmdargs = cmdargs,
        )
