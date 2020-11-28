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
    if [[ "$1" == "--valgrind" ]]
    then
      valgrind ./{_party} --table={table} --config={config} --party=$party \
               --machine=$machine --batches={batches} --batch={batch} \
               --span={span} --cutoff={cutoff} \
               > logs/party$party-$machine.log 2>&1 &
      PARTY_IDS+=($!)
    else
      {_party} --table={table} --config={config} --party=$party \
               --machine=$machine --batches={batches} --batch={batch} \
               --span={span} --cutoff={cutoff} \
               > logs/party$party-$machine.log 2>&1 &
      PARTY_IDS+=($!)
    fi
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
  if [[ "$1" == "--valgrind" ]]
  then
    valgrind ./{_client} --table={table} --config={config} --machine=$machine \
             --queries={queries} > logs/client-$machine 2>&1 &
    CLIENT_IDS+=($!)
  else
    ./{_client} --table={table} --config={config} --machine=$machine \
                --queries={queries} > logs/client-$machine 2>&1 &
    CLIENT_IDS+=($!)
  fi
done
echo ""

# In the background, wait a certian timeout, then attempt to forcefully
# kill protocol if timeout is exceeded!
(
  sleep {max_time} &&
  echo "Timeout expired!" &&
  for ID in "${{CLIENT_IDS[@]}}"; do kill -9 $ID; done &&
  for ID in "${{PARTY_IDS[@]}}"; do kill -9 $ID; done &&
  echo "Killed all" &&
  echo "" &&
  exit 1
) &
TIMEOUT_PID=$!

# Wait for all clients and parties
for ID in "${{CLIENT_IDS[@]}}"
do
  # Use wait to get status code.
  wait $ID
  code=$?
  if [ "$code" -ne "0" ]
  then
    EXIT_CODE=$code
  fi
done
for ID in "${{PARTY_IDS[@]}}"
do
  wait $ID
  code=$?
  if [ "$code" -ne "0" ]
  then
    EXIT_CODE=$code
  fi
done

# in case we are done before timeout, kill the timeout task.
kill -9 $TIMEOUT_PID &> /dev/null
wait $! &> /dev/null

# dump logs
echo "Dumping logs..."
echo ""
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

# Wait to make sure everything died properly.
sleep 1

# Exit with the deduced exit code.
echo "Sucess: $EXIT_CODE"
exit $EXIT_CODE
"""

def _end_to_end_test_impl(ctx):
    # automatically compute count of batches if needed.
    batches = ctx.attr.batches
    if batches == 0:
      batches = ctx.attr.queries // ctx.attr.batch
    if batches * ctx.attr.batch != ctx.attr.queries:
      fail("Batches count, batch size, and queries are incompatible!")

    ctx.actions.write(
        is_executable = True,
        output = ctx.outputs.executable,
        content = SCRIPT.format(
            _party=ctx.file._party.short_path,
            _client=ctx.file._client.short_path,
            parties=ctx.attr.parties,
            parallelism=ctx.attr.parallelism,
            batches=batches,
            batch=ctx.attr.batch,
            queries=ctx.attr.queries,
            table=ctx.file.table.short_path,
            config=ctx.file.config.short_path,
            span=ctx.attr.span,
            cutoff=ctx.attr.cutoff,
            max_time=ctx.attr.max_time,
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
        "batches": attr.int(
            doc = "Count of batches for which the protocol is running.",
            mandatory = False,
            default = 0,
        ),
        "batch": attr.int(
            doc = "The batch size.",
            mandatory = True,
        ),
        "queries": attr.int(
            doc = "The number of queries each client makes.",
            mandatory = True,
        ),
        "span": attr.string(
            doc = "The span of the DP distribution",
            mandatory = True,
        ),
        "cutoff": attr.string(
            doc = "The cutoff for the DP distribution",
            mandatory = True,
        ),
        "max_time": attr.int(
            doc = "Test timeout in seconds.",
            mandatory = False,
            default = 60,
        ),
    },
)
