# Copyright 2020 multiparty.org

# Contains rule for running a test that consists of many binary
# runs in parallel (as separate processes).

SCRIPT = """
# Create log directory
mkdir -p logs

# Store all spawned process ids here.
declare -a PARTY_IDS=()
declare -a CLIENT_IDS=()

# The final exit code
EXIT_CODE=0

# Run parties
echo "Running parties..."
for (( party={parties}; party>0; party-- ))
do
  for (( machine=1; machine<={parallelism}; machine++ ))
  do
    echo "Running party $party-$machine"
    ./{_party} --table={table} --config={config} \
                             --party=$party --machine=$machine \
                             --batch={batch} > logs/party$party-$machine.log 2>&1 &
    PARTY_IDS+=($!)
  done
done
echo ""

# Sleep to give time for websocket servers to be setup
sleep 5

# Run clients
echo "Running clients..."
for (( machine=1; machine<={parallelism}; machine++ ))
do
  echo "Running client $machine"
  ./{_client} --table={table} --config={config} --machine=$machine \
                              --queries={queries} > logs/client-$machine 2>&1 &
  CLIENT_IDS+=($!)
done
echo ""

# Wait for all clients
for ID in "${{CLIENT_IDS[@]}}"
do
  wait $ID
  code=$?
  if [ "$code" -ne "0" ]
  then
    EXIT_CODE=$code
  fi
done

# dump logs
echo "Dumping logs..."
for (( party={parties}; party>0; party-- ))
do
  for (( machine=1; machine<={parallelism}; machine++ ))
  do
    echo "Party $party-$machine logs"
    echo "=========================="
    cat logs/party$party-$machine.log
    echo "=========================="
    echo ""
  done
done

for (( machine=1; machine<={parallelism}; machine++ ))
do
  echo "Client $machine logs"
  echo "=========================="
  cat logs/client-$machine
  echo "=========================="
  echo ""
done


# Kill all parties
for ID in "${{PARTY_IDS[@]}}"
do
  kill -9 $ID &> /dev/null
done

exit $EXIT_CODE
"""

def _end_to_end_test_impl(ctx):
    ctx.actions.write(
        is_executable = True,
        output = ctx.outputs.executable,
        content = SCRIPT.format(
            _party=ctx.file._party.short_path,
            _client=ctx.file._client.short_path,
            parties=ctx.attr.parties,
            parallelism=ctx.attr.parallelism,
            batch=ctx.attr.batch,
            queries=ctx.attr.queries,
            table=ctx.file.table.short_path,
            config=ctx.file.config.short_path,
        ),
    )

    files = [ctx.file._party, ctx.file._client, ctx.file.table, ctx.file.config]
    return DefaultInfo(
        runfiles = ctx.runfiles(files = files),
    )

end_to_end_test = rule(
    doc = "Runs the entire protocol over TCP sockets and different processes.",
    implementation = _end_to_end_test_impl,
    test = True,
    attrs = {
        "_party": attr.label(
            doc = "Party main entry point.",
            mandatory = False,
            allow_single_file = True,
            default = "//drivacy:main",
        ),
        "_client": attr.label(
            doc = "Client main entry point.",
            mandatory = False,
            allow_single_file = True,
            default = "//drivacy:client",
        ),
        "table": attr.label(
            doc = "The path to table.json.",
            mandatory = True,
            allow_single_file = True,
        ),
        "config": attr.label(
            doc = "The path to config.json.",
            mandatory = True,
            allow_single_file = True,
        ),
        "parties": attr.int(
            doc = "The number of parties.",
            mandatory = True,
        ),
        "parallelism": attr.int(
            doc = "The number of machines per party.",
            mandatory = True,
        ),
        "batch": attr.int(
            doc = "The batch size.",
            mandatory = True,
        ),
        "queries": attr.int(
            doc = "The number of queries each client makes.",
            mandatory = True,
        ),
    },
)
