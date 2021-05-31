#!/bin/bash
CONFIG=experiments/dppir/config.json
TABLE=experiments/dppir/table.json

# Read args
party_id=$1
machine_id=$2
batch=$3
span=$4
cutoff=$5
online="$6"

echo "Running ${online} party for ${party_id}-${machine_id} with ${batch} ${span} ${cutoff}"
./bazel-bin/drivacy/party_${online} \
    --config=${CONFIG} --table=${TABLE} --party=${party_id} \
    --machine=${machine_id} --batch=${batch} --span=${span} --cutoff=${cutoff}
