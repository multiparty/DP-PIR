#!/bin/bash
CONFIG=experiments/dppir/config.json
TABLE=experiments/dppir/table.json

# Read args
machine_id=$1
client_id=$2
queries=$3
online="$4"
table_arg="--table=${TABLE}"
if [[ $online == "offline" ]]
then
  table_arg=""
fi

echo "Running ${online} client for ${machine_id} with ${queries}"
./bazel-bin/drivacy/client_${online} \
    --config=${CONFIG} ${table_arg} --machine=${machine_id} \
    --client=${client_id} --queries=${queries}
