# Copyright 2020 multiparty.org

# Contains rule for running a test that consists of many binary
# runs in parallel (as separate processes).

SCRIPT = """
# Create log directory
valgrind ./{_main} --config={config} --table={table} --span={span} \
                   --cutoff={cutoff} --batch={batch}
"""

def _simulated_test_impl(ctx):
    ctx.actions.write(
        is_executable = True,
        output = ctx.outputs.executable,
        content = SCRIPT.format(
            _main=ctx.file._main.short_path,
            config=ctx.file.config.short_path,
            table=ctx.file.table.short_path,
            span=ctx.attr.span,
            cutoff=ctx.attr.cutoff,
            batch=ctx.attr.batch,
        ),
    )

    files = [ctx.file._main, ctx.file.config, ctx.file.table]
    return DefaultInfo(
        runfiles = ctx.runfiles(files = files),
    )

simulated_test = rule(
    doc = "Runs the entire protocol over TCP sockets and different processes.",
    implementation = _simulated_test_impl,
    test = True,
    attrs = {
        "_main": attr.label(
            doc = "Test main entry point.",
            mandatory = False,
            allow_single_file = True,
            default = "//drivacy/test:main",
        ),
        "config": attr.label(
            doc = "The path to config.json.",
            mandatory = True,
            allow_single_file = True,
        ),
        "table": attr.label(
            doc = "The path to table.json.",
            mandatory = True,
            allow_single_file = True,
        ),
        "span": attr.string(
            doc = "The span of the DP distribution",
            mandatory = True,
        ),
        "cutoff": attr.string(
            doc = "The cutoff for the DP distribution",
            mandatory = True,
        ),
        "batch": attr.int(
            doc = "The batch size.",
            mandatory = True,
        ),
    },
)
